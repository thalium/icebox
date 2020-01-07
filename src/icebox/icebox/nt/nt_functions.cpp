#include "nt_os.hpp"

#define FDP_MODULE "nt::functions"
#include "log.hpp"

namespace
{
    constexpr auto x86_cs = 0x23;

    opt<arg_t> to_arg(opt<uint64_t> arg)
    {
        if(!arg)
            return {};

        return arg_t{*arg};
    }

    opt<arg_t> read_stack32(const memory::Io& io, uint64_t sp, size_t index)
    {
        return to_arg(io.le32(sp + index * sizeof(uint32_t)));
    }

    bool write_stack32(core::Core& /*core*/, size_t /*index*/, uint32_t /*arg*/)
    {
        LOG(ERROR, "not implemented");
        return false;
    }

    opt<arg_t> read_stack64(const memory::Io& io, uint64_t sp, size_t index)
    {
        return to_arg(io.le64(sp + index * sizeof(uint64_t)));
    }

    bool write_stack64(core::Core& /*core*/, size_t /*index*/, uint64_t /*arg*/)
    {
        LOG(ERROR, "not implemented");
        return false;
    }

    opt<arg_t> read_arg32(const memory::Io& io, uint64_t sp, size_t index)
    {
        return read_stack32(io, sp, index + 1);
    }

    bool write_arg32(core::Core& core, size_t index, arg_t arg)
    {
        return write_stack32(core, index + 1, static_cast<uint32_t>(arg.val));
    }

    opt<arg_t> read_arg64(core::Core& core, const memory::Io& io, uint64_t sp, size_t index)
    {
        switch(index)
        {
            case 0:     return to_arg      (registers::read(core, reg_e::rcx));
            case 1:     return to_arg      (registers::read(core, reg_e::rdx));
            case 2:     return to_arg      (registers::read(core, reg_e::r8));
            case 3:     return to_arg      (registers::read(core, reg_e::r9));
            default:    return read_stack64(io, sp, index + 1);
        }
    }

    bool write_arg64(core::Core& core, size_t index, arg_t arg)
    {
        switch(index)
        {
            case 0:     return registers::write(core, reg_e::rcx, arg.val);
            case 1:     return registers::write(core, reg_e::rdx, arg.val);
            case 2:     return registers::write(core, reg_e::r8, arg.val);
            case 3:     return registers::write(core, reg_e::r9, arg.val);
            default:    return write_stack64(core, index + 1, arg.val);
        }
    }
}

opt<arg_t> nt::Os::read_stack(size_t index)
{
    const auto cs       = registers::read(core_, reg_e::cs);
    const auto is_32bit = cs == x86_cs;
    const auto sp       = registers::read(core_, reg_e::rsp);
    const auto io       = memory::make_io_current(core_);
    if(is_32bit)
        return read_stack32(io, sp, index);

    return read_stack64(io, sp, index);
}

opt<arg_t> nt::Os::read_arg(size_t index)
{
    const auto cs       = registers::read(core_, reg_e::cs);
    const auto is_32bit = cs == x86_cs;
    const auto sp       = registers::read(core_, reg_e::rsp);
    const auto io       = memory::make_io_current(core_);
    if(is_32bit)
        return read_arg32(io, sp, index);

    return read_arg64(core_, io, sp, index);
}

bool nt::Os::write_arg(size_t index, arg_t arg)
{
    const auto cs       = registers::read(core_, reg_e::cs);
    const auto is_32bit = cs == x86_cs;
    if(is_32bit)
        return write_arg32(core_, index, arg);

    return write_arg64(core_, index, arg);
}
