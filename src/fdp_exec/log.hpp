#pragma once

#include <fmt/format.h>
#include <loguru.hpp>

#ifndef FDP_MODULE
#    error "missing FDP_MODULE define"
#endif

#define LOG(INFO, FMT, ...)                               \
    do                                                    \
    {                                                     \
        LOG_F(INFO, "%s", fmt::format(FMT_STRING(FDP_MODULE ": " FMT), ##__VA_ARGS__).data()); \
    } while(0)

#define FAIL(VALUE, FMT, ...)           \
    do                                  \
    {                                   \
        LOG(ERROR, FMT, ##__VA_ARGS__); \
        return VALUE;                   \
    } while(0)
