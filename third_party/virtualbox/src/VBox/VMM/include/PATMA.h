/* $Id: PATMA.h $ */
/** @file
 * PATM macros & definitions (identical to PATMA.mac!).
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

#ifndef ___PATMA_H
#define ___PATMA_H

/** @name Patch Fixup Types
 * @remarks These fixups types are part of the saved state.
 * @{ */
#define PATM_ASMFIX_VMFLAGS                     0xF1ABCD00
#ifdef VBOX_WITH_STATISTICS
# define PATM_ASMFIX_ALLPATCHCALLS              0xF1ABCD01
# define PATM_ASMFIX_PERPATCHCALLS              0xF1ABCD02
#endif
#define PATM_ASMFIX_JUMPDELTA                   0xF1ABCD03
#ifdef VBOX_WITH_STATISTICS
# define PATM_ASMFIX_IRETEFLAGS                 0xF1ABCD04
# define PATM_ASMFIX_IRETCS                     0xF1ABCD05
# define PATM_ASMFIX_IRETEIP                    0xF1ABCD06
#endif
#define PATM_ASMFIX_FIXUP                       0xF1ABCD07
#define PATM_ASMFIX_PENDINGACTION               0xF1ABCD08
#define PATM_ASMFIX_CPUID_STD_PTR               0xF1ABCD09  /**< Legacy, saved state only. */
#define PATM_ASMFIX_CPUID_EXT_PTR               0xF1ABCD0a  /**< Legacy, saved state only. */
#define PATM_ASMFIX_CPUID_DEF_PTR               0xF1ABCD0b  /**< Legacy, saved state only. */
#define PATM_ASMFIX_STACKBASE                   0xF1ABCD0c  /**< Stack to store our private patch return addresses */
#define PATM_ASMFIX_STACKBASE_GUEST             0xF1ABCD0d  /**< Stack to store guest return addresses */
#define PATM_ASMFIX_STACKPTR                    0xF1ABCD0e
#define PATM_ASMFIX_PATCHBASE                   0xF1ABCD0f
#define PATM_ASMFIX_INTERRUPTFLAG               0xF1ABCD10
#define PATM_ASMFIX_INHIBITIRQADDR              0xF1ABCD11
#define PATM_ASMFIX_VM_FORCEDACTIONS            0xF1ABCD12
#define PATM_ASMFIX_TEMP_EAX                    0xF1ABCD13  /**< Location for original EAX register */
#define PATM_ASMFIX_TEMP_ECX                    0xF1ABCD14  /**< Location for original ECX register */
#define PATM_ASMFIX_TEMP_EDI                    0xF1ABCD15  /**< Location for original EDI register */
#define PATM_ASMFIX_TEMP_EFLAGS                 0xF1ABCD16  /**< Location for original eflags */
#define PATM_ASMFIX_TEMP_RESTORE_FLAGS          0xF1ABCD17  /**< Which registers to restore */
#define PATM_ASMFIX_CALL_PATCH_TARGET_ADDR      0xF1ABCD18
#define PATM_ASMFIX_CALL_RETURN_ADDR            0xF1ABCD19
#define PATM_ASMFIX_CPUID_CENTAUR_PTR           0xF1ABCD1a  /**< Legacy, saved state only. */
#define PATM_ASMFIX_REUSE_LATER_0               0xF1ABCD1b
#define PATM_ASMFIX_REUSE_LATER_1               0xF1ABCD1c
#define PATM_ASMFIX_REUSE_LATER_2               0xF1ABCD1d
#define PATM_ASMFIX_REUSE_LATER_3               0xF1ABCD1e
#define PATM_ASMFIX_HELPER_CPUM_CPUID           0xF1ABCD1f

