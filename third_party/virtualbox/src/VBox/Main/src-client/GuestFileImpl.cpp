/* $Id: GuestFileImpl.cpp $ */
/** @file
 * VirtualBox Main - Guest file handling.
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
#define LOG_GROUP LOG_GROUP_GUEST_CONTROL //LOG_GROUP_MAIN_GUESTFILE
#include "LoggingNew.h"

#ifndef VBOX_WITH_GUEST_CONTROL
# error "VBOX_WITH_GUEST_CONTROL must defined in this file"
#endif
#include "GuestFileImpl.h"
#include "GuestSessionImpl.h"
#include "GuestCtrlImplPrivate.h"
#include "ConsoleImpl.h"
#include "VirtualBoxErrorInfoImpl.h"

#include "Global.h"
#include "AutoCaller.h"
#include "VBoxEvents.h"

#include <iprt/cpp/utils.h> /* For unconst(). */
#include <iprt/file.h>

#include <VBox/com/array.h>
#include <VBox/com/listeners.h>


/**
 * Internal listener class to serve events in an
 * active manner, e.g. without polling delays.
 */
class GuestFileListener
{
public:

    GuestFileListener(void)
    {
    }

    virtual ~GuestFileListener()
    {
    }

    HRESULT init(GuestFile *pFile)
    {
        AssertPtrReturn(pFile, E_POINTER);
        mFile = pFile;
        return S_OK;
    }

    void uninit(void)
    {
        mFile = NULL;
    }

    STDMETHOD(HandleEvent)(VBoxEventType_T aType, IEvent *aEvent)
    {
        switch (aType)
        {
            case VBoxEventType_OnGuestFileStateChanged:
            case VBoxEventType_OnGuestFileOffsetChanged:
            case VBoxEventType_OnGuestFileRead:
            case VBoxEventType_OnGuestFileWrite:
            {
                AssertPtrReturn(mFile, E_POINTER);
                int rc2 = mFile->signalWaitEvent(aType, aEvent);
                NOREF(rc2);
#ifdef DEBUG_andy
                LogFlowFunc(("Signalling events of type=%RU32, file=%p resulted in rc=%Rrc\n",
                             aType, mFile, rc2));
#endif
                break;
            }

            default:
                AssertMsgFailed(("Unhandled event %RU32\n", aType));
                break;
        }

        return S_OK;
    }

private:

    GuestFile *mFile;
};
typedef ListenerImpl<GuestFileListener, GuestFile*> GuestFileListenerImpl;

VBOX_LISTENER_DECLARE(GuestFileListenerImpl)

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

DEFINE_EMPTY_CTOR_DTOR(GuestFile)

HRESULT GuestFile::FinalConstruct(void)
{
    LogFlowThisFuncEnter();
    return BaseFinalConstruct();
}

void GuestFile::FinalRelease(void)
{
    LogFlowThisFuncEnter();
    uninit();
    BaseFinalRelease();
    LogFlowThisFuncLeave();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

/**
 * Initializes a file object but does *not* open the file on the guest
 * yet. This is done in the dedidcated openFile call.
 *
 * @return  IPRT status code.
 * @param   pConsole                Pointer to console object.
 * @param   pSession                Pointer to session object.
 * @param   uFileID                 Host-based file ID (part of the context ID).
 * @param   openInfo                File opening information.
 */
int GuestFile::init(Console *pConsole, GuestSession *pSession,
                    ULONG uFileID, const GuestFileOpenInfo &openInfo)
{
    LogFlowThisFunc(("pConsole=%p, pSession=%p, uFileID=%RU32, strPath=%s\n",
                     pConsole, pSession, uFileID, openInfo.mFileName.c_str()));

    AssertPtrReturn(pConsole, VERR_INVALID_POINTER);
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);

    /* Enclose the state transition NotReady->InInit->Ready. */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), VERR_OBJECT_DESTROYED);

    int vrc = bindToSession(pConsole, pSession, uFileID /* Object ID */);
    if (RT_SUCCESS(vrc))
    {
        mSession = pSession;

        mData.mID = uFileID;
        mData.mInitialSize = 0;
        mData.mStatus = FileStatus_Undefined;
        mData.mOpenInfo = openInfo;

        unconst(mEventSource).createObject();
        HRESULT hr = mEventSource->init();
        if (FAILED(hr))
            vrc = VERR_COM_UNEXPECTED;
    }

    if (RT_SUCCESS(vrc))
    {
        try
        {
            GuestFileListener *pListener = new GuestFileListener();
            ComObjPtr<GuestFileListenerImpl> thisListener;
            HRESULT hr = thisListener.createObject();
            if (SUCCEEDED(hr))
                hr = thisListener->init(pListener, this);

            if (SUCCEEDED(hr))
            {
                com::SafeArray <VBoxEventType_T> eventTypes;
                eventTypes.push_back(VBoxEventType_OnGuestFileStateChanged);
                eventTypes.push_back(VBoxEventType_OnGuestFileOffsetChanged);
                eventTypes.push_back(VBoxEventType_OnGuestFileRead);
                eventTypes.push_back(VBoxEventType_OnGuestFileWrite);
                hr = mEventSource->RegisterListener(thisListener,
                                                    ComSafeArrayAsInParam(eventTypes),
                                                    TRUE /* Active listener */);
                if (SUCCEEDED(hr))
                {
                    vrc = baseInit();
                    if (RT_SUCCESS(vrc))
                    {
                        mLocalListener = thisListener;
                    }
                }
                else
                    vrc = VERR_COM_UNEXPECTED;
            }
            else
                vrc = VERR_COM_UNEXPECTED;
        }
        catch(std::bad_alloc &)
        {
            vrc = VERR_NO_MEMORY;
        }
    }

    if (RT_SUCCESS(vrc))
    {
        /* Confirm a successful initialization when it's the case. */
        autoInitSpan.setSucceeded();
    }
    else
        autoInitSpan.setFailed();

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Uninitializes the instance.
 * Called from FinalRelease().
 */
