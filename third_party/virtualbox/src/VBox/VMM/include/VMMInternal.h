/* $Id: VMMInternal.h $ */
/** @file
 * VMM - Internal header file.
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

#ifndef ___VMMInternal_h
#define ___VMMInternal_h

#include <VBox/cdefs.h>
#include <VBox/sup.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/vmm.h>
#include <VBox/log.h>
#include <iprt/critsect.h>

#if !defined(IN_VMM_R3) && !defined(IN_VMM_R0) && !defined(IN_VMM_RC)
# error "Not in VMM! This is an internal header!"
#endif
#if defined(RT_OS_DARWIN) && HC_ARCH_BITS == 32
# error "32-bit darwin is no longer supported. Go back to 4.3 or earlier!"
#endif



/** @defgroup grp_vmm_int   Internals
 * @ingroup grp_vmm
 * @internal
 * @{
 */

/** @def VBOX_WITH_RC_RELEASE_LOGGING
 * Enables RC release logging. */
#define VBOX_WITH_RC_RELEASE_LOGGING

/** @def VBOX_WITH_R0_LOGGING
 * Enables Ring-0 logging (non-release).
 *
 * Ring-0 logging isn't 100% safe yet (thread id reuse / process exit cleanup),
 * so you have to sign up here by adding your defined(DEBUG_<userid>) to the
 * \#if, or by adding VBOX_WITH_R0_LOGGING to your LocalConfig.kmk.
 */
#if defined(DEBUG_sandervl) || defined(DEBUG_frank) || defined(DEBUG_ramshankar) || defined(DOXYGEN_RUNNING)
# define VBOX_WITH_R0_LOGGING
#endif

/** @def VBOX_STRICT_VMM_STACK
 * Enables VMM stack guard pages to catch stack over- and underruns. */
#if defined(VBOX_STRICT) || defined(DOXYGEN_RUNNING)
# define VBOX_STRICT_VMM_STACK
#endif


/**
 * Converts a VMM pointer into a VM pointer.
 * @returns Pointer to the VM structure the VMM is part of.
 * @param   pVMM   Pointer to VMM instance data.
 */
#define VMM2VM(pVMM)  ( (PVM)((char*)pVMM - pVMM->offVM) )


/**
 * Switcher function, HC to RC.
 *
 * @param   pVM         The cross context VM structure.
 * @returns Return code indicating the action to take.
 */
typedef DECLASMTYPE(int) FNVMMSWITCHERHC(PVM pVM);
/** Pointer to switcher function. */
typedef FNVMMSWITCHERHC *PFNVMMSWITCHERHC;

/**
 * Switcher function, RC to HC.
 *
 * @param   rc          VBox status code.
 */
typedef DECLASMTYPE(void) FNVMMSWITCHERRC(int rc);
/** Pointer to switcher function. */
typedef FNVMMSWITCHERRC *PFNVMMSWITCHERRC;


/**
 * The ring-0 logger instance wrapper.
 *
 * We need to be able to find the VM handle from the logger instance, so we wrap
 * it in this structure.
 */
typedef struct VMMR0LOGGER
{
    /** Pointer to Pointer to the VM. */
    R0PTRTYPE(PVM)              pVM;
    /** Size of the allocated logger instance (Logger). */
    uint32_t                    cbLogger;
    /** Flag indicating whether we've create the logger Ring-0 instance yet. */
    bool                        fCreated;
    /** Flag indicating whether we've disabled flushing (world switch) or not. */
    bool                        fFlushingDisabled;
    /** Flag indicating whether we've registered the instance already. */
    bool                        fRegistered;
    bool                        a8Alignment;
    /** The CPU ID. */
    VMCPUID                     idCpu;
#if HC_ARCH_BITS == 64
    uint32_t                    u32Alignment;
#endif
    /** The ring-0 logger instance. This extends beyond the size.  */
    RTLOGGER                    Logger;
} VMMR0LOGGER;
/** Pointer to a ring-0 logger instance wrapper. */
typedef VMMR0LOGGER *PVMMR0LOGGER;


/**
 * Jump buffer for the setjmp/longjmp like constructs used to
 * quickly 'call' back into Ring-3.
 */
