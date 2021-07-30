#include "registers.hpp"

#define PRIVATE_CORE_
#include "fdp.hpp"

uint64_t registers::read(core::Core& core, reg_e reg)
{
    const auto ret = fdp::read_register(core, reg);
    return ret ? *ret : 0;
}

bool registers::write(core::Core& core, reg_e reg, uint64_t value)
{
    return fdp::write_register(core, reg, value);
}

uint64_t registers::read_msr(core::Core& core, msr_e reg)
{
    const auto ret = fdp::read_msr_register(core, reg);
    return ret ? *ret : 0;
}

bool registers::write_msr(core::Core& core, msr_e reg, uint64_t value)
{
    return fdp::write_msr_register(core, reg, value);
}

std::string_view registers::to_string(reg_e reg)
{
    switch(reg)
    {
        case reg_e::cs:     return "cs";
        case reg_e::rsp:    return "rsp";
        case reg_e::rip:    return "rip";
        case reg_e::rbp:    return "rbp";
        case reg_e::cr3:    return "cr3";
        case reg_e::cr8:    return "cr8";
        case reg_e::rax:    return "rax";
        case reg_e::rbx:    return "rbx";
        case reg_e::rcx:    return "rcx";
        case reg_e::rdx:    return "rdx";
        case reg_e::rsi:    return "rsi";
        case reg_e::rdi:    return "rdi";
        case reg_e::r8:     return "r8";
        case reg_e::r9:     return "r9";
        case reg_e::r10:    return "r10";
        case reg_e::r11:    return "r11";
        case reg_e::r12:    return "r12";
        case reg_e::r13:    return "r13";
        case reg_e::r14:    return "r14";
        case reg_e::r15:    return "r15";
    }
    return "?";
}

std::string_view registers::to_string(msr_e reg)
{
    switch(reg)
    {
        case msr_e::lstar:          return "lstar";
        case msr_e::fs_base:        return "fs_base";
        case msr_e::gs_base:        return "gs_base";
        case msr_e::kernel_gs_base: return "kernel_gs_base";
    }
    return "?";
}
