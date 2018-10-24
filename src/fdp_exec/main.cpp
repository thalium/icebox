#include "core.hpp"

#define FDP_MODULE "main"
#include "log.hpp"
#include "os.hpp"

#include <thread>
#include <chrono>
#include <filesystem>

namespace fs = std::experimental::filesystem;

namespace
{
    bool test_core(core::Core& core)
    {
        LOG(INFO, "drivers:");
        core.os->driver_list([&](driver_t drv)
        {
            const auto name = core.os->driver_name(drv);
            const auto span = core.os->driver_span(drv);
            LOG(INFO, "    driver: %llx %s 0x%llx 0x%llx", drv.id, name ? name->data() : "<noname>", span ? span->addr : 0, span ? span->size : 0);
            return WALK_NEXT;
        });

        const auto pc = core.os->proc_current();
        LOG(INFO, "current process: %llx dtb: %llx %s", pc->id, pc->dtb, core.os->proc_name(*pc)->data());

        const auto tc = core.os->thread_current();
        LOG(INFO, "current thread: %llx", tc->id);

        LOG(INFO, "processes:");
        core.os->proc_list([&](proc_t proc)
        {
            const auto procname = core.os->proc_name(proc);
            LOG(INFO, "proc: %llx %s", proc.id, procname ? procname->data() : "<noname>");
            return WALK_NEXT;
        });

        const char proc_target[] = "notepad.exe";
        LOG(INFO, "searching %s", proc_target);
        const auto target = core.os->proc_find(proc_target);
        if(!target)
            return false;

        LOG(INFO, "%s: %" PRIx64 " dtb: %" PRIx64 " %s", proc_target, target->id, target->dtb, core.os->proc_name(*target)->data());
        const auto join = core.state.proc_join(*target, core::JOIN_USER_MODE);
        if(!join)
            return false;

        std::vector<uint8_t> buffer;
        size_t modcount = 0;
        core.os->mod_list(*target, [&](mod_t)
        {
            ++modcount;
            return WALK_NEXT;
        });
        size_t modi = 0;
        core.os->mod_list(*target, [&](mod_t mod)
        {
            const auto name = core.os->mod_name(*target, mod);
            const auto span = core.os->mod_span(*target, mod);
            if(!name || !span)
                return WALK_NEXT;

            LOG(INFO, "module[%03zd/%03zd] %s: 0x%" PRIx64 " 0x%zx", modi, modcount, name->data(), span->addr, span->size);
            ++modi;
            buffer.resize(span->size);
            auto ok = core.mem.virtual_read(&buffer[0], span->addr, span->size);
            if(!ok)
                return WALK_NEXT;

            const auto fname = fs::path(*name).filename().replace_extension("");
            ok = core.sym.insert(fname.generic_string().data(), *span, &buffer[0]);
            if(!ok)
                return WALK_NEXT;

            return WALK_NEXT;
        });

        core.os->thread_list(*target, [&](thread_t thread)
        {
            const auto rip = core.os->thread_pc(*target, thread);
            if(!rip)
                return WALK_NEXT;

            const auto name = core.sym.find(*rip);
            LOG(INFO, "thread: %" PRIx64 " 0x%" PRIx64 "%s", thread.id, *rip, name ? (" " + name->module + "!" + name->symbol + "+" + std::to_string(name->offset)).data() : "");
            return WALK_NEXT;
        });

        // check breakpoints
        {
            const auto ptr = core.sym.symbol("nt", "SwapContext");
            const auto bp = core.state.set_breakpoint(*ptr, *target, core::ANY_CR3, [&]
            {
                const auto rip = core.regs.read(FDP_RIP_REGISTER);
                if(!rip)
                    return;

                const auto proc = core.os->proc_current();
                const auto thread = core.os->thread_current();
                const auto procname = proc ? core.os->proc_name(*proc) : std::nullopt;
                const auto sym = core.sym.find(*rip);
                LOG(INFO, "BREAK! rip: %" PRIx64 " %s proc: %" PRIx64 " %s thread: %" PRIx64,
                    *rip, sym ? sym::to_string(*sym).data() : "", proc ? proc->id : 0, procname ? procname->data() : "",
                    thread ? thread->id : 0);
            });
            for(size_t i = 0; i < 16; ++i)
            {
                core.state.resume();
                core.state.wait();
            }
        }

        return true;
    }
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

    core::Core core;
    const auto ok = core::setup(core, name);
    if(!ok)
        FAIL(-1, "unable to start core at %s", name.data());

    core.state.resume();
    core.state.pause();
    const auto valid = test_core(core);
    core.state.resume();
    return !valid;
}
