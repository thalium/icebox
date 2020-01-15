/* $Id: VBoxMMNotificationClient.cpp $ */
/** @file
 * VBoxMMNotificationClient.cpp - Implementation of the IMMNotificationClient interface
 *                                to detect audio endpoint changes.
 */

/*
 * Copyright (C) 2017-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "VBoxMMNotificationClient.h"

#include <iprt/win/windows.h>

#pragma warning(push)
#pragma warning(disable: 4201)
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#pragma warning(pop)

#ifdef LOG_GROUP
# undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_DRV_HOST_AUDIO
#include <VBox/log.h>

VBoxMMNotificationClient::VBoxMMNotificationClient(void)
    : m_fRegisteredClient(false)
    , m_cRef(1)
{
}

VBoxMMNotificationClient::~VBoxMMNotificationClient(void)
{
}

/**
 * Uninitializes the mulitmedia notification client implementation.
 */
void VBoxMMNotificationClient::Dispose(void)
{
    DetachFromEndpoint();

    if (m_fRegisteredClient)
    {
        m_pEnum->UnregisterEndpointNotificationCallback(this);

        m_fRegisteredClient = false;
    }
}

/**
 * Initializes the mulitmedia notification client implementation.
 *
 * @return  HRESULT
 */
HRESULT VBoxMMNotificationClient::Initialize(void)
{
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 0, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                  (void **)&m_pEnum);
    if (SUCCEEDED(hr))
    {
        hr = m_pEnum->RegisterEndpointNotificationCallback(this);
        if (SUCCEEDED(hr))
        {
            m_fRegisteredClient = true;

            hr = AttachToDefaultEndpoint();
        }
    }

    LogFunc(("Returning %Rhrc\n",  hr));
    return hr;
}

/**
 * Registration callback implementation for storing our (required) contexts.
 *
 * @return  IPRT status code.
 * @param   pDrvIns             Driver instance to register the notification client to.
 * @param   pfnCallback         Audio callback to call by the notification client in case of new events.
 */
int VBoxMMNotificationClient::RegisterCallback(PPDMDRVINS pDrvIns, PFNPDMHOSTAUDIOCALLBACK pfnCallback)
{
    this->m_pDrvIns     = pDrvIns;
    this->m_pfnCallback = pfnCallback;

    return VINF_SUCCESS;
}

/**
 * Unregistration callback implementation for cleaning up our mess when we're done handling
 * with notifications.
 */
void VBoxMMNotificationClient::UnregisterCallback(void)
{
    this->m_pDrvIns     = NULL;
    this->m_pfnCallback = NULL;
}

/**
 * Stub being called when attaching to the default audio endpoint.
 * Does nothing at the moment.
 */
HRESULT VBoxMMNotificationClient::AttachToDefaultEndpoint(void)
{
    return S_OK;
}

/**
 * Stub being called when detaching from the default audio endpoint.
 * Does nothing at the moment.
 */
void VBoxMMNotificationClient::DetachFromEndpoint(void)
{

}

/**
 * Handler implementation which is called when an audio device state
 * has been changed.
 *
 * @return  HRESULT
 * @param   pwstrDeviceId       Device ID the state is announced for.
 * @param   dwNewState          New state the device is now in.
 */
STDMETHODIMP VBoxMMNotificationClient::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
    char *pszState = "unknown";

    switch (dwNewState)
    {
        case DEVICE_STATE_ACTIVE:
            pszState = "active";
            break;
        case DEVICE_STATE_DISABLED:
            pszState = "disabled";
            break;
        case DEVICE_STATE_NOTPRESENT:
            pszState = "not present";
            break;
        case DEVICE_STATE_UNPLUGGED:
            pszState = "unplugged";
            break;
        default:
            break;
    }

    LogRel2(("Audio: Device '%ls' has changed state to '%s'\n", pwstrDeviceId, pszState));

#ifdef VBOX_WITH_AUDIO_CALLBACKS
    AssertPtr(this->m_pDrvIns);
    AssertPtr(this->m_pfnCallback);

    if (this->m_pfnCallback)
        /* Ignore rc */ this->m_pfnCallback(this->m_pDrvIns, PDMAUDIOBACKENDCBTYPE_DEVICES_CHANGED, NULL, 0);
#endif

    return S_OK;
}

/**
 * Handler implementation which is called when a new audio device has been added.
 *
 * @return  HRESULT
 * @param   pwstrDeviceId       Device ID which has been added.
 */
STDMETHODIMP VBoxMMNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
    RT_NOREF(pwstrDeviceId);
    LogFunc(("%ls\n", pwstrDeviceId));
    return S_OK;
}

/**
 * Handler implementation which is called when an audio device has been removed.
 *
 * @return  HRESULT
 * @param   pwstrDeviceId       Device ID which has been removed.
 */
STDMETHODIMP VBoxMMNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
    RT_NOREF(pwstrDeviceId);
    LogFunc(("%ls\n", pwstrDeviceId));
    return S_OK;
}

/**
 * Handler implementation which is called when the device audio device has been
 * changed.
 *
 * @return  HRESULT
 * @param   eFlow                     Flow direction of the new default device.
 * @param   eRole                     Role of the new default device.
 * @param   pwstrDefaultDeviceId      ID of the new default device.
 */
STDMETHODIMP VBoxMMNotificationClient::OnDefaultDeviceChanged(EDataFlow eFlow, ERole eRole, LPCWSTR pwstrDefaultDeviceId)
{
    RT_NOREF(eFlow, eRole, pwstrDefaultDeviceId);

    if (eFlow == eRender)
    {

    }

    return S_OK;
}

STDMETHODIMP VBoxMMNotificationClient::QueryInterface(REFIID interfaceID, void **ppvInterface)
{
    const IID IID_IMMNotificationClient = __uuidof(IMMNotificationClient);

    if (   IsEqualIID(interfaceID, IID_IUnknown)
        || IsEqualIID(interfaceID, IID_IMMNotificationClient))
    {
        *ppvInterface = static_cast<IMMNotificationClient*>(this);
        AddRef();
        return S_OK;
    }

    *ppvInterface = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) VBoxMMNotificationClient::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) VBoxMMNotificationClient::Release(void)
{
    long lRef = InterlockedDecrement(&m_cRef);
    if (lRef == 0)
        delete this;

    return lRef;
}

