/* $Id: UIFrameBuffer.cpp $ */
/** @file
 * VBox Qt GUI - UIFrameBuffer class implementation.
 */

/*
 * Copyright (C) 2010-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */
/* Qt includes: */
# include <QImage>
# include <QRegion>
# include <QPainter>
# include <QTransform>
/* GUI includes: */
# include "UIFrameBuffer.h"
# include "UISession.h"
# include "UIMachineLogic.h"
# include "UIMachineWindow.h"
# include "UIMachineView.h"
# include "UIPopupCenter.h"
# include "UIExtraDataManager.h"
# include "VBoxGlobal.h"
# ifdef VBOX_WITH_MASKED_SEAMLESS
#  include "UIMachineWindow.h"
# endif /* VBOX_WITH_MASKED_SEAMLESS */
# ifdef VBOX_WITH_VIDEOHWACCEL
#  include "VBoxFBOverlay.h"
# endif /* VBOX_WITH_VIDEOHWACCEL */
/* COM includes: */
# include "CConsole.h"
# include "CDisplay.h"
# include "CFramebuffer.h"
# include "CDisplaySourceBitmap.h"
#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* VirtualBox interface declarations: */
#ifndef VBOX_WITH_XPCOM
# include "VirtualBox.h"
#else /* !VBOX_WITH_XPCOM */
# include "VirtualBox_XPCOM.h"
#endif /* VBOX_WITH_XPCOM */

/* Other VBox includes: */
#include <iprt/critsect.h>
#include <VBoxVideo3D.h>

/* Other includes: */
#include <math.h>

#ifdef VBOX_WS_X11
/* X11 includes: */
# include <QX11Info>
# include <X11/Xlib.h>
# undef Bool // Qt5 vs Xlib gift..
#endif /* VBOX_WS_X11 */


/** IFramebuffer implementation used to maintain VM display video memory. */
class ATL_NO_VTABLE UIFrameBufferPrivate : public QObject,
                                           public ATL::CComObjectRootEx<ATL::CComMultiThreadModel>,
                                           VBOX_SCRIPTABLE_IMPL(IFramebuffer)
{
    Q_OBJECT;

signals:

    /** Notifies listener about guest-screen resolution changes. */
    void sigNotifyChange(int iWidth, int iHeight);
    /** Notifies listener about guest-screen updates. */
    void sigNotifyUpdate(int iX, int iY, int iWidth, int iHeight);
    /** Notifies listener about guest-screen visible-region changes. */
    void sigSetVisibleRegion(QRegion region);
    /** Notifies listener about guest 3D capability changes. */
    void sigNotifyAbout3DOverlayVisibilityChange(bool fVisible);

public:

    /** Frame-buffer constructor. */
    UIFrameBufferPrivate();
    /** Frame-buffer destructor. */
    ~UIFrameBufferPrivate();

    /** Frame-buffer initialization.
      * @param pMachineView defines machine-view this frame-buffer is bounded to. */
    virtual HRESULT init(UIMachineView *pMachineView);

    /** Assigns machine-view frame-buffer will be bounded to.
      * @param pMachineView defines machine-view this frame-buffer is bounded to. */
    virtual void setView(UIMachineView *pMachineView);

    /** Returns the copy of the IDisplay wrapper. */
    CDisplay display() const { return m_display; }
    /** Attach frame-buffer to IDisplay. */
    void attach();
    /** Detach frame-buffer from IDisplay. */
    void detach();

    /** Returns frame-buffer data address. */
    uchar *address() { return m_image.bits(); }
    /** Returns frame-buffer width. */
    ulong width() const { return m_iWidth; }
    /** Returns frame-buffer height. */
    ulong height() const { return m_iHeight; }
    /** Returns frame-buffer bits-per-pixel value. */
    ulong bitsPerPixel() const { return m_image.depth(); }
    /** Returns frame-buffer bytes-per-line value. */
    ulong bytesPerLine() const { return m_image.bytesPerLine(); }
    /** Returns default frame-buffer pixel-format. */
    ulong pixelFormat() const { return KBitmapFormat_BGR; }
    /** Returns the visual-state this frame-buffer is used for. */
    UIVisualStateType visualState() const { return m_pMachineView ? m_pMachineView->visualStateType() : UIVisualStateType_Invalid; }

    /** Defines whether frame-buffer is <b>unused</b>.
      * @note Refer to m_fUnused for more information.
      * @note Calls to this and any other EMT callback are synchronized (from GUI side). */
    void setMarkAsUnused(bool fUnused);

    /** Returns whether frame-buffer is <b>auto-enabled</b>.
      * @note Refer to m_fAutoEnabled for more information. */
    bool isAutoEnabled() const { return m_fAutoEnabled; }
    /** Defines whether frame-buffer is <b>auto-enabled</b>.
      * @note Refer to m_fAutoEnabled for more information. */
    void setAutoEnabled(bool fAutoEnabled) { m_fAutoEnabled = fAutoEnabled; }

    /** Returns the frame-buffer's scaled-size. */
    QSize scaledSize() const { return m_scaledSize; }
    /** Defines host-to-guest scale ratio as @a size. */
    void setScaledSize(const QSize &size = QSize()) { m_scaledSize = size; }
    /** Returns x-origin of the host (scaled) content corresponding to x-origin of guest (actual) content. */
    inline int convertGuestXTo(int x) const { return m_scaledSize.isValid() ? qRound((double)m_scaledSize.width() / m_iWidth * x) : x; }
    /** Returns y-origin of the host (scaled) content corresponding to y-origin of guest (actual) content. */
    inline int convertGuestYTo(int y) const { return m_scaledSize.isValid() ? qRound((double)m_scaledSize.height() / m_iHeight * y) : y; }
    /** Returns x-origin of the guest (actual) content corresponding to x-origin of host (scaled) content. */
    inline int convertHostXTo(int x) const  { return m_scaledSize.isValid() ? qRound((double)m_iWidth / m_scaledSize.width() * x) : x; }
    /** Returns y-origin of the guest (actual) content corresponding to y-origin of host (scaled) content. */
    inline int convertHostYTo(int y) const  { return m_scaledSize.isValid() ? qRound((double)m_iHeight / m_scaledSize.height() * y) : y; }

    /** Returns the scale-factor used by the frame-buffer. */
    double scaleFactor() const { return m_dScaleFactor; }
    /** Define the scale-factor used by the frame-buffer. */
    void setScaleFactor(double dScaleFactor) { m_dScaleFactor = dScaleFactor; }

    /** Returns backing-scale-factor used by HiDPI frame-buffer. */
    double backingScaleFactor() const { return m_dBackingScaleFactor; }
    /** Defines backing-scale-factor used by HiDPI frame-buffer. */
    void setBackingScaleFactor(double dBackingScaleFactor) { m_dBackingScaleFactor = dBackingScaleFactor; }

    /** Returns whether frame-buffer should use unscaled HiDPI output. */
    bool useUnscaledHiDPIOutput() const { return m_fUseUnscaledHiDPIOutput; }
    /** Defines whether frame-buffer should use unscaled HiDPI output. */
    void setUseUnscaledHiDPIOutput(bool fUseUnscaledHiDPIOutput) { m_fUseUnscaledHiDPIOutput = fUseUnscaledHiDPIOutput; }

    /** Returns frame-buffer scaling optimization type. */
    ScalingOptimizationType scalingOptimizationType() const { return m_enmScalingOptimizationType; }
    /** Defines frame-buffer scaling optimization type: */
    void setScalingOptimizationType(ScalingOptimizationType type) { m_enmScalingOptimizationType = type; }

    /** Returns HiDPI frame-buffer optimization type. */
    HiDPIOptimizationType hiDPIOptimizationType() const { return m_hiDPIOptimizationType; }
    /** Defines HiDPI frame-buffer optimization type: */
    void setHiDPIOptimizationType(HiDPIOptimizationType type) { m_hiDPIOptimizationType = type; }

    DECLARE_NOT_AGGREGATABLE(UIFrameBufferPrivate)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(UIFrameBufferPrivate)
        COM_INTERFACE_ENTRY(IFramebuffer)
        COM_INTERFACE_ENTRY2(IDispatch,IFramebuffer)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.m_p)
    END_COM_MAP()

    HRESULT FinalConstruct();
    void FinalRelease();

    STDMETHOD(COMGETTER(Width))(ULONG *puWidth);
    STDMETHOD(COMGETTER(Height))(ULONG *puHeight);
    STDMETHOD(COMGETTER(BitsPerPixel))(ULONG *puBitsPerPixel);
    STDMETHOD(COMGETTER(BytesPerLine))(ULONG *puBytesPerLine);
    STDMETHOD(COMGETTER(PixelFormat))(BitmapFormat_T *puPixelFormat);
    STDMETHOD(COMGETTER(HeightReduction))(ULONG *puHeightReduction);
    STDMETHOD(COMGETTER(Overlay))(IFramebufferOverlay **ppOverlay);
    STDMETHOD(COMGETTER(WinId))(LONG64 *pWinId);
    STDMETHOD(COMGETTER(Capabilities))(ComSafeArrayOut(FramebufferCapabilities_T, aCapabilities));

    /** EMT callback: Notifies frame-buffer about guest-screen size change.
      * @param        uScreenId Guest screen number.
      * @param        uX        Horizontal origin of the update rectangle, in pixels.
      * @param        uY        Vertical origin of the update rectangle, in pixels.
      * @param        uWidth    Width of the guest display, in pixels.
      * @param        uHeight   Height of the guest display, in pixels.
      * @note         Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
      * @note         Calls to this and #setMarkAsUnused method are synchronized (from GUI side). */
    STDMETHOD(NotifyChange)(ULONG uScreenId, ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight);

    /** EMT callback: Notifies frame-buffer about guest-screen update.
      * @param        uX      Horizontal origin of the update rectangle, in pixels.
      * @param        uY      Vertical origin of the update rectangle, in pixels.
      * @param        uWidth  Width of the update rectangle, in pixels.
      * @param        uHeight Height of the update rectangle, in pixels.
      * @note         Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
      * @note         Calls to this and #setMarkAsUnused method are synchronized (from GUI side). */
    STDMETHOD(NotifyUpdate)(ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight);

    /** EMT callback: Notifies frame-buffer about guest-screen update.
      * @param        uX      Horizontal origin of the update rectangle, in pixels.
      * @param        uY      Vertical origin of the update rectangle, in pixels.
      * @param        uWidth  Width of the update rectangle, in pixels.
      * @param        uHeight Height of the update rectangle, in pixels.
      * @param        image   Brings image container which can be used to copy data from.
      * @note         Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
      * @note         Calls to this and #setMarkAsUnused method are synchronized (from GUI side). */
    STDMETHOD(NotifyUpdateImage)(ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight, ComSafeArrayIn(BYTE, image));

    /** EMT callback: Returns whether the frame-buffer implementation is willing to support a given video-mode.
      * @param        uWidth      Width of the guest display, in pixels.
      * @param        uHeight     Height of the guest display, in pixels.
      * @param        uBPP        Color depth, bits per pixel.
      * @param        pfSupported Is frame-buffer able/willing to render the video mode or not.
      * @note         Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
      * @note         Calls to this and #setMarkAsUnused method are synchronized (from GUI side). */
    STDMETHOD(VideoModeSupported)(ULONG uWidth, ULONG uHeight, ULONG uBPP, BOOL *pbSupported);

    /** EMT callback which is not used in current implementation. */
    STDMETHOD(GetVisibleRegion)(BYTE *pRectangles, ULONG uCount, ULONG *puCountCopied);
    /** EMT callback: Suggests new visible-region to this frame-buffer.
      * @param        pRectangles Pointer to the RTRECT array.
      * @param        uCount      Number of RTRECT elements in the rectangles array.
      * @note         Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
      * @note         Calls to this and #setMarkAsUnused method are synchronized (from GUI side). */
    STDMETHOD(SetVisibleRegion)(BYTE *pRectangles, ULONG uCount);

    /** EMT callback which is not used in current implementation. */
    STDMETHOD(ProcessVHWACommand)(BYTE *pCommand);

    /** EMT callback: Notifies frame-buffer about 3D backend event.
      * @param        uType Event type. Currently only VBOX3D_NOTIFY_EVENT_TYPE_VISIBLE_3DDATA is supported.
      * @param        aData Event-specific data, depends on the supplied event type.
      * @note         Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
      * @note         Calls to this and #setMarkAsUnused method are synchronized (from GUI side). */
    STDMETHOD(Notify3DEvent)(ULONG uType, ComSafeArrayIn(BYTE, data));

    /** Locks frame-buffer access. */
    void lock() const { RTCritSectEnter(&m_critSect); }
    /** Unlocks frame-buffer access. */
    void unlock() const { RTCritSectLeave(&m_critSect); }

    /** Handles frame-buffer notify-change-event. */
    virtual void handleNotifyChange(int iWidth, int iHeight);
    /** Handles frame-buffer paint-event. */
    virtual void handlePaintEvent(QPaintEvent *pEvent);
    /** Handles frame-buffer set-visible-region-event. */
    virtual void handleSetVisibleRegion(const QRegion &region);

    /** Performs frame-buffer resizing. */
    virtual void performResize(int iWidth, int iHeight);
    /** Performs frame-buffer rescaling. */
    virtual void performRescale();

