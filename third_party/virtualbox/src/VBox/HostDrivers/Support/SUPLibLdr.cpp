/* $Id: SUPLibLdr.cpp $ */
/** @file
 * VirtualBox Support Library - Loader related bits.
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

/** @page   pg_sup          SUP - The Support Library
 *
 * The support library is responsible for providing facilities to load
 * VMM Host Ring-0 code, to call Host VMM Ring-0 code from Ring-3 Host
 * code, to pin down physical memory, and more.
 *
 * The VMM Host Ring-0 code can be combined in the support driver if
 * permitted by kernel module license policies. If it is not combined
 * it will be externalized in a .r0 module that will be loaded using
 * the IPRT loader.
 *
 * The Ring-0 calling is done thru a generic SUP interface which will
 * transfer an argument set and call a predefined entry point in the Host
 * VMM Ring-0 code.
 *
 * See @ref grp_sup "SUP - Support APIs" for API details.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_SUP
#include <VBox/sup.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/log.h>
#include <VBox/VBoxTpG.h>

#include <iprt/assert.h>
#include <iprt/alloc.h>
#include <iprt/alloca.h>
#include <iprt/ldr.h>
#include <iprt/asm.h>
#include <iprt/mp.h>
#include <iprt/cpuset.h>
#include <iprt/thread.h>
#include <iprt/process.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/env.h>
#include <iprt/rand.h>
#include <iprt/x86.h>

#include "SUPDrvIOC.h"
#include "SUPLibInternal.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** R0 VMM module name. */
#define VMMR0_NAME      "VMMR0"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef DECLCALLBACK(int) FNCALLVMMR0(PVMR0 pVMR0, unsigned uOperation, void *pvArg);
typedef FNCALLVMMR0 *PFNCALLVMMR0;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** VMMR0 Load Address. */
static RTR0PTR                  g_pvVMMR0 = NIL_RTR0PTR;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int               supLoadModule(const char *pszFilename, const char *pszModule, const char *pszSrvReqHandler,
                                       PRTERRINFO pErrInfo, void **ppvImageBase);
static DECLCALLBACK(int) supLoadModuleResolveImport(RTLDRMOD hLdrMod, const char *pszModule, const char *pszSymbol,
                                                    unsigned uSymbol, RTUINTPTR *pValue, void *pvUser);


SUPR3DECL(int) SUPR3LoadModule(const char *pszFilename, const char *pszModule, void **ppvImageBase, PRTERRINFO pErrInfo)
{
    /*
     * Check that the module can be trusted.
     */
    int rc = SUPR3HardenedVerifyPlugIn(pszFilename, pErrInfo);
    if (RT_SUCCESS(rc))
    {
        rc = supLoadModule(pszFilename, pszModule, NULL, pErrInfo, ppvImageBase);
        if (RT_FAILURE(rc) && !RTErrInfoIsSet(pErrInfo))
            RTErrInfoSetF(pErrInfo, rc, "SUPR3LoadModule: supLoadModule returned %Rrc", rc);
    }
    return rc;
}


SUPR3DECL(int) SUPR3LoadServiceModule(const char *pszFilename, const char *pszModule,
                                      const char *pszSrvReqHandler, void **ppvImageBase)
{
    AssertPtrReturn(pszSrvReqHandler, VERR_INVALID_PARAMETER);

    /*
     * Check that the module can be trusted.
     */
    int rc = SUPR3HardenedVerifyPlugIn(pszFilename, NULL /*pErrInfo*/);
    if (RT_SUCCESS(rc))
        rc = supLoadModule(pszFilename, pszModule, pszSrvReqHandler, NULL /*pErrInfo*/, ppvImageBase);
    else
        LogRel(("SUPR3LoadServiceModule: Verification of \"%s\" failed, rc=%Rrc\n", pszFilename, rc));
    return rc;
}


/**
 * Argument package for supLoadModuleResolveImport.
 */
typedef struct SUPLDRRESIMPARGS
{
    const char *pszModule;
    PRTERRINFO  pErrInfo;
} SUPLDRRESIMPARGS, *PSUPLDRRESIMPARGS;

/**
 * Resolve an external symbol during RTLdrGetBits().
 *
 * @returns VBox status code.
 * @param   hLdrMod         The loader module handle.
 * @param   pszModule       Module name.
 * @param   pszSymbol       Symbol name, NULL if uSymbol should be used.
 * @param   uSymbol         Symbol ordinal, ~0 if pszSymbol should be used.
 * @param   pValue          Where to store the symbol value (address).
 * @param   pvUser          User argument.
 */
