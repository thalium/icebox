/* $Id: UIMachineView.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineView class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QMainWindow>
# include <QPainter>
# include <QScrollBar>
# include <QTimer>

/* GUI includes: */
# include "VBoxGlobal.h"
# include "UIDesktopWidgetWatchdog.h"
# include "UIExtraDataManager.h"
# include "UIMessageCenter.h"
# include "UISession.h"
# include "UIMachineLogic.h"
# include "UIMachineWindow.h"
# include "UIMachineViewNormal.h"
# include "UIMachineViewFullscreen.h"
# include "UIMachineViewSeamless.h"
# include "UIMachineViewScale.h"
# include "UIKeyboardHandler.h"
# include "UIMouseHandler.h"
# include "UIFrameBuffer.h"
# include "VBoxFBOverlay.h"
# ifdef VBOX_WS_MAC
#  include "UICocoaApplication.h"
# endif /* VBOX_WS_MAC */
# ifdef VBOX_WITH_DRAG_AND_DROP
#  include "UIDnDHandler.h"
# endif /* VBOX_WITH_DRAG_AND_DROP */

/* VirtualBox interface declarations: */
# ifndef VBOX_WITH_XPCOM
#  include "VirtualBox.h"
# else /* VBOX_WITH_XPCOM */
#  include "VirtualBox_XPCOM.h"
# endif /* VBOX_WITH_XPCOM */

/* COM includes: */
# include "CConsole.h"
# include "CDisplay.h"
# include "CSession.h"
# ifdef VBOX_WITH_DRAG_AND_DROP
#  include "CDnDSource.h"
#  include "CDnDTarget.h"
#  include "CGuest.h"
# endif /* VBOX_WITH_DRAG_AND_DROP */

/* Other VBox includes: */
# include <iprt/asm.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QAbstractNativeEventFilter>

/* GUI includes: */
#ifdef VBOX_WS_MAC
# include "DarwinKeyboard.h"
# include "DockIconPreview.h"
#endif /* VBOX_WS_MAC */

/* COM includes: */
#include "CFramebuffer.h"
#ifdef VBOX_WITH_DRAG_AND_DROP
# include "CGuestDnDSource.h"
# include "CGuestDnDTarget.h"
#endif /* VBOX_WITH_DRAG_AND_DROP */

/* Other VBox includes: */
#include <VBox/VBoxOGL.h>
#include <VBoxVideo.h>
#ifdef VBOX_WS_MAC
# include <VBox/err.h>
#endif /* VBOX_WS_MAC */

/* External includes: */
#include <math.h>
#ifdef VBOX_WS_MAC
# include <Carbon/Carbon.h>
#endif
#ifdef VBOX_WS_X11
#  include <xcb/xcb.h>
#endif

#ifdef DEBUG_andy
/* Macro for debugging drag and drop actions which usually would
 * go to Main's logging group. */
# define DNDDEBUG(x) LogFlowFunc(x)
#else
# define DNDDEBUG(x)
#endif


/** QAbstractNativeEventFilter extension
  * allowing to pre-process native platform events. */
class UINativeEventFilter : public QAbstractNativeEventFilter
{
public:

    /** Constructs native event filter storing @a pParent to redirect events to. */
    UINativeEventFilter(UIMachineView *pParent)
        : m_pParent(pParent)
    {}

    /** Redirects all the native events to parent. */
    bool nativeEventFilter(const QByteArray &eventType, void *pMessage, long * /* pResult */)
    {
        return m_pParent->nativeEventPreprocessor(eventType, pMessage);
    }

private:

    /** Holds the passed parent reference. */
    UIMachineView *m_pParent;
};


/* static */
UIMachineView* UIMachineView::create(  UIMachineWindow *pMachineWindow
                                     , ulong uScreenId
                                     , UIVisualStateType visualStateType
#ifdef VBOX_WITH_VIDEOHWACCEL
                                     , bool bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
                                     )
{
    UIMachineView *pMachineView = 0;
    switch (visualStateType)
    {
        case UIVisualStateType_Normal:
            pMachineView = new UIMachineViewNormal(  pMachineWindow
                                                   , uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                                                   , bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
                                                   );
            break;
        case UIVisualStateType_Fullscreen:
            pMachineView = new UIMachineViewFullscreen(  pMachineWindow
                                                       , uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                                                       , bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
                                                       );
            break;
        case UIVisualStateType_Seamless:
            pMachineView = new UIMachineViewSeamless(  pMachineWindow
                                                     , uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                                                     , bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
                                                     );
            break;
        case UIVisualStateType_Scale:
            pMachineView = new UIMachineViewScale(  pMachineWindow
                                                  , uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                                                  , bAccelerate2DVideo
#endif
                                                  );
            break;
        default:
            break;
    }

    /* Load machine-view settings: */
    pMachineView->loadMachineViewSettings();

    /* Prepare viewport: */
    pMachineView->prepareViewport();

    /* Prepare frame-buffer: */
    pMachineView->prepareFrameBuffer();

    /* Prepare common things: */
    pMachineView->prepareCommon();

    /* Prepare event-filters: */
    pMachineView->prepareFilters();

    /* Prepare connections: */
    pMachineView->prepareConnections();

    /* Prepare console connections: */
    pMachineView->prepareConsoleConnections();

    /* Initialization: */
    pMachineView->sltMachineStateChanged();
    /** @todo Can we move the call to sltAdditionsStateChanged() from the
     *        subclass constructors here too?  It is called for Normal and Seamless,
     *        but not for Fullscreen and Scale.  However for Scale it is a no op.,
     *        so it would not hurt.  Would it hurt for fullscreen? */

    /* Set a preliminary maximum size: */
    pMachineView->setMaxGuestSize();

    /* Resend the last resize hint finally: */
    pMachineView->resendSizeHint();

    /* Return the created view: */
    return pMachineView;
}

/* static */
void UIMachineView::destroy(UIMachineView *pMachineView)
{
    if (!pMachineView)
        return;

    /* Cleanup frame-buffer: */
    pMachineView->cleanupFrameBuffer();

#ifdef VBOX_WITH_DRAG_AND_DROP
    if (pMachineView->m_pDnDHandler)
    {
        delete pMachineView->m_pDnDHandler;
        pMachineView->m_pDnDHandler = NULL;
    }
#endif

    /* Cleanup native filters: */
    pMachineView->cleanupNativeFilters();

    delete pMachineView;
}

void UIMachineView::applyMachineViewScaleFactor()
{
    /* Take the scale-factor related attributes into account: */
    const double dScaleFactor = gEDataManager->scaleFactor(vboxGlobal().managedVMUuid());
    const bool fUseUnscaledHiDPIOutput = gEDataManager->useUnscaledHiDPIOutput(vboxGlobal().managedVMUuid());
    frameBuffer()->setScaleFactor(dScaleFactor);
    frameBuffer()->setUseUnscaledHiDPIOutput(fUseUnscaledHiDPIOutput);
    /* Propagate the scale-factor related attributes to 3D service if necessary: */
    if (machine().GetAccelerate3DEnabled() && vboxGlobal().is3DAvailable())
    {
        display().NotifyScaleFactorChange(m_uScreenId,
                                          (uint32_t)(dScaleFactor * VBOX_OGL_SCALE_FACTOR_MULTIPLIER),
                                          (uint32_t)(dScaleFactor * VBOX_OGL_SCALE_FACTOR_MULTIPLIER));
        display().NotifyHiDPIOutputPolicyChange(fUseUnscaledHiDPIOutput);
    }

    /* Perform frame-buffer rescaling: */
    frameBuffer()->performRescale();

    /* Update console's display viewport and 3D overlay: */
    updateViewport();
}

double UIMachineView::aspectRatio() const
{
    return frameBuffer() ? (double)(frameBuffer()->width()) / frameBuffer()->height() : 0;
}

void UIMachineView::updateViewport()
{
    display().ViewportChanged(screenId(), contentsX(), contentsY(), visibleWidth(), visibleHeight());
}

