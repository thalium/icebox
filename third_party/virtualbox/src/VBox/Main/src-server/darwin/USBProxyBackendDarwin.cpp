/* $Id: USBProxyBackendDarwin.cpp $ */
/** @file
 * VirtualBox USB Proxy Service (in VBoxSVC), Darwin Specialization.
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
#include "USBProxyBackend.h"
#include "Logging.h"
#include "iokit.h"

#include <VBox/usb.h>
#include <VBox/usblib.h>
#include <VBox/err.h>

#include <iprt/string.h>
#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/err.h>
#include <iprt/asm.h>


/**
 * Initialize data members.
 */
USBProxyBackendDarwin::USBProxyBackendDarwin()
    : USBProxyBackend(), mServiceRunLoopRef(NULL), mNotifyOpaque(NULL), mWaitABitNextTime(false), mUSBLibInitialized(false)
{
}

USBProxyBackendDarwin::~USBProxyBackendDarwin()
{
}

/**
 * Initializes the object (called right after construction).
 *
 * @returns VBox status code.
 */
int USBProxyBackendDarwin::init(USBProxyService *pUsbProxyService, const com::Utf8Str &strId,
                                const com::Utf8Str &strAddress, bool fLoadingSettings)
{
    USBProxyBackend::init(pUsbProxyService, strId, strAddress, fLoadingSettings);

    unconst(m_strBackend) = Utf8Str("host");

    /*
     * Initialize the USB library.
     */
    int rc = USBLibInit();
    if (RT_FAILURE(rc))
        return rc;

    mUSBLibInitialized = true;

    /*
     * Start the poller thread.
     */
    start();
    return VINF_SUCCESS;
}


/**
 * Stop all service threads and free the device chain.
 */
void USBProxyBackendDarwin::uninit()
{
    LogFlowThisFunc(("\n"));

    /*
     * Stop the service.
     */
    if (isActive())
        stop();

    /*
     * Terminate the USB library - it'll
     */
    if (mUSBLibInitialized)
    {
        USBLibTerm();
        mUSBLibInitialized = false;
    }

    USBProxyBackend::uninit();
}


void *USBProxyBackendDarwin::insertFilter(PCUSBFILTER aFilter)
{
    return USBLibAddFilter(aFilter);
}


void USBProxyBackendDarwin::removeFilter(void *aId)
{
    USBLibRemoveFilter(aId);
}


int USBProxyBackendDarwin::captureDevice(HostUSBDevice *aDevice)
{
    /*
     * Check preconditions.
     */
    AssertReturn(aDevice, VERR_GENERAL_FAILURE);
    AssertReturn(!aDevice->isWriteLockOnCurrentThread(), VERR_GENERAL_FAILURE);

    AutoReadLock devLock(aDevice COMMA_LOCKVAL_SRC_POS);
    LogFlowThisFunc(("aDevice=%s\n", aDevice->i_getName().c_str()));

    Assert(aDevice->i_getUnistate() == kHostUSBDeviceState_Capturing);

    /*
     * Create a one-shot capture filter for the device (don't
     * match on port) and trigger a re-enumeration of it.
     */
    USBFILTER Filter;
    USBFilterInit(&Filter, USBFILTERTYPE_ONESHOT_CAPTURE);
    initFilterFromDevice(&Filter, aDevice);

    void *pvId = USBLibAddFilter(&Filter);
    if (!pvId)
        return VERR_GENERAL_FAILURE;

    int rc = DarwinReEnumerateUSBDevice(aDevice->i_getUsbData());
    if (RT_SUCCESS(rc))
        aDevice->i_setBackendUserData(pvId);
    else
    {
        USBLibRemoveFilter(pvId);
        pvId = NULL;
    }
    LogFlowThisFunc(("returns %Rrc pvId=%p\n", rc, pvId));
    return rc;
}


void USBProxyBackendDarwin::captureDeviceCompleted(HostUSBDevice *aDevice, bool aSuccess)
{
    AssertReturnVoid(aDevice->isWriteLockOnCurrentThread());

    /*
     * Remove the one-shot filter if necessary.
     */
    LogFlowThisFunc(("aDevice=%s aSuccess=%RTbool mOneShotId=%p\n", aDevice->i_getName().c_str(), aSuccess, aDevice->i_getBackendUserData()));
    if (!aSuccess && aDevice->i_getBackendUserData())
        USBLibRemoveFilter(aDevice->i_getBackendUserData());
    aDevice->i_setBackendUserData(NULL);
    USBProxyBackend::captureDeviceCompleted(aDevice, aSuccess);
}


