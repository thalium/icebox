/* $Id: VBoxManageGuestCtrlListener.cpp $ */
/** @file
 * VBoxManage - Guest control listener implementations.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "VBoxManage.h"
#include "VBoxManageGuestCtrl.h"

#ifndef VBOX_ONLY_DOCS

#include <VBox/com/com.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>

#include <iprt/time.h>

#include <map>
#include <vector>



/*
 * GuestListenerBase
 * GuestListenerBase
 * GuestListenerBase
 */

GuestListenerBase::GuestListenerBase(void)
    : mfVerbose(false)
{
}

GuestListenerBase::~GuestListenerBase(void)
{
}

HRESULT GuestListenerBase::init(bool fVerbose)
{
    mfVerbose = fVerbose;
    return S_OK;
}



/*
 * GuestFileEventListener
 * GuestFileEventListener
 * GuestFileEventListener
 */

GuestFileEventListener::GuestFileEventListener(void)
{
}

GuestFileEventListener::~GuestFileEventListener(void)
{
}

void GuestFileEventListener::uninit(void)
{

}

STDMETHODIMP GuestFileEventListener::HandleEvent(VBoxEventType_T aType, IEvent *aEvent)
{
    switch (aType)
    {
        case VBoxEventType_OnGuestFileStateChanged:
        {
            HRESULT rc;
            do
            {
                ComPtr<IGuestFileStateChangedEvent> pEvent = aEvent;
                Assert(!pEvent.isNull());

                ComPtr<IGuestFile> pProcess;
                CHECK_ERROR_BREAK(pEvent, COMGETTER(File)(pProcess.asOutParam()));
                AssertBreak(!pProcess.isNull());
                FileStatus_T fileSts;
                CHECK_ERROR_BREAK(pEvent, COMGETTER(Status)(&fileSts));
                Bstr strPath;
                CHECK_ERROR_BREAK(pProcess, COMGETTER(FileName)(strPath.asOutParam()));
                ULONG uID;
                CHECK_ERROR_BREAK(pProcess, COMGETTER(Id)(&uID));

                RTPrintf("File ID=%RU32 \"%s\" changed status to [%s]\n",
                         uID, Utf8Str(strPath).c_str(), gctlFileStatusToText(fileSts));

            } while (0);
            break;
        }

        default:
            AssertFailed();
    }

    return S_OK;
}


/*
 * GuestProcessEventListener
 * GuestProcessEventListener
 * GuestProcessEventListener
 */

GuestProcessEventListener::GuestProcessEventListener(void)
{
}

GuestProcessEventListener::~GuestProcessEventListener(void)
{
}

void GuestProcessEventListener::uninit(void)
{

}

STDMETHODIMP GuestProcessEventListener::HandleEvent(VBoxEventType_T aType, IEvent *aEvent)
{
    switch (aType)
    {
        case VBoxEventType_OnGuestProcessStateChanged:
        {
            HRESULT rc;
            do
            {
                ComPtr<IGuestProcessStateChangedEvent> pEvent = aEvent;
                Assert(!pEvent.isNull());

                ComPtr<IGuestProcess> pProcess;
                CHECK_ERROR_BREAK(pEvent, COMGETTER(Process)(pProcess.asOutParam()));
                AssertBreak(!pProcess.isNull());
                ProcessStatus_T procSts;
                CHECK_ERROR_BREAK(pEvent, COMGETTER(Status)(&procSts));
                Bstr strPath;
                CHECK_ERROR_BREAK(pProcess, COMGETTER(ExecutablePath)(strPath.asOutParam()));
                ULONG uPID;
                CHECK_ERROR_BREAK(pProcess, COMGETTER(PID)(&uPID));

                RTPrintf("Process PID=%RU32 \"%s\" changed status to [%s]\n",
                         uPID, Utf8Str(strPath).c_str(), gctlProcessStatusToText(procSts));

            } while (0);
            break;
        }

        default:
            AssertFailed();
    }

    return S_OK;
}


