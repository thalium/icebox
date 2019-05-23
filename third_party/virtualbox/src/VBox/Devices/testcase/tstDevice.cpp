/* $Id: tstDevice.cpp $ */
/** @file
 * tstDevice - Test framework for PDM devices/drivers
 */

/*
 * Copyright (C) 2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DEFAULT /** @todo */
#include <VBox/types.h>
#include <VBox/sup.h>
#include <VBox/version.h>
#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/ldr.h>
#include <iprt/log.h>
#include <iprt/list.h>
#include <iprt/mem.h>
#include <iprt/once.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/stream.h>
#include <iprt/trace.h>

#include "tstDeviceInternal.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/


/**
 * Testcase plugin descriptor.
 */
typedef struct TSTDEVPLUGIN
{
    /** Node for the plugin list. */
    RTLISTNODE                      NdPlugins;
    /** Copy of the filename. */
    char                            *pszFilename;
    /** Loader handle. */
    RTLDRMOD                        hMod;
    /** Number of references to this plugin. */
    volatile uint32_t               cRefs;
} TSTDEVPLUGIN;
/** Pointer to a device plugin descriptor. */
typedef TSTDEVPLUGIN *PTSTDEVPLUGIN;
/** Pointer to a const plugin descriptor. */
typedef const TSTDEVPLUGIN *PCTSTDEVPLUGIN;


/**
 * Testcase descriptor.
 */
typedef struct TSTDEVTESTCASE
{
    /** Node for the list of registered testcases. */
    RTLISTNODE                      NdTestcases;
    /** Pointer to the plugin the testcase belongs to. */
    PCTSTDEVPLUGIN                  pPlugin;
    /** Pointer to the testcase descriptor. */
    PCTSTDEVTESTCASEREG             pTestcaseReg;
} TSTDEVTESTCASE;
/** Pointer to a testcase descriptor. */
typedef TSTDEVTESTCASE *PTSTDEVTESTCASE;
/** Pointer to a constant testcase descriptor. */
typedef const TSTDEVTESTCASE *PCTSTDEVTESTCASE;


/**
 * PDM R0/RC module trampoline descriptor.
 */
#pragma pack(1)
typedef struct TSTDEVPDMMODTRAMPOLINE
{
    /** Jump instruction. */
    uint8_t                         abJmp[6];
    /** Address to jump to. */
    uintptr_t                       AddrTarget;
    /** Padding to get a 16byte aligned structure. */
    uint8_t                         abPadding[HC_ARCH_BITS == 64 ? 2 : 5];
} TSTDEVPDMMODTRAMPOLINE;
#pragma pack()
AssertCompileSize(TSTDEVPDMMODTRAMPOLINE, 16);
/** Pointer to a trampoline descriptor. */
typedef TSTDEVPDMMODTRAMPOLINE *PTSTDEVPDMMODTRAMPOLINE;

/**
 * PDM module descriptor.
 */
typedef struct TSTDEVPDMMOD
{
    /** Node for the module list. */
    RTLISTNODE                      NdPdmMods;
    /** Type of module (R3/R0/RC). */
    TSTDEVPDMMODTYPE                enmType;
    /** Copy of the filename. */
    char                            *pszFilename;
    /** Loader handle. */
    RTLDRMOD                        hLdrMod;
    /** Number of references to this plugin. */
    volatile uint32_t               cRefs;
    /** R0/RC Module type dependent data. */
    struct
    {
        /** The exectuable image bits. */
        void                        *pvBits;
        /** Size of the memory buffer. */
        size_t                      cbBits;
        /** Pointer to the executable memory containing the trampoline code. */
        uint8_t                     *pbTrampoline;
        /** Number of trampoline entries supported. */
        uint32_t                    cTrampolinesMax;
        /** Number of trampoline entries used. */
        uint32_t                    cTrampolines;
        /** Pointer to the next unused trampoline entry. */
        PTSTDEVPDMMODTRAMPOLINE     pTrampolineNext;
    } R0Rc;
} TSTDEVPDMMOD;
/** Pointer to a PDM module descriptor. */
typedef TSTDEVPDMMOD *PTSTDEVPDMMOD;
/** Pointer to a const PDM module descriptor. */
typedef const TSTDEVPDMMOD *PCTSTDEVPDMMOD;

/**
 * PDM device descriptor.
 */
