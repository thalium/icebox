/* $Id: VBoxUSB.cpp $ */
/** @file
 * VirtualBox USB driver for Darwin.
 *
 * This driver is responsible for hijacking USB devices when any of the
 * VBoxSVC daemons requests it. It is also responsible for arbitrating
 * access to hijacked USB devices.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP  LOG_GROUP_USB_DRV
/* Deal with conflicts first.
 * (This is mess inherited from BSD. The *BSDs has clean this up long ago.) */
#include <sys/param.h>
#undef PVM
#include <IOKit/IOLib.h> /* Assert as function */

#include "VBoxUSBInterface.h"
#include "VBoxUSBFilterMgr.h"
#include <VBox/version.h>
#include <VBox/usblib-darwin.h>
#include <VBox/log.h>
#include <iprt/types.h>
#include <iprt/initterm.h>
#include <iprt/assert.h>
#include <iprt/semaphore.h>
#include <iprt/process.h>
#include <iprt/alloc.h>
#include <iprt/err.h>
#include <iprt/asm.h>

#include <mach/kmod.h>
#include <miscfs/devfs/devfs.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/ioccom.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <kern/task.h>
#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/usb/USB.h>
#include <IOKit/usb/IOUSBDevice.h>
#include <IOKit/usb/IOUSBInterface.h>
#include <IOKit/usb/IOUSBUserClient.h>

/* private: */
RT_C_DECLS_BEGIN
extern void     *get_bsdtask_info(task_t);
RT_C_DECLS_END


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Locks the lists. */
#define VBOXUSB_LOCK()      do { int rc = RTSemFastMutexRequest(g_Mtx); AssertRC(rc); } while (0)
/** Unlocks the lists. */
#define VBOXUSB_UNLOCK()    do { int rc = RTSemFastMutexRelease(g_Mtx); AssertRC(rc); } while (0)


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
static kern_return_t    VBoxUSBStart(struct kmod_info *pKModInfo, void *pvData);
static kern_return_t    VBoxUSBStop(struct kmod_info *pKModInfo, void *pvData);
RT_C_DECLS_END


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * The service class.
 *
 * This is the management service that VBoxSVC and the VMs speak to.
 *
 * @remark  The method prototypes are ordered somewhat after their order of
 *          invocation, while the implementation is ordered by pair.
 */
class org_virtualbox_VBoxUSB : public IOService
{
    OSDeclareDefaultStructors(org_virtualbox_VBoxUSB);

public:
    RTR0MEMEF_NEW_AND_DELETE_OPERATORS_IOKIT();

    /** @name IOService
     * @{ */
    virtual bool init(OSDictionary *pDictionary = 0);
    virtual bool start(IOService *pProvider);
    virtual bool open(IOService *pForClient, IOOptionBits fOptions = 0, void *pvArg = 0);
    virtual bool terminate(IOOptionBits fOptions);
    virtual void close(IOService *pForClient, IOOptionBits fOptions = 0);
    virtual void stop(IOService *pProvider);
    virtual void free();
    /** @} */

private:
    /** Guard against the parent class growing and us using outdated headers. */
    uint8_t m_abSafetyPadding[256];
};
OSDefineMetaClassAndStructors(org_virtualbox_VBoxUSB, IOService);


/**
 * The user client class that pairs up with org_virtualbox_VBoxUSB.
 */
class org_virtualbox_VBoxUSBClient : public IOUserClient
{
    OSDeclareDefaultStructors(org_virtualbox_VBoxUSBClient);

public:
    RTR0MEMEF_NEW_AND_DELETE_OPERATORS_IOKIT();

    /** @name IOService & IOUserClient
     * @{ */
    virtual bool initWithTask(task_t OwningTask, void *pvSecurityId, UInt32 u32Type);
    virtual bool start(IOService *pProvider);
    virtual IOReturn clientClose(void);
    virtual IOReturn clientDied(void);
    virtual bool terminate(IOOptionBits fOptions = 0);
    virtual bool finalize(IOOptionBits fOptions);
    virtual void stop(IOService *pProvider);
    virtual void free();
    virtual IOExternalMethod *getTargetAndMethodForIndex(IOService **ppService, UInt32 iMethod);
    /** @} */

    /** @name User client methods
     * @{ */
    IOReturn addFilter(PUSBFILTER pFilter, PVBOXUSBADDFILTEROUT pOut, IOByteCount cbFilter, IOByteCount *pcbOut);
    IOReturn removeFilter(uintptr_t *puId, int *prc, IOByteCount cbIn, IOByteCount *pcbOut);
    /** @} */

    static bool isClientTask(task_t ClientTask);

private:
    /** Guard against the parent class growing and us using outdated headers. */
    uint8_t m_abSafetyPadding[256];
    /** The service provider. */
    org_virtualbox_VBoxUSB *m_pProvider;
    /** The client task. */
    task_t m_Task;
    /** The client process. */
    RTPROCESS m_Process;
    /** Pointer to the next user client. */
    org_virtualbox_VBoxUSBClient * volatile m_pNext;
    /** List of user clients. Protected by g_Mtx. */
    static org_virtualbox_VBoxUSBClient * volatile s_pHead;
};
OSDefineMetaClassAndStructors(org_virtualbox_VBoxUSBClient, IOUserClient);


/**
 * The IOUSBDevice driver class.
 *
 * The main purpose of this is hijack devices matching current filters.
 *
 * @remarks This is derived from IOUSBUserClientInit instead of IOService because we must make
 *          sure IOUSBUserClientInit::start() gets invoked for this provider. The problem is that
 *          there is some kind of magic that prevents this from happening if we boost the probe
 *          score to high. With the result that we don't have the required plugin entry for
 *          user land and consequently cannot open it.
 *
 *          So, to avoid having to write a lot of code we just inherit from IOUSBUserClientInit
 *          and make some possibly bold assumptions about it not changing. This just means
 *          we'll have to keep an eye on the source apple releases or only call
 *          IOUSBUserClientInit::start() and hand the rest of the super calls to IOService. For
 *          now we're doing it by the C++ book.
 */
class org_virtualbox_VBoxUSBDevice : public IOUSBUserClientInit
{
    OSDeclareDefaultStructors(org_virtualbox_VBoxUSBDevice);

public:
    RTR0MEMEF_NEW_AND_DELETE_OPERATORS_IOKIT();

    /** @name IOService
     * @{ */
    virtual bool init(OSDictionary *pDictionary = 0);
    virtual IOService *probe(IOService *pProvider, SInt32 *pi32Score);
    virtual bool start(IOService *pProvider);
    virtual bool terminate(IOOptionBits fOptions = 0);
    virtual void stop(IOService *pProvider);
    virtual void free();
    virtual IOReturn message(UInt32 enmMsg, IOService *pProvider, void *pvArg = 0);
    /** @} */

    static void  scheduleReleaseByOwner(RTPROCESS Owner);
private:
    /** Padding to guard against parent class expanding (see class remarks). */
    uint8_t m_abSafetyPadding[256];
    /** The interface we're driving (aka. the provider). */
    IOUSBDevice *m_pDevice;
    /** The owner process, meaning the VBoxSVC process. */
    RTPROCESS volatile m_Owner;
    /** The client process, meaning the VM process. */
    RTPROCESS volatile m_Client;
    /** The ID of the matching filter. */
    uintptr_t m_uId;
    /** Have we opened the device or not? */
    bool volatile m_fOpen;
    /** Should be open the device on the next close notification message? */
    bool volatile m_fOpenOnWasClosed;
    /** Whether to re-enumerate this device when the client closes it.
     * This is something we'll do when the filter owner dies. */
    bool volatile m_fReleaseOnClose;
    /** Whether we're being unloaded or not.
     * Only valid in stop(). */
    bool m_fBeingUnloaded;
    /** Pointer to the next device in the list. */
    org_virtualbox_VBoxUSBDevice * volatile m_pNext;
    /** Pointer to the list head. Protected by g_Mtx. */
    static org_virtualbox_VBoxUSBDevice * volatile s_pHead;

#ifdef DEBUG
    /** The interest notifier. */
    IONotifier *m_pNotifier;

    static IOReturn MyInterestHandler(void *pvTarget, void *pvRefCon, UInt32 enmMsgType,
                                      IOService *pProvider, void * pvMsgArg, vm_size_t cbMsgArg);
#endif
};
OSDefineMetaClassAndStructors(org_virtualbox_VBoxUSBDevice, IOUSBUserClientInit);


/**
 * The IOUSBInterface driver class.
 *
 * The main purpose of this is hijack interfaces which device is driven
 * by org_virtualbox_VBoxUSBDevice.
 *
 * @remarks See org_virtualbox_VBoxUSBDevice for why we use IOUSBUserClientInit.
 */
