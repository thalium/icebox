/* $Id: IEMInternal.h $ */
/** @file
 * IEM - Internal header file.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___IEMInternal_h
#define ___IEMInternal_h

#include <VBox/vmm/cpum.h>
#include <VBox/vmm/iem.h>
#include <VBox/vmm/stam.h>
#include <VBox/param.h>

#include <setjmp.h>


RT_C_DECLS_BEGIN


/** @defgroup grp_iem_int       Internals
 * @ingroup grp_iem
 * @internal
 * @{
 */

/** For expanding symbol in slickedit and other products tagging and
 *  crossreferencing IEM symbols. */
#ifndef IEM_STATIC
# define IEM_STATIC static
#endif

/** @def IEM_WITH_3DNOW
 * Includes the 3DNow decoding.  */
#define IEM_WITH_3DNOW

/** @def IEM_WITH_THREE_0F_38
 * Includes the three byte opcode map for instrs starting with 0x0f 0x38. */
#define IEM_WITH_THREE_0F_38

/** @def IEM_WITH_THREE_0F_3A
 * Includes the three byte opcode map for instrs starting with 0x0f 0x38. */
#define IEM_WITH_THREE_0F_3A

/** @def IEM_WITH_VEX
 * Includes the VEX decoding. */
#define IEM_WITH_VEX


/** @def IEM_VERIFICATION_MODE_FULL
 * Shorthand for:
 *    defined(IEM_VERIFICATION_MODE) && !defined(IEM_VERIFICATION_MODE_MINIMAL)
 */
#if (defined(IEM_VERIFICATION_MODE) && !defined(IEM_VERIFICATION_MODE_MINIMAL) && !defined(IEM_VERIFICATION_MODE_FULL)) \
  || defined(DOXYGEN_RUNNING)
# define IEM_VERIFICATION_MODE_FULL
#endif


/** @def IEM_CFG_TARGET_CPU
 * The minimum target CPU for the IEM emulation (IEMTARGETCPU_XXX value).
 *
 * By default we allow this to be configured by the user via the
 * CPUM/GuestCpuName config string, but this comes at a slight cost during
 * decoding.  So, for applications of this code where there is no need to
 * be dynamic wrt target CPU, just modify this define.
 */
#if !defined(IEM_CFG_TARGET_CPU) || defined(DOXYGEN_RUNNING)
# define IEM_CFG_TARGET_CPU     IEMTARGETCPU_DYNAMIC
#endif


//#define IEM_WITH_CODE_TLB// - work in progress


#if !defined(IN_TSTVMSTRUCT) && !defined(DOXYGEN_RUNNING)
/** Instruction statistics.   */
typedef struct IEMINSTRSTATS
{
# define IEM_DO_INSTR_STAT(a_Name, a_szDesc) uint32_t a_Name;
# include "IEMInstructionStatisticsTmpl.h"
# undef IEM_DO_INSTR_STAT
} IEMINSTRSTATS;
#else
struct IEMINSTRSTATS;
typedef struct IEMINSTRSTATS IEMINSTRSTATS;
#endif
/** Pointer to IEM instruction statistics. */
typedef IEMINSTRSTATS *PIEMINSTRSTATS;

/** Finish and move to types.h */
typedef union
{
    uint32_t u32;
} RTFLOAT32U;
typedef RTFLOAT32U *PRTFLOAT32U;
typedef RTFLOAT32U const *PCRTFLOAT32U;


/**
 * Extended operand mode that includes a representation of 8-bit.
 *
 * This is used for packing down modes when invoking some C instruction
 * implementations.
 */
typedef enum IEMMODEX
{
    IEMMODEX_16BIT = IEMMODE_16BIT,
    IEMMODEX_32BIT = IEMMODE_32BIT,
    IEMMODEX_64BIT = IEMMODE_64BIT,
    IEMMODEX_8BIT
} IEMMODEX;
AssertCompileSize(IEMMODEX, 4);


/**
 * Branch types.
 */
typedef enum IEMBRANCH
{
    IEMBRANCH_JUMP = 1,
    IEMBRANCH_CALL,
    IEMBRANCH_TRAP,
    IEMBRANCH_SOFTWARE_INT,
    IEMBRANCH_HARDWARE_INT
} IEMBRANCH;
AssertCompileSize(IEMBRANCH, 4);


/**
 * INT instruction types.
 */
typedef enum IEMINT
{
    /** INT n instruction (opcode 0xcd imm). */
    IEMINT_INTN  = 0,
    /** Single byte INT3 instruction (opcode 0xcc). */
    IEMINT_INT3  = IEM_XCPT_FLAGS_BP_INSTR,
    /** Single byte INTO instruction (opcode 0xce). */
    IEMINT_INTO  = IEM_XCPT_FLAGS_OF_INSTR,
    /** Single byte INT1 (ICEBP) instruction (opcode 0xf1). */
    IEMINT_INT1 = IEM_XCPT_FLAGS_ICEBP_INSTR
} IEMINT;
AssertCompileSize(IEMINT, 4);


/**
 * A FPU result.
 */
typedef struct IEMFPURESULT
{
    /** The output value. */
    RTFLOAT80U      r80Result;
    /** The output status. */
    uint16_t        FSW;
} IEMFPURESULT;
AssertCompileMemberOffset(IEMFPURESULT, FSW, 10);
/** Pointer to a FPU result. */
typedef IEMFPURESULT *PIEMFPURESULT;
/** Pointer to a const FPU result. */
typedef IEMFPURESULT const *PCIEMFPURESULT;


/**
 * A FPU result consisting of two output values and FSW.
 */
typedef struct IEMFPURESULTTWO
{
    /** The first output value. */
    RTFLOAT80U      r80Result1;
    /** The output status. */
    uint16_t        FSW;
    /** The second output value. */
    RTFLOAT80U      r80Result2;
} IEMFPURESULTTWO;
AssertCompileMemberOffset(IEMFPURESULTTWO, FSW, 10);
AssertCompileMemberOffset(IEMFPURESULTTWO, r80Result2, 12);
/** Pointer to a FPU result consisting of two output values and FSW. */
typedef IEMFPURESULTTWO *PIEMFPURESULTTWO;
/** Pointer to a const FPU result consisting of two output values and FSW. */
typedef IEMFPURESULTTWO const *PCIEMFPURESULTTWO;



#ifdef IEM_VERIFICATION_MODE_FULL

/**
 * Verification event type.
 */
typedef enum IEMVERIFYEVENT
{
    IEMVERIFYEVENT_INVALID = 0,
    IEMVERIFYEVENT_IOPORT_READ,
    IEMVERIFYEVENT_IOPORT_WRITE,
    IEMVERIFYEVENT_IOPORT_STR_READ,
    IEMVERIFYEVENT_IOPORT_STR_WRITE,
    IEMVERIFYEVENT_RAM_WRITE,
    IEMVERIFYEVENT_RAM_READ
} IEMVERIFYEVENT;

/** Checks if the event type is a RAM read or write. */
# define IEMVERIFYEVENT_IS_RAM(a_enmType)    ((a_enmType) == IEMVERIFYEVENT_RAM_WRITE || (a_enmType) == IEMVERIFYEVENT_RAM_READ)

/**
 * Verification event record.
 */
typedef struct IEMVERIFYEVTREC
{
    /** Pointer to the next record in the list. */
    struct IEMVERIFYEVTREC *pNext;
    /** The event type. */
    IEMVERIFYEVENT          enmEvent;
    /** The event data. */
    union
    {
        /** IEMVERIFYEVENT_IOPORT_READ */
        struct
        {
            RTIOPORT    Port;
            uint8_t     cbValue;
        } IOPortRead;

        /** IEMVERIFYEVENT_IOPORT_WRITE */
        struct
        {
            RTIOPORT    Port;
            uint8_t     cbValue;
            uint32_t    u32Value;
        } IOPortWrite;

        /** IEMVERIFYEVENT_IOPORT_STR_READ */
        struct
        {
            RTIOPORT    Port;
            uint8_t     cbValue;
            RTGCUINTREG cTransfers;
        } IOPortStrRead;

        /** IEMVERIFYEVENT_IOPORT_STR_WRITE */
        struct
        {
            RTIOPORT    Port;
            uint8_t     cbValue;
            RTGCUINTREG cTransfers;
        } IOPortStrWrite;

        /** IEMVERIFYEVENT_RAM_READ */
        struct
        {
            RTGCPHYS    GCPhys;
            uint32_t    cb;
        } RamRead;

        /** IEMVERIFYEVENT_RAM_WRITE */
        struct
        {
            RTGCPHYS    GCPhys;
            uint32_t    cb;
            uint8_t     ab[512];
        } RamWrite;
    } u;
} IEMVERIFYEVTREC;
/** Pointer to an IEM event verification records. */
typedef IEMVERIFYEVTREC *PIEMVERIFYEVTREC;

#endif /* IEM_VERIFICATION_MODE_FULL */


/**
 * IEM TLB entry.
 *
 * Lookup assembly:
 * @code{.asm}
        ; Calculate tag.
        mov     rax, [VA]
        shl     rax, 16
        shr     rax, 16 + X86_PAGE_SHIFT
        or      rax, [uTlbRevision]

        ; Do indexing.
        movzx   ecx, al
        lea     rcx, [pTlbEntries + rcx]

        ; Check tag.
        cmp     [rcx + IEMTLBENTRY.uTag], rax
        jne     .TlbMiss

        ; Check access.
        movsx   rax, ACCESS_FLAGS | MAPPING_R3_NOT_VALID | 0xffffff00
        and     rax, [rcx + IEMTLBENTRY.fFlagsAndPhysRev]
        cmp     rax, [uTlbPhysRev]
        jne     .TlbMiss

        ; Calc address and we're done.
        mov     eax, X86_PAGE_OFFSET_MASK
        and     eax, [VA]
        or      rax, [rcx + IEMTLBENTRY.pMappingR3]
    %ifdef VBOX_WITH_STATISTICS
        inc     qword [cTlbHits]
    %endif
        jmp     .Done

    .TlbMiss:
        mov     r8d, ACCESS_FLAGS
        mov     rdx, [VA]
        mov     rcx, [pVCpu]
        call    iemTlbTypeMiss
    .Done:

   @endcode
 *
 */
typedef struct IEMTLBENTRY
{
    /** The TLB entry tag.
     * Bits 35 thru 0 are made up of the virtual address shifted right 12 bits.
     * Bits 63 thru 36 are made up of the TLB revision (zero means invalid).
     *
     * The TLB lookup code uses the current TLB revision, which won't ever be zero,
     * enabling an extremely cheap TLB invalidation most of the time.  When the TLB
     * revision wraps around though, the tags needs to be zeroed.
     *
     * @note    Try use SHRD instruction?  After seeing
     *          https://gmplib.org/~tege/x86-timing.pdf, maybe not.
     */
    uint64_t                uTag;
    /** Access flags and physical TLB revision.
     *
     * - Bit  0 - page tables   - not executable (X86_PTE_PAE_NX).
     * - Bit  1 - page tables   - not writable (complemented X86_PTE_RW).
     * - Bit  2 - page tables   - not user (complemented X86_PTE_US).
     * - Bit  3 - pgm phys/virt - not directly writable.
     * - Bit  4 - pgm phys page - not directly readable.
     * - Bit  5 - currently unused.
     * - Bit  6 - page tables   - not dirty (complemented X86_PTE_D).
     * - Bit  7 - tlb entry     - pMappingR3 member not valid.
     * - Bits 63 thru 8 are used for the physical TLB revision number.
     *
     * We're using complemented bit meanings here because it makes it easy to check
     * whether special action is required.  For instance a user mode write access
     * would do a "TEST fFlags, (X86_PTE_RW | X86_PTE_US | X86_PTE_D)" and a
     * non-zero result would mean special handling needed because either it wasn't
     * writable, or it wasn't user, or the page wasn't dirty.  A user mode read
     * access would do "TEST fFlags, X86_PTE_US"; and a kernel mode read wouldn't
     * need to check any PTE flag.
     */
    uint64_t                fFlagsAndPhysRev;
    /** The guest physical page address. */
    uint64_t                GCPhys;
    /** Pointer to the ring-3 mapping (possibly also valid in ring-0). */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    R3PTRTYPE(uint8_t *)    pbMappingR3;
#else
    R3R0PTRTYPE(uint8_t *)  pbMappingR3;
#endif
#if HC_ARCH_BITS == 32
    uint32_t                u32Padding1;
#endif
} IEMTLBENTRY;
AssertCompileSize(IEMTLBENTRY, 32);
/** Pointer to an IEM TLB entry. */
typedef IEMTLBENTRY *PIEMTLBENTRY;

/** @name IEMTLBE_F_XXX - TLB entry flags (IEMTLBENTRY::fFlagsAndPhysRev)
 * @{  */
#define IEMTLBE_F_PT_NO_EXEC        RT_BIT_64(0) /**< Page tables: Not executable. */
#define IEMTLBE_F_PT_NO_WRITE       RT_BIT_64(1) /**< Page tables: Not writable. */
#define IEMTLBE_F_PT_NO_USER        RT_BIT_64(2) /**< Page tables: Not user accessible (supervisor only). */
#define IEMTLBE_F_PG_NO_WRITE       RT_BIT_64(3) /**< Phys page:   Not writable (access handler, ROM, whatever). */
#define IEMTLBE_F_PG_NO_READ        RT_BIT_64(4) /**< Phys page:   Not readable (MMIO / access handler, ROM) */
#define IEMTLBE_F_PATCH_CODE        RT_BIT_64(5) /**< Code TLB:    Patch code (PATM). */
#define IEMTLBE_F_PT_NO_DIRTY       RT_BIT_64(6) /**< Page tables: Not dirty (needs to be made dirty on write). */
#define IEMTLBE_F_NO_MAPPINGR3      RT_BIT_64(7) /**< TLB entry:   The IEMTLBENTRY::pMappingR3 member is invalid. */
#define IEMTLBE_F_PHYS_REV          UINT64_C(0xffffffffffffff00) /**< Physical revision mask. */
/** @} */


