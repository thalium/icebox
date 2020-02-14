#include "nt_os.hpp"

#define FDP_MODULE "nt::process"
#include "log.hpp"
#include "nt.hpp"
#include "utils/path.hpp"

namespace
{
    opt<dtb_t> read_user_dtb(nt::Os& os, uint64_t kprocess)
    {
        const auto dtb = os.io_.read(kprocess + os.offsets_[KPROCESS_UserDirectoryTableBase]);
        if(dtb && *dtb != 0 && *dtb != 1)
            return dtb_t{*dtb};

        if(os.offsets_[KPROCESS_DirectoryTableBase] == os.offsets_[KPROCESS_UserDirectoryTableBase])
            return {};

        const auto kdtb = os.io_.read(kprocess + os.offsets_[KPROCESS_DirectoryTableBase]);
        if(!kdtb)
            return {};

        return dtb_t{*kdtb};
    }
}

opt<proc_t> nt::make_proc(nt::Os& os, uint64_t eproc)
{
    const auto dtb = read_user_dtb(os, eproc + os.offsets_[EPROCESS_Pcb]);
    if(!dtb)
        return {};

    const auto kdtb = os.io_.read(eproc + os.offsets_[EPROCESS_Pcb] + os.offsets_[KPROCESS_DirectoryTableBase]);
    if(!kdtb)
        return {};

    return proc_t{eproc, dtb_t{*kdtb}, *dtb};
}

bool nt::Os::proc_list(process::on_proc_fn on_process)
{
    const auto head = *symbols_[PsActiveProcessHead];
    for(auto link = io_.read(head); link != head; link = io_.read(*link))
    {
        const auto eproc = *link - offsets_[EPROCESS_ActiveProcessLinks];
        const auto proc  = make_proc(*this, eproc);
        if(!proc)
            continue;

        const auto err = on_process(*proc);
        if(err == walk_e::stop)
            break;
    }
    return true;
}

opt<proc_t> nt::Os::proc_current()
{
    const auto current = thread_current();
    if(!current)
        return FAIL(std::nullopt, "unable to get current thread");

    return thread_proc(*current);
}

opt<proc_t> nt::Os::proc_find(std::string_view name, flags_t flags)
{
    opt<proc_t> found;
    proc_list([&](proc_t proc)
    {
        const auto got = proc_name(proc);
        if(*got != name)
            return walk_e::next;

        const auto f = proc_flags(proc);
        if(!os::check_flags(f, flags))
            return walk_e::next;

        if(!proc_is_valid(proc))
            return walk_e::next;

        found = proc;
        return walk_e::stop;
    });
    return found;
}

opt<proc_t> nt::Os::proc_find(uint64_t pid)
{
    opt<proc_t> found;
    proc_list([&](proc_t proc)
    {
        const auto got = proc_id(proc);
        if(got != pid)
            return walk_e::next;

        found = proc;
        return walk_e::stop;
    });
    return found;
}

opt<std::string> nt::Os::proc_name(proc_t proc)
{
    // EPROCESS.ImageFileName is 16 bytes, but only 14 are actually used
    auto buffer   = std::array<char, 14 + 1>{};
    const auto ok = io_.read_all(&buffer[0], proc.id + offsets_[EPROCESS_ImageFileName], sizeof buffer);
    buffer.back() = 0;
    if(!ok)
        return {};

    const auto name = std::string{&buffer[0]};
    if(name.size() < sizeof buffer - 1)
        return name;

    const auto image_file_name = io_.read(proc.id + offsets_[EPROCESS_SeAuditProcessCreationInfo] + offsets_[SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName]);
    if(!image_file_name)
        return name;

    const auto path = nt::read_unicode_string(io_, *image_file_name + offsets_[OBJECT_NAME_INFORMATION_Name]);
    if(!path)
        return name;

    return path::filename(*path).generic_string();
}

uint64_t nt::Os::proc_id(proc_t proc)
{
    const auto pid = io_.read(proc.id + offsets_[EPROCESS_UniqueProcessId]);
    if(!pid)
        return 0;

    return *pid;
}

