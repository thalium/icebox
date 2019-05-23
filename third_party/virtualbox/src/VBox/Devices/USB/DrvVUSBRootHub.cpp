/* $Id: DrvVUSBRootHub.cpp $ */
/** @file
 * Virtual USB - Root Hub Driver.
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


/** @page pg_dev_vusb   VUSB - Virtual USB
 *
 * @todo read thru this and correct typos. Merge with old docs.
 *
 *
 * The Virtual USB component glues USB devices and host controllers together.
 * The VUSB takes the form of a PDM driver which is attached to the HCI. USB
 * devices are created by, attached to, and managed by the VUSB roothub. The
 * VUSB also exposes an interface which is used by Main to attach and detach
 * proxied USB devices.
 *
 *
 * @section sec_dev_vusb_urb     The Life of an URB
 *
 * The URB is created when the HCI calls the roothub (VUSB) method pfnNewUrb.
 * VUSB has a pool of URBs, if no free URBs are available a new one is
 * allocated. The returned URB starts life in the ALLOCATED state and all
 * fields are initialized with sensible defaults.
 *
 * The HCI then copies any request data into the URB if it's an host2dev
 * transfer. It then submits the URB by calling the pfnSubmitUrb roothub
 * method.
 *
 * pfnSubmitUrb will start by checking if it knows the device address, and if
 * it doesn't the URB is completed with a device-not-ready error. When the
 * device address is known to it, action is taken based on the kind of
 * transfer it is. There are four kinds of transfers: 1. control, 2. bulk,
 * 3. interrupt, and 4. isochronous. In either case something eventually ends
 * up being submitted to the device.
 *
 *
 * If an URB fails submitting, may or may not be completed. This depends on
 * heuristics in some cases and on the kind of failure in others. If
 * pfnSubmitUrb returns a failure, the HCI should retry submitting it at a
 * later time. If pfnSubmitUrb returns success the URB is submitted, and it
 * can even been completed.
 *
 * The URB is in the IN_FLIGHT state from the time it's successfully submitted
 * and till it's reaped or cancelled.
 *
 * When an URB transfer or in some case submit failure occurs, the pfnXferError
 * callback of the HCI is consulted about what to do. If pfnXferError indicates
 * that the URB should be retried, pfnSubmitUrb will fail. If it indicates that
 * it should fail, the URB will be completed.
 *
 * Completing an URB means that the URB status is set and the HCI
 * pfnXferCompletion callback is invoked with the URB. The HCI is the supposed
 * to report the transfer status to the guest OS. After completion the URB
 * is freed and returned to the pool, unless it was cancelled. If it was
 * cancelled it will have to await reaping before it's actually freed.
 *
 *
 * @subsection subsec_dev_vusb_urb_ctrl  Control
 *
 * The control transfer is the most complex one, from VUSB's point of view,
 * with its three stages and being bi-directional. A control transfer starts
 * with a SETUP packet containing the request description and two basic
 * parameters. It is followed by zero or more DATA packets which either picks
 * up incoming data (dev2host) or supplies the request data (host2dev). This
 * can then be followed by a STATUS packet which gets the status of the whole
 * transfer.
 *
 * What makes the control transfer complicated is that for a host2dev request
 * the URB is assembled from the SETUP and DATA stage, and for a dev2host
 * request the returned data must be kept around for the DATA stage. For both
 * transfer directions the status of the transfer has to be kept around for
 * the STATUS stage.
 *
 * To complicate matters further, VUSB must intercept and in some cases emulate
 * some of the standard requests in order to keep the virtual device state
 * correct and provide the correct virtualization of a device.
 *
 * @subsection subsec_dev_vusb_urb_bulk  Bulk and Interrupt
 *
 * The bulk and interrupt transfer types are relativly simple compared to the
 * control transfer. VUSB is not inspecting the request content or anything,
 * but passes it down the device.
 *
 * @subsection subsec_dev_vusb_urb_isoc  Isochronous
 *
 * This kind of transfers hasn't yet been implemented.
 *
 */


/** @page pg_dev_vusb_old   VUSB - Virtual USB Core
 *
 * The virtual USB core is controlled by the roothub and the underlying HCI
 * emulator, it is responsible for device addressing, managing configurations,
 * interfaces and endpoints, assembling and splitting multi-part control
 * messages and in general acts as a middle layer between the USB device
 * emulation code and USB HCI emulation code.
 *
 * All USB devices are represented by a struct vusb_dev. This structure
 * contains things like the device state, device address, all the configuration
 * descriptors, the currently selected configuration and a mapping between
 * endpoint addresses and endpoint descriptors.
 *
 * Each vusb_dev also has a pointer to a vusb_dev_ops structure which serves as
 * the virtual method table and includes a virtual constructor and destructor.
 * After a vusb_dev is created it may be attached to a hub device such as a
 * roothub (using vusbHubAttach). Although each hub structure has cPorts
 * and cDevices fields, it is the responsibility of the hub device to allocate
 * a free port for the new device.
 *
 * Devices can chose one of two interfaces for dealing with requests, the
 * synchronous interface or the asynchronous interface. The synchronous
 * interface is much simpler and ought to be used for devices which are
 * unlikely to sleep for long periods in order to serve requests. The
 * asynchronous interface on the other hand is more difficult to use but is
 * useful for the USB proxy or if one were to write a mass storage device
 * emulator. Currently the synchronous interface only supports control and bulk
 * endpoints and is no longer used by anything.
 *
 * In order to use the asynchronous interface, the queue_urb, cancel_urb and
 * pfnUrbReap fields must be set in the devices vusb_dev_ops structure. The
 * queue_urb method is used to submit a request to a device without blocking,
 * it returns 1 if successful and 0 on any kind of failure. A successfully
 * queued URB is completed when the pfnUrbReap method returns it. Each function
 * address is reference counted so that pfnUrbReap will only be called if there
 * are URBs outstanding. For a roothub to reap an URB from any one of it's
 * devices, the vusbRhReapAsyncUrbs() function is used.
 *
 * There are four types of messages an URB may contain:
 *      -# Control - represents a single packet of a multi-packet control
 *         transfer, these are only really used by the host controller to
 *         submit the parts to the usb core.
 *      -# Message - the usb core assembles multiple control transfers in
 *         to single message transfers. In this case the data buffer
 *         contains the setup packet in little endian followed by the full
 *         buffer. In the case of an host-to-device control message, the
 *         message packet is created when the STATUS transfer is seen. In
 *         the case of device-to-host  messages, the message packet is
 *         created after the SETUP transfer is seen. Also, certain control
 *         requests never go the real device and get handled synchronously.
 *      -# Bulk - Currently the only endpoint type that does error checking
 *         and endpoint halting.
 *      -# Interrupt - The only non-periodic type supported.
 *
 * Hubs are special cases of devices, they have a number of downstream ports
 * that other devices can be attached to and removed from.
 *
 * After a device has been attached (vusbHubAttach):
 *      -# The hub attach method is called, which sends a hub status
 *         change message to the OS.
 *      -# The OS resets the device, and it appears on the default
 *         address with it's config 0 selected (a pseudo-config that
 *         contains only 1 interface with 1 endpoint - the default
 *         message pipe).
 *      -# The OS assigns the device a new address and selects an
 *         appropriate config.
 *      -# The device is ready.
 *
 * After a device has been detached (vusbDevDetach):
 *      -# All pending URBs are cancelled.
 *      -# The devices address is unassigned.
 *      -# The hub detach method is called which signals the OS
 *         of the status change.
 *      -# The OS unlinks the ED's for that device.
 *
 * A device can also request detachment from within its own methods by
 * calling vusbDevUnplugged().
 *
 * Roothubs are responsible for driving the whole system, they are special
 * cases of hubs and as such implement attach and detach methods, each one
 * is described by a struct vusb_roothub. Once a roothub has submitted an
 * URB to the USB core, a number of callbacks to the roothub are required
 * for when the URB completes, since the roothub typically wants to inform
 * the OS when transfers are completed.
 *
 * There are four callbacks to be concerned with:
 *      -# prepare - This is called after the URB is successfully queued.
 *      -# completion - This is called after the URB completed.
 *      -# error - This is called if the URB errored, some systems have
 *         automatic resubmission of failed requests, so this callback
 *         should keep track of the error count and return 1 if the count
 *         is above the number of allowed resubmissions.
 *      -# halt_ep - This is called after errors on bulk pipes in order
 *         to halt the pipe.
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DRV_VUSB
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/vmapi.h>
#include <VBox/err.h>
#include <iprt/alloc.h>
#include <VBox/log.h>
#include <iprt/time.h>
#include <iprt/thread.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/uuid.h>
#include "VUSBInternal.h"
#include "VBoxDD.h"



/**
 * Attaches a device to a specific hub.
 *
 * This function is called by the vusb_add_device() and vusbRhAttachDevice().
 *
 * @returns VBox status code.
 * @param   pHub        The hub to attach it to.
 * @param   pDev        The device to attach.
 * @thread  EMT
 */
