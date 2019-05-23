/* $Id: NATEngineImpl.h $ */

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

#ifndef ____H_NATENGINE
#define ____H_NATENGINE

#include "NATEngineWrap.h"

namespace settings
{
    struct NAT;
}


class ATL_NO_VTABLE NATEngine :
    public NATEngineWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(NATEngine)

    HRESULT FinalConstruct();
    void FinalRelease();

    HRESULT init(Machine *aParent, INetworkAdapter *aAdapter);
    HRESULT init(Machine *aParent, INetworkAdapter *aAdapter, NATEngine *aThat);
    HRESULT initCopy(Machine *aParent, INetworkAdapter *aAdapter, NATEngine *aThat);
    void uninit();

    bool i_isModified();
    void i_rollback();
    void i_commit();
    void i_copyFrom(NATEngine *aThat);
    void i_applyDefaults();
    bool i_hasDefaults();
    HRESULT i_loadSettings(const settings::NAT &data);
    HRESULT i_saveSettings(settings::NAT &data);

private:

    // wrapped INATEngine properties
    HRESULT setNetwork(const com::Utf8Str &aNetwork);
    HRESULT getNetwork(com::Utf8Str &aNetwork);
    HRESULT setHostIP(const com::Utf8Str &aHostIP);
    HRESULT getHostIP(com::Utf8Str &aBindIP);
    /* TFTP properties */
    HRESULT setTFTPPrefix(const com::Utf8Str &aTFTPPrefix);
    HRESULT getTFTPPrefix(com::Utf8Str &aTFTPPrefix);
    HRESULT setTFTPBootFile(const com::Utf8Str &aTFTPBootFile);
    HRESULT getTFTPBootFile(com::Utf8Str &aTFTPBootFile);
    HRESULT setTFTPNextServer(const com::Utf8Str &aTFTPNextServer);
    HRESULT getTFTPNextServer(com::Utf8Str &aTFTPNextServer);
    /* DNS properties */
    HRESULT setDNSPassDomain(BOOL aDNSPassDomain);
    HRESULT getDNSPassDomain(BOOL *aDNSPassDomain);
    HRESULT setDNSProxy(BOOL aDNSProxy);
    HRESULT getDNSProxy(BOOL *aDNSProxy);
    HRESULT getDNSUseHostResolver(BOOL *aDNSUseHostResolver);
    HRESULT setDNSUseHostResolver(BOOL aDNSUseHostResolver);
    /* Alias properties */
    HRESULT setAliasMode(ULONG aAliasMode);
    HRESULT getAliasMode(ULONG *aAliasMode);

    HRESULT getRedirects(std::vector<com::Utf8Str> &aRedirects);

    HRESULT setNetworkSettings(ULONG aMtu,
                               ULONG aSockSnd,
                               ULONG aSockRcv,
                               ULONG aTcpWndSnd,
                               ULONG aTcpWndRcv);

    HRESULT getNetworkSettings(ULONG *aMtu,
                               ULONG *aSockSnd,
                               ULONG *aSockRcv,
                               ULONG *aTcpWndSnd,
                               ULONG *aTcpWndRcv);

    HRESULT addRedirect(const com::Utf8Str  &aName,
                              NATProtocol_T aProto,
                        const com::Utf8Str  &aHostIP,
                              USHORT        aHostPort,
                        const com::Utf8Str  &aGuestIP,
                              USHORT        aGuestPort);

    HRESULT removeRedirect(const com::Utf8Str &aName);

    struct Data;
    Data *mData;
    const ComObjPtr<NATEngine> mPeer;
    Machine * const mParent;
    INetworkAdapter * const mAdapter;
};
#endif
