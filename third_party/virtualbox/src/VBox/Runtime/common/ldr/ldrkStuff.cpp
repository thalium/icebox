/* $Id: ldrkStuff.cpp $ */
/** @file
 * IPRT - Binary Image Loader, kLdr Interface.
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
#include <iprt/ldr.h>
#include "internal/iprt.h"

#include <iprt/file.h>
#include <iprt/alloc.h>
#include <iprt/alloca.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/path.h>
#include <iprt/log.h>
#include <iprt/param.h>
#include <iprt/err.h>
#include "internal/ldr.h"
#define KLDR_ALREADY_INCLUDE_STD_TYPES
#define KLDR_NO_KLDR_H_INCLUDES
#include <k/kLdr.h>
#include <k/kRdrAll.h>
#include <k/kErr.h>
#include <k/kErrors.h>
#include <k/kMagics.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * kLdr file provider.
 */
typedef struct RTKLDRRDR
{
    /** The core. */
    KRDR            Core;
    /** The IPRT bit reader. */
    PRTLDRREADER    pReader;
} RTKLDRRDR, *PRTKLDRRDR;

/**
 * IPRT module.
 */
typedef struct RTLDRMODKLDR
{
    /** The Core module structure. */
    RTLDRMODINTERNAL    Core;
    /** The kLdr module. */
    PKLDRMOD            pMod;
} RTLDRMODKLDR, *PRTLDRMODKLDR;


/**
 * Arguments for a RTLDRMODKLDR callback wrapper.
 */
typedef struct RTLDRMODKLDRARGS
{
    union
    {
        PFNRT            pfn;
        PFNRTLDRENUMDBG  pfnEnumDbgInfo;
        PFNRTLDRENUMSYMS pfnEnumSyms;
        PFNRTLDRIMPORT   pfnGetImport;
    }                   u;
    void               *pvUser;
    const void         *pvBits;
    PRTLDRMODKLDR       pMod;
    int                 rc;
} RTLDRMODKLDRARGS, *PRTLDRMODKLDRARGS;


/**
 * Converts a kLdr error code to an IPRT one.
 */
