#define FDP_MODULE "log"
#include "log.hpp"

#include <loguru.hpp>

void logg::init(int argc, char** argv)
{
    loguru::g_preamble_uptime = false;
    loguru::g_preamble_date   = false;
    loguru::g_preamble_thread = false;
    loguru::g_preamble_file   = false;
    loguru::init(argc, argv);
}

void logg::print(logg::level_t level, std::string_view arg)
{
    switch(level)
    {
        case level_t::info:
            LOG_F(INFO, "%s", arg.data());
            return;

        case level_t::error:
            LOG_F(ERROR, "%s", arg.data());
            return;
    }
}
