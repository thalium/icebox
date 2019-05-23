/* $Id: HostDnsService.cpp $ */
/** @file
 * Base class for Host DNS & Co services.
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

#include <VBox/com/array.h>
#include <VBox/com/ptr.h>
#include <VBox/com/string.h>

#include <iprt/cpp/utils.h>

#include "Logging.h"
#include "VirtualBoxImpl.h"
#include <iprt/time.h>
#include <iprt/thread.h>
#include <iprt/semaphore.h>
#include <iprt/critsect.h>

#include <algorithm>
#include <set>
#include <string>
#include "HostDnsService.h"


static HostDnsMonitor *g_monitor;

static void dumpHostDnsInformation(const HostDnsInformation&);
static void dumpHostDnsStrVector(const std::string&, const std::vector<std::string>&);


bool HostDnsInformation::equals(const HostDnsInformation &info, uint32_t fLaxComparison) const
{
    bool fSameServers;
    if ((fLaxComparison & IGNORE_SERVER_ORDER) == 0)
    {
        fSameServers = (servers == info.servers);
    }
    else
    {
        std::set<std::string> l(servers.begin(), servers.end());
        std::set<std::string> r(info.servers.begin(), info.servers.end());

        fSameServers = (l == r);
    }

    bool fSameDomain, fSameSearchList;
    if ((fLaxComparison & IGNORE_SUFFIXES) == 0)
    {
        fSameDomain = (domain == info.domain);
        fSameSearchList = (searchList == info.searchList);
    }
    else
    {
        fSameDomain = fSameSearchList = true;
    }

    return fSameServers && fSameDomain && fSameSearchList;
}

inline static void detachVectorOfString(const std::vector<std::string>& v,
                                        std::vector<com::Utf8Str> &aArray)
{
    aArray.resize(v.size());
    size_t i = 0;
    for (std::vector<std::string>::const_iterator it = v.begin(); it != v.end(); ++it, ++i)
        aArray[i] = Utf8Str(it->c_str());
}

struct HostDnsMonitor::Data
{
    Data(bool aThreaded)
      : uLastExtraDataPoll(0),
        fLaxComparison(0),
        fThreaded(aThreaded),
        virtualbox(NULL)
    {}

    std::vector<PCHostDnsMonitorProxy> proxies;
    HostDnsInformation info;
    uint64_t uLastExtraDataPoll;
    uint32_t fLaxComparison;
    const bool fThreaded;
    RTTHREAD hMonitoringThread;
    RTSEMEVENT hDnsInitEvent;
    VirtualBox *virtualbox;
};

struct HostDnsMonitorProxy::Data
{
    Data(const HostDnsMonitor *aMonitor, VirtualBox *aParent)
      : info(NULL)
      , virtualbox(aParent)
      , monitor(aMonitor)
      , fModified(true)
    {}

    virtual ~Data()
    {
        if (info)
        {
            delete info;
            info = NULL;
        }
    }

    HostDnsInformation *info;
    VirtualBox *virtualbox;
    const HostDnsMonitor *monitor;
    bool fModified;
};


HostDnsMonitor::HostDnsMonitor(bool fThreaded)
  : m(NULL)
{
   m = new HostDnsMonitor::Data(fThreaded);
}

HostDnsMonitor::~HostDnsMonitor()
{
    if (m)
    {
        delete m;
        m = NULL;
    }
}

const HostDnsMonitor *HostDnsMonitor::getHostDnsMonitor(VirtualBox *aParent)
{
    /* XXX: Moved initialization from HostImpl.cpp */
    if (!g_monitor)
    {
# if defined (RT_OS_DARWIN)
        g_monitor = new HostDnsServiceDarwin();
# elif defined(RT_OS_WINDOWS)
        g_monitor = new HostDnsServiceWin();
# elif defined(RT_OS_LINUX)
        g_monitor = new HostDnsServiceLinux();
# elif defined(RT_OS_SOLARIS)
        g_monitor =  new HostDnsServiceSolaris();
# elif defined(RT_OS_FREEBSD)
        g_monitor = new HostDnsServiceFreebsd();
# elif defined(RT_OS_OS2)
        g_monitor = new HostDnsServiceOs2();
# else
        g_monitor = new HostDnsService();
# endif
        g_monitor->init(aParent);
    }

    return g_monitor;
}

void HostDnsMonitor::addMonitorProxy(PCHostDnsMonitorProxy proxy) const
{
    RTCLock grab(m_LockMtx);
    m->proxies.push_back(proxy);
    proxy->notify();
}

