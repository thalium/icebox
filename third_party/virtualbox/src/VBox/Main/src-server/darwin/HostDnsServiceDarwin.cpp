/* $Id: HostDnsServiceDarwin.cpp $ */
/** @file
 * Darwin specific DNS information fetching.
 */

/*
 * Copyright (C) 2004-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <VBox/com/string.h>
#include <VBox/com/ptr.h>


#include <iprt/err.h>
#include <iprt/thread.h>
#include <iprt/semaphore.h>

#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SCDynamicStore.h>

#include <string>
#include <vector>
#include "../HostDnsService.h"


struct HostDnsServiceDarwin::Data
{
    SCDynamicStoreRef m_store;
    CFRunLoopSourceRef m_DnsWatcher;
    CFRunLoopRef m_RunLoopRef;
    CFRunLoopSourceRef m_Stopper;
    bool m_fStop;
    RTSEMEVENT m_evtStop;
    static void performShutdownCallback(void *);
};


static const CFStringRef kStateNetworkGlobalDNSKey = CFSTR("State:/Network/Global/DNS");


HostDnsServiceDarwin::HostDnsServiceDarwin():HostDnsMonitor(true),m(NULL)
{
    m = new HostDnsServiceDarwin::Data();
}


HostDnsServiceDarwin::~HostDnsServiceDarwin()
{
    if (!m)
        return;

    monitorThreadShutdown();

    CFRelease(m->m_RunLoopRef);

    CFRelease(m->m_DnsWatcher);

    CFRelease(m->m_store);

    RTSemEventDestroy(m->m_evtStop);

    delete m;
    m = NULL;
}


void HostDnsServiceDarwin::hostDnsServiceStoreCallback(void *, void *, void *info)
{
    HostDnsServiceDarwin *pThis = (HostDnsServiceDarwin *)info;

    RTCLock grab(pThis->m_LockMtx);
    pThis->updateInfo();
}


HRESULT HostDnsServiceDarwin::init(VirtualBox *virtualbox)
{
    SCDynamicStoreContext ctx;
    RT_ZERO(ctx);

    ctx.info = this;

    m->m_store = SCDynamicStoreCreate(NULL, CFSTR("org.virtualbox.VBoxSVC"),
                                   (SCDynamicStoreCallBack)HostDnsServiceDarwin::hostDnsServiceStoreCallback,
                                   &ctx);
    AssertReturn(m->m_store, E_FAIL);

    m->m_DnsWatcher = SCDynamicStoreCreateRunLoopSource(NULL, m->m_store, 0);
    if (!m->m_DnsWatcher)
        return E_OUTOFMEMORY;

    int rc = RTSemEventCreate(&m->m_evtStop);
    AssertRCReturn(rc, E_FAIL);

    CFRunLoopSourceContext sctx;
    RT_ZERO(sctx);
    sctx.perform = HostDnsServiceDarwin::Data::performShutdownCallback;
    m->m_Stopper = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &sctx);
    AssertReturn(m->m_Stopper, E_FAIL);

    HRESULT hrc = HostDnsMonitor::init(virtualbox);
    AssertComRCReturn(hrc, hrc);

    return updateInfo();
}


void HostDnsServiceDarwin::monitorThreadShutdown()
{
    RTCLock grab(m_LockMtx);
    if (!m->m_fStop)
    {
        CFRunLoopSourceSignal(m->m_Stopper);
        CFRunLoopWakeUp(m->m_RunLoopRef);

        RTSemEventWait(m->m_evtStop, RT_INDEFINITE_WAIT);
    }
}


int HostDnsServiceDarwin::monitorWorker()
{
    m->m_RunLoopRef = CFRunLoopGetCurrent();
    AssertReturn(m->m_RunLoopRef, VERR_INTERNAL_ERROR);

    CFRetain(m->m_RunLoopRef);

    CFArrayRef watchingArrayRef = CFArrayCreate(NULL,
                                                (const void **)&kStateNetworkGlobalDNSKey,
                                                1, &kCFTypeArrayCallBacks);
    if (!watchingArrayRef)
    {
        CFRelease(m->m_DnsWatcher);
        return E_OUTOFMEMORY;
    }

    if(SCDynamicStoreSetNotificationKeys(m->m_store, watchingArrayRef, NULL))
        CFRunLoopAddSource(CFRunLoopGetCurrent(), m->m_DnsWatcher, kCFRunLoopCommonModes);

    CFRelease(watchingArrayRef);

    monitorThreadInitializationDone();

    while (!m->m_fStop)
    {
        CFRunLoopRun();
    }

    CFRelease(m->m_RunLoopRef);

    /* We're notifying stopper thread. */
    RTSemEventSignal(m->m_evtStop);

    return VINF_SUCCESS;
}


