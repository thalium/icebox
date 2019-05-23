/* $Id: UIFrameBuffer.h $ */
/** @file
 * VBox Qt GUI - UIFrameBuffer class declaration.
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

#ifndef ___UIFrameBuffer_h___
#define ___UIFrameBuffer_h___

/* Qt includes: */
#include <QSize>

/* GUI includes: */
#include "UIExtraDataDefs.h"

/* Other VBox includes: */
#include <VBox/com/ptr.h>

/* Forward declarations: */
class UIFrameBufferPrivate;
class UIMachineView;
class QResizeEvent;
class QPaintEvent;
class QRegion;

/** IFramebuffer implementation used to maintain VM display video memory. */
class UIFrameBuffer : public QObject
{
    Q_OBJECT;

public:

#ifdef VBOX_WITH_VIDEOHWACCEL
    /** Frame-buffer constructor.
      * @param m_fAccelerate2DVideo defines whether we should use VBoxOverlayFrameBuffer
      *                             instead of the default one. */
    UIFrameBuffer(bool m_fAccelerate2DVideo);
#else /* !VBOX_WITH_VIDEOHWACCEL */
    /** Frame-buffer constructor. */
    UIFrameBuffer();
#endif /* !VBOX_WITH_VIDEOHWACCEL */

    /** Frame-buffer destructor. */
    ~UIFrameBuffer();

    /** Frame-buffer initialization.
      * @param pMachineView defines machine-view this frame-buffer is bounded to. */
    HRESULT init(UIMachineView *pMachineView);

    /** Assigns machine-view frame-buffer will be bounded to.
      * @param pMachineView defines machine-view this frame-buffer is bounded to. */
    void setView(UIMachineView *pMachineView);

    /** Attach frame-buffer to the Display. */
    void attach();
    /** Detach frame-buffer from the Display. */
    void detach();

    /** Returns frame-buffer data address. */
    uchar* address();
    /** Returns frame-buffer width. */
    ulong width() const;
    /** Returns frame-buffer height. */
    ulong height() const;
    /** Returns frame-buffer bits-per-pixel value. */
    ulong bitsPerPixel() const;
    /** Returns frame-buffer bytes-per-line value. */
    ulong bytesPerLine() const;
    /** Returns the visual-state this frame-buffer is used for. */
    UIVisualStateType visualState() const;

    /** Defines whether frame-buffer is <b>unused</b>.
      * @note Calls to this and any other EMT callback are synchronized (from GUI side). */
    void setMarkAsUnused(bool fUnused);

    /** Returns whether frame-buffer is <b>auto-enabled</b>.
      * @note Refer to m_fAutoEnabled for more information. */
    bool isAutoEnabled() const;
    /** Defines whether frame-buffer is <b>auto-enabled</b>.
      * @note Refer to m_fAutoEnabled for more information. */
    void setAutoEnabled(bool fAutoEnabled);

    /** Returns the frame-buffer's scaled-size. */
    QSize scaledSize() const;
    /** Defines host-to-guest scale ratio as @a size. */
    void setScaledSize(const QSize &size = QSize());
    /** Returns x-origin of the guest (actual) content corresponding to x-origin of host (scaled) content. */
    int convertHostXTo(int iX) const;
    /** Returns y-origin of the guest (actual) content corresponding to y-origin of host (scaled) content. */
    int convertHostYTo(int iY) const;

    /** Returns the scale-factor used by the frame-buffer. */
    double scaleFactor() const;
    /** Define the scale-factor used by the frame-buffer. */
    void setScaleFactor(double dScaleFactor);

    /** Returns backing-scale-factor used by HiDPI frame-buffer. */
    double backingScaleFactor() const;
    /** Defines backing-scale-factor used by HiDPI frame-buffer. */
    void setBackingScaleFactor(double dBackingScaleFactor);

    /** Returns whether frame-buffer should use unscaled HiDPI output. */
    bool useUnscaledHiDPIOutput() const;
    /** Defines whether frame-buffer should use unscaled HiDPI output. */
    void setUseUnscaledHiDPIOutput(bool fUseUnscaledHiDPIOutput);

    /** Returns the frame-buffer scaling optimization type. */
    ScalingOptimizationType scalingOptimizationType() const;
    /** Defines the frame-buffer scaling optimization type. */
    void setScalingOptimizationType(ScalingOptimizationType type);

    /** Returns HiDPI frame-buffer optimization type. */
    HiDPIOptimizationType hiDPIOptimizationType() const;
    /** Defines HiDPI frame-buffer optimization type: */
    void setHiDPIOptimizationType(HiDPIOptimizationType type);

    /** Handles frame-buffer notify-change-event. */
    void handleNotifyChange(int iWidth, int iHeight);
    /** Handles frame-buffer paint-event. */
    void handlePaintEvent(QPaintEvent *pEvent);
    /** Handles frame-buffer set-visible-region-event. */
    void handleSetVisibleRegion(const QRegion &region);

    /** Performs frame-buffer resizing. */
    void performResize(int iWidth, int iHeight);
    /** Performs frame-buffer rescaling. */
    void performRescale();

#ifdef VBOX_WITH_VIDEOHWACCEL
    /** Performs Video HW Acceleration command. */
    void doProcessVHWACommand(QEvent *pEvent);
    /** Handles viewport resize-event. */
    void viewportResized(QResizeEvent *pEvent);
    /** Handles viewport scroll-event. */
    void viewportScrolled(int iX, int iY);
#endif /* VBOX_WITH_VIDEOHWACCEL */

private:

    /** Holds the frame-buffer private instance. */
    ComObjPtr<UIFrameBufferPrivate> m_pFrameBuffer;
};

#endif /* !___UIFrameBuffer_h___ */
