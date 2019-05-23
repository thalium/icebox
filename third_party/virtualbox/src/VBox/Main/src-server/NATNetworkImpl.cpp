/* $Id: NATNetworkImpl.cpp $ */
/** @file
 * INATNetwork implementation.
 */

/*
 * Copyright (C) 2013-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <string>
#include "NetworkServiceRunner.h"
#include "DHCPServerImpl.h"
#include "NATNetworkImpl.h"
#include "AutoCaller.h"
#include "Logging.h"

#include <iprt/asm.h>
#include <iprt/cpp/utils.h>
#include <iprt/cidr.h>
#include <iprt/net.h>
#include <VBox/com/array.h>
#include <VBox/com/ptr.h>
#include <VBox/settings.h>

#include "EventImpl.h"

#include "VirtualBoxImpl.h"
#include <algorithm>
#include <list>

#ifndef RT_OS_WINDOWS
# include <netinet/in.h>
#else
# define IN_LOOPBACKNET 127
#endif


// constructor / destructor
/////////////////////////////////////////////////////////////////////////////
struct NATNetwork::Data
{
    Data()
      : pVirtualBox(NULL)
      , offGateway(0)
      , offDhcp(0)
    {
    }
    virtual ~Data(){}
    const ComObjPtr<EventSource> pEventSource;
#ifdef VBOX_WITH_NAT_SERVICE
    NATNetworkServiceRunner NATRunner;
    ComObjPtr<IDHCPServer> dhcpServer;
#endif
    /** weak VirtualBox parent */
    VirtualBox * const pVirtualBox;

    /** NATNetwork settings */
    settings::NATNetwork s;

    com::Utf8Str IPv4Gateway;
    com::Utf8Str IPv4NetworkMask;
    com::Utf8Str IPv4DhcpServer;
    com::Utf8Str IPv4DhcpServerLowerIp;
    com::Utf8Str IPv4DhcpServerUpperIp;

    uint32_t offGateway;
    uint32_t offDhcp;
};


NATNetwork::NATNetwork()
    : m(NULL)
{
}


NATNetwork::~NATNetwork()
{
}


HRESULT NATNetwork::FinalConstruct()
{
    return BaseFinalConstruct();
}


void NATNetwork::FinalRelease()
{
    uninit();

    BaseFinalRelease();
}


void NATNetwork::uninit()
{
    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;
    unconst(m->pVirtualBox) = NULL;
    delete m;
    m = NULL;
}

HRESULT NATNetwork::init(VirtualBox *aVirtualBox, com::Utf8Str aName)
{
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m = new Data();
    /* share VirtualBox weakly */
    unconst(m->pVirtualBox) = aVirtualBox;
    m->s.strNetworkName = aName;
    m->s.strIPv4NetworkCidr = "10.0.2.0/24";
    m->offGateway = 1;
    i_recalculateIPv6Prefix();  /* set m->strIPv6Prefix based on IPv4 */

    settings::NATHostLoopbackOffset off;
    off.strLoopbackHostAddress = "127.0.0.1";
    off.u32Offset = (uint32_t)2;
    m->s.llHostLoopbackOffsetList.push_back(off);

    i_recalculateIpv4AddressAssignments();

    HRESULT hrc = unconst(m->pEventSource).createObject();
    if (FAILED(hrc)) throw hrc;

    hrc = m->pEventSource->init();
    if (FAILED(hrc)) throw hrc;

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}


HRESULT NATNetwork::i_loadSettings(const settings::NATNetwork &data)
{
    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    m->s = data;
    if (   m->s.strIPv6Prefix.isEmpty()
           /* also clean up bogus old default */
        || m->s.strIPv6Prefix == "fe80::/64")
        i_recalculateIPv6Prefix(); /* set m->strIPv6Prefix based on IPv4 */
    i_recalculateIpv4AddressAssignments();

    return S_OK;
}

