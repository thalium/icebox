/* $Id: UIMachineWindow.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineWindow class implementation.
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
# include <QCloseEvent>
# include <QProcess>
# include <QTimer>

/* GUI includes: */
# include "VBoxGlobal.h"
# include "UIConverter.h"
# include "UIModalWindowManager.h"
# include "UIExtraDataManager.h"
# include "UIMessageCenter.h"
# include "UISession.h"
# include "UIMachineLogic.h"
# include "UIMachineWindow.h"
# include "UIMachineWindowNormal.h"
# include "UIMachineWindowFullscreen.h"
# include "UIMachineWindowSeamless.h"
# include "UIMachineWindowScale.h"
# include "UIMachineView.h"
# include "UIKeyboardHandler.h"
# include "UIMouseHandler.h"
# include "UIVMCloseDialog.h"

/* COM includes: */
# include "CConsole.h"
# include "CSnapshot.h"

/* Other VBox includes: */
# include <VBox/version.h>
# ifdef VBOX_BLEEDING_EDGE
#  include <iprt/buildconfig.h>
# endif /* VBOX_BLEEDING_EDGE */

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* static */
UIMachineWindow* UIMachineWindow::create(UIMachineLogic *pMachineLogic, ulong uScreenId)
{
    /* Create machine-window: */
    UIMachineWindow *pMachineWindow = 0;
    switch (pMachineLogic->visualStateType())
    {
        case UIVisualStateType_Normal:
            pMachineWindow = new UIMachineWindowNormal(pMachineLogic, uScreenId);
            break;
        case UIVisualStateType_Fullscreen:
            pMachineWindow = new UIMachineWindowFullscreen(pMachineLogic, uScreenId);
            break;
        case UIVisualStateType_Seamless:
            pMachineWindow = new UIMachineWindowSeamless(pMachineLogic, uScreenId);
            break;
        case UIVisualStateType_Scale:
            pMachineWindow = new UIMachineWindowScale(pMachineLogic, uScreenId);
            break;
        default:
            AssertMsgFailed(("Incorrect visual state!"));
            break;
    }
    /* Prepare machine-window: */
    pMachineWindow->prepare();
    /* Return machine-window: */
    return pMachineWindow;
}

/* static */
void UIMachineWindow::destroy(UIMachineWindow *pWhichWindow)
{
    /* Cleanup machine-window: */
    pWhichWindow->cleanup();
    /* Delete machine-window: */
    delete pWhichWindow;
}

void UIMachineWindow::prepare()
{
    /* Prepare session-connections: */
    prepareSessionConnections();

    /* Prepare main-layout: */
    prepareMainLayout();

    /* Prepare menu: */
    prepareMenu();

    /* Prepare status-bar: */
    prepareStatusBar();

    /* Prepare visual-state: */
    prepareVisualState();

    /* Prepare machine-view: */
    prepareMachineView();

    /* Prepare handlers: */
    prepareHandlers();

    /* Load settings: */
    loadSettings();

    /* Retranslate window: */
    retranslateUi();

    /* Show (must be done before updating the appearance): */
    showInNecessaryMode();

    /* Update all the elements: */
    updateAppearanceOf(UIVisualElement_AllStuff);

#ifdef VBOX_WS_X11
    /* Prepare default class/name values: */
    const QString strWindowClass = QString("VirtualBox Machine");
    QString strWindowName = strWindowClass;
    /* Check if we want Window Manager to distinguish Virtual Machine windows: */
    if (gEDataManager->distinguishMachineWindowGroups(vboxGlobal().managedVMUuid()))
        strWindowName = QString("VirtualBox Machine UUID: %1").arg(vboxGlobal().managedVMUuid());
    /* Assign WM_CLASS property: */
    VBoxGlobal::setWMClass(this, strWindowName, strWindowClass);
#endif
}

void UIMachineWindow::cleanup()
{
    /* Save window settings: */
    saveSettings();

    /* Cleanup handlers: */
    cleanupHandlers();

    /* Cleanup visual-state: */
    cleanupVisualState();

    /* Cleanup machine-view: */
    cleanupMachineView();

    /* Cleanup status-bar: */
    cleanupStatusBar();

    /* Cleanup menu: */
    cleanupMenu();

    /* Cleanup main layout: */
    cleanupMainLayout();

    /* Cleanup session connections: */
    cleanupSessionConnections();
}

