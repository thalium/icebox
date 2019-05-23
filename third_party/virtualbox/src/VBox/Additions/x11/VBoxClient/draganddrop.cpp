/* $Id: draganddrop.cpp $ */
/** @file
 * X11 guest client - Drag and drop implementation.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#ifdef VBOX_DND_WITH_XTEST
# include <X11/extensions/XTest.h>
#endif

#include <iprt/asm.h>
#include <iprt/buildconfig.h>
#include <iprt/critsect.h>
#include <iprt/thread.h>
#include <iprt/time.h>

#include <iprt/cpp/mtlist.h>
#include <iprt/cpp/ministring.h>

#include <limits.h>

#ifdef LOG_GROUP
# undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_GUEST_DND
#include <VBox/log.h>
#include <VBox/VBoxGuestLib.h>

#include "VBox/HostServices/DragAndDropSvc.h"
#include "VBoxClient.h"

/* Enable this define to see the proxy window(s) when debugging
 * their behavior. Don't have this enabled in release builds! */
#ifdef DEBUG
//# define VBOX_DND_DEBUG_WND
#endif

/**
 * For X11 guest Xdnd is used. See http://www.acc.umu.se/~vatten/XDND.html for
 * a walk trough.
 *
 * Host -> Guest:
 *     For X11 this means mainly forwarding all the events from HGCM to the
 *     appropriate X11 events. There exists a proxy window, which is invisible and
 *     used for all the X11 communication. On a HGCM Enter event, we set our proxy
 *     window as XdndSelection owner with the given mime-types. On every HGCM move
 *     event, we move the X11 mouse cursor to the new position and query for the
 *     window below that position. Depending on if it is XdndAware, a new window or
 *     a known window, we send the appropriate X11 messages to it. On HGCM drop, we
 *     send a XdndDrop message to the current window and wait for a X11
 *     SelectionMessage from the target window. Because we didn't have the data in
 *     the requested mime-type, yet, we save that message and ask the host for the
 *     data. When the data is successfully received from the host, we put the data
 *     as a property to the window and send a X11 SelectionNotify event to the
 *     target window.
 *
 * Guest -> Host:
 *     This is a lot more trickery than H->G. When a pending event from HGCM
 *     arrives, we ask if there currently is an owner of the XdndSelection
 *     property. If so, our proxy window is shown (1x1, but without backing store)
 *     and some mouse event is triggered. This should be followed by an XdndEnter
 *     event send to the proxy window. From this event we can fetch the necessary
 *     info of the MIME types and allowed actions and send this back to the host.
 *     On a drop request from the host, we query for the selection and should get
 *     the data in the specified mime-type. This data is send back to the host.
 *     After that we send a XdndLeave event to the source window.
 *
 ** @todo Cancelling (e.g. with ESC key) doesn't work.
 ** @todo INCR (incremental transfers) support.
 ** @todo Really check for the Xdnd version and the supported features.
 ** @todo Either get rid of the xHelpers class or properly unify the code with the drag instance class.
 */

#define VBOX_XDND_VERSION    (4)
#define VBOX_MAX_XPROPERTIES (LONG_MAX-1)

/**
 * Structure for storing new X11 events and HGCM messages
 * into a single vent queue.
 */
struct DnDEvent
{
    enum DnDEventType
    {
        HGCM_Type = 1,
        X11_Type
    };
    DnDEventType type;
    union
    {
        VBGLR3DNDHGCMEVENT hgcm;
        XEvent x11;
    };
};

enum XA_Type
{
    /* States */
    XA_WM_STATE = 0,
    /* Properties */
    XA_TARGETS,
    XA_MULTIPLE,
    XA_INCR,
    /* Mime Types */
    XA_image_bmp,
    XA_image_jpg,
    XA_image_tiff,
    XA_image_png,
    XA_text_uri_list,
    XA_text_uri,
    XA_text_plain,
    XA_TEXT,
    /* Xdnd */
    XA_XdndSelection,
    XA_XdndAware,
    XA_XdndEnter,
    XA_XdndLeave,
    XA_XdndTypeList,
    XA_XdndActionList,
    XA_XdndPosition,
    XA_XdndActionCopy,
    XA_XdndActionMove,
    XA_XdndActionLink,
    XA_XdndStatus,
    XA_XdndDrop,
    XA_XdndFinished,
    /* Our own stop marker */
    XA_dndstop,
    /* End marker */
    XA_End
};

/**
 * Xdnd message value indexes, sorted by message type.
 */
typedef enum XdndMsg
{
    /** XdndEnter. */
    XdndEnterTypeCount = 3,         /* Maximum number of types in XdndEnter message. */

    XdndEnterWindow = 0,            /* Source window (sender). */
    XdndEnterFlags,                 /* Version in high byte, bit 0 => more data types. */
    XdndEnterType1,                 /* First available data type. */
    XdndEnterType2,                 /* Second available data type. */
    XdndEnterType3,                 /* Third available data type. */

    XdndEnterMoreTypesFlag = 1,     /* Set if there are more than XdndEnterTypeCount. */
    XdndEnterVersionRShift = 24,    /* Right shift to position version number. */
    XdndEnterVersionMask   = 0xFF,  /* Mask to get version after shifting. */

    /** XdndHere. */
    XdndHereWindow = 0,             /* Source window (sender). */
    XdndHereFlags,                  /* Reserved. */
    XdndHerePt,                     /* X + Y coordinates of mouse (root window coords). */
    XdndHereTimeStamp,              /* Timestamp for requesting data. */
    XdndHereAction,                 /* Action requested by user. */

    /** XdndPosition. */
    XdndPositionWindow = 0,         /* Source window (sender). */
    XdndPositionFlags,              /* Flags. */
    XdndPositionXY,                 /* X/Y coordinates of the mouse position relative to the root window. */
    XdndPositionTimeStamp,          /* Time stamp for retrieving the data. */
    XdndPositionAction,             /* Action requested by the user. */

    /** XdndStatus. */
    XdndStatusWindow = 0,           /* Target window (sender).*/
    XdndStatusFlags,                /* Flags returned by target. */
    XdndStatusNoMsgXY,              /* X + Y of "no msg" rectangle (root window coords). */
    XdndStatusNoMsgWH,              /* Width + height of "no msg" rectangle. */
    XdndStatusAction,               /* Action accepted by target. */

    XdndStatusAcceptDropFlag = 1,   /* Set if target will accept the drop. */
    XdndStatusSendHereFlag   = 2,   /* Set if target wants a stream of XdndPosition. */

    /** XdndLeave. */
    XdndLeaveWindow = 0,            /* Source window (sender). */
    XdndLeaveFlags,                 /* Reserved. */

    /** XdndDrop. */
    XdndDropWindow = 0,             /* Source window (sender). */
    XdndDropFlags,                  /* Reserved. */
    XdndDropTimeStamp,              /* Timestamp for requesting data. */

    /** XdndFinished. */
    XdndFinishedWindow = 0,         /* Target window (sender). */
    XdndFinishedFlags,              /* Version 5: Bit 0 is set if the current target accepted the drop. */
    XdndFinishedAction              /* Version 5: Contains the action performed by the target. */

} XdndMsg;

class DragAndDropService;

/** List of Atoms. */
#define VBoxDnDAtomList RTCList<Atom>

/*******************************************************************************
 *
 * xHelpers Declaration
 *
 ******************************************************************************/

class xHelpers
{
public:

    static xHelpers *getInstance(Display *pDisplay = 0)
    {
        if (!m_pInstance)
        {
            AssertPtrReturn(pDisplay, NULL);
            m_pInstance = new xHelpers(pDisplay);
        }

        return m_pInstance;
    }

    static void destroyInstance(void)
    {
        if (m_pInstance)
        {
            delete m_pInstance;
            m_pInstance = NULL;
        }
    }

    inline Display *display()    const { return m_pDisplay; }
    inline Atom xAtom(XA_Type e) const { return m_xAtoms[e]; }

    inline Atom stringToxAtom(const char *pcszString) const
    {
        return XInternAtom(m_pDisplay, pcszString, False);
    }
    inline RTCString xAtomToString(Atom atom) const
    {
        if (atom == None) return "None";

        char* pcsAtom = XGetAtomName(m_pDisplay, atom);
        RTCString strAtom(pcsAtom);
        XFree(pcsAtom);

        return strAtom;
    }

    inline RTCString xAtomListToString(const VBoxDnDAtomList &formatList)
    {
        RTCString format;
        for (size_t i = 0; i < formatList.size(); ++i)
            format += xAtomToString(formatList.at(i)) + "\r\n";
        return format;
    }

    RTCString xErrorToString(int xRc) const;
    Window applicationWindowBelowCursor(Window parentWin) const;

private:

    xHelpers(Display *pDisplay)
      : m_pDisplay(pDisplay)
    {
        /* Not all x11 atoms we use are defined in the headers. Create the
         * additional one we need here. */
        for (int i = 0; i < XA_End; ++i)
            m_xAtoms[i] = XInternAtom(m_pDisplay, m_xAtomNames[i], False);
    };

    /* Private member vars */
    static xHelpers   *m_pInstance;
    Display           *m_pDisplay;
    Atom               m_xAtoms[XA_End];
    static const char *m_xAtomNames[XA_End];
};

/* Some xHelpers convenience defines. */
#define gX11 xHelpers::getInstance()
#define xAtom(xa) gX11->xAtom((xa))
#define xAtomToString(xa) gX11->xAtomToString((xa))

/*******************************************************************************
 *
 * xHelpers Implementation
 *
 ******************************************************************************/

xHelpers *xHelpers::m_pInstance = NULL;

/* Has to be in sync with the XA_Type enum. */
const char *xHelpers::m_xAtomNames[] =
{
    /* States */
    "WM_STATE",
    /* Properties */
    "TARGETS",
    "MULTIPLE",
    "INCR",
    /* Mime Types */
    "image/bmp",
    "image/jpg",
    "image/tiff",
    "image/png",
    "text/uri-list",
    "text/uri",
    "text/plain",
    "TEXT",
    /* Xdnd */
    "XdndSelection",
    "XdndAware",
    "XdndEnter",
    "XdndLeave",
    "XdndTypeList",
    "XdndActionList",
    "XdndPosition",
    "XdndActionCopy",
    "XdndActionMove",
    "XdndActionLink",
    "XdndStatus",
    "XdndDrop",
    "XdndFinished",
    /* Our own stop marker */
    "dndstop"
};

RTCString xHelpers::xErrorToString(int xRc) const
{
    switch (xRc)
    {
        case Success:           return RTCStringFmt("%d (Success)", xRc);           break;
        case BadRequest:        return RTCStringFmt("%d (BadRequest)", xRc);        break;
        case BadValue:          return RTCStringFmt("%d (BadValue)", xRc);          break;
        case BadWindow:         return RTCStringFmt("%d (BadWindow)", xRc);         break;
        case BadPixmap:         return RTCStringFmt("%d (BadPixmap)", xRc);         break;
        case BadAtom:           return RTCStringFmt("%d (BadAtom)", xRc);           break;
        case BadCursor:         return RTCStringFmt("%d (BadCursor)", xRc);         break;
        case BadFont:           return RTCStringFmt("%d (BadFont)", xRc);           break;
        case BadMatch:          return RTCStringFmt("%d (BadMatch)", xRc);          break;
        case BadDrawable:       return RTCStringFmt("%d (BadDrawable)", xRc);       break;
        case BadAccess:         return RTCStringFmt("%d (BadAccess)", xRc);         break;
        case BadAlloc:          return RTCStringFmt("%d (BadAlloc)", xRc);          break;
        case BadColor:          return RTCStringFmt("%d (BadColor)", xRc);          break;
        case BadGC:             return RTCStringFmt("%d (BadGC)", xRc);             break;
        case BadIDChoice:       return RTCStringFmt("%d (BadIDChoice)", xRc);       break;
        case BadName:           return RTCStringFmt("%d (BadName)", xRc);           break;
        case BadLength:         return RTCStringFmt("%d (BadLength)", xRc);         break;
        case BadImplementation: return RTCStringFmt("%d (BadImplementation)", xRc); break;
    }
    return RTCStringFmt("%d (unknown)", xRc);
}

/** @todo Make this iterative. */
Window xHelpers::applicationWindowBelowCursor(Window wndParent) const
{
    /* No parent, nothing to do. */
    if(wndParent == 0)
        return 0;

    Window wndApp = 0;
    int cProps = -1;

    /* Fetch all x11 window properties of the parent window. */
    Atom *pProps = XListProperties(m_pDisplay, wndParent, &cProps);
    if (cProps > 0)
    {
        /* We check the window for the WM_STATE property. */
        for (int i = 0; i < cProps; ++i)
        {
            if (pProps[i] == xAtom(XA_WM_STATE))
            {
                /* Found it. */
                wndApp = wndParent;
                break;
            }
        }

        /* Cleanup */
        XFree(pProps);
    }

    if (!wndApp)
    {
        Window wndChild, wndTemp;
        int tmp;
        unsigned int utmp;

        /* Query the next child window of the parent window at the current
         * mouse position. */
        XQueryPointer(m_pDisplay, wndParent, &wndTemp, &wndChild, &tmp, &tmp, &tmp, &tmp, &utmp);

        /* Recursive call our self to dive into the child tree. */
        wndApp = applicationWindowBelowCursor(wndChild);
    }

    return wndApp;
}

/*******************************************************************************
 *
 * DragInstance Declaration
 *
 ******************************************************************************/

#ifdef DEBUG
# define VBOX_DND_FN_DECL_LOG(x) inline x /* For LogFlowXXX logging. */
#else
# define VBOX_DND_FN_DECL_LOG(x) x
#endif

/** @todo Move all proxy window-related stuff into this class! Clean up this mess. */
class VBoxDnDProxyWnd
{

public:

    VBoxDnDProxyWnd(void);

    virtual ~VBoxDnDProxyWnd(void);

public:

    int init(Display *pDisplay);
    void destroy();

    int sendFinished(Window hWndSource, uint32_t uAction);

public:

    Display *pDisp;
    /** Proxy window handle. */
    Window   hWnd;
    int      iX;
    int      iY;
    int      iWidth;
    int      iHeight;
};

/**
 * Class for handling a single drag and drop operation, that is,
 * one source and one target at a time.
 *
 * For now only one DragInstance will exits when the app is running.
 */
class DragInstance
{
public:

    enum State
    {
        Uninitialized = 0,
        Initialized,
        Dragging,
        Dropped,
        State_32BIT_Hack = 0x7fffffff
    };

    enum Mode
    {
        Unknown = 0,
        HG,
        GH,
        Mode_32Bit_Hack = 0x7fffffff
    };

    DragInstance(Display *pDisplay, DragAndDropService *pParent);

public:

    int  init(uint32_t u32ScreenId);
    void uninit(void);
    void reset(void);

    /* Logging. */
    VBOX_DND_FN_DECL_LOG(void) logInfo(const char *pszFormat, ...);
    VBOX_DND_FN_DECL_LOG(void) logError(const char *pszFormat, ...);

    /* X11 message processing. */
    int onX11ClientMessage(const XEvent &e);
    int onX11MotionNotify(const XEvent &e);
    int onX11SelectionClear(const XEvent &e);
    int onX11SelectionNotify(const XEvent &e);
    int onX11SelectionRequest(const XEvent &e);
    int onX11Event(const XEvent &e);
    int  waitForStatusChange(uint32_t enmState, RTMSINTERVAL uTimeoutMS = 30000);
    bool waitForX11Msg(XEvent &evX, int iType, RTMSINTERVAL uTimeoutMS = 100);
    bool waitForX11ClientMsg(XClientMessageEvent &evMsg, Atom aType, RTMSINTERVAL uTimeoutMS = 100);

