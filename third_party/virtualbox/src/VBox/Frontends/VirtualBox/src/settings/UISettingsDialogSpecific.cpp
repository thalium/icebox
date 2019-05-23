/* $Id: UISettingsDialogSpecific.cpp $ */
/** @file
 * VBox Qt GUI - UISettingsDialogSpecific class implementation.
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

/* GUI includes: */
# include "UISettingsDialogSpecific.h"
# include "UISettingsSerializer.h"
# include "UISettingsDefs.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"
# include "UIExtraDataManager.h"
# include "QIWidgetValidator.h"
# include "UISettingsSelector.h"
# include "UIVirtualBoxEventHandler.h"

# include "UIGlobalSettingsGeneral.h"
# include "UIGlobalSettingsInput.h"
# ifdef VBOX_GUI_WITH_NETWORK_MANAGER
#  include "UIGlobalSettingsUpdate.h"
# endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
# include "UIGlobalSettingsLanguage.h"
# include "UIGlobalSettingsDisplay.h"
# include "UIGlobalSettingsNetwork.h"
# include "UIGlobalSettingsExtension.h"
# ifdef VBOX_GUI_WITH_NETWORK_MANAGER
#  include "UIGlobalSettingsProxy.h"
# endif /* VBOX_GUI_WITH_NETWORK_MANAGER */

# include "UIMachineSettingsGeneral.h"
# include "UIMachineSettingsSystem.h"
# include "UIMachineSettingsDisplay.h"
# include "UIMachineSettingsStorage.h"
# include "UIMachineSettingsAudio.h"
# include "UIMachineSettingsNetwork.h"
# include "UIMachineSettingsSerial.h"
# include "UIMachineSettingsUSB.h"
# include "UIMachineSettingsSF.h"
# include "UIMachineSettingsInterface.h"

/* COM includes: */
# include "CUSBController.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

#ifdef VBOX_WS_MAC
# define VBOX_GUI_WITH_TOOLBAR_SETTINGS
#endif /* VBOX_WS_MAC */

