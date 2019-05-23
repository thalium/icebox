/** $Id: clipboard.cpp $ */
/** @file
 * Guest Additions - X11 Shared Clipboard.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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
#include <iprt/alloc.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/process.h>
#include <iprt/semaphore.h>

#include <VBox/log.h>
#include <VBox/VBoxGuestLib.h>
#include <VBox/HostServices/VBoxClipboardSvc.h>
#include <VBox/GuestHost/SharedClipboard.h>

#include "VBoxClient.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/

/**
 * Global clipboard context information.
 */
struct _VBOXCLIPBOARDCONTEXT
{
    /** Client ID for the clipboard subsystem */
    uint32_t client;

    /** Pointer to the X11 clipboard backend */
    CLIPBACKEND *pBackend;
};

/** Only one client is supported. There seems to be no need for more clients. */
static VBOXCLIPBOARDCONTEXT g_ctx;


/**
 * Transfer clipboard data from the guest to the host.
 *
 * @returns VBox result code
 * @param   u32Format The format of the data being sent
 * @param   pv        Pointer to the data being sent
 * @param   cb        Size of the data being sent in bytes
 */
static int vboxClipboardSendData(uint32_t u32Format, void *pv, uint32_t cb)
{
    int rc;
    LogRelFlowFunc(("u32Format=%d, pv=%p, cb=%d\n", u32Format, pv, cb));
    rc = VbglR3ClipboardWriteData(g_ctx.client, u32Format, pv, cb);
    LogRelFlowFunc(("rc=%Rrc\n", rc));
    return rc;
}


/**
 * Get clipboard data from the host.
 *
 * @returns VBox result code
 * @param   pCtx      Our context information
 * @param   u32Format The format of the data being requested
 * @retval  ppv       On success and if pcb > 0, this will point to a buffer
 *                    to be freed with RTMemFree containing the data read.
 * @retval  pcb       On success, this contains the number of bytes of data
 *                    returned
 */
int ClipRequestDataForX11(VBOXCLIPBOARDCONTEXT *pCtx, uint32_t u32Format, void **ppv, uint32_t *pcb)
{
    RT_NOREF1(pCtx);
    int rc = VINF_SUCCESS;
    uint32_t cb = 1024;
    void *pv = RTMemAlloc(cb);

    *ppv = 0;
    LogRelFlowFunc(("u32Format=%u\n", u32Format));
    if (RT_UNLIKELY(!pv))
        rc = VERR_NO_MEMORY;
    if (RT_SUCCESS(rc))
        rc = VbglR3ClipboardReadData(g_ctx.client, u32Format, pv, cb, pcb);
    if (RT_SUCCESS(rc) && (rc != VINF_BUFFER_OVERFLOW))
        *ppv = pv;
    /* A return value of VINF_BUFFER_OVERFLOW tells us to try again with a
     * larger buffer.  The size of the buffer needed is placed in *pcb.
     * So we start all over again. */
    if (rc == VINF_BUFFER_OVERFLOW)
    {
        cb = *pcb;
        RTMemFree(pv);
        pv = RTMemAlloc(cb);
        if (RT_UNLIKELY(!pv))
            rc = VERR_NO_MEMORY;
        if (RT_SUCCESS(rc))
            rc = VbglR3ClipboardReadData(g_ctx.client, u32Format, pv, cb, pcb);
        if (RT_SUCCESS(rc) && (rc != VINF_BUFFER_OVERFLOW))
            *ppv = pv;
    }
    /* Catch other errors. This also catches the case in which the buffer was
     * too small a second time, possibly because the clipboard contents
     * changed half-way through the operation.  Since we can't say whether or
     * not this is actually an error, we just return size 0.
     */
    if (RT_FAILURE(rc) || (VINF_BUFFER_OVERFLOW == rc))
    {
        *pcb = 0;
        if (pv != NULL)
            RTMemFree(pv);
    }
    LogRelFlowFunc(("returning %Rrc\n", rc));
    if (RT_SUCCESS(rc))
        LogRelFlow(("    *pcb=%d\n", *pcb));
    return rc;
}

/** Opaque data structure describing a request from the host for clipboard
 * data, passed in when the request is forwarded to the X11 backend so that
 * it can be completed correctly. */
struct _CLIPREADCBREQ
{
    /** The data format that was requested. */
    uint32_t u32Format;
};

/**
 * Tell the host that new clipboard formats are available.
 *
 * @param  pCtx      Our context information
 * @param u32Formats The formats to advertise
 */
void ClipReportX11Formats(VBOXCLIPBOARDCONTEXT *pCtx, uint32_t u32Formats)
{
    RT_NOREF1(pCtx);
    LogRelFlowFunc(("u32Formats=%d\n", u32Formats));
    int rc = VbglR3ClipboardReportFormats(g_ctx.client, u32Formats);
    LogRelFlowFunc(("rc=%Rrc\n", rc));
}

/** This is called by the backend to tell us that a request for data from
 * X11 has completed.
 * @param  pCtx      Our context information
 * @param  rc        the iprt result code of the request
 * @param  pReq      the request structure that we passed in when we started
 *                   the request.  We RTMemFree() this in this function.
 * @param  pv        the clipboard data returned from X11 if the request
 *                   succeeded (see @a rc)
 * @param  cb        the size of the data in @a pv
 */
