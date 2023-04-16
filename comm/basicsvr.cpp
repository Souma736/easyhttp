/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/
#include "basicsvr.h"
#include "utils.h"

using namespace std;

namespace comm {

    BasicSvr::BasicSvr(const comm::BasicSvrOptions &oOpts, BasicSvrHandler *pHandler) {
        m_oOpts = oOpts;
        m_bIsStarted = false;
        m_pHandler = pHandler;
        SetSignalHandlers();
    }

    int BasicSvr::Start() {
        if (m_bIsStarted) {
            return -1;
        }

        // 创建一个Socket，并绑定端口
        shared_ptr<ServerSocket> pSvrSocket = nullptr;
        int iRet = ServerSocket::Create(m_oOpts.m_uPort, pSvrSocket);
        if (iRet) {
            LOG("创建或绑定端口失败, iRet[%d], errno[%d]", iRet, errno);
            return -2;
        }

        // 监听端口
        iRet = pSvrSocket->Listen(m_oOpts.m_uQueueSize);
        if (iRet) {
            LOG("listen error, iRet[%d], errno[%d]", iRet, errno);
            return -3;
        }

        m_bIsStarted = true;
        while (true) {
            Socket *pCliSocket = nullptr;
            iRet = pSvrSocket->Accept(pCliSocket);
            if (iRet) {
                LOG("accept error, iRet[%d], errno[%d]", iRet, errno);
                continue;
            }

            if (m_pHandler == nullptr) {
                LOG("handler is nullptr");
                exit(-1);
            }
            // socket交给handler处理
            m_pHandler->Handle(pCliSocket);
        }
    }

    void BasicSvr::SetPort(uint16_t uPort) {
        if (m_bIsStarted) {
            return;
        }
        m_oOpts.m_uPort = uPort;
    }

    void BasicSvr::SetSignalHandlers() {
        // 在往已close的socket中写入数据时，进程会收到SIGPIPE, Broken pipe信号。
        // 默认行为是进程退出，所以这里我们需要让它保持静默不退出。
        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &sa, nullptr);
        // for perf top -p
        sigaction(SIGTRAP, &sa, nullptr);
    }
}