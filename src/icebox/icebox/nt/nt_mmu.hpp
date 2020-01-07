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

struct MMPTE_HARDWARE
{
    uint64_t Valid                  : 1;
    uint64_t Dirty1                 : 1;
    uint64_t Owner                  : 1;
    uint64_t WriteThrough           : 1;
    uint64_t CacheDisable           : 1;
    uint64_t Accessed               : 1;
    uint64_t Dirty                  : 1;
    uint64_t LargePage              : 1;
    uint64_t Global                 : 1;
    uint64_t CopyOnWrite            : 1;
    uint64_t Unused                 : 1;
    uint64_t Write                  : 1;
    uint64_t PageFrameNumber        : 36;
    uint64_t ReservedForHardware    : 4;
    uint64_t ReservedForSoftware    : 4;
    uint64_t WsleAge                : 4;
    uint64_t WsleProtection         : 3;
    uint64_t NoExecute              : 1;
};

struct MMPTE_PROTOTYPE
{
    uint64_t Valid                  : 1;
    uint64_t DemandFillProto        : 1;
    uint64_t HiberVerifyConverted   : 1;
    uint64_t ReadOnly               : 1;
    uint64_t SwizzleBit             : 1;
    uint64_t Protection             : 5;
    uint64_t Prototype              : 1;
    uint64_t Combined               : 1;
    uint64_t Unused1                : 4;
    int64_t ProtoAddress            : 48; // sign extended
};

struct MMPTE_TRANSITION
{
    uint64_t Valid              : 1;
    uint64_t Write              : 1;
    uint64_t Spare              : 1;
    uint64_t IoTracker          : 1;
    uint64_t SwizzleBit         : 1;
    uint64_t Protection         : 5;
    uint64_t Prototype          : 1;
    uint64_t Transition         : 1;
    uint64_t PageFrameNumber    : 28;
    uint64_t Unused             : 24;
};

struct MMPTE_SUBSECTION
{
    uint64_t Valid              : 1;
    uint64_t Unused0            : 3;
    uint64_t SwizzleBit         : 1;
    uint64_t Protection         : 5;
    uint64_t Prototype          : 1;
    uint64_t ColdPage           : 1;
    uint64_t Unused1            : 3;
    uint64_t ExecutePrivilege   : 1;
    int64_t SubsectionAddress   : 48;
};

struct MMPTE_SOFTWARE
{
    uint64_t Valid                  : 1;
    uint64_t PageFileReserved       : 1;
    uint64_t PageFileAllocated      : 1;
    uint64_t ColdPage               : 1;
    uint64_t SwizzleBit             : 1;
    uint64_t Protection             : 5;
    uint64_t Prototype              : 1;
    uint64_t Transition             : 1;
    uint64_t PageFileLow            : 4;
    uint64_t UsedPageTableEntries   : 10;
    uint64_t ShadowStack            : 1;
    uint64_t Unused                 : 5;
    uint64_t PageFileHigh           : 32;
};

struct MMPTE
{
    union
    {
        uint64_t         value;
        MMPTE_HARDWARE   hard;
        MMPTE_PROTOTYPE  proto;
        MMPTE_TRANSITION trans;
        MMPTE_SUBSECTION subsect;
        MMPTE_SOFTWARE   soft;
    } u;
};
