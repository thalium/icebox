#define FDP_MODULE "tests_linux"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/os.hpp>
#include <icebox/utils/fnview.hpp>

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
