/**
* @author 今天又写BUG了
* @date 2023-04-16 12:00:00
*/
#pragma once
#include <unistd.h>
#include <ctime>
#include <sys/time.h>
#include <iostream>

#define LOG(fmt, ...) \
    {char __pc[1024 + 50]; snprintf(__pc, 1024 + 50, "%s <%s:%d->%s> " fmt, GetDateTimeString().c_str(), __FILE__, __LINE__, __func__, ##__VA_ARGS__); printf("%s\n", __pc);}


namespace comm {
    /**
     * @return 当前时间戳(秒级)
     */
    uint64_t CurTimeMillis();

    /**
     * @return 返回格式为 "<2023-03-22 21:10:00 023>" 的字符串
     */
    std::string GetDateTimeString();
}