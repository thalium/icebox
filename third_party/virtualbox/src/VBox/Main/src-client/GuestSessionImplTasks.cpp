/* $Id: GuestSessionImplTasks.cpp $ */
/** @file
 * VirtualBox Main - Guest session tasks.
 */

/*
 * Copyright (C) 2012-2016 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_GUEST_CONTROL //LOG_GROUP_MAIN_GUESTSESSION
#include "LoggingNew.h"

#include "GuestImpl.h"
#ifndef VBOX_WITH_GUEST_CONTROL
# error "VBOX_WITH_GUEST_CONTROL must defined in this file"
#endif
#include "GuestSessionImpl.h"
#include "GuestCtrlImplPrivate.h"

#include "Global.h"
#include "AutoCaller.h"
#include "ConsoleImpl.h"
#include "ProgressImpl.h"

#include <memory> /* For auto_ptr. */

#include <iprt/env.h>
#include <iprt/file.h> /* For CopyTo/From. */
#include <iprt/path.h>


/*********************************************************************************************************************************
*   Defines                                                                                                                      *
*********************************************************************************************************************************/

/**
 * Update file flags.
 */
#define UPDATEFILE_FLAG_NONE                0
/** Copy over the file from host to the
 *  guest. */
#define UPDATEFILE_FLAG_COPY_FROM_ISO       RT_BIT(0)
/** Execute file on the guest after it has
 *  been successfully transfered. */
#define UPDATEFILE_FLAG_EXECUTE             RT_BIT(7)
/** File is optional, does not have to be
 *  existent on the .ISO. */
#define UPDATEFILE_FLAG_OPTIONAL            RT_BIT(8)


// session task classes
/////////////////////////////////////////////////////////////////////////////

GuestSessionTask::GuestSessionTask(GuestSession *pSession)
    : ThreadTask("GenericGuestSessionTask")
{
    mSession = pSession;
}

GuestSessionTask::~GuestSessionTask(void)
{
}

HRESULT GuestSessionTask::createAndSetProgressObject()
{
    LogFlowThisFunc(("Task Description = %s, pTask=%p\n", mDesc.c_str(), this));

    ComObjPtr<Progress> pProgress;
    HRESULT hr = S_OK;
    /* Create the progress object. */
    hr = pProgress.createObject();
    if (FAILED(hr))
        return VERR_COM_UNEXPECTED;

    hr = pProgress->init(static_cast<IGuestSession*>(mSession),
                         Bstr(mDesc).raw(),
                         TRUE /* aCancelable */);
    if (FAILED(hr))
        return VERR_COM_UNEXPECTED;

    mProgress = pProgress;

    LogFlowFuncLeave();

    return hr;
}

int GuestSessionTask::RunAsync(const Utf8Str &strDesc, ComObjPtr<Progress> &pProgress)
{
    LogFlowThisFunc(("strDesc=%s\n", strDesc.c_str()));

    mDesc = strDesc;
    mProgress = pProgress;
    HRESULT hrc = createThreadWithType(RTTHREADTYPE_MAIN_HEAVY_WORKER);

    LogFlowThisFunc(("Returning hrc=%Rhrc\n", hrc));
    return Global::vboxStatusCodeToCOM(hrc);
}


int GuestSessionTask::getGuestProperty(const ComObjPtr<Guest> &pGuest,
                                       const Utf8Str &strPath, Utf8Str &strValue)
{
    ComObjPtr<Console> pConsole = pGuest->i_getConsole();
    const ComPtr<IMachine> pMachine = pConsole->i_machine();

    Assert(!pMachine.isNull());
    Bstr strTemp, strFlags;
    LONG64 i64Timestamp;
    HRESULT hr = pMachine->GetGuestProperty(Bstr(strPath).raw(),
                                            strTemp.asOutParam(),
                                            &i64Timestamp, strFlags.asOutParam());
    if (SUCCEEDED(hr))
    {
        strValue = strTemp;
        return VINF_SUCCESS;
    }
    return VERR_NOT_FOUND;
}

int GuestSessionTask::setProgress(ULONG uPercent)
{
    if (mProgress.isNull()) /* Progress is optional. */
        return VINF_SUCCESS;

    BOOL fCanceled;
    if (   SUCCEEDED(mProgress->COMGETTER(Canceled(&fCanceled)))
        && fCanceled)
        return VERR_CANCELLED;
    BOOL fCompleted;
    if (   SUCCEEDED(mProgress->COMGETTER(Completed(&fCompleted)))
        && fCompleted)
    {
        AssertMsgFailed(("Setting value of an already completed progress\n"));
        return VINF_SUCCESS;
    }
    HRESULT hr = mProgress->SetCurrentOperationProgress(uPercent);
    if (FAILED(hr))
        return VERR_COM_UNEXPECTED;

    return VINF_SUCCESS;
}

int GuestSessionTask::setProgressSuccess(void)
{
    if (mProgress.isNull()) /* Progress is optional. */
        return VINF_SUCCESS;

    BOOL fCanceled;
    BOOL fCompleted;
    if (   SUCCEEDED(mProgress->COMGETTER(Canceled(&fCanceled)))
        && !fCanceled
        && SUCCEEDED(mProgress->COMGETTER(Completed(&fCompleted)))
        && !fCompleted)
    {
        HRESULT hr = mProgress->i_notifyComplete(S_OK);
        if (FAILED(hr))
            return VERR_COM_UNEXPECTED; /** @todo Find a better rc. */
    }

    return VINF_SUCCESS;
}

HRESULT GuestSessionTask::setProgressErrorMsg(HRESULT hr, const Utf8Str &strMsg)
{
    LogFlowFunc(("hr=%Rhrc, strMsg=%s\n",
                 hr, strMsg.c_str()));

    if (mProgress.isNull()) /* Progress is optional. */
        return hr; /* Return original rc. */

    BOOL fCanceled;
    BOOL fCompleted;
    if (   SUCCEEDED(mProgress->COMGETTER(Canceled(&fCanceled)))
        && !fCanceled
        && SUCCEEDED(mProgress->COMGETTER(Completed(&fCompleted)))
        && !fCompleted)
    {
        HRESULT hr2 = mProgress->i_notifyComplete(hr,
                                                  COM_IIDOF(IGuestSession),
                                                  GuestSession::getStaticComponentName(),
                                                  strMsg.c_str());
        if (FAILED(hr2))
            return hr2;
    }
    return hr; /* Return original rc. */
}

SessionTaskOpen::SessionTaskOpen(GuestSession *pSession,
                                 uint32_t uFlags,
                                 uint32_t uTimeoutMS)
                                 : GuestSessionTask(pSession),
                                   mFlags(uFlags),
                                   mTimeoutMS(uTimeoutMS)
{
    m_strTaskName = "gctlSesOpen";
}

SessionTaskOpen::~SessionTaskOpen(void)
{

}

