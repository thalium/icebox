#pragma once

#include "core.hpp"

#ifndef PRIVATE_CORE__
#    error do not include this header directly
#endif

namespace fdp { struct shm; }

namespace core
{
    struct Core::Data
    {
        Data(std::string_view name);

        const std::string name_;
        fdp::shm*         shm_;
    };
} // namespace core
