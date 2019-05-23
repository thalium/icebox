/* $Id: tstDeviceSUP.cpp $ */
/** @file
 * tstDevice - Test framework for PDM devices/drivers, SUP library exports.
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
#include <VBox/sup.h>

#include <iprt/mem.h>
#include <iprt/semaphore.h>

#include "tstDeviceInternal.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/



/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/



/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/



/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

SUPR3DECL(int) SUPR3HardenedLdrLoadAppPriv(const char *pszFilename, PRTLDRMOD phLdrMod, uint32_t fFlags, PRTERRINFO pErrInfo)
{
    RT_NOREF(fFlags, pErrInfo);
    int rc = VINF_SUCCESS;

    if (!RTStrCmp(pszFilename, "VBoxVMM"))
        rc = RTLdrLoad("tstDeviceVBoxVMMStubs", phLdrMod);
    else
    {
        /* Implement loading of any missing libraries here. */
        AssertFailed();
        rc = VERR_NOT_FOUND;
    }

    return rc;
}


SUPDECL(int) SUPSemEventCreate(PSUPDRVSESSION pSession, PSUPSEMEVENT phEvent)
{
    PTSTDEVSUPDRVSESSION pThis = TSTDEV_PSUPDRVSESSION_2_PTSTDEVSUPDRVSESSION(pSession);
    int rc = VINF_SUCCESS;
    PTSTDEVSUPSEMEVENT pSupSem = (PTSTDEVSUPSEMEVENT)RTMemAllocZ(sizeof(TSTDEVSUPSEMEVENT));

    if (VALID_PTR(pSupSem))
    {
        pSupSem->fMulti = false;
        rc = RTSemEventCreate(&pSupSem->u.hSemEvt);
        if (RT_SUCCESS(rc))
        {
            RTListAppend(&pThis->LstSupSem, &pSupSem->NdSupSem);
            *phEvent = TSTDEV_PTSTDEVSUPSEMEVENT_2_SUPSEMEVENT(pSupSem);
        }
        else
            RTMemFree(pSupSem);
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

SUPDECL(int) SUPSemEventClose(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent)
{
    PTSTDEVSUPDRVSESSION pThis = TSTDEV_PSUPDRVSESSION_2_PTSTDEVSUPDRVSESSION(pSession);
    PTSTDEVSUPSEMEVENT pSupSem = TSTDEV_SUPSEMEVENT_2_PTSTDEVSUPSEMEVENT(hEvent);

    PTSTDEVSUPSEMEVENT pIt;
    RTListForEach(&pThis->LstSupSem, pIt, TSTDEVSUPSEMEVENT, NdSupSem)
    {
        if (pIt == pSupSem)
        {
            AssertReturn(!pSupSem->fMulti, VERR_INVALID_HANDLE);
            RTListNodeRemove(&pSupSem->NdSupSem);
            RTSemEventDestroy(pSupSem->u.hSemEvt);
            RTMemFree(pSupSem);
            return VINF_OBJECT_DESTROYED;
        }
    }

    AssertFailed();
    return VERR_INVALID_HANDLE;
}

SUPDECL(int) SUPSemEventSignal(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent)
{
    RT_NOREF(pSession);
    PTSTDEVSUPSEMEVENT pSupSem = TSTDEV_SUPSEMEVENT_2_PTSTDEVSUPSEMEVENT(hEvent);
    AssertReturn(!pSupSem->fMulti, VERR_INVALID_HANDLE);

    return RTSemEventSignal(pSupSem->u.hSemEvt);
}

SUPDECL(int) SUPSemEventWait(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent, uint32_t cMillies)
{
    RT_NOREF(pSession);
    PTSTDEVSUPSEMEVENT pSupSem = TSTDEV_SUPSEMEVENT_2_PTSTDEVSUPSEMEVENT(hEvent);
    AssertReturn(!pSupSem->fMulti, VERR_INVALID_HANDLE);

    return RTSemEventWait(pSupSem->u.hSemEvt, cMillies);
}

SUPDECL(int) SUPSemEventWaitNoResume(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent, uint32_t cMillies)
{
    RT_NOREF(pSession);
    PTSTDEVSUPSEMEVENT pSupSem = TSTDEV_SUPSEMEVENT_2_PTSTDEVSUPSEMEVENT(hEvent);
    AssertReturn(!pSupSem->fMulti, VERR_INVALID_HANDLE);

    return RTSemEventWaitNoResume(pSupSem->u.hSemEvt, cMillies);
}

SUPDECL(int) SUPSemEventWaitNsAbsIntr(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent, uint64_t uNsTimeout)
{
    RT_NOREF(pSession);
    PTSTDEVSUPSEMEVENT pSupSem = TSTDEV_SUPSEMEVENT_2_PTSTDEVSUPSEMEVENT(hEvent);
    AssertReturn(!pSupSem->fMulti, VERR_INVALID_HANDLE);

#if 0
    return RTSemEventWaitEx(pSupSem->u.hSemEvt,
                            RTSEMWAIT_FLAGS_INTERRUPTIBLE | RTSEMWAIT_FLAGS_ABSOLUTE | RTSEMWAIT_FLAGS_NANOSECS,
                            uNsTimeout);
#else
    RT_NOREF(uNsTimeout);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
#endif
}

SUPDECL(int) SUPSemEventWaitNsRelIntr(PSUPDRVSESSION pSession, SUPSEMEVENT hEvent, uint64_t cNsTimeout)
{
    RT_NOREF(pSession);
    PTSTDEVSUPSEMEVENT pSupSem = TSTDEV_SUPSEMEVENT_2_PTSTDEVSUPSEMEVENT(hEvent);
    AssertReturn(!pSupSem->fMulti, VERR_INVALID_HANDLE);

#if 0
    return RTSemEventWaitEx(pSupSem->u.hSemEvt,
                            RTSEMWAIT_FLAGS_INTERRUPTIBLE | RTSEMWAIT_FLAGS_RELATIVE | RTSEMWAIT_FLAGS_NANOSECS,
                            cNsTimeout);
#else
    RT_NOREF(cNsTimeout);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
#endif
}

SUPDECL(uint32_t) SUPSemEventGetResolution(PSUPDRVSESSION pSession)
{
    RT_NOREF(pSession);
#if 0
    return RTSemEventGetResolution();
#else
    AssertFailed();
    return 0;
#endif
}

SUPDECL(int) SUPSemEventMultiCreate(PSUPDRVSESSION pSession, PSUPSEMEVENTMULTI phEventMulti)
{
    RT_NOREF(pSession, phEventMulti);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}

SUPDECL(int) SUPSemEventMultiClose(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti)
{
    RT_NOREF(pSession, hEventMulti);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}

SUPDECL(int) SUPSemEventMultiSignal(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti)
{
    RT_NOREF(pSession, hEventMulti);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}

SUPDECL(int) SUPSemEventMultiReset(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti)
{
    RT_NOREF(pSession, hEventMulti);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}

SUPDECL(int) SUPSemEventMultiWait(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti, uint32_t cMillies)
{
    RT_NOREF(pSession, hEventMulti, cMillies);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}

SUPDECL(int) SUPSemEventMultiWaitNoResume(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti, uint32_t cMillies)
{
    RT_NOREF(pSession, hEventMulti, cMillies);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}

SUPDECL(int) SUPSemEventMultiWaitNsAbsIntr(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti, uint64_t uNsTimeout)
{
    RT_NOREF(pSession, hEventMulti, uNsTimeout);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}

SUPDECL(int) SUPSemEventMultiWaitNsRelIntr(PSUPDRVSESSION pSession, SUPSEMEVENTMULTI hEventMulti, uint64_t cNsTimeout)
{
    RT_NOREF(pSession, hEventMulti, cNsTimeout);
    AssertFailed();
    return VERR_NOT_IMPLEMENTED;
}

SUPDECL(uint32_t) SUPSemEventMultiGetResolution(PSUPDRVSESSION pSession)
{
    RT_NOREF(pSession);
    AssertFailed();
    return 0;
}

