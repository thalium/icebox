/* $Id: VMMR0.cpp $ */
/** @file
 * VMM - Host Context Ring 0.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_VMM
#include <VBox/vmm/vmm.h>
#include <VBox/sup.h>
#include <VBox/vmm/trpm.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/tm.h>
#include "VMMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/gvm.h>
#ifdef VBOX_WITH_PCI_PASSTHROUGH
# include <VBox/vmm/pdmpci.h>
#endif
#include <VBox/vmm/apic.h>

#include <VBox/vmm/gvmm.h>
#include <VBox/vmm/gmm.h>
#include <VBox/vmm/gim.h>
#include <VBox/intnet.h>
#include <VBox/vmm/hm.h>
#include <VBox/param.h>
#include <VBox/err.h>
#include <VBox/version.h>
#include <VBox/log.h>

#include <iprt/asm-amd64-x86.h>
#include <iprt/assert.h>
#include <iprt/crc.h>
#include <iprt/mp.h>
#include <iprt/once.h>
#include <iprt/stdarg.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/timer.h>

#include "dtrace/VBoxVMM.h"


#if defined(_MSC_VER) && defined(RT_ARCH_AMD64) /** @todo check this with with VC7! */
#  pragma intrinsic(_AddressOfReturnAddress)
#endif

#if defined(RT_OS_DARWIN) && ARCH_BITS == 32
# error "32-bit darwin is no longer supported. Go back to 4.3 or earlier!"
#endif



/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** @def VMM_CHECK_SMAP_SETUP
 * SMAP check setup. */
/** @def VMM_CHECK_SMAP_CHECK
 * Checks that the AC flag is set if SMAP is enabled. If AC is not set,
 * it will be logged and @a a_BadExpr is executed. */
/** @def VMM_CHECK_SMAP_CHECK2
 * Checks that the AC flag is set if SMAP is enabled.  If AC is not set, it will
 * be logged, written to the VMs assertion text buffer, and @a a_BadExpr is
 * executed. */
#if defined(VBOX_STRICT) || 1
# define VMM_CHECK_SMAP_SETUP() uint32_t const fKernelFeatures = SUPR0GetKernelFeatures()
# define VMM_CHECK_SMAP_CHECK(a_BadExpr) \
    do { \
        if (fKernelFeatures & SUPKERNELFEATURES_SMAP) \
        { \
            RTCCUINTREG fEflCheck = ASMGetFlags(); \
            if (RT_LIKELY(fEflCheck & X86_EFL_AC)) \
            { /* likely */ } \
            else \
            { \
                SUPR0Printf("%s, line %d: EFLAGS.AC is clear! (%#x)\n", __FUNCTION__, __LINE__, (uint32_t)fEflCheck); \
                a_BadExpr; \
            } \
        } \
    } while (0)
# define VMM_CHECK_SMAP_CHECK2(a_pVM, a_BadExpr) \
    do { \
        if (fKernelFeatures & SUPKERNELFEATURES_SMAP) \
        { \
            RTCCUINTREG fEflCheck = ASMGetFlags(); \
            if (RT_LIKELY(fEflCheck & X86_EFL_AC)) \
            { /* likely */ } \
            else \
            { \
                SUPR0BadContext((a_pVM) ? (a_pVM)->pSession : NULL, __FILE__, __LINE__, "EFLAGS.AC is zero!"); \
                RTStrPrintf(pVM->vmm.s.szRing0AssertMsg1, sizeof(pVM->vmm.s.szRing0AssertMsg1), \
                            "%s, line %d: EFLAGS.AC is clear! (%#x)\n", __FUNCTION__, __LINE__, (uint32_t)fEflCheck); \
                a_BadExpr; \
            } \
        } \
    } while (0)
#else
# define VMM_CHECK_SMAP_SETUP()            uint32_t const fKernelFeatures = 0
# define VMM_CHECK_SMAP_CHECK(a_BadExpr)            NOREF(fKernelFeatures)
# define VMM_CHECK_SMAP_CHECK2(a_pVM, a_BadExpr)    NOREF(fKernelFeatures)
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
#if defined(RT_ARCH_X86) && (defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD))
extern uint64_t __udivdi3(uint64_t, uint64_t);
extern uint64_t __umoddi3(uint64_t, uint64_t);
#endif
RT_C_DECLS_END


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Drag in necessary library bits.
 * The runtime lives here (in VMMR0.r0) and VBoxDD*R0.r0 links against us. */
PFNRT g_VMMR0Deps[] =
{
    (PFNRT)RTCrc32,
    (PFNRT)RTOnce,
#if defined(RT_ARCH_X86) && (defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD))
    (PFNRT)__udivdi3,
    (PFNRT)__umoddi3,
#endif
    NULL
};

#ifdef RT_OS_SOLARIS
/* Dependency information for the native solaris loader. */
extern "C" { char _depends_on[] = "vboxdrv"; }
#endif

/** The result of SUPR0GetRawModeUsability(), set by ModuleInit(). */
int g_rcRawModeUsability = VINF_SUCCESS;


/**
 * Initialize the module.
 * This is called when we're first loaded.
 *
 * @returns 0 on success.
 * @returns VBox status on failure.
 * @param   hMod        Image handle for use in APIs.
 */
DECLEXPORT(int) ModuleInit(void *hMod)
{
    VMM_CHECK_SMAP_SETUP();
    VMM_CHECK_SMAP_CHECK(RT_NOTHING);

#ifdef VBOX_WITH_DTRACE_R0
    /*
     * The first thing to do is register the static tracepoints.
     * (Deregistration is automatic.)
     */
    int rc2 = SUPR0TracerRegisterModule(hMod, &g_VTGObjHeader);
    if (RT_FAILURE(rc2))
        return rc2;
#endif
    LogFlow(("ModuleInit:\n"));

#ifdef VBOX_WITH_64ON32_CMOS_DEBUG
    /*
     * Display the CMOS debug code.
     */
    ASMOutU8(0x72, 0x03);
    uint8_t bDebugCode = ASMInU8(0x73);
    LogRel(("CMOS Debug Code: %#x (%d)\n", bDebugCode, bDebugCode));
    RTLogComPrintf("CMOS Debug Code: %#x (%d)\n", bDebugCode, bDebugCode);
#endif

    /*
     * Initialize the VMM, GVMM, GMM, HM, PGM (Darwin) and INTNET.
     */
    int rc = vmmInitFormatTypes();
    if (RT_SUCCESS(rc))
    {
        VMM_CHECK_SMAP_CHECK(RT_NOTHING);
        rc = GVMMR0Init();
        if (RT_SUCCESS(rc))
        {
            VMM_CHECK_SMAP_CHECK(RT_NOTHING);
            rc = GMMR0Init();
            if (RT_SUCCESS(rc))
            {
                VMM_CHECK_SMAP_CHECK(RT_NOTHING);
                rc = HMR0Init();
                if (RT_SUCCESS(rc))
                {
                    VMM_CHECK_SMAP_CHECK(RT_NOTHING);
                    rc = PGMRegisterStringFormatTypes();
                    if (RT_SUCCESS(rc))
                    {
                        VMM_CHECK_SMAP_CHECK(RT_NOTHING);
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
                        rc = PGMR0DynMapInit();
#endif
                        if (RT_SUCCESS(rc))
                        {
                            VMM_CHECK_SMAP_CHECK(RT_NOTHING);
                            rc = IntNetR0Init();
                            if (RT_SUCCESS(rc))
                            {
#ifdef VBOX_WITH_PCI_PASSTHROUGH
                                VMM_CHECK_SMAP_CHECK(RT_NOTHING);
                                rc = PciRawR0Init();
#endif
                                if (RT_SUCCESS(rc))
                                {
                                    VMM_CHECK_SMAP_CHECK(RT_NOTHING);
                                    rc = CPUMR0ModuleInit();
                                    if (RT_SUCCESS(rc))
                                    {
#ifdef VBOX_WITH_TRIPLE_FAULT_HACK
                                        VMM_CHECK_SMAP_CHECK(RT_NOTHING);
                                        rc = vmmR0TripleFaultHackInit();
                                        if (RT_SUCCESS(rc))
#endif
                                        {
                                            VMM_CHECK_SMAP_CHECK(rc = VERR_VMM_SMAP_BUT_AC_CLEAR);
                                            if (RT_SUCCESS(rc))
                                            {
                                                g_rcRawModeUsability = SUPR0GetRawModeUsability();
                                                if (g_rcRawModeUsability != VINF_SUCCESS)
                                                    SUPR0Printf("VMMR0!ModuleInit: SUPR0GetRawModeUsability -> %Rrc\n",
                                                                g_rcRawModeUsability);
                                                LogFlow(("ModuleInit: returns success\n"));
                                                return VINF_SUCCESS;
                                            }
                                        }

                                        /*
                                         * Bail out.
                                         */
#ifdef VBOX_WITH_TRIPLE_FAULT_HACK
                                        vmmR0TripleFaultHackTerm();
#endif
                                    }
                                    else
                                        LogRel(("ModuleInit: CPUMR0ModuleInit -> %Rrc\n", rc));
#ifdef VBOX_WITH_PCI_PASSTHROUGH
                                    PciRawR0Term();
#endif
                                }
                                else
                                    LogRel(("ModuleInit: PciRawR0Init -> %Rrc\n", rc));
                                IntNetR0Term();
                            }
                            else
                                LogRel(("ModuleInit: IntNetR0Init -> %Rrc\n", rc));
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
                            PGMR0DynMapTerm();
#endif
                        }
                        else
                            LogRel(("ModuleInit: PGMR0DynMapInit -> %Rrc\n", rc));
                        PGMDeregisterStringFormatTypes();
                    }
                    else
                        LogRel(("ModuleInit: PGMRegisterStringFormatTypes -> %Rrc\n", rc));
                    HMR0Term();
                }
                else
                    LogRel(("ModuleInit: HMR0Init -> %Rrc\n", rc));
                GMMR0Term();
            }
            else
                LogRel(("ModuleInit: GMMR0Init -> %Rrc\n", rc));
            GVMMR0Term();
        }
        else
            LogRel(("ModuleInit: GVMMR0Init -> %Rrc\n", rc));
        vmmTermFormatTypes();
    }
    else
        LogRel(("ModuleInit: vmmInitFormatTypes -> %Rrc\n", rc));

    LogFlow(("ModuleInit: failed %Rrc\n", rc));
    return rc;
}


/**
 * Terminate the module.
 * This is called when we're finally unloaded.
 *
 * @param   hMod        Image handle for use in APIs.
 */