UISettingsDialogGlobal::UISettingsDialogGlobal(QWidget *pParent,
                                               const QString &strCategory /* = QString() */,
                                               const QString &strControl /* = QString() */)
    : UISettingsDialog(pParent)
{
    /* Window icon: */
#ifndef VBOX_WS_MAC
    setWindowIcon(QIcon(":/global_settings_16px.png"));
#endif /* !VBOX_WS_MAC */

    /* Creating settings pages: */
    QList<GlobalSettingsPageType> restrictedGlobalSettingsPages = gEDataManager->restrictedGlobalSettingsPages();
    for (int iPageIndex = GlobalSettingsPageType_General; iPageIndex < GlobalSettingsPageType_Max; ++iPageIndex)
    {
        /* Make sure page was not restricted: */
        if (restrictedGlobalSettingsPages.contains(static_cast<GlobalSettingsPageType>(iPageIndex)))
            continue;

        /* Make sure page is available: */
        if (isPageAvailable(iPageIndex))
        {
            UISettingsPage *pSettingsPage = 0;
            switch (iPageIndex)
            {
                /* General page: */
                case GlobalSettingsPageType_General:
                {
                    pSettingsPage = new UIGlobalSettingsGeneral;
                    addItem(":/machine_32px.png", ":/machine_24px.png", ":/machine_16px.png",
                            iPageIndex, "#general", pSettingsPage);
                    break;
                }
                /* Input page: */
                case GlobalSettingsPageType_Input:
                {
                    pSettingsPage = new UIGlobalSettingsInput;
                    addItem(":/keyboard_32px.png", ":/keyboard_24px.png", ":/keyboard_16px.png",
                            iPageIndex, "#input", pSettingsPage);
                    break;
                }
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
                /* Update page: */
                case GlobalSettingsPageType_Update:
                {
                    pSettingsPage = new UIGlobalSettingsUpdate;
                    addItem(":/refresh_32px.png", ":/refresh_24px.png", ":/refresh_16px.png",
                            iPageIndex, "#update", pSettingsPage);
                    break;
                }
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
                /* Language page: */
                case GlobalSettingsPageType_Language:
                {
                    pSettingsPage = new UIGlobalSettingsLanguage;
                    addItem(":/site_32px.png", ":/site_24px.png", ":/site_16px.png",
                            iPageIndex, "#language", pSettingsPage);
                    break;
                }
                /* Display page: */
                case GlobalSettingsPageType_Display:
                {
                    pSettingsPage = new UIGlobalSettingsDisplay;
                    addItem(":/vrdp_32px.png", ":/vrdp_24px.png", ":/vrdp_16px.png",
                            iPageIndex, "#display", pSettingsPage);
                    break;
                }
                /* Network page: */
                case GlobalSettingsPageType_Network:
                {
                    pSettingsPage = new UIGlobalSettingsNetwork;
                    addItem(":/nw_32px.png", ":/nw_24px.png", ":/nw_16px.png",
                            iPageIndex, "#network", pSettingsPage);
                    break;
                }
                /* Extensions page: */
                case GlobalSettingsPageType_Extensions:
                {
                    pSettingsPage = new UIGlobalSettingsExtension;
                    addItem(":/extension_pack_32px.png", ":/extension_pack_24px.png", ":/extension_pack_16px.png",
                            iPageIndex, "#extensions", pSettingsPage);
                    break;
                }
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
                /* Proxy page: */
                case GlobalSettingsPageType_Proxy:
                {
                    pSettingsPage = new UIGlobalSettingsProxy;
                    addItem(":/proxy_32px.png", ":/proxy_24px.png", ":/proxy_16px.png",
                            iPageIndex, "#proxy", pSettingsPage);
                    break;
                }
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
                default:
                    break;
            }
        }
    }

    /* Assign default (full) configuration access level: */
    setConfigurationAccessLevel(ConfigurationAccessLevel_Full);

    /* Retranslate UI: */
    retranslateUi();

    /* Setup settings window: */
    if (!strCategory.isNull())
    {
        m_pSelector->selectByLink(strCategory);
        /* Search for a widget with the given name: */
        if (!strControl.isNull())
        {
            if (QWidget *pWidget = m_pStack->findChild<QWidget*>(strControl))
            {
                QList<QWidget*> parents;
                QWidget *pParentWidget = pWidget;
                while ((pParentWidget = pParentWidget->parentWidget()) != 0)
                {
                    if (QTabWidget *pTabWidget = qobject_cast<QTabWidget*>(pParentWidget))
                    {
                        /* The tab contents widget is two steps down
                         * (QTabWidget -> QStackedWidget -> QWidget): */
                        QWidget *pTabPage = parents[parents.count() - 1];
                        if (pTabPage)
                            pTabPage = parents[parents.count() - 2];
                        if (pTabPage)
                            pTabWidget->setCurrentWidget(pTabPage);
                    }
                    parents.append(pParentWidget);
                }
                pWidget->setFocus();
            }
        }
    }
    /* First item as default: */
    else
        m_pSelector->selectById(GlobalSettingsPageType_General);
}

void UISettingsDialogGlobal::loadOwnData()
{
    /* Get properties: */
    CSystemProperties properties = vboxGlobal().virtualBox().GetSystemProperties();
    /* Prepare global data: */
    qRegisterMetaType<UISettingsDataGlobal>();
    UISettingsDataGlobal data(properties);
    QVariant varData = QVariant::fromValue(data);

    /* Call to base-class: */
    UISettingsDialog::loadData(varData);
}

void UISettingsDialogGlobal::saveOwnData()
{
    /* Get properties: */
    CSystemProperties properties = vboxGlobal().virtualBox().GetSystemProperties();
    /* Prepare global data: */
    qRegisterMetaType<UISettingsDataGlobal>();
    UISettingsDataGlobal data(properties);
    QVariant varData = QVariant::fromValue(data);

    /* Call to base-class: */
    UISettingsDialog::saveData(varData);

    /* Get updated properties: */
    CSystemProperties newProperties = varData.value<UISettingsDataGlobal>().m_properties;
    /* If properties are not OK => show the error: */
    if (!newProperties.isOk())
        msgCenter().cannotSetSystemProperties(newProperties, this);

    /* Mark as saved: */
    sltMarkSaved();
}

