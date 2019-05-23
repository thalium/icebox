/* $Id: VirtualBoxClientImpl.h $ */

/** @file
 * Header file for the VirtualBoxClient (IVirtualBoxClient) class, VBoxC.
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
 */

#ifndef ____H_VIRTUALBOXCLIENTIMPL
#define ____H_VIRTUALBOXCLIENTIMPL

#include "VirtualBoxClientWrap.h"
#include "EventImpl.h"

#ifdef RT_OS_WINDOWS
# include "win/resource.h"
#endif

class ATL_NO_VTABLE VirtualBoxClient :
    public VirtualBoxClientWrap
#ifdef RT_OS_WINDOWS
    , public ATL::CComCoClass<VirtualBoxClient, &CLSID_VirtualBoxClient>
#endif
{
public:
    DECLARE_CLASSFACTORY_SINGLETON(VirtualBoxClient)

    // Do not use any ATL registry support.
    //DECLARE_REGISTRY_RESOURCEID(IDR_VIRTUALBOX)

    DECLARE_NOT_AGGREGATABLE(VirtualBoxClient)

    HRESULT FinalConstruct();
    void FinalRelease();

    // public initializer/uninitializer for internal purposes only
    HRESULT init();
    void uninit();

#ifdef RT_OS_WINDOWS
    /* HACK ALERT! Implemented in dllmain.cpp. */
    ULONG InternalRelease();
#endif

private:
    // wrapped IVirtualBoxClient properties
    virtual HRESULT getVirtualBox(ComPtr<IVirtualBox> &aVirtualBox);
    virtual HRESULT getSession(ComPtr<ISession> &aSession);
    virtual HRESULT getEventSource(ComPtr<IEventSource> &aEventSource);

    // wrapped IVirtualBoxClient methods
    virtual HRESULT checkMachineError(const ComPtr<IMachine> &aMachine);

    /** Instance counter for simulating something similar to a singleton.
     * Only the first instance will be a usable object, all additional
     * instances will return a failure at creation time and will not work. */
    static uint32_t g_cInstances;

#ifdef RT_OS_WINDOWS
    virtual HRESULT i_investigateVirtualBoxObjectCreationFailure(HRESULT hrc);
#endif

#ifdef VBOX_WITH_SDS
    int     i_getServiceAccountAndStartType(const wchar_t *pwszServiceName,
                                            wchar_t *pwszAccountName, size_t cwcAccountName, uint32_t *puStartType);
#endif

    static DECLCALLBACK(int) SVCWatcherThread(RTTHREAD ThreadSelf, void *pvUser);

    struct Data
    {
        Data() : m_ThreadWatcher(NIL_RTTHREAD), m_SemEvWatcher(NIL_RTSEMEVENT)
        {}

        ~Data()
        {
            /* HACK ALERT! This is for DllCanUnloadNow(). */
            if (m_pEventSource.isNotNull())
            {
                s_cUnnecessaryAtlModuleLocks--;
                AssertMsg(s_cUnnecessaryAtlModuleLocks == 0, ("%d\n", s_cUnnecessaryAtlModuleLocks));
            }
        }

        ComPtr<IVirtualBox> m_pVirtualBox;
        ComPtr<IToken> m_pToken;
        const ComObjPtr<EventSource> m_pEventSource;

        RTTHREAD m_ThreadWatcher;
        RTSEMEVENT m_SemEvWatcher;
    };

    Data mData;

public:
    /** Hack for discounting the AtlModule lock held by Data::m_pEventSource during
     * DllCanUnloadNow().  This is incremented to 1 when init() initialized
     * m_pEventSource and is decremented by the Data destructor (above). */
    static LONG s_cUnnecessaryAtlModuleLocks;
};

#endif
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
