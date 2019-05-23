/* $Id: vboxhgsmi.c $ */
/** @file
 * VBox HGCM connection
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
 */

#error "Unused? If so, it's out of date wrt I/O controls."

//#define IN_GUEST
#ifdef IN_GUEST /* entire file */

#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h>
# include <ddraw.h>
#else
# include <sys/ioctl.h>
# include <errno.h>
# include <fcntl.h>
# include <string.h>
# include <unistd.h>
#endif

#include "cr_error.h"
#include "cr_net.h"
#include "cr_bufpool.h"
#include "cr_mem.h"
#include "cr_string.h"
#include "cr_endian.h"
#include "cr_threads.h"
#include "net_internals.h"
#include "cr_process.h"

#include <iprt/thread.h>
#include <iprt/assert.h>

#include <VBoxCrHgsmi.h>
#if 1 /** @todo Try use the Vbgl interface instead of talking directly to the driver? */
# include <VBox/VBoxGuest.h>
#else
# include <VBox/VBoxGuestLib.h>
#endif
#include <VBox/HostServices/VBoxCrOpenGLSvc.h>

#ifndef IN_GUEST
# error "Hgsmi is supported for guest lib only!!"
#endif

#ifdef DEBUG_misha
# ifdef CRASSERT
#  undef CRASSERT
# endif
# define CRASSERT Assert
#endif

#define VBOX_WITH_CRHGSMIPROFILE
#ifdef VBOX_WITH_CRHGSMIPROFILE
#include <iprt/time.h>
#include <stdio.h>

typedef struct VBOXCRHGSMIPROFILE
{
    uint64_t cStartTime;
    uint64_t cStepsTime;
    uint64_t cSteps;
} VBOXCRHGSMIPROFILE, *PVBOXCRHGSMIPROFILE;

#define VBOXCRHGSMIPROFILE_GET_TIME_NANO() RTTimeNanoTS()
#define VBOXCRHGSMIPROFILE_GET_TIME_MILLI() RTTimeMilliTS()

/* 10 sec */
#define VBOXCRHGSMIPROFILE_LOG_STEP_TIME (10000000000.)

DECLINLINE(void) vboxCrHgsmiProfileStart(PVBOXCRHGSMIPROFILE pProfile)
{
    pProfile->cStepsTime = 0;
    pProfile->cSteps = 0;
    pProfile->cStartTime = VBOXCRHGSMIPROFILE_GET_TIME_NANO();
}

DECLINLINE(void) vboxCrHgsmiProfileStep(PVBOXCRHGSMIPROFILE pProfile, uint64_t cStepTime)
{
    pProfile->cStepsTime += cStepTime;
    ++pProfile->cSteps;
}

typedef struct VBOXCRHGSMIPROFILE_SCOPE
{
    uint64_t cStartTime;
//    bool bDisable;
} VBOXCRHGSMIPROFILE_SCOPE, *PVBOXCRHGSMIPROFILE_SCOPE;

static VBOXCRHGSMIPROFILE g_VBoxProfile;

static void vboxCrHgsmiLog(char * szString, ...)
{
    char szBuffer[4096] = {0};
     va_list pArgList;
     va_start(pArgList, szString);
     _vsnprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), szString, pArgList);
     va_end(pArgList);

     VBoxCrHgsmiLog(szBuffer);
}

DECLINLINE(void) vboxCrHgsmiProfileLog(PVBOXCRHGSMIPROFILE pProfile, uint64_t cTime)
{
    uint64_t profileTime = cTime - pProfile->cStartTime;
    double percent = ((double)100.0) * pProfile->cStepsTime / profileTime;
    double cps = ((double)1000000000.) * pProfile->cSteps / profileTime;
    vboxCrHgsmiLog("hgsmi: cps: %.1f, host %.1f%%\n", cps, percent);
}

DECLINLINE(void) vboxCrHgsmiProfileScopeEnter(PVBOXCRHGSMIPROFILE_SCOPE pScope)
{
//    pScope->bDisable = false;
    pScope->cStartTime = VBOXCRHGSMIPROFILE_GET_TIME_NANO();
}

DECLINLINE(void) vboxCrHgsmiProfileScopeExit(PVBOXCRHGSMIPROFILE_SCOPE pScope)
{
//    if (!pScope->bDisable)
    {
        uint64_t cTime = VBOXCRHGSMIPROFILE_GET_TIME_NANO();
        vboxCrHgsmiProfileStep(&g_VBoxProfile, cTime - pScope->cStartTime);
        if (VBOXCRHGSMIPROFILE_LOG_STEP_TIME < cTime - g_VBoxProfile.cStartTime)
        {
            vboxCrHgsmiProfileLog(&g_VBoxProfile, cTime);
            vboxCrHgsmiProfileStart(&g_VBoxProfile);
        }
    }
}


#define VBOXCRHGSMIPROFILE_INIT() vboxCrHgsmiProfileStart(&g_VBoxProfile)
#define VBOXCRHGSMIPROFILE_TERM() do {} while (0)

#define VBOXCRHGSMIPROFILE_FUNC_PROLOGUE() \
        VBOXCRHGSMIPROFILE_SCOPE __vboxCrHgsmiProfileScope; \
        vboxCrHgsmiProfileScopeEnter(&__vboxCrHgsmiProfileScope);

#define VBOXCRHGSMIPROFILE_FUNC_EPILOGUE() \
        vboxCrHgsmiProfileScopeExit(&__vboxCrHgsmiProfileScope); \


#else
#define VBOXCRHGSMIPROFILE_INIT() do {} while (0)
#define VBOXCRHGSMIPROFILE_TERM() do {} while (0)
#define VBOXCRHGSMIPROFILE_FUNC_PROLOGUE() do {} while (0)
#define VBOXCRHGSMIPROFILE_FUNC_EPILOGUE() do {} while (0)
#endif


typedef struct {
    int                  initialized;
    int                  num_conns;
    CRConnection         **conns;
#ifdef CHROMIUM_THREADSAFE
    CRmutex              mutex;
    CRmutex              recvmutex;
#endif
    CRNetReceiveFuncList *recv_list;
    CRNetCloseFuncList   *close_list;
    CRBufferPool *mempool;
} CRVBOXHGSMIDATA;

#define CR_VBOXHGSMI_BUFFER_MAGIC 0xEDCBA123

typedef struct CRVBOXHGSMIBUFFER {
    uint32_t             magic;
    union
    {
        struct
        {
            uint32_t             cbLock : 31;
            uint32_t             bIsBuf : 1;
        };
        uint32_t Value;
    };
    union
    {
        PVBOXUHGSMI_BUFFER pBuffer;
        uint32_t u32Len; /* <- sys mem buf specific */
        uint64_t u64Align;
    };
} CRVBOXHGSMIBUFFER;

typedef struct CRVBOXHGSMI_CLIENT {
    PVBOXUHGSMI pHgsmi;
    PVBOXUHGSMI_BUFFER pCmdBuffer;
    PVBOXUHGSMI_BUFFER pHGBuffer;
    void *pvHGBuffer;
    CRBufferPool *bufpool;
} CRVBOXHGSMI_CLIENT, *PCRVBOXHGSMI_CLIENT;


