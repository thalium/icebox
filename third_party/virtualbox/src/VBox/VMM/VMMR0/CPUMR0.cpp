/* $Id: CPUMR0.cpp $ */
/** @file
 * CPUM - Host Context Ring 0.
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
#define LOG_GROUP LOG_GROUP_CPUM
#include <VBox/vmm/cpum.h>
#include "CPUMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/vmm/hm.h>
#include <iprt/assert.h>
#include <iprt/asm-amd64-x86.h>
#ifdef VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI
# include <iprt/mem.h>
# include <iprt/memobj.h>
# include <VBox/apic.h>
#endif
#include <iprt/x86.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
#ifdef VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI
/**
 * Local APIC mappings.
 */
typedef struct CPUMHOSTLAPIC
{
    /** Indicates that the entry is in use and have valid data. */
    bool        fEnabled;
    /** Whether it's operating in X2APIC mode (EXTD). */
    bool        fX2Apic;
    /** The APIC version number. */
    uint32_t    uVersion;
    /** The physical address of the APIC registers. */
    RTHCPHYS    PhysBase;
    /** The memory object entering the physical address. */
    RTR0MEMOBJ  hMemObj;
    /** The mapping object for hMemObj. */
    RTR0MEMOBJ  hMapObj;
    /** The mapping address APIC registers.
     * @remarks Different CPUs may use the same physical address to map their
     *          APICs, so this pointer is only valid when on the CPU owning the
     *          APIC. */
    void       *pv;
} CPUMHOSTLAPIC;
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#ifdef VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI
static CPUMHOSTLAPIC g_aLApics[RTCPUSET_MAX_CPUS];
#endif

/**
 * CPUID bits to unify among all cores.
 */
static struct
{
    uint32_t uLeaf;  /**< Leaf to check. */
    uint32_t uEcx;   /**< which bits in ecx to unify between CPUs. */
    uint32_t uEdx;   /**< which bits in edx to unify between CPUs. */
}
const g_aCpuidUnifyBits[] =
{
    {
        0x00000001,
        X86_CPUID_FEATURE_ECX_CX16 | X86_CPUID_FEATURE_ECX_MONITOR,
        X86_CPUID_FEATURE_EDX_CX8
    }
};



/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
#ifdef VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI
static int  cpumR0MapLocalApics(void);
static void cpumR0UnmapLocalApics(void);
#endif
static int  cpumR0SaveHostDebugState(PVMCPU pVCpu);


/**
 * Does the Ring-0 CPU initialization once during module load.
 * XXX Host-CPU hot-plugging?
 */
VMMR0_INT_DECL(int) CPUMR0ModuleInit(void)
{
    int rc = VINF_SUCCESS;
#ifdef VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI
    rc = cpumR0MapLocalApics();
#endif
    return rc;
}


/**
 * Terminate the module.
 */
VMMR0_INT_DECL(int) CPUMR0ModuleTerm(void)
{
#ifdef VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI
    cpumR0UnmapLocalApics();
#endif
    return VINF_SUCCESS;
}


/**
 *
 *
 * Check the CPUID features of this particular CPU and disable relevant features
 * for the guest which do not exist on this CPU. We have seen systems where the
 * X86_CPUID_FEATURE_ECX_MONITOR feature flag is only set on some host CPUs, see
 * @bugref{5436}.
 *
 * @note This function might be called simultaneously on more than one CPU!
 *
 * @param   idCpu       The identifier for the CPU the function is called on.
 * @param   pvUser1     Pointer to the VM structure.
 * @param   pvUser2     Ignored.
 */
static DECLCALLBACK(void) cpumR0CheckCpuid(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    PVM     pVM   = (PVM)pvUser1;

    NOREF(idCpu); NOREF(pvUser2);
    for (uint32_t i = 0; i < RT_ELEMENTS(g_aCpuidUnifyBits); i++)
    {
        /* Note! Cannot use cpumCpuIdGetLeaf from here because we're not
                 necessarily in the VM process context.  So, we using the
                 legacy arrays as temporary storage. */

        uint32_t   uLeaf = g_aCpuidUnifyBits[i].uLeaf;
        PCPUMCPUID pLegacyLeaf;
        if (uLeaf < RT_ELEMENTS(pVM->cpum.s.aGuestCpuIdPatmStd))
            pLegacyLeaf = &pVM->cpum.s.aGuestCpuIdPatmStd[uLeaf];
        else if (uLeaf - UINT32_C(0x80000000) < RT_ELEMENTS(pVM->cpum.s.aGuestCpuIdPatmExt))
            pLegacyLeaf = &pVM->cpum.s.aGuestCpuIdPatmExt[uLeaf - UINT32_C(0x80000000)];
        else if (uLeaf - UINT32_C(0xc0000000) < RT_ELEMENTS(pVM->cpum.s.aGuestCpuIdPatmCentaur))
            pLegacyLeaf = &pVM->cpum.s.aGuestCpuIdPatmCentaur[uLeaf - UINT32_C(0xc0000000)];
        else
            continue;

        uint32_t eax, ebx, ecx, edx;
        ASMCpuIdExSlow(uLeaf, 0, 0, 0, &eax, &ebx, &ecx, &edx);

        ASMAtomicAndU32(&pLegacyLeaf->uEcx, ecx | ~g_aCpuidUnifyBits[i].uEcx);
        ASMAtomicAndU32(&pLegacyLeaf->uEdx, edx | ~g_aCpuidUnifyBits[i].uEdx);
    }
}