void UISettingsDialogGlobal::retranslateUi()
{
    /* Selector itself: */
    m_pSelector->widget()->setWhatsThis(tr("Allows to navigate through Global Property categories"));

    /* General page: */
    m_pSelector->setItemText(GlobalSettingsPageType_General, tr("General"));

    /* Input page: */
    m_pSelector->setItemText(GlobalSettingsPageType_Input, tr("Input"));

#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    /* Update page: */
    m_pSelector->setItemText(GlobalSettingsPageType_Update, tr("Update"));
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */

    /* Language page: */
    m_pSelector->setItemText(GlobalSettingsPageType_Language, tr("Language"));

    /* Display page: */
    m_pSelector->setItemText(GlobalSettingsPageType_Display, tr("Display"));

    /* Network page: */
    m_pSelector->setItemText(GlobalSettingsPageType_Network, tr("Network"));

    /* Extension page: */
    m_pSelector->setItemText(GlobalSettingsPageType_Extensions, tr("Extensions"));

#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    /* Proxy page: */
    m_pSelector->setItemText(GlobalSettingsPageType_Proxy, tr("Proxy"));
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */

    /* Polish the selector: */
    m_pSelector->polish();

    /* Base-class UI translation: */
    UISettingsDialog::retranslateUi();

    /* Set dialog's name: */
    setWindowTitle(title());
}

QString UISettingsDialogGlobal::titleExtension() const
{
#ifdef VBOX_GUI_WITH_TOOLBAR_SETTINGS
    return m_pSelector->itemText(m_pSelector->currentId());
#else /* VBOX_GUI_WITH_TOOLBAR_SETTINGS */
    return tr("Preferences");
#endif /* !VBOX_GUI_WITH_TOOLBAR_SETTINGS */
}

QString UISettingsDialogGlobal::title() const
{
    return tr("VirtualBox - %1").arg(titleExtension());
}

bool UISettingsDialogGlobal::isPageAvailable(int iPageId)
{
    switch (iPageId)
    {
        case GlobalSettingsPageType_Network:
        {
#ifndef VBOX_WITH_NETFLT
            return false;
#endif /* !VBOX_WITH_NETFLT */
            break;
        }
        default:
            break;
    }
    return true;
}

