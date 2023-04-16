/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/
#include "httpprocessor.h"
#include "utils.h"


using namespace std;

namespace comm {

    BasicHttpProcessor::BasicHttpProcessor(shared_ptr<Socket> pSocket, const HttpProcessorOptions &oOpts) {
        m_oOpts = oOpts;
        m_pSocket = pSocket;
        // 尽量使用非阻塞模式，阻塞时会被恶意占用资源
        m_pSocket->SetNonBlocking();
        // 创建Buffer
        m_pBuffer = make_shared<IOBuffer>(m_oOpts.m_uInitBuffSize, m_oOpts.m_uMaxBuffSize);
    }

    int BasicHttpProcessor::Process() {
        int iRet = ParseRequest();
        if (iRet) {
            LOG("解析请求失败, iRet[%d]", iRet);
            return BadRequestHandle();
        }

        iRet = Logic();
        if (iRet) {
            LOG("逻辑错误, iRet[%d]", iRet);
            return iRet;
        }

        iRet = SendResponse();
        if (iRet) {
            LOG("响应失败, iRet[%d]", iRet);
            return iRet;
        }
        return 0;
    }

    int BasicHttpProcessor::ParseRequest() {
        int iRet = ParseRequestLine();
        if (iRet) {
            LOG("解析RequestLine错误, iRet[%d]", iRet);
            return 2;
        }

        iRet = ParseRequestHeaders();
        if (iRet) {
            LOG("解析headers错误, iRet[%d]", iRet);
            return 3;
        }

        if (m_sMethod != "POST") {
            return 0;
        }

        // 对于GET方法而言，读取数据就到此为止了。对于POST而言，还需要再按header中的content-length读取
        // 在指示数据包大小方面，有两种方式，1:Transfer-Encoding，2:Content-Length。一般HTTP请求用2，这里偷个懒只实现2。
        // 这里content-length小写是因为上面处理的时候转了小写
        auto it = m_mapRequestHeaders.find("content-length");
        if (it == m_mapRequestHeaders.end()) {
            LOG("请求headers中没有content-length, input[%s]", m_pBuffer->GetString().c_str());
            return 4;
        }

        // if you want to change the m_oOpts, now it's the time
        BeforeReadPostBody();

        uint64_t uContentLength = strtoull(it->second.c_str(), nullptr, 10);
        if (uContentLength >= m_oOpts.m_uMaxBodyBytes) {
            LOG("请求headers中content-length范围错误, content_len[%lu], max_body_bytes[%u]", uContentLength, m_oOpts.m_uMaxBodyBytes);
            return 5;
        }

        size_t uBodyBytesRead = m_pBuffer->Size() - m_uDoubleCRLFPos - 4;
        size_t uBytesLeft2Read = uContentLength > uBodyBytesRead ? uContentLength - uBodyBytesRead : 0;
        if (uBytesLeft2Read > 0) {
            uint64_t uTimeLeftToRead = m_oOpts.m_uReadTimeoutMs > m_oRequestTimeCostCounter.GetDurationMs() ? m_oOpts.m_uReadTimeoutMs - m_oRequestTimeCostCounter.GetDurationMs() : 0;
            m_oRequestTimeCostCounter.Start();
            iRet = m_pSocket->Read(*m_pBuffer, uBytesLeft2Read, uTimeLeftToRead);
            if (iRet < 0) {
                LOG("Socket读取错误, iRet[%d], errno[%d]", iRet, errno);
                return 6;
            }
            m_oRequestTimeCostCounter.End();
        }
        m_sRequestBody = m_pBuffer->GetString(m_uDoubleCRLFPos + 4);
        return 0;
    }

    int BasicHttpProcessor::ParseRequestLine() {
        m_oRequestTimeCostCounter.Start();
        int iRet = m_pSocket->ReadUntil(*m_pBuffer, "\r\n", m_oOpts.m_uMaxRequestLineBytes, m_oOpts.m_uReadTimeoutMs,m_uFirstCRLFPos);
        if (iRet < 0) {
            LOG("Socket读取错误, iRet[%d], errno[%d], content[%s]", iRet, errno, m_pBuffer->GetString().c_str());
            return 1;
        }
        m_oRequestTimeCostCounter.End();

        // 找到第一个空格
        size_t uFirstSpacePos = m_pBuffer->Find(" ");
        if (uFirstSpacePos == string::npos) {
            LOG("RequestLine中没空格, content[%s]", m_pBuffer->GetString().c_str());
            return 2;
        }

        // 解析Method，暂时只支持GET和POST方法
        m_sMethod = m_pBuffer->GetString(0, uFirstSpacePos);
        if (m_sMethod != "GET" && m_sMethod != "POST") {
            LOG("请求http方法不支持, method[%s]", m_sMethod.c_str());
            return 3;
        }

        // 找到第二个空格
        size_t uSecondSpacePos = m_pBuffer->Find(" ", uFirstSpacePos + 1);
        if (uSecondSpacePos == string::npos) {
            LOG("RequestLine缺少空格, content[%s]", m_pBuffer->GetString().c_str());
            return 4;
        }

        // 确保\r\n在第二个空格后面
        if (m_uFirstCRLFPos <= uSecondSpacePos + 1) {
            LOG("\\r\\n位置错误, content[%s]", m_pBuffer->GetString().c_str());
            return 5;
        }

        m_sRequestUrl = m_pBuffer->GetString(uFirstSpacePos + 1, uSecondSpacePos);
        m_sProtocol = m_pBuffer->GetString(uSecondSpacePos + 1, m_uFirstCRLFPos);
        if (m_sProtocol != "HTTP/1.1" && m_sProtocol != "HTTP/1.0") {
            LOG("协议不支持, protocol[%s]", m_sProtocol.c_str());
            return 5;
        }
        return 0;
    }