typedef struct VMMR0JMPBUF
{
    /** Traditional jmp_buf stuff
     * @{ */
#if HC_ARCH_BITS == 32
    uint32_t                    ebx;
    uint32_t                    esi;
    uint32_t                    edi;
    uint32_t                    ebp;
    uint32_t                    esp;
    uint32_t                    eip;
    uint32_t                    eflags;
#endif
#if HC_ARCH_BITS == 64
    uint64_t                    rbx;
# ifdef RT_OS_WINDOWS
    uint64_t                    rsi;
    uint64_t                    rdi;
# endif
    uint64_t                    rbp;
    uint64_t                    r12;
    uint64_t                    r13;
    uint64_t                    r14;
    uint64_t                    r15;
    uint64_t                    rsp;
    uint64_t                    rip;
# ifdef RT_OS_WINDOWS
    uint128_t                   xmm6;
    uint128_t                   xmm7;
    uint128_t                   xmm8;
    uint128_t                   xmm9;
    uint128_t                   xmm10;
    uint128_t                   xmm11;
    uint128_t                   xmm12;
    uint128_t                   xmm13;
    uint128_t                   xmm14;
    uint128_t                   xmm15;
# endif
    uint64_t                    rflags;
#endif
    /** @} */

    /** Flag that indicates that we've done a ring-3 call. */
    bool                        fInRing3Call;
    /** The number of bytes we've saved. */
    uint32_t                    cbSavedStack;
    /** Pointer to the buffer used to save the stack.
     * This is assumed to be 8KB. */
    RTR0PTR                     pvSavedStack;
    /** Esp we we match against esp on resume to make sure the stack wasn't relocated. */
    RTHCUINTREG                 SpCheck;
    /** The esp we should resume execution with after the restore. */
    RTHCUINTREG                 SpResume;
    /** ESP/RSP at the time of the jump to ring 3. */
    RTHCUINTREG                 SavedEsp;
    /** EBP/RBP at the time of the jump to ring 3. */
    RTHCUINTREG                 SavedEbp;

    /** Stats: Max amount of stack used. */
    uint32_t                    cbUsedMax;
    /** Stats: Average stack usage. (Avg = cbUsedTotal / cUsedTotal) */
    uint32_t                    cbUsedAvg;
    /** Stats: Total amount of stack used. */
    uint64_t                    cbUsedTotal;
    /** Stats: Number of stack usages. */
    uint64_t                    cUsedTotal;
} VMMR0JMPBUF;
/** Pointer to a ring-0 jump buffer. */
typedef VMMR0JMPBUF *PVMMR0JMPBUF;


/**
 * VMM Data (part of VM)
 */
