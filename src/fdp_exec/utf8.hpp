#pragma once

#include "types.hpp"

#include <string>

namespace utf8
{
    opt<std::string>  convert(const std::wstring& value);
    opt<std::wstring> convert(const std::string& value);
}
