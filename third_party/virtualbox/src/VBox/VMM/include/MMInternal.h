/* $Id: MMInternal.h $ */
/** @file
 * MM - Internal header file.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___MMInternal_h
#define ___MMInternal_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/sup.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/pdmcritsect.h>
#include <iprt/assert.h>
#include <iprt/avl.h>
#include <iprt/critsect.h>



/** @defgroup grp_mm_int   Internals
 * @ingroup grp_mm
 * @internal
 * @{
 */


/** @name MMR3Heap - VM Ring-3 Heap Internals
 * @{
 */

/** @def MMR3HEAP_SIZE_ALIGNMENT
 * The allocation size alignment of the MMR3Heap.
 */
#define MMR3HEAP_SIZE_ALIGNMENT     16

/** @def MMR3HEAP_WITH_STATISTICS
 * Enable MMR3Heap statistics.
 */
#if !defined(MMR3HEAP_WITH_STATISTICS) && defined(VBOX_WITH_STATISTICS)
# define MMR3HEAP_WITH_STATISTICS
#endif

/**
 * Heap statistics record.
 * There is one global and one per allocation tag.
 */
typedef struct MMHEAPSTAT
{
    /** Core avl node, key is the tag. */
    AVLULNODECORE           Core;
    /** Pointer to the heap the memory belongs to. */
    struct MMHEAP          *pHeap;
#ifdef MMR3HEAP_WITH_STATISTICS
    /** Number of bytes currently allocated. */
    size_t                  cbCurAllocated;
    /** Number of allocation. */
    uint64_t                cAllocations;
    /** Number of reallocations. */
    uint64_t                cReallocations;
    /** Number of frees. */
    uint64_t                cFrees;
    /** Failures. */
    uint64_t                cFailures;
    /** Number of bytes allocated (sum). */
    uint64_t                cbAllocated;
    /** Number of bytes freed. */
    uint64_t                cbFreed;
#endif
} MMHEAPSTAT;
#if defined(MMR3HEAP_WITH_STATISTICS) && defined(IN_RING3)
AssertCompileMemberAlignment(MMHEAPSTAT, cAllocations, 8);
AssertCompileSizeAlignment(MMHEAPSTAT, 8);
#endif
/** Pointer to heap statistics record. */
typedef MMHEAPSTAT *PMMHEAPSTAT;




/**
 * Additional heap block header for relating allocations to the VM.
 */
typedef struct MMHEAPHDR
{
    /** Pointer to the next record. */
    struct MMHEAPHDR       *pNext;
    /** Pointer to the previous record. */
    struct MMHEAPHDR       *pPrev;
    /** Pointer to the heap statistics record.
     * (Where the a PVM can be found.) */
    PMMHEAPSTAT             pStat;
    /** Size of the allocation (including this header). */
    size_t                  cbSize;
} MMHEAPHDR;
/** Pointer to MM heap header. */
typedef MMHEAPHDR *PMMHEAPHDR;


/** MM Heap structure. */
typedef struct MMHEAP
{
    /** Lock protecting the heap. */
    RTCRITSECT              Lock;
    /** Heap block list head. */
    PMMHEAPHDR              pHead;
    /** Heap block list tail. */
    PMMHEAPHDR              pTail;
    /** Heap per tag statistics tree. */
    PAVLULNODECORE          pStatTree;
    /** The VM handle. */
    PUVM                    pUVM;
    /** Heap global statistics. */
    MMHEAPSTAT              Stat;
} MMHEAP;
/** Pointer to MM Heap structure. */
typedef MMHEAP *PMMHEAP;

/** @} */


/** @name MMUkHeap - VM User-kernel Heap Internals
 * @{
 */

/** @def MMUKHEAP_SIZE_ALIGNMENT
 * The allocation size alignment of the MMR3UkHeap.
 */
#define MMUKHEAP_SIZE_ALIGNMENT   16

/** @def MMUKHEAP_WITH_STATISTICS
 * Enable MMUkHeap statistics.
 */
