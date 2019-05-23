/* $Id: SUPLibSem.cpp $ */
/** @file
 * VirtualBox Support Library - Semaphores, ring-3 implementation.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_SUP
#include <VBox/sup.h>

#include <VBox/err.h>
#include <VBox/param.h>
#include <iprt/assert.h>

#include "SUPLibInternal.h"
#include "SUPDrvIOC.h"


/**
 * Worker that makes a SUP_IOCTL_SEM_OP2 request.
 *
 * @returns VBox status code.
 * @param   pSession            The session handle.
 * @param   uType               The semaphore type.
 * @param   hSem                The semaphore handle.
 * @param   uOp                 The operation.
 * @param   u64Arg              The argument if applicable, otherwise 0.
 */
DECLINLINE(int) supSemOp2(PSUPDRVSESSION pSession, uint32_t uType, uintptr_t hSem, uint32_t uOp, uint64_t u64Arg)
{
    NOREF(pSession);
    SUPSEMOP2 Req;
    Req.Hdr.u32Cookie           = g_u32Cookie;
    Req.Hdr.u32SessionCookie    = g_u32SessionCookie;
    Req.Hdr.cbIn                = SUP_IOCTL_SEM_OP2_SIZE_IN;
    Req.Hdr.cbOut               = SUP_IOCTL_SEM_OP2_SIZE_OUT;
    Req.Hdr.fFlags              = SUPREQHDR_FLAGS_DEFAULT;
    Req.Hdr.rc                  = VERR_INTERNAL_ERROR;
    Req.u.In.uType              = uType;
    Req.u.In.hSem               = (uint32_t)hSem;
    AssertReturn(Req.u.In.hSem == hSem, VERR_INVALID_HANDLE);
    Req.u.In.uOp                = uOp;
    Req.u.In.uReserved          = 0;
    Req.u.In.uArg.u64           = u64Arg;
    int rc = suplibOsIOCtl(&g_supLibData, SUP_IOCTL_SEM_OP2, &Req, sizeof(Req));
    if (RT_SUCCESS(rc))
        rc = Req.Hdr.rc;

    return rc;
}


/**
 * Worker that makes a SUP_IOCTL_SEM_OP3 request.
 *
 * @returns VBox status code.
 * @param   pSession            The session handle.
 * @param   uType               The semaphore type.
 * @param   hSem                The semaphore handle.
 * @param   uOp                 The operation.
 * @param   pReq                The request structure.  The caller should pick
 *                              the output data from it himself.
 */
DECLINLINE(int) supSemOp3(PSUPDRVSESSION pSession, uint32_t uType, uintptr_t hSem, uint32_t uOp, PSUPSEMOP3 pReq)
{
    NOREF(pSession);
    pReq->Hdr.u32Cookie           = g_u32Cookie;
    pReq->Hdr.u32SessionCookie    = g_u32SessionCookie;
    pReq->Hdr.cbIn                = SUP_IOCTL_SEM_OP3_SIZE_IN;
    pReq->Hdr.cbOut               = SUP_IOCTL_SEM_OP3_SIZE_OUT;
    pReq->Hdr.fFlags              = SUPREQHDR_FLAGS_DEFAULT;
    pReq->Hdr.rc                  = VERR_INTERNAL_ERROR;
    pReq->u.In.uType              = uType;
    pReq->u.In.hSem               = (uint32_t)hSem;
    AssertReturn(pReq->u.In.hSem == hSem, VERR_INVALID_HANDLE);
    pReq->u.In.uOp                = uOp;
    pReq->u.In.u32Reserved        = 0;
    pReq->u.In.u64Reserved        = 0;
    int rc = suplibOsIOCtl(&g_supLibData, SUP_IOCTL_SEM_OP3, pReq, sizeof(*pReq));
    if (RT_SUCCESS(rc))
        rc = pReq->Hdr.rc;

    return rc;
}


SUPDECL(int) SUPSemEventCreate(PSUPDRVSESSION pSession, PSUPSEMEVENT phEvent)
{
    AssertPtrReturn(phEvent, VERR_INVALID_POINTER);

    SUPSEMOP3 Req;
    int rc = supSemOp3(pSession, SUP_SEM_TYPE_EVENT, (uintptr_t)NIL_SUPSEMEVENT, SUPSEMOP3_CREATE, &Req);
    if (RT_SUCCESS(rc))
        *phEvent = (SUPSEMEVENT)(uintptr_t)Req.u.Out.hSem;
    return rc;
}


SUPDECL(int) SUPSemEventClose(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent)
{
    if (hEvent == NIL_SUPSEMEVENT)
        return VINF_SUCCESS;
    return supSemOp2(pSession, SUP_SEM_TYPE_EVENT, (uintptr_t)hEvent, SUPSEMOP2_CLOSE, 0);
}