void HostDnsMonitor::releaseMonitorProxy(PCHostDnsMonitorProxy proxy) const
{
    RTCLock grab(m_LockMtx);
    std::vector<PCHostDnsMonitorProxy>::iterator it;
    it = std::find(m->proxies.begin(), m->proxies.end(), proxy);

    if (it == m->proxies.end())
        return;

    m->proxies.erase(it);
}

void HostDnsMonitor::shutdown()
{
    /** @todo never called.
     * HostDnsMonitor should be referenced by HostDnsMonitorProxy objects and the Host object
     * and automatically deleted when not referenced anymore.
     * Currently HostDnsMonitor can use already deleted m->virtualbox.
     */
    if (g_monitor)
    {
        delete g_monitor;
        g_monitor = NULL;
    }
}

const HostDnsInformation &HostDnsMonitor::getInfo() const
{
    return m->info;
}

void HostDnsMonitor::setInfo(const HostDnsInformation &info)
{
    RTCLock grab(m_LockMtx);

    pollGlobalExtraData();

    if (info.equals(m->info))
        return;

    LogRel(("HostDnsMonitor: old information\n"));
    dumpHostDnsInformation(m->info);
    LogRel(("HostDnsMonitor: new information\n"));
    dumpHostDnsInformation(info);

    bool fIgnore = m->fLaxComparison && info.equals(m->info, m->fLaxComparison);
    m->info = info;

    if (fIgnore)
    {
        LogRel(("HostDnsMonitor: lax comparison %#x, not notifying\n", m->fLaxComparison));
        return;
    }

    std::vector<PCHostDnsMonitorProxy>::const_iterator it;
    for (it = m->proxies.begin(); it != m->proxies.end(); ++it)
        (*it)->notify();
}

HRESULT HostDnsMonitor::init(VirtualBox *virtualbox)
{
    m->virtualbox = virtualbox;

    pollGlobalExtraData();

    if (m->fThreaded)
    {
        int rc = RTSemEventCreate(&m->hDnsInitEvent);
        AssertRCReturn(rc, E_FAIL);

        rc = RTThreadCreate(&m->hMonitoringThread,
                            HostDnsMonitor::threadMonitoringRoutine,
                            this, 128 * _1K, RTTHREADTYPE_IO, 0, "dns-monitor");
        AssertRCReturn(rc, E_FAIL);

        RTSemEventWait(m->hDnsInitEvent, RT_INDEFINITE_WAIT);
    }
    return S_OK;
}


void HostDnsMonitor::pollGlobalExtraData()
{
    uint64_t uNow = RTTimeNanoTS();
    if (m->virtualbox && (uNow - m->uLastExtraDataPoll >= RT_NS_30SEC || m->uLastExtraDataPoll == 0))
    {
        m->uLastExtraDataPoll = uNow;

        /*
         * Should we ignore the order of DNS servers?
         */
        const com::Bstr bstrHostDNSOrderIgnoreKey("VBoxInternal2/HostDNSOrderIgnore");
        com::Bstr bstrHostDNSOrderIgnore;
        m->virtualbox->GetExtraData(bstrHostDNSOrderIgnoreKey.raw(),
                                    bstrHostDNSOrderIgnore.asOutParam());
        uint32_t fDNSOrderIgnore = 0;
        if (bstrHostDNSOrderIgnore.isNotEmpty())
        {
            if (bstrHostDNSOrderIgnore != "0")
                fDNSOrderIgnore = HostDnsInformation::IGNORE_SERVER_ORDER;
        }

        if (fDNSOrderIgnore != (m->fLaxComparison & HostDnsInformation::IGNORE_SERVER_ORDER))
        {

            m->fLaxComparison ^= HostDnsInformation::IGNORE_SERVER_ORDER;
            LogRel(("HostDnsMonitor: %ls=%ls\n",
                    bstrHostDNSOrderIgnoreKey.raw(),
                    bstrHostDNSOrderIgnore.raw()));
        }

        /*
         * Should we ignore changes to the domain name or the search list?
         */
        const com::Bstr bstrHostDNSSuffixesIgnoreKey("VBoxInternal2/HostDNSSuffixesIgnore");
        com::Bstr bstrHostDNSSuffixesIgnore;
        m->virtualbox->GetExtraData(bstrHostDNSSuffixesIgnoreKey.raw(),
                                    bstrHostDNSSuffixesIgnore.asOutParam());
        uint32_t fDNSSuffixesIgnore = 0;
        if (bstrHostDNSSuffixesIgnore.isNotEmpty())
        {
            if (bstrHostDNSSuffixesIgnore != "0")
                fDNSSuffixesIgnore = HostDnsInformation::IGNORE_SUFFIXES;
        }

        if (fDNSSuffixesIgnore != (m->fLaxComparison & HostDnsInformation::IGNORE_SUFFIXES))
        {

            m->fLaxComparison ^= HostDnsInformation::IGNORE_SUFFIXES;
            LogRel(("HostDnsMonitor: %ls=%ls\n",
                    bstrHostDNSSuffixesIgnoreKey.raw(),
                    bstrHostDNSSuffixesIgnore.raw()));
        }
    }
}