static int vusbHubAttach(PVUSBHUB pHub, PVUSBDEV pDev)
{
    LogFlow(("vusbHubAttach: pHub=%p[%s] pDev=%p[%s]\n", pHub, pHub->pszName, pDev, pDev->pUsbIns->pszName));
    AssertMsg(pDev->enmState == VUSB_DEVICE_STATE_DETACHED, ("enmState=%d\n", pDev->enmState));

    pDev->pHub = pHub;
    pDev->enmState = VUSB_DEVICE_STATE_ATTACHED;

    /* noone else ever messes with the default pipe while we are attached */
    vusbDevMapEndpoint(pDev, &g_Endpoint0);
    vusbDevDoSelectConfig(pDev, &g_Config0);

    int rc = pHub->pOps->pfnAttach(pHub, pDev);
    if (RT_FAILURE(rc))
    {
        pDev->pHub = NULL;
        pDev->enmState = VUSB_DEVICE_STATE_DETACHED;
    }
    return rc;
}


/* -=-=-=-=-=- PDMUSBHUBREG methods -=-=-=-=-=- */

/** @interface_method_impl{PDMUSBHUBREG,pfnAttachDevice} */
static DECLCALLBACK(int) vusbPDMHubAttachDevice(PPDMDRVINS pDrvIns, PPDMUSBINS pUsbIns, const char *pszCaptureFilename, uint32_t *piPort)
{
    PVUSBROOTHUB pThis = PDMINS_2_DATA(pDrvIns, PVUSBROOTHUB);

    /*
     * Allocate a new VUSB device and initialize it.
     */
    PVUSBDEV pDev = (PVUSBDEV)RTMemAllocZ(sizeof(*pDev));
    AssertReturn(pDev, VERR_NO_MEMORY);
    int rc = vusbDevInit(pDev, pUsbIns, pszCaptureFilename);
    if (RT_SUCCESS(rc))
    {
        pUsbIns->pvVUsbDev2 = pDev;
        rc = vusbHubAttach(&pThis->Hub, pDev);
        if (RT_SUCCESS(rc))
        {
            *piPort = UINT32_MAX; /// @todo implement piPort
            return rc;
        }

        RTMemFree(pDev->paIfStates);
        pUsbIns->pvVUsbDev2 = NULL;
    }
    vusbDevRelease(pDev);
    return rc;
}


/** @interface_method_impl{PDMUSBHUBREG,pfnDetachDevice} */
static DECLCALLBACK(int) vusbPDMHubDetachDevice(PPDMDRVINS pDrvIns, PPDMUSBINS pUsbIns, uint32_t iPort)
{
    RT_NOREF(pDrvIns, iPort);
    PVUSBDEV pDev = (PVUSBDEV)pUsbIns->pvVUsbDev2;
    Assert(pDev);

    /*
     * Deal with pending async reset.
     * (anything but reset)
     */
    vusbDevSetStateCmp(pDev, VUSB_DEVICE_STATE_DEFAULT, VUSB_DEVICE_STATE_RESET);

    /*
     * Detach and free resources.
     */
    if (pDev->pHub)
        vusbDevDetach(pDev);

    vusbDevRelease(pDev);
    return VINF_SUCCESS;
}

/**
 * The hub registration structure.
 */
static const PDMUSBHUBREG g_vusbHubReg =
{
    PDM_USBHUBREG_VERSION,
    vusbPDMHubAttachDevice,
    vusbPDMHubDetachDevice,
    PDM_USBHUBREG_VERSION
};


/* -=-=-=-=-=- VUSBIROOTHUBCONNECTOR methods -=-=-=-=-=- */


/**
 * Finds an device attached to a roothub by it's address.
 *
 * @returns Pointer to the device.
 * @returns NULL if not found.
 * @param   pRh         Pointer to the root hub.
 * @param   Address     The device address.
 */
static PVUSBDEV vusbRhFindDevByAddress(PVUSBROOTHUB pRh, uint8_t Address)
{
    unsigned iHash = vusbHashAddress(Address);
    PVUSBDEV pDev = NULL;

    RTCritSectEnter(&pRh->CritSectDevices);
    for (PVUSBDEV pCur = pRh->apAddrHash[iHash]; pCur; pCur = pCur->pNextHash)
        if (pCur->u8Address == Address)
        {
            pDev = pCur;
            break;
        }

    if (pDev)
        vusbDevRetain(pDev);
    RTCritSectLeave(&pRh->CritSectDevices);
    return pDev;
}


/**
 * Callback for freeing an URB.
 * @param   pUrb    The URB to free.
 */
static DECLCALLBACK(void) vusbRhFreeUrb(PVUSBURB pUrb)
{
    /*
     * Assert sanity.
     */
    vusbUrbAssert(pUrb);
    PVUSBROOTHUB pRh = (PVUSBROOTHUB)pUrb->pVUsb->pvFreeCtx;
    Assert(pRh);

    Assert(pUrb->enmState != VUSBURBSTATE_FREE);

    /*
     * Free the URB description (logging builds only).
     */
    if (pUrb->pszDesc)
    {
        RTStrFree(pUrb->pszDesc);
        pUrb->pszDesc = NULL;
    }

    /* The URB comes from the roothub if there is no device (invalid address). */
    if (pUrb->pVUsb->pDev)
    {
        PVUSBDEV pDev = pUrb->pVUsb->pDev;

        vusbUrbPoolFree(&pUrb->pVUsb->pDev->UrbPool, pUrb);
        vusbDevRelease(pDev);
    }
    else
        vusbUrbPoolFree(&pRh->Hub.Dev.UrbPool, pUrb);
}


/**
 * Worker routine for vusbRhConnNewUrb().
 */
static PVUSBURB vusbRhNewUrb(PVUSBROOTHUB pRh, uint8_t DstAddress, PVUSBDEV pDev, VUSBXFERTYPE enmType,
                             VUSBDIRECTION enmDir, uint32_t cbData, uint32_t cTds, const char *pszTag)
{
    RT_NOREF(pszTag);
    PVUSBURBPOOL pUrbPool = &pRh->Hub.Dev.UrbPool;

    if (!pDev)
        pDev = vusbRhFindDevByAddress(pRh, DstAddress);
    else
        vusbDevRetain(pDev);

    if (pDev)
        pUrbPool = &pDev->UrbPool;

    PVUSBURB pUrb = vusbUrbPoolAlloc(pUrbPool, enmType, enmDir, cbData,
                                     pRh->cbHci, pRh->cbHciTd, cTds);
    if (RT_LIKELY(pUrb))
    {
        pUrb->pVUsb->pvFreeCtx = pRh;
        pUrb->pVUsb->pfnFree   = vusbRhFreeUrb;
        pUrb->DstAddress       = DstAddress;
        pUrb->pVUsb->pDev      = pDev;

#ifdef LOG_ENABLED
        const char *pszType = NULL;

        switch(pUrb->enmType)
        {
            case VUSBXFERTYPE_CTRL:
                pszType = "ctrl";
                break;
            case VUSBXFERTYPE_INTR:
                pszType = "intr";
                break;
            case VUSBXFERTYPE_BULK:
                pszType = "bulk";
                break;
            case VUSBXFERTYPE_ISOC:
                pszType = "isoc";
                break;
            default:
                pszType = "invld";
                break;
        }

        pRh->iSerial = (pRh->iSerial + 1) % 10000;
        RTStrAPrintf(&pUrb->pszDesc, "URB %p %s%c%04d (%s)", pUrb, pszType,
                     (pUrb->enmDir == VUSBDIRECTION_IN) ? '<' : ((pUrb->enmDir == VUSBDIRECTION_SETUP) ? 's' : '>'),
                     pRh->iSerial, pszTag ? pszTag : "<none>");
#endif
    }

    return pUrb;
}


/**
 * Calculate frame timer variables given a frame rate.
 */
static void vusbRhR3CalcTimerIntervals(PVUSBROOTHUB pThis, uint32_t u32FrameRate)
{
    pThis->nsWait     = RT_NS_1SEC / u32FrameRate;
    pThis->uFrameRate = u32FrameRate;
    /* Inform the HCD about the new frame rate. */
    pThis->pIRhPort->pfnFrameRateChanged(pThis->pIRhPort, u32FrameRate);
}


/**
 * Calculates the new frame rate based on the idle detection and number of idle
 * cycles.
 *
 * @returns nothing.
 * @param   pThis    The roothub instance data.
 * @param   fIdle    Flag whether the last frame didn't produce any activity.
 */