/*
 * GuestSessionEventListener
 * GuestSessionEventListener
 * GuestSessionEventListener
 */

GuestSessionEventListener::GuestSessionEventListener(void)
{
}

GuestSessionEventListener::~GuestSessionEventListener(void)
{
}

void GuestSessionEventListener::uninit(void)
{
    GuestEventProcs::iterator itProc = mProcs.begin();
    while (itProc != mProcs.end())
    {
        if (!itProc->first.isNull())
        {
            HRESULT rc;
            do
            {
                /* Listener unregistration. */
                ComPtr<IEventSource> pES;
                CHECK_ERROR_BREAK(itProc->first, COMGETTER(EventSource)(pES.asOutParam()));
                if (!pES.isNull())
                    CHECK_ERROR_BREAK(pES, UnregisterListener(itProc->second.mListener));
            } while (0);
            itProc->first->Release();
        }

        ++itProc;
    }
    mProcs.clear();

    GuestEventFiles::iterator itFile = mFiles.begin();
    while (itFile != mFiles.end())
    {
        if (!itFile->first.isNull())
        {
            HRESULT rc;
            do
            {
                /* Listener unregistration. */
                ComPtr<IEventSource> pES;
                CHECK_ERROR_BREAK(itFile->first, COMGETTER(EventSource)(pES.asOutParam()));
                if (!pES.isNull())
                    CHECK_ERROR_BREAK(pES, UnregisterListener(itFile->second.mListener));
            } while (0);
            itFile->first->Release();
        }

        ++itFile;
    }
    mFiles.clear();
}

