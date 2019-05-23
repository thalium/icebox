/* $Id: crservice.cpp $ */
/** @file
 * VBox crOpenGL - Host service entry points.
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
 */

#define __STDC_CONSTANT_MACROS  /* needed for a definition in iprt/string.h */

#define LOG_GROUP LOG_GROUP_SHARED_CROPENGL

#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/critsect.h>
#include <iprt/mem.h>
#include <iprt/semaphore.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/thread.h>

#include <VBox/hgcmsvc.h>
#include <VBox/log.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/VirtualBox.h>
#include <VBox/com/errorprint.h>
#include <VBox/HostServices/VBoxCrOpenGLSvc.h>
#include <VBox/vmm/ssm.h>
#include <VBox/VBoxOGL.h>

#include "cr_mem.h"
#include "cr_server.h"

PVBOXHGCMSVCHELPERS g_pHelpers;
static IConsole* g_pConsole = NULL;
static uint32_t g_u32ScreenCount = 0;
static PVM g_pVM = NULL;
static uint32_t g_u32fCrHgcmDisabled = 0;

#ifndef RT_OS_WINDOWS
# define DWORD int
# define WINAPI
#endif

static const char* gszVBoxOGLSSMMagic = "***OpenGL state data***";

/* Used to process guest calls exceeding maximum allowed HGCM call size in a sequence of smaller calls */
typedef struct _CRVBOXSVCBUFFER_t {
    uint32_t uiId;
    uint32_t uiSize;
    void*    pData;
    _CRVBOXSVCBUFFER_t *pNext, *pPrev;
} CRVBOXSVCBUFFER_t;

static CRVBOXSVCBUFFER_t *g_pCRVBoxSVCBuffers = NULL;
static uint32_t g_CRVBoxSVCBufferID = 0;

/* svcPresentFBO related data */
typedef struct _CRVBOXSVCPRESENTFBOCMD_t {
    void *pData;
    int32_t screenId, x, y, w, h;
    _CRVBOXSVCPRESENTFBOCMD_t *pNext;
} CRVBOXSVCPRESENTFBOCMD_t, *PCRVBOXSVCPRESENTFBOCMD_t;


static DECLCALLBACK(void) svcNotifyEventCB(int32_t screenId, uint32_t uEvent, void* pvData, uint32_t cbData)
{
    ComPtr<IDisplay> pDisplay;
    ComPtr<IFramebuffer> pFramebuffer;

    if (!g_pConsole)
    {
        crWarning("Console not defined!");
        return;
    }

    CHECK_ERROR2I_STMT(g_pConsole, COMGETTER(Display)(pDisplay.asOutParam()), return);

    CHECK_ERROR2I_STMT(pDisplay, QueryFramebuffer(screenId, pFramebuffer.asOutParam()), return);

    if (!pFramebuffer)
        return;

    com::SafeArray<BYTE> data(cbData);
    if (cbData)
        memcpy(data.raw(), pvData, cbData);

    pFramebuffer->Notify3DEvent(uEvent, ComSafeArrayAsInParam(data));
}


static DECLCALLBACK(int) svcUnload (void *)
{
    int rc = VINF_SUCCESS;

    Log(("SHARED_CROPENGL svcUnload\n"));

    crVBoxServerTearDown();

    return rc;
}

static DECLCALLBACK(int) svcConnect (void *, uint32_t u32ClientID, void *pvClient)
{
    int rc = VINF_SUCCESS;

    NOREF(pvClient);

    if (g_u32fCrHgcmDisabled)
    {
        WARN(("connect not expected"));
        return VERR_INVALID_STATE;
    }

    Log(("SHARED_CROPENGL svcConnect: u32ClientID = %d\n", u32ClientID));

    rc = crVBoxServerAddClient(u32ClientID);

    return rc;
}

static DECLCALLBACK(int) svcDisconnect (void *, uint32_t u32ClientID, void *pvClient)
{
    int rc = VINF_SUCCESS;

    NOREF(pvClient);

    if (g_u32fCrHgcmDisabled)
    {
        WARN(("disconnect not expected"));
        return VINF_SUCCESS;
    }

    Log(("SHARED_CROPENGL svcDisconnect: u32ClientID = %d\n", u32ClientID));

    crVBoxServerRemoveClient(u32ClientID);

    return rc;
}

