#define FDP_MODULE "tests_win10"
#include <icebox/callstack.hpp>
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/os.hpp>
#include <icebox/plugins/sym_loader.hpp>
#include <icebox/reader.hpp>
#include <icebox/sym.hpp>
#include <icebox/tracer/syscalls.gen.hpp>
#include <icebox/tracer/syscalls32.gen.hpp>
#include <icebox/tracer/tracer.hpp>
#include <icebox/utils/fnview.hpp>
#include <icebox/waiter.hpp>
#include <tests/common.hpp>

#define GTEST_DONT_DEFINE_FAIL 1
#include <gtest/gtest.h>

#include <map>
#include <unordered_set>

namespace
{
    struct Win10Test
        : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            bool core_setup = false;
            ASSERT_EXEC_BEFORE_TIMEOUT_NS(core_setup = core.setup("win10"), 30 * SECOND_NS);
            ASSERT_TRUE(core_setup);

            const auto paused = core.state.pause();
            ASSERT_TRUE(paused);
        }

        void TearDown() override
        {
            const auto resumed = core.state.resume();
            EXPECT_TRUE(resumed);
        }

        core::Core core;
    };
}

TEST_F(Win10Test, attach)
{
}

TEST_F(Win10Test, drivers)
{
    using Driver  = std::tuple<uint64_t, uint64_t, size_t>;
    using Drivers = std::map<std::string, Driver>;

    Drivers drivers;
    core.os->driver_list([&](driver_t drv)
    {
        const auto name = core.os->driver_name(drv);
        EXPECT_TRUE(name);

        const auto span = core.os->driver_span(drv);
        EXPECT_TRUE(span);

        if(name)
            drivers.emplace(*name, Driver{drv.id, span->addr, span->size});

        return WALK_NEXT;
    });
    ASSERT_NE(drivers.size(), 0u);
    const auto it = drivers.find(R"(\SystemRoot\system32\ntoskrnl.exe)");
    ASSERT_NE(it, drivers.end());

    const auto [id, addr, size] = it->second;
    EXPECT_NE(id, 0u);
    EXPECT_NE(addr, 0u);
    EXPECT_GT(size, 0u);

    const auto want = addr + (size >> 1);
    const auto drv  = core.os->driver_find(want);
    ASSERT_TRUE(drv);
    EXPECT_EQ(id, drv->id);
}

TEST_F(Win10Test, processes)
{
    using Process   = std::tuple<uint64_t, uint64_t, uint64_t, flags_e>;
    using Processes = std::multimap<std::string, Process>;

    Processes processes;
    core.os->proc_list([&](proc_t proc)
    {
        const auto name = core.os->proc_name(proc);
        EXPECT_TRUE(name);

        const auto pid = core.os->proc_id(proc);
        EXPECT_NE(pid, 0u);

        const auto flags = core.os->proc_flags(proc);

        if(name)
            processes.emplace(*name, Process{proc.id, proc.dtb.val, pid, flags});

        return WALK_NEXT;
    });
    ASSERT_NE(processes.size(), 0u);
    const auto it = processes.find("explorer.exe");
    ASSERT_NE(it, processes.end());

    const auto [id, dtb, pid, flags] = it->second;
    EXPECT_NE(id, 0u);
    EXPECT_NE(dtb, 0u);
    EXPECT_NE(pid, 0u);

    const auto proc = core.os->proc_find(pid);
    ASSERT_TRUE(proc && proc->id && proc->dtb.val);
    EXPECT_EQ(id, proc->id);
    EXPECT_EQ(dtb, proc->dtb.val);

    const auto valid = core.os->proc_is_valid(*proc);
    EXPECT_TRUE(valid);

    // check parent
    const auto parent = core.os->proc_parent(*proc);
    EXPECT_TRUE(parent && parent->id && parent->dtb.val);
    if(parent)
    {
        const auto parent_name = core.os->proc_name(*parent);
        EXPECT_TRUE(parent_name);
        if(parent_name)
            EXPECT_EQ(*parent_name, "userinit.exe");
    }

    // join proc in kernel
    ASSERT_EXEC_BEFORE_TIMEOUT_NS(core.os->proc_join(*proc, os::JOIN_ANY_MODE), 5 * SECOND_NS);
    const auto kcur = core.os->proc_current();
    EXPECT_TRUE(kcur && kcur->id && kcur->dtb.val);
    EXPECT_EQ(id, kcur->id);
    EXPECT_EQ(dtb, kcur->dtb.val);

    // run during multiple slice times to leave the process
    ASSERT_TRUE(tests::run_for_ns_with_rand(core, 300 * MILLISECOND_NS, 500 * MILLISECOND_NS));

    // join proc in user-mode
    ASSERT_EXEC_BEFORE_TIMEOUT_NS(core.os->proc_join(*proc, os::JOIN_USER_MODE), 5 * SECOND_NS);
    const auto cur = core.os->proc_current();
    EXPECT_TRUE(cur && cur->id && cur->dtb.val);
    EXPECT_EQ(id, cur->id);
    EXPECT_EQ(dtb, cur->dtb.val);
    EXPECT_TRUE(tests::is_user_mode(core));

    // run during multiple slice times to leave the process for next tests
    ASSERT_TRUE(tests::run_for_ns_with_rand(core, 300 * MILLISECOND_NS, 500 * MILLISECOND_NS)); // multiple slice time (which is 100ms by defaut)
}