/**
 * Does Ring-0 CPUM initialization.
 *
 * This is mainly to check that the Host CPU mode is compatible
 * with VBox.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR0_INT_DECL(int) CPUMR0InitVM(PVM pVM)
{
    LogFlow(("CPUMR0Init: %p\n", pVM));

    /*
     * Check CR0 & CR4 flags.
     */
    uint32_t u32CR0 = ASMGetCR0();
    if ((u32CR0 & (X86_CR0_PE | X86_CR0_PG)) != (X86_CR0_PE | X86_CR0_PG)) /* a bit paranoid perhaps.. */
    {
        Log(("CPUMR0Init: PE or PG not set. cr0=%#x\n", u32CR0));
        return VERR_UNSUPPORTED_CPU_MODE;
    }

    /*
     * Check for sysenter and syscall usage.
     */
    if (ASMHasCpuId())
    {
        /*
         * SYSENTER/SYSEXIT
         *
         * Intel docs claim you should test both the flag and family, model &
         * stepping because some Pentium Pro CPUs have the SEP cpuid flag set,
         * but don't support it.  AMD CPUs may support this feature in legacy
         * mode, they've banned it from long mode.  Since we switch to 32-bit
         * mode when entering raw-mode context the feature would become
         * accessible again on AMD CPUs, so we have to check regardless of
         * host bitness.
         */
        uint32_t u32CpuVersion;
        uint32_t u32Dummy;
        uint32_t fFeatures;
        ASMCpuId(1, &u32CpuVersion, &u32Dummy, &u32Dummy, &fFeatures);
        uint32_t const u32Family   = u32CpuVersion >> 8;
        uint32_t const u32Model    = (u32CpuVersion >> 4) & 0xF;
        uint32_t const u32Stepping = u32CpuVersion & 0xF;
        if (    (fFeatures & X86_CPUID_FEATURE_EDX_SEP)
            &&  (   u32Family   != 6    /* (> pentium pro) */
                 || u32Model    >= 3
                 || u32Stepping >= 3
                 || !ASMIsIntelCpu())
           )
        {
            /*
             * Read the MSR and see if it's in use or not.
             */
            uint32_t u32 = ASMRdMsr_Low(MSR_IA32_SYSENTER_CS);
            if (u32)
            {
                pVM->cpum.s.fHostUseFlags |= CPUM_USE_SYSENTER;
                Log(("CPUMR0Init: host uses sysenter cs=%08x%08x\n", ASMRdMsr_High(MSR_IA32_SYSENTER_CS), u32));
            }
        }

        /*
         * SYSCALL/SYSRET
         *
         * This feature is indicated by the SEP bit returned in EDX by CPUID
         * function 0x80000001.  Intel CPUs only supports this feature in
         * long mode.  Since we're not running 64-bit guests in raw-mode there
         * are no issues with 32-bit intel hosts.
         */
        uint32_t cExt = 0;
        ASMCpuId(0x80000000, &cExt, &u32Dummy, &u32Dummy, &u32Dummy);
        if (ASMIsValidExtRange(cExt))
        {
            uint32_t fExtFeaturesEDX = ASMCpuId_EDX(0x80000001);
            if (fExtFeaturesEDX & X86_CPUID_EXT_FEATURE_EDX_SYSCALL)
            {
#ifdef RT_ARCH_X86
                if (!ASMIsIntelCpu())
#endif
                {
                    uint64_t fEfer = ASMRdMsr(MSR_K6_EFER);
                    if (fEfer & MSR_K6_EFER_SCE)
                    {
                        pVM->cpum.s.fHostUseFlags |= CPUM_USE_SYSCALL;
                        Log(("CPUMR0Init: host uses syscall\n"));
                    }
                }
            }
        }

        /*
         * Unify/cross check some CPUID feature bits on all available CPU cores
         * and threads.  We've seen CPUs where the monitor support differed.
         *
         * Because the hyper heap isn't always mapped into ring-0, we cannot
         * access it from a RTMpOnAll callback.  We use the legacy CPUID arrays
         * as temp ring-0 accessible memory instead, ASSUMING that they're all
         * up to date when we get here.
         */
        RTMpOnAll(cpumR0CheckCpuid, pVM, NULL);

        for (uint32_t i = 0; i < RT_ELEMENTS(g_aCpuidUnifyBits); i++)
        {
            bool            fIgnored;
            uint32_t        uLeaf = g_aCpuidUnifyBits[i].uLeaf;
            PCPUMCPUIDLEAF  pLeaf = cpumCpuIdGetLeafEx(pVM, uLeaf, 0, &fIgnored);
            if (pLeaf)
            {
                PCPUMCPUID pLegacyLeaf;
                if (uLeaf < RT_ELEMENTS(pVM->cpum.s.aGuestCpuIdPatmStd))
                    pLegacyLeaf = &pVM->cpum.s.aGuestCpuIdPatmStd[uLeaf];
                else if (uLeaf - UINT32_C(0x80000000) < RT_ELEMENTS(pVM->cpum.s.aGuestCpuIdPatmExt))
                    pLegacyLeaf = &pVM->cpum.s.aGuestCpuIdPatmExt[uLeaf - UINT32_C(0x80000000)];
                else if (uLeaf - UINT32_C(0xc0000000) < RT_ELEMENTS(pVM->cpum.s.aGuestCpuIdPatmCentaur))
                    pLegacyLeaf = &pVM->cpum.s.aGuestCpuIdPatmCentaur[uLeaf - UINT32_C(0xc0000000)];
                else
                    continue;

                pLeaf->uEcx = pLegacyLeaf->uEcx;
                pLeaf->uEdx = pLegacyLeaf->uEdx;
            }
        }

    }


    /*
     * Check if debug registers are armed.
     * This ASSUMES that DR7.GD is not set, or that it's handled transparently!
     */
    uint32_t u32DR7 = ASMGetDR7();
    if (u32DR7 & X86_DR7_ENABLED_MASK)
    {
        for (VMCPUID i = 0; i < pVM->cCpus; i++)
            pVM->aCpus[i].cpum.s.fUseFlags |= CPUM_USE_DEBUG_REGS_HOST;
        Log(("CPUMR0Init: host uses debug registers (dr7=%x)\n", u32DR7));
    }

    return VINF_SUCCESS;
}


