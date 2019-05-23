/* $Id: VMM.cpp $ */
/** @file
 * VMM - The Virtual Machine Monitor Core.
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

//#define NO_SUPCALLR0VMM

/** @page pg_vmm        VMM - The Virtual Machine Monitor
 *
 * The VMM component is two things at the moment, it's a component doing a few
 * management and routing tasks, and it's the whole virtual machine monitor
 * thing.  For hysterical reasons, it is not doing all the management that one
 * would expect, this is instead done by @ref pg_vm.  We'll address this
 * misdesign eventually, maybe.
 *
 * VMM is made up of these components:
 *  - @subpage pg_cfgm
 *  - @subpage pg_cpum
 *  - @subpage pg_csam
 *  - @subpage pg_dbgf
 *  - @subpage pg_em
 *  - @subpage pg_gim
 *  - @subpage pg_gmm
 *  - @subpage pg_gvmm
 *  - @subpage pg_hm
 *  - @subpage pg_iem
 *  - @subpage pg_iom
 *  - @subpage pg_mm
 *  - @subpage pg_patm
 *  - @subpage pg_pdm
 *  - @subpage pg_pgm
 *  - @subpage pg_rem
 *  - @subpage pg_selm
 *  - @subpage pg_ssm
 *  - @subpage pg_stam
 *  - @subpage pg_tm
 *  - @subpage pg_trpm
 *  - @subpage pg_vm
 *
 *
 * @see @ref grp_vmm @ref grp_vm @subpage pg_vmm_guideline @subpage pg_raw
 *
 *
 * @section sec_vmmstate        VMM State
 *
 * @image html VM_Statechart_Diagram.gif
 *
 * To be written.
 *
 *
 * @subsection  subsec_vmm_init     VMM Initialization
 *
 * To be written.
 *
 *
 * @subsection  subsec_vmm_term     VMM Termination
 *
 * To be written.
 *
 *
 * @section sec_vmm_limits     VMM Limits
 *
 * There are various resource limits imposed by the VMM and it's
 * sub-components.  We'll list some of them here.
 *
 * On 64-bit hosts:
 *      - Max 8191 VMs.  Imposed by GVMM's handle allocation (GVMM_MAX_HANDLES),
 *        can be increased up to 64K - 1.
 *      - Max 16TB - 64KB of the host memory can be used for backing VM RAM and
 *        ROM pages.  The limit is imposed by the 32-bit page ID used by GMM.
 *      - A VM can be assigned all the memory we can use (16TB), however, the
 *        Main API will restrict this to 2TB (MM_RAM_MAX_IN_MB).
 *      - Max 32 virtual CPUs (VMM_MAX_CPU_COUNT).
 *
 * On 32-bit hosts:
 *      - Max 127 VMs.  Imposed by GMM's per page structure.
 *      - Max 64GB - 64KB of the host memory can be used for backing VM RAM and
 *        ROM pages.  The limit is imposed by the 28-bit page ID used
 *        internally in GMM.  It is also limited by PAE.
 *      - A VM can be assigned all the memory GMM can allocate, however, the
 *        Main API will restrict this to 3584MB (MM_RAM_MAX_IN_MB).
 *      - Max 32 virtual CPUs (VMM_MAX_CPU_COUNT).
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_VMM
#include <VBox/vmm/vmm.h>
#include <VBox/vmm/vmapi.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/cfgm.h>
#include <VBox/vmm/pdmqueue.h>
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/vmm/pdmcritsectrw.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/gim.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/iom.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/em.h>
#include <VBox/sup.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/csam.h>
#include <VBox/vmm/patm.h>
#include <VBox/vmm/apic.h>
#ifdef VBOX_WITH_REM
# include <VBox/vmm/rem.h>
#endif
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/ftm.h>
#include <VBox/vmm/tm.h>
#include "VMMInternal.h"
#include "VMMSwitcher.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>

#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/version.h>
#include <VBox/vmm/hm.h>
#include <iprt/assert.h>
#include <iprt/alloc.h>
#include <iprt/asm.h>
#include <iprt/time.h>
#include <iprt/semaphore.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/stdarg.h>
#include <iprt/ctype.h>
#include <iprt/x86.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The saved state version. */
#define VMM_SAVED_STATE_VERSION     4
/** The saved state version used by v3.0 and earlier. (Teleportation) */
#define VMM_SAVED_STATE_VERSION_3_0 3


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int                  vmmR3InitStacks(PVM pVM);
static int                  vmmR3InitLoggers(PVM pVM);
static void                 vmmR3InitRegisterStats(PVM pVM);
static DECLCALLBACK(int)    vmmR3Save(PVM pVM, PSSMHANDLE pSSM);
static DECLCALLBACK(int)    vmmR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass);
static DECLCALLBACK(void)   vmmR3YieldEMT(PVM pVM, PTMTIMER pTimer, void *pvUser);
static VBOXSTRICTRC         vmmR3EmtRendezvousCommon(PVM pVM, PVMCPU pVCpu, bool fIsCaller,
                                                     uint32_t fFlags, PFNVMMEMTRENDEZVOUS pfnRendezvous, void *pvUser);
static int                  vmmR3ServiceCallRing3Request(PVM pVM, PVMCPU pVCpu);
static DECLCALLBACK(void)   vmmR3InfoFF(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs);


/**
 * Initializes the VMM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) VMMR3Init(PVM pVM)
{
    LogFlow(("VMMR3Init\n"));

    /*
     * Assert alignment, sizes and order.
     */
    AssertMsg(pVM->vmm.s.offVM == 0, ("Already initialized!\n"));
    AssertCompile(sizeof(pVM->vmm.s) <= sizeof(pVM->vmm.padding));
    AssertCompile(sizeof(pVM->aCpus[0].vmm.s) <= sizeof(pVM->aCpus[0].vmm.padding));

    /*
     * Init basic VM VMM members.
     */
    pVM->vmm.s.offVM = RT_OFFSETOF(VM, vmm);
    pVM->vmm.s.pahEvtRendezvousEnterOrdered     = NULL;
    pVM->vmm.s.hEvtRendezvousEnterOneByOne      = NIL_RTSEMEVENT;
    pVM->vmm.s.hEvtMulRendezvousEnterAllAtOnce  = NIL_RTSEMEVENTMULTI;
    pVM->vmm.s.hEvtMulRendezvousDone            = NIL_RTSEMEVENTMULTI;
    pVM->vmm.s.hEvtRendezvousDoneCaller         = NIL_RTSEMEVENT;
    pVM->vmm.s.hEvtMulRendezvousRecursionPush   = NIL_RTSEMEVENTMULTI;
    pVM->vmm.s.hEvtMulRendezvousRecursionPop    = NIL_RTSEMEVENTMULTI;
    pVM->vmm.s.hEvtRendezvousRecursionPushCaller = NIL_RTSEMEVENT;
    pVM->vmm.s.hEvtRendezvousRecursionPopCaller = NIL_RTSEMEVENT;

    /** @cfgm{/YieldEMTInterval, uint32_t, 1, UINT32_MAX, 23, ms}
     * The EMT yield interval.  The EMT yielding is a hack we employ to play a
     * bit nicer with the rest of the system (like for instance the GUI).
     */
    int rc = CFGMR3QueryU32Def(CFGMR3GetRoot(pVM), "YieldEMTInterval", &pVM->vmm.s.cYieldEveryMillies,
                               23 /* Value arrived at after experimenting with the grub boot prompt. */);
    AssertMsgRCReturn(rc, ("Configuration error. Failed to query \"YieldEMTInterval\", rc=%Rrc\n", rc), rc);


    /** @cfgm{/VMM/UsePeriodicPreemptionTimers, boolean, true}
     * Controls whether we employ per-cpu preemption timers to limit the time
     * spent executing guest code.  This option is not available on all
     * platforms and we will silently ignore this setting then.  If we are
     * running in VT-x mode, we will use the VMX-preemption timer instead of
     * this one when possible.
     */
    PCFGMNODE pCfgVMM = CFGMR3GetChild(CFGMR3GetRoot(pVM), "VMM");
    rc = CFGMR3QueryBoolDef(pCfgVMM, "UsePeriodicPreemptionTimers", &pVM->vmm.s.fUsePeriodicPreemptionTimers, true);
    AssertMsgRCReturn(rc, ("Configuration error. Failed to query \"VMM/UsePeriodicPreemptionTimers\", rc=%Rrc\n", rc), rc);

    /*
     * Initialize the VMM rendezvous semaphores.
     */
    pVM->vmm.s.pahEvtRendezvousEnterOrdered = (PRTSEMEVENT)MMR3HeapAlloc(pVM, MM_TAG_VMM, sizeof(RTSEMEVENT) * pVM->cCpus);
    if (!pVM->vmm.s.pahEvtRendezvousEnterOrdered)
        return VERR_NO_MEMORY;
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
        pVM->vmm.s.pahEvtRendezvousEnterOrdered[i] = NIL_RTSEMEVENT;
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        rc = RTSemEventCreate(&pVM->vmm.s.pahEvtRendezvousEnterOrdered[i]);
        AssertRCReturn(rc, rc);
    }
    rc = RTSemEventCreate(&pVM->vmm.s.hEvtRendezvousEnterOneByOne);
    AssertRCReturn(rc, rc);
    rc = RTSemEventMultiCreate(&pVM->vmm.s.hEvtMulRendezvousEnterAllAtOnce);
    AssertRCReturn(rc, rc);
    rc = RTSemEventMultiCreate(&pVM->vmm.s.hEvtMulRendezvousDone);
    AssertRCReturn(rc, rc);
    rc = RTSemEventCreate(&pVM->vmm.s.hEvtRendezvousDoneCaller);
    AssertRCReturn(rc, rc);
    rc = RTSemEventMultiCreate(&pVM->vmm.s.hEvtMulRendezvousRecursionPush);
    AssertRCReturn(rc, rc);
    rc = RTSemEventMultiCreate(&pVM->vmm.s.hEvtMulRendezvousRecursionPop);
    AssertRCReturn(rc, rc);
    rc = RTSemEventCreate(&pVM->vmm.s.hEvtRendezvousRecursionPushCaller);
    AssertRCReturn(rc, rc);
    rc = RTSemEventCreate(&pVM->vmm.s.hEvtRendezvousRecursionPopCaller);
    AssertRCReturn(rc, rc);

    /*
     * Register the saved state data unit.
     */
    rc = SSMR3RegisterInternal(pVM, "vmm", 1, VMM_SAVED_STATE_VERSION, VMM_STACK_SIZE + sizeof(RTGCPTR),
                               NULL, NULL, NULL,
                               NULL, vmmR3Save, NULL,
                               NULL, vmmR3Load, NULL);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Register the Ring-0 VM handle with the session for fast ioctl calls.
     */
    rc = SUPR3SetVMForFastIOCtl(pVM->pVMR0);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Init various sub-components.
     */
    rc = vmmR3SwitcherInit(pVM);
    if (RT_SUCCESS(rc))
    {
        rc = vmmR3InitStacks(pVM);
        if (RT_SUCCESS(rc))
        {
            rc = vmmR3InitLoggers(pVM);

#ifdef VBOX_WITH_NMI
            /*
             * Allocate mapping for the host APIC.
             */
            if (RT_SUCCESS(rc))
            {
                rc = MMR3HyperReserve(pVM, PAGE_SIZE, "Host APIC", &pVM->vmm.s.GCPtrApicBase);
                AssertRC(rc);
            }
#endif
            if (RT_SUCCESS(rc))
            {
                /*
                 * Debug info and statistics.
                 */
                DBGFR3InfoRegisterInternal(pVM, "fflags", "Displays the current Forced actions Flags.", vmmR3InfoFF);
                vmmR3InitRegisterStats(pVM);
                vmmInitFormatTypes();

                return VINF_SUCCESS;
            }
        }
        /** @todo Need failure cleanup. */

        //more todo in here?
        //if (RT_SUCCESS(rc))
        //{
        //}
        //int rc2 = vmmR3TermCoreCode(pVM);
        //AssertRC(rc2));
    }

    return rc;
}


/**
 * Allocate & setup the VMM RC stack(s) (for EMTs).
 *
 * The stacks are also used for long jumps in Ring-0.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 *
 * @remarks The optional guard page gets it protection setup up during R3 init
 *          completion because of init order issues.
 */
static int vmmR3InitStacks(PVM pVM)
{
    int rc = VINF_SUCCESS;
#ifdef VMM_R0_SWITCH_STACK
    uint32_t fFlags = MMHYPER_AONR_FLAGS_KERNEL_MAPPING;
#else
    uint32_t fFlags = 0;
#endif

    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = &pVM->aCpus[idCpu];

#ifdef VBOX_STRICT_VMM_STACK
        rc = MMR3HyperAllocOnceNoRelEx(pVM, PAGE_SIZE + VMM_STACK_SIZE + PAGE_SIZE,
#else
        rc = MMR3HyperAllocOnceNoRelEx(pVM, VMM_STACK_SIZE,
#endif
                                       PAGE_SIZE, MM_TAG_VMM, fFlags, (void **)&pVCpu->vmm.s.pbEMTStackR3);
        if (RT_SUCCESS(rc))
        {
#ifdef VBOX_STRICT_VMM_STACK
            pVCpu->vmm.s.pbEMTStackR3 += PAGE_SIZE;
#endif
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
            /* MMHyperR3ToR0 returns R3 when not doing hardware assisted virtualization. */
            if (!HMIsEnabled(pVM))
                pVCpu->vmm.s.CallRing3JmpBufR0.pvSavedStack = NIL_RTR0PTR;
            else
#endif
                pVCpu->vmm.s.CallRing3JmpBufR0.pvSavedStack = MMHyperR3ToR0(pVM, pVCpu->vmm.s.pbEMTStackR3);
            pVCpu->vmm.s.pbEMTStackRC       = MMHyperR3ToRC(pVM, pVCpu->vmm.s.pbEMTStackR3);
            pVCpu->vmm.s.pbEMTStackBottomRC = pVCpu->vmm.s.pbEMTStackRC + VMM_STACK_SIZE;
            AssertRelease(pVCpu->vmm.s.pbEMTStackRC);

            CPUMSetHyperESP(pVCpu, pVCpu->vmm.s.pbEMTStackBottomRC);
        }
    }

    return rc;
}


/**
 * Initialize the loggers.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
static int vmmR3InitLoggers(PVM pVM)
{
    int rc;
#define RTLogCalcSizeForR0(cGroups, fFlags) (RT_OFFSETOF(VMMR0LOGGER, Logger.afGroups[cGroups]) + PAGE_SIZE)

    /*
     * Allocate RC & R0 Logger instances (they are finalized in the relocator).
     */
#ifdef LOG_ENABLED
    PRTLOGGER pLogger = RTLogDefaultInstance();
    if (pLogger)
    {
        if (!HMIsEnabled(pVM))
        {
            pVM->vmm.s.cbRCLogger = RT_OFFSETOF(RTLOGGERRC, afGroups[pLogger->cGroups]);
            rc = MMR3HyperAllocOnceNoRel(pVM, pVM->vmm.s.cbRCLogger, 0, MM_TAG_VMM, (void **)&pVM->vmm.s.pRCLoggerR3);
            if (RT_FAILURE(rc))
                return rc;
            pVM->vmm.s.pRCLoggerRC = MMHyperR3ToRC(pVM, pVM->vmm.s.pRCLoggerR3);
        }

# ifdef VBOX_WITH_R0_LOGGING
        size_t const cbLogger = RTLogCalcSizeForR0(pLogger->cGroups, 0);
        for (VMCPUID i = 0; i < pVM->cCpus; i++)
        {
            PVMCPU pVCpu = &pVM->aCpus[i];
            rc = MMR3HyperAllocOnceNoRelEx(pVM, cbLogger, PAGE_SIZE, MM_TAG_VMM, MMHYPER_AONR_FLAGS_KERNEL_MAPPING,
                                           (void **)&pVCpu->vmm.s.pR0LoggerR3);
            if (RT_FAILURE(rc))
                return rc;
            pVCpu->vmm.s.pR0LoggerR3->pVM        = pVM->pVMR0;
            //pVCpu->vmm.s.pR0LoggerR3->fCreated = false;
            pVCpu->vmm.s.pR0LoggerR3->cbLogger   = (uint32_t)cbLogger;
            pVCpu->vmm.s.pR0LoggerR0 = MMHyperR3ToR0(pVM, pVCpu->vmm.s.pR0LoggerR3);
        }
# endif
    }
#endif /* LOG_ENABLED */

#ifdef VBOX_WITH_RC_RELEASE_LOGGING
    /*
     * Allocate RC release logger instances (finalized in the relocator).
     */
    if (!HMIsEnabled(pVM))
    {
        PRTLOGGER pRelLogger = RTLogRelGetDefaultInstance();
        if (pRelLogger)
        {
            pVM->vmm.s.cbRCRelLogger = RT_OFFSETOF(RTLOGGERRC, afGroups[pRelLogger->cGroups]);
            rc = MMR3HyperAllocOnceNoRel(pVM, pVM->vmm.s.cbRCRelLogger, 0, MM_TAG_VMM, (void **)&pVM->vmm.s.pRCRelLoggerR3);
            if (RT_FAILURE(rc))
                return rc;
            pVM->vmm.s.pRCRelLoggerRC = MMHyperR3ToRC(pVM, pVM->vmm.s.pRCRelLoggerR3);
        }
    }