STDMETHODIMP GuestSessionEventListener::HandleEvent(VBoxEventType_T aType, IEvent *aEvent)
{
    switch (aType)
    {
        case VBoxEventType_OnGuestFileRegistered:
        {
            HRESULT rc;
            do
            {
                ComPtr<IGuestFileRegisteredEvent> pEvent = aEvent;
                Assert(!pEvent.isNull());

                ComPtr<IGuestFile> pFile;
                CHECK_ERROR_BREAK(pEvent, COMGETTER(File)(pFile.asOutParam()));
                AssertBreak(!pFile.isNull());
                BOOL fRegistered;
                CHECK_ERROR_BREAK(pEvent, COMGETTER(Registered)(&fRegistered));
                Bstr strPath;
                CHECK_ERROR_BREAK(pFile, COMGETTER(FileName)(strPath.asOutParam()));

                RTPrintf("File \"%s\" %s\n",
                         Utf8Str(strPath).c_str(),
                         fRegistered ? "registered" : "unregistered");
                if (fRegistered)
                {
                    if (mfVerbose)
                        RTPrintf("Registering ...\n");

                    /* Register for IGuestFile events. */
                    ComObjPtr<GuestFileEventListenerImpl> pListener;
                    pListener.createObject();
                    CHECK_ERROR_BREAK(pListener, init(new GuestFileEventListener()));

                    ComPtr<IEventSource> es;
                    CHECK_ERROR_BREAK(pFile, COMGETTER(EventSource)(es.asOutParam()));
                    com::SafeArray<VBoxEventType_T> eventTypes;
                    eventTypes.push_back(VBoxEventType_OnGuestFileStateChanged);
                    CHECK_ERROR_BREAK(es, RegisterListener(pListener, ComSafeArrayAsInParam(eventTypes),
                                                           true /* Active listener */));

                    GuestFileStats fileStats(pListener);
                    mFiles[pFile] = fileStats;
                }
                else
                {
                    GuestEventFiles::iterator itFile = mFiles.find(pFile);
                    if (itFile != mFiles.end())
                    {
                        if (mfVerbose)
                            RTPrintf("Unregistering file ...\n");

                        if (!itFile->first.isNull())
                        {
                            /* Listener unregistration. */
                            ComPtr<IEventSource> pES;
                            CHECK_ERROR(itFile->first, COMGETTER(EventSource)(pES.asOutParam()));
                            if (!pES.isNull())
                                CHECK_ERROR(pES, UnregisterListener(itFile->second.mListener));
                            itFile->first->Release();
                        }

                        mFiles.erase(itFile);
                    }
                }

            } while (0);
            break;
        }

        case VBoxEventType_OnGuestProcessRegistered:
        {
            HRESULT rc;
            do
            {
                ComPtr<IGuestProcessRegisteredEvent> pEvent = aEvent;
                Assert(!pEvent.isNull());

                ComPtr<IGuestProcess> pProcess;
                CHECK_ERROR_BREAK(pEvent, COMGETTER(Process)(pProcess.asOutParam()));
                AssertBreak(!pProcess.isNull());
                BOOL fRegistered;
                CHECK_ERROR_BREAK(pEvent, COMGETTER(Registered)(&fRegistered));
                Bstr strPath;
                CHECK_ERROR_BREAK(pProcess, COMGETTER(ExecutablePath)(strPath.asOutParam()));

                RTPrintf("Process \"%s\" %s\n",
                         Utf8Str(strPath).c_str(),
                         fRegistered ? "registered" : "unregistered");
                if (fRegistered)
                {
                    if (mfVerbose)
                        RTPrintf("Registering ...\n");

                    /* Register for IGuestProcess events. */
                    ComObjPtr<GuestProcessEventListenerImpl> pListener;
                    pListener.createObject();
                    CHECK_ERROR_BREAK(pListener, init(new GuestProcessEventListener()));

                    ComPtr<IEventSource> es;
                    CHECK_ERROR_BREAK(pProcess, COMGETTER(EventSource)(es.asOutParam()));
                    com::SafeArray<VBoxEventType_T> eventTypes;
                    eventTypes.push_back(VBoxEventType_OnGuestProcessStateChanged);
                    CHECK_ERROR_BREAK(es, RegisterListener(pListener, ComSafeArrayAsInParam(eventTypes),
                                                           true /* Active listener */));

                    GuestProcStats procStats(pListener);
                    mProcs[pProcess] = procStats;
                }
                else
                {
                    GuestEventProcs::iterator itProc = mProcs.find(pProcess);
                    if (itProc != mProcs.end())
                    {
                        if (mfVerbose)
                            RTPrintf("Unregistering process ...\n");

                        if (!itProc->first.isNull())
                        {
                            /* Listener unregistration. */
                            ComPtr<IEventSource> pES;
                            CHECK_ERROR(itProc->first, COMGETTER(EventSource)(pES.asOutParam()));
                            if (!pES.isNull())
                                CHECK_ERROR(pES, UnregisterListener(itProc->second.mListener));
                            itProc->first->Release();
                        }

                        mProcs.erase(itProc);
                    }
                }

            } while (0);
            break;
        }

        case VBoxEventType_OnGuestSessionStateChanged:
        {
            HRESULT rc;
            do
            {
                ComPtr<IGuestSessionStateChangedEvent> pEvent = aEvent;
                Assert(!pEvent.isNull());
                ComPtr<IGuestSession> pSession;
                CHECK_ERROR_BREAK(pEvent, COMGETTER(Session)(pSession.asOutParam()));
                AssertBreak(!pSession.isNull());

                GuestSessionStatus_T sessSts;
                CHECK_ERROR_BREAK(pSession, COMGETTER(Status)(&sessSts));
                ULONG uID;
                CHECK_ERROR_BREAK(pSession, COMGETTER(Id)(&uID));
                Bstr strName;
                CHECK_ERROR_BREAK(pSession, COMGETTER(Name)(strName.asOutParam()));

                RTPrintf("Session ID=%RU32 \"%s\" changed status to [%s]\n",
                         uID, Utf8Str(strName).c_str(), gctlGuestSessionStatusToText(sessSts));

            } while (0);
            break;
        }

        default:
            AssertFailed();
    }

    return S_OK;
}