static void vusbRhR3FrameRateCalcNew(PVUSBROOTHUB pThis, bool fIdle)
{
    uint32_t uNewFrameRate = pThis->uFrameRate;

    /*
     * Adjust the frame timer interval based on idle detection.
     */
    if (fIdle)
    {
        pThis->cIdleCycles++;
        /* Set the new frame rate based on how long we've been idle. Tunable. */
        switch (pThis->cIdleCycles)
        {
            case 4: uNewFrameRate = 500;    break;  /*  2ms interval */
            case 16:uNewFrameRate = 125;    break;  /*  8ms interval */
            case 24:uNewFrameRate = 50;     break;  /* 20ms interval */
            default:    break;
        }
        /* Avoid overflow. */
        if (pThis->cIdleCycles > 60000)
            pThis->cIdleCycles = 20000;
    }
    else
    {
        if (pThis->cIdleCycles)
        {
            pThis->cIdleCycles = 0;
            uNewFrameRate      = pThis->uFrameRateDefault;
        }
    }

    if (   uNewFrameRate != pThis->uFrameRate
        && uNewFrameRate)
    {
        LogFlow(("Frame rate changed from %u to %u\n", pThis->uFrameRate, uNewFrameRate));
        vusbRhR3CalcTimerIntervals(pThis, uNewFrameRate);
    }
}


/**
 * The core frame processing routine keeping track of the elapsed time and calling into
 * the device emulation above us to do the work.
 *
 * @returns Relative timespan when to process the next frame.
 * @param   pThis     The roothub instance data.
 * @param   fCallback Flag whether this method is called from the URB completion callback or
 *                    from the worker thread (only used for statistics).
 */
DECLHIDDEN(uint64_t) vusbRhR3ProcessFrame(PVUSBROOTHUB pThis, bool fCallback)
{
    uint64_t tsNext = 0;
    uint64_t tsNanoStart = RTTimeNanoTS();

    /* Don't do anything if we are not supposed to process anything (EHCI and XHCI). */
    if (!pThis->uFrameRateDefault)
        return 0;

    if (ASMAtomicXchgBool(&pThis->fFrameProcessing, true))
        return pThis->nsWait;

    if (   tsNanoStart > pThis->tsFrameProcessed
        && tsNanoStart - pThis->tsFrameProcessed >= 750 * RT_NS_1US)
    {
        LogFlowFunc(("Starting new frame at ts %llu\n", tsNanoStart));

        bool fIdle = pThis->pIRhPort->pfnStartFrame(pThis->pIRhPort, 0 /* u32FrameNo */);
        vusbRhR3FrameRateCalcNew(pThis, fIdle);

        uint64_t tsNow = RTTimeNanoTS();
        tsNext = (tsNanoStart + pThis->nsWait) > tsNow ? (tsNanoStart + pThis->nsWait) - tsNow : 0;
        pThis->tsFrameProcessed = tsNanoStart;
        LogFlowFunc(("Current frame took %llu nano seconds to process, next frame in %llu ns\n", tsNow - tsNanoStart, tsNext));
        if (fCallback)
            STAM_COUNTER_INC(&pThis->StatFramesProcessedClbk);
        else
            STAM_COUNTER_INC(&pThis->StatFramesProcessedThread);
    }
    else
    {
        tsNext = (pThis->tsFrameProcessed + pThis->nsWait) > tsNanoStart ? (pThis->tsFrameProcessed + pThis->nsWait) - tsNanoStart : 0;
        LogFlowFunc(("Next frame is too far away in the future, waiting... (tsNanoStart=%llu tsFrameProcessed=%llu)\n",
                     tsNanoStart, pThis->tsFrameProcessed));
    }

    ASMAtomicXchgBool(&pThis->fFrameProcessing, false);
    LogFlowFunc(("returns %llu\n", tsNext));
    return tsNext;
}


/**
 * Worker for processing frames periodically.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The driver instance.
 * @param   pThread     The PDM thread structure for the thread this worker runs on.
 */
static DECLCALLBACK(int) vusbRhR3PeriodFrameWorker(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    RT_NOREF(pDrvIns);
    int rc = VINF_SUCCESS;
    PVUSBROOTHUB pThis = (PVUSBROOTHUB)pThread->pvUser;

    if (pThread->enmState == PDMTHREADSTATE_INITIALIZING)
        return VINF_SUCCESS;

    while (pThread->enmState == PDMTHREADSTATE_RUNNING)
    {
        while (   !ASMAtomicReadU32(&pThis->uFrameRateDefault)
               && pThread->enmState == PDMTHREADSTATE_RUNNING)
        {
            /* Signal the waiter that we are stopped now. */
            rc = RTSemEventMultiSignal(pThis->hSemEventPeriodFrameStopped);
            AssertRC(rc);

            rc = RTSemEventMultiWait(pThis->hSemEventPeriodFrame, RT_INDEFINITE_WAIT);
            RTSemEventMultiReset(pThis->hSemEventPeriodFrame);

            /*
             * Notify the device above about the frame rate changed if we are supposed to
             * process frames.
             */
            uint32_t uFrameRate = ASMAtomicReadU32(&pThis->uFrameRateDefault);
            if (uFrameRate)
                vusbRhR3CalcTimerIntervals(pThis, uFrameRate);
        }

        AssertLogRelMsgReturn(RT_SUCCESS(rc) || rc == VERR_TIMEOUT, ("%Rrc\n", rc), rc);
        if (RT_UNLIKELY(pThread->enmState != PDMTHREADSTATE_RUNNING))
            break;

        uint64_t tsNext = vusbRhR3ProcessFrame(pThis, false /* fCallback */);

        if (tsNext >= 250 * RT_NS_1US)
        {
            rc = RTSemEventMultiWaitEx(pThis->hSemEventPeriodFrame, RTSEMWAIT_FLAGS_RELATIVE | RTSEMWAIT_FLAGS_NANOSECS | RTSEMWAIT_FLAGS_UNINTERRUPTIBLE,
                                       tsNext);
            AssertLogRelMsg(RT_SUCCESS(rc) || rc == VERR_TIMEOUT, ("%Rrc\n", rc));
            RTSemEventMultiReset(pThis->hSemEventPeriodFrame);
        }
    }

    return VINF_SUCCESS;
}


/**
 * Unblock the periodic frame thread so it can respond to a state change.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The driver instance.
 * @param   pThread     The send thread.
 */
static DECLCALLBACK(int) vusbRhR3PeriodFrameWorkerWakeup(PPDMDRVINS pDrvIns, PPDMTHREAD pThread)
{
    RT_NOREF(pThread);
    PVUSBROOTHUB pThis = PDMINS_2_DATA(pDrvIns, PVUSBROOTHUB);
    return RTSemEventMultiSignal(pThis->hSemEventPeriodFrame);
}


/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnSetUrbParams} */
static DECLCALLBACK(int) vusbRhSetUrbParams(PVUSBIROOTHUBCONNECTOR pInterface, size_t cbHci, size_t cbHciTd)
{
    PVUSBROOTHUB pRh = VUSBIROOTHUBCONNECTOR_2_VUSBROOTHUB(pInterface);

    pRh->cbHci   = cbHci;
    pRh->cbHciTd = cbHciTd;

    return VINF_SUCCESS;
}


/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnNewUrb} */
static DECLCALLBACK(PVUSBURB) vusbRhConnNewUrb(PVUSBIROOTHUBCONNECTOR pInterface, uint8_t DstAddress, PVUSBIDEVICE pDev, VUSBXFERTYPE enmType,
                                               VUSBDIRECTION enmDir, uint32_t cbData, uint32_t cTds, const char *pszTag)
{
    PVUSBROOTHUB pRh = VUSBIROOTHUBCONNECTOR_2_VUSBROOTHUB(pInterface);
    return vusbRhNewUrb(pRh, DstAddress, (PVUSBDEV)pDev, enmType, enmDir, cbData, cTds, pszTag);
}


/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnFreeUrb} */
static DECLCALLBACK(int) vusbRhConnFreeUrb(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBURB pUrb)
{
    RT_NOREF(pInterface);
    pUrb->pVUsb->pfnFree(pUrb);
    return VINF_SUCCESS;
}


