/* $Id: VBoxDnDDataObject.cpp $ */
/** @file
 * VBoxDnDDataObject.cpp - IDataObject implementation.
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
#include <iprt/win/shlobj.h>

#include <iprt/path.h>
#include <iprt/semaphore.h>
#include <iprt/uri.h>

#ifdef LOG_GROUP
# undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_GUEST_DND
#include <VBox/log.h>

#include "VBoxTray.h"
#include "VBoxHelpers.h"
#include "VBoxDnD.h"

#ifdef DEBUG
  /* Enable the following line to get much more debug output about
   * (un)known clipboard formats. */
//# define VBOX_DND_DEBUG_FORMATS
#endif

/** @todo Implement IDataObjectAsyncCapability interface? */

VBoxDnDDataObject::VBoxDnDDataObject(LPFORMATETC pFormatEtc, LPSTGMEDIUM pStgMed, ULONG cFormats)
    : mStatus(Uninitialized),
      mRefCount(1),
      mcFormats(0),
      mpvData(NULL),
      mcbData(0)
{
    HRESULT hr;

    ULONG cFixedFormats = 1;
    ULONG cAllFormats   = cFormats + cFixedFormats;

    try
    {
        mpFormatEtc = new FORMATETC[cAllFormats];
        RT_BZERO(mpFormatEtc, sizeof(FORMATETC) * cAllFormats);
        mpStgMedium = new STGMEDIUM[cAllFormats];
        RT_BZERO(mpStgMedium, sizeof(STGMEDIUM) * cAllFormats);

        /*
         * Registration of dynamic formats needed?
         */
        LogFlowFunc(("%RU32 dynamic formats\n", cFormats));
        if (cFormats)
        {
            AssertPtr(pFormatEtc);
            AssertPtr(pStgMed);

            for (ULONG i = 0; i < cFormats; i++)
            {
                LogFlowFunc(("Format %RU32: cfFormat=%RI16, tyMed=%RU32, dwAspect=%RU32\n",
                             i, pFormatEtc[i].cfFormat, pFormatEtc[i].tymed, pFormatEtc[i].dwAspect));
                mpFormatEtc[i] = pFormatEtc[i];
                mpStgMedium[i] = pStgMed[i];
            }
        }

        hr = S_OK;
    }
    catch (std::bad_alloc &)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        int rc2 = RTSemEventCreate(&mSemEvent);
        AssertRC(rc2);

        /*
         * Register fixed formats.
         */
#if 0
        /* CF_HDROP. */
        RegisterFormat(&mpFormatEtc[cFormats], CF_HDROP);
        mpStgMedium[cFormats++].tymed = TYMED_HGLOBAL;

        /* IStream. */
        RegisterFormat(&mpFormatEtc[cFormats++],
                       RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR));
        RegisterFormat(&mpFormatEtc[cFormats++],
                       RegisterClipboardFormat(CFSTR_FILECONTENTS),
                       TYMED_ISTREAM, 0 /* lIndex */);

        /* Required for e.g. Windows Media Player. */
        RegisterFormat(&mpFormatEtc[cFormats++],
                       RegisterClipboardFormat(CFSTR_FILENAME));
        RegisterFormat(&mpFormatEtc[cFormats++],
                       RegisterClipboardFormat(CFSTR_FILENAMEW));
        RegisterFormat(&mpFormatEtc[cFormats++],
                       RegisterClipboardFormat(CFSTR_SHELLIDLIST));
        RegisterFormat(&mpFormatEtc[cFormats++],
                       RegisterClipboardFormat(CFSTR_SHELLIDLISTOFFSET));
#endif
        mcFormats = cFormats;
        mStatus   = Initialized;
    }

    LogFlowFunc(("cFormats=%RU32, hr=%Rhrc\n", cFormats, hr));
}

