/* $Id: HMInternal.h $ */
/** @file
 * HM - Internal header file.
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

#ifndef ___HMInternal_h
#define ___HMInternal_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/vmm/em.h>
#include <VBox/vmm/stam.h>
#include <VBox/dis.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/hm_vmx.h>
#include <VBox/vmm/hm_svm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/trpm.h>
#include <iprt/memobj.h>
#include <iprt/cpuset.h>
#include <iprt/mp.h>
#include <iprt/avl.h>
#include <iprt/string.h>

#if defined(RT_OS_DARWIN) && HC_ARCH_BITS == 32
# error "32-bit darwin is no longer supported. Go back to 4.3 or earlier!"
#endif

#if HC_ARCH_BITS == 64 || defined (VBOX_WITH_64_BITS_GUESTS)
/* Enable 64 bits guest support. */
# define VBOX_ENABLE_64_BITS_GUESTS
#endif

#if HC_ARCH_BITS == 32 && defined(VBOX_ENABLE_64_BITS_GUESTS)
# define VMX_USE_CACHED_VMCS_ACCESSES
#endif

/** @def HM_PROFILE_EXIT_DISPATCH
 * Enables profiling of the VM exit handler dispatching. */
#if 0 || defined(DOXYGEN_RUNNING)
# define HM_PROFILE_EXIT_DISPATCH
#endif

RT_C_DECLS_BEGIN


/** @defgroup grp_hm_int       Internal
 * @ingroup grp_hm
 * @internal
 * @{
 */

/** @def HMCPU_CF_CLEAR
 * Clears a HM-context flag.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlag   The flag to clear.
 */
#define HMCPU_CF_CLEAR(pVCpu, fFlag)              (ASMAtomicUoAndU32(&(pVCpu)->hm.s.fContextUseFlags, ~(fFlag)))

/** @def HMCPU_CF_SET
 * Sets a HM-context flag.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlag   The flag to set.
 */
#define HMCPU_CF_SET(pVCpu, fFlag)                (ASMAtomicUoOrU32(&(pVCpu)->hm.s.fContextUseFlags, (fFlag)))

/** @def HMCPU_CF_IS_SET
 * Checks if all the flags in the specified HM-context set is pending.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlag   The flag to check.
 */
#define HMCPU_CF_IS_SET(pVCpu, fFlag)             ((ASMAtomicUoReadU32(&(pVCpu)->hm.s.fContextUseFlags) & (fFlag)) == (fFlag))

/** @def HMCPU_CF_IS_PENDING
 * Checks if one or more of the flags in the specified HM-context set is
 * pending.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlags  The flags to check for.
 */
#define HMCPU_CF_IS_PENDING(pVCpu, fFlags)        RT_BOOL(ASMAtomicUoReadU32(&(pVCpu)->hm.s.fContextUseFlags) & (fFlags))

/** @def HMCPU_CF_IS_PENDING_ONLY
 * Checks if -only- one or more of the specified HM-context flags is pending.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlags  The flags to check for.
 */
#define HMCPU_CF_IS_PENDING_ONLY(pVCpu, fFlags)   !RT_BOOL(ASMAtomicUoReadU32(&(pVCpu)->hm.s.fContextUseFlags) & ~(fFlags))

/** @def HMCPU_CF_IS_SET_ONLY
 * Checks if -only- all the flags in the specified HM-context set is pending.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlags  The flags to check for.
 */
#define HMCPU_CF_IS_SET_ONLY(pVCpu, fFlags)       (ASMAtomicUoReadU32(&(pVCpu)->hm.s.fContextUseFlags) == (fFlags))

/** @def HMCPU_CF_RESET_TO
 * Resets the HM-context flags to the specified value.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   fFlags  The new value.
 */
#define HMCPU_CF_RESET_TO(pVCpu, fFlags)          (ASMAtomicUoWriteU32(&(pVCpu)->hm.s.fContextUseFlags, (fFlags)))

/** @def HMCPU_CF_VALUE
 * Returns the current HM-context flags value.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 */
#define HMCPU_CF_VALUE(pVCpu)                     (ASMAtomicUoReadU32(&(pVCpu)->hm.s.fContextUseFlags))


/** Resets/initializes the VM-exit/\#VMEXIT history array. */
#define HMCPU_EXIT_HISTORY_RESET(pVCpu)           (memset(&(pVCpu)->hm.s.auExitHistory, 0xff, sizeof((pVCpu)->hm.s.auExitHistory)))

/** Updates the VM-exit/\#VMEXIT history array. */
#define HMCPU_EXIT_HISTORY_ADD(pVCpu, a_ExitReason) \
    do { \
        AssertMsg((pVCpu)->hm.s.idxExitHistoryFree < RT_ELEMENTS((pVCpu)->hm.s.auExitHistory), ("%u\n", (pVCpu)->hm.s.idxExitHistoryFree)); \
        (pVCpu)->hm.s.auExitHistory[(pVCpu)->hm.s.idxExitHistoryFree++] = (uint16_t)(a_ExitReason); \
        if ((pVCpu)->hm.s.idxExitHistoryFree == RT_ELEMENTS((pVCpu)->hm.s.auExitHistory)) \
            (pVCpu)->hm.s.idxExitHistoryFree = 0; \
        (pVCpu)->hm.s.auExitHistory[(pVCpu)->hm.s.idxExitHistoryFree] = UINT16_MAX; \
    } while (0)

/** Maximum number of exit reason statistics counters. */
#define MAX_EXITREASON_STAT        0x100
#define MASK_EXITREASON_STAT       0xff
#define MASK_INJECT_IRQ_STAT       0xff

/** @name HM changed flags.
 * These flags are used to keep track of which important registers that have
 * been changed since last they were reset.
 *
 * Flags marked "shared" are used for registers that are common to both the host
 * and guest (i.e. without dedicated VMCS/VMCB fields for guest bits).
 *
 * @{
 */
