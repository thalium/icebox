/* $Id: VBoxUsbHook.h $ */
/** @file
 * Driver Dispatch Table Hooking API impl
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

#ifndef ___VBoxUsbHook_h___
#define ___VBoxUsbHook_h___

#include "VBoxUsbMon.h"

typedef struct VBOXUSBHOOK_ENTRY
{
    LIST_ENTRY RequestList;
    KSPIN_LOCK Lock;
    BOOLEAN fIsInstalled;
    PDRIVER_DISPATCH pfnOldHandler;
    VBOXDRVTOOL_REF HookRef;
    PDRIVER_OBJECT pDrvObj;
    UCHAR iMjFunction;
    PDRIVER_DISPATCH pfnHook;
} VBOXUSBHOOK_ENTRY, *PVBOXUSBHOOK_ENTRY;

typedef struct VBOXUSBHOOK_REQUEST
{
    LIST_ENTRY ListEntry;
    PVBOXUSBHOOK_ENTRY pHook;
    IO_STACK_LOCATION OldLocation;
    PDEVICE_OBJECT pDevObj;
    PIRP pIrp;
    BOOLEAN bCompletionStopped;
} VBOXUSBHOOK_REQUEST, *PVBOXUSBHOOK_REQUEST;

DECLINLINE(BOOLEAN) VBoxUsbHookRetain(PVBOXUSBHOOK_ENTRY pHook)
{
    KIRQL Irql;
    KeAcquireSpinLock(&pHook->Lock, &Irql);
    if (!pHook->fIsInstalled)
    {
        KeReleaseSpinLock(&pHook->Lock, Irql);
        return FALSE;
    }

    VBoxDrvToolRefRetain(&pHook->HookRef);
    KeReleaseSpinLock(&pHook->Lock, Irql);
    return TRUE;
}

DECLINLINE(VOID) VBoxUsbHookRelease(PVBOXUSBHOOK_ENTRY pHook)
{
    VBoxDrvToolRefRelease(&pHook->HookRef);
}

VOID VBoxUsbHookInit(PVBOXUSBHOOK_ENTRY pHook, PDRIVER_OBJECT pDrvObj, UCHAR iMjFunction, PDRIVER_DISPATCH pfnHook);
NTSTATUS VBoxUsbHookInstall(PVBOXUSBHOOK_ENTRY pHook);
NTSTATUS VBoxUsbHookUninstall(PVBOXUSBHOOK_ENTRY pHook);
BOOLEAN VBoxUsbHookIsInstalled(PVBOXUSBHOOK_ENTRY pHook);
NTSTATUS VBoxUsbHookRequestPassDownHookCompletion(PVBOXUSBHOOK_ENTRY pHook, PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_COMPLETION_ROUTINE pfnCompletion, PVBOXUSBHOOK_REQUEST pRequest);
NTSTATUS VBoxUsbHookRequestPassDownHookSkip(PVBOXUSBHOOK_ENTRY pHook, PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS VBoxUsbHookRequestMoreProcessingRequired(PVBOXUSBHOOK_ENTRY pHook, PDEVICE_OBJECT pDevObj, PIRP pIrp, PVBOXUSBHOOK_REQUEST pRequest);
NTSTATUS VBoxUsbHookRequestComplete(PVBOXUSBHOOK_ENTRY pHook, PDEVICE_OBJECT pDevObj, PIRP pIrp, PVBOXUSBHOOK_REQUEST pRequest);
VOID VBoxUsbHookVerifyCompletion(PVBOXUSBHOOK_ENTRY pHook, PVBOXUSBHOOK_REQUEST pRequest, PIRP pIrp);

#endif /* #ifndef ___VBoxUsbHook_h___ */