class org_virtualbox_VBoxUSBInterface : public IOUSBUserClientInit
{
    OSDeclareDefaultStructors(org_virtualbox_VBoxUSBInterface);

public:
    RTR0MEMEF_NEW_AND_DELETE_OPERATORS_IOKIT();

    /** @name IOService
     * @{ */
    virtual bool init(OSDictionary *pDictionary = 0);
    virtual IOService *probe(IOService *pProvider, SInt32 *pi32Score);
    virtual bool start(IOService *pProvider);
    virtual bool terminate(IOOptionBits fOptions = 0);
    virtual void stop(IOService *pProvider);
    virtual void free();
    virtual IOReturn message(UInt32 enmMsg, IOService *pProvider, void *pvArg = 0);
    /** @} */

private:
    /** Padding to guard against parent class expanding (see class remarks). */
    uint8_t m_abSafetyPadding[256];
    /** The interface we're driving (aka. the provider). */
    IOUSBInterface *m_pInterface;
    /** Have we opened the device or not? */
    bool volatile m_fOpen;
    /** Should be open the device on the next close notification message? */
    bool volatile m_fOpenOnWasClosed;
};
OSDefineMetaClassAndStructors(org_virtualbox_VBoxUSBInterface, IOUSBUserClientInit);





/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/*
 * Declare the module stuff.
 */
RT_C_DECLS_BEGIN
extern kern_return_t _start(struct kmod_info *pKModInfo, void *pvData);
extern kern_return_t _stop(struct kmod_info *pKModInfo, void *pvData);

KMOD_EXPLICIT_DECL(VBoxDrv, VBOX_VERSION_STRING, _start, _stop)
DECLHIDDEN(kmod_start_func_t *) _realmain = VBoxUSBStart;
DECLHIDDEN(kmod_stop_func_t  *) _antimain = VBoxUSBStop;
DECLHIDDEN(int)                 _kext_apple_cc = __APPLE_CC__;
RT_C_DECLS_END

/** Mutex protecting the lists. */
static RTSEMFASTMUTEX g_Mtx = NIL_RTSEMFASTMUTEX;
org_virtualbox_VBoxUSBClient * volatile org_virtualbox_VBoxUSBClient::s_pHead = NULL;
org_virtualbox_VBoxUSBDevice * volatile org_virtualbox_VBoxUSBDevice::s_pHead = NULL;

/** Global instance count - just for checking proving that everything is destroyed correctly. */
static volatile uint32_t g_cInstances = 0;


/**
 * Start the kernel module.
 */
static kern_return_t VBoxUSBStart(struct kmod_info *pKModInfo, void *pvData)
{
    RT_NOREF(pKModInfo, pvData);
    int rc;
    Log(("VBoxUSBStart\n"));

    /*
     * Initialize IPRT.
     */
    rc = RTR0Init(0);
    if (RT_SUCCESS(rc))
    {
        /*
         * Create the spinlock.
         */
        rc = RTSemFastMutexCreate(&g_Mtx);
        if (RT_SUCCESS(rc))
        {
            rc = VBoxUSBFilterInit();
            if (RT_SUCCESS(rc))
            {
#if 0 /* testing */
                USBFILTER Flt;
                USBFilterInit(&Flt, USBFILTERTYPE_CAPTURE);
                USBFilterSetNumExact(&Flt, USBFILTERIDX_VENDOR_ID, 0x096e, true);
                uintptr_t uId;
                rc = VBoxUSBFilterAdd(&Flt, 1, &uId);
                printf("VBoxUSB: VBoxUSBFilterAdd #1 -> %d + %p\n", rc, uId);

                USBFilterInit(&Flt, USBFILTERTYPE_CAPTURE);
                USBFilterSetStringPattern(&Flt, USBFILTERIDX_PRODUCT_STR, "*DISK*", true);
                rc = VBoxUSBFilterAdd(&Flt, 2, &uId);
                printf("VBoxUSB: VBoxUSBFilterAdd #2 -> %d + %p\n", rc, uId);
#endif
                return KMOD_RETURN_SUCCESS;
            }
            printf("VBoxUSB: VBoxUSBFilterInit failed (rc=%d)\n", rc);
            RTSemFastMutexDestroy(g_Mtx);
            g_Mtx = NIL_RTSEMFASTMUTEX;
        }
        else
            printf("VBoxUSB: RTSemFastMutexCreate failed (rc=%d)\n", rc);
        RTR0Term();
    }
    else
        printf("VBoxUSB: failed to initialize IPRT (rc=%d)\n", rc);

    return KMOD_RETURN_FAILURE;
}


/**
 * Stop the kernel module.
 */
static kern_return_t VBoxUSBStop(struct kmod_info *pKModInfo, void *pvData)
{
    RT_NOREF(pKModInfo, pvData);
    Log(("VBoxUSBStop: g_cInstances=%d\n", g_cInstances));

    /** @todo Fix problem with crashing when unloading a driver that's in use. */

    /*
     * Undo the work done during start (in reverse order).
     */
    VBoxUSBFilterTerm();

    int rc = RTSemFastMutexDestroy(g_Mtx);
    AssertRC(rc);
    g_Mtx = NIL_RTSEMFASTMUTEX;

    RTR0Term();

    Log(("VBoxUSBStop - done\n"));
    return KMOD_RETURN_SUCCESS;
}





#ifdef LOG_ENABLED
/**
 * Gets the name of a IOKit message.
 *
 * @returns Message name (read only).
 * @param   enmMsg      The message.
 */
DECLINLINE(const char *) DbgGetIOKitMessageName(UInt32 enmMsg)
{
    switch (enmMsg)
    {
# define MY_CASE(enm) case enm: return #enm; break
        MY_CASE(kIOMessageServiceIsTerminated);
        MY_CASE(kIOMessageServiceIsSuspended);
        MY_CASE(kIOMessageServiceIsResumed);
        MY_CASE(kIOMessageServiceIsRequestingClose);
        MY_CASE(kIOMessageServiceIsAttemptingOpen);
        MY_CASE(kIOMessageServiceWasClosed);
        MY_CASE(kIOMessageServiceBusyStateChange);
        MY_CASE(kIOMessageServicePropertyChange);
        MY_CASE(kIOMessageCanDevicePowerOff);
        MY_CASE(kIOMessageDeviceWillPowerOff);
        MY_CASE(kIOMessageDeviceWillNotPowerOff);
        MY_CASE(kIOMessageDeviceHasPoweredOn);
        MY_CASE(kIOMessageCanSystemPowerOff);
        MY_CASE(kIOMessageSystemWillPowerOff);
        MY_CASE(kIOMessageSystemWillNotPowerOff);
        MY_CASE(kIOMessageCanSystemSleep);
        MY_CASE(kIOMessageSystemWillSleep);
        MY_CASE(kIOMessageSystemWillNotSleep);
        MY_CASE(kIOMessageSystemHasPoweredOn);
        MY_CASE(kIOMessageSystemWillRestart);
        MY_CASE(kIOMessageSystemWillPowerOn);
        MY_CASE(kIOUSBMessageHubResetPort);
        MY_CASE(kIOUSBMessageHubSuspendPort);
        MY_CASE(kIOUSBMessageHubResumePort);
        MY_CASE(kIOUSBMessageHubIsDeviceConnected);
        MY_CASE(kIOUSBMessageHubIsPortEnabled);
        MY_CASE(kIOUSBMessageHubReEnumeratePort);
        MY_CASE(kIOUSBMessagePortHasBeenReset);
        MY_CASE(kIOUSBMessagePortHasBeenResumed);
        MY_CASE(kIOUSBMessageHubPortClearTT);
        MY_CASE(kIOUSBMessagePortHasBeenSuspended);
        MY_CASE(kIOUSBMessageFromThirdParty);
        MY_CASE(kIOUSBMessagePortWasNotSuspended);
        MY_CASE(kIOUSBMessageExpressCardCantWake);
//        MY_CASE(kIOUSBMessageCompositeDriverReconfigured);
# undef MY_CASE
    }
    return "unknown";
}
#endif /* LOG_ENABLED */





/*
 *
 * org_virtualbox_VBoxUSB
 *
 */


/**
 * Initialize the object.
 * @remark  Only for logging.
 */
bool
org_virtualbox_VBoxUSB::init(OSDictionary *pDictionary)
{
    uint32_t cInstances = ASMAtomicIncU32(&g_cInstances);
    Log(("VBoxUSB::init([%p], %p) new g_cInstances=%d\n", this, pDictionary, cInstances));
    RT_NOREF_PV(cInstances);

    if (IOService::init(pDictionary))
    {
        /* init members. */
        return true;
    }
    ASMAtomicDecU32(&g_cInstances);
    return false;
}


