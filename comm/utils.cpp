/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/
#include "utils.h"
#include <iostream>
#include <stdarg.h>

using namespace std;
namespace comm {
    uint64_t CurTimeMillis() {
        timeval val;
        gettimeofday(&val, nullptr);
        return val.tv_sec * 1000 + val.tv_usec / 1000;
    }

    string GetDateTimeString() {
        std::time_t tt = std::time(nullptr);
        std::tm *now = std::localtime(&tt);
        timeval tv;
        gettimeofday(&tv, nullptr);
        char cs[26];
        snprintf(cs, 26, "<%04d-%02d-%02d %02d:%02d:%02d %03d>", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, (int)(tv.tv_usec/1000));
        return cs;
    }
}