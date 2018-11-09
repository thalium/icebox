#pragma once

#include <loguru.hpp>

#ifndef FDP_MODULE
#error "missing FDP_MODULE define"
#endif

#define LOG(INFO, FMT, ...) do {\
    LOG_F(INFO, FDP_MODULE ": " FMT, ## __VA_ARGS__);\
    /* we want compile-time check of log arguments */\
    if(false) { printf(FDP_MODULE FMT, ## __VA_ARGS__); }\
} while(0)

#define FAIL(VALUE, FMT, ...) do {\
    LOG(ERROR, FMT, ## __VA_ARGS__);\
    return VALUE;\
} while(0)
