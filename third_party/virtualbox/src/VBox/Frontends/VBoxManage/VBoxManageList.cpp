/* $Id: VBoxManageList.cpp $ */
/** @file
 * VBoxManage - The 'list' command.
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
 */

#ifndef VBOX_ONLY_DOCS


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/Guid.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>

#include <VBox/com/VirtualBox.h>

#include <VBox/log.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/time.h>
#include <iprt/getopt.h>
#include <iprt/ctype.h>

#include <vector>
#include <algorithm>

#include "VBoxManage.h"
using namespace com;

#ifdef VBOX_WITH_HOSTNETIF_API
static const char *getHostIfMediumTypeText(HostNetworkInterfaceMediumType_T enmType)
{
    switch (enmType)
    {
        case HostNetworkInterfaceMediumType_Ethernet: return "Ethernet";
        case HostNetworkInterfaceMediumType_PPP: return "PPP";
        case HostNetworkInterfaceMediumType_SLIP: return "SLIP";
        case HostNetworkInterfaceMediumType_Unknown: return "Unknown";
    }
    return "unknown";
}

static const char *getHostIfStatusText(HostNetworkInterfaceStatus_T enmStatus)
{
    switch (enmStatus)
    {
        case HostNetworkInterfaceStatus_Up: return "Up";
        case HostNetworkInterfaceStatus_Down: return "Down";
        case HostNetworkInterfaceStatus_Unknown: return "Unknown";
    }
    return "unknown";
}
#endif /* VBOX_WITH_HOSTNETIF_API */

static const char*getDeviceTypeText(DeviceType_T enmType)
{
    switch (enmType)
    {
        case DeviceType_HardDisk: return "HardDisk";
        case DeviceType_DVD: return "DVD";
        case DeviceType_Floppy: return "Floppy";
        /* Make MSC happy */
        case DeviceType_Null: return "Null";
        case DeviceType_Network:        return "Network";
        case DeviceType_USB:            return "USB";
        case DeviceType_SharedFolder:   return "SharedFolder";
        case DeviceType_Graphics3D:     return "Graphics3D";
    }
    return "Unknown";
}


/**
 * List internal networks.
 *
 * @returns See produceList.
 * @param   pVirtualBox         Reference to the IVirtualBox smart pointer.
 */
static HRESULT listInternalNetworks(const ComPtr<IVirtualBox> pVirtualBox)
{
    HRESULT rc;
    com::SafeArray<BSTR> internalNetworks;
    CHECK_ERROR(pVirtualBox, COMGETTER(InternalNetworks)(ComSafeArrayAsOutParam(internalNetworks)));
    for (size_t i = 0; i < internalNetworks.size(); ++i)
    {
        RTPrintf("Name:        %ls\n", internalNetworks[i]);
    }
    return rc;
}


/**
 * List network interfaces information (bridged/host only).
 *
 * @returns See produceList.
 * @param   pVirtualBox         Reference to the IVirtualBox smart pointer.
 * @param   fIsBridged          Selects between listing host interfaces (for
 *                              use with bridging) or host only interfaces.
 */
static HRESULT listNetworkInterfaces(const ComPtr<IVirtualBox> pVirtualBox,
                                     bool fIsBridged)
{
    HRESULT rc;
    ComPtr<IHost> host;
    CHECK_ERROR(pVirtualBox, COMGETTER(Host)(host.asOutParam()));
    com::SafeIfaceArray<IHostNetworkInterface> hostNetworkInterfaces;
#if defined(VBOX_WITH_NETFLT)
    if (fIsBridged)
        CHECK_ERROR(host, FindHostNetworkInterfacesOfType(HostNetworkInterfaceType_Bridged,
                                                          ComSafeArrayAsOutParam(hostNetworkInterfaces)));
    else
        CHECK_ERROR(host, FindHostNetworkInterfacesOfType(HostNetworkInterfaceType_HostOnly,
                                                          ComSafeArrayAsOutParam(hostNetworkInterfaces)));
#else
    RT_NOREF(fIsBridged);
    CHECK_ERROR(host, COMGETTER(NetworkInterfaces)(ComSafeArrayAsOutParam(hostNetworkInterfaces)));
#endif
    for (size_t i = 0; i < hostNetworkInterfaces.size(); ++i)
    {
        ComPtr<IHostNetworkInterface> networkInterface = hostNetworkInterfaces[i];
#ifndef VBOX_WITH_HOSTNETIF_API
        Bstr interfaceName;
        networkInterface->COMGETTER(Name)(interfaceName.asOutParam());
        RTPrintf("Name:        %ls\n", interfaceName.raw());
        Guid interfaceGuid;
        networkInterface->COMGETTER(Id)(interfaceGuid.asOutParam());
        RTPrintf("GUID:        %ls\n\n", Bstr(interfaceGuid.toString()).raw());
#else /* VBOX_WITH_HOSTNETIF_API */
        Bstr interfaceName;
        networkInterface->COMGETTER(Name)(interfaceName.asOutParam());
        RTPrintf("Name:            %ls\n", interfaceName.raw());
        Bstr interfaceGuid;
        networkInterface->COMGETTER(Id)(interfaceGuid.asOutParam());
        RTPrintf("GUID:            %ls\n", interfaceGuid.raw());
        BOOL bDHCPEnabled;
        networkInterface->COMGETTER(DHCPEnabled)(&bDHCPEnabled);
        RTPrintf("DHCP:            %s\n", bDHCPEnabled ? "Enabled" : "Disabled");

        Bstr IPAddress;
        networkInterface->COMGETTER(IPAddress)(IPAddress.asOutParam());
        RTPrintf("IPAddress:       %ls\n", IPAddress.raw());
        Bstr NetworkMask;
        networkInterface->COMGETTER(NetworkMask)(NetworkMask.asOutParam());
        RTPrintf("NetworkMask:     %ls\n", NetworkMask.raw());
        Bstr IPV6Address;
        networkInterface->COMGETTER(IPV6Address)(IPV6Address.asOutParam());
        RTPrintf("IPV6Address:     %ls\n", IPV6Address.raw());
        ULONG IPV6NetworkMaskPrefixLength;
        networkInterface->COMGETTER(IPV6NetworkMaskPrefixLength)(&IPV6NetworkMaskPrefixLength);
        RTPrintf("IPV6NetworkMaskPrefixLength: %d\n", IPV6NetworkMaskPrefixLength);
        Bstr HardwareAddress;
        networkInterface->COMGETTER(HardwareAddress)(HardwareAddress.asOutParam());
        RTPrintf("HardwareAddress: %ls\n", HardwareAddress.raw());
        HostNetworkInterfaceMediumType_T Type;
        networkInterface->COMGETTER(MediumType)(&Type);
        RTPrintf("MediumType:      %s\n", getHostIfMediumTypeText(Type));
        BOOL fWireless;
        networkInterface->COMGETTER(Wireless)(&fWireless);
        RTPrintf("Wireless:        %s\n", fWireless ? "Yes" : "No");
        HostNetworkInterfaceStatus_T Status;
        networkInterface->COMGETTER(Status)(&Status);
        RTPrintf("Status:          %s\n", getHostIfStatusText(Status));
        Bstr netName;
        networkInterface->COMGETTER(NetworkName)(netName.asOutParam());
        RTPrintf("VBoxNetworkName: %ls\n\n", netName.raw());
#endif
    }
    return rc;
}


/**
 * List host information.
 *
 * @returns See produceList.
 * @param   pVirtualBox         Reference to the IVirtualBox smart pointer.
 */