int SessionTaskOpen::Run(void)
{
    LogFlowThisFuncEnter();

    ComObjPtr<GuestSession> pSession = mSession;
    Assert(!pSession.isNull());

    AutoCaller autoCaller(pSession);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    int vrc = pSession->i_startSessionInternal(NULL /*pvrcGuest*/);
    /* Nothing to do here anymore. */

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

SessionTaskCopyTo::SessionTaskCopyTo(GuestSession *pSession,
                                     const Utf8Str &strSource, const Utf8Str &strDest, uint32_t uFlags)
                                     : GuestSessionTask(pSession),
                                       mSource(strSource),
                                       mSourceFile(NULL),
                                       mSourceOffset(0),
                                       mSourceSize(0),
                                       mDest(strDest)
{
    mCopyFileFlags = uFlags;
    m_strTaskName = "gctlCpyTo";
}

/** @todo Merge this and the above call and let the above call do the open/close file handling so that the
 *        inner code only has to deal with file handles. No time now ... */
SessionTaskCopyTo::SessionTaskCopyTo(GuestSession *pSession,
                                     PRTFILE pSourceFile, size_t cbSourceOffset, uint64_t cbSourceSize,
                                     const Utf8Str &strDest, uint32_t uFlags)
                                     : GuestSessionTask(pSession)
{
    mSourceFile    = pSourceFile;
    mSourceOffset  = cbSourceOffset;
    mSourceSize    = cbSourceSize;
    mDest          = strDest;
    mCopyFileFlags = uFlags;
    m_strTaskName = "gctlCpyTo";
}

SessionTaskCopyTo::~SessionTaskCopyTo(void)
{

}

int SessionTaskCopyTo::Run(void)
{
    LogFlowThisFuncEnter();

    ComObjPtr<GuestSession> pSession = mSession;
    Assert(!pSession.isNull());

    AutoCaller autoCaller(pSession);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    if (mCopyFileFlags)
    {
        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                            Utf8StrFmt(GuestSession::tr("Copy flags (%#x) not implemented yet"),
                            mCopyFileFlags));
        return VERR_INVALID_PARAMETER;
    }

    int rc;

    RTMSINTERVAL msTimeout = 30 * 1000; /** @todo 30s timeout for all actions. Make this configurable? */

    RTFILE fileLocal;
    PRTFILE pFile = &fileLocal;

    if (!mSourceFile)
    {
        /* Does our source file exist? */
        if (!RTFileExists(mSource.c_str()))
        {
            rc = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                     Utf8StrFmt(GuestSession::tr("Source file \"%s\" does not exist or is not a file"),
                                                mSource.c_str()));
        }
        else
        {
            rc = RTFileOpen(pFile, mSource.c_str(),
                            RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_WRITE);
            if (RT_FAILURE(rc))
            {
                rc = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                         Utf8StrFmt(GuestSession::tr("Could not open source file \"%s\" for reading: %Rrc"),
                                                    mSource.c_str(), rc));
            }
            else
            {
                rc = RTFileGetSize(*pFile, &mSourceSize);
                if (RT_FAILURE(rc))
                {
                    setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                        Utf8StrFmt(GuestSession::tr("Could not query file size of \"%s\": %Rrc"),
                                                   mSource.c_str(), rc));
                }
            }
        }
    }
    else
    {
        rc = VINF_SUCCESS;
        pFile = mSourceFile;
        /* Size + offset are optional. */
    }

    /*
     * Query information about our destination first.
     */
    int guestRc = VERR_IPE_UNINITIALIZED_STATUS;
    if (RT_SUCCESS(rc))
    {
        GuestFsObjData objData;
        rc = pSession->i_directoryQueryInfoInternal(mDest, true /* fFollowSymlinks */, objData, &guestRc);
        if (RT_SUCCESS(rc))
        {
            mDest = Utf8StrFmt("%s/%s", mDest.c_str(), RTPathFilename(mSource.c_str()));
        }
        else if (rc == VERR_NOT_A_DIRECTORY)
        {
            rc = VINF_SUCCESS;
        }
    }

    /** @todo Implement sparse file support? */

    /*
     * Start the actual copying process by cat'ing the source file to the
     * destination file on the guest.
     */
    GuestProcessStartupInfo procInfo;
    procInfo.mExecutable = Utf8Str(VBOXSERVICE_TOOL_CAT);
    procInfo.mFlags      = ProcessCreateFlag_Hidden;

    /* Set arguments.*/
    procInfo.mArguments.push_back(procInfo.mExecutable);                     /* Set argv0. */
    procInfo.mArguments.push_back(Utf8StrFmt("--output=%s", mDest.c_str())); /** @todo Do we need path conversion? */

    /* Startup process. */
    ComObjPtr<GuestProcess> pProcess;
    if (RT_SUCCESS(rc))
        rc = pSession->i_processCreateExInternal(procInfo, pProcess);
    if (RT_SUCCESS(rc))
    {
        Assert(!pProcess.isNull());
        rc = pProcess->i_startProcess(msTimeout, &guestRc);
    }

    if (RT_FAILURE(rc))
    {
        switch (rc)
        {
            case VERR_GSTCTL_GUEST_ERROR:
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    GuestProcess::i_guestErrorToString(guestRc));
                break;

            default:
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(GuestSession::tr(
                                    "Error while creating guest process for copying file \"%s\" from guest to host: %Rrc"),
                                    mSource.c_str(), rc));
                break;
        }
    }

    if (RT_SUCCESS(rc))
    {
        ProcessWaitResult_T waitRes;
        BYTE byBuf[_64K];

        BOOL fCanceled = FALSE;
        uint64_t cbWrittenTotal = 0;
        uint64_t cbToRead = mSourceSize;

        for (;;)
        {
            rc = pProcess->i_waitFor(ProcessWaitForFlag_StdIn, msTimeout, waitRes, &guestRc);
            if (   RT_FAILURE(rc)
                || (   waitRes != ProcessWaitResult_StdIn
                    && waitRes != ProcessWaitResult_WaitFlagNotSupported))
            {
                break;
            }

            /* If the guest does not support waiting for stdin, we now yield in
             * order to reduce the CPU load due to busy waiting. */
            if (waitRes == ProcessWaitResult_WaitFlagNotSupported)
                RTThreadYield(); /* Optional, don't check rc. */

            size_t cbRead = 0;
            if (mSourceSize) /* If we have nothing to write, take a shortcut. */
            {
                /** @todo Not very efficient, but works for now. */
                rc = RTFileSeek(*pFile, mSourceOffset + cbWrittenTotal,
                                RTFILE_SEEK_BEGIN, NULL /* poffActual */);
                if (RT_SUCCESS(rc))
                {
                    rc = RTFileRead(*pFile, (uint8_t*)byBuf,
                                    RT_MIN((size_t)cbToRead, sizeof(byBuf)), &cbRead);
                    /*
                     * Some other error occured? There might be a chance that RTFileRead
                     * could not resolve/map the native error code to an IPRT code, so just
                     * print a generic error.
                     */
                    if (RT_FAILURE(rc))
                    {
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                            Utf8StrFmt(GuestSession::tr("Could not read from host file \"%s\" (%Rrc)"),
                                                       mSource.c_str(), rc));
                        break;
                    }
                }
                else
                {
                    setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                        Utf8StrFmt(GuestSession::tr("Seeking host file \"%s\" to offset %RU64 failed: %Rrc"),
                                                   mSource.c_str(), cbWrittenTotal, rc));
                    break;
                }
            }

            uint32_t fFlags = ProcessInputFlag_None;

            /* Did we reach the end of the content we want to transfer (last chunk)? */
            if (   (cbRead < sizeof(byBuf))
                /* Did we reach the last block which is exactly _64K? */
                || (cbToRead - cbRead == 0)
                /* ... or does the user want to cancel? */
                || (   !mProgress.isNull()
                    && SUCCEEDED(mProgress->COMGETTER(Canceled(&fCanceled)))
                    && fCanceled)
               )
            {
                LogFlowThisFunc(("Writing last chunk cbRead=%RU64\n", cbRead));
                fFlags |= ProcessInputFlag_EndOfFile;
            }

            uint32_t cbWritten;
            Assert(sizeof(byBuf) >= cbRead);
            rc = pProcess->i_writeData(0 /* StdIn */, fFlags,
                                       byBuf, cbRead,
                                       msTimeout, &cbWritten, &guestRc);
            if (RT_FAILURE(rc))
            {
                switch (rc)
                {
                    case VERR_GSTCTL_GUEST_ERROR:
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                            GuestProcess::i_guestErrorToString(guestRc));
                        break;

                    default:
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                            Utf8StrFmt(GuestSession::tr("Writing to guest file \"%s\" (offset %RU64) failed: %Rrc"),
                                            mDest.c_str(), cbWrittenTotal, rc));
                        break;
                }

                break;
            }

            /* Only subtract bytes reported written by the guest. */
            Assert(cbToRead >= cbWritten);
            cbToRead -= cbWritten;

            /* Update total bytes written to the guest. */
            cbWrittenTotal += cbWritten;
            Assert(cbWrittenTotal <= mSourceSize);

            LogFlowThisFunc(("rc=%Rrc, cbWritten=%RU32, cbToRead=%RU64, cbWrittenTotal=%RU64, cbFileSize=%RU64\n",
                             rc, cbWritten, cbToRead, cbWrittenTotal, mSourceSize));

            /* Did the user cancel the operation above? */
            if (fCanceled)
                break;

            /* Update the progress.
             * Watch out for division by zero. */
            mSourceSize > 0
                ? rc = setProgress((ULONG)(cbWrittenTotal * 100 / mSourceSize))
                : rc = setProgress(100);
            if (RT_FAILURE(rc))
                break;

            /* End of file reached? */
            if (!cbToRead)
                break;
        } /* for */

        LogFlowThisFunc(("Copy loop ended with rc=%Rrc, cbToRead=%RU64, cbWrittenTotal=%RU64, cbFileSize=%RU64\n",
                         rc, cbToRead, cbWrittenTotal, mSourceSize));

        /*
         * Wait on termination of guest process until it completed all operations.
         */
        if (   !fCanceled
            || RT_SUCCESS(rc))
        {
            rc = pProcess->i_waitFor(ProcessWaitForFlag_Terminate, msTimeout, waitRes, &guestRc);
            if (   RT_FAILURE(rc)
                || waitRes != ProcessWaitResult_Terminate)
            {
                if (RT_FAILURE(rc))
                    setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                        Utf8StrFmt(
                                        GuestSession::tr("Waiting on termination for copying file \"%s\" to guest failed: %Rrc"),
                                        mSource.c_str(), rc));
                else
                {
                    setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                        Utf8StrFmt(GuestSession::tr(
                                                   "Waiting on termination for copying file \"%s\" to guest failed with wait result %ld"),
                                                   mSource.c_str(), waitRes));
                    rc = VERR_GENERAL_FAILURE; /* Fudge. */
                }
            }
        }

        if (RT_SUCCESS(rc))
        {
            /*
             * Newer VBoxService toolbox versions report what went wrong via exit code.
             * So handle this first.
             */
            /** @todo This code sequence is duplicated in CopyFrom...   */
            ProcessStatus_T procStatus = ProcessStatus_TerminatedAbnormally;
            HRESULT hrc = pProcess->COMGETTER(Status(&procStatus));
            if (!SUCCEEDED(hrc))
                procStatus = ProcessStatus_TerminatedAbnormally;

            LONG exitCode = 42424242;
            hrc = pProcess->COMGETTER(ExitCode(&exitCode));
            if (!SUCCEEDED(hrc))
                exitCode = 42424242;

            if (   procStatus != ProcessStatus_TerminatedNormally
                || exitCode != 0)
            {
                LogFlowThisFunc(("procStatus=%d, exitCode=%d\n", procStatus, exitCode));
                if (procStatus == ProcessStatus_TerminatedNormally)
                    rc = GuestProcessTool::i_exitCodeToRc(procInfo, exitCode);
                else
                    rc = VERR_GENERAL_FAILURE;
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(GuestSession::tr("Copying file \"%s\" to guest failed: %Rrc"),
                                               mSource.c_str(), rc));
            }
            /*
             * Even if we succeeded until here make sure to check whether we really transfered
             * everything.
             */
            else if (   mSourceSize > 0
                     && cbWrittenTotal == 0)
            {
                /* If nothing was transfered but the file size was > 0 then "vbox_cat" wasn't able to write
                 * to the destination -> access denied. */
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(GuestSession::tr("Access denied when copying file \"%s\" to guest \"%s\""),
                                               mSource.c_str(), mDest.c_str()));
                rc = VERR_ACCESS_DENIED;
            }
            else if (cbWrittenTotal < mSourceSize)
            {
                /* If we did not copy all let the user know. */
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(GuestSession::tr("Copying file \"%s\" to guest failed (%RU64/%RU64 bytes transfered)"),
                                               mSource.c_str(), cbWrittenTotal, mSourceSize));
                rc = VERR_INTERRUPTED;
            }
            else
                rc = setProgressSuccess();
        }
    } /* processCreateExInteral */

    if (!mSourceFile) /* Only close locally opened files. */
        RTFileClose(*pFile);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