/**
 * An IEM TLB.
 *
 * We've got two of these, one for data and one for instructions.
 */
typedef struct IEMTLB
{
    /** The TLB entries.
     * We've choosen 256 because that way we can obtain the result directly from a
     * 8-bit register without an additional AND instruction. */
    IEMTLBENTRY         aEntries[256];
    /** The TLB revision.
     * This is actually only 28 bits wide (see IEMTLBENTRY::uTag) and is incremented
     * by adding RT_BIT_64(36) to it.  When it wraps around and becomes zero, all
     * the tags in the TLB must be zeroed and the revision set to RT_BIT_64(36).
     * (The revision zero indicates an invalid TLB entry.)
     *
     * The initial value is choosen to cause an early wraparound. */
    uint64_t            uTlbRevision;
    /** The TLB physical address revision - shadow of PGM variable.
     *
     * This is actually only 56 bits wide (see IEMTLBENTRY::fFlagsAndPhysRev) and is
     * incremented by adding RT_BIT_64(8).  When it wraps around and becomes zero,
     * a rendezvous is called and each CPU wipe the IEMTLBENTRY::pMappingR3 as well
     * as IEMTLBENTRY::fFlagsAndPhysRev bits 63 thru 8, 4, and 3.
     *
     * The initial value is choosen to cause an early wraparound. */
    uint64_t volatile   uTlbPhysRev;

    /* Statistics: */

    /** TLB hits (VBOX_WITH_STATISTICS only). */
    uint64_t            cTlbHits;
    /** TLB misses. */
    uint32_t            cTlbMisses;
    /** Slow read path.  */
    uint32_t            cTlbSlowReadPath;
#if 0
    /** TLB misses because of tag mismatch. */
    uint32_t            cTlbMissesTag;
    /** TLB misses because of virtual access violation. */
    uint32_t            cTlbMissesVirtAccess;
    /** TLB misses because of dirty bit. */
    uint32_t            cTlbMissesDirty;
    /** TLB misses because of MMIO */
    uint32_t            cTlbMissesMmio;
    /** TLB misses because of write access handlers. */
    uint32_t            cTlbMissesWriteHandler;
    /** TLB misses because no r3(/r0) mapping. */
    uint32_t            cTlbMissesMapping;
#endif
    /** Alignment padding. */
    uint32_t            au32Padding[3+5];
} IEMTLB;
AssertCompileSizeAlignment(IEMTLB, 64);
/** IEMTLB::uTlbRevision increment.  */
#define IEMTLB_REVISION_INCR    RT_BIT_64(36)
/** IEMTLB::uTlbPhysRev increment.  */
#define IEMTLB_PHYS_REV_INCR    RT_BIT_64(8)


/**
 * The per-CPU IEM state.
 */
typedef struct IEMCPU
{
    /** Info status code that needs to be propagated to the IEM caller.
     * This cannot be passed internally, as it would complicate all success
     * checks within the interpreter making the code larger and almost impossible
     * to get right.  Instead, we'll store status codes to pass on here.  Each
     * source of these codes will perform appropriate sanity checks. */
    int32_t                 rcPassUp;                                                                       /* 0x00 */

    /** The current CPU execution mode (CS). */
    IEMMODE                 enmCpuMode;                                                                     /* 0x04 */
    /** The CPL. */
    uint8_t                 uCpl;                                                                           /* 0x05 */

    /** Whether to bypass access handlers or not. */
    bool                    fBypassHandlers;                                                                /* 0x06 */
    /** Indicates that we're interpreting patch code - RC only! */
    bool                    fInPatchCode;                                                                   /* 0x07 */

    /** @name Decoder state.
     * @{ */
#ifdef IEM_WITH_CODE_TLB
    /** The offset of the next instruction byte. */
    uint32_t                offInstrNextByte;                                                               /* 0x08 */
    /** The number of bytes available at pbInstrBuf for the current instruction.
     * This takes the max opcode length into account so that doesn't need to be
     * checked separately. */
    uint32_t                cbInstrBuf;                                                                     /* 0x0c */
    /** Pointer to the page containing RIP, user specified buffer or abOpcode.
     * This can be NULL if the page isn't mappable for some reason, in which
     * case we'll do fallback stuff.
     *
     * If we're executing an instruction from a user specified buffer,
     * IEMExecOneWithPrefetchedByPC and friends, this is not necessarily a page
     * aligned pointer but pointer to the user data.
     *
     * For instructions crossing pages, this will start on the first page and be
     * advanced to the next page by the time we've decoded the instruction.  This
     * therefore precludes stuff like <tt>pbInstrBuf[offInstrNextByte + cbInstrBuf - cbCurInstr]</tt>
     */
    uint8_t const          *pbInstrBuf;                                                                     /* 0x10 */
# if ARCH_BITS == 32
    uint32_t                uInstrBufHigh; /** The high dword of the host context pbInstrBuf member. */
# endif
    /** The program counter corresponding to pbInstrBuf.
     * This is set to a non-canonical address when we need to invalidate it. */
    uint64_t                uInstrBufPc;                                                                    /* 0x18 */
    /** The number of bytes available at pbInstrBuf in total (for IEMExecLots).
     * This takes the CS segment limit into account. */
    uint16_t                cbInstrBufTotal;                                                                /* 0x20 */
    /** Offset into pbInstrBuf of the first byte of the current instruction.
     * Can be negative to efficiently handle cross page instructions. */
    int16_t                 offCurInstrStart;                                                               /* 0x22 */

    /** The prefix mask (IEM_OP_PRF_XXX). */
    uint32_t                fPrefixes;                                                                      /* 0x24 */
    /** The extra REX ModR/M register field bit (REX.R << 3). */
    uint8_t                 uRexReg;                                                                        /* 0x28 */
    /** The extra REX ModR/M r/m field, SIB base and opcode reg bit
     * (REX.B << 3). */
    uint8_t                 uRexB;                                                                          /* 0x29 */
    /** The extra REX SIB index field bit (REX.X << 3). */
    uint8_t                 uRexIndex;                                                                      /* 0x2a */

    /** The effective segment register (X86_SREG_XXX). */
    uint8_t                 iEffSeg;                                                                        /* 0x2b */

#else
    /** The size of what has currently been fetched into abOpcode. */
    uint8_t                 cbOpcode;                                                                       /*       0x08 */
    /** The current offset into abOpcode. */
    uint8_t                 offOpcode;                                                                      /*       0x09 */

    /** The effective segment register (X86_SREG_XXX). */
    uint8_t                 iEffSeg;                                                                        /*       0x0a */

    /** The extra REX ModR/M register field bit (REX.R << 3). */
    uint8_t                 uRexReg;                                                                        /*       0x0b */
    /** The prefix mask (IEM_OP_PRF_XXX). */
    uint32_t                fPrefixes;                                                                      /*       0x0c */
    /** The extra REX ModR/M r/m field, SIB base and opcode reg bit
     * (REX.B << 3). */
    uint8_t                 uRexB;                                                                          /*       0x10 */
    /** The extra REX SIB index field bit (REX.X << 3). */
    uint8_t                 uRexIndex;                                                                      /*       0x11 */

#endif

    /** The effective operand mode. */
    IEMMODE                 enmEffOpSize;                                                                   /* 0x2c, 0x12 */
    /** The default addressing mode. */
    IEMMODE                 enmDefAddrMode;                                                                 /* 0x2d, 0x13 */
    /** The effective addressing mode. */
    IEMMODE                 enmEffAddrMode;                                                                 /* 0x2e, 0x14 */
    /** The default operand mode. */
    IEMMODE                 enmDefOpSize;                                                                   /* 0x2f, 0x15 */

    /** Prefix index (VEX.pp) for two byte and three byte tables. */
    uint8_t                 idxPrefix;                                                                      /* 0x30, 0x16 */
    /** 3rd VEX/EVEX/XOP register.
     * Please use IEM_GET_EFFECTIVE_VVVV to access.  */
    uint8_t                 uVex3rdReg;                                                                     /* 0x31, 0x17 */
    /** The VEX/EVEX/XOP length field. */
    uint8_t                 uVexLength;                                                                     /* 0x32, 0x18 */
    /** Additional EVEX stuff. */
    uint8_t                 fEvexStuff;                                                                     /* 0x33, 0x19 */

    /** The FPU opcode (FOP). */
    uint16_t                uFpuOpcode;                                                                     /* 0x34, 0x1a */

    /** Explicit alignment padding. */
#ifdef IEM_WITH_CODE_TLB
    uint8_t                 abAlignment2a[2];                                                               /* 0x36       */
#endif

    /** The opcode bytes. */
    uint8_t                 abOpcode[15];                                                                   /* 0x48, 0x1c */
    /** Explicit alignment padding. */
#ifdef IEM_WITH_CODE_TLB
    uint8_t                 abAlignment2c[0x48 - 0x47];                                                     /* 0x37 */
#else
    uint8_t                 abAlignment2c[0x48 - 0x2b];                                                     /*       0x2b */
#endif
    /** @} */


    /** The flags of the current exception / interrupt. */
    uint32_t                fCurXcpt;                                                                       /* 0x48, 0x48 */
    /** The current exception / interrupt. */
    uint8_t                 uCurXcpt;
    /** Exception / interrupt recursion depth. */
    int8_t                  cXcptRecursions;

    /** The number of active guest memory mappings. */
    uint8_t                 cActiveMappings;
    /** The next unused mapping index. */
    uint8_t                 iNextMapping;
    /** Records for tracking guest memory mappings. */
    struct
    {
        /** The address of the mapped bytes. */
        void               *pv;
#if defined(IN_RC) && HC_ARCH_BITS == 64
        uint32_t            u32Alignment3; /**< Alignment padding. */
#endif
        /** The access flags (IEM_ACCESS_XXX).
         * IEM_ACCESS_INVALID if the entry is unused. */
        uint32_t            fAccess;
#if HC_ARCH_BITS == 64
        uint32_t            u32Alignment4; /**< Alignment padding. */
#endif
    } aMemMappings[3];

    /** Locking records for the mapped memory. */
    union
    {
        PGMPAGEMAPLOCK      Lock;
        uint64_t            au64Padding[2];
    } aMemMappingLocks[3];

    /** Bounce buffer info.
     * This runs in parallel to aMemMappings. */
    struct
    {
        /** The physical address of the first byte. */
        RTGCPHYS            GCPhysFirst;
        /** The physical address of the second page. */
        RTGCPHYS            GCPhysSecond;
        /** The number of bytes in the first page. */
        uint16_t            cbFirst;
        /** The number of bytes in the second page. */
        uint16_t            cbSecond;
        /** Whether it's unassigned memory. */
        bool                fUnassigned;
        /** Explicit alignment padding. */
        bool                afAlignment5[3];
    } aMemBbMappings[3];

    /** Bounce buffer storage.
     * This runs in parallel to aMemMappings and aMemBbMappings. */
    struct
    {
        uint8_t             ab[512];
    } aBounceBuffers[3];


    /** Pointer set jump buffer - ring-3 context. */
    R3PTRTYPE(jmp_buf *)    pJmpBufR3;
    /** Pointer set jump buffer - ring-0 context. */
    R0PTRTYPE(jmp_buf *)    pJmpBufR0;
    /** Pointer set jump buffer - raw-mode context. */
    RCPTRTYPE(jmp_buf *)    pJmpBufRC;

    /** @todo Should move this near @a fCurXcpt later. */
    /** The error code for the current exception / interrupt. */
    uint32_t                uCurXcptErr;
    /** The CR2 for the current exception / interrupt. */
    uint64_t                uCurXcptCr2;

    /** @name Statistics
     * @{  */
    /** The number of instructions we've executed. */
    uint32_t                cInstructions;
    /** The number of potential exits. */
    uint32_t                cPotentialExits;
    /** The number of bytes data or stack written (mostly for IEMExecOneEx).
     * This may contain uncommitted writes.  */
    uint32_t                cbWritten;
    /** Counts the VERR_IEM_INSTR_NOT_IMPLEMENTED returns. */
    uint32_t                cRetInstrNotImplemented;
    /** Counts the VERR_IEM_ASPECT_NOT_IMPLEMENTED returns. */
    uint32_t                cRetAspectNotImplemented;
    /** Counts informational statuses returned (other than VINF_SUCCESS). */
    uint32_t                cRetInfStatuses;
    /** Counts other error statuses returned. */
    uint32_t                cRetErrStatuses;
    /** Number of times rcPassUp has been used. */
    uint32_t                cRetPassUpStatus;
    /** Number of times RZ left with instruction commit pending for ring-3. */
    uint32_t                cPendingCommit;
    /** Number of long jumps. */
    uint32_t                cLongJumps;
    uint32_t                uAlignment6; /**< Alignment padding. */
#ifdef IEM_VERIFICATION_MODE_FULL
    /** The Number of I/O port reads that has been performed. */
    uint32_t                cIOReads;
    /** The Number of I/O port writes that has been performed. */
    uint32_t                cIOWrites;
    /** Set if no comparison to REM is currently performed.
     * This is used to skip past really slow bits.  */
    bool                    fNoRem;
    /** Saved fNoRem flag used by #iemInitExec and #iemUninitExec. */
    bool                    fNoRemSavedByExec;
    /** Indicates that RAX and RDX differences should be ignored since RDTSC
     *  and RDTSCP are timing sensitive.  */
    bool                    fIgnoreRaxRdx;
    /** Indicates that a MOVS instruction with overlapping source and destination
     *  was executed, causing the memory write records to be incorrrect. */
    bool                    fOverlappingMovs;
    /** Set if there are problematic memory accesses (MMIO, write monitored, ++). */
    bool                    fProblematicMemory;
    /** This is used to communicate a CPL changed caused by IEMInjectTrap that
     * CPUM doesn't yet reflect. */
    uint8_t                 uInjectCpl;
    /** To prevent EMR3HmSingleInstruction from triggering endless recursion via
     *  emR3ExecuteInstruction and iemExecVerificationModeCheck. */
    uint8_t                 cVerifyDepth;
    bool                    afAlignment7[2];
    /** Mask of undefined eflags.
     * The verifier will any difference in these flags. */
    uint32_t                fUndefinedEFlags;
    /** The CS of the instruction being interpreted. */
    RTSEL                   uOldCs;
    /** The RIP of the instruction being interpreted. */
    uint64_t                uOldRip;
    /** The physical address corresponding to abOpcodes[0]. */
    RTGCPHYS                GCPhysOpcodes;
#endif
    /** @} */

    /** @name Target CPU information.
     * @{ */
#if IEM_CFG_TARGET_CPU == IEMTARGETCPU_DYNAMIC
    /** The target CPU. */
    uint32_t                uTargetCpu;
#else
    uint32_t                u32TargetCpuPadding;
#endif
    /** The CPU vendor. */
    CPUMCPUVENDOR           enmCpuVendor;
    /** @} */

    /** @name Host CPU information.
     * @{ */
    /** The CPU vendor. */
    CPUMCPUVENDOR           enmHostCpuVendor;
    /** @} */

    uint32_t                au32Alignment8[HC_ARCH_BITS == 64 ? 4 + 8 : 4]; /**< Alignment padding. */

    /** Data TLB.
     * @remarks Must be 64-byte aligned. */
    IEMTLB                  DataTlb;
    /** Instruction TLB.
     * @remarks Must be 64-byte aligned. */
    IEMTLB                  CodeTlb;

    /** Pointer to the CPU context - ring-3 context.
     * @todo put inside IEM_VERIFICATION_MODE_FULL++. */
    R3PTRTYPE(PCPUMCTX)     pCtxR3;
    /** Pointer to the CPU context - ring-0 context. */
    R0PTRTYPE(PCPUMCTX)     pCtxR0;
    /** Pointer to the CPU context - raw-mode context. */
    RCPTRTYPE(PCPUMCTX)     pCtxRC;

    /** Pointer to instruction statistics for raw-mode context (same as R0). */
    RCPTRTYPE(PIEMINSTRSTATS) pStatsRC;
    /** Pointer to instruction statistics for ring-0 context (same as RC). */
    R0PTRTYPE(PIEMINSTRSTATS) pStatsR0;
    /** Pointer to instruction statistics for non-ring-3 code. */
    R3PTRTYPE(PIEMINSTRSTATS) pStatsCCR3;
    /** Pointer to instruction statistics for ring-3 context. */
    R3PTRTYPE(PIEMINSTRSTATS) pStatsR3;

#ifdef IEM_VERIFICATION_MODE_FULL
    /** The event verification records for what IEM did (LIFO). */
    R3PTRTYPE(PIEMVERIFYEVTREC)     pIemEvtRecHead;
    /** Insertion point for pIemEvtRecHead. */
    R3PTRTYPE(PIEMVERIFYEVTREC *)   ppIemEvtRecNext;
    /** The event verification records for what the other party did (FIFO). */
    R3PTRTYPE(PIEMVERIFYEVTREC)     pOtherEvtRecHead;
    /** Insertion point for pOtherEvtRecHead. */
    R3PTRTYPE(PIEMVERIFYEVTREC *)   ppOtherEvtRecNext;
    /** List of free event records. */
    R3PTRTYPE(PIEMVERIFYEVTREC)     pFreeEvtRec;
#endif
} IEMCPU;
AssertCompileMemberOffset(IEMCPU, fCurXcpt, 0x48);
AssertCompileMemberAlignment(IEMCPU, DataTlb, 64);
AssertCompileMemberAlignment(IEMCPU, CodeTlb, 64);
/** Pointer to the per-CPU IEM state. */
typedef IEMCPU *PIEMCPU;
/** Pointer to the const per-CPU IEM state. */
typedef IEMCPU const *PCIEMCPU;