static HRESULT listHostInfo(const ComPtr<IVirtualBox> pVirtualBox)
{
    static struct
    {
        ProcessorFeature_T feature;
        const char *pszName;
    } features[]
    =
    {
        { ProcessorFeature_HWVirtEx,     "HW virtualization" },
        { ProcessorFeature_PAE,          "PAE" },
        { ProcessorFeature_LongMode,     "long mode" },
        { ProcessorFeature_NestedPaging, "nested paging" },
    };
    HRESULT rc;
    ComPtr<IHost> Host;
    CHECK_ERROR(pVirtualBox, COMGETTER(Host)(Host.asOutParam()));

    RTPrintf("Host Information:\n\n");

    LONG64      u64UtcTime = 0;
    CHECK_ERROR(Host, COMGETTER(UTCTime)(&u64UtcTime));
    RTTIMESPEC  timeSpec;
    char        szTime[32];
    RTPrintf("Host time: %s\n", RTTimeSpecToString(RTTimeSpecSetMilli(&timeSpec, u64UtcTime), szTime, sizeof(szTime)));

    ULONG processorOnlineCount = 0;
    CHECK_ERROR(Host, COMGETTER(ProcessorOnlineCount)(&processorOnlineCount));
    RTPrintf("Processor online count: %lu\n", processorOnlineCount);
    ULONG processorCount = 0;
    CHECK_ERROR(Host, COMGETTER(ProcessorCount)(&processorCount));
    RTPrintf("Processor count: %lu\n", processorCount);
    ULONG processorOnlineCoreCount = 0;
    CHECK_ERROR(Host, COMGETTER(ProcessorOnlineCoreCount)(&processorOnlineCoreCount));
    RTPrintf("Processor online core count: %lu\n", processorOnlineCoreCount);
    ULONG processorCoreCount = 0;
    CHECK_ERROR(Host, COMGETTER(ProcessorCoreCount)(&processorCoreCount));
    RTPrintf("Processor core count: %lu\n", processorCoreCount);
    for (unsigned i = 0; i < RT_ELEMENTS(features); i++)
    {
        BOOL supported;
        CHECK_ERROR(Host, GetProcessorFeature(features[i].feature, &supported));
        RTPrintf("Processor supports %s: %s\n", features[i].pszName, supported ? "yes" : "no");
    }
    for (ULONG i = 0; i < processorCount; i++)
    {
        ULONG processorSpeed = 0;
        CHECK_ERROR(Host, GetProcessorSpeed(i, &processorSpeed));
        if (processorSpeed)
            RTPrintf("Processor#%u speed: %lu MHz\n", i, processorSpeed);
        else
            RTPrintf("Processor#%u speed: unknown\n", i);
        Bstr processorDescription;
        CHECK_ERROR(Host, GetProcessorDescription(i, processorDescription.asOutParam()));
        RTPrintf("Processor#%u description: %ls\n", i, processorDescription.raw());
    }

    ULONG memorySize = 0;
    CHECK_ERROR(Host, COMGETTER(MemorySize)(&memorySize));
    RTPrintf("Memory size: %lu MByte\n", memorySize);

    ULONG memoryAvailable = 0;
    CHECK_ERROR(Host, COMGETTER(MemoryAvailable)(&memoryAvailable));
    RTPrintf("Memory available: %lu MByte\n", memoryAvailable);

    Bstr operatingSystem;
    CHECK_ERROR(Host, COMGETTER(OperatingSystem)(operatingSystem.asOutParam()));
    RTPrintf("Operating system: %ls\n", operatingSystem.raw());

    Bstr oSVersion;
    CHECK_ERROR(Host, COMGETTER(OSVersion)(oSVersion.asOutParam()));
    RTPrintf("Operating system version: %ls\n", oSVersion.raw());
    return rc;
}


/**
 * List media information.
 *
 * @returns See produceList.
 * @param   pVirtualBox         Reference to the IVirtualBox smart pointer.
 * @param   aMedia              Medium objects to list information for.
 * @param   pszParentUUIDStr    String with the parent UUID string (or "base").
 * @param   fOptLong            Long (@c true) or short list format.
 */
static HRESULT listMedia(const ComPtr<IVirtualBox> pVirtualBox,
                         const com::SafeIfaceArray<IMedium> &aMedia,
                         const char *pszParentUUIDStr,
                         bool fOptLong)
{
    HRESULT rc = S_OK;
    for (size_t i = 0; i < aMedia.size(); ++i)
    {
        ComPtr<IMedium> pMedium = aMedia[i];

        rc = showMediumInfo(pVirtualBox, pMedium, pszParentUUIDStr, fOptLong);

        RTPrintf("\n");

        com::SafeIfaceArray<IMedium> children;
        CHECK_ERROR(pMedium, COMGETTER(Children)(ComSafeArrayAsOutParam(children)));
        if (children.size() > 0)
        {
            Bstr uuid;
            pMedium->COMGETTER(Id)(uuid.asOutParam());

            // depth first listing of child media
            rc = listMedia(pVirtualBox, children, Utf8Str(uuid).c_str(), fOptLong);
        }
    }

    return rc;
}


/**
 * List virtual image backends.
 *
 * @returns See produceList.
 * @param   pVirtualBox         Reference to the IVirtualBox smart pointer.
 */
static HRESULT listHddBackends(const ComPtr<IVirtualBox> pVirtualBox)
{
    HRESULT rc;
    ComPtr<ISystemProperties> systemProperties;
    CHECK_ERROR(pVirtualBox, COMGETTER(SystemProperties)(systemProperties.asOutParam()));
    com::SafeIfaceArray<IMediumFormat> mediumFormats;
    CHECK_ERROR(systemProperties, COMGETTER(MediumFormats)(ComSafeArrayAsOutParam(mediumFormats)));

    RTPrintf("Supported hard disk backends:\n\n");
    for (size_t i = 0; i < mediumFormats.size(); ++i)
    {
        /* General information */
        Bstr id;
        CHECK_ERROR(mediumFormats[i], COMGETTER(Id)(id.asOutParam()));

        Bstr description;
        CHECK_ERROR(mediumFormats[i],
                    COMGETTER(Name)(description.asOutParam()));

        ULONG caps = 0;
        com::SafeArray <MediumFormatCapabilities_T> mediumFormatCap;
        CHECK_ERROR(mediumFormats[i],
                    COMGETTER(Capabilities)(ComSafeArrayAsOutParam(mediumFormatCap)));
        for (ULONG j = 0; j < mediumFormatCap.size(); j++)
            caps |= mediumFormatCap[j];


        RTPrintf("Backend %u: id='%ls' description='%ls' capabilities=%#06x extensions='",
                i, id.raw(), description.raw(), caps);

        /* File extensions */
        com::SafeArray<BSTR> fileExtensions;
        com::SafeArray<DeviceType_T> deviceTypes;
        CHECK_ERROR(mediumFormats[i],
                    DescribeFileExtensions(ComSafeArrayAsOutParam(fileExtensions), ComSafeArrayAsOutParam(deviceTypes)));
        for (size_t j = 0; j < fileExtensions.size(); ++j)
        {
            RTPrintf("%ls (%s)", Bstr(fileExtensions[j]).raw(), getDeviceTypeText(deviceTypes[j]));
            if (j != fileExtensions.size()-1)
                RTPrintf(",");
        }
        RTPrintf("'");

        /* Configuration keys */
        com::SafeArray<BSTR> propertyNames;
        com::SafeArray<BSTR> propertyDescriptions;
        com::SafeArray<DataType_T> propertyTypes;
        com::SafeArray<ULONG> propertyFlags;
        com::SafeArray<BSTR> propertyDefaults;
        CHECK_ERROR(mediumFormats[i],
                    DescribeProperties(ComSafeArrayAsOutParam(propertyNames),
                                        ComSafeArrayAsOutParam(propertyDescriptions),
                                        ComSafeArrayAsOutParam(propertyTypes),
                                        ComSafeArrayAsOutParam(propertyFlags),
                                        ComSafeArrayAsOutParam(propertyDefaults)));

        RTPrintf(" properties=(");
        if (propertyNames.size() > 0)
        {
            for (size_t j = 0; j < propertyNames.size(); ++j)
            {
                RTPrintf("\n  name='%ls' desc='%ls' type=",
                        Bstr(propertyNames[j]).raw(), Bstr(propertyDescriptions[j]).raw());
                switch (propertyTypes[j])
                {
                    case DataType_Int32: RTPrintf("int"); break;
                    case DataType_Int8: RTPrintf("byte"); break;
                    case DataType_String: RTPrintf("string"); break;
                }
                RTPrintf(" flags=%#04x", propertyFlags[j]);
                RTPrintf(" default='%ls'", Bstr(propertyDefaults[j]).raw());
                if (j != propertyNames.size()-1)
                    RTPrintf(", ");
            }
        }
        RTPrintf(")\n");
    }
    return rc;
}


