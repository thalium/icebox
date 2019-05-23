/* $Id: VBoxCredProvFactory.cpp $ */
/** @file
 * VBoxCredentialProvFactory - The VirtualBox Credential Provider Factory.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "VBoxCredentialProvider.h"
#include "VBoxCredProvFactory.h"
#include "VBoxCredProvProvider.h"


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
extern HRESULT VBoxCredProvProviderCreate(REFIID interfaceID, void **ppvInterface);


VBoxCredProvFactory::VBoxCredProvFactory(void) :
    m_cRefs(1) /* Start with one instance. */
{
}

VBoxCredProvFactory::~VBoxCredProvFactory(void)
{
}

ULONG
VBoxCredProvFactory::AddRef(void)
{
    LONG cRefs = InterlockedIncrement(&m_cRefs);
    VBoxCredProvVerbose(0, "VBoxCredProvFactory: AddRef: Returning refcount=%ld\n",
                        cRefs);
    return cRefs;
}

ULONG
VBoxCredProvFactory::Release(void)
{
    LONG cRefs = InterlockedDecrement(&m_cRefs);
    VBoxCredProvVerbose(0, "VBoxCredProvFactory: Release: Returning refcount=%ld\n",
                        cRefs);
    if (!cRefs)
    {
        VBoxCredProvVerbose(0, "VBoxCredProvFactory: Calling destructor\n");
        delete this;
    }
    return cRefs;
}

HRESULT
VBoxCredProvFactory::QueryInterface(REFIID interfaceID, void **ppvInterface)
{
    VBoxCredProvVerbose(0, "VBoxCredProvFactory: QueryInterface\n");

    HRESULT hr = S_OK;
    if (ppvInterface)
    {
        if (   IID_IClassFactory == interfaceID
            || IID_IUnknown      == interfaceID)
        {
            *ppvInterface = static_cast<IUnknown*>(this);
            reinterpret_cast<IUnknown*>(*ppvInterface)->AddRef();
        }
        else
        {
            *ppvInterface = NULL;
            hr = E_NOINTERFACE;
        }
    }
    else
        hr = E_INVALIDARG;
    return hr;
}

HRESULT
VBoxCredProvFactory::CreateInstance(IUnknown *pUnkOuter, REFIID interfaceID, void **ppvInterface)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;
    return VBoxCredProvProviderCreate(interfaceID, ppvInterface);
}

HRESULT
VBoxCredProvFactory::LockServer(BOOL fLock)
{
    if (fLock)
        VBoxCredentialProviderAcquire();
    else
        VBoxCredentialProviderRelease();
    return S_OK;
}

