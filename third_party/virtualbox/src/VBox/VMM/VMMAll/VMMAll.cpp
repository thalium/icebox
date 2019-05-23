/* $Id: VMMAll.cpp $ */
/** @file
 * VMM All Contexts.
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
#include "VMMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/vmcpuset.h>
#include <VBox/param.h>
#include <iprt/thread.h>
#include <iprt/mp.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** User counter for the vmmInitFormatTypes function (pro forma). */
static volatile uint32_t g_cFormatTypeUsers = 0;


/**
 * Helper that formats a decimal number in the range 0..9999.
 *
 * @returns The length of the formatted number.
 * @param   pszBuf              Output buffer with sufficient space.
 * @param   uNumber             The number to format.
 */
static unsigned vmmFormatTypeShortNumber(char *pszBuf, uint32_t uNumber)
{
    unsigned  off = 0;
    if (uNumber >= 10)
    {
        if (uNumber >= 100)
        {
            if (uNumber >= 1000)
                pszBuf[off++] = ((uNumber / 1000) % 10) + '0';
            pszBuf[off++] = ((uNumber / 100) % 10) + '0';
        }
        pszBuf[off++] = ((uNumber / 10) % 10) + '0';
    }
    pszBuf[off++] = (uNumber % 10) + '0';
    pszBuf[off] = '\0';
    return off;
}


/**
 * @callback_method_impl{FNRTSTRFORMATTYPE, vmsetcpu}
 */
static DECLCALLBACK(size_t) vmmFormatTypeVmCpuSet(PFNRTSTROUTPUT pfnOutput, void *pvArgOutput,
                                                  const char *pszType, void const *pvValue,
                                                  int cchWidth, int cchPrecision, unsigned fFlags,
                                                  void *pvUser)
{
    NOREF(pszType); NOREF(cchWidth); NOREF(cchPrecision); NOREF(fFlags);

    PCVMCPUSET  pSet   = (PCVMCPUSET)pvValue;
    uint32_t    cCpus  = 0;
    uint32_t    iCpu   = RT_ELEMENTS(pSet->au32Bitmap) * 32;
    while (iCpu--)
        if (VMCPUSET_IS_PRESENT(pSet, iCpu))
            cCpus++;

    char szTmp[32];
    AssertCompile(RT_ELEMENTS(pSet->au32Bitmap) * 32 < 999);
    if (cCpus == 1)
    {
        iCpu = RT_ELEMENTS(pSet->au32Bitmap) * 32;
        while (iCpu--)
            if (VMCPUSET_IS_PRESENT(pSet, iCpu))
            {
                szTmp[0] = 'c';
                szTmp[1] = 'p';
                szTmp[2] = 'u';
                return pfnOutput(pvArgOutput, szTmp, 3 + vmmFormatTypeShortNumber(&szTmp[3], iCpu));
            }
        cCpus = 0;
    }
    if (cCpus == 0)
        return pfnOutput(pvArgOutput, RT_STR_TUPLE("<empty>"));
    if (cCpus == RT_ELEMENTS(pSet->au32Bitmap) * 32)
        return pfnOutput(pvArgOutput, RT_STR_TUPLE("<full>"));

    /*
     * Print cpus that are present: {1,2,7,9 ... }
     */
    size_t cchRet = pfnOutput(pvArgOutput, "{", 1);

    cCpus = 0;
    iCpu  = 0;
    while (iCpu < RT_ELEMENTS(pSet->au32Bitmap) * 32)
    {
        if (VMCPUSET_IS_PRESENT(pSet, iCpu))
        {
            /* Output the first cpu number. */
            int off = 0;
            if (cCpus != 0)
                szTmp[off++] = ',';
            cCpus++;
            off += vmmFormatTypeShortNumber(&szTmp[off], iCpu);

            /* Check for sequence. */
            uint32_t const iStart = ++iCpu;
            while (   iCpu < RT_ELEMENTS(pSet->au32Bitmap) * 32
                   && VMCPUSET_IS_PRESENT(pSet, iCpu))
            {
                iCpu++;
                cCpus++;
            }
            if (iCpu != iStart)
            {
                szTmp[off++] = '-';
                off += vmmFormatTypeShortNumber(&szTmp[off], iCpu);
            }

            /* Terminate and output. */
            szTmp[off] = '\0';
            cchRet += pfnOutput(pvArgOutput, szTmp, off);
        }
        iCpu++;
    }

    cchRet += pfnOutput(pvArgOutput, "}", 1);
    NOREF(pvUser);
    return cchRet;
}


/**
 * Registers the VMM wide format types.
 *
 * Called by VMMR3Init, VMMR0Init and VMMRCInit.
 */
int vmmInitFormatTypes(void)
{
    int rc = VINF_SUCCESS;
    if (ASMAtomicIncU32(&g_cFormatTypeUsers) == 1)
        rc = RTStrFormatTypeRegister("vmcpuset", vmmFormatTypeVmCpuSet, NULL);
    return rc;
}