/** @def IEM_GET_CTX
 * Gets the guest CPU context for the calling EMT.
 * @returns PCPUMCTX
 * @param   a_pVCpu The cross context virtual CPU structure of the calling thread.
 */
#if !defined(IEM_VERIFICATION_MODE_FULL) && !defined(IEM_VERIFICATION_MODE) \
 && !defined(IEM_VERIFICATION_MODE_MINIMAL) && defined(VMCPU_INCL_CPUM_GST_CTX)
# define IEM_GET_CTX(a_pVCpu)           (&(a_pVCpu)->cpum.GstCtx)
#else
# define IEM_GET_CTX(a_pVCpu)           ((a_pVCpu)->iem.s.CTX_SUFF(pCtx))
#endif

/** Gets the current IEMTARGETCPU value.
 * @returns IEMTARGETCPU value.
 * @param   a_pVCpu The cross context virtual CPU structure of the calling thread.
 */
#if IEM_CFG_TARGET_CPU != IEMTARGETCPU_DYNAMIC
# define IEM_GET_TARGET_CPU(a_pVCpu)    (IEM_CFG_TARGET_CPU)
#else
# define IEM_GET_TARGET_CPU(a_pVCpu)    ((a_pVCpu)->iem.s.uTargetCpu)
#endif

/** @def Gets the instruction length. */
#ifdef IEM_WITH_CODE_TLB
# define IEM_GET_INSTR_LEN(a_pVCpu)     ((a_pVCpu)->iem.s.offInstrNextByte - (uint32_t)(int32_t)(a_pVCpu)->iem.s.offCurInstrStart)
#else
# define IEM_GET_INSTR_LEN(a_pVCpu)     ((a_pVCpu)->iem.s.offOpcode)
#endif


/** @name IEM_ACCESS_XXX - Access details.
 * @{ */
#define IEM_ACCESS_INVALID              UINT32_C(0x000000ff)
#define IEM_ACCESS_TYPE_READ            UINT32_C(0x00000001)
#define IEM_ACCESS_TYPE_WRITE           UINT32_C(0x00000002)
#define IEM_ACCESS_TYPE_EXEC            UINT32_C(0x00000004)
#define IEM_ACCESS_TYPE_MASK            UINT32_C(0x00000007)
#define IEM_ACCESS_WHAT_CODE            UINT32_C(0x00000010)
#define IEM_ACCESS_WHAT_DATA            UINT32_C(0x00000020)
#define IEM_ACCESS_WHAT_STACK           UINT32_C(0x00000030)
#define IEM_ACCESS_WHAT_SYS             UINT32_C(0x00000040)
#define IEM_ACCESS_WHAT_MASK            UINT32_C(0x00000070)
/** The writes are partial, so if initialize the bounce buffer with the
 * orignal RAM content. */
#define IEM_ACCESS_PARTIAL_WRITE        UINT32_C(0x00000100)
/** Used in aMemMappings to indicate that the entry is bounce buffered. */
#define IEM_ACCESS_BOUNCE_BUFFERED      UINT32_C(0x00000200)
/** Bounce buffer with ring-3 write pending, first page. */
#define IEM_ACCESS_PENDING_R3_WRITE_1ST UINT32_C(0x00000400)
/** Bounce buffer with ring-3 write pending, second page. */
#define IEM_ACCESS_PENDING_R3_WRITE_2ND UINT32_C(0x00000800)
/** Valid bit mask. */
#define IEM_ACCESS_VALID_MASK           UINT32_C(0x00000fff)
/** Read+write data alias. */
#define IEM_ACCESS_DATA_RW              (IEM_ACCESS_TYPE_READ  | IEM_ACCESS_TYPE_WRITE | IEM_ACCESS_WHAT_DATA)
/** Write data alias. */
#define IEM_ACCESS_DATA_W               (IEM_ACCESS_TYPE_WRITE | IEM_ACCESS_WHAT_DATA)
/** Read data alias. */
#define IEM_ACCESS_DATA_R               (IEM_ACCESS_TYPE_READ  | IEM_ACCESS_WHAT_DATA)
/** Instruction fetch alias. */
#define IEM_ACCESS_INSTRUCTION          (IEM_ACCESS_TYPE_EXEC  | IEM_ACCESS_WHAT_CODE)
/** Stack write alias. */
#define IEM_ACCESS_STACK_W              (IEM_ACCESS_TYPE_WRITE | IEM_ACCESS_WHAT_STACK)
/** Stack read alias. */
#define IEM_ACCESS_STACK_R              (IEM_ACCESS_TYPE_READ  | IEM_ACCESS_WHAT_STACK)
/** Stack read+write alias. */
#define IEM_ACCESS_STACK_RW             (IEM_ACCESS_TYPE_READ  | IEM_ACCESS_TYPE_WRITE | IEM_ACCESS_WHAT_STACK)
/** Read system table alias. */
#define IEM_ACCESS_SYS_R                (IEM_ACCESS_TYPE_READ  | IEM_ACCESS_WHAT_SYS)
/** Read+write system table alias. */
#define IEM_ACCESS_SYS_RW               (IEM_ACCESS_TYPE_READ  | IEM_ACCESS_TYPE_WRITE | IEM_ACCESS_WHAT_SYS)
/** @} */

/** @name Prefix constants (IEMCPU::fPrefixes)
 * @{ */
#define IEM_OP_PRF_SEG_CS               RT_BIT_32(0)  /**< CS segment prefix (0x2e). */
#define IEM_OP_PRF_SEG_SS               RT_BIT_32(1)  /**< SS segment prefix (0x36). */
#define IEM_OP_PRF_SEG_DS               RT_BIT_32(2)  /**< DS segment prefix (0x3e). */
#define IEM_OP_PRF_SEG_ES               RT_BIT_32(3)  /**< ES segment prefix (0x26). */
#define IEM_OP_PRF_SEG_FS               RT_BIT_32(4)  /**< FS segment prefix (0x64). */
#define IEM_OP_PRF_SEG_GS               RT_BIT_32(5)  /**< GS segment prefix (0x65). */
#define IEM_OP_PRF_SEG_MASK             UINT32_C(0x3f)

#define IEM_OP_PRF_SIZE_OP              RT_BIT_32(8)  /**< Operand size prefix (0x66). */
#define IEM_OP_PRF_SIZE_REX_W           RT_BIT_32(9)  /**< REX.W prefix (0x48-0x4f). */
#define IEM_OP_PRF_SIZE_ADDR            RT_BIT_32(10) /**< Address size prefix (0x67). */

#define IEM_OP_PRF_LOCK                 RT_BIT_32(16) /**< Lock prefix (0xf0). */
#define IEM_OP_PRF_REPNZ                RT_BIT_32(17) /**< Repeat-not-zero prefix (0xf2). */
#define IEM_OP_PRF_REPZ                 RT_BIT_32(18) /**< Repeat-if-zero prefix (0xf3). */

#define IEM_OP_PRF_REX                  RT_BIT_32(24) /**< Any REX prefix (0x40-0x4f). */
#define IEM_OP_PRF_REX_R                RT_BIT_32(25) /**< REX.R prefix (0x44,0x45,0x46,0x47,0x4c,0x4d,0x4e,0x4f). */
#define IEM_OP_PRF_REX_B                RT_BIT_32(26) /**< REX.B prefix (0x41,0x43,0x45,0x47,0x49,0x4b,0x4d,0x4f). */
#define IEM_OP_PRF_REX_X                RT_BIT_32(27) /**< REX.X prefix (0x42,0x43,0x46,0x47,0x4a,0x4b,0x4e,0x4f). */
/** Mask with all the REX prefix flags.
 * This is generally for use when needing to undo the REX prefixes when they
 * are followed legacy prefixes and therefore does not immediately preceed
 * the first opcode byte.
 * For testing whether any REX prefix is present, use  IEM_OP_PRF_REX instead. */
#define IEM_OP_PRF_REX_MASK  (IEM_OP_PRF_REX | IEM_OP_PRF_REX_R | IEM_OP_PRF_REX_B | IEM_OP_PRF_REX_X | IEM_OP_PRF_SIZE_REX_W )

#define IEM_OP_PRF_VEX                  RT_BIT_32(28) /**< Indiciates VEX prefix. */
#define IEM_OP_PRF_EVEX                 RT_BIT_32(29) /**< Indiciates EVEX prefix. */
#define IEM_OP_PRF_XOP                  RT_BIT_32(30) /**< Indiciates XOP prefix. */
/** @} */

/** @name IEMOPFORM_XXX - Opcode forms
 * @note These are ORed together with IEMOPHINT_XXX.
 * @{ */
/** ModR/M: reg, r/m */
#define IEMOPFORM_RM            0
/** ModR/M: reg, r/m (register) */
#define IEMOPFORM_RM_REG        (IEMOPFORM_RM | IEMOPFORM_MOD3)
/** ModR/M: reg, r/m (memory)   */
#define IEMOPFORM_RM_MEM        (IEMOPFORM_RM | IEMOPFORM_NOT_MOD3)
/** ModR/M: r/m, reg */
#define IEMOPFORM_MR            1
/** ModR/M: r/m (register), reg */
#define IEMOPFORM_MR_REG        (IEMOPFORM_MR | IEMOPFORM_MOD3)
/** ModR/M: r/m (memory), reg */
#define IEMOPFORM_MR_MEM        (IEMOPFORM_MR | IEMOPFORM_NOT_MOD3)
/** ModR/M: r/m only */
#define IEMOPFORM_M             2
/** ModR/M: r/m only (register). */
#define IEMOPFORM_M_REG         (IEMOPFORM_M | IEMOPFORM_MOD3)
/** ModR/M: r/m only (memory). */
#define IEMOPFORM_M_MEM         (IEMOPFORM_M | IEMOPFORM_NOT_MOD3)
/** ModR/M: reg only */
#define IEMOPFORM_R             3

