#include "utf8.hpp"

#define FDP_MODULE "utf8"
#include "log.hpp"

#define NOMINMAX
#include <windows.h>

opt<std::string> utf8::convert(const std::wstring& value)
{
    const auto src = value.data();
    const auto src_size = static_cast<int>(value.size());
    const auto want = WideCharToMultiByte(CP_UTF8, 0, src, src_size, nullptr, 0, nullptr, nullptr);
    if(!want)
        FAIL(std::nullopt, "unable to convert %S to utf-8", src);

    std::string reply(want, 0);
    const auto size = WideCharToMultiByte(CP_UTF8, 0, src, src_size, reply.data(), want, nullptr, nullptr);
    if(size != want)
        FAIL(std::nullopt, "unable to convert %S to utf-8: got %d, want %d", src, size, want);

    return reply;
}

opt<std::wstring> utf8::convert(const std::string& value)
{
    const auto src = value.data();
    const auto src_size = static_cast<int>(value.size());
    const auto want = MultiByteToWideChar(CP_UTF8, 0, src, src_size, nullptr, 0);
    if(!want)
        FAIL(std::nullopt, "unable to convert %s to utf-8", src);

    std::wstring reply(want, 0);
    const auto size = MultiByteToWideChar(CP_UTF8, 0, src, src_size, reply.data(), want);
    if(size != want)
        FAIL(std::nullopt, "unable to convert %s to unicode: got %d, want %d", src, size, want);

    return reply;
}
