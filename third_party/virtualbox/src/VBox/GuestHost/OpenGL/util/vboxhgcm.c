/* $Id: vboxhgcm.c $ */
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

#include <iprt/initterm.h>
#include <iprt/thread.h>

#include <VBox/VBoxGuestLib.h>
#include <VBox/HostServices/VBoxCrOpenGLSvc.h>

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
#include <VBoxCrHgsmi.h>
#endif

/** @todo move define somewhere else, and make sure it's less than VBGLR0_MAX_HGCM_KERNEL_PARM*/
/*If we fail to pass data in one chunk, send it in chunks of this size instead*/
#define CR_HGCM_SPLIT_BUFFER_SIZE (8*_1M)

#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef DEBUG_misha
#ifdef CRASSERT
# undef CRASSERT
#endif
#define CRASSERT Assert
#endif
/*#define IN_GUEST
#if defined(IN_GUEST)
#define VBOX_WITH_CRHGSMIPROFILE
#endif */
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
/*    bool bDisable;*/
} VBOXCRHGSMIPROFILE_SCOPE, *PVBOXCRHGSMIPROFILE_SCOPE;

static VBOXCRHGSMIPROFILE g_VBoxProfile;

static void vboxCrHgsmiLog(char * szString, ...)
{
    char szBuffer[4096] = {0};
     va_list pArgList;
     va_start(pArgList, szString);
     _vsnprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), szString, pArgList);
     va_end(pArgList);

#ifdef VBOX_WITH_CRHGSMI
     VBoxCrHgsmiLog(szBuffer);
#else
     OutputDebugString(szBuffer);
#endif
}

DECLINLINE(void) vboxCrHgsmiProfileLog(PVBOXCRHGSMIPROFILE pProfile, uint64_t cTime)
{
    uint64_t profileTime = cTime - pProfile->cStartTime;
    double percent = ((double)100.0) * pProfile->cStepsTime / profileTime;
    double cps = ((double)1000000000.) * pProfile->cSteps / profileTime;
    vboxCrHgsmiLog("hgcm: cps: %.1f, host %.1f%%\n", cps, percent);
}

DECLINLINE(void) vboxCrHgsmiProfileScopeEnter(PVBOXCRHGSMIPROFILE_SCOPE pScope)
{
/*    pScope->bDisable = false; */
    pScope->cStartTime = VBOXCRHGSMIPROFILE_GET_TIME_NANO();
}

DECLINLINE(void) vboxCrHgsmiProfileScopeExit(PVBOXCRHGSMIPROFILE_SCOPE pScope)
{
/*    if (!pScope->bDisable) */
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
    CRBufferPool         *bufpool;
#ifdef CHROMIUM_THREADSAFE
    CRmutex              mutex;
    CRmutex              recvmutex;
#endif
    CRNetReceiveFuncList *recv_list;
    CRNetCloseFuncList   *close_list;
#ifdef RT_OS_WINDOWS
    LPDIRECTDRAW         pDirectDraw;
#endif
#ifdef IN_GUEST
    uint32_t             u32HostCaps;
    bool                 fHostCapsInitialized;
#endif
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    bool bHgsmiOn;
#endif
} CRVBOXHGCMDATA;

static CRVBOXHGCMDATA g_crvboxhgcm = {0,};

typedef enum {
    CR_VBOXHGCM_USERALLOCATED,
    CR_VBOXHGCM_MEMORY,
    CR_VBOXHGCM_MEMORY_BIG
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    ,CR_VBOXHGCM_UHGSMI_BUFFER
#endif
} CRVBOXHGCMBUFFERKIND;

#define CR_VBOXHGCM_BUFFER_MAGIC 0xABCDE321

typedef struct CRVBOXHGCMBUFFER {
    uint32_t             magic;
    CRVBOXHGCMBUFFERKIND kind;
    union
    {
        struct
        {
            uint32_t             len;
            uint32_t             allocated;
        };

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        PVBOXUHGSMI_BUFFER pBuffer;
#endif
    };
} CRVBOXHGCMBUFFER;

#ifndef RT_OS_WINDOWS
    #define TRUE true
    #define FALSE false
    #define INVALID_HANDLE_VALUE (-1)
#endif


#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)

/* add sizeof header + page align */
#define CRVBOXHGSMI_PAGE_ALIGN(_s) (((_s)  + 0xfff) & ~0xfff)
#define CRVBOXHGSMI_BUF_HDR_SIZE() (sizeof (CRVBOXHGCMBUFFER))
#define CRVBOXHGSMI_BUF_SIZE(_s) CRVBOXHGSMI_PAGE_ALIGN((_s) + CRVBOXHGSMI_BUF_HDR_SIZE())
#define CRVBOXHGSMI_BUF_LOCK_SIZE(_s) ((_s) + CRVBOXHGSMI_BUF_HDR_SIZE())
#define CRVBOXHGSMI_BUF_DATA(_p) ((void*)(((CRVBOXHGCMBUFFER*)(_p)) + 1))
#define CRVBOXHGSMI_BUF_HDR(_p) (((CRVBOXHGCMBUFFER*)(_p)) - 1)
#define CRVBOXHGSMI_BUF_OFFSET(_st2, _st1) ((uint32_t)(((uint8_t*)(_st2)) - ((uint8_t*)(_st1))))

static int _crVBoxHGSMIClientInit(PCRVBOXHGSMI_CLIENT pClient, PVBOXUHGSMI pHgsmi)
{
    int rc;
    VBOXUHGSMI_BUFFER_TYPE_FLAGS Flags = {0};
    pClient->pHgsmi = pHgsmi;
    Flags.fCommand = 1;
    rc = pHgsmi->pfnBufferCreate(pHgsmi, CRVBOXHGSMI_PAGE_ALIGN(1), Flags, &pClient->pCmdBuffer);
    if (RT_SUCCESS(rc))
    {
        Flags.Value = 0;
        rc = pHgsmi->pfnBufferCreate(pHgsmi, CRVBOXHGSMI_PAGE_ALIGN(1), Flags, &pClient->pHGBuffer);
        if (RT_SUCCESS(rc))
        {
            pClient->pvHGBuffer = NULL;
            pClient->bufpool = crBufferPoolInit(16);
            return VINF_SUCCESS;
        }
        else
            crWarning("_crVBoxHGSMIClientInit: pfnBufferCreate failed to allocate host->guest buffer");

        pClient->pCmdBuffer->pfnDestroy(pClient->pCmdBuffer);
    }
    else
        crWarning("_crVBoxHGSMIClientInit: pfnBufferCreate failed to allocate cmd buffer");

    pClient->pHgsmi = NULL;
    return rc;
}

void _crVBoxHGSMIBufferFree(void *data)
{
    PVBOXUHGSMI_BUFFER pBuffer = (PVBOXUHGSMI_BUFFER)data;
    pBuffer->pfnDestroy(pBuffer);
}

static int _crVBoxHGSMIClientTerm(PCRVBOXHGSMI_CLIENT pClient, PVBOXUHGSMI *ppHgsmi)
{
    if (pClient->bufpool)
        crBufferPoolCallbackFree(pClient->bufpool, _crVBoxHGSMIBufferFree);
    pClient->bufpool = NULL;

    if (pClient->pHGBuffer)
    {
        pClient->pHGBuffer->pfnDestroy(pClient->pHGBuffer);
        pClient->pHGBuffer = NULL;
    }

    if (pClient->pCmdBuffer)
    {
        pClient->pCmdBuffer->pfnDestroy(pClient->pCmdBuffer);
        pClient->pCmdBuffer = NULL;
    }

    if (ppHgsmi)
    {
        *ppHgsmi = pClient->pHgsmi;
    }
    pClient->pHgsmi = NULL;

    return VINF_SUCCESS;
}


#ifdef VBOX_CRHGSMI_WITH_D3DDEV

static DECLCALLBACK(HVBOXCRHGSMI_CLIENT) _crVBoxHGSMIClientCreate(PVBOXUHGSMI pHgsmi)
{
    PCRVBOXHGSMI_CLIENT pClient = crAlloc(sizeof (CRVBOXHGSMI_CLIENT));

    if (pClient)
    {
        int rc = _crVBoxHGSMIClientInit(pClient, pHgsmi);
        if (RT_SUCCESS(rc))
            return (HVBOXCRHGSMI_CLIENT)pClient;
        else
            crWarning("_crVBoxHGSMIClientCreate: _crVBoxHGSMIClientInit failed rc %d", rc);

        crFree(pCLient);
    }

    return NULL;
}

static DECLCALLBACK(void) _crVBoxHGSMIClientDestroy(HVBOXCRHGSMI_CLIENT hClient)
{
    PCRVBOXHGSMI_CLIENT pClient = (PCRVBOXHGSMI_CLIENT)hClient;
    _crVBoxHGSMIClientTerm(pClient, NULL);
    crFree(pClient);
}
#endif

DECLINLINE(PCRVBOXHGSMI_CLIENT) _crVBoxHGSMIClientGet(CRConnection *conn)
{
#ifdef VBOX_CRHGSMI_WITH_D3DDEV
    PCRVBOXHGSMI_CLIENT pClient = (PCRVBOXHGSMI_CLIENT)VBoxCrHgsmiQueryClient();
    CRASSERT(pClient);
    return pClient;
#else
    if (conn->HgsmiClient.pHgsmi)
        return &conn->HgsmiClient;
    {
        PVBOXUHGSMI pHgsmi = conn->pExternalHgsmi ? conn->pExternalHgsmi : VBoxCrHgsmiCreate();
        if (pHgsmi)
        {
            int rc = _crVBoxHGSMIClientInit(&conn->HgsmiClient, pHgsmi);
            if (RT_SUCCESS(rc))
            {
                CRASSERT(conn->HgsmiClient.pHgsmi);
                return &conn->HgsmiClient;
            }
            else
                crWarning("_crVBoxHGSMIClientGet: _crVBoxHGSMIClientInit failed rc %d", rc);
            if (!conn->pExternalHgsmi)
                VBoxCrHgsmiDestroy(pHgsmi);
        }
        else
        {
            crWarning("VBoxCrHgsmiCreate failed");
        }
    }
    return NULL;
#endif
}

