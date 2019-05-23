/* $Id: HostImpl.cpp $ */
/** @file
 * VirtualBox COM class implementation: Host
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

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

// VBoxNetCfg-win.h needs winsock2.h and thus MUST be included before any other
// header file includes Windows.h.
#if defined(RT_OS_WINDOWS) && defined(VBOX_WITH_NETFLT)
# include <VBox/VBoxNetCfg-win.h>
#endif

// for some reason Windows burns in sdk\...\winsock.h if this isn't included first
#include "VBox/com/ptr.h"

#include "HostImpl.h"

#ifdef VBOX_WITH_USB
# include "HostUSBDeviceImpl.h"
# include "USBDeviceFilterImpl.h"
# include "USBProxyService.h"
#else
# include "VirtualBoxImpl.h"
#endif // VBOX_WITH_USB

#include "HostNetworkInterfaceImpl.h"
#include "HostVideoInputDeviceImpl.h"
#include "MachineImpl.h"
#include "AutoCaller.h"
#include "Logging.h"
#include "Performance.h"

#include "MediumImpl.h"
#include "HostPower.h"

#if defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD)
# include <HostHardwareLinux.h>
#endif

#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_FREEBSD)
# include <set>
#endif

#ifdef VBOX_WITH_RESOURCE_USAGE_API
# include "PerformanceImpl.h"
#endif /* VBOX_WITH_RESOURCE_USAGE_API */

#if defined(RT_OS_DARWIN) && ARCH_BITS == 32
# include <sys/types.h>
# include <sys/sysctl.h>
#endif

#ifdef RT_OS_LINUX
# include <sys/ioctl.h>
# include <errno.h>
# include <net/if.h>
# include <net/if_arp.h>
#endif /* RT_OS_LINUX */

#ifdef RT_OS_SOLARIS
# include <fcntl.h>
# include <unistd.h>
# include <stropts.h>
# include <errno.h>
# include <limits.h>
# include <stdio.h>
# include <libdevinfo.h>
# include <sys/mkdev.h>
# include <sys/scsi/generic/inquiry.h>
# include <net/if.h>
# include <sys/socket.h>
# include <sys/sockio.h>
# include <net/if_arp.h>
# include <net/if.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/cdio.h>
# include <sys/dkio.h>
# include <sys/mnttab.h>
# include <sys/mntent.h>
/* Dynamic loading of libhal on Solaris hosts */
# ifdef VBOX_USE_LIBHAL
#  include "vbox-libhal.h"
extern "C" char *getfullrawname(char *);
# endif
# include "solaris/DynLoadLibSolaris.h"

/**
 * Solaris DVD drive list as returned by getDVDInfoFromDevTree().
 */
typedef struct SOLARISDVD
{
    struct SOLARISDVD *pNext;
    char szDescription[512];
    char szRawDiskPath[PATH_MAX];
} SOLARISDVD;
/** Pointer to a Solaris DVD descriptor. */
typedef SOLARISDVD *PSOLARISDVD;



#endif /* RT_OS_SOLARIS */

#ifdef RT_OS_WINDOWS
# define _WIN32_DCOM
# include <iprt/win/windows.h>
# include <shellapi.h>
# define INITGUID
# include <guiddef.h>
# include <devguid.h>
# include <iprt/win/objbase.h>
//# include <iprt/win/setupapi.h>
# include <iprt/win/shlobj.h>
# include <cfgmgr32.h>
# include <tchar.h>
#endif /* RT_OS_WINDOWS */

#ifdef RT_OS_DARWIN
# include "darwin/iokit.h"
#endif

#ifdef VBOX_WITH_CROGL
#include <VBox/VBoxOGL.h>
#endif /* VBOX_WITH_CROGL */

#include <iprt/asm-amd64-x86.h>
#include <iprt/string.h>
#include <iprt/mp.h>
#include <iprt/time.h>
#include <iprt/param.h>
#include <iprt/env.h>
#include <iprt/mem.h>
#include <iprt/system.h>
#ifndef RT_OS_WINDOWS
# include <iprt/path.h>
#endif
#ifdef RT_OS_SOLARIS
# include <iprt/ctype.h>
#endif
#ifdef VBOX_WITH_HOSTNETIF_API
# include "netif.h"
#endif

#include <VBox/usb.h>
#include <VBox/err.h>
#include <VBox/settings.h>
#include <VBox/sup.h>
#include <iprt/x86.h>

#include "VBox/com/MultiResult.h"
#include "VBox/com/array.h"

#include <stdio.h>

#include <algorithm>
#include <string>
#include <vector>

#include "HostDnsService.h"

////////////////////////////////////////////////////////////////////////////////
//
// Host private data definition
//
////////////////////////////////////////////////////////////////////////////////

struct Host::Data
{
    Data()
        :
          fDVDDrivesListBuilt(false),
          fFloppyDrivesListBuilt(false)
    {};

    VirtualBox              *pParent;

    HostNetworkInterfaceList llNetIfs;                  // list of network interfaces

#ifdef VBOX_WITH_USB
    USBDeviceFilterList     llChildren;                 // all USB device filters
    USBDeviceFilterList     llUSBDeviceFilters;         // USB device filters in use by the USB proxy service

    /** Pointer to the USBProxyService object. */
    USBProxyService         *pUSBProxyService;
#endif /* VBOX_WITH_USB */

    // list of host drives; lazily created by getDVDDrives() and getFloppyDrives(),
    // and protected by the medium tree lock handle (including the bools).
    MediaList               llDVDDrives,
                            llFloppyDrives;
    bool                    fDVDDrivesListBuilt,
                            fFloppyDrivesListBuilt;

#if defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD)
    /** Object with information about host drives */
    VBoxMainDriveInfo       hostDrives;
#endif

    /** @name Features that can be queried with GetProcessorFeature.
     * @{ */
    bool                    fVTSupported,
                            fLongModeSupported,
                            fPAESupported,
                            fNestedPagingSupported,
                            fRecheckVTSupported;

    /** @}  */

    /** 3D hardware acceleration supported? Tristate, -1 meaning not probed. */
    int                     f3DAccelerationSupported;

    HostPowerService        *pHostPowerService;
    /** Host's DNS informaton fetching */
    HostDnsMonitorProxy     hostDnsMonitorProxy;
};


////////////////////////////////////////////////////////////////////////////////
//
// Constructor / destructor
//
////////////////////////////////////////////////////////////////////////////////
DEFINE_EMPTY_CTOR_DTOR(Host)

HRESULT Host::FinalConstruct()
{
    return BaseFinalConstruct();
}

void Host::FinalRelease()
{
    uninit();
    BaseFinalRelease();
}

/**
 * Initializes the host object.
 *
 * @param aParent   VirtualBox parent object.
 */
HRESULT Host::init(VirtualBox *aParent)
{
    HRESULT hrc;
    LogFlowThisFunc(("aParent=%p\n", aParent));

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m = new Data();

    m->pParent = aParent;

#ifdef VBOX_WITH_USB
    /*
     * Create and initialize the USB Proxy Service.
     */
    m->pUSBProxyService = new USBProxyService(this);
    hrc = m->pUSBProxyService->init();
    AssertComRCReturn(hrc, hrc);
#endif /* VBOX_WITH_USB */

#ifdef VBOX_WITH_RESOURCE_USAGE_API
    i_registerMetrics(aParent->i_performanceCollector());
#endif /* VBOX_WITH_RESOURCE_USAGE_API */
    /* Create the list of network interfaces so their metrics get registered. */
    i_updateNetIfList();

    m->hostDnsMonitorProxy.init(HostDnsMonitor::getHostDnsMonitor(m->pParent), m->pParent);

#if defined(RT_OS_WINDOWS)
    m->pHostPowerService = new HostPowerServiceWin(m->pParent);
#elif defined(RT_OS_LINUX) && defined(VBOX_WITH_DBUS)
    m->pHostPowerService = new HostPowerServiceLinux(m->pParent);
#elif defined(RT_OS_DARWIN)
    m->pHostPowerService = new HostPowerServiceDarwin(m->pParent);
#else
    m->pHostPowerService = new HostPowerService(m->pParent);
#endif

    /* Cache the features reported by GetProcessorFeature. */
    m->fVTSupported = false;
    m->fLongModeSupported = false;
    m->fPAESupported = false;
    m->fNestedPagingSupported = false;
    m->fRecheckVTSupported = false;

    if (ASMHasCpuId())
    {
        /* Note! This code is duplicated in SUPDrv.c and other places! */
        uint32_t uMaxId, uVendorEBX, uVendorECX, uVendorEDX;
        ASMCpuId(0, &uMaxId, &uVendorEBX, &uVendorECX, &uVendorEDX);
        if (ASMIsValidStdRange(uMaxId))
        {
            /* PAE? */
            uint32_t uDummy, fFeaturesEcx, fFeaturesEdx;
            ASMCpuId(1, &uDummy, &uDummy, &fFeaturesEcx, &fFeaturesEdx);
            m->fPAESupported = RT_BOOL(fFeaturesEdx & X86_CPUID_FEATURE_EDX_PAE);

            /* Long Mode? */
            uint32_t uExtMaxId, fExtFeaturesEcx, fExtFeaturesEdx;
            ASMCpuId(0x80000000, &uExtMaxId, &uDummy, &uDummy, &uDummy);
            ASMCpuId(0x80000001, &uDummy, &uDummy, &fExtFeaturesEcx, &fExtFeaturesEdx);
            m->fLongModeSupported = ASMIsValidExtRange(uExtMaxId)
                                 && (fExtFeaturesEdx & X86_CPUID_EXT_FEATURE_EDX_LONG_MODE);

#if defined(RT_OS_DARWIN) && ARCH_BITS == 32 /* darwin.x86 has some optimizations of 64-bit on 32-bit. */
            int     f64bitCapable = 0;
            size_t  cbParameter   = sizeof(f64bitCapable);
            if (sysctlbyname("hw.cpu64bit_capable", &f64bitCapable, &cbParameter, NULL, NULL) != -1)
                m->fLongModeSupported = f64bitCapable != 0;
#endif

            /* VT-x? */
            if (   ASMIsIntelCpuEx(uVendorEBX, uVendorECX, uVendorEDX)
                || ASMIsViaCentaurCpuEx(uVendorEBX, uVendorECX, uVendorEDX))
            {
                if (    (fFeaturesEcx & X86_CPUID_FEATURE_ECX_VMX)
                     && (fFeaturesEdx & X86_CPUID_FEATURE_EDX_MSR)
                     && (fFeaturesEdx & X86_CPUID_FEATURE_EDX_FXSR)
                   )
                {
                    int rc = SUPR3QueryVTxSupported();
                    if (RT_SUCCESS(rc))
                        m->fVTSupported = true;
                }
            }
            /* AMD-V */
            else if (ASMIsAmdCpuEx(uVendorEBX, uVendorECX, uVendorEDX))
            {
                if (   (fExtFeaturesEcx & X86_CPUID_AMD_FEATURE_ECX_SVM)
                    && (fFeaturesEdx    & X86_CPUID_FEATURE_EDX_MSR)
                    && (fFeaturesEdx    & X86_CPUID_FEATURE_EDX_FXSR)
                    && ASMIsValidExtRange(uExtMaxId)
                   )
                {
                    m->fVTSupported = true;

                    /* Query AMD features. */
                    if (uExtMaxId >= 0x8000000a)
                    {
                        uint32_t fSVMFeaturesEdx;
                        ASMCpuId(0x8000000a, &uDummy, &uDummy, &uDummy, &fSVMFeaturesEdx);
                        if (fSVMFeaturesEdx & X86_CPUID_SVM_FEATURE_EDX_NESTED_PAGING)
                            m->fNestedPagingSupported = true;
                    }
                }
            }
        }
    }

    /* Check with SUPDrv if VT-x and AMD-V are really supported (may fail). */
    if (m->fVTSupported)
    {
        int rc = SUPR3InitEx(false /*fUnrestricted*/, NULL);
        if (RT_SUCCESS(rc))
        {
            uint32_t fVTCaps;
            rc = SUPR3QueryVTCaps(&fVTCaps);
            if (RT_SUCCESS(rc))
            {
                Assert(fVTCaps & (SUPVTCAPS_AMD_V | SUPVTCAPS_VT_X));
                if (fVTCaps & SUPVTCAPS_NESTED_PAGING)
                    m->fNestedPagingSupported = true;
                else
                    Assert(m->fNestedPagingSupported == false);
            }
            else
            {
                LogRel(("SUPR0QueryVTCaps -> %Rrc\n", rc));
                m->fVTSupported = m->fNestedPagingSupported = false;
            }
            rc = SUPR3Term(false);
            AssertRC(rc);
        }
        else
            m->fRecheckVTSupported = true; /* Try again later when the driver is loaded. */
    }

#ifdef VBOX_WITH_CROGL
    /* Test for 3D hardware acceleration support later when (if ever) need. */
    m->f3DAccelerationSupported = -1;
#else
    m->f3DAccelerationSupported = false;
#endif

#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_FREEBSD)
    /* Extract the list of configured host-only interfaces */
    std::set<Utf8Str> aConfiguredNames;
    SafeArray<BSTR> aGlobalExtraDataKeys;
    hrc = aParent->GetExtraDataKeys(ComSafeArrayAsOutParam(aGlobalExtraDataKeys));
    AssertMsg(SUCCEEDED(hrc), ("VirtualBox::GetExtraDataKeys failed with %Rhrc\n", hrc));
    for (size_t i = 0; i < aGlobalExtraDataKeys.size(); ++i)
    {
        Utf8Str strKey = aGlobalExtraDataKeys[i];

        if (!strKey.startsWith("HostOnly/vboxnet"))
            continue;

        size_t pos = strKey.find("/", sizeof("HostOnly/vboxnet"));
        if (pos != Utf8Str::npos)
            aConfiguredNames.insert(strKey.substr(sizeof("HostOnly"),
                                                  pos - sizeof("HostOnly")));
    }

    for (std::set<Utf8Str>::const_iterator it = aConfiguredNames.begin();
         it != aConfiguredNames.end();
         ++it)
    {
        ComPtr<IHostNetworkInterface> hif;
        ComPtr<IProgress> progress;

        int r = NetIfCreateHostOnlyNetworkInterface(m->pParent,
                                                    hif.asOutParam(),
                                                    progress.asOutParam(),
                                                    it->c_str());
        if (RT_FAILURE(r))
            LogRel(("failed to create %s, error (0x%x)\n", it->c_str(), r));
    }

#endif /* defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_FREEBSD) */

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}

