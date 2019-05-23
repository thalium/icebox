/* $Id: VBoxCredProvProvider.h $ */
/** @file
 * VBoxCredProvProvider - The actual credential provider class.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBOX_CREDPROV_PROVIDER_H___
#define ___VBOX_CREDPROV_PROVIDER_H___

#include <credentialprovider.h>
#include <iprt/win/windows.h>
#include <strsafe.h>

#include <VBox/VBoxGuestLib.h>

#include "VBoxCredProvCredential.h"
#include "VBoxCredProvPoller.h"

class VBoxCredProvProvider : public ICredentialProvider
{
public:

    /** @name IUnknown methods.
     * @{ */
    IFACEMETHODIMP_(ULONG) AddRef(void);
    IFACEMETHODIMP_(ULONG) Release(void);
    IFACEMETHODIMP         QueryInterface(REFIID interfaceID, void **ppvInterface);
    /** @}  */


    /** @name ICredentialProvider interface
     * @{ */
    IFACEMETHODIMP SetUsageScenario(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpUsageScenario, DWORD dwFlags);
    IFACEMETHODIMP SetSerialization(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *pcpCredentialSerialization);

    IFACEMETHODIMP Advise(__in ICredentialProviderEvents *pcpEvents, UINT_PTR upAdviseContext);
    IFACEMETHODIMP UnAdvise();

    IFACEMETHODIMP GetFieldDescriptorCount(__out DWORD* pdwCount);
    IFACEMETHODIMP GetFieldDescriptorAt(DWORD dwIndex,  __deref_out CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR **ppFieldDescriptor);

    IFACEMETHODIMP GetCredentialCount(__out DWORD *pdwCount,
                                      __out DWORD *pdwDefault,
                                      __out BOOL *pfAutoLogonWithDefault);
    IFACEMETHODIMP GetCredentialAt(DWORD dwIndex,
                                   __out ICredentialProviderCredential **ppCredProvCredential);
    /** @} */

    friend HRESULT VBoxCredProvProviderCreate(REFIID riid, __deref_out void **ppvInterface);

protected:

    VBoxCredProvProvider(void);
    virtual ~VBoxCredProvProvider(void);

public:

    /** Loads the configuration from the registry. */
    DWORD LoadConfiguration(void);
    /** Determines whether the current session this provider is
     *  loaded into needs to be handled or not. */
    bool HandleCurrentSession(void);
    /** Event which gets triggered by the poller thread in case
     *  there are credentials available from the host. */
    void OnCredentialsProvided(void);

private:

    /** Interface reference count. */
    LONG                                     m_cRefs;
    /** Our one and only credential. */
    VBoxCredProvCredential                  *m_pCred;
    /** Poller thread for credential lookup. */
    VBoxCredProvPoller                      *m_pPoller;
    /** Used to tell our owner to re-enumerate credentials. */
    ICredentialProviderEvents               *m_pEvents;
    /** Used to tell our owner who we are when asking to re-enumerate credentials. */
    UINT_PTR                                 m_upAdviseContext;
    /** Saved usage scenario. */
    CREDENTIAL_PROVIDER_USAGE_SCENARIO       m_enmUsageScenario;
    /** Flag whether we need to handle remote session over Windows Remote
     *  Desktop Service. */
    bool                                     m_fHandleRemoteSessions;
};

#endif /* !___VBOX_CREDPROV_PROVIDER_H___ */

