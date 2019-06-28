#define FDP_MODULE "tests_linux"
#include <chrono>
#include <future>
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/os.hpp>
#include <icebox/utils/fnview.hpp>
#include <thread>

#define GTEST_DONT_DEFINE_FAIL 1
#include <gtest/gtest.h>

#define UTILITY_NAME "linux_tst_ibx"

namespace
{
    struct LinuxTest
        : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            const auto core_setup = core.setup("linux");
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

TEST_F(LinuxTest, attach)
{
}

namespace
{
    opt<proc_t> utility_child(core::Core& core)
    {
        std::vector<proc_t> utilities;
        core.os->proc_list([&](proc_t proc)
        {
            const auto name = core.os->proc_name(proc);
            if(name && *name == UTILITY_NAME)
                utilities.push_back(proc);

            return WALK_NEXT;
        });

        if(utilities.empty())
            return FAIL(ext::nullopt, "Execute ./linux_tst_ibx before running tests (see TESTS.md for details)");

        if(utilities.size() > 2)
            return FAIL(ext::nullopt, "Multiple instance of ./linux_tst_ibx found, kill them and execute it only once");

        for(const auto utility : utilities)
        {
            opt<proc_t> proc = utility;
            auto pid         = core.os->proc_id(*proc);

            while(pid <= 4194304 && pid != 0)
            {
                proc = core.os->proc_parent(*proc);
                if(!proc)
                    break;

                const auto name = core.os->proc_name(*proc);
                if(!name)
                    break;

                if(*name == UTILITY_NAME)
                    return utility;

                pid = core.os->proc_id(*proc);
            }
        }

        return {};
    }

    uint8_t cpu_ring(core::Core& core)
    {
        return core.regs.read(FDP_CS_REGISTER) & 0b11ull;
    }

    template <class _Rep,
              class _Per>
    void proc_join_with_timeout(core::Core& core, proc_t proc, os::join_e mode, const std::chrono::duration<_Rep, _Per> timeout)
    {
        std::packaged_task<void()> task([&]
        {
            core.os->proc_join(proc, mode);
        });
        const auto future = task.get_future();

        std::thread thread(std::move(task));
        thread.detach();

        const auto status = future.wait_for(timeout);
        ASSERT_EQ(status, std::future_status::ready);
    }

    template <class _Rep,
              class _Per>
    bool run_for(core::Core& core, const std::chrono::duration<_Rep, _Per> duration)
    {
        if(!core.state.resume())
            return false;
        std::this_thread::sleep_for(duration);
        return core.state.pause();
    }
}

TEST_F(LinuxTest, processes)
{
    bool proc_list_empty = true;
    core.os->proc_list([&](proc_t proc)
    {
        EXPECT_NE(proc.id, 0);
        if(!proc.id)
            return WALK_NEXT;

        proc_list_empty = false;

        const auto name = core.os->proc_name(proc);
        EXPECT_TRUE(!!name & !name->empty());

        if(name && *name == UTILITY_NAME)
            EXPECT_EQ(core.os->proc_flags(proc), FLAGS_32BIT);

        const auto pid = core.os->proc_id(proc);
        EXPECT_TRUE(pid <= 4194304); // PID <= 4194304 for linux

        if(pid <= 1) // swapper and systemd/initrd
            EXPECT_EQ(core.os->proc_flags(proc), FLAGS_NONE);

        opt<proc_t> children = proc;
        auto children_pid    = pid;
        while(children_pid <= 4194304 && children_pid != 0)
        {
            children = core.os->proc_parent(*children);
            EXPECT_TRUE(children);
            if(!children)
                break;

            children_pid = core.os->proc_id(*children);
        }
        EXPECT_EQ(children_pid, 0);

        return WALK_NEXT;
    });
    ASSERT_FALSE(proc_list_empty);

    const auto child = utility_child(core);
    ASSERT_TRUE(child);

    const auto child_find_by_pid = core.os->proc_find(core.os->proc_id(*child));
    EXPECT_EQ(child->id, child_find_by_pid->id);
    EXPECT_EQ(child->dtb.val, child_find_by_pid->dtb.val);

    const auto utility_find_by_name = core.os->proc_find(UTILITY_NAME, FLAGS_NONE);
    EXPECT_TRUE(utility_find_by_name);
    if(utility_find_by_name)
    {
        const auto name = core.os->proc_name(*utility_find_by_name);
        EXPECT_TRUE(name);
        if(name)
            EXPECT_EQ(*name, UTILITY_NAME);
    }

    proc_join_with_timeout(core, *child, os::JOIN_ANY_MODE, std::chrono::seconds(5));
    auto current = core.os->proc_current();
    EXPECT_TRUE(current);
    EXPECT_EQ(child->id, current->id);
    EXPECT_EQ(child->dtb.val, current->dtb.val);

    srand(time(nullptr) & UINT_MAX);
    ASSERT_TRUE(run_for(core, std::chrono::milliseconds(300 + (rand() % 200)))); // run for 300 to 500 ms

    proc_join_with_timeout(core, *child, os::JOIN_USER_MODE, std::chrono::seconds(5));
    current = core.os->proc_current();
    EXPECT_TRUE(current);
    EXPECT_EQ(child->id, current->id);
    EXPECT_EQ(child->dtb.val, current->dtb.val);
    EXPECT_EQ(cpu_ring(core), 3);
    EXPECT_FALSE(core.os->is_kernel_address(core.regs.read(FDP_RIP_REGISTER)));
}