/**
 * Trap handler for device-not-available fault (\#NM).
 * Device not available, FP or (F)WAIT instruction.
 *
 * @returns VBox status code.
 * @retval VINF_SUCCESS           if the guest FPU state is loaded.
 * @retval VINF_EM_RAW_GUEST_TRAP if it is a guest trap.
 * @retval VINF_CPUM_HOST_CR0_MODIFIED if we modified the host CR0.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR0_INT_DECL(int) CPUMR0Trap07Handler(PVM pVM, PVMCPU pVCpu)
{
    Assert(pVM->cpum.s.HostFeatures.fFxSaveRstor);
    Assert(ASMGetCR4() & X86_CR4_OSFXSR);

    /* If the FPU state has already been loaded, then it's a guest trap. */
    if (CPUMIsGuestFPUStateActive(pVCpu))
    {
        Assert(    ((pVCpu->cpum.s.Guest.cr0 & (X86_CR0_MP | X86_CR0_EM | X86_CR0_TS)) == (X86_CR0_MP | X86_CR0_TS))
               ||  ((pVCpu->cpum.s.Guest.cr0 & (X86_CR0_MP | X86_CR0_EM | X86_CR0_TS)) == (X86_CR0_MP | X86_CR0_TS | X86_CR0_EM)));
        return VINF_EM_RAW_GUEST_TRAP;
    }

    /*
     * There are two basic actions:
     *   1. Save host fpu and restore guest fpu.
     *   2. Generate guest trap.
     *
     * When entering the hypervisor we'll always enable MP (for proper wait
     * trapping) and TS (for intercepting all fpu/mmx/sse stuff). The EM flag
     * is taken from the guest OS in order to get proper SSE handling.
     *
     *
     * Actions taken depending on the guest CR0 flags:
     *
     *   3    2    1
     *  TS | EM | MP | FPUInstr | WAIT :: VMM Action
     * ------------------------------------------------------------------------
     *   0 |  0 |  0 | Exec     | Exec :: Clear TS & MP, Save HC, Load GC.
     *   0 |  0 |  1 | Exec     | Exec :: Clear TS, Save HC, Load GC.
     *   0 |  1 |  0 | #NM      | Exec :: Clear TS & MP, Save HC, Load GC.
     *   0 |  1 |  1 | #NM      | Exec :: Clear TS, Save HC, Load GC.
     *   1 |  0 |  0 | #NM      | Exec :: Clear MP, Save HC, Load GC. (EM is already cleared.)
     *   1 |  0 |  1 | #NM      | #NM  :: Go to guest taking trap there.
     *   1 |  1 |  0 | #NM      | Exec :: Clear MP, Save HC, Load GC. (EM is already set.)
     *   1 |  1 |  1 | #NM      | #NM  :: Go to guest taking trap there.
     */

    switch (pVCpu->cpum.s.Guest.cr0 & (X86_CR0_MP | X86_CR0_EM | X86_CR0_TS))
    {
        case X86_CR0_MP | X86_CR0_TS:
        case X86_CR0_MP | X86_CR0_TS | X86_CR0_EM:
            return VINF_EM_RAW_GUEST_TRAP;
        default:
            break;
    }

    return CPUMR0LoadGuestFPU(pVM, pVCpu);
}


/**
 * Saves the host-FPU/XMM state (if necessary) and (always) loads the guest-FPU
 * state into the CPU.
 *
 * @returns VINF_SUCCESS on success, host CR0 unmodified.
 * @returns VINF_CPUM_HOST_CR0_MODIFIED on success when the host CR0 was
 *          modified and VT-x needs to update the value in the VMCS.
 *
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure.
 */
