/* $Id: initterm-r0drv-nt.cpp $ */
/** @file
 * IPRT - Initialization & Termination, R0 Driver, NT.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "the-nt-kernel.h"
#include <iprt/asm-amd64-x86.h>
#include <iprt/err.h>
#include <iprt/string.h>
#include "internal/initterm.h"
#include "internal-r0drv-nt.h"
#include "symdb.h"
#include "symdbdata.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** ExSetTimerResolution, introduced in W2K. */
PFNMYEXSETTIMERRESOLUTION               g_pfnrtNtExSetTimerResolution;
/** KeFlushQueuedDpcs, introduced in XP. */
PFNMYKEFLUSHQUEUEDDPCS                  g_pfnrtNtKeFlushQueuedDpcs;
/** HalRequestIpi, version introduced with windows 7. */
PFNHALREQUESTIPI_W7PLUS                 g_pfnrtHalRequestIpiW7Plus;
/** HalRequestIpi, version valid up to windows vista?? */
PFNHALREQUESTIPI_PRE_W7                 g_pfnrtHalRequestIpiPreW7;
/** Worker for RTMpPokeCpu. */
PFNRTSENDIPI                            g_pfnrtMpPokeCpuWorker;
/** KeIpiGenericCall - Introduced in Windows Server 2003. */
PFNRTKEIPIGENERICCALL                   g_pfnrtKeIpiGenericCall;
/** KeSetTargetProcessorDpcEx - Introduced in Windows 7. */
PFNKESETTARGETPROCESSORDPCEX            g_pfnrtKeSetTargetProcessorDpcEx;
/** KeInitializeAffinityEx - Introducted in Windows 7. */
PFNKEINITIALIZEAFFINITYEX               g_pfnrtKeInitializeAffinityEx;
/** KeAddProcessorAffinityEx - Introducted in Windows 7. */
PFNKEADDPROCESSORAFFINITYEX             g_pfnrtKeAddProcessorAffinityEx;
/** KeGetProcessorIndexFromNumber - Introducted in Windows 7. */
PFNKEGETPROCESSORINDEXFROMNUMBER        g_pfnrtKeGetProcessorIndexFromNumber;
/** KeGetProcessorNumberFromIndex - Introducted in Windows 7. */
PFNKEGETPROCESSORNUMBERFROMINDEX        g_pfnrtKeGetProcessorNumberFromIndex;
/** KeGetCurrentProcessorNumberEx - Introducted in Windows 7. */
PFNKEGETCURRENTPROCESSORNUMBEREX        g_pfnrtKeGetCurrentProcessorNumberEx;
/** KeQueryActiveProcessors - Introducted in Windows 2000. */
PFNKEQUERYACTIVEPROCESSORS              g_pfnrtKeQueryActiveProcessors;
/** KeQueryMaximumProcessorCount   - Introducted in Vista and obsoleted W7. */
PFNKEQUERYMAXIMUMPROCESSORCOUNT         g_pfnrtKeQueryMaximumProcessorCount;
/** KeQueryMaximumProcessorCountEx - Introducted in Windows 7. */
PFNKEQUERYMAXIMUMPROCESSORCOUNTEX       g_pfnrtKeQueryMaximumProcessorCountEx;
/** KeQueryMaximumGroupCount - Introducted in Windows 7. */
PFNKEQUERYMAXIMUMGROUPCOUNT             g_pfnrtKeQueryMaximumGroupCount;
/** KeQueryActiveProcessorCount   - Introducted in Vista and obsoleted W7. */
PFNKEQUERYACTIVEPROCESSORCOUNT          g_pfnrtKeQueryActiveProcessorCount;
/** KeQueryActiveProcessorCountEx - Introducted in Windows 7. */
PFNKEQUERYACTIVEPROCESSORCOUNTEX        g_pfnrtKeQueryActiveProcessorCountEx;
/** KeQueryLogicalProcessorRelationship - Introducted in Windows 7. */
PFNKEQUERYLOGICALPROCESSORRELATIONSHIP  g_pfnrtKeQueryLogicalProcessorRelationship;
/** KeRegisterProcessorChangeCallback - Introducted in Windows 7. */
PFNKEREGISTERPROCESSORCHANGECALLBACK    g_pfnrtKeRegisterProcessorChangeCallback;
/** KeDeregisterProcessorChangeCallback - Introducted in Windows 7. */
PFNKEDEREGISTERPROCESSORCHANGECALLBACK  g_pfnrtKeDeregisterProcessorChangeCallback;
/** RtlGetVersion, introduced in ??. */
PFNRTRTLGETVERSION                      g_pfnrtRtlGetVersion;
#ifndef RT_ARCH_AMD64
/** KeQueryInterruptTime - exported/new in Windows 2000. */
PFNRTKEQUERYINTERRUPTTIME               g_pfnrtKeQueryInterruptTime;
/** KeQuerySystemTime - exported/new in Windows 2000. */
PFNRTKEQUERYSYSTEMTIME                  g_pfnrtKeQuerySystemTime;
#endif
/** KeQueryInterruptTimePrecise - new in Windows 8. */
PFNRTKEQUERYINTERRUPTTIMEPRECISE        g_pfnrtKeQueryInterruptTimePrecise;
/** KeQuerySystemTimePrecise - new in Windows 8. */
PFNRTKEQUERYSYSTEMTIMEPRECISE           g_pfnrtKeQuerySystemTimePrecise;

