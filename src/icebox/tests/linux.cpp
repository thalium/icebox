#define FDP_MODULE "tests_linux"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/os.hpp>

#define GTEST_DONT_DEFINE_FAIL 1
#include <gtest/gtest.h>

namespace
{
    struct LinuxTest
        : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            const auto ok = core.setup("linux");
            EXPECT_TRUE(ok);
            if(!ok)
                return;
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

TEST_F(LinuxTest, attach)
{
    core.state.resume();
}