void UIMachineView::sltPerformGuestResize(const QSize &toSize)
{
    /* If this slot is invoked directly then use the passed size otherwise get
     * the available size for the guest display. We assume here that centralWidget()
     * contains this view only and gives it all available space: */
    QSize size(toSize.isValid() ? toSize : machineWindow()->centralWidget()->size());
    AssertMsg(size.isValid(), ("Size should be valid!\n"));

    /* Take the scale-factor(s) into account: */
    size = scaledBackward(size);

    /* Expand current limitations: */
    setMaxGuestSize(size);

    /* Send new size-hint to the guest: */
    LogRel(("GUI: UIMachineView::sltPerformGuestResize: "
            "Sending guest size-hint to screen %d as %dx%d if necessary\n",
            (int)screenId(), size.width(), size.height()));

    /* If auto-mount of guest-screens (auto-pilot) enabled: */
    if (gEDataManager->autoMountGuestScreensEnabled(vboxGlobal().managedVMUuid()))
    {
        /* Do not send a hint if nothing has changed to prevent the guest being notified about its own changes: */
        if (   (int)m_pFrameBuffer->width() != size.width() || (int)m_pFrameBuffer->height() != size.height()
            || uisession()->isScreenVisible(screenId()) != uisession()->isScreenVisibleHostDesires(screenId()))
        {
            /* If host and guest have same opinion about guest-screen visibility: */
            if (uisession()->isScreenVisible(screenId()) == uisession()->isScreenVisibleHostDesires(screenId()))
                display().SetVideoModeHint(screenId(),
                                           uisession()->isScreenVisible(screenId()),
                                           false, 0, 0, size.width(), size.height(), 0);
            /* If host desires to have guest-screen disabled and guest-screen is enabled, retrying: */
            else if (!uisession()->isScreenVisibleHostDesires(screenId()))
                display().SetVideoModeHint(screenId(), false, false, 0, 0, 0, 0, 0);
            /* If host desires to have guest-screen enabled and guest-screen is disabled, retrying: */
            else if (uisession()->isScreenVisibleHostDesires(screenId()))
                display().SetVideoModeHint(screenId(), true, false, 0, 0, size.width(), size.height(), 0);
        }
    }
    /* If auto-mount of guest-screens (auto-pilot) disabled: */
    else
    {
        /* Do not send a hint if nothing has changed to prevent the guest being notified about its own changes: */
        if ((int)m_pFrameBuffer->width() != size.width() || (int)m_pFrameBuffer->height() != size.height())
            display().SetVideoModeHint(screenId(),
                                       uisession()->isScreenVisible(screenId()),
                                       false, 0, 0, size.width(), size.height(), 0);
    }
}

void UIMachineView::sltHandleNotifyChange(int iWidth, int iHeight)
{
    LogRel(("GUI: UIMachineView::sltHandleNotifyChange: Screen=%d, Size=%dx%d\n",
            (unsigned long)m_uScreenId, iWidth, iHeight));

    /* Some situations require frame-buffer resize-events to be ignored at all,
     * leaving machine-window, machine-view and frame-buffer sizes preserved: */
    if (uisession()->isGuestResizeIgnored())
        return;

    /* In some situations especially in some VM states, guest-screen is not drawable: */
    if (uisession()->isGuestScreenUnDrawable())
        return;

    /* Get old frame-buffer size: */
    const QSize frameBufferSizeOld = QSize(frameBuffer()->width(),
                                           frameBuffer()->height());

    /* Perform frame-buffer mode-change: */
    frameBuffer()->handleNotifyChange(iWidth, iHeight);

    /* Get new frame-buffer size: */
    const QSize frameBufferSizeNew = QSize(frameBuffer()->width(),
                                           frameBuffer()->height());

    /* For 'scale' mode: */
    if (visualStateType() == UIVisualStateType_Scale)
    {
        /* Assign new frame-buffer logical-size: */
        frameBuffer()->setScaledSize(size());

        /* Forget the last full-screen size: */
        uisession()->setLastFullScreenSize(screenId(), QSize(-1, -1));
    }
    /* For other than 'scale' mode: */
    else
    {
        /* Adjust maximum-size restriction for machine-view: */
        setMaximumSize(sizeHint());

        /* Disable the resize hint override hack and forget the last full-screen size: */
        m_sizeHintOverride = QSize(-1, -1);
        if (visualStateType() == UIVisualStateType_Normal)
            uisession()->setLastFullScreenSize(screenId(), QSize(-1, -1));

        /* Force machine-window update own layout: */
        QCoreApplication::sendPostedEvents(0, QEvent::LayoutRequest);

        /* Update machine-view sliders: */
        updateSliders();

        /* By some reason Win host forgets to update machine-window central-widget
         * after main-layout was updated, let's do it for all the hosts: */
        machineWindow()->centralWidget()->update();

        /* Normalize 'normal' machine-window geometry if necessary: */
        if (visualStateType() == UIVisualStateType_Normal &&
            frameBufferSizeNew != frameBufferSizeOld)
            machineWindow()->normalizeGeometry(true /* adjust position */);
    }

    /* Perform frame-buffer rescaling: */
    frameBuffer()->performRescale();

#ifdef VBOX_WS_MAC
    /* Update MacOS X dock icon size: */
    machineLogic()->updateDockIconSize(screenId(), iWidth, iHeight);
#endif /* VBOX_WS_MAC */

    /* Notify frame-buffer resize: */
    emit sigFrameBufferResize();

    /* Ask for just required guest display update (it will also update
     * the viewport through IFramebuffer::NotifyUpdate): */
    display().InvalidateAndUpdateScreen(m_uScreenId);

    /* If we are in normal or scaled mode and if GA are active,
     * remember the guest-screen size to be able to restore it when necessary: */
    if (!isFullscreenOrSeamless() && uisession()->isGuestSupportsGraphics())
        storeGuestSizeHint(QSize(iWidth, iHeight));

    LogRelFlow(("GUI: UIMachineView::sltHandleNotifyChange: Complete for Screen=%d, Size=%dx%d\n",
                (unsigned long)m_uScreenId, iWidth, iHeight));
}

void UIMachineView::sltHandleNotifyUpdate(int iX, int iY, int iWidth, int iHeight)
{
    /* Prepare corresponding viewport part: */
    QRect rect(iX, iY, iWidth, iHeight);

    /* Take the scaling into account: */
    const double dScaleFactor = frameBuffer()->scaleFactor();
    const QSize scaledSize = frameBuffer()->scaledSize();
    if (scaledSize.isValid())
    {
        /* Calculate corresponding scale-factors: */
        const double xScaleFactor = visualStateType() == UIVisualStateType_Scale ?
                                    (double)scaledSize.width()  / frameBuffer()->width()  : dScaleFactor;
        const double yScaleFactor = visualStateType() == UIVisualStateType_Scale ?
                                    (double)scaledSize.height() / frameBuffer()->height() : dScaleFactor;
        /* Adjust corresponding viewport part: */
        rect.moveTo((int)floor((double)rect.x() * xScaleFactor) - 1,
                    (int)floor((double)rect.y() * yScaleFactor) - 1);
        rect.setSize(QSize((int)ceil((double)rect.width()  * xScaleFactor) + 2,
                           (int)ceil((double)rect.height() * yScaleFactor) + 2));
    }

    /* Shift has to be scaled by the backing-scale-factor
     * but not scaled by the scale-factor. */
    rect.translate(-contentsX(), -contentsY());

#ifdef VBOX_WS_MAC
    /* Take the backing-scale-factor into account: */
    if (frameBuffer()->useUnscaledHiDPIOutput())
    {
        const double dBackingScaleFactor = frameBuffer()->backingScaleFactor();
        if (dBackingScaleFactor > 1.0)
        {
            rect.moveTo((int)floor((double)rect.x() / dBackingScaleFactor) - 1,
                        (int)floor((double)rect.y() / dBackingScaleFactor) - 1);
            rect.setSize(QSize((int)ceil((double)rect.width()  / dBackingScaleFactor) + 2,
                               (int)ceil((double)rect.height() / dBackingScaleFactor) + 2));
        }
    }
#endif /* VBOX_WS_MAC */

    /* Limit the resulting part by the viewport rectangle: */
    rect &= viewport()->rect();

    /* Update corresponding viewport part: */
    viewport()->update(rect);
}

void UIMachineView::sltHandleSetVisibleRegion(QRegion region)
{
    /* Used only in seamless-mode. */
    Q_UNUSED(region);
}

void UIMachineView::sltHandle3DOverlayVisibilityChange(bool fVisible)
{
    machineLogic()->notifyAbout3DOverlayVisibilityChange(fVisible);
}

