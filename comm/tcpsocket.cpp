/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <time.h>

#include "tcpsocket.h"
#include "utils.h"

using namespace std;

namespace comm {
    int BasicSocket::Read(IOBuffer &oBuffer, size_t uBytes, uint32_t uTimeoutMs) {
       size_t uNotMatter;
       return ReadUntil(oBuffer, "", uBytes, uTimeoutMs, uNotMatter);
    }

    int BasicSocket::ReadUntil(IOBuffer &oBuffer, const string &sToFind, size_t uMaxBytes, uint32_t uTimeoutMs, size_t &uFindPos) {
        if (m_iFd <= 0) {
            return FD_INCORRECT;
        }

        if (uTimeoutMs == 0) {
            return READ_TIMEOUT;
        }

        if (uMaxBytes == 0) {
            return 0;
        }

        size_t uBufferStartPos = oBuffer.Size();
        size_t uBytesRead = 0;
        auto LoopWork = [&]() -> int {
            if (oBuffer.GetSpace() == 0) {
                return -4;
            }

            int iReadCnt = recv(m_iFd, oBuffer.GetCursor(), min(oBuffer.GetSpace(), (uint32_t) (uMaxBytes - uBytesRead)), 0);
            if (iReadCnt <= 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
                    // 说明Socket Buffer没数据，它可能正在读，也可能已经没数据了
                    // 一般我们会使用非阻塞模式，因此这里会循环多次读，为了避免cpu占用率过高，每次发现没数据就睡眠0.5ms
                    usleep(500);
                    return -2;
                }
                // 说明已经读失败了
                return -1;
            }
            uBytesRead += iReadCnt;
            oBuffer.MoveCursor(iReadCnt);

            // 每次读到新数据都Find一遍
            if (!sToFind.empty()) {
                uFindPos = oBuffer.Find(sToFind, uBufferStartPos + uBytesRead - iReadCnt);
                if (uFindPos != string::npos) {
                    return 0;
                }
            }

            // 已经读完了还没找到
            if (uBytesRead >= uMaxBytes) {
                return sToFind.empty() ? 0 : -5;
            }

            // 还没读完，也还没找到
            return -3;
        };

