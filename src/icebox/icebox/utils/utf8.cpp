#include "utf8.hpp"

#include <codecvt>
#include <locale>

#define U16_STRING_TYPE char16_t
#ifdef _MSC_VER
#    undef U16_STRING_TYPE
#    define U16_STRING_TYPE uint16_t // NOTE we have to use uint16_t or suffer incompatibilities with msvc 2017
#endif

std::string utf8::from_utf16(const void* vptr, const void* vend)
{
    auto        convert = std::wstring_convert<std::codecvt_utf8_utf16<U16_STRING_TYPE, 0x10ffff, std::codecvt_mode::little_endian>, U16_STRING_TYPE>{};
    const auto* ptr     = reinterpret_cast<const U16_STRING_TYPE*>(vptr);
    const auto* end     = reinterpret_cast<const U16_STRING_TYPE*>(vend);
    return convert.to_bytes(ptr, end);
}

std::wstring utf8::to_utf16(std::string_view src)
{
    auto        convert = std::wstring_convert<std::codecvt_utf8_utf16<U16_STRING_TYPE, 0x10ffff, std::codecvt_mode::little_endian>, U16_STRING_TYPE>{};
    const auto* ptr     = src.data();
    const auto  ret     = convert.from_bytes(ptr, ptr + src.size());
    return std::wstring(ret.begin(), ret.end());
}