void UIMachineView::sltDesktopResized()
{
    setMaxGuestSize();
}

void UIMachineView::sltHandleScaleFactorChange(const QString &strMachineID)
{
    /* Skip unrelated machine IDs: */
    if (strMachineID != vboxGlobal().managedVMUuid())
        return;

    /* Take the scale-factor into account: */
    const double dScaleFactor = gEDataManager->scaleFactor(vboxGlobal().managedVMUuid());
    frameBuffer()->setScaleFactor(dScaleFactor);
    /* Propagate the scale-factor to 3D service if necessary: */
    if (machine().GetAccelerate3DEnabled() && vboxGlobal().is3DAvailable())
    {
        display().NotifyScaleFactorChange(m_uScreenId,
                                          (uint32_t)(dScaleFactor * VBOX_OGL_SCALE_FACTOR_MULTIPLIER),
                                          (uint32_t)(dScaleFactor * VBOX_OGL_SCALE_FACTOR_MULTIPLIER));
    }

    /* Handle scale attributes change: */
    handleScaleChange();
    /* Adjust guest-screen size: */
    adjustGuestScreenSize();

    /* Update scaled pause pixmap, if necessary: */
    updateScaledPausePixmap();
    viewport()->update();

    /* Update console's display viewport and 3D overlay: */
    updateViewport();
}

void UIMachineView::sltHandleScalingOptimizationChange(const QString &strMachineID)
{
    /* Skip unrelated machine IDs: */
    if (strMachineID != vboxGlobal().managedVMUuid())
        return;

    /* Take the scaling-optimization type into account: */
    frameBuffer()->setScalingOptimizationType(gEDataManager->scalingOptimizationType(vboxGlobal().managedVMUuid()));

    /* Update viewport: */
    viewport()->update();
}

void UIMachineView::sltHandleHiDPIOptimizationChange(const QString &strMachineID)
{
    /* Skip unrelated machine IDs: */
    if (strMachineID != vboxGlobal().managedVMUuid())
        return;

    /* Take the HiDPI-optimization type into account: */
    frameBuffer()->setHiDPIOptimizationType(gEDataManager->hiDPIOptimizationType(vboxGlobal().managedVMUuid()));

    /* Update viewport: */
    viewport()->update();
}

void UIMachineView::sltHandleUnscaledHiDPIOutputModeChange(const QString &strMachineID)
{
    /* Skip unrelated machine IDs: */
    if (strMachineID != vboxGlobal().managedVMUuid())
        return;

    /* Take the unscaled HiDPI output mode into account: */
    const bool fUseUnscaledHiDPIOutput = gEDataManager->useUnscaledHiDPIOutput(vboxGlobal().managedVMUuid());
    frameBuffer()->setUseUnscaledHiDPIOutput(fUseUnscaledHiDPIOutput);
    /* Propagate the unscaled HiDPI output mode to 3D service if necessary: */
    if (machine().GetAccelerate3DEnabled() && vboxGlobal().is3DAvailable())
    {
        display().NotifyHiDPIOutputPolicyChange(fUseUnscaledHiDPIOutput);
    }

    /* Handle scale attributes change: */
    handleScaleChange();
    /* Adjust guest-screen size: */
    adjustGuestScreenSize();

    /* Update scaled pause pixmap, if necessary: */
    updateScaledPausePixmap();
    viewport()->update();

    /* Update console's display viewport and 3D overlay: */
    updateViewport();
}

void UIMachineView::sltMachineStateChanged()
{
    /* Get machine state: */
    KMachineState state = uisession()->machineState();
    switch (state)
    {
        case KMachineState_Paused:
        case KMachineState_TeleportingPausedVM:
        {
            if (   m_pFrameBuffer
                && (   state           != KMachineState_TeleportingPausedVM
                    || m_previousState != KMachineState_Teleporting))
            {
                /* Take live pause-pixmap: */
                takePausePixmapLive();
                /* Fully repaint to pick up pause-pixmap: */
                viewport()->update();
            }
            break;
        }
        case KMachineState_Restoring:
        {
            /* Only works with the primary screen currently. */
            if (screenId() == 0)
            {
                /* Take snapshot pause-pixmap: */
                takePausePixmapSnapshot();
                /* Fully repaint to pick up pause-pixmap: */
                viewport()->update();
            }
            break;
        }
        case KMachineState_Running:
        {
            if (m_previousState == KMachineState_Paused ||
                m_previousState == KMachineState_TeleportingPausedVM ||
                m_previousState == KMachineState_Restoring)
            {
                if (m_pFrameBuffer)
                {
                    /* Reset pause-pixmap: */
                    resetPausePixmap();
                    /* Ask for full guest display update (it will also update
                     * the viewport through IFramebuffer::NotifyUpdate): */
                    display().InvalidateAndUpdate();
                }
            }
            /* Reapply machine-view scale-factor if necessary: */
            if (m_pFrameBuffer)
                applyMachineViewScaleFactor();
            break;
        }
        default:
            break;
    }

    m_previousState = state;
}

UIMachineView::UIMachineView(  UIMachineWindow *pMachineWindow
                             , ulong uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                             , bool bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
                             )
    : QAbstractScrollArea(pMachineWindow->centralWidget())
    , m_pMachineWindow(pMachineWindow)
    , m_uScreenId(uScreenId)
    , m_pFrameBuffer(0)
    , m_previousState(KMachineState_Null)
#ifdef VBOX_WS_MAC
    , m_iHostScreenNumber(0)
#endif /* VBOX_WS_MAC */
    , m_maxGuestSizePolicy(MaxGuestResolutionPolicy_Automatic)
    , m_u64MaxGuestSize(0)
#ifdef VBOX_WITH_VIDEOHWACCEL
    , m_fAccelerate2DVideo(bAccelerate2DVideo)
#endif /* VBOX_WITH_VIDEOHWACCEL */
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
    , m_fIsDraggingFromGuest(false)
#endif
    , m_pNativeEventFilter(0)
{
}

void UIMachineView::loadMachineViewSettings()
{
    /* Global settings: */
    {
        /* Remember the maximum guest size policy for
         * telling the guest about video modes we like: */
        m_maxGuestSizePolicy = gEDataManager->maxGuestResolutionPolicy();
        if (m_maxGuestSizePolicy == MaxGuestResolutionPolicy_Fixed)
            m_fixedMaxGuestSize = gEDataManager->maxGuestResolutionForPolicyFixed();
    }
}

void UIMachineView::prepareViewport()
{
    /* Prepare viewport: */
    AssertPtrReturnVoid(viewport());
    {
        /* Enable manual painting: */
        viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
        /* Enable multi-touch support: */
        viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
    }
}

