/* $Id: UIConverterBackendGlobal.cpp $ */
/** @file
 * VBox Qt GUI - UIConverterBackendGlobal implementation.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
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
# include <QHash>
# include <QRegularExpression>

/* GUI includes: */
# include "UIConverterBackend.h"
# include "UIIconPool.h"
# include "VBoxGlobal.h"

/* COM includes: */
# include "CSystemProperties.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* Determines if <Object of type X> can be converted to object of other type.
 * These functions returns 'true' for all allowed conversions. */
template<> bool canConvert<SizeSuffix>() { return true; }
template<> bool canConvert<StorageSlot>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::MenuType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::MenuApplicationActionType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::MenuHelpActionType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuMachineActionType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuViewActionType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuInputActionType>() { return true; }
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuDevicesActionType>() { return true; }
#ifdef VBOX_WITH_DEBUGGER_GUI
template<> bool canConvert<UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType>() { return true; }
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
template<> bool canConvert<UIExtraDataMetaDefs::MenuWindowActionType>() { return true; }
#endif /* VBOX_WS_MAC */
template<> bool canConvert<ToolTypeMachine>() { return true; }
template<> bool canConvert<ToolTypeGlobal>() { return true; }
template<> bool canConvert<UIVisualStateType>() { return true; }
template<> bool canConvert<DetailsElementType>() { return true; }
template<> bool canConvert<PreviewUpdateIntervalType>() { return true; }
template<> bool canConvert<EventHandlingType>() { return true; }
template<> bool canConvert<GUIFeatureType>() { return true; }
template<> bool canConvert<GlobalSettingsPageType>() { return true; }
template<> bool canConvert<MachineSettingsPageType>() { return true; }
template<> bool canConvert<WizardType>() { return true; }
template<> bool canConvert<IndicatorType>() { return true; }
template<> bool canConvert<MachineCloseAction>() { return true; }
template<> bool canConvert<MouseCapturePolicy>() { return true; }
template<> bool canConvert<GuruMeditationHandlerType>() { return true; }
template<> bool canConvert<ScalingOptimizationType>() { return true; }
template<> bool canConvert<HiDPIOptimizationType>() { return true; }
#ifndef VBOX_WS_MAC
template<> bool canConvert<MiniToolbarAlignment>() { return true; }
#endif /* !VBOX_WS_MAC */
template<> bool canConvert<InformationElementType>() { return true; }
template<> bool canConvert<MaxGuestResolutionPolicy>() { return true; }

/* QString <= SizeSuffix: */
template<> QString toString(const SizeSuffix &sizeSuffix)
{
    QString strResult;
    switch (sizeSuffix)
    {
        case SizeSuffix_Byte:     strResult = QApplication::translate("VBoxGlobal", "B", "size suffix Bytes"); break;
        case SizeSuffix_KiloByte: strResult = QApplication::translate("VBoxGlobal", "KB", "size suffix KBytes=1024 Bytes"); break;
        case SizeSuffix_MegaByte: strResult = QApplication::translate("VBoxGlobal", "MB", "size suffix MBytes=1024 KBytes"); break;
        case SizeSuffix_GigaByte: strResult = QApplication::translate("VBoxGlobal", "GB", "size suffix GBytes=1024 MBytes"); break;
        case SizeSuffix_TeraByte: strResult = QApplication::translate("VBoxGlobal", "TB", "size suffix TBytes=1024 GBytes"); break;
        case SizeSuffix_PetaByte: strResult = QApplication::translate("VBoxGlobal", "PB", "size suffix PBytes=1024 TBytes"); break;
        default:
        {
            AssertMsgFailed(("No text for size suffix=%d", sizeSuffix));
            break;
        }
    }
    return strResult;
}

/* SizeSuffix <= QString: */
template<> SizeSuffix fromString<SizeSuffix>(const QString &strSizeSuffix)
{
    QHash<QString, SizeSuffix> list;
    list.insert(QApplication::translate("VBoxGlobal", "B", "size suffix Bytes"),               SizeSuffix_Byte);
    list.insert(QApplication::translate("VBoxGlobal", "KB", "size suffix KBytes=1024 Bytes"),  SizeSuffix_KiloByte);
    list.insert(QApplication::translate("VBoxGlobal", "MB", "size suffix MBytes=1024 KBytes"), SizeSuffix_MegaByte);
    list.insert(QApplication::translate("VBoxGlobal", "GB", "size suffix GBytes=1024 MBytes"), SizeSuffix_GigaByte);
    list.insert(QApplication::translate("VBoxGlobal", "TB", "size suffix TBytes=1024 GBytes"), SizeSuffix_TeraByte);
    list.insert(QApplication::translate("VBoxGlobal", "PB", "size suffix PBytes=1024 TBytes"), SizeSuffix_PetaByte);
    if (!list.contains(strSizeSuffix))
    {
        AssertMsgFailed(("No value for '%s'", strSizeSuffix.toUtf8().constData()));
    }
    return list.value(strSizeSuffix);
}

/* QString <= StorageSlot: */
template<> QString toString(const StorageSlot &storageSlot)
{
    QString strResult;
    switch (storageSlot.bus)
    {
        case KStorageBus_IDE:
        {
            int iMaxPort = vboxGlobal().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            int iMaxDevice = vboxGlobal().virtualBox().GetSystemProperties().GetMaxDevicesPerPortForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device < 0 || storageSlot.device > iMaxDevice)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            if (storageSlot.port == 0 && storageSlot.device == 0)
                strResult = QApplication::translate("VBoxGlobal", "IDE Primary Master", "StorageSlot");
            else if (storageSlot.port == 0 && storageSlot.device == 1)
                strResult = QApplication::translate("VBoxGlobal", "IDE Primary Slave", "StorageSlot");
            else if (storageSlot.port == 1 && storageSlot.device == 0)
                strResult = QApplication::translate("VBoxGlobal", "IDE Secondary Master", "StorageSlot");
            else if (storageSlot.port == 1 && storageSlot.device == 1)
                strResult = QApplication::translate("VBoxGlobal", "IDE Secondary Slave", "StorageSlot");
            break;
        }
        case KStorageBus_SATA:
        {
            int iMaxPort = vboxGlobal().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("VBoxGlobal", "SATA Port %1", "StorageSlot").arg(storageSlot.port);
            break;
        }
        case KStorageBus_SCSI:
        {
            int iMaxPort = vboxGlobal().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("VBoxGlobal", "SCSI Port %1", "StorageSlot").arg(storageSlot.port);
            break;
        }
        case KStorageBus_SAS:
        {
            int iMaxPort = vboxGlobal().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("VBoxGlobal", "SAS Port %1", "StorageSlot").arg(storageSlot.port);
            break;
        }
        case KStorageBus_Floppy:
        {
            int iMaxDevice = vboxGlobal().virtualBox().GetSystemProperties().GetMaxDevicesPerPortForStorageBus(storageSlot.bus);
            if (storageSlot.port != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device < 0 || storageSlot.device > iMaxDevice)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("VBoxGlobal", "Floppy Device %1", "StorageSlot").arg(storageSlot.device);
            break;
        }
        case KStorageBus_USB:
        {
            int iMaxPort = vboxGlobal().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("VBoxGlobal", "USB Port %1", "StorageSlot").arg(storageSlot.port);
            break;
        }
        case KStorageBus_PCIe:
        {
            int iMaxPort = vboxGlobal().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(storageSlot.bus);
            if (storageSlot.port < 0 || storageSlot.port > iMaxPort)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d", storageSlot.bus, storageSlot.port));
                break;
            }
            if (storageSlot.device != 0)
            {
                AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
                break;
            }
            strResult = QApplication::translate("VBoxGlobal", "NVMe Port %1", "StorageSlot").arg(storageSlot.port);
            break;
        }
        default:
        {
            AssertMsgFailed(("No text for bus=%d & port=%d & device=%d", storageSlot.bus, storageSlot.port, storageSlot.device));
            break;
        }
    }
    return strResult;
}