static DECLCALLBACK(int) svcSaveState(void *, uint32_t u32ClientID, void *pvClient, PSSMHANDLE pSSM)
{
    int rc = VINF_SUCCESS;

    NOREF(pvClient);

    Log(("SHARED_CROPENGL svcSaveState: u32ClientID = %d\n", u32ClientID));

    /* Start*/
    rc = SSMR3PutStrZ(pSSM, gszVBoxOGLSSMMagic);
    AssertRCReturn(rc, rc);

    /* Version */
    rc = SSMR3PutU32(pSSM, (uint32_t) SHCROGL_SSM_VERSION);
    AssertRCReturn(rc, rc);

    /* The state itself */
    rc = crVBoxServerSaveState(pSSM);
    AssertRCReturn(rc, rc);

    /* Save svc buffers info */
    {
        CRVBOXSVCBUFFER_t *pBuffer = g_pCRVBoxSVCBuffers;

        rc = SSMR3PutU32(pSSM, g_CRVBoxSVCBufferID);
        AssertRCReturn(rc, rc);

        while (pBuffer)
        {
            rc = SSMR3PutU32(pSSM, pBuffer->uiId);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutU32(pSSM, pBuffer->uiSize);
            AssertRCReturn(rc, rc);

            rc = SSMR3PutMem(pSSM, pBuffer->pData, pBuffer->uiSize);
            AssertRCReturn(rc, rc);

            pBuffer = pBuffer->pNext;
        }

        rc = SSMR3PutU32(pSSM, 0);
        AssertRCReturn(rc, rc);
    }

    /* End */
    rc = SSMR3PutStrZ(pSSM, gszVBoxOGLSSMMagic);
    AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) svcLoadState(void *, uint32_t u32ClientID, void *pvClient, PSSMHANDLE pSSM)
{
    int rc = VINF_SUCCESS;

    NOREF(pvClient);

    Log(("SHARED_CROPENGL svcLoadState: u32ClientID = %d\n", u32ClientID));

    char psz[2000];
    uint32_t ui32;

    /* Start of data */
    rc = SSMR3GetStrZEx(pSSM, psz, 2000, NULL);
    AssertRCReturn(rc, rc);
    if (strcmp(gszVBoxOGLSSMMagic, psz))
        return VERR_SSM_UNEXPECTED_DATA;

    /* Version */
    rc = SSMR3GetU32(pSSM, &ui32);
    AssertRCReturn(rc, rc);

    /* The state itself */
    rc = crVBoxServerLoadState(pSSM, ui32);

    if (rc==VERR_SSM_DATA_UNIT_FORMAT_CHANGED && ui32!=SHCROGL_SSM_VERSION)
    {
        LogRel(("OpenGL: svcLoadState: Unsupported save state version %d\n", ui32));

        /** @todo ugly hack, as we don't know size of stored opengl data try to read untill end of opengl data marker*/
        /*VBoxSharedCrOpenGL isn't last hgcm service now, so can't use SSMR3SkipToEndOfUnit*/
        {
            const char *pMatch = &gszVBoxOGLSSMMagic[0];
            char current;

            while (*pMatch)
            {
                rc = SSMR3GetS8(pSSM, (int8_t*)&current);
                AssertRCReturn(rc, rc);

                if (current==*pMatch)
                {
                    pMatch++;
                }
                else
                {
                    pMatch = &gszVBoxOGLSSMMagic[0];
                }
            }
        }

        return VINF_SUCCESS;
    }
    AssertRCReturn(rc, rc);

    /* Load svc buffers info */
    if (ui32>=24)
    {
        uint32_t uiId;

        rc = SSMR3GetU32(pSSM, &g_CRVBoxSVCBufferID);
        AssertRCReturn(rc, rc);

        rc = SSMR3GetU32(pSSM, &uiId);
        AssertRCReturn(rc, rc);

        while (uiId)
        {
            CRVBOXSVCBUFFER_t *pBuffer = (CRVBOXSVCBUFFER_t *) RTMemAlloc(sizeof(CRVBOXSVCBUFFER_t));
            if (!pBuffer)
            {
                return VERR_NO_MEMORY;
            }
            pBuffer->uiId = uiId;

            rc = SSMR3GetU32(pSSM, &pBuffer->uiSize);
            AssertRCReturn(rc, rc);

            pBuffer->pData = RTMemAlloc(pBuffer->uiSize);
            if (!pBuffer->pData)
            {
                RTMemFree(pBuffer);
                return VERR_NO_MEMORY;
            }

            rc = SSMR3GetMem(pSSM, pBuffer->pData, pBuffer->uiSize);
            AssertRCReturn(rc, rc);

            pBuffer->pNext = g_pCRVBoxSVCBuffers;
            pBuffer->pPrev = NULL;
            if (g_pCRVBoxSVCBuffers)
            {
                g_pCRVBoxSVCBuffers->pPrev = pBuffer;
            }
            g_pCRVBoxSVCBuffers = pBuffer;

            rc = SSMR3GetU32(pSSM, &uiId);
            AssertRCReturn(rc, rc);
        }
    }

    /* End of data */
    rc = SSMR3GetStrZEx(pSSM, psz, 2000, NULL);
    AssertRCReturn(rc, rc);
    if (strcmp(gszVBoxOGLSSMMagic, psz))
        return VERR_SSM_UNEXPECTED_DATA;

    return VINF_SUCCESS;
}

static void svcClientVersionUnsupported(uint32_t minor, uint32_t major)
{
    LogRel(("OpenGL: Unsupported client version %d.%d\n", minor, major));

    /*MS's opengl32 tries to load our ICD around 30 times on failure...this is to prevent unnecessary spam*/
    static int shown = 0;

    if (g_pVM && !shown)
    {
        VMSetRuntimeError(g_pVM, VMSETRTERR_FLAGS_NO_WAIT, "3DSupportIncompatibleAdditions",
        "An attempt by the virtual machine to use hardware 3D acceleration failed. "
        "The version of the Guest Additions installed in the virtual machine does not match the "
        "version of VirtualBox on the host. Please install appropriate Guest Additions to fix this issue");
        shown = 1;
    }
}

static CRVBOXSVCBUFFER_t* svcGetBuffer(uint32_t iBuffer, uint32_t cbBufferSize)
{
    CRVBOXSVCBUFFER_t* pBuffer;

    if (iBuffer)
    {
        pBuffer = g_pCRVBoxSVCBuffers;
        while (pBuffer)
        {
            if (pBuffer->uiId == iBuffer)
            {
                if (cbBufferSize && pBuffer->uiSize!=cbBufferSize)
                {
                    static int shown=0;

                    if (shown<20)
                    {
                        shown++;
                        LogRel(("OpenGL: svcGetBuffer: Invalid buffer(%i) size %i instead of %i\n",
                                iBuffer, pBuffer->uiSize, cbBufferSize));
                    }
                    return NULL;
                }
                return pBuffer;
            }
            pBuffer = pBuffer->pNext;
        }
        return NULL;
    }
    else /*allocate new buffer*/
    {
        pBuffer = (CRVBOXSVCBUFFER_t*) RTMemAlloc(sizeof(CRVBOXSVCBUFFER_t));
        if (pBuffer)
        {
            pBuffer->pData = RTMemAlloc(cbBufferSize);
            if (!pBuffer->pData)
            {
                LogRel(("OpenGL: svcGetBuffer: Not enough memory (%d)\n", cbBufferSize));
                RTMemFree(pBuffer);
                return NULL;
            }
            pBuffer->uiId = ++g_CRVBoxSVCBufferID;
            if (!pBuffer->uiId)
            {
                pBuffer->uiId = ++g_CRVBoxSVCBufferID;
            }
            Assert(pBuffer->uiId);
            pBuffer->uiSize = cbBufferSize;
            pBuffer->pPrev = NULL;
            pBuffer->pNext = g_pCRVBoxSVCBuffers;
            if (g_pCRVBoxSVCBuffers)
            {
                g_pCRVBoxSVCBuffers->pPrev = pBuffer;
            }
            g_pCRVBoxSVCBuffers = pBuffer;
        }
        else
        {
            LogRel(("OpenGL: svcGetBuffer: Not enough memory (%d)\n", sizeof(CRVBOXSVCBUFFER_t)));
        }
        return pBuffer;
    }
}

static void svcFreeBuffer(CRVBOXSVCBUFFER_t* pBuffer)
{
    Assert(pBuffer);

    if (pBuffer->pPrev)
    {
        pBuffer->pPrev->pNext = pBuffer->pNext;
    }
    else
    {
        Assert(pBuffer==g_pCRVBoxSVCBuffers);
        g_pCRVBoxSVCBuffers = pBuffer->pNext;
    }

    if (pBuffer->pNext)
    {
        pBuffer->pNext->pPrev = pBuffer->pPrev;
    }

    RTMemFree(pBuffer->pData);
    RTMemFree(pBuffer);
}