static int rtkldrConvertError(int krc)
{
    if (!krc)
        return VINF_SUCCESS;
    switch (krc)
    {
        case KERR_INVALID_PARAMETER:                        return VERR_INVALID_PARAMETER;
        case KERR_INVALID_HANDLE:                           return VERR_INVALID_HANDLE;
        case KERR_NO_MEMORY:                                return VERR_NO_MEMORY;
        case KLDR_ERR_CPU_ARCH_MISMATCH:                    return VERR_LDR_ARCH_MISMATCH;

        case KLDR_ERR_UNKNOWN_FORMAT:
        case KLDR_ERR_MZ_NOT_SUPPORTED:                     return VERR_MZ_EXE_NOT_SUPPORTED;
        case KLDR_ERR_NE_NOT_SUPPORTED:                     return VERR_NE_EXE_NOT_SUPPORTED;
        case KLDR_ERR_LX_NOT_SUPPORTED:                     return VERR_LX_EXE_NOT_SUPPORTED;
        case KLDR_ERR_LE_NOT_SUPPORTED:                     return VERR_LE_EXE_NOT_SUPPORTED;
        case KLDR_ERR_PE_NOT_SUPPORTED:                     return VERR_PE_EXE_NOT_SUPPORTED;
        case KLDR_ERR_ELF_NOT_SUPPORTED:                    return VERR_ELF_EXE_NOT_SUPPORTED;
        case KLDR_ERR_MACHO_NOT_SUPPORTED:                  return VERR_INVALID_EXE_SIGNATURE;
        case KLDR_ERR_AOUT_NOT_SUPPORTED:                   return VERR_AOUT_EXE_NOT_SUPPORTED;

        case KLDR_ERR_MODULE_NOT_FOUND:                     return VERR_MODULE_NOT_FOUND;
        case KLDR_ERR_PREREQUISITE_MODULE_NOT_FOUND:        return VERR_MODULE_NOT_FOUND;
        case KLDR_ERR_MAIN_STACK_ALLOC_FAILED:              return VERR_NO_MEMORY;
        case KERR_BUFFER_OVERFLOW:                          return VERR_BUFFER_OVERFLOW;
        case KLDR_ERR_SYMBOL_NOT_FOUND:                     return VERR_SYMBOL_NOT_FOUND;
        case KLDR_ERR_FORWARDER_SYMBOL:                     return VERR_BAD_EXE_FORMAT;
        case KLDR_ERR_BAD_FIXUP:                            AssertMsgFailedReturn(("KLDR_ERR_BAD_FIXUP\n"), VERR_BAD_EXE_FORMAT);
        case KLDR_ERR_IMPORT_ORDINAL_OUT_OF_BOUNDS:         return VERR_BAD_EXE_FORMAT;
        case KLDR_ERR_NO_DEBUG_INFO:                        return VERR_FILE_NOT_FOUND;
        case KLDR_ERR_ALREADY_MAPPED:                       return VERR_WRONG_ORDER;
        case KLDR_ERR_NOT_MAPPED:                           return VERR_WRONG_ORDER;
        case KLDR_ERR_ADDRESS_OVERFLOW:                     return VERR_NUMBER_TOO_BIG;
        case KLDR_ERR_TODO:                                 return VERR_NOT_IMPLEMENTED;

        case KLDR_ERR_NOT_LOADED_DYNAMICALLY:
        case KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE:
        case KLDR_ERR_TOO_LONG_FORWARDER_CHAIN:
        case KLDR_ERR_MODULE_TERMINATING:
        case KLDR_ERR_PREREQUISITE_MODULE_TERMINATING:
        case KLDR_ERR_MODULE_INIT_FAILED:
        case KLDR_ERR_PREREQUISITE_MODULE_INIT_FAILED:
        case KLDR_ERR_MODULE_INIT_FAILED_ALREADY:
        case KLDR_ERR_PREREQUISITE_MODULE_INIT_FAILED_ALREADY:
        case KLDR_ERR_PREREQUISITE_RECURSED_TOO_DEEPLY:
        case KLDR_ERR_THREAD_ATTACH_FAILED:
        case KRDR_ERR_TOO_MANY_MAPPINGS:
        case KLDR_ERR_NOT_DLL:
        case KLDR_ERR_NOT_EXE:
            AssertMsgFailedReturn(("krc=%d (%#x): %s\n", krc, krc, kErrName(krc)), VERR_LDR_GENERAL_FAILURE);


        case KLDR_ERR_PE_UNSUPPORTED_MACHINE:
        case KLDR_ERR_PE_BAD_FILE_HEADER:
        case KLDR_ERR_PE_BAD_OPTIONAL_HEADER:
        case KLDR_ERR_PE_BAD_SECTION_HEADER:
        case KLDR_ERR_PE_BAD_FORWARDER:
        case KLDR_ERR_PE_FORWARDER_IMPORT_NOT_FOUND:
        case KLDR_ERR_PE_BAD_FIXUP:
        case KLDR_ERR_PE_BAD_IMPORT:
            AssertMsgFailedReturn(("krc=%d (%#x): %s\n", krc, krc, kErrName(krc)), VERR_LDR_GENERAL_FAILURE);

        case KLDR_ERR_LX_BAD_HEADER:
        case KLDR_ERR_LX_BAD_LOADER_SECTION:
        case KLDR_ERR_LX_BAD_FIXUP_SECTION:
        case KLDR_ERR_LX_BAD_OBJECT_TABLE:
        case KLDR_ERR_LX_BAD_PAGE_MAP:
        case KLDR_ERR_LX_BAD_ITERDATA:
        case KLDR_ERR_LX_BAD_ITERDATA2:
        case KLDR_ERR_LX_BAD_BUNDLE:
        case KLDR_ERR_LX_NO_SONAME:
        case KLDR_ERR_LX_BAD_SONAME:
        case KLDR_ERR_LX_BAD_FORWARDER:
        case KLDR_ERR_LX_NRICHAIN_NOT_SUPPORTED:
            AssertMsgFailedReturn(("krc=%d (%#x): %s\n", krc, krc, kErrName(krc)), VERR_LDR_GENERAL_FAILURE);

        case KLDR_ERR_MACHO_UNSUPPORTED_FILE_TYPE:          return VERR_LDR_GENERAL_FAILURE;
        case KLDR_ERR_MACHO_OTHER_ENDIAN_NOT_SUPPORTED:
        case KLDR_ERR_MACHO_BAD_HEADER:
        case KLDR_ERR_MACHO_UNSUPPORTED_MACHINE:
        case KLDR_ERR_MACHO_BAD_LOAD_COMMAND:
        case KLDR_ERR_MACHO_UNKNOWN_LOAD_COMMAND:
        case KLDR_ERR_MACHO_UNSUPPORTED_LOAD_COMMAND:
        case KLDR_ERR_MACHO_BAD_SECTION:
        case KLDR_ERR_MACHO_UNSUPPORTED_SECTION:
#ifdef KLDR_ERR_MACHO_UNSUPPORTED_INIT_SECTION
        case KLDR_ERR_MACHO_UNSUPPORTED_INIT_SECTION:
        case KLDR_ERR_MACHO_UNSUPPORTED_TERM_SECTION:
#endif
        case KLDR_ERR_MACHO_UNKNOWN_SECTION:
        case KLDR_ERR_MACHO_BAD_SECTION_ORDER:
        case KLDR_ERR_MACHO_BIT_MIX:
        case KLDR_ERR_MACHO_BAD_OBJECT_FILE:
        case KLDR_ERR_MACHO_BAD_SYMBOL:
        case KLDR_ERR_MACHO_UNSUPPORTED_FIXUP_TYPE:
            AssertMsgFailedReturn(("krc=%d (%#x): %s\n", krc, krc, kErrName(krc)), VERR_LDR_GENERAL_FAILURE);

        default:
            if (RT_FAILURE(krc))
                return krc;
            AssertMsgFailedReturn(("krc=%d (%#x): %s\n", krc, krc, kErrName(krc)), VERR_NO_TRANSLATION);
    }
}


/**
 * Converts a IPRT error code to an kLdr one.
 */
static int rtkldrConvertErrorFromIPRT(int rc)
{
    if (RT_SUCCESS(rc))
        return 0;
    switch (rc)
    {
        case VERR_NO_MEMORY:            return KERR_NO_MEMORY;
        case VERR_INVALID_PARAMETER:    return KERR_INVALID_PARAMETER;
        case VERR_INVALID_HANDLE:       return KERR_INVALID_HANDLE;
        case VERR_BUFFER_OVERFLOW:      return KERR_BUFFER_OVERFLOW;
        default:
            return rc;
    }
}






/** @interface_method_impl{KLDRRDROPS,pfnCreate}
 * @remark This is a dummy which isn't used. */
static int      rtkldrRdr_Create(  PPKRDR ppRdr, const char *pszFilename)
{
    NOREF(ppRdr); NOREF(pszFilename);
    AssertReleaseFailed();
    return -1;
}


/** @interface_method_impl{KLDRRDROPS,pfnDestroy} */
static int      rtkldrRdr_Destroy( PKRDR pRdr)
{
    PRTKLDRRDR pThis = (PRTKLDRRDR)pRdr;
    pThis->pReader = NULL;
    RTMemFree(pThis);
    return 0;
}


/** @interface_method_impl{KLDRRDROPS,pfnRead} */
static int      rtkldrRdr_Read(    PKRDR pRdr, void *pvBuf, KSIZE cb, KFOFF off)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    int rc = pReader->pfnRead(pReader, pvBuf, cb, off);
    return rtkldrConvertErrorFromIPRT(rc);
}