static PVBOXUHGSMI_BUFFER _crVBoxHGSMIBufAlloc(PCRVBOXHGSMI_CLIENT pClient, uint32_t cbSize)
{
    PVBOXUHGSMI_BUFFER buf;
    int rc;

    buf = (PVBOXUHGSMI_BUFFER ) crBufferPoolPop(pClient->bufpool, cbSize);

    if (!buf)
    {
        VBOXUHGSMI_BUFFER_TYPE_FLAGS Flags = {0};
        crDebug("Buffer pool %p was empty; allocating new %d byte buffer.",
                        (void *) pClient->bufpool,
                        cbSize);
        rc = pClient->pHgsmi->pfnBufferCreate(pClient->pHgsmi, cbSize, Flags, &buf);
        if (RT_FAILURE(rc))
            crWarning("_crVBoxHGSMIBufAlloc: Failed to Create a buffer of size(%d), rc(%d)\n", cbSize, rc);
    }
    return buf;
}

static PVBOXUHGSMI_BUFFER _crVBoxHGSMIBufFromHdr(CRVBOXHGCMBUFFER *pHdr)
{
    PVBOXUHGSMI_BUFFER pBuf;
    int rc;
    CRASSERT(pHdr->magic == CR_VBOXHGCM_BUFFER_MAGIC);
    CRASSERT(pHdr->kind == CR_VBOXHGCM_UHGSMI_BUFFER);
    pBuf = pHdr->pBuffer;
    rc = pBuf->pfnUnlock(pBuf);
    if (RT_FAILURE(rc))
    {
        crWarning("_crVBoxHGSMIBufFromHdr: pfnUnlock failed rc %d", rc);
        return NULL;
    }
    return pBuf;
}

static void _crVBoxHGSMIBufFree(PCRVBOXHGSMI_CLIENT pClient, PVBOXUHGSMI_BUFFER pBuf)
{
    crBufferPoolPush(pClient->bufpool, pBuf, pBuf->cbBuffer);
}

static CRVBOXHGSMIHDR *_crVBoxHGSMICmdBufferLock(PCRVBOXHGSMI_CLIENT pClient, uint32_t cbBuffer)
{
    /* in theory it is OK to use one cmd buffer for asynch cmd submission
     * because bDiscard flag should result in allocating a new memory backend if the
     * allocation is still in use.
     * However, NOTE: since one and the same semaphore synch event is used for completion notification,
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
    if (RT_SUCCESS(rc))
        return pHdr;
    else
        crWarning("_crVBoxHGSMICmdBufferLock: pfnLock failed rc %d", rc);

    crWarning("Failed to Lock the command buffer of size(%d), rc(%d)\n", cbBuffer, rc);
    return NULL;
}

static CRVBOXHGSMIHDR *_crVBoxHGSMICmdBufferLockRo(PCRVBOXHGSMI_CLIENT pClient, uint32_t cbBuffer)
{
    /* in theory it is OK to use one cmd buffer for asynch cmd submission
     * because bDiscard flag should result in allocating a new memory backend if the
     * allocation is still in use.
     * However, NOTE: since one and the same semaphore synch event is used for completion notification,
     * for the notification mechanism working as expected
     * 1. host must complete commands in the same order as it receives them
     * (to avoid situation when guest receives notification for another command completion)
     * 2. guest must eventually wait for command completion unless he specified bDoNotSignalCompletion
     * 3. guest must wait for command completion in the same order as it submits them
     * in case we can not satisfy any of the above, we should introduce multiple command buffers */
    CRVBOXHGSMIHDR * pHdr = NULL;
    VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags;
    int rc;
    fFlags.Value = 0;
    fFlags.bReadOnly = 1;
    rc = pClient->pCmdBuffer->pfnLock(pClient->pCmdBuffer, 0, cbBuffer, fFlags, (void**)&pHdr);
    if (RT_FAILURE(rc))
        crWarning("Failed to Lock the command buffer of size(%d), rc(%d)\n", cbBuffer, rc);
    return pHdr;
}

static void _crVBoxHGSMICmdBufferUnlock(PCRVBOXHGSMI_CLIENT pClient)
{
    int rc = pClient->pCmdBuffer->pfnUnlock(pClient->pCmdBuffer);
    if (RT_FAILURE(rc))
        crError("Failed to Unlock the command buffer rc(%d)\n", rc);
}

static int32_t _crVBoxHGSMICmdBufferGetRc(PCRVBOXHGSMI_CLIENT pClient)
{
    CRVBOXHGSMIHDR * pHdr;
    VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags;
    int rc;

    fFlags.Value = 0;
    fFlags.bReadOnly = 1;
    rc = pClient->pCmdBuffer->pfnLock(pClient->pCmdBuffer, 0, sizeof (*pHdr), fFlags, (void**)&pHdr);
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
    CRASSERT(!pClient->pvHGBuffer);
    fFlags.Value = 0;
    rc = pClient->pHGBuffer->pfnLock(pClient->pHGBuffer, 0, cbBuffer, fFlags, &pClient->pvHGBuffer);
    if (RT_SUCCESS(rc))
        return pClient->pvHGBuffer;
    else
        crWarning("_crVBoxHGSMIRecvBufData: pfnLock failed rc %d", rc);

    return NULL;
}

DECLINLINE(void) _crVBoxHGSMIFillCmd(VBOXUHGSMI_BUFFER_SUBMIT *pSubm, PCRVBOXHGSMI_CLIENT pClient, uint32_t cbData)
{
    pSubm->pBuf = pClient->pCmdBuffer;
    pSubm->offData = 0;
    pSubm->cbData = cbData;
    pSubm->fFlags.Value = 0;
    pSubm->fFlags.bDoNotRetire = 1;
# if 0
    pSubm->fFlags.bDoNotSignalCompletion = 1; /* <- we do not need that actually since
                                               * in case we want completion,
                                               * we will block in _crVBoxHGSMICmdBufferGetRc (when locking the buffer)
                                               * which is needed for getting the result */
# endif
}
#endif

/* Some forward declarations */
static void _crVBoxHGCMReceiveMessage(CRConnection *conn);

#ifndef IN_GUEST
static bool _crVBoxHGCMReadBytes(CRConnection *conn, void *buf, uint32_t len)
{
    CRASSERT(conn && buf);

    if (!conn->pBuffer || (conn->cbBuffer<len))
        return FALSE;

    crMemcpy(buf, conn->pBuffer, len);

    conn->cbBuffer -= len;
    conn->pBuffer = conn->cbBuffer>0 ? (uint8_t*)conn->pBuffer+len : NULL;

    return TRUE;
}
#endif

#ifndef IN_GUEST
/** @todo get rid of it*/
static bool _crVBoxHGCMWriteBytes(CRConnection *conn, const void *buf, uint32_t len)
{
    CRASSERT(conn && buf);

    /* make sure there's host buffer and it's clear */
    CRASSERT(conn->pHostBuffer && !conn->cbHostBuffer);

    if (conn->cbHostBufferAllocated < len)
    {
        crDebug("Host buffer too small %d out of requested %d bytes, reallocating", conn->cbHostBufferAllocated, len);
        crFree(conn->pHostBuffer);
        conn->pHostBuffer = crAlloc(len);
        if (!conn->pHostBuffer)
        {
            conn->cbHostBufferAllocated = 0;
            crError("OUT_OF_MEMORY trying to allocate %d bytes", len);
            return FALSE;
        }
        conn->cbHostBufferAllocated = len;
    }

    crMemcpy(conn->pHostBuffer, buf, len);
    conn->cbHostBuffer = len;

    return TRUE;
}
#endif

/**
 * Send an HGCM request
 *
 * @return VBox status code
 * @param   pvData      Data pointer
 * @param   cbData      Data size
 */
static int crVBoxHGCMCall(CRConnection *conn, PVBGLIOCHGCMCALL pData, unsigned cbData)
{
#ifdef IN_GUEST
    int rc;
# ifndef VBOX_WITH_CRHGSMI
    RT_NOREF(conn);
# else
    PCRVBOXHGSMI_CLIENT pClient = g_crvboxhgcm.bHgsmiOn ? _crVBoxHGSMIClientGet(conn) : NULL;
    if (pClient)
        rc = VBoxCrHgsmiCtlConCall(pClient->pHgsmi, pData, cbData);
    else
# endif
    {
        rc = VbglR3HGCMCall(pData, cbData);
        if (RT_SUCCESS(rc))
        { /* likely */ }
        else
        {
            crWarning("vboxCall failed with VBox status code %Rrc\n", rc);
# ifndef RT_OS_WINDOWS
            if (rc == VERR_INTERRUPTED)
            {
                /* Not sure why we're doing the sleep stuff here.  The original coder didn't
                   bother to mention why he thought it necessary.  :-( */
                RTMSINTERVAL msSleep;
                int i;
                for (i = 0, msSleep = 50; i < 6; i++, msSleep = msSleep * 2)
                {
                    RTThreadSleep(msSleep);
                    rc = VbglR3HGCMCall(pData, cbData);
                    if (rc != VERR_INTERRUPTED)
                    {
                        if (RT_SUCCESS(rc))
                            crWarning("vboxCall retry(%i) succeeded", i + 1);
                        else
                            crWarning("vboxCall retry(%i) failed with VBox status code %Rrc", i + 1, rc);
                        break;
                    }
                }
            }
#  endif
        }
    }
    return rc;

#else  /* IN_GUEST */
    RT_NOREF(conn, pData, cbData);
    crError("crVBoxHGCMCall called on host side!");
    CRASSERT(FALSE);
    return VERR_NOT_SUPPORTED;
#endif /* IN_GUEST */
}

