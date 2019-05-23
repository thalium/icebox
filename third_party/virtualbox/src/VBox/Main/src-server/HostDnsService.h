/* $Id: HostDnsService.h $ */
/** @file
 * Host DNS listener.
 */

/*
 * Copyright (C) 2005-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___H_DNSHOSTSERVICE
#define ___H_DNSHOSTSERVICE
#include "VirtualBoxBase.h"

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/cpp/lock.h>

#include <list>
#include <vector>

typedef std::list<com::Utf8Str> Utf8StrList;
typedef Utf8StrList::iterator Utf8StrListIterator;

class HostDnsMonitorProxy;
typedef const HostDnsMonitorProxy *PCHostDnsMonitorProxy;

class HostDnsInformation
{
  public:
    static const uint32_t IGNORE_SERVER_ORDER = RT_BIT_32(0);
    static const uint32_t IGNORE_SUFFIXES     = RT_BIT_32(1);

  public:
    std::vector<std::string> servers;
    std::string domain;
    std::vector<std::string> searchList;
    bool equals(const HostDnsInformation &, uint32_t fLaxComparison = 0) const;
};

/**
 * This class supposed to be a real DNS monitor object it should be singleton,
 * it lifecycle starts and ends together with VBoxSVC.
 */
class HostDnsMonitor
{
  public:
    static const HostDnsMonitor *getHostDnsMonitor(VirtualBox *virtualbox);
    static void shutdown();

    void addMonitorProxy(PCHostDnsMonitorProxy) const;
    void releaseMonitorProxy(PCHostDnsMonitorProxy) const;
    const HostDnsInformation &getInfo() const;
    /* @note: method will wait till client call
       HostDnsService::monitorThreadInitializationDone() */
    virtual HRESULT init(VirtualBox *virtualbox);

  protected:
    explicit HostDnsMonitor(bool fThreaded = false);
    virtual ~HostDnsMonitor();

    void setInfo(const HostDnsInformation &);

    /* this function used only if HostDnsMonitor::HostDnsMonitor(true) */
    void monitorThreadInitializationDone();
    virtual void monitorThreadShutdown() = 0;
    virtual int monitorWorker() = 0;

  private:
    HostDnsMonitor(const HostDnsMonitor &);
    HostDnsMonitor& operator= (const HostDnsMonitor &);
    static DECLCALLBACK(int) threadMonitoringRoutine(RTTHREAD, void *);
    void pollGlobalExtraData();

  protected:
    mutable RTCLockMtx m_LockMtx;

  public:
    struct Data;
    Data *m;
};

/**
 * This class supposed to be a proxy for events on changing Host Name Resolving configurations.
 */
class HostDnsMonitorProxy
{
    public:
    HostDnsMonitorProxy();
    ~HostDnsMonitorProxy();
    void init(const HostDnsMonitor *aMonitor, VirtualBox *virtualbox);
    void notify() const;

    HRESULT GetNameServers(std::vector<com::Utf8Str> &aNameServers);
    HRESULT GetDomainName(com::Utf8Str *pDomainName);
    HRESULT GetSearchStrings(std::vector<com::Utf8Str> &aSearchStrings);

    bool operator==(PCHostDnsMonitorProxy&);

    private:
    void updateInfo();

  private:
    mutable RTCLockMtx m_LockMtx;

    private:
    struct Data;
    Data *m;
};

# if defined(RT_OS_DARWIN) || defined(DOXYGEN_RUNNING)
class HostDnsServiceDarwin : public HostDnsMonitor
{
  public:
    HostDnsServiceDarwin();
    ~HostDnsServiceDarwin();
    virtual HRESULT init(VirtualBox *virtualbox);

    protected:
    virtual void monitorThreadShutdown();
    virtual int monitorWorker();

    private:
    HRESULT updateInfo();
    static void hostDnsServiceStoreCallback(void *store, void *arrayRef, void *info);
    struct Data;
    Data *m;
};
# endif
# if defined(RT_OS_WINDOWS) || defined(DOXYGEN_RUNNING)
class HostDnsServiceWin : public HostDnsMonitor
{
    public:
    HostDnsServiceWin();
    ~HostDnsServiceWin();
    virtual HRESULT init(VirtualBox *virtualbox);

    protected:
    virtual void monitorThreadShutdown();
    virtual int monitorWorker();

    private:
    HRESULT updateInfo();

    private:
    struct Data;
    Data *m;
};
# endif
# if defined(RT_OS_SOLARIS) || defined(RT_OS_LINUX) || defined(RT_OS_OS2) || defined(RT_OS_FREEBSD) \
    || defined(DOXYGEN_RUNNING)
class HostDnsServiceResolvConf: public HostDnsMonitor
{
  public:
    explicit HostDnsServiceResolvConf(bool fThreaded = false) : HostDnsMonitor(fThreaded), m(NULL) {}
    virtual ~HostDnsServiceResolvConf();
    virtual HRESULT init(VirtualBox *virtualbox, const char *aResolvConfFileName);
    const std::string& resolvConf() const;

  protected:
    HRESULT readResolvConf();
    /* While not all hosts supports Hosts DNS change notifiaction
     * default implementation offers return VERR_IGNORE.
     */
    virtual void monitorThreadShutdown() {}
    virtual int monitorWorker() {return VERR_IGNORED;}

  protected:
    struct Data;
    Data *m;
};
#  if defined(RT_OS_SOLARIS) || defined(DOXYGEN_RUNNING)
/**
 * XXX: https://blogs.oracle.com/praks/entry/file_events_notification
 */
class HostDnsServiceSolaris : public HostDnsServiceResolvConf
{
  public:
    HostDnsServiceSolaris(){}
    ~HostDnsServiceSolaris(){}
    virtual HRESULT init(VirtualBox *virtualbox) {
        return HostDnsServiceResolvConf::init(virtualbox, "/etc/resolv.conf");
    }
};

#  endif
#  if defined(RT_OS_LINUX) || defined(DOXYGEN_RUNNING)
class HostDnsServiceLinux : public HostDnsServiceResolvConf
{
  public:
    HostDnsServiceLinux():HostDnsServiceResolvConf(true){}
    virtual ~HostDnsServiceLinux();
    virtual HRESULT init(VirtualBox *virtualbox) {
        return HostDnsServiceResolvConf::init(virtualbox, "/etc/resolv.conf");
    }

  protected:
    virtual void monitorThreadShutdown();
    virtual int monitorWorker();
};

#  endif
#  if defined(RT_OS_FREEBSD) || defined(DOXYGEN_RUNNING)
class HostDnsServiceFreebsd: public HostDnsServiceResolvConf
{
    public:
    HostDnsServiceFreebsd(){}
    ~HostDnsServiceFreebsd(){}
    virtual HRESULT init(VirtualBox *virtualbox) {
        return HostDnsServiceResolvConf::init(virtualbox, "/etc/resolv.conf");
    }
};

#  endif
#  if defined(RT_OS_OS2) || defined(DOXYGEN_RUNNING)
class HostDnsServiceOs2 : public HostDnsServiceResolvConf
{
  public:
    HostDnsServiceOs2(){}
    ~HostDnsServiceOs2(){}
    /* XXX: \\MPTN\\ETC should be taken from environment variable ETC  */
    virtual HRESULT init(VirtualBox *virtualbox) {
        return HostDnsServiceResolvConf::init(virtualbox, "\\MPTN\\ETC\\RESOLV2");
    }
};

#  endif
# endif

#endif /* !___H_DNSHOSTSERVICE */