/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnSubmitUrb} */
static DECLCALLBACK(int) vusbRhSubmitUrb(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBURB pUrb, PPDMLED pLed)
{
    PVUSBROOTHUB pRh = VUSBIROOTHUBCONNECTOR_2_VUSBROOTHUB(pInterface);
    STAM_PROFILE_START(&pRh->StatSubmitUrb, a);

#ifdef VBOX_WITH_STATISTICS
    /*
     * Total and per-type submit statistics.
     */
    Assert(pUrb->enmType >= 0 && pUrb->enmType < (int)RT_ELEMENTS(pRh->aTypes));
    STAM_COUNTER_INC(&pRh->Total.StatUrbsSubmitted);
    STAM_COUNTER_INC(&pRh->aTypes[pUrb->enmType].StatUrbsSubmitted);

    STAM_COUNTER_ADD(&pRh->Total.StatReqBytes, pUrb->cbData);
    STAM_COUNTER_ADD(&pRh->aTypes[pUrb->enmType].StatReqBytes, pUrb->cbData);
    if (pUrb->enmDir == VUSBDIRECTION_IN)
    {
        STAM_COUNTER_ADD(&pRh->Total.StatReqReadBytes, pUrb->cbData);
        STAM_COUNTER_ADD(&pRh->aTypes[pUrb->enmType].StatReqReadBytes, pUrb->cbData);
    }
    else
    {
        STAM_COUNTER_ADD(&pRh->Total.StatReqWriteBytes, pUrb->cbData);
        STAM_COUNTER_ADD(&pRh->aTypes[pUrb->enmType].StatReqWriteBytes, pUrb->cbData);
    }

    if (pUrb->enmType == VUSBXFERTYPE_ISOC)
    {
        STAM_COUNTER_ADD(&pRh->StatIsocReqPkts, pUrb->cIsocPkts);
        if (pUrb->enmDir == VUSBDIRECTION_IN)
            STAM_COUNTER_ADD(&pRh->StatIsocReqReadPkts, pUrb->cIsocPkts);
        else
            STAM_COUNTER_ADD(&pRh->StatIsocReqWritePkts, pUrb->cIsocPkts);
    }
#endif

    /* If there is a sniffer on the roothub record the URB there. */
    if (pRh->hSniffer != VUSBSNIFFER_NIL)
    {
        int rc = VUSBSnifferRecordEvent(pRh->hSniffer, pUrb, VUSBSNIFFEREVENT_SUBMIT);
        if (RT_FAILURE(rc))
            LogRel(("VUSB: Capturing URB submit event on the root hub failed with %Rrc\n", rc));
    }

    /*
     * The device was resolved when we allocated the URB.
     * Submit it to the device if we found it, if not fail with device-not-ready.
     */
    int rc;
    if (   pUrb->pVUsb->pDev
        && pUrb->pVUsb->pDev->pUsbIns)
    {
        switch (pUrb->enmDir)
        {
            case VUSBDIRECTION_IN:
                pLed->Asserted.s.fReading = pLed->Actual.s.fReading = 1;
                rc = vusbUrbSubmit(pUrb);
                pLed->Actual.s.fReading = 0;
                break;
            case VUSBDIRECTION_OUT:
                pLed->Asserted.s.fWriting = pLed->Actual.s.fWriting = 1;
                rc = vusbUrbSubmit(pUrb);
                pLed->Actual.s.fWriting = 0;
                break;
            default:
                rc = vusbUrbSubmit(pUrb);
                break;
        }

        if (RT_FAILURE(rc))
        {
            LogFlow(("vusbRhSubmitUrb: freeing pUrb=%p\n", pUrb));
            pUrb->pVUsb->pfnFree(pUrb);
        }
    }
    else
    {
        vusbDevRetain(&pRh->Hub.Dev);
        pUrb->pVUsb->pDev = &pRh->Hub.Dev;
        Log(("vusb: pRh=%p: SUBMIT: Address %i not found!!!\n", pRh, pUrb->DstAddress));

        pUrb->enmState = VUSBURBSTATE_REAPED;
        pUrb->enmStatus = VUSBSTATUS_DNR;
        vusbUrbCompletionRh(pUrb);
        rc = VINF_SUCCESS;
    }

    STAM_PROFILE_STOP(&pRh->StatSubmitUrb, a);
    return rc;
}


static DECLCALLBACK(int) vusbRhReapAsyncUrbsWorker(PVUSBDEV pDev, RTMSINTERVAL cMillies)
{
    if (!cMillies)
        vusbUrbDoReapAsync(&pDev->LstAsyncUrbs, 0);
    else
    {
        uint64_t u64Start = RTTimeMilliTS();
        do
        {
            vusbUrbDoReapAsync(&pDev->LstAsyncUrbs, RT_MIN(cMillies >> 8, 10));
        } while (   !RTListIsEmpty(&pDev->LstAsyncUrbs)
                 && RTTimeMilliTS() - u64Start < cMillies);
    }

    return VINF_SUCCESS;
}

/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnReapAsyncUrbs} */
static DECLCALLBACK(void) vusbRhReapAsyncUrbs(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBIDEVICE pDevice, RTMSINTERVAL cMillies)
{
    PVUSBROOTHUB pRh = VUSBIROOTHUBCONNECTOR_2_VUSBROOTHUB(pInterface); NOREF(pRh);
    PVUSBDEV pDev = (PVUSBDEV)pDevice;

    if (RTListIsEmpty(&pDev->LstAsyncUrbs))
        return;

    STAM_PROFILE_START(&pRh->StatReapAsyncUrbs, a);
    int rc = vusbDevIoThreadExecSync(pDev, (PFNRT)vusbRhReapAsyncUrbsWorker, 2, pDev, cMillies);
    AssertRC(rc);
    STAM_PROFILE_STOP(&pRh->StatReapAsyncUrbs, a);
}


/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnCancelUrbsEp} */
static DECLCALLBACK(int) vusbRhCancelUrbsEp(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBURB pUrb)
{
    PVUSBROOTHUB pRh = VUSBIROOTHUBCONNECTOR_2_VUSBROOTHUB(pInterface);
    AssertReturn(pRh, VERR_INVALID_PARAMETER);
    AssertReturn(pUrb, VERR_INVALID_PARAMETER);

    /// @todo This method of URB canceling may not work on non-Linux hosts.
    /*
     * Cancel and reap the URB(s) on an endpoint.
     */
    LogFlow(("vusbRhCancelUrbsEp: pRh=%p pUrb=%p\n", pRh, pUrb));

    vusbUrbCancelAsync(pUrb, CANCELMODE_UNDO);

    /* The reaper thread will take care of completing the URB. */

    return VINF_SUCCESS;
}

/**
 * Worker doing the actual cancelling of all outstanding URBs on the device I/O thread.
 *
 * @returns VBox status code.
 * @param   pDev    USB device instance data.
 */
static DECLCALLBACK(int) vusbRhCancelAllUrbsWorker(PVUSBDEV pDev)
{
    /*
     * Cancel the URBS.
     *
     * Not using th CritAsyncUrbs critical section here is safe
     * as the I/O thread is the only thread accessing this struture at the
     * moment.
     */
    PVUSBURBVUSB pVUsbUrb, pVUsbUrbNext;
    RTListForEachSafe(&pDev->LstAsyncUrbs, pVUsbUrb, pVUsbUrbNext, VUSBURBVUSBINT, NdLst)
    {
        PVUSBURB pUrb = pVUsbUrb->pUrb;
        /* Call the worker directly. */
        vusbUrbCancelWorker(pUrb, CANCELMODE_FAIL);
    }

    return VINF_SUCCESS;
}

/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnCancelAllUrbs} */
static DECLCALLBACK(void) vusbRhCancelAllUrbs(PVUSBIROOTHUBCONNECTOR pInterface)
{
    PVUSBROOTHUB pRh = VUSBIROOTHUBCONNECTOR_2_VUSBROOTHUB(pInterface);

    RTCritSectEnter(&pRh->CritSectDevices);
    PVUSBDEV pDev = pRh->pDevices;
    while (pDev)
    {
        vusbDevIoThreadExecSync(pDev, (PFNRT)vusbRhCancelAllUrbsWorker, 1, pDev);
        pDev = pDev->pNext;
    }
    RTCritSectLeave(&pRh->CritSectDevices);
}

/**
 * Worker doing the actual cancelling of all outstanding per-EP URBs on the
 * device I/O thread.
 *
 * @returns VBox status code.
 * @param   pDev    USB device instance data.
 * @param   EndPt   Endpoint number.
 * @param   enmDir  Endpoint direction.
 */
static DECLCALLBACK(int) vusbRhAbortEpWorker(PVUSBDEV pDev, int EndPt, VUSBDIRECTION enmDir)
{
    /*
     * Iterate the URBs, find ones corresponding to given EP, and cancel them.
     */
    PVUSBURBVUSB pVUsbUrb, pVUsbUrbNext;
    RTListForEachSafe(&pDev->LstAsyncUrbs, pVUsbUrb, pVUsbUrbNext, VUSBURBVUSBINT, NdLst)
    {
        PVUSBURB pUrb = pVUsbUrb->pUrb;

        Assert(pUrb->pVUsb->pDev == pDev);

        /* For the default control EP, direction does not matter. */
        if (pUrb->EndPt == EndPt && (pUrb->enmDir == enmDir || !EndPt))
        {
            LogFlow(("%s: vusbRhAbortEpWorker: CANCELING URB\n", pUrb->pszDesc));
            int rc = vusbUrbCancelWorker(pUrb, CANCELMODE_UNDO);
            AssertRC(rc);
        }
    }

    return VINF_SUCCESS;
}


/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnAbortEp} */
static DECLCALLBACK(int) vusbRhAbortEp(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBIDEVICE pDevice, int EndPt, VUSBDIRECTION enmDir)
{
    PVUSBROOTHUB pRh = VUSBIROOTHUBCONNECTOR_2_VUSBROOTHUB(pInterface);
    if (&pRh->Hub != ((PVUSBDEV)pDevice)->pHub)
        AssertFailedReturn(VERR_INVALID_PARAMETER);

    RTCritSectEnter(&pRh->CritSectDevices);
    PVUSBDEV pDev = (PVUSBDEV)pDevice;
    vusbDevIoThreadExecSync(pDev, (PFNRT)vusbRhAbortEpWorker, 3, pDev, EndPt, enmDir);
    RTCritSectLeave(&pRh->CritSectDevices);

    /* The reaper thread will take care of completing the URB. */

    return VINF_SUCCESS;
}


