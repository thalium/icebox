#pragma once

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace pcap
{
    struct metadata_t
    {
        uint32_t    if_id;
        uint64_t    timestamp;
        uint32_t    sec;
        uint32_t    usec;
        std::string comment;
    };

    struct Writer
    {
         Writer();
        ~Writer();

        void    add_packet  (const metadata_t& p, const void* data, size_t size);
        bool    write       (const std::string& filepath);

        struct Data;
        std::unique_ptr<Data> d;
    };
} // namespace pcap