/** VEX+ModR/M: reg, r/m */
#define IEMOPFORM_VEX_RM        4
/** VEX+ModR/M: reg, r/m (register) */
#define IEMOPFORM_VEX_RM_REG    (IEMOPFORM_VEX_RM | IEMOPFORM_MOD3)
/** VEX+ModR/M: reg, r/m (memory)   */
#define IEMOPFORM_VEX_RM_MEM    (IEMOPFORM_VEX_RM | IEMOPFORM_NOT_MOD3)
/** VEX+ModR/M: r/m, reg */
#define IEMOPFORM_VEX_MR        5
/** VEX+ModR/M: r/m (register), reg */
#define IEMOPFORM_VEX_MR_REG    (IEMOPFORM_VEX_MR | IEMOPFORM_MOD3)
/** VEX+ModR/M: r/m (memory), reg */
#define IEMOPFORM_VEX_MR_MEM    (IEMOPFORM_VEX_MR | IEMOPFORM_NOT_MOD3)
/** VEX+ModR/M: r/m only */
#define IEMOPFORM_VEX_M         6
/** VEX+ModR/M: r/m only (register). */
#define IEMOPFORM_VEX_M_REG     (IEMOPFORM_VEX_M | IEMOPFORM_MOD3)
/** VEX+ModR/M: r/m only (memory). */
#define IEMOPFORM_VEX_M_MEM     (IEMOPFORM_VEX_M | IEMOPFORM_NOT_MOD3)
/** VEX+ModR/M: reg only */
#define IEMOPFORM_VEX_R         7
/** VEX+ModR/M: reg, vvvv, r/m */
#define IEMOPFORM_VEX_RVM       8
/** VEX+ModR/M: reg, vvvv, r/m (register). */
#define IEMOPFORM_VEX_RVM_REG   (IEMOPFORM_VEX_RVM | IEMOPFORM_MOD3)
/** VEX+ModR/M: reg, vvvv, r/m (memory). */
#define IEMOPFORM_VEX_RVM_MEM   (IEMOPFORM_VEX_RVM | IEMOPFORM_NOT_MOD3)
/** VEX+ModR/M: r/m, vvvv, reg */
#define IEMOPFORM_VEX_MVR       9
/** VEX+ModR/M: r/m, vvvv, reg (register) */
#define IEMOPFORM_VEX_MVR_REG   (IEMOPFORM_VEX_MVR | IEMOPFORM_MOD3)
/** VEX+ModR/M: r/m, vvvv, reg (memory) */
#define IEMOPFORM_VEX_MVR_MEM   (IEMOPFORM_VEX_MVR | IEMOPFORM_NOT_MOD3)

/** Fixed register instruction, no R/M. */
#define IEMOPFORM_FIXED         16

/** The r/m is a register. */
#define IEMOPFORM_MOD3          RT_BIT_32(8)
/** The r/m is a memory access. */
#define IEMOPFORM_NOT_MOD3      RT_BIT_32(9)
/** @} */

/** @name IEMOPHINT_XXX - Additional Opcode Hints
 * @note These are ORed together with IEMOPFORM_XXX.
 * @{ */
/** Ignores the operand size prefix (66h). */
#define IEMOPHINT_IGNORES_OZ_PFX    RT_BIT_32(10)
/** Ignores REX.W (aka WIG). */
#define IEMOPHINT_IGNORES_REXW      RT_BIT_32(11)
/** Both the operand size prefixes (66h + REX.W) are ignored. */
#define IEMOPHINT_IGNORES_OP_SIZES  (IEMOPHINT_IGNORES_OZ_PFX | IEMOPHINT_IGNORES_REXW)
/** Allowed with the lock prefix. */
#define IEMOPHINT_LOCK_ALLOWED      RT_BIT_32(11)
/** The VEX.L value is ignored (aka LIG). */
#define IEMOPHINT_VEX_L_IGNORED     RT_BIT_32(12)
/** The VEX.L value must be zero (i.e. 128-bit width only). */
#define IEMOPHINT_VEX_L_ZERO        RT_BIT_32(13)

/** Hint to IEMAllInstructionPython.py that this macro should be skipped.  */
#define IEMOPHINT_SKIP_PYTHON       RT_BIT_32(31)
/** @} */

/**
 * Possible hardware task switch sources.
 */
typedef enum IEMTASKSWITCH
{
    /** Task switch caused by an interrupt/exception. */
    IEMTASKSWITCH_INT_XCPT = 1,
    /** Task switch caused by a far CALL. */
    IEMTASKSWITCH_CALL,
    /** Task switch caused by a far JMP. */
    IEMTASKSWITCH_JUMP,
    /** Task switch caused by an IRET. */
    IEMTASKSWITCH_IRET
} IEMTASKSWITCH;
AssertCompileSize(IEMTASKSWITCH, 4);

/**
 * Possible CrX load (write) sources.
 */
typedef enum IEMACCESSCRX
{
    /** CrX access caused by 'mov crX' instruction. */
    IEMACCESSCRX_MOV_CRX,
    /** CrX (CR0) write caused by 'lmsw' instruction. */
    IEMACCESSCRX_LMSW,
    /** CrX (CR0) write caused by 'clts' instruction. */
    IEMACCESSCRX_CLTS,
    /** CrX (CR0) read caused by 'smsw' instruction. */
    IEMACCESSCRX_SMSW
} IEMACCESSCRX;

/**
 * Tests if verification mode is enabled.
 *
 * This expands to @c false when IEM_VERIFICATION_MODE is not defined and
 * should therefore cause the compiler to eliminate the verification branch
 * of an if statement.  */
#ifdef IEM_VERIFICATION_MODE_FULL
# define IEM_VERIFICATION_ENABLED(a_pVCpu)      (!(a_pVCpu)->iem.s.fNoRem)
#elif defined(IEM_VERIFICATION_MODE_MINIMAL)
# define IEM_VERIFICATION_ENABLED(a_pVCpu)      (true)
#else
# define IEM_VERIFICATION_ENABLED(a_pVCpu)      (false)
#endif

/**
 * Tests if full verification mode is enabled.
 *
 * This expands to @c false when IEM_VERIFICATION_MODE_FULL is not defined and
 * should therefore cause the compiler to eliminate the verification branch
 * of an if statement.  */
#ifdef IEM_VERIFICATION_MODE_FULL
# define IEM_FULL_VERIFICATION_ENABLED(a_pVCpu) (!(a_pVCpu)->iem.s.fNoRem)
#else
# define IEM_FULL_VERIFICATION_ENABLED(a_pVCpu) (false)
#endif

/**
 * Tests if full verification mode is enabled again REM.
 *
 * This expands to @c false when IEM_VERIFICATION_MODE_FULL is not defined and
 * should therefore cause the compiler to eliminate the verification branch
 * of an if statement.  */
#ifdef IEM_VERIFICATION_MODE_FULL
# ifdef IEM_VERIFICATION_MODE_FULL_HM
#  define IEM_FULL_VERIFICATION_REM_ENABLED(a_pVCpu)    (!(a_pVCpu)->iem.s.fNoRem && !HMIsEnabled((a_pVCpu)->CTX_SUFF(pVM)))
# else
#  define IEM_FULL_VERIFICATION_REM_ENABLED(a_pVCpu)    (!(a_pVCpu)->iem.s.fNoRem)
# endif
#else
# define IEM_FULL_VERIFICATION_REM_ENABLED(a_pVCpu)     (false)
#endif

/** @def IEM_VERIFICATION_MODE
 * Indicates that one of the verfication modes are enabled.
 */
#if (defined(IEM_VERIFICATION_MODE_FULL) || defined(IEM_VERIFICATION_MODE_MINIMAL)) && !defined(IEM_VERIFICATION_MODE) \
 || defined(DOXYGEN_RUNNING)
# define IEM_VERIFICATION_MODE
#endif

/**
 * Indicates to the verifier that the given flag set is undefined.
 *
 * Can be invoked again to add more flags.
 *
 * This is a NOOP if the verifier isn't compiled in.
 */
#ifdef IEM_VERIFICATION_MODE_FULL
# define IEMOP_VERIFICATION_UNDEFINED_EFLAGS(a_fEfl) do { pVCpu->iem.s.fUndefinedEFlags |= (a_fEfl); } while (0)
#else
# define IEMOP_VERIFICATION_UNDEFINED_EFLAGS(a_fEfl) do { } while (0)
#endif


/** @def IEM_DECL_IMPL_TYPE
 * For typedef'ing an instruction implementation function.
 *
 * @param   a_RetType           The return type.
 * @param   a_Name              The name of the type.
 * @param   a_ArgList           The argument list enclosed in parentheses.
 */

/** @def IEM_DECL_IMPL_DEF
 * For defining an instruction implementation function.
 *
 * @param   a_RetType           The return type.
 * @param   a_Name              The name of the type.
 * @param   a_ArgList           The argument list enclosed in parentheses.
 */

#if defined(__GNUC__) && defined(RT_ARCH_X86)
# define IEM_DECL_IMPL_TYPE(a_RetType, a_Name, a_ArgList) \
    __attribute__((__fastcall__)) a_RetType (a_Name) a_ArgList
# define IEM_DECL_IMPL_DEF(a_RetType, a_Name, a_ArgList) \
    __attribute__((__fastcall__, __nothrow__)) a_RetType a_Name a_ArgList

#elif defined(_MSC_VER) && defined(RT_ARCH_X86)
# define IEM_DECL_IMPL_TYPE(a_RetType, a_Name, a_ArgList) \
    a_RetType (__fastcall a_Name) a_ArgList
# define IEM_DECL_IMPL_DEF(a_RetType, a_Name, a_ArgList) \
    a_RetType __fastcall a_Name a_ArgList

#else
# define IEM_DECL_IMPL_TYPE(a_RetType, a_Name, a_ArgList) \
    a_RetType (VBOXCALL a_Name) a_ArgList
# define IEM_DECL_IMPL_DEF(a_RetType, a_Name, a_ArgList) \
    a_RetType VBOXCALL a_Name a_ArgList

#endif

/** @name Arithmetic assignment operations on bytes (binary).
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLBINU8,  (uint8_t  *pu8Dst,  uint8_t  u8Src,  uint32_t *pEFlags));
typedef FNIEMAIMPLBINU8  *PFNIEMAIMPLBINU8;
FNIEMAIMPLBINU8 iemAImpl_add_u8, iemAImpl_add_u8_locked;
FNIEMAIMPLBINU8 iemAImpl_adc_u8, iemAImpl_adc_u8_locked;
FNIEMAIMPLBINU8 iemAImpl_sub_u8, iemAImpl_sub_u8_locked;
FNIEMAIMPLBINU8 iemAImpl_sbb_u8, iemAImpl_sbb_u8_locked;
FNIEMAIMPLBINU8  iemAImpl_or_u8,  iemAImpl_or_u8_locked;
FNIEMAIMPLBINU8 iemAImpl_xor_u8, iemAImpl_xor_u8_locked;
FNIEMAIMPLBINU8 iemAImpl_and_u8, iemAImpl_and_u8_locked;
/** @} */

/** @name Arithmetic assignment operations on words (binary).
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLBINU16,  (uint16_t *pu16Dst, uint16_t u16Src, uint32_t *pEFlags));
typedef FNIEMAIMPLBINU16  *PFNIEMAIMPLBINU16;
FNIEMAIMPLBINU16 iemAImpl_add_u16, iemAImpl_add_u16_locked;
FNIEMAIMPLBINU16 iemAImpl_adc_u16, iemAImpl_adc_u16_locked;
FNIEMAIMPLBINU16 iemAImpl_sub_u16, iemAImpl_sub_u16_locked;
FNIEMAIMPLBINU16 iemAImpl_sbb_u16, iemAImpl_sbb_u16_locked;
FNIEMAIMPLBINU16  iemAImpl_or_u16,  iemAImpl_or_u16_locked;
FNIEMAIMPLBINU16 iemAImpl_xor_u16, iemAImpl_xor_u16_locked;
FNIEMAIMPLBINU16 iemAImpl_and_u16, iemAImpl_and_u16_locked;
/** @}  */

/** @name Arithmetic assignment operations on double words (binary).
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLBINU32, (uint32_t *pu32Dst, uint32_t u32Src, uint32_t *pEFlags));
typedef FNIEMAIMPLBINU32 *PFNIEMAIMPLBINU32;
FNIEMAIMPLBINU32 iemAImpl_add_u32, iemAImpl_add_u32_locked;
FNIEMAIMPLBINU32 iemAImpl_adc_u32, iemAImpl_adc_u32_locked;
FNIEMAIMPLBINU32 iemAImpl_sub_u32, iemAImpl_sub_u32_locked;
FNIEMAIMPLBINU32 iemAImpl_sbb_u32, iemAImpl_sbb_u32_locked;
FNIEMAIMPLBINU32  iemAImpl_or_u32,  iemAImpl_or_u32_locked;
FNIEMAIMPLBINU32 iemAImpl_xor_u32, iemAImpl_xor_u32_locked;
FNIEMAIMPLBINU32 iemAImpl_and_u32, iemAImpl_and_u32_locked;
/** @}  */

/** @name Arithmetic assignment operations on quad words (binary).
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLBINU64, (uint64_t *pu64Dst, uint64_t u64Src, uint32_t *pEFlags));
typedef FNIEMAIMPLBINU64 *PFNIEMAIMPLBINU64;
FNIEMAIMPLBINU64 iemAImpl_add_u64, iemAImpl_add_u64_locked;
FNIEMAIMPLBINU64 iemAImpl_adc_u64, iemAImpl_adc_u64_locked;
FNIEMAIMPLBINU64 iemAImpl_sub_u64, iemAImpl_sub_u64_locked;
FNIEMAIMPLBINU64 iemAImpl_sbb_u64, iemAImpl_sbb_u64_locked;
FNIEMAIMPLBINU64  iemAImpl_or_u64,  iemAImpl_or_u64_locked;
FNIEMAIMPLBINU64 iemAImpl_xor_u64, iemAImpl_xor_u64_locked;
FNIEMAIMPLBINU64 iemAImpl_and_u64, iemAImpl_and_u64_locked;
/** @}  */

