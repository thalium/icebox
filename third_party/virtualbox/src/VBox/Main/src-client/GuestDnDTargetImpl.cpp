/* $Id: GuestDnDTargetImpl.cpp $ */
/** @file
 * VBox Console COM Class implementation - Guest drag'n drop target.
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
#define LOG_GROUP LOG_GROUP_GUEST_DND //LOG_GROUP_MAIN_GUESTDNDTARGET
#include "LoggingNew.h"

#include "GuestImpl.h"
#include "GuestDnDTargetImpl.h"
#include "ConsoleImpl.h"

#include "Global.h"
#include "AutoCaller.h"
#include "ThreadTask.h"

#include <algorithm>        /* For std::find(). */

#include <iprt/asm.h>
#include <iprt/file.h>
#include <iprt/dir.h>
#include <iprt/path.h>
#include <iprt/uri.h>
#include <iprt/cpp/utils.h> /* For unconst(). */

#include <VBox/com/array.h>

#include <VBox/GuestHost/DragAndDrop.h>
#include <VBox/HostServices/Service.h>


/**
 * Base class for a target task.
 */
class GuestDnDTargetTask : public ThreadTask
{
public:

    GuestDnDTargetTask(GuestDnDTarget *pTarget)
        : ThreadTask("GenericGuestDnDTargetTask")
        , mTarget(pTarget)
        , mRC(VINF_SUCCESS) { }

    virtual ~GuestDnDTargetTask(void) { }

    int getRC(void) const { return mRC; }
    bool isOk(void) const { return RT_SUCCESS(mRC); }
    const ComObjPtr<GuestDnDTarget> &getTarget(void) const { return mTarget; }

protected:

    const ComObjPtr<GuestDnDTarget>     mTarget;
    int                                 mRC;
};

/**
 * Task structure for sending data to a target using
 * a worker thread.
 */
class SendDataTask : public GuestDnDTargetTask
{
public:

    SendDataTask(GuestDnDTarget *pTarget, PSENDDATACTX pCtx)
        : GuestDnDTargetTask(pTarget),
          mpCtx(pCtx)
    {
        m_strTaskName = "dndTgtSndData";
    }

    void handler()
    {
        GuestDnDTarget::i_sendDataThreadTask(this);
    }

    virtual ~SendDataTask(void)
    {
        if (mpCtx)
        {
            delete mpCtx;
            mpCtx = NULL;
        }
    }


    PSENDDATACTX getCtx(void) { return mpCtx; }

protected:

    /** Pointer to send data context. */
    PSENDDATACTX mpCtx;
};

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

DEFINE_EMPTY_CTOR_DTOR(GuestDnDTarget)

HRESULT GuestDnDTarget::FinalConstruct(void)
{
    /* Set the maximum block size our guests can handle to 64K. This always has
     * been hardcoded until now. */
    /* Note: Never ever rely on information from the guest; the host dictates what and
     *       how to do something, so try to negogiate a sensible value here later. */
    mData.mcbBlockSize = _64K; /** @todo Make this configurable. */

    LogFlowThisFunc(("\n"));
    return BaseFinalConstruct();
}

void GuestDnDTarget::FinalRelease(void)
{
    LogFlowThisFuncEnter();
    uninit();
    BaseFinalRelease();
    LogFlowThisFuncLeave();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

int GuestDnDTarget::init(const ComObjPtr<Guest>& pGuest)
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
void GuestDnDTarget::uninit(void)
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady. */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;
}

// implementation of wrapped IDnDBase methods.
/////////////////////////////////////////////////////////////////////////////

HRESULT GuestDnDTarget::isFormatSupported(const com::Utf8Str &aFormat, BOOL *aSupported)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_isFormatSupported(aFormat, aSupported);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::getFormats(GuestDnDMIMEList &aFormats)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_getFormats(aFormats);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::addFormats(const GuestDnDMIMEList &aFormats)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_addFormats(aFormats);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::removeFormats(const GuestDnDMIMEList &aFormats)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_removeFormats(aFormats);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::getProtocolVersion(ULONG *aProtocolVersion)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    return GuestDnDBase::i_getProtocolVersion(aProtocolVersion);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

// implementation of wrapped IDnDTarget methods.
/////////////////////////////////////////////////////////////////////////////

