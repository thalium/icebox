/* $Id: UISelectorWindow.cpp $ */
/** @file
 * VBox Qt GUI - UISelectorWindow class implementation.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
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
# include <QMenuBar>
# include <QResizeEvent>
# include <QStandardPaths>
# include <QStatusBar>
# include <QToolButton>
# include <QTimer>

/* GUI includes: */
# include "QIFileDialog.h"
# include "QISplitter.h"
# include "UIActionPoolSelector.h"
# include "UIDesktopServices.h"
# include "UIErrorString.h"
# include "UIExtraDataManager.h"
# include "UIGChooser.h"
# include "UIGlobalSettingsExtension.h"
# include "UIHostNetworkManager.h"
# include "UIMedium.h"
# include "UIMediumManager.h"
# include "UIMessageCenter.h"
# include "UIModalWindowManager.h"
# include "UISelectorWindow.h"
# include "UISettingsDialogSpecific.h"
# include "UISlidingWidget.h"
# include "UITabBar.h"
# include "UIToolBar.h"
# include "UIVMLogViewer.h"
# include "UIVMItem.h"
# include "UIToolsPaneMachine.h"
# include "UIToolsToolbar.h"
# include "UIVirtualBoxEventHandler.h"
# include "UIWizardCloneVM.h"
# include "UIWizardExportApp.h"
# include "UIWizardImportApp.h"
# ifdef VBOX_GUI_WITH_NETWORK_MANAGER
#  include "UINetworkManager.h"
#  include "UINetworkManagerIndicator.h"
# endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
# ifdef VBOX_WS_MAC
#  include "UIImageTools.h"
#  include "UIWindowMenuManager.h"
#  include "VBoxUtils.h"
# endif /* VBOX_WS_MAC */
# ifdef VBOX_WS_X11
#  include "UIDesktopWidgetWatchdog.h"
# endif
# ifndef VBOX_WS_MAC
#  include "UIMenuBar.h"
# endif

/* Other VBox stuff: */
# include <iprt/buildconfig.h>
# include <VBox/version.h>
# ifdef VBOX_WS_X11
#  include <iprt/env.h>
# endif /* VBOX_WS_X11 */

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* static */
UISelectorWindow* UISelectorWindow::m_spInstance = 0;

/* static */
void UISelectorWindow::create()
{
    /* Return if selector-window is already created: */
    if (m_spInstance)
        return;

    /* Create selector-window: */
    new UISelectorWindow;
    /* Prepare selector-window: */
    m_spInstance->prepare();
    /* Show selector-window: */
    m_spInstance->show();
}

/* static */
void UISelectorWindow::destroy()
{
    /* Make sure selector-window is created: */
    AssertPtrReturnVoid(m_spInstance);

    /* Cleanup selector-window: */
    m_spInstance->cleanup();
    /* Destroy machine UI: */
    delete m_spInstance;
}

UISelectorWindow::UISelectorWindow()
    : m_fPolished(false)
    , m_fWarningAboutInaccessibleMediaShown(false)
    , m_pActionPool(0)
    , m_pSlidingWidget(0)
    , m_pSplitter(0)
    , m_pToolBar(0)
    , m_pTabBarMachine(0)
    , m_pTabBarGlobal(0)
    , m_pActionTabBarMachine(0)
    , m_pActionTabBarGlobal(0)
    , m_pToolbarTools(0)
    , m_pPaneChooser(0)
    , m_pPaneToolsMachine(0)
    , m_pPaneToolsGlobal(0)
    , m_pGroupMenuAction(0)
    , m_pMachineMenuAction(0)
    , m_pManagerVirtualMedia(0)
    , m_pManagerHostNetwork(0)
{
    m_spInstance = this;
}

UISelectorWindow::~UISelectorWindow()
{
    m_spInstance = 0;
}

bool UISelectorWindow::shouldBeMaximized() const
{
    return gEDataManager->selectorWindowShouldBeMaximized();
}

void UISelectorWindow::sltHandlePolishEvent()
{
    /* Get current item: */
    UIVMItem *pItem = currentItem();

    /* Make sure there is accessible VM item chosen: */
    if (pItem && pItem->accessible())
    {
        // WORKAROUND:
        // By some reason some of X11 DEs unable to update() tab-bars on startup.
        // Let's just _create_ them later, asynchronously after the showEvent().
        /* Restore previously opened Machine tools at startup: */
        QMap<ToolTypeMachine, QAction*> mapActionsMachine;
        mapActionsMachine[ToolTypeMachine_Details] = actionPool()->action(UIActionIndexST_M_Tools_M_Machine_S_Details);
        mapActionsMachine[ToolTypeMachine_Snapshots] = actionPool()->action(UIActionIndexST_M_Tools_M_Machine_S_Snapshots);
        for (int i = m_orderMachine.size() - 1; i >= 0; --i)
            if (m_orderMachine.at(i) != ToolTypeMachine_Invalid)
                mapActionsMachine.value(m_orderMachine.at(i))->trigger();
        /* Make sure further action triggering cause tool type switch as well: */
        actionPool()->action(UIActionIndexST_M_Tools_T_Machine)->setProperty("watch_child_activation", true);
    }
}

#ifdef VBOX_WS_X11
void UISelectorWindow::sltHandleHostScreenAvailableAreaChange()
{
    /* Prevent handling if fake screen detected: */
    if (gpDesktop->isFakeScreenDetected())
        return;

    /* Restore the geometry cached by the window: */
    resize(m_geometry.size());
    move(m_geometry.topLeft());
}
#endif /* VBOX_WS_X11 */

void UISelectorWindow::sltShowSelectorWindowContextMenu(const QPoint &position)
{
    /* Populate toolbar/statusbar acctions: */
    QList<QAction*> actions;

    /* Create 'Show Toolbar' action: */
    QAction *pShowToolBar = new QAction(tr("Show Toolbar"), 0);
    AssertPtrReturnVoid(pShowToolBar);
    {
        /* Configure action: */
        pShowToolBar->setCheckable(true);
        pShowToolBar->setChecked(m_pToolBar->isVisible());

        /* Add into action list: */
        actions << pShowToolBar;
    }

    /* Create 'Show Toolbar Text' action: */
    QAction *pShowToolBarText = new QAction(tr("Show Toolbar Text"), 0);
    AssertPtrReturnVoid(pShowToolBarText);
    {
        /* Configure action: */
        pShowToolBarText->setCheckable(true);
        pShowToolBarText->setChecked(m_pToolBar->toolButtonStyle() == Qt::ToolButtonTextUnderIcon);

        /* Add into action list: */
        actions << pShowToolBarText;
    }

    /* Create 'Show Statusbar' action: */
    QAction *pShowStatusBar = new QAction(tr("Show Statusbar"), 0);
    AssertPtrReturnVoid(pShowStatusBar);
    {
        /* Configure action: */
        pShowStatusBar->setCheckable(true);
        pShowStatusBar->setChecked(statusBar()->isVisible());

        /* Add into action list: */
        actions << pShowStatusBar;
    }

    /* Prepare the menu position: */
    QPoint globalPosition = position;
    QWidget *pSender = static_cast<QWidget*>(sender());
    if (pSender)
        globalPosition = pSender->mapToGlobal(position);
    QAction *pResult = QMenu::exec(actions, globalPosition);
    if (pResult == pShowToolBar)
    {
        if (pResult->isChecked())
            m_pToolBar->show();
        else
            m_pToolBar->hide();
    }
    else if (pResult == pShowToolBarText)
    {
        m_pToolBar->setToolButtonStyle(pResult->isChecked()
                                       ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
        m_pToolbarTools->setToolButtonStyle(pResult->isChecked()
                                            ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
    }
    else if (pResult == pShowStatusBar)
    {
        if (pResult->isChecked())
            statusBar()->show();
        else
            statusBar()->hide();
    }
}

void UISelectorWindow::sltHandleChooserPaneIndexChange(bool fUpdateDetails /* = true */,
                                                       bool fUpdateSnapshots /* = true */)
{
    /* Get current item: */
    UIVMItem *pItem = currentItem();

    /* Update action visibility: */
    updateActionsVisibility();
    /* Update action appearance: */
    updateActionsAppearance();

    /* Update Tools-pane: */
    m_pPaneToolsMachine->setCurrentItem(pItem);

    /* Update Machine tab-bar visibility */
    m_pTabBarMachine->setEnabled(pItem && pItem->accessible());

    /* If current item exists & accessible: */
    if (pItem && pItem->accessible())
    {
        /* If Desktop pane is chosen currently: */
        if (m_pPaneToolsMachine->currentTool() == ToolTypeMachine_Desktop)
        {
            /* Make sure Details or Snapshot pane is chosen if opened: */
            if (m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Details))
                actionPool()->action(UIActionIndexST_M_Tools_M_Machine_S_Details)->trigger();
            else
            if (m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Snapshots))
                actionPool()->action(UIActionIndexST_M_Tools_M_Machine_S_Snapshots)->trigger();
        }

        /* Update Details-pane (if requested): */
        if (   fUpdateDetails
            && m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Details))
            m_pPaneToolsMachine->setItems(currentItems());
        /* Update Snapshots-pane (if requested): */
        if (   fUpdateSnapshots
            && m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Snapshots))
            m_pPaneToolsMachine->setMachine(pItem->machine());
    }
    else
    {
        /* Make sure Desktop-pane raised: */
        m_pPaneToolsMachine->openTool(ToolTypeMachine_Desktop);

        /* Note that the machine becomes inaccessible (or if the last VM gets
         * deleted), we have to update all fields, ignoring input arguments. */
        if (pItem)
        {
            /* The VM is inaccessible: */
            m_pPaneToolsMachine->setDetailsError(UIErrorString::formatErrorInfo(pItem->accessError()));
        }

        /* Update Details-pane (in any case): */
        if (m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Details))
            m_pPaneToolsMachine->setItems(currentItems());
        /* Update Snapshots-pane (in any case): */
        if (m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Snapshots))
            m_pPaneToolsMachine->setMachine(CMachine());
    }
}

void UISelectorWindow::sltHandleMediumEnumerationFinish()
{
    /* We try to warn about inaccessible mediums only once
     * (after media emumeration started from main() at startup),
     * to avoid annoying the user: */
    if (m_fWarningAboutInaccessibleMediaShown)
        return;
    m_fWarningAboutInaccessibleMediaShown = true;

    /* Make sure MM window is not opened: */
    if (   m_pManagerVirtualMedia
        || m_pPaneToolsGlobal->isToolOpened(ToolTypeGlobal_VirtualMedia))
        return;

    /* Look for at least one inaccessible medium: */
    bool fIsThereAnyInaccessibleMedium = false;
    foreach (const QString &strMediumID, vboxGlobal().mediumIDs())
    {
        if (vboxGlobal().medium(strMediumID).state() == KMediumState_Inaccessible)
        {
            fIsThereAnyInaccessibleMedium = true;
            break;
        }
    }

    /* Warn the user about inaccessible medium: */
    if (fIsThereAnyInaccessibleMedium && !msgCenter().warnAboutInaccessibleMedia())
    {
        /* Open the MM window: */
        sltOpenVirtualMediumManagerWindow();
    }
}

void UISelectorWindow::sltOpenUrls(QList<QUrl> list /* = QList<QUrl>() */)
{
    /* Make sure any pending D&D events are consumed. */
    /// @todo What? So dangerous method for so cheap purpose?
    qApp->processEvents();

    if (list.isEmpty())
    {
        list = vboxGlobal().argUrlList();
        vboxGlobal().argUrlList().clear();
    }
    /* Check if we are can handle the dropped urls. */
    for (int i = 0; i < list.size(); ++i)
    {
#ifdef VBOX_WS_MAC
        QString strFile = ::darwinResolveAlias(list.at(i).toLocalFile());
#else /* VBOX_WS_MAC */
        QString strFile = list.at(i).toLocalFile();
#endif /* !VBOX_WS_MAC */
        if (!strFile.isEmpty() && QFile::exists(strFile))
        {
            if (VBoxGlobal::hasAllowedExtension(strFile, VBoxFileExts))
            {
                /* VBox config files. */
                CVirtualBox vbox = vboxGlobal().virtualBox();
                CMachine machine = vbox.FindMachine(strFile);
                if (!machine.isNull())
                {
                    CVirtualBox vbox = vboxGlobal().virtualBox();
                    CMachine machine = vbox.FindMachine(strFile);
                    if (!machine.isNull())
                        vboxGlobal().launchMachine(machine);
                }
                else
                    sltOpenAddMachineDialog(strFile);
            }
            else if (VBoxGlobal::hasAllowedExtension(strFile, OVFFileExts))
            {
                /* OVF/OVA. Only one file at the time. */
                sltOpenImportApplianceWizard(strFile);
                break;
            }
            else if (VBoxGlobal::hasAllowedExtension(strFile, VBoxExtPackFileExts))
            {
                /* Prevent update manager from proposing us to update EP: */
                vboxGlobal().setEPInstallationRequested(true);
                /* Propose the user to install EP described by the arguments @a list. */
                UIGlobalSettingsExtension::doInstallation(strFile, QString(), this, NULL);
                /* Allow update manager to propose us to update EP: */
                vboxGlobal().setEPInstallationRequested(false);
            }
        }
    }
}

void UISelectorWindow::sltHandleGroupSavingProgressChange()
{
    updateActionsAppearance();
}

