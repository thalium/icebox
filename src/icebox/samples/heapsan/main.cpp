#define FDP_MODULE "main"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/plugins/heapsan.hpp>
#include <icebox/plugins/sym_loader.hpp>
#include <icebox/waiter.hpp>

namespace
{
    static int heapsan(core::Core& core, std::string_view target)
    {
        LOG(INFO, "waiting for {}...", target);
        const auto proc = waiter::proc_wait(core, target, FLAGS_NONE);
        if(!proc)
            return FAIL(-1, "unable to wait for {}", target);

        LOG(INFO, "process {} active", target);
        const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_NONE);
        if(!ntdll)
            return FAIL(-1, "unable to load ntdll.dll");

        LOG(INFO, "ntdll module loaded");
        auto loader   = sym::Loader{core};
        const auto ok = loader.mod_load(*proc, *ntdll);
        if(!ok)
            return FAIL(-1, "unable to load ntdll.dll symbols");

        LOG(INFO, "listening events...");
        const auto fdpsan = plugins::HeapSan{core, loader.symbols(), *proc};
        const auto now    = std::chrono::high_resolution_clock::now();
        const auto end    = now + std::chrono::minutes(5);
        while(std::chrono::high_resolution_clock::now() < end)
        {
            core.state.resume();
            core.state.wait();
        }
        return 0;
    }
}

int main(int argc, char** argv)
{
    logg::init(argc, argv);
    if(argc != 3)
        return FAIL(-1, "usage: nt_writefile <name> <process_name>");

    const auto name   = std::string{argv[1]};
    const auto target = std::string_view{argv[2]};
    LOG(INFO, "starting on {}", name.data());

    core::Core core;
    const auto ok = core.setup(name);
    if(!ok)
        return FAIL(-1, "unable to start core at {}", name.data());

    core.state.pause();
    const auto ret = heapsan(core, target);
    core.state.resume();
    return ret;
}
