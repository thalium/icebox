/* $Id: ldrNative-posix.cpp $ */
/** @file
 * IPRT - Binary Image Loader, POSIX native.
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
#define LOG_GROUP RTLOGGROUP_LDR
#include <dlfcn.h>

#include <iprt/ldr.h>
#include <iprt/assert.h>
#include <iprt/path.h>
#include <iprt/alloca.h>
#include <iprt/string.h>
#include <iprt/err.h>
#include <iprt/log.h>
#include "internal/ldr.h"


int rtldrNativeLoad(const char *pszFilename, uintptr_t *phHandle, uint32_t fFlags, PRTERRINFO pErrInfo)
{
    /*
     * Do we need to add an extension?
     */
    if (!RTPathHasSuffix(pszFilename))
    {
#if defined(RT_OS_OS2) || defined(RT_OS_WINDOWS)
        static const char s_szSuff[] = ".DLL";
#elif defined(RT_OS_L4)
        static const char s_szSuff[] = ".s.so";
#elif defined(RT_OS_DARWIN)
        static const char s_szSuff[] = ".dylib";
#else
        static const char s_szSuff[] = ".so";
#endif
        size_t cch = strlen(pszFilename);
        char *psz = (char *)alloca(cch + sizeof(s_szSuff));
        if (!psz)
            return RTErrInfoSet(pErrInfo, VERR_NO_MEMORY, "alloca failed");
        memcpy(psz, pszFilename, cch);
        memcpy(psz + cch, s_szSuff, sizeof(s_szSuff));
        pszFilename = psz;
    }

    /*
     * Attempt load.
     */
    int fFlagsNative = RTLD_NOW;
    if (fFlags & RTLDRLOAD_FLAGS_GLOBAL)
        fFlagsNative |= RTLD_GLOBAL;
    else
        fFlagsNative |= RTLD_LOCAL;
    void *pvMod = dlopen(pszFilename, fFlagsNative);
    if (pvMod)
    {
        *phHandle = (uintptr_t)pvMod;
        return VINF_SUCCESS;
    }

    const char *pszDlError = dlerror();
    RTErrInfoSet(pErrInfo, VERR_FILE_NOT_FOUND, pszDlError);
    LogRel(("rtldrNativeLoad: dlopen('%s', RTLD_NOW | RTLD_LOCAL) failed: %s\n", pszFilename, pszDlError));
    return VERR_FILE_NOT_FOUND;
}


DECLCALLBACK(int) rtldrNativeGetSymbol(PRTLDRMODINTERNAL pMod, const char *pszSymbol, void **ppvValue)
{
    PRTLDRMODNATIVE pModNative = (PRTLDRMODNATIVE)pMod;
#ifdef RT_OS_OS2
    /* Prefix the symbol with an underscore (assuming __cdecl/gcc-default). */
    size_t cch = strlen(pszSymbol);
    char *psz = (char *)alloca(cch + 2);
    psz[0] = '_';
    memcpy(psz + 1, pszSymbol, cch + 1);
    pszSymbol = psz;
#endif
    *ppvValue = dlsym((void *)pModNative->hNative, pszSymbol);
    if (*ppvValue)
        return VINF_SUCCESS;
    return VERR_SYMBOL_NOT_FOUND;
}


DECLCALLBACK(int) rtldrNativeClose(PRTLDRMODINTERNAL pMod)
{
    PRTLDRMODNATIVE pModNative = (PRTLDRMODNATIVE)pMod;
#ifdef __SANITIZE_ADDRESS__
    /* If we are compiled with enabled address sanitizer (gcc/llvm), don't
     * unload the module to prevent <unknown module> in the stack trace */
    pModNative->fFlags |= RTLDRLOAD_FLAGS_NO_UNLOAD;
#endif
    if (   (pModNative->fFlags & RTLDRLOAD_FLAGS_NO_UNLOAD)
        || !dlclose((void *)pModNative->hNative))
    {
        pModNative->hNative = (uintptr_t)0;
        return VINF_SUCCESS;
    }
    Log(("rtldrNativeFree: dlclose(%p) failed: %s\n", pModNative->hNative, dlerror()));
    return VERR_GENERAL_FAILURE;
}


int rtldrNativeLoadSystem(const char *pszFilename, const char *pszExt, uint32_t fFlags, PRTLDRMOD phLdrMod)
{
    RT_NOREF_PV(pszFilename); RT_NOREF_PV(pszExt); RT_NOREF_PV(fFlags); RT_NOREF_PV(phLdrMod);
    /** @todo implement this in some sensible fashion. */
    return VERR_NOT_SUPPORTED;
}

