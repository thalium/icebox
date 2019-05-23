/* $Id: RemoteUSBDeviceImpl.h $ */

/** @file
 *
 * VirtualBox IHostUSBDevice COM interface implementation
 * for remote (VRDP) USB devices
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

#ifndef ____H_REMOTEUSBDEVICEIMPL
#define ____H_REMOTEUSBDEVICEIMPL

#include "HostUSBDeviceWrap.h"

struct _VRDEUSBDEVICEDESC;
typedef _VRDEUSBDEVICEDESC VRDEUSBDEVICEDESC;

class ATL_NO_VTABLE RemoteUSBDevice :
    public HostUSBDeviceWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(RemoteUSBDevice)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(uint32_t u32ClientId, VRDEUSBDEVICEDESC *pDevDesc, bool fDescExt);
    void uninit();

    // public methods only for internal purposes
    bool dirty(void) const { return mData.dirty; }
    void dirty(bool aDirty) { mData.dirty = aDirty; }

    uint16_t devId(void) const { return mData.devId; }
    uint32_t clientId(void) { return mData.clientId; }

    bool captured(void) const { return mData.state == USBDeviceState_Captured; }
    void captured(bool aCaptured)
    {
        if (aCaptured)
        {
            Assert(mData.state == USBDeviceState_Available);
            mData.state = USBDeviceState_Captured;
        }
        else
        {
            Assert(mData.state == USBDeviceState_Captured);
            mData.state = USBDeviceState_Available;
        }
    }

private:

    // wrapped IUSBDevice properties
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

    // wrapped IHostUSBDevice properties
    HRESULT getState(USBDeviceState_T *aState);


    struct Data
    {
        Data() : vendorId(0), productId(0), revision(0), port(0), version(1),
                 portVersion(1), speed(USBConnectionSpeed_Null), dirty(FALSE),
                 devId(0), clientId(0) {}

        const Guid id;

        const uint16_t vendorId;
        const uint16_t productId;
        const uint16_t revision;

        const Utf8Str manufacturer;
        const Utf8Str product;
        const Utf8Str serialNumber;

        const Utf8Str address;
        const Utf8Str backend;

        const uint16_t port;
        const uint16_t version;
        const uint16_t portVersion;
        const USBConnectionSpeed_T speed;

        USBDeviceState_T state;
        bool dirty;

        const uint16_t devId;
        const uint32_t clientId;
    };

    Data mData;
};

#endif // ____H_REMOTEUSBDEVICEIMPL
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
