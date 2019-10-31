#define FDP_MODULE "tests_linux"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/os.hpp>
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
            ASSERT_EXEC_BEFORE_TIMEOUT_NS(ptr_core = core::attach("linux"), 30 * SECOND_NS);
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

TEST_F(LinuxTest, attach)
{
}

namespace
{
    opt<proc_t> utility_child(core::Core& core)
    {
        std::vector<proc_t> utilities;
        process::list(core, [&](proc_t proc)
        {
            const auto name = process::name(core, proc);
            if(name && *name == UTILITY_NAME)
                utilities.push_back(proc);

            return walk_e::next;
        });

        if(utilities.empty())
            return FAIL(ext::nullopt, "Execute ./linux_tst_ibx before running tests (see TESTS.md for details)");

        if(utilities.size() > 2)
            return FAIL(ext::nullopt, "Multiple instance of ./linux_tst_ibx found, kill them and execute it only once");

        for(const auto utility : utilities)
        {
            opt<proc_t> proc = utility;
            auto pid         = process::pid(core, *proc);

            while(pid <= 4194304 && pid != 0)
            {
                proc = process::parent(core, *proc);
                if(!proc)
                    break;

                const auto name = process::name(core, *proc);
                if(!name)
                    break;

                if(*name == UTILITY_NAME)
                    return utility;

                pid = process::pid(core, *proc);
            }
        }

        return {};
    }
}

TEST_F(LinuxTest, processes)
{
    auto& core           = *ptr_core;
    bool proc_list_empty = true;
    process::list(core, [&](proc_t proc)
    {
        EXPECT_NE(proc.id, 0u);
        if(!proc.id)
            return walk_e::next;

        proc_list_empty = false;

        const auto name = process::name(core, proc);
        EXPECT_TRUE(name && !name->empty());

        if(name && *name == UTILITY_NAME)
        {
            EXPECT_TRUE(process::flags(core, proc).is_x86);
        }

        const auto pid = process::pid(core, proc);
        EXPECT_LE(pid, 4194304u); // PID <= 4194304 for linux

        if(pid <= 1) // swapper and systemd/initrd
        {
            EXPECT_TRUE(process::flags(core, proc).is_x64);
        }

        opt<proc_t> children = proc;
        auto children_pid    = pid;
        while(children_pid <= 4194304 && children_pid != 0)
        {
            children = process::parent(core, *children);
            EXPECT_TRUE(children && children->id);
            if(!children)
                break;

            children_pid = process::pid(core, *children);
        }
        EXPECT_EQ(children_pid, 0u);

        return walk_e::next;
    });
    ASSERT_FALSE(proc_list_empty);

    const auto child = utility_child(core);
    ASSERT_TRUE(child && child->id && child->dtb.val);

    const auto child_find_by_pid = process::find_pid(core, process::pid(core, *child));
    EXPECT_EQ(child->id, child_find_by_pid->id);
    EXPECT_EQ(child->dtb.val, child_find_by_pid->dtb.val);

    const auto utility_find_by_name = process::find_name(core, UTILITY_NAME, {});
    EXPECT_TRUE(utility_find_by_name && utility_find_by_name->id && utility_find_by_name->dtb.val);
    if(utility_find_by_name)
    {
        const auto name = process::name(core, *utility_find_by_name);
        EXPECT_TRUE(name);
        if(name)
        {
            EXPECT_EQ(*name, UTILITY_NAME);
        }
    }

    ASSERT_EXEC_BEFORE_TIMEOUT_NS(process::join(core, *child, mode_e::kernel), 5 * SECOND_NS);
    auto current = process::current(core);
    EXPECT_TRUE(current && current->id && current->dtb.val);
    EXPECT_EQ(child->id, current->id);
    EXPECT_EQ(child->dtb.val, current->dtb.val);

    ASSERT_TRUE(tests::run_for_ns_with_rand(core, 300 * MILLISECOND_NS, 500 * MILLISECOND_NS)); // multiple slice time (which is 100ms by defaut)

    ASSERT_EXEC_BEFORE_TIMEOUT_NS(process::join(core, *child, mode_e::user), 5 * SECOND_NS);
    current = process::current(core);
    EXPECT_TRUE(current && current->id && current->dtb.val);
    EXPECT_EQ(child->id, current->id);
    EXPECT_EQ(child->dtb.val, current->dtb.val);
    EXPECT_TRUE(tests::is_user_mode(core));

    ASSERT_TRUE(tests::run_for_ns_with_rand(core, 300 * MILLISECOND_NS, 500 * MILLISECOND_NS)); // multiple slice time (which is 100ms by defaut)
}

