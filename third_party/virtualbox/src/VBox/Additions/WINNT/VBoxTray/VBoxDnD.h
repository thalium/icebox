/* $Id: VBoxDnD.h $ */
/** @file
 * VBoxDnD.h - Windows-specific bits of the drag'n drop service.
 */

/*
 * Copyright (C) 2013-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __VBOXTRAYDND__H
#define __VBOXTRAYDND__H

#include <iprt/critsect.h>

#include <iprt/cpp/mtlist.h>
#include <iprt/cpp/ministring.h>

class VBoxDnDWnd;

class VBoxDnDDataObject : public IDataObject
{
public:

    enum Status
    {
        Uninitialized = 0,
        Initialized,
        Dropping,
        Dropped,
        Aborted
    };

public:

    VBoxDnDDataObject(LPFORMATETC pFormatEtc = NULL, LPSTGMEDIUM pStgMed = NULL, ULONG cFormats = 0);
    virtual ~VBoxDnDDataObject(void);

public: /* IUnknown methods. */

    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

public: /* IDataObject methods. */

    STDMETHOD(GetData)(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium);
    STDMETHOD(GetDataHere)(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium);
    STDMETHOD(QueryGetData)(LPFORMATETC pFormatEtc);
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pFormatEct,  LPFORMATETC pFormatEtcOut);
    STDMETHOD(SetData)(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium, BOOL fRelease);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
    STDMETHOD(DAdvise)(LPFORMATETC pFormatEtc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
    STDMETHOD(DUnadvise)(DWORD dwConnection);
    STDMETHOD(EnumDAdvise)(IEnumSTATDATA **ppEnumAdvise);

public:

    static const char* ClipboardFormatToString(CLIPFORMAT fmt);

    int Abort(void);
    void SetStatus(Status status);
    int Signal(const RTCString &strFormat, const void *pvData, uint32_t cbData);

protected:

    bool LookupFormatEtc(LPFORMATETC pFormatEtc, ULONG *puIndex);
    static HGLOBAL MemDup(HGLOBAL hMemSource);
    void RegisterFormat(LPFORMATETC pFormatEtc, CLIPFORMAT clipFormat, TYMED tyMed = TYMED_HGLOBAL,
                        LONG lindex = -1, DWORD dwAspect = DVASPECT_CONTENT, DVTARGETDEVICE *pTargetDevice = NULL);

    Status      mStatus;
    LONG        mRefCount;
    ULONG       mcFormats;
    LPFORMATETC mpFormatEtc;
    LPSTGMEDIUM mpStgMedium;
    RTSEMEVENT  mSemEvent;
    RTCString   mstrFormat;
    void       *mpvData;
    uint32_t    mcbData;
};

class VBoxDnDDropSource : public IDropSource
{
public:

    VBoxDnDDropSource(VBoxDnDWnd *pThis);
    virtual ~VBoxDnDDropSource(void);

public:

    uint32_t GetCurrentAction(void) { return muCurAction; }

public: /* IUnknown methods. */

    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

public: /* IDropSource methods. */

    STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD dwKeyState);
    STDMETHOD(GiveFeedback)(DWORD dwEffect);

protected:

    /** Reference count of this object. */
    LONG                  mRefCount;
    /** Pointer to parent proxy window. */
    VBoxDnDWnd           *mpWndParent;
    /** Current drag effect. */
    DWORD                 mdwCurEffect;
    /** Current action to perform on the host. */
    uint32_t              muCurAction;
};

class VBoxDnDDropTarget : public IDropTarget
{
public:

    VBoxDnDDropTarget(VBoxDnDWnd *pThis);
    virtual ~VBoxDnDDropTarget(void);

public: /* IUnknown methods. */

    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

public: /* IDropTarget methods. */

