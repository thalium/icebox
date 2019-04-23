#define FDP_MODULE "linux"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/os.hpp>
#include <icebox/utils/fnview.hpp>

int main(int argc, char** argv)
{
    logg::init(argc, argv);
    /*if(argc != 2)
        return FAIL(-1, "usage: linux <name>");

    const auto name = std::string{argv[1]};
    LOG(INFO, "starting on {}", name.data());

    core::Core core;
    const auto ok = core.setup(name);
    if(!ok)
        return FAIL(-1, "unable to start core at {}", name.data());*/

    // lit le .dwarf
    const auto ptr = sym::make_dwarf({}, {}, {});

    const auto size = ptr->struc_size("task_struct");
    LOG(INFO, "task_struct size : {} bytes", size ? *size : -1);

    const auto offest_mm = ptr->struc_offset("task_struct", "mm");
    LOG(INFO, "offset of mm in task_struct : {:#x}", offest_mm ? *offest_mm : -1);

    // liste les processus
    /*core.state.pause();
    LOG(INFO, "processes:");
    core.os->proc_list([&](proc_t proc)
    {
        const auto proc_pid  = core.os->proc_id(proc);
        const auto proc_name = core.os->proc_name(proc);
        LOG(INFO, "proc: {:#x} pid:{} '{}'", proc.id, proc_pid ? proc_pid : 0, proc_name ? proc_name->data() : "<noname>");
        return WALK_NEXT;
    });
    core.state.resume();
    return 0;*/

    // Affiche le proc current a chaque appui de touche
    /*while(true)
    {
        core.state.pause();
        const auto proc = core.os->proc_current();
        if(proc)
        {
            const auto proc_pid = core.os->proc_id(*proc);
            LOG(INFO, "current proc: {:#x} pid:{}", (*proc).id, proc_pid ? proc_pid : 0);
        }
        core.state.resume();
        system("pause");
    }
    return 0;*/
}
