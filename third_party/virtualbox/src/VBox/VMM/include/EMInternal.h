/* $Id: EMInternal.h $ */
/** @file
 * EM - Internal header file.
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

#ifndef ___EMInternal_h
#define ___EMInternal_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/patm.h>
#include <VBox/dis.h>
#include <VBox/vmm/pdmcritsect.h>
#include <iprt/avl.h>
#include <setjmp.h>

RT_C_DECLS_BEGIN


/** @defgroup grp_em_int       Internal
 * @ingroup grp_em
 * @internal
 * @{
 */

/** The saved state version. */
#define EM_SAVED_STATE_VERSION                          5
#define EM_SAVED_STATE_VERSION_PRE_IEM                  4
#define EM_SAVED_STATE_VERSION_PRE_MWAIT                3
#define EM_SAVED_STATE_VERSION_PRE_SMP                  2


/** @name MWait state flags.
 * @{
 */
/** MWait activated. */
#define EMMWAIT_FLAG_ACTIVE             RT_BIT(0)
/** MWait will continue when an interrupt is pending even when IF=0. */
#define EMMWAIT_FLAG_BREAKIRQIF0        RT_BIT(1)
/** Monitor instruction was executed previously. */
#define EMMWAIT_FLAG_MONITOR_ACTIVE     RT_BIT(2)
/** @} */

/** EM time slice in ms; used for capping execution time. */
#define EM_TIME_SLICE                   100

/**
 * Cli node structure
 */
typedef struct CLISTAT
{
    /** The key is the cli address. */
    AVLGCPTRNODECORE        Core;
#if HC_ARCH_BITS == 32 && !defined(RT_OS_WINDOWS)
    /** Padding. */
    uint32_t                u32Padding;
#endif
    /** Occurrences. */
    STAMCOUNTER             Counter;
} CLISTAT, *PCLISTAT;
#ifdef IN_RING3
AssertCompileMemberAlignment(CLISTAT, Counter, 8);
#endif


/**
 * Excessive EM statistics.
 */