opt<bpid_t> nt::Os::listen_proc_create(const process::on_event_fn& on_create)
{
    const auto bp = state::break_on_physical(core_, "LdrpInitializeProcess", LdrpInitializeProcess_, [=]
    {
        const auto proc = process::current(core_);
        if(!proc)
            return;

        on_create(*proc);
    });
    return state::save_breakpoint(core_, bp);
}

opt<bpid_t> nt::Os::listen_proc_delete(const process::on_event_fn& on_delete)
{
    const auto bp = state::break_on(core_, "PspExitProcess", *symbols_[PspExitProcess], [=]
    {
        const auto eproc       = registers::read(core_, reg_e::rdx);
        const auto head        = eproc + offsets_[EPROCESS_ThreadListHead];
        const auto item        = io_.read(head);
        const auto has_threads = item && item != head;
        if(!has_threads)
            if(const auto proc = make_proc(*this, eproc))
                on_delete(*proc);
    });
    return state::save_breakpoint(core_, bp);
}

bool nt::Os::proc_is_valid(proc_t proc)
{
    const auto io       = memory::make_io(core_, proc);
    const auto vad_root = nt::read_vad_root_addr(*this, io, proc, offsets_[EPROCESS_VadRoot]);
    return vad_root && *vad_root;
}

flags_t nt::Os::proc_flags(proc_t proc)
{
    const auto io    = memory::make_io(core_, proc);
    auto flags       = flags_t{};
    const auto wow64 = io.read(proc.id + offsets_[EPROCESS_Wow64Process]);
    if(*wow64)
        flags.is_x86 = true;
    else
        flags.is_x64 = true;
    return flags;
}

namespace
{
    template <typename T>
    void break_on_any_return_of(nt::Os& os, proc_t proc, const std::string& name, uint64_t addr, const T& read_ret_addr)
    {
        auto hit      = false;
        auto rets     = std::unordered_set<uint64_t>{};
        auto bps      = std::vector<state::Breakpoint>{};
        const auto bp = state::break_on_process(os.core_, name, proc, addr, [&]
        {
            const auto opt_ret = read_ret_addr();
            if(!opt_ret)
                return;

            const auto ret_addr = *opt_ret;
            if(rets.count(ret_addr))
                return;

            const auto bp_ret = state::break_on_process(os.core_, name + " return", proc, ret_addr, [&]
            {
                hit = true;
            });
            bps.emplace_back(bp_ret);
            rets.insert(ret_addr);
        });
        while(!hit)
            state::exec(os.core_);
    }

    void proc_join_kernel(nt::Os& os, proc_t proc)
    {
        while(true)
        {
            state::run_to_cr_write(os.core_, reg_e::cr3);
            const auto curr = process::current(os.core_);
            if(curr && curr->id == proc.id)
                return;
        }
    }

    void proc_join_user(nt::Os& os, proc_t proc)
    {
        // if KiKernelSysretExit doesn't exist, KiSystemCall* in lstar has user return address in rcx
        const auto where = os.symbols_[KiKernelSysretExit] ? *os.symbols_[KiKernelSysretExit] : registers::read_msr(os.core_, msr_e::lstar);
        break_on_any_return_of(os, proc, "KiKernelSysretExit", where, [&]
        {
            return std::make_optional(registers::read(os.core_, reg_e::rcx));
        });
    }
}

bool nt::is_user_mode(uint64_t cs)
{
    return !!(cs & 3);
}

void nt::Os::proc_join(proc_t proc, mode_e mode)
{
    const auto current   = proc_current();
    const auto same_proc = current && current->id == proc.id;
    const auto user_mode = is_user_mode(registers::read(core_, reg_e::cs));
    if(mode == mode_e::kernel && same_proc && !user_mode)
        return;

    if(mode == mode_e::kernel)
        return proc_join_kernel(*this, proc);

    if(same_proc && user_mode)
        return;

    return proc_join_user(*this, proc);
}

opt<proc_t> nt::Os::proc_parent(proc_t proc)
{
    const auto io         = memory::make_io(core_, proc);
    const auto parent_pid = io.read(proc.id + offsets_[EPROCESS_InheritedFromUniqueProcessId]);
    if(!parent_pid)
        return {};

    return proc_find(*parent_pid);
}