void UIMachineView::prepareFrameBuffer()
{
    /* Check whether we already have corresponding frame-buffer: */
    UIFrameBuffer *pFrameBuffer = uisession()->frameBuffer(screenId());

    /* If we do: */
    if (pFrameBuffer)
    {
        /* Assign it's view: */
        pFrameBuffer->setView(this);
        /* Mark frame-buffer as used again: */
        LogRelFlow(("GUI: UIMachineView::prepareFrameBuffer: Start EMT callbacks accepting for screen: %d\n", screenId()));
        pFrameBuffer->setMarkAsUnused(false);
        /* And remember our choice: */
        m_pFrameBuffer = pFrameBuffer;
    }
    /* If we do not: */
    else
    {
#ifdef VBOX_WITH_VIDEOHWACCEL
        /* Create new frame-buffer: */
        m_pFrameBuffer = new UIFrameBuffer(m_fAccelerate2DVideo);
        m_pFrameBuffer->init(this);
#else /* VBOX_WITH_VIDEOHWACCEL */
        /* Create new frame-buffer: */
        m_pFrameBuffer = new UIFrameBuffer;
        m_pFrameBuffer->init(this);
#endif /* !VBOX_WITH_VIDEOHWACCEL */

        /* Take HiDPI optimization type into account: */
        m_pFrameBuffer->setHiDPIOptimizationType(gEDataManager->hiDPIOptimizationType(vboxGlobal().managedVMUuid()));

        /* Take scaling optimization type into account: */
        m_pFrameBuffer->setScalingOptimizationType(gEDataManager->scalingOptimizationType(vboxGlobal().managedVMUuid()));

#ifdef VBOX_WS_MAC
        /* Take backing scale-factor into account: */
        m_pFrameBuffer->setBackingScaleFactor(darwinBackingScaleFactor(machineWindow()));
#endif /* VBOX_WS_MAC */

        /* Take the scale-factor related attributes into account: */
        const double dScaleFactor = gEDataManager->scaleFactor(vboxGlobal().managedVMUuid());
        const bool fUseUnscaledHiDPIOutput = gEDataManager->useUnscaledHiDPIOutput(vboxGlobal().managedVMUuid());
        m_pFrameBuffer->setScaleFactor(dScaleFactor);
        m_pFrameBuffer->setUseUnscaledHiDPIOutput(fUseUnscaledHiDPIOutput);
        /* Propagate the scale-factor related attributes to 3D service if necessary: */
        if (machine().GetAccelerate3DEnabled() && vboxGlobal().is3DAvailable())
        {
            display().NotifyScaleFactorChange(m_uScreenId,
                                              (uint32_t)(dScaleFactor * VBOX_OGL_SCALE_FACTOR_MULTIPLIER),
                                              (uint32_t)(dScaleFactor * VBOX_OGL_SCALE_FACTOR_MULTIPLIER));
            display().NotifyHiDPIOutputPolicyChange(fUseUnscaledHiDPIOutput);
        }

        /* Perform frame-buffer rescaling: */
        m_pFrameBuffer->performRescale();

        /* Associate uisession with frame-buffer finally: */
        uisession()->setFrameBuffer(screenId(), m_pFrameBuffer);
    }

    /* Make sure frame-buffer was prepared: */
    AssertReturnVoid(m_pFrameBuffer);

    /* Reattach to IDisplay: */
    m_pFrameBuffer->detach();
    m_pFrameBuffer->attach();

    /* Calculate frame-buffer size: */
    QSize size;
    {
#ifdef VBOX_WS_X11
        /* Processing pseudo resize-event to synchronize frame-buffer with stored framebuffer size.
         * On X11 this will be additional done when the machine state was 'saved'. */
        if (machine().GetState() == KMachineState_Saved)
            size = guestScreenSizeHint();
#endif /* VBOX_WS_X11 */

        /* If there is a preview image saved,
         * we will resize the framebuffer to the size of that image: */
        ULONG uWidth = 0, uHeight = 0;
        QVector<KBitmapFormat> formats = machine().QuerySavedScreenshotInfo(0, uWidth, uHeight);
        if (formats.size() > 0)
        {
            /* Init with the screenshot size: */
            size = QSize(uWidth, uHeight);
            /* Try to get the real guest dimensions from the save-state: */
            ULONG uGuestOriginX = 0, uGuestOriginY = 0, uGuestWidth = 0, uGuestHeight = 0;
            BOOL fEnabled = true;
            machine().QuerySavedGuestScreenInfo(m_uScreenId, uGuestOriginX, uGuestOriginY, uGuestWidth, uGuestHeight, fEnabled);
            if (uGuestWidth  > 0 && uGuestHeight > 0)
                size = QSize(uGuestWidth, uGuestHeight);
        }

        /* If we have a valid size, resize/rescale the frame-buffer. */
        if (size.width() > 0 && size.height() > 0)
        {
            frameBuffer()->performResize(size.width(), size.height());
            frameBuffer()->performRescale();
        }
    }
}

void UIMachineView::prepareCommon()
{
    /* Prepare view frame: */
    setFrameStyle(QFrame::NoFrame);

    /* Setup palette: */
    QPalette palette(viewport()->palette());
    palette.setColor(viewport()->backgroundRole(), Qt::black);
    viewport()->setPalette(palette);

    /* Setup focus policy: */
    setFocusPolicy(Qt::WheelFocus);

#ifdef VBOX_WITH_DRAG_AND_DROP
    /* Enable drag & drop. */
    setAcceptDrops(true);

    /* Create the drag and drop handler instance.
     * At the moment we only support one instance per machine window. */
    m_pDnDHandler = new UIDnDHandler(uisession(), this /* pParent */);
#endif /* VBOX_WITH_DRAG_AND_DROP */
}

void UIMachineView::prepareFilters()
{
    /* Enable MouseMove events: */
    viewport()->setMouseTracking(true);

    /* We have to watch for own events too: */
    installEventFilter(this);

    /* QScrollView does the below on its own, but let's
     * do it anyway for the case it will not do it in the future: */
    viewport()->installEventFilter(this);

    /* We want to be notified on some parent's events: */
    machineWindow()->installEventFilter(this);
}

void UIMachineView::prepareConnections()
{
    /* Desktop resolution change (e.g. monitor hotplug): */
    connect(gpDesktop, SIGNAL(sigHostScreenResized(int)), this,
            SLOT(sltDesktopResized()));
    /* Scale-factor change: */
    connect(gEDataManager, SIGNAL(sigScaleFactorChange(const QString&)),
            this, SLOT(sltHandleScaleFactorChange(const QString&)));
    /* Scaling-optimization change: */
    connect(gEDataManager, SIGNAL(sigScalingOptimizationTypeChange(const QString&)),
            this, SLOT(sltHandleScalingOptimizationChange(const QString&)));
    /* HiDPI-optimization change: */
    connect(gEDataManager, SIGNAL(sigHiDPIOptimizationTypeChange(const QString&)),
            this, SLOT(sltHandleHiDPIOptimizationChange(const QString&)));
    /* Unscaled HiDPI output mode change: */
    connect(gEDataManager, SIGNAL(sigUnscaledHiDPIOutputModeChange(const QString&)),
            this, SLOT(sltHandleUnscaledHiDPIOutputModeChange(const QString&)));
}

void UIMachineView::prepareConsoleConnections()
{
    /* Machine state-change updater: */
    connect(uisession(), SIGNAL(sigMachineStateChange()), this, SLOT(sltMachineStateChanged()));
}

void UIMachineView::cleanupFrameBuffer()
{
    /* Make sure proper framebuffer assigned: */
    AssertReturnVoid(m_pFrameBuffer);
    AssertReturnVoid(m_pFrameBuffer == uisession()->frameBuffer(screenId()));

    /* Mark framebuffer as unused: */
    LogRelFlow(("GUI: UIMachineView::cleanupFrameBuffer: Stop EMT callbacks accepting for screen: %d\n", screenId()));
    m_pFrameBuffer->setMarkAsUnused(true);

    /* Process pending framebuffer events: */
    QApplication::sendPostedEvents(this, QEvent::MetaCall);

#ifdef VBOX_WITH_VIDEOHWACCEL
    if (m_fAccelerate2DVideo)
        QApplication::sendPostedEvents(this, VHWACommandProcessType);
#endif /* VBOX_WITH_VIDEOHWACCEL */

    /* Temporarily detach the framebuffer from IDisplay before detaching
     * from view in order to respect the thread synchonisation logic (see UIFrameBuffer.h).
     * Note: VBOX_WITH_CROGL additionally requires us to call DetachFramebuffer
     * to ensure 3D gets notified of view being destroyed... */
    if (console().isOk() && !display().isNull())
        m_pFrameBuffer->detach();

    /* Detach framebuffer from view: */
    m_pFrameBuffer->setView(NULL);
}

void UIMachineView::cleanupNativeFilters()
{
    /* If native event filter exists: */
    if (m_pNativeEventFilter)
    {
        /* Uninstall/destroy existing native event filter: */
        qApp->removeNativeEventFilter(m_pNativeEventFilter);
        delete m_pNativeEventFilter;
        m_pNativeEventFilter = 0;
    }
}

UISession* UIMachineView::uisession() const
{
    return machineWindow()->uisession();
}

CSession& UIMachineView::session() const
{
    return uisession()->session();
}

CMachine& UIMachineView::machine() const
{
    return uisession()->machine();
}

CConsole& UIMachineView::console() const
{
    return uisession()->console();
}

CDisplay& UIMachineView::display() const
{
    return uisession()->display();
}

CGuest& UIMachineView::guest() const
{
    return uisession()->guest();
}

UIActionPool* UIMachineView::actionPool() const
{
    return machineWindow()->actionPool();
}

UIMachineLogic* UIMachineView::machineLogic() const
{
    return machineWindow()->machineLogic();
}