#define HM_CHANGED_GUEST_CR0                     RT_BIT(0)      /* Shared */
#define HM_CHANGED_GUEST_CR3                     RT_BIT(1)
#define HM_CHANGED_GUEST_CR4                     RT_BIT(2)
#define HM_CHANGED_GUEST_GDTR                    RT_BIT(3)
#define HM_CHANGED_GUEST_IDTR                    RT_BIT(4)
#define HM_CHANGED_GUEST_LDTR                    RT_BIT(5)
#define HM_CHANGED_GUEST_TR                      RT_BIT(6)
#define HM_CHANGED_GUEST_SEGMENT_REGS            RT_BIT(7)
#define HM_CHANGED_GUEST_DEBUG                   RT_BIT(8)      /* Shared */
#define HM_CHANGED_GUEST_RIP                     RT_BIT(9)
#define HM_CHANGED_GUEST_RSP                     RT_BIT(10)
#define HM_CHANGED_GUEST_RFLAGS                  RT_BIT(11)
#define HM_CHANGED_GUEST_CR2                     RT_BIT(12)
#define HM_CHANGED_GUEST_SYSENTER_CS_MSR         RT_BIT(13)
#define HM_CHANGED_GUEST_SYSENTER_EIP_MSR        RT_BIT(14)
#define HM_CHANGED_GUEST_SYSENTER_ESP_MSR        RT_BIT(15)
#define HM_CHANGED_GUEST_EFER_MSR                RT_BIT(16)
#define HM_CHANGED_GUEST_LAZY_MSRS               RT_BIT(17)     /* Shared */ /** @todo Move this to VT-x specific? */
#define HM_CHANGED_GUEST_XCPT_INTERCEPTS         RT_BIT(18)
/* VT-x specific state. */
#define HM_CHANGED_VMX_GUEST_AUTO_MSRS           RT_BIT(19)
#define HM_CHANGED_VMX_GUEST_ACTIVITY_STATE      RT_BIT(20)
#define HM_CHANGED_VMX_GUEST_APIC_STATE          RT_BIT(21)
#define HM_CHANGED_VMX_ENTRY_CTLS                RT_BIT(22)
#define HM_CHANGED_VMX_EXIT_CTLS                 RT_BIT(23)
/* AMD-V specific state. */
#define HM_CHANGED_SVM_GUEST_APIC_STATE          RT_BIT(19)
#define HM_CHANGED_SVM_RESERVED1                 RT_BIT(20)
#define HM_CHANGED_SVM_RESERVED2                 RT_BIT(21)
#define HM_CHANGED_SVM_RESERVED3                 RT_BIT(22)
#define HM_CHANGED_SVM_RESERVED4                 RT_BIT(23)

#define HM_CHANGED_ALL_GUEST                     (  HM_CHANGED_GUEST_CR0                \
                                                  | HM_CHANGED_GUEST_CR3                \
                                                  | HM_CHANGED_GUEST_CR4                \
                                                  | HM_CHANGED_GUEST_GDTR               \
                                                  | HM_CHANGED_GUEST_IDTR               \
                                                  | HM_CHANGED_GUEST_LDTR               \
                                                  | HM_CHANGED_GUEST_TR                 \
                                                  | HM_CHANGED_GUEST_SEGMENT_REGS       \
                                                  | HM_CHANGED_GUEST_DEBUG              \
                                                  | HM_CHANGED_GUEST_RIP                \
                                                  | HM_CHANGED_GUEST_RSP                \
                                                  | HM_CHANGED_GUEST_RFLAGS             \
                                                  | HM_CHANGED_GUEST_CR2                \
                                                  | HM_CHANGED_GUEST_SYSENTER_CS_MSR    \
                                                  | HM_CHANGED_GUEST_SYSENTER_EIP_MSR   \
                                                  | HM_CHANGED_GUEST_SYSENTER_ESP_MSR   \
                                                  | HM_CHANGED_GUEST_EFER_MSR           \
                                                  | HM_CHANGED_GUEST_LAZY_MSRS          \
                                                  | HM_CHANGED_GUEST_XCPT_INTERCEPTS    \
                                                  | HM_CHANGED_VMX_GUEST_AUTO_MSRS      \
                                                  | HM_CHANGED_VMX_GUEST_ACTIVITY_STATE \
                                                  | HM_CHANGED_VMX_GUEST_APIC_STATE     \
                                                  | HM_CHANGED_VMX_ENTRY_CTLS           \
                                                  | HM_CHANGED_VMX_EXIT_CTLS)

#define HM_CHANGED_HOST_CONTEXT                  RT_BIT(24)

/* Bits shared between host and guest. */
#define HM_CHANGED_HOST_GUEST_SHARED_STATE       (  HM_CHANGED_GUEST_CR0                \
                                                  | HM_CHANGED_GUEST_DEBUG              \
                                                  | HM_CHANGED_GUEST_LAZY_MSRS)
/** @} */

/** Size for the EPT identity page table (1024 4 MB pages to cover the entire address space). */
#define HM_EPT_IDENTITY_PG_TABLE_SIZE   PAGE_SIZE
/** Size of the TSS structure + 2 pages for the IO bitmap + end byte. */
#define HM_VTX_TSS_SIZE                 (sizeof(VBOXTSS) + 2 * PAGE_SIZE + 1)
/** Total guest mapped memory needed. */
#define HM_VTX_TOTAL_DEVHEAP_MEM        (HM_EPT_IDENTITY_PG_TABLE_SIZE + HM_VTX_TSS_SIZE)


/** @name Macros for enabling and disabling preemption.
 * These are really just for hiding the RTTHREADPREEMPTSTATE and asserting that
 * preemption has already been disabled when there is no context hook.
 * @{ */
#ifdef VBOX_STRICT
# define HM_DISABLE_PREEMPT() \
    RTTHREADPREEMPTSTATE PreemptStateInternal = RTTHREADPREEMPTSTATE_INITIALIZER; \
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD) || VMMR0ThreadCtxHookIsEnabled(pVCpu)); \
    RTThreadPreemptDisable(&PreemptStateInternal)
#else
# define HM_DISABLE_PREEMPT() \
    RTTHREADPREEMPTSTATE PreemptStateInternal = RTTHREADPREEMPTSTATE_INITIALIZER; \
    RTThreadPreemptDisable(&PreemptStateInternal)
#endif /* VBOX_STRICT */
#define HM_RESTORE_PREEMPT()    do { RTThreadPreemptRestore(&PreemptStateInternal); } while(0)
/** @} */


/** Enable for TPR guest patching. */
#define VBOX_HM_WITH_GUEST_PATCHING

/** @name HM saved state versions
 * @{
 */
