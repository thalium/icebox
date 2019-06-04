#define FDP_MODULE "tests"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/os.hpp>
#include <icebox/plugins/sym_loader.hpp>
#include <icebox/tracer/syscalls32.gen.hpp>
#include <icebox/utils/fnview.hpp>
#include <icebox/waiter.hpp>

#define GTEST_DONT_DEFINE_FAIL 1
#include <gtest/gtest.h>

#include <map>

namespace
{
    struct Win10Test
        : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            const auto ok = core.setup("win10");
            EXPECT_TRUE(ok);
            const auto paused = core.state.pause();
            EXPECT_TRUE(paused);
        }

        void TearDown() override
        {
            const auto ok = core.state.resume();
            EXPECT_TRUE(ok);
        }

        core::Core core;
    };
}

int main(int argc, char** argv)
{
    logg::init(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST_F(Win10Test, attach)
{
    core.state.resume();
}

TEST_F(Win10Test, drivers)
{
    using Driver  = std::tuple<uint64_t, uint64_t, size_t>;
    using Drivers = std::map<std::string, Driver>;

    Drivers drivers;
    core.os->driver_list([&](driver_t drv)
    {
        const auto name = core.os->driver_name(drv);
        EXPECT_TRUE(!!name);
        const auto span = core.os->driver_span(drv);
        EXPECT_TRUE(!!span);
        drivers.emplace(*name, Driver{drv.id, span->addr, span->size});
        return WALK_NEXT;
    });
    EXPECT_NE(drivers.size(), 0u);
    const auto it = drivers.find(R"(\SystemRoot\system32\ntoskrnl.exe)");
    EXPECT_NE(it, drivers.end());

    const auto [id, addr, size] = it->second;
    EXPECT_NE(id, 0u);
    EXPECT_NE(addr, 0u);
    EXPECT_GT(size, 0u);

    const auto want = addr + (size >> 1);
    const auto drv  = core.os->driver_find(want);
    EXPECT_TRUE(!!drv);
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
        EXPECT_TRUE(!!name);
        const auto pid = core.os->proc_id(proc);
        EXPECT_NE(pid, 0u);
        const auto flags = core.os->proc_flags(proc);
        processes.emplace(*name, Process{proc.id, proc.dtb.val, pid, flags});
        return WALK_NEXT;
    });
    EXPECT_NE(processes.size(), 0u);
    const auto it = processes.find("explorer.exe");
    EXPECT_NE(it, processes.end());

    const auto [id, dtb, pid, flags] = it->second;
    EXPECT_NE(id, 0u);
    EXPECT_NE(dtb, 0u);
    EXPECT_NE(pid, 0u);

    const auto proc = core.os->proc_find(pid);
    EXPECT_TRUE(!!proc);
    EXPECT_EQ(id, proc->id);
    EXPECT_EQ(dtb, proc->dtb.val);

    const auto valid = core.os->proc_is_valid(*proc);
    EXPECT_TRUE(valid);

    // check parent
    const auto parent = core.os->proc_parent(*proc);
    EXPECT_TRUE(!!parent);
    const auto parent_name = core.os->proc_name(*parent);
    EXPECT_TRUE(!!parent_name);
    EXPECT_EQ(*parent_name, "userinit.exe");

    // join proc in kernel
    core.os->proc_join(*proc, os::JOIN_ANY_MODE);
    const auto kcur = core.os->proc_current();
    EXPECT_TRUE(!!kcur);
    EXPECT_EQ(id, kcur->id);
    EXPECT_EQ(dtb, kcur->dtb.val);

    // join proc in user-mode
    core.os->proc_join(*proc, os::JOIN_USER_MODE);
    const auto cur = core.os->proc_current();
    EXPECT_TRUE(!!cur);
    EXPECT_EQ(id, cur->id);
    EXPECT_EQ(dtb, cur->dtb.val);
}

TEST_F(Win10Test, threads)
{
    using Threads = std::set<uint64_t>;

    const auto explorer = core.os->proc_find("explorer.exe", flags_e::FLAGS_NONE);
    EXPECT_TRUE(!!explorer);

    Threads threads;
    core.os->thread_list(*explorer, [&](thread_t thread)
    {
        const auto proc = core.os->thread_proc(thread);
        EXPECT_TRUE(!!proc);
        EXPECT_EQ(proc->id, explorer->id);
        const auto tid = core.os->thread_id(*proc, thread);
        EXPECT_NE(tid, 0u);
        threads.emplace(tid);
        return WALK_NEXT;
    });
    EXPECT_NE(threads.size(), 0u);

    core.os->proc_join(*explorer, os::JOIN_ANY_MODE);
    const auto current = core.os->thread_current();
    EXPECT_TRUE(!!current);

    const auto tid = core.os->thread_id(*explorer, *current);
    const auto it  = threads.find(tid);
    EXPECT_NE(it, threads.end());
}

TEST_F(Win10Test, modules)
{
    using Module  = std::tuple<uint64_t, uint64_t, size_t, flags_e>;
    using Modules = std::multimap<std::string, Module>;

    const auto proc = core.os->proc_find("explorer.exe", flags_e::FLAGS_NONE);
    EXPECT_TRUE(!!proc);

    Modules modules;
    core.os->mod_list(*proc, [&](mod_t mod)
    {
        const auto name = core.os->mod_name(*proc, mod);
        if(!name)
            return WALK_NEXT; // FIXME

        const auto span = core.os->mod_span(*proc, mod);
        EXPECT_TRUE(!!span);
        modules.emplace(*name, Module{mod.id, span->addr, span->size, mod.flags});
        return WALK_NEXT;
    });
    EXPECT_NE(modules.size(), 0u);

    const auto it = modules.find(R"(C:\Windows\SYSTEM32\ntdll.dll)");
    EXPECT_NE(it, modules.end());

    const auto [id, addr, size, flags] = it->second;
    EXPECT_NE(id, 0u);
    EXPECT_NE(addr, 0u);
    EXPECT_GT(size, 0u);

    const auto want = addr + (size >> 1);
    const auto mod  = core.os->mod_find(*proc, want);
    EXPECT_TRUE(!!mod);
    EXPECT_EQ(id, mod->id);
}

TEST_F(Win10Test, unable_to_single_step_query_information_process)
{
    const auto target = "ProcessHacker.exe";
    const auto proc   = waiter::proc_wait(core, target, FLAGS_NONE);
    EXPECT_TRUE(!!proc);

    const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_32BIT);
    EXPECT_TRUE(!!ntdll);

    auto loader   = sym::Loader{core, *proc};
    const auto ok = loader.load(*ntdll);
    EXPECT_TRUE(ok);

    wow64::syscalls32 tracer{core, loader.symbols(), "ntdll"};
    bool found = false;
    // ZwQueryInformationProcess in 32-bit has code reading itself
    // we need to ensure we can break this function & resume properly
    // FDP had a bug where this was not possible
    tracer.register_ZwQueryInformationProcess(*proc, [&](wow64::HANDLE, wow64::PROCESSINFOCLASS, wow64::PVOID, wow64::ULONG, wow64::PULONG)
    {
        found = true;
    });
    const auto now = std::chrono::high_resolution_clock::now();
    const auto end = now + std::chrono::seconds(8);
    while(!found && std::chrono::high_resolution_clock::now() < end)
    {
        core.state.resume();
        core.state.wait();
    }
    EXPECT_TRUE(found);
}