/* StorageSlot <= QString: */
template<> StorageSlot fromString<StorageSlot>(const QString &strStorageSlot)
{
    QHash<int, QString> list;
    list[0] = QApplication::translate("VBoxGlobal", "IDE Primary Master", "StorageSlot");
    list[1] = QApplication::translate("VBoxGlobal", "IDE Primary Slave", "StorageSlot");
    list[2] = QApplication::translate("VBoxGlobal", "IDE Secondary Master", "StorageSlot");
    list[3] = QApplication::translate("VBoxGlobal", "IDE Secondary Slave", "StorageSlot");
    list[4] = QApplication::translate("VBoxGlobal", "SATA Port %1", "StorageSlot");
    list[5] = QApplication::translate("VBoxGlobal", "SCSI Port %1", "StorageSlot");
    list[6] = QApplication::translate("VBoxGlobal", "SAS Port %1", "StorageSlot");
    list[7] = QApplication::translate("VBoxGlobal", "Floppy Device %1", "StorageSlot");
    list[8] = QApplication::translate("VBoxGlobal", "USB Port %1", "StorageSlot");
    int index = -1;
    QRegExp regExp;
    for (int i = 0; i < list.size(); ++i)
    {
        regExp = QRegExp(i >= 0 && i <= 3 ? list[i] : list[i].arg("(\\d+)"));
        if (regExp.indexIn(strStorageSlot) != -1)
        {
            index = i;
            break;
        }
    }

    StorageSlot result;
    switch (index)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        {
            KStorageBus bus = KStorageBus_IDE;
            int iMaxPort = vboxGlobal().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(bus);
            int iMaxDevice = vboxGlobal().virtualBox().GetSystemProperties().GetMaxDevicesPerPortForStorageBus(bus);
            LONG iPort = index / iMaxPort;
            LONG iDevice = index % iMaxPort;
            if (iPort < 0 || iPort > iMaxPort)
            {
                AssertMsgFailed(("No storage slot for text='%s'", strStorageSlot.toUtf8().constData()));
                break;
            }
            if (iDevice < 0 || iDevice > iMaxDevice)
            {
                AssertMsgFailed(("No storage slot for text='%s'", strStorageSlot.toUtf8().constData()));
                break;
            }
            result.bus = bus;
            result.port = iPort;
            result.device = iDevice;
            break;
        }
        case 4:
        {
            KStorageBus bus = KStorageBus_SATA;
            int iMaxPort = vboxGlobal().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(bus);
            LONG iPort = regExp.cap(1).toInt();
            LONG iDevice = 0;
            if (iPort < 0 || iPort > iMaxPort)
            {
                AssertMsgFailed(("No storage slot for text='%s'", strStorageSlot.toUtf8().constData()));
                break;
            }
            result.bus = bus;
            result.port = iPort;
            result.device = iDevice;
            break;
        }
        case 5:
        {
            KStorageBus bus = KStorageBus_SCSI;
            int iMaxPort = vboxGlobal().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(bus);
            LONG iPort = regExp.cap(1).toInt();
            LONG iDevice = 0;
            if (iPort < 0 || iPort > iMaxPort)
            {
                AssertMsgFailed(("No storage slot for text='%s'", strStorageSlot.toUtf8().constData()));
                break;
            }
            result.bus = bus;
            result.port = iPort;
            result.device = iDevice;
            break;
        }
        case 6:
        {
            KStorageBus bus = KStorageBus_SAS;
            int iMaxPort = vboxGlobal().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(bus);
            LONG iPort = regExp.cap(1).toInt();
            LONG iDevice = 0;
            if (iPort < 0 || iPort > iMaxPort)
            {
                AssertMsgFailed(("No storage slot for text='%s'", strStorageSlot.toUtf8().constData()));
                break;
            }
            result.bus = bus;
            result.port = iPort;
            result.device = iDevice;
            break;
        }
        case 7:
        {
            KStorageBus bus = KStorageBus_Floppy;
            int iMaxDevice = vboxGlobal().virtualBox().GetSystemProperties().GetMaxDevicesPerPortForStorageBus(bus);
            LONG iPort = 0;
            LONG iDevice = regExp.cap(1).toInt();
            if (iDevice < 0 || iDevice > iMaxDevice)
            {
                AssertMsgFailed(("No storage slot for text='%s'", strStorageSlot.toUtf8().constData()));
                break;
            }
            result.bus = bus;
            result.port = iPort;
            result.device = iDevice;
            break;
        }
        case 8:
        {
            KStorageBus bus = KStorageBus_USB;
            int iMaxPort = vboxGlobal().virtualBox().GetSystemProperties().GetMaxPortCountForStorageBus(bus);
            LONG iPort = regExp.cap(1).toInt();
            LONG iDevice = 0;
            if (iPort < 0 || iPort > iMaxPort)
            {
                AssertMsgFailed(("No storage slot for text='%s'", strStorageSlot.toUtf8().constData()));
                break;
            }
            result.bus = bus;
            result.port = iPort;
            result.device = iDevice;
            break;
        }
        default:
        {
            AssertMsgFailed(("No storage slot for text='%s'", strStorageSlot.toUtf8().constData()));
            break;
        }
    }
    return result;
}

/* QString <= UIExtraDataMetaDefs::MenuType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::MenuType &menuType)
{
    QString strResult;
    switch (menuType)
    {
        case UIExtraDataMetaDefs::MenuType_Application: strResult = "Application"; break;
        case UIExtraDataMetaDefs::MenuType_Machine:     strResult = "Machine"; break;
        case UIExtraDataMetaDefs::MenuType_View:        strResult = "View"; break;
        case UIExtraDataMetaDefs::MenuType_Input:       strResult = "Input"; break;
        case UIExtraDataMetaDefs::MenuType_Devices:     strResult = "Devices"; break;
#ifdef VBOX_WITH_DEBUGGER_GUI
        case UIExtraDataMetaDefs::MenuType_Debug:       strResult = "Debug"; break;
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef RT_OS_DARWIN
        case UIExtraDataMetaDefs::MenuType_Window:      strResult = "Window"; break;
#endif /* RT_OS_DARWIN */
        case UIExtraDataMetaDefs::MenuType_Help:        strResult = "Help"; break;
        case UIExtraDataMetaDefs::MenuType_All:         strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for indicator type=%d", menuType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::MenuType <= QString: */