DECLEXPORT(void) ModuleTerm(void *hMod)
{
    NOREF(hMod);
    LogFlow(("ModuleTerm:\n"));

    /*
     * Terminate the CPUM module (Local APIC cleanup).
     */
    CPUMR0ModuleTerm();

    /*
     * Terminate the internal network service.
     */
    IntNetR0Term();

    /*
     * PGM (Darwin), HM and PciRaw global cleanup.
     */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
    PGMR0DynMapTerm();
#endif
#ifdef VBOX_WITH_PCI_PASSTHROUGH
    PciRawR0Term();
#endif
    PGMDeregisterStringFormatTypes();
    HMR0Term();
#ifdef VBOX_WITH_TRIPLE_FAULT_HACK
    vmmR0TripleFaultHackTerm();
#endif

    /*
     * Destroy the GMM and GVMM instances.
     */
    GMMR0Term();
    GVMMR0Term();

    vmmTermFormatTypes();

    LogFlow(("ModuleTerm: returns\n"));
}


/**
 * Initiates the R0 driver for a particular VM instance.
 *
 * @returns VBox status code.
 *
 * @param   pGVM        The global (ring-0) VM structure.
 * @param   pVM         The cross context VM structure.
 * @param   uSvnRev     The SVN revision of the ring-3 part.
 * @param   uBuildType  Build type indicator.
 * @thread  EMT(0)
 */
static int vmmR0InitVM(PGVM pGVM, PVM pVM, uint32_t uSvnRev, uint32_t uBuildType)
{
    VMM_CHECK_SMAP_SETUP();
    VMM_CHECK_SMAP_CHECK(return VERR_VMM_SMAP_BUT_AC_CLEAR);

    /*
     * Match the SVN revisions and build type.
     */
    if (uSvnRev != VMMGetSvnRev())
    {
        LogRel(("VMMR0InitVM: Revision mismatch, r3=%d r0=%d\n", uSvnRev, VMMGetSvnRev()));
        SUPR0Printf("VMMR0InitVM: Revision mismatch, r3=%d r0=%d\n", uSvnRev, VMMGetSvnRev());
        return VERR_VMM_R0_VERSION_MISMATCH;
    }
    if (uBuildType != vmmGetBuildType())
    {
        LogRel(("VMMR0InitVM: Build type mismatch, r3=%#x r0=%#x\n", uBuildType, vmmGetBuildType()));
        SUPR0Printf("VMMR0InitVM: Build type mismatch, r3=%#x r0=%#x\n", uBuildType, vmmGetBuildType());
        return VERR_VMM_R0_VERSION_MISMATCH;
    }

    int rc = GVMMR0ValidateGVMandVMandEMT(pGVM, pVM, 0 /*idCpu*/);
    if (RT_FAILURE(rc))
        return rc;


#ifdef LOG_ENABLED
    /*
     * Register the EMT R0 logger instance for VCPU 0.
     */
    PVMCPU pVCpu = &pVM->aCpus[0];

    PVMMR0LOGGER pR0Logger = pVCpu->vmm.s.pR0LoggerR0;
    if (pR0Logger)
    {
# if 0 /* testing of the logger. */
        LogCom(("vmmR0InitVM: before %p\n", RTLogDefaultInstance()));
        LogCom(("vmmR0InitVM: pfnFlush=%p actual=%p\n", pR0Logger->Logger.pfnFlush, vmmR0LoggerFlush));
        LogCom(("vmmR0InitVM: pfnLogger=%p actual=%p\n", pR0Logger->Logger.pfnLogger, vmmR0LoggerWrapper));
        LogCom(("vmmR0InitVM: offScratch=%d fFlags=%#x fDestFlags=%#x\n", pR0Logger->Logger.offScratch, pR0Logger->Logger.fFlags, pR0Logger->Logger.fDestFlags));

        RTLogSetDefaultInstanceThread(&pR0Logger->Logger, (uintptr_t)pVM->pSession);
        LogCom(("vmmR0InitVM: after %p reg\n", RTLogDefaultInstance()));
        RTLogSetDefaultInstanceThread(NULL, pVM->pSession);
        LogCom(("vmmR0InitVM: after %p dereg\n", RTLogDefaultInstance()));

        pR0Logger->Logger.pfnLogger("hello ring-0 logger\n");
        LogCom(("vmmR0InitVM: returned successfully from direct logger call.\n"));
        pR0Logger->Logger.pfnFlush(&pR0Logger->Logger);
        LogCom(("vmmR0InitVM: returned successfully from direct flush call.\n"));

        RTLogSetDefaultInstanceThread(&pR0Logger->Logger, (uintptr_t)pVM->pSession);
        LogCom(("vmmR0InitVM: after %p reg2\n", RTLogDefaultInstance()));
        pR0Logger->Logger.pfnLogger("hello ring-0 logger\n");
        LogCom(("vmmR0InitVM: returned successfully from direct logger call (2). offScratch=%d\n", pR0Logger->Logger.offScratch));
        RTLogSetDefaultInstanceThread(NULL, pVM->pSession);
        LogCom(("vmmR0InitVM: after %p dereg2\n", RTLogDefaultInstance()));

        RTLogLoggerEx(&pR0Logger->Logger, 0, ~0U, "hello ring-0 logger (RTLogLoggerEx)\n");
        LogCom(("vmmR0InitVM: RTLogLoggerEx returned fine offScratch=%d\n", pR0Logger->Logger.offScratch));

        RTLogSetDefaultInstanceThread(&pR0Logger->Logger, (uintptr_t)pVM->pSession);
        RTLogPrintf("hello ring-0 logger (RTLogPrintf)\n");
        LogCom(("vmmR0InitVM: RTLogPrintf returned fine offScratch=%d\n", pR0Logger->Logger.offScratch));
# endif
        Log(("Switching to per-thread logging instance %p (key=%p)\n", &pR0Logger->Logger, pVM->pSession));
        RTLogSetDefaultInstanceThread(&pR0Logger->Logger, (uintptr_t)pVM->pSession);
        pR0Logger->fRegistered = true;
    }
#endif /* LOG_ENABLED */

    /*
     * Check if the host supports high resolution timers or not.
     */
    if (   pVM->vmm.s.fUsePeriodicPreemptionTimers
        && !RTTimerCanDoHighResolution())
        pVM->vmm.s.fUsePeriodicPreemptionTimers = false;

    /*
     * Initialize the per VM data for GVMM and GMM.
     */
    VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
    rc = GVMMR0InitVM(pGVM);
//    if (RT_SUCCESS(rc))
//        rc = GMMR0InitPerVMData(pVM);
    if (RT_SUCCESS(rc))
    {
        /*
         * Init HM, CPUM and PGM (Darwin only).
         */
        VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
        rc = HMR0InitVM(pVM);
        if (RT_SUCCESS(rc))
            VMM_CHECK_SMAP_CHECK2(pVM, rc = VERR_VMM_RING0_ASSERTION); /* CPUR0InitVM will otherwise panic the host */
        if (RT_SUCCESS(rc))
        {
            rc = CPUMR0InitVM(pVM);
            if (RT_SUCCESS(rc))
            {
                VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
                rc = PGMR0DynMapInitVM(pVM);
#endif
                if (RT_SUCCESS(rc))
                {
                    VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
#ifdef VBOX_WITH_PCI_PASSTHROUGH
                    rc = PciRawR0InitVM(pGVM, pVM);
#endif
                    if (RT_SUCCESS(rc))
                    {
                        VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
                        rc = GIMR0InitVM(pVM);
                        if (RT_SUCCESS(rc))
                        {
                            VMM_CHECK_SMAP_CHECK2(pVM, rc = VERR_VMM_RING0_ASSERTION);
                            if (RT_SUCCESS(rc))
                            {
                                GVMMR0DoneInitVM(pGVM);
                                VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
                                return rc;
                            }

                            /* bail out*/
                            GIMR0TermVM(pVM);
                        }
#ifdef VBOX_WITH_PCI_PASSTHROUGH
                        PciRawR0TermVM(pGVM, pVM);
#endif
                    }
                }
            }
            HMR0TermVM(pVM);
        }
    }

    RTLogSetDefaultInstanceThread(NULL, (uintptr_t)pVM->pSession);
    return rc;
}


/**
 * Terminates the R0 bits for a particular VM instance.
 *
 * This is normally called by ring-3 as part of the VM termination process, but
 * may alternatively be called during the support driver session cleanup when
 * the VM object is destroyed (see GVMM).
 *
 * @returns VBox status code.
 *
 * @param   pGVM        The global (ring-0) VM structure.
 * @param   pVM         The cross context VM structure.
 * @param   idCpu       Set to 0 if EMT(0) or NIL_VMCPUID if session cleanup
 *                      thread.
 * @thread  EMT(0) or session clean up thread.
 */
VMMR0_INT_DECL(int) VMMR0TermVM(PGVM pGVM, PVM pVM, VMCPUID idCpu)
{
    /*
     * Check EMT(0) claim if we're called from userland.
     */
    if (idCpu != NIL_VMCPUID)
    {
        AssertReturn(idCpu == 0, VERR_INVALID_CPU_ID);
        int rc = GVMMR0ValidateGVMandVMandEMT(pGVM, pVM, idCpu);
        if (RT_FAILURE(rc))
            return rc;
    }

#ifdef VBOX_WITH_PCI_PASSTHROUGH
    PciRawR0TermVM(pGVM, pVM);
#endif

    /*
     * Tell GVMM what we're up to and check that we only do this once.
     */
    if (GVMMR0DoingTermVM(pGVM))
    {
        GIMR0TermVM(pVM);

        /** @todo I wish to call PGMR0PhysFlushHandyPages(pVM, &pVM->aCpus[idCpu])
         *        here to make sure we don't leak any shared pages if we crash... */
#ifdef VBOX_WITH_2X_4GB_ADDR_SPACE
        PGMR0DynMapTermVM(pVM);
#endif
        HMR0TermVM(pVM);
    }

    /*
     * Deregister the logger.
     */
    RTLogSetDefaultInstanceThread(NULL, (uintptr_t)pVM->pSession);
    return VINF_SUCCESS;
}


/**
 * VMM ring-0 thread-context callback.
 *
 * This does common HM state updating and calls the HM-specific thread-context
 * callback.
 *
 * @param   enmEvent    The thread-context event.
 * @param   pvUser      Opaque pointer to the VMCPU.
 *
 * @thread  EMT(pvUser)
 */
