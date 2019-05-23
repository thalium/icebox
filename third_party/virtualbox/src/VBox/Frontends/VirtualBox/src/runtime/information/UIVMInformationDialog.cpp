/* $Id: UIVMInformationDialog.cpp $ */
/** @file
 * VBox Qt GUI - UIVMInformationDialog class implementation.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
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
# include <QScrollBar>
# include <QPushButton>

/* GUI includes: */
# include "UIVMInformationDialog.h"
# include "UIExtraDataManager.h"
# include "UISession.h"
# include "UIMachineLogic.h"
# include "UIMachineWindow.h"
# include "UIMachineView.h"
# include "UIConverter.h"
# include "UIIconPool.h"
# include "QITabWidget.h"
# include "QIDialogButtonBox.h"
# include "VBoxGlobal.h"
# include "VBoxUtils.h"
# include "UIInformationConfiguration.h"
# include "UIMachine.h"
# include "UIVMItem.h"
# include "UIInformationRuntime.h"

/* COM includes: */
# include "COMEnums.h"
# include "CMachine.h"
# include "CConsole.h"
# include "CSystemProperties.h"
# include "CMachineDebugger.h"
# include "CDisplay.h"
# include "CGuest.h"
# include "CStorageController.h"
# include "CMediumAttachment.h"
# include "CNetworkAdapter.h"

/* Other VBox includes: */
# include <iprt/time.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

#include "CVRDEServerInfo.h"


/* static */
UIVMInformationDialog* UIVMInformationDialog::s_pInstance = 0;

void UIVMInformationDialog::invoke(UIMachineWindow *pMachineWindow)
{
    /* Make sure dialog instance exists: */
    if (!s_pInstance)
    {
        /* Create new dialog instance if it doesn't exists yet: */
        new UIVMInformationDialog(pMachineWindow);
    }

    /* Show dialog: */
    s_pInstance->show();
    /* Raise it: */
    s_pInstance->raise();
    /* De-miniaturize if necessary: */
    s_pInstance->setWindowState(s_pInstance->windowState() & ~Qt::WindowMinimized);
    /* And activate finally: */
    s_pInstance->activateWindow();
}

UIVMInformationDialog::UIVMInformationDialog(UIMachineWindow *pMachineWindow)
    : QIWithRetranslateUI<QIMainWindow>(0)
    , m_pTabWidget(0)
    , m_pMachineWindow(pMachineWindow)
{
    /* Initialize instance: */
    s_pInstance = this;

    /* Prepare: */
    prepare();
}

UIVMInformationDialog::~UIVMInformationDialog()
{
    /* Cleanup: */
    cleanup();

    /* Deinitialize instance: */
    s_pInstance = 0;
}

bool UIVMInformationDialog::shouldBeMaximized() const
{
    return gEDataManager->informationWindowShouldBeMaximized(vboxGlobal().managedVMUuid());
}

void UIVMInformationDialog::retranslateUi()
{
    /* Setup dialog title: */
    setWindowTitle(tr("%1 - Session Information").arg(m_pMachineWindow->machine().GetName()));

    /* Translate tabs: */
    m_pTabWidget->setTabText(0, tr("Configuration &Details"));
    m_pTabWidget->setTabText(1, tr("&Runtime Information"));
}

