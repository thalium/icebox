/** @file
 *
 * Shared Clipboard:
 * Linux host, a stub version with no functionality for use on headless hosts.
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

#include <VBox/HostServices/VBoxClipboardSvc.h>

#include <iprt/alloc.h>
#include <iprt/asm.h>        /* For atomic operations */
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/process.h>
#include <iprt/semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "VBoxClipboard.h"

/** Initialise the host side of the shared clipboard - called by the hgcm layer. */
int vboxClipboardInit (void)
{
    LogFlowFunc(("called, returning VINF_SUCCESS.\n"));
    return VINF_SUCCESS;
}

/** Terminate the host side of the shared clipboard - called by the hgcm layer. */
void vboxClipboardDestroy (void)
{
    LogFlowFunc(("called, returning.\n"));
}

/**
  * Enable the shared clipboard - called by the hgcm clipboard subsystem.
  *
  * @param   pClient    Structure containing context information about the guest system
  * @param   fHeadless  Whether headless.
  * @returns RT status code
  */
int vboxClipboardConnect (VBOXCLIPBOARDCLIENTDATA *pClient,
                          bool fHeadless)
{
    RT_NOREF(pClient, fHeadless);
    LogFlowFunc(("called, returning VINF_SUCCESS.\n"));
    return VINF_SUCCESS;
}

/**
 * Synchronise the contents of the host clipboard with the guest, called by the HGCM layer
 * after a save and restore of the guest.
 */
int vboxClipboardSync (VBOXCLIPBOARDCLIENTDATA * /* pClient */)
{
    LogFlowFunc(("called, returning VINF_SUCCESS.\n"));
    return VINF_SUCCESS;
}

/**
 * Shut down the shared clipboard subsystem and "disconnect" the guest.
 *
 * @param   pClient    Structure containing context information about the guest system
 */
void vboxClipboardDisconnect (VBOXCLIPBOARDCLIENTDATA *pClient)
{
    RT_NOREF(pClient);
    LogFlowFunc(("called, returning.\n"));
}

/**
 * The guest is taking possession of the shared clipboard.  Called by the HGCM clipboard
 * subsystem.
 *
 * @param pClient    Context data for the guest system
 * @param u32Formats Clipboard formats the guest is offering
 */
void vboxClipboardFormatAnnounce (VBOXCLIPBOARDCLIENTDATA *pClient,
                                  uint32_t u32Formats)
{
    RT_NOREF(pClient, u32Formats);
    LogFlowFunc(("called, returning.\n"));
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
                           void *pv, uint32_t cb, uint32_t *pcbActual)
{
    RT_NOREF(pClient, u32Format, pv, cb);
    LogFlowFunc(("called, returning VINF_SUCCESS.\n"));
    /* No data available. */
    *pcbActual = 0;
    return VINF_SUCCESS;
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
    RT_NOREF(pClient, pv, cb, u32Format);
    LogFlowFunc(("called, returning.\n"));
}
