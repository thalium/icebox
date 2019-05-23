/* $Id: tstUSBLinux.h $ */
/** @file
 * VirtualBox USB Proxy Service class, test version for Linux hosts.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


#ifndef ____H_TSTUSBLINUX
#define ____H_TSTUSBLINUX

typedef int HRESULT;
enum { S_OK = 0, E_NOTIMPL = 1 };

#include <VBox/usb.h>
#include <VBox/usbfilter.h>

#include <VBox/err.h>

#ifdef VBOX_USB_WITH_SYSFS
# include <libhal.h>
#endif

#include <stdio.h>
/**
 * The Linux hosted USB Proxy Service.
 */
class USBProxyServiceLinux
{
public:
    USBProxyServiceLinux()
        : mLastError(VINF_SUCCESS)
    {}

    HRESULT initSysfs(void);
    PUSBDEVICE getDevicesFromSysfs(void);
    int getLastError(void)
    {
        return mLastError;
    }

private:
    int start(void) { return VINF_SUCCESS; }
    static void freeDevice(PUSBDEVICE) {}  /* We don't care about leaks in a test. */
    int usbProbeInterfacesFromLibhal(const char *pszHalUuid, PUSBDEVICE pDev);
    int mLastError;
#  ifdef VBOX_USB_WITH_SYSFS
    /** Our connection to DBus for getting information from hal.  This will be
     * NULL if the initialisation failed. */
    DBusConnection *mDBusConnection;
    /** Handle to libhal. */
    LibHalContext *mLibHalContext;
#  endif
};

#endif /* !____H_TSTUSBLINUX */