QSize UIMachineView::sizeHint() const
{
    /* Temporarily restrict the size to prevent a brief resize to the
     * frame-buffer dimensions when we exit full-screen.  This is only
     * applied if the frame-buffer is at full-screen dimensions and
     * until the first machine view resize. */

    /* Get the frame-buffer dimensions: */
    QSize frameBufferSize(frameBuffer()->width(), frameBuffer()->height());
    /* Take the scale-factor(s) into account: */
    frameBufferSize = scaledForward(frameBufferSize);
    /* Check against the last full-screen size. */
    if (frameBufferSize == uisession()->lastFullScreenSize(screenId()) && m_sizeHintOverride.isValid())
        return m_sizeHintOverride;

    /* Get frame-buffer size-hint: */
    QSize size(m_pFrameBuffer->width(), m_pFrameBuffer->height());

    /* Take the scale-factor(s) into account: */
    size = scaledForward(size);

#ifdef VBOX_WITH_DEBUGGER_GUI
    /// @todo Fix all DEBUGGER stuff!
    /* HACK ALERT! Really ugly workaround for the resizing to 9x1 done by DevVGA if provoked before power on. */
    if (size.width() < 16 || size.height() < 16)
        if (vboxGlobal().shouldStartPaused() || vboxGlobal().isDebuggerAutoShowEnabled())
            size = QSize(640, 480);
#endif /* !VBOX_WITH_DEBUGGER_GUI */

    /* Return the resulting size-hint: */
    return QSize(size.width() + frameWidth() * 2, size.height() + frameWidth() * 2);
}

int UIMachineView::contentsX() const
{
    return horizontalScrollBar()->value();
}

int UIMachineView::contentsY() const
{
    return verticalScrollBar()->value();
}

int UIMachineView::contentsWidth() const
{
    return m_pFrameBuffer->width();
}

int UIMachineView::contentsHeight() const
{
    return m_pFrameBuffer->height();
}

int UIMachineView::visibleWidth() const
{
    return horizontalScrollBar()->pageStep();
}

int UIMachineView::visibleHeight() const
{
    return verticalScrollBar()->pageStep();
}

void UIMachineView::setMaxGuestSize(const QSize &minimumSizeHint /* = QSize() */)
{
    QSize maxSize;
    switch (m_maxGuestSizePolicy)
    {
        case MaxGuestResolutionPolicy_Fixed:
            maxSize = m_fixedMaxGuestSize;
            break;
        case MaxGuestResolutionPolicy_Automatic:
            maxSize = calculateMaxGuestSize().expandedTo(minimumSizeHint);
            break;
        case MaxGuestResolutionPolicy_Any:
            /* (0, 0) means any of course. */
            maxSize = QSize(0, 0);
    }
    ASMAtomicWriteU64(&m_u64MaxGuestSize,
                      RT_MAKE_U64(maxSize.height(), maxSize.width()));
}

QSize UIMachineView::maxGuestSize()
{
    uint64_t u64Size = ASMAtomicReadU64(&m_u64MaxGuestSize);
    return QSize(int(RT_HI_U32(u64Size)), int(RT_LO_U32(u64Size)));
}

bool UIMachineView::guestScreenVisibilityStatus() const
{
    /* Always 'true' for primary guest-screen: */
    if (m_uScreenId == 0)
        return true;

    /* Actual value for other guest-screens: */
    return gEDataManager->lastGuestScreenVisibilityStatus(m_uScreenId, vboxGlobal().managedVMUuid());
}

QSize UIMachineView::guestScreenSizeHint() const
{
    /* Load guest-screen size-hint: */
    QSize sizeHint = gEDataManager->lastGuestScreenSizeHint(m_uScreenId, vboxGlobal().managedVMUuid());

    /* Invent the default if necessary: */
    if (!sizeHint.isValid())
        sizeHint = QSize(800, 600);

    /* Take the scale-factor(s) into account: */
    sizeHint = scaledForward(sizeHint);

    /* Return size-hint: */
    return sizeHint;
}

void UIMachineView::storeGuestSizeHint(const QSize &sizeHint)
{
    /* Save guest-screen size-hint: */
    LogRel(("GUI: UIMachineView::storeGuestSizeHint: "
            "Storing guest-screen size-hint for screen %d as %dx%d\n",
            (int)screenId(), sizeHint.width(), sizeHint.height()));
    gEDataManager->setLastGuestScreenSizeHint(m_uScreenId, sizeHint, vboxGlobal().managedVMUuid());
}

void UIMachineView::handleScaleChange()
{
    LogRel(("GUI: UIMachineView::handleScaleChange: Screen=%d\n",
            (unsigned long)m_uScreenId));

    /* If machine-window is visible: */
    if (uisession()->isScreenVisible(m_uScreenId))
    {
        /* For 'scale' mode: */
        if (visualStateType() == UIVisualStateType_Scale)
        {
            /* Assign new frame-buffer logical-size: */
            frameBuffer()->setScaledSize(size());
        }
        /* For other than 'scale' mode: */
        else
        {
            /* Adjust maximum-size restriction for machine-view: */
            setMaximumSize(sizeHint());

            /* Force machine-window update own layout: */
            QCoreApplication::sendPostedEvents(0, QEvent::LayoutRequest);

            /* Update machine-view sliders: */
            updateSliders();

            /* By some reason Win host forgets to update machine-window central-widget
             * after main-layout was updated, let's do it for all the hosts: */
            machineWindow()->centralWidget()->update();

            /* Normalize 'normal' machine-window geometry: */
            if (visualStateType() == UIVisualStateType_Normal)
                machineWindow()->normalizeGeometry(true /* adjust position */);
        }

        /* Perform frame-buffer rescaling: */
        frameBuffer()->performRescale();
    }

    LogRelFlow(("GUI: UIMachineView::handleScaleChange: Complete for Screen=%d\n",
                (unsigned long)m_uScreenId));
}

void UIMachineView::resetPausePixmap()
{
    /* Reset pixmap(s): */
    m_pausePixmap = QPixmap();
    m_pausePixmapScaled = QPixmap();
}

void UIMachineView::takePausePixmapLive()
{
    /* Prepare a screen-shot: */
    QImage screenShot = QImage(m_pFrameBuffer->width(), m_pFrameBuffer->height(), QImage::Format_RGB32);
    /* Which will be a 'black image' by default. */
    screenShot.fill(0);

    /* For separate process: */
    if (vboxGlobal().isSeparateProcess())
    {
        /* Take screen-data to array: */
        const QVector<BYTE> screenData = display().TakeScreenShotToArray(screenId(), screenShot.width(), screenShot.height(), KBitmapFormat_BGR0);
        /* And copy that data to screen-shot if it is Ok: */
        if (display().isOk() && !screenData.isEmpty())
            memcpy(screenShot.bits(), screenData.data(), screenShot.width() * screenShot.height() * 4);
    }
    /* For the same process: */
    else
    {
        /* Take the screen-shot directly: */
        display().TakeScreenShot(screenId(), screenShot.bits(), screenShot.width(), screenShot.height(), KBitmapFormat_BGR0);
    }

    /* Dim screen-shot if it is Ok: */
    if (display().isOk() && !screenShot.isNull())
        dimImage(screenShot);

    /* Finally copy the screen-shot to pause-pixmap: */
    m_pausePixmap = QPixmap::fromImage(screenShot);
#ifdef VBOX_WS_MAC
    /* Adjust backing-scale-factor if necessary: */
    const double dBackingScaleFactor = frameBuffer()->backingScaleFactor();
    if (dBackingScaleFactor > 1.0 && frameBuffer()->useUnscaledHiDPIOutput())
        m_pausePixmap.setDevicePixelRatio(dBackingScaleFactor);
#endif /* VBOX_WS_MAC */

    /* Update scaled pause pixmap: */
    updateScaledPausePixmap();
}