UISettingsDialogMachine::UISettingsDialogMachine(QWidget *pParent, const QString &strMachineId,
                                                 const QString &strCategory, const QString &strControl)
    : UISettingsDialog(pParent)
    , m_strMachineId(strMachineId)
    , m_fAllowResetFirstRunFlag(false)
    , m_fResetFirstRunFlag(false)
{
    /* Window icon: */
#ifndef VBOX_WS_MAC
    setWindowIcon(QIcon(":/vm_settings_16px.png"));
#endif /* VBOX_WS_MAC */

    /* Allow to reset first-run flag just when medium enumeration was finished: */
    connect(&vboxGlobal(), SIGNAL(sigMediumEnumerationFinished()), this, SLOT(sltAllowResetFirstRunFlag()));

    /* Make sure settings window will be updated on session/machine state/data changes: */
    connect(gVBoxEvents, SIGNAL(sigSessionStateChange(QString, KSessionState)),
            this, SLOT(sltSessionStateChanged(QString, KSessionState)));
    connect(gVBoxEvents, SIGNAL(sigMachineStateChange(QString, KMachineState)),
            this, SLOT(sltMachineStateChanged(QString, KMachineState)));
    connect(gVBoxEvents, SIGNAL(sigMachineDataChange(QString)),
            this, SLOT(sltMachineDataChanged(QString)));

    /* Get corresponding machine (required to determine dialog type and page availability): */
    m_machine = vboxGlobal().virtualBox().FindMachine(m_strMachineId);
    AssertMsg(!m_machine.isNull(), ("Can't find corresponding machine!\n"));
    m_sessionState = m_machine.GetSessionState();
    m_machineState = m_machine.GetState();

    /* Creating settings pages: */
    QList<MachineSettingsPageType> restrictedMachineSettingsPages = gEDataManager->restrictedMachineSettingsPages(m_strMachineId);
    for (int iPageIndex = MachineSettingsPageType_General; iPageIndex < MachineSettingsPageType_Max; ++iPageIndex)
    {
        /* Make sure page was not restricted: */
        if (restrictedMachineSettingsPages.contains(static_cast<MachineSettingsPageType>(iPageIndex)))
            continue;

        /* Make sure page is available: */
        if (isPageAvailable(iPageIndex))
        {
            UISettingsPage *pSettingsPage = 0;
            switch (iPageIndex)
            {
                /* General page: */
                case MachineSettingsPageType_General:
                {
                    pSettingsPage = new UIMachineSettingsGeneral;
                    addItem(":/machine_32px.png", ":/machine_24px.png", ":/machine_16px.png",
                            iPageIndex, "#general", pSettingsPage);
                    break;
                }
                /* System page: */
                case MachineSettingsPageType_System:
                {
                    pSettingsPage = new UIMachineSettingsSystem;
                    addItem(":/chipset_32px.png", ":/chipset_24px.png", ":/chipset_16px.png",
                            iPageIndex, "#system", pSettingsPage);
                    break;
                }
                /* Display page: */
                case MachineSettingsPageType_Display:
                {
                    pSettingsPage = new UIMachineSettingsDisplay;
                    addItem(":/vrdp_32px.png", ":/vrdp_24px.png", ":/vrdp_16px.png",
                            iPageIndex, "#display", pSettingsPage);
                    break;
                }
                /* Storage page: */
                case MachineSettingsPageType_Storage:
                {
                    pSettingsPage = new UIMachineSettingsStorage;
                    connect(pSettingsPage, SIGNAL(sigStorageChanged()), this, SLOT(sltResetFirstRunFlag()));
                    addItem(":/hd_32px.png", ":/hd_24px.png", ":/hd_16px.png",
                            iPageIndex, "#storage", pSettingsPage);
                    break;
                }
                /* Audio page: */
                case MachineSettingsPageType_Audio:
                {
                    pSettingsPage = new UIMachineSettingsAudio;
                    addItem(":/sound_32px.png", ":/sound_24px.png", ":/sound_16px.png",
                            iPageIndex, "#audio", pSettingsPage);
                    break;
                }
                /* Network page: */
                case MachineSettingsPageType_Network:
                {
                    pSettingsPage = new UIMachineSettingsNetworkPage;
                    addItem(":/nw_32px.png", ":/nw_24px.png", ":/nw_16px.png",
                            iPageIndex, "#network", pSettingsPage);
                    break;
                }
                /* Ports page: */
                case MachineSettingsPageType_Ports:
                {
                    addItem(":/serial_port_32px.png", ":/serial_port_24px.png", ":/serial_port_16px.png",
                            iPageIndex, "#ports");
                    break;
                }
                /* Serial page: */
                case MachineSettingsPageType_Serial:
                {
                    pSettingsPage = new UIMachineSettingsSerialPage;
                    addItem(":/serial_port_32px.png", ":/serial_port_24px.png", ":/serial_port_16px.png",
                            iPageIndex, "#serialPorts", pSettingsPage, MachineSettingsPageType_Ports);
                    break;
                }
                /* USB page: */
                case MachineSettingsPageType_USB:
                {
                    pSettingsPage = new UIMachineSettingsUSB;
                    addItem(":/usb_32px.png", ":/usb_24px.png", ":/usb_16px.png",
                            iPageIndex, "#usb", pSettingsPage, MachineSettingsPageType_Ports);
                    break;
                }
                /* Shared Folders page: */
                case MachineSettingsPageType_SF:
                {
                    pSettingsPage = new UIMachineSettingsSF;
                    addItem(":/sf_32px.png", ":/sf_24px.png", ":/sf_16px.png",
                            iPageIndex, "#sharedFolders", pSettingsPage);
                    break;
                }
                /* Interface page: */
                case MachineSettingsPageType_Interface:
                {
                    pSettingsPage = new UIMachineSettingsInterface(m_machine.GetId());
                    addItem(":/interface_32px.png", ":/interface_24px.png", ":/interface_16px.png",
                            iPageIndex, "#userInterface", pSettingsPage);
                    break;
                }
                default:
                    break;
            }
        }
    }

    /* Calculate initial configuration access level: */
    setConfigurationAccessLevel(::configurationAccessLevel(m_sessionState, m_machineState));

    /* Retranslate UI: */
    retranslateUi();

    /* Setup settings window: */
    if (!strCategory.isNull())
    {
        m_pSelector->selectByLink(strCategory);
        /* Search for a widget with the given name: */
        if (!strControl.isNull())
        {
            if (QWidget *pWidget = m_pStack->findChild<QWidget*>(strControl))
            {
                QList<QWidget*> parents;
                QWidget *pParentWidget = pWidget;
                while ((pParentWidget = pParentWidget->parentWidget()) != 0)
                {
                    if (QTabWidget *pTabWidget = qobject_cast<QTabWidget*>(pParentWidget))
                    {
                        /* The tab contents widget is two steps down
                         * (QTabWidget -> QStackedWidget -> QWidget): */
                        QWidget *pTabPage = parents[parents.count() - 1];
                        if (pTabPage)
                            pTabPage = parents[parents.count() - 2];
                        if (pTabPage)
                            pTabWidget->setCurrentWidget(pTabPage);
                    }
                    parents.append(pParentWidget);
                }
                pWidget->setFocus();
            }
        }
    }
    /* First item as default: */
    else
        m_pSelector->selectById(MachineSettingsPageType_General);
}