static DECLCALLBACK(void) svcCall (void *, VBOXHGCMCALLHANDLE callHandle, uint32_t u32ClientID, void *pvClient, uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;

    NOREF(pvClient);

    if (g_u32fCrHgcmDisabled)
    {
        WARN(("cr hgcm disabled!"));
        return;
    }

    Log(("SHARED_CROPENGL svcCall: u32ClientID = %d, fn = %d, cParms = %d, pparms = %d\n", u32ClientID, u32Function, cParms, paParms));

#ifdef DEBUG
    uint32_t i;

    for (i = 0; i < cParms; i++)
    {
        /** @todo parameters other than 32 bit */
        Log(("    pparms[%d]: type %d value %d\n", i, paParms[i].type, paParms[i].u.uint32));
    }
#endif

    switch (u32Function)
    {
        case SHCRGL_GUEST_FN_WRITE:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_WRITE\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_WRITE)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            if (    paParms[0].type != VBOX_HGCM_SVC_PARM_PTR     /* pBuffer */
               )
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                /* Fetch parameters. */
                uint8_t *pBuffer  = (uint8_t *)paParms[0].u.pointer.addr;
                uint32_t cbBuffer = paParms[0].u.pointer.size;

                /* Execute the function. */
                rc = crVBoxServerClientWrite(u32ClientID, pBuffer, cbBuffer);
                if (!RT_SUCCESS(rc))
                {
                    Assert(VERR_NOT_SUPPORTED==rc);
                    svcClientVersionUnsupported(0, 0);
                }

            }
            break;
        }

        case SHCRGL_GUEST_FN_INJECT:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_INJECT\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_INJECT)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            if (    paParms[0].type != VBOX_HGCM_SVC_PARM_32BIT /* u32ClientID */
                 || paParms[1].type != VBOX_HGCM_SVC_PARM_PTR   /* pBuffer */
               )
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                /* Fetch parameters. */
                uint32_t u32InjectClientID = paParms[0].u.uint32;
                uint8_t *pBuffer  = (uint8_t *)paParms[1].u.pointer.addr;
                uint32_t cbBuffer = paParms[1].u.pointer.size;

                /* Execute the function. */
                rc = crVBoxServerClientWrite(u32InjectClientID, pBuffer, cbBuffer);
                if (!RT_SUCCESS(rc))
                {
                    if (VERR_NOT_SUPPORTED==rc)
                    {
                        svcClientVersionUnsupported(0, 0);
                    }
                    else
                    {
                        crWarning("SHCRGL_GUEST_FN_INJECT failed to inject for %i from %i", u32InjectClientID, u32ClientID);
                    }
                }
            }
            break;
        }

        case SHCRGL_GUEST_FN_READ:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_READ\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_READ)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            if (    paParms[0].type != VBOX_HGCM_SVC_PARM_PTR     /* pBuffer */
                 || paParms[1].type != VBOX_HGCM_SVC_PARM_32BIT   /* cbBuffer */
               )
            {
                rc = VERR_INVALID_PARAMETER;
            }

            /* Fetch parameters. */
            uint8_t *pBuffer  = (uint8_t *)paParms[0].u.pointer.addr;
            uint32_t cbBuffer = paParms[0].u.pointer.size;

            /* Execute the function. */
            rc = crVBoxServerClientRead(u32ClientID, pBuffer, &cbBuffer);

            if (RT_SUCCESS(rc))
            {
                /* Update parameters.*/
                paParms[0].u.pointer.size = cbBuffer; /// @todo guest doesn't see this change somehow?
            } else if (VERR_NOT_SUPPORTED==rc)
            {
                svcClientVersionUnsupported(0, 0);
            }

            /* Return the required buffer size always */
            paParms[1].u.uint32 = cbBuffer;

            break;
        }

        case SHCRGL_GUEST_FN_WRITE_READ:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_WRITE_READ\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_WRITE_READ)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            if (    paParms[0].type != VBOX_HGCM_SVC_PARM_PTR     /* pBuffer */
                 || paParms[1].type != VBOX_HGCM_SVC_PARM_PTR     /* pWriteback */
                 || paParms[2].type != VBOX_HGCM_SVC_PARM_32BIT   /* cbWriteback */
               )
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                /* Fetch parameters. */
                uint8_t *pBuffer     = (uint8_t *)paParms[0].u.pointer.addr;
                uint32_t cbBuffer    = paParms[0].u.pointer.size;

                uint8_t *pWriteback  = (uint8_t *)paParms[1].u.pointer.addr;
                uint32_t cbWriteback = paParms[1].u.pointer.size;

                /* Execute the function. */
                rc = crVBoxServerClientWrite(u32ClientID, pBuffer, cbBuffer);
                if (!RT_SUCCESS(rc))
                {
                    Assert(VERR_NOT_SUPPORTED==rc);
                    svcClientVersionUnsupported(0, 0);
                }

                rc = crVBoxServerClientRead(u32ClientID, pWriteback, &cbWriteback);

                if (RT_SUCCESS(rc))
                {
                    /* Update parameters.*/
                    paParms[1].u.pointer.size = cbWriteback;
                }
                /* Return the required buffer size always */
                paParms[2].u.uint32 = cbWriteback;
            }

            break;
        }

        case SHCRGL_GUEST_FN_SET_VERSION:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_SET_VERSION\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_SET_VERSION)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            if (    paParms[0].type != VBOX_HGCM_SVC_PARM_32BIT     /* vMajor */
                 || paParms[1].type != VBOX_HGCM_SVC_PARM_32BIT     /* vMinor */
               )
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                /* Fetch parameters. */
                uint32_t vMajor    = paParms[0].u.uint32;
                uint32_t vMinor    = paParms[1].u.uint32;

                /* Execute the function. */
                rc = crVBoxServerClientSetVersion(u32ClientID, vMajor, vMinor);

                if (!RT_SUCCESS(rc))
                {
                    svcClientVersionUnsupported(vMajor, vMinor);
                }
            }

            break;
        }

        case SHCRGL_GUEST_FN_SET_PID:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_SET_PID\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_SET_PID)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            if (paParms[0].type != VBOX_HGCM_SVC_PARM_64BIT)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                /* Fetch parameters. */
                uint64_t pid    = paParms[0].u.uint64;

                /* Execute the function. */
                rc = crVBoxServerClientSetPID(u32ClientID, pid);
            }

            break;
        }

        case SHCRGL_GUEST_FN_WRITE_BUFFER:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_WRITE_BUFFER\n"));
            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_WRITE_BUFFER)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            if (   paParms[0].type != VBOX_HGCM_SVC_PARM_32BIT /*iBufferID*/
                || paParms[1].type != VBOX_HGCM_SVC_PARM_32BIT /*cbBufferSize*/
                || paParms[2].type != VBOX_HGCM_SVC_PARM_32BIT /*ui32Offset*/
                || paParms[3].type != VBOX_HGCM_SVC_PARM_PTR   /*pBuffer*/
               )
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                /* Fetch parameters. */
                uint32_t iBuffer      = paParms[0].u.uint32;
                uint32_t cbBufferSize = paParms[1].u.uint32;
                uint32_t ui32Offset   = paParms[2].u.uint32;
                uint8_t *pBuffer      = (uint8_t *)paParms[3].u.pointer.addr;
                uint32_t cbBuffer     = paParms[3].u.pointer.size;

                /* Execute the function. */
                CRVBOXSVCBUFFER_t *pSvcBuffer = svcGetBuffer(iBuffer, cbBufferSize);
                if (!pSvcBuffer || ((uint64_t)ui32Offset+cbBuffer)>cbBufferSize)
                {
                    rc = VERR_INVALID_PARAMETER;
                }
                else
                {
                    memcpy((void*)((uintptr_t)pSvcBuffer->pData+ui32Offset), pBuffer, cbBuffer);

                    /* Return the buffer id */
                    paParms[0].u.uint32 = pSvcBuffer->uiId;
                }
            }

            break;
        }

        case SHCRGL_GUEST_FN_WRITE_READ_BUFFERED:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_WRITE_READ_BUFFERED\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_WRITE_READ_BUFFERED)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            if (    paParms[0].type != VBOX_HGCM_SVC_PARM_32BIT   /* iBufferID */
                 || paParms[1].type != VBOX_HGCM_SVC_PARM_PTR     /* pWriteback */
                 || paParms[2].type != VBOX_HGCM_SVC_PARM_32BIT   /* cbWriteback */
                 || !paParms[0].u.uint32 /*iBufferID can't be 0 here*/
               )
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                /* Fetch parameters. */
                uint32_t iBuffer = paParms[0].u.uint32;
                uint8_t *pWriteback  = (uint8_t *)paParms[1].u.pointer.addr;
                uint32_t cbWriteback = paParms[1].u.pointer.size;

                CRVBOXSVCBUFFER_t *pSvcBuffer = svcGetBuffer(iBuffer, 0);
                if (!pSvcBuffer)
                {
                    LogRel(("OpenGL: svcCall(WRITE_READ_BUFFERED): Invalid buffer (%d)\n", iBuffer));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }

                uint8_t *pBuffer     = (uint8_t *)pSvcBuffer->pData;
                uint32_t cbBuffer    = pSvcBuffer->uiSize;

                /* Execute the function. */
                rc = crVBoxServerClientWrite(u32ClientID, pBuffer, cbBuffer);
                if (!RT_SUCCESS(rc))
                {
                    Assert(VERR_NOT_SUPPORTED==rc);
                    svcClientVersionUnsupported(0, 0);
                }

                rc = crVBoxServerClientRead(u32ClientID, pWriteback, &cbWriteback);

                if (RT_SUCCESS(rc))
                {
                    /* Update parameters.*/
                    paParms[1].u.pointer.size = cbWriteback;
                }
                /* Return the required buffer size always */
                paParms[2].u.uint32 = cbWriteback;

                svcFreeBuffer(pSvcBuffer);
            }

            break;
        }

        case SHCRGL_GUEST_FN_GET_CAPS_NEW:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_GET_CAPS_NEW\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_GET_CAPS_NEW)
            {
                WARN(("invalid parameter count"));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            if (paParms[0].type != VBOX_HGCM_SVC_PARM_PTR)
            {
                WARN(("invalid parameter"));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            if (paParms[0].u.pointer.size < sizeof (CR_CAPS_INFO))
            {
                WARN(("invalid buffer size"));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            CR_CAPS_INFO *pInfo = (CR_CAPS_INFO*)paParms[0].u.pointer.addr;
            rc = crVBoxServerClientGetCapsNew(u32ClientID, pInfo);
            AssertRC(rc);

            break;
        }

        case SHCRGL_GUEST_FN_GET_CAPS_LEGACY:
        {
            Log(("svcCall: SHCRGL_GUEST_FN_GET_CAPS_LEGACY\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_GET_CAPS_LEGACY)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else if (paParms[0].type != VBOX_HGCM_SVC_PARM_32BIT)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                /* Execute the function. */
                rc = crVBoxServerClientGetCapsLegacy(u32ClientID, &paParms[0].u.uint32);
                AssertRC(rc);
            }

            break;
        }

        default:
        {
            WARN(("svcCall: unexpected u32Function %d", u32Function));
            rc = VERR_NOT_IMPLEMENTED;
        }
    }


    LogFlow(("svcCall: rc = %Rrc\n", rc));

    g_pHelpers->pfnCallComplete (callHandle, rc);
}

static void crScreenshotHandle(CRVBOXHGCMTAKESCREENSHOT *pScreenshot, uint32_t idScreen, uint64_t u64Now)
{
    if (!pScreenshot->pfnScreenshotBegin || pScreenshot->pfnScreenshotBegin(pScreenshot->pvContext, idScreen, u64Now))
    {
        CR_SCREENSHOT Screenshot;

        int rc = crServerVBoxScreenshotGet(idScreen, pScreenshot->u32Width, pScreenshot->u32Height, pScreenshot->u32Pitch, pScreenshot->pvBuffer, &Screenshot);
        if (RT_SUCCESS(rc))
        {
            if (pScreenshot->pfnScreenshotPerform)
                pScreenshot->pfnScreenshotPerform(pScreenshot->pvContext, idScreen,
                        0, 0, 32,
                        Screenshot.Img.pitch, Screenshot.Img.width, Screenshot.Img.height,
                        (uint8_t*)Screenshot.Img.pvData, u64Now);
            crServerVBoxScreenshotRelease(&Screenshot);
        }
        else
        {
            Assert(rc == VERR_INVALID_STATE);
        }

        if (pScreenshot->pfnScreenshotEnd)
            pScreenshot->pfnScreenshotEnd(pScreenshot->pvContext, idScreen, u64Now);
    }
}

/*
 * We differentiate between a function handler for the guest and one for the host.
 */
static int svcHostCallPerform(uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;

    Log(("SHARED_CROPENGL svcHostCall: fn = %d, cParms = %d, pparms = %d\n", u32Function, cParms, paParms));

#ifdef DEBUG
    uint32_t i;

    for (i = 0; i < cParms; i++)
    {
        /** @todo parameters other than 32 bit */
        Log(("    pparms[%d]: type %d value %d\n", i, paParms[i].type, paParms[i].u.uint32));
    }
#endif

    switch (u32Function)
    {
#ifdef VBOX_WITH_CRHGSMI
        case SHCRGL_HOST_FN_CRHGSMI_CMD:
        {
            Assert(cParms == 1 && paParms[0].type == VBOX_HGCM_SVC_PARM_PTR);
            if (cParms == 1 && paParms[0].type == VBOX_HGCM_SVC_PARM_PTR)
            {
                rc = crVBoxServerCrHgsmiCmd((PVBOXVDMACMD_CHROMIUM_CMD)paParms[0].u.pointer.addr, paParms[0].u.pointer.size);
                if (VERR_NOT_SUPPORTED == rc)
                {
                    svcClientVersionUnsupported(0, 0);
                }
            }
            else
                rc = VERR_INVALID_PARAMETER;
        } break;
        case SHCRGL_HOST_FN_CRHGSMI_CTL:
        {
            Assert(cParms == 1 && paParms[0].type == VBOX_HGCM_SVC_PARM_PTR);
            if (cParms == 1 && paParms[0].type == VBOX_HGCM_SVC_PARM_PTR)
                rc = crVBoxServerCrHgsmiCtl((PVBOXVDMACMD_CHROMIUM_CTL)paParms[0].u.pointer.addr, paParms[0].u.pointer.size);
            else
                rc = VERR_INVALID_PARAMETER;
        } break;
#endif
        case SHCRGL_HOST_FN_SET_CONSOLE:
        {
            Log(("svcCall: SHCRGL_HOST_FN_SET_DISPLAY\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_SET_CONSOLE)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else if (paParms[0].type != VBOX_HGCM_SVC_PARM_PTR)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                /* Fetch parameters. */
                IConsole* pConsole = (IConsole*)paParms[0].u.pointer.addr;
                uint32_t  cbData = paParms[0].u.pointer.size;

                /* Verify parameters values. */
                if (cbData != sizeof (IConsole*))
                {
                    rc = VERR_INVALID_PARAMETER;
                }
                else if (!pConsole)
                {
                    rc = VERR_INVALID_PARAMETER;
                }
                else /* Execute the function. */
                {
                    ComPtr<IMachine> pMachine;
                    ComPtr<IDisplay> pDisplay;
                    ComPtr<IFramebuffer> pFramebuffer;
                    LONG xo, yo;
                    LONG64 winId = 0;
                    ULONG monitorCount, i, w, h;

                    CHECK_ERROR_BREAK(pConsole, COMGETTER(Machine)(pMachine.asOutParam()));
                    CHECK_ERROR_BREAK(pMachine, COMGETTER(MonitorCount)(&monitorCount));
                    CHECK_ERROR_BREAK(pConsole, COMGETTER(Display)(pDisplay.asOutParam()));

                    g_pConsole = pConsole;
                    g_u32ScreenCount = monitorCount;

                    rc = crVBoxServerSetScreenCount(monitorCount);
                    AssertRCReturn(rc, rc);

#if 1
                    crServerVBoxCompositionSetEnableStateGlobal(GL_FALSE);

                    for (i=0; i<monitorCount; ++i)
                    {
                        CHECK_ERROR_RET(pDisplay, QueryFramebuffer(i, pFramebuffer.asOutParam()), rc);

                        if (!pFramebuffer)
                        {
                            rc = crVBoxServerUnmapScreen(i);
                            AssertRCReturn(rc, rc);
                        }
                        else
                        {
                            CHECK_ERROR_RET(pFramebuffer, COMGETTER(WinId)(&winId), rc);
                            CHECK_ERROR_RET(pFramebuffer, COMGETTER(Width)(&w), rc);
                            CHECK_ERROR_RET(pFramebuffer, COMGETTER(Height)(&h), rc);
                            ULONG dummy;
                            GuestMonitorStatus_T monitorStatus;
                            CHECK_ERROR_RET(pDisplay, GetScreenResolution(i, &dummy, &dummy, &dummy, &xo, &yo, &monitorStatus), rc);

                            rc = crVBoxServerMapScreen(i, xo, yo, w, h, winId);
                            AssertRCReturn(rc, rc);
                        }
                    }

                    crServerVBoxCompositionSetEnableStateGlobal(GL_TRUE);
#endif

                    rc = VINF_SUCCESS;
                }
            }
            break;
        }
        case SHCRGL_HOST_FN_SET_VM:
        {
            Log(("svcCall: SHCRGL_HOST_FN_SET_VM\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_SET_VM)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else if (paParms[0].type != VBOX_HGCM_SVC_PARM_PTR)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                /* Fetch parameters. */
                PVM pVM = (PVM)paParms[0].u.pointer.addr;
                uint32_t  cbData = paParms[0].u.pointer.size;

                /* Verify parameters values. */
                if (cbData != sizeof (PVM))
                {
                    rc = VERR_INVALID_PARAMETER;
                }
                else
                {
                    /* Execute the function. */
                    g_pVM = pVM;
                    rc = VINF_SUCCESS;
                }
            }
            break;
        }
        case SHCRGL_HOST_FN_SET_VISIBLE_REGION:
        {
            Log(("svcCall: SHCRGL_HOST_FN_SET_VISIBLE_REGION\n"));

            if (cParms != SHCRGL_CPARMS_SET_VISIBLE_REGION)
            {
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            if (    paParms[0].type != VBOX_HGCM_SVC_PARM_PTR     /* pRects */
               )
            {
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            Assert(sizeof (RTRECT) == 4 * sizeof (GLint));

            rc = crVBoxServerSetRootVisibleRegion(paParms[0].u.pointer.size / sizeof (RTRECT), (const RTRECT*)paParms[0].u.pointer.addr);
            break;
        }
        case SHCRGL_HOST_FN_SCREEN_CHANGED:
        {
            Log(("svcCall: SHCRGL_HOST_FN_SCREEN_CHANGED\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_SCREEN_CHANGED)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else if (paParms[0].type != VBOX_HGCM_SVC_PARM_32BIT)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                /* Fetch parameters. */
                uint32_t screenId = paParms[0].u.uint32;

                /* Execute the function. */
                ComPtr<IDisplay> pDisplay;
                ComPtr<IFramebuffer> pFramebuffer;
                LONG xo, yo;
                LONG64 winId = 0;
                ULONG w, h;

                Assert(g_pConsole);
                CHECK_ERROR_RET(g_pConsole, COMGETTER(Display)(pDisplay.asOutParam()), rc);
                CHECK_ERROR_RET(pDisplay, QueryFramebuffer(screenId, pFramebuffer.asOutParam()), rc);

                crServerVBoxCompositionSetEnableStateGlobal(GL_FALSE);

                if (!pFramebuffer)
                {
                    rc = crVBoxServerUnmapScreen(screenId);
                    AssertRCReturn(rc, rc);
                }
                else
                {
                    do {
                        /* determine if the framebuffer is functional */
                        com::SafeArray<BYTE> data;
                        rc = pFramebuffer->Notify3DEvent(VBOX3D_NOTIFY_EVENT_TYPE_TEST_FUNCTIONAL, ComSafeArrayAsInParam(data));

                        if (rc == S_OK)
                            CHECK_ERROR_BREAK(pFramebuffer, COMGETTER(WinId)(&winId));

                        if (!winId)
                        {
                            /* View associated with framebuffer is destroyed, happens with 2d accel enabled */
                            rc = crVBoxServerUnmapScreen(screenId);
                            AssertRCReturn(rc, rc);
                        }
                        else
                        {
                            CHECK_ERROR_BREAK(pFramebuffer, COMGETTER(Width)(&w));
                            CHECK_ERROR_BREAK(pFramebuffer, COMGETTER(Height)(&h));
                            ULONG dummy;
                            GuestMonitorStatus_T monitorStatus;
                            CHECK_ERROR_BREAK(pDisplay, GetScreenResolution(screenId, &dummy, &dummy, &dummy, &xo, &yo, &monitorStatus));

                            rc = crVBoxServerMapScreen(screenId, xo, yo, w, h, winId);
                            AssertRCReturn(rc, rc);
                        }
                    } while (0);
                }

                crServerVBoxCompositionSetEnableStateGlobal(GL_TRUE);

                rc = VINF_SUCCESS;
            }
            break;
        }
        case SHCRGL_HOST_FN_TAKE_SCREENSHOT:
        {
            if (cParms != 1)
            {
                LogRel(("OpenGL: SHCRGL_HOST_FN_TAKE_SCREENSHOT: cParms invalid - %d", cParms));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            if (paParms->type != VBOX_HGCM_SVC_PARM_PTR)
            {
                AssertMsgFailed(("invalid param\n"));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            if (!paParms->u.pointer.addr)
            {
                AssertMsgFailed(("invalid param\n"));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            if (paParms->u.pointer.size != sizeof (CRVBOXHGCMTAKESCREENSHOT))
            {
                AssertMsgFailed(("invalid param\n"));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            CRVBOXHGCMTAKESCREENSHOT *pScreenshot = (CRVBOXHGCMTAKESCREENSHOT*)paParms->u.pointer.addr;
            uint64_t u64Now = RTTimeProgramMilliTS();

            if (pScreenshot->u32Screen == CRSCREEN_ALL)
            {
                for (uint32_t i = 0; i < g_u32ScreenCount; ++i)
                {
                    crScreenshotHandle(pScreenshot, i, u64Now);
                }
            }
            else if (pScreenshot->u32Screen < g_u32ScreenCount)
            {
                crScreenshotHandle(pScreenshot, pScreenshot->u32Screen, u64Now);
            }
            else
            {
                AssertMsgFailed(("invalid screen id\n"));
                rc = VERR_INVALID_PARAMETER;
                break;
            }
            break;
        }
        case SHCRGL_HOST_FN_DEV_RESIZE:
        {
            Log(("svcCall: SHCRGL_HOST_FN_DEV_RESIZE\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_DEV_RESIZE)
            {
                LogRel(("OpenGL: SHCRGL_HOST_FN_DEV_RESIZE: cParms invalid - %d", cParms));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            if (paParms->type != VBOX_HGCM_SVC_PARM_PTR)
            {
                AssertMsgFailed(("invalid param\n"));
                return VERR_INVALID_PARAMETER;
            }

            if (!paParms->u.pointer.addr)
            {
                AssertMsgFailed(("invalid param\n"));
                return VERR_INVALID_PARAMETER;
            }

            if (paParms->u.pointer.size != sizeof (CRVBOXHGCMDEVRESIZE))
            {
                AssertMsgFailed(("invalid param\n"));
                return VERR_INVALID_PARAMETER;
            }

            CRVBOXHGCMDEVRESIZE *pResize = (CRVBOXHGCMDEVRESIZE*)paParms->u.pointer.addr;

            rc = crVBoxServerNotifyResize(&pResize->Screen, pResize->pvVRAM);
            break;
        }
        case SHCRGL_HOST_FN_VIEWPORT_CHANGED:
        {
            Log(("svcCall: SHCRGL_HOST_FN_VIEWPORT_CHANGED\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_VIEWPORT_CHANGED)
            {
                LogRel(("OpenGL: SHCRGL_HOST_FN_VIEWPORT_CHANGED: cParms invalid - %d", cParms));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            for (int i = 0; i < SHCRGL_CPARMS_VIEWPORT_CHANGED; ++i)
            {
                if (paParms[i].type != VBOX_HGCM_SVC_PARM_32BIT)
                {
                    LogRel(("OpenGL: SHCRGL_HOST_FN_VIEWPORT_CHANGED: param[%d] type invalid - %d", i, paParms[i].type));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }
            }

            if (!RT_SUCCESS(rc))
            {
                LogRel(("OpenGL: SHCRGL_HOST_FN_VIEWPORT_CHANGED: param validation failed, returning.."));
                break;
            }

            crServerVBoxCompositionSetEnableStateGlobal(GL_FALSE);

            rc = crVBoxServerSetScreenViewport((int)paParms[0].u.uint32,
                    paParms[1].u.uint32, /* x */
                    paParms[2].u.uint32, /* y */
                    paParms[3].u.uint32, /* w */
                    paParms[4].u.uint32  /* h */);
            if (!RT_SUCCESS(rc))
            {
                LogRel(("OpenGL: SHCRGL_HOST_FN_VIEWPORT_CHANGED: crVBoxServerSetScreenViewport failed, rc %d", rc));
            }

            crServerVBoxCompositionSetEnableStateGlobal(GL_TRUE);

            break;
        }
        case SHCRGL_HOST_FN_VIEWPORT_CHANGED2:
        {
            Log(("svcCall: SHCRGL_HOST_FN_VIEWPORT_CHANGED\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_VIEWPORT_CHANGED)
            {
                LogRel(("OpenGL: SHCRGL_HOST_FN_VIEWPORT_CHANGED: cParms invalid - %d", cParms));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            if (paParms[0].type != VBOX_HGCM_SVC_PARM_PTR
                    || !paParms[0].u.pointer.addr
                    || paParms[0].u.pointer.size != sizeof (CRVBOXHGCMVIEWPORT))
            {
                LogRel(("OpenGL: SHCRGL_HOST_FN_VIEWPORT_CHANGED: param invalid - %d, %#x, %d",
                        paParms[0].type,
                        paParms[0].u.pointer.addr,
                        paParms[0].u.pointer.size));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            crServerVBoxCompositionSetEnableStateGlobal(GL_FALSE);

            CRVBOXHGCMVIEWPORT *pViewportInfo = (CRVBOXHGCMVIEWPORT*)paParms[0].u.pointer.addr;

            rc = crVBoxServerSetScreenViewport(pViewportInfo->u32Screen,
                    pViewportInfo->x, /* x */
                    pViewportInfo->y, /* y */
                    pViewportInfo->width, /* w */
                    pViewportInfo->height  /* h */);
            if (!RT_SUCCESS(rc))
            {
                LogRel(("OpenGL: SHCRGL_HOST_FN_VIEWPORT_CHANGED: crVBoxServerSetScreenViewport failed, rc %d", rc));
            }

            crServerVBoxCompositionSetEnableStateGlobal(GL_TRUE);

            break;
        }
        case SHCRGL_HOST_FN_SET_OUTPUT_REDIRECT:
        {
            /*
             * OutputRedirect.
             * Note: the service calls OutputRedirect callbacks directly
             *       and they must not block. If asynchronous processing is needed,
             *       the callback provider must organize this.
             */
            Log(("svcCall: SHCRGL_HOST_FN_SET_OUTPUT_REDIRECT\n"));

            /* Verify parameter count and types. */
            if (cParms != SHCRGL_CPARMS_SET_OUTPUT_REDIRECT)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else if (paParms[0].type != VBOX_HGCM_SVC_PARM_PTR)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                /* Fetch parameters. */
                H3DOUTPUTREDIRECT *pOutputRedirect = (H3DOUTPUTREDIRECT *)paParms[0].u.pointer.addr;
                uint32_t cbData = paParms[0].u.pointer.size;

                /* Verify parameters values. */
                if (cbData != sizeof (H3DOUTPUTREDIRECT))
                {
                    rc = VERR_INVALID_PARAMETER;
                }
                else /* Execute the function. */
                {
                    if (pOutputRedirect->H3DORBegin != NULL)
                    {
                        CROutputRedirect outputRedirect;
                        outputRedirect.pvContext = pOutputRedirect->pvContext;
                        outputRedirect.CRORBegin = pOutputRedirect->H3DORBegin;
                        outputRedirect.CRORGeometry = pOutputRedirect->H3DORGeometry;
                        outputRedirect.CRORVisibleRegion = pOutputRedirect->H3DORVisibleRegion;
                        outputRedirect.CRORFrame = pOutputRedirect->H3DORFrame;
                        outputRedirect.CROREnd = pOutputRedirect->H3DOREnd;
                        outputRedirect.CRORContextProperty = pOutputRedirect->H3DORContextProperty;
                        rc = crVBoxServerOutputRedirectSet(&outputRedirect);
                        if (RT_SUCCESS(rc))
                        {
                            rc = crVBoxServerSetOffscreenRendering(GL_TRUE);
                        }
                    }
                    else
                    {
                        /* Redirection is disabled. */
                        crVBoxServerSetOffscreenRendering(GL_FALSE);
                        crVBoxServerOutputRedirectSet(NULL);
                    }
                }
            }
            break;
        }
        case SHCRGL_HOST_FN_WINDOWS_SHOW:
        {
            /* Verify parameter count and types. */
            if (cParms != 1)
            {
                WARN(("invalid parameter"));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            if (paParms[0].type != VBOX_HGCM_SVC_PARM_32BIT)
            {
                WARN(("invalid parameter"));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            rc = crServerVBoxWindowsShow(!!paParms[0].u.uint32);
            if (!RT_SUCCESS(rc))
                WARN(("crServerVBoxWindowsShow failed rc %d", rc));

            break;
        }
        case SHCRGL_HOST_FN_SET_SCALE_FACTOR:
        {
            /* Verify parameter count and types. */
            if (cParms != 1
             || paParms[0].type != VBOX_HGCM_SVC_PARM_PTR
             || paParms[0].u.pointer.size != sizeof(CRVBOXHGCMSETSCALEFACTOR)
             || !paParms[0].u.pointer.addr)
            {
                WARN(("invalid parameter"));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            CRVBOXHGCMSETSCALEFACTOR *pData = (CRVBOXHGCMSETSCALEFACTOR *)paParms[0].u.pointer.addr;
            double dScaleFactorW = (double)(pData->u32ScaleFactorWMultiplied) / VBOX_OGL_SCALE_FACTOR_MULTIPLIER;
            double dScaleFactorH = (double)(pData->u32ScaleFactorHMultiplied) / VBOX_OGL_SCALE_FACTOR_MULTIPLIER;

            rc = VBoxOglSetScaleFactor(pData->u32Screen, dScaleFactorW, dScaleFactorH);

            /* Log scaling factor rounded to nearest 'int' value (not so precise). */
            LogRel(("OpenGL: Set 3D content scale factor to (%u, %u), multiplier %d (rc=%Rrc)\n",
                pData->u32ScaleFactorWMultiplied,
                pData->u32ScaleFactorHMultiplied,
                (int)VBOX_OGL_SCALE_FACTOR_MULTIPLIER,
                rc));

            break;
        }

        case SHCRGL_HOST_FN_SET_UNSCALED_HIDPI:
        {
            /* Verify parameter count and types. */
            if (cParms != 1
             || paParms[0].type != VBOX_HGCM_SVC_PARM_PTR
             || paParms[0].u.pointer.size != sizeof(CRVBOXHGCMSETUNSCALEDHIDPIOUTPUT)
             || !paParms[0].u.pointer.addr)
            {
                WARN(("invalid parameter"));
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            CRVBOXHGCMSETUNSCALEDHIDPIOUTPUT *pData = (CRVBOXHGCMSETUNSCALEDHIDPIOUTPUT *)paParms[0].u.pointer.addr;
            crServerSetUnscaledHiDPI(pData->fUnscaledHiDPI);
            LogRel(("OpenGL: Set OpenGL scale policy on HiDPI displays (fUnscaledHiDPI=%d)\n", pData->fUnscaledHiDPI));
            break;
        }

        default:
            WARN(("svcHostCallPerform: unexpected u32Function %d", u32Function));
            rc = VERR_NOT_IMPLEMENTED;
            break;
    }

    LogFlow(("svcHostCall: rc = %Rrc\n", rc));
    return rc;
}

int crVBoxServerHostCtl(VBOXCRCMDCTL *pCtl, uint32_t cbCtl)
{
    if ((cbCtl - sizeof (VBOXCRCMDCTL)) % sizeof(VBOXHGCMSVCPARM))
    {
        WARN(("invalid param size"));
        return VERR_INVALID_PARAMETER;
    }
    uint32_t cParams = (cbCtl - sizeof (VBOXCRCMDCTL)) / sizeof (VBOXHGCMSVCPARM);
    bool fHasCallout = VBOXCRCMDCTL_IS_CALLOUT_AVAILABLE(pCtl);
    if (fHasCallout)
        crVBoxServerCalloutEnable(pCtl);

    int rc = svcHostCallPerform(pCtl->u32Function, cParams, (VBOXHGCMSVCPARM*)(pCtl + 1));

    if (fHasCallout)
        crVBoxServerCalloutDisable();

    return rc;
}

static DECLCALLBACK(int) svcHostCall(void *, uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    switch (u32Function)
    {
        case SHCRGL_HOST_FN_CTL:
        {
            if (cParms != 1)
            {
                WARN(("cParams != 1"));
                return VERR_INVALID_PARAMETER;
            }

            if (paParms->type != VBOX_HGCM_SVC_PARM_PTR)
            {
                WARN(("invalid param type"));
                return VERR_INVALID_PARAMETER;
            }

            if (paParms->u.pointer.size < sizeof (VBOXCRCMDCTL))
            {
                WARN(("invalid param size"));
                return VERR_INVALID_PARAMETER;
            }

            VBOXCRCMDCTL *pCtl = (VBOXCRCMDCTL*)paParms->u.pointer.addr;
            switch (pCtl->enmType)
            {
                case VBOXCRCMDCTL_TYPE_HGCM:
                {
                    return crVBoxServerHostCtl(pCtl, paParms->u.pointer.size);
                }
                case VBOXCRCMDCTL_TYPE_DISABLE:
                {
                    if (paParms->u.pointer.size != sizeof (VBOXCRCMDCTL_DISABLE))
                        WARN(("invalid param size"));
                    VBOXCRCMDCTL_DISABLE *pDisable = (VBOXCRCMDCTL_DISABLE*)pCtl;
                    int rc = crVBoxServerHgcmDisable(&pDisable->Data);
                    if (RT_SUCCESS(rc))
                        g_u32fCrHgcmDisabled = 1;
                    else
                        WARN(("crVBoxServerHgcmDisable failed %d", rc));
                    return rc;
                }
                case VBOXCRCMDCTL_TYPE_ENABLE:
                {
                    if (paParms->u.pointer.size != sizeof (VBOXCRCMDCTL_ENABLE))
                        WARN(("invalid param size"));
                    VBOXCRCMDCTL_ENABLE *pEnable = (VBOXCRCMDCTL_ENABLE*)pCtl;
                    int rc = crVBoxServerHgcmEnable(&pEnable->Data);
                    if (RT_SUCCESS(rc))
                        g_u32fCrHgcmDisabled = 0;
                    else
                        WARN(("crVBoxServerHgcmEnable failed %d", rc));
                    return rc;
                }
                default:
                    WARN(("svcHostCall: invalid function %d", pCtl->enmType));
                    return VERR_INVALID_PARAMETER;
            }
            /* not reached. */
        }

        default:
            if (g_u32fCrHgcmDisabled)
            {
                WARN(("cr hgcm disabled!"));
                return VERR_INVALID_STATE;
            }
            return svcHostCallPerform(u32Function, cParms, paParms);
    }
}

extern "C" DECLCALLBACK(DECLEXPORT(int)) VBoxHGCMSvcLoad (VBOXHGCMSVCFNTABLE *ptable)
{
    int rc = VINF_SUCCESS;

    Log(("SHARED_CROPENGL VBoxHGCMSvcLoad: ptable = %p\n", ptable));

    if (!ptable)
    {
        rc = VERR_INVALID_PARAMETER;
    }
    else
    {
        Log(("VBoxHGCMSvcLoad: ptable->cbSize = %d, ptable->u32Version = 0x%08X\n", ptable->cbSize, ptable->u32Version));

        if (    ptable->cbSize != sizeof (VBOXHGCMSVCFNTABLE)
            ||  ptable->u32Version != VBOX_HGCM_SVC_VERSION)
        {
            rc = VERR_INVALID_PARAMETER;
        }
        else
        {
            g_pHelpers = ptable->pHelpers;

            g_u32fCrHgcmDisabled = 0;

            ptable->cbClient = sizeof (void*);

            ptable->pfnUnload     = svcUnload;
            ptable->pfnConnect    = svcConnect;
            ptable->pfnDisconnect = svcDisconnect;
            ptable->pfnCall       = svcCall;
            ptable->pfnHostCall   = svcHostCall;
            ptable->pfnSaveState  = svcSaveState;
            ptable->pfnLoadState  = svcLoadState;
            ptable->pvService     = NULL;

            if (!crVBoxServerInit())
                return VERR_NOT_SUPPORTED;

            crServerVBoxSetNotifyEventCB(svcNotifyEventCB);
        }
    }

    return rc;
}

#ifdef RT_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <iprt/win/windows.h>
BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    (void) lpvReserved;

    switch (fdwReason)
    {
        case DLL_THREAD_ATTACH:
        {
            crStateVBoxAttachThread();
            break;
        }

        case DLL_PROCESS_DETACH:
        /* do exactly the same thing as for DLL_THREAD_DETACH since
         * DLL_THREAD_DETACH is not called for the thread doing DLL_PROCESS_DETACH according to msdn docs */
        case DLL_THREAD_DETACH:
        {
            crStateVBoxDetachThread();
            break;
        }

        case DLL_PROCESS_ATTACH:
        default:
            break;
    }

    return TRUE;
}
#endif