typedef struct EMSTATS
{
    /** GC: Profiling of EMInterpretInstruction(). */
    STAMPROFILE             StatRZEmulate;
    /** HC: Profiling of EMInterpretInstruction(). */
    STAMPROFILE             StatR3Emulate;

    /** @name Interpreter Instruction statistics.
     * @{
     */
    STAMCOUNTER             StatRZInterpretSucceeded;
    STAMCOUNTER             StatR3InterpretSucceeded;

    STAMCOUNTER             StatRZAnd;
    STAMCOUNTER             StatR3And;
    STAMCOUNTER             StatRZCpuId;
    STAMCOUNTER             StatR3CpuId;
    STAMCOUNTER             StatRZDec;
    STAMCOUNTER             StatR3Dec;
    STAMCOUNTER             StatRZHlt;
    STAMCOUNTER             StatR3Hlt;
    STAMCOUNTER             StatRZInc;
    STAMCOUNTER             StatR3Inc;
    STAMCOUNTER             StatRZInvlPg;
    STAMCOUNTER             StatR3InvlPg;
    STAMCOUNTER             StatRZIret;
    STAMCOUNTER             StatR3Iret;
    STAMCOUNTER             StatRZLLdt;
    STAMCOUNTER             StatR3LLdt;
    STAMCOUNTER             StatRZLIdt;
    STAMCOUNTER             StatR3LIdt;
    STAMCOUNTER             StatRZLGdt;
    STAMCOUNTER             StatR3LGdt;
    STAMCOUNTER             StatRZMov;
    STAMCOUNTER             StatR3Mov;
    STAMCOUNTER             StatRZMovCRx;
    STAMCOUNTER             StatR3MovCRx;
    STAMCOUNTER             StatRZMovDRx;
    STAMCOUNTER             StatR3MovDRx;
    STAMCOUNTER             StatRZOr;
    STAMCOUNTER             StatR3Or;
    STAMCOUNTER             StatRZPop;
    STAMCOUNTER             StatR3Pop;
    STAMCOUNTER             StatRZSti;
    STAMCOUNTER             StatR3Sti;
    STAMCOUNTER             StatRZXchg;
    STAMCOUNTER             StatR3Xchg;
    STAMCOUNTER             StatRZXor;
    STAMCOUNTER             StatR3Xor;
    STAMCOUNTER             StatRZMonitor;
    STAMCOUNTER             StatR3Monitor;
    STAMCOUNTER             StatRZMWait;
    STAMCOUNTER             StatR3MWait;
    STAMCOUNTER             StatRZAdd;
    STAMCOUNTER             StatR3Add;
    STAMCOUNTER             StatRZSub;
    STAMCOUNTER             StatR3Sub;
    STAMCOUNTER             StatRZAdc;
    STAMCOUNTER             StatR3Adc;
    STAMCOUNTER             StatRZRdtsc;
    STAMCOUNTER             StatR3Rdtsc;
    STAMCOUNTER             StatRZRdpmc;
    STAMCOUNTER             StatR3Rdpmc;
    STAMCOUNTER             StatRZBtr;
    STAMCOUNTER             StatR3Btr;
    STAMCOUNTER             StatRZBts;
    STAMCOUNTER             StatR3Bts;
    STAMCOUNTER             StatRZBtc;
    STAMCOUNTER             StatR3Btc;
    STAMCOUNTER             StatRZCmpXchg;
    STAMCOUNTER             StatR3CmpXchg;
    STAMCOUNTER             StatRZCmpXchg8b;
    STAMCOUNTER             StatR3CmpXchg8b;
    STAMCOUNTER             StatRZXAdd;
    STAMCOUNTER             StatR3XAdd;
    STAMCOUNTER             StatRZClts;
    STAMCOUNTER             StatR3Clts;
    STAMCOUNTER             StatRZStosWD;
    STAMCOUNTER             StatR3StosWD;
    STAMCOUNTER             StatR3Rdmsr;
    STAMCOUNTER             StatR3Wrmsr;
    STAMCOUNTER             StatRZRdmsr;
    STAMCOUNTER             StatRZWrmsr;
    STAMCOUNTER             StatRZWbInvd;
    STAMCOUNTER             StatR3WbInvd;
    STAMCOUNTER             StatRZLmsw;
    STAMCOUNTER             StatR3Lmsw;
    STAMCOUNTER             StatRZSmsw;
    STAMCOUNTER             StatR3Smsw;

    STAMCOUNTER             StatRZInterpretFailed;
    STAMCOUNTER             StatR3InterpretFailed;

    STAMCOUNTER             StatRZFailedAnd;
    STAMCOUNTER             StatR3FailedAnd;
    STAMCOUNTER             StatRZFailedCpuId;
    STAMCOUNTER             StatR3FailedCpuId;
    STAMCOUNTER             StatRZFailedDec;
    STAMCOUNTER             StatR3FailedDec;
    STAMCOUNTER             StatRZFailedHlt;
    STAMCOUNTER             StatR3FailedHlt;
    STAMCOUNTER             StatRZFailedInc;
    STAMCOUNTER             StatR3FailedInc;
    STAMCOUNTER             StatRZFailedInvlPg;
    STAMCOUNTER             StatR3FailedInvlPg;
    STAMCOUNTER             StatRZFailedIret;
    STAMCOUNTER             StatR3FailedIret;
    STAMCOUNTER             StatRZFailedLLdt;
    STAMCOUNTER             StatR3FailedLLdt;
    STAMCOUNTER             StatRZFailedLGdt;
    STAMCOUNTER             StatR3FailedLGdt;
    STAMCOUNTER             StatRZFailedLIdt;
    STAMCOUNTER             StatR3FailedLIdt;
    STAMCOUNTER             StatRZFailedMisc;
    STAMCOUNTER             StatR3FailedMisc;
    STAMCOUNTER             StatRZFailedMov;
    STAMCOUNTER             StatR3FailedMov;
    STAMCOUNTER             StatRZFailedMovCRx;
    STAMCOUNTER             StatR3FailedMovCRx;
    STAMCOUNTER             StatRZFailedMovDRx;
    STAMCOUNTER             StatR3FailedMovDRx;
    STAMCOUNTER             StatRZFailedOr;
    STAMCOUNTER             StatR3FailedOr;
    STAMCOUNTER             StatRZFailedPop;
    STAMCOUNTER             StatR3FailedPop;
    STAMCOUNTER             StatRZFailedSti;
    STAMCOUNTER             StatR3FailedSti;
    STAMCOUNTER             StatRZFailedXchg;
    STAMCOUNTER             StatR3FailedXchg;
    STAMCOUNTER             StatRZFailedXor;
    STAMCOUNTER             StatR3FailedXor;
    STAMCOUNTER             StatRZFailedMonitor;
    STAMCOUNTER             StatR3FailedMonitor;
    STAMCOUNTER             StatRZFailedMWait;
    STAMCOUNTER             StatR3FailedMWait;
    STAMCOUNTER             StatR3FailedRdmsr;
    STAMCOUNTER             StatR3FailedWrmsr;
    STAMCOUNTER             StatRZFailedRdmsr;
    STAMCOUNTER             StatRZFailedWrmsr;
    STAMCOUNTER             StatRZFailedLmsw;
    STAMCOUNTER             StatR3FailedLmsw;
    STAMCOUNTER             StatRZFailedSmsw;
    STAMCOUNTER             StatR3FailedSmsw;

    STAMCOUNTER             StatRZFailedAdd;
    STAMCOUNTER             StatR3FailedAdd;
    STAMCOUNTER             StatRZFailedAdc;
    STAMCOUNTER             StatR3FailedAdc;
    STAMCOUNTER             StatRZFailedBtr;
    STAMCOUNTER             StatR3FailedBtr;
    STAMCOUNTER             StatRZFailedBts;
    STAMCOUNTER             StatR3FailedBts;
    STAMCOUNTER             StatRZFailedBtc;
    STAMCOUNTER             StatR3FailedBtc;
    STAMCOUNTER             StatRZFailedCli;
    STAMCOUNTER             StatR3FailedCli;
    STAMCOUNTER             StatRZFailedCmpXchg;
    STAMCOUNTER             StatR3FailedCmpXchg;
    STAMCOUNTER             StatRZFailedCmpXchg8b;
    STAMCOUNTER             StatR3FailedCmpXchg8b;
    STAMCOUNTER             StatRZFailedXAdd;
    STAMCOUNTER             StatR3FailedXAdd;
    STAMCOUNTER             StatR3FailedMovNTPS;
    STAMCOUNTER             StatRZFailedMovNTPS;
    STAMCOUNTER             StatRZFailedStosWD;
    STAMCOUNTER             StatR3FailedStosWD;
    STAMCOUNTER             StatRZFailedSub;
    STAMCOUNTER             StatR3FailedSub;
    STAMCOUNTER             StatRZFailedWbInvd;
    STAMCOUNTER             StatR3FailedWbInvd;
    STAMCOUNTER             StatRZFailedRdtsc;
    STAMCOUNTER             StatR3FailedRdtsc;
    STAMCOUNTER             StatRZFailedRdpmc;
    STAMCOUNTER             StatR3FailedRdpmc;
    STAMCOUNTER             StatRZFailedClts;
    STAMCOUNTER             StatR3FailedClts;

    STAMCOUNTER             StatRZFailedUserMode;
    STAMCOUNTER             StatR3FailedUserMode;
    STAMCOUNTER             StatRZFailedPrefix;
    STAMCOUNTER             StatR3FailedPrefix;
    /** @} */

    /** @name Privileged Instructions Ending Up In HC.
     * @{ */
    STAMCOUNTER             StatIoRestarted;
    STAMCOUNTER             StatIoIem;
    STAMCOUNTER             StatCli;
    STAMCOUNTER             StatSti;
    STAMCOUNTER             StatInvlpg;
    STAMCOUNTER             StatHlt;
    STAMCOUNTER             StatMovReadCR[DISCREG_CR4 + 1];
    STAMCOUNTER             StatMovWriteCR[DISCREG_CR4 + 1];
    STAMCOUNTER             StatMovDRx;
    STAMCOUNTER             StatIret;
    STAMCOUNTER             StatMovLgdt;
    STAMCOUNTER             StatMovLldt;
    STAMCOUNTER             StatMovLidt;
    STAMCOUNTER             StatMisc;
    STAMCOUNTER             StatSysEnter;
    STAMCOUNTER             StatSysExit;
    STAMCOUNTER             StatSysCall;
    STAMCOUNTER             StatSysRet;
    /** @} */

} EMSTATS;
/** Pointer to the excessive EM statistics. */
typedef EMSTATS *PEMSTATS;