/**
 * Free the object.
 * @remark  Only for logging.
 */
void
org_virtualbox_VBoxUSB::free()
{
    uint32_t cInstances = ASMAtomicDecU32(&g_cInstances); NOREF(cInstances);
    Log(("VBoxUSB::free([%p]) new g_cInstances=%d\n", this, cInstances));
    IOService::free();
}


/**
 * Start this service.
 */
bool
org_virtualbox_VBoxUSB::start(IOService *pProvider)
{
    Log(("VBoxUSB::start([%p], %p {%s})\n", this, pProvider, pProvider->getName()));

    if (IOService::start(pProvider))
    {
        /* register the service. */
        registerService();
        return true;
    }
    return false;
}


/**
 * Stop this service.
 * @remark  Only for logging.
 */
void
org_virtualbox_VBoxUSB::stop(IOService *pProvider)
{
    Log(("VBoxUSB::stop([%p], %p (%s))\n", this, pProvider, pProvider->getName()));
    IOService::stop(pProvider);
}


/**
 * Stop this service.
 * @remark  Only for logging.
 */
bool
org_virtualbox_VBoxUSB::open(IOService *pForClient, IOOptionBits fOptions/* = 0*/, void *pvArg/* = 0*/)
{
    Log(("VBoxUSB::open([%p], %p, %#x, %p)\n", this, pForClient, fOptions, pvArg));
    bool fRc = IOService::open(pForClient, fOptions, pvArg);
    Log(("VBoxUSB::open([%p], %p, %#x, %p) -> %d\n", this, pForClient, fOptions, pvArg, fRc));
    return fRc;
}


/**
 * Stop this service.
 * @remark  Only for logging.
 */
void
org_virtualbox_VBoxUSB::close(IOService *pForClient, IOOptionBits fOptions/* = 0*/)
{
    Log(("VBoxUSB::close([%p], %p, %#x)\n", this, pForClient, fOptions));
    IOService::close(pForClient, fOptions);
}


/**
 * Terminate request.
 * @remark  Only for logging.
 */
bool
org_virtualbox_VBoxUSB::terminate(IOOptionBits fOptions)
{
    Log(("VBoxUSB::terminate([%p], %#x): g_cInstances=%d\n", this, fOptions, g_cInstances));
    bool fRc = IOService::terminate(fOptions);
    Log(("VBoxUSB::terminate([%p], %#x): returns %d\n", this, fOptions, fRc));
    return fRc;
}











/*
 *
 * org_virtualbox_VBoxUSBClient
 *
 */


/**
 * Initializer called when the client opens the service.
 */
bool
org_virtualbox_VBoxUSBClient::initWithTask(task_t OwningTask, void *pvSecurityId, UInt32 u32Type)
{
    if (!OwningTask)
    {
        Log(("VBoxUSBClient::initWithTask([%p], %p, %p, %#x) -> false (no task)\n", this, OwningTask, pvSecurityId, u32Type));
        return false;
    }
    if (u32Type != VBOXUSB_DARWIN_IOSERVICE_COOKIE)
    {
        Log(("VBoxUSBClient::initWithTask: Bade cookie %#x\n", u32Type));
        return false;
    }

    proc_t pProc = (proc_t)get_bsdtask_info(OwningTask); /* we need the pid */
    Log(("VBoxUSBClient::initWithTask([%p], %p(->%p:{.pid=%d}, %p, %#x)\n",
             this, OwningTask, pProc, pProc ? proc_pid(pProc) : -1, pvSecurityId, u32Type));

    if (IOUserClient::initWithTask(OwningTask, pvSecurityId , u32Type))
    {
        /*
         * In theory we have to call task_reference() to make sure that the task is
         * valid during the lifetime of this object. The pointer is only used to check
         * for the context this object is called in though and never dereferenced
         * or passed to anything which might, so we just skip this step.
         */
        m_pProvider = NULL;
        m_Task = OwningTask;
        m_Process = pProc ? proc_pid(pProc) : NIL_RTPROCESS;
        m_pNext = NULL;

        uint32_t cInstances = ASMAtomicIncU32(&g_cInstances);
        Log(("VBoxUSBClient::initWithTask([%p], %p(->%p:{.pid=%d}, %p, %#x) -> true; new g_cInstances=%d\n",
                 this, OwningTask, pProc, pProc ? proc_pid(pProc) : -1, pvSecurityId, u32Type, cInstances));
        RT_NOREF_PV(cInstances);
        return true;
    }

    Log(("VBoxUSBClient::initWithTask([%p], %p(->%p:{.pid=%d}, %p, %#x) -> false\n",
             this, OwningTask, pProc, pProc ? proc_pid(pProc) : -1, pvSecurityId, u32Type));
    return false;
}


/**
 * Free the object.
 * @remark  Only for logging.
 */
void
org_virtualbox_VBoxUSBClient::free()
{
    uint32_t cInstances = ASMAtomicDecU32(&g_cInstances); NOREF(cInstances);
    Log(("VBoxUSBClient::free([%p]) new g_cInstances=%d\n", this, cInstances));
    IOUserClient::free();
}


/**
 * Start the client service.
 */
bool
org_virtualbox_VBoxUSBClient::start(IOService *pProvider)
{
    Log(("VBoxUSBClient::start([%p], %p)\n", this, pProvider));
    if (IOUserClient::start(pProvider))
    {
        m_pProvider = OSDynamicCast(org_virtualbox_VBoxUSB, pProvider);
        if (m_pProvider)
        {
            /*
             * Add ourselves to the list of user clients.
             */
            VBOXUSB_LOCK();

            m_pNext = s_pHead;
            s_pHead = this;

            VBOXUSB_UNLOCK();

            return true;
        }
        Log(("VBoxUSBClient::start: %p isn't org_virtualbox_VBoxUSB\n", pProvider));
    }
    return false;
}


/**
 * Client exits normally.
 */
IOReturn
org_virtualbox_VBoxUSBClient::clientClose(void)
{
    Log(("VBoxUSBClient::clientClose([%p:{.m_Process=%d}])\n", this, (int)m_Process));

    /*
     * Remove this process from the client list.
     */
    VBOXUSB_LOCK();

    org_virtualbox_VBoxUSBClient *pPrev = NULL;
    for (org_virtualbox_VBoxUSBClient *pCur = s_pHead; pCur; pCur = pCur->m_pNext)
    {
        if (pCur == this)
        {
            if (pPrev)
                pPrev->m_pNext = m_pNext;
            else
                s_pHead = m_pNext;
            m_pNext = NULL;
            break;
        }
        pPrev = pCur;
    }

    VBOXUSB_UNLOCK();

    /*
     * Drop all filters owned by this client.
     */
    if (m_Process != NIL_RTPROCESS)
        VBoxUSBFilterRemoveOwner(m_Process);

    /*
     * Schedule all devices owned (filtered) by this process for
     * immediate release or release upon close.
     */
    if (m_Process != NIL_RTPROCESS)
        org_virtualbox_VBoxUSBDevice::scheduleReleaseByOwner(m_Process);

    /*
     * Initiate termination.
     */
    m_pProvider = NULL;
    terminate();

    return kIOReturnSuccess;
}


/**
 * The client exits abnormally / forgets to do cleanups.
 * @remark  Only for logging.
 */
IOReturn
org_virtualbox_VBoxUSBClient::clientDied(void)
{
    Log(("VBoxUSBClient::clientDied([%p]) m_Task=%p R0Process=%p Process=%d\n",
             this, m_Task, RTR0ProcHandleSelf(), RTProcSelf()));

    /* IOUserClient::clientDied() calls clientClose... */
    return IOUserClient::clientDied();
}


/**
 * Terminate the service (initiate the destruction).
 * @remark  Only for logging.
 */
bool
org_virtualbox_VBoxUSBClient::terminate(IOOptionBits fOptions)
{
    /* kIOServiceRecursing, kIOServiceRequired, kIOServiceTerminate, kIOServiceSynchronous - interesting option bits */
    Log(("VBoxUSBClient::terminate([%p], %#x)\n", this, fOptions));
    return IOUserClient::terminate(fOptions);
}


/**
 * The final stage of the client service destruction.
 * @remark  Only for logging.
 */
bool
org_virtualbox_VBoxUSBClient::finalize(IOOptionBits fOptions)
{
    Log(("VBoxUSBClient::finalize([%p], %#x)\n", this, fOptions));
    return IOUserClient::finalize(fOptions);
}


/**
 * Stop the client service.
 */