static DECLCALLBACK(void) vmmR0ThreadCtxCallback(RTTHREADCTXEVENT enmEvent, void *pvUser)
{
    PVMCPU pVCpu = (PVMCPU)pvUser;

    switch (enmEvent)
    {
        case RTTHREADCTXEVENT_IN:
        {
            /*
             * Linux may call us with preemption enabled (really!) but technically we
             * cannot get preempted here, otherwise we end up in an infinite recursion
             * scenario (i.e. preempted in resume hook -> preempt hook -> resume hook...
             * ad infinitum). Let's just disable preemption for now...
             */
            /** @todo r=bird: I don't believe the above. The linux code is clearly enabling
             *        preemption after doing the callout (one or two functions up the
             *        call chain). */
            /** @todo r=ramshankar: See @bugref{5313#c30}. */
            RTTHREADPREEMPTSTATE ParanoidPreemptState = RTTHREADPREEMPTSTATE_INITIALIZER;
            RTThreadPreemptDisable(&ParanoidPreemptState);

            /* We need to update the VCPU <-> host CPU mapping. */
            RTCPUID idHostCpu;
            uint32_t iHostCpuSet = RTMpCurSetIndexAndId(&idHostCpu);
            pVCpu->iHostCpuSet   = iHostCpuSet;
            ASMAtomicWriteU32(&pVCpu->idHostCpu, idHostCpu);

            /* In the very unlikely event that the GIP delta for the CPU we're
               rescheduled needs calculating, try force a return to ring-3.
               We unfortunately cannot do the measurements right here. */
            if (RT_UNLIKELY(SUPIsTscDeltaAvailableForCpuSetIndex(iHostCpuSet)))
                VMCPU_FF_SET(pVCpu, VMCPU_FF_TO_R3);

            /* Invoke the HM-specific thread-context callback. */
            HMR0ThreadCtxCallback(enmEvent, pvUser);

            /* Restore preemption. */
            RTThreadPreemptRestore(&ParanoidPreemptState);
            break;
        }

        case RTTHREADCTXEVENT_OUT:
        {
            /* Invoke the HM-specific thread-context callback. */
            HMR0ThreadCtxCallback(enmEvent, pvUser);

            /*
             * Sigh. See VMMGetCpu() used by VMCPU_ASSERT_EMT(). We cannot let several VCPUs
             * have the same host CPU associated with it.
             */
            pVCpu->iHostCpuSet = UINT32_MAX;
            ASMAtomicWriteU32(&pVCpu->idHostCpu, NIL_RTCPUID);
            break;
        }

        default:
            /* Invoke the HM-specific thread-context callback. */
            HMR0ThreadCtxCallback(enmEvent, pvUser);
            break;
    }
}


/**
 * Creates thread switching hook for the current EMT thread.
 *
 * This is called by GVMMR0CreateVM and GVMMR0RegisterVCpu.  If the host
 * platform does not implement switcher hooks, no hooks will be create and the
 * member set to NIL_RTTHREADCTXHOOK.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @thread  EMT(pVCpu)
 */
VMMR0_INT_DECL(int) VMMR0ThreadCtxHookCreateForEmt(PVMCPU pVCpu)
{
    VMCPU_ASSERT_EMT(pVCpu);
    Assert(pVCpu->vmm.s.hCtxHook == NIL_RTTHREADCTXHOOK);

#if 1 /* To disable this stuff change to zero. */
    int rc = RTThreadCtxHookCreate(&pVCpu->vmm.s.hCtxHook, 0, vmmR0ThreadCtxCallback, pVCpu);
    if (RT_SUCCESS(rc))
        return rc;
#else
    RT_NOREF(vmmR0ThreadCtxCallback);
    int rc = VERR_NOT_SUPPORTED;
#endif

    pVCpu->vmm.s.hCtxHook = NIL_RTTHREADCTXHOOK;
    if (rc == VERR_NOT_SUPPORTED)
        return VINF_SUCCESS;

    LogRelMax(32, ("RTThreadCtxHookCreate failed! rc=%Rrc pVCpu=%p idCpu=%RU32\n", rc, pVCpu, pVCpu->idCpu));
    return VINF_SUCCESS; /* Just ignore it, we can live without context hooks. */
}


/**
 * Destroys the thread switching hook for the specified VCPU.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @remarks Can be called from any thread.
 */
VMMR0_INT_DECL(void) VMMR0ThreadCtxHookDestroyForEmt(PVMCPU pVCpu)
{
    int rc = RTThreadCtxHookDestroy(pVCpu->vmm.s.hCtxHook);
    AssertRC(rc);
    pVCpu->vmm.s.hCtxHook = NIL_RTTHREADCTXHOOK;
}


/**
 * Disables the thread switching hook for this VCPU (if we got one).
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @thread  EMT(pVCpu)
 *
 * @remarks This also clears VMCPU::idHostCpu, so the mapping is invalid after
 *          this call.  This means you have to be careful with what you do!
 */
VMMR0_INT_DECL(void) VMMR0ThreadCtxHookDisable(PVMCPU pVCpu)
{
    /*
     * Clear the VCPU <-> host CPU mapping as we've left HM context.
     * @bugref{7726#c19} explains the need for this trick:
     *
     *      hmR0VmxCallRing3Callback/hmR0SvmCallRing3Callback &
     *      hmR0VmxLeaveSession/hmR0SvmLeaveSession disables context hooks during
     *      longjmp & normal return to ring-3, which opens a window where we may be
     *      rescheduled without changing VMCPUID::idHostCpu and cause confusion if
     *      the CPU starts executing a different EMT.  Both functions first disables
     *      preemption and then calls HMR0LeaveCpu which invalids idHostCpu, leaving
     *      an opening for getting preempted.
     */
    /** @todo Make HM not need this API!  Then we could leave the hooks enabled
     *        all the time. */
    /** @todo move this into the context hook disabling if(). */
    ASMAtomicWriteU32(&pVCpu->idHostCpu, NIL_RTCPUID);

    /*
     * Disable the context hook, if we got one.
     */
    if (pVCpu->vmm.s.hCtxHook != NIL_RTTHREADCTXHOOK)
    {
        Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
        int rc = RTThreadCtxHookDisable(pVCpu->vmm.s.hCtxHook);
        AssertRC(rc);
    }
}


/**
 * Internal version of VMMR0ThreadCtxHooksAreRegistered.
 *
 * @returns true if registered, false otherwise.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
DECLINLINE(bool) vmmR0ThreadCtxHookIsEnabled(PVMCPU pVCpu)
{
    return RTThreadCtxHookIsEnabled(pVCpu->vmm.s.hCtxHook);
}


/**
 * Whether thread-context hooks are registered for this VCPU.
 *
 * @returns true if registered, false otherwise.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR0_INT_DECL(bool) VMMR0ThreadCtxHookIsEnabled(PVMCPU pVCpu)
{
    return vmmR0ThreadCtxHookIsEnabled(pVCpu);
}


#ifdef VBOX_WITH_STATISTICS
/**
 * Record return code statistics
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   rc          The status code.
 */
static void vmmR0RecordRC(PVM pVM, PVMCPU pVCpu, int rc)
{
    /*
     * Collect statistics.
     */
    switch (rc)
    {
        case VINF_SUCCESS:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetNormal);
            break;
        case VINF_EM_RAW_INTERRUPT:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetInterrupt);
            break;
        case VINF_EM_RAW_INTERRUPT_HYPER:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetInterruptHyper);
            break;
        case VINF_EM_RAW_GUEST_TRAP:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetGuestTrap);
            break;
        case VINF_EM_RAW_RING_SWITCH:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetRingSwitch);
            break;
        case VINF_EM_RAW_RING_SWITCH_INT:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetRingSwitchInt);
            break;
        case VINF_EM_RAW_STALE_SELECTOR:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetStaleSelector);
            break;
        case VINF_EM_RAW_IRET_TRAP:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetIRETTrap);
            break;
        case VINF_IOM_R3_IOPORT_READ:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetIORead);
            break;
        case VINF_IOM_R3_IOPORT_WRITE:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetIOWrite);
            break;
        case VINF_IOM_R3_IOPORT_COMMIT_WRITE:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetIOCommitWrite);
            break;
        case VINF_IOM_R3_MMIO_READ:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetMMIORead);
            break;
        case VINF_IOM_R3_MMIO_WRITE:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetMMIOWrite);
            break;
        case VINF_IOM_R3_MMIO_COMMIT_WRITE:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetMMIOCommitWrite);
            break;
        case VINF_IOM_R3_MMIO_READ_WRITE:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetMMIOReadWrite);
            break;
        case VINF_PATM_HC_MMIO_PATCH_READ:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetMMIOPatchRead);
            break;
        case VINF_PATM_HC_MMIO_PATCH_WRITE:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetMMIOPatchWrite);
            break;
        case VINF_CPUM_R3_MSR_READ:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetMSRRead);
            break;
        case VINF_CPUM_R3_MSR_WRITE:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetMSRWrite);
            break;
        case VINF_EM_RAW_EMULATE_INSTR:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetEmulate);
            break;
        case VINF_EM_RAW_EMULATE_IO_BLOCK:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetIOBlockEmulate);
            break;
        case VINF_PATCH_EMULATE_INSTR:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetPatchEmulate);
            break;
        case VINF_EM_RAW_EMULATE_INSTR_LDT_FAULT:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetLDTFault);
            break;
        case VINF_EM_RAW_EMULATE_INSTR_GDT_FAULT:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetGDTFault);
            break;
        case VINF_EM_RAW_EMULATE_INSTR_IDT_FAULT:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetIDTFault);
            break;
        case VINF_EM_RAW_EMULATE_INSTR_TSS_FAULT:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetTSSFault);
            break;
        case VINF_CSAM_PENDING_ACTION:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetCSAMTask);
            break;
        case VINF_PGM_SYNC_CR3:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetSyncCR3);
            break;
        case VINF_PATM_PATCH_INT3:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetPatchInt3);
            break;
        case VINF_PATM_PATCH_TRAP_PF:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetPatchPF);
            break;
        case VINF_PATM_PATCH_TRAP_GP:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetPatchGP);
            break;
        case VINF_PATM_PENDING_IRQ_AFTER_IRET:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetPatchIretIRQ);
            break;
        case VINF_EM_RESCHEDULE_REM:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetRescheduleREM);
            break;
        case VINF_EM_RAW_TO_R3:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetToR3Total);
            if (VM_FF_IS_PENDING(pVM, VM_FF_TM_VIRTUAL_SYNC))
                STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetToR3TMVirt);
            else if (VM_FF_IS_PENDING(pVM, VM_FF_PGM_NEED_HANDY_PAGES))
                STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetToR3HandyPages);
            else if (VM_FF_IS_PENDING(pVM, VM_FF_PDM_QUEUES))
                STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetToR3PDMQueues);
            else if (VM_FF_IS_PENDING(pVM, VM_FF_EMT_RENDEZVOUS))
                STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetToR3Rendezvous);
            else if (VM_FF_IS_PENDING(pVM, VM_FF_PDM_DMA))
                STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetToR3DMA);
            else if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_TIMER))
                STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetToR3Timer);
            else if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_PDM_CRITSECT))
                STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetToR3CritSect);
            else if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_TO_R3))
                STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetToR3FF);
            else if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_IEM))
                STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetToR3Iem);
            else if (VMCPU_FF_IS_PENDING(pVCpu, VMCPU_FF_IOM))
                STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetToR3Iom);
            else
                STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetToR3Unknown);
            break;

        case VINF_EM_RAW_TIMER_PENDING:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetTimerPending);
            break;
        case VINF_EM_RAW_INTERRUPT_PENDING:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetInterruptPending);
            break;
        case VINF_VMM_CALL_HOST:
            switch (pVCpu->vmm.s.enmCallRing3Operation)
            {
                case VMMCALLRING3_PDM_CRIT_SECT_ENTER:
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRZCallPDMCritSectEnter);
                    break;
                case VMMCALLRING3_PDM_LOCK:
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRZCallPDMLock);
                    break;
                case VMMCALLRING3_PGM_POOL_GROW:
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRZCallPGMPoolGrow);
                    break;
                case VMMCALLRING3_PGM_LOCK:
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRZCallPGMLock);
                    break;
                case VMMCALLRING3_PGM_MAP_CHUNK:
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRZCallPGMMapChunk);
                    break;
                case VMMCALLRING3_PGM_ALLOCATE_HANDY_PAGES:
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRZCallPGMAllocHandy);
                    break;
                case VMMCALLRING3_REM_REPLAY_HANDLER_NOTIFICATIONS:
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRZCallRemReplay);
                    break;
                case VMMCALLRING3_VMM_LOGGER_FLUSH:
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRZCallLogFlush);
                    break;
                case VMMCALLRING3_VM_SET_ERROR:
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRZCallVMSetError);
                    break;
                case VMMCALLRING3_VM_SET_RUNTIME_ERROR:
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRZCallVMSetRuntimeError);
                    break;
                case VMMCALLRING3_VM_R0_ASSERTION:
                default:
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetCallRing3);
                    break;
            }
            break;
        case VINF_PATM_DUPLICATE_FUNCTION:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetPATMDuplicateFn);
            break;
        case VINF_PGM_CHANGE_MODE:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetPGMChangeMode);
            break;
        case VINF_PGM_POOL_FLUSH_PENDING:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetPGMFlushPending);
            break;
        case VINF_EM_PENDING_REQUEST:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetPendingRequest);
            break;
        case VINF_EM_HM_PATCH_TPR_INSTR:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetPatchTPR);
            break;
        default:
            STAM_COUNTER_INC(&pVM->vmm.s.StatRZRetMisc);
            break;
    }
}
#endif /* VBOX_WITH_STATISTICS */


