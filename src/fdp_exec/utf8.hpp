#pragma once

#include <optional>
#include <string>

namespace utf8
{
    std::optional<std::string>  convert(const std::wstring& value);
    std::optional<std::wstring> convert(const std::string& value);
}