template<> UIExtraDataMetaDefs::MenuType fromInternalString<UIExtraDataMetaDefs::MenuType>(const QString &strMenuType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;      QList<UIExtraDataMetaDefs::MenuType> values;
    keys << "Application"; values << UIExtraDataMetaDefs::MenuType_Application;
    keys << "Machine";     values << UIExtraDataMetaDefs::MenuType_Machine;
    keys << "View";        values << UIExtraDataMetaDefs::MenuType_View;
    keys << "Input";       values << UIExtraDataMetaDefs::MenuType_Input;
    keys << "Devices";     values << UIExtraDataMetaDefs::MenuType_Devices;
#ifdef VBOX_WITH_DEBUGGER_GUI
    keys << "Debug";       values << UIExtraDataMetaDefs::MenuType_Debug;
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef RT_OS_DARWIN
    keys << "Window";      values << UIExtraDataMetaDefs::MenuType_Window;
#endif /* RT_OS_DARWIN */
    keys << "Help";        values << UIExtraDataMetaDefs::MenuType_Help;
    keys << "All";         values << UIExtraDataMetaDefs::MenuType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strMenuType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::MenuType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMenuType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::MenuApplicationActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::MenuApplicationActionType &menuApplicationActionType)
{
    QString strResult;
    switch (menuApplicationActionType)
    {
#ifdef VBOX_WS_MAC
        case UIExtraDataMetaDefs::MenuApplicationActionType_About:                strResult = "About"; break;
#endif /* VBOX_WS_MAC */
        case UIExtraDataMetaDefs::MenuApplicationActionType_Preferences:          strResult = "Preferences"; break;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
        case UIExtraDataMetaDefs::MenuApplicationActionType_NetworkAccessManager: strResult = "NetworkAccessManager"; break;
        case UIExtraDataMetaDefs::MenuApplicationActionType_CheckForUpdates:      strResult = "CheckForUpdates"; break;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
        case UIExtraDataMetaDefs::MenuApplicationActionType_ResetWarnings:        strResult = "ResetWarnings"; break;
        case UIExtraDataMetaDefs::MenuApplicationActionType_Close:                strResult = "Close"; break;
        case UIExtraDataMetaDefs::MenuApplicationActionType_All:                  strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", menuApplicationActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::MenuApplicationActionType <= QString: */
template<> UIExtraDataMetaDefs::MenuApplicationActionType fromInternalString<UIExtraDataMetaDefs::MenuApplicationActionType>(const QString &strMenuApplicationActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;               QList<UIExtraDataMetaDefs::MenuApplicationActionType> values;
#ifdef VBOX_WS_MAC
    keys << "About";                values << UIExtraDataMetaDefs::MenuApplicationActionType_About;
#endif /* VBOX_WS_MAC */
    keys << "Preferences";          values << UIExtraDataMetaDefs::MenuApplicationActionType_Preferences;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    keys << "NetworkAccessManager"; values << UIExtraDataMetaDefs::MenuApplicationActionType_NetworkAccessManager;
    keys << "CheckForUpdates";      values << UIExtraDataMetaDefs::MenuApplicationActionType_CheckForUpdates;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
    keys << "ResetWarnings";        values << UIExtraDataMetaDefs::MenuApplicationActionType_ResetWarnings;
    keys << "Close";                values << UIExtraDataMetaDefs::MenuApplicationActionType_Close;
    keys << "All";                  values << UIExtraDataMetaDefs::MenuApplicationActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strMenuApplicationActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::MenuApplicationActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMenuApplicationActionType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::MenuHelpActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::MenuHelpActionType &menuHelpActionType)
{
    QString strResult;
    switch (menuHelpActionType)
    {
        case UIExtraDataMetaDefs::MenuHelpActionType_Contents:             strResult = "Contents"; break;
        case UIExtraDataMetaDefs::MenuHelpActionType_WebSite:              strResult = "WebSite"; break;
        case UIExtraDataMetaDefs::MenuHelpActionType_BugTracker:           strResult = "BugTracker"; break;
        case UIExtraDataMetaDefs::MenuHelpActionType_Forums:               strResult = "Forums"; break;
        case UIExtraDataMetaDefs::MenuHelpActionType_Oracle:               strResult = "Oracle"; break;
#ifndef VBOX_WS_MAC
        case UIExtraDataMetaDefs::MenuHelpActionType_About:                strResult = "About"; break;
#endif /* !VBOX_WS_MAC */
        case UIExtraDataMetaDefs::MenuHelpActionType_All:                  strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", menuHelpActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::MenuHelpActionType <= QString: */
template<> UIExtraDataMetaDefs::MenuHelpActionType fromInternalString<UIExtraDataMetaDefs::MenuHelpActionType>(const QString &strMenuHelpActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;               QList<UIExtraDataMetaDefs::MenuHelpActionType> values;
    keys << "Contents";             values << UIExtraDataMetaDefs::MenuHelpActionType_Contents;
    keys << "WebSite";              values << UIExtraDataMetaDefs::MenuHelpActionType_WebSite;
    keys << "BugTracker";           values << UIExtraDataMetaDefs::MenuHelpActionType_BugTracker;
    keys << "Forums";               values << UIExtraDataMetaDefs::MenuHelpActionType_Forums;
    keys << "Oracle";               values << UIExtraDataMetaDefs::MenuHelpActionType_Oracle;
#ifndef VBOX_WS_MAC
    keys << "About";                values << UIExtraDataMetaDefs::MenuHelpActionType_About;
#endif /* !VBOX_WS_MAC */
    keys << "All";                  values << UIExtraDataMetaDefs::MenuHelpActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strMenuHelpActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::MenuHelpActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMenuHelpActionType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::RuntimeMenuMachineActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuMachineActionType &runtimeMenuMachineActionType)
{
    QString strResult;
    switch (runtimeMenuMachineActionType)
    {
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SettingsDialog:    strResult = "SettingsDialog"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_TakeSnapshot:      strResult = "TakeSnapshot"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_InformationDialog: strResult = "InformationDialog"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Pause:             strResult = "Pause"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Reset:             strResult = "Reset"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Detach:            strResult = "Detach"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SaveState:         strResult = "SaveState"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Shutdown:          strResult = "Shutdown"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_PowerOff:          strResult = "PowerOff"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Nothing:           strResult = "Nothing"; break;
        case UIExtraDataMetaDefs::RuntimeMenuMachineActionType_All:               strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", runtimeMenuMachineActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::RuntimeMenuMachineActionType <= QString: */
template<> UIExtraDataMetaDefs::RuntimeMenuMachineActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuMachineActionType>(const QString &strRuntimeMenuMachineActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;            QList<UIExtraDataMetaDefs::RuntimeMenuMachineActionType> values;
    keys << "SettingsDialog";    values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SettingsDialog;
    keys << "TakeSnapshot";      values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_TakeSnapshot;
    keys << "InformationDialog"; values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_InformationDialog;
    keys << "Pause";             values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Pause;
    keys << "Reset";             values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Reset;
    keys << "Detach";            values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Detach;
    keys << "SaveState";         values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_SaveState;
    keys << "Shutdown";          values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Shutdown;
    keys << "PowerOff";          values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_PowerOff;
    keys << "Nothing";           values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Nothing;
    keys << "All";               values << UIExtraDataMetaDefs::RuntimeMenuMachineActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strRuntimeMenuMachineActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strRuntimeMenuMachineActionType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::RuntimeMenuViewActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuViewActionType &runtimeMenuViewActionType)
{
    QString strResult;
    switch (runtimeMenuViewActionType)
    {
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_Fullscreen:           strResult = "Fullscreen"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_Seamless:             strResult = "Seamless"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_Scale:                strResult = "Scale"; break;
#ifndef VBOX_WS_MAC
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_MinimizeWindow:       strResult = "MinimizeWindow"; break;
#endif /* !VBOX_WS_MAC */
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_AdjustWindow:         strResult = "AdjustWindow"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_GuestAutoresize:      strResult = "GuestAutoresize"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_TakeScreenshot:       strResult = "TakeScreenshot"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_VideoCapture:         strResult = "VideoCapture"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_VideoCaptureSettings: strResult = "VideoCaptureSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_StartVideoCapture:    strResult = "StartVideoCapture"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_VRDEServer:           strResult = "VRDEServer"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBar:              strResult = "MenuBar"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBarSettings:      strResult = "MenuBarSettings"; break;
#ifndef VBOX_WS_MAC
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleMenuBar:        strResult = "ToggleMenuBar"; break;
#endif /* !VBOX_WS_MAC */
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBar:            strResult = "StatusBar"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBarSettings:    strResult = "StatusBarSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleStatusBar:      strResult = "ToggleStatusBar"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_ScaleFactor:          strResult = "ScaleFactor"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_Resize:               strResult = "Resize"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_Multiscreen:          strResult = "Multiscreen"; break;
        case UIExtraDataMetaDefs::RuntimeMenuViewActionType_All:                  strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", runtimeMenuViewActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::RuntimeMenuViewActionType <= QString: */
template<> UIExtraDataMetaDefs::RuntimeMenuViewActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuViewActionType>(const QString &strRuntimeMenuViewActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;               QList<UIExtraDataMetaDefs::RuntimeMenuViewActionType> values;
    keys << "Fullscreen";           values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_Fullscreen;
    keys << "Seamless";             values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_Seamless;
    keys << "Scale";                values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_Scale;
#ifndef VBOX_WS_MAC
    keys << "MinimizeWindow";       values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_MinimizeWindow;
#endif /* !VBOX_WS_MAC */
    keys << "AdjustWindow";         values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_AdjustWindow;
    keys << "GuestAutoresize";      values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_GuestAutoresize;
    keys << "TakeScreenshot";       values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_TakeScreenshot;
    keys << "VideoCapture";         values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_VideoCapture;
    keys << "VideoCaptureSettings"; values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_VideoCaptureSettings;
    keys << "StartVideoCapture";    values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_StartVideoCapture;
    keys << "VRDEServer";           values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_VRDEServer;
    keys << "MenuBar";              values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBar;
    keys << "MenuBarSettings";      values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_MenuBarSettings;
#ifndef VBOX_WS_MAC
    keys << "ToggleMenuBar";        values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleMenuBar;
#endif /* !VBOX_WS_MAC */
    keys << "StatusBar";            values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBar;
    keys << "StatusBarSettings";    values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_StatusBarSettings;
    keys << "ToggleStatusBar";      values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_ToggleStatusBar;
    keys << "ScaleFactor";          values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_ScaleFactor;
    keys << "Resize";               values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_Resize;
    keys << "Multiscreen";          values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_Multiscreen;
    keys << "All";                  values << UIExtraDataMetaDefs::RuntimeMenuViewActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strRuntimeMenuViewActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::RuntimeMenuViewActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strRuntimeMenuViewActionType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::RuntimeMenuInputActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuInputActionType &runtimeMenuInputActionType)
{
    QString strResult;
    switch (runtimeMenuInputActionType)
    {
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_Keyboard:           strResult = "Keyboard"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_KeyboardSettings:   strResult = "KeyboardSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCAD:            strResult = "TypeCAD"; break;
#ifdef VBOX_WS_X11
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCABS:           strResult = "TypeCABS"; break;
#endif /* VBOX_WS_X11 */
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCtrlBreak:      strResult = "TypeCtrlBreak"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeInsert:         strResult = "TypeInsert"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypePrintScreen:    strResult = "TypePrintScreen"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeAltPrintScreen: strResult = "TypeAltPrintScreen"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_Mouse:              strResult = "Mouse"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_MouseIntegration:   strResult = "MouseIntegration"; break;
        case UIExtraDataMetaDefs::RuntimeMenuInputActionType_All:                strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", runtimeMenuInputActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::RuntimeMenuInputActionType <= QString: */
template<> UIExtraDataMetaDefs::RuntimeMenuInputActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuInputActionType>(const QString &strRuntimeMenuInputActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;             QList<UIExtraDataMetaDefs::RuntimeMenuInputActionType> values;
    keys << "Keyboard";           values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_Keyboard;
    keys << "KeyboardSettings";   values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_KeyboardSettings;
    keys << "TypeCAD";            values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCAD;
#ifdef VBOX_WS_X11
    keys << "TypeCABS";           values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCABS;
#endif /* VBOX_WS_X11 */
    keys << "TypeCtrlBreak";      values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeCtrlBreak;
    keys << "TypeInsert";         values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeInsert;
    keys << "TypePrintScreen";    values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypePrintScreen;
    keys << "TypeAltPrintScreen"; values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_TypeAltPrintScreen;
    keys << "Mouse";              values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_Mouse;
    keys << "MouseIntegration";   values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_MouseIntegration;
    keys << "All";                values << UIExtraDataMetaDefs::RuntimeMenuInputActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strRuntimeMenuInputActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::RuntimeMenuInputActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strRuntimeMenuInputActionType, Qt::CaseInsensitive)));
}

/* QString <= UIExtraDataMetaDefs::RuntimeMenuDevicesActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuDevicesActionType &runtimeMenuDevicesActionType)
{
    QString strResult;
    switch (runtimeMenuDevicesActionType)
    {
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrives:            strResult = "HardDrives"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrivesSettings:    strResult = "HardDrivesSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_OpticalDevices:        strResult = "OpticalDevices"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_FloppyDevices:         strResult = "FloppyDevices"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Audio:                 strResult = "Audio"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioOutput:           strResult = "AudioOutput"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioInput:            strResult = "AudioInput"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Network:               strResult = "Network"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_NetworkSettings:       strResult = "NetworkSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevices:            strResult = "USBDevices"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevicesSettings:    strResult = "USBDevicesSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_WebCams:               strResult = "WebCams"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedClipboard:       strResult = "SharedClipboard"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_DragAndDrop:           strResult = "DragAndDrop"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFolders:         strResult = "SharedFolders"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFoldersSettings: strResult = "SharedFoldersSettings"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_InstallGuestTools:     strResult = "InstallGuestTools"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Nothing:               strResult = "Nothing"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_All:                   strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", runtimeMenuDevicesActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::RuntimeMenuDevicesActionType <= QString: */
template<> UIExtraDataMetaDefs::RuntimeMenuDevicesActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuDevicesActionType>(const QString &strRuntimeMenuDevicesActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;                QList<UIExtraDataMetaDefs::RuntimeMenuDevicesActionType> values;
    keys << "HardDrives";            values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrives;
    keys << "HardDrivesSettings";    values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_HardDrivesSettings;
    keys << "OpticalDevices";        values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_OpticalDevices;
    keys << "FloppyDevices";         values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_FloppyDevices;
    keys << "Audio";                 values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Audio;
    keys << "AudioOutput";           values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioOutput;
    keys << "AudioInput";            values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_AudioInput;
    keys << "Network";               values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Network;
    keys << "NetworkSettings";       values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_NetworkSettings;
    keys << "USBDevices";            values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevices;
    keys << "USBDevicesSettings";    values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_USBDevicesSettings;
    keys << "WebCams";               values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_WebCams;
    keys << "SharedClipboard";       values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedClipboard;
    keys << "DragAndDrop";           values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_DragAndDrop;
    keys << "SharedFolders";         values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFolders;
    keys << "SharedFoldersSettings"; values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_SharedFoldersSettings;
    keys << "InstallGuestTools";     values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_InstallGuestTools;
    keys << "Nothing";               values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Nothing;
    keys << "All";                   values << UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strRuntimeMenuDevicesActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strRuntimeMenuDevicesActionType, Qt::CaseInsensitive)));
}

#ifdef VBOX_WITH_DEBUGGER_GUI
/* QString <= UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType &runtimeMenuDebuggerActionType)
{
    QString strResult;
    switch (runtimeMenuDebuggerActionType)
    {
        case UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Statistics:  strResult = "Statistics"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_CommandLine: strResult = "CommandLine"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Logging:     strResult = "Logging"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_LogDialog:   strResult = "LogDialog"; break;
        case UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_All:         strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", runtimeMenuDebuggerActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType <= QString: */
template<> UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType fromInternalString<UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType>(const QString &strRuntimeMenuDebuggerActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;      QList<UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType> values;
    keys << "Statistics";  values << UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Statistics;
    keys << "CommandLine"; values << UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_CommandLine;
    keys << "Logging";     values << UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Logging;
    keys << "LogDialog";   values << UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_LogDialog;
    keys << "All";         values << UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strRuntimeMenuDebuggerActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strRuntimeMenuDebuggerActionType, Qt::CaseInsensitive)));
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
/* QString <= UIExtraDataMetaDefs::MenuWindowActionType: */
template<> QString toInternalString(const UIExtraDataMetaDefs::MenuWindowActionType &menuWindowActionType)
{
    QString strResult;
    switch (menuWindowActionType)
    {
        case UIExtraDataMetaDefs::MenuWindowActionType_Minimize: strResult = "Minimize"; break;
        case UIExtraDataMetaDefs::MenuWindowActionType_Switch:   strResult = "Switch"; break;
        case UIExtraDataMetaDefs::MenuWindowActionType_All:      strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for action type=%d", menuWindowActionType));
            break;
        }
    }
    return strResult;
}

/* UIExtraDataMetaDefs::MenuWindowActionType <= QString: */
template<> UIExtraDataMetaDefs::MenuWindowActionType fromInternalString<UIExtraDataMetaDefs::MenuWindowActionType>(const QString &strMenuWindowActionType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;   QList<UIExtraDataMetaDefs::MenuWindowActionType> values;
    keys << "Minimize"; values << UIExtraDataMetaDefs::MenuWindowActionType_Minimize;
    keys << "Switch";   values << UIExtraDataMetaDefs::MenuWindowActionType_Switch;
    keys << "All";      values << UIExtraDataMetaDefs::MenuWindowActionType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strMenuWindowActionType, Qt::CaseInsensitive))
        return UIExtraDataMetaDefs::MenuWindowActionType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMenuWindowActionType, Qt::CaseInsensitive)));
}
#endif /* VBOX_WS_MAC */