void GuestFile::uninit(void)
{
    /* Enclose the state transition Ready->InUninit->NotReady. */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    LogFlowThisFuncEnter();

    baseUninit();
    LogFlowThisFuncLeave();
}

// implementation of public getters/setters for attributes
/////////////////////////////////////////////////////////////////////////////

HRESULT GuestFile::getCreationMode(ULONG *aCreationMode)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aCreationMode = mData.mOpenInfo.mCreationMode;

    return S_OK;
}

HRESULT GuestFile::getOpenAction(FileOpenAction_T *aOpenAction)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aOpenAction = mData.mOpenInfo.mOpenAction;

    return S_OK;
}

HRESULT GuestFile::getEventSource(ComPtr<IEventSource> &aEventSource)
{
    /* No need to lock - lifetime constant. */
    mEventSource.queryInterfaceTo(aEventSource.asOutParam());

    return S_OK;
}

HRESULT GuestFile::getFileName(com::Utf8Str &aFileName)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aFileName = mData.mOpenInfo.mFileName;

    return S_OK;
}

HRESULT GuestFile::getId(ULONG *aId)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aId = mData.mID;

    return S_OK;
}

HRESULT GuestFile::getInitialSize(LONG64 *aInitialSize)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aInitialSize = mData.mInitialSize;

    return S_OK;
}

HRESULT GuestFile::getOffset(LONG64 *aOffset)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aOffset = mData.mOffCurrent;

    return S_OK;
}

HRESULT GuestFile::getAccessMode(FileAccessMode_T *aAccessMode)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAccessMode = mData.mOpenInfo.mAccessMode;

    return S_OK;
}

HRESULT GuestFile::getStatus(FileStatus_T *aStatus)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aStatus = mData.mStatus;

    return S_OK;
}

// private methods
/////////////////////////////////////////////////////////////////////////////

int GuestFile::i_callbackDispatcher(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCb)
{
    AssertPtrReturn(pCbCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pSvcCb, VERR_INVALID_POINTER);

    LogFlowThisFunc(("strName=%s, uContextID=%RU32, uFunction=%RU32, pSvcCb=%p\n",
                     mData.mOpenInfo.mFileName.c_str(), pCbCtx->uContextID, pCbCtx->uFunction, pSvcCb));

    int vrc;
    switch (pCbCtx->uFunction)
    {
        case GUEST_DISCONNECTED:
            vrc = i_onGuestDisconnected(pCbCtx, pSvcCb);
            break;

        case GUEST_FILE_NOTIFY:
            vrc = i_onFileNotify(pCbCtx, pSvcCb);
            break;

        default:
            /* Silently ignore not implemented functions. */
            vrc = VERR_NOT_SUPPORTED;
            break;
    }

#ifdef DEBUG
    LogFlowFuncLeaveRC(vrc);
#endif
    return vrc;
}

