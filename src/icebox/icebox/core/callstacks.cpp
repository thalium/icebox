#include "callstacks.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "callstacks"
#include "core.hpp"
#include "core_private.hpp"

size_t callstacks::read(core::Core& core, caller_t* callers, size_t num_callers, proc_t proc)
{
    return core.callstacks_->read(callers, num_callers, proc);
}

size_t callstacks::read_from(core::Core& core, caller_t* callers, size_t num_callers, proc_t proc, const context_t& where)
{
    return core.callstacks_->read_from(callers, num_callers, proc, where);
}
