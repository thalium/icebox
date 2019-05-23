/* $Id: VBoxGuest-darwin.cpp $ */
/** @file
 * VBoxGuest - Darwin Specifics.
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
#define LOG_GROUP LOG_GROUP_VGDRV
/*
 * Deal with conflicts first.
 * PVM - BSD mess, that FreeBSD has correct a long time ago.
 * iprt/types.h before sys/param.h - prevents UINT32_C and friends.
 */
#include <iprt/types.h>
#include <sys/param.h>
#undef PVM

#include <IOKit/IOLib.h> /* Assert as function */

#include <VBox/version.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/process.h>
#include <iprt/power.h>
#include <iprt/semaphore.h>
#include <iprt/spinlock.h>
#include <iprt/string.h>
#include <VBox/err.h>
#include <VBox/log.h>

#include <mach/kmod.h>
#include <miscfs/devfs/devfs.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/ioccom.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOFilterInterruptEventSource.h>
#include "VBoxGuestInternal.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** The system device node name. */
#define DEVICE_NAME_SYS     "vboxguest"
/** The user device node name. */
#define DEVICE_NAME_USR     "vboxguestu"


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
static kern_return_t    vgdrvDarwinStart(struct kmod_info *pKModInfo, void *pvData);
static kern_return_t    vgdrvDarwinStop(struct kmod_info *pKModInfo, void *pvData);
static int              vgdrvDarwinCharDevRemove(void);

static int              vgdrvDarwinOpen(dev_t Dev, int fFlags, int fDevType, struct proc *pProcess);
static int              vgdrvDarwinClose(dev_t Dev, int fFlags, int fDevType, struct proc *pProcess);
static int              vgdrvDarwinIOCtlSlow(PVBOXGUESTSESSION pSession, u_long iCmd, caddr_t pData, struct proc *pProcess);
static int              vgdrvDarwinIOCtl(dev_t Dev, u_long iCmd, caddr_t pData, int fFlags, struct proc *pProcess);

static int              vgdrvDarwinErr2DarwinErr(int rc);

static IOReturn         vgdrvDarwinSleepHandler(void *pvTarget, void *pvRefCon, UInt32 uMessageType, IOService *pProvider, void *pvMessageArgument, vm_size_t argSize);
RT_C_DECLS_END


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * The service class for handling the VMMDev PCI device.
 *
 * Instantiated when the module is loaded (and on PCI hotplugging?).
 */
class org_virtualbox_VBoxGuest : public IOService
{
    OSDeclareDefaultStructors(org_virtualbox_VBoxGuest);

private:
    IOPCIDevice                *m_pIOPCIDevice;
    IOMemoryMap                *m_pMap;
    IOFilterInterruptEventSource *m_pInterruptSrc;

    bool setupVmmDevInterrupts(IOService *pProvider);
    bool disableVmmDevInterrupts(void);
    bool isVmmDev(IOPCIDevice *pIOPCIDevice);

protected:
    IOWorkLoop                *m_pWorkLoop;

public:
    virtual bool start(IOService *pProvider);
    virtual void stop(IOService *pProvider);
    virtual bool terminate(IOOptionBits fOptions);
    IOWorkLoop * getWorkLoop();
};

OSDefineMetaClassAndStructors(org_virtualbox_VBoxGuest, IOService);


/**
 * An attempt at getting that clientDied() notification.
 * I don't think it'll work as I cannot figure out where/what creates the correct
 * port right.
 *
 * Instantiated when userland does IOServiceOpen().
 */
class org_virtualbox_VBoxGuestClient : public IOUserClient
{
    OSDeclareDefaultStructors(org_virtualbox_VBoxGuestClient);

private:
    PVBOXGUESTSESSION           m_pSession;     /**< The session. */
    task_t                      m_Task;         /**< The client task. */
    org_virtualbox_VBoxGuest   *m_pProvider;    /**< The service provider. */

public:
    virtual bool initWithTask(task_t OwningTask, void *pvSecurityId, UInt32 u32Type);
    virtual bool start(IOService *pProvider);
    static  void sessionClose(RTPROCESS Process);
    virtual IOReturn clientClose(void);
};

OSDefineMetaClassAndStructors(org_virtualbox_VBoxGuestClient, IOUserClient);



/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Declare the module stuff.
 */
RT_C_DECLS_BEGIN
extern kern_return_t _start(struct kmod_info *pKModInfo, void *pvData);
extern kern_return_t _stop(struct kmod_info *pKModInfo, void *pvData);

KMOD_EXPLICIT_DECL(VBoxGuest, VBOX_VERSION_STRING, _start, _stop)
DECLHIDDEN(kmod_start_func_t *) _realmain = vgdrvDarwinStart;
DECLHIDDEN(kmod_stop_func_t *)  _antimain = vgdrvDarwinStop;
DECLHIDDEN(int)                 _kext_apple_cc = __APPLE_CC__;
RT_C_DECLS_END


/**
 * Device extention & session data association structure.
 */
static VBOXGUESTDEVEXT  g_DevExt;

/**
 * The character device switch table for the driver.
 */