SessionTaskCopyFrom::SessionTaskCopyFrom(GuestSession *pSession,
                                         const Utf8Str &strSource, const Utf8Str &strDest, uint32_t uFlags)
                                         : GuestSessionTask(pSession)
{
    mSource = strSource;
    mDest   = strDest;
    mFlags  = uFlags;
    m_strTaskName = "gctlCpyFrom";
}

SessionTaskCopyFrom::~SessionTaskCopyFrom(void)
{

}

int SessionTaskCopyFrom::Run(void)
{
    LogFlowThisFuncEnter();

    ComObjPtr<GuestSession> pSession = mSession;
    Assert(!pSession.isNull());

    AutoCaller autoCaller(pSession);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    RTMSINTERVAL msTimeout = 30 * 1000; /** @todo 30s timeout for all actions. Make this configurable? */

    /*
     * Note: There will be races between querying file size + reading the guest file's
     *       content because we currently *do not* lock down the guest file when doing the
     *       actual operations.
     ** @todo Use the IGuestFile API for locking down the file on the guest!
     */
    GuestFsObjData objData; int guestRc;
    int rc = pSession->i_fileQueryInfoInternal(Utf8Str(mSource), false /*fFollowSymlinks*/, objData, &guestRc);
    if (RT_FAILURE(rc))
    {
        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                            Utf8StrFmt(GuestSession::tr("Querying guest file information for \"%s\" failed: %Rrc"),
                                       mSource.c_str(), rc));
    }
    else if (objData.mType != FsObjType_File) /* Only single files are supported at the moment. */
    {
        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                            Utf8StrFmt(GuestSession::tr("Object \"%s\" on the guest is not a file"), mSource.c_str()));
        rc = VERR_NOT_A_FILE;
    }

    if (RT_SUCCESS(rc))
    {
        RTFILE fileDest;
        rc = RTFileOpen(&fileDest, mDest.c_str(),
                        RTFILE_O_WRITE | RTFILE_O_OPEN_CREATE | RTFILE_O_DENY_WRITE); /** @todo Use the correct open modes! */
        if (RT_FAILURE(rc))
        {
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(GuestSession::tr("Opening/creating destination file on host \"%s\" failed: %Rrc"),
                                           mDest.c_str(), rc));
        }
        else
        {
            GuestProcessStartupInfo procInfo;
            procInfo.mName = Utf8StrFmt(GuestSession::tr("Copying file \"%s\" from guest to the host to \"%s\" (%RI64 bytes)"),
                                        mSource.c_str(), mDest.c_str(), objData.mObjectSize);
            procInfo.mExecutable = Utf8Str(VBOXSERVICE_TOOL_CAT);
            procInfo.mFlags      = ProcessCreateFlag_Hidden | ProcessCreateFlag_WaitForStdOut;

            /* Set arguments.*/
            procInfo.mArguments.push_back(procInfo.mExecutable); /* Set argv0. */
            procInfo.mArguments.push_back(mSource);              /* Which file to output? */

            /* Startup process. */
            ComObjPtr<GuestProcess> pProcess;
            rc = pSession->i_processCreateExInternal(procInfo, pProcess);
            if (RT_SUCCESS(rc))
                rc = pProcess->i_startProcess(msTimeout, &guestRc);
            if (RT_FAILURE(rc))
            {
                switch (rc)
                {
                    case VERR_GSTCTL_GUEST_ERROR:
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR, GuestProcess::i_guestErrorToString(guestRc));
                        break;

                    default:
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                            Utf8StrFmt(GuestSession::tr(
                                           "Error while creating guest process for copying file \"%s\" from guest to host: %Rrc"),
                                           mSource.c_str(), rc));
                        break;
                }
            }
            else
            {
                ProcessWaitResult_T waitRes;
                BYTE byBuf[_64K];

                BOOL fCanceled = FALSE;
                uint64_t cbWrittenTotal = 0;
                uint64_t cbToRead = objData.mObjectSize;

                for (;;)
                {
                    rc = pProcess->i_waitFor(ProcessWaitForFlag_StdOut, msTimeout, waitRes, &guestRc);
                    if (RT_FAILURE(rc))
                    {
                        switch (rc)
                        {
                            case VERR_GSTCTL_GUEST_ERROR:
                                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                    GuestProcess::i_guestErrorToString(guestRc));
                                break;

                            default:
                                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                    Utf8StrFmt(GuestSession::tr("Error while creating guest process for copying file \"%s\" from guest to host: %Rrc"),
                                                               mSource.c_str(), rc));
                                break;
                        }

                        break;
                    }

                    if (   waitRes == ProcessWaitResult_StdOut
                        || waitRes == ProcessWaitResult_WaitFlagNotSupported)
                    {
                        /* If the guest does not support waiting for stdin, we now yield in
                         * order to reduce the CPU load due to busy waiting. */
                        if (waitRes == ProcessWaitResult_WaitFlagNotSupported)
                            RTThreadYield(); /* Optional, don't check rc. */

                        uint32_t cbRead = 0; /* readData can return with VWRN_GSTCTL_OBJECTSTATE_CHANGED. */
                        rc = pProcess->i_readData(OUTPUT_HANDLE_ID_STDOUT, sizeof(byBuf),
                                                  msTimeout, byBuf, sizeof(byBuf),
                                                  &cbRead, &guestRc);
                        if (RT_FAILURE(rc))
                        {
                            switch (rc)
                            {
                                case VERR_GSTCTL_GUEST_ERROR:
                                    setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                        GuestProcess::i_guestErrorToString(guestRc));
                                    break;

                                default:
                                    setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                        Utf8StrFmt(GuestSession::tr("Reading from guest file \"%s\" (offset %RU64) failed: %Rrc"),
                                                                   mSource.c_str(), cbWrittenTotal, rc));
                                    break;
                            }

                            break;
                        }

                        if (cbRead)
                        {
                            rc = RTFileWrite(fileDest, byBuf, cbRead, NULL /* No partial writes */);
                            if (RT_FAILURE(rc))
                            {
                                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                    Utf8StrFmt(GuestSession::tr("Writing to host file \"%s\" (%RU64 bytes left) failed: %Rrc"),
                                                                mDest.c_str(), cbToRead, rc));
                                break;
                            }

                            /* Only subtract bytes reported written by the guest. */
                            Assert(cbToRead >= cbRead);
                            cbToRead -= cbRead;

                            /* Update total bytes written to the guest. */
                            cbWrittenTotal += cbRead;
                            Assert(cbWrittenTotal <= (uint64_t)objData.mObjectSize);

                            /* Did the user cancel the operation above? */
                            if (   SUCCEEDED(mProgress->COMGETTER(Canceled(&fCanceled)))
                                && fCanceled)
                                break;

                            rc = setProgress((ULONG)(cbWrittenTotal / ((uint64_t)objData.mObjectSize / 100.0)));
                            if (RT_FAILURE(rc))
                                break;
                        }
                    }
                    else
                    {
                        break;
                    }

                } /* for */

                LogFlowThisFunc(("rc=%Rrc, guestrc=%Rrc, waitRes=%ld, cbWrittenTotal=%RU64, cbSize=%RI64, cbToRead=%RU64\n",
                                 rc, guestRc, waitRes, cbWrittenTotal, objData.mObjectSize, cbToRead));

                /*
                 * Wait on termination of guest process until it completed all operations.
                 */
                if (   !fCanceled
                    || RT_SUCCESS(rc))
                {
                    rc = pProcess->i_waitFor(ProcessWaitForFlag_Terminate, msTimeout, waitRes, &guestRc);
                    if (   RT_FAILURE(rc)
                        || waitRes != ProcessWaitResult_Terminate)
                    {
                        if (RT_FAILURE(rc))
                            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                Utf8StrFmt(
                                                GuestSession::tr("Waiting on termination for copying file \"%s\" from guest failed: %Rrc"),
                                                mSource.c_str(), rc));
                        else
                        {
                            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                Utf8StrFmt(GuestSession::tr(
                                                           "Waiting on termination for copying file \"%s\" from guest failed with wait result %ld"),
                                                           mSource.c_str(), waitRes));
                            rc = VERR_GENERAL_FAILURE; /* Fudge. */
                        }
                    }
                }

                if (RT_SUCCESS(rc))
                {
                    /** @todo this code sequence is duplicated in CopyTo   */
                    ProcessStatus_T procStatus = ProcessStatus_TerminatedAbnormally;
                    HRESULT hrc = pProcess->COMGETTER(Status(&procStatus));
                    if (!SUCCEEDED(hrc))
                        procStatus = ProcessStatus_TerminatedAbnormally;

                    LONG exitCode = 42424242;
                    hrc = pProcess->COMGETTER(ExitCode(&exitCode));
                    if (!SUCCEEDED(hrc))
                        exitCode = 42424242;

                    if (   procStatus != ProcessStatus_TerminatedNormally
                        || exitCode != 0)
                    {
                        LogFlowThisFunc(("procStatus=%d, exitCode=%d\n", procStatus, exitCode));
                        if (procStatus == ProcessStatus_TerminatedNormally)
                            rc = GuestProcessTool::i_exitCodeToRc(procInfo, exitCode);
                        else
                            rc = VERR_GENERAL_FAILURE;
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                            Utf8StrFmt(GuestSession::tr("Copying file \"%s\" to host failed: %Rrc"),
                                                       mSource.c_str(), rc));
                    }
                    /*
                     * Even if we succeeded until here make sure to check whether we really transfered
                     * everything.
                     */
                    else if (   objData.mObjectSize > 0
                             && cbWrittenTotal == 0)
                    {
                        /* If nothing was transfered but the file size was > 0 then "vbox_cat" wasn't able to write
                         * to the destination -> access denied. */
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                            Utf8StrFmt(GuestSession::tr("Writing guest file \"%s\" to host to \"%s\" failed: Access denied"),
                                                       mSource.c_str(), mDest.c_str()));
                        rc = VERR_ACCESS_DENIED;
                    }
                    else if (cbWrittenTotal < (uint64_t)objData.mObjectSize)
                    {
                        /* If we did not copy all let the user know. */
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                            Utf8StrFmt(GuestSession::tr("Copying guest file \"%s\" to host to \"%s\" failed (%RU64/%RI64 bytes transfered)"),
                                                       mSource.c_str(), mDest.c_str(), cbWrittenTotal, objData.mObjectSize));
                        rc = VERR_INTERRUPTED;
                    }
                    else
                        rc = setProgressSuccess();
                }
            }

            RTFileClose(fileDest);
        }
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

