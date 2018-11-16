#include "monitor_helpers.hpp"

#define FDP_MODULE "monitor_helpers"
#include "core/helpers.hpp"

namespace
{
    static const int pointer_size = 0x8;
}

return_t<arg_t> monitor_helpers::get_param_by_index(core::Core& core, int index)
{
    //TODO Deal with x86
    arg_t arg;
    return_t<uint64_t> res;
    switch(index)
    {
        case 0: res = core.regs.read(FDP_RCX_REGISTER); break;
        case 1: res = core.regs.read(FDP_RDX_REGISTER); break;
        case 2: res = core.regs.read(FDP_R8_REGISTER);  break;
        case 3: res = core.regs.read(FDP_R9_REGISTER);  break;
        default: res = monitor_helpers::get_stack_by_index(core, index + 1);
    }
    if (!res)
        return {};

    arg.val = *res;
    return arg;
}

return_t<uint64_t> monitor_helpers::get_stack_by_index(core::Core& core, int index)
{

    const auto rsp = core.regs.read(FDP_RSP_REGISTER);
    return core::read_ptr(core, *rsp+ index*pointer_size);
}

return_t<uint64_t> monitor_helpers::get_return_value(core::Core& core, proc_t proc)
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