/**
 * List USB devices attached to the host.
 *
 * @returns See produceList.
 * @param   pVirtualBox         Reference to the IVirtualBox smart pointer.
 */
static HRESULT listUsbHost(const ComPtr<IVirtualBox> &pVirtualBox)
{
    HRESULT rc;
    ComPtr<IHost> Host;
    CHECK_ERROR_RET(pVirtualBox, COMGETTER(Host)(Host.asOutParam()), 1);

    SafeIfaceArray<IHostUSBDevice> CollPtr;
    CHECK_ERROR_RET(Host, COMGETTER(USBDevices)(ComSafeArrayAsOutParam(CollPtr)), 1);

    RTPrintf("Host USB Devices:\n\n");

    if (CollPtr.size() == 0)
    {
        RTPrintf("<none>\n\n");
    }
    else
    {
        for (size_t i = 0; i < CollPtr.size(); ++i)
        {
            ComPtr<IHostUSBDevice> dev = CollPtr[i];

            /* Query info. */
            Bstr id;
            CHECK_ERROR_RET(dev, COMGETTER(Id)(id.asOutParam()), 1);
            USHORT usVendorId;
            CHECK_ERROR_RET(dev, COMGETTER(VendorId)(&usVendorId), 1);
            USHORT usProductId;
            CHECK_ERROR_RET(dev, COMGETTER(ProductId)(&usProductId), 1);
            USHORT bcdRevision;
            CHECK_ERROR_RET(dev, COMGETTER(Revision)(&bcdRevision), 1);
            USHORT usPort;
            CHECK_ERROR_RET(dev, COMGETTER(Port)(&usPort), 1);
            USHORT usVersion;
            CHECK_ERROR_RET(dev, COMGETTER(Version)(&usVersion), 1);
            USHORT usPortVersion;
            CHECK_ERROR_RET(dev, COMGETTER(PortVersion)(&usPortVersion), 1);
            USBConnectionSpeed_T enmSpeed;
            CHECK_ERROR_RET(dev, COMGETTER(Speed)(&enmSpeed), 1);

            RTPrintf("UUID:               %s\n"
                     "VendorId:           %#06x (%04X)\n"
                     "ProductId:          %#06x (%04X)\n"
                     "Revision:           %u.%u (%02u%02u)\n"
                     "Port:               %u\n",
                     Utf8Str(id).c_str(),
                     usVendorId, usVendorId, usProductId, usProductId,
                     bcdRevision >> 8, bcdRevision & 0xff,
                     bcdRevision >> 8, bcdRevision & 0xff,
                     usPort);

            const char *pszSpeed = "?";
            switch (enmSpeed)
            {
                case USBConnectionSpeed_Low:
                    pszSpeed = "Low";
                    break;
                case USBConnectionSpeed_Full:
                    pszSpeed = "Full";
                    break;
                case USBConnectionSpeed_High:
                    pszSpeed = "High";
                    break;
                case USBConnectionSpeed_Super:
                    pszSpeed = "Super";
                    break;
                case USBConnectionSpeed_SuperPlus:
                    pszSpeed = "SuperPlus";
                    break;
                default:
                    ASSERT(false);
                    break;
            }

            RTPrintf("USB version/speed:  %u/%s\n", usVersion, pszSpeed);

            /* optional stuff. */
            SafeArray<BSTR> CollDevInfo;
            Bstr bstr;
            CHECK_ERROR_RET(dev, COMGETTER(DeviceInfo)(ComSafeArrayAsOutParam(CollDevInfo)), 1);
            if (CollDevInfo.size() >= 1)
                bstr = Bstr(CollDevInfo[0]);
            if (!bstr.isEmpty())
                RTPrintf("Manufacturer:       %ls\n", bstr.raw());
            if (CollDevInfo.size() >= 2)
                bstr = Bstr(CollDevInfo[1]);
            if (!bstr.isEmpty())
                RTPrintf("Product:            %ls\n", bstr.raw());
            CHECK_ERROR_RET(dev, COMGETTER(SerialNumber)(bstr.asOutParam()), 1);
            if (!bstr.isEmpty())
                RTPrintf("SerialNumber:       %ls\n", bstr.raw());
            CHECK_ERROR_RET(dev, COMGETTER(Address)(bstr.asOutParam()), 1);
            if (!bstr.isEmpty())
                RTPrintf("Address:            %ls\n", bstr.raw());

            /* current state  */
            USBDeviceState_T state;
            CHECK_ERROR_RET(dev, COMGETTER(State)(&state), 1);
            const char *pszState = "?";
            switch (state)
            {
                case USBDeviceState_NotSupported:
                    pszState = "Not supported";
                    break;
                case USBDeviceState_Unavailable:
                    pszState = "Unavailable";
                    break;
                case USBDeviceState_Busy:
                    pszState = "Busy";
                    break;
                case USBDeviceState_Available:
                    pszState = "Available";
                    break;
                case USBDeviceState_Held:
                    pszState = "Held";
                    break;
                case USBDeviceState_Captured:
                    pszState = "Captured";
                    break;
                default:
                    ASSERT(false);
                    break;
            }
            RTPrintf("Current State:      %s\n\n", pszState);
        }
    }
    return rc;
}


/**
 * List USB filters.
 *
 * @returns See produceList.
 * @param   pVirtualBox         Reference to the IVirtualBox smart pointer.
 */
static HRESULT listUsbFilters(const ComPtr<IVirtualBox> &pVirtualBox)
{
    HRESULT rc;

    RTPrintf("Global USB Device Filters:\n\n");

    ComPtr<IHost> host;
    CHECK_ERROR_RET(pVirtualBox, COMGETTER(Host)(host.asOutParam()), 1);

    SafeIfaceArray<IHostUSBDeviceFilter> coll;
    CHECK_ERROR_RET(host, COMGETTER(USBDeviceFilters)(ComSafeArrayAsOutParam(coll)), 1);

    if (coll.size() == 0)
    {
        RTPrintf("<none>\n\n");
    }
    else
    {
        for (size_t index = 0; index < coll.size(); ++index)
        {
            ComPtr<IHostUSBDeviceFilter> flt = coll[index];

            /* Query info. */

            RTPrintf("Index:            %zu\n", index);

            BOOL active = FALSE;
            CHECK_ERROR_RET(flt, COMGETTER(Active)(&active), 1);
            RTPrintf("Active:           %s\n", active ? "yes" : "no");

            USBDeviceFilterAction_T action;
            CHECK_ERROR_RET(flt, COMGETTER(Action)(&action), 1);
            const char *pszAction = "<invalid>";
            switch (action)
            {
                case USBDeviceFilterAction_Ignore:
                    pszAction = "Ignore";
                    break;
                case USBDeviceFilterAction_Hold:
                    pszAction = "Hold";
                    break;
                default:
                    break;
            }
            RTPrintf("Action:           %s\n", pszAction);

            Bstr bstr;
            CHECK_ERROR_RET(flt, COMGETTER(Name)(bstr.asOutParam()), 1);
            RTPrintf("Name:             %ls\n", bstr.raw());
            CHECK_ERROR_RET(flt, COMGETTER(VendorId)(bstr.asOutParam()), 1);
            RTPrintf("VendorId:         %ls\n", bstr.raw());
            CHECK_ERROR_RET(flt, COMGETTER(ProductId)(bstr.asOutParam()), 1);
            RTPrintf("ProductId:        %ls\n", bstr.raw());
            CHECK_ERROR_RET(flt, COMGETTER(Revision)(bstr.asOutParam()), 1);
            RTPrintf("Revision:         %ls\n", bstr.raw());
            CHECK_ERROR_RET(flt, COMGETTER(Manufacturer)(bstr.asOutParam()), 1);
            RTPrintf("Manufacturer:     %ls\n", bstr.raw());
            CHECK_ERROR_RET(flt, COMGETTER(Product)(bstr.asOutParam()), 1);
            RTPrintf("Product:          %ls\n", bstr.raw());
            CHECK_ERROR_RET(flt, COMGETTER(SerialNumber)(bstr.asOutParam()), 1);
            RTPrintf("Serial Number:    %ls\n\n", bstr.raw());
        }
    }
    return rc;
}