#endif /* VBOX_WITH_RC_RELEASE_LOGGING */
    return VINF_SUCCESS;
}


/**
 * VMMR3Init worker that register the statistics with STAM.
 *
 * @param   pVM         The cross context VM structure.
 */
static void vmmR3InitRegisterStats(PVM pVM)
{
    RT_NOREF_PV(pVM);

    /*
     * Statistics.
     */
    STAM_REG(pVM, &pVM->vmm.s.StatRunRC,                    STAMTYPE_COUNTER, "/VMM/RunRC",                     STAMUNIT_OCCURENCES, "Number of context switches.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetNormal,              STAMTYPE_COUNTER, "/VMM/RZRet/Normal",              STAMUNIT_OCCURENCES, "Number of VINF_SUCCESS returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetInterrupt,           STAMTYPE_COUNTER, "/VMM/RZRet/Interrupt",           STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_INTERRUPT returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetInterruptHyper,      STAMTYPE_COUNTER, "/VMM/RZRet/InterruptHyper",      STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_INTERRUPT_HYPER returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetGuestTrap,           STAMTYPE_COUNTER, "/VMM/RZRet/GuestTrap",           STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_GUEST_TRAP returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetRingSwitch,          STAMTYPE_COUNTER, "/VMM/RZRet/RingSwitch",          STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_RING_SWITCH returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetRingSwitchInt,       STAMTYPE_COUNTER, "/VMM/RZRet/RingSwitchInt",       STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_RING_SWITCH_INT returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetStaleSelector,       STAMTYPE_COUNTER, "/VMM/RZRet/StaleSelector",       STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_STALE_SELECTOR returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetIRETTrap,            STAMTYPE_COUNTER, "/VMM/RZRet/IRETTrap",            STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_IRET_TRAP returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetEmulate,             STAMTYPE_COUNTER, "/VMM/RZRet/Emulate",             STAMUNIT_OCCURENCES, "Number of VINF_EM_EXECUTE_INSTRUCTION returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetIOBlockEmulate,      STAMTYPE_COUNTER, "/VMM/RZRet/EmulateIOBlock",      STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_EMULATE_IO_BLOCK returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetPatchEmulate,        STAMTYPE_COUNTER, "/VMM/RZRet/PatchEmulate",        STAMUNIT_OCCURENCES, "Number of VINF_PATCH_EMULATE_INSTR returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetIORead,              STAMTYPE_COUNTER, "/VMM/RZRet/IORead",              STAMUNIT_OCCURENCES, "Number of VINF_IOM_R3_IOPORT_READ returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetIOWrite,             STAMTYPE_COUNTER, "/VMM/RZRet/IOWrite",             STAMUNIT_OCCURENCES, "Number of VINF_IOM_R3_IOPORT_WRITE returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetIOCommitWrite,       STAMTYPE_COUNTER, "/VMM/RZRet/IOCommitWrite",       STAMUNIT_OCCURENCES, "Number of VINF_IOM_R3_IOPORT_COMMIT_WRITE returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetMMIORead,            STAMTYPE_COUNTER, "/VMM/RZRet/MMIORead",            STAMUNIT_OCCURENCES, "Number of VINF_IOM_R3_MMIO_READ returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetMMIOWrite,           STAMTYPE_COUNTER, "/VMM/RZRet/MMIOWrite",           STAMUNIT_OCCURENCES, "Number of VINF_IOM_R3_MMIO_WRITE returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetMMIOCommitWrite,     STAMTYPE_COUNTER, "/VMM/RZRet/MMIOCommitWrite",     STAMUNIT_OCCURENCES, "Number of VINF_IOM_R3_MMIO_COMMIT_WRITE returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetMMIOReadWrite,       STAMTYPE_COUNTER, "/VMM/RZRet/MMIOReadWrite",       STAMUNIT_OCCURENCES, "Number of VINF_IOM_R3_MMIO_READ_WRITE returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetMMIOPatchRead,       STAMTYPE_COUNTER, "/VMM/RZRet/MMIOPatchRead",       STAMUNIT_OCCURENCES, "Number of VINF_IOM_HC_MMIO_PATCH_READ returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetMMIOPatchWrite,      STAMTYPE_COUNTER, "/VMM/RZRet/MMIOPatchWrite",      STAMUNIT_OCCURENCES, "Number of VINF_IOM_HC_MMIO_PATCH_WRITE returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetMSRRead,             STAMTYPE_COUNTER, "/VMM/RZRet/MSRRead",             STAMUNIT_OCCURENCES, "Number of VINF_CPUM_R3_MSR_READ returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetMSRWrite,            STAMTYPE_COUNTER, "/VMM/RZRet/MSRWrite",            STAMUNIT_OCCURENCES, "Number of VINF_CPUM_R3_MSR_WRITE returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetLDTFault,            STAMTYPE_COUNTER, "/VMM/RZRet/LDTFault",            STAMUNIT_OCCURENCES, "Number of VINF_EM_EXECUTE_INSTRUCTION_GDT_FAULT returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetGDTFault,            STAMTYPE_COUNTER, "/VMM/RZRet/GDTFault",            STAMUNIT_OCCURENCES, "Number of VINF_EM_EXECUTE_INSTRUCTION_LDT_FAULT returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetIDTFault,            STAMTYPE_COUNTER, "/VMM/RZRet/IDTFault",            STAMUNIT_OCCURENCES, "Number of VINF_EM_EXECUTE_INSTRUCTION_IDT_FAULT returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetTSSFault,            STAMTYPE_COUNTER, "/VMM/RZRet/TSSFault",            STAMUNIT_OCCURENCES, "Number of VINF_EM_EXECUTE_INSTRUCTION_TSS_FAULT returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetCSAMTask,            STAMTYPE_COUNTER, "/VMM/RZRet/CSAMTask",            STAMUNIT_OCCURENCES, "Number of VINF_CSAM_PENDING_ACTION returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetSyncCR3,             STAMTYPE_COUNTER, "/VMM/RZRet/SyncCR",              STAMUNIT_OCCURENCES, "Number of VINF_PGM_SYNC_CR3 returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetMisc,                STAMTYPE_COUNTER, "/VMM/RZRet/Misc",                STAMUNIT_OCCURENCES, "Number of misc returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetPatchInt3,           STAMTYPE_COUNTER, "/VMM/RZRet/PatchInt3",           STAMUNIT_OCCURENCES, "Number of VINF_PATM_PATCH_INT3 returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetPatchPF,             STAMTYPE_COUNTER, "/VMM/RZRet/PatchPF",             STAMUNIT_OCCURENCES, "Number of VINF_PATM_PATCH_TRAP_PF returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetPatchGP,             STAMTYPE_COUNTER, "/VMM/RZRet/PatchGP",             STAMUNIT_OCCURENCES, "Number of VINF_PATM_PATCH_TRAP_GP returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetPatchIretIRQ,        STAMTYPE_COUNTER, "/VMM/RZRet/PatchIret",           STAMUNIT_OCCURENCES, "Number of VINF_PATM_PENDING_IRQ_AFTER_IRET returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetRescheduleREM,       STAMTYPE_COUNTER, "/VMM/RZRet/ScheduleREM",         STAMUNIT_OCCURENCES, "Number of VINF_EM_RESCHEDULE_REM returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetToR3Total,           STAMTYPE_COUNTER, "/VMM/RZRet/ToR3",                STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TO_R3 returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetToR3Unknown,         STAMTYPE_COUNTER, "/VMM/RZRet/ToR3/Unknown",        STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TO_R3 returns without responsible force flag.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetToR3FF,              STAMTYPE_COUNTER, "/VMM/RZRet/ToR3/ToR3",           STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TO_R3 returns with VMCPU_FF_TO_R3.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetToR3TMVirt,          STAMTYPE_COUNTER, "/VMM/RZRet/ToR3/TMVirt",         STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TO_R3 returns with VM_FF_TM_VIRTUAL_SYNC.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetToR3HandyPages,      STAMTYPE_COUNTER, "/VMM/RZRet/ToR3/Handy",          STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TO_R3 returns with VM_FF_PGM_NEED_HANDY_PAGES.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetToR3PDMQueues,       STAMTYPE_COUNTER, "/VMM/RZRet/ToR3/PDMQueue",       STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TO_R3 returns with VM_FF_PDM_QUEUES.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetToR3Rendezvous,      STAMTYPE_COUNTER, "/VMM/RZRet/ToR3/Rendezvous",     STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TO_R3 returns with VM_FF_EMT_RENDEZVOUS.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetToR3Timer,           STAMTYPE_COUNTER, "/VMM/RZRet/ToR3/Timer",          STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TO_R3 returns with VMCPU_FF_TIMER.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetToR3DMA,             STAMTYPE_COUNTER, "/VMM/RZRet/ToR3/DMA",            STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TO_R3 returns with VM_FF_PDM_DMA.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetToR3CritSect,        STAMTYPE_COUNTER, "/VMM/RZRet/ToR3/CritSect",       STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TO_R3 returns with VMCPU_FF_PDM_CRITSECT.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetToR3Iem,             STAMTYPE_COUNTER, "/VMM/RZRet/ToR3/IEM",            STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TO_R3 returns with VMCPU_FF_IEM.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetToR3Iom,             STAMTYPE_COUNTER, "/VMM/RZRet/ToR3/IOM",            STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TO_R3 returns with VMCPU_FF_IOM.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetTimerPending,        STAMTYPE_COUNTER, "/VMM/RZRet/TimerPending",        STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_TIMER_PENDING returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetInterruptPending,    STAMTYPE_COUNTER, "/VMM/RZRet/InterruptPending",    STAMUNIT_OCCURENCES, "Number of VINF_EM_RAW_INTERRUPT_PENDING returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetPATMDuplicateFn,     STAMTYPE_COUNTER, "/VMM/RZRet/PATMDuplicateFn",     STAMUNIT_OCCURENCES, "Number of VINF_PATM_DUPLICATE_FUNCTION returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetPGMChangeMode,       STAMTYPE_COUNTER, "/VMM/RZRet/PGMChangeMode",       STAMUNIT_OCCURENCES, "Number of VINF_PGM_CHANGE_MODE returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetPGMFlushPending,     STAMTYPE_COUNTER, "/VMM/RZRet/PGMFlushPending",     STAMUNIT_OCCURENCES, "Number of VINF_PGM_POOL_FLUSH_PENDING returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetPendingRequest,      STAMTYPE_COUNTER, "/VMM/RZRet/PendingRequest",      STAMUNIT_OCCURENCES, "Number of VINF_EM_PENDING_REQUEST returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetPatchTPR,            STAMTYPE_COUNTER, "/VMM/RZRet/PatchTPR",            STAMUNIT_OCCURENCES, "Number of VINF_EM_HM_PATCH_TPR_INSTR returns.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZRetCallRing3,           STAMTYPE_COUNTER, "/VMM/RZCallR3/Misc",             STAMUNIT_OCCURENCES, "Number of Other ring-3 calls.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZCallPDMLock,            STAMTYPE_COUNTER, "/VMM/RZCallR3/PDMLock",          STAMUNIT_OCCURENCES, "Number of VMMCALLRING3_PDM_LOCK calls.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZCallPDMCritSectEnter,   STAMTYPE_COUNTER, "/VMM/RZCallR3/PDMCritSectEnter", STAMUNIT_OCCURENCES, "Number of VMMCALLRING3_PDM_CRITSECT_ENTER calls.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZCallPGMLock,            STAMTYPE_COUNTER, "/VMM/RZCallR3/PGMLock",          STAMUNIT_OCCURENCES, "Number of VMMCALLRING3_PGM_LOCK calls.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZCallPGMPoolGrow,        STAMTYPE_COUNTER, "/VMM/RZCallR3/PGMPoolGrow",      STAMUNIT_OCCURENCES, "Number of VMMCALLRING3_PGM_POOL_GROW calls.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZCallPGMMapChunk,        STAMTYPE_COUNTER, "/VMM/RZCallR3/PGMMapChunk",      STAMUNIT_OCCURENCES, "Number of VMMCALLRING3_PGM_MAP_CHUNK calls.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZCallPGMAllocHandy,      STAMTYPE_COUNTER, "/VMM/RZCallR3/PGMAllocHandy",    STAMUNIT_OCCURENCES, "Number of VMMCALLRING3_PGM_ALLOCATE_HANDY_PAGES calls.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZCallRemReplay,          STAMTYPE_COUNTER, "/VMM/RZCallR3/REMReplay",        STAMUNIT_OCCURENCES, "Number of VMMCALLRING3_REM_REPLAY_HANDLER_NOTIFICATIONS calls.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZCallLogFlush,           STAMTYPE_COUNTER, "/VMM/RZCallR3/VMMLogFlush",      STAMUNIT_OCCURENCES, "Number of VMMCALLRING3_VMM_LOGGER_FLUSH calls.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZCallVMSetError,         STAMTYPE_COUNTER, "/VMM/RZCallR3/VMSetError",       STAMUNIT_OCCURENCES, "Number of VMMCALLRING3_VM_SET_ERROR calls.");
    STAM_REG(pVM, &pVM->vmm.s.StatRZCallVMSetRuntimeError,  STAMTYPE_COUNTER, "/VMM/RZCallR3/VMRuntimeError",   STAMUNIT_OCCURENCES, "Number of VMMCALLRING3_VM_SET_RUNTIME_ERROR calls.");

#ifdef VBOX_WITH_STATISTICS
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        STAMR3RegisterF(pVM, &pVM->aCpus[i].vmm.s.CallRing3JmpBufR0.cbUsedMax,  STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,      "Max amount of stack used.", "/VMM/Stack/CPU%u/Max", i);
        STAMR3RegisterF(pVM, &pVM->aCpus[i].vmm.s.CallRing3JmpBufR0.cbUsedAvg,  STAMTYPE_U32,       STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,      "Average stack usage.",      "/VMM/Stack/CPU%u/Avg", i);
        STAMR3RegisterF(pVM, &pVM->aCpus[i].vmm.s.CallRing3JmpBufR0.cUsedTotal, STAMTYPE_U64,       STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Number of stack usages.",   "/VMM/Stack/CPU%u/Uses", i);
    }
#endif
}


/**
 * Initializes the R0 VMM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) VMMR3InitR0(PVM pVM)
{
    int    rc;
    PVMCPU pVCpu = VMMGetCpu(pVM);
    Assert(pVCpu && pVCpu->idCpu == 0);

#ifdef LOG_ENABLED
    /*
     * Initialize the ring-0 logger if we haven't done so yet.
     */
    if (    pVCpu->vmm.s.pR0LoggerR3
        &&  !pVCpu->vmm.s.pR0LoggerR3->fCreated)
    {
        rc = VMMR3UpdateLoggers(pVM);
        if (RT_FAILURE(rc))
            return rc;
    }
#endif

    /*
     * Call Ring-0 entry with init code.
     */
    for (;;)
    {
#ifdef NO_SUPCALLR0VMM
        //rc = VERR_GENERAL_FAILURE;
        rc = VINF_SUCCESS;
#else
        rc = SUPR3CallVMMR0Ex(pVM->pVMR0, 0 /*idCpu*/, VMMR0_DO_VMMR0_INIT,
                              RT_MAKE_U64(VMMGetSvnRev(), vmmGetBuildType()), NULL);
#endif
        /*
         * Flush the logs.
         */
#ifdef LOG_ENABLED
        if (    pVCpu->vmm.s.pR0LoggerR3
            &&  pVCpu->vmm.s.pR0LoggerR3->Logger.offScratch > 0)
            RTLogFlushR0(NULL, &pVCpu->vmm.s.pR0LoggerR3->Logger);
#endif
        if (rc != VINF_VMM_CALL_HOST)
            break;
        rc = vmmR3ServiceCallRing3Request(pVM, pVCpu);
        if (RT_FAILURE(rc) || (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST))
            break;
        /* Resume R0 */
    }

    if (RT_FAILURE(rc) || (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST))
    {
        LogRel(("VMM: R0 init failed, rc=%Rra\n", rc));
        if (RT_SUCCESS(rc))
            rc = VERR_IPE_UNEXPECTED_INFO_STATUS;
    }

    /* Log whether thread-context hooks are used (on Linux this can depend on how the kernel is configured). */
    if (pVM->aCpus[0].vmm.s.hCtxHook != NIL_RTTHREADCTXHOOK)
        LogRel(("VMM: Enabled thread-context hooks\n"));
    else
        LogRel(("VMM: Thread-context hooks unavailable\n"));

    return rc;
}


