#pragma once

#include "types.hpp"

namespace file
{
    bool write(const fs::path& output, const void* data, size_t size);
}