void UIMachineView::takePausePixmapSnapshot()
{
    /* Acquire the screen-data from the saved-state: */
    ULONG uWidth = 0, uHeight = 0;
    const QVector<BYTE> screenData = machine().ReadSavedScreenshotToArray(0, KBitmapFormat_PNG, uWidth, uHeight);

    /* Make sure there is saved-state screen-data: */
    if (screenData.isEmpty())
        return;

    /* Acquire the screen-data properties from the saved-state: */
    ULONG uGuestOriginX = 0, uGuestOriginY = 0, uGuestWidth = 0, uGuestHeight = 0;
    BOOL fEnabled = true;
    machine().QuerySavedGuestScreenInfo(m_uScreenId, uGuestOriginX, uGuestOriginY, uGuestWidth, uGuestHeight, fEnabled);

    /* Create a screen-shot on the basis of the screen-data we have in saved-state: */
    QImage screenShot = QImage::fromData(screenData.data(), screenData.size(), "PNG").scaled(uGuestWidth > 0 ? QSize(uGuestWidth, uGuestHeight) : guestScreenSizeHint());

    /* Dim screen-shot if it is Ok: */
    if (machine().isOk() && !screenShot.isNull())
        dimImage(screenShot);

    /* Finally copy the screen-shot to pause-pixmap: */
    m_pausePixmap = QPixmap::fromImage(screenShot);
#ifdef VBOX_WS_MAC
    /* Adjust backing-scale-factor if necessary: */
    const double dBackingScaleFactor = frameBuffer()->backingScaleFactor();
    if (dBackingScaleFactor > 1.0 && frameBuffer()->useUnscaledHiDPIOutput())
        m_pausePixmap.setDevicePixelRatio(dBackingScaleFactor);
#endif /* VBOX_WS_MAC */

    /* Update scaled pause pixmap: */
    updateScaledPausePixmap();
}

void UIMachineView::updateScaledPausePixmap()
{
    /* Make sure pause pixmap is not null: */
    if (pausePixmap().isNull())
        return;

    /* Make sure scaled-size is not null: */
    const QSize scaledSize = frameBuffer()->scaledSize();
    if (!scaledSize.isValid())
        return;

    /* Update pause pixmap finally: */
    m_pausePixmapScaled = pausePixmap().scaled(scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
#ifdef VBOX_WS_MAC
    /* Adjust backing-scale-factor if necessary: */
    const double dBackingScaleFactor = frameBuffer()->backingScaleFactor();
    if (dBackingScaleFactor > 1.0 && frameBuffer()->useUnscaledHiDPIOutput())
        m_pausePixmapScaled.setDevicePixelRatio(dBackingScaleFactor);
#endif /* VBOX_WS_MAC */
}

void UIMachineView::updateSliders()
{
    /* Get current viewport size: */
    QSize curViewportSize = viewport()->size();
    /* Get maximum viewport size: */
    const QSize maxViewportSize = maximumViewportSize();
    /* Get current frame-buffer size: */
    QSize frameBufferSize = QSize(frameBuffer()->width(), frameBuffer()->height());

    /* Take the scale-factor(s) into account: */
    frameBufferSize = scaledForward(frameBufferSize);

    /* If maximum viewport size can cover whole frame-buffer => no scroll-bars required: */
    if (maxViewportSize.expandedTo(frameBufferSize) == maxViewportSize)
        curViewportSize = maxViewportSize;

    /* What length we want scroll-bars of? */
    int xRange = frameBufferSize.width()  - curViewportSize.width();
    int yRange = frameBufferSize.height() - curViewportSize.height();

#ifdef VBOX_WS_MAC
    /* Due to Qt 4.x doesn't supports HiDPI directly
     * we should take the backing-scale-factor into account.
     * See also viewportToContents()... */
    if (frameBuffer()->useUnscaledHiDPIOutput())
    {
        const double dBackingScaleFactor = frameBuffer()->backingScaleFactor();
        if (dBackingScaleFactor > 1.0)
        {
            xRange *= dBackingScaleFactor;
            yRange *= dBackingScaleFactor;
        }
    }
#endif /* VBOX_WS_MAC */

    /* Configure scroll-bars: */
    horizontalScrollBar()->setRange(0, xRange);
    verticalScrollBar()->setRange(0, yRange);
    horizontalScrollBar()->setPageStep(curViewportSize.width());
    verticalScrollBar()->setPageStep(curViewportSize.height());
}

QPoint UIMachineView::viewportToContents(const QPoint &vp) const
{
    /* Get physical contents shifts of scroll-bars: */
    int iContentsX = contentsX();
    int iContentsY = contentsY();

#ifdef VBOX_WS_MAC
    /* Due to Qt 4.x doesn't supports HiDPI directly
     * we should take the backing-scale-factor into account.
     * See also updateSliders()... */
    if (frameBuffer()->useUnscaledHiDPIOutput())
    {
        const double dBackingScaleFactor = frameBuffer()->backingScaleFactor();
        if (dBackingScaleFactor > 1.0)
        {
            iContentsX /= dBackingScaleFactor;
            iContentsY /= dBackingScaleFactor;
        }
    }
#endif /* VBOX_WS_MAC */

    /* Return point shifted according scroll-bars: */
    return QPoint(vp.x() + iContentsX, vp.y() + iContentsY);
}

void UIMachineView::scrollBy(int dx, int dy)
{
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() + dx);
    verticalScrollBar()->setValue(verticalScrollBar()->value() + dy);
}

void UIMachineView::dimImage(QImage &img)
{
    for (int y = 0; y < img.height(); ++ y)
    {
        if (y % 2)
        {
            if (img.depth() == 32)
            {
                for (int x = 0; x < img.width(); ++ x)
                {
                    int gray = qGray(img.pixel (x, y)) / 2;
                    img.setPixel(x, y, qRgb (gray, gray, gray));
                }
            }
            else
            {
                ::memset(img.scanLine (y), 0, img.bytesPerLine());
            }
        }
        else
        {
            if (img.depth() == 32)
            {
                for (int x = 0; x < img.width(); ++ x)
                {
                    int gray = (2 * qGray (img.pixel (x, y))) / 3;
                    img.setPixel(x, y, qRgb (gray, gray, gray));
                }
            }
        }
    }
}

void UIMachineView::scrollContentsBy(int dx, int dy)
{
#ifdef VBOX_WITH_VIDEOHWACCEL
    if (m_pFrameBuffer)
    {
        m_pFrameBuffer->viewportScrolled(dx, dy);
    }
#endif /* VBOX_WITH_VIDEOHWACCEL */
    QAbstractScrollArea::scrollContentsBy(dx, dy);

    /* Update console's display viewport and 3D overlay: */
    updateViewport();
}


#ifdef VBOX_WS_MAC
void UIMachineView::updateDockIcon()
{
    machineLogic()->updateDockIcon();
}

CGImageRef UIMachineView::vmContentImage()
{
    /* Use pause-image if exists: */
    if (!pausePixmap().isNull())
        return darwinToCGImageRef(&pausePixmap());

    /* Create the image ref out of the frame-buffer: */
    return frameBuffertoCGImageRef(m_pFrameBuffer);
}

CGImageRef UIMachineView::frameBuffertoCGImageRef(UIFrameBuffer *pFrameBuffer)
{
    CGImageRef ir = 0;
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    if (cs)
    {
        /* Create the image copy of the framebuffer */
        CGDataProviderRef dp = CGDataProviderCreateWithData(pFrameBuffer, pFrameBuffer->address(), pFrameBuffer->bitsPerPixel() / 8 * pFrameBuffer->width() * pFrameBuffer->height(), NULL);
        if (dp)
        {
            ir = CGImageCreate(pFrameBuffer->width(), pFrameBuffer->height(), 8, 32, pFrameBuffer->bytesPerLine(), cs,
                               kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host, dp, 0, false,
                               kCGRenderingIntentDefault);
            CGDataProviderRelease(dp);
        }
        CGColorSpaceRelease(cs);
    }
    return ir;
}
#endif /* VBOX_WS_MAC */

UIVisualStateType UIMachineView::visualStateType() const
{
    return machineLogic()->visualStateType();
}

bool UIMachineView::isFullscreenOrSeamless() const
{
    return    visualStateType() == UIVisualStateType_Fullscreen
           || visualStateType() == UIVisualStateType_Seamless;
}

bool UIMachineView::event(QEvent *pEvent)
{
    switch ((UIEventType)pEvent->type())
    {
#ifdef VBOX_WS_MAC
        /* Event posted OnShowWindow: */
        case ShowWindowEventType:
        {
            /* Dunno what Qt3 thinks a window that has minimized to the dock should be - it is not hidden,
             * neither is it minimized. OTOH it is marked shown and visible, but not activated.
             * This latter isn't of much help though, since at this point nothing is marked activated.
             * I might have overlooked something, but I'm buggered what if I know what. So, I'll just always
             * show & activate the stupid window to make it get out of the dock when the user wishes to show a VM: */
            window()->show();
            window()->activateWindow();
            return true;
        }
#endif /* VBOX_WS_MAC */

#ifdef VBOX_WITH_VIDEOHWACCEL
        case VHWACommandProcessType:
        {
            m_pFrameBuffer->doProcessVHWACommand(pEvent);
            return true;
        }
#endif /* VBOX_WITH_VIDEOHWACCEL */

        default:
            break;
    }

    return QAbstractScrollArea::event(pEvent);
}

