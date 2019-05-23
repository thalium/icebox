/* $Id: IEMR3.cpp $ */
/** @file
 * IEM - Interpreted Execution Manager.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_EM
#include <VBox/vmm/iem.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/mm.h>
#include "IEMInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/err.h>

#include <iprt/asm-amd64-x86.h>
#include <iprt/assert.h>

static const char *iemGetTargetCpuName(uint32_t enmTargetCpu)
{
    switch (enmTargetCpu)
    {
#define CASE_RET_STR(enmValue) case enmValue: return #enmValue + (sizeof("IEMTARGETCPU_") - 1)
        CASE_RET_STR(IEMTARGETCPU_8086);
        CASE_RET_STR(IEMTARGETCPU_V20);
        CASE_RET_STR(IEMTARGETCPU_186);
        CASE_RET_STR(IEMTARGETCPU_286);
        CASE_RET_STR(IEMTARGETCPU_386);
        CASE_RET_STR(IEMTARGETCPU_486);
        CASE_RET_STR(IEMTARGETCPU_PENTIUM);
        CASE_RET_STR(IEMTARGETCPU_PPRO);
        CASE_RET_STR(IEMTARGETCPU_CURRENT);
#undef CASE_RET_STR
        default: return "Unknown";
    }
}

/**
 * Initializes the interpreted execution manager.
 *
 * This must be called after CPUM as we're quering information from CPUM about
 * the guest and host CPUs.
 *
 * @returns VBox status code.
 * @param   pVM                The cross context VM structure.
 */