static DECLCALLBACK(int) supLoadModuleResolveImport(RTLDRMOD hLdrMod, const char *pszModule,
                                                    const char *pszSymbol, unsigned uSymbol, RTUINTPTR *pValue, void *pvUser)
{
    NOREF(hLdrMod); NOREF(uSymbol);
    AssertPtr(pValue);
    AssertPtr(pvUser);
    PSUPLDRRESIMPARGS pArgs = (PSUPLDRRESIMPARGS)pvUser;

    /*
     * Only SUPR0 and VMMR0.r0
     */
    if (    pszModule
        &&  *pszModule
        &&  strcmp(pszModule, "VBoxDrv.sys")
        &&  strcmp(pszModule, "VMMR0.r0"))
    {
        AssertMsgFailed(("%s is importing from %s! (expected 'SUPR0.dll' or 'VMMR0.r0', case-sensitive)\n", pArgs->pszModule, pszModule));
        return RTErrInfoSetF(pArgs->pErrInfo, VERR_SYMBOL_NOT_FOUND,
                             "Unexpected import module '%s' in '%s'", pszModule, pArgs->pszModule);
    }

    /*
     * No ordinals.
     */
    if (uSymbol != ~0U)
    {
        AssertMsgFailed(("%s is importing by ordinal (ord=%d)\n", pArgs->pszModule, uSymbol));
        return RTErrInfoSetF(pArgs->pErrInfo, VERR_SYMBOL_NOT_FOUND,
                             "Unexpected ordinal import (%#x) in '%s'", uSymbol, pArgs->pszModule);
    }

    /*
     * Lookup symbol.
     */
    /** @todo is this actually used??? */
    /* skip the 64-bit ELF import prefix first. */
    if (!strncmp(pszSymbol, RT_STR_TUPLE("SUPR0$")))
        pszSymbol += sizeof("SUPR0$") - 1;

    /*
     * Check the VMMR0.r0 module if loaded.
     */
    /** @todo call the SUPR3LoadModule caller.... */
    /** @todo proper reference counting and such. */
    if (g_pvVMMR0 != NIL_RTR0PTR)
    {
        void *pvValue;
        if (!SUPR3GetSymbolR0((void *)g_pvVMMR0, pszSymbol, &pvValue))
        {
            *pValue = (uintptr_t)pvValue;
            return VINF_SUCCESS;
        }
    }

    /* iterate the function table. */
    int c = g_pSupFunctions->u.Out.cFunctions;
    PSUPFUNC pFunc = &g_pSupFunctions->u.Out.aFunctions[0];
    while (c-- > 0)
    {
        if (!strcmp(pFunc->szName, pszSymbol))
        {
            *pValue = (uintptr_t)pFunc->pfn;
            return VINF_SUCCESS;
        }
        pFunc++;
    }

    /*
     * The GIP.
     */
    if (    pszSymbol
        &&  g_pSUPGlobalInfoPage
        &&  g_pSUPGlobalInfoPageR0
        &&  !strcmp(pszSymbol, "g_SUPGlobalInfoPage")
       )
    {
        *pValue = (uintptr_t)g_pSUPGlobalInfoPageR0;
        return VINF_SUCCESS;
    }

    /*
     * Symbols that are undefined by convention.
     */
#ifdef RT_OS_SOLARIS
    static const char * const s_apszConvSyms[] =
    {
        "", "mod_getctl",
        "", "mod_install",
        "", "mod_remove",
        "", "mod_info",
        "", "mod_miscops",
    };
    for (unsigned i = 0; i < RT_ELEMENTS(s_apszConvSyms); i += 2)
    {
        if (   !RTStrCmp(s_apszConvSyms[i],     pszModule)
            && !RTStrCmp(s_apszConvSyms[i + 1], pszSymbol))
        {
            *pValue = ~(uintptr_t)0;
            return VINF_SUCCESS;
        }
    }
#endif

    /*
     * Despair.
     */
    c = g_pSupFunctions->u.Out.cFunctions;
    pFunc = &g_pSupFunctions->u.Out.aFunctions[0];
    while (c-- > 0)
    {
        RTAssertMsg2Weak("%d: %s\n", g_pSupFunctions->u.Out.cFunctions - c, pFunc->szName);
        pFunc++;
    }
    RTAssertMsg2Weak("%s is importing %s which we couldn't find\n", pArgs->pszModule, pszSymbol);

    AssertLogRelMsgFailed(("%s is importing %s which we couldn't find\n", pArgs->pszModule, pszSymbol));
    if (g_uSupFakeMode)
    {
        *pValue = 0xdeadbeef;
        return VINF_SUCCESS;
    }
    return RTErrInfoSetF(pArgs->pErrInfo, VERR_SYMBOL_NOT_FOUND,
                         "Unable to locate imported symbol '%s%s%s' for module '%s'",
                         pszModule ? pszModule : "",
                         pszModule && *pszModule ? "." : "",
                         pszSymbol,
                         pArgs->pszModule);
}


