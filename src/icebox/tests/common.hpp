#pragma once

#include <future>
#include <icebox/core.hpp>

#define GTEST_DONT_DEFINE_FAIL 1
#include <gtest/gtest.h>

#define NANOSECOND_NS   1ull
#define MICROSECOND_NS  1000ull * NANOSECOND_NS
#define MILLISECOND_NS  1000ull * MICROSECOND_NS
#define SECOND_NS       1000ull * MILLISECOND_NS
#define MINUTE_NS       60ull * SECOND_NS
#define HOUR_NS         60ull * MINUTE_NS

#define ASSERT_EXEC_BEFORE_TIMEOUT_NS(function, timeout_ns) tests::assert_exec_before_timeout_ns(std::packaged_task<void()>([&] { function; }), timeout_ns)

#define EXPECT_CONDITION_RUNNING_BEFORE_TIMEOUT_NS(core, condition, timeout_ns) EXPECT_TRUE(tests::run_until_or_timeout_ns(core, timeout_ns, [&] { return condition; }));
#define ASSERT_CONDITION_RUNNING_BEFORE_TIMEOUT_NS(core, condition, timeout_ns) ASSERT_TRUE(tests::run_until_or_timeout_ns(core, timeout_ns, [&] { return condition; }));

namespace tests
{
    void    assert_exec_before_timeout_ns   (std::packaged_task<void()> task, const uint64_t timeout_ns);
    bool    is_user_mode                    (core::Core& core);

    bool    run_for_ns          (core::Core& core, const uint64_t duration_ns);
    bool    run_for_ns_with_rand(core::Core& core, const uint64_t min_duration_ns, const uint64_t max_duration_ns);

    bool run_until_or_timeout_ns(core::Core& core, const uint64_t timeout_ns, const std::function<bool()>& predicate);
} // namespace tests