typedef struct TSTDEVPDMDEV
{
    /** Node for the known device list. */
    RTLISTNODE                      NdPdmDevs;
    /** Pointer to the PDM module containing the device. */
    PCTSTDEVPDMMOD                  pPdmMod;
    /** Device registration structure. */
    const struct PDMDEVREG          *pReg;
} TSTDEVPDMDEV;
/** Pointer to a PDM device descriptor .*/
typedef TSTDEVPDMDEV *PTSTDEVPDMDEV;
/** Pointer to a constant PDM device descriptor .*/
typedef const TSTDEVPDMDEV *PCTSTDEVPDMDEV;


/**
 * Internal callback structure pointer.
 * The main purpose is to define the extra data we associate
 * with PDMDEVREGCB so we can find the plugin the device is associated with etc.
 */
typedef struct TSTDEVPDMDEVREGCBINT
{
    /** The callback structure. */
    PDMDEVREGCB       Core;
    /** A bit of padding. */
    uint32_t          u32[4];
    /** Pointer to plugin. */
    PTSTDEVPDMMOD     pMod;
} TSTDEVPDMDEVREGCBINT;
/** Pointer to a PDMDEVREGCBINT structure. */
typedef TSTDEVPDMDEVREGCBINT *PTSTDEVPDMDEVREGCBINT;
/** Pointer to a const PDMDEVREGCBINT structure. */
typedef const TSTDEVPDMDEVREGCBINT *PCTSTDEVPDMDEVREGCBINT;


typedef struct TSTDEVPDMR0IMPORTS
{
    /** The symbol name. */
    const char                      *pszSymbol;
    /** The pointer. */
    PFNRT                           pfn;
} TSTDEVPDMR0IMPORTS;
typedef const TSTDEVPDMR0IMPORTS *PCTSTDEVPDMR0IMPORTS;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** List of registered testcase plugins. */
RTLISTANCHOR g_LstPlugins;
/** List of registered testcases. */
RTLISTANCHOR g_LstTestcases;
/** List of registered PDM modules. */
RTLISTANCHOR g_LstPdmMods;
/** List of registered PDM devices. */
RTLISTANCHOR g_LstPdmDevs;

extern const TSTDEVVMMCALLBACKS g_tstDevVmmCallbacks;

/**
 * PDM R0 imports we implement.
 */
static const TSTDEVPDMR0IMPORTS g_aPdmR0Imports[] =
{
    {"IOMMMIOMapMMIO2Page",            (PFNRT)IOMMMIOMapMMIO2Page},
    {"IOMMMIOResetRegion",             (PFNRT)IOMMMIOResetRegion},
    {"IntNetR0IfSend",                 (PFNRT)/*IntNetR0IfSend*/NULL},
    {"IntNetR0IfSetPromiscuousMode",   (PFNRT)/*IntNetR0IfSetPromiscuousMode*/NULL},
    {"PDMCritSectEnterDebug",          (PFNRT)PDMCritSectEnterDebug},
    {"PDMCritSectIsOwner",             (PFNRT)PDMCritSectIsOwner},
    {"PDMCritSectLeave",               (PFNRT)PDMCritSectLeave},
    {"PDMCritSectTryEnterDebug",       (PFNRT)PDMCritSectTryEnterDebug},
    {"PDMHCCritSectScheduleExitEvent", (PFNRT)PDMHCCritSectScheduleExitEvent},
    {"PDMNsAllocateBandwidth",         (PFNRT)PDMNsAllocateBandwidth},
    {"PDMQueueAlloc",                  (PFNRT)PDMQueueAlloc},
    {"PDMQueueInsert",                 (PFNRT)PDMQueueInsert},
    {"PGMHandlerPhysicalPageTempOff",  (PFNRT)PGMHandlerPhysicalPageTempOff},
    {"PGMShwMakePageWritable",         (PFNRT)PGMShwMakePageWritable},
    {"RTAssertMsg1Weak",               (PFNRT)RTAssertMsg1Weak},
    {"RTAssertMsg2Weak",               (PFNRT)RTAssertMsg2Weak},
    {"RTAssertShouldPanic",            (PFNRT)RTAssertShouldPanic},
    {"RTLogDefaultInstanceEx",         (PFNRT)RTLogDefaultInstanceEx},
    {"RTLogLoggerEx",                  (PFNRT)RTLogLoggerEx},
    {"RTLogRelGetDefaultInstanceEx",   (PFNRT)RTLogRelGetDefaultInstanceEx},
    {"RTOnceSlow",                     (PFNRT)RTOnceSlow},
    {"RTR0AssertPanicSystem",          (PFNRT)0x10101010},
    {"RTThreadSleep",                  (PFNRT)RTThreadSleep},
    {"RTTimeMilliTS",                  (PFNRT)RTTimeMilliTS},
    {"RTTimeNanoTS",                   (PFNRT)RTTimeNanoTS},
    {"RTTraceBufAddMsgF",              (PFNRT)RTTraceBufAddMsgF},
    {"SUPSemEventSignal",              (PFNRT)SUPSemEventSignal},
    {"TMTimerGet",                     (PFNRT)TMTimerGet},
    {"TMTimerGetFreq",                 (PFNRT)TMTimerGetFreq},
    {"TMTimerIsActive",                (PFNRT)TMTimerIsActive},
    {"TMTimerIsLockOwner",             (PFNRT)TMTimerIsLockOwner},
    {"TMTimerLock",                    (PFNRT)TMTimerLock},
    {"TMTimerSet",                     (PFNRT)TMTimerSet},
    {"TMTimerSetFrequencyHint",        (PFNRT)TMTimerSetFrequencyHint},
    {"TMTimerSetMicro",                (PFNRT)TMTimerSetMicro},
    {"TMTimerSetMillies",              (PFNRT)TMTimerSetMillies},
    {"TMTimerSetNano",                 (PFNRT)TMTimerSetNano},
    {"TMTimerStop",                    (PFNRT)TMTimerStop},
    {"TMTimerUnlock",                  (PFNRT)TMTimerUnlock},
    {"nocrt_memcmp",                   (PFNRT)memcmp},
    {"nocrt_memcpy",                   (PFNRT)memcpy},
    {"nocrt_memmove",                  (PFNRT)memmove},
    {"nocrt_memset",                   (PFNRT)memset},
    {"nocrt_strlen",                   (PFNRT)strlen},
};


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

