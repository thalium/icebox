#include "sanitizer.hpp"

#include <algorithm>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

std::string sanitizer::sanitize_filename(std::string name)
{
    std::string s = name;

#ifdef _MSC_VER
    const auto c1 = '/';
    const auto c2 = '\\';
#else
    const auto c1 = '\\';
    const auto c2 = '/';
    const auto pattern = std::string("C:");

    const auto i = s.find(pattern);
    if (i != std::string::npos)
        s.replace(i, i+pattern.size(), "");
#endif

    std::replace(s.begin(), s.end(), c1, c2);

    return fs::path(s).filename().replace_extension("").generic_string();
}
