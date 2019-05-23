/* $Id: GuestFileImpl.h $ */
/** @file
 * VirtualBox Main - Guest file handling implementation.
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

#ifndef ____H_GUESTFILEIMPL
#define ____H_GUESTFILEIMPL

#include "VirtualBoxBase.h"
#include "EventImpl.h"

#include "GuestCtrlImplPrivate.h"
#include "GuestFileWrap.h"

class Console;
class GuestSession;
class GuestProcess;

class ATL_NO_VTABLE GuestFile :
    public GuestFileWrap,
    public GuestObject
{
public:
    /** @name COM and internal init/term/mapping cruft.
     * @{ */
    DECLARE_EMPTY_CTOR_DTOR(GuestFile)

    int     init(Console *pConsole, GuestSession *pSession, ULONG uFileID, const GuestFileOpenInfo &openInfo);
    void    uninit(void);

    HRESULT FinalConstruct(void);
    void    FinalRelease(void);
    /** @}  */

public:
    /** @name Public internal methods.
     * @{ */
    int             i_callbackDispatcher(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCb);
    int             i_closeFile(int *pGuestRc);
    EventSource    *i_getEventSource(void) { return mEventSource; }
    static Utf8Str  i_guestErrorToString(int guestRc);
    int             i_onFileNotify(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData);
    int             i_onGuestDisconnected(PVBOXGUESTCTRLHOSTCBCTX pCbCtx, PVBOXGUESTCTRLHOSTCALLBACK pSvcCbData);
    int             i_onRemove(void);
    int             i_openFile(uint32_t uTimeoutMS, int *pGuestRc);
    int             i_readData(uint32_t uSize, uint32_t uTimeoutMS, void* pvData, uint32_t cbData, uint32_t* pcbRead);
    int             i_readDataAt(uint64_t uOffset, uint32_t uSize, uint32_t uTimeoutMS,
                                 void* pvData, size_t cbData, size_t* pcbRead);
    int             i_seekAt(int64_t iOffset, GUEST_FILE_SEEKTYPE eSeekType, uint32_t uTimeoutMS, uint64_t *puOffset);
    static HRESULT  i_setErrorExternal(VirtualBoxBase *pInterface, int guestRc);
    int             i_setFileStatus(FileStatus_T fileStatus, int fileRc);
    int             i_waitForOffsetChange(GuestWaitEvent *pEvent, uint32_t uTimeoutMS, uint64_t *puOffset);
    int             i_waitForRead(GuestWaitEvent *pEvent, uint32_t uTimeoutMS, void *pvData, size_t cbData, uint32_t *pcbRead);
    int             i_waitForStatusChange(GuestWaitEvent *pEvent, uint32_t uTimeoutMS, FileStatus_T *pFileStatus, int *pGuestRc);
    int             i_waitForWrite(GuestWaitEvent *pEvent, uint32_t uTimeoutMS, uint32_t *pcbWritten);
    int             i_writeData(uint32_t uTimeoutMS, void *pvData, uint32_t cbData, uint32_t *pcbWritten);
    int             i_writeDataAt(uint64_t uOffset, uint32_t uTimeoutMS, void *pvData, uint32_t cbData, uint32_t *pcbWritten);
    /** @}  */

private:

    /** @name Wrapped IGuestFile properties.
     * @{ */
    HRESULT getCreationMode(ULONG *aCreationMode);
    HRESULT getEventSource(ComPtr<IEventSource> &aEventSource);
    HRESULT getId(ULONG *aId);
    HRESULT getInitialSize(LONG64 *aInitialSize);
    HRESULT getOffset(LONG64 *aOffset);
    HRESULT getStatus(FileStatus_T *aStatus);
    HRESULT getFileName(com::Utf8Str &aFileName);
    HRESULT getAccessMode(FileAccessMode_T *aAccessMode);
    HRESULT getOpenAction(FileOpenAction_T *aOpenAction);
    /** @}  */

    /** @name Wrapped IGuestFile methods.
     * @{ */
    HRESULT close();
    HRESULT queryInfo(ComPtr<IFsObjInfo> &aObjInfo);
    HRESULT querySize(LONG64 *aSize);
    HRESULT read(ULONG aToRead,
                 ULONG aTimeoutMS,
                 std::vector<BYTE> &aData);
    HRESULT readAt(LONG64 aOffset,
                   ULONG aToRead,
                   ULONG aTimeoutMS,
                   std::vector<BYTE> &aData);
    HRESULT seek(LONG64 aOffset,
                 FileSeekOrigin_T aWhence,
                 LONG64 *aNewOffset);
    HRESULT setACL(const com::Utf8Str &aAcl,
                   ULONG aMode);
    HRESULT setSize(LONG64 aSize);
    HRESULT write(const std::vector<BYTE> &aData,
                  ULONG aTimeoutMS,
                  ULONG *aWritten);
    HRESULT writeAt(LONG64 aOffset,
                    const std::vector<BYTE> &aData,
                    ULONG aTimeoutMS,
                    ULONG *aWritten);
    /** @}  */

    /** This can safely be used without holding any locks.
     * An AutoCaller suffices to prevent it being destroy while in use and
     * internally there is a lock providing the necessary serialization. */
    const ComObjPtr<EventSource> mEventSource;

    struct Data
    {
        /** The file's open info. */
        GuestFileOpenInfo       mOpenInfo;
        /** The file's initial size on open. */
        uint64_t                mInitialSize;
        /** The file's ID. */
        uint32_t                mID;
        /** The current file status. */
        FileStatus_T            mStatus;
        /** The last returned process status
         *  returned from the guest side. */
        int                     mLastError;
        /** The file's current offset. */
        uint64_t                mOffCurrent;
    } mData;
};

#endif /* !____H_GUESTFILEIMPL */