#ifndef IN_RC
/**
 * Counterpart to vmmInitFormatTypes, called by VMMR3Term and VMMR0Term.
 */
void vmmTermFormatTypes(void)
{
    if (ASMAtomicDecU32(&g_cFormatTypeUsers) == 0)
        RTStrFormatTypeDeregister("vmcpuset");
}
#endif


/**
 * Gets the bottom of the hypervisor stack - RC Ptr.
 *
 * (The returned address is not actually writable, only after it's decremented
 * by a push/ret/whatever does it become writable.)
 *
 * @returns bottom of the stack.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMM_INT_DECL(RTRCPTR) VMMGetStackRC(PVMCPU pVCpu)
{
    return (RTRCPTR)pVCpu->vmm.s.pbEMTStackBottomRC;
}


/**
 * Gets the ID of the virtual CPU associated with the calling thread.
 *
 * @returns The CPU ID. NIL_VMCPUID if the thread isn't an EMT.
 *
 * @param   pVM         The cross context VM structure.
 * @internal
 */
VMMDECL(VMCPUID) VMMGetCpuId(PVM pVM)
{
#if defined(IN_RING3)
    return VMR3GetVMCPUId(pVM);

#elif defined(IN_RING0)
    if (pVM->cCpus == 1)
        return 0;

    /* Search first by host cpu id (most common case)
     * and then by native thread id (page fusion case).
     */
    if (!RTThreadPreemptIsEnabled(NIL_RTTHREAD))
    {
        /** @todo r=ramshankar: This doesn't buy us anything in terms of performance
         *        leaving it here for hysterical raisins and as a reference if we
         *        implemented a hashing approach in the future. */
        RTCPUID idHostCpu = RTMpCpuId();

        /** @todo optimize for large number of VCPUs when that becomes more common. */
        for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
        {
            PVMCPU pVCpu = &pVM->aCpus[idCpu];

            if (pVCpu->idHostCpu == idHostCpu)
                return pVCpu->idCpu;
        }
    }

    /* RTThreadGetNativeSelf had better be cheap. */
    RTNATIVETHREAD hThread = RTThreadNativeSelf();

    /** @todo optimize for large number of VCPUs when that becomes more common. */
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = &pVM->aCpus[idCpu];

        if (pVCpu->hNativeThreadR0 == hThread)
            return pVCpu->idCpu;
    }
    return NIL_VMCPUID;

#else /* RC: Always EMT(0) */
    NOREF(pVM);
    return 0;
#endif
}


/**
 * Returns the VMCPU of the calling EMT.
 *
 * @returns The VMCPU pointer. NULL if not an EMT.
 *
 * @param   pVM         The cross context VM structure.
 * @internal
 */
VMMDECL(PVMCPU) VMMGetCpu(PVM pVM)
{
#ifdef IN_RING3
    VMCPUID idCpu = VMR3GetVMCPUId(pVM);
    if (idCpu == NIL_VMCPUID)
        return NULL;
    Assert(idCpu < pVM->cCpus);
    return &pVM->aCpus[idCpu];

#elif defined(IN_RING0)
    if (pVM->cCpus == 1)
        return &pVM->aCpus[0];

    /*
     * Search first by host cpu id (most common case)
     * and then by native thread id (page fusion case).
     */
    if (!RTThreadPreemptIsEnabled(NIL_RTTHREAD))
    {
        /** @todo r=ramshankar: This doesn't buy us anything in terms of performance
         *        leaving it here for hysterical raisins and as a reference if we
         *        implemented a hashing approach in the future. */
        RTCPUID idHostCpu = RTMpCpuId();

        /** @todo optimize for large number of VCPUs when that becomes more common. */
        for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
        {
            PVMCPU pVCpu = &pVM->aCpus[idCpu];

            if (pVCpu->idHostCpu == idHostCpu)
                return pVCpu;
        }
    }

    /* RTThreadGetNativeSelf had better be cheap. */
    RTNATIVETHREAD hThread = RTThreadNativeSelf();

    /** @todo optimize for large number of VCPUs when that becomes more common.
     * Use a map like GIP does that's indexed by the host CPU index.  */
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = &pVM->aCpus[idCpu];

        if (pVCpu->hNativeThreadR0 == hThread)
            return pVCpu;
    }
    return NULL;

#else /* RC: Always EMT(0) */
    return &pVM->aCpus[0];
#endif /* IN_RING0 */
}


/**
 * Returns the VMCPU of the first EMT thread.
 *
 * @returns The VMCPU pointer.
 * @param   pVM         The cross context VM structure.
 * @internal
 */
VMMDECL(PVMCPU) VMMGetCpu0(PVM pVM)
{
    Assert(pVM->cCpus == 1);
    return &pVM->aCpus[0];
}