/* QString <= ToolTypeMachine: */
template<> QString toInternalString(const ToolTypeMachine &enmToolTypeMachine)
{
    QString strResult;
    switch (enmToolTypeMachine)
    {
        case ToolTypeMachine_Invalid:   strResult = "None"; break;
        case ToolTypeMachine_Details:   strResult = "Details"; break;
        case ToolTypeMachine_Snapshots: strResult = "Snapshots"; break;
        default:
        {
            AssertMsgFailed(("No text for machine tool type=%d", enmToolTypeMachine));
            break;
        }
    }
    return strResult;
}

/* ToolTypeMachine <= QString: */
template<> ToolTypeMachine fromInternalString<ToolTypeMachine>(const QString &strToolTypeMachine)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;    QList<ToolTypeMachine> values;
    keys << "None";      values << ToolTypeMachine_Invalid;
    keys << "Details";   values << ToolTypeMachine_Details;
    keys << "Snapshots"; values << ToolTypeMachine_Snapshots;
    /* Invalid type for unknown words: */
    if (!keys.contains(strToolTypeMachine, Qt::CaseInsensitive))
        return ToolTypeMachine_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strToolTypeMachine, Qt::CaseInsensitive)));
}

/* QString <= ToolTypeGlobal: */
template<> QString toInternalString(const ToolTypeGlobal &enmToolTypeGlobal)
{
    QString strResult;
    switch (enmToolTypeGlobal)
    {
        case ToolTypeGlobal_VirtualMedia: strResult = "VirtualMedia"; break;
        case ToolTypeGlobal_HostNetwork:  strResult = "HostNetwork"; break;
        default:
        {
            AssertMsgFailed(("No text for global tool type=%d", enmToolTypeGlobal));
            break;
        }
    }
    return strResult;
}

/* ToolTypeGlobal <= QString: */
template<> ToolTypeGlobal fromInternalString<ToolTypeGlobal>(const QString &strToolTypeGlobal)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;       QList<ToolTypeGlobal> values;
    keys << "VirtualMedia"; values << ToolTypeGlobal_VirtualMedia;
    keys << "HostNetwork";  values << ToolTypeGlobal_HostNetwork;
    /* Invalid type for unknown words: */
    if (!keys.contains(strToolTypeGlobal, Qt::CaseInsensitive))
        return ToolTypeGlobal_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strToolTypeGlobal, Qt::CaseInsensitive)));
}

/* QString <= UIVisualStateType: */
template<> QString toInternalString(const UIVisualStateType &visualStateType)
{
    QString strResult;
    switch (visualStateType)
    {
        case UIVisualStateType_Normal:     strResult = "Normal"; break;
        case UIVisualStateType_Fullscreen: strResult = "Fullscreen"; break;
        case UIVisualStateType_Seamless:   strResult = "Seamless"; break;
        case UIVisualStateType_Scale:      strResult = "Scale"; break;
        case UIVisualStateType_All:        strResult = "All"; break;
        default:
        {
            AssertMsgFailed(("No text for visual state type=%d", visualStateType));
            break;
        }
    }
    return strResult;
}

/* UIVisualStateType <= QString: */
template<> UIVisualStateType fromInternalString<UIVisualStateType>(const QString &strVisualStateType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;     QList<UIVisualStateType> values;
    keys << "Normal";     values << UIVisualStateType_Normal;
    keys << "Fullscreen"; values << UIVisualStateType_Fullscreen;
    keys << "Seamless";   values << UIVisualStateType_Seamless;
    keys << "Scale";      values << UIVisualStateType_Scale;
    keys << "All";        values << UIVisualStateType_All;
    /* Invalid type for unknown words: */
    if (!keys.contains(strVisualStateType, Qt::CaseInsensitive))
        return UIVisualStateType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strVisualStateType, Qt::CaseInsensitive)));
}

/* QString <= DetailsElementType: */
template<> QString toString(const DetailsElementType &detailsElementType)
{
    QString strResult;
    switch (detailsElementType)
    {
        case DetailsElementType_General:     strResult = QApplication::translate("VBoxGlobal", "General", "DetailsElementType"); break;
        case DetailsElementType_Preview:     strResult = QApplication::translate("VBoxGlobal", "Preview", "DetailsElementType"); break;
        case DetailsElementType_System:      strResult = QApplication::translate("VBoxGlobal", "System", "DetailsElementType"); break;
        case DetailsElementType_Display:     strResult = QApplication::translate("VBoxGlobal", "Display", "DetailsElementType"); break;
        case DetailsElementType_Storage:     strResult = QApplication::translate("VBoxGlobal", "Storage", "DetailsElementType"); break;
        case DetailsElementType_Audio:       strResult = QApplication::translate("VBoxGlobal", "Audio", "DetailsElementType"); break;
        case DetailsElementType_Network:     strResult = QApplication::translate("VBoxGlobal", "Network", "DetailsElementType"); break;
        case DetailsElementType_Serial:      strResult = QApplication::translate("VBoxGlobal", "Serial ports", "DetailsElementType"); break;
        case DetailsElementType_USB:         strResult = QApplication::translate("VBoxGlobal", "USB", "DetailsElementType"); break;
        case DetailsElementType_SF:          strResult = QApplication::translate("VBoxGlobal", "Shared folders", "DetailsElementType"); break;
        case DetailsElementType_UI:          strResult = QApplication::translate("VBoxGlobal", "User interface", "DetailsElementType"); break;
        case DetailsElementType_Description: strResult = QApplication::translate("VBoxGlobal", "Description", "DetailsElementType"); break;
        default:
        {
            AssertMsgFailed(("No text for details element type=%d", detailsElementType));
            break;
        }
    }
    return strResult;
}

/* DetailsElementType <= QString: */
template<> DetailsElementType fromString<DetailsElementType>(const QString &strDetailsElementType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;                                                                      QList<DetailsElementType> values;
    keys << QApplication::translate("VBoxGlobal", "General", "DetailsElementType");        values << DetailsElementType_General;
    keys << QApplication::translate("VBoxGlobal", "Preview", "DetailsElementType");        values << DetailsElementType_Preview;
    keys << QApplication::translate("VBoxGlobal", "System", "DetailsElementType");         values << DetailsElementType_System;
    keys << QApplication::translate("VBoxGlobal", "Display", "DetailsElementType");        values << DetailsElementType_Display;
    keys << QApplication::translate("VBoxGlobal", "Storage", "DetailsElementType");        values << DetailsElementType_Storage;
    keys << QApplication::translate("VBoxGlobal", "Audio", "DetailsElementType");          values << DetailsElementType_Audio;
    keys << QApplication::translate("VBoxGlobal", "Network", "DetailsElementType");        values << DetailsElementType_Network;
    keys << QApplication::translate("VBoxGlobal", "Serial ports", "DetailsElementType");   values << DetailsElementType_Serial;
    keys << QApplication::translate("VBoxGlobal", "USB", "DetailsElementType");            values << DetailsElementType_USB;
    keys << QApplication::translate("VBoxGlobal", "Shared folders", "DetailsElementType"); values << DetailsElementType_SF;
    keys << QApplication::translate("VBoxGlobal", "User interface", "DetailsElementType"); values << DetailsElementType_UI;
    keys << QApplication::translate("VBoxGlobal", "Description", "DetailsElementType");    values << DetailsElementType_Description;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementType, Qt::CaseInsensitive))
        return DetailsElementType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementType, Qt::CaseInsensitive)));
}

/* QString <= DetailsElementType: */
template<> QString toInternalString(const DetailsElementType &detailsElementType)
{
    QString strResult;
    switch (detailsElementType)
    {
        case DetailsElementType_General:     strResult = "general"; break;
        case DetailsElementType_Preview:     strResult = "preview"; break;
        case DetailsElementType_System:      strResult = "system"; break;
        case DetailsElementType_Display:     strResult = "display"; break;
        case DetailsElementType_Storage:     strResult = "storage"; break;
        case DetailsElementType_Audio:       strResult = "audio"; break;
        case DetailsElementType_Network:     strResult = "network"; break;
        case DetailsElementType_Serial:      strResult = "serialPorts"; break;
        case DetailsElementType_USB:         strResult = "usb"; break;
        case DetailsElementType_SF:          strResult = "sharedFolders"; break;
        case DetailsElementType_UI:          strResult = "userInterface"; break;
        case DetailsElementType_Description: strResult = "description"; break;
        default:
        {
            AssertMsgFailed(("No text for details element type=%d", detailsElementType));
            break;
        }
    }
    return strResult;
}

