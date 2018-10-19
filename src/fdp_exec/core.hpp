#pragma once

#include "state.hpp"
#include "memory.hpp"
#include "registers.hpp"
#include "sym.hpp"
#include "types.hpp"

#include <memory>

namespace os { struct IModule; }

namespace core
{
    struct Core
    {
         Core();
        ~Core();

        // members
        Registers                       regs;
        Memory                          mem;
        State                           state;
        sym::Symbols                    sym;
        std::unique_ptr<os::IModule>    os;

        // private data
        struct Data;
        std::unique_ptr<Data> d_;
    };

    bool setup(Core& core, const std::string& name);
}