#if 0
/**
 * Parses the options given to the testcase.
 *
 * @returns Process status code.
 * @param   cArgs       Number of arguments given.
 * @param   paszArgs    Pointer to the argument vector.
 */
static RTEXITCODE tstDevParseOptions(int cArgs, char *paszArgs[])
{
    static RTGETOPTDEF const s_aOptions[] =
    {
    };


    int ch;
    RTGETOPTUNION Value;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, 0 /* fFlags */);
    while ((ch = RTGetOpt(&GetState, &Value)))
    {
        switch (ch)
        {
            default:
                return RTGetOptPrintError(ch, &Value);
        }
    }
}
#endif


/**
 * Checks whether the given testcase name is already existing.
 *
 * @returns Pointer to existing testcase, NULL if not found.
 * @param   pszFilename    The filename to check.
 */
static PCTSTDEVTESTCASE tstDevTestcaseFind(const char *pszName)
{
    PCTSTDEVTESTCASE pIt;
    RTListForEach(&g_LstTestcases, pIt, TSTDEVTESTCASE, NdTestcases)
    {
        if (!RTStrCmp(pIt->pTestcaseReg->szName, pszName))
            return pIt;
    }

    return NULL;
}


/**
 * @interface_method_impl{TSTDEVPLUGINREGISTER,pfnRegisterTestcase}
 */