#ifdef VBOX_HM_WITH_GUEST_PATCHING
# define HM_SAVED_STATE_VERSION                 5
# define HM_SAVED_STATE_VERSION_NO_PATCHING     4
#else
# define HM_SAVED_STATE_VERSION                 4
# define HM_SAVED_STATE_VERSION_NO_PATCHING     4
#endif
#define HM_SAVED_STATE_VERSION_2_0_X            3
/** @} */

/**
 * Global per-cpu information. (host)
 */
typedef struct HMGLOBALCPUINFO
{
    /** The CPU ID. */
    RTCPUID             idCpu;
    /** The VM_HSAVE_AREA (AMD-V) / VMXON region (Intel) memory backing. */
    RTR0MEMOBJ          hMemObj;
    /** The physical address of the first page in hMemObj (it's a
     *  physcially contigous allocation if it spans multiple pages). */
    RTHCPHYS            HCPhysMemObj;
    /** The address of the memory (for pfnEnable). */
    void               *pvMemObj;
    /** Current ASID (AMD-V) / VPID (Intel). */
    uint32_t            uCurrentAsid;
    /** TLB flush count. */
    uint32_t            cTlbFlushes;
    /** Whether to flush each new ASID/VPID before use. */
    bool                fFlushAsidBeforeUse;
    /** Configured for VT-x or AMD-V. */
    bool                fConfigured;
    /** Set if the VBOX_HWVIRTEX_IGNORE_SVM_IN_USE hack is active. */
    bool                fIgnoreAMDVInUseError;
    /** In use by our code. (for power suspend) */
    volatile bool       fInUse;
} HMGLOBALCPUINFO;
/** Pointer to the per-cpu global information. */
typedef HMGLOBALCPUINFO *PHMGLOBALCPUINFO;

typedef enum
{
    HMPENDINGIO_INVALID = 0,
    HMPENDINGIO_PORT_READ,
    /* not implemented: HMPENDINGIO_STRING_READ, */
    /* not implemented: HMPENDINGIO_STRING_WRITE, */
    /** The usual 32-bit paranoia. */
    HMPENDINGIO_32BIT_HACK   = 0x7fffffff
} HMPENDINGIO;


typedef enum
{
    HMTPRINSTR_INVALID,
    HMTPRINSTR_READ,
    HMTPRINSTR_READ_SHR4,
    HMTPRINSTR_WRITE_REG,
    HMTPRINSTR_WRITE_IMM,
    HMTPRINSTR_JUMP_REPLACEMENT,
    /** The usual 32-bit paranoia. */
    HMTPRINSTR_32BIT_HACK   = 0x7fffffff
} HMTPRINSTR;

typedef struct
{
    /** The key is the address of patched instruction. (32 bits GC ptr) */
    AVLOU32NODECORE         Core;
    /** Original opcode. */
    uint8_t                 aOpcode[16];
    /** Instruction size. */
    uint32_t                cbOp;
    /** Replacement opcode. */
    uint8_t                 aNewOpcode[16];
    /** Replacement instruction size. */
    uint32_t                cbNewOp;
    /** Instruction type. */
    HMTPRINSTR              enmType;
    /** Source operand. */
    uint32_t                uSrcOperand;
    /** Destination operand. */
    uint32_t                uDstOperand;
    /** Number of times the instruction caused a fault. */
    uint32_t                cFaults;
    /** Patch address of the jump replacement. */
    RTGCPTR32               pJumpTarget;
} HMTPRPATCH;
/** Pointer to HMTPRPATCH. */
typedef HMTPRPATCH *PHMTPRPATCH;


/**
 * Makes a HMEXITSTAT::uKey value from a program counter and an exit code.
 *
 * @returns 64-bit key
 * @param   a_uPC           The RIP + CS.BASE value of the exit.
 * @param   a_uExit         The exit code.
 * @todo    Add CPL?
 */
#define HMEXITSTAT_MAKE_KEY(a_uPC, a_uExit) (((a_uPC) & UINT64_C(0x0000ffffffffffff)) | (uint64_t)(a_uExit) << 48)

typedef struct HMEXITINFO
{
    /** See HMEXITSTAT_MAKE_KEY(). */
    uint64_t                uKey;
    /** Number of recent hits (depreciates with time). */
    uint32_t volatile       cHits;
    /** The age + lock. */
    uint16_t volatile       uAge;
    /** Action or action table index. */
    uint16_t                iAction;
} HMEXITINFO;
AssertCompileSize(HMEXITINFO, 16); /* Lots of these guys, so don't add any unnecessary stuff! */

typedef struct HMEXITHISTORY
{
    /** The exit timestamp. */
    uint64_t                uTscExit;
    /** The index of the corresponding HMEXITINFO entry.
     * UINT32_MAX if none (too many collisions, race, whatever). */
    uint32_t                iExitInfo;
    /** Figure out later, needed for padding now. */
    uint32_t                uSomeClueOrSomething;
} HMEXITHISTORY;

/**
 * Switcher function, HC to the special 64-bit RC.
 *
 * @param   pVM             The cross context VM structure.
 * @param   offCpumVCpu     Offset from pVM->cpum to pVM->aCpus[idCpu].cpum.
 * @returns Return code indicating the action to take.
 */
typedef DECLCALLBACK(int) FNHMSWITCHERHC(PVM pVM, uint32_t offCpumVCpu);
/** Pointer to switcher function. */
typedef FNHMSWITCHERHC *PFNHMSWITCHERHC;

/**
 * HM VM Instance data.
 * Changes to this must checked against the padding of the hm union in VM!
 */