int GuestFile::i_closeFile(int *pGuestRc)
{
    LogFlowThisFunc(("strFile=%s\n", mData.mOpenInfo.mFileName.c_str()));

    int vrc;

    GuestWaitEvent *pEvent = NULL;
    GuestEventTypes eventTypes;
    try
    {
        eventTypes.push_back(VBoxEventType_OnGuestFileStateChanged);

        vrc = registerWaitEvent(eventTypes, &pEvent);
    }
    catch (std::bad_alloc)
    {
        vrc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(vrc))
        return vrc;

    /* Prepare HGCM call. */
    VBOXHGCMSVCPARM paParms[4];
    int i = 0;
    paParms[i++].setUInt32(pEvent->ContextID());
    paParms[i++].setUInt32(mData.mID /* Guest file ID */);

    vrc = sendCommand(HOST_FILE_CLOSE, i, paParms);
    if (RT_SUCCESS(vrc))
        vrc = i_waitForStatusChange(pEvent, 30 * 1000 /* Timeout in ms */,
                                    NULL /* FileStatus */, pGuestRc);
    unregisterWaitEvent(pEvent);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/* static */
Utf8Str GuestFile::i_guestErrorToString(int guestRc)
{
    Utf8Str strError;

    /** @todo pData->u32Flags: int vs. uint32 -- IPRT errors are *negative* !!! */
    switch (guestRc)
    {
        case VERR_ALREADY_EXISTS:
            strError += Utf8StrFmt(tr("File already exists"));
            break;

        case VERR_FILE_NOT_FOUND:
            strError += Utf8StrFmt(tr("File not found"));
            break;

        case VERR_NET_HOST_NOT_FOUND:
            strError += Utf8StrFmt(tr("Host name not found"));
            break;

        case VERR_SHARING_VIOLATION:
            strError += Utf8StrFmt(tr("Sharing violation"));
            break;

        default:
            strError += Utf8StrFmt("%Rrc", guestRc);
            break;
    }

    return strError;
}

int GuestFile::i_onFileNotify(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData)
{
    AssertPtrReturn(pCbCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pSvcCbData, VERR_INVALID_POINTER);

    LogFlowThisFuncEnter();

    if (pSvcCbData->mParms < 3)
        return VERR_INVALID_PARAMETER;

    int vrc = VINF_SUCCESS;

    int idx = 1; /* Current parameter index. */
    CALLBACKDATA_FILE_NOTIFY dataCb;
    /* pSvcCb->mpaParms[0] always contains the context ID. */
    pSvcCbData->mpaParms[idx++].getUInt32(&dataCb.uType);
    pSvcCbData->mpaParms[idx++].getUInt32(&dataCb.rc);

    int guestRc = (int)dataCb.rc; /* uint32_t vs. int. */

    LogFlowFunc(("uType=%RU32, guestRc=%Rrc\n",
                 dataCb.uType, guestRc));

    if (RT_FAILURE(guestRc))
    {
        int rc2 = i_setFileStatus(FileStatus_Error, guestRc);
        AssertRC(rc2);

        rc2 = signalWaitEventInternal(pCbCtx,
                                      guestRc, NULL /* pPayload */);
        AssertRC(rc2);

        return VINF_SUCCESS; /* Report to the guest. */
    }

    switch (dataCb.uType)
    {
        case GUEST_FILE_NOTIFYTYPE_ERROR:
        {
            int rc2 = i_setFileStatus(FileStatus_Error, guestRc);
            AssertRC(rc2);

            break;
        }

        case GUEST_FILE_NOTIFYTYPE_OPEN:
        {
            if (pSvcCbData->mParms == 4)
            {
                pSvcCbData->mpaParms[idx++].getUInt32(&dataCb.u.open.uHandle);

                AssertMsg(mData.mID == VBOX_GUESTCTRL_CONTEXTID_GET_OBJECT(pCbCtx->uContextID),
                          ("File ID %RU32 does not match context ID %RU32\n", mData.mID,
                           VBOX_GUESTCTRL_CONTEXTID_GET_OBJECT(pCbCtx->uContextID)));

                /* Set the process status. */
                int rc2 = i_setFileStatus(FileStatus_Open, guestRc);
                AssertRC(rc2);
            }
            else
                vrc = VERR_NOT_SUPPORTED;

            break;
        }

        case GUEST_FILE_NOTIFYTYPE_CLOSE:
        {
            int rc2 = i_setFileStatus(FileStatus_Closed, guestRc);
            AssertRC(rc2);

            break;
        }

        case GUEST_FILE_NOTIFYTYPE_READ:
        {
            if (pSvcCbData->mParms == 4)
            {
                pSvcCbData->mpaParms[idx++].getPointer(&dataCb.u.read.pvData,
                                                       &dataCb.u.read.cbData);
                uint32_t cbRead = dataCb.u.read.cbData;

                AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

                mData.mOffCurrent += cbRead;

                alock.release();

                com::SafeArray<BYTE> data((size_t)cbRead);
                data.initFrom((BYTE*)dataCb.u.read.pvData, cbRead);

                fireGuestFileReadEvent(mEventSource, mSession, this, mData.mOffCurrent,
                                       cbRead, ComSafeArrayAsInParam(data));
            }
            else
                vrc = VERR_NOT_SUPPORTED;
            break;
        }

        case GUEST_FILE_NOTIFYTYPE_WRITE:
        {
            if (pSvcCbData->mParms == 4)
            {
                pSvcCbData->mpaParms[idx++].getUInt32(&dataCb.u.write.cbWritten);

                AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

                mData.mOffCurrent += dataCb.u.write.cbWritten;
                uint64_t uOffCurrent = mData.mOffCurrent;

                alock.release();

                fireGuestFileWriteEvent(mEventSource, mSession, this, uOffCurrent,
                                        dataCb.u.write.cbWritten);
            }
            else
                vrc = VERR_NOT_SUPPORTED;
            break;
        }

        case GUEST_FILE_NOTIFYTYPE_SEEK:
        {
            if (pSvcCbData->mParms == 4)
            {
                pSvcCbData->mpaParms[idx++].getUInt64(&dataCb.u.seek.uOffActual);

                AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

                mData.mOffCurrent = dataCb.u.seek.uOffActual;

                alock.release();

                fireGuestFileOffsetChangedEvent(mEventSource, mSession, this,
                                                dataCb.u.seek.uOffActual, 0 /* Processed */);
            }
            else
                vrc = VERR_NOT_SUPPORTED;
            break;
        }

        case GUEST_FILE_NOTIFYTYPE_TELL:
        {
            if (pSvcCbData->mParms == 4)
            {
                pSvcCbData->mpaParms[idx++].getUInt64(&dataCb.u.tell.uOffActual);

                AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

                mData.mOffCurrent = dataCb.u.tell.uOffActual;

                alock.release();

                fireGuestFileOffsetChangedEvent(mEventSource, mSession, this,
                                                dataCb.u.tell.uOffActual, 0 /* Processed */);
            }
            else
                vrc = VERR_NOT_SUPPORTED;
            break;
        }

        default:
            vrc = VERR_NOT_SUPPORTED;
            break;
    }

    if (RT_SUCCESS(vrc))
    {
        GuestWaitEventPayload payload(dataCb.uType, &dataCb, sizeof(dataCb));
        int rc2 = signalWaitEventInternal(pCbCtx, guestRc, &payload);
        AssertRC(rc2);
    }

    LogFlowThisFunc(("uType=%RU32, guestRc=%Rrc\n",
                     dataCb.uType, dataCb.rc));

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestFile::i_onGuestDisconnected(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData)
{
    AssertPtrReturn(pCbCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pSvcCbData, VERR_INVALID_POINTER);

    int vrc = i_setFileStatus(FileStatus_Down, VINF_SUCCESS);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Called by IGuestSession right before this file gets removed
 * from the public file list.
 */
int GuestFile::i_onRemove(void)
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    int vrc = VINF_SUCCESS;

    /*
     * Note: The event source stuff holds references to this object,
     *       so make sure that this is cleaned up *before* calling uninit().
     */
    if (!mEventSource.isNull())
    {
        mEventSource->UnregisterListener(mLocalListener);

        mLocalListener.setNull();
        unconst(mEventSource).setNull();
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestFile::i_openFile(uint32_t uTimeoutMS, int *pGuestRc)
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowThisFunc(("strFile=%s, enmAccessMode=%d (%s) enmOpenAction=%d (%s) uCreationMode=%RU32, mfOpenEx=%RU32\n",
                     mData.mOpenInfo.mFileName.c_str(), mData.mOpenInfo.mAccessMode, mData.mOpenInfo.mpszAccessMode,
                     mData.mOpenInfo.mOpenAction, mData.mOpenInfo.mpszOpenAction, mData.mOpenInfo.mCreationMode,
                     mData.mOpenInfo.mfOpenEx));
    int vrc;

    GuestWaitEvent *pEvent = NULL;
    GuestEventTypes eventTypes;
    try
    {
        eventTypes.push_back(VBoxEventType_OnGuestFileStateChanged);

        vrc = registerWaitEvent(eventTypes, &pEvent);
    }
    catch (std::bad_alloc)
    {
        vrc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(vrc))
        return vrc;

    /* Prepare HGCM call. */
    VBOXHGCMSVCPARM paParms[8];
    int i = 0;
    paParms[i++].setUInt32(pEvent->ContextID());
    paParms[i++].setPointer((void*)mData.mOpenInfo.mFileName.c_str(),
                            (ULONG)mData.mOpenInfo.mFileName.length() + 1);
    paParms[i++].setString(mData.mOpenInfo.mpszAccessMode);
    paParms[i++].setString(mData.mOpenInfo.mpszOpenAction);
    paParms[i++].setString(""); /** @todo sharing mode. */
    paParms[i++].setUInt32(mData.mOpenInfo.mCreationMode);
    paParms[i++].setUInt64(0 /* initial offset */);
    /** @todo Next protocol version: add flags, replace strings, remove initial offset. */

    alock.release(); /* Drop write lock before sending. */

    vrc = sendCommand(HOST_FILE_OPEN, i, paParms);
    if (RT_SUCCESS(vrc))
        vrc = i_waitForStatusChange(pEvent, uTimeoutMS,
                                    NULL /* FileStatus */, pGuestRc);

    unregisterWaitEvent(pEvent);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestFile::i_readData(uint32_t uSize, uint32_t uTimeoutMS,
                          void* pvData, uint32_t cbData, uint32_t* pcbRead)
{
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData, VERR_INVALID_PARAMETER);

    LogFlowThisFunc(("uSize=%RU32, uTimeoutMS=%RU32, pvData=%p, cbData=%zu\n",
                     uSize, uTimeoutMS, pvData, cbData));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    int vrc;

    GuestWaitEvent *pEvent = NULL;
    GuestEventTypes eventTypes;
    try
    {
        eventTypes.push_back(VBoxEventType_OnGuestFileStateChanged);
        eventTypes.push_back(VBoxEventType_OnGuestFileRead);

        vrc = registerWaitEvent(eventTypes, &pEvent);
    }
    catch (std::bad_alloc)
    {
        vrc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(vrc))
        return vrc;

    /* Prepare HGCM call. */
    VBOXHGCMSVCPARM paParms[4];
    int i = 0;
    paParms[i++].setUInt32(pEvent->ContextID());
    paParms[i++].setUInt32(mData.mID /* File handle */);
    paParms[i++].setUInt32(uSize /* Size (in bytes) to read */);

    alock.release(); /* Drop write lock before sending. */

    vrc = sendCommand(HOST_FILE_READ, i, paParms);
    if (RT_SUCCESS(vrc))
    {
        uint32_t cbRead = 0;
        vrc = i_waitForRead(pEvent, uTimeoutMS, pvData, cbData, &cbRead);
        if (RT_SUCCESS(vrc))
        {
            LogFlowThisFunc(("cbRead=%RU32\n", cbRead));
            if (pcbRead)
                *pcbRead = cbRead;
        }
    }

    unregisterWaitEvent(pEvent);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestFile::i_readDataAt(uint64_t uOffset, uint32_t uSize, uint32_t uTimeoutMS,
                            void* pvData, size_t cbData, size_t* pcbRead)
{
    LogFlowThisFunc(("uOffset=%RU64, uSize=%RU32, uTimeoutMS=%RU32, pvData=%p, cbData=%zu\n",
                     uOffset, uSize, uTimeoutMS, pvData, cbData));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    int vrc;

    GuestWaitEvent *pEvent = NULL;
    GuestEventTypes eventTypes;
    try
    {
        eventTypes.push_back(VBoxEventType_OnGuestFileStateChanged);
        eventTypes.push_back(VBoxEventType_OnGuestFileRead);

        vrc = registerWaitEvent(eventTypes, &pEvent);
    }
    catch (std::bad_alloc)
    {
        vrc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(vrc))
        return vrc;

    /* Prepare HGCM call. */
    VBOXHGCMSVCPARM paParms[4];
    int i = 0;
    paParms[i++].setUInt32(pEvent->ContextID());
    paParms[i++].setUInt32(mData.mID /* File handle */);
    paParms[i++].setUInt64(uOffset /* Offset (in bytes) to start reading */);
    paParms[i++].setUInt32(uSize /* Size (in bytes) to read */);

    alock.release(); /* Drop write lock before sending. */

    vrc = sendCommand(HOST_FILE_READ_AT, i, paParms);
    if (RT_SUCCESS(vrc))
    {
        uint32_t cbRead = 0;
        vrc = i_waitForRead(pEvent, uTimeoutMS, pvData, cbData, &cbRead);
        if (RT_SUCCESS(vrc))
        {
            LogFlowThisFunc(("cbRead=%RU32\n", cbRead));

            if (pcbRead)
                *pcbRead = cbRead;
        }
    }

    unregisterWaitEvent(pEvent);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestFile::i_seekAt(int64_t iOffset, GUEST_FILE_SEEKTYPE eSeekType,
                        uint32_t uTimeoutMS, uint64_t *puOffset)
{
    LogFlowThisFunc(("iOffset=%RI64, uTimeoutMS=%RU32\n",
                     iOffset, uTimeoutMS));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    int vrc;

    GuestWaitEvent *pEvent = NULL;
    GuestEventTypes eventTypes;
    try
    {
        eventTypes.push_back(VBoxEventType_OnGuestFileStateChanged);
        eventTypes.push_back(VBoxEventType_OnGuestFileOffsetChanged);

        vrc = registerWaitEvent(eventTypes, &pEvent);
    }
    catch (std::bad_alloc)
    {
        vrc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(vrc))
        return vrc;

    /* Prepare HGCM call. */
    VBOXHGCMSVCPARM paParms[4];
    int i = 0;
    paParms[i++].setUInt32(pEvent->ContextID());
    paParms[i++].setUInt32(mData.mID /* File handle */);
    paParms[i++].setUInt32(eSeekType /* Seek method */);
    /** @todo uint64_t vs. int64_t! */
    paParms[i++].setUInt64((uint64_t)iOffset /* Offset (in bytes) to start reading */);

    alock.release(); /* Drop write lock before sending. */

    vrc = sendCommand(HOST_FILE_SEEK, i, paParms);
    if (RT_SUCCESS(vrc))
        vrc = i_waitForOffsetChange(pEvent, uTimeoutMS, puOffset);

    unregisterWaitEvent(pEvent);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/* static */
HRESULT GuestFile::i_setErrorExternal(VirtualBoxBase *pInterface, int guestRc)
{
    AssertPtr(pInterface);
    AssertMsg(RT_FAILURE(guestRc), ("Guest rc does not indicate a failure when setting error\n"));

    return pInterface->setError(VBOX_E_IPRT_ERROR, GuestFile::i_guestErrorToString(guestRc).c_str());
}

int GuestFile::i_setFileStatus(FileStatus_T fileStatus, int fileRc)
{
    LogFlowThisFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowThisFunc(("oldStatus=%RU32, newStatus=%RU32, fileRc=%Rrc\n",
                     mData.mStatus, fileStatus, fileRc));

#ifdef VBOX_STRICT
    if (fileStatus == FileStatus_Error)
    {
        AssertMsg(RT_FAILURE(fileRc), ("Guest rc must be an error (%Rrc)\n", fileRc));
    }
    else
        AssertMsg(RT_SUCCESS(fileRc), ("Guest rc must not be an error (%Rrc)\n", fileRc));
#endif

    if (mData.mStatus != fileStatus)
    {
        mData.mStatus    = fileStatus;
        mData.mLastError = fileRc;

        ComObjPtr<VirtualBoxErrorInfo> errorInfo;
        HRESULT hr = errorInfo.createObject();
        ComAssertComRC(hr);
        if (RT_FAILURE(fileRc))
        {
            hr = errorInfo->initEx(VBOX_E_IPRT_ERROR, fileRc,
                                   COM_IIDOF(IGuestFile), getComponentName(),
                                   i_guestErrorToString(fileRc));
            ComAssertComRC(hr);
        }

        alock.release(); /* Release lock before firing off event. */

        fireGuestFileStateChangedEvent(mEventSource, mSession,
                                       this, fileStatus, errorInfo);
    }

    return VINF_SUCCESS;
}

int GuestFile::i_waitForOffsetChange(GuestWaitEvent *pEvent,
                                     uint32_t uTimeoutMS, uint64_t *puOffset)
{
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);

    VBoxEventType_T evtType;
    ComPtr<IEvent> pIEvent;
    int vrc = waitForEvent(pEvent, uTimeoutMS,
                           &evtType, pIEvent.asOutParam());
    if (RT_SUCCESS(vrc))
    {
        if (evtType == VBoxEventType_OnGuestFileOffsetChanged)
        {
            if (puOffset)
            {
                ComPtr<IGuestFileOffsetChangedEvent> pFileEvent = pIEvent;
                Assert(!pFileEvent.isNull());

                HRESULT hr = pFileEvent->COMGETTER(Offset)((LONG64*)puOffset);
                ComAssertComRC(hr);
            }
        }
        else
            vrc = VWRN_GSTCTL_OBJECTSTATE_CHANGED;
    }

    return vrc;
}

int GuestFile::i_waitForRead(GuestWaitEvent *pEvent, uint32_t uTimeoutMS,
                             void *pvData, size_t cbData, uint32_t *pcbRead)
{
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);

    VBoxEventType_T evtType;
    ComPtr<IEvent> pIEvent;
    int vrc = waitForEvent(pEvent, uTimeoutMS,
                           &evtType, pIEvent.asOutParam());
    if (RT_SUCCESS(vrc))
    {
        if (evtType == VBoxEventType_OnGuestFileRead)
        {
            ComPtr<IGuestFileReadEvent> pFileEvent = pIEvent;
            Assert(!pFileEvent.isNull());

            HRESULT hr;
            if (pvData)
            {
                com::SafeArray <BYTE> data;
                hr = pFileEvent->COMGETTER(Data)(ComSafeArrayAsOutParam(data));
                ComAssertComRC(hr);
                size_t cbRead = data.size();
                if (   cbRead
                    && cbRead <= cbData)
                {
                    memcpy(pvData, data.raw(), data.size());
                }
                else
                    vrc = VERR_BUFFER_OVERFLOW;
            }
            if (pcbRead)
            {
                hr = pFileEvent->COMGETTER(Processed)((ULONG*)pcbRead);
                ComAssertComRC(hr);
            }
        }
        else
            vrc = VWRN_GSTCTL_OBJECTSTATE_CHANGED;
    }

    return vrc;
}

int GuestFile::i_waitForStatusChange(GuestWaitEvent *pEvent, uint32_t uTimeoutMS,
                                     FileStatus_T *pFileStatus, int *pGuestRc)
{
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);
    /* pFileStatus is optional. */

    VBoxEventType_T evtType;
    ComPtr<IEvent> pIEvent;
    int vrc = waitForEvent(pEvent, uTimeoutMS,
                           &evtType, pIEvent.asOutParam());
    if (RT_SUCCESS(vrc))
    {
        Assert(evtType == VBoxEventType_OnGuestFileStateChanged);
        ComPtr<IGuestFileStateChangedEvent> pFileEvent = pIEvent;
        Assert(!pFileEvent.isNull());

        HRESULT hr;
        if (pFileStatus)
        {
            hr = pFileEvent->COMGETTER(Status)(pFileStatus);
            ComAssertComRC(hr);
        }

        ComPtr<IVirtualBoxErrorInfo> errorInfo;
        hr = pFileEvent->COMGETTER(Error)(errorInfo.asOutParam());
        ComAssertComRC(hr);

        LONG lGuestRc;
        hr = errorInfo->COMGETTER(ResultDetail)(&lGuestRc);
        ComAssertComRC(hr);

        LogFlowThisFunc(("resultDetail=%RI32 (%Rrc)\n",
                         lGuestRc, lGuestRc));

        if (RT_FAILURE((int)lGuestRc))
            vrc = VERR_GSTCTL_GUEST_ERROR;

        if (pGuestRc)
            *pGuestRc = (int)lGuestRc;
    }

    return vrc;
}

int GuestFile::i_waitForWrite(GuestWaitEvent *pEvent,
                              uint32_t uTimeoutMS, uint32_t *pcbWritten)
{
    AssertPtrReturn(pEvent, VERR_INVALID_POINTER);

    VBoxEventType_T evtType;
    ComPtr<IEvent> pIEvent;
    int vrc = waitForEvent(pEvent, uTimeoutMS,
                           &evtType, pIEvent.asOutParam());
    if (RT_SUCCESS(vrc))
    {
        if (evtType == VBoxEventType_OnGuestFileWrite)
        {
            if (pcbWritten)
            {
                ComPtr<IGuestFileWriteEvent> pFileEvent = pIEvent;
                Assert(!pFileEvent.isNull());

                HRESULT hr = pFileEvent->COMGETTER(Processed)((ULONG*)pcbWritten);
                ComAssertComRC(hr);
            }
        }
        else
            vrc = VWRN_GSTCTL_OBJECTSTATE_CHANGED;
    }

    return vrc;
}

int GuestFile::i_writeData(uint32_t uTimeoutMS, void *pvData, uint32_t cbData,
                           uint32_t *pcbWritten)
{
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData, VERR_INVALID_PARAMETER);

    LogFlowThisFunc(("uTimeoutMS=%RU32, pvData=%p, cbData=%zu\n",
                     uTimeoutMS, pvData, cbData));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    int vrc;

    GuestWaitEvent *pEvent = NULL;
    GuestEventTypes eventTypes;
    try
    {
        eventTypes.push_back(VBoxEventType_OnGuestFileStateChanged);
        eventTypes.push_back(VBoxEventType_OnGuestFileWrite);

        vrc = registerWaitEvent(eventTypes, &pEvent);
    }
    catch (std::bad_alloc)
    {
        vrc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(vrc))
        return vrc;

    /* Prepare HGCM call. */
    VBOXHGCMSVCPARM paParms[8];
    int i = 0;
    paParms[i++].setUInt32(pEvent->ContextID());
    paParms[i++].setUInt32(mData.mID /* File handle */);
    paParms[i++].setUInt32(cbData /* Size (in bytes) to write */);
    paParms[i++].setPointer(pvData, cbData);

    alock.release(); /* Drop write lock before sending. */

    vrc = sendCommand(HOST_FILE_WRITE, i, paParms);
    if (RT_SUCCESS(vrc))
    {
        uint32_t cbWritten = 0;
        vrc = i_waitForWrite(pEvent, uTimeoutMS, &cbWritten);
        if (RT_SUCCESS(vrc))
        {
            LogFlowThisFunc(("cbWritten=%RU32\n", cbWritten));
            if (cbWritten)
                *pcbWritten = cbWritten;
        }
    }

    unregisterWaitEvent(pEvent);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

int GuestFile::i_writeDataAt(uint64_t uOffset, uint32_t uTimeoutMS,
                             void *pvData, uint32_t cbData, uint32_t *pcbWritten)
{
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData, VERR_INVALID_PARAMETER);

    LogFlowThisFunc(("uOffset=%RU64, uTimeoutMS=%RU32, pvData=%p, cbData=%zu\n",
                     uOffset, uTimeoutMS, pvData, cbData));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    int vrc;

    GuestWaitEvent *pEvent = NULL;
    GuestEventTypes eventTypes;
    try
    {
        eventTypes.push_back(VBoxEventType_OnGuestFileStateChanged);
        eventTypes.push_back(VBoxEventType_OnGuestFileWrite);

        vrc = registerWaitEvent(eventTypes, &pEvent);
    }
    catch (std::bad_alloc)
    {
        vrc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(vrc))
        return vrc;

    /* Prepare HGCM call. */
    VBOXHGCMSVCPARM paParms[8];
    int i = 0;
    paParms[i++].setUInt32(pEvent->ContextID());
    paParms[i++].setUInt32(mData.mID /* File handle */);
    paParms[i++].setUInt64(uOffset /* Offset where to starting writing */);
    paParms[i++].setUInt32(cbData /* Size (in bytes) to write */);
    paParms[i++].setPointer(pvData, cbData);

    alock.release(); /* Drop write lock before sending. */

    vrc = sendCommand(HOST_FILE_WRITE_AT, i, paParms);
    if (RT_SUCCESS(vrc))
    {
        uint32_t cbWritten = 0;
        vrc = i_waitForWrite(pEvent, uTimeoutMS, &cbWritten);
        if (RT_SUCCESS(vrc))
        {
            LogFlowThisFunc(("cbWritten=%RU32\n", cbWritten));
            if (cbWritten)
                *pcbWritten = cbWritten;
        }
    }

    unregisterWaitEvent(pEvent);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

// Wrapped IGuestFile methods
/////////////////////////////////////////////////////////////////////////////
HRESULT GuestFile::close()
{
    LogFlowThisFuncEnter();

    /* Close file on guest. */
    int guestRc;
    int rc = i_closeFile(&guestRc);
    /* On failure don't return here, instead do all the cleanup
     * work first and then return an error. */

    AssertPtr(mSession);
    int rc2 = mSession->i_fileRemoveFromList(this);
    if (RT_SUCCESS(rc))
        rc = rc2;

    if (RT_FAILURE(rc))
    {
        if (rc == VERR_GSTCTL_GUEST_ERROR)
            return GuestFile::i_setErrorExternal(this, guestRc);

        return setError(VBOX_E_IPRT_ERROR,
                        tr("Closing guest file failed with %Rrc\n"), rc);
    }

    LogFlowThisFunc(("Returning rc=%Rrc\n", rc));
    return S_OK;
}

HRESULT GuestFile::queryInfo(ComPtr<IFsObjInfo> &aObjInfo)
{
    RT_NOREF(aObjInfo);
    ReturnComNotImplemented();
}

HRESULT GuestFile::querySize(LONG64 *aSize)
{
    RT_NOREF(aSize);
    ReturnComNotImplemented();
}

HRESULT GuestFile::read(ULONG aToRead, ULONG aTimeoutMS, std::vector<BYTE> &aData)
{
    if (aToRead == 0)
        return setError(E_INVALIDARG, tr("The size to read is zero"));

    aData.resize(aToRead);

    HRESULT hr = S_OK;

    uint32_t cbRead;
    int vrc = i_readData(aToRead, aTimeoutMS,
                         &aData.front(), aToRead, &cbRead);

    if (RT_SUCCESS(vrc))
    {
        if (aData.size() != cbRead)
            aData.resize(cbRead);
    }
    else
    {
        aData.resize(0);

        switch (vrc)
        {
            default:
                hr = setError(VBOX_E_IPRT_ERROR,
                              tr("Reading from file \"%s\" failed: %Rrc"),
                              mData.mOpenInfo.mFileName.c_str(), vrc);
                break;
        }
    }

    LogFlowFuncLeaveRC(vrc);
    return hr;
}
HRESULT GuestFile::readAt(LONG64 aOffset, ULONG aToRead, ULONG aTimeoutMS, std::vector<BYTE> &aData)

{
    if (aToRead == 0)
        return setError(E_INVALIDARG, tr("The size to read is zero"));

    aData.resize(aToRead);

    HRESULT hr = S_OK;

    size_t cbRead;
    int vrc = i_readDataAt(aOffset, aToRead, aTimeoutMS,
                           &aData.front(), aToRead, &cbRead);
    if (RT_SUCCESS(vrc))
    {
        if (aData.size() != cbRead)
            aData.resize(cbRead);
    }
    else
    {
        aData.resize(0);

        switch (vrc)
        {
            default:
                hr = setError(VBOX_E_IPRT_ERROR,
                              tr("Reading from file \"%s\" (at offset %RU64) failed: %Rrc"),
                              mData.mOpenInfo.mFileName.c_str(), aOffset, vrc);
                break;
        }
    }

    LogFlowFuncLeaveRC(vrc);
    return hr;
}

HRESULT GuestFile::seek(LONG64 aOffset, FileSeekOrigin_T aWhence, LONG64 *aNewOffset)
{
    LogFlowThisFuncEnter();

    HRESULT hr = S_OK;

    GUEST_FILE_SEEKTYPE eSeekType;
    switch (aWhence)
    {
        case FileSeekOrigin_Begin:
            eSeekType = GUEST_FILE_SEEKTYPE_BEGIN;
            break;

        case FileSeekOrigin_Current:
            eSeekType = GUEST_FILE_SEEKTYPE_CURRENT;
            break;

        case FileSeekOrigin_End:
            eSeekType = GUEST_FILE_SEEKTYPE_END;
            break;

        default:
            return setError(E_INVALIDARG, tr("Invalid seek type specified"));
            break; /* Never reached. */
    }

    uint64_t uNewOffset;
    int vrc = i_seekAt(aOffset, eSeekType,
                       30 * 1000 /* 30s timeout */, &uNewOffset);
    if (RT_SUCCESS(vrc))
        *aNewOffset = RT_MIN(uNewOffset, (uint64_t)INT64_MAX);
    else
    {
        switch (vrc)
        {
            default:
                hr = setError(VBOX_E_IPRT_ERROR,
                              tr("Seeking file \"%s\" (to offset %RI64) failed: %Rrc"),
                              mData.mOpenInfo.mFileName.c_str(), aOffset, vrc);
                break;
        }
    }

    LogFlowFuncLeaveRC(vrc);
    return hr;
}

HRESULT GuestFile::setACL(const com::Utf8Str &aAcl, ULONG aMode)
{
    RT_NOREF(aAcl, aMode);
    ReturnComNotImplemented();
}

HRESULT GuestFile::setSize(LONG64 aSize)
{
    RT_NOREF(aSize);
    ReturnComNotImplemented();
}

HRESULT GuestFile::write(const std::vector<BYTE> &aData, ULONG aTimeoutMS, ULONG *aWritten)
{
    LogFlowThisFuncEnter();

    HRESULT hr = S_OK;

    uint32_t cbData = (uint32_t)aData.size();
    void *pvData = cbData > 0? (void *)&aData.front(): NULL;
    int vrc = i_writeData(aTimeoutMS, pvData, cbData,
                          (uint32_t*)aWritten);
    if (RT_FAILURE(vrc))
    {
        switch (vrc)
        {
            default:
                hr = setError(VBOX_E_IPRT_ERROR,
                              tr("Writing %zubytes to file \"%s\" failed: %Rrc"),
                              aData.size(), mData.mOpenInfo.mFileName.c_str(), vrc);
                break;
        }
    }

    LogFlowFuncLeaveRC(vrc);
    return hr;
}

HRESULT GuestFile::writeAt(LONG64 aOffset, const std::vector<BYTE> &aData, ULONG aTimeoutMS, ULONG *aWritten)

{
    LogFlowThisFuncEnter();

    HRESULT hr = S_OK;

    uint32_t cbData = (uint32_t)aData.size();
    void *pvData = cbData > 0? (void *)&aData.front(): NULL;
    int vrc = i_writeData(aTimeoutMS, pvData, cbData,
                          (uint32_t*)aWritten);
    if (RT_FAILURE(vrc))
    {
        switch (vrc)
        {
            default:
                hr = setError(VBOX_E_IPRT_ERROR,
                              tr("Writing %zubytes to file \"%s\" (at offset %RU64) failed: %Rrc"),
                              aData.size(), mData.mOpenInfo.mFileName.c_str(), aOffset, vrc);
                break;
        }
    }

    LogFlowFuncLeaveRC(vrc);
    return hr;
}

