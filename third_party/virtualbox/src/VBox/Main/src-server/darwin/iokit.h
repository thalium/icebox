/* $Id: iokit.h $ */
/** @file
 * Main - Darwin IOKit Routines.
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

#ifndef ___darwin_iokit_h___
#define ___darwin_iokit_h___

#include <iprt/cdefs.h>
#include <iprt/types.h>
#ifdef VBOX_WITH_USB
# include <VBox/usb.h>
#endif

/**
 * Darwin DVD descriptor as returned by DarwinGetDVDDrives().
 */
typedef struct DARWINDVD
{
    /** Pointer to the next DVD. */
    struct DARWINDVD *pNext;
    /** Variable length name / identifier. */
    char szName[1];
} DARWINDVD;
/** Pointer to a Darwin DVD descriptor. */
typedef DARWINDVD *PDARWINDVD;


/**
 * Darwin ethernet controller descriptor as returned by DarwinGetEthernetControllers().
 */
typedef struct DARWINETHERNIC
{
    /** Pointer to the next NIC. */
    struct DARWINETHERNIC *pNext;
    /** The BSD name. (like en0)*/
    char szBSDName[16];
    /** The fake unique identifier. */
    RTUUID Uuid;
    /** The MAC address. */
    RTMAC Mac;
    /** Whether it's wireless (true) or wired (false). */
    bool fWireless;
    /** Whether it is an AirPort device. */
    bool fAirPort;
    /** Whether it's built in or not. */
    bool fBuiltin;
    /** Whether it's a USB device or not. */
    bool fUSB;
    /** Whether it's the primary interface. */
    bool fPrimaryIf;
    /** A variable length descriptive name if possible. */
    char szName[1];
} DARWINETHERNIC;
/** Pointer to a Darwin ethernet controller descriptor.  */
typedef DARWINETHERNIC *PDARWINETHERNIC;


/** The run loop mode string used by iokit.cpp when it registers
 * notifications events. */
#define VBOX_IOKIT_MODE_STRING "VBoxIOKitMode"

RT_C_DECLS_BEGIN
#ifdef VBOX_WITH_USB
void *          DarwinSubscribeUSBNotifications(void);
void            DarwinUnsubscribeUSBNotifications(void *pvOpaque);
PUSBDEVICE      DarwinGetUSBDevices(void);
void            DarwinFreeUSBDeviceFromIOKit(PUSBDEVICE pCur);
int             DarwinReEnumerateUSBDevice(PCUSBDEVICE pCur);
#endif /* VBOX_WITH_USB */
PDARWINDVD      DarwinGetDVDDrives(void);
PDARWINETHERNIC DarwinGetEthernetControllers(void);
RT_C_DECLS_END

#endif
