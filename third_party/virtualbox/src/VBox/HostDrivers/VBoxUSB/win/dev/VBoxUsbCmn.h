/* $Id: VBoxUsbCmn.h $ */
/** @file
 * VBoxUsmCmn.h - USB device. Common defs
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
#ifndef ___VBoxUsbCmn_h___
#define ___VBoxUsbCmn_h___

#include "../cmn/VBoxDrvTool.h"
#include "../cmn/VBoxUsbTool.h"

#include <iprt/cdefs.h>
#include <iprt/asm.h>

#include <VBox/usblib-win.h>

#define VBOXUSB_CFG_IDLE_TIME_MS 5000

typedef struct VBOXUSBDEV_EXT *PVBOXUSBDEV_EXT;

RT_C_DECLS_BEGIN

#ifdef _WIN64
#define DECLSPEC_USBIMPORT                      DECLSPEC_IMPORT
#else
#define DECLSPEC_USBIMPORT

#define USBD_ParseDescriptors                   _USBD_ParseDescriptors
#define USBD_ParseConfigurationDescriptorEx     _USBD_ParseConfigurationDescriptorEx
#define USBD_CreateConfigurationRequestEx       _USBD_CreateConfigurationRequestEx
#endif

DECLSPEC_USBIMPORT PUSB_COMMON_DESCRIPTOR
USBD_ParseDescriptors(
    IN PVOID DescriptorBuffer,
    IN ULONG TotalLength,
    IN PVOID StartPosition,
    IN LONG DescriptorType
    );

DECLSPEC_USBIMPORT PUSB_INTERFACE_DESCRIPTOR
USBD_ParseConfigurationDescriptorEx(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PVOID StartPosition,
    IN LONG InterfaceNumber,
    IN LONG AlternateSetting,
    IN LONG InterfaceClass,
    IN LONG InterfaceSubClass,
    IN LONG InterfaceProtocol
    );

DECLSPEC_USBIMPORT PURB
USBD_CreateConfigurationRequestEx(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSBD_INTERFACE_LIST_ENTRY InterfaceList
    );

RT_C_DECLS_END

DECLHIDDEN(PVOID) vboxUsbMemAlloc(SIZE_T cbBytes);
DECLHIDDEN(PVOID) vboxUsbMemAllocZ(SIZE_T cbBytes);
DECLHIDDEN(VOID) vboxUsbMemFree(PVOID pvMem);

#include "VBoxUsbRt.h"
#include "VBoxUsbPnP.h"
#include "VBoxUsbPwr.h"
#include "VBoxUsbDev.h"


#endif /* #ifndef ___VBoxUsbCmn_h___ */