/**
 * The Ring 0 entry point, called by the fast-ioctl path.
 *
 * @param   pGVM            The global (ring-0) VM structure.
 * @param   pVM             The cross context VM structure.
 *                          The return code is stored in pVM->vmm.s.iLastGZRc.
 * @param   idCpu           The Virtual CPU ID of the calling EMT.
 * @param   enmOperation    Which operation to execute.
 * @remarks Assume called with interrupts _enabled_.
 */
VMMR0DECL(void) VMMR0EntryFast(PGVM pGVM, PVM pVM, VMCPUID idCpu, VMMR0OPERATION enmOperation)
{
    /*
     * Validation.
     */
    if (   idCpu < pGVM->cCpus
        && pGVM->cCpus == pVM->cCpus)
    { /*likely*/ }
    else
    {
        SUPR0Printf("VMMR0EntryFast: Bad idCpu=%#x cCpus=%#x/%#x\n", idCpu, pGVM->cCpus, pVM->cCpus);
        return;
    }

    PGVMCPU pGVCpu = &pGVM->aCpus[idCpu];
    PVMCPU  pVCpu  = &pVM->aCpus[idCpu];
    RTNATIVETHREAD const hNativeThread = RTThreadNativeSelf();
    if (RT_LIKELY(   pGVCpu->hEMT           == hNativeThread
                  && pVCpu->hNativeThreadR0 == hNativeThread))
    { /* likely */ }
    else
    {
        SUPR0Printf("VMMR0EntryFast: Bad thread idCpu=%#x hNativeSelf=%p pGVCpu->hEmt=%p pVCpu->hNativeThreadR0=%p\n",
                    idCpu, hNativeThread, pGVCpu->hEMT, pVCpu->hNativeThreadR0);
        return;
    }

    /*
     * SMAP fun.
     */
    VMM_CHECK_SMAP_SETUP();
    VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);

    /*
     * Perform requested operation.
     */
    switch (enmOperation)
    {
        /*
         * Switch to GC and run guest raw mode code.
         * Disable interrupts before doing the world switch.
         */
        case VMMR0_DO_RAW_RUN:
        {
#ifdef VBOX_WITH_RAW_MODE
# ifndef VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0
            /* Some safety precautions first. */
            if (RT_UNLIKELY(!PGMGetHyperCR3(pVCpu)))
            {
                pVCpu->vmm.s.iLastGZRc = VERR_PGM_NO_CR3_SHADOW_ROOT;
                break;
            }
# endif
            if (RT_SUCCESS(g_rcRawModeUsability))
            { /* likely */ }
            else
            {
                pVCpu->vmm.s.iLastGZRc = g_rcRawModeUsability;
                break;
            }

            /*
             * Disable preemption.
             */
            RTTHREADPREEMPTSTATE PreemptState = RTTHREADPREEMPTSTATE_INITIALIZER;
            RTThreadPreemptDisable(&PreemptState);

            /*
             * Get the host CPU identifiers, make sure they are valid and that
             * we've got a TSC delta for the CPU.
             */
            RTCPUID  idHostCpu;
            uint32_t iHostCpuSet = RTMpCurSetIndexAndId(&idHostCpu);
            if (RT_LIKELY(   iHostCpuSet < RTCPUSET_MAX_CPUS
                          && SUPIsTscDeltaAvailableForCpuSetIndex(iHostCpuSet)))
            {
                /*
                 * Commit the CPU identifiers and update the periodict preemption timer if it's active.
                 */
# ifdef VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI
                CPUMR0SetLApic(pVCpu, iHostCpuSet);
# endif
                pVCpu->iHostCpuSet = iHostCpuSet;
                ASMAtomicWriteU32(&pVCpu->idHostCpu, idHostCpu);

                if (pVM->vmm.s.fUsePeriodicPreemptionTimers)
                    GVMMR0SchedUpdatePeriodicPreemptionTimer(pVM, pVCpu->idHostCpu, TMCalcHostTimerFrequency(pVM, pVCpu));

                /*
                 * We might need to disable VT-x if the active switcher turns off paging.
                  */
                bool fVTxDisabled;
                int rc = HMR0EnterSwitcher(pVM, pVM->vmm.s.enmSwitcher, &fVTxDisabled);
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Disable interrupts and run raw-mode code.  The loop is for efficiently
                     * dispatching tracepoints that fired in raw-mode context.
                     */
                    RTCCUINTREG uFlags = ASMIntDisableFlags();

                    for (;;)
                    {
                        VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED_EXEC);
                        TMNotifyStartOfExecution(pVCpu);

                        rc = pVM->vmm.s.pfnR0ToRawMode(pVM);
                        pVCpu->vmm.s.iLastGZRc = rc;

                        TMNotifyEndOfExecution(pVCpu);
                        VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED);

                        if (rc != VINF_VMM_CALL_TRACER)
                            break;
                        SUPR0TracerUmodProbeFire(pVM->pSession, &pVCpu->vmm.s.TracerCtx);
                    }

                    /*
                     * Re-enable VT-x before we dispatch any pending host interrupts and
                     * re-enables interrupts.
                     */
                    HMR0LeaveSwitcher(pVM, fVTxDisabled);

                    if (    rc == VINF_EM_RAW_INTERRUPT
                        ||  rc == VINF_EM_RAW_INTERRUPT_HYPER)
                        TRPMR0DispatchHostInterrupt(pVM);

                    ASMSetFlags(uFlags);

                    /* Fire dtrace probe and collect statistics. */
                    VBOXVMM_R0_VMM_RETURN_TO_RING3_RC(pVCpu, CPUMQueryGuestCtxPtr(pVCpu), rc);
# ifdef VBOX_WITH_STATISTICS
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRunRC);
                    vmmR0RecordRC(pVM, pVCpu, rc);
# endif
                }
                else
                    pVCpu->vmm.s.iLastGZRc = rc;

                /*
                 * Invalidate the host CPU identifiers as we restore preemption.
                 */
                pVCpu->iHostCpuSet = UINT32_MAX;
                ASMAtomicWriteU32(&pVCpu->idHostCpu, NIL_RTCPUID);

                RTThreadPreemptRestore(&PreemptState);
            }
            /*
             * Invalid CPU set index or TSC delta in need of measuring.
             */
            else
            {
                RTThreadPreemptRestore(&PreemptState);
                if (iHostCpuSet < RTCPUSET_MAX_CPUS)
                {
                    int rc = SUPR0TscDeltaMeasureBySetIndex(pVM->pSession, iHostCpuSet, 0 /*fFlags*/,
                                                            2 /*cMsWaitRetry*/, 5*RT_MS_1SEC /*cMsWaitThread*/,
                                                            0 /*default cTries*/);
                    if (RT_SUCCESS(rc) || rc == VERR_CPU_OFFLINE)
                        pVCpu->vmm.s.iLastGZRc = VINF_EM_RAW_TO_R3;
                    else
                        pVCpu->vmm.s.iLastGZRc = rc;
                }
                else
                    pVCpu->vmm.s.iLastGZRc = VERR_INVALID_CPU_INDEX;
            }

#else  /* !VBOX_WITH_RAW_MODE */
            pVCpu->vmm.s.iLastGZRc = VERR_RAW_MODE_NOT_SUPPORTED;