static struct cdevsw    g_DevCW =
{
    /*.d_open     = */ vgdrvDarwinOpen,
    /*.d_close    = */ vgdrvDarwinClose,
    /*.d_read     = */ eno_rdwrt,
    /*.d_write    = */ eno_rdwrt,
    /*.d_ioctl    = */ vgdrvDarwinIOCtl,
    /*.d_stop     = */ eno_stop,
    /*.d_reset    = */ eno_reset,
    /*.d_ttys     = */ NULL,
    /*.d_select   = */ eno_select,
    /*.d_mmap     = */ eno_mmap,
    /*.d_strategy = */ eno_strat,
    /*.d_getc     = */ (void *)(uintptr_t)&enodev, //eno_getc,
    /*.d_putc     = */ (void *)(uintptr_t)&enodev, //eno_putc,
    /*.d_type     = */ 0
};

/** Major device number. */
static int                  g_iMajorDeviceNo    = -1;
/** Registered devfs device handle. */
static void                *g_hDevFsDeviceSys   = NULL;
/** Registered devfs device handle for the user device. */
static void                *g_hDevFsDeviceUsr   = NULL; /**< @todo 4 later */

/** Spinlock protecting g_apSessionHashTab. */
static RTSPINLOCK           g_Spinlock          = NIL_RTSPINLOCK;
/** Hash table */
static PVBOXGUESTSESSION    g_apSessionHashTab[19];
/** Calculates the index into g_apSessionHashTab.*/
#define SESSION_HASH(pid)   ((pid) % RT_ELEMENTS(g_apSessionHashTab))
/** The number of open sessions. */
static int32_t volatile     g_cSessions         = 0;
/** The number of IOService class instances. */
static bool volatile        g_fInstantiated     = 0;
/** The notifier handle for the sleep callback handler. */
static IONotifier          *g_pSleepNotifier    = NULL;

/* States of atimic variable aimed to protect dynamic object allocation in SMP environment. */
#define VBOXGUEST_OBJECT_UNINITIALIZED  (0)
#define VBOXGUEST_OBJECT_INITIALIZING   (1)
#define VBOXGUEST_OBJECT_INITIALIZED    (2)
#define VBOXGUEST_OBJECT_INVALID        (3)
/** Atomic variable used to protect work loop allocation when multiple threads attempt to obtain it. */
static uint8_t volatile     g_fWorkLoopCreated  = VBOXGUEST_OBJECT_UNINITIALIZED;


/**
 * Start the kernel module.
 */
static kern_return_t    vgdrvDarwinStart(struct kmod_info *pKModInfo, void *pvData)
{
    RT_NOREF(pKModInfo, pvData);

    /*
     * Initialize IPRT.
     */
    int rc = RTR0Init(0);
    if (RT_SUCCESS(rc))
    {
        Log(("VBoxGuest: driver loaded\n"));
        return KMOD_RETURN_SUCCESS;
    }

    printf("VBoxGuest: RTR0Init failed with rc=%d\n", rc);
    return KMOD_RETURN_FAILURE;
}


/**
 * Register VBoxGuest char device
 */
static int vgdrvDarwinCharDevInit(void)
{
    int rc = RTSpinlockCreate(&g_Spinlock, RTSPINLOCK_FLAGS_INTERRUPT_SAFE, "VBoxGuestDarwin");
    if (RT_SUCCESS(rc))
    {
        /*
         * Registering ourselves as a character device.
         */
        g_iMajorDeviceNo = cdevsw_add(-1, &g_DevCW);
        if (g_iMajorDeviceNo >= 0)
        {
            g_hDevFsDeviceSys = devfs_make_node(makedev(g_iMajorDeviceNo, 0), DEVFS_CHAR,
                                                UID_ROOT, GID_WHEEL, 0666, DEVICE_NAME_SYS);
            if (g_hDevFsDeviceSys != NULL)
            {
                /*
                 * Register a sleep/wakeup notification callback.
                 */
                g_pSleepNotifier = registerPrioritySleepWakeInterest(&vgdrvDarwinSleepHandler, &g_DevExt, NULL);
                if (g_pSleepNotifier != NULL)
                {
                    return KMOD_RETURN_SUCCESS;
                }
            }
        }
        vgdrvDarwinCharDevRemove();
    }
    return KMOD_RETURN_FAILURE;
}


/**
 * Stop the kernel module.
 */
static kern_return_t vgdrvDarwinStop(struct kmod_info *pKModInfo, void *pvData)
{
    RT_NOREF(pKModInfo, pvData);
    RTR0TermForced();

    printf("VBoxGuest: driver unloaded\n");
    return KMOD_RETURN_SUCCESS;
}


/* Unregister VBoxGuest char device */
static int vgdrvDarwinCharDevRemove(void)
{
    int rc = KMOD_RETURN_SUCCESS;

    if (g_pSleepNotifier)
    {
        g_pSleepNotifier->remove();
        g_pSleepNotifier = NULL;
    }

    if (g_hDevFsDeviceSys)
    {
        devfs_remove(g_hDevFsDeviceSys);
        g_hDevFsDeviceSys = NULL;
    }

    if (g_hDevFsDeviceUsr)
    {
        devfs_remove(g_hDevFsDeviceUsr);
        g_hDevFsDeviceUsr = NULL;
    }

    if (g_iMajorDeviceNo != -1)
    {
        int rc2 = cdevsw_remove(g_iMajorDeviceNo, &g_DevCW);
        Assert(rc2 == g_iMajorDeviceNo); NOREF(rc2);
        g_iMajorDeviceNo = -1;
    }

    if (g_Spinlock != NIL_RTSPINLOCK)
    {
        int rc2 = RTSpinlockDestroy(g_Spinlock); AssertRC(rc2);
        g_Spinlock = NIL_RTSPINLOCK;
    }

    return rc;
}


