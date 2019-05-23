/* $Id: utils.h $ */
/** @file
 * ComHostUtils.cpp
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#ifndef _NETLIB_UTILS_H_
#define _NETLIB_UTILS_H_

#include "cpp/utils.h"

typedef ComPtr<IVirtualBox> ComVirtualBoxPtr;
typedef ComPtr<IVirtualBoxClient> ComVirtualBoxClientPtr;
typedef ComPtr<IDHCPServer> ComDhcpServerPtr;
typedef ComPtr<IHost> ComHostPtr;
typedef ComPtr<INATNetwork> ComNatPtr;
typedef com::SafeArray<BSTR> ComBstrArray;

typedef std::vector<RTNETADDRIPV4> AddressList;
typedef std::map<RTNETADDRIPV4, int> AddressToOffsetMapping;


inline bool isDhcpRequired(const ComNatPtr& nat)
{
    BOOL fNeedDhcpServer = false;
    if (FAILED(nat->COMGETTER(NeedDhcpServer)(&fNeedDhcpServer)))
        return false;

    return RT_BOOL(fNeedDhcpServer);
}


inline int findDhcpServer(const ComVirtualBoxPtr& vbox, const std::string& name, ComDhcpServerPtr& dhcp)
{
    HRESULT hrc = vbox->FindDHCPServerByNetworkName(com::Bstr(name.c_str()).raw(),
                                                          dhcp.asOutParam());
    AssertComRCReturn(hrc, VERR_NOT_FOUND);

    return VINF_SUCCESS;
}


inline int findNatNetwork(const ComVirtualBoxPtr& vbox, const std::string& name, ComNatPtr& nat)
{
    HRESULT hrc = vbox->FindNATNetworkByName(com::Bstr(name.c_str()).raw(),
                                                   nat.asOutParam());

    AssertComRCReturn(hrc, VERR_NOT_FOUND);

    return VINF_SUCCESS;
}


inline RTNETADDRIPV4 networkid(const RTNETADDRIPV4& addr, const RTNETADDRIPV4& netmask)
{
    RTNETADDRIPV4 netid;
    netid.u = addr.u & netmask.u;
    return netid;
}


int localMappings(const ComNatPtr&, AddressToOffsetMapping&);
int hostDnsSearchList(const ComHostPtr&, std::vector<std::string>&);
int hostDnsDomain(const ComHostPtr&, std::string& domainStr);


class NATNetworkEventAdapter
{
    public:
    virtual HRESULT HandleEvent(VBoxEventType_T aEventType, IEvent *pEvent) = 0;
};


class NATNetworkListener
{
    public:
    NATNetworkListener():m_pNAT(NULL){}

    HRESULT init(NATNetworkEventAdapter *pNAT)
    {
        AssertPtrReturn(pNAT, E_INVALIDARG);

        m_pNAT = pNAT;
        return S_OK;
    }

    HRESULT init()
    {
        m_pNAT = NULL;
        return S_OK;
    }

    void uninit() { m_pNAT = NULL; }

    HRESULT HandleEvent(VBoxEventType_T aEventType, IEvent *pEvent)
    {
        if (m_pNAT)
            return m_pNAT->HandleEvent(aEventType, pEvent);
        else
            return E_FAIL;
    }

    private:
    NATNetworkEventAdapter *m_pNAT;
};
typedef ListenerImpl<NATNetworkListener, NATNetworkEventAdapter*> NATNetworkListenerImpl;

# ifdef VBOX_WITH_XPCOM
class NS_CLASSINFO_NAME(NATNetworkListenerImpl);
# endif

typedef ComPtr<NATNetworkListenerImpl> ComNatListenerPtr;
typedef com::SafeArray<VBoxEventType_T> ComEventTypeArray;

/* XXX: const is commented out because of compilation erro on Windows host, but it's intended that this function
 isn't modify event type array */
int createNatListener(ComNatListenerPtr& listener, const ComVirtualBoxPtr& vboxptr,
                      NATNetworkEventAdapter *adapter, /* const */ ComEventTypeArray& events);
int destroyNatListener(ComNatListenerPtr& listener, const ComVirtualBoxPtr& vboxptr);
int createClientListener(ComNatListenerPtr& listener, const ComVirtualBoxClientPtr& vboxclientptr,
                         NATNetworkEventAdapter *adapter, /* const */ ComEventTypeArray& events);
int destroyClientListener(ComNatListenerPtr& listener, const ComVirtualBoxClientPtr& vboxclientptr);

#endif