#endif
            break;
        }

        /*
         * Run guest code using the available hardware acceleration technology.
         */
        case VMMR0_DO_HM_RUN:
        {
            /*
             * Disable preemption.
             */
            Assert(!vmmR0ThreadCtxHookIsEnabled(pVCpu));
            RTTHREADPREEMPTSTATE PreemptState = RTTHREADPREEMPTSTATE_INITIALIZER;
            RTThreadPreemptDisable(&PreemptState);

            /*
             * Get the host CPU identifiers, make sure they are valid and that
             * we've got a TSC delta for the CPU.
             */
            RTCPUID  idHostCpu;
            uint32_t iHostCpuSet = RTMpCurSetIndexAndId(&idHostCpu);
            if (RT_LIKELY(   iHostCpuSet < RTCPUSET_MAX_CPUS
                          && SUPIsTscDeltaAvailableForCpuSetIndex(iHostCpuSet)))
            {
                pVCpu->iHostCpuSet = iHostCpuSet;
                ASMAtomicWriteU32(&pVCpu->idHostCpu, idHostCpu);

                /*
                 * Update the periodic preemption timer if it's active.
                 */
                if (pVM->vmm.s.fUsePeriodicPreemptionTimers)
                    GVMMR0SchedUpdatePeriodicPreemptionTimer(pVM, pVCpu->idHostCpu, TMCalcHostTimerFrequency(pVM, pVCpu));
                VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);

#ifdef LOG_ENABLED
                /*
                 * Ugly: Lazy registration of ring 0 loggers.
                 */
                if (pVCpu->idCpu > 0)
                {
                    PVMMR0LOGGER pR0Logger = pVCpu->vmm.s.pR0LoggerR0;
                    if (   pR0Logger
                        && RT_UNLIKELY(!pR0Logger->fRegistered))
                    {
                        RTLogSetDefaultInstanceThread(&pR0Logger->Logger, (uintptr_t)pVM->pSession);
                        pR0Logger->fRegistered = true;
                    }
                }
#endif

#ifdef VMM_R0_TOUCH_FPU
                /*
                 * Make sure we've got the FPU state loaded so and we don't need to clear
                 * CR0.TS and get out of sync with the host kernel when loading the guest
                 * FPU state.  @ref sec_cpum_fpu (CPUM.cpp) and @bugref{4053}.
                 */
                CPUMR0TouchHostFpu();
#endif
                int  rc;
                bool fPreemptRestored = false;
                if (!HMR0SuspendPending())
                {
                    /*
                     * Enable the context switching hook.
                     */
                    if (pVCpu->vmm.s.hCtxHook != NIL_RTTHREADCTXHOOK)
                    {
                        Assert(!RTThreadCtxHookIsEnabled(pVCpu->vmm.s.hCtxHook));
                        int rc2 = RTThreadCtxHookEnable(pVCpu->vmm.s.hCtxHook); AssertRC(rc2);
                    }

                    /*
                     * Enter HM context.
                     */
                    rc = HMR0Enter(pVM, pVCpu);
                    if (RT_SUCCESS(rc))
                    {
                        VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED_HM);

                        /*
                         * When preemption hooks are in place, enable preemption now that
                         * we're in HM context.
                         */
                        if (vmmR0ThreadCtxHookIsEnabled(pVCpu))
                        {
                            fPreemptRestored = true;
                            RTThreadPreemptRestore(&PreemptState);
                        }

                        /*
                         * Setup the longjmp machinery and execute guest code (calls HMR0RunGuestCode).
                         */
                        VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
                        rc = vmmR0CallRing3SetJmp(&pVCpu->vmm.s.CallRing3JmpBufR0, HMR0RunGuestCode, pVM, pVCpu);
                        VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);

                        /*
                         * Assert sanity on the way out.  Using manual assertions code here as normal
                         * assertions are going to panic the host since we're outside the setjmp/longjmp zone.
                         */
                        if (RT_UNLIKELY(   VMCPU_GET_STATE(pVCpu) != VMCPUSTATE_STARTED_HM
                                        && RT_SUCCESS_NP(rc)  && rc !=  VINF_VMM_CALL_HOST ))
                        {
                            pVM->vmm.s.szRing0AssertMsg1[0] = '\0';
                            RTStrPrintf(pVM->vmm.s.szRing0AssertMsg2, sizeof(pVM->vmm.s.szRing0AssertMsg2),
                                        "Got VMCPU state %d expected %d.\n", VMCPU_GET_STATE(pVCpu), VMCPUSTATE_STARTED_HM);
                            rc = VERR_VMM_WRONG_HM_VMCPU_STATE;
                        }
                        /** @todo Get rid of this. HM shouldn't disable the context hook. */
                        else if (RT_UNLIKELY(vmmR0ThreadCtxHookIsEnabled(pVCpu)))
                        {
                            pVM->vmm.s.szRing0AssertMsg1[0] = '\0';
                            RTStrPrintf(pVM->vmm.s.szRing0AssertMsg2, sizeof(pVM->vmm.s.szRing0AssertMsg2),
                                        "Thread-context hooks still enabled! VCPU=%p Id=%u rc=%d.\n", pVCpu, pVCpu->idCpu, rc);
                            rc = VERR_INVALID_STATE;
                        }

                        VMCPU_SET_STATE(pVCpu, VMCPUSTATE_STARTED);
                    }
                    STAM_COUNTER_INC(&pVM->vmm.s.StatRunRC);

                    /*
                     * Invalidate the host CPU identifiers before we disable the context
                     * hook / restore preemption.
                     */
                    pVCpu->iHostCpuSet = UINT32_MAX;
                    ASMAtomicWriteU32(&pVCpu->idHostCpu, NIL_RTCPUID);

                    /*
                     * Disable context hooks.  Due to unresolved cleanup issues, we
                     * cannot leave the hooks enabled when we return to ring-3.
                     *
                     * Note! At the moment HM may also have disabled the hook
                     *       when we get here, but the IPRT API handles that.
                     */
                    if (pVCpu->vmm.s.hCtxHook != NIL_RTTHREADCTXHOOK)
                    {
                        ASMAtomicWriteU32(&pVCpu->idHostCpu, NIL_RTCPUID);
                        RTThreadCtxHookDisable(pVCpu->vmm.s.hCtxHook);
                    }
                }
                /*
                 * The system is about to go into suspend mode; go back to ring 3.
                 */
                else
                {
                    rc = VINF_EM_RAW_INTERRUPT;
                    pVCpu->iHostCpuSet = UINT32_MAX;
                    ASMAtomicWriteU32(&pVCpu->idHostCpu, NIL_RTCPUID);
                }

                /** @todo When HM stops messing with the context hook state, we'll disable
                 *        preemption again before the RTThreadCtxHookDisable call. */
                if (!fPreemptRestored)
                    RTThreadPreemptRestore(&PreemptState);

                pVCpu->vmm.s.iLastGZRc = rc;

                /* Fire dtrace probe and collect statistics. */
                VBOXVMM_R0_VMM_RETURN_TO_RING3_HM(pVCpu, CPUMQueryGuestCtxPtr(pVCpu), rc);
#ifdef VBOX_WITH_STATISTICS
                vmmR0RecordRC(pVM, pVCpu, rc);
#endif
            }
            /*
             * Invalid CPU set index or TSC delta in need of measuring.
             */
            else
            {
                pVCpu->iHostCpuSet = UINT32_MAX;
                ASMAtomicWriteU32(&pVCpu->idHostCpu, NIL_RTCPUID);
                RTThreadPreemptRestore(&PreemptState);
                if (iHostCpuSet < RTCPUSET_MAX_CPUS)
                {
                    int rc = SUPR0TscDeltaMeasureBySetIndex(pVM->pSession, iHostCpuSet, 0 /*fFlags*/,
                                                            2 /*cMsWaitRetry*/, 5*RT_MS_1SEC /*cMsWaitThread*/,
                                                            0 /*default cTries*/);
                    if (RT_SUCCESS(rc) || rc == VERR_CPU_OFFLINE)
                        pVCpu->vmm.s.iLastGZRc = VINF_EM_RAW_TO_R3;
                    else
                        pVCpu->vmm.s.iLastGZRc = rc;
                }
                else
                    pVCpu->vmm.s.iLastGZRc = VERR_INVALID_CPU_INDEX;
            }
            break;
        }

        /*
         * For profiling.
         */
        case VMMR0_DO_NOP:
            pVCpu->vmm.s.iLastGZRc = VINF_SUCCESS;
            break;

        /*
         * Impossible.
         */
        default:
            AssertMsgFailed(("%#x\n", enmOperation));
            pVCpu->vmm.s.iLastGZRc = VERR_NOT_SUPPORTED;
            break;
    }
    VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
}


/**
 * Validates a session or VM session argument.
 *
 * @returns true / false accordingly.
 * @param   pVM             The cross context VM structure.
 * @param   pClaimedSession The session claim to validate.
 * @param   pSession        The session argument.
 */
DECLINLINE(bool) vmmR0IsValidSession(PVM pVM, PSUPDRVSESSION pClaimedSession, PSUPDRVSESSION pSession)
{
    /* This must be set! */
    if (!pSession)
        return false;

    /* Only one out of the two. */
    if (pVM && pClaimedSession)
        return false;
    if (pVM)
        pClaimedSession = pVM->pSession;
    return pClaimedSession == pSession;
}


/**
 * VMMR0EntryEx worker function, either called directly or when ever possible
 * called thru a longjmp so we can exit safely on failure.
 *
 * @returns VBox status code.
 * @param   pGVM            The global (ring-0) VM structure.
 * @param   pVM             The cross context VM structure.
 * @param   idCpu           Virtual CPU ID argument. Must be NIL_VMCPUID if pVM
 *                          is NIL_RTR0PTR, and may be NIL_VMCPUID if it isn't
 * @param   enmOperation    Which operation to execute.
 * @param   pReqHdr         This points to a SUPVMMR0REQHDR packet. Optional.
 *                          The support driver validates this if it's present.
 * @param   u64Arg          Some simple constant argument.
 * @param   pSession        The session of the caller.
 *
 * @remarks Assume called with interrupts _enabled_.
 */