/** @interface_method_impl{KLDRRDROPS,pfnAllMap} */
static int      rtkldrRdr_AllMap(  PKRDR pRdr, const void **ppvBits)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    int rc = pReader->pfnMap(pReader, ppvBits);
    return rtkldrConvertErrorFromIPRT(rc);
}


/** @interface_method_impl{KLDRRDROPS,pfnAllUnmap} */
static int      rtkldrRdr_AllUnmap(PKRDR pRdr, const void *pvBits)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    int rc = pReader->pfnUnmap(pReader, pvBits);
    return rtkldrConvertErrorFromIPRT(rc);
}


/** @interface_method_impl{KLDRRDROPS,pfnSize} */
static KFOFF rtkldrRdr_Size(    PKRDR pRdr)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    return (KFOFF)pReader->pfnSize(pReader);
}


/** @interface_method_impl{KLDRRDROPS,pfnTell} */
static KFOFF rtkldrRdr_Tell(    PKRDR pRdr)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    return (KFOFF)pReader->pfnTell(pReader);
}


/** @interface_method_impl{KLDRRDROPS,pfnName} */
static const char * rtkldrRdr_Name(PKRDR pRdr)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    return pReader->pfnLogName(pReader);
}


/** @interface_method_impl{KLDRRDROPS,pfnNativeFH} */
static KIPTR rtkldrRdr_NativeFH(PKRDR pRdr)
{
    NOREF(pRdr);
    AssertFailed();
    return -1;
}


/** @interface_method_impl{KLDRRDROPS,pfnPageSize} */
static KSIZE rtkldrRdr_PageSize(PKRDR pRdr)
{
    NOREF(pRdr);
    return PAGE_SIZE;
}


/** @interface_method_impl{KLDRRDROPS,pfnMap} */
static int      rtkldrRdr_Map(     PKRDR pRdr, void **ppvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed)
{
    //PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    NOREF(pRdr); NOREF(ppvBase); NOREF(cSegments); NOREF(paSegments); NOREF(fFixed);
    AssertFailed();
    return -1;
}


/** @interface_method_impl{KLDRRDROPS,pfnRefresh} */
static int      rtkldrRdr_Refresh( PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments)
{
    //PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    NOREF(pRdr); NOREF(pvBase); NOREF(cSegments); NOREF(paSegments);
    AssertFailed();
    return -1;
}


/** @interface_method_impl{KLDRRDROPS,pfnProtect} */
static int      rtkldrRdr_Protect( PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect)
{
    //PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    NOREF(pRdr); NOREF(pvBase); NOREF(cSegments); NOREF(paSegments); NOREF(fUnprotectOrProtect);
    AssertFailed();
    return -1;
}


/** @interface_method_impl{KLDRRDROPS,pfnUnmap} */
static int      rtkldrRdr_Unmap(   PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments)
{
    //PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    NOREF(pRdr); NOREF(pvBase); NOREF(cSegments); NOREF(paSegments);
    AssertFailed();
    return -1;
}

/** @interface_method_impl{KLDRRDROPS,pfnDone} */
static void     rtkldrRdr_Done(    PKRDR pRdr)
{
    NOREF(pRdr);
    //PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
}


/**
 * The file reader operations.
 * We provide our own based on IPRT instead of using the kLdr ones.
 */
extern "C" const KRDROPS g_kLdrRdrFileOps;
extern "C" const KRDROPS g_kLdrRdrFileOps =
{
    /* .pszName = */        "IPRT",
    /* .pNext = */          NULL,
    /* .pfnCreate = */      rtkldrRdr_Create,
    /* .pfnDestroy = */     rtkldrRdr_Destroy,
    /* .pfnRead = */        rtkldrRdr_Read,
    /* .pfnAllMap = */      rtkldrRdr_AllMap,
    /* .pfnAllUnmap = */    rtkldrRdr_AllUnmap,
    /* .pfnSize = */        rtkldrRdr_Size,
    /* .pfnTell = */        rtkldrRdr_Tell,
    /* .pfnName = */        rtkldrRdr_Name,
    /* .pfnNativeFH = */    rtkldrRdr_NativeFH,
    /* .pfnPageSize = */    rtkldrRdr_PageSize,
    /* .pfnMap = */         rtkldrRdr_Map,
    /* .pfnRefresh = */     rtkldrRdr_Refresh,
    /* .pfnProtect = */     rtkldrRdr_Protect,
    /* .pfnUnmap =  */      rtkldrRdr_Unmap,
    /* .pfnDone = */        rtkldrRdr_Done,
    /* .u32Dummy = */       42
};





/** @interface_method_impl{RTLDROPS,pfnClose} */
static DECLCALLBACK(int) rtkldr_Close(PRTLDRMODINTERNAL pMod)
{
    PKLDRMOD pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    int rc = kLdrModClose(pModkLdr);
    return rtkldrConvertError(rc);
}


/** @interface_method_impl{RTLDROPS,pfnDone} */
static DECLCALLBACK(int) rtkldr_Done(PRTLDRMODINTERNAL pMod)
{
    PKLDRMOD pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    int rc = kLdrModMostlyDone(pModkLdr);
    return rtkldrConvertError(rc);
}


