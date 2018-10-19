#include "utf8.hpp"

#include <codecvt>
#include <locale>

opt<std::string> utf8::convert(const void* vptr, const void* vend)
{
#define U16_STRING_TYPE char16_t
#ifdef _MSC_VER
#undef  U16_STRING_TYPE
#define U16_STRING_TYPE uint16_t // NOTE we have to use uint16_t or suffer incompatibilities with msvc 2017
#endif
    std::wstring_convert<std::codecvt_utf8_utf16<U16_STRING_TYPE, 0x10ffff, std::codecvt_mode::little_endian>, U16_STRING_TYPE> convert;
    const auto ptr = reinterpret_cast<const U16_STRING_TYPE*>(vptr);
    const auto end = reinterpret_cast<const U16_STRING_TYPE*>(vend);
    return convert.to_bytes(ptr, end);
}
