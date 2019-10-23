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

            const auto paused = state::pause(core);
            ASSERT_TRUE(paused);
        }

        void TearDown() override
        {
            const auto resumed = state::resume(core);
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
        EXPECT_NE(proc.id, 0u);
        if(!proc.id)
            return WALK_NEXT;

        proc_list_empty = false;

        const auto name = core.os->proc_name(proc);
        EXPECT_TRUE(name && !name->empty());

        if(name && *name == UTILITY_NAME)
        {
            EXPECT_EQ(core.os->proc_flags(proc), FLAGS_32BIT);
        }

        const auto pid = core.os->proc_id(proc);
        EXPECT_LE(pid, 4194304u); // PID <= 4194304 for linux

        if(pid <= 1) // swapper and systemd/initrd
        {
            EXPECT_EQ(core.os->proc_flags(proc), FLAGS_NONE);
        }

        opt<proc_t> children = proc;
        auto children_pid    = pid;
        while(children_pid <= 4194304 && children_pid != 0)
        {
            children = core.os->proc_parent(*children);
            EXPECT_TRUE(children && children->id);
            if(!children)
                break;

            children_pid = core.os->proc_id(*children);
        }
        EXPECT_EQ(children_pid, 0u);

        return WALK_NEXT;
    });
    ASSERT_FALSE(proc_list_empty);

    const auto child = utility_child(core);
    ASSERT_TRUE(child && child->id && child->dtb.val);

    const auto child_find_by_pid = core.os->proc_find(core.os->proc_id(*child));
    EXPECT_EQ(child->id, child_find_by_pid->id);
    EXPECT_EQ(child->dtb.val, child_find_by_pid->dtb.val);

    const auto utility_find_by_name = core.os->proc_find(UTILITY_NAME, FLAGS_NONE);
    EXPECT_TRUE(utility_find_by_name && utility_find_by_name->id && utility_find_by_name->dtb.val);
    if(utility_find_by_name)
    {
        const auto name = core.os->proc_name(*utility_find_by_name);
        EXPECT_TRUE(name);
        if(name)
        {
            EXPECT_EQ(*name, UTILITY_NAME);
        }
    }

    ASSERT_EXEC_BEFORE_TIMEOUT_NS(core.os->proc_join(*child, os::JOIN_ANY_MODE), 5 * SECOND_NS);
    auto current = core.os->proc_current();
    EXPECT_TRUE(current && current->id && current->dtb.val);
    EXPECT_EQ(child->id, current->id);
    EXPECT_EQ(child->dtb.val, current->dtb.val);

    ASSERT_TRUE(tests::run_for_ns_with_rand(core, 300 * MILLISECOND_NS, 500 * MILLISECOND_NS)); // multiple slice time (which is 100ms by defaut)

    ASSERT_EXEC_BEFORE_TIMEOUT_NS(core.os->proc_join(*child, os::JOIN_USER_MODE), 5 * SECOND_NS);
    current = core.os->proc_current();
    EXPECT_TRUE(current && current->id && current->dtb.val);
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
    {
        EXPECT_EQ(current_pc, registers::read(core, FDP_RIP_REGISTER));
    }

    const auto child = utility_child(core);
    ASSERT_TRUE(child && child->id && child->dtb.val);

    int thread_list_counter = 0;
    core.os->thread_list(*child, [&](thread_t thread)
    {
        EXPECT_NE(thread.id, 0u);
        if(!thread.id)
            return WALK_NEXT;

        thread_list_counter++;

        const auto proc = core.os->thread_proc(thread);
        EXPECT_TRUE(proc && proc->id && proc->dtb.val);
        EXPECT_EQ(child->id, proc->id);
        EXPECT_EQ(child->dtb.val, proc->dtb.val);

        const auto pid = core.os->thread_id({}, thread);
        EXPECT_TRUE(pid <= 4194304); // PID <= 4194304 for linux

        const auto pc = core.os->thread_pc({}, thread);
        EXPECT_TRUE(pc && *pc);
        if(pc)
            if(thread.id != current->id)
            {
                EXPECT_TRUE(core.os->is_kernel_address(*pc));
            }

        return WALK_NEXT;
    });
    ASSERT_EQ(thread_list_counter, 2);
}

