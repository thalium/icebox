/* $Id: NetworkAdapterImpl.h $ */

/** @file
 *
 * VirtualBox COM class implementation
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

#ifndef ____H_NETWORKADAPTER
#define ____H_NETWORKADAPTER

#include "NetworkAdapterWrap.h"

class GuestOSType;
class BandwidthControl;
class BandwidthGroup;
class NATEngine;

namespace settings
{
    struct NetworkAdapter;
}

class ATL_NO_VTABLE NetworkAdapter :
    public NetworkAdapterWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(NetworkAdapter)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Machine *aParent, ULONG aSlot);
    HRESULT init(Machine *aParent, NetworkAdapter *aThat, bool aReshare = false);
    HRESULT initCopy(Machine *aParent, NetworkAdapter *aThat);
    void uninit();

    // public methods only for internal purposes
    HRESULT i_loadSettings(BandwidthControl *bwctl, const settings::NetworkAdapter &data);
    HRESULT i_saveSettings(settings::NetworkAdapter &data);

    bool i_isModified();
    void i_rollback();
    void i_commit();
    void i_copyFrom(NetworkAdapter *aThat);
    void i_applyDefaults(GuestOSType *aOsType);
    bool i_hasDefaults();

    ComObjPtr<NetworkAdapter> i_getPeer();

private:

    // wrapped INetworkAdapter properties
    HRESULT getAdapterType(NetworkAdapterType_T *aAdapterType);
    HRESULT setAdapterType(NetworkAdapterType_T aAdapterType);
    HRESULT getSlot(ULONG *aSlot);
    HRESULT getEnabled(BOOL *aEnabled);
    HRESULT setEnabled(BOOL aEnabled);
    HRESULT getMACAddress(com::Utf8Str &aMACAddress);
    HRESULT setMACAddress(const com::Utf8Str &aMACAddress);
    HRESULT getAttachmentType(NetworkAttachmentType_T *aAttachmentType);
    HRESULT setAttachmentType(NetworkAttachmentType_T aAttachmentType);
    HRESULT getBridgedInterface(com::Utf8Str &aBridgedInterface);
    HRESULT setBridgedInterface(const com::Utf8Str &aBridgedInterface);
    HRESULT getHostOnlyInterface(com::Utf8Str &aHostOnlyInterface);
    HRESULT setHostOnlyInterface(const com::Utf8Str &aHostOnlyInterface);
    HRESULT getInternalNetwork(com::Utf8Str &aInternalNetwork);
    HRESULT setInternalNetwork(const com::Utf8Str &aInternalNetwork);
    HRESULT getNATNetwork(com::Utf8Str &aNATNetwork);
    HRESULT setNATNetwork(const com::Utf8Str &aNATNetwork);
    HRESULT getGenericDriver(com::Utf8Str &aGenericDriver);
    HRESULT setGenericDriver(const com::Utf8Str &aGenericDriver);
    HRESULT getCableConnected(BOOL *aCableConnected);
    HRESULT setCableConnected(BOOL aCableConnected);
    HRESULT getLineSpeed(ULONG *aLineSpeed);
    HRESULT setLineSpeed(ULONG aLineSpeed);
    HRESULT getPromiscModePolicy(NetworkAdapterPromiscModePolicy_T *aPromiscModePolicy);
    HRESULT setPromiscModePolicy(NetworkAdapterPromiscModePolicy_T aPromiscModePolicy);
    HRESULT getTraceEnabled(BOOL *aTraceEnabled);
    HRESULT setTraceEnabled(BOOL aTraceEnabled);
    HRESULT getTraceFile(com::Utf8Str &aTraceFile);
    HRESULT setTraceFile(const com::Utf8Str &aTraceFile);
    HRESULT getNATEngine(ComPtr<INATEngine> &aNATEngine);
    HRESULT getBootPriority(ULONG *aBootPriority);
    HRESULT setBootPriority(ULONG aBootPriority);
    HRESULT getBandwidthGroup(ComPtr<IBandwidthGroup> &aBandwidthGroup);
    HRESULT setBandwidthGroup(const ComPtr<IBandwidthGroup> &aBandwidthGroup);

    // wrapped INetworkAdapter methods
    HRESULT getProperty(const com::Utf8Str &aKey,
                        com::Utf8Str &aValue);
    HRESULT setProperty(const com::Utf8Str &aKey,
                        const com::Utf8Str &aValue);
    HRESULT getProperties(const com::Utf8Str &aNames,
                          std::vector<com::Utf8Str> &aReturnNames,
                          std::vector<com::Utf8Str> &aReturnValues);
    // Misc.
    void i_generateMACAddress();
    HRESULT i_updateMacAddress(Utf8Str aMacAddress);
    void i_updateBandwidthGroup(BandwidthGroup *aBwGroup);
    HRESULT i_switchFromNatNetworking(const com::Utf8Str &aNatnetworkName);
    HRESULT i_switchToNatNetworking(const com::Utf8Str &aNatNetworkName);


    Machine * const     mParent;
    const ComObjPtr<NetworkAdapter> mPeer;
    const ComObjPtr<NATEngine> mNATEngine;

    Backupable<settings::NetworkAdapter> mData;
};

#endif // ____H_NETWORKADAPTER
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