HRESULT GuestDnDTarget::enter(ULONG aScreenId, ULONG aX, ULONG aY,
                              DnDAction_T                      aDefaultAction,
                              const std::vector<DnDAction_T>  &aAllowedActions,
                              const GuestDnDMIMEList          &aFormats,
                              DnDAction_T                     *aResultAction)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    /* Input validation. */
    if (aDefaultAction == DnDAction_Ignore)
        return setError(E_INVALIDARG, tr("No default action specified"));
    if (!aAllowedActions.size())
        return setError(E_INVALIDARG, tr("Number of allowed actions is empty"));
    if (!aFormats.size())
        return setError(E_INVALIDARG, tr("Number of supported formats is empty"));

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* Determine guest DnD protocol to use. */
    GuestDnDBase::getProtocolVersion(&mDataBase.m_uProtocolVersion);

    /* Default action is ignoring. */
    DnDAction_T resAction = DnDAction_Ignore;

    /* Check & convert the drag & drop actions */
    uint32_t uDefAction      = 0;
    uint32_t uAllowedActions = 0;
    GuestDnD::toHGCMActions(aDefaultAction, &uDefAction,
                            aAllowedActions, &uAllowedActions);
    /* If there is no usable action, ignore this request. */
    if (isDnDIgnoreAction(uDefAction))
        return S_OK;

    /*
     * Make a flat data string out of the supported format list.
     * In the GuestDnDTarget case the source formats are from the host,
     * as GuestDnDTarget acts as a source for the guest.
     */
    Utf8Str strFormats       = GuestDnD::toFormatString(GuestDnD::toFilteredFormatList(m_lstFmtSupported, aFormats));
    if (strFormats.isEmpty())
        return setError(E_INVALIDARG, tr("No or not supported format(s) specified"));
    const uint32_t cbFormats = (uint32_t)strFormats.length() + 1; /* Include terminating zero. */

    LogRel2(("DnD: Offered formats to guest:\n"));
    RTCList<RTCString> lstFormats = strFormats.split("\r\n");
    for (size_t i = 0; i < lstFormats.size(); i++)
        LogRel2(("DnD: \t%s\n", lstFormats[i].c_str()));

    /* Save the formats offered to the guest. This is needed to later
     * decide what to do with the data when sending stuff to the guest. */
    m_lstFmtOffered = aFormats;
    Assert(m_lstFmtOffered.size());

    HRESULT hr = S_OK;

    /* Adjust the coordinates in a multi-monitor setup. */
    int rc = GuestDnDInst()->adjustScreenCoordinates(aScreenId, &aX, &aY);
    if (RT_SUCCESS(rc))
    {
        GuestDnDMsg Msg;
        Msg.setType(HOST_DND_HG_EVT_ENTER);
        if (mDataBase.m_uProtocolVersion >= 3)
            Msg.setNextUInt32(0); /** @todo ContextID not used yet. */
        Msg.setNextUInt32(aScreenId);
        Msg.setNextUInt32(aX);
        Msg.setNextUInt32(aY);
        Msg.setNextUInt32(uDefAction);
        Msg.setNextUInt32(uAllowedActions);
        Msg.setNextPointer((void *)strFormats.c_str(), cbFormats);
        Msg.setNextUInt32(cbFormats);

        rc = GuestDnDInst()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
        if (RT_SUCCESS(rc))
        {
            GuestDnDResponse *pResp = GuestDnDInst()->response();
            if (pResp && RT_SUCCESS(pResp->waitForGuestResponse()))
                resAction = GuestDnD::toMainAction(pResp->defAction());
        }
    }

    if (RT_FAILURE(rc))
        hr = VBOX_E_IPRT_ERROR;

    if (SUCCEEDED(hr))
    {
        if (aResultAction)
            *aResultAction = resAction;
    }

    LogFlowFunc(("hr=%Rhrc, resAction=%ld\n", hr, resAction));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::move(ULONG aScreenId, ULONG aX, ULONG aY,
                             DnDAction_T                      aDefaultAction,
                             const std::vector<DnDAction_T>  &aAllowedActions,
                             const GuestDnDMIMEList          &aFormats,
                             DnDAction_T                     *aResultAction)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    /* Input validation. */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* Default action is ignoring. */
    DnDAction_T resAction = DnDAction_Ignore;

    /* Check & convert the drag & drop actions. */
    uint32_t uDefAction      = 0;
    uint32_t uAllowedActions = 0;
    GuestDnD::toHGCMActions(aDefaultAction, &uDefAction,
                            aAllowedActions, &uAllowedActions);
    /* If there is no usable action, ignore this request. */
    if (isDnDIgnoreAction(uDefAction))
        return S_OK;

    /*
     * Make a flat data string out of the supported format list.
     * In the GuestDnDTarget case the source formats are from the host,
     * as GuestDnDTarget acts as a source for the guest.
     */
    Utf8Str strFormats       = GuestDnD::toFormatString(GuestDnD::toFilteredFormatList(m_lstFmtSupported, aFormats));
    if (strFormats.isEmpty())
        return setError(E_INVALIDARG, tr("No or not supported format(s) specified"));
    const uint32_t cbFormats = (uint32_t)strFormats.length() + 1; /* Include terminating zero. */

    HRESULT hr = S_OK;

    int rc = GuestDnDInst()->adjustScreenCoordinates(aScreenId, &aX, &aY);
    if (RT_SUCCESS(rc))
    {
        GuestDnDMsg Msg;
        Msg.setType(HOST_DND_HG_EVT_MOVE);
        if (mDataBase.m_uProtocolVersion >= 3)
            Msg.setNextUInt32(0); /** @todo ContextID not used yet. */
        Msg.setNextUInt32(aScreenId);
        Msg.setNextUInt32(aX);
        Msg.setNextUInt32(aY);
        Msg.setNextUInt32(uDefAction);
        Msg.setNextUInt32(uAllowedActions);
        Msg.setNextPointer((void *)strFormats.c_str(), cbFormats);
        Msg.setNextUInt32(cbFormats);

        rc = GuestDnDInst()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
        if (RT_SUCCESS(rc))
        {
            GuestDnDResponse *pResp = GuestDnDInst()->response();
            if (pResp && RT_SUCCESS(pResp->waitForGuestResponse()))
                resAction = GuestDnD::toMainAction(pResp->defAction());
        }
    }

    if (RT_FAILURE(rc))
        hr = VBOX_E_IPRT_ERROR;

    if (SUCCEEDED(hr))
    {
        if (aResultAction)
            *aResultAction = resAction;
    }

    LogFlowFunc(("hr=%Rhrc, *pResultAction=%ld\n", hr, resAction));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::leave(ULONG uScreenId)
{
    RT_NOREF(uScreenId);
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    HRESULT hr = S_OK;
    int rc = GuestDnDInst()->hostCall(HOST_DND_HG_EVT_LEAVE,
                                      0 /* cParms */, NULL /* paParms */);
    if (RT_SUCCESS(rc))
    {
        GuestDnDResponse *pResp = GuestDnDInst()->response();
        if (pResp)
            pResp->waitForGuestResponse();
    }

    if (RT_FAILURE(rc))
        hr = VBOX_E_IPRT_ERROR;

    LogFlowFunc(("hr=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

HRESULT GuestDnDTarget::drop(ULONG aScreenId, ULONG aX, ULONG aY,
                             DnDAction_T                      aDefaultAction,
                             const std::vector<DnDAction_T>  &aAllowedActions,
                             const GuestDnDMIMEList          &aFormats,
                             com::Utf8Str                    &aFormat,
                             DnDAction_T                     *aResultAction)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    if (aDefaultAction == DnDAction_Ignore)
        return setError(E_INVALIDARG, tr("Invalid default action specified"));
    if (!aAllowedActions.size())
        return setError(E_INVALIDARG, tr("Invalid allowed actions specified"));
    if (!aFormats.size())
        return setError(E_INVALIDARG, tr("No drop format(s) specified"));
    /* aResultAction is optional. */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* Default action is ignoring. */
    DnDAction_T resAction    = DnDAction_Ignore;

    /* Check & convert the drag & drop actions to HGCM codes. */
    uint32_t uDefAction      = DND_IGNORE_ACTION;
    uint32_t uAllowedActions = 0;
    GuestDnD::toHGCMActions(aDefaultAction,  &uDefAction,
                            aAllowedActions, &uAllowedActions);
    /* If there is no usable action, ignore this request. */
    if (isDnDIgnoreAction(uDefAction))
    {
        aFormat = "";
        if (aResultAction)
            *aResultAction = DnDAction_Ignore;
        return S_OK;
    }

    /*
     * Make a flat data string out of the supported format list.
     * In the GuestDnDTarget case the source formats are from the host,
     * as GuestDnDTarget acts as a source for the guest.
     */
    Utf8Str strFormats       = GuestDnD::toFormatString(GuestDnD::toFilteredFormatList(m_lstFmtSupported, aFormats));
    if (strFormats.isEmpty())
        return setError(E_INVALIDARG, tr("No or not supported format(s) specified"));
    const uint32_t cbFormats = (uint32_t)strFormats.length() + 1; /* Include terminating zero. */

    /* Adjust the coordinates in a multi-monitor setup. */
    HRESULT hr = GuestDnDInst()->adjustScreenCoordinates(aScreenId, &aX, &aY);
    if (SUCCEEDED(hr))
    {
        GuestDnDMsg Msg;
        Msg.setType(HOST_DND_HG_EVT_DROPPED);
        if (mDataBase.m_uProtocolVersion >= 3)
            Msg.setNextUInt32(0); /** @todo ContextID not used yet. */
        Msg.setNextUInt32(aScreenId);
        Msg.setNextUInt32(aX);
        Msg.setNextUInt32(aY);
        Msg.setNextUInt32(uDefAction);
        Msg.setNextUInt32(uAllowedActions);
        Msg.setNextPointer((void*)strFormats.c_str(), cbFormats);
        Msg.setNextUInt32(cbFormats);

        int rc = GuestDnDInst()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
        if (RT_SUCCESS(rc))
        {
            GuestDnDResponse *pResp = GuestDnDInst()->response();
            AssertPtr(pResp);

            rc = pResp->waitForGuestResponse();
            if (RT_SUCCESS(rc))
            {
                resAction = GuestDnD::toMainAction(pResp->defAction());

                GuestDnDMIMEList lstFormats = pResp->formats();
                if (lstFormats.size() == 1) /* Exactly one format to use specified? */
                {
                    aFormat = lstFormats.at(0);
                    LogFlowFunc(("resFormat=%s, resAction=%RU32\n", aFormat.c_str(), pResp->defAction()));
                }
                else
                    hr = setError(VBOX_E_IPRT_ERROR, tr("Guest returned invalid drop formats (%zu formats)"), lstFormats.size());
            }
            else
                hr = setError(VBOX_E_IPRT_ERROR, tr("Waiting for response of dropped event failed (%Rrc)"), rc);
        }
        else
            hr = setError(VBOX_E_IPRT_ERROR, tr("Sending dropped event to guest failed (%Rrc)"), rc);
    }
    else
        hr = setError(hr, tr("Retrieving drop coordinates failed"));

    if (SUCCEEDED(hr))
    {
        if (aResultAction)
            *aResultAction = resAction;
    }

    LogFlowFunc(("Returning hr=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

/* static */
void GuestDnDTarget::i_sendDataThreadTask(SendDataTask *pTask)
{
    LogFlowFunc(("pTask=%p\n", pTask));

    AssertPtrReturnVoid(pTask);

    const ComObjPtr<GuestDnDTarget> pThis(pTask->getTarget());
    Assert(!pThis.isNull());

    AutoCaller autoCaller(pThis);
    if (FAILED(autoCaller.rc()))
        return;

    int vrc = pThis->i_sendData(pTask->getCtx(), RT_INDEFINITE_WAIT /* msTimeout */);
    NOREF(vrc);
/** @todo
 *
 *  r=bird: What happens with @a vrc?
 *
 */

    AutoWriteLock alock(pThis COMMA_LOCKVAL_SRC_POS);

    Assert(pThis->mDataBase.m_cTransfersPending);
    if (pThis->mDataBase.m_cTransfersPending)
        pThis->mDataBase.m_cTransfersPending--;

    LogFlowFunc(("pTarget=%p vrc=%Rrc (ignored)\n", (GuestDnDTarget *)pThis, vrc));
}

/**
 * Initiates a data transfer from the host to the guest. The source is the host whereas the target is the
 * guest in this case.
 *
 * @return  HRESULT
 * @param   aScreenId
 * @param   aFormat
 * @param   aData
 * @param   aProgress
 */
HRESULT GuestDnDTarget::sendData(ULONG aScreenId, const com::Utf8Str &aFormat, const std::vector<BYTE> &aData,
                                 ComPtr<IProgress> &aProgress)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* Input validation. */
    if (RT_UNLIKELY((aFormat.c_str()) == NULL || *(aFormat.c_str()) == '\0'))
        return setError(E_INVALIDARG, tr("No data format specified"));
    if (RT_UNLIKELY(!aData.size()))
        return setError(E_INVALIDARG, tr("No data to send specified"));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* At the moment we only support one transfer at a time. */
    if (mDataBase.m_cTransfersPending)
        return setError(E_INVALIDARG, tr("Another drop operation already is in progress"));

    /* Ditto. */
    GuestDnDResponse *pResp = GuestDnDInst()->response();
    AssertPtr(pResp);

    HRESULT hr = pResp->resetProgress(m_pGuest);
    if (FAILED(hr))
        return hr;

    SendDataTask *pTask = NULL;
    PSENDDATACTX pSendCtx = NULL;

    try
    {
        //pSendCtx is passed into SendDataTask where one is deleted in destructor
        pSendCtx = new SENDDATACTX;
        RT_BZERO(pSendCtx, sizeof(SENDDATACTX));

        pSendCtx->mpTarget      = this;
        pSendCtx->mpResp        = pResp;
        pSendCtx->mScreenID     = aScreenId;
        pSendCtx->mFmtReq       = aFormat;
        pSendCtx->mData.getMeta().add(aData);

        /* pTask is responsible for deletion of pSendCtx after creating */
        pTask = new SendDataTask(this, pSendCtx);
        if (!pTask->isOk())
        {
            delete pTask;
            LogRel2(("DnD: Could not create SendDataTask object \n"));
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
        LogRel2(("DnD: Could not create thread for data sending task\n"));
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
        hr = setError(VBOX_E_IPRT_ERROR, tr("Starting thread for GuestDnDTarget::i_sendDataThread (%Rhrc)"), hr);

    LogFlowFunc(("Returning hr=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

int GuestDnDTarget::i_cancelOperation(void)
{
    /** @todo Check for pending cancel requests. */

#if 0 /** @todo Later. */
    /* Cancel any outstanding waits for guest responses first. */
    if (pResp)
        pResp->notifyAboutGuestResponse();
#endif

    LogFlowFunc(("Cancelling operation, telling guest ...\n"));
    return GuestDnDInst()->hostCall(HOST_DND_HG_EVT_CANCEL, 0 /* cParms */, NULL /*paParms*/);
}

/* static */
Utf8Str GuestDnDTarget::i_guestErrorToString(int guestRc)
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
            strError += Utf8StrFmt(tr("The guest was not able to process the drag and drop data within time"));
            break;

        default:
            strError += Utf8StrFmt(tr("Drag and drop error from guest (%Rrc)"), guestRc);
            break;
    }

    return strError;
}

/* static */
Utf8Str GuestDnDTarget::i_hostErrorToString(int hostRc)
{
    Utf8Str strError;

    switch (hostRc)
    {
        case VERR_ACCESS_DENIED:
            strError += Utf8StrFmt(tr("For one or more host files or directories selected for transferring to the guest your host "
                                      "user does not have the appropriate access rights for. Please make sure that all selected "
                                      "elements can be accessed and that your host user has the appropriate rights."));
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

/**
 * @returns VBox status code that the caller ignores. Not sure if that's
 *          intentional or not.
 */
int GuestDnDTarget::i_sendData(PSENDDATACTX pCtx, RTMSINTERVAL msTimeout)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    int rc;

    ASMAtomicWriteBool(&pCtx->mIsActive, true);

    /* Clear all remaining outgoing messages. */
    mDataBase.m_lstMsgOut.clear();

    /**
     * Do we need to build up a file tree?
     * Note: The decision whether we need to build up a file tree and sending
     *       actual file data only depends on the actual formats offered by this target.
     *       If the guest does not want an URI list ("text/uri-list") but text ("TEXT" and
     *       friends) instead, still send the data over to the guest -- the file as such still
     *       is needed on the guest in this case, as the guest then just wants a simple path
     *       instead of an URI list (pointing to a file on the guest itself).
     *
     ** @todo Support more than one format; add a format<->function handler concept. Later. */
    bool fHasURIList = std::find(m_lstFmtOffered.begin(),
                                 m_lstFmtOffered.end(), "text/uri-list") != m_lstFmtOffered.end();
    if (fHasURIList)
    {
        rc = i_sendURIData(pCtx, msTimeout);
    }
    else
    {
        rc = i_sendRawData(pCtx, msTimeout);
    }

    ASMAtomicWriteBool(&pCtx->mIsActive, false);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDTarget::i_sendDataBody(PSENDDATACTX pCtx, GuestDnDData *pData)
{
    AssertPtrReturn(pCtx,  VERR_INVALID_POINTER);
    AssertPtrReturn(pData, VERR_INVALID_POINTER);

    /** @todo Add support for multiple HOST_DND_HG_SND_DATA messages in case of more than 64K data! */
    if (pData->getMeta().getSize() > _64K)
        return VERR_NOT_IMPLEMENTED;

    GuestDnDMsg Msg;

    LogFlowFunc(("cbFmt=%RU32, cbMeta=%RU32, cbChksum=%RU32\n",
                 pData->getFmtSize(), pData->getMeta().getSize(), pData->getChkSumSize()));

    Msg.setType(HOST_DND_HG_SND_DATA);
    if (mDataBase.m_uProtocolVersion < 3)
    {
        Msg.setNextUInt32(pCtx->mScreenID);                                                /* uScreenId */
        Msg.setNextPointer(pData->getFmtMutable(), pData->getFmtSize());                   /* pvFormat */
        Msg.setNextUInt32(pData->getFmtSize());                                            /* cbFormat */
        Msg.setNextPointer(pData->getMeta().getDataMutable(), pData->getMeta().getSize()); /* pvData */
        /* Fill in the current data block size to send.
         * Note: Only supports uint32_t. */
        Msg.setNextUInt32((uint32_t)pData->getMeta().getSize());                           /* cbData */
    }
    else
    {
        Msg.setNextUInt32(0); /** @todo ContextID not used yet. */
        Msg.setNextPointer(pData->getMeta().getDataMutable(), pData->getMeta().getSize()); /* pvData */
        Msg.setNextUInt32(pData->getMeta().getSize());                                     /* cbData */
        Msg.setNextPointer(pData->getChkSumMutable(), pData->getChkSumSize());             /** @todo pvChecksum; not used yet. */
        Msg.setNextUInt32(pData->getChkSumSize());                                         /** @todo cbChecksum; not used yet. */
    }

    int rc = GuestDnDInst()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());
    if (RT_SUCCESS(rc))
        rc = updateProgress(pData, pCtx->mpResp, pData->getMeta().getSize());

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDTarget::i_sendDataHeader(PSENDDATACTX pCtx, GuestDnDData *pData, GuestDnDURIData *pURIData /* = NULL */)
{
    AssertPtrReturn(pCtx,  VERR_INVALID_POINTER);
    AssertPtrReturn(pData, VERR_INVALID_POINTER);
    /* pURIData is optional. */

    GuestDnDMsg Msg;

    Msg.setType(HOST_DND_HG_SND_DATA_HDR);

    Msg.setNextUInt32(0);                                                /** @todo uContext; not used yet. */
    Msg.setNextUInt32(0);                                                /** @todo uFlags; not used yet. */
    Msg.setNextUInt32(pCtx->mScreenID);                                  /* uScreen */
    Msg.setNextUInt64(pData->getTotal());                                /* cbTotal */
    Msg.setNextUInt32(pData->getMeta().getSize());                       /* cbMeta*/
    Msg.setNextPointer(pData->getFmtMutable(), pData->getFmtSize());     /* pvMetaFmt */
    Msg.setNextUInt32(pData->getFmtSize());                              /* cbMetaFmt */
    Msg.setNextUInt64(pURIData ? pURIData->getObjToProcess() : 0);       /* cObjects */
    Msg.setNextUInt32(0);                                                /** @todo enmCompression; not used yet. */
    Msg.setNextUInt32(0);                                                /** @todo enmChecksumType; not used yet. */
    Msg.setNextPointer(NULL, 0);                                         /** @todo pvChecksum; not used yet. */
    Msg.setNextUInt32(0);                                                /** @todo cbChecksum; not used yet. */

    int rc = GuestDnDInst()->hostCall(Msg.getType(), Msg.getCount(), Msg.getParms());

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDTarget::i_sendDirectory(PSENDDATACTX pCtx, GuestDnDURIObjCtx *pObjCtx, GuestDnDMsg *pMsg)
{
    AssertPtrReturn(pCtx,    VERR_INVALID_POINTER);
    AssertPtrReturn(pObjCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pMsg,    VERR_INVALID_POINTER);

    DnDURIObject *pObj = pObjCtx->getObj();
    AssertPtr(pObj);

    RTCString strPath = pObj->GetDestPath();
    if (strPath.isEmpty())
        return VERR_INVALID_PARAMETER;
    if (strPath.length() >= RTPATH_MAX) /* Note: Maximum is RTPATH_MAX on guest side. */
        return VERR_BUFFER_OVERFLOW;

    LogRel2(("DnD: Transferring host directory to guest: %s\n", strPath.c_str()));

    pMsg->setType(HOST_DND_HG_SND_DIR);
    if (mDataBase.m_uProtocolVersion >= 3)
        pMsg->setNextUInt32(0); /** @todo ContextID not used yet. */
    pMsg->setNextString(strPath.c_str());                  /* path */
    pMsg->setNextUInt32((uint32_t)(strPath.length() + 1)); /* path length (maximum is RTPATH_MAX on guest side). */
    pMsg->setNextUInt32(pObj->GetMode());                  /* mode */

    return VINF_SUCCESS;
}

int GuestDnDTarget::i_sendFile(PSENDDATACTX pCtx, GuestDnDURIObjCtx *pObjCtx, GuestDnDMsg *pMsg)
{
    AssertPtrReturn(pCtx,    VERR_INVALID_POINTER);
    AssertPtrReturn(pObjCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pMsg,    VERR_INVALID_POINTER);

    DnDURIObject *pObj = pObjCtx->getObj();
    AssertPtr(pObj);

    RTCString strPathSrc = pObj->GetSourcePath();
    if (strPathSrc.isEmpty())
        return VERR_INVALID_PARAMETER;

    int rc = VINF_SUCCESS;

    LogFlowFunc(("Sending file with %RU32 bytes buffer, using protocol v%RU32 ...\n",
                  mData.mcbBlockSize, mDataBase.m_uProtocolVersion));
    LogFlowFunc(("strPathSrc=%s, fIsOpen=%RTbool, cbSize=%RU64\n", strPathSrc.c_str(), pObj->IsOpen(), pObj->GetSize()));

    if (!pObj->IsOpen())
    {
        LogRel2(("DnD: Opening host file for transferring to guest: %s\n", strPathSrc.c_str()));
        rc = pObj->OpenEx(strPathSrc, DnDURIObject::File, DnDURIObject::Source,
                          RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_WRITE, 0 /* fFlags */);
        if (RT_FAILURE(rc))
            LogRel(("DnD: Error opening host file '%s', rc=%Rrc\n", strPathSrc.c_str(), rc));
    }

    bool fSendData = false;
    if (RT_SUCCESS(rc))
    {
        if (mDataBase.m_uProtocolVersion >= 2)
        {
            uint32_t fState = pObjCtx->getState();
            if (!(fState & DND_OBJCTX_STATE_HAS_HDR))
            {
                /*
                 * Since protocol v2 the file header and the actual file contents are
                 * separate messages, so send the file header first.
                 * The just registered callback will be called by the guest afterwards.
                 */
                pMsg->setType(HOST_DND_HG_SND_FILE_HDR);
                pMsg->setNextUInt32(0); /** @todo ContextID not used yet. */
                pMsg->setNextString(pObj->GetDestPath().c_str());                  /* pvName */
                pMsg->setNextUInt32((uint32_t)(pObj->GetDestPath().length() + 1)); /* cbName */
                pMsg->setNextUInt32(0);                                            /* uFlags */
                pMsg->setNextUInt32(pObj->GetMode());                              /* fMode */
                pMsg->setNextUInt64(pObj->GetSize());                              /* uSize */

                LogFlowFunc(("Sending file header ...\n"));
                LogRel2(("DnD: Transferring host file to guest: %s (%RU64 bytes, mode 0x%x)\n",
                         strPathSrc.c_str(), pObj->GetSize(), pObj->GetMode()));

                /** @todo Set progress object title to current file being transferred? */

                pObjCtx->setState(fState | DND_OBJCTX_STATE_HAS_HDR);
            }
            else
            {
                /* File header was sent, so only send the actual file data. */
                fSendData = true;
            }
        }
        else /* Protocol v1. */
        {
            /* Always send the file data, every time. */
            fSendData = true;
        }
    }

    if (   RT_SUCCESS(rc)
        && fSendData)
    {
        rc = i_sendFileData(pCtx, pObjCtx, pMsg);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDTarget::i_sendFileData(PSENDDATACTX pCtx, GuestDnDURIObjCtx *pObjCtx, GuestDnDMsg *pMsg)
{
    AssertPtrReturn(pCtx,    VERR_INVALID_POINTER);
    AssertPtrReturn(pObjCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pMsg,    VERR_INVALID_POINTER);

    DnDURIObject *pObj = pObjCtx->getObj();
    AssertPtr(pObj);

    AssertPtr(pCtx->mpResp);

    /** @todo Don't allow concurrent reads per context! */

    /*
     * Start sending stuff.
     */

    /* Set the message type. */
    pMsg->setType(HOST_DND_HG_SND_FILE_DATA);

    /* Protocol version 1 sends the file path *every* time with a new file chunk.
     * In protocol version 2 we only do this once with HOST_DND_HG_SND_FILE_HDR. */
    if (mDataBase.m_uProtocolVersion <= 1)
    {
        pMsg->setNextString(pObj->GetDestPath().c_str());                  /* pvName */
        pMsg->setNextUInt32((uint32_t)(pObj->GetDestPath().length() + 1)); /* cbName */
    }
    else if (mDataBase.m_uProtocolVersion >= 2)
    {
        pMsg->setNextUInt32(0);                                            /** @todo ContextID not used yet. */
    }

    uint32_t cbRead = 0;

    int rc = pObj->Read(pCtx->mURI.getBufferMutable(), pCtx->mURI.getBufferSize(), &cbRead);
    if (RT_SUCCESS(rc))
    {
        pCtx->mData.addProcessed(cbRead);
        LogFlowFunc(("cbBufSize=%zu, cbRead=%RU32\n", pCtx->mURI.getBufferSize(), cbRead));

        if (mDataBase.m_uProtocolVersion <= 1)
        {
            pMsg->setNextPointer(pCtx->mURI.getBufferMutable(), cbRead);    /* pvData */
            pMsg->setNextUInt32(cbRead);                                    /* cbData */
            pMsg->setNextUInt32(pObj->GetMode());                           /* fMode */
        }
        else /* Protocol v2 and up. */
        {
            pMsg->setNextPointer(pCtx->mURI.getBufferMutable(), cbRead);    /* pvData */
            pMsg->setNextUInt32(cbRead);                                    /* cbData */

            if (mDataBase.m_uProtocolVersion >= 3)
            {
                /** @todo Calculate checksum. */
                pMsg->setNextPointer(NULL, 0);                              /* pvChecksum */
                pMsg->setNextUInt32(0);                                     /* cbChecksum */
            }
        }

        if (pObj->IsComplete()) /* Done reading? */
        {
            LogRel2(("DnD: File transfer to guest complete: %s\n", pObj->GetSourcePath().c_str()));
            LogFlowFunc(("File '%s' complete\n", pObj->GetSourcePath().c_str()));

            /* DnDURIObject::Read() returns VINF_EOF when finished reading the entire fire,
             * but we don't want this here -- so just override this with VINF_SUCCESS. */
            rc = VINF_SUCCESS;
        }
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/* static */
DECLCALLBACK(int) GuestDnDTarget::i_sendURIDataCallback(uint32_t uMsg, void *pvParms, size_t cbParms, void *pvUser)
{
    PSENDDATACTX pCtx = (PSENDDATACTX)pvUser;
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    GuestDnDTarget *pThis = pCtx->mpTarget;
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    LogFlowFunc(("pThis=%p, uMsg=%RU32\n", pThis, uMsg));

    int  rc      = VINF_SUCCESS;
    int  rcGuest = VINF_SUCCESS; /* Contains error code from guest in case of VERR_GSTDND_GUEST_ERROR. */
    bool fNotify = false;

    switch (uMsg)
    {
        case GUEST_DND_CONNECT:
            /* Nothing to do here (yet). */
            break;

        case GUEST_DND_DISCONNECT:
            rc = VERR_CANCELLED;
            break;

        case GUEST_DND_GET_NEXT_HOST_MSG:
        {
            PVBOXDNDCBHGGETNEXTHOSTMSG pCBData = reinterpret_cast<PVBOXDNDCBHGGETNEXTHOSTMSG>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBHGGETNEXTHOSTMSG) == cbParms, VERR_INVALID_PARAMETER);
            AssertReturn(CB_MAGIC_DND_HG_GET_NEXT_HOST_MSG == pCBData->hdr.uMagic, VERR_INVALID_PARAMETER);

            try
            {
                GuestDnDMsg *pMsg = new GuestDnDMsg();

                rc = pThis->i_sendURIDataLoop(pCtx, pMsg);
                if (rc == VINF_EOF) /* Transfer complete? */
                {
                    LogFlowFunc(("Last URI item processed, bailing out\n"));
                }
                else if (RT_SUCCESS(rc))
                {
                    rc = pThis->msgQueueAdd(pMsg);
                    if (RT_SUCCESS(rc)) /* Return message type & required parameter count to the guest. */
                    {
                        LogFlowFunc(("GUEST_DND_GET_NEXT_HOST_MSG -> %RU32 (%RU32 params)\n", pMsg->getType(), pMsg->getCount()));
                        pCBData->uMsg   = pMsg->getType();
                        pCBData->cParms = pMsg->getCount();
                    }
                }

                if (   RT_FAILURE(rc)
                    || rc == VINF_EOF) /* Transfer complete? */
                {
                    delete pMsg;
                    pMsg = NULL;
                }
            }
            catch(std::bad_alloc & /*e*/)
            {
                rc = VERR_NO_MEMORY;
            }
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
                AssertMsgFailed(("Guest has sent an error event but did not specify an actual error code\n"));
                pCBData->rc = VERR_GENERAL_FAILURE; /* Make sure some error is set. */
            }

            rc = pCtx->mpResp->setProgress(100, DND_PROGRESS_ERROR, pCBData->rc,
                                           GuestDnDTarget::i_guestErrorToString(pCBData->rc));
            if (RT_SUCCESS(rc))
            {
                rc      = VERR_GSTDND_GUEST_ERROR;
                rcGuest = pCBData->rc;
            }
            break;
        }
        case HOST_DND_HG_SND_DIR:
        case HOST_DND_HG_SND_FILE_HDR:
        case HOST_DND_HG_SND_FILE_DATA:
        {
            PVBOXDNDCBHGGETNEXTHOSTMSGDATA pCBData
                = reinterpret_cast<PVBOXDNDCBHGGETNEXTHOSTMSGDATA>(pvParms);
            AssertPtr(pCBData);
            AssertReturn(sizeof(VBOXDNDCBHGGETNEXTHOSTMSGDATA) == cbParms, VERR_INVALID_PARAMETER);

            LogFlowFunc(("pCBData->uMsg=%RU32, paParms=%p, cParms=%RU32\n", pCBData->uMsg, pCBData->paParms, pCBData->cParms));

            GuestDnDMsg *pMsg = pThis->msgQueueGetNext();
            if (pMsg)
            {
                /*
                 * Sanity checks.
                 */
                if (   pCBData->uMsg    != uMsg
                    || pCBData->paParms == NULL
                    || pCBData->cParms  != pMsg->getCount())
                {
                    LogFlowFunc(("Current message does not match:\n"));
                    LogFlowFunc(("\tCallback: uMsg=%RU32, cParms=%RU32, paParms=%p\n",
                                 pCBData->uMsg, pCBData->cParms, pCBData->paParms));
                    LogFlowFunc(("\t    Next: uMsg=%RU32, cParms=%RU32\n", pMsg->getType(), pMsg->getCount()));

                    /* Start over. */
                    pThis->msgQueueClear();

                    rc = VERR_INVALID_PARAMETER;
                }

                if (RT_SUCCESS(rc))
                {
                    LogFlowFunc(("Returning uMsg=%RU32\n", uMsg));
                    rc = HGCM::Message::copyParms(pCBData->paParms, pCBData->cParms,  pMsg->getParms(), pMsg->getCount());
                    if (RT_SUCCESS(rc))
                    {
                        pCBData->cParms = pMsg->getCount();
                        pThis->msgQueueRemoveNext();
                    }
                    else
                        LogFlowFunc(("Copying parameters failed with rc=%Rrc\n", rc));
                }
            }
            else
                rc = VERR_NO_DATA;

            LogFlowFunc(("Processing next message ended with rc=%Rrc\n", rc));
            break;
        }
        default:
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    int rcToGuest = VINF_SUCCESS; /* Status which will be sent back to the guest. */

    /*
     * Resolve errors.
     */
    switch (rc)
    {
        case VINF_SUCCESS:
            break;

        case VINF_EOF:
        {
            LogRel2(("DnD: Transfer to guest complete\n"));

            /* Complete operation on host side. */
            fNotify = true;

            /* The guest expects VERR_NO_DATA if the transfer is complete. */
            rcToGuest = VERR_NO_DATA;
            break;
        }

        case VERR_GSTDND_GUEST_ERROR:
        {
            LogRel(("DnD: Guest reported error %Rrc, aborting transfer to guest\n", rcGuest));
            break;
        }

        case VERR_CANCELLED:
        {
            LogRel2(("DnD: Transfer to guest canceled\n"));
            rcToGuest = VERR_CANCELLED; /* Also cancel on guest side. */
            break;
        }

        default:
        {
            LogRel(("DnD: Host error %Rrc occurred, aborting transfer to guest\n", rc));
            rcToGuest = VERR_CANCELLED; /* Also cancel on guest side. */
            break;
        }
    }

    if (RT_FAILURE(rc))
    {
        /* Unregister this callback. */
        AssertPtr(pCtx->mpResp);
        int rc2 = pCtx->mpResp->setCallback(uMsg, NULL /* PFNGUESTDNDCALLBACK */);
        AssertRC(rc2);

        /* Let the waiter(s) know. */
        fNotify = true;
    }

    LogFlowFunc(("fNotify=%RTbool, rc=%Rrc, rcToGuest=%Rrc\n", fNotify, rc, rcToGuest));

    if (fNotify)
    {
        int rc2 = pCtx->mCBEvent.Notify(rc); /** @todo Also pass guest error back? */
        AssertRC(rc2);
    }

    LogFlowFuncLeaveRC(rc);
    return rcToGuest; /* Tell the guest. */
}

int GuestDnDTarget::i_sendURIData(PSENDDATACTX pCtx, RTMSINTERVAL msTimeout)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertPtr(pCtx->mpResp);

#define REGISTER_CALLBACK(x)                                            \
    do {                                                                \
        rc = pCtx->mpResp->setCallback(x, i_sendURIDataCallback, pCtx); \
        if (RT_FAILURE(rc))                                             \
            return rc;                                                  \
    } while (0)

#define UNREGISTER_CALLBACK(x)                        \
    do {                                              \
        int rc2 = pCtx->mpResp->setCallback(x, NULL); \
        AssertRC(rc2);                                \
    } while (0)

    int rc = pCtx->mURI.init(mData.mcbBlockSize);
    if (RT_FAILURE(rc))
        return rc;

    rc = pCtx->mCBEvent.Reset();
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Register callbacks.
     */
    /* Guest callbacks. */
    REGISTER_CALLBACK(GUEST_DND_CONNECT);
    REGISTER_CALLBACK(GUEST_DND_DISCONNECT);
    REGISTER_CALLBACK(GUEST_DND_GET_NEXT_HOST_MSG);
    REGISTER_CALLBACK(GUEST_DND_GH_EVT_ERROR);
    /* Host callbacks. */
    REGISTER_CALLBACK(HOST_DND_HG_SND_DIR);
    if (mDataBase.m_uProtocolVersion >= 2)
        REGISTER_CALLBACK(HOST_DND_HG_SND_FILE_HDR);
    REGISTER_CALLBACK(HOST_DND_HG_SND_FILE_DATA);

    do
    {
        /*
         * Extract URI list from current meta data.
         */
        GuestDnDData    *pData = &pCtx->mData;
        GuestDnDURIData *pURI  = &pCtx->mURI;

        rc = pURI->fromLocalMetaData(pData->getMeta());
        if (RT_FAILURE(rc))
            break;

        LogFlowFunc(("URI root objects: %zu, total bytes (raw data to transfer): %zu\n",
                     pURI->getURIList().RootCount(), pURI->getURIList().TotalBytes()));

        /*
         * Set the new meta data with the URI list in it.
         */
        rc = pData->getMeta().fromURIList(pURI->getURIList());
        if (RT_FAILURE(rc))
            break;

        /*
         * Set the estimated data sizes we are going to send.
         * The total size also contains the meta data size.
         */
        const uint32_t cbMeta = pData->getMeta().getSize();
        pData->setEstimatedSize(pURI->getURIList().TotalBytes() + cbMeta /* cbTotal */,
                                                                  cbMeta /* cbMeta  */);

        /*
         * Set the meta format.
         */
        void    *pvFmt = (void *)pCtx->mFmtReq.c_str();
        uint32_t cbFmt = (uint32_t)pCtx->mFmtReq.length() + 1;           /* Include terminating zero. */

        pData->setFmt(pvFmt, cbFmt);

        /*
         * The first message always is the data header. The meta data itself then follows
         * and *only* contains the root elements of an URI list.
         *
         * After the meta data we generate the messages required to send the
         * file/directory data itself.
         *
         * Note: Protocol < v3 use the first data message to tell what's being sent.
         */
        GuestDnDMsg Msg;

        /*
         * Send the data header first.
         */
        if (mDataBase.m_uProtocolVersion >= 3)
            rc = i_sendDataHeader(pCtx, pData, &pCtx->mURI);

        /*
         * Send the (meta) data body.
         */
        if (RT_SUCCESS(rc))
            rc = i_sendDataBody(pCtx, pData);

        if (RT_SUCCESS(rc))
        {
            rc = waitForEvent(&pCtx->mCBEvent, pCtx->mpResp, msTimeout);
            if (RT_SUCCESS(rc))
                pCtx->mpResp->setProgress(100, DND_PROGRESS_COMPLETE, VINF_SUCCESS);
        }

    } while (0);

    /*
     * Unregister callbacks.
     */
    /* Guest callbacks. */
    UNREGISTER_CALLBACK(GUEST_DND_CONNECT);
    UNREGISTER_CALLBACK(GUEST_DND_DISCONNECT);
    UNREGISTER_CALLBACK(GUEST_DND_GET_NEXT_HOST_MSG);
    UNREGISTER_CALLBACK(GUEST_DND_GH_EVT_ERROR);
    /* Host callbacks. */
    UNREGISTER_CALLBACK(HOST_DND_HG_SND_DIR);
    if (mDataBase.m_uProtocolVersion >= 2)
        UNREGISTER_CALLBACK(HOST_DND_HG_SND_FILE_HDR);
    UNREGISTER_CALLBACK(HOST_DND_HG_SND_FILE_DATA);

#undef REGISTER_CALLBACK
#undef UNREGISTER_CALLBACK

    if (RT_FAILURE(rc))
    {
        if (rc == VERR_CANCELLED)
            pCtx->mpResp->setProgress(100, DND_PROGRESS_CANCELLED, VINF_SUCCESS);
        else if (rc != VERR_GSTDND_GUEST_ERROR) /* Guest-side error are already handled in the callback. */
            pCtx->mpResp->setProgress(100, DND_PROGRESS_ERROR, rc,
                                      GuestDnDTarget::i_hostErrorToString(rc));
    }

    /*
     * Now that we've cleaned up tell the guest side to cancel.
     * This does not imply we're waiting for the guest to react, as the
     * host side never must depend on anything from the guest.
     */
    if (rc == VERR_CANCELLED)
    {
        int rc2 = sendCancel();
        AssertRC(rc2);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDTarget::i_sendURIDataLoop(PSENDDATACTX pCtx, GuestDnDMsg *pMsg)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pMsg, VERR_INVALID_POINTER);

    int rc = updateProgress(&pCtx->mData, pCtx->mpResp);
    AssertRC(rc);

    if (   pCtx->mData.isComplete()
        && pCtx->mURI.isComplete())
    {
        return VINF_EOF;
    }

    GuestDnDURIObjCtx &objCtx = pCtx->mURI.getObjCurrent();
    if (!objCtx.isValid())
        return VERR_WRONG_ORDER;

    DnDURIObject *pCurObj = objCtx.getObj();
    AssertPtr(pCurObj);

    uint32_t fMode = pCurObj->GetMode();
    LogRel3(("DnD: Processing: srcPath=%s, dstPath=%s, fMode=0x%x, cbSize=%RU32, fIsDir=%RTbool, fIsFile=%RTbool\n",
             pCurObj->GetSourcePath().c_str(), pCurObj->GetDestPath().c_str(),
             fMode, pCurObj->GetSize(),
             RTFS_IS_DIRECTORY(fMode), RTFS_IS_FILE(fMode)));

    if (RTFS_IS_DIRECTORY(fMode))
    {
        rc = i_sendDirectory(pCtx, &objCtx, pMsg);
    }
    else if (RTFS_IS_FILE(fMode))
    {
        rc = i_sendFile(pCtx, &objCtx, pMsg);
    }
    else
    {
        AssertMsgFailed(("fMode=0x%x is not supported for srcPath=%s, dstPath=%s\n",
                         fMode, pCurObj->GetSourcePath().c_str(), pCurObj->GetDestPath().c_str()));
        rc = VERR_NOT_SUPPORTED;
    }

    bool fRemove = false; /* Remove current entry? */
    if (   pCurObj->IsComplete()
        || RT_FAILURE(rc))
    {
        fRemove = true;
    }

    if (fRemove)
    {
        LogFlowFunc(("Removing \"%s\" from list, rc=%Rrc\n", pCurObj->GetSourcePath().c_str(), rc));
        pCtx->mURI.removeObjCurrent();
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int GuestDnDTarget::i_sendRawData(PSENDDATACTX pCtx, RTMSINTERVAL msTimeout)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    NOREF(msTimeout);

    GuestDnDData *pData = &pCtx->mData;

    /** @todo At the moment we only allow sending up to 64K raw data.
     *        For protocol v1+v2: Fix this by using HOST_DND_HG_SND_MORE_DATA.
     *        For protocol v3   : Send another HOST_DND_HG_SND_DATA message. */
    if (!pData->getMeta().getSize())
        return VINF_SUCCESS;

    int rc = VINF_SUCCESS;

    /*
     * Send the data header first.
     */
    if (mDataBase.m_uProtocolVersion >= 3)
        rc = i_sendDataHeader(pCtx, pData, NULL /* URI list */);

    /*
     * Send the (meta) data body.
     */
    if (RT_SUCCESS(rc))
        rc = i_sendDataBody(pCtx, pData);

    int rc2;
    if (RT_FAILURE(rc))
        rc2 = pCtx->mpResp->setProgress(100, DND_PROGRESS_ERROR, rc,
                                        GuestDnDTarget::i_hostErrorToString(rc));
    else
        rc2 = pCtx->mpResp->setProgress(100, DND_PROGRESS_COMPLETE, rc);
    AssertRC(rc2);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

HRESULT GuestDnDTarget::cancel(BOOL *aVeto)
{
#if !defined(VBOX_WITH_DRAG_AND_DROP)
    ReturnComNotImplemented();
#else /* VBOX_WITH_DRAG_AND_DROP */

    int rc = i_cancelOperation();

    if (aVeto)
        *aVeto = FALSE; /** @todo */

    HRESULT hr = RT_SUCCESS(rc) ? S_OK : VBOX_E_IPRT_ERROR;

    LogFlowFunc(("hr=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

