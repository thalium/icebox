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

#define SYSTEM_PAUSE    \
    if(system("pause")) \
    {                   \
    }

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

    LOG(INFO, "thread : 0x%" PRIx64 "  id:%s %s %s",
        thread.id,
        thread_id <= 4194304 ? std::to_string(thread_id).append(7 - std::to_string(thread_id).length(), ' ').data() : "no",
        std::string("").append(39, ' ').data(),
        thread_pc(core, thread).data());
}

const std::string pad_with(const std::string& arg, int pad)
{
    const auto need = std::max(0, pad - static_cast<int>(arg.size()));
    auto reply      = arg;
    return reply.append(need, ' ');
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

    LOG(INFO, "process: 0x%" PRIx64 " pid:%s parent:%s %s %s   %s %s pgd:0x%" PRIx64 "%s",
        proc.id,
        pad_with(proc_pid <= 4194304 ? std::to_string(proc_pid) : "no", 7).data(),
        pad_with(proc_parent_pid <= 4194304 ? std::to_string(proc_parent_pid) : "error", 7).data(),
        proc_32bits ? "x86" : "x64",
        pad_with(*proc_name, 16).data(),
        leader_thread_pc.data(),
        threads_count > 0 ? ("+" + std::to_string(threads_count) + " threads (" + threads + ")").data() : "",
        proc.dtb.val,
        proc.dtb.val ? "" : " (kernel)");
}

void display_mod(core::Core& core, const proc_t& proc)
{
    core.state.pause();
    core.os->mod_list(proc, [&](mod_t mod)
    {
        const auto span = core.os->mod_span(proc, mod);
        auto name       = core.os->mod_name({}, mod);
        if(!name)
            name = "<no-name>";

        LOG(INFO, "module: 0x%" PRIx64 " %s %s %zd bytes",
            span->addr,
            name->append(32 - name->length(), ' ').data(),
            mod.flags & FLAGS_32BIT ? "x86" : "x64",
            span->size);

        return WALK_NEXT;
    });
    core.state.resume();
}

void display_vm_area(core::Core& core, const proc_t& proc)
{
    core.state.pause();
    core.os->vm_area_list(proc, [&](vm_area_t vm_area)
    {
        const auto span      = core.os->vm_area_span(proc, vm_area);
        const auto type      = core.os->vm_area_type(proc, vm_area);
        std::string type_str = "             ";
        if(type == vma_type_e::main_binary)
            type_str = "[main-binary]";
        else if(type == vma_type_e::heap)
            type_str = "[heap]       ";
        else if(type == vma_type_e::stack)
            type_str = "[stack]      ";
        else if(type == vma_type_e::module)
            type_str = "[module]     ";
        else if(type == vma_type_e::specific_os)
            type_str = "[os-area]    ";

        const auto access      = core.os->vm_area_access(proc, vm_area);
        std::string access_str = "";
        access_str += (access & VMA_ACCESS_READ) ? "r" : "-";
        access_str += (access & VMA_ACCESS_WRITE) ? "w" : "-";
        access_str += (access & VMA_ACCESS_EXEC) ? "x" : "-";
        access_str += (access & VMA_ACCESS_SHARED) ? "s" : "p";

        auto name = core.os->vm_area_name(proc, vm_area);
        if(!name)
            name = "";

        LOG(INFO, "vm_area: 0x%" PRIx64 "-0x%" PRIx64 " %s %s %s",
            span ? span->addr : 0,
            span ? span->addr + span->size : 0,
            access_str.data(),
            type_str.data(),
            name->data());

        return WALK_NEXT;
    });
    core.state.resume();
}

opt<proc_t> select_process(core::Core& core)
{
    while(true)
    {
        int pid;
        std::cout << "Enter a process PID or -1 to skip : ";
        std::cin >> pid;
        while(std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Enter a process PID or -1 to skip : ";
            std::cin >> pid;
        }

        if(pid == -1)
            return {};

        core.state.pause();
        const auto target = core.os->proc_find(pid);
        core.state.resume();
        if(target)
            return *target;

        LOG(ERROR, "unable to find a process with PID %d", pid);
    }
}

void proc_join(core::Core& core, proc_t target, os::join_e mode)
{
    core.state.pause();

    printf("Process found, VM running...\n");
    core.os->proc_join(target, mode);

    const auto thread = core.os->thread_current();
    if(thread)
    {
        std::cout << "Current thread  : ";
        display_thread(core, *thread);
    }
    else
        LOG(ERROR, "no current thread");

    const auto proc = core.os->proc_current();
    if(proc)
    {
        std::cout << "Current process : ";
        display_proc(core, *proc);
    }
    else
        LOG(ERROR, "no current proc");

    printf("\nPress a key to resume VM...\n");
    SYSTEM_PAUSE
    core.state.resume();
}

int main(int argc, char** argv)
{
    logg::init(argc, argv);

    // core initialization
    if(argc != 2)
        return FAIL(-1, "usage: linux <name>");

    SYSTEM_PAUSE

    const auto name = std::string{argv[1]};
    LOG(INFO, "starting on %s", name.data());

    core::Core core;
    const auto ok = core.setup(name);
    core.state.resume();
    if(!ok)
        return FAIL(-1, "unable to start core at %s", name.data());

    SYSTEM_PAUSE
    printf("\n");

    // get list of processes
    core.state.pause();
    core.os->proc_list([&](proc_t proc)
    {
        display_proc(core, proc);
        return WALK_NEXT;
    });
    core.state.resume();

    // proc_join in kernel mode
    printf("\n--- Join a process in kernel mode ---\n");
    auto target = select_process(core);
    if(target)
        proc_join(core, *target, os::JOIN_ANY_MODE);

    // proc_join in user mode
    printf("\n--- Join a process in user mode ---\n");
    target = select_process(core);
    if(target)
        proc_join(core, *target, os::JOIN_USER_MODE);

    printf("\n");
    SYSTEM_PAUSE
    printf("\n");

    // get list of drivers
    core.state.pause();
    core.os->driver_list([&](driver_t driver)
    {
        const auto span = core.os->driver_span(driver);
        auto name       = core.os->driver_name(driver);
        if(!name)
            name = "<no-name>";

        LOG(INFO, "driver: 0x%" PRIx64 " %s %zd bytes",
            span->addr,
            name->append(32 - name->length(), ' ').data(),
            span->size);

        return WALK_NEXT;
    });
    core.state.resume();

    printf("\n");
    SYSTEM_PAUSE
    printf("\n");

    // get list of vm_area
    printf("\n--- Display virtual memory areas and modules of a process ---\n");
    target = select_process(core);
    if(target)
    {
        printf("\nVirtual memory areas :\n");
        display_vm_area(core, *target);

        printf("\n");
        SYSTEM_PAUSE

        printf("\nModules :\n");
        display_mod(core, *target);
    }

    printf("\n");
    SYSTEM_PAUSE
    printf("\n");

    return 0;
}