static int vmmR0EntryExWorker(PGVM pGVM, PVM pVM, VMCPUID idCpu, VMMR0OPERATION enmOperation,
                              PSUPVMMR0REQHDR pReqHdr, uint64_t u64Arg, PSUPDRVSESSION pSession)
{
    /*
     * Validate pGVM, pVM and idCpu for consistency and validity.
     */
    if (   pGVM != NULL
        || pVM  != NULL)
    {
        if (RT_LIKELY(   RT_VALID_PTR(pGVM)
                      && RT_VALID_PTR(pVM)
                      && ((uintptr_t)pVM & PAGE_OFFSET_MASK) == 0))
        { /* likely */ }
        else
        {
            SUPR0Printf("vmmR0EntryExWorker: Invalid pGVM=%p and/or pVM=%p! (op=%d)\n", pGVM, pVM, enmOperation);
            return VERR_INVALID_POINTER;
        }

        if (RT_LIKELY(pGVM->pVM == pVM))
        { /* likely */ }
        else
        {
            SUPR0Printf("vmmR0EntryExWorker: pVM mismatch: got %p, pGVM->pVM=%p\n", pVM, pGVM->pVM);
            return VERR_INVALID_PARAMETER;
        }

        if (RT_LIKELY(idCpu == NIL_VMCPUID || idCpu < pGVM->cCpus))
        { /* likely */ }
        else
        {
            SUPR0Printf("vmmR0EntryExWorker: Invalid idCpu %#x (cCpus=%#x)\n", idCpu, pGVM->cCpus);
            return VERR_INVALID_PARAMETER;
        }

        if (RT_LIKELY(   pVM->enmVMState >= VMSTATE_CREATING
                      && pVM->enmVMState <= VMSTATE_TERMINATED
                      && pVM->cCpus      == pGVM->cCpus
                      && pVM->pSession   == pSession
                      && pVM->pVMR0      == pVM))
        { /* likely */ }
        else
        {
            SUPR0Printf("vmmR0EntryExWorker: Invalid pVM=%p:{.enmVMState=%d, .cCpus=%#x(==%#x), .pSession=%p(==%p), .pVMR0=%p(==%p)}! (op=%d)\n",
                        pVM, pVM->enmVMState, pVM->cCpus, pGVM->cCpus, pVM->pSession, pSession, pVM->pVMR0, pVM, enmOperation);
            return VERR_INVALID_POINTER;
        }
    }
    else if (RT_LIKELY(idCpu == NIL_VMCPUID))
    { /* likely */ }
    else
    {
        SUPR0Printf("vmmR0EntryExWorker: Invalid idCpu=%u\n", idCpu);
        return VERR_INVALID_PARAMETER;
    }

    /*
     * SMAP fun.
     */
    VMM_CHECK_SMAP_SETUP();
    VMM_CHECK_SMAP_CHECK(RT_NOTHING);

    /*
     * Process the request.
     */
    int rc;
    switch (enmOperation)
    {
        /*
         * GVM requests
         */
        case VMMR0_DO_GVMM_CREATE_VM:
            if (pGVM == NULL && pVM == NULL && u64Arg == 0 && idCpu == NIL_VMCPUID)
                rc = GVMMR0CreateVMReq((PGVMMCREATEVMREQ)pReqHdr, pSession);
            else
                rc = VERR_INVALID_PARAMETER;
            VMM_CHECK_SMAP_CHECK(RT_NOTHING);
            break;

        case VMMR0_DO_GVMM_DESTROY_VM:
            if (pReqHdr == NULL && u64Arg == 0)
                rc = GVMMR0DestroyVM(pGVM, pVM);
            else
                rc = VERR_INVALID_PARAMETER;
            VMM_CHECK_SMAP_CHECK(RT_NOTHING);
            break;

        case VMMR0_DO_GVMM_REGISTER_VMCPU:
            if (pGVM != NULL && pVM != NULL)
                rc = GVMMR0RegisterVCpu(pGVM, pVM, idCpu);
            else
                rc = VERR_INVALID_PARAMETER;
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GVMM_DEREGISTER_VMCPU:
            if (pGVM != NULL && pVM != NULL)
                rc = GVMMR0DeregisterVCpu(pGVM, pVM, idCpu);
            else
                rc = VERR_INVALID_PARAMETER;
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GVMM_SCHED_HALT:
            if (pReqHdr)
                return VERR_INVALID_PARAMETER;
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            rc = GVMMR0SchedHalt(pGVM, pVM, idCpu, u64Arg);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GVMM_SCHED_WAKE_UP:
            if (pReqHdr || u64Arg)
                return VERR_INVALID_PARAMETER;
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            rc = GVMMR0SchedWakeUp(pGVM, pVM, idCpu);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GVMM_SCHED_POKE:
            if (pReqHdr || u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GVMMR0SchedPoke(pGVM, pVM, idCpu);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GVMM_SCHED_WAKE_UP_AND_POKE_CPUS:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GVMMR0SchedWakeUpAndPokeCpusReq(pGVM, pVM, (PGVMMSCHEDWAKEUPANDPOKECPUSREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GVMM_SCHED_POLL:
            if (pReqHdr || u64Arg > 1)
                return VERR_INVALID_PARAMETER;
            rc = GVMMR0SchedPoll(pGVM, pVM, idCpu, !!u64Arg);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GVMM_QUERY_STATISTICS:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GVMMR0QueryStatisticsReq(pGVM, pVM, (PGVMMQUERYSTATISTICSSREQ)pReqHdr, pSession);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GVMM_RESET_STATISTICS:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GVMMR0ResetStatisticsReq(pGVM, pVM, (PGVMMRESETSTATISTICSSREQ)pReqHdr, pSession);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        /*
         * Initialize the R0 part of a VM instance.
         */
        case VMMR0_DO_VMMR0_INIT:
            rc = vmmR0InitVM(pGVM, pVM, RT_LODWORD(u64Arg), RT_HIDWORD(u64Arg));
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        /*
         * Terminate the R0 part of a VM instance.
         */
        case VMMR0_DO_VMMR0_TERM:
            rc = VMMR0TermVM(pGVM, pVM, 0 /*idCpu*/);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        /*
         * Attempt to enable hm mode and check the current setting.
         */
        case VMMR0_DO_HM_ENABLE:
            rc = HMR0EnableAllCpus(pVM);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        /*
         * Setup the hardware accelerated session.
         */
        case VMMR0_DO_HM_SETUP_VM:
            rc = HMR0SetupVM(pVM);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        /*
         * Switch to RC to execute Hypervisor function.
         */
        case VMMR0_DO_CALL_HYPERVISOR:
        {
#ifdef VBOX_WITH_RAW_MODE
            /*
             * Validate input / context.
             */
            if (RT_UNLIKELY(idCpu != 0))
                return VERR_INVALID_CPU_ID;
            if (RT_UNLIKELY(pVM->cCpus != 1))
                return VERR_INVALID_PARAMETER;
            PVMCPU pVCpu = &pVM->aCpus[idCpu];
# ifndef VBOX_WITH_2X_4GB_ADDR_SPACE_IN_R0
            if (RT_UNLIKELY(!PGMGetHyperCR3(pVCpu)))
                return VERR_PGM_NO_CR3_SHADOW_ROOT;
# endif
            if (RT_FAILURE(g_rcRawModeUsability))
                return g_rcRawModeUsability;

            /*
             * Disable interrupts.
             */
            RTCCUINTREG fFlags = ASMIntDisableFlags();

            /*
             * Get the host CPU identifiers, make sure they are valid and that
             * we've got a TSC delta for the CPU.
             */
            RTCPUID  idHostCpu;
            uint32_t iHostCpuSet = RTMpCurSetIndexAndId(&idHostCpu);
            if (RT_UNLIKELY(iHostCpuSet >= RTCPUSET_MAX_CPUS))
            {
                ASMSetFlags(fFlags);
                return VERR_INVALID_CPU_INDEX;
            }
            if (RT_UNLIKELY(!SUPIsTscDeltaAvailableForCpuSetIndex(iHostCpuSet)))
            {
                ASMSetFlags(fFlags);
                rc = SUPR0TscDeltaMeasureBySetIndex(pVM->pSession, iHostCpuSet, 0 /*fFlags*/,
                                                    2 /*cMsWaitRetry*/, 5*RT_MS_1SEC /*cMsWaitThread*/,
                                                    0 /*default cTries*/);
                if (RT_FAILURE(rc) && rc != VERR_CPU_OFFLINE)
                {
                    VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
                    return rc;
                }
            }

            /*
             * Commit the CPU identifiers.
             */
# ifdef VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI
            CPUMR0SetLApic(pVCpu, iHostCpuSet);
# endif
            pVCpu->iHostCpuSet = iHostCpuSet;
            ASMAtomicWriteU32(&pVCpu->idHostCpu, idHostCpu);

            /*
             * We might need to disable VT-x if the active switcher turns off paging.
             */
            bool fVTxDisabled;
            rc = HMR0EnterSwitcher(pVM, pVM->vmm.s.enmSwitcher, &fVTxDisabled);
            if (RT_SUCCESS(rc))
            {
                /*
                 * Go through the wormhole...
                 */
                rc = pVM->vmm.s.pfnR0ToRawMode(pVM);

                /*
                 * Re-enable VT-x before we dispatch any pending host interrupts.
                 */
                HMR0LeaveSwitcher(pVM, fVTxDisabled);

                if (   rc == VINF_EM_RAW_INTERRUPT
                    || rc == VINF_EM_RAW_INTERRUPT_HYPER)
                    TRPMR0DispatchHostInterrupt(pVM);
            }

            /*
             * Invalidate the host CPU identifiers as we restore interrupts.
             */
            pVCpu->iHostCpuSet = UINT32_MAX;
            ASMAtomicWriteU32(&pVCpu->idHostCpu, NIL_RTCPUID);
            ASMSetFlags(fFlags);

#else  /* !VBOX_WITH_RAW_MODE */
            rc = VERR_RAW_MODE_NOT_SUPPORTED;
#endif
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;
        }

        /*
         * PGM wrappers.
         */
        case VMMR0_DO_PGM_ALLOCATE_HANDY_PAGES:
            if (idCpu == NIL_VMCPUID)
                return VERR_INVALID_CPU_ID;
            rc = PGMR0PhysAllocateHandyPages(pGVM, pVM, idCpu);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_PGM_FLUSH_HANDY_PAGES:
            if (idCpu == NIL_VMCPUID)
                return VERR_INVALID_CPU_ID;
            rc = PGMR0PhysFlushHandyPages(pGVM, pVM, idCpu);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_PGM_ALLOCATE_LARGE_HANDY_PAGE:
            if (idCpu == NIL_VMCPUID)
                return VERR_INVALID_CPU_ID;
            rc = PGMR0PhysAllocateLargeHandyPage(pGVM, pVM, idCpu);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_PGM_PHYS_SETUP_IOMMU:
            if (idCpu != 0)
                return VERR_INVALID_CPU_ID;
            rc = PGMR0PhysSetupIoMmu(pGVM, pVM);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        /*
         * GMM wrappers.
         */
        case VMMR0_DO_GMM_INITIAL_RESERVATION:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0InitialReservationReq(pGVM, pVM, idCpu, (PGMMINITIALRESERVATIONREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_UPDATE_RESERVATION:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0UpdateReservationReq(pGVM, pVM, idCpu, (PGMMUPDATERESERVATIONREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_ALLOCATE_PAGES:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0AllocatePagesReq(pGVM, pVM, idCpu, (PGMMALLOCATEPAGESREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_FREE_PAGES:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0FreePagesReq(pGVM, pVM, idCpu, (PGMMFREEPAGESREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_FREE_LARGE_PAGE:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0FreeLargePageReq(pGVM, pVM, idCpu, (PGMMFREELARGEPAGEREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_QUERY_HYPERVISOR_MEM_STATS:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0QueryHypervisorMemoryStatsReq((PGMMMEMSTATSREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_QUERY_MEM_STATS:
            if (idCpu == NIL_VMCPUID)
                return VERR_INVALID_CPU_ID;
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0QueryMemoryStatsReq(pGVM, pVM, idCpu, (PGMMMEMSTATSREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_BALLOONED_PAGES:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0BalloonedPagesReq(pGVM, pVM, idCpu, (PGMMBALLOONEDPAGESREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_MAP_UNMAP_CHUNK:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0MapUnmapChunkReq(pGVM, pVM, (PGMMMAPUNMAPCHUNKREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_SEED_CHUNK:
            if (pReqHdr)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0SeedChunk(pGVM, pVM, idCpu, (RTR3PTR)u64Arg);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_REGISTER_SHARED_MODULE:
            if (idCpu == NIL_VMCPUID)
                return VERR_INVALID_CPU_ID;
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0RegisterSharedModuleReq(pGVM, pVM, idCpu, (PGMMREGISTERSHAREDMODULEREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_UNREGISTER_SHARED_MODULE:
            if (idCpu == NIL_VMCPUID)
                return VERR_INVALID_CPU_ID;
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0UnregisterSharedModuleReq(pGVM, pVM, idCpu, (PGMMUNREGISTERSHAREDMODULEREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_RESET_SHARED_MODULES:
            if (idCpu == NIL_VMCPUID)
                return VERR_INVALID_CPU_ID;
            if (    u64Arg
                ||  pReqHdr)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0ResetSharedModules(pGVM, pVM, idCpu);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

#ifdef VBOX_WITH_PAGE_SHARING
        case VMMR0_DO_GMM_CHECK_SHARED_MODULES:
        {
            if (idCpu == NIL_VMCPUID)
                return VERR_INVALID_CPU_ID;
            if (    u64Arg
                ||  pReqHdr)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0CheckSharedModules(pGVM, pVM, idCpu);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;
        }
#endif

#if defined(VBOX_STRICT) && HC_ARCH_BITS == 64
        case VMMR0_DO_GMM_FIND_DUPLICATE_PAGE:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0FindDuplicatePageReq(pGVM, pVM, (PGMMFINDDUPLICATEPAGEREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;
#endif

        case VMMR0_DO_GMM_QUERY_STATISTICS:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0QueryStatisticsReq(pGVM, pVM, (PGMMQUERYSTATISTICSSREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_GMM_RESET_STATISTICS:
            if (u64Arg)
                return VERR_INVALID_PARAMETER;
            rc = GMMR0ResetStatisticsReq(pGVM, pVM, (PGMMRESETSTATISTICSSREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        /*
         * A quick GCFGM mock-up.
         */
        /** @todo GCFGM with proper access control, ring-3 management interface and all that. */
        case VMMR0_DO_GCFGM_SET_VALUE:
        case VMMR0_DO_GCFGM_QUERY_VALUE:
        {
            if (pGVM || pVM || !pReqHdr || u64Arg || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            PGCFGMVALUEREQ pReq = (PGCFGMVALUEREQ)pReqHdr;
            if (pReq->Hdr.cbReq != sizeof(*pReq))
                return VERR_INVALID_PARAMETER;
            if (enmOperation == VMMR0_DO_GCFGM_SET_VALUE)
            {
                rc = GVMMR0SetConfig(pReq->pSession, &pReq->szName[0], pReq->u64Value);
                //if (rc == VERR_CFGM_VALUE_NOT_FOUND)
                //    rc = GMMR0SetConfig(pReq->pSession, &pReq->szName[0], pReq->u64Value);
            }
            else
            {
                rc = GVMMR0QueryConfig(pReq->pSession, &pReq->szName[0], &pReq->u64Value);
                //if (rc == VERR_CFGM_VALUE_NOT_FOUND)
                //    rc = GMMR0QueryConfig(pReq->pSession, &pReq->szName[0], &pReq->u64Value);
            }
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;
        }

        /*
         * PDM Wrappers.
         */
        case VMMR0_DO_PDM_DRIVER_CALL_REQ_HANDLER:
        {
            if (!pReqHdr || u64Arg || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            rc = PDMR0DriverCallReqHandler(pGVM, pVM, (PPDMDRIVERCALLREQHANDLERREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;
        }

        case VMMR0_DO_PDM_DEVICE_CALL_REQ_HANDLER:
        {
            if (!pReqHdr || u64Arg || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            rc = PDMR0DeviceCallReqHandler(pGVM, pVM, (PPDMDEVICECALLREQHANDLERREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;
        }

        /*
         * Requests to the internal networking service.
         */
        case VMMR0_DO_INTNET_OPEN:
        {
            PINTNETOPENREQ pReq = (PINTNETOPENREQ)pReqHdr;
            if (u64Arg || !pReq || !vmmR0IsValidSession(pVM, pReq->pSession, pSession) || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            rc = IntNetR0OpenReq(pSession, pReq);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;
        }

        case VMMR0_DO_INTNET_IF_CLOSE:
            if (u64Arg || !pReqHdr || !vmmR0IsValidSession(pVM, ((PINTNETIFCLOSEREQ)pReqHdr)->pSession, pSession) || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            rc = IntNetR0IfCloseReq(pSession, (PINTNETIFCLOSEREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;


        case VMMR0_DO_INTNET_IF_GET_BUFFER_PTRS:
            if (u64Arg || !pReqHdr || !vmmR0IsValidSession(pVM, ((PINTNETIFGETBUFFERPTRSREQ)pReqHdr)->pSession, pSession) || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            rc = IntNetR0IfGetBufferPtrsReq(pSession, (PINTNETIFGETBUFFERPTRSREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_INTNET_IF_SET_PROMISCUOUS_MODE:
            if (u64Arg || !pReqHdr || !vmmR0IsValidSession(pVM, ((PINTNETIFSETPROMISCUOUSMODEREQ)pReqHdr)->pSession, pSession) || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            rc = IntNetR0IfSetPromiscuousModeReq(pSession, (PINTNETIFSETPROMISCUOUSMODEREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_INTNET_IF_SET_MAC_ADDRESS:
            if (u64Arg || !pReqHdr || !vmmR0IsValidSession(pVM, ((PINTNETIFSETMACADDRESSREQ)pReqHdr)->pSession, pSession) || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            rc = IntNetR0IfSetMacAddressReq(pSession, (PINTNETIFSETMACADDRESSREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_INTNET_IF_SET_ACTIVE:
            if (u64Arg || !pReqHdr || !vmmR0IsValidSession(pVM, ((PINTNETIFSETACTIVEREQ)pReqHdr)->pSession, pSession) || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            rc = IntNetR0IfSetActiveReq(pSession, (PINTNETIFSETACTIVEREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_INTNET_IF_SEND:
            if (u64Arg || !pReqHdr || !vmmR0IsValidSession(pVM, ((PINTNETIFSENDREQ)pReqHdr)->pSession, pSession) || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            rc = IntNetR0IfSendReq(pSession, (PINTNETIFSENDREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_INTNET_IF_WAIT:
            if (u64Arg || !pReqHdr || !vmmR0IsValidSession(pVM, ((PINTNETIFWAITREQ)pReqHdr)->pSession, pSession) || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            rc = IntNetR0IfWaitReq(pSession, (PINTNETIFWAITREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

        case VMMR0_DO_INTNET_IF_ABORT_WAIT:
            if (u64Arg || !pReqHdr || !vmmR0IsValidSession(pVM, ((PINTNETIFWAITREQ)pReqHdr)->pSession, pSession) || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            rc = IntNetR0IfAbortWaitReq(pSession, (PINTNETIFABORTWAITREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;

#ifdef VBOX_WITH_PCI_PASSTHROUGH
        /*
         * Requests to host PCI driver service.
         */
        case VMMR0_DO_PCIRAW_REQ:
            if (u64Arg || !pReqHdr || !vmmR0IsValidSession(pVM, ((PPCIRAWSENDREQ)pReqHdr)->pSession, pSession) || idCpu != NIL_VMCPUID)
                return VERR_INVALID_PARAMETER;
            rc = PciRawR0ProcessReq(pGVM, pVM, pSession, (PPCIRAWSENDREQ)pReqHdr);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;
#endif
        /*
         * For profiling.
         */
        case VMMR0_DO_NOP:
        case VMMR0_DO_SLOW_NOP:
            return VINF_SUCCESS;

        /*
         * For testing Ring-0 APIs invoked in this environment.
         */
        case VMMR0_DO_TESTS:
            /** @todo make new test */
            return VINF_SUCCESS;


#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
        case VMMR0_DO_TEST_SWITCHER3264:
            if (idCpu == NIL_VMCPUID)
                return VERR_INVALID_CPU_ID;
            rc = HMR0TestSwitcher3264(pVM);
            VMM_CHECK_SMAP_CHECK2(pVM, RT_NOTHING);
            break;
#endif
        default:
            /*
             * We're returning VERR_NOT_SUPPORT here so we've got something else
             * than -1 which the interrupt gate glue code might return.
             */
            Log(("operation %#x is not supported\n", enmOperation));
            return VERR_NOT_SUPPORTED;
    }
    return rc;
}


/**
 * Argument for vmmR0EntryExWrapper containing the arguments for VMMR0EntryEx.
 */
typedef struct VMMR0ENTRYEXARGS
{
    PGVM                pGVM;
    PVM                 pVM;
    VMCPUID             idCpu;
    VMMR0OPERATION      enmOperation;
    PSUPVMMR0REQHDR     pReq;
    uint64_t            u64Arg;
    PSUPDRVSESSION      pSession;
} VMMR0ENTRYEXARGS;
/** Pointer to a vmmR0EntryExWrapper argument package. */
typedef VMMR0ENTRYEXARGS *PVMMR0ENTRYEXARGS;

/**
 * This is just a longjmp wrapper function for VMMR0EntryEx calls.
 *
 * @returns VBox status code.
 * @param   pvArgs      The argument package
 */
static DECLCALLBACK(int) vmmR0EntryExWrapper(void *pvArgs)
{
    return vmmR0EntryExWorker(((PVMMR0ENTRYEXARGS)pvArgs)->pGVM,
                              ((PVMMR0ENTRYEXARGS)pvArgs)->pVM,
                              ((PVMMR0ENTRYEXARGS)pvArgs)->idCpu,
                              ((PVMMR0ENTRYEXARGS)pvArgs)->enmOperation,
                              ((PVMMR0ENTRYEXARGS)pvArgs)->pReq,
                              ((PVMMR0ENTRYEXARGS)pvArgs)->u64Arg,
                              ((PVMMR0ENTRYEXARGS)pvArgs)->pSession);
}


/**
 * The Ring 0 entry point, called by the support library (SUP).
 *
 * @returns VBox status code.
 * @param   pGVM            The global (ring-0) VM structure.
 * @param   pVM             The cross context VM structure.
 * @param   idCpu           Virtual CPU ID argument. Must be NIL_VMCPUID if pVM
 *                          is NIL_RTR0PTR, and may be NIL_VMCPUID if it isn't
 * @param   enmOperation    Which operation to execute.
 * @param   pReq            Pointer to the SUPVMMR0REQHDR packet. Optional.
 * @param   u64Arg          Some simple constant argument.
 * @param   pSession        The session of the caller.
 * @remarks Assume called with interrupts _enabled_.
 */
VMMR0DECL(int) VMMR0EntryEx(PGVM pGVM, PVM pVM, VMCPUID idCpu, VMMR0OPERATION enmOperation,
                            PSUPVMMR0REQHDR pReq, uint64_t u64Arg, PSUPDRVSESSION pSession)
{
    /*
     * Requests that should only happen on the EMT thread will be
     * wrapped in a setjmp so we can assert without causing trouble.
     */
    if (   pVM  != NULL
        && pGVM != NULL
        && idCpu < pGVM->cCpus
        && pVM->pVMR0 != NULL)
    {
        switch (enmOperation)
        {
            /* These might/will be called before VMMR3Init. */
            case VMMR0_DO_GMM_INITIAL_RESERVATION:
            case VMMR0_DO_GMM_UPDATE_RESERVATION:
            case VMMR0_DO_GMM_ALLOCATE_PAGES:
            case VMMR0_DO_GMM_FREE_PAGES:
            case VMMR0_DO_GMM_BALLOONED_PAGES:
            /* On the mac we might not have a valid jmp buf, so check these as well. */
            case VMMR0_DO_VMMR0_INIT:
            case VMMR0_DO_VMMR0_TERM:
            {
                PGVMCPU pGVCpu = &pGVM->aCpus[idCpu];
                PVMCPU  pVCpu  = &pVM->aCpus[idCpu];
                RTNATIVETHREAD hNativeThread = RTThreadNativeSelf();
                if (RT_LIKELY(   pGVCpu->hEMT           == hNativeThread
                              && pVCpu->hNativeThreadR0 == hNativeThread))
                {
                    if (!pVCpu->vmm.s.CallRing3JmpBufR0.pvSavedStack)
                        break;

                    /** @todo validate this EMT claim... GVM knows. */
                    VMMR0ENTRYEXARGS Args;
                    Args.pGVM = pGVM;
                    Args.pVM = pVM;
                    Args.idCpu = idCpu;
                    Args.enmOperation = enmOperation;
                    Args.pReq = pReq;
                    Args.u64Arg = u64Arg;
                    Args.pSession = pSession;
                    return vmmR0CallRing3SetJmpEx(&pVCpu->vmm.s.CallRing3JmpBufR0, vmmR0EntryExWrapper, &Args);
                }
                return VERR_VM_THREAD_NOT_EMT;
            }

            default:
                break;
        }
    }
    return vmmR0EntryExWorker(pGVM, pVM, idCpu, enmOperation, pReq, u64Arg, pSession);
}


/**
 * Checks whether we've armed the ring-0 long jump machinery.
 *
 * @returns @c true / @c false
 * @param   pVCpu           The cross context virtual CPU structure.
 * @thread  EMT
 * @sa      VMMIsLongJumpArmed
 */
VMMR0_INT_DECL(bool) VMMR0IsLongJumpArmed(PVMCPU pVCpu)
{
#ifdef RT_ARCH_X86
    return pVCpu->vmm.s.CallRing3JmpBufR0.eip
        && !pVCpu->vmm.s.CallRing3JmpBufR0.fInRing3Call;
#else
    return pVCpu->vmm.s.CallRing3JmpBufR0.rip
        && !pVCpu->vmm.s.CallRing3JmpBufR0.fInRing3Call;
#endif
}


/**
 * Checks whether we've done a ring-3 long jump.
 *
 * @returns @c true / @c false
 * @param   pVCpu       The cross context virtual CPU structure.
 * @thread  EMT
 */
VMMR0_INT_DECL(bool) VMMR0IsInRing3LongJump(PVMCPU pVCpu)
{
    return pVCpu->vmm.s.CallRing3JmpBufR0.fInRing3Call;
}


/**
 * Internal R0 logger worker: Flush logger.
 *
 * @param   pLogger     The logger instance to flush.
 * @remark  This function must be exported!
 */
VMMR0DECL(void) vmmR0LoggerFlush(PRTLOGGER pLogger)
{
#ifdef LOG_ENABLED
    /*
     * Convert the pLogger into a VM handle and 'call' back to Ring-3.
     * (This is a bit paranoid code.)
     */
    PVMMR0LOGGER pR0Logger = (PVMMR0LOGGER)((uintptr_t)pLogger - RT_OFFSETOF(VMMR0LOGGER, Logger));
    if (    !VALID_PTR(pR0Logger)
        ||  !VALID_PTR(pR0Logger + 1)
        ||  pLogger->u32Magic != RTLOGGER_MAGIC)
    {
# ifdef DEBUG
        SUPR0Printf("vmmR0LoggerFlush: pLogger=%p!\n", pLogger);
# endif
        return;
    }
    if (pR0Logger->fFlushingDisabled)
        return; /* quietly */

    PVM pVM = pR0Logger->pVM;
    if (    !VALID_PTR(pVM)
        ||  pVM->pVMR0 != pVM)
    {
# ifdef DEBUG
        SUPR0Printf("vmmR0LoggerFlush: pVM=%p! pVMR0=%p! pLogger=%p\n", pVM, pVM->pVMR0, pLogger);
# endif
        return;
    }

    PVMCPU pVCpu = VMMGetCpu(pVM);
    if (pVCpu)
    {
        /*
         * Check that the jump buffer is armed.
         */
# ifdef RT_ARCH_X86
        if (    !pVCpu->vmm.s.CallRing3JmpBufR0.eip
            ||  pVCpu->vmm.s.CallRing3JmpBufR0.fInRing3Call)
# else
        if (    !pVCpu->vmm.s.CallRing3JmpBufR0.rip
            ||  pVCpu->vmm.s.CallRing3JmpBufR0.fInRing3Call)
# endif
        {
# ifdef DEBUG
            SUPR0Printf("vmmR0LoggerFlush: Jump buffer isn't armed!\n");
# endif
            return;
        }
        VMMRZCallRing3(pVM, pVCpu, VMMCALLRING3_VMM_LOGGER_FLUSH, 0);
    }
# ifdef DEBUG
    else
        SUPR0Printf("vmmR0LoggerFlush: invalid VCPU context!\n");
# endif
#else
    NOREF(pLogger);
#endif  /* LOG_ENABLED */
}

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
VMMR0DECL(size_t) vmmR0LoggerPrefix(PRTLOGGER pLogger, char *pchBuf, size_t cchBuf, void *pvUser)
{
    NOREF(pvUser);
#ifdef LOG_ENABLED
    PVMMR0LOGGER pR0Logger = (PVMMR0LOGGER)((uintptr_t)pLogger - RT_OFFSETOF(VMMR0LOGGER, Logger));
    if (    !VALID_PTR(pR0Logger)
        ||  !VALID_PTR(pR0Logger + 1)
        ||  pLogger->u32Magic != RTLOGGER_MAGIC
        ||  cchBuf < 2)
        return 0;

    static const char s_szHex[17] = "0123456789abcdef";
    VMCPUID const     idCpu       = pR0Logger->idCpu;
    pchBuf[1] = s_szHex[ idCpu       & 15];
    pchBuf[0] = s_szHex[(idCpu >> 4) & 15];

    return 2;
#else
    NOREF(pLogger); NOREF(pchBuf); NOREF(cchBuf);
    return 0;
#endif
}

#ifdef LOG_ENABLED

/**
 * Disables flushing of the ring-0 debug log.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR0_INT_DECL(void) VMMR0LogFlushDisable(PVMCPU pVCpu)
{
    if (pVCpu->vmm.s.pR0LoggerR0)
        pVCpu->vmm.s.pR0LoggerR0->fFlushingDisabled = true;
}


/**
 * Enables flushing of the ring-0 debug log.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR0_INT_DECL(void) VMMR0LogFlushEnable(PVMCPU pVCpu)
{
    if (pVCpu->vmm.s.pR0LoggerR0)
        pVCpu->vmm.s.pR0LoggerR0->fFlushingDisabled = false;
}


/**
 * Checks if log flushing is disabled or not.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR0_INT_DECL(bool) VMMR0IsLogFlushDisabled(PVMCPU pVCpu)
{
    if (pVCpu->vmm.s.pR0LoggerR0)
        return pVCpu->vmm.s.pR0LoggerR0->fFlushingDisabled;
    return true;
}
#endif /* LOG_ENABLED */

/**
 * Jump back to ring-3 if we're the EMT and the longjmp is armed.
 *
 * @returns true if the breakpoint should be hit, false if it should be ignored.
 */
DECLEXPORT(bool) RTCALL RTAssertShouldPanic(void)
{
#if 0
    return true;
#else
    PVM pVM = GVMMR0GetVMByEMT(NIL_RTNATIVETHREAD);
    if (pVM)
    {
        PVMCPU pVCpu = VMMGetCpu(pVM);

        if (pVCpu)
        {
#ifdef RT_ARCH_X86
            if (    pVCpu->vmm.s.CallRing3JmpBufR0.eip
                &&  !pVCpu->vmm.s.CallRing3JmpBufR0.fInRing3Call)
#else
            if (    pVCpu->vmm.s.CallRing3JmpBufR0.rip
                &&  !pVCpu->vmm.s.CallRing3JmpBufR0.fInRing3Call)
#endif
            {
                int rc = VMMRZCallRing3(pVM, pVCpu, VMMCALLRING3_VM_R0_ASSERTION, 0);
                return RT_FAILURE_NP(rc);
            }
        }
    }
#ifdef RT_OS_LINUX
    return true;
#else
    return false;
#endif
#endif
}


/**
 * Override this so we can push it up to ring-3.
 *
 * @param   pszExpr     Expression. Can be NULL.
 * @param   uLine       Location line number.
 * @param   pszFile     Location file name.
 * @param   pszFunction Location function name.
 */
DECLEXPORT(void) RTCALL RTAssertMsg1Weak(const char *pszExpr, unsigned uLine, const char *pszFile, const char *pszFunction)
{
    /*
     * To the log.
     */
    LogAlways(("\n!!R0-Assertion Failed!!\n"
               "Expression: %s\n"
               "Location  : %s(%d) %s\n",
               pszExpr, pszFile, uLine, pszFunction));

    /*
     * To the global VMM buffer.
     */
    PVM pVM = GVMMR0GetVMByEMT(NIL_RTNATIVETHREAD);
    if (pVM)
        RTStrPrintf(pVM->vmm.s.szRing0AssertMsg1, sizeof(pVM->vmm.s.szRing0AssertMsg1),
                    "\n!!R0-Assertion Failed!!\n"
                    "Expression: %.*s\n"
                    "Location  : %s(%d) %s\n",
                    sizeof(pVM->vmm.s.szRing0AssertMsg1) / 4 * 3, pszExpr,
                    pszFile, uLine, pszFunction);

    /*
     * Continue the normal way.
     */
    RTAssertMsg1(pszExpr, uLine, pszFile, pszFunction);
}


/**
 * Callback for RTLogFormatV which writes to the ring-3 log port.
 * See PFNLOGOUTPUT() for details.
 */
static DECLCALLBACK(size_t) rtLogOutput(void *pv, const char *pachChars, size_t cbChars)
{
    for (size_t i = 0; i < cbChars; i++)
    {
        LogAlways(("%c", pachChars[i])); NOREF(pachChars);
    }

    NOREF(pv);
    return cbChars;
}


/**
 * Override this so we can push it up to ring-3.
 *
 * @param   pszFormat   The format string.
 * @param   va          Arguments.
 */
DECLEXPORT(void) RTCALL RTAssertMsg2WeakV(const char *pszFormat, va_list va)
{
    va_list vaCopy;

    /*
     * Push the message to the loggers.
     */
    PRTLOGGER pLog = RTLogGetDefaultInstance(); /* Don't initialize it here... */
    if (pLog)
    {
        va_copy(vaCopy, va);
        RTLogFormatV(rtLogOutput, pLog, pszFormat, vaCopy);
        va_end(vaCopy);
    }
    pLog = RTLogRelGetDefaultInstance();
    if (pLog)
    {
        va_copy(vaCopy, va);
        RTLogFormatV(rtLogOutput, pLog, pszFormat, vaCopy);
        va_end(vaCopy);
    }

    /*
     * Push it to the global VMM buffer.
     */
    PVM pVM = GVMMR0GetVMByEMT(NIL_RTNATIVETHREAD);
    if (pVM)
    {
        va_copy(vaCopy, va);
        RTStrPrintfV(pVM->vmm.s.szRing0AssertMsg2, sizeof(pVM->vmm.s.szRing0AssertMsg2), pszFormat, vaCopy);
        va_end(vaCopy);
    }

    /*
     * Continue the normal way.
     */
    RTAssertMsg2V(pszFormat, va);
}

