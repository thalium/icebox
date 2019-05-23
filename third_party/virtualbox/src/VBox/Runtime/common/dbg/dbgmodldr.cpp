/* $Id: dbgmodldr.cpp $ */
/** @file
 * IPRT - Debug Module Image Interpretation by RTLdr.
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
#include <iprt/dbg.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/ldr.h>
#include <iprt/mem.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include "internal/dbgmod.h"
#include "internal/ldr.h"
#include "internal/magics.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * The instance data of the RTLdr based image reader.
 */
typedef struct RTDBGMODLDR
{
    /** The loader handle. */
    RTLDRMOD        hLdrMod;
} RTDBGMODLDR;
/** Pointer to instance data NM map reader. */
typedef RTDBGMODLDR *PRTDBGMODLDR;



/** @interface_method_impl{RTDBGMODVTIMG,pfnQueryProp} */
static DECLCALLBACK(int) rtDbgModLdr_QueryProp(PRTDBGMODINT pMod, RTLDRPROP enmProp, void *pvBuf, size_t cbBuf)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)pMod->pvImgPriv;
    return RTLdrQueryProp(pThis->hLdrMod, enmProp, pvBuf, cbBuf);
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnGetArch} */
static DECLCALLBACK(RTLDRARCH) rtDbgModLdr_GetArch(PRTDBGMODINT pMod)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)pMod->pvImgPriv;
    return RTLdrGetArch(pThis->hLdrMod);
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnGetFormat} */
static DECLCALLBACK(RTLDRFMT) rtDbgModLdr_GetFormat(PRTDBGMODINT pMod)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)pMod->pvImgPriv;
    return RTLdrGetFormat(pThis->hLdrMod);
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnReadAt} */
static DECLCALLBACK(int) rtDbgModLdr_ReadAt(PRTDBGMODINT pMod, uint32_t iDbgInfoHint, RTFOFF off, void *pvBuf, size_t cb)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)pMod->pvImgPriv;
    RT_NOREF_PV(iDbgInfoHint);
    return rtLdrReadAt(pThis->hLdrMod, pvBuf, UINT32_MAX /** @todo iDbgInfo*/, off, cb);
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnUnmapPart} */
static DECLCALLBACK(int) rtDbgModLdr_UnmapPart(PRTDBGMODINT pMod, size_t cb, void const **ppvMap)
{
    NOREF(pMod); NOREF(cb);
    RTMemFree((void *)*ppvMap);
    *ppvMap = NULL;
    return VINF_SUCCESS;
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnMapPart} */
static DECLCALLBACK(int) rtDbgModLdr_MapPart(PRTDBGMODINT pMod, uint32_t iDbgInfo, RTFOFF off, size_t cb, void const **ppvMap)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)pMod->pvImgPriv;

    void *pvMap = RTMemAlloc(cb);
    if (!pvMap)
        return VERR_NO_MEMORY;

    int rc = rtLdrReadAt(pThis->hLdrMod, pvMap, iDbgInfo, off, cb);
    if (RT_SUCCESS(rc))
        *ppvMap = pvMap;
    else
    {
        RTMemFree(pvMap);
        *ppvMap = NULL;
    }
    return rc;
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnImageSize} */
static DECLCALLBACK(RTUINTPTR) rtDbgModLdr_ImageSize(PRTDBGMODINT pMod)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)pMod->pvImgPriv;
    return RTLdrSize(pThis->hLdrMod);
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnRvaToSegOffset} */
static DECLCALLBACK(int) rtDbgModLdr_RvaToSegOffset(PRTDBGMODINT pMod, RTLDRADDR Rva, PRTDBGSEGIDX piSeg, PRTLDRADDR poffSeg)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)pMod->pvImgPriv;
    return RTLdrRvaToSegOffset(pThis->hLdrMod, Rva, piSeg, poffSeg);
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnLinkAddressToSegOffset} */
static DECLCALLBACK(int) rtDbgModLdr_LinkAddressToSegOffset(PRTDBGMODINT pMod, RTLDRADDR LinkAddress,
                                                            PRTDBGSEGIDX piSeg, PRTLDRADDR poffSeg)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)pMod->pvImgPriv;
    return RTLdrLinkAddressToSegOffset(pThis->hLdrMod, LinkAddress, piSeg, poffSeg);
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnEnumSymbols} */
static DECLCALLBACK(int) rtDbgModLdr_EnumSymbols(PRTDBGMODINT pMod, uint32_t fFlags, RTLDRADDR BaseAddress,
                                                 PFNRTLDRENUMSYMS pfnCallback, void *pvUser)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)pMod->pvImgPriv;
    return RTLdrEnumSymbols(pThis->hLdrMod, fFlags, NULL /*pvBits*/, BaseAddress, pfnCallback, pvUser);
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnEnumSegments} */
static DECLCALLBACK(int) rtDbgModLdr_EnumSegments(PRTDBGMODINT pMod, PFNRTLDRENUMSEGS pfnCallback, void *pvUser)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)pMod->pvImgPriv;
    return RTLdrEnumSegments(pThis->hLdrMod, pfnCallback, pvUser);
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnEnumDbgInfo} */
static DECLCALLBACK(int) rtDbgModLdr_EnumDbgInfo(PRTDBGMODINT pMod, PFNRTLDRENUMDBG pfnCallback, void *pvUser)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)pMod->pvImgPriv;
    return RTLdrEnumDbgInfo(pThis->hLdrMod, NULL, pfnCallback, pvUser);
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnClose} */
static DECLCALLBACK(int) rtDbgModLdr_Close(PRTDBGMODINT pMod)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)pMod->pvImgPriv;
    AssertPtr(pThis);

    int rc = RTLdrClose(pThis->hLdrMod); AssertRC(rc);
    pThis->hLdrMod = NIL_RTLDRMOD;

    RTMemFree(pThis);

    return VINF_SUCCESS;
}