#ifdef VBOX_WS_MAC
void UISelectorWindow::sltActionHovered(UIAction *pAction)
{
    /* Show the action message for a ten seconds: */
    statusBar()->showMessage(pAction->statusTip(), 10000);
}
#endif /* VBOX_WS_MAC */

void UISelectorWindow::sltHandleStateChange(QString)
{
    /* Get current item: */
    UIVMItem *pItem = currentItem();

    /* Make sure current item present: */
    if (!pItem)
        return;

    /* Update actions: */
    updateActionsAppearance();
}

void UISelectorWindow::sltOpenVirtualMediumManagerWindow()
{
    /* First check if instance of widget opened embedded: */
    if (m_pPaneToolsGlobal->isToolOpened(ToolTypeGlobal_VirtualMedia))
    {
        sltHandleToolOpenedGlobal(ToolTypeGlobal_VirtualMedia);
        return;
    }

    /* Create instance if not yet created: */
    if (!m_pManagerVirtualMedia)
    {
        UIMediumManagerFactory().prepare(m_pManagerVirtualMedia, this);
        connect(m_pManagerVirtualMedia, &QIManagerDialog::sigClose,
                this, &UISelectorWindow::sltCloseVirtualMediumManagerWindow);
    }

    /* Show instance: */
    m_pManagerVirtualMedia->show();
    m_pManagerVirtualMedia->setWindowState(m_pManagerVirtualMedia->windowState() & ~Qt::WindowMinimized);
    m_pManagerVirtualMedia->activateWindow();
}

void UISelectorWindow::sltCloseVirtualMediumManagerWindow()
{
    /* Destroy instance if still exists: */
    if (m_pManagerVirtualMedia)
        UIMediumManagerFactory().cleanup(m_pManagerVirtualMedia);
}

void UISelectorWindow::sltOpenHostNetworkManagerWindow()
{
    /* First check if instance of widget opened embedded: */
    if (m_pPaneToolsGlobal->isToolOpened(ToolTypeGlobal_HostNetwork))
    {
        sltHandleToolOpenedGlobal(ToolTypeGlobal_HostNetwork);
        return;
    }

    /* Create instance if not yet created: */
    if (!m_pManagerHostNetwork)
    {
        UIHostNetworkManagerFactory().prepare(m_pManagerHostNetwork, this);
        connect(m_pManagerHostNetwork, &QIManagerDialog::sigClose,
                this, &UISelectorWindow::sltCloseHostNetworkManagerWindow);
    }

    /* Show instance: */
    m_pManagerHostNetwork->show();
    m_pManagerHostNetwork->setWindowState(m_pManagerHostNetwork->windowState() & ~Qt::WindowMinimized);
    m_pManagerHostNetwork->activateWindow();
}

void UISelectorWindow::sltCloseHostNetworkManagerWindow()
{
    /* Destroy instance if still exists: */
    if (m_pManagerHostNetwork)
        UIHostNetworkManagerFactory().cleanup(m_pManagerHostNetwork);
}

void UISelectorWindow::sltOpenImportApplianceWizard(const QString &strFileName /* = QString() */)
{
    /* Show Import Appliance wizard: */
#ifdef VBOX_WS_MAC
    QString strTmpFile = ::darwinResolveAlias(strFileName);
#else /* VBOX_WS_MAC */
    QString strTmpFile = strFileName;
#endif /* !VBOX_WS_MAC */

    /* Lock the action preventing cascade calls: */
    actionPool()->action(UIActionIndexST_M_File_S_ImportAppliance)->setEnabled(false);

    /* Use the "safe way" to open stack of Mac OS X Sheets: */
    QWidget *pWizardParent = windowManager().realParentWindow(this);
    UISafePointerWizardImportApp pWizard = new UIWizardImportApp(pWizardParent, strTmpFile);
    windowManager().registerNewParent(pWizard, pWizardParent);
    pWizard->prepare();
    if (strFileName.isEmpty() || pWizard->isValid())
        pWizard->exec();
    if (pWizard)
        delete pWizard;

    /* Unlock the action allowing further calls: */
    actionPool()->action(UIActionIndexST_M_File_S_ImportAppliance)->setEnabled(true);
}

void UISelectorWindow::sltOpenExportApplianceWizard()
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* Populate the list of VM names: */
    QStringList names;
    for (int i = 0; i < items.size(); ++i)
        names << items[i]->name();

    /* Lock the action preventing cascade calls: */
    actionPool()->action(UIActionIndexST_M_File_S_ExportAppliance)->setEnabled(false);

    /* Use the "safe way" to open stack of Mac OS X Sheets: */
    QWidget *pWizardParent = windowManager().realParentWindow(this);
    UISafePointerWizard pWizard = new UIWizardExportApp(pWizardParent, names);
    windowManager().registerNewParent(pWizard, pWizardParent);
    pWizard->prepare();
    pWizard->exec();
    if (pWizard)
        delete pWizard;

    /* Unlock the action allowing further calls: */
    actionPool()->action(UIActionIndexST_M_File_S_ExportAppliance)->setEnabled(true);
}

#ifdef VBOX_GUI_WITH_EXTRADATA_MANAGER_UI
void UISelectorWindow::sltOpenExtraDataManagerWindow()
{
    gEDataManager->openWindow(this);
}
#endif /* VBOX_GUI_WITH_EXTRADATA_MANAGER_UI */

void UISelectorWindow::sltOpenPreferencesDialog()
{
    /* Check that we do NOT handling that already: */
    if (actionPool()->action(UIActionIndex_M_Application_S_Preferences)->data().toBool())
        return;
    /* Remember that we handling that already: */
    actionPool()->action(UIActionIndex_M_Application_S_Preferences)->setData(true);

    /* Don't show the inaccessible warning
     * if the user tries to open global settings: */
    m_fWarningAboutInaccessibleMediaShown = true;

    /* Create and execute global settings window: */
    UISettingsDialogGlobal dialog(this);
    dialog.execute();

    /* Remember that we do NOT handling that already: */
    actionPool()->action(UIActionIndex_M_Application_S_Preferences)->setData(false);
}

void UISelectorWindow::sltPerformExit()
{
    close();
}

void UISelectorWindow::sltOpenAddMachineDialog(const QString &strFileName /* = QString() */)
{
    /* Initialize variables: */
#ifdef VBOX_WS_MAC
    QString strTmpFile = ::darwinResolveAlias(strFileName);
#else /* VBOX_WS_MAC */
    QString strTmpFile = strFileName;
#endif /* !VBOX_WS_MAC */
    CVirtualBox vbox = vboxGlobal().virtualBox();
    if (strTmpFile.isEmpty())
    {
        QString strBaseFolder = vbox.GetSystemProperties().GetDefaultMachineFolder();
        QString strTitle = tr("Select a virtual machine file");
        QStringList extensions;
        for (int i = 0; i < VBoxFileExts.size(); ++i)
            extensions << QString("*.%1").arg(VBoxFileExts[i]);
        QString strFilter = tr("Virtual machine files (%1)").arg(extensions.join(" "));
        /* Create open file dialog: */
        QStringList fileNames = QIFileDialog::getOpenFileNames(strBaseFolder, strFilter, this, strTitle, 0, true, true);
        if (!fileNames.isEmpty())
            strTmpFile = fileNames.at(0);
    }
    /* Nothing was chosen? */
    if (strTmpFile.isEmpty())
        return;

    /* Make sure this machine can be opened: */
    CMachine newMachine = vbox.OpenMachine(strTmpFile);
    if (!vbox.isOk())
    {
        msgCenter().cannotOpenMachine(vbox, strTmpFile);
        return;
    }

    /* Make sure this machine was NOT registered already: */
    CMachine oldMachine = vbox.FindMachine(newMachine.GetId());
    if (!oldMachine.isNull())
    {
        msgCenter().cannotReregisterExistingMachine(strTmpFile, oldMachine.GetName());
        return;
    }

    /* Register that machine: */
    vbox.RegisterMachine(newMachine);
}

void UISelectorWindow::sltOpenMachineSettingsDialog(const QString &strCategoryRef /* = QString() */,
                                                    const QString &strControlRef /* = QString() */,
                                                    const QString &strID /* = QString() */)
{
    /* Check that we do NOT handling that already: */
    if (actionPool()->action(UIActionIndexST_M_Machine_S_Settings)->data().toBool())
        return;
    /* Remember that we handling that already: */
    actionPool()->action(UIActionIndexST_M_Machine_S_Settings)->setData(true);

    /* Process href from VM details / description: */
    if (!strCategoryRef.isEmpty() && strCategoryRef[0] != '#')
    {
        vboxGlobal().openURL(strCategoryRef);
        return;
    }

    /* Get category and control: */
    QString strCategory = strCategoryRef;
    QString strControl = strControlRef;
    /* Check if control is coded into the URL by %%: */
    if (strControl.isEmpty())
    {
        QStringList parts = strCategory.split("%%");
        if (parts.size() == 2)
        {
            strCategory = parts.at(0);
            strControl = parts.at(1);
        }
    }

    /* Don't show the inaccessible warning
     * if the user tries to open VM settings: */
    m_fWarningAboutInaccessibleMediaShown = true;

    /* Create and execute corresponding VM settings window: */
    UISettingsDialogMachine dialog(this,
                                   QUuid(strID).isNull() ? currentItem()->id() : strID,
                                   strCategory, strControl);
    dialog.execute();

    /* Remember that we do NOT handling that already: */
    actionPool()->action(UIActionIndexST_M_Machine_S_Settings)->setData(false);
}

void UISelectorWindow::sltOpenCloneMachineWizard()
{
    /* Get current item: */
    UIVMItem *pItem = currentItem();
    AssertMsgReturnVoid(pItem, ("Current item should be selected!\n"));

    /* Lock the action preventing cascade calls: */
    actionPool()->action(UIActionIndexST_M_Machine_S_Clone)->setEnabled(false);

    /* Use the "safe way" to open stack of Mac OS X Sheets: */
    QWidget *pWizardParent = windowManager().realParentWindow(this);
    UISafePointerWizard pWizard = new UIWizardCloneVM(pWizardParent, pItem->machine());
    windowManager().registerNewParent(pWizard, pWizardParent);
    pWizard->prepare();
    pWizard->exec();
    if (pWizard)
        delete pWizard;

    /* Unlock the action allowing further calls: */
    actionPool()->action(UIActionIndexST_M_Machine_S_Clone)->setEnabled(true);
}

void UISelectorWindow::sltPerformStartOrShowMachine()
{
    /* Start selected VMs in corresponding mode: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));
    performStartOrShowVirtualMachines(items, VBoxGlobal::LaunchMode_Invalid);
}

void UISelectorWindow::sltPerformStartMachineNormal()
{
    /* Start selected VMs in corresponding mode: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));
    performStartOrShowVirtualMachines(items, VBoxGlobal::LaunchMode_Default);
}

void UISelectorWindow::sltPerformStartMachineHeadless()
{
    /* Start selected VMs in corresponding mode: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));
    performStartOrShowVirtualMachines(items, VBoxGlobal::LaunchMode_Headless);
}

void UISelectorWindow::sltPerformStartMachineDetachable()
{
    /* Start selected VMs in corresponding mode: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));
    performStartOrShowVirtualMachines(items, VBoxGlobal::LaunchMode_Separate);
}

void UISelectorWindow::sltPerformDiscardMachineState()
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* Prepare the list of the machines to be discarded: */
    QStringList machineNames;
    QList<UIVMItem*> itemsToDiscard;
    foreach (UIVMItem *pItem, items)
        if (isActionEnabled(UIActionIndexST_M_Group_S_Discard, QList<UIVMItem*>() << pItem))
        {
            machineNames << pItem->name();
            itemsToDiscard << pItem;
        }
    AssertMsg(!machineNames.isEmpty(), ("This action should not be allowed!"));

    /* Confirm discarding saved VM state: */
    if (!msgCenter().confirmDiscardSavedState(machineNames.join(", ")))
        return;

    /* For every confirmed item: */
    foreach (UIVMItem *pItem, itemsToDiscard)
    {
        /* Open a session to modify VM: */
        CSession session = vboxGlobal().openSession(pItem->id());
        if (session.isNull())
            return;

        /* Get session machine: */
        CMachine machine = session.GetMachine();
        machine.DiscardSavedState(true);
        if (!machine.isOk())
            msgCenter().cannotDiscardSavedState(machine);

        /* Unlock machine finally: */
        session.UnlockMachine();
    }
}

void UISelectorWindow::sltPerformPauseOrResumeMachine(bool fPause)
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* For every selected item: */
    foreach (UIVMItem *pItem, items)
    {
        /* Get item state: */
        KMachineState state = pItem->machineState();

        /* Check if current item could be paused/resumed: */
        if (!isActionEnabled(UIActionIndexST_M_Group_T_Pause, QList<UIVMItem*>() << pItem))
            continue;

        /* Check if current item already paused: */
        if (fPause &&
            (state == KMachineState_Paused ||
             state == KMachineState_TeleportingPausedVM))
            continue;

        /* Check if current item already resumed: */
        if (!fPause &&
            (state == KMachineState_Running ||
             state == KMachineState_Teleporting ||
             state == KMachineState_LiveSnapshotting))
            continue;

        /* Open a session to modify VM state: */
        CSession session = vboxGlobal().openExistingSession(pItem->id());
        if (session.isNull())
            return;

        /* Get session console: */
        CConsole console = session.GetConsole();
        /* Pause/resume VM: */
        if (fPause)
            console.Pause();
        else
            console.Resume();
        bool ok = console.isOk();
        if (!ok)
        {
            if (fPause)
                msgCenter().cannotPauseMachine(console);
            else
                msgCenter().cannotResumeMachine(console);
        }

        /* Unlock machine finally: */
        session.UnlockMachine();
    }
}

