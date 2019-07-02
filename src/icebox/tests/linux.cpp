#define FDP_MODULE "tests_linux"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/os.hpp>
#include <icebox/utils/fnview.hpp>
#include <tests/common.hpp>

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
            bool core_setup = false;
            ASSERT_EXEC_BEFORE_TIMEOUT_NS(core_setup = core.setup("linux"), 30 * SECOND_NS);
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

    ASSERT_EXEC_BEFORE_TIMEOUT_NS(core.os->proc_join(*child, os::JOIN_ANY_MODE), 5 * SECOND_NS);
    auto current = core.os->proc_current();
    EXPECT_TRUE(current);
    EXPECT_EQ(child->id, current->id);
    EXPECT_EQ(child->dtb.val, current->dtb.val);

    ASSERT_TRUE(tests::run_for_ns_with_rand(core, 300 * MILLISECOND_NS, 500 * MILLISECOND_NS)); // multiple slice time (which is 100ms by defaut)

    ASSERT_EXEC_BEFORE_TIMEOUT_NS(core.os->proc_join(*child, os::JOIN_USER_MODE), 5 * SECOND_NS);
    current = core.os->proc_current();
    EXPECT_TRUE(current);
    EXPECT_EQ(child->id, current->id);
    EXPECT_EQ(child->dtb.val, current->dtb.val);
    EXPECT_TRUE(tests::is_user_mode(core));

    ASSERT_TRUE(tests::run_for_ns_with_rand(core, 300 * MILLISECOND_NS, 500 * MILLISECOND_NS)); // multiple slice time (which is 100ms by defaut)
}

TEST_F(LinuxTest, threads)
{
    const auto current = core.os->thread_current();
    ASSERT_TRUE(current && current->id);

    const auto current_pc = core.os->thread_pc({}, *current);
    EXPECT_TRUE(current_pc && *current_pc);
    if(current_pc)
        EXPECT_EQ(current_pc, core.regs.read(FDP_RIP_REGISTER));

    const auto child = utility_child(core);
    ASSERT_TRUE(child && child->id);

    int thread_list_counter = 0;
    core.os->thread_list(*child, [&](thread_t thread)
    {
        EXPECT_NE(thread.id, 0);
        if(!thread.id)
            return WALK_NEXT;

        thread_list_counter++;

        const auto proc = core.os->thread_proc(thread);
        EXPECT_TRUE(proc && proc->id);
        EXPECT_EQ(child->id, proc->id);

        const auto pid = core.os->thread_id({}, thread);
        EXPECT_TRUE(pid <= 4194304); // PID <= 4194304 for linux

        const auto pc = core.os->thread_pc({}, thread);
        EXPECT_TRUE(pc && *pc);
        if(pc)
            if(thread.id != current->id)
                EXPECT_TRUE(core.os->is_kernel_address(*pc));

        return WALK_NEXT;
    });
    ASSERT_EQ(thread_list_counter, 2);
}