TEST_F(Win10Test, threads)
{
    using Threads = std::set<uint64_t>;

    const auto explorer = core.os->proc_find("explorer.exe", flags_e::FLAGS_NONE);
    ASSERT_TRUE(explorer && explorer->id && explorer->dtb.val);

    Threads threads;
    core.os->thread_list(*explorer, [&](thread_t thread)
    {
        const auto proc = core.os->thread_proc(thread);
        EXPECT_TRUE(proc && proc->id && proc->dtb.val);
        EXPECT_EQ(proc->id, explorer->id);

        const auto tid = core.os->thread_id(*proc, thread);
        EXPECT_NE(tid, 0u);

        if(tid)
            threads.emplace(tid);

        return WALK_NEXT;
    });

    ASSERT_EXEC_BEFORE_TIMEOUT_NS(core.os->proc_join(*explorer, os::JOIN_ANY_MODE), 5 * SECOND_NS);
    const auto current = core.os->thread_current();
    ASSERT_TRUE(current && current->id);

    const auto tid = core.os->thread_id(*explorer, *current);
    ASSERT_NE(tid, 0u);

    ASSERT_NE(threads.size(), 0u);
    const auto it = threads.find(tid);
    EXPECT_NE(it, threads.end());
}

TEST_F(Win10Test, modules)
{
    using Module  = std::tuple<uint64_t, uint64_t, size_t, flags_e>;
    using Modules = std::multimap<std::string, Module>;

    const auto proc = core.os->proc_find("explorer.exe", flags_e::FLAGS_NONE);
    ASSERT_TRUE(proc && proc->id && proc->dtb.val);

    Modules modules;
    core.os->mod_list(*proc, [&](mod_t mod)
    {
        const auto name = core.os->mod_name(*proc, mod);
        if(!name)
            return WALK_NEXT; // FIXME

        const auto span = core.os->mod_span(*proc, mod);
        EXPECT_TRUE(span);

        modules.emplace(*name, Module{mod.id, span->addr, span->size, mod.flags});
        return WALK_NEXT;
    });
    ASSERT_NE(modules.size(), 0u);

    const auto it = modules.find(R"(C:\Windows\SYSTEM32\ntdll.dll)");
    ASSERT_NE(it, modules.end());

    const auto [id, addr, size, flags] = it->second;
    EXPECT_NE(id, 0u);
    EXPECT_NE(addr, 0u);
    EXPECT_GT(size, 0u);

    const auto want = addr + (size >> 1);
    const auto mod  = core.os->mod_find(*proc, want);
    ASSERT_TRUE(mod);
    EXPECT_EQ(id, mod->id);
}

