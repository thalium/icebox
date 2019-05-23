/* $Id: UIDesktopWidgetWatchdog.cpp $ */
/** @file
 * VBox Qt GUI - UIDesktopWidgetWatchdog class implementation.
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
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
# include <QApplication>
# include <QDesktopWidget>
# include <QScreen>
# ifdef VBOX_WS_X11
#  include <QTimer>
# endif

/* GUI includes: */
# include "UIDesktopWidgetWatchdog.h"
# ifdef VBOX_WS_X11
#  include "VBoxGlobal.h"
# endif /* VBOX_WS_X11 */

/* Other VBox includes: */
# include <iprt/assert.h>
# include <VBox/log.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


#ifdef VBOX_WS_X11

/** QWidget extension used as
  * an invisible window on the basis of which we
  * can calculate available host-screen geometry. */
class UIInvisibleWindow : public QWidget
{
    Q_OBJECT;

signals:

    /** Notifies listeners about host-screen available-geometry was calulated.
      * @param iHostScreenIndex  holds the index of the host-screen this window created for.
      * @param availableGeometry holds the available-geometry of the host-screen this window created for. */
    void sigHostScreenAvailableGeometryCalculated(int iHostScreenIndex, QRect availableGeometry);

public:

    /** Constructs invisible window for the host-screen with @a iHostScreenIndex. */
    UIInvisibleWindow(int iHostScreenIndex);

private slots:

    /** Performs fallback drop. */
    void sltFallback();

private:

    /** Move @a pEvent handler. */
    void moveEvent(QMoveEvent *pEvent);
    /** Resize @a pEvent handler. */
    void resizeEvent(QResizeEvent *pEvent);

    /** Holds the index of the host-screen this window created for. */
    const int m_iHostScreenIndex;

    /** Holds whether the move event came. */
    bool m_fMoveCame;
    /** Holds whether the resize event came. */
    bool m_fResizeCame;
};


/*********************************************************************************************************************************
*   Class UIInvisibleWindow implementation.                                                                                      *
*********************************************************************************************************************************/

UIInvisibleWindow::UIInvisibleWindow(int iHostScreenIndex)
    : QWidget(0, Qt::Window | Qt::FramelessWindowHint)
    , m_iHostScreenIndex(iHostScreenIndex)
    , m_fMoveCame(false)
    , m_fResizeCame(false)
{
    /* Resize to minimum size of 1 pixel: */
    resize(1, 1);
    /* Apply visual and mouse-event mask for that 1 pixel: */
    setMask(QRect(0, 0, 1, 1));
    /* For composite WMs make this 1 pixel transparent: */
    if (vboxGlobal().isCompositingManagerRunning())
        setAttribute(Qt::WA_TranslucentBackground);
    /* Install fallback handler: */
    QTimer::singleShot(5000, this, SLOT(sltFallback()));
}

void UIInvisibleWindow::sltFallback()
{
    /* Sanity check for fallback geometry: */
    QRect fallbackGeometry(x(), y(), width(), height());
    if (   fallbackGeometry.width() <= 1
        || fallbackGeometry.height() <= 1)
        fallbackGeometry = gpDesktop->screenGeometry(m_iHostScreenIndex);
    LogRel(("GUI: UIInvisibleWindow::sltFallback: %s event haven't came. "
            "Screen: %d, work area: %dx%d x %dx%d\n",
            !m_fMoveCame ? "Move" : !m_fResizeCame ? "Resize" : "Some",
            m_iHostScreenIndex, fallbackGeometry.x(), fallbackGeometry.y(), fallbackGeometry.width(), fallbackGeometry.height()));
    emit sigHostScreenAvailableGeometryCalculated(m_iHostScreenIndex, fallbackGeometry);
}

void UIInvisibleWindow::moveEvent(QMoveEvent *pEvent)
{
    /* We do have both move and resize events,
     * with no idea who will come first, but we need
     * to send a final signal after last of events arrived. */

    /* Call to base-class: */
    QWidget::moveEvent(pEvent);

    /* Ignore 'not-yet-shown' case: */
    if (!isVisible())
        return;

    /* Mark move event as received: */
    m_fMoveCame = true;

    /* If the resize event already came: */
    if (m_fResizeCame)
    {
        /* Notify listeners about host-screen available-geometry was calulated: */
        LogRel2(("GUI: UIInvisibleWindow::moveEvent: Screen: %d, work area: %dx%d x %dx%d\n", m_iHostScreenIndex,
                 x(), y(), width(), height()));
        emit sigHostScreenAvailableGeometryCalculated(m_iHostScreenIndex, QRect(x(), y(), width(), height()));
    }
}