typedef struct HM
{
    /** Set when we've initialized VMX or SVM. */
    bool                        fInitialized;
    /** Set if nested paging is enabled. */
    bool                        fNestedPaging;
    /** Set if nested paging is allowed. */
    bool                        fAllowNestedPaging;
    /** Set if large pages are enabled (requires nested paging). */
    bool                        fLargePages;
    /** Set if we can support 64-bit guests or not. */
    bool                        fAllow64BitGuests;
    /** Set when TPR patching is allowed. */
    bool                        fTprPatchingAllowed;
    /** Set when we initialize VT-x or AMD-V once for all CPUs. */
    bool                        fGlobalInit;
    /** Set when TPR patching is active. */
    bool                        fTPRPatchingActive;
    /** Set when the debug facility has breakpoints/events enabled that requires
     *  us to use the debug execution loop in ring-0. */
    bool                        fUseDebugLoop;
    /** Set if hardware APIC virtualization is enabled. */
    bool                        fVirtApicRegs;
    /** Set if posted interrupt processing is enabled. */
    bool                        fPostedIntrs;
    /** Set if indirect branch prediction barrier on VM exit. */
    bool                        fIbpbOnVmExit;
    /** Set if indirect branch prediction barrier on VM entry. */
    bool                        fIbpbOnVmEntry;
    /** Set if host manages speculation control settings. */
    bool                        fSpecCtrlByHost;
    /** Explicit padding. */
    bool                        afPadding[2];

    /** Maximum ASID allowed. */
    uint32_t                    uMaxAsid;
    /** The maximum number of resumes loops allowed in ring-0 (safety precaution).
     * This number is set much higher when RTThreadPreemptIsPending is reliable. */
    uint32_t                    cMaxResumeLoops;

    /** Host kernel flags that HM might need to know (SUPKERNELFEATURES_XXX). */
    uint32_t                    fHostKernelFeatures;

    /** Size of the guest patch memory block. */
    uint32_t                    cbGuestPatchMem;
    /** Guest allocated memory for patching purposes. */
    RTGCPTR                     pGuestPatchMem;
    /** Current free pointer inside the patch block. */
    RTGCPTR                     pFreeGuestPatchMem;

#if HC_ARCH_BITS == 32 && defined(VBOX_ENABLE_64_BITS_GUESTS)
    /** 32 to 64 bits switcher entrypoint. */
    R0PTRTYPE(PFNHMSWITCHERHC)  pfnHost32ToGuest64R0;
    RTR0PTR                     pvR0Alignment0;
#endif

    struct
    {
        /** Set by the ring-0 side of HM to indicate VMX is supported by the
         *  CPU. */
        bool                        fSupported;
        /** Set when we've enabled VMX. */
        bool                        fEnabled;
        /** Set if VPID is supported. */
        bool                        fVpid;
        /** Set if VT-x VPID is allowed. */
        bool                        fAllowVpid;
        /** Set if unrestricted guest execution is in use (real and protected mode without paging). */
        bool                        fUnrestrictedGuest;
        /** Set if unrestricted guest execution is allowed to be used. */
        bool                        fAllowUnrestricted;
        /** Whether we're using the preemption timer or not. */
        bool                        fUsePreemptTimer;
        /** The shift mask employed by the VMX-Preemption timer. */
        uint8_t                     cPreemptTimerShift;

        /** Virtual address of the TSS page used for real mode emulation. */
        R3PTRTYPE(PVBOXTSS)         pRealModeTSS;
        /** Virtual address of the identity page table used for real mode and protected mode without paging emulation in EPT mode. */
        R3PTRTYPE(PX86PD)           pNonPagingModeEPTPageTable;

        /** Physical address of the APIC-access page. */
        RTHCPHYS                    HCPhysApicAccess;
        /** R0 memory object for the APIC-access page. */
        RTR0MEMOBJ                  hMemObjApicAccess;
        /** Virtual address of the APIC-access page. */
        R0PTRTYPE(uint8_t *)        pbApicAccess;

#ifdef VBOX_WITH_CRASHDUMP_MAGIC
        RTHCPHYS                    HCPhysScratch;
        RTR0MEMOBJ                  hMemObjScratch;
        R0PTRTYPE(uint8_t *)        pbScratch;
#endif

        /** Internal Id of which flush-handler to use for tagged-TLB entries. */
        uint32_t                    uFlushTaggedTlb;

        /** Pause-loop exiting (PLE) gap in ticks. */
        uint32_t                    cPleGapTicks;
        /** Pause-loop exiting (PLE) window in ticks. */
        uint32_t                    cPleWindowTicks;
        uint32_t                    u32Alignment0;

        /** Host CR4 value (set by ring-0 VMX init) */
        uint64_t                    u64HostCr4;
        /** Host SMM monitor control (set by ring-0 VMX init) */
        uint64_t                    u64HostSmmMonitorCtl;
        /** Host EFER value (set by ring-0 VMX init) */
        uint64_t                    u64HostEfer;
        /** Whether the CPU supports VMCS fields for swapping EFER. */
        bool                        fSupportsVmcsEfer;
        uint8_t                     u8Alignment2[7];

        /** VMX MSR values. */
        VMXMSRS                     Msrs;

        /** Flush types for invept & invvpid; they depend on capabilities. */
        VMXFLUSHEPT                 enmFlushEpt;
        VMXFLUSHVPID                enmFlushVpid;

        /** Host-physical address for a failing VMXON instruction. */
        RTHCPHYS                    HCPhysVmxEnableError;
    } vmx;

    struct
    {
        /** Set by the ring-0 side of HM to indicate SVM is supported by the
         *  CPU. */
        bool                        fSupported;
        /** Set when we've enabled SVM. */
        bool                        fEnabled;
        /** Set if erratum 170 affects the AMD cpu. */
        bool                        fAlwaysFlushTLB;
        /** Set when the hack to ignore VERR_SVM_IN_USE is active. */
        bool                        fIgnoreInUseError;
        uint8_t                     u8Alignment0[4];

        /** Physical address of the IO bitmap (12kb). */
        RTHCPHYS                    HCPhysIOBitmap;
        /** R0 memory object for the IO bitmap (12kb). */
        RTR0MEMOBJ                  hMemObjIOBitmap;
        /** Virtual address of the IO bitmap. */
        R0PTRTYPE(void *)           pvIOBitmap;

        /* HWCR MSR (for diagnostics) */
        uint64_t                    u64MsrHwcr;

        /** SVM revision. */
        uint32_t                    u32Rev;
        /** SVM feature bits from cpuid 0x8000000a */
        uint32_t                    u32Features;

        /** Pause filter counter. */
        uint16_t                    cPauseFilter;
        /** Pause filter treshold in ticks. */
        uint16_t                    cPauseFilterThresholdTicks;
        uint32_t                    u32Alignment0;
    } svm;

    /**
     * AVL tree with all patches (active or disabled) sorted by guest instruction
     * address.
     */
    AVLOU32TREE                     PatchTree;
    uint32_t                        cPatches;
    HMTPRPATCH                      aPatches[64];

    struct
    {
        uint32_t                    u32AMDFeatureECX;
        uint32_t                    u32AMDFeatureEDX;
    } cpuid;

    /** Saved error from detection */
    int32_t                 lLastError;

    /** HMR0Init was run */
    bool                    fHMR0Init;
    bool                    u8Alignment1[3];

    STAMCOUNTER             StatTprPatchSuccess;
    STAMCOUNTER             StatTprPatchFailure;
    STAMCOUNTER             StatTprReplaceSuccessCr8;
    STAMCOUNTER             StatTprReplaceSuccessVmc;
    STAMCOUNTER             StatTprReplaceFailure;
} HM;
/** Pointer to HM VM instance data. */
typedef HM *PHM;