void UISettingsDialogMachine::loadOwnData()
{
    /* Check that session is NOT created: */
    if (!m_session.isNull())
        return;

    /* Prepare session: */
    m_session = configurationAccessLevel() == ConfigurationAccessLevel_Null ? CSession() :
                configurationAccessLevel() == ConfigurationAccessLevel_Full ? vboxGlobal().openSession(m_strMachineId) :
                                                                              vboxGlobal().openExistingSession(m_strMachineId);
    /* Check that session was created: */
    if (m_session.isNull())
        return;

    /* Get machine and console: */
    m_machine = m_session.GetMachine();
    m_console = configurationAccessLevel() == ConfigurationAccessLevel_Full ? CConsole() : m_session.GetConsole();
    /* Prepare machine data: */
    qRegisterMetaType<UISettingsDataMachine>();
    UISettingsDataMachine data(m_machine, m_console);
    QVariant varData = QVariant::fromValue(data);

    /* Call to base-class: */
    UISettingsDialog::loadData(varData);
}

void UISettingsDialogMachine::saveOwnData()
{
    /* Check that session is NOT created: */
    if (!m_session.isNull())
        return;

    /* Prepare session: */
    m_session = configurationAccessLevel() == ConfigurationAccessLevel_Null ? CSession() :
                configurationAccessLevel() == ConfigurationAccessLevel_Full ? vboxGlobal().openSession(m_strMachineId) :
                                                                              vboxGlobal().openExistingSession(m_strMachineId);
    /* Check that session was created: */
    if (m_session.isNull())
        return;

    /* Get machine and console: */
    m_machine = m_session.GetMachine();
    m_console = configurationAccessLevel() == ConfigurationAccessLevel_Full ? CConsole() : m_session.GetConsole();
    /* Prepare machine data: */
    qRegisterMetaType<UISettingsDataMachine>();
    UISettingsDataMachine data(m_machine, m_console);
    QVariant varData = QVariant::fromValue(data);

    /* Call to base-class: */
    UISettingsDialog::saveData(varData);

    /* Get updated machine: */
    m_machine = varData.value<UISettingsDataMachine>().m_machine;
    /* If machine is OK => perform final operations: */
    if (m_machine.isOk())
    {
        /* Guest OS type & VT-x/AMD-V option correlation auto-fix: */
        UIMachineSettingsGeneral *pGeneralPage =
            qobject_cast<UIMachineSettingsGeneral*>(m_pSelector->idToPage(MachineSettingsPageType_General));
        UIMachineSettingsSystem *pSystemPage =
            qobject_cast<UIMachineSettingsSystem*>(m_pSelector->idToPage(MachineSettingsPageType_System));
        if (pGeneralPage && pSystemPage &&
            pGeneralPage->is64BitOSTypeSelected() && !pSystemPage->isHWVirtExEnabled())
            m_machine.SetHWVirtExProperty(KHWVirtExPropertyType_Enabled, true);

#ifdef VBOX_WITH_VIDEOHWACCEL
        /* Disable 2D Video Acceleration for non-Windows guests: */
        if (pGeneralPage && !pGeneralPage->isWindowsOSTypeSelected())
        {
            UIMachineSettingsDisplay *pDisplayPage =
                qobject_cast<UIMachineSettingsDisplay*>(m_pSelector->idToPage(MachineSettingsPageType_Display));
            if (pDisplayPage && pDisplayPage->isAcceleration2DVideoSelected())
                m_machine.SetAccelerate2DVideoEnabled(false);
        }
#endif /* VBOX_WITH_VIDEOHWACCEL */

        /* Enable OHCI controller if HID is enabled but no USB controllers present: */
        if (pSystemPage && pSystemPage->isHIDEnabled() && m_machine.GetUSBControllers().isEmpty())
            m_machine.AddUSBController("OHCI", KUSBControllerType_OHCI);

        /* Disable First RUN Wizard: */
        if (m_fResetFirstRunFlag)
            gEDataManager->setMachineFirstTimeStarted(false, m_strMachineId);

        /* Save settings finally: */
        m_machine.SaveSettings();
    }

    /* If machine is NOT OK => show the error message: */
    if (!m_machine.isOk())
        msgCenter().cannotSaveMachineSettings(m_machine, this);

    /* Mark as saved: */
    sltMarkSaved();
}

