#pragma once

#include <string>

namespace utf8
{
    std::string     from_utf16  (const void* ptr, const void* end);
    std::wstring    to_utf16    (std::string_view src);
} // namespace utf8