void UISelectorWindow::sltPerformResetMachine()
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* Prepare the list of the machines to be reseted: */
    QStringList machineNames;
    QList<UIVMItem*> itemsToReset;
    foreach (UIVMItem *pItem, items)
        if (isActionEnabled(UIActionIndexST_M_Group_S_Reset, QList<UIVMItem*>() << pItem))
        {
            machineNames << pItem->name();
            itemsToReset << pItem;
        }
    AssertMsg(!machineNames.isEmpty(), ("This action should not be allowed!"));

    /* Confirm reseting VM: */
    if (!msgCenter().confirmResetMachine(machineNames.join(", ")))
        return;

    /* For each selected item: */
    foreach (UIVMItem *pItem, itemsToReset)
    {
        /* Open a session to modify VM state: */
        CSession session = vboxGlobal().openExistingSession(pItem->id());
        if (session.isNull())
            return;

        /* Get session console: */
        CConsole console = session.GetConsole();
        /* Reset VM: */
        console.Reset();

        /* Unlock machine finally: */
        session.UnlockMachine();
    }
}

void UISelectorWindow::sltPerformDetachMachineUI()
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* For each selected item: */
    foreach (UIVMItem *pItem, items)
    {
        /* Check if current item could be detached: */
        if (!isActionEnabled(UIActionIndexST_M_Machine_M_Close_S_Detach, QList<UIVMItem*>() << pItem))
            continue;

        /// @todo Detach separate UI process..
        AssertFailed();
    }
}

void UISelectorWindow::sltPerformSaveMachineState()
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* For each selected item: */
    foreach (UIVMItem *pItem, items)
    {
        /* Check if current item could be saved: */
        if (!isActionEnabled(UIActionIndexST_M_Machine_M_Close_S_SaveState, QList<UIVMItem*>() << pItem))
            continue;

        /* Open a session to modify VM state: */
        CSession session = vboxGlobal().openExistingSession(pItem->id());
        if (session.isNull())
            return;

        /* Get session console: */
        CConsole console = session.GetConsole();
        /* Get session machine: */
        CMachine machine = session.GetMachine();
        /* Pause VM first if necessary: */
        if (pItem->machineState() != KMachineState_Paused)
            console.Pause();
        if (console.isOk())
        {
            /* Prepare machine state saving: */
            CProgress progress = machine.SaveState();
            if (machine.isOk())
            {
                /* Show machine state saving progress: */
                msgCenter().showModalProgressDialog(progress, machine.GetName(), ":/progress_state_save_90px.png");
                if (!progress.isOk() || progress.GetResultCode() != 0)
                    msgCenter().cannotSaveMachineState(progress, machine.GetName());
            }
            else
                msgCenter().cannotSaveMachineState(machine);
        }
        else
            msgCenter().cannotPauseMachine(console);

        /* Unlock machine finally: */
        session.UnlockMachine();
    }
}

void UISelectorWindow::sltPerformShutdownMachine()
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* Prepare the list of the machines to be shutdowned: */
    QStringList machineNames;
    QList<UIVMItem*> itemsToShutdown;
    foreach (UIVMItem *pItem, items)
        if (isActionEnabled(UIActionIndexST_M_Machine_M_Close_S_Shutdown, QList<UIVMItem*>() << pItem))
        {
            machineNames << pItem->name();
            itemsToShutdown << pItem;
        }
    AssertMsg(!machineNames.isEmpty(), ("This action should not be allowed!"));

    /* Confirm ACPI shutdown current VM: */
    if (!msgCenter().confirmACPIShutdownMachine(machineNames.join(", ")))
        return;

    /* For each selected item: */
    foreach (UIVMItem *pItem, itemsToShutdown)
    {
        /* Open a session to modify VM state: */
        CSession session = vboxGlobal().openExistingSession(pItem->id());
        if (session.isNull())
            return;

        /* Get session console: */
        CConsole console = session.GetConsole();
        /* ACPI Shutdown: */
        console.PowerButton();
        if (!console.isOk())
            msgCenter().cannotACPIShutdownMachine(console);

        /* Unlock machine finally: */
        session.UnlockMachine();
    }
}

void UISelectorWindow::sltPerformPowerOffMachine()
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* Prepare the list of the machines to be powered off: */
    QStringList machineNames;
    QList<UIVMItem*> itemsToPowerOff;
    foreach (UIVMItem *pItem, items)
        if (isActionEnabled(UIActionIndexST_M_Machine_M_Close_S_PowerOff, QList<UIVMItem*>() << pItem))
        {
            machineNames << pItem->name();
            itemsToPowerOff << pItem;
        }
    AssertMsg(!machineNames.isEmpty(), ("This action should not be allowed!"));

    /* Confirm Power Off current VM: */
    if (!msgCenter().confirmPowerOffMachine(machineNames.join(", ")))
        return;

    /* For each selected item: */
    foreach (UIVMItem *pItem, itemsToPowerOff)
    {
        /* Open a session to modify VM state: */
        CSession session = vboxGlobal().openExistingSession(pItem->id());
        if (session.isNull())
            return;

        /* Get session console: */
        CConsole console = session.GetConsole();
        /* Prepare machine power down: */
        CProgress progress = console.PowerDown();
        if (console.isOk())
        {
            /* Show machine power down progress: */
            CMachine machine = session.GetMachine();
            msgCenter().showModalProgressDialog(progress, machine.GetName(), ":/progress_poweroff_90px.png");
            if (!progress.isOk() || progress.GetResultCode() != 0)
                msgCenter().cannotPowerDownMachine(progress, machine.GetName());
        }
        else
            msgCenter().cannotPowerDownMachine(console);

        /* Unlock machine finally: */
        session.UnlockMachine();
    }
}

void UISelectorWindow::sltOpenMachineLogDialog()
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* For each selected item: */
    foreach (UIVMItem *pItem, items)
    {
        /* Check if log could be show for the current item: */
        if (!isActionEnabled(UIActionIndexST_M_Group_S_ShowLogDialog, QList<UIVMItem*>() << pItem))
            continue;

        /* Show VM Log Viewer: */
        UIVMLogViewer::showLogViewerFor(this, pItem->machine());
    }
}

void UISelectorWindow::sltShowMachineInFileManager()
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* For each selected item: */
    foreach (UIVMItem *pItem, items)
    {
        /* Check if that item could be shown in file-browser: */
        if (!isActionEnabled(UIActionIndexST_M_Group_S_ShowInFileManager, QList<UIVMItem*>() << pItem))
            continue;

        /* Show VM in filebrowser: */
        UIDesktopServices::openInFileManager(pItem->machine().GetSettingsFilePath());
    }
}

void UISelectorWindow::sltPerformCreateMachineShortcut()
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    /* For each selected item: */
    foreach (UIVMItem *pItem, items)
    {
        /* Check if shortcuts could be created for this item: */
        if (!isActionEnabled(UIActionIndexST_M_Group_S_CreateShortcut, QList<UIVMItem*>() << pItem))
            continue;

        /* Create shortcut for this VM: */
        const CMachine &machine = pItem->machine();
        UIDesktopServices::createMachineShortcut(machine.GetSettingsFilePath(),
                                                 QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                 machine.GetName(), machine.GetId());
    }
}

void UISelectorWindow::sltGroupCloseMenuAboutToShow()
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    actionPool()->action(UIActionIndexST_M_Group_M_Close_S_Shutdown)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_M_Close_S_Shutdown, items));
}

void UISelectorWindow::sltMachineCloseMenuAboutToShow()
{
    /* Get selected items: */
    QList<UIVMItem*> items = currentItems();
    AssertMsgReturnVoid(!items.isEmpty(), ("At least one item should be selected!\n"));

    actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_Shutdown)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_M_Close_S_Shutdown, items));
}

void UISelectorWindow::sltHandleToolsTypeSwitch()
{
    /* If Machine tool button is checked => go backward: */
    if (actionPool()->action(UIActionIndexST_M_Tools_T_Machine)->isChecked())
        m_pSlidingWidget->moveBackward();

    else

    /* If Global tool button is checked => go forward: */
    if (actionPool()->action(UIActionIndexST_M_Tools_T_Global)->isChecked())
        m_pSlidingWidget->moveForward();

    /* Update action visibility: */
    updateActionsVisibility();

    /* Make sure chosen item fetched: */
    sltHandleChooserPaneIndexChange(false /* update details? */, false /* update snapshots? */);
}

void UISelectorWindow::sltHandleShowTabBarMachine()
{
    m_pActionTabBarGlobal->setVisible(false);
    m_pActionTabBarMachine->setVisible(true);
}

void UISelectorWindow::sltHandleShowTabBarGlobal()
{
    m_pActionTabBarMachine->setVisible(false);
    m_pActionTabBarGlobal->setVisible(true);
}

void UISelectorWindow::sltHandleToolOpenedMachine(ToolTypeMachine enmType)
{
    /* First, make sure corresponding tool set opened: */
    if (   !actionPool()->action(UIActionIndexST_M_Tools_T_Machine)->isChecked()
        && actionPool()->action(UIActionIndexST_M_Tools_T_Machine)->property("watch_child_activation").toBool())
        actionPool()->action(UIActionIndexST_M_Tools_T_Machine)->setChecked(true);

    /* Open corresponding tool: */
    m_pPaneToolsMachine->openTool(enmType);
    /* If that was 'Details' => pass there current items: */
    if (   enmType == ToolTypeMachine_Details
        && m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Details))
        m_pPaneToolsMachine->setItems(currentItems());
    /* If that was 'Snapshot' => pass there current or null machine: */
    if (   enmType == ToolTypeMachine_Snapshots
        && m_pPaneToolsMachine->isToolOpened(ToolTypeMachine_Snapshots))
    {
        UIVMItem *pItem = currentItem();
        m_pPaneToolsMachine->setMachine(pItem ? pItem->machine() : CMachine());
    }
}

void UISelectorWindow::sltHandleToolOpenedGlobal(ToolTypeGlobal enmType)
{
    /* First, make sure corresponding tool set opened: */
    if (   !actionPool()->action(UIActionIndexST_M_Tools_T_Global)->isChecked()
        && actionPool()->action(UIActionIndexST_M_Tools_T_Global)->property("watch_child_activation").toBool())
        actionPool()->action(UIActionIndexST_M_Tools_T_Global)->setChecked(true);

    /* Open corresponding tool: */
    m_pPaneToolsGlobal->openTool(enmType);
}

void UISelectorWindow::sltHandleToolClosedMachine(ToolTypeMachine enmType)
{
    /* Close corresponding tool: */
    m_pPaneToolsMachine->closeTool(enmType);
}

void UISelectorWindow::sltHandleToolClosedGlobal(ToolTypeGlobal enmType)
{
    /* Close corresponding tool: */
    m_pPaneToolsGlobal->closeTool(enmType);
}

UIVMItem* UISelectorWindow::currentItem() const
{
    return m_pPaneChooser->currentItem();
}

QList<UIVMItem*> UISelectorWindow::currentItems() const
{
    return m_pPaneChooser->currentItems();
}

void UISelectorWindow::retranslateUi()
{
    /* Set window title: */
    QString strTitle(VBOX_PRODUCT);
    strTitle += " " + tr("Manager", "Note: main window title which is pretended by the product name.");
#ifdef VBOX_BLEEDING_EDGE
    strTitle += QString(" EXPERIMENTAL build ")
             +  QString(RTBldCfgVersion())
             +  QString(" r")
             +  QString(RTBldCfgRevisionStr())
             +  QString(" - " VBOX_BLEEDING_EDGE);
#endif /* VBOX_BLEEDING_EDGE */
    setWindowTitle(strTitle);

    /* Make sure chosen item fetched: */
    sltHandleChooserPaneIndexChange(false /* update details? */, false /* update snapshots? */);

#ifdef VBOX_WS_MAC
    // WORKAROUND:
    // There is a bug in Qt Cocoa which result in showing a "more arrow" when
    // the necessary size of the toolbar is increased. Also for some languages
    // the with doesn't match if the text increase. So manually adjust the size
    // after changing the text.
    m_pToolBar->updateLayout();
#endif
}