/**
 * Device open. Called on open /dev/vboxguest and (later) /dev/vboxguestu.
 *
 * @param   Dev         The device number.
 * @param   fFlags      ???.
 * @param   fDevType    ???.
 * @param   pProcess    The process issuing this request.
 */
static int vgdrvDarwinOpen(dev_t Dev, int fFlags, int fDevType, struct proc *pProcess)
{
    RT_NOREF(fFlags,  fDevType);

    /*
     * Only two minor devices numbers are allowed.
     */
    if (minor(Dev) != 0 && minor(Dev) != 1)
        return EACCES;

    /*
     * Find the session created by org_virtualbox_VBoxGuestClient, fail
     * if no such session, and mark it as opened. We set the uid & gid
     * here too, since that is more straight forward at this point.
     */
    //const bool          fUnrestricted = minor(Dev) == 0;
    int                 rc = VINF_SUCCESS;
    PVBOXGUESTSESSION   pSession = NULL;
    kauth_cred_t        pCred = kauth_cred_proc_ref(pProcess);
    if (pCred)
    {
        RTPROCESS       Process = RTProcSelf();
        unsigned        iHash = SESSION_HASH(Process);
        RTSpinlockAcquire(g_Spinlock);

        pSession = g_apSessionHashTab[iHash];
        while (pSession && pSession->Process != Process)
            pSession = pSession->pNextHash;
        if (pSession)
        {
            if (!pSession->fOpened)
            {
                pSession->fOpened = true;
                /*pSession->fUnrestricted = fUnrestricted; - later */
            }
            else
                rc = VERR_ALREADY_LOADED;
        }
        else
            rc = VERR_GENERAL_FAILURE;

        RTSpinlockRelease(g_Spinlock);
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1050
        kauth_cred_unref(&pCred);
#else  /* 10.4 */
        /* The 10.4u SDK headers and 10.4.11 kernel source have inconsistent definitions
           of kauth_cred_unref(), so use the other (now deprecated) API for releasing it. */
        kauth_cred_rele(pCred);
#endif /* 10.4 */
    }
    else
        rc = VERR_INVALID_PARAMETER;

    Log(("vgdrvDarwinOpen: g_DevExt=%p pSession=%p rc=%d pid=%d\n", &g_DevExt, pSession, rc, proc_pid(pProcess)));
    return vgdrvDarwinErr2DarwinErr(rc);
}


/**
 * Close device.
 */
static int vgdrvDarwinClose(dev_t Dev, int fFlags, int fDevType, struct proc *pProcess)
{
    RT_NOREF(Dev, fFlags, fDevType, pProcess);
    Log(("vgdrvDarwinClose: pid=%d\n", (int)RTProcSelf()));
    Assert(proc_pid(pProcess) == (int)RTProcSelf());

    /*
     * Hand the session closing to org_virtualbox_VBoxGuestClient.
     */
    org_virtualbox_VBoxGuestClient::sessionClose(RTProcSelf());
    return 0;
}


/**
 * Device I/O Control entry point.
 *
 * @returns Darwin for slow IOCtls and VBox status code for the fast ones.
 * @param   Dev         The device number (major+minor).
 * @param   iCmd        The IOCtl command.
 * @param   pData     Pointer to the request data.
 * @param   fFlags      Flag saying we're a character device (like we didn't know already).
 * @param   pProcess    The process issuing this request.
 */
static int vgdrvDarwinIOCtl(dev_t Dev, u_long iCmd, caddr_t pData, int fFlags, struct proc *pProcess)
{
    RT_NOREF(Dev, fFlags);
    //const bool          fUnrestricted = minor(Dev) == 0;
    const RTPROCESS     Process = proc_pid(pProcess);
    const unsigned      iHash = SESSION_HASH(Process);
    PVBOXGUESTSESSION   pSession;

    /*
     * Find the session.
     */
    RTSpinlockAcquire(g_Spinlock);
    pSession = g_apSessionHashTab[iHash];
    while (pSession && pSession->Process != Process && (/*later: pSession->fUnrestricted != fUnrestricted ||*/  !pSession->fOpened))
        pSession = pSession->pNextHash;
    RTSpinlockRelease(g_Spinlock);
    if (!pSession)
    {
        Log(("VBoxDrvDarwinIOCtl: WHAT?!? pSession == NULL! This must be a mistake... pid=%d iCmd=%#lx\n", (int)Process, iCmd));
        return EINVAL;
    }

    /*
     * Deal with the high-speed IOCtl.
     */
    if (VBGL_IOCTL_IS_FAST(iCmd))
        return VGDrvCommonIoCtlFast(iCmd, &g_DevExt, pSession);

    return vgdrvDarwinIOCtlSlow(pSession, iCmd, pData, pProcess);
}


/**
 * Worker for VBoxDrvDarwinIOCtl that takes the slow IOCtl functions.
 *
 * @returns Darwin errno.
 *
 * @param pSession  The session.
 * @param iCmd      The IOCtl command.
 * @param pData     Pointer to the request data.
 * @param pProcess  The calling process.
 */
