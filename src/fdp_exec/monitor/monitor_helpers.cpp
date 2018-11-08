#include "monitor_helpers.hpp"

#define FDP_MODULE "monitor_helpers"
#include "core/helpers.hpp"

namespace
{
    static const int pointer_size = 0x8;
}

opt<uint64_t> monitor_helpers::get_param_by_index(core::Core& core, int index)
{
    //TODO Deal with x86

    switch(index)
    {
        case 0: return core.regs.read(FDP_RCX_REGISTER);
        case 1: return core.regs.read(FDP_RDX_REGISTER);
        case 2: return core.regs.read(FDP_R8_REGISTER);
        case 3: return core.regs.read(FDP_R9_REGISTER);
        default: return monitor_helpers::get_stack_by_index(core, index + 1);
    }
}

opt<uint64_t> monitor_helpers::get_stack_by_index(core::Core& core, int index)
{

    const auto rsp = core.regs.read(FDP_RSP_REGISTER);
    return core::read_ptr(core, *rsp+ index*pointer_size);
}

opt<uint64_t> monitor_helpers::get_return_value(core::Core& core, proc_t proc)
{
    const auto rsp = core.regs.read(FDP_RSP_REGISTER);
    const auto return_addr = core::read_ptr(core, *rsp+pointer_size);

    {
        const auto bp = core.state.set_breakpoint(*return_addr, proc, core::FILTER_CR3);

        //Should we set a callback ?
        core.state.resume();
        core.state.wait();

        return core.regs.read(FDP_RAX_REGISTER);
    }
}