/* DetailsElementType <= QString: */
template<> DetailsElementType fromInternalString<DetailsElementType>(const QString &strDetailsElementType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;        QList<DetailsElementType> values;
    keys << "general";       values << DetailsElementType_General;
    keys << "preview";       values << DetailsElementType_Preview;
    keys << "system";        values << DetailsElementType_System;
    keys << "display";       values << DetailsElementType_Display;
    keys << "storage";       values << DetailsElementType_Storage;
    keys << "audio";         values << DetailsElementType_Audio;
    keys << "network";       values << DetailsElementType_Network;
    keys << "serialPorts";   values << DetailsElementType_Serial;
    keys << "usb";           values << DetailsElementType_USB;
    keys << "sharedFolders"; values << DetailsElementType_SF;
    keys << "userInterface"; values << DetailsElementType_UI;
    keys << "description";   values << DetailsElementType_Description;
    /* Invalid type for unknown words: */
    if (!keys.contains(strDetailsElementType, Qt::CaseInsensitive))
        return DetailsElementType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strDetailsElementType, Qt::CaseInsensitive)));
}

/* QIcon <= DetailsElementType: */
template<> QIcon toIcon(const DetailsElementType &detailsElementType)
{
    switch (detailsElementType)
    {
        case DetailsElementType_General:     return UIIconPool::iconSet(":/machine_16px.png");
        case DetailsElementType_Preview:     return UIIconPool::iconSet(":/machine_16px.png");
        case DetailsElementType_System:      return UIIconPool::iconSet(":/chipset_16px.png");
        case DetailsElementType_Display:     return UIIconPool::iconSet(":/vrdp_16px.png");
        case DetailsElementType_Storage:     return UIIconPool::iconSet(":/hd_16px.png");
        case DetailsElementType_Audio:       return UIIconPool::iconSet(":/sound_16px.png");
        case DetailsElementType_Network:     return UIIconPool::iconSet(":/nw_16px.png");
        case DetailsElementType_Serial:      return UIIconPool::iconSet(":/serial_port_16px.png");
        case DetailsElementType_USB:         return UIIconPool::iconSet(":/usb_16px.png");
        case DetailsElementType_SF:          return UIIconPool::iconSet(":/sf_16px.png");
        case DetailsElementType_UI:          return UIIconPool::iconSet(":/interface_16px.png");
        case DetailsElementType_Description: return UIIconPool::iconSet(":/description_16px.png");
        default:
        {
            AssertMsgFailed(("No icon for details element type=%d", detailsElementType));
            break;
        }
    }
    return QIcon();
}

/* QString <= PreviewUpdateIntervalType: */
template<> QString toInternalString(const PreviewUpdateIntervalType &previewUpdateIntervalType)
{
    /* Return corresponding QString representation for passed enum value: */
    switch (previewUpdateIntervalType)
    {
        case PreviewUpdateIntervalType_Disabled: return "disabled";
        case PreviewUpdateIntervalType_500ms:    return "500";
        case PreviewUpdateIntervalType_1000ms:   return "1000";
        case PreviewUpdateIntervalType_2000ms:   return "2000";
        case PreviewUpdateIntervalType_5000ms:   return "5000";
        case PreviewUpdateIntervalType_10000ms:  return "10000";
        default: AssertMsgFailed(("No text for '%d'", previewUpdateIntervalType)); break;
    }
    /* Return QString() by default: */
    return QString();
}

/* PreviewUpdateIntervalType <= QString: */
template<> PreviewUpdateIntervalType fromInternalString<PreviewUpdateIntervalType>(const QString &strPreviewUpdateIntervalType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;   QList<PreviewUpdateIntervalType> values;
    keys << "disabled"; values << PreviewUpdateIntervalType_Disabled;
    keys << "500";      values << PreviewUpdateIntervalType_500ms;
    keys << "1000";     values << PreviewUpdateIntervalType_1000ms;
    keys << "2000";     values << PreviewUpdateIntervalType_2000ms;
    keys << "5000";     values << PreviewUpdateIntervalType_5000ms;
    keys << "10000";    values << PreviewUpdateIntervalType_10000ms;
    /* 1000ms type for unknown words: */
    if (!keys.contains(strPreviewUpdateIntervalType, Qt::CaseInsensitive))
        return PreviewUpdateIntervalType_1000ms;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strPreviewUpdateIntervalType, Qt::CaseInsensitive)));
}

/* int <= PreviewUpdateIntervalType: */
template<> int toInternalInteger(const PreviewUpdateIntervalType &previewUpdateIntervalType)
{
    /* Return corresponding integer representation for passed enum value: */
    switch (previewUpdateIntervalType)
    {
        case PreviewUpdateIntervalType_Disabled: return 0;
        case PreviewUpdateIntervalType_500ms:    return 500;
        case PreviewUpdateIntervalType_1000ms:   return 1000;
        case PreviewUpdateIntervalType_2000ms:   return 2000;
        case PreviewUpdateIntervalType_5000ms:   return 5000;
        case PreviewUpdateIntervalType_10000ms:  return 10000;
        default: AssertMsgFailed(("No value for '%d'", previewUpdateIntervalType)); break;
    }
    /* Return 0 by default: */
    return 0;
}

/* PreviewUpdateIntervalType <= int: */
template<> PreviewUpdateIntervalType fromInternalInteger<PreviewUpdateIntervalType>(const int &iPreviewUpdateIntervalType)
{
    /* Add all the enum values into the hash: */
    QHash<int, PreviewUpdateIntervalType> hash;
    hash.insert(0,     PreviewUpdateIntervalType_Disabled);
    hash.insert(500,   PreviewUpdateIntervalType_500ms);
    hash.insert(1000,  PreviewUpdateIntervalType_1000ms);
    hash.insert(2000,  PreviewUpdateIntervalType_2000ms);
    hash.insert(5000,  PreviewUpdateIntervalType_5000ms);
    hash.insert(10000, PreviewUpdateIntervalType_10000ms);
    /* Make sure hash contains incoming integer representation: */
    if (!hash.contains(iPreviewUpdateIntervalType))
        AssertMsgFailed(("No value for '%d'", iPreviewUpdateIntervalType));
    /* Return corresponding enum value for passed integer representation: */
    return hash.value(iPreviewUpdateIntervalType);
}

/* EventHandlingType <= QString: */
template<> EventHandlingType fromInternalString<EventHandlingType>(const QString &strEventHandlingType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;  QList<EventHandlingType> values;
    keys << "Active";  values << EventHandlingType_Active;
    keys << "Passive"; values << EventHandlingType_Passive;
    /* Passive type for unknown words: */
    if (!keys.contains(strEventHandlingType, Qt::CaseInsensitive))
        return EventHandlingType_Passive;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strEventHandlingType, Qt::CaseInsensitive)));
}

/* QString <= GUIFeatureType: */
template<> QString toInternalString(const GUIFeatureType &guiFeatureType)
{
    QString strResult;
    switch (guiFeatureType)
    {
        case GUIFeatureType_NoSelector:  strResult = "noSelector"; break;
        case GUIFeatureType_NoMenuBar:   strResult = "noMenuBar"; break;
        case GUIFeatureType_NoStatusBar: strResult = "noStatusBar"; break;
        default:
        {
            AssertMsgFailed(("No text for GUI feature type=%d", guiFeatureType));
            break;
        }
    }
    return strResult;
}

/* GUIFeatureType <= QString: */
template<> GUIFeatureType fromInternalString<GUIFeatureType>(const QString &strGuiFeatureType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;      QList<GUIFeatureType> values;
    keys << "noSelector";  values << GUIFeatureType_NoSelector;
    keys << "noMenuBar";   values << GUIFeatureType_NoMenuBar;
    keys << "noStatusBar"; values << GUIFeatureType_NoStatusBar;
    /* None type for unknown words: */
    if (!keys.contains(strGuiFeatureType, Qt::CaseInsensitive))
        return GUIFeatureType_None;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strGuiFeatureType, Qt::CaseInsensitive)));
}

/* QString <= GlobalSettingsPageType: */
template<> QString toInternalString(const GlobalSettingsPageType &globalSettingsPageType)
{
    QString strResult;
    switch (globalSettingsPageType)
    {
        case GlobalSettingsPageType_General:    strResult = "General"; break;
        case GlobalSettingsPageType_Input:      strResult = "Input"; break;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
        case GlobalSettingsPageType_Update:     strResult = "Update"; break;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
        case GlobalSettingsPageType_Language:   strResult = "Language"; break;
        case GlobalSettingsPageType_Display:    strResult = "Display"; break;
        case GlobalSettingsPageType_Network:    strResult = "Network"; break;
        case GlobalSettingsPageType_Extensions: strResult = "Extensions"; break;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
        case GlobalSettingsPageType_Proxy:      strResult = "Proxy"; break;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
        default:
        {
            AssertMsgFailed(("No text for settings page type=%d", globalSettingsPageType));
            break;
        }
    }
    return strResult;
}

/* GlobalSettingsPageType <= QString: */
template<> GlobalSettingsPageType fromInternalString<GlobalSettingsPageType>(const QString &strGlobalSettingsPageType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;     QList<GlobalSettingsPageType> values;
    keys << "General";    values << GlobalSettingsPageType_General;
    keys << "Input";      values << GlobalSettingsPageType_Input;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    keys << "Update";     values << GlobalSettingsPageType_Update;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
    keys << "Language";   values << GlobalSettingsPageType_Language;
    keys << "Display";    values << GlobalSettingsPageType_Display;
    keys << "Network";    values << GlobalSettingsPageType_Network;
    keys << "Extensions"; values << GlobalSettingsPageType_Extensions;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    keys << "Proxy";      values << GlobalSettingsPageType_Proxy;
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
    /* Invalid type for unknown words: */
    if (!keys.contains(strGlobalSettingsPageType, Qt::CaseInsensitive))
        return GlobalSettingsPageType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strGlobalSettingsPageType, Qt::CaseInsensitive)));
}

