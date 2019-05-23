/* $Id: GuestDirectoryImpl.cpp $ */
/** @file
 * VirtualBox Main - Guest directory handling.
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
#define LOG_GROUP LOG_GROUP_GUEST_CONTROL //LOG_GROUP_MAIN_GUESTDIRECTORY
#include "LoggingNew.h"

#ifndef VBOX_WITH_GUEST_CONTROL
# error "VBOX_WITH_GUEST_CONTROL must defined in this file"
#endif
#include "GuestDirectoryImpl.h"
#include "GuestSessionImpl.h"
#include "GuestCtrlImplPrivate.h"

#include "Global.h"
#include "AutoCaller.h"

#include <VBox/com/array.h>


// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

DEFINE_EMPTY_CTOR_DTOR(GuestDirectory)

HRESULT GuestDirectory::FinalConstruct(void)
{
    LogFlowThisFunc(("\n"));
    return BaseFinalConstruct();
}

void GuestDirectory::FinalRelease(void)
{
    LogFlowThisFuncEnter();
    uninit();
    BaseFinalRelease();
    LogFlowThisFuncLeave();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

int GuestDirectory::init(Console *pConsole, GuestSession *pSession,
                         ULONG uDirID, const GuestDirectoryOpenInfo &openInfo)
{
    LogFlowThisFunc(("pConsole=%p, pSession=%p, uDirID=%RU32, strPath=%s, strFilter=%s, uFlags=%x\n",
                     pConsole, pSession, uDirID, openInfo.mPath.c_str(), openInfo.mFilter.c_str(),
                     openInfo.mFlags));

    AssertPtrReturn(pConsole, VERR_INVALID_POINTER);
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);

    /* Enclose the state transition NotReady->InInit->Ready. */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    int vrc = bindToSession(pConsole, pSession, uDirID /* Object ID */);
    if (RT_SUCCESS(vrc))
    {
        mSession = pSession;

        mData.mID = uDirID;
        mData.mOpenInfo = openInfo;
    }

    if (RT_SUCCESS(vrc))
    {
        /* Start the directory process on the guest. */
        GuestProcessStartupInfo procInfo;
        procInfo.mName      = Utf8StrFmt(tr("Reading directory \"%s\""), openInfo.mPath.c_str());
        procInfo.mTimeoutMS = 5 * 60 * 1000; /* 5 minutes timeout. */
        procInfo.mFlags     = ProcessCreateFlag_WaitForStdOut;
        procInfo.mExecutable= Utf8Str(VBOXSERVICE_TOOL_LS);

        procInfo.mArguments.push_back(procInfo.mExecutable);
        procInfo.mArguments.push_back(Utf8Str("--machinereadable"));
        /* We want the long output format which contains all the object details. */
        procInfo.mArguments.push_back(Utf8Str("-l"));
#if 0 /* Flags are not supported yet. */
        if (uFlags & DirectoryOpenFlag_NoSymlinks)
            procInfo.mArguments.push_back(Utf8Str("--nosymlinks")); /** @todo What does GNU here? */
#endif
        /** @todo Recursion support? */
        procInfo.mArguments.push_back(openInfo.mPath); /* The directory we want to open. */

        /*
         * Start the process asynchronously and keep it around so that we can use
         * it later in subsequent read() calls.
         * Note: No guest rc available because operation is asynchronous.
         */
        vrc = mData.mProcessTool.Init(mSession, procInfo,
                                      true /* Async */, NULL /* Guest rc */);
    }

    if (RT_SUCCESS(vrc))
    {
        /* Confirm a successful initialization when it's the case. */
        autoInitSpan.setSucceeded();
        return vrc;
    }
    else
        autoInitSpan.setFailed();

    return vrc;
}

/**
 * Uninitializes the instance.
 * Called from FinalRelease().
 */
void GuestDirectory::uninit(void)
{
    LogFlowThisFuncEnter();

    /* Enclose the state transition Ready->InUninit->NotReady. */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    LogFlowThisFuncLeave();
}

// implementation of private wrapped getters/setters for attributes
/////////////////////////////////////////////////////////////////////////////

HRESULT GuestDirectory::getDirectoryName(com::Utf8Str &aDirectoryName)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aDirectoryName = mData.mOpenInfo.mPath;

    return S_OK;
}

HRESULT GuestDirectory::getFilter(com::Utf8Str &aFilter)
{
    LogFlowThisFuncEnter();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aFilter = mData.mOpenInfo.mFilter;

    return S_OK;
}

// private methods
/////////////////////////////////////////////////////////////////////////////

