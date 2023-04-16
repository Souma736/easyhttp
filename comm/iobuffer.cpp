/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/
#include <string.h>

#include "iobuffer.h"

using namespace std;

namespace comm {
    IOBuffer::IOBuffer(uint32_t uBufSize, uint32_t uMaxBufSize) {
        uBufSize = max(1u, uBufSize);
        uMaxBufSize = max(uBufSize, uMaxBufSize);
        m_uBufSize = uBufSize;
        m_uOffset = 0;
        m_uMaxBufSize = uMaxBufSize;
        m_pBuf = (char *) malloc(uBufSize);
    }

    char * IOBuffer::GetCursor() const {
        return m_pBuf + m_uOffset;
    }

    uint32_t IOBuffer::GetSpace() const {
        return m_uBufSize - m_uOffset;
    }

    uint32_t IOBuffer::Size() const {
        return m_uOffset;
    }

    void IOBuffer::MoveCursor(uint32_t uOffset) {
        if (uOffset > m_uBufSize - m_uOffset) {
            m_uOffset = m_uBufSize;
        } else {
            m_uOffset += uOffset;
        }

        if (m_uOffset < (m_uBufSize >> 1) || m_uBufSize == m_uMaxBufSize) {
            // 不需要扩容
            return;
        }

        // 避免溢出
        if (m_uMaxBufSize / m_uBufSize < 4) {
            Expand(m_uMaxBufSize);
            return;
        }
        Expand(m_uBufSize << 2);
    }

    string IOBuffer::GetString(size_t uStartPos, size_t uEndPos) const {
        if (uStartPos >= uEndPos || uStartPos >= m_uOffset) {
            return "";
        }
        uEndPos = min(uEndPos, (size_t) m_uOffset);
        return string(m_pBuf + uStartPos, uEndPos - uStartPos);
    }

    void IOBuffer::Expand(uint32_t uNewBufSize) {
        if (uNewBufSize <= m_uBufSize) {
            return;
        }
        char *pNewBuf = (char *)malloc(uNewBufSize);
        memcpy(pNewBuf, m_pBuf, m_uOffset);
        delete m_pBuf;
        m_pBuf = pNewBuf;
        m_uBufSize = uNewBufSize;
    }

    void IOBuffer::GetNextArr(const string &sTarget, size_t *pNextArr) {
        if (sTarget.size() < 2) {
            return;
        }

        const char *pArr = sTarget.c_str();
        pNextArr[0] = pNextArr[1] = 0;
        size_t uComparePos = 0;
        size_t uArrPos = 2;
        while (uArrPos <= sTarget.size()) {
            if (pArr[uArrPos - 1] == pArr[uComparePos]) {
                pNextArr[uArrPos++] = ++uComparePos;
            } else if (uComparePos == 0) {
                pNextArr[uArrPos++] = 0;
            } else {
                uComparePos = pNextArr[uComparePos];
            }
        }
    }

    size_t IOBuffer::Find(const string &sToFind, size_t uStartPos) const {
        if (uStartPos >= m_uOffset || sToFind.empty() || sToFind.size() > m_uOffset - uStartPos) {
            return string::npos;
        }

        size_t pNextArr[sToFind.size() + 1];
        GetNextArr(sToFind, pNextArr);

        const char *pSrc = m_pBuf;
        const char *pToFind = sToFind.c_str();
        size_t uSrcCursor = uStartPos, uTargetCursor = 0;
        while (uSrcCursor < m_uOffset && uTargetCursor < sToFind.size()) {
            if (pSrc[uSrcCursor] == pToFind[uTargetCursor]) {
                uSrcCursor++;
                uTargetCursor++;
            } else if (uTargetCursor == 0) {
                uSrcCursor++;
            } else {
                uTargetCursor = pNextArr[uTargetCursor];
            }
        }

        if (uTargetCursor == sToFind.size()) {
            return uSrcCursor - uTargetCursor;
        }
        return string::npos;
    }

    IOBuffer::~IOBuffer() {
        delete m_pBuf, m_pBuf = nullptr;
    }
}