void
org_virtualbox_VBoxUSBClient::stop(IOService *pProvider)
{
    Log(("VBoxUSBClient::stop([%p])\n", this));
    IOUserClient::stop(pProvider);

    /*
     * Paranoia.
     */
    VBOXUSB_LOCK();

    org_virtualbox_VBoxUSBClient *pPrev = NULL;
    for (org_virtualbox_VBoxUSBClient *pCur = s_pHead; pCur; pCur = pCur->m_pNext)
    {
        if (pCur == this)
        {
            if (pPrev)
                pPrev->m_pNext = m_pNext;
            else
                s_pHead = m_pNext;
            m_pNext = NULL;
            break;
        }
        pPrev = pCur;
    }

    VBOXUSB_UNLOCK();
}


/**
 * Translate a user method index into a service object and an external method structure.
 *
 * @returns Pointer to external method structure descripting the method.
 *          NULL if the index isn't valid.
 * @param   ppService       Where to store the service object on success.
 * @param   iMethod         The method index.
 */
IOExternalMethod *
org_virtualbox_VBoxUSBClient::getTargetAndMethodForIndex(IOService **ppService, UInt32 iMethod)
{
    static IOExternalMethod s_aMethods[VBOXUSBMETHOD_END] =
    {
        /*[VBOXUSBMETHOD_ADD_FILTER] = */
        {
            (IOService *)0,                                         /* object */
            (IOMethod)&org_virtualbox_VBoxUSBClient::addFilter,     /* func */
            kIOUCStructIStructO,                                    /* flags - struct input (count0) and struct output (count1) */
            sizeof(USBFILTER),                                      /* count0 - size of the input struct. */
            sizeof(VBOXUSBADDFILTEROUT)                             /* count1 - size of the return struct. */
        },
        /* [VBOXUSBMETHOD_FILTER_REMOVE] = */
        {
            (IOService *)0,                                         /* object */
            (IOMethod)&org_virtualbox_VBoxUSBClient::removeFilter,  /* func */
            kIOUCStructIStructO,                                    /* flags - struct input (count0) and struct output (count1) */
            sizeof(uintptr_t),                                      /* count0 - size of the input (id) */
            sizeof(int)                                             /* count1 - size of the output (rc) */
        },
    };

    if (RT_UNLIKELY(iMethod >= RT_ELEMENTS(s_aMethods)))
        return NULL;

    *ppService = this;
    return &s_aMethods[iMethod];
}


/**
 * Add filter user request.
 *
 * @returns IOKit status code.
 * @param   pFilter     The filter to add.
 * @param   pOut        Pointer to the output structure.
 * @param   cbFilter    Size of the filter structure.
 * @param   pcbOut      In/Out - sizeof(*pOut).
 */
IOReturn
org_virtualbox_VBoxUSBClient::addFilter(PUSBFILTER pFilter, PVBOXUSBADDFILTEROUT pOut, IOByteCount cbFilter, IOByteCount *pcbOut)
{
    Log(("VBoxUSBClient::addFilter: [%p:{.m_Process=%d}] pFilter=%p pOut=%p\n", this, (int)m_Process, pFilter, pOut));

    /*
     * Validate input.
     */
    if (RT_UNLIKELY(    cbFilter != sizeof(*pFilter)
                    ||  *pcbOut != sizeof(*pOut)))
    {
        printf("VBoxUSBClient::addFilter: cbFilter=%#x expected %#x; *pcbOut=%#x expected %#x\n",
               (int)cbFilter, (int)sizeof(*pFilter), (int)*pcbOut, (int)sizeof(*pOut));
        return kIOReturnBadArgument;
    }

    /*
     * Log the filter details.
     */
#ifdef DEBUG
    Log2(("VBoxUSBClient::addFilter: idVendor=%#x idProduct=%#x bcdDevice=%#x bDeviceClass=%#x bDeviceSubClass=%#x bDeviceProtocol=%#x bBus=%#x bPort=%#x\n",
              USBFilterGetNum(pFilter, USBFILTERIDX_VENDOR_ID),
              USBFilterGetNum(pFilter, USBFILTERIDX_PRODUCT_ID),
              USBFilterGetNum(pFilter, USBFILTERIDX_DEVICE_REV),
              USBFilterGetNum(pFilter, USBFILTERIDX_DEVICE_CLASS),
              USBFilterGetNum(pFilter, USBFILTERIDX_DEVICE_SUB_CLASS),
              USBFilterGetNum(pFilter, USBFILTERIDX_DEVICE_PROTOCOL),
              USBFilterGetNum(pFilter, USBFILTERIDX_BUS),
              USBFilterGetNum(pFilter, USBFILTERIDX_PORT)));
    Log2(("VBoxUSBClient::addFilter: Manufacturer=%s Product=%s Serial=%s\n",
              USBFilterGetString(pFilter, USBFILTERIDX_MANUFACTURER_STR)  ? USBFilterGetString(pFilter, USBFILTERIDX_MANUFACTURER_STR)  : "<null>",
              USBFilterGetString(pFilter, USBFILTERIDX_PRODUCT_STR)       ? USBFilterGetString(pFilter, USBFILTERIDX_PRODUCT_STR)       : "<null>",
              USBFilterGetString(pFilter, USBFILTERIDX_SERIAL_NUMBER_STR) ? USBFilterGetString(pFilter, USBFILTERIDX_SERIAL_NUMBER_STR) : "<null>"));
#endif

    /*
     * Since we cannot query the bus number, make sure the filter
     * isn't requiring that field to be present.
     */
    int rc = USBFilterSetMustBePresent(pFilter, USBFILTERIDX_BUS, false /* fMustBePresent */); AssertRC(rc);

    /*
     * Add the filter.
     */
    pOut->uId = 0;
    pOut->rc = VBoxUSBFilterAdd(pFilter, m_Process, &pOut->uId);

    Log(("VBoxUSBClient::addFilter: returns *pOut={.rc=%d, .uId=%p}\n", pOut->rc, (void *)pOut->uId));
    return kIOReturnSuccess;
}


/**
 * Removes filter user request.
 *
 * @returns IOKit status code.
 * @param   puId    Where to get the filter ID.
 * @param   prc     Where to store the return code.
 * @param   cbIn    sizeof(*puId).
 * @param   pcbOut  In/Out - sizeof(*prc).
 */
IOReturn
org_virtualbox_VBoxUSBClient::removeFilter(uintptr_t *puId, int *prc, IOByteCount cbIn, IOByteCount *pcbOut)
{
    Log(("VBoxUSBClient::removeFilter: [%p:{.m_Process=%d}] *puId=%p m_Proc\n", this, (int)m_Process, *puId));

    /*
     * Validate input.
     */
    if (RT_UNLIKELY(    cbIn != sizeof(*puId)
                    ||  *pcbOut != sizeof(*prc)))
    {
        printf("VBoxUSBClient::removeFilter: cbIn=%#x expected %#x; *pcbOut=%#x expected %#x\n",
               (int)cbIn, (int)sizeof(*puId), (int)*pcbOut, (int)sizeof(*prc));
        return kIOReturnBadArgument;
    }

    /*
     * Remove the filter.
     */
    *prc = VBoxUSBFilterRemove(m_Process, *puId);

    Log(("VBoxUSBClient::removeFilter: returns *prc=%d\n", *prc));
    return kIOReturnSuccess;
}


/**
 * Checks whether the specified task is a VBoxUSB client task or not.
 *
 * This is used to validate clients trying to open any of the device
 * or interfaces that we've hijacked.
 *
 * @returns true / false.
 * @param   ClientTask      The task.
 *
 * @remark  This protecting against other user clients is not currently implemented
 *          as it turned out to be more bothersome than first imagined.
 */
/* static*/ bool
org_virtualbox_VBoxUSBClient::isClientTask(task_t ClientTask)
{
    VBOXUSB_LOCK();

    for (org_virtualbox_VBoxUSBClient *pCur = s_pHead; pCur; pCur = pCur->m_pNext)
        if (pCur->m_Task == ClientTask)
        {
            VBOXUSB_UNLOCK();
            return true;
        }

    VBOXUSB_UNLOCK();
    return false;
}














/*
 *
 * org_virtualbox_VBoxUSBDevice
 *
 */

/**
 * Initialize instance data.
 *
 * @returns Success indicator.
 * @param   pDictionary     The dictionary that will become the registry entry's
 *                          property table, or NULL. Hand it up to our parents.
 */
