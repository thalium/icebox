/* $Id: UIMachineViewScale.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineViewScale class implementation.
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
# include <QTimer>

/* GUI includes */
# include "VBoxGlobal.h"
# include "UISession.h"
# include "UIMachineLogic.h"
# include "UIMachineWindow.h"
# include "UIMachineViewScale.h"
# include "UIFrameBuffer.h"
# include "UIExtraDataManager.h"
# include "UIDesktopWidgetWatchdog.h"

/* COM includes: */
# include "CConsole.h"
# include "CDisplay.h"

/* Other VBox includes: */
# include "VBox/log.h"
# include <VBox/VBoxOGL.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIMachineViewScale::UIMachineViewScale(  UIMachineWindow *pMachineWindow
                                       , ulong uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                                       , bool bAccelerate2DVideo
#endif
                                       )
    : UIMachineView(  pMachineWindow
                    , uScreenId
#ifdef VBOX_WITH_VIDEOHWACCEL
                    , bAccelerate2DVideo
#endif
                    )
{
}

void UIMachineViewScale::sltPerformGuestScale()
{
    /* Adjust frame-buffer scaled-size: */
    frameBuffer()->setScaledSize(size());
    frameBuffer()->performRescale();

    /* If scaled-size is valid: */
    const QSize scaledSize = frameBuffer()->scaledSize();
    if (scaledSize.isValid())
    {
        /* Propagate scale-factor to 3D service if necessary: */
        if (machine().GetAccelerate3DEnabled() && vboxGlobal().is3DAvailable())
        {
            const double xScaleFactor = (double)scaledSize.width()  / frameBuffer()->width();
            const double yScaleFactor = (double)scaledSize.height() / frameBuffer()->height();
            display().NotifyScaleFactorChange(m_uScreenId,
                                              (uint32_t)(xScaleFactor * VBOX_OGL_SCALE_FACTOR_MULTIPLIER),
                                              (uint32_t)(yScaleFactor * VBOX_OGL_SCALE_FACTOR_MULTIPLIER));
        }
    }

    /* Scale the pause-pixmap: */
    updateScaledPausePixmap();

    /* Update viewport: */
    viewport()->repaint();

    /* Update machine-view sliders: */
    updateSliders();
}

bool UIMachineViewScale::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    if (pWatched != 0 && pWatched == viewport())
    {
        switch (pEvent->type())
        {
            case QEvent::Resize:
            {
                /* Perform the actual resize: */
                sltPerformGuestScale();
                break;
            }
            default:
                break;
        }
    }

    return UIMachineView::eventFilter(pWatched, pEvent);
}

void UIMachineViewScale::applyMachineViewScaleFactor()
{
    /* If scaled-size is valid: */
    const QSize scaledSize = frameBuffer()->scaledSize();
    if (scaledSize.isValid())
    {
        /* Propagate scale-factor to 3D service if necessary: */
        if (machine().GetAccelerate3DEnabled() && vboxGlobal().is3DAvailable())
        {
            const double xScaleFactor = (double)scaledSize.width()  / frameBuffer()->width();
            const double yScaleFactor = (double)scaledSize.height() / frameBuffer()->height();
            display().NotifyScaleFactorChange(m_uScreenId,
                                              (uint32_t)(xScaleFactor * VBOX_OGL_SCALE_FACTOR_MULTIPLIER),
                                              (uint32_t)(yScaleFactor * VBOX_OGL_SCALE_FACTOR_MULTIPLIER));
        }
    }

    /* Take unscaled HiDPI output mode into account: */
    const bool fUseUnscaledHiDPIOutput = gEDataManager->useUnscaledHiDPIOutput(vboxGlobal().managedVMUuid());
    frameBuffer()->setUseUnscaledHiDPIOutput(fUseUnscaledHiDPIOutput);
    /* Propagate unscaled-hidpi-output feature to 3D service if necessary: */
    if (machine().GetAccelerate3DEnabled() && vboxGlobal().is3DAvailable())
    {
        display().NotifyHiDPIOutputPolicyChange(fUseUnscaledHiDPIOutput);
    }

    /* Perform frame-buffer rescaling: */
    frameBuffer()->performRescale();

    /* Update console's display viewport and 3D overlay: */
    updateViewport();
}

void UIMachineViewScale::resendSizeHint()
{
    /* Get the last guest-screen size-hint, taking the scale factor into account. */
    const QSize sizeHint = scaledBackward(guestScreenSizeHint());
    LogRel(("GUI: UIMachineViewScale::resendSizeHint: Restoring guest size-hint for screen %d to %dx%d\n",
            (int)screenId(), sizeHint.width(), sizeHint.height()));

    /* Expand current limitations: */
    setMaxGuestSize(sizeHint);

    /* Send saved size-hint to the guest: */
    display().SetVideoModeHint(screenId(),
                               guestScreenVisibilityStatus(),
                               false, 0, 0, sizeHint.width(), sizeHint.height(), 0);
    uisession()->setScreenVisibleHostDesires(screenId(), guestScreenVisibilityStatus());
}

QSize UIMachineViewScale::sizeHint() const
{
    /* Base-class have its own thoughts about size-hint
     * but scale-mode needs no size-hint to be set: */
    return QSize();
}

QRect UIMachineViewScale::workingArea() const
{
    return gpDesktop->availableGeometry(this);
}

QSize UIMachineViewScale::calculateMaxGuestSize() const
{
    /* 1) The calculation below is not reliable on some (X11) platforms until we
     *    have been visible for a fraction of a second, so so the best we can
     *    otherwise.
     * 2) We also get called early before "machineWindow" has been fully
     *    initialised, at which time we can't perform the calculation. */
    if (!isVisible())
        return workingArea().size() * 0.95;
    /* The area taken up by the machine window on the desktop, including window
     * frame, title, menu bar and status bar. */
    QSize windowSize = machineWindow()->frameGeometry().size();
    /* The window shouldn't be allowed to expand beyond the working area
     * unless it already does.  In that case the guest shouldn't expand it
     * any further though. */
    QSize maximumSize = workingArea().size().expandedTo(windowSize);
    /* The current size of the machine display. */
    QSize centralWidgetSize = machineWindow()->centralWidget()->size();
    /* To work out how big the guest display can get without the window going
     * over the maximum size we calculated above, we work out how much space
     * the other parts of the window (frame, menu bar, status bar and so on)
     * take up and subtract that space from the maximum window size. The
     * central widget shouldn't be bigger than the window, but we bound it for
     * sanity (or insanity) reasons. */
    return maximumSize - (windowSize - centralWidgetSize.boundedTo(windowSize));
}

void UIMachineViewScale::updateSliders()
{
    if (horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    if (verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