#ifdef VBOX_WITH_VIDEOHWACCEL
    /** Performs Video HW Acceleration command. */
    virtual void doProcessVHWACommand(QEvent*) {}
    /** Handles viewport resize-event. */
    virtual void viewportResized(QResizeEvent*) {}
    /** Handles viewport scroll-event. */
    virtual void viewportScrolled(int, int) {}
#endif /* VBOX_WITH_VIDEOHWACCEL */

protected:

    /** Prepare connections routine. */
    void prepareConnections();
    /** Cleanup connections routine. */
    void cleanupConnections();

    /** Updates coordinate-system: */
    void updateCoordinateSystem();

    /** Default paint routine. */
    void paintDefault(QPaintEvent *pEvent);
    /** Paint routine for seamless mode. */
    void paintSeamless(QPaintEvent *pEvent);

    /** Returns the transformation mode corresponding to the passed @a dScaleFactor and ScalingOptimizationType. */
    static Qt::TransformationMode transformationMode(ScalingOptimizationType type, double dScaleFactor = 0);

    /** Erases corresponding @a rect with @a painter. */
    static void eraseImageRect(QPainter &painter, const QRect &rect,
                               bool fUseUnscaledHiDPIOutput,
                               HiDPIOptimizationType hiDPIOptimizationType,
                               double dBackingScaleFactor);
    /** Draws corresponding @a rect of passed @a image with @a painter. */
    static void drawImageRect(QPainter &painter, const QImage &image, const QRect &rect,
                              int iContentsShiftX, int iContentsShiftY,
                              bool fUseUnscaledHiDPIOutput,
                              HiDPIOptimizationType hiDPIOptimizationType,
                              double dBackingScaleFactor);

    /** Holds the screen-id. */
    ulong m_uScreenId;

    /** Holds the QImage buffer. */
    QImage m_image;
    /** Holds frame-buffer width. */
    int m_iWidth;
    /** Holds frame-buffer height. */
    int m_iHeight;

    /** Holds the copy of the IDisplay wrapper. */
    CDisplay m_display;
    /** Source bitmap from IDisplay. */
    CDisplaySourceBitmap m_sourceBitmap;
    /** Source bitmap from IDisplay (acquired but not yet applied). */
    CDisplaySourceBitmap m_pendingSourceBitmap;
    /** Holds whether there is a pending source bitmap which must be applied. */
    bool m_fPendingSourceBitmap;

    /** Holds machine-view this frame-buffer is bounded to. */
    UIMachineView *m_pMachineView;
    /** Holds window ID this frame-buffer referring to. */
    int64_t m_iWinId;

    /** Reflects whether screen-updates are allowed. */
    bool m_fUpdatesAllowed;

    /** Defines whether frame-buffer is <b>unused</b>.
      * <b>Unused</b> status determines whether frame-buffer should ignore EMT events or not. */
    bool m_fUnused;

    /** Defines whether frame-buffer is <b>auto-enabled</b>.
      * <b>Auto-enabled</b> status means that guest-screen corresponding to this frame-buffer
      * was automatically enabled by the multi-screen layout (auto-mount guest-screen) functionality,
      * so have potentially incorrect size-hint posted into guest event queue.
      * Machine-view will try to automatically adjust guest-screen size as soon as possible. */
    bool m_fAutoEnabled;

    /** RTCRITSECT object to protect frame-buffer access. */
    mutable RTCRITSECT m_critSect;

    /** @name Scale-factor related variables.
     * @{ */
    /** Holds the scale-factor used by the scaled-size. */
    double m_dScaleFactor;
    /** Holds the scaling optimization type used by the scaling mechanism. */
    ScalingOptimizationType m_enmScalingOptimizationType;
    /** Holds the coordinate-system for the scale-factor above. */
    QTransform m_transform;
    /** Holds the frame-buffer's scaled-size. */
    QSize m_scaledSize;
    /** @} */

    /** @name Seamless mode related variables.
     * @{ */
    /* To avoid a seamless flicker which caused by the latency between
     * the initial visible-region arriving from EMT thread
     * and actual visible-region appliance on GUI thread
     * it was decided to use two visible-region instances: */
    /** Sync visible-region which being updated synchronously by locking EMT thread.
      * Used for immediate manual clipping of the painting operations. */
    QRegion m_syncVisibleRegion;
    /** Async visible-region which being updated asynchronously by posting async-event from EMT to GUI thread,
      * Used to update viewport parts for visible-region changes,
      * because NotifyUpdate doesn't take into account these changes. */
    QRegion m_asyncVisibleRegion;
    /** When the frame-buffer is being resized, visible region is saved here.
      * The saved region is applied when updates are enabled again. */
    QRegion m_pendingSyncVisibleRegion;
    /** @} */

    /** @name HiDPI screens related variables.
     * @{ */
    /** Holds backing-scale-factor used by HiDPI frame-buffer. */
    double m_dBackingScaleFactor;
    /** Holds whether frame-buffer should use unscaled HiDPI output. */
    bool m_fUseUnscaledHiDPIOutput;
    /** Holds HiDPI frame-buffer optimization type. */
    HiDPIOptimizationType m_hiDPIOptimizationType;
    /** @} */

