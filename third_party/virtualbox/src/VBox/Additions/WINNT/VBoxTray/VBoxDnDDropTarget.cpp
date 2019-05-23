/* $Id: VBoxDnDDropTarget.cpp $ */
/** @file
 * VBoxDnDTarget.cpp - IDropTarget implementation.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
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
#include <iprt/win/shlobj.h> /* For DROPFILES and friends. */

#include "VBoxTray.h"
#include "VBoxHelpers.h"
#include "VBoxDnD.h"

#include "VBox/GuestHost/DragAndDrop.h"
#include "VBox/HostServices/DragAndDropSvc.h"

#ifdef LOG_GROUP
# undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_GUEST_DND
#include <VBox/log.h>



VBoxDnDDropTarget::VBoxDnDDropTarget(VBoxDnDWnd *pParent)
    : mRefCount(1),
      mpWndParent(pParent),
      mdwCurEffect(0),
      mpvData(NULL),
      mcbData(0),
      hEventDrop(NIL_RTSEMEVENT)
{
    int rc = RTSemEventCreate(&hEventDrop);
    LogFlowFunc(("rc=%Rrc\n", rc)); NOREF(rc);
}

VBoxDnDDropTarget::~VBoxDnDDropTarget(void)
{
    reset();

    int rc2 = RTSemEventDestroy(hEventDrop);
    AssertRC(rc2);

    LogFlowFunc(("rc=%Rrc, mRefCount=%RI32\n", rc2, mRefCount));
}

/*
 * IUnknown methods.
 */

STDMETHODIMP_(ULONG) VBoxDnDDropTarget::AddRef(void)
{
    return InterlockedIncrement(&mRefCount);
}

STDMETHODIMP_(ULONG) VBoxDnDDropTarget::Release(void)
{
    LONG lCount = InterlockedDecrement(&mRefCount);
    if (lCount == 0)
    {
        delete this;
        return 0;
    }

    return lCount;
}

STDMETHODIMP VBoxDnDDropTarget::QueryInterface(REFIID iid, void **ppvObject)
{
    AssertPtrReturn(ppvObject, E_INVALIDARG);

    if (   iid == IID_IDropSource
        || iid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }

    *ppvObject = 0;
    return E_NOINTERFACE;
}

/* static */
void VBoxDnDDropTarget::DumpFormats(IDataObject *pDataObject)
{
    AssertPtrReturnVoid(pDataObject);

    /* Enumerate supported source formats. This shouldn't happen too often
     * on day to day use, but still keep it in here. */
    IEnumFORMATETC *pEnumFormats;
    HRESULT hr2 = pDataObject->EnumFormatEtc(DATADIR_GET, &pEnumFormats);
    if (SUCCEEDED(hr2))
    {
        LogRel(("DnD: The following formats were offered to us:\n"));

        FORMATETC curFormatEtc;
        while (pEnumFormats->Next(1, &curFormatEtc,
                                  NULL /* pceltFetched */) == S_OK)
        {
            WCHAR wszCfName[128]; /* 128 chars should be enough, rest will be truncated. */
            hr2 = GetClipboardFormatNameW(curFormatEtc.cfFormat, wszCfName,
                                          sizeof(wszCfName) / sizeof(WCHAR));
            LogRel(("\tcfFormat=%RI16 (%s), tyMed=%RI32, dwAspect=%RI32, strCustomName=%ls, hr=%Rhrc\n",
                    curFormatEtc.cfFormat,
                    VBoxDnDDataObject::ClipboardFormatToString(curFormatEtc.cfFormat),
                    curFormatEtc.tymed,
                    curFormatEtc.dwAspect,
                    wszCfName, hr2));
        }

        pEnumFormats->Release();
    }
}

/*
 * IDropTarget methods.
 */