SessionTaskUpdateAdditions::SessionTaskUpdateAdditions(GuestSession *pSession,
                                                       const Utf8Str &strSource,
                                                       const ProcessArguments &aArguments,
                                                       uint32_t uFlags)
                                                       : GuestSessionTask(pSession)
{
    mSource = strSource;
    mArguments = aArguments;
    mFlags  = uFlags;
    m_strTaskName = "gctlUpGA";
}

SessionTaskUpdateAdditions::~SessionTaskUpdateAdditions(void)
{

}

int SessionTaskUpdateAdditions::i_addProcessArguments(ProcessArguments &aArgumentsDest,
                                                      const ProcessArguments &aArgumentsSource)
{
    int rc = VINF_SUCCESS;

    try
    {
        /* Filter out arguments which already are in the destination to
         * not end up having them specified twice. Not the fastest method on the
         * planet but does the job. */
        ProcessArguments::const_iterator itSource = aArgumentsSource.begin();
        while (itSource != aArgumentsSource.end())
        {
            bool fFound = false;
            ProcessArguments::iterator itDest = aArgumentsDest.begin();
            while (itDest != aArgumentsDest.end())
            {
                if ((*itDest).equalsIgnoreCase((*itSource)))
                {
                    fFound = true;
                    break;
                }
                ++itDest;
            }

            if (!fFound)
                aArgumentsDest.push_back((*itSource));

            ++itSource;
        }
    }
    catch(std::bad_alloc &)
    {
        return VERR_NO_MEMORY;
    }

    return rc;
}