VBoxDnDDataObject::~VBoxDnDDataObject(void)
{
    if (mpFormatEtc)
        delete[] mpFormatEtc;

    if (mpStgMedium)
        delete[] mpStgMedium;

    if (mpvData)
        RTMemFree(mpvData);

    LogFlowFunc(("mRefCount=%RI32\n", mRefCount));
}

/*
 * IUnknown methods.
 */

STDMETHODIMP_(ULONG) VBoxDnDDataObject::AddRef(void)
{
    return InterlockedIncrement(&mRefCount);
}

STDMETHODIMP_(ULONG) VBoxDnDDataObject::Release(void)
{
    LONG lCount = InterlockedDecrement(&mRefCount);
    if (lCount == 0)
    {
        delete this;
        return 0;
    }

    return lCount;
}

STDMETHODIMP VBoxDnDDataObject::QueryInterface(REFIID iid, void **ppvObject)
{
    AssertPtrReturn(ppvObject, E_INVALIDARG);

    if (   iid == IID_IDataObject
        || iid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }

    *ppvObject = 0;
    return E_NOINTERFACE;
}

/**
 * Retrieves the data stored in this object and store the result in
 * pMedium.
 *
 * @return  IPRT status code.
 * @return  HRESULT
 * @param   pFormatEtc
 * @param   pMedium
 */
