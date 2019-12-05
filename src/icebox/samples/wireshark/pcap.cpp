#include "pcap.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

namespace
{
    struct BlockHeader
    {
        uint32_t type;
        uint32_t length;
    };

    constexpr uint32_t typeSHB = 0x0A0D0D0A;

    struct SHB
    {
        uint32_t byte_order;
        uint16_t version_major; // major version number
        uint16_t version_minor; // minor version number
        int64_t  section_length;
    };

    constexpr uint32_t typeEPB = 6;

    struct EPB
    {
        uint32_t if_id;
        uint32_t timestamp_high;
        uint32_t timestamp_low;
        uint32_t cap_length;
        uint32_t original_length;
    };

    constexpr uint32_t typeIDB = 1;

    struct IDB
    {
        uint16_t link_type;
        uint16_t reserved;
        uint32_t snap_len;
    };

    struct Option
    {
        uint16_t code;
        uint16_t length;
    };

    struct Packet
    {
        pcap::metadata_t     meta;
        std::vector<uint8_t> data;
    };
}

struct pcap::Writer::Data
{
    std::vector<Packet> packets;
};

pcap::Writer::Writer()
    : d(std::make_unique<Data>())
{
}

pcap::Writer::~Writer() = default;

void pcap::Writer::add_packet(const metadata_t& m, const void* data, size_t size)
{
    Packet p;
    p.meta = m;
    p.data.resize(size);
    memcpy(&p.data[0], data, p.data.size());
    d->packets.emplace_back(p);
}

namespace
{
    bool write_hdr(FILE* file, BlockHeader& hdr)
    {
        SHB shb;
        hdr.type           = typeSHB;
        hdr.length         = static_cast<uint32_t>(sizeof(BlockHeader) + sizeof(SHB) + sizeof hdr.length);
        shb.byte_order     = 0x1a2b3c4d;
        shb.version_major  = 1;
        shb.version_minor  = 0;
        shb.section_length = -1;

        auto bytes = fwrite(&hdr, 1, sizeof hdr, file);
        if(bytes != sizeof hdr)
            return false;

        bytes = fwrite(&shb, 1, sizeof shb, file);
        if(bytes != sizeof shb)
            return false;

        bytes = fwrite(&hdr.length, 1, sizeof hdr.length, file);
        if(bytes != sizeof hdr.length)
            return false;

        return true;
    }

    bool write_idb(FILE* file, BlockHeader& hdr)
    {
        IDB idb;
        hdr.type      = typeIDB;
        hdr.length    = static_cast<uint32_t>(sizeof(BlockHeader) + sizeof(IDB) + sizeof hdr.length);
        idb.link_type = 1; // ETHERNET
        idb.reserved  = 0;
        idb.snap_len  = 0;
        auto bytes    = fwrite(&hdr, 1, sizeof hdr, file);
        if(bytes != sizeof hdr)
            return false;

        bytes = fwrite(&idb, 1, sizeof idb, file);
        if(bytes != sizeof idb)
            return false;

        bytes = fwrite(&hdr.length, 1, sizeof hdr.length, file);
        if(bytes != sizeof hdr.length)
            return false;

        return true;
    }

    bool write_comment(FILE* file, Packet& p, uint32_t padding)
    {
        Option option;
        uint32_t paddingBuf = 0;

        option.code   = 1; // comment
        option.length = (uint16_t) p.meta.comment.size();

        auto bytes = fwrite(&option, 1, sizeof option, file);
        if(bytes != sizeof option)
            return false;

        bytes = fwrite(p.meta.comment.data(), 1, option.length, file);
        if(bytes != option.length)
            return false;

        if(option.length % 4)
        {
            padding = 4 - (option.length % 4);
            bytes   = fwrite(&paddingBuf, 1, padding, file);
            if(bytes != padding)
                return false;
        }
        option.code   = 0;
        option.length = 0;

        bytes = fwrite(&option, 1, sizeof option, file);
        if(bytes != sizeof option)
            return false;

        return true;
    }

    bool write_packet(FILE* file, Packet& p, BlockHeader& hdr)
    {
        auto size             = p.data.size();
        uint32_t optionLength = 0;
        uint32_t padding      = 0;
        if(size % 4)
            padding = 4 - (size % 4);

        if(!p.meta.comment.empty())
            optionLength = 8 + (uint32_t) p.meta.comment.size();

        EPB epb;
        hdr.type = typeEPB;
        if(sizeof(BlockHeader) + sizeof(EPB) + sizeof hdr.length + size + padding + optionLength > UINT32_MAX)
            return false;

        hdr.length = static_cast<uint32_t>(sizeof(BlockHeader) + sizeof(EPB) + sizeof hdr.length + size + padding + optionLength);
        auto bytes = fwrite(&hdr, 1, sizeof hdr, file);
        if(bytes != sizeof hdr)
            return false;

        auto timestamp = p.meta.timestamp;
        if(!timestamp)
            timestamp = p.meta.sec * 1000000 + p.meta.usec;

        epb.if_id          = 0;
        epb.timestamp_high = timestamp >> 32;
        epb.timestamp_low  = timestamp & 0xFFFFFFFF;
        if(size > UINT32_MAX)
            return false;

        epb.cap_length      = static_cast<uint32_t>(size);
        epb.original_length = static_cast<uint32_t>(size);

        bytes = fwrite(&epb, 1, sizeof epb, file);
        if(bytes != sizeof epb)
            return false;

        auto packet_data = p.data;
        bytes            = fwrite(&packet_data[0], 1, size + padding, file);
        if(bytes != size + padding)
            return false;

        // write comment option if any
        if(!p.meta.comment.empty())
        {
            const auto ok = write_comment(file, p, padding);
            if(!ok)
                return false;
        }

        bytes = fwrite(&hdr.length, 1, sizeof hdr.length, file);
        if(bytes != sizeof hdr.length)
            return false;

        return true;
    }

    bool close_and_exit(FILE* file, bool ret)
    {
        fclose(file);
        return ret;
    }
}

bool pcap::Writer::write(const std::string& filepath)
{
    auto file = fopen(filepath.data(), "wb");
    if(file == nullptr)
        return false;

    // Write the Section Header Block
    BlockHeader hdr;
    auto ok = write_hdr(file, hdr);
    if(!ok)
        return close_and_exit(file, false);

    // IDB
    ok = write_idb(file, hdr);
    if(!ok)
        return close_and_exit(file, false);

    // Write an Enhanced Packet Block for each packet
    for(auto& p : d->packets)
    {
        ok = write_packet(file, p, hdr);
        if(!ok)
            return close_and_exit(file, false);
    }

    return close_and_exit(file, true);
}