HRESULT NATNetwork::i_saveSettings(settings::NATNetwork &data)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    AssertReturn(!m->s.strNetworkName.isEmpty(), E_FAIL);
    data = m->s;

    m->pVirtualBox->i_onNATNetworkSetting(Bstr(m->s.strNetworkName).raw(),
                                          m->s.fEnabled,
                                          Bstr(m->s.strIPv4NetworkCidr).raw(),
                                          Bstr(m->IPv4Gateway).raw(),
                                          m->s.fAdvertiseDefaultIPv6Route,
                                          m->s.fNeedDhcpServer);

    /* Notify listerners listening on this network only */
    fireNATNetworkSettingEvent(m->pEventSource,
                               Bstr(m->s.strNetworkName).raw(),
                               m->s.fEnabled,
                               Bstr(m->s.strIPv4NetworkCidr).raw(),
                               Bstr(m->IPv4Gateway).raw(),
                               m->s.fAdvertiseDefaultIPv6Route,
                               m->s.fNeedDhcpServer);

    return S_OK;
}

HRESULT NATNetwork::getEventSource(ComPtr<IEventSource> &aEventSource)
{
    /* event source is const, no need to lock */
    m->pEventSource.queryInterfaceTo(aEventSource.asOutParam());
    return S_OK;
}

HRESULT NATNetwork::getNetworkName(com::Utf8Str &aNetworkName)
{
    AssertReturn(!m->s.strNetworkName.isEmpty(), E_FAIL);
    aNetworkName = m->s.strNetworkName;
    return S_OK;
}

HRESULT NATNetwork::setNetworkName(const com::Utf8Str &aNetworkName)
{
    if (m->s.strNetworkName.isEmpty())
        return setError(E_INVALIDARG,
                        tr("Network name cannot be empty"));
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (aNetworkName == m->s.strNetworkName)
            return S_OK;

        m->s.strNetworkName = aNetworkName;
    }
    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);

    return S_OK;
}

HRESULT NATNetwork::getEnabled(BOOL *aEnabled)
{
    *aEnabled = m->s.fEnabled;

    i_recalculateIpv4AddressAssignments();
    return S_OK;
}

HRESULT NATNetwork::setEnabled(const BOOL aEnabled)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        if (RT_BOOL(aEnabled) == m->s.fEnabled)
            return S_OK;
        m->s.fEnabled = RT_BOOL(aEnabled);
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}

HRESULT NATNetwork::getGateway(com::Utf8Str &aIPv4Gateway)
{
    aIPv4Gateway = m->IPv4Gateway;
    return S_OK;
}

HRESULT NATNetwork::getNetwork(com::Utf8Str &aNetwork)
{
    aNetwork = m->s.strIPv4NetworkCidr;
    return S_OK;
}


HRESULT NATNetwork::setNetwork(const com::Utf8Str &aIPv4NetworkCidr)
{
    {

        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        if (aIPv4NetworkCidr == m->s.strIPv4NetworkCidr)
            return S_OK;

        /* silently ignore network cidr update for now.
         * todo: keep internally guest address of port forward rule
         * as offset from network id.
         */
        if (!m->s.mapPortForwardRules4.empty())
            return S_OK;


        m->s.strIPv4NetworkCidr = aIPv4NetworkCidr;
        i_recalculateIpv4AddressAssignments();
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);
    return S_OK;
}


HRESULT NATNetwork::getIPv6Enabled(BOOL *aIPv6Enabled)
{
    *aIPv6Enabled = m->s.fIPv6Enabled;

    return S_OK;
}


HRESULT NATNetwork::setIPv6Enabled(const BOOL aIPv6Enabled)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        if (RT_BOOL(aIPv6Enabled) == m->s.fIPv6Enabled)
            return S_OK;

        m->s.fIPv6Enabled = RT_BOOL(aIPv6Enabled);
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);

    return S_OK;
}


HRESULT NATNetwork::getIPv6Prefix(com::Utf8Str &aIPv6Prefix)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aIPv6Prefix = m->s.strIPv6Prefix;
    return S_OK;
}

HRESULT NATNetwork::setIPv6Prefix(const com::Utf8Str &aIPv6Prefix)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        if (aIPv6Prefix == m->s.strIPv6Prefix)
            return S_OK;

        /* silently ignore network IPv6 prefix update.
         * todo: see similar todo in NATNetwork::COMSETTER(Network)(IN_BSTR)
         */
        if (!m->s.mapPortForwardRules6.empty())
            return S_OK;

        m->s.strIPv6Prefix = aIPv6Prefix;
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);

    return S_OK;
}


