#pragma once

#include "errors.hpp"
#include "types.hpp"

namespace file
{
    status_t write(const fs::path& output, const void* data, size_t size);
}