int SessionTaskUpdateAdditions::i_copyFileToGuest(GuestSession *pSession, PRTISOFSFILE pISO,
                                                  Utf8Str const &strFileSource, const Utf8Str &strFileDest,
                                                  bool fOptional, uint32_t *pcbSize)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertPtrReturn(pISO, VERR_INVALID_POINTER);
    /* pcbSize is optional. */

    uint32_t cbOffset;
    size_t cbSize;

    int rc = RTIsoFsGetFileInfo(pISO, strFileSource.c_str(), &cbOffset, &cbSize);
    if (RT_FAILURE(rc))
    {
        if (fOptional)
            return VINF_SUCCESS;

        return rc;
    }

    Assert(cbOffset);
    Assert(cbSize);
    rc = RTFileSeek(pISO->file, cbOffset, RTFILE_SEEK_BEGIN, NULL);

    HRESULT hr = S_OK;
    /* Copy over the Guest Additions file to the guest. */
    if (RT_SUCCESS(rc))
    {
        LogRel(("Copying Guest Additions installer file \"%s\" to \"%s\" on guest ...\n",
                strFileSource.c_str(), strFileDest.c_str()));

        if (RT_SUCCESS(rc))
        {
            SessionTaskCopyTo *pTask = NULL;
            ComObjPtr<Progress> pProgressCopyTo;
            try
            {
                try
                {
                    pTask = new SessionTaskCopyTo(pSession /* GuestSession */,
                                                  &pISO->file, cbOffset, cbSize,
                                                  strFileDest, FileCopyFlag_None);
                }
                catch(...)
                {
                    hr = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                  GuestSession::tr("Failed to create SessionTaskCopyTo object "));
                    throw;
                }

                hr = pTask->Init(Utf8StrFmt(GuestSession::tr("Copying Guest Additions installer file \"%s\" to \"%s\" on guest"),
                                            mSource.c_str(), strFileDest.c_str()));
                if (FAILED(hr))
                {
                    delete pTask;
                    hr = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                  GuestSession::tr("Creating progress object for SessionTaskCopyTo object failed"));
                    throw hr;
                }

                hr = pTask->createThreadWithType(RTTHREADTYPE_MAIN_HEAVY_WORKER);

                if (SUCCEEDED(hr))
                {
                    /* Return progress to the caller. */
                    pProgressCopyTo = pTask->GetProgressObject();
                }
                else
                    hr = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                  GuestSession::tr("Starting thread for updating additions failed "));
            }
            catch(std::bad_alloc &)
            {
                hr = E_OUTOFMEMORY;
            }
            catch(HRESULT eHR)
            {
                hr = eHR;
                LogFlowThisFunc(("Exception was caught in the function \n"));
            }

            if (SUCCEEDED(hr))
            {
                BOOL fCanceled = FALSE;
                hr = pProgressCopyTo->WaitForCompletion(-1);
                if (   SUCCEEDED(pProgressCopyTo->COMGETTER(Canceled)(&fCanceled))
                    && fCanceled)
                {
                    rc = VERR_GENERAL_FAILURE; /* Fudge. */
                }
                else if (FAILED(hr))
                {
                    Assert(FAILED(hr));
                    rc = VERR_GENERAL_FAILURE; /* Fudge. */
                }
            }
        }
    }

    /** @todo Note: Since there is no file locking involved at the moment, there can be modifications
     *              between finished copying, the verification and the actual execution. */

    /* Determine where the installer image ended up and if it has the correct size. */
    if (RT_SUCCESS(rc))
    {
        LogRel(("Verifying Guest Additions installer file \"%s\" ...\n",
                strFileDest.c_str()));

        GuestFsObjData objData;
        int64_t cbSizeOnGuest; int guestRc;
        rc = pSession->i_fileQuerySizeInternal(strFileDest, false /*fFollowSymlinks*/, &cbSizeOnGuest, &guestRc);
        if (   RT_SUCCESS(rc)
            && cbSize == (uint64_t)cbSizeOnGuest)
        {
            LogFlowThisFunc(("Guest Additions installer file \"%s\" successfully verified\n",
                             strFileDest.c_str()));
        }
        else
        {
            if (RT_SUCCESS(rc)) /* Size does not match. */
            {
                LogRel(("Size of Guest Additions installer file \"%s\" does not match: %RI64 bytes copied, %RU64 bytes expected\n",
                        strFileDest.c_str(), cbSizeOnGuest, cbSize));
                rc = VERR_BROKEN_PIPE; /** @todo Find a better error. */
            }
            else
            {
                switch (rc)
                {
                    case VERR_GSTCTL_GUEST_ERROR:
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                            GuestProcess::i_guestErrorToString(guestRc));
                        break;

                    default:
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                            Utf8StrFmt(GuestSession::tr("Error while querying size for file \"%s\": %Rrc"),
                                                       strFileDest.c_str(), rc));
                        break;
                }
            }
        }

        if (RT_SUCCESS(rc))
        {
            if (pcbSize)
                *pcbSize = (uint32_t)cbSizeOnGuest;
        }
    }

    return rc;
}