static CRVBOXHGSMIDATA g_crvboxhgsmi = {0,};

DECLINLINE(PCRVBOXHGSMI_CLIENT) _crVBoxHGSMIClientGet()
{
    PCRVBOXHGSMI_CLIENT pClient = (PCRVBOXHGSMI_CLIENT)VBoxCrHgsmiQueryClient();
    Assert(pClient);
    return pClient;
}

#ifndef RT_OS_WINDOWS
    #define TRUE true
    #define FALSE false
    #define INVALID_HANDLE_VALUE (-1)
#endif

/* Some forward declarations */
static void _crVBoxHGSMIReceiveMessage(CRConnection *conn, PCRVBOXHGSMI_CLIENT pClient);

/* we still use hgcm for establishing the connection
 * @todo: join the common code of HGSMI & HGCM */
/**
 * Send an HGCM request
 *
 * @return VBox status code
 * @param   pvData      Data pointer
 * @param   cbData      Data size
 */
/** @todo use vbglR3DoIOCtl here instead */
static int crVBoxHGCMCall(void *pvData, unsigned cbData)
{
#ifdef IN_GUEST

# ifdef RT_OS_WINDOWS
    DWORD cbReturned;

    if (DeviceIoControl (g_crvboxhgsmi.hGuestDrv,
                         VBOXGUEST_IOCTL_HGCM_CALL(cbData),
                         pvData, cbData,
                         pvData, cbData,
                         &cbReturned,
                         NULL))
    {
        return VINF_SUCCESS;
    }
    Assert(0);
    crDebug("vboxCall failed with %x\n", GetLastError());
    return VERR_NOT_SUPPORTED;
# else
    int rc;
#  if defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
    VBGLBIGREQ Hdr;
    Hdr.u32Magic = VBGLBIGREQ_MAGIC;
    Hdr.cbData = cbData;
    Hdr.pvDataR3 = pvData;
#   if HC_ARCH_BITS == 32
    Hdr.u32Padding = 0;
#   endif
    rc = ioctl(g_crvboxhgsmi.iGuestDrv, VBOXGUEST_IOCTL_HGCM_CALL(cbData), &Hdr);
#  else
    rc = ioctl(g_crvboxhgsmi.iGuestDrv, VBOXGUEST_IOCTL_HGCM_CALL(cbData), pvData);
#  endif
#  ifdef RT_OS_LINUX
    if (rc == 0)
#  else
    if (rc >= 0)
#  endif
    {
        return VINF_SUCCESS;
    }
#  ifdef RT_OS_LINUX
    if (rc >= 0) /* positive values are negated VBox error status codes. */
    {
        crWarning("vboxCall failed with VBox status code %d\n", -rc);
        if (rc==VINF_INTERRUPTED)
        {
            RTMSINTERVAL sl;
            int i;

            for (i=0, sl=50; i<6; i++, sl=sl*2)
            {
                RTThreadSleep(sl);
                rc = ioctl(g_crvboxhgsmi.iGuestDrv, VBOXGUEST_IOCTL_HGCM_CALL(cbData), pvData);
                if (rc==0)
                {
                    crWarning("vboxCall retry(%i) succeeded", i+1);
                    return VINF_SUCCESS;
                }
                else if (rc==VINF_INTERRUPTED)
                {
                    continue;
                }
                else
                {
                    crWarning("vboxCall retry(%i) failed with VBox status code %d", i+1, -rc);
                    break;
                }
            }
        }
    }
    else
#  endif
        crWarning("vboxCall failed with %x\n", errno);
    return VERR_NOT_SUPPORTED;
# endif /*#ifdef RT_OS_WINDOWS*/

#else /*#ifdef IN_GUEST*/
    crError("crVBoxHGCMCall called on host side!");
    CRASSERT(FALSE);
    return VERR_NOT_SUPPORTED;
#endif
}


/* add sizeof header + page align */
#define CRVBOXHGSMI_PAGE_ALIGN(_s) (((_s)  + 0xfff) & ~0xfff)
#define CRVBOXHGSMI_BUF_HDR_SIZE() (sizeof (CRVBOXHGSMIBUFFER))
#define CRVBOXHGSMI_BUF_SIZE(_s) CRVBOXHGSMI_PAGE_ALIGN((_s) + CRVBOXHGSMI_BUF_HDR_SIZE())
#define CRVBOXHGSMI_BUF_LOCK_SIZE(_s) ((_s) + CRVBOXHGSMI_BUF_HDR_SIZE())
#define CRVBOXHGSMI_BUF_DATA(_p) ((void*)(((CRVBOXHGSMIBUFFER*)(_p)) + 1))
#define CRVBOXHGSMI_BUF_HDR(_p) (((CRVBOXHGSMIBUFFER*)(_p)) - 1)
#define CRVBOXHGSMI_BUF_OFFSET(_st2, _st1) ((uint32_t)(((uint8_t*)(_st2)) - ((uint8_t*)(_st1))))

static CRVBOXHGSMIHDR *_crVBoxHGSMICmdBufferLock(PCRVBOXHGSMI_CLIENT pClient, uint32_t cbBuffer)
{
    /* in theory it is OK to use one cmd buffer for async cmd submission
     * because bDiscard flag should result in allocating a new memory backend if the
     * allocation is still in use.
     * However, NOTE: since one and the same semaphore sync event is used for completion notification,
     * for the notification mechanism working as expected
     * 1. host must complete commands in the same order as it receives them
     * (to avoid situation when guest receives notification for another command completion)
     * 2. guest must eventually wait for command completion unless he specified bDoNotSignalCompletion
     * 3. guest must wait for command completion in the same order as it submits them
     * in case we can not satisfy any of the above, we should introduce multiple command buffers */
    CRVBOXHGSMIHDR * pHdr;
    VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags;
    int rc;
    fFlags.Value = 0;
    fFlags.bDiscard = 1;
    rc = pClient->pCmdBuffer->pfnLock(pClient->pCmdBuffer, 0, cbBuffer, fFlags, (void**)&pHdr);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
        return pHdr;

    crWarning("Failed to Lock the command buffer of size(%d), rc(%d)\n", cbBuffer, rc);
    return NULL;
}

static CRVBOXHGSMIHDR *_crVBoxHGSMICmdBufferLockRo(PCRVBOXHGSMI_CLIENT pClient, uint32_t cbBuffer)
{
    /* in theory it is OK to use one cmd buffer for async cmd submission
     * because bDiscard flag should result in allocating a new memory backend if the
     * allocation is still in use.
     * However, NOTE: since one and the same semaphore sync event is used for completion notification,
     * for the notification mechanism working as expected
     * 1. host must complete commands in the same order as it receives them
     * (to avoid situation when guest receives notification for another command completion)
     * 2. guest must eventually wait for command completion unless he specified bDoNotSignalCompletion
     * 3. guest must wait for command completion in the same order as it submits them
     * in case we can not satisfy any of the above, we should introduce multiple command buffers */
    CRVBOXHGSMIHDR * pHdr;
    VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags;
    int rc;
    fFlags.Value = 0;
    fFlags.bReadOnly = 1;
    rc = pClient->pCmdBuffer->pfnLock(pClient->pCmdBuffer, 0, cbBuffer, fFlags, (void**)&pHdr);
    AssertRC(rc);
    if (RT_FAILURE(rc))
        crWarning("Failed to Lock the command buffer of size(%d), rc(%d)\n", cbBuffer, rc);
    return pHdr;
}