bool
org_virtualbox_VBoxUSBDevice::init(OSDictionary *pDictionary)
{
    uint32_t cInstances = ASMAtomicIncU32(&g_cInstances);
    Log(("VBoxUSBDevice::init([%p], %p) new g_cInstances=%d\n", this, pDictionary, cInstances));
    RT_NOREF_PV(cInstances);

    m_pDevice = NULL;
    m_Owner = NIL_RTPROCESS;
    m_Client = NIL_RTPROCESS;
    m_uId = ~(uintptr_t)0;
    m_fOpen = false;
    m_fOpenOnWasClosed = false;
    m_fReleaseOnClose = false;
    m_fBeingUnloaded = false;
    m_pNext = NULL;
#ifdef DEBUG
    m_pNotifier = NULL;
#endif

    return IOUSBUserClientInit::init(pDictionary);
}

/**
 * Free the object.
 * @remark  Only for logging.
 */
void
org_virtualbox_VBoxUSBDevice::free()
{
    uint32_t cInstances = ASMAtomicDecU32(&g_cInstances); NOREF(cInstances);
    Log(("VBoxUSBDevice::free([%p]) new g_cInstances=%d\n", this, cInstances));
    IOUSBUserClientInit::free();
}


/**
 * The device/driver probing.
 *
 * I/O Kit will iterate all device drivers suitable for this kind of device
 * (this is something it figures out from the property file) and call their
 * probe() method in order to try determine which is the best match for the
 * device. We will match the device against the registered filters and set
 * a ridiculously high score if we find it, thus making it extremely likely
 * that we'll be the first driver to be started. We'll also set a couple of
 * attributes so that it's not necessary to do a rematch in init to find
 * the appropriate filter (might not be necessary..., see todo).
 *
 * @returns Service instance to be started and *pi32Score if matching.
 *          NULL if not a device suitable for this driver.
 *
 * @param   pProvider       The provider instance.
 * @param   pi32Score       Where to store the probe score.
 */
IOService *
org_virtualbox_VBoxUSBDevice::probe(IOService *pProvider, SInt32 *pi32Score)
{
    Log(("VBoxUSBDevice::probe([%p], %p {%s}, %p={%d})\n", this,
             pProvider, pProvider->getName(), pi32Score, pi32Score ? *pi32Score : 0));

    /*
     * Check against filters.
     */
    USBFILTER Device;
    USBFilterInit(&Device, USBFILTERTYPE_CAPTURE);

    static const struct
    {
        const char     *pszName;
        USBFILTERIDX    enmIdx;
        bool            fNumeric;
    } s_aProps[] =
    {
        { kUSBVendorID,             USBFILTERIDX_VENDOR_ID,         true },
        { kUSBProductID,            USBFILTERIDX_PRODUCT_ID,        true },
        { kUSBDeviceReleaseNumber,  USBFILTERIDX_DEVICE_REV,        true },
        { kUSBDeviceClass,          USBFILTERIDX_DEVICE_CLASS,      true },
        { kUSBDeviceSubClass,       USBFILTERIDX_DEVICE_SUB_CLASS,  true },
        { kUSBDeviceProtocol,       USBFILTERIDX_DEVICE_PROTOCOL,   true },
        { "PortNum",                USBFILTERIDX_PORT,              true },
        /// @todo {          ,                USBFILTERIDX_BUS,              true }, - must be derived :-/
        /// Seems to be the upper byte of locationID and our "grand parent" has a USBBusNumber prop.
        { "USB Vendor Name",        USBFILTERIDX_MANUFACTURER_STR,  false },
        { "USB Product Name",       USBFILTERIDX_PRODUCT_STR,       false },
        { "USB Serial Number",      USBFILTERIDX_SERIAL_NUMBER_STR, false },
    };
    for (unsigned i = 0; i < RT_ELEMENTS(s_aProps); i++)
    {
        OSObject *pObj = pProvider->getProperty(s_aProps[i].pszName);
        if (!pObj)
            continue;
        if (s_aProps[i].fNumeric)
        {
            OSNumber *pNum = OSDynamicCast(OSNumber, pObj);
            if (pNum)
            {
                uint16_t u16 = pNum->unsigned16BitValue();
                Log2(("VBoxUSBDevice::probe: %d/%s - %#x (32bit=%#x)\n", i, s_aProps[i].pszName, u16, pNum->unsigned32BitValue()));
                int vrc = USBFilterSetNumExact(&Device, s_aProps[i].enmIdx, u16, true);
                if (RT_FAILURE(vrc))
                    Log(("VBoxUSBDevice::probe: pObj=%p pNum=%p - %d/%s - rc=%d!\n", pObj, pNum, i, s_aProps[i].pszName, vrc));
            }
            else
                Log(("VBoxUSBDevice::probe: pObj=%p pNum=%p - %d/%s!\n", pObj, pNum, i, s_aProps[i].pszName));
        }
        else
        {
            OSString *pStr = OSDynamicCast(OSString, pObj);
            if (pStr)
            {
                Log2(("VBoxUSBDevice::probe: %d/%s - %s\n", i, s_aProps[i].pszName, pStr->getCStringNoCopy()));
                int vrc = USBFilterSetStringExact(&Device, s_aProps[i].enmIdx, pStr->getCStringNoCopy(),
                                                  true /*fMustBePresent*/, true /*fPurge*/);
                if (RT_FAILURE(vrc))
                    Log(("VBoxUSBDevice::probe: pObj=%p pStr=%p - %d/%s - rc=%d!\n", pObj, pStr, i, s_aProps[i].pszName, vrc));
            }
            else
                Log(("VBoxUSBDevice::probe: pObj=%p pStr=%p - %d/%s\n", pObj, pStr, i, s_aProps[i].pszName));
        }
    }
    /** @todo try figure the blasted bus number */

    /*
     * Run filters on it.
     */
    uintptr_t uId = 0;
    RTPROCESS Owner = VBoxUSBFilterMatch(&Device, &uId);
    USBFilterDelete(&Device);
    if (Owner == NIL_RTPROCESS)
    {
        Log(("VBoxUSBDevice::probe: returns NULL uId=%d\n", uId));
        return NULL;
    }

    /*
     * It matched. Save the owner in the provider registry (hope that works).
     */
    /*IOService *pRet = IOUSBUserClientInit::probe(pProvider, pi32Score); - call always returns NULL on 10.11+ */
    /*AssertMsg(pRet == this, ("pRet=%p this=%p *pi32Score=%d \n", pRet, this, pi32Score ? *pi32Score : 0)); - call always returns NULL on 10.11+ */
    IOService *pRet = this;
    m_Owner = Owner;
    m_uId = uId;
    Log(("%p: m_Owner=%d m_uId=%d\n", this, (int)m_Owner, (int)m_uId));
    *pi32Score = _1G;
    Log(("VBoxUSBDevice::probe: returns %p and *pi32Score=%d\n", pRet, *pi32Score));
    return pRet;
}


/**
 * Try start the device driver.
 *
 * We will do device linking, copy the filter and owner properties from the provider,
 * set the client property, retain the device, and try open (seize) the device.
 *
 * @returns Success indicator.
 * @param   pProvider       The provider instance.
 */
bool
org_virtualbox_VBoxUSBDevice::start(IOService *pProvider)
{
    Log(("VBoxUSBDevice::start([%p:{.m_Owner=%d, .m_uId=%p}], %p {%s})\n",
             this, m_Owner, m_uId, pProvider, pProvider->getName()));

    m_pDevice = OSDynamicCast(IOUSBDevice, pProvider);
    if (!m_pDevice)
    {
        printf("VBoxUSBDevice::start([%p], %p {%s}): failed!\n", this, pProvider, pProvider->getName());
        return false;
    }

#ifdef DEBUG
    /* for some extra log messages */
    m_pNotifier = pProvider->registerInterest(gIOGeneralInterest,
                                              &org_virtualbox_VBoxUSBDevice::MyInterestHandler,
                                              this,     /* pvTarget */
                                              NULL);    /* pvRefCon */
#endif

    /*
     * Exploit IOUSBUserClientInit to process IOProviderMergeProperties.
     */
    IOUSBUserClientInit::start(pProvider); /* returns false */

    /*
     * Link ourselves into the list of hijacked device.
     */
    VBOXUSB_LOCK();

    m_pNext = s_pHead;
    s_pHead = this;

    VBOXUSB_UNLOCK();

    /*
     * Set the VBoxUSB properties.
     */
    if (!setProperty(VBOXUSB_OWNER_KEY, (unsigned long long)m_Owner, sizeof(m_Owner) * 8 /* bits */))
        Log(("VBoxUSBDevice::start: failed to set the '" VBOXUSB_OWNER_KEY "' property!\n"));
    if (!setProperty(VBOXUSB_CLIENT_KEY, (unsigned long long)m_Client, sizeof(m_Client) * 8 /* bits */))
        Log(("VBoxUSBDevice::start: failed to set the '" VBOXUSB_CLIENT_KEY "' property!\n"));
    if (!setProperty(VBOXUSB_FILTER_KEY, (unsigned long long)m_uId, sizeof(m_uId) * 8 /* bits */))
        Log(("VBoxUSBDevice::start: failed to set the '" VBOXUSB_FILTER_KEY "' property!\n"));

    /*
     * Retain and open the device.
     */
    m_pDevice->retain();
    m_fOpen = m_pDevice->open(this, kIOServiceSeize, 0);
    if (!m_fOpen)
        Log(("VBoxUSBDevice::start: failed to open the device!\n"));
    m_fOpenOnWasClosed = !m_fOpen;

    Log(("VBoxUSBDevice::start: returns %d\n", true));
    return true;
}


