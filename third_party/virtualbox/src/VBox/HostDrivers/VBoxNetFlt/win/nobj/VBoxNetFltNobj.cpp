/* $Id: VBoxNetFltNobj.cpp $ */
/** @file
 * VBoxNetFltNobj.cpp - Notify Object for Bridged Networking Driver.
 * Used to filter Bridged Networking Driver bindings
 */
/*
 * Copyright (C) 2011-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#include "VBoxNetFltNobj.h"
#include <iprt/win/ntddndis.h>
#include <assert.h>
#include <stdio.h>

#include <VBoxNetFltNobjT_i.c>

//# define VBOXNETFLTNOTIFY_DEBUG_BIND

#ifdef DEBUG
# define NonStandardAssert(a) assert(a)
# define NonStandardAssertBreakpoint() assert(0)
#else
# define NonStandardAssert(a) do{}while (0)
# define NonStandardAssertBreakpoint() do{}while (0)
#endif

VBoxNetFltNobj::VBoxNetFltNobj() :
    mpNetCfg(NULL),
    mpNetCfgComponent(NULL),
    mbInstalling(FALSE)
{
}

VBoxNetFltNobj::~VBoxNetFltNobj()
{
    cleanup();
}

void VBoxNetFltNobj::cleanup()
{
    if (mpNetCfg)
    {
        mpNetCfg->Release();
        mpNetCfg = NULL;
    }

    if (mpNetCfgComponent)
    {
        mpNetCfgComponent->Release();
        mpNetCfgComponent = NULL;
    }
}

void VBoxNetFltNobj::init(IN INetCfgComponent *pNetCfgComponent, IN INetCfg *pNetCfg, IN BOOL bInstalling)
{
    cleanup();

    NonStandardAssert(pNetCfg);
    NonStandardAssert(pNetCfgComponent);
    if (pNetCfg)
    {
        pNetCfg->AddRef();
        mpNetCfg = pNetCfg;
    }

    if (pNetCfgComponent)
    {
        pNetCfgComponent->AddRef();
        mpNetCfgComponent = pNetCfgComponent;
    }

    mbInstalling = bInstalling;
}

/* INetCfgComponentControl methods */
STDMETHODIMP VBoxNetFltNobj::Initialize(IN INetCfgComponent *pNetCfgComponent, IN INetCfg *pNetCfg, IN BOOL bInstalling)
{
    init(pNetCfgComponent, pNetCfg, bInstalling);
    return S_OK;
}

STDMETHODIMP VBoxNetFltNobj::ApplyRegistryChanges()
{
    return S_OK;
}

STDMETHODIMP VBoxNetFltNobj::ApplyPnpChanges(IN INetCfgPnpReconfigCallback *pCallback)
{
    RT_NOREF1(pCallback);
    return S_OK;
}

STDMETHODIMP VBoxNetFltNobj::CancelChanges()
{
    return S_OK;
}

static HRESULT vboxNetFltWinQueryInstanceKey(IN INetCfgComponent *pComponent, OUT PHKEY phKey)
{
    LPWSTR pPnpId;
    HRESULT hr = pComponent->GetPnpDevNodeId(&pPnpId);
    if (hr == S_OK)
    {
        WCHAR KeyName[MAX_PATH];
        wcscpy(KeyName, L"SYSTEM\\CurrentControlSet\\Enum\\");
        wcscat(KeyName,pPnpId);

        LONG winEr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, KeyName,
          0, /*__reserved DWORD ulOptions*/
          KEY_READ, /*__in REGSAM samDesired*/
          phKey);

        if (winEr != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(winEr);
            NonStandardAssertBreakpoint();
        }

        CoTaskMemFree(pPnpId);
    }
    else
    {
        NonStandardAssertBreakpoint();
    }

    return hr;
}