static void _crVBoxHGSMICmdBufferUnlock(PCRVBOXHGSMI_CLIENT pClient)
{
    int rc = pClient->pCmdBuffer->pfnUnlock(pClient->pCmdBuffer);
    AssertRC(rc);
    if (RT_FAILURE(rc))
        crWarning("Failed to Unlock the command buffer rc(%d)\n", rc);
}

static int32_t _crVBoxHGSMICmdBufferGetRc(PCRVBOXHGSMI_CLIENT pClient)
{
    CRVBOXHGSMIHDR * pHdr;
    VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags;
    int rc;

    fFlags.Value = 0;
    fFlags.bReadOnly = 1;
    rc = pClient->pCmdBuffer->pfnLock(pClient->pCmdBuffer, 0, sizeof (*pHdr), fFlags, (void**)&pHdr);
    AssertRC(rc);
    if (RT_FAILURE(rc))
    {
        crWarning("Failed to Lock the command buffer of size(%d), rc(%d)\n", sizeof (*pHdr), rc);
        return rc;
    }

    rc = pHdr->result;
    AssertRC(rc);
    pClient->pCmdBuffer->pfnUnlock(pClient->pCmdBuffer);

    return rc;
}

DECLINLINE(PVBOXUHGSMI_BUFFER) _crVBoxHGSMIRecvBufGet(PCRVBOXHGSMI_CLIENT pClient)
{
    if (pClient->pvHGBuffer)
    {
        int rc = pClient->pHGBuffer->pfnUnlock(pClient->pHGBuffer);
        if (RT_FAILURE(rc))
        {
            return NULL;
        }
        pClient->pvHGBuffer = NULL;
    }
    return pClient->pHGBuffer;
}

DECLINLINE(void*) _crVBoxHGSMIRecvBufData(PCRVBOXHGSMI_CLIENT pClient, uint32_t cbBuffer)
{
    VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags;
    int rc;
    Assert(!pClient->pvHGBuffer);
    fFlags.Value = 0;
    rc = pClient->pHGBuffer->pfnLock(pClient->pHGBuffer, 0, cbBuffer, fFlags, &pClient->pvHGBuffer);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
    {
        return pClient->pvHGBuffer;
    }
    return NULL;
}


static PVBOXUHGSMI_BUFFER _crVBoxHGSMIBufFromMemPtr(void *pvBuf)
{
    CRVBOXHGSMIBUFFER *pHdr = CRVBOXHGSMI_BUF_HDR(pvBuf);
    PVBOXUHGSMI_BUFFER pBuf;
    int rc;

    CRASSERT(pHdr->magic == CR_VBOXHGSMI_BUFFER_MAGIC);
    pBuf = pHdr->pBuffer;
    rc = pBuf->pfnUnlock(pBuf);
    AssertRC(rc);
    if (RT_FAILURE(rc))
    {
        return NULL;
    }
    return pBuf;
}

static PVBOXUHGSMI_BUFFER _crVBoxHGSMIBufAlloc(PCRVBOXHGSMI_CLIENT pClient, uint32_t cbSize)
{
    PVBOXUHGSMI_BUFFER buf;
    int rc;

    buf = (PVBOXUHGSMI_BUFFER ) crBufferPoolPop(pClient->bufpool, cbSize);

    if (!buf)
    {
        crDebug("Buffer pool %p was empty; allocating new %d byte buffer.",
                        (void *) pClient->bufpool,
                        cbSize);
        rc = pClient->pHgsmi->pfnBufferCreate(pClient->pHgsmi, cbSize,
                                VBOXUHGSMI_SYNCHOBJECT_TYPE_NONE, NULL,
                                &buf);
        AssertRC(rc);
        if (RT_FAILURE(rc))
            crWarning("Failed to Create a buffer of size(%d), rc(%d)\n", cbSize, rc);
    }
    return buf;
}

static void _crVBoxHGSMIBufFree(PCRVBOXHGSMI_CLIENT pClient, PVBOXUHGSMI_BUFFER pBuf)
{
    crBufferPoolPush(pClient->bufpool, pBuf, pBuf->cbBuffer);
}

static CRVBOXHGSMIBUFFER* _crVBoxHGSMISysMemAlloc(uint32_t cbSize)
{
    CRVBOXHGSMIBUFFER *pBuf;
#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgsmi.mutex);
#endif
    pBuf = crBufferPoolPop(g_crvboxhgsmi.mempool, cbSize);
    if (!pBuf)
    {
        pBuf = (CRVBOXHGSMIBUFFER*)crAlloc(CRVBOXHGSMI_BUF_HDR_SIZE() + cbSize);
        Assert(pBuf);
        if (pBuf)
        {
            pBuf->magic = CR_VBOXHGSMI_BUFFER_MAGIC;
            pBuf->cbLock = cbSize;
            pBuf->bIsBuf = false;
            pBuf->u32Len = 0;
        }
    }
#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgsmi.mutex);
#endif
    return pBuf;
}

static void *_crVBoxHGSMIDoAlloc(PCRVBOXHGSMI_CLIENT pClient, uint32_t cbSize)
{
    PVBOXUHGSMI_BUFFER buf;
    CRVBOXHGSMIBUFFER *pData = NULL;
    int rc;

    buf = _crVBoxHGSMIBufAlloc(pClient, CRVBOXHGSMI_BUF_SIZE(cbSize));
    Assert(buf);
    if (buf)
    {
        VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags;
        buf->pvUserData = pClient;
        fFlags.Value = 0;
        fFlags.bDiscard = 1;
        rc = buf->pfnLock(buf, 0, CRVBOXHGSMI_BUF_LOCK_SIZE(cbSize), fFlags, (void**)&pData);
        if (RT_SUCCESS(rc))
        {
            pData->magic = CR_VBOXHGSMI_BUFFER_MAGIC;
            pData->cbLock = CRVBOXHGSMI_BUF_LOCK_SIZE(cbSize);
            pData->bIsBuf = true;
            pData->pBuffer = buf;
        }
        else
        {
            crWarning("Failed to Lock the buffer, rc(%d)\n", rc);
        }
    }

    return CRVBOXHGSMI_BUF_DATA(pData);

}
static void *crVBoxHGSMIAlloc(CRConnection *conn)
{
    PCRVBOXHGSMI_CLIENT pClient;
    void *pvBuf;

    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

    pClient = _crVBoxHGSMIClientGet();
    pvBuf = _crVBoxHGSMIDoAlloc(pClient, conn->buffer_size);
    Assert(pvBuf);

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();

    return pvBuf;
}

DECLINLINE(void) _crVBoxHGSMIFillCmd(VBOXUHGSMI_BUFFER_SUBMIT *pSubm, PCRVBOXHGSMI_CLIENT pClient, uint32_t cbData)
{
    pSubm->pBuf = pClient->pCmdBuffer;
    pSubm->offData = 0;
    pSubm->cbData = cbData;
    pSubm->fFlags.Value = 0;
    pSubm->fFlags.bDoNotRetire = 1;
//    pSubm->fFlags.bDoNotSignalCompletion = 1; /* <- we do not need that actually since
//                                                       * in case we want completion,
//                                                       * we will block in _crVBoxHGSMICmdBufferGetRc (when locking the buffer)
//                                                       * which is needed for getting the result */
}