STDMETHODIMP VBoxDnDDropTarget::DragEnter(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    RT_NOREF(pt);
    AssertPtrReturn(pDataObject, E_INVALIDARG);
    AssertPtrReturn(pdwEffect, E_INVALIDARG);

    LogFlowFunc(("pDataObject=0x%p, grfKeyState=0x%x, x=%ld, y=%ld, dwEffect=%RU32\n",
                 pDataObject, grfKeyState, pt.x, pt.y, *pdwEffect));

    reset();

    /** @todo At the moment we only support one DnD format at a time. */

#ifdef DEBUG
    VBoxDnDDropTarget::DumpFormats(pDataObject);
#endif

    /* Try different formats.
     * CF_HDROP is the most common one, so start with this. */
    FORMATETC fmtEtc = { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    HRESULT hr = pDataObject->QueryGetData(&fmtEtc);
    if (hr == S_OK)
    {
        mstrFormats = "text/uri-list";
    }
    else
    {
        LogFlowFunc(("CF_HDROP not wanted, hr=%Rhrc\n", hr));

        /* So we couldn't retrieve the data in CF_HDROP format; try with
         * CF_UNICODETEXT + CF_TEXT formats now. Rest stays the same. */
        fmtEtc.cfFormat = CF_UNICODETEXT;
        hr = pDataObject->QueryGetData(&fmtEtc);
        if (hr == S_OK)
        {
            mstrFormats = "text/plain;charset=utf-8";
        }
        else
        {
            LogFlowFunc(("CF_UNICODETEXT not wanted, hr=%Rhrc\n", hr));

            fmtEtc.cfFormat = CF_TEXT;
            hr = pDataObject->QueryGetData(&fmtEtc);
            if (hr == S_OK)
            {
                mstrFormats = "text/plain;charset=utf-8";
            }
            else
            {
                LogFlowFunc(("CF_TEXT not wanted, hr=%Rhrc\n", hr));
                fmtEtc.cfFormat = 0; /* Set it to non-supported. */

                /* Clean up. */
                reset();
            }
        }
    }

    /* Did we find a format that we support? */
    if (fmtEtc.cfFormat)
    {
        LogFlowFunc(("Found supported format %RI16 (%s)\n",
                     fmtEtc.cfFormat, VBoxDnDDataObject::ClipboardFormatToString(fmtEtc.cfFormat)));

        /* Make a copy of the FORMATETC structure so that we later can
         * use this for comparrison and stuff. */
        /** @todo The DVTARGETDEVICE member only is a shallow copy for now! */
        memcpy(&mFormatEtc, &fmtEtc, sizeof(FORMATETC));

        /* Which drop effect we're going to use? */
        /* Note: pt is not used since we don't need to differentiate within our
         *       proxy window. */
        *pdwEffect = VBoxDnDDropTarget::GetDropEffect(grfKeyState, *pdwEffect);
    }
    else
    {
        /* No or incompatible data -- so no drop effect required. */
        *pdwEffect = DROPEFFECT_NONE;

        switch (hr)
        {
            case ERROR_INVALID_FUNCTION:
            {
                LogRel(("DnD: Drag and drop format is not supported by VBoxTray\n"));
                VBoxDnDDropTarget::DumpFormats(pDataObject);
                break;
            }

            default:
                break;
        }
    }

    LogFlowFunc(("Returning mstrFormats=%s, cfFormat=%RI16, pdwEffect=%ld, hr=%Rhrc\n",
                 mstrFormats.c_str(), fmtEtc.cfFormat, *pdwEffect, hr));
    return hr;
}

STDMETHODIMP VBoxDnDDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    RT_NOREF(pt);
    AssertPtrReturn(pdwEffect, E_INVALIDARG);

#ifdef DEBUG_andy
    LogFlowFunc(("cfFormat=%RI16, grfKeyState=0x%x, x=%ld, y=%ld\n",
                 mFormatEtc.cfFormat, grfKeyState, pt.x, pt.y));
#endif

    if (mFormatEtc.cfFormat)
    {
        /* Note: pt is not used since we don't need to differentiate within our
         *       proxy window. */
        *pdwEffect = VBoxDnDDropTarget::GetDropEffect(grfKeyState, *pdwEffect);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

#ifdef DEBUG_andy
    LogFlowFunc(("Returning *pdwEffect=%ld\n", *pdwEffect));
#endif
    return S_OK;
}

STDMETHODIMP VBoxDnDDropTarget::DragLeave(void)
{
#ifdef DEBUG_andy
    LogFlowFunc(("cfFormat=%RI16\n", mFormatEtc.cfFormat));
#endif

    if (mpWndParent)
        mpWndParent->hide();

    return S_OK;
}

STDMETHODIMP VBoxDnDDropTarget::Drop(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    RT_NOREF(pt);
    AssertPtrReturn(pDataObject, E_INVALIDARG);
    AssertPtrReturn(pdwEffect,   E_INVALIDARG);

    LogFlowFunc(("mFormatEtc.cfFormat=%RI16 (%s), pDataObject=0x%p, grfKeyState=0x%x, x=%ld, y=%ld\n",
                 mFormatEtc.cfFormat, VBoxDnDDataObject::ClipboardFormatToString(mFormatEtc.cfFormat),
                 pDataObject, grfKeyState, pt.x, pt.y));

    HRESULT hr = S_OK;

    if (mFormatEtc.cfFormat) /* Did we get a supported format yet? */
    {
        /* Make sure the data object's data format is still valid. */
        hr = pDataObject->QueryGetData(&mFormatEtc);
        AssertMsg(SUCCEEDED(hr),
                  ("Data format changed to invalid between DragEnter() and Drop(), cfFormat=%RI16 (%s), hr=%Rhrc\n",
                  mFormatEtc.cfFormat, VBoxDnDDataObject::ClipboardFormatToString(mFormatEtc.cfFormat), hr));
    }

    int rc = VINF_SUCCESS;

    if (SUCCEEDED(hr))
    {
        STGMEDIUM stgMed;
        hr = pDataObject->GetData(&mFormatEtc, &stgMed);
        if (SUCCEEDED(hr))
        {
            /*
             * First stage: Prepare the access to the storage medium.
             *              For now we only support HGLOBAL stuff.
             */
            PVOID pvData = NULL; /** @todo Put this in an own union? */

            switch (mFormatEtc.tymed)
            {
                case TYMED_HGLOBAL:
                    pvData = GlobalLock(stgMed.hGlobal);
                    if (!pvData)
                    {
                        LogFlowFunc(("Locking HGLOBAL storage failed with %Rrc\n",
                                     RTErrConvertFromWin32(GetLastError())));
                        rc = VERR_INVALID_HANDLE;
                        hr = E_INVALIDARG; /* Set special hr for OLE. */
                    }
                    break;

                default:
                    AssertMsgFailed(("Storage medium type %RI32 supported\n",
                                     mFormatEtc.tymed));
                    rc = VERR_NOT_SUPPORTED;
                    hr = DV_E_TYMED; /* Set special hr for OLE. */
                    break;
            }

            if (RT_SUCCESS(rc))
            {
                /*
                 * Second stage: Do the actual copying of the data object's data,
                 *               based on the storage medium type.
                 */
                switch (mFormatEtc.cfFormat)
                {
                    case CF_TEXT:
                    /* Fall through is intentional. */
                    case CF_UNICODETEXT:
                    {
                        AssertPtr(pvData);
                        size_t cbSize = GlobalSize(pvData);
                        LogFlowFunc(("CF_TEXT/CF_UNICODETEXT 0x%p got %zu bytes\n", pvData, cbSize));
                        if (cbSize)
                        {
                            char *pszText = NULL;

                            rc = mFormatEtc.cfFormat == CF_TEXT
                               /* ANSI codepage -> UTF-8 */
                               ? RTStrCurrentCPToUtf8(&pszText, (char *)pvData)
                               /* Unicode  -> UTF-8 */
                               : RTUtf16ToUtf8((PCRTUTF16)pvData, &pszText);

                            if (RT_SUCCESS(rc))
                            {
                                AssertPtr(pszText);

                                size_t cbText = strlen(pszText) + 1; /* Include termination. */

                                mpvData = RTMemDup((void *)pszText, cbText);
                                mcbData = cbText;

                                RTStrFree(pszText);
                                pszText = NULL;
                            }
                        }

                        break;
                    }

                    case CF_HDROP:
                    {
                        AssertPtr(pvData);

                        /* Convert to a string list, separated by \r\n. */
                        DROPFILES *pDropFiles = (DROPFILES *)pvData;
                        AssertPtr(pDropFiles);
                        bool fUnicode = RT_BOOL(pDropFiles->fWide);

                        /* Get the offset of the file list. */
                        Assert(pDropFiles->pFiles >= sizeof(DROPFILES));
                        /* Note: This is *not* pDropFiles->pFiles! DragQueryFile only
                         *       will work with the plain storage medium pointer! */
                        HDROP hDrop = (HDROP)(pvData);

                        /* First, get the file count. */
                        /** @todo Does this work on Windows 2000 / NT4? */
                        char *pszFiles = NULL;
                        uint32_t cchFiles = 0;
                        UINT cFiles = DragQueryFile(hDrop, UINT32_MAX /* iFile */,
                                                    NULL /* lpszFile */, 0 /* cchFile */);
                        LogFlowFunc(("CF_HDROP got %RU16 file(s)\n", cFiles));

                        for (UINT i = 0; i < cFiles; i++)
                        {
                            UINT cch = DragQueryFile(hDrop, i /* File index */,
                                                     NULL /* Query size first */,
                                                     0 /* cchFile */);
                            Assert(cch);

                            if (RT_FAILURE(rc))
                                break;

                            char *pszFile = NULL; /* UTF-8 version. */
                            UINT cchFile = 0;
                            if (fUnicode)
                            {
                                /* Allocate enough space (including terminator). */
                                WCHAR *pwszFile = (WCHAR *)RTMemAlloc((cch + 1) * sizeof(WCHAR));
                                if (pwszFile)
                                {
                                    cchFile = DragQueryFileW(hDrop, i /* File index */,
                                                             pwszFile, cch + 1 /* Include terminator */);
                                    AssertMsg(cchFile == cch, ("cchCopied (%RU16) does not match cchFile (%RU16)\n",
                                                               cchFile, cch));
                                    rc = RTUtf16ToUtf8(pwszFile, &pszFile);
                                    AssertRC(rc);

                                    RTMemFree(pwszFile);
                                }
                                else
                                    rc = VERR_NO_MEMORY;
                            }
                            else /* ANSI */
                            {
                                /* Allocate enough space (including terminator). */
                                pszFile = (char *)RTMemAlloc((cch + 1) * sizeof(char));
                                if (pszFile)
                                {
                                    cchFile = DragQueryFileA(hDrop, i /* File index */,
                                                             pszFile, cchFile + 1 /* Include terminator */);
                                    AssertMsg(cchFile == cch, ("cchCopied (%RU16) does not match cchFile (%RU16)\n",
                                                               cchFile, cch));
                                }
                                else
                                    rc = VERR_NO_MEMORY;
                            }

                            if (RT_SUCCESS(rc))
                            {
                                LogFlowFunc(("\tFile: %s (cchFile=%RU32)\n", pszFile, cchFile));
                                rc = RTStrAAppendExN(&pszFiles, 1 /* cPairs */,
                                                     pszFile, cchFile);
                                if (RT_SUCCESS(rc))
                                    cchFiles += cchFile;
                            }

                            if (pszFile)
                                RTStrFree(pszFile);

                            if (RT_FAILURE(rc))
                                break;

                            /* Add separation between filenames.
                             * Note: Also do this for the last element of the list. */
                            rc = RTStrAAppendExN(&pszFiles, 1 /* cPairs */,
                                                 "\r\n", 2 /* Bytes */);
                            if (RT_SUCCESS(rc))
                                cchFiles += 2; /* Include \r\n */
                        }

                        if (RT_SUCCESS(rc))
                        {
                            cchFiles += 1; /* Add string termination. */
                            uint32_t cbFiles = cchFiles * sizeof(char);

                            LogFlowFunc(("cFiles=%u, cchFiles=%RU32, cbFiles=%RU32, pszFiles=0x%p\n",
                                         cFiles, cchFiles, cbFiles, pszFiles));

                            /* Translate the list into URI elements. */
                            DnDURIList lstURI;
                            rc = lstURI.AppendNativePathsFromList(pszFiles, cbFiles,
                                                                  DNDURILIST_FLAGS_ABSOLUTE_PATHS);
                            if (RT_SUCCESS(rc))
                            {
                                RTCString strRoot = lstURI.RootToString();
                                size_t cbRoot = strRoot.length() + 1; /* Include termination */

                                mpvData = RTMemAlloc(cbRoot);
                                if (mpvData)
                                {
                                    memcpy(mpvData, strRoot.c_str(), cbRoot);
                                    mcbData = cbRoot;
                                }
                                else
                                    rc = VERR_NO_MEMORY;
                            }
                        }

                        LogFlowFunc(("Building CF_HDROP list rc=%Rrc, pszFiles=0x%p, cFiles=%RU16, cchFiles=%RU32\n",
                                     rc, pszFiles, cFiles, cchFiles));

                        if (pszFiles)
                            RTStrFree(pszFiles);
                        break;
                    }

                    default:
                        /* Note: Should not happen due to the checks done in DragEnter(). */
                        AssertMsgFailed(("Format of type %RI16 (%s) not supported\n",
                                         mFormatEtc.cfFormat, VBoxDnDDataObject::ClipboardFormatToString(mFormatEtc.cfFormat)));
                        hr = DV_E_CLIPFORMAT; /* Set special hr for OLE. */
                        break;
                }

                /*
                 * Third stage: Unlock + release access to the storage medium again.
                 */
                switch (mFormatEtc.tymed)
                {
                    case TYMED_HGLOBAL:
                        GlobalUnlock(stgMed.hGlobal);
                        break;

                    default:
                        AssertMsgFailed(("Really should not happen -- see init stage!\n"));
                        break;
                }
            }

            /* Release storage medium again. */
            ReleaseStgMedium(&stgMed);

            /* Signal waiters. */
            mDroppedRc = rc;
            RTSemEventSignal(hEventDrop);
        }
    }

    if (RT_SUCCESS(rc))
    {
        /* Note: pt is not used since we don't need to differentiate within our
         *       proxy window. */
        *pdwEffect = VBoxDnDDropTarget::GetDropEffect(grfKeyState, *pdwEffect);
    }
    else
        *pdwEffect = DROPEFFECT_NONE;

    if (mpWndParent)
        mpWndParent->hide();

    LogFlowFunc(("Returning with hr=%Rhrc (%Rrc), mFormatEtc.cfFormat=%RI16 (%s), *pdwEffect=%RI32\n",
                 hr, rc, mFormatEtc.cfFormat, VBoxDnDDataObject::ClipboardFormatToString(mFormatEtc.cfFormat),
                 *pdwEffect));

    return hr;
}

/* static */
DWORD VBoxDnDDropTarget::GetDropEffect(DWORD grfKeyState, DWORD dwAllowedEffects)
{
    DWORD dwEffect = DROPEFFECT_NONE;

    if(grfKeyState & MK_CONTROL)
        dwEffect = dwAllowedEffects & DROPEFFECT_COPY;
    else if(grfKeyState & MK_SHIFT)
        dwEffect = dwAllowedEffects & DROPEFFECT_MOVE;

    /* If there still was no drop effect assigned, check for the handed-in
     * allowed effects and assign one of them.
     *
     * Note: A move action has precendence over a copy action! */
    if (dwEffect == DROPEFFECT_NONE)
    {
        if (dwAllowedEffects & DROPEFFECT_COPY)
            dwEffect = DROPEFFECT_COPY;
        if (dwAllowedEffects & DROPEFFECT_MOVE)
            dwEffect = DROPEFFECT_MOVE;
    }

#ifdef DEBUG_andy
    LogFlowFunc(("grfKeyState=0x%x, dwAllowedEffects=0x%x, dwEffect=0x%x\n",
                 grfKeyState, dwAllowedEffects, dwEffect));
#endif
    return dwEffect;
}

void VBoxDnDDropTarget::reset(void)
{
    LogFlowFuncEnter();

    if (mpvData)
    {
        RTMemFree(mpvData);
        mpvData = NULL;
    }

    mcbData = 0;

    RT_ZERO(mFormatEtc);
    mstrFormats = "";
}

RTCString VBoxDnDDropTarget::Formats(void) const
{
    return mstrFormats;
}

int VBoxDnDDropTarget::WaitForDrop(RTMSINTERVAL msTimeout)
{
    LogFlowFunc(("msTimeout=%RU32\n", msTimeout));

    int rc = RTSemEventWait(hEventDrop, msTimeout);
    if (RT_SUCCESS(rc))
        rc = mDroppedRc;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