#ifdef VBOX_WITH_RAW_MODE
/**
 * Initializes the RC VMM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) VMMR3InitRC(PVM pVM)
{
    PVMCPU pVCpu = VMMGetCpu(pVM);
    Assert(pVCpu && pVCpu->idCpu == 0);

    /* In VMX mode, there's no need to init RC. */
    if (HMIsEnabled(pVM))
        return VINF_SUCCESS;

    AssertReturn(pVM->cCpus == 1, VERR_RAW_MODE_INVALID_SMP);

    /*
     * Call VMMRCInit():
     *      -# resolve the address.
     *      -# setup stackframe and EIP to use the trampoline.
     *      -# do a generic hypervisor call.
     */
    RTRCPTR RCPtrEP;
    int rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "VMMRCEntry", &RCPtrEP);
    if (RT_SUCCESS(rc))
    {
        CPUMSetHyperESP(pVCpu, pVCpu->vmm.s.pbEMTStackBottomRC); /* Clear the stack. */
        uint64_t u64TS = RTTimeProgramStartNanoTS();
        CPUMPushHyper(pVCpu, RT_HI_U32(u64TS));           /* Param 4: The program startup TS - Hi. */
        CPUMPushHyper(pVCpu, RT_LO_U32(u64TS));           /* Param 4: The program startup TS - Lo. */
        CPUMPushHyper(pVCpu, vmmGetBuildType());          /* Param 3: Version argument. */
        CPUMPushHyper(pVCpu, VMMGetSvnRev());             /* Param 2: Version argument. */
        CPUMPushHyper(pVCpu, VMMRC_DO_VMMRC_INIT);        /* Param 1: Operation. */
        CPUMPushHyper(pVCpu, pVM->pVMRC);                 /* Param 0: pVM */
        CPUMPushHyper(pVCpu, 6 * sizeof(RTRCPTR));        /* trampoline param: stacksize.  */
        CPUMPushHyper(pVCpu, RCPtrEP);                    /* Call EIP. */
        CPUMSetHyperEIP(pVCpu, pVM->vmm.s.pfnCallTrampolineRC);
        Assert(CPUMGetHyperCR3(pVCpu) && CPUMGetHyperCR3(pVCpu) == PGMGetHyperCR3(pVCpu));

        for (;;)
        {
#ifdef NO_SUPCALLR0VMM
            //rc = VERR_GENERAL_FAILURE;
            rc = VINF_SUCCESS;
#else
            rc = SUPR3CallVMMR0(pVM->pVMR0, 0 /* VCPU 0 */, VMMR0_DO_CALL_HYPERVISOR, NULL);
#endif
#ifdef LOG_ENABLED
            PRTLOGGERRC pLogger = pVM->vmm.s.pRCLoggerR3;
            if (    pLogger
                &&  pLogger->offScratch > 0)
                RTLogFlushRC(NULL, pLogger);
#endif
#ifdef VBOX_WITH_RC_RELEASE_LOGGING
            PRTLOGGERRC pRelLogger = pVM->vmm.s.pRCRelLoggerR3;
            if (RT_UNLIKELY(pRelLogger && pRelLogger->offScratch > 0))
                RTLogFlushRC(RTLogRelGetDefaultInstance(), pRelLogger);
#endif
            if (rc != VINF_VMM_CALL_HOST)
                break;
            rc = vmmR3ServiceCallRing3Request(pVM, pVCpu);
            if (RT_FAILURE(rc) || (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST))
                break;
        }

        /* Don't trigger assertions or guru if raw-mode is unavailable. */
        if (rc != VERR_SUPDRV_NO_RAW_MODE_HYPER_V_ROOT)
        {
            if (RT_FAILURE(rc) || (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST))
            {
                VMMR3FatalDump(pVM, pVCpu, rc);
                if (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST)
                    rc = VERR_IPE_UNEXPECTED_INFO_STATUS;
            }
            AssertRC(rc);
        }
    }
    return rc;
}
#endif /* VBOX_WITH_RAW_MODE */


/**
 * Called when an init phase completes.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   enmWhat             Which init phase.
 */
VMMR3_INT_DECL(int) VMMR3InitCompleted(PVM pVM, VMINITCOMPLETED enmWhat)
{
    int rc = VINF_SUCCESS;

    switch (enmWhat)
    {
        case VMINITCOMPLETED_RING3:
        {
            /*
             * Set page attributes to r/w for stack pages.
             */
            for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
            {
                rc = PGMMapSetPage(pVM, pVM->aCpus[idCpu].vmm.s.pbEMTStackRC, VMM_STACK_SIZE,
                                   X86_PTE_P | X86_PTE_A | X86_PTE_D | X86_PTE_RW);
                AssertRCReturn(rc, rc);
            }

            /*
             * Create the EMT yield timer.
             */
            rc = TMR3TimerCreateInternal(pVM, TMCLOCK_REAL, vmmR3YieldEMT, NULL, "EMT Yielder", &pVM->vmm.s.pYieldTimer);
            AssertRCReturn(rc, rc);

            rc = TMTimerSetMillies(pVM->vmm.s.pYieldTimer, pVM->vmm.s.cYieldEveryMillies);
            AssertRCReturn(rc, rc);

#ifdef VBOX_WITH_NMI
            /*
             * Map the host APIC into GC - This is AMD/Intel + Host OS specific!
             */
            rc = PGMMap(pVM, pVM->vmm.s.GCPtrApicBase, 0xfee00000, PAGE_SIZE,
                        X86_PTE_P | X86_PTE_RW | X86_PTE_PWT | X86_PTE_PCD | X86_PTE_A | X86_PTE_D);
            AssertRCReturn(rc, rc);
#endif

#ifdef VBOX_STRICT_VMM_STACK
            /*
             * Setup the stack guard pages: Two inaccessible pages at each sides of the
             * stack to catch over/under-flows.
             */
            for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
            {
                uint8_t *pbEMTStackR3 = pVM->aCpus[idCpu].vmm.s.pbEMTStackR3;

                memset(pbEMTStackR3 - PAGE_SIZE, 0xcc, PAGE_SIZE);
                MMR3HyperSetGuard(pVM, pbEMTStackR3 - PAGE_SIZE, PAGE_SIZE, true /*fSet*/);

                memset(pbEMTStackR3 + VMM_STACK_SIZE, 0xcc, PAGE_SIZE);
                MMR3HyperSetGuard(pVM, pbEMTStackR3 + VMM_STACK_SIZE, PAGE_SIZE, true /*fSet*/);
            }
            pVM->vmm.s.fStackGuardsStationed = true;
#endif
            break;
        }

        case VMINITCOMPLETED_HM:
        {
            /*
             * Disable the periodic preemption timers if we can use the
             * VMX-preemption timer instead.
             */
            if (   pVM->vmm.s.fUsePeriodicPreemptionTimers
                && HMR3IsVmxPreemptionTimerUsed(pVM))
                pVM->vmm.s.fUsePeriodicPreemptionTimers = false;
            LogRel(("VMM: fUsePeriodicPreemptionTimers=%RTbool\n", pVM->vmm.s.fUsePeriodicPreemptionTimers));

            /*
             * Last chance for GIM to update its CPUID leaves if it requires
             * knowledge/information from HM initialization.
             */
            rc = GIMR3InitCompleted(pVM);
            AssertRCReturn(rc, rc);

            /*
             * CPUM's post-initialization (print CPUIDs).
             */
            CPUMR3LogCpuIds(pVM);
            break;
        }

        default: /* shuts up gcc */
            break;
    }

    return rc;
}


/**
 * Terminate the VMM bits.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) VMMR3Term(PVM pVM)
{
    PVMCPU pVCpu = VMMGetCpu(pVM);
    Assert(pVCpu && pVCpu->idCpu == 0);

    /*
     * Call Ring-0 entry with termination code.
     */
    int rc;
    for (;;)
    {
#ifdef NO_SUPCALLR0VMM
        //rc = VERR_GENERAL_FAILURE;
        rc = VINF_SUCCESS;
#else
        rc = SUPR3CallVMMR0Ex(pVM->pVMR0, 0 /*idCpu*/, VMMR0_DO_VMMR0_TERM, 0, NULL);
#endif
        /*
         * Flush the logs.
         */
#ifdef LOG_ENABLED
        if (    pVCpu->vmm.s.pR0LoggerR3
            &&  pVCpu->vmm.s.pR0LoggerR3->Logger.offScratch > 0)
            RTLogFlushR0(NULL, &pVCpu->vmm.s.pR0LoggerR3->Logger);
#endif
        if (rc != VINF_VMM_CALL_HOST)
            break;
        rc = vmmR3ServiceCallRing3Request(pVM, pVCpu);
        if (RT_FAILURE(rc) || (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST))
            break;
        /* Resume R0 */
    }
    if (RT_FAILURE(rc) || (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST))
    {
        LogRel(("VMM: VMMR3Term: R0 term failed, rc=%Rra. (warning)\n", rc));
        if (RT_SUCCESS(rc))
            rc = VERR_IPE_UNEXPECTED_INFO_STATUS;
    }

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        RTSemEventDestroy(pVM->vmm.s.pahEvtRendezvousEnterOrdered[i]);
        pVM->vmm.s.pahEvtRendezvousEnterOrdered[i] = NIL_RTSEMEVENT;
    }
    RTSemEventDestroy(pVM->vmm.s.hEvtRendezvousEnterOneByOne);
    pVM->vmm.s.hEvtRendezvousEnterOneByOne = NIL_RTSEMEVENT;
    RTSemEventMultiDestroy(pVM->vmm.s.hEvtMulRendezvousEnterAllAtOnce);
    pVM->vmm.s.hEvtMulRendezvousEnterAllAtOnce = NIL_RTSEMEVENTMULTI;
    RTSemEventMultiDestroy(pVM->vmm.s.hEvtMulRendezvousDone);
    pVM->vmm.s.hEvtMulRendezvousDone = NIL_RTSEMEVENTMULTI;
    RTSemEventDestroy(pVM->vmm.s.hEvtRendezvousDoneCaller);
    pVM->vmm.s.hEvtRendezvousDoneCaller = NIL_RTSEMEVENT;
    RTSemEventMultiDestroy(pVM->vmm.s.hEvtMulRendezvousRecursionPush);
    pVM->vmm.s.hEvtMulRendezvousRecursionPush = NIL_RTSEMEVENTMULTI;
    RTSemEventMultiDestroy(pVM->vmm.s.hEvtMulRendezvousRecursionPop);
    pVM->vmm.s.hEvtMulRendezvousRecursionPop = NIL_RTSEMEVENTMULTI;
    RTSemEventDestroy(pVM->vmm.s.hEvtRendezvousRecursionPushCaller);
    pVM->vmm.s.hEvtRendezvousRecursionPushCaller = NIL_RTSEMEVENT;
    RTSemEventDestroy(pVM->vmm.s.hEvtRendezvousRecursionPopCaller);
    pVM->vmm.s.hEvtRendezvousRecursionPopCaller = NIL_RTSEMEVENT;

#ifdef VBOX_STRICT_VMM_STACK
    /*
     * Make the two stack guard pages present again.
     */
    if (pVM->vmm.s.fStackGuardsStationed)
    {
        for (VMCPUID i = 0; i < pVM->cCpus; i++)
        {
            uint8_t *pbEMTStackR3 = pVM->aCpus[i].vmm.s.pbEMTStackR3;
            MMR3HyperSetGuard(pVM, pbEMTStackR3 - PAGE_SIZE,      PAGE_SIZE, false /*fSet*/);
            MMR3HyperSetGuard(pVM, pbEMTStackR3 + VMM_STACK_SIZE, PAGE_SIZE, false /*fSet*/);
        }
        pVM->vmm.s.fStackGuardsStationed = false;
    }
#endif

    vmmTermFormatTypes();
    return rc;
}


/**
 * Applies relocations to data and code managed by this
 * component. This function will be called at init and
 * whenever the VMM need to relocate it self inside the GC.
 *
 * The VMM will need to apply relocations to the core code.
 *
 * @param   pVM         The cross context VM structure.
 * @param   offDelta    The relocation delta.
 */
VMMR3_INT_DECL(void) VMMR3Relocate(PVM pVM, RTGCINTPTR offDelta)
{
    LogFlow(("VMMR3Relocate: offDelta=%RGv\n", offDelta));

    /*
     * Recalc the RC address.
     */
#ifdef VBOX_WITH_RAW_MODE
    pVM->vmm.s.pvCoreCodeRC = MMHyperR3ToRC(pVM, pVM->vmm.s.pvCoreCodeR3);
#endif

    /*
     * The stack.
     */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];

        CPUMSetHyperESP(pVCpu, CPUMGetHyperESP(pVCpu) + offDelta);

        pVCpu->vmm.s.pbEMTStackRC       = MMHyperR3ToRC(pVM, pVCpu->vmm.s.pbEMTStackR3);
        pVCpu->vmm.s.pbEMTStackBottomRC = pVCpu->vmm.s.pbEMTStackRC + VMM_STACK_SIZE;
    }

    /*
     * All the switchers.
     */
    vmmR3SwitcherRelocate(pVM, offDelta);

    /*
     * Get other RC entry points.
     */
    if (!HMIsEnabled(pVM))
    {
        int rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "CPUMGCResumeGuest", &pVM->vmm.s.pfnCPUMRCResumeGuest);
        AssertReleaseMsgRC(rc, ("CPUMGCResumeGuest not found! rc=%Rra\n", rc));

        rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "CPUMGCResumeGuestV86", &pVM->vmm.s.pfnCPUMRCResumeGuestV86);
        AssertReleaseMsgRC(rc, ("CPUMGCResumeGuestV86 not found! rc=%Rra\n", rc));
    }

    /*
     * Update the logger.
     */
    VMMR3UpdateLoggers(pVM);
}


/**
 * Updates the settings for the RC and R0 loggers.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(int) VMMR3UpdateLoggers(PVM pVM)
{
    /*
     * Simply clone the logger instance (for RC).
     */
    int rc = VINF_SUCCESS;
    RTRCPTR RCPtrLoggerFlush = 0;

    if (   pVM->vmm.s.pRCLoggerR3
#ifdef VBOX_WITH_RC_RELEASE_LOGGING
        || pVM->vmm.s.pRCRelLoggerR3
#endif
       )
    {
        Assert(!HMIsEnabled(pVM));
        rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "vmmGCLoggerFlush", &RCPtrLoggerFlush);
        AssertReleaseMsgRC(rc, ("vmmGCLoggerFlush not found! rc=%Rra\n", rc));
    }

    if (pVM->vmm.s.pRCLoggerR3)
    {
        Assert(!HMIsEnabled(pVM));
        RTRCPTR RCPtrLoggerWrapper = 0;
        rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "vmmGCLoggerWrapper", &RCPtrLoggerWrapper);
        AssertReleaseMsgRC(rc, ("vmmGCLoggerWrapper not found! rc=%Rra\n", rc));

        pVM->vmm.s.pRCLoggerRC = MMHyperR3ToRC(pVM, pVM->vmm.s.pRCLoggerR3);
        rc = RTLogCloneRC(NULL /* default */, pVM->vmm.s.pRCLoggerR3, pVM->vmm.s.cbRCLogger,
                          RCPtrLoggerWrapper, RCPtrLoggerFlush, RTLOGFLAGS_BUFFERED);
        AssertReleaseMsgRC(rc, ("RTLogCloneRC failed! rc=%Rra\n", rc));
    }

#ifdef VBOX_WITH_RC_RELEASE_LOGGING
    if (pVM->vmm.s.pRCRelLoggerR3)
    {
        Assert(!HMIsEnabled(pVM));
        RTRCPTR RCPtrLoggerWrapper = 0;
        rc = PDMR3LdrGetSymbolRC(pVM, VMMRC_MAIN_MODULE_NAME, "vmmGCRelLoggerWrapper", &RCPtrLoggerWrapper);
        AssertReleaseMsgRC(rc, ("vmmGCRelLoggerWrapper not found! rc=%Rra\n", rc));

        pVM->vmm.s.pRCRelLoggerRC = MMHyperR3ToRC(pVM, pVM->vmm.s.pRCRelLoggerR3);
        rc = RTLogCloneRC(RTLogRelGetDefaultInstance(), pVM->vmm.s.pRCRelLoggerR3, pVM->vmm.s.cbRCRelLogger,
                          RCPtrLoggerWrapper, RCPtrLoggerFlush, RTLOGFLAGS_BUFFERED);
        AssertReleaseMsgRC(rc, ("RTLogCloneRC failed! rc=%Rra\n", rc));
    }
#endif /* VBOX_WITH_RC_RELEASE_LOGGING */