VMMR0_INT_DECL(int) CPUMR0LoadGuestFPU(PVM pVM, PVMCPU pVCpu)
{
    int rc = VINF_SUCCESS;
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    Assert(!(pVCpu->cpum.s.fUseFlags & CPUM_USED_FPU_GUEST));
    Assert(!(pVCpu->cpum.s.fUseFlags & CPUM_SYNC_FPU_STATE));

#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
    if (CPUMIsGuestInLongModeEx(&pVCpu->cpum.s.Guest))
    {
        Assert(!(pVCpu->cpum.s.fUseFlags & CPUM_USED_MANUAL_XMM_RESTORE));

        /* Save the host state if necessary. */
        if (!(pVCpu->cpum.s.fUseFlags & CPUM_USED_FPU_HOST))
            rc = cpumRZSaveHostFPUState(&pVCpu->cpum.s);

        /* Restore the state on entry as we need to be in 64-bit mode to access the full state. */
        pVCpu->cpum.s.fUseFlags |= CPUM_SYNC_FPU_STATE;

        Assert(   (pVCpu->cpum.s.fUseFlags & (CPUM_USED_FPU_HOST | CPUM_USED_FPU_SINCE_REM))
               ==                            (CPUM_USED_FPU_HOST | CPUM_USED_FPU_SINCE_REM));
    }
    else
#endif
    {
        if (!pVM->cpum.s.HostFeatures.fLeakyFxSR)
        {
            Assert(!(pVCpu->cpum.s.fUseFlags & CPUM_USED_MANUAL_XMM_RESTORE));
            rc = cpumR0SaveHostRestoreGuestFPUState(&pVCpu->cpum.s);
        }
        else
        {
            Assert(!(pVCpu->cpum.s.fUseFlags & CPUM_USED_MANUAL_XMM_RESTORE) || (pVCpu->cpum.s.fUseFlags & CPUM_USED_FPU_HOST));
            /** @todo r=ramshankar: Can't we used a cached value here
             *        instead of reading the MSR? host EFER doesn't usually
             *        change. */
            uint64_t uHostEfer = ASMRdMsr(MSR_K6_EFER);
            if (!(uHostEfer & MSR_K6_EFER_FFXSR))
                rc = cpumR0SaveHostRestoreGuestFPUState(&pVCpu->cpum.s);
            else
            {
                RTCCUINTREG const uSavedFlags = ASMIntDisableFlags();
                pVCpu->cpum.s.fUseFlags |= CPUM_USED_MANUAL_XMM_RESTORE;
                ASMWrMsr(MSR_K6_EFER, uHostEfer & ~MSR_K6_EFER_FFXSR);
                rc = cpumR0SaveHostRestoreGuestFPUState(&pVCpu->cpum.s);
                ASMWrMsr(MSR_K6_EFER, uHostEfer | MSR_K6_EFER_FFXSR);
                ASMSetFlags(uSavedFlags);
            }
        }
        Assert(   (pVCpu->cpum.s.fUseFlags & (CPUM_USED_FPU_GUEST | CPUM_USED_FPU_HOST | CPUM_USED_FPU_SINCE_REM))
               ==                            (CPUM_USED_FPU_GUEST | CPUM_USED_FPU_HOST | CPUM_USED_FPU_SINCE_REM));
    }
    return rc;
}


/**
 * Saves the guest FPU/XMM state if needed, restores the host FPU/XMM state as
 * needed.
 *
 * @returns true if we saved the guest state.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMMR0_INT_DECL(bool) CPUMR0FpuStateMaybeSaveGuestAndRestoreHost(PVMCPU pVCpu)
{
    bool fSavedGuest;
    Assert(pVCpu->CTX_SUFF(pVM)->cpum.s.HostFeatures.fFxSaveRstor);
    Assert(ASMGetCR4() & X86_CR4_OSFXSR);
    if (pVCpu->cpum.s.fUseFlags & (CPUM_USED_FPU_GUEST | CPUM_USED_FPU_HOST))
    {
        fSavedGuest = RT_BOOL(pVCpu->cpum.s.fUseFlags & CPUM_USED_FPU_GUEST);
#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
        if (CPUMIsGuestInLongModeEx(&pVCpu->cpum.s.Guest))
        {
            if (pVCpu->cpum.s.fUseFlags & CPUM_USED_FPU_GUEST)
            {
                Assert(!(pVCpu->cpum.s.fUseFlags & CPUM_SYNC_FPU_STATE));
                HMR0SaveFPUState(pVCpu->CTX_SUFF(pVM), pVCpu, &pVCpu->cpum.s.Guest);
            }
            else
                pVCpu->cpum.s.fUseFlags &= ~CPUM_SYNC_FPU_STATE;
            cpumR0RestoreHostFPUState(&pVCpu->cpum.s);
        }
        else
#endif
        {
            if (!(pVCpu->cpum.s.fUseFlags & CPUM_USED_MANUAL_XMM_RESTORE))
                cpumR0SaveGuestRestoreHostFPUState(&pVCpu->cpum.s);
            else
            {
                /* Temporarily clear MSR_K6_EFER_FFXSR or else we'll be unable to
                   save/restore the XMM state with fxsave/fxrstor. */
                uint64_t uHostEfer = ASMRdMsr(MSR_K6_EFER);
                if (uHostEfer & MSR_K6_EFER_FFXSR)
                {
                    RTCCUINTREG const uSavedFlags = ASMIntDisableFlags();
                    ASMWrMsr(MSR_K6_EFER, uHostEfer & ~MSR_K6_EFER_FFXSR);
                    cpumR0SaveGuestRestoreHostFPUState(&pVCpu->cpum.s);
                    ASMWrMsr(MSR_K6_EFER, uHostEfer | MSR_K6_EFER_FFXSR);
                    ASMSetFlags(uSavedFlags);
                }
                else
                    cpumR0SaveGuestRestoreHostFPUState(&pVCpu->cpum.s);
                pVCpu->cpum.s.fUseFlags &= ~CPUM_USED_MANUAL_XMM_RESTORE;
            }
        }
    }
    else
        fSavedGuest = false;
    Assert(!(  pVCpu->cpum.s.fUseFlags
             & (CPUM_USED_FPU_GUEST | CPUM_USED_FPU_HOST | CPUM_SYNC_FPU_STATE | CPUM_USED_MANUAL_XMM_RESTORE)));
    return fSavedGuest;
}