/**
 *  Uninitializes the host object and sets the ready flag to FALSE.
 *  Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void Host::uninit()
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

#ifdef VBOX_WITH_RESOURCE_USAGE_API
    PerformanceCollector *aCollector = m->pParent->i_performanceCollector();
    i_unregisterMetrics(aCollector);
#endif /* VBOX_WITH_RESOURCE_USAGE_API */
    /*
     * Note that unregisterMetrics() has unregistered all metrics associated
     * with Host including network interface ones. We can destroy network
     * interface objects now. Don't forget the uninit call, otherwise this
     * causes a race with crashing API clients getting their stale references
     * cleaned up and VirtualBox shutting down.
     */
    while (!m->llNetIfs.empty())
    {
        ComObjPtr<HostNetworkInterface> &pNet = m->llNetIfs.front();
        pNet->uninit();
        m->llNetIfs.pop_front();
    }

#ifdef VBOX_WITH_USB
    /* wait for USB proxy service to terminate before we uninit all USB
     * devices */
    LogFlowThisFunc(("Stopping USB proxy service...\n"));
    delete m->pUSBProxyService;
    m->pUSBProxyService = NULL;
    LogFlowThisFunc(("Done stopping USB proxy service.\n"));
#endif

    delete m->pHostPowerService;

#ifdef VBOX_WITH_USB
    /* uninit all USB device filters still referenced by clients
     * Note! HostUSBDeviceFilter::uninit() will modify llChildren.
     * This list should be already empty, but better be safe than sorry. */
    while (!m->llChildren.empty())
    {
        ComObjPtr<HostUSBDeviceFilter> &pChild = m->llChildren.front();
        pChild->uninit();
        m->llChildren.pop_front();
    }

    /* No need to uninit these, as either Machine::uninit() or the above loop
     * already covered them all. Subset of llChildren. */
    m->llUSBDeviceFilters.clear();
#endif

    /* uninit all host DVD medium objects */
    while (!m->llDVDDrives.empty())
    {
        ComObjPtr<Medium> &pMedium = m->llDVDDrives.front();
        pMedium->uninit();
        m->llDVDDrives.pop_front();
    }
    /* uninit all host floppy medium objects */
    while (!m->llFloppyDrives.empty())
    {
        ComObjPtr<Medium> &pMedium = m->llFloppyDrives.front();
        pMedium->uninit();
        m->llFloppyDrives.pop_front();
    }

    delete m;
    m = NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
// IHost public methods
//
////////////////////////////////////////////////////////////////////////////////

/**
 * Returns a list of host DVD drives.
 *
 * @returns COM status code
 * @param aDVDDrives    address of result pointer
 */

HRESULT Host::getDVDDrives(std::vector<ComPtr<IMedium> > &aDVDDrives)
{
    AutoWriteLock treeLock(m->pParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);

    MediaList *pList;
    HRESULT rc = i_getDrives(DeviceType_DVD, true /* fRefresh */, pList, treeLock);
    if (FAILED(rc))
        return rc;

    aDVDDrives.resize(pList->size());
    size_t i = 0;
    for (MediaList::const_iterator it = pList->begin(); it != pList->end(); ++it, ++i)
        (*it).queryInterfaceTo(aDVDDrives[i].asOutParam());

    return S_OK;
}

/**
 * Returns a list of host floppy drives.
 *
 * @returns COM status code
 * @param   aFloppyDrives   address of result pointer
 */
HRESULT Host::getFloppyDrives(std::vector<ComPtr<IMedium> > &aFloppyDrives)
{
    AutoWriteLock treeLock(m->pParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);

    MediaList *pList;
    HRESULT rc = i_getDrives(DeviceType_Floppy, true /* fRefresh */, pList, treeLock);
    if (FAILED(rc))
        return rc;

    aFloppyDrives.resize(pList->size());
    size_t i = 0;
    for (MediaList::const_iterator it = pList->begin(); it != pList->end(); ++it, ++i)
        (*it).queryInterfaceTo(aFloppyDrives[i].asOutParam());

    return S_OK;
}


#if defined(RT_OS_WINDOWS) && defined(VBOX_WITH_NETFLT)
# define VBOX_APP_NAME L"VirtualBox"

static int vboxNetWinAddComponent(std::list< ComObjPtr<HostNetworkInterface> > *pPist,
                                  INetCfgComponent *pncc)
{
    LPWSTR lpszName;
    GUID IfGuid;
    HRESULT hr;
    int rc = VERR_GENERAL_FAILURE;

    hr = pncc->GetDisplayName(&lpszName);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        Bstr name((CBSTR)lpszName);

        hr = pncc->GetInstanceGuid(&IfGuid);
        Assert(hr == S_OK);
        if (hr == S_OK)
        {
            /* create a new object and add it to the list */
            ComObjPtr<HostNetworkInterface> iface;
            iface.createObject();
            /* remove the curly bracket at the end */
            if (SUCCEEDED(iface->init(name, name, Guid(IfGuid), HostNetworkInterfaceType_Bridged)))
            {
//                iface->setVirtualBox(m->pParent);
                pPist->push_back(iface);
                rc = VINF_SUCCESS;
            }
            else
            {
                Assert(0);
            }
        }
        CoTaskMemFree(lpszName);
    }

    return rc;
}
#endif /* defined(RT_OS_WINDOWS) && defined(VBOX_WITH_NETFLT) */

/**
 * Returns a list of host network interfaces.
 *
 * @returns COM status code
 * @param   aNetworkInterfaces  address of result pointer
 */
HRESULT Host::getNetworkInterfaces(std::vector<ComPtr<IHostNetworkInterface> > &aNetworkInterfaces)
{
#if defined(RT_OS_WINDOWS) || defined(VBOX_WITH_NETFLT) /*|| defined(RT_OS_OS2)*/
# ifdef VBOX_WITH_HOSTNETIF_API
    HRESULT rc = i_updateNetIfList();
    if (FAILED(rc))
    {
        Log(("Failed to update host network interface list with rc=%Rhrc\n", rc));
        return rc;
    }

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aNetworkInterfaces.resize(m->llNetIfs.size());
    size_t i = 0;
    for (HostNetworkInterfaceList::iterator it = m->llNetIfs.begin(); it != m->llNetIfs.end(); ++it, ++i)
        (*it).queryInterfaceTo(aNetworkInterfaces[i].asOutParam());

    return S_OK;
# else
    std::list<ComObjPtr<HostNetworkInterface> > list;

#  if defined(RT_OS_DARWIN)
    PDARWINETHERNIC pEtherNICs = DarwinGetEthernetControllers();
    while (pEtherNICs)
    {
        ComObjPtr<HostNetworkInterface> IfObj;
        IfObj.createObject();
        if (SUCCEEDED(IfObj->init(Bstr(pEtherNICs->szName), Guid(pEtherNICs->Uuid), HostNetworkInterfaceType_Bridged)))
            list.push_back(IfObj);

        /* next, free current */
        void *pvFree = pEtherNICs;
        pEtherNICs = pEtherNICs->pNext;
        RTMemFree(pvFree);
    }

#  elif defined RT_OS_WINDOWS
#   ifndef VBOX_WITH_NETFLT
    hr = E_NOTIMPL;
#   else /* #  if defined VBOX_WITH_NETFLT */
    INetCfg              *pNc;
    INetCfgComponent     *pMpNcc;
    INetCfgComponent     *pTcpIpNcc;
    LPWSTR               lpszApp;
    HRESULT              hr;
    IEnumNetCfgBindingPath      *pEnumBp;
    INetCfgBindingPath          *pBp;
    IEnumNetCfgBindingInterface *pEnumBi;
    INetCfgBindingInterface *pBi;

    /* we are using the INetCfg API for getting the list of miniports */
    hr = VBoxNetCfgWinQueryINetCfg(FALSE,
                                   VBOX_APP_NAME,
                                   &pNc,
                                   &lpszApp);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
#    ifdef VBOX_NETFLT_ONDEMAND_BIND
        /* for the protocol-based approach for now we just get all miniports the MS_TCPIP protocol binds to */
        hr = pNc->FindComponent(L"MS_TCPIP", &pTcpIpNcc);
#    else
        /* for the filter-based approach we get all miniports our filter (oracle_VBoxNetLwf)is bound to */
        hr = pNc->FindComponent(L"oracle_VBoxNetLwf", &pTcpIpNcc);
        if (hr != S_OK)
        {
            /* fall back to NDIS5 miniport lookup (sun_VBoxNetFlt) */
            hr = pNc->FindComponent(L"sun_VBoxNetFlt", &pTcpIpNcc);
        }
#     ifndef VBOX_WITH_HARDENING
        if (hr != S_OK)
        {
            /** @todo try to install the netflt from here */
        }
#     endif

#    endif

        if (hr == S_OK)
        {
            hr = VBoxNetCfgWinGetBindingPathEnum(pTcpIpNcc, EBP_BELOW, &pEnumBp);
            Assert(hr == S_OK);
            if (hr == S_OK)
            {
                hr = VBoxNetCfgWinGetFirstBindingPath(pEnumBp, &pBp);
                Assert(hr == S_OK || hr == S_FALSE);
                while (hr == S_OK)
                {
                    /* S_OK == enabled, S_FALSE == disabled */
                    if (pBp->IsEnabled() == S_OK)
                    {
                        hr = VBoxNetCfgWinGetBindingInterfaceEnum(pBp, &pEnumBi);
                        Assert(hr == S_OK);
                        if (hr == S_OK)
                        {
                            hr = VBoxNetCfgWinGetFirstBindingInterface(pEnumBi, &pBi);
                            Assert(hr == S_OK);
                            while (hr == S_OK)
                            {
                                hr = pBi->GetLowerComponent(&pMpNcc);
                                Assert(hr == S_OK);
                                if (hr == S_OK)
                                {
                                    ULONG uComponentStatus;
                                    hr = pMpNcc->GetDeviceStatus(&uComponentStatus);
                                    Assert(hr == S_OK);
                                    if (hr == S_OK)
                                    {
                                        if (uComponentStatus == 0)
                                        {
                                            vboxNetWinAddComponent(&list, pMpNcc);
                                        }
                                    }
                                    VBoxNetCfgWinReleaseRef(pMpNcc);
                                }
                                VBoxNetCfgWinReleaseRef(pBi);

                                hr = VBoxNetCfgWinGetNextBindingInterface(pEnumBi, &pBi);
                            }
                            VBoxNetCfgWinReleaseRef(pEnumBi);
                        }
                    }
                    VBoxNetCfgWinReleaseRef(pBp);

                    hr = VBoxNetCfgWinGetNextBindingPath(pEnumBp, &pBp);
                }
                VBoxNetCfgWinReleaseRef(pEnumBp);
            }
            VBoxNetCfgWinReleaseRef(pTcpIpNcc);
        }
        else
        {
            LogRel(("failed to get the oracle_VBoxNetLwf(sun_VBoxNetFlt) component, error (0x%x)\n", hr));
        }

        VBoxNetCfgWinReleaseINetCfg(pNc, FALSE);
    }
#   endif /* #  if defined VBOX_WITH_NETFLT */


#  elif defined RT_OS_LINUX
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0)
    {
        char pBuffer[2048];
        struct ifconf ifConf;
        ifConf.ifc_len = sizeof(pBuffer);
        ifConf.ifc_buf = pBuffer;
        if (ioctl(sock, SIOCGIFCONF, &ifConf) >= 0)
        {
            for (struct ifreq *pReq = ifConf.ifc_req; (char*)pReq < pBuffer + ifConf.ifc_len; pReq++)
            {
                if (ioctl(sock, SIOCGIFHWADDR, pReq) >= 0)
                {
                    if (pReq->ifr_hwaddr.sa_family == ARPHRD_ETHER)
                    {
                        RTUUID uuid;
                        Assert(sizeof(uuid) <= sizeof(*pReq));
                        memcpy(&uuid, pReq, sizeof(uuid));

                        ComObjPtr<HostNetworkInterface> IfObj;
                        IfObj.createObject();
                        if (SUCCEEDED(IfObj->init(Bstr(pReq->ifr_name), Guid(uuid), HostNetworkInterfaceType_Bridged)))
                            list.push_back(IfObj);
                    }
                }
            }
        }
        close(sock);
    }
#  endif /* RT_OS_LINUX */

    aNetworkInterfaces.resize(list.size());
    size_t i = 0;
    for (std::list<ComObjPtr<HostNetworkInterface> >::const_iterator it = list.begin(); it != list.end(); ++it, ++i)
        aNetworkInterfaces[i] = *it;

    return S_OK;
# endif
#else
    /* Not implemented / supported on this platform. */
    RT_NOREF(aNetworkInterfaces);
    ReturnComNotImplemented();
#endif
}

HRESULT Host::getUSBDevices(std::vector<ComPtr<IHostUSBDevice> > &aUSBDevices)
{
#ifdef VBOX_WITH_USB
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    MultiResult rc = i_checkUSBProxyService();
    if (FAILED(rc))
        return rc;

    return m->pUSBProxyService->getDeviceCollection(aUSBDevices);
#else
    /* Note: The GUI depends on this method returning E_NOTIMPL with no
     * extended error info to indicate that USB is simply not available
     * (w/o treating it as a failure), for example, as in OSE. */
    NOREF(aUSBDevices);
# ifndef RT_OS_WINDOWS
    NOREF(aUSBDevices);
# endif
    ReturnComNotImplemented();
#endif
}

/**
 * This method return the list of registered name servers
 */
HRESULT Host::getNameServers(std::vector<com::Utf8Str> &aNameServers)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    return m->hostDnsMonitorProxy.GetNameServers(aNameServers);
}


/**
 * This method returns the domain name of the host
 */
