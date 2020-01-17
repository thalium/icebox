#pragma once

#include <chrono>
#include <string>

namespace bench
{
    struct Log
    {
        using timestamp_t = decltype(std::chrono::high_resolution_clock::now());

        Log(const char* name)
            : begin(std::chrono::high_resolution_clock::now())
            , name(name)
        {
        }

        ~Log()
        {
            const auto end      = std::chrono::high_resolution_clock::now();
            const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            LOG(INFO, "%s: %" PRId64 " ms", name.data(), duration);
        }

        const timestamp_t begin;
        const std::string name;
    };
} // namespace bench
