/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/

#include "comm/basicsvr.h"
#include "comm/httpprocessor.h"
#include "comm/utils.h"

using namespace std;
using namespace comm;

class MyHttpProcessor : public BasicHttpProcessor {
public:
    MyHttpProcessor(shared_ptr<Socket> pSocket, const HttpProcessorOptions &oOpts) : BasicHttpProcessor(pSocket, oOpts) {}
    int Logic() {
        // 写一下我们的业务逻辑，比如直接返回200 ok
        LOG("处理来自<%s:%u>的[%s]请求", inet_ntoa(m_pSocket->GetSockAddr().sin_addr), ntohs(m_pSocket->GetSockAddr().sin_port), m_sRequestUrl.c_str());
        SetStatus(200, "ok");
        SetHeader("Content-Type", "text/html; charset=UTF-8");
        m_sResponseBody = "你请求方法是:" + m_sMethod + ", 路径是:" + m_sRequestUrl;
        return 0;
    }
};

class HttpSvrHandler : public BasicSvrHandler {
    void Handle(Socket *pRawSocket) {
        // 这样就不需要手动释放Socket，该作用域结束后自动close
        auto pSocket = shared_ptr<Socket>(pRawSocket);
        // oOpts可自定义最大长度，超时时间等。这里直接用默认值。
        HttpProcessorOptions oOpts;
        // 一个HTTP连接对应一个HttpProcessor
        // 这里简单示例，使用串行调用，无法充分利用CPU。实际生产中应该用线程池、协程池。
        MyHttpProcessor oProcessor(pSocket, oOpts);
        int iRet = oProcessor.Process();
        if (iRet) {
            LOG("HTTP请求处理异常, iRet[%d]", iRet);
        }
    }
};

int main() {
    BasicSvrOptions oSvrOpts;
    oSvrOpts.m_uPort = 7777;
    oSvrOpts.m_uQueueSize = 1024;
    HttpSvrHandler oHandler;
    BasicSvr oSvr(oSvrOpts, &oHandler);
    LOG("服务器启动中，端口[%u]", oSvrOpts.m_uPort);
    // 调用栈 BasicSvr -> HttpSvrHandler -> MyHttpProcessor
    int iRet = oSvr.Start();
    if (iRet) {
        LOG("服务器启动失败，端口[%u]", oSvrOpts.m_uPort);
    }
    return 0;
}