typedef struct VMM
{
    /** Offset to the VM structure.
     * See VMM2VM(). */
    RTINT                       offVM;

    /** @name World Switcher and Related
     * @{
     */
    /** Size of the core code. */
    RTUINT                      cbCoreCode;
    /** Physical address of core code. */
    RTHCPHYS                    HCPhysCoreCode;
    /** Pointer to core code ring-3 mapping - contiguous memory.
     * At present this only means the context switcher code. */
    RTR3PTR                     pvCoreCodeR3;
    /** Pointer to core code ring-0 mapping - contiguous memory.
     * At present this only means the context switcher code. */
    RTR0PTR                     pvCoreCodeR0;
    /** Pointer to core code guest context mapping. */
    RTRCPTR                     pvCoreCodeRC;
    RTRCPTR                     pRCPadding0; /**< Alignment padding. */
#ifdef VBOX_WITH_NMI
    /** The guest context address of the APIC (host) mapping. */
    RTRCPTR                     GCPtrApicBase;
    RTRCPTR                     pRCPadding1; /**< Alignment padding. */
#endif
    /** The current switcher.
     * This will be set before the VMM is fully initialized. */
    VMMSWITCHER                 enmSwitcher;
    /** Array of offsets to the different switchers within the core code. */
    uint32_t                    aoffSwitchers[VMMSWITCHER_MAX];
    uint32_t                    u32Padding2; /**< Alignment padding. */

    /** Resume Guest Execution. See CPUMGCResumeGuest(). */
    RTRCPTR                     pfnCPUMRCResumeGuest;
    /** Resume Guest Execution in V86 mode. See CPUMGCResumeGuestV86(). */
    RTRCPTR                     pfnCPUMRCResumeGuestV86;
    /** Call Trampoline. See vmmGCCallTrampoline(). */
    RTRCPTR                     pfnCallTrampolineRC;
    /** Guest to host switcher entry point. */
    RCPTRTYPE(PFNVMMSWITCHERRC) pfnRCToHost;
    /** Host to guest switcher entry point. */
    R0PTRTYPE(PFNVMMSWITCHERHC) pfnR0ToRawMode;
    /** @}  */

    /** @name Logging
     * @{
     */
    /** Size of the allocated logger instance (pRCLoggerRC/pRCLoggerR3). */
    uint32_t                    cbRCLogger;
    /** Pointer to the RC logger instance - RC Ptr.
     * This is NULL if logging is disabled. */
    RCPTRTYPE(PRTLOGGERRC)      pRCLoggerRC;
    /** Pointer to the GC logger instance - R3 Ptr.
     * This is NULL if logging is disabled. */
    R3PTRTYPE(PRTLOGGERRC)      pRCLoggerR3;
    /** Pointer to the GC release logger instance - R3 Ptr. */
    R3PTRTYPE(PRTLOGGERRC)      pRCRelLoggerR3;
    /** Pointer to the GC release logger instance - RC Ptr. */
    RCPTRTYPE(PRTLOGGERRC)      pRCRelLoggerRC;
    /** Size of the allocated release logger instance (pRCRelLoggerRC/pRCRelLoggerR3).
     * This may differ from cbRCLogger. */
    uint32_t                    cbRCRelLogger;
    /** Whether log flushing has been disabled or not. */
    bool                        fRCLoggerFlushingDisabled;
    bool                        afAlignment1[5]; /**< Alignment padding. */
    /** @} */

    /** Whether the stack guard pages have been stationed or not. */
    bool                        fStackGuardsStationed;
    /** Whether we should use the periodic preemption timers. */
    bool                        fUsePeriodicPreemptionTimers;

    /** The EMT yield timer. */
    PTMTIMERR3                  pYieldTimer;
    /** The period to the next timeout when suspended or stopped.
     * This is 0 when running. */
    uint32_t                    cYieldResumeMillies;
    /** The EMT yield timer interval (milliseconds). */
    uint32_t                    cYieldEveryMillies;
    /** The timestamp of the previous yield. (nano) */
    uint64_t                    u64LastYield;

    /** @name EMT Rendezvous
     * @{ */
    /** Semaphore to wait on upon entering ordered execution. */
    R3PTRTYPE(PRTSEMEVENT)      pahEvtRendezvousEnterOrdered;
    /** Semaphore to wait on upon entering for one-by-one execution. */
    RTSEMEVENT                  hEvtRendezvousEnterOneByOne;
    /** Semaphore to wait on upon entering for all-at-once execution. */
    RTSEMEVENTMULTI             hEvtMulRendezvousEnterAllAtOnce;
    /** Semaphore to wait on when done. */
    RTSEMEVENTMULTI             hEvtMulRendezvousDone;
    /** Semaphore the VMMR3EmtRendezvous caller waits on at the end. */
    RTSEMEVENT                  hEvtRendezvousDoneCaller;
    /** Semaphore to wait on upon recursing. */
    RTSEMEVENTMULTI             hEvtMulRendezvousRecursionPush;
    /** Semaphore to wait on after done with recursion (caller restoring state). */
    RTSEMEVENTMULTI             hEvtMulRendezvousRecursionPop;
    /** Semaphore the initiator waits on while the EMTs are getting into position
     *  on hEvtMulRendezvousRecursionPush. */
    RTSEMEVENT                  hEvtRendezvousRecursionPushCaller;
    /** Semaphore the initiator waits on while the EMTs sitting on
     *  hEvtMulRendezvousRecursionPop wakes up and leave. */
    RTSEMEVENT                  hEvtRendezvousRecursionPopCaller;
    /** Callback. */
    R3PTRTYPE(PFNVMMEMTRENDEZVOUS) volatile pfnRendezvous;
    /** The user argument for the callback. */
    RTR3PTR volatile            pvRendezvousUser;
    /** Flags. */
    volatile uint32_t           fRendezvousFlags;
    /** The number of EMTs that has entered. */
    volatile uint32_t           cRendezvousEmtsEntered;
    /** The number of EMTs that has done their job. */
    volatile uint32_t           cRendezvousEmtsDone;
    /** The number of EMTs that has returned. */
    volatile uint32_t           cRendezvousEmtsReturned;
    /** The status code. */
    volatile int32_t            i32RendezvousStatus;
    /** Spin lock. */
    volatile uint32_t           u32RendezvousLock;
    /** The recursion depth. */
    volatile uint32_t           cRendezvousRecursions;
    /** The number of EMTs that have entered the recursion routine. */
    volatile uint32_t           cRendezvousEmtsRecursingPush;
    /** The number of EMTs that have leaft the recursion routine. */
    volatile uint32_t           cRendezvousEmtsRecursingPop;
    /** Triggers rendezvous recursion in the other threads. */
    volatile bool               fRendezvousRecursion;

    /** @} */
    bool                        afAlignment2[HC_ARCH_BITS == 32 ? 7 : 3]; /**< Alignment padding. */

    /** Buffer for storing the standard assertion message for a ring-0 assertion.
     * Used for saving the assertion message text for the release log and guru
     * meditation dump. */
    char                        szRing0AssertMsg1[512];
    /** Buffer for storing the custom message for a ring-0 assertion. */
    char                        szRing0AssertMsg2[256];

    /** Number of VMMR0_DO_RUN_GC calls. */
    STAMCOUNTER                 StatRunRC;

    /** Statistics for each of the RC/R0 return codes.
     * @{ */
    STAMCOUNTER                 StatRZRetNormal;
    STAMCOUNTER                 StatRZRetInterrupt;
    STAMCOUNTER                 StatRZRetInterruptHyper;
    STAMCOUNTER                 StatRZRetGuestTrap;
    STAMCOUNTER                 StatRZRetRingSwitch;
    STAMCOUNTER                 StatRZRetRingSwitchInt;
    STAMCOUNTER                 StatRZRetStaleSelector;
    STAMCOUNTER                 StatRZRetIRETTrap;
    STAMCOUNTER                 StatRZRetEmulate;
    STAMCOUNTER                 StatRZRetIOBlockEmulate;
    STAMCOUNTER                 StatRZRetPatchEmulate;
    STAMCOUNTER                 StatRZRetIORead;
    STAMCOUNTER                 StatRZRetIOWrite;
    STAMCOUNTER                 StatRZRetIOCommitWrite;
    STAMCOUNTER                 StatRZRetMMIORead;
    STAMCOUNTER                 StatRZRetMMIOWrite;
    STAMCOUNTER                 StatRZRetMMIOCommitWrite;
    STAMCOUNTER                 StatRZRetMMIOPatchRead;
    STAMCOUNTER                 StatRZRetMMIOPatchWrite;
    STAMCOUNTER                 StatRZRetMMIOReadWrite;
    STAMCOUNTER                 StatRZRetMSRRead;
    STAMCOUNTER                 StatRZRetMSRWrite;
    STAMCOUNTER                 StatRZRetLDTFault;
    STAMCOUNTER                 StatRZRetGDTFault;
    STAMCOUNTER                 StatRZRetIDTFault;
    STAMCOUNTER                 StatRZRetTSSFault;
    STAMCOUNTER                 StatRZRetCSAMTask;
    STAMCOUNTER                 StatRZRetSyncCR3;
    STAMCOUNTER                 StatRZRetMisc;
    STAMCOUNTER                 StatRZRetPatchInt3;
    STAMCOUNTER                 StatRZRetPatchPF;
    STAMCOUNTER                 StatRZRetPatchGP;
    STAMCOUNTER                 StatRZRetPatchIretIRQ;
    STAMCOUNTER                 StatRZRetRescheduleREM;
    STAMCOUNTER                 StatRZRetToR3Total;
    STAMCOUNTER                 StatRZRetToR3FF;
    STAMCOUNTER                 StatRZRetToR3Unknown;
    STAMCOUNTER                 StatRZRetToR3TMVirt;
    STAMCOUNTER                 StatRZRetToR3HandyPages;
    STAMCOUNTER                 StatRZRetToR3PDMQueues;
    STAMCOUNTER                 StatRZRetToR3Rendezvous;
    STAMCOUNTER                 StatRZRetToR3Timer;
    STAMCOUNTER                 StatRZRetToR3DMA;
    STAMCOUNTER                 StatRZRetToR3CritSect;
    STAMCOUNTER                 StatRZRetToR3Iem;
    STAMCOUNTER                 StatRZRetToR3Iom;
    STAMCOUNTER                 StatRZRetTimerPending;
    STAMCOUNTER                 StatRZRetInterruptPending;
    STAMCOUNTER                 StatRZRetCallRing3;
    STAMCOUNTER                 StatRZRetPATMDuplicateFn;
    STAMCOUNTER                 StatRZRetPGMChangeMode;
    STAMCOUNTER                 StatRZRetPendingRequest;
    STAMCOUNTER                 StatRZRetPGMFlushPending;
    STAMCOUNTER                 StatRZRetPatchTPR;
    STAMCOUNTER                 StatRZCallPDMCritSectEnter;
    STAMCOUNTER                 StatRZCallPDMLock;
    STAMCOUNTER                 StatRZCallLogFlush;
    STAMCOUNTER                 StatRZCallPGMPoolGrow;
    STAMCOUNTER                 StatRZCallPGMMapChunk;
    STAMCOUNTER                 StatRZCallPGMAllocHandy;
    STAMCOUNTER                 StatRZCallRemReplay;
    STAMCOUNTER                 StatRZCallVMSetError;
    STAMCOUNTER                 StatRZCallVMSetRuntimeError;
    STAMCOUNTER                 StatRZCallPGMLock;
    /** @} */
} VMM;
/** Pointer to VMM. */
typedef VMM *PVMM;