/** @copydoc FNKLDRMODENUMSYMS */
static int rtkldrEnumSymbolsWrapper(PKLDRMOD pMod, uint32_t iSymbol,
                                    const char *pchSymbol, KSIZE cchSymbol, const char *pszVersion,
                                    KLDRADDR uValue, uint32_t fKind, void *pvUser)
{
    PRTLDRMODKLDRARGS pArgs = (PRTLDRMODKLDRARGS)pvUser;
    NOREF(pMod); NOREF(pszVersion); NOREF(fKind);

    /* If not zero terminated we'll have to use a temporary buffer. */
    const char *pszSymbol = pchSymbol;
    if (pchSymbol && pchSymbol[cchSymbol])
    {
        char *psz = (char *)alloca(cchSymbol + 1);
        memcpy(psz, pchSymbol, cchSymbol);
        psz[cchSymbol] = '\0';
        pszSymbol = psz;
    }

#if defined(RT_OS_OS2) || defined(RT_OS_DARWIN)
    /* skip the underscore prefix. */
    if (*pszSymbol == '_')
        pszSymbol++;
#endif

    int rc = pArgs->u.pfnEnumSyms(&pArgs->pMod->Core, pszSymbol, iSymbol, uValue, pArgs->pvUser);
    if (RT_FAILURE(rc))
        return rc; /* don't bother converting. */
    return 0;
}


/** @interface_method_impl{RTLDROPS,pfnEnumSymbols} */
static DECLCALLBACK(int) rtkldr_EnumSymbols(PRTLDRMODINTERNAL pMod, unsigned fFlags, const void *pvBits, RTUINTPTR BaseAddress,
                                            PFNRTLDRENUMSYMS pfnCallback, void *pvUser)
{
    PKLDRMOD            pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    RTLDRMODKLDRARGS    Args;
    Args.pvUser         = pvUser;
    Args.u.pfnEnumSyms  = pfnCallback;
    Args.pMod           = (PRTLDRMODKLDR)pMod;
    Args.pvBits         = pvBits;
    Args.rc             = VINF_SUCCESS;
    int rc = kLdrModEnumSymbols(pModkLdr, pvBits, BaseAddress,
                                fFlags & RTLDR_ENUM_SYMBOL_FLAGS_ALL ? KLDRMOD_ENUM_SYMS_FLAGS_ALL : 0,
                                rtkldrEnumSymbolsWrapper, &Args);
    if (Args.rc != VINF_SUCCESS)
        rc = Args.rc;
    else
        rc = rtkldrConvertError(rc);
    return rc;
}


/** @interface_method_impl{RTLDROPS,pfnGetImageSize} */
static DECLCALLBACK(size_t) rtkldr_GetImageSize(PRTLDRMODINTERNAL pMod)
{
    PKLDRMOD pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    return kLdrModSize(pModkLdr);
}


/** @copydoc FNKLDRMODGETIMPORT */
static int rtkldrGetImportWrapper(PKLDRMOD pMod, uint32_t iImport, uint32_t iSymbol, const char *pchSymbol, KSIZE cchSymbol,
                                  const char *pszVersion, PKLDRADDR puValue, uint32_t *pfKind, void *pvUser)
{
    PRTLDRMODKLDRARGS pArgs = (PRTLDRMODKLDRARGS)pvUser;
    NOREF(pMod); NOREF(pszVersion); NOREF(pfKind);

    /* If not zero terminated we'll have to use a temporary buffer. */
    const char *pszSymbol = pchSymbol;
    if (pchSymbol && pchSymbol[cchSymbol])
    {
        char *psz = (char *)alloca(cchSymbol + 1);
        memcpy(psz, pchSymbol, cchSymbol);
        psz[cchSymbol] = '\0';
        pszSymbol = psz;
    }

#if defined(RT_OS_OS2) || defined(RT_OS_DARWIN)
    /* skip the underscore prefix. */
    if (*pszSymbol == '_')
        pszSymbol++;
#endif

    /* get the import module name - TODO: cache this */
    const char *pszModule = NULL;
    if (iImport != NIL_KLDRMOD_IMPORT)
    {
        char *pszBuf = (char *)alloca(64);
        int rc = kLdrModGetImport(pMod, pArgs->pvBits, iImport, pszBuf, 64);
        if (rc)
            return rc;
        pszModule = pszBuf;
    }

    /* do the query */
    RTUINTPTR Value;
    int rc = pArgs->u.pfnGetImport(&pArgs->pMod->Core, pszModule, pszSymbol, pszSymbol ? ~0 : iSymbol, &Value, pArgs->pvUser);
    if (RT_SUCCESS(rc))
    {
        *puValue = Value;
        return 0;
    }
    return rtkldrConvertErrorFromIPRT(rc);
}


/** @interface_method_impl{RTLDROPS,pfnGetBits} */
static DECLCALLBACK(int) rtkldr_GetBits(PRTLDRMODINTERNAL pMod, void *pvBits, RTUINTPTR BaseAddress,
                                        PFNRTLDRIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMOD            pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    RTLDRMODKLDRARGS    Args;
    Args.pvUser         = pvUser;
    Args.u.pfnGetImport = pfnGetImport;
    Args.pMod           = (PRTLDRMODKLDR)pMod;
    Args.pvBits         = pvBits;
    Args.rc             = VINF_SUCCESS;
    int rc = kLdrModGetBits(pModkLdr, pvBits, BaseAddress, rtkldrGetImportWrapper, &Args);
    if (Args.rc != VINF_SUCCESS)
        rc = Args.rc;
    else
        rc = rtkldrConvertError(rc);
    return rc;
}


/** @interface_method_impl{RTLDROPS,pfnRelocate} */
static DECLCALLBACK(int) rtkldr_Relocate(PRTLDRMODINTERNAL pMod, void *pvBits, RTUINTPTR NewBaseAddress,
                                         RTUINTPTR OldBaseAddress, PFNRTLDRIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMOD            pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    RTLDRMODKLDRARGS    Args;
    Args.pvUser         = pvUser;
    Args.u.pfnGetImport = pfnGetImport;
    Args.pMod           = (PRTLDRMODKLDR)pMod;
    Args.pvBits         = pvBits;
    Args.rc             = VINF_SUCCESS;
    int rc = kLdrModRelocateBits(pModkLdr, pvBits, NewBaseAddress, OldBaseAddress, rtkldrGetImportWrapper, &Args);
    if (Args.rc != VINF_SUCCESS)
        rc = Args.rc;
    else
        rc = rtkldrConvertError(rc);
    return rc;
}