/**
 * Returns the VMCPU of the specified virtual CPU.
 *
 * @returns The VMCPU pointer. NULL if idCpu is invalid.
 *
 * @param   pVM         The cross context VM structure.
 * @param   idCpu       The ID of the virtual CPU.
 * @internal
 */
VMMDECL(PVMCPU) VMMGetCpuById(PVM pVM, RTCPUID idCpu)
{
    AssertReturn(idCpu < pVM->cCpus, NULL);
    return &pVM->aCpus[idCpu];
}


/**
 * Gets the VBOX_SVN_REV.
 *
 * This is just to avoid having to compile a bunch of big files
 * and requires less Makefile mess.
 *
 * @returns VBOX_SVN_REV.
 */
VMM_INT_DECL(uint32_t) VMMGetSvnRev(void)
{
    return VBOX_SVN_REV;
}


/**
 * Queries the current switcher
 *
 * @returns active switcher
 * @param   pVM             The cross context VM structure.
 */
VMM_INT_DECL(VMMSWITCHER) VMMGetSwitcher(PVM pVM)
{
    return pVM->vmm.s.enmSwitcher;
}


/**
 * Checks whether we're in a ring-3 call or not.
 *
 * @returns true / false.
 * @param   pVCpu   The cross context virtual CPU structure of the calling EMT.
 * @thread  EMT
 */
VMM_INT_DECL(bool) VMMIsInRing3Call(PVMCPU pVCpu)
{
#ifdef RT_ARCH_X86
    return pVCpu->vmm.s.CallRing3JmpBufR0.fInRing3Call;
#else
    return pVCpu->vmm.s.CallRing3JmpBufR0.fInRing3Call;
#endif
}


/**
 * Returns the build type for matching components.
 *
 * @returns Build type value.
 */
uint32_t vmmGetBuildType(void)
{
    uint32_t uRet = 0xbeef0000;
#ifdef DEBUG
    uRet |= RT_BIT_32(0);
#endif
#ifdef VBOX_WITH_STATISTICS
    uRet |= RT_BIT_32(1);
#endif
    return uRet;
}


/**
 * Patches the instructions necessary for making a hypercall to the hypervisor.
 * Used by GIM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pvBuf       The buffer in the hypercall page(s) to be patched.
 * @param   cbBuf       The size of the buffer.
 * @param   pcbWritten  Where to store the number of bytes patched. This
 *                      is reliably updated only when this function returns
 *                      VINF_SUCCESS.
 */
VMM_INT_DECL(int) VMMPatchHypercall(PVM pVM, void *pvBuf, size_t cbBuf, size_t *pcbWritten)
{
    AssertReturn(pvBuf, VERR_INVALID_POINTER);
    AssertReturn(pcbWritten, VERR_INVALID_POINTER);

    CPUMCPUVENDOR enmHostCpu = CPUMGetHostCpuVendor(pVM);
    switch (enmHostCpu)
    {
        case CPUMCPUVENDOR_AMD:
        {
            uint8_t abHypercall[] = { 0x0F, 0x01, 0xD9 };   /* VMMCALL */
            if (RT_LIKELY(cbBuf >= sizeof(abHypercall)))
            {
                memcpy(pvBuf, abHypercall, sizeof(abHypercall));
                *pcbWritten = sizeof(abHypercall);
                return VINF_SUCCESS;
            }
            return VERR_BUFFER_OVERFLOW;
        }

        case CPUMCPUVENDOR_INTEL:
        case CPUMCPUVENDOR_VIA:
        {
            uint8_t abHypercall[] = { 0x0F, 0x01, 0xC1 };   /* VMCALL */
            if (RT_LIKELY(cbBuf >= sizeof(abHypercall)))
            {
                memcpy(pvBuf, abHypercall, sizeof(abHypercall));
                *pcbWritten = sizeof(abHypercall);
                return VINF_SUCCESS;
            }
            return VERR_BUFFER_OVERFLOW;
        }

        default:
            AssertFailed();
            return VERR_UNSUPPORTED_CPU;
    }
}


/**
 * Notifies VMM that paravirtualized hypercalls are now enabled.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 */
VMM_INT_DECL(void) VMMHypercallsEnable(PVMCPU pVCpu)
{
    /* If there is anything to do for raw-mode, do it here. */
#ifndef IN_RC
    if (HMIsEnabled(pVCpu->CTX_SUFF(pVM)))
        HMHypercallsEnable(pVCpu);
#else
    RT_NOREF_PV(pVCpu);
#endif
}


/**
 * Notifies VMM that paravirtualized hypercalls are now disabled.
 *
 * @param   pVCpu   The cross context virtual CPU structure.
 */
VMM_INT_DECL(void) VMMHypercallsDisable(PVMCPU pVCpu)
{
    /* If there is anything to do for raw-mode, do it here. */
#ifndef IN_RC
    if (HMIsEnabled(pVCpu->CTX_SUFF(pVM)))
        HMHypercallsDisable(pVCpu);
#else
    RT_NOREF_PV(pVCpu);
#endif
}