AssertCompileMemberAlignment(HM, StatTprPatchSuccess, 8);

/* Maximum number of cached entries. */
#define VMCSCACHE_MAX_ENTRY                             128

/**
 * Structure for storing read and write VMCS actions.
 */
typedef struct VMCSCACHE
{
#ifdef VBOX_WITH_CRASHDUMP_MAGIC
    /* Magic marker for searching in crash dumps. */
    uint8_t         aMagic[16];
    uint64_t        uMagic;
    uint64_t        u64TimeEntry;
    uint64_t        u64TimeSwitch;
    uint64_t        cResume;
    uint64_t        interPD;
    uint64_t        pSwitcher;
    uint32_t        uPos;
    uint32_t        idCpu;
#endif
    /* CR2 is saved here for EPT syncing. */
    uint64_t        cr2;
    struct
    {
        uint32_t    cValidEntries;
        uint32_t    uAlignment;
        uint32_t    aField[VMCSCACHE_MAX_ENTRY];
        uint64_t    aFieldVal[VMCSCACHE_MAX_ENTRY];
    } Write;
    struct
    {
        uint32_t    cValidEntries;
        uint32_t    uAlignment;
        uint32_t    aField[VMCSCACHE_MAX_ENTRY];
        uint64_t    aFieldVal[VMCSCACHE_MAX_ENTRY];
    } Read;
#ifdef VBOX_STRICT
    struct
    {
        RTHCPHYS    HCPhysCpuPage;
        RTHCPHYS    HCPhysVmcs;
        RTGCPTR     pCache;
        RTGCPTR     pCtx;
    } TestIn;
    struct
    {
        RTHCPHYS    HCPhysVmcs;
        RTGCPTR     pCache;
        RTGCPTR     pCtx;
        uint64_t    eflags;
        uint64_t    cr8;
    } TestOut;
    struct
    {
        uint64_t    param1;
        uint64_t    param2;
        uint64_t    param3;
        uint64_t    param4;
    } ScratchPad;
#endif
} VMCSCACHE;
/** Pointer to VMCSCACHE. */
typedef VMCSCACHE *PVMCSCACHE;
AssertCompileSizeAlignment(VMCSCACHE, 8);

/**
 * VMX StartVM function.
 *
 * @returns VBox status code (no informational stuff).
 * @param   fResume     Whether to use VMRESUME (true) or VMLAUNCH (false).
 * @param   pCtx        The CPU register context.
 * @param   pCache      The VMCS cache.
 * @param   pVM         Pointer to the cross context VM structure.
 * @param   pVCpu       Pointer to the cross context per-CPU structure.
 */
typedef DECLCALLBACK(int) FNHMVMXSTARTVM(RTHCUINT fResume, PCPUMCTX pCtx, PVMCSCACHE pCache, PVM pVM, PVMCPU pVCpu);
/** Pointer to a VMX StartVM function. */
typedef R0PTRTYPE(FNHMVMXSTARTVM *) PFNHMVMXSTARTVM;

/** SVM VMRun function. */
typedef DECLCALLBACK(int) FNHMSVMVMRUN(RTHCPHYS pVmcbHostPhys, RTHCPHYS pVmcbPhys, PCPUMCTX pCtx, PVM pVM, PVMCPU pVCpu);
/** Pointer to a SVM VMRun function. */
typedef R0PTRTYPE(FNHMSVMVMRUN *) PFNHMSVMVMRUN;

/**
 * HM VMCPU Instance data.
 *
 * Note! If you change members of this struct, make sure to check if the
 * assembly counterpart in HMInternal.mac needs to be updated as well.
 */