TEST_F(Win10Test, unable_to_single_step_query_information_process)
{
    const auto proc = waiter::proc_wait(core, "ProcessHacker.exe", FLAGS_NONE);
    ASSERT_TRUE(proc && proc->id && proc->dtb.val);

    const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_32BIT);
    ASSERT_TRUE(ntdll);

    auto loader           = sym::Loader{core};
    const auto load_ntdll = loader.mod_load(*proc, *ntdll);
    ASSERT_TRUE(load_ntdll);

    wow64::syscalls32 tracer{core, loader.symbols(), "ntdll"};
    bool found = false;
    // ZwQueryInformationProcess in 32-bit has code reading itself
    // we need to ensure we can break this function & resume properly
    // FDP had a bug where this was not possible
    tracer.register_ZwQueryInformationProcess(*proc, [&](wow64::HANDLE, wow64::PROCESSINFOCLASS, wow64::PVOID, wow64::ULONG, wow64::PULONG)
    {
        found = true;
    });
    EXPECT_CONDITION_RUNNING_BEFORE_TIMEOUT_NS(core, found, 8 * SECOND_NS);
}

TEST_F(Win10Test, unset_bp_when_two_bps_share_phy_page)
{
    const auto proc = waiter::proc_wait(core, "ProcessHacker.exe", FLAGS_NONE);
    ASSERT_TRUE(proc && proc->id && proc->dtb.val);

    const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_32BIT);
    ASSERT_TRUE(ntdll);

    auto loader           = sym::Loader{core};
    const auto load_ntdll = loader.mod_load(*proc, *ntdll);
    ASSERT_TRUE(load_ntdll);

    // break on a single function once
    wow64::syscalls32 tracer{core, loader.symbols(), "ntdll"};
    int func_start = 0;
    tracer.register_ZwWaitForSingleObject(*proc, [&](wow64::HANDLE, wow64::BOOLEAN, wow64::PLARGE_INTEGER)
    {
        ++func_start;
    });
    ASSERT_CONDITION_RUNNING_BEFORE_TIMEOUT_NS(core, func_start > 0, 8 * SECOND_NS);

    // set a breakpoint on next instruction
    core.state.single_step(); // TODO check single step worked well
    const auto addr_a = core.regs.read(FDP_RIP_REGISTER);
    int func_a        = 0;
    auto bp_a         = core.state.set_breakpoint("ZwWaitForSingleObject + $1", addr_a, *proc, [&]
    {
        func_a++;
    });

    // set a breakpoint on next instruction again
    // we are sure the previous bp share a physical page with at least one bp
    core.state.single_step(); // TODO check single step worked well
    const auto addr_b = core.regs.read(FDP_RIP_REGISTER);
    int func_b        = 0;
    const auto bp_b   = core.state.set_breakpoint("ZwWaitForSingleObject + $2", addr_b, *proc, [&]
    {
        func_b++;
    });

    // wait to break on third breakpoint
    EXPECT_CONDITION_RUNNING_BEFORE_TIMEOUT_NS(core, func_b > 0, 8 * SECOND_NS);

    // remove mid breakpoint
    bp_a.reset();

    // ensure vm is not frozen
    ASSERT_CONDITION_RUNNING_BEFORE_TIMEOUT_NS(core, func_start > 4, 8 * SECOND_NS);
}

TEST_F(Win10Test, memory)
{
    const auto proc = core.os->proc_find("explorer.exe", flags_e::FLAGS_NONE);
    ASSERT_TRUE(proc && proc->id && proc->dtb.val);
    LOG(INFO, "explorer dtb: {:#x}", proc->dtb.val);

    ASSERT_EXEC_BEFORE_TIMEOUT_NS(core.os->proc_join(*proc, os::JOIN_USER_MODE), 5 * SECOND_NS);

    auto from_reader  = std::vector<uint8_t>{};
    auto from_virtual = std::vector<uint8_t>{};
    const auto reader = reader::make(core, *proc);
    core.os->mod_list(*proc, [&](mod_t mod)
    {
        const auto span = core.os->mod_span(*proc, mod);
        EXPECT_TRUE(span);
        if(!span)
            return WALK_NEXT;

        from_reader.resize(span->size);
        auto read_memory = reader.read(&from_reader[0], span->addr, span->size);
        EXPECT_TRUE(read_memory);

        from_virtual.resize(span->size);
        read_memory = core.mem.read_virtual(&from_virtual[0], proc->dtb, span->addr, span->size);
        EXPECT_TRUE(read_memory);

        EXPECT_EQ(0, memcmp(&from_reader[0], &from_virtual[0], span->size));

        const auto phy = core.mem.virtual_to_physical(span->addr, proc->dtb);
        EXPECT_TRUE(phy);

        return WALK_NEXT;
    });
}