HRESULT Host::getDomainName(com::Utf8Str &aDomainName)
{
    /* XXX: note here should be synchronization with thread polling state
     * changes in name resoving system on host */
    return m->hostDnsMonitorProxy.GetDomainName(&aDomainName);
}


/**
 * This method returns the search string.
 */
HRESULT Host::getSearchStrings(std::vector<com::Utf8Str> &aSearchStrings)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    return m->hostDnsMonitorProxy.GetSearchStrings(aSearchStrings);
}

HRESULT Host::getUSBDeviceFilters(std::vector<ComPtr<IHostUSBDeviceFilter> > &aUSBDeviceFilters)
{
#ifdef VBOX_WITH_USB
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    MultiResult rc = i_checkUSBProxyService();
    if (FAILED(rc))
        return rc;

    aUSBDeviceFilters.resize(m->llUSBDeviceFilters.size());
    size_t i = 0;
    for (USBDeviceFilterList::iterator it = m->llUSBDeviceFilters.begin(); it != m->llUSBDeviceFilters.end(); ++it, ++i)
        (*it).queryInterfaceTo(aUSBDeviceFilters[i].asOutParam());

    return rc;
#else
    /* Note: The GUI depends on this method returning E_NOTIMPL with no
     * extended error info to indicate that USB is simply not available
     * (w/o treating it as a failure), for example, as in OSE. */
    NOREF(aUSBDeviceFilters);
# ifndef RT_OS_WINDOWS
    NOREF(aUSBDeviceFilters);
# endif
    ReturnComNotImplemented();
#endif
}

/**
 * Returns the number of installed logical processors
 *
 * @returns COM status code
 * @param   aCount  address of result variable
 */

HRESULT Host::getProcessorCount(ULONG *aCount)
{
    // no locking required

    *aCount = RTMpGetPresentCount();
    return S_OK;
}

/**
 * Returns the number of online logical processors
 *
 * @returns COM status code
 * @param   aCount  address of result variable
 */
HRESULT Host::getProcessorOnlineCount(ULONG *aCount)
{
    // no locking required

    *aCount = RTMpGetOnlineCount();
    return S_OK;
}

/**
 * Returns the number of installed physical processor cores.
 *
 * @returns COM status code
 * @param   aCount  address of result variable
 */
HRESULT Host::getProcessorCoreCount(ULONG *aCount)
{
    // no locking required

    *aCount = RTMpGetPresentCoreCount();
    return S_OK;
}

/**
 * Returns the number of installed physical processor cores.
 *
 * @returns COM status code
 * @param   aCount  address of result variable
 */
HRESULT Host::getProcessorOnlineCoreCount(ULONG *aCount)
{
    // no locking required

    *aCount = RTMpGetOnlineCoreCount();
    return S_OK;
}

/**
 * Returns the (approximate) maximum speed of the given host CPU in MHz
 *
 * @returns COM status code
 * @param   aCpuId  id to get info for.
 * @param   aSpeed  address of result variable, speed is 0 if unknown or aCpuId
 *          is invalid.
 */
HRESULT Host::getProcessorSpeed(ULONG aCpuId,
                                ULONG *aSpeed)
{
    // no locking required

    *aSpeed = RTMpGetMaxFrequency(aCpuId);
    return S_OK;
}

/**
 * Returns a description string for the host CPU
 *
 * @returns COM status code
 * @param   aCpuId  id to get info for.
 * @param   aDescription address of result variable, empty string if not known
 *          or aCpuId is invalid.
 */
HRESULT Host::getProcessorDescription(ULONG aCpuId, com::Utf8Str &aDescription)
{
    // no locking required

    char szCPUModel[80];
    szCPUModel[0] = 0;
    int vrc = RTMpGetDescription(aCpuId, szCPUModel, sizeof(szCPUModel));
    if (RT_FAILURE(vrc))
        return E_FAIL; /** @todo error reporting? */

    aDescription = Utf8Str(szCPUModel);

    return S_OK;
}

/**
 * Returns whether a host processor feature is supported or not
 *
 * @returns COM status code
 * @param   aFeature    to query.
 * @param   aSupported  supported bool result variable
 */
HRESULT Host::getProcessorFeature(ProcessorFeature_T aFeature, BOOL *aSupported)
{
    /* Validate input. */
    switch (aFeature)
    {
        case ProcessorFeature_HWVirtEx:
        case ProcessorFeature_PAE:
        case ProcessorFeature_LongMode:
        case ProcessorFeature_NestedPaging:
            break;
        default:
            return setError(E_INVALIDARG, tr("The aFeature value %d (%#x) is out of range."), (int)aFeature, (int)aFeature);
    }

    /* Do the job. */
    AutoCaller autoCaller(this);
    HRESULT hrc = autoCaller.rc();
    if (SUCCEEDED(hrc))
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

        if (   m->fRecheckVTSupported
            && (   aFeature == ProcessorFeature_HWVirtEx
                || aFeature == ProcessorFeature_NestedPaging)
           )
        {
            alock.release();

            /* Perhaps the driver is available now... */
            int rc = SUPR3InitEx(false /*fUnrestricted*/, NULL);
            if (RT_SUCCESS(rc))
            {
                uint32_t fVTCaps;
                rc = SUPR3QueryVTCaps(&fVTCaps);

                AutoWriteLock wlock(this COMMA_LOCKVAL_SRC_POS);
                if (RT_SUCCESS(rc))
                {
                    Assert(fVTCaps & (SUPVTCAPS_AMD_V | SUPVTCAPS_VT_X));
                    if (fVTCaps & SUPVTCAPS_NESTED_PAGING)
                        m->fNestedPagingSupported = true;
                    else
                        Assert(m->fNestedPagingSupported == false);
                }
                else
                {
                    LogRel(("SUPR0QueryVTCaps -> %Rrc\n", rc));
                    m->fVTSupported = m->fNestedPagingSupported = true;
                }
                rc = SUPR3Term(false);
                AssertRC(rc);
                m->fRecheckVTSupported = false; /* No need to try again, we cached everything. */
            }

            alock.acquire();
        }

        switch (aFeature)
        {
            case ProcessorFeature_HWVirtEx:
                *aSupported = m->fVTSupported;
                break;

            case ProcessorFeature_PAE:
                *aSupported = m->fPAESupported;
                break;

            case ProcessorFeature_LongMode:
                *aSupported = m->fLongModeSupported;
                break;

            case ProcessorFeature_NestedPaging:
                *aSupported = m->fNestedPagingSupported;
                break;

            default:
                AssertFailed();
        }
    }
    return hrc;
}

/**
 * Returns the specific CPUID leaf.
 *
 * @returns COM status code
 * @param   aCpuId              The CPU number. Mostly ignored.
 * @param   aLeaf               The leaf number.
 * @param   aSubLeaf            The sub-leaf number.
 * @param   aValEAX             Where to return EAX.
 * @param   aValEBX             Where to return EBX.
 * @param   aValECX             Where to return ECX.
 * @param   aValEDX             Where to return EDX.
 */
HRESULT Host::getProcessorCPUIDLeaf(ULONG aCpuId, ULONG aLeaf, ULONG aSubLeaf,
                                    ULONG *aValEAX, ULONG *aValEBX, ULONG *aValECX, ULONG *aValEDX)
{
    // no locking required

    /* Check that the CPU is online. */
    /** @todo later use RTMpOnSpecific. */
    if (!RTMpIsCpuOnline(aCpuId))
        return RTMpIsCpuPresent(aCpuId)
             ? setError(E_FAIL, tr("CPU no.%u is not present"), aCpuId)
             : setError(E_FAIL, tr("CPU no.%u is not online"), aCpuId);

    uint32_t uEAX, uEBX, uECX, uEDX;
    ASMCpuId_Idx_ECX(aLeaf, aSubLeaf, &uEAX, &uEBX, &uECX, &uEDX);
    *aValEAX = uEAX;
    *aValEBX = uEBX;
    *aValECX = uECX;
    *aValEDX = uEDX;

    return S_OK;
}

/**
 * Returns the amount of installed system memory in megabytes
 *
 * @returns COM status code
 * @param   aSize   address of result variable
 */
HRESULT Host::getMemorySize(ULONG *aSize)
{
    // no locking required

    uint64_t cb;
    int rc = RTSystemQueryTotalRam(&cb);
    if (RT_FAILURE(rc))
        return E_FAIL;
    *aSize = (ULONG)(cb / _1M);
    return S_OK;
}

/**
 * Returns the current system memory free space in megabytes
 *
 * @returns COM status code
 * @param   aAvailable  address of result variable
 */
HRESULT Host::getMemoryAvailable(ULONG *aAvailable)
{
    // no locking required

    uint64_t cb;
    int rc = RTSystemQueryAvailableRam(&cb);
    if (RT_FAILURE(rc))
        return E_FAIL;
    *aAvailable = (ULONG)(cb / _1M);
    return S_OK;
}

/**
 * Returns the name string of the host operating system
 *
 * @returns COM status code
 * @param   aOperatingSystem result variable
 */
HRESULT Host::getOperatingSystem(com::Utf8Str &aOperatingSystem)
{
    // no locking required

    char szOSName[80];
    int vrc = RTSystemQueryOSInfo(RTSYSOSINFO_PRODUCT, szOSName, sizeof(szOSName));
    if (RT_FAILURE(vrc))
        return E_FAIL; /** @todo error reporting? */
    aOperatingSystem = Utf8Str(szOSName);
    return S_OK;
}

/**
 * Returns the version string of the host operating system
 *
 * @returns COM status code
 * @param   aVersion    address of result variable
 */
HRESULT Host::getOSVersion(com::Utf8Str &aVersion)
{
    // no locking required

    /* Get the OS release. Reserve some buffer space for the service pack. */
    char szOSRelease[128];
    int vrc = RTSystemQueryOSInfo(RTSYSOSINFO_RELEASE, szOSRelease, sizeof(szOSRelease) - 32);
    if (RT_FAILURE(vrc))
        return E_FAIL; /** @todo error reporting? */

    /* Append the service pack if present. */
    char szOSServicePack[80];
    vrc = RTSystemQueryOSInfo(RTSYSOSINFO_SERVICE_PACK, szOSServicePack, sizeof(szOSServicePack));
    if (RT_FAILURE(vrc))
    {
        if (vrc != VERR_NOT_SUPPORTED)
            return E_FAIL; /** @todo error reporting? */
        szOSServicePack[0] = '\0';
    }
    if (szOSServicePack[0] != '\0')
    {
        char *psz = strchr(szOSRelease, '\0');
        RTStrPrintf(psz, &szOSRelease[sizeof(szOSRelease)] - psz, "sp%s", szOSServicePack);
    }

    aVersion = szOSRelease;
    return S_OK;
}

/**
 * Returns the current host time in milliseconds since 1970-01-01 UTC.
 *
 * @returns COM status code
 * @param   aUTCTime    address of result variable
 */
HRESULT Host::getUTCTime(LONG64 *aUTCTime)
{
    // no locking required

    RTTIMESPEC now;
    *aUTCTime = RTTimeSpecGetMilli(RTTimeNow(&now));

    return S_OK;
}


HRESULT Host::getAcceleration3DAvailable(BOOL *aSupported)
{
    HRESULT hrc = S_OK;
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    if (m->f3DAccelerationSupported != -1)
        *aSupported = m->f3DAccelerationSupported;
    else
    {
        alock.release();

#ifdef VBOX_WITH_CROGL
        bool fSupported = VBoxOglIs3DAccelerationSupported();
#else
        bool fSupported = false; /* shoudn't get here, but just in case. */
#endif
        AutoWriteLock alock2(this COMMA_LOCKVAL_SRC_POS);

        m->f3DAccelerationSupported = fSupported;
        alock2.release();
        *aSupported = fSupported;
    }

#ifdef DEBUG_misha
    AssertMsgFailed(("should not be here any more!\n"));
#endif

    return hrc;
}

HRESULT Host::createHostOnlyNetworkInterface(ComPtr<IHostNetworkInterface> &aHostInterface,
                                             ComPtr<IProgress> &aProgress)

