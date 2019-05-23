/* $Id: VBoxMF.h $ */
/** @file
 * VBox Mouse Filter Driver - Internal Header.
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
 */

#ifndef VBOXMF_H
#define VBOXMF_H

#include <iprt/cdefs.h>
#define LOG_GROUP LOG_GROUP_DRV_MOUSE
#include <VBox/log.h>
#include <iprt/err.h>
#include <iprt/assert.h>
#include "../common/VBoxMouseLog.h"
#include <iprt/nt/ntddk.h>
RT_C_DECLS_BEGIN
#include <ntddmou.h>
#include <ntddkbd.h>
#include <ntdd8042.h>
RT_C_DECLS_END
#include <VBox/VMMDev.h> /* for VMMDevReqMouseStatus */

#define IOCTL_INTERNAL_MOUSE_CONNECT CTL_CODE(FILE_DEVICE_MOUSE, 0x0080, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef VOID (*PFNSERVICECB)(PDEVICE_OBJECT DeviceObject, PMOUSE_INPUT_DATA InputDataStart,
                             PMOUSE_INPUT_DATA InputDataEnd, PULONG InputDataConsumed);

typedef struct _INTERNAL_MOUSE_CONNECT_DATA
{
    PDEVICE_OBJECT pDO;
    PFNSERVICECB pfnServiceCB;
} INTERNAL_MOUSE_CONNECT_DATA, *PINTERNAL_MOUSE_CONNECT_DATA;

typedef struct _VBOXMOUSE_DEVEXT
{
    LIST_ENTRY ListEntry;
    PDEVICE_OBJECT pdoMain;           /* PDO passed to VBoxDrvAddDevice */
    PDEVICE_OBJECT pdoSelf;           /* our PDO created in VBoxDrvAddDevice*/
    PDEVICE_OBJECT pdoParent;         /* Highest PDO in chain before we've attached our filter */

    BOOLEAN bHostMouse;               /* Indicates if we're filtering the chain with emulated i8042 PS/2 adapter */

    INTERNAL_MOUSE_CONNECT_DATA OriginalConnectData; /* Original connect data intercepted in IOCTL_INTERNAL_MOUSE_CONNECT */
    VMMDevReqMouseStatus       *pSCReq;              /* Preallocated request to use in pfnServiceCB */

    IO_REMOVE_LOCK RemoveLock;
} VBOXMOUSE_DEVEXT, *PVBOXMOUSE_DEVEXT;

/* Interface functions */
RT_C_DECLS_BEGIN
 NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
RT_C_DECLS_END

NTSTATUS VBoxDrvAddDevice(IN PDRIVER_OBJECT Driver, IN PDEVICE_OBJECT PDO);
VOID VBoxDrvUnload(IN PDRIVER_OBJECT Driver);

/* IRP handlers */
NTSTATUS VBoxIrpPassthrough(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS VBoxIrpInternalIOCTL(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS VBoxIrpPnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS VBoxIrpPower(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

/* Internal functions */
void VBoxMouFltInitGlobals(void);
void VBoxMouFltDeleteGlobals(void);
void VBoxDeviceAdded(PVBOXMOUSE_DEVEXT pDevExt);
void VBoxInformHost(PVBOXMOUSE_DEVEXT pDevExt);
void VBoxDeviceRemoved(PVBOXMOUSE_DEVEXT pDevExt);

VOID VBoxDrvNotifyServiceCB(PVBOXMOUSE_DEVEXT pDevExt, PMOUSE_INPUT_DATA InputDataStart, PMOUSE_INPUT_DATA InputDataEnd, PULONG  InputDataConsumed);

#endif /*VBOXMF_H*/
