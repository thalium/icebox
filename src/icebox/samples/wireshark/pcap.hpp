#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace pcap
{

    class Packet
    {
        std::vector<uint8_t> data;
        uint32_t ifId      = 0;
        uint64_t timestamp = 0;
        uint32_t sec       = 0;
        uint32_t usec      = 0;
        std::string comment;

      public:
         Packet(std::vector<uint8_t> d);
        ~Packet();
        uint32_t getDataLength() { return (uint32_t) data.size(); }
        std::vector<uint8_t> getData() { return data; }
        uint32_t getSeconds() { return sec; }
        uint32_t getMicroSeconds() { return usec; }
        uint64_t getTimestamp() { return timestamp; }
        std::string getComment() { return comment; }
        void setComment(std::string c);
        void setTime(uint64_t t) { timestamp = t; }
        void setTime(uint32_t s, uint32_t u)
        {
            sec  = s;
            usec = u;
        }
    };

    class FileWriterNG
    {
        FILE* file = nullptr;
        std::vector<Packet*> packets;

      public:
        FileWriterNG() {}
        void    addPacket   (Packet* p);
        void    addPackets  (std::vector<Packet*> packets);
        uint32_t packetCount() { return (uint32_t) packets.size(); }
        bool write(const char* filePath);
    };
} // namespace pcap