    STDMETHOD(DragEnter)(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragLeave)(void);
    STDMETHOD(Drop)(IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

protected:

    static void DumpFormats(IDataObject *pDataObject);
    static DWORD GetDropEffect(DWORD grfKeyState, DWORD dwAllowedEffects);
    void reset(void);

public:

    void *DataMutableRaw(void) const { return mpvData; }
    size_t DataSize(void) const { return mcbData; }
    RTCString Formats(void) const;
    int WaitForDrop(RTMSINTERVAL msTimeout);

protected:

    /** Reference count of this object. */
    LONG                  mRefCount;
    /** Pointer to parent proxy window. */
    VBoxDnDWnd           *mpWndParent;
    /** Current drop effect. */
    DWORD                 mdwCurEffect;
    /** Copy of the data object's current FORMATETC struct.
     *  Note: We don't keep the pointer of the DVTARGETDEVICE here! */
    FORMATETC             mFormatEtc;
    /** Stringified data object's formats string.  */
    RTCString             mstrFormats;
    /** Pointer to actual format data. */
    void                 *mpvData;
    /** Size (in bytes) of format data. */
    size_t                mcbData;
    /** Event for waiting on the "drop" event. */
    RTSEMEVENT            hEventDrop;
    /** Result of the drop event. */
    int                   mDroppedRc;
};

class VBoxDnDEnumFormatEtc : public IEnumFORMATETC
{
public:

    VBoxDnDEnumFormatEtc(LPFORMATETC pFormatEtc, ULONG cFormats);
    virtual ~VBoxDnDEnumFormatEtc(void);

public:

    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    STDMETHOD(Next)(ULONG cFormats, LPFORMATETC pFormatEtc, ULONG *pcFetched);
    STDMETHOD(Skip)(ULONG cFormats);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumFORMATETC **ppEnumFormatEtc);

public:

    static void CopyFormat(LPFORMATETC pFormatDest, LPFORMATETC pFormatSource);
    static HRESULT CreateEnumFormatEtc(UINT cFormats, LPFORMATETC pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc);

private:

    LONG        m_lRefCount;
    ULONG       m_nIndex;
    ULONG       m_nNumFormats;
    LPFORMATETC m_pFormatEtc;
};

struct VBOXDNDCONTEXT;
class VBoxDnDWnd;

/*
 * A drag'n drop event from the host.
 */
typedef struct VBOXDNDEVENT
{
    /** The actual event data. */
    VBGLR3DNDHGCMEVENT Event;

} VBOXDNDEVENT, *PVBOXDNDEVENT;

/**
 * DnD context data.
 */
typedef struct VBOXDNDCONTEXT
{
    /** Pointer to the service environment. */
    const VBOXSERVICEENV      *pEnv;
    /** Shutdown indicator. */
    bool                       fShutdown;
    /** The registered window class. */
    ATOM                       wndClass;
    /** The DnD main event queue. */
    RTCMTList<VBOXDNDEVENT>    lstEvtQueue;
    /** Semaphore for waiting on main event queue
     *  events. */
    RTSEMEVENT                 hEvtQueueSem;
    /** List of drag'n drop proxy windows.
     *  Note: At the moment only one window is supported. */
    RTCMTList<VBoxDnDWnd*>     lstWnd;
    /** The DnD command context. */
    VBGLR3GUESTDNDCMDCTX       cmdCtx;

} VBOXDNDCONTEXT, *PVBOXDNDCONTEXT;

/**
 * Everything which is required to successfully start
 * a drag'n drop operation via DoDragDrop().
 */
typedef struct VBOXDNDSTARTUPINFO
{
    /** Our DnD data object, holding
     *  the raw DnD data. */
    VBoxDnDDataObject         *pDataObject;
    /** The drop source for sending the
     *  DnD request to a IDropTarget. */
    VBoxDnDDropSource         *pDropSource;
    /** The DnD effects which are wanted / allowed. */
    DWORD                      dwOKEffects;

} VBOXDNDSTARTUPINFO, *PVBOXDNDSTARTUPINFO;

/**
 * Class for handling a DnD proxy window.
 ** @todo Unify this and VBoxClient's DragInstance!
 */