/* QPixmap <= GlobalSettingsPageType: */
template<> QPixmap toWarningPixmap(const GlobalSettingsPageType &type)
{
    switch (type)
    {
        case GlobalSettingsPageType_General:    return UIIconPool::pixmap(":/machine_warning_16px.png");
        case GlobalSettingsPageType_Input:      return UIIconPool::pixmap(":/hostkey_warning_16px.png");
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
        case GlobalSettingsPageType_Update:     return UIIconPool::pixmap(":/refresh_warning_16px.png");
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
        case GlobalSettingsPageType_Language:   return UIIconPool::pixmap(":/site_warning_16px.png");
        case GlobalSettingsPageType_Display:    return UIIconPool::pixmap(":/vrdp_warning_16px.png");
        case GlobalSettingsPageType_Network:    return UIIconPool::pixmap(":/nw_warning_16px.png");
        case GlobalSettingsPageType_Extensions: return UIIconPool::pixmap(":/extension_pack_warning_16px.png");
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
        case GlobalSettingsPageType_Proxy:      return UIIconPool::pixmap(":/proxy_warning_16px.png");
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
        default: AssertMsgFailed(("No pixmap for %d", type)); break;
    }
    return QPixmap();
}

/* QString <= MachineSettingsPageType: */
template<> QString toInternalString(const MachineSettingsPageType &machineSettingsPageType)
{
    QString strResult;
    switch (machineSettingsPageType)
    {
        case MachineSettingsPageType_General:   strResult = "General"; break;
        case MachineSettingsPageType_System:    strResult = "System"; break;
        case MachineSettingsPageType_Display:   strResult = "Display"; break;
        case MachineSettingsPageType_Storage:   strResult = "Storage"; break;
        case MachineSettingsPageType_Audio:     strResult = "Audio"; break;
        case MachineSettingsPageType_Network:   strResult = "Network"; break;
        case MachineSettingsPageType_Ports:     strResult = "Ports"; break;
        case MachineSettingsPageType_Serial:    strResult = "Serial"; break;
        case MachineSettingsPageType_USB:       strResult = "USB"; break;
        case MachineSettingsPageType_SF:        strResult = "SharedFolders"; break;
        case MachineSettingsPageType_Interface: strResult = "Interface"; break;
        default:
        {
            AssertMsgFailed(("No text for settings page type=%d", machineSettingsPageType));
            break;
        }
    }
    return strResult;
}

/* MachineSettingsPageType <= QString: */
template<> MachineSettingsPageType fromInternalString<MachineSettingsPageType>(const QString &strMachineSettingsPageType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;        QList<MachineSettingsPageType> values;
    keys << "General";       values << MachineSettingsPageType_General;
    keys << "System";        values << MachineSettingsPageType_System;
    keys << "Display";       values << MachineSettingsPageType_Display;
    keys << "Storage";       values << MachineSettingsPageType_Storage;
    keys << "Audio";         values << MachineSettingsPageType_Audio;
    keys << "Network";       values << MachineSettingsPageType_Network;
    keys << "Ports";         values << MachineSettingsPageType_Ports;
    keys << "Serial";        values << MachineSettingsPageType_Serial;
    keys << "USB";           values << MachineSettingsPageType_USB;
    keys << "SharedFolders"; values << MachineSettingsPageType_SF;
    keys << "Interface";     values << MachineSettingsPageType_Interface;
    /* Invalid type for unknown words: */
    if (!keys.contains(strMachineSettingsPageType, Qt::CaseInsensitive))
        return MachineSettingsPageType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMachineSettingsPageType, Qt::CaseInsensitive)));
}

/* QPixmap <= MachineSettingsPageType: */
template<> QPixmap toWarningPixmap(const MachineSettingsPageType &type)
{
    switch (type)
    {
        case MachineSettingsPageType_General:   return UIIconPool::pixmap(":/machine_warning_16px.png");
        case MachineSettingsPageType_System:    return UIIconPool::pixmap(":/chipset_warning_16px.png");
        case MachineSettingsPageType_Display:   return UIIconPool::pixmap(":/vrdp_warning_16px.png");
        case MachineSettingsPageType_Storage:   return UIIconPool::pixmap(":/hd_warning_16px.png");
        case MachineSettingsPageType_Audio:     return UIIconPool::pixmap(":/sound_warning_16px.png");
        case MachineSettingsPageType_Network:   return UIIconPool::pixmap(":/nw_warning_16px.png");
        case MachineSettingsPageType_Ports:     return UIIconPool::pixmap(":/serial_port_warning_16px.png");
        case MachineSettingsPageType_Serial:    return UIIconPool::pixmap(":/serial_port_warning_16px.png");
        case MachineSettingsPageType_USB:       return UIIconPool::pixmap(":/usb_warning_16px.png");
        case MachineSettingsPageType_SF:        return UIIconPool::pixmap(":/sf_warning_16px.png");
        case MachineSettingsPageType_Interface: return UIIconPool::pixmap(":/interface_warning_16px.png");
        default: AssertMsgFailed(("No pixmap for %d", type)); break;
    }
    return QPixmap();
}

/* QString <= WizardType: */
template<> QString toInternalString(const WizardType &wizardType)
{
    QString strResult;
    switch (wizardType)
    {
        case WizardType_NewVM:           strResult = "NewVM"; break;
        case WizardType_CloneVM:         strResult = "CloneVM"; break;
        case WizardType_ExportAppliance: strResult = "ExportAppliance"; break;
        case WizardType_ImportAppliance: strResult = "ImportAppliance"; break;
        case WizardType_FirstRun:        strResult = "FirstRun"; break;
        case WizardType_NewVD:           strResult = "NewVD"; break;
        case WizardType_CloneVD:         strResult = "CloneVD"; break;
        default:
        {
            AssertMsgFailed(("No text for wizard type=%d", wizardType));
            break;
        }
    }
    return strResult;
}

/* WizardType <= QString: */
template<> WizardType fromInternalString<WizardType>(const QString &strWizardType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;          QList<WizardType> values;
    keys << "NewVM";           values << WizardType_NewVM;
    keys << "CloneVM";         values << WizardType_CloneVM;
    keys << "ExportAppliance"; values << WizardType_ExportAppliance;
    keys << "ImportAppliance"; values << WizardType_ImportAppliance;
    keys << "FirstRun";        values << WizardType_FirstRun;
    keys << "NewVD";           values << WizardType_NewVD;
    keys << "CloneVD";         values << WizardType_CloneVD;
    /* Invalid type for unknown words: */
    if (!keys.contains(strWizardType, Qt::CaseInsensitive))
        return WizardType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strWizardType, Qt::CaseInsensitive)));
}

/* QString <= IndicatorType: */
template<> QString toInternalString(const IndicatorType &indicatorType)
{
    QString strResult;
    switch (indicatorType)
    {
        case IndicatorType_HardDisks:     strResult = "HardDisks"; break;
        case IndicatorType_OpticalDisks:  strResult = "OpticalDisks"; break;
        case IndicatorType_FloppyDisks:   strResult = "FloppyDisks"; break;
        case IndicatorType_Audio:         strResult = "Audio"; break;
        case IndicatorType_Network:       strResult = "Network"; break;
        case IndicatorType_USB:           strResult = "USB"; break;
        case IndicatorType_SharedFolders: strResult = "SharedFolders"; break;
        case IndicatorType_Display:       strResult = "Display"; break;
        case IndicatorType_VideoCapture:  strResult = "VideoCapture"; break;
        case IndicatorType_Features:      strResult = "Features"; break;
        case IndicatorType_Mouse:         strResult = "Mouse"; break;
        case IndicatorType_Keyboard:      strResult = "Keyboard"; break;
        default:
        {
            AssertMsgFailed(("No text for indicator type=%d", indicatorType));
            break;
        }
    }
    return strResult;
}

/* IndicatorType <= QString: */
template<> IndicatorType fromInternalString<IndicatorType>(const QString &strIndicatorType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;        QList<IndicatorType> values;
    keys << "HardDisks";     values << IndicatorType_HardDisks;
    keys << "OpticalDisks";  values << IndicatorType_OpticalDisks;
    keys << "FloppyDisks";   values << IndicatorType_FloppyDisks;
    keys << "Audio";         values << IndicatorType_Audio;
    keys << "Network";       values << IndicatorType_Network;
    keys << "USB";           values << IndicatorType_USB;
    keys << "SharedFolders"; values << IndicatorType_SharedFolders;
    keys << "Display";       values << IndicatorType_Display;
    keys << "VideoCapture";  values << IndicatorType_VideoCapture;
    keys << "Features";      values << IndicatorType_Features;
    keys << "Mouse";         values << IndicatorType_Mouse;
    keys << "Keyboard";      values << IndicatorType_Keyboard;
    /* Invalid type for unknown words: */
    if (!keys.contains(strIndicatorType, Qt::CaseInsensitive))
        return IndicatorType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strIndicatorType, Qt::CaseInsensitive)));
}