/**
 * Saves the host debug state, setting CPUM_USED_HOST_DEBUG_STATE and loading
 * DR7 with safe values.
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
static int cpumR0SaveHostDebugState(PVMCPU pVCpu)
{
    /*
     * Save the host state.
     */
    pVCpu->cpum.s.Host.dr0 = ASMGetDR0();
    pVCpu->cpum.s.Host.dr1 = ASMGetDR1();
    pVCpu->cpum.s.Host.dr2 = ASMGetDR2();
    pVCpu->cpum.s.Host.dr3 = ASMGetDR3();
    pVCpu->cpum.s.Host.dr6 = ASMGetDR6();
    /** @todo dr7 might already have been changed to 0x400; don't care right now as it's harmless. */
    pVCpu->cpum.s.Host.dr7 = ASMGetDR7();

    /* Preemption paranoia. */
    ASMAtomicOrU32(&pVCpu->cpum.s.fUseFlags, CPUM_USED_DEBUG_REGS_HOST);

    /*
     * Make sure DR7 is harmless or else we could trigger breakpoints when
     * load guest or hypervisor DRx values later.
     */
    if (pVCpu->cpum.s.Host.dr7 != X86_DR7_INIT_VAL)
        ASMSetDR7(X86_DR7_INIT_VAL);

    return VINF_SUCCESS;
}


/**
 * Saves the guest DRx state residing in host registers and restore the host
 * register values.
 *
 * The guest DRx state is only saved if CPUMR0LoadGuestDebugState was called,
 * since it's assumed that we're shadowing the guest DRx register values
 * accurately when using the combined hypervisor debug register values
 * (CPUMR0LoadHyperDebugState).
 *
 * @returns true if either guest or hypervisor debug registers were loaded.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   fDr6        Whether to include DR6 or not.
 * @thread  EMT(pVCpu)
 */
VMMR0_INT_DECL(bool) CPUMR0DebugStateMaybeSaveGuestAndRestoreHost(PVMCPU pVCpu, bool fDr6)
{
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    bool const fDrXLoaded = RT_BOOL(pVCpu->cpum.s.fUseFlags & (CPUM_USED_DEBUG_REGS_GUEST | CPUM_USED_DEBUG_REGS_HYPER));

    /*
     * Do we need to save the guest DRx registered loaded into host registers?
     * (DR7 and DR6 (if fDr6 is true) are left to the caller.)
     */
    if (pVCpu->cpum.s.fUseFlags & CPUM_USED_DEBUG_REGS_GUEST)
    {
#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
        if (CPUMIsGuestInLongModeEx(&pVCpu->cpum.s.Guest))
        {
            uint64_t uDr6 = pVCpu->cpum.s.Guest.dr[6];
            HMR0SaveDebugState(pVCpu->CTX_SUFF(pVM), pVCpu, &pVCpu->cpum.s.Guest);
            if (!fDr6)
                pVCpu->cpum.s.Guest.dr[6] = uDr6;
        }
        else
#endif
        {
            pVCpu->cpum.s.Guest.dr[0] = ASMGetDR0();
            pVCpu->cpum.s.Guest.dr[1] = ASMGetDR1();
            pVCpu->cpum.s.Guest.dr[2] = ASMGetDR2();
            pVCpu->cpum.s.Guest.dr[3] = ASMGetDR3();
            if (fDr6)
                pVCpu->cpum.s.Guest.dr[6] = ASMGetDR6();
        }
    }
    ASMAtomicAndU32(&pVCpu->cpum.s.fUseFlags, ~(  CPUM_USED_DEBUG_REGS_GUEST | CPUM_USED_DEBUG_REGS_HYPER
                                                | CPUM_SYNC_DEBUG_REGS_GUEST | CPUM_SYNC_DEBUG_REGS_HYPER));

    /*
     * Restore the host's debug state. DR0-3, DR6 and only then DR7!
     */
    if (pVCpu->cpum.s.fUseFlags & CPUM_USED_DEBUG_REGS_HOST)
    {
        /* A bit of paranoia first... */
        uint64_t uCurDR7 = ASMGetDR7();
        if (uCurDR7 != X86_DR7_INIT_VAL)
            ASMSetDR7(X86_DR7_INIT_VAL);

        ASMSetDR0(pVCpu->cpum.s.Host.dr0);
        ASMSetDR1(pVCpu->cpum.s.Host.dr1);
        ASMSetDR2(pVCpu->cpum.s.Host.dr2);
        ASMSetDR3(pVCpu->cpum.s.Host.dr3);
        /** @todo consider only updating if they differ, esp. DR6. Need to figure how
         *        expensive DRx reads are over DRx writes.  */
        ASMSetDR6(pVCpu->cpum.s.Host.dr6);
        ASMSetDR7(pVCpu->cpum.s.Host.dr7);

        ASMAtomicAndU32(&pVCpu->cpum.s.fUseFlags, ~CPUM_USED_DEBUG_REGS_HOST);
    }

    return fDrXLoaded;
}


/**
 * Saves the guest DRx state if it resides host registers.
 *
 * This does NOT clear any use flags, so the host registers remains loaded with
 * the guest DRx state upon return.  The purpose is only to make sure the values
 * in the CPU context structure is up to date.
 *
 * @returns true if the host registers contains guest values, false if not.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   fDr6        Whether to include DR6 or not.
 * @thread  EMT(pVCpu)
 */
