/* $Id: USBGetDevices.h $ */
/** @file
 * VirtualBox Linux host USB device enumeration.
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

#ifndef ___USBGetDevices_h
#define ___USBGetDevices_h

#include <VBox/usb.h>
#include <iprt/mem.h>
#include <iprt/string.h>

/**
 * Free all the members of a USB device created by the Linux enumeration code.
 *
 * @note This duplicates a USBProxyService method which we needed access too
 *       without pulling in the rest of the proxy service code.
 *
 * @param   pDevice     Pointer to the device.
 */
DECLINLINE(void) deviceFreeMembers(PUSBDEVICE pDevice)
{
    RTStrFree((char *)pDevice->pszManufacturer);
    pDevice->pszManufacturer = NULL;
    RTStrFree((char *)pDevice->pszProduct);
    pDevice->pszProduct = NULL;
    RTStrFree((char *)pDevice->pszSerialNumber);
    pDevice->pszSerialNumber = NULL;

    RTStrFree((char *)pDevice->pszAddress);
    pDevice->pszAddress = NULL;
}

/**
 * Free one USB device created by the Linux enumeration code.
 *
 * @note This duplicates a USBProxyService method which we needed access too
 *       without pulling in the rest of the proxy service code.
 *
 * @param   pDevice     Pointer to the device. NULL is OK.
 */
DECLINLINE(void) deviceFree(PUSBDEVICE pDevice)
{
    if (pDevice)
    {
        deviceFreeMembers(pDevice);
        RTMemFree(pDevice);
    }
}

/**
 * Free a linked list of USB devices created by the Linux enumeration code.
 * @param  ppHead  Pointer to the first device in the linked list
 */
DECLINLINE(void) deviceListFree(PUSBDEVICE *ppHead)
{
    PUSBDEVICE pHead = *ppHead;
    while (pHead)
    {
        PUSBDEVICE pNext = pHead->pNext;
        deviceFree(pHead);
        pHead = pNext;
    }
    *ppHead = NULL;
}

RT_C_DECLS_BEGIN

extern bool USBProxyLinuxCheckDeviceRoot(const char *pcszRoot, bool fIsDeviceNodes);

#ifdef UNIT_TEST
void TestUSBSetupInit(const char *pcszUsbfsRoot, bool fUsbfsAccessible,
                      const char *pcszDevicesRoot, bool fDevicesAccessible,
                      int rcMethodInitResult);
void TestUSBSetEnv(const char *pcszEnvUsb, const char *pcszEnvUsbRoot);
#endif

extern int USBProxyLinuxChooseMethod(bool *pfUsingUsbfsDevices, const char **ppcszDevicesRoot);
#ifdef UNIT_TEST
extern void TestUSBSetAvailableUsbfsDevices(const char **pacszDeviceAddresses);
extern void TestUSBSetAccessibleFiles(const char **pacszAccessibleFiles);
#endif

extern PUSBDEVICE USBProxyLinuxGetDevices(const char *pcszDevicesRoot, bool fUseSysfs);

RT_C_DECLS_END

#endif /* !___USBGetDevices_h */

