#pragma once

#include "types.hpp"

#include <string>

namespace utf8
{
    std::string convert(const void* ptr, const void* end);
}