#if !defined(MMUKHEAP_WITH_STATISTICS) && defined(VBOX_WITH_STATISTICS)
# define MMUKHEAP_WITH_STATISTICS
#endif


/**
 * Heap statistics record.
 * There is one global and one per allocation tag.
 */
typedef struct MMUKHEAPSTAT
{
    /** Core avl node, key is the tag. */
    AVLULNODECORE           Core;
    /** Number of allocation. */
    uint64_t                cAllocations;
    /** Number of reallocations. */
    uint64_t                cReallocations;
    /** Number of frees. */
    uint64_t                cFrees;
    /** Failures. */
    uint64_t                cFailures;
    /** Number of bytes allocated (sum). */
    uint64_t                cbAllocated;
    /** Number of bytes freed. */
    uint64_t                cbFreed;
    /** Number of bytes currently allocated. */
    size_t                  cbCurAllocated;
} MMUKHEAPSTAT;
#ifdef IN_RING3
AssertCompileMemberAlignment(MMUKHEAPSTAT, cAllocations, 8);
#endif
/** Pointer to heap statistics record. */
typedef MMUKHEAPSTAT *PMMUKHEAPSTAT;

/**
 * Sub heap tracking record.
 */
typedef struct MMUKHEAPSUB
{
    /** Pointer to the next sub-heap. */
    struct MMUKHEAPSUB     *pNext;
    /** The base address of the sub-heap. */
    void                   *pv;
    /** The size of the sub-heap.  */
    size_t                  cb;
    /** The handle of the simple block pointer. */
    RTHEAPSIMPLE            hSimple;
    /** The ring-0 address corresponding to MMUKHEAPSUB::pv. */
    RTR0PTR                 pvR0;
} MMUKHEAPSUB;
/** Pointer to a sub-heap tracking record. */
typedef MMUKHEAPSUB *PMMUKHEAPSUB;


/** MM User-kernel Heap structure. */
typedef struct MMUKHEAP
{
    /** Lock protecting the heap. */
    RTCRITSECT              Lock;
    /** Head of the sub-heap LIFO. */
    PMMUKHEAPSUB            pSubHeapHead;
    /** Heap per tag statistics tree. */
    PAVLULNODECORE          pStatTree;
    /** The VM handle. */
    PUVM                    pUVM;
#if HC_ARCH_BITS == 32
    /** Aligning the statistics on an 8 byte boundary (for uint64_t and STAM). */
    void                   *pvAlignment;
#endif
    /** Heap global statistics. */
    MMUKHEAPSTAT            Stat;
} MMUKHEAP;
#ifdef IN_RING3
AssertCompileMemberAlignment(MMUKHEAP, Stat, 8);
#endif
/** Pointer to MM Heap structure. */
typedef MMUKHEAP *PMMUKHEAP;

/** @} */



/** @name Hypervisor Heap Internals
 * @{
 */

/** @def MMHYPER_HEAP_FREE_DELAY
 * If defined, it indicates the number of frees that should be delayed.
 */
#if defined(DOXYGEN_RUNNING)
# define MMHYPER_HEAP_FREE_DELAY            64
#endif

/** @def MMHYPER_HEAP_FREE_POISON
 * If defined, it indicates that freed memory should be poisoned
 * with the value it has.
 */
#if defined(VBOX_STRICT) || defined(DOXYGEN_RUNNING)
# define MMHYPER_HEAP_FREE_POISON           0xcb
#endif

/** @def MMHYPER_HEAP_STRICT
 * Enables a bunch of assertions in the heap code. */
#if defined(VBOX_STRICT) || defined(DOXYGEN_RUNNING)
# define MMHYPER_HEAP_STRICT 1
# if 0 || defined(DOXYGEN_RUNNING)
/** @def MMHYPER_HEAP_STRICT_FENCE
 * Enables tail fence. */
#  define MMHYPER_HEAP_STRICT_FENCE
/** @def MMHYPER_HEAP_STRICT_FENCE_SIZE
 * The fence size in bytes. */
#  define MMHYPER_HEAP_STRICT_FENCE_SIZE    256
/** @def MMHYPER_HEAP_STRICT_FENCE_U32
 * The fence filler. */