void UISettingsDialogMachine::retranslateUi()
{
    /* Selector itself: */
    m_pSelector->widget()->setWhatsThis(tr("Allows to navigate through VM Settings categories"));

    /* We have to make sure that the Network, Serial pages are retranslated
     * before they are revalidated. Cause: They do string comparing within
     * vboxGlobal which is retranslated at that point already: */
    QEvent event(QEvent::LanguageChange);
    if (QWidget *pPage = m_pSelector->idToPage(MachineSettingsPageType_Network))
        qApp->sendEvent(pPage, &event);
    if (QWidget *pPage = m_pSelector->idToPage(MachineSettingsPageType_Serial))
        qApp->sendEvent(pPage, &event);

    /* General page: */
    m_pSelector->setItemText(MachineSettingsPageType_General, tr("General"));

    /* System page: */
    m_pSelector->setItemText(MachineSettingsPageType_System, tr("System"));

    /* Display page: */
    m_pSelector->setItemText(MachineSettingsPageType_Display, tr("Display"));

    /* Storage page: */
    m_pSelector->setItemText(MachineSettingsPageType_Storage, tr("Storage"));

    /* Audio page: */
    m_pSelector->setItemText(MachineSettingsPageType_Audio, tr("Audio"));

    /* Network page: */
    m_pSelector->setItemText(MachineSettingsPageType_Network, tr("Network"));

    /* Ports page: */
    m_pSelector->setItemText(MachineSettingsPageType_Ports, tr("Ports"));

    /* Serial page: */
    m_pSelector->setItemText(MachineSettingsPageType_Serial, tr("Serial Ports"));

    /* USB page: */
    m_pSelector->setItemText(MachineSettingsPageType_USB, tr("USB"));

    /* SFolders page: */
    m_pSelector->setItemText(MachineSettingsPageType_SF, tr("Shared Folders"));

    /* Interface page: */
    m_pSelector->setItemText(MachineSettingsPageType_Interface, tr("User Interface"));

    /* Polish the selector: */
    m_pSelector->polish();

    /* Base-class UI translation: */
    UISettingsDialog::retranslateUi();

    /* Set dialog's name: */
    setWindowTitle(title());
}