        uint64_t lStartMs = CurTimeMillis();
        while (true) {
            // 如果超时我们返回错误码
            if (uTimeoutMs > 0 && CurTimeMillis() - lStartMs > uTimeoutMs) {
                return READ_TIMEOUT;
            }

            int iRet = LoopWork();
            switch (iRet) {
                case 0:
                    return uBytesRead;
                case -1:
                    return READ_ERROR;
                case -4:
                    return READ_BUFFER_NOT_ENOUGH;
                case -5:
                    return READ_UNTIL_NOT_FOUND;
            }
        }
    }

    int BasicSocket::Write(const string &sContent, uint32_t uTimeoutMs) {
        if (m_iFd <= 0) {
            return FD_INCORRECT;
        }

        const char *pContent = sContent.c_str();
        uint32_t uBytesWrite = 0;
        auto LoopWork = [&]() -> int {
            int iWriteCnt = send(m_iFd, pContent + uBytesWrite, sContent.size() - uBytesWrite, 0);
            if (iWriteCnt <= 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
                    // 一般我们会使用非阻塞模式，因此这里会循环多次，为了避免cpu占用率过高，每次发现没数据就睡眠0.5ms
                    usleep(500);
                    return -2;
                }
                // 说明已经写失败了
                return -1;
            }

            uBytesWrite += iWriteCnt;
            return uBytesWrite >= sContent.size() ? 0 : -3;
        };

        uint64_t lStartMs = CurTimeMillis();
        while (true) {
            if (uTimeoutMs > 0 && CurTimeMillis() - lStartMs > uTimeoutMs) {
                return WRITE_TIMEOUT;
            }

            int iRet = LoopWork();
            switch (iRet) {
                case 0:
                    return 0;
                case -1:
                    return WRITE_ERROR;
            }
        }
    }

    sockaddr_in BasicSocket::GetSockAddr() const {
        return m_oSockAddr;
    }

    void BasicSocket::SetNonBlocking() {
        if (m_iFd <= 0) {
            return;
        }
        int iFlags = 0;
        fcntl(m_iFd, F_GETFL, iFlags);
        fcntl(m_iFd, F_SETFL, iFlags | O_NONBLOCK);
    }

    BasicSocket::BasicSocket(int iFd, const sockaddr_in &oSockAddr) {
        m_iFd = iFd;
        m_oSockAddr = oSockAddr;
    }

    BasicSocket::~BasicSocket() {
        if (m_iFd > 0) {
            close(m_iFd);
            m_iFd = 0;
        }
    }

    int Socket::Create(const char *sIp, uint16_t uPort, shared_ptr<Socket> &pSocket) {
        // TODO 默认ipv4, tcp
        int iFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (iFd == -1) {
            return SOCKET_CREATE_ERROR;
        }

        sockaddr_in oSockAddr;
        oSockAddr.sin_port = htons(uPort);
        oSockAddr.sin_family = AF_INET;
        if (inet_pton(AF_INET, sIp, &oSockAddr.sin_addr) < 0) {
            return IP_INCORRECT;
        }
        pSocket = shared_ptr<Socket>(new Socket(iFd, oSockAddr));
        return 0;
    }

    int Socket::Connect() {
        if (m_iFd <= 0) {
            return FD_INCORRECT;
        }

        int iRet = connect(m_iFd, (sockaddr *) &m_oSockAddr, sizeof(m_oSockAddr));
        if (iRet) {
            return CONNECT_ERROR;
        }
        return 0;
    }

    Socket::Socket(int iFd, const sockaddr_in &oSockAddr) : BasicSocket(iFd, oSockAddr) {
        // 设置socketopt后，尽管在阻塞模式下读写超时后也能跳出不傻等，不过不是所有机器都支持
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500;
        setsockopt(m_iFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(m_iFd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    }

    int ServerSocket::Create(const char *sIp, uint16_t uPort, shared_ptr<ServerSocket> &pSvrSocket) {
        // TODO 默认ipv4, tcp
        int iFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (iFd == -1) {
            return SOCKET_CREATE_ERROR;
        }

        sockaddr_in oSockAddr;
        oSockAddr.sin_port = htons(uPort);
        oSockAddr.sin_family = AF_INET;
        if (inet_pton(AF_INET, sIp, &oSockAddr.sin_addr) < 0) {
            return IP_INCORRECT;
        }

        int iRet = bind(iFd, (sockaddr *) &oSockAddr, sizeof(oSockAddr));
        if (iRet) {
            close(iFd);
            return BIND_ERROR;
        }
        pSvrSocket = shared_ptr<ServerSocket>(new ServerSocket(iFd, oSockAddr));
        return 0;
    }

    int ServerSocket::Create(uint16_t uPort, std::shared_ptr<ServerSocket> &pSvrSocket) {
        return Create("0.0.0.0", uPort, pSvrSocket);
    }

    int ServerSocket::Listen(uint32_t uQueueSize) {
        if (m_iFd <= 0) {
            return FD_INCORRECT;
        }

        if (m_bIsListening) {
            return SVR_IS_LISTENING_ALREADY;
        }

        int iRet = listen(m_iFd, uQueueSize);
        if (iRet != 0) {
            return LISTEN_ERROR;
        }
        m_bIsListening = true;
        return 0;
    }

    int ServerSocket::Accept(std::shared_ptr<Socket> &pSocket) {
        Socket *pRawSocket = nullptr;
        int iRet = Accept(pRawSocket);
        if (iRet) {
            return iRet;
        }
        pSocket = shared_ptr<Socket>(pRawSocket);
        return 0;
    }

    int ServerSocket::Accept(Socket *&pSocket) {
        if (m_iFd <= 0) {
            return FD_INCORRECT;
        }

        if (!m_bIsListening) {
            return SVR_IS_NOT_LISTENING;
        }

        sockaddr_in oAddr;
        socklen_t tAddr;
        int iConnFd = accept(m_iFd, (sockaddr *) &oAddr, &tAddr);
        if (iConnFd == -1) {
            return ACCEPT_ERROR;
        }
        pSocket = new Socket(iConnFd, oAddr);
        return 0;
    }

    ServerSocket::ServerSocket(int iFd, const sockaddr_in &oSockAddr) : BasicSocket(iFd, oSockAddr) {
        m_bIsListening = false;
    }
}