{
#ifdef VBOX_WITH_HOSTNETIF_API
    /* No need to lock anything. If there ever will - watch out, the function
     * called below grabs the VirtualBox lock. */

    int r = NetIfCreateHostOnlyNetworkInterface(m->pParent, aHostInterface.asOutParam(), aProgress.asOutParam());
    if (RT_SUCCESS(r))
    {
        if (aHostInterface.isNull())
            return setError(E_FAIL,
                            tr("Unable to create a host network interface"));

#if !defined(RT_OS_WINDOWS)
        Bstr tmpAddr, tmpMask, tmpName;
        HRESULT hrc;
        hrc = aHostInterface->COMGETTER(Name)(tmpName.asOutParam());
        ComAssertComRCRet(hrc, hrc);
        hrc = aHostInterface->COMGETTER(IPAddress)(tmpAddr.asOutParam());
        ComAssertComRCRet(hrc, hrc);
        hrc = aHostInterface->COMGETTER(NetworkMask)(tmpMask.asOutParam());
        ComAssertComRCRet(hrc, hrc);
        /*
         * We need to write the default IP address and mask to extra data now,
         * so the interface gets re-created after vboxnetadp.ko reload.
         * Note that we avoid calling EnableStaticIpConfig since it would
         * change the address on host's interface as well and we want to
         * postpone the change until VM actually starts.
         */
        hrc = m->pParent->SetExtraData(BstrFmt("HostOnly/%ls/IPAddress",
                                               tmpName.raw()).raw(),
                                               tmpAddr.raw());
        ComAssertComRCRet(hrc, hrc);

        hrc = m->pParent->SetExtraData(BstrFmt("HostOnly/%ls/IPNetMask",
                                               tmpName.raw()).raw(),
                                               tmpMask.raw());
        ComAssertComRCRet(hrc, hrc);
#endif
    }

    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT Host::removeHostOnlyNetworkInterface(const com::Guid &aId,
                                             ComPtr<IProgress> &aProgress)

{
#ifdef VBOX_WITH_HOSTNETIF_API
    /* No need to lock anything, the code below does not touch the state
     * of the host object. If that ever changes then check for lock order
     * violations with the called functions. */

    Bstr name;
    HRESULT rc;

    /* first check whether an interface with the given name already exists */
    {
        ComPtr<IHostNetworkInterface> iface;
        rc = findHostNetworkInterfaceById(aId, iface);
        if (FAILED(rc))
            return setError(VBOX_E_OBJECT_NOT_FOUND,
                            tr("Host network interface with UUID {%RTuuid} does not exist"),
                            Guid(aId).raw());
        rc = iface->COMGETTER(Name)(name.asOutParam());
        ComAssertComRCRet(rc, rc);
    }

    int r = NetIfRemoveHostOnlyNetworkInterface(m->pParent, Guid(aId).ref(), aProgress.asOutParam());
    if (RT_SUCCESS(r))
    {
        /* Drop configuration parameters for removed interface */
        rc = m->pParent->SetExtraData(BstrFmt("HostOnly/%ls/IPAddress", name.raw()).raw(), NULL);
        rc = m->pParent->SetExtraData(BstrFmt("HostOnly/%ls/IPNetMask", name.raw()).raw(), NULL);
        rc = m->pParent->SetExtraData(BstrFmt("HostOnly/%ls/IPV6Address", name.raw()).raw(), NULL);
        rc = m->pParent->SetExtraData(BstrFmt("HostOnly/%ls/IPV6NetMask", name.raw()).raw(), NULL);

        return S_OK;
    }

    return r == VERR_NOT_IMPLEMENTED ? E_NOTIMPL : E_FAIL;
#else
    return E_NOTIMPL;
#endif
}

HRESULT Host::createUSBDeviceFilter(const com::Utf8Str &aName,
                                    ComPtr<IHostUSBDeviceFilter> &aFilter)
{
#ifdef VBOX_WITH_USB

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    ComObjPtr<HostUSBDeviceFilter> filter;
    filter.createObject();
    HRESULT rc = filter->init(this, Bstr(aName).raw());
    ComAssertComRCRet(rc, rc);
    rc = filter.queryInterfaceTo(aFilter.asOutParam());
    AssertComRCReturn(rc, rc);
    return S_OK;
#else
    /* Note: The GUI depends on this method returning E_NOTIMPL with no
     * extended error info to indicate that USB is simply not available
     * (w/o treating it as a failure), for example, as in OSE. */
    NOREF(aName);
    NOREF(aFilter);
    ReturnComNotImplemented();
#endif
}

HRESULT Host::insertUSBDeviceFilter(ULONG aPosition,
                                    const ComPtr<IHostUSBDeviceFilter> &aFilter)
{
#ifdef VBOX_WITH_USB
    /* Note: HostUSBDeviceFilter and USBProxyService also uses this lock. */

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    MultiResult rc = i_checkUSBProxyService();
    if (FAILED(rc))
        return rc;

    ComObjPtr<HostUSBDeviceFilter> pFilter;
    for (USBDeviceFilterList::iterator it = m->llChildren.begin();
         it != m->llChildren.end();
         ++it)
    {
        if (*it == aFilter)
        {
            pFilter = *it;
            break;
        }
    }
    if (pFilter.isNull())
        return setError(VBOX_E_INVALID_OBJECT_STATE,
                        tr("The given USB device filter is not created within this VirtualBox instance"));

    if (pFilter->mInList)
        return setError(E_INVALIDARG,
                        tr("The given USB device filter is already in the list"));

    /* iterate to the position... */
    USBDeviceFilterList::iterator itPos = m->llUSBDeviceFilters.begin();
    std::advance(itPos, aPosition);
    /* ...and insert */
    m->llUSBDeviceFilters.insert(itPos, pFilter);
    pFilter->mInList = true;

    /* notify the proxy (only when the filter is active) */
    if (    m->pUSBProxyService->isActive()
         && pFilter->i_getData().mData.fActive)
    {
        ComAssertRet(pFilter->i_getId() == NULL, E_FAIL);
        pFilter->i_getId() = m->pUSBProxyService->insertFilter(&pFilter->i_getData().mUSBFilter);
    }

    // save the global settings; for that we should hold only the VirtualBox lock
    alock.release();
    AutoWriteLock vboxLock(m->pParent COMMA_LOCKVAL_SRC_POS);
    return rc = m->pParent->i_saveSettings();
#else

    /* Note: The GUI depends on this method returning E_NOTIMPL with no
     * extended error info to indicate that USB is simply not available
     * (w/o treating it as a failure), for example, as in OSE. */
    NOREF(aPosition);
    NOREF(aFilter);
    ReturnComNotImplemented();
#endif
}

HRESULT Host::removeUSBDeviceFilter(ULONG aPosition)
{
#ifdef VBOX_WITH_USB

    /* Note: HostUSBDeviceFilter and USBProxyService also uses this lock. */
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    MultiResult rc = i_checkUSBProxyService();
    if (FAILED(rc))
        return rc;

    if (!m->llUSBDeviceFilters.size())
        return setError(E_INVALIDARG,
                        tr("The USB device filter list is empty"));

    if (aPosition >= m->llUSBDeviceFilters.size())
        return setError(E_INVALIDARG,
                        tr("Invalid position: %lu (must be in range [0, %lu])"),
                        aPosition, m->llUSBDeviceFilters.size() - 1);

    ComObjPtr<HostUSBDeviceFilter> filter;
    {
        /* iterate to the position... */
        USBDeviceFilterList::iterator it = m->llUSBDeviceFilters.begin();
        std::advance(it, aPosition);
        /* ...get an element from there... */
        filter = *it;
        /* ...and remove */
        filter->mInList = false;
        m->llUSBDeviceFilters.erase(it);
    }

    /* notify the proxy (only when the filter is active) */
    if (m->pUSBProxyService->isActive() && filter->i_getData().mData.fActive)
    {
        ComAssertRet(filter->i_getId() != NULL, E_FAIL);
        m->pUSBProxyService->removeFilter(filter->i_getId());
        filter->i_getId() = NULL;
    }

    // save the global settings; for that we should hold only the VirtualBox lock
    alock.release();
    AutoWriteLock vboxLock(m->pParent COMMA_LOCKVAL_SRC_POS);
    return rc = m->pParent->i_saveSettings();
#else
    /* Note: The GUI depends on this method returning E_NOTIMPL with no
     * extended error info to indicate that USB is simply not available
     * (w/o treating it as a failure), for example, as in OSE. */
    NOREF(aPosition);
    ReturnComNotImplemented();
#endif
}

HRESULT Host::findHostDVDDrive(const com::Utf8Str &aName,
                               ComPtr<IMedium> &aDrive)
{
    ComObjPtr<Medium> medium;
    HRESULT rc = i_findHostDriveByNameOrId(DeviceType_DVD, aName, medium);
    if (SUCCEEDED(rc))
        rc = medium.queryInterfaceTo(aDrive.asOutParam());
    else
        rc = setError(rc, Medium::tr("The host DVD drive named '%s' could not be found"), aName.c_str());
    return rc;
}

HRESULT Host::findHostFloppyDrive(const com::Utf8Str &aName, ComPtr<IMedium> &aDrive)
{
    aDrive = NULL;

    ComObjPtr<Medium>medium;

    HRESULT rc = i_findHostDriveByNameOrId(DeviceType_Floppy, aName, medium);
    if (SUCCEEDED(rc))
        return medium.queryInterfaceTo(aDrive.asOutParam());
    else
        return setError(rc, Medium::tr("The host floppy drive named '%s' could not be found"), aName.c_str());
}

HRESULT Host::findHostNetworkInterfaceByName(const com::Utf8Str &aName,
                                             ComPtr<IHostNetworkInterface> &aNetworkInterface)
{
#ifndef VBOX_WITH_HOSTNETIF_API
    return E_NOTIMPL;
#else
    if (!aName.length())
        return E_INVALIDARG;

    HRESULT rc = i_updateNetIfList();
    if (FAILED(rc))
    {
        Log(("Failed to update host network interface list with rc=%Rhrc\n", rc));
        return rc;
    }

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    ComObjPtr<HostNetworkInterface> found;
    for (HostNetworkInterfaceList::iterator it = m->llNetIfs.begin(); it != m->llNetIfs.end(); ++it)
    {
        Bstr n;
        (*it)->COMGETTER(Name)(n.asOutParam());
        if (n == aName)
            found = *it;
    }

    if (!found)
        return setError(E_INVALIDARG,
                        HostNetworkInterface::tr("The host network interface named '%s' could not be found"), aName.c_str());

    return found.queryInterfaceTo(aNetworkInterface.asOutParam());
#endif
}

HRESULT Host::findHostNetworkInterfaceById(const com::Guid &aId,
                                           ComPtr<IHostNetworkInterface> &aNetworkInterface)
{
#ifndef VBOX_WITH_HOSTNETIF_API
    return E_NOTIMPL;
#else
    if (!aId.isValid())
        return E_INVALIDARG;

    HRESULT rc = i_updateNetIfList();
    if (FAILED(rc))
    {
        Log(("Failed to update host network interface list with rc=%Rhrc\n", rc));
        return rc;
    }

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    ComObjPtr<HostNetworkInterface> found;
    for (HostNetworkInterfaceList::iterator it = m->llNetIfs.begin(); it != m->llNetIfs.end(); ++it)
    {
        Bstr g;
        (*it)->COMGETTER(Id)(g.asOutParam());
        if (Guid(g) == aId)
            found = *it;
    }

    if (!found)
        return setError(E_INVALIDARG,
                        HostNetworkInterface::tr("The host network interface with the given GUID could not be found"));
    return found.queryInterfaceTo(aNetworkInterface.asOutParam());

#endif
}

HRESULT Host::findHostNetworkInterfacesOfType(HostNetworkInterfaceType_T aType,
                                              std::vector<ComPtr<IHostNetworkInterface> > &aNetworkInterfaces)
{
#ifdef VBOX_WITH_HOSTNETIF_API
    HRESULT rc = i_updateNetIfList();
    if (FAILED(rc))
    {
        Log(("Failed to update host network interface list with rc=%Rhrc\n", rc));
        return rc;
    }

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    HostNetworkInterfaceList resultList;
    for (HostNetworkInterfaceList::iterator it = m->llNetIfs.begin(); it != m->llNetIfs.end(); ++it)
    {
        HostNetworkInterfaceType_T t;
        HRESULT hr = (*it)->COMGETTER(InterfaceType)(&t);
        if (FAILED(hr))
            return hr;

        if (t == aType)
            resultList.push_back(*it);
    }
    aNetworkInterfaces.resize(resultList.size());
    size_t i = 0;
    for (HostNetworkInterfaceList::iterator it = resultList.begin(); it != resultList.end(); ++it, ++i)
    {
        (*it).queryInterfaceTo(aNetworkInterfaces[i].asOutParam());
    }

    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

HRESULT Host::findUSBDeviceByAddress(const com::Utf8Str &aName,
                                     ComPtr<IHostUSBDevice> &aDevice)
{
#ifdef VBOX_WITH_USB

    aDevice = NULL;
    SafeIfaceArray<IHostUSBDevice> devsvec;
    HRESULT rc = COMGETTER(USBDevices)(ComSafeArrayAsOutParam(devsvec));
    if (FAILED(rc))
        return rc;

    for (size_t i = 0; i < devsvec.size(); ++i)
    {
        Bstr address;
        rc = devsvec[i]->COMGETTER(Address)(address.asOutParam());
        if (FAILED(rc))
            return rc;
        if (address == aName)
        {
            return (ComPtr<IHostUSBDevice>(devsvec[i]).queryInterfaceTo(aDevice.asOutParam()));
        }
    }

    return setErrorNoLog(VBOX_E_OBJECT_NOT_FOUND,
                         tr("Could not find a USB device with address '%s'"),
                         aName.c_str());

#else   /* !VBOX_WITH_USB */
    NOREF(aName);
    NOREF(aDevice);
    return E_NOTIMPL;
#endif  /* !VBOX_WITH_USB */
}
HRESULT Host::findUSBDeviceById(const com::Guid &aId,
                                ComPtr<IHostUSBDevice> &aDevice)
{
#ifdef VBOX_WITH_USB
    if (!aId.isValid())
        return E_INVALIDARG;

    aDevice = NULL;

    SafeIfaceArray<IHostUSBDevice> devsvec;
    HRESULT rc = COMGETTER(USBDevices)(ComSafeArrayAsOutParam(devsvec));
    if (FAILED(rc))
        return rc;

    for (size_t i = 0; i < devsvec.size(); ++i)
    {
        Bstr id;
        rc = devsvec[i]->COMGETTER(Id)(id.asOutParam());
        if (FAILED(rc))
            return rc;
        if (Guid(id) == aId)
        {
            return (ComPtr<IHostUSBDevice>(devsvec[i]).queryInterfaceTo(aDevice.asOutParam()));
        }
    }
    return setErrorNoLog(VBOX_E_OBJECT_NOT_FOUND,
                         tr("Could not find a USB device with uuid {%RTuuid}"),
                         aId.raw());

#else   /* !VBOX_WITH_USB */
    NOREF(aId);
    NOREF(aDevice);
    return E_NOTIMPL;
#endif  /* !VBOX_WITH_USB */
}

HRESULT Host::generateMACAddress(com::Utf8Str &aAddress)
{
    // no locking required
    i_generateMACAddress(aAddress);
    return S_OK;
}

/**
 * Returns a list of host video capture devices (webcams, etc).
 *
 * @returns COM status code
 * @param aVideoInputDevices Array of interface pointers to be filled.
 */
HRESULT Host::getVideoInputDevices(std::vector<ComPtr<IHostVideoInputDevice> > &aVideoInputDevices)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    HostVideoInputDeviceList list;

    HRESULT rc = HostVideoInputDevice::queryHostDevices(m->pParent, &list);
    if (FAILED(rc))
        return rc;

    aVideoInputDevices.resize(list.size());
    size_t i = 0;
    for (HostVideoInputDeviceList::const_iterator it = list.begin(); it != list.end(); ++it, ++i)
        (*it).queryInterfaceTo(aVideoInputDevices[i].asOutParam());

    return S_OK;
}

HRESULT Host::addUSBDeviceSource(const com::Utf8Str &aBackend, const com::Utf8Str &aId, const com::Utf8Str &aAddress,
                                 const std::vector<com::Utf8Str> &aPropertyNames, const std::vector<com::Utf8Str> &aPropertyValues)
{
#ifdef VBOX_WITH_USB
    /* The USB proxy service will do the locking. */
    return m->pUSBProxyService->addUSBDeviceSource(aBackend, aId, aAddress, aPropertyNames, aPropertyValues);
#else
    ReturnComNotImplemented();
#endif
}

HRESULT Host::removeUSBDeviceSource(const com::Utf8Str &aId)
{
#ifdef VBOX_WITH_USB
    /* The USB proxy service will do the locking. */
    return m->pUSBProxyService->removeUSBDeviceSource(aId);
#else
    ReturnComNotImplemented();
#endif
}

// public methods only for internal purposes
////////////////////////////////////////////////////////////////////////////////

HRESULT Host::i_loadSettings(const settings::Host &data)
{
    HRESULT rc = S_OK;
#ifdef VBOX_WITH_USB
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc()))
        return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    for (settings::USBDeviceFiltersList::const_iterator it = data.llUSBDeviceFilters.begin();
         it != data.llUSBDeviceFilters.end();
         ++it)
    {
        const settings::USBDeviceFilter &f = *it;
        ComObjPtr<HostUSBDeviceFilter> pFilter;
        pFilter.createObject();
        rc = pFilter->init(this, f);
        if (FAILED(rc))
            break;

        m->llUSBDeviceFilters.push_back(pFilter);
        pFilter->mInList = true;

        /* notify the proxy (only when the filter is active) */
        if (pFilter->i_getData().mData.fActive)
        {
            HostUSBDeviceFilter *flt = pFilter; /* resolve ambiguity */
            flt->i_getId() = m->pUSBProxyService->insertFilter(&pFilter->i_getData().mUSBFilter);
        }
    }

    rc = m->pUSBProxyService->i_loadSettings(data.llUSBDeviceSources);