/**
 * VMMCPU Data (part of VMCPU)
 */
typedef struct VMMCPU
{
    /** Offset to the VMCPU structure.
     * See VMM2VMCPU(). */
    int32_t                     offVMCPU;

    /** The last RC/R0 return code. */
    int32_t                     iLastGZRc;

    /** VMM stack, pointer to the top of the stack in R3.
     * Stack is allocated from the hypervisor heap and is page aligned
     * and always writable in RC. */
    R3PTRTYPE(uint8_t *)        pbEMTStackR3;
    /** Pointer to the bottom of the stack - needed for doing relocations. */
    RCPTRTYPE(uint8_t *)        pbEMTStackRC;
    /** Pointer to the bottom of the stack - needed for doing relocations. */
    RCPTRTYPE(uint8_t *)        pbEMTStackBottomRC;

    /** Pointer to the R0 logger instance - R3 Ptr.
     * This is NULL if logging is disabled. */
    R3PTRTYPE(PVMMR0LOGGER)     pR0LoggerR3;
    /** Pointer to the R0 logger instance - R0 Ptr.
     * This is NULL if logging is disabled. */
    R0PTRTYPE(PVMMR0LOGGER)     pR0LoggerR0;

    /** Thread context switching hook (ring-0). */
    RTTHREADCTXHOOK             hCtxHook;

    /** @name Rendezvous
     * @{ */
    /** Whether the EMT is executing a rendezvous right now. For detecting
     *  attempts at recursive rendezvous. */
    bool volatile               fInRendezvous;
    bool                        afPadding[HC_ARCH_BITS == 32 ? 3+4 : 7+8];
    /** @} */

    /** @name Raw-mode context tracing data.
     * @{ */
    SUPDRVTRACERUSRCTX          TracerCtx;
    /** @} */

    /** Alignment padding, making sure u64CallRing3Arg is nicely aligned. */
    uint32_t                    au32Padding1[3];

    /** @name Call Ring-3
     * Formerly known as host calls.
     * @{ */
    /** The disable counter. */
    uint32_t                    cCallRing3Disabled;
    /** The pending operation. */
    VMMCALLRING3                enmCallRing3Operation;
    /** The result of the last operation. */
    int32_t                     rcCallRing3;
    /** The argument to the operation. */
    uint64_t                    u64CallRing3Arg;
    /** The Ring-0 notification callback. */
    R0PTRTYPE(PFNVMMR0CALLRING3NOTIFICATION)   pfnCallRing3CallbackR0;
    /** The Ring-0 notification callback user argument. */
    R0PTRTYPE(void *)           pvCallRing3CallbackUserR0;
    /** The Ring-0 jmp buffer.
     * @remarks The size of this type isn't stable in assembly, so don't put
     *          anything that needs to be accessed from assembly after it. */
    VMMR0JMPBUF                 CallRing3JmpBufR0;
    /** @} */
} VMMCPU;
AssertCompileMemberAlignment(VMMCPU, TracerCtx, 8);
/** Pointer to VMMCPU. */
typedef VMMCPU *PVMMCPU;


