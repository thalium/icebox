#define FDP_MODULE "linux"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/os.hpp>
#include <icebox/sym.hpp>
#include <icebox/utils/fnview.hpp>

#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

std::string thread_pc(const core::Core& core, const thread_t& thread)
{
    const auto pc = core.os->thread_pc({}, thread);

    if(!pc)
        return "<err>";

    auto syms = core.os->kernel_symbols().find("kernel_sym");
    opt<sym::ModCursor> cursor;
    const uint64_t START_KERNEL = 0xffffffff80000000, END_KERNEL = 0xfffffffffff00000;

    if(!syms || *pc < START_KERNEL || *pc >= END_KERNEL || !(cursor = syms->symbol(*pc)))
    {
        std::stringstream stream;
        stream << "0x" << std::setw(16) << std::setfill('0') << std::hex << *pc;
        return stream.str();
    }

    std::string internalOffset = "";
    if(*pc != (*cursor).offset)
        internalOffset = "+" + std::to_string(*pc - (*cursor).offset);

    return (*cursor).symbol + internalOffset;
}

void display_thread(const core::Core& core, const thread_t& thread)
{
    const auto thread_id = core.os->thread_id({}, thread);

    LOG(INFO, "thread : {:#x}  id:{} {} {}",
        thread.id,
        (thread_id <= 4194304) ? std::to_string(thread_id).append(7 - std::to_string(thread_id).length(), ' ') : "no",
        std::string("").append(39, ' '),
        thread_pc(core, thread));
}

void display_proc(const core::Core& core, const proc_t& proc)
{
    const auto proc_pid      = core.os->proc_id(proc);
    auto proc_name           = core.os->proc_name(proc);
    const bool proc_32bits   = (core.os->proc_flags(proc) & FLAGS_32BIT);
    const auto proc_parent   = core.os->proc_parent(proc);
    uint64_t proc_parent_pid = 0xffffffffffffffff;
    if(proc_parent)
        proc_parent_pid = core.os->proc_id(*proc_parent);

    std::string leader_thread_pc;
    std::string threads;
    int threads_count = -1;
    core.os->thread_list(proc, [&](thread_t thread)
    {
        if(threads_count++ < 0)
        {
            leader_thread_pc = thread_pc(core, thread);
            return WALK_NEXT;
        }

        if(threads_count > 1)
            threads.append(", ");

        threads.append(std::to_string(core.os->thread_id({}, thread)));
        return WALK_NEXT;
    });

    if(!proc_name)
        proc_name = "<noname>";

    LOG(INFO, "process: {:#x} pid:{} parent:{} {} '{}'{}   {} {} pgd:{:#x}{}",
        proc.id,
        (proc_pid <= 4194304) ? std::to_string(proc_pid).append(7 - std::to_string(proc_pid).length(), ' ') : "no     ",
        (proc_parent_pid <= 4194304) ? std::to_string(proc_parent_pid).append(7 - std::to_string(proc_parent_pid).length(), ' ') : "error  ",
        (proc_32bits) ? "x86" : "x64",
        (*proc_name),
        std::string(16 - (*proc_name).length(), ' '),
        leader_thread_pc,
        (threads_count > 0) ? "+" + std::to_string(threads_count) + " threads (" + threads + ")" : "",
        proc.dtb.val,
        (proc.dtb.val) ? "" : " (kernel)");
}

void display_mod(const core::Core& core, const proc_t& proc)
{
    core.os->mod_list(proc, [&](mod_t mod)
    {
        const auto span = core.os->mod_span(proc, mod);
        auto name       = core.os->mod_name({}, mod);
        if(!name)
            name = "<no-name>";

        LOG(INFO, "module: {:#x} {} {} {} bytes",
            span->addr,
            (*name).append(32 - name->length(), ' '),
            (mod.flags & FLAGS_32BIT) ? "x86" : "x64",
            span->size);

        return WALK_NEXT;
    });
}

int main(int argc, char** argv)
{
    logg::init(argc, argv);

    // core initialization
    if(argc != 2)
        return FAIL(-1, "usage: linux <name>");

    auto ret = system("pause");
    (void) ret;

    const auto name = std::string{argv[1]};
    LOG(INFO, "starting on {}", name.data());

    core::Core core;
    const auto ok = core.setup(name);
    core.state.resume();
    if(!ok)
        return FAIL(-1, "unable to start core at {}", name.data());

    ret = system("pause");
    (void) ret;
    std::cout << "\n";

    // get list of drivers
    core.os->driver_list([&](driver_t driver)
    {
        const auto span = core.os->driver_span(driver);
        auto name       = core.os->driver_name(driver);
        if(!name)
            name = "<no-name>";

        LOG(INFO, "driver: {:#x} {} {} bytes",
            span->addr,
            (*name).append(32 - name->length(), ' '),
            span->size);

        return WALK_NEXT;
    });

    ret = system("pause");
    (void) ret;
    std::cout << "\n";

    // get list of processes
    core.state.pause();
    core.os->proc_list([&](proc_t proc)
    {
        display_proc(core, proc);
        return WALK_NEXT;
    });
    core.state.resume();

    // run until a process given its PID and get its info
    // if PID -1 is given, get current process infos
    while(true)
    {
        int pid;
        std::cout << "\nEnter a process PID or -1 for current process : ";
        std::cin >> pid;
        while(std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Enter a process PID or -1 for current process : ";
            std::cin >> pid;
        }

        core.state.pause();

        if(pid != -1)
        {
            const auto target = core.os->proc_find(pid);
            if(!target)
            {
                LOG(ERROR, "unable to find a process with PID {}", pid);
                core.state.resume();
                continue;
            }

            core.os->proc_join(*target, os::JOIN_USER_MODE);
        }

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

        display_mod(core, *proc);

        core.state.resume();
    }

    core.state.resume();
    return 0;
}