void UIInvisibleWindow::resizeEvent(QResizeEvent *pEvent)
{
    /* We do have both move and resize events,
     * with no idea who will come first, but we need
     * to send a final signal after last of events arrived. */

    /* Call to base-class: */
    QWidget::resizeEvent(pEvent);

    /* Ignore 'not-yet-shown' case: */
    if (!isVisible())
        return;

    /* Mark resize event as received: */
    m_fResizeCame = true;

    /* If the move event already came: */
    if (m_fMoveCame)
    {
        /* Notify listeners about host-screen available-geometry was calulated: */
        LogRel2(("GUI: UIInvisibleWindow::resizeEvent: Screen: %d, work area: %dx%d x %dx%d\n", m_iHostScreenIndex,
                 x(), y(), width(), height()));
        emit sigHostScreenAvailableGeometryCalculated(m_iHostScreenIndex, QRect(x(), y(), width(), height()));
    }
}

#endif /* VBOX_WS_X11 */


/*********************************************************************************************************************************
*   Class UIDesktopWidgetWatchdog implementation.                                                                                *
*********************************************************************************************************************************/

/* static */
UIDesktopWidgetWatchdog *UIDesktopWidgetWatchdog::m_spInstance = 0;

/* static */
void UIDesktopWidgetWatchdog::create()
{
    /* Make sure instance isn't created: */
    AssertReturnVoid(!m_spInstance);

    /* Create/prepare instance: */
    new UIDesktopWidgetWatchdog;
    AssertReturnVoid(m_spInstance);
    m_spInstance->prepare();
}

/* static */
void UIDesktopWidgetWatchdog::destroy()
{
    /* Make sure instance is created: */
    AssertReturnVoid(m_spInstance);

    /* Cleanup/destroy instance: */
    m_spInstance->cleanup();
    delete m_spInstance;
    AssertReturnVoid(!m_spInstance);
}

UIDesktopWidgetWatchdog::UIDesktopWidgetWatchdog()
{
    /* Initialize instance: */
    m_spInstance = this;
}

UIDesktopWidgetWatchdog::~UIDesktopWidgetWatchdog()
{
    /* Deinitialize instance: */
    m_spInstance = 0;
}

int UIDesktopWidgetWatchdog::overallDesktopWidth() const
{
    /* Redirect call to desktop-widget: */
    return QApplication::desktop()->width();
}

int UIDesktopWidgetWatchdog::overallDesktopHeight() const
{
    /* Redirect call to desktop-widget: */
    return QApplication::desktop()->height();
}

int UIDesktopWidgetWatchdog::screenCount() const
{
    /* Redirect call to desktop-widget: */
    return QApplication::desktop()->screenCount();
}

int UIDesktopWidgetWatchdog::screenNumber(const QWidget *pWidget) const
{
    /* Redirect call to desktop-widget: */
    return QApplication::desktop()->screenNumber(pWidget);
}

int UIDesktopWidgetWatchdog::screenNumber(const QPoint &point) const
{
    /* Redirect call to desktop-widget: */
    return QApplication::desktop()->screenNumber(point);
}

const QRect UIDesktopWidgetWatchdog::screenGeometry(int iHostScreenIndex /* = -1 */) const
{
    /* Make sure index is valid: */
    if (iHostScreenIndex < 0 || iHostScreenIndex >= screenCount())
        iHostScreenIndex = QApplication::desktop()->primaryScreen();
    AssertReturn(iHostScreenIndex >= 0 && iHostScreenIndex < screenCount(), QRect());

    /* Redirect call to desktop-widget: */
    return QApplication::desktop()->screenGeometry(iHostScreenIndex);
}

const QRect UIDesktopWidgetWatchdog::screenGeometry(const QWidget *pWidget) const
{
    /* Redirect call to wrapper above: */
    return screenGeometry(screenNumber(pWidget));
}

const QRect UIDesktopWidgetWatchdog::screenGeometry(const QPoint &point) const
{
    /* Redirect call to wrapper above: */
    return screenGeometry(screenNumber(point));
}