#ifdef LOG_ENABLED
    /*
     * For the ring-0 EMT logger, we use a per-thread logger instance
     * in ring-0. Only initialize it once.
     */
    PRTLOGGER const pDefault = RTLogDefaultInstance();
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU       pVCpu = &pVM->aCpus[i];
        PVMMR0LOGGER pR0LoggerR3 = pVCpu->vmm.s.pR0LoggerR3;
        if (pR0LoggerR3)
        {
            if (!pR0LoggerR3->fCreated)
            {
                RTR0PTR pfnLoggerWrapper = NIL_RTR0PTR;
                rc = PDMR3LdrGetSymbolR0(pVM, VMMR0_MAIN_MODULE_NAME, "vmmR0LoggerWrapper", &pfnLoggerWrapper);
                AssertReleaseMsgRCReturn(rc, ("vmmR0LoggerWrapper not found! rc=%Rra\n", rc), rc);

                RTR0PTR pfnLoggerFlush = NIL_RTR0PTR;
                rc = PDMR3LdrGetSymbolR0(pVM, VMMR0_MAIN_MODULE_NAME, "vmmR0LoggerFlush", &pfnLoggerFlush);
                AssertReleaseMsgRCReturn(rc, ("vmmR0LoggerFlush not found! rc=%Rra\n", rc), rc);

                rc = RTLogCreateForR0(&pR0LoggerR3->Logger, pR0LoggerR3->cbLogger,
                                      pVCpu->vmm.s.pR0LoggerR0 + RT_OFFSETOF(VMMR0LOGGER, Logger),
                                      pfnLoggerWrapper, pfnLoggerFlush,
                                      RTLOGFLAGS_BUFFERED, RTLOGDEST_DUMMY);
                AssertReleaseMsgRCReturn(rc, ("RTLogCreateForR0 failed! rc=%Rra\n", rc), rc);

                RTR0PTR pfnLoggerPrefix = NIL_RTR0PTR;
                rc = PDMR3LdrGetSymbolR0(pVM, VMMR0_MAIN_MODULE_NAME, "vmmR0LoggerPrefix", &pfnLoggerPrefix);
                AssertReleaseMsgRCReturn(rc, ("vmmR0LoggerPrefix not found! rc=%Rra\n", rc), rc);
                rc = RTLogSetCustomPrefixCallbackForR0(&pR0LoggerR3->Logger,
                                                       pVCpu->vmm.s.pR0LoggerR0 + RT_OFFSETOF(VMMR0LOGGER, Logger),
                                                       pfnLoggerPrefix, NIL_RTR0PTR);
                AssertReleaseMsgRCReturn(rc, ("RTLogSetCustomPrefixCallback failed! rc=%Rra\n", rc), rc);

                pR0LoggerR3->idCpu = i;
                pR0LoggerR3->fCreated = true;
                pR0LoggerR3->fFlushingDisabled = false;

            }

            rc = RTLogCopyGroupsAndFlagsForR0(&pR0LoggerR3->Logger, pVCpu->vmm.s.pR0LoggerR0 + RT_OFFSETOF(VMMR0LOGGER, Logger),
                                              pDefault, RTLOGFLAGS_BUFFERED, UINT32_MAX);
            AssertRC(rc);
        }
    }
#endif
    return rc;
}


/**
 * Gets the pointer to a buffer containing the R0/RC RTAssertMsg1Weak output.
 *
 * @returns Pointer to the buffer.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(const char *) VMMR3GetRZAssertMsg1(PVM pVM)
{
    if (HMIsEnabled(pVM))
        return pVM->vmm.s.szRing0AssertMsg1;

    RTRCPTR RCPtr;
    int rc = PDMR3LdrGetSymbolRC(pVM, NULL, "g_szRTAssertMsg1", &RCPtr);
    if (RT_SUCCESS(rc))
        return (const char *)MMHyperRCToR3(pVM, RCPtr);

    return NULL;
}


/**
 * Returns the VMCPU of the specified virtual CPU.
 *
 * @returns The VMCPU pointer. NULL if @a idCpu or @a pUVM is invalid.
 *
 * @param   pUVM        The user mode VM handle.
 * @param   idCpu       The ID of the virtual CPU.
 */
VMMR3DECL(PVMCPU) VMMR3GetCpuByIdU(PUVM pUVM, RTCPUID idCpu)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, NULL);
    AssertReturn(idCpu < pUVM->cCpus, NULL);
    VM_ASSERT_VALID_EXT_RETURN(pUVM->pVM, NULL);
    return &pUVM->pVM->aCpus[idCpu];
}


/**
 * Gets the pointer to a buffer containing the R0/RC RTAssertMsg2Weak output.
 *
 * @returns Pointer to the buffer.
 * @param   pVM         The cross context VM structure.
 */
VMMR3DECL(const char *) VMMR3GetRZAssertMsg2(PVM pVM)
{
    if (HMIsEnabled(pVM))
        return pVM->vmm.s.szRing0AssertMsg2;

    RTRCPTR RCPtr;
    int rc = PDMR3LdrGetSymbolRC(pVM, NULL, "g_szRTAssertMsg2", &RCPtr);
    if (RT_SUCCESS(rc))
        return (const char *)MMHyperRCToR3(pVM, RCPtr);

    return NULL;
}


/**
 * Execute state save operation.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            SSM operation handle.
 */
static DECLCALLBACK(int) vmmR3Save(PVM pVM, PSSMHANDLE pSSM)
{
    LogFlow(("vmmR3Save:\n"));

    /*
     * Save the started/stopped state of all CPUs except 0 as it will always
     * be running. This avoids breaking the saved state version. :-)
     */
    for (VMCPUID i = 1; i < pVM->cCpus; i++)
        SSMR3PutBool(pSSM, VMCPUSTATE_IS_STARTED(VMCPU_GET_STATE(&pVM->aCpus[i])));

    return SSMR3PutU32(pSSM, UINT32_MAX); /* terminator */
}


/**
 * Execute state load operation.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            SSM operation handle.
 * @param   uVersion        Data layout version.
 * @param   uPass           The data pass.
 */
static DECLCALLBACK(int) vmmR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    LogFlow(("vmmR3Load:\n"));
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    /*
     * Validate version.
     */
    if (    uVersion != VMM_SAVED_STATE_VERSION
        &&  uVersion != VMM_SAVED_STATE_VERSION_3_0)
    {
        AssertMsgFailed(("vmmR3Load: Invalid version uVersion=%u!\n", uVersion));
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    }

    if (uVersion <= VMM_SAVED_STATE_VERSION_3_0)
    {
        /* Ignore the stack bottom, stack pointer and stack bits. */
        RTRCPTR RCPtrIgnored;
        SSMR3GetRCPtr(pSSM, &RCPtrIgnored);
        SSMR3GetRCPtr(pSSM, &RCPtrIgnored);
#ifdef RT_OS_DARWIN
        if (   SSMR3HandleVersion(pSSM)  >= VBOX_FULL_VERSION_MAKE(3,0,0)
            && SSMR3HandleVersion(pSSM)  <  VBOX_FULL_VERSION_MAKE(3,1,0)
            && SSMR3HandleRevision(pSSM) >= 48858
            && (   !strcmp(SSMR3HandleHostOSAndArch(pSSM), "darwin.x86")
                || !strcmp(SSMR3HandleHostOSAndArch(pSSM), "") )
           )
            SSMR3Skip(pSSM, 16384);
        else
            SSMR3Skip(pSSM, 8192);
#else
        SSMR3Skip(pSSM, 8192);
#endif
    }

    /*
     * Restore the VMCPU states. VCPU 0 is always started.
     */
    VMCPU_SET_STATE(&pVM->aCpus[0], VMCPUSTATE_STARTED);
    for (VMCPUID i = 1; i < pVM->cCpus; i++)
    {
        bool fStarted;
        int rc = SSMR3GetBool(pSSM, &fStarted);
        if (RT_FAILURE(rc))
            return rc;
        VMCPU_SET_STATE(&pVM->aCpus[i], fStarted ? VMCPUSTATE_STARTED : VMCPUSTATE_STOPPED);
    }

    /* terminator */
    uint32_t u32;
    int rc = SSMR3GetU32(pSSM, &u32);
    if (RT_FAILURE(rc))
        return rc;
    if (u32 != UINT32_MAX)
    {
        AssertMsgFailed(("u32=%#x\n", u32));
        return VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
    }
    return VINF_SUCCESS;
}


#ifdef VBOX_WITH_RAW_MODE
/**
 * Resolve a builtin RC symbol.
 *
 * Called by PDM when loading or relocating RC modules.
 *
 * @returns VBox status
 * @param   pVM             The cross context VM structure.
 * @param   pszSymbol       Symbol to resolve.
 * @param   pRCPtrValue     Where to store the symbol value.
 *
 * @remark  This has to work before VMMR3Relocate() is called.
 */
VMMR3_INT_DECL(int) VMMR3GetImportRC(PVM pVM, const char *pszSymbol, PRTRCPTR pRCPtrValue)
{
    if (!strcmp(pszSymbol, "g_Logger"))
    {
        if (pVM->vmm.s.pRCLoggerR3)
            pVM->vmm.s.pRCLoggerRC = MMHyperR3ToRC(pVM, pVM->vmm.s.pRCLoggerR3);
        *pRCPtrValue = pVM->vmm.s.pRCLoggerRC;
    }
    else if (!strcmp(pszSymbol, "g_RelLogger"))
    {
# ifdef VBOX_WITH_RC_RELEASE_LOGGING
        if (pVM->vmm.s.pRCRelLoggerR3)
            pVM->vmm.s.pRCRelLoggerRC = MMHyperR3ToRC(pVM, pVM->vmm.s.pRCRelLoggerR3);
        *pRCPtrValue = pVM->vmm.s.pRCRelLoggerRC;
# else
        *pRCPtrValue = NIL_RTRCPTR;
# endif
    }
    else
        return VERR_SYMBOL_NOT_FOUND;
    return VINF_SUCCESS;
}
#endif /* VBOX_WITH_RAW_MODE */


/**
 * Suspends the CPU yielder.
 *
 * @param   pVM             The cross context VM structure.
 */
VMMR3_INT_DECL(void) VMMR3YieldSuspend(PVM pVM)
{
    VMCPU_ASSERT_EMT(&pVM->aCpus[0]);
    if (!pVM->vmm.s.cYieldResumeMillies)
    {
        uint64_t u64Now = TMTimerGet(pVM->vmm.s.pYieldTimer);
        uint64_t u64Expire = TMTimerGetExpire(pVM->vmm.s.pYieldTimer);
        if (u64Now >= u64Expire || u64Expire == ~(uint64_t)0)
            pVM->vmm.s.cYieldResumeMillies = pVM->vmm.s.cYieldEveryMillies;
        else
            pVM->vmm.s.cYieldResumeMillies = TMTimerToMilli(pVM->vmm.s.pYieldTimer, u64Expire - u64Now);
        TMTimerStop(pVM->vmm.s.pYieldTimer);
    }
    pVM->vmm.s.u64LastYield = RTTimeNanoTS();
}


/**
 * Stops the CPU yielder.
 *
 * @param   pVM             The cross context VM structure.
 */
VMMR3_INT_DECL(void) VMMR3YieldStop(PVM pVM)
{
    if (!pVM->vmm.s.cYieldResumeMillies)
        TMTimerStop(pVM->vmm.s.pYieldTimer);
    pVM->vmm.s.cYieldResumeMillies = pVM->vmm.s.cYieldEveryMillies;
    pVM->vmm.s.u64LastYield = RTTimeNanoTS();
}


/**
 * Resumes the CPU yielder when it has been a suspended or stopped.
 *
 * @param   pVM             The cross context VM structure.
 */
VMMR3_INT_DECL(void) VMMR3YieldResume(PVM pVM)
{
    if (pVM->vmm.s.cYieldResumeMillies)
    {
        TMTimerSetMillies(pVM->vmm.s.pYieldTimer, pVM->vmm.s.cYieldResumeMillies);
        pVM->vmm.s.cYieldResumeMillies = 0;
    }
}


/**
 * Internal timer callback function.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pTimer          The timer handle.
 * @param   pvUser          User argument specified upon timer creation.
 */
static DECLCALLBACK(void) vmmR3YieldEMT(PVM pVM, PTMTIMER pTimer, void *pvUser)
{
    NOREF(pvUser);

    /*
     * This really needs some careful tuning. While we shouldn't be too greedy since
     * that'll cause the rest of the system to stop up, we shouldn't be too nice either
     * because that'll cause us to stop up.
     *
     * The current logic is to use the default interval when there is no lag worth
     * mentioning, but when we start accumulating lag we don't bother yielding at all.
     *
     * (This depends on the TMCLOCK_VIRTUAL_SYNC to be scheduled before TMCLOCK_REAL
     * so the lag is up to date.)
     */
    const uint64_t u64Lag = TMVirtualSyncGetLag(pVM);
    if (    u64Lag     <   50000000 /* 50ms */
        ||  (   u64Lag < 1000000000 /*  1s */
             && RTTimeNanoTS() - pVM->vmm.s.u64LastYield < 500000000 /* 500 ms */)
       )
    {
        uint64_t u64Elapsed = RTTimeNanoTS();
        pVM->vmm.s.u64LastYield = u64Elapsed;

        RTThreadYield();

#ifdef LOG_ENABLED
        u64Elapsed = RTTimeNanoTS() - u64Elapsed;
        Log(("vmmR3YieldEMT: %RI64 ns\n", u64Elapsed));
#endif
    }
    TMTimerSetMillies(pTimer, pVM->vmm.s.cYieldEveryMillies);
}


#ifdef VBOX_WITH_RAW_MODE
/**
 * Executes guest code in the raw-mode context.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR3_INT_DECL(int) VMMR3RawRunGC(PVM pVM, PVMCPU pVCpu)
{
    Log2(("VMMR3RawRunGC: (cs:eip=%04x:%08x)\n", CPUMGetGuestCS(pVCpu), CPUMGetGuestEIP(pVCpu)));

    AssertReturn(pVM->cCpus == 1, VERR_RAW_MODE_INVALID_SMP);

    /*
     * Set the hypervisor to resume executing a CPUM resume function
     * in CPUMRCA.asm.
     */
    CPUMSetHyperState(pVCpu,
                        CPUMGetGuestEFlags(pVCpu) & X86_EFL_VM
                      ? pVM->vmm.s.pfnCPUMRCResumeGuestV86
                      : pVM->vmm.s.pfnCPUMRCResumeGuest,  /* eip */
                      pVCpu->vmm.s.pbEMTStackBottomRC,    /* esp */
                      0,                                  /* eax */
                      VM_RC_ADDR(pVM, &pVCpu->cpum)       /* edx */);

    /*
     * We hide log flushes (outer) and hypervisor interrupts (inner).
     */
    for (;;)
    {
#ifdef VBOX_STRICT
        if (RT_UNLIKELY(!CPUMGetHyperCR3(pVCpu) || CPUMGetHyperCR3(pVCpu) != PGMGetHyperCR3(pVCpu)))
            EMR3FatalError(pVCpu, VERR_VMM_HYPER_CR3_MISMATCH);
        PGMMapCheck(pVM);
# ifdef VBOX_WITH_SAFE_STR
        SELMR3CheckShadowTR(pVM);
# endif
#endif
        int rc;
        do
        {
#ifdef NO_SUPCALLR0VMM
            rc = VERR_GENERAL_FAILURE;
#else
            rc = SUPR3CallVMMR0Fast(pVM->pVMR0, VMMR0_DO_RAW_RUN, 0);
            if (RT_LIKELY(rc == VINF_SUCCESS))
                rc = pVCpu->vmm.s.iLastGZRc;
#endif
        } while (rc == VINF_EM_RAW_INTERRUPT_HYPER);

        /*
         * Flush the logs.
         */
#ifdef LOG_ENABLED
        PRTLOGGERRC pLogger = pVM->vmm.s.pRCLoggerR3;
        if (    pLogger
            &&  pLogger->offScratch > 0)
            RTLogFlushRC(NULL, pLogger);
#endif
#ifdef VBOX_WITH_RC_RELEASE_LOGGING
        PRTLOGGERRC pRelLogger = pVM->vmm.s.pRCRelLoggerR3;
        if (RT_UNLIKELY(pRelLogger && pRelLogger->offScratch > 0))
            RTLogFlushRC(RTLogRelGetDefaultInstance(), pRelLogger);
#endif
        if (rc != VINF_VMM_CALL_HOST)
        {
            Log2(("VMMR3RawRunGC: returns %Rrc (cs:eip=%04x:%08x)\n", rc, CPUMGetGuestCS(pVCpu), CPUMGetGuestEIP(pVCpu)));
            return rc;
        }
        rc = vmmR3ServiceCallRing3Request(pVM, pVCpu);
        if (RT_FAILURE(rc))
            return rc;
        /* Resume GC */
    }
}
#endif /* VBOX_WITH_RAW_MODE */


/**
 * Executes guest code (Intel VT-x and AMD-V).
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR3_INT_DECL(int) VMMR3HmRunGC(PVM pVM, PVMCPU pVCpu)
{
    Log2(("VMMR3HmRunGC: (cs:rip=%04x:%RX64)\n", CPUMGetGuestCS(pVCpu), CPUMGetGuestRIP(pVCpu)));

    for (;;)
    {
        int rc;
        do
        {
#ifdef NO_SUPCALLR0VMM
            rc = VERR_GENERAL_FAILURE;
#else
            rc = SUPR3CallVMMR0Fast(pVM->pVMR0, VMMR0_DO_HM_RUN, pVCpu->idCpu);
            if (RT_LIKELY(rc == VINF_SUCCESS))
                rc = pVCpu->vmm.s.iLastGZRc;
#endif
        } while (rc == VINF_EM_RAW_INTERRUPT_HYPER);

#if 0 /** @todo triggers too often */
        Assert(!VMCPU_FF_IS_SET(pVCpu, VMCPU_FF_TO_R3));
#endif

#ifdef LOG_ENABLED
        /*
         * Flush the log
         */
        PVMMR0LOGGER pR0LoggerR3 = pVCpu->vmm.s.pR0LoggerR3;
        if (    pR0LoggerR3
            &&  pR0LoggerR3->Logger.offScratch > 0)
            RTLogFlushR0(NULL, &pR0LoggerR3->Logger);