TEST_F(LinuxTest, drivers)
{
    int driver_list_counter = 0;
    core.os->driver_list([&](driver_t driver)
    {
        EXPECT_NE(driver.id, 0ull);
        if(!driver.id)
            return WALK_NEXT;

        driver_list_counter++;

        const auto name = core.os->driver_name(driver);
        EXPECT_TRUE(name);
        if(name)
        {
            EXPECT_NE(*name, "");
        }

        const auto span = core.os->driver_span(driver);
        EXPECT_TRUE(span);
        if(span)
        {
            EXPECT_NE(span->addr, 0ull);
            EXPECT_TRUE(core.os->is_kernel_address(span->addr));
            EXPECT_NE(span->size, 0ull);
        }

        return WALK_NEXT;
    });
    EXPECT_NE(driver_list_counter, 0);
}

namespace
{
    void test_vma_modules(const core::Core& core, proc_t proc, std::string proc_name, int nb_mod)
    {
        int vma_heap_or_stack = 0;
        core.os->vm_area_list(proc, [&](vm_area_t vm_area)
        {
            EXPECT_NE(vm_area.id, 0ull);
            if(!vm_area.id)
                return WALK_NEXT;

            const auto span = core.os->vm_area_span(proc, vm_area);
            EXPECT_TRUE(span);
            if(span)
            {
                EXPECT_NE(span->addr, 0ull);
                EXPECT_FALSE(core.os->is_kernel_address(span->addr));
                EXPECT_NE(span->size, 0ull);
            }

            const auto name = core.os->vm_area_name(proc, vm_area);
            const auto type = core.os->vm_area_type(proc, vm_area);

            if(type == vma_type_e::heap || type == vma_type_e::stack)
            {
                vma_heap_or_stack++;

                EXPECT_FALSE(name);
                EXPECT_EQ(core.os->vm_area_access(proc, vm_area), VMA_ACCESS_READ + VMA_ACCESS_WRITE);
            }

            if(type == vma_type_e::module && span && span->addr && span->size)
            {
                const auto mod = core.os->mod_find(proc, span->addr + span->size / 2);
                EXPECT_TRUE(mod);

                if(mod && name)
                {
                    EXPECT_EQ(*name, core.os->mod_name(proc, *mod));
                }
            }

            if(type == vma_type_e::main_binary && name)
            {
                EXPECT_EQ(*name, proc_name);
            }

            return WALK_NEXT;
        });
        EXPECT_EQ(vma_heap_or_stack, 2);

        int mod_list_counter = 0;
        opt<std::string> last_mod_name;
        bool first_mod = true;
        core.os->mod_list(proc, [&](mod_t mod)
        {
            EXPECT_NE(mod.id, 0ull);
            if(!mod.id)
                return WALK_NEXT;

            mod_list_counter++;

            const auto span = core.os->mod_span(proc, mod);
            EXPECT_TRUE(span);
            if(span)
            {
                EXPECT_NE(span->addr, 0ull);
                EXPECT_FALSE(core.os->is_kernel_address(span->addr));
                EXPECT_NE(span->size, 0ull);
            }

            const auto last_mod_name = core.os->mod_name(proc, mod);
            EXPECT_TRUE(last_mod_name);

            if(first_mod)
            {
                first_mod = false;
                if(last_mod_name)
                {
                    EXPECT_EQ(*last_mod_name, proc_name);
                }
            }

            return WALK_NEXT;
        });
        ASSERT_EQ(mod_list_counter, nb_mod);

        if(last_mod_name)
        {
            ASSERT_EQ(last_mod_name->substr(0, 3), "ld-");
        }
    }
}

TEST_F(LinuxTest, vma_modules)
{
    const auto child = utility_child(core);
    ASSERT_TRUE(child && child->id && child->dtb.val);

    test_vma_modules(core, *child, "linux_tst_ibx", 4); // 32 bits process

    const auto bash = core.os->proc_parent(*child);
    ASSERT_TRUE(bash && bash->id && bash->dtb.val);

    const auto bash_name = core.os->proc_name(*bash);
    ASSERT_TRUE(bash_name);
    ASSERT_EQ(*bash_name, "sh");

    test_vma_modules(core, *bash, "dash", 3); // 64 bits process
}