#ifdef RT_OS_WINDOWS
#define CRVBOXHGSMI_BUF_WAIT(_pBub) WaitForSingleObject((_pBub)->hSynch, INFINITE);
#else
# error "Port Me!!"
#endif

DECLINLINE(void) _crVBoxHGSMIWaitCmd(PCRVBOXHGSMI_CLIENT pClient)
{
    int rc = CRVBOXHGSMI_BUF_WAIT(pClient->pCmdBuffer);
    Assert(rc == 0);
}

static void _crVBoxHGSMIWriteExact(CRConnection *conn, PCRVBOXHGSMI_CLIENT pClient, PVBOXUHGSMI_BUFFER pBuf, uint32_t offStart, unsigned int len)
{
    int rc;
    int32_t callRes;
    VBOXUHGSMI_BUFFER_SUBMIT aSubmit[2];

#ifdef IN_GUEST
    if (conn->u32InjectClientID)
    {
        CRVBOXHGSMIINJECT *parms = (CRVBOXHGSMIINJECT *)_crVBoxHGSMICmdBufferLock(pClient, sizeof (*parms));
        Assert(parms);
        if (!parms)
        {
            return;
        }

        parms->hdr.result      = VERR_WRONG_ORDER;
        parms->hdr.u32ClientID = conn->u32ClientID;
        parms->hdr.u32Function = SHCRGL_GUEST_FN_INJECT;
//        parms->hdr.u32Reserved = 0;

        parms->u32ClientID = conn->u32InjectClientID;

        parms->iBuffer = 1;
        _crVBoxHGSMICmdBufferUnlock(pClient);

        _crVBoxHGSMIFillCmd(&aSubmit[0], pClient, sizeof (*parms));

        aSubmit[1].pBuf = pBuf;
        aSubmit[1].offData = offStart;
        aSubmit[1].cbData = len;
        aSubmit[1].fFlags.Value = 0;
        aSubmit[1].fFlags.bHostReadOnly = 1;

        rc = pClient->pHgsmi->pfnBufferSubmitAsynch(pClient->pHgsmi, aSubmit, 2);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            _crVBoxHGSMIWaitCmd(pClient);
                /** @todo do we need to wait for completion actually?
                 * NOTE: in case we do not need completion,
                 * we MUST specify bDoNotSignalCompletion flag for the command buffer */
//                CRVBOXHGSMI_BUF_WAIT(pClient->pCmdBuffer);

            callRes = _crVBoxHGSMICmdBufferGetRc(pClient);
        }
    }
    else
#endif
    {
        CRVBOXHGSMIWRITE * parms = (CRVBOXHGSMIWRITE *)_crVBoxHGSMICmdBufferLock(pClient, sizeof (*parms));;

        parms->hdr.result      = VERR_WRONG_ORDER;
        parms->hdr.u32ClientID = conn->u32ClientID;
        parms->hdr.u32Function = SHCRGL_GUEST_FN_WRITE;
//        parms->hdr.u32Reserved = 0;

        parms->iBuffer = 1;
        _crVBoxHGSMICmdBufferUnlock(pClient);

        _crVBoxHGSMIFillCmd(&aSubmit[0], pClient, sizeof (*parms));

        aSubmit[1].pBuf = pBuf;
        aSubmit[1].offData = offStart;
        aSubmit[1].cbData = len;
        aSubmit[1].fFlags.Value = 0;
        aSubmit[1].fFlags.bHostReadOnly = 1;

        rc = pClient->pHgsmi->pfnBufferSubmitAsynch(pClient->pHgsmi, aSubmit, 2);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            _crVBoxHGSMIWaitCmd(pClient);
                /** @todo do we need to wait for completion actually?
                 * NOTE: in case we do not need completion,
                 * we MUST specify bDoNotSignalCompletion flag for the command buffer */
//                CRVBOXHGSMI_BUF_WAIT(pClient->pCmdBuffer);

            callRes = _crVBoxHGSMICmdBufferGetRc(pClient);
        }
    }

    if (RT_FAILURE(rc) || RT_FAILURE(callRes))
    {
        crWarning("SHCRGL_GUEST_FN_WRITE failed with %x %x\n", rc, callRes);
    }
}

static void crVBoxHGSMIWriteExact(CRConnection *conn, const void *buf, unsigned int len)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    Assert(0);

    CRASSERT(0);
//    PCRVBOXHGSMI_CLIENT pClient;
//    PVBOXUHGSMI_BUFFER pBuf = _crVBoxHGSMIBufFromMemPtr(buf);
//    if (!pBuf)
//        return;
//    pClient = (PCRVBOXHGSMI_CLIENT)pBuf->pvUserData;
//    _crVBoxHGSMIWriteExact(conn, pClient, pBuf, 0, len);
//    _crVBoxHGSMIBufFree(pClient, pBuf);
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void _crVBoxHGSMIPollHost(CRConnection *conn, PCRVBOXHGSMI_CLIENT pClient)
{
    CRVBOXHGSMIREAD *parms = (CRVBOXHGSMIREAD *)_crVBoxHGSMICmdBufferLock(pClient, sizeof (*parms));
    int rc;
    VBOXUHGSMI_BUFFER_SUBMIT aSubmit[2];
    PVBOXUHGSMI_BUFFER pRecvBuffer;
    uint32_t cbBuffer;

    Assert(parms);

    parms->hdr.result      = VERR_WRONG_ORDER;
    parms->hdr.u32ClientID = conn->u32ClientID;
    parms->hdr.u32Function = SHCRGL_GUEST_FN_READ;
//    parms->hdr.u32Reserved = 0;

    CRASSERT(!conn->pBuffer); //make sure there's no data to process
    parms->iBuffer = 1;
    parms->cbBuffer = 0;

    _crVBoxHGSMICmdBufferUnlock(pClient);

    pRecvBuffer = _crVBoxHGSMIRecvBufGet(pClient);
    Assert(pRecvBuffer);
    if (!pRecvBuffer)
        return;

    _crVBoxHGSMIFillCmd(&aSubmit[0], pClient, sizeof (*parms));

    aSubmit[1].pBuf = pRecvBuffer;
    aSubmit[1].offData = 0;
    aSubmit[1].cbData = pRecvBuffer->cbBuffer;
    aSubmit[1].fFlags.Value = 0;
    aSubmit[1].fFlags.bHostWriteOnly = 1;

    rc = pClient->pHgsmi->pfnBufferSubmitAsynch(pClient->pHgsmi, aSubmit, 2);
    AssertRC(rc);
    if (RT_FAILURE(rc))
    {
        crWarning("pfnBufferSubmitAsynch failed with %d \n", rc);
        return;
    }

    _crVBoxHGSMIWaitCmd(pClient);

    parms = (CRVBOXHGSMIREAD *)_crVBoxHGSMICmdBufferLockRo(pClient, sizeof (*parms));
    Assert(parms);
    if (!parms)
    {
        crWarning("_crVBoxHGSMICmdBufferLockRo failed\n");
        return;
    }

    if (RT_SUCCESS(parms->hdr.result))
        cbBuffer = parms->cbBuffer;
    else
        cbBuffer = 0;

    _crVBoxHGSMICmdBufferUnlock(pClient);

    if (cbBuffer)
    {
        void *pvData = _crVBoxHGSMIRecvBufData(pClient, cbBuffer);
        Assert(pvData);
        if (pvData)
        {
            conn->pBuffer  = pvData;
            conn->cbBuffer = cbBuffer;
        }
    }
}