/** @interface_method_impl{RTLDROPS,pfnGetSymbolEx} */
static DECLCALLBACK(int) rtkldr_GetSymbolEx(PRTLDRMODINTERNAL pMod, const void *pvBits, RTUINTPTR BaseAddress,
                                            uint32_t iOrdinal, const char *pszSymbol, RTUINTPTR *pValue)
{
    PKLDRMOD pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    KLDRADDR uValue;

#if defined(RT_OS_OS2) || defined(RT_OS_DARWIN)
    /*
     * Add underscore prefix.
     */
    if (pszSymbol)
    {
        size_t cch = strlen(pszSymbol);
        char *psz = (char *)alloca(cch + 2);
        memcpy(psz + 1, pszSymbol, cch + 1);
        *psz = '_';
        pszSymbol = psz;
    }
#endif

    int rc = kLdrModQuerySymbol(pModkLdr, pvBits, BaseAddress,
                                iOrdinal == UINT32_MAX ? NIL_KLDRMOD_SYM_ORDINAL : iOrdinal,
                                pszSymbol, strlen(pszSymbol), NULL,
                                NULL, NULL,
                                &uValue, NULL);
    if (!rc)
    {
        *pValue = uValue;
        return VINF_SUCCESS;
    }
    return rtkldrConvertError(rc);
}


/** @copydoc FNKLDRENUMDBG */
static int rtkldrEnumDbgInfoWrapper(PKLDRMOD pMod, KU32 iDbgInfo, KLDRDBGINFOTYPE enmType, KI16 iMajorVer, KI16 iMinorVer,
                                    const char *pszPartNm, KLDRFOFF offFile, KLDRADDR LinkAddress, KLDRSIZE cb,
                                    const char *pszExtFile, void *pvUser)
{
    PRTLDRMODKLDRARGS pArgs = (PRTLDRMODKLDRARGS)pvUser;
    RT_NOREF_PV(pMod); RT_NOREF_PV(iMajorVer); RT_NOREF_PV(iMinorVer);


    RTLDRDBGINFO DbgInfo;
    RT_ZERO(DbgInfo.u);
    DbgInfo.iDbgInfo        = iDbgInfo;
    DbgInfo.offFile         = offFile;
    DbgInfo.LinkAddress     = LinkAddress;
    DbgInfo.cb              = cb;
    DbgInfo.pszExtFile      = pszExtFile;

    switch (enmType)
    {
        case KLDRDBGINFOTYPE_UNKNOWN:
            DbgInfo.enmType = RTLDRDBGINFOTYPE_UNKNOWN;
            break;
        case KLDRDBGINFOTYPE_STABS:
            DbgInfo.enmType = RTLDRDBGINFOTYPE_STABS;
            break;
        case KLDRDBGINFOTYPE_DWARF:
            DbgInfo.enmType = RTLDRDBGINFOTYPE_DWARF;
            if (!pszExtFile)
                DbgInfo.u.Dwarf.pszSection = pszPartNm;
            else
            {
                DbgInfo.enmType = RTLDRDBGINFOTYPE_DWARF_DWO;
                DbgInfo.u.Dwo.uCrc32 = 0;
            }
            break;
        case KLDRDBGINFOTYPE_CODEVIEW:
            DbgInfo.enmType = RTLDRDBGINFOTYPE_CODEVIEW;
            /* Should make some more effort here... Assume the IPRT loader will kick in before we get here! */
            break;
        case KLDRDBGINFOTYPE_WATCOM:
            DbgInfo.enmType = RTLDRDBGINFOTYPE_WATCOM;
            break;
        case KLDRDBGINFOTYPE_HLL:
            DbgInfo.enmType = RTLDRDBGINFOTYPE_HLL;
            break;
        default:
            AssertFailed();
            DbgInfo.enmType = RTLDRDBGINFOTYPE_UNKNOWN;
            break;
    }

    int rc = pArgs->u.pfnEnumDbgInfo(&pArgs->pMod->Core, &DbgInfo, pArgs->pvUser);
    if (RT_FAILURE(rc))
        return rc; /* don't bother converting. */
    return 0;
}


/** @interface_method_impl{RTLDROPS,pfnEnumDbgInfo} */
static DECLCALLBACK(int) rtkldr_EnumDbgInfo(PRTLDRMODINTERNAL pMod, const void *pvBits,
                                            PFNRTLDRENUMDBG pfnCallback, void *pvUser)
{
    PRTLDRMODKLDR       pThis = (PRTLDRMODKLDR)pMod;
    RTLDRMODKLDRARGS    Args;
    Args.pvUser             = pvUser;
    Args.u.pfnEnumDbgInfo   = pfnCallback;
    Args.pvBits             = pvBits;
    Args.pMod               = pThis;
    Args.rc                 = VINF_SUCCESS;
    int rc = kLdrModEnumDbgInfo(pThis->pMod, pvBits, rtkldrEnumDbgInfoWrapper, &Args);
    if (Args.rc != VINF_SUCCESS)
        rc = Args.rc;
    return rc;
}