int USBProxyBackendDarwin::releaseDevice(HostUSBDevice *aDevice)
{
    /*
     * Check preconditions.
     */
    AssertReturn(aDevice, VERR_GENERAL_FAILURE);
    AssertReturn(!aDevice->isWriteLockOnCurrentThread(), VERR_GENERAL_FAILURE);

    AutoReadLock devLock(aDevice COMMA_LOCKVAL_SRC_POS);
    LogFlowThisFunc(("aDevice=%s\n", aDevice->i_getName().c_str()));

    Assert(aDevice->i_getUnistate() == kHostUSBDeviceState_ReleasingToHost);

    /*
     * Create a one-shot ignore filter for the device
     * and trigger a re-enumeration of it.
     */
    USBFILTER Filter;
    USBFilterInit(&Filter, USBFILTERTYPE_ONESHOT_IGNORE);
    initFilterFromDevice(&Filter, aDevice);
    Log(("USBFILTERIDX_PORT=%#x\n", USBFilterGetNum(&Filter, USBFILTERIDX_PORT)));
    Log(("USBFILTERIDX_BUS=%#x\n", USBFilterGetNum(&Filter, USBFILTERIDX_BUS)));

    void *pvId = USBLibAddFilter(&Filter);
    if (!pvId)
        return VERR_GENERAL_FAILURE;

    int rc = DarwinReEnumerateUSBDevice(aDevice->i_getUsbData());
    if (RT_SUCCESS(rc))
        aDevice->i_setBackendUserData(pvId);
    else
    {
        USBLibRemoveFilter(pvId);
        pvId = NULL;
    }
    LogFlowThisFunc(("returns %Rrc pvId=%p\n", rc, pvId));
    return rc;
}


void USBProxyBackendDarwin::releaseDeviceCompleted(HostUSBDevice *aDevice, bool aSuccess)
{
    AssertReturnVoid(aDevice->isWriteLockOnCurrentThread());

    /*
     * Remove the one-shot filter if necessary.
     */
    LogFlowThisFunc(("aDevice=%s aSuccess=%RTbool mOneShotId=%p\n", aDevice->i_getName().c_str(), aSuccess, aDevice->i_getBackendUserData()));
    if (!aSuccess && aDevice->i_getBackendUserData())
        USBLibRemoveFilter(aDevice->i_getBackendUserData());
    aDevice->i_setBackendUserData(NULL);
    USBProxyBackend::releaseDeviceCompleted(aDevice, aSuccess);
}


/**
 * Returns whether devices reported by this backend go through a de/re-attach
 * and device re-enumeration cycle when they are captured or released.
 */
bool USBProxyBackendDarwin::i_isDevReEnumerationRequired()
{
    return true;
}


int USBProxyBackendDarwin::wait(RTMSINTERVAL aMillies)
{
    SInt32 rc = CFRunLoopRunInMode(CFSTR(VBOX_IOKIT_MODE_STRING),
                                   mWaitABitNextTime && aMillies >= 1000
                                   ? 1.0 /* seconds */
                                   : aMillies >= 5000 /* Temporary measure to poll for status changes (MSD). */
                                   ? 5.0 /* seconds */
                                   : aMillies / 1000.0,
                                   true);
    mWaitABitNextTime = rc != kCFRunLoopRunTimedOut;

    return VINF_SUCCESS;
}


int USBProxyBackendDarwin::interruptWait(void)
{
    if (mServiceRunLoopRef)
        CFRunLoopStop(mServiceRunLoopRef);
    return 0;
}


PUSBDEVICE USBProxyBackendDarwin::getDevices(void)
{
    /* call iokit.cpp */
    return DarwinGetUSBDevices();
}


void USBProxyBackendDarwin::serviceThreadInit(void)
{
    mServiceRunLoopRef = CFRunLoopGetCurrent();
    mNotifyOpaque = DarwinSubscribeUSBNotifications();
}


void USBProxyBackendDarwin::serviceThreadTerm(void)
{
    DarwinUnsubscribeUSBNotifications(mNotifyOpaque);
    mServiceRunLoopRef = NULL;
}


/**
 * Wrapper called from iokit.cpp.
 *
 * @param   pCur    The USB device to free.
 */
void DarwinFreeUSBDeviceFromIOKit(PUSBDEVICE pCur)
{
    USBProxyBackend::freeDevice(pCur);
}

