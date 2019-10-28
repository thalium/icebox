#define FDP_MODULE "main"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/plugins/syscalls.hpp>
#include <icebox/reader.hpp>
#include <icebox/utils/path.hpp>
#include <icebox/utils/pe.hpp>

#include <chrono>
#include <string>
#include <thread>

namespace
{
    template <typename T>
    void test_tracer(core::Core& core, proc_t target)
    {
        T syscall_plugin(core, target);

        LOG(INFO, "Everything is set up ! Please trigger some syscalls");

        const auto n = 100;
        for(size_t i = 0; i < n; ++i)
        {
            state::resume(core);
            state::wait(core);
        }

        syscall_plugin.generate("output.json");
    }

    void test_wait_and_trace(core::Core& core, const std::string& proc_target)
    {
        LOG(INFO, "searching for 32 bits %s process", proc_target.data());
        const auto target = process::wait(core, proc_target, FLAGS_32BIT);
        if(!target)
            return;

        process::join(core, *target, mode_e::user);
        const auto mod = modules::wait(core, *target, "ntdll.dll", FLAGS_32BIT);
        if(!mod)
            return;

        process::join(core, *target, mode_e::user);
        const auto span = modules::span(core, *target, *mod);
        if(!span)
            return;

        const auto inserted = symbols::load_module_at(core, *target, "wntdll", *span);
        if(!inserted)
            return;

        plugins::Syscalls32 syscalls(core, *target);
        LOG(INFO, "Every thing is ready ! Please trigger some syscalls");

        for(size_t i = 0; i < 3000; ++i)
        {
            state::resume(core);
            state::wait(core);
            if(i % 200 == 0)
                LOG(INFO, "%zd", i);
        }

        syscalls.generate("output.json");
    }

    bool test_core(core::Core& core)
    {
        LOG(INFO, "drivers:");
        drivers::list(core, [&](driver_t drv)
        {
            const auto name     = drivers::name(core, drv);
            const auto span     = drivers::span(core, drv);
            const auto filename = name ? path::filename(*name).generic_string() : "_";
            LOG(INFO, "    driver: 0x%" PRIx64 " %s 0x%" PRIx64 " 0x%" PRIx64, drv.id, filename.data(), span ? span->addr : 0, span ? span->size : 0);
            return walk_e::next;
        });

        const auto pc = process::current(core);
        LOG(INFO, "current process: 0x%" PRIx64 " dtb: 0x%" PRIx64 " %s", pc->id, pc->dtb.val, process::name(core, *pc)->data());

        const auto tc = threads::current(core);
        LOG(INFO, "current thread: 0x%" PRIx64, tc->id);

        LOG(INFO, "processes:");
        process::list(core, [&](proc_t proc)
        {
            const auto procname = process::name(core, proc);
            LOG(INFO, "proc: 0x%" PRIx64 " %s", proc.id, procname ? procname->data() : "<noname>");
            return walk_e::next;
        });

        const char proc_target[] = "notepad.exe";
        LOG(INFO, "searching for %s", proc_target);
        const auto target = process::wait(core, proc_target, FLAGS_NONE);
        if(!target)
            return false;

        LOG(INFO, "%s: 0x%" PRIx64 " dtb: 0x%" PRIx64 " %s", proc_target, target->id, target->dtb.val, process::name(core, *target)->data());
        process::join(core, *target, mode_e::kernel);
        process::join(core, *target, mode_e::user);

        const auto is_32bit = process::flags(core, *target) & FLAGS_32BIT;

        std::vector<uint8_t> buffer;
        size_t modcount = 0;
        modules::list(core, *target, [&](mod_t)
        {
            ++modcount;
            return walk_e::next;
        });
        size_t modi = 0;
        modules::list(core, *target, [&](mod_t mod)
        {
            const auto name = modules::name(core, *target, mod);
            const auto span = modules::span(core, *target, mod);
            if(!name || !span)
                return walk_e::next;

            LOG(INFO, "module[%2zd/%-2zd] %s: 0x%" PRIx64 " 0x%" PRIx64 "%s", modi, modcount, name->data(), span->addr, span->size,
                mod.flags & FLAGS_32BIT ? " wow64" : "");
            ++modi;

            const auto inserted = symbols::load_module(core, *target, mod);
            if(!inserted)
                return walk_e::next;

            return walk_e::next;
        });

        threads::list(core, *target, [&](thread_t thread)
        {
            const auto rip = threads::program_counter(core, *target, thread);
            if(!rip)
                return walk_e::next;

            const auto name = symbols::find(core, *target, *rip);
            LOG(INFO, "thread: 0x%" PRIx64 " 0x%" PRIx64 "%s", thread.id, *rip, symbols::to_string(name).data());
            return walk_e::next;
        });

        // check breakpoints
        {
            const auto ptr = symbols::symbol(core, symbols::kernel, "nt", "SwapContext");
            const auto bp  = state::set_breakpoint(core, "SwapContext", *ptr, [&]
            {
                const auto rip = registers::read(core, reg_e::rip);
                if(!rip)
                    return;

                const auto proc     = process::current(core);
                const auto pid      = process::pid(core, *proc);
                const auto thread   = threads::current(core);
                const auto tid      = threads::tid(core, *proc, *thread);
                const auto procname = proc ? process::name(core, *proc) : ext::nullopt;
                const auto symbol   = symbols::find(core, *proc, rip);
                LOG(INFO, "BREAK! rip: 0x%" PRIx64 " %s %s pid:%" PRId64 " tid:%" PRId64,
                    rip, symbols::to_string(symbol).data(), procname ? procname->data() : "", pid, tid);
            });
            for(size_t i = 0; i < 16; ++i)
            {
                state::resume(core);
                state::wait(core);
            }
        }

        // test callstack
        {
            const auto pdb_name  = "ntdll";
            const auto func_name = "RtlAllocateHeap";
            const auto func_addr = symbols::symbol(core, *target, pdb_name, func_name);
            LOG(INFO, "%s = 0x%" PRIx64, func_name, func_addr ? *func_addr : 0);

            auto callers  = std::vector<callstacks::caller_t>(128);
            const auto bp = state::set_breakpoint(core, func_name, *func_addr, *target, [&]
            {
                const auto n = callstacks::read(core, &callers[0], callers.size(), *target);
                for(size_t i = 0; i < n; ++i)
                {
                    const auto addr   = callers[i].addr;
                    const auto symbol = symbols::find(core, *target, addr);
                    LOG(INFO, "%-2zd - %s", i, symbols::to_string(symbol).data());
                }
                LOG(INFO, " ");
            });
            for(size_t i = 0; i < 3; ++i)
            {
                state::resume(core);
                state::wait(core);
            }
        }

        // test syscall plugin
        {
            if(is_32bit)
                test_tracer<plugins::Syscalls32>(core, *target);
            else
                test_tracer<plugins::Syscalls>(core, *target);
        }

        {
            if(false)
                test_wait_and_trace(core, "notepad.exe");
        }

        return true;
    }
}

int main(int argc, char* argv[])
{
    logg::init(argc, argv);
    if(argc != 2)
        return FAIL(-1, "usage: icebox <name>");

    const auto name = std::string{argv[1]};
    LOG(INFO, "starting on %s", name.data());

    const auto core = core::attach(name);
    if(!core)
        return FAIL(-1, "unable to start core at %s", name.data());

    // core.state.resume();
    state::pause(*core);
    const auto valid = test_core(*core);
    state::resume(*core);
    return !valid;
}