/** @interface_method_impl{RTLDROPS,pfnEnumSegments} */
static DECLCALLBACK(int) rtkldr_EnumSegments(PRTLDRMODINTERNAL pMod, PFNRTLDRENUMSEGS pfnCallback, void *pvUser)
{
    PRTLDRMODKLDR   pThis      = (PRTLDRMODKLDR)pMod;
    uint32_t const  cSegments  = pThis->pMod->cSegments;
    PCKLDRSEG       paSegments = &pThis->pMod->aSegments[0];
    char            szName[128];

    for (uint32_t iSeg = 0; iSeg < cSegments; iSeg++)
    {
        RTLDRSEG Seg;

        if (!paSegments[iSeg].cchName)
        {
            Seg.pszName = szName;
            Seg.cchName = (uint32_t)RTStrPrintf(szName, sizeof(szName), "Seg%02u", iSeg);
        }
        else if (paSegments[iSeg].pchName[paSegments[iSeg].cchName])
        {
            AssertReturn(paSegments[iSeg].cchName < sizeof(szName), VERR_INTERNAL_ERROR_3);
            RTStrCopyEx(szName, sizeof(szName), paSegments[iSeg].pchName, paSegments[iSeg].cchName);
            Seg.pszName = szName;
            Seg.cchName = paSegments[iSeg].cchName;
        }
        else
        {
            Seg.pszName = paSegments[iSeg].pchName;
            Seg.cchName = paSegments[iSeg].cchName;
        }
        Seg.SelFlat     = paSegments[iSeg].SelFlat;
        Seg.Sel16bit    = paSegments[iSeg].Sel16bit;
        Seg.fFlags      = paSegments[iSeg].fFlags;
        AssertCompile(KLDRSEG_FLAG_16BIT          == RTLDRSEG_FLAG_16BIT      );
        AssertCompile(KLDRSEG_FLAG_OS2_ALIAS16    == RTLDRSEG_FLAG_OS2_ALIAS16);
        AssertCompile(KLDRSEG_FLAG_OS2_CONFORM    == RTLDRSEG_FLAG_OS2_CONFORM);
        AssertCompile(KLDRSEG_FLAG_OS2_IOPL       == RTLDRSEG_FLAG_OS2_IOPL   );

        switch (paSegments[iSeg].enmProt)
        {
            default:
                AssertMsgFailed(("%d\n", paSegments[iSeg].enmProt));
                RT_FALL_THRU();
            case KPROT_NOACCESS:
                Seg.fProt = 0;
                break;

            case KPROT_READONLY:            Seg.fProt = RTMEM_PROT_READ; break;
            case KPROT_READWRITE:           Seg.fProt = RTMEM_PROT_READ | RTMEM_PROT_WRITE; break;
            case KPROT_WRITECOPY:           Seg.fProt = RTMEM_PROT_WRITE; break;
            case KPROT_EXECUTE:             Seg.fProt = RTMEM_PROT_EXEC; break;
            case KPROT_EXECUTE_READ:        Seg.fProt = RTMEM_PROT_EXEC | RTMEM_PROT_READ; break;
            case KPROT_EXECUTE_READWRITE:   Seg.fProt = RTMEM_PROT_EXEC | RTMEM_PROT_READ | RTMEM_PROT_WRITE; break;
            case KPROT_EXECUTE_WRITECOPY:   Seg.fProt = RTMEM_PROT_EXEC | RTMEM_PROT_WRITE; break;
        }
        Seg.cb          = paSegments[iSeg].cb;
        Seg.Alignment   = paSegments[iSeg].Alignment;
        Seg.LinkAddress = paSegments[iSeg].LinkAddress;
        Seg.offFile     = paSegments[iSeg].offFile;
        Seg.cbFile      = paSegments[iSeg].cbFile;
        Seg.RVA         = paSegments[iSeg].RVA;
        Seg.cbMapped    = paSegments[iSeg].cbMapped;

        int rc = pfnCallback(pMod, &Seg, pvUser);
        if (rc != VINF_SUCCESS)
            return rc;
    }

    return VINF_SUCCESS;
}


/** @interface_method_impl{RTLDROPS,pfnLinkAddressToSegOffset} */
static DECLCALLBACK(int) rtkldr_LinkAddressToSegOffset(PRTLDRMODINTERNAL pMod, RTLDRADDR LinkAddress,
                                                       uint32_t *piSeg, PRTLDRADDR poffSeg)
{
    PRTLDRMODKLDR   pThis      = (PRTLDRMODKLDR)pMod;
    uint32_t const  cSegments  = pThis->pMod->cSegments;
    PCKLDRSEG       paSegments = &pThis->pMod->aSegments[0];

    for (uint32_t iSeg = 0; iSeg < cSegments; iSeg++)
    {
        KLDRADDR offSeg = LinkAddress - paSegments[iSeg].LinkAddress;
        if (   offSeg < paSegments[iSeg].cbMapped
            || offSeg < paSegments[iSeg].cb)
        {
            *piSeg = iSeg;
            *poffSeg = offSeg;
            return VINF_SUCCESS;
        }
    }

    return VERR_LDR_INVALID_LINK_ADDRESS;
}


/** @interface_method_impl{RTLDROPS,pfnLinkAddressToRva}. */
static DECLCALLBACK(int) rtkldr_LinkAddressToRva(PRTLDRMODINTERNAL pMod, RTLDRADDR LinkAddress, PRTLDRADDR pRva)
{
    PRTLDRMODKLDR   pThis      = (PRTLDRMODKLDR)pMod;
    uint32_t const  cSegments  = pThis->pMod->cSegments;
    PCKLDRSEG       paSegments = &pThis->pMod->aSegments[0];

    for (uint32_t iSeg = 0; iSeg < cSegments; iSeg++)
    {
        KLDRADDR offSeg = LinkAddress - paSegments[iSeg].LinkAddress;
        if (   offSeg < paSegments[iSeg].cbMapped
            || offSeg < paSegments[iSeg].cb)
        {
            *pRva = paSegments[iSeg].RVA + offSeg;
            return VINF_SUCCESS;
        }
    }

    return VERR_LDR_INVALID_RVA;
}


/** @interface_method_impl{RTLDROPS,pfnSegOffsetToRva} */
static DECLCALLBACK(int) rtkldr_SegOffsetToRva(PRTLDRMODINTERNAL pMod, uint32_t iSeg, RTLDRADDR offSeg, PRTLDRADDR pRva)
{
    PRTLDRMODKLDR pThis = (PRTLDRMODKLDR)pMod;

    if (iSeg >= pThis->pMod->cSegments)
        return VERR_LDR_INVALID_SEG_OFFSET;
    PCKLDRSEG const pSegment = &pThis->pMod->aSegments[iSeg];

    if (   offSeg > pSegment->cbMapped
        && offSeg > pSegment->cb
        && (    pSegment->cbFile < 0
            ||  offSeg > (uint64_t)pSegment->cbFile))
        return VERR_LDR_INVALID_SEG_OFFSET;

    *pRva = pSegment->RVA + offSeg;
    return VINF_SUCCESS;
}