#  define MMHYPER_HEAP_STRICT_FENCE_U32     UINT32_C(0xdeadbeef)
# endif
#endif

/**
 * Hypervisor heap statistics record.
 * There is one global and one per allocation tag.
 */
typedef struct MMHYPERSTAT
{
    /** Core avl node, key is the tag.
     * @todo The type is wrong! Get your lazy a$$ over and create that offsetted uint32_t version we need here!  */
    AVLOGCPHYSNODECORE      Core;
    /** Aligning the 64-bit fields on a 64-bit line. */
    uint32_t                u32Padding0;
    /** Indicator for whether these statistics are registered with STAM or not. */
    bool                    fRegistered;
    /** Number of allocation. */
    uint64_t                cAllocations;
    /** Number of frees. */
    uint64_t                cFrees;
    /** Failures. */
    uint64_t                cFailures;
    /** Number of bytes allocated (sum). */
    uint64_t                cbAllocated;
    /** Number of bytes freed (sum). */
    uint64_t                cbFreed;
    /** Number of bytes currently allocated. */
    uint32_t                cbCurAllocated;
    /** Max number of bytes allocated. */
    uint32_t                cbMaxAllocated;
} MMHYPERSTAT;
AssertCompileMemberAlignment(MMHYPERSTAT, cAllocations, 8);
/** Pointer to hypervisor heap statistics record. */
typedef MMHYPERSTAT *PMMHYPERSTAT;

/**
 * Hypervisor heap chunk.
 */
typedef struct MMHYPERCHUNK
{
    /** Previous block in the list of all blocks.
     * This is relative to the start of the heap. */
    uint32_t                offNext;
    /** Offset to the previous block relative to this one. */
    int32_t                 offPrev;
    /** The statistics record this allocation belongs to (self relative). */
    int32_t                 offStat;
    /** Offset to the heap block (self relative). */
    int32_t                 offHeap;
} MMHYPERCHUNK;
/** Pointer to a hypervisor heap chunk. */
typedef MMHYPERCHUNK *PMMHYPERCHUNK;


/**
 * Hypervisor heap chunk.
 */
typedef struct MMHYPERCHUNKFREE
{
    /** Main list. */
    MMHYPERCHUNK            core;
    /** Offset of the next chunk in the list of free nodes. */
    uint32_t                offNext;
    /** Offset of the previous chunk in the list of free nodes. */
    int32_t                 offPrev;
    /** Size of the block. */
    uint32_t                cb;
} MMHYPERCHUNKFREE;
/** Pointer to a free hypervisor heap chunk. */
typedef MMHYPERCHUNKFREE *PMMHYPERCHUNKFREE;


/**
 * The hypervisor heap.
 */
typedef struct MMHYPERHEAP
{
    /** The typical magic (MMHYPERHEAP_MAGIC). */
    uint32_t                u32Magic;
    /** The heap size. (This structure is not included!) */
    uint32_t                cbHeap;
    /** Lock protecting the heap. */
    PDMCRITSECT             Lock;
    /** The HC ring-3 address of the heap. */
    R3PTRTYPE(uint8_t *)    pbHeapR3;
    /** The HC ring-3 address of the shared VM structure. */
    PVMR3                   pVMR3;
    /** The HC ring-0 address of the heap. */
    R0PTRTYPE(uint8_t *)    pbHeapR0;
    /** The HC ring-0 address of the shared VM structure. */
    PVMR0                   pVMR0;
    /** The RC address of the heap. */
    RCPTRTYPE(uint8_t *)    pbHeapRC;
    /** The RC address of the shared VM structure. */
    PVMRC                   pVMRC;
    /** The amount of free memory in the heap. */
    uint32_t                cbFree;
    /** Offset of the first free chunk in the heap.
     * The offset is relative to the start of the heap. */
    uint32_t                offFreeHead;
    /** Offset of the last free chunk in the heap.
     * The offset is relative to the start of the heap. */
    uint32_t                offFreeTail;
    /** Offset of the first page aligned block in the heap.
     * The offset is equal to cbHeap initially. */
    uint32_t                offPageAligned;
    /** Tree of hypervisor heap statistics. */
    AVLOGCPHYSTREE          HyperHeapStatTree;
#ifdef MMHYPER_HEAP_FREE_DELAY
    /** Where to insert the next free. */
    uint32_t                iDelayedFree;
    /** Array of delayed frees. Circular. Offsets relative to this structure. */
    struct
    {
        /** The free caller address. */
        RTUINTPTR           uCaller;
        /** The offset of the freed chunk. */
        uint32_t            offChunk;
    } aDelayedFrees[MMHYPER_HEAP_FREE_DELAY];
#else
    /** Padding the structure to a 64-bit aligned size. */
    uint32_t                u32Padding0;
#endif
    /** The heap physical pages. */
    R3PTRTYPE(PSUPPAGE)     paPages;
#if HC_ARCH_BITS == 32
    /** Padding the structure to a 64-bit aligned size. */
    uint32_t                u32Padding1;
#endif
} MMHYPERHEAP;
/** Pointer to the hypervisor heap. */
typedef MMHYPERHEAP *PMMHYPERHEAP;

