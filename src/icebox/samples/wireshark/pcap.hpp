#pragma once

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace pcap
{
    struct packet_metadata_t
    {
        uint32_t    ifId;
        uint64_t    timestamp;
        uint32_t    sec;
        uint32_t    usec;
        std::string comment;
    };

    struct Packet
    {
        packet_metadata_t    meta;
        std::vector<uint8_t> data;
    };

    struct FileWriterNG
    {
         FileWriterNG();
        ~FileWriterNG();

        void    add_packet  (const pcap::Packet& p);
        bool    write       (const std::string& filepath);

        struct Data;
        std::unique_ptr<Data> d;
    };
} // namespace pcap
