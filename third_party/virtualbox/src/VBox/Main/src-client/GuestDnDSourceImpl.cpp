/* $Id: GuestDnDSourceImpl.cpp $ */
/** @file
 * VBox Console COM Class implementation - Guest drag and drop source.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_GUEST_DND //LOG_GROUP_MAIN_GUESTDNDSOURCE
#include "LoggingNew.h"

#include "GuestImpl.h"
#include "GuestDnDSourceImpl.h"
#include "GuestDnDPrivate.h"
#include "ConsoleImpl.h"

#include "Global.h"
#include "AutoCaller.h"
#include "ThreadTask.h"

#include <iprt/asm.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/path.h>
#include <iprt/uri.h>

#include <iprt/cpp/utils.h> /* For unconst(). */

#include <VBox/com/array.h>


/**
 * Base class for a source task.
 */
class GuestDnDSourceTask : public ThreadTask
{
public:

    GuestDnDSourceTask(GuestDnDSource *pSource)
        : ThreadTask("GenericGuestDnDSourceTask")
        , mSource(pSource)
        , mRC(VINF_SUCCESS) { }

    virtual ~GuestDnDSourceTask(void) { }

    int getRC(void) const { return mRC; }
    bool isOk(void) const { return RT_SUCCESS(mRC); }
    const ComObjPtr<GuestDnDSource> &getSource(void) const { return mSource; }

protected:

    const ComObjPtr<GuestDnDSource>     mSource;
    int                                 mRC;
};

/**
 * Task structure for receiving data from a source using
 * a worker thread.
 */
class RecvDataTask : public GuestDnDSourceTask
{
public:

    RecvDataTask(GuestDnDSource *pSource, PRECVDATACTX pCtx)
        : GuestDnDSourceTask(pSource)
        , mpCtx(pCtx)
    {
        m_strTaskName = "dndSrcRcvData";
    }

    void handler()
    {
        GuestDnDSource::i_receiveDataThreadTask(this);
    }

    virtual ~RecvDataTask(void) { }

    PRECVDATACTX getCtx(void) { return mpCtx; }

protected:

    /** Pointer to receive data context. */
    PRECVDATACTX mpCtx;
};

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

DEFINE_EMPTY_CTOR_DTOR(GuestDnDSource)

HRESULT GuestDnDSource::FinalConstruct(void)
{
    /*
     * Set the maximum block size this source can handle to 64K. This always has
     * been hardcoded until now.
     *
     * Note: Never ever rely on information from the guest; the host dictates what and
     *       how to do something, so try to negogiate a sensible value here later.
     */
    mData.mcbBlockSize = _64K; /** @todo Make this configurable. */

    LogFlowThisFunc(("\n"));
    return BaseFinalConstruct();
}

void GuestDnDSource::FinalRelease(void)
{
    LogFlowThisFuncEnter();
    uninit();
    BaseFinalRelease();
    LogFlowThisFuncLeave();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

int GuestDnDSource::init(const ComObjPtr<Guest>& pGuest)
{
    LogFlowThisFuncEnter();

    /* Enclose the state transition NotReady->InInit->Ready. */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    unconst(m_pGuest) = pGuest;

    /* Confirm a successful initialization when it's the case. */
    autoInitSpan.setSucceeded();

    return VINF_SUCCESS;
}

/**
 * Uninitializes the instance.
 * Called from FinalRelease().
 */
void GuestDnDSource::uninit(void)
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady. */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;
}

// implementation of wrapped IDnDBase methods.
/////////////////////////////////////////////////////////////////////////////

HRESULT GuestDnDSource::isFormatSupported(const com::Utf8Str &aFormat, BOOL *aSupported)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP) || !defined(VBOX_WITH_DRAG_AND_DROP_GH)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_isFormatSupported(aFormat, aSupported);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDSource::getFormats(GuestDnDMIMEList &aFormats)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP) || !defined(VBOX_WITH_DRAG_AND_DROP_GH)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_getFormats(aFormats);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDSource::addFormats(const GuestDnDMIMEList &aFormats)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP) || !defined(VBOX_WITH_DRAG_AND_DROP_GH)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_addFormats(aFormats);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDSource::removeFormats(const GuestDnDMIMEList &aFormats)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP) || !defined(VBOX_WITH_DRAG_AND_DROP_GH)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_removeFormats(aFormats);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDSource::getProtocolVersion(ULONG *aProtocolVersion)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_getProtocolVersion(aProtocolVersion);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

// implementation of wrapped IDnDSource methods.
/////////////////////////////////////////////////////////////////////////////