QString UISettingsDialogMachine::titleExtension() const
{
#ifdef VBOX_GUI_WITH_TOOLBAR_SETTINGS
    return m_pSelector->itemText(m_pSelector->currentId());
#else /* VBOX_GUI_WITH_TOOLBAR_SETTINGS */
    return tr("Settings");
#endif /* !VBOX_GUI_WITH_TOOLBAR_SETTINGS */
}

QString UISettingsDialogMachine::title() const
{
    QString strDialogTitle;
    /* Get corresponding machine (required to compose dialog title): */
    const CMachine &machine = vboxGlobal().virtualBox().FindMachine(m_strMachineId);
    if (!machine.isNull())
        strDialogTitle = tr("%1 - %2").arg(machine.GetName()).arg(titleExtension());
    return strDialogTitle;
}

void UISettingsDialogMachine::recorrelate(UISettingsPage *pSettingsPage)
{
    switch (pSettingsPage->id())
    {
        /* General page correlations: */
        case MachineSettingsPageType_General:
        {
            /* Make changes on 'general' page influent 'display' page: */
            UIMachineSettingsGeneral *pGeneralPage = qobject_cast<UIMachineSettingsGeneral*>(pSettingsPage);
            UIMachineSettingsDisplay *pDisplayPage = qobject_cast<UIMachineSettingsDisplay*>(m_pSelector->idToPage(MachineSettingsPageType_Display));
            if (pGeneralPage && pDisplayPage)
                pDisplayPage->setGuestOSType(pGeneralPage->guestOSType());
            break;
        }
        /* System page correlations: */
        case MachineSettingsPageType_System:
        {
            /* Make changes on 'system' page influent 'general' and 'storage' page: */
            UIMachineSettingsSystem *pSystemPage = qobject_cast<UIMachineSettingsSystem*>(pSettingsPage);
            UIMachineSettingsGeneral *pGeneralPage = qobject_cast<UIMachineSettingsGeneral*>(m_pSelector->idToPage(MachineSettingsPageType_General));
            UIMachineSettingsStorage *pStoragePage = qobject_cast<UIMachineSettingsStorage*>(m_pSelector->idToPage(MachineSettingsPageType_Storage));
            if (pSystemPage)
            {
                if (pGeneralPage)
                    pGeneralPage->setHWVirtExEnabled(pSystemPage->isHWVirtExEnabled());
                if (pStoragePage)
                    pStoragePage->setChipsetType(pSystemPage->chipsetType());
            }
            break;
        }
        /* USB page correlations: */
        case MachineSettingsPageType_USB:
        {
            /* Make changes on 'usb' page influent 'system' page: */
            UIMachineSettingsUSB *pUsbPage = qobject_cast<UIMachineSettingsUSB*>(pSettingsPage);
            UIMachineSettingsSystem *pSystemPage = qobject_cast<UIMachineSettingsSystem*>(m_pSelector->idToPage(MachineSettingsPageType_System));
            if (pUsbPage && pSystemPage)
                pSystemPage->setUSBEnabled(pUsbPage->isUSBEnabled());
            break;
        }
        default:
            break;
    }
}

void UISettingsDialogMachine::sltMarkLoaded()
{
    /* Call for base-class: */
    UISettingsDialog::sltMarkLoaded();

    /* No need to reset 'first run' flag: */
    m_fResetFirstRunFlag = false;

    /* Unlock the session if exists: */
    if (!m_session.isNull())
    {
        m_session.UnlockMachine();
        m_session = CSession();
        m_machine = CMachine();
        m_console = CConsole();
    }
}

void UISettingsDialogMachine::sltMarkSaved()
{
    /* Call for base-class: */
    UISettingsDialog::sltMarkSaved();

    /* Unlock the session if exists: */
    if (!m_session.isNull())
    {
        m_session.UnlockMachine();
        m_session = CSession();
        m_machine = CMachine();
        m_console = CConsole();
    }
}

