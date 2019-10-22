#pragma once

#include "memory.hpp"
#include "registers.hpp"
#include "state.hpp"
#include "sym.hpp"
#include "types.hpp"

#include <memory>
#include <string_view>

namespace os { struct IModule; }

namespace core
{
    struct Core
    {
         Core();
        ~Core();

        bool setup(std::string_view name);

        // members
        Memory                       mem;
        State                        state;
        std::unique_ptr<os::IModule> os;

        // private data
        struct Data;
        std::unique_ptr<Data> d_;
    };

} // namespace core