static HRESULT vboxNetFltWinQueryDriverKey(IN HKEY InstanceKey, OUT PHKEY phKey)
{
    DWORD Type = REG_SZ;
    WCHAR Value[MAX_PATH];
    DWORD cbValue = sizeof(Value);
    HRESULT hr = S_OK;
    LONG winEr = RegQueryValueExW(InstanceKey,
            L"Driver", /*__in_opt LPCTSTR lpValueName*/
            0, /*__reserved LPDWORD lpReserved*/
            &Type, /*__out_opt LPDWORD lpType*/
            (LPBYTE)Value, /*__out_opt LPBYTE lpData*/
            &cbValue/*__inout_opt LPDWORD lpcbData*/
            );

    if (winEr == ERROR_SUCCESS)
    {
        WCHAR KeyName[MAX_PATH];
        wcscpy(KeyName, L"SYSTEM\\CurrentControlSet\\Control\\Class\\");
        wcscat(KeyName,Value);

        winEr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, KeyName,
          0, /*__reserved DWORD ulOptions*/
          KEY_READ, /*__in REGSAM samDesired*/
          phKey);

        if (winEr != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(winEr);
            NonStandardAssertBreakpoint();
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(winEr);
        NonStandardAssertBreakpoint();
    }

    return hr;
}

static HRESULT vboxNetFltWinQueryDriverKey(IN INetCfgComponent *pComponent, OUT PHKEY phKey)
{
    HKEY InstanceKey;
    HRESULT hr = vboxNetFltWinQueryInstanceKey(pComponent, &InstanceKey);
    if (hr == S_OK)
    {
        hr = vboxNetFltWinQueryDriverKey(InstanceKey, phKey);
        if (hr != S_OK)
        {
            NonStandardAssertBreakpoint();
        }
        RegCloseKey(InstanceKey);
    }
    else
    {
        NonStandardAssertBreakpoint();
    }

    return hr;
}

static HRESULT vboxNetFltWinNotifyCheckNetAdp(IN INetCfgComponent *pComponent, OUT bool * pbShouldBind)
{
    HRESULT hr;
    LPWSTR pDevId;
    hr = pComponent->GetId(&pDevId);
    if (hr == S_OK)
    {
        if (!_wcsnicmp(pDevId, L"sun_VBoxNetAdp", sizeof(L"sun_VBoxNetAdp")/2))
        {
            *pbShouldBind = false;
        }
        else
        {
            hr = S_FALSE;
        }
        CoTaskMemFree(pDevId);
    }
    else
    {
        NonStandardAssertBreakpoint();
    }

    return hr;
}

static HRESULT vboxNetFltWinNotifyCheckMsLoop(IN INetCfgComponent *pComponent, OUT bool * pbShouldBind)
{
    HRESULT hr;
    LPWSTR pDevId;
    hr = pComponent->GetId(&pDevId);
    if (hr == S_OK)
    {
        if (!_wcsnicmp(pDevId, L"*msloop", sizeof(L"*msloop")/2))
        {
            /* we need to detect the medium the adapter is presenting
             * to do that we could examine in the registry the *msloop params */
            HKEY DriverKey;
            hr = vboxNetFltWinQueryDriverKey(pComponent, &DriverKey);
            if (hr == S_OK)
            {
                DWORD Type = REG_SZ;
                WCHAR Value[64]; /* 2 should be enough actually, paranoid check for extra spaces */
                DWORD cbValue = sizeof(Value);
                LONG winEr = RegQueryValueExW(DriverKey,
                            L"Medium", /*__in_opt LPCTSTR lpValueName*/
                            0, /*__reserved LPDWORD lpReserved*/
                            &Type, /*__out_opt LPDWORD lpType*/
                            (LPBYTE)Value, /*__out_opt LPBYTE lpData*/
                            &cbValue/*__inout_opt LPDWORD lpcbData*/
                            );
                if (winEr == ERROR_SUCCESS)
                {
                    PWCHAR endPrt;
                    ULONG enmMedium = wcstoul(Value,
                       &endPrt,
                       0 /* base*/);

                    winEr = errno;
                    if (winEr == ERROR_SUCCESS)
                    {
                        if (enmMedium == 0) /* 0 is Ethernet */
                        {
                            *pbShouldBind = true;
                        }
                        else
                        {
                            *pbShouldBind = false;
                        }
                    }
                    else
                    {
                        NonStandardAssertBreakpoint();
                        *pbShouldBind = true;
                    }
                }
                else
                {
                    /** @todo we should check the default medium in HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4D36E972-E325-11CE-BFC1-08002bE10318}\<driver_id>\Ndi\Params\Medium, REG_SZ "Default" value */
                    NonStandardAssertBreakpoint();
                    *pbShouldBind = true;
                }

                RegCloseKey(DriverKey);
            }
            else
            {
                NonStandardAssertBreakpoint();
            }
        }
        else
        {
            hr = S_FALSE;
        }
        CoTaskMemFree(pDevId);
    }
    else
    {
        NonStandardAssertBreakpoint();
    }

    return hr;
}