void UIMachineWindow::sltMachineStateChanged()
{
    /* Update window-title: */
    updateAppearanceOf(UIVisualElement_WindowTitle);
}

UIMachineWindow::UIMachineWindow(UIMachineLogic *pMachineLogic, ulong uScreenId)
    : QIWithRetranslateUI2<QMainWindow>(0, pMachineLogic->windowFlags(uScreenId))
    , m_pMachineLogic(pMachineLogic)
    , m_pMachineView(0)
    , m_uScreenId(uScreenId)
    , m_pMainLayout(0)
    , m_pTopSpacer(0)
    , m_pBottomSpacer(0)
    , m_pLeftSpacer(0)
    , m_pRightSpacer(0)
{
#ifndef VBOX_WS_MAC
    /* Set machine-window icon if any: */
    // On macOS window icon is referenced in info.plist.
    if (uisession() && uisession()->machineWindowIcon())
        setWindowIcon(*uisession()->machineWindowIcon());
#endif /* !VBOX_WS_MAC */
}

UIActionPool* UIMachineWindow::actionPool() const
{
    return machineLogic()->actionPool();
}

UISession* UIMachineWindow::uisession() const
{
    return machineLogic()->uisession();
}

CSession& UIMachineWindow::session() const
{
    return uisession()->session();
}

CMachine& UIMachineWindow::machine() const
{
    return uisession()->machine();
}

CConsole& UIMachineWindow::console() const
{
    return uisession()->console();
}

const QString& UIMachineWindow::machineName() const
{
    return uisession()->machineName();
}

void UIMachineWindow::adjustMachineViewSize()
{
    /* We need to adjust guest-screen size if necessary: */
    machineView()->adjustGuestScreenSize();
}

void UIMachineWindow::sendMachineViewSizeHint()
{
    /* Send machine-view size-hint to the guest: */
    machineView()->resendSizeHint();
}

#ifdef VBOX_WITH_MASKED_SEAMLESS
void UIMachineWindow::setMask(const QRegion &region)
{
    /* Call to base-class: */
    QMainWindow::setMask(region);
}
#endif /* VBOX_WITH_MASKED_SEAMLESS */

void UIMachineWindow::retranslateUi()
{
    /* Compose window-title prefix: */
    m_strWindowTitlePrefix = VBOX_PRODUCT;
#ifdef VBOX_BLEEDING_EDGE
    m_strWindowTitlePrefix += UIMachineWindow::tr(" EXPERIMENTAL build %1r%2 - %3")
                              .arg(RTBldCfgVersion())
                              .arg(RTBldCfgRevisionStr())
                              .arg(VBOX_BLEEDING_EDGE);
#endif /* VBOX_BLEEDING_EDGE */
    /* Update appearance of the window-title: */
    updateAppearanceOf(UIVisualElement_WindowTitle);
}

void UIMachineWindow::showEvent(QShowEvent *pShowEvent)
{
    /* Call to base class: */
    QMainWindow::showEvent(pShowEvent);

    /* Update appearance for indicator-pool: */
    updateAppearanceOf(UIVisualElement_IndicatorPoolStuff);
}