/** @interface_method_impl{RTLDROPS,pfnRvaToSegOffset} */
static DECLCALLBACK(int) rtkldr_RvaToSegOffset(PRTLDRMODINTERNAL pMod, RTLDRADDR Rva, uint32_t *piSeg, PRTLDRADDR poffSeg)
{
    PRTLDRMODKLDR   pThis      = (PRTLDRMODKLDR)pMod;
    uint32_t const  cSegments  = pThis->pMod->cSegments;
    PCKLDRSEG       paSegments = &pThis->pMod->aSegments[0];

    for (uint32_t iSeg = 0; iSeg < cSegments; iSeg++)
    {
        KLDRADDR offSeg = Rva - paSegments[iSeg].RVA;
        if (   offSeg < paSegments[iSeg].cbMapped
            || offSeg < paSegments[iSeg].cb)
        {
            *piSeg = iSeg;
            *poffSeg = offSeg;
            return VINF_SUCCESS;
        }
    }

    return VERR_LDR_INVALID_RVA;
}


/** @interface_method_impl{RTLDROPS,pfnReadDbgInfo} */
static DECLCALLBACK(int) rtkldr_ReadDbgInfo(PRTLDRMODINTERNAL pMod, uint32_t iDbgInfo, RTFOFF off, size_t cb, void *pvBuf)
{
    PRTLDRMODKLDR   pThis = (PRTLDRMODKLDR)pMod;
    RT_NOREF_PV(iDbgInfo);
    /** @todo May have to apply fixups here. */
    return pThis->Core.pReader->pfnRead(pThis->Core.pReader, pvBuf, cb, off);
}


/** @interface_method_impl{RTLDROPS,pfnQueryProp} */
static DECLCALLBACK(int) rtkldr_QueryProp(PRTLDRMODINTERNAL pMod, RTLDRPROP enmProp, void const *pvBits,
                                          void *pvBuf, size_t cbBuf, size_t *pcbRet)
{
    PRTLDRMODKLDR pThis = (PRTLDRMODKLDR)pMod;
    int           rc;
    size_t const  cbBufAvail = cbBuf;
    switch (enmProp)
    {
        case RTLDRPROP_UUID:
            rc = kLdrModQueryImageUuid(pThis->pMod, /*pvBits*/ NULL, (uint8_t *)pvBuf, cbBuf);
            if (rc == KLDR_ERR_NO_IMAGE_UUID)
                return VERR_NOT_FOUND;
            AssertReturn(rc == 0, VERR_INVALID_PARAMETER);
            cbBuf = RT_MIN(cbBuf, sizeof(RTUUID));
            break;

        case RTLDRPROP_INTERNAL_NAME:
            if (!pThis->pMod->cchName)
                return VERR_NOT_FOUND;
            cbBuf = pThis->pMod->cchName + 1;
            if (cbBufAvail < cbBuf)
            {
                if (pcbRet)
                    *pcbRet = cbBufAvail;
                return VERR_BUFFER_OVERFLOW;
            }
            memcpy(pvBuf, pThis->pMod->pszName, cbBuf);
            break;

        default:
            return VERR_NOT_FOUND;
    }
    if (pcbRet)
        *pcbRet = cbBuf;
    RT_NOREF_PV(pvBits);
    return VINF_SUCCESS;
}


/**
 * Operations for a kLdr module.
 */
static const RTLDROPS g_rtkldrOps =
{
    "kLdr",
    rtkldr_Close,
    NULL,
    rtkldr_Done,
    rtkldr_EnumSymbols,
    /* ext */
    rtkldr_GetImageSize,
    rtkldr_GetBits,
    rtkldr_Relocate,
    rtkldr_GetSymbolEx,
    NULL /*pfnQueryForwarderInfo*/,
    rtkldr_EnumDbgInfo,
    rtkldr_EnumSegments,
    rtkldr_LinkAddressToSegOffset,
    rtkldr_LinkAddressToRva,
    rtkldr_SegOffsetToRva,
    rtkldr_RvaToSegOffset,
    rtkldr_ReadDbgInfo,
    rtkldr_QueryProp,
    NULL,
    NULL,
    42
};


/**
 * Open a image using kLdr.
 *
 * @returns iprt status code.
 * @param   pReader     The loader reader instance which will provide the raw image bits.
 * @param   fFlags      Reserved, MBZ.
 * @param   enmArch     CPU architecture specifier for the image to be loaded.
 * @param   phLdrMod    Where to store the handle.
 * @param   pErrInfo    Where to return extended error information. Optional.
 */