#else
    NOREF(data);
#endif /* VBOX_WITH_USB */
    return rc;
}

HRESULT Host::i_saveSettings(settings::Host &data)
{
#ifdef VBOX_WITH_USB
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc()))
        return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    data.llUSBDeviceFilters.clear();
    data.llUSBDeviceSources.clear();

    for (USBDeviceFilterList::const_iterator it = m->llUSBDeviceFilters.begin();
         it != m->llUSBDeviceFilters.end();
         ++it)
    {
        ComObjPtr<HostUSBDeviceFilter> pFilter = *it;
        settings::USBDeviceFilter f;
        pFilter->i_saveSettings(f);
        data.llUSBDeviceFilters.push_back(f);
    }

    return m->pUSBProxyService->i_saveSettings(data.llUSBDeviceSources);
#else
    NOREF(data);
    return S_OK;
#endif /* VBOX_WITH_USB */

}

/**
 * Sets the given pointer to point to the static list of DVD or floppy
 * drives in the Host instance data, depending on the @a mediumType
 * parameter.
 *
 * This builds the list on the first call; it adds or removes host drives
 * that may have changed if fRefresh == true.
 *
 * The caller must hold the medium tree write lock before calling this.
 * To protect the list to which the caller's pointer points, the caller
 * must also hold that lock.
 *
 * @param mediumType Must be DeviceType_Floppy or DeviceType_DVD.
 * @param fRefresh Whether to refresh the host drives list even if this is not the first call.
 * @param pll Caller's pointer which gets set to the static list of host drives.
 * @param treeLock Reference to media tree lock, need to drop it temporarily.
 * @returns COM status code
 */
HRESULT Host::i_getDrives(DeviceType_T mediumType,
                          bool fRefresh,
                          MediaList *&pll,
                          AutoWriteLock &treeLock)
{
    HRESULT rc = S_OK;
    Assert(m->pParent->i_getMediaTreeLockHandle().isWriteLockOnCurrentThread());

    MediaList llNew;
    MediaList *pllCached;
    bool *pfListBuilt = NULL;

    switch (mediumType)
    {
        case DeviceType_DVD:
            if (!m->fDVDDrivesListBuilt || fRefresh)
            {
                rc = i_buildDVDDrivesList(llNew);
                if (FAILED(rc))
                    return rc;
                pfListBuilt = &m->fDVDDrivesListBuilt;
            }
            pllCached = &m->llDVDDrives;
        break;

        case DeviceType_Floppy:
            if (!m->fFloppyDrivesListBuilt || fRefresh)
            {
                rc = i_buildFloppyDrivesList(llNew);
                if (FAILED(rc))
                    return rc;
                pfListBuilt = &m->fFloppyDrivesListBuilt;
            }
            pllCached = &m->llFloppyDrives;
        break;

        default:
            return E_INVALIDARG;
    }

    if (pfListBuilt)
    {
        // a list was built in llNew above:
        if (!*pfListBuilt)
        {
            // this was the first call (instance bool is still false): then just copy the whole list and return
            *pllCached = llNew;
            // and mark the instance data as "built"
            *pfListBuilt = true;
        }
        else
        {
            // list was built, and this was a subsequent call: then compare the old and the new lists

            // remove drives from the cached list which are no longer present
            for (MediaList::iterator itCached = pllCached->begin();
                 itCached != pllCached->end();
                 /*nothing */)
            {
                Medium *pCached = *itCached;
                const Utf8Str strLocationCached = pCached->i_getLocationFull();
                bool fFound = false;
                for (MediaList::iterator itNew = llNew.begin();
                     itNew != llNew.end();
                     ++itNew)
                {
                    Medium *pNew = *itNew;
                    const Utf8Str strLocationNew = pNew->i_getLocationFull();
                    if (strLocationNew == strLocationCached)
                    {
                        fFound = true;
                        break;
                    }
                }
                if (!fFound)
                {
                    pCached->uninit();
                    itCached = pllCached->erase(itCached);
                }
                else
                    ++itCached;
            }

            // add drives to the cached list that are not on there yet
            for (MediaList::iterator itNew = llNew.begin();
                 itNew != llNew.end();
                 ++itNew)
            {
                Medium *pNew = *itNew;
                const Utf8Str strLocationNew = pNew->i_getLocationFull();
                bool fFound = false;
                for (MediaList::iterator itCached = pllCached->begin();
                     itCached != pllCached->end();
                     ++itCached)
                {
                    Medium *pCached = *itCached;
                    const Utf8Str strLocationCached = pCached->i_getLocationFull();
                    if (strLocationNew == strLocationCached)
                    {
                        fFound = true;
                        break;
                    }
                }

                if (!fFound)
                    pllCached->push_back(pNew);
            }
        }
    }

    // return cached list to caller
    pll = pllCached;

    // Make sure the media tree lock is released before llNew is cleared,
    // as this usually triggers calls to uninit().
    treeLock.release();

    llNew.clear();

    treeLock.acquire();

    return rc;
}

/**
 * Goes through the list of host drives that would be returned by getDrives()
 * and looks for a host drive with the given UUID. If found, it sets pMedium
 * to that drive; otherwise returns VBOX_E_OBJECT_NOT_FOUND.
 *
 * @param mediumType Must be DeviceType_DVD or DeviceType_Floppy.
 * @param uuid Medium UUID of host drive to look for.
 * @param fRefresh Whether to refresh the host drives list (see getDrives())
 * @param pMedium Medium object, if found...
 * @return VBOX_E_OBJECT_NOT_FOUND if not found, or S_OK if found, or errors from getDrives().
 */
HRESULT Host::i_findHostDriveById(DeviceType_T mediumType,
                                  const Guid &uuid,
                                  bool fRefresh,
                                  ComObjPtr<Medium> &pMedium)
{
    MediaList *pllMedia;

    AutoWriteLock treeLock(m->pParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = i_getDrives(mediumType, fRefresh, pllMedia, treeLock);
    if (SUCCEEDED(rc))
    {
        for (MediaList::iterator it = pllMedia->begin();
             it != pllMedia->end();
             ++it)
        {
            Medium *pThis = *it;
            AutoCaller mediumCaller(pThis);
            AutoReadLock mediumLock(pThis COMMA_LOCKVAL_SRC_POS);
            if (pThis->i_getId() == uuid)
            {
                pMedium = pThis;
                return S_OK;
            }
        }
    }

    return VBOX_E_OBJECT_NOT_FOUND;
}

/**
 * Goes through the list of host drives that would be returned by getDrives()
 * and looks for a host drive with the given name. If found, it sets pMedium
 * to that drive; otherwise returns VBOX_E_OBJECT_NOT_FOUND.
 *
 * @param mediumType Must be DeviceType_DVD or DeviceType_Floppy.
 * @param strLocationFull Name (path) of host drive to look for.
 * @param fRefresh Whether to refresh the host drives list (see getDrives())
 * @param pMedium Medium object, if found
 * @return VBOX_E_OBJECT_NOT_FOUND if not found, or S_OK if found, or errors from getDrives().
 */
HRESULT Host::i_findHostDriveByName(DeviceType_T mediumType,
                                    const Utf8Str &strLocationFull,
                                    bool fRefresh,
                                    ComObjPtr<Medium> &pMedium)
{
    MediaList *pllMedia;

    AutoWriteLock treeLock(m->pParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = i_getDrives(mediumType, fRefresh, pllMedia, treeLock);
    if (SUCCEEDED(rc))
    {
        for (MediaList::iterator it = pllMedia->begin();
             it != pllMedia->end();
             ++it)
        {
            Medium *pThis = *it;
            AutoCaller mediumCaller(pThis);
            AutoReadLock mediumLock(pThis COMMA_LOCKVAL_SRC_POS);
            if (pThis->i_getLocationFull() == strLocationFull)
            {
                pMedium = pThis;
                return S_OK;
            }
        }
    }

    return VBOX_E_OBJECT_NOT_FOUND;
}

/**
 * Goes through the list of host drives that would be returned by getDrives()
 * and looks for a host drive with the given name, location or ID. If found,
 * it sets pMedium to that drive; otherwise returns VBOX_E_OBJECT_NOT_FOUND.
 *
 * @param mediumType  Must be DeviceType_DVD or DeviceType_Floppy.
 * @param strNameOrId Name or full location or UUID of host drive to look for.
 * @param pMedium     Medium object, if found...
 * @return VBOX_E_OBJECT_NOT_FOUND if not found, or S_OK if found, or errors from getDrives().
 */
HRESULT Host::i_findHostDriveByNameOrId(DeviceType_T mediumType,
                                        const Utf8Str &strNameOrId,
                                        ComObjPtr<Medium> &pMedium)
{
    AutoWriteLock wlock(m->pParent->i_getMediaTreeLockHandle() COMMA_LOCKVAL_SRC_POS);

    Guid uuid(strNameOrId);
    if (uuid.isValid() && !uuid.isZero())
        return i_findHostDriveById(mediumType, uuid, true /* fRefresh */, pMedium);

    // string is not a syntactically valid UUID: try a name then
    return i_findHostDriveByName(mediumType, strNameOrId, true /* fRefresh */, pMedium);
}

/**
 * Called from getDrives() to build the DVD drives list.
 * @param   list    Media list
 * @return
 */
HRESULT Host::i_buildDVDDrivesList(MediaList &list)
{
    HRESULT rc = S_OK;

    Assert(m->pParent->i_getMediaTreeLockHandle().isWriteLockOnCurrentThread());

    try
    {
#if defined(RT_OS_WINDOWS)
        int sz = GetLogicalDriveStrings(0, NULL);
        TCHAR *hostDrives = new TCHAR[sz+1];
        GetLogicalDriveStrings(sz, hostDrives);
        wchar_t driveName[3] = { '?', ':', '\0' };
        TCHAR *p = hostDrives;
        do
        {
            if (GetDriveType(p) == DRIVE_CDROM)
            {
                driveName[0] = *p;
                ComObjPtr<Medium> hostDVDDriveObj;
                hostDVDDriveObj.createObject();
                hostDVDDriveObj->init(m->pParent, DeviceType_DVD, Bstr(driveName));
                list.push_back(hostDVDDriveObj);
            }
            p += _tcslen(p) + 1;
        }
        while (*p);
        delete[] hostDrives;

#elif defined(RT_OS_SOLARIS)
# ifdef VBOX_USE_LIBHAL
        if (!i_getDVDInfoFromHal(list))
# endif
        {
            i_getDVDInfoFromDevTree(list);
        }

#elif defined(RT_OS_LINUX) || defined(RT_OS_FREEBSD)
        if (RT_SUCCESS(m->hostDrives.updateDVDs()))
            for (DriveInfoList::const_iterator it = m->hostDrives.DVDBegin();
                SUCCEEDED(rc) && it != m->hostDrives.DVDEnd(); ++it)
            {
                ComObjPtr<Medium> hostDVDDriveObj;
                Utf8Str location(it->mDevice);
                Utf8Str description(it->mDescription);
                if (SUCCEEDED(rc))
                    rc = hostDVDDriveObj.createObject();
                if (SUCCEEDED(rc))
                    rc = hostDVDDriveObj->init(m->pParent, DeviceType_DVD, location, description);
                if (SUCCEEDED(rc))
                    list.push_back(hostDVDDriveObj);
            }
#elif defined(RT_OS_DARWIN)
        PDARWINDVD cur = DarwinGetDVDDrives();
        while (cur)
        {
            ComObjPtr<Medium> hostDVDDriveObj;
            hostDVDDriveObj.createObject();
            hostDVDDriveObj->init(m->pParent, DeviceType_DVD, Bstr(cur->szName));
            list.push_back(hostDVDDriveObj);

            /* next */
            void *freeMe = cur;
            cur = cur->pNext;
            RTMemFree(freeMe);
        }
#else
    /* PORTME */
#endif
    }
    catch(std::bad_alloc &)
    {
        rc = E_OUTOFMEMORY;
    }
    return rc;
}

/**
 * Called from getDrives() to build the floppy drives list.
 * @param list
 * @return
 */
HRESULT Host::i_buildFloppyDrivesList(MediaList &list)
{
    HRESULT rc = S_OK;

    Assert(m->pParent->i_getMediaTreeLockHandle().isWriteLockOnCurrentThread());

    try
    {
#ifdef RT_OS_WINDOWS
        int sz = GetLogicalDriveStrings(0, NULL);
        TCHAR *hostDrives = new TCHAR[sz+1];
        GetLogicalDriveStrings(sz, hostDrives);
        wchar_t driveName[3] = { '?', ':', '\0' };
        TCHAR *p = hostDrives;
        do
        {
            if (GetDriveType(p) == DRIVE_REMOVABLE)
            {
                driveName[0] = *p;
                ComObjPtr<Medium> hostFloppyDriveObj;
                hostFloppyDriveObj.createObject();
                hostFloppyDriveObj->init(m->pParent, DeviceType_Floppy, Bstr(driveName));
                list.push_back(hostFloppyDriveObj);
            }
            p += _tcslen(p) + 1;
        }
        while (*p);
        delete[] hostDrives;
#elif defined(RT_OS_LINUX)
        if (RT_SUCCESS(m->hostDrives.updateFloppies()))
            for (DriveInfoList::const_iterator it = m->hostDrives.FloppyBegin();
                SUCCEEDED(rc) && it != m->hostDrives.FloppyEnd(); ++it)
            {
                ComObjPtr<Medium> hostFloppyDriveObj;
                Utf8Str location(it->mDevice);
                Utf8Str description(it->mDescription);
                if (SUCCEEDED(rc))
                    rc = hostFloppyDriveObj.createObject();
                if (SUCCEEDED(rc))
                    rc = hostFloppyDriveObj->init(m->pParent, DeviceType_Floppy, location, description);
                if (SUCCEEDED(rc))
                    list.push_back(hostFloppyDriveObj);
            }
#else
    NOREF(list);
    /* PORTME */
#endif
    }
    catch(std::bad_alloc &)
    {
        rc = E_OUTOFMEMORY;
    }

    return rc;
}

#ifdef VBOX_WITH_USB
USBProxyService* Host::i_usbProxyService()
{
    return m->pUSBProxyService;
}

HRESULT Host::i_addChild(HostUSBDeviceFilter *pChild)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc()))
        return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->llChildren.push_back(pChild);

    return S_OK;
}