void HostDnsMonitor::monitorThreadInitializationDone()
{
    RTSemEventSignal(m->hDnsInitEvent);
}


DECLCALLBACK(int) HostDnsMonitor::threadMonitoringRoutine(RTTHREAD, void *pvUser)
{
    HostDnsMonitor *pThis = static_cast<HostDnsMonitor *>(pvUser);
    return pThis->monitorWorker();
}

/* HostDnsMonitorProxy */
HostDnsMonitorProxy::HostDnsMonitorProxy()
  : m(NULL)
{
}

HostDnsMonitorProxy::~HostDnsMonitorProxy()
{
    if (m)
    {
        if (m->monitor)
            m->monitor->releaseMonitorProxy(this);
        delete m;
        m = NULL;
    }
}

void HostDnsMonitorProxy::init(const HostDnsMonitor *mon, VirtualBox* aParent)
{
    m = new HostDnsMonitorProxy::Data(mon, aParent);
    m->monitor->addMonitorProxy(this);
    updateInfo();
}

void HostDnsMonitorProxy::notify() const
{
    LogRel(("HostDnsMonitorProxy::notify\n"));
    m->fModified = true;
    m->virtualbox->i_onHostNameResolutionConfigurationChange();
}

HRESULT HostDnsMonitorProxy::GetNameServers(std::vector<com::Utf8Str> &aNameServers)
{
    AssertReturn(m && m->info, E_FAIL);
    RTCLock grab(m_LockMtx);

    if (m->fModified)
        updateInfo();

    LogRel(("HostDnsMonitorProxy::GetNameServers:\n"));
    dumpHostDnsStrVector("name server", m->info->servers);

    detachVectorOfString(m->info->servers, aNameServers);

    return S_OK;
}

HRESULT HostDnsMonitorProxy::GetDomainName(com::Utf8Str *pDomainName)
{
    AssertReturn(m && m->info, E_FAIL);
    RTCLock grab(m_LockMtx);

    if (m->fModified)
        updateInfo();

    LogRel(("HostDnsMonitorProxy::GetDomainName: %s\n",
            m->info->domain.empty() ? "no domain set" : m->info->domain.c_str()));

    *pDomainName = m->info->domain.c_str();

    return S_OK;
}

HRESULT HostDnsMonitorProxy::GetSearchStrings(std::vector<com::Utf8Str> &aSearchStrings)
{
    AssertReturn(m && m->info, E_FAIL);
    RTCLock grab(m_LockMtx);

    if (m->fModified)
        updateInfo();

    LogRel(("HostDnsMonitorProxy::GetSearchStrings:\n"));
    dumpHostDnsStrVector("search string", m->info->searchList);

    detachVectorOfString(m->info->searchList, aSearchStrings);

    return S_OK;
}

bool HostDnsMonitorProxy::operator==(PCHostDnsMonitorProxy& rhs)
{
    if (!m || !rhs->m)
        return false;

    /**
     * we've assigned to the same instance of VirtualBox.
     */
    return m->virtualbox == rhs->m->virtualbox;
}

void HostDnsMonitorProxy::updateInfo()
{
    HostDnsInformation *info = new HostDnsInformation(m->monitor->getInfo());
    HostDnsInformation *old = m->info;

    m->info = info;
    if (old)
    {
        delete old;
    }

    m->fModified = false;
}


static void dumpHostDnsInformation(const HostDnsInformation& info)
{
    dumpHostDnsStrVector("server", info.servers);

    if (!info.domain.empty())
        LogRel(("  domain: %s\n", info.domain.c_str()));
    else
        LogRel(("  no domain set\n"));

    dumpHostDnsStrVector("search string", info.searchList);
}


static void dumpHostDnsStrVector(const std::string& prefix, const std::vector<std::string>& v)
{
    int i = 1;
    for (std::vector<std::string>::const_iterator it = v.begin();
         it != v.end();
         ++it, ++i)
        LogRel(("  %s %d: %s\n", prefix.c_str(), i, it->c_str()));
    if (v.empty())
        LogRel(("  no %s entries\n", prefix.c_str()));
}