HRESULT NATNetwork::getAdvertiseDefaultIPv6RouteEnabled(BOOL *aAdvertiseDefaultIPv6Route)
{
    *aAdvertiseDefaultIPv6Route = m->s.fAdvertiseDefaultIPv6Route;

    return S_OK;
}


HRESULT NATNetwork::setAdvertiseDefaultIPv6RouteEnabled(const BOOL aAdvertiseDefaultIPv6Route)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        if (RT_BOOL(aAdvertiseDefaultIPv6Route) == m->s.fAdvertiseDefaultIPv6Route)
            return S_OK;

        m->s.fAdvertiseDefaultIPv6Route = RT_BOOL(aAdvertiseDefaultIPv6Route);

    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);

    return S_OK;
}


HRESULT NATNetwork::getNeedDhcpServer(BOOL *aNeedDhcpServer)
{
    *aNeedDhcpServer = m->s.fNeedDhcpServer;

    return S_OK;
}

HRESULT NATNetwork::setNeedDhcpServer(const BOOL aNeedDhcpServer)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        if (RT_BOOL(aNeedDhcpServer) == m->s.fNeedDhcpServer)
            return S_OK;

        m->s.fNeedDhcpServer = RT_BOOL(aNeedDhcpServer);

        i_recalculateIpv4AddressAssignments();

    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = m->pVirtualBox->i_saveSettings();
    ComAssertComRCRetRC(rc);

    return S_OK;
}

HRESULT NATNetwork::getLocalMappings(std::vector<com::Utf8Str> &aLocalMappings)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aLocalMappings.resize(m->s.llHostLoopbackOffsetList.size());
    size_t i = 0;
    for (settings::NATLoopbackOffsetList::const_iterator it = m->s.llHostLoopbackOffsetList.begin();
         it != m->s.llHostLoopbackOffsetList.end(); ++it, ++i)
    {
        aLocalMappings[i] = Utf8StrFmt("%s=%d",
                            (*it).strLoopbackHostAddress.c_str(),
                            (*it).u32Offset);
    }

    return S_OK;
}

HRESULT NATNetwork::addLocalMapping(const com::Utf8Str &aHostId, LONG aOffset)
{
    RTNETADDRIPV4 addr, net, mask;

    int rc = RTNetStrToIPv4Addr(Utf8Str(aHostId).c_str(), &addr);
    if (RT_FAILURE(rc))
        return E_INVALIDARG;

    /* check against 127/8 */
    if ((RT_N2H_U32(addr.u) >> IN_CLASSA_NSHIFT) != IN_LOOPBACKNET)
        return E_INVALIDARG;

    /* check against networkid vs network mask */
    rc = RTCidrStrToIPv4(Utf8Str(m->s.strIPv4NetworkCidr).c_str(), &net, &mask);
    if (RT_FAILURE(rc))
        return E_INVALIDARG;

    if (((net.u + aOffset) & mask.u) != net.u)
        return E_INVALIDARG;

    settings::NATLoopbackOffsetList::iterator it;

    it = std::find(m->s.llHostLoopbackOffsetList.begin(),
                   m->s.llHostLoopbackOffsetList.end(),
                   aHostId);
    if (it != m->s.llHostLoopbackOffsetList.end())
    {
        if (aOffset == 0) /* erase */
            m->s.llHostLoopbackOffsetList.erase(it, it);
        else /* modify */
        {
            settings::NATLoopbackOffsetList::iterator it1;
            it1 = std::find(m->s.llHostLoopbackOffsetList.begin(),
                            m->s.llHostLoopbackOffsetList.end(),
                            (uint32_t)aOffset);
            if (it1 != m->s.llHostLoopbackOffsetList.end())
                return E_INVALIDARG; /* this offset is already registered. */

            (*it).u32Offset = aOffset;
        }

        AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
        return m->pVirtualBox->i_saveSettings();
    }

    /* injection */
    it = std::find(m->s.llHostLoopbackOffsetList.begin(),
                   m->s.llHostLoopbackOffsetList.end(),
                   (uint32_t)aOffset);

    if (it != m->s.llHostLoopbackOffsetList.end())
        return E_INVALIDARG; /* offset is already registered. */

    settings::NATHostLoopbackOffset off;
    off.strLoopbackHostAddress = aHostId;
    off.u32Offset = (uint32_t)aOffset;
    m->s.llHostLoopbackOffsetList.push_back(off);

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    return m->pVirtualBox->i_saveSettings();
}