void UISettingsDialogMachine::sltSessionStateChanged(QString strMachineId, KSessionState sessionState)
{
    /* Ignore if serialization is in progress: */
    if (isSerializationInProgress())
        return;

    /* Ignore if thats NOT our VM: */
    if (strMachineId != m_strMachineId)
        return;

    /* Ignore if state was NOT actually changed: */
    if (m_sessionState == sessionState)
        return;

    /* Update current session state: */
    m_sessionState = sessionState;

    /* Recalculate configuration access level: */
    updateConfigurationAccessLevel();
}

void UISettingsDialogMachine::sltMachineStateChanged(QString strMachineId, KMachineState machineState)
{
    /* Ignore if serialization is in progress: */
    if (isSerializationInProgress())
        return;

    /* Ignore if thats NOT our VM: */
    if (strMachineId != m_strMachineId)
        return;

    /* Ignore if state was NOT actually changed: */
    if (m_machineState == machineState)
        return;

    /* Update current machine state: */
    m_machineState = machineState;

    /* Recalculate configuration access level: */
    updateConfigurationAccessLevel();
}

void UISettingsDialogMachine::sltMachineDataChanged(QString strMachineId)
{
    /* Ignore if serialization is in progress: */
    if (isSerializationInProgress())
        return;

    /* Ignore if thats NOT our VM: */
    if (strMachineId != m_strMachineId)
        return;

    /* Check if user had changed something and warn him about he will loose settings on reloading: */
    if (isSettingsChanged() && !msgCenter().confirmSettingsReloading(this))
        return;

    /* Reload data: */
    loadOwnData();
}

void UISettingsDialogMachine::sltCategoryChanged(int cId)
{
    if (serializeProcess())
        serializeProcess()->raisePriorityOfPage(cId);

    UISettingsDialog::sltCategoryChanged(cId);
}

void UISettingsDialogMachine::sltAllowResetFirstRunFlag()
{
    m_fAllowResetFirstRunFlag = true;
}

void UISettingsDialogMachine::sltResetFirstRunFlag()
{
    if (m_fAllowResetFirstRunFlag)
        m_fResetFirstRunFlag = true;
}

bool UISettingsDialogMachine::isPageAvailable(int iPageId)
{
    if (m_machine.isNull())
        return false;

    switch (iPageId)
    {
        case MachineSettingsPageType_Serial:
        {
            /* Depends on ports availability: */
            if (!isPageAvailable(MachineSettingsPageType_Ports))
                return false;
            break;
        }
        case MachineSettingsPageType_USB:
        {
            /* Depends on ports availability: */
            if (!isPageAvailable(MachineSettingsPageType_Ports))
                return false;
            /* Check if USB is implemented: */
            if (!m_machine.GetUSBProxyAvailable())
                return false;
            /* Get the USB controller object: */
            CUSBControllerVector controllerColl = m_machine.GetUSBControllers();
            /* Show the machine error message if any: */
            if (   !m_machine.isReallyOk()
                && controllerColl.size() > 0
                && !m_machine.GetUSBControllers().isEmpty())
                msgCenter().warnAboutUnaccessibleUSB(m_machine, parentWidget());
            break;
        }
        default:
            break;
    }
    return true;
}

bool UISettingsDialogMachine::isSettingsChanged()
{
    bool fIsSettingsChanged = false;
    foreach (UISettingsPage *pPage, m_pSelector->settingPages())
    {
        pPage->putToCache();
        if (!fIsSettingsChanged && pPage->changed())
            fIsSettingsChanged = true;
    }
    return fIsSettingsChanged;
}

void UISettingsDialogMachine::updateConfigurationAccessLevel()
{
    /* Determine new configuration access level: */
    ConfigurationAccessLevel newConfigurationAccessLevel = ::configurationAccessLevel(m_sessionState, m_machineState);

    /* Make sure someting changed: */
    if (configurationAccessLevel() == newConfigurationAccessLevel)
        return;

    /* Should we warn a user about access level decrease? */
    bool fShouldWeWarn = configurationAccessLevel() == ConfigurationAccessLevel_Full;

    /* Apply new configuration access level: */
    setConfigurationAccessLevel(newConfigurationAccessLevel);

    /* Show a warning about access level decrease if we should: */
    if (isSettingsChanged() && fShouldWeWarn)
        msgCenter().warnAboutStateChange(this);
}

