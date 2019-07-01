#define FDP_MODULE "tests_common"

#include <chrono>
#include <random>
#include <thread>

#include <icebox/log.hpp>
#include <icebox/os.hpp>
#include <tests/common.hpp>

#define GTEST_DONT_DEFINE_FAIL 1
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    logg::init(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

void tests::assert_exec_before_timeout_ns(std::packaged_task<void()> task, const uint64_t timeout_ns)
{
    const auto future = task.get_future();

    std::thread thread(std::move(task));
    thread.detach();

    std::future_status status = future.wait_for(std::chrono::nanoseconds(timeout_ns));
    ASSERT_EQ(status, std::future_status::ready);
}

namespace
{
    uint8_t cpu_ring(core::Core& core)
    {
        return core.regs.read(FDP_CS_REGISTER) & 0b11ull;
    }

    bool is_rip_in_kernel(core::Core& core)
    {
        return core.os->is_kernel_address(core.regs.read(FDP_RIP_REGISTER));
    }
}

bool tests::is_user_mode(core::Core& core)
{
    return (cpu_ring(core) == 3) & !is_rip_in_kernel(core);
}

bool tests::run_for_ns(core::Core& core, const uint64_t duration_ns)
{
    if(!core.state.resume())
        return false;
    std::this_thread::sleep_for(std::chrono::nanoseconds(duration_ns));
    return core.state.pause();
}

bool tests::run_for_ns_with_rand(core::Core& core, const uint64_t min_duration_ns, const uint64_t max_duration_ns)
{
    std::random_device rand_dev;
    std::mt19937                            generator   (rand_dev());
    std::uniform_int_distribution<uint64_t> distr       (min_duration_ns, max_duration_ns);

    return run_for_ns(core, distr(generator));
}
