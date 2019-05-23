/** @file
 *
 * Shared Clipboard
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __VBOXCLIPBOARD__H
#define __VBOXCLIPBOARD__H

#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD
#include <VBox/hgcmsvc.h>
#include <VBox/log.h>

struct _VBOXCLIPBOARDCONTEXT;
typedef struct _VBOXCLIPBOARDCONTEXT VBOXCLIPBOARDCONTEXT;


typedef struct _VBOXCLIPBOARDCLIENTDATA
{
    struct _VBOXCLIPBOARDCLIENTDATA *pNext;
    struct _VBOXCLIPBOARDCLIENTDATA *pPrev;

    VBOXCLIPBOARDCONTEXT *pCtx;

    uint32_t u32ClientID;

    bool fAsync;        /* Guest is waiting for a message. */
    bool fReadPending;  /* The guest is waiting for data from the host */

    bool fMsgQuit;
    bool fMsgReadData;
    bool fMsgFormats;

    struct {
        VBOXHGCMCALLHANDLE callHandle;
        VBOXHGCMSVCPARM *paParms;
    } async;

    struct {
        VBOXHGCMCALLHANDLE callHandle;
        VBOXHGCMSVCPARM *paParms;
    } asyncRead;

    struct {
         void *pv;
         uint32_t cb;
         uint32_t u32Format;
    } data;

    uint32_t u32AvailableFormats;
    uint32_t u32RequestedFormat;

} VBOXCLIPBOARDCLIENTDATA;

/*
 * The service functions. Locking is between the service thread and the platform dependent windows thread.
 */
bool vboxSvcClipboardLock (void);
void vboxSvcClipboardUnlock (void);

void vboxSvcClipboardReportMsg (VBOXCLIPBOARDCLIENTDATA *pClient, uint32_t u32Msg, uint32_t u32Formats);

void vboxSvcClipboardCompleteReadData(VBOXCLIPBOARDCLIENTDATA *pClient, int rc, uint32_t cbActual);

bool vboxSvcClipboardGetHeadless(void);

/*
 * Platform dependent functions.
 */
int vboxClipboardInit (void);
void vboxClipboardDestroy (void);

int vboxClipboardConnect (VBOXCLIPBOARDCLIENTDATA *pClient, bool fHeadless);
void vboxClipboardDisconnect (VBOXCLIPBOARDCLIENTDATA *pClient);

void vboxClipboardFormatAnnounce (VBOXCLIPBOARDCLIENTDATA *pClient, uint32_t u32Formats);

int vboxClipboardReadData (VBOXCLIPBOARDCLIENTDATA *pClient, uint32_t u32Format, void *pv, uint32_t cb, uint32_t *pcbActual);

void vboxClipboardWriteData (VBOXCLIPBOARDCLIENTDATA *pClient, void *pv, uint32_t cb, uint32_t u32Format);

int vboxClipboardSync (VBOXCLIPBOARDCLIENTDATA *pClient);

/* Host unit testing interface */
#ifdef UNIT_TEST
uint32_t TestClipSvcGetMode(void);
#endif

#endif /* __VBOXCLIPBOARD__H */