/*
 * GuestEventListener
 * GuestEventListener
 * GuestEventListener
 */

GuestEventListener::GuestEventListener(void)
{
}

GuestEventListener::~GuestEventListener(void)
{
}

void GuestEventListener::uninit(void)
{
    GuestEventSessions::iterator itSession = mSessions.begin();
    while (itSession != mSessions.end())
    {
        if (!itSession->first.isNull())
        {
            HRESULT rc;
            do
            {
                /* Listener unregistration. */
                ComPtr<IEventSource> pES;
                CHECK_ERROR_BREAK(itSession->first, COMGETTER(EventSource)(pES.asOutParam()));
                if (!pES.isNull())
                    CHECK_ERROR_BREAK(pES, UnregisterListener(itSession->second.mListener));

            } while (0);
            itSession->first->Release();
        }

        ++itSession;
    }
    mSessions.clear();
}

STDMETHODIMP GuestEventListener::HandleEvent(VBoxEventType_T aType, IEvent *aEvent)
{
    switch (aType)
    {
        case VBoxEventType_OnGuestSessionRegistered:
        {
            HRESULT rc;
            do
            {
                ComPtr<IGuestSessionRegisteredEvent> pEvent = aEvent;
                Assert(!pEvent.isNull());

                ComPtr<IGuestSession> pSession;
                CHECK_ERROR_BREAK(pEvent, COMGETTER(Session)(pSession.asOutParam()));
                AssertBreak(!pSession.isNull());
                BOOL fRegistered;
                CHECK_ERROR_BREAK(pEvent, COMGETTER(Registered)(&fRegistered));
                Bstr strName;
                CHECK_ERROR_BREAK(pSession, COMGETTER(Name)(strName.asOutParam()));
                ULONG uID;
                CHECK_ERROR_BREAK(pSession, COMGETTER(Id)(&uID));

                RTPrintf("Session ID=%RU32 \"%s\" %s\n",
                         uID, Utf8Str(strName).c_str(),
                         fRegistered ? "registered" : "unregistered");
                if (fRegistered)
                {
                    if (mfVerbose)
                        RTPrintf("Registering ...\n");

                    /* Register for IGuestSession events. */
                    ComObjPtr<GuestSessionEventListenerImpl> pListener;
                    pListener.createObject();
                    CHECK_ERROR_BREAK(pListener, init(new GuestSessionEventListener()));

                    ComPtr<IEventSource> es;
                    CHECK_ERROR_BREAK(pSession, COMGETTER(EventSource)(es.asOutParam()));
                    com::SafeArray<VBoxEventType_T> eventTypes;
                    eventTypes.push_back(VBoxEventType_OnGuestFileRegistered);
                    eventTypes.push_back(VBoxEventType_OnGuestProcessRegistered);
                    CHECK_ERROR_BREAK(es, RegisterListener(pListener, ComSafeArrayAsInParam(eventTypes),
                                                           true /* Active listener */));

                    GuestSessionStats sessionStats(pListener);
                    mSessions[pSession] = sessionStats;
                }
                else
                {
                    GuestEventSessions::iterator itSession = mSessions.find(pSession);
                    if (itSession != mSessions.end())
                    {
                        if (mfVerbose)
                            RTPrintf("Unregistering ...\n");

                        if (!itSession->first.isNull())
                        {
                            /* Listener unregistration. */
                            ComPtr<IEventSource> pES;
                            CHECK_ERROR_BREAK(itSession->first, COMGETTER(EventSource)(pES.asOutParam()));
                            if (!pES.isNull())
                                CHECK_ERROR_BREAK(pES, UnregisterListener(itSession->second.mListener));
                            itSession->first->Release();
                        }

                        mSessions.erase(itSession);
                    }
                }

            } while (0);
            break;
        }

        default:
            AssertFailed();
    }

    return S_OK;
}

#endif /* !VBOX_ONLY_DOCS */