/** Argument package for supLoadModuleCalcSizeCB. */
typedef struct SUPLDRCALCSIZEARGS
{
    size_t          cbStrings;
    uint32_t        cSymbols;
    size_t          cbImage;
} SUPLDRCALCSIZEARGS, *PSUPLDRCALCSIZEARGS;

/**
 * Callback used to calculate the image size.
 * @return VINF_SUCCESS
 */
static DECLCALLBACK(int) supLoadModuleCalcSizeCB(RTLDRMOD hLdrMod, const char *pszSymbol, unsigned uSymbol, RTUINTPTR Value, void *pvUser)
{
    PSUPLDRCALCSIZEARGS pArgs = (PSUPLDRCALCSIZEARGS)pvUser;
    if (    pszSymbol != NULL
        &&  *pszSymbol
        &&  Value <= pArgs->cbImage)
    {
        pArgs->cSymbols++;
        pArgs->cbStrings += strlen(pszSymbol) + 1;
    }
    NOREF(hLdrMod); NOREF(uSymbol);
    return VINF_SUCCESS;
}


/** Argument package for supLoadModuleCreateTabsCB. */
typedef struct SUPLDRCREATETABSARGS
{
    size_t          cbImage;
    PSUPLDRSYM      pSym;
    char           *pszBase;
    char           *psz;
} SUPLDRCREATETABSARGS, *PSUPLDRCREATETABSARGS;

/**
 * Callback used to calculate the image size.
 * @return VINF_SUCCESS
 */
static DECLCALLBACK(int) supLoadModuleCreateTabsCB(RTLDRMOD hLdrMod, const char *pszSymbol, unsigned uSymbol, RTUINTPTR Value, void *pvUser)
{
    PSUPLDRCREATETABSARGS pArgs = (PSUPLDRCREATETABSARGS)pvUser;
    if (    pszSymbol != NULL
        &&  *pszSymbol
        &&  Value <= pArgs->cbImage)
    {
        pArgs->pSym->offSymbol = (uint32_t)Value;
        pArgs->pSym->offName = pArgs->psz - pArgs->pszBase;
        pArgs->pSym++;

        size_t cbCopy = strlen(pszSymbol) + 1;
        memcpy(pArgs->psz, pszSymbol, cbCopy);
        pArgs->psz += cbCopy;
    }
    NOREF(hLdrMod); NOREF(uSymbol);
    return VINF_SUCCESS;
}


/**
 * Worker for SUPR3LoadModule().
 *
 * @returns VBox status code.
 * @param   pszFilename         Name of the VMMR0 image file
 * @param   pszModule           The modulen name.
 * @param   pszSrvReqHandler    The service request handler symbol name,
 *                              optional.
 * @param   pErrInfo            Where to store detailed error info. Optional.
 * @param   ppvImageBase        Where to return the load address.
 */