static DECLCALLBACK(int) tstDevRegisterTestcase(void *pvUser, PCTSTDEVTESTCASEREG pTestcaseReg)
{
    int rc = VINF_SUCCESS;
    PTSTDEVPLUGIN pPlugin = (PTSTDEVPLUGIN)pvUser;

    /* Try to find a testcase with the name first. */
    if (!tstDevTestcaseFind(pTestcaseReg->szName))
    {
        PTSTDEVTESTCASE pTestcase = (PTSTDEVTESTCASE)RTMemAllocZ(sizeof(TSTDEVPLUGIN));
        if (RT_LIKELY(pTestcase))
        {
            pTestcase->pPlugin = pPlugin;
            pPlugin->cRefs++;
            pTestcase->pTestcaseReg = pTestcaseReg;
            RTListAppend(&g_LstTestcases, &pTestcase->NdTestcases);
            return VINF_SUCCESS;
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else
        rc = VERR_ALREADY_EXISTS;

    return rc;
}


/**
 * Checks whether the given plugin filename was already loaded.
 *
 * @returns Pointer to already loaded plugin, NULL if not found.
 * @param   pszFilename    The filename to check.
 */
static PCTSTDEVPLUGIN tstDevPluginFind(const char *pszFilename)
{
    PCTSTDEVPLUGIN pIt;
    RTListForEach(&g_LstPlugins, pIt, TSTDEVPLUGIN, NdPlugins)
    {
        if (!RTStrCmp(pIt->pszFilename, pszFilename))
            return pIt;
    }

    return NULL;
}


/**
 * Tries to loads the given plugin.
 *
 * @returns VBox status code.
 * @param   pszFilename    The filename to load.
 */
static int tstDevLoadPlugin(const char *pszFilename)
{
    int rc = VINF_SUCCESS;

    /* Check whether the plugin is loaded first. */
    if (!tstDevPluginFind(pszFilename))
    {
        PTSTDEVPLUGIN pPlugin = (PTSTDEVPLUGIN)RTMemAllocZ(sizeof(TSTDEVPLUGIN));
        if (RT_LIKELY(pPlugin))
        {
            pPlugin->pszFilename = RTStrDup(pszFilename);
            pPlugin->cRefs       = 1;
            rc = RTLdrLoad(pszFilename, &pPlugin->hMod);
            if (RT_SUCCESS(rc))
            {
                TSTDEVPLUGINREGISTER TestcaseRegister;
                PFNTSTDEVPLUGINLOAD pfnPluginLoad = NULL;

                TestcaseRegister.pfnRegisterTestcase = tstDevRegisterTestcase;

                rc = RTLdrGetSymbol(pPlugin->hMod, TSTDEV_PLUGIN_LOAD_NAME, (void**)&pfnPluginLoad);
                if (RT_FAILURE(rc) || !pfnPluginLoad)
                {
                    LogFunc(("error resolving the entry point %s in plugin %s, rc=%Rrc, pfnPluginLoad=%#p\n",
                             TSTDEV_PLUGIN_LOAD_NAME, pszFilename, rc, pfnPluginLoad));
                    if (RT_SUCCESS(rc))
                        rc = VERR_SYMBOL_NOT_FOUND;
                }

                if (RT_SUCCESS(rc))
                {
                    /* Get the function table. */
                    rc = pfnPluginLoad(pPlugin, &TestcaseRegister);
                }
                else
                    LogFunc(("ignored plugin '%s': rc=%Rrc\n", pszFilename, rc));

                /* Create a plugin entry on success. */
                if (RT_SUCCESS(rc))
                {
                    RTListAppend(&g_LstPlugins, &pPlugin->NdPlugins);
                    return VINF_SUCCESS;
                }
                else
                    RTLdrClose(pPlugin->hMod);
            }

            RTMemFree(pPlugin);
        }
        else
            rc = VERR_NO_MEMORY;
    }

    return rc;
}


/**
 * Checks whether the given testcase name is already existing.
 *
 * @returns Pointer to already loaded plugin, NULL if not found.
 * @param   pszFilename    The filename to check.
 */
static PCTSTDEVPDMDEV tstDevPdmDeviceFind(const char *pszName)
{
    PCTSTDEVPDMDEV pIt;
    RTListForEach(&g_LstPdmDevs, pIt, TSTDEVPDMDEV, NdPdmDevs)
    {
        if (!RTStrCmp(pIt->pReg->szName, pszName))
            return pIt;
    }

    return NULL;
}


/**
 * Checks that a PDMDRVREG::szName, PDMDEVREG::szName or PDMUSBREG::szName
 * field contains only a limited set of ASCII characters.
 *
 * @returns true / false.
 * @param   pszName             The name to validate.
 */
bool tstDevPdmR3IsValidName(const char *pszName)
{
    char ch;
    while (   (ch = *pszName) != '\0'
           && (   RT_C_IS_ALNUM(ch)
               || ch == '-'
               || ch == ' ' /** @todo disallow this! */
               || ch == '_') )
        pszName++;
    return ch == '\0';
}


/**
 * @interface_method_impl{PDMDEVREGCB,pfnRegister}
 */
static DECLCALLBACK(int) tstDevPdmR3DevReg_Register(PPDMDEVREGCB pCallbacks, PCPDMDEVREG pReg)
{
    /*
     * Validate the registration structure (mostly copy and paste from PDMDevice.cpp).
     */
    Assert(pReg);
    AssertMsgReturn(pReg->u32Version == PDM_DEVREG_VERSION,
                    ("Unknown struct version %#x!\n", pReg->u32Version),
                    VERR_PDM_UNKNOWN_DEVREG_VERSION);

    AssertMsgReturn(    pReg->szName[0]
                    &&  strlen(pReg->szName) < sizeof(pReg->szName)
                    &&  tstDevPdmR3IsValidName(pReg->szName),
                    ("Invalid name '%.*s'\n", sizeof(pReg->szName), pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn(   !(pReg->fFlags & PDM_DEVREG_FLAGS_RC)
                    || (   pReg->szRCMod[0]
                        && strlen(pReg->szRCMod) < sizeof(pReg->szRCMod)),
                    ("Invalid GC module name '%s' - (Device %s)\n", pReg->szRCMod, pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn(   !(pReg->fFlags & PDM_DEVREG_FLAGS_R0)
                    || (   pReg->szR0Mod[0]
                        && strlen(pReg->szR0Mod) < sizeof(pReg->szR0Mod)),
                    ("Invalid R0 module name '%s' - (Device %s)\n", pReg->szR0Mod, pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn((pReg->fFlags & PDM_DEVREG_FLAGS_HOST_BITS_MASK) == PDM_DEVREG_FLAGS_HOST_BITS_DEFAULT,
                    ("Invalid host bits flags! fFlags=%#x (Device %s)\n", pReg->fFlags, pReg->szName),
                    VERR_PDM_INVALID_DEVICE_HOST_BITS);
    AssertMsgReturn((pReg->fFlags & PDM_DEVREG_FLAGS_GUEST_BITS_MASK),
                    ("Invalid guest bits flags! fFlags=%#x (Device %s)\n", pReg->fFlags, pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn(pReg->fClass,
                    ("No class! (Device %s)\n", pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn(pReg->cMaxInstances > 0,
                    ("Max instances %u! (Device %s)\n", pReg->cMaxInstances, pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn(pReg->cbInstance <= (uint32_t)(pReg->fFlags & (PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0)  ? 96 * _1K : _1M),
                    ("Instance size %d bytes! (Device %s)\n", pReg->cbInstance, pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertMsgReturn(pReg->pfnConstruct,
                    ("No constructor! (Device %s)\n", pReg->szName),
                    VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn((pReg->fFlags & PDM_DEVREG_FLAGS_GUEST_BITS_MASK) == PDM_DEVREG_FLAGS_GUEST_BITS_DEFAULT,
                          ("PDM: Rejected device '%s' because it didn't match the guest bits.\n", pReg->szName),
                          VERR_PDM_INVALID_DEVICE_GUEST_BITS);
    AssertLogRelMsg(pReg->u32VersionEnd == PDM_DEVREG_VERSION,
                    ("u32VersionEnd=%#x, expected %#x. (szName=%s)\n",
                     pReg->u32VersionEnd, PDM_DEVREG_VERSION, pReg->szName));

    /*
     * Check for duplicates.
     */
    int rc = VINF_SUCCESS;
    PCTSTDEVPDMDEVREGCBINT pRegCB = (PCTSTDEVPDMDEVREGCBINT)pCallbacks;
    if (!tstDevPdmDeviceFind(pReg->szName))
    {
        PTSTDEVPDMDEV pPdmDev = (PTSTDEVPDMDEV)RTMemAllocZ(sizeof(TSTDEVPDMDEV));
        if (RT_LIKELY(pPdmDev))
        {
            pPdmDev->pPdmMod = pRegCB->pMod;
            pRegCB->pMod->cRefs++;
            pPdmDev->pReg = pReg;
            RTListAppend(&g_LstPdmDevs, &pPdmDev->NdPdmDevs);
            return VINF_SUCCESS;
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else
        rc = VERR_PDM_DEVICE_NAME_CLASH;

    return rc;
}


/**
 * Checks whether the given PDM module filename was already loaded.
 *
 * @returns Pointer to already loaded plugin, NULL if not found.
 * @param   pszFilename    The filename to check.
 */
static PCTSTDEVPDMMOD tstDevPdmModFind(const char *pszFilename)
{
    PCTSTDEVPDMMOD pIt;
    RTListForEach(&g_LstPdmMods, pIt, TSTDEVPDMMOD, NdPdmMods)
    {
        if (!RTStrCmp(pIt->pszFilename, pszFilename))
            return pIt;
    }

    return NULL;
}


/**
 * Resolve an external symbol during RTLdrGetBits().
 *
 * @returns iprt status code.
 * @param   hLdrMod         The loader module handle.
 * @param   pszModule       Module name.
 * @param   pszSymbol       Symbol name, NULL if uSymbol should be used.
 * @param   uSymbol         Symbol ordinal, ~0 if pszSymbol should be used.
 * @param   pValue          Where to store the symbol value (address).
 * @param   pvUser          User argument.
 */
static DECLCALLBACK(int) tstDevPdmLoadR0RcModGetImport(RTLDRMOD hLdrMod, const char *pszModule, const char *pszSymbol,
                                                       unsigned uSymbol, RTUINTPTR *pValue, void *pvUser)
{
    RT_NOREF(hLdrMod, uSymbol, pszModule);
    PTSTDEVPDMMOD pMod = (PTSTDEVPDMMOD)pvUser;

    /* Resolve the import. */
    PCTSTDEVPDMR0IMPORTS pImpDesc = NULL;
    bool fFound = false;
    for (uint32_t i = 0; i < RT_ELEMENTS(g_aPdmR0Imports); i++)
    {
        pImpDesc = &g_aPdmR0Imports[i];
        if (!strcmp(pszSymbol, pImpDesc->pszSymbol))
        {
            fFound = true;
            break;
        }
    }

    int rc = VERR_SYMBOL_NOT_FOUND;
    if (fFound)
    {
        /* Check whether the symbol has a trampoline already. */
        PTSTDEVPDMMODTRAMPOLINE pTrampoline = (PTSTDEVPDMMODTRAMPOLINE)pMod->R0Rc.pbTrampoline;
        for (uint32_t i = 0; i < pMod->R0Rc.cTrampolines; i++)
        {
            if (pTrampoline->AddrTarget == (uintptr_t)pImpDesc->pfn)
                break;
            pTrampoline++;
        }

        /* Create new trampoline if not found. */
        if (pTrampoline->AddrTarget != (uintptr_t)pImpDesc->pfn)
        {
            if (pMod->R0Rc.cTrampolines < pMod->R0Rc.cTrampolinesMax)
            {
                pTrampoline = pMod->R0Rc.pTrampolineNext;
                pMod->R0Rc.pTrampolineNext++;
                pMod->R0Rc.cTrampolines++;
                pTrampoline->abJmp[0]   = 0xff; /* jmp */
                pTrampoline->abJmp[1]   = 0x25; /* rip */
                pTrampoline->abJmp[2]   = 0x00; /* offset */
                pTrampoline->abJmp[3]   = 0x00;
                pTrampoline->abJmp[4]   = 0x00;
                pTrampoline->abJmp[5]   = 0x00;
                pTrampoline->AddrTarget = (uintptr_t)pImpDesc->pfn;
                rc = VINF_SUCCESS;
            }
            else
            {
                rc = VERR_SYMBOL_NOT_FOUND;
                AssertFailed();
            }
        }
        else
            rc = VINF_SUCCESS;

        if (RT_SUCCESS(rc))
            *pValue = (RTUINTPTR)pTrampoline;
    }
    else
        AssertFailed();

    return rc;
}


/**
 * Loads a new R0 modules given by the filename.
 *
 * @returns VBox status code.
 * @param   pMod                    Pointer to module structure.
 */
static int tstDevPdmLoadR0RcMod(PTSTDEVPDMMOD pMod)
{
    int rc = VINF_SUCCESS;
    const char *pszFile = RTPathFilename(pMod->pszFilename);

    /* Check whether the plugin is loaded first. */
    if (!tstDevPdmModFind(pszFile))
    {
        /*
         * R0 modules need special treatment as these are relocatable images
         * which are supposed to run in ring 0.
         */
        rc = RTLdrOpen(pMod->pszFilename, 0, RTLDRARCH_HOST, &pMod->hLdrMod);
        if (RT_SUCCESS(rc))
        {
            size_t cb = RTLdrSize(pMod->hLdrMod) + 1024 * sizeof(TSTDEVPDMMODTRAMPOLINE);

            /* Allocate bits. */
            uint32_t fFlags = RTMEMALLOCEX_FLAGS_EXEC;
#ifdef RT_OS_LINUX
            /*
             * amd64 ELF binaries support only a 2GB code segment everything must be in
             * (X86_64_PC32 relocation) so we have to use a trampoline to the final destination
             * which is kept close to the imported module.
             */
            fFlags |= RTMEMALLOCEX_FLAGS_32BIT_REACH;
#endif
            rc = RTMemAllocEx(cb, 0, fFlags, (void **)&pMod->R0Rc.pbTrampoline);
            pMod->R0Rc.cbBits = cb;
            if (RT_SUCCESS(rc))
            {
                pMod->R0Rc.pvBits          = pMod->R0Rc.pbTrampoline + 1024 * sizeof(TSTDEVPDMMODTRAMPOLINE);
                pMod->R0Rc.cTrampolinesMax = 1024;
                pMod->R0Rc.cTrampolines    = 0;
                pMod->R0Rc.pTrampolineNext = (PTSTDEVPDMMODTRAMPOLINE)pMod->R0Rc.pbTrampoline;
                /* Get the bits. */
                rc = RTLdrGetBits(pMod->hLdrMod, pMod->R0Rc.pvBits, (uintptr_t)pMod->R0Rc.pvBits,
                                  tstDevPdmLoadR0RcModGetImport, pMod);
                if (RT_FAILURE(rc))
                    RTMemFreeEx(pMod->R0Rc.pbTrampoline, pMod->R0Rc.cbBits);
            }

            if (RT_FAILURE(rc))
                RTLdrClose(pMod->hLdrMod);
        }
    }

    return rc;
}


/**
 * Loads the given
 */
static int tstDevPdmLoadR3Mod(PTSTDEVPDMMOD pMod)
{
    int rc = RTLdrLoad(pMod->pszFilename, &pMod->hLdrMod);
    if (RT_SUCCESS(rc))
    {
        FNPDMVBOXDEVICESREGISTER *pfnVBoxDevicesRegister;
        rc = RTLdrGetSymbol(pMod->hLdrMod, "VBoxDevicesRegister", (void**)&pfnVBoxDevicesRegister);
        if (RT_FAILURE(rc) || !pfnVBoxDevicesRegister)
        {
            LogFunc(("error resolving the entry point %s in plugin %s, rc=%Rrc, pfnPluginLoad=%#p\n",
                     "VBoxDevicesRegister", pMod->pszFilename, rc, pfnVBoxDevicesRegister));
            if (RT_SUCCESS(rc))
                rc = VERR_SYMBOL_NOT_FOUND;
        }

        if (RT_SUCCESS(rc))
        {
            TSTDEVPDMDEVREGCBINT RegCB;
            RegCB.Core.u32Version  = PDM_DEVREG_CB_VERSION;
            RegCB.Core.pfnRegister = tstDevPdmR3DevReg_Register;
            RegCB.pMod             = pMod;
            rc = pfnVBoxDevicesRegister(&RegCB.Core, VBOX_VERSION);
        }
        else
            LogFunc(("ignored plugin '%s': rc=%Rrc\n", pMod->pszFilename, rc));

        if (RT_FAILURE(rc))
            RTLdrClose(pMod->hLdrMod);
    }

    return rc;
}


/**
 * Tries to loads the given PDM module.
 *
 * @returns VBox status code.
 * @param   pszFilename    The filename to load.
 * @param   enmModType     The module type.
 */
static int tstDevPdmLoadMod(const char *pszFilename, TSTDEVPDMMODTYPE enmModType)
{
    int rc = VINF_SUCCESS;

    /* Check whether the plugin is loaded first. */
    if (!tstDevPdmModFind(pszFilename))
    {
        PTSTDEVPDMMOD pMod = (PTSTDEVPDMMOD)RTMemAllocZ(sizeof(TSTDEVPDMMOD));
        if (RT_LIKELY(pMod))
        {
            pMod->pszFilename = RTStrDup(pszFilename);
            pMod->cRefs       = 1;
            pMod->enmType     = enmModType;

            if (enmModType == TSTDEVPDMMODTYPE_R3)
                rc = tstDevPdmLoadR3Mod(pMod);
            else if (enmModType == TSTDEVPDMMODTYPE_RC || enmModType == TSTDEVPDMMODTYPE_R0)
                rc = tstDevPdmLoadR0RcMod(pMod);

            if (RT_SUCCESS(rc))
                RTListAppend(&g_LstPdmMods, &pMod->NdPdmMods);
            else
                RTMemFree(pMod);
        }
        else
            rc = VERR_NO_MEMORY;
    }

    return rc;
}


/**
 * Tries to resolve the given symbol from the module given.
 *
 * @returns VBox status code.
 * @param   pThis                   The device under test instance.
 * @param   pszMod                  The module name.
 * @param   enmModType              The module type if the module needs to be loaded.
 * @param   pszSymbol               The symbol to resolve.
 * @param   ppfn                    Where to store the value on success.
 */
DECLHIDDEN(int) tstDevPdmLdrGetSymbol(PTSTDEVDUTINT pThis, const char *pszMod, TSTDEVPDMMODTYPE enmModType,
                                      const char *pszSymbol, PFNRT *ppfn)
{
    RT_NOREF(pThis);

    int rc = VINF_SUCCESS;
    PCTSTDEVPDMMOD pMod = tstDevPdmModFind(pszMod);
    if (!pMod)
    {
        /* Try to load the module. */
        rc = tstDevPdmLoadMod(pszMod, enmModType);
        if (RT_SUCCESS(rc))
        {
            pMod = tstDevPdmModFind(pszMod);
            AssertPtr(pMod);
        }
    }

    if (RT_SUCCESS(rc))
    {
        if (pMod->enmType == TSTDEVPDMMODTYPE_R0 || pMod->enmType == TSTDEVPDMMODTYPE_RC)
            rc = RTLdrGetSymbolEx(pMod->hLdrMod, pMod->R0Rc.pvBits, (uintptr_t)pMod->R0Rc.pvBits,
                                  UINT32_MAX, pszSymbol, (PRTLDRADDR)ppfn);
        else
            rc = RTLdrGetSymbol(pMod->hLdrMod, pszSymbol, (void **)ppfn);
    }

    return rc;
}


static TSTDEVCFGITEM s_aTestcaseCfg[] =
{
    {"CtrlMemBufSize", TSTDEVCFGITEMTYPE_INTEGER, "0"  },
    {NULL,             TSTDEVCFGITEMTYPE_INVALID, NULL }
};

static TSTDEVTESTCASEREG s_TestcaseDef =
{
    "test",
    "Testcase during implementation",
    "nvme",
    0,
    &s_aTestcaseCfg[0],
    NULL,
};

/**
 * Create a new PDM device with default config.
 *
 * @returns VBox status code.
 * @param   pszName                 Name of the device to create.
 */
static int tstDevPdmDevCreate(const char *pszName)
{
    int rc = VINF_SUCCESS;
    PCTSTDEVPDMDEV pPdmDev = tstDevPdmDeviceFind(pszName);
    if (RT_LIKELY(pPdmDev))
    {
        TSTDEVDUTINT Dut;
        Dut.pTestcaseReg    = &s_TestcaseDef;
        Dut.enmCtx          = TSTDEVDUTCTX_R3;
        Dut.pVm             = NULL;
        Dut.SupSession.pDut = &Dut;
        RTListInit(&Dut.LstIoPorts);
        RTListInit(&Dut.LstTimers);
        RTListInit(&Dut.LstMmHeap);
        RTListInit(&Dut.LstPdmThreads);
        RTListInit(&Dut.SupSession.LstSupSem);
        CFGMNODE Cfg;
        Cfg.pVmmCallbacks = &g_tstDevVmmCallbacks;
        Cfg.pDut = &Dut;

        rc = RTCritSectRwInit(&Dut.CritSectLists);
        AssertRC(rc);

        PPDMDEVINS pDevIns = (PPDMDEVINS)RTMemAllocZ(RT_OFFSETOF(PDMDEVINS, achInstanceData[pPdmDev->pReg->cbInstance]));
        pDevIns->u32Version               = PDM_DEVINS_VERSION;
        pDevIns->iInstance                = 0;
        pDevIns->pReg                     = pPdmDev->pReg;
        pDevIns->pvInstanceDataR3         = &pDevIns->achInstanceData[0];
        pDevIns->pHlpR3                   = &g_tstDevPdmDevHlpR3;
        pDevIns->pCfg                     = &Cfg;
        pDevIns->Internal.s.pVmmCallbacks = &g_tstDevVmmCallbacks;
        pDevIns->Internal.s.pDut          = &Dut;
        rc = pPdmDev->pReg->pfnConstruct(pDevIns, 0, &Cfg);
        if (RT_SUCCESS(rc))
        {
            PRTDEVDUTIOPORT pIoPort = RTListGetFirst(&Dut.LstIoPorts, RTDEVDUTIOPORT, NdIoPorts);
            uint32_t uVal = 0;
            PDMCritSectEnter(pDevIns->pCritSectRoR3, VERR_IGNORED);
            pIoPort->pfnInR0(pDevIns, pIoPort->pvUserR0, pIoPort->PortStart, &uVal, sizeof(uint8_t));
            PDMCritSectLeave(pDevIns->pCritSectRoR3);
        }
    }
    else
        rc = VERR_NOT_FOUND;

    return rc;
}

int main(int argc, char *argv[])
{
    /*
     * Init the runtime and parse the arguments.
     */
    RTEXITCODE rcExit = RTEXITCODE_SUCCESS;
    int rc = RTR3InitExe(argc, &argv, 0);
    if (RT_SUCCESS(rc))
    {
        RTListInit(&g_LstPlugins);
        RTListInit(&g_LstTestcases);
        RTListInit(&g_LstPdmMods);
        RTListInit(&g_LstPdmDevs);

        rc = tstDevLoadPlugin("TSTDevTestcases");
        if (RT_SUCCESS(rc) || true)
        {
            rc = tstDevPdmLoadMod(argv[1], TSTDEVPDMMODTYPE_R3);
            if (RT_SUCCESS(rc))
                rc = tstDevPdmDevCreate("nvme");
            else
                rcExit = RTEXITCODE_FAILURE;
        }
        else
            rcExit = RTEXITCODE_FAILURE;

        //rcExit = tstDevParseOptions(argc, argv);
    }
    else
        rcExit = RTEXITCODE_FAILURE;

    return rcExit;
}