HRESULT NATNetwork::getLoopbackIp6(LONG *aLoopbackIp6)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aLoopbackIp6 = m->s.u32HostLoopback6Offset;
    return S_OK;
}


HRESULT NATNetwork::setLoopbackIp6(LONG aLoopbackIp6)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        if (aLoopbackIp6 < 0)
            return E_INVALIDARG;

        if (static_cast<uint32_t>(aLoopbackIp6) == m->s.u32HostLoopback6Offset)
            return S_OK;

        m->s.u32HostLoopback6Offset = aLoopbackIp6;
    }

    AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
    return m->pVirtualBox->i_saveSettings();
}


HRESULT NATNetwork::getPortForwardRules4(std::vector<com::Utf8Str> &aPortForwardRules4)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    i_getPortForwardRulesFromMap(aPortForwardRules4,
                                 m->s.mapPortForwardRules4);
    return S_OK;
}

HRESULT NATNetwork::getPortForwardRules6(std::vector<com::Utf8Str> &aPortForwardRules6)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    i_getPortForwardRulesFromMap(aPortForwardRules6,
                                 m->s.mapPortForwardRules6);
    return S_OK;
}

HRESULT NATNetwork::addPortForwardRule(BOOL aIsIpv6,
                                       const com::Utf8Str &aPortForwardRuleName,
                                       NATProtocol_T aProto,
                                       const com::Utf8Str &aHostIp,
                                       USHORT aHostPort,
                                       const com::Utf8Str &aGuestIp,
                                       USHORT aGuestPort)
{
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        Utf8Str name = aPortForwardRuleName;
        Utf8Str proto;
        settings::NATRule r;
        settings::NATRulesMap &mapRules = aIsIpv6 ? m->s.mapPortForwardRules6 : m->s.mapPortForwardRules4;
        switch (aProto)
        {
            case NATProtocol_TCP:
                proto = "tcp";
                break;
            case NATProtocol_UDP:
                proto = "udp";
                break;
            default:
                return E_INVALIDARG;
        }
        if (name.isEmpty())
            name = Utf8StrFmt("%s_[%s]%%%d_[%s]%%%d", proto.c_str(),
                              aHostIp.c_str(), aHostPort,
                              aGuestIp.c_str(), aGuestPort);

        for (settings::NATRulesMap::iterator it = mapRules.begin(); it != mapRules.end(); ++it)
        {
            r = it->second;
            if (it->first == name)
                return setError(E_INVALIDARG,
                                tr("A NAT rule of this name already exists"));
            if (   r.strHostIP == aHostIp
                   && r.u16HostPort == aHostPort
                   && r.proto == aProto)
                return setError(E_INVALIDARG,
                                tr("A NAT rule for this host port and this host IP already exists"));
        }

        r.strName = name.c_str();
        r.proto = aProto;
        r.strHostIP = aHostIp;
        r.u16HostPort = aHostPort;
        r.strGuestIP = aGuestIp;
        r.u16GuestPort = aGuestPort;
        mapRules.insert(std::make_pair(name, r));
    }
    {
        AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
        HRESULT rc = m->pVirtualBox->i_saveSettings();
        ComAssertComRCRetRC(rc);
    }

    m->pVirtualBox->i_onNATNetworkPortForward(Bstr(m->s.strNetworkName).raw(), TRUE, aIsIpv6,
                                              Bstr(aPortForwardRuleName).raw(), aProto,
                                              Bstr(aHostIp).raw(), aHostPort,
                                              Bstr(aGuestIp).raw(), aGuestPort);

    /* Notify listerners listening on this network only */
    fireNATNetworkPortForwardEvent(m->pEventSource, Bstr(m->s.strNetworkName).raw(), TRUE,
                                   aIsIpv6, Bstr(aPortForwardRuleName).raw(), aProto,
                                   Bstr(aHostIp).raw(), aHostPort,
                                   Bstr(aGuestIp).raw(), aGuestPort);

    return S_OK;
}

