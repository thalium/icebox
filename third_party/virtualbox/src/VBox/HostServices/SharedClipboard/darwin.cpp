/* $Id: darwin.cpp $ */
/** @file
 * Shared Clipboard: Mac OS X host.
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

#include <VBox/HostServices/VBoxClipboardSvc.h>

#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/thread.h>

#include "VBoxClipboard.h"
#include "darwin-pasteboard.h"

/** Global clipboard context information */
struct _VBOXCLIPBOARDCONTEXT
{
    /** We have a separate thread to poll for new clipboard content */
    RTTHREAD thread;
    bool volatile fTerminate;

    /** The reference to the current pasteboard */
    PasteboardRef pasteboard;

    VBOXCLIPBOARDCLIENTDATA *pClient;
};

/** Only one client is supported. There seems to be no need for more clients. */
static VBOXCLIPBOARDCONTEXT g_ctx;


/**
 * Checks if something is present on the clipboard and calls vboxSvcClipboardReportMsg.
 *
 * @returns IPRT status code (ignored).
 * @param   pCtx    The context.
 */
static int vboxClipboardChanged (VBOXCLIPBOARDCONTEXT *pCtx)
{
    if (pCtx->pClient == NULL)
        return VINF_SUCCESS;

    uint32_t fFormats = 0;
    bool fChanged = false;
    /* Retrieve the formats currently in the clipboard and supported by vbox */
    int rc = queryNewPasteboardFormats (pCtx->pasteboard, &fFormats, &fChanged);
    if (RT_SUCCESS (rc) && fChanged)
    {
        vboxSvcClipboardReportMsg (pCtx->pClient, VBOX_SHARED_CLIPBOARD_HOST_MSG_FORMATS, fFormats);
        Log (("vboxClipboardChanged fFormats %02X\n", fFormats));
    }

    return rc;
}


/**
 * The poller thread.
 *
 * This thread will check for the arrival of new data on the clipboard.
 *
 * @returns VINF_SUCCESS (not used).
 * @param   ThreadSelf  Our thread handle.
 * @param   pvUser      Pointer to the VBOXCLIPBOARDCONTEXT structure.
 *
 */
static int vboxClipboardThread (RTTHREAD ThreadSelf, void *pvUser)
{
    Log (("vboxClipboardThread: starting clipboard thread\n"));

    AssertPtrReturn (pvUser, VERR_INVALID_PARAMETER);
    VBOXCLIPBOARDCONTEXT *pCtx = (VBOXCLIPBOARDCONTEXT *) pvUser;

    while (!pCtx->fTerminate)
    {
        /* call this behind the lock because we don't know if the api is
           thread safe and in any case we're calling several methods. */
        vboxSvcClipboardLock();
        vboxClipboardChanged (pCtx);
        vboxSvcClipboardUnlock();

        /* Sleep for 200 msecs before next poll */
        RTThreadUserWait (ThreadSelf, 200);
    }

    Log (("vboxClipboardThread: clipboard thread terminated successfully with return code %Rrc\n", VINF_SUCCESS));
    return VINF_SUCCESS;
}

/*
 * Public platform dependent functions.
 */

/** Initialise the host side of the shared clipboard - called by the hgcm layer. */
int vboxClipboardInit (void)
{
    Log (("vboxClipboardInit\n"));

    g_ctx.fTerminate = false;

    int rc = initPasteboard (&g_ctx.pasteboard);
    AssertRCReturn (rc, rc);

    rc = RTThreadCreate (&g_ctx.thread, vboxClipboardThread, &g_ctx, 0,
                         RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, "SHCLIP");
    if (RT_FAILURE (rc))
    {
        g_ctx.thread = NIL_RTTHREAD;
        destroyPasteboard (&g_ctx.pasteboard);
    }

    return rc;
}

/** Terminate the host side of the shared clipboard - called by the hgcm layer. */
void vboxClipboardDestroy (void)
{
    Log (("vboxClipboardDestroy\n"));

    /*
     * Signal the termination of the polling thread and wait for it to respond.
     */
    ASMAtomicWriteBool (&g_ctx.fTerminate, true);
    int rc = RTThreadUserSignal (g_ctx.thread);
    AssertRC (rc);
    rc = RTThreadWait (g_ctx.thread, RT_INDEFINITE_WAIT, NULL);
    AssertRC (rc);

    /*
     * Destroy the pasteboard and uninitialize the global context record.
     */
    destroyPasteboard (&g_ctx.pasteboard);
    g_ctx.thread = NIL_RTTHREAD;
    g_ctx.pClient = NULL;
}