VMMR0_INT_DECL(bool) CPUMR0DebugStateMaybeSaveGuest(PVMCPU pVCpu, bool fDr6)
{
    /*
     * Do we need to save the guest DRx registered loaded into host registers?
     * (DR7 and DR6 (if fDr6 is true) are left to the caller.)
     */
    if (pVCpu->cpum.s.fUseFlags & CPUM_USED_DEBUG_REGS_GUEST)
    {
#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
        if (CPUMIsGuestInLongModeEx(&pVCpu->cpum.s.Guest))
        {
            uint64_t uDr6 = pVCpu->cpum.s.Guest.dr[6];
            HMR0SaveDebugState(pVCpu->CTX_SUFF(pVM), pVCpu, &pVCpu->cpum.s.Guest);
            if (!fDr6)
                pVCpu->cpum.s.Guest.dr[6] = uDr6;
        }
        else
#endif
        {
            pVCpu->cpum.s.Guest.dr[0] = ASMGetDR0();
            pVCpu->cpum.s.Guest.dr[1] = ASMGetDR1();
            pVCpu->cpum.s.Guest.dr[2] = ASMGetDR2();
            pVCpu->cpum.s.Guest.dr[3] = ASMGetDR3();
            if (fDr6)
                pVCpu->cpum.s.Guest.dr[6] = ASMGetDR6();
        }
        return true;
    }
    return false;
}


/**
 * Lazily sync in the debug state.
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   fDr6        Whether to include DR6 or not.
 * @thread  EMT(pVCpu)
 */
VMMR0_INT_DECL(void) CPUMR0LoadGuestDebugState(PVMCPU pVCpu, bool fDr6)
{
    /*
     * Save the host state and disarm all host BPs.
     */
    cpumR0SaveHostDebugState(pVCpu);
    Assert(ASMGetDR7() == X86_DR7_INIT_VAL);

    /*
     * Activate the guest state DR0-3.
     * DR7 and DR6 (if fDr6 is true) are left to the caller.
     */
#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
    if (CPUMIsGuestInLongModeEx(&pVCpu->cpum.s.Guest))
        ASMAtomicOrU32(&pVCpu->cpum.s.fUseFlags, CPUM_SYNC_DEBUG_REGS_GUEST); /* Postpone it to the world switch. */
    else
#endif
    {
        ASMSetDR0(pVCpu->cpum.s.Guest.dr[0]);
        ASMSetDR1(pVCpu->cpum.s.Guest.dr[1]);
        ASMSetDR2(pVCpu->cpum.s.Guest.dr[2]);
        ASMSetDR3(pVCpu->cpum.s.Guest.dr[3]);
        if (fDr6)
            ASMSetDR6(pVCpu->cpum.s.Guest.dr[6]);

        ASMAtomicOrU32(&pVCpu->cpum.s.fUseFlags, CPUM_USED_DEBUG_REGS_GUEST);
    }
}


/**
 * Lazily sync in the hypervisor debug state
 *
 * @returns VBox status code.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   fDr6        Whether to include DR6 or not.
 * @thread  EMT(pVCpu)
 */
VMMR0_INT_DECL(void) CPUMR0LoadHyperDebugState(PVMCPU pVCpu, bool fDr6)
{
    /*
     * Save the host state and disarm all host BPs.
     */
    cpumR0SaveHostDebugState(pVCpu);
    Assert(ASMGetDR7() == X86_DR7_INIT_VAL);

    /*
     * Make sure the hypervisor values are up to date.
     */
    CPUMRecalcHyperDRx(pVCpu, UINT8_MAX /* no loading, please */, true);

    /*
     * Activate the guest state DR0-3.
     * DR7 and DR6 (if fDr6 is true) are left to the caller.
     */
#if HC_ARCH_BITS == 32 && defined(VBOX_WITH_64_BITS_GUESTS)
    if (CPUMIsGuestInLongModeEx(&pVCpu->cpum.s.Guest))
        ASMAtomicOrU32(&pVCpu->cpum.s.fUseFlags, CPUM_SYNC_DEBUG_REGS_HYPER); /* Postpone it. */
    else
#endif
    {
        ASMSetDR0(pVCpu->cpum.s.Hyper.dr[0]);
        ASMSetDR1(pVCpu->cpum.s.Hyper.dr[1]);
        ASMSetDR2(pVCpu->cpum.s.Hyper.dr[2]);
        ASMSetDR3(pVCpu->cpum.s.Hyper.dr[3]);
        if (fDr6)
            ASMSetDR6(X86_DR6_INIT_VAL);

        ASMAtomicOrU32(&pVCpu->cpum.s.fUseFlags, CPUM_USED_DEBUG_REGS_HYPER);
    }
}

#ifdef VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI

/**
 * Per-CPU callback that probes the CPU for APIC support.
 *
 * @param   idCpu       The identifier for the CPU the function is called on.
 * @param   pvUser1     Ignored.
 * @param   pvUser2     Ignored.
 */