bool UISelectorWindow::event(QEvent *pEvent)
{
    /* Which event do we have? */
    switch (pEvent->type())
    {
        /* Handle every Resize and Move we keep track of the geometry. */
        case QEvent::Resize:
        {
#ifdef VBOX_WS_X11
            /* Prevent handling if fake screen detected: */
            if (gpDesktop->isFakeScreenDetected())
                break;
#endif /* VBOX_WS_X11 */

            if (isVisible() && (windowState() & (Qt::WindowMaximized | Qt::WindowMinimized | Qt::WindowFullScreen)) == 0)
            {
                QResizeEvent *pResizeEvent = static_cast<QResizeEvent*>(pEvent);
                m_geometry.setSize(pResizeEvent->size());
            }
            break;
        }
        case QEvent::Move:
        {
#ifdef VBOX_WS_X11
            /* Prevent handling if fake screen detected: */
            if (gpDesktop->isFakeScreenDetected())
                break;
#endif /* VBOX_WS_X11 */

            if (isVisible() && (windowState() & (Qt::WindowMaximized | Qt::WindowMinimized | Qt::WindowFullScreen)) == 0)
            {
#ifdef VBOX_WS_MAC
                QMoveEvent *pMoveEvent = static_cast<QMoveEvent*>(pEvent);
                m_geometry.moveTo(pMoveEvent->pos());
#else /* VBOX_WS_MAC */
                m_geometry.moveTo(geometry().x(), geometry().y());
#endif /* !VBOX_WS_MAC */
            }
            break;
        }
        case QEvent::WindowDeactivate:
        {
            /* Make sure every status bar hint is cleared when the window lost focus. */
            statusBar()->clearMessage();
            break;
        }
#ifdef VBOX_WS_MAC
        case QEvent::ContextMenu:
        {
            /* This is the unified context menu event. Lets show the context menu. */
            QContextMenuEvent *pContextMenuEvent = static_cast<QContextMenuEvent*>(pEvent);
            sltShowSelectorWindowContextMenu(pContextMenuEvent->globalPos());
            /* Accept it to interrupt the chain. */
            pContextMenuEvent->accept();
            return false;
            break;
        }
#endif /* VBOX_WS_MAC */
        default:
            break;
    }
    /* Call to base-class: */
    return QIMainWindow::event(pEvent);
}

void UISelectorWindow::showEvent(QShowEvent *pEvent)
{
    /* Call to base-class: */
    QIMainWindow::showEvent(pEvent);

    /* Is polishing required? */
    if (!m_fPolished)
    {
        /* Pass the show-event to polish-event: */
        polishEvent(pEvent);
        /* Mark as polished: */
        m_fPolished = true;
    }
}

void UISelectorWindow::polishEvent(QShowEvent*)
{
    /* Make sure user warned about inaccessible medium(s)
     * even if enumeration had finished before selector window shown: */
    QTimer::singleShot(0, this, SLOT(sltHandleMediumEnumerationFinish()));

    /* Call for async polishing: */
    QMetaObject::invokeMethod(this, "sltHandlePolishEvent", Qt::QueuedConnection);
}

#ifdef VBOX_WS_MAC
bool UISelectorWindow::eventFilter(QObject *pObject, QEvent *pEvent)
{
    /* Ignore for non-active window except for FileOpen event which should be always processed: */
    if (!isActiveWindow() && pEvent->type() != QEvent::FileOpen)
        return QIWithRetranslateUI<QIMainWindow>::eventFilter(pObject, pEvent);

    /* Ignore for other objects: */
    if (qobject_cast<QWidget*>(pObject) &&
        qobject_cast<QWidget*>(pObject)->window() != this)
        return QIWithRetranslateUI<QIMainWindow>::eventFilter(pObject, pEvent);

    /* Which event do we have? */
    switch (pEvent->type())
    {
        case QEvent::FileOpen:
        {
            sltOpenUrls(QList<QUrl>() << static_cast<QFileOpenEvent*>(pEvent)->url());
            pEvent->accept();
            return true;
            break;
        }
        default:
            break;
    }
    /* Call to base-class: */
    return QIWithRetranslateUI<QIMainWindow>::eventFilter(pObject, pEvent);
}
#endif /* VBOX_WS_MAC */

void UISelectorWindow::prepare()
{
#ifdef VBOX_WS_X11
    /* Assign same name to both WM_CLASS name & class for now: */
    VBoxGlobal::setWMClass(this, "VirtualBox Manager", "VirtualBox Manager");
#endif

#ifdef VBOX_WS_MAC
    /* We have to make sure that we are getting the front most process: */
    ::darwinSetFrontMostProcess();
#endif /* VBOX_WS_MAC */

    /* Cache medium data early if necessary: */
    if (vboxGlobal().agressiveCaching())
        vboxGlobal().startMediumEnumeration();

    /* Prepare: */
    prepareIcon();
    prepareMenuBar();
    prepareStatusBar();
    prepareToolbar();
    prepareWidgets();
    prepareConnections();

    /* Load settings: */
    loadSettings();

    /* Translate UI: */
    retranslateUi();

#ifdef VBOX_WS_MAC
    /* Enable unified toolbar: */
    m_pToolBar->enableMacToolbar();

    /* Beta label? */
    if (vboxGlobal().isBeta())
    {
        QPixmap betaLabel = ::betaLabel(QSize(100, 16));
        ::darwinLabelWindow(this, &betaLabel, true);
    }

    /* General event filter: */
    qApp->installEventFilter(this);
#endif /* VBOX_WS_MAC */

    /* Make sure current Chooser-pane index fetched: */
    sltHandleChooserPaneIndexChange();
}

void UISelectorWindow::prepareIcon()
{
    /* Prepare application icon.
     * On Win host it's built-in to the executable.
     * On Mac OS X the icon referenced in info.plist is used.
     * On X11 we will provide as much icons as we can. */
#if !(defined (VBOX_WS_WIN) || defined (VBOX_WS_MAC))
    QIcon icon(":/VirtualBox.svg");
    icon.addFile(":/VirtualBox_48px.png");
    icon.addFile(":/VirtualBox_64px.png");
    setWindowIcon(icon);
#endif /* !VBOX_WS_WIN && !VBOX_WS_MAC */
}

void UISelectorWindow::prepareMenuBar()
{
#ifndef VBOX_WS_MAC
    /* Create menu-bar: */
    setMenuBar(new UIMenuBar);
#endif

    /* Create action-pool: */
    m_pActionPool = UIActionPool::create(UIActionPoolType_Selector);

    /* Prepare File-menu: */
    prepareMenuFile(actionPool()->action(UIActionIndexST_M_File)->menu());
    menuBar()->addMenu(actionPool()->action(UIActionIndexST_M_File)->menu());

    /* Prepare 'Group' / 'Start or Show' menu: */
    prepareMenuGroupStartOrShow(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow)->menu());

    /* Prepare 'Machine' / 'Start or Show' menu: */
    prepareMenuMachineStartOrShow(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow)->menu());

    /* Prepare 'Group' / 'Close' menu: */
    prepareMenuGroupClose(actionPool()->action(UIActionIndexST_M_Group_M_Close)->menu());

    /* Prepare 'Machine' / 'Close' menu: */
    prepareMenuMachineClose(actionPool()->action(UIActionIndexST_M_Machine_M_Close)->menu());

    /* Prepare Group-menu: */
    prepareMenuGroup(actionPool()->action(UIActionIndexST_M_Group)->menu());
    m_pGroupMenuAction = menuBar()->addMenu(actionPool()->action(UIActionIndexST_M_Group)->menu());

    /* Prepare Machine-menu: */
    prepareMenuMachine(actionPool()->action(UIActionIndexST_M_Machine)->menu());
    m_pMachineMenuAction = menuBar()->addMenu(actionPool()->action(UIActionIndexST_M_Machine)->menu());

#ifdef VBOX_WS_MAC
    /* Prepare 'Window' menu: */
    UIWindowMenuManager::create();
    menuBar()->addMenu(gpWindowMenuManager->createMenu(this));
    gpWindowMenuManager->addWindow(this);
#endif /* VBOX_WS_MAC */

    /* Prepare Help-menu: */
    menuBar()->addMenu(actionPool()->action(UIActionIndex_Menu_Help)->menu());

    /* Setup menubar policy: */
    menuBar()->setContextMenuPolicy(Qt::CustomContextMenu);
}

void UISelectorWindow::prepareMenuFile(QMenu *pMenu)
{
    /* Do not touch if filled already: */
    if (!pMenu->isEmpty())
        return;

    /* The Application / 'File' menu contents is very different depending on host type. */

#ifdef VBOX_WS_MAC
    /* 'About' action goes to Application menu: */
    pMenu->addAction(actionPool()->action(UIActionIndex_M_Application_S_About));
# ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    /* 'Check for Updates' action goes to Application menu: */
    pMenu->addAction(actionPool()->action(UIActionIndex_M_Application_S_CheckForUpdates));
    /* 'Network Access Manager' action goes to Application menu: */
    pMenu->addAction(actionPool()->action(UIActionIndex_M_Application_S_NetworkAccessManager));
# endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
    /* 'Reset Warnings' action goes to Application menu: */
    pMenu->addAction(actionPool()->action(UIActionIndex_M_Application_S_ResetWarnings));
    /* 'Preferences' action goes to Application menu: */
    pMenu->addAction(actionPool()->action(UIActionIndex_M_Application_S_Preferences));
    /* 'Close' action goes to Application menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_File_S_Close));

    /* 'Import Appliance' action goes to 'File' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_File_S_ImportAppliance));
    /* 'Export Appliance' action goes to 'File' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_File_S_ExportAppliance));
# ifdef VBOX_GUI_WITH_EXTRADATA_MANAGER_UI
    /* 'Show Extra-data Manager' action goes to 'File' menu for Debug build: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_File_S_ShowExtraDataManager));
# endif /* VBOX_GUI_WITH_EXTRADATA_MANAGER_UI */
    /* 'Show Virtual Medium Manager' action goes to 'File' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_File_S_ShowVirtualMediumManager));
    /* 'Show Host Network Manager' action goes to 'File' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_File_S_ShowHostNetworkManager));

#else /* !VBOX_WS_MAC */

# ifdef VBOX_WS_X11
    // WORKAROUND:
    // There is an issue under Ubuntu which uses special kind of QPA
    // plugin (appmenu-qt5) which redirects actions added to Qt menu-bar
    // directly to Ubuntu Application menu-bar. In that case action
    // shortcuts are not being handled by the Qt and that way ignored.
    // As a workaround we can add those actions into QMainWindow as well.
    addAction(actionPool()->action(UIActionIndex_M_Application_S_Preferences));
    addAction(actionPool()->action(UIActionIndexST_M_File_S_ImportAppliance));
    addAction(actionPool()->action(UIActionIndexST_M_File_S_ExportAppliance));
#  ifdef VBOX_GUI_WITH_EXTRADATA_MANAGER_UI
    addAction(actionPool()->action(UIActionIndexST_M_File_S_ShowExtraDataManager));
#  endif /* VBOX_GUI_WITH_EXTRADATA_MANAGER_UI */
    addAction(actionPool()->action(UIActionIndexST_M_File_S_ShowVirtualMediumManager));
    addAction(actionPool()->action(UIActionIndexST_M_File_S_ShowHostNetworkManager));
#  ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    addAction(actionPool()->action(UIActionIndex_M_Application_S_NetworkAccessManager));
    addAction(actionPool()->action(UIActionIndex_M_Application_S_CheckForUpdates));
#  endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
    addAction(actionPool()->action(UIActionIndex_M_Application_S_ResetWarnings));
    addAction(actionPool()->action(UIActionIndexST_M_File_S_Close));
# endif /* VBOX_WS_X11 */

    /* 'Preferences' action goes to 'File' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndex_M_Application_S_Preferences));
    /* Separator after 'Preferences' action of the 'File' menu: */
    pMenu->addSeparator();
    /* 'Import Appliance' action goes to 'File' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_File_S_ImportAppliance));
    /* 'Export Appliance' action goes to 'File' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_File_S_ExportAppliance));
    /* Separator after 'Export Appliance' action of the 'File' menu: */
    pMenu->addSeparator();
# ifdef VBOX_GUI_WITH_EXTRADATA_MANAGER_UI
    /* 'Extra-data Manager' action goes to 'File' menu for Debug build: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_File_S_ShowExtraDataManager));
# endif /* VBOX_GUI_WITH_EXTRADATA_MANAGER_UI */
    /* 'Show Virtual Medium Manager' action goes to 'File' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_File_S_ShowVirtualMediumManager));
    /* 'Show Host Network Manager' action goes to 'File' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_File_S_ShowHostNetworkManager));
# ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    /* 'Network Access Manager' action goes to 'File' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndex_M_Application_S_NetworkAccessManager));
    /* 'Check for Updates' action goes to 'File' menu: */
    if (gEDataManager->applicationUpdateEnabled())
        pMenu->addAction(actionPool()->action(UIActionIndex_M_Application_S_CheckForUpdates));
# endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
    /* 'Reset Warnings' action goes 'File' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndex_M_Application_S_ResetWarnings));
    /* Separator after 'Reset Warnings' action of the 'File' menu: */
    pMenu->addSeparator();
    /* 'Close' action goes to 'File' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_File_S_Close));
#endif /* !VBOX_WS_MAC */
}

void UISelectorWindow::prepareMenuGroup(QMenu *pMenu)
{
    /* Do not touch if filled already: */
    if (!pMenu->isEmpty())
        return;

#ifdef VBOX_WS_X11
    // WORKAROUND:
    // There is an issue under Ubuntu which uses special kind of QPA
    // plugin (appmenu-qt5) which redirects actions added to Qt menu-bar
    // directly to Ubuntu Application menu-bar. In that case action
    // shortcuts are not being handled by the Qt and that way ignored.
    // As a workaround we can add those actions into QMainWindow as well.
    addAction(actionPool()->action(UIActionIndexST_M_Group_S_New));
    addAction(actionPool()->action(UIActionIndexST_M_Group_S_Add));
    addAction(actionPool()->action(UIActionIndexST_M_Group_S_Rename));
    addAction(actionPool()->action(UIActionIndexST_M_Group_S_Remove));
    addAction(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow));
    addAction(actionPool()->action(UIActionIndexST_M_Group_T_Pause));
    addAction(actionPool()->action(UIActionIndexST_M_Group_S_Reset));
    addAction(actionPool()->action(UIActionIndexST_M_Group_S_Discard));
    addAction(actionPool()->action(UIActionIndexST_M_Group_S_ShowLogDialog));
    addAction(actionPool()->action(UIActionIndexST_M_Group_S_Refresh));
    addAction(actionPool()->action(UIActionIndexST_M_Group_S_ShowInFileManager));
    addAction(actionPool()->action(UIActionIndexST_M_Group_S_CreateShortcut));
    addAction(actionPool()->action(UIActionIndexST_M_Group_S_Sort));