HRESULT NATNetwork::removePortForwardRule(BOOL aIsIpv6, const com::Utf8Str &aPortForwardRuleName)
{
    Utf8Str strHostIP;
    Utf8Str strGuestIP;
    uint16_t u16HostPort;
    uint16_t u16GuestPort;
    NATProtocol_T proto;

    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        settings::NATRulesMap &mapRules = aIsIpv6 ? m->s.mapPortForwardRules6 : m->s.mapPortForwardRules4;
        settings::NATRulesMap::iterator it = mapRules.find(aPortForwardRuleName);

        if (it == mapRules.end())
            return E_INVALIDARG;

        strHostIP = it->second.strHostIP;
        strGuestIP = it->second.strGuestIP;
        u16HostPort = it->second.u16HostPort;
        u16GuestPort = it->second.u16GuestPort;
        proto = it->second.proto;

        mapRules.erase(it);
    }

    {
        AutoWriteLock vboxLock(m->pVirtualBox COMMA_LOCKVAL_SRC_POS);
        HRESULT rc = m->pVirtualBox->i_saveSettings();
        ComAssertComRCRetRC(rc);
    }

    m->pVirtualBox->i_onNATNetworkPortForward(Bstr(m->s.strNetworkName).raw(), FALSE, aIsIpv6,
                                              Bstr(aPortForwardRuleName).raw(), proto,
                                              Bstr(strHostIP).raw(), u16HostPort,
                                              Bstr(strGuestIP).raw(), u16GuestPort);

    /* Notify listerners listening on this network only */
    fireNATNetworkPortForwardEvent(m->pEventSource, Bstr(m->s.strNetworkName).raw(), FALSE,
                                   aIsIpv6, Bstr(aPortForwardRuleName).raw(), proto,
                                   Bstr(strHostIP).raw(), u16HostPort,
                                   Bstr(strGuestIP).raw(), u16GuestPort);
    return S_OK;
}


HRESULT  NATNetwork::start(const com::Utf8Str &aTrunkType)
{
#ifdef VBOX_WITH_NAT_SERVICE
    if (!m->s.fEnabled) return S_OK;
    AssertReturn(!m->s.strNetworkName.isEmpty(), E_FAIL);

    m->NATRunner.setOption(NetworkServiceRunner::kNsrKeyNetwork, Utf8Str(m->s.strNetworkName).c_str());
    m->NATRunner.setOption(NetworkServiceRunner::kNsrKeyTrunkType, Utf8Str(aTrunkType).c_str());
    m->NATRunner.setOption(NetworkServiceRunner::kNsrIpAddress, Utf8Str(m->IPv4Gateway).c_str());
    m->NATRunner.setOption(NetworkServiceRunner::kNsrIpNetmask, Utf8Str(m->IPv4NetworkMask).c_str());

    /* No portforwarding rules from command-line, all will be fetched via API */

    if (m->s.fNeedDhcpServer)
    {
        /*
         * Just to as idea... via API (on creation user pass the cidr of network and)
         * and we calculate it's addreses (mutable?).
         */

        /*
         * Configuration and running DHCP server:
         * 1. find server first createDHCPServer
         * 2. if return status is E_INVALARG => server already exists just find and start.
         * 3. if return status neither E_INVALRG nor S_OK => return E_FAIL
         * 4. if return status S_OK proceed to DHCP server configuration
         * 5. call setConfiguration() and pass all required parameters
         * 6. start dhcp server.
         */
        HRESULT hrc = m->pVirtualBox->FindDHCPServerByNetworkName(Bstr(m->s.strNetworkName).raw(),
                                                                  m->dhcpServer.asOutParam());
        switch (hrc)
        {
            case E_INVALIDARG:
                /* server haven't beeen found let create it then */
                hrc = m->pVirtualBox->CreateDHCPServer(Bstr(m->s.strNetworkName).raw(),
                                                       m->dhcpServer.asOutParam());
                if (FAILED(hrc))
                  return E_FAIL;
                /* breakthrough */

            {
                LogFunc(("gateway: %s, dhcpserver:%s, dhcplowerip:%s, dhcpupperip:%s\n",
                         m->IPv4Gateway.c_str(),
                         m->IPv4DhcpServer.c_str(),
                         m->IPv4DhcpServerLowerIp.c_str(),
                         m->IPv4DhcpServerUpperIp.c_str()));

                hrc = m->dhcpServer->COMSETTER(Enabled)(true);

                BSTR dhcpip = NULL;
                BSTR netmask = NULL;
                BSTR lowerip = NULL;
                BSTR upperip = NULL;

                m->IPv4DhcpServer.cloneTo(&dhcpip);
                m->IPv4NetworkMask.cloneTo(&netmask);
                m->IPv4DhcpServerLowerIp.cloneTo(&lowerip);
                m->IPv4DhcpServerUpperIp.cloneTo(&upperip);
                hrc = m->dhcpServer->SetConfiguration(dhcpip,
                                                     netmask,
                                                     lowerip,
                                                     upperip);
            }
            case S_OK:
                break;

            default:
                return E_FAIL;
        }

        /* XXX: AddGlobalOption(DhcpOpt_Router,) - enables attachement of DhcpServer to Main. */
        m->dhcpServer->AddGlobalOption(DhcpOpt_Router, Bstr(m->IPv4Gateway).raw());

        hrc = m->dhcpServer->Start(Bstr(m->s.strNetworkName).raw(), Bstr("").raw(), Bstr(aTrunkType).raw());
        if (FAILED(hrc))
        {
            m->dhcpServer.setNull();
            return E_FAIL;
        }
    }

    if (RT_SUCCESS(m->NATRunner.start(false /* KillProcOnStop */)))
    {
        m->pVirtualBox->i_onNATNetworkStartStop(Bstr(m->s.strNetworkName).raw(), TRUE);
        return S_OK;
    }
    /** @todo missing setError()! */
    return E_FAIL;
#else
    NOREF(aTrunkType);
    ReturnComNotImplemented();
#endif
}

