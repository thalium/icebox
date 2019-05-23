/* $Id: USBDeviceImpl.h $ */

/** @file
 * Header file for the OUSBDevice (IUSBDevice) class, VBoxC.
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

#ifndef ____H_USBDEVICEIMPL
#define ____H_USBDEVICEIMPL

#include "USBDeviceWrap.h"

/**
 * Object class used for maintaining devices attached to a USB controller.
 * Generally this contains much less information.
 */
class ATL_NO_VTABLE OUSBDevice :
    public USBDeviceWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(OUSBDevice)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(IUSBDevice *a_pUSBDevice);
    void uninit();

    // public methods only for internal purposes
    const Guid &i_id() const { return mData.id; }

private:

    // Wrapped IUSBDevice properties
    HRESULT getId(com::Guid &aId);
    HRESULT getVendorId(USHORT *aVendorId);
    HRESULT getProductId(USHORT *aProductId);
    HRESULT getRevision(USHORT *aRevision);
    HRESULT getManufacturer(com::Utf8Str &aManufacturer);
    HRESULT getProduct(com::Utf8Str &aProduct);
    HRESULT getSerialNumber(com::Utf8Str &aSerialNumber);
    HRESULT getAddress(com::Utf8Str &aAddress);
    HRESULT getPort(USHORT *aPort);
    HRESULT getVersion(USHORT *aVersion);
    HRESULT getPortVersion(USHORT *aPortVersion);
    HRESULT getSpeed(USBConnectionSpeed_T *aSpeed);
    HRESULT getRemote(BOOL *aRemote);
    HRESULT getBackend(com::Utf8Str &aBackend);
    HRESULT getDeviceInfo(std::vector<com::Utf8Str> &aInfo);

    struct Data
    {
        Data() : vendorId(0), productId(0), revision(0), port(0),
                 version(1), portVersion(1), speed(USBConnectionSpeed_Null),
                 remote(FALSE) {}

        /** The UUID of this device. */
        const Guid id;

        /** The vendor id of this USB device. */
        const USHORT vendorId;
        /** The product id of this USB device. */
        const USHORT productId;
        /** The product revision number of this USB device.
         * (high byte = integer; low byte = decimal) */
        const USHORT revision;
        /** The Manufacturer string. (Quite possibly NULL.) */
        const com::Utf8Str manufacturer;
        /** The Product string. (Quite possibly NULL.) */
        const com::Utf8Str product;
        /** The SerialNumber string. (Quite possibly NULL.) */
        const com::Utf8Str serialNumber;
        /** The host specific address of the device. */
        const com::Utf8Str address;
        /** The device specific backend. */
        const com::Utf8Str backend;
        /** The host port number. */
        const USHORT port;
        /** The major USB version number of the device. */
        const USHORT version;
        /** The major USB version number of the port the device is attached to. */
        const USHORT portVersion;
        /** The speed at which the device is communicating. */
        const USBConnectionSpeed_T speed;
        /** Remote (VRDP) or local device. */
        const BOOL remote;
    };

    Data mData;
};

#endif // ____H_USBDEVICEIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