typedef struct HMCPU
{
    /** Set if we need to flush the TLB during the world switch. */
    bool                        fForceTLBFlush;
    /** Set when we're using VT-x or AMD-V at that moment. */
    bool                        fActive;
    /** Set when the TLB has been checked until we return from the world switch. */
    volatile bool               fCheckedTLBFlush;
    /** Whether we've completed the inner HM leave function. */
    bool                        fLeaveDone;
    /** Whether we're using the hyper DR7 or guest DR7. */
    bool                        fUsingHyperDR7;
    /** Set if XCR0 needs to be loaded and saved when entering and exiting guest
     * code execution. */
    bool                        fLoadSaveGuestXcr0;

    /** Whether we should use the debug loop because of single stepping or special
     *  debug breakpoints / events are armed. */
    bool                        fUseDebugLoop;
    /** Whether we are currently executing in the debug loop.
     *  Mainly for assertions. */
    bool                        fUsingDebugLoop;
    /** Set if we using the debug loop and wish to intercept RDTSC. */
    bool                        fDebugWantRdTscExit;
    /** Whether we're executing a single instruction. */
    bool                        fSingleInstruction;
    /** Set if we need to clear the trap flag because of single stepping. */
    bool                        fClearTrapFlag;

    /** Whether \#UD needs to be intercepted (required by certain GIM providers). */
    bool                        fGIMTrapXcptUD;
    /** Whether paravirt. hypercalls are enabled. */
    bool                        fHypercallsEnabled;
    uint8_t                     u8Alignment0[3];

    /** World switch exit counter. */
    volatile uint32_t           cWorldSwitchExits;
    /** HM_CHANGED_* flags. */
    volatile uint32_t           fContextUseFlags;
    /** Id of the last cpu we were executing code on (NIL_RTCPUID for the first
     *  time). */
    RTCPUID                     idLastCpu;
    /** TLB flush count. */
    uint32_t                    cTlbFlushes;
    /** Current ASID in use by the VM. */
    uint32_t                    uCurrentAsid;
    /** An additional error code used for some gurus. */
    uint32_t                    u32HMError;
    /** Host's TSC_AUX MSR (used when RDTSCP doesn't cause VM-exits). */
    uint64_t                    u64HostTscAux;

    struct
    {
        /** Ring 0 handlers for VT-x. */
        PFNHMVMXSTARTVM             pfnStartVM;
#if HC_ARCH_BITS == 32
        uint32_t                    u32Alignment0;
#endif
        /** Current VMX_VMCS32_CTRL_PIN_EXEC. */
        uint32_t                    u32PinCtls;
        /** Current VMX_VMCS32_CTRL_PROC_EXEC. */
        uint32_t                    u32ProcCtls;
        /** Current VMX_VMCS32_CTRL_PROC_EXEC2. */
        uint32_t                    u32ProcCtls2;
        /** Current VMX_VMCS32_CTRL_EXIT. */
        uint32_t                    u32ExitCtls;
        /** Current VMX_VMCS32_CTRL_ENTRY. */
        uint32_t                    u32EntryCtls;

        /** Current CR0 mask. */
        uint32_t                    u32CR0Mask;
        /** Current CR4 mask. */
        uint32_t                    u32CR4Mask;
        /** Current exception bitmap. */
        uint32_t                    u32XcptBitmap;
        /** The updated-guest-state mask. */
        volatile uint32_t           fUpdatedGuestState;
        uint32_t                    u32Alignment1;

        /** Physical address of the VM control structure (VMCS). */
        RTHCPHYS                    HCPhysVmcs;
        /** R0 memory object for the VM control structure (VMCS). */
        RTR0MEMOBJ                  hMemObjVmcs;
        /** Virtual address of the VM control structure (VMCS). */
        R0PTRTYPE(void *)           pvVmcs;

        /** Physical address of the virtual APIC page for TPR caching. */
        RTHCPHYS                    HCPhysVirtApic;
        /** Padding. */
        R0PTRTYPE(void *)           pvAlignment0;
        /** Virtual address of the virtual APIC page for TPR caching. */
        R0PTRTYPE(uint8_t *)        pbVirtApic;

        /** Physical address of the MSR bitmap. */
        RTHCPHYS                    HCPhysMsrBitmap;
        /** R0 memory object for the MSR bitmap. */
        RTR0MEMOBJ                  hMemObjMsrBitmap;
        /** Virtual address of the MSR bitmap. */
        R0PTRTYPE(void *)           pvMsrBitmap;

        /** Physical address of the VM-entry MSR-load and VM-exit MSR-store area (used
         *  for guest MSRs). */
        RTHCPHYS                    HCPhysGuestMsr;
        /** R0 memory object of the VM-entry MSR-load and VM-exit MSR-store area
         *  (used for guest MSRs). */
        RTR0MEMOBJ                  hMemObjGuestMsr;
        /** Virtual address of the VM-entry MSR-load and VM-exit MSR-store area (used
         *  for guest MSRs). */
        R0PTRTYPE(void *)           pvGuestMsr;

        /** Physical address of the VM-exit MSR-load area (used for host MSRs). */
        RTHCPHYS                    HCPhysHostMsr;
        /** R0 memory object for the VM-exit MSR-load area (used for host MSRs). */
        RTR0MEMOBJ                  hMemObjHostMsr;
        /** Virtual address of the VM-exit MSR-load area (used for host MSRs). */
        R0PTRTYPE(void *)           pvHostMsr;

        /** Current EPTP. */
        RTHCPHYS                    HCPhysEPTP;

        /** Number of guest/host MSR pairs in the auto-load/store area. */
        uint32_t                    cMsrs;
        /** Whether the host MSR values are up-to-date in the auto-load/store area. */
        bool                        fUpdatedHostMsrs;
        uint8_t                     u8Alignment0[3];

        /** Host LSTAR MSR value to restore lazily while leaving VT-x. */
        uint64_t                    u64HostLStarMsr;
        /** Host STAR MSR value to restore lazily while leaving VT-x. */
        uint64_t                    u64HostStarMsr;
        /** Host SF_MASK MSR value to restore lazily while leaving VT-x. */
        uint64_t                    u64HostSFMaskMsr;
        /** Host KernelGS-Base MSR value to restore lazily while leaving VT-x. */
        uint64_t                    u64HostKernelGSBaseMsr;
        /** A mask of which MSRs have been swapped and need restoration. */
        uint32_t                    fLazyMsrs;
        uint32_t                    u32Alignment2;

        /** The cached APIC-base MSR used for identifying when to map the HC physical APIC-access page. */
        uint64_t                    u64MsrApicBase;
        /** Last use TSC offset value. (cached) */
        uint64_t                    u64TSCOffset;

        /** VMCS cache. */
        VMCSCACHE                   VMCSCache;

        /** Real-mode emulation state. */
        struct
        {
            X86DESCATTR             AttrCS;
            X86DESCATTR             AttrDS;
            X86DESCATTR             AttrES;
            X86DESCATTR             AttrFS;
            X86DESCATTR             AttrGS;
            X86DESCATTR             AttrSS;
            X86EFLAGS               Eflags;
            uint32_t                fRealOnV86Active;
        } RealMode;

        /** VT-x error-reporting (mainly for ring-3 propagation). */
        struct
        {
            uint64_t                u64VMCSPhys;
            uint32_t                u32VMCSRevision;
            uint32_t                u32InstrError;
            uint32_t                u32ExitReason;
            RTCPUID                 idEnteredCpu;
            RTCPUID                 idCurrentCpu;
            uint32_t                u32Alignment0;
        } LastError;

        /** Current state of the VMCS. */
        uint32_t                    uVmcsState;
        /** Which host-state bits to restore before being preempted. */
        uint32_t                    fRestoreHostFlags;
        /** The host-state restoration structure. */
        VMXRESTOREHOST              RestoreHost;

        /** Set if guest was executing in real mode (extra checks). */
        bool                        fWasInRealMode;
        /** Set if guest switched to 64-bit mode on a 32-bit host. */
        bool                        fSwitchedTo64on32;

        uint8_t                     u8Alignment1[6];
    } vmx;

    struct
    {
        /** Ring 0 handlers for VT-x. */
        PFNHMSVMVMRUN               pfnVMRun;
#if HC_ARCH_BITS == 32
        uint32_t                    u32Alignment0;
#endif

        /** Physical address of the host VMCB which holds additional host-state. */
        RTHCPHYS                    HCPhysVmcbHost;
        /** R0 memory object for the host VMCB which holds additional host-state. */
        RTR0MEMOBJ                  hMemObjVmcbHost;
        /** Padding. */
        R0PTRTYPE(void *)           pvPadding;

        /** Physical address of the guest VMCB. */
        RTHCPHYS                    HCPhysVmcb;
        /** R0 memory object for the guest VMCB. */
        RTR0MEMOBJ                  hMemObjVmcb;
        /** Pointer to the guest VMCB. */
        R0PTRTYPE(PSVMVMCB)         pVmcb;

        /** Physical address of the MSR bitmap (8 KB). */
        RTHCPHYS                    HCPhysMsrBitmap;
        /** R0 memory object for the MSR bitmap (8 KB). */
        RTR0MEMOBJ                  hMemObjMsrBitmap;
        /** Pointer to the MSR bitmap. */
        R0PTRTYPE(void *)           pvMsrBitmap;

        /** Whether VTPR with V_INTR_MASKING set is in effect, indicating
         *  we should check if the VTPR changed on every VM-exit. */
        bool                        fSyncVTpr;
        uint8_t                     u8Alignment0[7];

        /** Cache of the nested-guest's VMCB fields that we modify in order to run the
         *  nested-guest using AMD-V. This will be restored on \#VMEXIT. */
        SVMNESTEDVMCBCACHE          NstGstVmcbCache;
    } svm;

    /** Event injection state. */
    struct
    {
        uint32_t                    fPending;
        uint32_t                    u32ErrCode;
        uint32_t                    cbInstr;
        uint32_t                    u32Padding; /**< Explicit alignment padding. */
        uint64_t                    u64IntInfo;
        RTGCUINTPTR                 GCPtrFaultAddress;
    } Event;

    /** IO Block emulation state. */
    struct
    {
        bool                    fEnabled;
        uint8_t                 u8Align[7];

        /** RIP at the start of the io code we wish to emulate in the recompiler. */
        RTGCPTR                 GCPtrFunctionEip;

        uint64_t                cr0;
    } EmulateIoBlock;

    /* */
    struct
    {
        /** Pending IO operation type. */
        HMPENDINGIO             enmType;
        uint32_t                u32Alignment0;
        RTGCPTR                 GCPtrRip;
        RTGCPTR                 GCPtrRipNext;
        union
        {
            struct
            {
                uint32_t        uPort;
                uint32_t        uAndVal;
                uint32_t        cbSize;
            } Port;
            uint64_t            aRaw[2];
        } s;
    } PendingIO;

    /** The PAE PDPEs used with Nested Paging (only valid when
     *  VMCPU_FF_HM_UPDATE_PAE_PDPES is set). */
    X86PDPE                 aPdpes[4];

    /** Current shadow paging mode. */
    PGMMODE                 enmShadowMode;

    /** The CPU ID of the CPU currently owning the VMCS. Set in
     * HMR0Enter and cleared in HMR0Leave. */
    RTCPUID                 idEnteredCpu;

    /** VT-x/AMD-V VM-exit/\#VMXEXIT history, circular array. */
    uint16_t                auExitHistory[31];
    /** The index of the next free slot in the history array. */
    uint16_t                idxExitHistoryFree;

    /** For saving stack space, the disassembler state is allocated here instead of
     * on the stack. */
    DISCPUSTATE             DisState;

    STAMPROFILEADV          StatEntry;
    STAMPROFILEADV          StatExit1;
    STAMPROFILEADV          StatExit2;
    STAMPROFILEADV          StatExitIO;
    STAMPROFILEADV          StatExitMovCRx;
    STAMPROFILEADV          StatExitXcptNmi;
    STAMPROFILEADV          StatLoadGuestState;
    STAMPROFILEADV          StatInGC;

#if HC_ARCH_BITS == 32 && defined(VBOX_ENABLE_64_BITS_GUESTS)
    STAMPROFILEADV          StatWorldSwitch3264;
#endif
    STAMPROFILEADV          StatPoke;
    STAMPROFILEADV          StatSpinPoke;
    STAMPROFILEADV          StatSpinPokeFailed;

    STAMCOUNTER             StatInjectInterrupt;
    STAMCOUNTER             StatInjectXcpt;
    STAMCOUNTER             StatInjectPendingReflect;
    STAMCOUNTER             StatInjectPendingInterpret;

    STAMCOUNTER             StatExitAll;
    STAMCOUNTER             StatExitShadowNM;
    STAMCOUNTER             StatExitGuestNM;
    STAMCOUNTER             StatExitShadowPF;       /**< Misleading, currently used for MMIO \#PFs as well. */
    STAMCOUNTER             StatExitShadowPFEM;
    STAMCOUNTER             StatExitGuestPF;
    STAMCOUNTER             StatExitGuestUD;
    STAMCOUNTER             StatExitGuestSS;
    STAMCOUNTER             StatExitGuestNP;
    STAMCOUNTER             StatExitGuestTS;
    STAMCOUNTER             StatExitGuestGP;
    STAMCOUNTER             StatExitGuestDE;
    STAMCOUNTER             StatExitGuestDB;
    STAMCOUNTER             StatExitGuestMF;
    STAMCOUNTER             StatExitGuestBP;
    STAMCOUNTER             StatExitGuestXF;
    STAMCOUNTER             StatExitGuestXcpUnk;
    STAMCOUNTER             StatExitInvlpg;
    STAMCOUNTER             StatExitInvd;
    STAMCOUNTER             StatExitWbinvd;
    STAMCOUNTER             StatExitPause;
    STAMCOUNTER             StatExitCpuid;
    STAMCOUNTER             StatExitRdtsc;
    STAMCOUNTER             StatExitRdtscp;
    STAMCOUNTER             StatExitRdpmc;
    STAMCOUNTER             StatExitVmcall;
    STAMCOUNTER             StatExitRdrand;
    STAMCOUNTER             StatExitCli;
    STAMCOUNTER             StatExitSti;
    STAMCOUNTER             StatExitPushf;
    STAMCOUNTER             StatExitPopf;
    STAMCOUNTER             StatExitIret;
    STAMCOUNTER             StatExitInt;
    STAMCOUNTER             StatExitCRxWrite[16];
    STAMCOUNTER             StatExitCRxRead[16];
    STAMCOUNTER             StatExitDRxWrite;
    STAMCOUNTER             StatExitDRxRead;
    STAMCOUNTER             StatExitRdmsr;
    STAMCOUNTER             StatExitWrmsr;
    STAMCOUNTER             StatExitClts;
    STAMCOUNTER             StatExitXdtrAccess;
    STAMCOUNTER             StatExitHlt;
    STAMCOUNTER             StatExitMwait;
    STAMCOUNTER             StatExitMonitor;
    STAMCOUNTER             StatExitLmsw;
    STAMCOUNTER             StatExitIOWrite;
    STAMCOUNTER             StatExitIORead;
    STAMCOUNTER             StatExitIOStringWrite;
    STAMCOUNTER             StatExitIOStringRead;
    STAMCOUNTER             StatExitIntWindow;
    STAMCOUNTER             StatExitExtInt;
    STAMCOUNTER             StatExitHostNmiInGC;
    STAMCOUNTER             StatExitPreemptTimer;
    STAMCOUNTER             StatExitTprBelowThreshold;
    STAMCOUNTER             StatExitTaskSwitch;
    STAMCOUNTER             StatExitMtf;
    STAMCOUNTER             StatExitApicAccess;
    STAMCOUNTER             StatPendingHostIrq;

    STAMCOUNTER             StatFlushPage;
    STAMCOUNTER             StatFlushPageManual;
    STAMCOUNTER             StatFlushPhysPageManual;
    STAMCOUNTER             StatFlushTlb;
    STAMCOUNTER             StatFlushTlbManual;
    STAMCOUNTER             StatFlushTlbWorldSwitch;
    STAMCOUNTER             StatNoFlushTlbWorldSwitch;
    STAMCOUNTER             StatFlushEntire;
    STAMCOUNTER             StatFlushAsid;
    STAMCOUNTER             StatFlushNestedPaging;
    STAMCOUNTER             StatFlushTlbInvlpgVirt;
    STAMCOUNTER             StatFlushTlbInvlpgPhys;
    STAMCOUNTER             StatTlbShootdown;
    STAMCOUNTER             StatTlbShootdownFlush;

    STAMCOUNTER             StatSwitchTprMaskedIrq;
    STAMCOUNTER             StatSwitchGuestIrq;
    STAMCOUNTER             StatSwitchHmToR3FF;
    STAMCOUNTER             StatSwitchExitToR3;
    STAMCOUNTER             StatSwitchLongJmpToR3;
    STAMCOUNTER             StatSwitchMaxResumeLoops;
    STAMCOUNTER             StatSwitchHltToR3;
    STAMCOUNTER             StatSwitchApicAccessToR3;
    STAMCOUNTER             StatSwitchPreempt;
    STAMCOUNTER             StatSwitchPreemptSaveHostState;

    STAMCOUNTER             StatTscParavirt;
    STAMCOUNTER             StatTscOffset;
    STAMCOUNTER             StatTscIntercept;

    STAMCOUNTER             StatExitReasonNpf;
    STAMCOUNTER             StatDRxArmed;
    STAMCOUNTER             StatDRxContextSwitch;
    STAMCOUNTER             StatDRxIoCheck;

    STAMCOUNTER             StatLoadMinimal;
    STAMCOUNTER             StatLoadFull;

    STAMCOUNTER             StatVmxCheckBadRmSelBase;
    STAMCOUNTER             StatVmxCheckBadRmSelLimit;
    STAMCOUNTER             StatVmxCheckRmOk;

    STAMCOUNTER             StatVmxCheckBadSel;
    STAMCOUNTER             StatVmxCheckBadRpl;
    STAMCOUNTER             StatVmxCheckBadLdt;
    STAMCOUNTER             StatVmxCheckBadTr;
    STAMCOUNTER             StatVmxCheckPmOk;

#if HC_ARCH_BITS == 32 && defined(VBOX_ENABLE_64_BITS_GUESTS)
    STAMCOUNTER             StatFpu64SwitchBack;
    STAMCOUNTER             StatDebug64SwitchBack;
#endif

#ifdef VBOX_WITH_STATISTICS
    R3PTRTYPE(PSTAMCOUNTER) paStatExitReason;
    R0PTRTYPE(PSTAMCOUNTER) paStatExitReasonR0;
    R3PTRTYPE(PSTAMCOUNTER) paStatInjectedIrqs;
    R0PTRTYPE(PSTAMCOUNTER) paStatInjectedIrqsR0;
#endif
#ifdef HM_PROFILE_EXIT_DISPATCH
    STAMPROFILEADV          StatExitDispatch;
#endif
} HMCPU;
/** Pointer to HM VMCPU instance data. */
typedef HMCPU *PHMCPU;
AssertCompileMemberAlignment(HMCPU, vmx, 8);
AssertCompileMemberAlignment(HMCPU, svm, 8);
AssertCompileMemberAlignment(HMCPU, Event, 8);

