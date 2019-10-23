#include "threads.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "thread"
#include "core.hpp"
#include "core_private.hpp"
#include "os_private.hpp"

bool threads::list(core::Core& core, proc_t proc, threads::on_thread_fn on_thread)
{
    return core.os_->thread_list(proc, on_thread);
}

opt<thread_t> threads::current(core::Core& core)
{
    return core.os_->thread_current();
}

opt<proc_t> threads::process(core::Core& core, thread_t thread)
{
    return core.os_->thread_proc(thread);
}

opt<uint64_t> threads::program_counter(core::Core& core, proc_t proc, thread_t thread)
{
    return core.os_->thread_pc(proc, thread);
}

uint64_t threads::tid(core::Core& core, proc_t proc, thread_t thread)
{
    return core.os_->thread_id(proc, thread);
}