static HRESULT vboxNetFltWinNotifyCheckLowerRange(IN INetCfgComponent *pComponent, OUT bool * pbShouldBind)
{
    HKEY DriverKey;
    HKEY InterfacesKey;
    HRESULT hr = vboxNetFltWinQueryDriverKey(pComponent, &DriverKey);
    if (hr == S_OK)
    {
        LONG winEr = RegOpenKeyExW(DriverKey, L"Ndi\\Interfaces",
                        0, /*__reserved DWORD ulOptions*/
                        KEY_READ, /*__in REGSAM samDesired*/
                        &InterfacesKey);
        if (winEr == ERROR_SUCCESS)
        {
            DWORD Type = REG_SZ;
            WCHAR Value[MAX_PATH];
            DWORD cbValue = sizeof(Value);
            winEr = RegQueryValueExW(InterfacesKey,
                    L"LowerRange", /*__in_opt LPCTSTR lpValueName*/
                    0, /*__reserved LPDWORD lpReserved*/
                    &Type, /*__out_opt LPDWORD lpType*/
                    (LPBYTE)Value, /*__out_opt LPBYTE lpData*/
                    &cbValue/*__inout_opt LPDWORD lpcbData*/
                    );
            if (winEr == ERROR_SUCCESS)
            {
                if (wcsstr(Value,L"ethernet") || wcsstr(Value, L"wan"))
                {
                    *pbShouldBind = true;
                }
                else
                {
                    *pbShouldBind = false;
                }
            }
            else
            {
                /* do not set err status to it */
                *pbShouldBind = false;
                NonStandardAssertBreakpoint();
            }

            RegCloseKey(InterfacesKey);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(winEr);
            NonStandardAssertBreakpoint();
        }

        RegCloseKey(DriverKey);
    }
    else
    {
        NonStandardAssertBreakpoint();
    }

    return hr;
}

static HRESULT vboxNetFltWinNotifyShouldBind(IN INetCfgComponent *pComponent, OUT bool *pbShouldBind)
{
    DWORD fCharacteristics;
    HRESULT hr;

    do
    {
        /* filter out only physical adapters */
        hr = pComponent->GetCharacteristics(&fCharacteristics);
        if (hr != S_OK)
        {
            NonStandardAssertBreakpoint();
            break;
        }


        if (fCharacteristics & NCF_HIDDEN)
        {
            /* we are not binding to hidden adapters */
            *pbShouldBind = false;
            break;
        }

        hr = vboxNetFltWinNotifyCheckMsLoop(pComponent, pbShouldBind);
        if (hr == S_OK)
        {
            /* this is a loopback adapter,
             * the pbShouldBind already contains the result */
            break;
        }
        else if (hr != S_FALSE)
        {
            /* error occurred */
            break;
        }

        hr = vboxNetFltWinNotifyCheckNetAdp(pComponent, pbShouldBind);
        if (hr == S_OK)
        {
            /* this is a VBoxNetAdp adapter,
             * the pbShouldBind already contains the result */
            break;
        }
        else if (hr != S_FALSE)
        {
            /* error occurred */
            break;
        }

        /* hr == S_FALSE means this is not a loopback adpater, set it to S_OK */
        hr = S_OK;

//        if (!(fCharacteristics & NCF_PHYSICAL))
//        {
//            /* we are binding to physical adapters only */
//            *pbShouldBind = false;
//            break;
//        }

        hr = vboxNetFltWinNotifyCheckLowerRange(pComponent, pbShouldBind);
        if (hr == S_OK)
        {
            /* the vboxNetFltWinNotifyCheckLowerRange ccucceeded,
             * the pbShouldBind already contains the result */
            break;
        }
        /* we are here because of the fail, nothing else to do */
    } while (0);

    return hr;
}