/** Magic value for MMHYPERHEAP. (C. S. Lewis) */
#define MMHYPERHEAP_MAGIC               UINT32_C(0x18981129)


/**
 * Hypervisor heap minimum alignment (16 bytes).
 */
#define MMHYPER_HEAP_ALIGN_MIN          16

/**
 * The aligned size of the MMHYPERHEAP structure.
 */
#define MMYPERHEAP_HDR_SIZE             RT_ALIGN_Z(sizeof(MMHYPERHEAP), MMHYPER_HEAP_ALIGN_MIN * 4)

/** @name Hypervisor heap chunk flags.
 * The flags are put in the first bits of the MMHYPERCHUNK::offPrev member.
 * These bits aren't used anyway because of the chunk minimal alignment (16 bytes).
 * @{ */
/** The chunk is free. (The code ASSUMES this is 0!) */
#define MMHYPERCHUNK_FLAGS_FREE         0x0
/** The chunk is in use. */
#define MMHYPERCHUNK_FLAGS_USED         0x1
/** The type mask. */
#define MMHYPERCHUNK_FLAGS_TYPE_MASK    0x1
/** The flag mask */
#define MMHYPERCHUNK_FLAGS_MASK         0x1

/** Checks if the chunk is free. */
#define MMHYPERCHUNK_ISFREE(pChunk)                 ( (((pChunk)->offPrev) & MMHYPERCHUNK_FLAGS_TYPE_MASK) == MMHYPERCHUNK_FLAGS_FREE )
/** Checks if the chunk is used. */
#define MMHYPERCHUNK_ISUSED(pChunk)                 ( (((pChunk)->offPrev) & MMHYPERCHUNK_FLAGS_TYPE_MASK) == MMHYPERCHUNK_FLAGS_USED )
/** Toggles FREE/USED flag of a chunk. */
#define MMHYPERCHUNK_SET_TYPE(pChunk, type)         do { (pChunk)->offPrev = ((pChunk)->offPrev & ~MMHYPERCHUNK_FLAGS_TYPE_MASK) | ((type) & MMHYPERCHUNK_FLAGS_TYPE_MASK); } while (0)

/** Gets the prev offset without the flags. */
#define MMHYPERCHUNK_GET_OFFPREV(pChunk)            ((int32_t)((pChunk)->offPrev & ~MMHYPERCHUNK_FLAGS_MASK))
/** Sets the prev offset without changing the flags. */
#define MMHYPERCHUNK_SET_OFFPREV(pChunk, off)       do { (pChunk)->offPrev = (off) | ((pChunk)->offPrev & MMHYPERCHUNK_FLAGS_MASK); } while (0)
#if 0
/** Clears one or more flags. */
#define MMHYPERCHUNK_FLAGS_OP_CLEAR(pChunk, fFlags) do { ((pChunk)->offPrev) &= ~((fFlags) & MMHYPERCHUNK_FLAGS_MASK); } while (0)
/** Sets one or more flags. */
#define MMHYPERCHUNK_FLAGS_OP_SET(pChunk, fFlags)   do { ((pChunk)->offPrev) |=  ((fFlags) & MMHYPERCHUNK_FLAGS_MASK); } while (0)
/** Checks if one is set. */
#define MMHYPERCHUNK_FLAGS_OP_ISSET(pChunk, fFlag)  (!!(((pChunk)->offPrev) & ((fFlag) & MMHYPERCHUNK_FLAGS_MASK)))
#endif
/** @} */

