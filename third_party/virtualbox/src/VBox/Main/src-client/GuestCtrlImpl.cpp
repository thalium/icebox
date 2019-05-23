/* $Id: GuestCtrlImpl.cpp $ */
/** @file
 * VirtualBox COM class implementation: Guest
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_GROUP LOG_GROUP_GUEST_CONTROL
#include "LoggingNew.h"

#include "GuestImpl.h"
#ifdef VBOX_WITH_GUEST_CONTROL
# include "GuestSessionImpl.h"
# include "GuestCtrlImplPrivate.h"
#endif

#include "Global.h"
#include "ConsoleImpl.h"
#include "ProgressImpl.h"
#include "VBoxEvents.h"
#include "VMMDev.h"

#include "AutoCaller.h"

#include <VBox/VMMDev.h>
#ifdef VBOX_WITH_GUEST_CONTROL
# include <VBox/com/array.h>
# include <VBox/com/ErrorInfo.h>
#endif
#include <iprt/cpp/utils.h>
#include <iprt/file.h>
#include <iprt/getopt.h>
#include <iprt/isofs.h>
#include <iprt/list.h>
#include <iprt/path.h>
#include <VBox/vmm/pgm.h>

#include <memory>


/*
 * This #ifdef goes almost to the end of the file where there are a couple of
 * IGuest method implementations.
 */
#ifdef VBOX_WITH_GUEST_CONTROL


// public methods only for internal purposes
/////////////////////////////////////////////////////////////////////////////

/**
 * Static callback function for receiving updates on guest control commands
 * from the guest. Acts as a dispatcher for the actual class instance.
 *
 * @returns VBox status code.
 *
 * @todo
 *
 */
/* static */
DECLCALLBACK(int) Guest::i_notifyCtrlDispatcher(void    *pvExtension,
                                                uint32_t u32Function,
                                                void    *pvData,
                                                uint32_t cbData)
{
    using namespace guestControl;

    /*
     * No locking, as this is purely a notification which does not make any
     * changes to the object state.
     */
    LogFlowFunc(("pvExtension=%p, u32Function=%RU32, pvParms=%p, cbParms=%RU32\n",
                 pvExtension, u32Function, pvData, cbData));
    ComObjPtr<Guest> pGuest = reinterpret_cast<Guest *>(pvExtension);
    Assert(!pGuest.isNull());

    /*
     * For guest control 2.0 using the legacy commands we need to do the following here:
     * - Get the callback header to access the context ID
     * - Get the context ID of the callback
     * - Extract the session ID out of the context ID
     * - Dispatch the whole stuff to the appropriate session (if still exists)
     */
    if (cbData != sizeof(VBOXGUESTCTRLHOSTCALLBACK))
        return VERR_NOT_SUPPORTED;
    PVBOXGUESTCTRLHOSTCALLBACK pSvcCb = (PVBOXGUESTCTRLHOSTCALLBACK)pvData;
    AssertPtr(pSvcCb);

    if (!pSvcCb->mParms) /* At least context ID must be present. */
        return VERR_INVALID_PARAMETER;

    uint32_t uContextID;
    int rc = pSvcCb->mpaParms[0].getUInt32(&uContextID);
    AssertMsgRCReturn(rc, ("Unable to extract callback context ID, pvData=%p\n", pSvcCb), rc);
#ifdef DEBUG
    LogFlowFunc(("CID=%RU32, uSession=%RU32, uObject=%RU32, uCount=%RU32\n",
                 uContextID,
                 VBOX_GUESTCTRL_CONTEXTID_GET_SESSION(uContextID),
                 VBOX_GUESTCTRL_CONTEXTID_GET_OBJECT(uContextID),
                 VBOX_GUESTCTRL_CONTEXTID_GET_COUNT(uContextID)));
#endif

    VBOXGUESTCTRLHOSTCBCTX ctxCb = { u32Function, uContextID };
    rc = pGuest->i_dispatchToSession(&ctxCb, pSvcCb);

    LogFlowFunc(("Returning rc=%Rrc\n", rc));
    return rc;
}

// private methods
/////////////////////////////////////////////////////////////////////////////