/* Anything larger doesn't require a fixup */
#define PATM_ASMFIX_NO_FIXUP                    0xF1ABCE00
#define PATM_ASMFIX_CPUID_STD_MAX               0xF1ABCE00
#define PATM_ASMFIX_CPUID_EXT_MAX               0xF1ABCE01
#define PATM_ASMFIX_RETURNADDR                  0xF1ABCE02
#define PATM_ASMFIX_PATCHNEXTBLOCK              0xF1ABCE03
#define PATM_ASMFIX_CALLTARGET                  0xF1ABCE04  /**< relative call target */
#define PATM_ASMFIX_NEXTINSTRADDR               0xF1ABCE05  /**< absolute guest address of the next instruction */
#define PATM_ASMFIX_CURINSTRADDR                0xF1ABCE06  /**< absolute guest address of the current instruction */
#define PATM_ASMFIX_LOOKUP_AND_CALL_FUNCTION    0xF1ABCE07  /**< Relative address of global PATM lookup and call function. */
#define PATM_ASMFIX_RETURN_FUNCTION             0xF1ABCE08  /**< Relative address of global PATM return function. */
#define PATM_ASMFIX_LOOKUP_AND_JUMP_FUNCTION    0xF1ABCE09  /**< Relative address of global PATM lookup and jump function. */
#define PATM_ASMFIX_IRET_FUNCTION               0xF1ABCE0A  /**< Relative address of global PATM iret function. */
#define PATM_ASMFIX_CPUID_CENTAUR_MAX           0xF1ABCE0B

/** Identifies an patch fixup type value (with reasonable accuracy). */
#define PATM_IS_ASMFIX(a_uValue) \
    ( ((a_uValue) & UINT32_C(0xfffffC00)) == UINT32_C(0xF1ABCC00) && ((a_uValue) & UINT32_C(0xff)) < UINT32_C(0x30) )
/** @} */


/** Everything except IOPL, NT, IF, VM, VIF, VIP and RF */
#define PATM_FLAGS_MASK                         (  X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF \
                                                 | X86_EFL_TF | X86_EFL_DF | X86_EFL_OF | X86_EFL_AC | X86_EFL_ID)

/** Flags that PATM virtualizes. Currently only IF & IOPL. */
#define PATM_VIRTUAL_FLAGS_MASK                 (X86_EFL_IF | X86_EFL_IOPL)

/* PATM stack size (identical in PATMA.mac!!) */
#define PATM_STACK_SIZE                         (4096)
#define PATM_STACK_TOTAL_SIZE                   (2 * PATM_STACK_SIZE)
#define PATM_MAX_STACK                          (PATM_STACK_SIZE/sizeof(RTRCPTR))

/** @name Patch Manager pending actions (in GCSTATE).
 * @{  */
#define PATM_ACTION_LOOKUP_ADDRESS              1
#define PATM_ACTION_DISPATCH_PENDING_IRQ        2
#define PATM_ACTION_PENDING_IRQ_AFTER_IRET      3
#define PATM_ACTION_DO_V86_IRET                 4
#define PATM_ACTION_LOG_IF1                     5
#define PATM_ACTION_LOG_CLI                     6
#define PATM_ACTION_LOG_STI                     7
#define PATM_ACTION_LOG_POPF_IF1                8
#define PATM_ACTION_LOG_POPF_IF0                9
#define PATM_ACTION_LOG_PUSHF                   10
#define PATM_ACTION_LOG_IRET                    11
#define PATM_ACTION_LOG_RET                     12
#define PATM_ACTION_LOG_CALL                    13
#define PATM_ACTION_LOG_GATE_ENTRY              14
/** @} */

/** Magic dword found in ecx for patm pending actions. */
#define PATM_ACTION_MAGIC                       0xABCD4321

/** @name PATM_ASMFIX_TEMP_RESTORE_FLAGS
 * @{ */
#define PATM_RESTORE_EAX                        RT_BIT(0)
#define PATM_RESTORE_ECX                        RT_BIT(1)
#define PATM_RESTORE_EDI                        RT_BIT(2)
/** @} */

/** Relocation entry for PATCHASMRECORD. */
typedef struct PATCHASMRELOC
{
    /** The relocation type. */
    uint32_t uType;
    /** Additional information specific to the relocation type. */
    uint32_t uInfo;
} PATCHASMRELOC;
typedef PATCHASMRELOC const *PCPATCHASMRELOC;

/**
 * Assembly patch descriptor record.
 */