HRESULT Host::i_removeChild(HostUSBDeviceFilter *pChild)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc()))
        return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    for (USBDeviceFilterList::iterator it = m->llChildren.begin();
         it != m->llChildren.end();
         ++it)
    {
        if (*it == pChild)
        {
            m->llChildren.erase(it);
            break;
        }
    }

    return S_OK;
}

VirtualBox* Host::i_parent()
{
    return m->pParent;
}

/**
 *  Called by setter methods of all USB device filters.
 */
HRESULT Host::i_onUSBDeviceFilterChange(HostUSBDeviceFilter *aFilter,
                                        BOOL aActiveChanged /* = FALSE */)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc()))
        return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (aFilter->mInList)
    {
        if (aActiveChanged)
        {
            // insert/remove the filter from the proxy
            if (aFilter->i_getData().mData.fActive)
            {
                ComAssertRet(aFilter->i_getId() == NULL, E_FAIL);
                aFilter->i_getId() = m->pUSBProxyService->insertFilter(&aFilter->i_getData().mUSBFilter);
            }
            else
            {
                ComAssertRet(aFilter->i_getId() != NULL, E_FAIL);
                m->pUSBProxyService->removeFilter(aFilter->i_getId());
                aFilter->i_getId() = NULL;
            }
        }
        else
        {
            if (aFilter->i_getData().mData.fActive)
            {
                // update the filter in the proxy
                ComAssertRet(aFilter->i_getId() != NULL, E_FAIL);
                m->pUSBProxyService->removeFilter(aFilter->i_getId());
                aFilter->i_getId() = m->pUSBProxyService->insertFilter(&aFilter->i_getData().mUSBFilter);
            }
        }

        // save the global settings... yeah, on every single filter property change
        // for that we should hold only the VirtualBox lock
        alock.release();
        AutoWriteLock vboxLock(m->pParent COMMA_LOCKVAL_SRC_POS);
        return m->pParent->i_saveSettings();
    }

    return S_OK;
}


/**
 * Interface for obtaining a copy of the USBDeviceFilterList,
 * used by the USBProxyService.
 *
 * @param   aGlobalFilters      Where to put the global filter list copy.
 * @param   aMachines           Where to put the machine vector.
 */
void Host::i_getUSBFilters(Host::USBDeviceFilterList *aGlobalFilters)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aGlobalFilters = m->llUSBDeviceFilters;
}

#endif /* VBOX_WITH_USB */

// private methods
////////////////////////////////////////////////////////////////////////////////

#if defined(RT_OS_SOLARIS) && defined(VBOX_USE_LIBHAL)

/**
 * Helper function to get the slice number from a device path
 *
 * @param   pszDevLinkPath      Pointer to a device path (/dev/(r)dsk/c7d1t0d0s3 etc.)
 * @returns Pointer to the slice portion of the given path.
 */
static char *solarisGetSliceFromPath(const char *pszDevLinkPath)
{
    char *pszFound = NULL;
    char *pszSlice = strrchr(pszDevLinkPath, 's');
    char *pszDisk  = strrchr(pszDevLinkPath, 'd');
    if (pszSlice && pszSlice > pszDisk)
        pszFound = pszSlice;
    else
        pszFound = pszDisk;

    if (pszFound && RT_C_IS_DIGIT(pszFound[1]))
        return pszFound;

    return NULL;
}

/**
 * Walk device links and returns an allocated path for the first one in the snapshot.
 *
 * @param   DevLink     Handle to the device link being walked.
 * @param   pvArg       Opaque data containing the pointer to the path.
 * @returns Pointer to an allocated device path string.
 */
static int solarisWalkDevLink(di_devlink_t DevLink, void *pvArg)
{
    char **ppszPath = (char **)pvArg;
    *ppszPath = strdup(di_devlink_path(DevLink));
    return DI_WALK_TERMINATE;
}

/**
 * Walk all devices in the system and enumerate CD/DVD drives.
 * @param   Node        Handle to the current node.
 * @param   pvArg       Opaque data (holds list pointer).
 * @returns Solaris specific code whether to continue walking or not.
 */
static int solarisWalkDeviceNodeForDVD(di_node_t Node, void *pvArg)
{
    PSOLARISDVD *ppDrives = (PSOLARISDVD *)pvArg;

    /*
     * Check for "removable-media" or "hotpluggable" instead of "SCSI" so that we also include USB CD-ROMs.
     * As unfortunately the Solaris drivers only export these common properties.
     */
    int *pInt = NULL;
    if (   di_prop_lookup_ints(DDI_DEV_T_ANY, Node, "removable-media", &pInt) >= 0
        || di_prop_lookup_ints(DDI_DEV_T_ANY, Node, "hotpluggable", &pInt) >= 0)
    {
        if (di_prop_lookup_ints(DDI_DEV_T_ANY, Node, "inquiry-device-type", &pInt) > 0
            && (   *pInt == DTYPE_RODIRECT                                              /* CDROM */
                || *pInt == DTYPE_OPTICAL))                                             /* Optical Drive */
        {
            char *pszProduct = NULL;
            if (di_prop_lookup_strings(DDI_DEV_T_ANY, Node, "inquiry-product-id", &pszProduct) > 0)
            {
                char *pszVendor = NULL;
                if (di_prop_lookup_strings(DDI_DEV_T_ANY, Node, "inquiry-vendor-id", &pszVendor) > 0)
                {
                    /*
                     * Found a DVD drive, we need to scan the minor nodes to find the correct
                     * slice that represents the whole drive. "s2" is always the whole drive for CD/DVDs.
                     */
                    int Major = di_driver_major(Node);
                    di_minor_t Minor = DI_MINOR_NIL;
                    di_devlink_handle_t DevLink = di_devlink_init(NULL /* name */, 0 /* flags */);
                    if (DevLink)
                    {
                        while ((Minor = di_minor_next(Node, Minor)) != DI_MINOR_NIL)
                        {
                            dev_t Dev = di_minor_devt(Minor);
                            if (   Major != (int)major(Dev)
                                || di_minor_spectype(Minor) == S_IFBLK
                                || di_minor_type(Minor) != DDM_MINOR)
                            {
                                continue;
                            }

                            char *pszMinorPath = di_devfs_minor_path(Minor);
                            if (!pszMinorPath)
                                continue;

                            char *pszDevLinkPath = NULL;
                            di_devlink_walk(DevLink, NULL, pszMinorPath, DI_PRIMARY_LINK, &pszDevLinkPath, solarisWalkDevLink);
                            di_devfs_path_free(pszMinorPath);

                            if (pszDevLinkPath)
                            {
                                char *pszSlice = solarisGetSliceFromPath(pszDevLinkPath);
                                if (   pszSlice && !strcmp(pszSlice, "s2")
                                    && !strncmp(pszDevLinkPath, RT_STR_TUPLE("/dev/rdsk")))   /* We want only raw disks */
                                {
                                    /*
                                     * We've got a fully qualified DVD drive. Add it to the list.
                                     */
                                    PSOLARISDVD pDrive = (PSOLARISDVD)RTMemAllocZ(sizeof(SOLARISDVD));
                                    if (RT_LIKELY(pDrive))
                                    {
                                        RTStrPrintf(pDrive->szDescription, sizeof(pDrive->szDescription),
                                                    "%s %s", pszVendor, pszProduct);
                                        RTStrCopy(pDrive->szRawDiskPath, sizeof(pDrive->szRawDiskPath), pszDevLinkPath);
                                        if (*ppDrives)
                                            pDrive->pNext = *ppDrives;
                                        *ppDrives = pDrive;

                                        /* We're not interested in any of the other slices, stop minor nodes traversal. */
                                        free(pszDevLinkPath);
                                        break;
                                    }
                                }
                                free(pszDevLinkPath);
                            }
                        }
                        di_devlink_fini(&DevLink);
                    }
                }
            }
        }
    }
    return DI_WALK_CONTINUE;
}

/**
 * Solaris specific function to enumerate CD/DVD drives via the device tree.
 * Works on Solaris 10 as well as OpenSolaris without depending on libhal.
 */
void Host::i_getDVDInfoFromDevTree(std::list<ComObjPtr<Medium> > &list)
{
    PSOLARISDVD pDrives = NULL;
    di_node_t RootNode = di_init("/", DINFOCPYALL);
    if (RootNode != DI_NODE_NIL)
        di_walk_node(RootNode, DI_WALK_CLDFIRST, &pDrives, solarisWalkDeviceNodeForDVD);

    di_fini(RootNode);

    while (pDrives)
    {
        ComObjPtr<Medium> hostDVDDriveObj;
        hostDVDDriveObj.createObject();
        hostDVDDriveObj->init(m->pParent, DeviceType_DVD, Bstr(pDrives->szRawDiskPath), Bstr(pDrives->szDescription));
        list.push_back(hostDVDDriveObj);

        void *pvDrive = pDrives;
        pDrives = pDrives->pNext;
        RTMemFree(pvDrive);
    }
}

/* Solaris hosts, loading libhal at runtime */

/**
 * Helper function to query the hal subsystem for information about DVD drives attached to the
 * system.
 *
 * @returns true if information was successfully obtained, false otherwise
 * @retval  list drives found will be attached to this list
 */
bool Host::i_getDVDInfoFromHal(std::list<ComObjPtr<Medium> > &list)
{
    bool halSuccess = false;
    DBusError dbusError;
    if (!gLibHalCheckPresence())
        return false;
    gDBusErrorInit(&dbusError);
    DBusConnection *dbusConnection = gDBusBusGet(DBUS_BUS_SYSTEM, &dbusError);
    if (dbusConnection != 0)
    {
        LibHalContext *halContext = gLibHalCtxNew();
        if (halContext != 0)
        {
            if (gLibHalCtxSetDBusConnection(halContext, dbusConnection))
            {
                if (gLibHalCtxInit(halContext, &dbusError))
                {
                    int numDevices;
                    char **halDevices = gLibHalFindDeviceStringMatch(halContext,
                                                "storage.drive_type", "cdrom",
                                                &numDevices, &dbusError);
                    if (halDevices != 0)
                    {
                        /* Hal is installed and working, so if no devices are reported, assume
                           that there are none. */
                        halSuccess = true;
                        for (int i = 0; i < numDevices; i++)
                        {
                            char *devNode = gLibHalDeviceGetPropertyString(halContext,
                                                    halDevices[i], "block.device", &dbusError);
#ifdef RT_OS_SOLARIS
                            /* The CD/DVD ioctls work only for raw device nodes. */
                            char *tmp = getfullrawname(devNode);
                            gLibHalFreeString(devNode);
                            devNode = tmp;
#endif

                            if (devNode != 0)
                            {
//                                if (validateDevice(devNode, true))
//                                {
                                    Utf8Str description;
                                    char *vendor, *product;
                                    /* We do not check the error here, as this field may
                                       not even exist. */
                                    vendor = gLibHalDeviceGetPropertyString(halContext,
                                                    halDevices[i], "info.vendor", 0);
                                    product = gLibHalDeviceGetPropertyString(halContext,
                                                    halDevices[i], "info.product", &dbusError);
                                    if ((product != 0 && product[0] != 0))
                                    {
                                        if ((vendor != 0) && (vendor[0] != 0))
                                        {
                                            description = Utf8StrFmt("%s %s",
                                                                     vendor, product);
                                        }
                                        else
                                        {
                                            description = product;
                                        }
                                        ComObjPtr<Medium> hostDVDDriveObj;
                                        hostDVDDriveObj.createObject();
                                        hostDVDDriveObj->init(m->pParent, DeviceType_DVD,
                                                              Bstr(devNode), Bstr(description));
                                        list.push_back(hostDVDDriveObj);
                                    }
                                    else
                                    {
                                        if (product == 0)
                                        {
                                            LogRel(("Host::COMGETTER(DVDDrives): failed to get property \"info.product\" for device %s.  dbus error: %s (%s)\n",
                                                    halDevices[i], dbusError.name, dbusError.message));
                                            gDBusErrorFree(&dbusError);
                                        }
                                        ComObjPtr<Medium> hostDVDDriveObj;
                                        hostDVDDriveObj.createObject();
                                        hostDVDDriveObj->init(m->pParent, DeviceType_DVD,
                                                              Bstr(devNode));
                                        list.push_back(hostDVDDriveObj);
                                    }
                                    if (vendor != 0)
                                    {
                                        gLibHalFreeString(vendor);
                                    }
                                    if (product != 0)
                                    {
                                        gLibHalFreeString(product);
                                    }
//                                }
//                                else
//                                {
//                                    LogRel(("Host::COMGETTER(DVDDrives): failed to validate the block device %s as a DVD drive\n"));
//                                }
#ifndef RT_OS_SOLARIS
                                gLibHalFreeString(devNode);
#else
                                free(devNode);
#endif
                            }
                            else
                            {
                                LogRel(("Host::COMGETTER(DVDDrives): failed to get property \"block.device\" for device %s.  dbus error: %s (%s)\n",
                                        halDevices[i], dbusError.name, dbusError.message));
                                gDBusErrorFree(&dbusError);
                            }
                        }
                        gLibHalFreeStringArray(halDevices);
                    }
                    else
                    {
                        LogRel(("Host::COMGETTER(DVDDrives): failed to get devices with capability \"storage.cdrom\".  dbus error: %s (%s)\n", dbusError.name, dbusError.message));
                        gDBusErrorFree(&dbusError);
                    }
                    if (!gLibHalCtxShutdown(halContext, &dbusError))  /* what now? */
                    {
                        LogRel(("Host::COMGETTER(DVDDrives): failed to shutdown the libhal context.  dbus error: %s (%s)\n",
                                dbusError.name, dbusError.message));
                        gDBusErrorFree(&dbusError);
                    }
                }
                else
                {
                    LogRel(("Host::COMGETTER(DVDDrives): failed to initialise libhal context.  dbus error: %s (%s)\n",
                            dbusError.name, dbusError.message));
                    gDBusErrorFree(&dbusError);
                }
                gLibHalCtxFree(halContext);
            }
            else
            {
                LogRel(("Host::COMGETTER(DVDDrives): failed to set libhal connection to dbus.\n"));
            }
        }
        else
        {
            LogRel(("Host::COMGETTER(DVDDrives): failed to get a libhal context - out of memory?\n"));
        }
        gDBusConnectionUnref(dbusConnection);
    }
    else
    {
        LogRel(("Host::COMGETTER(DVDDrives): failed to connect to dbus.  dbus error: %s (%s)\n",
                dbusError.name, dbusError.message));
        gDBusErrorFree(&dbusError);
    }
    return halSuccess;
}