STDMETHODIMP VBoxDnDDataObject::GetData(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium)
{
    AssertPtrReturn(pFormatEtc, DV_E_FORMATETC);
    AssertPtrReturn(pMedium, DV_E_FORMATETC);

    ULONG lIndex;
    if (!LookupFormatEtc(pFormatEtc, &lIndex)) /* Format supported? */
        return DV_E_FORMATETC;
    if (lIndex >= mcFormats) /* Paranoia. */
        return DV_E_FORMATETC;

    LPFORMATETC pThisFormat = &mpFormatEtc[lIndex];
    AssertPtr(pThisFormat);

    LPSTGMEDIUM pThisMedium = &mpStgMedium[lIndex];
    AssertPtr(pThisMedium);

    LogFlowFunc(("Using pThisFormat=%p, pThisMedium=%p\n", pThisFormat, pThisMedium));

    HRESULT hr = DV_E_FORMATETC; /* Play safe. */

    LogFlowFunc(("mStatus=%ld\n", mStatus));
    if (mStatus == Dropping)
    {
        LogFlowFunc(("Waiting for event ...\n"));
        int rc2 = RTSemEventWait(mSemEvent, RT_INDEFINITE_WAIT);
        LogFlowFunc(("rc2=%Rrc, mStatus=%ld\n", rc2, mStatus)); NOREF(rc2);
    }

    if (mStatus == Dropped)
    {
        LogRel3(("DnD: cfFormat=%RI16, sFormat=%s, tyMed=%RU32, dwAspect=%RU32\n",
                 pThisFormat->cfFormat, VBoxDnDDataObject::ClipboardFormatToString(pFormatEtc->cfFormat),
                 pThisFormat->tymed, pThisFormat->dwAspect));
        LogRel3(("DnD: Got strFormat=%s, pvData=%p, cbData=%RU32\n",
                  mstrFormat.c_str(), mpvData, mcbData));

        /*
         * Initialize default values.
         */
        pMedium->tymed          = pThisFormat->tymed;
        pMedium->pUnkForRelease = NULL;

        /*
         * URI list handling.
         */
        if (mstrFormat.equalsIgnoreCase("text/uri-list"))
        {
            int rc = VINF_SUCCESS;

            RTCList<RTCString> lstFilesURI = RTCString((char*)mpvData, mcbData).split("\r\n");
            RTCList<RTCString> lstFiles;
            for (size_t i = 0; i < lstFilesURI.size(); i++)
            {
                char *pszFilePath = RTUriFilePath(lstFilesURI.at(i).c_str());
                if (pszFilePath)
                {
                    lstFiles.append(pszFilePath);
                    RTStrFree(pszFilePath);
                }
                else /* Unable to parse -- refuse entire request. */
                {
                    lstFiles.clear();
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }
            }

            size_t cFiles = lstFiles.size();
            if (   RT_SUCCESS(rc)
                && cFiles)
            {
#ifdef DEBUG
                LogFlowFunc(("Files (%zu)\n", cFiles));
                for (size_t i = 0; i < cFiles; i++)
                    LogFlowFunc(("\tFile: %s\n", lstFiles.at(i).c_str()));
#endif

#if 0
                if (   (pFormatEtc->tymed & TYMED_ISTREAM)
                    && (pFormatEtc->dwAspect == DVASPECT_CONTENT)
                    && (pFormatEtc->cfFormat == CF_FILECONTENTS))
                {

                }
                else if  (   (pFormatEtc->tymed & TYMED_HGLOBAL)
                          && (pFormatEtc->dwAspect == DVASPECT_CONTENT)
                          && (pFormatEtc->cfFormat == CF_FILEDESCRIPTOR))
                {

                }
                else if (   (pFormatEtc->tymed & TYMED_HGLOBAL)
                         && (pFormatEtc->cfFormat == CF_PREFERREDDROPEFFECT))
                {
                    HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_ZEROINIT, sizeof(DWORD));
                    DWORD *pdwEffect = (DWORD *)GlobalLock(hData);
                    AssertPtr(pdwEffect);
                    *pdwEffect = DROPEFFECT_COPY;
                    GlobalUnlock(hData);

                    pMedium->hGlobal = hData;
                    pMedium->tymed = TYMED_HGLOBAL;
                }
                else
#endif
                if (   (pFormatEtc->tymed & TYMED_HGLOBAL)
                    && (pFormatEtc->dwAspect == DVASPECT_CONTENT)
                    && (pFormatEtc->cfFormat == CF_TEXT))
                {
                    pMedium->hGlobal = GlobalAlloc(GHND, mcbData + 1);
                    if (pMedium->hGlobal)
                    {
                        char *pcDst  = (char *)GlobalLock(pMedium->hGlobal);
                        memcpy(pcDst, mpvData, mcbData);
                        pcDst[mcbData] = '\0';
                        GlobalUnlock(pMedium->hGlobal);

                        hr = S_OK;
                    }
                }
                else if (   (pFormatEtc->tymed & TYMED_HGLOBAL)
                         && (pFormatEtc->dwAspect == DVASPECT_CONTENT)
                         && (pFormatEtc->cfFormat == CF_HDROP))
                {
                    size_t cchFiles = 0; /* Number of ASCII characters. */
                    for (size_t i = 0; i < cFiles; i++)
                    {
                        cchFiles += strlen(lstFiles.at(i).c_str());
                        cchFiles += 1; /* Terminating '\0'. */
                    }

                    size_t cbBuf = sizeof(DROPFILES) + ((cchFiles + 1) * sizeof(RTUTF16));
                    DROPFILES *pBuf = (DROPFILES *)RTMemAllocZ(cbBuf);
                    if (pBuf)
                    {
                        pBuf->pFiles = sizeof(DROPFILES);
                        pBuf->fWide = 1; /* We use unicode. Always. */

                        uint8_t *pCurFile = (uint8_t *)pBuf + pBuf->pFiles;
                        AssertPtr(pCurFile);

                        for (size_t i = 0; i < cFiles && RT_SUCCESS(rc); i++)
                        {
                            size_t cchCurFile;
                            PRTUTF16 pwszFile;
                            rc = RTStrToUtf16(lstFiles.at(i).c_str(), &pwszFile);
                            if (RT_SUCCESS(rc))
                            {
                                cchCurFile = RTUtf16Len(pwszFile);
                                Assert(cchCurFile);
                                memcpy(pCurFile, pwszFile, cchCurFile * sizeof(RTUTF16));
                                RTUtf16Free(pwszFile);
                            }
                            else
                                break;

                            pCurFile += cchCurFile * sizeof(RTUTF16);

                            /* Terminate current file name. */
                            *pCurFile = L'\0';
                            pCurFile += sizeof(RTUTF16);
                        }

                        if (RT_SUCCESS(rc))
                        {
                            *pCurFile = L'\0'; /* Final list terminator. */

                            pMedium->tymed = TYMED_HGLOBAL;
                            pMedium->pUnkForRelease = NULL;
                            pMedium->hGlobal = GlobalAlloc(  GMEM_ZEROINIT
                                                           | GMEM_MOVEABLE
                                                           | GMEM_DDESHARE, cbBuf);
                            if (pMedium->hGlobal)
                            {
                                LPVOID pMem = GlobalLock(pMedium->hGlobal);
                                if (pMem)
                                {
                                    memcpy(pMem, pBuf, cbBuf);
                                    GlobalUnlock(pMedium->hGlobal);

                                    hr = S_OK;
                                }
                            }
                        }

                        RTMemFree(pBuf);
                    }
                    else
                        rc = VERR_NO_MEMORY;
                }
            }

            if (RT_FAILURE(rc))
                hr = DV_E_FORMATETC;
        }
        /*
         * Plain text handling.
         */
        else if (   mstrFormat.equalsIgnoreCase("text/plain")
                 || mstrFormat.equalsIgnoreCase("text/html")
                 || mstrFormat.equalsIgnoreCase("text/plain;charset=utf-8")
                 || mstrFormat.equalsIgnoreCase("text/plain;charset=utf-16")
                 || mstrFormat.equalsIgnoreCase("text/richtext")
                 || mstrFormat.equalsIgnoreCase("UTF8_STRING")
                 || mstrFormat.equalsIgnoreCase("TEXT")
                 || mstrFormat.equalsIgnoreCase("STRING"))
        {
            pMedium->hGlobal = GlobalAlloc(GHND, mcbData + 1);
            if (pMedium->hGlobal)
            {
                char *pcDst  = (char *)GlobalLock(pMedium->hGlobal);
                memcpy(pcDst, mpvData, mcbData);
                pcDst[mcbData] = '\0';
                GlobalUnlock(pMedium->hGlobal);

                hr = S_OK;
            }
        }
        else
            LogRel(("DnD: Error: Format '%s' not implemented\n", mstrFormat.c_str()));
    }

    /* Error handling; at least return some basic data. */
    if (FAILED(hr))
    {
        LogFlowFunc(("Copying medium ...\n"));
        switch (pThisMedium->tymed)
        {

        case TYMED_HGLOBAL:
            pMedium->hGlobal = (HGLOBAL)OleDuplicateData(pThisMedium->hGlobal,
                                                         pThisFormat->cfFormat, NULL);
            break;

        default:
            break;
        }

        pMedium->tymed          = pThisFormat->tymed;
        pMedium->pUnkForRelease = NULL;
    }

    if (hr == DV_E_FORMATETC)
        LogRel(("DnD: Error handling format '%s' (%RU32 bytes)\n", mstrFormat.c_str(), mcbData));

    LogFlowFunc(("hr=%Rhrc\n", hr));
    return hr;
}