static int supLoadModule(const char *pszFilename, const char *pszModule, const char *pszSrvReqHandler,
                         PRTERRINFO pErrInfo, void **ppvImageBase)
{
    int rc;

    /*
     * Validate input.
     */
    AssertPtrReturn(pszFilename, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszModule, VERR_INVALID_PARAMETER);
    AssertPtrReturn(ppvImageBase, VERR_INVALID_PARAMETER);
    AssertReturn(strlen(pszModule) < RT_SIZEOFMEMB(SUPLDROPEN, u.In.szName), VERR_FILENAME_TOO_LONG);
    char szAbsFilename[RT_SIZEOFMEMB(SUPLDROPEN, u.In.szFilename)];
    rc = RTPathAbs(pszFilename, szAbsFilename, sizeof(szAbsFilename));
    if (RT_FAILURE(rc))
        return rc;
    pszFilename = szAbsFilename;

    const bool fIsVMMR0 = !strcmp(pszModule, "VMMR0.r0");
    AssertReturn(!pszSrvReqHandler || !fIsVMMR0, VERR_INTERNAL_ERROR);
    *ppvImageBase = NULL;

    /*
     * Open image file and figure its size.
     */
    RTLDRMOD hLdrMod;
    rc = RTLdrOpen(pszFilename, 0, RTLDRARCH_HOST, &hLdrMod);
    if (!RT_SUCCESS(rc))
    {
        LogRel(("SUP: RTLdrOpen failed for %s (%s) %Rrc\n", pszModule, pszFilename, rc));
        return rc;
    }

    SUPLDRCALCSIZEARGS CalcArgs;
    CalcArgs.cbStrings = 0;
    CalcArgs.cSymbols = 0;
    CalcArgs.cbImage = RTLdrSize(hLdrMod);
    rc = RTLdrEnumSymbols(hLdrMod, 0, NULL, 0, supLoadModuleCalcSizeCB, &CalcArgs);
    if (RT_SUCCESS(rc))
    {
        const uint32_t  offSymTab = RT_ALIGN_32(CalcArgs.cbImage, 8);
        const uint32_t  offStrTab = offSymTab + CalcArgs.cSymbols * sizeof(SUPLDRSYM);
        const uint32_t  cbImageWithTabs = RT_ALIGN_32(offStrTab + CalcArgs.cbStrings, 8);

        /*
         * Open the R0 image.
         */
        SUPLDROPEN OpenReq;
        OpenReq.Hdr.u32Cookie = g_u32Cookie;
        OpenReq.Hdr.u32SessionCookie = g_u32SessionCookie;
        OpenReq.Hdr.cbIn = SUP_IOCTL_LDR_OPEN_SIZE_IN;
        OpenReq.Hdr.cbOut = SUP_IOCTL_LDR_OPEN_SIZE_OUT;
        OpenReq.Hdr.fFlags = SUPREQHDR_FLAGS_DEFAULT;
        OpenReq.Hdr.rc = VERR_INTERNAL_ERROR;
        OpenReq.u.In.cbImageWithTabs = cbImageWithTabs;
        OpenReq.u.In.cbImageBits = (uint32_t)CalcArgs.cbImage;
        strcpy(OpenReq.u.In.szName, pszModule);
        strcpy(OpenReq.u.In.szFilename, pszFilename);
        if (!g_uSupFakeMode)
        {
            rc = suplibOsIOCtl(&g_supLibData, SUP_IOCTL_LDR_OPEN, &OpenReq, SUP_IOCTL_LDR_OPEN_SIZE);
            if (RT_SUCCESS(rc))
                rc = OpenReq.Hdr.rc;
        }
        else
        {
            OpenReq.u.Out.fNeedsLoading = true;
            OpenReq.u.Out.pvImageBase = 0xef423420;
        }
        *ppvImageBase = (void *)OpenReq.u.Out.pvImageBase;
        if (    RT_SUCCESS(rc)
            &&  OpenReq.u.Out.fNeedsLoading)
        {
            /*
             * We need to load it.
             * Allocate memory for the image bits.
             */
            PSUPLDRLOAD pLoadReq = (PSUPLDRLOAD)RTMemTmpAlloc(SUP_IOCTL_LDR_LOAD_SIZE(cbImageWithTabs));
            if (pLoadReq)
            {
                /*
                 * Get the image bits.
                 */

                SUPLDRRESIMPARGS Args = { pszModule, pErrInfo };
                rc = RTLdrGetBits(hLdrMod, &pLoadReq->u.In.abImage[0], (uintptr_t)OpenReq.u.Out.pvImageBase,
                                  supLoadModuleResolveImport, &Args);

                if (RT_SUCCESS(rc))
                {
                    /*
                     * Get the entry points.
                     */
                    RTUINTPTR VMMR0EntryFast = 0;
                    RTUINTPTR VMMR0EntryEx = 0;
                    RTUINTPTR SrvReqHandler = 0;
                    RTUINTPTR ModuleInit = 0;
                    RTUINTPTR ModuleTerm = 0;
                    const char *pszEp = NULL;
                    if (fIsVMMR0)
                    {
                        rc = RTLdrGetSymbolEx(hLdrMod, &pLoadReq->u.In.abImage[0], (uintptr_t)OpenReq.u.Out.pvImageBase,
                                              UINT32_MAX, pszEp = "VMMR0EntryFast", &VMMR0EntryFast);
                        if (RT_SUCCESS(rc))
                            rc = RTLdrGetSymbolEx(hLdrMod, &pLoadReq->u.In.abImage[0], (uintptr_t)OpenReq.u.Out.pvImageBase,
                                                  UINT32_MAX, pszEp = "VMMR0EntryEx", &VMMR0EntryEx);
                    }
                    else if (pszSrvReqHandler)
                        rc = RTLdrGetSymbolEx(hLdrMod, &pLoadReq->u.In.abImage[0], (uintptr_t)OpenReq.u.Out.pvImageBase,
                                              UINT32_MAX, pszEp = pszSrvReqHandler, &SrvReqHandler);
                    if (RT_SUCCESS(rc))
                    {
                        int rc2 = RTLdrGetSymbolEx(hLdrMod, &pLoadReq->u.In.abImage[0], (uintptr_t)OpenReq.u.Out.pvImageBase,
                                                   UINT32_MAX, pszEp = "ModuleInit", &ModuleInit);
                        if (RT_FAILURE(rc2))
                            ModuleInit = 0;

                        rc2 = RTLdrGetSymbolEx(hLdrMod, &pLoadReq->u.In.abImage[0], (uintptr_t)OpenReq.u.Out.pvImageBase,
                                               UINT32_MAX, pszEp = "ModuleTerm", &ModuleTerm);
                        if (RT_FAILURE(rc2))
                            ModuleTerm = 0;
                    }
                    if (RT_SUCCESS(rc))
                    {
                        /*
                         * Create the symbol and string tables.
                         */
                        SUPLDRCREATETABSARGS CreateArgs;
                        CreateArgs.cbImage = CalcArgs.cbImage;
                        CreateArgs.pSym    = (PSUPLDRSYM)&pLoadReq->u.In.abImage[offSymTab];
                        CreateArgs.pszBase =     (char *)&pLoadReq->u.In.abImage[offStrTab];
                        CreateArgs.psz     = CreateArgs.pszBase;
                        rc = RTLdrEnumSymbols(hLdrMod, 0, NULL, 0, supLoadModuleCreateTabsCB, &CreateArgs);
                        if (RT_SUCCESS(rc))
                        {
                            AssertRelease((size_t)(CreateArgs.psz - CreateArgs.pszBase) <= CalcArgs.cbStrings);
                            AssertRelease((size_t)(CreateArgs.pSym - (PSUPLDRSYM)&pLoadReq->u.In.abImage[offSymTab]) <= CalcArgs.cSymbols);

                            /*
                             * Upload the image.
                             */
                            pLoadReq->Hdr.u32Cookie = g_u32Cookie;
                            pLoadReq->Hdr.u32SessionCookie = g_u32SessionCookie;
                            pLoadReq->Hdr.cbIn = SUP_IOCTL_LDR_LOAD_SIZE_IN(cbImageWithTabs);
                            pLoadReq->Hdr.cbOut = SUP_IOCTL_LDR_LOAD_SIZE_OUT;
                            pLoadReq->Hdr.fFlags = SUPREQHDR_FLAGS_MAGIC | SUPREQHDR_FLAGS_EXTRA_IN;
                            pLoadReq->Hdr.rc = VERR_INTERNAL_ERROR;

                            pLoadReq->u.In.pfnModuleInit              = (RTR0PTR)ModuleInit;
                            pLoadReq->u.In.pfnModuleTerm              = (RTR0PTR)ModuleTerm;
                            if (fIsVMMR0)
                            {
                                pLoadReq->u.In.eEPType                = SUPLDRLOADEP_VMMR0;
                                pLoadReq->u.In.EP.VMMR0.pvVMMR0       = OpenReq.u.Out.pvImageBase;
                                pLoadReq->u.In.EP.VMMR0.pvVMMR0EntryFast= (RTR0PTR)VMMR0EntryFast;
                                pLoadReq->u.In.EP.VMMR0.pvVMMR0EntryEx  = (RTR0PTR)VMMR0EntryEx;
                            }
                            else if (pszSrvReqHandler)
                            {
                                pLoadReq->u.In.eEPType                = SUPLDRLOADEP_SERVICE;
                                pLoadReq->u.In.EP.Service.pfnServiceReq = (RTR0PTR)SrvReqHandler;
                                pLoadReq->u.In.EP.Service.apvReserved[0] = NIL_RTR0PTR;
                                pLoadReq->u.In.EP.Service.apvReserved[1] = NIL_RTR0PTR;
                                pLoadReq->u.In.EP.Service.apvReserved[2] = NIL_RTR0PTR;
                            }
                            else
                                pLoadReq->u.In.eEPType                = SUPLDRLOADEP_NOTHING;
                            pLoadReq->u.In.offStrTab                  = offStrTab;
                            pLoadReq->u.In.cbStrTab                   = (uint32_t)CalcArgs.cbStrings;
                            AssertRelease(pLoadReq->u.In.cbStrTab == CalcArgs.cbStrings);
                            pLoadReq->u.In.cbImageBits                = (uint32_t)CalcArgs.cbImage;
                            pLoadReq->u.In.offSymbols                 = offSymTab;
                            pLoadReq->u.In.cSymbols                   = CalcArgs.cSymbols;
                            pLoadReq->u.In.cbImageWithTabs            = cbImageWithTabs;
                            pLoadReq->u.In.pvImageBase                = OpenReq.u.Out.pvImageBase;
                            if (!g_uSupFakeMode)
                            {
                                rc = suplibOsIOCtl(&g_supLibData, SUP_IOCTL_LDR_LOAD, pLoadReq, SUP_IOCTL_LDR_LOAD_SIZE(cbImageWithTabs));
                                if (RT_SUCCESS(rc))
                                    rc = pLoadReq->Hdr.rc;
                                else
                                    LogRel(("SUP: SUP_IOCTL_LDR_LOAD ioctl for %s (%s) failed rc=%Rrc\n", pszModule, pszFilename, rc));
                            }
                            else
                                rc = VINF_SUCCESS;
                            if (    RT_SUCCESS(rc)
                                ||  rc == VERR_ALREADY_LOADED /* A competing process. */
                               )
                            {
                                LogRel(("SUP: Loaded %s (%s) at %#RKv - ModuleInit at %RKv and ModuleTerm at %RKv%s\n",
                                        pszModule, pszFilename, OpenReq.u.Out.pvImageBase, (RTR0PTR)ModuleInit, (RTR0PTR)ModuleTerm,
                                        OpenReq.u.Out.fNativeLoader ? " using the native ring-0 loader" : ""));
                                if (fIsVMMR0)
                                {
                                    g_pvVMMR0 = OpenReq.u.Out.pvImageBase;
                                    LogRel(("SUP: VMMR0EntryEx located at %RKv and VMMR0EntryFast at %RKv\n", (RTR0PTR)VMMR0EntryEx, (RTR0PTR)VMMR0EntryFast));
                                }
#ifdef RT_OS_WINDOWS
                                LogRel(("SUP: windbg> .reload /f %s=%#RKv\n", pszFilename, OpenReq.u.Out.pvImageBase));
#endif

                                RTMemTmpFree(pLoadReq);
                                RTLdrClose(hLdrMod);
                                return VINF_SUCCESS;
                            }

                            /*
                             * Failed, bail out.
                             */
                            LogRel(("SUP: Loading failed for %s (%s) rc=%Rrc\n", pszModule, pszFilename, rc));
                            if (   pLoadReq->u.Out.uErrorMagic == SUPLDRLOAD_ERROR_MAGIC
                                && pLoadReq->u.Out.szError[0] != '\0')
                            {
                                LogRel(("SUP: %s\n", pLoadReq->u.Out.szError));
                                RTErrInfoSet(pErrInfo, rc, pLoadReq->u.Out.szError);
                            }
                            else
                                RTErrInfoSet(pErrInfo, rc, "SUP_IOCTL_LDR_LOAD failed");
                        }
                        else
                        {
                            LogRel(("SUP: RTLdrEnumSymbols failed for %s (%s) rc=%Rrc\n", pszModule, pszFilename, rc));
                            RTErrInfoSetF(pErrInfo, rc, "RTLdrEnumSymbols #2 failed");
                        }
                    }
                    else
                    {
                        LogRel(("SUP: Failed to get entry point '%s' for %s (%s) rc=%Rrc\n", pszEp, pszModule, pszFilename, rc));
                        RTErrInfoSetF(pErrInfo, rc, "Failed to resolve entry point '%s'", pszEp);
                    }
                }
                else
                {
                    LogRel(("SUP: RTLdrGetBits failed for %s (%s). rc=%Rrc\n", pszModule, pszFilename, rc));
                    if (!RTErrInfoIsSet(pErrInfo))
                        RTErrInfoSetF(pErrInfo, rc, "RTLdrGetBits failed");
                }
                RTMemTmpFree(pLoadReq);
            }
            else
            {
                AssertMsgFailed(("failed to allocated %u bytes for SUPLDRLOAD_IN structure!\n", SUP_IOCTL_LDR_LOAD_SIZE(cbImageWithTabs)));
                rc = VERR_NO_TMP_MEMORY;
                RTErrInfoSetF(pErrInfo, rc, "Failed to allocate %u bytes for the load request", SUP_IOCTL_LDR_LOAD_SIZE(cbImageWithTabs));
            }
        }
        /*
         * Already loaded?
         */
        else if (RT_SUCCESS(rc))
        {
            if (fIsVMMR0)
                g_pvVMMR0 = OpenReq.u.Out.pvImageBase;
            LogRel(("SUP: Opened %s (%s) at %#RKv%s.\n", pszModule, pszFilename, OpenReq.u.Out.pvImageBase,
                    OpenReq.u.Out.fNativeLoader ? " loaded by the native ring-0 loader" : ""));
#ifdef RT_OS_WINDOWS
            LogRel(("SUP: windbg> .reload /f %s=%#RKv\n", pszFilename, OpenReq.u.Out.pvImageBase));
#endif
        }
        /*
         * No, failed.
         */
        else
            RTErrInfoSet(pErrInfo, rc, "SUP_IOCTL_LDR_OPEN failed");
    }
    else
        RTErrInfoSetF(pErrInfo, rc, "RTLdrEnumSymbols #1 failed");
    RTLdrClose(hLdrMod);
    return rc;
}


