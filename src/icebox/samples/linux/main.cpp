#define FDP_MODULE "linux"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/os.hpp>
#include <icebox/utils/fnview.hpp>

#include <icebox/linux/map.hpp>

void display_thread(const core::Core& core, const thread_t& thread)
{
    const auto thread_id = core.os->thread_id({}, thread);

    LOG(INFO, "thread: {:#x} id:{}",
        thread.id,
        (thread_id <= 4194304) ? std::to_string(thread_id) : "no");
}

void display_proc(const core::Core& core, const proc_t& proc)
{
    const auto proc_pid  = core.os->proc_id(proc);
    const auto proc_name = core.os->proc_name(proc);

    std::string threads;
    int threads_count = -1;
    core.os->thread_list(proc, [&](thread_t thread)
    {
        if(threads_count++ < 0)
            return WALK_NEXT;

        if(threads_count > 1)
            threads.append(", ");

        threads.append(std::to_string(core.os->thread_id({}, thread)));
        return WALK_NEXT;
    });

    LOG(INFO, "process: {:#x} pid:{} '{}' {}",
        proc.id,
        (proc_pid <= 4194304) ? std::to_string(proc_pid) : "no",
        (proc_name) ? *proc_name : "<noname>",
        (threads_count > 0) ? "+" + std::to_string(threads_count) + " threads (" + threads + ")" : "");
}

int main(int argc, char** argv)
{
    logg::init(argc, argv);

    // core initialization
    if(argc != 2)
        return FAIL(-1, "usage: linux <name>");

    const auto name = std::string{argv[1]};
    LOG(INFO, "starting on {}", name.data());

    core::Core core;
    const auto ok = core.setup(name);
    core.state.resume();
    if(!ok)
        return FAIL(-1, "unable to start core at {}", name.data());

    // get list of processes
    core.state.pause();
    core.os->proc_list([&](proc_t proc)
    {
        display_proc(core, proc);
        return WALK_NEXT;
    });
    core.state.resume();

    // get current thread and current process pressing a key
    while(true)
    {
        system("pause");
        core.state.pause();
        const auto thread = core.os->thread_current();
        if(thread)
            display_thread(core, *thread);
        else
            LOG(ERROR, "no current thread");

        const auto proc = core.os->proc_current();
        if(proc)
            display_proc(core, *proc);
        else
            LOG(ERROR, "no current proc");
        core.state.resume();
    }

    core.state.resume();
    return 0;
}
