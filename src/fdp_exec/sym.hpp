#pragma once

#include "types.hpp"

#include <memory>
#include <optional>

namespace sym
{
    struct IModule
    {
        virtual ~IModule() = default;

        virtual opt<uint64_t> get_offset(const char* symbol) = 0;
        virtual opt<uint64_t> get_struc_member_offset(const char* struc, const char* member) = 0;
    };

    std::unique_ptr<IModule> make_pdb(const char* module, const char* guid);
}