static int vgdrvDarwinIOCtlSlow(PVBOXGUESTSESSION pSession, u_long iCmd, caddr_t pData, struct proc *pProcess)
{
    RT_NOREF(pProcess);
    LogFlow(("vgdrvDarwinIOCtlSlow: pSession=%p iCmd=%p pData=%p pProcess=%p\n", pSession, iCmd, pData, pProcess));


    /*
     * Buffered or unbuffered?
     */
    PVBGLREQHDR pHdr;
    user_addr_t pUser = 0;
    void *pvPageBuf = NULL;
    uint32_t cbReq = IOCPARM_LEN(iCmd);
    if ((IOC_DIRMASK & iCmd) == IOC_INOUT)
    {
        pHdr = (PVBGLREQHDR)pData;
        if (RT_UNLIKELY(cbReq < sizeof(*pHdr)))
        {
            LogRel(("vgdrvDarwinIOCtlSlow: cbReq=%#x < %#x; iCmd=%#lx\n", cbReq, (int)sizeof(*pHdr), iCmd));
            return EINVAL;
        }
        if (RT_UNLIKELY(pHdr->uVersion != VBGLREQHDR_VERSION))
        {
            LogRel(("vgdrvDarwinIOCtlSlow: bad uVersion=%#x; iCmd=%#lx\n", pHdr->uVersion, iCmd));
            return EINVAL;
        }
        if (RT_UNLIKELY(   RT_MAX(pHdr->cbIn, pHdr->cbOut) != cbReq
                        || pHdr->cbIn < sizeof(*pHdr)
                        || (pHdr->cbOut < sizeof(*pHdr) && pHdr->cbOut != 0)))
        {
            LogRel(("vgdrvDarwinIOCtlSlow: max(%#x,%#x) != %#x; iCmd=%#lx\n", pHdr->cbIn, pHdr->cbOut, cbReq, iCmd));
            return EINVAL;
        }
    }
    else if ((IOC_DIRMASK & iCmd) == IOC_VOID && !cbReq)
    {
        /*
         * Get the header and figure out how much we're gonna have to read.
         */
        VBGLREQHDR Hdr;
        pUser = (user_addr_t)*(void **)pData;
        int rc = copyin(pUser, &Hdr, sizeof(Hdr));
        if (RT_UNLIKELY(rc))
        {
            LogRel(("vgdrvDarwinIOCtlSlow: copyin(%llx,Hdr,) -> %#x; iCmd=%#lx\n", (unsigned long long)pUser, rc, iCmd));
            return rc;
        }
        if (RT_UNLIKELY(Hdr.uVersion != VBGLREQHDR_VERSION))
        {
            LogRel(("vgdrvDarwinIOCtlSlow: bad uVersion=%#x; iCmd=%#lx\n", Hdr.uVersion, iCmd));
            return EINVAL;
        }
        cbReq = RT_MAX(Hdr.cbIn, Hdr.cbOut);
        if (RT_UNLIKELY(   Hdr.cbIn < sizeof(Hdr)
                        || (Hdr.cbOut < sizeof(Hdr) && Hdr.cbOut != 0)
                        || cbReq > _1M*16))
        {
            LogRel(("vgdrvDarwinIOCtlSlow: max(%#x,%#x); iCmd=%#lx\n", Hdr.cbIn, Hdr.cbOut, iCmd));
            return EINVAL;
        }

        /*
         * Allocate buffer and copy in the data.
         */
        pHdr = (PVBGLREQHDR)RTMemTmpAlloc(cbReq);
        if (!pHdr)
            pvPageBuf = pHdr = (PVBGLREQHDR)IOMallocAligned(RT_ALIGN_Z(cbReq, PAGE_SIZE), 8);
        if (RT_UNLIKELY(!pHdr))
        {
            LogRel(("vgdrvDarwinIOCtlSlow: failed to allocate buffer of %d bytes; iCmd=%#lx\n", cbReq, iCmd));
            return ENOMEM;
        }
        rc = copyin(pUser, pHdr, Hdr.cbIn);
        if (RT_UNLIKELY(rc))
        {
            LogRel(("vgdrvDarwinIOCtlSlow: copyin(%llx,%p,%#x) -> %#x; iCmd=%#lx\n",
                    (unsigned long long)pUser, pHdr, Hdr.cbIn, rc, iCmd));
            if (pvPageBuf)
                IOFreeAligned(pvPageBuf, RT_ALIGN_Z(cbReq, PAGE_SIZE));
            else
                RTMemTmpFree(pHdr);
            return rc;
        }
        if (Hdr.cbIn < cbReq)
            RT_BZERO((uint8_t *)pHdr + Hdr.cbIn, cbReq - Hdr.cbIn);
    }
    else
    {
        Log(("vgdrvDarwinIOCtlSlow: huh? cbReq=%#x iCmd=%#lx\n", cbReq, iCmd));
        return EINVAL;
    }

    /*
     * Process the IOCtl.
     */
    int rc = VGDrvCommonIoCtl(iCmd, &g_DevExt, pSession, pHdr, cbReq);
    if (RT_LIKELY(!rc))
    {
        /*
         * If not buffered, copy back the buffer before returning.
         */
        if (pUser)
        {
            uint32_t cbOut = pHdr->cbOut;
            if (cbOut > cbReq)
            {
                LogRel(("vgdrvDarwinIOCtlSlow: too much output! %#x > %#x; uCmd=%#lx!\n", cbOut, cbReq, iCmd));
                cbOut = cbReq;
            }
            rc = copyout(pHdr, pUser, cbOut);
            if (RT_UNLIKELY(rc))
                LogRel(("vgdrvDarwinIOCtlSlow: copyout(%p,%llx,%#x) -> %d; uCmd=%#lx!\n",
                        pHdr, (unsigned long long)pUser, cbOut, rc, iCmd));

            /* cleanup */
            if (pvPageBuf)
                IOFreeAligned(pvPageBuf, RT_ALIGN_Z(cbReq, PAGE_SIZE));
            else
                RTMemTmpFree(pHdr);
        }
    }
    else
    {
        /*
         * The request failed, just clean up.
         */
        if (pUser)
        {
            if (pvPageBuf)
                IOFreeAligned(pvPageBuf, RT_ALIGN_Z(cbReq, PAGE_SIZE));
            else
                RTMemTmpFree(pHdr);
        }

        Log(("vgdrvDarwinIOCtlSlow: pid=%d iCmd=%lx pData=%p failed, rc=%d\n", proc_pid(pProcess), iCmd, (void *)pData, rc));
        rc = EINVAL;
    }

    Log2(("vgdrvDarwinIOCtlSlow: returns %d\n", rc));
    return rc;
}


