#include "core.hpp"

#define FDP_MODULE "main"
#include "log.hpp"

int main(int argc, char* argv[])
{
    loguru::g_preamble_uptime = false;
    loguru::g_preamble_date = false;
    loguru::g_preamble_thread = false;
    loguru::g_preamble_file = false;
    loguru::init(argc, argv);
    if(argc != 2)
        FAIL(-1, "usage: fdp_exec <name>");

    const auto name = std::string{argv[1]};
    LOG(INFO, "starting on %s", name.data());
    const auto core = make_core(name);
    if(!core)
        FAIL(-1, "unable to start core at %s", name.data());

    return 0;
}