static void _crVBoxHGSMIReadExact(CRConnection *conn, PCRVBOXHGSMI_CLIENT pClient)
{
    _crVBoxHGSMIPollHost(conn, pClient);

    if (conn->cbBuffer)
        _crVBoxHGSMIReceiveMessage(conn, pClient);
}

/* Same as crVBoxHGCMWriteExact, but combined with read of writeback data.
 * This halves the number of HGCM calls we do,
 * most likely crVBoxHGCMPollHost shouldn't be called at all now.
 */
static void
_crVBoxHGSMIWriteReadExact(CRConnection *conn, PCRVBOXHGSMI_CLIENT pClient, void *buf, uint32_t offBuffer, unsigned int len, bool bIsBuffer)
{
    CRVBOXHGSMIWRITEREAD *parms = (CRVBOXHGSMIWRITEREAD*)_crVBoxHGSMICmdBufferLock(pClient, sizeof (*parms));
    int rc;
    VBOXUHGSMI_BUFFER_SUBMIT aSubmit[3];
    PVBOXUHGSMI_BUFFER pBuf = NULL;
    VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags;
//    uint32_t cbBuffer;

    parms->hdr.result      = VERR_WRONG_ORDER;
    parms->hdr.u32ClientID = conn->u32ClientID;
    parms->hdr.u32Function = SHCRGL_GUEST_FN_WRITE_READ;
//    parms->hdr.u32Reserved = 0;

    parms->iBuffer = 1;

    CRASSERT(!conn->pBuffer); //make sure there's no data to process
    parms->iWriteback = 2;
    parms->cbWriteback = 0;

    _crVBoxHGSMICmdBufferUnlock(pClient);

    if (!bIsBuffer)
    {
        void *pvBuf;
        pBuf = _crVBoxHGSMIBufAlloc(pClient, len);
        Assert(pBuf);
        if (!pBuf)
            return;

        Assert(!offBuffer);

        offBuffer = 0;
        fFlags.Value = 0;
        fFlags.bDiscard = 1;
        fFlags.bWriteOnly = 1;
        rc = pBuf->pfnLock(pBuf, 0, len, fFlags, &pvBuf);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            memcpy(pvBuf, buf, len);
            rc = pBuf->pfnUnlock(pBuf);
            AssertRC(rc);
            CRASSERT(RT_SUCCESS(rc));
        }
        else
        {
            _crVBoxHGSMIBufFree(pClient, pBuf);
            return;
        }
    }
    else
    {
        pBuf = (PVBOXUHGSMI_BUFFER)buf;
    }

    do
    {
        PVBOXUHGSMI_BUFFER pRecvBuffer = _crVBoxHGSMIRecvBufGet(pClient);
        Assert(pRecvBuffer);
        if (!pRecvBuffer)
            return;

        _crVBoxHGSMIFillCmd(&aSubmit[0], pClient, sizeof (*parms));

        aSubmit[1].pBuf = pBuf;
        aSubmit[1].offData = offBuffer;
        aSubmit[1].cbData = len;
        aSubmit[1].fFlags.Value = 0;
        aSubmit[1].fFlags.bHostReadOnly = 1;

        aSubmit[2].pBuf = pRecvBuffer;
        aSubmit[2].offData = 0;
        aSubmit[2].cbData = pRecvBuffer->cbBuffer;
        aSubmit[2].fFlags.Value = 0;

        rc = pClient->pHgsmi->pfnBufferSubmitAsynch(pClient->pHgsmi, aSubmit, 3);
        AssertRC(rc);
        if (RT_FAILURE(rc))
        {
            crWarning("pfnBufferSubmitAsynch failed with %d \n", rc);
            break;
        }

        _crVBoxHGSMIWaitCmd(pClient);

        parms = (CRVBOXHGSMIWRITEREAD *)_crVBoxHGSMICmdBufferLockRo(pClient, sizeof (*parms));
        Assert(parms);
        if (parms)
        {
            uint32_t cbWriteback = parms->cbWriteback;
            rc = parms->hdr.result;
            _crVBoxHGSMICmdBufferUnlock(pClient);
#ifdef DEBUG
            parms = NULL;
#endif
            if (RT_SUCCESS(rc))
            {
                if (cbWriteback)
                {
                    void *pvData = _crVBoxHGSMIRecvBufData(pClient, cbWriteback);
                    Assert(pvData);
                    if (pvData)
                    {
                        conn->pBuffer  = pvData;
                        conn->cbBuffer = cbWriteback;
                        _crVBoxHGSMIReceiveMessage(conn, pClient);
                    }
                }
            }
            else if (VERR_BUFFER_OVERFLOW == rc)
            {
                PVBOXUHGSMI_BUFFER pOldBuf = pClient->pHGBuffer;
                Assert(!pClient->pvHGBuffer);
                CRASSERT(cbWriteback>pClient->pHGBuffer->cbBuffer);
                crDebug("Reallocating host buffer from %d to %d bytes", conn->cbHostBufferAllocated, cbWriteback);

                rc = pClient->pHgsmi->pfnBufferCreate(pClient->pHgsmi, CRVBOXHGSMI_PAGE_ALIGN(cbWriteback),
                                VBOXUHGSMI_SYNCHOBJECT_TYPE_NONE, NULL, &pClient->pHGBuffer);
                AssertRC(rc);
                CRASSERT(RT_SUCCESS(rc));
                if (RT_SUCCESS(rc))
                {
                    rc = pOldBuf->pfnDestroy(pOldBuf);
                    CRASSERT(RT_SUCCESS(rc));

                    _crVBoxHGSMIReadExact(conn, pClient/*, cbWriteback*/);
                }
            }
            else
            {
                crWarning("SHCRGL_GUEST_FN_WRITE_READ (%i) failed with %x \n", len, rc);
            }
        }
        else
        {
            crWarning("_crVBoxHGSMICmdBufferLockRo failed\n");
            break;
        }
    } while (0);

    if (!bIsBuffer)
        _crVBoxHGSMIBufFree(pClient, pBuf);
}