bool UIMachineView::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    if (pWatched == viewport())
    {
        switch (pEvent->type())
        {
            case QEvent::Resize:
            {
#ifdef VBOX_WITH_VIDEOHWACCEL
                QResizeEvent* pResizeEvent = static_cast<QResizeEvent*>(pEvent);
                if (m_pFrameBuffer)
                    m_pFrameBuffer->viewportResized(pResizeEvent);
#endif /* VBOX_WITH_VIDEOHWACCEL */
                /* Update console's display viewport and 3D overlay: */
                updateViewport();
                break;
            }
            default:
                break;
        }
    }

    if (pWatched == this)
    {
        switch (pEvent->type())
        {
            case QEvent::Move:
            {
                /* Update console's display viewport and 3D overlay: */
                updateViewport();
                break;
            }
            default:
                break;
        }
    }

    if (pWatched == machineWindow())
    {
        switch (pEvent->type())
        {
            case QEvent::WindowStateChange:
            {
                /* During minimizing and state restoring machineWindow() gets
                 * the focus which belongs to console view window, so returning it properly. */
                QWindowStateChangeEvent *pWindowEvent = static_cast<QWindowStateChangeEvent*>(pEvent);
                if (pWindowEvent->oldState() & Qt::WindowMinimized)
                {
                    if (QApplication::focusWidget())
                    {
                        QApplication::focusWidget()->clearFocus();
                        qApp->processEvents();
                    }
                    QTimer::singleShot(0, this, SLOT(setFocus()));
                }
                break;
            }
#ifdef VBOX_WS_MAC
            case QEvent::Move:
            {
                /* Get current host-screen number: */
                const int iCurrentHostScreenNumber = gpDesktop->screenNumber(this);
                if (m_iHostScreenNumber != iCurrentHostScreenNumber)
                {
                    /* Recache current host screen: */
                    m_iHostScreenNumber = iCurrentHostScreenNumber;

                    /* Update frame-buffer arguments: */
                    if (m_pFrameBuffer)
                    {
                        /* Update backing-scale-factor for underlying frame-buffer: */
                        m_pFrameBuffer->setBackingScaleFactor(darwinBackingScaleFactor(machineWindow()));
                        /* Perform frame-buffer rescaling: */
                        m_pFrameBuffer->performRescale();
                    }

                    /* Update console's display viewport and 3D overlay: */
                    updateViewport();
                }
                break;
            }
#endif /* VBOX_WS_MAC */
            default:
                break;
        }
    }

    return QAbstractScrollArea::eventFilter(pWatched, pEvent);
}

void UIMachineView::resizeEvent(QResizeEvent *pEvent)
{
    updateSliders();
    return QAbstractScrollArea::resizeEvent(pEvent);
}

void UIMachineView::moveEvent(QMoveEvent *pEvent)
{
    return QAbstractScrollArea::moveEvent(pEvent);
}

void UIMachineView::paintEvent(QPaintEvent *pPaintEvent)
{
    /* Use pause-image if exists: */
    if (!pausePixmap().isNull())
    {
        /* We have a snapshot for the paused state: */
        QRect rect = pPaintEvent->rect().intersected(viewport()->rect());
        QPainter painter(viewport());
        /* Take the scale-factor into account: */
        if (frameBuffer()->scaleFactor() == 1.0 && !frameBuffer()->scaledSize().isValid())
            painter.drawPixmap(rect.topLeft(), pausePixmap());
        else
            painter.drawPixmap(rect.topLeft(), pausePixmapScaled());
#ifdef VBOX_WS_MAC
        /* Update the dock icon: */
        updateDockIcon();
#endif /* VBOX_WS_MAC */
        return;
    }

    /* Delegate the paint function to the UIFrameBuffer interface: */
    if (m_pFrameBuffer)
        m_pFrameBuffer->handlePaintEvent(pPaintEvent);
#ifdef VBOX_WS_MAC
    /* Update the dock icon if we are in the running state: */
    if (uisession()->isRunning())
        updateDockIcon();
#endif /* VBOX_WS_MAC */
}

void UIMachineView::focusInEvent(QFocusEvent *pEvent)
{
    /* Call to base-class: */
    QAbstractScrollArea::focusInEvent(pEvent);

    /* If native event filter isn't exists: */
    if (!m_pNativeEventFilter)
    {
        /* Create/install new native event filter: */
        m_pNativeEventFilter = new UINativeEventFilter(this);
        qApp->installNativeEventFilter(m_pNativeEventFilter);
    }
}

void UIMachineView::focusOutEvent(QFocusEvent *pEvent)
{
    /* If native event filter exists: */
    if (m_pNativeEventFilter)
    {
        /* Uninstall/destroy existing native event filter: */
        qApp->removeNativeEventFilter(m_pNativeEventFilter);
        delete m_pNativeEventFilter;
        m_pNativeEventFilter = 0;
    }

    /* Call to base-class: */
    QAbstractScrollArea::focusOutEvent(pEvent);
}

#ifdef VBOX_WITH_DRAG_AND_DROP
bool UIMachineView::dragAndDropCanAccept(void) const
{
    bool fAccept =  m_pDnDHandler
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
                 && !m_fIsDraggingFromGuest
#endif
                 && machine().GetDnDMode() != KDnDMode_Disabled;
    return fAccept;
}

bool UIMachineView::dragAndDropIsActive(void) const
{
    return (   m_pDnDHandler
            && machine().GetDnDMode() != KDnDMode_Disabled);
}

void UIMachineView::dragEnterEvent(QDragEnterEvent *pEvent)
{
    AssertPtrReturnVoid(pEvent);

    int rc = dragAndDropCanAccept() ? VINF_SUCCESS : VERR_ACCESS_DENIED;
    if (RT_SUCCESS(rc))
    {
        /* Get mouse-pointer location. */
        const QPoint &cpnt = viewportToContents(pEvent->pos());

        /* Ask the target for starting a DnD event. */
        Qt::DropAction result = m_pDnDHandler->dragEnter(screenId(),
                                                         frameBuffer()->convertHostXTo(cpnt.x()),
                                                         frameBuffer()->convertHostYTo(cpnt.y()),
                                                         pEvent->proposedAction(),
                                                         pEvent->possibleActions(),
                                                         pEvent->mimeData());

        /* Set the DnD action returned by the guest. */
        pEvent->setDropAction(result);
        pEvent->accept();
    }

    DNDDEBUG(("DnD: dragEnterEvent ended with rc=%Rrc\n", rc));
}

void UIMachineView::dragMoveEvent(QDragMoveEvent *pEvent)
{
    AssertPtrReturnVoid(pEvent);

    int rc = dragAndDropCanAccept() ? VINF_SUCCESS : VERR_ACCESS_DENIED;
    if (RT_SUCCESS(rc))
    {
        /* Get mouse-pointer location. */
        const QPoint &cpnt = viewportToContents(pEvent->pos());

        /* Ask the guest for moving the drop cursor. */
        Qt::DropAction result = m_pDnDHandler->dragMove(screenId(),
                                                        frameBuffer()->convertHostXTo(cpnt.x()),
                                                        frameBuffer()->convertHostYTo(cpnt.y()),
                                                        pEvent->proposedAction(),
                                                        pEvent->possibleActions(),
                                                        pEvent->mimeData());

        /* Set the DnD action returned by the guest. */
        pEvent->setDropAction(result);
        pEvent->accept();
    }

    DNDDEBUG(("DnD: dragMoveEvent ended with rc=%Rrc\n", rc));
}

void UIMachineView::dragLeaveEvent(QDragLeaveEvent *pEvent)
{
    AssertPtrReturnVoid(pEvent);

    int rc = dragAndDropCanAccept() ? VINF_SUCCESS : VERR_ACCESS_DENIED;
    if (RT_SUCCESS(rc))
    {
        m_pDnDHandler->dragLeave(screenId());

        pEvent->accept();
    }

    DNDDEBUG(("DnD: dragLeaveEvent ended with rc=%Rrc\n", rc));
}

