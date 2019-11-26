#define FDP_MODULE "tests_win10"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/reader.hpp>
#include <icebox/tracer/syscalls.gen.hpp>
#include <icebox/tracer/syscalls32.gen.hpp>
#include <icebox/tracer/tracer.hpp>

#include <fmt/format.h>

#define GTEST_DONT_DEFINE_FAIL 1
#include <gtest/gtest.h>

#include <map>
#include <thread>
#include <unordered_set>

namespace
{
    struct win10
        : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ptr_core = core::attach("win10");
            ASSERT_TRUE(ptr_core);
            const auto paused = state::pause(*ptr_core);
            ASSERT_TRUE(paused);
        }

        void TearDown() override
        {
            const auto resumed = state::resume(*ptr_core);
            EXPECT_TRUE(resumed);
        }

        std::shared_ptr<core::Core> ptr_core;
    };
}

TEST(win10_, attach_detach)
{
    for(size_t i = 0; i < 16; ++i)
    {
        const auto core = core::attach("win10");
        EXPECT_TRUE(!!core);
        state::resume(*core);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

TEST_F(win10, attach)
{
}

TEST_F(win10, drivers)
{
    using Driver  = std::tuple<uint64_t, uint64_t, size_t>;
    using Drivers = std::map<std::string, Driver>;

    Drivers drivers;
    auto& core = *ptr_core;
    drivers::list(core, [&](driver_t drv)
    {
        const auto name = drivers::name(core, drv);
        EXPECT_TRUE(!!name);
        const auto span = drivers::span(core, drv);
        EXPECT_TRUE(!!span);
        drivers.emplace(*name, Driver{drv.id, span->addr, span->size});
        return walk_e::next;
    });
    EXPECT_NE(drivers.size(), 0u);
    const auto it = drivers.find(R"(\SystemRoot\system32\ntoskrnl.exe)");
    EXPECT_NE(it, drivers.end());

    const auto [id, addr, size] = it->second;
    EXPECT_NE(id, 0u);
    EXPECT_NE(addr, 0u);
    EXPECT_GT(size, 0u);

    const auto want = addr + (size >> 1);
    const auto drv  = drivers::find(core, want);
    EXPECT_TRUE(!!drv);
    EXPECT_EQ(id, drv->id);
}

TEST_F(win10, processes)
{
    using Process   = std::tuple<uint64_t, uint64_t, uint64_t, flags_t>;
    using Processes = std::multimap<std::string, Process>;

    Processes processes;
    auto& core = *ptr_core;
    process::list(core, [&](proc_t proc)
    {
        EXPECT_NE(proc.dtb.val, 0u);
        EXPECT_NE(proc.dtb.val, 1u);
        const auto name = process::name(core, proc);
        EXPECT_TRUE(!!name);
        const auto pid = process::pid(core, proc);
        EXPECT_NE(pid, 0u);
        const auto flags = process::flags(core, proc);
        processes.emplace(*name, Process{proc.id, proc.dtb.val, pid, flags});
        return walk_e::next;
    });
    EXPECT_NE(processes.size(), 0u);
    const auto it = processes.find("explorer.exe");
    EXPECT_NE(it, processes.end());

    const auto [id, dtb, pid, flags] = it->second;
    EXPECT_NE(id, 0u);
    EXPECT_NE(dtb, 0u);
    EXPECT_NE(pid, 0u);
    UNUSED(flags);

    const auto proc = process::find_pid(core, pid);
    EXPECT_TRUE(!!proc);
    EXPECT_EQ(id, proc->id);
    EXPECT_EQ(dtb, proc->dtb.val);

    const auto valid = process::is_valid(core, *proc);
    EXPECT_TRUE(valid);

    // check parent
    const auto parent = process::parent(core, *proc);
    EXPECT_TRUE(!!parent);
    const auto parent_name = process::name(core, *parent);
    EXPECT_TRUE(!!parent_name);
    EXPECT_EQ(*parent_name, "userinit.exe");

    // join proc in kernel
    process::join(core, *proc, mode_e::kernel);
    const auto kcur = process::current(core);
    EXPECT_TRUE(!!kcur);
    EXPECT_EQ(id, kcur->id);
    EXPECT_EQ(dtb, kcur->dtb.val);

    // join proc in user-mode
    process::join(core, *proc, mode_e::user);
    const auto cur = process::current(core);
    EXPECT_TRUE(!!cur);
    EXPECT_EQ(id, cur->id);
    EXPECT_EQ(dtb, cur->dtb.val);
}

TEST_F(win10, threads)
{
    using Threads = std::set<uint64_t>;

    auto& core          = *ptr_core;
    const auto explorer = process::find_name(core, "explorer.exe", {});
    EXPECT_TRUE(!!explorer);

    Threads threads;
    threads::list(core, *explorer, [&](thread_t thread)
    {
        const auto proc = threads::process(core, thread);
        EXPECT_TRUE(!!proc);
        EXPECT_EQ(proc->id, explorer->id);
        const auto tid = threads::tid(core, *proc, thread);
        EXPECT_NE(tid, 0u);
        threads.emplace(tid);
        return walk_e::next;
    });
    EXPECT_NE(threads.size(), 0u);

    process::join(core, *explorer, mode_e::kernel);
    const auto current = threads::current(core);
    EXPECT_TRUE(!!current);

    const auto tid = threads::tid(core, *explorer, *current);
    const auto it  = threads.find(tid);
    EXPECT_NE(it, threads.end());
}

TEST_F(win10, modules)
{
    using Module  = std::tuple<uint64_t, uint64_t, size_t, flags_t>;
    using Modules = std::multimap<std::string, Module>;

    auto& core      = *ptr_core;
    const auto proc = process::find_name(core, "explorer.exe", {});
    EXPECT_TRUE(!!proc);

    Modules modules;
    modules::list(core, *proc, [&](mod_t mod)
    {
        const auto name = modules::name(core, *proc, mod);
        if(!name)
            return walk_e::next; // FIXME

        const auto span = modules::span(core, *proc, mod);
        EXPECT_TRUE(!!span);
        modules.emplace(*name, Module{mod.id, span->addr, span->size, mod.flags});
        return walk_e::next;
    });
    EXPECT_NE(modules.size(), 0u);

    const auto it = modules.find(R"(C:\Windows\SYSTEM32\ntdll.dll)");
    EXPECT_NE(it, modules.end());

    const auto [id, addr, size, flags] = it->second;
    EXPECT_NE(id, 0u);
    EXPECT_NE(addr, 0u);
    EXPECT_GT(size, 0u);
    UNUSED(flags);

    const auto want = addr + (size >> 1);
    const auto mod  = modules::find(core, *proc, want);
    EXPECT_TRUE(!!mod);
    EXPECT_EQ(id, mod->id);
}

namespace
{
    template <typename T>
    static void run_until(core::Core& core, T predicate)
    {
        const auto now = std::chrono::high_resolution_clock::now();
        const auto end = now + std::chrono::seconds(8);
        while(!predicate() && std::chrono::high_resolution_clock::now() < end)
        {
            state::resume(core);
            state::wait(core);
        }
        EXPECT_TRUE(predicate());
    }
}

TEST_F(win10, unable_to_single_step_query_information_process)
{
    auto& core      = *ptr_core;
    const auto proc = process::wait(core, "ProcessHacker.exe", flags::x86);
    EXPECT_TRUE(!!proc);

    const auto ntdll = modules::wait(core, *proc, "ntdll.dll", flags::x86);
    EXPECT_TRUE(!!ntdll);

    process::join(core, *proc, mode_e::user);
    const auto ok = symbols::load_module(core, *proc, *ntdll);
    EXPECT_TRUE(ok);

    wow64::syscalls32 tracer{core, "ntdll"};
    bool found = false;
    // ZwQueryInformationProcess in 32-bit has code reading itself
    // we need to ensure we can break this function & resume properly
    // FDP had a bug where this was not possible
    tracer.register_ZwQueryInformationProcess(*proc, [&](wow64::HANDLE, wow64::PROCESSINFOCLASS, wow64::PVOID, wow64::ULONG, wow64::PULONG)
    {
        found = true;
    });
    run_until(core, [&] { return found; });
}

TEST_F(win10, unset_bp_when_two_bps_share_phy_page)
{
    auto& core      = *ptr_core;
    const auto proc = process::wait(core, "ProcessHacker.exe", flags::x86);
    EXPECT_TRUE(!!proc);

    const auto ntdll = modules::wait(core, *proc, "ntdll.dll", flags::x86);
    EXPECT_TRUE(!!ntdll);

    process::join(core, *proc, mode_e::user);
    const auto ok = symbols::load_module(core, *proc, *ntdll);
    EXPECT_TRUE(ok);

    // break on a single function once
    wow64::syscalls32 tracer{core, "ntdll"};
    int func_start = 0;
    tracer.register_ZwWaitForSingleObject(*proc, [&](wow64::HANDLE, wow64::BOOLEAN, wow64::PLARGE_INTEGER)
    {
        ++func_start;
    });
    run_until(core, [&] { return func_start > 0; });

    // set a breakpoint on next instruction
    state::single_step(core);
    const auto addr_a = registers::read(core, reg_e::rip);
    int func_a        = 0;
    auto bp_a         = state::break_on_process(core, "ZwWaitForSingleObject + $1", *proc, addr_a, [&]
    {
        func_a++;
    });

    // set a breakpoint on next instruction again
    // we are sure the previous bp share a physical page with at least one bp
    state::single_step(core);
    const auto addr_b = registers::read(core, reg_e::rip);
    int func_b        = 0;
    const auto bp_b   = state::break_on_process(core, "ZwWaitForSingleObject + $2", *proc, addr_b, [&]
    {
        func_b++;
    });

    // wait to break on third breakpoint
    run_until(core, [&] { return func_b > 0; });

    // remove mid breakpoint
    bp_a.reset();

    // ensure vm is not frozen
    run_until(core, [&] { return func_start > 4; });
}

TEST_F(win10, memory)
{
    auto& core      = *ptr_core;
    const auto proc = process::find_name(core, "explorer.exe", {});
    EXPECT_TRUE(!!proc);
    LOG(INFO, "explorer dtb: 0x%" PRIx64, proc->dtb.val);

    process::join(core, *proc, mode_e::user);

    auto from_reader  = std::vector<uint8_t>{};
    auto from_virtual = std::vector<uint8_t>{};
    const auto reader = reader::make(core, *proc);
    modules::list(core, *proc, [&](mod_t mod)
    {
        const auto span = modules::span(core, *proc, mod);
        EXPECT_TRUE(!!span);

        from_reader.resize(span->size);
        auto ok = reader.read_all(&from_reader[0], span->addr, span->size);
        EXPECT_TRUE(ok);

        from_virtual.resize(span->size);
        ok = memory::read_virtual_with_dtb(core, &from_virtual[0], proc->dtb, span->addr, span->size);
        EXPECT_TRUE(ok);

        EXPECT_EQ(0, memcmp(&from_reader[0], &from_virtual[0], span->size));

        const auto phy = memory::virtual_to_physical(core, span->addr, proc->dtb);
        EXPECT_TRUE(!!phy);
        return walk_e::next;
    });
}

TEST_F(win10, loader)
{
    auto& core      = *ptr_core;
    const auto proc = process::wait(core, "dwm.exe", {});
    ASSERT_TRUE(!!proc);

    process::join(core, *proc, mode_e::kernel);
    symbols::load_drivers(core);

    process::join(core, *proc, mode_e::user);
    symbols::listen_and_load(core, *proc, {});

    const auto ntdll = modules::wait(core, *proc, "ntdll.dll", {});
    ASSERT_TRUE(ntdll);
}

TEST_F(win10, tracer)
{
    auto& core      = *ptr_core;
    const auto proc = process::wait(core, "dwm.exe", {});
    ASSERT_TRUE(!!proc);

    process::join(core, *proc, mode_e::user);
    const auto ntdll = modules::wait(core, *proc, "ntdll.dll", {});
    ASSERT_TRUE(ntdll);

    process::join(core, *proc, mode_e::user);
    const auto ok = symbols::load_module(core, *proc, *ntdll);
    ASSERT_TRUE(ok);

    using Calls = std::unordered_set<std::string>;
    auto calls  = Calls{};
    auto tracer = nt::syscalls{core, "ntdll"};
    auto count  = 0;
    tracer.register_all(*proc, [&](const auto& cfg)
    {
        calls.insert(cfg.name);
        ++count;
    });
    run_until(core, [&] { return count > 32; });
    for(const auto& call : calls)
        LOG(INFO, "call: %s", call.data());
}

namespace
{
    static std::string dump_address(core::Core& core, proc_t proc, uint64_t addr)
    {
        const auto symbol = symbols::find(core, proc, addr);
        return symbols::to_string(symbol);
    }
}

TEST_F(win10, callstacks)
{
    auto& core      = *ptr_core;
    const auto proc = process::wait(core, "dwm.exe", {});
    ASSERT_TRUE(!!proc);

    const auto ntdll = modules::wait(core, *proc, "ntdll.dll", {});
    ASSERT_TRUE(ntdll);

    process::join(core, *proc, mode_e::user);

    symbols::listen_and_load(core, *proc, {});
    callstacks::autoload(core, *proc);
    auto tracer      = nt::syscalls{core, "ntdll"};
    auto count       = size_t{0};
    auto num_callers = size_t{0};
    tracer.register_all(*proc, [&](const auto& /* cfg*/)
    {
        LOG(INFO, " ");
        auto callers = std::vector<callstacks::caller_t>(128);
        const auto n = callstacks::read(core, &callers[0], callers.size(), *proc);
        for(size_t i = 0; i < n; ++i)
            LOG(INFO, "0x%02" PRIx64 ": %s", i, dump_address(core, *proc, callers[i].addr).data());
        count++;
        num_callers += n;
    });
    run_until(core, [&] { return count > 32; });
    EXPECT_EQ(count, 33u);
    EXPECT_NE(num_callers, 0u);
}

TEST_F(win10, listen_module_wow64)
{
    auto& core      = *ptr_core;
    const auto proc = process::wait(core, "ProcessHacker.exe", flags::x86);
    EXPECT_TRUE(!!proc);

    modules::listen_create(core, *proc, flags::x64, [&](mod_t mod)
    {
        const auto name = modules::name(core, *proc, mod);
        if(!name)
            return;

        const auto span = modules::span(core, *proc, mod);
        if(!span)
            return;

        LOG(INFO, "module loaded: 64-bit: %s 0x%" PRIx64 "-0x%" PRIx64, name->data(), span->addr, span->addr + span->size);
    });

    modules::listen_create(core, *proc, flags::x86, [&](mod_t mod)
    {
        const auto name = modules::name(core, *proc, mod);
        if(!name)
            return;

        const auto span = modules::span(core, *proc, mod);
        if(!span)
            return;

        LOG(INFO, "module loaded: 32-bit: %s 0x%" PRIx64 "-0x%" PRIx64, name->data(), span->addr, span->addr + span->size);
    });

    const auto nt64 = modules::wait(core, *proc, "ntdll.dll", flags::x64);
    EXPECT_TRUE(!!nt64);

    const auto ntwow64 = modules::wait(core, *proc, "ntdll.dll", flags::x86);
    EXPECT_TRUE(!!ntwow64);
}