/**
 * @note This code is duplicated on other platforms with variations, so please
 *       keep them all up to date when making changes!
 */
int VBOXCALL VBoxGuestIDC(void *pvSession, uintptr_t uReq, PVBGLREQHDR pReqHdr, size_t cbReq)
{
    /*
     * Simple request validation (common code does the rest).
     */
    int rc;
    if (   RT_VALID_PTR(pReqHdr)
        && cbReq >= sizeof(*pReqHdr))
    {
        /*
         * All requests except the connect one requires a valid session.
         */
        PVBOXGUESTSESSION pSession = (PVBOXGUESTSESSION)pvSession;
        if (pSession)
        {
            if (   RT_VALID_PTR(pSession)
                && pSession->pDevExt == &g_DevExt)
                rc = VGDrvCommonIoCtl(uReq, &g_DevExt, pSession, pReqHdr, cbReq);
            else
                rc = VERR_INVALID_HANDLE;
        }
        else if (uReq == VBGL_IOCTL_IDC_CONNECT)
        {
            rc = VGDrvCommonCreateKernelSession(&g_DevExt, &pSession);
            if (RT_SUCCESS(rc))
            {
                rc = VGDrvCommonIoCtl(uReq, &g_DevExt, pSession, pReqHdr, cbReq);
                if (RT_FAILURE(rc))
                    VGDrvCommonCloseSession(&g_DevExt, pSession);
            }
        }
        else
            rc = VERR_INVALID_HANDLE;
    }
    else
        rc = VERR_INVALID_POINTER;
    return rc;
}


void VGDrvNativeISRMousePollEvent(PVBOXGUESTDEVEXT pDevExt)
{
    NOREF(pDevExt);
}


/**
 * Callback for blah blah blah.
 */
static IOReturn vgdrvDarwinSleepHandler(void *pvTarget, void *pvRefCon, UInt32 uMessageType,
                                        IOService *pProvider, void *pvMsgArg, vm_size_t cbMsgArg)
{
    RT_NOREF(pvTarget, pProvider, pvMsgArg, cbMsgArg);
    LogFlow(("VBoxGuest: Got sleep/wake notice. Message type was %x\n", uMessageType));

    if (uMessageType == kIOMessageSystemWillSleep)
        RTPowerSignalEvent(RTPOWEREVENT_SUSPEND);
    else if (uMessageType == kIOMessageSystemHasPoweredOn)
        RTPowerSignalEvent(RTPOWEREVENT_RESUME);

    acknowledgeSleepWakeNotification(pvRefCon);

    return 0;
}


/**
 * Converts an IPRT error code to a darwin error code.
 *
 * @returns corresponding darwin error code.
 * @param   rc      IPRT status code.
 */
static int vgdrvDarwinErr2DarwinErr(int rc)
{
    switch (rc)
    {
        case VINF_SUCCESS:              return 0;
        case VERR_GENERAL_FAILURE:      return EACCES;
        case VERR_INVALID_PARAMETER:    return EINVAL;
        case VERR_INVALID_MAGIC:        return EILSEQ;
        case VERR_INVALID_HANDLE:       return ENXIO;
        case VERR_INVALID_POINTER:      return EFAULT;
        case VERR_LOCK_FAILED:          return ENOLCK;
        case VERR_ALREADY_LOADED:       return EEXIST;
        case VERR_PERMISSION_DENIED:    return EPERM;
        case VERR_VERSION_MISMATCH:     return ENOSYS;
    }

    return EPERM;
}


/*
 *
 * org_virtualbox_VBoxGuest
 *
 */


/**
 * Lazy initialization of the m_pWorkLoop member.
 *
 * @returns m_pWorkLoop.
 */