/**
 * The VMMRCEntry() codes.
 */
typedef enum VMMRCOPERATION
{
    /** Do GC module init. */
    VMMRC_DO_VMMRC_INIT = 1,

    /** The first Trap testcase. */
    VMMRC_DO_TESTCASE_TRAP_FIRST = 0x0dead000,
    /** Trap 0 testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_0 = VMMRC_DO_TESTCASE_TRAP_FIRST,
    /** Trap 1 testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_1,
    /** Trap 2 testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_2,
    /** Trap 3 testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_3,
    /** Trap 4 testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_4,
    /** Trap 5 testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_5,
    /** Trap 6 testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_6,
    /** Trap 7 testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_7,
    /** Trap 8 testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_8,
    /** Trap 9 testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_9,
    /** Trap 0a testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_0A,
    /** Trap 0b testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_0B,
    /** Trap 0c testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_0C,
    /** Trap 0d testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_0D,
    /** Trap 0e testcases, uArg selects the variation. */
    VMMRC_DO_TESTCASE_TRAP_0E,
    /** The last trap testcase (exclusive). */
    VMMRC_DO_TESTCASE_TRAP_LAST,
    /** Testcase for checking interrupt forwarding. */
    VMMRC_DO_TESTCASE_HYPER_INTERRUPT,
    /** Switching testing and profiling stub. */
    VMMRC_DO_TESTCASE_NOP,
    /** Testcase for checking interrupt masking. */
    VMMRC_DO_TESTCASE_INTERRUPT_MASKING,
    /** Switching testing and profiling stub. */
    VMMRC_DO_TESTCASE_HM_NOP,

    /** The usual 32-bit hack. */
    VMMRC_DO_32_BIT_HACK = 0x7fffffff
} VMMRCOPERATION;



