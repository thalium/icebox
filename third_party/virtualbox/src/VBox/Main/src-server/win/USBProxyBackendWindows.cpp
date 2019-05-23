/* $Id: USBProxyBackendWindows.cpp $ */
/** @file
 * VirtualBox USB Proxy Service, Windows Specialization.
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

#include <VBox/usb.h>
#include <VBox/err.h>

#include <iprt/string.h>
#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/err.h>

#include <VBox/usblib.h>


/**
 * Initialize data members.
 */
USBProxyBackendWindows::USBProxyBackendWindows()
    : USBProxyBackend(), mhEventInterrupt(INVALID_HANDLE_VALUE)
{
    LogFlowThisFunc(("\n"));
}

USBProxyBackendWindows::~USBProxyBackendWindows()
{
}

/**
 * Initializes the object (called right after construction).
 *
 * @returns S_OK on success and non-fatal failures, some COM error otherwise.
 */
int USBProxyBackendWindows::init(USBProxyService *aUsbProxyService, const com::Utf8Str &strId,
                                 const com::Utf8Str &strAddress, bool fLoadingSettings)
{
    USBProxyBackend::init(aUsbProxyService, strId, strAddress, fLoadingSettings);

    unconst(m_strBackend) = Utf8Str("host");

    /*
     * Create the semaphore (considered fatal).
     */
    mhEventInterrupt = CreateEvent(NULL, FALSE, FALSE, NULL);
    AssertReturn(mhEventInterrupt != INVALID_HANDLE_VALUE, VERR_OUT_OF_RESOURCES);

    /*
     * Initialize the USB lib and stuff.
     */
    int rc = USBLibInit();
    if (RT_SUCCESS(rc))
    {
        /*
         * Start the poller thread.
         */
        rc = start();
        if (RT_SUCCESS(rc))
        {
            LogFlowThisFunc(("returns successfully\n"));
            return VINF_SUCCESS;
        }

        USBLibTerm();
    }

    CloseHandle(mhEventInterrupt);
    mhEventInterrupt = INVALID_HANDLE_VALUE;

    LogFlowThisFunc(("returns failure!!! (rc=%Rrc)\n", rc));
    return rc;
}


/**
 * Stop all service threads and free the device chain.
 */
void USBProxyBackendWindows::uninit()
{
    LogFlowThisFunc(("\n"));

    /*
     * Stop the service.
     */
    if (isActive())
        stop();

    if (mhEventInterrupt != INVALID_HANDLE_VALUE)
        CloseHandle(mhEventInterrupt);
    mhEventInterrupt = INVALID_HANDLE_VALUE;

    /*
     * Terminate the library...
     */
    int rc = USBLibTerm();
    AssertRC(rc);
    USBProxyBackend::uninit();
}


void *USBProxyBackendWindows::insertFilter(PCUSBFILTER aFilter)
{
    AssertReturn(aFilter, NULL);

    LogFlow(("USBProxyBackendWindows::insertFilter()\n"));

    void *pvId = USBLibAddFilter(aFilter);

    LogFlow(("USBProxyBackendWindows::insertFilter(): returning pvId=%p\n", pvId));

    return pvId;
}


void USBProxyBackendWindows::removeFilter(void *aID)
{
    LogFlow(("USBProxyBackendWindows::removeFilter(): id=%p\n", aID));

    AssertReturnVoid(aID);

    USBLibRemoveFilter(aID);
}


int USBProxyBackendWindows::captureDevice(HostUSBDevice *aDevice)
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
     * Create a one-shot ignore filter for the device
     * and trigger a re-enumeration of it.
     */
    USBFILTER Filter;
    USBFilterInit(&Filter, USBFILTERTYPE_ONESHOT_CAPTURE);
    initFilterFromDevice(&Filter, aDevice);
    Log(("USBFILTERIDX_PORT=%#x\n", USBFilterGetNum(&Filter, USBFILTERIDX_PORT)));
    Log(("USBFILTERIDX_BUS=%#x\n", USBFilterGetNum(&Filter, USBFILTERIDX_BUS)));

    void *pvId = USBLibAddFilter(&Filter);
    if (!pvId)
    {
        AssertMsgFailed(("Add one-shot Filter failed\n"));
        return VERR_GENERAL_FAILURE;
    }

    int rc = USBLibRunFilters();
    if (!RT_SUCCESS(rc))
    {
        AssertMsgFailed(("Run Filters failed\n"));
        USBLibRemoveFilter(pvId);
        return rc;
    }

    return VINF_SUCCESS;
}


int USBProxyBackendWindows::releaseDevice(HostUSBDevice *aDevice)
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
    {
        AssertMsgFailed(("Add one-shot Filter failed\n"));
        return VERR_GENERAL_FAILURE;
    }

    int rc = USBLibRunFilters();
    if (!RT_SUCCESS(rc))
    {
        AssertMsgFailed(("Run Filters failed\n"));
        USBLibRemoveFilter(pvId);
        return rc;
    }


    return VINF_SUCCESS;
}


/**
 * Returns whether devices reported by this backend go through a de/re-attach
 * and device re-enumeration cycle when they are captured or released.
 */
bool USBProxyBackendWindows::i_isDevReEnumerationRequired()
{
    return true;
}


int USBProxyBackendWindows::wait(unsigned aMillies)
{
    return USBLibWaitChange(aMillies);
}


int USBProxyBackendWindows::interruptWait(void)
{
    return USBLibInterruptWaitChange();
}

/**
 * Gets a list of all devices the VM can grab
 */
PUSBDEVICE USBProxyBackendWindows::getDevices(void)
{
    PUSBDEVICE pDevices = NULL;
    uint32_t cDevices = 0;

    Log(("USBProxyBackendWindows::getDevices\n"));
    USBLibGetDevices(&pDevices, &cDevices);
    return pDevices;
}