IOWorkLoop *org_virtualbox_VBoxGuest::getWorkLoop()
{
/** @todo r=bird: This is actually a classic RTOnce scenario, except it's
 *        tied to a org_virtualbox_VBoxGuest instance.  */
    /*
     * Handle the case when work loop was not created yet.
     */
    if (ASMAtomicCmpXchgU8(&g_fWorkLoopCreated, VBOXGUEST_OBJECT_INITIALIZING, VBOXGUEST_OBJECT_UNINITIALIZED))
    {
        m_pWorkLoop = IOWorkLoop::workLoop();
        if (m_pWorkLoop)
        {
            /* Notify the rest of threads about the fact that work
             * loop was successully allocated and can be safely used */
            Log(("VBoxGuest: created new work loop\n"));
            ASMAtomicWriteU8(&g_fWorkLoopCreated, VBOXGUEST_OBJECT_INITIALIZED);
        }
        else
        {
            /* Notify the rest of threads about the fact that there was
             * an error during allocation of a work loop */
            Log(("VBoxGuest: failed to create new work loop!\n"));
            ASMAtomicWriteU8(&g_fWorkLoopCreated, VBOXGUEST_OBJECT_UNINITIALIZED);
        }
    }
    /*
     * Handle the case when work loop is already create or
     * in the process of being.
     */
    else
    {
        uint8_t fWorkLoopCreated = ASMAtomicReadU8(&g_fWorkLoopCreated);
        while (fWorkLoopCreated == VBOXGUEST_OBJECT_INITIALIZING)
        {
            thread_block(0);
            fWorkLoopCreated = ASMAtomicReadU8(&g_fWorkLoopCreated);
        }
        if (fWorkLoopCreated != VBOXGUEST_OBJECT_INITIALIZED)
            Log(("VBoxGuest: No work loop!\n"));
    }

    return m_pWorkLoop;
}


/**
 * Perform pending wake ups in work loop context.
 */
static void vgdrvDarwinDeferredIrqHandler(OSObject *pOwner, IOInterruptEventSource *pSrc, int cInts)
{
    NOREF(pOwner); NOREF(pSrc); NOREF(cInts);

    VGDrvCommonWaitDoWakeUps(&g_DevExt);
}


/**
 * Callback triggered when interrupt occurs.
 */
static bool vgdrvDarwinDirectIrqHandler(OSObject *pOwner, IOFilterInterruptEventSource *pSrc)
{
    RT_NOREF(pOwner);
    if (!pSrc)
        return false;

    bool fTaken = VGDrvCommonISR(&g_DevExt);
    if (!fTaken) /** @todo r=bird: This looks bogus as we might actually be sharing interrupts with someone. */
        Log(("VGDrvCommonISR error\n"));

    return fTaken;
}


bool org_virtualbox_VBoxGuest::setupVmmDevInterrupts(IOService *pProvider)
{
    IOWorkLoop *pWorkLoop = getWorkLoop();
    if (!pWorkLoop)
        return false;

    m_pInterruptSrc = IOFilterInterruptEventSource::filterInterruptEventSource(this,
                                                                               &vgdrvDarwinDeferredIrqHandler,
                                                                               &vgdrvDarwinDirectIrqHandler,
                                                                               pProvider);
    IOReturn rc = pWorkLoop->addEventSource(m_pInterruptSrc);
    if (rc == kIOReturnSuccess)
    {
        m_pInterruptSrc->enable();
        return true;
    }

    m_pInterruptSrc->disable();
    m_pInterruptSrc->release();
    m_pInterruptSrc = NULL;
    return false;
}


bool org_virtualbox_VBoxGuest::disableVmmDevInterrupts(void)
{
    IOWorkLoop *pWorkLoop = (IOWorkLoop *)getWorkLoop();

    if (!pWorkLoop)
        return false;

    if (!m_pInterruptSrc)
        return false;

    m_pInterruptSrc->disable();
    pWorkLoop->removeEventSource(m_pInterruptSrc);
    m_pInterruptSrc->release();
    m_pInterruptSrc = NULL;

    return true;
}


bool org_virtualbox_VBoxGuest::isVmmDev(IOPCIDevice *pIOPCIDevice)
{
    UInt16 uVendorId, uDeviceId;

    if (!pIOPCIDevice)
        return false;

    uVendorId = m_pIOPCIDevice->configRead16(kIOPCIConfigVendorID);
    uDeviceId = m_pIOPCIDevice->configRead16(kIOPCIConfigDeviceID);

    if (uVendorId == VMMDEV_VENDORID && uDeviceId == VMMDEV_DEVICEID)
        return true;

    return true;
}


/**
 * Start this service.
 */