/**
 * List system properties.
 *
 * @returns See produceList.
 * @param   pVirtualBox         Reference to the IVirtualBox smart pointer.
 */
static HRESULT listSystemProperties(const ComPtr<IVirtualBox> &pVirtualBox)
{
    ComPtr<ISystemProperties> systemProperties;
    pVirtualBox->COMGETTER(SystemProperties)(systemProperties.asOutParam());

    Bstr str;
    ULONG ulValue;
    LONG64 i64Value;
    BOOL fValue;
    const char *psz;

    pVirtualBox->COMGETTER(APIVersion)(str.asOutParam());
    RTPrintf("API version:                     %ls\n", str.raw());

    systemProperties->COMGETTER(MinGuestRAM)(&ulValue);
    RTPrintf("Minimum guest RAM size:          %u Megabytes\n", ulValue);
    systemProperties->COMGETTER(MaxGuestRAM)(&ulValue);
    RTPrintf("Maximum guest RAM size:          %u Megabytes\n", ulValue);
    systemProperties->COMGETTER(MinGuestVRAM)(&ulValue);
    RTPrintf("Minimum video RAM size:          %u Megabytes\n", ulValue);
    systemProperties->COMGETTER(MaxGuestVRAM)(&ulValue);
    RTPrintf("Maximum video RAM size:          %u Megabytes\n", ulValue);
    systemProperties->COMGETTER(MaxGuestMonitors)(&ulValue);
    RTPrintf("Maximum guest monitor count:     %u\n", ulValue);
    systemProperties->COMGETTER(MinGuestCPUCount)(&ulValue);
    RTPrintf("Minimum guest CPU count:         %u\n", ulValue);
    systemProperties->COMGETTER(MaxGuestCPUCount)(&ulValue);
    RTPrintf("Maximum guest CPU count:         %u\n", ulValue);
    systemProperties->COMGETTER(InfoVDSize)(&i64Value);
    RTPrintf("Virtual disk limit (info):       %lld Bytes\n", i64Value);
    systemProperties->COMGETTER(SerialPortCount)(&ulValue);
    RTPrintf("Maximum Serial Port count:       %u\n", ulValue);
    systemProperties->COMGETTER(ParallelPortCount)(&ulValue);
    RTPrintf("Maximum Parallel Port count:     %u\n", ulValue);
    systemProperties->COMGETTER(MaxBootPosition)(&ulValue);
    RTPrintf("Maximum Boot Position:           %u\n", ulValue);
    systemProperties->GetMaxNetworkAdapters(ChipsetType_PIIX3, &ulValue);
    RTPrintf("Maximum PIIX3 Network Adapter count:   %u\n", ulValue);
    systemProperties->GetMaxNetworkAdapters(ChipsetType_ICH9,  &ulValue);
    RTPrintf("Maximum ICH9 Network Adapter count:   %u\n", ulValue);
    systemProperties->GetMaxInstancesOfStorageBus(ChipsetType_PIIX3, StorageBus_IDE, &ulValue);
    RTPrintf("Maximum PIIX3 IDE Controllers:   %u\n", ulValue);
    systemProperties->GetMaxInstancesOfStorageBus(ChipsetType_ICH9, StorageBus_IDE, &ulValue);
    RTPrintf("Maximum ICH9 IDE Controllers:    %u\n", ulValue);
    systemProperties->GetMaxPortCountForStorageBus(StorageBus_IDE, &ulValue);
    RTPrintf("Maximum IDE Port count:          %u\n", ulValue);
    systemProperties->GetMaxDevicesPerPortForStorageBus(StorageBus_IDE, &ulValue);
    RTPrintf("Maximum Devices per IDE Port:    %u\n", ulValue);
    systemProperties->GetMaxInstancesOfStorageBus(ChipsetType_PIIX3, StorageBus_SATA, &ulValue);
    RTPrintf("Maximum PIIX3 SATA Controllers:  %u\n", ulValue);
    systemProperties->GetMaxInstancesOfStorageBus(ChipsetType_ICH9, StorageBus_SATA, &ulValue);
    RTPrintf("Maximum ICH9 SATA Controllers:   %u\n", ulValue);
    systemProperties->GetMaxPortCountForStorageBus(StorageBus_SATA, &ulValue);
    RTPrintf("Maximum SATA Port count:         %u\n", ulValue);
    systemProperties->GetMaxDevicesPerPortForStorageBus(StorageBus_SATA, &ulValue);
    RTPrintf("Maximum Devices per SATA Port:   %u\n", ulValue);
    systemProperties->GetMaxInstancesOfStorageBus(ChipsetType_PIIX3, StorageBus_SCSI, &ulValue);
    RTPrintf("Maximum PIIX3 SCSI Controllers:  %u\n", ulValue);
    systemProperties->GetMaxInstancesOfStorageBus(ChipsetType_ICH9, StorageBus_SCSI, &ulValue);
    RTPrintf("Maximum ICH9 SCSI Controllers:   %u\n", ulValue);
    systemProperties->GetMaxPortCountForStorageBus(StorageBus_SCSI, &ulValue);
    RTPrintf("Maximum SCSI Port count:         %u\n", ulValue);
    systemProperties->GetMaxDevicesPerPortForStorageBus(StorageBus_SCSI, &ulValue);
    RTPrintf("Maximum Devices per SCSI Port:   %u\n", ulValue);
    systemProperties->GetMaxInstancesOfStorageBus(ChipsetType_PIIX3, StorageBus_SAS, &ulValue);
    RTPrintf("Maximum SAS PIIX3 Controllers:   %u\n", ulValue);
    systemProperties->GetMaxInstancesOfStorageBus(ChipsetType_ICH9, StorageBus_SAS, &ulValue);
    RTPrintf("Maximum SAS ICH9 Controllers:    %u\n", ulValue);
    systemProperties->GetMaxPortCountForStorageBus(StorageBus_SAS, &ulValue);
    RTPrintf("Maximum SAS Port count:          %u\n", ulValue);
    systemProperties->GetMaxDevicesPerPortForStorageBus(StorageBus_SAS, &ulValue);
    RTPrintf("Maximum Devices per SAS Port:    %u\n", ulValue);
    systemProperties->GetMaxInstancesOfStorageBus(ChipsetType_PIIX3, StorageBus_PCIe, &ulValue);
    RTPrintf("Maximum NVMe PIIX3 Controllers:  %u\n", ulValue);
    systemProperties->GetMaxInstancesOfStorageBus(ChipsetType_ICH9, StorageBus_PCIe, &ulValue);
    RTPrintf("Maximum NVMe ICH9 Controllers:   %u\n", ulValue);
    systemProperties->GetMaxPortCountForStorageBus(StorageBus_PCIe, &ulValue);
    RTPrintf("Maximum NVMe Port count:         %u\n", ulValue);
    systemProperties->GetMaxDevicesPerPortForStorageBus(StorageBus_PCIe, &ulValue);
    RTPrintf("Maximum Devices per NVMe Port:   %u\n", ulValue);
    systemProperties->GetMaxInstancesOfStorageBus(ChipsetType_PIIX3, StorageBus_Floppy, &ulValue);
    RTPrintf("Maximum PIIX3 Floppy Controllers:%u\n", ulValue);
    systemProperties->GetMaxInstancesOfStorageBus(ChipsetType_ICH9, StorageBus_Floppy, &ulValue);
    RTPrintf("Maximum ICH9 Floppy Controllers: %u\n", ulValue);
    systemProperties->GetMaxPortCountForStorageBus(StorageBus_Floppy, &ulValue);
    RTPrintf("Maximum Floppy Port count:       %u\n", ulValue);
    systemProperties->GetMaxDevicesPerPortForStorageBus(StorageBus_Floppy, &ulValue);
    RTPrintf("Maximum Devices per Floppy Port: %u\n", ulValue);
#if 0
    systemProperties->GetFreeDiskSpaceWarning(&i64Value);
    RTPrintf("Free disk space warning at:      %u Bytes\n", i64Value);
    systemProperties->GetFreeDiskSpacePercentWarning(&ulValue);
    RTPrintf("Free disk space warning at:      %u %%\n", ulValue);
    systemProperties->GetFreeDiskSpaceError(&i64Value);
    RTPrintf("Free disk space error at:        %u Bytes\n", i64Value);
    systemProperties->GetFreeDiskSpacePercentError(&ulValue);
    RTPrintf("Free disk space error at:        %u %%\n", ulValue);
#endif
    systemProperties->COMGETTER(DefaultMachineFolder)(str.asOutParam());
    RTPrintf("Default machine folder:          %ls\n", str.raw());
    systemProperties->COMGETTER(RawModeSupported)(&fValue);
    RTPrintf("Raw-mode Supported:              %s\n", fValue ? "yes" : "no");
    systemProperties->COMGETTER(ExclusiveHwVirt)(&fValue);
    RTPrintf("Exclusive HW virtualization use: %s\n", fValue ? "on" : "off");
    systemProperties->COMGETTER(DefaultHardDiskFormat)(str.asOutParam());
    RTPrintf("Default hard disk format:        %ls\n", str.raw());
    systemProperties->COMGETTER(VRDEAuthLibrary)(str.asOutParam());
    RTPrintf("VRDE auth library:               %ls\n", str.raw());
    systemProperties->COMGETTER(WebServiceAuthLibrary)(str.asOutParam());
    RTPrintf("Webservice auth. library:        %ls\n", str.raw());
    systemProperties->COMGETTER(DefaultVRDEExtPack)(str.asOutParam());
    RTPrintf("Remote desktop ExtPack:          %ls\n", str.raw());
    systemProperties->COMGETTER(LogHistoryCount)(&ulValue);
    RTPrintf("Log history count:               %u\n", ulValue);
    systemProperties->COMGETTER(DefaultFrontend)(str.asOutParam());
    RTPrintf("Default frontend:                %ls\n", str.raw());
    AudioDriverType_T enmAudio;
    systemProperties->COMGETTER(DefaultAudioDriver)(&enmAudio);
    switch (enmAudio)
    {
        case AudioDriverType_Null:          psz = "Null";           break;
        case AudioDriverType_WinMM:         psz = "WinMM";          break;
        case AudioDriverType_OSS:           psz = "OSS";            break;
        case AudioDriverType_ALSA:          psz = "ALSA";           break;
        case AudioDriverType_DirectSound:   psz = "DirectSound";    break;
        case AudioDriverType_CoreAudio:     psz = "CoreAudio";      break;
        case AudioDriverType_MMPM:          psz = "MMPM";           break;
        case AudioDriverType_Pulse:         psz = "Pulse";          break;
        case AudioDriverType_SolAudio:      psz = "SolAudio";       break;
        default:                            psz = "Unknown";
    }
    RTPrintf("Default audio driver:            %s\n", psz);
    systemProperties->COMGETTER(AutostartDatabasePath)(str.asOutParam());
    RTPrintf("Autostart database path:         %ls\n", str.raw());
    systemProperties->COMGETTER(DefaultAdditionsISO)(str.asOutParam());
    RTPrintf("Default Guest Additions ISO:     %ls\n", str.raw());
    systemProperties->COMGETTER(LoggingLevel)(str.asOutParam());
    RTPrintf("Logging Level:                   %ls\n", str.raw());
    return S_OK;
}


