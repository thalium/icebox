#define FDP_MODULE "main"
#include <icebox/callstack.hpp>
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/os.hpp>
#include <icebox/plugins/syscalls.hpp>
#include <icebox/reader.hpp>
#include <icebox/utils/fnview.hpp>
#include <icebox/utils/path.hpp>
#include <icebox/utils/pe.hpp>
#include <icebox/waiter.hpp>

#include <chrono>
#include <thread>

namespace
{
    template <typename T>
    void test_tracer(core::Core& core, sym::Symbols& syms, proc_t target)
    {
        T syscall_plugin(core, syms, target);

        LOG(INFO, "Everything is set up ! Please trigger some syscalls");

        const auto n = 100;
        for(size_t i = 0; i < n; ++i)
        {
            core.state.resume();
            core.state.wait();
        }

        syscall_plugin.generate("output.json");
    }

    void test_wait_and_trace(core::Core& core, std::string_view proc_target)
    {
        LOG(INFO, "searching for 32 bits {} process", proc_target);
        const auto target = waiter::proc_wait(core, proc_target, FLAGS_32BIT);
        if(!target)
            return;

        core.os->proc_join(*target, os::JOIN_USER_MODE);
        const auto reader = reader::make(core, *target);
        const auto mod    = waiter::mod_wait(core, *target, "ntdll.dll", FLAGS_32BIT);
        if(!mod)
            return;

        core.os->proc_join(*target, os::JOIN_USER_MODE);
        const auto span = core.os->mod_span(*target, *mod);
        if(!span)
            return;

        // Load ntdll32 pdb
        const auto debug = pe::find_debug_codeview(reader, *span);
        if(!debug)
            return;

        std::vector<uint8_t> buffer;
        buffer.resize(debug->size);
        auto ok = reader.read(&buffer[0], debug->addr, debug->size);
        if(!ok)
            return;

        sym::Symbols symbols;
        const auto inserted = symbols.insert("ntdll32", *span, &buffer[0], buffer.size());
        if(!inserted)
            return;

        plugins::Syscalls32 syscalls(core, symbols, *target);
        LOG(INFO, "Every thing is ready ! Please trigger some syscalls");

        for(size_t i = 0; i < 3000; ++i)
        {
            core.state.resume();
            core.state.wait();
            if(i % 200 == 0)
                LOG(INFO, "{}", i);
        }

        syscalls.generate("output.json");
    }