SUPDECL(int) SUPSemEventSignal(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent)
{
    return supSemOp2(pSession, SUP_SEM_TYPE_EVENT, (uintptr_t)hEvent, SUPSEMOP2_SIGNAL, 0);
}


SUPDECL(int) SUPSemEventWaitNoResume(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent, uint32_t cMillies)
{
    return supSemOp2(pSession, SUP_SEM_TYPE_EVENT, (uintptr_t)hEvent, SUPSEMOP2_WAIT_MS_REL, cMillies);
}


SUPDECL(int) SUPSemEventWaitNsAbsIntr(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent, uint64_t uNsTimeout)
{
    return supSemOp2(pSession, SUP_SEM_TYPE_EVENT, (uintptr_t)hEvent, SUPSEMOP2_WAIT_NS_ABS, uNsTimeout);
}


SUPDECL(int) SUPSemEventWaitNsRelIntr(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent, uint64_t cNsTimeout)
{
    return supSemOp2(pSession, SUP_SEM_TYPE_EVENT, (uintptr_t)hEvent, SUPSEMOP2_WAIT_NS_REL, cNsTimeout);
}


SUPDECL(uint32_t) SUPSemEventGetResolution(PSUPDRVSESSION pSession)
{
    SUPSEMOP3 Req;
    int rc = supSemOp3(pSession, SUP_SEM_TYPE_EVENT, (uintptr_t)NIL_SUPSEMEVENT, SUPSEMOP3_GET_RESOLUTION, &Req);
    if (RT_SUCCESS(rc))
        return Req.u.Out.cNsResolution;
    return 1000 / 100;
}





SUPDECL(int) SUPSemEventMultiCreate(PSUPDRVSESSION pSession, PSUPSEMEVENTMULTI phEventMulti)
{
    AssertPtrReturn(phEventMulti, VERR_INVALID_POINTER);

    SUPSEMOP3 Req;
    int rc = supSemOp3(pSession, SUP_SEM_TYPE_EVENT_MULTI, (uintptr_t)NIL_SUPSEMEVENTMULTI, SUPSEMOP3_CREATE, &Req);
    if (RT_SUCCESS(rc))
        *phEventMulti = (SUPSEMEVENTMULTI)(uintptr_t)Req.u.Out.hSem;
    return rc;
}


SUPDECL(int) SUPSemEventMultiClose(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti)
{
    if (hEventMulti == NIL_SUPSEMEVENTMULTI)
        return VINF_SUCCESS;
    return supSemOp2(pSession, SUP_SEM_TYPE_EVENT_MULTI, (uintptr_t)hEventMulti, SUPSEMOP2_CLOSE, 0);
}


SUPDECL(int) SUPSemEventMultiSignal(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti)
{
    return supSemOp2(pSession, SUP_SEM_TYPE_EVENT_MULTI, (uintptr_t)hEventMulti, SUPSEMOP2_SIGNAL, 0);
}


SUPDECL(int) SUPSemEventMultiReset(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti)
{
    return supSemOp2(pSession, SUP_SEM_TYPE_EVENT_MULTI, (uintptr_t)hEventMulti, SUPSEMOP2_RESET, 0);
}


SUPDECL(int) SUPSemEventMultiWaitNoResume(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti, uint32_t cMillies)
{
    return supSemOp2(pSession, SUP_SEM_TYPE_EVENT_MULTI, (uintptr_t)hEventMulti, SUPSEMOP2_WAIT_MS_REL, cMillies);
}


SUPDECL(int) SUPSemEventMultiWaitNsAbsIntr(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti, uint64_t uNsTimeout)
{
    return supSemOp2(pSession, SUP_SEM_TYPE_EVENT_MULTI, (uintptr_t)hEventMulti, SUPSEMOP2_WAIT_NS_ABS, uNsTimeout);
}


SUPDECL(int) SUPSemEventMultiWaitNsRelIntr(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti, uint64_t cNsTimeout)
{
    return supSemOp2(pSession, SUP_SEM_TYPE_EVENT_MULTI, (uintptr_t)hEventMulti, SUPSEMOP2_WAIT_NS_REL, cNsTimeout);
}


SUPDECL(uint32_t) SUPSemEventMultiGetResolution(PSUPDRVSESSION pSession)
{
    SUPSEMOP3 Req;
    int rc = supSemOp3(pSession, SUP_SEM_TYPE_EVENT_MULTI, (uintptr_t)NIL_SUPSEMEVENTMULTI, SUPSEMOP3_GET_RESOLUTION, &Req);
    if (RT_SUCCESS(rc))
        return Req.u.Out.cNsResolution;
    return 1000 / 100;
}

