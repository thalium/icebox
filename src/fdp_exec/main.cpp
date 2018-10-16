#include "core.hpp"
#include "os.hpp"

#define FDP_MODULE "main"
#include "log.hpp"

#include <thread>
#include <chrono>

void print_state(core::IHandler& core)
{
    const auto rip = core.read_reg(FDP_RIP_REGISTER);
    const auto cr3 = core.read_reg(FDP_CR3_REGISTER);
    LOG(INFO, "rip: 0x%llx cr3: 0x%llx", rip ? *rip : 0, cr3 ? *cr3 : 0);
}

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
    const auto core = core::make_core(name);
    if(!core)
        FAIL(-1, "unable to start core at %s", name.data());

    core->resume();
    core->pause();
    auto& os = core->os();
    auto& sym = core->sym();

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
            const auto span = os.get_mod_span(proc, mod);
            if(false)
                LOG(INFO, "    module: %llx %s 0x%llx 0x%llx", mod, modname ? modname->data() : "<noname>", span ? span->addr : 0, span ? span->size : 0);
            return WALK_NEXT;
        });
        return WALK_NEXT;
    });

    LOG(INFO, "searching notepad.exe");
    const auto notepad = os.get_proc("notepad.exe");
    LOG(INFO, "notepad.exe: %llx %s", notepad->id, os.get_proc_name(*notepad)->data());

    const auto write_file = sym.get_symbol("nt", "NtWriteFile");
    LOG(INFO, "WriteFile = 0x%llx", write_file ? *write_file : 0);
    if(!write_file)
        return -1;

    {
        const auto bpa = core->set_breakpoint(*write_file, *notepad, core::FILTER_CR3, [&]
        {
            print_state(*core);
        });

        for(auto i = 0; i < 4; ++i)
        {
            core->resume();
            core->wait();
        }
    }
    core->resume();


    return 0;
}