/** @} */


/** @name Page Pool Internals
 * @{
 */

/**
 * Page sub pool
 *
 * About the allocation of this structure. To keep the number of heap blocks,
 * the number of heap calls, and fragmentation low we allocate all the data
 * related to a MMPAGESUBPOOL node in one chunk. That means that after the
 * bitmap (which is of variable size) comes the SUPPAGE records and then
 * follows the lookup tree nodes. (The heap in question is the hyper heap.)
 */
typedef struct MMPAGESUBPOOL
{
    /** Pointer to next sub pool. */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    R3PTRTYPE(struct MMPAGESUBPOOL *)   pNext;
#else
    R3R0PTRTYPE(struct MMPAGESUBPOOL *) pNext;
#endif
    /** Pointer to next sub pool in the free chain.
     * This is NULL if we're not in the free chain or at the end of it. */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    R3PTRTYPE(struct MMPAGESUBPOOL *)   pNextFree;
#else
    R3R0PTRTYPE(struct MMPAGESUBPOOL *) pNextFree;
#endif
    /** Pointer to array of lock ranges.
     * This is allocated together with the MMPAGESUBPOOL and thus needs no freeing.
     * It follows immediately after the bitmap.
     * The reserved field is a pointer to this structure.
     */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    R3PTRTYPE(PSUPPAGE)                 paPhysPages;
#else
    R3R0PTRTYPE(PSUPPAGE)               paPhysPages;
#endif
    /** Pointer to the first page. */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    R3PTRTYPE(void *)                   pvPages;
#else
    R3R0PTRTYPE(void *)                 pvPages;
#endif
    /** Size of the subpool. */
    uint32_t                            cPages;
    /** Number of free pages. */
    uint32_t                            cPagesFree;
    /** The allocation bitmap.
     * This may extend beyond the end of the defined array size.
     */
    uint32_t                            auBitmap[1];
    /* ... SUPPAGE                      aRanges[1]; */
} MMPAGESUBPOOL;
/** Pointer to page sub pool. */
typedef MMPAGESUBPOOL *PMMPAGESUBPOOL;

/**
 * Page pool.
 */