/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnAttachDevice} */
static DECLCALLBACK(int) vusbRhAttachDevice(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBIDEVICE pDevice)
{
    PVUSBROOTHUB pRh = VUSBIROOTHUBCONNECTOR_2_VUSBROOTHUB(pInterface);
    return vusbHubAttach(&pRh->Hub, (PVUSBDEV)pDevice);
}


/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnDetachDevice} */
static DECLCALLBACK(int) vusbRhDetachDevice(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBIDEVICE pDevice)
{
    PVUSBROOTHUB pRh = VUSBIROOTHUBCONNECTOR_2_VUSBROOTHUB(pInterface);
    if (&pRh->Hub != ((PVUSBDEV)pDevice)->pHub)
        AssertFailedReturn(VERR_INVALID_PARAMETER);
    return vusbDevDetach((PVUSBDEV)pDevice);
}


/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnSetPeriodicFrameProcessing} */
static DECLCALLBACK(int) vusbRhSetFrameProcessing(PVUSBIROOTHUBCONNECTOR pInterface, uint32_t uFrameRate)
{
    int rc = VINF_SUCCESS;
    PVUSBROOTHUB pThis = VUSBIROOTHUBCONNECTOR_2_VUSBROOTHUB(pInterface);

    /* Create the frame thread lazily. */
    if (   !pThis->hThreadPeriodFrame
        && uFrameRate)
    {
        ASMAtomicXchgU32(&pThis->uFrameRateDefault, uFrameRate);
        pThis->uFrameRate = uFrameRate;
        vusbRhR3CalcTimerIntervals(pThis, uFrameRate);

        rc = RTSemEventMultiCreate(&pThis->hSemEventPeriodFrame);
        AssertRCReturn(rc, rc);

        rc = RTSemEventMultiCreate(&pThis->hSemEventPeriodFrameStopped);
        AssertRCReturn(rc, rc);

        rc = PDMDrvHlpThreadCreate(pThis->pDrvIns, &pThis->hThreadPeriodFrame, pThis, vusbRhR3PeriodFrameWorker,
                                   vusbRhR3PeriodFrameWorkerWakeup, 0, RTTHREADTYPE_IO, "VUsbPeriodFrm");
        AssertRCReturn(rc, rc);

        VMSTATE enmState = PDMDrvHlpVMState(pThis->pDrvIns);
        if (   enmState == VMSTATE_RUNNING
            || enmState == VMSTATE_RUNNING_LS
            || enmState == VMSTATE_RUNNING_FT)
        {
            rc = PDMR3ThreadResume(pThis->hThreadPeriodFrame);
            AssertRCReturn(rc, rc);
        }
    }
    else if (   pThis->hThreadPeriodFrame
             && !uFrameRate)
    {
        /* Stop processing. */
        uint32_t uFrameRateOld = ASMAtomicXchgU32(&pThis->uFrameRateDefault, uFrameRate);
        if (uFrameRateOld)
        {
            rc = RTSemEventMultiReset(pThis->hSemEventPeriodFrameStopped);
            AssertRC(rc);

            /* Signal the frame thread to stop. */
            RTSemEventMultiSignal(pThis->hSemEventPeriodFrame);

            /* Wait for signal from the thread that it stopped. */
            rc = RTSemEventMultiWait(pThis->hSemEventPeriodFrameStopped, RT_INDEFINITE_WAIT);
            AssertRC(rc);
        }
    }
    else if (   pThis->hThreadPeriodFrame
             && uFrameRate)
    {
        /* Just switch to the new frame rate and let the periodic frame thread pick it up. */
        uint32_t uFrameRateOld = ASMAtomicXchgU32(&pThis->uFrameRateDefault, uFrameRate);

        /* Signal the frame thread to continue if it was stopped. */
        if (!uFrameRateOld)
            RTSemEventMultiSignal(pThis->hSemEventPeriodFrame);
    }

    return rc;
}


/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnGetPeriodicFrameRate} */
static DECLCALLBACK(uint32_t) vusbRhGetPeriodicFrameRate(PVUSBIROOTHUBCONNECTOR pInterface)
{
    PVUSBROOTHUB pThis = VUSBIROOTHUBCONNECTOR_2_VUSBROOTHUB(pInterface);

    return pThis->uFrameRate;
}

/** @interface_method_impl{VUSBIROOTHUBCONNECTOR,pfnUpdateIsocFrameDelta} */
static DECLCALLBACK(uint32_t) vusbRhUpdateIsocFrameDelta(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBIDEVICE pDevice,
                                                         int EndPt, VUSBDIRECTION enmDir, uint16_t uNewFrameID, uint8_t uBits)
{
    PVUSBROOTHUB    pRh = VUSBIROOTHUBCONNECTOR_2_VUSBROOTHUB(pInterface);
    AssertReturn(pRh, 0);
    PVUSBDEV        pDev = (PVUSBDEV)pDevice;
    PVUSBPIPE       pPipe = &pDev->aPipes[EndPt];
    uint32_t        *puLastFrame;
    int32_t         uFrameDelta;
    uint32_t        uMaxVal = 1 << uBits;

    puLastFrame  = enmDir == VUSBDIRECTION_IN ? &pPipe->uLastFrameIn : &pPipe->uLastFrameOut;
    uFrameDelta  = uNewFrameID - *puLastFrame;
    *puLastFrame = uNewFrameID;
    /* Take care of wrap-around. */
    if (uFrameDelta < 0)
        uFrameDelta += uMaxVal;

    return (uint16_t)uFrameDelta;
}

/* -=-=-=-=-=- VUSB Device methods (for the root hub) -=-=-=-=-=- */


/**
 * @interface_method_impl{VUSBIDEVICE,pfnReset}
 */
static DECLCALLBACK(int) vusbRhDevReset(PVUSBIDEVICE pInterface, bool fResetOnLinux,
                                        PFNVUSBRESETDONE pfnDone, void *pvUser, PVM pVM)
{
    RT_NOREF(pfnDone, pvUser, pVM);
    PVUSBROOTHUB pRh = RT_FROM_MEMBER(pInterface, VUSBROOTHUB, Hub.Dev.IDevice);
    Assert(!pfnDone);
    return pRh->pIRhPort->pfnReset(pRh->pIRhPort, fResetOnLinux); /** @todo change rc from bool to vbox status everywhere! */
}


/**
 * @interface_method_impl{VUSBIDEVICE,pfnPowerOn}
 */