#endif /* VBOX_WS_X11 */

    /* Populate Machine-menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_S_New));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Add));
    pMenu->addSeparator();
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Rename));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Remove));
    pMenu->addSeparator();
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_T_Pause));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Reset));
    pMenu->addMenu(actionPool()->action(UIActionIndexST_M_Group_M_Close)->menu());
    pMenu->addSeparator();
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Discard));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_S_ShowLogDialog));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Refresh));
    pMenu->addSeparator();
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_S_ShowInFileManager));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_S_CreateShortcut));
    pMenu->addSeparator();
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_S_Sort));

    /* Remember action list: */
    m_groupActions << actionPool()->action(UIActionIndexST_M_Group_S_New)
                   << actionPool()->action(UIActionIndexST_M_Group_S_Add)
                   << actionPool()->action(UIActionIndexST_M_Group_S_Rename)
                   << actionPool()->action(UIActionIndexST_M_Group_S_Remove)
                   << actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow)
                   << actionPool()->action(UIActionIndexST_M_Group_T_Pause)
                   << actionPool()->action(UIActionIndexST_M_Group_S_Reset)
                   << actionPool()->action(UIActionIndexST_M_Group_S_Discard)
                   << actionPool()->action(UIActionIndexST_M_Group_S_ShowLogDialog)
                   << actionPool()->action(UIActionIndexST_M_Group_S_Refresh)
                   << actionPool()->action(UIActionIndexST_M_Group_S_ShowInFileManager)
                   << actionPool()->action(UIActionIndexST_M_Group_S_CreateShortcut)
                   << actionPool()->action(UIActionIndexST_M_Group_S_Sort);
}

void UISelectorWindow::prepareMenuMachine(QMenu *pMenu)
{
    /* Do not touch if filled already: */
    if (!pMenu->isEmpty())
        return;

#ifdef VBOX_WS_X11
    // WORKAROUND:
    // There is an issue under Ubuntu which uses special kind of QPA
    // plugin (appmenu-qt5) which redirects actions added to Qt menu-bar
    // directly to Ubuntu Application menu-bar. In that case action
    // shortcuts are not being handled by the Qt and that way ignored.
    // As a workaround we can add those actions into QMainWindow as well.
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_New));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Add));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Settings));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Clone));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Remove));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_AddGroup));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_T_Pause));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Reset));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Discard));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_ShowLogDialog));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Refresh));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_ShowInFileManager));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_CreateShortcut));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_S_SortParent));
#endif /* VBOX_WS_X11 */

    /* Populate Machine-menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_New));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Add));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Settings));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Clone));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Remove));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_AddGroup));
    pMenu->addSeparator();
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_T_Pause));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Reset));
    pMenu->addMenu(actionPool()->action(UIActionIndexST_M_Machine_M_Close)->menu());
    pMenu->addSeparator();
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Discard));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_ShowLogDialog));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Refresh));
    pMenu->addSeparator();
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_ShowInFileManager));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_CreateShortcut));
    pMenu->addSeparator();
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_SortParent));

    /* Remember action list: */
    m_machineActions << actionPool()->action(UIActionIndexST_M_Machine_S_New)
                     << actionPool()->action(UIActionIndexST_M_Machine_S_Add)
                     << actionPool()->action(UIActionIndexST_M_Machine_S_Settings)
                     << actionPool()->action(UIActionIndexST_M_Machine_S_Clone)
                     << actionPool()->action(UIActionIndexST_M_Machine_S_Remove)
                     << actionPool()->action(UIActionIndexST_M_Machine_S_AddGroup)
                     << actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow)
                     << actionPool()->action(UIActionIndexST_M_Machine_T_Pause)
                     << actionPool()->action(UIActionIndexST_M_Machine_S_Reset)
                     << actionPool()->action(UIActionIndexST_M_Machine_S_Discard)
                     << actionPool()->action(UIActionIndexST_M_Machine_S_ShowLogDialog)
                     << actionPool()->action(UIActionIndexST_M_Machine_S_Refresh)
                     << actionPool()->action(UIActionIndexST_M_Machine_S_ShowInFileManager)
                     << actionPool()->action(UIActionIndexST_M_Machine_S_CreateShortcut)
                     << actionPool()->action(UIActionIndexST_M_Machine_S_SortParent);
}

void UISelectorWindow::prepareMenuGroupStartOrShow(QMenu *pMenu)
{
    /* Do not touch if filled already: */
    if (!pMenu->isEmpty())
        return;

#ifdef VBOX_WS_X11
    // WORKAROUND:
    // There is an issue under Ubuntu which uses special kind of QPA
    // plugin (appmenu-qt5) which redirects actions added to Qt menu-bar
    // directly to Ubuntu Application menu-bar. In that case action
    // shortcuts are not being handled by the Qt and that way ignored.
    // As a workaround we can add those actions into QMainWindow as well.
    addAction(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartNormal));
    addAction(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartHeadless));
    addAction(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartDetachable));
#endif /* VBOX_WS_X11 */

    /* Populate 'Group' / 'Start or Show' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartNormal));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartHeadless));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartDetachable));

    /* Remember action list: */
    m_groupActions << actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartNormal)
                   << actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartHeadless)
                   << actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartDetachable);
}

void UISelectorWindow::prepareMenuMachineStartOrShow(QMenu *pMenu)
{
    /* Do not touch if filled already: */
    if (!pMenu->isEmpty())
        return;

#ifdef VBOX_WS_X11
    // WORKAROUND:
    // There is an issue under Ubuntu which uses special kind of QPA
    // plugin (appmenu-qt5) which redirects actions added to Qt menu-bar
    // directly to Ubuntu Application menu-bar. In that case action
    // shortcuts are not being handled by the Qt and that way ignored.
    // As a workaround we can add those actions into QMainWindow as well.
    addAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartNormal));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartHeadless));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartDetachable));
#endif /* VBOX_WS_X11 */

    /* Populate 'Machine' / 'Start or Show' menu: */
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartNormal));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartHeadless));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartDetachable));

    /* Remember action list: */
    m_machineActions << actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartNormal)
                     << actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartHeadless)
                     << actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartDetachable);
}

void UISelectorWindow::prepareMenuGroupClose(QMenu *pMenu)
{
    /* Do not touch if filled already: */
    if (!pMenu->isEmpty())
        return;

#ifdef VBOX_WS_X11
    // WORKAROUND:
    // There is an issue under Ubuntu which uses special kind of QPA
    // plugin (appmenu-qt5) which redirects actions added to Qt menu-bar
    // directly to Ubuntu Application menu-bar. In that case action
    // shortcuts are not being handled by the Qt and that way ignored.
    // As a workaround we can add those actions into QMainWindow as well.
    // addAction(actionPool()->action(UIActionIndexST_M_Group_M_Close_S_Detach));
    addAction(actionPool()->action(UIActionIndexST_M_Group_M_Close_S_SaveState));
    addAction(actionPool()->action(UIActionIndexST_M_Group_M_Close_S_Shutdown));
    addAction(actionPool()->action(UIActionIndexST_M_Group_M_Close_S_PowerOff));
#endif /* VBOX_WS_X11 */

    /* Populate 'Group' / 'Close' menu: */
    // pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_M_Close_S_Detach));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_M_Close_S_SaveState));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_M_Close_S_Shutdown));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Group_M_Close_S_PowerOff));

    /* Remember action list: */
    m_groupActions // << actionPool()->action(UIActionIndexST_M_Group_M_Close_S_Detach)
                   << actionPool()->action(UIActionIndexST_M_Group_M_Close_S_SaveState)
                   << actionPool()->action(UIActionIndexST_M_Group_M_Close_S_Shutdown)
                   << actionPool()->action(UIActionIndexST_M_Group_M_Close_S_PowerOff);
}

void UISelectorWindow::prepareMenuMachineClose(QMenu *pMenu)
{
    /* Do not touch if filled already: */
    if (!pMenu->isEmpty())
        return;

#ifdef VBOX_WS_X11
    // WORKAROUND:
    // There is an issue under Ubuntu which uses special kind of QPA
    // plugin (appmenu-qt5) which redirects actions added to Qt menu-bar
    // directly to Ubuntu Application menu-bar. In that case action
    // shortcuts are not being handled by the Qt and that way ignored.
    // As a workaround we can add those actions into QMainWindow as well.
    // addAction(actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_Detach));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_SaveState));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_Shutdown));
    addAction(actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_PowerOff));
#endif /* VBOX_WS_X11 */

    /* Populate 'Machine' / 'Close' menu: */
    // pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_Detach));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_SaveState));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_Shutdown));
    pMenu->addAction(actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_PowerOff));

    /* Remember action list: */
    m_machineActions // << actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_Detach)
                     << actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_SaveState)
                     << actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_Shutdown)
                     << actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_PowerOff);
}

void UISelectorWindow::prepareStatusBar()
{
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    /* Setup statusbar policy: */
    statusBar()->setContextMenuPolicy(Qt::CustomContextMenu);

    /* Add network-manager indicator: */
    UINetworkManagerIndicator *pIndicator = gNetworkManager->createIndicator();
    statusBar()->addPermanentWidget(pIndicator);
    pIndicator->updateAppearance();
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */

#ifdef VBOX_WS_MAC
    /* Make sure the status-bar is aware of action hovering: */
    connect(actionPool(), SIGNAL(sigActionHovered(UIAction *)),
            this, SLOT(sltActionHovered(UIAction *)));
#endif /* VBOX_WS_MAC */
}

void UISelectorWindow::prepareToolbar()
{
    /* Create Main toolbar: */
    m_pToolBar = new UIToolBar(this);
    AssertPtrReturnVoid(m_pToolBar);
    {
        /* Configure toolbar: */
        m_pToolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        m_pToolBar->setContextMenuPolicy(Qt::CustomContextMenu);
        m_pToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        /// @todo Get rid of hard-coded stuff:
        const QSize toolBarIconSize = m_pToolBar->iconSize();
        if (toolBarIconSize.width() < 32 || toolBarIconSize.height() < 32)
            m_pToolBar->setIconSize(QSize(32, 32));

        /* Add main actions block: */
        m_pToolBar->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_New));
        m_pToolBar->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Settings));
        m_pToolBar->addAction(actionPool()->action(UIActionIndexST_M_Machine_S_Discard));
        m_pToolBar->addAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow));
#ifdef VBOX_WS_MAC
        // WORKAROUND:
        // Actually Qt should do that itself but by some unknown reason it sometimes
        // forget to update toolbar after changing its actions on cocoa platform.
        connect(actionPool()->action(UIActionIndexST_M_Machine_S_New), &UIAction::changed,
                m_pToolBar, static_cast<void(UIToolBar::*)(void)>(&UIToolBar::update));
        connect(actionPool()->action(UIActionIndexST_M_Machine_S_Settings), &UIAction::changed,
                m_pToolBar, static_cast<void(UIToolBar::*)(void)>(&UIToolBar::update));
        connect(actionPool()->action(UIActionIndexST_M_Machine_S_Discard), &UIAction::changed,
                m_pToolBar, static_cast<void(UIToolBar::*)(void)>(&UIToolBar::update));
        connect(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow), &UIAction::changed,
                m_pToolBar, static_cast<void(UIToolBar::*)(void)>(&UIToolBar::update));
#endif /* VBOX_WS_MAC */

        /* Create Machine tab-bar: */
        m_pTabBarMachine = new UITabBar;
        AssertPtrReturnVoid(m_pTabBarMachine);
        {
            /* Configure tab-bar: */
            const int iL = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 2;
            const int iR = qApp->style()->pixelMetric(QStyle::PM_LayoutRightMargin) / 2;
            m_pTabBarMachine->setContentsMargins(iL, 0, iR, 0);

            /* Add into toolbar: */
            m_pActionTabBarMachine = m_pToolBar->addWidget(m_pTabBarMachine);
        }

        /* Create Global tab-bar: */
        m_pTabBarGlobal = new UITabBar;
        AssertPtrReturnVoid(m_pTabBarGlobal);
        {
            /* Configure tab-bar: */
            const int iL = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 2;
            const int iR = qApp->style()->pixelMetric(QStyle::PM_LayoutRightMargin) / 2;
            m_pTabBarGlobal->setContentsMargins(iL, 0, iR, 0);

            /* Add into toolbar: */
            m_pActionTabBarGlobal = m_pToolBar->addWidget(m_pTabBarGlobal);
        }

        /* Create Tools toolbar: */
        m_pToolbarTools = new UIToolsToolbar(actionPool());
        if (m_pToolbarTools)
        {
            /* Configure toolbar: */
            m_pToolbarTools->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
            connect(m_pToolbarTools, &UIToolsToolbar::sigShowTabBarMachine,
                    this, &UISelectorWindow::sltHandleShowTabBarMachine);
            connect(m_pToolbarTools, &UIToolsToolbar::sigShowTabBarGlobal,
                    this, &UISelectorWindow::sltHandleShowTabBarGlobal);
            m_pToolbarTools->setTabBars(m_pTabBarMachine, m_pTabBarGlobal);

            /* Create exclusive action-group: */
            QActionGroup *pActionGroupTools = new QActionGroup(m_pToolbarTools);
            AssertPtrReturnVoid(pActionGroupTools);
            {
                /* Configure action-group: */
                pActionGroupTools->setExclusive(true);

                /* Add 'Tools' actions into action-group: */
                pActionGroupTools->addAction(actionPool()->action(UIActionIndexST_M_Tools_T_Machine));
                pActionGroupTools->addAction(actionPool()->action(UIActionIndexST_M_Tools_T_Global));
            }

            /* Add into toolbar: */
            m_pToolBar->addWidget(m_pToolbarTools);
        }

