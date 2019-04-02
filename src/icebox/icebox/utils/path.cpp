#include "path.hpp"

#include <regex>

namespace
{
    const auto re_slashes = std::regex{"[\\\\]+"};
    const auto re_drives  = std::regex{"^[A-Z]:/"};
}

fs::path path::filename(const std::string& filename)
{
    const auto s = std::regex_replace(filename, re_slashes, "/");
    const auto d = std::regex_replace(s, re_drives, "/");
    return fs::path(s).filename();
}
