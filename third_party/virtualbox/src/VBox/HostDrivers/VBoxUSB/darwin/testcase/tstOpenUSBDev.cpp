/* $Id: tstOpenUSBDev.cpp $ */
/** @file
 * Testcase that attempts to locate and open the specfied device.
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
#include <mach/mach.h>
#include <Carbon/Carbon.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h>
#include <IOKit/scsi/SCSITaskLib.h>
#include <mach/mach_error.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>

#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/process.h>
#include <iprt/assert.h>
#include <iprt/thread.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/stream.h>

/**
 * Gets an unsigned 32-bit integer value.
 *
 * @returns Success indicator (true/false).
 * @param   DictRef     The dictionary.
 * @param   KeyStrRef   The key name.
 * @param   pu32        Where to store the key value.
 */
static bool tstDictGetU32(CFMutableDictionaryRef DictRef, CFStringRef KeyStrRef, uint32_t *pu32)
{
    CFTypeRef ValRef = CFDictionaryGetValue(DictRef, KeyStrRef);
    if (ValRef)
    {
        if (CFNumberGetValue((CFNumberRef)ValRef, kCFNumberSInt32Type, pu32))
            return true;
    }
    *pu32 = 0;
    return false;
}


/**
 * Gets an unsigned 64-bit integer value.
 *
 * @returns Success indicator (true/false).
 * @param   DictRef     The dictionary.
 * @param   KeyStrRef   The key name.
 * @param   pu64        Where to store the key value.
 */
static bool tstDictGetU64(CFMutableDictionaryRef DictRef, CFStringRef KeyStrRef, uint64_t *pu64)
{
    CFTypeRef ValRef = CFDictionaryGetValue(DictRef, KeyStrRef);
    if (ValRef)
    {
        if (CFNumberGetValue((CFNumberRef)ValRef, kCFNumberSInt64Type, pu64))
            return true;
    }
    *pu64 = 0;
    return false;
}


static int tstDoWork(io_object_t USBDevice, const char *argv0)
{
    /*
     * Create a plugin interface for the device and query its IOUSBDeviceInterface.
     */
    int vrc = VINF_SUCCESS;
    SInt32 Score = 0;
    IOCFPlugInInterface **ppPlugInInterface = NULL;
    IOReturn irc = IOCreatePlugInInterfaceForService(USBDevice, kIOUSBDeviceUserClientTypeID,
                                                     kIOCFPlugInInterfaceID, &ppPlugInInterface, &Score);
    if (irc == kIOReturnSuccess)
    {
        IOUSBDeviceInterface245 **ppDevI = NULL;
        HRESULT hrc = (*ppPlugInInterface)->QueryInterface(ppPlugInInterface,
                                                           CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID245),
                                                           (LPVOID *)&ppDevI);
        irc = IODestroyPlugInInterface(ppPlugInInterface); Assert(irc == kIOReturnSuccess);
        ppPlugInInterface = NULL;
        if (hrc == S_OK)
        {
            /*
             * Try open the device for exclusive access.
             */
            irc = (*ppDevI)->USBDeviceOpenSeize(ppDevI);
            if (irc == kIOReturnExclusiveAccess)
            {
                RTThreadSleep(20);
                irc = (*ppDevI)->USBDeviceOpenSeize(ppDevI);
            }
            if (irc == kIOReturnSuccess)
            {
#if 0
                /*
                 * Re-enumerate the device and bail out.
                 */
                irc = (*ppDevI)->USBDeviceReEnumerate(ppDevI, 0);
                if (irc != kIOReturnSuccess)
                {
                    vrc = RTErrConvertFromDarwinIO(irc);
                    RTPrintf("%s: Failed to re-enumerate the device, irc=%#x (vrc=%Rrc).\n", argv0, irc, vrc);
                }
#endif

                (*ppDevI)->USBDeviceClose(ppDevI);
            }
            else if (irc == kIOReturnExclusiveAccess)
            {
                vrc = VERR_SHARING_VIOLATION;
                RTPrintf("%s: The device is being used by another process (irc=kIOReturnExclusiveAccess)\n", argv0);
            }
            else
            {
                vrc = VERR_OPEN_FAILED;
                RTPrintf("%s: Failed to open the device, irc=%#x (vrc=%Rrc).\n", argv0, irc, vrc);
            }
        }
        else
        {
            vrc = VERR_OPEN_FAILED;
            RTPrintf("%s: Failed to create plugin interface for the device, hrc=%#x (vrc=%Rrc).\n", argv0, hrc, vrc);
        }

        (*ppDevI)->Release(ppDevI);
    }
    else
    {
        vrc = RTErrConvertFromDarwinIO(irc);
        RTPrintf("%s: Failed to open the device, plug-in creation failed with irc=%#x (vrc=%Rrc).\n", argv0, irc, vrc);
    }

    return vrc;
}


