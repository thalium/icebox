#include "monitor.hpp"

#define FDP_MODULE "monitor"
#include "core.hpp"
#include "log.hpp"
#include "os.hpp"
#include "reader.hpp"

namespace
{
    opt<arg_t> to_arg(const opt<uint64_t>& arg)
    {
        if(!arg)
            return {};

        return arg_t{*arg};
    }

    opt<uint64_t> get_stack_by_index64(core::Core& core, size_t index)
    {
        const auto rsp    = core.regs.read(FDP_RSP_REGISTER);
        const auto reader = reader::make(core);
        return reader.read(rsp + index * sizeof(uint64_t));
    }

    opt<uint64_t> get_stack_by_index32(core::Core& core, size_t index)
    {
        const auto esp    = core.regs.read(FDP_RSP_REGISTER);
        const auto reader = reader::make(core);
        const auto val = reader.le32(esp + index * sizeof(uint32_t));
        if(!val)
            return {};

        return *val & 0x00000000ffffffff;
    }

    opt<arg_t> get_arg_by_index64(core::Core& core, size_t index)
    {
        static const int pointer_size = 0x8;
        switch(index)
        {
            case 0:     return to_arg  (core.regs.read(FDP_RCX_REGISTER));
            case 1:     return to_arg  (core.regs.read(FDP_RDX_REGISTER));
            case 2:     return to_arg  (core.regs.read(FDP_R8_REGISTER));
            case 3:     return to_arg  (core.regs.read(FDP_R9_REGISTER));
            default:    return to_arg  (get_stack_by_index64(core, index + 1));
        }
    }

    opt<arg_t> get_arg_by_index32(core::Core& core, size_t index)
    {
        static const int pointer_size = 0x4;
        return to_arg (get_stack_by_index32(core, index + 1));
    }

    bool set_arg_by_index64(core::Core& core, size_t index, uint64_t value)
    {
        static const int pointer_size = 0x8;
        switch(index)
        {
            case 0:     return core.regs.write(FDP_RCX_REGISTER, value);
            case 1:     return core.regs.write(FDP_RDX_REGISTER, value);
            case 2:     return core.regs.write(FDP_R8_REGISTER, value);
            case 3:     return core.regs.write(FDP_R9_REGISTER, value);
            default:    return monitor::set_stack_by_index(core, index + 1, value);
        }
    }

    bool set_arg_by_index32(core::Core& core, size_t index, uint64_t value)
    {
        // TODO : cast value to uint32_t !!
        return monitor::set_stack_by_index(core, index + 1, value);
    }
}

opt<arg_t> monitor::get_arg_by_index(core::Core& core, size_t index)
{
    if(core.os->proc_ctx_is_x64())
        return get_arg_by_index64(core, index);

    return get_arg_by_index32(core, index);
}

bool monitor::set_arg_by_index(core::Core& core, size_t index, uint64_t value)
{
    if(core.os->proc_ctx_is_x64())
        return set_arg_by_index64(core, index, value);

    return set_arg_by_index32(core, index, value);
}

opt<uint64_t> monitor::get_stack_by_index(core::Core& core, size_t index)
{
    if(core.os->proc_ctx_is_x64())
        return get_stack_by_index64(core, index);

    return get_stack_by_index32(core, index);
}

bool monitor::set_stack_by_index(core::Core& /*core*/, size_t /*index*/, uint64_t /*value*/)
{
    LOG(ERROR, "NOT IMPLEMENTED");
    return false;
}

opt<uint64_t> monitor::get_return_value(core::Core& core, proc_t proc)
{
    const auto reader      = reader::make(core, proc);
    const auto rsp         = core.regs.read(FDP_RSP_REGISTER);
    const auto return_addr = reader.read(rsp);
    const auto thread_curr = core.os->thread_current();

    const auto bp = core.state.set_breakpoint(*return_addr, proc, {});

    // Should we set a callback ?
    uint64_t rip;
    do
    {
        core.state.resume();
        core.state.wait();
        rip = core.regs.read(FDP_RIP_REGISTER);
    } while(*return_addr != rip || thread_curr->id != core.os->thread_current()->id);

    return core.regs.read(FDP_RAX_REGISTER);
}

bool monitor::set_return_value(core::Core& core, uint64_t value)
{
    return core.regs.write(FDP_RAX_REGISTER, value);
}