HRESULT HostDnsServiceDarwin::updateInfo()
{
    CFPropertyListRef propertyRef = SCDynamicStoreCopyValue(m->m_store,
                                                            kStateNetworkGlobalDNSKey);
    /**
     * # scutil
     * \> get State:/Network/Global/DNS
     * \> d.show
     * \<dictionary\> {
     * DomainName : vvl-domain
     * SearchDomains : \<array\> {
     * 0 : vvl-domain
     * 1 : de.vvl-domain.com
     * }
     * ServerAddresses : \<array\> {
     * 0 : 192.168.1.4
     * 1 : 192.168.1.1
     * 2 : 8.8.4.4
     *   }
     * }
     */

    if (!propertyRef)
        return S_OK;

    HostDnsInformation info;
    CFStringRef domainNameRef = (CFStringRef)CFDictionaryGetValue(
      static_cast<CFDictionaryRef>(propertyRef), CFSTR("DomainName"));
    if (domainNameRef)
    {
        const char *pszDomainName = CFStringGetCStringPtr(domainNameRef,
                                                    CFStringGetSystemEncoding());
        if (pszDomainName)
            info.domain = pszDomainName;
    }

    int i, arrayCount;
    CFArrayRef serverArrayRef = (CFArrayRef)CFDictionaryGetValue(
      static_cast<CFDictionaryRef>(propertyRef), CFSTR("ServerAddresses"));
    if (serverArrayRef)
    {
        arrayCount = CFArrayGetCount(serverArrayRef);
        for (i = 0; i < arrayCount; ++i)
        {
            CFStringRef serverAddressRef = (CFStringRef)CFArrayGetValueAtIndex(serverArrayRef, i);
            if (!serverArrayRef)
                continue;

            const char *pszServerAddress = CFStringGetCStringPtr(serverAddressRef,
                                                           CFStringGetSystemEncoding());
            if (!pszServerAddress)
                continue;

            info.servers.push_back(std::string(pszServerAddress));
        }
    }

    CFArrayRef searchArrayRef = (CFArrayRef)CFDictionaryGetValue(
      static_cast<CFDictionaryRef>(propertyRef), CFSTR("SearchDomains"));
    if (searchArrayRef)
    {
        arrayCount = CFArrayGetCount(searchArrayRef);

        for (i = 0; i < arrayCount; ++i)
        {
            CFStringRef searchStringRef = (CFStringRef)CFArrayGetValueAtIndex(searchArrayRef, i);
            if (!searchArrayRef)
                continue;

            const char *pszSearchString = CFStringGetCStringPtr(searchStringRef,
                                                          CFStringGetSystemEncoding());
            if (!pszSearchString)
                continue;

            info.searchList.push_back(std::string(pszSearchString));
        }
    }

    CFRelease(propertyRef);

    setInfo(info);

    return S_OK;
}

void HostDnsServiceDarwin::Data::performShutdownCallback(void *info)
{
    HostDnsServiceDarwin::Data *pThis = static_cast<HostDnsServiceDarwin::Data *>(info);
    pThis->m_fStop = true;
}
