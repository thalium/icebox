/* $Id: SUPLib-darwin.cpp $ */
/** @file
 * VirtualBox Support Library - Darwin specific parts.
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
#define LOG_GROUP LOG_GROUP_SUP
#ifdef IN_SUP_HARDENED_R3
# undef DEBUG /* Warning: disables RT_STRICT */
# ifndef LOG_DISABLED
#  define LOG_DISABLED
# endif
# define RTLOG_REL_DISABLED
# include <iprt/log.h>
#endif

#include <VBox/types.h>
#include <VBox/sup.h>
#include <VBox/param.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/path.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/string.h>
#include "../SUPLibInternal.h"
#include "../SUPDrvIOC.h"

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <mach/mach_port.h>
#include <IOKit/IOKitLib.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** System device name. */
#define DEVICE_NAME_SYS "/dev/vboxdrv"
/** User device name. */
#define DEVICE_NAME_USR "/dev/vboxdrvu"
/** The IOClass key of the service (see SUPDrv-darwin.cpp / Info.plist). */
#define IOCLASS_NAME    "org_virtualbox_SupDrv"



/**
 * Opens the BSD device node.
 *
 * @returns VBox status code.
 */
static int suplibDarwinOpenDevice(PSUPLIBDATA pThis, bool fUnrestricted)
{
    /*
     * Open the BSD device.
     * This will connect to the session created when the SupDrvClient was
     * started, so it has to be done after opening the service (IOC v9.1+).
     */
    int hDevice = open(fUnrestricted ? DEVICE_NAME_SYS : DEVICE_NAME_USR, O_RDWR, 0);
    if (hDevice < 0)
    {
        int rc;
        switch (errno)
        {
            case ENODEV:    rc = VERR_VM_DRIVER_LOAD_ERROR; break;
            case EPERM:
            case EACCES:    rc = VERR_VM_DRIVER_NOT_ACCESSIBLE; break;
            case ENOENT:    rc = VERR_VM_DRIVER_NOT_INSTALLED; break;
            default:        rc = VERR_VM_DRIVER_OPEN_ERROR; break;
        }
        LogRel(("SUP: Failed to open \"%s\", errno=%d, rc=%Rrc\n", fUnrestricted ? DEVICE_NAME_SYS : DEVICE_NAME_USR, errno, rc));
        return rc;
    }

    /*
     * Mark the file handle close on exec.
     */
    if (fcntl(hDevice, F_SETFD, FD_CLOEXEC) != 0)
    {
#ifdef IN_SUP_HARDENED_R3
        int rc = VERR_INTERNAL_ERROR;
#else
        int err = errno;
        int rc = RTErrConvertFromErrno(err);
        LogRel(("suplibOSInit: setting FD_CLOEXEC failed, errno=%d (%Rrc)\n", err, rc));
#endif
        close(hDevice);
        return rc;
    }

    pThis->hDevice       = hDevice;
    pThis->fUnrestricted = fUnrestricted;
    return VINF_SUCCESS;
}


/**
 * Opens the IOKit service, instantiating org_virtualbox_SupDrvClient.
 *
 * @returns VBox status code.
 */
static int suplibDarwinOpenService(PSUPLIBDATA pThis)
{
    /*
     * Open the IOKit client first - The first step is finding the service.
     */
    mach_port_t MasterPort;
    kern_return_t kr = IOMasterPort(MACH_PORT_NULL, &MasterPort);
    if (kr != kIOReturnSuccess)
    {
        LogRel(("IOMasterPort -> %d\n", kr));
        return VERR_GENERAL_FAILURE;
    }

    CFDictionaryRef ClassToMatch = IOServiceMatching(IOCLASS_NAME);
    if (!ClassToMatch)
    {
        LogRel(("IOServiceMatching(\"%s\") failed.\n", IOCLASS_NAME));
        return VERR_GENERAL_FAILURE;
    }

    /* Create an io_iterator_t for all instances of our drivers class that exist in the IORegistry. */
    io_iterator_t Iterator;
    kr = IOServiceGetMatchingServices(MasterPort, ClassToMatch, &Iterator);
    if (kr != kIOReturnSuccess)
    {
        LogRel(("IOServiceGetMatchingServices returned %d\n", kr));
        return VERR_GENERAL_FAILURE;
    }

    /* Get the first item in the iterator and release it. */
    io_service_t ServiceObject = IOIteratorNext(Iterator);
    IOObjectRelease(Iterator);
    if (!ServiceObject)
    {
        LogRel(("SUP: Couldn't find any matches. The kernel module is probably not loaded.\n"));
        return VERR_VM_DRIVER_NOT_INSTALLED;
    }

    /*
     * Open the service.
     *
     * This will cause the user client class in SUPDrv-darwin.cpp to be
     * instantiated and create a session for this process.
     */
    io_connect_t Connection = 0;
    kr = IOServiceOpen(ServiceObject, mach_task_self(), SUP_DARWIN_IOSERVICE_COOKIE, &Connection);
    IOObjectRelease(ServiceObject);
    if (kr != kIOReturnSuccess)
    {
        LogRel(("SUP: IOServiceOpen returned %d. Driver open failed.\n", kr));
        pThis->uConnection = 0;
        return VERR_VM_DRIVER_OPEN_ERROR;
    }

    AssertCompile(sizeof(pThis->uConnection) >= sizeof(Connection));
    pThis->uConnection = Connection;
    return VINF_SUCCESS;
}