typedef struct MMPAGEPOOL
{
    /** List of subpools. */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    R3PTRTYPE(PMMPAGESUBPOOL)           pHead;
#else
    R3R0PTRTYPE(PMMPAGESUBPOOL)         pHead;
#endif
    /** Head of subpools with free pages. */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    R3PTRTYPE(PMMPAGESUBPOOL)           pHeadFree;
#else
    R3R0PTRTYPE(PMMPAGESUBPOOL)         pHeadFree;
#endif
    /** AVLPV tree for looking up HC virtual addresses.
     * The tree contains MMLOOKUPVIRTPP records.
     */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    R3PTRTYPE(PAVLPVNODECORE)           pLookupVirt;
#else
    R3R0PTRTYPE(PAVLPVNODECORE)         pLookupVirt;
#endif
    /** Tree for looking up HC physical addresses.
     * The tree contains MMLOOKUPPHYSHC records.
     */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    R3PTRTYPE(AVLHCPHYSTREE)            pLookupPhys;
#else
    R3R0PTRTYPE(AVLHCPHYSTREE)          pLookupPhys;
#endif
    /** Pointer to the VM this pool belongs. */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    PVMR3                               pVM;
#else
    R3R0PTRTYPE(PVM)                    pVM;
#endif
    /** Flag indicating the allocation method.
     * Set: SUPR3LowAlloc().
     * Clear: SUPR3PageAllocEx(). */
    bool                                fLow;
    /** Number of subpools. */
    uint32_t                            cSubPools;
    /** Number of pages in pool. */
    uint32_t                            cPages;
#ifdef VBOX_WITH_STATISTICS
    /** Number of free pages in pool. */
    uint32_t                            cFreePages;
# if HC_ARCH_BITS == 32
    /** Aligning the statistics on an 8 byte boundary. */
    uint32_t                            u32Alignment;
# endif
    /** Number of alloc calls. */
    STAMCOUNTER                         cAllocCalls;
    /** Number of free calls. */
    STAMCOUNTER                         cFreeCalls;
    /** Number of to phys conversions. */
    STAMCOUNTER                         cToPhysCalls;
    /** Number of to virtual conversions. */
    STAMCOUNTER                         cToVirtCalls;
    /** Number of real errors. */
    STAMCOUNTER                         cErrors;
#endif
} MMPAGEPOOL;
#ifndef IN_RC
AssertCompileMemberAlignment(MMPAGEPOOL, cSubPools, 4);
# ifdef VBOX_WITH_STATISTICS
AssertCompileMemberAlignment(MMPAGEPOOL, cAllocCalls, 8);
# endif
#endif
/** Pointer to page pool. */
typedef MMPAGEPOOL *PMMPAGEPOOL;

/**
 * Lookup record for HC virtual memory in the page pool.
 */
typedef struct MMPPLOOKUPHCPTR
{
    /** The key is virtual address. */
    AVLPVNODECORE           Core;
    /** Pointer to subpool if lookup record for a pool. */
    struct MMPAGESUBPOOL   *pSubPool;
} MMPPLOOKUPHCPTR;
/** Pointer to virtual memory lookup record. */
typedef MMPPLOOKUPHCPTR *PMMPPLOOKUPHCPTR;

/**
 * Lookup record for HC physical memory.
 */
typedef struct MMPPLOOKUPHCPHYS
{
    /** The key is physical address. */
    AVLHCPHYSNODECORE       Core;
    /** Pointer to SUPPAGE record for this physical address. */
    PSUPPAGE                pPhysPage;
} MMPPLOOKUPHCPHYS;
/** Pointer to physical memory lookup record. */
typedef MMPPLOOKUPHCPHYS *PMMPPLOOKUPHCPHYS;

/** @} */


/**
 * Hypervisor memory mapping type.
 */
typedef enum MMLOOKUPHYPERTYPE
{
    /** Invalid record. This is used for record which are incomplete. */
    MMLOOKUPHYPERTYPE_INVALID = 0,
    /** Mapping of locked memory. */
    MMLOOKUPHYPERTYPE_LOCKED,
    /** Mapping of contiguous HC physical memory. */
    MMLOOKUPHYPERTYPE_HCPHYS,
    /** Mapping of contiguous GC physical memory. */
    MMLOOKUPHYPERTYPE_GCPHYS,
    /** Mapping of MMIO2 memory. */
    MMLOOKUPHYPERTYPE_MMIO2,
    /** Dynamic mapping area (MMR3HyperReserve).
     * A conversion will require to check what's in the page table for the pages. */
    MMLOOKUPHYPERTYPE_DYNAMIC
} MMLOOKUPHYPERTYPE;

/**
 * Lookup record for the hypervisor memory area.
 */
