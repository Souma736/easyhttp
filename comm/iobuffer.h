/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/
#pragma once
#include <iostream>

namespace comm {
    class IOBuffer {
    public:
        /**
         * @param uBufSize 初始缓存大小, 不会小于1字节。默认1024字节
         * @param uMaxBufSize 最大缓存大小，不会小于uBufSize。默认2^28-1，用满时内存占500MB
         */
        IOBuffer(uint32_t uBufSize = 1024, uint32_t uMaxBufSize = -1u >> 4);

        /**
         * @return 获得当前指针位置
         */
        char * GetCursor() const;

        /**
         * @return 获得剩余可写字节数, 返回0说明该Buffer已经写满
         */
        uint32_t GetSpace() const;

        /**
         * @return 已写入字符的Size，非缓存大小
         */
        uint32_t Size() const;

        /**
         * @param uOffset 把当前指针后移uOffset个字节，越界则截断
         * @return 0:正常, -1:越界
         */
        void MoveCursor(uint32_t uOffset);

        /**
         * @param uStartPos 起始位置，越界则返空
         * @param uEndPos 终止位置，若小于uStartPos则返空，越界则截断
         * @return 获得string实例
         */
        std::string GetString(size_t uStartPos = 0, size_t uEndPos = -1) const;

        /**
         * @param sToFind 需要寻找字符串，为空则返回string::npos
         * @param uStartPos 开始寻找的pos，越界则返回string::npos
         * @return 如果没找着返回string::npos
         */
        size_t Find(const std::string &sToFind, size_t uStartPos = 0) const;

        ~IOBuffer();

        IOBuffer & operator=(const IOBuffer &oBuffer) = delete;

        IOBuffer(const IOBuffer &oBuffer) = delete;

        bool operator==(const IOBuffer &ioBuffer) = delete;

    private:
        void Expand(uint32_t uNewBufSize);
        static void GetNextArr(const std::string &sTarget, size_t *pNextArr);

    private:
        char *m_pBuf;
        uint32_t m_uBufSize;
        uint32_t m_uOffset;
        uint32_t m_uMaxBufSize;
    };
}