int rtldrkLdrOpen(PRTLDRREADER pReader, uint32_t fFlags, RTLDRARCH enmArch, PRTLDRMOD phLdrMod, PRTERRINFO pErrInfo)
{
    RT_NOREF_PV(pErrInfo);

    /* Convert enmArch to k-speak. */
    KCPUARCH enmCpuArch;
    switch (enmArch)
    {
        case RTLDRARCH_WHATEVER:
            enmCpuArch = KCPUARCH_UNKNOWN;
            break;
        case RTLDRARCH_X86_32:
            enmCpuArch = KCPUARCH_X86_32;
            break;
        case RTLDRARCH_AMD64:
            enmCpuArch = KCPUARCH_AMD64;
            break;
        default:
            return VERR_INVALID_PARAMETER;
    }
    KU32 fKFlags = 0;
    if (fFlags & RTLDR_O_FOR_DEBUG)
        fKFlags |= KLDRMOD_OPEN_FLAGS_FOR_INFO;

    /* Create a rtkldrRdr_ instance. */
    PRTKLDRRDR pRdr = (PRTKLDRRDR)RTMemAllocZ(sizeof(*pRdr));
    if (!pRdr)
        return VERR_NO_MEMORY;
    pRdr->Core.u32Magic = KRDR_MAGIC;
    pRdr->Core.pOps = &g_kLdrRdrFileOps;
    pRdr->pReader = pReader;

    /* Try open it. */
    PKLDRMOD pMod;
    int krc = kLdrModOpenFromRdr(&pRdr->Core, fKFlags, enmCpuArch, &pMod);
    if (!krc)
    {
        /* Create a module wrapper for it. */
        PRTLDRMODKLDR pNewMod = (PRTLDRMODKLDR)RTMemAllocZ(sizeof(*pNewMod));
        if (pNewMod)
        {
            pNewMod->Core.u32Magic = RTLDRMOD_MAGIC;
            pNewMod->Core.eState   = LDR_STATE_OPENED;
            pNewMod->Core.pOps     = &g_rtkldrOps;
            pNewMod->Core.pReader  = pReader;
            switch (pMod->enmFmt)
            {
                case KLDRFMT_NATIVE:    pNewMod->Core.enmFormat = RTLDRFMT_NATIVE; break;
                case KLDRFMT_AOUT:      pNewMod->Core.enmFormat = RTLDRFMT_AOUT; break;
                case KLDRFMT_ELF:       pNewMod->Core.enmFormat = RTLDRFMT_ELF; break;
                case KLDRFMT_LX:        pNewMod->Core.enmFormat = RTLDRFMT_LX; break;
                case KLDRFMT_MACHO:     pNewMod->Core.enmFormat = RTLDRFMT_MACHO; break;
                case KLDRFMT_PE:        pNewMod->Core.enmFormat = RTLDRFMT_PE; break;
                default:
                    AssertMsgFailed(("%d\n", pMod->enmFmt));
                    pNewMod->Core.enmFormat = RTLDRFMT_NATIVE;
                    break;
            }
            switch (pMod->enmType)
            {
                case KLDRTYPE_OBJECT:                       pNewMod->Core.enmType = RTLDRTYPE_OBJECT; break;
                case KLDRTYPE_EXECUTABLE_FIXED:             pNewMod->Core.enmType = RTLDRTYPE_EXECUTABLE_FIXED; break;
                case KLDRTYPE_EXECUTABLE_RELOCATABLE:       pNewMod->Core.enmType = RTLDRTYPE_EXECUTABLE_RELOCATABLE; break;
                case KLDRTYPE_EXECUTABLE_PIC:               pNewMod->Core.enmType = RTLDRTYPE_EXECUTABLE_PIC; break;
                case KLDRTYPE_SHARED_LIBRARY_FIXED:         pNewMod->Core.enmType = RTLDRTYPE_SHARED_LIBRARY_FIXED; break;
                case KLDRTYPE_SHARED_LIBRARY_RELOCATABLE:   pNewMod->Core.enmType = RTLDRTYPE_SHARED_LIBRARY_RELOCATABLE; break;
                case KLDRTYPE_SHARED_LIBRARY_PIC:           pNewMod->Core.enmType = RTLDRTYPE_SHARED_LIBRARY_PIC; break;
                case KLDRTYPE_FORWARDER_DLL:                pNewMod->Core.enmType = RTLDRTYPE_FORWARDER_DLL; break;
                case KLDRTYPE_CORE:                         pNewMod->Core.enmType = RTLDRTYPE_CORE; break;
                case KLDRTYPE_DEBUG_INFO:                   pNewMod->Core.enmType = RTLDRTYPE_DEBUG_INFO; break;
                default:
                    AssertMsgFailed(("%d\n", pMod->enmType));
                    pNewMod->Core.enmType = RTLDRTYPE_OBJECT;
                    break;
            }
            switch (pMod->enmEndian)
            {
                case KLDRENDIAN_LITTLE:     pNewMod->Core.enmEndian = RTLDRENDIAN_LITTLE; break;
                case KLDRENDIAN_BIG:        pNewMod->Core.enmEndian = RTLDRENDIAN_BIG; break;
                case KLDRENDIAN_NA:         pNewMod->Core.enmEndian = RTLDRENDIAN_NA; break;
                default:
                    AssertMsgFailed(("%d\n", pMod->enmEndian));
                    pNewMod->Core.enmEndian = RTLDRENDIAN_NA;
                    break;
            }
            switch (pMod->enmArch)
            {
                case KCPUARCH_X86_32:       pNewMod->Core.enmArch   = RTLDRARCH_X86_32; break;
                case KCPUARCH_AMD64:        pNewMod->Core.enmArch   = RTLDRARCH_AMD64; break;
                default:
                    AssertMsgFailed(("%d\n", pMod->enmArch));
                    pNewMod->Core.enmArch = RTLDRARCH_WHATEVER;
                    break;
            }
            pNewMod->pMod          = pMod;
            *phLdrMod = &pNewMod->Core;

#ifdef LOG_ENABLED
            Log(("rtldrkLdrOpen: '%s' (%s) %u segments\n",
                 pMod->pszName, pMod->pszFilename, pMod->cSegments));
            for (unsigned iSeg = 0; iSeg < pMod->cSegments; iSeg++)
            {
                Log(("Segment #%-2u: RVA=%08llx cb=%08llx '%.*s'\n", iSeg,
                     pMod->aSegments[iSeg].RVA,
                     pMod->aSegments[iSeg].cb,
                     pMod->aSegments[iSeg].cchName,
                     pMod->aSegments[iSeg].pchName));
            }
#endif
            return VINF_SUCCESS;
        }

        /* bail out */
        kLdrModClose(pMod);
        krc = KERR_NO_MEMORY;
    }
    else
        RTMemFree(pRdr);

    return rtkldrConvertError(krc);
}