class VBoxDnDWnd
{
    /**
     * Current state of a DnD proxy
     * window.
     */
    enum State
    {
        Uninitialized = 0,
        Initialized,
        Dragging,
        Dropped,
        Canceled
    };

    /**
     * Current operation mode of
     * a DnD proxy window.
     */
    enum Mode
    {
        /** Unknown mode. */
        Unknown = 0,
        /** Host to guest. */
        HG,
        /** Guest to host. */
        GH
    };

public:

    VBoxDnDWnd(void);
    virtual ~VBoxDnDWnd(void);

public:

    int Initialize(PVBOXDNDCONTEXT pContext);
    void Destroy(void);

public:

    /** The window's thread for the native message pump and
     *  OLE context. */
    static int Thread(RTTHREAD hThread, void *pvUser);

public:

    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM lParam);
    /** The per-instance wndproc routine. */
    LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:

#ifdef VBOX_WITH_DRAG_AND_DROP_GH
    int RegisterAsDropTarget(void);
    int UnregisterAsDropTarget(void);
#endif

public:

    int OnCreate(void);
    void OnDestroy(void);

    /* H->G */
    int OnHgEnter(const RTCList<RTCString> &formats, uint32_t uAllActions);
    int OnHgMove(uint32_t u32xPos, uint32_t u32yPos, uint32_t uAllActions);
    int OnHgDrop(void);
    int OnHgLeave(void);
    int OnHgDataReceived(const void *pvData, uint32_t cData);
    int OnHgCancel(void);

#ifdef VBOX_WITH_DRAG_AND_DROP_GH
    /* G->H */
    int OnGhIsDnDPending(uint32_t uScreenID);
    int OnGhDropped(const char *pszFormat, uint32_t cbFormats, uint32_t uDefAction);
#endif

    void PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    int ProcessEvent(PVBOXDNDEVENT pEvent);

public:

    int hide(void);

protected:

    int makeFullscreen(void);
    int mouseMove(int x, int y, DWORD dwMouseInputFlags);
    int mouseRelease(void);
    void reset(void);
    int setMode(Mode enmMode);

public: /** @todo Make protected! */

    /** Pointer to DnD context. */
    PVBOXDNDCONTEXT            pCtx;
    /** The proxy window's main thread for processing
     *  window messages. */
    RTTHREAD                   hThread;
    RTCRITSECT                 mCritSect;
    RTSEMEVENT                 mEventSem;
#ifdef RT_OS_WINDOWS
    /** The window's handle. */
    HWND                       hWnd;
    /** List of allowed MIME types this
     *  client can handle. Make this a per-instance
     *  property so that we can selectively allow/forbid
     *  certain types later on runtime. */
    RTCList<RTCString>         lstFmtSup;
    /** List of formats for the current
     *  drag'n drop operation. */
    RTCList<RTCString>         lstFmtActive;
    /** Flags of all current drag'n drop
     *  actions allowed. */
    uint32_t                   uAllActions;
    /** The startup information required
     *  for the actual DoDragDrop() call. */
    VBOXDNDSTARTUPINFO         startupInfo;
    /** Is the left mouse button being pressed
     *  currently while being in this window? */
    bool                       mfMouseButtonDown;
# ifdef VBOX_WITH_DRAG_AND_DROP_GH
    /** IDropTarget implementation for guest -> host
     *  support. */
    VBoxDnDDropTarget         *pDropTarget;
# endif /* VBOX_WITH_DRAG_AND_DROP_GH */
#else /* !RT_OS_WINDOWS */
    /** @todo Implement me. */
#endif /* !RT_OS_WINDOWS */

    /** The window's own DnD context. */
    VBGLR3GUESTDNDCMDCTX       mDnDCtx;
    /** The current operation mode. */
    Mode                       mMode;
    /** The current state. */
    State                      mState;
    /** Format being requested. */
    RTCString                  mFormatRequested;
};

#endif /* !__VBOXTRAYDND__H */