SUPR3DECL(int) SUPR3FreeModule(void *pvImageBase)
{
    /* fake */
    if (RT_UNLIKELY(g_uSupFakeMode))
    {
        g_pvVMMR0 = NIL_RTR0PTR;
        return VINF_SUCCESS;
    }

    /*
     * Free the requested module.
     */
    SUPLDRFREE Req;
    Req.Hdr.u32Cookie = g_u32Cookie;
    Req.Hdr.u32SessionCookie = g_u32SessionCookie;
    Req.Hdr.cbIn = SUP_IOCTL_LDR_FREE_SIZE_IN;
    Req.Hdr.cbOut = SUP_IOCTL_LDR_FREE_SIZE_OUT;
    Req.Hdr.fFlags = SUPREQHDR_FLAGS_DEFAULT;
    Req.Hdr.rc = VERR_INTERNAL_ERROR;
    Req.u.In.pvImageBase = (RTR0PTR)pvImageBase;
    int rc = suplibOsIOCtl(&g_supLibData, SUP_IOCTL_LDR_FREE, &Req, SUP_IOCTL_LDR_FREE_SIZE);
    if (RT_SUCCESS(rc))
        rc = Req.Hdr.rc;
    if (    RT_SUCCESS(rc)
        &&  (RTR0PTR)pvImageBase == g_pvVMMR0)
        g_pvVMMR0 = NIL_RTR0PTR;
    return rc;
}