    int BasicHttpProcessor::ParseRequestHeaders() {
        int iRet = 0;
        // 找到\r\n\r\n，找不到的话再读一次
        m_uDoubleCRLFPos = m_pBuffer->Find("\r\n\r\n", m_uFirstCRLFPos + 2);
        if (m_uDoubleCRLFPos == string::npos) {
            uint64_t uTimeLeftToRead = m_oOpts.m_uReadTimeoutMs > m_oRequestTimeCostCounter.GetDurationMs() ? m_oOpts.m_uReadTimeoutMs - m_oRequestTimeCostCounter.GetDurationMs() : 0;
            m_oRequestTimeCostCounter.Start();
            iRet = m_pSocket->ReadUntil(*m_pBuffer, "\r\n\r\n", m_oOpts.m_uMaxHeadersBytes - (m_pBuffer->Size() - m_uFirstCRLFPos - 2), uTimeLeftToRead, m_uDoubleCRLFPos);
            if (iRet < 0) {
                LOG("Socket读取错误, iRet[%d], errno[%d]", iRet, errno);
                return 2;
            }
            m_oRequestTimeCostCounter.End();
        }

        // 循环解析每个Header
        size_t uLeftPos = m_uFirstCRLFPos + 2, uRightPos = m_pBuffer->Find("\r\n", uLeftPos);
        while (uRightPos != string::npos && uRightPos <= m_uDoubleCRLFPos) {
            // 解析单个Header
            iRet = ParseSingleHeader(m_pBuffer->GetString(uLeftPos, uRightPos));
            if (iRet) {
                LOG("解析单个header失败, iRet[%d]", iRet);
                return 4;
            }
            uLeftPos = uRightPos + 2;
            uRightPos = m_pBuffer->Find("\r\n", uLeftPos);
        }
        return 0;
    }

    int BasicHttpProcessor::ParseSingleHeader(const std::string &sHeader) {
        if (sHeader.empty()) {
            return 1;
        }
        // header是 "key: value"的形式，分界线是":"
        // 根据RFC2616规定(HTTP/1.1)，key不区分大小写，value前可有任意个连续空格，但推荐一个。
        // 因此这里注意key判断时要ignore case，并且value要trim空格方便后续处理
        size_t uPos = sHeader.find(":");
        if (uPos == string::npos || uPos == sHeader.size() - 1 || uPos == 0) {
            // 没有":" 或 以":"开头 或 以":"结尾
            LOG("header格式错误, header[%s]", sHeader.c_str());
            return 2;
        }

        // 这里手动循环赋值header是为了让key变全小写，用于判重复
        // 另一个原因是检查下有没空格，这里不允许空格
        string sKey = string(uPos, '\0');
        for (size_t u = 0; u < uPos; ++u) {
            if (sHeader[u] == ' ') {
                LOG("header格式错误, header[%s]", sHeader.c_str());
                return 3;
            }
            sKey[u] = tolower(sHeader[u]);
        }

        size_t uValueStartPos = sHeader.find_first_not_of(' ', uPos + 1);
        if (uValueStartPos == string::npos) {
            LOG("header格式错误, header[%s]", sHeader.c_str());
            return 4;
        }

        // 根据RFC2616，重复key的value是拼接在一起的，例如下面1+2=3
        // 1. cookie: A
        // 2. cookie: B
        // 3. cookie: A, B
        string sValue = sHeader.substr(uValueStartPos);
        auto it = m_mapRequestHeaders.find(sKey);
        if (it == m_mapRequestHeaders.end()) {
            m_mapRequestHeaders[sKey] = sValue;
        } else {
            m_mapRequestHeaders[sKey] += ", " + sValue;
        }
        return 0;
    }

    int BasicHttpProcessor::SendResponse() {

        // 组装Response Line，格式是 [protocol] [code] [status]。例如 HTTP/1.1 200 ok
        string sResponseLine = m_sProtocol + " " + to_string(m_iHttpStatusCode) + " " + m_sHttpStatus;
        string sResponseHeaders;

        // 组装响应Headers
        for (auto it = m_mapResponseHeaders.cbegin(); it != m_mapResponseHeaders.cend(); ++it) {
            sResponseHeaders += it->first + ": " + it->second + "\r\n";
        }

        // 告诉客户端body长度
        sResponseHeaders += "Content-Length: " + to_string(m_sResponseBody.size()) + "\r\n";

        // 组装所有要发送的信息并发送
        string sHtmlOutput = sResponseLine + "\r\n" + sResponseHeaders + "\r\n" + m_sResponseBody;
        int iRet = m_pSocket->Write(sHtmlOutput, m_oOpts.m_uWriteTimeoutMs);
        if (iRet) {
            LOG("Socket写入失败, iRet[%d], errno[%d]", iRet, errno);
            return iRet;
        }
        return 0;
    }

    void BasicHttpProcessor::SetHeader(const std::string &sKey, const std::string &sValue) {
        m_mapResponseHeaders[sKey] = sValue;
    }

    void BasicHttpProcessor::SetStatus(int iStatusCode, const std::string &sStatus) {
        m_iHttpStatusCode = iStatusCode;
        m_sHttpStatus = sStatus;
    }

    int BasicHttpProcessor::BadRequestHandle() {
        SetStatus(400, "bad request");
        m_sResponseBody = "";
        return SendResponse();
    }

}
