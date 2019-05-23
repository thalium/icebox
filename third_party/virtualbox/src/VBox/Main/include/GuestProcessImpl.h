/* $Id: GuestProcessImpl.h $ */
/** @file
 * VirtualBox Main - Guest process handling implementation.
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

#ifndef ____H_GUESTPROCESSIMPL
#define ____H_GUESTPROCESSIMPL

#include "GuestCtrlImplPrivate.h"
#include "GuestProcessWrap.h"

#include <iprt/cpp/utils.h>

class Console;
class GuestSession;
class GuestProcessStartTask;

/**
 * Class for handling a guest process.
 */
class ATL_NO_VTABLE GuestProcess :
    public GuestProcessWrap,
    public GuestObject
{
public:
    /** @name COM and internal init/term/mapping cruft.
     * @{ */
    DECLARE_EMPTY_CTOR_DTOR(GuestProcess)

    int     init(Console *aConsole, GuestSession *aSession, ULONG aProcessID,
                 const GuestProcessStartupInfo &aProcInfo, const GuestEnvironment *pBaseEnv);
    void    uninit(void);
    HRESULT FinalConstruct(void);
    void    FinalRelease(void);
    /** @}  */

public:
    /** @name Public internal methods.
     * @{ */
    int i_callbackDispatcher(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCb);
    inline int i_checkPID(uint32_t uPID);
    int i_onRemove(void);
    int i_readData(uint32_t uHandle, uint32_t uSize, uint32_t uTimeoutMS, void *pvData, size_t cbData, uint32_t *pcbRead, int *pGuestRc);
    int i_startProcess(uint32_t cMsTimeout, int *pGuestRc);
    int i_startProcessInner(uint32_t cMsTimeout, AutoWriteLock &rLock, GuestWaitEvent *pEvent, int *pGuestRc);
    int i_startProcessAsync(void);
    int i_terminateProcess(uint32_t uTimeoutMS, int *pGuestRc);
    ProcessWaitResult_T i_waitFlagsToResult(uint32_t fWaitFlags);
    int i_waitFor(uint32_t fWaitFlags, ULONG uTimeoutMS, ProcessWaitResult_T &waitResult, int *pGuestRc);
    int i_waitForInputNotify(GuestWaitEvent *pEvent, uint32_t uHandle, uint32_t uTimeoutMS, ProcessInputStatus_T *pInputStatus, uint32_t *pcbProcessed);
    int i_waitForOutput(GuestWaitEvent *pEvent, uint32_t uHandle, uint32_t uTimeoutMS, void* pvData, size_t cbData, uint32_t *pcbRead);
    int i_waitForStatusChange(GuestWaitEvent *pEvent, uint32_t uTimeoutMS, ProcessStatus_T *pProcessStatus, int *pGuestRc);
    int i_writeData(uint32_t uHandle, uint32_t uFlags, void *pvData, size_t cbData, uint32_t uTimeoutMS, uint32_t *puWritten, int *pGuestRc);
    /** @}  */

    /** @name Static internal methods.
     * @{ */
    static Utf8Str i_guestErrorToString(int guestRc);
    static bool i_isGuestError(int guestRc);
    static HRESULT i_setErrorExternal(VirtualBoxBase *pInterface, int guestRc);
    static ProcessWaitResult_T i_waitFlagsToResultEx(uint32_t fWaitFlags, ProcessStatus_T oldStatus, ProcessStatus_T newStatus, uint32_t uProcFlags, uint32_t uProtocol);
    static bool i_waitResultImpliesEx(ProcessWaitResult_T waitResult, ProcessStatus_T procStatus, uint32_t uProcFlags, uint32_t uProtocol);
    /** @}  */

protected:
    /** @name Protected internal methods.
     * @{ */
    inline bool i_isAlive(void);
    inline bool i_hasEnded(void);
    int i_onGuestDisconnected(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData);
    int i_onProcessInputStatus(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData);
    int i_onProcessNotifyIO(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData);
    int i_onProcessStatusChange(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData);
    int i_onProcessOutput(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData);
    int i_prepareExecuteEnv(const char *pszEnv, void **ppvList, ULONG *pcbList, ULONG *pcEnvVars);
    int i_setProcessStatus(ProcessStatus_T procStatus, int procRc);
    static void i_startProcessThreadTask(GuestProcessStartTask *pTask);
    /** @}  */

private:
    /** Wrapped @name IProcess properties.
     * @{ */
    HRESULT getArguments(std::vector<com::Utf8Str> &aArguments);
    HRESULT getEnvironment(std::vector<com::Utf8Str> &aEnvironment);
    HRESULT getEventSource(ComPtr<IEventSource> &aEventSource);
    HRESULT getExecutablePath(com::Utf8Str &aExecutablePath);
    HRESULT getExitCode(LONG *aExitCode);
    HRESULT getName(com::Utf8Str &aName);
    HRESULT getPID(ULONG *aPID);
    HRESULT getStatus(ProcessStatus_T *aStatus);
    /** @}  */

    /** Wrapped @name IProcess methods.
     * @{ */
    HRESULT waitFor(ULONG aWaitFor,
                    ULONG aTimeoutMS,
                    ProcessWaitResult_T *aReason);
    HRESULT waitForArray(const std::vector<ProcessWaitForFlag_T> &aWaitFor,
                         ULONG aTimeoutMS,
                         ProcessWaitResult_T *aReason);
    HRESULT read(ULONG aHandle,
                 ULONG aToRead,
                 ULONG aTimeoutMS,
                 std::vector<BYTE> &aData);
    HRESULT write(ULONG aHandle,
                  ULONG aFlags,
                  const std::vector<BYTE> &aData,
                  ULONG aTimeoutMS,
                  ULONG *aWritten);
    HRESULT writeArray(ULONG aHandle,
                       const std::vector<ProcessInputFlag_T> &aFlags,
                       const std::vector<BYTE> &aData,
                       ULONG aTimeoutMS,
                       ULONG *aWritten);
    HRESULT terminate(void);
    /** @}  */

    /**
     * This can safely be used without holding any locks.
     * An AutoCaller suffices to prevent it being destroy while in use and
     * internally there is a lock providing the necessary serialization.
     */
    const ComObjPtr<EventSource> mEventSource;

    struct Data
    {
        /** The process startup information. */
        GuestProcessStartupInfo  mProcess;
        /** Reference to the immutable session base environment. NULL if the
         * environment feature isn't supported.
         * @remarks If there is proof that the uninit order of GuestSession and
         *          this class is what GuestObjectBase claims, then this isn't
         *          strictly necessary. */
        GuestEnvironment const  *mpSessionBaseEnv;
        /** Exit code if process has been terminated. */
        LONG                     mExitCode;
        /** PID reported from the guest. */
        ULONG                    mPID;
        /** The current process status. */
        ProcessStatus_T          mStatus;
        /** The last returned process status
         *  returned from the guest side. */
        int                      mLastError;

        Data(void) : mpSessionBaseEnv(NULL)
        { }
        ~Data(void)
        {
            if (mpSessionBaseEnv)
            {
                mpSessionBaseEnv->releaseConst();
                mpSessionBaseEnv = NULL;
            }
        }
    } mData;

    friend class GuestProcessStartTask;
};

