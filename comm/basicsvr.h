/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/
#pragma once
#include <iostream>
#include <memory>
#include <signal.h>

#include "tcpsocket.h"

namespace comm {

    struct BasicSvrOptions {
        uint16_t m_uPort;
        uint32_t m_uQueueSize;
    };

    class BasicSvrHandler {
    public:
        virtual void Handle(Socket *pSocket) = 0;
        virtual ~BasicSvrHandler() {}
    };

    class BasicSvr {
    public:
        BasicSvr(const BasicSvrOptions &oOpts, BasicSvrHandler *pHandler);

        /**
         * @return 0:正常
         */
        int Start();

        /**
         * 如果服务器已经启动了，该函数无效
         * @param uPort
         */
        void SetPort(uint16_t uPort);

    private:
        void SetSignalHandlers();

    private:
        bool m_bIsStarted;
        BasicSvrOptions m_oOpts;
        BasicSvrHandler *m_pHandler;
    };
}