#ifdef IN_RING0
VMMR0_INT_DECL(PHMGLOBALCPUINFO) hmR0GetCurrentCpu(void);

# ifdef VBOX_STRICT
VMMR0_INT_DECL(void) hmR0DumpRegs(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx);
VMMR0_INT_DECL(void) hmR0DumpDescriptor(PCX86DESCHC pDesc, RTSEL Sel, const char *pszMsg);
# else
#  define hmR0DumpRegs(a, b ,c)          do { } while (0)
#  define hmR0DumpDescriptor(a, b, c)    do { } while (0)
# endif /* VBOX_STRICT */

# ifdef VBOX_WITH_KERNEL_USING_XMM
DECLASM(int) hmR0VMXStartVMWrapXMM(RTHCUINT fResume, PCPUMCTX pCtx, PVMCSCACHE pCache, PVM pVM, PVMCPU pVCpu, PFNHMVMXSTARTVM pfnStartVM);
DECLASM(int) hmR0SVMRunWrapXMM(RTHCPHYS pVmcbHostPhys, RTHCPHYS pVmcbPhys, PCPUMCTX pCtx, PVM pVM, PVMCPU pVCpu, PFNHMSVMVMRUN pfnVMRun);
# endif

#endif /* IN_RING0 */

/** @} */

RT_C_DECLS_END

#endif