SUPR3DECL(int) SUPR3GetSymbolR0(void *pvImageBase, const char *pszSymbol, void **ppvValue)
{
    *ppvValue = NULL;

    /* fake */
    if (RT_UNLIKELY(g_uSupFakeMode))
    {
        *ppvValue = (void *)(uintptr_t)0xdeadf00d;
        return VINF_SUCCESS;
    }

    /*
     * Do ioctl.
     */
    SUPLDRGETSYMBOL Req;
    Req.Hdr.u32Cookie = g_u32Cookie;
    Req.Hdr.u32SessionCookie = g_u32SessionCookie;
    Req.Hdr.cbIn = SUP_IOCTL_LDR_GET_SYMBOL_SIZE_IN;
    Req.Hdr.cbOut = SUP_IOCTL_LDR_GET_SYMBOL_SIZE_OUT;
    Req.Hdr.fFlags = SUPREQHDR_FLAGS_DEFAULT;
    Req.Hdr.rc = VERR_INTERNAL_ERROR;
    Req.u.In.pvImageBase = (RTR0PTR)pvImageBase;
    size_t cchSymbol = strlen(pszSymbol);
    if (cchSymbol >= sizeof(Req.u.In.szSymbol))
        return VERR_SYMBOL_NOT_FOUND;
    memcpy(Req.u.In.szSymbol, pszSymbol, cchSymbol + 1);
    int rc = suplibOsIOCtl(&g_supLibData, SUP_IOCTL_LDR_GET_SYMBOL, &Req, SUP_IOCTL_LDR_GET_SYMBOL_SIZE);
    if (RT_SUCCESS(rc))
        rc = Req.Hdr.rc;
    if (RT_SUCCESS(rc))
        *ppvValue = (void *)Req.u.Out.pvSymbol;
    return rc;
}