void UIMachineWindow::closeEvent(QCloseEvent *pCloseEvent)
{
    /* Always ignore close-event first: */
    pCloseEvent->ignore();

    /* Make sure machine is in one of the allowed states: */
    if (!uisession()->isRunning() && !uisession()->isPaused() && !uisession()->isStuck())
        return;

    /* If there is a close hook script defined: */
    const QString strScript = gEDataManager->machineCloseHookScript(vboxGlobal().managedVMUuid());
    if (!strScript.isEmpty())
    {
        /* Execute asynchronously and leave: */
        QProcess::startDetached(strScript, QStringList() << machine().GetId());
        return;
    }

    /* Choose the close action: */
    MachineCloseAction closeAction = MachineCloseAction_Invalid;

    /* If default close-action defined and not restricted: */
    MachineCloseAction defaultCloseAction = uisession()->defaultCloseAction();
    MachineCloseAction restrictedCloseActions = uisession()->restrictedCloseActions();
    if ((defaultCloseAction != MachineCloseAction_Invalid) &&
        !(restrictedCloseActions & defaultCloseAction))
    {
        switch (defaultCloseAction)
        {
            /* If VM is stuck, and the default close-action is 'detach', 'save-state' or 'shutdown',
             * we should ask the user about what to do: */
            case MachineCloseAction_Detach:
            case MachineCloseAction_SaveState:
            case MachineCloseAction_Shutdown:
                closeAction = uisession()->isStuck() ? MachineCloseAction_Invalid : defaultCloseAction;
                break;
            /* Otherwise we just use what we have: */
            default:
                closeAction = defaultCloseAction;
                break;
        }
    }

    /* If the close-action still undefined: */
    if (closeAction == MachineCloseAction_Invalid)
    {
        /* Prepare close-dialog: */
        QWidget *pParentDlg = windowManager().realParentWindow(this);
        QPointer<UIVMCloseDialog> pCloseDlg = new UIVMCloseDialog(pParentDlg, machine(),
                                                                  console().GetGuestEnteredACPIMode(),
                                                                  restrictedCloseActions);
        /* Configure close-dialog: */
        if (uisession() && uisession()->machineWindowIcon())
        {
            const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize);
            pCloseDlg->setPixmap(uisession()->machineWindowIcon()->pixmap(QSize(iIconMetric, iIconMetric)));
        }

        /* Make sure close-dialog is valid: */
        if (pCloseDlg->isValid())
        {
            /* We are going to show close-dialog: */
            bool fShowCloseDialog = true;
            /* Check if VM is paused or stuck: */
            const bool fWasPaused = uisession()->isPaused();
            const bool fIsStuck = uisession()->isStuck();
            /* If VM is NOT paused and NOT stuck: */
            if (!fWasPaused && !fIsStuck)
            {
                /* We should pause it first: */
                const bool fIsPaused = uisession()->pause();
                /* If we were unable to pause VM: */
                if (!fIsPaused)
                {
                    /* If that is NOT the separate VM process UI: */
                    if (!vboxGlobal().isSeparateProcess())
                    {
                        /* We are not going to show close-dialog: */
                        fShowCloseDialog = false;
                    }
                    /* If that is the separate VM process UI: */
                    else
                    {
                        /* We are going to show close-dialog only
                         * if headless frontend stopped/killed already: */
                        CMachine machine = uisession()->machine();
                        KMachineState machineState = machine.GetState();
                        fShowCloseDialog = !machine.isOk() || machineState == KMachineState_Null;
                    }
                }
            }
            /* If we are going to show close-dialog: */
            if (fShowCloseDialog)
            {
                /* Show close-dialog to let the user make the choice: */
                windowManager().registerNewParent(pCloseDlg, pParentDlg);
                closeAction = static_cast<MachineCloseAction>(pCloseDlg->exec());

                /* Make sure the dialog still valid: */
                if (!pCloseDlg)
                    return;

                /* If VM was not paused before but paused now,
                 * we should resume it if user canceled dialog or chosen shutdown: */
                if (!fWasPaused && uisession()->isPaused() &&
                    (closeAction == MachineCloseAction_Invalid ||
                     closeAction == MachineCloseAction_Detach ||
                     closeAction == MachineCloseAction_Shutdown))
                {
                    /* If we unable to resume VM, cancel closing: */
                    if (!uisession()->unpause())
                        closeAction = MachineCloseAction_Invalid;
                }
            }
        }
        else
        {
            /* Else user misconfigured .vbox file, we will reject closing UI: */
            closeAction = MachineCloseAction_Invalid;
        }

        /* Cleanup close-dialog: */
        delete pCloseDlg;
    }

    /* Depending on chosen result: */
    switch (closeAction)
    {
        case MachineCloseAction_Detach:
        {
            /* Just close Runtime UI: */
            LogRel(("GUI: Request for close-action to detach GUI.\n"));
            machineLogic()->detach();
            break;
        }
        case MachineCloseAction_SaveState:
        {
            /* Save VM state: */
            LogRel(("GUI: Request for close-action to save VM state.\n"));
            machineLogic()->saveState();
            break;
        }
        case MachineCloseAction_Shutdown:
        {
            /* Shutdown VM: */
            LogRel(("GUI: Request for close-action to shutdown VM.\n"));
            machineLogic()->shutdown();
            break;
        }
        case MachineCloseAction_PowerOff:
        case MachineCloseAction_PowerOff_RestoringSnapshot:
        {
            /* Power VM off: */
            LogRel(("GUI: Request for close-action to power VM off.\n"));
            machineLogic()->powerOff(closeAction == MachineCloseAction_PowerOff_RestoringSnapshot);
            break;
        }
        default:
            break;
    }
}