static void crVBoxHGSMISend(CRConnection *conn, void **bufp,
                           const void *start, unsigned int len)
{
    PCRVBOXHGSMI_CLIENT pClient;
    PVBOXUHGSMI_BUFFER pBuf;

    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

    if (!bufp) /* We're sending a user-allocated buffer. */
    {
        pClient = _crVBoxHGSMIClientGet();
#ifndef IN_GUEST
            /// @todo remove temp buffer allocation in unpacker
            /* we're at the host side, so just store data until guest polls us */
            _crVBoxHGCMWriteBytes(conn, start, len);
#else
        CRASSERT(!conn->u32InjectClientID);
        crDebug("SHCRGL: sending userbuf with %d bytes\n", len);
        _crVBoxHGSMIWriteReadExact(conn, pClient, (void*)start, 0, len, false);
#endif
        VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
        return;
    }

    /* The region [start .. start + len + 1] lies within a buffer that
     * was allocated with crVBoxHGCMAlloc() and can be put into the free
     * buffer pool when we're done sending it.
     */

    pBuf = _crVBoxHGSMIBufFromMemPtr(*bufp);
    Assert(pBuf);
    if (!pBuf)
    {
        VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
        return;
    }

    pClient = (PCRVBOXHGSMI_CLIENT)pBuf->pvUserData;

    /* Length would be passed as part of HGCM pointer description
     * No need to prepend it to the buffer
     */
#ifdef IN_GUEST
    if (conn->u32InjectClientID)
    {
        _crVBoxHGSMIWriteExact(conn, pClient, pBuf, CRVBOXHGSMI_BUF_OFFSET(start, *bufp) + CRVBOXHGSMI_BUF_HDR_SIZE(), len);
    }
    else
#endif
    {
        _crVBoxHGSMIWriteReadExact(conn, pClient, pBuf, CRVBOXHGSMI_BUF_OFFSET(start, *bufp) + CRVBOXHGSMI_BUF_HDR_SIZE(), len, true);
    }

    /* Reclaim this pointer for reuse */
    _crVBoxHGSMIBufFree(pClient, pBuf);
    /* Since the buffer's now in the 'free' buffer pool, the caller can't
     * use it any more.  Setting bufp to NULL will make sure the caller
     * doesn't try to re-use the buffer.
     */
    *bufp = NULL;

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void crVBoxHGSMISingleRecv(CRConnection *conn, void *buf, unsigned int len)
{
    PCRVBOXHGSMI_CLIENT pClient;
    PVBOXUHGSMI_BUFFER pBuf;
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    pBuf = _crVBoxHGSMIBufFromMemPtr(buf);
    Assert(pBuf);
    Assert(0);
    CRASSERT(0);
    if (!pBuf)
    {
        VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
        return;
    }
    pClient = (PCRVBOXHGSMI_CLIENT)pBuf->pvUserData;
    _crVBoxHGSMIReadExact(conn, pClient);
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void _crVBoxHGSMIFree(CRConnection *conn, void *buf)
{
    CRVBOXHGSMIBUFFER *hgsmi_buffer = (CRVBOXHGSMIBUFFER *) buf - 1;

    CRASSERT(hgsmi_buffer->magic == CR_VBOXHGSMI_BUFFER_MAGIC);

    if (hgsmi_buffer->bIsBuf)
    {
        PVBOXUHGSMI_BUFFER pBuf = hgsmi_buffer->pBuffer;
        PCRVBOXHGSMI_CLIENT pClient = (PCRVBOXHGSMI_CLIENT)pBuf->pvUserData;
        pBuf->pfnUnlock(pBuf);
        _crVBoxHGSMIBufFree(pClient, pBuf);
    }
    else
    {
        /** @todo wrong len for redir buffers*/
        conn->recv_credits += hgsmi_buffer->u32Len;

#ifdef CHROMIUM_THREADSAFE
            crLockMutex(&g_crvboxhgsmi.mutex);
#endif
            crBufferPoolPush(g_crvboxhgsmi.mempool, hgsmi_buffer, hgsmi_buffer->cbLock);
#ifdef CHROMIUM_THREADSAFE
            crUnlockMutex(&g_crvboxhgsmi.mutex);
#endif
    }
}
static void crVBoxHGSMIFree(CRConnection *conn, void *buf)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    _crVBoxHGSMIFree(conn, buf);
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void _crVBoxHGSMIReceiveMessage(CRConnection *conn, PCRVBOXHGSMI_CLIENT pClient)
{
    uint32_t len;
    CRVBOXHGSMIBUFFER* pSysBuf;
    CRMessage *msg;
    CRMessageType cached_type;

    len = conn->cbBuffer;
    Assert(len > 0);
    Assert(conn->pBuffer);
    CRASSERT(len > 0);
    CRASSERT(conn->pBuffer);

#ifndef IN_GUEST
    if (conn->allow_redir_ptr)
    {
#endif //IN_GUEST
        CRASSERT(conn->buffer_size >= sizeof(CRMessageRedirPtr));

        pSysBuf = _crVBoxHGSMISysMemAlloc( conn->buffer_size );
        pSysBuf->u32Len = sizeof(CRMessageRedirPtr);

        msg = (CRMessage *)CRVBOXHGSMI_BUF_DATA(pSysBuf);

        msg->header.type = CR_MESSAGE_REDIR_PTR;
        msg->redirptr.pMessage = (CRMessageHeader*) (conn->pBuffer);
        msg->header.conn_id = msg->redirptr.pMessage->conn_id;

        cached_type = msg->redirptr.pMessage->type;

        conn->cbBuffer = 0;
        conn->pBuffer  = NULL;
#ifndef IN_GUEST
    }
    else
    {
        if ( len <= conn->buffer_size )
        {
            /* put in pre-allocated buffer */
            hgcm_buffer = (CRVBOXHGCMBUFFER *) crVBoxHGCMAlloc( conn ) - 1;
        }
        else
        {
            /* allocate new buffer,
             * not using pool here as it's most likely one time transfer of huge texture
             */
            hgcm_buffer            = (CRVBOXHGCMBUFFER *) crAlloc( sizeof(CRVBOXHGCMBUFFER) + len );
            hgcm_buffer->magic     = CR_VBOXHGCM_BUFFER_MAGIC;
            hgcm_buffer->kind      = CR_VBOXHGCM_MEMORY_BIG;
            hgcm_buffer->allocated = sizeof(CRVBOXHGCMBUFFER) + len;
# ifdef RT_OS_WINDOWS
            hgcm_buffer->pDDS      = NULL;
# endif
        }

        hgcm_buffer->len = len;

        _crVBoxHGCMReadBytes(conn, hgcm_buffer + 1, len);

        msg = (CRMessage *) (hgcm_buffer + 1);
        cached_type = msg->header.type;
    }
#endif //IN_GUEST

    conn->recv_credits     -= len;
    conn->total_bytes_recv += len;
    conn->recv_count++;

    crNetDispatchMessage( g_crvboxhgsmi.recv_list, conn, msg, len );

    /* CR_MESSAGE_OPCODES is freed in crserverlib/server_stream.c with crNetFree.
     * OOB messages are the programmer's problem.  -- Humper 12/17/01
     */
    if (cached_type != CR_MESSAGE_OPCODES
        && cached_type != CR_MESSAGE_OOB
        && cached_type != CR_MESSAGE_GATHER)
    {
        _crVBoxHGSMIFree(conn, msg);
    }
}

static void crVBoxHGSMIReceiveMessage(CRConnection *conn)
{
    PCRVBOXHGSMI_CLIENT pClient;
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

    pClient = _crVBoxHGSMIClientGet();

    Assert(pClient);
    Assert(0);

    _crVBoxHGSMIReceiveMessage(conn, pClient);
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}
/*
 * Called on host side only, to "accept" client connection
 */
static void crVBoxHGSMIAccept( CRConnection *conn, const char *hostname, unsigned short port )
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    Assert(0);

    CRASSERT(conn && conn->pHostBuffer);
#ifdef IN_GUEST
    CRASSERT(FALSE);
#endif
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static int crVBoxHGSMISetVersion(CRConnection *conn, unsigned int vMajor, unsigned int vMinor)
{
    CRVBOXHGCMSETVERSION parms;
    int rc;

    VBGL_HGCM_HDR_INIT(&parms.hdr, conn->u32ClientID, SHCRGL_GUEST_FN_SET_VERSION, SHCRGL_CPARMS_SET_VERSION);

    parms.vMajor.type      = VMMDevHGCMParmType_32bit;
    parms.vMajor.u.value32 = CR_PROTOCOL_VERSION_MAJOR;
    parms.vMinor.type      = VMMDevHGCMParmType_32bit;
    parms.vMinor.u.value32 = CR_PROTOCOL_VERSION_MINOR;

    rc = crVBoxHGCMCall(&parms, sizeof(parms));

    AssertRC(rc);

    if (RT_FAILURE(rc) || RT_FAILURE(parms.hdr.result))
    {
        crWarning("Host doesn't accept our version %d.%d. Make sure you have appropriate additions installed!",
                  parms.vMajor.u.value32, parms.vMinor.u.value32);
        return FALSE;
    }

    conn->vMajor = CR_PROTOCOL_VERSION_MAJOR;
    conn->vMinor = CR_PROTOCOL_VERSION_MINOR;

    return TRUE;
}

static int crVBoxHGSMISetPID(CRConnection *conn, unsigned long long pid)
{
    CRVBOXHGCMSETPID parms;
    int rc;

    VBGL_HGCM_HDR_INIT(&parms.hdr, conn->u32ClientID, SHCRGL_GUEST_FN_SET_PID, SHCRGL_CPARMS_SET_PID);

    parms.u64PID.type     = VMMDevHGCMParmType_64bit;
    parms.u64PID.u.value64 = pid;

    rc = crVBoxHGCMCall(&parms, sizeof(parms));

    if (RT_FAILURE(rc) || RT_FAILURE(parms.hdr.result))
    {
        Assert(0);

        crWarning("SHCRGL_GUEST_FN_SET_PID failed!");
        return FALSE;
    }

    return TRUE;
}

/**
 * The function that actually connects.  This should only be called by clients,
 * guests in vbox case.
 * Servers go through crVBoxHGCMAccept;
 */
static int crVBoxHGSMIDoConnect( CRConnection *conn )
{
#ifdef IN_GUEST
    int rc;
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    rc = VbglR3InitUser();
    if (RT_SUCCESS(rc))
    {
        uint32_t idClient;
        rc = VbglR3HGCMConnect("VBoxSharedCrOpenGL", &idClient);
        if (RT_SUCCESS(rc))
        {
            conn->u32ClientID = idClient;
            crDebug("HGCM connect was successful: client id =0x%x\n", conn->u32ClientID);

            rc = crVBoxHGCMSetVersion(conn, CR_PROTOCOL_VERSION_MAJOR, CR_PROTOCOL_VERSION_MINOR);
            if (RT_FAILURE(rc))
            {
                WARN(("crVBoxHGCMSetVersion failed %d", rc));
/** @todo r=bird: Someone else messed up the return values here, not me.   */
                return rc;
            }
#ifdef RT_OS_WINDOWS
            rc = crVBoxHGCMSetPID(conn, GetCurrentProcessId());
#else
            rc = crVBoxHGCMSetPID(conn, crGetPID());
#endif
            if (RT_FAILURE(rc))
                WARN(("crVBoxHGCMSetPID failed %Rrc", rc));
/** @todo r=bird: Someone else messed up the return values here, not me.   */
            VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
            return rc;
        }

        crDebug("HGCM connect failed: %Rrc\n", rc);
        VbglR3Term();
    }
    else
        crDebug("Failed to initialize VbglR3 library: %Rrc\n", rc);

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
    return FALSE;

#else /*#ifdef IN_GUEST*/
    crError("crVBoxHGSMIDoConnect called on host side!");
    CRASSERT(FALSE);
    return FALSE;
#endif
}

/** @todo same, replace DeviceIoControl with vbglR3DoIOCtl */
static void crVBoxHGSMIDoDisconnect( CRConnection *conn )
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

    if (conn->pHostBuffer)
    {
        crFree(conn->pHostBuffer);
        conn->pHostBuffer = NULL;
        conn->cbHostBuffer = 0;
        conn->cbHostBufferAllocated = 0;
    }

    conn->pBuffer = NULL;
    conn->cbBuffer = 0;

    /// @todo hold lock here?
    if (conn->type == CR_VBOXHGCM)
    {
        --g_crvboxhgsmi.num_conns;

        if (conn->index < g_crvboxhgsmi.num_conns)
        {
            g_crvboxhgsmi.conns[conn->index] = g_crvboxhgsmi.conns[g_crvboxhgsmi.num_conns];
            g_crvboxhgsmi.conns[conn->index]->index = conn->index;
        }
        else g_crvboxhgsmi.conns[conn->index] = NULL;

        conn->type = CR_NO_CONNECTION;
    }

#ifndef IN_GUEST
#else /* IN_GUEST */
    if (conn->u32ClientID)
    {
        int rc = VbglR3HGCMDisconnect(conn->u32ClientID);
        if (RT_FAILURE(rc))
            crDebug("Disconnect failed with %Rrc\n", rc);
        conn->u32ClientID = 0;

        VbglR3Term();
    }

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
#endif /* IN_GUEST */
}

static void crVBoxHGSMIInstantReclaim(CRConnection *conn, CRMessage *mess)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    Assert(0);

    _crVBoxHGSMIFree(conn, mess);
    CRASSERT(FALSE);

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void crVBoxHGSMIHandleNewMessage( CRConnection *conn, CRMessage *msg, unsigned int len )
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    Assert(0);

    CRASSERT(FALSE);
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static DECLCALLBACK(HVBOXCRHGSMI_CLIENT) _crVBoxHGSMIClientCreate(PVBOXUHGSMI pHgsmi)
{
    PCRVBOXHGSMI_CLIENT pClient = crAlloc(sizeof (CRVBOXHGSMI_CLIENT));

    if (pClient)
    {
        int rc;
        pClient->pHgsmi = pHgsmi;
        rc = pHgsmi->pfnBufferCreate(pHgsmi, CRVBOXHGSMI_PAGE_ALIGN(1),
                                VBOXUHGSMI_SYNCHOBJECT_TYPE_EVENT,
                                NULL,
                                &pClient->pCmdBuffer);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            rc = pHgsmi->pfnBufferCreate(pHgsmi, CRVBOXHGSMI_PAGE_ALIGN(0x800000),
                                            VBOXUHGSMI_SYNCHOBJECT_TYPE_EVENT,
                                            NULL,
                                            &pClient->pHGBuffer);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                pClient->pvHGBuffer = NULL;
                pClient->bufpool = crBufferPoolInit(16);
                return (HVBOXCRHGSMI_CLIENT) pClient;
            }
        }
    }

    return NULL;
}

