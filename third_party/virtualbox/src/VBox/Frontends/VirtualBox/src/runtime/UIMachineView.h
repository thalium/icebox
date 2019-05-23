/* $Id: UIMachineView.h $ */
/** @file
 * VBox Qt GUI - UIMachineView class declaration.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIMachineView_h___
#define ___UIMachineView_h___

/* Qt includes: */
#include <QAbstractScrollArea>
#include <QEventLoop>

/* GUI includes: */
#include "UIExtraDataDefs.h"
#include "UIMachineDefs.h"
#ifdef VBOX_WITH_DRAG_AND_DROP
# include "UIDnDHandler.h"
#endif /* VBOX_WITH_DRAG_AND_DROP */

/* COM includes: */
#include "COMEnums.h"

/* Other VBox includes: */
#include "VBox/com/ptr.h"
#ifdef VBOX_WS_MAC
#  include <ApplicationServices/ApplicationServices.h>
#endif /* VBOX_WS_MAC */

/* External includes: */
#ifdef VBOX_WS_MAC
# include <CoreFoundation/CFBase.h>
#endif /* VBOX_WS_MAC */

/* Forward declarations: */
class UIActionPool;
class UISession;
class UIMachineLogic;
class UIMachineWindow;
class UIFrameBuffer;
class UINativeEventFilter;
class CConsole;
class CDisplay;
class CGuest;
class CMachine;
class CSession;
#ifdef VBOX_WITH_DRAG_AND_DROP
class CDnDTarget;
#endif


class UIMachineView : public QAbstractScrollArea
{
    Q_OBJECT;

signals:

    /** Notifies about frame-buffer resize. */
    void sigFrameBufferResize();

public:

    /* Factory function to create machine-view: */
    static UIMachineView* create(  UIMachineWindow *pMachineWindow
                                 , ulong uScreenId
                                 , UIVisualStateType visualStateType
#ifdef VBOX_WITH_VIDEOHWACCEL
                                 , bool bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
    );
    /* Factory function to destroy required machine-view: */
    static void destroy(UIMachineView *pMachineView);

    /** Returns whether the guest-screen auto-resize is enabled. */
    virtual bool isGuestAutoresizeEnabled() const { return true; }
    /** Defines whether the guest-screen auto-resize is @a fEnabled. */
    virtual void setGuestAutoresizeEnabled(bool fEnabled) { Q_UNUSED(fEnabled); }

    /** Send saved guest-screen size-hint to the guest.
      * @note Reimplemented in sub-classes. Base implementation does nothing. */
    virtual void resendSizeHint() {}

    /** Adjusts guest-screen size to correspond current visual-style.
      * @note Reimplemented in sub-classes. Base implementation does nothing. */
    virtual void adjustGuestScreenSize() {}

    /** Applies machine-view scale-factor. */
    virtual void applyMachineViewScaleFactor();

    /* Framebuffer aspect ratio: */
    double aspectRatio() const;

    /** Updates console's display viewport.
      * @remarks Used to update 3D-service overlay viewport as well. */
    void updateViewport();

protected slots:

    /* Slot to perform guest resize: */
    void sltPerformGuestResize(const QSize &aSize = QSize());

    /* Handler: Frame-buffer NotifyChange stuff: */
    virtual void sltHandleNotifyChange(int iWidth, int iHeight);

    /* Handler: Frame-buffer NotifyUpdate stuff: */
    virtual void sltHandleNotifyUpdate(int iX, int iY, int iWidth, int iHeight);

    /* Handler: Frame-buffer SetVisibleRegion stuff: */
    virtual void sltHandleSetVisibleRegion(QRegion region);

    /* Handler: Frame-buffer 3D overlay visibility stuff: */
    virtual void sltHandle3DOverlayVisibilityChange(bool fVisible);

    /* Watch dog for desktop resizes: */
    void sltDesktopResized();