#endif /* !LOG_ENABLED */
        if (rc != VINF_VMM_CALL_HOST)
        {
            Log2(("VMMR3HmRunGC: returns %Rrc (cs:rip=%04x:%RX64)\n", rc, CPUMGetGuestCS(pVCpu), CPUMGetGuestRIP(pVCpu)));
            return rc;
        }
        rc = vmmR3ServiceCallRing3Request(pVM, pVCpu);
        if (RT_FAILURE(rc))
            return rc;
        /* Resume R0 */
    }
}


/**
 * VCPU worker for VMMSendStartupIpi.
 *
 * @param   pVM         The cross context VM structure.
 * @param   idCpu       Virtual CPU to perform SIPI on.
 * @param   uVector     The SIPI vector.
 */
static DECLCALLBACK(int) vmmR3SendStarupIpi(PVM pVM, VMCPUID idCpu, uint32_t uVector)
{
    PVMCPU pVCpu = VMMGetCpuById(pVM, idCpu);
    VMCPU_ASSERT_EMT(pVCpu);

    /*
     * Active, halt and shutdown states of the processor all block SIPIs.
     * So we can safely discard the SIPI. See Intel spec. 26.6.2 "Activity State".
     */
    if (EMGetState(pVCpu) != EMSTATE_WAIT_SIPI)
        return VERR_ACCESS_DENIED;


    PCPUMCTX pCtx = CPUMQueryGuestCtxPtr(pVCpu);

    pCtx->cs.Sel        = uVector << 8;
    pCtx->cs.ValidSel   = uVector << 8;
    pCtx->cs.fFlags     = CPUMSELREG_FLAGS_VALID;
    pCtx->cs.u64Base    = uVector << 12;
    pCtx->cs.u32Limit   = UINT32_C(0x0000ffff);
    pCtx->rip           = 0;

    Log(("vmmR3SendSipi for VCPU %d with vector %x\n", idCpu, uVector));

# if 1 /* If we keep the EMSTATE_WAIT_SIPI method, then move this to EM.cpp. */
    EMSetState(pVCpu, EMSTATE_HALTED);
    return VINF_EM_RESCHEDULE;
# else /* And if we go the VMCPU::enmState way it can stay here. */
    VMCPU_ASSERT_STATE(pVCpu, VMCPUSTATE_STOPPED);
    VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED);
    return VINF_SUCCESS;
# endif
}


static DECLCALLBACK(int) vmmR3SendInitIpi(PVM pVM, VMCPUID idCpu)
{
    PVMCPU pVCpu = VMMGetCpuById(pVM, idCpu);
    VMCPU_ASSERT_EMT(pVCpu);

    Log(("vmmR3SendInitIpi for VCPU %d\n", idCpu));

    /** @todo Figure out how to handle a nested-guest intercepts here for INIT
     *  IPI (e.g. SVM_EXIT_INIT). */

    PGMR3ResetCpu(pVM, pVCpu);
    PDMR3ResetCpu(pVCpu);   /* Only clears pending interrupts force flags */
    APICR3InitIpi(pVCpu);
    TRPMR3ResetCpu(pVCpu);
    CPUMR3ResetCpu(pVM, pVCpu);
    EMR3ResetCpu(pVCpu);
    HMR3ResetCpu(pVCpu);

    /* This will trickle up on the target EMT. */
    return VINF_EM_WAIT_SIPI;
}


/**
 * Sends a Startup IPI to the virtual CPU by setting CS:EIP into
 * vector-dependent state and unhalting processor.
 *
 * @param   pVM         The cross context VM structure.
 * @param   idCpu       Virtual CPU to perform SIPI on.
 * @param   uVector     SIPI vector.
 */
VMMR3_INT_DECL(void) VMMR3SendStartupIpi(PVM pVM, VMCPUID idCpu,  uint32_t uVector)
{
    AssertReturnVoid(idCpu < pVM->cCpus);

    int rc = VMR3ReqCallNoWait(pVM, idCpu, (PFNRT)vmmR3SendStarupIpi, 3, pVM, idCpu, uVector);
    AssertRC(rc);
}


/**
 * Sends init IPI to the virtual CPU.
 *
 * @param   pVM         The cross context VM structure.
 * @param   idCpu       Virtual CPU to perform int IPI on.
 */
VMMR3_INT_DECL(void) VMMR3SendInitIpi(PVM pVM, VMCPUID idCpu)
{
    AssertReturnVoid(idCpu < pVM->cCpus);

    int rc = VMR3ReqCallNoWait(pVM, idCpu, (PFNRT)vmmR3SendInitIpi, 2, pVM, idCpu);
    AssertRC(rc);
}


/**
 * Registers the guest memory range that can be used for patching.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatchMem   Patch memory range.
 * @param   cbPatchMem  Size of the memory range.
 */
VMMR3DECL(int) VMMR3RegisterPatchMemory(PVM pVM, RTGCPTR pPatchMem, unsigned cbPatchMem)
{
    VM_ASSERT_EMT(pVM);
    if (HMIsEnabled(pVM))
        return HMR3EnablePatching(pVM, pPatchMem, cbPatchMem);

    return VERR_NOT_SUPPORTED;
}


/**
 * Deregisters the guest memory range that can be used for patching.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pPatchMem   Patch memory range.
 * @param   cbPatchMem  Size of the memory range.
 */
VMMR3DECL(int) VMMR3DeregisterPatchMemory(PVM pVM, RTGCPTR pPatchMem, unsigned cbPatchMem)
{
    if (HMIsEnabled(pVM))
        return HMR3DisablePatching(pVM, pPatchMem, cbPatchMem);

    return VINF_SUCCESS;
}


/**
 * Common recursion handler for the other EMTs.
 *
 * @returns Strict VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 * @param   rcStrict            Current status code to be combined with the one
 *                              from this recursion and returned.
 */
static VBOXSTRICTRC vmmR3EmtRendezvousCommonRecursion(PVM pVM, PVMCPU pVCpu, VBOXSTRICTRC rcStrict)
{
    int rc2;

    /*
     * We wait here while the initiator of this recursion reconfigures
     * everything.  The last EMT to get in signals the initiator.
     */
    if (ASMAtomicIncU32(&pVM->vmm.s.cRendezvousEmtsRecursingPush) == pVM->cCpus)
    {
        rc2 = RTSemEventSignal(pVM->vmm.s.hEvtRendezvousRecursionPushCaller);
        AssertLogRelRC(rc2);
    }

    rc2 = RTSemEventMultiWait(pVM->vmm.s.hEvtMulRendezvousRecursionPush, RT_INDEFINITE_WAIT);
    AssertLogRelRC(rc2);

    /*
     * Do the normal rendezvous processing.
     */
    VBOXSTRICTRC rcStrict2 = vmmR3EmtRendezvousCommon(pVM, pVCpu, false /* fIsCaller */, pVM->vmm.s.fRendezvousFlags,
                                                      pVM->vmm.s.pfnRendezvous, pVM->vmm.s.pvRendezvousUser);

    /*
     * Wait for the initiator to restore everything.
     */
    rc2 = RTSemEventMultiWait(pVM->vmm.s.hEvtMulRendezvousRecursionPop, RT_INDEFINITE_WAIT);
    AssertLogRelRC(rc2);

    /*
     * Last thread out of here signals the initiator.
     */
    if (ASMAtomicIncU32(&pVM->vmm.s.cRendezvousEmtsRecursingPop) == pVM->cCpus)
    {
        rc2 = RTSemEventSignal(pVM->vmm.s.hEvtRendezvousRecursionPopCaller);
        AssertLogRelRC(rc2);
    }

    /*
     * Merge status codes and return.
     */
    AssertRC(VBOXSTRICTRC_VAL(rcStrict2));
    if (    rcStrict2 != VINF_SUCCESS
        &&  (   rcStrict == VINF_SUCCESS
             || rcStrict > rcStrict2))
        rcStrict = rcStrict2;
    return rcStrict;
}


/**
 * Count returns and have the last non-caller EMT wake up the caller.
 *
 * @returns VBox strict informational status code for EM scheduling. No failures
 *          will be returned here, those are for the caller only.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   rcStrict            The current accumulated recursive status code,
 *                              to be merged with i32RendezvousStatus and
 *                              returned.
 */
DECL_FORCE_INLINE(VBOXSTRICTRC) vmmR3EmtRendezvousNonCallerReturn(PVM pVM, VBOXSTRICTRC rcStrict)
{
    VBOXSTRICTRC rcStrict2 = ASMAtomicReadS32(&pVM->vmm.s.i32RendezvousStatus);

    uint32_t cReturned = ASMAtomicIncU32(&pVM->vmm.s.cRendezvousEmtsReturned);
    if (cReturned == pVM->cCpus - 1U)
    {
        int rc = RTSemEventSignal(pVM->vmm.s.hEvtRendezvousDoneCaller);
        AssertLogRelRC(rc);
    }

    /*
     * Merge the status codes, ignoring error statuses in this code path.
     */
    AssertLogRelMsgReturn(   rcStrict2 <= VINF_SUCCESS
                          || (rcStrict2 >= VINF_EM_FIRST && rcStrict2 <= VINF_EM_LAST),
                          ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict2)),
                          VERR_IPE_UNEXPECTED_INFO_STATUS);

    if (RT_SUCCESS(rcStrict2))
    {
        if (    rcStrict2 != VINF_SUCCESS
            &&  (   rcStrict == VINF_SUCCESS
                 || rcStrict > rcStrict2))
            rcStrict = rcStrict2;
    }
    return rcStrict;
}


/**
 * Common worker for VMMR3EmtRendezvous and VMMR3EmtRendezvousFF.
 *
 * @returns VBox strict informational status code for EM scheduling. No failures
 *          will be returned here, those are for the caller only.  When
 *          fIsCaller is set, VINF_SUCCESS is always returned.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pVCpu               The cross context virtual CPU structure of the calling EMT.
 * @param   fIsCaller           Whether we're the VMMR3EmtRendezvous caller or
 *                              not.
 * @param   fFlags              The flags.
 * @param   pfnRendezvous       The callback.
 * @param   pvUser              The user argument for the callback.
 */
