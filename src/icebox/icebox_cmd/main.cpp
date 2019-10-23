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
#include <string>
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
            state::resume(core);
            state::wait(core);
        }

        syscall_plugin.generate("output.json");
    }

    void test_wait_and_trace(core::Core& core, const std::string& proc_target)
    {
        LOG(INFO, "searching for 32 bits %s process", proc_target.data());
        const auto target = waiter::proc_wait(core, proc_target, FLAGS_32BIT);
        if(!target)
            return;

        process::join(core, *target, process::JOIN_USER_MODE);
        const auto reader = reader::make(core, *target);
        const auto mod    = waiter::mod_wait(core, *target, "ntdll.dll", FLAGS_32BIT);
        if(!mod)
            return;

        process::join(core, *target, process::JOIN_USER_MODE);
        const auto span = modules::span(core, *target, *mod);
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
            return WALK_NEXT;
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
            return WALK_NEXT;
        });

        const char proc_target[] = "notepad.exe";
        LOG(INFO, "searching for %s", proc_target);
        const auto target = waiter::proc_wait(core, proc_target, FLAGS_NONE);
        if(!target)
            return false;

        LOG(INFO, "%s: 0x%" PRIx64 " dtb: 0x%" PRIx64 " %s", proc_target, target->id, target->dtb.val, process::name(core, *target)->data());
        process::join(core, *target, process::JOIN_ANY_MODE);
        process::join(core, *target, process::JOIN_USER_MODE);

        const auto is_32bit = process::flags(core, *target) & FLAGS_32BIT;

        std::vector<uint8_t> buffer;
        const auto reader = reader::make(core, *target);
        size_t modcount   = 0;
        modules::list(core, *target, [&](mod_t)
        {
            ++modcount;
            return WALK_NEXT;
        });
        size_t modi = 0;
        sym::Symbols syms;
        modules::list(core, *target, [&](mod_t mod)
        {
            const auto name = modules::name(core, *target, mod);
            const auto span = modules::span(core, *target, mod);
            if(!name || !span)
                return WALK_NEXT;

            LOG(INFO, "module[%2zd/%-2zd] %s: 0x%" PRIx64 " 0x%" PRIx64 "%s", modi, modcount, name->data(), span->addr, span->size,
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

        threads::list(core, *target, [&](thread_t thread)
        {
            const auto rip = threads::program_counter(core, *target, thread);
            if(!rip)
                return WALK_NEXT;

            const auto name = syms.find(*rip);
            LOG(INFO, "thread: 0x%" PRIx64 " 0x%" PRIx64 "%s", thread.id, *rip, name ? (" " + name->module + "!" + name->symbol + "+" + std::to_string(name->offset)).data() : "");
            return WALK_NEXT;
        });

        // check breakpoints
        {
            const auto ptr = os::kernel_symbols(core).symbol("nt", "SwapContext");
            const auto bp  = state::set_breakpoint(core, "SwapContext", *ptr, [&]
            {
                const auto rip = registers::read(core, FDP_RIP_REGISTER);
                if(!rip)
                    return;

                const auto proc     = process::current(core);
                const auto pid      = process::pid(core, *proc);
                const auto thread   = threads::current(core);
                const auto tid      = threads::tid(core, *proc, *thread);
                const auto procname = proc ? process::name(core, *proc) : ext::nullopt;
                const auto sym      = syms.find(rip);
                LOG(INFO, "BREAK! rip: 0x%" PRIx64 " %s %s pid:%" PRId64 " tid:%" PRId64,
                    rip, sym ? sym::to_string(*sym).data() : "", procname ? procname->data() : "", pid, tid);
            });
            for(size_t i = 0; i < 16; ++i)
            {
                state::resume(core);
                state::wait(core);
            }
        }

        // test callstack
        {
            const auto callstack = callstack::make_callstack_nt(core);
            const auto cs_depth  = 40;
            const auto pdb_name  = "ntdll";
            const auto func_name = "RtlAllocateHeap";
            const auto func_addr = syms.symbol(pdb_name, func_name);
            LOG(INFO, "%s = 0x%" PRIx64, func_name, func_addr ? *func_addr : 0);

            const auto bp = state::set_breakpoint(core, func_name, *func_addr, *target, [&]
            {
                int k = 0;
                callstack->get_callstack(*target, [&](callstack::callstep_t callstep)
                {
                    auto cursor = syms.find(callstep.addr);
                    if(!cursor)
                        cursor = sym::Cursor{"_", "_", callstep.addr};

                    LOG(INFO, "%-2d - %s", k, sym::to_string(*cursor).data());
                    k++;
                    if(k >= cs_depth)
                        return WALK_STOP;

                    return WALK_NEXT;
                });
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