/**
 * Stop the device driver.
 *
 * We'll unlink the device, start device re-enumeration and close it. And call
 * the parent stop method of course.
 *
 * @param   pProvider       The provider instance.
 */
void
org_virtualbox_VBoxUSBDevice::stop(IOService *pProvider)
{
    Log(("VBoxUSBDevice::stop([%p], %p {%s})\n", this, pProvider, pProvider->getName()));

    /*
     * Remove ourselves from the list of device.
     */
    VBOXUSB_LOCK();

    org_virtualbox_VBoxUSBDevice *pPrev = NULL;
    for (org_virtualbox_VBoxUSBDevice *pCur = s_pHead; pCur; pCur = pCur->m_pNext)
    {
        if (pCur == this)
        {
            if (pPrev)
                pPrev->m_pNext = m_pNext;
            else
                s_pHead = m_pNext;
            m_pNext = NULL;
            break;
        }
        pPrev = pCur;
    }

    VBOXUSB_UNLOCK();

    /*
     * Should we release the device?
     */
    if (m_fBeingUnloaded)
    {
        if (m_pDevice)
        {
            IOReturn irc = m_pDevice->ReEnumerateDevice(0); NOREF(irc);
            Log(("VBoxUSBDevice::stop([%p], %p {%s}): m_pDevice=%p unload & ReEnumerateDevice -> %#x\n",
                     this, pProvider, pProvider->getName(), m_pDevice, irc));
        }
        else
        {
            IOUSBDevice *pDevice = OSDynamicCast(IOUSBDevice, pProvider);
            if (pDevice)
            {
                IOReturn irc = pDevice->ReEnumerateDevice(0); NOREF(irc);
                Log(("VBoxUSBDevice::stop([%p], %p {%s}): pDevice=%p unload & ReEnumerateDevice -> %#x\n",
                         this, pProvider, pProvider->getName(), pDevice, irc));
            }
            else
                Log(("VBoxUSBDevice::stop([%p], %p {%s}): failed to cast provider to IOUSBDevice\n",
                         this, pProvider, pProvider->getName()));
        }
    }
    else if (m_fReleaseOnClose)
    {
        ASMAtomicWriteBool(&m_fReleaseOnClose, false);
        if (m_pDevice)
        {
            IOReturn irc = m_pDevice->ReEnumerateDevice(0); NOREF(irc);
            Log(("VBoxUSBDevice::stop([%p], %p {%s}): m_pDevice=%p close & ReEnumerateDevice -> %#x\n",
                     this, pProvider, pProvider->getName(), m_pDevice, irc));
        }
    }

    /*
     * Close and release the IOUSBDevice if didn't do that already in message().
     */
    if (m_pDevice)
    {
        /* close it */
        if (m_fOpen)
        {
            m_fOpenOnWasClosed = false;
            m_fOpen = false;
            m_pDevice->close(this, 0);
        }

        /* release it (see start()) */
        m_pDevice->release();
        m_pDevice = NULL;
    }

#ifdef DEBUG
    /* avoid crashing on unload. */
    if (m_pNotifier)
    {
        m_pNotifier->release();
        m_pNotifier = NULL;
    }
#endif

    IOUSBUserClientInit::stop(pProvider);
    Log(("VBoxUSBDevice::stop: returns void\n"));
}


/**
 * Terminate the service (initiate the destruction).
 * @remark  Only for logging.
 */
bool
org_virtualbox_VBoxUSBDevice::terminate(IOOptionBits fOptions)
{
    /* kIOServiceRecursing, kIOServiceRequired, kIOServiceTerminate, kIOServiceSynchronous - interesting option bits */
    Log(("VBoxUSBDevice::terminate([%p], %#x)\n", this, fOptions));

    /*
     * There aren't too many reasons why we gets terminated.
     * The most common one is that the device is being unplugged. Another is
     * that we've triggered reenumeration. In both cases we'll get a
     * kIOMessageServiceIsTerminated message before we're stopped.
     *
     * But, when we're unloaded the provider service isn't terminated, and
     * for some funny reason we're frequently causing kernel panics when the
     * device is detached (after we're unloaded). So, for now, let's try
     * re-enumerate it in stop.
     *
     * To avoid creating unnecessary trouble we'll try guess if we're being
     * unloaded from the option bit mask. (kIOServiceRecursing is private btw.)
     */
    /** @todo would be nice if there was a documented way of doing the unload detection this, or
     * figure out what exactly we're doing wrong in the unload scenario. */
    if ((fOptions & 0xffff) == (kIOServiceRequired | kIOServiceSynchronous))
        m_fBeingUnloaded = true;

    return IOUSBUserClientInit::terminate(fOptions);
}


/**
 * Intercept open requests and only let Mr. Right (the VM process) open the device.
 * This is where it all gets a bit complicated...
 *
 * @return  Status code.
 *
 * @param   enmMsg          The message number.
 * @param   pProvider       Pointer to the provider instance.
 * @param   pvArg           Message argument.
 */
IOReturn
org_virtualbox_VBoxUSBDevice::message(UInt32 enmMsg, IOService *pProvider, void *pvArg)
{
    Log(("VBoxUSBDevice::message([%p], %#x {%s}, %p {%s}, %p) - pid=%d\n",
         this, enmMsg, DbgGetIOKitMessageName(enmMsg), pProvider, pProvider->getName(), pvArg, RTProcSelf()));

    IOReturn irc;
    switch (enmMsg)
    {
        /*
         * This message is send to the current IOService client from IOService::handleOpen(),
         * expecting it to call pProvider->close() if it agrees to the other party seizing
         * the service. It is also called in IOService::didTerminate() and perhaps some other
         * odd places. The way to find out is to examin the pvArg, which would be including
         * kIOServiceSeize if it's the handleOpen case.
         *
         * How to validate that the other end is actually our VM process? Well, IOKit doesn't
         * provide any clue about the new client really. But fortunately, it seems like the
         * calling task/process context when the VM tries to open the device is the VM process.
         * We'll ASSUME this'll remain like this for now...
         */
        case kIOMessageServiceIsRequestingClose:
            irc = kIOReturnExclusiveAccess;
            /* If it's not a seize request, assume it's didTerminate and pray that it isn't a rouge driver.
               ... weird, doesn't seem to match for the post has-terminated messages. */
            if (!((uintptr_t)pvArg & kIOServiceSeize))
            {
                Log(("VBoxUSBDevice::message([%p],%p {%s}, %p) - pid=%d: not seize - closing...\n",
                         this, pProvider, pProvider->getName(), pvArg, RTProcSelf()));
                m_fOpen = false;
                m_fOpenOnWasClosed = false;
                if (m_pDevice)
                    m_pDevice->close(this, 0);
                m_Client = NIL_RTPROCESS;
                irc = kIOReturnSuccess;
            }
            else
            {
                if (org_virtualbox_VBoxUSBClient::isClientTask(current_task()))
                {
                    Log(("VBoxUSBDevice::message([%p],%p {%s}, %p) - pid=%d task=%p: client process, closing.\n",
                             this, pProvider, pProvider->getName(), pvArg, RTProcSelf(), current_task()));
                    m_fOpen = false;
                    m_fOpenOnWasClosed = false;
                    if (m_pDevice)
                        m_pDevice->close(this, 0);
                    m_fOpenOnWasClosed = true;
                    m_Client = RTProcSelf();
                    irc = kIOReturnSuccess;
                }
                else
                    Log(("VBoxUSBDevice::message([%p],%p {%s}, %p) - pid=%d task=%p: not client process!\n",
                             this, pProvider, pProvider->getName(), pvArg, RTProcSelf(), current_task()));
            }
            if (!setProperty(VBOXUSB_CLIENT_KEY, (unsigned long long)m_Client, sizeof(m_Client) * 8 /* bits */))
                Log(("VBoxUSBDevice::message: failed to set the '" VBOXUSB_CLIENT_KEY "' property!\n"));
            break;

        /*
         * The service was closed by the current client.
         * Update the client property, check for scheduled re-enumeration and re-open.
         *
         * Note that we will not be called if we're doing the closing. (Even if we was
         * called in that case, the code should be able to handle it.)
         */
        case kIOMessageServiceWasClosed:
            /*
             * Update the client property value.
             */
            if (m_Client != NIL_RTPROCESS)
            {
                m_Client = NIL_RTPROCESS;
                if (!setProperty(VBOXUSB_CLIENT_KEY, (unsigned long long)m_Client, sizeof(m_Client) * 8 /* bits */))
                    Log(("VBoxUSBDevice::message: failed to set the '" VBOXUSB_CLIENT_KEY "' property!\n"));
            }

            if (m_pDevice)
            {
                /*
                 * Should we release the device?
                 */
                if (ASMAtomicXchgBool(&m_fReleaseOnClose, false))
                {
                    m_fOpenOnWasClosed = false;
                    irc = m_pDevice->ReEnumerateDevice(0);
                    Log(("VBoxUSBDevice::message([%p], %p {%s}) - ReEnumerateDevice() -> %#x\n",
                             this, pProvider, pProvider->getName(), irc));
                }
                /*
                 * Should we attempt to re-open the device?
                 */
                else if (m_fOpenOnWasClosed)
                {
                    Log(("VBoxUSBDevice::message: attempting to re-open the device...\n"));
                    m_fOpenOnWasClosed = false;
                    m_fOpen = m_pDevice->open(this, kIOServiceSeize, 0);
                    if (!m_fOpen)
                        Log(("VBoxUSBDevice::message: failed to open the device!\n"));
                    m_fOpenOnWasClosed = !m_fOpen;
                }
            }

            irc = IOUSBUserClientInit::message(enmMsg, pProvider, pvArg);
            break;

        /*
         * The IOUSBDevice is shutting down, so close it if we've opened it.
         */
        case kIOMessageServiceIsTerminated:
            m_fBeingUnloaded = false;
            ASMAtomicWriteBool(&m_fReleaseOnClose, false);
            if (m_pDevice)
            {
                /* close it */
                if (m_fOpen)
                {
                    m_fOpen = false;
                    m_fOpenOnWasClosed = false;
                    Log(("VBoxUSBDevice::message: closing the device (%p)...\n", m_pDevice));
                    m_pDevice->close(this, 0);
                }

                /* release it (see start()) */
                Log(("VBoxUSBDevice::message: releasing the device (%p)...\n", m_pDevice));
                m_pDevice->release();
                m_pDevice = NULL;
            }

            irc = IOUSBUserClientInit::message(enmMsg, pProvider, pvArg);
            break;

        default:
            irc = IOUSBUserClientInit::message(enmMsg, pProvider, pvArg);
            break;
    }

    Log(("VBoxUSBDevice::message([%p], %#x {%s}, %p {%s}, %p) -> %#x\n",
         this, enmMsg, DbgGetIOKitMessageName(enmMsg), pProvider, pProvider->getName(), pvArg, irc));
    return irc;
}