static VBOXSTRICTRC vmmR3EmtRendezvousCommon(PVM pVM, PVMCPU pVCpu, bool fIsCaller,
                                             uint32_t fFlags, PFNVMMEMTRENDEZVOUS pfnRendezvous, void *pvUser)
{
    int rc;
    VBOXSTRICTRC rcStrictRecursion = VINF_SUCCESS;

    /*
     * Enter, the last EMT triggers the next callback phase.
     */
    uint32_t cEntered = ASMAtomicIncU32(&pVM->vmm.s.cRendezvousEmtsEntered);
    if (cEntered != pVM->cCpus)
    {
        if ((fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ONE_BY_ONE)
        {
            /* Wait for our turn. */
            for (;;)
            {
                rc = RTSemEventWait(pVM->vmm.s.hEvtRendezvousEnterOneByOne, RT_INDEFINITE_WAIT);
                AssertLogRelRC(rc);
                if (!pVM->vmm.s.fRendezvousRecursion)
                    break;
                rcStrictRecursion = vmmR3EmtRendezvousCommonRecursion(pVM, pVCpu, rcStrictRecursion);
            }
        }
        else if ((fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ALL_AT_ONCE)
        {
            /* Wait for the last EMT to arrive and wake everyone up. */
            rc = RTSemEventMultiWait(pVM->vmm.s.hEvtMulRendezvousEnterAllAtOnce, RT_INDEFINITE_WAIT);
            AssertLogRelRC(rc);
            Assert(!pVM->vmm.s.fRendezvousRecursion);
        }
        else if (   (fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ASCENDING
                 || (fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_DESCENDING)
        {
            /* Wait for our turn. */
            for (;;)
            {
                rc = RTSemEventWait(pVM->vmm.s.pahEvtRendezvousEnterOrdered[pVCpu->idCpu], RT_INDEFINITE_WAIT);
                AssertLogRelRC(rc);
                if (!pVM->vmm.s.fRendezvousRecursion)
                    break;
                rcStrictRecursion = vmmR3EmtRendezvousCommonRecursion(pVM, pVCpu, rcStrictRecursion);
            }
        }
        else
        {
            Assert((fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE);

            /*
             * The execute once is handled specially to optimize the code flow.
             *
             * The last EMT to arrive will perform the callback and the other
             * EMTs will wait on the Done/DoneCaller semaphores (instead of
             * the EnterOneByOne/AllAtOnce) in the meanwhile. When the callback
             * returns, that EMT will initiate the normal return sequence.
             */
            if (!fIsCaller)
            {
                for (;;)
                {
                    rc = RTSemEventMultiWait(pVM->vmm.s.hEvtMulRendezvousDone, RT_INDEFINITE_WAIT);
                    AssertLogRelRC(rc);
                    if (!pVM->vmm.s.fRendezvousRecursion)
                        break;
                    rcStrictRecursion = vmmR3EmtRendezvousCommonRecursion(pVM, pVCpu, rcStrictRecursion);
                }

                return vmmR3EmtRendezvousNonCallerReturn(pVM, rcStrictRecursion);
            }
            return VINF_SUCCESS;
        }
    }
    else
    {
        /*
         * All EMTs are waiting, clear the FF and take action according to the
         * execution method.
         */
        VM_FF_CLEAR(pVM, VM_FF_EMT_RENDEZVOUS);

        if ((fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ALL_AT_ONCE)
        {
            /* Wake up everyone. */
            rc = RTSemEventMultiSignal(pVM->vmm.s.hEvtMulRendezvousEnterAllAtOnce);
            AssertLogRelRC(rc);
        }
        else if (   (fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ASCENDING
                 || (fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_DESCENDING)
        {
            /* Figure out who to wake up and wake it up. If it's ourself, then
               it's easy otherwise wait for our turn. */
            VMCPUID iFirst = (fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ASCENDING
                           ? 0
                           : pVM->cCpus - 1U;
            if (pVCpu->idCpu != iFirst)
            {
                rc = RTSemEventSignal(pVM->vmm.s.pahEvtRendezvousEnterOrdered[iFirst]);
                AssertLogRelRC(rc);
                for (;;)
                {
                    rc = RTSemEventWait(pVM->vmm.s.pahEvtRendezvousEnterOrdered[pVCpu->idCpu], RT_INDEFINITE_WAIT);
                    AssertLogRelRC(rc);
                    if (!pVM->vmm.s.fRendezvousRecursion)
                        break;
                    rcStrictRecursion = vmmR3EmtRendezvousCommonRecursion(pVM, pVCpu, rcStrictRecursion);
                }
            }
        }
        /* else: execute the handler on the current EMT and wake up one or more threads afterwards. */
    }


    /*
     * Do the callback and update the status if necessary.
     */
    if (    !(fFlags & VMMEMTRENDEZVOUS_FLAGS_STOP_ON_ERROR)
        ||  RT_SUCCESS(ASMAtomicUoReadS32(&pVM->vmm.s.i32RendezvousStatus)) )
    {
        VBOXSTRICTRC rcStrict2 = pfnRendezvous(pVM, pVCpu, pvUser);
        if (rcStrict2 != VINF_SUCCESS)
        {
            AssertLogRelMsg(   rcStrict2 <= VINF_SUCCESS
                            || (rcStrict2 >= VINF_EM_FIRST && rcStrict2 <= VINF_EM_LAST),
                            ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict2)));
            int32_t i32RendezvousStatus;
            do
            {
                i32RendezvousStatus = ASMAtomicUoReadS32(&pVM->vmm.s.i32RendezvousStatus);
                if (    rcStrict2 == i32RendezvousStatus
                    ||  RT_FAILURE(i32RendezvousStatus)
                    ||  (   i32RendezvousStatus != VINF_SUCCESS
                         && rcStrict2 > i32RendezvousStatus))
                    break;
            } while (!ASMAtomicCmpXchgS32(&pVM->vmm.s.i32RendezvousStatus, VBOXSTRICTRC_VAL(rcStrict2), i32RendezvousStatus));
        }
    }

    /*
     * Increment the done counter and take action depending on whether we're
     * the last to finish callback execution.
     */
    uint32_t cDone = ASMAtomicIncU32(&pVM->vmm.s.cRendezvousEmtsDone);
    if (    cDone != pVM->cCpus
        &&  (fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) != VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE)
    {
        /* Signal the next EMT? */
        if ((fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ONE_BY_ONE)
        {
            rc = RTSemEventSignal(pVM->vmm.s.hEvtRendezvousEnterOneByOne);
            AssertLogRelRC(rc);
        }
        else if ((fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ASCENDING)
        {
            Assert(cDone == pVCpu->idCpu + 1U);
            rc = RTSemEventSignal(pVM->vmm.s.pahEvtRendezvousEnterOrdered[pVCpu->idCpu + 1U]);
            AssertLogRelRC(rc);
        }
        else if ((fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_DESCENDING)
        {
            Assert(pVM->cCpus - cDone == pVCpu->idCpu);
            rc = RTSemEventSignal(pVM->vmm.s.pahEvtRendezvousEnterOrdered[pVM->cCpus - cDone - 1U]);
            AssertLogRelRC(rc);
        }

        /* Wait for the rest to finish (the caller waits on hEvtRendezvousDoneCaller). */
        if (!fIsCaller)
        {
            for (;;)
            {
                rc = RTSemEventMultiWait(pVM->vmm.s.hEvtMulRendezvousDone, RT_INDEFINITE_WAIT);
                AssertLogRelRC(rc);
                if (!pVM->vmm.s.fRendezvousRecursion)
                    break;
                rcStrictRecursion = vmmR3EmtRendezvousCommonRecursion(pVM, pVCpu, rcStrictRecursion);
            }
        }
    }
    else
    {
        /* Callback execution is all done, tell the rest to return. */
        rc = RTSemEventMultiSignal(pVM->vmm.s.hEvtMulRendezvousDone);
        AssertLogRelRC(rc);
    }

    if (!fIsCaller)
        return vmmR3EmtRendezvousNonCallerReturn(pVM, rcStrictRecursion);
    return rcStrictRecursion;
}


/**
 * Called in response to VM_FF_EMT_RENDEZVOUS.
 *
 * @returns VBox strict status code - EM scheduling.  No errors will be returned
 *          here, nor will any non-EM scheduling status codes be returned.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 *
 * @thread  EMT
 */
VMMR3_INT_DECL(int) VMMR3EmtRendezvousFF(PVM pVM, PVMCPU pVCpu)
{
    Assert(!pVCpu->vmm.s.fInRendezvous);
    Log(("VMMR3EmtRendezvousFF: EMT%#u\n", pVCpu->idCpu));
    pVCpu->vmm.s.fInRendezvous = true;
    VBOXSTRICTRC rcStrict = vmmR3EmtRendezvousCommon(pVM, pVCpu, false /* fIsCaller */, pVM->vmm.s.fRendezvousFlags,
                                                     pVM->vmm.s.pfnRendezvous, pVM->vmm.s.pvRendezvousUser);
    pVCpu->vmm.s.fInRendezvous = false;
    Log(("VMMR3EmtRendezvousFF: EMT%#u returns %Rrc\n", pVCpu->idCpu, VBOXSTRICTRC_VAL(rcStrict)));
    return VBOXSTRICTRC_TODO(rcStrict);
}


/**
 * Helper for resetting an single wakeup event sempahore.
 *
 * @returns VERR_TIMEOUT on success, RTSemEventWait status otherwise.
 * @param   hEvt        The event semaphore to reset.
 */
static int vmmR3HlpResetEvent(RTSEMEVENT hEvt)
{
    for (uint32_t cLoops = 0; ; cLoops++)
    {
        int rc = RTSemEventWait(hEvt, 0 /*cMsTimeout*/);
        if (rc != VINF_SUCCESS || cLoops > _4K)
            return rc;
    }
}


/**
 * Worker for VMMR3EmtRendezvous that handles recursion.
 *
 * @returns VBox strict status code.  This will be the first error,
 *          VINF_SUCCESS, or an EM scheduling status code.
 *
 * @param   pVM             The cross context VM structure.
 * @param   pVCpu           The cross context virtual CPU structure of the
 *                          calling EMT.
 * @param   fFlags          Flags indicating execution methods. See
 *                          grp_VMMR3EmtRendezvous_fFlags.
 * @param   pfnRendezvous   The callback.
 * @param   pvUser          User argument for the callback.
 *
 * @thread  EMT(pVCpu)
 */
static VBOXSTRICTRC vmmR3EmtRendezvousRecursive(PVM pVM, PVMCPU pVCpu, uint32_t fFlags,
                                                PFNVMMEMTRENDEZVOUS pfnRendezvous, void *pvUser)
{
    Log(("vmmR3EmtRendezvousRecursive: %#x EMT#%u depth=%d\n", fFlags, pVCpu->idCpu, pVM->vmm.s.cRendezvousRecursions));
    AssertLogRelReturn(pVM->vmm.s.cRendezvousRecursions < 3, VERR_DEADLOCK);
    Assert(pVCpu->vmm.s.fInRendezvous);

    /*
     * Save the current state.
     */
    uint32_t const              fParentFlags    = pVM->vmm.s.fRendezvousFlags;
    uint32_t const              cParentDone     = pVM->vmm.s.cRendezvousEmtsDone;
    int32_t const               iParentStatus   = pVM->vmm.s.i32RendezvousStatus;
    PFNVMMEMTRENDEZVOUS const   pfnParent       = pVM->vmm.s.pfnRendezvous;
    void * const                pvParentUser    = pVM->vmm.s.pvRendezvousUser;

    /*
     * Check preconditions and save the current state.
     */
    AssertReturn(   (fParentFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ASCENDING
                 || (fParentFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_DESCENDING
                 || (fParentFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ONE_BY_ONE
                 || (fParentFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE,
                 VERR_INTERNAL_ERROR);
    AssertReturn(pVM->vmm.s.cRendezvousEmtsEntered == pVM->cCpus, VERR_INTERNAL_ERROR_2);
    AssertReturn(pVM->vmm.s.cRendezvousEmtsReturned == 0, VERR_INTERNAL_ERROR_3);

    /*
     * Reset the recursion prep and pop semaphores.
     */
    int rc = RTSemEventMultiReset(pVM->vmm.s.hEvtMulRendezvousRecursionPush);
    AssertLogRelRCReturn(rc, rc);
    rc = RTSemEventMultiReset(pVM->vmm.s.hEvtMulRendezvousRecursionPop);
    AssertLogRelRCReturn(rc, rc);
    rc = vmmR3HlpResetEvent(pVM->vmm.s.hEvtRendezvousRecursionPushCaller);
    AssertLogRelMsgReturn(rc == VERR_TIMEOUT, ("%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS);
    rc = vmmR3HlpResetEvent(pVM->vmm.s.hEvtRendezvousRecursionPopCaller);
    AssertLogRelMsgReturn(rc == VERR_TIMEOUT, ("%Rrc\n", rc), RT_FAILURE_NP(rc) ? rc : VERR_IPE_UNEXPECTED_INFO_STATUS);

    /*
     * Usher the other thread into the recursion routine.
     */
    ASMAtomicWriteU32(&pVM->vmm.s.cRendezvousEmtsRecursingPush, 0);
    ASMAtomicWriteBool(&pVM->vmm.s.fRendezvousRecursion, true);

    uint32_t cLeft = pVM->cCpus - (cParentDone + 1U);
    if ((fParentFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ONE_BY_ONE)
        while (cLeft-- > 0)
        {
            rc = RTSemEventSignal(pVM->vmm.s.hEvtRendezvousEnterOneByOne);
            AssertLogRelRC(rc);
        }
    else if ((fParentFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ASCENDING)
    {
        Assert(cLeft == pVM->cCpus - (pVCpu->idCpu + 1U));
        for (VMCPUID iCpu = pVCpu->idCpu + 1U; iCpu < pVM->cCpus; iCpu++)
        {
            rc = RTSemEventSignal(pVM->vmm.s.pahEvtRendezvousEnterOrdered[iCpu]);
            AssertLogRelRC(rc);
        }
    }
    else if ((fParentFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_DESCENDING)
    {
        Assert(cLeft == pVCpu->idCpu);
        for (VMCPUID iCpu = pVCpu->idCpu; iCpu > 0; iCpu--)
        {
            rc = RTSemEventSignal(pVM->vmm.s.pahEvtRendezvousEnterOrdered[iCpu - 1U]);
            AssertLogRelRC(rc);
        }
    }
    else
        AssertLogRelReturn((fParentFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE,
                           VERR_INTERNAL_ERROR_4);

    rc = RTSemEventMultiSignal(pVM->vmm.s.hEvtMulRendezvousDone);
    AssertLogRelRC(rc);
    rc = RTSemEventSignal(pVM->vmm.s.hEvtRendezvousDoneCaller);
    AssertLogRelRC(rc);


    /*
     * Wait for the EMTs to wake up and get out of the parent rendezvous code.
     */
    if (ASMAtomicIncU32(&pVM->vmm.s.cRendezvousEmtsRecursingPush) != pVM->cCpus)
    {
        rc = RTSemEventWait(pVM->vmm.s.hEvtRendezvousRecursionPushCaller, RT_INDEFINITE_WAIT);
        AssertLogRelRC(rc);
    }

    ASMAtomicWriteBool(&pVM->vmm.s.fRendezvousRecursion, false);

    /*
     * Clear the slate and setup the new rendezvous.
     */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        rc = vmmR3HlpResetEvent(pVM->vmm.s.pahEvtRendezvousEnterOrdered[i]);
        AssertLogRelMsg(rc == VERR_TIMEOUT, ("%Rrc\n", rc));
    }
    rc = vmmR3HlpResetEvent(pVM->vmm.s.hEvtRendezvousEnterOneByOne);        AssertLogRelMsg(rc == VERR_TIMEOUT, ("%Rrc\n", rc));
    rc = RTSemEventMultiReset(pVM->vmm.s.hEvtMulRendezvousEnterAllAtOnce);  AssertLogRelRC(rc);
    rc = RTSemEventMultiReset(pVM->vmm.s.hEvtMulRendezvousDone);            AssertLogRelRC(rc);
    rc = vmmR3HlpResetEvent(pVM->vmm.s.hEvtRendezvousDoneCaller);           AssertLogRelMsg(rc == VERR_TIMEOUT, ("%Rrc\n", rc));

    ASMAtomicWriteU32(&pVM->vmm.s.cRendezvousEmtsEntered, 0);
    ASMAtomicWriteU32(&pVM->vmm.s.cRendezvousEmtsDone, 0);
    ASMAtomicWriteU32(&pVM->vmm.s.cRendezvousEmtsReturned, 0);
    ASMAtomicWriteS32(&pVM->vmm.s.i32RendezvousStatus, VINF_SUCCESS);
    ASMAtomicWritePtr((void * volatile *)&pVM->vmm.s.pfnRendezvous, (void *)(uintptr_t)pfnRendezvous);
    ASMAtomicWritePtr(&pVM->vmm.s.pvRendezvousUser, pvUser);
    ASMAtomicWriteU32(&pVM->vmm.s.fRendezvousFlags, fFlags);
    ASMAtomicIncU32(&pVM->vmm.s.cRendezvousRecursions);

    /*
     * We're ready to go now, do normal rendezvous processing.
     */
    rc = RTSemEventMultiSignal(pVM->vmm.s.hEvtMulRendezvousRecursionPush);
    AssertLogRelRC(rc);

    VBOXSTRICTRC rcStrict = vmmR3EmtRendezvousCommon(pVM, pVCpu, true /*fIsCaller*/, fFlags, pfnRendezvous, pvUser);

    /*
     * The caller waits for the other EMTs to be done, return and waiting on the
     * pop semaphore.
     */
    for (;;)
    {
        rc = RTSemEventWait(pVM->vmm.s.hEvtRendezvousDoneCaller, RT_INDEFINITE_WAIT);
        AssertLogRelRC(rc);
        if (!pVM->vmm.s.fRendezvousRecursion)
            break;
        rcStrict = vmmR3EmtRendezvousCommonRecursion(pVM, pVCpu, rcStrict);
    }

    /*
     * Get the return code and merge it with the above recursion status.
     */
    VBOXSTRICTRC rcStrict2 = pVM->vmm.s.i32RendezvousStatus;
    if (    rcStrict2 != VINF_SUCCESS
        &&  (   rcStrict == VINF_SUCCESS
             || rcStrict > rcStrict2))
        rcStrict = rcStrict2;

    /*
     * Restore the parent rendezvous state.
     */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        rc = vmmR3HlpResetEvent(pVM->vmm.s.pahEvtRendezvousEnterOrdered[i]);
        AssertLogRelMsg(rc == VERR_TIMEOUT, ("%Rrc\n", rc));
    }
    rc = vmmR3HlpResetEvent(pVM->vmm.s.hEvtRendezvousEnterOneByOne);        AssertLogRelMsg(rc == VERR_TIMEOUT, ("%Rrc\n", rc));
    rc = RTSemEventMultiReset(pVM->vmm.s.hEvtMulRendezvousEnterAllAtOnce);  AssertLogRelRC(rc);
    rc = RTSemEventMultiReset(pVM->vmm.s.hEvtMulRendezvousDone);            AssertLogRelRC(rc);
    rc = vmmR3HlpResetEvent(pVM->vmm.s.hEvtRendezvousDoneCaller);           AssertLogRelMsg(rc == VERR_TIMEOUT, ("%Rrc\n", rc));

    ASMAtomicWriteU32(&pVM->vmm.s.cRendezvousEmtsEntered,   pVM->cCpus);
    ASMAtomicWriteU32(&pVM->vmm.s.cRendezvousEmtsReturned,  0);
    ASMAtomicWriteU32(&pVM->vmm.s.cRendezvousEmtsDone,      cParentDone);
    ASMAtomicWriteS32(&pVM->vmm.s.i32RendezvousStatus,      iParentStatus);
    ASMAtomicWriteU32(&pVM->vmm.s.fRendezvousFlags,         fParentFlags);
    ASMAtomicWritePtr(&pVM->vmm.s.pvRendezvousUser,         pvParentUser);
    ASMAtomicWritePtr((void * volatile *)&pVM->vmm.s.pfnRendezvous, (void *)(uintptr_t)pfnParent);

    /*
     * Usher the other EMTs back to their parent recursion routine, waiting
     * for them to all get there before we return (makes sure they've been
     * scheduled and are past the pop event sem, see below).
     */
    ASMAtomicWriteU32(&pVM->vmm.s.cRendezvousEmtsRecursingPop, 0);
    rc = RTSemEventMultiSignal(pVM->vmm.s.hEvtMulRendezvousRecursionPop);
    AssertLogRelRC(rc);

    if (ASMAtomicIncU32(&pVM->vmm.s.cRendezvousEmtsRecursingPop) != pVM->cCpus)
    {
        rc = RTSemEventWait(pVM->vmm.s.hEvtRendezvousRecursionPopCaller, RT_INDEFINITE_WAIT);
        AssertLogRelRC(rc);
    }

    /*
     * We must reset the pop semaphore on the way out (doing the pop caller too,
     * just in case).  The parent may be another recursion.
     */
    rc = RTSemEventMultiReset(pVM->vmm.s.hEvtMulRendezvousRecursionPop);    AssertLogRelRC(rc);
    rc = vmmR3HlpResetEvent(pVM->vmm.s.hEvtRendezvousRecursionPopCaller);   AssertLogRelMsg(rc == VERR_TIMEOUT, ("%Rrc\n", rc));

    ASMAtomicDecU32(&pVM->vmm.s.cRendezvousRecursions);

    Log(("vmmR3EmtRendezvousRecursive: %#x EMT#%u depth=%d returns %Rrc\n",
         fFlags, pVCpu->idCpu, pVM->vmm.s.cRendezvousRecursions, VBOXSTRICTRC_VAL(rcStrict)));
    return rcStrict;
}


/**
 * EMT rendezvous.
 *
 * Gathers all the EMTs and execute some code on each of them, either in a one
 * by one fashion or all at once.
 *
 * @returns VBox strict status code.  This will be the first error,
 *          VINF_SUCCESS, or an EM scheduling status code.
 *
 * @retval  VERR_DEADLOCK if recursion is attempted using a rendezvous type that
 *          doesn't support it or if the recursion is too deep.
 *
 * @param   pVM             The cross context VM structure.
 * @param   fFlags          Flags indicating execution methods. See
 *                          grp_VMMR3EmtRendezvous_fFlags.  The one-by-one,
 *                          descending and ascending rendezvous types support
 *                          recursion from inside @a pfnRendezvous.
 * @param   pfnRendezvous   The callback.
 * @param   pvUser          User argument for the callback.
 *
 * @thread  Any.
 */
VMMR3DECL(int) VMMR3EmtRendezvous(PVM pVM, uint32_t fFlags, PFNVMMEMTRENDEZVOUS pfnRendezvous, void *pvUser)
{
    /*
     * Validate input.
     */
    AssertReturn(pVM, VERR_INVALID_VM_HANDLE);
    AssertMsg(   (fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) != VMMEMTRENDEZVOUS_FLAGS_TYPE_INVALID
              && (fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) <= VMMEMTRENDEZVOUS_FLAGS_TYPE_DESCENDING
              && !(fFlags & ~VMMEMTRENDEZVOUS_FLAGS_VALID_MASK), ("%#x\n", fFlags));
    AssertMsg(   !(fFlags & VMMEMTRENDEZVOUS_FLAGS_STOP_ON_ERROR)
              || (   (fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) != VMMEMTRENDEZVOUS_FLAGS_TYPE_ALL_AT_ONCE
                  && (fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) != VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE),
              ("type %u\n", fFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK));

    VBOXSTRICTRC rcStrict;
    PVMCPU pVCpu = VMMGetCpu(pVM);
    if (!pVCpu)
    {
        /*
         * Forward the request to an EMT thread.
         */
        Log(("VMMR3EmtRendezvous: %#x non-EMT\n", fFlags));
        if (!(fFlags & VMMEMTRENDEZVOUS_FLAGS_PRIORITY))
            rcStrict = VMR3ReqCallWait(pVM, VMCPUID_ANY, (PFNRT)VMMR3EmtRendezvous, 4, pVM, fFlags, pfnRendezvous, pvUser);
        else
            rcStrict = VMR3ReqPriorityCallWait(pVM, VMCPUID_ANY, (PFNRT)VMMR3EmtRendezvous, 4, pVM, fFlags, pfnRendezvous, pvUser);
        Log(("VMMR3EmtRendezvous: %#x non-EMT returns %Rrc\n", fFlags, VBOXSTRICTRC_VAL(rcStrict)));
    }
    else if (   pVM->cCpus == 1
             || (   pVM->enmVMState == VMSTATE_DESTROYING
                 && VMR3GetActiveEmts(pVM->pUVM) < pVM->cCpus ) )
    {
        /*
         * Shortcut for the single EMT case.
         *
         * We also ends up here if EMT(0) (or others) tries to issue a rendezvous
         * during vmR3Destroy after other emulation threads have started terminating.
         */
        if (!pVCpu->vmm.s.fInRendezvous)
        {
            Log(("VMMR3EmtRendezvous: %#x EMT (uni)\n", fFlags));
            pVCpu->vmm.s.fInRendezvous  = true;
            pVM->vmm.s.fRendezvousFlags = fFlags;
            rcStrict = pfnRendezvous(pVM, pVCpu, pvUser);
            pVCpu->vmm.s.fInRendezvous  = false;
        }
        else
        {
            /* Recursion. Do the same checks as in the SMP case. */
            Log(("VMMR3EmtRendezvous: %#x EMT (uni), recursion depth=%d\n", fFlags, pVM->vmm.s.cRendezvousRecursions));
            uint32_t fType = pVM->vmm.s.fRendezvousFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK;
            AssertLogRelReturn(   !pVCpu->vmm.s.fInRendezvous
                               || fType == VMMEMTRENDEZVOUS_FLAGS_TYPE_ASCENDING
                               || fType == VMMEMTRENDEZVOUS_FLAGS_TYPE_DESCENDING
                               || fType == VMMEMTRENDEZVOUS_FLAGS_TYPE_ONE_BY_ONE
                               || fType == VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE
                               , VERR_DEADLOCK);

            AssertLogRelReturn(pVM->vmm.s.cRendezvousRecursions < 3, VERR_DEADLOCK);
            pVM->vmm.s.cRendezvousRecursions++;
            uint32_t const fParentFlags = pVM->vmm.s.fRendezvousFlags;
            pVM->vmm.s.fRendezvousFlags = fFlags;

            rcStrict = pfnRendezvous(pVM, pVCpu, pvUser);

            pVM->vmm.s.fRendezvousFlags = fParentFlags;
            pVM->vmm.s.cRendezvousRecursions--;
        }
        Log(("VMMR3EmtRendezvous: %#x EMT (uni) returns %Rrc\n", fFlags, VBOXSTRICTRC_VAL(rcStrict)));
    }
    else
    {
        /*
         * Spin lock. If busy, check for recursion, if not recursing wait for
         * the other EMT to finish while keeping a lookout for the RENDEZVOUS FF.
         */
        int rc;
        rcStrict = VINF_SUCCESS;
        if (RT_UNLIKELY(!ASMAtomicCmpXchgU32(&pVM->vmm.s.u32RendezvousLock, 0x77778888, 0)))
        {
            /* Allow recursion in some cases. */
            if (   pVCpu->vmm.s.fInRendezvous
                && (   (pVM->vmm.s.fRendezvousFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ASCENDING
                    || (pVM->vmm.s.fRendezvousFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_DESCENDING
                    || (pVM->vmm.s.fRendezvousFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ONE_BY_ONE
                    || (pVM->vmm.s.fRendezvousFlags & VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK) == VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE
                       ))
                return VBOXSTRICTRC_TODO(vmmR3EmtRendezvousRecursive(pVM, pVCpu, fFlags, pfnRendezvous, pvUser));

            AssertLogRelMsgReturn(!pVCpu->vmm.s.fInRendezvous, ("fRendezvousFlags=%#x\n", pVM->vmm.s.fRendezvousFlags),
                                  VERR_DEADLOCK);

            Log(("VMMR3EmtRendezvous: %#x EMT#%u, waiting for lock...\n", fFlags, pVCpu->idCpu));
            while (!ASMAtomicCmpXchgU32(&pVM->vmm.s.u32RendezvousLock, 0x77778888, 0))
            {
                if (VM_FF_IS_PENDING(pVM, VM_FF_EMT_RENDEZVOUS))
                {
                    rc = VMMR3EmtRendezvousFF(pVM, pVCpu);
                    if (    rc != VINF_SUCCESS
                        &&  (   rcStrict == VINF_SUCCESS
                             || rcStrict > rc))
                        rcStrict = rc;
                    /** @todo Perhaps deal with termination here?  */
                }
                ASMNopPause();
            }
        }

        Log(("VMMR3EmtRendezvous: %#x EMT#%u\n", fFlags, pVCpu->idCpu));
        Assert(!VM_FF_IS_PENDING(pVM, VM_FF_EMT_RENDEZVOUS));
        Assert(!pVCpu->vmm.s.fInRendezvous);
        pVCpu->vmm.s.fInRendezvous = true;

        /*
         * Clear the slate and setup the rendezvous. This is a semaphore ping-pong orgy. :-)
         */
        for (VMCPUID i = 0; i < pVM->cCpus; i++)
        {
            rc = RTSemEventWait(pVM->vmm.s.pahEvtRendezvousEnterOrdered[i], 0);
            AssertLogRelMsg(rc == VERR_TIMEOUT || rc == VINF_SUCCESS, ("%Rrc\n", rc));
        }
        rc = RTSemEventWait(pVM->vmm.s.hEvtRendezvousEnterOneByOne, 0);         AssertLogRelMsg(rc == VERR_TIMEOUT || rc == VINF_SUCCESS, ("%Rrc\n", rc));
        rc = RTSemEventMultiReset(pVM->vmm.s.hEvtMulRendezvousEnterAllAtOnce);  AssertLogRelRC(rc);
        rc = RTSemEventMultiReset(pVM->vmm.s.hEvtMulRendezvousDone);            AssertLogRelRC(rc);
        rc = RTSemEventWait(pVM->vmm.s.hEvtRendezvousDoneCaller, 0);            AssertLogRelMsg(rc == VERR_TIMEOUT || rc == VINF_SUCCESS, ("%Rrc\n", rc));
        ASMAtomicWriteU32(&pVM->vmm.s.cRendezvousEmtsEntered, 0);
        ASMAtomicWriteU32(&pVM->vmm.s.cRendezvousEmtsDone, 0);
        ASMAtomicWriteU32(&pVM->vmm.s.cRendezvousEmtsReturned, 0);
        ASMAtomicWriteS32(&pVM->vmm.s.i32RendezvousStatus, VINF_SUCCESS);
        ASMAtomicWritePtr((void * volatile *)&pVM->vmm.s.pfnRendezvous, (void *)(uintptr_t)pfnRendezvous);
        ASMAtomicWritePtr(&pVM->vmm.s.pvRendezvousUser, pvUser);
        ASMAtomicWriteU32(&pVM->vmm.s.fRendezvousFlags, fFlags);

        /*
         * Set the FF and poke the other EMTs.
         */
        VM_FF_SET(pVM, VM_FF_EMT_RENDEZVOUS);
        VMR3NotifyGlobalFFU(pVM->pUVM, VMNOTIFYFF_FLAGS_POKE);

        /*
         * Do the same ourselves.
         */
        VBOXSTRICTRC rcStrict2 = vmmR3EmtRendezvousCommon(pVM, pVCpu, true /* fIsCaller */, fFlags, pfnRendezvous, pvUser);

        /*
         * The caller waits for the other EMTs to be done and return before doing
         * the cleanup. This makes away with wakeup / reset races we would otherwise
         * risk in the multiple release event semaphore code (hEvtRendezvousDoneCaller).
         */
        for (;;)
        {
            rc = RTSemEventWait(pVM->vmm.s.hEvtRendezvousDoneCaller, RT_INDEFINITE_WAIT);
            AssertLogRelRC(rc);
            if (!pVM->vmm.s.fRendezvousRecursion)
                break;
            rcStrict2 = vmmR3EmtRendezvousCommonRecursion(pVM, pVCpu, rcStrict2);
        }

        /*
         * Get the return code and clean up a little bit.
         */
        VBOXSTRICTRC rcStrict3 = pVM->vmm.s.i32RendezvousStatus;
        ASMAtomicWriteNullPtr((void * volatile *)&pVM->vmm.s.pfnRendezvous);

        ASMAtomicWriteU32(&pVM->vmm.s.u32RendezvousLock, 0);
        pVCpu->vmm.s.fInRendezvous = false;

        /*
         * Merge rcStrict, rcStrict2 and rcStrict3.
         */
        AssertRC(VBOXSTRICTRC_VAL(rcStrict));
        AssertRC(VBOXSTRICTRC_VAL(rcStrict2));
        if (    rcStrict2 != VINF_SUCCESS
            &&  (   rcStrict == VINF_SUCCESS
                 || rcStrict > rcStrict2))
            rcStrict = rcStrict2;
        if (    rcStrict3 != VINF_SUCCESS
            &&  (   rcStrict == VINF_SUCCESS
                 || rcStrict > rcStrict3))
            rcStrict = rcStrict3;
        Log(("VMMR3EmtRendezvous: %#x EMT#%u returns %Rrc\n", fFlags, pVCpu->idCpu, VBOXSTRICTRC_VAL(rcStrict)));
    }

    AssertLogRelMsgReturn(   rcStrict <= VINF_SUCCESS
                          || (rcStrict >= VINF_EM_FIRST && rcStrict <= VINF_EM_LAST),
                          ("%Rrc\n", VBOXSTRICTRC_VAL(rcStrict)),
                          VERR_IPE_UNEXPECTED_INFO_STATUS);
    return VBOXSTRICTRC_VAL(rcStrict);
}


/**
 * Read from the ring 0 jump buffer stack.
 *
 * @returns VBox status code.
 *
 * @param   pVM             The cross context VM structure.
 * @param   idCpu           The ID of the source CPU context (for the address).
 * @param   R0Addr          Where to start reading.
 * @param   pvBuf           Where to store the data we've read.
 * @param   cbRead          The number of bytes to read.
 */
VMMR3_INT_DECL(int) VMMR3ReadR0Stack(PVM pVM, VMCPUID idCpu, RTHCUINTPTR R0Addr, void *pvBuf, size_t cbRead)
{
    PVMCPU pVCpu = VMMGetCpuById(pVM, idCpu);
    AssertReturn(pVCpu, VERR_INVALID_PARAMETER);

#ifdef VMM_R0_SWITCH_STACK
    RTHCUINTPTR off = R0Addr - MMHyperCCToR0(pVM, pVCpu->vmm.s.pbEMTStackR3);
#else
    RTHCUINTPTR off = pVCpu->vmm.s.CallRing3JmpBufR0.cbSavedStack - (pVCpu->vmm.s.CallRing3JmpBufR0.SpCheck - R0Addr);
#endif
    if (   off          >  VMM_STACK_SIZE
        || off + cbRead >= VMM_STACK_SIZE)
        return VERR_INVALID_POINTER;

    memcpy(pvBuf, &pVCpu->vmm.s.pbEMTStackR3[off], cbRead);
    return VINF_SUCCESS;
}

#ifdef VBOX_WITH_RAW_MODE

/**
 * Calls a RC function.
 *
 * @param   pVM         The cross context VM structure.
 * @param   RCPtrEntry  The address of the RC function.
 * @param   cArgs       The number of arguments in the ....
 * @param   ...         Arguments to the function.
 */
VMMR3DECL(int) VMMR3CallRC(PVM pVM, RTRCPTR RCPtrEntry, unsigned cArgs, ...)
{
    va_list args;
    va_start(args, cArgs);
    int rc = VMMR3CallRCV(pVM, RCPtrEntry, cArgs, args);
    va_end(args);
    return rc;
}


/**
 * Calls a RC function.
 *
 * @param   pVM         The cross context VM structure.
 * @param   RCPtrEntry  The address of the RC function.
 * @param   cArgs       The number of arguments in the ....
 * @param   args        Arguments to the function.
 */
VMMR3DECL(int) VMMR3CallRCV(PVM pVM, RTRCPTR RCPtrEntry, unsigned cArgs, va_list args)
{
    /* Raw mode implies 1 VCPU. */
    AssertReturn(pVM->cCpus == 1, VERR_RAW_MODE_INVALID_SMP);
    PVMCPU pVCpu = &pVM->aCpus[0];

    Log2(("VMMR3CallGCV: RCPtrEntry=%RRv cArgs=%d\n", RCPtrEntry, cArgs));

    /*
     * Setup the call frame using the trampoline.
     */
    CPUMSetHyperState(pVCpu,
                      pVM->vmm.s.pfnCallTrampolineRC, /* eip */
                      pVCpu->vmm.s.pbEMTStackBottomRC - cArgs * sizeof(RTGCUINTPTR32),  /* esp */
                      RCPtrEntry,  /* eax */
                      cArgs        /* edx */
                      );

#if 0
    memset(pVCpu->vmm.s.pbEMTStackR3, 0xaa, VMM_STACK_SIZE); /* Clear the stack. */
#endif
    PRTGCUINTPTR32 pFrame = (PRTGCUINTPTR32)(pVCpu->vmm.s.pbEMTStackR3 + VMM_STACK_SIZE) - cArgs;
    int i = cArgs;
    while (i-- > 0)
        *pFrame++ = va_arg(args, RTGCUINTPTR32);

    CPUMPushHyper(pVCpu, cArgs * sizeof(RTGCUINTPTR32));                          /* stack frame size */
    CPUMPushHyper(pVCpu, RCPtrEntry);                                             /* what to call */

    /*
     * We hide log flushes (outer) and hypervisor interrupts (inner).
     */
    for (;;)
    {
        int rc;
        Assert(CPUMGetHyperCR3(pVCpu) && CPUMGetHyperCR3(pVCpu) == PGMGetHyperCR3(pVCpu));
        do
        {
#ifdef NO_SUPCALLR0VMM
            rc = VERR_GENERAL_FAILURE;
#else
            rc = SUPR3CallVMMR0Fast(pVM->pVMR0, VMMR0_DO_RAW_RUN, 0);
            if (RT_LIKELY(rc == VINF_SUCCESS))
                rc = pVCpu->vmm.s.iLastGZRc;
#endif
        } while (rc == VINF_EM_RAW_INTERRUPT_HYPER);

        /*
         * Flush the loggers.
         */
#ifdef LOG_ENABLED
        PRTLOGGERRC pLogger = pVM->vmm.s.pRCLoggerR3;
        if (    pLogger
            &&  pLogger->offScratch > 0)
            RTLogFlushRC(NULL, pLogger);
#endif
#ifdef VBOX_WITH_RC_RELEASE_LOGGING
        PRTLOGGERRC pRelLogger = pVM->vmm.s.pRCRelLoggerR3;
        if (RT_UNLIKELY(pRelLogger && pRelLogger->offScratch > 0))
            RTLogFlushRC(RTLogRelGetDefaultInstance(), pRelLogger);
#endif
        if (rc == VERR_TRPM_PANIC || rc == VERR_TRPM_DONT_PANIC)
            VMMR3FatalDump(pVM, pVCpu, rc);
        if (rc != VINF_VMM_CALL_HOST)
        {
            Log2(("VMMR3CallGCV: returns %Rrc (cs:eip=%04x:%08x)\n", rc, CPUMGetGuestCS(pVCpu), CPUMGetGuestEIP(pVCpu)));
            return rc;
        }
        rc = vmmR3ServiceCallRing3Request(pVM, pVCpu);
        if (RT_FAILURE(rc))
            return rc;
    }
}

#endif /* VBOX_WITH_RAW_MODE */

/**
 * Wrapper for SUPR3CallVMMR0Ex which will deal with VINF_VMM_CALL_HOST returns.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   uOperation  Operation to execute.
 * @param   u64Arg      Constant argument.
 * @param   pReqHdr     Pointer to a request header. See SUPR3CallVMMR0Ex for
 *                      details.
 */
VMMR3DECL(int) VMMR3CallR0(PVM pVM, uint32_t uOperation, uint64_t u64Arg, PSUPVMMR0REQHDR pReqHdr)
{
    PVMCPU pVCpu = VMMGetCpu(pVM);
    AssertReturn(pVCpu, VERR_VM_THREAD_NOT_EMT);

    /*
     * Call Ring-0 entry with init code.
     */
    int rc;
    for (;;)
    {
#ifdef NO_SUPCALLR0VMM
        rc = VERR_GENERAL_FAILURE;
#else
        rc = SUPR3CallVMMR0Ex(pVM->pVMR0, pVCpu->idCpu, uOperation, u64Arg, pReqHdr);
#endif
        /*
         * Flush the logs.
         */
#ifdef LOG_ENABLED
        if (    pVCpu->vmm.s.pR0LoggerR3
            &&  pVCpu->vmm.s.pR0LoggerR3->Logger.offScratch > 0)
            RTLogFlushR0(NULL, &pVCpu->vmm.s.pR0LoggerR3->Logger);
#endif
        if (rc != VINF_VMM_CALL_HOST)
            break;
        rc = vmmR3ServiceCallRing3Request(pVM, pVCpu);
        if (RT_FAILURE(rc) || (rc >= VINF_EM_FIRST && rc <= VINF_EM_LAST))
            break;
        /* Resume R0 */
    }

    AssertLogRelMsgReturn(rc == VINF_SUCCESS || RT_FAILURE(rc),
                          ("uOperation=%u rc=%Rrc\n", uOperation, rc),
                          VERR_IPE_UNEXPECTED_INFO_STATUS);
    return rc;
}


#ifdef VBOX_WITH_RAW_MODE
/**
 * Resumes executing hypervisor code when interrupted by a queue flush or a
 * debug event.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR3DECL(int) VMMR3ResumeHyper(PVM pVM, PVMCPU pVCpu)
{
    Log(("VMMR3ResumeHyper: eip=%RRv esp=%RRv\n", CPUMGetHyperEIP(pVCpu), CPUMGetHyperESP(pVCpu)));
    AssertReturn(pVM->cCpus == 1, VERR_RAW_MODE_INVALID_SMP);

    /*
     * We hide log flushes (outer) and hypervisor interrupts (inner).
     */
    for (;;)
    {
        int rc;
        Assert(CPUMGetHyperCR3(pVCpu) && CPUMGetHyperCR3(pVCpu) == PGMGetHyperCR3(pVCpu));
        do
        {
# ifdef NO_SUPCALLR0VMM
            rc = VERR_GENERAL_FAILURE;
# else
            rc = SUPR3CallVMMR0Fast(pVM->pVMR0, VMMR0_DO_RAW_RUN, 0);
            if (RT_LIKELY(rc == VINF_SUCCESS))
                rc = pVCpu->vmm.s.iLastGZRc;
# endif
        } while (rc == VINF_EM_RAW_INTERRUPT_HYPER);

        /*
         * Flush the loggers.
         */
# ifdef LOG_ENABLED
        PRTLOGGERRC pLogger = pVM->vmm.s.pRCLoggerR3;
        if (    pLogger
            &&  pLogger->offScratch > 0)
            RTLogFlushRC(NULL, pLogger);
# endif
# ifdef VBOX_WITH_RC_RELEASE_LOGGING
        PRTLOGGERRC pRelLogger = pVM->vmm.s.pRCRelLoggerR3;
        if (RT_UNLIKELY(pRelLogger && pRelLogger->offScratch > 0))
            RTLogFlushRC(RTLogRelGetDefaultInstance(), pRelLogger);
# endif
        if (rc == VERR_TRPM_PANIC || rc == VERR_TRPM_DONT_PANIC)
            VMMR3FatalDump(pVM, pVCpu, rc);
        if (rc != VINF_VMM_CALL_HOST)
        {
            Log(("VMMR3ResumeHyper: returns %Rrc\n", rc));
            return rc;
        }
        rc = vmmR3ServiceCallRing3Request(pVM, pVCpu);
        if (RT_FAILURE(rc))
            return rc;
    }
}
#endif /* VBOX_WITH_RAW_MODE */


/**
 * Service a call to the ring-3 host code.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure.
 * @remarks Careful with critsects.
 */
static int vmmR3ServiceCallRing3Request(PVM pVM, PVMCPU pVCpu)
{
    /*
     * We must also check for pending critsect exits or else we can deadlock
     * when entering other critsects here.
     */
    if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_PDM_CRITSECT))
        PDMCritSectBothFF(pVCpu);

    switch (pVCpu->vmm.s.enmCallRing3Operation)
    {
        /*
         * Acquire a critical section.
         */
        case VMMCALLRING3_PDM_CRIT_SECT_ENTER:
        {
            pVCpu->vmm.s.rcCallRing3 = PDMR3CritSectEnterEx((PPDMCRITSECT)(uintptr_t)pVCpu->vmm.s.u64CallRing3Arg,
                                                            true /*fCallRing3*/);
            break;
        }

        /*
         * Enter a r/w critical section exclusively.
         */
        case VMMCALLRING3_PDM_CRIT_SECT_RW_ENTER_EXCL:
        {
            pVCpu->vmm.s.rcCallRing3 = PDMR3CritSectRwEnterExclEx((PPDMCRITSECTRW)(uintptr_t)pVCpu->vmm.s.u64CallRing3Arg,
                                                                    true /*fCallRing3*/);
            break;
        }

        /*
         * Enter a r/w critical section shared.
         */
        case VMMCALLRING3_PDM_CRIT_SECT_RW_ENTER_SHARED:
        {
            pVCpu->vmm.s.rcCallRing3 = PDMR3CritSectRwEnterSharedEx((PPDMCRITSECTRW)(uintptr_t)pVCpu->vmm.s.u64CallRing3Arg,
                                                                    true /*fCallRing3*/);
            break;
        }

        /*
         * Acquire the PDM lock.
         */
        case VMMCALLRING3_PDM_LOCK:
        {
            pVCpu->vmm.s.rcCallRing3 = PDMR3LockCall(pVM);
            break;
        }

        /*
         * Grow the PGM pool.
         */
        case VMMCALLRING3_PGM_POOL_GROW:
        {
            pVCpu->vmm.s.rcCallRing3 = PGMR3PoolGrow(pVM);
            break;
        }

        /*
         * Maps an page allocation chunk into ring-3 so ring-0 can use it.
         */
        case VMMCALLRING3_PGM_MAP_CHUNK:
        {
            pVCpu->vmm.s.rcCallRing3 = PGMR3PhysChunkMap(pVM, pVCpu->vmm.s.u64CallRing3Arg);
            break;
        }

        /*
         * Allocates more handy pages.
         */
        case VMMCALLRING3_PGM_ALLOCATE_HANDY_PAGES:
        {
            pVCpu->vmm.s.rcCallRing3 = PGMR3PhysAllocateHandyPages(pVM);
            break;
        }

        /*
         * Allocates a large page.
         */
        case VMMCALLRING3_PGM_ALLOCATE_LARGE_HANDY_PAGE:
        {
            pVCpu->vmm.s.rcCallRing3 = PGMR3PhysAllocateLargeHandyPage(pVM, pVCpu->vmm.s.u64CallRing3Arg);
            break;
        }

        /*
         * Acquire the PGM lock.
         */
        case VMMCALLRING3_PGM_LOCK:
        {
            pVCpu->vmm.s.rcCallRing3 = PGMR3LockCall(pVM);
            break;
        }

        /*
         * Acquire the MM hypervisor heap lock.
         */
        case VMMCALLRING3_MMHYPER_LOCK:
        {
            pVCpu->vmm.s.rcCallRing3 = MMR3LockCall(pVM);
            break;
        }

#ifdef VBOX_WITH_REM
        /*
         * Flush REM handler notifications.
         */
        case VMMCALLRING3_REM_REPLAY_HANDLER_NOTIFICATIONS:
        {
            REMR3ReplayHandlerNotifications(pVM);
            pVCpu->vmm.s.rcCallRing3 = VINF_SUCCESS;
            break;
        }
#endif

        /*
         * This is a noop. We just take this route to avoid unnecessary
         * tests in the loops.
         */
        case VMMCALLRING3_VMM_LOGGER_FLUSH:
            pVCpu->vmm.s.rcCallRing3 = VINF_SUCCESS;
            LogAlways(("*FLUSH*\n"));
            break;

        /*
         * Set the VM error message.
         */
        case VMMCALLRING3_VM_SET_ERROR:
            VMR3SetErrorWorker(pVM);
            pVCpu->vmm.s.rcCallRing3 = VINF_SUCCESS;
            break;

        /*
         * Set the VM runtime error message.
         */
        case VMMCALLRING3_VM_SET_RUNTIME_ERROR:
            pVCpu->vmm.s.rcCallRing3 = VMR3SetRuntimeErrorWorker(pVM);
            break;

        /*
         * Signal a ring 0 hypervisor assertion.
         * Cancel the longjmp operation that's in progress.
         */
        case VMMCALLRING3_VM_R0_ASSERTION:
            pVCpu->vmm.s.enmCallRing3Operation = VMMCALLRING3_INVALID;
            pVCpu->vmm.s.CallRing3JmpBufR0.fInRing3Call = false;
#ifdef RT_ARCH_X86
            pVCpu->vmm.s.CallRing3JmpBufR0.eip = 0;
#else
            pVCpu->vmm.s.CallRing3JmpBufR0.rip = 0;
#endif
#ifdef VMM_R0_SWITCH_STACK
            *(uint64_t *)pVCpu->vmm.s.pbEMTStackR3 = 0; /* clear marker  */
#endif
            LogRel(("%s", pVM->vmm.s.szRing0AssertMsg1));
            LogRel(("%s", pVM->vmm.s.szRing0AssertMsg2));
            return VERR_VMM_RING0_ASSERTION;

        /*
         * A forced switch to ring 0 for preemption purposes.
         */
        case VMMCALLRING3_VM_R0_PREEMPT:
            pVCpu->vmm.s.rcCallRing3 = VINF_SUCCESS;
            break;

        case VMMCALLRING3_FTM_SET_CHECKPOINT:
            pVCpu->vmm.s.rcCallRing3 = FTMR3SetCheckpoint(pVM, (FTMCHECKPOINTTYPE)pVCpu->vmm.s.u64CallRing3Arg);
            break;

        default:
            AssertMsgFailed(("enmCallRing3Operation=%d\n", pVCpu->vmm.s.enmCallRing3Operation));
            return VERR_VMM_UNKNOWN_RING3_CALL;
    }

    pVCpu->vmm.s.enmCallRing3Operation = VMMCALLRING3_INVALID;
    return VINF_SUCCESS;
}


/**
 * Displays the Force action Flags.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pHlp        The output helpers.
 * @param   pszArgs     The additional arguments (ignored).
 */
static DECLCALLBACK(void) vmmR3InfoFF(PVM pVM, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    int         c;
    uint32_t    f;
    NOREF(pszArgs);

#define PRINT_FLAG(prf,flag) do { \
        if (f & (prf##flag)) \
        { \
            static const char *s_psz = #flag; \
            if (!(c % 6)) \
                pHlp->pfnPrintf(pHlp, "%s\n    %s", c ? "," : "", s_psz); \
            else \
                pHlp->pfnPrintf(pHlp, ", %s", s_psz); \
            c++; \
            f &= ~(prf##flag); \
        } \
    } while (0)

#define PRINT_GROUP(prf,grp,sfx) do { \
        if (f & (prf##grp##sfx)) \
        { \
            static const char *s_psz = #grp; \
            if (!(c % 5)) \
                pHlp->pfnPrintf(pHlp, "%s    %s", c ? ",\n" : "  Groups:\n", s_psz); \
            else \
                pHlp->pfnPrintf(pHlp, ", %s", s_psz); \
            c++; \
        } \
    } while (0)

    /*
     * The global flags.
     */
    const uint32_t fGlobalForcedActions = pVM->fGlobalForcedActions;
    pHlp->pfnPrintf(pHlp, "Global FFs: %#RX32", fGlobalForcedActions);

    /* show the flag mnemonics  */
    c = 0;
    f = fGlobalForcedActions;
    PRINT_FLAG(VM_FF_,TM_VIRTUAL_SYNC);
    PRINT_FLAG(VM_FF_,PDM_QUEUES);
    PRINT_FLAG(VM_FF_,PDM_DMA);
    PRINT_FLAG(VM_FF_,DBGF);
    PRINT_FLAG(VM_FF_,REQUEST);
    PRINT_FLAG(VM_FF_,CHECK_VM_STATE);
    PRINT_FLAG(VM_FF_,RESET);
    PRINT_FLAG(VM_FF_,EMT_RENDEZVOUS);
    PRINT_FLAG(VM_FF_,PGM_NEED_HANDY_PAGES);
    PRINT_FLAG(VM_FF_,PGM_NO_MEMORY);
    PRINT_FLAG(VM_FF_,PGM_POOL_FLUSH_PENDING);
    PRINT_FLAG(VM_FF_,REM_HANDLER_NOTIFY);
    PRINT_FLAG(VM_FF_,DEBUG_SUSPEND);
    if (f)
        pHlp->pfnPrintf(pHlp, "%s\n    Unknown bits: %#RX32\n", c ? "," : "", f);
    else
        pHlp->pfnPrintf(pHlp, "\n");

    /* the groups */
    c = 0;
    f = fGlobalForcedActions;
    PRINT_GROUP(VM_FF_,EXTERNAL_SUSPENDED,_MASK);
    PRINT_GROUP(VM_FF_,EXTERNAL_HALTED,_MASK);
    PRINT_GROUP(VM_FF_,HIGH_PRIORITY_PRE,_MASK);
    PRINT_GROUP(VM_FF_,HIGH_PRIORITY_PRE_RAW,_MASK);
    PRINT_GROUP(VM_FF_,HIGH_PRIORITY_POST,_MASK);
    PRINT_GROUP(VM_FF_,NORMAL_PRIORITY_POST,_MASK);
    PRINT_GROUP(VM_FF_,NORMAL_PRIORITY,_MASK);
    PRINT_GROUP(VM_FF_,ALL_REM,_MASK);
    if (c)
        pHlp->pfnPrintf(pHlp, "\n");

    /*
     * Per CPU flags.
     */
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        const uint32_t fLocalForcedActions = pVM->aCpus[i].fLocalForcedActions;
        pHlp->pfnPrintf(pHlp, "CPU %u FFs: %#RX32", i, fLocalForcedActions);

        /* show the flag mnemonics */
        c = 0;
        f = fLocalForcedActions;
        PRINT_FLAG(VMCPU_FF_,INTERRUPT_APIC);
        PRINT_FLAG(VMCPU_FF_,INTERRUPT_PIC);
        PRINT_FLAG(VMCPU_FF_,TIMER);
        PRINT_FLAG(VMCPU_FF_,INTERRUPT_NMI);
        PRINT_FLAG(VMCPU_FF_,INTERRUPT_SMI);
        PRINT_FLAG(VMCPU_FF_,PDM_CRITSECT);
        PRINT_FLAG(VMCPU_FF_,UNHALT);
        PRINT_FLAG(VMCPU_FF_,IEM);
        PRINT_FLAG(VMCPU_FF_,UPDATE_APIC);
        PRINT_FLAG(VMCPU_FF_,DBGF);
        PRINT_FLAG(VMCPU_FF_,REQUEST);
        PRINT_FLAG(VMCPU_FF_,HM_UPDATE_CR3);
        PRINT_FLAG(VMCPU_FF_,HM_UPDATE_PAE_PDPES);
        PRINT_FLAG(VMCPU_FF_,PGM_SYNC_CR3);
        PRINT_FLAG(VMCPU_FF_,PGM_SYNC_CR3_NON_GLOBAL);
        PRINT_FLAG(VMCPU_FF_,TLB_FLUSH);
        PRINT_FLAG(VMCPU_FF_,INHIBIT_INTERRUPTS);
        PRINT_FLAG(VMCPU_FF_,BLOCK_NMIS);
        PRINT_FLAG(VMCPU_FF_,TO_R3);
        PRINT_FLAG(VMCPU_FF_,IOM);
#ifdef VBOX_WITH_RAW_MODE
        PRINT_FLAG(VMCPU_FF_,TRPM_SYNC_IDT);
        PRINT_FLAG(VMCPU_FF_,SELM_SYNC_TSS);
        PRINT_FLAG(VMCPU_FF_,SELM_SYNC_GDT);
        PRINT_FLAG(VMCPU_FF_,SELM_SYNC_LDT);
        PRINT_FLAG(VMCPU_FF_,CSAM_SCAN_PAGE);
        PRINT_FLAG(VMCPU_FF_,CSAM_PENDING_ACTION);
        PRINT_FLAG(VMCPU_FF_,CPUM);
#endif
        if (f)
            pHlp->pfnPrintf(pHlp, "%s\n    Unknown bits: %#RX32\n", c ? "," : "", f);
        else
            pHlp->pfnPrintf(pHlp, "\n");

        if (fLocalForcedActions & VMCPU_FF_INHIBIT_INTERRUPTS)
            pHlp->pfnPrintf(pHlp, "    intr inhibit RIP: %RGp\n", EMGetInhibitInterruptsPC(&pVM->aCpus[i]));

        /* the groups */
        c = 0;
        f = fLocalForcedActions;
        PRINT_GROUP(VMCPU_FF_,EXTERNAL_SUSPENDED,_MASK);
        PRINT_GROUP(VMCPU_FF_,EXTERNAL_HALTED,_MASK);
        PRINT_GROUP(VMCPU_FF_,HIGH_PRIORITY_PRE,_MASK);
        PRINT_GROUP(VMCPU_FF_,HIGH_PRIORITY_PRE_RAW,_MASK);
        PRINT_GROUP(VMCPU_FF_,HIGH_PRIORITY_POST,_MASK);
        PRINT_GROUP(VMCPU_FF_,NORMAL_PRIORITY_POST,_MASK);
        PRINT_GROUP(VMCPU_FF_,NORMAL_PRIORITY,_MASK);
        PRINT_GROUP(VMCPU_FF_,RESUME_GUEST,_MASK);
        PRINT_GROUP(VMCPU_FF_,HM_TO_R3,_MASK);
        PRINT_GROUP(VMCPU_FF_,ALL_REM,_MASK);
        if (c)
            pHlp->pfnPrintf(pHlp, "\n");
    }

#undef PRINT_FLAG
#undef PRINT_GROUP
}