static DECLCALLBACK(void) _crVBoxHGSMIClientDestroy(HVBOXCRHGSMI_CLIENT hClient)
{
    Assert(0);

    /** @todo */
}


bool crVBoxHGSMIInit(CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl, unsigned int mtu)
{
    /* static */ int bHasHGSMI = -1; /* do it for all connections */
    (void) mtu;

    if (bHasHGSMI < 0)
    {
        int rc;
        VBOXCRHGSMI_CALLBACKS Callbacks;
        Callbacks.pfnClientCreate = _crVBoxHGSMIClientCreate;
        Callbacks.pfnClientDestroy = _crVBoxHGSMIClientDestroy;
        rc = VBoxCrHgsmiInit(&Callbacks);
        AssertRC(rc);
        if (RT_SUCCESS(rc))
            bHasHGSMI = 1;
        else
            bHasHGSMI = 0;
    }

    Assert(bHasHGSMI);

    if (!bHasHGSMI)
    {
#ifdef DEBUG_misha
        AssertRelease(0);
#endif
        return false;
    }

    g_crvboxhgsmi.recv_list = rfl;
    g_crvboxhgsmi.close_list = cfl;
    if (g_crvboxhgsmi.initialized)
    {
        return true;
    }

    g_crvboxhgsmi.initialized = 1;

    g_crvboxhgsmi.num_conns = 0;
    g_crvboxhgsmi.conns     = NULL;
    g_crvboxhgsmi.mempool = crBufferPoolInit(16);

    /* Can't open VBox guest driver here, because it gets called for host side as well */
    /** @todo as we have 2 dll versions, can do it now.*/

#ifdef RT_OS_WINDOWS
    g_crvboxhgsmi.hGuestDrv = INVALID_HANDLE_VALUE;
#else
    g_crvboxhgsmi.iGuestDrv = INVALID_HANDLE_VALUE;
#endif

#ifdef CHROMIUM_THREADSAFE
    crInitMutex(&g_crvboxhgsmi.mutex);
    crInitMutex(&g_crvboxhgsmi.recvmutex);
#endif

    return true;
}