/**
 * Schedule all devices belonging to the specified process for release.
 *
 * Devices that aren't currently in use will be released immediately.
 *
 * @param   Owner       The owner process.
 */
/* static */ void
org_virtualbox_VBoxUSBDevice::scheduleReleaseByOwner(RTPROCESS Owner)
{
    Log2(("VBoxUSBDevice::scheduleReleaseByOwner: Owner=%d\n", Owner));
    AssertReturnVoid(Owner && Owner != NIL_RTPROCESS);

    /*
     * Walk the list of devices looking for device belonging to this process.
     *
     * If we release a device, we have to lave the spinlock and will therefore
     * have to restart the search.
     */
    VBOXUSB_LOCK();

    org_virtualbox_VBoxUSBDevice *pCur;
    do
    {
        for (pCur = s_pHead; pCur; pCur = pCur->m_pNext)
        {
            Log2(("VBoxUSBDevice::scheduleReleaseByOwner: pCur=%p m_Owner=%d (%s) m_fReleaseOnClose=%d\n",
                      pCur, pCur->m_Owner, pCur->m_Owner == Owner ? "match" : "mismatch", pCur->m_fReleaseOnClose));
            if (pCur->m_Owner == Owner)
            {
                /* make sure we won't hit it again. */
                pCur->m_Owner = NIL_RTPROCESS;
                IOUSBDevice *pDevice = pCur->m_pDevice;
                if (    pDevice
                    &&  !pCur->m_fReleaseOnClose)
                {
                    pCur->m_fOpenOnWasClosed = false;
                    if (pCur->m_Client != NIL_RTPROCESS)
                    {
                        /* It's currently open, so just schedule it for re-enumeration on close. */
                        ASMAtomicWriteBool(&pCur->m_fReleaseOnClose, true);
                        Log(("VBoxUSBDevice::scheduleReleaseByOwner: %p {%s} - used by %d\n",
                                 pDevice, pDevice->getName(), pCur->m_Client));
                    }
                    else
                    {
                        /*
                         * Get the USBDevice object and do the re-enumeration now.
                         * Retain the device so we don't run into any trouble.
                         */
                        pDevice->retain();
                        VBOXUSB_UNLOCK();

                        IOReturn irc = pDevice->ReEnumerateDevice(0); NOREF(irc);
                        Log(("VBoxUSBDevice::scheduleReleaseByOwner: %p {%s} - ReEnumerateDevice -> %#x\n",
                                 pDevice, pDevice->getName(), irc));

                        pDevice->release();
                        VBOXUSB_LOCK();
                        break;
                    }
                }
            }
        }
    } while (pCur);

    VBOXUSB_UNLOCK();
}


#ifdef DEBUG
/*static*/ IOReturn
org_virtualbox_VBoxUSBDevice::MyInterestHandler(void *pvTarget, void *pvRefCon, UInt32 enmMsgType,
                                                IOService *pProvider, void * pvMsgArg, vm_size_t cbMsgArg)
{
    org_virtualbox_VBoxUSBDevice *pThis = (org_virtualbox_VBoxUSBDevice *)pvTarget;
    if (!pThis)
        return kIOReturnError;

    switch (enmMsgType)
    {
        case kIOMessageServiceIsAttemptingOpen:
            /* pvMsgArg == the open() fOptions, so we could check for kIOServiceSeize if we care.
               We'll also get a kIIOServiceRequestingClose message() for that...  */
            Log(("VBoxUSBDevice::MyInterestHandler: kIOMessageServiceIsAttemptingOpen - pvRefCon=%p pProvider=%p pvMsgArg=%p cbMsgArg=%d\n",
                 pvRefCon, pProvider, pvMsgArg, cbMsgArg));
            break;

        case kIOMessageServiceWasClosed:
            Log(("VBoxUSBDevice::MyInterestHandler: kIOMessageServiceWasClosed - pvRefCon=%p pProvider=%p pvMsgArg=%p cbMsgArg=%d\n",
                 pvRefCon, pProvider, pvMsgArg, cbMsgArg));
            break;

        case kIOMessageServiceIsTerminated:
            Log(("VBoxUSBDevice::MyInterestHandler: kIOMessageServiceIsTerminated - pvRefCon=%p pProvider=%p pvMsgArg=%p cbMsgArg=%d\n",
                 pvRefCon, pProvider, pvMsgArg, cbMsgArg));
            break;

        case kIOUSBMessagePortHasBeenReset:
            Log(("VBoxUSBDevice::MyInterestHandler: kIOUSBMessagePortHasBeenReset - pvRefCon=%p pProvider=%p pvMsgArg=%p cbMsgArg=%d\n",
                 pvRefCon, pProvider, pvMsgArg, cbMsgArg));
            break;

        default:
            Log(("VBoxUSBDevice::MyInterestHandler: %#x (%s) - pvRefCon=%p pProvider=%p pvMsgArg=%p cbMsgArg=%d\n",
                 enmMsgType, DbgGetIOKitMessageName(enmMsgType), pvRefCon, pProvider, pvMsgArg, cbMsgArg));
            break;
    }

    return kIOReturnSuccess;
}
#endif /* DEBUG */














/*
 *
 * org_virtualbox_VBoxUSBInterface
 *
 */

/**
 * Initialize our data members.
 */
bool
org_virtualbox_VBoxUSBInterface::init(OSDictionary *pDictionary)
{
    uint32_t cInstances = ASMAtomicIncU32(&g_cInstances);
    Log(("VBoxUSBInterface::init([%p], %p) new g_cInstances=%d\n", this, pDictionary, cInstances));
    RT_NOREF_PV(cInstances);

    m_pInterface = NULL;
    m_fOpen = false;
    m_fOpenOnWasClosed = false;

    return IOUSBUserClientInit::init(pDictionary);
}


