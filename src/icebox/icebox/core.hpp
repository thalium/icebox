#pragma once

#include <memory>
#include <string>

#include "memory.hpp"
#include "os.hpp"
#include "registers.hpp"
#include "state.hpp"
#include "sym.hpp"

namespace core
{
    struct Core;
    std::shared_ptr<Core> attach(const std::string& name);
} // namespace core