const QRect UIDesktopWidgetWatchdog::availableGeometry(int iHostScreenIndex /* = -1 */) const
{
    /* Make sure index is valid: */
    if (iHostScreenIndex < 0 || iHostScreenIndex >= screenCount())
        iHostScreenIndex = QApplication::desktop()->primaryScreen();
    AssertReturn(iHostScreenIndex >= 0 && iHostScreenIndex < screenCount(), QRect());

#ifdef VBOX_WS_X11
    /* Get cached available-geometry: */
    const QRect availableGeometry = m_availableGeometryData.value(iHostScreenIndex);
    /* Return cached available-geometry if it's valid or screen-geometry otherwise: */
    return availableGeometry.isValid() ? availableGeometry :
           QApplication::desktop()->screenGeometry(iHostScreenIndex);
#else /* !VBOX_WS_X11 */
    /* Redirect call to desktop-widget: */
    return QApplication::desktop()->availableGeometry(iHostScreenIndex);
#endif /* !VBOX_WS_X11 */
}

const QRect UIDesktopWidgetWatchdog::availableGeometry(const QWidget *pWidget) const
{
    /* Redirect call to wrapper above: */
    return availableGeometry(screenNumber(pWidget));
}

const QRect UIDesktopWidgetWatchdog::availableGeometry(const QPoint &point) const
{
    /* Redirect call to wrapper above: */
    return availableGeometry(screenNumber(point));
}

const QRegion UIDesktopWidgetWatchdog::overallScreenRegion() const
{
    /* Calculate region: */
    QRegion region;
    for (int iScreenIndex = 0; iScreenIndex < gpDesktop->screenCount(); ++iScreenIndex)
    {
        /* Get enumerated screen's available area: */
        QRect rect = gpDesktop->screenGeometry(iScreenIndex);
#ifdef VBOX_WS_WIN
        /* On Windows host window can exceed the available
         * area in maximized/sticky-borders state: */
        rect.adjust(-10, -10, 10, 10);
#endif /* VBOX_WS_WIN */
        /* Append rectangle: */
        region += rect;
    }
    /* Return region: */
    return region;
}

const QRegion UIDesktopWidgetWatchdog::overallAvailableRegion() const
{
    /* Calculate region: */
    QRegion region;
    for (int iScreenIndex = 0; iScreenIndex < gpDesktop->screenCount(); ++iScreenIndex)
    {
        /* Get enumerated screen's available area: */
        QRect rect = gpDesktop->availableGeometry(iScreenIndex);
#ifdef VBOX_WS_WIN
        /* On Windows host window can exceed the available
         * area in maximized/sticky-borders state: */
        rect.adjust(-10, -10, 10, 10);
#endif /* VBOX_WS_WIN */
        /* Append rectangle: */
        region += rect;
    }
    /* Return region: */
    return region;
}

#ifdef VBOX_WS_X11
bool UIDesktopWidgetWatchdog::isFakeScreenDetected() const
{
    // WORKAROUND:
    // In 5.6.1 Qt devs taught the XCB plugin to silently swap last detached screen
    // with a fake one, and there is no API-way to distinguish fake from real one
    // because all they do is erasing output for the last real screen, keeping
    // all other screen attributes stale. Gladly output influencing screen name
    // so we can use that horrible workaround to detect a fake XCB screen.
    return    qApp->screens().size() == 0 /* zero-screen case is impossible after 5.6.1 */
           || (qApp->screens().size() == 1 && qApp->screens().first()->name() == ":0.0");
}
#endif /* VBOX_WS_X11 */

double UIDesktopWidgetWatchdog::devicePixelRatio(int iHostScreenIndex)
{
    /* First, we should check whether the screen is valid: */
    QScreen *pScreen = iHostScreenIndex == -1
                     ? QGuiApplication::primaryScreen()
                     : QGuiApplication::screens().value(iHostScreenIndex);
    AssertPtrReturn(pScreen, 1.0);
    /* Then acquire device-pixel-ratio: */
    return pScreen->devicePixelRatio();
}

double UIDesktopWidgetWatchdog::devicePixelRatio(QWidget *pWidget)
{
    /* Redirect call to wrapper above: */
    return devicePixelRatio(screenNumber(pWidget));
}

void UIDesktopWidgetWatchdog::sltHostScreenAdded(QScreen *pHostScreen)
{
//    printf("UIDesktopWidgetWatchdog::sltHostScreenAdded(%d)\n", screenCount());

    /* Listen for screen signals: */
    connect(pHostScreen, &QScreen::geometryChanged,
            this, &UIDesktopWidgetWatchdog::sltHandleHostScreenResized);
    connect(pHostScreen, &QScreen::availableGeometryChanged,
            this, &UIDesktopWidgetWatchdog::sltHandleHostScreenWorkAreaResized);

# ifdef VBOX_WS_X11
    /* Update host-screen configuration: */
    updateHostScreenConfiguration();
# endif /* VBOX_WS_X11 */

    /* Notify listeners: */
    emit sigHostScreenCountChanged(screenCount());
}