int SessionTaskUpdateAdditions::i_runFileOnGuest(GuestSession *pSession, GuestProcessStartupInfo &procInfo)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);

    LogRel(("Running %s ...\n", procInfo.mName.c_str()));

    GuestProcessTool procTool; int guestRc;
    int vrc = procTool.Init(pSession, procInfo, false /* Async */, &guestRc);
    if (RT_SUCCESS(vrc))
    {
        if (RT_SUCCESS(guestRc))
            vrc = procTool.i_wait(GUESTPROCESSTOOL_FLAG_NONE, &guestRc);
        if (RT_SUCCESS(vrc))
            vrc = procTool.i_terminatedOk();
    }

    if (RT_FAILURE(vrc))
    {
        switch (vrc)
        {
            case VWRN_GSTCTL_PROCESS_EXIT_CODE:
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(GuestSession::tr("Running update file \"%s\" on guest failed: %Rrc"),
                                               procInfo.mExecutable.c_str(), procTool.i_getRc()));
                break;

            case VERR_GSTCTL_GUEST_ERROR:
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    GuestProcess::i_guestErrorToString(guestRc));
                break;

            case VERR_INVALID_STATE: /** @todo Special guest control rc needed! */
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(GuestSession::tr("Update file \"%s\" reported invalid running state"),
                                               procInfo.mExecutable.c_str()));
                break;

            default:
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(GuestSession::tr("Error while running update file \"%s\" on guest: %Rrc"),
                                               procInfo.mExecutable.c_str(), vrc));
                break;
        }
    }

    return vrc;
}

int SessionTaskUpdateAdditions::Run(void)
{
    LogFlowThisFuncEnter();

    ComObjPtr<GuestSession> pSession = mSession;
    Assert(!pSession.isNull());

    AutoCaller autoCaller(pSession);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    int rc = setProgress(10);
    if (RT_FAILURE(rc))
        return rc;

    HRESULT hr = S_OK;

    LogRel(("Automatic update of Guest Additions started, using \"%s\"\n", mSource.c_str()));

    ComObjPtr<Guest> pGuest(mSession->i_getParent());
#if 0
    /*
     * Wait for the guest being ready within 30 seconds.
     */
    AdditionsRunLevelType_T addsRunLevel;
    uint64_t tsStart = RTTimeSystemMilliTS();
    while (   SUCCEEDED(hr = pGuest->COMGETTER(AdditionsRunLevel)(&addsRunLevel))
           && (    addsRunLevel != AdditionsRunLevelType_Userland
                && addsRunLevel != AdditionsRunLevelType_Desktop))
    {
        if ((RTTimeSystemMilliTS() - tsStart) > 30 * 1000)
        {
            rc = VERR_TIMEOUT;
            break;
        }

        RTThreadSleep(100); /* Wait a bit. */
    }

    if (FAILED(hr)) rc = VERR_TIMEOUT;
    if (rc == VERR_TIMEOUT)
        hr = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                 Utf8StrFmt(GuestSession::tr("Guest Additions were not ready within time, giving up")));
#else
    /*
     * For use with the GUI we don't want to wait, just return so that the manual .ISO mounting
     * can continue.
     */
    AdditionsRunLevelType_T addsRunLevel;
    if (   FAILED(hr = pGuest->COMGETTER(AdditionsRunLevel)(&addsRunLevel))
        || (   addsRunLevel != AdditionsRunLevelType_Userland
            && addsRunLevel != AdditionsRunLevelType_Desktop))
    {
        if (addsRunLevel == AdditionsRunLevelType_System)
            hr = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                     Utf8StrFmt(GuestSession::tr("Guest Additions are installed but not fully loaded yet, aborting automatic update")));
        else
            hr = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                     Utf8StrFmt(GuestSession::tr("Guest Additions not installed or ready, aborting automatic update")));
        rc = VERR_NOT_SUPPORTED;
    }
#endif

    if (RT_SUCCESS(rc))
    {
        /*
         * Determine if we are able to update automatically. This only works
         * if there are recent Guest Additions installed already.
         */
        Utf8Str strAddsVer;
        rc = getGuestProperty(pGuest, "/VirtualBox/GuestAdd/Version", strAddsVer);
        if (   RT_SUCCESS(rc)
            && RTStrVersionCompare(strAddsVer.c_str(), "4.1") < 0)
        {
            hr = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                     Utf8StrFmt(GuestSession::tr("Guest has too old Guest Additions (%s) installed for automatic updating, please update manually"),
                                                strAddsVer.c_str()));
            rc = VERR_NOT_SUPPORTED;
        }
    }

    Utf8Str strOSVer;
    eOSType osType = eOSType_Unknown;
    if (RT_SUCCESS(rc))
    {
        /*
         * Determine guest OS type and the required installer image.
         */
        Utf8Str strOSType;
        rc = getGuestProperty(pGuest, "/VirtualBox/GuestInfo/OS/Product", strOSType);
        if (RT_SUCCESS(rc))
        {
            if (   strOSType.contains("Microsoft", Utf8Str::CaseInsensitive)
                || strOSType.contains("Windows", Utf8Str::CaseInsensitive))
            {
                osType = eOSType_Windows;

                /*
                 * Determine guest OS version.
                 */
                rc = getGuestProperty(pGuest, "/VirtualBox/GuestInfo/OS/Release", strOSVer);
                if (RT_FAILURE(rc))
                {
                    hr = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                             Utf8StrFmt(GuestSession::tr("Unable to detected guest OS version, please update manually")));
                    rc = VERR_NOT_SUPPORTED;
                }

                /* Because Windows 2000 + XP and is bitching with WHQL popups even if we have signed drivers we
                 * can't do automated updates here. */
                /* Windows XP 64-bit (5.2) is a Windows 2003 Server actually, so skip this here. */
                if (   RT_SUCCESS(rc)
                    && RTStrVersionCompare(strOSVer.c_str(), "5.0") >= 0)
                {
                    if (   strOSVer.startsWith("5.0") /* Exclude the build number. */
                        || strOSVer.startsWith("5.1") /* Exclude the build number. */)
                    {
                        /* If we don't have AdditionsUpdateFlag_WaitForUpdateStartOnly set we can't continue
                         * because the Windows Guest Additions installer will fail because of WHQL popups. If the
                         * flag is set this update routine ends successfully as soon as the installer was started
                         * (and the user has to deal with it in the guest). */
                        if (!(mFlags & AdditionsUpdateFlag_WaitForUpdateStartOnly))
                        {
                            hr = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                                     Utf8StrFmt(GuestSession::tr("Windows 2000 and XP are not supported for automatic updating due to WHQL interaction, please update manually")));
                            rc = VERR_NOT_SUPPORTED;
                        }
                    }
                }
                else
                {
                    hr = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                             Utf8StrFmt(GuestSession::tr("%s (%s) not supported for automatic updating, please update manually"),
                                                        strOSType.c_str(), strOSVer.c_str()));
                    rc = VERR_NOT_SUPPORTED;
                }
            }
            else if (strOSType.contains("Solaris", Utf8Str::CaseInsensitive))
            {
                osType = eOSType_Solaris;
            }
            else /* Everything else hopefully means Linux :-). */
                osType = eOSType_Linux;