HRESULT NATNetwork::stop()
{
#ifdef VBOX_WITH_NAT_SERVICE
    m->pVirtualBox->i_onNATNetworkStartStop(Bstr(m->s.strNetworkName).raw(), FALSE);

    if (!m->dhcpServer.isNull())
        m->dhcpServer->Stop();

    if (RT_SUCCESS(m->NATRunner.stop()))
        return S_OK;

    /** @todo missing setError()! */
    return E_FAIL;
#else
    ReturnComNotImplemented();
#endif
}


void NATNetwork::i_getPortForwardRulesFromMap(std::vector<com::Utf8Str> &aPortForwardRules, settings::NATRulesMap &aRules)
{
    aPortForwardRules.resize(aRules.size());
    size_t i = 0;
    for (settings::NATRulesMap::const_iterator it = aRules.begin();
         it != aRules.end(); ++it, ++i)
    {
        settings::NATRule r = it->second;
        aPortForwardRules[i] =  Utf8StrFmt("%s:%s:[%s]:%d:[%s]:%d",
                                           r.strName.c_str(),
                                           (r.proto == NATProtocol_TCP ? "tcp" : "udp"),
                                           r.strHostIP.c_str(),
                                           r.u16HostPort,
                                           r.strGuestIP.c_str(),
                                           r.u16GuestPort);
    }
}


int NATNetwork::i_findFirstAvailableOffset(ADDRESSLOOKUPTYPE addrType, uint32_t *poff)
{
    RTNETADDRIPV4 network, netmask;

    int rc = RTCidrStrToIPv4(m->s.strIPv4NetworkCidr.c_str(),
                             &network,
                             &netmask);
    AssertRCReturn(rc, rc);

    uint32_t off;
    for (off = 1; off < ~netmask.u; ++off)
    {
        bool skip = false;
        for (settings::NATLoopbackOffsetList::iterator it = m->s.llHostLoopbackOffsetList.begin();
             it != m->s.llHostLoopbackOffsetList.end();
             ++it)
        {
            if ((*it).u32Offset == off)
            {
                skip = true;
                break;
            }

        }

        if (skip)
            continue;

        if (off == m->offGateway)
        {
            if (addrType == ADDR_GATEWAY)
                break;
            else
                continue;
        }

        if (off == m->offDhcp)
        {
            if (addrType == ADDR_DHCP)
                break;
            else
                continue;
        }

        if (!skip)
            break;
    }

    if (poff)
        *poff = off;

    return VINF_SUCCESS;
}