void ClipCompleteDataRequestFromX11(VBOXCLIPBOARDCONTEXT *pCtx, int rc, CLIPREADCBREQ *pReq, void *pv, uint32_t cb)
{
    RT_NOREF1(pCtx);
    if (RT_SUCCESS(rc))
        vboxClipboardSendData(pReq->u32Format, pv, cb);
    else
        vboxClipboardSendData(0, NULL, 0);
    RTMemFree(pReq);
}

/**
 * Connect the guest clipboard to the host.
 *
 * @returns VBox status code
 */
int vboxClipboardConnect(void)
{
    int rc = VINF_SUCCESS;
    LogRelFlowFunc(("\n"));

    /* Sanity */
    AssertReturn(g_ctx.client == 0, VERR_WRONG_ORDER);
    g_ctx.pBackend = ClipConstructX11(&g_ctx, false);
    if (!g_ctx.pBackend)
        rc = VERR_NO_MEMORY;
    if (RT_SUCCESS(rc))
        rc = ClipStartX11(g_ctx.pBackend);
    if (RT_SUCCESS(rc))
    {
        rc = VbglR3ClipboardConnect(&g_ctx.client);
        if (RT_FAILURE(rc))
            LogRel(("Error connecting to host. rc=%Rrc\n", rc));
        else if (!g_ctx.client)
        {
            LogRel(("Invalid client ID of 0\n"));
            rc = VERR_NOT_SUPPORTED;
        }
    }

    if (rc != VINF_SUCCESS && g_ctx.pBackend)
        ClipDestructX11(g_ctx.pBackend);
    LogRelFlowFunc(("g_ctx.client=%u rc=%Rrc\n", g_ctx.client, rc));
    return rc;
}

/**
 * The main loop of our clipboard reader.
 */
int vboxClipboardMain(void)
{
    int rc;
    LogRelFlowFunc(("Starting guest clipboard service\n"));
    bool fExiting = false;

    while (!fExiting)
    {
        uint32_t Msg;
        uint32_t fFormats;
        rc = VbglR3ClipboardGetHostMsg(g_ctx.client, &Msg, &fFormats);
        if (RT_SUCCESS(rc))
        {
            switch (Msg)
            {
                case VBOX_SHARED_CLIPBOARD_HOST_MSG_FORMATS:
                {
                    /* The host has announced available clipboard formats.
                     * Save the information so that it is available for
                     * future requests from guest applications.
                     */
                    LogRelFlowFunc(("VBOX_SHARED_CLIPBOARD_HOST_MSG_FORMATS fFormats=%x\n", fFormats));
                    ClipAnnounceFormatToX11(g_ctx.pBackend, fFormats);
                    break;
                }

                case VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA:
                {
                    /* The host needs data in the specified format. */
                    LogRelFlowFunc(("VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA fFormats=%x\n", fFormats));
                    CLIPREADCBREQ *pReq;
                    pReq = (CLIPREADCBREQ *)RTMemAllocZ(sizeof(*pReq));
                    if (!pReq)
                    {
                        rc = VERR_NO_MEMORY;
                        fExiting = true;
                    }
                    else
                    {
                        pReq->u32Format = fFormats;
                        ClipRequestDataFromX11(g_ctx.pBackend, fFormats,
                                               pReq);
                    }
                    break;
                }

                case VBOX_SHARED_CLIPBOARD_HOST_MSG_QUIT:
                {
                    /* The host is terminating. */
                    LogRelFlowFunc(("VBOX_SHARED_CLIPBOARD_HOST_MSG_QUIT\n"));
                    if (RT_SUCCESS(ClipStopX11(g_ctx.pBackend)))
                        ClipDestructX11(g_ctx.pBackend);
                    fExiting = true;
                    break;
                }

                default:
                    LogRel2(("Unsupported message from host!!!\n"));
            }
        }

        LogRelFlow(("processed host event rc = %d\n", rc));
    }
    LogRelFlowFunc(("rc=%d\n", rc));
    return rc;
}

static const char *getPidFilePath()
{
    return ".vboxclient-clipboard.pid";
}

static int run(struct VBCLSERVICE **ppInterface, bool fDaemonised)
{
    RT_NOREF2(ppInterface, fDaemonised);

    /* Initialise the guest library. */
    int rc = VbglR3InitUser();
    if (RT_FAILURE(rc))
        VBClFatalError(("Failed to connect to the VirtualBox kernel service, rc=%Rrc\n", rc));
    rc = vboxClipboardConnect();
    /* Not RT_SUCCESS: VINF_PERMISSION_DENIED is host service not present. */
    if (rc == VINF_SUCCESS)
        rc = vboxClipboardMain();
    if (rc == VERR_NOT_SUPPORTED)
        rc = VINF_SUCCESS;  /* Prevent automatic restart. */
    if (RT_FAILURE(rc))
        LogRelFunc(("guest clipboard service terminated abnormally: return code %Rrc\n", rc));
    return rc;
}

static void cleanup(struct VBCLSERVICE **ppInterface)
{
    NOREF(ppInterface);
    VbglR3Term();
}

struct VBCLSERVICE vbclClipboardInterface =
{
    getPidFilePath,
    VBClServiceDefaultHandler, /* init */
    run,
    cleanup
};

struct CLIPBOARDSERVICE
{
    struct VBCLSERVICE *pInterface;
};

struct VBCLSERVICE **VBClGetClipboardService()
{
    struct CLIPBOARDSERVICE *pService =
        (struct CLIPBOARDSERVICE *)RTMemAlloc(sizeof(*pService));

    if (!pService)
        VBClFatalError(("Out of memory\n"));
    pService->pInterface = &vbclClipboardInterface;
    return &pService->pInterface;
}