#ifdef VBOX_WS_MAC
        // WORKAROUND:
        // There is a bug in Qt Cocoa which result in showing a "more arrow" when
        // the necessary size of the toolbar is increased. Also for some languages
        // the with doesn't match if the text increase. So manually adjust the size
        // after changing the text.
        m_pToolBar->updateLayout();
#endif
    }
}

void UISelectorWindow::prepareWidgets()
{
    /* Create central-widget: */
    QWidget *pWidget = new QWidget;
    AssertPtrReturnVoid(pWidget);
    {
        /* Configure central-widget: */
        setCentralWidget(pWidget);

        /* Create central-layout: */
        QVBoxLayout *pLayout = new QVBoxLayout(pWidget);
        AssertPtrReturnVoid(pLayout);
        {
            /* Configure layout: */
            pLayout->setSpacing(0);
            pLayout->setContentsMargins(0, 0, 0, 0);

#ifdef VBOX_WS_MAC
            /* Native toolbar on MAC: */
            addToolBar(m_pToolBar);
#else
            /* Add into layout: */
            pLayout->addWidget(m_pToolBar);
#endif

            /* Create sliding-widget: */
            m_pSlidingWidget = new UISlidingWidget;
            AssertPtrReturnVoid(m_pSlidingWidget);
            {
                /* Create splitter: */
                m_pSplitter = new QISplitter;
                AssertPtrReturnVoid(m_pSplitter);
                {
                    /* Configure splitter: */
#ifdef VBOX_WS_X11
                    m_pSplitter->setHandleType(QISplitter::Native);
#endif

                    /* Create Chooser-pane: */
                    m_pPaneChooser = new UIGChooser(this);
                    AssertPtrReturnVoid(m_pPaneChooser);
                    {
                        /* Add into splitter: */
                        m_pSplitter->addWidget(m_pPaneChooser);
                    }

                    /* Create Machine Tools-pane: */
                    m_pPaneToolsMachine = new UIToolsPaneMachine(actionPool());
                    AssertPtrReturnVoid(m_pPaneToolsMachine);
                    {
                        /* Add into splitter: */
                        m_pSplitter->addWidget(m_pPaneToolsMachine);
                    }

                    /* Adjust splitter colors according to main widgets it splits: */
                    m_pSplitter->configureColors(m_pPaneChooser->palette().color(QPalette::Active, QPalette::Window),
                                                 m_pPaneToolsMachine->palette().color(QPalette::Active, QPalette::Window));
                    /* Set the initial distribution. The right site is bigger. */
                    m_pSplitter->setStretchFactor(0, 2);
                    m_pSplitter->setStretchFactor(1, 3);
                }

                /* Create Global Tools-pane: */
                m_pPaneToolsGlobal = new UIToolsPaneGlobal(actionPool());
                AssertPtrReturnVoid(m_pPaneToolsGlobal);

                /* Add left/right widgets into sliding widget: */
                m_pSlidingWidget->setWidgets(m_pSplitter, m_pPaneToolsGlobal);

                /* Add into layout: */
                pLayout->addWidget(m_pSlidingWidget);
            }
        }
    }

    /* Bring the VM list to the focus: */
    m_pPaneChooser->setFocus();
}

void UISelectorWindow::prepareConnections()
{
#ifdef VBOX_WS_X11
    /* Desktop event handlers: */
    connect(gpDesktop, SIGNAL(sigHostScreenWorkAreaResized(int)), this, SLOT(sltHandleHostScreenAvailableAreaChange()));
#endif /* VBOX_WS_X11 */

    /* Medium enumeration connections: */
    connect(&vboxGlobal(), SIGNAL(sigMediumEnumerationFinished()), this, SLOT(sltHandleMediumEnumerationFinish()));

    /* Menu-bar connections: */
    connect(menuBar(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(sltShowSelectorWindowContextMenu(const QPoint&)));

    /* 'File' menu connections: */
    connect(actionPool()->action(UIActionIndexST_M_File_S_ShowVirtualMediumManager), SIGNAL(triggered()), this, SLOT(sltOpenVirtualMediumManagerWindow()));
    connect(actionPool()->action(UIActionIndexST_M_File_S_ShowHostNetworkManager), SIGNAL(triggered()), this, SLOT(sltOpenHostNetworkManagerWindow()));
    connect(actionPool()->action(UIActionIndexST_M_File_S_ImportAppliance), SIGNAL(triggered()), this, SLOT(sltOpenImportApplianceWizard()));
    connect(actionPool()->action(UIActionIndexST_M_File_S_ExportAppliance), SIGNAL(triggered()), this, SLOT(sltOpenExportApplianceWizard()));
#ifdef VBOX_GUI_WITH_EXTRADATA_MANAGER_UI
    connect(actionPool()->action(UIActionIndexST_M_File_S_ShowExtraDataManager), SIGNAL(triggered()), this, SLOT(sltOpenExtraDataManagerWindow()));
#endif /* VBOX_GUI_WITH_EXTRADATA_MANAGER_UI */
    connect(actionPool()->action(UIActionIndex_M_Application_S_Preferences), SIGNAL(triggered()), this, SLOT(sltOpenPreferencesDialog()));
    connect(actionPool()->action(UIActionIndexST_M_File_S_Close), SIGNAL(triggered()), this, SLOT(sltPerformExit()));

    /* 'Group' menu connections: */
    connect(actionPool()->action(UIActionIndexST_M_Group_S_Add), SIGNAL(triggered()), this, SLOT(sltOpenAddMachineDialog()));
    connect(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow), SIGNAL(triggered()), this, SLOT(sltPerformStartOrShowMachine()));
    connect(actionPool()->action(UIActionIndexST_M_Group_T_Pause), SIGNAL(toggled(bool)), this, SLOT(sltPerformPauseOrResumeMachine(bool)));
    connect(actionPool()->action(UIActionIndexST_M_Group_S_Reset), SIGNAL(triggered()), this, SLOT(sltPerformResetMachine()));
    connect(actionPool()->action(UIActionIndexST_M_Group_S_Discard), SIGNAL(triggered()), this, SLOT(sltPerformDiscardMachineState()));
    connect(actionPool()->action(UIActionIndexST_M_Group_S_ShowLogDialog), SIGNAL(triggered()), this, SLOT(sltOpenMachineLogDialog()));
    connect(actionPool()->action(UIActionIndexST_M_Group_S_ShowInFileManager), SIGNAL(triggered()), this, SLOT(sltShowMachineInFileManager()));
    connect(actionPool()->action(UIActionIndexST_M_Group_S_CreateShortcut), SIGNAL(triggered()), this, SLOT(sltPerformCreateMachineShortcut()));

    /* 'Machine' menu connections: */
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_Add), SIGNAL(triggered()), this, SLOT(sltOpenAddMachineDialog()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_Settings), SIGNAL(triggered()), this, SLOT(sltOpenMachineSettingsDialog()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_Clone), SIGNAL(triggered()), this, SLOT(sltOpenCloneMachineWizard()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow), SIGNAL(triggered()), this, SLOT(sltPerformStartOrShowMachine()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_T_Pause), SIGNAL(toggled(bool)), this, SLOT(sltPerformPauseOrResumeMachine(bool)));
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_Reset), SIGNAL(triggered()), this, SLOT(sltPerformResetMachine()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_Discard), SIGNAL(triggered()), this, SLOT(sltPerformDiscardMachineState()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_ShowLogDialog), SIGNAL(triggered()), this, SLOT(sltOpenMachineLogDialog()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_ShowInFileManager), SIGNAL(triggered()), this, SLOT(sltShowMachineInFileManager()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_S_CreateShortcut), SIGNAL(triggered()), this, SLOT(sltPerformCreateMachineShortcut()));

    /* 'Group/Start or Show' menu connections: */
    connect(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartNormal), SIGNAL(triggered()), this, SLOT(sltPerformStartMachineNormal()));
    connect(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartHeadless), SIGNAL(triggered()), this, SLOT(sltPerformStartMachineHeadless()));
    connect(actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartDetachable), SIGNAL(triggered()), this, SLOT(sltPerformStartMachineDetachable()));

    /* 'Machine/Start or Show' menu connections: */
    connect(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartNormal), SIGNAL(triggered()), this, SLOT(sltPerformStartMachineNormal()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartHeadless), SIGNAL(triggered()), this, SLOT(sltPerformStartMachineHeadless()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartDetachable), SIGNAL(triggered()), this, SLOT(sltPerformStartMachineDetachable()));

    /* 'Group/Close' menu connections: */
    connect(actionPool()->action(UIActionIndexST_M_Group_M_Close)->menu(), SIGNAL(aboutToShow()), this, SLOT(sltGroupCloseMenuAboutToShow()));
    connect(actionPool()->action(UIActionIndexST_M_Group_M_Close_S_Detach), SIGNAL(triggered()), this, SLOT(sltPerformDetachMachineUI()));
    connect(actionPool()->action(UIActionIndexST_M_Group_M_Close_S_SaveState), SIGNAL(triggered()), this, SLOT(sltPerformSaveMachineState()));
    connect(actionPool()->action(UIActionIndexST_M_Group_M_Close_S_Shutdown), SIGNAL(triggered()), this, SLOT(sltPerformShutdownMachine()));
    connect(actionPool()->action(UIActionIndexST_M_Group_M_Close_S_PowerOff), SIGNAL(triggered()), this, SLOT(sltPerformPowerOffMachine()));

    /* 'Machine/Close' menu connections: */
    connect(actionPool()->action(UIActionIndexST_M_Machine_M_Close)->menu(), SIGNAL(aboutToShow()), this, SLOT(sltMachineCloseMenuAboutToShow()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_Detach), SIGNAL(triggered()), this, SLOT(sltPerformDetachMachineUI()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_SaveState), SIGNAL(triggered()), this, SLOT(sltPerformSaveMachineState()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_Shutdown), SIGNAL(triggered()), this, SLOT(sltPerformShutdownMachine()));
    connect(actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_PowerOff), SIGNAL(triggered()), this, SLOT(sltPerformPowerOffMachine()));

    /* 'Tools' actions connections: */
    connect(actionPool()->action(UIActionIndexST_M_Tools_T_Machine), &UIAction::toggled,
            this, &UISelectorWindow::sltHandleToolsTypeSwitch);
    connect(actionPool()->action(UIActionIndexST_M_Tools_T_Global), &UIAction::toggled,
            this, &UISelectorWindow::sltHandleToolsTypeSwitch);

    /* Status-bar connections: */
    connect(statusBar(), SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(sltShowSelectorWindowContextMenu(const QPoint&)));

    /* Graphics VM chooser connections: */
    connect(m_pPaneChooser, SIGNAL(sigSelectionChanged()), this, SLOT(sltHandleChooserPaneIndexChange()));
    connect(m_pPaneChooser, SIGNAL(sigSlidingStarted()), m_pPaneToolsMachine, SIGNAL(sigSlidingStarted()));
    connect(m_pPaneChooser, SIGNAL(sigToggleStarted()), m_pPaneToolsMachine, SIGNAL(sigToggleStarted()));
    connect(m_pPaneChooser, SIGNAL(sigToggleFinished()), m_pPaneToolsMachine, SIGNAL(sigToggleFinished()));
    connect(m_pPaneChooser, SIGNAL(sigGroupSavingStateChanged()), this, SLOT(sltHandleGroupSavingProgressChange()));

    /* Tool-bar connections: */
#ifndef VBOX_WS_MAC
    connect(m_pToolBar, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(sltShowSelectorWindowContextMenu(const QPoint&)));
#else /* VBOX_WS_MAC */
    /* We want to receive right click notifications on the title bar, so register our own handler: */
    ::darwinRegisterForUnifiedToolbarContextMenuEvents(this);
#endif /* VBOX_WS_MAC */
    connect(m_pToolbarTools, &UIToolsToolbar::sigToolOpenedMachine, this, &UISelectorWindow::sltHandleToolOpenedMachine);
    connect(m_pToolbarTools, &UIToolsToolbar::sigToolOpenedGlobal,  this, &UISelectorWindow::sltHandleToolOpenedGlobal);
    connect(m_pToolbarTools, &UIToolsToolbar::sigToolClosedMachine, this, &UISelectorWindow::sltHandleToolClosedMachine);
    connect(m_pToolbarTools, &UIToolsToolbar::sigToolClosedGlobal,  this, &UISelectorWindow::sltHandleToolClosedGlobal);

    /* VM desktop connections: */
    connect(m_pPaneToolsMachine, SIGNAL(sigLinkClicked(const QString&, const QString&, const QString&)),
            this, SLOT(sltOpenMachineSettingsDialog(const QString&, const QString&, const QString&)));

    /* Global event handlers: */
    connect(gVBoxEvents, SIGNAL(sigMachineStateChange(QString, KMachineState)), this, SLOT(sltHandleStateChange(QString)));
    connect(gVBoxEvents, SIGNAL(sigSessionStateChange(QString, KSessionState)), this, SLOT(sltHandleStateChange(QString)));
}