SUPR3DECL(int) SUPR3LoadVMM(const char *pszFilename)
{
    void *pvImageBase;
    return SUPR3LoadModule(pszFilename, "VMMR0.r0", &pvImageBase, NULL /*pErrInfo*/);
}


SUPR3DECL(int) SUPR3UnloadVMM(void)
{
    return SUPR3FreeModule((void*)g_pvVMMR0);
}


/**
 * Worker for SUPR3HardenedLdrLoad and SUPR3HardenedLdrLoadAppPriv.
 *
 * @returns iprt status code.
 * @param   pszFilename     The full file name.
 * @param   phLdrMod        Where to store the handle to the loaded module.
 * @param   fFlags          See RTLDFLAGS_.
 * @param   pErrInfo        Where to return extended error information.
 *                          Optional.
 *
 */
static int supR3HardenedLdrLoadIt(const char *pszFilename, PRTLDRMOD phLdrMod, uint32_t fFlags, PRTERRINFO pErrInfo)
{
#ifdef VBOX_WITH_HARDENING
    /*
     * Verify the image file.
     */
    int rc = SUPR3HardenedVerifyInit();
    if (RT_FAILURE(rc))
        rc = supR3HardenedVerifyFixedFile(pszFilename, false /* fFatal */);
    if (RT_FAILURE(rc))
    {
        LogRel(("supR3HardenedLdrLoadIt: Verification of \"%s\" failed, rc=%Rrc\n", pszFilename, rc));
        return RTErrInfoSet(pErrInfo, rc, "supR3HardenedVerifyFixedFile failed");
    }
#endif

    /*
     * Try load it.
     */
    return RTLdrLoadEx(pszFilename, phLdrMod, fFlags, pErrInfo);
}


