/* $Id: RTSystemQueryDmiString-darwin.cpp $ */
/** @file
 * IPRT - RTSystemQueryDmiString, darwin ring-3.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
#include <iprt/system.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/string.h>

#include <mach/mach_port.h>
#include <IOKit/IOKitLib.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define IOCLASS_PLATFORMEXPERTDEVICE         "IOPlatformExpertDevice"
#define PROP_PRODUCT_NAME                    "product-name"
#define PROP_PRODUCT_VERSION                 "version"
#define PROP_PRODUCT_SERIAL                  "IOPlatformSerialNumber"
#define PROP_PRODUCT_UUID                    "IOPlatformUUID"
#define PROP_MANUFACTURER                    "manufacturer"


RTDECL(int) RTSystemQueryDmiString(RTSYSDMISTR enmString, char *pszBuf, size_t cbBuf)
{
    AssertPtrReturn(pszBuf, VERR_INVALID_POINTER);
    AssertReturn(cbBuf > 0, VERR_INVALID_PARAMETER);
    *pszBuf = '\0';
    AssertReturn(enmString > RTSYSDMISTR_INVALID && enmString < RTSYSDMISTR_END, VERR_INVALID_PARAMETER);

    CFStringRef PropStringRef = NULL;
    switch (enmString)
    {
        case RTSYSDMISTR_PRODUCT_NAME:    PropStringRef = CFSTR(PROP_PRODUCT_NAME);    break;
        case RTSYSDMISTR_PRODUCT_VERSION: PropStringRef = CFSTR(PROP_PRODUCT_VERSION); break;
        case RTSYSDMISTR_PRODUCT_SERIAL:  PropStringRef = CFSTR(PROP_PRODUCT_SERIAL);  break;
        case RTSYSDMISTR_PRODUCT_UUID:    PropStringRef = CFSTR(PROP_PRODUCT_UUID);    break;
        case RTSYSDMISTR_MANUFACTURER:    PropStringRef = CFSTR(PROP_MANUFACTURER);    break;
        default:
            return VERR_NOT_SUPPORTED;
    }

    mach_port_t MasterPort;
    kern_return_t kr = IOMasterPort(MACH_PORT_NULL, &MasterPort);
    if (kr != kIOReturnSuccess)
    {
        if (kr == KERN_NO_ACCESS)
            return VERR_ACCESS_DENIED;
        return RTErrConvertFromDarwinIO(kr);
    }

    CFDictionaryRef ClassToMatch = IOServiceMatching(IOCLASS_PLATFORMEXPERTDEVICE);
    if (!ClassToMatch)
        return VERR_NOT_SUPPORTED;

    /* IOServiceGetMatchingServices will always consume ClassToMatch. */
    io_iterator_t Iterator;
    kr = IOServiceGetMatchingServices(MasterPort, ClassToMatch, &Iterator);
    if (kr != kIOReturnSuccess)
        return RTErrConvertFromDarwinIO(kr);

    int rc = VERR_NOT_SUPPORTED;
    io_service_t ServiceObject;
    while ((ServiceObject = IOIteratorNext(Iterator)))
    {
        if (   enmString == RTSYSDMISTR_PRODUCT_NAME
            || enmString == RTSYSDMISTR_PRODUCT_VERSION
            || enmString == RTSYSDMISTR_MANUFACTURER
           )
        {
            CFDataRef DataRef = (CFDataRef)IORegistryEntryCreateCFProperty(ServiceObject, PropStringRef,
                                                                           kCFAllocatorDefault, kNilOptions);
            if (DataRef)
            {
                size_t         cbData  = CFDataGetLength(DataRef);
                const char    *pchData = (const char *)CFDataGetBytePtr(DataRef);
                rc = RTStrCopyEx(pszBuf, cbBuf, pchData, cbData);
                CFRelease(DataRef);
                break;
            }
        }
        else
        {
            CFStringRef StringRef = (CFStringRef)IORegistryEntryCreateCFProperty(ServiceObject, PropStringRef,
                                                                                 kCFAllocatorDefault, kNilOptions);
            if (StringRef)
            {
                Boolean fRc = CFStringGetCString(StringRef, pszBuf, cbBuf, kCFStringEncodingUTF8);
                if (fRc)
                    rc = VINF_SUCCESS;
                else
                {
                    CFIndex cwc    = CFStringGetLength(StringRef);
                    size_t  cbTmp  = cwc + 1;
                    char   *pszTmp = (char *)RTMemTmpAlloc(cbTmp);
                    int     cTries = 1;
                    while (   pszTmp
                           && (fRc = CFStringGetCString(StringRef, pszTmp, cbTmp, kCFStringEncodingUTF8)) == FALSE
                           && cTries++ < 4)
                    {
                        RTMemTmpFree(pszTmp);
                        cbTmp *= 2;
                        pszTmp = (char *)RTMemTmpAlloc(cbTmp);
                    }
                    if (fRc)
                        rc = RTStrCopy(pszBuf, cbBuf, pszTmp);
                    else if (!pszTmp)
                        rc = VERR_NO_TMP_MEMORY;
                    else
                        rc = VERR_ACCESS_DENIED;
                    RTMemFree(pszTmp);
                }
                CFRelease(StringRef);
                break;
            }
        }
    }

    IOObjectRelease(ServiceObject);
    IOObjectRelease(Iterator);
    return rc;
}