/**
 * List extension packs.
 *
 * @returns See produceList.
 * @param   pVirtualBox         Reference to the IVirtualBox smart pointer.
 */
static HRESULT listExtensionPacks(const ComPtr<IVirtualBox> &pVirtualBox)
{
    ComObjPtr<IExtPackManager> ptrExtPackMgr;
    CHECK_ERROR2I_RET(pVirtualBox, COMGETTER(ExtensionPackManager)(ptrExtPackMgr.asOutParam()), hrcCheck);

    SafeIfaceArray<IExtPack> extPacks;
    CHECK_ERROR2I_RET(ptrExtPackMgr, COMGETTER(InstalledExtPacks)(ComSafeArrayAsOutParam(extPacks)), hrcCheck);
    RTPrintf("Extension Packs: %u\n", extPacks.size());

    HRESULT hrc = S_OK;
    for (size_t i = 0; i < extPacks.size(); i++)
    {
        /* Read all the properties. */
        Bstr bstrName;
        CHECK_ERROR2I_STMT(extPacks[i], COMGETTER(Name)(bstrName.asOutParam()),          hrc = hrcCheck; bstrName.setNull());
        Bstr bstrDesc;
        CHECK_ERROR2I_STMT(extPacks[i], COMGETTER(Description)(bstrDesc.asOutParam()),   hrc = hrcCheck; bstrDesc.setNull());
        Bstr bstrVersion;
        CHECK_ERROR2I_STMT(extPacks[i], COMGETTER(Version)(bstrVersion.asOutParam()),    hrc = hrcCheck; bstrVersion.setNull());
        ULONG uRevision;
        CHECK_ERROR2I_STMT(extPacks[i], COMGETTER(Revision)(&uRevision),                 hrc = hrcCheck; uRevision = 0);
        Bstr bstrEdition;
        CHECK_ERROR2I_STMT(extPacks[i], COMGETTER(Edition)(bstrEdition.asOutParam()),    hrc = hrcCheck; bstrEdition.setNull());
        Bstr bstrVrdeModule;
        CHECK_ERROR2I_STMT(extPacks[i], COMGETTER(VRDEModule)(bstrVrdeModule.asOutParam()),hrc=hrcCheck; bstrVrdeModule.setNull());
        BOOL fUsable;
        CHECK_ERROR2I_STMT(extPacks[i], COMGETTER(Usable)(&fUsable),                     hrc = hrcCheck; fUsable = FALSE);
        Bstr bstrWhy;
        CHECK_ERROR2I_STMT(extPacks[i], COMGETTER(WhyUnusable)(bstrWhy.asOutParam()),    hrc = hrcCheck; bstrWhy.setNull());

        /* Display them. */
        if (i)
            RTPrintf("\n");
        RTPrintf("Pack no.%2zu:   %ls\n"
                 "Version:      %ls\n"
                 "Revision:     %u\n"
                 "Edition:      %ls\n"
                 "Description:  %ls\n"
                 "VRDE Module:  %ls\n"
                 "Usable:       %RTbool\n"
                 "Why unusable: %ls\n",
                 i, bstrName.raw(),
                 bstrVersion.raw(),
                 uRevision,
                 bstrEdition.raw(),
                 bstrDesc.raw(),
                 bstrVrdeModule.raw(),
                 fUsable != FALSE,
                 bstrWhy.raw());

        /* Query plugins and display them. */
    }
    return hrc;
}


/**
 * List machine groups.
 *
 * @returns See produceList.
 * @param   pVirtualBox         Reference to the IVirtualBox smart pointer.
 */
static HRESULT listGroups(const ComPtr<IVirtualBox> &pVirtualBox)
{
    SafeArray<BSTR> groups;
    CHECK_ERROR2I_RET(pVirtualBox, COMGETTER(MachineGroups)(ComSafeArrayAsOutParam(groups)), hrcCheck);

    for (size_t i = 0; i < groups.size(); i++)
    {
        RTPrintf("\"%ls\"\n", groups[i]);
    }
    return S_OK;
}