    /* Host -> Guest handling. */
    int hgEnter(const RTCList<RTCString> &formats, uint32_t actions);
    int hgLeave(void);
    int hgMove(uint32_t u32xPos, uint32_t u32yPos, uint32_t uDefaultAction);
    int hgDrop(uint32_t u32xPos, uint32_t u32yPos, uint32_t uDefaultAction);
    int hgDataReceived(const void *pvData, uint32_t cData);

#ifdef VBOX_WITH_DRAG_AND_DROP_GH
    /* Guest -> Host handling. */
    int ghIsDnDPending(void);
    int ghDropped(const RTCString &strFormat, uint32_t action);
#endif

    /* X11 helpers. */
    int  mouseCursorFakeMove(void) const;
    int  mouseCursorMove(int iPosX, int iPosY) const;
    void mouseButtonSet(Window wndDest, int rx, int ry, int iButton, bool fPress);
    int proxyWinShow(int *piRootX = NULL, int *piRootY = NULL) const;
    int proxyWinHide(void);

    /* X11 window helpers. */
    char *wndX11GetNameA(Window wndThis) const;

    /* Xdnd protocol helpers. */
    void wndXDnDClearActionList(Window wndThis) const;
    void wndXDnDClearFormatList(Window wndThis) const;
    int wndXDnDGetActionList(Window wndThis, VBoxDnDAtomList &lstActions) const;
    int wndXDnDGetFormatList(Window wndThis, VBoxDnDAtomList &lstTypes) const;
    int wndXDnDSetActionList(Window wndThis, const VBoxDnDAtomList &lstActions) const;
    int wndXDnDSetFormatList(Window wndThis, Atom atmProp, const VBoxDnDAtomList &lstFormats) const;

    /* Atom / HGCM formatting helpers. */
    int             toAtomList(const RTCList<RTCString> &lstFormats, VBoxDnDAtomList &lstAtoms) const;
    int             toAtomList(const void *pvData, uint32_t cbData, VBoxDnDAtomList &lstAtoms) const;
    static Atom     toAtomAction(uint32_t uAction);
    static int      toAtomActions(uint32_t uActions, VBoxDnDAtomList &lstAtoms);
    static uint32_t toHGCMAction(Atom atom);
    static uint32_t toHGCMActions(const VBoxDnDAtomList &actionsList);

protected:

    /** The instance's own DnD context. */
    VBGLR3GUESTDNDCMDCTX        m_dndCtx;
    /** Pointer to service instance. */
    DragAndDropService         *m_pParent;
    /** Pointer to X display operating on. */
    Display                    *m_pDisplay;
    /** X screen ID to operate on. */
    int                         m_screenId;
    /** Pointer to X screen operating on. */
    Screen                     *m_pScreen;
    /** Root window handle. */
    Window                      m_wndRoot;
    /** Proxy window. */
    VBoxDnDProxyWnd             m_wndProxy;
    /** Current source/target window handle. */
    Window                      m_wndCur;
    /** The XDnD protocol version the current
     *  source/target window is using. */
    long                        m_curVer;
    /** List of (Atom) formats the source window supports. */
    VBoxDnDAtomList             m_lstFormats;
    /** List of (Atom) actions the source window supports. */
    VBoxDnDAtomList             m_lstActions;
    /** Buffer for answering the target window's selection request. */
    void                       *m_pvSelReqData;
    /** Size (in bytes) of selection request data buffer. */
    uint32_t                    m_cbSelReqData;
    /** Current operation mode. */
    volatile uint32_t           m_enmMode;
    /** Current state of operation mode. */
    volatile uint32_t           m_enmState;
    /** The instance's own X event queue. */
    RTCMTList<XEvent>           m_eventQueueList;
    /** Critical section for providing serialized access to list
     *  event queue's contents. */
    RTCRITSECT                  m_eventQueueCS;
    /** Event for notifying this instance in case of a new
     *  event. */
    RTSEMEVENT                  m_eventQueueEvent;
    /** Critical section for data access. */
    RTCRITSECT                  m_dataCS;
    /** List of allowed formats. */
    RTCList<RTCString>          m_lstAllowedFormats;
    /** Number of failed attempts by the host
     *  to query for an active drag and drop operation on the guest. */
    uint16_t                    m_cFailedPendingAttempts;
};

/*******************************************************************************
 *
 * DragAndDropService Declaration
 *
 ******************************************************************************/

class DragAndDropService
{
public:
    DragAndDropService(void)
      : m_pDisplay(NULL)
      , m_hHGCMThread(NIL_RTTHREAD)
      , m_hX11Thread(NIL_RTTHREAD)
      , m_hEventSem(NIL_RTSEMEVENT)
      , m_pCurDnD(NULL)
      , m_fSrvStopping(false)
    {}

    int init(void);
    int run(bool fDaemonised = false);
    void cleanup(void);

private:

    static DECLCALLBACK(int) hgcmEventThread(RTTHREAD hThread, void *pvUser);
    static DECLCALLBACK(int) x11EventThread(RTTHREAD hThread, void *pvUser);

    /* Private member vars */
    Display             *m_pDisplay;

    /** Our (thread-safe) event queue with
     *  mixed events (DnD HGCM / X11). */
    RTCMTList<DnDEvent>    m_eventQueue;
    /** Critical section for providing serialized access to list
     *  event queue's contents. */
    RTCRITSECT           m_eventQueueCS;
    RTTHREAD             m_hHGCMThread;
    RTTHREAD             m_hX11Thread;
    RTSEMEVENT           m_hEventSem;
    DragInstance        *m_pCurDnD;
    bool                 m_fSrvStopping;

    friend class DragInstance;
};

/*******************************************************************************
 *
 * DragInstanc Implementation
 *
 ******************************************************************************/

DragInstance::DragInstance(Display *pDisplay, DragAndDropService *pParent)
    : m_pParent(pParent)
    , m_pDisplay(pDisplay)
    , m_pScreen(0)
    , m_wndRoot(0)
    , m_wndCur(0)
    , m_curVer(-1)
    , m_pvSelReqData(NULL)
    , m_cbSelReqData(0)
    , m_enmMode(Unknown)
    , m_enmState(Uninitialized)
{
}

/**
 * Unitializes (destroys) this drag instance.
 */
void DragInstance::uninit(void)
{
    LogFlowFuncEnter();

    if (m_wndProxy.hWnd != 0)
        XDestroyWindow(m_pDisplay, m_wndProxy.hWnd);

    int rc2 = VbglR3DnDDisconnect(&m_dndCtx);

    if (m_pvSelReqData)
        RTMemFree(m_pvSelReqData);

    rc2 = RTSemEventDestroy(m_eventQueueEvent);
    AssertRC(rc2);

    rc2 = RTCritSectDelete(&m_eventQueueCS);
    AssertRC(rc2);

    rc2 = RTCritSectDelete(&m_dataCS);
    AssertRC(rc2);
}

/**
 * Resets this drag instance.
 */
void DragInstance::reset(void)
{
    LogFlowFuncEnter();

    /* Hide the proxy win. */
    proxyWinHide();

    int rc2 = RTCritSectEnter(&m_dataCS);
    if (RT_SUCCESS(rc2))
    {
        /* If we are currently the Xdnd selection owner, clear that. */
        Window pWnd = XGetSelectionOwner(m_pDisplay, xAtom(XA_XdndSelection));
        if (pWnd == m_wndProxy.hWnd)
            XSetSelectionOwner(m_pDisplay, xAtom(XA_XdndSelection), None, CurrentTime);

        /* Clear any other DnD specific data on the proxy window. */
        wndXDnDClearFormatList(m_wndProxy.hWnd);
        wndXDnDClearActionList(m_wndProxy.hWnd);

        /* Reset the internal state. */
        m_lstActions.clear();
        m_lstFormats.clear();
        m_wndCur    = 0;
        m_curVer    = -1;
        m_enmState  = Initialized;
        m_enmMode   = Unknown;
        m_eventQueueList.clear();
        m_cFailedPendingAttempts = 0;

        /* Reset the selection request buffer. */
        if (m_pvSelReqData)
        {
            RTMemFree(m_pvSelReqData);
            m_pvSelReqData = NULL;

            Assert(m_cbSelReqData);
            m_cbSelReqData = 0;
        }

        RTCritSectLeave(&m_dataCS);
    }
}

/**
 * Initializes this drag instance.
 *
 * @return  IPRT status code.
 * @param   u32ScreenID             X' screen ID to use.
 */
