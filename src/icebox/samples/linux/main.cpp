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

std::string thread_pc(core::Core& core, const thread_t& thread)
{
    const auto pc = threads::program_counter(core, {}, thread);

    if(!pc)
        return "<err>";

    auto syms = os::kernel_symbols(core).find("kernel_sym");
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

void display_thread(core::Core& core, const thread_t& thread)
{
    const auto thread_id = threads::tid(core, {}, thread);

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

void display_proc(core::Core& core, const proc_t& proc)
{
    const auto proc_pid      = process::pid(core, proc);
    auto proc_name           = process::name(core, proc);
    const bool proc_32bits   = (process::flags(core, proc) & FLAGS_32BIT);
    const auto proc_parent   = process::parent(core, proc);
    uint64_t proc_parent_pid = 0xffffffffffffffff;
    if(proc_parent)
        proc_parent_pid = process::pid(core, *proc_parent);

    std::string leader_thread_pc;
    std::string threads;
    int threads_count = -1;
    threads::list(core, proc, [&](thread_t thread)
    {
        if(threads_count++ < 0)
        {
            leader_thread_pc = thread_pc(core, thread);
            return WALK_NEXT;
        }

        if(threads_count > 1)
            threads.append(", ");

        threads.append(std::to_string(threads::tid(core, {}, thread)));
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
    state::pause(core);
    modules::list(core, proc, [&](mod_t mod)
    {
        const auto span = modules::span(core, proc, mod);
        auto name       = modules::name(core, {}, mod);
        if(!name)
            name = "<no-name>";

        LOG(INFO, "module: 0x%" PRIx64 " %s %s %zd bytes",
            span->addr,
            name->append(32 - name->length(), ' ').data(),
            mod.flags & FLAGS_32BIT ? "x86" : "x64",
            span->size);

        return WALK_NEXT;
    });
    state::resume(core);
}

void display_vm_area(core::Core& core, const proc_t& proc)
{
    state::pause(core);
    os::vm_area_list(core, proc, [&](vm_area_t vm_area)
    {
        const auto span      = os::vm_area_span(core, proc, vm_area);
        const auto type      = os::vm_area_type(core, proc, vm_area);
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

        const auto access      = os::vm_area_access(core, proc, vm_area);
        std::string access_str = "";
        access_str += (access & VMA_ACCESS_READ) ? "r" : "-";
        access_str += (access & VMA_ACCESS_WRITE) ? "w" : "-";
        access_str += (access & VMA_ACCESS_EXEC) ? "x" : "-";
        access_str += (access & VMA_ACCESS_SHARED) ? "s" : "p";

        auto name = os::vm_area_name(core, proc, vm_area);
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
    state::resume(core);
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

        state::pause(core);
        const auto target = process::find_pid(core, pid);
        state::resume(core);
        if(target)
            return *target;

        LOG(ERROR, "unable to find a process with PID %d", pid);
    }
}

void proc_join(core::Core& core, proc_t target, process::join_e mode)
{
    state::pause(core);

    printf("Process found, VM running...\n");
    process::join(core, target, mode);

    const auto thread = threads::current(core);
    if(thread)
    {
        std::cout << "Current thread  : ";
        display_thread(core, *thread);
    }
    else
        LOG(ERROR, "no current thread");

    const auto proc = process::current(core);
    if(proc)
    {
        std::cout << "Current process : ";
        display_proc(core, *proc);
    }
    else
        LOG(ERROR, "no current proc");

    printf("\nPress a key to resume VM...\n");
    SYSTEM_PAUSE
    state::resume(core);
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

    const auto core = core::attach(name);
    if(!core)
        return FAIL(-1, "unable to start core at %s", name.data());

    state::resume(*core);
    SYSTEM_PAUSE
    printf("\n");

    // get list of processes
    state::pause(*core);
    process::list(*core, [&](proc_t proc)
    {
        display_proc(*core, proc);
        return WALK_NEXT;
    });
    state::resume(*core);

    // proc_join in kernel mode
    printf("\n--- Join a process in kernel mode ---\n");
    auto target = select_process(*core);
    if(target)
        process::join(*core, *target, process::JOIN_ANY_MODE);

    // proc_join in user mode
    printf("\n--- Join a process in user mode ---\n");
    target = select_process(*core);
    if(target)
        process::join(*core, *target, process::JOIN_USER_MODE);

    printf("\n");
    SYSTEM_PAUSE
    printf("\n");

    // get list of drivers
    state::pause(*core);
    os::driver_list(*core, [&](driver_t driver)
    {
        const auto span = os::driver_span(*core, driver);
        auto name       = os::driver_name(*core, driver);
        if(!name)
            name = "<no-name>";

        LOG(INFO, "driver: 0x%" PRIx64 " %s %zd bytes",
            span->addr,
            name->append(32 - name->length(), ' ').data(),
            span->size);

        return WALK_NEXT;
    });
    state::resume(*core);

    printf("\n");
    SYSTEM_PAUSE
    printf("\n");

    // get list of vm_area
    printf("\n--- Display virtual memory areas and modules of a process ---\n");
    target = select_process(*core);
    if(target)
    {
        printf("\nVirtual memory areas :\n");
        display_vm_area(*core, *target);

        printf("\n");
        SYSTEM_PAUSE

        printf("\nModules :\n");
        display_mod(*core, *target);
    }

    printf("\n");
    SYSTEM_PAUSE
    printf("\n");

    return 0;
}