/**
 * List video capture devices.
 *
 * @returns See produceList.
 * @param   pVirtualBox         Reference to the IVirtualBox pointer.
 */
static HRESULT listVideoInputDevices(const ComPtr<IVirtualBox> pVirtualBox)
{
    HRESULT rc;
    ComPtr<IHost> host;
    CHECK_ERROR(pVirtualBox, COMGETTER(Host)(host.asOutParam()));
    com::SafeIfaceArray<IHostVideoInputDevice> hostVideoInputDevices;
    CHECK_ERROR(host, COMGETTER(VideoInputDevices)(ComSafeArrayAsOutParam(hostVideoInputDevices)));
    RTPrintf("Video Input Devices: %u\n", hostVideoInputDevices.size());
    for (size_t i = 0; i < hostVideoInputDevices.size(); ++i)
    {
        ComPtr<IHostVideoInputDevice> p = hostVideoInputDevices[i];
        Bstr name;
        p->COMGETTER(Name)(name.asOutParam());
        Bstr path;
        p->COMGETTER(Path)(path.asOutParam());
        Bstr alias;
        p->COMGETTER(Alias)(alias.asOutParam());
        RTPrintf("%ls \"%ls\"\n%ls\n", alias.raw(), name.raw(), path.raw());
    }
    return rc;
}

/**
 * List supported screen shot formats.
 *
 * @returns See produceList.
 * @param   pVirtualBox         Reference to the IVirtualBox pointer.
 */
static HRESULT listScreenShotFormats(const ComPtr<IVirtualBox> pVirtualBox)
{
    HRESULT rc = S_OK;
    ComPtr<ISystemProperties> systemProperties;
    CHECK_ERROR(pVirtualBox, COMGETTER(SystemProperties)(systemProperties.asOutParam()));
    com::SafeArray<BitmapFormat_T> formats;
    CHECK_ERROR(systemProperties, COMGETTER(ScreenShotFormats)(ComSafeArrayAsOutParam(formats)));

    RTPrintf("Supported %d screen shot formats:\n", formats.size());
    for (size_t i = 0; i < formats.size(); ++i)
    {
        uint32_t u32Format = (uint32_t)formats[i];
        char szFormat[5];
        szFormat[0] = RT_BYTE1(u32Format);
        szFormat[1] = RT_BYTE2(u32Format);
        szFormat[2] = RT_BYTE3(u32Format);
        szFormat[3] = RT_BYTE4(u32Format);
        szFormat[4] = 0;
        RTPrintf("    BitmapFormat_%s (0x%08X)\n", szFormat, u32Format);
    }
    return rc;
}


/**
 * The type of lists we can produce.
 */
enum enmListType
{
    kListNotSpecified = 1000,
    kListVMs,
    kListRunningVMs,
    kListOsTypes,
    kListHostDvds,
    kListHostFloppies,
    kListInternalNetworks,
    kListBridgedInterfaces,
#if defined(VBOX_WITH_NETFLT)
    kListHostOnlyInterfaces,
#endif
    kListHostCpuIDs,
    kListHostInfo,
    kListHddBackends,
    kListHdds,
    kListDvds,
    kListFloppies,
    kListUsbHost,
    kListUsbFilters,
    kListSystemProperties,
    kListDhcpServers,
    kListExtPacks,
    kListGroups,
    kListNatNetworks,
    kListVideoInputDevices,
    kListScreenShotFormats
};


/**
 * Produces the specified listing.
 *
 * @returns S_OK or some COM error code that has been reported in full.
 * @param   enmList             The list to produce.
 * @param   fOptLong            Long (@c true) or short list format.
 * @param   pVirtualBox         Reference to the IVirtualBox smart pointer.
 */