int NATNetwork::i_recalculateIpv4AddressAssignments()
{
    RTNETADDRIPV4 network, netmask;
    int rc = RTCidrStrToIPv4(m->s.strIPv4NetworkCidr.c_str(),
                             &network,
                             &netmask);
    AssertRCReturn(rc, rc);

    i_findFirstAvailableOffset(ADDR_GATEWAY, &m->offGateway);
    if (m->s.fNeedDhcpServer)
        i_findFirstAvailableOffset(ADDR_DHCP, &m->offDhcp);

    /* I don't remember the reason CIDR calculated on the host. */
    RTNETADDRIPV4 gateway = network;
    gateway.u += m->offGateway;
    gateway.u = RT_H2N_U32(gateway.u);
    char szTmpIp[16];
    RTStrPrintf(szTmpIp, sizeof(szTmpIp), "%RTnaipv4", gateway);
    m->IPv4Gateway = szTmpIp;

    if (m->s.fNeedDhcpServer)
    {
        RTNETADDRIPV4 dhcpserver = network;
        dhcpserver.u += m->offDhcp;

        /* XXX: adding more services should change the math here */
        RTNETADDRIPV4 dhcplowerip = network;
        uint32_t offDhcpLowerIp;
        i_findFirstAvailableOffset(ADDR_DHCPLOWERIP, &offDhcpLowerIp);
        dhcplowerip.u = RT_H2N_U32(dhcplowerip.u + offDhcpLowerIp);

        RTNETADDRIPV4 dhcpupperip;
        dhcpupperip.u = RT_H2N_U32((network.u | ~netmask.u) - 1);

        dhcpserver.u = RT_H2N_U32(dhcpserver.u);
        network.u = RT_H2N_U32(network.u);

        RTStrPrintf(szTmpIp, sizeof(szTmpIp), "%RTnaipv4", dhcpserver);
        m->IPv4DhcpServer = szTmpIp;
        RTStrPrintf(szTmpIp, sizeof(szTmpIp), "%RTnaipv4", dhcplowerip);
        m->IPv4DhcpServerLowerIp = szTmpIp;
        RTStrPrintf(szTmpIp, sizeof(szTmpIp), "%RTnaipv4", dhcpupperip);
        m->IPv4DhcpServerUpperIp = szTmpIp;

        LogFunc(("network:%RTnaipv4, dhcpserver:%RTnaipv4, dhcplowerip:%RTnaipv4, dhcpupperip:%RTnaipv4\n",
                 network, dhcpserver, dhcplowerip, dhcpupperip));
    }

    /* we need IPv4NetworkMask for NAT's gw service start */
    netmask.u = RT_H2N_U32(netmask.u);
    RTStrPrintf(szTmpIp, 16, "%RTnaipv4", netmask);
    m->IPv4NetworkMask = szTmpIp;

    LogFlowFunc(("getaway:%RTnaipv4, netmask:%RTnaipv4\n", gateway, netmask));
    return VINF_SUCCESS;
}


int NATNetwork::i_recalculateIPv6Prefix()
{
    int rc;

    RTNETADDRIPV4 net, mask;
    rc = RTCidrStrToIPv4(Utf8Str(m->s.strIPv4NetworkCidr).c_str(), &net, &mask);
    if (RT_FAILURE(rc))
        return rc;

    net.u = RT_H2N_U32(net.u);  /* XXX: fix RTCidrStrToIPv4! */

    /*
     * [fd17:625c:f037:XXXX::/64] - RFC 4193 (ULA) Locally Assigned
     * Global ID where XXXX, 16 bit Subnet ID, are two bytes from the
     * middle of the IPv4 address, e.g. :dead: for 10.222.173.1
     */
    RTNETADDRIPV6 prefix;
    RT_ZERO(prefix);

    prefix.au8[0] = 0xFD;
    prefix.au8[1] = 0x17;

    prefix.au8[2] = 0x62;
    prefix.au8[3] = 0x5C;

    prefix.au8[4] = 0xF0;
    prefix.au8[5] = 0x37;

    prefix.au8[6] = net.au8[1];
    prefix.au8[7] = net.au8[2];

    char szBuf[32];
    RTStrPrintf(szBuf, sizeof(szBuf), "%RTnaipv6/64", &prefix);

    m->s.strIPv6Prefix = szBuf;
    return VINF_SUCCESS;
}