bool org_virtualbox_VBoxGuest::start(IOService *pProvider)
{
    /*
     * Low level initialization / device initialization should be performed only once.
     */
    if (!ASMAtomicCmpXchgBool(&g_fInstantiated, true, false))
        return false;

    if (!IOService::start(pProvider))
        return false;

    m_pIOPCIDevice = OSDynamicCast(IOPCIDevice, pProvider);
    if (m_pIOPCIDevice)
    {
        if (isVmmDev(m_pIOPCIDevice))
        {
            /* Enable memory response from VMM device */
            m_pIOPCIDevice->setMemoryEnable(true);
            m_pIOPCIDevice->setIOEnable(true);

            IOMemoryDescriptor *pMem = m_pIOPCIDevice->getDeviceMemoryWithIndex(0);
            if (pMem)
            {
                IOPhysicalAddress IOPortBasePhys = pMem->getPhysicalAddress();
                /* Check that returned value is from I/O port range (at least it is 16-bit lenght) */
                if((IOPortBasePhys >> 16) == 0)
                {

                    RTIOPORT IOPortBase = (RTIOPORT)IOPortBasePhys;
                    void    *pvMMIOBase = NULL;
                    uint32_t cbMMIO     = 0;
                    m_pMap = m_pIOPCIDevice->mapDeviceMemoryWithIndex(1);
                    if (m_pMap)
                    {
                        pvMMIOBase = (void *)m_pMap->getVirtualAddress();
                        cbMMIO     = m_pMap->getLength();
                    }

                    int rc = VGDrvCommonInitDevExt(&g_DevExt,
                                                   IOPortBase,
                                                   pvMMIOBase,
                                                   cbMMIO,
#if ARCH_BITS == 64
                                                   VBOXOSTYPE_MacOS_x64,
#else
                                                   VBOXOSTYPE_MacOS,
#endif
                                                   0);
                    if (RT_SUCCESS(rc))
                    {
                        rc = vgdrvDarwinCharDevInit();
                        if (rc == KMOD_RETURN_SUCCESS)
                        {
                            if (setupVmmDevInterrupts(pProvider))
                            {
                                /* register the service. */
                                registerService();
                                LogRel(("VBoxGuest: IOService started\n"));
                                return true;
                            }

                            LogRel(("VBoxGuest: Failed to set up interrupts\n"));
                            vgdrvDarwinCharDevRemove();
                        }
                        else
                            LogRel(("VBoxGuest: Failed to initialize character device (rc=%d).\n", rc));

                        VGDrvCommonDeleteDevExt(&g_DevExt);
                    }
                    else
                        LogRel(("VBoxGuest: Failed to initialize common code (rc=%d).\n", rc));

                    if (m_pMap)
                    {
                        m_pMap->release();
                        m_pMap = NULL;
                    }
                }
            }
            else
                LogRel(("VBoxGuest: The device missing is the I/O port range (#0).\n"));
        }
        else
            LogRel(("VBoxGuest: Not the VMMDev (%#x:%#x).\n",
                   m_pIOPCIDevice->configRead16(kIOPCIConfigVendorID), m_pIOPCIDevice->configRead16(kIOPCIConfigDeviceID)));
    }
    else
        LogRel(("VBoxGuest: Provider is not an instance of IOPCIDevice.\n"));

    ASMAtomicXchgBool(&g_fInstantiated, false);
    IOService::stop(pProvider);
    return false;
}


/**
 * Stop this service.
 */
void org_virtualbox_VBoxGuest::stop(IOService *pProvider)
{
    /* Do not use Log*() here (in IOService instance) because its instance
     * already terminated in BSD's module unload callback! */
    Log(("org_virtualbox_VBoxGuest::stop([%p], %p)\n", this, pProvider));

    AssertReturnVoid(ASMAtomicReadBool(&g_fInstantiated));

    /* Low level termination should be performed only once */
    if (!disableVmmDevInterrupts())
        printf("VBoxGuest: unable to unregister interrupt handler\n");

    vgdrvDarwinCharDevRemove();
    VGDrvCommonDeleteDevExt(&g_DevExt);

    if (m_pMap)
    {
        m_pMap->release();
        m_pMap = NULL;
    }

    IOService::stop(pProvider);

    ASMAtomicWriteBool(&g_fInstantiated, false);

    printf("VBoxGuest: IOService stopped\n");
}


/**
 * Termination request.
 *
 * @return  true if we're ok with shutting down now, false if we're not.
 * @param   fOptions        Flags.
 */
bool org_virtualbox_VBoxGuest::terminate(IOOptionBits fOptions)
{
    /* Do not use Log*() here (in IOService instance) because its instance
     * already terminated in BSD's module unload callback! */
#ifdef LOG_ENABLED
    printf("org_virtualbox_VBoxGuest::terminate: reference_count=%d g_cSessions=%d (fOptions=%#x)\n",
           KMOD_INFO_NAME.reference_count, ASMAtomicUoReadS32(&g_cSessions), fOptions);
#endif

    bool fRc;
    if (    KMOD_INFO_NAME.reference_count != 0
        ||  ASMAtomicUoReadS32(&g_cSessions))
        fRc = false;
    else
        fRc = IOService::terminate(fOptions);

#ifdef LOG_ENABLED
    printf("org_virtualbox_SupDrv::terminate: returns %d\n", fRc);
#endif
    return fRc;
}


/*
 *
 * org_virtualbox_VBoxGuestClient
 *
 */


/**
 * Initializer called when the client opens the service.
 */
bool org_virtualbox_VBoxGuestClient::initWithTask(task_t OwningTask, void *pvSecurityId, UInt32 u32Type)
{
    LogFlow(("org_virtualbox_VBoxGuestClient::initWithTask([%p], %#x, %p, %#x) (cur pid=%d proc=%p)\n",
             this, OwningTask, pvSecurityId, u32Type, RTProcSelf(), RTR0ProcHandleSelf()));
    AssertMsg((RTR0PROCESS)OwningTask == RTR0ProcHandleSelf(), ("%p %p\n", OwningTask, RTR0ProcHandleSelf()));

    if (!OwningTask)
        return false;

    if (u32Type != VBOXGUEST_DARWIN_IOSERVICE_COOKIE)
    {
        Log(("org_virtualbox_VBoxGuestClient::initWithTask: Bad cookie %#x\n", u32Type));
        return false;
    }

    if (IOUserClient::initWithTask(OwningTask, pvSecurityId , u32Type))
    {
        /*
         * In theory we have to call task_reference() to make sure that the task is
         * valid during the lifetime of this object. The pointer is only used to check
         * for the context this object is called in though and never dereferenced
         * or passed to anything which might, so we just skip this step.
         */
        m_Task = OwningTask;
        m_pSession = NULL;
        m_pProvider = NULL;
        return true;
    }
    return false;
}


/**
 * Start the client service.
 */