/**
 * Guest process tool flags.
 */
/** No flags specified; wait until process terminates.
 *  The maximum waiting time is set in the process' startup
 *  info. */
#define GUESTPROCESSTOOL_FLAG_NONE            0
/** Wait until next stream block from stdout has been
 *  read in completely, then return.
 */
#define GUESTPROCESSTOOL_FLAG_STDOUT_BLOCK    RT_BIT(0)

/**
 * Structure for keeping a VBoxService toolbox tool's error info around.
 */
struct GuestProcessToolErrorInfo
{
    /** Return code from the guest side for executing the process tool. */
    int  guestRc;
    /** The process tool's returned exit code. */
    LONG lExitCode;
};

/**
 * Internal class for handling the BusyBox-like tools built into VBoxService
 * on the guest side. It's also called the VBoxService Toolbox (tm).
 *
 * Those initially were necessary to guarantee execution of commands (like "ls", "cat")
 * under the behalf of a certain guest user.
 *
 * This class essentially helps to wrap all the gory details like process creation,
 * information extraction and maintaining the overall status.
 */
class GuestProcessTool
{
public:

    GuestProcessTool(void);

    virtual ~GuestProcessTool(void);

public:

    int Init(GuestSession *pGuestSession, const GuestProcessStartupInfo &startupInfo, bool fAsync, int *pGuestRc);

    int i_getCurrentBlock(uint32_t uHandle, GuestProcessStreamBlock &strmBlock);

    int i_getRc(void) const;

    GuestProcessStream &i_getStdOut(void) { return mStdOut; }

    GuestProcessStream &i_getStdErr(void) { return mStdErr; }

    int i_wait(uint32_t fFlags, int *pGuestRc);

    int i_waitEx(uint32_t fFlags, GuestProcessStreamBlock *pStreamBlock, int *pGuestRc);

    bool i_isRunning(void);

    int i_terminatedOk(LONG *plExitCode = NULL);

    int i_terminate(uint32_t uTimeoutMS, int *pGuestRc);

public:

    static int i_run(GuestSession *pGuestSession, const GuestProcessStartupInfo &startupInfo, int *pGuestRc);

    static int i_runErrorInfo(GuestSession *pGuestSession, const GuestProcessStartupInfo &startupInfo, GuestProcessToolErrorInfo &errorInfo);

    static int i_runEx(GuestSession *pGuestSession, const GuestProcessStartupInfo &startupInfo,
                       GuestCtrlStreamObjects *pStrmOutObjects, uint32_t cStrmOutObjects, int *pGuestRc);

    static int i_runExErrorInfo(GuestSession *pGuestSession, const GuestProcessStartupInfo &startupInfo,
                                GuestCtrlStreamObjects *pStrmOutObjects, uint32_t cStrmOutObjects, GuestProcessToolErrorInfo &errorInfo);

    static int i_exitCodeToRc(const GuestProcessStartupInfo &startupInfo, LONG lExitCode);

    static int i_exitCodeToRc(const char *pszTool, LONG lExitCode);

protected:

    ComObjPtr<GuestSession>     pSession;
    ComObjPtr<GuestProcess>     pProcess;
    GuestProcessStartupInfo     mStartupInfo;
    GuestProcessStream          mStdOut;
    GuestProcessStream          mStdErr;

};

#endif /* !____H_GUESTPROCESSIMPL */