/* Callback function used to free buffer pool entries */
void _crVBoxHGSMISysMemFree(void *data)
{
    Assert(0);

    crFree(data);
}

void crVBoxHGSMITearDown(void)
{
    int32_t i, cCons;

    Assert(0);

    if (!g_crvboxhgsmi.initialized) return;

    /* Connection count would be changed in calls to crNetDisconnect, so we have to store original value.
     * Walking array backwards is not a good idea as it could cause some issues if we'd disconnect clients not in the
     * order of their connection.
     */
    cCons = g_crvboxhgsmi.num_conns;
    for (i=0; i<cCons; i++)
    {
        /* Note that [0] is intended, as the connections array would be shifted in each call to crNetDisconnect */
        crNetDisconnect(g_crvboxhgsmi.conns[0]);
    }
    CRASSERT(0==g_crvboxhgsmi.num_conns);

#ifdef CHROMIUM_THREADSAFE
    crFreeMutex(&g_crvboxhgsmi.mutex);
    crFreeMutex(&g_crvboxhgsmi.recvmutex);
#endif

    if (g_crvboxhgsmi.mempool)
        crBufferPoolCallbackFree(g_crvboxhgsmi.mempool, _crVBoxHGSMISysMemFree);
    g_crvboxhgsmi.mempool = NULL;

    g_crvboxhgsmi.initialized = 0;

    crFree(g_crvboxhgsmi.conns);
    g_crvboxhgsmi.conns = NULL;
}

void crVBoxHGSMIConnection(CRConnection *conn)
{
    int i, found = 0;
    int n_bytes;

    CRASSERT(g_crvboxhgsmi.initialized);

    conn->type = CR_VBOXHGCM;
    conn->Alloc = crVBoxHGSMIAlloc;
    conn->Send = crVBoxHGSMISend;
    conn->SendExact = crVBoxHGSMIWriteExact;
    conn->Recv = crVBoxHGSMISingleRecv;
    conn->RecvMsg = crVBoxHGSMIReceiveMessage;
    conn->Free = crVBoxHGSMIFree;
    conn->Accept = crVBoxHGSMIAccept;
    conn->Connect = crVBoxHGSMIDoConnect;
    conn->Disconnect = crVBoxHGSMIDoDisconnect;
    conn->InstantReclaim = crVBoxHGSMIInstantReclaim;
    conn->HandleNewMessage = crVBoxHGSMIHandleNewMessage;
    conn->index = g_crvboxhgsmi.num_conns;
    conn->sizeof_buffer_header = sizeof(CRVBOXHGSMIBUFFER);
    conn->actual_network = 1;

    conn->krecv_buf_size = 0;

    conn->pBuffer = NULL;
    conn->cbBuffer = 0;
    conn->allow_redir_ptr = 1;

    /// @todo remove this crap at all later
    conn->cbHostBufferAllocated = 0;//2*1024;
    conn->pHostBuffer = NULL;//(uint8_t*) crAlloc(conn->cbHostBufferAllocated);
//    CRASSERT(conn->pHostBuffer);
    conn->cbHostBuffer = 0;

    /* Find a free slot */
    for (i = 0; i < g_crvboxhgsmi.num_conns; i++) {
        if (g_crvboxhgsmi.conns[i] == NULL) {
            conn->index = i;
            g_crvboxhgsmi.conns[i] = conn;
            found = 1;
            break;
        }
    }

    /* Realloc connection stack if we couldn't find a free slot */
    if (found == 0) {
        n_bytes = ( g_crvboxhgsmi.num_conns + 1 ) * sizeof(*g_crvboxhgsmi.conns);
        crRealloc( (void **) &g_crvboxhgsmi.conns, n_bytes );
        g_crvboxhgsmi.conns[g_crvboxhgsmi.num_conns++] = conn;
    }
}

int crVBoxHGSMIRecv(void)
{
    int32_t i;
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

#ifdef IN_GUEST
    /* we're on guest side, poll host if it got something for us */
    for (i=0; i<g_crvboxhgsmi.num_conns; i++)
    {
        CRConnection *conn = g_crvboxhgsmi.conns[i];

        if ( !conn || conn->type == CR_NO_CONNECTION )
            continue;

        if (!conn->pBuffer)
        {
            PCRVBOXHGSMI_CLIENT pClient = _crVBoxHGSMIClientGet();
            _crVBoxHGSMIPollHost(conn, pClient);
        }
    }
#endif

    for (i=0; i<g_crvboxhgsmi.num_conns; i++)
    {
        CRConnection *conn = g_crvboxhgsmi.conns[i];

        if ( !conn || conn->type == CR_NO_CONNECTION )
            continue;

        if (conn->cbBuffer>0)
        {
            PCRVBOXHGSMI_CLIENT pClient = _crVBoxHGSMIClientGet();
            _crVBoxHGSMIReceiveMessage(conn, pClient);
        }
    }

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
    return 0;
}

CRConnection** crVBoxHGSMIDump( int *num )
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    Assert(0);
    *num = g_crvboxhgsmi.num_conns;

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
    return g_crvboxhgsmi.conns;
}
#endif /* #ifdef IN_GUEST */
