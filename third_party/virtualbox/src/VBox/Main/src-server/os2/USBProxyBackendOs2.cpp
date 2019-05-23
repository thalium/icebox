/* $Id: USBProxyBackendOs2.cpp $ */
/** @file
 * VirtualBox USB Proxy Service, OS/2 Specialization.
 */

/*
 * Copyright (C) 2005-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define INCL_BASE
#define INCL_ERRORS
#include "USBProxyBackend.h"
#include "Logging.h"

#include <VBox/usb.h>
#include <VBox/err.h>

#include <iprt/string.h>
#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/err.h>


/**
 * Initialize data members.
 */
USBProxyBackendOs2::USBProxyBackendOs2(USBProxyService *aUsbProxyService, const com::Utf8Str &strId)
    : USBProxyBackend(aUsbProxyService, strId), mhev(NULLHANDLE), mhmod(NULLHANDLE),
    mpfnUsbRegisterChangeNotification(NULL), mpfnUsbDeregisterNotification(NULL),
    mpfnUsbQueryNumberDevices(NULL), mpfnUsbQueryDeviceReport(NULL)
{
    LogFlowThisFunc(("aUsbProxyService=%p\n", aUsbProxyService));

    /*
     * Try initialize the usbcalls stuff.
     */
    int rc = DosCreateEventSem(NULL, &mhev, 0, FALSE);
    rc = RTErrConvertFromOS2(rc);
    if (RT_SUCCESS(rc))
    {
        rc = DosLoadModule(NULL, 0, (PCSZ)"usbcalls", &mhmod);
        rc = RTErrConvertFromOS2(rc);
        if (RT_SUCCESS(rc))
        {
            if (    (rc = DosQueryProcAddr(mhmod, 0, (PCSZ)"UsbQueryNumberDevices",         (PPFN)&mpfnUsbQueryNumberDevices))          == NO_ERROR
                &&  (rc = DosQueryProcAddr(mhmod, 0, (PCSZ)"UsbQueryDeviceReport",          (PPFN)&mpfnUsbQueryDeviceReport))           == NO_ERROR
                &&  (rc = DosQueryProcAddr(mhmod, 0, (PCSZ)"UsbRegisterChangeNotification", (PPFN)&mpfnUsbRegisterChangeNotification))  == NO_ERROR
                &&  (rc = DosQueryProcAddr(mhmod, 0, (PCSZ)"UsbDeregisterNotification",     (PPFN)&mpfnUsbDeregisterNotification))      == NO_ERROR
               )
            {
                rc = mpfnUsbRegisterChangeNotification(&mNotifyId, mhev, mhev);
                if (!rc)
                {
                    /*
                     * Start the poller thread.
                     */
                    rc = start();
                    if (RT_SUCCESS(rc))
                    {
                        LogFlowThisFunc(("returns successfully - mNotifyId=%d\n", mNotifyId));
                        mLastError = VINF_SUCCESS;
                        return;
                    }
                }

                LogRel(("USBProxyBackendOs2: failed to register change notification, rc=%d\n", rc));
            }
            else
                LogRel(("USBProxyBackendOs2: failed to load usbcalls\n"));

            DosFreeModule(mhmod);
        }
        else
            LogRel(("USBProxyBackendOs2: failed to load usbcalls, rc=%d\n", rc));
        mhmod = NULLHANDLE;
    }
    else
        mhev = NULLHANDLE;

    mLastError = rc;
    LogFlowThisFunc(("returns failure!!! (rc=%Rrc)\n", rc));
}


/**
 * Stop all service threads and free the device chain.
 */
USBProxyBackendOs2::~USBProxyBackendOs2()
{
    LogFlowThisFunc(("\n"));

    /*
     * Stop the service.
     */
    if (isActive())
        stop();

    /*
     * Free resources.
     */
    if (mhmod)
    {
        if (mpfnUsbDeregisterNotification)
            mpfnUsbDeregisterNotification(mNotifyId);

        mpfnUsbRegisterChangeNotification = NULL;
        mpfnUsbDeregisterNotification = NULL;
        mpfnUsbQueryNumberDevices = NULL;
        mpfnUsbQueryDeviceReport = NULL;

        DosFreeModule(mhmod);
        mhmod = NULLHANDLE;
    }
}


int USBProxyBackendOs2::captureDevice(HostUSBDevice *aDevice)
{
    AssertReturn(aDevice, VERR_GENERAL_FAILURE);
    AssertReturn(!aDevice->isWriteLockOnCurrentThread(), VERR_GENERAL_FAILURE);

    AutoReadLock devLock(aDevice COMMA_LOCKVAL_SRC_POS);
    LogFlowThisFunc(("aDevice=%s\n", aDevice->getName().c_str()));

    /*
     * Don't think we need to do anything when the device is held... fake it.
     */
    Assert(aDevice->isStatePending());
    devLock.release();
    interruptWait();

    return VINF_SUCCESS;
}