static HRESULT produceList(enum enmListType enmCommand, bool fOptLong, bool fOptSorted, const ComPtr<IVirtualBox> &pVirtualBox)
{
    HRESULT rc = S_OK;
    switch (enmCommand)
    {
        case kListNotSpecified:
            AssertFailed();
            return E_FAIL;

        case kListVMs:
        {
            /*
             * Get the list of all registered VMs
             */
            com::SafeIfaceArray<IMachine> machines;
            rc = pVirtualBox->COMGETTER(Machines)(ComSafeArrayAsOutParam(machines));
            if (SUCCEEDED(rc))
            {
                /*
                 * Display it.
                 */
                if (!fOptSorted)
                {
                    for (size_t i = 0; i < machines.size(); ++i)
                        if (machines[i])
                            rc = showVMInfo(pVirtualBox, machines[i], NULL, fOptLong ? VMINFO_STANDARD : VMINFO_COMPACT);
                }
                else
                {
                    /*
                     * Sort the list by name before displaying it.
                     */
                    std::vector<std::pair<com::Bstr, IMachine *> > sortedMachines;
                    for (size_t i = 0; i < machines.size(); ++i)
                    {
                        IMachine *pMachine = machines[i];
                        if (pMachine) /* no idea why we need to do this... */
                        {
                            Bstr bstrName;
                            pMachine->COMGETTER(Name)(bstrName.asOutParam());
                            sortedMachines.push_back(std::pair<com::Bstr, IMachine *>(bstrName, pMachine));
                        }
                    }

                    std::sort(sortedMachines.begin(), sortedMachines.end());

                    for (size_t i = 0; i < sortedMachines.size(); ++i)
                        rc = showVMInfo(pVirtualBox, sortedMachines[i].second, NULL, fOptLong ? VMINFO_STANDARD : VMINFO_COMPACT);
                }
            }
            break;
        }

        case kListRunningVMs:
        {
            /*
             * Get the list of all _running_ VMs
             */
            com::SafeIfaceArray<IMachine> machines;
            rc = pVirtualBox->COMGETTER(Machines)(ComSafeArrayAsOutParam(machines));
            com::SafeArray<MachineState_T> states;
            if (SUCCEEDED(rc))
                rc = pVirtualBox->GetMachineStates(ComSafeArrayAsInParam(machines), ComSafeArrayAsOutParam(states));
            if (SUCCEEDED(rc))
            {
                /*
                 * Iterate through the collection
                 */
                for (size_t i = 0; i < machines.size(); ++i)
                {
                    if (machines[i])
                    {
                        MachineState_T machineState = states[i];
                        switch (machineState)
                        {
                            case MachineState_Running:
                            case MachineState_Teleporting:
                            case MachineState_LiveSnapshotting:
                            case MachineState_Paused:
                            case MachineState_TeleportingPausedVM:
                                rc = showVMInfo(pVirtualBox, machines[i], NULL, fOptLong ? VMINFO_STANDARD : VMINFO_COMPACT);
                                break;
                            default: break; /* Shut up MSC */
                        }
                    }
                }
            }
            break;
        }

        case kListOsTypes:
        {
            com::SafeIfaceArray<IGuestOSType> coll;
            rc = pVirtualBox->COMGETTER(GuestOSTypes)(ComSafeArrayAsOutParam(coll));
            if (SUCCEEDED(rc))
            {
                /*
                 * Iterate through the collection.
                 */
                for (size_t i = 0; i < coll.size(); ++i)
                {
                    ComPtr<IGuestOSType> guestOS;
                    guestOS = coll[i];
                    Bstr guestId;
                    guestOS->COMGETTER(Id)(guestId.asOutParam());
                    RTPrintf("ID:          %ls\n", guestId.raw());
                    Bstr guestDescription;
                    guestOS->COMGETTER(Description)(guestDescription.asOutParam());
                    RTPrintf("Description: %ls\n", guestDescription.raw());
                    Bstr familyId;
                    guestOS->COMGETTER(FamilyId)(familyId.asOutParam());
                    RTPrintf("Family ID:   %ls\n", familyId.raw());
                    Bstr familyDescription;
                    guestOS->COMGETTER(FamilyDescription)(familyDescription.asOutParam());
                    RTPrintf("Family Desc: %ls\n", familyDescription.raw());
                    BOOL is64Bit;
                    guestOS->COMGETTER(Is64Bit)(&is64Bit);
                    RTPrintf("64 bit:      %RTbool\n", is64Bit);
                    RTPrintf("\n");
                }
            }
            break;
        }

        case kListHostDvds:
        {
            ComPtr<IHost> host;
            CHECK_ERROR(pVirtualBox, COMGETTER(Host)(host.asOutParam()));
            com::SafeIfaceArray<IMedium> coll;
            CHECK_ERROR(host, COMGETTER(DVDDrives)(ComSafeArrayAsOutParam(coll)));
            if (SUCCEEDED(rc))
            {
                for (size_t i = 0; i < coll.size(); ++i)
                {
                    ComPtr<IMedium> dvdDrive = coll[i];
                    Bstr uuid;
                    dvdDrive->COMGETTER(Id)(uuid.asOutParam());
                    RTPrintf("UUID:         %s\n", Utf8Str(uuid).c_str());
                    Bstr location;
                    dvdDrive->COMGETTER(Location)(location.asOutParam());
                    RTPrintf("Name:         %ls\n\n", location.raw());
                }
            }
            break;
        }

        case kListHostFloppies:
        {
            ComPtr<IHost> host;
            CHECK_ERROR(pVirtualBox, COMGETTER(Host)(host.asOutParam()));
            com::SafeIfaceArray<IMedium> coll;
            CHECK_ERROR(host, COMGETTER(FloppyDrives)(ComSafeArrayAsOutParam(coll)));
            if (SUCCEEDED(rc))
            {
                for (size_t i = 0; i < coll.size(); ++i)
                {
                    ComPtr<IMedium> floppyDrive = coll[i];
                    Bstr uuid;
                    floppyDrive->COMGETTER(Id)(uuid.asOutParam());
                    RTPrintf("UUID:         %s\n", Utf8Str(uuid).c_str());
                    Bstr location;
                    floppyDrive->COMGETTER(Location)(location.asOutParam());
                    RTPrintf("Name:         %ls\n\n", location.raw());
                }
            }
            break;
        }

        case kListInternalNetworks:
            rc = listInternalNetworks(pVirtualBox);
            break;

        case kListBridgedInterfaces:
#if defined(VBOX_WITH_NETFLT)
        case kListHostOnlyInterfaces:
#endif
            rc = listNetworkInterfaces(pVirtualBox, enmCommand == kListBridgedInterfaces);
            break;

        case kListHostInfo:
            rc = listHostInfo(pVirtualBox);
            break;

        case kListHostCpuIDs:
        {
            ComPtr<IHost> Host;
            CHECK_ERROR(pVirtualBox, COMGETTER(Host)(Host.asOutParam()));

            RTPrintf("Host CPUIDs:\n\nLeaf no.  EAX      EBX      ECX      EDX\n");
            ULONG uCpuNo = 0; /* ASSUMES that CPU#0 is online. */
            static uint32_t const s_auCpuIdRanges[] =
            {
                UINT32_C(0x00000000), UINT32_C(0x0000007f),
                UINT32_C(0x80000000), UINT32_C(0x8000007f),
                UINT32_C(0xc0000000), UINT32_C(0xc000007f)
            };
            for (unsigned i = 0; i < RT_ELEMENTS(s_auCpuIdRanges); i += 2)
            {
                ULONG uEAX, uEBX, uECX, uEDX, cLeafs;
                CHECK_ERROR(Host, GetProcessorCPUIDLeaf(uCpuNo, s_auCpuIdRanges[i], 0, &cLeafs, &uEBX, &uECX, &uEDX));
                if (cLeafs < s_auCpuIdRanges[i] || cLeafs > s_auCpuIdRanges[i+1])
                    continue;
                cLeafs++;
                for (ULONG iLeaf = s_auCpuIdRanges[i]; iLeaf <= cLeafs; iLeaf++)
                {
                    CHECK_ERROR(Host, GetProcessorCPUIDLeaf(uCpuNo, iLeaf, 0, &uEAX, &uEBX, &uECX, &uEDX));
                    RTPrintf("%08x  %08x %08x %08x %08x\n", iLeaf, uEAX, uEBX, uECX, uEDX);
                }
            }
            break;
        }

        case kListHddBackends:
            rc = listHddBackends(pVirtualBox);
            break;

        case kListHdds:
        {
            com::SafeIfaceArray<IMedium> hdds;
            CHECK_ERROR(pVirtualBox, COMGETTER(HardDisks)(ComSafeArrayAsOutParam(hdds)));
            rc = listMedia(pVirtualBox, hdds, "base", fOptLong);
            break;
        }

        case kListDvds:
        {
            com::SafeIfaceArray<IMedium> dvds;
            CHECK_ERROR(pVirtualBox, COMGETTER(DVDImages)(ComSafeArrayAsOutParam(dvds)));
            rc = listMedia(pVirtualBox, dvds, NULL, fOptLong);
            break;
        }

        case kListFloppies:
        {
            com::SafeIfaceArray<IMedium> floppies;
            CHECK_ERROR(pVirtualBox, COMGETTER(FloppyImages)(ComSafeArrayAsOutParam(floppies)));
            rc = listMedia(pVirtualBox, floppies, NULL, fOptLong);
            break;
        }

        case kListUsbHost:
            rc = listUsbHost(pVirtualBox);
            break;

        case kListUsbFilters:
            rc = listUsbFilters(pVirtualBox);
            break;

        case kListSystemProperties:
            rc = listSystemProperties(pVirtualBox);
            break;

        case kListDhcpServers:
        {
            com::SafeIfaceArray<IDHCPServer> svrs;
            CHECK_ERROR(pVirtualBox, COMGETTER(DHCPServers)(ComSafeArrayAsOutParam(svrs)));
            for (size_t i = 0; i < svrs.size(); ++i)
            {
                ComPtr<IDHCPServer> svr = svrs[i];
                Bstr netName;
                svr->COMGETTER(NetworkName)(netName.asOutParam());
                RTPrintf("NetworkName:    %ls\n", netName.raw());
                Bstr ip;
                svr->COMGETTER(IPAddress)(ip.asOutParam());
                RTPrintf("IP:             %ls\n", ip.raw());
                Bstr netmask;
                svr->COMGETTER(NetworkMask)(netmask.asOutParam());
                RTPrintf("NetworkMask:    %ls\n", netmask.raw());
                Bstr lowerIp;
                svr->COMGETTER(LowerIP)(lowerIp.asOutParam());
                RTPrintf("lowerIPAddress: %ls\n", lowerIp.raw());
                Bstr upperIp;
                svr->COMGETTER(UpperIP)(upperIp.asOutParam());
                RTPrintf("upperIPAddress: %ls\n", upperIp.raw());
                BOOL fEnabled;
                svr->COMGETTER(Enabled)(&fEnabled);
                RTPrintf("Enabled:        %s\n", fEnabled ? "Yes" : "No");
                RTPrintf("\n");
            }
            break;
        }

        case kListExtPacks:
            rc = listExtensionPacks(pVirtualBox);
            break;

        case kListGroups:
            rc = listGroups(pVirtualBox);
            break;

        case kListNatNetworks:
        {
            com::SafeIfaceArray<INATNetwork> nets;
            CHECK_ERROR(pVirtualBox, COMGETTER(NATNetworks)(ComSafeArrayAsOutParam(nets)));
            for (size_t i = 0; i < nets.size(); ++i)
            {
                ComPtr<INATNetwork> net = nets[i];
                Bstr netName;
                net->COMGETTER(NetworkName)(netName.asOutParam());
                RTPrintf("NetworkName:    %ls\n", netName.raw());
                Bstr gateway;
                net->COMGETTER(Gateway)(gateway.asOutParam());
                RTPrintf("IP:             %ls\n", gateway.raw());
                Bstr network;
                net->COMGETTER(Network)(network.asOutParam());
                RTPrintf("Network:        %ls\n", network.raw());
                BOOL fEnabled;
                net->COMGETTER(IPv6Enabled)(&fEnabled);
                RTPrintf("IPv6 Enabled:   %s\n", fEnabled ? "Yes" : "No");
                Bstr ipv6prefix;
                net->COMGETTER(IPv6Prefix)(ipv6prefix.asOutParam());
                RTPrintf("IPv6 Prefix:    %ls\n", ipv6prefix.raw());
                net->COMGETTER(NeedDhcpServer)(&fEnabled);
                RTPrintf("DHCP Enabled:   %s\n", fEnabled ? "Yes" : "No");
                net->COMGETTER(Enabled)(&fEnabled);
                RTPrintf("Enabled:        %s\n", fEnabled ? "Yes" : "No");

#define PRINT_STRING_ARRAY(title) \
                if (strs.size() > 0)    \
                { \
                    RTPrintf(title); \
                    size_t j = 0; \
                    for (;j < strs.size(); ++j) \
                        RTPrintf("        %s\n", Utf8Str(strs[j]).c_str()); \
                }

                com::SafeArray<BSTR> strs;

                CHECK_ERROR(nets[i], COMGETTER(PortForwardRules4)(ComSafeArrayAsOutParam(strs)));
                PRINT_STRING_ARRAY("Port-forwarding (ipv4)\n");
                strs.setNull();

                CHECK_ERROR(nets[i], COMGETTER(PortForwardRules6)(ComSafeArrayAsOutParam(strs)));
                PRINT_STRING_ARRAY("Port-forwarding (ipv6)\n");
                strs.setNull();

                CHECK_ERROR(nets[i], COMGETTER(LocalMappings)(ComSafeArrayAsOutParam(strs)));
                PRINT_STRING_ARRAY("loopback mappings (ipv4)\n");
                strs.setNull();

#undef PRINT_STRING_ARRAY
                RTPrintf("\n");
            }
            break;
        }

        case kListVideoInputDevices:
            rc = listVideoInputDevices(pVirtualBox);
            break;

        case kListScreenShotFormats:
            rc = listScreenShotFormats(pVirtualBox);
            break;

        /* No default here, want gcc warnings. */

    } /* end switch */

    return rc;
}