    /** Handles the scale-factor change. */
    void sltHandleScaleFactorChange(const QString &strMachineID);

    /** Handles the scaling-optimization change. */
    void sltHandleScalingOptimizationChange(const QString &strMachineID);

    /** Handles the HiDPI-optimization change. */
    void sltHandleHiDPIOptimizationChange(const QString &strMachineID);

    /** Handles the unscaled HiDPI output mode change. */
    void sltHandleUnscaledHiDPIOutputModeChange(const QString &strMachineID);

    /* Console callback handlers: */
    virtual void sltMachineStateChanged();

protected:

    /* Machine-view constructor: */
    UIMachineView(  UIMachineWindow *pMachineWindow
                  , ulong uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                  , bool bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
    );
    /* Machine-view destructor: */
    virtual ~UIMachineView() {}

    /* Prepare routines: */
    virtual void loadMachineViewSettings();
    //virtual void prepareNativeFilters() {}
    virtual void prepareViewport();
    virtual void prepareFrameBuffer();
    virtual void prepareCommon();
    virtual void prepareFilters();
    virtual void prepareConnections();
    virtual void prepareConsoleConnections();

    /* Cleanup routines: */
    //virtual void cleanupConsoleConnections() {}
    //virtual void cleanupConnections() {}
    //virtual void cleanupFilters() {}
    //virtual void cleanupCommon() {}
    virtual void cleanupFrameBuffer();
    //virtual void cleanupViewport();
    virtual void cleanupNativeFilters();
    //virtual void saveMachineViewSettings() {}

    /** Returns the session UI reference. */
    UISession* uisession() const;

    /** Returns the session reference. */
    CSession& session() const;
    /** Returns the session's machine reference. */
    CMachine& machine() const;
    /** Returns the session's console reference. */
    CConsole& console() const;
    /** Returns the console's display reference. */
    CDisplay& display() const;
    /** Returns the console's guest reference. */
    CGuest& guest() const;

    /* Protected getters: */
    UIMachineWindow* machineWindow() const { return m_pMachineWindow; }
    UIActionPool* actionPool() const;
    UIMachineLogic* machineLogic() const;
    QSize sizeHint() const;
    int contentsX() const;
    int contentsY() const;
    int contentsWidth() const;
    int contentsHeight() const;
    int visibleWidth() const;
    int visibleHeight() const;
    ulong screenId() const { return m_uScreenId; }
    UIFrameBuffer* frameBuffer() const { return m_pFrameBuffer; }
    /** Atomically store the maximum guest resolution which we currently wish
     * to handle for @a maxGuestSize() to read.  Should be called if anything
     * happens (e.g. a screen hotplug) which might cause the value to change.
     * @sa m_u64MaxGuestSize. */
    void setMaxGuestSize(const QSize &minimumSizeHint = QSize());
    /** Atomically read the maximum guest resolution which we currently wish to
     * handle.  This may safely be called from another thread (called by
     * UIFramebuffer on EMT).
     * @sa m_u64MaxGuestSize. */
    QSize maxGuestSize();

    /** Retrieves the last guest-screen visibility status from extra-data. */
    bool guestScreenVisibilityStatus() const;

    /** Retrieves the last guest-screen size-hint from extra-data. */
    QSize guestScreenSizeHint() const;
    /** Stores a guest-screen size-hint to extra-data. */
    void storeGuestSizeHint(const QSize &sizeHint);

    /** Handles machine-view scale changes. */
    void handleScaleChange();

    /** Returns the pause-pixmap: */
    const QPixmap& pausePixmap() const { return m_pausePixmap; }
    /** Returns the scaled pause-pixmap: */
    const QPixmap& pausePixmapScaled() const { return m_pausePixmapScaled; }
    /** Resets the pause-pixmap. */
    void resetPausePixmap();
    /** Acquires live pause-pixmap. */
    void takePausePixmapLive();
    /** Acquires snapshot pause-pixmap. */
    void takePausePixmapSnapshot();
    /** Updates the scaled pause-pixmap. */
    void updateScaledPausePixmap();