private:

#ifdef Q_OS_WIN
     ComPtr<IUnknown> m_pUnkMarshaler;
#endif /* Q_OS_WIN */
     /** Identifier returned by AttachFramebuffer. Used in DetachFramebuffer. */
     QString m_strFramebufferId;
};


#ifdef VBOX_WITH_VIDEOHWACCEL
/** UIFrameBufferPrivate reimplementation used to maintain VM display video memory
  *                      for the case when 2D Video Acceleration is enabled. */
class VBoxOverlayFrameBuffer : public UIFrameBufferPrivate
{
    Q_OBJECT;

public:

    VBoxOverlayFrameBuffer()
    {
    }

    virtual HRESULT init(UIMachineView *pView)
    {
        mpView = pView;
        UIFrameBufferPrivate::init(mpView);
        mOverlay.init(mpView->viewport(), mpView, &(mpView->session()), mpView->screenId()),
        /* sync with frame-buffer */
        mOverlay.onResizeEventPostprocess (VBoxFBSizeInfo(this), QPoint(mpView->contentsX(), mpView->contentsY()));
        return S_OK;
    }

    STDMETHOD(ProcessVHWACommand)(BYTE *pCommand)
    {
        int rc;
        UIFrameBufferPrivate::lock();
        /* Make sure frame-buffer is used: */
        if (m_fUnused)
        {
            LogRel2(("GUI: ProcessVHWACommand: Postponed!\n"));
            /* Unlock access to frame-buffer: */
            UIFrameBufferPrivate::unlock();
            /* tell client to pend ProcessVHWACommand */
            return E_ACCESSDENIED;
        }
        rc = mOverlay.onVHWACommand ((struct VBOXVHWACMD*)pCommand);
        UIFrameBufferPrivate::unlock();
        if (rc == VINF_CALLBACK_RETURN)
            return S_OK;
        else if (RT_SUCCESS(rc))
            return S_FALSE;
        else if (rc == VERR_INVALID_STATE)
            return E_ACCESSDENIED;
        return E_FAIL;
    }

    void doProcessVHWACommand (QEvent * pEvent)
    {
        mOverlay.onVHWACommandEvent (pEvent);
    }

    STDMETHOD(NotifyUpdate) (ULONG aX, ULONG aY,
                             ULONG aW, ULONG aH)
    {
        HRESULT hr = S_OK;
        UIFrameBufferPrivate::lock();
        /* Make sure frame-buffer is used: */
        if (m_fUnused)
        {
            LogRel3(("GUI: NotifyUpdate: Ignored!\n"));
            mOverlay.onNotifyUpdateIgnore (aX, aY, aW, aH);
            /* Unlock access to frame-buffer: */
            UIFrameBufferPrivate::unlock();
            /*can we actually ignore the notify update?*/
            /* Ignore NotifyUpdate: */
            return E_FAIL;
        }

        if (!mOverlay.onNotifyUpdate (aX, aY, aW, aH))
            hr = UIFrameBufferPrivate::NotifyUpdate (aX, aY, aW, aH);
        UIFrameBufferPrivate::unlock();
        return hr;
    }

    void performResize(int iWidth, int iHeight)
    {
        UIFrameBufferPrivate::performResize(iWidth, iHeight);
        mOverlay.onResizeEventPostprocess(VBoxFBSizeInfo(this),
                QPoint(mpView->contentsX(), mpView->contentsY()));
    }

    void performRescale()
    {
        UIFrameBufferPrivate::performRescale();
        mOverlay.onResizeEventPostprocess(VBoxFBSizeInfo(this),
                QPoint(mpView->contentsX(), mpView->contentsY()));
    }

    void viewportResized (QResizeEvent * re)
    {
        mOverlay.onViewportResized (re);
        UIFrameBufferPrivate::viewportResized (re);
    }

    void viewportScrolled (int dx, int dy)
    {
        mOverlay.onViewportScrolled (QPoint(mpView->contentsX(), mpView->contentsY()));
        UIFrameBufferPrivate::viewportScrolled (dx, dy);
    }

    void setView(UIMachineView * pView)
    {
        /* lock to ensure we do not collide with the EMT thread passing commands to us */
        UIFrameBufferPrivate::lock();
        UIFrameBufferPrivate::setView(pView);
        mpView = pView;
        mOverlay.updateAttachment(pView ? pView->viewport() : NULL, pView);
        UIFrameBufferPrivate::unlock();
    }

private:

    VBoxQGLOverlay mOverlay;
    UIMachineView *mpView;
};
#endif /* VBOX_WITH_VIDEOHWACCEL */


#ifdef VBOX_WITH_XPCOM
NS_DECL_CLASSINFO(UIFrameBufferPrivate)
NS_IMPL_THREADSAFE_ISUPPORTS1_CI(UIFrameBufferPrivate, IFramebuffer)
#endif /* VBOX_WITH_XPCOM */


UIFrameBufferPrivate::UIFrameBufferPrivate()
    : m_uScreenId(0)
    , m_iWidth(0), m_iHeight(0)
    , m_fPendingSourceBitmap(false)
    , m_pMachineView(NULL)
    , m_iWinId(0)
    , m_fUpdatesAllowed(false)
    , m_fUnused(false)
    , m_fAutoEnabled(false)
    , m_dScaleFactor(1.0)
    , m_enmScalingOptimizationType(ScalingOptimizationType_None)
    , m_dBackingScaleFactor(1.0)
    , m_fUseUnscaledHiDPIOutput(false)
    , m_hiDPIOptimizationType(HiDPIOptimizationType_None)
{
    /* Update coordinate-system: */
    updateCoordinateSystem();
}

HRESULT UIFrameBufferPrivate::init(UIMachineView *pMachineView)
{
    LogRel2(("GUI: UIFrameBufferPrivate::init %p\n", this));

    /* Assign mahine-view: */
    m_pMachineView = pMachineView;

    /* Assign index: */
    m_uScreenId = m_pMachineView->screenId();

    /* Cache window ID: */
    m_iWinId = (m_pMachineView && m_pMachineView->viewport()) ? (LONG64)m_pMachineView->viewport()->winId() : 0;

#ifdef VBOX_WS_X11
    /* Sync Qt and X11 Server (see xTracker #7547). */
    XSync(QX11Info::display(), false);
#endif

    /* Assign display: */
    m_display = m_pMachineView->uisession()->display();

    /* Initialize critical-section: */
    int rc = RTCritSectInit(&m_critSect);
    AssertRC(rc);

    /* Connect handlers: */
    if (m_pMachineView)
        prepareConnections();

    /* Resize/rescale frame-buffer to the default size: */
    performResize(640, 480);
    performRescale();

#ifdef Q_OS_WIN
    CoCreateFreeThreadedMarshaler(this, m_pUnkMarshaler.asOutParam());
#endif /* Q_OS_WIN */
    return S_OK;
}