/** Offset of the _KPRCB::QuantumEnd field. 0 if not found. */
uint32_t                                g_offrtNtPbQuantumEnd;
/** Size of the _KPRCB::QuantumEnd field. 0 if not found. */
uint32_t                                g_cbrtNtPbQuantumEnd;
/** Offset of the _KPRCB::DpcQueueDepth field. 0 if not found. */
uint32_t                                g_offrtNtPbDpcQueueDepth;

/** The combined NT version, see RTNT_MAKE_VERSION. */
uint32_t                                g_uRtNtVersion;
/** The major version number. */
uint8_t                                 g_uRtNtMajorVer;
/** The minor version number. */
uint8_t                                 g_uRtNtMinorVer;
/** The build number. */
uint32_t                                g_uRtNtBuildNo;


/**
 * Determines the NT kernel verison information.
 *
 * @param   pOsVerInfo          Where to return the version information.
 *
 * @remarks pOsVerInfo->fSmp is only definitive if @c true.
 * @remarks pOsVerInfo->uCsdNo is set to MY_NIL_CSD if it cannot be determined.
 */
static void rtR0NtGetOsVersionInfo(PRTNTSDBOSVER pOsVerInfo)
{
    ULONG       ulMajorVersion = 0;
    ULONG       ulMinorVersion = 0;
    ULONG       ulBuildNumber  = 0;

    pOsVerInfo->fChecked    = PsGetVersion(&ulMajorVersion, &ulMinorVersion, &ulBuildNumber, NULL) == TRUE;
    pOsVerInfo->uMajorVer   = (uint8_t)ulMajorVersion;
    pOsVerInfo->uMinorVer   = (uint8_t)ulMinorVersion;
    pOsVerInfo->uBuildNo    = ulBuildNumber;
#define MY_NIL_CSD      0x3f
    pOsVerInfo->uCsdNo      = MY_NIL_CSD;

    if (g_pfnrtRtlGetVersion)
    {
        RTL_OSVERSIONINFOEXW VerInfo;
        RT_ZERO(VerInfo);
        VerInfo.dwOSVersionInfoSize = sizeof(VerInfo);

        NTSTATUS rcNt = g_pfnrtRtlGetVersion(&VerInfo);
        if (NT_SUCCESS(rcNt))
            pOsVerInfo->uCsdNo = VerInfo.wServicePackMajor;
    }

    /* Note! We cannot quite say if something is MP or UNI. So, fSmp is
             redefined to indicate that it must be MP.
       Note! RTMpGetCount is not available here. */
    pOsVerInfo->fSmp = ulMajorVersion >= 6; /* Vista and later has no UNI kernel AFAIK. */
    if (!pOsVerInfo->fSmp)
    {
        if (   g_pfnrtKeQueryMaximumProcessorCountEx
            && g_pfnrtKeQueryMaximumProcessorCountEx(ALL_PROCESSOR_GROUPS) > 1)
            pOsVerInfo->fSmp = true;
        else if (   g_pfnrtKeQueryMaximumProcessorCount
                 && g_pfnrtKeQueryMaximumProcessorCount() > 1)
            pOsVerInfo->fSmp = true;
        else if (   g_pfnrtKeQueryActiveProcessors
                 && g_pfnrtKeQueryActiveProcessors() > 1)
            pOsVerInfo->fSmp = true;
        else if (KeNumberProcessors > 1)
            pOsVerInfo->fSmp = true;
    }
}