int suplibOsInit(PSUPLIBDATA pThis, bool fPreInited, bool fUnrestricted, SUPINITOP *penmWhat, PRTERRINFO pErrInfo)
{
    RT_NOREF(penmWhat, pErrInfo);

    /*
     * Nothing to do if pre-inited.
     */
    if (fPreInited)
        return VINF_SUCCESS;

    /*
     * Do the job.
     */
    Assert(pThis->hDevice == (intptr_t)NIL_RTFILE);
    int rc = suplibDarwinOpenService(pThis);
    if (RT_SUCCESS(rc))
    {
        rc = suplibDarwinOpenDevice(pThis, fUnrestricted);
        if (RT_FAILURE(rc))
        {
            kern_return_t kr = IOServiceClose((io_connect_t)pThis->uConnection);
            if (kr != kIOReturnSuccess)
            {
                LogRel(("Warning: IOServiceClose(%RCv) returned %d\n", pThis->uConnection, kr));
                AssertFailed();
            }
            pThis->uConnection = 0;
        }
    }

    return rc;
}


int suplibOsTerm(PSUPLIBDATA pThis)
{
    /*
     * Close the connection to the IOService.
     * This will cause the SUPDRVSESSION to be closed (starting IOC 9.1).
     */
    if (pThis->uConnection)
    {
        kern_return_t kr = IOServiceClose((io_connect_t)pThis->uConnection);
        if (kr != kIOReturnSuccess)
        {
            LogRel(("Warning: IOServiceClose(%RCv) returned %d\n", pThis->uConnection, kr));
            AssertFailed();
        }
        pThis->uConnection = 0;
    }

    /*
     * Check if we're inited at all.
     */
    if (pThis->hDevice != (intptr_t)NIL_RTFILE)
    {
        if (close(pThis->hDevice))
            AssertFailed();
        pThis->hDevice = (intptr_t)NIL_RTFILE;
    }

    return VINF_SUCCESS;
}


#ifndef IN_SUP_HARDENED_R3

int suplibOsInstall(void)
{
    return VERR_NOT_IMPLEMENTED;
}


int suplibOsUninstall(void)
{
    return VERR_NOT_IMPLEMENTED;
}


int suplibOsIOCtl(PSUPLIBDATA pThis, uintptr_t uFunction, void *pvReq, size_t cbReq)
{
    RT_NOREF(cbReq);
    if (RT_LIKELY(ioctl(pThis->hDevice, uFunction, pvReq) >= 0))
        return VINF_SUCCESS;
    return RTErrConvertFromErrno(errno);
}


int suplibOsIOCtlFast(PSUPLIBDATA pThis, uintptr_t uFunction, uintptr_t idCpu)
{
    int rc = ioctl(pThis->hDevice, uFunction, idCpu);
    if (rc == -1)
        rc = errno;
    return rc;
}


int suplibOsPageAlloc(PSUPLIBDATA pThis, size_t cPages, void **ppvPages)
{
    NOREF(pThis);
    *ppvPages = valloc(cPages << PAGE_SHIFT);
    if (*ppvPages)
    {
        memset(*ppvPages, 0, cPages << PAGE_SHIFT);
        return VINF_SUCCESS;
    }
    return RTErrConvertFromErrno(errno);
}


int suplibOsPageFree(PSUPLIBDATA pThis, void *pvPages, size_t /* cPages */)
{
    NOREF(pThis);
    free(pvPages);
    return VINF_SUCCESS;
}

#endif /* !IN_SUP_HARDENED_R3 */