#if 1 /* Only Windows is supported (and tested) at the moment. */
            if (   RT_SUCCESS(rc)
                && osType != eOSType_Windows)
            {
                hr = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                         Utf8StrFmt(GuestSession::tr("Detected guest OS (%s) does not support automatic Guest Additions updating, please update manually"),
                                                    strOSType.c_str()));
                rc = VERR_NOT_SUPPORTED;
            }
#endif
        }
    }

    RTISOFSFILE iso;
    if (RT_SUCCESS(rc))
    {
        /*
         * Try to open the .ISO file to extract all needed files.
         */
        rc = RTIsoFsOpen(&iso, mSource.c_str());
        if (RT_FAILURE(rc))
        {
            hr = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                     Utf8StrFmt(GuestSession::tr("Unable to open Guest Additions .ISO file \"%s\": %Rrc"),
                                     mSource.c_str(), rc));
        }
        else
        {
            /* Set default installation directories. */
            Utf8Str strUpdateDir = "/tmp/";
            if (osType == eOSType_Windows)
                 strUpdateDir = "C:\\Temp\\";

            rc = setProgress(5);

            /* Try looking up the Guest Additions installation directory. */
            if (RT_SUCCESS(rc))
            {
                /* Try getting the installed Guest Additions version to know whether we
                 * can install our temporary Guest Addition data into the original installation
                 * directory.
                 *
                 * Because versions prior to 4.2 had bugs wrt spaces in paths we have to choose
                 * a different location then.
                 */
                bool fUseInstallDir = false;

                Utf8Str strAddsVer;
                rc = getGuestProperty(pGuest, "/VirtualBox/GuestAdd/Version", strAddsVer);
                if (   RT_SUCCESS(rc)
                    && RTStrVersionCompare(strAddsVer.c_str(), "4.2r80329") > 0)
                {
                    fUseInstallDir = true;
                }

                if (fUseInstallDir)
                {
                    if (RT_SUCCESS(rc))
                        rc = getGuestProperty(pGuest, "/VirtualBox/GuestAdd/InstallDir", strUpdateDir);
                    if (RT_SUCCESS(rc))
                    {
                        if (osType == eOSType_Windows)
                        {
                            strUpdateDir.findReplace('/', '\\');
                            strUpdateDir.append("\\Update\\");
                        }
                        else
                            strUpdateDir.append("/update/");
                    }
                }
            }

            if (RT_SUCCESS(rc))
                LogRel(("Guest Additions update directory is: %s\n",
                        strUpdateDir.c_str()));

            /* Create the installation directory. */
            int guestRc;
            rc = pSession->i_directoryCreateInternal(strUpdateDir,
                                                     755 /* Mode */, DirectoryCreateFlag_Parents, &guestRc);
            if (RT_FAILURE(rc))
            {
                switch (rc)
                {
                    case VERR_GSTCTL_GUEST_ERROR:
                        hr = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                 GuestProcess::i_guestErrorToString(guestRc));
                        break;

                    default:
                        hr = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                 Utf8StrFmt(GuestSession::tr("Error creating installation directory \"%s\" on the guest: %Rrc"),
                                                            strUpdateDir.c_str(), rc));
                        break;
                }
            }
            if (RT_SUCCESS(rc))
                rc = setProgress(10);

            if (RT_SUCCESS(rc))
            {
                /* Prepare the file(s) we want to copy over to the guest and
                 * (maybe) want to run. */
                switch (osType)
                {
                    case eOSType_Windows:
                    {
                        /* Do we need to install our certificates? We do this for W2K and up. */
                        bool fInstallCert = false;

                        /* Only Windows 2000 and up need certificates to be installed. */
                        if (RTStrVersionCompare(strOSVer.c_str(), "5.0") >= 0)
                        {
                            fInstallCert = true;
                            LogRel(("Certificates for auto updating WHQL drivers will be installed\n"));
                        }
                        else
                            LogRel(("Skipping installation of certificates for WHQL drivers\n"));

                        if (fInstallCert)
                        {
                            static struct { const char *pszDst, *pszIso; } const s_aCertFiles[] =
                            {
                                { "vbox.cer",           "CERT/VBOX.CER" },
                                { "vbox-sha1.cer",      "CERT/VBOX_SHA1.CER" },
                                { "vbox-sha256.cer",    "CERT/VBOX_SHA256.CER" },
                                { "vbox-sha256-r3.cer", "CERT/VBOX_SHA256_R3.CER" },
                                { "oracle-vbox.cer",    "CERT/ORACLE_VBOX.CER" },
                            };
                            uint32_t fCopyCertUtil = UPDATEFILE_FLAG_COPY_FROM_ISO;
                            for (uint32_t i = 0; i < RT_ELEMENTS(s_aCertFiles); i++)
                            {
                                /* Skip if not present on the ISO. */
                                uint32_t offIgn;
                                size_t   cbIgn;
                                rc = RTIsoFsGetFileInfo(&iso, s_aCertFiles[i].pszIso, &offIgn, &cbIgn);
                                if (RT_FAILURE(rc))
                                    continue;

                                /* Copy the certificate certificate. */
                                Utf8Str const strDstCert(strUpdateDir + s_aCertFiles[i].pszDst);
                                mFiles.push_back(InstallerFile(s_aCertFiles[i].pszIso,
                                                               strDstCert,
                                                               UPDATEFILE_FLAG_COPY_FROM_ISO | UPDATEFILE_FLAG_OPTIONAL));

                                /* Out certificate installation utility. */
                                /* First pass: Copy over the file (first time only) + execute it to remove any
                                 *             existing VBox certificates. */
                                GuestProcessStartupInfo siCertUtilRem;
                                siCertUtilRem.mName = "VirtualBox Certificate Utility, removing old VirtualBox certificates";
                                siCertUtilRem.mArguments.push_back(Utf8Str("remove-trusted-publisher"));
                                siCertUtilRem.mArguments.push_back(Utf8Str("--root")); /* Add root certificate as well. */
                                siCertUtilRem.mArguments.push_back(strDstCert);
                                siCertUtilRem.mArguments.push_back(strDstCert);
                                mFiles.push_back(InstallerFile("CERT/VBOXCERTUTIL.EXE",
                                                               strUpdateDir + "VBoxCertUtil.exe",
                                                               fCopyCertUtil | UPDATEFILE_FLAG_EXECUTE | UPDATEFILE_FLAG_OPTIONAL,
                                                               siCertUtilRem));
                                fCopyCertUtil = 0;
                                /* Second pass: Only execute (but don't copy) again, this time installng the
                                 *              recent certificates just copied over. */
                                GuestProcessStartupInfo siCertUtilAdd;
                                siCertUtilAdd.mName = "VirtualBox Certificate Utility, installing VirtualBox certificates";
                                siCertUtilAdd.mArguments.push_back(Utf8Str("add-trusted-publisher"));
                                siCertUtilAdd.mArguments.push_back(Utf8Str("--root")); /* Add root certificate as well. */
                                siCertUtilAdd.mArguments.push_back(strDstCert);
                                siCertUtilAdd.mArguments.push_back(strDstCert);
                                mFiles.push_back(InstallerFile("CERT/VBOXCERTUTIL.EXE",
                                                               strUpdateDir + "VBoxCertUtil.exe",
                                                               UPDATEFILE_FLAG_EXECUTE | UPDATEFILE_FLAG_OPTIONAL,
                                                               siCertUtilAdd));
                            }
                        }
                        /* The installers in different flavors, as we don't know (and can't assume)
                         * the guest's bitness. */
                        mFiles.push_back(InstallerFile("VBOXWINDOWSADDITIONS_X86.EXE",
                                                       strUpdateDir + "VBoxWindowsAdditions-x86.exe",
                                                       UPDATEFILE_FLAG_COPY_FROM_ISO));
                        mFiles.push_back(InstallerFile("VBOXWINDOWSADDITIONS_AMD64.EXE",
                                                       strUpdateDir + "VBoxWindowsAdditions-amd64.exe",
                                                       UPDATEFILE_FLAG_COPY_FROM_ISO));
                        /* The stub loader which decides which flavor to run. */
                        GuestProcessStartupInfo siInstaller;
                        siInstaller.mName = "VirtualBox Windows Guest Additions Installer";
                        /* Set a running timeout of 5 minutes -- the Windows Guest Additions
                         * setup can take quite a while, so be on the safe side. */
                        siInstaller.mTimeoutMS = 5 * 60 * 1000;
                        siInstaller.mArguments.push_back(Utf8Str("/S")); /* We want to install in silent mode. */
                        siInstaller.mArguments.push_back(Utf8Str("/l")); /* ... and logging enabled. */
                        /* Don't quit VBoxService during upgrade because it still is used for this
                         * piece of code we're in right now (that is, here!) ... */
                        siInstaller.mArguments.push_back(Utf8Str("/no_vboxservice_exit"));
                        /* Tell the installer to report its current installation status
                         * using a running VBoxTray instance via balloon messages in the
                         * Windows taskbar. */
                        siInstaller.mArguments.push_back(Utf8Str("/post_installstatus"));
                        /* Add optional installer command line arguments from the API to the
                         * installer's startup info. */
                        rc = i_addProcessArguments(siInstaller.mArguments, mArguments);
                        AssertRC(rc);
                        /* If the caller does not want to wait for out guest update process to end,
                         * complete the progress object now so that the caller can do other work. */
                        if (mFlags & AdditionsUpdateFlag_WaitForUpdateStartOnly)
                            siInstaller.mFlags |= ProcessCreateFlag_WaitForProcessStartOnly;
                        mFiles.push_back(InstallerFile("VBOXWINDOWSADDITIONS.EXE",
                                                       strUpdateDir + "VBoxWindowsAdditions.exe",
                                                       UPDATEFILE_FLAG_COPY_FROM_ISO | UPDATEFILE_FLAG_EXECUTE, siInstaller));
                        break;
                    }
                    case eOSType_Linux:
                        /** @todo Add Linux support. */
                        break;
                    case eOSType_Solaris:
                        /** @todo Add Solaris support. */
                        break;
                    default:
                        AssertReleaseMsgFailed(("Unsupported guest type: %d\n", osType));
                        break;
                }
            }

            if (RT_SUCCESS(rc))
            {
                /* We want to spend 40% total for all copying operations. So roughly
                 * calculate the specific percentage step of each copied file. */
                uint8_t uOffset = 20; /* Start at 20%. */
                uint8_t uStep = 40 / (uint8_t)mFiles.size(); Assert(mFiles.size() <= 10);

                LogRel(("Copying over Guest Additions update files to the guest ...\n"));

                std::vector<InstallerFile>::const_iterator itFiles = mFiles.begin();
                while (itFiles != mFiles.end())
                {
                    if (itFiles->fFlags & UPDATEFILE_FLAG_COPY_FROM_ISO)
                    {
                        bool fOptional = false;
                        if (itFiles->fFlags & UPDATEFILE_FLAG_OPTIONAL)
                            fOptional = true;
                        rc = i_copyFileToGuest(pSession, &iso, itFiles->strSource, itFiles->strDest,
                                               fOptional, NULL /* cbSize */);
                        if (RT_FAILURE(rc))
                        {
                            hr = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                     Utf8StrFmt(GuestSession::tr("Error while copying file \"%s\" to \"%s\" on the guest: %Rrc"),
                                                                itFiles->strSource.c_str(), itFiles->strDest.c_str(), rc));
                            break;
                        }
                    }

                    rc = setProgress(uOffset);
                    if (RT_FAILURE(rc))
                        break;
                    uOffset += uStep;

                    ++itFiles;
                }
            }

            /* Done copying, close .ISO file. */
            RTIsoFsClose(&iso);

            if (RT_SUCCESS(rc))
            {
                /* We want to spend 35% total for all copying operations. So roughly
                 * calculate the specific percentage step of each copied file. */
                uint8_t uOffset = 60; /* Start at 60%. */
                uint8_t uStep = 35 / (uint8_t)mFiles.size(); Assert(mFiles.size() <= 10);

                LogRel(("Executing Guest Additions update files ...\n"));

                std::vector<InstallerFile>::iterator itFiles = mFiles.begin();
                while (itFiles != mFiles.end())
                {
                    if (itFiles->fFlags & UPDATEFILE_FLAG_EXECUTE)
                    {
                        rc = i_runFileOnGuest(pSession, itFiles->mProcInfo);
                        if (RT_FAILURE(rc))
                            break;
                    }

                    rc = setProgress(uOffset);
                    if (RT_FAILURE(rc))
                        break;
                    uOffset += uStep;

                    ++itFiles;
                }
            }

            if (RT_SUCCESS(rc))
            {
                LogRel(("Automatic update of Guest Additions succeeded\n"));
                rc = setProgressSuccess();
            }
        }
    }

    if (RT_FAILURE(rc))
    {
        if (rc == VERR_CANCELLED)
        {
            LogRel(("Automatic update of Guest Additions was canceled\n"));

            hr = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                     Utf8StrFmt(GuestSession::tr("Installation was canceled")));
        }
        else
        {
            Utf8Str strError = Utf8StrFmt("No further error information available (%Rrc)", rc);
            if (!mProgress.isNull()) /* Progress object is optional. */
            {
                com::ProgressErrorInfo errorInfo(mProgress);
                if (   errorInfo.isFullAvailable()
                    || errorInfo.isBasicAvailable())
                {
                    strError = errorInfo.getText();
                }
            }

            LogRel(("Automatic update of Guest Additions failed: %s (%Rhrc)\n",
                    strError.c_str(), hr));
        }

        LogRel(("Please install Guest Additions manually\n"));
    }

    /** @todo Clean up copied / left over installation files. */

    LogFlowFuncLeaveRC(rc);
    return rc;
}