    /** The available area on the current screen for application windows. */
    virtual QRect workingArea() const = 0;
    /** Calculate how big the guest desktop can be while still fitting on one
     * host screen. */
    virtual QSize calculateMaxGuestSize() const = 0;
    virtual void updateSliders();
    QPoint viewportToContents(const QPoint &vp) const;
    void scrollBy(int dx, int dy);
    static void dimImage(QImage &img);
    void scrollContentsBy(int dx, int dy);
#ifdef VBOX_WS_MAC
    void updateDockIcon();
    CGImageRef vmContentImage();
    CGImageRef frameBuffertoCGImageRef(UIFrameBuffer *pFrameBuffer);
#endif /* VBOX_WS_MAC */
    /** What view mode (normal, fullscreen etc.) are we in? */
    UIVisualStateType visualStateType() const;
    /** Is this a fullscreen-type view? */
    bool isFullscreenOrSeamless() const;

    /* Cross-platforms event processors: */
    bool event(QEvent *pEvent);
    bool eventFilter(QObject *pWatched, QEvent *pEvent);
    void resizeEvent(QResizeEvent *pEvent);
    void moveEvent(QMoveEvent *pEvent);
    void paintEvent(QPaintEvent *pEvent);

    /** Handles focus-in @a pEvent. */
    void focusInEvent(QFocusEvent *pEvent);
    /** Handles focus-out @a pEvent. */
    void focusOutEvent(QFocusEvent *pEvent);

#ifdef VBOX_WITH_DRAG_AND_DROP
    /**
     * Returns @true if the VM window can accept (start is, start) a drag and drop
     * operation, @false if not.
     */
    bool dragAndDropCanAccept(void) const;

    /**
     * Returns @true if drag and drop for this machine is active
     * (that is, host->guest, guest->host or bidirectional), @false if not.
     */
    bool dragAndDropIsActive(void) const;

    /**
     * Host -> Guest: Issued when the host cursor enters the guest (VM) window.
     *                The guest will receive the relative cursor coordinates of the
     *                appropriate screen ID.
     *
     * @param pEvent                Related enter event.
     */
    void dragEnterEvent(QDragEnterEvent *pEvent);

    /**
     * Host -> Guest: Issued when the host cursor moves inside (over) the guest (VM) window.
     *                The guest will receive the relative cursor coordinates of the
     *                appropriate screen ID.
     *
     * @param pEvent                Related move event.
     */
    void dragLeaveEvent(QDragLeaveEvent *pEvent);

    /**
     * Host -> Guest: Issued when the host cursor leaves the guest (VM) window again.
     *                This will ask the guest to stop any further drag'n drop operation.
     *
     * @param pEvent                Related leave event.
     */
    void dragMoveEvent(QDragMoveEvent *pEvent);

    /**
     * Guest -> Host: Checks for a pending drag and drop event within the guest
     *                and (optionally) starts a drag and drop operation on the host.
     */
    int dragCheckPending(void);

    /**
     * Guest -> Host: Starts a drag and drop operation from guest to the host. This
     *                internally either uses Qt's abstract QDrag methods or some other
     *                OS-dependent implementation.
     */
    int dragStart(void);

    /**
     * Guest -> Host: Aborts (and resets) the current (pending) guest to host
     *                drag and drop operation.
     */
    int dragStop(void);

    /**
     * Host -> Guest: Issued when the host drops data into the guest (VM) window.
     *
     * @param pEvent                Related drop event.
     */
    void dropEvent(QDropEvent *pEvent);
#endif /* VBOX_WITH_DRAG_AND_DROP */

    /** Qt5: Performs pre-processing of all the native events. */
    virtual bool nativeEventPreprocessor(const QByteArray &eventType, void *pMessage);