static DECLCALLBACK(void) cpumR0MapLocalApicCpuProber(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    NOREF(pvUser1); NOREF(pvUser2);
    int iCpu = RTMpCpuIdToSetIndex(idCpu);
    AssertReturnVoid(iCpu >= 0 && (unsigned)iCpu < RT_ELEMENTS(g_aLApics));

    /*
     * Check for APIC support.
     */
    uint32_t uMaxLeaf, u32EBX, u32ECX, u32EDX;
    ASMCpuId(0, &uMaxLeaf, &u32EBX, &u32ECX, &u32EDX);
    if (   (   ASMIsIntelCpuEx(u32EBX, u32ECX, u32EDX)
            || ASMIsAmdCpuEx(u32EBX, u32ECX, u32EDX)
            || ASMIsViaCentaurCpuEx(u32EBX, u32ECX, u32EDX))
        && ASMIsValidStdRange(uMaxLeaf))
    {
        uint32_t uDummy;
        ASMCpuId(1, &uDummy, &u32EBX, &u32ECX, &u32EDX);
        if (    (u32EDX & X86_CPUID_FEATURE_EDX_APIC)
            &&  (u32EDX & X86_CPUID_FEATURE_EDX_MSR))
        {
            /*
             * Safe to access the MSR. Read it and calc the BASE (a little complicated).
             */
            uint64_t u64ApicBase = ASMRdMsr(MSR_IA32_APICBASE);
            uint64_t u64Mask     = MSR_IA32_APICBASE_BASE_MIN;

            /* see Intel Manual: Local APIC Status and Location: MAXPHYADDR default is bit 36 */
            uint32_t uMaxExtLeaf;
            ASMCpuId(0x80000000, &uMaxExtLeaf, &u32EBX, &u32ECX, &u32EDX);
            if (   uMaxExtLeaf >= UINT32_C(0x80000008)
                && ASMIsValidExtRange(uMaxExtLeaf))
            {
                uint32_t u32PhysBits;
                ASMCpuId(0x80000008, &u32PhysBits, &u32EBX, &u32ECX, &u32EDX);
                u32PhysBits &= 0xff;
                u64Mask = ((UINT64_C(1) << u32PhysBits) - 1) & UINT64_C(0xfffffffffffff000);
            }

            AssertCompile(sizeof(g_aLApics[iCpu].PhysBase) == sizeof(u64ApicBase));
            g_aLApics[iCpu].PhysBase    = u64ApicBase & u64Mask;
            g_aLApics[iCpu].fEnabled    = RT_BOOL(u64ApicBase & MSR_IA32_APICBASE_EN);
            g_aLApics[iCpu].fX2Apic     =    (u64ApicBase & (MSR_IA32_APICBASE_EXTD | MSR_IA32_APICBASE_EN))
                                          == (MSR_IA32_APICBASE_EXTD | MSR_IA32_APICBASE_EN);
        }
    }
}



/**
 * Per-CPU callback that verifies our APIC expectations.
 *
 * @param   idCpu       The identifier for the CPU the function is called on.
 * @param   pvUser1     Ignored.
 * @param   pvUser2     Ignored.
 */
static DECLCALLBACK(void) cpumR0MapLocalApicCpuChecker(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    NOREF(pvUser1); NOREF(pvUser2);

    int iCpu = RTMpCpuIdToSetIndex(idCpu);
    AssertReturnVoid(iCpu >= 0 && (unsigned)iCpu < RT_ELEMENTS(g_aLApics));
    if (!g_aLApics[iCpu].fEnabled)
        return;

    /*
     * 0x0X       82489 external APIC
     * 0x1X       Local APIC
     * 0x2X..0xFF reserved
     */
    uint32_t uApicVersion;
    if (g_aLApics[iCpu].fX2Apic)
        uApicVersion = ApicX2RegRead32(APIC_REG_VERSION);
    else
        uApicVersion = ApicRegRead(g_aLApics[iCpu].pv, APIC_REG_VERSION);
    if ((APIC_REG_VERSION_GET_VER(uApicVersion) & 0xF0) == 0x10)
    {
        g_aLApics[iCpu].uVersion    = uApicVersion;

#if 0 /* enable if you need it. */
        if (g_aLApics[iCpu].fX2Apic)
            SUPR0Printf("CPUM: X2APIC %02u - ver %#010x, lint0=%#07x lint1=%#07x pc=%#07x thmr=%#07x cmci=%#07x\n",
                        iCpu, uApicVersion,
                        ApicX2RegRead32(APIC_REG_LVT_LINT0), ApicX2RegRead32(APIC_REG_LVT_LINT1),
                        ApicX2RegRead32(APIC_REG_LVT_PC), ApicX2RegRead32(APIC_REG_LVT_THMR),
                        ApicX2RegRead32(APIC_REG_LVT_CMCI));
        else
        {
            SUPR0Printf("CPUM: APIC %02u at %RGp (mapped at %p) - ver %#010x, lint0=%#07x lint1=%#07x pc=%#07x thmr=%#07x cmci=%#07x\n",
                        iCpu, g_aLApics[iCpu].PhysBase, g_aLApics[iCpu].pv, uApicVersion,
                        ApicRegRead(g_aLApics[iCpu].pv, APIC_REG_LVT_LINT0), ApicRegRead(g_aLApics[iCpu].pv, APIC_REG_LVT_LINT1),
                        ApicRegRead(g_aLApics[iCpu].pv, APIC_REG_LVT_PC), ApicRegRead(g_aLApics[iCpu].pv, APIC_REG_LVT_THMR),
                        ApicRegRead(g_aLApics[iCpu].pv, APIC_REG_LVT_CMCI));
            if (uApicVersion & 0x80000000)
            {
                uint32_t uExtFeatures = ApicRegRead(g_aLApics[iCpu].pv, 0x400);
                uint32_t cEiLvt = (uExtFeatures >> 16) & 0xff;
                SUPR0Printf("CPUM: APIC %02u: ExtSpace available. extfeat=%08x eilvt[0..3]=%08x %08x %08x %08x\n",
                            iCpu,
                            ApicRegRead(g_aLApics[iCpu].pv, 0x400),
                            cEiLvt >= 1 ? ApicRegRead(g_aLApics[iCpu].pv, 0x500) : 0,
                            cEiLvt >= 2 ? ApicRegRead(g_aLApics[iCpu].pv, 0x510) : 0,
                            cEiLvt >= 3 ? ApicRegRead(g_aLApics[iCpu].pv, 0x520) : 0,
                            cEiLvt >= 4 ? ApicRegRead(g_aLApics[iCpu].pv, 0x530) : 0);
            }
        }
#endif
    }
    else
    {
        g_aLApics[iCpu].fEnabled = false;
        g_aLApics[iCpu].fX2Apic  = false;
        SUPR0Printf("VBox/CPUM: Unsupported APIC version %#x (iCpu=%d)\n", uApicVersion, iCpu);
    }
}


