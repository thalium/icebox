/* $Id: HostVideoInputDeviceImpl.cpp $ */
/** @file
 *
 * Host video capture device implementation.
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

#include "HostVideoInputDeviceImpl.h"
#include "Logging.h"
#include "VirtualBoxImpl.h"
#ifdef VBOX_WITH_EXTPACK
# include "ExtPackManagerImpl.h"
#endif

#include <iprt/ldr.h>
#include <iprt/path.h>

#include <VBox/sup.h>

/*
 * HostVideoInputDevice implementation.
 */
DEFINE_EMPTY_CTOR_DTOR(HostVideoInputDevice)

HRESULT HostVideoInputDevice::FinalConstruct()
{
    return BaseFinalConstruct();
}

void HostVideoInputDevice::FinalRelease()
{
    uninit();

    BaseFinalRelease();
}

/*
 * Initializes the instance.
 */
HRESULT HostVideoInputDevice::init(const com::Utf8Str &name, const com::Utf8Str &path, const com::Utf8Str &alias)
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m.name = name;
    m.path = path;
    m.alias = alias;

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}

/*
 * Uninitializes the instance.
 * Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void HostVideoInputDevice::uninit()
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    m.name.setNull();
    m.path.setNull();
    m.alias.setNull();
}

static HRESULT hostVideoInputDeviceAdd(HostVideoInputDeviceList *pList,
                                       const com::Utf8Str &name,
                                       const com::Utf8Str &path,
                                       const com::Utf8Str &alias)
{
    ComObjPtr<HostVideoInputDevice> obj;
    HRESULT hr = obj.createObject();
    if (SUCCEEDED(hr))
    {
        hr = obj->init(name, path, alias);
        if (SUCCEEDED(hr))
            pList->push_back(obj);
    }
    return hr;
}

static DECLCALLBACK(int) hostWebcamAdd(void *pvUser,
                                       const char *pszName,
                                       const char *pszPath,
                                       const char *pszAlias,
                                       uint64_t *pu64Result)
{
    HostVideoInputDeviceList *pList = (HostVideoInputDeviceList *)pvUser;
    HRESULT hr = hostVideoInputDeviceAdd(pList, pszName, pszPath, pszAlias);
    if (FAILED(hr))
    {
        *pu64Result = (uint64_t)hr;
        return VERR_NOT_SUPPORTED;
    }
    return VINF_SUCCESS;
}

/** @todo These typedefs must be in a header. */
typedef DECLCALLBACK(int) FNVBOXHOSTWEBCAMADD(void *pvUser,
                                              const char *pszName,
                                              const char *pszPath,
                                              const char *pszAlias,
                                              uint64_t *pu64Result);
typedef FNVBOXHOSTWEBCAMADD *PFNVBOXHOSTWEBCAMADD;

typedef DECLCALLBACK(int) FNVBOXHOSTWEBCAMLIST(PFNVBOXHOSTWEBCAMADD pfnWebcamAdd,
                                               void *pvUser,
                                               uint64_t *pu64WebcamAddResult);
typedef FNVBOXHOSTWEBCAMLIST *PFNVBOXHOSTWEBCAMLIST;

static int loadHostWebcamLibrary(const char *pszPath, RTLDRMOD *phmod, PFNVBOXHOSTWEBCAMLIST *ppfn)
{
    int rc;
    if (RTPathHavePath(pszPath))
    {
        RTLDRMOD hmod = NIL_RTLDRMOD;
        RTERRINFOSTATIC ErrInfo;
        rc = SUPR3HardenedLdrLoadPlugIn(pszPath, &hmod, RTErrInfoInitStatic(&ErrInfo));
        if (RT_SUCCESS(rc))
        {
            static const char s_szSymbol[] = "VBoxHostWebcamList";
            rc = RTLdrGetSymbol(hmod, s_szSymbol, (void **)ppfn);
            if (RT_SUCCESS(rc))
                *phmod = hmod;
            else
            {
                if (rc != VERR_SYMBOL_NOT_FOUND)
                    LogRel(("Resolving symbol '%s': %Rrc\n", s_szSymbol, rc));
                RTLdrClose(hmod);
                hmod = NIL_RTLDRMOD;
            }
        }
        else
        {
            LogRel(("Loading the library '%s': %Rrc\n", pszPath, rc));
            if (RTErrInfoIsSet(&ErrInfo.Core))
                LogRel(("  %s\n", ErrInfo.Core.pszMsg));
        }
    }
    else
    {
        LogRel(("Loading the library '%s': No path! Refusing to try loading it!\n", pszPath));
        rc = VERR_INVALID_PARAMETER;
    }
    return rc;
}


static HRESULT fillDeviceList(VirtualBox *pVirtualBox, HostVideoInputDeviceList *pList)
{
    HRESULT hr;
    Utf8Str strLibrary;

#ifdef VBOX_WITH_EXTPACK
    ExtPackManager *pExtPackMgr = pVirtualBox->i_getExtPackManager();
    hr = pExtPackMgr->i_getLibraryPathForExtPack("VBoxHostWebcam", ORACLE_PUEL_EXTPACK_NAME, &strLibrary);
#else
    hr = E_NOTIMPL;
#endif

    if (SUCCEEDED(hr))
    {
        PFNVBOXHOSTWEBCAMLIST pfn = NULL;
        RTLDRMOD hmod = NIL_RTLDRMOD;
        int rc = loadHostWebcamLibrary(strLibrary.c_str(), &hmod, &pfn);

        LogRel(("Load [%s] rc %Rrc\n", strLibrary.c_str(), rc));

        if (RT_SUCCESS(rc))
        {
            uint64_t u64Result = S_OK;
            rc = pfn(hostWebcamAdd, pList, &u64Result);
            Log(("VBoxHostWebcamList rc %Rrc, result 0x%08X\n", rc, u64Result));
            if (RT_FAILURE(rc))
            {
                hr = (HRESULT)u64Result;
            }

            RTLdrClose(hmod);
            hmod = NIL_RTLDRMOD;
        }

        if (SUCCEEDED(hr))
        {
            if (RT_FAILURE(rc))
                hr = pVirtualBox->setError(VBOX_E_IPRT_ERROR,
                         "Failed to get webcam list: %Rrc", rc);
        }
    }

    return hr;
}

/* static */ HRESULT HostVideoInputDevice::queryHostDevices(VirtualBox *pVirtualBox, HostVideoInputDeviceList *pList)
{
    HRESULT hr = fillDeviceList(pVirtualBox, pList);

    if (FAILED(hr))
    {
        pList->clear();
    }

    return hr;
}

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