/**
 * Free the object.
 * @remark  Only for logging.
 */
void
org_virtualbox_VBoxUSBInterface::free()
{
    uint32_t cInstances = ASMAtomicDecU32(&g_cInstances); NOREF(cInstances);
    Log(("VBoxUSBInterfaces::free([%p]) new g_cInstances=%d\n", this, cInstances));
    IOUSBUserClientInit::free();
}


/**
 * Probe the interface to see if we're the right driver for it.
 *
 * We implement this similarly to org_virtualbox_VBoxUSBDevice, except that
 * we don't bother matching filters but instead just check if the parent is
 * handled by org_virtualbox_VBoxUSBDevice or not.
 */
IOService *
org_virtualbox_VBoxUSBInterface::probe(IOService *pProvider, SInt32 *pi32Score)
{
    Log(("VBoxUSBInterface::probe([%p], %p {%s}, %p={%d})\n", this,
             pProvider, pProvider->getName(), pi32Score, pi32Score ? *pi32Score : 0));

    /*
     * Check if VBoxUSBDevice is the parent's driver.
     */
    bool fHijackIt = false;
    const IORegistryPlane *pServicePlane = getPlane(kIOServicePlane);
    IORegistryEntry *pParent = pProvider->getParentEntry(pServicePlane);
    if (pParent)
    {
        Log(("VBoxUSBInterface::probe: pParent=%p {%s}\n", pParent, pParent->getName()));

        OSIterator *pSiblings = pParent->getChildIterator(pServicePlane);
        if (pSiblings)
        {
            IORegistryEntry *pSibling;
            while ( (pSibling = OSDynamicCast(IORegistryEntry, pSiblings->getNextObject())) )
            {
                const OSMetaClass *pMetaClass = pSibling->getMetaClass();
                Log2(("sibling: %p - %s - %s\n", pMetaClass, pSibling->getName(), pMetaClass->getClassName()));
                if (pMetaClass == &org_virtualbox_VBoxUSBDevice::gMetaClass)
                {
                    fHijackIt = true;
                    break;
                }
            }
            pSiblings->release();
        }
    }
    if (!fHijackIt)
    {
        Log(("VBoxUSBInterface::probe: returns NULL\n"));
        return NULL;
    }

    /* IOService *pRet = IOUSBUserClientInit::probe(pProvider, pi32Score); - call always returns NULL on 10.11+ */
    IOService *pRet = this;
    *pi32Score = _1G;
    Log(("VBoxUSBInterface::probe: returns %p and *pi32Score=%d - hijack it.\n", pRet, *pi32Score));
    return pRet;
}


/**
 * Start the driver (this), retain and open the USB interface object (pProvider).
 */
bool
org_virtualbox_VBoxUSBInterface::start(IOService *pProvider)
{
    Log(("VBoxUSBInterface::start([%p], %p {%s})\n", this, pProvider, pProvider->getName()));

    /*
     * Exploit IOUSBUserClientInit to process IOProviderMergeProperties.
     */
    IOUSBUserClientInit::start(pProvider); /* returns false */

    /*
     * Retain the and open the interface (stop() or message() cleans up).
     */
    bool fRc = true;
    m_pInterface = OSDynamicCast(IOUSBInterface, pProvider);
    if (m_pInterface)
    {
        m_pInterface->retain();
        m_fOpen = m_pInterface->open(this, kIOServiceSeize, 0);
        if (!m_fOpen)
            Log(("VBoxUSBInterface::start: failed to open the interface!\n"));
        m_fOpenOnWasClosed = !m_fOpen;
    }
    else
    {
        printf("VBoxUSBInterface::start([%p], %p {%s}): failed!\n", this, pProvider, pProvider->getName());
        fRc = false;
    }

    Log(("VBoxUSBInterface::start: returns %d\n", fRc));
    return fRc;
}


/**
 * Close and release the USB interface object (pProvider) and stop the driver (this).
 */
void
org_virtualbox_VBoxUSBInterface::stop(IOService *pProvider)
{
    Log(("org_virtualbox_VBoxUSBInterface::stop([%p], %p {%s})\n", this, pProvider, pProvider->getName()));

    /*
     * Close and release the IOUSBInterface if didn't do that already in message().
     */
    if (m_pInterface)
    {
        /* close it */
        if (m_fOpen)
        {
            m_fOpenOnWasClosed = false;
            m_fOpen = false;
            m_pInterface->close(this, 0);
        }

        /* release it (see start()) */
        m_pInterface->release();
        m_pInterface = NULL;
    }

    IOUSBUserClientInit::stop(pProvider);
    Log(("VBoxUSBInterface::stop: returns void\n"));
}


/**
 * Terminate the service (initiate the destruction).
 * @remark  Only for logging.
 */
bool
org_virtualbox_VBoxUSBInterface::terminate(IOOptionBits fOptions)
{
    /* kIOServiceRecursing, kIOServiceRequired, kIOServiceTerminate, kIOServiceSynchronous - interesting option bits */
    Log(("VBoxUSBInterface::terminate([%p], %#x)\n", this, fOptions));
    return IOUSBUserClientInit::terminate(fOptions);
}


/**
 * @copydoc org_virtualbox_VBoxUSBDevice::message
 */
IOReturn
org_virtualbox_VBoxUSBInterface::message(UInt32 enmMsg, IOService *pProvider, void *pvArg)
{
    Log(("VBoxUSBInterface::message([%p], %#x {%s}, %p {%s}, %p)\n",
         this, enmMsg, DbgGetIOKitMessageName(enmMsg), pProvider, pProvider->getName(), pvArg));

    IOReturn irc;
    switch (enmMsg)
    {
        /*
         * See explanation in org_virtualbox_VBoxUSBDevice::message.
         */
        case kIOMessageServiceIsRequestingClose:
            irc = kIOReturnExclusiveAccess;
            if (!((uintptr_t)pvArg & kIOServiceSeize))
            {
                Log(("VBoxUSBInterface::message([%p],%p {%s}, %p) - pid=%d: not seize - closing...\n",
                         this, pProvider, pProvider->getName(), pvArg, RTProcSelf()));
                m_fOpen = false;
                m_fOpenOnWasClosed = false;
                if (m_pInterface)
                    m_pInterface->close(this, 0);
                irc = kIOReturnSuccess;
            }
            else
            {
                if (org_virtualbox_VBoxUSBClient::isClientTask(current_task()))
                {
                    Log(("VBoxUSBInterface::message([%p],%p {%s}, %p) - pid=%d task=%p: client process, closing.\n",
                             this, pProvider, pProvider->getName(), pvArg, RTProcSelf(), current_task()));
                    m_fOpen = false;
                    m_fOpenOnWasClosed = false;
                    if (m_pInterface)
                        m_pInterface->close(this, 0);
                    m_fOpenOnWasClosed = true;
                    irc = kIOReturnSuccess;
                }
                else
                    Log(("VBoxUSBInterface::message([%p],%p {%s}, %p) - pid=%d task=%p: not client process!\n",
                             this, pProvider, pProvider->getName(), pvArg, RTProcSelf(), current_task()));
            }
            break;

        /*
         * The service was closed by the current client, check for re-open.
         */
        case kIOMessageServiceWasClosed:
            if (m_pInterface && m_fOpenOnWasClosed)
            {
                Log(("VBoxUSBInterface::message: attempting to re-open the interface...\n"));
                m_fOpenOnWasClosed = false;
                m_fOpen = m_pInterface->open(this, kIOServiceSeize, 0);
                if (!m_fOpen)
                    Log(("VBoxUSBInterface::message: failed to open the interface!\n"));
                m_fOpenOnWasClosed = !m_fOpen;
            }

            irc = IOUSBUserClientInit::message(enmMsg, pProvider, pvArg);
            break;

        /*
         * The IOUSBInterface/Device is shutting down, so close and release.
         */
        case kIOMessageServiceIsTerminated:
            if (m_pInterface)
            {
                /* close it */
                if (m_fOpen)
                {
                    m_fOpen = false;
                    m_fOpenOnWasClosed = false;
                    m_pInterface->close(this, 0);
                }

                /* release it (see start()) */
                m_pInterface->release();
                m_pInterface = NULL;
            }

            irc = IOUSBUserClientInit::message(enmMsg, pProvider, pvArg);
            break;

        default:
            irc = IOUSBUserClientInit::message(enmMsg, pProvider, pvArg);
            break;
    }

    Log(("VBoxUSBInterface::message([%p], %#x {%s}, %p {%s}, %p) -> %#x\n",
         this, enmMsg, DbgGetIOKitMessageName(enmMsg), pProvider, pProvider->getName(), pvArg, irc));
    return irc;
}