int Guest::i_dispatchToSession(PVBOXGUESTCTRLHOSTCBCTX pCtxCb, PVBOXGUESTCTRLHOSTCALLBACK pSvcCb)
{
    LogFlowFunc(("pCtxCb=%p, pSvcCb=%p\n", pCtxCb, pSvcCb));

    AssertPtrReturn(pCtxCb, VERR_INVALID_POINTER);
    AssertPtrReturn(pSvcCb, VERR_INVALID_POINTER);

    LogFlowFunc(("uFunction=%RU32, uContextID=%RU32, uProtocol=%RU32\n",
                  pCtxCb->uFunction, pCtxCb->uContextID, pCtxCb->uProtocol));

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    uint32_t uSessionID = VBOX_GUESTCTRL_CONTEXTID_GET_SESSION(pCtxCb->uContextID);
#ifdef DEBUG
    LogFlowFunc(("uSessionID=%RU32 (%zu total)\n",
                 uSessionID, mData.mGuestSessions.size()));
#endif
    GuestSessions::const_iterator itSession
        = mData.mGuestSessions.find(uSessionID);

    int rc;
    if (itSession != mData.mGuestSessions.end())
    {
        ComObjPtr<GuestSession> pSession(itSession->second);
        Assert(!pSession.isNull());

        alock.release();

        bool fDispatch = true;
#ifdef DEBUG
        /*
         * Pre-check: If we got a status message with an error and VERR_TOO_MUCH_DATA
         *            it means that that guest could not handle the entire message
         *            because of its exceeding size. This should not happen on daily
         *            use but testcases might try this. It then makes no sense to dispatch
         *            this further because we don't have a valid context ID.
         */
        if (   pCtxCb->uFunction == GUEST_EXEC_STATUS
            && pSvcCb->mParms    >= 5)
        {
            CALLBACKDATA_PROC_STATUS dataCb;
            /* pSvcCb->mpaParms[0] always contains the context ID. */
            pSvcCb->mpaParms[1].getUInt32(&dataCb.uPID);
            pSvcCb->mpaParms[2].getUInt32(&dataCb.uStatus);
            pSvcCb->mpaParms[3].getUInt32(&dataCb.uFlags);
            pSvcCb->mpaParms[4].getPointer(&dataCb.pvData, &dataCb.cbData);

            if (   (         dataCb.uStatus == PROC_STS_ERROR)
                   /** @todo Note: Due to legacy reasons we cannot change uFlags to
                    *              int32_t, so just cast it for now. */
                && ((int32_t)dataCb.uFlags  == VERR_TOO_MUCH_DATA))
            {
                LogFlowFunc(("Requested command with too much data, skipping dispatching ...\n"));

                Assert(dataCb.uPID == 0);
                fDispatch = false;
            }
        }
#endif
        if (fDispatch)
        {
            switch (pCtxCb->uFunction)
            {
                case GUEST_DISCONNECTED:
                    rc = pSession->i_dispatchToThis(pCtxCb, pSvcCb);
                    break;

                case GUEST_EXEC_STATUS:
                case GUEST_EXEC_OUTPUT:
                case GUEST_EXEC_INPUT_STATUS:
                case GUEST_EXEC_IO_NOTIFY:
                    rc = pSession->i_dispatchToProcess(pCtxCb, pSvcCb);
                    break;

                case GUEST_FILE_NOTIFY:
                    rc = pSession->i_dispatchToFile(pCtxCb, pSvcCb);
                    break;

                case GUEST_SESSION_NOTIFY:
                    rc = pSession->i_dispatchToThis(pCtxCb, pSvcCb);
                    break;

                default:
                    /*
                     * Try processing generic messages which might
                     * (or might not) supported by certain objects.
                     * If the message either is not found or supported
                     * by the approprirate object, try handling it
                     * in this session object.
                     */
                    rc = pSession->i_dispatchToObject(pCtxCb, pSvcCb);
                    if (   rc == VERR_NOT_FOUND
                        || rc == VERR_NOT_SUPPORTED)
                    {
                        alock.acquire();

                        rc = pSession->dispatchGeneric(pCtxCb, pSvcCb);
                    }
#ifndef DEBUG_andy
                    if (rc == VERR_NOT_IMPLEMENTED)
                        AssertMsgFailed(("Received not handled function %RU32\n", pCtxCb->uFunction));
#endif
                    break;
            }
        }
        else
            rc = VERR_NOT_FOUND;
    }
    else
        rc = VERR_NOT_FOUND;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int Guest::i_sessionRemove(GuestSession *pSession)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);

    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    int rc = VERR_NOT_FOUND;

    LogFlowThisFunc(("Removing session (ID=%RU32) ...\n", pSession->i_getId()));

    GuestSessions::iterator itSessions = mData.mGuestSessions.begin();
    while (itSessions != mData.mGuestSessions.end())
    {
        if (pSession == itSessions->second)
        {
#ifdef DEBUG_andy
            ULONG cRefs = pSession->AddRef();
            Assert(cRefs >= 2);
            LogFlowThisFunc(("pCurSession=%p, cRefs=%RU32\n", pSession, cRefs - 2));
            pSession->Release();
#endif
            /* Make sure to consume the pointer before the one of the
             * iterator gets released. */
            ComObjPtr<GuestSession> pCurSession = pSession;

            LogFlowThisFunc(("Removing session (pSession=%p, ID=%RU32) (now total %ld sessions)\n",
                             pSession, pSession->i_getId(), mData.mGuestSessions.size() - 1));

            rc = pSession->i_onRemove();
            mData.mGuestSessions.erase(itSessions);

            alock.release(); /* Release lock before firing off event. */

            fireGuestSessionRegisteredEvent(mEventSource, pCurSession,
                                            false /* Unregistered */);
            pCurSession.setNull();
            break;
        }

        ++itSessions;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int Guest::i_sessionCreate(const GuestSessionStartupInfo &ssInfo,
                           const GuestCredentials &guestCreds, ComObjPtr<GuestSession> &pGuestSession)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    int rc = VERR_MAX_PROCS_REACHED;
    if (mData.mGuestSessions.size() >= VBOX_GUESTCTRL_MAX_SESSIONS)
        return rc;

    try
    {
        /* Create a new session ID and assign it. */
        uint32_t uNewSessionID = VBOX_GUESTCTRL_SESSION_ID_BASE;
        uint32_t uTries = 0;

        for (;;)
        {
            /* Is the context ID already used? */
            if (!i_sessionExists(uNewSessionID))
            {
                rc = VINF_SUCCESS;
                break;
            }
            uNewSessionID++;
            if (uNewSessionID >= VBOX_GUESTCTRL_MAX_SESSIONS)
                uNewSessionID = VBOX_GUESTCTRL_SESSION_ID_BASE;

            if (++uTries == VBOX_GUESTCTRL_MAX_SESSIONS)
                break; /* Don't try too hard. */
        }
        if (RT_FAILURE(rc)) throw rc;

        /* Create the session object. */
        HRESULT hr = pGuestSession.createObject();
        if (FAILED(hr)) throw VERR_COM_UNEXPECTED;

        /** @todo Use an overloaded copy operator. Later. */
        GuestSessionStartupInfo startupInfo;
        startupInfo.mID = uNewSessionID; /* Assign new session ID. */
        startupInfo.mName = ssInfo.mName;
        startupInfo.mOpenFlags = ssInfo.mOpenFlags;
        startupInfo.mOpenTimeoutMS = ssInfo.mOpenTimeoutMS;

        GuestCredentials guestCredentials;
        if (!guestCreds.mUser.isEmpty())
        {
            /** @todo Use an overloaded copy operator. Later. */
            guestCredentials.mUser = guestCreds.mUser;
            guestCredentials.mPassword = guestCreds.mPassword;
            guestCredentials.mDomain = guestCreds.mDomain;
        }
        else
        {
            /* Internal (annonymous) session. */
            startupInfo.mIsInternal = true;
        }

        rc = pGuestSession->init(this, startupInfo, guestCredentials);
        if (RT_FAILURE(rc)) throw rc;

        /*
         * Add session object to our session map. This is necessary
         * before calling openSession because the guest calls back
         * with the creation result of this session.
         */
        mData.mGuestSessions[uNewSessionID] = pGuestSession;

        alock.release(); /* Release lock before firing off event. */

        fireGuestSessionRegisteredEvent(mEventSource, pGuestSession,
                                        true /* Registered */);
    }
    catch (int rc2)
    {
        rc = rc2;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

inline bool Guest::i_sessionExists(uint32_t uSessionID)
{
    GuestSessions::const_iterator itSessions = mData.mGuestSessions.find(uSessionID);
    return (itSessions == mData.mGuestSessions.end()) ? false : true;
}

#endif /* VBOX_WITH_GUEST_CONTROL */


// implementation of public methods
/////////////////////////////////////////////////////////////////////////////
HRESULT Guest::createSession(const com::Utf8Str &aUser, const com::Utf8Str &aPassword, const com::Utf8Str &aDomain,
                             const com::Utf8Str &aSessionName, ComPtr<IGuestSession> &aGuestSession)

{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else /* VBOX_WITH_GUEST_CONTROL */

    LogFlowFuncEnter();

    /* Do not allow anonymous sessions (with system rights) with public API. */
    if (RT_UNLIKELY(!aUser.length()))
        return setError(E_INVALIDARG, tr("No user name specified"));

    GuestSessionStartupInfo startupInfo;
    startupInfo.mName = aSessionName;

    GuestCredentials guestCreds;
    guestCreds.mUser = aUser;
    guestCreds.mPassword = aPassword;
    guestCreds.mDomain = aDomain;

    ComObjPtr<GuestSession> pSession;
    int rc = i_sessionCreate(startupInfo, guestCreds, pSession);
    if (RT_SUCCESS(rc))
    {
        /* Return guest session to the caller. */
        HRESULT hr2 = pSession.queryInterfaceTo(aGuestSession.asOutParam());
        if (FAILED(hr2))
            rc = VERR_COM_OBJECT_NOT_FOUND;
    }

    if (RT_SUCCESS(rc))
        /* Start (fork) the session asynchronously
         * on the guest. */
        rc = pSession->i_startSessionAsync();

    HRESULT hr = S_OK;

    if (RT_FAILURE(rc))
    {
        switch (rc)
        {
            case VERR_MAX_PROCS_REACHED:
                hr = setError(VBOX_E_IPRT_ERROR, tr("Maximum number of concurrent guest sessions (%ld) reached"),
                              VBOX_GUESTCTRL_MAX_SESSIONS);
                break;

            /** @todo Add more errors here. */

            default:
                hr = setError(VBOX_E_IPRT_ERROR, tr("Could not create guest session: %Rrc"), rc);
                break;
        }
    }

    LogFlowThisFunc(("Returning rc=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_GUEST_CONTROL */
}

HRESULT Guest::findSession(const com::Utf8Str &aSessionName, std::vector<ComPtr<IGuestSession> > &aSessions)
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else /* VBOX_WITH_GUEST_CONTROL */

    LogFlowFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    Utf8Str strName(aSessionName);
    std::list < ComObjPtr<GuestSession> > listSessions;

    GuestSessions::const_iterator itSessions = mData.mGuestSessions.begin();
    while (itSessions != mData.mGuestSessions.end())
    {
        if (strName.contains(itSessions->second->i_getName())) /** @todo Use a (simple) pattern match (IPRT?). */
            listSessions.push_back(itSessions->second);
        ++itSessions;
    }

    LogFlowFunc(("Sessions with \"%s\" = %RU32\n",
                 aSessionName.c_str(), listSessions.size()));

    aSessions.resize(listSessions.size());
    if (!listSessions.empty())
    {
        size_t i = 0;
        for (std::list < ComObjPtr<GuestSession> >::const_iterator it = listSessions.begin(); it != listSessions.end(); ++it, ++i)
            (*it).queryInterfaceTo(aSessions[i].asOutParam());

        return S_OK;

    }

    return setErrorNoLog(VBOX_E_OBJECT_NOT_FOUND,
                         tr("Could not find sessions with name '%s'"),
                         aSessionName.c_str());
#endif /* VBOX_WITH_GUEST_CONTROL */
}

HRESULT Guest::updateGuestAdditions(const com::Utf8Str &aSource, const std::vector<com::Utf8Str> &aArguments,
                                    const std::vector<AdditionsUpdateFlag_T> &aFlags, ComPtr<IProgress> &aProgress)
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else /* VBOX_WITH_GUEST_CONTROL */

    /* Validate flags. */
    uint32_t fFlags = AdditionsUpdateFlag_None;
    if (aFlags.size())
        for (size_t i = 0; i < aFlags.size(); ++i)
            fFlags |= aFlags[i];

    if (fFlags && !(fFlags & AdditionsUpdateFlag_WaitForUpdateStartOnly))
        return setError(E_INVALIDARG, tr("Unknown flags (%#x)"), fFlags);

    int rc = VINF_SUCCESS;

    ProcessArguments aArgs;
    aArgs.resize(0);

    if (aArguments.size())
    {
        try
        {
            for (size_t i = 0; i < aArguments.size(); ++i)
                aArgs.push_back(aArguments[i]);
        }
        catch(std::bad_alloc &)
        {
            rc = VERR_NO_MEMORY;
        }
    }

    HRESULT hr = S_OK;

    /*
     * Create an anonymous session. This is required to run the Guest Additions
     * update process with administrative rights.
     */
    GuestSessionStartupInfo startupInfo;
    startupInfo.mName = "Updating Guest Additions";

    GuestCredentials guestCreds;
    RT_ZERO(guestCreds);

    ComObjPtr<GuestSession> pSession;
    if (RT_SUCCESS(rc))
        rc = i_sessionCreate(startupInfo, guestCreds, pSession);
    if (RT_FAILURE(rc))
    {
        switch (rc)
        {
            case VERR_MAX_PROCS_REACHED:
                hr = setError(VBOX_E_IPRT_ERROR, tr("Maximum number of concurrent guest sessions (%ld) reached"),
                              VBOX_GUESTCTRL_MAX_SESSIONS);
                break;

            /** @todo Add more errors here. */

           default:
                hr = setError(VBOX_E_IPRT_ERROR, tr("Could not create guest session: %Rrc"), rc);
                break;
        }
    }
    else
    {
        Assert(!pSession.isNull());
        int guestRc;
        rc = pSession->i_startSessionInternal(&guestRc);
        if (RT_FAILURE(rc))
        {
            /** @todo Handle guestRc! */

            hr = setError(VBOX_E_IPRT_ERROR, tr("Could not open guest session: %Rrc"), rc);
        }
        else
        {

            ComObjPtr<Progress> pProgress;
            SessionTaskUpdateAdditions *pTask = NULL;
            try
            {
                try
                {
                    pTask = new SessionTaskUpdateAdditions(pSession /* GuestSession */, aSource, aArgs, fFlags);
                }
                catch(...)
                {
                    hr = setError(VBOX_E_IPRT_ERROR, tr("Failed to create SessionTaskUpdateAdditions object "));
                    throw;
                }


                hr = pTask->Init(Utf8StrFmt(tr("Updating Guest Additions")));
                if (FAILED(hr))
                {
                    delete pTask;
                    hr = setError(VBOX_E_IPRT_ERROR,
                                  tr("Creating progress object for SessionTaskUpdateAdditions object failed"));
                    throw hr;
                }

                hr = pTask->createThreadWithType(RTTHREADTYPE_MAIN_HEAVY_WORKER);

                if (SUCCEEDED(hr))
                {
                    /* Return progress to the caller. */
                    pProgress = pTask->GetProgressObject();
                    hr = pProgress.queryInterfaceTo(aProgress.asOutParam());
                }
                else
                    hr = setError(VBOX_E_IPRT_ERROR,
                                  tr("Starting thread for updating Guest Additions on the guest failed "));
            }
            catch(std::bad_alloc &)
            {
                hr = E_OUTOFMEMORY;
            }
            catch(...)
            {
                LogFlowThisFunc(("Exception was caught in the function\n"));
            }
        }
    }

    LogFlowFunc(("Returning hr=%Rhrc\n", hr));
    return hr;
#endif /* VBOX_WITH_GUEST_CONTROL */
}