bool UIVMInformationDialog::event(QEvent *pEvent)
{
    /* Pre-process through base-class: */
    const bool fResult = QIMainWindow::event(pEvent);

    /* Process required events: */
    switch (pEvent->type())
    {
        /* Handle Resize event to keep track of the geometry: */
        case QEvent::Resize:
        {
            if (isVisible() && (windowState() & (Qt::WindowMaximized | Qt::WindowMinimized | Qt::WindowFullScreen)) == 0)
            {
                QResizeEvent *pResizeEvent = static_cast<QResizeEvent*>(pEvent);
                m_geometry.setSize(pResizeEvent->size());
            }
            break;
        }
        /* Handle Move event to keep track of the geometry: */
        case QEvent::Move:
        {
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
        default:
            break;
    }

    /* Return result: */
    return fResult;
}

void UIVMInformationDialog::sltHandlePageChanged(int iIndex)
{
    /* Focus the browser on shown page: */
    m_pTabWidget->widget(iIndex)->setFocus();
}

void UIVMInformationDialog::prepare()
{
    /* Prepare dialog: */
    prepareThis();
    /* Load settings: */
    loadSettings();
}

void UIVMInformationDialog::prepareThis()
{
    /* Delete dialog on close: */
    setAttribute(Qt::WA_DeleteOnClose);
    /* Delete dialog on machine-window destruction: */
    connect(m_pMachineWindow, SIGNAL(destroyed(QObject*)), this, SLOT(suicide()));

#ifdef VBOX_WS_MAC
    /* No window-icon on Mac OS X, because it acts as proxy icon which isn't necessary here. */
    setWindowIcon(QIcon());
#else
    /* Assign window-icons: */
    setWindowIcon(UIIconPool::iconSetFull(":/session_info_32px.png", ":/session_info_16px.png"));
#endif

    /* Prepare central-widget: */
    prepareCentralWidget();

    /* Retranslate: */
    retranslateUi();
}

void UIVMInformationDialog::prepareCentralWidget()
{
    /* Create central-widget: */
    setCentralWidget(new QWidget);
    AssertPtrReturnVoid(centralWidget());
    {
        /* Create main-layout: */
        new QVBoxLayout(centralWidget());
        AssertPtrReturnVoid(centralWidget()->layout());
        {
            /* Create tab-widget: */
            prepareTabWidget();
            /* Create button-box: */
            prepareButtonBox();
        }
    }
}

void UIVMInformationDialog::prepareTabWidget()
{
    /* Create tab-widget: */
    m_pTabWidget = new QITabWidget;
    AssertPtrReturnVoid(m_pTabWidget);
    {
        /* Prepare tab-widget: */
        m_pTabWidget->setTabIcon(0, UIIconPool::iconSet(":/session_info_details_16px.png"));
        m_pTabWidget->setTabIcon(1, UIIconPool::iconSet(":/session_info_runtime_16px.png"));

        /* Create Configuration Details tab: */
        UIInformationConfiguration *pInformationConfigurationWidget =
            new UIInformationConfiguration(this, m_pMachineWindow->machine(), m_pMachineWindow->console());
        AssertPtrReturnVoid(pInformationConfigurationWidget);
        {
            m_tabs.insert(0, pInformationConfigurationWidget);
            m_pTabWidget->addTab(m_tabs.value(0), QString());
        }

        /* Create Runtime Information tab: */
        UIInformationRuntime *pInformationRuntimeWidget =
            new UIInformationRuntime(this, m_pMachineWindow->machine(), m_pMachineWindow->console());
        AssertPtrReturnVoid(pInformationRuntimeWidget);
        {
            m_tabs.insert(1, pInformationRuntimeWidget);
            m_pTabWidget->addTab(m_tabs.value(1), QString());
        }

        /* Set Runtime Information tab as default: */
        m_pTabWidget->setCurrentIndex(1);

        /* Assign tab-widget page change handler: */
        connect(m_pTabWidget, SIGNAL(currentChanged(int)), this, SLOT(sltHandlePageChanged(int)));

        /* Add tab-widget into main-layout: */
        centralWidget()->layout()->addWidget(m_pTabWidget);
    }
}

void UIVMInformationDialog::prepareButtonBox()
{
    /* Create button-box: */
    m_pButtonBox = new QIDialogButtonBox;
    AssertPtrReturnVoid(m_pButtonBox);
    {
        /* Configure button-box: */
        m_pButtonBox->setStandardButtons(QDialogButtonBox::Close);
        m_pButtonBox->button(QDialogButtonBox::Close)->setShortcut(Qt::Key_Escape);
        connect(m_pButtonBox, SIGNAL(rejected()), this, SLOT(close()));
        /* Add button-box into main-layout: */
        centralWidget()->layout()->addWidget(m_pButtonBox);
    }
}

void UIVMInformationDialog::loadSettings()
{
    /* Restore window geometry: */
    {
        /* Load geometry: */
        m_geometry = gEDataManager->informationWindowGeometry(this, m_pMachineWindow, vboxGlobal().managedVMUuid());

        /* Restore geometry: */
        LogRel2(("GUI: UIVMInformationDialog: Restoring geometry to: Origin=%dx%d, Size=%dx%d\n",
                 m_geometry.x(), m_geometry.y(), m_geometry.width(), m_geometry.height()));
        restoreGeometry();
    }
}

void UIVMInformationDialog::saveSettings()
{
    /* Save window geometry: */
    {
        /* Save geometry: */
#ifdef VBOX_WS_MAC
        gEDataManager->setInformationWindowGeometry(m_geometry, ::darwinIsWindowMaximized(this), vboxGlobal().managedVMUuid());
#else /* VBOX_WS_MAC */
        gEDataManager->setInformationWindowGeometry(m_geometry, isMaximized(), vboxGlobal().managedVMUuid());
#endif /* !VBOX_WS_MAC */
        LogRel2(("GUI: UIVMInformationDialog: Geometry saved as: Origin=%dx%d, Size=%dx%d\n",
                 m_geometry.x(), m_geometry.y(), m_geometry.width(), m_geometry.height()));
    }
}

void UIVMInformationDialog::cleanup()
{
    /* Save settings: */
    saveSettings();
}