TEST_F(LinuxTest, threads)
{
    auto& core         = *ptr_core;
    const auto current = threads::current(core);
    ASSERT_TRUE(current && current->id);

    const auto current_pc = threads::program_counter(core, {}, *current);
    EXPECT_TRUE(current_pc && *current_pc);
    if(current_pc)
    {
        EXPECT_EQ(current_pc, registers::read(core, reg_e::rip));
    }

    const auto child = utility_child(core);
    ASSERT_TRUE(child && child->id && child->dtb.val);

    int thread_list_counter = 0;
    threads::list(core, *child, [&](thread_t thread)
    {
        EXPECT_NE(thread.id, 0u);
        if(!thread.id)
            return walk_e::next;

        thread_list_counter++;

        const auto proc = threads::process(core, thread);
        EXPECT_TRUE(proc && proc->id && proc->dtb.val);
        EXPECT_EQ(child->id, proc->id);
        EXPECT_EQ(child->dtb.val, proc->dtb.val);

        const auto pid = threads::tid(core, {}, thread);
        EXPECT_TRUE(pid <= 4194304); // PID <= 4194304 for linux

        const auto pc = threads::program_counter(core, {}, thread);
        EXPECT_TRUE(pc && *pc);
        if(pc)
            if(thread.id != current->id)
            {
                EXPECT_TRUE(os::is_kernel_address(core, *pc));
            }

        return walk_e::next;
    });
    ASSERT_EQ(thread_list_counter, 2);
}

TEST_F(LinuxTest, drivers)
{
    auto& core              = *ptr_core;
    int driver_list_counter = 0;
    drivers::list(core, [&](driver_t driver)
    {
        EXPECT_NE(driver.id, 0ull);
        if(!driver.id)
            return walk_e::next;

        driver_list_counter++;

        const auto name = drivers::name(core, driver);
        EXPECT_TRUE(name);
        if(name)
        {
            EXPECT_NE(*name, "");
        }

        const auto span = drivers::span(core, driver);
        EXPECT_TRUE(span);
        if(span)
        {
            EXPECT_NE(span->addr, 0ull);
            EXPECT_TRUE(os::is_kernel_address(core, span->addr));
            EXPECT_NE(span->size, 0ull);
        }

        return walk_e::next;
    });
    EXPECT_NE(driver_list_counter, 0);
}

namespace
{
    void test_vma_modules(core::Core& core, proc_t proc, std::string proc_name, int nb_mod)
    {
        int vma_heap_or_stack = 0;
        vm_area::list(core, proc, [&](vm_area_t vm_area)
        {
            EXPECT_NE(vm_area.id, 0ull);
            if(!vm_area.id)
                return walk_e::next;

            const auto span = vm_area::span(core, proc, vm_area);
            EXPECT_TRUE(span);
            if(span)
            {
                EXPECT_NE(span->addr, 0ull);
                EXPECT_FALSE(os::is_kernel_address(core, span->addr));
                EXPECT_NE(span->size, 0ull);
            }

            const auto name = vm_area::name(core, proc, vm_area);
            const auto type = vm_area::type(core, proc, vm_area);

            if(type == vma_type_e::heap || type == vma_type_e::stack)
            {
                vma_heap_or_stack++;

                EXPECT_FALSE(name);
                EXPECT_EQ(vm_area::access(core, proc, vm_area), VMA_ACCESS_READ + VMA_ACCESS_WRITE);
            }

            if(type == vma_type_e::module && span && span->addr && span->size)
            {
                const auto mod = modules::find(core, proc, span->addr + span->size / 2);
                EXPECT_TRUE(mod);

                if(mod && name)
                {
                    EXPECT_EQ(*name, modules::name(core, proc, *mod));
                }
            }

            if(type == vma_type_e::binary && name)
            {
                EXPECT_EQ(*name, proc_name);
            }

            return walk_e::next;
        });
        EXPECT_EQ(vma_heap_or_stack, 2);

        int mod_list_counter = 0;
        opt<std::string> last_mod_name;
        bool first_mod = true;
        modules::list(core, proc, [&](mod_t mod)
        {
            EXPECT_NE(mod.id, 0ull);
            if(!mod.id)
                return walk_e::next;

            mod_list_counter++;

            const auto span = modules::span(core, proc, mod);
            EXPECT_TRUE(span);
            if(span)
            {
                EXPECT_NE(span->addr, 0ull);
                EXPECT_FALSE(os::is_kernel_address(core, span->addr));
                EXPECT_NE(span->size, 0ull);
            }

            const auto last_mod_name = modules::name(core, proc, mod);
            EXPECT_TRUE(last_mod_name);

            if(first_mod)
            {
                first_mod = false;
                if(last_mod_name)
                {
                    EXPECT_EQ(*last_mod_name, proc_name);
                }
            }

            return walk_e::next;
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
    auto& core       = *ptr_core;
    const auto child = utility_child(core);
    ASSERT_TRUE(child && child->id && child->dtb.val);

    test_vma_modules(core, *child, "linux_tst_ibx", 4); // 32 bits process

    const auto bash = process::parent(core, *child);
    ASSERT_TRUE(bash && bash->id && bash->dtb.val);

    const auto bash_name = process::name(core, *bash);
    ASSERT_TRUE(bash_name);
    ASSERT_EQ(*bash_name, "sh");

    test_vma_modules(core, *bash, "dash", 3); // 64 bits process
}
