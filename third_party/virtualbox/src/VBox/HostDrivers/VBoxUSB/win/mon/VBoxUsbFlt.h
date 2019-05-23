/* $Id: VBoxUsbFlt.h $ */
/** @file
 * VBox USB Monitor Device Filtering functionality
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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