HRESULT GuestDnDSource::dragIsPending(ULONG uScreenId, GuestDnDMIMEList &aFormats,
                                      std::vector<DnDAction_T> &aAllowedActions, DnDAction_T *aDefaultAction)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP) || !defined(VBOX_WITH_DRAG_AND_DROP_GH)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    /* aDefaultAction is optional. */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* Determine guest DnD protocol to use. */
    GuestDnDBase::getProtocolVersion(&mDataBase.m_uProtocolVersion);

    /* Default is ignoring the action. */
    if (aDefaultAction)
        *aDefaultAction = DnDAction_Ignore;

    HRESULT hr = S_OK;

    GuestDnDMsg Msg;
    Msg.setType(HOST_DND_GH_REQ_PENDING);
    if (mDataBase.m_uProtocolVersion >= 3)
        Msg.setNextUInt32(0); /** @todo ContextID not used yet. */
    Msg.setNextUInt32(uScreenId);

    int rc = GuestDnDInst()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
    if (RT_SUCCESS(rc))
    {
        GuestDnDResponse *pResp = GuestDnDInst()->response();
        AssertPtr(pResp);

        bool fFetchResult = true;

        rc = pResp->waitForGuestResponse(100 /* Timeout in ms */);
        if (RT_FAILURE(rc))
            fFetchResult = false;

        if (   fFetchResult
            && isDnDIgnoreAction(pResp->defAction()))
            fFetchResult = false;

        /* Fetch the default action to use. */
        if (fFetchResult)
        {
            /*
             * In the GuestDnDSource case the source formats are from the guest,
             * as GuestDnDSource acts as a target for the guest. The host always
             * dictates what's supported and what's not, so filter out all formats
             * which are not supported by the host.
             */
            GuestDnDMIMEList lstFiltered  = GuestDnD::toFilteredFormatList(m_lstFmtSupported, pResp->formats());
            if (lstFiltered.size())
            {
                LogRel3(("DnD: Host offered the following formats:\n"));
                for (size_t i = 0; i < lstFiltered.size(); i++)
                    LogRel3(("DnD:\tFormat #%zu: %s\n", i, lstFiltered.at(i).c_str()));

                aFormats            = lstFiltered;
                aAllowedActions     = GuestDnD::toMainActions(pResp->allActions());
                if (aDefaultAction)
                    *aDefaultAction = GuestDnD::toMainAction(pResp->defAction());

                /* Apply the (filtered) formats list. */
                m_lstFmtOffered     = lstFiltered;
            }
            else
                LogRel2(("DnD: Negotiation of formats between guest and host failed, drag and drop to host not possible\n"));
        }

        LogFlowFunc(("fFetchResult=%RTbool, allActions=0x%x\n", fFetchResult, pResp->allActions()));
    }

    LogFlowFunc(("hr=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDSource::drop(const com::Utf8Str &aFormat, DnDAction_T aAction, ComPtr<IProgress> &aProgress)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP) || !defined(VBOX_WITH_DRAG_AND_DROP_GH)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    LogFunc(("aFormat=%s, aAction=%RU32\n", aFormat.c_str(), aAction));

    /* Input validation. */
    if (RT_UNLIKELY((aFormat.c_str()) == NULL || *(aFormat.c_str()) == '\0'))
        return setError(E_INVALIDARG, tr("No drop format specified"));

    /* Is the specified format in our list of (left over) offered formats? */
    if (!GuestDnD::isFormatInFormatList(aFormat, m_lstFmtOffered))
        return setError(E_INVALIDARG, tr("Specified format '%s' is not supported"), aFormat.c_str());

    uint32_t uAction = GuestDnD::toHGCMAction(aAction);
    if (isDnDIgnoreAction(uAction)) /* If there is no usable action, ignore this request. */
        return S_OK;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* At the moment we only support one transfer at a time. */
    if (mDataBase.m_cTransfersPending)
        return setError(E_INVALIDARG, tr("Another drop operation already is in progress"));

    /* Dito. */
    GuestDnDResponse *pResp = GuestDnDInst()->response();
    AssertPtr(pResp);

    HRESULT hr = pResp->resetProgress(m_pGuest);
    if (FAILED(hr))
        return hr;

    RecvDataTask *pTask = NULL;

    try
    {
        mData.mRecvCtx.mIsActive   = false;
        mData.mRecvCtx.mpSource    = this;
        mData.mRecvCtx.mpResp      = pResp;
        mData.mRecvCtx.mFmtReq     = aFormat;
        mData.mRecvCtx.mFmtOffered = m_lstFmtOffered;

        LogRel2(("DnD: Requesting data from guest in format: %s\n", aFormat.c_str()));

        pTask = new RecvDataTask(this, &mData.mRecvCtx);
        if (!pTask->isOk())
        {
            delete pTask;
            LogRel2(("DnD: Could not create RecvDataTask object \n"));
            throw hr = E_FAIL;
        }

        /* This function delete pTask in case of exceptions,
         * so there is no need in the call of delete operator. */
        hr = pTask->createThreadWithType(RTTHREADTYPE_MAIN_WORKER);

    }
    catch (std::bad_alloc &)
    {
        hr = setError(E_OUTOFMEMORY);
    }
    catch (...)
    {
        LogRel2(("DnD: Could not create thread for data receiving task\n"));
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        mDataBase.m_cTransfersPending++;

        hr = pResp->queryProgressTo(aProgress.asOutParam());
        ComAssertComRC(hr);

        /* Note: pTask is now owned by the worker thread. */
    }
    else
        hr = setError(VBOX_E_IPRT_ERROR, tr("Starting thread for GuestDnDSource::i_receiveDataThread failed (%Rhrc)"), hr);
    /* Note: mDataBase.mfTransferIsPending will be set to false again by i_receiveDataThread. */

    LogFlowFunc(("Returning hr=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDSource::receiveData(std::vector<BYTE> &aData)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP) || !defined(VBOX_WITH_DRAG_AND_DROP_GH)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowThisFunc(("cTransfersPending=%RU32\n", mDataBase.m_cTransfersPending));

    /* Don't allow receiving the actual data until our transfer actually is complete. */
    if (mDataBase.m_cTransfersPending)
        return setError(E_FAIL, tr("Current drop operation still in progress"));

    PRECVDATACTX pCtx = &mData.mRecvCtx;
    HRESULT hr = S_OK;

    try
    {
        bool fHasURIList = DnDMIMENeedsDropDir(pCtx->mFmtRecv.c_str(), pCtx->mFmtRecv.length());
        if (fHasURIList)
        {
            LogRel2(("DnD: Drop directory is: %s\n", pCtx->mURI.getDroppedFiles().GetDirAbs()));
            int rc2 = pCtx->mURI.toMetaData(aData);
            if (RT_FAILURE(rc2))
                hr = E_OUTOFMEMORY;
        }
        else
        {
            const size_t cbData = pCtx->mData.getMeta().getSize();
            LogFlowFunc(("cbData=%zu\n", cbData));
            if (cbData)
            {
                /* Copy the data into a safe array of bytes. */
                aData.resize(cbData);
                memcpy(&aData.front(), pCtx->mData.getMeta().getData(), cbData);
            }
            else
                aData.resize(0);
        }
    }
    catch (std::bad_alloc &)
    {
        hr = E_OUTOFMEMORY;
    }

    LogFlowFunc(("Returning hr=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

// implementation of internal methods.
/////////////////////////////////////////////////////////////////////////////

/* static */
Utf8Str GuestDnDSource::i_guestErrorToString(int guestRc)
{
    Utf8Str strError;

    switch (guestRc)
    {
        case VERR_ACCESS_DENIED:
            strError += Utf8StrFmt(tr("For one or more guest files or directories selected for transferring to the host your guest "
                                      "user does not have the appropriate access rights for. Please make sure that all selected "
                                      "elements can be accessed and that your guest user has the appropriate rights"));
            break;

        case VERR_NOT_FOUND:
            /* Should not happen due to file locking on the guest, but anyway ... */
            strError += Utf8StrFmt(tr("One or more guest files or directories selected for transferring to the host were not"
                                      "found on the guest anymore. This can be the case if the guest files were moved and/or"
                                      "altered while the drag and drop operation was in progress"));
            break;

        case VERR_SHARING_VIOLATION:
            strError += Utf8StrFmt(tr("One or more guest files or directories selected for transferring to the host were locked. "
                                      "Please make sure that all selected elements can be accessed and that your guest user has "
                                      "the appropriate rights"));
            break;

        case VERR_TIMEOUT:
            strError += Utf8StrFmt(tr("The guest was not able to retrieve the drag and drop data within time"));
            break;

        default:
            strError += Utf8StrFmt(tr("Drag and drop error from guest (%Rrc)"), guestRc);
            break;
    }

    return strError;
}

/* static */
Utf8Str GuestDnDSource::i_hostErrorToString(int hostRc)
{
    Utf8Str strError;

    switch (hostRc)
    {
        case VERR_ACCESS_DENIED:
            strError += Utf8StrFmt(tr("For one or more host files or directories selected for transferring to the guest your host "
                                      "user does not have the appropriate access rights for. Please make sure that all selected "
                                      "elements can be accessed and that your host user has the appropriate rights."));
            break;

        case VERR_DISK_FULL:
            strError += Utf8StrFmt(tr("Host disk ran out of space (disk is full)."));
            break;

        case VERR_NOT_FOUND:
            /* Should not happen due to file locking on the host, but anyway ... */
            strError += Utf8StrFmt(tr("One or more host files or directories selected for transferring to the host were not"
                                      "found on the host anymore. This can be the case if the host files were moved and/or"
                                      "altered while the drag and drop operation was in progress."));
            break;

        case VERR_SHARING_VIOLATION:
            strError += Utf8StrFmt(tr("One or more host files or directories selected for transferring to the guest were locked. "
                                      "Please make sure that all selected elements can be accessed and that your host user has "
                                      "the appropriate rights."));
            break;

        default:
            strError += Utf8StrFmt(tr("Drag and drop error from host (%Rrc)"), hostRc);
            break;
    }

    return strError;
}

#ifdef VBOX_WITH_DRAG_AND_DROP_GH
int GuestDnDSource::i_onReceiveDataHdr(PRECVDATACTX pCtx, PVBOXDNDSNDDATAHDR pDataHdr)
{
    AssertPtrReturn(pCtx,  VERR_INVALID_POINTER);
    AssertReturn(pDataHdr, VERR_INVALID_POINTER);

    pCtx->mData.setEstimatedSize(pDataHdr->cbTotal, pDataHdr->cbMeta);

    Assert(pCtx->mURI.getObjToProcess() == 0);
    pCtx->mURI.reset();
    pCtx->mURI.setEstimatedObjects(pDataHdr->cObjects);

    /** @todo Handle compression type. */
    /** @todo Handle checksum type. */

    LogFlowFuncLeave();
    return VINF_SUCCESS;
}

int GuestDnDSource::i_onReceiveData(PRECVDATACTX pCtx, PVBOXDNDSNDDATA pSndData)
{
    AssertPtrReturn(pCtx,     VERR_INVALID_POINTER);
    AssertPtrReturn(pSndData, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;

    try
    {
        GuestDnDData    *pData = &pCtx->mData;
        GuestDnDURIData *pURI  = &pCtx->mURI;

        uint32_t cbData;
        void    *pvData;
        uint64_t cbTotal;
        uint32_t cbMeta;

        if (mDataBase.m_uProtocolVersion < 3)
        {
            cbData  = pSndData->u.v1.cbData;
            pvData  = pSndData->u.v1.pvData;

            /* Sends the total data size to receive for every data chunk. */
            cbTotal = pSndData->u.v1.cbTotalSize;

            /* Meta data size always is cbData, meaning there cannot be an
             * extended data chunk transfer by sending further data. */
            cbMeta  = cbData;
        }
        else
        {
            cbData  = pSndData->u.v3.cbData;
            pvData  = pSndData->u.v3.pvData;

            /* Note: Data sizes get updated in i_onReceiveDataHdr(). */
            cbTotal = pData->getTotal();
            cbMeta  = pData->getMeta().getSize();
        }
        Assert(cbTotal);

        if (   cbData == 0
            || cbData >  cbTotal /* Paranoia */)
        {
            LogFlowFunc(("Incoming data size invalid: cbData=%RU32, cbToProcess=%RU64\n", cbData, pData->getTotal()));
            rc = VERR_INVALID_PARAMETER;
        }
        else if (cbTotal < cbMeta)
        {
            AssertMsgFailed(("cbTotal (%RU64) is smaller than cbMeta (%RU32)\n", cbTotal, cbMeta));
            rc = VERR_INVALID_PARAMETER;
        }

        if (RT_SUCCESS(rc))
        {
            cbMeta = pData->getMeta().add(pvData, cbData);
            LogFlowThisFunc(("cbMetaSize=%zu, cbData=%RU32, cbMeta=%RU32, cbTotal=%RU64\n",
                             pData->getMeta().getSize(), cbData, cbMeta, cbTotal));
        }

        if (RT_SUCCESS(rc))
        {
            /*
             * (Meta) Data transfer complete?
             */
            Assert(cbMeta <= pData->getMeta().getSize());
            if (cbMeta == pData->getMeta().getSize())
            {
                bool fHasURIList = DnDMIMENeedsDropDir(pCtx->mFmtRecv.c_str(), pCtx->mFmtRecv.length());
                LogFlowThisFunc(("fHasURIList=%RTbool\n", fHasURIList));
                if (fHasURIList)
                {
                    /* Try parsing the data as URI list. */
                    rc = pURI->fromRemoteMetaData(pData->getMeta());
                    if (RT_SUCCESS(rc))
                    {
                        if (mDataBase.m_uProtocolVersion < 3)
                            pData->setEstimatedSize(cbTotal, cbMeta);

                        /*
                         * Update our process with the data we already received.
                         * Note: The total size will consist of the meta data (in pVecData) and
                         *       the actual accumulated file/directory data from the guest.
                         */
                        rc = updateProgress(pData, pCtx->mpResp, (uint32_t)pData->getMeta().getSize());
                    }
                }
                else /* Raw data. */
                    rc = updateProgress(pData, pCtx->mpResp, cbData);
            }
        }
    }
    catch (std::bad_alloc &)
    {
        rc = VERR_NO_MEMORY;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDSource::i_onReceiveDir(PRECVDATACTX pCtx, const char *pszPath, uint32_t cbPath, uint32_t fMode)
{
    AssertPtrReturn(pCtx,    VERR_INVALID_POINTER);
    AssertPtrReturn(pszPath, VERR_INVALID_POINTER);
    AssertReturn(cbPath,     VERR_INVALID_PARAMETER);

    LogFlowFunc(("pszPath=%s, cbPath=%RU32, fMode=0x%x\n", pszPath, cbPath, fMode));

    /*
     * Sanity checking.
     */
    if (   !cbPath
        || cbPath > RTPATH_MAX)
    {
        LogFlowFunc(("Path length invalid, bailing out\n"));
        return VERR_INVALID_PARAMETER;
    }

    int rc = RTStrValidateEncodingEx(pszPath, RTSTR_MAX, 0);
    if (RT_FAILURE(rc))
    {
        LogFlowFunc(("Path validation failed with %Rrc, bailing out\n", rc));
        return VERR_INVALID_PARAMETER;
    }

    if (pCtx->mURI.isComplete())
    {
        LogFlowFunc(("Data transfer already complete, bailing out\n"));
        return VERR_INVALID_PARAMETER;
    }

    GuestDnDURIObjCtx &objCtx = pCtx->mURI.getObj(0); /** @todo Fill in context ID. */

    rc = objCtx.createIntermediate(DnDURIObject::Directory);
    if (RT_FAILURE(rc))
        return rc;

    DnDURIObject *pObj = objCtx.getObj();
    AssertPtr(pObj);

    const char *pszDroppedFilesDir = pCtx->mURI.getDroppedFiles().GetDirAbs();
    char *pszDir = RTPathJoinA(pszDroppedFilesDir, pszPath);
    if (pszDir)
    {
#ifdef RT_OS_WINDOWS
        RTPathChangeToDosSlashes(pszDir, true /* fForce */);
#else
        RTPathChangeToDosSlashes(pszDir, true /* fForce */);
#endif
        rc = RTDirCreateFullPath(pszDir, fMode);
        if (RT_SUCCESS(rc))
        {
            pCtx->mURI.processObject(*pObj);

            /* Add for having a proper rollback. */
            int rc2 = pCtx->mURI.getDroppedFiles().AddDir(pszDir);
            AssertRC(rc2);

            objCtx.reset();
            LogRel2(("DnD: Created guest directory on host: %s\n", pszDir));
        }
        else
            LogRel(("DnD: Error creating guest directory '%s' on host, rc=%Rrc\n", pszDir, rc));

        RTStrFree(pszDir);
    }
    else
         rc = VERR_NO_MEMORY;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDSource::i_onReceiveFileHdr(PRECVDATACTX pCtx, const char *pszPath, uint32_t cbPath,
                                       uint64_t cbSize, uint32_t fMode, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    AssertPtrReturn(pCtx,    VERR_INVALID_POINTER);
    AssertPtrReturn(pszPath, VERR_INVALID_POINTER);
    AssertReturn(cbPath,     VERR_INVALID_PARAMETER);
    AssertReturn(fMode,      VERR_INVALID_PARAMETER);
    /* fFlags are optional. */

    LogFlowFunc(("pszPath=%s, cbPath=%RU32, cbSize=%RU64, fMode=0x%x, fFlags=0x%x\n", pszPath, cbPath, cbSize, fMode, fFlags));

    /*
     * Sanity checking.
     */
    if (   !cbPath
        || cbPath > RTPATH_MAX)
    {
        return VERR_INVALID_PARAMETER;
    }

    if (!RTStrIsValidEncoding(pszPath))
        return VERR_INVALID_PARAMETER;

    if (cbSize > pCtx->mData.getTotal())
    {
        AssertMsgFailed(("File size (%RU64) exceeds total size to transfer (%RU64)\n", cbSize, pCtx->mData.getTotal()));
        return VERR_INVALID_PARAMETER;
    }

    if (pCtx->mURI.getObjToProcess() && pCtx->mURI.isComplete())
        return VERR_INVALID_PARAMETER;

    int rc = VINF_SUCCESS;

    do
    {
        GuestDnDURIObjCtx &objCtx = pCtx->mURI.getObj(0); /** @todo Fill in context ID. */
        DnDURIObject *pObj        = objCtx.getObj();

        /*
         * Sanity checking.
         */
        if (pObj)
        {
            if (    pObj->IsOpen()
                && !pObj->IsComplete())
            {
                AssertMsgFailed(("Object '%s' not complete yet\n", pObj->GetDestPath().c_str()));
                rc = VERR_WRONG_ORDER;
                break;
            }

            if (pObj->IsOpen()) /* File already opened? */
            {
                AssertMsgFailed(("Current opened object is '%s', close this first\n", pObj->GetDestPath().c_str()));
                rc = VERR_WRONG_ORDER;
                break;
            }
        }
        else
        {
            /*
             * Create new intermediate object to work with.
             */
            rc = objCtx.createIntermediate();
        }

        if (RT_SUCCESS(rc))
        {
            pObj = objCtx.getObj();
            AssertPtr(pObj);

            const char *pszDroppedFilesDir = pCtx->mURI.getDroppedFiles().GetDirAbs();
            AssertPtr(pszDroppedFilesDir);

            char pszPathAbs[RTPATH_MAX];
            rc = RTPathJoin(pszPathAbs, sizeof(pszPathAbs), pszDroppedFilesDir, pszPath);
            if (RT_FAILURE(rc))
            {
                LogFlowFunc(("Warning: Rebasing current file failed with rc=%Rrc\n", rc));
                break;
            }

            rc = DnDPathSanitize(pszPathAbs, sizeof(pszPathAbs));
            if (RT_FAILURE(rc))
            {
                LogFlowFunc(("Warning: Rebasing current file failed with rc=%Rrc\n", rc));
                break;
            }

            LogFunc(("Rebased to: %s\n", pszPathAbs));

            /** @todo Add sparse file support based on fFlags? (Use Open(..., fFlags | SPARSE). */
            rc = pObj->OpenEx(pszPathAbs, DnDURIObject::File, DnDURIObject::Target,
                              RTFILE_O_CREATE_REPLACE | RTFILE_O_WRITE | RTFILE_O_DENY_WRITE,
                              (fMode & RTFS_UNIX_MASK) | RTFS_UNIX_IRUSR | RTFS_UNIX_IWUSR);
            if (RT_SUCCESS(rc))
            {
                /* Add for having a proper rollback. */
                int rc2 = pCtx->mURI.getDroppedFiles().AddFile(pszPathAbs);
                AssertRC(rc2);
            }
        }

        if (RT_SUCCESS(rc))
        {
            /* Note: Protocol v1 does not send any file sizes, so always 0. */
            if (mDataBase.m_uProtocolVersion >= 2)
                rc = pObj->SetSize(cbSize);

            /** @todo Unescpae path before printing. */
            LogRel2(("DnD: Transferring guest file to host: %s (%RU64 bytes, mode 0x%x)\n",
                     pObj->GetDestPath().c_str(), pObj->GetSize(), pObj->GetMode()));

            /** @todo Set progress object title to current file being transferred? */

            if (!cbSize) /* 0-byte file? Close again. */
                pObj->Close();
        }

        if (RT_FAILURE(rc))
        {
            LogRel2(("DnD: Error opening/creating guest file '%s' on host, rc=%Rrc\n",
                     pObj->GetDestPath().c_str(), rc));
            break;
        }

    } while (0);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDSource::i_onReceiveFileData(PRECVDATACTX pCtx, const void *pvData, uint32_t cbData)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData, VERR_INVALID_PARAMETER);

    int rc = VINF_SUCCESS;

    LogFlowFunc(("pvData=%p, cbData=%RU32, cbBlockSize=%RU32\n", pvData, cbData, mData.mcbBlockSize));

    /*
     * Sanity checking.
     */
    if (cbData > mData.mcbBlockSize)
        return VERR_INVALID_PARAMETER;

    do
    {
        GuestDnDURIObjCtx &objCtx = pCtx->mURI.getObj(0); /** @todo Fill in context ID. */
        DnDURIObject *pObj        = objCtx.getObj();

        if (!pObj)
        {
            LogFlowFunc(("Warning: No current object set\n"));
            rc = VERR_WRONG_ORDER;
            break;
        }

        if (pObj->IsComplete())
        {
            LogFlowFunc(("Warning: Object '%s' already completed\n", pObj->GetDestPath().c_str()));
            rc = VERR_WRONG_ORDER;
            break;
        }

        if (!pObj->IsOpen()) /* File opened on host? */
        {
            LogFlowFunc(("Warning: Object '%s' not opened\n", pObj->GetDestPath().c_str()));
            rc = VERR_WRONG_ORDER;
            break;
        }

        uint32_t cbWritten;
        rc = pObj->Write(pvData, cbData, &cbWritten);
        if (RT_SUCCESS(rc))
        {
            Assert(cbWritten <= cbData);
            if (cbWritten < cbData)
            {
                /** @todo What to do when the host's disk is full? */
                rc = VERR_DISK_FULL;
            }

            if (RT_SUCCESS(rc))
                rc = updateProgress(&pCtx->mData, pCtx->mpResp, cbWritten);
        }
        else /* Something went wrong; close the object. */
            pObj->Close();

        if (RT_SUCCESS(rc))
        {
            if (pObj->IsComplete())
            {
                /** @todo Sanitize path. */
                LogRel2(("DnD: File transfer to host complete: %s\n", pObj->GetDestPath().c_str()));
                pCtx->mURI.processObject(*pObj);
                objCtx.reset();
            }
        }
        else
        {
            /** @todo What to do when the host's disk is full? */
            LogRel(("DnD: Error writing guest file to host to '%s': %Rrc\n", pObj->GetDestPath().c_str(), rc));
        }

    } while (0);

    LogFlowFuncLeaveRC(rc);
    return rc;
}
#endif /* VBOX_WITH_DRAG_AND_DROP_GH */

/**
 * @returns VBox status code that the caller ignores. Not sure if that's
 *          intentional or not.
 */
int GuestDnDSource::i_receiveData(PRECVDATACTX pCtx, RTMSINTERVAL msTimeout)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    /* Is this context already in receiving state? */
    if (ASMAtomicReadBool(&pCtx->mIsActive))
        return VERR_WRONG_ORDER;
    ASMAtomicWriteBool(&pCtx->mIsActive, true);

    GuestDnD *pInst = GuestDnDInst();
    if (!pInst)
        return VERR_INVALID_POINTER;

    GuestDnDResponse *pResp = pCtx->mpResp;
    AssertPtr(pCtx->mpResp);

    int rc = pCtx->mCBEvent.Reset();
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Reset any old data.
     */
    pCtx->mData.reset();
    pCtx->mURI.reset();
    pResp->reset();

    /*
     * Do we need to receive a different format than initially requested?
     *
     * For example, receiving a file link as "text/plain" requires still to receive
     * the file from the guest as "text/uri-list" first, then pointing to
     * the file path on the host in the "text/plain" data returned.
     */

    bool fFoundFormat = true; /* Whether we've found a common format between host + guest. */

    LogFlowFunc(("mFmtReq=%s, mFmtRecv=%s, mAction=0x%x\n",
                 pCtx->mFmtReq.c_str(), pCtx->mFmtRecv.c_str(), pCtx->mAction));

    /* Plain text wanted? */
    if (   pCtx->mFmtReq.equalsIgnoreCase("text/plain")
        || pCtx->mFmtReq.equalsIgnoreCase("text/plain;charset=utf-8"))
    {
        /* Did the guest offer a file? Receive a file instead. */
        if (GuestDnD::isFormatInFormatList("text/uri-list", pCtx->mFmtOffered))
            pCtx->mFmtRecv = "text/uri-list";
        /* Guest only offers (plain) text. */
        else
            pCtx->mFmtRecv = "text/plain;charset=utf-8";

        /** @todo Add more conversions here. */
    }
    /* File(s) wanted? */
    else if (pCtx->mFmtReq.equalsIgnoreCase("text/uri-list"))
    {
        /* Does the guest support sending files? */
        if (GuestDnD::isFormatInFormatList("text/uri-list", pCtx->mFmtOffered))
            pCtx->mFmtRecv = "text/uri-list";
        else /* Bail out. */
            fFoundFormat = false;
    }

    if (fFoundFormat)
    {
        Assert(!pCtx->mFmtReq.isEmpty());
        Assert(!pCtx->mFmtRecv.isEmpty());

        if (!pCtx->mFmtRecv.equals(pCtx->mFmtReq))
            LogRel3(("DnD: Requested data in format '%s', receiving in intermediate format '%s' now\n",
                     pCtx->mFmtReq.c_str(), pCtx->mFmtRecv.c_str()));

        /*
         * Call the appropriate receive handler based on the data format to handle.
         */
        bool fURIData = DnDMIMENeedsDropDir(pCtx->mFmtRecv.c_str(), pCtx->mFmtRecv.length());
        if (fURIData)
        {
            rc = i_receiveURIData(pCtx, msTimeout);
        }
        else
        {
            rc = i_receiveRawData(pCtx, msTimeout);
        }
    }
    else /* Just inform the user (if verbose release logging is enabled). */
    {
        LogRel2(("DnD: The guest does not support format '%s':\n", pCtx->mFmtReq.c_str()));
        LogRel2(("DnD: Guest offered the following formats:\n"));
        for (size_t i = 0; i < pCtx->mFmtOffered.size(); i++)
            LogRel2(("DnD:\tFormat #%zu: %s\n", i, pCtx->mFmtOffered.at(i).c_str()));
    }

    ASMAtomicWriteBool(&pCtx->mIsActive, false);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/* static */
void GuestDnDSource::i_receiveDataThreadTask(RecvDataTask *pTask)
{
    LogFlowFunc(("pTask=%p\n", pTask));
    AssertPtrReturnVoid(pTask);

    const ComObjPtr<GuestDnDSource> pThis(pTask->getSource());
    Assert(!pThis.isNull());

    AutoCaller autoCaller(pThis);
    if (FAILED(autoCaller.rc()))
        return;

    int vrc = pThis->i_receiveData(pTask->getCtx(), RT_INDEFINITE_WAIT /* msTimeout */);
    AssertRC(vrc);
/** @todo
 *
 *  r=bird: What happens with @a vrc?
 *
 */

    AutoWriteLock alock(pThis COMMA_LOCKVAL_SRC_POS);

    Assert(pThis->mDataBase.m_cTransfersPending);
    if (pThis->mDataBase.m_cTransfersPending)
        pThis->mDataBase.m_cTransfersPending--;

    LogFlowFunc(("pSource=%p vrc=%Rrc (ignored)\n", (GuestDnDSource *)pThis, vrc));
}

int GuestDnDSource::i_receiveRawData(PRECVDATACTX pCtx, RTMSINTERVAL msTimeout)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    int rc;

    LogFlowFuncEnter();

    GuestDnDResponse *pResp = pCtx->mpResp;
    AssertPtr(pCtx->mpResp);

    GuestDnD *pInst = GuestDnDInst();
    if (!pInst)
        return VERR_INVALID_POINTER;

#define REGISTER_CALLBACK(x) \
    do {                                                            \
        rc = pResp->setCallback(x, i_receiveRawDataCallback, pCtx); \
        if (RT_FAILURE(rc))                                         \
            return rc;                                              \
    } while (0)

#define UNREGISTER_CALLBACK(x)                                      \
    do {                                                            \
        int rc2 = pResp->setCallback(x, NULL);                      \
        AssertRC(rc2);                                              \
    } while (0)

    /*
     * Register callbacks.
     */
    REGISTER_CALLBACK(GUEST_DND_CONNECT);
    REGISTER_CALLBACK(GUEST_DND_DISCONNECT);
    REGISTER_CALLBACK(GUEST_DND_GH_EVT_ERROR);
    if (mDataBase.m_uProtocolVersion >= 3)
        REGISTER_CALLBACK(GUEST_DND_GH_SND_DATA_HDR);
    REGISTER_CALLBACK(GUEST_DND_GH_SND_DATA);

    do
    {
        /*
         * Receive the raw data.
         */
        GuestDnDMsg Msg;
        Msg.setType(HOST_DND_GH_EVT_DROPPED);
        if (mDataBase.m_uProtocolVersion >= 3)
            Msg.setNextUInt32(0); /** @todo ContextID not used yet. */
        Msg.setNextPointer((void*)pCtx->mFmtRecv.c_str(), (uint32_t)pCtx->mFmtRecv.length() + 1);
        Msg.setNextUInt32((uint32_t)pCtx->mFmtRecv.length() + 1);
        Msg.setNextUInt32(pCtx->mAction);

        /* Make the initial call to the guest by telling that we initiated the "dropped" event on
         * the host and therefore now waiting for the actual raw data. */
        rc = pInst->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
        if (RT_SUCCESS(rc))
        {
            rc = waitForEvent(&pCtx->mCBEvent, pCtx->mpResp, msTimeout);
            if (RT_SUCCESS(rc))
                rc = pCtx->mpResp->setProgress(100, DND_PROGRESS_COMPLETE, VINF_SUCCESS);
        }

    } while (0);

    /*
     * Unregister callbacks.
     */
    UNREGISTER_CALLBACK(GUEST_DND_CONNECT);
    UNREGISTER_CALLBACK(GUEST_DND_DISCONNECT);
    UNREGISTER_CALLBACK(GUEST_DND_GH_EVT_ERROR);
    if (mDataBase.m_uProtocolVersion >= 3)
        UNREGISTER_CALLBACK(GUEST_DND_GH_SND_DATA_HDR);
    UNREGISTER_CALLBACK(GUEST_DND_GH_SND_DATA);

#undef REGISTER_CALLBACK
#undef UNREGISTER_CALLBACK

    if (RT_FAILURE(rc))
    {
        if (rc == VERR_CANCELLED)
        {
            int rc2 = pCtx->mpResp->setProgress(100, DND_PROGRESS_CANCELLED);
            AssertRC(rc2);

            rc2 = sendCancel();
            AssertRC(rc2);
        }
        else if (rc != VERR_GSTDND_GUEST_ERROR) /* Guest-side error are already handled in the callback. */
        {
            int rc2 = pCtx->mpResp->setProgress(100, DND_PROGRESS_ERROR,
                                                rc, GuestDnDSource::i_hostErrorToString(rc));
            AssertRC(rc2);
        }
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDSource::i_receiveURIData(PRECVDATACTX pCtx, RTMSINTERVAL msTimeout)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    int rc;

    LogFlowFuncEnter();

    GuestDnDResponse *pResp = pCtx->mpResp;
    AssertPtr(pCtx->mpResp);

    GuestDnD *pInst = GuestDnDInst();
    if (!pInst)
        return VERR_INVALID_POINTER;

#define REGISTER_CALLBACK(x)                                        \
    do {                                                            \
        rc = pResp->setCallback(x, i_receiveURIDataCallback, pCtx); \
        if (RT_FAILURE(rc))                                         \
            return rc;                                              \
    } while (0)

#define UNREGISTER_CALLBACK(x)                                      \
    do {                                                            \
        int rc2 = pResp->setCallback(x, NULL);                      \
        AssertRC(rc2);                                              \
    } while (0)

    /*
     * Register callbacks.
     */
    /* Guest callbacks. */
    REGISTER_CALLBACK(GUEST_DND_CONNECT);
    REGISTER_CALLBACK(GUEST_DND_DISCONNECT);
    REGISTER_CALLBACK(GUEST_DND_GH_EVT_ERROR);
    if (mDataBase.m_uProtocolVersion >= 3)
        REGISTER_CALLBACK(GUEST_DND_GH_SND_DATA_HDR);
    REGISTER_CALLBACK(GUEST_DND_GH_SND_DATA);
    REGISTER_CALLBACK(GUEST_DND_GH_SND_DIR);
    if (mDataBase.m_uProtocolVersion >= 2)
        REGISTER_CALLBACK(GUEST_DND_GH_SND_FILE_HDR);
    REGISTER_CALLBACK(GUEST_DND_GH_SND_FILE_DATA);

    DnDDroppedFiles &droppedFiles = pCtx->mURI.getDroppedFiles();

    do
    {
        rc = droppedFiles.OpenTemp(0 /* fFlags */);
        if (RT_FAILURE(rc))
            break;
        LogFlowFunc(("rc=%Rrc, strDropDir=%s\n", rc, droppedFiles.GetDirAbs()));
        if (RT_FAILURE(rc))
            break;

        /*
         * Receive the URI list.
         */
        GuestDnDMsg Msg;
        Msg.setType(HOST_DND_GH_EVT_DROPPED);
        if (mDataBase.m_uProtocolVersion >= 3)
            Msg.setNextUInt32(0); /** @todo ContextID not used yet. */
        Msg.setNextPointer((void*)pCtx->mFmtRecv.c_str(), (uint32_t)pCtx->mFmtRecv.length() + 1);
        Msg.setNextUInt32((uint32_t)pCtx->mFmtRecv.length() + 1);
        Msg.setNextUInt32(pCtx->mAction);

        /* Make the initial call to the guest by telling that we initiated the "dropped" event on
         * the host and therefore now waiting for the actual URI data. */
        rc = pInst->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
        if (RT_SUCCESS(rc))
        {
            LogFlowFunc(("Waiting ...\n"));

            rc = waitForEvent(&pCtx->mCBEvent, pCtx->mpResp, msTimeout);
            if (RT_SUCCESS(rc))
                rc = pCtx->mpResp->setProgress(100, DND_PROGRESS_COMPLETE, VINF_SUCCESS);

            LogFlowFunc(("Waiting ended with rc=%Rrc\n", rc));
        }

    } while (0);

    /*
     * Unregister callbacks.
     */
    UNREGISTER_CALLBACK(GUEST_DND_CONNECT);
    UNREGISTER_CALLBACK(GUEST_DND_DISCONNECT);
    UNREGISTER_CALLBACK(GUEST_DND_GH_EVT_ERROR);
    UNREGISTER_CALLBACK(GUEST_DND_GH_SND_DATA_HDR);
    UNREGISTER_CALLBACK(GUEST_DND_GH_SND_DATA);
    UNREGISTER_CALLBACK(GUEST_DND_GH_SND_DIR);
    UNREGISTER_CALLBACK(GUEST_DND_GH_SND_FILE_HDR);
    UNREGISTER_CALLBACK(GUEST_DND_GH_SND_FILE_DATA);

#undef REGISTER_CALLBACK
#undef UNREGISTER_CALLBACK

    if (RT_FAILURE(rc))
    {
        if (rc == VERR_CANCELLED)
        {
            int rc2 = pCtx->mpResp->setProgress(100, DND_PROGRESS_CANCELLED);
            AssertRC(rc2);

            rc2 = sendCancel();
            AssertRC(rc2);
        }
        else if (rc != VERR_GSTDND_GUEST_ERROR) /* Guest-side error are already handled in the callback. */
        {
            int rc2 = pCtx->mpResp->setProgress(100, DND_PROGRESS_ERROR,
                                                rc, GuestDnDSource::i_hostErrorToString(rc));
            AssertRC(rc2);
        }
    }

    if (RT_FAILURE(rc))
    {
        int rc2 = droppedFiles.Rollback();
        if (RT_FAILURE(rc2))
            LogRel(("DnD: Deleting left over temporary files failed (%Rrc). Please remove directory manually: %s\n",
                    rc2, droppedFiles.GetDirAbs()));
    }

    droppedFiles.Close();

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/* static */
DECLCALLBACK(int) GuestDnDSource::i_receiveRawDataCallback(uint32_t uMsg, void *pvParms, size_t cbParms, void *pvUser)
{
    PRECVDATACTX pCtx = (PRECVDATACTX)pvUser;
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    GuestDnDSource *pThis = pCtx->mpSource;
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    LogFlowFunc(("pThis=%p, uMsg=%RU32\n", pThis, uMsg));

    int rc = VINF_SUCCESS;

    int rcCallback = VINF_SUCCESS; /* rc for the callback. */
    bool fNotify   = false;

    switch (uMsg)
    {
        case GUEST_DND_CONNECT:
            /* Nothing to do here (yet). */
            break;

        case GUEST_DND_DISCONNECT:
            rc = VERR_CANCELLED;
            break;

#ifdef VBOX_WITH_DRAG_AND_DROP_GH
        case GUEST_DND_GH_SND_DATA_HDR:
        {
            PVBOXDNDCBSNDDATAHDRDATA pCBData = reinterpret_cast<PVBOXDNDCBSNDDATAHDRDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBSNDDATAHDRDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(CB_MAGIC_DND_GH_SND_DATA_HDR == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            rc = pThis->i_onReceiveDataHdr(pCtx, &pCBData->data);
            break;
        }
        case GUEST_DND_GH_SND_DATA:
        {
            PVBOXDNDCBSNDDATADATA pCBData = reinterpret_cast<PVBOXDNDCBSNDDATADATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBSNDDATADATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(CB_MAGIC_DND_GH_SND_DATA == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            rc = pThis->i_onReceiveData(pCtx, &pCBData->data);
            break;
        }
        case GUEST_DND_GH_EVT_ERROR:
        {
            PVBOXDNDCBEVTERRORDATA pCBData = reinterpret_cast<PVBOXDNDCBEVTERRORDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBEVTERRORDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(CB_MAGIC_DND_GH_EVT_ERROR == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            pCtx->mpResp->reset();

            if (RT_SUCCESS(pCBData->rc))
            {
                AssertMsgFailed(("Received guest error with no error code set\n"));
                pCBData->rc = VERR_GENERAL_FAILURE; /* Make sure some error is set. */
            }
            else if (pCBData->rc == VERR_WRONG_ORDER)
            {
                rc = pCtx->mpResp->setProgress(100, DND_PROGRESS_CANCELLED);
            }
            else
                rc = pCtx->mpResp->setProgress(100, DND_PROGRESS_ERROR, pCBData->rc,
                                               GuestDnDSource::i_guestErrorToString(pCBData->rc));

            LogRel3(("DnD: Guest reported data transfer error: %Rrc\n", pCBData->rc));

            if (RT_SUCCESS(rc))
                rcCallback = VERR_GSTDND_GUEST_ERROR;
            break;
        }
#endif /* VBOX_WITH_DRAG_AND_DROP_GH */
        default:
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    if (   RT_FAILURE(rc)
        || RT_FAILURE(rcCallback))
    {
        fNotify = true;
        if (RT_SUCCESS(rcCallback))
            rcCallback = rc;
    }

    if (RT_FAILURE(rc))
    {
        switch (rc)
        {
            case VERR_NO_DATA:
                LogRel2(("DnD: Data transfer to host complete\n"));
                break;

            case VERR_CANCELLED:
                LogRel2(("DnD: Data transfer to host canceled\n"));
                break;

            default:
                LogRel(("DnD: Error %Rrc occurred, aborting data transfer to host\n", rc));
                break;
        }

        /* Unregister this callback. */
        AssertPtr(pCtx->mpResp);
        int rc2 = pCtx->mpResp->setCallback(uMsg, NULL /* PFNGUESTDNDCALLBACK */);
        AssertRC(rc2);
    }

    /* All data processed? */
    if (pCtx->mData.isComplete())
        fNotify = true;

    LogFlowFunc(("cbProcessed=%RU64, cbToProcess=%RU64, fNotify=%RTbool, rcCallback=%Rrc, rc=%Rrc\n",
                 pCtx->mData.getProcessed(), pCtx->mData.getTotal(), fNotify, rcCallback, rc));

    if (fNotify)
    {
        int rc2 = pCtx->mCBEvent.Notify(rcCallback);
        AssertRC(rc2);
    }

    LogFlowFuncLeaveRC(rc);
    return rc; /* Tell the guest. */
}

/* static */
DECLCALLBACK(int) GuestDnDSource::i_receiveURIDataCallback(uint32_t uMsg, void *pvParms, size_t cbParms, void *pvUser)
{
    PRECVDATACTX pCtx = (PRECVDATACTX)pvUser;
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    GuestDnDSource *pThis = pCtx->mpSource;
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    LogFlowFunc(("pThis=%p, uMsg=%RU32\n", pThis, uMsg));

    int rc = VINF_SUCCESS;

    int rcCallback = VINF_SUCCESS; /* rc for the callback. */
    bool fNotify = false;

    switch (uMsg)
    {
        case GUEST_DND_CONNECT:
            /* Nothing to do here (yet). */
            break;

        case GUEST_DND_DISCONNECT:
            rc = VERR_CANCELLED;
            break;

#ifdef VBOX_WITH_DRAG_AND_DROP_GH
        case GUEST_DND_GH_SND_DATA_HDR:
        {
            PVBOXDNDCBSNDDATAHDRDATA pCBData = reinterpret_cast<PVBOXDNDCBSNDDATAHDRDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBSNDDATAHDRDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(CB_MAGIC_DND_GH_SND_DATA_HDR == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            rc = pThis->i_onReceiveDataHdr(pCtx, &pCBData->data);
            break;
        }
        case GUEST_DND_GH_SND_DATA:
        {
            PVBOXDNDCBSNDDATADATA pCBData = reinterpret_cast<PVBOXDNDCBSNDDATADATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBSNDDATADATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(CB_MAGIC_DND_GH_SND_DATA == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            rc = pThis->i_onReceiveData(pCtx, &pCBData->data);
            break;
        }
        case GUEST_DND_GH_SND_DIR:
        {
            PVBOXDNDCBSNDDIRDATA pCBData = reinterpret_cast<PVBOXDNDCBSNDDIRDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBSNDDIRDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(CB_MAGIC_DND_GH_SND_DIR == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            rc = pThis->i_onReceiveDir(pCtx, pCBData->pszPath, pCBData->cbPath, pCBData->fMode);
            break;
        }
        case GUEST_DND_GH_SND_FILE_HDR:
        {
            PVBOXDNDCBSNDFILEHDRDATA pCBData = reinterpret_cast<PVBOXDNDCBSNDFILEHDRDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBSNDFILEHDRDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(CB_MAGIC_DND_GH_SND_FILE_HDR == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            rc = pThis->i_onReceiveFileHdr(pCtx, pCBData->pszFilePath, pCBData->cbFilePath,
                                           pCBData->cbSize, pCBData->fMode, pCBData->fFlags);
            break;
        }
        case GUEST_DND_GH_SND_FILE_DATA:
        {
            PVBOXDNDCBSNDFILEDATADATA pCBData = reinterpret_cast<PVBOXDNDCBSNDFILEDATADATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBSNDFILEDATADATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(CB_MAGIC_DND_GH_SND_FILE_DATA == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            if (pThis->mDataBase.m_uProtocolVersion <= 1)
            {
                /**
                 * Notes for protocol v1 (< VBox 5.0):
                 * - Every time this command is being sent it includes the file header,
                 *   so just process both calls here.
                 * - There was no information whatsoever about the total file size; the old code only
                 *   appended data to the desired file. So just pass 0 as cbSize.
                 */
                rc = pThis->i_onReceiveFileHdr(pCtx, pCBData->u.v1.pszFilePath, pCBData->u.v1.cbFilePath,
                                               0 /* cbSize */, pCBData->u.v1.fMode, 0 /* fFlags */);
                if (RT_SUCCESS(rc))
                    rc = pThis->i_onReceiveFileData(pCtx, pCBData->pvData, pCBData->cbData);
            }
            else /* Protocol v2 and up. */
                rc = pThis->i_onReceiveFileData(pCtx, pCBData->pvData, pCBData->cbData);
            break;
        }
        case GUEST_DND_GH_EVT_ERROR:
        {
            PVBOXDNDCBEVTERRORDATA pCBData = reinterpret_cast<PVBOXDNDCBEVTERRORDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBEVTERRORDATA) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(CB_MAGIC_DND_GH_EVT_ERROR == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            pCtx->mpResp->reset();

            if (RT_SUCCESS(pCBData->rc))
            {
                AssertMsgFailed(("Received guest error with no error code set\n"));
                pCBData->rc = VERR_GENERAL_FAILURE; /* Make sure some error is set. */
            }
            else if (pCBData->rc == VERR_WRONG_ORDER)
            {
                rc = pCtx->mpResp->setProgress(100, DND_PROGRESS_CANCELLED);
            }
            else
                rc = pCtx->mpResp->setProgress(100, DND_PROGRESS_ERROR, pCBData->rc,
                                               GuestDnDSource::i_guestErrorToString(pCBData->rc));

            LogRel3(("DnD: Guest reported file transfer error: %Rrc\n", pCBData->rc));

            if (RT_SUCCESS(rc))
                rcCallback = VERR_GSTDND_GUEST_ERROR;
            break;
        }
#endif /* VBOX_WITH_DRAG_AND_DROP_GH */
        default:
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    if (   RT_FAILURE(rc)
        || RT_FAILURE(rcCallback))
    {
        fNotify = true;
        if (RT_SUCCESS(rcCallback))
            rcCallback = rc;
    }

    if (RT_FAILURE(rc))
    {
        switch (rc)
        {
            case VERR_NO_DATA:
                LogRel2(("DnD: File transfer to host complete\n"));
                break;

            case VERR_CANCELLED:
                LogRel2(("DnD: File transfer to host canceled\n"));
                break;

            default:
                LogRel(("DnD: Error %Rrc occurred, aborting file transfer to host\n", rc));
                break;
        }

        /* Unregister this callback. */
        AssertPtr(pCtx->mpResp);
        int rc2 = pCtx->mpResp->setCallback(uMsg, NULL /* PFNGUESTDNDCALLBACK */);
        AssertRC(rc2);
    }

    /* All data processed? */
    if (   pCtx->mURI.isComplete()
        && pCtx->mData.isComplete())
    {
        fNotify = true;
    }

    LogFlowFunc(("cbProcessed=%RU64, cbToProcess=%RU64, fNotify=%RTbool, rcCallback=%Rrc, rc=%Rrc\n",
                 pCtx->mData.getProcessed(), pCtx->mData.getTotal(), fNotify, rcCallback, rc));

    if (fNotify)
    {
        int rc2 = pCtx->mCBEvent.Notify(rcCallback);
        AssertRC(rc2);
    }

    LogFlowFuncLeaveRC(rc);
    return rc; /* Tell the guest. */
}