    bool test_core(core::Core& core)
    {
        LOG(INFO, "drivers:");
        core.os->driver_list([&](driver_t drv)
        {
            const auto name     = core.os->driver_name(drv);
            const auto span     = core.os->driver_span(drv);
            const auto filename = name ? path::filename(*name).generic_string() : "_";
            LOG(INFO, "    driver: {:#x} {} {:#x} {:#x}", drv.id, filename.data(), span ? span->addr : 0, span ? span->size : 0);
            return WALK_NEXT;
        });

        const auto pc = core.os->proc_current();
        LOG(INFO, "current process: {:#x} dtb: {:#x} {}", pc->id, pc->dtb.val, core.os->proc_name(*pc)->data());

        const auto tc = core.os->thread_current();
        LOG(INFO, "current thread: {:#x}", tc->id);

        LOG(INFO, "processes:");
        core.os->proc_list([&](proc_t proc)
        {
            const auto procname = core.os->proc_name(proc);
            LOG(INFO, "proc: {:#x} {}", proc.id, procname ? procname->data() : "<noname>");
            return WALK_NEXT;
        });

        const char proc_target[] = "notepad.exe";
        LOG(INFO, "searching for {}", proc_target);
        const auto target = waiter::proc_wait(core, proc_target, FLAGS_NONE);
        if(!target)
            return false;

        LOG(INFO, "{}: {:#x} dtb: {:#x} {}", proc_target, target->id, target->dtb.val, core.os->proc_name(*target)->data());
        core.os->proc_join(*target, os::JOIN_ANY_MODE);
        core.os->proc_join(*target, os::JOIN_USER_MODE);

        const auto is_32bit = core.os->proc_flags(*target) & FLAGS_32BIT;

        std::vector<uint8_t> buffer;
        const auto reader = reader::make(core, *target);
        size_t modcount   = 0;
        core.os->mod_list(*target, [&](mod_t)
        {
            ++modcount;
            return WALK_NEXT;
        });
        size_t modi = 0;
        sym::Symbols syms;
        core.os->mod_list(*target, [&](mod_t mod)
        {
            const auto name = core.os->mod_name(*target, mod);
            const auto span = core.os->mod_span(*target, mod);
            if(!name || !span)
                return WALK_NEXT;

            LOG(INFO, "module[{:>2}/{:<2}] {}: {:#x} {:#x}{}", modi, modcount, name->data(), span->addr, span->size,
                mod.flags & FLAGS_32BIT ? " wow64" : "");
            ++modi;

            const auto debug = pe::find_debug_codeview(reader, *span);
            if(!debug)
                return WALK_NEXT;

            buffer.resize(debug->size);
            const auto ok = reader.read(&buffer[0], debug->addr, debug->size);
            if(!ok)
                return FAIL(WALK_NEXT, "Unable to read IMAGE_CODEVIEW (RSDS)");

            const auto filename = path::filename(*name).replace_extension("").generic_string();
            const auto inserted = syms.insert(filename.data(), *span, &buffer[0], buffer.size());
            if(!inserted)
                return WALK_NEXT;

            return WALK_NEXT;
        });

        core.os->thread_list(*target, [&](thread_t thread)
        {
            const auto rip = core.os->thread_pc(*target, thread);
            if(!rip)
                return WALK_NEXT;

            const auto name = syms.find(*rip);
            LOG(INFO, "thread: {:#x} {:#x}{}", thread.id, *rip, name ? (" " + name->module + "!" + name->symbol + "+" + std::to_string(name->offset)).data() : "");
            return WALK_NEXT;
        });

        // check breakpoints
        {
            const auto ptr = core.os->kernel_symbols().symbol("nt", "SwapContext");
            const auto bp  = core.state.set_breakpoint("SwapContext", *ptr, [&]
            {
                const auto rip = core.regs.read(FDP_RIP_REGISTER);
                if(!rip)
                    return;

                const auto proc     = core.os->proc_current();
                const auto pid      = core.os->proc_id(*proc);
                const auto thread   = core.os->thread_current();
                const auto tid      = core.os->thread_id(*proc, *thread);
                const auto procname = proc ? core.os->proc_name(*proc) : ext::nullopt;
                const auto sym      = syms.find(rip);
                LOG(INFO, "BREAK! rip: {:#x} {} {} pid:{} tid:{}",
                    rip, sym ? sym::to_string(*sym).data() : "", procname ? procname->data() : "", pid, tid);
            });
            for(size_t i = 0; i < 16; ++i)
            {
                core.state.resume();
                core.state.wait();
            }
        }

        // test callstack
        {
            const auto callstack = callstack::make_callstack_nt(core);
            const auto cs_depth  = 40;
            const auto pdb_name  = "ntdll";
            const auto func_name = "RtlAllocateHeap";
            const auto func_addr = syms.symbol(pdb_name, func_name);
            LOG(INFO, "{} = {:#x}", func_name, func_addr ? *func_addr : 0);

            const auto bp = core.state.set_breakpoint(func_name, *func_addr, *target, [&]
            {
                int k = 0;
                callstack->get_callstack(*target, [&](callstack::callstep_t callstep)
                {
                    auto cursor = syms.find(callstep.addr);
                    if(!cursor)
                        cursor = sym::Cursor{"_", "_", callstep.addr};

                    LOG(INFO, "{:>2} - {}", k, sym::to_string(*cursor).data());
                    k++;
                    if(k >= cs_depth)
                        return WALK_STOP;

                    return WALK_NEXT;
                });
                LOG(INFO, "");
            });
            for(size_t i = 0; i < 3; ++i)
            {
                core.state.resume();
                core.state.wait();
            }
        }

        // test syscall plugin
        {
            if(is_32bit)
                test_tracer<plugins::Syscalls32>(core, syms, *target);
            else
                test_tracer<plugins::Syscalls>(core, syms, *target);
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
    LOG(INFO, "starting on {}", name.data());

    core::Core core;
    const auto ok = core.setup(name);
    if(!ok)
        return FAIL(-1, "unable to start core at {}", name.data());

    // core.state.resume();
    core.state.pause();
    const auto valid = test_core(core);
    core.state.resume();
    return !valid;
}