/**
 * Only required for IStream / IStorage interfaces.
 *
 * @return  IPRT status code.
 * @return  HRESULT
 * @param   pFormatEtc
 * @param   pMedium
 */
STDMETHODIMP VBoxDnDDataObject::GetDataHere(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium)
{
    RT_NOREF(pFormatEtc, pMedium);
    LogFlowFunc(("\n"));
    return DATA_E_FORMATETC;
}

/**
 * Query if this objects supports a specific format.
 *
 * @return  IPRT status code.
 * @return  HRESULT
 * @param   pFormatEtc
 */
STDMETHODIMP VBoxDnDDataObject::QueryGetData(LPFORMATETC pFormatEtc)
{
    LogFlowFunc(("\n"));
    return (LookupFormatEtc(pFormatEtc, NULL /* puIndex */)) ? S_OK : DV_E_FORMATETC;
}

STDMETHODIMP VBoxDnDDataObject::GetCanonicalFormatEtc(LPFORMATETC pFormatEtc, LPFORMATETC pFormatEtcOut)
{
    RT_NOREF(pFormatEtc);
    LogFlowFunc(("\n"));

    /* Set this to NULL in any case. */
    pFormatEtcOut->ptd = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP VBoxDnDDataObject::SetData(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium, BOOL fRelease)
{
    RT_NOREF(pFormatEtc, pMedium, fRelease);
    return E_NOTIMPL;
}

STDMETHODIMP VBoxDnDDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
{
    LogFlowFunc(("dwDirection=%RI32, mcFormats=%RI32, mpFormatEtc=%p\n", dwDirection, mcFormats, mpFormatEtc));

    HRESULT hr;
    if (dwDirection == DATADIR_GET)
        hr = VBoxDnDEnumFormatEtc::CreateEnumFormatEtc(mcFormats, mpFormatEtc, ppEnumFormatEtc);
    else
        hr = E_NOTIMPL;

    LogFlowFunc(("hr=%Rhrc\n", hr));
    return hr;
}

STDMETHODIMP VBoxDnDDataObject::DAdvise(LPFORMATETC pFormatEtc, DWORD fAdvise, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    RT_NOREF(pFormatEtc, fAdvise, pAdvSink, pdwConnection);
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP VBoxDnDDataObject::DUnadvise(DWORD dwConnection)
{
    RT_NOREF(dwConnection);
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP VBoxDnDDataObject::EnumDAdvise(IEnumSTATDATA **ppEnumAdvise)
{
    RT_NOREF(ppEnumAdvise);
    return OLE_E_ADVISENOTSUPPORTED;
}

/*
 * Own stuff.
 */

int VBoxDnDDataObject::Abort(void)
{
    LogFlowFunc(("Aborting ...\n"));
    mStatus = Aborted;
    return RTSemEventSignal(mSemEvent);
}

/* static */
const char* VBoxDnDDataObject::ClipboardFormatToString(CLIPFORMAT fmt)
{
#if 0
    char szFormat[128];
    if (GetClipboardFormatName(fmt, szFormat, sizeof(szFormat)))
        LogFlowFunc(("wFormat=%RI16, szName=%s\n", fmt, szFormat));
#endif

    switch (fmt)
    {

    case 1:
        return "CF_TEXT";
    case 2:
        return "CF_BITMAP";
    case 3:
        return "CF_METAFILEPICT";
    case 4:
        return "CF_SYLK";
    case 5:
        return "CF_DIF";
    case 6:
        return "CF_TIFF";
    case 7:
        return "CF_OEMTEXT";
    case 8:
        return "CF_DIB";
    case 9:
        return "CF_PALETTE";
    case 10:
        return "CF_PENDATA";
    case 11:
        return "CF_RIFF";
    case 12:
        return "CF_WAVE";
    case 13:
        return "CF_UNICODETEXT";
    case 14:
        return "CF_ENHMETAFILE";
    case 15:
        return "CF_HDROP";
    case 16:
        return "CF_LOCALE";
    case 17:
        return "CF_DIBV5";
    case 18:
        return "CF_MAX";
    case 49158:
        return "FileName";
    case 49159:
        return "FileNameW";
    case 49161:
        return "DATAOBJECT";
    case 49171:
        return "Ole Private Data";
    case 49314:
        return "Shell Object Offsets";
    case 49316:
        return "File Contents";
    case 49317:
        return "File Group Descriptor";
    case 49323:
        return "Preferred Drop Effect";
    case 49380:
        return "Shell Object Offsets";
    case 49382:
        return "FileContents";
    case 49383:
        return "FileGroupDescriptor";
    case 49389:
        return "Preferred DropEffect";
    case 49268:
        return "Shell IDList Array";
    case 49619:
        return "RenPrivateFileAttachments";
    default:
        break;
    }

    return "unknown";
}

bool VBoxDnDDataObject::LookupFormatEtc(LPFORMATETC pFormatEtc, ULONG *puIndex)
{
    AssertReturn(pFormatEtc, false);
    /* puIndex is optional. */

    for (ULONG i = 0; i < mcFormats; i++)
    {
        if(    (pFormatEtc->tymed & mpFormatEtc[i].tymed)
            && pFormatEtc->cfFormat == mpFormatEtc[i].cfFormat
            && pFormatEtc->dwAspect == mpFormatEtc[i].dwAspect)
        {
            LogRel3(("DnD: Format found: tyMed=%RI32, cfFormat=%RI16, sFormats=%s, dwAspect=%RI32, ulIndex=%RU32\n",
                      pFormatEtc->tymed, pFormatEtc->cfFormat, VBoxDnDDataObject::ClipboardFormatToString(mpFormatEtc[i].cfFormat),
                      pFormatEtc->dwAspect, i));
            if (puIndex)
                *puIndex = i;
            return true;
        }
    }

    LogRel3(("DnD: Format NOT found: tyMed=%RI32, cfFormat=%RI16, sFormats=%s, dwAspect=%RI32\n",
             pFormatEtc->tymed, pFormatEtc->cfFormat, VBoxDnDDataObject::ClipboardFormatToString(pFormatEtc->cfFormat),
             pFormatEtc->dwAspect));

    return false;
}

/* static */
HGLOBAL VBoxDnDDataObject::MemDup(HGLOBAL hMemSource)
{
    DWORD dwLen    = GlobalSize(hMemSource);
    AssertReturn(dwLen, NULL);
    PVOID pvSource = GlobalLock(hMemSource);
    if (pvSource)
    {
        PVOID pvDest = GlobalAlloc(GMEM_FIXED, dwLen);
        if (pvDest)
            memcpy(pvDest, pvSource, dwLen);

        GlobalUnlock(hMemSource);
        return pvDest;
    }

    return NULL;
}

void VBoxDnDDataObject::RegisterFormat(LPFORMATETC pFormatEtc, CLIPFORMAT clipFormat,
                                       TYMED tyMed, LONG lIndex, DWORD dwAspect,
                                       DVTARGETDEVICE *pTargetDevice)
{
    AssertPtr(pFormatEtc);

    pFormatEtc->cfFormat = clipFormat;
    pFormatEtc->tymed    = tyMed;
    pFormatEtc->lindex   = lIndex;
    pFormatEtc->dwAspect = dwAspect;
    pFormatEtc->ptd      = pTargetDevice;

    LogFlowFunc(("Registered format=%ld, sFormat=%s\n",
                 pFormatEtc->cfFormat, VBoxDnDDataObject::ClipboardFormatToString(pFormatEtc->cfFormat)));
}

void VBoxDnDDataObject::SetStatus(Status status)
{
    LogFlowFunc(("Setting status to %ld\n", status));
    mStatus = status;
}

int VBoxDnDDataObject::Signal(const RTCString &strFormat,
                              const void *pvData, uint32_t cbData)
{
    LogFlowFunc(("Signalling ...\n"));

    int rc;

    mStatus = Dropped;
    mstrFormat = strFormat;
    if (cbData)
    {
        mpvData = RTMemAlloc(cbData);
        if (mpvData)
        {
            memcpy(mpvData, pvData, cbData);
            mcbData = cbData;
            rc = VINF_SUCCESS;
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else
        rc = VINF_SUCCESS;

    if (RT_FAILURE(rc))
        mStatus = Aborted;

    /* Signal in any case. */
    int rc2 = RTSemEventSignal(mSemEvent);
    if (RT_SUCCESS(rc))
        rc = rc2;

    return rc;
}

