/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/
#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "iobuffer.h"

namespace comm {

    enum TCP_SOCKET_ERR_CODE {
        READ_ERROR = -100,
        READ_TIMEOUT = -101,
        FD_INCORRECT = -102,
        IP_INCORRECT = -103,
        SOCKET_CREATE_ERROR = -104,
        CONNECT_ERROR = -105,
        BIND_ERROR = -106,
        SVR_IS_LISTENING_ALREADY = -107,
        LISTEN_ERROR = -108,
        SVR_IS_NOT_LISTENING = -109,
        ACCEPT_ERROR = -110,
        WRITE_ERROR = -111,
        WRITE_TIMEOUT = -112,
        READ_BUFFER_NOT_ENOUGH = -113,
        READ_UNTIL_NOT_FOUND = -114,
    };

    class BasicSocket {
    public:

        /**
         *
         * @param oBuffer 若该buffer最大可写入空间小于uBytes会截断并返回错误码
         * @param uBytes 想从Socket Buffer中读取的字节数
         * @param uTimeoutMs 超时时间，0则永不超时
         * @return >=0: 读到oBuffer的字节数，其他: 参考TCP_SOCKET_ERR_CODE
         */
        int Read(IOBuffer &oBuffer, size_t uBytes, uint32_t uTimeoutMs);

        /**
         *
         * @param oBuffer
         * @param sToFind 需要读到的字符串
         * @param uMaxBytes 最多读多少个字节
         * @param uTimeoutMs 超时时间，0则永不超时
         * @param uFindPos sToFind在oBuffer中的绝对偏移，如果return > 0则保证有效，否则不保证
         * @return >=0: 读到oBuffer的字节数， 其他: 参考TCP_SOCKET_ERR_CODE
         */
        int ReadUntil(IOBuffer &oBuffer, const std::string &sToFind, size_t uMaxBytes, uint32_t uTimeoutMs, size_t &uFindPos);

        /**
         *
         * @param sContent 想写入Socket Buffer的内容，如果写超时会截断并返回异常
         * @param uTimeoutMs 超时时间，0则永不超时
         * @return 0:正常 其他:参考TCP_SOCKET_ERR_CODE
         */
        int Write(const std::string &sContent, uint32_t uTimeoutMs);

        /**
         * @return sockaddr_in
         */
        sockaddr_in GetSockAddr() const;

        /**
         * 设置非阻塞模式
         */
        void SetNonBlocking();

        BasicSocket(const BasicSocket &oSocket) = delete;
        BasicSocket & operator=(const BasicSocket &oSocket) = delete;
        
    protected:
        BasicSocket(int iFd, const sockaddr_in &oSockAddr);
        ~BasicSocket();
        
    protected:
        int m_iFd = 0;
        sockaddr_in m_oSockAddr;
    };


    class Socket : public BasicSocket {
    public:
        friend class ServerSocket;

        /**
         * @param sIp
         * @param uPort
         * @param pSocket
         * @return 0:正常 其他:参考TCP_SOCKET_ERR_CODE
         */
        static int Create(const char *sIp, uint16_t uPort, std::shared_ptr<Socket> &pSocket);

        /**
         * @return 0:正常 其他:参考TCP_SOCKET_ERR_CODE
         */
        int Connect();

    private:
        Socket(int iFd, const sockaddr_in &oSockAddr);
    };

    class ServerSocket : public BasicSocket {
    public:

        /**
         * @param sIp
         * @param uPort
         * @param pSvrSocket
         * @return 0:正常 其他:参考TCP_SOCKET_ERR_CODE
         */
        static int Create(const char *sIp, uint16_t uPort, std::shared_ptr<ServerSocket> &pSvrSocket);

        /**
         * 默认绑定ip 0.0.0.0
         * @param uPort
         * @param pSvrSocket
         * @return 0:正常 其他:参考TCP_SOCKET_ERR_CODE
         */
        static int Create(uint16_t uPort, std::shared_ptr<ServerSocket> &pSvrSocket);

        /**
         * @param uQueueSize 全连接池队列Size，决定了最大同时有多少个ESTABLISHED但未accept的连接
         * @return 0:正常 其他:参考TCP_SOCKET_ERR_CODE
         */
        int Listen(uint32_t uQueueSize);

        /**
         * @param pSocket
         * @return 0:正常 其他:参考TCP_SOCKET_ERR_CODE
         */
        int Accept(std::shared_ptr<Socket> &pSocket);

        /**
         * @param pSocket 这个指针需要自己delete
         * @return 0:正常 其他:参考TCP_SOCKET_ERR_CODE
         */
        int Accept(Socket *&pSocket);

    private:
        ServerSocket(int iFd, const sockaddr_in &oSockAddr);
    private:
        bool m_bIsListening;
    };
}