SUPR3DECL(int) SUPR3HardenedLdrLoad(const char *pszFilename, PRTLDRMOD phLdrMod, uint32_t fFlags, PRTERRINFO pErrInfo)
{
    /*
     * Validate input.
     */
    RTErrInfoClear(pErrInfo);
    AssertPtrReturn(pszFilename, VERR_INVALID_POINTER);
    AssertPtrReturn(phLdrMod, VERR_INVALID_POINTER);
    *phLdrMod = NIL_RTLDRMOD;
    AssertReturn(RTPathHavePath(pszFilename), VERR_INVALID_PARAMETER);

    /*
     * Add the default extension if it's missing.
     */
    if (!RTPathHasSuffix(pszFilename))
    {
        const char *pszSuff = RTLdrGetSuff();
        size_t      cchSuff = strlen(pszSuff);
        size_t      cchFilename = strlen(pszFilename);
        char       *psz = (char *)alloca(cchFilename + cchSuff + 1);
        AssertReturn(psz, VERR_NO_TMP_MEMORY);
        memcpy(psz, pszFilename, cchFilename);
        memcpy(psz + cchFilename, pszSuff, cchSuff + 1);
        pszFilename = psz;
    }

    /*
     * Pass it on to the common library loader.
     */
    return supR3HardenedLdrLoadIt(pszFilename, phLdrMod, fFlags, pErrInfo);
}


SUPR3DECL(int) SUPR3HardenedLdrLoadAppPriv(const char *pszFilename, PRTLDRMOD phLdrMod, uint32_t fFlags, PRTERRINFO pErrInfo)
{
    LogFlow(("SUPR3HardenedLdrLoadAppPriv: pszFilename=%p:{%s} phLdrMod=%p fFlags=%08x pErrInfo=%p\n", pszFilename, pszFilename, phLdrMod, fFlags, pErrInfo));

    /*
     * Validate input.
     */
    RTErrInfoClear(pErrInfo);
    AssertPtrReturn(phLdrMod, VERR_INVALID_PARAMETER);
    *phLdrMod = NIL_RTLDRMOD;
    AssertPtrReturn(pszFilename, VERR_INVALID_PARAMETER);
    AssertMsgReturn(!RTPathHavePath(pszFilename), ("%s\n", pszFilename), VERR_INVALID_PARAMETER);

    /*
     * Check the filename.
     */
    size_t cchFilename = strlen(pszFilename);
    AssertMsgReturn(cchFilename < (RTPATH_MAX / 4) * 3, ("%zu\n", cchFilename), VERR_INVALID_PARAMETER);

    const char *pszExt = "";
    size_t cchExt = 0;
    if (!RTPathHasSuffix(pszFilename))
    {
        pszExt = RTLdrGetSuff();
        cchExt = strlen(pszExt);
    }

    /*
     * Construct the private arch path and check if the file exists.
     */
    char szPath[RTPATH_MAX];
    int rc = RTPathAppPrivateArch(szPath, sizeof(szPath) - 1 - cchExt - cchFilename);
    AssertRCReturn(rc, rc);

    char *psz = strchr(szPath, '\0');
    *psz++ = RTPATH_SLASH;
    memcpy(psz, pszFilename, cchFilename);
    psz += cchFilename;
    memcpy(psz, pszExt, cchExt + 1);

    if (!RTPathExists(szPath))
    {
        LogRel(("SUPR3HardenedLdrLoadAppPriv: \"%s\" not found\n", szPath));
        return VERR_FILE_NOT_FOUND;
    }

    /*
     * Pass it on to SUPR3HardenedLdrLoad.
     */
    rc = SUPR3HardenedLdrLoad(szPath, phLdrMod, fFlags, pErrInfo);

    LogFlow(("SUPR3HardenedLdrLoadAppPriv: returns %Rrc\n", rc));
    return rc;
}


SUPR3DECL(int) SUPR3HardenedLdrLoadPlugIn(const char *pszFilename, PRTLDRMOD phLdrMod, PRTERRINFO pErrInfo)
{
    /*
     * Validate input.
     */
    RTErrInfoClear(pErrInfo);
    AssertPtrReturn(phLdrMod, VERR_INVALID_PARAMETER);
    *phLdrMod = NIL_RTLDRMOD;
    AssertPtrReturn(pszFilename, VERR_INVALID_PARAMETER);
    AssertReturn(RTPathStartsWithRoot(pszFilename), VERR_INVALID_PARAMETER);

#ifdef VBOX_WITH_HARDENING
    /*
     * Verify the image file.
     */
    int rc = supR3HardenedVerifyFile(pszFilename, RTHCUINTPTR_MAX, true /*fMaybe3rdParty*/, pErrInfo);
    if (RT_FAILURE(rc))
    {
        if (!RTErrInfoIsSet(pErrInfo))
            LogRel(("supR3HardenedVerifyFile: Verification of \"%s\" failed, rc=%Rrc\n", pszFilename, rc));
        return rc;
    }
#endif

    /*
     * Try load it.
     */
    return RTLdrLoadEx(pszFilename, phLdrMod, RTLDRLOAD_FLAGS_LOCAL, pErrInfo);
}

