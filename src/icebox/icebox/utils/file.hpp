#pragma once

#include "icebox/types.hpp"

namespace file
{
    bool write(const fs::path& output, const void* data, size_t size);
}