/**
 * MSR test result entry.
 */
typedef struct VMMTESTMSRENTRY
{
    /** The MSR number, including padding.
     * Set to UINT64_MAX if invalid MSR. */
    uint64_t    uMsr;
    /** The register value. */
    uint64_t    uValue;
} VMMTESTMSRENTRY;
/** Pointer to an MSR test result entry. */
typedef VMMTESTMSRENTRY *PVMMTESTMSRENTRY;



RT_C_DECLS_BEGIN

int  vmmInitFormatTypes(void);
void vmmTermFormatTypes(void);
uint32_t vmmGetBuildType(void);

#ifdef IN_RING3
int  vmmR3SwitcherInit(PVM pVM);
void vmmR3SwitcherRelocate(PVM pVM, RTGCINTPTR offDelta);
#endif /* IN_RING3 */

#ifdef IN_RING0
/**
 * World switcher assembly routine.
 * It will call VMMRCEntry().
 *
 * @returns return code from VMMRCEntry().
 * @param   pVM         The cross context VM structure.
 * @param   uArg        See VMMRCEntry().
 * @internal
 */
DECLASM(int)    vmmR0WorldSwitch(PVM pVM, unsigned uArg);

/**
 * Callback function for vmmR0CallRing3SetJmp.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
typedef DECLCALLBACK(int) FNVMMR0SETJMP(PVM pVM, PVMCPU pVCpu);
/** Pointer to FNVMMR0SETJMP(). */
typedef FNVMMR0SETJMP *PFNVMMR0SETJMP;

/**
 * The setjmp variant used for calling Ring-3.
 *
 * This differs from the normal setjmp in that it will resume VMMRZCallRing3 if we're
 * in the middle of a ring-3 call. Another differences is the function pointer and
 * argument. This has to do with resuming code and the stack frame of the caller.
 *
 * @returns VINF_SUCCESS on success or whatever is passed to vmmR0CallRing3LongJmp.
 * @param   pJmpBuf     The jmp_buf to set.
 * @param   pfn         The function to be called when not resuming.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 */