int DragInstance::init(uint32_t u32ScreenID)
{
    int rc = VbglR3DnDConnect(&m_dndCtx);
    /* Note: Can return VINF_PERMISSION_DENIED if HGCM host service is not available. */
    if (rc != VINF_SUCCESS)
        return rc;

    do
    {
        rc = RTSemEventCreate(&m_eventQueueEvent);
        if (RT_FAILURE(rc))
            break;

        rc = RTCritSectInit(&m_eventQueueCS);
        if (RT_FAILURE(rc))
            break;

        rc = RTCritSectInit(&m_dataCS);
        if (RT_FAILURE(rc))
            break;

        /*
         * Enough screens configured in the x11 server?
         */
        if ((int)u32ScreenID > ScreenCount(m_pDisplay))
        {
            rc = VERR_INVALID_PARAMETER;
            break;
        }
#if 0
        /* Get the screen number from the x11 server. */
        pDrag->screen = ScreenOfDisplay(m_pDisplay, u32ScreenId);
        if (!pDrag->screen)
        {
            rc = VERR_GENERAL_FAILURE;
            break;
        }
#endif
        m_screenId = u32ScreenID;

        /* Now query the corresponding root window of this screen. */
        m_wndRoot = RootWindow(m_pDisplay, m_screenId);
        if (!m_wndRoot)
        {
            rc = VERR_GENERAL_FAILURE;
            break;
        }

        /*
         * Create an invisible window which will act as proxy for the DnD
         * operation. This window will be used for both the GH and HG
         * direction.
         */
        XSetWindowAttributes attr;
        RT_ZERO(attr);
        attr.event_mask            =   EnterWindowMask  | LeaveWindowMask
                                     | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask;
        attr.override_redirect     = True;
        attr.do_not_propagate_mask = NoEventMask;
#ifdef VBOX_DND_DEBUG_WND
        attr.background_pixel      = XWhitePixel(m_pDisplay, m_screenId);
        attr.border_pixel          = XBlackPixel(m_pDisplay, m_screenId);
        m_wndProxy.hWnd = XCreateWindow(m_pDisplay, m_wndRoot                /* Parent */,
                                   100, 100,                                 /* Position */
                                   100, 100,                                 /* Width + height */
                                   2,                                        /* Border width */
                                   CopyFromParent,                           /* Depth */
                                   InputOutput,                              /* Class */
                                   CopyFromParent,                           /* Visual */
                                     CWBackPixel
                                   | CWBorderPixel
                                   | CWOverrideRedirect
                                   | CWDontPropagate,                        /* Value mask */
                                   &attr);                                   /* Attributes for value mask */
#else
        m_wndProxy.hWnd = XCreateWindow(m_pDisplay, m_wndRoot            /* Parent */,
                                   0, 0,                                 /* Position */
                                   1, 1,                                 /* Width + height */
                                   0,                                    /* Border width */
                                   CopyFromParent,                       /* Depth */
                                   InputOnly,                            /* Class */
                                   CopyFromParent,                       /* Visual */
                                   CWOverrideRedirect | CWDontPropagate, /* Value mask */
                                   &attr);                               /* Attributes for value mask */
#endif
        if (!m_wndProxy.hWnd)
        {
            LogRel(("DnD: Error creating proxy window\n"));
            rc = VERR_GENERAL_FAILURE;
            break;
        }

        rc = m_wndProxy.init(m_pDisplay);
        if (RT_FAILURE(rc))
        {
            LogRel(("DnD: Error initializing proxy window, rc=%Rrc\n", rc));
            break;
        }

#ifdef VBOX_DND_DEBUG_WND
        XFlush(m_pDisplay);
        XMapWindow(m_pDisplay, m_wndProxy.hWnd);
        XRaiseWindow(m_pDisplay, m_wndProxy.hWnd);
        XFlush(m_pDisplay);
#endif
        logInfo("Proxy window=0x%x, root window=0x%x ...\n", m_wndProxy.hWnd, m_wndRoot);

        /* Set the window's name for easier lookup. */
        XStoreName(m_pDisplay, m_wndProxy.hWnd, "VBoxClientWndDnD");

        /* Make the new window Xdnd aware. */
        Atom ver = VBOX_XDND_VERSION;
        XChangeProperty(m_pDisplay, m_wndProxy.hWnd, xAtom(XA_XdndAware), XA_ATOM, 32, PropModeReplace,
                        reinterpret_cast<unsigned char*>(&ver), 1);
    } while (0);

    if (RT_SUCCESS(rc))
    {
        reset();
    }
    else
        logError("Initializing drag instance for screen %RU32 failed with rc=%Rrc\n", u32ScreenID, rc);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Logs an error message to the (release) logging instance.
 *
 * @param   pszFormat               Format string to log.
 */
VBOX_DND_FN_DECL_LOG(void) DragInstance::logError(const char *pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    char *psz = NULL;
    RTStrAPrintfV(&psz, pszFormat, args);
    va_end(args);

    AssertPtr(psz);
    LogFlowFunc(("%s", psz));
    LogRel(("DnD: %s", psz));

    RTStrFree(psz);
}

/**
 * Logs an info message to the (release) logging instance.
 *
 * @param   pszFormat               Format string to log.
 */
VBOX_DND_FN_DECL_LOG(void) DragInstance::logInfo(const char *pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    char *psz = NULL;
    RTStrAPrintfV(&psz, pszFormat, args);
    va_end(args);

    AssertPtr(psz);
    LogFlowFunc(("%s", psz));
    LogRel2(("DnD: %s", psz));

    RTStrFree(psz);
}

/**
 * Callback handler for a generic client message from a window.
 *
 * @return  IPRT status code.
 * @param   e                       X11 event to handle.
 */
int DragInstance::onX11ClientMessage(const XEvent &e)
{
    AssertReturn(e.type == ClientMessage, VERR_INVALID_PARAMETER);

    LogFlowThisFunc(("mode=%RU32, state=%RU32\n", m_enmMode, m_enmState));
    LogFlowThisFunc(("Event wnd=%#x, msg=%s\n", e.xclient.window, xAtomToString(e.xclient.message_type).c_str()));

    int rc = VINF_SUCCESS;

    switch (m_enmMode)
    {
        case HG:
        {
            /*
             * Client messages are used to inform us about the status of a XdndAware
             * window, in response of some events we send to them.
             */
            if (   e.xclient.message_type == xAtom(XA_XdndStatus)
                && m_wndCur               == static_cast<Window>(e.xclient.data.l[XdndStatusWindow]))
            {
                bool fAcceptDrop     = ASMBitTest   (&e.xclient.data.l[XdndStatusFlags], 0); /* Does the target accept the drop? */
                RTCString strActions = xAtomToString( e.xclient.data.l[XdndStatusAction]);
#ifdef LOG_ENABLED
                bool fWantsPosition  = ASMBitTest   (&e.xclient.data.l[XdndStatusFlags], 1); /* Does the target want XdndPosition messages? */
                char *pszWndName = wndX11GetNameA(e.xclient.data.l[XdndStatusWindow]);
                AssertPtr(pszWndName);

                /*
                 * The XdndStatus message tell us if the window will accept the DnD
                 * event and with which action. We immediately send this info down to
                 * the host as a response of a previous DnD message.
                 */
                LogFlowThisFunc(("XA_XdndStatus: wnd=%#x ('%s'), fAcceptDrop=%RTbool, fWantsPosition=%RTbool, strActions=%s\n",
                                 e.xclient.data.l[XdndStatusWindow], pszWndName, fAcceptDrop, fWantsPosition, strActions.c_str()));

                RTStrFree(pszWndName);

                uint16_t x  = RT_HI_U16((uint32_t)e.xclient.data.l[XdndStatusNoMsgXY]);
                uint16_t y  = RT_LO_U16((uint32_t)e.xclient.data.l[XdndStatusNoMsgXY]);
                uint16_t cx = RT_HI_U16((uint32_t)e.xclient.data.l[XdndStatusNoMsgWH]);
                uint16_t cy = RT_LO_U16((uint32_t)e.xclient.data.l[XdndStatusNoMsgWH]);
                LogFlowThisFunc(("\tReported dead area: x=%RU16, y=%RU16, cx=%RU16, cy=%RU16\n", x, y, cx, cy));
#endif

                uint32_t uAction = DND_IGNORE_ACTION; /* Default is ignoring. */
                /** @todo Compare this with the allowed actions. */
                if (fAcceptDrop)
                    uAction = toHGCMAction(static_cast<Atom>(e.xclient.data.l[XdndStatusAction]));

                rc = VbglR3DnDHGSendAckOp(&m_dndCtx, uAction);
            }
            else if (e.xclient.message_type == xAtom(XA_XdndFinished))
            {
#ifdef LOG_ENABLED
                bool fSucceeded = ASMBitTest(&e.xclient.data.l[XdndFinishedFlags], 0);

                char *pszWndName = wndX11GetNameA(e.xclient.data.l[XdndFinishedWindow]);
                AssertPtr(pszWndName);

                /* This message is sent on an un/successful DnD drop request. */
                LogFlowThisFunc(("XA_XdndFinished: wnd=%#x ('%s'), success=%RTbool, action=%s\n",
                                 e.xclient.data.l[XdndFinishedWindow], pszWndName, fSucceeded,
                                 xAtomToString(e.xclient.data.l[XdndFinishedAction]).c_str()));

                RTStrFree(pszWndName);
#endif

                reset();
            }
            else
            {
                char *pszWndName = wndX11GetNameA(e.xclient.data.l[0]);
                AssertPtr(pszWndName);
                LogFlowThisFunc(("Unhandled: wnd=%#x ('%s'), msg=%s\n",
                                 e.xclient.data.l[0], pszWndName, xAtomToString(e.xclient.message_type).c_str()));
                RTStrFree(pszWndName);

                rc = VERR_NOT_SUPPORTED;
            }

            break;
        }

        case Unknown: /* Mode not set (yet). */
        case GH:
        {
            /*
             * This message marks the beginning of a new drag and drop
             * operation on the guest.
             */
            if (e.xclient.message_type == xAtom(XA_XdndEnter))
            {
                LogFlowFunc(("XA_XdndEnter\n"));

                /*
                 * Get the window which currently has the XA_XdndSelection
                 * bit set.
                 */
                Window wndSelection = XGetSelectionOwner(m_pDisplay, xAtom(XA_XdndSelection));

                char *pszWndName = wndX11GetNameA(wndSelection);
                AssertPtr(pszWndName);
                LogFlowThisFunc(("wndSelection=%#x ('%s'), wndProxy=%#x\n", wndSelection, pszWndName, m_wndProxy.hWnd));
                RTStrFree(pszWndName);

                mouseButtonSet(m_wndProxy.hWnd, -1, -1, 1, true /* fPress */);

                /*
                 * Update our state and the window handle to process.
                 */
                int rc2 = RTCritSectEnter(&m_dataCS);
                if (RT_SUCCESS(rc2))
                {
                    m_wndCur = wndSelection;
                    m_curVer = e.xclient.data.l[XdndEnterFlags] >> XdndEnterVersionRShift;
                    Assert(m_wndCur == (Window)e.xclient.data.l[XdndEnterWindow]); /* Source window. */
#ifdef DEBUG
                    XWindowAttributes xwa;
                    XGetWindowAttributes(m_pDisplay, m_wndCur, &xwa);
                    LogFlowThisFunc(("wndCur=%#x, x=%d, y=%d, width=%d, height=%d\n", m_wndCur, xwa.x, xwa.y, xwa.width, xwa.height));
#endif
                    /*
                     * Retrieve supported formats.
                     */

                    /* Check if the MIME types are in the message itself or if we need
                     * to fetch the XdndTypeList property from the window. */
                    bool fMoreTypes = e.xclient.data.l[XdndEnterFlags] & XdndEnterMoreTypesFlag;
                    LogFlowThisFunc(("XdndVer=%d, fMoreTypes=%RTbool\n", m_curVer, fMoreTypes));
                    if (!fMoreTypes)
                    {
                        /* Only up to 3 format types supported. */
                        /* Start with index 2 (first item). */
                        for (int i = 2; i < 5; i++)
                        {
                            LogFlowThisFunc(("\t%s\n", gX11->xAtomToString(e.xclient.data.l[i]).c_str()));
                            m_lstFormats.append(e.xclient.data.l[i]);
                        }
                    }
                    else
                    {
                        /* More than 3 format types supported. */
                        rc = wndXDnDGetFormatList(wndSelection, m_lstFormats);
                    }

                    /*
                     * Retrieve supported actions.
                     */
                    if (RT_SUCCESS(rc))
                    {
                        if (m_curVer >= 2) /* More than one action allowed since protocol version 2. */
                        {
                            rc = wndXDnDGetActionList(wndSelection, m_lstActions);
                        }
                        else /* Only "copy" action allowed on legacy applications. */
                            m_lstActions.append(XA_XdndActionCopy);
                    }

                    if (RT_SUCCESS(rc))
                    {
                        m_enmMode  = GH;
                        m_enmState = Dragging;
                    }

                    RTCritSectLeave(&m_dataCS);
                }
            }
            else if (   e.xclient.message_type == xAtom(XA_XdndPosition)
                     && m_wndCur               == static_cast<Window>(e.xclient.data.l[XdndPositionWindow]))
            {
                if (m_enmState != Dragging) /* Wrong mode? Bail out. */
                {
                    reset();
                    break;
                }
#ifdef LOG_ENABLED
                int32_t iPos      = e.xclient.data.l[XdndPositionXY];
                Atom    atmAction = m_curVer >= 2 /* Actions other than "copy" or only supported since protocol version 2. */
                                  ? e.xclient.data.l[XdndPositionAction] : xAtom(XA_XdndActionCopy);
                LogFlowThisFunc(("XA_XdndPosition: wndProxy=%#x, wndCur=%#x, x=%RI32, y=%RI32, strAction=%s\n",
                                 m_wndProxy.hWnd, m_wndCur, RT_HIWORD(iPos), RT_LOWORD(iPos),
                                 xAtomToString(atmAction).c_str()));
#endif

                bool fAcceptDrop = true;

                /* Reply with a XdndStatus message to tell the source whether
                 * the data can be dropped or not. */
                XClientMessageEvent m;
                RT_ZERO(m);
                m.type         = ClientMessage;
                m.display      = m_pDisplay;
                m.window       = e.xclient.data.l[XdndPositionWindow];
                m.message_type = xAtom(XA_XdndStatus);
                m.format       = 32;
                m.data.l[XdndStatusWindow]  = m_wndProxy.hWnd;
                m.data.l[XdndStatusFlags]   = fAcceptDrop ? RT_BIT(0) : 0; /* Whether to accept the drop or not. */

                /* We don't want any new XA_XdndPosition messages while being
                 * in our proxy window. */
                m.data.l[XdndStatusNoMsgXY] = RT_MAKE_U32(m_wndProxy.iY, m_wndProxy.iX);
                m.data.l[XdndStatusNoMsgWH] = RT_MAKE_U32(m_wndProxy.iHeight, m_wndProxy.iWidth);

                /** @todo Handle default action! */
                m.data.l[XdndStatusAction]  = fAcceptDrop ? toAtomAction(DND_COPY_ACTION) : None;

                int xRc = XSendEvent(m_pDisplay, e.xclient.data.l[XdndPositionWindow],
                                     False /* Propagate */, NoEventMask, reinterpret_cast<XEvent *>(&m));
                if (xRc == 0)
                    logError("Error sending position XA_XdndStatus event to current window=%#x: %s\n",
                              m_wndCur, gX11->xErrorToString(xRc).c_str());
            }
            else if (   e.xclient.message_type == xAtom(XA_XdndLeave)
                     && m_wndCur               == static_cast<Window>(e.xclient.data.l[XdndLeaveWindow]))
            {
                LogFlowThisFunc(("XA_XdndLeave\n"));
                logInfo("Guest to host transfer canceled by the guest source window\n");

                /* Start over. */
                reset();
            }
            else if (   e.xclient.message_type == xAtom(XA_XdndDrop)
                     && m_wndCur               == static_cast<Window>(e.xclient.data.l[XdndDropWindow]))
            {
                LogFlowThisFunc(("XA_XdndDrop\n"));

                if (m_enmState != Dropped) /* Wrong mode? Bail out. */
                {
                    /* Can occur when dragging from guest->host, but then back in to the guest again. */
                    logInfo("Could not drop on own proxy window\n"); /* Not fatal. */

                    /* Let the source know. */
                    rc = m_wndProxy.sendFinished(m_wndCur, DND_IGNORE_ACTION);

                    /* Start over. */
                    reset();
                    break;
                }

                m_eventQueueList.append(e);
                rc = RTSemEventSignal(m_eventQueueEvent);
            }
            else
            {
                logInfo("Unhandled event from wnd=%#x, msg=%s\n", e.xclient.window, xAtomToString(e.xclient.message_type).c_str());

                /* Let the source know. */
                rc = m_wndProxy.sendFinished(m_wndCur, DND_IGNORE_ACTION);

                /* Start over. */
                reset();
            }
            break;
        }

        default:
        {
            AssertMsgFailed(("Drag and drop mode not implemented: %RU32\n", m_enmMode));
            rc = VERR_NOT_IMPLEMENTED;
            break;
        }
    }

    LogFlowThisFunc(("Returning rc=%Rrc\n", rc));
    return rc;
}

int DragInstance::onX11MotionNotify(const XEvent &e)
{
    RT_NOREF1(e);
    LogFlowThisFunc(("mode=%RU32, state=%RU32\n", m_enmMode, m_enmState));

    return VINF_SUCCESS;
}

/**
 * Callback handler for being notified if some other window now
 * is the owner of the current selection.
 *
 * @return  IPRT status code.
 * @param   e                       X11 event to handle.
 *
 * @remark
 */
int DragInstance::onX11SelectionClear(const XEvent &e)
{
    RT_NOREF1(e);
    LogFlowThisFunc(("mode=%RU32, state=%RU32\n", m_enmMode, m_enmState));

    return VINF_SUCCESS;
}

/**
 * Callback handler for a XDnD selection notify from a window. This is needed
 * to let the us know if a certain window has drag'n drop data to share with us,
 * e.g. our proxy window.
 *
 * @return  IPRT status code.
 * @param   e                       X11 event to handle.
 */
int DragInstance::onX11SelectionNotify(const XEvent &e)
{
    AssertReturn(e.type == SelectionNotify, VERR_INVALID_PARAMETER);

    LogFlowThisFunc(("mode=%RU32, state=%RU32\n", m_enmMode, m_enmState));

    int rc;

    switch (m_enmMode)
    {
        case GH:
        {
            if (m_enmState == Dropped)
            {
                m_eventQueueList.append(e);
                rc = RTSemEventSignal(m_eventQueueEvent);
            }
            else
                rc = VERR_WRONG_ORDER;
            break;
        }

        default:
        {
            LogFlowThisFunc(("Unhandled: wnd=%#x, msg=%s\n",
                             e.xclient.data.l[0], xAtomToString(e.xclient.message_type).c_str()));
            rc = VERR_INVALID_STATE;
            break;
        }
    }

    LogFlowThisFunc(("Returning rc=%Rrc\n", rc));
    return rc;
}

/**
 * Callback handler for a XDnD selection request from a window. This is needed
 * to retrieve the data required to complete the actual drag'n drop operation.
 *
 * @returns IPRT status code.
 * @param   e                       X11 event to handle.
 */
int DragInstance::onX11SelectionRequest(const XEvent &e)
{
    AssertReturn(e.type == SelectionRequest, VERR_INVALID_PARAMETER);

    LogFlowThisFunc(("mode=%RU32, state=%RU32\n", m_enmMode, m_enmState));
    LogFlowThisFunc(("Event owner=%#x, requestor=%#x, selection=%s, target=%s, prop=%s, time=%u\n",
                     e.xselectionrequest.owner,
                     e.xselectionrequest.requestor,
                     xAtomToString(e.xselectionrequest.selection).c_str(),
                     xAtomToString(e.xselectionrequest.target).c_str(),
                     xAtomToString(e.xselectionrequest.property).c_str(),
                     e.xselectionrequest.time));
    int rc;

    switch (m_enmMode)
    {
        case HG:
        {
            rc = VINF_SUCCESS;

            char *pszWndName = wndX11GetNameA(e.xselectionrequest.requestor);
            AssertPtr(pszWndName);

            /*
             * Start by creating a refusal selection notify message.
             * That way we only need to care for the success case.
             */

            XEvent s;
            RT_ZERO(s);
            s.xselection.type      = SelectionNotify;
            s.xselection.display   = e.xselectionrequest.display;
            s.xselection.requestor = e.xselectionrequest.requestor;
            s.xselection.selection = e.xselectionrequest.selection;
            s.xselection.target    = e.xselectionrequest.target;
            s.xselection.property  = None;                          /* "None" means refusal. */
            s.xselection.time      = e.xselectionrequest.time;

            const XSelectionRequestEvent *pReq = &e.xselectionrequest;

#ifdef DEBUG
            LogFlowFunc(("Supported formats:\n"));
            for (size_t i = 0; i < m_lstFormats.size(); i++)
                LogFlowFunc(("\t%s\n", xAtomToString(m_lstFormats.at(i)).c_str()));
#endif
            /* Is the requestor asking for the possible MIME types? */
            if (pReq->target == xAtom(XA_TARGETS))
            {
                logInfo("Target window %#x ('%s') asking for target list\n", e.xselectionrequest.requestor, pszWndName);

                /* If so, set the window property with the formats on the requestor
                 * window. */
                rc = wndXDnDSetFormatList(pReq->requestor, pReq->property, m_lstFormats);
                if (RT_SUCCESS(rc))
                    s.xselection.property = pReq->property;
            }
            /* Is the requestor asking for a specific MIME type (we support)? */
            else if (m_lstFormats.contains(pReq->target))
            {
                logInfo("Target window %#x ('%s') is asking for data as '%s'\n",
                         pReq->requestor, pszWndName, xAtomToString(pReq->target).c_str());

                /* Did we not drop our stuff to the guest yet? Bail out. */
                if (m_enmState != Dropped)
                {
                    LogFlowThisFunc(("Wrong state (%RU32), refusing request\n", m_enmState));
                }
                /* Did we not store the requestor's initial selection request yet? Then do so now. */
                else
                {
                    /* Get the data format the requestor wants from us. */
                    RTCString strFormat = xAtomToString(pReq->target);
                    Assert(strFormat.isNotEmpty());
                    logInfo("Target window=%#x requested data from host as '%s', rc=%Rrc\n",
                            pReq->requestor, strFormat.c_str(), rc);

                    /* Make a copy of the MIME data to be passed back. The X server will be become
                     * the new owner of that data, so no deletion needed. */
                    /** @todo Do we need to do some more conversion here? XConvertSelection? */
                    void *pvData = RTMemDup(m_pvSelReqData, m_cbSelReqData);
                    uint32_t cbData = m_cbSelReqData;

                    /* Always return the requested property. */
                    s.xselection.property = pReq->property;

                    /* Note: Always seems to return BadRequest. Seems fine. */
                    int xRc = XChangeProperty(s.xselection.display, s.xselection.requestor, s.xselection.property,
                                              s.xselection.target, 8, PropModeReplace,
                                              reinterpret_cast<const unsigned char*>(pvData), cbData);

                    LogFlowFunc(("Changing property '%s' (target '%s') of window=0x%x: %s\n",
                                 xAtomToString(pReq->property).c_str(),
                                 xAtomToString(pReq->target).c_str(),
                                 pReq->requestor,
                                 gX11->xErrorToString(xRc).c_str()));
                    NOREF(xRc);
                }
            }
            /* Anything else. */
            else
            {
                logError("Refusing unknown command/format '%s' of wnd=%#x ('%s')\n",
                         xAtomToString(e.xselectionrequest.target).c_str(), pReq->requestor, pszWndName);
                rc = VERR_NOT_SUPPORTED;
            }

            LogFlowThisFunc(("Offering type '%s', property '%s' to wnd=%#x ...\n",
                             xAtomToString(pReq->target).c_str(),
                             xAtomToString(pReq->property).c_str(), pReq->requestor));

            int xRc = XSendEvent(pReq->display, pReq->requestor, True /* Propagate */, 0, &s);
            if (xRc == 0)
                logError("Error sending SelectionNotify(1) event to wnd=%#x: %s\n", pReq->requestor,
                         gX11->xErrorToString(xRc).c_str());
            XFlush(pReq->display);

            if (pszWndName)
                RTStrFree(pszWndName);
            break;
        }

        default:
            rc = VERR_INVALID_STATE;
            break;
    }

    LogFlowThisFunc(("Returning rc=%Rrc\n", rc));
    return rc;
}

/**
 * Handles X11 events, called by x11EventThread.
 *
 * @returns IPRT status code.
 * @param   e                       X11 event to handle.
 */
int DragInstance::onX11Event(const XEvent &e)
{
    int rc;

    LogFlowThisFunc(("X11 event, type=%d\n", e.type));
    switch (e.type)
    {
        /*
         * This can happen if a guest->host drag operation
         * goes back from the host to the guest. This is not what
         * we want and thus resetting everything.
         */
        case ButtonPress:
        case ButtonRelease:
            LogFlowThisFunc(("Mouse button press/release\n"));
            rc = VINF_SUCCESS;

            reset();
            break;

        case ClientMessage:
            rc = onX11ClientMessage(e);
            break;

        case SelectionClear:
            rc = onX11SelectionClear(e);
            break;

        case SelectionNotify:
            rc = onX11SelectionNotify(e);
            break;

        case SelectionRequest:
            rc = onX11SelectionRequest(e);
            break;

        case MotionNotify:
            rc = onX11MotionNotify(e);
            break;

        default:
            rc = VERR_NOT_IMPLEMENTED;
            break;
    }

    LogFlowThisFunc(("rc=%Rrc\n", rc));
    return rc;
}

int DragInstance::waitForStatusChange(uint32_t enmState, RTMSINTERVAL uTimeoutMS /* = 30000 */)
{
    const uint64_t uiStart = RTTimeMilliTS();
    volatile uint32_t enmCurState;

    int rc = VERR_TIMEOUT;

    LogFlowFunc(("enmState=%RU32, uTimeoutMS=%RU32\n", enmState, uTimeoutMS));

    do
    {
        enmCurState = ASMAtomicReadU32(&m_enmState);
        if (enmCurState == enmState)
        {
            rc = VINF_SUCCESS;
            break;
        }
    }
    while (RTTimeMilliTS() - uiStart < uTimeoutMS);

    LogFlowThisFunc(("Returning %Rrc\n", rc));
    return rc;
}

#ifdef VBOX_WITH_DRAG_AND_DROP_GH
/**
 * Waits for an X11 event of a specific type.
 *
 * @returns IPRT status code.
 * @param   evX                     Reference where to store the event into.
 * @param   iType                   Event type to wait for.
 * @param   uTimeoutMS              Timeout (in ms) to wait for the event.
 */
bool DragInstance::waitForX11Msg(XEvent &evX, int iType, RTMSINTERVAL uTimeoutMS /* = 100 */)
{
    LogFlowThisFunc(("iType=%d, uTimeoutMS=%RU32, cEventQueue=%zu\n", iType, uTimeoutMS, m_eventQueueList.size()));

    bool fFound = false;
    const uint64_t uiStart = RTTimeMilliTS();

    do
    {
        /* Check if there is a client message in the queue. */
        for (size_t i = 0; i < m_eventQueueList.size(); i++)
        {
            int rc2 = RTCritSectEnter(&m_eventQueueCS);
            if (RT_SUCCESS(rc2))
            {
                XEvent e = m_eventQueueList.at(i);

                fFound = e.type == iType;
                if (fFound)
                {
                    m_eventQueueList.removeAt(i);
                    evX = e;
                }

                rc2 = RTCritSectLeave(&m_eventQueueCS);
                AssertRC(rc2);

                if (fFound)
                    break;
            }
        }

        if (fFound)
            break;

        int rc2 = RTSemEventWait(m_eventQueueEvent, 25 /* ms */);
        if (   RT_FAILURE(rc2)
            && rc2 != VERR_TIMEOUT)
        {
            LogFlowFunc(("Waiting failed with rc=%Rrc\n", rc2));
            break;
        }
    }
    while (RTTimeMilliTS() - uiStart < uTimeoutMS);

    LogFlowThisFunc(("Returning fFound=%RTbool, msRuntime=%RU64\n", fFound, RTTimeMilliTS() - uiStart));
    return fFound;
}

/**
 * Waits for an X11 client message of a specific type.
 *
 * @returns IPRT status code.
 * @param   evMsg                   Reference where to store the event into.
 * @param   aType                   Event type to wait for.
 * @param   uTimeoutMS              Timeout (in ms) to wait for the event.
 */
bool DragInstance::waitForX11ClientMsg(XClientMessageEvent &evMsg, Atom aType,
                                       RTMSINTERVAL uTimeoutMS /* = 100 */)
{
    LogFlowThisFunc(("aType=%s, uTimeoutMS=%RU32, cEventQueue=%zu\n",
                     xAtomToString(aType).c_str(), uTimeoutMS, m_eventQueueList.size()));

    bool fFound = false;
    const uint64_t uiStart = RTTimeMilliTS();
    do
    {
        /* Check if there is a client message in the queue. */
        for (size_t i = 0; i < m_eventQueueList.size(); i++)
        {
            int rc2 = RTCritSectEnter(&m_eventQueueCS);
            if (RT_SUCCESS(rc2))
            {
                XEvent e = m_eventQueueList.at(i);
                if (   e.type                 == ClientMessage
                    && e.xclient.message_type == aType)
                {
                    m_eventQueueList.removeAt(i);
                    evMsg = e.xclient;

                    fFound = true;
                }

                if (e.type == ClientMessage)
                {
                    LogFlowThisFunc(("Client message: Type=%ld (%s)\n",
                                     e.xclient.message_type, xAtomToString(e.xclient.message_type).c_str()));
                }
                else
                    LogFlowThisFunc(("X message: Type=%d\n", e.type));

                rc2 = RTCritSectLeave(&m_eventQueueCS);
                AssertRC(rc2);

                if (fFound)
                    break;
            }
        }

        if (fFound)
            break;

        int rc2 = RTSemEventWait(m_eventQueueEvent, 25 /* ms */);
        if (   RT_FAILURE(rc2)
            && rc2 != VERR_TIMEOUT)
        {
            LogFlowFunc(("Waiting failed with rc=%Rrc\n", rc2));
            break;
        }
    }
    while (RTTimeMilliTS() - uiStart < uTimeoutMS);

    LogFlowThisFunc(("Returning fFound=%RTbool, msRuntime=%RU64\n", fFound, RTTimeMilliTS() - uiStart));
    return fFound;
}
#endif /* VBOX_WITH_DRAG_AND_DROP_GH */

/*
 * Host -> Guest
 */

/**
 * Host -> Guest: Event signalling that the host's (mouse) cursor just entered the VM's (guest's) display
 *                area.
 *
 * @returns IPRT status code.
 * @param   lstFormats              List of supported formats from the host.
 * @param   uActions                (ORed) List of supported actions from the host.
 */
int DragInstance::hgEnter(const RTCList<RTCString> &lstFormats, uint32_t uActions)
{
    LogFlowThisFunc(("mode=%RU32, state=%RU32\n", m_enmMode, m_enmState));

    if (m_enmMode != Unknown)
        return VERR_INVALID_STATE;

    reset();

#ifdef DEBUG
    LogFlowThisFunc(("uActions=0x%x, lstFormats=%zu: ", uActions, lstFormats.size()));
    for (size_t i = 0; i < lstFormats.size(); ++i)
        LogFlow(("'%s' ", lstFormats.at(i).c_str()));
    LogFlow(("\n"));
#endif

    int rc;

    do
    {
        rc = toAtomList(lstFormats, m_lstFormats);
        if (RT_FAILURE(rc))
            break;

        /* If we have more than 3 formats we have to use the type list extension. */
        if (m_lstFormats.size() > 3)
        {
            rc = wndXDnDSetFormatList(m_wndProxy.hWnd, xAtom(XA_XdndTypeList), m_lstFormats);
            if (RT_FAILURE(rc))
                break;
        }

        /* Announce the possible actions. */
        VBoxDnDAtomList lstActions;
        rc = toAtomActions(uActions, lstActions);
        if (RT_FAILURE(rc))
            break;
        rc = wndXDnDSetActionList(m_wndProxy.hWnd, lstActions);

        /* Set the DnD selection owner to our window. */
        /** @todo Don't use CurrentTime -- according to ICCCM section 2.1. */
        XSetSelectionOwner(m_pDisplay, xAtom(XA_XdndSelection), m_wndProxy.hWnd, CurrentTime);

        m_enmMode  = HG;
        m_enmState = Dragging;

    } while (0);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Host -> Guest: Event signalling that the host's (mouse) cursor has left the VM's (guest's)
 *                display area.
 */
int DragInstance::hgLeave(void)
{
    if (m_enmMode == HG) /* Only reset if in the right operation mode. */
        reset();

    return VINF_SUCCESS;
}

/**
 * Host -> Guest: Event signalling that the host's (mouse) cursor has been moved within the VM's
 *                (guest's) display area.
 *
 * @returns IPRT status code.
 * @param   u32xPos                 Relative X position within the guest's display area.
 * @param   u32yPos                 Relative Y position within the guest's display area.
 * @param   uDefaultAction          Default action the host wants to perform on the guest
 *                                  as soon as the operation successfully finishes.
 */
int DragInstance::hgMove(uint32_t u32xPos, uint32_t u32yPos, uint32_t uDefaultAction)
{
    LogFlowThisFunc(("mode=%RU32, state=%RU32\n", m_enmMode, m_enmState));
    LogFlowThisFunc(("u32xPos=%RU32, u32yPos=%RU32, uAction=%RU32\n", u32xPos, u32yPos, uDefaultAction));

    if (   m_enmMode  != HG
        || m_enmState != Dragging)
    {
        return VERR_INVALID_STATE;
    }

    int rc  = VINF_SUCCESS;
    int xRc = Success;

    /* Move the mouse cursor within the guest. */
    mouseCursorMove(u32xPos, u32yPos);

    long newVer = -1; /* This means the current window is _not_ XdndAware. */

    /* Search for the application window below the cursor. */
    Window wndCursor = gX11->applicationWindowBelowCursor(m_wndRoot);
    if (wndCursor != None)
    {
        /* Temp stuff for the XGetWindowProperty call. */
        Atom atmp;
        int fmt;
        unsigned long cItems, cbRemaining;
        unsigned char *pcData = NULL;

        /* Query the XdndAware property from the window. We are interested in
         * the version and if it is XdndAware at all. */
        xRc = XGetWindowProperty(m_pDisplay, wndCursor, xAtom(XA_XdndAware),
                                 0, 2, False, AnyPropertyType,
                                 &atmp, &fmt, &cItems, &cbRemaining, &pcData);
        if (xRc != Success)
        {
            logError("Error getting properties of cursor window=%#x: %s\n", wndCursor, gX11->xErrorToString(xRc).c_str());
        }
        else
        {
            if (pcData == NULL || fmt != 32 || cItems != 1)
            {
                /** @todo Do we need to deal with this? */
                logError("Wrong window properties for window %#x: pcData=%#x, iFmt=%d, cItems=%ul\n",
                         wndCursor, pcData, fmt, cItems);
            }
            else
            {
                /* Get the current window's Xdnd version. */
                newVer = reinterpret_cast<long *>(pcData)[0];
            }

            XFree(pcData);
        }
    }

#ifdef DEBUG
    char *pszNameCursor = wndX11GetNameA(wndCursor);
    AssertPtr(pszNameCursor);
    char *pszNameCur = wndX11GetNameA(m_wndCur);
    AssertPtr(pszNameCur);

    LogFlowThisFunc(("wndCursor=%x ('%s', Xdnd version %ld), wndCur=%x ('%s', Xdnd version %ld)\n",
                     wndCursor, pszNameCursor, newVer, m_wndCur, pszNameCur, m_curVer));

    RTStrFree(pszNameCursor);
    RTStrFree(pszNameCur);
#endif

    if (   wndCursor != m_wndCur
        && m_curVer  != -1)
    {
        LogFlowThisFunc(("XA_XdndLeave: window=%#x\n", m_wndCur));

        char *pszWndName = wndX11GetNameA(m_wndCur);
        AssertPtr(pszWndName);
        logInfo("Left old window %#x ('%s'), Xdnd version=%ld\n", m_wndCur, pszWndName, newVer);
        RTStrFree(pszWndName);

        /* We left the current XdndAware window. Announce this to the current indow. */
        XClientMessageEvent m;
        RT_ZERO(m);
        m.type                    = ClientMessage;
        m.display                 = m_pDisplay;
        m.window                  = m_wndCur;
        m.message_type            = xAtom(XA_XdndLeave);
        m.format                  = 32;
        m.data.l[XdndLeaveWindow] = m_wndProxy.hWnd;

        xRc = XSendEvent(m_pDisplay, m_wndCur, False, NoEventMask, reinterpret_cast<XEvent*>(&m));
        if (xRc == 0)
            logError("Error sending XA_XdndLeave event to old window=%#x: %s\n", m_wndCur, gX11->xErrorToString(xRc).c_str());

        /* Reset our current window. */
        m_wndCur = 0;
        m_curVer = -1;
    }

    /*
     * Do we have a new Xdnd-aware window which now is under the cursor?
     */
    if (   wndCursor != m_wndCur
        && newVer    != -1)
    {
        LogFlowThisFunc(("XA_XdndEnter: window=%#x\n", wndCursor));

        char *pszWndName = wndX11GetNameA(wndCursor);
        AssertPtr(pszWndName);
        logInfo("Entered new window %#x ('%s'), supports Xdnd version=%ld\n", wndCursor, pszWndName, newVer);
        RTStrFree(pszWndName);

        /*
         * We enter a new window. Announce the XdndEnter event to the new
         * window. The first three mime types are attached to the event (the
         * others could be requested by the XdndTypeList property from the
         * window itself).
         */
        XClientMessageEvent m;
        RT_ZERO(m);
        m.type         = ClientMessage;
        m.display      = m_pDisplay;
        m.window       = wndCursor;
        m.message_type = xAtom(XA_XdndEnter);
        m.format       = 32;
        m.data.l[XdndEnterWindow] = m_wndProxy.hWnd;
        m.data.l[XdndEnterFlags]  = RT_MAKE_U32_FROM_U8(
                                    /* Bit 0 is set if the source supports more than three data types. */
                                    m_lstFormats.size() > 3 ? RT_BIT(0) : 0,
                                    /* Reserved for future use. */
                                    0, 0,
                                    /* Protocol version to use. */
                                    RT_MIN(VBOX_XDND_VERSION, newVer));
        m.data.l[XdndEnterType1]  = m_lstFormats.value(0, None); /* First data type to use. */
        m.data.l[XdndEnterType2]  = m_lstFormats.value(1, None); /* Second data type to use. */
        m.data.l[XdndEnterType3]  = m_lstFormats.value(2, None); /* Third data type to use. */

        xRc = XSendEvent(m_pDisplay, wndCursor, False, NoEventMask, reinterpret_cast<XEvent*>(&m));
        if (xRc == 0)
            logError("Error sending XA_XdndEnter event to window=%#x: %s\n", wndCursor, gX11->xErrorToString(xRc).c_str());
    }

    if (newVer != -1)
    {
        Assert(wndCursor != None);

        LogFlowThisFunc(("XA_XdndPosition: xPos=%RU32, yPos=%RU32 to window=%#x\n", u32xPos, u32yPos, wndCursor));

        /*
         * Send a XdndPosition event with the proposed action to the guest.
         */
        Atom pa = toAtomAction(uDefaultAction);
        LogFlowThisFunc(("strAction=%s\n", xAtomToString(pa).c_str()));

        XClientMessageEvent m;
        RT_ZERO(m);
        m.type         = ClientMessage;
        m.display      = m_pDisplay;
        m.window       = wndCursor;
        m.message_type = xAtom(XA_XdndPosition);
        m.format       = 32;
        m.data.l[XdndPositionWindow]    = m_wndProxy.hWnd;               /* X window ID of source window. */
        m.data.l[XdndPositionXY]        = RT_MAKE_U32(u32yPos, u32xPos); /* Cursor coordinates relative to the root window. */
        m.data.l[XdndPositionTimeStamp] = CurrentTime;                   /* Timestamp for retrieving data. */
        m.data.l[XdndPositionAction]    = pa;                            /* Actions requested by the user. */

        xRc = XSendEvent(m_pDisplay, wndCursor, False, NoEventMask, reinterpret_cast<XEvent*>(&m));
        if (xRc == 0)
            logError("Error sending XA_XdndPosition event to current window=%#x: %s\n", wndCursor, gX11->xErrorToString(xRc).c_str());
    }

    if (newVer == -1)
    {
        /* No window to process, so send a ignore ack event to the host. */
        rc = VbglR3DnDHGSendAckOp(&m_dndCtx, DND_IGNORE_ACTION);
    }
    else
    {
        Assert(wndCursor != None);

        m_wndCur = wndCursor;
        m_curVer = newVer;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Host -> Guest: Event signalling that the host has dropped the data over the VM (guest) window.
 *
 * @returns IPRT status code.
 * @param   u32xPos                 Relative X position within the guest's display area.
 * @param   u32yPos                 Relative Y position within the guest's display area.
 * @param   uDefaultAction          Default action the host wants to perform on the guest
 *                                  as soon as the operation successfully finishes.
 */
int DragInstance::hgDrop(uint32_t u32xPos, uint32_t u32yPos, uint32_t uDefaultAction)
{


    /** @todo r=bird: Please, stop using 'u32' as a prefix unless you've got a _real_ _important_ reason for needing the bit count. */


    RT_NOREF3(u32xPos, u32yPos, uDefaultAction);
    LogFlowThisFunc(("wndCur=%#x, wndProxy=%#x, mode=%RU32, state=%RU32\n", m_wndCur, m_wndProxy.hWnd, m_enmMode, m_enmState));
    LogFlowThisFunc(("u32xPos=%RU32, u32yPos=%RU32, uAction=%RU32\n", u32xPos, u32yPos, uDefaultAction));

    if (   m_enmMode  != HG
        || m_enmState != Dragging)
    {
        return VERR_INVALID_STATE;
    }

    /* Set the state accordingly. */
    m_enmState = Dropped;

    /*
     * Ask the host to send the raw data, as we don't (yet) know which format
     * the guest exactly expects. As blocking in a SelectionRequest message turned
     * out to be very unreliable (e.g. with KDE apps) we request to start transferring
     * file/directory data (if any) here.
     */
    char szFormat[] = { "text/uri-list" };

    int rc = VbglR3DnDHGSendReqData(&m_dndCtx, szFormat);
    logInfo("Drop event from host resuled in: %Rrc\n", rc);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Host -> Guest: Event signalling that the host has finished sending drag'n drop
 *                data to the guest for further processing.
 *
 * @returns IPRT status code.
 * @param   pvData                  Pointer to (MIME) data from host.
 * @param   cbData                  Size (in bytes) of data from host.
 */
int DragInstance::hgDataReceived(const void *pvData, uint32_t cbData)
{
    LogFlowThisFunc(("mode=%RU32, state=%RU32\n", m_enmMode, m_enmState));
    LogFlowThisFunc(("pvData=%p, cbData=%RU32\n", pvData, cbData));

    if (   m_enmMode  != HG
        || m_enmState != Dropped)
    {
        return VERR_INVALID_STATE;
    }

    if (   pvData == NULL
        || cbData == 0)
    {
        return VERR_INVALID_PARAMETER;
    }

    int rc = VINF_SUCCESS;

    /*
     * At this point all data needed (including sent files/directories) should
     * be on the guest, so proceed working on communicating with the target window.
     */
    logInfo("Received %RU32 bytes MIME data from host\n", cbData);

    /* Destroy any old data. */
    if (m_pvSelReqData)
    {
        Assert(m_cbSelReqData);

        RTMemFree(m_pvSelReqData); /** @todo RTMemRealloc? */
        m_cbSelReqData = 0;
    }

    /** @todo Handle incremental transfers. */

    /* Make a copy of the data. This data later then will be used to fill into
     * the selection request. */
    if (cbData)
    {
        m_pvSelReqData = RTMemAlloc(cbData);
        if (!m_pvSelReqData)
            return VERR_NO_MEMORY;

        memcpy(m_pvSelReqData, pvData, cbData);
        m_cbSelReqData = cbData;
    }

    /*
     * Send a drop event to the current window (target).
     * This window in turn then will raise a SelectionRequest message to our proxy window,
     * which we will handle in our onX11SelectionRequest handler.
     *
     * The SelectionRequest will tell us in which format the target wants the data from the host.
     */
    XClientMessageEvent m;
    RT_ZERO(m);
    m.type         = ClientMessage;
    m.display      = m_pDisplay;
    m.window       = m_wndCur;
    m.message_type = xAtom(XA_XdndDrop);
    m.format       = 32;
    m.data.l[XdndDropWindow]    = m_wndProxy.hWnd;  /* Source window. */
    m.data.l[XdndDropFlags]     = 0;                /* Reserved for future use. */
    m.data.l[XdndDropTimeStamp] = CurrentTime;      /* Our DnD data does not rely on any timing, so just use the current time. */

    int xRc = XSendEvent(m_pDisplay, m_wndCur, False /* Propagate */, NoEventMask, reinterpret_cast<XEvent*>(&m));
    if (xRc == 0)
        logError("Error sending XA_XdndDrop event to window=%#x: %s\n", m_wndCur, gX11->xErrorToString(xRc).c_str());
    XFlush(m_pDisplay);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

#ifdef VBOX_WITH_DRAG_AND_DROP_GH
/**
 * Guest -> Host: Event signalling that the host is asking whether there is a pending
 *                drag event on the guest (to the host).
 *
 * @returns IPRT status code.
 */
int DragInstance::ghIsDnDPending(void)
{
    LogFlowThisFunc(("mode=%RU32, state=%RU32\n", m_enmMode, m_enmState));

    int rc;

    RTCString strFormats = "\r\n"; /** @todo If empty, IOCTL fails with VERR_ACCESS_DENIED. */
    uint32_t uDefAction  = DND_IGNORE_ACTION;
    uint32_t uAllActions = DND_IGNORE_ACTION;

    /* Currently in wrong mode? Bail out. */
    if (m_enmMode == HG)
        rc = VERR_INVALID_STATE;
    /* Message already processed successfully? */
    else if (   m_enmMode  == GH
             && (   m_enmState == Dragging
                 || m_enmState == Dropped)
            )
    {
        /* No need to query for the source window again. */
        rc = VINF_SUCCESS;
    }
    else
    {
        rc = VINF_SUCCESS;

        /* Determine the current window which currently has the XdndSelection set. */
        Window wndSelection = XGetSelectionOwner(m_pDisplay, xAtom(XA_XdndSelection));
        LogFlowThisFunc(("wndSelection=%#x, wndProxy=%#x, wndCur=%#x\n", wndSelection, m_wndProxy.hWnd, m_wndCur));

        /* Is this another window which has a Xdnd selection and not our proxy window? */
        if (   wndSelection
            && wndSelection != m_wndCur)
        {
            char *pszWndName = wndX11GetNameA(wndSelection);
            AssertPtr(pszWndName);
            logInfo("New guest source window %#x ('%s')\n", wndSelection, pszWndName);

            /* Start over. */
            reset();

            /* Map the window on the current cursor position, which should provoke
             * an XdndEnter event. */
            rc = proxyWinShow();
            if (RT_SUCCESS(rc))
            {
                rc = mouseCursorFakeMove();
                if (RT_SUCCESS(rc))
                {
                    bool fWaitFailed = false; /* Waiting for status changed failed? */

                    /* Wait until we're in "Dragging" state. */
                    rc = waitForStatusChange(Dragging, 100 /* 100ms timeout */);

                    /*
                     * Note: Don't wait too long here, as this mostly will make
                     *       the drag and drop experience on the host being laggy
                     *       and unresponsive.
                     *
                     *       Instead, let the host query multiple times with 100ms
                     *       timeout each (see above) and only report an error if
                     *       the overall querying time has been exceeded.<
                     */
                    if (RT_SUCCESS(rc))
                    {
                        m_enmMode = GH;
                    }
                    else if (rc == VERR_TIMEOUT)
                    {
                        /** @todo Make m_cFailedPendingAttempts configurable. For slower window managers? */
                        if (m_cFailedPendingAttempts++ > 50) /* Tolerate up to 5s total (100ms for each slot). */
                            fWaitFailed = true;
                        else
                            rc = VINF_SUCCESS;
                    }
                    else if (RT_FAILURE(rc))
                        fWaitFailed = true;

                    if (fWaitFailed)
                    {
                        logError("Error mapping proxy window to guest source window %#x ('%s'), rc=%Rrc\n",
                                 wndSelection, pszWndName, rc);

                        /* Reset the counter in any case. */
                        m_cFailedPendingAttempts = 0;
                    }
                }
            }

            RTStrFree(pszWndName);
        }
    }

    /*
     * Acknowledge to the host in any case, regardless
     * if something failed here or not. Be responsive.
     */

    int rc2 = RTCritSectEnter(&m_dataCS);
    if (RT_SUCCESS(rc2))
    {
        RTCString strFormatsCur = gX11->xAtomListToString(m_lstFormats);
        if (!strFormatsCur.isEmpty())
        {
            strFormats   = strFormatsCur;
            uDefAction   = DND_COPY_ACTION; /** @todo Handle default action! */
            uAllActions  = DND_COPY_ACTION; /** @todo Ditto. */
            uAllActions |= toHGCMActions(m_lstActions);
        }

        RTCritSectLeave(&m_dataCS);
    }

    rc2 = VbglR3DnDGHSendAckPending(&m_dndCtx, uDefAction, uAllActions,
                                    strFormats.c_str(), strFormats.length() + 1 /* Include termination */);
    LogFlowThisFunc(("uClientID=%RU32, uDefAction=0x%x, allActions=0x%x, strFormats=%s, rc=%Rrc\n",
                     m_dndCtx.uClientID, uDefAction, uAllActions, strFormats.c_str(), rc2));
    if (RT_FAILURE(rc2))
    {
        logError("Error reporting pending drag and drop operation status to host: %Rrc\n", rc2);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Guest -> Host: Event signalling that the host has dropped the item(s) on the
 *                host side.
 *
 * @returns IPRT status code.
 * @param   strFormat               Requested format to send to the host.
 * @param   uAction                 Requested action to perform on the guest.
 */
int DragInstance::ghDropped(const RTCString &strFormat, uint32_t uAction)
{
    LogFlowThisFunc(("mode=%RU32, state=%RU32, strFormat=%s, uAction=%RU32\n",
                     m_enmMode, m_enmState, strFormat.c_str(), uAction));

    /* Currently in wrong mode? Bail out. */
    if (   m_enmMode == Unknown
        || m_enmMode == HG)
    {
        return VERR_INVALID_STATE;
    }

    if (   m_enmMode  == GH
        && m_enmState != Dragging)
    {
        return VERR_INVALID_STATE;
    }

    int rc = VINF_SUCCESS;

    m_enmState = Dropped;

#ifdef DEBUG
    XWindowAttributes xwa;
    XGetWindowAttributes(m_pDisplay, m_wndCur, &xwa);
    LogFlowThisFunc(("wndProxy=%#x, wndCur=%#x, x=%d, y=%d, width=%d, height=%d\n",
                     m_wndProxy.hWnd, m_wndCur, xwa.x, xwa.y, xwa.width, xwa.height));

    Window wndSelection = XGetSelectionOwner(m_pDisplay, xAtom(XA_XdndSelection));
    LogFlowThisFunc(("wndSelection=%#x\n", wndSelection));
#endif

    /* We send a fake mouse move event to the current window, cause
     * this should have the grab. */
    mouseCursorFakeMove();

    /**
     * The fake button release event above should lead to a XdndDrop event from the
     * source window. Because of showing our proxy window, other Xdnd events can
     * occur before, e.g. a XdndPosition event. We are not interested
     * in those, so just try to get the right one.
     */

    XClientMessageEvent evDnDDrop;
    bool fDrop = waitForX11ClientMsg(evDnDDrop, xAtom(XA_XdndDrop), 5 * 1000 /* 5s timeout */);
    if (fDrop)
    {
        LogFlowThisFunc(("XA_XdndDrop\n"));

        /* Request to convert the selection in the specific format and
         * place it to our proxy window as property. */
        Assert(evDnDDrop.message_type == xAtom(XA_XdndDrop));

        Window wndSource = evDnDDrop.data.l[XdndDropWindow]; /* Source window which has sent the message. */
        Assert(wndSource == m_wndCur);

        Atom aFormat     = gX11->stringToxAtom(strFormat.c_str());

        Time tsDrop;
        if (m_curVer >= 1)
            tsDrop = evDnDDrop.data.l[XdndDropTimeStamp];
        else
            tsDrop = CurrentTime;

        XConvertSelection(m_pDisplay, xAtom(XA_XdndSelection), aFormat, xAtom(XA_XdndSelection),
                          m_wndProxy.hWnd, tsDrop);

        /* Wait for the selection notify event. */
        XEvent evSelNotify;
        RT_ZERO(evSelNotify);
        if (waitForX11Msg(evSelNotify, SelectionNotify, 5 * 1000 /* 5s timeout */))
        {
            bool fCancel = false;

            /* Make some paranoid checks. */
            if (   evSelNotify.xselection.type      == SelectionNotify
                && evSelNotify.xselection.display   == m_pDisplay
                && evSelNotify.xselection.selection == xAtom(XA_XdndSelection)
                && evSelNotify.xselection.requestor == m_wndProxy.hWnd
                && evSelNotify.xselection.target    == aFormat)
            {
                LogFlowThisFunc(("Selection notfiy (from wnd=%#x)\n", m_wndCur));

                Atom aPropType;
                int iPropFormat;
                unsigned long cItems, cbRemaining;
                unsigned char *pcData = NULL;
                int xRc = XGetWindowProperty(m_pDisplay, m_wndProxy.hWnd,
                                             xAtom(XA_XdndSelection)  /* Property */,
                                             0                        /* Offset */,
                                             VBOX_MAX_XPROPERTIES     /* Length of 32-bit multiples */,
                                             True                     /* Delete property? */,
                                             AnyPropertyType,         /* Property type */
                                             &aPropType, &iPropFormat, &cItems, &cbRemaining, &pcData);
                if (xRc != Success)
                    logError("Error getting XA_XdndSelection property of proxy window=%#x: %s\n",
                             m_wndProxy.hWnd, gX11->xErrorToString(xRc).c_str());

                LogFlowThisFunc(("strType=%s, iPropFormat=%d, cItems=%RU32, cbRemaining=%RU32\n",
                                 gX11->xAtomToString(aPropType).c_str(), iPropFormat, cItems, cbRemaining));

                if (   aPropType   != None
                    && pcData      != NULL
                    && iPropFormat >= 8
                    && cItems      >  0
                    && cbRemaining == 0)
                {
                    size_t cbData = cItems * (iPropFormat / 8);
                    LogFlowThisFunc(("cbData=%zu\n", cbData));

                    /* For whatever reason some of the string MIME types are not
                     * zero terminated. Check that and correct it when necessary,
                     * because the guest side wants this in any case. */
                    if (   m_lstAllowedFormats.contains(strFormat)
                        && pcData[cbData - 1] != '\0')
                    {
                        unsigned char *pvDataTmp = static_cast<unsigned char*>(RTMemAlloc(cbData + 1));
                        if (pvDataTmp)
                        {
                            memcpy(pvDataTmp, pcData, cbData);
                            pvDataTmp[cbData++] = '\0';

                            rc = VbglR3DnDGHSendData(&m_dndCtx, strFormat.c_str(), pvDataTmp, cbData);
                            RTMemFree(pvDataTmp);
                        }
                        else
                            rc = VERR_NO_MEMORY;
                    }
                    else
                    {
                        /* Send the raw data to the host. */
                        rc = VbglR3DnDGHSendData(&m_dndCtx, strFormat.c_str(), pcData, cbData);
                        LogFlowThisFunc(("Sent strFormat=%s, rc=%Rrc\n", strFormat.c_str(), rc));
                    }

                    if (RT_SUCCESS(rc))
                    {
                        rc = m_wndProxy.sendFinished(wndSource, uAction);
                    }
                    else
                        fCancel = true;
                }
                else
                {
                    if (aPropType == xAtom(XA_INCR))
                    {
                        /** @todo Support incremental transfers. */
                        AssertMsgFailed(("Incremental transfers are not supported yet\n"));

                        logError("Incremental transfers are not supported yet\n");
                        rc = VERR_NOT_IMPLEMENTED;
                    }
                    else
                    {
                        logError("Not supported data type: %s\n", gX11->xAtomToString(aPropType).c_str());
                        rc = VERR_NOT_SUPPORTED;
                    }

                    fCancel = true;
                }

                if (fCancel)
                {
                    logInfo("Cancelling dropping to host\n");

                    /* Cancel the operation -- inform the source window by
                     * sending a XdndFinished message so that the source can toss the required data. */
                    rc = m_wndProxy.sendFinished(wndSource, DND_IGNORE_ACTION);
                }

                /* Cleanup. */
                if (pcData)
                    XFree(pcData);
            }
            else
                rc = VERR_INVALID_PARAMETER;
        }
        else
            rc = VERR_TIMEOUT;
    }
    else
        rc = VERR_TIMEOUT;

    /* Inform the host on error. */
    if (RT_FAILURE(rc))
    {
        int rc2 = VbglR3DnDGHSendError(&m_dndCtx, rc);
        LogFlowThisFunc(("Sending error to host resulted in %Rrc\n", rc2)); NOREF(rc2);
        /* This is not fatal for us, just ignore. */
    }

    /* At this point, we have either successfully transfered any data or not.
     * So reset our internal state because we are done here for the current (ongoing)
     * drag and drop operation. */
    reset();

    LogFlowFuncLeaveRC(rc);
    return rc;
}
#endif /* VBOX_WITH_DRAG_AND_DROP_GH */

/*
 * Helpers
 */

/**
 * Fakes moving the mouse cursor to provoke various drag and drop
 * events such as entering a target window or moving within a
 * source window.
 *
 * Not the most elegant and probably correct function, but does
 * the work for now.
 *
 * @returns IPRT status code.
 */
int DragInstance::mouseCursorFakeMove(void) const
{
    int iScreenID = XDefaultScreen(m_pDisplay);
    /** @todo What about multiple screens? Test this! */

    const int iScrX = XDisplayWidth(m_pDisplay, iScreenID);
    const int iScrY = XDisplayHeight(m_pDisplay, iScreenID);

    int fx, fy, rx, ry;
    Window wndTemp, wndChild;
    int wx, wy; unsigned int mask;
    XQueryPointer(m_pDisplay, m_wndRoot, &wndTemp, &wndChild, &rx, &ry, &wx, &wy, &mask);

    /*
     * Apply some simple clipping and change the position slightly.
     */

    /* FakeX */
    if      (rx == 0)     fx = 1;
    else if (rx == iScrX) fx = iScrX - 1;
    else                  fx = rx + 1;

    /* FakeY */
    if      (ry == 0)     fy = 1;
    else if (ry == iScrY) fy = iScrY - 1;
    else                  fy = ry + 1;

    /*
     * Move the cursor to trigger the wanted events.
     */
    LogFlowThisFunc(("cursorRootX=%d, cursorRootY=%d\n", fx, fy));
    int rc = mouseCursorMove(fx, fy);
    if (RT_SUCCESS(rc))
    {
        /* Move the cursor back to its original position. */
        rc = mouseCursorMove(rx, ry);
    }

    return rc;
}

/**
 * Moves the mouse pointer to a specific position.
 *
 * @returns IPRT status code.
 * @param   iPosX                   Absolute X coordinate.
 * @param   iPosY                   Absolute Y coordinate.
 */
int DragInstance::mouseCursorMove(int iPosX, int iPosY) const
{
    int iScreenID = XDefaultScreen(m_pDisplay);
    /** @todo What about multiple screens? Test this! */

    const int iScrX = XDisplayWidth(m_pDisplay, iScreenID);
    const int iScrY = XDisplayHeight(m_pDisplay, iScreenID);

    iPosX = RT_CLAMP(iPosX, 0, iScrX);
    iPosY = RT_CLAMP(iPosY, 0, iScrY);

    LogFlowThisFunc(("iPosX=%d, iPosY=%d\n", iPosX, iPosY));

    /* Move the guest pointer to the DnD position, so we can find the window
     * below that position. */
    XWarpPointer(m_pDisplay, None, m_wndRoot, 0, 0, 0, 0, iPosX, iPosY);
    return VINF_SUCCESS;
}

/**
 * Sends a mouse button event to a specific window.
 *
 * @param   wndDest                 Window to send the mouse button event to.
 * @param   rx                      X coordinate relative to the root window's origin.
 * @param   ry                      Y coordinate relative to the root window's origin.
 * @param   iButton                 Mouse button to press/release.
 * @param   fPress                  Whether to press or release the mouse button.
 */
void DragInstance::mouseButtonSet(Window wndDest, int rx, int ry, int iButton, bool fPress)
{
    LogFlowThisFunc(("wndDest=%#x, rx=%d, ry=%d, iBtn=%d, fPress=%RTbool\n",
                     wndDest, rx, ry, iButton, fPress));

#ifdef VBOX_DND_WITH_XTEST
    /** @todo Make this check run only once. */
    int ev, er, ma, mi;
    if (XTestQueryExtension(m_pDisplay, &ev, &er, &ma, &mi))
    {
        LogFlowThisFunc(("XText extension available\n"));

        int xRc = XTestFakeButtonEvent(m_pDisplay, 1, fPress ? True : False, CurrentTime);
        if (Rc == 0)
            logError("Error sending XTestFakeButtonEvent event: %s\n", gX11->xErrorToString(xRc).c_str());
        XFlush(m_pDisplay);
    }
    else
    {
#endif
        LogFlowThisFunc(("Note: XText extension not available or disabled\n"));

        unsigned int mask = 0;

        if (   rx == -1
            && ry == -1)
        {
            Window wndRoot, wndChild;
            int wx, wy;
            XQueryPointer(m_pDisplay, m_wndRoot, &wndRoot, &wndChild, &rx, &ry, &wx, &wy, &mask);
            LogFlowThisFunc(("Mouse pointer is at root x=%d, y=%d\n", rx, ry));
        }

        XButtonEvent eBtn;
        RT_ZERO(eBtn);

        eBtn.display      = m_pDisplay;
        eBtn.root         = m_wndRoot;
        eBtn.window       = wndDest;
        eBtn.subwindow    = None;
        eBtn.same_screen  = True;
        eBtn.time         = CurrentTime;
        eBtn.button       = iButton;
        eBtn.state        = mask | (iButton == 1 ? Button1MotionMask :
                                    iButton == 2 ? Button2MotionMask :
                                    iButton == 3 ? Button3MotionMask :
                                    iButton == 4 ? Button4MotionMask :
                                    iButton == 5 ? Button5MotionMask : 0);
        eBtn.type         = fPress ? ButtonPress : ButtonRelease;
        eBtn.send_event   = False;
        eBtn.x_root       = rx;
        eBtn.y_root       = ry;

        XTranslateCoordinates(m_pDisplay, eBtn.root, eBtn.window, eBtn.x_root, eBtn.y_root, &eBtn.x, &eBtn.y, &eBtn.subwindow);
        LogFlowThisFunc(("state=0x%x, x=%d, y=%d\n", eBtn.state, eBtn.x, eBtn.y));

        int xRc = XSendEvent(m_pDisplay, wndDest, True /* fPropagate */,
                             ButtonPressMask,
                             reinterpret_cast<XEvent*>(&eBtn));
        if (xRc == 0)
            logError("Error sending XButtonEvent event to window=%#x: %s\n", wndDest, gX11->xErrorToString(xRc).c_str());

        XFlush(m_pDisplay);

#ifdef VBOX_DND_WITH_XTEST
    }
#endif
}

/**
 * Shows the (invisible) proxy window. The proxy window is needed for intercepting
 * drags from the host to the guest or from the guest to the host. It acts as a proxy
 * between the host and the actual (UI) element on the guest OS.
 *
 * To not make it miss any actions this window gets spawned across the entire guest
 * screen (think of an umbrella) to (hopefully) capture everything. A proxy window
 * which follows the cursor would be far too slow here.
 *
 * @returns IPRT status code.
 * @param   piRootX                 X coordinate relative to the root window's origin. Optional.
 * @param   piRootY                 Y coordinate relative to the root window's origin. Optional.
 */
int DragInstance::proxyWinShow(int *piRootX /* = NULL */, int *piRootY /* = NULL */) const
{
    /* piRootX is optional. */
    /* piRootY is optional. */

    LogFlowThisFuncEnter();

    int rc = VINF_SUCCESS;

#if 0
# ifdef VBOX_DND_WITH_XTEST
    XTestGrabControl(m_pDisplay, False);
# endif
#endif

    /* Get the mouse pointer position and determine if we're on the same screen as the root window
     * and return the current child window beneath our mouse pointer, if any. */
    int iRootX, iRootY;
    int iChildX, iChildY;
    unsigned int iMask;
    Window wndRoot, wndChild;
    Bool fInRootWnd = XQueryPointer(m_pDisplay, m_wndRoot, &wndRoot, &wndChild,
                                    &iRootX, &iRootY, &iChildX, &iChildY, &iMask);

    LogFlowThisFunc(("fInRootWnd=%RTbool, wndRoot=0x%x, wndChild=0x%x, iRootX=%d, iRootY=%d\n",
                     RT_BOOL(fInRootWnd), wndRoot, wndChild, iRootX, iRootY)); NOREF(fInRootWnd);

    if (piRootX)
        *piRootX = iRootX;
    if (piRootY)
        *piRootY = iRootY;

    XSynchronize(m_pDisplay, True /* Enable sync */);

    /* Bring our proxy window into foreground. */
    XMapWindow(m_pDisplay, m_wndProxy.hWnd);
    XRaiseWindow(m_pDisplay, m_wndProxy.hWnd);

    /* Spawn our proxy window over the entire screen, making it an easy drop target for the host's cursor. */
    LogFlowThisFunc(("Proxy window x=%d, y=%d, width=%d, height=%d\n",
                     m_wndProxy.iX, m_wndProxy.iY, m_wndProxy.iWidth, m_wndProxy.iHeight));
    XMoveResizeWindow(m_pDisplay, m_wndProxy.hWnd, m_wndProxy.iX, m_wndProxy.iY, m_wndProxy.iWidth, m_wndProxy.iHeight);

    XFlush(m_pDisplay);

    XSynchronize(m_pDisplay, False /* Disable sync */);

#if 0
# ifdef VBOX_DND_WITH_XTEST
    XTestGrabControl(m_pDisplay, True);
# endif
#endif

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Hides the (invisible) proxy window.
 */
int DragInstance::proxyWinHide(void)
{
    LogFlowFuncEnter();

    XUnmapWindow(m_pDisplay, m_wndProxy.hWnd);
    XFlush(m_pDisplay);

    m_eventQueueList.clear();

    return VINF_SUCCESS; /** @todo Add error checking. */
}

/**
 * Allocates the name (title) of an X window.
 * The returned pointer must be freed using RTStrFree().
 *
 * @returns Pointer to the allocated window name.
 * @param   wndThis                 Window to retrieve name for.
 *
 * @remark If the window title is not available, the text
 *         "<No name>" will be returned.
 */
char *DragInstance::wndX11GetNameA(Window wndThis) const
{
    char *pszName = NULL;

    XTextProperty propName;
    if (XGetWMName(m_pDisplay, wndThis, &propName))
    {
        if (propName.value)
            pszName = RTStrDup((char *)propName.value); /** @todo UTF8? */
        XFree(propName.value);
    }

    if (!pszName) /* No window name found? */
        pszName = RTStrDup("<No name>");

    return pszName;
}

/**
 * Clear a window's supported/accepted actions list.
 *
 * @param   wndThis                 Window to clear the list for.
 */
void DragInstance::wndXDnDClearActionList(Window wndThis) const
{
    XDeleteProperty(m_pDisplay, wndThis, xAtom(XA_XdndActionList));
}

/**
 * Clear a window's supported/accepted formats list.
 *
 * @param   wndThis                 Window to clear the list for.
 */
void DragInstance::wndXDnDClearFormatList(Window wndThis) const
{
    XDeleteProperty(m_pDisplay, wndThis, xAtom(XA_XdndTypeList));
}

/**
 * Retrieves a window's supported/accepted XDnD actions.
 *
 * @returns IPRT status code.
 * @param   wndThis                 Window to retrieve the XDnD actions for.
 * @param   lstActions              Reference to VBoxDnDAtomList to store the action into.
 */
int DragInstance::wndXDnDGetActionList(Window wndThis, VBoxDnDAtomList &lstActions) const
{
    Atom iActType = None;
    int iActFmt;
    unsigned long cItems, cbData;
    unsigned char *pcbData = NULL;

    /* Fetch the possible list of actions, if this property is set. */
    int xRc = XGetWindowProperty(m_pDisplay, wndThis,
                                 xAtom(XA_XdndActionList),
                                 0, VBOX_MAX_XPROPERTIES,
                                 False, XA_ATOM, &iActType, &iActFmt, &cItems, &cbData, &pcbData);
    if (xRc != Success)
    {
        LogFlowThisFunc(("Error getting XA_XdndActionList atoms from window=%#x: %s\n",
                         wndThis, gX11->xErrorToString(xRc).c_str()));
        return VERR_NOT_FOUND;
    }

    LogFlowThisFunc(("wndThis=%#x, cItems=%RU32, pcbData=%p\n", wndThis, cItems, pcbData));

    if (cItems > 0)
    {
        AssertPtr(pcbData);
        Atom *paData = reinterpret_cast<Atom *>(pcbData);

        for (unsigned i = 0; i < RT_MIN(VBOX_MAX_XPROPERTIES, cItems); i++)
        {
            LogFlowThisFunc(("\t%s\n", gX11->xAtomToString(paData[i]).c_str()));
            lstActions.append(paData[i]);
        }

        XFree(pcbData);
    }

    return VINF_SUCCESS;
}

/**
 * Retrieves a window's supported/accepted XDnD formats.
 *
 * @returns IPRT status code.
 * @param   wndThis                 Window to retrieve the XDnD formats for.
 * @param   lstTypes                Reference to VBoxDnDAtomList to store the formats into.
 */
int DragInstance::wndXDnDGetFormatList(Window wndThis, VBoxDnDAtomList &lstTypes) const
{
    Atom iActType = None;
    int iActFmt;
    unsigned long cItems, cbData;
    unsigned char *pcbData = NULL;

    int xRc = XGetWindowProperty(m_pDisplay, wndThis,
                             xAtom(XA_XdndTypeList),
                             0, VBOX_MAX_XPROPERTIES,
                             False, XA_ATOM, &iActType, &iActFmt, &cItems, &cbData, &pcbData);
    if (xRc != Success)
    {
        LogFlowThisFunc(("Error getting XA_XdndTypeList atoms from window=%#x: %s\n",
                         wndThis, gX11->xErrorToString(xRc).c_str()));
        return VERR_NOT_FOUND;
    }

    LogFlowThisFunc(("wndThis=%#x, cItems=%RU32, pcbData=%p\n", wndThis, cItems, pcbData));

    if (cItems > 0)
    {
        AssertPtr(pcbData);
        Atom *paData = reinterpret_cast<Atom *>(pcbData);

        for (unsigned i = 0; i < RT_MIN(VBOX_MAX_XPROPERTIES, cItems); i++)
        {
            LogFlowThisFunc(("\t%s\n", gX11->xAtomToString(paData[i]).c_str()));
            lstTypes.append(paData[i]);
        }

        XFree(pcbData);
    }

    return VINF_SUCCESS;
}

/**
 * Sets (replaces) a window's XDnD accepted/allowed actions.
 *
 * @returns IPRT status code.
 * @param   wndThis                 Window to set the format list for.
 * @param   lstActions              Reference to list of XDnD actions to set.
 *
 * @remark
 */
int DragInstance::wndXDnDSetActionList(Window wndThis, const VBoxDnDAtomList &lstActions) const
{
    if (lstActions.isEmpty())
        return VINF_SUCCESS;

    XChangeProperty(m_pDisplay, wndThis,
                    xAtom(XA_XdndActionList),
                    XA_ATOM, 32, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(lstActions.raw()),
                    lstActions.size());

    return VINF_SUCCESS;
}

/**
 * Sets (replaces) a window's XDnD accepted format list.
 *
 * @returns IPRT status code.
 * @param   wndThis                 Window to set the format list for.
 * @param   atmProp                 Property to set.
 * @param   lstFormats              Reference to list of XDnD formats to set.
 */
int DragInstance::wndXDnDSetFormatList(Window wndThis, Atom atmProp, const VBoxDnDAtomList &lstFormats) const
{
    if (lstFormats.isEmpty())
        return VERR_INVALID_PARAMETER;

    /* We support TARGETS and the data types. */
    VBoxDnDAtomList lstFormatsExt(lstFormats.size() + 1);
    lstFormatsExt.append(xAtom(XA_TARGETS));
    lstFormatsExt.append(lstFormats);

    /* Add the property with the property data to the window. */
    XChangeProperty(m_pDisplay, wndThis, atmProp,
                    XA_ATOM, 32, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(lstFormatsExt.raw()),
                    lstFormatsExt.size());

    return VINF_SUCCESS;
}

/**
 * Converts a RTCString list to VBoxDnDAtomList list.
 *
 * @returns IPRT status code.
 * @param   lstFormats              Reference to RTCString list to convert.
 * @param   lstAtoms                Reference to VBoxDnDAtomList list to store results in.
 */
int DragInstance::toAtomList(const RTCList<RTCString> &lstFormats, VBoxDnDAtomList &lstAtoms) const
{
    for (size_t i = 0; i < lstFormats.size(); ++i)
        lstAtoms.append(XInternAtom(m_pDisplay, lstFormats.at(i).c_str(), False));

    return VINF_SUCCESS;
}

/**
 * Converts a raw-data string list to VBoxDnDAtomList list.
 *
 * @returns IPRT status code.
 * @param   pvData                  Pointer to string data to convert.
 * @param   cbData                  Size (in bytes) to convert.
 * @param   lstAtoms                Reference to VBoxDnDAtomList list to store results in.
 */
int DragInstance::toAtomList(const void *pvData, uint32_t cbData, VBoxDnDAtomList &lstAtoms) const
{
    RT_NOREF1(lstAtoms);
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData, VERR_INVALID_PARAMETER);

    const char *pszStr = (char *)pvData;
    uint32_t cbStr = cbData;

    int rc = VINF_SUCCESS;

    VBoxDnDAtomList lstAtom;
    while (cbStr)
    {
        size_t cbSize = RTStrNLen(pszStr, cbStr);

        /* Create a copy with max N chars, so that we are on the save side,
         * even if the data isn't zero terminated. */
        char *pszTmp = RTStrDupN(pszStr, cbSize);
        if (!pszTmp)
        {
            rc = VERR_NO_MEMORY;
            break;
        }

        lstAtom.append(XInternAtom(m_pDisplay, pszTmp, False));
        RTStrFree(pszTmp);

        pszStr  += cbSize + 1;
        cbStr   -= cbSize + 1;
    }

    return rc;
}

/**
 * Converts a HGCM-based drag'n drop action to a Atom-based drag'n drop action.
 *
 * @returns Converted Atom-based drag'n drop action.
 * @param   uAction                 HGCM drag'n drop actions to convert.
 */
/* static */
Atom DragInstance::toAtomAction(uint32_t uAction)
{
    /* Ignore is None. */
    return (isDnDCopyAction(uAction) ? xAtom(XA_XdndActionCopy) :
            isDnDMoveAction(uAction) ? xAtom(XA_XdndActionMove) :
            isDnDLinkAction(uAction) ? xAtom(XA_XdndActionLink) :
            None);
}

/**
 * Converts HGCM-based drag'n drop actions to a VBoxDnDAtomList list.
 *
 * @returns IPRT status code.
 * @param   uActions                HGCM drag'n drop actions to convert.
 * @param   lstAtoms                Reference to VBoxDnDAtomList to store actions in.
 */
/* static */
int DragInstance::toAtomActions(uint32_t uActions, VBoxDnDAtomList &lstAtoms)
{
    if (hasDnDCopyAction(uActions))
        lstAtoms.append(xAtom(XA_XdndActionCopy));
    if (hasDnDMoveAction(uActions))
        lstAtoms.append(xAtom(XA_XdndActionMove));
    if (hasDnDLinkAction(uActions))
        lstAtoms.append(xAtom(XA_XdndActionLink));

    return VINF_SUCCESS;
}

/**
 * Converts an Atom-based drag'n drop action to a HGCM drag'n drop action.
 *
 * @returns HGCM drag'n drop action.
 * @param   atom                    Atom-based drag'n drop action to convert.
 */
/* static */
uint32_t DragInstance::toHGCMAction(Atom atom)
{
    uint32_t uAction = DND_IGNORE_ACTION;

    if (atom == xAtom(XA_XdndActionCopy))
        uAction = DND_COPY_ACTION;
    else if (atom == xAtom(XA_XdndActionMove))
        uAction = DND_MOVE_ACTION;
    else if (atom == xAtom(XA_XdndActionLink))
        uAction = DND_LINK_ACTION;

    return uAction;
}

/**
 * Converts an VBoxDnDAtomList list to an HGCM action list.
 *
 * @returns ORed HGCM action list.
 * @param   lstActions              List of Atom-based actions to convert.
 */
/* static */
uint32_t DragInstance::toHGCMActions(const VBoxDnDAtomList &lstActions)
{
    uint32_t uActions = DND_IGNORE_ACTION;

    for (size_t i = 0; i < lstActions.size(); i++)
        uActions |= toHGCMAction(lstActions.at(i));

    return uActions;
}

/*******************************************************************************
 * VBoxDnDProxyWnd implementation.
 ******************************************************************************/

VBoxDnDProxyWnd::VBoxDnDProxyWnd(void)
    : pDisp(NULL)
    , hWnd(0)
    , iX(0)
    , iY(0)
    , iWidth(0)
    , iHeight(0)
{

}

VBoxDnDProxyWnd::~VBoxDnDProxyWnd(void)
{
    destroy();
}

int VBoxDnDProxyWnd::init(Display *pDisplay)
{
    /** @todo What about multiple screens? Test this! */
    int iScreenID = XDefaultScreen(pDisplay);

    iWidth   = XDisplayWidth(pDisplay, iScreenID);
    iHeight  = XDisplayHeight(pDisplay, iScreenID);
    pDisp    = pDisplay;

    return VINF_SUCCESS;
}

void VBoxDnDProxyWnd::destroy(void)
{

}

int VBoxDnDProxyWnd::sendFinished(Window hWndSource, uint32_t uAction)
{
    /* Was the drop accepted by the host? That is, anything than ignoring. */
    bool fDropAccepted = uAction > DND_IGNORE_ACTION;

    LogFlowFunc(("uAction=%RU32\n", uAction));

    /* Confirm the result of the transfer to the target window. */
    XClientMessageEvent m;
    RT_ZERO(m);
    m.type         = ClientMessage;
    m.display      = pDisp;
    m.window       = hWnd;
    m.message_type = xAtom(XA_XdndFinished);
    m.format       = 32;
    m.data.l[XdndFinishedWindow] = hWnd;                                                       /* Target window. */
    m.data.l[XdndFinishedFlags]  = fDropAccepted ? RT_BIT(0) : 0;                              /* Was the drop accepted? */
    m.data.l[XdndFinishedAction] = fDropAccepted ? DragInstance::toAtomAction(uAction) : None; /* Action used on accept. */

    int xRc = XSendEvent(pDisp, hWndSource, True, NoEventMask, reinterpret_cast<XEvent*>(&m));
    if (xRc == 0)
    {
        LogRel(("DnD: Error sending XA_XdndFinished event to source window=%#x: %s\n",
                hWndSource, gX11->xErrorToString(xRc).c_str()));

        return VERR_GENERAL_FAILURE; /** @todo Fudge. */
    }

    return VINF_SUCCESS;
}

/*******************************************************************************
 * DragAndDropService implementation.
 ******************************************************************************/

/**
 * Initializes the drag and drop service.
 *
 * @returns IPRT status code.
 */
int DragAndDropService::init(void)
{
    LogFlowFuncEnter();

    /* Initialise the guest library. */
    int rc = VbglR3InitUser();
    if (RT_FAILURE(rc))
    {
        VBClFatalError(("DnD: Failed to connect to the VirtualBox kernel service, rc=%Rrc\n", rc));
        return rc;
    }

    /* Connect to the x11 server. */
    m_pDisplay = XOpenDisplay(NULL);
    if (!m_pDisplay)
    {
        VBClFatalError(("DnD: Unable to connect to X server -- running in a terminal session?\n"));
        return VERR_NOT_FOUND;
    }

    xHelpers *pHelpers = xHelpers::getInstance(m_pDisplay);
    if (!pHelpers)
        return VERR_NO_MEMORY;

    do
    {
        rc = RTSemEventCreate(&m_hEventSem);
        if (RT_FAILURE(rc))
            break;

        rc = RTCritSectInit(&m_eventQueueCS);
        if (RT_FAILURE(rc))
            break;

        /* Event thread for events coming from the HGCM device. */
        rc = RTThreadCreate(&m_hHGCMThread, hgcmEventThread, this,
                            0, RTTHREADTYPE_MSG_PUMP, RTTHREADFLAGS_WAITABLE, "dndHGCM");
        if (RT_FAILURE(rc))
            break;

        rc = RTThreadUserWait(m_hHGCMThread, 10 * 1000 /* 10s timeout */);
        if (RT_FAILURE(rc))
            break;

        if (ASMAtomicReadBool(&m_fSrvStopping))
            break;

        /* Event thread for events coming from the x11 system. */
        rc = RTThreadCreate(&m_hX11Thread, x11EventThread, this,
                            0, RTTHREADTYPE_MSG_PUMP, RTTHREADFLAGS_WAITABLE, "dndX11");
        if (RT_FAILURE(rc))
            break;

        rc = RTThreadUserWait(m_hX11Thread, 10 * 1000 /* 10s timeout */);
        if (RT_FAILURE(rc))
            break;

        if (ASMAtomicReadBool(&m_fSrvStopping))
            break;

    } while (0);

    if (m_fSrvStopping)
        rc = VERR_GENERAL_FAILURE; /** @todo Fudge! */

    if (RT_FAILURE(rc))
        LogRel(("DnD: Failed to initialize, rc=%Rrc\n", rc));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Main loop for the drag and drop service which does the HGCM message
 * processing and routing to the according drag and drop instance(s).
 *
 * @returns IPRT status code.
 * @param   fDaemonised             Whether to run in daemonized or not. Does not
 *                                  apply for this service.
 */
int DragAndDropService::run(bool fDaemonised /* = false */)
{
    RT_NOREF1(fDaemonised);
    LogFlowThisFunc(("fDaemonised=%RTbool\n", fDaemonised));

    int rc;
    do
    {
        m_pCurDnD = new DragInstance(m_pDisplay, this);
        if (!m_pCurDnD)
        {
            rc = VERR_NO_MEMORY;
            break;
        }

        /* Note: For multiple screen support in VBox it is not necessary to use
         * another screen number than zero. Maybe in the future it will become
         * necessary if VBox supports multiple X11 screens. */
        rc = m_pCurDnD->init(0 /* uScreenID */);
        /* Note: Can return VINF_PERMISSION_DENIED if HGCM host service is not available. */
        if (rc != VINF_SUCCESS)
        {
            if (RT_FAILURE(rc))
                LogRel(("DnD: Unable to connect to drag and drop service, rc=%Rrc\n", rc));
            else if (rc == VINF_PERMISSION_DENIED)
                LogRel(("DnD: Not available on host, terminating\n"));
            break;
        }

        LogRel(("DnD: Started\n"));
        LogRel2(("DnD: %sr%s\n", RTBldCfgVersion(), RTBldCfgRevisionStr()));

        /* Enter the main event processing loop. */
        do
        {
            DnDEvent e;
            RT_ZERO(e);

            LogFlowFunc(("Waiting for new event ...\n"));
            rc = RTSemEventWait(m_hEventSem, RT_INDEFINITE_WAIT);
            if (RT_FAILURE(rc))
                break;

            AssertMsg(m_eventQueue.size(),
                      ("Event queue is empty when it shouldn't\n"));

            e = m_eventQueue.first();
            m_eventQueue.removeFirst();

            if (e.type == DnDEvent::HGCM_Type)
            {
                LogFlowThisFunc(("HGCM event, type=%RU32\n", e.hgcm.uType));
                switch (e.hgcm.uType)
                {
                    case DragAndDropSvc::HOST_DND_HG_EVT_ENTER:
                    {
                        if (e.hgcm.cbFormats)
                        {
                            RTCList<RTCString> lstFormats = RTCString(e.hgcm.pszFormats, e.hgcm.cbFormats - 1).split("\r\n");
                            rc = m_pCurDnD->hgEnter(lstFormats, e.hgcm.u.a.uAllActions);
                            /* Enter is always followed by a move event. */
                        }
                        else
                        {
                            rc = VERR_INVALID_PARAMETER;
                            break;
                        }
                        /* Not breaking unconditionally is intentional. See comment above. */
                    }
                    RT_FALL_THRU();
                    case DragAndDropSvc::HOST_DND_HG_EVT_MOVE:
                    {
                        rc = m_pCurDnD->hgMove(e.hgcm.u.a.uXpos, e.hgcm.u.a.uYpos, e.hgcm.u.a.uDefAction);
                        break;
                    }
                    case DragAndDropSvc::HOST_DND_HG_EVT_LEAVE:
                    {
                        rc = m_pCurDnD->hgLeave();
                        break;
                    }
                    case DragAndDropSvc::HOST_DND_HG_EVT_DROPPED:
                    {
                        rc = m_pCurDnD->hgDrop(e.hgcm.u.a.uXpos, e.hgcm.u.a.uYpos, e.hgcm.u.a.uDefAction);
                        break;
                    }
                    case DragAndDropSvc::HOST_DND_HG_SND_DATA:
                    {
                        rc = m_pCurDnD->hgDataReceived(e.hgcm.u.b.pvData, e.hgcm.u.b.cbData);
                        break;
                    }
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
                    case DragAndDropSvc::HOST_DND_GH_REQ_PENDING:
                    {
                        rc = m_pCurDnD->ghIsDnDPending();
                        break;
                    }
                    case DragAndDropSvc::HOST_DND_GH_EVT_DROPPED:
                    {
                        rc = m_pCurDnD->ghDropped(e.hgcm.pszFormats, e.hgcm.u.a.uDefAction);
                        break;
                    }
#endif
                    default:
                    {
                        m_pCurDnD->logError("Received unsupported message: %RU32\n", e.hgcm.uType);
                        rc = VERR_NOT_SUPPORTED;
                        break;
                    }
                }

                LogFlowFunc(("Message %RU32 processed with %Rrc\n", e.hgcm.uType, rc));
                if (RT_FAILURE(rc))
                {
                    /* Tell the user. */
                    m_pCurDnD->logError("Error processing message %RU32, failed with %Rrc, resetting all\n", e.hgcm.uType, rc);

                    /* If anything went wrong, do a reset and start over. */
                    m_pCurDnD->reset();
                }

                /* Some messages require cleanup. */
                switch (e.hgcm.uType)
                {
                    case DragAndDropSvc::HOST_DND_HG_EVT_ENTER:
                    case DragAndDropSvc::HOST_DND_HG_EVT_MOVE:
                    case DragAndDropSvc::HOST_DND_HG_EVT_DROPPED:
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
                    case DragAndDropSvc::HOST_DND_GH_EVT_DROPPED:
#endif
                    {
                        if (e.hgcm.pszFormats)
                            RTMemFree(e.hgcm.pszFormats);
                        break;
                    }

                    case DragAndDropSvc::HOST_DND_HG_SND_DATA:
                    {
                        if (e.hgcm.pszFormats)
                            RTMemFree(e.hgcm.pszFormats);
                        if (e.hgcm.u.b.pvData)
                            RTMemFree(e.hgcm.u.b.pvData);
                        break;
                    }

                    default:
                        break;
                }
            }
            else if (e.type == DnDEvent::X11_Type)
            {
                m_pCurDnD->onX11Event(e.x11);
            }
            else
                AssertMsgFailed(("Unknown event queue type %d\n", e.type));

            /*
             * Make sure that any X11 requests have actually been sent to the
             * server, since we are waiting for responses using poll() on
             * another thread which will not automatically trigger flushing.
             */
            XFlush(m_pDisplay);

        } while (!ASMAtomicReadBool(&m_fSrvStopping));

        LogRel(("DnD: Stopped with rc=%Rrc\n", rc));

    } while (0);

    if (m_pCurDnD)
    {
        delete m_pCurDnD;
        m_pCurDnD = NULL;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

void DragAndDropService::cleanup(void)
{
    LogFlowFuncEnter();

    LogRel2(("DnD: Terminating threads ...\n"));

    ASMAtomicXchgBool(&m_fSrvStopping, true);

    /*
     * Wait for threads to terminate.
     */
    int rcThread, rc2;
    if (m_hHGCMThread != NIL_RTTHREAD)
    {
#if 0 /** @todo Does not work because we don't cancel the HGCM call! */
        rc2 = RTThreadWait(m_hHGCMThread, 30 * 1000 /* 30s timeout */, &rcThread);
#else
        rc2 = RTThreadWait(m_hHGCMThread, 200 /* 200ms timeout */, &rcThread);
#endif
        if (RT_SUCCESS(rc2))
            rc2 = rcThread;

        if (RT_FAILURE(rc2))
            LogRel(("DnD: Error waiting for HGCM thread to terminate: %Rrc\n", rc2));
    }

    if (m_hX11Thread != NIL_RTTHREAD)
    {
#if 0
        rc2 = RTThreadWait(m_hX11Thread, 30 * 1000 /* 30s timeout */, &rcThread);
#else
        rc2 = RTThreadWait(m_hX11Thread, 200 /* 200ms timeout */, &rcThread);
#endif
        if (RT_SUCCESS(rc2))
            rc2 = rcThread;

        if (RT_FAILURE(rc2))
            LogRel(("DnD: Error waiting for X11 thread to terminate: %Rrc\n", rc2));
    }

    LogRel2(("DnD: Terminating threads done\n"));

    xHelpers::destroyInstance();

    VbglR3Term();
}

/**
 * Static callback function for HGCM message processing thread. An internal
 * message queue will be filled which then will be processed by the according
 * drag'n drop instance.
 *
 * @returns IPRT status code.
 * @param   hThread                 Thread handle to use.
 * @param   pvUser                  Pointer to DragAndDropService instance to use.
 */
/* static */
DECLCALLBACK(int) DragAndDropService::hgcmEventThread(RTTHREAD hThread, void *pvUser)
{
    AssertPtrReturn(pvUser, VERR_INVALID_PARAMETER);
    DragAndDropService *pThis = static_cast<DragAndDropService*>(pvUser);
    AssertPtr(pThis);

    /* This thread has an own DnD context, e.g. an own client ID. */
    VBGLR3GUESTDNDCMDCTX dndCtx;

    /*
     * Initialize thread.
     */
    int rc = VbglR3DnDConnect(&dndCtx);

    /* Set stop indicator on failure. */
    if (RT_FAILURE(rc))
        ASMAtomicXchgBool(&pThis->m_fSrvStopping, true);

    /* Let the service instance know in any case. */
    int rc2 = RTThreadUserSignal(hThread);
    AssertRC(rc2);

    if (RT_FAILURE(rc))
        return rc;

    /* Number of invalid messages skipped in a row. */
    int cMsgSkippedInvalid = 0;
    DnDEvent e;

    do
    {
        RT_ZERO(e);
        e.type = DnDEvent::HGCM_Type;

        /* Wait for new events. */
        rc = VbglR3DnDRecvNextMsg(&dndCtx, &e.hgcm);
        if (   RT_SUCCESS(rc)
            || rc == VERR_CANCELLED)
        {
            cMsgSkippedInvalid = 0; /* Reset skipped messages count. */
            pThis->m_eventQueue.append(e);

            rc = RTSemEventSignal(pThis->m_hEventSem);
            if (RT_FAILURE(rc))
                break;
        }
        else
        {
            LogRel(("DnD: Processing next message failed with rc=%Rrc\n", rc));

            /* Old(er) hosts either are broken regarding DnD support or otherwise
             * don't support the stuff we do on the guest side, so make sure we
             * don't process invalid messages forever. */
            if (rc == VERR_INVALID_PARAMETER)
                cMsgSkippedInvalid++;
            if (cMsgSkippedInvalid > 32)
            {
                LogRel(("DnD: Too many invalid/skipped messages from host, exiting ...\n"));
                break;
            }
        }

    } while (!ASMAtomicReadBool(&pThis->m_fSrvStopping));

    VbglR3DnDDisconnect(&dndCtx);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Static callback function for X11 message processing thread. All X11 messages
 * will be directly routed to the according drag'n drop instance.
 *
 * @returns IPRT status code.
 * @param   hThread                 Thread handle to use.
 * @param   pvUser                  Pointer to DragAndDropService instance to use.
 */
/* static */
DECLCALLBACK(int) DragAndDropService::x11EventThread(RTTHREAD hThread, void *pvUser)
{
    AssertPtrReturn(pvUser, VERR_INVALID_PARAMETER);
    DragAndDropService *pThis = static_cast<DragAndDropService*>(pvUser);
    AssertPtr(pThis);

    int rc = VINF_SUCCESS;

    /* Note: Nothing to initialize here (yet). */

    /* Set stop indicator on failure. */
    if (RT_FAILURE(rc))
        ASMAtomicXchgBool(&pThis->m_fSrvStopping, true);

    /* Let the service instance know in any case. */
    int rc2 = RTThreadUserSignal(hThread);
    AssertRC(rc2);

    DnDEvent e;
    do
    {
        /*
         * Wait for new events. We can't use XIfEvent here, cause this locks
         * the window connection with a mutex and if no X11 events occurs this
         * blocks any other calls we made to X11. So instead check for new
         * events and if there are not any new one, sleep for a certain amount
         * of time.
         */
        if (XEventsQueued(pThis->m_pDisplay, QueuedAfterFlush) > 0)
        {
            RT_ZERO(e);
            e.type = DnDEvent::X11_Type;

            /* XNextEvent will block until a new X event becomes available. */
            XNextEvent(pThis->m_pDisplay, &e.x11);
            {
#ifdef DEBUG
                switch (e.x11.type)
                {
                    case ClientMessage:
                    {
                        XClientMessageEvent *pEvent = reinterpret_cast<XClientMessageEvent*>(&e);
                        AssertPtr(pEvent);

                        RTCString strType = xAtomToString(pEvent->message_type);
                        LogFlowFunc(("ClientMessage: %s from wnd=%#x\n", strType.c_str(), pEvent->window));
                        break;
                    }

                    default:
                        LogFlowFunc(("Received X event type=%d\n", e.x11.type));
                        break;
                }
#endif
                /* At the moment we only have one drag instance. */
                DragInstance *pInstance = pThis->m_pCurDnD;
                AssertPtr(pInstance);

                pInstance->onX11Event(e.x11);
            }
        }
        else
            RTThreadSleep(25 /* ms */);

    } while (!ASMAtomicReadBool(&pThis->m_fSrvStopping));

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/** Drag and drop magic number, start of a UUID. */
#define DRAGANDDROPSERVICE_MAGIC 0x67c97173

/** VBoxClient service class wrapping the logic for the service while
 *  the main VBoxClient code provides the daemon logic needed by all services.
 */
struct DRAGANDDROPSERVICE
{
    /** The service interface. */
    struct VBCLSERVICE *pInterface;
    /** Magic number for sanity checks. */
    uint32_t uMagic;
    /** Service object. */
    DragAndDropService mDragAndDrop;
};

static const char *getPidFilePath()
{
    return ".vboxclient-draganddrop.pid";
}

static int init(struct VBCLSERVICE **ppInterface)
{
    struct DRAGANDDROPSERVICE *pSelf = (struct DRAGANDDROPSERVICE *)ppInterface;

    if (pSelf->uMagic != DRAGANDDROPSERVICE_MAGIC)
        VBClFatalError(("Bad DnD service object!\n"));
    return pSelf->mDragAndDrop.init();
}

static int run(struct VBCLSERVICE **ppInterface, bool fDaemonised)
{
    struct DRAGANDDROPSERVICE *pSelf = (struct DRAGANDDROPSERVICE *)ppInterface;

    if (pSelf->uMagic != DRAGANDDROPSERVICE_MAGIC)
        VBClFatalError(("Bad DnD service object!\n"));
    return pSelf->mDragAndDrop.run(fDaemonised);
}

static void cleanup(struct VBCLSERVICE **ppInterface)
{
   struct DRAGANDDROPSERVICE *pSelf = (struct DRAGANDDROPSERVICE *)ppInterface;

    if (pSelf->uMagic != DRAGANDDROPSERVICE_MAGIC)
        VBClFatalError(("Bad DnD service object!\n"));
    return pSelf->mDragAndDrop.cleanup();
}

struct VBCLSERVICE vbclDragAndDropInterface =
{
    getPidFilePath,
    init,
    run,
    cleanup
};

/* Static factory. */
struct VBCLSERVICE **VBClGetDragAndDropService(void)
{
    struct DRAGANDDROPSERVICE *pService =
        (struct DRAGANDDROPSERVICE *)RTMemAlloc(sizeof(*pService));

    if (!pService)
        VBClFatalError(("Out of memory\n"));
    pService->pInterface = &vbclDragAndDropInterface;
    pService->uMagic = DRAGANDDROPSERVICE_MAGIC;
    new(&pService->mDragAndDrop) DragAndDropService();
    return &pService->pInterface;
}
