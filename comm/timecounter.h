/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/
#pragma once

#include "utils.h"

namespace comm {
    class TimeCounter {
    public:
        TimeCounter();

        /**
         * 可重复Start，以最后一次设定为准
         */
        void Start();

        /**
         * 只有在Start了之后才会有效增加duration
         */
        void End();

        /**
         * @return 正在计时返回当前经过时间，不在计时返回上次经过时间
         */
        uint64_t GetDurationMs() const;

    private:
        uint64_t m_uStartMs;
        uint64_t m_uDurationMs;
        bool m_bIsCounting;
    };
}