void UIDesktopWidgetWatchdog::sltHostScreenRemoved(QScreen *pHostScreen)
{
//    printf("UIDesktopWidgetWatchdog::sltHostScreenRemoved(%d)\n", screenCount());

    /* Forget about screen signals: */
    disconnect(pHostScreen, &QScreen::geometryChanged,
               this, &UIDesktopWidgetWatchdog::sltHandleHostScreenResized);
    disconnect(pHostScreen, &QScreen::availableGeometryChanged,
               this, &UIDesktopWidgetWatchdog::sltHandleHostScreenWorkAreaResized);

# ifdef VBOX_WS_X11
    /* Update host-screen configuration: */
    updateHostScreenConfiguration();
# endif /* VBOX_WS_X11 */

    /* Notify listeners: */
    emit sigHostScreenCountChanged(screenCount());
}

void UIDesktopWidgetWatchdog::sltHandleHostScreenResized(const QRect &geometry)
{
    /* Get the screen: */
    QScreen *pScreen = sender() ? qobject_cast<QScreen*>(sender()) : 0;
    AssertPtrReturnVoid(pScreen);

    /* Determine screen index: */
    const int iHostScreenIndex = qApp->screens().indexOf(pScreen);
    AssertReturnVoid(iHostScreenIndex != -1);
    LogRel(("GUI: UIDesktopWidgetWatchdog::sltHandleHostScreenResized: "
            "Screen %d is formally resized to: %dx%d x %dx%d\n",
            iHostScreenIndex, geometry.x(), geometry.y(),
            geometry.width(), geometry.height()));

# ifdef VBOX_WS_X11
    /* Update host-screen available-geometry: */
    updateHostScreenAvailableGeometry(iHostScreenIndex);
# endif /* VBOX_WS_X11 */

    /* Notify listeners: */
    emit sigHostScreenResized(iHostScreenIndex);
}

void UIDesktopWidgetWatchdog::sltHandleHostScreenWorkAreaResized(const QRect &availableGeometry)
{
    /* Get the screen: */
    QScreen *pScreen = sender() ? qobject_cast<QScreen*>(sender()) : 0;
    AssertPtrReturnVoid(pScreen);

    /* Determine screen index: */
    const int iHostScreenIndex = qApp->screens().indexOf(pScreen);
    AssertReturnVoid(iHostScreenIndex != -1);
    LogRel(("GUI: UIDesktopWidgetWatchdog::sltHandleHostScreenWorkAreaResized: "
            "Screen %d work area is formally resized to: %dx%d x %dx%d\n",
            iHostScreenIndex, availableGeometry.x(), availableGeometry.y(),
            availableGeometry.width(), availableGeometry.height()));

# ifdef VBOX_WS_X11
    /* Update host-screen available-geometry: */
    updateHostScreenAvailableGeometry(iHostScreenIndex);
# endif /* VBOX_WS_X11 */

    /* Notify listeners: */
    emit sigHostScreenWorkAreaResized(iHostScreenIndex);
}

#ifdef VBOX_WS_X11
void UIDesktopWidgetWatchdog::sltHandleHostScreenAvailableGeometryCalculated(int iHostScreenIndex, QRect availableGeometry)
{
    LogRel(("GUI: UIDesktopWidgetWatchdog::sltHandleHostScreenAvailableGeometryCalculated: "
            "Screen %d work area is actually resized to: %dx%d x %dx%d\n",
            iHostScreenIndex, availableGeometry.x(), availableGeometry.y(),
            availableGeometry.width(), availableGeometry.height()));

    /* Apply received data: */
    const bool fSendSignal = m_availableGeometryData.value(iHostScreenIndex).isValid();
    m_availableGeometryData[iHostScreenIndex] = availableGeometry;
    /* Forget finished worker: */
    AssertPtrReturnVoid(m_availableGeometryWorkers.value(iHostScreenIndex));
    m_availableGeometryWorkers.value(iHostScreenIndex)->disconnect();
    m_availableGeometryWorkers.value(iHostScreenIndex)->deleteLater();
    m_availableGeometryWorkers[iHostScreenIndex] = 0;

    /* Notify listeners: */
    if (fSendSignal)
        emit sigHostScreenWorkAreaRecalculated(iHostScreenIndex);
}
#endif /* VBOX_WS_X11 */

