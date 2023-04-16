/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/
#include "timecounter.h"

namespace comm {
    TimeCounter::TimeCounter() {
        m_uStartMs = 0;
        m_uDurationMs = 0;
        m_bIsCounting = false;
    }

    void TimeCounter::Start() {
        m_bIsCounting = true;
        m_uStartMs = CurTimeMillis();
    }

    void TimeCounter::End() {
        if (!m_bIsCounting) {
            return;
        }
        m_uDurationMs += CurTimeMillis() - m_uStartMs;
        m_bIsCounting = false;
    }

    uint64_t TimeCounter::GetDurationMs() const {
        if (!m_bIsCounting) {
            return m_uDurationMs;
        }
        return m_uDurationMs + CurTimeMillis() - m_uStartMs;
    }
}