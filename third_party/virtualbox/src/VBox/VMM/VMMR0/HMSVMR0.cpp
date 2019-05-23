/* $Id: HMSVMR0.cpp $ */
/** @file
 * HM SVM (AMD-V) - Host Context Ring-0.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_HM
#include <iprt/asm-amd64-x86.h>
#include <iprt/thread.h>

#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/iem.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/tm.h>
#include <VBox/vmm/gim.h>
#include <VBox/vmm/apic.h>
#include "HMInternal.h"
#include <VBox/vmm/vm.h>
#include "HMSVMR0.h"
#include "dtrace/VBoxVMM.h"

#define HMSVM_USE_IEM_EVENT_REFLECTION
#ifdef DEBUG_ramshankar
# define HMSVM_SYNC_FULL_GUEST_STATE
# define HMSVM_ALWAYS_TRAP_ALL_XCPTS
# define HMSVM_ALWAYS_TRAP_PF
# define HMSVM_ALWAYS_TRAP_TASK_SWITCH
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#ifdef VBOX_WITH_STATISTICS
# define HMSVM_EXITCODE_STAM_COUNTER_INC(u64ExitCode) do { \
        STAM_COUNTER_INC(&pVCpu->hm.s.StatExitAll); \
        if ((u64ExitCode) == SVM_EXIT_NPF) \
            STAM_COUNTER_INC(&pVCpu->hm.s.StatExitReasonNpf); \
        else \
            STAM_COUNTER_INC(&pVCpu->hm.s.paStatExitReasonR0[(u64ExitCode) & MASK_EXITREASON_STAT]); \
        } while (0)
#else
# define HMSVM_EXITCODE_STAM_COUNTER_INC(u64ExitCode) do { } while (0)
#endif

/** If we decide to use a function table approach this can be useful to
 *  switch to a "static DECLCALLBACK(int)". */
#define HMSVM_EXIT_DECL                 static int

/** Macro for checking and returning from the using function for
 * \#VMEXIT intercepts that maybe caused during delivering of another
 * event in the guest. */
#define HMSVM_CHECK_EXIT_DUE_TO_EVENT_DELIVERY() \
    do \
    { \
        int rc = hmR0SvmCheckExitDueToEventDelivery(pVCpu, pCtx, pSvmTransient); \
        if (RT_LIKELY(rc == VINF_SUCCESS)) { /* likely */ } \
        else if (rc == VINF_HM_DOUBLE_FAULT) \
            return VINF_SUCCESS; \
        else \
            return rc; \
    } while (0)

/** Macro for upgrading a @a a_rc to VINF_EM_DBG_STEPPED after emulating an
 * instruction that exited. */
#define HMSVM_CHECK_SINGLE_STEP(a_pVCpu, a_rc) \
    do { \
        if ((a_pVCpu)->hm.s.fSingleInstruction && (a_rc) == VINF_SUCCESS) \
            (a_rc) = VINF_EM_DBG_STEPPED; \
    } while (0)

/** Assert that preemption is disabled or covered by thread-context hooks. */
#define HMSVM_ASSERT_PREEMPT_SAFE()           Assert(   VMMR0ThreadCtxHookIsEnabled(pVCpu) \
                                                     || !RTThreadPreemptIsEnabled(NIL_RTTHREAD));

/** Assert that we haven't migrated CPUs when thread-context hooks are not
 *  used. */
#define HMSVM_ASSERT_CPU_SAFE()               AssertMsg(   VMMR0ThreadCtxHookIsEnabled(pVCpu) \
                                                        || pVCpu->hm.s.idEnteredCpu == RTMpCpuId(), \
                                                        ("Illegal migration! Entered on CPU %u Current %u\n", \
                                                        pVCpu->hm.s.idEnteredCpu, RTMpCpuId()));

/**
 * Exception bitmap mask for all contributory exceptions.
 *
 * Page fault is deliberately excluded here as it's conditional as to whether
 * it's contributory or benign. Page faults are handled separately.
 */
#define HMSVM_CONTRIBUTORY_XCPT_MASK  (  RT_BIT(X86_XCPT_GP) | RT_BIT(X86_XCPT_NP) | RT_BIT(X86_XCPT_SS) | RT_BIT(X86_XCPT_TS) \
                                       | RT_BIT(X86_XCPT_DE))

/**
 *  Mandatory/unconditional guest control intercepts.
 */
#define HMSVM_MANDATORY_GUEST_CTRL_INTERCEPTS           (  SVM_CTRL_INTERCEPT_INTR        \
                                                         | SVM_CTRL_INTERCEPT_NMI         \
                                                         | SVM_CTRL_INTERCEPT_INIT        \
                                                         | SVM_CTRL_INTERCEPT_RDPMC       \
                                                         | SVM_CTRL_INTERCEPT_CPUID       \
                                                         | SVM_CTRL_INTERCEPT_RSM         \
                                                         | SVM_CTRL_INTERCEPT_HLT         \
                                                         | SVM_CTRL_INTERCEPT_IOIO_PROT   \
                                                         | SVM_CTRL_INTERCEPT_MSR_PROT    \
                                                         | SVM_CTRL_INTERCEPT_INVLPGA     \
                                                         | SVM_CTRL_INTERCEPT_SHUTDOWN    \
                                                         | SVM_CTRL_INTERCEPT_FERR_FREEZE \
                                                         | SVM_CTRL_INTERCEPT_VMRUN       \
                                                         | SVM_CTRL_INTERCEPT_VMMCALL     \
                                                         | SVM_CTRL_INTERCEPT_VMLOAD      \
                                                         | SVM_CTRL_INTERCEPT_VMSAVE      \
                                                         | SVM_CTRL_INTERCEPT_STGI        \
                                                         | SVM_CTRL_INTERCEPT_CLGI        \
                                                         | SVM_CTRL_INTERCEPT_SKINIT      \
                                                         | SVM_CTRL_INTERCEPT_WBINVD      \
                                                         | SVM_CTRL_INTERCEPT_MONITOR     \
                                                         | SVM_CTRL_INTERCEPT_MWAIT       \
                                                         | SVM_CTRL_INTERCEPT_XSETBV)

/**
 *  Mandatory/unconditional nested-guest control intercepts.
 */
#define HMSVM_MANDATORY_NESTED_GUEST_CTRL_INTERCEPTS    (  HMSVM_MANDATORY_GUEST_CTRL_INTERCEPTS \
                                                         | SVM_CTRL_INTERCEPT_SMI)

/** @name VMCB Clean Bits.
 *
 * These flags are used for VMCB-state caching. A set VMCB Clean bit indicates
 * AMD-V doesn't need to reload the corresponding value(s) from the VMCB in
 * memory.
 *
 * @{ */
/** All intercepts vectors, TSC offset, PAUSE filter counter. */
#define HMSVM_VMCB_CLEAN_INTERCEPTS             RT_BIT(0)
/** I/O permission bitmap, MSR permission bitmap. */
#define HMSVM_VMCB_CLEAN_IOPM_MSRPM             RT_BIT(1)
/** ASID.  */
#define HMSVM_VMCB_CLEAN_ASID                   RT_BIT(2)
/** TRP: V_TPR, V_IRQ, V_INTR_PRIO, V_IGN_TPR, V_INTR_MASKING,
V_INTR_VECTOR. */
#define HMSVM_VMCB_CLEAN_TPR                    RT_BIT(3)
/** Nested Paging: Nested CR3 (nCR3), PAT. */
#define HMSVM_VMCB_CLEAN_NP                     RT_BIT(4)
/** Control registers (CR0, CR3, CR4, EFER). */
#define HMSVM_VMCB_CLEAN_CRX_EFER               RT_BIT(5)
/** Debug registers (DR6, DR7). */
#define HMSVM_VMCB_CLEAN_DRX                    RT_BIT(6)
/** GDT, IDT limit and base. */
#define HMSVM_VMCB_CLEAN_DT                     RT_BIT(7)
/** Segment register: CS, SS, DS, ES limit and base. */
#define HMSVM_VMCB_CLEAN_SEG                    RT_BIT(8)
/** CR2.*/
#define HMSVM_VMCB_CLEAN_CR2                    RT_BIT(9)
/** Last-branch record (DbgCtlMsr, br_from, br_to, lastint_from, lastint_to) */
#define HMSVM_VMCB_CLEAN_LBR                    RT_BIT(10)
/** AVIC (AVIC APIC_BAR; AVIC APIC_BACKING_PAGE, AVIC
PHYSICAL_TABLE and AVIC LOGICAL_TABLE Pointers). */
#define HMSVM_VMCB_CLEAN_AVIC                   RT_BIT(11)
/** Mask of all valid VMCB Clean bits. */
#define HMSVM_VMCB_CLEAN_ALL                    (  HMSVM_VMCB_CLEAN_INTERCEPTS  \
                                                 | HMSVM_VMCB_CLEAN_IOPM_MSRPM  \
                                                 | HMSVM_VMCB_CLEAN_ASID        \
                                                 | HMSVM_VMCB_CLEAN_TPR         \
                                                 | HMSVM_VMCB_CLEAN_NP          \
                                                 | HMSVM_VMCB_CLEAN_CRX_EFER    \
                                                 | HMSVM_VMCB_CLEAN_DRX         \
                                                 | HMSVM_VMCB_CLEAN_DT          \
                                                 | HMSVM_VMCB_CLEAN_SEG         \
                                                 | HMSVM_VMCB_CLEAN_CR2         \
                                                 | HMSVM_VMCB_CLEAN_LBR         \
                                                 | HMSVM_VMCB_CLEAN_AVIC)
/** @} */

/** @name SVM transient.
 *
 * A state structure for holding miscellaneous information across AMD-V
 * VMRUN/\#VMEXIT operation, restored after the transition.
 *
 * @{ */
typedef struct SVMTRANSIENT
{
    /** The host's rflags/eflags. */
    RTCCUINTREG     fEFlags;
#if HC_ARCH_BITS == 32
    uint32_t        u32Alignment0;
#endif

    /** The \#VMEXIT exit code (the EXITCODE field in the VMCB). */
    uint64_t        u64ExitCode;
    /** The guest's TPR value used for TPR shadowing. */
    uint8_t         u8GuestTpr;
    /** Alignment. */
    uint8_t         abAlignment0[7];

    /** Whether the guest debug state was active at the time of \#VMEXIT. */
    bool            fWasGuestDebugStateActive;
    /** Whether the hyper debug state was active at the time of \#VMEXIT. */
    bool            fWasHyperDebugStateActive;
    /** Whether the TSC offset mode needs to be updated. */
    bool            fUpdateTscOffsetting;
    /** Whether the TSC_AUX MSR needs restoring on \#VMEXIT. */
    bool            fRestoreTscAuxMsr;
    /** Whether the \#VMEXIT was caused by a page-fault during delivery of a
     *  contributary exception or a page-fault. */
    bool            fVectoringDoublePF;
    /** Whether the \#VMEXIT was caused by a page-fault during delivery of an
     *  external interrupt or NMI. */
    bool            fVectoringPF;
} SVMTRANSIENT, *PSVMTRANSIENT;
AssertCompileMemberAlignment(SVMTRANSIENT, u64ExitCode,             sizeof(uint64_t));
AssertCompileMemberAlignment(SVMTRANSIENT, fWasGuestDebugStateActive, sizeof(uint64_t));
/** @}  */

/**
 * MSRPM (MSR permission bitmap) read permissions (for guest RDMSR).
 */
typedef enum SVMMSREXITREAD
{
    /** Reading this MSR causes a \#VMEXIT. */
    SVMMSREXIT_INTERCEPT_READ = 0xb,
    /** Reading this MSR does not cause a \#VMEXIT. */
    SVMMSREXIT_PASSTHRU_READ
} SVMMSREXITREAD;

/**
 * MSRPM (MSR permission bitmap) write permissions (for guest WRMSR).
 */
typedef enum SVMMSREXITWRITE
{
    /** Writing to this MSR causes a \#VMEXIT. */
    SVMMSREXIT_INTERCEPT_WRITE = 0xd,
    /** Writing to this MSR does not cause a \#VMEXIT. */
    SVMMSREXIT_PASSTHRU_WRITE
} SVMMSREXITWRITE;

/**
 * SVM \#VMEXIT handler.
 *
 * @returns VBox status code.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pMixedCtx       Pointer to the guest-CPU context.
 * @param   pSvmTransient   Pointer to the SVM-transient structure.
 */
typedef int FNSVMEXITHANDLER(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient);


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void hmR0SvmSetMsrPermission(PSVMVMCB pVmcb, uint8_t *pbMsrBitmap, unsigned uMsr, SVMMSREXITREAD enmRead,
                                    SVMMSREXITWRITE enmWrite);
static void hmR0SvmPendingEventToTrpmTrap(PVMCPU pVCpu);
static void hmR0SvmLeave(PVMCPU pVCpu);

/** @name \#VMEXIT handlers.
 * @{
 */
static FNSVMEXITHANDLER hmR0SvmExitIntr;
static FNSVMEXITHANDLER hmR0SvmExitWbinvd;
static FNSVMEXITHANDLER hmR0SvmExitInvd;
static FNSVMEXITHANDLER hmR0SvmExitCpuid;
static FNSVMEXITHANDLER hmR0SvmExitRdtsc;
static FNSVMEXITHANDLER hmR0SvmExitRdtscp;
static FNSVMEXITHANDLER hmR0SvmExitRdpmc;
static FNSVMEXITHANDLER hmR0SvmExitInvlpg;
static FNSVMEXITHANDLER hmR0SvmExitHlt;
static FNSVMEXITHANDLER hmR0SvmExitMonitor;
static FNSVMEXITHANDLER hmR0SvmExitMwait;
static FNSVMEXITHANDLER hmR0SvmExitShutdown;
static FNSVMEXITHANDLER hmR0SvmExitUnexpected;
static FNSVMEXITHANDLER hmR0SvmExitReadCRx;
static FNSVMEXITHANDLER hmR0SvmExitWriteCRx;
static FNSVMEXITHANDLER hmR0SvmExitMsr;
static FNSVMEXITHANDLER hmR0SvmExitReadDRx;
static FNSVMEXITHANDLER hmR0SvmExitWriteDRx;
static FNSVMEXITHANDLER hmR0SvmExitXsetbv;
static FNSVMEXITHANDLER hmR0SvmExitIOInstr;
static FNSVMEXITHANDLER hmR0SvmExitNestedPF;
static FNSVMEXITHANDLER hmR0SvmExitVIntr;
static FNSVMEXITHANDLER hmR0SvmExitTaskSwitch;
static FNSVMEXITHANDLER hmR0SvmExitVmmCall;
static FNSVMEXITHANDLER hmR0SvmExitPause;
static FNSVMEXITHANDLER hmR0SvmExitIret;
static FNSVMEXITHANDLER hmR0SvmExitXcptPF;
static FNSVMEXITHANDLER hmR0SvmExitXcptUD;
static FNSVMEXITHANDLER hmR0SvmExitXcptMF;
static FNSVMEXITHANDLER hmR0SvmExitXcptDB;
static FNSVMEXITHANDLER hmR0SvmExitXcptAC;
static FNSVMEXITHANDLER hmR0SvmExitXcptBP;
#ifdef VBOX_WITH_NESTED_HWVIRT
static FNSVMEXITHANDLER hmR0SvmExitXcptPFNested;
static FNSVMEXITHANDLER hmR0SvmExitClgi;
static FNSVMEXITHANDLER hmR0SvmExitStgi;
static FNSVMEXITHANDLER hmR0SvmExitVmload;
static FNSVMEXITHANDLER hmR0SvmExitVmsave;
static FNSVMEXITHANDLER hmR0SvmExitInvlpga;
static FNSVMEXITHANDLER hmR0SvmExitVmrun;
static FNSVMEXITHANDLER hmR0SvmNestedExitIret;
static FNSVMEXITHANDLER hmR0SvmNestedExitXcptDB;
static FNSVMEXITHANDLER hmR0SvmNestedExitXcptBP;
#endif
/** @} */

static int hmR0SvmHandleExit(PVMCPU pVCpu, PCPUMCTX pMixedCtx, PSVMTRANSIENT pSvmTransient);
#ifdef VBOX_WITH_NESTED_HWVIRT
static int hmR0SvmHandleExitNested(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient);
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Ring-0 memory object for the IO bitmap. */
RTR0MEMOBJ                  g_hMemObjIOBitmap = NIL_RTR0MEMOBJ;
/** Physical address of the IO bitmap. */
RTHCPHYS                    g_HCPhysIOBitmap  = 0;
/** Pointer to the IO bitmap. */
R0PTRTYPE(void *)           g_pvIOBitmap      = NULL;

#ifdef VBOX_WITH_NESTED_HWVIRT
/** Ring-0 memory object for the nested-guest MSRPM bitmap. */
RTR0MEMOBJ                  g_hMemObjNstGstMsrBitmap = NIL_RTR0MEMOBJ;
/** Physical address of the nested-guest MSRPM bitmap. */
RTHCPHYS                    g_HCPhysNstGstMsrBitmap  = 0;
/** Pointer to the  nested-guest MSRPM bitmap. */
R0PTRTYPE(void *)           g_pvNstGstMsrBitmap      = NULL;
#endif

/**
 * Sets up and activates AMD-V on the current CPU.
 *
 * @returns VBox status code.
 * @param   pCpu            Pointer to the CPU info struct.
 * @param   pVM             The cross context VM structure. Can be
 *                          NULL after a resume!
 * @param   pvCpuPage       Pointer to the global CPU page.
 * @param   HCPhysCpuPage   Physical address of the global CPU page.
 * @param   fEnabledByHost  Whether the host OS has already initialized AMD-V.
 * @param   pvArg           Unused on AMD-V.
 */
VMMR0DECL(int) SVMR0EnableCpu(PHMGLOBALCPUINFO pCpu, PVM pVM, void *pvCpuPage, RTHCPHYS HCPhysCpuPage, bool fEnabledByHost,
                              void *pvArg)
{
    Assert(!fEnabledByHost);
    Assert(HCPhysCpuPage && HCPhysCpuPage != NIL_RTHCPHYS);
    Assert(RT_ALIGN_T(HCPhysCpuPage, _4K, RTHCPHYS) == HCPhysCpuPage);
    Assert(pvCpuPage); NOREF(pvCpuPage);
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));

    NOREF(pvArg);
    NOREF(fEnabledByHost);

    /* Paranoid: Disable interrupt as, in theory, interrupt handlers might mess with EFER. */
    RTCCUINTREG fEFlags = ASMIntDisableFlags();

    /*
     * We must turn on AMD-V and setup the host state physical address, as those MSRs are per CPU.
     */
    uint64_t u64HostEfer = ASMRdMsr(MSR_K6_EFER);
    if (u64HostEfer & MSR_K6_EFER_SVME)
    {
        /* If the VBOX_HWVIRTEX_IGNORE_SVM_IN_USE is active, then we blindly use AMD-V. */
        if (   pVM
            && pVM->hm.s.svm.fIgnoreInUseError)
        {
            pCpu->fIgnoreAMDVInUseError = true;
        }

        if (!pCpu->fIgnoreAMDVInUseError)
        {
            ASMSetFlags(fEFlags);
            return VERR_SVM_IN_USE;
        }
    }

    /* Turn on AMD-V in the EFER MSR. */
    ASMWrMsr(MSR_K6_EFER, u64HostEfer | MSR_K6_EFER_SVME);

    /* Write the physical page address where the CPU will store the host state while executing the VM. */
    ASMWrMsr(MSR_K8_VM_HSAVE_PA, HCPhysCpuPage);

    /* Restore interrupts. */
    ASMSetFlags(fEFlags);

    /*
     * Theoretically, other hypervisors may have used ASIDs, ideally we should flush all non-zero ASIDs
     * when enabling SVM. AMD doesn't have an SVM instruction to flush all ASIDs (flushing is done
     * upon VMRUN). Therefore, flag that we need to flush the TLB entirely with before executing any
     * guest code.
     */
    pCpu->fFlushAsidBeforeUse = true;

    /*
     * Ensure each VCPU scheduled on this CPU gets a new ASID on resume. See @bugref{6255}.
     */
    ++pCpu->cTlbFlushes;

    return VINF_SUCCESS;
}


/**
 * Deactivates AMD-V on the current CPU.
 *
 * @returns VBox status code.
 * @param   pCpu            Pointer to the CPU info struct.
 * @param   pvCpuPage       Pointer to the global CPU page.
 * @param   HCPhysCpuPage   Physical address of the global CPU page.
 */
VMMR0DECL(int) SVMR0DisableCpu(PHMGLOBALCPUINFO pCpu, void *pvCpuPage, RTHCPHYS HCPhysCpuPage)
{
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    AssertReturn(   HCPhysCpuPage
                 && HCPhysCpuPage != NIL_RTHCPHYS, VERR_INVALID_PARAMETER);
    AssertReturn(pvCpuPage, VERR_INVALID_PARAMETER);
    NOREF(pCpu);

    /* Paranoid: Disable interrupts as, in theory, interrupt handlers might mess with EFER. */
    RTCCUINTREG fEFlags = ASMIntDisableFlags();

    /* Turn off AMD-V in the EFER MSR. */
    uint64_t u64HostEfer = ASMRdMsr(MSR_K6_EFER);
    ASMWrMsr(MSR_K6_EFER, u64HostEfer & ~MSR_K6_EFER_SVME);

    /* Invalidate host state physical address. */
    ASMWrMsr(MSR_K8_VM_HSAVE_PA, 0);

    /* Restore interrupts. */
    ASMSetFlags(fEFlags);

    return VINF_SUCCESS;
}


/**
 * Does global AMD-V initialization (called during module initialization).
 *
 * @returns VBox status code.
 */
VMMR0DECL(int) SVMR0GlobalInit(void)
{
    /*
     * Allocate 12 KB for the IO bitmap. Since this is non-optional and we always intercept all IO accesses, it's done
     * once globally here instead of per-VM.
     */
    Assert(g_hMemObjIOBitmap == NIL_RTR0MEMOBJ);
    int rc = RTR0MemObjAllocCont(&g_hMemObjIOBitmap, SVM_IOPM_PAGES << X86_PAGE_4K_SHIFT, false /* fExecutable */);
    if (RT_FAILURE(rc))
        return rc;

    g_pvIOBitmap     = RTR0MemObjAddress(g_hMemObjIOBitmap);
    g_HCPhysIOBitmap = RTR0MemObjGetPagePhysAddr(g_hMemObjIOBitmap, 0 /* iPage */);

    /* Set all bits to intercept all IO accesses. */
    ASMMemFill32(g_pvIOBitmap, SVM_IOPM_PAGES << X86_PAGE_4K_SHIFT, UINT32_C(0xffffffff));

#ifdef VBOX_WITH_NESTED_HWVIRT
    /*
     * Allocate 8 KB for the MSR permission bitmap for the nested-guest.
     */
    Assert(g_hMemObjNstGstMsrBitmap == NIL_RTR0MEMOBJ);
    rc = RTR0MemObjAllocCont(&g_hMemObjNstGstMsrBitmap, SVM_MSRPM_PAGES << X86_PAGE_4K_SHIFT, false /* fExecutable */);
    if (RT_FAILURE(rc))
        return rc;

    g_pvNstGstMsrBitmap     = RTR0MemObjAddress(g_hMemObjNstGstMsrBitmap);
    g_HCPhysNstGstMsrBitmap = RTR0MemObjGetPagePhysAddr(g_hMemObjNstGstMsrBitmap, 0 /* iPage */);

    /* Set all bits to intercept all MSR accesses. */
    ASMMemFill32(g_pvNstGstMsrBitmap, SVM_MSRPM_PAGES << X86_PAGE_4K_SHIFT, UINT32_C(0xffffffff));
#endif

    return VINF_SUCCESS;
}


/**
 * Does global AMD-V termination (called during module termination).
 */
VMMR0DECL(void) SVMR0GlobalTerm(void)
{
    if (g_hMemObjIOBitmap != NIL_RTR0MEMOBJ)
    {
        RTR0MemObjFree(g_hMemObjIOBitmap, true /* fFreeMappings */);
        g_pvIOBitmap      = NULL;
        g_HCPhysIOBitmap  = 0;
        g_hMemObjIOBitmap = NIL_RTR0MEMOBJ;
    }

#ifdef VBOX_WITH_NESTED_HWVIRT
    if (g_hMemObjNstGstMsrBitmap != NIL_RTR0MEMOBJ)
    {
        RTR0MemObjFree(g_hMemObjNstGstMsrBitmap, true /* fFreeMappings */);
        g_pvNstGstMsrBitmap      = NULL;
        g_HCPhysNstGstMsrBitmap  = 0;
        g_hMemObjNstGstMsrBitmap = NIL_RTR0MEMOBJ;
    }
#endif
}


/**
 * Frees any allocated per-VCPU structures for a VM.
 *
 * @param   pVM     The cross context VM structure.
 */
DECLINLINE(void) hmR0SvmFreeStructs(PVM pVM)
{
    for (uint32_t i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];
        AssertPtr(pVCpu);

        if (pVCpu->hm.s.svm.hMemObjVmcbHost != NIL_RTR0MEMOBJ)
        {
            RTR0MemObjFree(pVCpu->hm.s.svm.hMemObjVmcbHost, false);
            pVCpu->hm.s.svm.HCPhysVmcbHost   = 0;
            pVCpu->hm.s.svm.hMemObjVmcbHost  = NIL_RTR0MEMOBJ;
        }

        if (pVCpu->hm.s.svm.hMemObjVmcb != NIL_RTR0MEMOBJ)
        {
            RTR0MemObjFree(pVCpu->hm.s.svm.hMemObjVmcb, false);
            pVCpu->hm.s.svm.pVmcb            = NULL;
            pVCpu->hm.s.svm.HCPhysVmcb       = 0;
            pVCpu->hm.s.svm.hMemObjVmcb      = NIL_RTR0MEMOBJ;
        }

        if (pVCpu->hm.s.svm.hMemObjMsrBitmap != NIL_RTR0MEMOBJ)
        {
            RTR0MemObjFree(pVCpu->hm.s.svm.hMemObjMsrBitmap, false);
            pVCpu->hm.s.svm.pvMsrBitmap      = NULL;
            pVCpu->hm.s.svm.HCPhysMsrBitmap  = 0;
            pVCpu->hm.s.svm.hMemObjMsrBitmap = NIL_RTR0MEMOBJ;
        }
    }
}


/**
 * Does per-VM AMD-V initialization.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR0DECL(int) SVMR0InitVM(PVM pVM)
{
    int rc = VERR_INTERNAL_ERROR_5;

    /*
     * Check for an AMD CPU erratum which requires us to flush the TLB before every world-switch.
     */
    uint32_t u32Family;
    uint32_t u32Model;
    uint32_t u32Stepping;
    if (HMAmdIsSubjectToErratum170(&u32Family, &u32Model, &u32Stepping))
    {
        Log4(("SVMR0InitVM: AMD cpu with erratum 170 family %#x model %#x stepping %#x\n", u32Family, u32Model, u32Stepping));
        pVM->hm.s.svm.fAlwaysFlushTLB = true;
    }

    /*
     * Initialize the R0 memory objects up-front so we can properly cleanup on allocation failures.
     */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];
        pVCpu->hm.s.svm.hMemObjVmcbHost  = NIL_RTR0MEMOBJ;
        pVCpu->hm.s.svm.hMemObjVmcb      = NIL_RTR0MEMOBJ;
        pVCpu->hm.s.svm.hMemObjMsrBitmap = NIL_RTR0MEMOBJ;
    }

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];

        /*
         * Allocate one page for the host-context VM control block (VMCB). This is used for additional host-state (such as
         * FS, GS, Kernel GS Base, etc.) apart from the host-state save area specified in MSR_K8_VM_HSAVE_PA.
         */
        rc = RTR0MemObjAllocCont(&pVCpu->hm.s.svm.hMemObjVmcbHost, SVM_VMCB_PAGES << PAGE_SHIFT, false /* fExecutable */);
        if (RT_FAILURE(rc))
            goto failure_cleanup;

        void *pvVmcbHost               = RTR0MemObjAddress(pVCpu->hm.s.svm.hMemObjVmcbHost);
        pVCpu->hm.s.svm.HCPhysVmcbHost = RTR0MemObjGetPagePhysAddr(pVCpu->hm.s.svm.hMemObjVmcbHost, 0 /* iPage */);
        Assert(pVCpu->hm.s.svm.HCPhysVmcbHost < _4G);
        ASMMemZeroPage(pvVmcbHost);

        /*
         * Allocate one page for the guest-state VMCB.
         */
        rc = RTR0MemObjAllocCont(&pVCpu->hm.s.svm.hMemObjVmcb, SVM_VMCB_PAGES << PAGE_SHIFT, false /* fExecutable */);
        if (RT_FAILURE(rc))
            goto failure_cleanup;

        pVCpu->hm.s.svm.pVmcb           = (PSVMVMCB)RTR0MemObjAddress(pVCpu->hm.s.svm.hMemObjVmcb);
        pVCpu->hm.s.svm.HCPhysVmcb      = RTR0MemObjGetPagePhysAddr(pVCpu->hm.s.svm.hMemObjVmcb, 0 /* iPage */);
        Assert(pVCpu->hm.s.svm.HCPhysVmcb < _4G);
        ASMMemZeroPage(pVCpu->hm.s.svm.pVmcb);

        /*
         * Allocate two pages (8 KB) for the MSR permission bitmap. There doesn't seem to be a way to convince
         * SVM to not require one.
         */
        rc = RTR0MemObjAllocCont(&pVCpu->hm.s.svm.hMemObjMsrBitmap, SVM_MSRPM_PAGES << X86_PAGE_4K_SHIFT,
                                 false /* fExecutable */);
        if (RT_FAILURE(rc))
            goto failure_cleanup;

        pVCpu->hm.s.svm.pvMsrBitmap     = RTR0MemObjAddress(pVCpu->hm.s.svm.hMemObjMsrBitmap);
        pVCpu->hm.s.svm.HCPhysMsrBitmap = RTR0MemObjGetPagePhysAddr(pVCpu->hm.s.svm.hMemObjMsrBitmap, 0 /* iPage */);
        /* Set all bits to intercept all MSR accesses (changed later on). */
        ASMMemFill32(pVCpu->hm.s.svm.pvMsrBitmap, SVM_MSRPM_PAGES << X86_PAGE_4K_SHIFT, UINT32_C(0xffffffff));
   }

    return VINF_SUCCESS;

failure_cleanup:
    hmR0SvmFreeStructs(pVM);
    return rc;
}


/**
 * Does per-VM AMD-V termination.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR0DECL(int) SVMR0TermVM(PVM pVM)
{
    hmR0SvmFreeStructs(pVM);
    return VINF_SUCCESS;
}


/**
 * Sets the permission bits for the specified MSR in the MSRPM.
 *
 * @param   pVmcb           Pointer to the VM control block.
 * @param   pbMsrBitmap     Pointer to the MSR bitmap.
 * @param   uMsr            The MSR for which the access permissions are being set.
 * @param   enmRead         MSR read permissions.
 * @param   enmWrite        MSR write permissions.
 */
static void hmR0SvmSetMsrPermission(PSVMVMCB pVmcb, uint8_t *pbMsrBitmap, unsigned uMsr, SVMMSREXITREAD enmRead,
                                    SVMMSREXITWRITE enmWrite)
{
    uint16_t offMsrpm;
    uint32_t uMsrpmBit;
    int rc = HMSvmGetMsrpmOffsetAndBit(uMsr, &offMsrpm, &uMsrpmBit);
    AssertRC(rc);

    Assert(uMsrpmBit < 0x3fff);
    Assert(offMsrpm < SVM_MSRPM_PAGES << X86_PAGE_4K_SHIFT);

    pbMsrBitmap += offMsrpm;
    if (enmRead == SVMMSREXIT_INTERCEPT_READ)
        ASMBitSet(pbMsrBitmap, uMsrpmBit);
    else
        ASMBitClear(pbMsrBitmap, uMsrpmBit);

    if (enmWrite == SVMMSREXIT_INTERCEPT_WRITE)
        ASMBitSet(pbMsrBitmap, uMsrpmBit + 1);
    else
        ASMBitClear(pbMsrBitmap, uMsrpmBit + 1);

    pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_IOPM_MSRPM;
}


/**
 * Sets up AMD-V for the specified VM.
 * This function is only called once per-VM during initalization.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR0DECL(int) SVMR0SetupVM(PVM pVM)
{
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    AssertReturn(pVM, VERR_INVALID_PARAMETER);
    Assert(pVM->hm.s.svm.fSupported);

    bool const fPauseFilter          = RT_BOOL(pVM->hm.s.svm.u32Features & X86_CPUID_SVM_FEATURE_EDX_PAUSE_FILTER);
    bool const fPauseFilterThreshold = RT_BOOL(pVM->hm.s.svm.u32Features & X86_CPUID_SVM_FEATURE_EDX_PAUSE_FILTER_THRESHOLD);
    bool const fUsePauseFilter       = fPauseFilter && pVM->hm.s.svm.cPauseFilter && pVM->hm.s.svm.cPauseFilterThresholdTicks;

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU   pVCpu = &pVM->aCpus[i];
        PSVMVMCB pVmcb = pVM->aCpus[i].hm.s.svm.pVmcb;

        AssertMsgReturn(pVmcb, ("Invalid pVmcb for vcpu[%u]\n", i), VERR_SVM_INVALID_PVMCB);

       /* Initialize the #VMEXIT history array with end-of-array markers (UINT16_MAX). */
        Assert(!pVCpu->hm.s.idxExitHistoryFree);
        HMCPU_EXIT_HISTORY_RESET(pVCpu);

        /* Always trap #AC for reasons of security. */
        pVmcb->ctrl.u32InterceptXcpt |= RT_BIT_32(X86_XCPT_AC);

        /* Always trap #DB for reasons of security. */
        pVmcb->ctrl.u32InterceptXcpt |= RT_BIT_32(X86_XCPT_DB);

        /* Trap exceptions unconditionally (debug purposes). */
#ifdef HMSVM_ALWAYS_TRAP_PF
        pVmcb->ctrl.u32InterceptXcpt |=   RT_BIT(X86_XCPT_PF);
#endif
#ifdef HMSVM_ALWAYS_TRAP_ALL_XCPTS
        /* If you add any exceptions here, make sure to update hmR0SvmHandleExit(). */
        pVmcb->ctrl.u32InterceptXcpt |= 0
                                             | RT_BIT(X86_XCPT_BP)
                                             | RT_BIT(X86_XCPT_DE)
                                             | RT_BIT(X86_XCPT_NM)
                                             | RT_BIT(X86_XCPT_UD)
                                             | RT_BIT(X86_XCPT_NP)
                                             | RT_BIT(X86_XCPT_SS)
                                             | RT_BIT(X86_XCPT_GP)
                                             | RT_BIT(X86_XCPT_PF)
                                             | RT_BIT(X86_XCPT_MF)
                                             ;
#endif

        /* Set up unconditional intercepts and conditions. */
        pVmcb->ctrl.u64InterceptCtrl = HMSVM_MANDATORY_GUEST_CTRL_INTERCEPTS;

        /* CR0, CR4 reads must be intercepted, our shadow values are not necessarily the same as the guest's. */
        pVmcb->ctrl.u16InterceptRdCRx = RT_BIT(0) | RT_BIT(4);

        /* CR0, CR4 writes must be intercepted for the same reasons as above. */
        pVmcb->ctrl.u16InterceptWrCRx = RT_BIT(0) | RT_BIT(4);

        /* Intercept all DRx reads and writes by default. Changed later on. */
        pVmcb->ctrl.u16InterceptRdDRx = 0xffff;
        pVmcb->ctrl.u16InterceptWrDRx = 0xffff;

        /* Virtualize masking of INTR interrupts. (reads/writes from/to CR8 go to the V_TPR register) */
        pVmcb->ctrl.IntCtrl.n.u1VIntrMasking = 1;

        /* Ignore the priority in the virtual TPR. This is necessary for delivering PIC style (ExtInt) interrupts
           and we currently deliver both PIC and APIC interrupts alike. See hmR0SvmInjectPendingEvent() */
        pVmcb->ctrl.IntCtrl.n.u1IgnoreTPR   = 1;

        /* Set IO and MSR bitmap permission bitmap physical addresses. */
        pVmcb->ctrl.u64IOPMPhysAddr  = g_HCPhysIOBitmap;
        pVmcb->ctrl.u64MSRPMPhysAddr = pVCpu->hm.s.svm.HCPhysMsrBitmap;

        /* No LBR virtualization. */
        pVmcb->ctrl.u64LBRVirt = 0;

        /* Initially set all VMCB clean bits to 0 indicating that everything should be loaded from the VMCB in memory. */
        pVmcb->ctrl.u64VmcbCleanBits = 0;

        /* The host ASID MBZ, for the guest start with 1. */
        pVmcb->ctrl.TLBCtrl.n.u32ASID = 1;

        /*
         * Setup the PAT MSR (applicable for Nested Paging only).
         * The default value should be 0x0007040600070406ULL, but we want to treat all guest memory as WB,
         * so choose type 6 for all PAT slots.
         */
        pVmcb->guest.u64GPAT = UINT64_C(0x0006060606060606);

        /* Setup Nested Paging. This doesn't change throughout the execution time of the VM. */
        pVmcb->ctrl.NestedPaging.n.u1NestedPaging = pVM->hm.s.fNestedPaging;

        /* Without Nested Paging, we need additionally intercepts. */
        if (!pVM->hm.s.fNestedPaging)
        {
            /* CR3 reads/writes must be intercepted; our shadow values differ from the guest values. */
            pVmcb->ctrl.u16InterceptRdCRx |= RT_BIT(3);
            pVmcb->ctrl.u16InterceptWrCRx |= RT_BIT(3);

            /* Intercept INVLPG and task switches (may change CR3, EFLAGS, LDT). */
            pVmcb->ctrl.u64InterceptCtrl |= SVM_CTRL_INTERCEPT_INVLPG
                                         |  SVM_CTRL_INTERCEPT_TASK_SWITCH;

            /* Page faults must be intercepted to implement shadow paging. */
            pVmcb->ctrl.u32InterceptXcpt |= RT_BIT(X86_XCPT_PF);
        }

#ifdef HMSVM_ALWAYS_TRAP_TASK_SWITCH
        pVmcb->ctrl.u64InterceptCtrl |= SVM_CTRL_INTERCEPT_TASK_SWITCH;
#endif

        /* Apply the exceptions intercepts needed by the GIM provider. */
        if (pVCpu->hm.s.fGIMTrapXcptUD)
            pVmcb->ctrl.u32InterceptXcpt |= RT_BIT(X86_XCPT_UD);

        /* Setup Pause Filter for guest pause-loop (spinlock) exiting. */
        if (fUsePauseFilter)
        {
            pVmcb->ctrl.u16PauseFilterCount = pVM->hm.s.svm.cPauseFilter;
            if (fPauseFilterThreshold)
                pVmcb->ctrl.u16PauseFilterThreshold = pVM->hm.s.svm.cPauseFilterThresholdTicks;
        }

        /*
         * The following MSRs are saved/restored automatically during the world-switch.
         * Don't intercept guest read/write accesses to these MSRs.
         */
        uint8_t *pbMsrBitmap = (uint8_t *)pVCpu->hm.s.svm.pvMsrBitmap;
        hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_K8_LSTAR,          SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_K8_CSTAR,          SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_K6_STAR,           SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_K8_SF_MASK,        SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_K8_FS_BASE,        SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_K8_GS_BASE,        SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_K8_KERNEL_GS_BASE, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_IA32_SYSENTER_CS,  SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_IA32_SYSENTER_ESP, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_IA32_SYSENTER_EIP, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
    }

    return VINF_SUCCESS;
}


/**
 * Invalidates a guest page by guest virtual address.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   GCVirt      Guest virtual address of the page to invalidate.
 */
VMMR0DECL(int) SVMR0InvalidatePage(PVM pVM, PVMCPU pVCpu, RTGCPTR GCVirt)
{
    AssertReturn(pVM, VERR_INVALID_PARAMETER);
    Assert(pVM->hm.s.svm.fSupported);

    bool fFlushPending = pVM->hm.s.svm.fAlwaysFlushTLB || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_TLB_FLUSH);

    /* Skip it if a TLB flush is already pending. */
    if (!fFlushPending)
    {
        Log4(("SVMR0InvalidatePage %RGv\n", GCVirt));

        PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        AssertMsgReturn(pVmcb, ("Invalid pVmcb!\n"), VERR_SVM_INVALID_PVMCB);

#if HC_ARCH_BITS == 32
        /* If we get a flush in 64-bit guest mode, then force a full TLB flush. INVLPGA takes only 32-bit addresses. */
        if (CPUMIsGuestInLongMode(pVCpu))
            VMCPU_FF_SET(pVCpu, VMCPU_FF_TLB_FLUSH);
        else
#endif
        {
            SVMR0InvlpgA(GCVirt, pVmcb->ctrl.TLBCtrl.n.u32ASID);
            STAM_COUNTER_INC(&pVCpu->hm.s.StatFlushTlbInvlpgVirt);
        }
    }
    return VINF_SUCCESS;
}


/**
 * Flushes the appropriate tagged-TLB entries.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   pCtx    Pointer to the guest-CPU or nested-guest-CPU context.
 * @param   pVmcb   Pointer to the VM control block.
 */
static void hmR0SvmFlushTaggedTlb(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMVMCB pVmcb)
{
    PVM pVM               = pVCpu->CTX_SUFF(pVM);
    PHMGLOBALCPUINFO pCpu = hmR0GetCurrentCpu();

    /*
     * Force a TLB flush for the first world switch if the current CPU differs from the one we ran on last.
     * This can happen both for start & resume due to long jumps back to ring-3.
     * If the TLB flush count changed, another VM (VCPU rather) has hit the ASID limit while flushing the TLB,
     * so we cannot reuse the ASIDs without flushing.
     */
    bool fNewAsid = false;
    Assert(pCpu->idCpu != NIL_RTCPUID);
    if (   pVCpu->hm.s.idLastCpu   != pCpu->idCpu
        || pVCpu->hm.s.cTlbFlushes != pCpu->cTlbFlushes)
    {
        STAM_COUNTER_INC(&pVCpu->hm.s.StatFlushTlbWorldSwitch);
        pVCpu->hm.s.fForceTLBFlush = true;
        fNewAsid = true;
    }

#ifdef VBOX_WITH_NESTED_HWVIRT
    if (CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
        fNewAsid = true;
#else
    RT_NOREF(pCtx);
#endif

    /* Set TLB flush state as checked until we return from the world switch. */
    ASMAtomicWriteBool(&pVCpu->hm.s.fCheckedTLBFlush, true);

    /* Check for explicit TLB flushes. */
    if (VMCPU_FF_TEST_AND_CLEAR(pVCpu, VMCPU_FF_TLB_FLUSH))
    {
        pVCpu->hm.s.fForceTLBFlush = true;
        STAM_COUNTER_INC(&pVCpu->hm.s.StatFlushTlb);
    }

    /*
     * If the AMD CPU erratum 170, We need to flush the entire TLB for each world switch. Sad.
     * This Host CPU requirement takes precedence.
     */
    if (pVM->hm.s.svm.fAlwaysFlushTLB)
    {
        pCpu->uCurrentAsid               = 1;
        pVCpu->hm.s.uCurrentAsid         = 1;
        pVCpu->hm.s.cTlbFlushes          = pCpu->cTlbFlushes;
        pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_ENTIRE;

        /* Clear the VMCB Clean Bit for NP while flushing the TLB. See @bugref{7152}. */
        pVmcb->ctrl.u64VmcbCleanBits    &= ~HMSVM_VMCB_CLEAN_NP;

        /* Keep track of last CPU ID even when flushing all the time. */
        if (fNewAsid)
            pVCpu->hm.s.idLastCpu = pCpu->idCpu;
    }
    else
    {
        pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_NOTHING;
        if (pVCpu->hm.s.fForceTLBFlush)
        {
            /* Clear the VMCB Clean Bit for NP while flushing the TLB. See @bugref{7152}. */
            pVmcb->ctrl.u64VmcbCleanBits    &= ~HMSVM_VMCB_CLEAN_NP;

            if (fNewAsid)
            {
                ++pCpu->uCurrentAsid;

                bool fHitASIDLimit = false;
                if (pCpu->uCurrentAsid >= pVM->hm.s.uMaxAsid)
                {
                    pCpu->uCurrentAsid = 1;      /* Wraparound at 1; host uses 0 */
                    pCpu->cTlbFlushes++;         /* All VCPUs that run on this host CPU must use a new ASID. */
                    fHitASIDLimit      = true;
                }

                if (   fHitASIDLimit
                    || pCpu->fFlushAsidBeforeUse)
                {
                    pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_ENTIRE;
                    pCpu->fFlushAsidBeforeUse = false;
                }

                pVCpu->hm.s.uCurrentAsid = pCpu->uCurrentAsid;
                pVCpu->hm.s.idLastCpu    = pCpu->idCpu;
                pVCpu->hm.s.cTlbFlushes  = pCpu->cTlbFlushes;
            }
            else
            {
                if (pVM->hm.s.svm.u32Features & X86_CPUID_SVM_FEATURE_EDX_FLUSH_BY_ASID)
                    pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_SINGLE_CONTEXT;
                else
                    pVmcb->ctrl.TLBCtrl.n.u8TLBFlush = SVM_TLB_FLUSH_ENTIRE;
            }

            pVCpu->hm.s.fForceTLBFlush = false;
        }
    }

    /* Update VMCB with the ASID. */
    if (pVmcb->ctrl.TLBCtrl.n.u32ASID != pVCpu->hm.s.uCurrentAsid)
    {
        pVmcb->ctrl.TLBCtrl.n.u32ASID = pVCpu->hm.s.uCurrentAsid;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_ASID;
    }

    AssertMsg(pVCpu->hm.s.idLastCpu == pCpu->idCpu,
              ("vcpu idLastCpu=%u pcpu idCpu=%u\n", pVCpu->hm.s.idLastCpu, pCpu->idCpu));
    AssertMsg(pVCpu->hm.s.cTlbFlushes == pCpu->cTlbFlushes,
              ("Flush count mismatch for cpu %u (%u vs %u)\n", pCpu->idCpu, pVCpu->hm.s.cTlbFlushes, pCpu->cTlbFlushes));
    AssertMsg(pCpu->uCurrentAsid >= 1 && pCpu->uCurrentAsid < pVM->hm.s.uMaxAsid,
              ("cpu%d uCurrentAsid = %x\n", pCpu->idCpu, pCpu->uCurrentAsid));
    AssertMsg(pVCpu->hm.s.uCurrentAsid >= 1 && pVCpu->hm.s.uCurrentAsid < pVM->hm.s.uMaxAsid,
              ("cpu%d VM uCurrentAsid = %x\n", pCpu->idCpu, pVCpu->hm.s.uCurrentAsid));

#ifdef VBOX_WITH_STATISTICS
    if (pVmcb->ctrl.TLBCtrl.n.u8TLBFlush == SVM_TLB_FLUSH_NOTHING)
        STAM_COUNTER_INC(&pVCpu->hm.s.StatNoFlushTlbWorldSwitch);
    else if (   pVmcb->ctrl.TLBCtrl.n.u8TLBFlush == SVM_TLB_FLUSH_SINGLE_CONTEXT
             || pVmcb->ctrl.TLBCtrl.n.u8TLBFlush == SVM_TLB_FLUSH_SINGLE_CONTEXT_RETAIN_GLOBALS)
    {
        STAM_COUNTER_INC(&pVCpu->hm.s.StatFlushAsid);
    }
    else
    {
        Assert(pVmcb->ctrl.TLBCtrl.n.u8TLBFlush == SVM_TLB_FLUSH_ENTIRE);
        STAM_COUNTER_INC(&pVCpu->hm.s.StatFlushEntire);
    }
#endif
}


/** @name 64-bit guest on 32-bit host OS helper functions.
 *
 * The host CPU is still 64-bit capable but the host OS is running in 32-bit
 * mode (code segment, paging). These wrappers/helpers perform the necessary
 * bits for the 32->64 switcher.
 *
 * @{ */
#if HC_ARCH_BITS == 32 && defined(VBOX_ENABLE_64_BITS_GUESTS)
/**
 * Prepares for and executes VMRUN (64-bit guests on a 32-bit host).
 *
 * @returns VBox status code.
 * @param   HCPhysVmcbHost  Physical address of host VMCB.
 * @param   HCPhysVmcb      Physical address of the VMCB.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 */
DECLASM(int) SVMR0VMSwitcherRun64(RTHCPHYS HCPhysVmcbHost, RTHCPHYS HCPhysVmcb, PCPUMCTX pCtx, PVM pVM, PVMCPU pVCpu)
{
    uint32_t aParam[8];
    aParam[0] = RT_LO_U32(HCPhysVmcbHost);              /* Param 1: HCPhysVmcbHost - Lo. */
    aParam[1] = RT_HI_U32(HCPhysVmcbHost);              /* Param 1: HCPhysVmcbHost - Hi. */
    aParam[2] = RT_LO_U32(HCPhysVmcb);                  /* Param 2: HCPhysVmcb - Lo. */
    aParam[3] = RT_HI_U32(HCPhysVmcb);                  /* Param 2: HCPhysVmcb - Hi. */
    aParam[4] = VM_RC_ADDR(pVM, pVM);
    aParam[5] = 0;
    aParam[6] = VM_RC_ADDR(pVM, pVCpu);
    aParam[7] = 0;

    return SVMR0Execute64BitsHandler(pVM, pVCpu, pCtx, HM64ON32OP_SVMRCVMRun64, RT_ELEMENTS(aParam), &aParam[0]);
}


/**
 * Executes the specified VMRUN handler in 64-bit mode.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   enmOp       The operation to perform.
 * @param   cParams     Number of parameters.
 * @param   paParam     Array of 32-bit parameters.
 */
VMMR0DECL(int) SVMR0Execute64BitsHandler(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, HM64ON32OP enmOp,
                                         uint32_t cParams, uint32_t *paParam)
{
    AssertReturn(pVM->hm.s.pfnHost32ToGuest64R0, VERR_HM_NO_32_TO_64_SWITCHER);
    Assert(enmOp > HM64ON32OP_INVALID && enmOp < HM64ON32OP_END);

    NOREF(pCtx);

    /* Disable interrupts. */
    RTHCUINTREG uOldEFlags = ASMIntDisableFlags();

#ifdef VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI
    RTCPUID idHostCpu = RTMpCpuId();
    CPUMR0SetLApic(pVCpu, idHostCpu);
#endif

    CPUMSetHyperESP(pVCpu, VMMGetStackRC(pVCpu));
    CPUMSetHyperEIP(pVCpu, enmOp);
    for (int i = (int)cParams - 1; i >= 0; i--)
        CPUMPushHyper(pVCpu, paParam[i]);

    STAM_PROFILE_ADV_START(&pVCpu->hm.s.StatWorldSwitch3264, z);
    /* Call the switcher. */
    int rc = pVM->hm.s.pfnHost32ToGuest64R0(pVM, RT_OFFSETOF(VM, aCpus[pVCpu->idCpu].cpum) - RT_OFFSETOF(VM, cpum));
    STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatWorldSwitch3264, z);

    /* Restore interrupts. */
    ASMSetFlags(uOldEFlags);
    return rc;
}

#endif /* HC_ARCH_BITS == 32 && defined(VBOX_ENABLE_64_BITS_GUESTS) */
/** @} */


/**
 * Adds an exception to the intercept exception bitmap in the VMCB and updates
 * the corresponding VMCB Clean bit.
 *
 * @param   pVmcb       Pointer to the VM control block.
 * @param   u32Xcpt     The value of the exception (X86_XCPT_*).
 */
DECLINLINE(void) hmR0SvmAddXcptIntercept(PSVMVMCB pVmcb, uint32_t u32Xcpt)
{
    if (!(pVmcb->ctrl.u32InterceptXcpt & RT_BIT(u32Xcpt)))
    {
        pVmcb->ctrl.u32InterceptXcpt |= RT_BIT(u32Xcpt);
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_INTERCEPTS;
    }
}


/**
 * Removes an exception from the intercept-exception bitmap in the VMCB and
 * updates the corresponding VMCB Clean bit.
 *
 * @param   pVmcb       Pointer to the VM control block.
 * @param   u32Xcpt     The value of the exception (X86_XCPT_*).
 */
DECLINLINE(void) hmR0SvmRemoveXcptIntercept(PSVMVMCB pVmcb, uint32_t u32Xcpt)
{
    Assert(u32Xcpt != X86_XCPT_DB);
    Assert(u32Xcpt != X86_XCPT_AC);
#ifndef HMSVM_ALWAYS_TRAP_ALL_XCPTS
    if (pVmcb->ctrl.u32InterceptXcpt & RT_BIT(u32Xcpt))
    {
        pVmcb->ctrl.u32InterceptXcpt &= ~RT_BIT(u32Xcpt);
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_INTERCEPTS;
    }
#endif
}


/**
 * Loads the guest CR0 control register into the guest-state area in the VMCB.
 * Although the guest CR0 is a separate field in the VMCB we have to consider
 * the FPU state itself which is shared between the host and the guest.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pVmcb       Pointer to the VM control block.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static void hmR0SvmLoadSharedCR0(PVMCPU pVCpu, PSVMVMCB pVmcb, PCPUMCTX pCtx)
{
    Assert(CPUMIsGuestFPUStateActive(pVCpu));

    uint64_t u64GuestCR0 = pCtx->cr0;

    /* Always enable caching. */
    u64GuestCR0 &= ~(X86_CR0_CD | X86_CR0_NW);

    /*
     * When Nested Paging is not available use shadow page tables and intercept #PFs (the latter done in SVMR0SetupVM()).
     */
    if (!pVCpu->CTX_SUFF(pVM)->hm.s.fNestedPaging)
    {
        u64GuestCR0 |= X86_CR0_PG;     /* When Nested Paging is not available, use shadow page tables. */
        u64GuestCR0 |= X86_CR0_WP;     /* Guest CPL 0 writes to its read-only pages should cause a #PF #VMEXIT. */
    }

    /*
     * Guest FPU bits.
     */
    bool fInterceptMF = false;
    u64GuestCR0 |= X86_CR0_NE;         /* Use internal x87 FPU exceptions handling rather than external interrupts. */

    /* Catch floating point exceptions if we need to report them to the guest in a different way. */
    if (!(pCtx->cr0 & X86_CR0_NE))
    {
        Log4(("hmR0SvmLoadGuestControlRegs: Intercepting Guest CR0.MP Old-style FPU handling!!!\n"));
        fInterceptMF = true;
    }

    /*
     * Update the exception intercept bitmap.
     */
    if (fInterceptMF)
        hmR0SvmAddXcptIntercept(pVmcb, X86_XCPT_MF);
    else
        hmR0SvmRemoveXcptIntercept(pVmcb, X86_XCPT_MF);

    pVmcb->guest.u64CR0 = u64GuestCR0;
    pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_CRX_EFER;
}


/**
 * Loads the guest/nested-guest control registers (CR2, CR3, CR4) into the VMCB.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pVmcb       Pointer to the VM control block.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static int hmR0SvmLoadGuestControlRegs(PVMCPU pVCpu, PSVMVMCB pVmcb, PCPUMCTX pCtx)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);

    /*
     * Guest CR2.
     */
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_CR2))
    {
        pVmcb->guest.u64CR2 = pCtx->cr2;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_CR2;
        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_CR2);
    }

    /*
     * Guest CR3.
     */
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_CR3))
    {
        if (pVM->hm.s.fNestedPaging)
        {
            PGMMODE enmShwPagingMode;
#if HC_ARCH_BITS == 32
            if (CPUMIsGuestInLongModeEx(pCtx))
                enmShwPagingMode = PGMMODE_AMD64_NX;
            else
#endif
                enmShwPagingMode = PGMGetHostMode(pVM);

            pVmcb->ctrl.u64NestedPagingCR3 = PGMGetNestedCR3(pVCpu, enmShwPagingMode);
            pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_NP;
            Assert(pVmcb->ctrl.u64NestedPagingCR3);
            pVmcb->guest.u64CR3 = pCtx->cr3;
        }
        else
        {
            pVmcb->guest.u64CR3 = PGMGetHyperCR3(pVCpu);
            Log4(("hmR0SvmLoadGuestControlRegs: CR3=%#RX64 (HyperCR3=%#RX64)\n", pCtx->cr3, pVmcb->guest.u64CR3));
        }

        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_CRX_EFER;
        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_CR3);
    }

    /*
     * Guest CR4.
     * ASSUMES this is done everytime we get in from ring-3! (XCR0)
     */
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_CR4))
    {
        uint64_t u64GuestCR4 = pCtx->cr4;
        Assert(RT_HI_U32(u64GuestCR4) == 0);
        if (!pVM->hm.s.fNestedPaging)
        {
            switch (pVCpu->hm.s.enmShadowMode)
            {
                case PGMMODE_REAL:
                case PGMMODE_PROTECTED:     /* Protected mode, no paging. */
                    AssertFailed();
                    return VERR_PGM_UNSUPPORTED_SHADOW_PAGING_MODE;

                case PGMMODE_32_BIT:        /* 32-bit paging. */
                    u64GuestCR4 &= ~X86_CR4_PAE;
                    break;

                case PGMMODE_PAE:           /* PAE paging. */
                case PGMMODE_PAE_NX:        /* PAE paging with NX enabled. */
                    /** Must use PAE paging as we could use physical memory > 4 GB */
                    u64GuestCR4 |= X86_CR4_PAE;
                    break;

                case PGMMODE_AMD64:         /* 64-bit AMD paging (long mode). */
                case PGMMODE_AMD64_NX:      /* 64-bit AMD paging (long mode) with NX enabled. */
#ifdef VBOX_ENABLE_64_BITS_GUESTS
                    break;
#else
                    AssertFailed();
                    return VERR_PGM_UNSUPPORTED_SHADOW_PAGING_MODE;
#endif

                default:                    /* shut up gcc */
                    AssertFailed();
                    return VERR_PGM_UNSUPPORTED_SHADOW_PAGING_MODE;
            }
        }

        pVmcb->guest.u64CR4 = u64GuestCR4;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_CRX_EFER;

        /* Whether to save/load/restore XCR0 during world switch depends on CR4.OSXSAVE and host+guest XCR0. */
        pVCpu->hm.s.fLoadSaveGuestXcr0 = (u64GuestCR4 & X86_CR4_OSXSAVE) && pCtx->aXcr[0] != ASMGetXcr0();

        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_CR4);
    }

    return VINF_SUCCESS;
}


#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Loads the nested-guest control registers (CR0, CR2, CR3, CR4) into the VMCB.
 *
 * @returns VBox status code.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pVmcbNstGst     Pointer to the nested-guest VM control block.
 * @param   pCtx            Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static int hmR0SvmLoadGuestControlRegsNested(PVMCPU pVCpu, PSVMVMCB pVmcbNstGst, PCPUMCTX pCtx)
{
    /*
     * Guest CR0.
     */
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_CR0))
    {
        pVmcbNstGst->guest.u64CR0 = pCtx->cr0;
        pVmcbNstGst->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_CRX_EFER;
        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_CR0);
    }

    return hmR0SvmLoadGuestControlRegs(pVCpu, pVmcbNstGst, pCtx);
}
#endif


/**
 * Loads the guest segment registers into the VMCB.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pVmcb       Pointer to the VM control block.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static void hmR0SvmLoadGuestSegmentRegs(PVMCPU pVCpu, PSVMVMCB pVmcb, PCPUMCTX pCtx)
{
    /* Guest Segment registers: CS, SS, DS, ES, FS, GS. */
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_SEGMENT_REGS))
    {
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, &pVmcb->guest, CS, cs);
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, &pVmcb->guest, SS, ss);
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, &pVmcb->guest, DS, ds);
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, &pVmcb->guest, ES, es);
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, &pVmcb->guest, FS, fs);
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, &pVmcb->guest, GS, gs);

        pVmcb->guest.u8CPL = pCtx->ss.Attr.n.u2Dpl;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_SEG;
        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_SEGMENT_REGS);
    }

    /* Guest TR. */
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_TR))
    {
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, &pVmcb->guest, TR, tr);
        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_TR);
    }

    /* Guest LDTR. */
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_LDTR))
    {
        HMSVM_SEG_REG_COPY_TO_VMCB(pCtx, &pVmcb->guest, LDTR, ldtr);
        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_LDTR);
    }

    /* Guest GDTR. */
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_GDTR))
    {
        pVmcb->guest.GDTR.u32Limit = pCtx->gdtr.cbGdt;
        pVmcb->guest.GDTR.u64Base  = pCtx->gdtr.pGdt;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_DT;
        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_GDTR);
    }

    /* Guest IDTR. */
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_IDTR))
    {
        pVmcb->guest.IDTR.u32Limit = pCtx->idtr.cbIdt;
        pVmcb->guest.IDTR.u64Base  = pCtx->idtr.pIdt;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_DT;
        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_IDTR);
    }
}


/**
 * Loads the guest MSRs into the VMCB.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pVmcb       Pointer to the VM control block.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static void hmR0SvmLoadGuestMsrs(PVMCPU pVCpu, PSVMVMCB pVmcb, PCPUMCTX pCtx)
{
    /* Guest Sysenter MSRs. */
    pVmcb->guest.u64SysEnterCS  = pCtx->SysEnter.cs;
    pVmcb->guest.u64SysEnterEIP = pCtx->SysEnter.eip;
    pVmcb->guest.u64SysEnterESP = pCtx->SysEnter.esp;

    /*
     * Guest EFER MSR.
     * AMD-V requires guest EFER.SVME to be set. Weird.
     * See AMD spec. 15.5.1 "Basic Operation" | "Canonicalization and Consistency Checks".
     */
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_EFER_MSR))
    {
        pVmcb->guest.u64EFER = pCtx->msrEFER | MSR_K6_EFER_SVME;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_CRX_EFER;
        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_EFER_MSR);
    }

    /* 64-bit MSRs. */
    if (CPUMIsGuestInLongModeEx(pCtx))
    {
        pVmcb->guest.FS.u64Base = pCtx->fs.u64Base;
        pVmcb->guest.GS.u64Base = pCtx->gs.u64Base;
    }
    else
    {
        /* If the guest isn't in 64-bit mode, clear MSR_K6_LME bit from guest EFER otherwise AMD-V expects amd64 shadow paging. */
        if (pCtx->msrEFER & MSR_K6_EFER_LME)
        {
            pVmcb->guest.u64EFER &= ~MSR_K6_EFER_LME;
            pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_CRX_EFER;
        }
    }

    /** @todo The following are used in 64-bit only (SYSCALL/SYSRET) but they might
     *        be writable in 32-bit mode. Clarify with AMD spec. */
    pVmcb->guest.u64STAR         = pCtx->msrSTAR;
    pVmcb->guest.u64LSTAR        = pCtx->msrLSTAR;
    pVmcb->guest.u64CSTAR        = pCtx->msrCSTAR;
    pVmcb->guest.u64SFMASK       = pCtx->msrSFMASK;
    pVmcb->guest.u64KernelGSBase = pCtx->msrKERNELGSBASE;
}


/**
 * Loads the guest (or nested-guest) debug state into the VMCB and programs the
 * necessary intercepts accordingly.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pVmcb       Pointer to the VM control block.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 * @remarks Requires EFLAGS to be up-to-date in the VMCB!
 */
static void hmR0SvmLoadSharedDebugState(PVMCPU pVCpu, PSVMVMCB pVmcb, PCPUMCTX pCtx)
{
    bool fInterceptMovDRx = false;

    /*
     * Anyone single stepping on the host side? If so, we'll have to use the
     * trap flag in the guest EFLAGS since AMD-V doesn't have a trap flag on
     * the VMM level like the VT-x implementations does.
     */
    bool const fStepping = pVCpu->hm.s.fSingleInstruction;
    if (fStepping)
    {
        pVCpu->hm.s.fClearTrapFlag = true;
        pVmcb->guest.u64RFlags |= X86_EFL_TF;
        fInterceptMovDRx = true; /* Need clean DR6, no guest mess. */
    }
    else
        Assert(!DBGFIsStepping(pVCpu));

    if (   fStepping
        || (CPUMGetHyperDR7(pVCpu) & X86_DR7_ENABLED_MASK))
    {
        /*
         * Use the combined guest and host DRx values found in the hypervisor
         * register set because the debugger has breakpoints active or someone
         * is single stepping on the host side.
         *
         * Note! DBGF expects a clean DR6 state before executing guest code.
         */
#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
        if (   CPUMIsGuestInLongModeEx(pCtx)
            && !CPUMIsHyperDebugStateActivePending(pVCpu))
        {
            CPUMR0LoadHyperDebugState(pVCpu, false /* include DR6 */);
            Assert(!CPUMIsGuestDebugStateActivePending(pVCpu));
            Assert(CPUMIsHyperDebugStateActivePending(pVCpu));
        }
        else
#endif
        if (!CPUMIsHyperDebugStateActive(pVCpu))
        {
            CPUMR0LoadHyperDebugState(pVCpu, false /* include DR6 */);
            Assert(!CPUMIsGuestDebugStateActive(pVCpu));
            Assert(CPUMIsHyperDebugStateActive(pVCpu));
        }

        /* Update DR6 & DR7. (The other DRx values are handled by CPUM one way or the other.) */
        if (   pVmcb->guest.u64DR6 != X86_DR6_INIT_VAL
            || pVmcb->guest.u64DR7 != CPUMGetHyperDR7(pVCpu))
        {
            pVmcb->guest.u64DR7 = CPUMGetHyperDR7(pVCpu);
            pVmcb->guest.u64DR6 = X86_DR6_INIT_VAL;
            pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_DRX;
            pVCpu->hm.s.fUsingHyperDR7 = true;
        }

        /** @todo If we cared, we could optimize to allow the guest to read registers
         *        with the same values. */
        fInterceptMovDRx = true;
        Log5(("hmR0SvmLoadSharedDebugState: Loaded hyper DRx\n"));
    }
    else
    {
        /*
         * Update DR6, DR7 with the guest values if necessary.
         */
        if (   pVmcb->guest.u64DR7 != pCtx->dr[7]
            || pVmcb->guest.u64DR6 != pCtx->dr[6])
        {
            pVmcb->guest.u64DR7 = pCtx->dr[7];
            pVmcb->guest.u64DR6 = pCtx->dr[6];
            pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_DRX;
            pVCpu->hm.s.fUsingHyperDR7 = false;
        }

        /*
         * If the guest has enabled debug registers, we need to load them prior to
         * executing guest code so they'll trigger at the right time.
         */
        if (pCtx->dr[7] & (X86_DR7_ENABLED_MASK | X86_DR7_GD)) /** @todo Why GD? */
        {
#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
            if (   CPUMIsGuestInLongModeEx(pCtx)
                && !CPUMIsGuestDebugStateActivePending(pVCpu))
            {
                CPUMR0LoadGuestDebugState(pVCpu, false /* include DR6 */);
                STAM_COUNTER_INC(&pVCpu->hm.s.StatDRxArmed);
                Assert(!CPUMIsHyperDebugStateActivePending(pVCpu));
                Assert(CPUMIsGuestDebugStateActivePending(pVCpu));
            }
            else
#endif
            if (!CPUMIsGuestDebugStateActive(pVCpu))
            {
                CPUMR0LoadGuestDebugState(pVCpu, false /* include DR6 */);
                STAM_COUNTER_INC(&pVCpu->hm.s.StatDRxArmed);
                Assert(!CPUMIsHyperDebugStateActive(pVCpu));
                Assert(CPUMIsGuestDebugStateActive(pVCpu));
            }
            Log5(("hmR0SvmLoadSharedDebugState: Loaded guest DRx\n"));
        }
        /*
         * If no debugging enabled, we'll lazy load DR0-3. We don't need to
         * intercept #DB as DR6 is updated in the VMCB.
         *
         * Note! If we cared and dared, we could skip intercepting \#DB here.
         *       However, \#DB shouldn't be performance critical, so we'll play safe
         *       and keep the code similar to the VT-x code and always intercept it.
         */
#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
        else if (   !CPUMIsGuestDebugStateActivePending(pVCpu)
                 && !CPUMIsGuestDebugStateActive(pVCpu))
#else
        else if (!CPUMIsGuestDebugStateActive(pVCpu))
#endif
        {
            fInterceptMovDRx = true;
        }
    }

    Assert(pVmcb->ctrl.u32InterceptXcpt & RT_BIT_32(X86_XCPT_DB));
    if (fInterceptMovDRx)
    {
        if (   pVmcb->ctrl.u16InterceptRdDRx != 0xffff
            || pVmcb->ctrl.u16InterceptWrDRx != 0xffff)
        {
            pVmcb->ctrl.u16InterceptRdDRx = 0xffff;
            pVmcb->ctrl.u16InterceptWrDRx = 0xffff;
            pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_INTERCEPTS;
        }
    }
    else
    {
        if (   pVmcb->ctrl.u16InterceptRdDRx
            || pVmcb->ctrl.u16InterceptWrDRx)
        {
            pVmcb->ctrl.u16InterceptRdDRx = 0;
            pVmcb->ctrl.u16InterceptWrDRx = 0;
            pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_INTERCEPTS;
        }
    }
    Log4(("hmR0SvmLoadSharedDebugState: DR6=%#RX64 DR7=%#RX64\n", pCtx->dr[6], pCtx->dr[7]));
}


#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Loads the nested-guest APIC state (currently just the TPR).
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pVmcbNstGst     Pointer to the nested-guest VM control block.
 */
static void hmR0SvmLoadGuestApicStateNested(PVMCPU pVCpu, PSVMVMCB pVmcbNstGst)
{
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_SVM_GUEST_APIC_STATE))
    {
        /* Always enable V_INTR_MASKING as we do not want to allow access to the physical APIC TPR. */
        pVmcbNstGst->ctrl.IntCtrl.n.u1VIntrMasking = 1;
        pVCpu->hm.s.svm.fSyncVTpr = false;
        pVmcbNstGst->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_TPR;

        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_SVM_GUEST_APIC_STATE);
    }
}
#endif

/**
 * Loads the guest APIC state (currently just the TPR).
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pVmcb       Pointer to the VM control block.
 * @param   pCtx        Pointer to the guest-CPU context.
 */
static int hmR0SvmLoadGuestApicState(PVMCPU pVCpu, PSVMVMCB pVmcb, PCPUMCTX pCtx)
{
    if (!HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_SVM_GUEST_APIC_STATE))
        return VINF_SUCCESS;

    int rc = VINF_SUCCESS;
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    if (   PDMHasApic(pVM)
        && APICIsEnabled(pVCpu))
    {
        bool    fPendingIntr;
        uint8_t u8Tpr;
        rc = APICGetTpr(pVCpu, &u8Tpr, &fPendingIntr, NULL /* pu8PendingIrq */);
        AssertRCReturn(rc, rc);

        /* Assume that we need to trap all TPR accesses and thus need not check on
           every #VMEXIT if we should update the TPR. */
        Assert(pVmcb->ctrl.IntCtrl.n.u1VIntrMasking);
        pVCpu->hm.s.svm.fSyncVTpr = false;

        /* 32-bit guests uses LSTAR MSR for patching guest code which touches the TPR. */
        if (pVM->hm.s.fTPRPatchingActive)
        {
            pCtx->msrLSTAR = u8Tpr;
            uint8_t *pbMsrBitmap = (uint8_t *)pVCpu->hm.s.svm.pvMsrBitmap;

            /* If there are interrupts pending, intercept LSTAR writes, otherwise don't intercept reads or writes. */
            if (fPendingIntr)
                hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_K8_LSTAR, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_INTERCEPT_WRITE);
            else
            {
                hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_K8_LSTAR, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
                pVCpu->hm.s.svm.fSyncVTpr = true;
            }
        }
        else
        {
            /* Bits 3-0 of the VTPR field correspond to bits 7-4 of the TPR (which is the Task-Priority Class). */
            pVmcb->ctrl.IntCtrl.n.u8VTPR = (u8Tpr >> 4);

            /* If there are interrupts pending, intercept CR8 writes to evaluate ASAP if we can deliver the interrupt to the guest. */
            if (fPendingIntr)
                pVmcb->ctrl.u16InterceptWrCRx |= RT_BIT(8);
            else
            {
                pVmcb->ctrl.u16InterceptWrCRx &= ~RT_BIT(8);
                pVCpu->hm.s.svm.fSyncVTpr = true;
            }

            pVmcb->ctrl.u64VmcbCleanBits &= ~(HMSVM_VMCB_CLEAN_INTERCEPTS | HMSVM_VMCB_CLEAN_TPR);
        }
    }

    HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_SVM_GUEST_APIC_STATE);
    return rc;
}


/**
 * Loads the exception interrupts required for guest (or nested-guest) execution in
 * the VMCB.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pVmcb       Pointer to the VM control block.
 */
static void hmR0SvmLoadGuestXcptIntercepts(PVMCPU pVCpu, PSVMVMCB pVmcb)
{
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_XCPT_INTERCEPTS))
    {
        /* Trap #UD for GIM provider (e.g. for hypercalls). */
        if (pVCpu->hm.s.fGIMTrapXcptUD)
            hmR0SvmAddXcptIntercept(pVmcb, X86_XCPT_UD);
        else
            hmR0SvmRemoveXcptIntercept(pVmcb, X86_XCPT_UD);

        /* Trap #BP for INT3 debug breakpoints set by the VM debugger. */
        if (pVCpu->CTX_SUFF(pVM)->dbgf.ro.cEnabledInt3Breakpoints)
            hmR0SvmAddXcptIntercept(pVmcb, X86_XCPT_BP);
        else
            hmR0SvmRemoveXcptIntercept(pVmcb, X86_XCPT_BP);

        /* The remaining intercepts are handled elsewhere, e.g. in hmR0SvmLoadSharedCR0(). */
        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_XCPT_INTERCEPTS);
    }
}


#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Loads the intercepts required for nested-guest execution in the VMCB.
 *
 * This merges the guest and nested-guest intercepts in a way that if the outer
 * guest intercepts an exception we need to intercept it in the nested-guest as
 * well and handle it accordingly.
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pVmcbNstGst     Pointer to the nested-guest VM control block.
 */
static void hmR0SvmLoadGuestXcptInterceptsNested(PVMCPU pVCpu, PSVMVMCB pVmcbNstGst)
{
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_XCPT_INTERCEPTS))
    {
        /* First, load the guest intercepts into the guest VMCB. */
        PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        hmR0SvmLoadGuestXcptIntercepts(pVCpu, pVmcb);

        /* Next, merge the intercepts into the nested-guest VMCB. */
        pVmcbNstGst->ctrl.u16InterceptRdCRx |= pVmcb->ctrl.u16InterceptRdCRx;
        pVmcbNstGst->ctrl.u16InterceptWrCRx |= pVmcb->ctrl.u16InterceptWrCRx;

        /*
         * CR3, CR4 reads and writes are intercepted as we modify them before
         * hardware-assisted SVM execution. In addition, PGM needs to be up to date
         * on paging mode changes in the nested-guest.
         *
         * CR0 writes are intercepted in case of paging mode changes. CR0 reads are not
         * intercepted as we currently don't modify CR0 while executing the nested-guest.
         */
        pVmcbNstGst->ctrl.u16InterceptRdCRx |= RT_BIT(4) | RT_BIT(3);
        pVmcbNstGst->ctrl.u16InterceptWrCRx |= RT_BIT(4) | RT_BIT(3) | RT_BIT(0);

        /** @todo Figure out debugging with nested-guests, till then just intercept
         *        all DR[0-15] accesses. */
        pVmcbNstGst->ctrl.u16InterceptRdDRx |= 0xffff;
        pVmcbNstGst->ctrl.u16InterceptWrDRx |= 0xffff;

        pVmcbNstGst->ctrl.u32InterceptXcpt  |= pVmcb->ctrl.u32InterceptXcpt;
        pVmcbNstGst->ctrl.u64InterceptCtrl  |= pVmcb->ctrl.u64InterceptCtrl
                                            |  HMSVM_MANDATORY_NESTED_GUEST_CTRL_INTERCEPTS;
        /*
         * Remove control intercepts that we don't need while executing the nested-guest.
         *
         * VMMCALL when not intercepted raises a \#UD exception in the guest. However,
         * other SVM instructions like VMSAVE when not intercept can cause havoc on the
         * host as they can write to any location in physical memory, hence they always
         * need to be intercepted (they are included in HMSVM_MANDATORY_GUEST_CTRL_INTERCEPTS).
         */
        Assert(   (pVmcbNstGst->ctrl.u64InterceptCtrl & HMSVM_MANDATORY_GUEST_CTRL_INTERCEPTS)
               == HMSVM_MANDATORY_GUEST_CTRL_INTERCEPTS);
        pVmcbNstGst->ctrl.u64InterceptCtrl  &= ~SVM_CTRL_INTERCEPT_VMMCALL;

        /* Remove exception intercepts that we don't need while executing the nested-guest. */
        pVmcbNstGst->ctrl.u32InterceptXcpt  &= ~RT_BIT(X86_XCPT_UD);

        Assert(!HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_XCPT_INTERCEPTS));
    }
}
#endif


/**
 * Sets up the appropriate function to run guest code.
 *
 * @returns VBox status code.
 * @param   pVCpu   The cross context virtual CPU structure.
 *
 * @remarks No-long-jump zone!!!
 */
static int hmR0SvmSetupVMRunHandler(PVMCPU pVCpu)
{
    if (CPUMIsGuestInLongMode(pVCpu))
    {
#ifndef VBOX_ENABLE_64_BITS_GUESTS
        return VERR_PGM_UNSUPPORTED_SHADOW_PAGING_MODE;
#endif
        Assert(pVCpu->CTX_SUFF(pVM)->hm.s.fAllow64BitGuests);    /* Guaranteed by hmR3InitFinalizeR0(). */
#if HC_ARCH_BITS == 32
        /* 32-bit host. We need to switch to 64-bit before running the 64-bit guest. */
        pVCpu->hm.s.svm.pfnVMRun = SVMR0VMSwitcherRun64;
#else
        /* 64-bit host or hybrid host. */
        pVCpu->hm.s.svm.pfnVMRun = SVMR0VMRun64;
#endif
    }
    else
    {
        /* Guest is not in long mode, use the 32-bit handler. */
        pVCpu->hm.s.svm.pfnVMRun = SVMR0VMRun;
    }
    return VINF_SUCCESS;
}


/**
 * Enters the AMD-V session.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCpu        Pointer to the CPU info struct.
 */
VMMR0DECL(int) SVMR0Enter(PVM pVM, PVMCPU pVCpu, PHMGLOBALCPUINFO pCpu)
{
    AssertPtr(pVM);
    AssertPtr(pVCpu);
    Assert(pVM->hm.s.svm.fSupported);
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    NOREF(pVM); NOREF(pCpu);

    LogFlowFunc(("pVM=%p pVCpu=%p\n", pVM, pVCpu));
    Assert(HMCPU_CF_IS_SET(pVCpu, HM_CHANGED_HOST_CONTEXT | HM_CHANGED_HOST_GUEST_SHARED_STATE));

    pVCpu->hm.s.fLeaveDone = false;
    return VINF_SUCCESS;
}


/**
 * Thread-context callback for AMD-V.
 *
 * @param   enmEvent        The thread-context event.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   fGlobalInit     Whether global VT-x/AMD-V init. is used.
 * @thread  EMT(pVCpu)
 */
VMMR0DECL(void) SVMR0ThreadCtxCallback(RTTHREADCTXEVENT enmEvent, PVMCPU pVCpu, bool fGlobalInit)
{
    NOREF(fGlobalInit);

    switch (enmEvent)
    {
        case RTTHREADCTXEVENT_OUT:
        {
            Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
            Assert(VMMR0ThreadCtxHookIsEnabled(pVCpu));
            VMCPU_ASSERT_EMT(pVCpu);

            /* No longjmps (log-flush, locks) in this fragile context. */
            VMMRZCallRing3Disable(pVCpu);

            if (!pVCpu->hm.s.fLeaveDone)
            {
                hmR0SvmLeave(pVCpu);
                pVCpu->hm.s.fLeaveDone = true;
            }

            /* Leave HM context, takes care of local init (term). */
            int rc = HMR0LeaveCpu(pVCpu);
            AssertRC(rc); NOREF(rc);

            /* Restore longjmp state. */
            VMMRZCallRing3Enable(pVCpu);
            STAM_REL_COUNTER_INC(&pVCpu->hm.s.StatSwitchPreempt);
            break;
        }

        case RTTHREADCTXEVENT_IN:
        {
            Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
            Assert(VMMR0ThreadCtxHookIsEnabled(pVCpu));
            VMCPU_ASSERT_EMT(pVCpu);

            /* No longjmps (log-flush, locks) in this fragile context. */
            VMMRZCallRing3Disable(pVCpu);

            /*
             * Initialize the bare minimum state required for HM. This takes care of
             * initializing AMD-V if necessary (onlined CPUs, local init etc.)
             */
            int rc = HMR0EnterCpu(pVCpu);
            AssertRC(rc); NOREF(rc);
            Assert(HMCPU_CF_IS_SET(pVCpu, HM_CHANGED_HOST_CONTEXT | HM_CHANGED_HOST_GUEST_SHARED_STATE));

            pVCpu->hm.s.fLeaveDone = false;

            /* Restore longjmp state. */
            VMMRZCallRing3Enable(pVCpu);
            break;
        }

        default:
            break;
    }
}


/**
 * Saves the host state.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 *
 * @remarks No-long-jump zone!!!
 */
VMMR0DECL(int) SVMR0SaveHostState(PVM pVM, PVMCPU pVCpu)
{
    NOREF(pVM);
    NOREF(pVCpu);
    /* Nothing to do here. AMD-V does this for us automatically during the world-switch. */
    HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_HOST_CONTEXT);
    return VINF_SUCCESS;
}


/**
 * Loads the guest state into the VMCB.
 *
 * The CPU state will be loaded from these fields on every successful VM-entry.
 * Also sets up the appropriate VMRUN function to execute guest code based on
 * the guest CPU mode.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static int hmR0SvmLoadGuestState(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
    AssertMsgReturn(pVmcb, ("Invalid pVmcb\n"), VERR_SVM_INVALID_PVMCB);

    STAM_PROFILE_ADV_START(&pVCpu->hm.s.StatLoadGuestState, x);

    int rc = hmR0SvmLoadGuestControlRegs(pVCpu, pVmcb, pCtx);
    AssertLogRelMsgRCReturn(rc, ("hmR0SvmLoadGuestControlRegs! rc=%Rrc (pVM=%p pVCpu=%p)\n", rc, pVM, pVCpu), rc);

    hmR0SvmLoadGuestSegmentRegs(pVCpu, pVmcb, pCtx);
    hmR0SvmLoadGuestMsrs(pVCpu, pVmcb, pCtx);

    pVmcb->guest.u64RIP    = pCtx->rip;
    pVmcb->guest.u64RSP    = pCtx->rsp;
    pVmcb->guest.u64RFlags = pCtx->eflags.u32;
    pVmcb->guest.u64RAX    = pCtx->rax;

    rc = hmR0SvmLoadGuestApicState(pVCpu, pVmcb, pCtx);
    AssertLogRelMsgRCReturn(rc, ("hmR0SvmLoadGuestApicState! rc=%Rrc (pVM=%p pVCpu=%p)\n", rc, pVM, pVCpu), rc);

    hmR0SvmLoadGuestXcptIntercepts(pVCpu, pVmcb);

    rc = hmR0SvmSetupVMRunHandler(pVCpu);
    AssertLogRelMsgRCReturn(rc, ("hmR0SvmSetupVMRunHandler! rc=%Rrc (pVM=%p pVCpu=%p)\n", rc, pVM, pVCpu), rc);

    /* Clear any unused and reserved bits. */
    HMCPU_CF_CLEAR(pVCpu,   HM_CHANGED_GUEST_RIP                  /* Unused (loaded unconditionally). */
                          | HM_CHANGED_GUEST_RSP
                          | HM_CHANGED_GUEST_RFLAGS
                          | HM_CHANGED_GUEST_SYSENTER_CS_MSR
                          | HM_CHANGED_GUEST_SYSENTER_EIP_MSR
                          | HM_CHANGED_GUEST_SYSENTER_ESP_MSR
                          | HM_CHANGED_GUEST_LAZY_MSRS            /* Unused. */
                          | HM_CHANGED_SVM_RESERVED1              /* Reserved. */
                          | HM_CHANGED_SVM_RESERVED2
                          | HM_CHANGED_SVM_RESERVED3
                          | HM_CHANGED_SVM_RESERVED4);

    /* All the guest state bits should be loaded except maybe the host context and/or shared host/guest bits. */
    AssertMsg(   !HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_ALL_GUEST)
              ||  HMCPU_CF_IS_PENDING_ONLY(pVCpu, HM_CHANGED_HOST_CONTEXT | HM_CHANGED_HOST_GUEST_SHARED_STATE),
               ("fContextUseFlags=%#RX32\n", HMCPU_CF_VALUE(pVCpu)));

    Log4(("hmR0SvmLoadGuestState: CS:RIP=%04x:%RX64 EFL=%#x CR0=%#RX32 CR3=%#RX32 CR4=%#RX32\n", pCtx->cs.Sel, pCtx->rip,
          pCtx->eflags.u, pCtx->cr0, pCtx->cr3, pCtx->cr4));
    STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatLoadGuestState, x);
    return rc;
}


#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Caches the nested-guest VMCB fields before we modify them for execution using
 * hardware-assisted SVM.
 *
 * @returns true if the VMCB was previously already cached, false otherwise.
 * @param   pCtx            Pointer to the guest-CPU context.
 *
 * @sa      HMSvmNstGstVmExitNotify.
 */
static bool hmR0SvmVmRunCacheVmcb(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    PSVMVMCB            pVmcbNstGst      = pCtx->hwvirt.svm.CTX_SUFF(pVmcb);
    PCSVMVMCBCTRL       pVmcbNstGstCtrl  = &pVmcbNstGst->ctrl;
    PCSVMVMCBSTATESAVE  pVmcbNstGstState = &pVmcbNstGst->guest;
    PSVMNESTEDVMCBCACHE pNstGstVmcbCache = &pVCpu->hm.s.svm.NstGstVmcbCache;

    /*
     * Cache the nested-guest programmed VMCB fields if we have not cached it yet.
     * Otherwise we risk re-caching the values we may have modified, see @bugref{7243#c44}.
     *
     * Nested-paging CR3 is not saved back into the VMCB on #VMEXIT, hence no need to
     * cache and restore it, see AMD spec. 15.25.4 "Nested Paging and VMRUN/#VMEXIT".
     */
    bool const fWasCached = pCtx->hwvirt.svm.fHMCachedVmcb;
    if (!fWasCached)
    {
        pNstGstVmcbCache->u16InterceptRdCRx = pVmcbNstGstCtrl->u16InterceptRdCRx;
        pNstGstVmcbCache->u16InterceptWrCRx = pVmcbNstGstCtrl->u16InterceptWrCRx;
        pNstGstVmcbCache->u16InterceptRdDRx = pVmcbNstGstCtrl->u16InterceptRdDRx;
        pNstGstVmcbCache->u16InterceptWrDRx = pVmcbNstGstCtrl->u16InterceptWrDRx;
        pNstGstVmcbCache->u32InterceptXcpt  = pVmcbNstGstCtrl->u32InterceptXcpt;
        pNstGstVmcbCache->u64InterceptCtrl  = pVmcbNstGstCtrl->u64InterceptCtrl;
        pNstGstVmcbCache->u64CR3            = pVmcbNstGstState->u64CR3;
        pNstGstVmcbCache->u64CR4            = pVmcbNstGstState->u64CR4;
        pNstGstVmcbCache->u64EFER           = pVmcbNstGstState->u64EFER;
        pNstGstVmcbCache->u64IOPMPhysAddr   = pVmcbNstGstCtrl->u64IOPMPhysAddr;
        pNstGstVmcbCache->u64MSRPMPhysAddr  = pVmcbNstGstCtrl->u64MSRPMPhysAddr;
        pNstGstVmcbCache->u64VmcbCleanBits  = pVmcbNstGstCtrl->u64VmcbCleanBits;
        pNstGstVmcbCache->fVIntrMasking     = pVmcbNstGstCtrl->IntCtrl.n.u1VIntrMasking;
        pNstGstVmcbCache->TLBCtrl           = pVmcbNstGstCtrl->TLBCtrl;
        pNstGstVmcbCache->NestedPagingCtrl  = pVmcbNstGstCtrl->NestedPaging;
        pCtx->hwvirt.svm.fHMCachedVmcb      = true;
        Log4(("hmR0SvmVmRunCacheVmcb: Cached VMCB fields\n"));
    }

    return fWasCached;
}


/**
 * Sets up the nested-guest VMCB for execution using hardware-assisted SVM.
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCtx            Pointer to the guest-CPU context.
 */
static void hmR0SvmVmRunSetupVmcb(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    RT_NOREF(pVCpu);
    PSVMVMCB     pVmcbNstGst     = pCtx->hwvirt.svm.CTX_SUFF(pVmcb);
    PSVMVMCBCTRL pVmcbNstGstCtrl = &pVmcbNstGst->ctrl;

    /*
     * First cache the nested-guest VMCB fields we may potentially modify.
     */
    bool const fVmcbCached = hmR0SvmVmRunCacheVmcb(pVCpu, pCtx);
    if (!fVmcbCached)
    {
        /*
         * The IOPM of the nested-guest can be ignored because the the guest always
         * intercepts all IO port accesses. Thus, we'll swap to the guest IOPM rather
         * into the nested-guest one and swap it back on the #VMEXIT.
         */
        pVmcbNstGstCtrl->u64IOPMPhysAddr = g_HCPhysIOBitmap;

        /*
         * Load the host-physical address into the MSRPM rather than the nested-guest
         * physical address (currently we trap all MSRs in the nested-guest).
         */
        pVmcbNstGstCtrl->u64MSRPMPhysAddr = g_HCPhysNstGstMsrBitmap;

        /*
         * Use the same nested-paging as the "outer" guest. We can't dynamically
         * switch off nested-paging suddenly while executing a VM (see assertion at the
         * end of Trap0eHandler in PGMAllBth.h).
         */
        pVmcbNstGstCtrl->NestedPaging.n.u1NestedPaging = pVCpu->CTX_SUFF(pVM)->hm.s.fNestedPaging;
    }
    else
    {
        Assert(pVmcbNstGstCtrl->u64IOPMPhysAddr == g_HCPhysIOBitmap);
        Assert(pVmcbNstGstCtrl->u64MSRPMPhysAddr = g_HCPhysNstGstMsrBitmap);
        Assert(pVmcbNstGstCtrl->NestedPaging.n.u1NestedPaging == pVCpu->CTX_SUFF(pVM)->hm.s.fNestedPaging);
    }
}


/**
 * Loads the nested-guest state into the VMCB.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static int hmR0SvmLoadGuestStateNested(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    STAM_PROFILE_ADV_START(&pVCpu->hm.s.StatLoadGuestState, x);

    PSVMVMCB pVmcbNstGst = pCtx->hwvirt.svm.CTX_SUFF(pVmcb);
    Assert(pVmcbNstGst);

    hmR0SvmLoadGuestSegmentRegs(pVCpu, pVmcbNstGst, pCtx);
    hmR0SvmLoadGuestMsrs(pVCpu, pVmcbNstGst, pCtx);

    pVmcbNstGst->guest.u64RIP    = pCtx->rip;
    pVmcbNstGst->guest.u64RSP    = pCtx->rsp;
    pVmcbNstGst->guest.u64RFlags = pCtx->eflags.u32;
    pVmcbNstGst->guest.u64RAX    = pCtx->rax;

    int rc = hmR0SvmLoadGuestControlRegsNested(pVCpu, pVmcbNstGst, pCtx);
    AssertRCReturn(rc, rc);

    hmR0SvmLoadGuestApicStateNested(pVCpu, pVmcbNstGst);
    hmR0SvmLoadGuestXcptInterceptsNested(pVCpu, pVmcbNstGst);

    rc = hmR0SvmSetupVMRunHandler(pVCpu);
    AssertRCReturn(rc, rc);

    /* Clear any unused and reserved bits. */
    HMCPU_CF_CLEAR(pVCpu,   HM_CHANGED_GUEST_RIP                  /* Unused (loaded unconditionally). */
                          | HM_CHANGED_GUEST_RSP
                          | HM_CHANGED_GUEST_RFLAGS
                          | HM_CHANGED_GUEST_SYSENTER_CS_MSR
                          | HM_CHANGED_GUEST_SYSENTER_EIP_MSR
                          | HM_CHANGED_GUEST_SYSENTER_ESP_MSR
                          | HM_CHANGED_GUEST_LAZY_MSRS            /* Unused. */
                          | HM_CHANGED_SVM_RESERVED1              /* Reserved. */
                          | HM_CHANGED_SVM_RESERVED2
                          | HM_CHANGED_SVM_RESERVED3
                          | HM_CHANGED_SVM_RESERVED4);

    /* All the guest state bits should be loaded except maybe the host context and/or shared host/guest bits. */
    AssertMsg(   !HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_ALL_GUEST)
              ||  HMCPU_CF_IS_PENDING_ONLY(pVCpu, HM_CHANGED_HOST_CONTEXT | HM_CHANGED_HOST_GUEST_SHARED_STATE),
               ("fContextUseFlags=%#RX32\n", HMCPU_CF_VALUE(pVCpu)));

    Log4(("hmR0SvmLoadGuestStateNested: CS:RIP=%04x:%RX64 EFL=%#x CR0=%#RX32 CR3=%#RX32 (HyperCR3=%#RX64) CR4=%#RX32 rc=%d\n",
          pCtx->cs.Sel, pCtx->rip, pCtx->eflags.u, pCtx->cr0, pCtx->cr3, pVmcbNstGst->guest.u64CR3, pCtx->cr4, rc));
    STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatLoadGuestState, x);
    return rc;
}
#endif


/**
 * Loads the state shared between the host and guest or nested-guest into the
 * VMCB.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pVmcb       Pointer to the VM control block.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
static void hmR0SvmLoadSharedState(PVMCPU pVCpu, PSVMVMCB pVmcb, PCPUMCTX pCtx)
{
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    Assert(!VMMRZCallRing3IsEnabled(pVCpu));

    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_CR0))
    {
#ifdef VBOX_WITH_NESTED_HWVIRT
        /* We use nested-guest CR0 unmodified, hence nothing to do here. */
        if (!CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
            hmR0SvmLoadSharedCR0(pVCpu, pVmcb, pCtx);
        else
            Assert(pVmcb->guest.u64CR0 == pCtx->cr0);
#else
        hmR0SvmLoadSharedCR0(pVCpu, pVmcb, pCtx);
#endif
        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_CR0);
    }

    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_GUEST_DEBUG))
    {
        /* We use nested-guest CR0 unmodified, hence nothing to do here. */
        if (!CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
            hmR0SvmLoadSharedDebugState(pVCpu, pVmcb, pCtx);
        else
        {
            pVmcb->guest.u64DR6 = pCtx->dr[6];
            pVmcb->guest.u64DR7 = pCtx->dr[7];
            Log4(("hmR0SvmLoadSharedState: DR6=%#RX64 DR7=%#RX64\n", pCtx->dr[6], pCtx->dr[7]));
        }

        HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_DEBUG);
    }

    /* Unused on AMD-V. */
    HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_GUEST_LAZY_MSRS);

    AssertMsg(!HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_HOST_GUEST_SHARED_STATE),
              ("fContextUseFlags=%#RX32\n", HMCPU_CF_VALUE(pVCpu)));
}


/**
 * Saves the guest (or nested-guest) state from the VMCB into the guest-CPU context.
 *
 * Currently there is no residual state left in the CPU that is not updated in the
 * VMCB.
 *
 * @returns VBox status code.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pMixedCtx       Pointer to the guest-CPU context. The data may be
 *                          out-of-sync. Make sure to update the required fields
 *                          before using them.
 * @param   pVmcb           Pointer to the VM control block.
 */
static void hmR0SvmSaveGuestState(PVMCPU pVCpu, PCPUMCTX pMixedCtx, PCSVMVMCB pVmcb)
{
    Assert(VMMRZCallRing3IsEnabled(pVCpu));

    pMixedCtx->rip        = pVmcb->guest.u64RIP;
    pMixedCtx->rsp        = pVmcb->guest.u64RSP;
    pMixedCtx->eflags.u32 = pVmcb->guest.u64RFlags;
    pMixedCtx->rax        = pVmcb->guest.u64RAX;

    /*
     * Guest interrupt shadow.
     */
    if (pVmcb->ctrl.u64IntShadow & SVM_INTERRUPT_SHADOW_ACTIVE)
        EMSetInhibitInterruptsPC(pVCpu, pMixedCtx->rip);
    else if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS))
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);

    /*
     * Guest Control registers: CR2, CR3 (handled at the end) - accesses to other control registers are always intercepted.
     */
    pMixedCtx->cr2        = pVmcb->guest.u64CR2;

#ifdef VBOX_WITH_NESTED_GUEST
    /*
     * The nested hypervisor might not be intercepting these control registers,
     */
    if (CPUMIsGuestInNestedHwVirtMode(pMixedCtx))
    {
        pMixedCtx->cr4    = pVmcb->guest.u64CR4;
        pMixedCtx->cr0    = pVmcb->guest.u64CR0;
    }
#endif

    /*
     * Guest MSRs.
     */
    pMixedCtx->msrSTAR         = pVmcb->guest.u64STAR;            /* legacy syscall eip, cs & ss */
    pMixedCtx->msrLSTAR        = pVmcb->guest.u64LSTAR;           /* 64-bit mode syscall rip */
    pMixedCtx->msrCSTAR        = pVmcb->guest.u64CSTAR;           /* compatibility mode syscall rip */
    pMixedCtx->msrSFMASK       = pVmcb->guest.u64SFMASK;          /* syscall flag mask */
    pMixedCtx->msrKERNELGSBASE = pVmcb->guest.u64KernelGSBase;    /* swapgs exchange value */
    pMixedCtx->SysEnter.cs     = pVmcb->guest.u64SysEnterCS;
    pMixedCtx->SysEnter.eip    = pVmcb->guest.u64SysEnterEIP;
    pMixedCtx->SysEnter.esp    = pVmcb->guest.u64SysEnterESP;

    /*
     * Guest segment registers (includes FS, GS base MSRs for 64-bit guests).
     */
    HMSVM_SEG_REG_COPY_FROM_VMCB(pMixedCtx, &pVmcb->guest, CS, cs);
    HMSVM_SEG_REG_COPY_FROM_VMCB(pMixedCtx, &pVmcb->guest, SS, ss);
    HMSVM_SEG_REG_COPY_FROM_VMCB(pMixedCtx, &pVmcb->guest, DS, ds);
    HMSVM_SEG_REG_COPY_FROM_VMCB(pMixedCtx, &pVmcb->guest, ES, es);
    HMSVM_SEG_REG_COPY_FROM_VMCB(pMixedCtx, &pVmcb->guest, FS, fs);
    HMSVM_SEG_REG_COPY_FROM_VMCB(pMixedCtx, &pVmcb->guest, GS, gs);

    /*
     * Correct the hidden CS granularity bit. Haven't seen it being wrong in any other
     * register (yet).
     */
    /** @todo SELM might need to be fixed as it too should not care about the
     *        granularity bit. See @bugref{6785}. */
    if (   !pMixedCtx->cs.Attr.n.u1Granularity
        && pMixedCtx->cs.Attr.n.u1Present
        && pMixedCtx->cs.u32Limit > UINT32_C(0xfffff))
    {
        Assert((pMixedCtx->cs.u32Limit & 0xfff) == 0xfff);
        pMixedCtx->cs.Attr.n.u1Granularity = 1;
    }

#ifdef VBOX_STRICT
# define HMSVM_ASSERT_SEG_GRANULARITY(reg) \
    AssertMsg(   !pMixedCtx->reg.Attr.n.u1Present \
              || (   pMixedCtx->reg.Attr.n.u1Granularity \
                  ? (pMixedCtx->reg.u32Limit & 0xfff) == 0xfff \
                  :  pMixedCtx->reg.u32Limit <= UINT32_C(0xfffff)), \
              ("Invalid Segment Attributes Limit=%#RX32 Attr=%#RX32 Base=%#RX64\n", pMixedCtx->reg.u32Limit, \
              pMixedCtx->reg.Attr.u, pMixedCtx->reg.u64Base))

    HMSVM_ASSERT_SEG_GRANULARITY(cs);
    HMSVM_ASSERT_SEG_GRANULARITY(ss);
    HMSVM_ASSERT_SEG_GRANULARITY(ds);
    HMSVM_ASSERT_SEG_GRANULARITY(es);
    HMSVM_ASSERT_SEG_GRANULARITY(fs);
    HMSVM_ASSERT_SEG_GRANULARITY(gs);

# undef HMSVM_ASSERT_SEL_GRANULARITY
#endif

    /*
     * Sync the hidden SS DPL field. AMD CPUs have a separate CPL field in the VMCB and uses that
     * and thus it's possible that when the CPL changes during guest execution that the SS DPL
     * isn't updated by AMD-V. Observed on some AMD Fusion CPUs with 64-bit guests.
     * See AMD spec. 15.5.1 "Basic operation".
     */
    Assert(!(pVmcb->guest.u8CPL & ~0x3));
    pMixedCtx->ss.Attr.n.u2Dpl = pVmcb->guest.u8CPL & 0x3;

    /*
     * Guest TR.
     * Fixup TR attributes so it's compatible with Intel. Important when saved-states are used
     * between Intel and AMD. See @bugref{6208#c39}.
     * ASSUME that it's normally correct and that we're in 32-bit or 64-bit mode.
     */
    HMSVM_SEG_REG_COPY_FROM_VMCB(pMixedCtx, &pVmcb->guest, TR, tr);
    if (pMixedCtx->tr.Attr.n.u4Type != X86_SEL_TYPE_SYS_386_TSS_BUSY)
    {
        if (   pMixedCtx->tr.Attr.n.u4Type == X86_SEL_TYPE_SYS_386_TSS_AVAIL
            || CPUMIsGuestInLongModeEx(pMixedCtx))
            pMixedCtx->tr.Attr.n.u4Type = X86_SEL_TYPE_SYS_386_TSS_BUSY;
        else if (pMixedCtx->tr.Attr.n.u4Type == X86_SEL_TYPE_SYS_286_TSS_AVAIL)
            pMixedCtx->tr.Attr.n.u4Type = X86_SEL_TYPE_SYS_286_TSS_BUSY;
    }

    /*
     * Guest Descriptor-Table registers.
     */
    HMSVM_SEG_REG_COPY_FROM_VMCB(pMixedCtx, &pVmcb->guest, LDTR, ldtr);
    pMixedCtx->gdtr.cbGdt = pVmcb->guest.GDTR.u32Limit;
    pMixedCtx->gdtr.pGdt  = pVmcb->guest.GDTR.u64Base;

    pMixedCtx->idtr.cbIdt = pVmcb->guest.IDTR.u32Limit;
    pMixedCtx->idtr.pIdt  = pVmcb->guest.IDTR.u64Base;

    /*
     * Guest Debug registers.
     */
    if (!pVCpu->hm.s.fUsingHyperDR7)
    {
        pMixedCtx->dr[6] = pVmcb->guest.u64DR6;
        pMixedCtx->dr[7] = pVmcb->guest.u64DR7;
    }
    else
    {
        Assert(pVmcb->guest.u64DR7 == CPUMGetHyperDR7(pVCpu));
        CPUMSetHyperDR6(pVCpu, pVmcb->guest.u64DR6);
    }

    /*
     * With Nested Paging, CR3 changes are not intercepted. Therefore, sync. it now.
     * This is done as the very last step of syncing the guest state, as PGMUpdateCR3() may cause longjmp's to ring-3.
     */
    if (   pVmcb->ctrl.NestedPaging.n.u1NestedPaging
        && pMixedCtx->cr3 != pVmcb->guest.u64CR3)
    {
        CPUMSetGuestCR3(pVCpu, pVmcb->guest.u64CR3);
        PGMUpdateCR3(pVCpu,    pVmcb->guest.u64CR3);
    }
}


/**
 * Does the necessary state syncing before returning to ring-3 for any reason
 * (longjmp, preemption, voluntary exits to ring-3) from AMD-V.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 *
 * @remarks No-long-jmp zone!!!
 */
static void hmR0SvmLeave(PVMCPU pVCpu)
{
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    Assert(!VMMRZCallRing3IsEnabled(pVCpu));
    Assert(VMMR0IsLogFlushDisabled(pVCpu));

    /*
     * !!! IMPORTANT !!!
     * If you modify code here, make sure to check whether hmR0SvmCallRing3Callback() needs to be updated too.
     */

    /* Restore host FPU state if necessary and resync on next R0 reentry .*/
    if (CPUMR0FpuStateMaybeSaveGuestAndRestoreHost(pVCpu))
        HMCPU_CF_SET(pVCpu, HM_CHANGED_GUEST_CR0);

    /*
     * Restore host debug registers if necessary and resync on next R0 reentry.
     */
#ifdef VBOX_STRICT
    if (CPUMIsHyperDebugStateActive(pVCpu))
    {
        PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        Assert(pVmcb->ctrl.u16InterceptRdDRx == 0xffff);
        Assert(pVmcb->ctrl.u16InterceptWrDRx == 0xffff);
    }
#endif
    if (CPUMR0DebugStateMaybeSaveGuestAndRestoreHost(pVCpu, false /* save DR6 */))
        HMCPU_CF_SET(pVCpu, HM_CHANGED_GUEST_DEBUG);

    Assert(!CPUMIsHyperDebugStateActive(pVCpu));
    Assert(!CPUMIsGuestDebugStateActive(pVCpu));

    STAM_PROFILE_ADV_SET_STOPPED(&pVCpu->hm.s.StatEntry);
    STAM_PROFILE_ADV_SET_STOPPED(&pVCpu->hm.s.StatLoadGuestState);
    STAM_PROFILE_ADV_SET_STOPPED(&pVCpu->hm.s.StatExit1);
    STAM_PROFILE_ADV_SET_STOPPED(&pVCpu->hm.s.StatExit2);
    STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchLongJmpToR3);

    VMCPU_CMPXCHG_STATE(pVCpu, VMCPUSTATE_STARTED_HM, VMCPUSTATE_STARTED_EXEC);
}


/**
 * Leaves the AMD-V session.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
static int hmR0SvmLeaveSession(PVMCPU pVCpu)
{
    HM_DISABLE_PREEMPT();
    Assert(!VMMRZCallRing3IsEnabled(pVCpu));
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));

    /* When thread-context hooks are used, we can avoid doing the leave again if we had been preempted before
       and done this from the SVMR0ThreadCtxCallback(). */
    if (!pVCpu->hm.s.fLeaveDone)
    {
        hmR0SvmLeave(pVCpu);
        pVCpu->hm.s.fLeaveDone = true;
    }

    /*
     * !!! IMPORTANT !!!
     * If you modify code here, make sure to check whether hmR0SvmCallRing3Callback() needs to be updated too.
     */

    /** @todo eliminate the need for calling VMMR0ThreadCtxHookDisable here!  */
    /* Deregister hook now that we've left HM context before re-enabling preemption. */
    VMMR0ThreadCtxHookDisable(pVCpu);

    /* Leave HM context. This takes care of local init (term). */
    int rc = HMR0LeaveCpu(pVCpu);

    HM_RESTORE_PREEMPT();
    return rc;
}


/**
 * Does the necessary state syncing before doing a longjmp to ring-3.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 *
 * @remarks No-long-jmp zone!!!
 */
static int hmR0SvmLongJmpToRing3(PVMCPU pVCpu)
{
    return hmR0SvmLeaveSession(pVCpu);
}


/**
 * VMMRZCallRing3() callback wrapper which saves the guest state (or restores
 * any remaining host state) before we longjump to ring-3 and possibly get
 * preempted.
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   enmOperation    The operation causing the ring-3 longjump.
 * @param   pvUser          The user argument (pointer to the possibly
 *                          out-of-date guest-CPU context).
 */
static DECLCALLBACK(int) hmR0SvmCallRing3Callback(PVMCPU pVCpu, VMMCALLRING3 enmOperation, void *pvUser)
{
    RT_NOREF_PV(pvUser);

    if (enmOperation == VMMCALLRING3_VM_R0_ASSERTION)
    {
        /*
         * !!! IMPORTANT !!!
         * If you modify code here, make sure to check whether hmR0SvmLeave() and hmR0SvmLeaveSession() needs
         * to be updated too. This is a stripped down version which gets out ASAP trying to not trigger any assertion.
         */
        VMMRZCallRing3RemoveNotification(pVCpu);
        VMMRZCallRing3Disable(pVCpu);
        HM_DISABLE_PREEMPT();

        /* Restore host FPU state if necessary and resync on next R0 reentry. */
        CPUMR0FpuStateMaybeSaveGuestAndRestoreHost(pVCpu);

        /* Restore host debug registers if necessary and resync on next R0 reentry. */
        CPUMR0DebugStateMaybeSaveGuestAndRestoreHost(pVCpu, false /* save DR6 */);

        /* Deregister the hook now that we've left HM context before re-enabling preemption. */
        /** @todo eliminate the need for calling VMMR0ThreadCtxHookDisable here!  */
        VMMR0ThreadCtxHookDisable(pVCpu);

        /* Leave HM context. This takes care of local init (term). */
        HMR0LeaveCpu(pVCpu);

        HM_RESTORE_PREEMPT();
        return VINF_SUCCESS;
    }

    Assert(pVCpu);
    Assert(pvUser);
    Assert(VMMRZCallRing3IsEnabled(pVCpu));
    HMSVM_ASSERT_PREEMPT_SAFE();

    VMMRZCallRing3Disable(pVCpu);
    Assert(VMMR0IsLogFlushDisabled(pVCpu));

    Log4(("hmR0SvmCallRing3Callback->hmR0SvmLongJmpToRing3\n"));
    int rc = hmR0SvmLongJmpToRing3(pVCpu);
    AssertRCReturn(rc, rc);

    VMMRZCallRing3Enable(pVCpu);
    return VINF_SUCCESS;
}


/**
 * Take necessary actions before going back to ring-3.
 *
 * An action requires us to go back to ring-3. This function does the necessary
 * steps before we can safely return to ring-3. This is not the same as longjmps
 * to ring-3, this is voluntary.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   rcExit      The reason for exiting to ring-3. Can be
 *                      VINF_VMM_UNKNOWN_RING3_CALL.
 */
static int hmR0SvmExitToRing3(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, int rcExit)
{
    Assert(pVM);
    Assert(pVCpu);
    Assert(pCtx);
    HMSVM_ASSERT_PREEMPT_SAFE();

    /* Please, no longjumps here (any logging shouldn't flush jump back to ring-3). NO LOGGING BEFORE THIS POINT! */
    VMMRZCallRing3Disable(pVCpu);
    Log4(("hmR0SvmExitToRing3: VCPU[%u]: rcExit=%d LocalFF=%#RX32 GlobalFF=%#RX32\n", pVCpu->idCpu, rcExit,
          pVCpu->fLocalForcedActions, pVM->fGlobalForcedActions));

    /* We need to do this only while truly exiting the "inner loop" back to ring-3 and -not- for any longjmp to ring3. */
    if (pVCpu->hm.s.Event.fPending)
    {
        hmR0SvmPendingEventToTrpmTrap(pVCpu);
        Assert(!pVCpu->hm.s.Event.fPending);
    }

    /* Sync. the necessary state for going back to ring-3. */
    hmR0SvmLeaveSession(pVCpu);
    STAM_COUNTER_DEC(&pVCpu->hm.s.StatSwitchLongJmpToR3);

    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_TO_R3);
    CPUMSetChangedFlags(pVCpu,  CPUM_CHANGED_SYSENTER_MSR
                              | CPUM_CHANGED_LDTR
                              | CPUM_CHANGED_GDTR
                              | CPUM_CHANGED_IDTR
                              | CPUM_CHANGED_TR
                              | CPUM_CHANGED_HIDDEN_SEL_REGS);
    if (   pVM->hm.s.fNestedPaging
        && CPUMIsGuestPagingEnabledEx(pCtx))
    {
        CPUMSetChangedFlags(pVCpu, CPUM_CHANGED_GLOBAL_TLB_FLUSH);
    }

    /* On our way back from ring-3 reload the guest state if there is a possibility of it being changed. */
    if (rcExit != VINF_EM_RAW_INTERRUPT)
        HMCPU_CF_SET(pVCpu, HM_CHANGED_ALL_GUEST);

#ifdef VBOX_WITH_NESTED_HWVIRT
    /*
     * We may inspect the nested-guest VMCB state in ring-3 (e.g. for injecting interrupts)
     * and thus we need to restore any modifications we may have made to it here if we're
     * still executing the nested-guest.
     */
    if (CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
        HMSvmNstGstVmExitNotify(pVCpu, pCtx);
#endif

    STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchExitToR3);

    /* We do -not- want any longjmp notifications after this! We must return to ring-3 ASAP. */
    VMMRZCallRing3RemoveNotification(pVCpu);
    VMMRZCallRing3Enable(pVCpu);

    /*
     * If we're emulating an instruction, we shouldn't have any TRPM traps pending
     * and if we're injecting an event we should have a TRPM trap pending.
     */
    AssertReturnStmt(rcExit != VINF_EM_RAW_INJECT_TRPM_EVENT || TRPMHasTrap(pVCpu),
                     pVCpu->hm.s.u32HMError = rcExit,
                     VERR_SVM_IPE_5);
    AssertReturnStmt(rcExit != VINF_EM_RAW_EMULATE_INSTR || !TRPMHasTrap(pVCpu),
                     pVCpu->hm.s.u32HMError = rcExit,
                     VERR_SVM_IPE_4);

    return rcExit;
}


/**
 * Updates the use of TSC offsetting mode for the CPU and adjusts the necessary
 * intercepts.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pVmcb       Pointer to the VM control block.
 *
 * @remarks No-long-jump zone!!!
 */
static void hmR0SvmUpdateTscOffsetting(PVM pVM, PVMCPU pVCpu, PSVMVMCB pVmcb)
{
    bool fParavirtTsc;
    bool fCanUseRealTsc = TMCpuTickCanUseRealTSC(pVM, pVCpu, &pVmcb->ctrl.u64TSCOffset, &fParavirtTsc);
    if (fCanUseRealTsc)
    {
        pVmcb->ctrl.u64InterceptCtrl &= ~SVM_CTRL_INTERCEPT_RDTSC;
        pVmcb->ctrl.u64InterceptCtrl &= ~SVM_CTRL_INTERCEPT_RDTSCP;
        STAM_COUNTER_INC(&pVCpu->hm.s.StatTscOffset);
    }
    else
    {
        pVmcb->ctrl.u64InterceptCtrl |= SVM_CTRL_INTERCEPT_RDTSC;
        pVmcb->ctrl.u64InterceptCtrl |= SVM_CTRL_INTERCEPT_RDTSCP;
        STAM_COUNTER_INC(&pVCpu->hm.s.StatTscIntercept);
    }
    pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_INTERCEPTS;

    /** @todo later optimize this to be done elsewhere and not before every
     *        VM-entry. */
    if (fParavirtTsc)
    {
        /* Currently neither Hyper-V nor KVM need to update their paravirt. TSC
           information before every VM-entry, hence disable it for performance sake. */
#if 0
        int rc = GIMR0UpdateParavirtTsc(pVM, 0 /* u64Offset */);
        AssertRC(rc);
#endif
        STAM_COUNTER_INC(&pVCpu->hm.s.StatTscParavirt);
    }
}


/**
 * Sets an event as a pending event to be injected into the guest.
 *
 * @param   pVCpu               The cross context virtual CPU structure.
 * @param   pEvent              Pointer to the SVM event.
 * @param   GCPtrFaultAddress   The fault-address (CR2) in case it's a
 *                              page-fault.
 *
 * @remarks Statistics counter assumes this is a guest event being reflected to
 *          the guest i.e. 'StatInjectPendingReflect' is incremented always.
 */
DECLINLINE(void) hmR0SvmSetPendingEvent(PVMCPU pVCpu, PSVMEVENT pEvent, RTGCUINTPTR GCPtrFaultAddress)
{
    Assert(!pVCpu->hm.s.Event.fPending);
    Assert(pEvent->n.u1Valid);

    pVCpu->hm.s.Event.u64IntInfo        = pEvent->u;
    pVCpu->hm.s.Event.fPending          = true;
    pVCpu->hm.s.Event.GCPtrFaultAddress = GCPtrFaultAddress;

    Log4(("hmR0SvmSetPendingEvent: u=%#RX64 u8Vector=%#x Type=%#x ErrorCodeValid=%RTbool ErrorCode=%#RX32\n", pEvent->u,
          pEvent->n.u8Vector, (uint8_t)pEvent->n.u3Type, !!pEvent->n.u1ErrorCodeValid, pEvent->n.u32ErrorCode));
}


/**
 * Sets an invalid-opcode (\#UD) exception as pending-for-injection into the VM.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(void) hmR0SvmSetPendingXcptUD(PVMCPU pVCpu)
{
    SVMEVENT Event;
    Event.u          = 0;
    Event.n.u1Valid  = 1;
    Event.n.u3Type   = SVM_EVENT_EXCEPTION;
    Event.n.u8Vector = X86_XCPT_UD;
    hmR0SvmSetPendingEvent(pVCpu, &Event, 0 /* GCPtrFaultAddress */);
}


/**
 * Sets a debug (\#DB) exception as pending-for-injection into the VM.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(void) hmR0SvmSetPendingXcptDB(PVMCPU pVCpu)
{
    SVMEVENT Event;
    Event.u          = 0;
    Event.n.u1Valid  = 1;
    Event.n.u3Type   = SVM_EVENT_EXCEPTION;
    Event.n.u8Vector = X86_XCPT_DB;
    hmR0SvmSetPendingEvent(pVCpu, &Event, 0 /* GCPtrFaultAddress */);
}


/**
 * Sets a page fault (\#PF) exception as pending-for-injection into the VM.
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   u32ErrCode      The error-code for the page-fault.
 * @param   uFaultAddress   The page fault address (CR2).
 *
 * @remarks This updates the guest CR2 with @a uFaultAddress!
 */
DECLINLINE(void) hmR0SvmSetPendingXcptPF(PVMCPU pVCpu, PCPUMCTX pCtx, uint32_t u32ErrCode, RTGCUINTPTR uFaultAddress)
{
    SVMEVENT Event;
    Event.u                  = 0;
    Event.n.u1Valid          = 1;
    Event.n.u3Type           = SVM_EVENT_EXCEPTION;
    Event.n.u8Vector         = X86_XCPT_PF;
    Event.n.u1ErrorCodeValid = 1;
    Event.n.u32ErrorCode     = u32ErrCode;

    /* Update CR2 of the guest. */
    if (pCtx->cr2 != uFaultAddress)
    {
        pCtx->cr2 = uFaultAddress;
        HMCPU_CF_SET(pVCpu, HM_CHANGED_GUEST_CR2);
    }

    hmR0SvmSetPendingEvent(pVCpu, &Event, uFaultAddress);
}


/**
 * Sets a math-fault (\#MF) exception as pending-for-injection into the VM.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(void) hmR0SvmSetPendingXcptMF(PVMCPU pVCpu)
{
    SVMEVENT Event;
    Event.u          = 0;
    Event.n.u1Valid  = 1;
    Event.n.u3Type   = SVM_EVENT_EXCEPTION;
    Event.n.u8Vector = X86_XCPT_MF;
    hmR0SvmSetPendingEvent(pVCpu, &Event, 0 /* GCPtrFaultAddress */);
}


/**
 * Sets a double fault (\#DF) exception as pending-for-injection into the VM.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(void) hmR0SvmSetPendingXcptDF(PVMCPU pVCpu)
{
    SVMEVENT Event;
    Event.u                  = 0;
    Event.n.u1Valid          = 1;
    Event.n.u3Type           = SVM_EVENT_EXCEPTION;
    Event.n.u8Vector         = X86_XCPT_DF;
    Event.n.u1ErrorCodeValid = 1;
    Event.n.u32ErrorCode     = 0;
    hmR0SvmSetPendingEvent(pVCpu, &Event, 0 /* GCPtrFaultAddress */);
}


/**
 * Injects an event into the guest upon VMRUN by updating the relevant field
 * in the VMCB.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pVmcb       Pointer to the guest VM control block.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   pEvent      Pointer to the event.
 *
 * @remarks No-long-jump zone!!!
 * @remarks Requires CR0!
 */
DECLINLINE(void) hmR0SvmInjectEventVmcb(PVMCPU pVCpu, PSVMVMCB pVmcb, PCPUMCTX pCtx, PSVMEVENT pEvent)
{
    NOREF(pVCpu); NOREF(pCtx);

    pVmcb->ctrl.EventInject.u = pEvent->u;
    STAM_COUNTER_INC(&pVCpu->hm.s.paStatInjectedIrqsR0[pEvent->n.u8Vector & MASK_INJECT_IRQ_STAT]);

    Log4(("hmR0SvmInjectEventVmcb: u=%#RX64 u8Vector=%#x Type=%#x ErrorCodeValid=%RTbool ErrorCode=%#RX32\n", pEvent->u,
          pEvent->n.u8Vector, (uint8_t)pEvent->n.u3Type, !!pEvent->n.u1ErrorCodeValid, pEvent->n.u32ErrorCode));
}



/**
 * Converts any TRPM trap into a pending HM event. This is typically used when
 * entering from ring-3 (not longjmp returns).
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 */
static void hmR0SvmTrpmTrapToPendingEvent(PVMCPU pVCpu)
{
    Assert(TRPMHasTrap(pVCpu));
    Assert(!pVCpu->hm.s.Event.fPending);

    uint8_t     uVector;
    TRPMEVENT   enmTrpmEvent;
    RTGCUINT    uErrCode;
    RTGCUINTPTR GCPtrFaultAddress;
    uint8_t     cbInstr;

    int rc = TRPMQueryTrapAll(pVCpu, &uVector, &enmTrpmEvent, &uErrCode, &GCPtrFaultAddress, &cbInstr);
    AssertRC(rc);

    SVMEVENT Event;
    Event.u          = 0;
    Event.n.u1Valid  = 1;
    Event.n.u8Vector = uVector;

    /* Refer AMD spec. 15.20 "Event Injection" for the format. */
    if (enmTrpmEvent == TRPM_TRAP)
    {
        Event.n.u3Type = SVM_EVENT_EXCEPTION;
        switch (uVector)
        {
            case X86_XCPT_NMI:
            {
                Event.n.u3Type = SVM_EVENT_NMI;
                break;
            }

            case X86_XCPT_PF:
            case X86_XCPT_DF:
            case X86_XCPT_TS:
            case X86_XCPT_NP:
            case X86_XCPT_SS:
            case X86_XCPT_GP:
            case X86_XCPT_AC:
            {
                Event.n.u1ErrorCodeValid = 1;
                Event.n.u32ErrorCode     = uErrCode;
                break;
            }
        }
    }
    else if (enmTrpmEvent == TRPM_HARDWARE_INT)
        Event.n.u3Type = SVM_EVENT_EXTERNAL_IRQ;
    else if (enmTrpmEvent == TRPM_SOFTWARE_INT)
        Event.n.u3Type = SVM_EVENT_SOFTWARE_INT;
    else
        AssertMsgFailed(("Invalid TRPM event type %d\n", enmTrpmEvent));

    rc = TRPMResetTrap(pVCpu);
    AssertRC(rc);

    Log4(("TRPM->HM event: u=%#RX64 u8Vector=%#x uErrorCodeValid=%RTbool uErrorCode=%#RX32\n", Event.u, Event.n.u8Vector,
          !!Event.n.u1ErrorCodeValid, Event.n.u32ErrorCode));

    hmR0SvmSetPendingEvent(pVCpu, &Event, GCPtrFaultAddress);
}


/**
 * Converts any pending SVM event into a TRPM trap. Typically used when leaving
 * AMD-V to execute any instruction.
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 */
static void hmR0SvmPendingEventToTrpmTrap(PVMCPU pVCpu)
{
    Assert(pVCpu->hm.s.Event.fPending);
    Assert(TRPMQueryTrap(pVCpu, NULL /* pu8TrapNo */, NULL /* pEnmType */) == VERR_TRPM_NO_ACTIVE_TRAP);

    SVMEVENT Event;
    Event.u = pVCpu->hm.s.Event.u64IntInfo;

    uint8_t   uVector     = Event.n.u8Vector;
    uint8_t   uVectorType = Event.n.u3Type;
    TRPMEVENT enmTrapType = HMSvmEventToTrpmEventType(&Event);

    Log4(("HM event->TRPM: uVector=%#x enmTrapType=%d\n", uVector, uVectorType));

    int rc = TRPMAssertTrap(pVCpu, uVector, enmTrapType);
    AssertRC(rc);

    if (Event.n.u1ErrorCodeValid)
        TRPMSetErrorCode(pVCpu, Event.n.u32ErrorCode);

    if (   uVectorType == SVM_EVENT_EXCEPTION
        && uVector     == X86_XCPT_PF)
    {
        TRPMSetFaultAddress(pVCpu, pVCpu->hm.s.Event.GCPtrFaultAddress);
        Assert(pVCpu->hm.s.Event.GCPtrFaultAddress == CPUMGetGuestCR2(pVCpu));
    }
    else if (uVectorType == SVM_EVENT_SOFTWARE_INT)
    {
        AssertMsg(   uVectorType == SVM_EVENT_SOFTWARE_INT
                  || (uVector == X86_XCPT_BP || uVector == X86_XCPT_OF),
                  ("Invalid vector: uVector=%#x uVectorType=%#x\n", uVector, uVectorType));
        TRPMSetInstrLength(pVCpu, pVCpu->hm.s.Event.cbInstr);
    }
    pVCpu->hm.s.Event.fPending = false;
}


/**
 * Advances the guest RIP by the number of bytes specified in @a cb.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   cb          RIP increment value in bytes.
 */
DECLINLINE(void) hmR0SvmAdvanceRip(PVMCPU pVCpu, PCPUMCTX pCtx, uint32_t cb)
{
    pCtx->rip += cb;

    /* Update interrupt shadow. */
    if (   VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS)
        && pCtx->rip != EMGetInhibitInterruptsPC(pVCpu))
        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
}


/**
 * Checks if the guest (or nested-guest) has an interrupt shadow active right
 * now.
 *
 * @returns true if the interrupt shadow is active, false otherwise.
 * @param   pVCpu   The cross context virtual CPU structure.
 * @param   pCtx    Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 * @remarks Has side-effects with VMCPU_FF_INHIBIT_INTERRUPTS force-flag.
 */
DECLINLINE(bool) hmR0SvmIsIntrShadowActive(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /*
     * Instructions like STI and MOV SS inhibit interrupts till the next instruction completes. Check if we should
     * inhibit interrupts or clear any existing interrupt-inhibition.
     */
    if (VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS))
    {
        if (pCtx->rip != EMGetInhibitInterruptsPC(pVCpu))
        {
            /*
             * We can clear the inhibit force flag as even if we go back to the recompiler without executing guest code in
             * AMD-V, the flag's condition to be cleared is met and thus the cleared state is correct.
             */
            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INHIBIT_INTERRUPTS);
            return false;
        }
        return true;
    }
    return false;
}


/**
 * Sets the virtual interrupt intercept control in the VMCB which
 * instructs AMD-V to cause a \#VMEXIT as soon as the guest is in a state to
 * receive interrupts.
 *
 * @param   pVmcb       Pointer to the VM control block.
 */
DECLINLINE(void) hmR0SvmSetVirtIntrIntercept(PSVMVMCB pVmcb)
{
    if (!(pVmcb->ctrl.u64InterceptCtrl & SVM_CTRL_INTERCEPT_VINTR))
    {
        pVmcb->ctrl.IntCtrl.n.u1VIrqPending = 1;     /* A virtual interrupt is pending. */
        pVmcb->ctrl.IntCtrl.n.u8VIntrVector = 0;     /* Vector not necessary as we #VMEXIT for delivering the interrupt. */
        pVmcb->ctrl.u64InterceptCtrl |= SVM_CTRL_INTERCEPT_VINTR;
        pVmcb->ctrl.u64VmcbCleanBits &= ~(HMSVM_VMCB_CLEAN_INTERCEPTS | HMSVM_VMCB_CLEAN_TPR);

        Log4(("Setting VINTR intercept\n"));
    }
}


#if 0
/**
 * Clears the virtual interrupt intercept control in the VMCB as
 * we are figured the guest is unable process any interrupts
 * at this point of time.
 *
 * @param   pVmcb       Pointer to the VM control block.
 */
DECLINLINE(void) hmR0SvmClearVirtIntrIntercept(PSVMVMCB pVmcb)
{
    if (pVmcb->ctrl.u64InterceptCtrl & SVM_CTRL_INTERCEPT_VINTR)
    {
        pVmcb->ctrl.u64InterceptCtrl &= ~SVM_CTRL_INTERCEPT_VINTR;
        pVmcb->ctrl.u64VmcbCleanBits &= ~(HMSVM_VMCB_CLEAN_INTERCEPTS);
        Log4(("Clearing VINTR intercept\n"));
    }
}
#endif


/**
 * Sets the IRET intercept control in the VMCB which instructs AMD-V to cause a
 * \#VMEXIT as soon as a guest starts executing an IRET. This is used to unblock
 * virtual NMIs.
 *
 * @param   pVmcb       Pointer to the VM control block.
 */
DECLINLINE(void) hmR0SvmSetIretIntercept(PSVMVMCB pVmcb)
{
    if (!(pVmcb->ctrl.u64InterceptCtrl & SVM_CTRL_INTERCEPT_IRET))
    {
        pVmcb->ctrl.u64InterceptCtrl |= SVM_CTRL_INTERCEPT_IRET;
        pVmcb->ctrl.u64VmcbCleanBits &= ~(HMSVM_VMCB_CLEAN_INTERCEPTS);

        Log4(("Setting IRET intercept\n"));
    }
}


/**
 * Clears the IRET intercept control in the VMCB.
 *
 * @param   pVmcb       Pointer to the VM control block.
 */
DECLINLINE(void) hmR0SvmClearIretIntercept(PSVMVMCB pVmcb)
{
    if (pVmcb->ctrl.u64InterceptCtrl & SVM_CTRL_INTERCEPT_IRET)
    {
        pVmcb->ctrl.u64InterceptCtrl &= ~SVM_CTRL_INTERCEPT_IRET;
        pVmcb->ctrl.u64VmcbCleanBits &= ~(HMSVM_VMCB_CLEAN_INTERCEPTS);

        Log4(("Clearing IRET intercept\n"));
    }
}

#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Checks whether the SVM nested-guest is in a state to receive physical (APIC)
 * interrupts.
 *
 * @returns true if it's ready, false otherwise.
 * @param   pCtx        The guest-CPU context.
 *
 * @remarks This function looks at the VMCB cache rather than directly at the
 *          nested-guest VMCB which may have been suitably modified for executing
 *          using hardware-assisted SVM.
 */
static bool hmR0SvmCanNstGstTakePhysIntr(PVMCPU pVCpu, PCCPUMCTX pCtx)
{
    Assert(pCtx->hwvirt.svm.fHMCachedVmcb);
    PCSVMNESTEDVMCBCACHE pVmcbNstGstCache = &pVCpu->hm.s.svm.NstGstVmcbCache;
    X86EFLAGS fEFlags;
    if (pVmcbNstGstCache->fVIntrMasking)
        fEFlags.u = pCtx->hwvirt.svm.HostState.rflags.u;
    else
        fEFlags.u = pCtx->eflags.u;

    return fEFlags.Bits.u1IF;
}


/**
 * Evaluates the event to be delivered to the nested-guest and sets it as the
 * pending event.
 *
 * @returns VBox strict status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 */
static VBOXSTRICTRC hmR0SvmEvaluatePendingEventNested(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    Log4Func(("\n"));

    Assert(!pVCpu->hm.s.Event.fPending);

    bool const fGif = pCtx->hwvirt.svm.fGif;
    if (fGif)
    {
        PSVMVMCB pVmcbNstGst = pCtx->hwvirt.svm.CTX_SUFF(pVmcb);

        bool const fIntShadow = hmR0SvmIsIntrShadowActive(pVCpu, pCtx);

        /*
         * Check if the nested-guest can receive NMIs.
         * NMIs are higher priority than regular interrupts.
         */
        /** @todo SMI. SMIs take priority over NMIs. */
        if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_INTERRUPT_NMI))
        {
            bool const fBlockNmi = VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_BLOCK_NMIS);
            if (fBlockNmi)
                hmR0SvmSetIretIntercept(pVmcbNstGst);
            else if (fIntShadow)
            {
                /** @todo Figure this out, how we shall manage virt. intercept if the
                 *        nested-guest already has one set and/or if we really need it? */
                //hmR0SvmSetVirtIntrIntercept(pVmcbNstGst);
            }
            else
            {
                Log4(("Pending NMI\n"));

                SVMEVENT Event;
                Event.u = 0;
                Event.n.u1Valid  = 1;
                Event.n.u8Vector = X86_XCPT_NMI;
                Event.n.u3Type   = SVM_EVENT_NMI;

                hmR0SvmSetPendingEvent(pVCpu, &Event, 0 /* GCPtrFaultAddress */);
                hmR0SvmSetIretIntercept(pVmcbNstGst);
                VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INTERRUPT_NMI);
                return VINF_SUCCESS;
            }
        }

        /*
         * Check if the nested-guest can receive external interrupts (generated by
         * the guest's PIC/APIC).
         *
         * External intercepts from the physical CPU are -always- intercepted when
         * executing using hardware-assisted SVM, see HMSVM_MANDATORY_NESTED_GUEST_CTRL_INTERCEPTS.
         *
         * External interrupts that are generated for the outer guest may be intercepted
         * depending on how the nested-guest VMCB was programmed by guest software.
         *
         * Physical interrupts always take priority over virtual interrupts,
         * see AMD spec. 15.21.4 "Injecting Virtual (INTR) Interrupts".
         */
        PCSVMNESTEDVMCBCACHE pVmcbNstGstCache = &pVCpu->hm.s.svm.NstGstVmcbCache;
        Assert(pCtx->hwvirt.svm.fHMCachedVmcb);
        if (   VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC)
            && !fIntShadow
            && !pVCpu->hm.s.fSingleInstruction
            && hmR0SvmCanNstGstTakePhysIntr(pVCpu, pCtx))
        {
            if (pVmcbNstGstCache->u64InterceptCtrl & SVM_CTRL_INTERCEPT_INTR)
            {
                Log4(("Intercepting external interrupt -> #VMEXIT\n"));
                return IEMExecSvmVmexit(pVCpu, SVM_EXIT_INTR, 0, 0);
            }

            uint8_t u8Interrupt;
            int rc = PDMGetInterrupt(pVCpu, &u8Interrupt);
            if (RT_SUCCESS(rc))
            {
                Log4(("Injecting external interrupt u8Interrupt=%#x\n", u8Interrupt));

                SVMEVENT Event;
                Event.u = 0;
                Event.n.u1Valid  = 1;
                Event.n.u8Vector = u8Interrupt;
                Event.n.u3Type   = SVM_EVENT_EXTERNAL_IRQ;

                hmR0SvmSetPendingEvent(pVCpu, &Event, 0 /* GCPtrFaultAddress */);
            }
            else if (rc == VERR_APIC_INTR_MASKED_BY_TPR)
            {
                /*
                 * AMD-V has no TPR thresholding feature. We just avoid posting the interrupt.
                 * We just avoid delivering the TPR-masked interrupt here. TPR will be updated
                 * always via hmR0SvmLoadGuestState() -> hmR0SvmLoadGuestApicState().
                 */
                STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchTprMaskedIrq);
            }
            else
                STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchGuestIrq);
        }

        /*
         * Check if the nested-guest can receive virtual (injected by VMRUN) interrupts.
         * We can safely call CPUMCanSvmNstGstTakeVirtIntr here as we don't cache/modify any
         * nested-guest VMCB interrupt control fields besides V_INTR_MASKING, see hmR0SvmVmRunCacheVmcb.
         */
        if (   (pVmcbNstGstCache->u64InterceptCtrl & SVM_CTRL_INTERCEPT_VINTR)
            && VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_INTERRUPT_NESTED_GUEST)
            && CPUMCanSvmNstGstTakeVirtIntr(pCtx))
        {
            Log4(("Intercepting virtual interrupt -> #VMEXIT\n"));
            return IEMExecSvmVmexit(pVCpu, SVM_EXIT_VINTR, 0, 0);
        }
    }

    return VINF_SUCCESS;
}
#endif

/**
 * Evaluates the event to be delivered to the guest and sets it as the pending
 * event.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 */
static void hmR0SvmEvaluatePendingEvent(PVMCPU pVCpu, PCPUMCTX pCtx)
{
    Assert(!pVCpu->hm.s.Event.fPending);

#ifdef VBOX_WITH_NESTED_HWVIRT
    bool const fGif       = pCtx->hwvirt.svm.fGif;
#else
    bool const fGif       = true;
#endif
    Log4Func(("fGif=%RTbool\n", fGif));

    /*
     * If the global interrupt flag (GIF) isn't set, even NMIs and other events are blocked.
     * See AMD spec. Table 15-10. "Effect of the GIF on Interrupt Handling".
     */
    if (fGif)
    {
        bool const fIntShadow = hmR0SvmIsIntrShadowActive(pVCpu, pCtx);
        bool const fBlockInt  = !(pCtx->eflags.u32 & X86_EFL_IF);
        bool const fBlockNmi  = VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_BLOCK_NMIS);
        PSVMVMCB pVmcb        = pVCpu->hm.s.svm.pVmcb;

        Log4Func(("fGif=%RTbool fBlockInt=%RTbool fIntShadow=%RTbool APIC/PIC_Pending=%RTbool\n", fGif, fBlockInt, fIntShadow,
                  VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC)));

        /** @todo SMI. SMIs take priority over NMIs. */
        if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_INTERRUPT_NMI))   /* NMI. NMIs take priority over regular interrupts. */
        {
            if (fBlockNmi)
                hmR0SvmSetIretIntercept(pVmcb);
            else if (fIntShadow)
                hmR0SvmSetVirtIntrIntercept(pVmcb);
            else
            {
                Log4(("Pending NMI\n"));

                SVMEVENT Event;
                Event.u = 0;
                Event.n.u1Valid  = 1;
                Event.n.u8Vector = X86_XCPT_NMI;
                Event.n.u3Type   = SVM_EVENT_NMI;

                hmR0SvmSetPendingEvent(pVCpu, &Event, 0 /* GCPtrFaultAddress */);
                hmR0SvmSetIretIntercept(pVmcb);
                VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INTERRUPT_NMI);
                return;
            }
        }
        else if (   VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_INTERRUPT_APIC | VMCPU_FF_INTERRUPT_PIC)
                 && !pVCpu->hm.s.fSingleInstruction)
        {
            /*
             * Check if the guest can receive external interrupts (PIC/APIC). Once PDMGetInterrupt() returns
             * a valid interrupt we -must- deliver the interrupt. We can no longer re-request it from the APIC.
             */
            if (   !fBlockInt
                && !fIntShadow)
            {
                uint8_t u8Interrupt;
                int rc = PDMGetInterrupt(pVCpu, &u8Interrupt);
                if (RT_SUCCESS(rc))
                {
                    Log4(("Injecting external interrupt u8Interrupt=%#x\n", u8Interrupt));

                    SVMEVENT Event;
                    Event.u = 0;
                    Event.n.u1Valid  = 1;
                    Event.n.u8Vector = u8Interrupt;
                    Event.n.u3Type   = SVM_EVENT_EXTERNAL_IRQ;

                    hmR0SvmSetPendingEvent(pVCpu, &Event, 0 /* GCPtrFaultAddress */);
                }
                else if (rc == VERR_APIC_INTR_MASKED_BY_TPR)
                {
                    /*
                     * AMD-V has no TPR thresholding feature. We just avoid posting the interrupt.
                     * We just avoid delivering the TPR-masked interrupt here. TPR will be updated
                     * always via hmR0SvmLoadGuestState() -> hmR0SvmLoadGuestApicState().
                     */
                    STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchTprMaskedIrq);
                }
                else
                    STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchGuestIrq);
            }
            else
                hmR0SvmSetVirtIntrIntercept(pVmcb);
        }
    }
}


/**
 * Injects any pending events into the guest or nested-guest.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   pVmcb       Pointer to the VM control block.
 */
static void hmR0SvmInjectPendingEvent(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMVMCB pVmcb)
{
    Assert(!TRPMHasTrap(pVCpu));
    Assert(!VMMRZCallRing3IsEnabled(pVCpu));

    bool const fIntShadow = hmR0SvmIsIntrShadowActive(pVCpu, pCtx);

    /*
     * When executing the nested-guest, we avoid assertions on whether the
     * event injection is valid purely based on EFLAGS, as V_INTR_MASKING
     * affects the interpretation of interruptibility (see CPUMCanSvmNstGstTakePhysIntr).
     */
#ifndef VBOX_WITH_NESTED_HWVIRT
    bool const fBlockInt  = !(pCtx->eflags.u32 & X86_EFL_IF);
#endif

    if (pVCpu->hm.s.Event.fPending)                                /* First, inject any pending HM events. */
    {
        SVMEVENT Event;
        Event.u = pVCpu->hm.s.Event.u64IntInfo;
        Assert(Event.n.u1Valid);

#ifndef VBOX_WITH_NESTED_HWVIRT
        if (Event.n.u3Type == SVM_EVENT_EXTERNAL_IRQ)
        {
            Assert(!fBlockInt);
            Assert(!fIntShadow);
        }
        else if (Event.n.u3Type == SVM_EVENT_NMI)
            Assert(!fIntShadow);
        NOREF(fBlockInt);
#else
        Assert(!pVmcb->ctrl.EventInject.n.u1Valid);
#endif

        Log4(("Injecting pending HM event\n"));
        hmR0SvmInjectEventVmcb(pVCpu, pVmcb, pCtx, &Event);
        pVCpu->hm.s.Event.fPending = false;

#ifdef VBOX_WITH_STATISTICS
        if (Event.n.u3Type == SVM_EVENT_EXTERNAL_IRQ)
            STAM_COUNTER_INC(&pVCpu->hm.s.StatInjectInterrupt);
        else
            STAM_COUNTER_INC(&pVCpu->hm.s.StatInjectXcpt);
#endif
    }

    /*
     * Update the guest interrupt shadow in the guest or nested-guest VMCB.
     *
     * For nested-guests: We need to update it too for the scenario where IEM executes
     * the nested-guest but execution later continues here with an interrupt shadow active.
     */
    pVmcb->ctrl.u64IntShadow = !!fIntShadow;
}


/**
 * Reports world-switch error and dumps some useful debug info.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   rcVMRun         The return code from VMRUN (or
 *                          VERR_SVM_INVALID_GUEST_STATE for invalid
 *                          guest-state).
 * @param   pCtx            Pointer to the guest-CPU context.
 */
static void hmR0SvmReportWorldSwitchError(PVM pVM, PVMCPU pVCpu, int rcVMRun, PCPUMCTX pCtx)
{
    NOREF(pCtx);
    HMSVM_ASSERT_PREEMPT_SAFE();
    PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;

    if (rcVMRun == VERR_SVM_INVALID_GUEST_STATE)
    {
        hmR0DumpRegs(pVM, pVCpu, pCtx); NOREF(pVM);
#ifdef VBOX_STRICT
        Log4(("ctrl.u64VmcbCleanBits             %#RX64\n",   pVmcb->ctrl.u64VmcbCleanBits));
        Log4(("ctrl.u16InterceptRdCRx            %#x\n",      pVmcb->ctrl.u16InterceptRdCRx));
        Log4(("ctrl.u16InterceptWrCRx            %#x\n",      pVmcb->ctrl.u16InterceptWrCRx));
        Log4(("ctrl.u16InterceptRdDRx            %#x\n",      pVmcb->ctrl.u16InterceptRdDRx));
        Log4(("ctrl.u16InterceptWrDRx            %#x\n",      pVmcb->ctrl.u16InterceptWrDRx));
        Log4(("ctrl.u32InterceptXcpt             %#x\n",      pVmcb->ctrl.u32InterceptXcpt));
        Log4(("ctrl.u64InterceptCtrl             %#RX64\n",   pVmcb->ctrl.u64InterceptCtrl));
        Log4(("ctrl.u64IOPMPhysAddr              %#RX64\n",   pVmcb->ctrl.u64IOPMPhysAddr));
        Log4(("ctrl.u64MSRPMPhysAddr             %#RX64\n",   pVmcb->ctrl.u64MSRPMPhysAddr));
        Log4(("ctrl.u64TSCOffset                 %#RX64\n",   pVmcb->ctrl.u64TSCOffset));

        Log4(("ctrl.TLBCtrl.u32ASID              %#x\n",      pVmcb->ctrl.TLBCtrl.n.u32ASID));
        Log4(("ctrl.TLBCtrl.u8TLBFlush           %#x\n",      pVmcb->ctrl.TLBCtrl.n.u8TLBFlush));
        Log4(("ctrl.TLBCtrl.u24Reserved          %#x\n",      pVmcb->ctrl.TLBCtrl.n.u24Reserved));

        Log4(("ctrl.IntCtrl.u8VTPR               %#x\n",      pVmcb->ctrl.IntCtrl.n.u8VTPR));
        Log4(("ctrl.IntCtrl.u1VIrqPending        %#x\n",      pVmcb->ctrl.IntCtrl.n.u1VIrqPending));
        Log4(("ctrl.IntCtrl.u7Reserved           %#x\n",      pVmcb->ctrl.IntCtrl.n.u7Reserved));
        Log4(("ctrl.IntCtrl.u4VIntrPrio          %#x\n",      pVmcb->ctrl.IntCtrl.n.u4VIntrPrio));
        Log4(("ctrl.IntCtrl.u1IgnoreTPR          %#x\n",      pVmcb->ctrl.IntCtrl.n.u1IgnoreTPR));
        Log4(("ctrl.IntCtrl.u3Reserved           %#x\n",      pVmcb->ctrl.IntCtrl.n.u3Reserved));
        Log4(("ctrl.IntCtrl.u1VIntrMasking       %#x\n",      pVmcb->ctrl.IntCtrl.n.u1VIntrMasking));
        Log4(("ctrl.IntCtrl.u6Reserved           %#x\n",      pVmcb->ctrl.IntCtrl.n.u6Reserved));
        Log4(("ctrl.IntCtrl.u8VIntrVector        %#x\n",      pVmcb->ctrl.IntCtrl.n.u8VIntrVector));
        Log4(("ctrl.IntCtrl.u24Reserved          %#x\n",      pVmcb->ctrl.IntCtrl.n.u24Reserved));

        Log4(("ctrl.u64IntShadow                 %#RX64\n",   pVmcb->ctrl.u64IntShadow));
        Log4(("ctrl.u64ExitCode                  %#RX64\n",   pVmcb->ctrl.u64ExitCode));
        Log4(("ctrl.u64ExitInfo1                 %#RX64\n",   pVmcb->ctrl.u64ExitInfo1));
        Log4(("ctrl.u64ExitInfo2                 %#RX64\n",   pVmcb->ctrl.u64ExitInfo2));
        Log4(("ctrl.ExitIntInfo.u8Vector         %#x\n",      pVmcb->ctrl.ExitIntInfo.n.u8Vector));
        Log4(("ctrl.ExitIntInfo.u3Type           %#x\n",      pVmcb->ctrl.ExitIntInfo.n.u3Type));
        Log4(("ctrl.ExitIntInfo.u1ErrorCodeValid %#x\n",      pVmcb->ctrl.ExitIntInfo.n.u1ErrorCodeValid));
        Log4(("ctrl.ExitIntInfo.u19Reserved      %#x\n",      pVmcb->ctrl.ExitIntInfo.n.u19Reserved));
        Log4(("ctrl.ExitIntInfo.u1Valid          %#x\n",      pVmcb->ctrl.ExitIntInfo.n.u1Valid));
        Log4(("ctrl.ExitIntInfo.u32ErrorCode     %#x\n",      pVmcb->ctrl.ExitIntInfo.n.u32ErrorCode));
        Log4(("ctrl.NestedPaging                 %#RX64\n",   pVmcb->ctrl.NestedPaging.u));
        Log4(("ctrl.EventInject.u8Vector         %#x\n",      pVmcb->ctrl.EventInject.n.u8Vector));
        Log4(("ctrl.EventInject.u3Type           %#x\n",      pVmcb->ctrl.EventInject.n.u3Type));
        Log4(("ctrl.EventInject.u1ErrorCodeValid %#x\n",      pVmcb->ctrl.EventInject.n.u1ErrorCodeValid));
        Log4(("ctrl.EventInject.u19Reserved      %#x\n",      pVmcb->ctrl.EventInject.n.u19Reserved));
        Log4(("ctrl.EventInject.u1Valid          %#x\n",      pVmcb->ctrl.EventInject.n.u1Valid));
        Log4(("ctrl.EventInject.u32ErrorCode     %#x\n",      pVmcb->ctrl.EventInject.n.u32ErrorCode));

        Log4(("ctrl.u64NestedPagingCR3           %#RX64\n",   pVmcb->ctrl.u64NestedPagingCR3));
        Log4(("ctrl.u64LBRVirt                   %#RX64\n",   pVmcb->ctrl.u64LBRVirt));

        Log4(("guest.CS.u16Sel                   %RTsel\n",   pVmcb->guest.CS.u16Sel));
        Log4(("guest.CS.u16Attr                  %#x\n",      pVmcb->guest.CS.u16Attr));
        Log4(("guest.CS.u32Limit                 %#RX32\n",   pVmcb->guest.CS.u32Limit));
        Log4(("guest.CS.u64Base                  %#RX64\n",   pVmcb->guest.CS.u64Base));
        Log4(("guest.DS.u16Sel                   %#RTsel\n",  pVmcb->guest.DS.u16Sel));
        Log4(("guest.DS.u16Attr                  %#x\n",      pVmcb->guest.DS.u16Attr));
        Log4(("guest.DS.u32Limit                 %#RX32\n",   pVmcb->guest.DS.u32Limit));
        Log4(("guest.DS.u64Base                  %#RX64\n",   pVmcb->guest.DS.u64Base));
        Log4(("guest.ES.u16Sel                   %RTsel\n",   pVmcb->guest.ES.u16Sel));
        Log4(("guest.ES.u16Attr                  %#x\n",      pVmcb->guest.ES.u16Attr));
        Log4(("guest.ES.u32Limit                 %#RX32\n",   pVmcb->guest.ES.u32Limit));
        Log4(("guest.ES.u64Base                  %#RX64\n",   pVmcb->guest.ES.u64Base));
        Log4(("guest.FS.u16Sel                   %RTsel\n",   pVmcb->guest.FS.u16Sel));
        Log4(("guest.FS.u16Attr                  %#x\n",      pVmcb->guest.FS.u16Attr));
        Log4(("guest.FS.u32Limit                 %#RX32\n",   pVmcb->guest.FS.u32Limit));
        Log4(("guest.FS.u64Base                  %#RX64\n",   pVmcb->guest.FS.u64Base));
        Log4(("guest.GS.u16Sel                   %RTsel\n",   pVmcb->guest.GS.u16Sel));
        Log4(("guest.GS.u16Attr                  %#x\n",      pVmcb->guest.GS.u16Attr));
        Log4(("guest.GS.u32Limit                 %#RX32\n",   pVmcb->guest.GS.u32Limit));
        Log4(("guest.GS.u64Base                  %#RX64\n",   pVmcb->guest.GS.u64Base));

        Log4(("guest.GDTR.u32Limit               %#RX32\n",   pVmcb->guest.GDTR.u32Limit));
        Log4(("guest.GDTR.u64Base                %#RX64\n",   pVmcb->guest.GDTR.u64Base));

        Log4(("guest.LDTR.u16Sel                 %RTsel\n",   pVmcb->guest.LDTR.u16Sel));
        Log4(("guest.LDTR.u16Attr                %#x\n",      pVmcb->guest.LDTR.u16Attr));
        Log4(("guest.LDTR.u32Limit               %#RX32\n",   pVmcb->guest.LDTR.u32Limit));
        Log4(("guest.LDTR.u64Base                %#RX64\n",   pVmcb->guest.LDTR.u64Base));

        Log4(("guest.IDTR.u32Limit               %#RX32\n",   pVmcb->guest.IDTR.u32Limit));
        Log4(("guest.IDTR.u64Base                %#RX64\n",   pVmcb->guest.IDTR.u64Base));

        Log4(("guest.TR.u16Sel                   %RTsel\n",   pVmcb->guest.TR.u16Sel));
        Log4(("guest.TR.u16Attr                  %#x\n",      pVmcb->guest.TR.u16Attr));
        Log4(("guest.TR.u32Limit                 %#RX32\n",   pVmcb->guest.TR.u32Limit));
        Log4(("guest.TR.u64Base                  %#RX64\n",   pVmcb->guest.TR.u64Base));

        Log4(("guest.u8CPL                       %#x\n",      pVmcb->guest.u8CPL));
        Log4(("guest.u64CR0                      %#RX64\n",   pVmcb->guest.u64CR0));
        Log4(("guest.u64CR2                      %#RX64\n",   pVmcb->guest.u64CR2));
        Log4(("guest.u64CR3                      %#RX64\n",   pVmcb->guest.u64CR3));
        Log4(("guest.u64CR4                      %#RX64\n",   pVmcb->guest.u64CR4));
        Log4(("guest.u64DR6                      %#RX64\n",   pVmcb->guest.u64DR6));
        Log4(("guest.u64DR7                      %#RX64\n",   pVmcb->guest.u64DR7));

        Log4(("guest.u64RIP                      %#RX64\n",   pVmcb->guest.u64RIP));
        Log4(("guest.u64RSP                      %#RX64\n",   pVmcb->guest.u64RSP));
        Log4(("guest.u64RAX                      %#RX64\n",   pVmcb->guest.u64RAX));
        Log4(("guest.u64RFlags                   %#RX64\n",   pVmcb->guest.u64RFlags));

        Log4(("guest.u64SysEnterCS               %#RX64\n",   pVmcb->guest.u64SysEnterCS));
        Log4(("guest.u64SysEnterEIP              %#RX64\n",   pVmcb->guest.u64SysEnterEIP));
        Log4(("guest.u64SysEnterESP              %#RX64\n",   pVmcb->guest.u64SysEnterESP));

        Log4(("guest.u64EFER                     %#RX64\n",   pVmcb->guest.u64EFER));
        Log4(("guest.u64STAR                     %#RX64\n",   pVmcb->guest.u64STAR));
        Log4(("guest.u64LSTAR                    %#RX64\n",   pVmcb->guest.u64LSTAR));
        Log4(("guest.u64CSTAR                    %#RX64\n",   pVmcb->guest.u64CSTAR));
        Log4(("guest.u64SFMASK                   %#RX64\n",   pVmcb->guest.u64SFMASK));
        Log4(("guest.u64KernelGSBase             %#RX64\n",   pVmcb->guest.u64KernelGSBase));
        Log4(("guest.u64GPAT                     %#RX64\n",   pVmcb->guest.u64GPAT));
        Log4(("guest.u64DBGCTL                   %#RX64\n",   pVmcb->guest.u64DBGCTL));
        Log4(("guest.u64BR_FROM                  %#RX64\n",   pVmcb->guest.u64BR_FROM));
        Log4(("guest.u64BR_TO                    %#RX64\n",   pVmcb->guest.u64BR_TO));
        Log4(("guest.u64LASTEXCPFROM             %#RX64\n",   pVmcb->guest.u64LASTEXCPFROM));
        Log4(("guest.u64LASTEXCPTO               %#RX64\n",   pVmcb->guest.u64LASTEXCPTO));
#endif  /* VBOX_STRICT */
    }
    else
        Log4(("hmR0SvmReportWorldSwitchError: rcVMRun=%d\n", rcVMRun));

    NOREF(pVmcb);
}


/**
 * Check per-VM and per-VCPU force flag actions that require us to go back to
 * ring-3 for one reason or another.
 *
 * @returns VBox status code (information status code included).
 * @retval VINF_SUCCESS if we don't have any actions that require going back to
 *         ring-3.
 * @retval VINF_PGM_SYNC_CR3 if we have pending PGM CR3 sync.
 * @retval VINF_EM_PENDING_REQUEST if we have pending requests (like hardware
 *         interrupts)
 * @retval VINF_PGM_POOL_FLUSH_PENDING if PGM is doing a pool flush and requires
 *         all EMTs to be in ring-3.
 * @retval VINF_EM_RAW_TO_R3 if there is pending DMA requests.
 * @retval VINF_EM_NO_MEMORY PGM is out of memory, we need to return
 *         to the EM loop.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 */
static int hmR0SvmCheckForceFlags(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    Assert(VMMRZCallRing3IsEnabled(pVCpu));

    /* On AMD-V we don't need to update CR3, PAE PDPES lazily. See hmR0SvmSaveGuestState(). */
    Assert(!VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HM_UPDATE_CR3));
    Assert(!VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HM_UPDATE_PAE_PDPES));

    /* Update pending interrupts into the APIC's IRR. */
    if (VMCPU_FF_TEST_AND_CLEAR(pVCpu, VMCPU_FF_UPDATE_APIC))
        APICUpdatePendingInterrupts(pVCpu);

    if (   VM_FF_IS_PENDING(pVM, !pVCpu->hm.s.fSingleInstruction
                            ? VM_FF_HP_R0_PRE_HM_MASK : VM_FF_HP_R0_PRE_HM_STEP_MASK)
        || VMCPU_FF_IS_PENDING(pVCpu, !pVCpu->hm.s.fSingleInstruction
                               ? VMCPU_FF_HP_R0_PRE_HM_MASK : VMCPU_FF_HP_R0_PRE_HM_STEP_MASK) )
    {
        /* Pending PGM C3 sync. */
        if (VMCPU_FF_IS_PENDING(pVCpu,VMCPU_FF_PGM_SYNC_CR3 | VMCPU_FF_PGM_SYNC_CR3_NON_GLOBAL))
        {
            int rc = PGMSyncCR3(pVCpu, pCtx->cr0, pCtx->cr3, pCtx->cr4, VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_PGM_SYNC_CR3));
            if (rc != VINF_SUCCESS)
            {
                Log4(("hmR0SvmCheckForceFlags: PGMSyncCR3 forcing us back to ring-3. rc=%d\n", rc));
                return rc;
            }
        }

        /* Pending HM-to-R3 operations (critsects, timers, EMT rendezvous etc.) */
        /* -XXX- what was that about single stepping?  */
        if (   VM_FF_IS_PENDING(pVM, VM_FF_HM_TO_R3_MASK)
            || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HM_TO_R3_MASK))
        {
            STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchHmToR3FF);
            int rc = RT_UNLIKELY(VM_FF_IS_PENDING(pVM, VM_FF_PGM_NO_MEMORY)) ? VINF_EM_NO_MEMORY : VINF_EM_RAW_TO_R3;
            Log4(("hmR0SvmCheckForceFlags: HM_TO_R3 forcing us back to ring-3. rc=%d\n", rc));
            return rc;
        }

        /* Pending VM request packets, such as hardware interrupts. */
        if (   VM_FF_IS_PENDING(pVM, VM_FF_REQUEST)
            || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_REQUEST))
        {
            Log4(("hmR0SvmCheckForceFlags: Pending VM request forcing us back to ring-3\n"));
            return VINF_EM_PENDING_REQUEST;
        }

        /* Pending PGM pool flushes. */
        if (VM_FF_IS_PENDING(pVM, VM_FF_PGM_POOL_FLUSH_PENDING))
        {
            Log4(("hmR0SvmCheckForceFlags: PGM pool flush pending forcing us back to ring-3\n"));
            return VINF_PGM_POOL_FLUSH_PENDING;
        }

        /* Pending DMA requests. */
        if (VM_FF_IS_PENDING(pVM, VM_FF_PDM_DMA))
        {
            Log4(("hmR0SvmCheckForceFlags: Pending DMA request forcing us back to ring-3\n"));
            return VINF_EM_RAW_TO_R3;
        }
    }

    return VINF_SUCCESS;
}


#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Does the preparations before executing nested-guest code in AMD-V.
 *
 * @returns VBox status code (informational status codes included).
 * @retval VINF_SUCCESS if we can proceed with running the guest.
 * @retval VINF_* scheduling changes, we have to go back to ring-3.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 *
 * @remarks Same caveats regarding longjumps as hmR0SvmPreRunGuest applies.
 * @sa      hmR0SvmPreRunGuest.
 */
static int hmR0SvmPreRunGuestNested(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_ASSERT_PREEMPT_SAFE();

    if (CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
    {
#ifdef VBOX_WITH_NESTED_HWVIRT_ONLY_IN_IEM
        Log2(("hmR0SvmPreRunGuest: Rescheduling to IEM due to nested-hwvirt or forced IEM exec -> VINF_EM_RESCHEDULE_REM\n"));
        return VINF_EM_RESCHEDULE_REM;
#endif
    }
    else
        return VINF_SVM_VMEXIT;

    /* Check force flag actions that might require us to go back to ring-3. */
    int rc = hmR0SvmCheckForceFlags(pVM, pVCpu, pCtx);
    if (rc != VINF_SUCCESS)
        return rc;

    hmR0SvmVmRunSetupVmcb(pVCpu, pCtx);

    if (TRPMHasTrap(pVCpu))
        hmR0SvmTrpmTrapToPendingEvent(pVCpu);
    else if (!pVCpu->hm.s.Event.fPending)
    {
        VBOXSTRICTRC rcStrict = hmR0SvmEvaluatePendingEventNested(pVCpu, pCtx);
        if (rcStrict != VINF_SUCCESS)
            return VBOXSTRICTRC_VAL(rcStrict);
    }

    /*
     * On the oldest AMD-V systems, we may not get enough information to reinject an NMI.
     * Just do it in software, see @bugref{8411}.
     * NB: If we could continue a task switch exit we wouldn't need to do this.
     */
    if (RT_UNLIKELY(   !pVM->hm.s.svm.u32Features
                    &&  pVCpu->hm.s.Event.fPending
                    &&  SVM_EVENT_GET_TYPE(pVCpu->hm.s.Event.u64IntInfo) == SVM_EVENT_NMI))
    {
        return VINF_EM_RAW_INJECT_TRPM_EVENT;
    }

    /*
     * Load the nested-guest state.
     */
    rc = hmR0SvmLoadGuestStateNested(pVCpu, pCtx);
    AssertRCReturn(rc, rc);
    /** @todo Get new STAM counter for this? */
    STAM_COUNTER_INC(&pVCpu->hm.s.StatLoadFull);

    Assert(pCtx->hwvirt.svm.fHMCachedVmcb);

    /*
     * No longjmps to ring-3 from this point on!!!
     * Asserts() will still longjmp to ring-3 (but won't return), which is intentional, better than a kernel panic.
     * This also disables flushing of the R0-logger instance (if any).
     */
    VMMRZCallRing3Disable(pVCpu);

    /*
     * We disable interrupts so that we don't miss any interrupts that would flag preemption (IPI/timers etc.)
     * when thread-context hooks aren't used and we've been running with preemption disabled for a while.
     *
     * We need to check for force-flags that could've possible been altered since we last checked them (e.g.
     * by PDMGetInterrupt() leaving the PDM critical section, see @bugref{6398}).
     *
     * We also check a couple of other force-flags as a last opportunity to get the EMT back to ring-3 before
     * executing guest code.
     */
    pSvmTransient->fEFlags = ASMIntDisableFlags();
    if (   VM_FF_IS_PENDING(pVM, VM_FF_EMT_RENDEZVOUS | VM_FF_TM_VIRTUAL_SYNC)
        || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HM_TO_R3_MASK))
    {
        ASMSetFlags(pSvmTransient->fEFlags);
        VMMRZCallRing3Enable(pVCpu);
        STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchHmToR3FF);
        return VINF_EM_RAW_TO_R3;
    }
    if (RTThreadPreemptIsPending(NIL_RTTHREAD))
    {
        ASMSetFlags(pSvmTransient->fEFlags);
        VMMRZCallRing3Enable(pVCpu);
        STAM_COUNTER_INC(&pVCpu->hm.s.StatPendingHostIrq);
        return VINF_EM_RAW_INTERRUPT;
    }

    /*
     * If we are injecting an NMI, we must set VMCPU_FF_BLOCK_NMIS only when we are going to execute
     * guest code for certain (no exits to ring-3). Otherwise, we could re-read the flag on re-entry into
     * AMD-V and conclude that NMI inhibition is active when we have not even delivered the NMI.
     *
     * With VT-x, this is handled by the Guest interruptibility information VMCS field which will set the
     * VMCS field after actually delivering the NMI which we read on VM-exit to determine the state.
     */
    if (pVCpu->hm.s.Event.fPending)
    {
        SVMEVENT Event;
        Event.u = pVCpu->hm.s.Event.u64IntInfo;
        if (    Event.n.u1Valid
            &&  Event.n.u3Type == SVM_EVENT_NMI
            &&  Event.n.u8Vector == X86_XCPT_NMI
            && !VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_BLOCK_NMIS))
        {
            VMCPU_FF_SET(pVCpu, VMCPU_FF_BLOCK_NMIS);
        }
    }

    return VINF_SUCCESS;
}
#endif


/**
 * Does the preparations before executing guest code in AMD-V.
 *
 * This may cause longjmps to ring-3 and may even result in rescheduling to the
 * recompiler. We must be cautious what we do here regarding committing
 * guest-state information into the VMCB assuming we assuredly execute the guest
 * in AMD-V. If we fall back to the recompiler after updating the VMCB and
 * clearing the common-state (TRPM/forceflags), we must undo those changes so
 * that the recompiler can (and should) use them when it resumes guest
 * execution. Otherwise such operations must be done when we can no longer
 * exit to ring-3.
 *
 * @returns VBox status code (informational status codes included).
 * @retval VINF_SUCCESS if we can proceed with running the guest.
 * @retval VINF_* scheduling changes, we have to go back to ring-3.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 */
static int hmR0SvmPreRunGuest(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_ASSERT_PREEMPT_SAFE();
    Assert(!CPUMIsGuestInSvmNestedHwVirtMode(pCtx));

#ifdef VBOX_WITH_NESTED_HWVIRT_ONLY_IN_IEM
    /* IEM only for executing nested guest, we shouldn't get here. */
    /** @todo Make this into an assertion since HMR3CanExecuteGuest already checks
     *        for it? */
    if (CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
    {
        Log2(("hmR0SvmPreRunGuest: Rescheduling to IEM due to nested-hwvirt or forced IEM exec -> VINF_EM_RESCHEDULE_REM\n"));
        return VINF_EM_RESCHEDULE_REM;
    }
#endif

    /* Check force flag actions that might require us to go back to ring-3. */
    int rc = hmR0SvmCheckForceFlags(pVM, pVCpu, pCtx);
    if (rc != VINF_SUCCESS)
        return rc;

    if (TRPMHasTrap(pVCpu))
        hmR0SvmTrpmTrapToPendingEvent(pVCpu);
    else if (!pVCpu->hm.s.Event.fPending)
        hmR0SvmEvaluatePendingEvent(pVCpu, pCtx);

    /*
     * On the oldest AMD-V systems, we may not get enough information to reinject an NMI.
     * Just do it in software, see @bugref{8411}.
     * NB: If we could continue a task switch exit we wouldn't need to do this.
     */
    if (RT_UNLIKELY(pVCpu->hm.s.Event.fPending && (((pVCpu->hm.s.Event.u64IntInfo >> 8) & 7) == SVM_EVENT_NMI)))
        if (RT_UNLIKELY(!pVM->hm.s.svm.u32Features))
            return VINF_EM_RAW_INJECT_TRPM_EVENT;

#ifdef HMSVM_SYNC_FULL_GUEST_STATE
    HMCPU_CF_SET(pVCpu, HM_CHANGED_ALL_GUEST);
#endif

    /* Load the guest bits that are not shared with the host in any way since we can longjmp or get preempted. */
    rc = hmR0SvmLoadGuestState(pVM, pVCpu, pCtx);
    AssertRCReturn(rc, rc);
    STAM_COUNTER_INC(&pVCpu->hm.s.StatLoadFull);

    /*
     * If we're not intercepting TPR changes in the guest, save the guest TPR before the world-switch
     * so we can update it on the way back if the guest changed the TPR.
     */
    if (pVCpu->hm.s.svm.fSyncVTpr)
    {
        if (pVM->hm.s.fTPRPatchingActive)
            pSvmTransient->u8GuestTpr = pCtx->msrLSTAR;
        else
        {
            PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
            pSvmTransient->u8GuestTpr = pVmcb->ctrl.IntCtrl.n.u8VTPR;
        }
    }

    /*
     * No longjmps to ring-3 from this point on!!!
     * Asserts() will still longjmp to ring-3 (but won't return), which is intentional, better than a kernel panic.
     * This also disables flushing of the R0-logger instance (if any).
     */
    VMMRZCallRing3Disable(pVCpu);

    /*
     * We disable interrupts so that we don't miss any interrupts that would flag preemption (IPI/timers etc.)
     * when thread-context hooks aren't used and we've been running with preemption disabled for a while.
     *
     * We need to check for force-flags that could've possible been altered since we last checked them (e.g.
     * by PDMGetInterrupt() leaving the PDM critical section, see @bugref{6398}).
     *
     * We also check a couple of other force-flags as a last opportunity to get the EMT back to ring-3 before
     * executing guest code.
     */
    pSvmTransient->fEFlags = ASMIntDisableFlags();
    if (   VM_FF_IS_PENDING(pVM, VM_FF_EMT_RENDEZVOUS | VM_FF_TM_VIRTUAL_SYNC)
        || VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_HM_TO_R3_MASK))
    {
        ASMSetFlags(pSvmTransient->fEFlags);
        VMMRZCallRing3Enable(pVCpu);
        STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchHmToR3FF);
        return VINF_EM_RAW_TO_R3;
    }
    if (RTThreadPreemptIsPending(NIL_RTTHREAD))
    {
        ASMSetFlags(pSvmTransient->fEFlags);
        VMMRZCallRing3Enable(pVCpu);
        STAM_COUNTER_INC(&pVCpu->hm.s.StatPendingHostIrq);
        return VINF_EM_RAW_INTERRUPT;
    }

    /*
     * If we are injecting an NMI, we must set VMCPU_FF_BLOCK_NMIS only when we are going to execute
     * guest code for certain (no exits to ring-3). Otherwise, we could re-read the flag on re-entry into
     * AMD-V and conclude that NMI inhibition is active when we have not even delivered the NMI.
     *
     * With VT-x, this is handled by the Guest interruptibility information VMCS field which will set the
     * VMCS field after actually delivering the NMI which we read on VM-exit to determine the state.
     */
    if (pVCpu->hm.s.Event.fPending)
    {
        SVMEVENT Event;
        Event.u = pVCpu->hm.s.Event.u64IntInfo;
        if (    Event.n.u1Valid
            &&  Event.n.u3Type == SVM_EVENT_NMI
            &&  Event.n.u8Vector == X86_XCPT_NMI
            && !VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_BLOCK_NMIS))
        {
            VMCPU_FF_SET(pVCpu, VMCPU_FF_BLOCK_NMIS);
        }
    }

    return VINF_SUCCESS;
}


#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Prepares to run nested-guest code in AMD-V and we've committed to doing so. This
 * means there is no backing out to ring-3 or anywhere else at this point.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 *
 * @remarks Called with preemption disabled.
 * @remarks No-long-jump zone!!!
 */
static void hmR0SvmPreRunGuestCommittedNested(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    Assert(!VMMRZCallRing3IsEnabled(pVCpu));
    Assert(VMMR0IsLogFlushDisabled(pVCpu));
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));

    VMCPU_ASSERT_STATE(pVCpu, VMCPUSTATE_STARTED_HM);
    VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC);            /* Indicate the start of guest execution. */

    PSVMVMCB pVmcbNstGst = pCtx->hwvirt.svm.CTX_SUFF(pVmcb);
    hmR0SvmInjectPendingEvent(pVCpu, pCtx, pVmcbNstGst);

    if (!CPUMIsGuestFPUStateActive(pVCpu))
    {
        CPUMR0LoadGuestFPU(pVM, pVCpu); /* (Ignore rc, no need to set HM_CHANGED_HOST_CONTEXT for SVM.) */
        HMCPU_CF_SET(pVCpu, HM_CHANGED_GUEST_CR0);
    }

    /* Load the state shared between host and nested-guest (FPU, debug). */
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_HOST_GUEST_SHARED_STATE))
        hmR0SvmLoadSharedState(pVCpu, pVmcbNstGst, pCtx);

    HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_HOST_CONTEXT);             /* Preemption might set this, nothing to do on AMD-V. */
    AssertMsg(!HMCPU_CF_VALUE(pVCpu), ("fContextUseFlags=%#RX32\n", HMCPU_CF_VALUE(pVCpu)));

    /* Setup TSC offsetting. */
    RTCPUID idCurrentCpu = hmR0GetCurrentCpu()->idCpu;
    if (   pSvmTransient->fUpdateTscOffsetting
        || idCurrentCpu != pVCpu->hm.s.idLastCpu)
    {
        hmR0SvmUpdateTscOffsetting(pVM, pVCpu, pVmcbNstGst);
        pSvmTransient->fUpdateTscOffsetting = false;
    }

    /* If we've migrating CPUs, mark the VMCB Clean bits as dirty. */
    if (idCurrentCpu != pVCpu->hm.s.idLastCpu)
        pVmcbNstGst->ctrl.u64VmcbCleanBits = 0;

    /* Store status of the shared guest-host state at the time of VMRUN. */
#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
    if (CPUMIsGuestInLongModeEx(pCtx))
    {
        pSvmTransient->fWasGuestDebugStateActive = CPUMIsGuestDebugStateActivePending(pVCpu);
        pSvmTransient->fWasHyperDebugStateActive = CPUMIsHyperDebugStateActivePending(pVCpu);
    }
    else
#endif
    {
        pSvmTransient->fWasGuestDebugStateActive = CPUMIsGuestDebugStateActive(pVCpu);
        pSvmTransient->fWasHyperDebugStateActive = CPUMIsHyperDebugStateActive(pVCpu);
    }

    /* The TLB flushing would've already been setup by the nested-hypervisor. */
    ASMAtomicWriteBool(&pVCpu->hm.s.fCheckedTLBFlush, true);    /* Used for TLB flushing, set this across the world switch. */
    hmR0SvmFlushTaggedTlb(pVCpu, pCtx, pVmcbNstGst);
    Assert(hmR0GetCurrentCpu()->idCpu == pVCpu->hm.s.idLastCpu);

    STAM_PROFILE_ADV_STOP_START(&pVCpu->hm.s.StatEntry, &pVCpu->hm.s.StatInGC, x);

    TMNotifyStartOfExecution(pVCpu);                            /* Finally, notify TM to resume its clocks as we're about
                                                                   to start executing. */

    /*
     * Save the current Host TSC_AUX and write the guest TSC_AUX to the host, so that
     * RDTSCPs (that don't cause exits) reads the guest MSR. See @bugref{3324}.
     *
     * This should be done -after- any RDTSCPs for obtaining the host timestamp (TM, STAM etc).
     */
    uint8_t *pbMsrBitmap = (uint8_t *)pCtx->hwvirt.svm.CTX_SUFF(pvMsrBitmap);
    if (    (pVM->hm.s.cpuid.u32AMDFeatureEDX & X86_CPUID_EXT_FEATURE_EDX_RDTSCP)
        && !(pVmcbNstGst->ctrl.u64InterceptCtrl & SVM_CTRL_INTERCEPT_RDTSCP))
    {
        hmR0SvmSetMsrPermission(pVmcbNstGst, pbMsrBitmap, MSR_K8_TSC_AUX, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        pVCpu->hm.s.u64HostTscAux = ASMRdMsr(MSR_K8_TSC_AUX);
        uint64_t u64GuestTscAux = CPUMR0GetGuestTscAux(pVCpu);
        if (u64GuestTscAux != pVCpu->hm.s.u64HostTscAux)
            ASMWrMsr(MSR_K8_TSC_AUX, u64GuestTscAux);
        pSvmTransient->fRestoreTscAuxMsr = true;
    }
    else
    {
        hmR0SvmSetMsrPermission(pVmcbNstGst, pbMsrBitmap, MSR_K8_TSC_AUX, SVMMSREXIT_INTERCEPT_READ, SVMMSREXIT_INTERCEPT_WRITE);
        pSvmTransient->fRestoreTscAuxMsr = false;
    }

    /*
     * If VMCB Clean bits isn't supported by the CPU or exposed by the guest,
     * mark all state-bits as dirty indicating to the CPU to re-load from VMCB.
     */
    if (   !(pVM->hm.s.svm.u32Features & X86_CPUID_SVM_FEATURE_EDX_VMCB_CLEAN)
        || !(pVM->cpum.ro.GuestFeatures.fSvmVmcbClean))
        pVmcbNstGst->ctrl.u64VmcbCleanBits = 0;
}
#endif


/**
 * Prepares to run guest code in AMD-V and we've committed to doing so. This
 * means there is no backing out to ring-3 or anywhere else at this
 * point.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 *
 * @remarks Called with preemption disabled.
 * @remarks No-long-jump zone!!!
 */
static void hmR0SvmPreRunGuestCommitted(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    Assert(!VMMRZCallRing3IsEnabled(pVCpu));
    Assert(VMMR0IsLogFlushDisabled(pVCpu));
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));

    VMCPU_ASSERT_STATE(pVCpu, VMCPUSTATE_STARTED_HM);
    VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC);            /* Indicate the start of guest execution. */

    PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
    hmR0SvmInjectPendingEvent(pVCpu, pCtx, pVmcb);

    if (!CPUMIsGuestFPUStateActive(pVCpu))
    {
        CPUMR0LoadGuestFPU(pVM, pVCpu); /* (Ignore rc, no need to set HM_CHANGED_HOST_CONTEXT for SVM.) */
        HMCPU_CF_SET(pVCpu, HM_CHANGED_GUEST_CR0);
    }

    /* Load the state shared between host and guest (FPU, debug). */
    if (HMCPU_CF_IS_PENDING(pVCpu, HM_CHANGED_HOST_GUEST_SHARED_STATE))
        hmR0SvmLoadSharedState(pVCpu, pVmcb, pCtx);

    HMCPU_CF_CLEAR(pVCpu, HM_CHANGED_HOST_CONTEXT);             /* Preemption might set this, nothing to do on AMD-V. */
    AssertMsg(!HMCPU_CF_VALUE(pVCpu), ("fContextUseFlags=%#RX32\n", HMCPU_CF_VALUE(pVCpu)));

    /* Setup TSC offsetting. */
    RTCPUID idCurrentCpu = hmR0GetCurrentCpu()->idCpu;
    if (   pSvmTransient->fUpdateTscOffsetting
        || idCurrentCpu != pVCpu->hm.s.idLastCpu)
    {
        hmR0SvmUpdateTscOffsetting(pVM, pVCpu, pVmcb);
        pSvmTransient->fUpdateTscOffsetting = false;
    }

    /* If we've migrating CPUs, mark the VMCB Clean bits as dirty. */
    if (idCurrentCpu != pVCpu->hm.s.idLastCpu)
        pVmcb->ctrl.u64VmcbCleanBits = 0;

    /* Store status of the shared guest-host state at the time of VMRUN. */
#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
    if (CPUMIsGuestInLongModeEx(pCtx))
    {
        pSvmTransient->fWasGuestDebugStateActive = CPUMIsGuestDebugStateActivePending(pVCpu);
        pSvmTransient->fWasHyperDebugStateActive = CPUMIsHyperDebugStateActivePending(pVCpu);
    }
    else
#endif
    {
        pSvmTransient->fWasGuestDebugStateActive = CPUMIsGuestDebugStateActive(pVCpu);
        pSvmTransient->fWasHyperDebugStateActive = CPUMIsHyperDebugStateActive(pVCpu);
    }

    /* Flush the appropriate tagged-TLB entries. */
    ASMAtomicWriteBool(&pVCpu->hm.s.fCheckedTLBFlush, true);    /* Used for TLB flushing, set this across the world switch. */
    hmR0SvmFlushTaggedTlb(pVCpu, pCtx, pVmcb);
    Assert(hmR0GetCurrentCpu()->idCpu == pVCpu->hm.s.idLastCpu);

    STAM_PROFILE_ADV_STOP_START(&pVCpu->hm.s.StatEntry, &pVCpu->hm.s.StatInGC, x);

    TMNotifyStartOfExecution(pVCpu);                            /* Finally, notify TM to resume its clocks as we're about
                                                                   to start executing. */

    /*
     * Save the current Host TSC_AUX and write the guest TSC_AUX to the host, so that
     * RDTSCPs (that don't cause exits) reads the guest MSR. See @bugref{3324}.
     *
     * This should be done -after- any RDTSCPs for obtaining the host timestamp (TM, STAM etc).
     */
    uint8_t *pbMsrBitmap = (uint8_t *)pVCpu->hm.s.svm.pvMsrBitmap;
    if (    (pVM->hm.s.cpuid.u32AMDFeatureEDX & X86_CPUID_EXT_FEATURE_EDX_RDTSCP)
        && !(pVmcb->ctrl.u64InterceptCtrl & SVM_CTRL_INTERCEPT_RDTSCP))
    {
        hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_K8_TSC_AUX, SVMMSREXIT_PASSTHRU_READ, SVMMSREXIT_PASSTHRU_WRITE);
        pVCpu->hm.s.u64HostTscAux = ASMRdMsr(MSR_K8_TSC_AUX);
        uint64_t u64GuestTscAux = CPUMR0GetGuestTscAux(pVCpu);
        if (u64GuestTscAux != pVCpu->hm.s.u64HostTscAux)
            ASMWrMsr(MSR_K8_TSC_AUX, u64GuestTscAux);
        pSvmTransient->fRestoreTscAuxMsr = true;
    }
    else
    {
        hmR0SvmSetMsrPermission(pVmcb, pbMsrBitmap, MSR_K8_TSC_AUX, SVMMSREXIT_INTERCEPT_READ, SVMMSREXIT_INTERCEPT_WRITE);
        pSvmTransient->fRestoreTscAuxMsr = false;
    }

    /* If VMCB Clean bits isn't supported by the CPU, simply mark all state-bits as dirty, indicating (re)load-from-VMCB. */
    if (!(pVM->hm.s.svm.u32Features & X86_CPUID_SVM_FEATURE_EDX_VMCB_CLEAN))
        pVmcb->ctrl.u64VmcbCleanBits = 0;
}


/**
 * Wrapper for running the guest code in AMD-V.
 *
 * @returns VBox strict status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
DECLINLINE(int) hmR0SvmRunGuest(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /*
     * 64-bit Windows uses XMM registers in the kernel as the Microsoft compiler expresses floating-point operations
     * using SSE instructions. Some XMM registers (XMM6-XMM15) are callee-saved and thus the need for this XMM wrapper.
     * Refer MSDN docs. "Configuring Programs for 64-bit / x64 Software Conventions / Register Usage" for details.
     */
#ifdef VBOX_WITH_KERNEL_USING_XMM
    return hmR0SVMRunWrapXMM(pVCpu->hm.s.svm.HCPhysVmcbHost, pVCpu->hm.s.svm.HCPhysVmcb, pCtx, pVM, pVCpu,
                             pVCpu->hm.s.svm.pfnVMRun);
#else
    return pVCpu->hm.s.svm.pfnVMRun(pVCpu->hm.s.svm.HCPhysVmcbHost, pVCpu->hm.s.svm.HCPhysVmcb, pCtx, pVM, pVCpu);
#endif
}


#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Wrapper for running the nested-guest code in AMD-V.
 *
 * @returns VBox strict status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 *
 * @remarks No-long-jump zone!!!
 */
DECLINLINE(int) hmR0SvmRunGuestNested(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /*
     * 64-bit Windows uses XMM registers in the kernel as the Microsoft compiler expresses floating-point operations
     * using SSE instructions. Some XMM registers (XMM6-XMM15) are callee-saved and thus the need for this XMM wrapper.
     * Refer MSDN docs. "Configuring Programs for 64-bit / x64 Software Conventions / Register Usage" for details.
     */
#ifdef VBOX_WITH_KERNEL_USING_XMM
    return hmR0SVMRunWrapXMM(pVCpu->hm.s.svm.HCPhysVmcbHost, pCtx->hwvirt.svm.HCPhysVmcb, pCtx, pVM, pVCpu,
                             pVCpu->hm.s.svm.pfnVMRun);
#else
    return pVCpu->hm.s.svm.pfnVMRun(pVCpu->hm.s.svm.HCPhysVmcbHost, pCtx->hwvirt.svm.HCPhysVmcb, pCtx, pVM, pVCpu);
#endif
}


/**
 * Performs some essential restoration of state after running nested-guest code in
 * AMD-V.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pMixedCtx       Pointer to the nested-guest-CPU context. The data maybe
 *                          out-of-sync. Make sure to update the required fields
 *                          before using them.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 * @param   rcVMRun         Return code of VMRUN.
 *
 * @remarks Called with interrupts disabled.
 * @remarks No-long-jump zone!!! This function will however re-enable longjmps
 *          unconditionally when it is safe to do so.
 */
static void hmR0SvmPostRunGuestNested(PVM pVM, PVMCPU pVCpu, PCPUMCTX pMixedCtx, PSVMTRANSIENT pSvmTransient, int rcVMRun)
{
    RT_NOREF(pVM);
    Assert(!VMMRZCallRing3IsEnabled(pVCpu));

    ASMAtomicWriteBool(&pVCpu->hm.s.fCheckedTLBFlush, false);   /* See HMInvalidatePageOnAllVCpus(): used for TLB flushing. */
    ASMAtomicIncU32(&pVCpu->hm.s.cWorldSwitchExits);            /* Initialized in vmR3CreateUVM(): used for EMT poking. */

    /* TSC read must be done early for maximum accuracy. */
    PSVMVMCB     pVmcbNstGst     = pMixedCtx->hwvirt.svm.CTX_SUFF(pVmcb);
    PSVMVMCBCTRL pVmcbNstGstCtrl = &pVmcbNstGst->ctrl;
    if (!(pVmcbNstGstCtrl->u64InterceptCtrl & SVM_CTRL_INTERCEPT_RDTSC))
        TMCpuTickSetLastSeen(pVCpu, ASMReadTSC() + pVmcbNstGstCtrl->u64TSCOffset);

    if (pSvmTransient->fRestoreTscAuxMsr)
    {
        uint64_t u64GuestTscAuxMsr = ASMRdMsr(MSR_K8_TSC_AUX);
        CPUMR0SetGuestTscAux(pVCpu, u64GuestTscAuxMsr);
        if (u64GuestTscAuxMsr != pVCpu->hm.s.u64HostTscAux)
            ASMWrMsr(MSR_K8_TSC_AUX, pVCpu->hm.s.u64HostTscAux);
    }

    STAM_PROFILE_ADV_STOP_START(&pVCpu->hm.s.StatInGC, &pVCpu->hm.s.StatExit1, x);
    TMNotifyEndOfExecution(pVCpu);                              /* Notify TM that the guest is no longer running. */
    VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED_HM);

    Assert(!(ASMGetFlags() & X86_EFL_IF));
    ASMSetFlags(pSvmTransient->fEFlags);                        /* Enable interrupts. */
    VMMRZCallRing3Enable(pVCpu);                                /* It is now safe to do longjmps to ring-3!!! */

    /* Mark the VMCB-state cache as unmodified by VMM. */
    pVmcbNstGstCtrl->u64VmcbCleanBits = HMSVM_VMCB_CLEAN_ALL;

    /* If VMRUN failed, we can bail out early. This does -not- cover SVM_EXIT_INVALID. */
    if (RT_UNLIKELY(rcVMRun != VINF_SUCCESS))
    {
        Log4(("VMRUN failure: rcVMRun=%Rrc\n", rcVMRun));
        return;
    }

    pSvmTransient->u64ExitCode  = pVmcbNstGstCtrl->u64ExitCode; /* Save the #VMEXIT reason. */
    HMCPU_EXIT_HISTORY_ADD(pVCpu, pVmcbNstGstCtrl->u64ExitCode);/* Update the #VMEXIT history array. */
    pSvmTransient->fVectoringDoublePF = false;                  /* Vectoring double page-fault needs to be determined later. */
    pSvmTransient->fVectoringPF       = false;                  /* Vectoring page-fault needs to be determined later. */

    Assert(!pVCpu->hm.s.svm.fSyncVTpr);
    hmR0SvmSaveGuestState(pVCpu, pMixedCtx, pVmcbNstGst);       /* Save the nested-guest state from the VMCB to the
                                                                   guest-CPU context. */

    HMSvmNstGstVmExitNotify(pVCpu, pMixedCtx);                  /* Restore modified VMCB fields for now, see @bugref{7243#c52} .*/
}
#endif

/**
 * Performs some essential restoration of state after running guest code in
 * AMD-V.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pMixedCtx       Pointer to the guest-CPU context. The data maybe
 *                          out-of-sync. Make sure to update the required fields
 *                          before using them.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 * @param   rcVMRun         Return code of VMRUN.
 *
 * @remarks Called with interrupts disabled.
 * @remarks No-long-jump zone!!! This function will however re-enable longjmps
 *          unconditionally when it is safe to do so.
 */
static void hmR0SvmPostRunGuest(PVM pVM, PVMCPU pVCpu, PCPUMCTX pMixedCtx, PSVMTRANSIENT pSvmTransient, int rcVMRun)
{
    Assert(!VMMRZCallRing3IsEnabled(pVCpu));

    ASMAtomicWriteBool(&pVCpu->hm.s.fCheckedTLBFlush, false);   /* See HMInvalidatePageOnAllVCpus(): used for TLB flushing. */
    ASMAtomicIncU32(&pVCpu->hm.s.cWorldSwitchExits);            /* Initialized in vmR3CreateUVM(): used for EMT poking. */

    PSVMVMCB pVmcb =pVCpu->hm.s.svm.pVmcb;
    pVmcb->ctrl.u64VmcbCleanBits = HMSVM_VMCB_CLEAN_ALL;        /* Mark the VMCB-state cache as unmodified by VMM. */

    /* TSC read must be done early for maximum accuracy. */
    if (!(pVmcb->ctrl.u64InterceptCtrl & SVM_CTRL_INTERCEPT_RDTSC))
        TMCpuTickSetLastSeen(pVCpu, ASMReadTSC() + pVmcb->ctrl.u64TSCOffset);

    if (pSvmTransient->fRestoreTscAuxMsr)
    {
        uint64_t u64GuestTscAuxMsr = ASMRdMsr(MSR_K8_TSC_AUX);
        CPUMR0SetGuestTscAux(pVCpu, u64GuestTscAuxMsr);
        if (u64GuestTscAuxMsr != pVCpu->hm.s.u64HostTscAux)
            ASMWrMsr(MSR_K8_TSC_AUX, pVCpu->hm.s.u64HostTscAux);
    }

    STAM_PROFILE_ADV_STOP_START(&pVCpu->hm.s.StatInGC, &pVCpu->hm.s.StatExit1, x);
    TMNotifyEndOfExecution(pVCpu);                              /* Notify TM that the guest is no longer running. */
    VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED_HM);

    Assert(!(ASMGetFlags() & X86_EFL_IF));
    ASMSetFlags(pSvmTransient->fEFlags);                        /* Enable interrupts. */
    VMMRZCallRing3Enable(pVCpu);                                /* It is now safe to do longjmps to ring-3!!! */

    /* If VMRUN failed, we can bail out early. This does -not- cover SVM_EXIT_INVALID. */
    if (RT_UNLIKELY(rcVMRun != VINF_SUCCESS))
    {
        Log4(("VMRUN failure: rcVMRun=%Rrc\n", rcVMRun));
        return;
    }

    pSvmTransient->u64ExitCode  = pVmcb->ctrl.u64ExitCode;      /* Save the #VMEXIT reason. */
    HMCPU_EXIT_HISTORY_ADD(pVCpu, pVmcb->ctrl.u64ExitCode);     /* Update the #VMEXIT history array. */
    pSvmTransient->fVectoringDoublePF = false;                  /* Vectoring double page-fault needs to be determined later. */
    pSvmTransient->fVectoringPF = false;                        /* Vectoring page-fault needs to be determined later. */

    hmR0SvmSaveGuestState(pVCpu, pMixedCtx, pVmcb);             /* Save the guest state from the VMCB to the guest-CPU context. */

    if (RT_LIKELY(pSvmTransient->u64ExitCode != SVM_EXIT_INVALID))
    {
        if (pVCpu->hm.s.svm.fSyncVTpr)
        {
            /* TPR patching (for 32-bit guests) uses LSTAR MSR for holding the TPR value, otherwise uses the VTPR. */
            if (   pVM->hm.s.fTPRPatchingActive
                && (pMixedCtx->msrLSTAR & 0xff) != pSvmTransient->u8GuestTpr)
            {
                int rc = APICSetTpr(pVCpu, pMixedCtx->msrLSTAR & 0xff);
                AssertRC(rc);
                HMCPU_CF_SET(pVCpu, HM_CHANGED_SVM_GUEST_APIC_STATE);
            }
            else if (pSvmTransient->u8GuestTpr != pVmcb->ctrl.IntCtrl.n.u8VTPR)
            {
                int rc = APICSetTpr(pVCpu, pVmcb->ctrl.IntCtrl.n.u8VTPR << 4);
                AssertRC(rc);
                HMCPU_CF_SET(pVCpu, HM_CHANGED_SVM_GUEST_APIC_STATE);
            }
        }
    }
}


/**
 * Runs the guest code using AMD-V.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   pcLoops     Pointer to the number of executed loops.
 */
static int hmR0SvmRunGuestCodeNormal(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, uint32_t *pcLoops)
{
    uint32_t const cMaxResumeLoops = pVM->hm.s.cMaxResumeLoops;
    Assert(pcLoops);
    Assert(*pcLoops <= cMaxResumeLoops);

    SVMTRANSIENT SvmTransient;
    SvmTransient.fUpdateTscOffsetting = true;

    int rc = VERR_INTERNAL_ERROR_5;
    for (;;)
    {
        Assert(!HMR0SuspendPending());
        HMSVM_ASSERT_CPU_SAFE();

        /* Preparatory work for running guest code, this may force us to return
           to ring-3.  This bugger disables interrupts on VINF_SUCCESS! */
        STAM_PROFILE_ADV_START(&pVCpu->hm.s.StatEntry, x);
        rc = hmR0SvmPreRunGuest(pVM, pVCpu, pCtx, &SvmTransient);
        if (rc != VINF_SUCCESS)
            break;

        /*
         * No longjmps to ring-3 from this point on!!!
         * Asserts() will still longjmp to ring-3 (but won't return), which is intentional, better than a kernel panic.
         * This also disables flushing of the R0-logger instance (if any).
         */
        hmR0SvmPreRunGuestCommitted(pVM, pVCpu, pCtx, &SvmTransient);
        rc = hmR0SvmRunGuest(pVM, pVCpu, pCtx);

        /* Restore any residual host-state and save any bits shared between host
           and guest into the guest-CPU state.  Re-enables interrupts! */
        hmR0SvmPostRunGuest(pVM, pVCpu, pCtx, &SvmTransient, rc);

        if (RT_UNLIKELY(   rc != VINF_SUCCESS                               /* Check for VMRUN errors. */
                        || SvmTransient.u64ExitCode == SVM_EXIT_INVALID))   /* Check for invalid guest-state errors. */
        {
            if (rc == VINF_SUCCESS)
                rc = VERR_SVM_INVALID_GUEST_STATE;
            STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatExit1, x);
            hmR0SvmReportWorldSwitchError(pVM, pVCpu, rc, pCtx);
            break;
        }

        /* Handle the #VMEXIT. */
        HMSVM_EXITCODE_STAM_COUNTER_INC(SvmTransient.u64ExitCode);
        STAM_PROFILE_ADV_STOP_START(&pVCpu->hm.s.StatExit1, &pVCpu->hm.s.StatExit2, x);
        VBOXVMM_R0_HMSVM_VMEXIT(pVCpu, pCtx, SvmTransient.u64ExitCode, pVCpu->hm.s.svm.pVmcb);
        rc = hmR0SvmHandleExit(pVCpu, pCtx, &SvmTransient);
        STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatExit2, x);
        if (rc != VINF_SUCCESS)
            break;
        if (++(*pcLoops) >= cMaxResumeLoops)
        {
            STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchMaxResumeLoops);
            rc = VINF_EM_RAW_INTERRUPT;
            break;
        }
    }

    STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatEntry, x);
    return rc;
}


/**
 * Runs the guest code using AMD-V in single step mode.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   pcLoops     Pointer to the number of executed loops.
 */
static int hmR0SvmRunGuestCodeStep(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, uint32_t *pcLoops)
{
    uint32_t const cMaxResumeLoops = pVM->hm.s.cMaxResumeLoops;
    Assert(pcLoops);
    Assert(*pcLoops <= cMaxResumeLoops);

    SVMTRANSIENT SvmTransient;
    SvmTransient.fUpdateTscOffsetting = true;

    uint16_t uCsStart  = pCtx->cs.Sel;
    uint64_t uRipStart = pCtx->rip;

    int rc = VERR_INTERNAL_ERROR_5;
    for (;;)
    {
        Assert(!HMR0SuspendPending());
        AssertMsg(pVCpu->hm.s.idEnteredCpu == RTMpCpuId(),
                  ("Illegal migration! Entered on CPU %u Current %u cLoops=%u\n", (unsigned)pVCpu->hm.s.idEnteredCpu,
                  (unsigned)RTMpCpuId(), *pcLoops));

        /* Preparatory work for running guest code, this may force us to return
           to ring-3.  This bugger disables interrupts on VINF_SUCCESS! */
        STAM_PROFILE_ADV_START(&pVCpu->hm.s.StatEntry, x);
        rc = hmR0SvmPreRunGuest(pVM, pVCpu, pCtx, &SvmTransient);
        if (rc != VINF_SUCCESS)
            break;

        /*
         * No longjmps to ring-3 from this point on!!!
         * Asserts() will still longjmp to ring-3 (but won't return), which is intentional, better than a kernel panic.
         * This also disables flushing of the R0-logger instance (if any).
         */
        VMMRZCallRing3Disable(pVCpu);
        VMMRZCallRing3RemoveNotification(pVCpu);
        hmR0SvmPreRunGuestCommitted(pVM, pVCpu, pCtx, &SvmTransient);

        rc = hmR0SvmRunGuest(pVM, pVCpu, pCtx);

        /*
         * Restore any residual host-state and save any bits shared between host and guest into the guest-CPU state.
         * This will also re-enable longjmps to ring-3 when it has reached a safe point!!!
         */
        hmR0SvmPostRunGuest(pVM, pVCpu, pCtx, &SvmTransient, rc);
        if (RT_UNLIKELY(   rc != VINF_SUCCESS                               /* Check for VMRUN errors. */
                        || SvmTransient.u64ExitCode == SVM_EXIT_INVALID))   /* Check for invalid guest-state errors. */
        {
            if (rc == VINF_SUCCESS)
                rc = VERR_SVM_INVALID_GUEST_STATE;
            STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatExit1, x);
            hmR0SvmReportWorldSwitchError(pVM, pVCpu, rc, pCtx);
            return rc;
        }

        /* Handle the #VMEXIT. */
        HMSVM_EXITCODE_STAM_COUNTER_INC(SvmTransient.u64ExitCode);
        STAM_PROFILE_ADV_STOP_START(&pVCpu->hm.s.StatExit1, &pVCpu->hm.s.StatExit2, x);
        VBOXVMM_R0_HMSVM_VMEXIT(pVCpu, pCtx, SvmTransient.u64ExitCode, pVCpu->hm.s.svm.pVmcb);
        rc = hmR0SvmHandleExit(pVCpu, pCtx, &SvmTransient);
        STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatExit2, x);
        if (rc != VINF_SUCCESS)
            break;
        if (++(*pcLoops) >= cMaxResumeLoops)
        {
            STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchMaxResumeLoops);
            rc = VINF_EM_RAW_INTERRUPT;
            break;
        }

        /*
         * Did the RIP change, if so, consider it a single step.
         * Otherwise, make sure one of the TFs gets set.
         */
        if (   pCtx->rip    != uRipStart
            || pCtx->cs.Sel != uCsStart)
        {
            rc = VINF_EM_DBG_STEPPED;
            break;
        }
        pVCpu->hm.s.fContextUseFlags |= HM_CHANGED_GUEST_DEBUG;
    }

    /*
     * Clear the X86_EFL_TF if necessary.
     */
    if (pVCpu->hm.s.fClearTrapFlag)
    {
        pVCpu->hm.s.fClearTrapFlag = false;
        pCtx->eflags.Bits.u1TF = 0;
    }

    STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatEntry, x);
    return rc;
}

#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Runs the nested-guest code using AMD-V.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   pcLoops     Pointer to the number of executed loops. If we're switching
 *                      from the guest-code execution loop to this nested-guest
 *                      execution loop pass the remainder value, else pass 0.
 */
static int hmR0SvmRunGuestCodeNested(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx, uint32_t *pcLoops)
{
    Assert(CPUMIsGuestInSvmNestedHwVirtMode(pCtx));
    Assert(pcLoops);
    Assert(*pcLoops <= pVM->hm.s.cMaxResumeLoops);

    SVMTRANSIENT SvmTransient;
    SvmTransient.fUpdateTscOffsetting = true;

    int rc = VERR_INTERNAL_ERROR_4;
    for (;;)
    {
        Assert(!HMR0SuspendPending());
        HMSVM_ASSERT_CPU_SAFE();

        /* Preparatory work for running nested-guest code, this may force us to return
           to ring-3.  This bugger disables interrupts on VINF_SUCCESS! */
        STAM_PROFILE_ADV_START(&pVCpu->hm.s.StatEntry, x);
        rc = hmR0SvmPreRunGuestNested(pVM, pVCpu, pCtx, &SvmTransient);
        if (rc != VINF_SUCCESS)
            break;

        /*
         * No longjmps to ring-3 from this point on!!!
         * Asserts() will still longjmp to ring-3 (but won't return), which is intentional, better than a kernel panic.
         * This also disables flushing of the R0-logger instance (if any).
         */
        hmR0SvmPreRunGuestCommittedNested(pVM, pVCpu, pCtx, &SvmTransient);

        rc = hmR0SvmRunGuestNested(pVM, pVCpu, pCtx);

        /* Restore any residual host-state and save any bits shared between host
           and guest into the guest-CPU state.  Re-enables interrupts! */
        hmR0SvmPostRunGuestNested(pVM, pVCpu, pCtx, &SvmTransient, rc);

        /** @todo This needs some work... we probably should cause a \#VMEXIT on
         *        SVM_EXIT_INVALID and handle rc != VINF_SUCCESS differently. */
        if (RT_UNLIKELY(   rc != VINF_SUCCESS                               /* Check for VMRUN errors. */
                        || SvmTransient.u64ExitCode == SVM_EXIT_INVALID))   /* Check for invalid guest-state errors. */
        {
            if (rc == VINF_SUCCESS)
                rc = VERR_SVM_INVALID_GUEST_STATE;
            STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatExit1, x);
            hmR0SvmReportWorldSwitchError(pVM, pVCpu, rc, pCtx);
            break;
        }

        /* Handle the #VMEXIT. */
        HMSVM_EXITCODE_STAM_COUNTER_INC(SvmTransient.u64ExitCode);
        STAM_PROFILE_ADV_STOP_START(&pVCpu->hm.s.StatExit1, &pVCpu->hm.s.StatExit2, x);
        VBOXVMM_R0_HMSVM_VMEXIT(pVCpu, pCtx, SvmTransient.u64ExitCode, pVCpu->hm.s.svm.pVmcb);
        rc = hmR0SvmHandleExitNested(pVCpu, pCtx, &SvmTransient);
        STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatExit2, x);
        if (rc != VINF_SUCCESS)
            break;
        if (++(*pcLoops) >= pVM->hm.s.cMaxResumeLoops)
        {
            STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchMaxResumeLoops);
            rc = VINF_EM_RAW_INTERRUPT;
            break;
        }

        /** @todo handle single-stepping   */
    }

    STAM_PROFILE_ADV_STOP(&pVCpu->hm.s.StatEntry, x);
    return rc;
}
#endif


/**
 * Runs the guest code using AMD-V.
 *
 * @returns Strict VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 */
VMMR0DECL(VBOXSTRICTRC) SVMR0RunGuestCode(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    Assert(VMMRZCallRing3IsEnabled(pVCpu));
    HMSVM_ASSERT_PREEMPT_SAFE();
    VMMRZCallRing3SetNotification(pVCpu, hmR0SvmCallRing3Callback, pCtx);

    uint32_t cLoops = 0;
    int      rc;
#ifdef VBOX_WITH_NESTED_HWVIRT
    if (!CPUMIsGuestInSvmNestedHwVirtMode(pCtx))
#endif
    {
        if (!pVCpu->hm.s.fSingleInstruction)
            rc = hmR0SvmRunGuestCodeNormal(pVM, pVCpu, pCtx, &cLoops);
        else
            rc = hmR0SvmRunGuestCodeStep(pVM, pVCpu, pCtx, &cLoops);
    }
#ifdef VBOX_WITH_NESTED_HWVIRT
    else
    {
        rc = VINF_SVM_VMRUN;
    }

    /* Re-check the nested-guest condition here as we may be transitioning from the normal
       execution loop into the nested-guest, hence this is not placed in the 'else' part above. */
    if (rc == VINF_SVM_VMRUN)
    {
        rc = hmR0SvmRunGuestCodeNested(pVM, pVCpu, pCtx, &cLoops);
        if (rc == VINF_SVM_VMEXIT)
            rc = VINF_SUCCESS;
    }
#endif

    /* Fixup error codes. */
    if (rc == VERR_EM_INTERPRETER)
        rc = VINF_EM_RAW_EMULATE_INSTR;
    else if (rc == VINF_EM_RESET)
        rc = VINF_EM_TRIPLE_FAULT;

    /* Prepare to return to ring-3. This will remove longjmp notifications. */
    rc = hmR0SvmExitToRing3(pVM, pVCpu, pCtx, rc);
    Assert(!VMMRZCallRing3IsNotificationSet(pVCpu));
    return rc;
}


#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Determines whether an IOIO intercept is active for the nested-guest or not.
 *
 * @param   pvIoBitmap      Pointer to the nested-guest IO bitmap.
 * @param   pIoExitInfo     Pointer to the SVMIOIOEXITINFO.
 */
static bool hmR0SvmIsIoInterceptActive(void *pvIoBitmap, PSVMIOIOEXITINFO pIoExitInfo)
{
    const uint16_t    u16Port       = pIoExitInfo->n.u16Port;
    const SVMIOIOTYPE enmIoType     = (SVMIOIOTYPE)pIoExitInfo->n.u1Type;
    const uint8_t     cbReg         = (pIoExitInfo->u >> SVM_IOIO_OP_SIZE_SHIFT) & 7;
    const uint8_t     cAddrSizeBits = ((pIoExitInfo->u >> SVM_IOIO_ADDR_SIZE_SHIFT) & 7) << 4;
    const uint8_t     iEffSeg       = pIoExitInfo->n.u3SEG;
    const bool        fRep          = pIoExitInfo->n.u1REP;
    const bool        fStrIo        = pIoExitInfo->n.u1STR;

    return HMSvmIsIOInterceptActive(pvIoBitmap, u16Port, enmIoType, cbReg, cAddrSizeBits, iEffSeg, fRep, fStrIo,
                                    NULL /* pIoExitInfo */);
}


/**
 * Handles a nested-guest \#VMEXIT (for all EXITCODE values except
 * SVM_EXIT_INVALID).
 *
 * @returns VBox status code (informational status codes included).
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 */
static int hmR0SvmHandleExitNested(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    Assert(pSvmTransient->u64ExitCode != SVM_EXIT_INVALID);
    Assert(pSvmTransient->u64ExitCode <= SVM_EXIT_MAX);

#define HM_SVM_VMEXIT_NESTED(a_pVCpu, a_uExitCode, a_uExitInfo1, a_uExitInfo2) \
            VBOXSTRICTRC_TODO(IEMExecSvmVmexit(a_pVCpu, a_uExitCode, a_uExitInfo1, a_uExitInfo2))
#define HM_SVM_IS_CTRL_INTERCEPT_SET(a_pCtx, a_Intercept)       CPUMIsGuestSvmCtrlInterceptSet(a_pCtx, (a_Intercept))
#define HM_SVM_IS_XCPT_INTERCEPT_SET(a_pCtx, a_Xcpt)            CPUMIsGuestSvmXcptInterceptSet(a_pCtx, (a_Xcpt))
#define HM_SVM_IS_READ_CR_INTERCEPT_SET(a_pCtx, a_uCr)          CPUMIsGuestSvmReadCRxInterceptSet(a_pCtx, (a_uCr))
#define HM_SVM_IS_READ_DR_INTERCEPT_SET(a_pCtx, a_uDr)          CPUMIsGuestSvmReadDRxInterceptSet(a_pCtx, (a_uDr))
#define HM_SVM_IS_WRITE_CR_INTERCEPT_SET(a_pCtx, a_uCr)         CPUMIsGuestSvmWriteCRxInterceptSet(a_pCtx, (a_uCr))
#define HM_SVM_IS_WRITE_DR_INTERCEPT_SET(a_pCtx, a_uDr)         CPUMIsGuestSvmWriteDRxInterceptSet(a_pCtx, (a_uDr))

    /*
     * For all the #VMEXITs here we primarily figure out if the #VMEXIT is expected
     * by the nested-guest. If it isn't, it should be handled by the (outer) guest.
     */
    PSVMVMCB            pVmcbNstGst      = pCtx->hwvirt.svm.CTX_SUFF(pVmcb);
    PSVMVMCBCTRL        pVmcbNstGstCtrl  = &pVmcbNstGst->ctrl;
    uint64_t const      uExitCode        = pVmcbNstGstCtrl->u64ExitCode;
    uint64_t const      uExitInfo1       = pVmcbNstGstCtrl->u64ExitInfo1;
    uint64_t const      uExitInfo2       = pVmcbNstGstCtrl->u64ExitInfo2;

    Assert(uExitCode == pVmcbNstGstCtrl->u64ExitCode);
    switch (uExitCode)
    {
        case SVM_EXIT_CPUID:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_CPUID))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitCpuid(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_RDTSC:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_RDTSC))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitRdtsc(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_RDTSCP:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_RDTSCP))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitRdtscp(pVCpu, pCtx, pSvmTransient);
        }


        case SVM_EXIT_MONITOR:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_MONITOR))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitMonitor(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_MWAIT:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_MWAIT))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitMwait(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_HLT:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_HLT))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitHlt(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_MSR:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_MSR_PROT))
            {
                uint32_t const idMsr = pCtx->ecx;
                uint16_t offMsrpm;
                uint32_t uMsrpmBit;
                int rc = HMSvmGetMsrpmOffsetAndBit(idMsr, &offMsrpm, &uMsrpmBit);
                if (RT_SUCCESS(rc))
                {
                    void const *pvMsrBitmap    = pCtx->hwvirt.svm.CTX_SUFF(pvMsrBitmap);
                    bool const fInterceptRead  = ASMBitTest(pvMsrBitmap, (offMsrpm << 3) + uMsrpmBit);
                    bool const fInterceptWrite = ASMBitTest(pvMsrBitmap, (offMsrpm << 3) + uMsrpmBit + 1);

                    if (   (fInterceptWrite && pVmcbNstGstCtrl->u64ExitInfo1 == SVM_EXIT1_MSR_WRITE)
                        || (fInterceptRead  && pVmcbNstGstCtrl->u64ExitInfo1 == SVM_EXIT1_MSR_READ))
                    {
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    }
                }
                else
                {
                    /*
                     * MSRs not covered by the MSRPM automatically cause an #VMEXIT.
                     * See AMD-V spec. "15.11 MSR Intercepts".
                     */
                    Assert(rc == VERR_OUT_OF_RANGE);
                    return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                }
            }
            return hmR0SvmExitMsr(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_IOIO:
        {
            /*
             * Figure out if the IO port access is intercepted by the nested-guest.
             */
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_IOIO_PROT))
            {
                void *pvIoBitmap = pCtx->hwvirt.svm.CTX_SUFF(pvIoBitmap);
                SVMIOIOEXITINFO IoExitInfo;
                IoExitInfo.u = pVmcbNstGst->ctrl.u64ExitInfo1;
                bool const fIntercept = hmR0SvmIsIoInterceptActive(pvIoBitmap, &IoExitInfo);
                if (fIntercept)
                    return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            }
            return hmR0SvmExitIOInstr(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_EXCEPTION_14:  /* X86_XCPT_PF */
        {
            PVM pVM = pVCpu->CTX_SUFF(pVM);
            if (pVM->hm.s.fNestedPaging)
            {
                uint32_t const u32ErrCode    = pVmcbNstGstCtrl->u64ExitInfo1;
                uint64_t const uFaultAddress = pVmcbNstGstCtrl->u64ExitInfo2;

                /* If the nested-guest is intercepting #PFs, cause a #PF #VMEXIT. */
                if (HM_SVM_IS_XCPT_INTERCEPT_SET(pCtx, X86_XCPT_PF))
                    return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, u32ErrCode, uFaultAddress);

                /* If the nested-guest is not intercepting #PFs, forward the #PF to the nested-guest. */
                hmR0SvmSetPendingXcptPF(pVCpu, pCtx, u32ErrCode, uFaultAddress);
                return VINF_SUCCESS;
            }
            return hmR0SvmExitXcptPFNested(pVCpu, pCtx,pSvmTransient);
        }

        case SVM_EXIT_EXCEPTION_6:   /* X86_XCPT_UD */
        {
            if (HM_SVM_IS_XCPT_INTERCEPT_SET(pCtx, X86_XCPT_UD))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            hmR0SvmSetPendingXcptUD(pVCpu);
            return VINF_SUCCESS;
        }

        case SVM_EXIT_EXCEPTION_16:  /* X86_XCPT_MF */
        {
            if (HM_SVM_IS_XCPT_INTERCEPT_SET(pCtx, X86_XCPT_MF))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            hmR0SvmSetPendingXcptMF(pVCpu);
            return VINF_SUCCESS;
        }

        case SVM_EXIT_EXCEPTION_1:   /* X86_XCPT_DB */
        {
            if (HM_SVM_IS_XCPT_INTERCEPT_SET(pCtx, X86_XCPT_DB))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmNestedExitXcptDB(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_EXCEPTION_17:  /* X86_XCPT_AC */
        {
            if (HM_SVM_IS_XCPT_INTERCEPT_SET(pCtx, X86_XCPT_AC))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitXcptAC(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_EXCEPTION_3:   /* X86_XCPT_BP */
        {
            if (HM_SVM_IS_XCPT_INTERCEPT_SET(pCtx, X86_XCPT_BP))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmNestedExitXcptBP(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_READ_CR0:
        case SVM_EXIT_READ_CR3:
        case SVM_EXIT_READ_CR4:
        {
            if (HM_SVM_IS_READ_CR_INTERCEPT_SET(pCtx, (1U << (uint16_t)(pSvmTransient->u64ExitCode - SVM_EXIT_READ_CR0))))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitReadCRx(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_WRITE_CR0:
        case SVM_EXIT_WRITE_CR3:
        case SVM_EXIT_WRITE_CR4:
        case SVM_EXIT_WRITE_CR8:   /** @todo Shouldn't writes to CR8 go to V_TPR instead since we run with V_INTR_MASKING set?? */
        {
            Log4(("hmR0SvmHandleExitNested: Write CRx: u16InterceptWrCRx=%#x u64ExitCode=%#RX64 %#x\n",
                  pVmcbNstGstCtrl->u16InterceptWrCRx, pSvmTransient->u64ExitCode,
                  (1U << (uint16_t)(pSvmTransient->u64ExitCode - SVM_EXIT_WRITE_CR0))));

            if (HM_SVM_IS_WRITE_CR_INTERCEPT_SET(pCtx, (1U << (uint16_t)(pSvmTransient->u64ExitCode - SVM_EXIT_WRITE_CR0))))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitWriteCRx(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_PAUSE:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_PAUSE))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitPause(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_VINTR:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_VINTR))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitUnexpected(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_INTR:
        {
            /* We shouldn't direct physical interrupts to the nested-guest. */
            return hmR0SvmExitIntr(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_FERR_FREEZE:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_FERR_FREEZE))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitIntr(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_NMI:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_NMI))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitIntr(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_INVLPG:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_INVLPG))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitInvlpg(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_WBINVD:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_WBINVD))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitWbinvd(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_INVD:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_INVD))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitInvd(pVCpu, pCtx, pSvmTransient);
        }

        case SVM_EXIT_RDPMC:
        {
            if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_RDPMC))
                return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
            return hmR0SvmExitRdpmc(pVCpu, pCtx, pSvmTransient);
        }

        default:
        {
            switch (pSvmTransient->u64ExitCode)
            {
                case SVM_EXIT_READ_DR0:     case SVM_EXIT_READ_DR1:     case SVM_EXIT_READ_DR2:     case SVM_EXIT_READ_DR3:
                case SVM_EXIT_READ_DR6:     case SVM_EXIT_READ_DR7:     case SVM_EXIT_READ_DR8:     case SVM_EXIT_READ_DR9:
                case SVM_EXIT_READ_DR10:    case SVM_EXIT_READ_DR11:    case SVM_EXIT_READ_DR12:    case SVM_EXIT_READ_DR13:
                case SVM_EXIT_READ_DR14:    case SVM_EXIT_READ_DR15:
                {
                    if (HM_SVM_IS_READ_DR_INTERCEPT_SET(pCtx, (1U << (uint16_t)(pSvmTransient->u64ExitCode - SVM_EXIT_READ_DR0))))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmExitReadDRx(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_WRITE_DR0:    case SVM_EXIT_WRITE_DR1:    case SVM_EXIT_WRITE_DR2:    case SVM_EXIT_WRITE_DR3:
                case SVM_EXIT_WRITE_DR6:    case SVM_EXIT_WRITE_DR7:    case SVM_EXIT_WRITE_DR8:    case SVM_EXIT_WRITE_DR9:
                case SVM_EXIT_WRITE_DR10:   case SVM_EXIT_WRITE_DR11:   case SVM_EXIT_WRITE_DR12:   case SVM_EXIT_WRITE_DR13:
                case SVM_EXIT_WRITE_DR14:   case SVM_EXIT_WRITE_DR15:
                {
                    if (HM_SVM_IS_WRITE_DR_INTERCEPT_SET(pCtx, (1U << (uint16_t)(pSvmTransient->u64ExitCode - SVM_EXIT_WRITE_DR0))))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmExitWriteDRx(pVCpu, pCtx, pSvmTransient);
                }

                /* The exceptions not handled here are already handled individually above (as they occur more frequently). */
                case SVM_EXIT_EXCEPTION_0:     /*case SVM_EXIT_EXCEPTION_1:*/   case SVM_EXIT_EXCEPTION_2:
                /*case SVM_EXIT_EXCEPTION_3:*/   case SVM_EXIT_EXCEPTION_4:     case SVM_EXIT_EXCEPTION_5:
                /*case SVM_EXIT_EXCEPTION_6:*/ /*case SVM_EXIT_EXCEPTION_7:*/   case SVM_EXIT_EXCEPTION_8:
                case SVM_EXIT_EXCEPTION_9:       case SVM_EXIT_EXCEPTION_10:    case SVM_EXIT_EXCEPTION_11:
                case SVM_EXIT_EXCEPTION_12:      case SVM_EXIT_EXCEPTION_13:  /*case SVM_EXIT_EXCEPTION_14:*/
                case SVM_EXIT_EXCEPTION_15:      case SVM_EXIT_EXCEPTION_16:  /*case SVM_EXIT_EXCEPTION_17:*/
                case SVM_EXIT_EXCEPTION_18:      case SVM_EXIT_EXCEPTION_19:    case SVM_EXIT_EXCEPTION_20:
                case SVM_EXIT_EXCEPTION_21:      case SVM_EXIT_EXCEPTION_22:    case SVM_EXIT_EXCEPTION_23:
                case SVM_EXIT_EXCEPTION_24:      case SVM_EXIT_EXCEPTION_25:    case SVM_EXIT_EXCEPTION_26:
                case SVM_EXIT_EXCEPTION_27:      case SVM_EXIT_EXCEPTION_28:    case SVM_EXIT_EXCEPTION_29:
                case SVM_EXIT_EXCEPTION_30:      case SVM_EXIT_EXCEPTION_31:
                {
                    if (HM_SVM_IS_XCPT_INTERCEPT_SET(pCtx, (uint32_t)(pSvmTransient->u64ExitCode - SVM_EXIT_EXCEPTION_0)))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    /** @todo Write hmR0SvmExitXcptGeneric! */
                    return VERR_NOT_IMPLEMENTED;
                }

                case SVM_EXIT_XSETBV:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_XSETBV))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmExitXsetbv(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_TASK_SWITCH:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_TASK_SWITCH))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmExitTaskSwitch(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_IRET:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_IRET))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmNestedExitIret(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_SHUTDOWN:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_SHUTDOWN))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmExitShutdown(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_SMI:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_SMI))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmExitUnexpected(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_INIT:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_INIT))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmExitUnexpected(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_VMMCALL:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_VMMCALL))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmExitVmmCall(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_CLGI:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_CLGI))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                     return hmR0SvmExitClgi(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_STGI:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_STGI))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                     return hmR0SvmExitStgi(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_VMLOAD:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_VMLOAD))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmExitVmload(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_VMSAVE:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_VMSAVE))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmExitVmsave(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_INVLPGA:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_INVLPGA))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmExitInvlpga(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_VMRUN:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_VMRUN))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    return hmR0SvmExitVmrun(pVCpu, pCtx, pSvmTransient);
                }

                case SVM_EXIT_RSM:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_RSM))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    hmR0SvmSetPendingXcptUD(pVCpu);
                    return VINF_SUCCESS;
                }

                case SVM_EXIT_SKINIT:
                {
                    if (HM_SVM_IS_CTRL_INTERCEPT_SET(pCtx, SVM_CTRL_INTERCEPT_SKINIT))
                        return HM_SVM_VMEXIT_NESTED(pVCpu, uExitCode, uExitInfo1, uExitInfo2);
                    hmR0SvmSetPendingXcptUD(pVCpu);
                    return VINF_SUCCESS;
                }

                case SVM_EXIT_NPF:
                {
                    /* We don't yet support nested-paging for nested-guests, so this should never really happen. */
                    Assert(!pVmcbNstGstCtrl->NestedPaging.n.u1NestedPaging);
                    return hmR0SvmExitUnexpected(pVCpu, pCtx, pSvmTransient);
                }

                default:
                {
                    AssertMsgFailed(("hmR0SvmHandleExitNested: Unknown exit code %#x\n", pSvmTransient->u64ExitCode));
                    pVCpu->hm.s.u32HMError = pSvmTransient->u64ExitCode;
                    return VERR_SVM_UNKNOWN_EXIT;
                }
            }
        }
    }
    /* not reached */

#undef HM_SVM_VMEXIT_NESTED
#undef HM_SVM_IS_CTRL_INTERCEPT_SET
#undef HM_SVM_IS_XCPT_INTERCEPT_SET
#undef HM_SVM_IS_READ_CR_INTERCEPT_SET
#undef HM_SVM_IS_READ_DR_INTERCEPT_SET
#undef HM_SVM_IS_WRITE_CR_INTERCEPT_SET
#undef HM_SVM_IS_WRITE_DR_INTERCEPT_SET
}
#endif


/**
 * Handles a guest \#VMEXIT (for all EXITCODE values except SVM_EXIT_INVALID).
 *
 * @returns VBox status code (informational status codes included).
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 */
static int hmR0SvmHandleExit(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    Assert(pSvmTransient->u64ExitCode != SVM_EXIT_INVALID);
    Assert(pSvmTransient->u64ExitCode <= SVM_EXIT_MAX);

    /*
     * The ordering of the case labels is based on most-frequently-occurring #VMEXITs for most guests under
     * normal workloads (for some definition of "normal").
     */
    uint32_t u32ExitCode = pSvmTransient->u64ExitCode;
    switch (pSvmTransient->u64ExitCode)
    {
        case SVM_EXIT_NPF:
            return hmR0SvmExitNestedPF(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_IOIO:
            return hmR0SvmExitIOInstr(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_RDTSC:
            return hmR0SvmExitRdtsc(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_RDTSCP:
            return hmR0SvmExitRdtscp(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_CPUID:
            return hmR0SvmExitCpuid(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_EXCEPTION_14:  /* X86_XCPT_PF */
            return hmR0SvmExitXcptPF(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_EXCEPTION_6:   /* X86_XCPT_UD */
            return hmR0SvmExitXcptUD(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_EXCEPTION_16:  /* X86_XCPT_MF */
            return hmR0SvmExitXcptMF(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_EXCEPTION_1:   /* X86_XCPT_DB */
            return hmR0SvmExitXcptDB(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_EXCEPTION_17:  /* X86_XCPT_AC */
            return hmR0SvmExitXcptAC(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_EXCEPTION_3:   /* X86_XCPT_BP */
            return hmR0SvmExitXcptBP(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_MONITOR:
            return hmR0SvmExitMonitor(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_MWAIT:
            return hmR0SvmExitMwait(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_HLT:
            return hmR0SvmExitHlt(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_READ_CR0:
        case SVM_EXIT_READ_CR3:
        case SVM_EXIT_READ_CR4:
            return hmR0SvmExitReadCRx(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_WRITE_CR0:
        case SVM_EXIT_WRITE_CR3:
        case SVM_EXIT_WRITE_CR4:
        case SVM_EXIT_WRITE_CR8:
            return hmR0SvmExitWriteCRx(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_PAUSE:
            return hmR0SvmExitPause(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_VMMCALL:
            return hmR0SvmExitVmmCall(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_VINTR:
            return hmR0SvmExitVIntr(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_INTR:
        case SVM_EXIT_FERR_FREEZE:
        case SVM_EXIT_NMI:
            return hmR0SvmExitIntr(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_MSR:
            return hmR0SvmExitMsr(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_INVLPG:
            return hmR0SvmExitInvlpg(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_WBINVD:
            return hmR0SvmExitWbinvd(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_INVD:
            return hmR0SvmExitInvd(pVCpu, pCtx, pSvmTransient);

        case SVM_EXIT_RDPMC:
            return hmR0SvmExitRdpmc(pVCpu, pCtx, pSvmTransient);

        default:
        {
            switch (pSvmTransient->u64ExitCode)
            {
                case SVM_EXIT_READ_DR0:     case SVM_EXIT_READ_DR1:     case SVM_EXIT_READ_DR2:     case SVM_EXIT_READ_DR3:
                case SVM_EXIT_READ_DR6:     case SVM_EXIT_READ_DR7:     case SVM_EXIT_READ_DR8:     case SVM_EXIT_READ_DR9:
                case SVM_EXIT_READ_DR10:    case SVM_EXIT_READ_DR11:    case SVM_EXIT_READ_DR12:    case SVM_EXIT_READ_DR13:
                case SVM_EXIT_READ_DR14:    case SVM_EXIT_READ_DR15:
                    return hmR0SvmExitReadDRx(pVCpu, pCtx, pSvmTransient);

                case SVM_EXIT_WRITE_DR0:    case SVM_EXIT_WRITE_DR1:    case SVM_EXIT_WRITE_DR2:    case SVM_EXIT_WRITE_DR3:
                case SVM_EXIT_WRITE_DR6:    case SVM_EXIT_WRITE_DR7:    case SVM_EXIT_WRITE_DR8:    case SVM_EXIT_WRITE_DR9:
                case SVM_EXIT_WRITE_DR10:   case SVM_EXIT_WRITE_DR11:   case SVM_EXIT_WRITE_DR12:   case SVM_EXIT_WRITE_DR13:
                case SVM_EXIT_WRITE_DR14:   case SVM_EXIT_WRITE_DR15:
                    return hmR0SvmExitWriteDRx(pVCpu, pCtx, pSvmTransient);

                case SVM_EXIT_XSETBV:
                    return hmR0SvmExitXsetbv(pVCpu, pCtx, pSvmTransient);

                case SVM_EXIT_TASK_SWITCH:
                    return hmR0SvmExitTaskSwitch(pVCpu, pCtx, pSvmTransient);

                case SVM_EXIT_IRET:
                    return hmR0SvmExitIret(pVCpu, pCtx, pSvmTransient);

                case SVM_EXIT_SHUTDOWN:
                    return hmR0SvmExitShutdown(pVCpu, pCtx, pSvmTransient);

                case SVM_EXIT_SMI:
                case SVM_EXIT_INIT:
                {
                    /*
                     * We don't intercept SMIs. As for INIT signals, it really shouldn't ever happen here.
                     * If it ever does, we want to know about it so log the exit code and bail.
                     */
                    return hmR0SvmExitUnexpected(pVCpu, pCtx, pSvmTransient);
                }

#ifdef VBOX_WITH_NESTED_HWVIRT
                case SVM_EXIT_CLGI:     return hmR0SvmExitClgi(pVCpu, pCtx, pSvmTransient);
                case SVM_EXIT_STGI:     return hmR0SvmExitStgi(pVCpu, pCtx, pSvmTransient);
                case SVM_EXIT_VMLOAD:   return hmR0SvmExitVmload(pVCpu, pCtx, pSvmTransient);
                case SVM_EXIT_VMSAVE:   return hmR0SvmExitVmsave(pVCpu, pCtx, pSvmTransient);
                case SVM_EXIT_INVLPGA:  return hmR0SvmExitInvlpga(pVCpu, pCtx, pSvmTransient);
                case SVM_EXIT_VMRUN:    return hmR0SvmExitVmrun(pVCpu, pCtx, pSvmTransient);
#else
                case SVM_EXIT_CLGI:
                case SVM_EXIT_STGI:
                case SVM_EXIT_VMLOAD:
                case SVM_EXIT_VMSAVE:
                case SVM_EXIT_INVLPGA:
                case SVM_EXIT_VMRUN:
#endif
                case SVM_EXIT_RSM:
                case SVM_EXIT_SKINIT:
                {
                    hmR0SvmSetPendingXcptUD(pVCpu);
                    return VINF_SUCCESS;
                }

#ifdef HMSVM_ALWAYS_TRAP_ALL_XCPTS
                case SVM_EXIT_EXCEPTION_0:             /* X86_XCPT_DE */
                /*   SVM_EXIT_EXCEPTION_1: */          /* X86_XCPT_DB - Handled above. */
                case SVM_EXIT_EXCEPTION_2:             /* X86_XCPT_NMI */
                /*   SVM_EXIT_EXCEPTION_3: */          /* X86_XCPT_BP - Handled above. */
                case SVM_EXIT_EXCEPTION_4:             /* X86_XCPT_OF */
                case SVM_EXIT_EXCEPTION_5:             /* X86_XCPT_BR */
                /*   SVM_EXIT_EXCEPTION_6: */          /* X86_XCPT_UD - Handled above. */
                case SVM_EXIT_EXCEPTION_7:             /* X86_XCPT_NM */
                case SVM_EXIT_EXCEPTION_8:             /* X86_XCPT_DF */
                case SVM_EXIT_EXCEPTION_9:             /* X86_XCPT_CO_SEG_OVERRUN */
                case SVM_EXIT_EXCEPTION_10:            /* X86_XCPT_TS */
                case SVM_EXIT_EXCEPTION_11:            /* X86_XCPT_NP */
                case SVM_EXIT_EXCEPTION_12:            /* X86_XCPT_SS */
                case SVM_EXIT_EXCEPTION_13:            /* X86_XCPT_GP */
                /*   SVM_EXIT_EXCEPTION_14: */         /* X86_XCPT_PF - Handled above. */
                case SVM_EXIT_EXCEPTION_15:            /* Reserved. */
                /*   SVM_EXIT_EXCEPTION_16: */         /* X86_XCPT_MF - Handled above. */
                /*   SVM_EXIT_EXCEPTION_17: */         /* X86_XCPT_AC - Handled above. */
                case SVM_EXIT_EXCEPTION_18:            /* X86_XCPT_MC */
                case SVM_EXIT_EXCEPTION_19:            /* X86_XCPT_XF */
                case SVM_EXIT_EXCEPTION_20: case SVM_EXIT_EXCEPTION_21: case SVM_EXIT_EXCEPTION_22:
                case SVM_EXIT_EXCEPTION_23: case SVM_EXIT_EXCEPTION_24: case SVM_EXIT_EXCEPTION_25:
                case SVM_EXIT_EXCEPTION_26: case SVM_EXIT_EXCEPTION_27: case SVM_EXIT_EXCEPTION_28:
                case SVM_EXIT_EXCEPTION_29: case SVM_EXIT_EXCEPTION_30: case SVM_EXIT_EXCEPTION_31:
                {
                    HMSVM_CHECK_EXIT_DUE_TO_EVENT_DELIVERY();

                    PSVMVMCB pVmcb   = pVCpu->hm.s.svm.pVmcb;
                    SVMEVENT Event;
                    Event.u          = 0;
                    Event.n.u1Valid  = 1;
                    Event.n.u3Type   = SVM_EVENT_EXCEPTION;
                    Event.n.u8Vector = pSvmTransient->u64ExitCode - SVM_EXIT_EXCEPTION_0;

                    switch (Event.n.u8Vector)
                    {
                        case X86_XCPT_DE:
                            STAM_COUNTER_INC(&pVCpu->hm.s.StatExitGuestDE);
                            break;

                        case X86_XCPT_NP:
                            Event.n.u1ErrorCodeValid    = 1;
                            Event.n.u32ErrorCode        = pVmcb->ctrl.u64ExitInfo1;
                            STAM_COUNTER_INC(&pVCpu->hm.s.StatExitGuestNP);
                            break;

                        case X86_XCPT_SS:
                            Event.n.u1ErrorCodeValid    = 1;
                            Event.n.u32ErrorCode        = pVmcb->ctrl.u64ExitInfo1;
                            STAM_COUNTER_INC(&pVCpu->hm.s.StatExitGuestSS);
                            break;

                        case X86_XCPT_GP:
                            Event.n.u1ErrorCodeValid    = 1;
                            Event.n.u32ErrorCode        = pVmcb->ctrl.u64ExitInfo1;
                            STAM_COUNTER_INC(&pVCpu->hm.s.StatExitGuestGP);
                            break;

                        default:
                            AssertMsgFailed(("hmR0SvmHandleExit: Unexpected exit caused by exception %#x\n", Event.n.u8Vector));
                            pVCpu->hm.s.u32HMError = Event.n.u8Vector;
                            return VERR_SVM_UNEXPECTED_XCPT_EXIT;
                    }

                    Log4(("#Xcpt: Vector=%#x at CS:RIP=%04x:%RGv\n", Event.n.u8Vector, pCtx->cs.Sel, (RTGCPTR)pCtx->rip));
                    hmR0SvmSetPendingEvent(pVCpu, &Event, 0 /* GCPtrFaultAddress */);
                    return VINF_SUCCESS;
                }
#endif  /* HMSVM_ALWAYS_TRAP_ALL_XCPTS */

                default:
                {
                    AssertMsgFailed(("hmR0SvmHandleExit: Unknown exit code %#x\n", u32ExitCode));
                    pVCpu->hm.s.u32HMError = u32ExitCode;
                    return VERR_SVM_UNKNOWN_EXIT;
                }
            }
        }
    }
    /* not reached */
}


#ifdef DEBUG
/* Is there some generic IPRT define for this that are not in Runtime/internal/\* ?? */
# define HMSVM_ASSERT_PREEMPT_CPUID_VAR() \
    RTCPUID const idAssertCpu = RTThreadPreemptIsEnabled(NIL_RTTHREAD) ? NIL_RTCPUID : RTMpCpuId()

# define HMSVM_ASSERT_PREEMPT_CPUID() \
    do \
    { \
         RTCPUID const idAssertCpuNow = RTThreadPreemptIsEnabled(NIL_RTTHREAD) ? NIL_RTCPUID : RTMpCpuId(); \
         AssertMsg(idAssertCpu == idAssertCpuNow, ("SVM %#x, %#x\n", idAssertCpu, idAssertCpuNow)); \
    } while (0)

# define HMSVM_VALIDATE_EXIT_HANDLER_PARAMS() \
    do { \
        AssertPtr(pVCpu); \
        AssertPtr(pCtx); \
        AssertPtr(pSvmTransient); \
        Assert(ASMIntAreEnabled()); \
        HMSVM_ASSERT_PREEMPT_SAFE(); \
        HMSVM_ASSERT_PREEMPT_CPUID_VAR(); \
        Log4Func(("vcpu[%u] -v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-v-\n", (uint32_t)pVCpu->idCpu)); \
        HMSVM_ASSERT_PREEMPT_SAFE(); \
        if (VMMR0IsLogFlushDisabled(pVCpu)) \
            HMSVM_ASSERT_PREEMPT_CPUID(); \
    } while (0)
#else   /* Release builds */
# define HMSVM_VALIDATE_EXIT_HANDLER_PARAMS() do { NOREF(pVCpu); NOREF(pCtx); NOREF(pSvmTransient); } while (0)
#endif


/**
 * Worker for hmR0SvmInterpretInvlpg().
 *
 * @return VBox status code.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCpu            Pointer to the disassembler state.
 * @param   pCtx            The guest CPU context.
 */
static int hmR0SvmInterpretInvlPgEx(PVMCPU pVCpu, PDISCPUSTATE pCpu, PCPUMCTX pCtx)
{
    DISQPVPARAMVAL Param1;
    RTGCPTR        GCPtrPage;

    int rc = DISQueryParamVal(CPUMCTX2CORE(pCtx), pCpu, &pCpu->Param1, &Param1, DISQPVWHICH_SRC);
    if (RT_FAILURE(rc))
        return VERR_EM_INTERPRETER;

    if (   Param1.type == DISQPV_TYPE_IMMEDIATE
        || Param1.type == DISQPV_TYPE_ADDRESS)
    {
        if (!(Param1.flags & (DISQPV_FLAG_32 | DISQPV_FLAG_64)))
            return VERR_EM_INTERPRETER;

        GCPtrPage = Param1.val.val64;
        VBOXSTRICTRC rc2 = EMInterpretInvlpg(pVCpu->CTX_SUFF(pVM), pVCpu, CPUMCTX2CORE(pCtx), GCPtrPage);
        rc = VBOXSTRICTRC_VAL(rc2);
    }
    else
    {
        Log4(("hmR0SvmInterpretInvlPgEx invalid parameter type %#x\n", Param1.type));
        rc = VERR_EM_INTERPRETER;
    }

    return rc;
}


/**
 * Interprets INVLPG.
 *
 * @returns VBox status code.
 * @retval  VINF_*                  Scheduling instructions.
 * @retval  VERR_EM_INTERPRETER     Something we can't cope with.
 * @retval  VERR_*                  Fatal errors.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        The guest CPU context.
 *
 * @remarks Updates the RIP if the instruction was executed successfully.
 */
static int hmR0SvmInterpretInvlpg(PVM pVM, PVMCPU pVCpu, PCPUMCTX pCtx)
{
    /* Only allow 32 & 64 bit code. */
    if (CPUMGetGuestCodeBits(pVCpu) != 16)
    {
        PDISSTATE pDis = &pVCpu->hm.s.DisState;
        int rc = EMInterpretDisasCurrent(pVM, pVCpu, pDis, NULL /* pcbInstr */);
        if (   RT_SUCCESS(rc)
            && pDis->pCurInstr->uOpcode == OP_INVLPG)
        {
            rc = hmR0SvmInterpretInvlPgEx(pVCpu, pDis, pCtx);
            if (RT_SUCCESS(rc))
                hmR0SvmAdvanceRip(pVCpu, pCtx, pDis->cbInstr);
            return rc;
        }
        else
            Log4(("hmR0SvmInterpretInvlpg: EMInterpretDisasCurrent returned %Rrc uOpCode=%#x\n", rc, pDis->pCurInstr->uOpcode));
    }
    return VERR_EM_INTERPRETER;
}


#ifdef HMSVM_USE_IEM_EVENT_REFLECTION
/**
 * Gets the IEM exception flags for the specified SVM event.
 *
 * @returns The IEM exception flags.
 * @param   pEvent      Pointer to the SVM event.
 *
 * @remarks This function currently only constructs flags required for
 *          IEMEvaluateRecursiveXcpt and not the complete flags (e.g. error-code
 *          and CR2 aspects of an exception are not included).
 */
static uint32_t hmR0SvmGetIemXcptFlags(PCSVMEVENT pEvent)
{
    uint8_t const uEventType = pEvent->n.u3Type;
    uint32_t      fIemXcptFlags;
    switch (uEventType)
    {
        case SVM_EVENT_EXCEPTION:
            /*
             * Only INT3 and INTO instructions can raise #BP and #OF exceptions.
             * See AMD spec. Table 8-1. "Interrupt Vector Source and Cause".
             */
            if (pEvent->n.u8Vector == X86_XCPT_BP)
            {
                fIemXcptFlags = IEM_XCPT_FLAGS_T_SOFT_INT | IEM_XCPT_FLAGS_BP_INSTR;
                break;
            }
            if (pEvent->n.u8Vector == X86_XCPT_OF)
            {
                fIemXcptFlags = IEM_XCPT_FLAGS_T_SOFT_INT | IEM_XCPT_FLAGS_OF_INSTR;
                break;
            }
            /** @todo How do we distinguish ICEBP \#DB from the regular one? */
            RT_FALL_THRU();
        case SVM_EVENT_NMI:
            fIemXcptFlags = IEM_XCPT_FLAGS_T_CPU_XCPT;
            break;

        case SVM_EVENT_EXTERNAL_IRQ:
            fIemXcptFlags = IEM_XCPT_FLAGS_T_EXT_INT;
            break;

        case SVM_EVENT_SOFTWARE_INT:
            fIemXcptFlags = IEM_XCPT_FLAGS_T_SOFT_INT;
            break;

        default:
            fIemXcptFlags = 0;
            AssertMsgFailed(("Unexpected event type! uEventType=%#x uVector=%#x", uEventType, pEvent->n.u8Vector));
            break;
    }
    return fIemXcptFlags;
}

#else
/**
 * Determines if an exception is a contributory exception.
 *
 * Contributory exceptions are ones which can cause double-faults unless the
 * original exception was a benign exception. Page-fault is intentionally not
 * included here as it's a conditional contributory exception.
 *
 * @returns true if the exception is contributory, false otherwise.
 * @param   uVector     The exception vector.
 */
DECLINLINE(bool) hmR0SvmIsContributoryXcpt(const uint32_t uVector)
{
    switch (uVector)
    {
        case X86_XCPT_GP:
        case X86_XCPT_SS:
        case X86_XCPT_NP:
        case X86_XCPT_TS:
        case X86_XCPT_DE:
            return true;
        default:
            break;
    }
    return false;
}
#endif /* HMSVM_USE_IEM_EVENT_REFLECTION */


/**
 * Handle a condition that occurred while delivering an event through the guest
 * IDT.
 *
 * @returns VBox status code (informational error codes included).
 * @retval VINF_SUCCESS if we should continue handling the \#VMEXIT.
 * @retval VINF_HM_DOUBLE_FAULT if a \#DF condition was detected and we ought to
 *         continue execution of the guest which will delivery the \#DF.
 * @retval VINF_EM_RESET if we detected a triple-fault condition.
 * @retval VERR_EM_GUEST_CPU_HANG if we detected a guest CPU hang.
 *
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pCtx            Pointer to the guest-CPU context.
 * @param   pSvmTransient   Pointer to the SVM transient structure.
 *
 * @remarks No-long-jump zone!!!
 */
static int hmR0SvmCheckExitDueToEventDelivery(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    int rc = VINF_SUCCESS;
    PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;

    Log4(("EXITINTINFO: Pending vectoring event %#RX64 Valid=%RTbool ErrValid=%RTbool Err=%#RX32 Type=%u Vector=%u\n",
          pVmcb->ctrl.ExitIntInfo.u, !!pVmcb->ctrl.ExitIntInfo.n.u1Valid, !!pVmcb->ctrl.ExitIntInfo.n.u1ErrorCodeValid,
          pVmcb->ctrl.ExitIntInfo.n.u32ErrorCode, pVmcb->ctrl.ExitIntInfo.n.u3Type, pVmcb->ctrl.ExitIntInfo.n.u8Vector));

    /* See AMD spec. 15.7.3 "EXITINFO Pseudo-Code". The EXITINTINFO (if valid) contains the prior exception (IDT vector)
     * that was trying to be delivered to the guest which caused a #VMEXIT which was intercepted (Exit vector). */
    if (pVmcb->ctrl.ExitIntInfo.n.u1Valid)
    {
#ifdef HMSVM_USE_IEM_EVENT_REFLECTION
        IEMXCPTRAISE     enmRaise;
        IEMXCPTRAISEINFO fRaiseInfo;
        bool const       fExitIsHwXcpt  = pSvmTransient->u64ExitCode - SVM_EXIT_EXCEPTION_0 <= SVM_EXIT_EXCEPTION_31;
        uint8_t const    uIdtVector     = pVmcb->ctrl.ExitIntInfo.n.u8Vector;
        if (fExitIsHwXcpt)
        {
            uint8_t  const uExitVector      = pSvmTransient->u64ExitCode - SVM_EXIT_EXCEPTION_0;
            uint32_t const fIdtVectorFlags  = hmR0SvmGetIemXcptFlags(&pVmcb->ctrl.ExitIntInfo);
            uint32_t const fExitVectorFlags = IEM_XCPT_FLAGS_T_CPU_XCPT;
            enmRaise = IEMEvaluateRecursiveXcpt(pVCpu, fIdtVectorFlags, uIdtVector, fExitVectorFlags, uExitVector, &fRaiseInfo);
        }
        else
        {
            /*
             * If delivery of an event caused a #VMEXIT that is not an exception (e.g. #NPF) then we
             * end up here.
             *
             * If the event was:
             *   - a software interrupt, we can re-execute the instruction which will regenerate
             *     the event.
             *   - an NMI, we need to clear NMI blocking and re-inject the NMI.
             *   - a hardware exception or external interrupt, we re-inject it.
             */
            fRaiseInfo = IEMXCPTRAISEINFO_NONE;
            if (pVmcb->ctrl.ExitIntInfo.n.u3Type == SVM_EVENT_SOFTWARE_INT)
                enmRaise = IEMXCPTRAISE_REEXEC_INSTR;
            else
                enmRaise = IEMXCPTRAISE_PREV_EVENT;
        }

        switch (enmRaise)
        {
            case IEMXCPTRAISE_CURRENT_XCPT:
            case IEMXCPTRAISE_PREV_EVENT:
            {
                /* For software interrupts, we shall re-execute the instruction. */
                if (!(fRaiseInfo & IEMXCPTRAISEINFO_SOFT_INT_XCPT))
                {
                    RTGCUINTPTR GCPtrFaultAddress = 0;

                    /* If we are re-injecting an NMI, clear NMI blocking. */
                    if (pVmcb->ctrl.ExitIntInfo.n.u3Type == SVM_EVENT_NMI)
                        VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_BLOCK_NMIS);

                    /* Determine a vectoring #PF condition, see comment in hmR0SvmExitXcptPF(). */
                    if (fRaiseInfo & (IEMXCPTRAISEINFO_EXT_INT_PF | IEMXCPTRAISEINFO_NMI_PF))
                        pSvmTransient->fVectoringPF = true;
                    else if (   pVmcb->ctrl.ExitIntInfo.n.u3Type == SVM_EVENT_EXCEPTION
                             && uIdtVector == X86_XCPT_PF)
                    {
                        /*
                         * If the previous exception was a #PF, we need to recover the CR2 value.
                         * This can't happen with shadow paging.
                         */
                        GCPtrFaultAddress = pCtx->cr2;
                    }

                    /*
                     * Without nested paging, when uExitVector is #PF, CR2 value will be updated from the VMCB's
                     * exit info. fields, if it's a guest #PF, see hmR0SvmExitXcptPF().
                     */
                    Assert(pVmcb->ctrl.ExitIntInfo.n.u3Type != SVM_EVENT_SOFTWARE_INT);
                    STAM_COUNTER_INC(&pVCpu->hm.s.StatInjectPendingReflect);
                    hmR0SvmSetPendingEvent(pVCpu, &pVmcb->ctrl.ExitIntInfo, GCPtrFaultAddress);

                    Log4(("IDT: Pending vectoring event %#RX64 ErrValid=%RTbool Err=%#RX32 GCPtrFaultAddress=%#RX64\n",
                          pVmcb->ctrl.ExitIntInfo.u, RT_BOOL(pVmcb->ctrl.ExitIntInfo.n.u1ErrorCodeValid),
                          pVmcb->ctrl.ExitIntInfo.n.u32ErrorCode, GCPtrFaultAddress));
                }
                break;
            }

            case IEMXCPTRAISE_REEXEC_INSTR:
            {
                Assert(rc == VINF_SUCCESS);
                break;
            }

            case IEMXCPTRAISE_DOUBLE_FAULT:
            {
                /*
                 * Determing a vectoring double #PF condition. Used later, when PGM evaluates the
                 * second #PF as a guest #PF (and not a shadow #PF) and needs to be converted into a #DF.
                 */
                if (fRaiseInfo & IEMXCPTRAISEINFO_PF_PF)
                {
                    pSvmTransient->fVectoringDoublePF = true;
                    Assert(rc == VINF_SUCCESS);
                }
                else
                {
                    STAM_COUNTER_INC(&pVCpu->hm.s.StatInjectPendingReflect);
                    hmR0SvmSetPendingXcptDF(pVCpu);
                    rc = VINF_HM_DOUBLE_FAULT;
                }
                break;
            }

            case IEMXCPTRAISE_TRIPLE_FAULT:
            {
                rc = VINF_EM_RESET;
                break;
            }

            case IEMXCPTRAISE_CPU_HANG:
            {
                rc = VERR_EM_GUEST_CPU_HANG;
                break;
            }

            default:
            {
                AssertMsgFailed(("hmR0SvmExitCpuid: EMInterpretCpuId failed with %Rrc\n", rc));
                rc = VERR_SVM_IPE_2;
                break;
            }
        }
#else
        uint8_t uIdtVector  = pVmcb->ctrl.ExitIntInfo.n.u8Vector;

        typedef enum
        {
            SVMREFLECTXCPT_XCPT,    /* Reflect the exception to the guest or for further evaluation by VMM. */
            SVMREFLECTXCPT_DF,      /* Reflect the exception as a double-fault to the guest. */
            SVMREFLECTXCPT_TF,      /* Indicate a triple faulted state to the VMM. */
            SVMREFLECTXCPT_HANG,    /* Indicate bad VM trying to deadlock the CPU. */
            SVMREFLECTXCPT_NONE     /* Nothing to reflect. */
        } SVMREFLECTXCPT;

        SVMREFLECTXCPT enmReflect = SVMREFLECTXCPT_NONE;
        bool fReflectingNmi = false;
        if (pVmcb->ctrl.ExitIntInfo.n.u3Type == SVM_EVENT_EXCEPTION)
        {
            if (pSvmTransient->u64ExitCode - SVM_EXIT_EXCEPTION_0 <= SVM_EXIT_EXCEPTION_31)
            {
                uint8_t uExitVector = (uint8_t)(pSvmTransient->u64ExitCode - SVM_EXIT_EXCEPTION_0);

#ifdef VBOX_STRICT
                if (   hmR0SvmIsContributoryXcpt(uIdtVector)
                    && uExitVector == X86_XCPT_PF)
                {
                    Log4(("IDT: Contributory #PF idCpu=%u uCR2=%#RX64\n", pVCpu->idCpu, pCtx->cr2));
                }
#endif

                if (   uIdtVector == X86_XCPT_BP
                    || uIdtVector == X86_XCPT_OF)
                {
                    /* Ignore INT3/INTO, just re-execute. See @bugref{8357}. */
                }
                else if (   uExitVector == X86_XCPT_PF
                         && uIdtVector  == X86_XCPT_PF)
                {
                    pSvmTransient->fVectoringDoublePF = true;
                    Log4(("IDT: Vectoring double #PF uCR2=%#RX64\n", pCtx->cr2));
                }
                else if (   uExitVector == X86_XCPT_AC
                         && uIdtVector == X86_XCPT_AC)
                {
                    enmReflect = SVMREFLECTXCPT_HANG;
                    Log4(("IDT: Nested #AC - Bad guest\n"));
                }
                else if (   (pVmcb->ctrl.u32InterceptXcpt & HMSVM_CONTRIBUTORY_XCPT_MASK)
                         && hmR0SvmIsContributoryXcpt(uExitVector)
                         && (   hmR0SvmIsContributoryXcpt(uIdtVector)
                             || uIdtVector == X86_XCPT_PF))
                {
                    enmReflect = SVMREFLECTXCPT_DF;
                    Log4(("IDT: Pending vectoring #DF %#RX64 uIdtVector=%#x uExitVector=%#x\n", pVCpu->hm.s.Event.u64IntInfo,
                          uIdtVector, uExitVector));
                }
                else if (uIdtVector == X86_XCPT_DF)
                {
                    enmReflect = SVMREFLECTXCPT_TF;
                    Log4(("IDT: Pending vectoring triple-fault %#RX64 uIdtVector=%#x uExitVector=%#x\n",
                          pVCpu->hm.s.Event.u64IntInfo, uIdtVector, uExitVector));
                }
                else
                    enmReflect = SVMREFLECTXCPT_XCPT;
            }
            else
            {
                /*
                 * If event delivery caused an #VMEXIT that is not an exception (e.g. #NPF) then reflect the original
                 * exception to the guest after handling the #VMEXIT.
                 */
                enmReflect = SVMREFLECTXCPT_XCPT;
            }
        }
        else if (   pVmcb->ctrl.ExitIntInfo.n.u3Type == SVM_EVENT_EXTERNAL_IRQ
                 || pVmcb->ctrl.ExitIntInfo.n.u3Type == SVM_EVENT_NMI)
        {
            enmReflect = SVMREFLECTXCPT_XCPT;
            fReflectingNmi = RT_BOOL(pVmcb->ctrl.ExitIntInfo.n.u3Type == SVM_EVENT_NMI);

            if (pSvmTransient->u64ExitCode - SVM_EXIT_EXCEPTION_0 <= SVM_EXIT_EXCEPTION_31)
            {
                uint8_t uExitVector = (uint8_t)(pSvmTransient->u64ExitCode - SVM_EXIT_EXCEPTION_0);
                if (uExitVector == X86_XCPT_PF)
                {
                    pSvmTransient->fVectoringPF = true;
                    Log4(("IDT: Vectoring #PF due to Ext-Int/NMI. uCR2=%#RX64\n", pCtx->cr2));
                }
            }
        }
        /* else: Ignore software interrupts (INT n) as they reoccur when restarting the instruction. */

        switch (enmReflect)
        {
            case SVMREFLECTXCPT_XCPT:
            {
                /* If we are re-injecting the NMI, clear NMI blocking. */
                if (fReflectingNmi)
                    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_BLOCK_NMIS);

                Assert(pVmcb->ctrl.ExitIntInfo.n.u3Type != SVM_EVENT_SOFTWARE_INT);
                STAM_COUNTER_INC(&pVCpu->hm.s.StatInjectPendingReflect);
                hmR0SvmSetPendingEvent(pVCpu, &pVmcb->ctrl.ExitIntInfo, 0 /* GCPtrFaultAddress */);

                /* If uExitVector is #PF, CR2 value will be updated from the VMCB if it's a guest #PF. See hmR0SvmExitXcptPF(). */
                Log4(("IDT: Pending vectoring event %#RX64 ErrValid=%RTbool Err=%#RX32\n", pVmcb->ctrl.ExitIntInfo.u,
                      !!pVmcb->ctrl.ExitIntInfo.n.u1ErrorCodeValid, pVmcb->ctrl.ExitIntInfo.n.u32ErrorCode));
                break;
            }

            case SVMREFLECTXCPT_DF:
            {
                STAM_COUNTER_INC(&pVCpu->hm.s.StatInjectPendingReflect);
                hmR0SvmSetPendingXcptDF(pVCpu);
                rc = VINF_HM_DOUBLE_FAULT;
                break;
            }

            case SVMREFLECTXCPT_TF:
            {
                rc = VINF_EM_RESET;
                break;
            }

            case SVMREFLECTXCPT_HANG:
            {
                rc = VERR_EM_GUEST_CPU_HANG;
                break;
            }

            default:
                Assert(rc == VINF_SUCCESS);
                break;
        }
#endif  /* HMSVM_USE_IEM_EVENT_REFLECTION */
    }
    Assert(rc == VINF_SUCCESS || rc == VINF_HM_DOUBLE_FAULT || rc == VINF_EM_RESET || rc == VERR_EM_GUEST_CPU_HANG);
    NOREF(pCtx);
    return rc;
}


/**
 * Returns whether the NRIP_SAVE feature is supported.
 *
 * @return @c true if supported, @c false otherwise.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(bool) hmR0SvmSupportsNextRipSave(PVMCPU pVCpu)
{
    return RT_BOOL(pVCpu->CTX_SUFF(pVM)->hm.s.svm.u32Features & X86_CPUID_SVM_FEATURE_EDX_NRIP_SAVE);
}


/**
 * Returns whether the Decode Assist feature is supported.
 *
 * @return @c true if supported, @c false otherwise.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(bool) hmR0SvmSupportsDecodeAssists(PVMCPU pVCpu)
{
    return RT_BOOL(pVCpu->CTX_SUFF(pVM)->hm.s.svm.u32Features & X86_CPUID_SVM_FEATURE_EDX_DECODE_ASSIST);
}


#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * Gets the length of the current instruction if the CPU supports the NRIP_SAVE
 * feature. Otherwise, returns the value in @a cbLikely.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   cbLikely    The likely instruction length.
 */
DECLINLINE(uint8_t) hmR0SvmGetInstrLengthHwAssist(PVMCPU pVCpu, PCPUMCTX pCtx, uint8_t cbLikely)
{
    Assert(cbLikely <= 15);   /* See Intel spec. 2.3.11 "AVX Instruction Length" */
    if (hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        uint8_t const cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
        Assert(cbInstr == cbLikely);
        return cbInstr;
    }
    return cbLikely;
}
#endif


/* -=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- #VMEXIT handlers -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

/** @name \#VMEXIT handlers.
 * @{
 */

/**
 * \#VMEXIT handler for external interrupts, NMIs, FPU assertion freeze and INIT
 * signals (SVM_EXIT_INTR, SVM_EXIT_NMI, SVM_EXIT_FERR_FREEZE, SVM_EXIT_INIT).
 */
HMSVM_EXIT_DECL hmR0SvmExitIntr(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    if (pSvmTransient->u64ExitCode == SVM_EXIT_NMI)
        STAM_REL_COUNTER_INC(&pVCpu->hm.s.StatExitHostNmiInGC);
    else if (pSvmTransient->u64ExitCode == SVM_EXIT_INTR)
        STAM_COUNTER_INC(&pVCpu->hm.s.StatExitExtInt);

    /*
     * AMD-V has no preemption timer and the generic periodic preemption timer has no way to signal -before- the timer
     * fires if the current interrupt is our own timer or a some other host interrupt. We also cannot examine what
     * interrupt it is until the host actually take the interrupt.
     *
     * Going back to executing guest code here unconditionally causes random scheduling problems (observed on an
     * AMD Phenom 9850 Quad-Core on Windows 64-bit host).
     */
    return VINF_EM_RAW_INTERRUPT;
}


/**
 * \#VMEXIT handler for WBINVD (SVM_EXIT_WBINVD). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitWbinvd(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    int rc;
    unsigned cbInstr;
    if (hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
        rc = VINF_SUCCESS;
    }
    else
    {
        PDISCPUSTATE pDis = &pVCpu->hm.s.DisState;
        rc = EMInterpretDisasCurrent(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, &cbInstr);
        if (   rc == VINF_SUCCESS
            && pDis->pCurInstr->uOpcode == OP_WBINVD)
            Assert(cbInstr > 0);
        else
        {
            cbInstr = 0;
            rc = VERR_EM_INTERPRETER;
        }
    }

    if (rc == VINF_SUCCESS)
        hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitWbinvd);
    HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
    return rc;
}


/**
 * \#VMEXIT handler for INVD (SVM_EXIT_INVD). Unconditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitInvd(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    int rc;
    unsigned cbInstr;
    if (hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
        rc = VINF_SUCCESS;
    }
    else
    {
        PDISCPUSTATE pDis = &pVCpu->hm.s.DisState;
        rc = EMInterpretDisasCurrent(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, &cbInstr);
        if (   rc == VINF_SUCCESS
            && pDis->pCurInstr->uOpcode == OP_INVD)
            Assert(cbInstr > 0);
        else
        {
            cbInstr = 0;
            rc = VERR_EM_INTERPRETER;
        }
    }

    if (rc == VINF_SUCCESS)
        hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitInvd);
    HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
    return rc;
}


/**
 * \#VMEXIT handler for INVD (SVM_EXIT_CPUID). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitCpuid(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    PVM pVM = pVCpu->CTX_SUFF(pVM);

    int rc;
    unsigned cbInstr = 0;
    if (hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
    }
    else
    {
        PDISCPUSTATE pDis = &pVCpu->hm.s.DisState;
        rc = EMInterpretDisasCurrent(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, &cbInstr);
        if (   rc == VINF_SUCCESS
            && pDis->pCurInstr->uOpcode == OP_CPUID)
            Assert(cbInstr > 0);
        else
            cbInstr = 0;
    }

    if (cbInstr)
    {
        rc = EMInterpretCpuId(pVM, pVCpu, CPUMCTX2CORE(pCtx));
        if (RT_LIKELY(rc == VINF_SUCCESS))
        {
            hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);
            HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
        }
        else
            rc = VERR_EM_INTERPRETER;
    }
    else
        rc = VERR_EM_INTERPRETER;
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitCpuid);
    return rc;
}


/**
 * \#VMEXIT handler for RDTSC (SVM_EXIT_RDTSC). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitRdtsc(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    int rc;
    unsigned cbInstr = 0;
    if (hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
    }
    else
    {
        PDISCPUSTATE pDis = &pVCpu->hm.s.DisState;
        rc = EMInterpretDisasCurrent(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, &cbInstr);
        if (   rc == VINF_SUCCESS
            && pDis->pCurInstr->uOpcode == OP_RDTSC)
            Assert(cbInstr > 0);
        else
            cbInstr = 0;
    }

    if (cbInstr)
    {
        rc = EMInterpretRdtsc(pVCpu->CTX_SUFF(pVM), pVCpu, CPUMCTX2CORE(pCtx));
        if (RT_LIKELY(rc == VINF_SUCCESS))
        {
            pSvmTransient->fUpdateTscOffsetting = true;
            hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);
            HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
        }
        else
            rc = VERR_EM_INTERPRETER;
    }
    else
        rc = VERR_EM_INTERPRETER;
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitRdtsc);
    return rc;
}


/**
 * \#VMEXIT handler for RDTSCP (SVM_EXIT_RDTSCP). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitRdtscp(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    VBOXSTRICTRC rcStrict;
    if (hmR0SvmSupportsNextRipSave(pVCpu))
    {
        rcStrict = EMInterpretRdtscp(pVCpu->CTX_SUFF(pVM), pVCpu, pCtx);
        if (RT_LIKELY(rcStrict == VINF_SUCCESS))
        {
            PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
            uint8_t const cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
            hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);
            pSvmTransient->fUpdateTscOffsetting = true;
        }
        else
        {
            AssertMsgFailed(("hmR0SvmExitRdtsc: EMInterpretRdtscp failed with %Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
            rcStrict = VERR_EM_INTERPRETER;
        }
    }
    else
        rcStrict = EMInterpretInstruction(pVCpu, CPUMCTX2CORE(pCtx), 0 /* pvFault */);
    HMSVM_CHECK_SINGLE_STEP(pVCpu, rcStrict);
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitRdtscp);
    return VBOXSTRICTRC_TODO(rcStrict);
}


/**
 * \#VMEXIT handler for RDPMC (SVM_EXIT_RDPMC). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitRdpmc(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    int rc;
    unsigned cbInstr = 0;
    if (hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
    }
    else
    {
        PDISCPUSTATE pDis = &pVCpu->hm.s.DisState;
        rc = EMInterpretDisasCurrent(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, &cbInstr);
        if (   rc == VINF_SUCCESS
            && pDis->pCurInstr->uOpcode == OP_RDPMC)
            Assert(cbInstr > 0);
        else
            cbInstr = 0;
    }

    if (cbInstr)
    {
        rc = EMInterpretRdpmc(pVCpu->CTX_SUFF(pVM), pVCpu, CPUMCTX2CORE(pCtx));
        if (RT_LIKELY(rc == VINF_SUCCESS))
        {
            hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);
            HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
        }
        else
            rc = VERR_EM_INTERPRETER;
    }
    else
        rc = VERR_EM_INTERPRETER;
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitRdtsc);
    return rc;
}


/**
 * \#VMEXIT handler for INVLPG (SVM_EXIT_INVLPG). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitInvlpg(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    Assert(!pVM->hm.s.fNestedPaging);
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitInvlpg);

    if (   hmR0SvmSupportsDecodeAssists(pVCpu)
        && hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        uint8_t const cbInstr   = pVmcb->ctrl.u64NextRIP - pCtx->rip;
        RTGCPTR const GCPtrPage = pVmcb->ctrl.u64ExitInfo1;
        VBOXSTRICTRC rcStrict = IEMExecDecodedInvlpg(pVCpu, cbInstr, GCPtrPage);
        HMSVM_CHECK_SINGLE_STEP(pVCpu, rcStrict);
        return VBOXSTRICTRC_VAL(rcStrict);
    }

    int rc = hmR0SvmInterpretInvlpg(pVM, pVCpu, pCtx);    /* Updates RIP if successful. */
    Assert(rc == VINF_SUCCESS || rc == VERR_EM_INTERPRETER);
    HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
    return rc;
}


/**
 * \#VMEXIT handler for HLT (SVM_EXIT_HLT). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitHlt(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    int rc;
    unsigned cbInstr = 0;
    if (hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
    }
    else
    {
        PDISCPUSTATE pDis = &pVCpu->hm.s.DisState;
        rc = EMInterpretDisasCurrent(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, &cbInstr);
        if (   rc == VINF_SUCCESS
            && pDis->pCurInstr->uOpcode == OP_HLT)
            Assert(cbInstr > 0);
        else
            cbInstr = 0;
    }

    if (cbInstr)
    {
        hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);
        rc = EMShouldContinueAfterHalt(pVCpu, pCtx) ? VINF_SUCCESS : VINF_EM_HALT;
    }
    else
        rc = VERR_EM_INTERPRETER;

    HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitHlt);
    if (rc != VINF_SUCCESS)
        STAM_COUNTER_INC(&pVCpu->hm.s.StatSwitchHltToR3);
    return rc;
}


/**
 * \#VMEXIT handler for MONITOR (SVM_EXIT_MONITOR). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitMonitor(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    int rc;
    unsigned cbInstr = 0;
    if (hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
    }
    else
    {
        PDISCPUSTATE pDis = &pVCpu->hm.s.DisState;
        rc = EMInterpretDisasCurrent(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, &cbInstr);
        if (   rc == VINF_SUCCESS
            && pDis->pCurInstr->uOpcode == OP_MONITOR)
            Assert(cbInstr > 0);
        else
            cbInstr = 0;
    }

    if (cbInstr)
    {
        rc = EMInterpretMonitor(pVCpu->CTX_SUFF(pVM), pVCpu, CPUMCTX2CORE(pCtx));
        if (RT_LIKELY(rc == VINF_SUCCESS))
        {
            hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);
            HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
        }
        else
            rc = VERR_EM_INTERPRETER;
    }
    else
        rc = VERR_EM_INTERPRETER;
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitMonitor);
    return rc;
}


/**
 * \#VMEXIT handler for MWAIT (SVM_EXIT_MWAIT). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitMwait(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    int rc;
    unsigned cbInstr = 0;
    if (hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
    }
    else
    {
        PDISCPUSTATE pDis = &pVCpu->hm.s.DisState;
        rc = EMInterpretDisasCurrent(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, &cbInstr);
        if (   rc == VINF_SUCCESS
            && pDis->pCurInstr->uOpcode == OP_MWAIT)
            Assert(cbInstr > 0);
        else
            cbInstr = 0;
    }

    VBOXSTRICTRC rcStrict;
    if (cbInstr)
    {
        rcStrict = EMInterpretMWait(pVCpu->CTX_SUFF(pVM), pVCpu, CPUMCTX2CORE(pCtx));
        if (    rcStrict == VINF_EM_HALT
            ||  rcStrict == VINF_SUCCESS)
        {
            hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);
            if (   rcStrict == VINF_EM_HALT
                && EMMonitorWaitShouldContinue(pVCpu, pCtx))
                rcStrict = VINF_SUCCESS;
        }
        else
            rcStrict = VERR_EM_INTERPRETER;
    }
    else
        rcStrict = VERR_EM_INTERPRETER;

    HMSVM_CHECK_SINGLE_STEP(pVCpu, rcStrict);
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitMwait);
    return VBOXSTRICTRC_TODO(rcStrict);
}


/**
 * \#VMEXIT handler for shutdown (triple-fault) (SVM_EXIT_SHUTDOWN). Conditional
 * \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitShutdown(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    return VINF_EM_RESET;
}


/**
 * \#VMEXIT handler for unexpected exits. Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitUnexpected(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    RT_NOREF(pCtx);
    AssertMsgFailed(("hmR0SvmExitUnexpected: ExitCode=%#RX64\n", pSvmTransient->u64ExitCode));
    pVCpu->hm.s.u32HMError = (uint32_t)pSvmTransient->u64ExitCode;
    return VERR_SVM_UNEXPECTED_EXIT;
}


/**
 * \#VMEXIT handler for CRx reads (SVM_EXIT_READ_CR*). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitReadCRx(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    Log4(("hmR0SvmExitReadCRx: CS:RIP=%04x:%#RX64\n", pCtx->cs.Sel, pCtx->rip));
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitCRxRead[pSvmTransient->u64ExitCode - SVM_EXIT_READ_CR0]);

    if (   hmR0SvmSupportsDecodeAssists(pVCpu)
        && hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        bool const fMovCRx = RT_BOOL(pVmcb->ctrl.u64ExitInfo1 & SVM_EXIT1_MOV_CRX_MASK);
        if (fMovCRx)
        {
            uint8_t const cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
            uint8_t const iCrReg  = pSvmTransient->u64ExitCode - SVM_EXIT_READ_CR0;
            uint8_t const iGReg   = pVmcb->ctrl.u64ExitInfo1 & SVM_EXIT1_MOV_CRX_GPR_NUMBER;
            VBOXSTRICTRC rcStrict = IEMExecDecodedMovCRxRead(pVCpu, cbInstr, iGReg, iCrReg);
            HMSVM_CHECK_SINGLE_STEP(pVCpu, rcStrict);
            return VBOXSTRICTRC_VAL(rcStrict);
        }
        /* else: SMSW instruction, fall back below to IEM for this. */
    }

    VBOXSTRICTRC rc2 = EMInterpretInstruction(pVCpu, CPUMCTX2CORE(pCtx), 0 /* pvFault */);
    int rc = VBOXSTRICTRC_VAL(rc2);
    AssertMsg(rc == VINF_SUCCESS || rc == VERR_EM_INTERPRETER || rc == VINF_PGM_CHANGE_MODE || rc == VINF_PGM_SYNC_CR3,
              ("hmR0SvmExitReadCRx: EMInterpretInstruction failed rc=%Rrc\n", rc));
    Assert((pSvmTransient->u64ExitCode - SVM_EXIT_READ_CR0) <= 15);
    HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
    return rc;
}


/**
 * \#VMEXIT handler for CRx writes (SVM_EXIT_WRITE_CR*). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitWriteCRx(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    uint8_t const iCrReg = pSvmTransient->u64ExitCode - SVM_EXIT_WRITE_CR0;
    Assert(iCrReg <= 15);

    VBOXSTRICTRC rcStrict = VERR_SVM_IPE_5;
    bool fDecodedInstr = false;
    if (   hmR0SvmSupportsDecodeAssists(pVCpu)
        && hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        bool const fMovCRx = RT_BOOL(pVmcb->ctrl.u64ExitInfo1 & SVM_EXIT1_MOV_CRX_MASK);
        if (fMovCRx)
        {
            uint8_t const cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
            uint8_t const iGReg   = pVmcb->ctrl.u64ExitInfo1 & SVM_EXIT1_MOV_CRX_GPR_NUMBER;
            Log4(("hmR0SvmExitWriteCRx: Mov CR%u w/ iGReg=%#x\n", iCrReg, iGReg));
            rcStrict = IEMExecDecodedMovCRxWrite(pVCpu, cbInstr, iCrReg, iGReg);
            fDecodedInstr = true;
        }
        /* else: LMSW or CLTS instruction, fall back below to IEM for this. */
    }

    if (!fDecodedInstr)
    {
        Log4(("hmR0SvmExitWriteCRx: iCrReg=%#x\n", iCrReg));
        rcStrict = IEMExecOneBypassEx(pVCpu, CPUMCTX2CORE(pCtx), NULL);
        if (RT_UNLIKELY(   rcStrict == VERR_IEM_ASPECT_NOT_IMPLEMENTED
                        || rcStrict == VERR_IEM_INSTR_NOT_IMPLEMENTED))
            rcStrict = VERR_EM_INTERPRETER;
    }

    if (rcStrict == VINF_SUCCESS)
    {
        switch (iCrReg)
        {
            case 0:     /* CR0. */
                HMCPU_CF_SET(pVCpu, HM_CHANGED_GUEST_CR0);
                break;

            case 3:     /* CR3. */
                Assert(!pVCpu->CTX_SUFF(pVM)->hm.s.fNestedPaging);
                HMCPU_CF_SET(pVCpu, HM_CHANGED_GUEST_CR3);
                break;

            case 4:     /* CR4. */
                HMCPU_CF_SET(pVCpu, HM_CHANGED_GUEST_CR4);
                break;

            case 8:     /* CR8 (TPR). */
                HMCPU_CF_SET(pVCpu, HM_CHANGED_SVM_GUEST_APIC_STATE);
                break;

            default:
                AssertMsgFailed(("hmR0SvmExitWriteCRx: Invalid/Unexpected Write-CRx exit. u64ExitCode=%#RX64 %#x\n",
                                 pSvmTransient->u64ExitCode, iCrReg));
                break;
        }
        HMSVM_CHECK_SINGLE_STEP(pVCpu, rcStrict);
    }
    else
        Assert(rcStrict == VERR_EM_INTERPRETER || rcStrict == VINF_PGM_CHANGE_MODE || rcStrict == VINF_PGM_SYNC_CR3);
    return VBOXSTRICTRC_TODO(rcStrict);
}


/**
 * \#VMEXIT handler for MSR read and writes (SVM_EXIT_MSR). Conditional
 * \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitMsr(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
    PVM      pVM   = pVCpu->CTX_SUFF(pVM);

    int rc;
    if (pVmcb->ctrl.u64ExitInfo1 == SVM_EXIT1_MSR_WRITE)
    {
        STAM_COUNTER_INC(&pVCpu->hm.s.StatExitWrmsr);
        Log4(("MSR Write: idMsr=%#RX32\n", pCtx->ecx));

        unsigned cbInstr;
        if (hmR0SvmSupportsNextRipSave(pVCpu))
            cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
        else
        {
            PDISCPUSTATE pDis = &pVCpu->hm.s.DisState;
            rc = EMInterpretDisasCurrent(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, &cbInstr);
            if (   rc == VINF_SUCCESS
                && pDis->pCurInstr->uOpcode == OP_WRMSR)
                Assert(cbInstr > 0);
            else
                cbInstr = 0;
        }

        if (cbInstr)
        {
            /* Handle TPR patching; intercepted LSTAR write. */
            if (   pVM->hm.s.fTPRPatchingActive
                && pCtx->ecx == MSR_K8_LSTAR)
            {
                if ((pCtx->eax & 0xff) != pSvmTransient->u8GuestTpr)
                {
                    /* Our patch code uses LSTAR for TPR caching for 32-bit guests. */
                    int rc2 = APICSetTpr(pVCpu, pCtx->eax & 0xff);
                    AssertRC(rc2);
                    HMCPU_CF_SET(pVCpu, HM_CHANGED_SVM_GUEST_APIC_STATE);
                }
                rc = VINF_SUCCESS;
                hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);
                HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
                return rc;
            }

            /* Regular MSR write. */
            rc = EMInterpretWrmsr(pVM, pVCpu, CPUMCTX2CORE(pCtx));
            if (RT_LIKELY(rc == VINF_SUCCESS))
            {
                hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);

                /* If this is an X2APIC WRMSR access, update the APIC state as well. */
                if (   pCtx->ecx >= MSR_IA32_X2APIC_START
                    && pCtx->ecx <= MSR_IA32_X2APIC_END)
                {
                    /*
                     * We've already saved the APIC related guest-state (TPR) in hmR0SvmPostRunGuest(). When full APIC register
                     * virtualization is implemented we'll have to make sure APIC state is saved from the VMCB before
                     * EMInterpretWrmsr() changes it.
                     */
                    HMCPU_CF_SET(pVCpu, HM_CHANGED_SVM_GUEST_APIC_STATE);
                }
                else if (pCtx->ecx == MSR_K6_EFER)
                    HMCPU_CF_SET(pVCpu, HM_CHANGED_GUEST_EFER_MSR);
                else if (pCtx->ecx == MSR_IA32_TSC)
                    pSvmTransient->fUpdateTscOffsetting = true;
            }
            else
                AssertMsg(   rc == VERR_EM_INTERPRETER
                          || rc == VINF_CPUM_R3_MSR_WRITE, ("hmR0SvmExitMsr: EMInterpretWrmsr failed rc=%Rrc\n", rc));
        }
        else
            rc = VERR_EM_INTERPRETER;
    }
    else
    {
        /* MSR Read access. */
        STAM_COUNTER_INC(&pVCpu->hm.s.StatExitRdmsr);
        Assert(pVmcb->ctrl.u64ExitInfo1 == SVM_EXIT1_MSR_READ);
        Log4(("MSR Read: idMsr=%#RX32\n", pCtx->ecx));

        unsigned cbInstr;
        if (hmR0SvmSupportsNextRipSave(pVCpu))
            cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
        else
        {
            PDISCPUSTATE pDis = &pVCpu->hm.s.DisState;
            rc = EMInterpretDisasCurrent(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, &cbInstr);
            if (   rc == VINF_SUCCESS
                && pDis->pCurInstr->uOpcode == OP_RDMSR)
                Assert(cbInstr > 0);
            else
                cbInstr = 0;
        }

        if (cbInstr)
        {
            rc = EMInterpretRdmsr(pVM, pVCpu, CPUMCTX2CORE(pCtx));
            if (RT_LIKELY(rc == VINF_SUCCESS))
                hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);
            else
                AssertMsg(   rc == VERR_EM_INTERPRETER
                          || rc == VINF_CPUM_R3_MSR_READ, ("hmR0SvmExitMsr: EMInterpretRdmsr failed rc=%Rrc\n", rc));
        }
        else
            rc = VERR_EM_INTERPRETER;
    }

    HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
    return rc;
}


/**
 * \#VMEXIT handler for DRx read (SVM_EXIT_READ_DRx). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitReadDRx(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitDRxRead);

    /* We should -not- get this #VMEXIT if the guest's debug registers were active. */
    if (pSvmTransient->fWasGuestDebugStateActive)
    {
        AssertMsgFailed(("hmR0SvmExitReadDRx: Unexpected exit %#RX32\n", (uint32_t)pSvmTransient->u64ExitCode));
        pVCpu->hm.s.u32HMError = (uint32_t)pSvmTransient->u64ExitCode;
        return VERR_SVM_UNEXPECTED_EXIT;
    }

    /*
     * Lazy DR0-3 loading.
     */
    if (   !pSvmTransient->fWasHyperDebugStateActive
#ifdef VBOX_WITH_NESTED_HWVIRT
           && !CPUMIsGuestInSvmNestedHwVirtMode(pCtx) /** @todo implement single-stepping when executing a nested-guest. */
#endif
       )
    {
        Assert(!DBGFIsStepping(pVCpu)); Assert(!pVCpu->hm.s.fSingleInstruction);
        Log5(("hmR0SvmExitReadDRx: Lazy loading guest debug registers\n"));

        /* Don't intercept DRx read and writes. */
        PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        pVmcb->ctrl.u16InterceptRdDRx = 0;
        pVmcb->ctrl.u16InterceptWrDRx = 0;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_INTERCEPTS;

        /* We're playing with the host CPU state here, make sure we don't preempt or longjmp. */
        VMMRZCallRing3Disable(pVCpu);
        HM_DISABLE_PREEMPT();

        /* Save the host & load the guest debug state, restart execution of the MOV DRx instruction. */
        CPUMR0LoadGuestDebugState(pVCpu, false /* include DR6 */);
        Assert(CPUMIsGuestDebugStateActive(pVCpu) || HC_ARCH_BITS == 32);

        HM_RESTORE_PREEMPT();
        VMMRZCallRing3Enable(pVCpu);

        STAM_COUNTER_INC(&pVCpu->hm.s.StatDRxContextSwitch);
        return VINF_SUCCESS;
    }

    /*
     * Interpret the read/writing of DRx.
     */
    /** @todo Decode assist.  */
    VBOXSTRICTRC rc = EMInterpretInstruction(pVCpu, CPUMCTX2CORE(pCtx), 0 /* pvFault */);
    Log5(("hmR0SvmExitReadDRx: Emulated DRx access: rc=%Rrc\n", VBOXSTRICTRC_VAL(rc)));
    if (RT_LIKELY(rc == VINF_SUCCESS))
    {
        /* Not necessary for read accesses but whatever doesn't hurt for now, will be fixed with decode assist. */
        /** @todo CPUM should set this flag! */
        HMCPU_CF_SET(pVCpu, HM_CHANGED_GUEST_DEBUG);
        HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
    }
    else
        Assert(rc == VERR_EM_INTERPRETER);
    return VBOXSTRICTRC_TODO(rc);
}


/**
 * \#VMEXIT handler for DRx write (SVM_EXIT_WRITE_DRx). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitWriteDRx(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    /* For now it's the same since we interpret the instruction anyway. Will change when using of Decode Assist is implemented. */
    int rc = hmR0SvmExitReadDRx(pVCpu, pCtx, pSvmTransient);
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitDRxWrite);
    STAM_COUNTER_DEC(&pVCpu->hm.s.StatExitDRxRead);
    return rc;
}


/**
 * \#VMEXIT handler for XCRx write (SVM_EXIT_XSETBV). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitXsetbv(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /** @todo decode assists... */
    VBOXSTRICTRC rcStrict = IEMExecOne(pVCpu);
    if (rcStrict == VINF_IEM_RAISED_XCPT)
        HMCPU_CF_SET(pVCpu, HM_CHANGED_ALL_GUEST);

    pVCpu->hm.s.fLoadSaveGuestXcr0 = (pCtx->cr4 & X86_CR4_OSXSAVE) && pCtx->aXcr[0] != ASMGetXcr0();
    Log4(("hmR0SvmExitXsetbv: New XCR0=%#RX64 fLoadSaveGuestXcr0=%d (cr4=%RX64) rcStrict=%Rrc\n",
          pCtx->aXcr[0], pVCpu->hm.s.fLoadSaveGuestXcr0, pCtx->cr4, VBOXSTRICTRC_VAL(rcStrict)));

    HMSVM_CHECK_SINGLE_STEP(pVCpu, rcStrict);
    return VBOXSTRICTRC_TODO(rcStrict);
}


/**
 * \#VMEXIT handler for I/O instructions (SVM_EXIT_IOIO). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitIOInstr(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /* I/O operation lookup arrays. */
    static uint32_t const s_aIOSize[8]  = { 0, 1, 2, 0, 4, 0, 0, 0 };                   /* Size of the I/O accesses in bytes. */
    static uint32_t const s_aIOOpAnd[8] = { 0, 0xff, 0xffff, 0, 0xffffffff, 0, 0, 0 };  /* AND masks for saving
                                                                                            the result (in AL/AX/EAX). */
    Log4(("hmR0SvmExitIOInstr: CS:RIP=%04x:%#RX64\n", pCtx->cs.Sel, pCtx->rip));

    PVM      pVM   = pVCpu->CTX_SUFF(pVM);
    PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;

    /* Refer AMD spec. 15.10.2 "IN and OUT Behaviour" and Figure 15-2. "EXITINFO1 for IOIO Intercept" for the format. */
    SVMIOIOEXITINFO IoExitInfo;
    IoExitInfo.u       = (uint32_t)pVmcb->ctrl.u64ExitInfo1;
    uint32_t uIOWidth  = (IoExitInfo.u >> 4) & 0x7;
    uint32_t cbValue   = s_aIOSize[uIOWidth];
    uint32_t uAndVal   = s_aIOOpAnd[uIOWidth];

    if (RT_UNLIKELY(!cbValue))
    {
        AssertMsgFailed(("hmR0SvmExitIOInstr: Invalid IO operation. uIOWidth=%u\n", uIOWidth));
        return VERR_EM_INTERPRETER;
    }

    VBOXSTRICTRC rcStrict;
    bool fUpdateRipAlready = false;
    if (IoExitInfo.n.u1STR)
    {
#ifdef VBOX_WITH_2ND_IEM_STEP
        /* INS/OUTS - I/O String instruction. */
        /** @todo Huh? why can't we use the segment prefix information given by AMD-V
         *        in EXITINFO1? Investigate once this thing is up and running. */
        Log4(("CS:RIP=%04x:%08RX64 %#06x/%u %c str\n", pCtx->cs.Sel, pCtx->rip, IoExitInfo.n.u16Port, cbValue,
              IoExitInfo.n.u1Type == SVM_IOIO_WRITE ? 'w' : 'r'));
        AssertReturn(pCtx->dx == IoExitInfo.n.u16Port, VERR_SVM_IPE_2);
        static IEMMODE const s_aenmAddrMode[8] =
        {
            (IEMMODE)-1, IEMMODE_16BIT, IEMMODE_32BIT, (IEMMODE)-1, IEMMODE_64BIT, (IEMMODE)-1, (IEMMODE)-1, (IEMMODE)-1
        };
        IEMMODE enmAddrMode = s_aenmAddrMode[(IoExitInfo.u >> 7) & 0x7];
        if (enmAddrMode != (IEMMODE)-1)
        {
            uint64_t cbInstr = pVmcb->ctrl.u64ExitInfo2 - pCtx->rip;
            if (cbInstr <= 15 && cbInstr >= 1)
            {
                Assert(cbInstr >= 1U + IoExitInfo.n.u1REP);
                if (IoExitInfo.n.u1Type == SVM_IOIO_WRITE)
                {
                    /* Don't know exactly how to detect whether u3SEG is valid, currently
                       only enabling it for Bulldozer and later with NRIP.  OS/2 broke on
                       2384 Opterons when only checking NRIP. */
                    if (   hmR0SvmSupportsNextRipSave(pVCpu)
                        && pVM->cpum.ro.GuestFeatures.enmMicroarch >= kCpumMicroarch_AMD_15h_First)
                    {
                        AssertMsg(IoExitInfo.n.u3SEG == X86_SREG_DS || cbInstr > 1U + IoExitInfo.n.u1REP,
                                  ("u32Seg=%d cbInstr=%d u1REP=%d", IoExitInfo.n.u3SEG, cbInstr, IoExitInfo.n.u1REP));
                        rcStrict = IEMExecStringIoWrite(pVCpu, cbValue, enmAddrMode, IoExitInfo.n.u1REP, (uint8_t)cbInstr,
                                                        IoExitInfo.n.u3SEG, true /*fIoChecked*/);
                    }
                    else if (cbInstr == 1U + IoExitInfo.n.u1REP)
                        rcStrict = IEMExecStringIoWrite(pVCpu, cbValue, enmAddrMode, IoExitInfo.n.u1REP, (uint8_t)cbInstr,
                                                        X86_SREG_DS, true /*fIoChecked*/);
                    else
                        rcStrict = IEMExecOne(pVCpu);
                    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitIOStringWrite);
                }
                else
                {
                    AssertMsg(IoExitInfo.n.u3SEG == X86_SREG_ES /*=0*/, ("%#x\n", IoExitInfo.n.u3SEG));
                    rcStrict = IEMExecStringIoRead(pVCpu, cbValue, enmAddrMode, IoExitInfo.n.u1REP, (uint8_t)cbInstr,
                                                   true /*fIoChecked*/);
                    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitIOStringRead);
                }
            }
            else
            {
                AssertMsgFailed(("rip=%RX64 nrip=%#RX64 cbInstr=%#RX64\n", pCtx->rip, pVmcb->ctrl.u64ExitInfo2, cbInstr));
                rcStrict = IEMExecOne(pVCpu);
            }
        }
        else
        {
            AssertMsgFailed(("IoExitInfo=%RX64\n", IoExitInfo.u));
            rcStrict = IEMExecOne(pVCpu);
        }
        fUpdateRipAlready = true;

#else
        /* INS/OUTS - I/O String instruction. */
        PDISCPUSTATE pDis = &pVCpu->hm.s.DisState;

        /** @todo Huh? why can't we use the segment prefix information given by AMD-V
         *        in EXITINFO1? Investigate once this thing is up and running. */

        rcStrict = EMInterpretDisasCurrent(pVM, pVCpu, pDis, NULL);
        if (rcStrict == VINF_SUCCESS)
        {
            if (IoExitInfo.n.u1Type == SVM_IOIO_WRITE)
            {
                rcStrict = IOMInterpretOUTSEx(pVM, pVCpu, CPUMCTX2CORE(pCtx), IoExitInfo.n.u16Port, pDis->fPrefix,
                                              (DISCPUMODE)pDis->uAddrMode, cbValue);
                STAM_COUNTER_INC(&pVCpu->hm.s.StatExitIOStringWrite);
            }
            else
            {
                rcStrict = IOMInterpretINSEx(pVM, pVCpu, CPUMCTX2CORE(pCtx), IoExitInfo.n.u16Port, pDis->fPrefix,
                                             (DISCPUMODE)pDis->uAddrMode, cbValue);
                STAM_COUNTER_INC(&pVCpu->hm.s.StatExitIOStringRead);
            }
        }
        else
            rcStrict = VINF_EM_RAW_EMULATE_INSTR;
#endif
    }
    else
    {
        /* IN/OUT - I/O instruction. */
        Assert(!IoExitInfo.n.u1REP);

        if (IoExitInfo.n.u1Type == SVM_IOIO_WRITE)
        {
            rcStrict = IOMIOPortWrite(pVM, pVCpu, IoExitInfo.n.u16Port, pCtx->eax & uAndVal, cbValue);
            STAM_COUNTER_INC(&pVCpu->hm.s.StatExitIOWrite);
        }
        else
        {
            uint32_t u32Val = 0;
            rcStrict = IOMIOPortRead(pVM, pVCpu, IoExitInfo.n.u16Port, &u32Val, cbValue);
            if (IOM_SUCCESS(rcStrict))
            {
                /* Save result of I/O IN instr. in AL/AX/EAX. */
                /** @todo r=bird: 32-bit op size should clear high bits of rax! */
                pCtx->eax = (pCtx->eax & ~uAndVal) | (u32Val & uAndVal);
            }
            else if (rcStrict == VINF_IOM_R3_IOPORT_READ)
                HMR0SavePendingIOPortRead(pVCpu, pCtx->rip, pVmcb->ctrl.u64ExitInfo2, IoExitInfo.n.u16Port, uAndVal, cbValue);

            STAM_COUNTER_INC(&pVCpu->hm.s.StatExitIORead);
        }
    }

    if (IOM_SUCCESS(rcStrict))
    {
        /* AMD-V saves the RIP of the instruction following the IO instruction in EXITINFO2. */
        if (!fUpdateRipAlready)
        {
            pCtx->rip = pVmcb->ctrl.u64ExitInfo2;
            /** @todo should probably call update interrupt-shadow here, see
             *        hmR0SvmAdvanceRip. */
        }

        /*
         * If any I/O breakpoints are armed, we need to check if one triggered
         * and take appropriate action.
         * Note that the I/O breakpoint type is undefined if CR4.DE is 0.
         */
        /** @todo Optimize away the DBGFBpIsHwIoArmed call by having DBGF tell the
         *  execution engines about whether hyper BPs and such are pending. */
        uint32_t const uDr7 = pCtx->dr[7];
        if (RT_UNLIKELY(   (   (uDr7 & X86_DR7_ENABLED_MASK)
                            && X86_DR7_ANY_RW_IO(uDr7)
                            && (pCtx->cr4 & X86_CR4_DE))
                        || DBGFBpIsHwIoArmed(pVM)))
        {
            /* We're playing with the host CPU state here, make sure we don't preempt or longjmp. */
            VMMRZCallRing3Disable(pVCpu);
            HM_DISABLE_PREEMPT();

            STAM_COUNTER_INC(&pVCpu->hm.s.StatDRxIoCheck);
            CPUMR0DebugStateMaybeSaveGuest(pVCpu, false /*fDr6*/);

            VBOXSTRICTRC rcStrict2 = DBGFBpCheckIo(pVM, pVCpu, pCtx, IoExitInfo.n.u16Port, cbValue);
            if (rcStrict2 == VINF_EM_RAW_GUEST_TRAP)
            {
                /* Raise #DB. */
                pVmcb->guest.u64DR6 = pCtx->dr[6];
                pVmcb->guest.u64DR7 = pCtx->dr[7];
                pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_DRX;
                hmR0SvmSetPendingXcptDB(pVCpu);
            }
            /* rcStrict is VINF_SUCCESS, VINF_IOM_R3_IOPORT_COMMIT_WRITE, or in [VINF_EM_FIRST..VINF_EM_LAST],
               however we can ditch VINF_IOM_R3_IOPORT_COMMIT_WRITE as it has VMCPU_FF_IOM as backup. */
            else if (   rcStrict2 != VINF_SUCCESS
                     && (rcStrict == VINF_SUCCESS || rcStrict2 < rcStrict))
                rcStrict = rcStrict2;
            AssertCompile(VINF_EM_LAST < VINF_IOM_R3_IOPORT_COMMIT_WRITE);

            HM_RESTORE_PREEMPT();
            VMMRZCallRing3Enable(pVCpu);
        }

        HMSVM_CHECK_SINGLE_STEP(pVCpu, rcStrict);
    }

#ifdef VBOX_STRICT
    if (rcStrict == VINF_IOM_R3_IOPORT_READ)
        Assert(IoExitInfo.n.u1Type == SVM_IOIO_READ);
    else if (rcStrict == VINF_IOM_R3_IOPORT_WRITE || rcStrict == VINF_IOM_R3_IOPORT_COMMIT_WRITE)
        Assert(IoExitInfo.n.u1Type == SVM_IOIO_WRITE);
    else
    {
        /** @todo r=bird: This is missing a bunch of VINF_EM_FIRST..VINF_EM_LAST
         *        statuses, that the VMM device and some others may return. See
         *        IOM_SUCCESS() for guidance. */
        AssertMsg(   RT_FAILURE(rcStrict)
                  || rcStrict == VINF_SUCCESS
                  || rcStrict == VINF_EM_RAW_EMULATE_INSTR
                  || rcStrict == VINF_EM_DBG_BREAKPOINT
                  || rcStrict == VINF_EM_RAW_GUEST_TRAP
                  || rcStrict == VINF_EM_RAW_TO_R3
                  || rcStrict == VINF_TRPM_XCPT_DISPATCHED
                  || rcStrict == VINF_EM_TRIPLE_FAULT, ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)));
    }
#endif
    return VBOXSTRICTRC_TODO(rcStrict);
}


/**
 * \#VMEXIT handler for Nested Page-faults (SVM_EXIT_NPF). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitNestedPF(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    Assert(pVM->hm.s.fNestedPaging);

    HMSVM_CHECK_EXIT_DUE_TO_EVENT_DELIVERY();

    /* See AMD spec. 15.25.6 "Nested versus Guest Page Faults, Fault Ordering" for VMCB details for #NPF. */
    PSVMVMCB pVmcb           = pVCpu->hm.s.svm.pVmcb;
    uint32_t u32ErrCode      = pVmcb->ctrl.u64ExitInfo1;
    RTGCPHYS GCPhysFaultAddr = pVmcb->ctrl.u64ExitInfo2;

    Log4(("#NPF at CS:RIP=%04x:%#RX64 faultaddr=%RGp errcode=%#x \n", pCtx->cs.Sel, pCtx->rip, GCPhysFaultAddr, u32ErrCode));

#ifdef VBOX_HM_WITH_GUEST_PATCHING
    /* TPR patching for 32-bit guests, using the reserved bit in the page tables for MMIO regions.  */
    if (   pVM->hm.s.fTprPatchingAllowed
        && (GCPhysFaultAddr & PAGE_OFFSET_MASK) == XAPIC_OFF_TPR
        && (   !(u32ErrCode & X86_TRAP_PF_P)                                                             /* Not present */
            || (u32ErrCode & (X86_TRAP_PF_P | X86_TRAP_PF_RSVD)) == (X86_TRAP_PF_P | X86_TRAP_PF_RSVD))  /* MMIO page. */
        && !CPUMIsGuestInLongModeEx(pCtx)
        && !CPUMGetGuestCPL(pVCpu)
        && pVM->hm.s.cPatches < RT_ELEMENTS(pVM->hm.s.aPatches))
    {
        RTGCPHYS GCPhysApicBase = APICGetBaseMsrNoCheck(pVCpu);
        GCPhysApicBase &= PAGE_BASE_GC_MASK;

        if (GCPhysFaultAddr == GCPhysApicBase + XAPIC_OFF_TPR)
        {
            /* Only attempt to patch the instruction once. */
            PHMTPRPATCH pPatch = (PHMTPRPATCH)RTAvloU32Get(&pVM->hm.s.PatchTree, (AVLOU32KEY)pCtx->eip);
            if (!pPatch)
                return VINF_EM_HM_PATCH_TPR_INSTR;
        }
    }
#endif

    /*
     * Determine the nested paging mode.
     */
    PGMMODE enmNestedPagingMode;
#if HC_ARCH_BITS == 32
    if (CPUMIsGuestInLongModeEx(pCtx))
        enmNestedPagingMode = PGMMODE_AMD64_NX;
    else
#endif
        enmNestedPagingMode = PGMGetHostMode(pVM);

    /*
     * MMIO optimization using the reserved (RSVD) bit in the guest page tables for MMIO pages.
     */
    int rc;
    Assert((u32ErrCode & (X86_TRAP_PF_RSVD | X86_TRAP_PF_P)) != X86_TRAP_PF_RSVD);
    if ((u32ErrCode & (X86_TRAP_PF_RSVD | X86_TRAP_PF_P)) == (X86_TRAP_PF_RSVD | X86_TRAP_PF_P))
    {
        /* If event delivery causes an MMIO #NPF, go back to instruction emulation as
           otherwise injecting the original pending event would most likely cause the same MMIO #NPF. */
        if (pVCpu->hm.s.Event.fPending)
            return VINF_EM_RAW_INJECT_TRPM_EVENT;

        VBOXSTRICTRC rc2 = PGMR0Trap0eHandlerNPMisconfig(pVM, pVCpu, enmNestedPagingMode, CPUMCTX2CORE(pCtx), GCPhysFaultAddr,
                                                         u32ErrCode);
        rc = VBOXSTRICTRC_VAL(rc2);

        /*
         * If we succeed, resume guest execution.
         * If we fail in interpreting the instruction because we couldn't get the guest physical address
         * of the page containing the instruction via the guest's page tables (we would invalidate the guest page
         * in the host TLB), resume execution which would cause a guest page fault to let the guest handle this
         * weird case. See @bugref{6043}.
         */
        if (   rc == VINF_SUCCESS
            || rc == VERR_PAGE_TABLE_NOT_PRESENT
            || rc == VERR_PAGE_NOT_PRESENT)
        {
            /* Successfully handled MMIO operation. */
            HMCPU_CF_SET(pVCpu, HM_CHANGED_SVM_GUEST_APIC_STATE);
            rc = VINF_SUCCESS;
        }
        return rc;
    }

    TRPMAssertXcptPF(pVCpu, GCPhysFaultAddr, u32ErrCode);
    rc = PGMR0Trap0eHandlerNestedPaging(pVM, pVCpu, enmNestedPagingMode, u32ErrCode, CPUMCTX2CORE(pCtx), GCPhysFaultAddr);
    TRPMResetTrap(pVCpu);

    Log4(("#NPF: PGMR0Trap0eHandlerNestedPaging returned %Rrc CS:RIP=%04x:%#RX64\n", rc, pCtx->cs.Sel, pCtx->rip));

    /*
     * Same case as PGMR0Trap0eHandlerNPMisconfig(). See comment above, @bugref{6043}.
     */
    if (   rc == VINF_SUCCESS
        || rc == VERR_PAGE_TABLE_NOT_PRESENT
        || rc == VERR_PAGE_NOT_PRESENT)
    {
        /* We've successfully synced our shadow page tables. */
        STAM_COUNTER_INC(&pVCpu->hm.s.StatExitShadowPF);
        rc = VINF_SUCCESS;
    }

    return rc;
}


/**
 * \#VMEXIT handler for virtual interrupt (SVM_EXIT_VINTR). Conditional
 * \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitVIntr(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
    pVmcb->ctrl.IntCtrl.n.u1VIrqPending = 0;  /* No virtual interrupts pending, we'll inject the current one/NMI before reentry. */
    pVmcb->ctrl.IntCtrl.n.u8VIntrVector = 0;

    /* Indicate that we no longer need to #VMEXIT when the guest is ready to receive interrupts/NMIs, it is now ready. */
    pVmcb->ctrl.u64InterceptCtrl &= ~SVM_CTRL_INTERCEPT_VINTR;
    pVmcb->ctrl.u64VmcbCleanBits &= ~(HMSVM_VMCB_CLEAN_INTERCEPTS | HMSVM_VMCB_CLEAN_TPR);

    /* Deliver the pending interrupt/NMI via hmR0SvmEvaluatePendingEvent() and resume guest execution. */
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitIntWindow);
    return VINF_SUCCESS;
}


/**
 * \#VMEXIT handler for task switches (SVM_EXIT_TASK_SWITCH). Conditional
 * \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitTaskSwitch(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    HMSVM_CHECK_EXIT_DUE_TO_EVENT_DELIVERY();

#ifndef HMSVM_ALWAYS_TRAP_TASK_SWITCH
    Assert(!pVCpu->CTX_SUFF(pVM)->hm.s.fNestedPaging);
#endif

    /* Check if this task-switch occurred while delivering an event through the guest IDT. */
    if (pVCpu->hm.s.Event.fPending)  /* Can happen with exceptions/NMI. See @bugref{8411}. */
    {
        /*
         * AMD-V provides us with the exception which caused the TS; we collect
         * the information in the call to hmR0SvmCheckExitDueToEventDelivery.
         */
        Log4(("hmR0SvmExitTaskSwitch: TS occurred during event delivery.\n"));
        STAM_COUNTER_INC(&pVCpu->hm.s.StatExitTaskSwitch);
        return VINF_EM_RAW_INJECT_TRPM_EVENT;
    }

    /** @todo Emulate task switch someday, currently just going back to ring-3 for
     *        emulation. */
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitTaskSwitch);
    return VERR_EM_INTERPRETER;
}


/**
 * \#VMEXIT handler for VMMCALL (SVM_EXIT_VMMCALL). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitVmmCall(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitVmcall);

    int rc = VINF_SUCCESS;
    unsigned cbInstr;
    if (hmR0SvmSupportsNextRipSave(pVCpu))
    {
        PCSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
        cbInstr = pVmcb->ctrl.u64NextRIP - pCtx->rip;
        Assert(cbInstr > 0);
    }
    else
    {
        PDISCPUSTATE pDis = &pVCpu->hm.s.DisState;
        rc = EMInterpretDisasCurrent(pVCpu->CTX_SUFF(pVM), pVCpu, pDis, &cbInstr);
        if (   rc == VINF_SUCCESS
            && pDis->pCurInstr->uOpcode == OP_VMMCALL)
            Assert(cbInstr > 0);
        else
            cbInstr = 0;
    }

    if (cbInstr)
    {
        bool fRipUpdated;
        VBOXSTRICTRC rcStrict = HMSvmVmmcall(pVCpu, pCtx, &fRipUpdated);
        if (RT_SUCCESS(rcStrict))
        {
            /* Only update the RIP if we're continuing guest execution and not
               in the case of say VINF_GIM_R3_HYPERCALL. */
            if (   rcStrict == VINF_SUCCESS
                && !fRipUpdated)
                hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);

            /* If the hypercall or TPR patching changes anything other than guest's general-purpose registers,
               we would need to reload the guest changed bits here before VM-entry. */
            return VBOXSTRICTRC_VAL(rcStrict);
        }
    }
    else
        AssertMsgFailed(("Failed to disassemble VMMCALL rc=%Rrc\n", rc));

    hmR0SvmSetPendingXcptUD(pVCpu);
    return VINF_SUCCESS;
}


/**
 * \#VMEXIT handler for VMMCALL (SVM_EXIT_VMMCALL). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitPause(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitPause);
    return VINF_EM_RAW_INTERRUPT;
}


/**
 * \#VMEXIT handler for IRET (SVM_EXIT_IRET). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitIret(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /* Clear NMI blocking. */
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_BLOCK_NMIS);

    /* Indicate that we no longer need to #VMEXIT when the guest is ready to receive NMIs, it is now ready. */
    PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
    hmR0SvmClearIretIntercept(pVmcb);

    /* Deliver the pending NMI via hmR0SvmEvaluatePendingEvent() and resume guest execution. */
    return VINF_SUCCESS;
}


/**
 * \#VMEXIT handler for page-fault exceptions (SVM_EXIT_EXCEPTION_14).
 * Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitXcptPF(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    HMSVM_CHECK_EXIT_DUE_TO_EVENT_DELIVERY();

    /* See AMD spec. 15.12.15 "#PF (Page Fault)". */
    PSVMVMCB    pVmcb         = pVCpu->hm.s.svm.pVmcb;
    uint32_t    u32ErrCode    = pVmcb->ctrl.u64ExitInfo1;
    RTGCUINTPTR uFaultAddress = pVmcb->ctrl.u64ExitInfo2;
    PVM         pVM           = pVCpu->CTX_SUFF(pVM);

#if defined(HMSVM_ALWAYS_TRAP_ALL_XCPTS) || defined(HMSVM_ALWAYS_TRAP_PF)
    if (pVM->hm.s.fNestedPaging)
    {
        pVCpu->hm.s.Event.fPending = false;     /* In case it's a contributory or vectoring #PF. */
        if (!pSvmTransient->fVectoringDoublePF)
        {
            /* A genuine guest #PF, reflect it to the guest. */
            hmR0SvmSetPendingXcptPF(pVCpu, pCtx, u32ErrCode, uFaultAddress);
            Log4(("#PF: Guest page fault at %04X:%RGv FaultAddr=%RGv ErrCode=%#x\n", pCtx->cs.Sel, (RTGCPTR)pCtx->rip,
                  uFaultAddress, u32ErrCode));
        }
        else
        {
            /* A guest page-fault occurred during delivery of a page-fault. Inject #DF. */
            hmR0SvmSetPendingXcptDF(pVCpu);
            Log4(("Pending #DF due to vectoring #PF. NP\n"));
        }
        STAM_COUNTER_INC(&pVCpu->hm.s.StatExitGuestPF);
        return VINF_SUCCESS;
    }
#endif

    Assert(!pVM->hm.s.fNestedPaging);

#ifdef VBOX_HM_WITH_GUEST_PATCHING
    /* Shortcut for APIC TPR reads and writes; only applicable to 32-bit guests. */
    if (   pVM->hm.s.fTprPatchingAllowed
        && (uFaultAddress & 0xfff) == XAPIC_OFF_TPR
        && !(u32ErrCode & X86_TRAP_PF_P)              /* Not present. */
        && !CPUMIsGuestInLongModeEx(pCtx)
        && !CPUMGetGuestCPL(pVCpu)
        && pVM->hm.s.cPatches < RT_ELEMENTS(pVM->hm.s.aPatches))
    {
        RTGCPHYS GCPhysApicBase;
        GCPhysApicBase  = APICGetBaseMsrNoCheck(pVCpu);
        GCPhysApicBase &= PAGE_BASE_GC_MASK;

        /* Check if the page at the fault-address is the APIC base. */
        RTGCPHYS GCPhysPage;
        int rc2 = PGMGstGetPage(pVCpu, (RTGCPTR)uFaultAddress, NULL /* pfFlags */, &GCPhysPage);
        if (   rc2 == VINF_SUCCESS
            && GCPhysPage == GCPhysApicBase)
        {
            /* Only attempt to patch the instruction once. */
            PHMTPRPATCH pPatch = (PHMTPRPATCH)RTAvloU32Get(&pVM->hm.s.PatchTree, (AVLOU32KEY)pCtx->eip);
            if (!pPatch)
                return VINF_EM_HM_PATCH_TPR_INSTR;
        }
    }
#endif

    Log4(("#PF: uFaultAddress=%#RX64 CS:RIP=%#04x:%#RX64 u32ErrCode %#RX32 cr3=%#RX64\n", uFaultAddress, pCtx->cs.Sel,
          pCtx->rip, u32ErrCode, pCtx->cr3));

    /* If it's a vectoring #PF, emulate injecting the original event injection as PGMTrap0eHandler() is incapable
       of differentiating between instruction emulation and event injection that caused a #PF. See @bugref{6607}. */
    if (pSvmTransient->fVectoringPF)
    {
        Assert(pVCpu->hm.s.Event.fPending);
        return VINF_EM_RAW_INJECT_TRPM_EVENT;
    }

    TRPMAssertXcptPF(pVCpu, uFaultAddress, u32ErrCode);
    int rc = PGMTrap0eHandler(pVCpu, u32ErrCode, CPUMCTX2CORE(pCtx), (RTGCPTR)uFaultAddress);

    Log4(("#PF rc=%Rrc\n", rc));

    if (rc == VINF_SUCCESS)
    {
        /* Successfully synced shadow pages tables or emulated an MMIO instruction. */
        TRPMResetTrap(pVCpu);
        STAM_COUNTER_INC(&pVCpu->hm.s.StatExitShadowPF);
        HMCPU_CF_SET(pVCpu, HM_CHANGED_ALL_GUEST);
        return rc;
    }
    else if (rc == VINF_EM_RAW_GUEST_TRAP)
    {
        pVCpu->hm.s.Event.fPending = false;     /* In case it's a contributory or vectoring #PF. */

        if (!pSvmTransient->fVectoringDoublePF)
        {
            /* It's a guest page fault and needs to be reflected to the guest. */
            u32ErrCode = TRPMGetErrorCode(pVCpu);        /* The error code might have been changed. */
            TRPMResetTrap(pVCpu);
            hmR0SvmSetPendingXcptPF(pVCpu, pCtx, u32ErrCode, uFaultAddress);
        }
        else
        {
            /* A guest page-fault occurred during delivery of a page-fault. Inject #DF. */
            TRPMResetTrap(pVCpu);
            hmR0SvmSetPendingXcptDF(pVCpu);
            Log4(("#PF: Pending #DF due to vectoring #PF\n"));
        }

        STAM_COUNTER_INC(&pVCpu->hm.s.StatExitGuestPF);
        return VINF_SUCCESS;
    }

    TRPMResetTrap(pVCpu);
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitShadowPFEM);
    return rc;
}


/**
 * \#VMEXIT handler for undefined opcode (SVM_EXIT_EXCEPTION_6).
 * Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitXcptUD(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /* Paranoia; Ensure we cannot be called as a result of event delivery. */
    PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
    Assert(!pVmcb->ctrl.ExitIntInfo.n.u1Valid);  NOREF(pVmcb);

    int rc = VERR_SVM_UNEXPECTED_XCPT_EXIT;
    if (pVCpu->hm.s.fGIMTrapXcptUD)
    {
        uint8_t cbInstr = 0;
        VBOXSTRICTRC rcStrict = GIMXcptUD(pVCpu, pCtx, NULL /* pDis */, &cbInstr);
        if (rcStrict == VINF_SUCCESS)
        {
            /* #UD #VMEXIT does not have valid NRIP information, manually advance RIP. See @bugref{7270#c170}. */
            hmR0SvmAdvanceRip(pVCpu, pCtx, cbInstr);
            rc = VINF_SUCCESS;
            HMSVM_CHECK_SINGLE_STEP(pVCpu, rc);
        }
        else if (rcStrict == VINF_GIM_HYPERCALL_CONTINUING)
            rc = VINF_SUCCESS;
        else if (rcStrict == VINF_GIM_R3_HYPERCALL)
            rc = VINF_GIM_R3_HYPERCALL;
        else
            Assert(RT_FAILURE(VBOXSTRICTRC_VAL(rcStrict)));
    }

    /* If the GIM #UD exception handler didn't succeed for some reason or wasn't needed, raise #UD. */
    if (RT_FAILURE(rc))
    {
        hmR0SvmSetPendingXcptUD(pVCpu);
        rc = VINF_SUCCESS;
    }

    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitGuestUD);
    return rc;
}


/**
 * \#VMEXIT handler for math-fault exceptions (SVM_EXIT_EXCEPTION_16).
 * Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitXcptMF(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /* Paranoia; Ensure we cannot be called as a result of event delivery. */
    PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
    Assert(!pVmcb->ctrl.ExitIntInfo.n.u1Valid); NOREF(pVmcb);

    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitGuestMF);

    if (!(pCtx->cr0 & X86_CR0_NE))
    {
        PVM       pVM  = pVCpu->CTX_SUFF(pVM);
        PDISSTATE pDis = &pVCpu->hm.s.DisState;
        unsigned  cbOp;
        int rc = EMInterpretDisasCurrent(pVM, pVCpu, pDis, &cbOp);
        if (RT_SUCCESS(rc))
        {
            /* Convert a #MF into a FERR -> IRQ 13. See @bugref{6117}. */
            rc = PDMIsaSetIrq(pVCpu->CTX_SUFF(pVM), 13, 1, 0 /* uTagSrc */);
            if (RT_SUCCESS(rc))
                hmR0SvmAdvanceRip(pVCpu, pCtx, cbOp);
        }
        else
            Log4(("hmR0SvmExitXcptMF: EMInterpretDisasCurrent returned %Rrc uOpCode=%#x\n", rc, pDis->pCurInstr->uOpcode));
        return rc;
    }

    hmR0SvmSetPendingXcptMF(pVCpu);
    return VINF_SUCCESS;
}


/**
 * \#VMEXIT handler for debug exceptions (SVM_EXIT_EXCEPTION_1). Conditional
 * \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitXcptDB(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /* If this #DB is the result of delivering an event, go back to the interpreter. */
    HMSVM_CHECK_EXIT_DUE_TO_EVENT_DELIVERY();
    if (RT_UNLIKELY(pVCpu->hm.s.Event.fPending))
    {
        STAM_COUNTER_INC(&pVCpu->hm.s.StatInjectPendingInterpret);
        return VINF_EM_RAW_INJECT_TRPM_EVENT;
    }

    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitGuestDB);

    /* This can be a fault-type #DB (instruction breakpoint) or a trap-type #DB (data breakpoint). However, for both cases
       DR6 and DR7 are updated to what the exception handler expects. See AMD spec. 15.12.2 "#DB (Debug)". */
    PVM      pVM   = pVCpu->CTX_SUFF(pVM);
    PSVMVMCB pVmcb = pVCpu->hm.s.svm.pVmcb;
    int rc = DBGFRZTrap01Handler(pVM, pVCpu, CPUMCTX2CORE(pCtx), pVmcb->guest.u64DR6, pVCpu->hm.s.fSingleInstruction);
    if (rc == VINF_EM_RAW_GUEST_TRAP)
    {
        Log5(("hmR0SvmExitXcptDB: DR6=%#RX64 -> guest trap\n", pVmcb->guest.u64DR6));
        if (CPUMIsHyperDebugStateActive(pVCpu))
            CPUMSetGuestDR6(pVCpu, CPUMGetGuestDR6(pVCpu) | pVmcb->guest.u64DR6);

        /* Reflect the exception back to the guest. */
        hmR0SvmSetPendingXcptDB(pVCpu);
        rc = VINF_SUCCESS;
    }

    /*
     * Update DR6.
     */
    if (CPUMIsHyperDebugStateActive(pVCpu))
    {
        Log5(("hmR0SvmExitXcptDB: DR6=%#RX64 -> %Rrc\n", pVmcb->guest.u64DR6, rc));
        pVmcb->guest.u64DR6 = X86_DR6_INIT_VAL;
        pVmcb->ctrl.u64VmcbCleanBits &= ~HMSVM_VMCB_CLEAN_DRX;
    }
    else
    {
        AssertMsg(rc == VINF_SUCCESS, ("rc=%Rrc\n", rc));
        Assert(!pVCpu->hm.s.fSingleInstruction && !DBGFIsStepping(pVCpu));
    }

    return rc;
}


/**
 * \#VMEXIT handler for alignment check exceptions (SVM_EXIT_EXCEPTION_17).
 * Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitXcptAC(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /** @todo if triple-fault is returned in nested-guest scenario convert to a
     *        shutdown VMEXIT. */
    HMSVM_CHECK_EXIT_DUE_TO_EVENT_DELIVERY();

    SVMEVENT Event;
    Event.u          = 0;
    Event.n.u1Valid  = 1;
    Event.n.u3Type   = SVM_EVENT_EXCEPTION;
    Event.n.u8Vector = X86_XCPT_AC;
    Event.n.u1ErrorCodeValid = 1;
    hmR0SvmSetPendingEvent(pVCpu, &Event, 0 /* GCPtrFaultAddress */);
    return VINF_SUCCESS;
}


/**
 * \#VMEXIT handler for breakpoint exceptions (SVM_EXIT_EXCEPTION_3).
 * Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitXcptBP(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    HMSVM_CHECK_EXIT_DUE_TO_EVENT_DELIVERY();

    int rc = DBGFRZTrap03Handler(pVCpu->CTX_SUFF(pVM), pVCpu, CPUMCTX2CORE(pCtx));
    if (rc == VINF_EM_RAW_GUEST_TRAP)
    {
        SVMEVENT Event;
        Event.u          = 0;
        Event.n.u1Valid  = 1;
        Event.n.u3Type   = SVM_EVENT_EXCEPTION;
        Event.n.u8Vector = X86_XCPT_BP;
        hmR0SvmSetPendingEvent(pVCpu, &Event, 0 /* GCPtrFaultAddress */);
    }

    Assert(rc == VINF_SUCCESS || rc == VINF_EM_RAW_GUEST_TRAP || rc == VINF_EM_DBG_BREAKPOINT);
    return rc;
}


#ifdef VBOX_WITH_NESTED_HWVIRT
/**
 * \#VMEXIT handler for #PF occuring while in nested-guest execution
 * (SVM_EXIT_EXCEPTION_14). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitXcptPFNested(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    HMSVM_CHECK_EXIT_DUE_TO_EVENT_DELIVERY();

    /* See AMD spec. 15.12.15 "#PF (Page Fault)". */
    PSVMVMCB       pVmcb         = pVCpu->hm.s.svm.pVmcb;
    uint32_t       u32ErrCode    = pVmcb->ctrl.u64ExitInfo1;
    uint64_t const uFaultAddress = pVmcb->ctrl.u64ExitInfo2;

    Log4(("#PFNested: uFaultAddress=%#RX64 CS:RIP=%#04x:%#RX64 u32ErrCode=%#RX32 CR3=%#RX64\n", uFaultAddress, pCtx->cs.Sel,
          pCtx->rip, u32ErrCode, pCtx->cr3));

    /* If it's a vectoring #PF, emulate injecting the original event injection as PGMTrap0eHandler() is incapable
       of differentiating between instruction emulation and event injection that caused a #PF. See @bugref{6607}. */
    if (pSvmTransient->fVectoringPF)
    {
        Assert(pVCpu->hm.s.Event.fPending);
        return VINF_EM_RAW_INJECT_TRPM_EVENT;
    }

    Assert(!pVCpu->CTX_SUFF(pVM)->hm.s.fNestedPaging);

    TRPMAssertXcptPF(pVCpu, uFaultAddress, u32ErrCode);
    int rc = PGMTrap0eHandler(pVCpu, u32ErrCode, CPUMCTX2CORE(pCtx), (RTGCPTR)uFaultAddress);

    Log4(("#PFNested: rc=%Rrc\n", rc));

    if (rc == VINF_SUCCESS)
    {
        /* Successfully synced shadow pages tables or emulated an MMIO instruction. */
        TRPMResetTrap(pVCpu);
        STAM_COUNTER_INC(&pVCpu->hm.s.StatExitShadowPF);
        HMCPU_CF_SET(pVCpu, HM_CHANGED_ALL_GUEST);
        return rc;
    }

    if (rc == VINF_EM_RAW_GUEST_TRAP)
    {
        pVCpu->hm.s.Event.fPending = false;     /* In case it's a contributory or vectoring #PF. */

        if (!pSvmTransient->fVectoringDoublePF)
        {
            /* It's a nested-guest page fault and needs to be reflected to the nested-guest. */
            u32ErrCode = TRPMGetErrorCode(pVCpu);        /* The error code might have been changed. */
            TRPMResetTrap(pVCpu);
            hmR0SvmSetPendingXcptPF(pVCpu, pCtx, u32ErrCode, uFaultAddress);
        }
        else
        {
            /* A nested-guest page-fault occurred during delivery of a page-fault. Inject #DF. */
            TRPMResetTrap(pVCpu);
            hmR0SvmSetPendingXcptDF(pVCpu);
            Log4(("#PF: Pending #DF due to vectoring #PF\n"));
        }

        STAM_COUNTER_INC(&pVCpu->hm.s.StatExitGuestPF);
        return VINF_SUCCESS;
    }

    TRPMResetTrap(pVCpu);
    STAM_COUNTER_INC(&pVCpu->hm.s.StatExitShadowPFEM);
    return rc;
}


/**
 * \#VMEXIT handler for CLGI (SVM_EXIT_CLGI). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitClgi(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /** @todo Stat. */
    /* STAM_COUNTER_INC(&pVCpu->hm.s.StatExitClgi); */
    uint8_t const cbInstr = hmR0SvmGetInstrLengthHwAssist(pVCpu, pCtx, 3);
    VBOXSTRICTRC rcStrict = IEMExecDecodedClgi(pVCpu, cbInstr);
    return VBOXSTRICTRC_VAL(rcStrict);
}


/**
 * \#VMEXIT handler for STGI (SVM_EXIT_STGI). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitStgi(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /** @todo Stat. */
    /* STAM_COUNTER_INC(&pVCpu->hm.s.StatExitStgi); */
    uint8_t const cbInstr = hmR0SvmGetInstrLengthHwAssist(pVCpu, pCtx, 3);
    VBOXSTRICTRC rcStrict = IEMExecDecodedStgi(pVCpu, cbInstr);
    return VBOXSTRICTRC_VAL(rcStrict);
}


/**
 * \#VMEXIT handler for VMLOAD (SVM_EXIT_VMLOAD). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitVmload(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /** @todo Stat. */
    /* STAM_COUNTER_INC(&pVCpu->hm.s.StatExitVmload); */
    uint8_t const cbInstr = hmR0SvmGetInstrLengthHwAssist(pVCpu, pCtx, 3);
    VBOXSTRICTRC rcStrict = IEMExecDecodedVmload(pVCpu, cbInstr);
    if (rcStrict == VINF_SUCCESS)
    {
        /* We skip flagging changes made to LSTAR, STAR, SFMASK and other MSRs as they are always re-loaded. */
        HMCPU_CF_SET(pVCpu,   HM_CHANGED_GUEST_SEGMENT_REGS
                            | HM_CHANGED_GUEST_TR
                            | HM_CHANGED_GUEST_LDTR);
    }
    return VBOXSTRICTRC_VAL(rcStrict);
}


/**
 * \#VMEXIT handler for VMSAVE (SVM_EXIT_VMSAVE). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitVmsave(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /** @todo Stat. */
    /* STAM_COUNTER_INC(&pVCpu->hm.s.StatExitVmsave); */
    uint8_t const cbInstr = hmR0SvmGetInstrLengthHwAssist(pVCpu, pCtx, 3);
    VBOXSTRICTRC rcStrict = IEMExecDecodedVmsave(pVCpu, cbInstr);
    return VBOXSTRICTRC_VAL(rcStrict);
}


/**
 * \#VMEXIT handler for INVLPGA (SVM_EXIT_INVLPGA). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitInvlpga(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    /** @todo Stat. */
    /* STAM_COUNTER_INC(&pVCpu->hm.s.StatExitInvlpga); */
    uint8_t const cbInstr = hmR0SvmGetInstrLengthHwAssist(pVCpu, pCtx, 3);
    VBOXSTRICTRC rcStrict = IEMExecDecodedInvlpga(pVCpu, cbInstr);
    return VBOXSTRICTRC_VAL(rcStrict);
}


/**
 * \#VMEXIT handler for STGI (SVM_EXIT_VMRUN). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmExitVmrun(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();
    /** @todo Stat. */
    /* STAM_COUNTER_INC(&pVCpu->hm.s.StatExitVmrun); */
#if 0
    VBOXSTRICTRC rcStrict;
    uint8_t const cbInstr = hmR0SvmGetInstrLengthHwAssist(pVCpu, pCtx, 3);
    rcStrict = IEMExecDecodedVmrun(pVCpu, cbInstr);
    Log4(("IEMExecDecodedVmrun: returned %d\n", VBOXSTRICTRC_VAL(rcStrict)));
    if (rcStrict == VINF_SUCCESS)
    {
        rcStrict = VINF_SVM_VMRUN;
        HMCPU_CF_SET(pVCpu, HM_CHANGED_ALL_GUEST);
    }
    return VBOXSTRICTRC_VAL(rcStrict);
#endif
    return VERR_EM_INTERPRETER;
}


/**
 * Nested-guest \#VMEXIT handler for IRET (SVM_EXIT_VMRUN). Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmNestedExitIret(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /* Clear NMI blocking. */
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_BLOCK_NMIS);

    /* Indicate that we no longer need to #VMEXIT when the guest is ready to receive NMIs, it is now ready. */
    PSVMVMCB pVmcbNstGst = pCtx->hwvirt.svm.CTX_SUFF(pVmcb);
    hmR0SvmClearIretIntercept(pVmcbNstGst);

    /* Deliver the pending NMI via hmR0SvmEvaluatePendingEventNested() and resume guest execution. */
    return VINF_SUCCESS;
}


/**
 * Nested-guest \#VMEXIT handler for debug exceptions (SVM_EXIT_EXCEPTION_1).
 * Unconditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmNestedExitXcptDB(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /* If this #DB is the result of delivering an event, go back to the interpreter. */
    /** @todo if triple-fault is returned in nested-guest scenario convert to a
     *        shutdown VMEXIT. */
    HMSVM_CHECK_EXIT_DUE_TO_EVENT_DELIVERY();
    if (RT_UNLIKELY(pVCpu->hm.s.Event.fPending))
    {
        STAM_COUNTER_INC(&pVCpu->hm.s.StatInjectPendingInterpret);
        return VINF_EM_RAW_INJECT_TRPM_EVENT;
    }

    hmR0SvmSetPendingXcptDB(pVCpu);
    return VINF_SUCCESS;
}


/**
 * Nested-guest \#VMEXIT handler for breakpoint exceptions (SVM_EXIT_EXCEPTION_3).
 * Conditional \#VMEXIT.
 */
HMSVM_EXIT_DECL hmR0SvmNestedExitXcptBP(PVMCPU pVCpu, PCPUMCTX pCtx, PSVMTRANSIENT pSvmTransient)
{
    HMSVM_VALIDATE_EXIT_HANDLER_PARAMS();

    /** @todo if triple-fault is returned in nested-guest scenario convert to a
     *        shutdown VMEXIT. */
    HMSVM_CHECK_EXIT_DUE_TO_EVENT_DELIVERY();

    SVMEVENT Event;
    Event.u          = 0;
    Event.n.u1Valid  = 1;
    Event.n.u3Type   = SVM_EVENT_EXCEPTION;
    Event.n.u8Vector = X86_XCPT_BP;
    hmR0SvmSetPendingEvent(pVCpu, &Event, 0 /* GCPtrFaultAddress */);
    return VINF_SUCCESS;
}

#endif /* VBOX_WITH_NESTED_HWVIRT */


/** @} */