void UIDesktopWidgetWatchdog::prepare()
{
    /* Prepare connections: */
    connect(qApp, &QGuiApplication::screenAdded,
            this, &UIDesktopWidgetWatchdog::sltHostScreenAdded);
    connect(qApp, &QGuiApplication::screenRemoved,
            this, &UIDesktopWidgetWatchdog::sltHostScreenRemoved);
    foreach (QScreen *pHostScreen, qApp->screens())
    {
        connect(pHostScreen, &QScreen::geometryChanged,
                this, &UIDesktopWidgetWatchdog::sltHandleHostScreenResized);
        connect(pHostScreen, &QScreen::availableGeometryChanged,
                this, &UIDesktopWidgetWatchdog::sltHandleHostScreenWorkAreaResized);
    }

#ifdef VBOX_WS_X11
    /* Update host-screen configuration: */
    updateHostScreenConfiguration();
#endif /* VBOX_WS_X11 */
}

void UIDesktopWidgetWatchdog::cleanup()
{
    /* Cleanup connections: */
    disconnect(qApp, &QGuiApplication::screenAdded,
               this, &UIDesktopWidgetWatchdog::sltHostScreenAdded);
    disconnect(qApp, &QGuiApplication::screenRemoved,
               this, &UIDesktopWidgetWatchdog::sltHostScreenRemoved);
    foreach (QScreen *pHostScreen, qApp->screens())
    {
        disconnect(pHostScreen, &QScreen::geometryChanged,
                   this, &UIDesktopWidgetWatchdog::sltHandleHostScreenResized);
        disconnect(pHostScreen, &QScreen::availableGeometryChanged,
                   this, &UIDesktopWidgetWatchdog::sltHandleHostScreenWorkAreaResized);
    }

#ifdef VBOX_WS_X11
    /* Cleanup existing workers finally: */
    cleanupExistingWorkers();
#endif /* VBOX_WS_X11 */
}

#ifdef VBOX_WS_X11
void UIDesktopWidgetWatchdog::updateHostScreenConfiguration(int cHostScreenCount /* = -1 */)
{
    /* Acquire new host-screen count: */
    if (cHostScreenCount == -1)
        cHostScreenCount = screenCount();

    /* Cleanup existing workers first: */
    cleanupExistingWorkers();

    /* Resize workers vectors to new host-screen count: */
    m_availableGeometryWorkers.resize(cHostScreenCount);
    m_availableGeometryData.resize(cHostScreenCount);

    /* Update host-screen available-geometry for each particular host-screen: */
    for (int iHostScreenIndex = 0; iHostScreenIndex < cHostScreenCount; ++iHostScreenIndex)
        updateHostScreenAvailableGeometry(iHostScreenIndex);
}

void UIDesktopWidgetWatchdog::updateHostScreenAvailableGeometry(int iHostScreenIndex)
{
    /* Make sure index is valid: */
    if (iHostScreenIndex < 0 || iHostScreenIndex >= screenCount())
        iHostScreenIndex = QApplication::desktop()->primaryScreen();
    AssertReturnVoid(iHostScreenIndex >= 0 && iHostScreenIndex < screenCount());

    /* Create invisible frame-less window worker: */
    UIInvisibleWindow *pWorker = new UIInvisibleWindow(iHostScreenIndex);
    AssertPtrReturnVoid(pWorker);
    {
        /* Remember created worker (replace if necessary): */
        if (m_availableGeometryWorkers.value(iHostScreenIndex))
            delete m_availableGeometryWorkers.value(iHostScreenIndex);
        m_availableGeometryWorkers[iHostScreenIndex] = pWorker;

        /* Get the screen-geometry: */
        const QRect hostScreenGeometry = screenGeometry(iHostScreenIndex);

        /* Connect worker listener: */
        connect(pWorker, &UIInvisibleWindow::sigHostScreenAvailableGeometryCalculated,
                this, &UIDesktopWidgetWatchdog::sltHandleHostScreenAvailableGeometryCalculated);

        /* Place worker to corresponding host-screen: */
        pWorker->move(hostScreenGeometry.center());
        /* And finally, maximize it: */
        pWorker->showMaximized();
    }
}

void UIDesktopWidgetWatchdog::cleanupExistingWorkers()
{
    /* Destroy existing workers: */
    qDeleteAll(m_availableGeometryWorkers);
    /* And clear their vector: */
    m_availableGeometryWorkers.clear();
}

# include "UIDesktopWidgetWatchdog.moc"
#endif /* VBOX_WS_X11 */