typedef struct MMLOOKUPHYPER
{
    /** Byte offset from the start of this record to the next.
     * If the value is NIL_OFFSET the chain is terminated. */
    int32_t                 offNext;
    /** Offset into the hypervisor memory area. */
    uint32_t                off;
    /** Size of this part. */
    uint32_t                cb;
    /** Locking type. */
    MMLOOKUPHYPERTYPE       enmType;
    /** Type specific data */
    union
    {
        /** Locked memory. */
        struct
        {
            /** Host context ring-3 pointer. */
            R3PTRTYPE(void *)       pvR3;
            /** Host context ring-0 pointer. Optional. */
            RTR0PTR                 pvR0;
            /** Pointer to an array containing the physical address of each page. */
            R3PTRTYPE(PRTHCPHYS)    paHCPhysPages;
        } Locked;

        /** Contiguous physical memory. */
        struct
        {
            /** Host context ring-3 pointer. */
            R3PTRTYPE(void *)       pvR3;
            /** Host context ring-0 pointer. Optional. */
            RTR0PTR                 pvR0;
            /** HC physical address corresponding to pvR3/pvR0. */
            RTHCPHYS                HCPhys;
        } HCPhys;

        /** Contiguous guest physical memory. */
        struct
        {
            /** The memory address (Guest Context). */
            RTGCPHYS                GCPhys;
        } GCPhys;

        /** MMIO2 memory. */
        struct
        {
            /** The device instance owning the MMIO2 region. */
            PPDMDEVINSR3            pDevIns;
            /** The sub-device number. */
            uint32_t                iSubDev;
            /** The region number. */
            uint32_t                iRegion;
#if HC_ARCH_BITS == 32
            /** Alignment padding. */
            uint32_t                uPadding;
#endif
            /** The offset into the MMIO2 region. */
            RTGCPHYS                off;
        } MMIO2;
    } u;
    /** Description. */
    R3PTRTYPE(const char *) pszDesc;
} MMLOOKUPHYPER;
/** Pointer to a hypervisor memory lookup record. */
typedef MMLOOKUPHYPER *PMMLOOKUPHYPER;


/**
 * Converts a MM pointer into a VM pointer.
 * @returns Pointer to the VM structure the MM is part of.
 * @param   pMM   Pointer to MM instance data.
 */
#define MM2VM(pMM)  ( (PVM)((uint8_t *)pMM - pMM->offVM) )


/**
 * MM Data (part of VM)
 */
typedef struct MM
{
    /** Offset to the VM structure.
     * See MM2VM(). */
    RTINT                       offVM;

    /** Set if MMR3InitPaging has been called. */
    bool                        fDoneMMR3InitPaging;
    /** Set if PGM has been initialized and we can safely call PGMR3Map(). */
    bool                        fPGMInitialized;
#if GC_ARCH_BITS == 64 || HC_ARCH_BITS == 64
    uint32_t                    u32Padding1; /**< alignment padding. */
#endif

    /** Lookup list for the Hypervisor Memory Area.
     * The offset is relative to the start of the heap.
     * Use pHyperHeapR3, pHyperHeapR0 or pHypeRHeapRC to calculate the address.
     */
    RTUINT                      offLookupHyper;

    /** The offset of the next static mapping in the Hypervisor Memory Area. */
    RTUINT                      offHyperNextStatic;
    /** The size of the HMA.
     * Starts at 12MB and will be fixed late in the init process. */
    RTUINT                      cbHyperArea;

    /** Guest address of the Hypervisor Memory Area.
     * @remarks It's still a bit open whether this should be change to RTRCPTR or
     *          remain a RTGCPTR. */
    RTGCPTR                     pvHyperAreaGC;

    /** The hypervisor heap (GC Ptr). */
    RCPTRTYPE(PMMHYPERHEAP)     pHyperHeapRC;
#if HC_ARCH_BITS == 64 && GC_ARCH_BITS == 64
    uint32_t                    u32Padding2;
#endif

    /** The hypervisor heap (R0 Ptr). */
    R0PTRTYPE(PMMHYPERHEAP)     pHyperHeapR0;
#ifndef VBOX_WITH_2X_4GB_ADDR_SPACE
    /** Page pool - R0 Ptr. */
    R0PTRTYPE(PMMPAGEPOOL)      pPagePoolR0;
    /** Page pool pages in low memory R0 Ptr. */
    R0PTRTYPE(PMMPAGEPOOL)      pPagePoolLowR0;
#endif /* !VBOX_WITH_2X_4GB_ADDR_SPACE */

    /** The hypervisor heap (R3 Ptr). */
    R3PTRTYPE(PMMHYPERHEAP)     pHyperHeapR3;
    /** Page pool - R3 Ptr. */
    R3PTRTYPE(PMMPAGEPOOL)      pPagePoolR3;
    /** Page pool pages in low memory R3 Ptr. */
    R3PTRTYPE(PMMPAGEPOOL)      pPagePoolLowR3;

    /** Pointer to the dummy page.
     * The dummy page is a paranoia thingy used for instance for pure MMIO RAM ranges
     * to make sure any bugs will not harm whatever the system stores in the first
     * physical page. */
    R3PTRTYPE(void *)           pvDummyPage;
    /** Physical address of the dummy page. */
    RTHCPHYS                    HCPhysDummyPage;

    /** Size of the base RAM in bytes. (The CFGM RamSize value.) */
    uint64_t                    cbRamBase;
    /** Number of bytes of RAM above 4GB, starting at address 4GB.  */
    uint64_t                    cbRamAbove4GB;
    /** Size of the below 4GB RAM hole. */
    uint32_t                    cbRamHole;
    /** Number of bytes of RAM below 4GB, starting at address 0.  */
    uint32_t                    cbRamBelow4GB;
    /** The number of base RAM pages that PGM has reserved (GMM).
     * @remarks Shadow ROMs will be counted twice (RAM+ROM), so it won't be 1:1 with
     *          what the guest sees. */
    uint64_t                    cBasePages;
    /** The number of handy pages that PGM has reserved (GMM).
     * These are kept out of cBasePages and thus out of the saved state. */
    uint32_t                    cHandyPages;
    /** The number of shadow pages PGM has reserved (GMM). */
    uint32_t                    cShadowPages;
    /** The number of fixed pages we've reserved (GMM). */
    uint32_t                    cFixedPages;
    /** Padding. */
    uint32_t                    u32Padding0;
} MM;
/** Pointer to MM Data (part of VM). */
typedef MM *PMM;