static DECLCALLBACK(int) vusbRhDevPowerOn(PVUSBIDEVICE pInterface)
{
    PVUSBROOTHUB pRh = RT_FROM_MEMBER(pInterface, VUSBROOTHUB, Hub.Dev.IDevice);
    LogFlow(("vusbRhDevPowerOn: pRh=%p\n", pRh));

    Assert(     pRh->Hub.Dev.enmState != VUSB_DEVICE_STATE_DETACHED
           &&   pRh->Hub.Dev.enmState != VUSB_DEVICE_STATE_RESET);

    if (pRh->Hub.Dev.enmState == VUSB_DEVICE_STATE_ATTACHED)
        pRh->Hub.Dev.enmState = VUSB_DEVICE_STATE_POWERED;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{VUSBIDEVICE,pfnPowerOff}
 */
static DECLCALLBACK(int) vusbRhDevPowerOff(PVUSBIDEVICE pInterface)
{
    PVUSBROOTHUB pRh = RT_FROM_MEMBER(pInterface, VUSBROOTHUB, Hub.Dev.IDevice);
    LogFlow(("vusbRhDevPowerOff: pRh=%p\n", pRh));

    Assert(     pRh->Hub.Dev.enmState != VUSB_DEVICE_STATE_DETACHED
           &&   pRh->Hub.Dev.enmState != VUSB_DEVICE_STATE_RESET);

    /*
     * Cancel all URBs and reap them.
     */
    VUSBIRhCancelAllUrbs(&pRh->IRhConnector);
    RTCritSectEnter(&pRh->CritSectDevices);
    PVUSBDEV pDev = pRh->pDevices;
    while (pDev)
    {
        VUSBIRhReapAsyncUrbs(&pRh->IRhConnector, (PVUSBIDEVICE)pDev, 0);
        pDev = pDev->pNext;
    }
    RTCritSectLeave(&pRh->CritSectDevices);

    pRh->Hub.Dev.enmState = VUSB_DEVICE_STATE_ATTACHED;
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{VUSBIDEVICE,pfnGetState}
 */
static DECLCALLBACK(VUSBDEVICESTATE) vusbRhDevGetState(PVUSBIDEVICE pInterface)
{
    PVUSBROOTHUB pRh = RT_FROM_MEMBER(pInterface, VUSBROOTHUB, Hub.Dev.IDevice);
    return pRh->Hub.Dev.enmState;
}


static const char *vusbGetSpeedString(VUSBSPEED enmSpeed)
{
    const char  *pszSpeed = NULL;

    switch (enmSpeed)
    {
        case VUSB_SPEED_LOW:
            pszSpeed = "Low";
            break;
        case VUSB_SPEED_FULL:
            pszSpeed = "Full";
            break;
        case VUSB_SPEED_HIGH:
            pszSpeed = "High";
            break;
        case VUSB_SPEED_VARIABLE:
            pszSpeed = "Variable";
            break;
        case VUSB_SPEED_SUPER:
            pszSpeed = "Super";
            break;
        case VUSB_SPEED_SUPERPLUS:
            pszSpeed = "SuperPlus";
            break;
        default:
            pszSpeed = "Unknown";
            break;
    }
    return pszSpeed;
}

/* -=-=-=-=-=- VUSB Hub methods -=-=-=-=-=- */


/**
 * Attach the device to the hub.
 * Port assignments and all such stuff is up to this routine.
 *
 * @returns VBox status code.
 * @param   pHub        Pointer to the hub.
 * @param   pDev        Pointer to the device.
 */
static int vusbRhHubOpAttach(PVUSBHUB pHub, PVUSBDEV pDev)
{
    PVUSBROOTHUB pRh = (PVUSBROOTHUB)pHub;

    /*
     * Assign a port.
     */
    int iPort = ASMBitFirstSet(&pRh->Bitmap, sizeof(pRh->Bitmap) * 8);
    if (iPort < 0)
    {
        LogRel(("VUSB: No ports available!\n"));
        return VERR_VUSB_NO_PORTS;
    }
    ASMBitClear(&pRh->Bitmap, iPort);
    pHub->cDevices++;
    pDev->i16Port = iPort;

    /*
     * Call the HCI attach routine and let it have its say before the device is
     * linked into the device list of this hub.
     */
    int rc = pRh->pIRhPort->pfnAttach(pRh->pIRhPort, &pDev->IDevice, iPort);
    if (RT_SUCCESS(rc))
    {
        RTCritSectEnter(&pRh->CritSectDevices);
        pDev->pNext = pRh->pDevices;
        pRh->pDevices = pDev;
        RTCritSectLeave(&pRh->CritSectDevices);
        LogRel(("VUSB: Attached '%s' to port %d on %s (%sSpeed)\n", pDev->pUsbIns->pszName,
                iPort, pHub->pszName, vusbGetSpeedString(pDev->pUsbIns->enmSpeed)));
    }
    else
    {
        ASMBitSet(&pRh->Bitmap, iPort);
        pHub->cDevices--;
        pDev->i16Port = -1;
        LogRel(("VUSB: Failed to attach '%s' to port %d, rc=%Rrc\n", pDev->pUsbIns->pszName, iPort, rc));
    }
    return rc;
}


/**
 * Detach the device from the hub.
 *
 * @returns VBox status code.
 * @param   pHub        Pointer to the hub.
 * @param   pDev        Pointer to the device.
 */
static void vusbRhHubOpDetach(PVUSBHUB pHub, PVUSBDEV pDev)
{
    PVUSBROOTHUB pRh = (PVUSBROOTHUB)pHub;
    Assert(pDev->i16Port != -1);

    /*
     * Check that it's attached and unlink it from the linked list.
     */
    RTCritSectEnter(&pRh->CritSectDevices);
    if (pRh->pDevices != pDev)
    {
        PVUSBDEV pPrev = pRh->pDevices;
        while (pPrev && pPrev->pNext != pDev)
            pPrev = pPrev->pNext;
        Assert(pPrev);
        pPrev->pNext = pDev->pNext;
    }
    else
        pRh->pDevices = pDev->pNext;
    pDev->pNext = NULL;
    RTCritSectLeave(&pRh->CritSectDevices);

    /*
     * Detach the device and mark the port as available.
     */
    unsigned uPort = pDev->i16Port;
    pRh->pIRhPort->pfnDetach(pRh->pIRhPort, &pDev->IDevice, uPort);
    LogRel(("VUSB: Detached '%s' from port %u on %s\n", pDev->pUsbIns->pszName, uPort, pHub->pszName));
    ASMBitSet(&pRh->Bitmap, uPort);
    pHub->cDevices--;
}


/**
 * The Hub methods implemented by the root hub.
 */
static const VUSBHUBOPS s_VUsbRhHubOps =
{
    vusbRhHubOpAttach,
    vusbRhHubOpDetach
};



/* -=-=-=-=-=- PDM Base interface methods -=-=-=-=-=- */


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) vusbRhQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS      pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PVUSBROOTHUB    pRh     = PDMINS_2_DATA(pDrvIns, PVUSBROOTHUB);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, VUSBIROOTHUBCONNECTOR, &pRh->IRhConnector);
    PDMIBASE_RETURN_INTERFACE(pszIID, VUSBIDEVICE, &pRh->Hub.Dev.IDevice);
    return NULL;
}


/* -=-=-=-=-=- PDM Driver methods -=-=-=-=-=- */


/**
 * Destruct a driver instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that any non-VM
 * resources can be freed correctly.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) vusbRhDestruct(PPDMDRVINS pDrvIns)
{
    PVUSBROOTHUB pRh = PDMINS_2_DATA(pDrvIns, PVUSBROOTHUB);
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);

    vusbUrbPoolDestroy(&pRh->Hub.Dev.UrbPool);
    if (pRh->Hub.pszName)
    {
        RTStrFree(pRh->Hub.pszName);
        pRh->Hub.pszName = NULL;
    }
    if (pRh->hSniffer != VUSBSNIFFER_NIL)
        VUSBSnifferDestroy(pRh->hSniffer);

    if (pRh->hSemEventPeriodFrame)
        RTSemEventMultiDestroy(pRh->hSemEventPeriodFrame);

    if (pRh->hSemEventPeriodFrameStopped)
        RTSemEventMultiDestroy(pRh->hSemEventPeriodFrameStopped);

    RTCritSectDelete(&pRh->CritSectDevices);
}


/**
 * Construct a root hub driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) vusbRhConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    LogFlow(("vusbRhConstruct: Instance %d\n", pDrvIns->iInstance));
    PVUSBROOTHUB pThis = PDMINS_2_DATA(pDrvIns, PVUSBROOTHUB);

    /*
     * Validate configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, "CaptureFilename\0"))
        return VERR_PDM_DRVINS_UNKNOWN_CFG_VALUES;

    /*
     * Check that there are no drivers below us.
     */
    AssertMsgReturn(PDMDrvHlpNoAttach(pDrvIns) == VERR_PDM_NO_ATTACHED_DRIVER,
                    ("Configuration error: Not possible to attach anything to this driver!\n"),
                    VERR_PDM_DRVINS_NO_ATTACH);

    /*
     * Initialize the critical sections.
     */
    int rc = RTCritSectInit(&pThis->CritSectDevices);
    if (RT_FAILURE(rc))
        return rc;

    char *pszCaptureFilename = NULL;
    rc = CFGMR3QueryStringAlloc(pCfg, "CaptureFilename", &pszCaptureFilename);
    if (   RT_FAILURE(rc)
        && rc != VERR_CFGM_VALUE_NOT_FOUND)
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("Configuration error: Failed to query value of \"CaptureFilename\""));

    /*
     * Initialize the data members.
     */
    pDrvIns->IBase.pfnQueryInterface    = vusbRhQueryInterface;
    /* the usb device */
    pThis->Hub.Dev.enmState             = VUSB_DEVICE_STATE_ATTACHED;
    pThis->Hub.Dev.u8Address            = VUSB_INVALID_ADDRESS;
    pThis->Hub.Dev.u8NewAddress         = VUSB_INVALID_ADDRESS;
    pThis->Hub.Dev.i16Port              = -1;
    pThis->Hub.Dev.cRefs                = 1;
    pThis->Hub.Dev.IDevice.pfnReset     = vusbRhDevReset;
    pThis->Hub.Dev.IDevice.pfnPowerOn   = vusbRhDevPowerOn;
    pThis->Hub.Dev.IDevice.pfnPowerOff  = vusbRhDevPowerOff;
    pThis->Hub.Dev.IDevice.pfnGetState  = vusbRhDevGetState;
    /* the hub */
    pThis->Hub.pOps                     = &s_VUsbRhHubOps;
    pThis->Hub.pRootHub                 = pThis;
    //pThis->hub.cPorts                - later
    pThis->Hub.cDevices                 = 0;
    pThis->Hub.Dev.pHub                 = &pThis->Hub;
    RTStrAPrintf(&pThis->Hub.pszName, "RootHub#%d", pDrvIns->iInstance);
    /* misc */
    pThis->pDrvIns                      = pDrvIns;
    /* the connector */
    pThis->IRhConnector.pfnSetUrbParams               = vusbRhSetUrbParams;
    pThis->IRhConnector.pfnNewUrb                     = vusbRhConnNewUrb;
    pThis->IRhConnector.pfnFreeUrb                    = vusbRhConnFreeUrb;
    pThis->IRhConnector.pfnSubmitUrb                  = vusbRhSubmitUrb;
    pThis->IRhConnector.pfnReapAsyncUrbs              = vusbRhReapAsyncUrbs;
    pThis->IRhConnector.pfnCancelUrbsEp               = vusbRhCancelUrbsEp;
    pThis->IRhConnector.pfnCancelAllUrbs              = vusbRhCancelAllUrbs;
    pThis->IRhConnector.pfnAbortEp                    = vusbRhAbortEp;
    pThis->IRhConnector.pfnAttachDevice               = vusbRhAttachDevice;
    pThis->IRhConnector.pfnDetachDevice               = vusbRhDetachDevice;
    pThis->IRhConnector.pfnSetPeriodicFrameProcessing = vusbRhSetFrameProcessing;
    pThis->IRhConnector.pfnGetPeriodicFrameRate       = vusbRhGetPeriodicFrameRate;
    pThis->IRhConnector.pfnUpdateIsocFrameDelta       = vusbRhUpdateIsocFrameDelta;
    pThis->hSniffer                                   = VUSBSNIFFER_NIL;
    pThis->cbHci                                      = 0;
    pThis->cbHciTd                                    = 0;
    pThis->fFrameProcessing                           = false;
