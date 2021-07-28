#define FDP_MODULE "winpe_boot"
#include <icebox/core.hpp>
#include <icebox/log.hpp>

int main(int argc, char** argv)
{
    logg::init(argc, argv);
    if(argc != 2)
        return FAIL(-1, "usage: winpe_boot <name>");

    const auto name = std::string{argv[1]};
    LOG(INFO, "starting on %s", name.data());

    while(!core::attach(name))
        continue;

    return 0;
}