void UIMachineWindow::prepareSessionConnections()
{
    /* We should watch for console events: */
    connect(uisession(), SIGNAL(sigMachineStateChange()), this, SLOT(sltMachineStateChanged()));
}

void UIMachineWindow::prepareMainLayout()
{
    /* Create central-widget: */
    setCentralWidget(new QWidget);

    /* Create main-layout: */
    m_pMainLayout = new QGridLayout(centralWidget());
    m_pMainLayout->setMargin(0);
    m_pMainLayout->setSpacing(0);

    /* Create shifting-spacers: */
    m_pTopSpacer = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_pBottomSpacer = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_pLeftSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_pRightSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);

    /* Add shifting-spacers into main-layout: */
    m_pMainLayout->addItem(m_pTopSpacer, 0, 1);
    m_pMainLayout->addItem(m_pBottomSpacer, 2, 1);
    m_pMainLayout->addItem(m_pLeftSpacer, 1, 0);
    m_pMainLayout->addItem(m_pRightSpacer, 1, 2);
}

void UIMachineWindow::prepareMachineView()
{
#ifdef VBOX_WITH_VIDEOHWACCEL
    /* Need to force the QGL framebuffer in case 2D Video Acceleration is supported & enabled: */
    bool bAccelerate2DVideo = machine().GetAccelerate2DVideoEnabled() && VBoxGlobal::isAcceleration2DVideoAvailable();
#endif /* VBOX_WITH_VIDEOHWACCEL */

    /* Get visual-state type: */
    UIVisualStateType visualStateType = machineLogic()->visualStateType();

    /* Create machine-view: */
    m_pMachineView = UIMachineView::create(  this
                                           , m_uScreenId
                                           , visualStateType
#ifdef VBOX_WITH_VIDEOHWACCEL
                                           , bAccelerate2DVideo
#endif /* VBOX_WITH_VIDEOHWACCEL */
                                           );

    /* Listen for frame-buffer resize: */
    connect(m_pMachineView, SIGNAL(sigFrameBufferResize()), this, SIGNAL(sigFrameBufferResize()));

    /* Add machine-view into main-layout: */
    m_pMainLayout->addWidget(m_pMachineView, 1, 1, viewAlignment(visualStateType));

    /* Install focus-proxy: */
    setFocusProxy(m_pMachineView);
}

void UIMachineWindow::prepareHandlers()
{
    /* Register keyboard-handler: */
    machineLogic()->keyboardHandler()->prepareListener(m_uScreenId, this);

    /* Register mouse-handler: */
    machineLogic()->mouseHandler()->prepareListener(m_uScreenId, this);
}

void UIMachineWindow::cleanupHandlers()
{
    /* Unregister mouse-handler: */
    machineLogic()->mouseHandler()->cleanupListener(m_uScreenId);

    /* Unregister keyboard-handler: */
    machineLogic()->keyboardHandler()->cleanupListener(m_uScreenId);
}

void UIMachineWindow::cleanupMachineView()
{
    /* Destroy machine-view: */
    UIMachineView::destroy(m_pMachineView);
    m_pMachineView = 0;
}

void UIMachineWindow::cleanupSessionConnections()
{
    /* We should stop watching for console events: */
    disconnect(uisession(), SIGNAL(sigMachineStateChange()), this, SLOT(sltMachineStateChanged()));
}