static HRESULT vboxNetFltWinNotifyShouldBind(IN INetCfgBindingInterface *pIf, OUT bool *pbShouldBind)
{
    INetCfgComponent * pAdapterComponent;
    HRESULT hr = pIf->GetLowerComponent(&pAdapterComponent);
    if (hr == S_OK)
    {
        hr = vboxNetFltWinNotifyShouldBind(pAdapterComponent, pbShouldBind);

        pAdapterComponent->Release();
    }
    else
    {
        NonStandardAssertBreakpoint();
    }

    return hr;
}

static HRESULT vboxNetFltWinNotifyShouldBind(IN INetCfgBindingPath *pPath, OUT bool *pbDoBind)
{
    IEnumNetCfgBindingInterface *pEnumBindingIf;
    HRESULT hr = pPath->EnumBindingInterfaces(&pEnumBindingIf);
    if (hr == S_OK)
    {
        hr = pEnumBindingIf->Reset();
        if (hr == S_OK)
        {
            ULONG ulCount;
            INetCfgBindingInterface *pBindingIf;
            do
            {
                hr = pEnumBindingIf->Next(1, &pBindingIf, &ulCount);
                if (hr == S_OK)
                {
                    hr = vboxNetFltWinNotifyShouldBind(pBindingIf, pbDoBind);

                    pBindingIf->Release();

                    if (hr == S_OK)
                    {
                        if (!(*pbDoBind))
                        {
                            break;
                        }
                    }
                    else
                    {
                        /* break on failure */
                        break;
                    }
                }
                else if (hr == S_FALSE)
                {
                    /* no more elements */
                    hr = S_OK;
                    break;
                }
                else
                {
                    NonStandardAssertBreakpoint();
                    /* break on falure */
                    break;
                }
            } while (true);
        }
        else
        {
            NonStandardAssertBreakpoint();
        }

        pEnumBindingIf->Release();
    }
    else
    {
        NonStandardAssertBreakpoint();
    }

    return hr;
}

static bool vboxNetFltWinNotifyShouldBind(IN INetCfgBindingPath *pPath)
{
#ifdef VBOXNETFLTNOTIFY_DEBUG_BIND
    return VBOXNETFLTNOTIFY_DEBUG_BIND;
#else
    bool bShouldBind;
    HRESULT hr = vboxNetFltWinNotifyShouldBind(pPath, &bShouldBind) ;
    if (hr != S_OK)
    {
        bShouldBind = VBOXNETFLTNOTIFY_ONFAIL_BINDDEFAULT;
    }

    return bShouldBind;
#endif
}


/* INetCfgComponentNotifyBinding methods */
STDMETHODIMP VBoxNetFltNobj::NotifyBindingPath(IN DWORD dwChangeFlag, IN INetCfgBindingPath *pNetCfgBP)
{
    if (!(dwChangeFlag & NCN_ENABLE) || (dwChangeFlag & NCN_REMOVE) || vboxNetFltWinNotifyShouldBind(pNetCfgBP))
        return S_OK;
    return NETCFG_S_DISABLE_QUERY;
}

STDMETHODIMP VBoxNetFltNobj::QueryBindingPath(IN DWORD dwChangeFlag, IN INetCfgBindingPath *pNetCfgBP)
{
    RT_NOREF1(dwChangeFlag);
    if (vboxNetFltWinNotifyShouldBind(pNetCfgBP))
        return S_OK;
    return NETCFG_S_DISABLE_QUERY;
}


static ATL::CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_VBoxNetFltNobj, VBoxNetFltNobj)
END_OBJECT_MAP()

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
    }
    return TRUE;
}

STDAPI DllCanUnloadNow()
{
    return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
// this is a "just in case" conditional, which is not defined
#ifdef VBOX_FORCE_REGISTER_SERVER
    return _Module.RegisterServer(TRUE);
#else
    return S_OK;
#endif
}

STDAPI DllUnregisterServer()
{
// this is a "just in case" conditional, which is not defined
#ifdef VBOX_FORCE_REGISTER_SERVER
    return _Module.UnregisterServer(TRUE);
#else
    return S_OK;
#endif
}