/** @name Compare operations (thrown in with the binary ops).
 * @{ */
FNIEMAIMPLBINU8  iemAImpl_cmp_u8;
FNIEMAIMPLBINU16 iemAImpl_cmp_u16;
FNIEMAIMPLBINU32 iemAImpl_cmp_u32;
FNIEMAIMPLBINU64 iemAImpl_cmp_u64;
/** @}  */

/** @name Test operations (thrown in with the binary ops).
 * @{ */
FNIEMAIMPLBINU8  iemAImpl_test_u8;
FNIEMAIMPLBINU16 iemAImpl_test_u16;
FNIEMAIMPLBINU32 iemAImpl_test_u32;
FNIEMAIMPLBINU64 iemAImpl_test_u64;
/** @}  */

/** @name Bit operations operations (thrown in with the binary ops).
 * @{ */
FNIEMAIMPLBINU16 iemAImpl_bt_u16,  iemAImpl_bt_u16_locked;
FNIEMAIMPLBINU32 iemAImpl_bt_u32,  iemAImpl_bt_u32_locked;
FNIEMAIMPLBINU64 iemAImpl_bt_u64,  iemAImpl_bt_u64_locked;
FNIEMAIMPLBINU16 iemAImpl_btc_u16, iemAImpl_btc_u16_locked;
FNIEMAIMPLBINU32 iemAImpl_btc_u32, iemAImpl_btc_u32_locked;
FNIEMAIMPLBINU64 iemAImpl_btc_u64, iemAImpl_btc_u64_locked;
FNIEMAIMPLBINU16 iemAImpl_btr_u16, iemAImpl_btr_u16_locked;
FNIEMAIMPLBINU32 iemAImpl_btr_u32, iemAImpl_btr_u32_locked;
FNIEMAIMPLBINU64 iemAImpl_btr_u64, iemAImpl_btr_u64_locked;
FNIEMAIMPLBINU16 iemAImpl_bts_u16, iemAImpl_bts_u16_locked;
FNIEMAIMPLBINU32 iemAImpl_bts_u32, iemAImpl_bts_u32_locked;
FNIEMAIMPLBINU64 iemAImpl_bts_u64, iemAImpl_bts_u64_locked;
/** @}  */

/** @name Exchange memory with register operations.
 * @{ */
IEM_DECL_IMPL_DEF(void, iemAImpl_xchg_u8, (uint8_t  *pu8Mem,  uint8_t  *pu8Reg));
IEM_DECL_IMPL_DEF(void, iemAImpl_xchg_u16,(uint16_t *pu16Mem, uint16_t *pu16Reg));
IEM_DECL_IMPL_DEF(void, iemAImpl_xchg_u32,(uint32_t *pu32Mem, uint32_t *pu32Reg));
IEM_DECL_IMPL_DEF(void, iemAImpl_xchg_u64,(uint64_t *pu64Mem, uint64_t *pu64Reg));
/** @}  */

/** @name Exchange and add operations.
 * @{ */