/**
 * Helper function to query the hal subsystem for information about floppy drives attached to the
 * system.
 *
 * @returns true if information was successfully obtained, false otherwise
 * @retval  list drives found will be attached to this list
 */
bool Host::i_getFloppyInfoFromHal(std::list< ComObjPtr<Medium> > &list)
{
    bool halSuccess = false;
    DBusError dbusError;
    if (!gLibHalCheckPresence())
        return false;
    gDBusErrorInit(&dbusError);
    DBusConnection *dbusConnection = gDBusBusGet(DBUS_BUS_SYSTEM, &dbusError);
    if (dbusConnection != 0)
    {
        LibHalContext *halContext = gLibHalCtxNew();
        if (halContext != 0)
        {
            if (gLibHalCtxSetDBusConnection(halContext, dbusConnection))
            {
                if (gLibHalCtxInit(halContext, &dbusError))
                {
                    int numDevices;
                    char **halDevices = gLibHalFindDeviceStringMatch(halContext,
                                                "storage.drive_type", "floppy",
                                                &numDevices, &dbusError);
                    if (halDevices != 0)
                    {
                        /* Hal is installed and working, so if no devices are reported, assume
                           that there are none. */
                        halSuccess = true;
                        for (int i = 0; i < numDevices; i++)
                        {
                            char *driveType = gLibHalDeviceGetPropertyString(halContext,
                                                    halDevices[i], "storage.drive_type", 0);
                            if (driveType != 0)
                            {
                                if (strcmp(driveType, "floppy") != 0)
                                {
                                    gLibHalFreeString(driveType);
                                    continue;
                                }
                                gLibHalFreeString(driveType);
                            }
                            else
                            {
                                /* An error occurred.  The attribute "storage.drive_type"
                                   probably didn't exist. */
                                continue;
                            }
                            char *devNode = gLibHalDeviceGetPropertyString(halContext,
                                                    halDevices[i], "block.device", &dbusError);
                            if (devNode != 0)
                            {
//                                if (validateDevice(devNode, false))
//                                {
                                    Utf8Str description;
                                    char *vendor, *product;
                                    /* We do not check the error here, as this field may
                                       not even exist. */
                                    vendor = gLibHalDeviceGetPropertyString(halContext,
                                                    halDevices[i], "info.vendor", 0);
                                    product = gLibHalDeviceGetPropertyString(halContext,
                                                    halDevices[i], "info.product", &dbusError);
                                    if ((product != 0) && (product[0] != 0))
                                    {
                                        if ((vendor != 0) && (vendor[0] != 0))
                                        {
                                            description = Utf8StrFmt("%s %s",
                                                                     vendor, product);
                                        }
                                        else
                                        {
                                            description = product;
                                        }
                                        ComObjPtr<Medium> hostFloppyDrive;
                                        hostFloppyDrive.createObject();
                                        hostFloppyDrive->init(m->pParent, DeviceType_DVD,
                                                              Bstr(devNode), Bstr(description));
                                        list.push_back(hostFloppyDrive);
                                    }
                                    else
                                    {
                                        if (product == 0)
                                        {
                                            LogRel(("Host::COMGETTER(FloppyDrives): failed to get property \"info.product\" for device %s.  dbus error: %s (%s)\n",
                                                    halDevices[i], dbusError.name, dbusError.message));
                                            gDBusErrorFree(&dbusError);
                                        }
                                        ComObjPtr<Medium> hostFloppyDrive;
                                        hostFloppyDrive.createObject();
                                        hostFloppyDrive->init(m->pParent, DeviceType_DVD,
                                                              Bstr(devNode));
                                        list.push_back(hostFloppyDrive);
                                    }
                                    if (vendor != 0)
                                    {
                                        gLibHalFreeString(vendor);
                                    }
                                    if (product != 0)
                                    {
                                        gLibHalFreeString(product);
                                    }
//                                }
//                                else
//                                {
//                                    LogRel(("Host::COMGETTER(FloppyDrives): failed to validate the block device %s as a floppy drive\n"));
//                                }
                                gLibHalFreeString(devNode);
                            }
                            else
                            {
                                LogRel(("Host::COMGETTER(FloppyDrives): failed to get property \"block.device\" for device %s.  dbus error: %s (%s)\n",
                                        halDevices[i], dbusError.name, dbusError.message));
                                gDBusErrorFree(&dbusError);
                            }
                        }
                        gLibHalFreeStringArray(halDevices);
                    }
                    else
                    {
                        LogRel(("Host::COMGETTER(FloppyDrives): failed to get devices with capability \"storage.cdrom\".  dbus error: %s (%s)\n", dbusError.name, dbusError.message));
                        gDBusErrorFree(&dbusError);
                    }
                    if (!gLibHalCtxShutdown(halContext, &dbusError))  /* what now? */
                    {
                        LogRel(("Host::COMGETTER(FloppyDrives): failed to shutdown the libhal context.  dbus error: %s (%s)\n",
                                dbusError.name, dbusError.message));
                        gDBusErrorFree(&dbusError);
                    }
                }
                else
                {
                    LogRel(("Host::COMGETTER(FloppyDrives): failed to initialise libhal context.  dbus error: %s (%s)\n",
                            dbusError.name, dbusError.message));
                    gDBusErrorFree(&dbusError);
                }
                gLibHalCtxFree(halContext);
            }
            else
            {
                LogRel(("Host::COMGETTER(FloppyDrives): failed to set libhal connection to dbus.\n"));
            }
        }
        else
        {
            LogRel(("Host::COMGETTER(FloppyDrives): failed to get a libhal context - out of memory?\n"));
        }
        gDBusConnectionUnref(dbusConnection);
    }
    else
    {
        LogRel(("Host::COMGETTER(FloppyDrives): failed to connect to dbus.  dbus error: %s (%s)\n",
                dbusError.name, dbusError.message));
        gDBusErrorFree(&dbusError);
    }
    return halSuccess;
}
#endif  /* RT_OS_SOLARIS and VBOX_USE_HAL */

/** @todo get rid of dead code below - RT_OS_SOLARIS and RT_OS_LINUX are never both set */
#if defined(RT_OS_SOLARIS)

/**
 * Helper function to parse the given mount file and add found entries
 */
void Host::i_parseMountTable(char *mountTable, std::list< ComObjPtr<Medium> > &list)
{
#ifdef RT_OS_LINUX
    FILE *mtab = setmntent(mountTable, "r");
    if (mtab)
    {
        struct mntent *mntent;
        char *mnt_type;
        char *mnt_dev;
        char *tmp;
        while ((mntent = getmntent(mtab)))
        {
            mnt_type = (char*)malloc(strlen(mntent->mnt_type) + 1);
            mnt_dev = (char*)malloc(strlen(mntent->mnt_fsname) + 1);
            strcpy(mnt_type, mntent->mnt_type);
            strcpy(mnt_dev, mntent->mnt_fsname);
            // supermount fs case
            if (strcmp(mnt_type, "supermount") == 0)
            {
                tmp = strstr(mntent->mnt_opts, "fs=");
                if (tmp)
                {
                    free(mnt_type);
                    mnt_type = strdup(tmp + strlen("fs="));
                    if (mnt_type)
                    {
                        tmp = strchr(mnt_type, ',');
                        if (tmp)
                            *tmp = '\0';
                    }
                }
                tmp = strstr(mntent->mnt_opts, "dev=");
                if (tmp)
                {
                    free(mnt_dev);
                    mnt_dev = strdup(tmp + strlen("dev="));
                    if (mnt_dev)
                    {
                        tmp = strchr(mnt_dev, ',');
                        if (tmp)
                            *tmp = '\0';
                    }
                }
            }
            // use strstr here to cover things fs types like "udf,iso9660"
            if (strstr(mnt_type, "iso9660") == 0)
            {
                /** @todo check whether we've already got the drive in our list! */
                if (i_validateDevice(mnt_dev, true))
                {
                    ComObjPtr<Medium> hostDVDDriveObj;
                    hostDVDDriveObj.createObject();
                    hostDVDDriveObj->init(m->pParent, DeviceType_DVD, Bstr(mnt_dev));
                    list.push_back (hostDVDDriveObj);
                }
            }
            free(mnt_dev);
            free(mnt_type);
        }
        endmntent(mtab);
    }
#else  // RT_OS_SOLARIS
    FILE *mntFile = fopen(mountTable, "r");
    if (mntFile)
    {
        struct mnttab mntTab;
        while (getmntent(mntFile, &mntTab) == 0)
        {
            const char *mountName = mntTab.mnt_special;
            const char *mountPoint = mntTab.mnt_mountp;
            const char *mountFSType = mntTab.mnt_fstype;
            if (mountName && mountPoint && mountFSType)
            {
                // skip devices we are not interested in
                if ((*mountName && mountName[0] == '/') &&                      // skip 'fake' devices (like -hosts,
                                                                                // proc, fd, swap)
                    (*mountFSType && (strncmp(mountFSType, RT_STR_TUPLE("devfs")) != 0 &&  // skip devfs
                                                                                           // (i.e. /devices)
                                      strncmp(mountFSType, RT_STR_TUPLE("dev")) != 0 &&    // skip dev (i.e. /dev)
                                      strncmp(mountFSType, RT_STR_TUPLE("lofs")) != 0)))   // skip loop-back file-system (lofs)
                {
                    char *rawDevName = getfullrawname((char *)mountName);
                    if (i_validateDevice(rawDevName, true))
                    {
                        ComObjPtr<Medium> hostDVDDriveObj;
                        hostDVDDriveObj.createObject();
                        hostDVDDriveObj->init(m->pParent, DeviceType_DVD, Bstr(rawDevName));
                        list.push_back(hostDVDDriveObj);
                    }
                    free(rawDevName);
                }
            }
        }

        fclose(mntFile);
    }
#endif
}

/**
 * Helper function to check whether the given device node is a valid drive
 */
bool Host::i_validateDevice(const char *deviceNode, bool isCDROM)
{
    struct stat statInfo;
    bool retValue = false;

    // sanity check
    if (!deviceNode)
    {
        return false;
    }

    // first a simple stat() call
    if (stat(deviceNode, &statInfo) < 0)
    {
        return false;
    }
    else
    {
        if (isCDROM)
        {
            if (S_ISCHR(statInfo.st_mode) || S_ISBLK(statInfo.st_mode))
            {
                int fileHandle;
                // now try to open the device
                fileHandle = open(deviceNode, O_RDONLY | O_NONBLOCK, 0);
                if (fileHandle >= 0)
                {
                    cdrom_subchnl cdChannelInfo;
                    cdChannelInfo.cdsc_format = CDROM_MSF;
                    // this call will finally reveal the whole truth
#ifdef RT_OS_LINUX
                    if ((ioctl(fileHandle, CDROMSUBCHNL, &cdChannelInfo) == 0) ||
                        (errno == EIO) || (errno == ENOENT) ||
                        (errno == EINVAL) || (errno == ENOMEDIUM))
#else
                    if ((ioctl(fileHandle, CDROMSUBCHNL, &cdChannelInfo) == 0) ||
                        (errno == EIO) || (errno == ENOENT) ||
                        (errno == EINVAL))
#endif
                    {
                        retValue = true;
                    }
                    close(fileHandle);
                }
            }
        } else
        {
            // floppy case
            if (S_ISCHR(statInfo.st_mode) || S_ISBLK(statInfo.st_mode))
            {
                /// @todo do some more testing, maybe a nice IOCTL!
                retValue = true;
            }
        }
    }
    return retValue;
}
#endif // RT_OS_SOLARIS