UIFrameBufferPrivate::~UIFrameBufferPrivate()
{
    LogRel2(("GUI: UIFrameBufferPrivate::~UIFrameBufferPrivate %p\n", this));

    /* Disconnect handlers: */
    if (m_pMachineView)
        cleanupConnections();

    /* Deinitialize critical-section: */
    RTCritSectDelete(&m_critSect);
}

void UIFrameBufferPrivate::setView(UIMachineView *pMachineView)
{
    /* Disconnect old handlers: */
    if (m_pMachineView)
        cleanupConnections();

    /* Reassign machine-view: */
    m_pMachineView = pMachineView;
    /* Recache window ID: */
    m_iWinId = (m_pMachineView && m_pMachineView->viewport()) ? (LONG64)m_pMachineView->viewport()->winId() : 0;

#ifdef VBOX_WS_X11
    /* Sync Qt and X11 Server (see xTracker #7547). */
    XSync(QX11Info::display(), false);
#endif

    /* Connect new handlers: */
    if (m_pMachineView)
        prepareConnections();
}

void UIFrameBufferPrivate::attach()
{
    m_strFramebufferId = display().AttachFramebuffer(m_uScreenId, CFramebuffer(this));
}

void UIFrameBufferPrivate::detach()
{
    CFramebuffer frameBuffer = display().QueryFramebuffer(m_uScreenId);
    if (!frameBuffer.isNull())
    {
        display().DetachFramebuffer(m_uScreenId, m_strFramebufferId);
        m_strFramebufferId.clear();
    }
}

void UIFrameBufferPrivate::setMarkAsUnused(bool fUnused)
{
    lock();
    m_fUnused = fUnused;
    unlock();
}

HRESULT UIFrameBufferPrivate::FinalConstruct()
{
    return 0;
}

