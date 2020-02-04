#pragma once

#include <memory>
#include <string>

#include "callstacks.hpp"
#include "drivers.hpp"
#include "functions.hpp"
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

    // attach returns a core from a live vm by name & try to auto-detect os.
    std::shared_ptr<Core> attach(const std::string& name);

    // attach_only returns a core from a live vm by name without os auto-detection.
    std::shared_ptr<Core> attach_only(const std::string& name);

    // detect try to detect current os & possibly change current os helper.
    bool detect(core::Core& core);
} // namespace core
