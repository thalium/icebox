#pragma once

#include "enums.hpp"
#include "os.hpp"
#include "sym.hpp"
#include "types.hpp"

#include <memory>
#include <optional>
#include <string_view>

struct ICore
{
    virtual ~ICore() = default;

    virtual os::IHelper&    os() = 0;
    virtual sym::IHandler&  sym() = 0;

    virtual bool pause() = 0;
    virtual bool resume() = 0;

    virtual opt<uint64_t>   read_msr    (msr_e reg) = 0;
    virtual bool            write_msr   (msr_e reg, uint64_t value) = 0;
    virtual opt<uint64_t>   read_reg    (reg_e reg) = 0;
    virtual bool            write_reg   (reg_e reg, uint64_t value) = 0;
    virtual bool            read_mem    (void* dst, uint64_t src, size_t size) = 0;
    virtual bool            write_mem   (uint64_t dst, const void* src, size_t size) = 0;

    struct Ctx;
    virtual std::shared_ptr<Ctx> switch_context(proc_t proc) = 0;
};

std::unique_ptr<ICore> make_core(const std::string_view& name);