/**
 * MM data kept in the UVM.
 */
typedef struct MMUSERPERVM
{
    /** Pointer to the MM R3 Heap. */
    R3PTRTYPE(PMMHEAP)          pHeap;
    /** Pointer to the MM Uk Heap. */
    R3PTRTYPE(PMMUKHEAP)        pUkHeap;
} MMUSERPERVM;
/** Pointer to the MM data kept in the UVM. */
typedef MMUSERPERVM *PMMUSERPERVM;


RT_C_DECLS_BEGIN


int  mmR3UpdateReservation(PVM pVM);

int  mmR3PagePoolInit(PVM pVM);
void mmR3PagePoolTerm(PVM pVM);

int  mmR3HeapCreateU(PUVM pUVM, PMMHEAP *ppHeap);
void mmR3HeapDestroy(PMMHEAP pHeap);

void mmR3UkHeapDestroy(PMMUKHEAP pHeap);
int  mmR3UkHeapCreateU(PUVM pUVM, PMMUKHEAP *ppHeap);


int  mmR3HyperInit(PVM pVM);
int  mmR3HyperTerm(PVM pVM);
int  mmR3HyperInitPaging(PVM pVM);

const char *mmGetTagName(MMTAG enmTag);

/**
 * Converts a pool address to a physical address.
 * The specified allocation type must match with the address.
 *
 * @returns Physical address.
 * @returns NIL_RTHCPHYS if not found or eType is not matching.
 * @param   pPool   Pointer to the page pool.
 * @param   pv      The address to convert.
 * @thread  The Emulation Thread.
 */
RTHCPHYS mmPagePoolPtr2Phys(PMMPAGEPOOL pPool, void *pv);

/**
 * Converts a pool physical address to a linear address.
 * The specified allocation type must match with the address.
 *
 * @returns Physical address.
 * @returns NULL if not found or eType is not matching.
 * @param   pPool       Pointer to the page pool.
 * @param   HCPhys      The address to convert.
 * @thread  The Emulation Thread.
 */
void *mmPagePoolPhys2Ptr(PMMPAGEPOOL pPool, RTHCPHYS HCPhys);

RT_C_DECLS_END

/** @} */

#endif

