/* $Id: VBoxUsbIdc.h $ */
/** @file
 * Windows USB Proxy - Monitor Driver communication interface.
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
#ifndef ___VBoxUsbIdc_h___
#define ___VBoxUsbIdc_h___

#define VBOXUSBIDC_VERSION_MAJOR 1
#define VBOXUSBIDC_VERSION_MINOR 0

#define VBOXUSBIDC_INTERNAL_IOCTL_GET_VERSION         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x618, METHOD_NEITHER, FILE_WRITE_ACCESS)
#define VBOXUSBIDC_INTERNAL_IOCTL_PROXY_STARTUP       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x619, METHOD_NEITHER, FILE_WRITE_ACCESS)
#define VBOXUSBIDC_INTERNAL_IOCTL_PROXY_TEARDOWN      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x61A, METHOD_NEITHER, FILE_WRITE_ACCESS)
#define VBOXUSBIDC_INTERNAL_IOCTL_PROXY_STATE_CHANGE  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x61B, METHOD_NEITHER, FILE_WRITE_ACCESS)

typedef struct
{
    uint32_t        u32Major;
    uint32_t        u32Minor;
} VBOXUSBIDC_VERSION, *PVBOXUSBIDC_VERSION;

typedef void *HVBOXUSBIDCDEV;

/* the initial device state is USBDEVICESTATE_HELD_BY_PROXY */
typedef struct VBOXUSBIDC_PROXY_STARTUP
{
    union
    {
        /* in: device PDO */
        PDEVICE_OBJECT pPDO;
        /* out: device handle to be used for subsequent USBSUP_PROXY_XXX calls */
        HVBOXUSBIDCDEV hDev;
    } u;
} VBOXUSBIDC_PROXY_STARTUP, *PVBOXUSBIDC_PROXY_STARTUP;

typedef struct VBOXUSBIDC_PROXY_TEARDOWN
{
    HVBOXUSBIDCDEV hDev;
} VBOXUSBIDC_PROXY_TEARDOWN, *PVBOXUSBIDC_PROXY_TEARDOWN;

typedef enum
{
    VBOXUSBIDC_PROXY_STATE_UNKNOWN = 0,
    VBOXUSBIDC_PROXY_STATE_IDLE,
    VBOXUSBIDC_PROXY_STATE_INITIAL = VBOXUSBIDC_PROXY_STATE_IDLE,
    VBOXUSBIDC_PROXY_STATE_USED_BY_GUEST
} VBOXUSBIDC_PROXY_STATE;

typedef struct VBOXUSBIDC_PROXY_STATE_CHANGE
{
    HVBOXUSBIDCDEV hDev;
    VBOXUSBIDC_PROXY_STATE enmState;
} VBOXUSBIDC_PROXY_STATE_CHANGE, *PVBOXUSBIDC_PROXY_STATE_CHANGE;

NTSTATUS VBoxUsbIdcInit();
VOID VBoxUsbIdcTerm();
NTSTATUS VBoxUsbIdcProxyStarted(PDEVICE_OBJECT pPDO, HVBOXUSBIDCDEV *phDev);
NTSTATUS VBoxUsbIdcProxyStopped(HVBOXUSBIDCDEV hDev);

#endif /* #ifndef ___VBoxUsbIdc_h___ */