void UIMachineWindow::updateAppearanceOf(int iElement)
{
    /* Update window title: */
    if (iElement & UIVisualElement_WindowTitle)
    {
        /* Get machine state: */
        KMachineState state = uisession()->machineState();
        /* Prepare full name: */
        QString strSnapshotName;
        if (machine().GetSnapshotCount() > 0)
        {
            CSnapshot snapshot = machine().GetCurrentSnapshot();
            strSnapshotName = " (" + snapshot.GetName() + ")";
        }
        QString strMachineName = machineName() + strSnapshotName;
        if (state != KMachineState_Null)
            strMachineName += " [" + gpConverter->toString(state) + "]";
        /* Unusual on the Mac. */
#ifndef VBOX_WS_MAC
        const QString strUserProductName = uisession()->machineWindowNamePostfix();
        strMachineName += " - " + (strUserProductName.isEmpty() ? defaultWindowTitle() : strUserProductName);
#endif /* !VBOX_WS_MAC */
        if (machine().GetMonitorCount() > 1)
            strMachineName += QString(" : %1").arg(m_uScreenId + 1);
        setWindowTitle(strMachineName);
    }
}

#ifdef VBOX_WITH_DEBUGGER_GUI
void UIMachineWindow::updateDbgWindows()
{
    /* The debugger windows are bind to the main VM window. */
    if (m_uScreenId == 0)
        machineLogic()->dbgAdjustRelativePos();
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

/* static */
Qt::Alignment UIMachineWindow::viewAlignment(UIVisualStateType visualStateType)
{
    switch (visualStateType)
    {
        case UIVisualStateType_Normal: return 0;
        case UIVisualStateType_Fullscreen: return Qt::AlignVCenter | Qt::AlignHCenter;
        case UIVisualStateType_Seamless: return 0;
        case UIVisualStateType_Scale: return 0;
        case UIVisualStateType_Invalid: case UIVisualStateType_All: break; /* Shut up, MSC! */
    }
    AssertMsgFailed(("Incorrect visual state!"));
    return 0;
}

#ifdef VBOX_WS_MAC
void UIMachineWindow::handleStandardWindowButtonCallback(StandardWindowButtonType enmButtonType, bool fWithOptionKey)
{
    switch (enmButtonType)
    {
        case StandardWindowButtonType_Zoom:
        {
            /* Handle 'Zoom' button for 'Normal' and 'Scaled' modes: */
            if (   machineLogic()->visualStateType() == UIVisualStateType_Normal
                || machineLogic()->visualStateType() == UIVisualStateType_Scale)
            {
                if (fWithOptionKey)
                {
                    /* Toggle window zoom: */
                    darwinToggleWindowZoom(this);
                }
                else
                {
                    /* Enter 'full-screen' mode: */
                    uisession()->setRequestedVisualState(UIVisualStateType_Invalid);
                    uisession()->changeVisualState(UIVisualStateType_Fullscreen);
                }
            }
            break;
        }
        default:
            break;
    }
}

/* static */
void UIMachineWindow::handleNativeNotification(const QString &strNativeNotificationName, QWidget *pWidget)
{
    /* Handle arrived notification: */
    LogRel(("GUI: UIMachineWindow::handleNativeNotification: Notification '%s' received\n",
            strNativeNotificationName.toLatin1().constData()));
    AssertPtrReturnVoid(pWidget);
    if (UIMachineWindow *pMachineWindow = qobject_cast<UIMachineWindow*>(pWidget))
    {
        /* Redirect arrived notification: */
        LogRel2(("UIMachineWindow::handleNativeNotification: Redirecting '%s' notification to corresponding machine-window...\n",
                 strNativeNotificationName.toLatin1().constData()));
        pMachineWindow->handleNativeNotification(strNativeNotificationName);
    }
}

/* static */
void UIMachineWindow::handleStandardWindowButtonCallback(StandardWindowButtonType enmButtonType, bool fWithOptionKey, QWidget *pWidget)
{
    /* Handle arrived callback: */
    LogRel(("GUI: UIMachineWindow::handleStandardWindowButtonCallback: Callback for standard window button '%d' with option key '%d' received\n",
            (int)enmButtonType, (int)fWithOptionKey));
    AssertPtrReturnVoid(pWidget);
    if (UIMachineWindow *pMachineWindow = qobject_cast<UIMachineWindow*>(pWidget))
    {
        /* Redirect arrived callback: */
        LogRel2(("UIMachineWindow::handleStandardWindowButtonCallback: Redirecting callback for standard window button '%d' with option key '%d' to corresponding machine-window...\n",
                 (int)enmButtonType, (int)fWithOptionKey));
        pMachineWindow->handleStandardWindowButtonCallback(enmButtonType, fWithOptionKey);
    }
}
#endif /* VBOX_WS_MAC */
