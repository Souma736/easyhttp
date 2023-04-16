/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/
#pragma once
#include <unordered_map>

#include "tcpsocket.h"
#include "iobuffer.h"
#include "timecounter.h"

namespace comm {

    struct HttpProcessorOptions {
        uint32_t m_uWriteTimeoutMs = 3000;
        uint32_t m_uReadTimeoutMs = 3000;
        uint32_t m_uInitBuffSize = 4096;
        uint32_t m_uMaxBuffSize = 10 * 1024 * 1024;
        uint32_t m_uMaxRequestLineBytes = 8192;
        uint32_t m_uMaxHeadersBytes = 8192 * 64;
        uint32_t m_uMaxBodyBytes = 2 * 1024 * 1024;
    };

    class BasicHttpProcessor {
    public:
        BasicHttpProcessor(std::shared_ptr<Socket> pSocket, const HttpProcessorOptions &oOpts);
        int Process();

    protected:
        /**
         * @return 0:业务正常
         */
        virtual int Logic() = 0;

        /**
         * 这个Hook函数的适用于，某Url Post方法耗时很长，需要调整HttpProcessorOptions防止数据超限制或超时
         * 示例: if (m_sUrl == "/postbigdata") m_oOpts.m_uReadTimeoutMs = 60000, m_oOpts.m_uMaxBodyBytes = 1024*1024*1024;
         */
        virtual void BeforeReadPostBody() {}

        int ParseRequest();
        int ParseRequestLine();
        int ParseRequestHeaders();
        int ParseSingleHeader(const std::string &sHeader);
        int SendResponse();
        void SetHeader(const std::string &sKey, const std::string &sValue);
        void SetStatus(int iStatusCode, const std::string &sStatus);
        int BadRequestHandle();

    protected:
        HttpProcessorOptions m_oOpts;
        TimeCounter m_oRequestTimeCostCounter;

        std::shared_ptr<Socket> m_pSocket;
        std::shared_ptr<IOBuffer> m_pBuffer;

        std::string m_sMethod;
        std::string m_sProtocol;
        std::string m_sRequestUrl;
        std::string m_sRequestBody;

        size_t m_uFirstCRLFPos;
        size_t m_uDoubleCRLFPos;

        std::unordered_map<std::string, std::string> m_mapRequestHeaders;
        std::unordered_map<std::string, std::string> m_mapResponseHeaders;
        std::string m_sResponseBody;
        int m_iHttpStatusCode;
        std::string m_sHttpStatus;
    };
};