#ifdef VBOX_WITH_USB
/**
 *  Checks for the presence and status of the USB Proxy Service.
 *  Returns S_OK when the Proxy is present and OK, VBOX_E_HOST_ERROR (as a
 *  warning) if the proxy service is not available due to the way the host is
 *  configured (at present, that means that usbfs and hal/DBus are not
 *  available on a Linux host) or E_FAIL and a corresponding error message
 *  otherwise. Intended to be used by methods that rely on the Proxy Service
 *  availability.
 *
 *  @note This method may return a warning result code. It is recommended to use
 *        MultiError to store the return value.
 *
 *  @note Locks this object for reading.
 */
HRESULT Host::i_checkUSBProxyService()
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc()))
        return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    AssertReturn(m->pUSBProxyService, E_FAIL);
    if (!m->pUSBProxyService->isActive())
    {
        /* disable the USB controller completely to avoid assertions if the
         * USB proxy service could not start. */

        switch (m->pUSBProxyService->getLastError())
        {
            case VERR_FILE_NOT_FOUND:  /** @todo what does this mean? */
                return setWarning(E_FAIL,
                                  tr("Could not load the Host USB Proxy Service (VERR_FILE_NOT_FOUND). The service might not be installed on the host computer"));
            case VERR_VUSB_USB_DEVICE_PERMISSION:
                return setWarning(E_FAIL,
                                  tr("VirtualBox is not currently allowed to access USB devices.  You can change this by adding your user to the 'vboxusers' group.  Please see the user manual for a more detailed explanation"));
            case VERR_VUSB_USBFS_PERMISSION:
                return setWarning(E_FAIL,
                                  tr("VirtualBox is not currently allowed to access USB devices.  You can change this by allowing your user to access the 'usbfs' folder and files.  Please see the user manual for a more detailed explanation"));
            case VINF_SUCCESS:
                return setWarning(E_FAIL,
                                  tr("The USB Proxy Service has not yet been ported to this host"));
            default:
                return setWarning(E_FAIL, "%s: %Rrc",
                                  tr("Could not load the Host USB Proxy service"),
                                  m->pUSBProxyService->getLastError());
        }
    }

    return S_OK;
}
#endif /* VBOX_WITH_USB */

HRESULT Host::i_updateNetIfList()
{
#ifdef VBOX_WITH_HOSTNETIF_API
    AssertReturn(!isWriteLockOnCurrentThread(), E_FAIL);

    /** @todo r=klaus it would save lots of clock cycles if for concurrent
     * threads executing this code we'd only do one interface enumeration
     * and update, and let the other threads use the result as is. However
     * if there's a constant hammering of this method, we don't want this
     * to cause update starvation. */
    HostNetworkInterfaceList list;
    int rc = NetIfList(list);
    if (rc)
    {
        Log(("Failed to get host network interface list with rc=%Rrc\n", rc));
        return E_FAIL;
    }

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    AssertReturn(m->pParent, E_FAIL);
    /* Make a copy as the original may be partially destroyed later. */
    HostNetworkInterfaceList listCopy(list);
    HostNetworkInterfaceList::iterator itOld, itNew;
# ifdef VBOX_WITH_RESOURCE_USAGE_API
    PerformanceCollector *aCollector = m->pParent->i_performanceCollector();
# endif
    for (itOld = m->llNetIfs.begin(); itOld != m->llNetIfs.end(); ++itOld)
    {
        bool fGone = true;
        Bstr nameOld;
        (*itOld)->COMGETTER(Name)(nameOld.asOutParam());
        for (itNew = listCopy.begin(); itNew != listCopy.end(); ++itNew)
        {
            Bstr nameNew;
            (*itNew)->COMGETTER(Name)(nameNew.asOutParam());
            if (nameNew == nameOld)
            {
                fGone = false;
                (*itNew)->uninit();
                listCopy.erase(itNew);
                break;
            }
        }
        if (fGone)
        {
# ifdef VBOX_WITH_RESOURCE_USAGE_API
            (*itOld)->i_unregisterMetrics(aCollector, this);
            (*itOld)->uninit();
# endif
        }
    }
    /*
     * Need to set the references to VirtualBox object in all interface objects
     * (see @bugref{6439}).
     */
    for (itNew = list.begin(); itNew != list.end(); ++itNew)
        (*itNew)->i_setVirtualBox(m->pParent);
    /* At this point listCopy will contain newly discovered interfaces only. */
    for (itNew = listCopy.begin(); itNew != listCopy.end(); ++itNew)
    {
        HostNetworkInterfaceType_T t;
        HRESULT hrc = (*itNew)->COMGETTER(InterfaceType)(&t);
        if (FAILED(hrc))
        {
            Bstr n;
            (*itNew)->COMGETTER(Name)(n.asOutParam());
            LogRel(("Host::updateNetIfList: failed to get interface type for %ls\n", n.raw()));
        }
        else if (t == HostNetworkInterfaceType_Bridged)
        {
# ifdef VBOX_WITH_RESOURCE_USAGE_API
            (*itNew)->i_registerMetrics(aCollector, this);
# endif
        }
    }
    m->llNetIfs = list;
    return S_OK;
#else
    return E_NOTIMPL;
#endif
}

#ifdef VBOX_WITH_RESOURCE_USAGE_API

void Host::i_registerDiskMetrics(PerformanceCollector *aCollector)
{
    pm::CollectorHAL *hal = aCollector->getHAL();
    /* Create sub metrics */
    Utf8StrFmt fsNameBase("FS/{%s}/Usage", "/");
    //Utf8StrFmt fsNameBase("Filesystem/[root]/Usage");
    pm::SubMetric *fsRootUsageTotal  = new pm::SubMetric(fsNameBase + "/Total",
        "Root file system size.");
    pm::SubMetric *fsRootUsageUsed   = new pm::SubMetric(fsNameBase + "/Used",
        "Root file system space currently occupied.");
    pm::SubMetric *fsRootUsageFree   = new pm::SubMetric(fsNameBase + "/Free",
        "Root file system space currently empty.");

    pm::BaseMetric *fsRootUsage = new pm::HostFilesystemUsage(hal, this,
                                                              fsNameBase, "/",
                                                              fsRootUsageTotal,
                                                              fsRootUsageUsed,
                                                              fsRootUsageFree);
    aCollector->registerBaseMetric(fsRootUsage);

    aCollector->registerMetric(new pm::Metric(fsRootUsage, fsRootUsageTotal, 0));
    aCollector->registerMetric(new pm::Metric(fsRootUsage, fsRootUsageTotal,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(fsRootUsage, fsRootUsageTotal,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(fsRootUsage, fsRootUsageTotal,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(fsRootUsage, fsRootUsageUsed, 0));
    aCollector->registerMetric(new pm::Metric(fsRootUsage, fsRootUsageUsed,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(fsRootUsage, fsRootUsageUsed,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(fsRootUsage, fsRootUsageUsed,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(fsRootUsage, fsRootUsageFree, 0));
    aCollector->registerMetric(new pm::Metric(fsRootUsage, fsRootUsageFree,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(fsRootUsage, fsRootUsageFree,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(fsRootUsage, fsRootUsageFree,
                                              new pm::AggregateMax()));

    /* For now we are concerned with the root file system only. */
    pm::DiskList disksUsage, disksLoad;
    int rc = hal->getDiskListByFs("/", disksUsage, disksLoad);
    if (RT_FAILURE(rc))
        return;
    pm::DiskList::iterator it;
    for (it = disksLoad.begin(); it != disksLoad.end(); ++it)
    {
        Utf8StrFmt strName("Disk/%s", it->c_str());
        pm::SubMetric *fsLoadUtil   = new pm::SubMetric(strName + "/Load/Util",
            "Percentage of time disk was busy serving I/O requests.");
        pm::BaseMetric *fsLoad  = new pm::HostDiskLoadRaw(hal, this, strName + "/Load",
                                                         *it, fsLoadUtil);
        aCollector->registerBaseMetric(fsLoad);

        aCollector->registerMetric(new pm::Metric(fsLoad, fsLoadUtil, 0));
        aCollector->registerMetric(new pm::Metric(fsLoad, fsLoadUtil,
                                                  new pm::AggregateAvg()));
        aCollector->registerMetric(new pm::Metric(fsLoad, fsLoadUtil,
                                                  new pm::AggregateMin()));
        aCollector->registerMetric(new pm::Metric(fsLoad, fsLoadUtil,
                                                  new pm::AggregateMax()));
    }
    for (it = disksUsage.begin(); it != disksUsage.end(); ++it)
    {
        Utf8StrFmt strName("Disk/%s", it->c_str());
        pm::SubMetric *fsUsageTotal = new pm::SubMetric(strName + "/Usage/Total",
            "Disk size.");
        pm::BaseMetric *fsUsage = new pm::HostDiskUsage(hal, this, strName + "/Usage",
                                                        *it, fsUsageTotal);
        aCollector->registerBaseMetric(fsUsage);

        aCollector->registerMetric(new pm::Metric(fsUsage, fsUsageTotal, 0));
        aCollector->registerMetric(new pm::Metric(fsUsage, fsUsageTotal,
                                                  new pm::AggregateAvg()));
        aCollector->registerMetric(new pm::Metric(fsUsage, fsUsageTotal,
                                                  new pm::AggregateMin()));
        aCollector->registerMetric(new pm::Metric(fsUsage, fsUsageTotal,
                                                  new pm::AggregateMax()));
    }
}

void Host::i_registerMetrics(PerformanceCollector *aCollector)
{
    pm::CollectorHAL *hal = aCollector->getHAL();
    /* Create sub metrics */
    pm::SubMetric *cpuLoadUser   = new pm::SubMetric("CPU/Load/User",
        "Percentage of processor time spent in user mode.");
    pm::SubMetric *cpuLoadKernel = new pm::SubMetric("CPU/Load/Kernel",
        "Percentage of processor time spent in kernel mode.");
    pm::SubMetric *cpuLoadIdle   = new pm::SubMetric("CPU/Load/Idle",
        "Percentage of processor time spent idling.");
    pm::SubMetric *cpuMhzSM      = new pm::SubMetric("CPU/MHz",
        "Average of current frequency of all processors.");
    pm::SubMetric *ramUsageTotal = new pm::SubMetric("RAM/Usage/Total",
        "Total physical memory installed.");
    pm::SubMetric *ramUsageUsed  = new pm::SubMetric("RAM/Usage/Used",
        "Physical memory currently occupied.");
    pm::SubMetric *ramUsageFree  = new pm::SubMetric("RAM/Usage/Free",
        "Physical memory currently available to applications.");
    pm::SubMetric *ramVMMUsed = new pm::SubMetric("RAM/VMM/Used",
        "Total physical memory used by the hypervisor.");
    pm::SubMetric *ramVMMFree = new pm::SubMetric("RAM/VMM/Free",
        "Total physical memory free inside the hypervisor.");
    pm::SubMetric *ramVMMBallooned  = new pm::SubMetric("RAM/VMM/Ballooned",
        "Total physical memory ballooned by the hypervisor.");
    pm::SubMetric *ramVMMShared = new pm::SubMetric("RAM/VMM/Shared",
        "Total physical memory shared between VMs.");


    /* Create and register base metrics */
    pm::BaseMetric *cpuLoad = new pm::HostCpuLoadRaw(hal, this, cpuLoadUser, cpuLoadKernel,
                                          cpuLoadIdle);
    aCollector->registerBaseMetric(cpuLoad);
    pm::BaseMetric *cpuMhz = new pm::HostCpuMhz(hal, this, cpuMhzSM);
    aCollector->registerBaseMetric(cpuMhz);
    pm::BaseMetric *ramUsage = new pm::HostRamUsage(hal, this,
                                                    ramUsageTotal,
                                                    ramUsageUsed,
                                                    ramUsageFree);
    aCollector->registerBaseMetric(ramUsage);
    pm::BaseMetric *ramVmm = new pm::HostRamVmm(aCollector->getGuestManager(), this,
                                                ramVMMUsed,
                                                ramVMMFree,
                                                ramVMMBallooned,
                                                ramVMMShared);
    aCollector->registerBaseMetric(ramVmm);

    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadUser, 0));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadUser,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadUser,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadUser,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadKernel, 0));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadKernel,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadKernel,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadKernel,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadIdle, 0));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadIdle,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadIdle,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(cpuLoad, cpuLoadIdle,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(cpuMhz, cpuMhzSM, 0));
    aCollector->registerMetric(new pm::Metric(cpuMhz, cpuMhzSM,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(cpuMhz, cpuMhzSM,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(cpuMhz, cpuMhzSM,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageTotal, 0));
    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageTotal,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageTotal,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageTotal,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageUsed, 0));
    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageUsed,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageUsed,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageUsed,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageFree, 0));
    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageFree,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageFree,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(ramUsage, ramUsageFree,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMUsed, 0));
    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMUsed,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMUsed,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMUsed,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMFree, 0));
    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMFree,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMFree,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMFree,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMBallooned, 0));
    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMBallooned,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMBallooned,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMBallooned,
                                              new pm::AggregateMax()));

    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMShared, 0));
    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMShared,
                                              new pm::AggregateAvg()));
    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMShared,
                                              new pm::AggregateMin()));
    aCollector->registerMetric(new pm::Metric(ramVmm, ramVMMShared,
                                              new pm::AggregateMax()));
    i_registerDiskMetrics(aCollector);
}

void Host::i_unregisterMetrics(PerformanceCollector *aCollector)
{
    aCollector->unregisterMetricsFor(this);
    aCollector->unregisterBaseMetricsFor(this);
}

#endif /* VBOX_WITH_RESOURCE_USAGE_API */


/* static */
void Host::i_generateMACAddress(Utf8Str &mac)
{
    /*
     * Our strategy is as follows: the first three bytes are our fixed
     * vendor ID (080027). The remaining 3 bytes will be taken from the
     * start of a GUID. This is a fairly safe algorithm.
     */
    Guid guid;
    guid.create();
    mac = Utf8StrFmt("080027%02X%02X%02X",
                     guid.raw()->au8[0], guid.raw()->au8[1], guid.raw()->au8[2]);
}

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
