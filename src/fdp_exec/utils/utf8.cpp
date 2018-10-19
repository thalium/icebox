#include "utf8.hpp"

#include <codecvt>
#include <locale>

opt<std::string> utf8::convert(const void* vptr, const void* vend)
{
    // NOTE we have to use uint16_t or suffer incompatibilities with msvc 2017
    std::wstring_convert<std::codecvt_utf8_utf16<uint16_t, 0x10ffff, std::codecvt_mode::little_endian>, uint16_t> convert;
    const auto ptr = reinterpret_cast<const uint16_t*>(vptr);
    const auto end = reinterpret_cast<const uint16_t*>(vend);
    return convert.to_bytes(ptr, end);
}
