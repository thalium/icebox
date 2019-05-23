/* $Id: UIMouseHandler.h $ */
/** @file
 * VBox Qt GUI - UIMouseHandler class declaration.
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

#ifndef ___UIMouseHandler_h___
#define ___UIMouseHandler_h___

/* Qt includes: */
#include <QMap>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>

/* GUI includes: */
#include "UIExtraDataDefs.h"

/* Forward declarations: */
class QTouchEvent;
class QWidget;
class UISession;
class UIMachineLogic;
class UIMachineWindow;
class UIMachineView;
class CDisplay;
class CMouse;


/* Delegate to control VM mouse functionality: */
class UIMouseHandler : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies listeners about state-change. */
    void sigStateChange(int iState);

public:

    /* Factory functions to create/destroy mouse-handler: */
    static UIMouseHandler* create(UIMachineLogic *pMachineLogic, UIVisualStateType visualStateType);
    static void destroy(UIMouseHandler *pMouseHandler);

    /* Prepare/cleanup listener for particular machine-window: */
    void prepareListener(ulong uIndex, UIMachineWindow *pMachineWindow);
    void cleanupListener(ulong uIndex);

    /* Commands to capture/release mouse: */
    void captureMouse(ulong uScreenId);
    void releaseMouse();

    /* Setter for mouse-integration feature: */
    void setMouseIntegrationEnabled(bool fEnabled);

    /* Current mouse state: */
    int state() const;

    /** Qt5: Performs pre-processing of all the native events. */
    bool nativeEventFilter(void *pMessage, ulong uScreenId);

protected slots:

    /* Machine state-change handler: */
    virtual void sltMachineStateChanged();

    /* Mouse capability-change handler: */
    virtual void sltMouseCapabilityChanged();

    /* Mouse pointer-shape-change handler: */
    virtual void sltMousePointerShapeChanged();

    /** Activate hovered window if any. */
    void sltMaybeActivateHoveredWindow();

protected:

    /* Mouse-handler constructor/destructor: */
    UIMouseHandler(UIMachineLogic *pMachineLogic);
    virtual ~UIMouseHandler();

    /* Getters: */
    UIMachineLogic* machineLogic() const;
    UISession* uisession() const;

    /** Returns the console's display reference. */
    CDisplay& display() const;
    /** Returns the console's mouse reference. */
    CMouse& mouse() const;

    /* Event handler for registered machine-view(s): */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Separate function to handle most of existing mouse-events: */
    bool mouseEvent(int iEventType, ulong uScreenId,
                    const QPoint &relativePos, const QPoint &globalPos,
                    Qt::MouseButtons mouseButtons,
                    int wheelDelta, Qt::Orientation wheelDirection);

    /* Separate function to handle incoming multi-touch events: */
    bool multiTouchEvent(QTouchEvent *pTouchEvent, ulong uScreenId);

#ifdef VBOX_WS_WIN
    /* This method is actually required only because under win-host
     * we do not really grab the mouse in case of capturing it: */
    void updateMouseCursorClipping();
    QRect m_mouseCursorClippingRect;
#endif /* VBOX_WS_WIN */

    /* Machine logic parent: */
    UIMachineLogic *m_pMachineLogic;

    /* Registered machine-windows(s): */
    QMap<ulong, QWidget*> m_windows;
    /* Registered machine-view(s): */
    QMap<ulong, UIMachineView*> m_views;
    /* Registered machine-view-viewport(s): */
    QMap<ulong, QWidget*> m_viewports;

    /** Hovered window to be activated. */
    QPointer<QWidget> m_pHoveredWindow;

    /* Other mouse variables: */
    QPoint m_lastMousePos;
    QPoint m_capturedMousePos;
    int m_iLastMouseWheelDelta;
    int m_iMouseCaptureViewIndex;
};

#endif // !___UIMouseHandler_h___