DECLASM(int)    vmmR0CallRing3SetJmp(PVMMR0JMPBUF pJmpBuf, PFNVMMR0SETJMP pfn, PVM pVM, PVMCPU pVCpu);

/**
 * Callback function for vmmR0CallRing3SetJmpEx.
 *
 * @returns VBox status code.
 * @param   pvUser      The user argument.
 */
typedef DECLCALLBACK(int) FNVMMR0SETJMPEX(void *pvUser);
/** Pointer to FNVMMR0SETJMP(). */
typedef FNVMMR0SETJMPEX *PFNVMMR0SETJMPEX;

/**
 * Same as vmmR0CallRing3SetJmp except for the function signature.
 *
 * @returns VINF_SUCCESS on success or whatever is passed to vmmR0CallRing3LongJmp.
 * @param   pJmpBuf     The jmp_buf to set.
 * @param   pfn         The function to be called when not resuming.
 * @param   pvUser      The argument of that function.
 */
DECLASM(int)    vmmR0CallRing3SetJmpEx(PVMMR0JMPBUF pJmpBuf, PFNVMMR0SETJMPEX pfn, void *pvUser);


/**
 * Worker for VMMRZCallRing3.
 * This will save the stack and registers.
 *
 * @returns rc.
 * @param   pJmpBuf     Pointer to the jump buffer.
 * @param   rc          The return code.
 */
DECLASM(int)    vmmR0CallRing3LongJmp(PVMMR0JMPBUF pJmpBuf, int rc);

/**
 * Internal R0 logger worker: Logger wrapper.
 */
VMMR0DECL(void) vmmR0LoggerWrapper(const char *pszFormat, ...);

/**
 * Internal R0 logger worker: Flush logger.
 *
 * @param   pLogger     The logger instance to flush.
 * @remark  This function must be exported!
 */
VMMR0DECL(void) vmmR0LoggerFlush(PRTLOGGER pLogger);

/**
 * Internal R0 logger worker: Custom prefix.
 *
 * @returns Number of chars written.
 *
 * @param   pLogger     The logger instance.
 * @param   pchBuf      The output buffer.
 * @param   cchBuf      The size of the buffer.
 * @param   pvUser      User argument (ignored).
 */
VMMR0DECL(size_t) vmmR0LoggerPrefix(PRTLOGGER pLogger, char *pchBuf, size_t cchBuf, void *pvUser);

# ifdef VBOX_WITH_TRIPLE_FAULT_HACK
int  vmmR0TripleFaultHackInit(void);
void vmmR0TripleFaultHackTerm(void);
# endif

#endif /* IN_RING0 */
#ifdef IN_RC

/**
 * Internal GC logger worker: Logger wrapper.
 */
VMMRCDECL(void) vmmGCLoggerWrapper(const char *pszFormat, ...);

/**
 * Internal GC release logger worker: Logger wrapper.
 */
VMMRCDECL(void) vmmGCRelLoggerWrapper(const char *pszFormat, ...);

/**
 * Internal GC logger worker: Flush logger.
 *
 * @returns VINF_SUCCESS.
 * @param   pLogger     The logger instance to flush.
 * @remark  This function must be exported!
 */
VMMRCDECL(int)  vmmGCLoggerFlush(PRTLOGGERRC pLogger);

/** @name Trap testcases and related labels.
 * @{ */
DECLASM(void)   vmmGCEnableWP(void);
DECLASM(void)   vmmGCDisableWP(void);
DECLASM(int)    vmmGCTestTrap3(void);
DECLASM(int)    vmmGCTestTrap8(void);
DECLASM(int)    vmmGCTestTrap0d(void);
DECLASM(int)    vmmGCTestTrap0e(void);
DECLASM(int)    vmmGCTestTrap0e_FaultEIP(void); /**< a label */
DECLASM(int)    vmmGCTestTrap0e_ResumeEIP(void); /**< a label */
/** @} */

#endif /* IN_RC */

RT_C_DECLS_END

/** @} */

#endif
