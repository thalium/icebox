/* $Id: VBoxUsbFlt.h $ */
/** @file
 * VBox USB Monitor Device Filtering functionality
 */

/*
 * Copyright (C) 2011-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___win_mon_VBoxUsbFlt_h___
#define ___win_mon_VBoxUsbFlt_h___

#include "VBoxUsbMon.h"
#include <VBoxUSBFilterMgr.h>

#include <VBox/usblib-win.h>

typedef struct VBOXUSBFLTCTX
{
    LIST_ENTRY ListEntry;
    PKEVENT pChangeEvent;
    RTPROCESS Process;
    uint32_t cActiveFilters;
    BOOLEAN bRemoved;
} VBOXUSBFLTCTX, *PVBOXUSBFLTCTX;

NTSTATUS VBoxUsbFltInit();
NTSTATUS VBoxUsbFltTerm();
NTSTATUS VBoxUsbFltCreate(PVBOXUSBFLTCTX pContext);
NTSTATUS VBoxUsbFltClose(PVBOXUSBFLTCTX pContext);
int VBoxUsbFltAdd(PVBOXUSBFLTCTX pContext, PUSBFILTER pFilter, uintptr_t *pId);
int VBoxUsbFltRemove(PVBOXUSBFLTCTX pContext, uintptr_t uId);
NTSTATUS VBoxUsbFltSetNotifyEvent(PVBOXUSBFLTCTX pContext, HANDLE hEvent);
NTSTATUS VBoxUsbFltFilterCheck(PVBOXUSBFLTCTX pContext);

NTSTATUS VBoxUsbFltGetDevice(PVBOXUSBFLTCTX pContext, HVBOXUSBDEVUSR hDevice, PUSBSUP_GETDEV_MON pInfo);

typedef void* HVBOXUSBFLTDEV;
HVBOXUSBFLTDEV VBoxUsbFltProxyStarted(PDEVICE_OBJECT pPdo);
void VBoxUsbFltProxyStopped(HVBOXUSBFLTDEV hDev);

NTSTATUS VBoxUsbFltPdoAdd(PDEVICE_OBJECT pPdo, BOOLEAN *pbFiltered);
NTSTATUS VBoxUsbFltPdoAddCompleted(PDEVICE_OBJECT pPdo);
NTSTATUS VBoxUsbFltPdoRemove(PDEVICE_OBJECT pPdo);
BOOLEAN VBoxUsbFltPdoIsFiltered(PDEVICE_OBJECT pPdo);

#endif