int UIMachineView::dragCheckPending(void)
{
    int rc;

    if (!dragAndDropIsActive())
        rc = VERR_ACCESS_DENIED;
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
    else if (!m_fIsDraggingFromGuest)
    {
        /** @todo Add guest->guest DnD functionality here by getting
         *        the source of guest B (when copying from B to A). */
        rc = m_pDnDHandler->dragCheckPending(screenId());
        if (RT_SUCCESS(rc))
            m_fIsDraggingFromGuest = true;
    }
    else /* Already dragging, so report success. */
        rc = VINF_SUCCESS;
#else
    rc = VERR_NOT_SUPPORTED;
#endif

    DNDDEBUG(("DnD: dragCheckPending ended with rc=%Rrc\n", rc));
    return rc;
}

int UIMachineView::dragStart(void)
{
    int rc;

    if (!dragAndDropIsActive())
        rc = VERR_ACCESS_DENIED;
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
    else if (!m_fIsDraggingFromGuest)
        rc = VERR_WRONG_ORDER;
    else
    {
        /** @todo Add guest->guest DnD functionality here by getting
         *        the source of guest B (when copying from B to A). */
        rc = m_pDnDHandler->dragStart(screenId());

        m_fIsDraggingFromGuest = false;
    }
#else
    rc = VERR_NOT_SUPPORTED;
#endif

    DNDDEBUG(("DnD: dragStart ended with rc=%Rrc\n", rc));
    return rc;
}

int UIMachineView::dragStop(void)
{
    int rc;

    if (!dragAndDropIsActive())
        rc = VERR_ACCESS_DENIED;
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
    else if (!m_fIsDraggingFromGuest)
        rc = VERR_WRONG_ORDER;
    else
        rc = m_pDnDHandler->dragStop(screenId());
#else
    rc = VERR_NOT_SUPPORTED;
#endif

    DNDDEBUG(("DnD: dragStop ended with rc=%Rrc\n", rc));
    return rc;
}

void UIMachineView::dropEvent(QDropEvent *pEvent)
{
    AssertPtrReturnVoid(pEvent);

    int rc = dragAndDropCanAccept() ? VINF_SUCCESS : VERR_ACCESS_DENIED;
    if (RT_SUCCESS(rc))
    {
        /* Get mouse-pointer location. */
        const QPoint &cpnt = viewportToContents(pEvent->pos());

        /* Ask the guest for dropping data. */
        Qt::DropAction result = m_pDnDHandler->dragDrop(screenId(),
                                                        frameBuffer()->convertHostXTo(cpnt.x()),
                                                        frameBuffer()->convertHostYTo(cpnt.y()),
                                                        pEvent->proposedAction(),
                                                        pEvent->possibleActions(),
                                                        pEvent->mimeData());

        /* Set the DnD action returned by the guest. */
        pEvent->setDropAction(result);
        pEvent->accept();
    }

    DNDDEBUG(("DnD: dropEvent ended with rc=%Rrc\n", rc));
}
#endif /* VBOX_WITH_DRAG_AND_DROP */

bool UIMachineView::nativeEventPreprocessor(const QByteArray &eventType, void *pMessage)
{
    /* Check if some event should be filtered out.
     * Returning @c true means filtering-out,
     * Returning @c false means passing event to Qt. */

# if defined(VBOX_WS_MAC)

    /* Make sure it's generic NSEvent: */
    if (eventType != "mac_generic_NSEvent")
        return false;
    EventRef event = static_cast<EventRef>(darwinCocoaToCarbonEvent(pMessage));

    switch (::GetEventClass(event))
    {
        // Keep in mind that this stuff should not be enabled while we are still using
        // own native keyboard filter installed through cocoa API, to be reworked.
        // S.a. registerForNativeEvents call in UIKeyboardHandler implementation.
#if 0
        /* Watch for keyboard-events: */
        case kEventClassKeyboard:
        {
            switch (::GetEventKind(event))
            {
                /* Watch for key-events: */
                case kEventRawKeyDown:
                case kEventRawKeyRepeat:
                case kEventRawKeyUp:
                case kEventRawKeyModifiersChanged:
                {
                    /* Delegate key-event handling to the keyboard-handler: */
                    return machineLogic()->keyboardHandler()->nativeEventFilter(pMessage, screenId());
                }
                default:
                    break;
            }
            break;
        }
#endif
        /* Watch for mouse-events: */
        case kEventClassMouse:
        {
            switch (::GetEventKind(event))
            {
                /* Watch for button-events: */
                case kEventMouseDown:
                case kEventMouseUp:
                {
                    /* Delegate button-event handling to the mouse-handler: */
                    return machineLogic()->mouseHandler()->nativeEventFilter(pMessage, screenId());
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }

# elif defined(VBOX_WS_WIN)

    /* Make sure it's generic MSG event: */
    if (eventType != "windows_generic_MSG")
        return false;
    MSG *pEvent = static_cast<MSG*>(pMessage);

    switch (pEvent->message)
    {
        /* Watch for key-events: */
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            // WORKAROUND:
            // There is an issue in the Windows Qt5 event processing sequence
            // causing QAbstractNativeEventFilter to receive Windows native events
            // coming not just to the top-level window but to actual target as well.
            // They are calling one - "global event" and another one - "context event".
            // That way native events are always duplicated with almost no possibility
            // to distinguish copies except the fact that synthetic event always have
            // time set to 0 (actually that field was not initialized at all, we had
            // fixed that in our private Qt tool). We should skip such events instantly.
            if (pEvent->time == 0)
                return false;

            /* Delegate key-event handling to the keyboard-handler: */
            return machineLogic()->keyboardHandler()->nativeEventFilter(pMessage, screenId());
        }
        default:
            break;
    }

# elif defined(VBOX_WS_X11)

    /* Make sure it's generic XCB event: */
    if (eventType != "xcb_generic_event_t")
        return false;
    xcb_generic_event_t *pEvent = static_cast<xcb_generic_event_t*>(pMessage);

    switch (pEvent->response_type & ~0x80)
    {
        /* Watch for key-events: */
        case XCB_KEY_PRESS:
        case XCB_KEY_RELEASE:
        {
            /* Delegate key-event handling to the keyboard-handler: */
            return machineLogic()->keyboardHandler()->nativeEventFilter(pMessage, screenId());
        }
        /* Watch for button-events: */
        case XCB_BUTTON_PRESS:
        case XCB_BUTTON_RELEASE:
        {
            /* Delegate button-event handling to the mouse-handler: */
            return machineLogic()->mouseHandler()->nativeEventFilter(pMessage, screenId());
        }
        default:
            break;
    }

# else

#  warning "port me!"

# endif

    /* Filter nothing by default: */
    return false;
}

QSize UIMachineView::scaledForward(QSize size) const
{
    /* Take the scale-factor into account: */
    const double dScaleFactor = frameBuffer()->scaleFactor();
    if (dScaleFactor != 1.0)
        size = QSize((int)(size.width() * dScaleFactor), (int)(size.height() * dScaleFactor));

#ifdef VBOX_WS_MAC
    /* Take the backing-scale-factor into account: */
    if (frameBuffer()->useUnscaledHiDPIOutput())
    {
        const double dBackingScaleFactor = frameBuffer()->backingScaleFactor();
        if (dBackingScaleFactor > 1.0)
            size = QSize(size.width() / dBackingScaleFactor, size.height() / dBackingScaleFactor);
    }
#endif /* VBOX_WS_MAC */

    /* Return result: */
    return size;
}

QSize UIMachineView::scaledBackward(QSize size) const
{
#ifdef VBOX_WS_MAC
    /* Take the backing-scale-factor into account: */
    if (frameBuffer()->useUnscaledHiDPIOutput())
    {
        const double dBackingScaleFactor = frameBuffer()->backingScaleFactor();
        if (dBackingScaleFactor > 1.0)
            size = QSize(size.width() * dBackingScaleFactor, size.height() * dBackingScaleFactor);
    }
#endif /* VBOX_WS_MAC */

    /* Take the scale-factor into account: */
    const double dScaleFactor = frameBuffer()->scaleFactor();
    if (dScaleFactor != 1.0)
        size = QSize((int)(size.width() / dScaleFactor), (int)(size.height() / dScaleFactor));

    /* Return result: */
    return size;
}

