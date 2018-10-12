#include "core.hpp"
#include "os.hpp"

#define FDP_MODULE "main"
#include "log.hpp"

#include <thread>
#include <chrono>

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

    core->pause();
    auto& os = core->os();
    LOG(INFO, "get proc");
    const auto pc = os.get_current_proc();
    LOG(INFO, "current process: %llx %s", pc->id, os.get_proc_name(*pc)->data());

    LOG(INFO, "processes:");
    os.list_procs([&](proc_t proc)
    {
        const auto procname = os.get_proc_name(proc);
        LOG(INFO, "proc: %llx %s", proc.id, procname ? procname->data() : "<noname>");
        os.list_mods(proc, [&](mod_t mod)
        {
            const auto modname = os.get_mod_name(proc, mod);
            LOG(INFO, "    module: %llx %s", mod, modname ? modname->data() : "<noname>");
            return WALK_NEXT;
        });
        return WALK_NEXT;
    });

    LOG(INFO, "searching wininit.exe");
    const auto wininit = os.get_proc("wininit.exe");
    LOG(INFO, "wininit.exe: %llx %s", wininit->id, os.get_proc_name(*wininit)->data());

    core->resume();
    return 0;
}