static int tstSyntax(const char *argv0)
{
    RTPrintf("syntax: %s [criteria]\n"
             "\n"
             "Criteria:\n"
             "  -l <location>\n"
             "  -s <session>\n"
             , argv0);
    return 1;
}


int main(int argc, char **argv)
{
    RTR3InitExe(argc, &argv, 0);

    /*
     * Show help if not arguments.
     */
    if (argc <= 1)
        return tstSyntax(argv[0]);

    /*
     * Parse arguments.
     */
    static const RTGETOPTDEF g_aOptions[] =
    {
        { "--location", 'l', RTGETOPT_REQ_UINT32 },
        { "--session",  's', RTGETOPT_REQ_UINT64 },
    };

    kern_return_t krc;
    uint64_t u64SessionId = 0;
    uint32_t u32LocationId = 0;

    int ch;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, argc, argv, g_aOptions, RT_ELEMENTS(g_aOptions), 1, 0 /* fFlags */);
    while ((ch = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (ch)
        {
            case 'l':
                u32LocationId = ValueUnion.u32;
                break;
            case 's':
                u64SessionId = ValueUnion.u64;
                break;
            case 'h':
                return tstSyntax(argv[0]);
            case 'V':
                RTPrintf("$Revision: 118839 $\n");
                return 0;

            default:
                return RTGetOptPrintError(ch, &ValueUnion);
        }
    }

    /*
     * Open the master port.
     */
    mach_port_t MasterPort = MACH_PORT_NULL;
    krc = IOMasterPort(MACH_PORT_NULL, &MasterPort);
    if (krc != KERN_SUCCESS)
    {
        RTPrintf("%s: IOMasterPort -> %x\n", argv[0], krc);
        return 1;
    }

    /*
     * Iterate the USB devices and find all that matches.
     */
    CFMutableDictionaryRef RefMatchingDict = IOServiceMatching(kIOUSBDeviceClassName);
    if (!RefMatchingDict)
    {
        RTPrintf("%s: IOServiceMatching failed\n", argv[0]);
        return 1;
    }

    io_iterator_t USBDevices = IO_OBJECT_NULL;
    IOReturn irc = IOServiceGetMatchingServices(MasterPort, RefMatchingDict, &USBDevices);
    if (irc != kIOReturnSuccess)
    {
        RTPrintf("%s: IOServiceGetMatchingServices -> %#x\n", argv[0], irc);
        return 1;
    }
    RefMatchingDict = NULL; /* the reference is consumed by IOServiceGetMatchingServices. */

    unsigned cDevices = 0;
    unsigned cMatches = 0;
    io_object_t USBDevice;
    while ((USBDevice = IOIteratorNext(USBDevices)))
    {
        cDevices++;
        CFMutableDictionaryRef PropsRef = 0;
        krc = IORegistryEntryCreateCFProperties(USBDevice, &PropsRef, kCFAllocatorDefault, kNilOptions);
        if (krc == KERN_SUCCESS)
        {
            uint64_t u64CurSessionId;
            uint32_t u32CurLocationId;
            if (    (    !u64SessionId
                     || (   tstDictGetU64(PropsRef, CFSTR("sessionID"), &u64CurSessionId)
                         && u64CurSessionId == u64SessionId))
                &&  (   !u32LocationId
                     || (   tstDictGetU32(PropsRef, CFSTR(kUSBDevicePropertyLocationID), &u32CurLocationId)
                         && u32CurLocationId == u32LocationId))
                )
            {
                cMatches++;
                CFRelease(PropsRef);
                tstDoWork(USBDevice, argv[0]);
            }
            else
                CFRelease(PropsRef);
        }
        IOObjectRelease(USBDevice);
    }
    IOObjectRelease(USBDevices);

    /*
     * Bitch if we didn't find anything matching the criteria.
     */
    if (!cMatches)
        RTPrintf("%s: No matching devices found from a total of %d.\n", argv[0], cDevices);
    return !cMatches;
}