/* QString <= IndicatorType: */
template<> QString toString(const IndicatorType &indicatorType)
{
    QString strResult;
    switch (indicatorType)
    {
        case IndicatorType_HardDisks:     strResult = QApplication::translate("VBoxGlobal", "Hard Disks", "IndicatorType"); break;
        case IndicatorType_OpticalDisks:  strResult = QApplication::translate("VBoxGlobal", "Optical Disks", "IndicatorType"); break;
        case IndicatorType_FloppyDisks:   strResult = QApplication::translate("VBoxGlobal", "Floppy Disks", "IndicatorType"); break;
        case IndicatorType_Audio:         strResult = QApplication::translate("VBoxGlobal", "Audio", "IndicatorType"); break;
        case IndicatorType_Network:       strResult = QApplication::translate("VBoxGlobal", "Network", "IndicatorType"); break;
        case IndicatorType_USB:           strResult = QApplication::translate("VBoxGlobal", "USB", "IndicatorType"); break;
        case IndicatorType_SharedFolders: strResult = QApplication::translate("VBoxGlobal", "Shared Folders", "IndicatorType"); break;
        case IndicatorType_Display:       strResult = QApplication::translate("VBoxGlobal", "Display", "IndicatorType"); break;
        case IndicatorType_VideoCapture:  strResult = QApplication::translate("VBoxGlobal", "Video Capture", "IndicatorType"); break;
        case IndicatorType_Features:      strResult = QApplication::translate("VBoxGlobal", "Features", "IndicatorType"); break;
        case IndicatorType_Mouse:         strResult = QApplication::translate("VBoxGlobal", "Mouse", "IndicatorType"); break;
        case IndicatorType_Keyboard:      strResult = QApplication::translate("VBoxGlobal", "Keyboard", "IndicatorType"); break;
        default:
        {
            AssertMsgFailed(("No text for indicator type=%d", indicatorType));
            break;
        }
    }
    return strResult;
}

/* QIcon <= IndicatorType: */
template<> QIcon toIcon(const IndicatorType &indicatorType)
{
    switch (indicatorType)
    {
        case IndicatorType_HardDisks:     return UIIconPool::iconSet(":/hd_16px.png");
        case IndicatorType_OpticalDisks:  return UIIconPool::iconSet(":/cd_16px.png");
        case IndicatorType_FloppyDisks:   return UIIconPool::iconSet(":/fd_16px.png");
        case IndicatorType_Audio:         return UIIconPool::iconSet(":/audio_16px.png");
        case IndicatorType_Network:       return UIIconPool::iconSet(":/nw_16px.png");
        case IndicatorType_USB:           return UIIconPool::iconSet(":/usb_16px.png");
        case IndicatorType_SharedFolders: return UIIconPool::iconSet(":/sf_16px.png");
        case IndicatorType_Display:       return UIIconPool::iconSet(":/display_software_16px.png");
        case IndicatorType_VideoCapture:  return UIIconPool::iconSet(":/video_capture_16px.png");
        case IndicatorType_Features:      return UIIconPool::iconSet(":/vtx_amdv_16px.png");
        case IndicatorType_Mouse:         return UIIconPool::iconSet(":/mouse_16px.png");
        case IndicatorType_Keyboard:      return UIIconPool::iconSet(":/hostkey_16px.png");
        default:
        {
            AssertMsgFailed(("No icon for indicator type=%d", indicatorType));
            break;
        }
    }
    return QIcon();
}

/* QString <= MachineCloseAction: */
template<> QString toInternalString(const MachineCloseAction &machineCloseAction)
{
    QString strResult;
    switch (machineCloseAction)
    {
        case MachineCloseAction_Detach:                     strResult = "Detach"; break;
        case MachineCloseAction_SaveState:                  strResult = "SaveState"; break;
        case MachineCloseAction_Shutdown:                   strResult = "Shutdown"; break;
        case MachineCloseAction_PowerOff:                   strResult = "PowerOff"; break;
        case MachineCloseAction_PowerOff_RestoringSnapshot: strResult = "PowerOffRestoringSnapshot"; break;
        default:
        {
            AssertMsgFailed(("No text for indicator type=%d", machineCloseAction));
            break;
        }
    }
    return strResult;
}

/* MachineCloseAction <= QString: */
template<> MachineCloseAction fromInternalString<MachineCloseAction>(const QString &strMachineCloseAction)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;                    QList<MachineCloseAction> values;
    keys << "Detach";                    values << MachineCloseAction_Detach;
    keys << "SaveState";                 values << MachineCloseAction_SaveState;
    keys << "Shutdown";                  values << MachineCloseAction_Shutdown;
    keys << "PowerOff";                  values << MachineCloseAction_PowerOff;
    keys << "PowerOffRestoringSnapshot"; values << MachineCloseAction_PowerOff_RestoringSnapshot;
    /* Invalid type for unknown words: */
    if (!keys.contains(strMachineCloseAction, Qt::CaseInsensitive))
        return MachineCloseAction_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMachineCloseAction, Qt::CaseInsensitive)));
}

/* QString <= MouseCapturePolicy: */
template<> QString toInternalString(const MouseCapturePolicy &mouseCapturePolicy)
{
    /* Return corresponding QString representation for passed enum value: */
    switch (mouseCapturePolicy)
    {
        case MouseCapturePolicy_Default:       return "Default";
        case MouseCapturePolicy_HostComboOnly: return "HostComboOnly";
        case MouseCapturePolicy_Disabled:      return "Disabled";
        default: AssertMsgFailed(("No text for '%d'", mouseCapturePolicy)); break;
    }
    /* Return QString() by default: */
    return QString();
}

/* MouseCapturePolicy <= QString: */
template<> MouseCapturePolicy fromInternalString<MouseCapturePolicy>(const QString &strMouseCapturePolicy)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;        QList<MouseCapturePolicy> values;
    keys << "Default";       values << MouseCapturePolicy_Default;
    keys << "HostComboOnly"; values << MouseCapturePolicy_HostComboOnly;
    keys << "Disabled";      values << MouseCapturePolicy_Disabled;
    /* Default type for unknown words: */
    if (!keys.contains(strMouseCapturePolicy, Qt::CaseInsensitive))
        return MouseCapturePolicy_Default;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMouseCapturePolicy, Qt::CaseInsensitive)));
}

/* QString <= GuruMeditationHandlerType: */
template<> QString toInternalString(const GuruMeditationHandlerType &guruMeditationHandlerType)
{
    QString strResult;
    switch (guruMeditationHandlerType)
    {
        case GuruMeditationHandlerType_Default:  strResult = "Default"; break;
        case GuruMeditationHandlerType_PowerOff: strResult = "PowerOff"; break;
        case GuruMeditationHandlerType_Ignore:   strResult = "Ignore"; break;
        default:
        {
            AssertMsgFailed(("No text for indicator type=%d", guruMeditationHandlerType));
            break;
        }
    }
    return strResult;
}

/* GuruMeditationHandlerType <= QString: */
template<> GuruMeditationHandlerType fromInternalString<GuruMeditationHandlerType>(const QString &strGuruMeditationHandlerType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;   QList<GuruMeditationHandlerType> values;
    keys << "Default";  values << GuruMeditationHandlerType_Default;
    keys << "PowerOff"; values << GuruMeditationHandlerType_PowerOff;
    keys << "Ignore";   values << GuruMeditationHandlerType_Ignore;
    /* Default type for unknown words: */
    if (!keys.contains(strGuruMeditationHandlerType, Qt::CaseInsensitive))
        return GuruMeditationHandlerType_Default;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strGuruMeditationHandlerType, Qt::CaseInsensitive)));
}

/* QString <= ScalingOptimizationType: */
template<> QString toInternalString(const ScalingOptimizationType &optimizationType)
{
    QString strResult;
    switch (optimizationType)
    {
        case ScalingOptimizationType_None:        strResult = "None"; break;
        case ScalingOptimizationType_Performance: strResult = "Performance"; break;
        default:
        {
            AssertMsgFailed(("No text for type=%d", optimizationType));
            break;
        }
    }
    return strResult;
}

/* ScalingOptimizationType <= QString: */
template<> ScalingOptimizationType fromInternalString<ScalingOptimizationType>(const QString &strOptimizationType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;      QList<ScalingOptimizationType> values;
    keys << "None";        values << ScalingOptimizationType_None;
    keys << "Performance"; values << ScalingOptimizationType_Performance;
    /* 'None' type for empty/unknown words: */
    if (!keys.contains(strOptimizationType, Qt::CaseInsensitive))
        return ScalingOptimizationType_None;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strOptimizationType, Qt::CaseInsensitive)));
}

/* QString <= HiDPIOptimizationType: */
template<> QString toInternalString(const HiDPIOptimizationType &optimizationType)
{
    QString strResult;
    switch (optimizationType)
    {
        case HiDPIOptimizationType_None:        strResult = "None"; break;
        case HiDPIOptimizationType_Performance: strResult = "Performance"; break;
        default:
        {
            AssertMsgFailed(("No text for type=%d", optimizationType));
            break;
        }
    }
    return strResult;
}

/* HiDPIOptimizationType <= QString: */
template<> HiDPIOptimizationType fromInternalString<HiDPIOptimizationType>(const QString &strOptimizationType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;      QList<HiDPIOptimizationType> values;
    keys << "None";        values << HiDPIOptimizationType_None;
    keys << "Performance"; values << HiDPIOptimizationType_Performance;
    /* 'Performance' type for empty/unknown words (for trunk): */
    if (!keys.contains(strOptimizationType, Qt::CaseInsensitive))
        return HiDPIOptimizationType_Performance;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strOptimizationType, Qt::CaseInsensitive)));
}

#ifndef VBOX_WS_MAC
/* QString <= MiniToolbarAlignment: */
template<> QString toInternalString(const MiniToolbarAlignment &miniToolbarAlignment)
{
    /* Return corresponding QString representation for passed enum value: */
    switch (miniToolbarAlignment)
    {
        case MiniToolbarAlignment_Bottom: return "Bottom";
        case MiniToolbarAlignment_Top:    return "Top";
        default: AssertMsgFailed(("No text for '%d'", miniToolbarAlignment)); break;
    }
    /* Return QString() by default: */
    return QString();
}

/* MiniToolbarAlignment <= QString: */
template<> MiniToolbarAlignment fromInternalString<MiniToolbarAlignment>(const QString &strMiniToolbarAlignment)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys; QList<MiniToolbarAlignment> values;
    keys << "Bottom"; values << MiniToolbarAlignment_Bottom;
    keys << "Top";    values << MiniToolbarAlignment_Top;
    /* Bottom type for unknown words: */
    if (!keys.contains(strMiniToolbarAlignment, Qt::CaseInsensitive))
        return MiniToolbarAlignment_Bottom;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMiniToolbarAlignment, Qt::CaseInsensitive)));
}
#endif /* !VBOX_WS_MAC */