IEM_DECL_IMPL_DEF(void, iemAImpl_xadd_u8, (uint8_t  *pu8Dst,  uint8_t  *pu8Reg,  uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_xadd_u16,(uint16_t *pu16Dst, uint16_t *pu16Reg, uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_xadd_u32,(uint32_t *pu32Dst, uint32_t *pu32Reg, uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_xadd_u64,(uint64_t *pu64Dst, uint64_t *pu64Reg, uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_xadd_u8_locked, (uint8_t  *pu8Dst,  uint8_t  *pu8Reg,  uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_xadd_u16_locked,(uint16_t *pu16Dst, uint16_t *pu16Reg, uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_xadd_u32_locked,(uint32_t *pu32Dst, uint32_t *pu32Reg, uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_xadd_u64_locked,(uint64_t *pu64Dst, uint64_t *pu64Reg, uint32_t *pEFlags));
/** @}  */

/** @name Compare and exchange.
 * @{ */
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg_u8,        (uint8_t  *pu8Dst,  uint8_t  *puAl,  uint8_t  uSrcReg, uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg_u8_locked, (uint8_t  *pu8Dst,  uint8_t  *puAl,  uint8_t  uSrcReg, uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg_u16,       (uint16_t *pu16Dst, uint16_t *puAx,  uint16_t uSrcReg, uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg_u16_locked,(uint16_t *pu16Dst, uint16_t *puAx,  uint16_t uSrcReg, uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg_u32,       (uint32_t *pu32Dst, uint32_t *puEax, uint32_t uSrcReg, uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg_u32_locked,(uint32_t *pu32Dst, uint32_t *puEax, uint32_t uSrcReg, uint32_t *pEFlags));
#ifdef RT_ARCH_X86
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg_u64,       (uint64_t *pu64Dst, uint64_t *puRax, uint64_t *puSrcReg, uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg_u64_locked,(uint64_t *pu64Dst, uint64_t *puRax, uint64_t *puSrcReg, uint32_t *pEFlags));
#else
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg_u64,       (uint64_t *pu64Dst, uint64_t *puRax, uint64_t uSrcReg, uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg_u64_locked,(uint64_t *pu64Dst, uint64_t *puRax, uint64_t uSrcReg, uint32_t *pEFlags));
#endif
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg8b,(uint64_t *pu64Dst, PRTUINT64U pu64EaxEdx, PRTUINT64U pu64EbxEcx,
                                            uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg8b_locked,(uint64_t *pu64Dst, PRTUINT64U pu64EaxEdx, PRTUINT64U pu64EbxEcx,
                                                   uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg16b,(PRTUINT128U pu128Dst, PRTUINT128U pu128RaxRdx, PRTUINT128U pu128RbxRcx,
                                             uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg16b_locked,(PRTUINT128U pu128Dst, PRTUINT128U pu128RaxRdx, PRTUINT128U pu128RbxRcx,
                                                    uint32_t *pEFlags));
IEM_DECL_IMPL_DEF(void, iemAImpl_cmpxchg16b_fallback,(PRTUINT128U pu128Dst, PRTUINT128U pu128RaxRdx,
                                                      PRTUINT128U pu128RbxRcx, uint32_t *pEFlags));
/** @} */

/** @name Memory ordering
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLMEMFENCE,(void));
typedef FNIEMAIMPLMEMFENCE *PFNIEMAIMPLMEMFENCE;
IEM_DECL_IMPL_DEF(void, iemAImpl_mfence,(void));
IEM_DECL_IMPL_DEF(void, iemAImpl_sfence,(void));
IEM_DECL_IMPL_DEF(void, iemAImpl_lfence,(void));
IEM_DECL_IMPL_DEF(void, iemAImpl_alt_mem_fence,(void));
/** @} */

/** @name Double precision shifts
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLSHIFTDBLU16,(uint16_t *pu16Dst, uint16_t u16Src, uint8_t cShift, uint32_t *pEFlags));
typedef FNIEMAIMPLSHIFTDBLU16  *PFNIEMAIMPLSHIFTDBLU16;
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLSHIFTDBLU32,(uint32_t *pu32Dst, uint32_t u32Src, uint8_t cShift, uint32_t *pEFlags));
typedef FNIEMAIMPLSHIFTDBLU32  *PFNIEMAIMPLSHIFTDBLU32;
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLSHIFTDBLU64,(uint64_t *pu64Dst, uint64_t u64Src, uint8_t cShift, uint32_t *pEFlags));
typedef FNIEMAIMPLSHIFTDBLU64  *PFNIEMAIMPLSHIFTDBLU64;
FNIEMAIMPLSHIFTDBLU16 iemAImpl_shld_u16;
FNIEMAIMPLSHIFTDBLU32 iemAImpl_shld_u32;
FNIEMAIMPLSHIFTDBLU64 iemAImpl_shld_u64;
FNIEMAIMPLSHIFTDBLU16 iemAImpl_shrd_u16;
FNIEMAIMPLSHIFTDBLU32 iemAImpl_shrd_u32;
FNIEMAIMPLSHIFTDBLU64 iemAImpl_shrd_u64;
/** @}  */


/** @name Bit search operations (thrown in with the binary ops).
 * @{ */
FNIEMAIMPLBINU16 iemAImpl_bsf_u16;
FNIEMAIMPLBINU32 iemAImpl_bsf_u32;
FNIEMAIMPLBINU64 iemAImpl_bsf_u64;
FNIEMAIMPLBINU16 iemAImpl_bsr_u16;
FNIEMAIMPLBINU32 iemAImpl_bsr_u32;
FNIEMAIMPLBINU64 iemAImpl_bsr_u64;
/** @}  */

/** @name Signed multiplication operations (thrown in with the binary ops).
 * @{ */
FNIEMAIMPLBINU16 iemAImpl_imul_two_u16;
FNIEMAIMPLBINU32 iemAImpl_imul_two_u32;
FNIEMAIMPLBINU64 iemAImpl_imul_two_u64;
/** @}  */

/** @name Arithmetic assignment operations on bytes (unary).
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLUNARYU8,  (uint8_t  *pu8Dst,  uint32_t *pEFlags));
typedef FNIEMAIMPLUNARYU8  *PFNIEMAIMPLUNARYU8;
FNIEMAIMPLUNARYU8 iemAImpl_inc_u8, iemAImpl_inc_u8_locked;
FNIEMAIMPLUNARYU8 iemAImpl_dec_u8, iemAImpl_dec_u8_locked;
FNIEMAIMPLUNARYU8 iemAImpl_not_u8, iemAImpl_not_u8_locked;
FNIEMAIMPLUNARYU8 iemAImpl_neg_u8, iemAImpl_neg_u8_locked;
/** @} */

/** @name Arithmetic assignment operations on words (unary).
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLUNARYU16,  (uint16_t  *pu16Dst,  uint32_t *pEFlags));
typedef FNIEMAIMPLUNARYU16  *PFNIEMAIMPLUNARYU16;
FNIEMAIMPLUNARYU16 iemAImpl_inc_u16, iemAImpl_inc_u16_locked;
FNIEMAIMPLUNARYU16 iemAImpl_dec_u16, iemAImpl_dec_u16_locked;
FNIEMAIMPLUNARYU16 iemAImpl_not_u16, iemAImpl_not_u16_locked;
FNIEMAIMPLUNARYU16 iemAImpl_neg_u16, iemAImpl_neg_u16_locked;
/** @} */

/** @name Arithmetic assignment operations on double words (unary).
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLUNARYU32,  (uint32_t  *pu32Dst,  uint32_t *pEFlags));
typedef FNIEMAIMPLUNARYU32  *PFNIEMAIMPLUNARYU32;
FNIEMAIMPLUNARYU32 iemAImpl_inc_u32, iemAImpl_inc_u32_locked;
FNIEMAIMPLUNARYU32 iemAImpl_dec_u32, iemAImpl_dec_u32_locked;
FNIEMAIMPLUNARYU32 iemAImpl_not_u32, iemAImpl_not_u32_locked;
FNIEMAIMPLUNARYU32 iemAImpl_neg_u32, iemAImpl_neg_u32_locked;
/** @} */

/** @name Arithmetic assignment operations on quad words (unary).
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLUNARYU64,  (uint64_t  *pu64Dst,  uint32_t *pEFlags));
typedef FNIEMAIMPLUNARYU64  *PFNIEMAIMPLUNARYU64;
FNIEMAIMPLUNARYU64 iemAImpl_inc_u64, iemAImpl_inc_u64_locked;
FNIEMAIMPLUNARYU64 iemAImpl_dec_u64, iemAImpl_dec_u64_locked;
FNIEMAIMPLUNARYU64 iemAImpl_not_u64, iemAImpl_not_u64_locked;
FNIEMAIMPLUNARYU64 iemAImpl_neg_u64, iemAImpl_neg_u64_locked;
/** @} */


/** @name Shift operations on bytes (Group 2).
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLSHIFTU8,(uint8_t *pu8Dst, uint8_t cShift, uint32_t *pEFlags));
typedef FNIEMAIMPLSHIFTU8  *PFNIEMAIMPLSHIFTU8;
FNIEMAIMPLSHIFTU8 iemAImpl_rol_u8;
FNIEMAIMPLSHIFTU8 iemAImpl_ror_u8;
FNIEMAIMPLSHIFTU8 iemAImpl_rcl_u8;
FNIEMAIMPLSHIFTU8 iemAImpl_rcr_u8;
FNIEMAIMPLSHIFTU8 iemAImpl_shl_u8;
FNIEMAIMPLSHIFTU8 iemAImpl_shr_u8;
FNIEMAIMPLSHIFTU8 iemAImpl_sar_u8;
/** @} */

/** @name Shift operations on words (Group 2).
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLSHIFTU16,(uint16_t *pu16Dst, uint8_t cShift, uint32_t *pEFlags));
typedef FNIEMAIMPLSHIFTU16  *PFNIEMAIMPLSHIFTU16;
FNIEMAIMPLSHIFTU16 iemAImpl_rol_u16;
FNIEMAIMPLSHIFTU16 iemAImpl_ror_u16;
FNIEMAIMPLSHIFTU16 iemAImpl_rcl_u16;
FNIEMAIMPLSHIFTU16 iemAImpl_rcr_u16;
FNIEMAIMPLSHIFTU16 iemAImpl_shl_u16;
FNIEMAIMPLSHIFTU16 iemAImpl_shr_u16;
FNIEMAIMPLSHIFTU16 iemAImpl_sar_u16;
/** @} */

/** @name Shift operations on double words (Group 2).
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLSHIFTU32,(uint32_t *pu32Dst, uint8_t cShift, uint32_t *pEFlags));
typedef FNIEMAIMPLSHIFTU32  *PFNIEMAIMPLSHIFTU32;
FNIEMAIMPLSHIFTU32 iemAImpl_rol_u32;
FNIEMAIMPLSHIFTU32 iemAImpl_ror_u32;
FNIEMAIMPLSHIFTU32 iemAImpl_rcl_u32;
FNIEMAIMPLSHIFTU32 iemAImpl_rcr_u32;
FNIEMAIMPLSHIFTU32 iemAImpl_shl_u32;
FNIEMAIMPLSHIFTU32 iemAImpl_shr_u32;
FNIEMAIMPLSHIFTU32 iemAImpl_sar_u32;
/** @} */

/** @name Shift operations on words (Group 2).
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLSHIFTU64,(uint64_t *pu64Dst, uint8_t cShift, uint32_t *pEFlags));
typedef FNIEMAIMPLSHIFTU64  *PFNIEMAIMPLSHIFTU64;
FNIEMAIMPLSHIFTU64 iemAImpl_rol_u64;
FNIEMAIMPLSHIFTU64 iemAImpl_ror_u64;
FNIEMAIMPLSHIFTU64 iemAImpl_rcl_u64;
FNIEMAIMPLSHIFTU64 iemAImpl_rcr_u64;
FNIEMAIMPLSHIFTU64 iemAImpl_shl_u64;
FNIEMAIMPLSHIFTU64 iemAImpl_shr_u64;
FNIEMAIMPLSHIFTU64 iemAImpl_sar_u64;
/** @} */

/** @name Multiplication and division operations.
 * @{ */
typedef IEM_DECL_IMPL_TYPE(int, FNIEMAIMPLMULDIVU8,(uint16_t *pu16AX, uint8_t u8FactorDivisor, uint32_t *pEFlags));
typedef FNIEMAIMPLMULDIVU8  *PFNIEMAIMPLMULDIVU8;
FNIEMAIMPLMULDIVU8 iemAImpl_mul_u8, iemAImpl_imul_u8;
FNIEMAIMPLMULDIVU8 iemAImpl_div_u8, iemAImpl_idiv_u8;

typedef IEM_DECL_IMPL_TYPE(int, FNIEMAIMPLMULDIVU16,(uint16_t *pu16AX, uint16_t *pu16DX, uint16_t u16FactorDivisor, uint32_t *pEFlags));
typedef FNIEMAIMPLMULDIVU16  *PFNIEMAIMPLMULDIVU16;
FNIEMAIMPLMULDIVU16 iemAImpl_mul_u16, iemAImpl_imul_u16;
FNIEMAIMPLMULDIVU16 iemAImpl_div_u16, iemAImpl_idiv_u16;

typedef IEM_DECL_IMPL_TYPE(int, FNIEMAIMPLMULDIVU32,(uint32_t *pu32EAX, uint32_t *pu32EDX, uint32_t u32FactorDivisor, uint32_t *pEFlags));
typedef FNIEMAIMPLMULDIVU32  *PFNIEMAIMPLMULDIVU32;
FNIEMAIMPLMULDIVU32 iemAImpl_mul_u32, iemAImpl_imul_u32;
FNIEMAIMPLMULDIVU32 iemAImpl_div_u32, iemAImpl_idiv_u32;

typedef IEM_DECL_IMPL_TYPE(int, FNIEMAIMPLMULDIVU64,(uint64_t *pu64RAX, uint64_t *pu64RDX, uint64_t u64FactorDivisor, uint32_t *pEFlags));
typedef FNIEMAIMPLMULDIVU64  *PFNIEMAIMPLMULDIVU64;
FNIEMAIMPLMULDIVU64 iemAImpl_mul_u64, iemAImpl_imul_u64;
FNIEMAIMPLMULDIVU64 iemAImpl_div_u64, iemAImpl_idiv_u64;
/** @} */

/** @name Byte Swap.
 * @{  */
IEM_DECL_IMPL_TYPE(void, iemAImpl_bswap_u16,(uint32_t *pu32Dst)); /* Yes, 32-bit register access. */
IEM_DECL_IMPL_TYPE(void, iemAImpl_bswap_u32,(uint32_t *pu32Dst));
IEM_DECL_IMPL_TYPE(void, iemAImpl_bswap_u64,(uint64_t *pu64Dst));
/** @}  */

/** @name Misc.
 * @{ */
FNIEMAIMPLBINU16 iemAImpl_arpl;
/** @} */


/** @name FPU operations taking a 32-bit float argument
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLFPUR32FSW,(PCX86FXSTATE pFpuState, uint16_t *pFSW,
                                                      PCRTFLOAT80U pr80Val1, PCRTFLOAT32U pr32Val2));
typedef FNIEMAIMPLFPUR32FSW *PFNIEMAIMPLFPUR32FSW;

typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLFPUR32,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes,
                                                   PCRTFLOAT80U pr80Val1, PCRTFLOAT32U pr32Val2));
typedef FNIEMAIMPLFPUR32    *PFNIEMAIMPLFPUR32;

FNIEMAIMPLFPUR32FSW iemAImpl_fcom_r80_by_r32;
FNIEMAIMPLFPUR32    iemAImpl_fadd_r80_by_r32;
FNIEMAIMPLFPUR32    iemAImpl_fmul_r80_by_r32;
FNIEMAIMPLFPUR32    iemAImpl_fsub_r80_by_r32;
FNIEMAIMPLFPUR32    iemAImpl_fsubr_r80_by_r32;
FNIEMAIMPLFPUR32    iemAImpl_fdiv_r80_by_r32;
FNIEMAIMPLFPUR32    iemAImpl_fdivr_r80_by_r32;

IEM_DECL_IMPL_DEF(void, iemAImpl_fld_r32_to_r80,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes, PCRTFLOAT32U pr32Val));
IEM_DECL_IMPL_DEF(void, iemAImpl_fst_r80_to_r32,(PCX86FXSTATE pFpuState, uint16_t *pu16FSW,
                                                 PRTFLOAT32U pr32Val, PCRTFLOAT80U pr80Val));
/** @} */

/** @name FPU operations taking a 64-bit float argument
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLFPUR64,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes,
                                                   PCRTFLOAT80U pr80Val1, PCRTFLOAT64U pr64Val2));
typedef FNIEMAIMPLFPUR64   *PFNIEMAIMPLFPUR64;

FNIEMAIMPLFPUR64  iemAImpl_fadd_r80_by_r64;
FNIEMAIMPLFPUR64  iemAImpl_fmul_r80_by_r64;
FNIEMAIMPLFPUR64  iemAImpl_fsub_r80_by_r64;
FNIEMAIMPLFPUR64  iemAImpl_fsubr_r80_by_r64;
FNIEMAIMPLFPUR64  iemAImpl_fdiv_r80_by_r64;
FNIEMAIMPLFPUR64  iemAImpl_fdivr_r80_by_r64;

IEM_DECL_IMPL_DEF(void, iemAImpl_fcom_r80_by_r64,(PCX86FXSTATE pFpuState, uint16_t *pFSW,
                                                  PCRTFLOAT80U pr80Val1, PCRTFLOAT64U pr64Val2));
IEM_DECL_IMPL_DEF(void, iemAImpl_fld_r64_to_r80,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes, PCRTFLOAT64U pr64Val));
IEM_DECL_IMPL_DEF(void, iemAImpl_fst_r80_to_r64,(PCX86FXSTATE pFpuState, uint16_t *pu16FSW,
                                                 PRTFLOAT64U pr32Val, PCRTFLOAT80U pr80Val));
/** @} */

/** @name FPU operations taking a 80-bit float argument
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLFPUR80,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes,
                                                   PCRTFLOAT80U pr80Val1, PCRTFLOAT80U pr80Val2));
typedef FNIEMAIMPLFPUR80    *PFNIEMAIMPLFPUR80;
FNIEMAIMPLFPUR80            iemAImpl_fadd_r80_by_r80;
FNIEMAIMPLFPUR80            iemAImpl_fmul_r80_by_r80;
FNIEMAIMPLFPUR80            iemAImpl_fsub_r80_by_r80;
FNIEMAIMPLFPUR80            iemAImpl_fsubr_r80_by_r80;
FNIEMAIMPLFPUR80            iemAImpl_fdiv_r80_by_r80;
FNIEMAIMPLFPUR80            iemAImpl_fdivr_r80_by_r80;
FNIEMAIMPLFPUR80            iemAImpl_fprem_r80_by_r80;
FNIEMAIMPLFPUR80            iemAImpl_fprem1_r80_by_r80;
FNIEMAIMPLFPUR80            iemAImpl_fscale_r80_by_r80;

FNIEMAIMPLFPUR80            iemAImpl_fpatan_r80_by_r80;
FNIEMAIMPLFPUR80            iemAImpl_fyl2x_r80_by_r80;
FNIEMAIMPLFPUR80            iemAImpl_fyl2xp1_r80_by_r80;

typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLFPUR80FSW,(PCX86FXSTATE pFpuState, uint16_t *pFSW,
                                                      PCRTFLOAT80U pr80Val1, PCRTFLOAT80U pr80Val2));
typedef FNIEMAIMPLFPUR80FSW *PFNIEMAIMPLFPUR80FSW;
FNIEMAIMPLFPUR80FSW         iemAImpl_fcom_r80_by_r80;
FNIEMAIMPLFPUR80FSW         iemAImpl_fucom_r80_by_r80;

typedef IEM_DECL_IMPL_TYPE(uint32_t, FNIEMAIMPLFPUR80EFL,(PCX86FXSTATE pFpuState, uint16_t *pu16Fsw,
                                                          PCRTFLOAT80U pr80Val1, PCRTFLOAT80U pr80Val2));
typedef FNIEMAIMPLFPUR80EFL *PFNIEMAIMPLFPUR80EFL;
FNIEMAIMPLFPUR80EFL         iemAImpl_fcomi_r80_by_r80;
FNIEMAIMPLFPUR80EFL         iemAImpl_fucomi_r80_by_r80;

typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLFPUR80UNARY,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes, PCRTFLOAT80U pr80Val));
typedef FNIEMAIMPLFPUR80UNARY *PFNIEMAIMPLFPUR80UNARY;
FNIEMAIMPLFPUR80UNARY       iemAImpl_fabs_r80;
FNIEMAIMPLFPUR80UNARY       iemAImpl_fchs_r80;
FNIEMAIMPLFPUR80UNARY       iemAImpl_f2xm1_r80;
FNIEMAIMPLFPUR80UNARY       iemAImpl_fsqrt_r80;
FNIEMAIMPLFPUR80UNARY       iemAImpl_frndint_r80;
FNIEMAIMPLFPUR80UNARY       iemAImpl_fsin_r80;
FNIEMAIMPLFPUR80UNARY       iemAImpl_fcos_r80;

typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLFPUR80UNARYFSW,(PCX86FXSTATE pFpuState, uint16_t *pu16Fsw, PCRTFLOAT80U pr80Val));
typedef FNIEMAIMPLFPUR80UNARYFSW *PFNIEMAIMPLFPUR80UNARYFSW;
FNIEMAIMPLFPUR80UNARYFSW    iemAImpl_ftst_r80;
FNIEMAIMPLFPUR80UNARYFSW    iemAImpl_fxam_r80;

typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLFPUR80LDCONST,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes));
typedef FNIEMAIMPLFPUR80LDCONST *PFNIEMAIMPLFPUR80LDCONST;
FNIEMAIMPLFPUR80LDCONST     iemAImpl_fld1;
FNIEMAIMPLFPUR80LDCONST     iemAImpl_fldl2t;
FNIEMAIMPLFPUR80LDCONST     iemAImpl_fldl2e;
FNIEMAIMPLFPUR80LDCONST     iemAImpl_fldpi;
FNIEMAIMPLFPUR80LDCONST     iemAImpl_fldlg2;
FNIEMAIMPLFPUR80LDCONST     iemAImpl_fldln2;
FNIEMAIMPLFPUR80LDCONST     iemAImpl_fldz;

typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLFPUR80UNARYTWO,(PCX86FXSTATE pFpuState, PIEMFPURESULTTWO pFpuResTwo,
                                                           PCRTFLOAT80U pr80Val));
typedef FNIEMAIMPLFPUR80UNARYTWO *PFNIEMAIMPLFPUR80UNARYTWO;
FNIEMAIMPLFPUR80UNARYTWO    iemAImpl_fptan_r80_r80;
FNIEMAIMPLFPUR80UNARYTWO    iemAImpl_fxtract_r80_r80;
FNIEMAIMPLFPUR80UNARYTWO    iemAImpl_fsincos_r80_r80;

IEM_DECL_IMPL_DEF(void, iemAImpl_fld_r80_from_r80,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes, PCRTFLOAT80U pr80Val));
IEM_DECL_IMPL_DEF(void, iemAImpl_fst_r80_to_r80,(PCX86FXSTATE pFpuState, uint16_t *pu16FSW,
                                                 PRTFLOAT80U pr80Dst, PCRTFLOAT80U pr80Src));

/** @} */

/** @name FPU operations taking a 16-bit signed integer argument
 * @{  */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLFPUI16,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes,
                                                   PCRTFLOAT80U pr80Val1, int16_t const *pi16Val2));
typedef FNIEMAIMPLFPUI16    *PFNIEMAIMPLFPUI16;

FNIEMAIMPLFPUI16    iemAImpl_fiadd_r80_by_i16;
FNIEMAIMPLFPUI16    iemAImpl_fimul_r80_by_i16;
FNIEMAIMPLFPUI16    iemAImpl_fisub_r80_by_i16;
FNIEMAIMPLFPUI16    iemAImpl_fisubr_r80_by_i16;
FNIEMAIMPLFPUI16    iemAImpl_fidiv_r80_by_i16;
FNIEMAIMPLFPUI16    iemAImpl_fidivr_r80_by_i16;

IEM_DECL_IMPL_DEF(void, iemAImpl_ficom_r80_by_i16,(PCX86FXSTATE pFpuState, uint16_t *pu16Fsw,
                                                   PCRTFLOAT80U pr80Val1, int16_t const *pi16Val2));

IEM_DECL_IMPL_DEF(void, iemAImpl_fild_i16_to_r80,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes, int16_t const *pi16Val));
IEM_DECL_IMPL_DEF(void, iemAImpl_fist_r80_to_i16,(PCX86FXSTATE pFpuState, uint16_t *pu16FSW,
                                                  int16_t *pi16Val, PCRTFLOAT80U pr80Val));
IEM_DECL_IMPL_DEF(void, iemAImpl_fistt_r80_to_i16,(PCX86FXSTATE pFpuState, uint16_t *pu16FSW,
                                                   int16_t *pi16Val, PCRTFLOAT80U pr80Val));
/** @}  */

/** @name FPU operations taking a 32-bit signed integer argument
 * @{  */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLFPUI32,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes,
                                                   PCRTFLOAT80U pr80Val1, int32_t const *pi32Val2));
typedef FNIEMAIMPLFPUI32    *PFNIEMAIMPLFPUI32;

FNIEMAIMPLFPUI32    iemAImpl_fiadd_r80_by_i32;
FNIEMAIMPLFPUI32    iemAImpl_fimul_r80_by_i32;
FNIEMAIMPLFPUI32    iemAImpl_fisub_r80_by_i32;
FNIEMAIMPLFPUI32    iemAImpl_fisubr_r80_by_i32;
FNIEMAIMPLFPUI32    iemAImpl_fidiv_r80_by_i32;
FNIEMAIMPLFPUI32    iemAImpl_fidivr_r80_by_i32;

IEM_DECL_IMPL_DEF(void, iemAImpl_ficom_r80_by_i32,(PCX86FXSTATE pFpuState, uint16_t *pu16Fsw,
                                                   PCRTFLOAT80U pr80Val1, int32_t const *pi32Val2));

IEM_DECL_IMPL_DEF(void, iemAImpl_fild_i32_to_r80,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes, int32_t const *pi32Val));
IEM_DECL_IMPL_DEF(void, iemAImpl_fist_r80_to_i32,(PCX86FXSTATE pFpuState, uint16_t *pu16FSW,
                                                  int32_t *pi32Val, PCRTFLOAT80U pr80Val));
IEM_DECL_IMPL_DEF(void, iemAImpl_fistt_r80_to_i32,(PCX86FXSTATE pFpuState, uint16_t *pu16FSW,
                                                   int32_t *pi32Val, PCRTFLOAT80U pr80Val));
/** @}  */

/** @name FPU operations taking a 64-bit signed integer argument
 * @{  */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLFPUI64,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes,
                                                   PCRTFLOAT80U pr80Val1, int64_t const *pi64Val2));
typedef FNIEMAIMPLFPUI64    *PFNIEMAIMPLFPUI64;

FNIEMAIMPLFPUI64    iemAImpl_fiadd_r80_by_i64;
FNIEMAIMPLFPUI64    iemAImpl_fimul_r80_by_i64;
FNIEMAIMPLFPUI64    iemAImpl_fisub_r80_by_i64;
FNIEMAIMPLFPUI64    iemAImpl_fisubr_r80_by_i64;
FNIEMAIMPLFPUI64    iemAImpl_fidiv_r80_by_i64;
FNIEMAIMPLFPUI64    iemAImpl_fidivr_r80_by_i64;

IEM_DECL_IMPL_DEF(void, iemAImpl_ficom_r80_by_i64,(PCX86FXSTATE pFpuState, uint16_t *pu16Fsw,
                                                   PCRTFLOAT80U pr80Val1, int64_t const *pi64Val2));

IEM_DECL_IMPL_DEF(void, iemAImpl_fild_i64_to_r80,(PCX86FXSTATE pFpuState, PIEMFPURESULT pFpuRes, int64_t const *pi64Val));
IEM_DECL_IMPL_DEF(void, iemAImpl_fist_r80_to_i64,(PCX86FXSTATE pFpuState, uint16_t *pu16FSW,
                                                  int64_t *pi64Val, PCRTFLOAT80U pr80Val));
IEM_DECL_IMPL_DEF(void, iemAImpl_fistt_r80_to_i64,(PCX86FXSTATE pFpuState, uint16_t *pu16FSW,
                                                   int64_t *pi32Val, PCRTFLOAT80U pr80Val));
/** @} */


/** Temporary type representing a 256-bit vector register. */
typedef struct {uint64_t au64[4]; } IEMVMM256;
/** Temporary type pointing to a 256-bit vector register. */
typedef IEMVMM256 *PIEMVMM256;
/** Temporary type pointing to a const 256-bit vector register. */
typedef IEMVMM256 *PCIEMVMM256;


/** @name Media (SSE/MMX/AVX) operations: full1 + full2 -> full1.
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLMEDIAF2U64,(PCX86FXSTATE pFpuState, uint64_t *pu64Dst, uint64_t const *pu64Src));
typedef FNIEMAIMPLMEDIAF2U64   *PFNIEMAIMPLMEDIAF2U64;
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLMEDIAF2U128,(PCX86FXSTATE pFpuState, PRTUINT128U pu128Dst, PCRTUINT128U pu128Src));
typedef FNIEMAIMPLMEDIAF2U128  *PFNIEMAIMPLMEDIAF2U128;
FNIEMAIMPLMEDIAF2U64  iemAImpl_pxor_u64,  iemAImpl_pcmpeqb_u64,  iemAImpl_pcmpeqw_u64,  iemAImpl_pcmpeqd_u64;
FNIEMAIMPLMEDIAF2U128 iemAImpl_pxor_u128, iemAImpl_pcmpeqb_u128, iemAImpl_pcmpeqw_u128, iemAImpl_pcmpeqd_u128;
/** @} */

/** @name Media (SSE/MMX/AVX) operations: lowhalf1 + lowhalf1 -> full1.
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLMEDIAF1L1U64,(PCX86FXSTATE pFpuState, uint64_t *pu64Dst, uint32_t const *pu32Src));
typedef FNIEMAIMPLMEDIAF1L1U64   *PFNIEMAIMPLMEDIAF1L1U64;
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLMEDIAF1L1U128,(PCX86FXSTATE pFpuState, PRTUINT128U pu128Dst, uint64_t const *pu64Src));
typedef FNIEMAIMPLMEDIAF1L1U128  *PFNIEMAIMPLMEDIAF1L1U128;
FNIEMAIMPLMEDIAF1L1U64  iemAImpl_punpcklbw_u64,  iemAImpl_punpcklwd_u64,  iemAImpl_punpckldq_u64;
FNIEMAIMPLMEDIAF1L1U128 iemAImpl_punpcklbw_u128, iemAImpl_punpcklwd_u128, iemAImpl_punpckldq_u128, iemAImpl_punpcklqdq_u128;
/** @} */

/** @name Media (SSE/MMX/AVX) operations: hihalf1 + hihalf2 -> full1.
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLMEDIAF1H1U64,(PCX86FXSTATE pFpuState, uint64_t *pu64Dst, uint64_t const *pu64Src));
typedef FNIEMAIMPLMEDIAF2U64   *PFNIEMAIMPLMEDIAF1H1U64;
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLMEDIAF1H1U128,(PCX86FXSTATE pFpuState, PRTUINT128U pu128Dst, PCRTUINT128U pu128Src));
typedef FNIEMAIMPLMEDIAF2U128  *PFNIEMAIMPLMEDIAF1H1U128;
FNIEMAIMPLMEDIAF1H1U64  iemAImpl_punpckhbw_u64,  iemAImpl_punpckhwd_u64,  iemAImpl_punpckhdq_u64;
FNIEMAIMPLMEDIAF1H1U128 iemAImpl_punpckhbw_u128, iemAImpl_punpckhwd_u128, iemAImpl_punpckhdq_u128, iemAImpl_punpckhqdq_u128;
/** @} */

/** @name Media (SSE/MMX/AVX) operation: Packed Shuffle Stuff (evil)
 * @{ */
typedef IEM_DECL_IMPL_TYPE(void, FNIEMAIMPLMEDIAPSHUF,(PCX86FXSTATE pFpuState, PRTUINT128U pu128Dst,
                                                       PCRTUINT128U pu128Src, uint8_t bEvil));
typedef FNIEMAIMPLMEDIAPSHUF *PFNIEMAIMPLMEDIAPSHUF;
FNIEMAIMPLMEDIAPSHUF iemAImpl_pshufhw, iemAImpl_pshuflw, iemAImpl_pshufd;
IEM_DECL_IMPL_DEF(void, iemAImpl_pshufw,(PCX86FXSTATE pFpuState, uint64_t *pu64Dst, uint64_t const *pu64Src, uint8_t bEvil));
/** @} */

/** @name Media (SSE/MMX/AVX) operation: Move Byte Mask
 * @{ */
IEM_DECL_IMPL_DEF(void, iemAImpl_pmovmskb_u64,(PCX86FXSTATE pFpuState, uint64_t *pu64Dst, uint64_t const *pu64Src));
IEM_DECL_IMPL_DEF(void, iemAImpl_pmovmskb_u128,(PCX86FXSTATE pFpuState, uint64_t *pu64Dst, PCRTUINT128U pu128Src));
/** @} */

/** @name Media (SSE/MMX/AVX) operation: Sort this later
 * @{ */
IEM_DECL_IMPL_DEF(void, iemAImpl_movsldup,(PCX86FXSTATE pFpuState, PRTUINT128U puDst, PCRTUINT128U puSrc));
IEM_DECL_IMPL_DEF(void, iemAImpl_movshdup,(PCX86FXSTATE pFpuState, PRTUINT128U puDst, PCRTUINT128U puSrc));
IEM_DECL_IMPL_DEF(void, iemAImpl_movddup,(PCX86FXSTATE pFpuState, PRTUINT128U puDst, uint64_t uSrc));

IEM_DECL_IMPL_DEF(void, iemAImpl_vmovsldup_256_rr,(PX86XSAVEAREA pXState, uint8_t iYRegDst, uint8_t iYRegSrc));
IEM_DECL_IMPL_DEF(void, iemAImpl_vmovsldup_256_rm,(PX86XSAVEAREA pXState, uint8_t iYRegDst, PCRTUINT256U pSrc));
IEM_DECL_IMPL_DEF(void, iemAImpl_vmovddup_256_rr,(PX86XSAVEAREA pXState, uint8_t iYRegDst, uint8_t iYRegSrc));
IEM_DECL_IMPL_DEF(void, iemAImpl_vmovddup_256_rm,(PX86XSAVEAREA pXState, uint8_t iYRegDst, PCRTUINT256U pSrc));

/** @} */


/** @name Function tables.
 * @{
 */

/**
 * Function table for a binary operator providing implementation based on
 * operand size.
 */
typedef struct IEMOPBINSIZES
{
    PFNIEMAIMPLBINU8  pfnNormalU8,    pfnLockedU8;
    PFNIEMAIMPLBINU16 pfnNormalU16,   pfnLockedU16;
    PFNIEMAIMPLBINU32 pfnNormalU32,   pfnLockedU32;
    PFNIEMAIMPLBINU64 pfnNormalU64,   pfnLockedU64;
} IEMOPBINSIZES;
/** Pointer to a binary operator function table. */
typedef IEMOPBINSIZES const *PCIEMOPBINSIZES;


/**
 * Function table for a unary operator providing implementation based on
 * operand size.
 */
typedef struct IEMOPUNARYSIZES
{
    PFNIEMAIMPLUNARYU8  pfnNormalU8,    pfnLockedU8;
    PFNIEMAIMPLUNARYU16 pfnNormalU16,   pfnLockedU16;
    PFNIEMAIMPLUNARYU32 pfnNormalU32,   pfnLockedU32;
    PFNIEMAIMPLUNARYU64 pfnNormalU64,   pfnLockedU64;
} IEMOPUNARYSIZES;
/** Pointer to a unary operator function table. */
typedef IEMOPUNARYSIZES const *PCIEMOPUNARYSIZES;


/**
 * Function table for a shift operator providing implementation based on
 * operand size.
 */
typedef struct IEMOPSHIFTSIZES
{
    PFNIEMAIMPLSHIFTU8  pfnNormalU8;
    PFNIEMAIMPLSHIFTU16 pfnNormalU16;
    PFNIEMAIMPLSHIFTU32 pfnNormalU32;
    PFNIEMAIMPLSHIFTU64 pfnNormalU64;
} IEMOPSHIFTSIZES;
/** Pointer to a shift operator function table. */
typedef IEMOPSHIFTSIZES const *PCIEMOPSHIFTSIZES;


/**
 * Function table for a multiplication or division operation.
 */
typedef struct IEMOPMULDIVSIZES
{
    PFNIEMAIMPLMULDIVU8  pfnU8;
    PFNIEMAIMPLMULDIVU16 pfnU16;
    PFNIEMAIMPLMULDIVU32 pfnU32;
    PFNIEMAIMPLMULDIVU64 pfnU64;
} IEMOPMULDIVSIZES;
/** Pointer to a multiplication or division operation function table. */
typedef IEMOPMULDIVSIZES const *PCIEMOPMULDIVSIZES;


/**
 * Function table for a double precision shift operator providing implementation
 * based on operand size.
 */
typedef struct IEMOPSHIFTDBLSIZES
{
    PFNIEMAIMPLSHIFTDBLU16 pfnNormalU16;
    PFNIEMAIMPLSHIFTDBLU32 pfnNormalU32;
    PFNIEMAIMPLSHIFTDBLU64 pfnNormalU64;
} IEMOPSHIFTDBLSIZES;
/** Pointer to a double precision shift function table. */
typedef IEMOPSHIFTDBLSIZES const *PCIEMOPSHIFTDBLSIZES;


/**
 * Function table for media instruction taking two full sized media registers,
 * optionally the 2nd being a memory reference (only modifying the first op.)
 */
typedef struct IEMOPMEDIAF2
{
    PFNIEMAIMPLMEDIAF2U64  pfnU64;
    PFNIEMAIMPLMEDIAF2U128 pfnU128;
} IEMOPMEDIAF2;
/** Pointer to a media operation function table for full sized ops. */
typedef IEMOPMEDIAF2 const *PCIEMOPMEDIAF2;

/**
 * Function table for media instruction taking taking one full and one lower
 * half media register.
 */
typedef struct IEMOPMEDIAF1L1
{
    PFNIEMAIMPLMEDIAF1L1U64  pfnU64;
    PFNIEMAIMPLMEDIAF1L1U128 pfnU128;
} IEMOPMEDIAF1L1;
/** Pointer to a media operation function table for lowhalf+lowhalf -> full. */
typedef IEMOPMEDIAF1L1 const *PCIEMOPMEDIAF1L1;

/**
 * Function table for media instruction taking taking one full and one high half
 * media register.
 */
typedef struct IEMOPMEDIAF1H1
{
    PFNIEMAIMPLMEDIAF1H1U64  pfnU64;
    PFNIEMAIMPLMEDIAF1H1U128 pfnU128;
} IEMOPMEDIAF1H1;
/** Pointer to a media operation function table for hihalf+hihalf -> full. */
typedef IEMOPMEDIAF1H1 const *PCIEMOPMEDIAF1H1;


/** @} */


/** @name C instruction implementations for anything slightly complicated.
 * @{ */

/**
 * For typedef'ing or declaring a C instruction implementation function taking
 * no extra arguments.
 *
 * @param   a_Name              The name of the type.
 */
# define IEM_CIMPL_DECL_TYPE_0(a_Name) \
    IEM_DECL_IMPL_TYPE(VBOXSTRICTRC, a_Name, (PVMCPU pVCpu, uint8_t cbInstr))
/**
 * For defining a C instruction implementation function taking no extra
 * arguments.
 *
 * @param   a_Name              The name of the function
 */
# define IEM_CIMPL_DEF_0(a_Name) \
    IEM_DECL_IMPL_DEF(VBOXSTRICTRC, a_Name, (PVMCPU pVCpu, uint8_t cbInstr))
/**
 * For calling a C instruction implementation function taking no extra
 * arguments.
 *
 * This special call macro adds default arguments to the call and allow us to
 * change these later.
 *
 * @param   a_fn                The name of the function.
 */
# define IEM_CIMPL_CALL_0(a_fn)            a_fn(pVCpu, cbInstr)

/**
 * For typedef'ing or declaring a C instruction implementation function taking
 * one extra argument.
 *
 * @param   a_Name              The name of the type.
 * @param   a_Type0             The argument type.
 * @param   a_Arg0              The argument name.
 */
# define IEM_CIMPL_DECL_TYPE_1(a_Name, a_Type0, a_Arg0) \
    IEM_DECL_IMPL_TYPE(VBOXSTRICTRC, a_Name, (PVMCPU pVCpu, uint8_t cbInstr, a_Type0 a_Arg0))
/**
 * For defining a C instruction implementation function taking one extra
 * argument.
 *
 * @param   a_Name              The name of the function
 * @param   a_Type0             The argument type.
 * @param   a_Arg0              The argument name.
 */
# define IEM_CIMPL_DEF_1(a_Name, a_Type0, a_Arg0) \
    IEM_DECL_IMPL_DEF(VBOXSTRICTRC, a_Name, (PVMCPU pVCpu, uint8_t cbInstr, a_Type0 a_Arg0))
/**
 * For calling a C instruction implementation function taking one extra
 * argument.
 *
 * This special call macro adds default arguments to the call and allow us to
 * change these later.
 *
 * @param   a_fn                The name of the function.
 * @param   a0                  The name of the 1st argument.
 */
# define IEM_CIMPL_CALL_1(a_fn, a0)        a_fn(pVCpu, cbInstr, (a0))

/**
 * For typedef'ing or declaring a C instruction implementation function taking
 * two extra arguments.
 *
 * @param   a_Name              The name of the type.
 * @param   a_Type0             The type of the 1st argument
 * @param   a_Arg0              The name of the 1st argument.
 * @param   a_Type1             The type of the 2nd argument.
 * @param   a_Arg1              The name of the 2nd argument.
 */
# define IEM_CIMPL_DECL_TYPE_2(a_Name, a_Type0, a_Arg0, a_Type1, a_Arg1) \
    IEM_DECL_IMPL_TYPE(VBOXSTRICTRC, a_Name, (PVMCPU pVCpu, uint8_t cbInstr, a_Type0 a_Arg0, a_Type1 a_Arg1))
/**
 * For defining a C instruction implementation function taking two extra
 * arguments.
 *
 * @param   a_Name              The name of the function.
 * @param   a_Type0             The type of the 1st argument
 * @param   a_Arg0              The name of the 1st argument.
 * @param   a_Type1             The type of the 2nd argument.
 * @param   a_Arg1              The name of the 2nd argument.
 */
# define IEM_CIMPL_DEF_2(a_Name, a_Type0, a_Arg0, a_Type1, a_Arg1) \
    IEM_DECL_IMPL_DEF(VBOXSTRICTRC, a_Name, (PVMCPU pVCpu, uint8_t cbInstr, a_Type0 a_Arg0, a_Type1 a_Arg1))
/**
 * For calling a C instruction implementation function taking two extra
 * arguments.
 *
 * This special call macro adds default arguments to the call and allow us to
 * change these later.
 *
 * @param   a_fn                The name of the function.
 * @param   a0                  The name of the 1st argument.
 * @param   a1                  The name of the 2nd argument.
 */
# define IEM_CIMPL_CALL_2(a_fn, a0, a1)    a_fn(pVCpu, cbInstr, (a0), (a1))

/**
 * For typedef'ing or declaring a C instruction implementation function taking
 * three extra arguments.
 *
 * @param   a_Name              The name of the type.
 * @param   a_Type0             The type of the 1st argument
 * @param   a_Arg0              The name of the 1st argument.
 * @param   a_Type1             The type of the 2nd argument.
 * @param   a_Arg1              The name of the 2nd argument.
 * @param   a_Type2             The type of the 3rd argument.
 * @param   a_Arg2              The name of the 3rd argument.
 */
# define IEM_CIMPL_DECL_TYPE_3(a_Name, a_Type0, a_Arg0, a_Type1, a_Arg1, a_Type2, a_Arg2) \
    IEM_DECL_IMPL_TYPE(VBOXSTRICTRC, a_Name, (PVMCPU pVCpu, uint8_t cbInstr, a_Type0 a_Arg0, a_Type1 a_Arg1, a_Type2 a_Arg2))
/**
 * For defining a C instruction implementation function taking three extra
 * arguments.
 *
 * @param   a_Name              The name of the function.
 * @param   a_Type0             The type of the 1st argument
 * @param   a_Arg0              The name of the 1st argument.
 * @param   a_Type1             The type of the 2nd argument.
 * @param   a_Arg1              The name of the 2nd argument.
 * @param   a_Type2             The type of the 3rd argument.
 * @param   a_Arg2              The name of the 3rd argument.
 */
# define IEM_CIMPL_DEF_3(a_Name, a_Type0, a_Arg0, a_Type1, a_Arg1, a_Type2, a_Arg2) \
    IEM_DECL_IMPL_DEF(VBOXSTRICTRC, a_Name, (PVMCPU pVCpu, uint8_t cbInstr, a_Type0 a_Arg0, a_Type1 a_Arg1, a_Type2 a_Arg2))
/**
 * For calling a C instruction implementation function taking three extra
 * arguments.
 *
 * This special call macro adds default arguments to the call and allow us to
 * change these later.
 *
 * @param   a_fn                The name of the function.
 * @param   a0                  The name of the 1st argument.
 * @param   a1                  The name of the 2nd argument.
 * @param   a2                  The name of the 3rd argument.
 */
# define IEM_CIMPL_CALL_3(a_fn, a0, a1, a2) a_fn(pVCpu, cbInstr, (a0), (a1), (a2))


/**
 * For typedef'ing or declaring a C instruction implementation function taking
 * four extra arguments.
 *
 * @param   a_Name              The name of the type.
 * @param   a_Type0             The type of the 1st argument
 * @param   a_Arg0              The name of the 1st argument.
 * @param   a_Type1             The type of the 2nd argument.
 * @param   a_Arg1              The name of the 2nd argument.
 * @param   a_Type2             The type of the 3rd argument.
 * @param   a_Arg2              The name of the 3rd argument.
 * @param   a_Type3             The type of the 4th argument.
 * @param   a_Arg3              The name of the 4th argument.
 */
# define IEM_CIMPL_DECL_TYPE_4(a_Name, a_Type0, a_Arg0, a_Type1, a_Arg1, a_Type2, a_Arg2, a_Type3, a_Arg3) \
    IEM_DECL_IMPL_TYPE(VBOXSTRICTRC, a_Name, (PVMCPU pVCpu, uint8_t cbInstr, a_Type0 a_Arg0, a_Type1 a_Arg1, a_Type2 a_Arg2, a_Type3 a_Arg3))
/**
 * For defining a C instruction implementation function taking four extra
 * arguments.
 *
 * @param   a_Name              The name of the function.
 * @param   a_Type0             The type of the 1st argument
 * @param   a_Arg0              The name of the 1st argument.
 * @param   a_Type1             The type of the 2nd argument.
 * @param   a_Arg1              The name of the 2nd argument.
 * @param   a_Type2             The type of the 3rd argument.
 * @param   a_Arg2              The name of the 3rd argument.
 * @param   a_Type3             The type of the 4th argument.
 * @param   a_Arg3              The name of the 4th argument.
 */
# define IEM_CIMPL_DEF_4(a_Name, a_Type0, a_Arg0, a_Type1, a_Arg1, a_Type2, a_Arg2, a_Type3, a_Arg3) \
    IEM_DECL_IMPL_DEF(VBOXSTRICTRC, a_Name, (PVMCPU pVCpu, uint8_t cbInstr, a_Type0 a_Arg0, a_Type1 a_Arg1, \
                                             a_Type2 a_Arg2, a_Type3 a_Arg3))
/**
 * For calling a C instruction implementation function taking four extra
 * arguments.
 *
 * This special call macro adds default arguments to the call and allow us to
 * change these later.
 *
 * @param   a_fn                The name of the function.
 * @param   a0                  The name of the 1st argument.
 * @param   a1                  The name of the 2nd argument.
 * @param   a2                  The name of the 3rd argument.
 * @param   a3                  The name of the 4th argument.
 */
# define IEM_CIMPL_CALL_4(a_fn, a0, a1, a2, a3) a_fn(pVCpu, cbInstr, (a0), (a1), (a2), (a3))


/**
 * For typedef'ing or declaring a C instruction implementation function taking
 * five extra arguments.
 *
 * @param   a_Name              The name of the type.
 * @param   a_Type0             The type of the 1st argument
 * @param   a_Arg0              The name of the 1st argument.
 * @param   a_Type1             The type of the 2nd argument.
 * @param   a_Arg1              The name of the 2nd argument.
 * @param   a_Type2             The type of the 3rd argument.
 * @param   a_Arg2              The name of the 3rd argument.
 * @param   a_Type3             The type of the 4th argument.
 * @param   a_Arg3              The name of the 4th argument.
 * @param   a_Type4             The type of the 5th argument.
 * @param   a_Arg4              The name of the 5th argument.
 */
# define IEM_CIMPL_DECL_TYPE_5(a_Name, a_Type0, a_Arg0, a_Type1, a_Arg1, a_Type2, a_Arg2, a_Type3, a_Arg3, a_Type4, a_Arg4) \
    IEM_DECL_IMPL_TYPE(VBOXSTRICTRC, a_Name, (PVMCPU pVCpu, uint8_t cbInstr, \
                                               a_Type0 a_Arg0, a_Type1 a_Arg1, a_Type2 a_Arg2, \
                                               a_Type3 a_Arg3, a_Type4 a_Arg4))
/**
 * For defining a C instruction implementation function taking five extra
 * arguments.
 *
 * @param   a_Name              The name of the function.
 * @param   a_Type0             The type of the 1st argument
 * @param   a_Arg0              The name of the 1st argument.
 * @param   a_Type1             The type of the 2nd argument.
 * @param   a_Arg1              The name of the 2nd argument.
 * @param   a_Type2             The type of the 3rd argument.
 * @param   a_Arg2              The name of the 3rd argument.
 * @param   a_Type3             The type of the 4th argument.
 * @param   a_Arg3              The name of the 4th argument.
 * @param   a_Type4             The type of the 5th argument.
 * @param   a_Arg4              The name of the 5th argument.
 */
# define IEM_CIMPL_DEF_5(a_Name, a_Type0, a_Arg0, a_Type1, a_Arg1, a_Type2, a_Arg2, a_Type3, a_Arg3, a_Type4, a_Arg4) \
    IEM_DECL_IMPL_DEF(VBOXSTRICTRC, a_Name, (PVMCPU pVCpu, uint8_t cbInstr, \
                                             a_Type0 a_Arg0, a_Type1 a_Arg1, a_Type2 a_Arg2, \
                                             a_Type3 a_Arg3, a_Type4 a_Arg4))
/**
 * For calling a C instruction implementation function taking five extra
 * arguments.
 *
 * This special call macro adds default arguments to the call and allow us to
 * change these later.
 *
 * @param   a_fn                The name of the function.
 * @param   a0                  The name of the 1st argument.
 * @param   a1                  The name of the 2nd argument.
 * @param   a2                  The name of the 3rd argument.
 * @param   a3                  The name of the 4th argument.
 * @param   a4                  The name of the 5th argument.
 */
# define IEM_CIMPL_CALL_5(a_fn, a0, a1, a2, a3, a4) a_fn(pVCpu, cbInstr, (a0), (a1), (a2), (a3), (a4))

/** @}  */


/** @} */

RT_C_DECLS_END

#endif

