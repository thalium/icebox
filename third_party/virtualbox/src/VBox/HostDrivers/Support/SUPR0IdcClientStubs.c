/* $Id: SUPR0IdcClientStubs.c $ */
/** @file
 * VirtualBox Support Driver - IDC Client Lib, Stubs for SUPR0 APIs.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
#include "SUPR0IdcClientInternal.h"
#include <VBox/err.h>
#include <iprt/asm.h>


/**
 * Resolves a symbol.
 *
 * @returns Pointer to the symbol on success, NULL on failure.
 *
 * @param   pHandle     The IDC handle.
 * @param   ppfn        Where to return the address of the symbol.
 * @param   pszName     The name of the symbol.
 */
static void supR0IdcGetSymbol(PSUPDRVIDCHANDLE pHandle, PFNRT *ppfn, const char *pszName)
{
    SUPDRVIDCREQGETSYM Req;
    int rc;

    /*
     * Create and send a get symbol request.
     */
    Req.Hdr.cb = sizeof(Req);
    Req.Hdr.rc = VERR_WRONG_ORDER;
    Req.Hdr.pSession = pHandle->s.pSession;
    Req.u.In.pszSymbol = pszName;
    Req.u.In.pszModule = NULL;
    rc = supR0IdcNativeCall(pHandle, SUPDRV_IDC_REQ_GET_SYMBOL, &Req.Hdr);
    if (RT_SUCCESS(rc))
        ASMAtomicWritePtr((void * volatile *)ppfn, (void *)(uintptr_t)Req.u.Out.pfnSymbol);
}


/**
 * Resolves a symbol.
 *
 * @returns Pointer to the symbol on success, NULL on failure.
 *
 * @param   pSession    The IDC session.
 * @param   ppfn        Where to return the address of the symbol.
 * @param   pszName     The name of the symbol.
 */
static void supR0IdcGetSymbolBySession(PSUPDRVSESSION pSession, PFNRT *ppfn, const char *pszName)
{
    PSUPDRVIDCHANDLE pHandle = supR0IdcGetHandleFromSession(pSession);
    if (pHandle)
        supR0IdcGetSymbol(pHandle, ppfn, pszName);
}


SUPR0DECL(void *) SUPR0ObjRegister(PSUPDRVSESSION pSession, SUPDRVOBJTYPE enmType, PFNSUPDRVDESTRUCTOR pfnDestructor, void *pvUser1, void *pvUser2)
{
    static DECLCALLBACKPTR(void *, s_pfn)(PSUPDRVSESSION /* pSession */, SUPDRVOBJTYPE /* enmType */, PFNSUPDRVDESTRUCTOR /* pfnDestructor */, void * /* pvUser1 */, void * /* pvUser2 */);
    DECLCALLBACKPTR(void *,          pfn)(PSUPDRVSESSION /* pSession */, SUPDRVOBJTYPE /* enmType */, PFNSUPDRVDESTRUCTOR /* pfnDestructor */, void * /* pvUser1 */, void * /* pvUser2 */);
    pfn = s_pfn;
    if (!pfn)
    {
        supR0IdcGetSymbolBySession(pSession, (PFNRT *)&s_pfn, "SUPR0ObjRegister");
        pfn = s_pfn;
        if (!pfn)
            return NULL;
    }

    return pfn(pSession, enmType, pfnDestructor, pvUser1, pvUser2);
}


SUPR0DECL(int) SUPR0ObjAddRef(void *pvObj, PSUPDRVSESSION pSession)
{
    static DECLCALLBACKPTR(int, s_pfn)(void * /* pvObj */, PSUPDRVSESSION /* pSession */);
    DECLCALLBACKPTR(int,          pfn)(void * /* pvObj */, PSUPDRVSESSION /* pSession */);
    pfn = s_pfn;
    if (!pfn)
    {
        supR0IdcGetSymbolBySession(pSession, (PFNRT *)&s_pfn, "SUPR0ObjAddRef");
        pfn = s_pfn;
        if (!pfn)
            return VERR_NOT_SUPPORTED;
    }

    return pfn(pvObj, pSession);
}


SUPR0DECL(int) SUPR0ObjRelease(void *pvObj, PSUPDRVSESSION pSession)
{
    static DECLCALLBACKPTR(int, s_pfn)(void * /* pvObj */, PSUPDRVSESSION /* pSession */);
    DECLCALLBACKPTR(int,          pfn)(void * /* pvObj */, PSUPDRVSESSION /* pSession */);
    pfn = s_pfn;
    if (!pfn)
    {
        supR0IdcGetSymbolBySession(pSession, (PFNRT *)&s_pfn, "SUPR0ObjRelease");
        pfn = s_pfn;
        if (!pfn)
            return VERR_NOT_SUPPORTED;
    }

    return pfn(pvObj, pSession);
}


SUPR0DECL(int) SUPR0ObjVerifyAccess(void *pvObj, PSUPDRVSESSION pSession, const char *pszObjName)
{
    static DECLCALLBACKPTR(int, s_pfn)(void * /* pvObj */, PSUPDRVSESSION /* pSession */, const char * /* pszObjName */);
    DECLCALLBACKPTR(int,          pfn)(void * /* pvObj */, PSUPDRVSESSION /* pSession */, const char * /* pszObjName */);
    pfn = s_pfn;
    if (!pfn)
    {
        supR0IdcGetSymbolBySession(pSession, (PFNRT *)&s_pfn, "SUPR0ObjVerifyAccess");
        pfn = s_pfn;
        if (!pfn)
            return VERR_NOT_SUPPORTED;
    }

    return pfn(pvObj, pSession, pszObjName);
}

