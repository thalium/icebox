#pragma once

#include <stdint.h>

struct virt_t
{
    union
    {
        uint64_t value;
        struct
        {
            uint64_t offset : 12;
            uint64_t pt     : 9;
            uint64_t pd     : 9;
            uint64_t pdp    : 9;
            uint64_t pml4   : 9;
            uint64_t unused : 16;
        } f;
    } u;
};
static_assert(sizeof(virt_t) * 8 == 64, "invalid virt_t size");

struct entry_t
{
    union
    {
        uint64_t value;
        struct
        {
            uint64_t can_read           : 1;
            uint64_t can_write          : 1;
            uint64_t user_mode          : 1;
            uint64_t write_through      : 1;
            uint64_t page_cache_disable : 1;
            uint64_t accessed           : 1;
            uint64_t dirty              : 1;
            uint64_t large_page         : 1;
            uint64_t _                  : 4;
            uint64_t page_frame_number  : 40;
            uint64_t __                 : 11;
            uint64_t execute_disable    : 1;
        } f;
    } u;
};
static_assert(sizeof(entry_t) * 8 == 64, "invalid entry_t size");