int GuestDirectory::i_callbackDispatcher(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCb)
{
    AssertPtrReturn(pCbCtx, VERR_INVALID_POINTER);
    AssertPtrReturn(pSvcCb, VERR_INVALID_POINTER);

    LogFlowThisFunc(("strPath=%s, uContextID=%RU32, uFunction=%RU32, pSvcCb=%p\n",
                     mData.mOpenInfo.mPath.c_str(), pCbCtx->uContextID, pCbCtx->uFunction, pSvcCb));

    int vrc;
    switch (pCbCtx->uFunction)
    {
        case GUEST_DIR_NOTIFY:
        {
            int idx = 1; /* Current parameter index. */
            CALLBACKDATA_DIR_NOTIFY dataCb;
            /* pSvcCb->mpaParms[0] always contains the context ID. */
            pSvcCb->mpaParms[idx++].getUInt32(&dataCb.uType);
            pSvcCb->mpaParms[idx++].getUInt32(&dataCb.rc);

            LogFlowFunc(("uType=%RU32, vrcGguest=%Rrc\n", dataCb.uType, (int)dataCb.rc));

            switch (dataCb.uType)
            {
                /* Nothing here yet, nothing to dispatch further. */

                default:
                    vrc = VERR_NOT_SUPPORTED;
                    break;
            }
            break;
        }

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

/* static */
Utf8Str GuestDirectory::i_guestErrorToString(int guestRc)
{
    Utf8Str strError;

    /** @todo pData->u32Flags: int vs. uint32 -- IPRT errors are *negative* !!! */
    switch (guestRc)
    {
        case VERR_DIR_NOT_EMPTY:
            strError += Utf8StrFmt("Directoy is not empty");
            break;

        default:
            strError += Utf8StrFmt("%Rrc", guestRc);
            break;
    }

    return strError;
}

/**
 * Called by IGuestSession right before this directory gets
 * removed from the public directory list.
 */
int GuestDirectory::i_onRemove(void)
{
    LogFlowThisFuncEnter();

    int vrc = VINF_SUCCESS;

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/* static */
HRESULT GuestDirectory::i_setErrorExternal(VirtualBoxBase *pInterface, int guestRc)
{
    AssertPtr(pInterface);
    AssertMsg(RT_FAILURE(guestRc), ("Guest rc does not indicate a failure when setting error\n"));

    return pInterface->setError(VBOX_E_IPRT_ERROR, GuestDirectory::i_guestErrorToString(guestRc).c_str());
}

// implementation of public methods
/////////////////////////////////////////////////////////////////////////////
HRESULT GuestDirectory::close()
{
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    HRESULT hr = S_OK;

    int guestRc;
    int rc = mData.mProcessTool.i_terminate(30 * 1000, &guestRc);
    if (RT_FAILURE(rc))
    {
        switch (rc)
        {
            case VERR_GSTCTL_GUEST_ERROR:
                hr = GuestProcess::i_setErrorExternal(this, guestRc);
                break;

            case VERR_NOT_SUPPORTED:
                /* Silently skip old Guest Additions which do not support killing the
                 * the guest directory handling process. */
                break;

            default:
                hr = setError(VBOX_E_IPRT_ERROR,
                              tr("Terminating open guest directory \"%s\" failed: %Rrc"),
                              mData.mOpenInfo.mPath.c_str(), rc);
                break;
        }
    }

    AssertPtr(mSession);
    int rc2 = mSession->i_directoryRemoveFromList(this);
    if (RT_SUCCESS(rc))
        rc = rc2;

    LogFlowThisFunc(("Returning rc=%Rrc\n", rc));
    return hr;
}

HRESULT GuestDirectory::read(ComPtr<IFsObjInfo> &aObjInfo)
{
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    GuestProcessStreamBlock curBlock;
    int guestRc;

    int rc = mData.mProcessTool.i_waitEx(GUESTPROCESSTOOL_FLAG_STDOUT_BLOCK,
                                         &curBlock, &guestRc);

    /*
     * Note: The guest process can still be around to serve the next
     *       upcoming stream block next time.
     */
    if (   RT_SUCCESS(rc)
        && !mData.mProcessTool.i_isRunning())
    {
        rc = mData.mProcessTool.i_terminatedOk();
    }

    if (RT_SUCCESS(rc))
    {
        if (curBlock.GetCount()) /* Did we get content? */
        {
            GuestFsObjData objData;
            rc = objData.FromLs(curBlock);
            if (RT_FAILURE(rc))
                rc = VERR_PATH_NOT_FOUND;

            if (RT_SUCCESS(rc))
            {
                /* Create the object. */
                ComObjPtr<GuestFsObjInfo> pFsObjInfo;
                HRESULT hr2 = pFsObjInfo.createObject();
                if (FAILED(hr2))
                    rc = VERR_COM_UNEXPECTED;

                if (RT_SUCCESS(rc))
                    rc = pFsObjInfo->init(objData);

                if (RT_SUCCESS(rc))
                {
                    /* Return info object to the caller. */
                    hr2 = pFsObjInfo.queryInterfaceTo(aObjInfo.asOutParam());
                    if (FAILED(hr2))
                        rc = VERR_COM_UNEXPECTED;
                }
            }
        }
        else
        {
            /* Nothing to read anymore. Tell the caller. */
            rc = VERR_NO_MORE_FILES;
        }
    }

    HRESULT hr = S_OK;

    if (RT_FAILURE(rc)) /** @todo Add more errors here. */
    {
        switch (rc)
        {
            case VERR_GSTCTL_GUEST_ERROR:
                hr = GuestProcess::i_setErrorExternal(this, guestRc);
                break;

            case VWRN_GSTCTL_PROCESS_EXIT_CODE:
                hr = setError(VBOX_E_IPRT_ERROR, tr("Reading directory \"%s\" failed: %Rrc"),
                              mData.mOpenInfo.mPath.c_str(), mData.mProcessTool.i_getRc());
                break;

            case VERR_PATH_NOT_FOUND:
                hr = setError(VBOX_E_IPRT_ERROR, tr("Reading directory \"%s\" failed: Path not found"),
                              mData.mOpenInfo.mPath.c_str());
                break;

            case VERR_NO_MORE_FILES:
                /* See SDK reference. */
                hr = setError(VBOX_E_OBJECT_NOT_FOUND, tr("No more entries for directory \"%s\""),
                              mData.mOpenInfo.mPath.c_str());
                break;

            default:
                hr = setError(VBOX_E_IPRT_ERROR, tr("Error while reading directory \"%s\": %Rrc\n"),
                              mData.mOpenInfo.mPath.c_str(), rc);
                break;
        }
    }

    LogFlowThisFunc(("Returning rc=%Rrc\n", rc));
    return hr;
}

