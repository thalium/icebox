/* $Id: VRDEServerImpl.h $ */

/** @file
 *
 * VirtualBox COM class implementation
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_VRDPSERVER
#define ____H_VRDPSERVER

#include "VRDEServerWrap.h"

namespace settings
{
    struct VRDESettings;
}

class ATL_NO_VTABLE VRDEServer :
    public VRDEServerWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(VRDEServer)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init(Machine *aParent);
    HRESULT init(Machine *aParent, VRDEServer *aThat);
    HRESULT initCopy(Machine *aParent, VRDEServer *aThat);
    void uninit();

    // public methods only for internal purposes
    HRESULT i_loadSettings(const settings::VRDESettings &data);
    HRESULT i_saveSettings(settings::VRDESettings &data);
    void i_rollback();
    void i_commit();
    void i_copyFrom(VRDEServer *aThat);

private:

     // wrapped IVRDEServer properties
     HRESULT getEnabled(BOOL *aEnabled);
     HRESULT setEnabled(BOOL aEnabled);
     HRESULT getAuthType(AuthType_T *aAuthType);
     HRESULT setAuthType(AuthType_T aAuthType);
     HRESULT getAuthTimeout(ULONG *aAuthTimeout);
     HRESULT setAuthTimeout(ULONG aAuthTimeout);
     HRESULT getAllowMultiConnection(BOOL *aAllowMultiConnection);
     HRESULT setAllowMultiConnection(BOOL aAllowMultiConnection);
     HRESULT getReuseSingleConnection(BOOL *aReuseSingleConnection);
     HRESULT setReuseSingleConnection(BOOL aReuseSingleConnection);
     HRESULT getVRDEExtPack(com::Utf8Str &aVRDEExtPack);
     HRESULT setVRDEExtPack(const com::Utf8Str &aVRDEExtPack);
     HRESULT getAuthLibrary(com::Utf8Str &aAuthLibrary);
     HRESULT setAuthLibrary(const com::Utf8Str &aAuthLibrary);
     HRESULT getVRDEProperties(std::vector<com::Utf8Str> &aVRDEProperties);

    // wrapped IVRDEServer methods
    HRESULT setVRDEProperty(const com::Utf8Str &aKey,
                            const com::Utf8Str &aValue);
    HRESULT getVRDEProperty(const com::Utf8Str &aKey,
                            com::Utf8Str &aValue);

    Machine * const     mParent;
    const ComObjPtr<VRDEServer> mPeer;

    Backupable<settings::VRDESettings> mData;
};

#endif // ____H_VRDPSERVER
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