void UISelectorWindow::loadSettings()
{
    /* Restore window geometry: */
    {
        /* Load geometry: */
        m_geometry = gEDataManager->selectorWindowGeometry(this);

        /* Restore geometry: */
        LogRel2(("GUI: UISelectorWindow: Restoring geometry to: Origin=%dx%d, Size=%dx%d\n",
                 m_geometry.x(), m_geometry.y(), m_geometry.width(), m_geometry.height()));
        restoreGeometry();
    }

    /* Restore splitter handle position: */
    {
        /* Read splitter hints: */
        QList<int> sizes = gEDataManager->selectorWindowSplitterHints();
        /* If both hints are zero, we have the 'default' case: */
        if (sizes[0] == 0 && sizes[1] == 0)
        {
            /* Propose some 'default' based on current dialog width: */
            sizes[0] = (int)(width() * .9 * (1.0 / 3));
            sizes[1] = (int)(width() * .9 * (2.0 / 3));
        }
        /* Pass hints to the splitter: */
        m_pSplitter->setSizes(sizes);
    }

    /* Restore toolbar and statusbar functionality: */
    {
#ifdef VBOX_WS_MAC
        // WORKAROUND:
        // There is an issue in Qt5 main-window toolbar implementation:
        // if you are hiding it before it's shown for the first time,
        // there is an ugly empty container appears instead, so we
        // have to hide toolbar asynchronously to avoid that.
        if (!gEDataManager->selectorWindowToolBarVisible())
            QMetaObject::invokeMethod(m_pToolBar, "hide", Qt::QueuedConnection);
#else
        m_pToolBar->setHidden(!gEDataManager->selectorWindowToolBarVisible());
#endif
        m_pToolBar->setToolButtonStyle(gEDataManager->selectorWindowToolBarTextVisible()
                                       ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
        m_pToolbarTools->setToolButtonStyle(gEDataManager->selectorWindowToolBarTextVisible()
                                            ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
        statusBar()->setHidden(!gEDataManager->selectorWindowStatusBarVisible());
    }

    /* Restore toolbar Machine/Global tools orders:  */
    {
        m_orderMachine = gEDataManager->selectorWindowToolsOrderMachine();
        m_orderGlobal = gEDataManager->selectorWindowToolsOrderGlobal();

        /* We can restore previously opened Global tools right here: */
        QMap<ToolTypeGlobal, QAction*> mapActionsGlobal;
        mapActionsGlobal[ToolTypeGlobal_VirtualMedia] = actionPool()->action(UIActionIndexST_M_Tools_M_Global_S_VirtualMediaManager);
        mapActionsGlobal[ToolTypeGlobal_HostNetwork] = actionPool()->action(UIActionIndexST_M_Tools_M_Global_S_HostNetworkManager);
        for (int i = m_orderGlobal.size() - 1; i >= 0; --i)
            if (m_orderGlobal.at(i) != ToolTypeGlobal_Invalid)
                mapActionsGlobal.value(m_orderGlobal.at(i))->trigger();
        /* Make sure further action triggering cause tool type switch as well: */
        actionPool()->action(UIActionIndexST_M_Tools_T_Global)->setProperty("watch_child_activation", true);

        /* But we can't restore previously opened Machine tools here,
         * see the reason in corresponding async sltHandlePolishEvent slot. */
    }
}

void UISelectorWindow::saveSettings()
{
    /* Save toolbar Machine/Global tools orders: */
    {
        gEDataManager->setSelectorWindowToolsOrderMachine(m_pToolbarTools->tabOrderMachine());
        gEDataManager->setSelectorWindowToolsOrderGlobal(m_pToolbarTools->tabOrderGlobal());
    }

    /* Save toolbar and statusbar visibility: */
    {
        gEDataManager->setSelectorWindowToolBarVisible(!m_pToolBar->isHidden());
        gEDataManager->setSelectorWindowToolBarTextVisible(m_pToolBar->toolButtonStyle() == Qt::ToolButtonTextUnderIcon);
        gEDataManager->setSelectorWindowStatusBarVisible(!statusBar()->isHidden());
    }

    /* Save splitter handle position: */
    {
        gEDataManager->setSelectorWindowSplitterHints(m_pSplitter->sizes());
    }

    /* Save window geometry: */
    {
#ifdef VBOX_WS_MAC
        gEDataManager->setSelectorWindowGeometry(m_geometry, ::darwinIsWindowMaximized(this));
#else /* VBOX_WS_MAC */
        gEDataManager->setSelectorWindowGeometry(m_geometry, isMaximized());
#endif /* !VBOX_WS_MAC */
        LogRel2(("GUI: UISelectorWindow: Geometry saved as: Origin=%dx%d, Size=%dx%d\n",
                 m_geometry.x(), m_geometry.y(), m_geometry.width(), m_geometry.height()));
    }
}

void UISelectorWindow::cleanupConnections()
{
#ifdef VBOX_WS_MAC
    /* Tool-bar connections: */
    ::darwinUnregisterForUnifiedToolbarContextMenuEvents(this);
#endif /* VBOX_WS_MAC */
}

void UISelectorWindow::cleanupMenuBar()
{
#ifdef VBOX_WS_MAC
    /* Cleanup 'Window' menu: */
    UIWindowMenuManager::destroy();
#endif /* VBOX_WS_MAC */

    /* Destroy action-pool: */
    UIActionPool::destroy(m_pActionPool);
}

void UISelectorWindow::cleanup()
{
    /* Close the sub-dialogs first: */
    sltCloseVirtualMediumManagerWindow();
    sltCloseHostNetworkManagerWindow();

    /* Save settings: */
    saveSettings();

    /* Cleanup: */
    cleanupConnections();
    cleanupMenuBar();
}

void UISelectorWindow::performStartOrShowVirtualMachines(const QList<UIVMItem*> &items, VBoxGlobal::LaunchMode enmLaunchMode)
{
    /* Do nothing while group saving is in progress: */
    if (m_pPaneChooser->isGroupSavingInProgress())
        return;

    /* Compose the list of startable items: */
    QStringList startableMachineNames;
    QList<UIVMItem*> startableItems;
    foreach (UIVMItem *pItem, items)
        if (isAtLeastOneItemCanBeStarted(QList<UIVMItem*>() << pItem))
        {
            startableItems << pItem;
            startableMachineNames << pItem->name();
        }

    /* Initially we have start auto-confirmed: */
    bool fStartConfirmed = true;
    /* But if we have more than one item to start =>
     * We should still ask user for a confirmation: */
    if (startableItems.size() > 1)
        fStartConfirmed = msgCenter().confirmStartMultipleMachines(startableMachineNames.join(", "));

    /* For every item => check if it could be launched: */
    foreach (UIVMItem *pItem, items)
        if (   isAtLeastOneItemCanBeShown(QList<UIVMItem*>() << pItem)
            || (   isAtLeastOneItemCanBeStarted(QList<UIVMItem*>() << pItem)
                && fStartConfirmed))
        {
            /* Fetch item launch mode: */
            VBoxGlobal::LaunchMode enmItemLaunchMode = enmLaunchMode;
            if (enmItemLaunchMode == VBoxGlobal::LaunchMode_Invalid)
                enmItemLaunchMode = UIVMItem::isItemRunningHeadless(pItem)         ? VBoxGlobal::LaunchMode_Separate :
                                    qApp->keyboardModifiers() == Qt::ShiftModifier ? VBoxGlobal::LaunchMode_Headless :
                                                                                     VBoxGlobal::LaunchMode_Default;

            /* Launch current VM: */
            CMachine machine = pItem->machine();
            vboxGlobal().launchMachine(machine, enmItemLaunchMode);
        }
}

void UISelectorWindow::updateActionsVisibility()
{
    /* Determine whether Machine or Group menu should be shown at all: */
    const bool fMachineOrGroupMenuShown = actionPool()->action(UIActionIndexST_M_Tools_T_Machine)->isChecked();
    const bool fMachineMenuShown = !m_pPaneChooser->isSingleGroupSelected();
    m_pMachineMenuAction->setVisible(fMachineOrGroupMenuShown && fMachineMenuShown);
    m_pGroupMenuAction->setVisible(fMachineOrGroupMenuShown && !fMachineMenuShown);

    /* Hide action shortcuts: */
    if (!fMachineMenuShown)
        foreach (UIAction *pAction, m_machineActions)
            pAction->hideShortcut();
    if (fMachineMenuShown)
        foreach (UIAction *pAction, m_groupActions)
            pAction->hideShortcut();

    /* Update actions visibility: */
    foreach (UIAction *pAction, m_machineActions)
        pAction->setVisible(fMachineOrGroupMenuShown);

    /* Show what should be shown: */
    if (fMachineMenuShown)
        foreach (UIAction *pAction, m_machineActions)
            pAction->showShortcut();
    if (!fMachineMenuShown)
        foreach (UIAction *pAction, m_groupActions)
            pAction->showShortcut();
}

void UISelectorWindow::updateActionsAppearance()
{
    /* Get current item(s): */
    UIVMItem *pItem = currentItem();
    QList<UIVMItem*> items = currentItems();

    /* Enable/disable group actions: */
    actionPool()->action(UIActionIndexST_M_Group_S_Rename)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_S_Rename, items));
    actionPool()->action(UIActionIndexST_M_Group_S_Remove)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_S_Remove, items));
    actionPool()->action(UIActionIndexST_M_Group_T_Pause)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_T_Pause, items));
    actionPool()->action(UIActionIndexST_M_Group_S_Reset)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_S_Reset, items));
    actionPool()->action(UIActionIndexST_M_Group_S_Discard)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_S_Discard, items));
    actionPool()->action(UIActionIndexST_M_Group_S_ShowLogDialog)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_S_ShowLogDialog, items));
    actionPool()->action(UIActionIndexST_M_Group_S_Refresh)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_S_Refresh, items));
    actionPool()->action(UIActionIndexST_M_Group_S_ShowInFileManager)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_S_ShowInFileManager, items));
    actionPool()->action(UIActionIndexST_M_Group_S_CreateShortcut)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_S_CreateShortcut, items));
    actionPool()->action(UIActionIndexST_M_Group_S_Sort)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_S_Sort, items));

    /* Enable/disable machine actions: */
    actionPool()->action(UIActionIndexST_M_Machine_S_Settings)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_S_Settings, items));
    actionPool()->action(UIActionIndexST_M_Machine_S_Clone)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_S_Clone, items));
    actionPool()->action(UIActionIndexST_M_Machine_S_Remove)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_S_Remove, items));
    actionPool()->action(UIActionIndexST_M_Machine_S_AddGroup)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_S_AddGroup, items));
    actionPool()->action(UIActionIndexST_M_Machine_T_Pause)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_T_Pause, items));
    actionPool()->action(UIActionIndexST_M_Machine_S_Reset)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_S_Reset, items));
    actionPool()->action(UIActionIndexST_M_Machine_S_Discard)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_S_Discard, items));
    actionPool()->action(UIActionIndexST_M_Machine_S_ShowLogDialog)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_S_ShowLogDialog, items));
    actionPool()->action(UIActionIndexST_M_Machine_S_Refresh)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_S_Refresh, items));
    actionPool()->action(UIActionIndexST_M_Machine_S_ShowInFileManager)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_S_ShowInFileManager, items));
    actionPool()->action(UIActionIndexST_M_Machine_S_CreateShortcut)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_S_CreateShortcut, items));
    actionPool()->action(UIActionIndexST_M_Machine_S_SortParent)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_S_SortParent, items));

    /* Enable/disable group-start-or-show actions: */
    actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_M_StartOrShow, items));
    actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartNormal)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_M_StartOrShow_S_StartNormal, items));
    actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartHeadless)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_M_StartOrShow_S_StartHeadless, items));
    actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow_S_StartDetachable)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_M_StartOrShow_S_StartDetachable, items));

    /* Enable/disable machine-start-or-show actions: */
    actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_M_StartOrShow, items));
    actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartNormal)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_M_StartOrShow_S_StartNormal, items));
    actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartHeadless)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_M_StartOrShow_S_StartHeadless, items));
    actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow_S_StartDetachable)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_M_StartOrShow_S_StartDetachable, items));

    /* Enable/disable group-close actions: */
    actionPool()->action(UIActionIndexST_M_Group_M_Close)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_M_Close, items));
    actionPool()->action(UIActionIndexST_M_Group_M_Close_S_Detach)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_M_Close_S_Detach, items));
    actionPool()->action(UIActionIndexST_M_Group_M_Close_S_SaveState)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_M_Close_S_SaveState, items));
    actionPool()->action(UIActionIndexST_M_Group_M_Close_S_Shutdown)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_M_Close_S_Shutdown, items));
    actionPool()->action(UIActionIndexST_M_Group_M_Close_S_PowerOff)->setEnabled(isActionEnabled(UIActionIndexST_M_Group_M_Close_S_PowerOff, items));

    /* Enable/disable machine-close actions: */
    actionPool()->action(UIActionIndexST_M_Machine_M_Close)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_M_Close, items));
    actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_Detach)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_M_Close_S_Detach, items));
    actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_SaveState)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_M_Close_S_SaveState, items));
    actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_Shutdown)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_M_Close_S_Shutdown, items));
    actionPool()->action(UIActionIndexST_M_Machine_M_Close_S_PowerOff)->setEnabled(isActionEnabled(UIActionIndexST_M_Machine_M_Close_S_PowerOff, items));

    /* Start/Show action is deremined by 1st item: */
    if (pItem && pItem->accessible())
    {
        actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow)->toActionPolymorphicMenu()->setState(UIVMItem::isItemPoweredOff(pItem) ? 0 : 1);
        actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow)->toActionPolymorphicMenu()->setState(UIVMItem::isItemPoweredOff(pItem) ? 0 : 1);
        QToolButton *pButton = qobject_cast<QToolButton*>(m_pToolBar->widgetForAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow)));
        if (pButton)
            pButton->setPopupMode(UIVMItem::isItemPoweredOff(pItem) ? QToolButton::MenuButtonPopup : QToolButton::DelayedPopup);
    }
    else
    {
        actionPool()->action(UIActionIndexST_M_Group_M_StartOrShow)->toActionPolymorphicMenu()->setState(0);
        actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow)->toActionPolymorphicMenu()->setState(0);
        QToolButton *pButton = qobject_cast<QToolButton*>(m_pToolBar->widgetForAction(actionPool()->action(UIActionIndexST_M_Machine_M_StartOrShow)));
        if (pButton)
            pButton->setPopupMode(UIVMItem::isItemPoweredOff(pItem) ? QToolButton::MenuButtonPopup : QToolButton::DelayedPopup);
    }

    /* Pause/Resume action is deremined by 1st started item: */
    UIVMItem *pFirstStartedAction = 0;
    foreach (UIVMItem *pSelectedItem, items)
        if (UIVMItem::isItemStarted(pSelectedItem))
        {
            pFirstStartedAction = pSelectedItem;
            break;
        }
    /* Update the group Pause/Resume action appearance: */
    actionPool()->action(UIActionIndexST_M_Group_T_Pause)->blockSignals(true);
    actionPool()->action(UIActionIndexST_M_Group_T_Pause)->setChecked(pFirstStartedAction && UIVMItem::isItemPaused(pFirstStartedAction));
    actionPool()->action(UIActionIndexST_M_Group_T_Pause)->retranslateUi();
    actionPool()->action(UIActionIndexST_M_Group_T_Pause)->blockSignals(false);
    /* Update the machine Pause/Resume action appearance: */
    actionPool()->action(UIActionIndexST_M_Machine_T_Pause)->blockSignals(true);
    actionPool()->action(UIActionIndexST_M_Machine_T_Pause)->setChecked(pFirstStartedAction && UIVMItem::isItemPaused(pFirstStartedAction));
    actionPool()->action(UIActionIndexST_M_Machine_T_Pause)->retranslateUi();
    actionPool()->action(UIActionIndexST_M_Machine_T_Pause)->blockSignals(false);

    /* Enable/disable tools actions: */
    actionPool()->action(UIActionIndexST_M_Tools_M_Machine)->setEnabled(isActionEnabled(UIActionIndexST_M_Tools_M_Machine, items));
    actionPool()->action(UIActionIndexST_M_Tools_M_Machine_S_Details)->setEnabled(isActionEnabled(UIActionIndexST_M_Tools_M_Machine_S_Details, items));
    actionPool()->action(UIActionIndexST_M_Tools_M_Machine_S_Snapshots)->setEnabled(isActionEnabled(UIActionIndexST_M_Tools_M_Machine_S_Snapshots, items));
}