/**
 * Enable the shared clipboard - called by the hgcm clipboard subsystem.
 *
 * @param   pClient Structure containing context information about the guest system
 * @param   fHeadless Whether headless.
 * @returns RT status code
 */
int vboxClipboardConnect (VBOXCLIPBOARDCLIENTDATA *pClient, bool fHeadless)
{
    NOREF(fHeadless);
    if (g_ctx.pClient != NULL)
    {
        /* One client only. */
        return VERR_NOT_SUPPORTED;
    }

    vboxSvcClipboardLock();

    pClient->pCtx = &g_ctx;
    pClient->pCtx->pClient = pClient;

    /* Initially sync the host clipboard content with the client. */
    int rc = vboxClipboardSync (pClient);

    vboxSvcClipboardUnlock();
    return rc;
}

/**
 * Synchronise the contents of the host clipboard with the guest, called by the HGCM layer
 * after a save and restore of the guest.
 */
int vboxClipboardSync (VBOXCLIPBOARDCLIENTDATA *pClient)
{
    /* Sync the host clipboard content with the client. */
    vboxSvcClipboardLock();
    int rc = vboxClipboardChanged (pClient->pCtx);
    vboxSvcClipboardUnlock();

    return rc;
}

/**
 * Shut down the shared clipboard subsystem and "disconnect" the guest.
 */
void vboxClipboardDisconnect (VBOXCLIPBOARDCLIENTDATA *pClient)
{
    Log (("vboxClipboardDisconnect\n"));

    vboxSvcClipboardLock();
    pClient->pCtx->pClient = NULL;
    vboxSvcClipboardUnlock();
}

/**
 * The guest is taking possession of the shared clipboard.  Called by the HGCM clipboard
 * subsystem.
 *
 * @param pClient    Context data for the guest system
 * @param u32Formats Clipboard formats the guest is offering
 */
void vboxClipboardFormatAnnounce (VBOXCLIPBOARDCLIENTDATA *pClient, uint32_t u32Formats)
{
    Log (("vboxClipboardFormatAnnounce u32Formats %02X\n", u32Formats));
    if (u32Formats == 0)
    {
        /* This is just an automatism, not a genuine announcement */
        return;
    }

    vboxSvcClipboardReportMsg (pClient, VBOX_SHARED_CLIPBOARD_HOST_MSG_READ_DATA,
                               u32Formats);
}

/**
 * Called by the HGCM clipboard subsystem when the guest wants to read the host clipboard.
 *
 * @param pClient   Context information about the guest VM
 * @param u32Format The format that the guest would like to receive the data in
 * @param pv        Where to write the data to
 * @param cb        The size of the buffer to write the data to
 * @param pcbActual Where to write the actual size of the written data
 */
int vboxClipboardReadData (VBOXCLIPBOARDCLIENTDATA *pClient, uint32_t u32Format,
                           void *pv, uint32_t cb, uint32_t * pcbActual)
{
    vboxSvcClipboardLock();

    /* Default to no data available. */
    *pcbActual = 0;
    int rc = readFromPasteboard (pClient->pCtx->pasteboard, u32Format, pv, cb, pcbActual);

    vboxSvcClipboardUnlock();
    return rc;
}

/**
 * Called by the HGCM clipboard subsystem when we have requested data and that data arrives.
 *
 * @param pClient   Context information about the guest VM
 * @param pv        Buffer to which the data was written
 * @param cb        The size of the data written
 * @param u32Format The format of the data written
 */
void vboxClipboardWriteData (VBOXCLIPBOARDCLIENTDATA *pClient, void *pv,
                             uint32_t cb, uint32_t u32Format)
{
    vboxSvcClipboardLock();

    writeToPasteboard (pClient->pCtx->pasteboard, pv, cb, u32Format);

    vboxSvcClipboardUnlock();
}