static void *_crVBoxHGCMAlloc(CRConnection *conn)
{
    CRVBOXHGCMBUFFER *buf;

#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif

    buf = (CRVBOXHGCMBUFFER *) crBufferPoolPop(g_crvboxhgcm.bufpool, conn->buffer_size);

    if (!buf)
    {
        crDebug("Buffer pool %p was empty; allocating new %d byte buffer.",
                        (void *) g_crvboxhgcm.bufpool,
                        (unsigned int)sizeof(CRVBOXHGCMBUFFER) + conn->buffer_size);

        /* We're either on host side, or we failed to allocate DDRAW buffer */
        if (!buf)
        {
            crDebug("Using system malloc\n");
            buf = (CRVBOXHGCMBUFFER *) crAlloc( sizeof(CRVBOXHGCMBUFFER) + conn->buffer_size );
            CRASSERT(buf);
            buf->magic = CR_VBOXHGCM_BUFFER_MAGIC;
            buf->kind  = CR_VBOXHGCM_MEMORY;
            buf->allocated = conn->buffer_size;
        }
    }

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif

    return (void *)( buf + 1 );

}

static void *crVBoxHGCMAlloc(CRConnection *conn)
{
    void *pvBuff;
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif
    pvBuff = _crVBoxHGCMAlloc(conn);
#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
    return pvBuff;
}

static void _crVBoxHGCMWriteExact(CRConnection *conn, const void *buf, unsigned int len)
{
    int rc;
    int32_t callRes;

#ifdef IN_GUEST
    if (conn->u32InjectClientID)
    {
        CRVBOXHGCMINJECT parms;

        VBGL_HGCM_HDR_INIT(&parms.hdr, conn->u32ClientID, SHCRGL_GUEST_FN_INJECT, SHCRGL_CPARMS_INJECT);

        parms.u32ClientID.type       = VMMDevHGCMParmType_32bit;
        parms.u32ClientID.u.value32  = conn->u32InjectClientID;

        parms.pBuffer.type                   = VMMDevHGCMParmType_LinAddr_In;
        parms.pBuffer.u.Pointer.size         = len;
        parms.pBuffer.u.Pointer.u.linearAddr = (uintptr_t) buf;

        rc = crVBoxHGCMCall(conn, &parms.hdr, sizeof(parms));
        callRes = parms.hdr.Hdr.rc; /** @todo now rc == parms.hdr.Hdr.rc */
    }
    else
#endif
    {
        CRVBOXHGCMWRITE parms;

        VBGL_HGCM_HDR_INIT(&parms.hdr, conn->u32ClientID, SHCRGL_GUEST_FN_WRITE, SHCRGL_CPARMS_WRITE);

        parms.pBuffer.type                   = VMMDevHGCMParmType_LinAddr_In;
        parms.pBuffer.u.Pointer.size         = len;
        parms.pBuffer.u.Pointer.u.linearAddr = (uintptr_t) buf;

        rc = crVBoxHGCMCall(conn, &parms.hdr, sizeof(parms));
        callRes = parms.hdr.Hdr.rc; /** @todo now rc == parms.hdr.Hdr.rc */
    }

    if (RT_FAILURE(rc) || RT_FAILURE(callRes))
    {
        crWarning("SHCRGL_GUEST_FN_WRITE failed with %x %x\n", rc, callRes);
    }
}

static void crVBoxHGCMWriteExact(CRConnection *conn, const void *buf, unsigned int len)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif
    _crVBoxHGCMWriteExact(conn, buf, len);
#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void crVBoxHGCMReadExact( CRConnection *conn, const void *buf, unsigned int len )
{
    CRVBOXHGCMREAD parms;
    int rc;
    RT_NOREF(buf, len);

    VBGL_HGCM_HDR_INIT(&parms.hdr, conn->u32ClientID, SHCRGL_GUEST_FN_READ, SHCRGL_CPARMS_READ);

    CRASSERT(!conn->pBuffer); /* make sure there's no data to process */
    parms.pBuffer.type                   = VMMDevHGCMParmType_LinAddr_Out;
    parms.pBuffer.u.Pointer.size         = conn->cbHostBufferAllocated;
    parms.pBuffer.u.Pointer.u.linearAddr = (uintptr_t) conn->pHostBuffer;

    parms.cbBuffer.type      = VMMDevHGCMParmType_32bit;
    parms.cbBuffer.u.value32 = 0;

    rc = crVBoxHGCMCall(conn, &parms.hdr, sizeof(parms));

    if (RT_FAILURE(rc) || RT_FAILURE(parms.hdr.Hdr.rc) /** @todo now rc == parms.hdr.Hdr.rc */)
    {
        crWarning("SHCRGL_GUEST_FN_READ failed with %x %x\n", rc, parms.hdr.Hdr.rc);
        return;
    }

    if (parms.cbBuffer.u.value32)
    {
        /*conn->pBuffer  = (uint8_t*) parms.pBuffer.u.Pointer.u.linearAddr; */
        conn->pBuffer  = conn->pHostBuffer;
        conn->cbBuffer = parms.cbBuffer.u.value32;
    }

    if (conn->cbBuffer)
        _crVBoxHGCMReceiveMessage(conn);

}

/* Same as crVBoxHGCMWriteExact, but combined with read of writeback data.
 * This halves the number of HGCM calls we do,
 * most likely crVBoxHGCMPollHost shouldn't be called at all now.
 */
static void
crVBoxHGCMWriteReadExact(CRConnection *conn, const void *buf, unsigned int len, CRVBOXHGCMBUFFERKIND bufferKind)
{
    CRVBOXHGCMWRITEREAD parms;
    int rc;

    VBGL_HGCM_HDR_INIT(&parms.hdr, conn->u32ClientID, SHCRGL_GUEST_FN_WRITE_READ, SHCRGL_CPARMS_WRITE_READ);

    parms.pBuffer.type                   = VMMDevHGCMParmType_LinAddr_In;
    parms.pBuffer.u.Pointer.size         = len;
    parms.pBuffer.u.Pointer.u.linearAddr = (uintptr_t) buf;

    CRASSERT(!conn->pBuffer); /*make sure there's no data to process*/
    parms.pWriteback.type                   = VMMDevHGCMParmType_LinAddr_Out;
    parms.pWriteback.u.Pointer.size         = conn->cbHostBufferAllocated;
    parms.pWriteback.u.Pointer.u.linearAddr = (uintptr_t) conn->pHostBuffer;

    parms.cbWriteback.type      = VMMDevHGCMParmType_32bit;
    parms.cbWriteback.u.value32 = 0;

    rc = crVBoxHGCMCall(conn, &parms.hdr, sizeof(parms));

#if defined(RT_OS_LINUX) || defined(RT_OS_WINDOWS)
    if (VERR_OUT_OF_RANGE==rc && CR_VBOXHGCM_USERALLOCATED==bufferKind)
    {
        /*Buffer is too big, so send it in split chunks*/
        CRVBOXHGCMWRITEBUFFER wbParms;

        VBGL_HGCM_HDR_INIT(&wbParms.hdr, conn->u32ClientID, SHCRGL_GUEST_FN_WRITE_BUFFER, SHCRGL_CPARMS_WRITE_BUFFER);

        wbParms.iBufferID.type = VMMDevHGCMParmType_32bit;
        wbParms.iBufferID.u.value32 = 0;

        wbParms.cbBufferSize.type = VMMDevHGCMParmType_32bit;
        wbParms.cbBufferSize.u.value32 = len;

        wbParms.ui32Offset.type = VMMDevHGCMParmType_32bit;
        wbParms.ui32Offset.u.value32 = 0;

        wbParms.pBuffer.type = VMMDevHGCMParmType_LinAddr_In;
        wbParms.pBuffer.u.Pointer.size         = MIN(CR_HGCM_SPLIT_BUFFER_SIZE, len);
        wbParms.pBuffer.u.Pointer.u.linearAddr = (uintptr_t) buf;

        if (len<CR_HGCM_SPLIT_BUFFER_SIZE)
        {
            crError("VERR_OUT_OF_RANGE in crVBoxHGCMWriteReadExact for %u bytes write", len);
            return;
        }

        while (wbParms.pBuffer.u.Pointer.size)
        {
            crDebug("SHCRGL_GUEST_FN_WRITE_BUFFER, offset=%u, size=%u", wbParms.ui32Offset.u.value32, wbParms.pBuffer.u.Pointer.size);

            rc = crVBoxHGCMCall(conn, &wbParms.hdr, sizeof(wbParms));
            if (RT_FAILURE(rc) || RT_FAILURE(wbParms.hdr.Hdr.rc) /** @todo now rc == wbParms.hdr.Hdr.rc */)
            {
                crError("SHCRGL_GUEST_FN_WRITE_BUFFER (%i) failed with %x %x\n", wbParms.pBuffer.u.Pointer.size, rc, wbParms.hdr.Hdr.rc);
                return;
            }

            wbParms.ui32Offset.u.value32 += wbParms.pBuffer.u.Pointer.size;
            wbParms.pBuffer.u.Pointer.u.linearAddr += (uintptr_t) wbParms.pBuffer.u.Pointer.size;
            wbParms.pBuffer.u.Pointer.size = MIN(CR_HGCM_SPLIT_BUFFER_SIZE, len-wbParms.ui32Offset.u.value32);
        }

        /*now issue GUEST_FN_WRITE_READ_BUFFERED referencing the buffer we'd made*/
        {
            CRVBOXHGCMWRITEREADBUFFERED wrbParms;

            VBGL_HGCM_HDR_INIT(&wrbParms.hdr, conn->u32ClientID, SHCRGL_GUEST_FN_WRITE_READ_BUFFERED, SHCRGL_CPARMS_WRITE_READ_BUFFERED);

            crMemcpy(&wrbParms.iBufferID, &wbParms.iBufferID, sizeof(HGCMFunctionParameter));
            crMemcpy(&wrbParms.pWriteback, &parms.pWriteback, sizeof(HGCMFunctionParameter));
            crMemcpy(&wrbParms.cbWriteback, &parms.cbWriteback, sizeof(HGCMFunctionParameter));

            rc = crVBoxHGCMCall(conn, &wrbParms.hdr, sizeof(wrbParms));

            /*bit of hack to reuse code below*/
            parms.hdr.Hdr.rc = wrbParms.hdr.Hdr.rc;
            crMemcpy(&parms.cbWriteback, &wrbParms.cbWriteback, sizeof(HGCMFunctionParameter));
            crMemcpy(&parms.pWriteback, &wrbParms.pWriteback, sizeof(HGCMFunctionParameter));
        }
    }
#endif

    if (RT_FAILURE(rc) || RT_FAILURE(parms.hdr.Hdr.rc) /** @todo now rc == parms.hdr.Hdr.rc */)
    {

        if ((VERR_BUFFER_OVERFLOW == parms.hdr.Hdr.rc) /* && RT_SUCCESS(rc) */)
        {
            /* reallocate buffer and retry */

            CRASSERT(parms.cbWriteback.u.value32>conn->cbHostBufferAllocated);

            crDebug("Reallocating host buffer from %d to %d bytes", conn->cbHostBufferAllocated, parms.cbWriteback.u.value32);

            crFree(conn->pHostBuffer);
            conn->cbHostBufferAllocated = parms.cbWriteback.u.value32;
            conn->pHostBuffer = crAlloc(conn->cbHostBufferAllocated);

            crVBoxHGCMReadExact(conn, buf, len);

            return;
        }
        else
        {
            crWarning("SHCRGL_GUEST_FN_WRITE_READ (%i) failed with %x %x\n", len, rc, parms.hdr.Hdr.rc);
            return;
        }
    }

    if (parms.cbWriteback.u.value32)
    {
        /*conn->pBuffer  = (uint8_t*) parms.pWriteback.u.Pointer.u.linearAddr;*/
        conn->pBuffer  = conn->pHostBuffer;
        conn->cbBuffer = parms.cbWriteback.u.value32;
    }

    if (conn->cbBuffer)
        _crVBoxHGCMReceiveMessage(conn);
}

