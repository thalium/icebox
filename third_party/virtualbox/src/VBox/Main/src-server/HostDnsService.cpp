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
      : proxy(NULL),
        fThreaded(aThreaded)
    {}

    HostDnsMonitorProxy *proxy;

    const bool fThreaded;
    RTSEMEVENT hDnsInitEvent;
    RTTHREAD hMonitoringThread;

    HostDnsInformation info;
};

struct HostDnsMonitorProxy::Data
{
    Data(HostDnsMonitor *aMonitor, VirtualBox *aParent)
      : virtualbox(aParent),
        monitor(aMonitor),
        uLastExtraDataPoll(0),
        fLaxComparison(0),
        info()
    {}

    VirtualBox *virtualbox;
    HostDnsMonitor *monitor;

    uint64_t uLastExtraDataPoll;
    uint32_t fLaxComparison;
    HostDnsInformation info;
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

HostDnsMonitor *HostDnsMonitor::createHostDnsMonitor()
{
    HostDnsMonitor *monitor = NULL;

#if defined (RT_OS_DARWIN)
    monitor = new HostDnsServiceDarwin();
#elif defined(RT_OS_WINDOWS)
    monitor = new HostDnsServiceWin();
#elif defined(RT_OS_LINUX)
    monitor = new HostDnsServiceLinux();
#elif defined(RT_OS_SOLARIS)
    monitor =  new HostDnsServiceSolaris();
#elif defined(RT_OS_FREEBSD)
    monitor = new HostDnsServiceFreebsd();
#elif defined(RT_OS_OS2)
    monitor = new HostDnsServiceOs2();
#else
    monitor = new HostDnsService();
#endif

    return monitor;
}


void HostDnsMonitor::shutdown()
{
    monitorThreadShutdown();
    int rc = RTThreadWait(m->hMonitoringThread, 5000, NULL);
    AssertRCSuccess(rc);
}


void HostDnsMonitor::setInfo(const HostDnsInformation &info)
{
    if (m->proxy != NULL)
        m->proxy->notify(info);
}

HRESULT HostDnsMonitor::init(HostDnsMonitorProxy *proxy)
{
    m->proxy = proxy;

    if (m->fThreaded)
    {
        int rc = RTSemEventCreate(&m->hDnsInitEvent);
        AssertRCReturn(rc, E_FAIL);

        rc = RTThreadCreate(&m->hMonitoringThread,
                            HostDnsMonitor::threadMonitoringRoutine,
                            this, 128 * _1K, RTTHREADTYPE_IO,
                            RTTHREADFLAGS_WAITABLE, "dns-monitor");
        AssertRCReturn(rc, E_FAIL);

        RTSemEventWait(m->hDnsInitEvent, RT_INDEFINITE_WAIT);
    }
    return S_OK;
}


void HostDnsMonitorProxy::pollGlobalExtraData()
{
    VirtualBox *virtualbox = m->virtualbox;
    if (RT_UNLIKELY(virtualbox == NULL))
        return;

    uint64_t uNow = RTTimeNanoTS();
    if (uNow - m->uLastExtraDataPoll >= RT_NS_30SEC || m->uLastExtraDataPoll == 0)
    {
        m->uLastExtraDataPoll = uNow;

        /*
         * Should we ignore the order of DNS servers?
         */
        const com::Bstr bstrHostDNSOrderIgnoreKey("VBoxInternal2/HostDNSOrderIgnore");
        com::Bstr bstrHostDNSOrderIgnore;
        virtualbox->GetExtraData(bstrHostDNSOrderIgnoreKey.raw(),
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
        virtualbox->GetExtraData(bstrHostDNSSuffixesIgnoreKey.raw(),
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
    Assert(!m);
}

void HostDnsMonitorProxy::init(VirtualBox* aParent)
{
    HostDnsMonitor *monitor = HostDnsMonitor::createHostDnsMonitor();
    m = new HostDnsMonitorProxy::Data(monitor, aParent);
    m->monitor->init(this);
}


void HostDnsMonitorProxy::uninit()
{
    if (m)
    {
        m->monitor->shutdown();
        delete m;
        m = NULL;
    }
}

void HostDnsMonitorProxy::notify(const HostDnsInformation &info)
{
    bool fNotify = updateInfo(info);
    if (fNotify)
        m->virtualbox->i_onHostNameResolutionConfigurationChange();
}

HRESULT HostDnsMonitorProxy::GetNameServers(std::vector<com::Utf8Str> &aNameServers)
{
    AssertReturn(m != NULL, E_FAIL);
    RTCLock grab(m_LockMtx);

    LogRel(("HostDnsMonitorProxy::GetNameServers:\n"));
    dumpHostDnsStrVector("name server", m->info.servers);

    detachVectorOfString(m->info.servers, aNameServers);

    return S_OK;
}

HRESULT HostDnsMonitorProxy::GetDomainName(com::Utf8Str *pDomainName)
{
    AssertReturn(m != NULL, E_FAIL);
    RTCLock grab(m_LockMtx);

    LogRel(("HostDnsMonitorProxy::GetDomainName: %s\n",
            m->info.domain.empty() ? "no domain set" : m->info.domain.c_str()));

    *pDomainName = m->info.domain.c_str();

    return S_OK;
}

HRESULT HostDnsMonitorProxy::GetSearchStrings(std::vector<com::Utf8Str> &aSearchStrings)
{
    AssertReturn(m != NULL, E_FAIL);
    RTCLock grab(m_LockMtx);

    LogRel(("HostDnsMonitorProxy::GetSearchStrings:\n"));
    dumpHostDnsStrVector("search string", m->info.searchList);

    detachVectorOfString(m->info.searchList, aSearchStrings);

    return S_OK;
}

bool HostDnsMonitorProxy::updateInfo(const HostDnsInformation &info)
{
    LogRel(("HostDnsMonitor::updateInfo\n"));
    RTCLock grab(m_LockMtx);

    if (info.equals(m->info))
    {
        LogRel(("HostDnsMonitor: unchanged\n"));
        return false;
    }

    pollGlobalExtraData();

    LogRel(("HostDnsMonitor: old information\n"));
    dumpHostDnsInformation(m->info);
    LogRel(("HostDnsMonitor: new information\n"));
    dumpHostDnsInformation(info);

    bool fIgnore = m->fLaxComparison != 0 && info.equals(m->info, m->fLaxComparison);
    m->info = info;

    if (fIgnore)
    {
        LogRel(("HostDnsMonitor: lax comparison %#x, not notifying\n", m->fLaxComparison));
        return false;
    }

    return true;
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
