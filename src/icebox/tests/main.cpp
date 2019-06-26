#define FDP_MODULE "tests"
#include <icebox/log.hpp>

#define GTEST_DONT_DEFINE_FAIL 1
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    logg::init(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