VMMR3DECL(int)      IEMR3Init(PVM pVM)
{
    uint64_t const uInitialTlbRevision = UINT64_C(0) - (IEMTLB_REVISION_INCR * 200U);
    uint64_t const uInitialTlbPhysRev  = UINT64_C(0) - (IEMTLB_PHYS_REV_INCR * 100U);

    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = &pVM->aCpus[idCpu];
        pVCpu->iem.s.pCtxR3 = CPUMQueryGuestCtxPtr(pVCpu);
        pVCpu->iem.s.pCtxR0 = VM_R0_ADDR(pVM, pVCpu->iem.s.pCtxR3);
        pVCpu->iem.s.pCtxRC = VM_RC_ADDR(pVM, pVCpu->iem.s.pCtxR3);

        pVCpu->iem.s.CodeTlb.uTlbRevision = pVCpu->iem.s.DataTlb.uTlbRevision = uInitialTlbRevision;
        pVCpu->iem.s.CodeTlb.uTlbPhysRev  = pVCpu->iem.s.DataTlb.uTlbPhysRev  = uInitialTlbPhysRev;

        STAMR3RegisterF(pVM, &pVCpu->iem.s.cInstructions,               STAMTYPE_U32,       STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Instructions interpreted",                     "/IEM/CPU%u/cInstructions", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cLongJumps,                  STAMTYPE_U32,       STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,
                        "Number of longjmp calls",                      "/IEM/CPU%u/cLongJumps", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cPotentialExits,             STAMTYPE_U32,       STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Potential exits",                              "/IEM/CPU%u/cPotentialExits", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cRetAspectNotImplemented,    STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "VERR_IEM_ASPECT_NOT_IMPLEMENTED",              "/IEM/CPU%u/cRetAspectNotImplemented", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cRetInstrNotImplemented,     STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "VERR_IEM_INSTR_NOT_IMPLEMENTED",               "/IEM/CPU%u/cRetInstrNotImplemented", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cRetInfStatuses,             STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Informational statuses returned",              "/IEM/CPU%u/cRetInfStatuses", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cRetErrStatuses,             STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Error statuses returned",                      "/IEM/CPU%u/cRetErrStatuses", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cbWritten,                   STAMTYPE_U32,       STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,
                        "Approx bytes written",                         "/IEM/CPU%u/cbWritten", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.cPendingCommit,              STAMTYPE_U32,       STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,
                        "Times RC/R0 had to postpone instruction committing to ring-3", "/IEM/CPU%u/cPendingCommit", idCpu);

#ifdef VBOX_WITH_STATISTICS
        STAMR3RegisterF(pVM, &pVCpu->iem.s.CodeTlb.cTlbHits,            STAMTYPE_U64_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Code TLB hits",                            "/IEM/CPU%u/CodeTlb-Hits", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.DataTlb.cTlbHits,            STAMTYPE_U64_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Data TLB hits",                            "/IEM/CPU%u/DataTlb-Hits", idCpu);
#endif
        STAMR3RegisterF(pVM, &pVCpu->iem.s.CodeTlb.cTlbMisses,          STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Code TLB misses",                          "/IEM/CPU%u/CodeTlb-Misses", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.CodeTlb.uTlbRevision,        STAMTYPE_X64,       STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                        "Code TLB revision",                        "/IEM/CPU%u/CodeTlb-Revision", idCpu);
        STAMR3RegisterF(pVM, (void *)&pVCpu->iem.s.CodeTlb.uTlbPhysRev, STAMTYPE_X64,       STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                        "Code TLB physical revision",               "/IEM/CPU%u/CodeTlb-PhysRev", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.CodeTlb.cTlbSlowReadPath,    STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                        "Code TLB slow read path",                  "/IEM/CPU%u/CodeTlb-SlowReads", idCpu);

        STAMR3RegisterF(pVM, &pVCpu->iem.s.DataTlb.cTlbMisses,          STAMTYPE_U32_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                        "Data TLB misses",                          "/IEM/CPU%u/DataTlb-Misses", idCpu);
        STAMR3RegisterF(pVM, &pVCpu->iem.s.DataTlb.uTlbRevision,        STAMTYPE_X64,       STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                        "Data TLB revision",                        "/IEM/CPU%u/DataTlb-Revision", idCpu);
        STAMR3RegisterF(pVM, (void *)&pVCpu->iem.s.DataTlb.uTlbPhysRev, STAMTYPE_X64,       STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                        "Data TLB physical revision",               "/IEM/CPU%u/DataTlb-PhysRev", idCpu);

#if defined(VBOX_WITH_STATISTICS) && !defined(DOXYGEN_RUNNING)
        /* Allocate instruction statistics and register them. */
        pVCpu->iem.s.pStatsR3 = (PIEMINSTRSTATS)MMR3HeapAllocZ(pVM, MM_TAG_IEM, sizeof(IEMINSTRSTATS));
        AssertLogRelReturn(pVCpu->iem.s.pStatsR3, VERR_NO_MEMORY);
        int rc = MMHyperAlloc(pVM, sizeof(IEMINSTRSTATS), sizeof(uint64_t), MM_TAG_IEM, (void **)&pVCpu->iem.s.pStatsCCR3);
        AssertLogRelRCReturn(rc, rc);
        pVCpu->iem.s.pStatsR0 = MMHyperR3ToR0(pVM, pVCpu->iem.s.pStatsCCR3);
        pVCpu->iem.s.pStatsRC = MMHyperR3ToR0(pVM, pVCpu->iem.s.pStatsCCR3);
# define IEM_DO_INSTR_STAT(a_Name, a_szDesc) \
            STAMR3RegisterF(pVM, &pVCpu->iem.s.pStatsCCR3->a_Name, STAMTYPE_U32_RESET, STAMVISIBILITY_USED, \
                            STAMUNIT_COUNT, a_szDesc, "/IEM/CPU%u/instr-RZ/" #a_Name, idCpu); \
            STAMR3RegisterF(pVM, &pVCpu->iem.s.pStatsR3->a_Name, STAMTYPE_U32_RESET, STAMVISIBILITY_USED, \
                            STAMUNIT_COUNT, a_szDesc, "/IEM/CPU%u/instr-R3/" #a_Name, idCpu);
# include "IEMInstructionStatisticsTmpl.h"
# undef IEM_DO_INSTR_STAT
#endif

        /*
         * Host and guest CPU information.
         */
        if (idCpu == 0)
        {
            pVCpu->iem.s.enmCpuVendor             = CPUMGetGuestCpuVendor(pVM);
            pVCpu->iem.s.enmHostCpuVendor         = CPUMGetHostCpuVendor(pVM);
#if IEM_CFG_TARGET_CPU == IEMTARGETCPU_DYNAMIC
            switch (pVM->cpum.ro.GuestFeatures.enmMicroarch)
            {
                case kCpumMicroarch_Intel_8086:     pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_8086; break;
                case kCpumMicroarch_Intel_80186:    pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_186; break;
                case kCpumMicroarch_Intel_80286:    pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_286; break;
                case kCpumMicroarch_Intel_80386:    pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_386; break;
                case kCpumMicroarch_Intel_80486:    pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_486; break;
                case kCpumMicroarch_Intel_P5:       pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_PENTIUM; break;
                case kCpumMicroarch_Intel_P6:       pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_PPRO; break;
                case kCpumMicroarch_NEC_V20:        pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_V20; break;
                case kCpumMicroarch_NEC_V30:        pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_V20; break;
                default:                            pVCpu->iem.s.uTargetCpu = IEMTARGETCPU_CURRENT; break;
            }
            LogRel(("IEM: TargetCpu=%s, Microarch=%s\n", iemGetTargetCpuName(pVCpu->iem.s.uTargetCpu), CPUMR3MicroarchName(pVM->cpum.ro.GuestFeatures.enmMicroarch)));
#endif
        }
        else
        {
            pVCpu->iem.s.enmCpuVendor             = pVM->aCpus[0].iem.s.enmCpuVendor;
            pVCpu->iem.s.enmHostCpuVendor         = pVM->aCpus[0].iem.s.enmHostCpuVendor;
#if IEM_CFG_TARGET_CPU == IEMTARGETCPU_DYNAMIC
            pVCpu->iem.s.uTargetCpu               = pVM->aCpus[0].iem.s.uTargetCpu;
#endif
        }

        /*
         * Mark all buffers free.
         */
        uint32_t iMemMap = RT_ELEMENTS(pVCpu->iem.s.aMemMappings);
        while (iMemMap-- > 0)
            pVCpu->iem.s.aMemMappings[iMemMap].fAccess = IEM_ACCESS_INVALID;
    }
    return VINF_SUCCESS;
}


VMMR3DECL(int)      IEMR3Term(PVM pVM)
{
    NOREF(pVM);
#if defined(VBOX_WITH_STATISTICS) && !defined(DOXYGEN_RUNNING)
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        PVMCPU pVCpu = &pVM->aCpus[idCpu];
        MMR3HeapFree(pVCpu->iem.s.pStatsR3);
        pVCpu->iem.s.pStatsR3 = NULL;
    }
#endif
    return VINF_SUCCESS;
}


VMMR3DECL(void)     IEMR3Relocate(PVM pVM)
{
    for (VMCPUID idCpu = 0; idCpu < pVM->cCpus; idCpu++)
    {
        pVM->aCpus[idCpu].iem.s.pCtxRC = VM_RC_ADDR(pVM, pVM->aCpus[idCpu].iem.s.pCtxR3);
        if (pVM->aCpus[idCpu].iem.s.pStatsRC)
            pVM->aCpus[idCpu].iem.s.pStatsRC = MMHyperR3ToRC(pVM, pVM->aCpus[idCpu].iem.s.pStatsCCR3);
    }
}