/**
 * Tries a set against the current kernel.
 *
 * @retval  true if it matched up, global variables are updated.
 * @retval  false otherwise (no globals updated).
 * @param   pSet                The data set.
 * @param   pbPrcb              Pointer to the processor control block.
 * @param   pszVendor           Pointer to the processor vendor string.
 * @param   pOsVerInfo          The OS version info.
 */
static bool rtR0NtTryMatchSymSet(PCRTNTSDBSET pSet, uint8_t *pbPrcb, const char *pszVendor, PCRTNTSDBOSVER pOsVerInfo)
{
    /*
     * Don't bother trying stuff where the NT kernel version number differs, or
     * if the build type or SMPness doesn't match up.
     */
    if (   pSet->OsVerInfo.uMajorVer != pOsVerInfo->uMajorVer
        || pSet->OsVerInfo.uMinorVer != pOsVerInfo->uMinorVer
        || pSet->OsVerInfo.fChecked  != pOsVerInfo->fChecked
        || (!pSet->OsVerInfo.fSmp && pOsVerInfo->fSmp /*must-be-smp*/) )
    {
        //DbgPrint("IPRT: #%d Version/type mismatch.\n", pSet - &g_artNtSdbSets[0]);
        return false;
    }

    /*
     * Do the CPU vendor test.
     *
     * Note! The MmIsAddressValid call is the real #PF security here as the
     *       __try/__except has limited/no ability to catch everything we need.
     */
    char *pszPrcbVendorString = (char *)&pbPrcb[pSet->KPRCB.offVendorString];
    if (!MmIsAddressValid(&pszPrcbVendorString[4 * 3 - 1]))
    {
        //DbgPrint("IPRT: #%d invalid vendor string address.\n", pSet - &g_artNtSdbSets[0]);
        return false;
    }
    __try
    {
        if (memcmp(pszPrcbVendorString, pszVendor, RT_MIN(4 * 3, pSet->KPRCB.cbVendorString)) != 0)
        {
            //DbgPrint("IPRT: #%d Vendor string mismatch.\n", pSet - &g_artNtSdbSets[0]);
            return false;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DbgPrint("IPRT: %#d Exception\n", pSet - &g_artNtSdbSets[0]);
        return false;
    }

    /*
     * Got a match, update the global variables and report succcess.
     */
    g_offrtNtPbQuantumEnd    = pSet->KPRCB.offQuantumEnd;
    g_cbrtNtPbQuantumEnd     = pSet->KPRCB.cbQuantumEnd;
    g_offrtNtPbDpcQueueDepth = pSet->KPRCB.offDpcQueueDepth;

#if 0
    DbgPrint("IPRT: Using data set #%u for %u.%usp%u build %u %s %s.\n",
             pSet - &g_artNtSdbSets[0],
             pSet->OsVerInfo.uMajorVer,
             pSet->OsVerInfo.uMinorVer,
             pSet->OsVerInfo.uCsdNo,
             pSet->OsVerInfo.uBuildNo,
             pSet->OsVerInfo.fSmp ? "smp" : "uni",
             pSet->OsVerInfo.fChecked ? "checked" : "free");
#endif
    return true;
}


DECLHIDDEN(int) rtR0InitNative(void)
{
    /*
     * Initialize the function pointers.
     */
#ifdef IPRT_TARGET_NT4
# define GET_SYSTEM_ROUTINE_EX(a_Prf, a_Name, a_pfnType) do { RT_CONCAT3(g_pfnrt, a_Prf, a_Name) = NULL; } while (0)
#else
    UNICODE_STRING RoutineName;
# define GET_SYSTEM_ROUTINE_EX(a_Prf, a_Name, a_pfnType) \
    do { \
        RtlInitUnicodeString(&RoutineName, L#a_Name); \
        RT_CONCAT3(g_pfnrt, a_Prf, a_Name) = (a_pfnType)MmGetSystemRoutineAddress(&RoutineName); \
    } while (0)
#endif
#define GET_SYSTEM_ROUTINE(a_Name)                 GET_SYSTEM_ROUTINE_EX(RT_NOTHING, a_Name, decltype(a_Name) *)
#define GET_SYSTEM_ROUTINE_PRF(a_Prf,a_Name)       GET_SYSTEM_ROUTINE_EX(a_Prf, a_Name, decltype(a_Name) *)
#define GET_SYSTEM_ROUTINE_TYPE(a_Name, a_pfnType) GET_SYSTEM_ROUTINE_EX(RT_NOTHING, a_Name, a_pfnType)

    GET_SYSTEM_ROUTINE_PRF(Nt,ExSetTimerResolution);
    GET_SYSTEM_ROUTINE_PRF(Nt,KeFlushQueuedDpcs);
    GET_SYSTEM_ROUTINE(KeIpiGenericCall);
    GET_SYSTEM_ROUTINE(KeSetTargetProcessorDpcEx);
    GET_SYSTEM_ROUTINE(KeInitializeAffinityEx);
    GET_SYSTEM_ROUTINE(KeAddProcessorAffinityEx);
    GET_SYSTEM_ROUTINE_TYPE(KeGetProcessorIndexFromNumber, PFNKEGETPROCESSORINDEXFROMNUMBER);
    GET_SYSTEM_ROUTINE(KeGetProcessorNumberFromIndex);
    GET_SYSTEM_ROUTINE_TYPE(KeGetCurrentProcessorNumberEx, PFNKEGETCURRENTPROCESSORNUMBEREX);
    GET_SYSTEM_ROUTINE(KeQueryActiveProcessors);
    GET_SYSTEM_ROUTINE(KeQueryMaximumProcessorCount);
    GET_SYSTEM_ROUTINE(KeQueryMaximumProcessorCountEx);
    GET_SYSTEM_ROUTINE(KeQueryMaximumGroupCount);
    GET_SYSTEM_ROUTINE(KeQueryActiveProcessorCount);
    GET_SYSTEM_ROUTINE(KeQueryActiveProcessorCountEx);
    GET_SYSTEM_ROUTINE(KeQueryLogicalProcessorRelationship);
    GET_SYSTEM_ROUTINE(KeRegisterProcessorChangeCallback);
    GET_SYSTEM_ROUTINE(KeDeregisterProcessorChangeCallback);

    GET_SYSTEM_ROUTINE_TYPE(RtlGetVersion, PFNRTRTLGETVERSION);
#ifndef RT_ARCH_AMD64
    GET_SYSTEM_ROUTINE(KeQueryInterruptTime);
    GET_SYSTEM_ROUTINE(KeQuerySystemTime);
#endif
    GET_SYSTEM_ROUTINE_TYPE(KeQueryInterruptTimePrecise, PFNRTKEQUERYINTERRUPTTIMEPRECISE);
    GET_SYSTEM_ROUTINE_TYPE(KeQuerySystemTimePrecise, PFNRTKEQUERYSYSTEMTIMEPRECISE);

#ifdef IPRT_TARGET_NT4
    g_pfnrtHalRequestIpiW7Plus = NULL;
    g_pfnrtHalRequestIpiPreW7 = NULL;
#else
    RtlInitUnicodeString(&RoutineName, L"HalRequestIpi");
    g_pfnrtHalRequestIpiW7Plus = (PFNHALREQUESTIPI_W7PLUS)MmGetSystemRoutineAddress(&RoutineName);
    g_pfnrtHalRequestIpiPreW7 = (PFNHALREQUESTIPI_PRE_W7)g_pfnrtHalRequestIpiW7Plus;
#endif

    /*
     * HACK ALERT! (and déjà vu warning - remember win32k.sys?)
     *
     * Try find _KPRCB::QuantumEnd and _KPRCB::[DpcData.]DpcQueueDepth.
     * For purpose of verification we use the VendorString member (12+1 chars).
     *
     * The offsets was initially derived by poking around with windbg
     * (dt _KPRCB, !prcb ++, and such like). Systematic harvesting was then
     * planned using dia2dump, grep and the symbol pack in a manner like this:
     *      dia2dump -type _KDPC_DATA -type _KPRCB EXE\ntkrnlmp.pdb | grep -wE "QuantumEnd|DpcData|DpcQueueDepth|VendorString"
     *
     * The final solution ended up using a custom harvester program called
     * ntBldSymDb that recursively searches thru unpacked symbol packages for
     * the desired structure offsets.  The program assumes that the packages
     * are unpacked into directories with the same name as the package, with
     * exception of some of the w2k packages which requires a 'w2k' prefix to
     * be distinguishable from another.
     */

    RTNTSDBOSVER OsVerInfo;
    rtR0NtGetOsVersionInfo(&OsVerInfo);

    /* Publish the version info in globals. */
    g_uRtNtVersion  = RTNT_MAKE_VERSION(OsVerInfo.uMajorVer, OsVerInfo.uMinorVer);
    g_uRtNtMinorVer = OsVerInfo.uMinorVer;
    g_uRtNtMajorVer = OsVerInfo.uMajorVer;
    g_uRtNtBuildNo  = OsVerInfo.uBuildNo;

    /*
     * Gather consistent CPU vendor string and PRCB pointers.
     */
    KIRQL OldIrql;
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql); /* make sure we stay on the same cpu */

    union
    {
        uint32_t auRegs[4];
        char szVendor[4*3+1];
    } u;
    ASMCpuId(0, &u.auRegs[3], &u.auRegs[0], &u.auRegs[2], &u.auRegs[1]);
    u.szVendor[4*3] = '\0';

    uint8_t *pbPrcb;
    __try /* Warning. This try/except statement may provide some false safety. */
    {
#if defined(RT_ARCH_X86)
        PKPCR    pPcr   = (PKPCR)__readfsdword(RT_OFFSETOF(KPCR,SelfPcr));
        pbPrcb = (uint8_t *)pPcr->Prcb;
#elif defined(RT_ARCH_AMD64)
        PKPCR    pPcr   = (PKPCR)__readgsqword(RT_OFFSETOF(KPCR,Self));
        pbPrcb = (uint8_t *)pPcr->CurrentPrcb;
#else
# error "port me"
        pbPrcb = NULL;
#endif
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        pbPrcb = NULL;
    }

    /*
     * Search the database
     */
    if (pbPrcb)
    {
        /* Find the best matching kernel version based on build number. */
        uint32_t iBest      = UINT32_MAX;
        int32_t  iBestDelta = INT32_MAX;
        for (uint32_t i = 0; i < RT_ELEMENTS(g_artNtSdbSets); i++)
        {
            if (g_artNtSdbSets[i].OsVerInfo.fChecked != OsVerInfo.fChecked)
                continue;
            if (OsVerInfo.fSmp /*must-be-smp*/ && !g_artNtSdbSets[i].OsVerInfo.fSmp)
                continue;

            int32_t iDelta = RT_ABS((int32_t)OsVerInfo.uBuildNo - (int32_t)g_artNtSdbSets[i].OsVerInfo.uBuildNo);
            if (   iDelta == 0
                && (g_artNtSdbSets[i].OsVerInfo.uCsdNo == OsVerInfo.uCsdNo || OsVerInfo.uCsdNo == MY_NIL_CSD))
            {
                /* prefect */
                iBestDelta = iDelta;
                iBest      = i;
                break;
            }
            if (   iDelta < iBestDelta
                || iBest == UINT32_MAX
                || (   iDelta == iBestDelta
                    && OsVerInfo.uCsdNo != MY_NIL_CSD
                    &&   RT_ABS(g_artNtSdbSets[i    ].OsVerInfo.uCsdNo - (int32_t)OsVerInfo.uCsdNo)
                       < RT_ABS(g_artNtSdbSets[iBest].OsVerInfo.uCsdNo - (int32_t)OsVerInfo.uCsdNo)
                   )
                )
            {
                iBestDelta = iDelta;
                iBest      = i;
            }
        }
        if (iBest < RT_ELEMENTS(g_artNtSdbSets))
        {
            /* Try all sets: iBest -> End; iBest -> Start. */
            bool    fDone = false;
            int32_t i     = iBest;
            while (   i < RT_ELEMENTS(g_artNtSdbSets)
                   && !(fDone = rtR0NtTryMatchSymSet(&g_artNtSdbSets[i], pbPrcb, u.szVendor, &OsVerInfo)))
                i++;
            if (!fDone)
            {
                i = (int32_t)iBest - 1;
                while (   i >= 0
                       && !(fDone = rtR0NtTryMatchSymSet(&g_artNtSdbSets[i], pbPrcb, u.szVendor, &OsVerInfo)))
                    i--;
            }
        }
        else
            DbgPrint("IPRT: Failed to locate data set.\n");
    }
    else
        DbgPrint("IPRT: Failed to get PCBR pointer.\n");

    KeLowerIrql(OldIrql); /* Lowering the IRQL early in the hope that we may catch exceptions below. */

#ifndef IN_GUEST
    if (!g_offrtNtPbQuantumEnd && !g_offrtNtPbDpcQueueDepth)
        DbgPrint("IPRT: Neither _KPRCB::QuantumEnd nor _KPRCB::DpcQueueDepth was not found! Kernel %u.%u %u %s\n",
                 OsVerInfo.uMajorVer, OsVerInfo.uMinorVer, OsVerInfo.uBuildNo, OsVerInfo.fChecked ? "checked" : "free");
# ifdef DEBUG
    else
        DbgPrint("IPRT: _KPRCB:{.QuantumEnd=%x/%d, .DpcQueueDepth=%x/%d} Kernel %u.%u %u %s\n",
                 g_offrtNtPbQuantumEnd, g_cbrtNtPbQuantumEnd, g_offrtNtPbDpcQueueDepth, g_offrtNtPbDpcQueueDepth,
                 OsVerInfo.uMajorVer, OsVerInfo.uMinorVer, OsVerInfo.uBuildNo, OsVerInfo.fChecked ? "checked" : "free");
# endif
#endif

    /*
     * Initialize multi processor stuff.  This registers a callback, so
     * we call rtR0TermNative to do the deregistration on failure.
     */
    int rc = rtR0MpNtInit(&OsVerInfo);
    if (RT_FAILURE(rc))
    {
        rtR0TermNative();
        DbgPrint("IPRT: Fatal: rtR0MpNtInit failed: %d\n", rc);
        return rc;
    }

    return VINF_SUCCESS;
}


DECLHIDDEN(void) rtR0TermNative(void)
{
    rtR0MpNtTerm();
}