#ifdef LOG_ENABLED
    pThis->iSerial                                    = 0;
#endif
    /*
     * Resolve interface(s).
     */
    pThis->pIRhPort = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, VUSBIROOTHUBPORT);
    AssertMsgReturn(pThis->pIRhPort, ("Configuration error: the device/driver above us doesn't expose any VUSBIROOTHUBPORT interface!\n"), VERR_PDM_MISSING_INTERFACE_ABOVE);

    /*
     * Get number of ports and the availability bitmap.
     * ASSUME that the number of ports reported now at creation time is the max number.
     */
    pThis->Hub.cPorts = pThis->pIRhPort->pfnGetAvailablePorts(pThis->pIRhPort, &pThis->Bitmap);
    Log(("vusbRhConstruct: cPorts=%d\n", pThis->Hub.cPorts));

    /*
     * Get the USB version of the attached HC.
     * ASSUME that version 2.0 implies high-speed.
     */
    pThis->fHcVersions = pThis->pIRhPort->pfnGetUSBVersions(pThis->pIRhPort);
    Log(("vusbRhConstruct: fHcVersions=%u\n", pThis->fHcVersions));

    rc = vusbUrbPoolInit(&pThis->Hub.Dev.UrbPool);
    if (RT_FAILURE(rc))
        return rc;

    if (pszCaptureFilename)
    {
        rc = VUSBSnifferCreate(&pThis->hSniffer, 0, pszCaptureFilename, NULL, NULL);
        if (RT_FAILURE(rc))
            return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                       N_("VUSBSniffer cannot open '%s' for writing. The directory must exist and it must be writable for the current user"),
                                       pszCaptureFilename);

        MMR3HeapFree(pszCaptureFilename);
    }

    /*
     * Register ourselves as a USB hub.
     * The current implementation uses the VUSBIRHCONFIG interface for communication.
     */
    PCPDMUSBHUBHLP pHlp; /* not used currently */
    rc = PDMDrvHlpUSBRegisterHub(pDrvIns, pThis->fHcVersions, pThis->Hub.cPorts, &g_vusbHubReg, &pHlp);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Statistics. (It requires a 30" monitor or extremely tiny fonts to edit this "table".)
     */
#ifdef VBOX_WITH_STATISTICS
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->Total.StatUrbsSubmitted,                     STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "The number of URBs submitted.",                  "/VUSB/%d/UrbsSubmitted",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatUrbsSubmitted, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Bulk transfer.",                                 "/VUSB/%d/UrbsSubmitted/Bulk",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatUrbsSubmitted, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Control transfer.",                              "/VUSB/%d/UrbsSubmitted/Ctrl",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatUrbsSubmitted, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Interrupt transfer.",                            "/VUSB/%d/UrbsSubmitted/Intr",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatUrbsSubmitted, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Isochronous transfer.",                          "/VUSB/%d/UrbsSubmitted/Isoc",          pDrvIns->iInstance);

    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->Total.StatUrbsCancelled,                     STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "The number of URBs cancelled. (included in failed)", "/VUSB/%d/UrbsCancelled",           pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatUrbsCancelled, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Bulk transfer.",                                 "/VUSB/%d/UrbsCancelled/Bulk",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatUrbsCancelled, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Control transfer.",                              "/VUSB/%d/UrbsCancelled/Ctrl",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatUrbsCancelled, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Interrupt transfer.",                            "/VUSB/%d/UrbsCancelled/Intr",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatUrbsCancelled, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Isochronous transfer.",                          "/VUSB/%d/UrbsCancelled/Isoc",          pDrvIns->iInstance);

    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->Total.StatUrbsFailed,                        STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "The number of URBs failing.",                    "/VUSB/%d/UrbsFailed",                  pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatUrbsFailed,    STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Bulk transfer.",                                 "/VUSB/%d/UrbsFailed/Bulk",             pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatUrbsFailed,    STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Control transfer.",                              "/VUSB/%d/UrbsFailed/Ctrl",             pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatUrbsFailed,    STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Interrupt transfer.",                            "/VUSB/%d/UrbsFailed/Intr",             pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatUrbsFailed,    STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Isochronous transfer.",                          "/VUSB/%d/UrbsFailed/Isoc",             pDrvIns->iInstance);

    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->Total.StatReqBytes,                          STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Total requested transfer.",                      "/VUSB/%d/ReqBytes",                    pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatReqBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Bulk transfer.",                                 "/VUSB/%d/ReqBytes/Bulk",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatReqBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Control transfer.",                              "/VUSB/%d/ReqBytes/Ctrl",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatReqBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Interrupt transfer.",                            "/VUSB/%d/ReqBytes/Intr",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatReqBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Isochronous transfer.",                          "/VUSB/%d/ReqBytes/Isoc",               pDrvIns->iInstance);

    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->Total.StatReqReadBytes,                      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Total requested read transfer.",                 "/VUSB/%d/ReqReadBytes",                pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatReqReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Bulk transfer.",                                 "/VUSB/%d/ReqReadBytes/Bulk",           pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatReqReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Control transfer.",                              "/VUSB/%d/ReqReadBytes/Ctrl",           pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatReqReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Interrupt transfer.",                            "/VUSB/%d/ReqReadBytes/Intr",           pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatReqReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Isochronous transfer.",                          "/VUSB/%d/ReqReadBytes/Isoc",           pDrvIns->iInstance);

    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->Total.StatReqWriteBytes,                     STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Total requested write transfer.",                "/VUSB/%d/ReqWriteBytes",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatReqWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Bulk transfer.",                                 "/VUSB/%d/ReqWriteBytes/Bulk",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatReqWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Control transfer.",                              "/VUSB/%d/ReqWriteBytes/Ctrl",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatReqWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Interrupt transfer.",                            "/VUSB/%d/ReqWriteBytes/Intr",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatReqWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Isochronous transfer.",                          "/VUSB/%d/ReqWriteBytes/Isoc",          pDrvIns->iInstance);

    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->Total.StatActBytes,                          STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Actual total transfer.",                         "/VUSB/%d/ActBytes",                    pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatActBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Bulk transfer.",                                 "/VUSB/%d/ActBytes/Bulk",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatActBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Control transfer.",                              "/VUSB/%d/ActBytes/Ctrl",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatActBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Interrupt transfer.",                            "/VUSB/%d/ActBytes/Intr",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatActBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Isochronous transfer.",                          "/VUSB/%d/ActBytes/Isoc",               pDrvIns->iInstance);

    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->Total.StatActReadBytes,                      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Actual total read transfer.",                    "/VUSB/%d/ActReadBytes",                pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatActReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Bulk transfer.",                                 "/VUSB/%d/ActReadBytes/Bulk",           pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatActReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Control transfer.",                              "/VUSB/%d/ActReadBytes/Ctrl",           pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatActReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Interrupt transfer.",                            "/VUSB/%d/ActReadBytes/Intr",           pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatActReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Isochronous transfer.",                          "/VUSB/%d/ActReadBytes/Isoc",           pDrvIns->iInstance);

    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->Total.StatActWriteBytes,                     STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Actual total write transfer.",                   "/VUSB/%d/ActWriteBytes",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatActWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Bulk transfer.",                                 "/VUSB/%d/ActWriteBytes/Bulk",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatActWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Control transfer.",                              "/VUSB/%d/ActWriteBytes/Ctrl",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatActWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Interrupt transfer.",                            "/VUSB/%d/ActWriteBytes/Intr",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatActWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Isochronous transfer.",                          "/VUSB/%d/ActWriteBytes/Isoc",          pDrvIns->iInstance);

    /* bulk */
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatUrbsSubmitted, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of submitted URBs.",                      "/VUSB/%d/Bulk/Urbs",                   pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatUrbsFailed,    STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of failed URBs.",                         "/VUSB/%d/Bulk/UrbsFailed",             pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatUrbsCancelled, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of cancelled URBs.",                      "/VUSB/%d/Bulk/UrbsFailed/Cancelled",   pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatActBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Number of bytes transferred.",                    "/VUSB/%d/Bulk/ActBytes",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatActReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Read.",                                          "/VUSB/%d/Bulk/ActBytes/Read",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatActWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Write.",                                         "/VUSB/%d/Bulk/ActBytes/Write",         pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatReqBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Requested number of bytes.",                     "/VUSB/%d/Bulk/ReqBytes",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatReqReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Read.",                                          "/VUSB/%d/Bulk/ReqBytes/Read",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_BULK].StatReqWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Write.",                                         "/VUSB/%d/Bulk/ReqBytes/Write",         pDrvIns->iInstance);

    /* control */
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatUrbsSubmitted, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of submitted URBs.",                      "/VUSB/%d/Ctrl/Urbs",                   pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatUrbsFailed,    STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of failed URBs.",                         "/VUSB/%d/Ctrl/UrbsFailed",             pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatUrbsCancelled, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of cancelled URBs.",                      "/VUSB/%d/Ctrl/UrbsFailed/Cancelled",   pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatActBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Number of bytes transferred.",                    "/VUSB/%d/Ctrl/ActBytes",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatActReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Read.",                                          "/VUSB/%d/Ctrl/ActBytes/Read",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatActWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Write.",                                         "/VUSB/%d/Ctrl/ActBytes/Write",         pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatReqBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Requested number of bytes.",                     "/VUSB/%d/Ctrl/ReqBytes",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatReqReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Read.",                                          "/VUSB/%d/Ctrl/ReqBytes/Read",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_CTRL].StatReqWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Write.",                                         "/VUSB/%d/Ctrl/ReqBytes/Write",         pDrvIns->iInstance);

    /* interrupt */
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatUrbsSubmitted, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of submitted URBs.",                      "/VUSB/%d/Intr/Urbs",                   pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatUrbsFailed,    STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of failed URBs.",                         "/VUSB/%d/Intr/UrbsFailed",             pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatUrbsCancelled, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of cancelled URBs.",                      "/VUSB/%d/Intr/UrbsFailed/Cancelled",   pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatActBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Number of bytes transferred.",                    "/VUSB/%d/Intr/ActBytes",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatActReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Read.",                                          "/VUSB/%d/Intr/ActBytes/Read",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatActWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Write.",                                         "/VUSB/%d/Intr/ActBytes/Write",         pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatReqBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Requested number of bytes.",                     "/VUSB/%d/Intr/ReqBytes",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatReqReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Read.",                                          "/VUSB/%d/Intr/ReqBytes/Read",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_INTR].StatReqWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Write.",                                         "/VUSB/%d/Intr/ReqBytes/Write",         pDrvIns->iInstance);

    /* isochronous */
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatUrbsSubmitted, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of submitted URBs.",                      "/VUSB/%d/Isoc/Urbs",                   pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatUrbsFailed,    STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of failed URBs.",                         "/VUSB/%d/Isoc/UrbsFailed",             pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatUrbsCancelled, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of cancelled URBs.",                      "/VUSB/%d/Isoc/UrbsFailed/Cancelled",   pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatActBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Number of bytes transferred.",                    "/VUSB/%d/Isoc/ActBytes",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatActReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Read.",                                          "/VUSB/%d/Isoc/ActBytes/Read",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatActWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Write.",                                         "/VUSB/%d/Isoc/ActBytes/Write",         pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatReqBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Requested number of bytes.",                     "/VUSB/%d/Isoc/ReqBytes",               pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatReqReadBytes,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Read.",                                          "/VUSB/%d/Isoc/ReqBytes/Read",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aTypes[VUSBXFERTYPE_ISOC].StatReqWriteBytes, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES, "Write.",                                         "/VUSB/%d/Isoc/ReqBytes/Write",         pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatIsocActPkts,                             STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Number of isochronous packets returning data.",  "/VUSB/%d/Isoc/ActPkts",                pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatIsocActReadPkts,                         STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Read.",                                          "/VUSB/%d/Isoc/ActPkts/Read",           pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatIsocActWritePkts,                        STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Write.",                                         "/VUSB/%d/Isoc/ActPkts/Write",          pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatIsocReqPkts,                             STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Requested number of isochronous packets.",       "/VUSB/%d/Isoc/ReqPkts",                pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatIsocReqReadPkts,                         STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Read.",                                          "/VUSB/%d/Isoc/ReqPkts/Read",           pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatIsocReqWritePkts,                        STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "Write.",                                         "/VUSB/%d/Isoc/ReqPkts/Write",          pDrvIns->iInstance);

    for (unsigned i = 0; i < RT_ELEMENTS(pThis->aStatIsocDetails); i++)
    {
        PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aStatIsocDetails[i].Pkts,                STAMTYPE_COUNTER, STAMVISIBILITY_USED,    STAMUNIT_COUNT, ".", "/VUSB/%d/Isoc/%d",                 pDrvIns->iInstance, i);
        PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aStatIsocDetails[i].Ok,                  STAMTYPE_COUNTER, STAMVISIBILITY_USED,    STAMUNIT_COUNT, ".", "/VUSB/%d/Isoc/%d/Ok",              pDrvIns->iInstance, i);
        PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aStatIsocDetails[i].Ok0,                 STAMTYPE_COUNTER, STAMVISIBILITY_USED,    STAMUNIT_COUNT, ".", "/VUSB/%d/Isoc/%d/Ok0",             pDrvIns->iInstance, i);
        PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aStatIsocDetails[i].DataUnderrun,        STAMTYPE_COUNTER, STAMVISIBILITY_USED,    STAMUNIT_COUNT, ".", "/VUSB/%d/Isoc/%d/DataUnderrun",    pDrvIns->iInstance, i);
        PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aStatIsocDetails[i].DataUnderrun0,       STAMTYPE_COUNTER, STAMVISIBILITY_USED,    STAMUNIT_COUNT, ".", "/VUSB/%d/Isoc/%d/DataUnderrun0",   pDrvIns->iInstance, i);
        PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aStatIsocDetails[i].DataOverrun,         STAMTYPE_COUNTER, STAMVISIBILITY_USED,    STAMUNIT_COUNT, ".", "/VUSB/%d/Isoc/%d/DataOverrun",     pDrvIns->iInstance, i);
        PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aStatIsocDetails[i].NotAccessed,         STAMTYPE_COUNTER, STAMVISIBILITY_USED,    STAMUNIT_COUNT, ".", "/VUSB/%d/Isoc/%d/NotAccessed",     pDrvIns->iInstance, i);
        PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aStatIsocDetails[i].Misc,                STAMTYPE_COUNTER, STAMVISIBILITY_USED,    STAMUNIT_COUNT, ".", "/VUSB/%d/Isoc/%d/Misc",            pDrvIns->iInstance, i);
        PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->aStatIsocDetails[i].Bytes,               STAMTYPE_COUNTER, STAMVISIBILITY_USED,    STAMUNIT_BYTES, ".", "/VUSB/%d/Isoc/%d/Bytes",           pDrvIns->iInstance, i);
    }

    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatReapAsyncUrbs, STAMTYPE_PROFILE, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Profiling the vusbRhReapAsyncUrbs body (omitting calls when nothing is in-flight).",  "/VUSB/%d/ReapAsyncUrbs", pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatSubmitUrb,     STAMTYPE_PROFILE, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Profiling the vusbRhSubmitUrb body.",                                 "/VUSB/%d/SubmitUrb",                 pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatFramesProcessedThread, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Processed frames in the dedicated thread", "/VUSB/%d/FramesProcessedThread",       pDrvIns->iInstance);
    PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatFramesProcessedClbk,   STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES, "Processed frames in the URB completion callback", "/VUSB/%d/FramesProcessedClbk",  pDrvIns->iInstance);
#endif
    PDMDrvHlpSTAMRegisterF(pDrvIns, (void *)&pThis->Hub.Dev.UrbPool.cUrbsInPool, STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT, "The number of URBs in the pool.",                                 "/VUSB/%d/cUrbsInPool",               pDrvIns->iInstance);

    return VINF_SUCCESS;
}


/**
 * VUSB Root Hub driver registration record.
 */
const PDMDRVREG g_DrvVUSBRootHub =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "VUSBRootHub",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "VUSB Root Hub Driver.",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_USB,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(VUSBROOTHUB),
    /* pfnConstruct */
    vusbRhConstruct,
    /* pfnDestruct */
    vusbRhDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};

/*
 * Local Variables:
 *  mode: c
 *  c-file-style: "bsd"
 *  c-basic-offset: 4
 *  tab-width: 4
 *  indent-tabs-mode: s
 * End:
 */

