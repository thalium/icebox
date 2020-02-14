#include "nt_os.hpp"

#define FDP_MODULE "nt::threads"
#include "log.hpp"

opt<bpid_t> nt::Os::listen_thread_create(const threads::on_event_fn& on_create)
{
    const auto bp = state::break_on(core_, "PspInsertThread", *symbols_[PspInsertThread], [=]
    {
        const auto thread = registers::read(core_, reg_e::rcx);
        on_create({thread});
    });
    return state::save_breakpoint(core_, bp);
}

opt<bpid_t> nt::Os::listen_thread_delete(const threads::on_event_fn& on_delete)
{
    const auto bp = state::break_on(core_, "PspExitThread", *symbols_[PspExitThread], [=]
    {
        if(const auto thread = thread_current())
            on_delete(*thread);
    });
    return state::save_breakpoint(core_, bp);
}

bool nt::Os::thread_list(proc_t proc, threads::on_thread_fn on_thread)
{
    const auto head = proc.id + offsets_[EPROCESS_ThreadListHead];
    for(auto link = io_.read(head); link && link != head; link = io_.read(*link))
        if(on_thread({*link - offsets_[ETHREAD_ThreadListEntry]}) == walk_e::stop)
            break;

    return true;
}

opt<thread_t> nt::Os::thread_current()
{
    const auto thread = io_.read(kpcr_ + offsets_[KPCR_Prcb] + offsets_[KPRCB_CurrentThread]);
    if(!thread)
        return FAIL(std::nullopt, "unable to read KPCR.Prcb.CurrentThread");

    return thread_t{*thread};
}

opt<proc_t> nt::Os::thread_proc(thread_t thread)
{
    const auto kproc = io_.read(thread.id + offsets_[KTHREAD_Process]);
    if(!kproc)
        return FAIL(std::nullopt, "unable to read KTHREAD.Process");

    const auto eproc = *kproc - offsets_[EPROCESS_Pcb];
    return make_proc(*this, eproc);
}

opt<uint64_t> nt::Os::thread_pc(proc_t /*proc*/, thread_t thread)
{
    const auto ktrap_frame = io_.read(thread.id + offsets_[ETHREAD_Tcb] + offsets_[KTHREAD_TrapFrame]);
    if(!ktrap_frame)
        return FAIL(std::nullopt, "unable to read KTHREAD.TrapFrame");

    if(!*ktrap_frame)
        return {};

    const auto rip = io_.read(*ktrap_frame + offsets_[KTRAP_FRAME_Rip]);
    if(!rip)
        return {};

    return *rip; // rip can be null
}

uint64_t nt::Os::thread_id(proc_t /*proc*/, thread_t thread)
{
    const auto tid = io_.read(thread.id + offsets_[ETHREAD_Cid] + offsets_[CLIENT_ID_UniqueThread]);
    if(!tid)
        return 0;

    return *tid;
}