bool UISelectorWindow::isActionEnabled(int iActionIndex, const QList<UIVMItem*> &items)
{
    /* No actions enabled for empty item list: */
    if (items.isEmpty())
        return false;

    /* Get first item: */
    UIVMItem *pItem = items.first();

    /* For known action types: */
    switch (iActionIndex)
    {
        case UIActionIndexST_M_Group_S_Rename:
        case UIActionIndexST_M_Group_S_Remove:
        {
            return !m_pPaneChooser->isGroupSavingInProgress() &&
                   isItemsPoweredOff(items);
        }
        case UIActionIndexST_M_Group_S_Sort:
        {
            return !m_pPaneChooser->isGroupSavingInProgress() &&
                   m_pPaneChooser->isSingleGroupSelected();
        }
        case UIActionIndexST_M_Machine_S_Settings:
        {
            return !m_pPaneChooser->isGroupSavingInProgress() &&
                   items.size() == 1 &&
                   pItem->configurationAccessLevel() != ConfigurationAccessLevel_Null;
        }
        case UIActionIndexST_M_Machine_S_Clone:
        {
            return !m_pPaneChooser->isGroupSavingInProgress() &&
                   items.size() == 1 &&
                   UIVMItem::isItemEditable(pItem);
        }
        case UIActionIndexST_M_Machine_S_Remove:
        {
            return !m_pPaneChooser->isGroupSavingInProgress() &&
                   isAtLeastOneItemRemovable(items);
        }
        case UIActionIndexST_M_Machine_S_AddGroup:
        {
            return !m_pPaneChooser->isGroupSavingInProgress() &&
                   !m_pPaneChooser->isAllItemsOfOneGroupSelected() &&
                   isItemsPoweredOff(items);
        }
        case UIActionIndexST_M_Group_M_StartOrShow:
        case UIActionIndexST_M_Group_M_StartOrShow_S_StartNormal:
        case UIActionIndexST_M_Group_M_StartOrShow_S_StartHeadless:
        case UIActionIndexST_M_Group_M_StartOrShow_S_StartDetachable:
        case UIActionIndexST_M_Machine_M_StartOrShow:
        case UIActionIndexST_M_Machine_M_StartOrShow_S_StartNormal:
        case UIActionIndexST_M_Machine_M_StartOrShow_S_StartHeadless:
        case UIActionIndexST_M_Machine_M_StartOrShow_S_StartDetachable:
        {
            return !m_pPaneChooser->isGroupSavingInProgress() &&
                   isAtLeastOneItemCanBeStartedOrShown(items);
        }
        case UIActionIndexST_M_Group_S_Discard:
        case UIActionIndexST_M_Machine_S_Discard:
        {
            return !m_pPaneChooser->isGroupSavingInProgress() &&
                   isAtLeastOneItemDiscardable(items);
        }
        case UIActionIndexST_M_Group_S_ShowLogDialog:
        case UIActionIndexST_M_Machine_S_ShowLogDialog:
        {
            return isAtLeastOneItemAccessible(items);
        }
        case UIActionIndexST_M_Group_T_Pause:
        case UIActionIndexST_M_Machine_T_Pause:
        {
            return isAtLeastOneItemStarted(items);
        }
        case UIActionIndexST_M_Group_S_Reset:
        case UIActionIndexST_M_Machine_S_Reset:
        {
            return isAtLeastOneItemRunning(items);
        }
        case UIActionIndexST_M_Group_S_Refresh:
        case UIActionIndexST_M_Machine_S_Refresh:
        {
            return isAtLeastOneItemInaccessible(items);
        }
        case UIActionIndexST_M_Group_S_ShowInFileManager:
        case UIActionIndexST_M_Machine_S_ShowInFileManager:
        {
            return isAtLeastOneItemAccessible(items);
        }
        case UIActionIndexST_M_Machine_S_SortParent:
        {
            return !m_pPaneChooser->isGroupSavingInProgress();
        }
        case UIActionIndexST_M_Group_S_CreateShortcut:
        case UIActionIndexST_M_Machine_S_CreateShortcut:
        {
            return isAtLeastOneItemSupportsShortcuts(items);
        }
        case UIActionIndexST_M_Group_M_Close:
        case UIActionIndexST_M_Machine_M_Close:
        {
            return isAtLeastOneItemStarted(items);
        }
        case UIActionIndexST_M_Group_M_Close_S_Detach:
        case UIActionIndexST_M_Machine_M_Close_S_Detach:
        {
            return isActionEnabled(UIActionIndexST_M_Machine_M_Close, items);
        }
        case UIActionIndexST_M_Group_M_Close_S_SaveState:
        case UIActionIndexST_M_Machine_M_Close_S_SaveState:
        {
            return isActionEnabled(UIActionIndexST_M_Machine_M_Close, items);
        }
        case UIActionIndexST_M_Group_M_Close_S_Shutdown:
        case UIActionIndexST_M_Machine_M_Close_S_Shutdown:
        {
            return isActionEnabled(UIActionIndexST_M_Machine_M_Close, items) &&
                   isAtLeastOneItemAbleToShutdown(items);
        }
        case UIActionIndexST_M_Group_M_Close_S_PowerOff:
        case UIActionIndexST_M_Machine_M_Close_S_PowerOff:
        {
            return isActionEnabled(UIActionIndexST_M_Machine_M_Close, items);
        }
        case UIActionIndexST_M_Tools_M_Machine:
        case UIActionIndexST_M_Tools_M_Machine_S_Details:
        case UIActionIndexST_M_Tools_M_Machine_S_Snapshots:
        {
            return pItem->accessible();
        }
        default:
            break;
    }

    /* Unknown actions are disabled: */
    return false;
}

/* static */
bool UISelectorWindow::isItemsPoweredOff(const QList<UIVMItem*> &items)
{
    foreach (UIVMItem *pItem, items)
        if (!UIVMItem::isItemPoweredOff(pItem))
            return false;
    return true;
}

/* static */
bool UISelectorWindow::isAtLeastOneItemAbleToShutdown(const QList<UIVMItem*> &items)
{
    /* Enumerate all the passed items: */
    foreach (UIVMItem *pItem, items)
    {
        /* Skip non-running machines: */
        if (!UIVMItem::isItemRunning(pItem))
            continue;
        /* Skip session failures: */
        CSession session = vboxGlobal().openExistingSession(pItem->id());
        if (session.isNull())
            continue;
        /* Skip console failures: */
        CConsole console = session.GetConsole();
        if (console.isNull())
        {
            /* Do not forget to release machine: */
            session.UnlockMachine();
            continue;
        }
        /* Is the guest entered ACPI mode? */
        bool fGuestEnteredACPIMode = console.GetGuestEnteredACPIMode();
        /* Do not forget to release machine: */
        session.UnlockMachine();
        /* True if the guest entered ACPI mode: */
        if (fGuestEnteredACPIMode)
            return true;
    }
    /* False by default: */
    return false;
}

/* static */
bool UISelectorWindow::isAtLeastOneItemSupportsShortcuts(const QList<UIVMItem*> &items)
{
    foreach (UIVMItem *pItem, items)
        if (pItem->accessible()
#ifdef VBOX_WS_MAC
            /* On Mac OS X this are real alias files, which don't work with the old legacy xml files. */
            && pItem->settingsFile().endsWith(".vbox", Qt::CaseInsensitive)
#endif /* VBOX_WS_MAC */
            )
            return true;
    return false;
}

/* static */
bool UISelectorWindow::isAtLeastOneItemAccessible(const QList<UIVMItem*> &items)
{
    foreach (UIVMItem *pItem, items)
        if (pItem->accessible())
            return true;
    return false;
}

/* static */
bool UISelectorWindow::isAtLeastOneItemInaccessible(const QList<UIVMItem*> &items)
{
    foreach (UIVMItem *pItem, items)
        if (!pItem->accessible())
            return true;
    return false;
}

/* static */
bool UISelectorWindow::isAtLeastOneItemRemovable(const QList<UIVMItem*> &items)
{
    foreach (UIVMItem *pItem, items)
        if (!pItem->accessible() || UIVMItem::isItemEditable(pItem))
            return true;
    return false;
}

/* static */
bool UISelectorWindow::isAtLeastOneItemCanBeStarted(const QList<UIVMItem*> &items)
{
    foreach (UIVMItem *pItem, items)
    {
        if (UIVMItem::isItemPoweredOff(pItem) && UIVMItem::isItemEditable(pItem))
            return true;
    }
    return false;
}

/* static */
bool UISelectorWindow::isAtLeastOneItemCanBeShown(const QList<UIVMItem*> &items)
{
    foreach (UIVMItem *pItem, items)
    {
        if (UIVMItem::isItemStarted(pItem) && (pItem->canSwitchTo() || UIVMItem::isItemRunningHeadless(pItem)))
            return true;
    }
    return false;
}

/* static */
bool UISelectorWindow::isAtLeastOneItemCanBeStartedOrShown(const QList<UIVMItem*> &items)
{
    foreach (UIVMItem *pItem, items)
    {
        if ((UIVMItem::isItemPoweredOff(pItem) && UIVMItem::isItemEditable(pItem)) ||
            (UIVMItem::isItemStarted(pItem) && (pItem->canSwitchTo() || UIVMItem::isItemRunningHeadless(pItem))))
            return true;
    }
    return false;
}

/* static */
bool UISelectorWindow::isAtLeastOneItemDiscardable(const QList<UIVMItem*> &items)
{
    foreach (UIVMItem *pItem, items)
        if (UIVMItem::isItemSaved(pItem) && UIVMItem::isItemEditable(pItem))
            return true;
    return false;
}

/* static */
bool UISelectorWindow::isAtLeastOneItemStarted(const QList<UIVMItem*> &items)
{
    foreach (UIVMItem *pItem, items)
        if (UIVMItem::isItemStarted(pItem))
            return true;
    return false;
}

/* static */
bool UISelectorWindow::isAtLeastOneItemRunning(const QList<UIVMItem*> &items)
{
    foreach (UIVMItem *pItem, items)
        if (UIVMItem::isItemRunning(pItem))
            return true;
    return false;
}