/* QString <= InformationElementType: */
template<> QString toString(const InformationElementType &informationElementType)
{
    QString strResult;
    switch (informationElementType)
    {
        case InformationElementType_General:           strResult = QApplication::translate("VBoxGlobal", "General", "InformationElementType"); break;
        case InformationElementType_Preview:           strResult = QApplication::translate("VBoxGlobal", "Preview", "InformationElementType"); break;
        case InformationElementType_System:            strResult = QApplication::translate("VBoxGlobal", "System", "InformationElementType"); break;
        case InformationElementType_Display:           strResult = QApplication::translate("VBoxGlobal", "Display", "InformationElementType"); break;
        case InformationElementType_Storage:           strResult = QApplication::translate("VBoxGlobal", "Storage", "InformationElementType"); break;
        case InformationElementType_Audio:             strResult = QApplication::translate("VBoxGlobal", "Audio", "InformationElementType"); break;
        case InformationElementType_Network:           strResult = QApplication::translate("VBoxGlobal", "Network", "InformationElementType"); break;
        case InformationElementType_Serial:            strResult = QApplication::translate("VBoxGlobal", "Serial ports", "InformationElementType"); break;
        case InformationElementType_USB:               strResult = QApplication::translate("VBoxGlobal", "USB", "InformationElementType"); break;
        case InformationElementType_SharedFolders:     strResult = QApplication::translate("VBoxGlobal", "Shared folders", "InformationElementType"); break;
        case InformationElementType_UI:                strResult = QApplication::translate("VBoxGlobal", "User interface", "InformationElementType"); break;
        case InformationElementType_Description:       strResult = QApplication::translate("VBoxGlobal", "Description", "InformationElementType"); break;
        case InformationElementType_RuntimeAttributes: strResult = QApplication::translate("VBoxGlobal", "Runtime attributes", "InformationElementType"); break;
        case InformationElementType_StorageStatistics: strResult = QApplication::translate("VBoxGlobal", "Storage statistics", "InformationElementType"); break;
        case InformationElementType_NetworkStatistics: strResult = QApplication::translate("VBoxGlobal", "Network statistics", "InformationElementType"); break;
        default:
        {
            AssertMsgFailed(("No text for information element type=%d", informationElementType));
            break;
        }
    }
    return strResult;
}

/* InformationElementType <= QString: */
template<> InformationElementType fromString<InformationElementType>(const QString &strInformationElementType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;                                                                              QList<InformationElementType> values;
    keys << QApplication::translate("VBoxGlobal", "General", "InformationElementType");            values << InformationElementType_General;
    keys << QApplication::translate("VBoxGlobal", "Preview", "InformationElementType");            values << InformationElementType_Preview;
    keys << QApplication::translate("VBoxGlobal", "System", "InformationElementType");             values << InformationElementType_System;
    keys << QApplication::translate("VBoxGlobal", "Display", "InformationElementType");            values << InformationElementType_Display;
    keys << QApplication::translate("VBoxGlobal", "Storage", "InformationElementType");            values << InformationElementType_Storage;
    keys << QApplication::translate("VBoxGlobal", "Audio", "InformationElementType");              values << InformationElementType_Audio;
    keys << QApplication::translate("VBoxGlobal", "Network", "InformationElementType");            values << InformationElementType_Network;
    keys << QApplication::translate("VBoxGlobal", "Serial ports", "InformationElementType");       values << InformationElementType_Serial;
    keys << QApplication::translate("VBoxGlobal", "USB", "InformationElementType");                values << InformationElementType_USB;
    keys << QApplication::translate("VBoxGlobal", "Shared folders", "InformationElementType");     values << InformationElementType_SharedFolders;
    keys << QApplication::translate("VBoxGlobal", "User interface", "InformationElementType");     values << InformationElementType_UI;
    keys << QApplication::translate("VBoxGlobal", "Description", "InformationElementType");        values << InformationElementType_Description;
    keys << QApplication::translate("VBoxGlobal", "Runtime attributes", "InformationElementType"); values << InformationElementType_RuntimeAttributes;
    keys << QApplication::translate("VBoxGlobal", "Storage statistics", "InformationElementType"); values << InformationElementType_StorageStatistics;
    keys << QApplication::translate("VBoxGlobal", "Network statistics", "InformationElementType"); values << InformationElementType_NetworkStatistics;
    /* Invalid type for unknown words: */
    if (!keys.contains(strInformationElementType, Qt::CaseInsensitive))
        return InformationElementType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strInformationElementType, Qt::CaseInsensitive)));
}

/* QString <= InformationElementType: */
template<> QString toInternalString(const InformationElementType &informationElementType)
{
    QString strResult;
    switch (informationElementType)
    {
        case InformationElementType_General:           strResult = "general"; break;
        case InformationElementType_Preview:           strResult = "preview"; break;
        case InformationElementType_System:            strResult = "system"; break;
        case InformationElementType_Display:           strResult = "display"; break;
        case InformationElementType_Storage:           strResult = "storage"; break;
        case InformationElementType_Audio:             strResult = "audio"; break;
        case InformationElementType_Network:           strResult = "network"; break;
        case InformationElementType_Serial:            strResult = "serialPorts"; break;
        case InformationElementType_USB:               strResult = "usb"; break;
        case InformationElementType_SharedFolders:     strResult = "sharedFolders"; break;
        case InformationElementType_UI:                strResult = "userInterface"; break;
        case InformationElementType_Description:       strResult = "description"; break;
        case InformationElementType_RuntimeAttributes: strResult = "runtime-attributes"; break;
        default:
        {
            AssertMsgFailed(("No text for information element type=%d", informationElementType));
            break;
        }
    }
    return strResult;
}

/* InformationElementType <= QString: */
template<> InformationElementType fromInternalString<InformationElementType>(const QString &strInformationElementType)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys;             QList<InformationElementType> values;
    keys << "general";            values << InformationElementType_General;
    keys << "preview";            values << InformationElementType_Preview;
    keys << "system";             values << InformationElementType_System;
    keys << "display";            values << InformationElementType_Display;
    keys << "storage";            values << InformationElementType_Storage;
    keys << "audio";              values << InformationElementType_Audio;
    keys << "network";            values << InformationElementType_Network;
    keys << "serialPorts";        values << InformationElementType_Serial;
    keys << "usb";                values << InformationElementType_USB;
    keys << "sharedFolders";      values << InformationElementType_SharedFolders;
    keys << "userInterface";      values << InformationElementType_UI;
    keys << "description";        values << InformationElementType_Description;
    keys << "runtime-attributes"; values << InformationElementType_RuntimeAttributes;
    /* Invalid type for unknown words: */
    if (!keys.contains(strInformationElementType, Qt::CaseInsensitive))
        return InformationElementType_Invalid;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strInformationElementType, Qt::CaseInsensitive)));
}

/* QIcon <= InformationElementType: */
template<> QIcon toIcon(const InformationElementType &informationElementType)
{
    switch (informationElementType)
    {
        case InformationElementType_General:           return UIIconPool::iconSet(":/machine_16px.png");
        case InformationElementType_Preview:           return UIIconPool::iconSet(":/machine_16px.png");
        case InformationElementType_System:            return UIIconPool::iconSet(":/chipset_16px.png");
        case InformationElementType_Display:           return UIIconPool::iconSet(":/vrdp_16px.png");
        case InformationElementType_Storage:           return UIIconPool::iconSet(":/hd_16px.png");
        case InformationElementType_Audio:             return UIIconPool::iconSet(":/sound_16px.png");
        case InformationElementType_Network:           return UIIconPool::iconSet(":/nw_16px.png");
        case InformationElementType_Serial:            return UIIconPool::iconSet(":/serial_port_16px.png");
        case InformationElementType_USB:               return UIIconPool::iconSet(":/usb_16px.png");
        case InformationElementType_SharedFolders:     return UIIconPool::iconSet(":/sf_16px.png");
        case InformationElementType_UI:                return UIIconPool::iconSet(":/interface_16px.png");
        case InformationElementType_Description:       return UIIconPool::iconSet(":/description_16px.png");
        case InformationElementType_RuntimeAttributes: return UIIconPool::iconSet(":/state_running_16px.png");
        case InformationElementType_StorageStatistics: return UIIconPool::iconSet(":/hd_16px.png");
        case InformationElementType_NetworkStatistics: return UIIconPool::iconSet(":/nw_16px.png");
        default:
        {
            AssertMsgFailed(("No icon for information element type=%d", informationElementType));
            break;
        }
    }
    return QIcon();
}

/* QString <= MaxGuestResolutionPolicy: */
template<> QString toInternalString(const MaxGuestResolutionPolicy &enmMaxGuestResolutionPolicy)
{
    QString strResult;
    switch (enmMaxGuestResolutionPolicy)
    {
        case MaxGuestResolutionPolicy_Automatic: strResult = ""; break;
        case MaxGuestResolutionPolicy_Any:       strResult = "any"; break;
        default:
        {
            AssertMsgFailed(("No text for max guest resolution policy=%d", enmMaxGuestResolutionPolicy));
            break;
        }
    }
    return strResult;
}

/* MaxGuestResolutionPolicy <= QString: */
template<> MaxGuestResolutionPolicy fromInternalString<MaxGuestResolutionPolicy>(const QString &strMaxGuestResolutionPolicy)
{
    /* Here we have some fancy stuff allowing us
     * to search through the keys using 'case-insensitive' rule: */
    QStringList keys; QList<MaxGuestResolutionPolicy> values;
    keys << "auto";   values << MaxGuestResolutionPolicy_Automatic;
    /* Auto type for empty value: */
    if (strMaxGuestResolutionPolicy.isEmpty())
        return MaxGuestResolutionPolicy_Automatic;
    /* Fixed type for value which can be parsed: */
    if (QRegularExpression("[1-9]\\d*,[1-9]\\d*").match(strMaxGuestResolutionPolicy).hasMatch())
        return MaxGuestResolutionPolicy_Fixed;
    /* Any type for unknown words: */
    if (!keys.contains(strMaxGuestResolutionPolicy, Qt::CaseInsensitive))
        return MaxGuestResolutionPolicy_Any;
    /* Corresponding type for known words: */
    return values.at(keys.indexOf(QRegExp(strMaxGuestResolutionPolicy, Qt::CaseInsensitive)));
}