    /** Scales passed size forward. */
    QSize scaledForward(QSize size) const;
    /** Scales passed size backward. */
    QSize scaledBackward(QSize size) const;

    /* Protected members: */
    UIMachineWindow *m_pMachineWindow;
    ulong m_uScreenId;
    UIFrameBuffer *m_pFrameBuffer;
    KMachineState m_previousState;
    /** HACK: when switching out of fullscreen or seamless we wish to override
     * the default size hint to avoid short resizes back to fullscreen size.
     * Not explicitly initialised (i.e. invalid by default). */
    QSize m_sizeHintOverride;

#ifdef VBOX_WS_MAC
    /** Holds current host-screen number. */
    int m_iHostScreenNumber;
#endif /* VBOX_WS_MAC */

    /** The policy for calculating the maximum guest resolution which we wish
     * to handle. */
    MaxGuestResolutionPolicy m_maxGuestSizePolicy;
    /** The maximum guest size for fixed size policy. */
    QSize m_fixedMaxGuestSize;
    /** Maximum guest resolution which we wish to handle.  Must be accessed
     * atomically.
     * @note The background for this variable is that we need this value to be
     * available to the EMT thread, but it can only be calculated by the
     * GUI, and GUI code can only safely be called on the GUI thread due to
     * (at least) X11 threading issues.  So we calculate the value in advance,
     * monitor things in case it changes and update it atomically when it does.
     */
    /** @todo This should be private. */
    volatile uint64_t m_u64MaxGuestSize;

#ifdef VBOX_WITH_VIDEOHWACCEL
    bool m_fAccelerate2DVideo : 1;
#endif /* VBOX_WITH_VIDEOHWACCEL */

    /** Holds the pause-pixmap. */
    QPixmap m_pausePixmap;
    /** Holds the scaled pause-pixmap. */
    QPixmap m_pausePixmapScaled;

#ifdef VBOX_WITH_DRAG_AND_DROP
    /** Pointer to drag and drop handler instance. */
    UIDnDHandler *m_pDnDHandler;
# ifdef VBOX_WITH_DRAG_AND_DROP_GH
    /** Flag indicating whether a guest->host drag currently is in
     *  progress or not. */
    bool m_fIsDraggingFromGuest;
# endif
#endif

    /** Holds the native event filter instance. */
    UINativeEventFilter *m_pNativeEventFilter;
    /** Allows the native event filter to redirect
      * events directly to nativeEventPreprocessor(). */
    friend class UINativeEventFilter;

    /* Friend classes: */
    friend class UIKeyboardHandler;
    friend class UIMouseHandler;
    friend class UIMachineLogic;
    friend class UIFrameBuffer;
    friend class UIFrameBufferPrivate;
    friend class VBoxOverlayFrameBuffer;
};

/* This maintenance class is a part of future roll-back mechanism.
 * It allows to block main GUI thread until specific event received.
 * Later it will become more abstract but now its just used to help
 * fullscreen & seamless modes to restore normal guest size hint. */
/** @todo This class is now unused - can it be removed altogether? */
class UIMachineViewBlocker : public QEventLoop
{
    Q_OBJECT;

public:

    UIMachineViewBlocker()
        : QEventLoop(0)
        , m_iTimerId(0)
    {
        /* Also start timer to unlock pool in case of
         * required condition doesn't happens by some reason: */
        m_iTimerId = startTimer(3000);
    }

    virtual ~UIMachineViewBlocker()
    {
        /* Kill the timer: */
        killTimer(m_iTimerId);
    }

protected:

    void timerEvent(QTimerEvent *pEvent)
    {
        /* If that timer event occurs => it seems
         * guest resize event doesn't comes in time,
         * shame on it, but we just unlocking 'this': */
        QEventLoop::timerEvent(pEvent);
        exit();
    }

    int m_iTimerId;
};

#endif // !___UIMachineView_h___
