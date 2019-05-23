/* $Id: VBoxDnDEnumFormat.cpp $ */
/** @file
 * VBoxDnDEnumFormat.cpp - IEnumFORMATETC ("Format et cetera") implementation.
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

#include <iprt/win/windows.h>
#include <new> /* For bad_alloc. */

#include "VBoxTray.h"
#include "VBoxHelpers.h"
#include "VBoxDnD.h"

#ifdef DEBUG
# define LOG_ENABLED
# define LOG_GROUP LOG_GROUP_DEFAULT
#endif
#include <VBox/log.h>



VBoxDnDEnumFormatEtc::VBoxDnDEnumFormatEtc(LPFORMATETC pFormatEtc, ULONG cFormats)
    : m_lRefCount(1),
      m_nIndex(0)
{
    HRESULT hr;

    try
    {
        LogFlowFunc(("pFormatEtc=%p, cFormats=%RU32\n", pFormatEtc, cFormats));
        m_pFormatEtc  = new FORMATETC[cFormats];

        for (ULONG i = 0; i < cFormats; i++)
        {
            LogFlowFunc(("Format %RU32: cfFormat=%RI16, sFormat=%s, tyMed=%RU32, dwAspect=%RU32\n",
                         i, pFormatEtc[i].cfFormat, VBoxDnDDataObject::ClipboardFormatToString(pFormatEtc[i].cfFormat),
                         pFormatEtc[i].tymed, pFormatEtc[i].dwAspect));
            VBoxDnDEnumFormatEtc::CopyFormat(&m_pFormatEtc[i], &pFormatEtc[i]);
        }

        m_nNumFormats = cFormats;
        hr = S_OK;
    }
    catch (std::bad_alloc &)
    {
        hr = E_OUTOFMEMORY;
    }

    LogFlowFunc(("hr=%Rhrc\n", hr));
}

VBoxDnDEnumFormatEtc::~VBoxDnDEnumFormatEtc(void)
{
    if (m_pFormatEtc)
    {
        for (ULONG i = 0; i < m_nNumFormats; i++)
        {
            if(m_pFormatEtc[i].ptd)
                CoTaskMemFree(m_pFormatEtc[i].ptd);
        }

        delete[] m_pFormatEtc;
        m_pFormatEtc = NULL;
    }

    LogFlowFunc(("m_lRefCount=%RI32\n", m_lRefCount));
}

/*
 * IUnknown methods.
 */

STDMETHODIMP_(ULONG) VBoxDnDEnumFormatEtc::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG) VBoxDnDEnumFormatEtc::Release(void)
{
    LONG lCount = InterlockedDecrement(&m_lRefCount);
    if (lCount == 0)
    {
        delete this;
        return 0;
    }

    return lCount;
}

STDMETHODIMP VBoxDnDEnumFormatEtc::QueryInterface(REFIID iid, void **ppvObject)
{
    if (   iid == IID_IEnumFORMATETC
        || iid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }

    *ppvObject = 0;
    return E_NOINTERFACE;
}

STDMETHODIMP VBoxDnDEnumFormatEtc::Next(ULONG cFormats, LPFORMATETC pFormatEtc, ULONG *pcFetched)
{
    ULONG ulCopied  = 0;

    if(cFormats == 0 || pFormatEtc == 0)
        return E_INVALIDARG;

    while (   m_nIndex < m_nNumFormats
           && ulCopied < cFormats)
    {
        VBoxDnDEnumFormatEtc::CopyFormat(&pFormatEtc[ulCopied],
                                         &m_pFormatEtc[m_nIndex]);
        ulCopied++;
        m_nIndex++;
    }

    if (pcFetched)
        *pcFetched = ulCopied;

    return (ulCopied == cFormats) ? S_OK : S_FALSE;
}

STDMETHODIMP VBoxDnDEnumFormatEtc::Skip(ULONG cFormats)
{
    m_nIndex += cFormats;
    return (m_nIndex <= m_nNumFormats) ? S_OK : S_FALSE;
}

STDMETHODIMP VBoxDnDEnumFormatEtc::Reset(void)
{
    m_nIndex = 0;
    return S_OK;
}

STDMETHODIMP VBoxDnDEnumFormatEtc::Clone(IEnumFORMATETC **ppEnumFormatEtc)
{
    HRESULT hResult =
        CreateEnumFormatEtc(m_nNumFormats, m_pFormatEtc, ppEnumFormatEtc);

    if (hResult == S_OK)
        ((VBoxDnDEnumFormatEtc *) *ppEnumFormatEtc)->m_nIndex = m_nIndex;

    return hResult;
}

/* static */
void VBoxDnDEnumFormatEtc::CopyFormat(LPFORMATETC pDest, LPFORMATETC pSource)
{
    AssertPtrReturnVoid(pDest);
    AssertPtrReturnVoid(pSource);

    *pDest = *pSource;

    if (pSource->ptd)
    {
        pDest->ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));
        *(pDest->ptd) = *(pSource->ptd);
    }
}

/* static */
HRESULT VBoxDnDEnumFormatEtc::CreateEnumFormatEtc(UINT nNumFormats, LPFORMATETC pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc)
{
    AssertReturn(nNumFormats, E_INVALIDARG);
    AssertPtrReturn(pFormatEtc, E_INVALIDARG);
    AssertPtrReturn(ppEnumFormatEtc, E_INVALIDARG);

    HRESULT hr;
    try
    {
        *ppEnumFormatEtc = new VBoxDnDEnumFormatEtc(pFormatEtc, nNumFormats);
        hr = S_OK;
    }
    catch(std::bad_alloc &)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