typedef struct
{
    /** Pointer to the patch code. */
    uint8_t        *pbFunction;
    /** Offset of the jump table? */
    uint32_t        offJump;
    /** Used only by loop/loopz/loopnz. */
    uint32_t        offRelJump;
    /** Size override byte position. */
    uint32_t        offSizeOverride;
    /** The size of the patch function. */
    uint32_t        cbFunction;
    /** The number of relocations in aRelocs. */
    uint32_t        cRelocs;
    /** Variable sized relocation table. */
    PATCHASMRELOC   aRelocs[1];
} PATCHASMRECORD;
/** Pointer to a const patch descriptor record. */
typedef PATCHASMRECORD const *PCPATCHASMRECORD;


/* For indirect calls/jump (identical in PATMA.h & PATMA.mac!) */
/** @note MUST BE A POWER OF TWO! */
/** @note direct calls have only one lookup slot (PATCHDIRECTJUMPTABLE_SIZE) */
/** @note Some statistics reveal that:
 *   - call: Windows XP boot  -> max 16, 127 replacements
 *   - call: Knoppix 3.7 boot -> max 9
 *   - ret:  Knoppix 5.0.1 boot -> max 16, 80000 replacements (3+ million hits)
 */
#define PATM_MAX_JUMPTABLE_ENTRIES        16
typedef struct
{
    uint16_t     nrSlots;
    uint16_t     ulInsertPos;
    uint32_t     cAddresses;
    struct
    {
        RTRCPTR      pInstrGC;
        RTRCUINTPTR  pRelPatchGC; /* relative to patch base */
    } Slot[1];
} PATCHJUMPTABLE, *PPATCHJUMPTABLE;


RT_C_DECLS_BEGIN

/** @name Patch Descriptor Records (in PATMA.asm)
 * @{ */
extern PATCHASMRECORD g_patmCliRecord;
extern PATCHASMRECORD g_patmStiRecord;
extern PATCHASMRECORD g_patmPopf32Record;
extern PATCHASMRECORD g_patmPopf16Record;
extern PATCHASMRECORD g_patmPopf16Record_NoExit;
extern PATCHASMRECORD g_patmPopf32Record_NoExit;
extern PATCHASMRECORD g_patmPushf32Record;
extern PATCHASMRECORD g_patmPushf16Record;
extern PATCHASMRECORD g_patmIretRecord;
extern PATCHASMRECORD g_patmIretRing1Record;
extern PATCHASMRECORD g_patmCpuidRecord;
extern PATCHASMRECORD g_patmLoopRecord;
extern PATCHASMRECORD g_patmLoopZRecord;
extern PATCHASMRECORD g_patmLoopNZRecord;
extern PATCHASMRECORD g_patmJEcxRecord;
extern PATCHASMRECORD g_patmIntEntryRecord;
extern PATCHASMRECORD g_patmIntEntryRecordErrorCode;
extern PATCHASMRECORD g_patmTrapEntryRecord;
extern PATCHASMRECORD g_patmTrapEntryRecordErrorCode;
extern PATCHASMRECORD g_patmPushCSRecord;

extern PATCHASMRECORD g_patmCheckIFRecord;
extern PATCHASMRECORD PATMJumpToGuest_IF1Record;

extern PATCHASMRECORD g_patmCallRecord;
extern PATCHASMRECORD g_patmCallIndirectRecord;
extern PATCHASMRECORD g_patmRetRecord;
extern PATCHASMRECORD g_patmJumpIndirectRecord;

extern PATCHASMRECORD g_patmLookupAndCallRecord;
extern PATCHASMRECORD g_patmRetFunctionRecord;
extern PATCHASMRECORD g_patmLookupAndJumpRecord;
extern PATCHASMRECORD g_patmIretFunctionRecord;

extern PATCHASMRECORD g_patmStatsRecord;

extern PATCHASMRECORD g_patmSetPIFRecord;
extern PATCHASMRECORD g_patmClearPIFRecord;

extern PATCHASMRECORD g_patmSetInhibitIRQRecord;
extern PATCHASMRECORD g_patmClearInhibitIRQFaultIF0Record;
extern PATCHASMRECORD g_patmClearInhibitIRQContIF0Record;

extern PATCHASMRECORD g_patmMovFromSSRecord;
/** @} */

extern const uint32_t g_fPatmInterruptFlag;

RT_C_DECLS_END

#endif