void UIFrameBufferPrivate::FinalRelease()
{
    return;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(Width)(ULONG *puWidth)
{
    if (!puWidth)
        return E_POINTER;
    *puWidth = (ULONG)width();
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(Height)(ULONG *puHeight)
{
    if (!puHeight)
        return E_POINTER;
    *puHeight = (ULONG)height();
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(BitsPerPixel)(ULONG *puBitsPerPixel)
{
    if (!puBitsPerPixel)
        return E_POINTER;
    *puBitsPerPixel = bitsPerPixel();
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(BytesPerLine)(ULONG *puBytesPerLine)
{
    if (!puBytesPerLine)
        return E_POINTER;
    *puBytesPerLine = bytesPerLine();
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(PixelFormat)(BitmapFormat_T *puPixelFormat)
{
    if (!puPixelFormat)
        return E_POINTER;
    *puPixelFormat = (BitmapFormat_T)pixelFormat();
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(HeightReduction)(ULONG *puHeightReduction)
{
    if (!puHeightReduction)
        return E_POINTER;
    *puHeightReduction = 0;
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(Overlay)(IFramebufferOverlay **ppOverlay)
{
    if (!ppOverlay)
        return E_POINTER;
    *ppOverlay = 0;
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(WinId)(LONG64 *pWinId)
{
    if (!pWinId)
        return E_POINTER;
    *pWinId = m_iWinId;
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::COMGETTER(Capabilities)(ComSafeArrayOut(FramebufferCapabilities_T, enmCapabilities))
{
    if (ComSafeArrayOutIsNull(enmCapabilities))
        return E_POINTER;

    com::SafeArray<FramebufferCapabilities_T> caps;
    if (vboxGlobal().isSeparateProcess())
    {
       caps.resize(1);
       caps[0] = FramebufferCapabilities_UpdateImage;
    }
    else
    {
       caps.resize(2);
       caps[0] = FramebufferCapabilities_VHWA;
       caps[1] = FramebufferCapabilities_VisibleRegion;
    }

    caps.detachTo(ComSafeArrayOutArg(enmCapabilities));
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::NotifyChange(ULONG uScreenId, ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight)
{
    CDisplaySourceBitmap sourceBitmap;
    if (!vboxGlobal().isSeparateProcess())
        display().QuerySourceBitmap(uScreenId, sourceBitmap);

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel(("GUI: UIFrameBufferPrivate::NotifyChange: Screen=%lu, Origin=%lux%lu, Size=%lux%lu, Ignored!\n",
                (unsigned long)uScreenId,
                (unsigned long)uX, (unsigned long)uY,
                (unsigned long)uWidth, (unsigned long)uHeight));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore NotifyChange: */
        return E_FAIL;
    }

    /* Disable screen updates: */
    m_fUpdatesAllowed = false;

    /* While updates are disabled, visible region will be saved:  */
    m_pendingSyncVisibleRegion = QRegion();

    if (!vboxGlobal().isSeparateProcess())
    {
       /* Acquire new pending bitmap: */
       m_pendingSourceBitmap = sourceBitmap;
       m_fPendingSourceBitmap = true;
    }

    /* Widget resize is NOT thread-safe and *probably* never will be,
     * We have to notify machine-view with the async-signal to perform resize operation. */
    LogRel(("GUI: UIFrameBufferPrivate::NotifyChange: Screen=%lu, Origin=%lux%lu, Size=%lux%lu, Sending to async-handler\n",
            (unsigned long)uScreenId,
            (unsigned long)uX, (unsigned long)uY,
            (unsigned long)uWidth, (unsigned long)uHeight));
    emit sigNotifyChange(uWidth, uHeight);

    /* Unlock access to frame-buffer: */
    unlock();

    /* Give up control token to other thread: */
    RTThreadYield();

    /* Confirm NotifyChange: */
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::NotifyUpdate(ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight)
{
    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel3(("GUI: UIFrameBufferPrivate::NotifyUpdate: Origin=%lux%lu, Size=%lux%lu, Ignored!\n",
                 (unsigned long)uX, (unsigned long)uY,
                 (unsigned long)uWidth, (unsigned long)uHeight));
        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore NotifyUpdate: */
        return E_FAIL;
    }

    /* Widget update is NOT thread-safe and *seems* never will be,
     * We have to notify machine-view with the async-signal to perform update operation. */
    LogRel3(("GUI: UIFrameBufferPrivate::NotifyUpdate: Origin=%lux%lu, Size=%lux%lu, Sending to async-handler\n",
             (unsigned long)uX, (unsigned long)uY,
             (unsigned long)uWidth, (unsigned long)uHeight));
    emit sigNotifyUpdate(uX, uY, uWidth, uHeight);

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm NotifyUpdate: */
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::NotifyUpdateImage(ULONG uX, ULONG uY,
                                                     ULONG uWidth, ULONG uHeight,
                                                     ComSafeArrayIn(BYTE, image))
{
    /* Wrapping received data: */
    com::SafeArray<BYTE> imageData(ComSafeArrayInArg(image));

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel3(("GUI: UIFrameBufferPrivate::NotifyUpdateImage: Origin=%lux%lu, Size=%lux%lu, Ignored!\n",
                 (unsigned long)uX, (unsigned long)uY,
                 (unsigned long)uWidth, (unsigned long)uHeight));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore NotifyUpdate: */
        return E_FAIL;
    }
    /* Directly update m_image if update fits: */
    if (   m_fUpdatesAllowed
        && uX + uWidth <= (ULONG)m_image.width()
        && uY + uHeight <= (ULONG)m_image.height())
    {
        /* Copy to m_image: */
        uchar *pu8Dst = m_image.bits() + uY * m_image.bytesPerLine() + uX * 4;
        uchar *pu8Src = imageData.raw();
        ULONG h;
        for (h = 0; h < uHeight; ++h)
        {
            memcpy(pu8Dst, pu8Src, uWidth * 4);
            pu8Dst += m_image.bytesPerLine();
            pu8Src += uWidth * 4;
        }

        /* Widget update is NOT thread-safe and *seems* never will be,
         * We have to notify machine-view with the async-signal to perform update operation. */
        LogRel3(("GUI: UIFrameBufferPrivate::NotifyUpdateImage: Origin=%lux%lu, Size=%lux%lu, Sending to async-handler\n",
                 (unsigned long)uX, (unsigned long)uY,
                 (unsigned long)uWidth, (unsigned long)uHeight));
        emit sigNotifyUpdate(uX, uY, uWidth, uHeight);
    }

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm NotifyUpdateImage: */
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::VideoModeSupported(ULONG uWidth, ULONG uHeight, ULONG uBPP, BOOL *pfSupported)
{
    /* Make sure result pointer is valid: */
    if (!pfSupported)
    {
        LogRel3(("GUI: UIFrameBufferPrivate::IsVideoModeSupported: Mode: BPP=%lu, Size=%lux%lu, Invalid pfSupported pointer!\n",
                 (unsigned long)uBPP, (unsigned long)uWidth, (unsigned long)uHeight));

        return E_POINTER;
    }

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel3(("GUI: UIFrameBufferPrivate::IsVideoModeSupported: Mode: BPP=%lu, Size=%lux%lu, Ignored!\n",
                 (unsigned long)uBPP, (unsigned long)uWidth, (unsigned long)uHeight));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore VideoModeSupported: */
        return E_FAIL;
    }

    /* Determine if supported: */
    *pfSupported = TRUE;
    QSize screenSize = m_pMachineView->maxGuestSize();
    if (   (screenSize.width() != 0)
        && (uWidth > (ULONG)screenSize.width())
        && (uWidth > (ULONG)width()))
        *pfSupported = FALSE;
    if (   (screenSize.height() != 0)
        && (uHeight > (ULONG)screenSize.height())
        && (uHeight > (ULONG)height()))
        *pfSupported = FALSE;
    LogRel3(("GUI: UIFrameBufferPrivate::IsVideoModeSupported: Mode: BPP=%lu, Size=%lux%lu, Supported=%s\n",
             (unsigned long)uBPP, (unsigned long)uWidth, (unsigned long)uHeight, *pfSupported ? "TRUE" : "FALSE"));

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm VideoModeSupported: */
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::GetVisibleRegion(BYTE *pRectangles, ULONG uCount, ULONG *puCountCopied)
{
    PRTRECT rects = (PRTRECT)pRectangles;

    if (!rects)
        return E_POINTER;

    Q_UNUSED(uCount);
    Q_UNUSED(puCountCopied);

    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::SetVisibleRegion(BYTE *pRectangles, ULONG uCount)
{
    /* Make sure rectangles were passed: */
    if (!pRectangles)
    {
        LogRel2(("GUI: UIFrameBufferPrivate::SetVisibleRegion: Rectangle count=%lu, Invalid pRectangles pointer!\n",
                 (unsigned long)uCount));

        return E_POINTER;
    }

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel2(("GUI: UIFrameBufferPrivate::SetVisibleRegion: Rectangle count=%lu, Ignored!\n",
                 (unsigned long)uCount));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore SetVisibleRegion: */
        return E_FAIL;
    }

    /* Compose region: */
    QRegion region;
    PRTRECT rects = (PRTRECT)pRectangles;
    for (ULONG uIndex = 0; uIndex < uCount; ++uIndex)
    {
        /* Get current rectangle: */
        QRect rect;
        rect.setLeft(rects->xLeft);
        rect.setTop(rects->yTop);
        /* Which is inclusive: */
        rect.setRight(rects->xRight - 1);
        rect.setBottom(rects->yBottom - 1);
        /* Append region: */
        region += rect;
        ++rects;
    }
    /* Tune according scale-factor: */
    if (scaleFactor() != 1.0 || backingScaleFactor() > 1.0)
        region = m_transform.map(region);

    if (m_fUpdatesAllowed)
    {
        /* We are directly updating synchronous visible-region: */
        m_syncVisibleRegion = region;
        /* And send async-signal to update asynchronous one: */
        LogRel2(("GUI: UIFrameBufferPrivate::SetVisibleRegion: Rectangle count=%lu, Sending to async-handler\n",
                 (unsigned long)uCount));
        emit sigSetVisibleRegion(region);
    }
    else
    {
        /* Save the region. */
        m_pendingSyncVisibleRegion = region;
        LogRel2(("GUI: UIFrameBufferPrivate::SetVisibleRegion: Rectangle count=%lu, Saved\n",
                 (unsigned long)uCount));
    }

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm SetVisibleRegion: */
    return S_OK;
}

STDMETHODIMP UIFrameBufferPrivate::ProcessVHWACommand(BYTE *pCommand)
{
    Q_UNUSED(pCommand);
    return E_NOTIMPL;
}

STDMETHODIMP UIFrameBufferPrivate::Notify3DEvent(ULONG uType, ComSafeArrayIn(BYTE, data))
{
    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fUnused)
    {
        LogRel2(("GUI: UIFrameBufferPrivate::Notify3DEvent: Ignored!\n"));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore Notify3DEvent: */
        return E_FAIL;
    }

    Q_UNUSED(data);
#ifdef VBOX_WITH_XPCOM
    Q_UNUSED(dataSize);
#endif /* VBOX_WITH_XPCOM */
    // com::SafeArray<BYTE> eventData(ComSafeArrayInArg(data));
    switch (uType)
    {
        case VBOX3D_NOTIFY_EVENT_TYPE_3DDATA_VISIBLE:
        case VBOX3D_NOTIFY_EVENT_TYPE_3DDATA_HIDDEN:
        {
            /* Notify machine-view with the async-signal
             * about 3D overlay visibility change: */
            BOOL fVisible = uType == VBOX3D_NOTIFY_EVENT_TYPE_3DDATA_VISIBLE;
            LogRel2(("GUI: UIFrameBufferPrivate::Notify3DEvent: Sending to async-handler: "
                     "(VBOX3D_NOTIFY_EVENT_TYPE_3DDATA_%s)\n",
                     fVisible ? "VISIBLE" : "HIDDEN"));
            emit sigNotifyAbout3DOverlayVisibilityChange(fVisible);

            /* Unlock access to frame-buffer: */
            unlock();

            /* Confirm Notify3DEvent: */
            return S_OK;
        }

        case VBOX3D_NOTIFY_EVENT_TYPE_TEST_FUNCTIONAL:
        {
            HRESULT hr = m_fUnused ? E_FAIL : S_OK;
            unlock();
            return hr;
        }

        default:
            break;
    }

    /* Unlock access to frame-buffer: */
    unlock();

    /* Ignore Notify3DEvent: */
    return E_INVALIDARG;
}

void UIFrameBufferPrivate::handleNotifyChange(int iWidth, int iHeight)
{
    LogRel(("GUI: UIFrameBufferPrivate::handleNotifyChange: Size=%dx%d\n", iWidth, iHeight));

    /* Make sure machine-view is assigned: */
    AssertPtrReturnVoid(m_pMachineView);

    /* Lock access to frame-buffer: */
    lock();

    /* If there is NO pending source-bitmap: */
    if (!vboxGlobal().isSeparateProcess() && !m_fPendingSourceBitmap)
    {
        /* Do nothing, change-event already processed: */
        LogRel2(("GUI: UIFrameBufferPrivate::handleNotifyChange: Already processed.\n"));
        /* Unlock access to frame-buffer: */
        unlock();
        /* Return immediately: */
        return;
    }

    /* Release the current bitmap and keep the pending one: */
    m_sourceBitmap = m_pendingSourceBitmap;
    m_pendingSourceBitmap = 0;
    m_fPendingSourceBitmap = false;

    /* Unlock access to frame-buffer: */
    unlock();

    /* Perform frame-buffer resize: */
    performResize(iWidth, iHeight);
}

void UIFrameBufferPrivate::handlePaintEvent(QPaintEvent *pEvent)
{
    LogRel3(("GUI: UIFrameBufferPrivate::handlePaintEvent: Origin=%lux%lu, Size=%dx%d\n",
             pEvent->rect().x(), pEvent->rect().y(),
             pEvent->rect().width(), pEvent->rect().height()));

    /* On mode switch the enqueued paint-event may still come
     * while the machine-view is already null (before the new machine-view set),
     * ignore paint-event in that case. */
    if (!m_pMachineView)
        return;

    /* Lock access to frame-buffer: */
    lock();

    /* But if updates disabled: */
    if (!m_fUpdatesAllowed)
    {
        /* Unlock access to frame-buffer: */
        unlock();
        /* And return immediately: */
        return;
    }

    /* Depending on visual-state type: */
    switch (m_pMachineView->machineLogic()->visualStateType())
    {
        case UIVisualStateType_Seamless:
            paintSeamless(pEvent);
            break;
        default:
            paintDefault(pEvent);
            break;
    }

    /* Unlock access to frame-buffer: */
    unlock();
}

void UIFrameBufferPrivate::handleSetVisibleRegion(const QRegion &region)
{
    /* Make sure async visible-region has changed or wasn't yet applied: */
    if (   m_asyncVisibleRegion == region
#ifdef VBOX_WITH_MASKED_SEAMLESS
        && m_asyncVisibleRegion == m_pMachineView->machineWindow()->mask()
#endif /* VBOX_WITH_MASKED_SEAMLESS */
           )
        return;

    /* We are accounting async visible-regions one-by-one
     * to keep corresponding viewport area always updated! */
    if (!m_asyncVisibleRegion.isEmpty())
        m_pMachineView->viewport()->update(m_asyncVisibleRegion - region);

    /* Remember last visible region: */
    m_asyncVisibleRegion = region;

#ifdef VBOX_WITH_MASKED_SEAMLESS
    /* We have to use async visible-region to apply to [Q]Widget [set]Mask: */
    m_pMachineView->machineWindow()->setMask(m_asyncVisibleRegion);
#endif /* VBOX_WITH_MASKED_SEAMLESS */
}

void UIFrameBufferPrivate::performResize(int iWidth, int iHeight)
{
    /* Make sure machine-view is assigned: */
    AssertReturnVoidStmt(m_pMachineView, LogRel(("GUI: UIFrameBufferPrivate::performResize: Size=%dx%d\n", iWidth, iHeight)));

    /* Invalidate visible-region (if necessary): */
    if (m_pMachineView->machineLogic()->visualStateType() == UIVisualStateType_Seamless &&
        (m_iWidth != iWidth || m_iHeight != iHeight))
    {
        lock();
        m_syncVisibleRegion = QRegion();
        m_asyncVisibleRegion = QRegion();
        unlock();
    }

    /* If source-bitmap invalid: */
    if (m_sourceBitmap.isNull())
    {
        LogRel(("GUI: UIFrameBufferPrivate::performResize: Size=%dx%d, "
                "Using fallback buffer since no source bitmap is provided\n", iWidth, iHeight));

        /* Remember new size came from hint: */
        m_iWidth = iWidth;
        m_iHeight = iHeight;

        /* And recreate fallback buffer: */
        m_image = QImage(m_iWidth, m_iHeight, QImage::Format_RGB32);
        m_image.fill(0);
    }
    /* If source-bitmap valid: */
    else
    {
        LogRel(("GUI: UIFrameBufferPrivate::performResize: Size=%dx%d, "
                "Directly using source bitmap content\n", iWidth, iHeight));

        /* Acquire source-bitmap attributes: */
        BYTE *pAddress = NULL;
        ULONG ulWidth = 0;
        ULONG ulHeight = 0;
        ULONG ulBitsPerPixel = 0;
        ULONG ulBytesPerLine = 0;
        KBitmapFormat bitmapFormat = KBitmapFormat_Opaque;
        m_sourceBitmap.QueryBitmapInfo(pAddress,
                                       ulWidth,
                                       ulHeight,
                                       ulBitsPerPixel,
                                       ulBytesPerLine,
                                       bitmapFormat);
        Assert(ulBitsPerPixel == 32);

        /* Remember new actual size: */
        m_iWidth = (int)ulWidth;
        m_iHeight = (int)ulHeight;

        /* Recreate QImage on the basis of source-bitmap content: */
        m_image = QImage(pAddress, m_iWidth, m_iHeight, ulBytesPerLine, QImage::Format_RGB32);

        /* Check whether guest color depth differs from the bitmap color depth: */
        ULONG ulGuestBitsPerPixel = 0;
        LONG xOrigin = 0;
        LONG yOrigin = 0;
        KGuestMonitorStatus monitorStatus = KGuestMonitorStatus_Enabled;
        display().GetScreenResolution(m_uScreenId, ulWidth, ulHeight, ulGuestBitsPerPixel, xOrigin, yOrigin, monitorStatus);

        /* Remind user if necessary, ignore text and VGA modes: */
        /* This check (supports graphics) is not quite right due to past mistakes
         * in the Guest Additions protocol, but in practice it should be fine. */
        if (   ulGuestBitsPerPixel != ulBitsPerPixel
            && ulGuestBitsPerPixel != 0
            && m_pMachineView->uisession()->isGuestSupportsGraphics())
            popupCenter().remindAboutWrongColorDepth(m_pMachineView->machineWindow(),
                                                     ulGuestBitsPerPixel, ulBitsPerPixel);
        else
            popupCenter().forgetAboutWrongColorDepth(m_pMachineView->machineWindow());
    }

    lock();

    /* Enable screen updates: */
    m_fUpdatesAllowed = true;

    if (!m_pendingSyncVisibleRegion.isEmpty())
    {
        /* Directly update synchronous visible-region: */
        m_syncVisibleRegion = m_pendingSyncVisibleRegion;
        m_pendingSyncVisibleRegion = QRegion();

        /* And send async-signal to update asynchronous one: */
        LogRel2(("GUI: UIFrameBufferPrivate::performResize: Rectangle count=%lu, Sending to async-handler\n",
                 (unsigned long)m_syncVisibleRegion.rectCount()));
        emit sigSetVisibleRegion(m_syncVisibleRegion);
    }

    /* Make sure that the current screen image is immediately displayed: */
    m_pMachineView->viewport()->update();

    unlock();
}

void UIFrameBufferPrivate::performRescale()
{
//    printf("UIFrameBufferPrivate::performRescale\n");

    /* Make sure machine-view is assigned: */
    AssertPtrReturnVoid(m_pMachineView);

    /* Depending on current visual state: */
    switch (m_pMachineView->machineLogic()->visualStateType())
    {
        case UIVisualStateType_Scale:
            m_scaledSize = m_scaledSize.width() == m_iWidth && m_scaledSize.height() == m_iHeight ? QSize() : m_scaledSize;
            break;
        default:
            m_scaledSize = scaleFactor() == 1.0 ? QSize() : QSize((int)(m_iWidth * scaleFactor()), (int)(m_iHeight * scaleFactor()));
            break;
    }

    /* Update coordinate-system: */
    updateCoordinateSystem();

//    printf("UIFrameBufferPrivate::performRescale: Complete: Scale-factor=%f, Scaled-size=%dx%d\n",
//           scaleFactor(), scaledSize().width(), scaledSize().height());
}

void UIFrameBufferPrivate::prepareConnections()
{
    /* Attach EMT connections: */
    connect(this, SIGNAL(sigNotifyChange(int, int)),
            m_pMachineView, SLOT(sltHandleNotifyChange(int, int)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(sigNotifyUpdate(int, int, int, int)),
            m_pMachineView, SLOT(sltHandleNotifyUpdate(int, int, int, int)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(sigSetVisibleRegion(QRegion)),
            m_pMachineView, SLOT(sltHandleSetVisibleRegion(QRegion)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(sigNotifyAbout3DOverlayVisibilityChange(bool)),
            m_pMachineView, SLOT(sltHandle3DOverlayVisibilityChange(bool)),
            Qt::QueuedConnection);
}

void UIFrameBufferPrivate::cleanupConnections()
{
    /* Detach EMT connections: */
    disconnect(this, SIGNAL(sigNotifyChange(int, int)),
               m_pMachineView, SLOT(sltHandleNotifyChange(int, int)));
    disconnect(this, SIGNAL(sigNotifyUpdate(int, int, int, int)),
               m_pMachineView, SLOT(sltHandleNotifyUpdate(int, int, int, int)));
    disconnect(this, SIGNAL(sigSetVisibleRegion(QRegion)),
               m_pMachineView, SLOT(sltHandleSetVisibleRegion(QRegion)));
    disconnect(this, SIGNAL(sigNotifyAbout3DOverlayVisibilityChange(bool)),
               m_pMachineView, SLOT(sltHandle3DOverlayVisibilityChange(bool)));
}

void UIFrameBufferPrivate::updateCoordinateSystem()
{
    /* Reset to default: */
    m_transform = QTransform();

    /* Apply the scale-factor if necessary: */
    if (scaleFactor() != 1.0)
        m_transform = m_transform.scale(scaleFactor(), scaleFactor());

    /* Apply the backing-scale-factor if necessary: */
    if (useUnscaledHiDPIOutput() && backingScaleFactor() > 1.0)
        m_transform = m_transform.scale(1.0 / backingScaleFactor(), 1.0 / backingScaleFactor());
}

void UIFrameBufferPrivate::paintDefault(QPaintEvent *pEvent)
{
    /* Scaled image is NULL by default: */
    QImage scaledImage;
    /* But if scaled-factor is set and current image is NOT null: */
    if (m_scaledSize.isValid() && !m_image.isNull())
    {
        /* We are doing a deep copy of the image to make sure it will not be
         * detached during scale process, otherwise we can get a frozen frame-buffer. */
        scaledImage = m_image.copy();
        /* And scaling the image to predefined scaled-factor: */
        switch (m_pMachineView->visualStateType())
        {
            case UIVisualStateType_Scale:
                scaledImage = scaledImage.scaled(m_scaledSize, Qt::IgnoreAspectRatio,
                                                 transformationMode(scalingOptimizationType()));
                break;
            default:
                scaledImage = scaledImage.scaled(m_scaledSize, Qt::IgnoreAspectRatio,
                                                 transformationMode(scalingOptimizationType(), m_dScaleFactor));
                break;
        }
    }
    /* Finally we are choosing image to paint from: */
    const QImage &sourceImage = scaledImage.isNull() ? m_image : scaledImage;

    /* Prepare the 'paint' rectangle: */
    QRect paintRect = pEvent->rect();

    /* Take the backing-scale-factor into account: */
    if (useUnscaledHiDPIOutput() && backingScaleFactor() > 1.0)
    {
        paintRect.moveTo(paintRect.topLeft() * backingScaleFactor());
        paintRect.setSize(paintRect.size() * backingScaleFactor());
    }

    /* Make sure paint-rectangle is within the image boundary: */
    paintRect = paintRect.intersected(sourceImage.rect());
    if (paintRect.isEmpty())
        return;

    /* Create painter: */
    QPainter painter(m_pMachineView->viewport());

#ifdef VBOX_WS_MAC
    /* On OSX for Qt5 we need to erase backing store first: */
    QRect eraseRect = paintRect;
    /* Take the backing-scale-factor into account: */
    if (useUnscaledHiDPIOutput() && backingScaleFactor() > 1.0)
    {
        eraseRect.moveTo(eraseRect.topLeft() / backingScaleFactor());
        eraseRect.setSize(eraseRect.size() / backingScaleFactor());
    }
    /* Replace translucent background with black one: */
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(eraseRect, QColor(Qt::black));
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
#endif /* VBOX_WS_MAC */

    /* Draw image rectangle: */
    drawImageRect(painter, sourceImage, paintRect,
                  m_pMachineView->contentsX(), m_pMachineView->contentsY(),
                  useUnscaledHiDPIOutput(), hiDPIOptimizationType(), backingScaleFactor());
}

void UIFrameBufferPrivate::paintSeamless(QPaintEvent *pEvent)
{
    /* Scaled image is NULL by default: */
    QImage scaledImage;
    /* But if scaled-factor is set and current image is NOT null: */
    if (m_scaledSize.isValid() && !m_image.isNull())
    {
        /* We are doing a deep copy of the image to make sure it will not be
         * detached during scale process, otherwise we can get a frozen frame-buffer. */
        scaledImage = m_image.copy();
        /* And scaling the image to predefined scaled-factor: */
        switch (m_pMachineView->visualStateType())
        {
            case UIVisualStateType_Scale:
                scaledImage = scaledImage.scaled(m_scaledSize, Qt::IgnoreAspectRatio,
                                                 transformationMode(scalingOptimizationType()));
                break;
            default:
                scaledImage = scaledImage.scaled(m_scaledSize, Qt::IgnoreAspectRatio,
                                                 transformationMode(scalingOptimizationType(), m_dScaleFactor));
                break;
        }
    }
    /* Finally we are choosing image to paint from: */
    const QImage &sourceImage = scaledImage.isNull() ? m_image : scaledImage;

    /* Prepare the 'paint' rectangle: */
    QRect paintRect = pEvent->rect();

    /* Prepare seamless regions to erase/paint: */
    lock();
    const QRegion eraseRegion = QRegion(paintRect) - m_syncVisibleRegion;
    const QRegion paintRegion = QRegion(paintRect) & m_syncVisibleRegion;
    unlock();

    /* Take the backing-scale-factor into account: */
    if (useUnscaledHiDPIOutput() && backingScaleFactor() > 1.0)
    {
        paintRect.moveTo(paintRect.topLeft() * backingScaleFactor());
        paintRect.setSize(paintRect.size() * backingScaleFactor());
    }

    /* Make sure paint-rectangle is within the image boundary: */
    paintRect = paintRect.intersected(sourceImage.rect());
    if (paintRect.isEmpty())
        return;

    /* Create painter: */
    QPainter painter(m_pMachineView->viewport());

    /* Apply painter clipping for erasing: */
    painter.setClipRegion(eraseRegion);
    /* Set composition-mode to erase: */
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    /* Erase rectangle: */
    eraseImageRect(painter, paintRect,
                   useUnscaledHiDPIOutput(), hiDPIOptimizationType(), backingScaleFactor());

    /* Apply painter clipping for painting: */
    painter.setClipRegion(paintRegion);
    /* Set composition-mode to paint: */
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

#if defined(VBOX_WITH_TRANSLUCENT_SEAMLESS)
    /* On OSX for Qt5 we need to erase backing store first: */
    QRect eraseRect = paintRect;
    /* Take the backing-scale-factor into account: */
    if (useUnscaledHiDPIOutput() && backingScaleFactor() > 1.0)
    {
        eraseRect.moveTo(eraseRect.topLeft() / backingScaleFactor());
        eraseRect.setSize(eraseRect.size() / backingScaleFactor());
    }
    /* Replace translucent background with black one: */
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(eraseRect, QColor(Qt::black));
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
#endif /* VBOX_WITH_TRANSLUCENT_SEAMLESS */

    /* Draw image rectangle: */
    drawImageRect(painter, sourceImage, paintRect,
                  m_pMachineView->contentsX(), m_pMachineView->contentsY(),
                  useUnscaledHiDPIOutput(), hiDPIOptimizationType(), backingScaleFactor());
}

/* static */
Qt::TransformationMode UIFrameBufferPrivate::transformationMode(ScalingOptimizationType type, double dScaleFactor /* = 0 */)
{
    switch (type)
    {
        /* Check if optimization type is forced to be 'Performance': */
        case ScalingOptimizationType_Performance: return Qt::FastTransformation;
        default: break;
    }
    /* For integer-scaling we are choosing the 'Performance' optimization type ourselves: */
    return dScaleFactor && floor(dScaleFactor) == dScaleFactor ? Qt::FastTransformation : Qt::SmoothTransformation;;
}

/* static */
void UIFrameBufferPrivate::eraseImageRect(QPainter &painter, const QRect &rect,
                                          bool fUseUnscaledHiDPIOutput,
                                          HiDPIOptimizationType hiDPIOptimizationType,
                                          double dBackingScaleFactor)
{
    /* Prepare sub-pixmap: */
    QPixmap subPixmap = QPixmap(rect.width(), rect.height());

    /* If HiDPI 'backing-scale-factor' defined: */
    if (dBackingScaleFactor > 1.0)
    {
        /* Should we
         * perform logical HiDPI scaling and optimize it for performance? */
        if (!fUseUnscaledHiDPIOutput && hiDPIOptimizationType == HiDPIOptimizationType_Performance)
        {
            /* Adjust sub-pixmap: */
            subPixmap = QPixmap((int)(rect.width() * dBackingScaleFactor),
                                (int)(rect.height() * dBackingScaleFactor));
        }

#ifdef VBOX_WS_MAC
# ifdef VBOX_GUI_WITH_HIDPI
        /* Should we
         * do not perform logical HiDPI scaling or
         * perform logical HiDPI scaling and optimize it for performance? */
        if (fUseUnscaledHiDPIOutput || hiDPIOptimizationType == HiDPIOptimizationType_Performance)
        {
            /* Mark sub-pixmap as HiDPI: */
            subPixmap.setDevicePixelRatio(dBackingScaleFactor);
        }
# endif /* VBOX_GUI_WITH_HIDPI */
#endif /* VBOX_WS_MAC */
    }

    /* Which point we should draw corresponding sub-pixmap? */
    QPointF paintPoint = rect.topLeft();

    /* Take the backing-scale-factor into account: */
    if (fUseUnscaledHiDPIOutput && dBackingScaleFactor > 1.0)
        paintPoint /= dBackingScaleFactor;

    /* Draw sub-pixmap: */
    painter.drawPixmap(paintPoint, subPixmap);
}

/* static */
void UIFrameBufferPrivate::drawImageRect(QPainter &painter, const QImage &image, const QRect &rect,
                                         int iContentsShiftX, int iContentsShiftY,
                                         bool fUseUnscaledHiDPIOutput,
                                         HiDPIOptimizationType hiDPIOptimizationType,
                                         double dBackingScaleFactor)
{
    /* Calculate offset: */
    size_t offset = (rect.x() + iContentsShiftX) * image.depth() / 8 +
                    (rect.y() + iContentsShiftY) * image.bytesPerLine();

    /* Restrain boundaries: */
    int iSubImageWidth = qMin(rect.width(), image.width() - rect.x() - iContentsShiftX);
    int iSubImageHeight = qMin(rect.height(), image.height() - rect.y() - iContentsShiftY);

    /* Create sub-image (no copy involved): */
    QImage subImage = QImage(image.bits() + offset,
                             iSubImageWidth, iSubImageHeight,
                             image.bytesPerLine(), image.format());

    /* Create sub-pixmap on the basis of sub-image above (1st copy involved): */
    QPixmap subPixmap = QPixmap::fromImage(subImage);

    /* If HiDPI 'backing-scale-factor' defined: */
    if (dBackingScaleFactor > 1.0)
    {
        /* Should we
         * perform logical HiDPI scaling and optimize it for performance? */
        if (!fUseUnscaledHiDPIOutput && hiDPIOptimizationType == HiDPIOptimizationType_Performance)
        {
            /* Fast scale sub-pixmap (2nd copy involved): */
            subPixmap = subPixmap.scaled(subPixmap.size() * dBackingScaleFactor,
                                         Qt::IgnoreAspectRatio, Qt::FastTransformation);
        }

#ifdef VBOX_WS_MAC
# ifdef VBOX_GUI_WITH_HIDPI
        /* Should we
         * do not perform logical HiDPI scaling or
         * perform logical HiDPI scaling and optimize it for performance? */
        if (fUseUnscaledHiDPIOutput || hiDPIOptimizationType == HiDPIOptimizationType_Performance)
        {
            /* Mark sub-pixmap as HiDPI: */
            subPixmap.setDevicePixelRatio(dBackingScaleFactor);
        }
# endif /* VBOX_GUI_WITH_HIDPI */
#endif /* VBOX_WS_MAC */
    }

    /* Which point we should draw corresponding sub-pixmap? */
    QPointF paintPoint = rect.topLeft();

    /* Take the backing-scale-factor into account: */
    if (fUseUnscaledHiDPIOutput && dBackingScaleFactor > 1.0)
        paintPoint /= dBackingScaleFactor;

    /* Draw sub-pixmap: */
    painter.drawPixmap(paintPoint, subPixmap);
}


#ifdef VBOX_WITH_VIDEOHWACCEL
UIFrameBuffer::UIFrameBuffer(bool m_fAccelerate2DVideo)
{
    if (m_fAccelerate2DVideo)
    {
        ComObjPtr<VBoxOverlayFrameBuffer> pFrameBuffer;
        pFrameBuffer.createObject();
        m_pFrameBuffer = pFrameBuffer;
    }
    else
    {
        m_pFrameBuffer.createObject();
    }
}
#else /* !VBOX_WITH_VIDEOHWACCEL */
UIFrameBuffer::UIFrameBuffer()
{
    m_pFrameBuffer.createObject();
}
#endif /* !VBOX_WITH_VIDEOHWACCEL */

UIFrameBuffer::~UIFrameBuffer()
{
    m_pFrameBuffer.setNull();
}

HRESULT UIFrameBuffer::init(UIMachineView *pMachineView)
{
    return m_pFrameBuffer->init(pMachineView);
}

void UIFrameBuffer::attach()
{
    m_pFrameBuffer->attach();
}

void UIFrameBuffer::detach()
{
    m_pFrameBuffer->detach();
}

uchar* UIFrameBuffer::address()
{
    return m_pFrameBuffer->address();
}

ulong UIFrameBuffer::width() const
{
    return m_pFrameBuffer->width();
}

ulong UIFrameBuffer::height() const
{
    return m_pFrameBuffer->height();
}

ulong UIFrameBuffer::bitsPerPixel() const
{
    return m_pFrameBuffer->bitsPerPixel();
}

ulong UIFrameBuffer::bytesPerLine() const
{
    return m_pFrameBuffer->bytesPerLine();
}

UIVisualStateType UIFrameBuffer::visualState() const
{
    return m_pFrameBuffer->visualState();
}

void UIFrameBuffer::setView(UIMachineView *pMachineView)
{
    m_pFrameBuffer->setView(pMachineView);
}

void UIFrameBuffer::setMarkAsUnused(bool fUnused)
{
    m_pFrameBuffer->setMarkAsUnused(fUnused);
}

bool UIFrameBuffer::isAutoEnabled() const
{
    return m_pFrameBuffer->isAutoEnabled();
}

void UIFrameBuffer::setAutoEnabled(bool fAutoEnabled)
{
    m_pFrameBuffer->setAutoEnabled(fAutoEnabled);
}

QSize UIFrameBuffer::scaledSize() const
{
    return m_pFrameBuffer->scaledSize();
}

void UIFrameBuffer::setScaledSize(const QSize &size /* = QSize() */)
{
    m_pFrameBuffer->setScaledSize(size);
}

int UIFrameBuffer::convertHostXTo(int iX) const
{
    return m_pFrameBuffer->convertHostXTo(iX);
}

int UIFrameBuffer::convertHostYTo(int iY) const
{
    return m_pFrameBuffer->convertHostXTo(iY);
}

double UIFrameBuffer::scaleFactor() const
{
    return m_pFrameBuffer->scaleFactor();
}

void UIFrameBuffer::setScaleFactor(double dScaleFactor)
{
    m_pFrameBuffer->setScaleFactor(dScaleFactor);
}

double UIFrameBuffer::backingScaleFactor() const
{
    return m_pFrameBuffer->backingScaleFactor();
}

void UIFrameBuffer::setBackingScaleFactor(double dBackingScaleFactor)
{
    m_pFrameBuffer->setBackingScaleFactor(dBackingScaleFactor);
}

bool UIFrameBuffer::useUnscaledHiDPIOutput() const
{
    return m_pFrameBuffer->useUnscaledHiDPIOutput();
}

void UIFrameBuffer::setUseUnscaledHiDPIOutput(bool fUseUnscaledHiDPIOutput)
{
    m_pFrameBuffer->setUseUnscaledHiDPIOutput(fUseUnscaledHiDPIOutput);
}

ScalingOptimizationType UIFrameBuffer::scalingOptimizationType() const
{
    return m_pFrameBuffer->scalingOptimizationType();
}

void UIFrameBuffer::setScalingOptimizationType(ScalingOptimizationType type)
{
    m_pFrameBuffer->setScalingOptimizationType(type);
}

HiDPIOptimizationType UIFrameBuffer::hiDPIOptimizationType() const
{
    return m_pFrameBuffer->hiDPIOptimizationType();
}

void UIFrameBuffer::setHiDPIOptimizationType(HiDPIOptimizationType type)
{
    m_pFrameBuffer->setHiDPIOptimizationType(type);
}

void UIFrameBuffer::handleNotifyChange(int iWidth, int iHeight)
{
    m_pFrameBuffer->handleNotifyChange(iWidth, iHeight);
}

void UIFrameBuffer::handlePaintEvent(QPaintEvent *pEvent)
{
    m_pFrameBuffer->handlePaintEvent(pEvent);
}

void UIFrameBuffer::handleSetVisibleRegion(const QRegion &region)
{
    m_pFrameBuffer->handleSetVisibleRegion(region);
}

void UIFrameBuffer::performResize(int iWidth, int iHeight)
{
    m_pFrameBuffer->performResize(iWidth, iHeight);
}

void UIFrameBuffer::performRescale()
{
    m_pFrameBuffer->performRescale();
}

#ifdef VBOX_WITH_VIDEOHWACCEL
void UIFrameBuffer::doProcessVHWACommand(QEvent *pEvent)
{
    m_pFrameBuffer->doProcessVHWACommand(pEvent);
}

void UIFrameBuffer::viewportResized(QResizeEvent *pEvent)
{
    m_pFrameBuffer->viewportResized(pEvent);
}

void UIFrameBuffer::viewportScrolled(int iX, int iY)
{
    m_pFrameBuffer->viewportScrolled(iX, iY);
}
#endif /* VBOX_WITH_VIDEOHWACCEL */

#include "UIFrameBuffer.moc"