/**
 * Handles the 'list' command.
 *
 * @returns Appropriate exit code.
 * @param   a                   Handler argument.
 */
RTEXITCODE handleList(HandlerArg *a)
{
    bool                fOptLong      = false;
    bool                fOptMultiple  = false;
    bool                fOptSorted    = false;
    enum enmListType    enmOptCommand = kListNotSpecified;

    static const RTGETOPTDEF s_aListOptions[] =
    {
        { "--long",             'l',                     RTGETOPT_REQ_NOTHING },
        { "--multiple",         'm',                     RTGETOPT_REQ_NOTHING }, /* not offical yet */
        { "--sorted",           's',                     RTGETOPT_REQ_NOTHING },
        { "vms",                kListVMs,                RTGETOPT_REQ_NOTHING },
        { "runningvms",         kListRunningVMs,         RTGETOPT_REQ_NOTHING },
        { "ostypes",            kListOsTypes,            RTGETOPT_REQ_NOTHING },
        { "hostdvds",           kListHostDvds,           RTGETOPT_REQ_NOTHING },
        { "hostfloppies",       kListHostFloppies,       RTGETOPT_REQ_NOTHING },
        { "intnets",            kListInternalNetworks,   RTGETOPT_REQ_NOTHING },
        { "hostifs",            kListBridgedInterfaces,  RTGETOPT_REQ_NOTHING }, /* backward compatibility */
        { "bridgedifs",         kListBridgedInterfaces,  RTGETOPT_REQ_NOTHING },
#if defined(VBOX_WITH_NETFLT)
        { "hostonlyifs",        kListHostOnlyInterfaces, RTGETOPT_REQ_NOTHING },
#endif
        { "natnetworks",        kListNatNetworks,        RTGETOPT_REQ_NOTHING },
        { "natnets",            kListNatNetworks,        RTGETOPT_REQ_NOTHING },
        { "hostinfo",           kListHostInfo,           RTGETOPT_REQ_NOTHING },
        { "hostcpuids",         kListHostCpuIDs,         RTGETOPT_REQ_NOTHING },
        { "hddbackends",        kListHddBackends,        RTGETOPT_REQ_NOTHING },
        { "hdds",               kListHdds,               RTGETOPT_REQ_NOTHING },
        { "dvds",               kListDvds,               RTGETOPT_REQ_NOTHING },
        { "floppies",           kListFloppies,           RTGETOPT_REQ_NOTHING },
        { "usbhost",            kListUsbHost,            RTGETOPT_REQ_NOTHING },
        { "usbfilters",         kListUsbFilters,         RTGETOPT_REQ_NOTHING },
        { "systemproperties",   kListSystemProperties,   RTGETOPT_REQ_NOTHING },
        { "dhcpservers",        kListDhcpServers,        RTGETOPT_REQ_NOTHING },
        { "extpacks",           kListExtPacks,           RTGETOPT_REQ_NOTHING },
        { "groups",             kListGroups,             RTGETOPT_REQ_NOTHING },
        { "webcams",            kListVideoInputDevices,  RTGETOPT_REQ_NOTHING },
        { "screenshotformats",  kListScreenShotFormats,  RTGETOPT_REQ_NOTHING },
    };

    int                 ch;
    RTGETOPTUNION       ValueUnion;
    RTGETOPTSTATE       GetState;
    RTGetOptInit(&GetState, a->argc, a->argv, s_aListOptions, RT_ELEMENTS(s_aListOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((ch = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (ch)
        {
            case 'l':  /* --long */
                fOptLong = true;
                break;

            case 's':
                fOptSorted = true;
                break;

            case 'm':
                fOptMultiple = true;
                if (enmOptCommand == kListNotSpecified)
                    break;
                ch = enmOptCommand;
                RT_FALL_THRU();

            case kListVMs:
            case kListRunningVMs:
            case kListOsTypes:
            case kListHostDvds:
            case kListHostFloppies:
            case kListInternalNetworks:
            case kListBridgedInterfaces:
#if defined(VBOX_WITH_NETFLT)
            case kListHostOnlyInterfaces:
#endif
            case kListHostInfo:
            case kListHostCpuIDs:
            case kListHddBackends:
            case kListHdds:
            case kListDvds:
            case kListFloppies:
            case kListUsbHost:
            case kListUsbFilters:
            case kListSystemProperties:
            case kListDhcpServers:
            case kListExtPacks:
            case kListGroups:
            case kListNatNetworks:
            case kListVideoInputDevices:
            case kListScreenShotFormats:
                enmOptCommand = (enum enmListType)ch;
                if (fOptMultiple)
                {
                    HRESULT hrc = produceList((enum enmListType)ch, fOptLong, fOptSorted, a->virtualBox);
                    if (FAILED(hrc))
                        return RTEXITCODE_FAILURE;
                }
                break;

            case VINF_GETOPT_NOT_OPTION:
                return errorSyntax(USAGE_LIST, "Unknown subcommand \"%s\".", ValueUnion.psz);

            default:
                return errorGetOpt(USAGE_LIST, ch, &ValueUnion);
        }
    }

    /*
     * If not in multiple list mode, we have to produce the list now.
     */
    if (enmOptCommand == kListNotSpecified)
        return errorSyntax(USAGE_LIST, "Missing subcommand for \"list\" command.\n");
    if (!fOptMultiple)
    {
        HRESULT hrc = produceList(enmOptCommand, fOptLong, fOptSorted, a->virtualBox);
        if (FAILED(hrc))
            return RTEXITCODE_FAILURE;
    }

    return RTEXITCODE_SUCCESS;
}

#endif /* !VBOX_ONLY_DOCS */
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