/** @interface_method_impl{RTDBGMODVTIMG,pfnTryOpen} */
static DECLCALLBACK(int) rtDbgModLdr_TryOpen(PRTDBGMODINT pMod, RTLDRARCH enmArch)
{
    RTLDRMOD hLdrMod;
    int rc = RTLdrOpen(pMod->pszImgFile, RTLDR_O_FOR_DEBUG, enmArch, &hLdrMod);
    if (RT_SUCCESS(rc))
    {
        rc = rtDbgModLdrOpenFromHandle(pMod, hLdrMod);
        if (RT_FAILURE(rc))
            RTLdrClose(hLdrMod);
    }
    return rc;
}


/** Virtual function table for the RTLdr based image reader. */
DECL_HIDDEN_CONST(RTDBGMODVTIMG) const g_rtDbgModVtImgLdr =
{
    /*.u32Magic = */                    RTDBGMODVTIMG_MAGIC,
    /*.fReserved = */                   0,
    /*.pszName = */                     "RTLdr",
    /*.pfnTryOpen = */                  rtDbgModLdr_TryOpen,
    /*.pfnClose = */                    rtDbgModLdr_Close,
    /*.pfnEnumDbgInfo = */              rtDbgModLdr_EnumDbgInfo,
    /*.pfnEnumSegments = */             rtDbgModLdr_EnumSegments,
    /*.pfnEnumSymbols = */              rtDbgModLdr_EnumSymbols,
    /*.pfnImageSize = */                rtDbgModLdr_ImageSize,
    /*.pfnLinkAddressToSegOffset = */   rtDbgModLdr_LinkAddressToSegOffset,
    /*.pfnRvaToSegOffset= */            rtDbgModLdr_RvaToSegOffset,
    /*.pfnMapPart = */                  rtDbgModLdr_MapPart,
    /*.pfnUnmapPart = */                rtDbgModLdr_UnmapPart,
    /*.pfnReadAt = */                   rtDbgModLdr_ReadAt,
    /*.pfnGetFormat = */                rtDbgModLdr_GetFormat,
    /*.pfnGetArch = */                  rtDbgModLdr_GetArch,
    /*.pfnQueryProp = */                rtDbgModLdr_QueryProp,

    /*.u32EndMagic = */                 RTDBGMODVTIMG_MAGIC
};


/**
 * Open PE-image trick.
 *
 * @returns IPRT status code
 * @param   pDbgMod             The debug module instance.
 * @param   hLdrMod             The module to open a image debug backend for.
 */
DECLHIDDEN(int) rtDbgModLdrOpenFromHandle(PRTDBGMODINT pDbgMod, RTLDRMOD hLdrMod)
{
    PRTDBGMODLDR pThis = (PRTDBGMODLDR)RTMemAllocZ(sizeof(RTDBGMODLDR));
    if (!pThis)
        return VERR_NO_MEMORY;

    pThis->hLdrMod     = hLdrMod;
    pDbgMod->pvImgPriv = pThis;
    return VINF_SUCCESS;
}

