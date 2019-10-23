#pragma once

#include <memory>
#include <string>

#include "drivers.hpp"
#include "function.hpp"
#include "memory.hpp"
#include "modules.hpp"
#include "os.hpp"
#include "process.hpp"
#include "registers.hpp"
#include "state.hpp"
#include "symbols.hpp"
#include "threads.hpp"
#include "vm_area.hpp"

namespace core
{
    struct Core;
    std::shared_ptr<Core> attach(const std::string& name);
} // namespace core
