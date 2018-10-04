#pragma once

#include "enums.hpp"
#include "os.hpp"

#include <memory>
#include <optional>
#include <string_view>

// for now assume all adresses can be stored in uint64_t
using addr_t = uint64_t;

struct ICore
{
    virtual ~ICore() = default;

    virtual os::IHelper& os() = 0;

    virtual std::optional<uint64_t> read_msr(msr_e reg) = 0;
    virtual bool                    read_mem(void* dst, addr_t src, size_t size) = 0;
};

std::unique_ptr<ICore> make_core(const std::string_view& name);