static void crVBoxHGCMSend(CRConnection *conn, void **bufp,
                           const void *start, unsigned int len)
{
    CRVBOXHGCMBUFFER *hgcm_buffer;
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif

    if (!bufp) /* We're sending a user-allocated buffer. */
    {
#ifndef IN_GUEST
            /** @todo remove temp buffer allocation in unpacker*/
            /* we're at the host side, so just store data until guest polls us */
            _crVBoxHGCMWriteBytes(conn, start, len);
#else
            CRASSERT(!conn->u32InjectClientID);
            crDebug("SHCRGL: sending userbuf with %d bytes\n", len);
            crVBoxHGCMWriteReadExact(conn, start, len, CR_VBOXHGCM_USERALLOCATED);
#endif
#ifdef CHROMIUM_THREADSAFE
            crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
            VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
        return;
    }

    /* The region [start .. start + len + 1] lies within a buffer that
     * was allocated with crVBoxHGCMAlloc() and can be put into the free
     * buffer pool when we're done sending it.
     */

    hgcm_buffer = (CRVBOXHGCMBUFFER *)(*bufp) - 1;
    CRASSERT(hgcm_buffer->magic == CR_VBOXHGCM_BUFFER_MAGIC);

    /* Length would be passed as part of HGCM pointer description
     * No need to prepend it to the buffer
     */
#ifdef IN_GUEST
    if (conn->u32InjectClientID)
    {
        _crVBoxHGCMWriteExact(conn, start, len);
    }
    else
#endif
    crVBoxHGCMWriteReadExact(conn, start, len, hgcm_buffer->kind);

    /* Reclaim this pointer for reuse */
#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif
    crBufferPoolPush(g_crvboxhgcm.bufpool, hgcm_buffer, hgcm_buffer->allocated);
#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif

    /* Since the buffer's now in the 'free' buffer pool, the caller can't
     * use it any more.  Setting bufp to NULL will make sure the caller
     * doesn't try to re-use the buffer.
     */
    *bufp = NULL;

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void crVBoxHGCMPollHost(CRConnection *conn)
{
    CRVBOXHGCMREAD parms;
    int rc;

    CRASSERT(!conn->pBuffer);

    VBGL_HGCM_HDR_INIT(&parms.hdr, conn->u32ClientID, SHCRGL_GUEST_FN_READ, SHCRGL_CPARMS_READ);

    parms.pBuffer.type                   = VMMDevHGCMParmType_LinAddr_Out;
    parms.pBuffer.u.Pointer.size         = conn->cbHostBufferAllocated;
    parms.pBuffer.u.Pointer.u.linearAddr = (uintptr_t) conn->pHostBuffer;

    parms.cbBuffer.type      = VMMDevHGCMParmType_32bit;
    parms.cbBuffer.u.value32 = 0;

    rc = crVBoxHGCMCall(conn, &parms.hdr, sizeof(parms));

    if (RT_FAILURE(rc) || RT_FAILURE(parms.hdr.Hdr.rc) /** @todo now rc == parms.hdr.Hdr.rc */)
    {
        crDebug("SHCRGL_GUEST_FN_READ failed with %x %x\n", rc, parms.hdr.Hdr.rc);
        return;
    }

    if (parms.cbBuffer.u.value32)
    {
        conn->pBuffer = (uint8_t*) parms.pBuffer.u.Pointer.u.linearAddr;
        conn->cbBuffer = parms.cbBuffer.u.value32;
    }
}

static void crVBoxHGCMSingleRecv(CRConnection *conn, void *buf, unsigned int len)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif
    crVBoxHGCMReadExact(conn, buf, len);
#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void _crVBoxHGCMFree(CRConnection *conn, void *buf)
{
    CRVBOXHGCMBUFFER *hgcm_buffer = (CRVBOXHGCMBUFFER *) buf - 1;

    CRASSERT(hgcm_buffer->magic == CR_VBOXHGCM_BUFFER_MAGIC);

    /** @todo wrong len for redir buffers*/
    conn->recv_credits += hgcm_buffer->len;

    switch (hgcm_buffer->kind)
    {
        case CR_VBOXHGCM_MEMORY:
#ifdef CHROMIUM_THREADSAFE
            crLockMutex(&g_crvboxhgcm.mutex);
#endif
            if (g_crvboxhgcm.bufpool) {
                /** @todo o'rly? */
                /* pool may have been deallocated just a bit earlier in response
                 * to a SIGPIPE (Broken Pipe) signal.
                 */
                crBufferPoolPush(g_crvboxhgcm.bufpool, hgcm_buffer, hgcm_buffer->allocated);
            }
#ifdef CHROMIUM_THREADSAFE
            crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
            break;

        case CR_VBOXHGCM_MEMORY_BIG:
            crFree( hgcm_buffer );
            break;

        default:
            crError( "Weird buffer kind trying to free in crVBoxHGCMFree: %d", hgcm_buffer->kind );
    }
}

static void crVBoxHGCMFree(CRConnection *conn, void *buf)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif
    _crVBoxHGCMFree(conn, buf);
#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void _crVBoxHGCMReceiveMessage(CRConnection *conn)
{
    uint32_t len;
    CRVBOXHGCMBUFFER *hgcm_buffer;
    CRMessage *msg;
    CRMessageType cached_type;

    len = conn->cbBuffer;
    CRASSERT(len > 0);
    CRASSERT(conn->pBuffer);

#ifndef IN_GUEST
    if (conn->allow_redir_ptr)
    {
#endif
        CRASSERT(conn->buffer_size >= sizeof(CRMessageRedirPtr));

        hgcm_buffer = (CRVBOXHGCMBUFFER *) _crVBoxHGCMAlloc( conn ) - 1;
        hgcm_buffer->len = sizeof(CRMessageRedirPtr);

        msg = (CRMessage *) (hgcm_buffer + 1);

        msg->header.type = CR_MESSAGE_REDIR_PTR;
        msg->redirptr.pMessage = (CRMessageHeader*) (conn->pBuffer);
        msg->header.conn_id = msg->redirptr.pMessage->conn_id;

#if defined(VBOX_WITH_CRHGSMI) && !defined(IN_GUEST)
        msg->redirptr.CmdData = conn->CmdData;
        CRVBOXHGSMI_CMDDATA_ASSERT_CONSISTENT(&msg->redirptr.CmdData);
        CRVBOXHGSMI_CMDDATA_CLEANUP(&conn->CmdData);
#endif

        cached_type = msg->redirptr.pMessage->type;

        conn->cbBuffer = 0;
        conn->pBuffer  = NULL;
#ifndef IN_GUEST
    }
    else
    {
        /* we should NEVER have redir_ptr disabled with HGSMI command now */
        CRASSERT(!conn->CmdData.pvCmd);
        if ( len <= conn->buffer_size )
        {
            /* put in pre-allocated buffer */
            hgcm_buffer = (CRVBOXHGCMBUFFER *) _crVBoxHGCMAlloc( conn ) - 1;
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
        }

        hgcm_buffer->len = len;
        _crVBoxHGCMReadBytes(conn, hgcm_buffer + 1, len);

        msg = (CRMessage *) (hgcm_buffer + 1);
        cached_type = msg->header.type;
    }
#endif /* !IN_GUEST*/

    conn->recv_credits     -= len;
    conn->total_bytes_recv += len;
    conn->recv_count++;

    crNetDispatchMessage( g_crvboxhgcm.recv_list, conn, msg, len );

    /* CR_MESSAGE_OPCODES is freed in crserverlib/server_stream.c with crNetFree.
     * OOB messages are the programmer's problem.  -- Humper 12/17/01
     */
    if (cached_type != CR_MESSAGE_OPCODES
        && cached_type != CR_MESSAGE_OOB
        && cached_type != CR_MESSAGE_GATHER)
    {
        _crVBoxHGCMFree(conn, msg);
    }
}

static void crVBoxHGCMReceiveMessage(CRConnection *conn)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif
    _crVBoxHGCMReceiveMessage(conn);
#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}


/*
 * Called on host side only, to "accept" client connection
 */
static void crVBoxHGCMAccept( CRConnection *conn, const char *hostname, unsigned short port )
{
    RT_NOREF(hostname, port);
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    CRASSERT(conn && conn->pHostBuffer);
#ifdef IN_GUEST
    CRASSERT(FALSE);
#endif
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static int crVBoxHGCMSetVersion(CRConnection *conn, unsigned int vMajor, unsigned int vMinor)
{
    CRVBOXHGCMSETVERSION parms;
    int rc;
    RT_NOREF(vMajor, vMinor);

    VBGL_HGCM_HDR_INIT(&parms.hdr, conn->u32ClientID, SHCRGL_GUEST_FN_SET_VERSION, SHCRGL_CPARMS_SET_VERSION);

    parms.vMajor.type      = VMMDevHGCMParmType_32bit;
    parms.vMajor.u.value32 = CR_PROTOCOL_VERSION_MAJOR;
    parms.vMinor.type      = VMMDevHGCMParmType_32bit;
    parms.vMinor.u.value32 = CR_PROTOCOL_VERSION_MINOR;

    rc = crVBoxHGCMCall(conn, &parms.hdr, sizeof(parms));

    if (RT_SUCCESS(rc))
    {
        rc =  parms.hdr.Hdr.rc; /** @todo now rc == parms.hdr.Hdr.rc */
        if (RT_SUCCESS(rc))
        {
            conn->vMajor = CR_PROTOCOL_VERSION_MAJOR;
            conn->vMinor = CR_PROTOCOL_VERSION_MINOR;

            return VINF_SUCCESS;
        }
        else
            WARN(("Host doesn't accept our version %d.%d. Make sure you have appropriate additions installed!",
                  parms.vMajor.u.value32, parms.vMinor.u.value32));
    }
    else
        WARN(("crVBoxHGCMCall failed %d", rc));

    return rc;
}

static int crVBoxHGCMGetHostCapsLegacy(CRConnection *conn, uint32_t *pu32HostCaps)
{
    CRVBOXHGCMGETCAPS caps;
    int rc;

    VBGL_HGCM_HDR_INIT(&caps.hdr, conn->u32ClientID, SHCRGL_GUEST_FN_GET_CAPS_LEGACY, SHCRGL_CPARMS_GET_CAPS_LEGACY);

    caps.Caps.type       = VMMDevHGCMParmType_32bit;
    caps.Caps.u.value32  = 0;

    rc = crVBoxHGCMCall(conn, &caps.hdr, sizeof(caps));

    if (RT_SUCCESS(rc))
    {
        rc = caps.hdr.Hdr.rc;
        if (RT_SUCCESS(rc))
        {
            *pu32HostCaps = caps.Caps.u.value32;
            return VINF_SUCCESS;
        }
        else
            WARN(("SHCRGL_GUEST_FN_GET_CAPS failed %d", rc));
        return FALSE;
    }
    else
        WARN(("crVBoxHGCMCall failed %d", rc));

    *pu32HostCaps = 0;

    return rc;
}

static int crVBoxHGCMSetPID(CRConnection *conn, unsigned long long pid)
{
    CRVBOXHGCMSETPID parms;
    int rc;

    VBGL_HGCM_HDR_INIT(&parms.hdr, conn->u32ClientID, SHCRGL_GUEST_FN_SET_PID, SHCRGL_CPARMS_SET_PID);

    parms.u64PID.type      = VMMDevHGCMParmType_64bit;
    parms.u64PID.u.value64 = pid;

    rc = crVBoxHGCMCall(conn, &parms.hdr, sizeof(parms));

    if (RT_FAILURE(rc) || RT_FAILURE(parms.hdr.Hdr.rc) /** @todo now rc == parms.hdr.Hdr.rc */)
    {
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
static int crVBoxHGCMDoConnect( CRConnection *conn )
{
#ifdef IN_GUEST
    int rc;
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    RTR3InitDll(RTR3INIT_FLAGS_UNOBTRUSIVE);
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
                return FALSE;
            }
#ifdef RT_OS_WINDOWS
            rc = crVBoxHGCMSetPID(conn, GetCurrentProcessId());
#else
            rc = crVBoxHGCMSetPID(conn, crGetPID());
#endif
            if (RT_FAILURE(rc))
            {
                WARN(("crVBoxHGCMSetPID failed %Rrc", rc));
                return FALSE;
            }

            if (!g_crvboxhgcm.fHostCapsInitialized)
            {
                rc = crVBoxHGCMGetHostCapsLegacy(conn, &g_crvboxhgcm.u32HostCaps);
                if (RT_FAILURE(rc))
                {
                    WARN(("VBoxCrHgsmiCtlConGetHostCaps failed %Rrc", rc));
                    g_crvboxhgcm.u32HostCaps = 0;
                }

                /* host may not support it, ignore any failures */
                g_crvboxhgcm.fHostCapsInitialized = true;
            }

            if (g_crvboxhgcm.u32HostCaps & CR_VBOX_CAP_HOST_CAPS_NOT_SUFFICIENT)
            {
                crDebug("HGCM connect: insufficient host capabilities\n");
                g_crvboxhgcm.u32HostCaps = 0;
                return FALSE;
            }

            VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
            return TRUE;
        }

        crDebug("HGCM connect failed: %Rrc\n", rc);
        VbglR3Term();
    }
    else
        crDebug("Failed to initialize VbglR3 library: %Rrc\n", rc);

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
    return FALSE;

#else  /* !IN_GUEST */
    crError("crVBoxHGCMDoConnect called on host side!");
    CRASSERT(FALSE);
    return FALSE;
#endif /* !IN_GUEST */
}

static bool _crVBoxCommonDoDisconnectLocked( CRConnection *conn )
{
    int i;
    if (conn->pHostBuffer)
    {
        crFree(conn->pHostBuffer);
        conn->pHostBuffer = NULL;
        conn->cbHostBuffer = 0;
        conn->cbHostBufferAllocated = 0;
    }

    conn->pBuffer = NULL;
    conn->cbBuffer = 0;

    if (conn->type == CR_VBOXHGCM)
    {
        --g_crvboxhgcm.num_conns;

        if (conn->index < g_crvboxhgcm.num_conns)
        {
            g_crvboxhgcm.conns[conn->index] = g_crvboxhgcm.conns[g_crvboxhgcm.num_conns];
            g_crvboxhgcm.conns[conn->index]->index = conn->index;
        }
        else g_crvboxhgcm.conns[conn->index] = NULL;

        conn->type = CR_NO_CONNECTION;
    }

    for (i = 0; i < g_crvboxhgcm.num_conns; i++)
        if (g_crvboxhgcm.conns[i] && g_crvboxhgcm.conns[i]->type != CR_NO_CONNECTION)
            return true;
    return false;
}

/** @todo same, replace DeviceIoControl with vbglR3DoIOCtl */
static void crVBoxHGCMDoDisconnect( CRConnection *conn )
{
    bool fHasActiveCons = false;

    if (!g_crvboxhgcm.initialized) return;

#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif

    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

    fHasActiveCons = _crVBoxCommonDoDisconnectLocked(conn);

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
#endif /* IN_GUEST */

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
}

static void crVBoxHGCMInstantReclaim(CRConnection *conn, CRMessage *mess)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif
    _crVBoxHGCMFree(conn, mess);
    CRASSERT(FALSE);
#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void crVBoxHGCMHandleNewMessage( CRConnection *conn, CRMessage *msg, unsigned int len )
{
    RT_NOREF(conn, msg, len);
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    CRASSERT(FALSE);
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)

bool _crVBoxHGSMIInit()
{
#ifndef VBOX_CRHGSMI_WITH_D3DDEV
    static
#endif
    int bHasHGSMI = -1;

    if (bHasHGSMI < 0)
    {
        int rc;
        rc = VBoxCrHgsmiInit();
        if (RT_SUCCESS(rc))
            bHasHGSMI = 1;
        else
            bHasHGSMI = 0;

        crDebug("CrHgsmi is %s", bHasHGSMI ? "ENABLED" : "DISABLED");
    }

    CRASSERT(bHasHGSMI >= 0);

    return bHasHGSMI;
}

void _crVBoxHGSMITearDown()
{
    VBoxCrHgsmiTerm();
}

static void *_crVBoxHGSMIDoAlloc(CRConnection *conn, PCRVBOXHGSMI_CLIENT pClient)
{
    PVBOXUHGSMI_BUFFER buf;
    CRVBOXHGCMBUFFER *pData = NULL;
    uint32_t cbSize = conn->buffer_size;
    int rc;

    buf = _crVBoxHGSMIBufAlloc(pClient, CRVBOXHGSMI_BUF_SIZE(cbSize));
    if (buf)
    {
        VBOXUHGSMI_BUFFER_LOCK_FLAGS fFlags;
        buf->pvUserData = pClient;
        fFlags.Value = 0;
        fFlags.bDiscard = 1;
        rc = buf->pfnLock(buf, 0, CRVBOXHGSMI_BUF_LOCK_SIZE(cbSize), fFlags, (void**)&pData);
        if (RT_SUCCESS(rc))
        {
            pData->magic = CR_VBOXHGCM_BUFFER_MAGIC;
            pData->kind = CR_VBOXHGCM_UHGSMI_BUFFER;
            pData->pBuffer = buf;
        }
        else
        {
            crWarning("Failed to Lock the buffer, rc(%d)\n", rc);
        }
        return CRVBOXHGSMI_BUF_DATA(pData);
    }
    else
    {
        crWarning("_crVBoxHGSMIBufAlloc failed to allocate buffer of size (%d)", CRVBOXHGSMI_BUF_SIZE(cbSize));
    }

    /* fall back */
    return _crVBoxHGCMAlloc(conn);
}

static void _crVBoxHGSMIFree(CRConnection *conn, void *buf)
{
    CRVBOXHGCMBUFFER *hgcm_buffer = (CRVBOXHGCMBUFFER *) buf - 1;

    CRASSERT(hgcm_buffer->magic == CR_VBOXHGCM_BUFFER_MAGIC);

    if (hgcm_buffer->kind == CR_VBOXHGCM_UHGSMI_BUFFER)
    {
        PVBOXUHGSMI_BUFFER pBuf = _crVBoxHGSMIBufFromHdr(hgcm_buffer);
        PCRVBOXHGSMI_CLIENT pClient = (PCRVBOXHGSMI_CLIENT)pBuf->pvUserData;
        _crVBoxHGSMIBufFree(pClient, pBuf);
    }
    else
    {
        _crVBoxHGCMFree(conn, buf);
    }
}

static void *crVBoxHGSMIAlloc(CRConnection *conn)
{
    PCRVBOXHGSMI_CLIENT pClient;
    void *pvBuf;

    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif

    pClient = _crVBoxHGSMIClientGet(conn);
    if (pClient)
    {
        pvBuf = _crVBoxHGSMIDoAlloc(conn, pClient);
        CRASSERT(pvBuf);
    }
    else
    {
        pvBuf = _crVBoxHGCMAlloc(conn);
    }

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();

    return pvBuf;
}

static void crVBoxHGSMIFree(CRConnection *conn, void *buf)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif
    _crVBoxHGSMIFree(conn, buf);
#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void _crVBoxHGSMIPollHost(CRConnection *conn, PCRVBOXHGSMI_CLIENT pClient)
{
    CRVBOXHGSMIREAD *parms = (CRVBOXHGSMIREAD *)_crVBoxHGSMICmdBufferLock(pClient, sizeof (*parms));
    int rc;
    VBOXUHGSMI_BUFFER_SUBMIT aSubmit[2];
    PVBOXUHGSMI_BUFFER pRecvBuffer;
    uint32_t cbBuffer;

    CRASSERT(parms);

    parms->hdr.result      = VERR_WRONG_ORDER;
    parms->hdr.u32ClientID = conn->u32ClientID;
    parms->hdr.u32Function = SHCRGL_GUEST_FN_READ;
/*    parms->hdr.u32Reserved = 0;*/

    CRASSERT(!conn->pBuffer); /* make sure there's no data to process */
    parms->iBuffer = 1;
    parms->cbBuffer = 0;

    _crVBoxHGSMICmdBufferUnlock(pClient);

    pRecvBuffer = _crVBoxHGSMIRecvBufGet(pClient);
    CRASSERT(pRecvBuffer);
    if (!pRecvBuffer)
        return;

    _crVBoxHGSMIFillCmd(&aSubmit[0], pClient, sizeof (*parms));

    aSubmit[1].pBuf = pRecvBuffer;
    aSubmit[1].offData = 0;
    aSubmit[1].cbData = pRecvBuffer->cbBuffer;
    aSubmit[1].fFlags.Value = 0;
    aSubmit[1].fFlags.bHostWriteOnly = 1;

    rc = pClient->pHgsmi->pfnBufferSubmit(pClient->pHgsmi, aSubmit, 2);
    if (RT_FAILURE(rc))
    {
        crError("pfnBufferSubmit failed with %d \n", rc);
        return;
    }

    parms = (CRVBOXHGSMIREAD *)_crVBoxHGSMICmdBufferLockRo(pClient, sizeof (*parms));
    CRASSERT(parms);
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
        CRASSERT(pvData);
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
        _crVBoxHGCMReceiveMessage(conn);
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
/*    uint32_t cbBuffer;*/

    parms->hdr.result      = VERR_WRONG_ORDER;
    parms->hdr.u32ClientID = conn->u32ClientID;
    parms->hdr.u32Function = SHCRGL_GUEST_FN_WRITE_READ;
/*    parms->hdr.u32Reserved = 0;*/

    parms->iBuffer = 1;

    CRASSERT(!conn->pBuffer); /* make sure there's no data to process */
    parms->iWriteback = 2;
    parms->cbWriteback = 0;

    _crVBoxHGSMICmdBufferUnlock(pClient);

    if (!bIsBuffer)
    {
        void *pvBuf;
        pBuf = _crVBoxHGSMIBufAlloc(pClient, len);

        if (!pBuf)
        {
            /* fallback */
            crVBoxHGCMWriteReadExact(conn, buf, len, CR_VBOXHGCM_USERALLOCATED);
            return;
        }

        CRASSERT(!offBuffer);

        offBuffer = 0;
        fFlags.Value = 0;
        fFlags.bDiscard = 1;
        fFlags.bWriteOnly = 1;
        rc = pBuf->pfnLock(pBuf, 0, len, fFlags, &pvBuf);
        if (RT_SUCCESS(rc))
        {
            memcpy(pvBuf, buf, len);
            rc = pBuf->pfnUnlock(pBuf);
            CRASSERT(RT_SUCCESS(rc));
        }
        else
        {
            crWarning("_crVBoxHGSMIWriteReadExact: pfnUnlock failed rc %d", rc);
            _crVBoxHGSMIBufFree(pClient, pBuf);
            /* fallback */
            crVBoxHGCMWriteReadExact(conn, buf, len, CR_VBOXHGCM_USERALLOCATED);
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
        CRASSERT(pRecvBuffer);
        if (!pRecvBuffer)
        {
            break;
        }

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

        rc = pClient->pHgsmi->pfnBufferSubmit(pClient->pHgsmi, aSubmit, 3);
        if (RT_FAILURE(rc))
        {
            crError("pfnBufferSubmit failed with %d \n", rc);
            break;
        }

        parms = (CRVBOXHGSMIWRITEREAD *)_crVBoxHGSMICmdBufferLockRo(pClient, sizeof (*parms));
        CRASSERT(parms);
        if (parms)
        {
            uint32_t cbWriteback = parms->cbWriteback;
            rc = parms->hdr.result;
#ifdef DEBUG_misha
            /* catch it here to test the buffer */
            Assert(RT_SUCCESS(parms->hdr.Hdr.rc) || parms->hdr.Hdr.rc == VERR_BUFFER_OVERFLOW);
#endif
            _crVBoxHGSMICmdBufferUnlock(pClient);
#ifdef DEBUG
            parms = NULL;
#endif
            if (RT_SUCCESS(rc))
            {
                if (cbWriteback)
                {
                    void *pvData = _crVBoxHGSMIRecvBufData(pClient, cbWriteback);
                    CRASSERT(pvData);
                    if (pvData)
                    {
                        conn->pBuffer  = pvData;
                        conn->cbBuffer = cbWriteback;
                        _crVBoxHGCMReceiveMessage(conn);
                    }
                }
            }
            else if (VERR_BUFFER_OVERFLOW == rc)
            {
                VBOXUHGSMI_BUFFER_TYPE_FLAGS Flags = {0};
                PVBOXUHGSMI_BUFFER pNewBuf;
                CRASSERT(!pClient->pvHGBuffer);
                CRASSERT(cbWriteback>pClient->pHGBuffer->cbBuffer);
                crDebug("Reallocating host buffer from %d to %d bytes", conn->cbHostBufferAllocated, cbWriteback);

                rc = pClient->pHgsmi->pfnBufferCreate(pClient->pHgsmi, CRVBOXHGSMI_PAGE_ALIGN(cbWriteback), Flags, &pNewBuf);
                if (RT_SUCCESS(rc))
                {
                    rc = pClient->pHGBuffer->pfnDestroy(pClient->pHGBuffer);
                    CRASSERT(RT_SUCCESS(rc));

                    pClient->pHGBuffer = pNewBuf;

                    _crVBoxHGSMIReadExact(conn, pClient/*, cbWriteback*/);
                }
                else
                {
                    crWarning("_crVBoxHGSMIWriteReadExact: pfnBufferCreate(%d) failed!", CRVBOXHGSMI_PAGE_ALIGN(cbWriteback));
                    if (conn->cbHostBufferAllocated < cbWriteback)
                    {
                        crFree(conn->pHostBuffer);
                        conn->cbHostBufferAllocated = cbWriteback;
                        conn->pHostBuffer = crAlloc(conn->cbHostBufferAllocated);
                    }
                    crVBoxHGCMReadExact(conn, NULL, cbWriteback);
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

    return;
}

static void _crVBoxHGSMIWriteExact(CRConnection *conn, PCRVBOXHGSMI_CLIENT pClient, PVBOXUHGSMI_BUFFER pBuf, uint32_t offStart, unsigned int len)
{
    int rc;
    int32_t callRes = VINF_SUCCESS; /* Shut up MSC. */
    VBOXUHGSMI_BUFFER_SUBMIT aSubmit[2];

#ifdef IN_GUEST
    if (conn->u32InjectClientID)
    {
        CRVBOXHGSMIINJECT *parms = (CRVBOXHGSMIINJECT *)_crVBoxHGSMICmdBufferLock(pClient, sizeof (*parms));
        CRASSERT(parms);
        if (!parms)
        {
            return;
        }

        parms->hdr.result      = VERR_WRONG_ORDER;
        parms->hdr.u32ClientID = conn->u32ClientID;
        parms->hdr.u32Function = SHCRGL_GUEST_FN_INJECT;
/*        parms->hdr.u32Reserved = 0;*/

        parms->u32ClientID = conn->u32InjectClientID;

        parms->iBuffer = 1;
        _crVBoxHGSMICmdBufferUnlock(pClient);

        _crVBoxHGSMIFillCmd(&aSubmit[0], pClient, sizeof (*parms));

        aSubmit[1].pBuf = pBuf;
        aSubmit[1].offData = offStart;
        aSubmit[1].cbData = len;
        aSubmit[1].fFlags.Value = 0;
        aSubmit[1].fFlags.bHostReadOnly = 1;

        rc = pClient->pHgsmi->pfnBufferSubmit(pClient->pHgsmi, aSubmit, 2);
        if (RT_SUCCESS(rc))
        {
            callRes = _crVBoxHGSMICmdBufferGetRc(pClient);
        }
        else
        {
            /* we can not recover at this point, report error & exit */
            crError("pfnBufferSubmit failed with %d \n", rc);
        }
    }
    else
#endif
    {
        CRVBOXHGSMIWRITE * parms = (CRVBOXHGSMIWRITE *)_crVBoxHGSMICmdBufferLock(pClient, sizeof (*parms));;

        parms->hdr.result      = VERR_WRONG_ORDER;
        parms->hdr.u32ClientID = conn->u32ClientID;
        parms->hdr.u32Function = SHCRGL_GUEST_FN_WRITE;
/*        parms->hdr.u32Reserved = 0; */

        parms->iBuffer = 1;
        _crVBoxHGSMICmdBufferUnlock(pClient);

        _crVBoxHGSMIFillCmd(&aSubmit[0], pClient, sizeof (*parms));

        aSubmit[1].pBuf = pBuf;
        aSubmit[1].offData = offStart;
        aSubmit[1].cbData = len;
        aSubmit[1].fFlags.Value = 0;
        aSubmit[1].fFlags.bHostReadOnly = 1;

        rc = pClient->pHgsmi->pfnBufferSubmit(pClient->pHgsmi, aSubmit, 2);
        if (RT_SUCCESS(rc))
        {
            callRes = _crVBoxHGSMICmdBufferGetRc(pClient);
        }
        else
        {
            /* we can not recover at this point, report error & exit */
            crError("Failed to submit CrHhgsmi buffer");
        }
    }

    if (RT_FAILURE(rc) || RT_FAILURE(callRes))
    {
        crWarning("SHCRGL_GUEST_FN_WRITE failed with %x %x\n", rc, callRes);
    }
}

static void crVBoxHGSMISend(CRConnection *conn, void **bufp,
                           const void *start, unsigned int len)
{
    PCRVBOXHGSMI_CLIENT pClient;
    PVBOXUHGSMI_BUFFER pBuf;
    CRVBOXHGCMBUFFER *hgcm_buffer;

    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif

    if (!bufp) /* We're sending a user-allocated buffer. */
    {
        pClient = _crVBoxHGSMIClientGet(conn);
        if (pClient)
        {
#ifndef IN_GUEST
                /** @todo remove temp buffer allocation in unpacker */
                /* we're at the host side, so just store data until guest polls us */
                _crVBoxHGCMWriteBytes(conn, start, len);
#else
            CRASSERT(!conn->u32InjectClientID);
            crDebug("SHCRGL: sending userbuf with %d bytes\n", len);
            _crVBoxHGSMIWriteReadExact(conn, pClient, (void*)start, 0, len, false);
#endif
#ifdef CHROMIUM_THREADSAFE
            crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
            VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
            return;
        }

        /* fallback */
        crVBoxHGCMSend(conn, bufp, start, len);
#ifdef CHROMIUM_THREADSAFE
        crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
        VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
        return;
    }

    hgcm_buffer = (CRVBOXHGCMBUFFER *) *bufp - 1;
    CRASSERT(hgcm_buffer->magic == CR_VBOXHGCM_BUFFER_MAGIC);
    if (hgcm_buffer->magic != CR_VBOXHGCM_BUFFER_MAGIC)
    {
        crError("HGCM buffer magic mismatch");
    }


    if (hgcm_buffer->kind != CR_VBOXHGCM_UHGSMI_BUFFER)
    {
        /* fallback */
        crVBoxHGCMSend(conn, bufp, start, len);
#ifdef CHROMIUM_THREADSAFE
        crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
        VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
        return;
    }

    /* The region [start .. start + len + 1] lies within a buffer that
     * was allocated with crVBoxHGCMAlloc() and can be put into the free
     * buffer pool when we're done sending it.
     */

    pBuf = _crVBoxHGSMIBufFromHdr(hgcm_buffer);
    CRASSERT(pBuf);
    if (!pBuf)
    {
        crVBoxHGCMSend(conn, bufp, start, len);
#ifdef CHROMIUM_THREADSAFE
        crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
        VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
        return;
    }

    pClient = (PCRVBOXHGSMI_CLIENT)pBuf->pvUserData;
    if (pClient != &conn->HgsmiClient)
    {
        crError("HGSMI client mismatch");
    }

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

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void crVBoxHGSMIWriteExact(CRConnection *conn, const void *buf, unsigned int len)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

    CRASSERT(0);

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void crVBoxHGSMISingleRecv(CRConnection *conn, void *buf, unsigned int len)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

    CRASSERT(0);

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void crVBoxHGSMIReceiveMessage(CRConnection *conn)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif

    CRASSERT(0);

    _crVBoxHGCMReceiveMessage(conn);

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

/*
 * Called on host side only, to "accept" client connection
 */
static void crVBoxHGSMIAccept( CRConnection *conn, const char *hostname, unsigned short port )
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    CRASSERT(0);

    CRASSERT(conn && conn->pHostBuffer);
#ifdef IN_GUEST
    CRASSERT(FALSE);
#endif
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static int crVBoxHGSMIDoConnect( CRConnection *conn )
{
    PCRVBOXHGSMI_CLIENT pClient;
    int rc = VINF_SUCCESS;

#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif

    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

    pClient = _crVBoxHGSMIClientGet(conn);
    if (pClient)
    {
        rc = VBoxCrHgsmiCtlConGetClientID(pClient->pHgsmi, &conn->u32ClientID);
        if (RT_FAILURE(rc))
        {
            WARN(("VBoxCrHgsmiCtlConGetClientID failed %d", rc));
        }
        if (!g_crvboxhgcm.fHostCapsInitialized)
        {
            rc = VBoxCrHgsmiCtlConGetHostCaps(pClient->pHgsmi, &g_crvboxhgcm.u32HostCaps);
            if (RT_SUCCESS(rc))
            {
                g_crvboxhgcm.fHostCapsInitialized = true;
            }
            else
            {
                WARN(("VBoxCrHgsmiCtlConGetHostCaps failed %d", rc));
                g_crvboxhgcm.u32HostCaps = 0;
            }
        }
    }
    else
    {
        WARN(("_crVBoxHGSMIClientGet failed %d", rc));
        rc = VERR_GENERAL_FAILURE;
    }

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
    return RT_SUCCESS(rc);
}

static void crVBoxHGSMIDoDisconnect( CRConnection *conn )
{
    bool fHasActiveCons = false;

    if (!g_crvboxhgcm.initialized) return;

    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif

    fHasActiveCons = _crVBoxCommonDoDisconnectLocked(conn);

#ifndef VBOX_CRHGSMI_WITH_D3DDEV
    if (conn->HgsmiClient.pHgsmi)
    {
        PVBOXUHGSMI pHgsmi;
        _crVBoxHGSMIClientTerm(&conn->HgsmiClient, &pHgsmi);
        CRASSERT(pHgsmi);
        if (!conn->pExternalHgsmi)
            VBoxCrHgsmiDestroy(pHgsmi);
    }
#else
# error "port me!"
#endif

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
}

static void crVBoxHGSMIInstantReclaim(CRConnection *conn, CRMessage *mess)
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif
    CRASSERT(0);

    _crVBoxHGSMIFree(conn, mess);

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}

static void crVBoxHGSMIHandleNewMessage( CRConnection *conn, CRMessage *msg, unsigned int len )
{
    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();
    CRASSERT(0);

    CRASSERT(FALSE);
    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
}
#endif

void crVBoxHGCMInit(CRNetReceiveFuncList *rfl, CRNetCloseFuncList *cfl, unsigned int mtu)
{
    (void) mtu;

    g_crvboxhgcm.recv_list = rfl;
    g_crvboxhgcm.close_list = cfl;
    if (g_crvboxhgcm.initialized)
    {
        return;
    }

    VBOXCRHGSMIPROFILE_INIT();

    g_crvboxhgcm.initialized = 1;

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    g_crvboxhgcm.bHgsmiOn = _crVBoxHGSMIInit();
#endif

    g_crvboxhgcm.num_conns = 0;
    g_crvboxhgcm.conns     = NULL;

    /* Can't open VBox guest driver here, because it gets called for host side as well */
    /** @todo as we have 2 dll versions, can do it now.*/

#ifdef RT_OS_WINDOWS
    g_crvboxhgcm.pDirectDraw = NULL;
#endif

#ifdef CHROMIUM_THREADSAFE
    crInitMutex(&g_crvboxhgcm.mutex);
    crInitMutex(&g_crvboxhgcm.recvmutex);
#endif
    g_crvboxhgcm.bufpool = crBufferPoolInit(16);

#ifdef IN_GUEST
    g_crvboxhgcm.fHostCapsInitialized = false;
    g_crvboxhgcm.u32HostCaps = 0;
#endif
}

/* Callback function used to free buffer pool entries */
static void crVBoxHGCMBufferFree(void *data)
{
    CRVBOXHGCMBUFFER *hgcm_buffer = (CRVBOXHGCMBUFFER *) data;

    CRASSERT(hgcm_buffer->magic == CR_VBOXHGCM_BUFFER_MAGIC);

    switch (hgcm_buffer->kind)
    {
        case CR_VBOXHGCM_MEMORY:
            crDebug("crVBoxHGCMBufferFree: CR_VBOXHGCM_MEMORY: %p", hgcm_buffer);
            crFree( hgcm_buffer );
            break;
        case CR_VBOXHGCM_MEMORY_BIG:
            crDebug("crVBoxHGCMBufferFree: CR_VBOXHGCM_MEMORY_BIG: %p", hgcm_buffer);
            crFree( hgcm_buffer );
            break;

        default:
            crError( "Weird buffer kind trying to free in crVBoxHGCMBufferFree: %d", hgcm_buffer->kind );
    }
}

void crVBoxHGCMTearDown(void)
{
    int32_t i, cCons;

    if (!g_crvboxhgcm.initialized) return;

#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif

    /* Connection count would be changed in calls to crNetDisconnect, so we have to store original value.
     * Walking array backwards is not a good idea as it could cause some issues if we'd disconnect clients not in the
     * order of their connection.
     */
    cCons = g_crvboxhgcm.num_conns;
    for (i=0; i<cCons; i++)
    {
        /* Note that [0] is intended, as the connections array would be shifted in each call to crNetDisconnect */
        crNetDisconnect(g_crvboxhgcm.conns[0]);
    }
    CRASSERT(0==g_crvboxhgcm.num_conns);

    g_crvboxhgcm.initialized = 0;

    if (g_crvboxhgcm.bufpool)
        crBufferPoolCallbackFree(g_crvboxhgcm.bufpool, crVBoxHGCMBufferFree);
    g_crvboxhgcm.bufpool = NULL;

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
    crFreeMutex(&g_crvboxhgcm.mutex);
    crFreeMutex(&g_crvboxhgcm.recvmutex);
#endif

    crFree(g_crvboxhgcm.conns);
    g_crvboxhgcm.conns = NULL;

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    if (g_crvboxhgcm.bHgsmiOn)
    {
        _crVBoxHGSMITearDown();
    }
#endif

#ifdef RT_OS_WINDOWS
    if (g_crvboxhgcm.pDirectDraw)
    {
        IDirectDraw_Release(g_crvboxhgcm.pDirectDraw);
        g_crvboxhgcm.pDirectDraw = NULL;
        crDebug("DirectDraw released\n");
    }
#endif
}

void crVBoxHGCMConnection(CRConnection *conn
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        , struct VBOXUHGSMI *pHgsmi
#endif
        )
{
    int i, found = 0;
    int n_bytes;

    CRASSERT(g_crvboxhgcm.initialized);

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    if (g_crvboxhgcm.bHgsmiOn)
    {
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
        conn->pExternalHgsmi = pHgsmi;
    }
    else
#endif
    {
        conn->type = CR_VBOXHGCM;
        conn->Alloc = crVBoxHGCMAlloc;
        conn->Send = crVBoxHGCMSend;
        conn->SendExact = crVBoxHGCMWriteExact;
        conn->Recv = crVBoxHGCMSingleRecv;
        conn->RecvMsg = crVBoxHGCMReceiveMessage;
        conn->Free = crVBoxHGCMFree;
        conn->Accept = crVBoxHGCMAccept;
        conn->Connect = crVBoxHGCMDoConnect;
        conn->Disconnect = crVBoxHGCMDoDisconnect;
        conn->InstantReclaim = crVBoxHGCMInstantReclaim;
        conn->HandleNewMessage = crVBoxHGCMHandleNewMessage;
    }
    conn->sizeof_buffer_header = sizeof(CRVBOXHGCMBUFFER);
    conn->actual_network = 1;

    conn->krecv_buf_size = 0;

    conn->pBuffer = NULL;
    conn->cbBuffer = 0;
    conn->allow_redir_ptr = 1;

    /** @todo remove this crap at all later */
    conn->cbHostBufferAllocated = 2*1024;
    conn->pHostBuffer = (uint8_t*) crAlloc(conn->cbHostBufferAllocated);
    CRASSERT(conn->pHostBuffer);
    conn->cbHostBuffer = 0;

#if !defined(IN_GUEST)
    RTListInit(&conn->PendingMsgList);
#endif

#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif
    /* Find a free slot */
    for (i = 0; i < g_crvboxhgcm.num_conns; i++) {
        if (g_crvboxhgcm.conns[i] == NULL) {
            conn->index = i;
            g_crvboxhgcm.conns[i] = conn;
            found = 1;
            break;
        }
    }

    /* Realloc connection stack if we couldn't find a free slot */
    if (found == 0) {
        n_bytes = ( g_crvboxhgcm.num_conns + 1 ) * sizeof(*g_crvboxhgcm.conns);
        crRealloc( (void **) &g_crvboxhgcm.conns, n_bytes );
        conn->index = g_crvboxhgcm.num_conns;
        g_crvboxhgcm.conns[g_crvboxhgcm.num_conns++] = conn;
    }
#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif
}

#if defined(IN_GUEST)
static void _crVBoxHGCMPerformPollHost(CRConnection *conn)
{
    if (conn->type == CR_NO_CONNECTION )
        return;

    if (!conn->pBuffer)
    {
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        PCRVBOXHGSMI_CLIENT pClient;
        if (g_crvboxhgcm.bHgsmiOn && !!(pClient = _crVBoxHGSMIClientGet(conn)))
        {
            _crVBoxHGSMIPollHost(conn, pClient);
        }
        else
#endif
        {
            crVBoxHGCMPollHost(conn);
        }
    }
}
#endif

static void _crVBoxHGCMPerformReceiveMessage(CRConnection *conn)
{
    if ( conn->type == CR_NO_CONNECTION )
        return;

    if (conn->cbBuffer>0)
    {
        _crVBoxHGCMReceiveMessage(conn);
    }
}

#ifdef IN_GUEST
uint32_t crVBoxHGCMHostCapsGet(void)
{
    Assert(g_crvboxhgcm.fHostCapsInitialized);
    return g_crvboxhgcm.u32HostCaps;
}
#endif

int crVBoxHGCMRecv(
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        CRConnection *conn
#else
        void
#endif
        )
{
    int32_t i;

    VBOXCRHGSMIPROFILE_FUNC_PROLOGUE();

#ifdef CHROMIUM_THREADSAFE
    crLockMutex(&g_crvboxhgcm.mutex);
#endif

#ifdef IN_GUEST
# if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    if (conn && g_crvboxhgcm.bHgsmiOn)
    {
        _crVBoxHGCMPerformPollHost(conn);
        _crVBoxHGCMPerformReceiveMessage(conn);
        VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();
        return 0;
    }
# endif
    /* we're on guest side, poll host if it got something for us */
    for (i=0; i<g_crvboxhgcm.num_conns; i++)
    {
        CRConnection *conn = g_crvboxhgcm.conns[i];

        if ( !conn  )
            continue;

        _crVBoxHGCMPerformPollHost(conn);
    }
#endif

    for (i=0; i<g_crvboxhgcm.num_conns; i++)
    {
        CRConnection *conn = g_crvboxhgcm.conns[i];

        if ( !conn )
            continue;

        _crVBoxHGCMPerformReceiveMessage(conn);
    }

#ifdef CHROMIUM_THREADSAFE
    crUnlockMutex(&g_crvboxhgcm.mutex);
#endif

    VBOXCRHGSMIPROFILE_FUNC_EPILOGUE();

    return 0;
}

CRConnection** crVBoxHGCMDump( int *num )
{
    *num = g_crvboxhgcm.num_conns;

    return g_crvboxhgcm.conns;
}
