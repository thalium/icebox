#define FDP_MODULE "tests_linux"
#include <icebox/core.hpp>
#include <icebox/log.hpp>

#define GTEST_DONT_DEFINE_FAIL 1
#include <gtest/gtest.h>

#include <thread>

namespace
{
    constexpr char vm_name[]      = "linux";
    constexpr char bintest_name[] = "linux_tst_ibx";

    struct Linux
        : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            ptr_core = core::attach(vm_name);
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

TEST(Linux_, attach_detach)
{
    for(size_t i = 0; i < 16; ++i)
    {
        const auto core = core::attach(vm_name);
        EXPECT_TRUE(!!core);
        state::resume(*core);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

TEST_F(Linux, attach)
{
}

namespace
{
    opt<proc_t> utility_child(core::Core& core)
    {
        auto utilities = std::vector<proc_t>{};
        process::list(core, [&](proc_t proc)
        {
            const auto name = process::name(core, proc);
            if(name && *name == bintest_name)
                utilities.push_back(proc);

            return walk_e::next;
        });

        if(utilities.empty())
            return FAIL(std::nullopt, "Execute ./linux_tst_ibx before running tests (see TESTS.md for details)");

        if(utilities.size() > 2)
            return FAIL(std::nullopt, "Multiple instance of ./linux_tst_ibx found, kill them and execute it only once");

        for(const auto utility : utilities)
        {
            auto proc = opt<proc_t>{utility};
            auto pid  = process::pid(core, *proc);
            while(pid <= 4194304 && pid != 0)
            {
                proc = process::parent(core, *proc);
                if(!proc)
                    break;

                const auto name = process::name(core, *proc);
                if(!name)
                    break;

                if(*name == bintest_name)
                    return utility;

                pid = process::pid(core, *proc);
            }
        }

        return {};
    }

    bool is_user_mode(core::Core& core)
    {
        const auto cs = registers::read(core, reg_e::cs);
        if(!(cs & 3))
            return false;

        const auto rip = registers::read(core, reg_e::rip);
        return !os::is_kernel_address(core, rip);
    }
}

TEST_F(Linux, processes)
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
        if(name && *name == bintest_name)
        {
            EXPECT_TRUE(process::flags(core, proc).is_x86);
        }

        const auto pid = process::pid(core, proc);
        EXPECT_LE(pid, 4194304u); // PID <= 4194304 for linux

        // swapper and systemd/initrd
        EXPECT_TRUE(pid > 1 || process::flags(core, proc).is_x64);

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
    ASSERT_TRUE(child && child->id && child->udtb.val);

    const auto child_find_by_pid = process::find_pid(core, process::pid(core, *child));
    EXPECT_EQ(child->id, child_find_by_pid->id);
    EXPECT_EQ(child->udtb.val, child_find_by_pid->udtb.val);

    const auto utility_find_by_name = process::find_name(core, bintest_name, {});
    EXPECT_TRUE(utility_find_by_name && utility_find_by_name->id && utility_find_by_name->udtb.val);
    if(utility_find_by_name)
    {
        const auto name = process::name(core, *utility_find_by_name);
        EXPECT_TRUE(name && *name == bintest_name);
    }

    process::join(core, *child, mode_e::kernel);
    auto current = process::current(core);
    EXPECT_TRUE(current && current->id && current->udtb.val);
    EXPECT_EQ(child->id, current->id);
    EXPECT_EQ(child->udtb.val, current->udtb.val);

    process::join(core, *child, mode_e::user);
    current = process::current(core);
    EXPECT_TRUE(current && current->id && current->udtb.val);
    EXPECT_EQ(child->id, current->id);
    EXPECT_EQ(child->udtb.val, current->udtb.val);
    EXPECT_TRUE(is_user_mode(core));
}

TEST_F(Linux, threads)
{
    auto& core         = *ptr_core;
    const auto current = threads::current(core);
    ASSERT_TRUE(current && current->id);

    const auto current_pc = threads::program_counter(core, {}, *current);
    EXPECT_TRUE(current_pc && *current_pc);
    EXPECT_EQ(current_pc, registers::read(core, reg_e::rip));

    const auto child = utility_child(core);
    ASSERT_TRUE(child && child->id && child->udtb.val);

    int thread_list_counter = 0;
    threads::list(core, *child, [&](thread_t thread)
    {
        EXPECT_NE(thread.id, 0u);
        if(!thread.id)
            return walk_e::next;

        thread_list_counter++;

        const auto proc = threads::process(core, thread);
        EXPECT_TRUE(proc && proc->id && proc->udtb.val);
        EXPECT_EQ(child->id, proc->id);
        EXPECT_EQ(child->udtb.val, proc->udtb.val);

        const auto pid = threads::tid(core, {}, thread);
        EXPECT_TRUE(pid <= 4194304); // PID <= 4194304 for linux

        const auto pc = threads::program_counter(core, {}, thread);
        EXPECT_TRUE(pc && *pc);
        EXPECT_TRUE(thread.id == current->id || os::is_kernel_address(core, *pc));
        return walk_e::next;
    });
    ASSERT_EQ(thread_list_counter, 2);
}

TEST_F(Linux, drivers)
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
        EXPECT_TRUE(name && !name->empty());

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
        struct vma_a
        {
            std::string  name;
            span_t       span;
            vma_type_e   type;
            vma_access_e access;
        };
        auto vmas = std::vector<vma_a>{};
        vm_area::list(core, proc, [&](vm_area_t vm_area)
        {
            EXPECT_NE(vm_area.id, 0ull);
            if(!vm_area.id)
                return walk_e::next;

            const auto span   = vm_area::span(core, proc, vm_area);
            const auto name   = vm_area::name(core, proc, vm_area);
            const auto type   = vm_area::type(core, proc, vm_area);
            const auto access = vm_area::access(core, proc, vm_area);
            EXPECT_TRUE(span);

            vmas.push_back(vma_a{name ? *name : "", *span, type, access});
            return walk_e::next;
        });

        auto mod = opt<mod_t>{};
        for(const auto& vma : vmas)
        {
            EXPECT_NE(vma.span.addr, 0U);
            EXPECT_FALSE(os::is_kernel_address(core, vma.span.addr));
            EXPECT_GT(vma.span.size, 0U);
            switch(vma.type)
            {
                case vma_type_e::heap:
                case vma_type_e::stack:
                    vma_heap_or_stack++;
                    EXPECT_TRUE(vma.name.empty());
                    EXPECT_EQ(vma.access, VMA_ACCESS_READ | VMA_ACCESS_WRITE);
                    break;

                case vma_type_e::module:
                    mod = modules::find(core, proc, vma.span.addr + vma.span.size / 2);
                    EXPECT_TRUE(mod);
                    EXPECT_EQ(vma.name, modules::name(core, proc, *mod));
                    break;

                case vma_type_e::other:
                case vma_type_e::none:
                    break;
            }
        }
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
                EXPECT_EQ(last_mod_name, proc_name);
            }
            first_mod = false;
            return walk_e::next;
        });
        ASSERT_EQ(mod_list_counter, nb_mod);
        if(last_mod_name)
        {
            ASSERT_EQ(last_mod_name->substr(0, 3), "ld-");
        }
    }
}

TEST_F(Linux, vma_modules)
{
    auto& core       = *ptr_core;
    const auto child = utility_child(core);
    ASSERT_TRUE(child && child->id && child->udtb.val);

    test_vma_modules(core, *child, "linux_tst_ibx", 4); // 32 bits process
}