/**
 * Converts a EM pointer into a VM pointer.
 * @returns Pointer to the VM structure the EM is part of.
 * @param   pEM   Pointer to EM instance data.
 */
#define EM2VM(pEM)  ( (PVM)((char*)pEM - pEM->offVM) )

/**
 * EM VM Instance data.
 * Changes to this must checked against the padding of the cfgm union in VM!
 */
typedef struct EM
{
    /** Offset to the VM structure.
     * See EM2VM(). */
    RTUINT                  offVM;

    /** Whether IEM executes everything. */
    bool                    fIemExecutesAll;
    /** Whether a triple fault triggers a guru. */
    bool                    fGuruOnTripleFault;
    /** Alignment padding. */
    bool                    afPadding[6];

    /** Id of the VCPU that last executed code in the recompiler. */
    VMCPUID                 idLastRemCpu;

#ifdef VBOX_WITH_REM
    /** REM critical section.
     * This protects recompiler usage
     */
    PDMCRITSECT             CritSectREM;
#endif
} EM;
/** Pointer to EM VM instance data. */
typedef EM *PEM;


/**
 * EM VMCPU Instance data.
 */
typedef struct EMCPU
{
    /** Execution Manager State. */
    EMSTATE volatile        enmState;

    /** The state prior to the suspending of the VM. */
    EMSTATE                 enmPrevState;

    /** Force raw-mode execution.
     * This is used to prevent REM from trying to execute patch code.
     * The flag is cleared upon entering emR3RawExecute() and updated in certain return paths. */
    bool                    fForceRAW;

    uint8_t                 u8Padding[3];

    /** The number of instructions we've executed in IEM since switching to the
     *  EMSTATE_IEM_THEN_REM state. */
    uint32_t                cIemThenRemInstructions;

    /** Inhibit interrupts for this instruction. Valid only when VM_FF_INHIBIT_INTERRUPTS is set. */
    RTGCUINTPTR             GCPtrInhibitInterrupts;

#ifdef VBOX_WITH_RAW_MODE
    /** Pointer to the PATM status structure. (R3 Ptr) */
    R3PTRTYPE(PPATMGCSTATE) pPatmGCState;
#endif

    /** Pointer to the guest CPUM state. (R3 Ptr) */
    R3PTRTYPE(PCPUMCTX)     pCtx;

#if GC_ARCH_BITS == 64
    RTGCPTR                 aPadding1;
#endif

    /** Start of the current time slice in ms. */
    uint64_t                u64TimeSliceStart;
    /** Start of the current time slice in thread execution time (ms). */
    uint64_t                u64TimeSliceStartExec;
    /** Current time slice value. */
    uint64_t                u64TimeSliceExec;
    uint64_t                u64Alignment;

    /** MWait halt state. */
    struct
    {
        uint32_t            fWait;          /** Type of mwait; see EMMWAIT_FLAG_*. */
        uint32_t            u32Padding;
        RTGCPTR             uMWaitRAX;      /** MWAIT hints. */
        RTGCPTR             uMWaitRCX;      /** MWAIT extensions. */
        RTGCPTR             uMonitorRAX;    /** Monitored address. */
        RTGCPTR             uMonitorRCX;    /** Monitor extension. */
        RTGCPTR             uMonitorRDX;    /** Monitor hint. */
    } MWait;

    union
    {
        /** Padding used in the other rings.
         * This must be larger than jmp_buf on any supported platform. */
        char                achPaddingFatalLongJump[HC_ARCH_BITS == 32 ? 176 : 256];
#ifdef IN_RING3
        /** Long buffer jump for fatal VM errors.
         * It will jump to before the outer EM loop is entered. */
        jmp_buf             FatalLongJump;
#endif
    } u;

    /** For saving stack space, the disassembler state is allocated here instead of
     * on the stack. */
    DISCPUSTATE             DisState;

    /** @name Execution profiling.
     * @{ */
    STAMPROFILE             StatForcedActions;
    STAMPROFILE             StatHalted;
    STAMPROFILEADV          StatCapped;
    STAMPROFILEADV          StatHmEntry;
    STAMPROFILE             StatHmExec;
    STAMPROFILE             StatIEMEmu;
    STAMPROFILE             StatIEMThenREM;
    STAMPROFILE             StatREMEmu;
    STAMPROFILE             StatREMExec;
    STAMPROFILE             StatREMSync;
    STAMPROFILEADV          StatREMTotal;
    STAMPROFILE             StatRAWExec;
    STAMPROFILEADV          StatRAWEntry;
    STAMPROFILEADV          StatRAWTail;
    STAMPROFILEADV          StatRAWTotal;
    STAMPROFILEADV          StatTotal;
    /** @} */

    /** R3: Profiling of emR3RawExecuteIOInstruction. */
    STAMPROFILE             StatIOEmu;
    /** R3: Profiling of emR3RawPrivileged. */
    STAMPROFILE             StatPrivEmu;
    /** R3: Number of time emR3HmExecute is called. */
    STAMCOUNTER             StatHmExecuteEntry;

    /** More statistics (R3). */
    R3PTRTYPE(PEMSTATS)     pStatsR3;
    /** More statistics (R0). */
    R0PTRTYPE(PEMSTATS)     pStatsR0;
    /** More statistics (RC). */
    RCPTRTYPE(PEMSTATS)     pStatsRC;
#if HC_ARCH_BITS == 64
    RTRCPTR                 padding0;
#endif

    /** Tree for keeping track of cli occurrences (debug only). */
    R3PTRTYPE(PAVLGCPTRNODECORE) pCliStatTree;
    STAMCOUNTER             StatTotalClis;
#if 0
    /** 64-bit Visual C++ rounds the struct size up to 16 byte. */
    uint64_t                padding1;
#endif
} EMCPU;
/** Pointer to EM VM instance data. */
typedef EMCPU *PEMCPU;

/** @} */

int     emR3InitDbg(PVM pVM);

int     emR3HmExecute(PVM pVM, PVMCPU pVCpu, bool *pfFFDone);
int     emR3RawExecute(PVM pVM, PVMCPU pVCpu, bool *pfFFDone);
int     emR3RawHandleRC(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, int rc);
int     emR3HmHandleRC(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, int rc);
EMSTATE emR3Reschedule(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx);
int     emR3ForcedActions(PVM pVM, PVMCPU pVCpu, int rc);
int     emR3HighPriorityPostForcedActions(PVM pVM, PVMCPU pVCpu, int rc);
int     emR3RawUpdateForceFlag(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, int rc);
int     emR3RawResumeHyper(PVM pVM, PVMCPU pVCpu);
int     emR3RawStep(PVM pVM, PVMCPU pVCpu);
int     emR3SingleStepExecRem(PVM pVM, PVMCPU pVCpu, uint32_t cIterations);
bool    emR3IsExecutionAllowed(PVM pVM, PVMCPU pVCpu);

RT_C_DECLS_END

#endif

