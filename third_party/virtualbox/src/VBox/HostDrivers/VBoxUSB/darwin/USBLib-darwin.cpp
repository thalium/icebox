/** $Id: USBLib-darwin.cpp $ */
/** @file
 * USBLib - Library for wrapping up the VBoxUSB functionality, Darwin flavor.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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
#include <VBox/usblib.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include "VBoxUSBInterface.h"

#include <iprt/assert.h>
#include <iprt/asm.h>

#include <mach/mach_port.h>
#include <IOKit/IOKitLib.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The IOClass key of the service (see VBoxUSB.cpp / Info.plist). */
#define IOCLASS_NAME    "org_virtualbox_VBoxUSB"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Reference counter. */
static uint32_t volatile    g_cUsers = 0;
/** The IOMasterPort. */
static mach_port_t          g_MasterPort = 0;
/** The current service connection. */
static io_connect_t         g_Connection = 0;



USBLIB_DECL(int) USBLibInit(void)
{
    /*
     * Already open?
     * This isn't properly serialized, but we'll be fine with the current usage.
     */
    if (g_cUsers)
    {
        ASMAtomicIncU32(&g_cUsers);
        return VINF_SUCCESS;
    }

    /*
     * Finding the VBoxUSB service.
     */
    mach_port_t MasterPort;
    kern_return_t kr = IOMasterPort(MACH_PORT_NULL, &MasterPort);
    if (kr != kIOReturnSuccess)
    {
        LogRel(("USBLib: IOMasterPort -> %#x\n", kr));
        return RTErrConvertFromDarwinKern(kr);
    }

    CFDictionaryRef ClassToMatch = IOServiceMatching(IOCLASS_NAME);
    if (!ClassToMatch)
    {
        LogRel(("USBLib: IOServiceMatching(\"%s\") failed.\n", IOCLASS_NAME));
        return VERR_GENERAL_FAILURE;
    }

    /* Create an io_iterator_t for all instances of our drivers class that exist in the IORegistry. */
    io_iterator_t Iterator;
    kr = IOServiceGetMatchingServices(g_MasterPort, ClassToMatch, &Iterator);
    if (kr != kIOReturnSuccess)
    {
        LogRel(("USBLib: IOServiceGetMatchingServices returned %#x\n", kr));
        return RTErrConvertFromDarwinKern(kr);
    }

    /* Get the first item in the iterator and release it. */
    io_service_t ServiceObject = IOIteratorNext(Iterator);
    IOObjectRelease(Iterator);
    if (!ServiceObject)
    {
        LogRel(("USBLib: Couldn't find any matches.\n"));
        return VERR_GENERAL_FAILURE;
    }

    /*
     * Open the service.
     * This will cause the user client class in VBoxUSB.cpp to be instantiated.
     */
    kr = IOServiceOpen(ServiceObject, mach_task_self(), VBOXUSB_DARWIN_IOSERVICE_COOKIE, &g_Connection);
    IOObjectRelease(ServiceObject);
    if (kr != kIOReturnSuccess)
    {
        LogRel(("USBLib: IOServiceOpen returned %#x\n", kr));
        return RTErrConvertFromDarwinKern(kr);
    }

    ASMAtomicIncU32(&g_cUsers);
    return VINF_SUCCESS;
}


USBLIB_DECL(int) USBLibTerm(void)
{
    if (!g_cUsers)
        return VERR_WRONG_ORDER;
    if (ASMAtomicDecU32(&g_cUsers) != 0)
        return VINF_SUCCESS;

    /*
     * We're the last guy, close down the connection.
     */
    kern_return_t kr = IOServiceClose(g_Connection);
    if (kr != kIOReturnSuccess)
    {
        LogRel(("USBLib: Warning: IOServiceClose(%p) returned %#x\n", g_Connection, kr));
        AssertMsgFailed(("%#x\n", kr));
    }
    g_Connection = 0;

    return VINF_SUCCESS;
}


USBLIB_DECL(void *) USBLibAddFilter(PCUSBFILTER pFilter)
{
    VBOXUSBADDFILTEROUT Out = { 0, VERR_WRONG_ORDER };
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
    IOByteCount cbOut = sizeof(Out);
    kern_return_t kr = IOConnectMethodStructureIStructureO(g_Connection,
                                                           VBOXUSBMETHOD_ADD_FILTER,
                                                           sizeof(*pFilter),
                                                           &cbOut,
                                                           (void *)pFilter,
                                                           &Out);
#else  /* 10.5 and later */
    size_t cbOut = sizeof(Out);
    kern_return_t kr = IOConnectCallStructMethod(g_Connection,
                                                 VBOXUSBMETHOD_ADD_FILTER,
                                                 (void *)pFilter, sizeof(*pFilter),
                                                 &Out, &cbOut);
#endif /* 10.5 and later */
    if (    kr == kIOReturnSuccess
        &&  RT_SUCCESS(Out.rc))
    {
        Assert(cbOut == sizeof(Out));
        Assert((void *)Out.uId != NULL);
        return (void *)Out.uId;
    }

    AssertMsgFailed(("kr=%#x Out.rc=%Rrc\n", kr, Out.rc));
    return NULL;
}


USBLIB_DECL(void) USBLibRemoveFilter(void *pvId)
{
    int rc = VERR_WRONG_ORDER;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
    IOByteCount cbOut = sizeof(rc);
    kern_return_t kr = IOConnectMethodStructureIStructureO(g_Connection,
                                                           VBOXUSBMETHOD_REMOVE_FILTER,
                                                           sizeof(pvId),
                                                           &cbOut,
                                                           &pvId,
                                                           &rc);
#else  /* 10.5 and later */
    size_t cbOut = sizeof(rc);
    kern_return_t kr = IOConnectCallStructMethod(g_Connection,
                                                 VBOXUSBMETHOD_REMOVE_FILTER,
                                                 (void *)&pvId, sizeof(pvId),
                                                 &rc, &cbOut);
#endif /* 10.5 and later */
    AssertMsg(kr == kIOReturnSuccess && RT_SUCCESS(rc), ("kr=%#x rc=%Rrc\n", kr, rc));
    NOREF(kr);
}