namespace
{
    static size_t count_symbols(sym::Symbols& symbols)
    {
        size_t count  = 0;
        const auto ok = symbols.list([&](const auto& /*sym*/)
        {
            ++count;
            return WALK_NEXT;
        });
        EXPECT_TRUE(ok);
        return count;
    }
}

TEST_F(Win10Test, loader)
{
    const auto proc = waiter::proc_wait(core, "dwm.exe", FLAGS_NONE);
    ASSERT_TRUE(proc && proc->id && proc->dtb.val);

    ASSERT_EXEC_BEFORE_TIMEOUT_NS(core.os->proc_join(*proc, os::JOIN_ANY_MODE), 5 * SECOND_NS);
    auto drivers = sym::Loader{core};
    drivers.drv_listen({});
    EXPECT_GE(count_symbols(drivers.symbols()), 128u);

    ASSERT_EXEC_BEFORE_TIMEOUT_NS(core.os->proc_join(*proc, os::JOIN_USER_MODE), 5 * SECOND_NS);
    auto modules = sym::Loader{core};
    modules.mod_listen(*proc, {});
    EXPECT_GE(count_symbols(modules.symbols()), 32u);

    const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_NONE);
    ASSERT_TRUE(ntdll);
}

TEST_F(Win10Test, tracer)
{
    const auto proc = waiter::proc_wait(core, "dwm.exe", FLAGS_NONE);
    ASSERT_TRUE(proc && proc->id && proc->dtb.val);

    ASSERT_EXEC_BEFORE_TIMEOUT_NS(core.os->proc_join(*proc, os::JOIN_USER_MODE), 5 * SECOND_NS);
    const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_NONE);
    ASSERT_TRUE(ntdll);

    auto loader = sym::Loader{core};
    loader.mod_load(*proc, *ntdll);

    using Calls = std::unordered_set<std::string>;
    auto calls  = Calls{};
    auto tracer = nt::syscalls{core, loader.symbols(), "ntdll"};
    auto count  = 0;
    tracer.register_all(*proc, [&](const auto& cfg)
    {
        calls.insert(cfg.name);
        ++count;
    });
    EXPECT_CONDITION_RUNNING_BEFORE_TIMEOUT_NS(core, count > 32, 8 * SECOND_NS);

    for(const auto& call : calls)
        LOG(INFO, "call: {}", call);
}

namespace
{
    static std::string dump_address(sym::Symbols& symbols, uint64_t addr)
    {
        const auto cur = symbols.find(addr);
        if(!cur)
            return fmt::format("{:#x}", addr);

        return sym::to_string(*cur);
    }
}

TEST_F(Win10Test, callstacks)
{
    const auto proc = waiter::proc_wait(core, "dwm.exe", FLAGS_NONE);
    ASSERT_TRUE(proc && proc->id && proc->dtb.val);

    auto loader = sym::Loader{core};
    loader.mod_listen(*proc, {});
    const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_NONE);
    ASSERT_TRUE(ntdll);

    auto& symbols   = loader.symbols();
    auto tracer     = nt::syscalls{core, symbols, "ntdll"};
    auto callstacks = callstack::make_callstack_nt(core);
    auto count      = size_t{0};
    tracer.register_all(*proc, [&](const auto& /* cfg*/)
    {
        LOG(INFO, "");
        auto idx = size_t{0};
        callstacks->get_callstack(*proc, [&](callstack::callstep_t step)
        {
            const auto symbol = dump_address(symbols, step.addr);
            LOG(INFO, "{:#x}: {}", idx++, symbol);
            return WALK_NEXT;
        });
        count++;
    });
    EXPECT_CONDITION_RUNNING_BEFORE_TIMEOUT_NS(core, count > 32, 8 * SECOND_NS);
}