int USBProxyBackendOs2::releaseDevice(HostUSBDevice *aDevice)
{
    AssertReturn(aDevice, VERR_GENERAL_FAILURE);
    AssertReturn(!aDevice->isWriteLockOnCurrentThread(), VERR_GENERAL_FAILURE);

    AutoReadLock devLock(aDevice COMMA_LOCKVAL_SRC_POS);
    LogFlowThisFunc(("aDevice=%s\n", aDevice->getName().c_str()));

    /*
     * We're not really holding it atm., just fake it.
     */
    Assert(aDevice->isStatePending());
    devLock.release();
    interruptWait();

    return VINF_SUCCESS;
}


#if 0
bool USBProxyBackendOs2::updateDeviceState(HostUSBDevice *aDevice, PUSBDEVICE aUSBDevice, bool *aRunFilters,
                                           SessionMachine **aIgnoreMachine)
{
    AssertReturn(aDevice, false);
    AssertReturn(!aDevice->isWriteLockOnCurrentThread(), false);
    return updateDeviceStateFake(aDevice, aUSBDevice, aRunFilters, aIgnoreMachine);
}
#endif



int USBProxyBackendOs2::wait(RTMSINTERVAL aMillies)
{
    int rc = DosWaitEventSem(mhev, aMillies);
    return RTErrConvertFromOS2(rc);
}


int USBProxyBackendOs2::interruptWait(void)
{
    int rc = DosPostEventSem(mhev);
    return rc == NO_ERROR || rc == ERROR_ALREADY_POSTED
         ? VINF_SUCCESS
         : RTErrConvertFromOS2(rc);
}

#include <stdio.h>

PUSBDEVICE USBProxyBackendOs2::getDevices(void)
{
    /*
     * Count the devices.
     */
    ULONG cDevices = 0;
    int rc = mpfnUsbQueryNumberDevices((PULONG)&cDevices); /* Thanks to com/xpcom, PULONG and ULONG * aren't the same. */
    if (rc)
        return NULL;

    /*
     * Retrieve information about each device.
     */
    PUSBDEVICE pFirst = NULL;
    PUSBDEVICE *ppNext = &pFirst;
    for (ULONG i = 0; i < cDevices; i++)
    {
        /*
         * Query the device and config descriptors.
         */
        uint8_t abBuf[1024];
        ULONG cb = sizeof(abBuf);
        rc = mpfnUsbQueryDeviceReport(i + 1, (PULONG)&cb, &abBuf[0]); /* see above (PULONG) */
        if (rc)
            continue;
        PUSBDEVICEDESC pDevDesc = (PUSBDEVICEDESC)&abBuf[0];
        if (    cb < sizeof(*pDevDesc)
            ||  pDevDesc->bDescriptorType != USB_DT_DEVICE
            ||  pDevDesc->bLength < sizeof(*pDevDesc)
            ||  pDevDesc->bLength > sizeof(*pDevDesc) * 2)
            continue;
        PUSBCONFIGDESC pCfgDesc = (PUSBCONFIGDESC)&abBuf[pDevDesc->bLength];
        if (    pCfgDesc->bDescriptorType != USB_DT_CONFIG
            ||  pCfgDesc->bLength >= sizeof(*pCfgDesc))
            pCfgDesc = NULL;

        /*
         * Skip it if it's some kind of hub.
         */
        if (pDevDesc->bDeviceClass == USB_HUB_CLASSCODE)
            continue;

        /*
         * Allocate a new device node and initialize it with the basic stuff.
         */
        PUSBDEVICE pCur = (PUSBDEVICE)RTMemAlloc(sizeof(*pCur));
        pCur->bcdUSB = pDevDesc->bcdUSB;
        pCur->bDeviceClass = pDevDesc->bDeviceClass;
        pCur->bDeviceSubClass = pDevDesc->bDeviceSubClass;
        pCur->bDeviceProtocol = pDevDesc->bDeviceProtocol;
        pCur->idVendor = pDevDesc->idVendor;
        pCur->idProduct = pDevDesc->idProduct;
        pCur->bcdDevice = pDevDesc->bcdDevice;
        pCur->pszManufacturer = RTStrDup("");
        pCur->pszProduct = RTStrDup("");
        pCur->pszSerialNumber = NULL;
        pCur->u64SerialHash = 0;
        //pCur->bNumConfigurations = pDevDesc->bNumConfigurations;
        pCur->bNumConfigurations = 0;
        pCur->paConfigurations = NULL;
        pCur->enmState = USBDEVICESTATE_USED_BY_HOST_CAPTURABLE;
        pCur->enmSpeed = USBDEVICESPEED_UNKNOWN;
        pCur->pszAddress = NULL;
        RTStrAPrintf((char **)&pCur->pszAddress, "p=0x%04RX16;v=0x%04RX16;r=0x%04RX16;e=0x%08RX32",
                     pDevDesc->idProduct, pDevDesc->idVendor, pDevDesc->bcdDevice, i);

        pCur->bBus = 0;
        pCur->bLevel = 0;
        pCur->bDevNum = 0;
        pCur->bDevNumParent = 0;
        pCur->bPort = 0;
        pCur->bNumDevices = 0;
        pCur->bMaxChildren = 0;

        /* link it */
        pCur->pNext = NULL;
        pCur->pPrev = *ppNext;
        *ppNext = pCur;
        ppNext = &pCur->pNext;
    }

    return pFirst;
}