/**
 * Map the MMIO page of each local APIC in the system.
 */
static int cpumR0MapLocalApics(void)
{
    /*
     * Check that we'll always stay within the array bounds.
     */
    if (RTMpGetArraySize() > RT_ELEMENTS(g_aLApics))
    {
        LogRel(("CPUM: Too many real CPUs/cores/threads - %u, max %u\n", RTMpGetArraySize(), RT_ELEMENTS(g_aLApics)));
        return VERR_TOO_MANY_CPUS;
    }

    /*
     * Create mappings for all online CPUs we think have legacy APICs.
     */
    int rc = RTMpOnAll(cpumR0MapLocalApicCpuProber, NULL, NULL);

    for (unsigned iCpu = 0; RT_SUCCESS(rc) && iCpu < RT_ELEMENTS(g_aLApics); iCpu++)
    {
        if (g_aLApics[iCpu].fEnabled && !g_aLApics[iCpu].fX2Apic)
        {
            rc = RTR0MemObjEnterPhys(&g_aLApics[iCpu].hMemObj, g_aLApics[iCpu].PhysBase,
                                     PAGE_SIZE, RTMEM_CACHE_POLICY_MMIO);
            if (RT_SUCCESS(rc))
            {
                rc = RTR0MemObjMapKernel(&g_aLApics[iCpu].hMapObj, g_aLApics[iCpu].hMemObj, (void *)-1,
                                         PAGE_SIZE, RTMEM_PROT_READ | RTMEM_PROT_WRITE);
                if (RT_SUCCESS(rc))
                {
                    g_aLApics[iCpu].pv = RTR0MemObjAddress(g_aLApics[iCpu].hMapObj);
                    continue;
                }
                RTR0MemObjFree(g_aLApics[iCpu].hMemObj, true /* fFreeMappings */);
            }
            g_aLApics[iCpu].fEnabled = false;
        }
        g_aLApics[iCpu].pv = NULL;
    }

    /*
     * Check the APICs.
     */
    if (RT_SUCCESS(rc))
        rc = RTMpOnAll(cpumR0MapLocalApicCpuChecker, NULL, NULL);

    if (RT_FAILURE(rc))
    {
        cpumR0UnmapLocalApics();
        return rc;
    }

#ifdef LOG_ENABLED
    /*
     * Log the result (pretty useless, requires enabling CPUM in VBoxDrv
     * and !VBOX_WITH_R0_LOGGING).
     */
    if (LogIsEnabled())
    {
        uint32_t cEnabled = 0;
        uint32_t cX2Apics = 0;
        for (unsigned iCpu = 0; iCpu < RT_ELEMENTS(g_aLApics); iCpu++)
            if (g_aLApics[iCpu].fEnabled)
            {
                cEnabled++;
                cX2Apics += g_aLApics[iCpu].fX2Apic;
            }
        Log(("CPUM: %u APICs, %u X2APICs\n", cEnabled, cX2Apics));
    }
#endif

    return VINF_SUCCESS;
}


/**
 * Unmap the Local APIC of all host CPUs.
 */
static void cpumR0UnmapLocalApics(void)
{
    for (unsigned iCpu = RT_ELEMENTS(g_aLApics); iCpu-- > 0;)
    {
        if (g_aLApics[iCpu].pv)
        {
            RTR0MemObjFree(g_aLApics[iCpu].hMapObj, true /* fFreeMappings */);
            RTR0MemObjFree(g_aLApics[iCpu].hMemObj, true /* fFreeMappings */);
            g_aLApics[iCpu].hMapObj  = NIL_RTR0MEMOBJ;
            g_aLApics[iCpu].hMemObj  = NIL_RTR0MEMOBJ;
            g_aLApics[iCpu].fEnabled = false;
            g_aLApics[iCpu].fX2Apic  = false;
            g_aLApics[iCpu].pv       = NULL;
        }
    }
}


/**
 * Updates CPUMCPU::pvApicBase and CPUMCPU::fX2Apic prior to world switch.
 *
 * Writes the Local APIC mapping address of the current host CPU to CPUMCPU so
 * the world switchers can access the APIC registers for the purpose of
 * disabling and re-enabling the NMIs.  Must be called with disabled preemption
 * or disabled interrupts!
 *
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   iHostCpuSet The CPU set index of the current host CPU.
 */
VMMR0_INT_DECL(void) CPUMR0SetLApic(PVMCPU pVCpu, uint32_t iHostCpuSet)
{
    Assert(iHostCpuSet <= RT_ELEMENTS(g_aLApics));
    pVCpu->cpum.s.pvApicBase = g_aLApics[iHostCpuSet].pv;
    pVCpu->cpum.s.fX2Apic    = g_aLApics[iHostCpuSet].fX2Apic;
//    Log6(("CPUMR0SetLApic: pvApicBase=%p fX2Apic=%d\n", g_aLApics[idxCpu].pv, g_aLApics[idxCpu].fX2Apic));
}

#endif /* VBOX_WITH_VMMR0_DISABLE_LAPIC_NMI */