bool org_virtualbox_VBoxGuestClient::start(IOService *pProvider)
{
    LogFlow(("org_virtualbox_VBoxGuestClient::start([%p], %p) (cur pid=%d proc=%p)\n",
             this, pProvider, RTProcSelf(), RTR0ProcHandleSelf() ));
    AssertMsgReturn((RTR0PROCESS)m_Task == RTR0ProcHandleSelf(),
                    ("%p %p\n", m_Task, RTR0ProcHandleSelf()),
                    false);

    if (IOUserClient::start(pProvider))
    {
        m_pProvider = OSDynamicCast(org_virtualbox_VBoxGuest, pProvider);
        if (m_pProvider)
        {
            Assert(!m_pSession);

            /*
             * Create a new session.
             */
            int rc = VGDrvCommonCreateUserSession(&g_DevExt, &m_pSession);
            if (RT_SUCCESS(rc))
            {
                m_pSession->fOpened = false;
                /* The fUnrestricted field is set on open. */

                /*
                 * Insert it into the hash table, checking that there isn't
                 * already one for this process first. (One session per proc!)
                 */
                unsigned iHash = SESSION_HASH(m_pSession->Process);
                RTSpinlockAcquire(g_Spinlock);

                PVBOXGUESTSESSION pCur = g_apSessionHashTab[iHash];
                if (pCur && pCur->Process != m_pSession->Process)
                {
                    do pCur = pCur->pNextHash;
                    while (pCur && pCur->Process != m_pSession->Process);
                }
                if (!pCur)
                {
                    m_pSession->pNextHash = g_apSessionHashTab[iHash];
                    g_apSessionHashTab[iHash] = m_pSession;
                    m_pSession->pvVBoxGuestClient = this;
                    ASMAtomicIncS32(&g_cSessions);
                    rc = VINF_SUCCESS;
                }
                else
                    rc = VERR_ALREADY_LOADED;

                RTSpinlockRelease(g_Spinlock);
                if (RT_SUCCESS(rc))
                {
                    Log(("org_virtualbox_VBoxGuestClient::start: created session %p for pid %d\n", m_pSession, (int)RTProcSelf()));
                    return true;
                }

                LogFlow(("org_virtualbox_VBoxGuestClient::start: already got a session for this process (%p)\n", pCur));
                VGDrvCommonCloseSession(&g_DevExt, m_pSession);
            }

            m_pSession = NULL;
            LogFlow(("org_virtualbox_VBoxGuestClient::start: rc=%Rrc from supdrvCreateSession\n", rc));
        }
        else
            LogFlow(("org_virtualbox_VBoxGuestClient::start: %p isn't org_virtualbox_VBoxGuest\n", pProvider));
    }
    return false;
}


/**
 * Common worker for clientClose and VBoxDrvDarwinClose.
 */
/* static */ void org_virtualbox_VBoxGuestClient::sessionClose(RTPROCESS Process)
{
    /*
     * Find the session and remove it from the hash table.
     *
     * Note! Only one session per process. (Both start() and
     * vgdrvDarwinOpen makes sure this is so.)
     */
    const unsigned  iHash = SESSION_HASH(Process);
    RTSpinlockAcquire(g_Spinlock);
    PVBOXGUESTSESSION  pSession = g_apSessionHashTab[iHash];
    if (pSession)
    {
        if (pSession->Process == Process)
        {
            g_apSessionHashTab[iHash] = pSession->pNextHash;
            pSession->pNextHash = NULL;
            ASMAtomicDecS32(&g_cSessions);
        }
        else
        {
            PVBOXGUESTSESSION pPrev = pSession;
            pSession = pSession->pNextHash;
            while (pSession)
            {
                if (pSession->Process == Process)
                {
                    pPrev->pNextHash = pSession->pNextHash;
                    pSession->pNextHash = NULL;
                    ASMAtomicDecS32(&g_cSessions);
                    break;
                }

                /* next */
                pPrev = pSession;
                pSession = pSession->pNextHash;
            }
        }
    }
    RTSpinlockRelease(g_Spinlock);
    if (!pSession)
    {
        Log(("VBoxGuestClient::sessionClose: pSession == NULL, pid=%d; freed already?\n", (int)Process));
        return;
    }

    /*
     * Remove it from the client object.
     */
    org_virtualbox_VBoxGuestClient *pThis = (org_virtualbox_VBoxGuestClient *)pSession->pvVBoxGuestClient;
    pSession->pvVBoxGuestClient = NULL;
    if (pThis)
    {
        Assert(pThis->m_pSession == pSession);
        pThis->m_pSession = NULL;
    }

    /*
     * Close the session.
     */
    VGDrvCommonCloseSession(&g_DevExt, pSession);
}


/**
 * Client exits normally.
 */
IOReturn org_virtualbox_VBoxGuestClient::clientClose(void)
{
    LogFlow(("org_virtualbox_VBoxGuestClient::clientClose([%p]) (cur pid=%d proc=%p)\n", this, RTProcSelf(), RTR0ProcHandleSelf()));
    AssertMsg((RTR0PROCESS)m_Task == RTR0ProcHandleSelf(), ("%p %p\n", m_Task, RTR0ProcHandleSelf()));

    /*
     * Clean up the session if it's still around.
     *
     * We cannot rely 100% on close, and in the case of a dead client
     * we'll end up hanging inside vm_map_remove() if we postpone it.
     */
    if (m_pSession)
    {
        sessionClose(RTProcSelf());
        Assert(!m_pSession);
    }

    m_pProvider = NULL;
    terminate();

    return kIOReturnSuccess;
}

