#pragma once

#include <fmt/format.h>

#ifndef FDP_MODULE
#    error "missing FDP_MODULE define"
#endif

namespace logg
{
    enum class level_t
    {
        info,
        error,
    };

    void    init    (int argc, char** argv);
    void    print   (level_t level, std::string_view arg);
} // namespace logg

#define LOG_WITH(LEVEL, FMT, ...)                                                          \
    do                                                                                     \
    {                                                                                      \
        logg::print((LEVEL), fmt::format(FMT_STRING(FDP_MODULE ": " FMT), ##__VA_ARGS__)); \
    } while(0)

#define LOG(LEVEL, FMT, ...) LOG_WITH(logg::level_t::info, FMT, ##__VA_ARGS__)

#define FAIL(VALUE, FMT, ...)                               \
    do                                                      \
    {                                                       \
        LOG_WITH(logg::level_t::error, FMT, ##__VA_ARGS__); \
        return VALUE;                                       \
    } while(0)
