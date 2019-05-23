/* $Id: UIInformationDataItem.cpp $ */
/** @file
 * VBox Qt GUI - UIInformationDataItem class implementation.
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
# include <QDir>
# include <QTimer>

/* GUI includes: */
# include "VBoxGlobal.h"
# include "UIMachine.h"
# include "UISession.h"
# include "UIConverter.h"
# include "UIInformationItem.h"
# include "UIInformationModel.h"
# include "UIInformationDataItem.h"

/* COM includes: */
# include "CMedium.h"
# include "CSerialPort.h"
# include "CVRDEServer.h"
# include "CAudioAdapter.h"
# include "CSharedFolder.h"
# include "CUSBController.h"
# include "CNetworkAdapter.h"
# include "CVRDEServerInfo.h"
# include "CUSBDeviceFilter.h"
# include "CMediumAttachment.h"
# include "CSystemProperties.h"
# include "CUSBDeviceFilters.h"
# include "CStorageController.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/*********************************************************************************************************************************
*   Class UIInformationDataItem implementation.                                                                                  *
*********************************************************************************************************************************/

UIInformationDataItem::UIInformationDataItem(InformationElementType enmType, const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : m_enmType(enmType)
    , m_machine(machine)
    , m_console(console)
    , m_pModel(pModel)
{
}

QVariant UIInformationDataItem::data(const QModelIndex & /* index */, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DisplayRole:  return gpConverter->toString(m_enmType);
        case Qt::UserRole + 2: return m_enmType;
        default:               break;
    }

    /* Return null QVariant by default: */
    return QVariant();
}


/*********************************************************************************************************************************
*   Class UIInformationDataGeneral implementation.                                                                               *
*********************************************************************************************************************************/

UIInformationDataGeneral::UIInformationDataGeneral(const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : UIInformationDataItem(InformationElementType_General, machine, console, pModel)
{
}

QVariant UIInformationDataGeneral::data(const QModelIndex &index, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DecorationRole:
        {
            return QString(":/machine_16px.png");
        }

        case Qt::UserRole + 1:
        {
            UITextTable p_text;
            p_text << UITextTableLine(tr("Name", "details report"), m_machine.GetName());
            p_text << UITextTableLine(tr("OS Type", "details report"), vboxGlobal().vmGuestOSTypeDescription(m_machine.GetOSTypeId()));
            return QVariant::fromValue(p_text);
        }

        default:
            break;
    }

    /* Call to base-class: */
    return UIInformationDataItem::data(index, role);
}


/*********************************************************************************************************************************
*   Class UIInformationDataSystem implementation.                                                                                *
*********************************************************************************************************************************/

UIInformationDataSystem::UIInformationDataSystem(const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : UIInformationDataItem(InformationElementType_System, machine, console, pModel)
{
}

QVariant UIInformationDataSystem::data(const QModelIndex &index, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DecorationRole:
        {
            return QString(":/chipset_16px.png");
        }

        case Qt::UserRole + 1:
        {
#ifdef VBOX_WITH_FULL_DETAILS_REPORT
            /* BIOS Settings holder: */
            CBIOSSettings biosSettings = aMachine.GetBIOSSettings();
            /* ACPI: */
            QString acpi = biosSettings.GetACPIEnabled() ? tr("Enabled", "details report (ACPI)") :
                                                           tr("Disabled", "details report (ACPI)");
            /* I/O APIC: */
            QString ioapic = biosSettings.GetIOAPICEnabled() ? tr("Enabled", "details report (I/O APIC)") :
                                                               tr("Disabled", "details report (I/O APIC)");
            /* PAE/NX: */
            QString pae = aMachine.GetCpuProperty(KCpuPropertyType_PAE) ? tr("Enabled", "details report (PAE/NX)") :
                                                                          tr("Disabled", "details report (PAE/NX)");

            iRowCount += 3; /* Full report rows: */
#endif /* VBOX_WITH_FULL_DETAILS_REPORT */
            /* Boot order: */
            QString bootOrder;
            for (ulong i = 1; i <= vboxGlobal().virtualBox().GetSystemProperties().GetMaxBootPosition(); ++i)
            {
                KDeviceType device = m_machine.GetBootOrder(i);
                if (device == KDeviceType_Null)
                    continue;
                if (!bootOrder.isEmpty())
                    bootOrder += ", ";
                bootOrder += gpConverter->toString(device);
            }
            if (bootOrder.isEmpty())
                bootOrder = gpConverter->toString(KDeviceType_Null);

            /* Prepare data: */
            UITextTable p_text;
            p_text << UITextTableLine(tr("Base Memory", "details report"), QString::number(m_machine.GetMemorySize()));
            p_text << UITextTableLine(tr("Processor(s)", "details report"), QString::number(m_machine.GetCPUCount()));
            p_text << UITextTableLine(tr("Execution Cap", "details report"), QString::number(m_machine.GetCPUExecutionCap()));
            p_text << UITextTableLine(tr("Boot Order", "details report"), bootOrder);
#ifdef VBOX_WITH_FULL_DETAILS_REPORT
            p_text << UITextTableLine(tr("ACPI", "details report"), acpi);
            p_text << UITextTableLine(tr("I/O APIC", "details report"), ioapic);
            p_text << UITextTableLine(tr("PAE/NX", "details report"), pae);
#endif /* VBOX_WITH_FULL_DETAILS_REPORT */

            /* VT-x/AMD-V availability: */
            bool fVTxAMDVSupported = vboxGlobal().host().GetProcessorFeature(KProcessorFeature_HWVirtEx);
            if (fVTxAMDVSupported)
            {
                /* VT-x/AMD-V: */
                QString virt = m_machine.GetHWVirtExProperty(KHWVirtExPropertyType_Enabled) ?
                               tr("Enabled", "details report (VT-x/AMD-V)") :
                               tr("Disabled", "details report (VT-x/AMD-V)");
                p_text << UITextTableLine(tr("VT-x/AMD-V", "details report"), virt);
                /* Nested Paging: */
                QString nested = m_machine.GetHWVirtExProperty(KHWVirtExPropertyType_NestedPaging) ?
                                 tr("Enabled", "details report (Nested Paging)") :
                                 tr("Disabled", "details report (Nested Paging)");
                p_text << UITextTableLine(tr("Nested Paging", "details report"), nested);
            }
            /* Paravirtualization Interface: */
            const QString strParavirtProvider = gpConverter->toString(m_machine.GetParavirtProvider());
            p_text << UITextTableLine(tr("Paravirtualization Interface", "details report"), strParavirtProvider);

            return QVariant::fromValue(p_text);
        }

        default:
            break;
    }

    /* Call to base-class: */
    return UIInformationDataItem::data(index, role);
}


/*********************************************************************************************************************************
*   Class UIInformationDataDisplay implementation.                                                                               *
*********************************************************************************************************************************/

UIInformationDataDisplay::UIInformationDataDisplay(const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : UIInformationDataItem(InformationElementType_Display, machine, console, pModel)
{
}

QVariant UIInformationDataDisplay::data(const QModelIndex &index, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DecorationRole:
        {
            return QString(":/vrdp_16px.png");
        }

        case Qt::UserRole + 1:
        {
            UITextTable p_text;
            /* Video tab: */
            p_text << UITextTableLine(tr("Video Memory", "details report"), QString::number(m_machine.GetVRAMSize()));

            int cGuestScreens = m_machine.GetMonitorCount();
            if (cGuestScreens > 1)
                p_text << UITextTableLine(tr("Screens", "details report"), QString::number(cGuestScreens));

            QString acc3d = m_machine.GetAccelerate3DEnabled() && vboxGlobal().is3DAvailable() ?
                            tr("Enabled", "details report (3D Acceleration)") :
                            tr("Disabled", "details report (3D Acceleration)");
            p_text << UITextTableLine(tr("3D Acceleration", "details report"), acc3d);

#ifdef VBOX_WITH_VIDEOHWACCEL
            QString acc2dVideo = m_machine.GetAccelerate2DVideoEnabled() ?
                                 tr("Enabled", "details report (2D Video Acceleration)") :
                                 tr("Disabled", "details report (2D Video Acceleration)");
            p_text << UITextTableLine(tr("2D Video Acceleration", "details report"), acc2dVideo);
#endif

            /* VRDP tab: */
            CVRDEServer srv = m_machine.GetVRDEServer();
            if (!srv.isNull())
            {
                if (srv.GetEnabled())
                    p_text << UITextTableLine(tr("Remote Desktop Server Port", "details report (VRDE Server)"), srv.GetVRDEProperty("TCP/Ports"));
                else
                    p_text << UITextTableLine(tr("Remote Desktop Server", "details report (VRDE Server)"), tr("Disabled", "details report (VRDE Server)"));
            }

            return QVariant::fromValue(p_text);
        }

        default:
            break;
    }

    /* Call to base-class: */
    return UIInformationDataItem::data(index, role);
}


/*********************************************************************************************************************************
*   Class UIInformationDataStorage implementation.                                                                               *
*********************************************************************************************************************************/

UIInformationDataStorage::UIInformationDataStorage(const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : UIInformationDataItem(InformationElementType_Storage, machine, console, pModel)
{
}

QVariant UIInformationDataStorage::data(const QModelIndex &index, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DecorationRole:
        {
            return QString(":/hd_16px.png");
        }

        case Qt::UserRole + 1:
        {
            UITextTable p_text;

            /* Iterate over the all machine controllers: */
            CStorageControllerVector controllers = m_machine.GetStorageControllers();
            for (int i = 0; i < controllers.size(); ++i)
            {
                /* Get current controller: */
                const CStorageController &controller = controllers[i];
                /* Add controller information: */
                QString strControllerName = QApplication::translate("UIMachineSettingsStorage", "Controller: %1");
                p_text << UITextTableLine(strControllerName.arg(controller.GetName()), QString());

                CMediumAttachmentVector attachments = m_machine.GetMediumAttachmentsOfController(controller.GetName());
                for (int j = 0; j < attachments.size(); ++j)
                {
                    /* Get current attachment: */
                    const CMediumAttachment &attachment = attachments[j];
                    /* Append 'device slot name' with 'device type name' for optical devices only: */
                    QString strDeviceType = attachment.GetType() == KDeviceType_DVD ? tr("(Optical Drive)") : QString();
                    if (!strDeviceType.isNull())
                        strDeviceType.prepend(' ');
                    /* Prepare current medium object: */
                    const CMedium &medium = attachment.GetMedium();
                    /* Prepare information about current medium & attachment: */
                    QString strAttachmentInfo = gpConverter->toString(StorageSlot(controller.GetBus(),
                                                                      attachment.GetPort(),
                                                                      attachment.GetDevice())) + strDeviceType;

                    /* Insert that attachment into map: */
                    if (attachment.isOk())
                        p_text << UITextTableLine(strAttachmentInfo, vboxGlobal().details(medium, false, false));
                }
            }

            return QVariant::fromValue(p_text);
        }

        default:
            break;
    }

    /* Call to base-class: */
    return UIInformationDataItem::data(index, role);
}


/*********************************************************************************************************************************
*   Class UIInformationDataAudio implementation.                                                                                 *
*********************************************************************************************************************************/

UIInformationDataAudio::UIInformationDataAudio(const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : UIInformationDataItem(InformationElementType_Audio, machine, console, pModel)
{
}

QVariant UIInformationDataAudio::data(const QModelIndex &index, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DecorationRole:
        {
            return QString(":/sound_16px.png");
        }

        case Qt::UserRole + 1:
        {
            UITextTable p_text;

            CAudioAdapter audio = m_machine.GetAudioAdapter();
            if (audio.GetEnabled())
            {
                p_text << UITextTableLine(tr("Host Driver", "details report (audio)"), gpConverter->toString(audio.GetAudioDriver()));
                p_text << UITextTableLine(tr("Controller", "details report (audio)"), gpConverter->toString(audio.GetAudioController()));
            }

            return QVariant::fromValue(p_text);
        }

        default:
            break;
    }

    /* Call to base-class: */
    return UIInformationDataItem::data(index, role);
}


/*********************************************************************************************************************************
*   Class UIInformationDataNetwork implementation.                                                                               *
*********************************************************************************************************************************/

UIInformationDataNetwork::UIInformationDataNetwork(const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : UIInformationDataItem(InformationElementType_Network, machine, console, pModel)
{
}

QVariant UIInformationDataNetwork::data(const QModelIndex &index, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DecorationRole:
        {
            return QString(":/nw_16px.png");
        }

        case Qt::UserRole + 1:
        {
            UITextTable p_text;

            ulong count = vboxGlobal().virtualBox().GetSystemProperties().GetMaxNetworkAdapters(m_machine.GetChipsetType());
            for (ulong slot = 0; slot < count; slot++)
            {
                CNetworkAdapter adapter = m_machine.GetNetworkAdapter(slot);
                if (adapter.GetEnabled())
                {
                    KNetworkAttachmentType type = adapter.GetAttachmentType();
                    QString attType = gpConverter->toString(adapter.GetAdapterType())
                                      .replace(QRegExp ("\\s\\(.+\\)"), " (%1)");
                    /* don't use the adapter type string for types that have
                     * an additional symbolic network/interface name field, use
                     * this name instead */
                    if (type == KNetworkAttachmentType_Bridged)
                        attType = attType.arg(tr("Bridged adapter, %1",
                            "details report (network)").arg(adapter.GetBridgedInterface()));
                    else if (type == KNetworkAttachmentType_Internal)
                        attType = attType.arg(tr("Internal network, '%1'",
                            "details report (network)").arg(adapter.GetInternalNetwork()));
                    else if (type == KNetworkAttachmentType_HostOnly)
                        attType = attType.arg(tr("Host-only adapter, '%1'",
                            "details report (network)").arg(adapter.GetHostOnlyInterface()));
                    else if (type == KNetworkAttachmentType_Generic)
                        attType = attType.arg(tr("Generic, '%1'",
                            "details report (network)").arg(adapter.GetGenericDriver()));
                    else if (type == KNetworkAttachmentType_NATNetwork)
                        attType = attType.arg(tr("NAT network, '%1'",
                            "details report (network)").arg(adapter.GetNATNetwork()));
                    else
                        attType = attType.arg(gpConverter->toString(type));

                    p_text << UITextTableLine(tr("Adapter %1", "details report (network)").arg(adapter.GetSlot() + 1), attType);
                }
            }

            return QVariant::fromValue(p_text);
        }

        default:
            break;
    }

    /* Call to base-class: */
    return UIInformationDataItem::data(index, role);
}


/*********************************************************************************************************************************
*   Class UIInformationDataSerialPorts implementation.                                                                           *
*********************************************************************************************************************************/

UIInformationDataSerialPorts::UIInformationDataSerialPorts(const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : UIInformationDataItem(InformationElementType_Serial, machine, console, pModel)
{
}

QVariant UIInformationDataSerialPorts::data(const QModelIndex &index, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DecorationRole:
        {
            return QString(":/serial_port_16px.png");
        }

        case Qt::UserRole + 1:
        {
            UITextTable p_text;
            ulong count = vboxGlobal().virtualBox().GetSystemProperties().GetSerialPortCount();

            for (ulong slot = 0; slot < count; slot++)
            {
                CSerialPort port = m_machine.GetSerialPort(slot);
                if (port.GetEnabled())
                {
                    KPortMode mode = port.GetHostMode();
                    QString data = vboxGlobal().toCOMPortName(port.GetIRQ(), port.GetIOBase()) + ", ";
                    if (mode == KPortMode_HostPipe ||
                        mode == KPortMode_HostDevice ||
                        mode == KPortMode_TCP ||
                        mode == KPortMode_RawFile)
                        data += QString("%1 (<nobr>%2</nobr>)")
                                       .arg(gpConverter->toString(mode))
                                       .arg(QDir::toNativeSeparators(port.GetPath()));
                    else
                        data += gpConverter->toString(mode);

                    p_text << UITextTableLine(tr("Port %1", "details report (serial ports)").arg(port.GetSlot() + 1), data);
                }
            }

            return QVariant::fromValue(p_text);
        }

        default:
            break;
    }

    /* Call to base-class: */
    return UIInformationDataItem::data(index, role);
}


/*********************************************************************************************************************************
*   Class UIInformationDataUSB implementation.                                                                                   *
*********************************************************************************************************************************/

UIInformationDataUSB::UIInformationDataUSB(const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : UIInformationDataItem(InformationElementType_USB, machine, console, pModel)
{
}

QVariant UIInformationDataUSB::data(const QModelIndex &index, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DecorationRole:
        {
            return QString(":/usb_16px.png");
        }

        case Qt::UserRole + 1:
        {
            UITextTable p_text;

            CUSBDeviceFilters flts = m_machine.GetUSBDeviceFilters();
            if (   !flts.isNull()
                && m_machine.GetUSBProxyAvailable())
            {
                /* The USB controller may be unavailable (i.e. in VirtualBox OSE): */
                if (m_machine.GetUSBControllers().isEmpty())
                    p_text << UITextTableLine(tr("Disabled", "details report (USB)"), QString());
                else
                {
                    CUSBDeviceFilterVector coll = flts.GetDeviceFilters();
                    uint active = 0;
                    for (int i = 0; i < coll.size(); ++i)
                        if (coll[i].GetActive())
                            active ++;

                    p_text << UITextTableLine(tr("Device Filters", "details report (USB)"), tr("%1 (%2 active)", "details report (USB)")
                                                                                              .arg(coll.size()).arg(active));
                }
            }
        }

        default:
            break;
    }

    /* Call to base-class: */
    return UIInformationDataItem::data(index, role);
}


/*********************************************************************************************************************************
*   Class UIInformationDataSharedFolders implementation.                                                                         *
*********************************************************************************************************************************/

UIInformationDataSharedFolders::UIInformationDataSharedFolders(const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : UIInformationDataItem(InformationElementType_SharedFolders, machine, console, pModel)
{
    connect(gpMachine->uisession(), SIGNAL(sigSharedFolderChange()), this, SLOT(updateData()));
}

QVariant UIInformationDataSharedFolders::data(const QModelIndex &index, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DecorationRole:
        {
            return QString(":/sf_16px.png");
        }

        case Qt::UserRole + 1:
        {
            UITextTable p_text;

            ulong count = m_machine.GetSharedFolders().size();
            if (count > 0)
                p_text << UITextTableLine(tr("Shared Folders", "details report (shared folders)"), QString::number(count));

            return QVariant::fromValue(p_text);
        }

        default:
            break;
    }

    /* Call to base-class: */
    return UIInformationDataItem::data(index, role);
}

void UIInformationDataSharedFolders::updateData()
{
    m_pModel->updateData(this);
}


/*********************************************************************************************************************************
*   Class UIInformationDataRuntimeAttributes implementation.                                                                     *
*********************************************************************************************************************************/

UIInformationDataRuntimeAttributes::UIInformationDataRuntimeAttributes(const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : UIInformationDataItem(InformationElementType_RuntimeAttributes, machine, console, pModel)
{
}

QVariant UIInformationDataRuntimeAttributes::data(const QModelIndex &index, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DecorationRole:
        {
            return QString(":/state_running_16px.png");
        }

        case Qt::UserRole + 1:
        {
            UITextTable p_text;

            ULONG cGuestScreens = m_machine.GetMonitorCount();
            QVector<QString> aResolutions(cGuestScreens);
            for (ULONG iScreen = 0; iScreen < cGuestScreens; ++iScreen)
            {
                /* Determine resolution: */
                ULONG uWidth = 0;
                ULONG uHeight = 0;
                ULONG uBpp = 0;
                LONG xOrigin = 0;
                LONG yOrigin = 0;
                KGuestMonitorStatus monitorStatus = KGuestMonitorStatus_Enabled;
                m_console.GetDisplay().GetScreenResolution(iScreen, uWidth, uHeight, uBpp, xOrigin, yOrigin, monitorStatus);
                QString strResolution = QString("%1x%2").arg(uWidth).arg(uHeight);
                if (uBpp)
                    strResolution += QString("x%1").arg(uBpp);
                strResolution += QString(" @%1,%2").arg(xOrigin).arg(yOrigin);
                if (monitorStatus == KGuestMonitorStatus_Disabled)
                {
                    strResolution += QString(" ");
                    strResolution += QString(VBoxGlobal::tr("off", "guest monitor status"));
                }
                aResolutions[iScreen] = strResolution;
            }

            /* Determine uptime: */
            CMachineDebugger debugger = m_console.GetDebugger();
            uint32_t uUpSecs = (debugger.GetUptime() / 5000) * 5;
            char szUptime[32];
            uint32_t uUpDays = uUpSecs / (60 * 60 * 24);
            uUpSecs -= uUpDays * 60 * 60 * 24;
            uint32_t uUpHours = uUpSecs / (60 * 60);
            uUpSecs -= uUpHours * 60 * 60;
            uint32_t uUpMins  = uUpSecs / 60;
            uUpSecs -= uUpMins * 60;
            RTStrPrintf(szUptime, sizeof(szUptime), "%dd %02d:%02d:%02d",
                        uUpDays, uUpHours, uUpMins, uUpSecs);
            QString strUptime = QString(szUptime);

            /* Determine clipboard mode: */
            QString strClipboardMode = gpConverter->toString(m_machine.GetClipboardMode());
            /* Determine Drag&Drop mode: */
            QString strDnDMode = gpConverter->toString(m_machine.GetDnDMode());

            /* Determine virtualization attributes: */
            QString strVirtualization = debugger.GetHWVirtExEnabled() ?
                                        VBoxGlobal::tr("Active", "details report (VT-x/AMD-V)") :
                                        VBoxGlobal::tr("Inactive", "details report (VT-x/AMD-V)");
            QString strNestedPaging = debugger.GetHWVirtExNestedPagingEnabled() ?
                                      VBoxGlobal::tr("Active", "details report (Nested Paging)") :
                                      VBoxGlobal::tr("Inactive", "details report (Nested Paging)");
            QString strUnrestrictedExecution = debugger.GetHWVirtExUXEnabled() ?
                                               VBoxGlobal::tr("Active", "details report (Unrestricted Execution)") :
                                               VBoxGlobal::tr("Inactive", "details report (Unrestricted Execution)");
            QString strParavirtProvider = gpConverter->toString(m_machine.GetEffectiveParavirtProvider());

            /* Guest information: */
            CGuest guest = m_console.GetGuest();
            QString strGAVersion = guest.GetAdditionsVersion();
            if (strGAVersion.isEmpty())
                strGAVersion = tr("Not Detected", "guest additions");
            else
            {
                ULONG uRevision = guest.GetAdditionsRevision();
                if (uRevision != 0)
                    strGAVersion += QString(" r%1").arg(uRevision);
            }
            QString strOSType = guest.GetOSTypeId();
            if (strOSType.isEmpty())
                strOSType = tr("Not Detected", "guest os type");
            else
                strOSType = vboxGlobal().vmGuestOSTypeDescription(strOSType);

            /* VRDE information: */
            int iVRDEPort = m_console.GetVRDEServerInfo().GetPort();
            QString strVRDEInfo = (iVRDEPort == 0 || iVRDEPort == -1)?
                tr("Not Available", "details report (VRDE server port)") :
                QString("%1").arg(iVRDEPort);

            /* Searching for longest string: */
            QStringList values;
            for (ULONG iScreen = 0; iScreen < cGuestScreens; ++iScreen)
                values << aResolutions[iScreen];
            values << strUptime
                   << strVirtualization << strNestedPaging << strUnrestrictedExecution
                   << strGAVersion << strOSType << strVRDEInfo;
            int iMaxLength = 0;
            foreach (const QString &strValue, values)
                iMaxLength = iMaxLength < QApplication::fontMetrics().width(strValue)
                             ? QApplication::fontMetrics().width(strValue) : iMaxLength;

            /* Summary: */
            for (ULONG iScreen = 0; iScreen < cGuestScreens; ++iScreen)
            {
                QString strLabel(tr("Screen Resolution"));
                /* The screen number makes sense only if there are multiple monitors in the guest: */
                if (cGuestScreens > 1)
                    strLabel += QString(" %1").arg(iScreen + 1);
                p_text << UITextTableLine(strLabel, aResolutions[iScreen]);
            }

            p_text << UITextTableLine(tr("VM Uptime"), strUptime);
            p_text << UITextTableLine(tr("Clipboard Mode"), strClipboardMode);
            p_text << UITextTableLine(tr("Drag and Drop Mode"), strDnDMode);
            p_text << UITextTableLine(tr("VT-x/AMD-V", "details report"), strVirtualization);
            p_text << UITextTableLine(tr("Nested Paging", "details report"), strNestedPaging);
            p_text << UITextTableLine(tr("Unrestricted Execution", "details report"), strUnrestrictedExecution);
            p_text << UITextTableLine(tr("Paravirtualization Interface", "details report"), strParavirtProvider);
            p_text << UITextTableLine(tr("Guest Additions"), strGAVersion);
            p_text << UITextTableLine(tr("Guest OS Type", "details report"), strOSType);
            p_text << UITextTableLine(tr("Remote Desktop Server Port", "details report (VRDE Server)"), strVRDEInfo);

            return QVariant::fromValue(p_text);
        }

        default:
            break;
    }

    /* Call to base-class: */
    return UIInformationDataItem::data(index, role);
}


/*********************************************************************************************************************************
*   Class UIInformationDataNetworkStatistics implementation.                                                                     *
*********************************************************************************************************************************/

UIInformationDataNetworkStatistics::UIInformationDataNetworkStatistics(const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : UIInformationDataItem(InformationElementType_NetworkStatistics, machine, console, pModel)
{
    /* Network statistics: */
    ulong count = vboxGlobal().virtualBox().GetSystemProperties().GetMaxNetworkAdapters(m_machine.GetChipsetType());
    for (ulong i = 0; i < count; ++i)
    {
        CNetworkAdapter na = machine.GetNetworkAdapter(i);
        KNetworkAdapterType ty = na.GetAdapterType();
        const char *name;

        switch (ty)
        {
            case KNetworkAdapterType_I82540EM:
            case KNetworkAdapterType_I82543GC:
            case KNetworkAdapterType_I82545EM:
                name = "E1k";
                break;
            case KNetworkAdapterType_Virtio:
                name = "VNet";
                break;
            default:
                name = "PCNet";
                break;
        }

        /* Names: */
        m_names[QString("/Devices/%1%2/TransmitBytes").arg(name).arg(i)] = tr("Data Transmitted");
        m_names[QString("/Devices/%1%2/ReceiveBytes").arg(name).arg(i)] = tr("Data Received");

        /* Units: */
        m_units[QString("/Devices/%1%2/TransmitBytes").arg(name).arg(i)] = "B";
        m_units[QString("/Devices/%1%2/ReceiveBytes").arg(name).arg(i)] = "B";

        /* Belongs to: */
        m_links[QString("NA%1").arg(i)] = QStringList()
            << QString("/Devices/%1%2/TransmitBytes").arg(name).arg(i)
            << QString("/Devices/%1%2/ReceiveBytes").arg(name).arg(i);
    }

    m_pTimer = new QTimer(this);

    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(sltProcessStatistics()));

    /* Statistics page update: */
    sltProcessStatistics();
    m_pTimer->start(5000);
}

QVariant UIInformationDataNetworkStatistics::data(const QModelIndex &index, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DecorationRole:
        {
            return QString(":/nw_16px.png");
        }

        case Qt::UserRole + 1:
        {
            UITextTable p_text;

            /* Enumerate network-adapters: */
            ulong uCount = vboxGlobal().virtualBox().GetSystemProperties().GetMaxNetworkAdapters(m_machine.GetChipsetType());
            for (ulong uSlot = 0; uSlot < uCount; ++uSlot)
            {
                /* Skip disabled adapters: */
                if (m_machine.GetNetworkAdapter(uSlot).GetEnabled())
                {
                    QStringList keys = m_links[QString("NA%1").arg(uSlot)];
                    p_text << UITextTableLine(VBoxGlobal::tr("Adapter %1", "details report (network)").arg(uSlot + 1), QString());

                    foreach (QString strKey, keys)
                        p_text << UITextTableLine(m_names[strKey], QString("%1 %2").arg(m_values[strKey]).arg(m_units[strKey]));
                }
            }
            return QVariant::fromValue(p_text);
        }

        default:
            break;
    }

    /* Call to base-class: */
    return UIInformationDataItem::data(index, role);
}

QString UIInformationDataNetworkStatistics::parseStatistics(const QString &strText)
{
    /* Filters VM statistics counters body: */
    QRegExp query("^.+<Statistics>\n(.+)\n</Statistics>.*$");
    if (query.indexIn(strText) == -1)
        return QString();

    /* Split whole VM statistics text to lines: */
    const QStringList text = query.cap(1).split("\n");

    /* Iterate through all VM statistics: */
    ULONG64 uSumm = 0;
    for (QStringList::const_iterator lineIt = text.begin(); lineIt != text.end(); ++lineIt)
    {
        /* Get current line: */
        QString strLine = *lineIt;
        strLine.remove(1, 1);
        strLine.remove(strLine.length() -2, 2);

        /* Parse incoming counter and fill the counter-element values: */
        CounterElementType counter;
        counter.type = strLine.section(" ", 0, 0);
        strLine = strLine.section(" ", 1);
        QStringList list = strLine.split("\" ");
        for (QStringList::Iterator it = list.begin(); it != list.end(); ++it)
        {
            QString pair = *it;
            QRegExp regExp("^(.+)=\"([^\"]*)\"?$");
            regExp.indexIn(pair);
            counter.list.insert(regExp.cap(1), regExp.cap(2));
        }

        /* Fill the output with the necessary counter's value.
         * Currently we are using "c" field of simple counter only. */
        QString result = counter.list.contains("c") ? counter.list["c"] : "0";
        uSumm += result.toULongLong();
    }

    return QString::number(uSumm);
}

void UIInformationDataNetworkStatistics::sltProcessStatistics()
{
    /* Get machine debugger: */
    CMachineDebugger dbg = m_console.GetDebugger();
    QString strInfo;

    /* Process selected VM statistics: */
    for (DataMapType::const_iterator it = m_names.begin(); it != m_names.end(); ++it)
    {
        strInfo = dbg.GetStats(it.key(), true);
        m_values[it.key()] = parseStatistics(strInfo);
    }

    QModelIndex index = m_pModel->index(1,0);
    m_pModel->updateData(index);
}


/*********************************************************************************************************************************
*   Class UIInformationDataStorageStatistics implementation.                                                                     *
*********************************************************************************************************************************/

UIInformationDataStorageStatistics::UIInformationDataStorageStatistics(const CMachine &machine, const CConsole &console, UIInformationModel *pModel)
    : UIInformationDataItem(InformationElementType_StorageStatistics, machine, console, pModel)
{
    /* Storage statistics: */
    CSystemProperties sp = vboxGlobal().virtualBox().GetSystemProperties();
    CStorageControllerVector controllers = m_machine.GetStorageControllers();
    int iIDECount = 0;
    foreach (const CStorageController &controller, controllers)
    {
        KStorageBus enmBus = controller.GetBus();

        switch (enmBus)
        {
            case KStorageBus_IDE:
            {
                for (ULONG i = 0; i < sp.GetMaxPortCountForStorageBus(KStorageBus_IDE); ++i)
                {
                    for (ULONG j = 0; j < sp.GetMaxDevicesPerPortForStorageBus(KStorageBus_IDE); ++j)
                    {
                        /* Names: */
                        m_names[QString("/Devices/IDE%1/ATA%2/Unit%3/*DMA")
                            .arg(iIDECount).arg(i).arg(j)] = tr("DMA Transfers");
                        m_names[QString("/Devices/IDE%1/ATA%2/Unit%3/*PIO")
                            .arg(iIDECount).arg(i).arg(j)] = tr("PIO Transfers");
                        m_names[QString("/Devices/IDE%1/ATA%2/Unit%3/ReadBytes")
                            .arg(iIDECount).arg(i).arg(j)] = tr("Data Read");
                        m_names[QString("/Devices/IDE%1/ATA%2/Unit%3/WrittenBytes")
                            .arg(iIDECount).arg(i).arg(j)] = tr("Data Written");

                        /* Units: */
                        m_units[QString("/Devices/IDE%1/ATA%2/Unit%3/*DMA")
                            .arg(iIDECount).arg(i).arg(j)] = "";
                        m_units[QString("/Devices/IDE%1/ATA%2/Unit%3/*PIO")
                            .arg(iIDECount).arg(i).arg(j)] = "";
                        m_units[QString("/Devices/IDE%1/ATA%2/Unit%3/ReadBytes")
                            .arg(iIDECount).arg(i).arg(j)] = "B";
                        m_units[QString("/Devices/IDE%1/ATA%2/Unit%3/WrittenBytes")
                            .arg(iIDECount).arg(i).arg(j)] = "B";

                        /* Belongs to */
                        m_links[QString("/Devices/IDE%1/ATA%2/Unit%3").arg(iIDECount).arg(i).arg(j)] = QStringList()
                            << QString("/Devices/IDE%1/ATA%2/Unit%3/*DMA").arg(iIDECount).arg(i).arg(j)
                            << QString("/Devices/IDE%1/ATA%2/Unit%3/*PIO").arg(iIDECount).arg(i).arg(j)
                            << QString("/Devices/IDE%1/ATA%2/Unit%3/ReadBytes").arg(iIDECount).arg(i).arg(j)
                            << QString("/Devices/IDE%1/ATA%2/Unit%3/WrittenBytes").arg(iIDECount).arg(i).arg(j);
                    }
                }
                ++iIDECount;
                break;
            }
            default:
            {
                /* Common code for the non IDE controllers. */
                uint32_t iInstance = controller.GetInstance();
                const char *pszCtrl = storCtrlType2Str(controller.GetControllerType());

                for (ULONG i = 0; i < sp.GetMaxPortCountForStorageBus(enmBus); ++i)
                {
                    for (ULONG j = 0; j < sp.GetMaxDevicesPerPortForStorageBus(enmBus); ++j)
                    {
                        /* Names: */
                        m_names[QString("/Devices/%1%2/Port%3/ReqsSubmitted").arg(pszCtrl).arg(iInstance).arg(i)]
                            = tr("Requests");
                        m_names[QString("/Devices/%1%2/Port%3/ReadBytes").arg(pszCtrl).arg(iInstance).arg(i)]
                            = tr("Data Read");
                        m_names[QString("/Devices/%1%2/Port%3/WrittenBytes").arg(pszCtrl).arg(iInstance).arg(i)]
                            = tr("Data Written");

                        /* Units: */
                        m_units[QString("/Devices/%1%2/Port%3/ReqsSubmitted").arg(pszCtrl).arg(iInstance).arg(i)] = "";
                        m_units[QString("/Devices/%1%2/Port%3/ReadBytes").arg(pszCtrl).arg(iInstance).arg(i)] = "B";
                        m_units[QString("/Devices/%1%2/Port%3/WrittenBytes").arg(pszCtrl).arg(iInstance).arg(i)] = "B";

                        /* Belongs to: */
                        m_links[QString("/Devices/%1%2/Port%3").arg(pszCtrl).arg(iInstance).arg(i)] = QStringList()
                            << QString("/Devices/%1%2/Port%3/ReqsSubmitted").arg(pszCtrl).arg(iInstance).arg(i)
                            << QString("/Devices/%1%2/Port%3/ReadBytes").arg(pszCtrl).arg(iInstance).arg(i)
                            << QString("/Devices/%1%2/Port%3/WrittenBytes").arg(pszCtrl).arg(iInstance).arg(i);
                    }
                }

                break;
            }
        }
    }

    m_pTimer = new QTimer(this);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(sltProcessStatistics()));
    /* Statistics page update: */
    sltProcessStatistics();
    m_pTimer->start(5000);
}

QVariant UIInformationDataStorageStatistics::data(const QModelIndex &index, int role) const
{
    /* For particular role: */
    switch (role)
    {
        case Qt::DecorationRole:
        {
            return QString(":/hd_16px.png");
        }

        case Qt::UserRole + 1:
        {
            UITextTable p_text;

            CStorageControllerVector controllers = m_machine.GetStorageControllers();
            int iIDECount = 0;
            foreach (const CStorageController &controller, controllers)
            {
                /* Get controller attributes: */
                QString strName = controller.GetName();
                KStorageBus busType = controller.GetBus();
                CMediumAttachmentVector attachments = m_machine.GetMediumAttachmentsOfController(strName);
                /* Skip empty and floppy attachments: */
                if (!attachments.isEmpty() && busType != KStorageBus_Floppy)
                {
                    /* Prepare full controller name: */
                    QString strControllerName = QApplication::translate("UIMachineSettingsStorage", "Controller: %1");
                    p_text << UITextTableLine(strControllerName.arg(controller.GetName()), QString());

                    /* Enumerate storage-attachments: */
                    foreach (const CMediumAttachment &attachment, attachments)
                    {
                        const LONG iPort = attachment.GetPort();
                        const LONG iDevice = attachment.GetDevice();

                        CStorageController ctr = m_machine.GetStorageControllerByName(strName);
                        QString strSecondName = gpConverter->toString(StorageSlot(ctr.GetBus(), iPort, iDevice));
                        p_text << UITextTableLine(QString(), QString());
                        p_text << UITextTableLine(strSecondName, QString());

                        switch (busType)
                        {
                            case KStorageBus_IDE:
                            {
                                QStringList keys = m_links[QString("/Devices/IDE%1/ATA%2/Unit%3").arg(iIDECount).arg(iPort).arg(iDevice)];
                                foreach (QString strKey, keys)
                                    p_text << UITextTableLine(m_names[strKey], QString("%1 %2").arg(m_values[strKey]).arg(m_units[strKey]));
                                break;

                            }
                            default:
                            {
                                uint32_t iInstance = ctr.GetInstance();
                                const KStorageControllerType enmCtrl = ctr.GetControllerType();
                                const char *pszCtrl = storCtrlType2Str(enmCtrl);
                                QStringList keys = m_links[QString("/Devices/%1%2/Port%3").arg(pszCtrl).arg(iInstance).arg(iPort)];
                                foreach (QString strKey, keys)
                                    p_text << UITextTableLine(m_names[strKey], QString("%1 %2").arg(m_values[strKey]).arg(m_units[strKey]));
                                break;
                            }
                        }
                    }
                }
                /* Increment controller counters: */
                switch (busType)
                {
                    case KStorageBus_IDE:  ++iIDECount; break;
                    default: break;
                }
            }
            return QVariant::fromValue(p_text);
        }

        default:
            break;
    }

    /* Call to base-class: */
    return UIInformationDataItem::data(index, role);
}

QString UIInformationDataStorageStatistics::parseStatistics(const QString &strText)
{
    /* Filters VM statistics counters body: */
    QRegExp query("^.+<Statistics>\n(.+)\n</Statistics>.*$");
    if (query.indexIn(strText) == -1)
        return QString();

    /* Split whole VM statistics text to lines: */
    const QStringList text = query.cap(1).split("\n");

    /* Iterate through all VM statistics: */
    ULONG64 uSumm = 0;
    for (QStringList::const_iterator lineIt = text.begin(); lineIt != text.end(); ++lineIt)
    {
        /* Get current line: */
        QString strLine = *lineIt;
        strLine.remove(1, 1);
        strLine.remove(strLine.length() -2, 2);

        /* Parse incoming counter and fill the counter-element values: */
        CounterElementType counter;
        counter.type = strLine.section(" ", 0, 0);
        strLine = strLine.section(" ", 1);
        QStringList list = strLine.split("\" ");
        for (QStringList::Iterator it = list.begin(); it != list.end(); ++it)
        {
            QString pair = *it;
            QRegExp regExp("^(.+)=\"([^\"]*)\"?$");
            regExp.indexIn(pair);
            counter.list.insert(regExp.cap(1), regExp.cap(2));
        }

        /* Fill the output with the necessary counter's value.
         * Currently we are using "c" field of simple counter only. */
        QString result = counter.list.contains("c") ? counter.list["c"] : "0";
        uSumm += result.toULongLong();
    }

    return QString::number(uSumm);
}

const char *UIInformationDataStorageStatistics::storCtrlType2Str(const KStorageControllerType enmCtrlType) const
{
    const char *pszCtrl = NULL;
    switch (enmCtrlType)
    {
        case KStorageControllerType_LsiLogic:
            pszCtrl = "LSILOGIC";
            break;
        case KStorageControllerType_BusLogic:
            pszCtrl = "BUSLOGIC";
            break;
        case KStorageControllerType_IntelAhci:
            pszCtrl = "AHCI";
            break;
        case KStorageControllerType_PIIX3:
        case KStorageControllerType_PIIX4:
        case KStorageControllerType_ICH6:
            pszCtrl = "PIIX3IDE";
            break;
        case KStorageControllerType_I82078:
            pszCtrl = "I82078";
            break;
        case KStorageControllerType_LsiLogicSas:
            pszCtrl = "LSILOGICSAS";
            break;
        case KStorageControllerType_USB:
            pszCtrl = "MSD";
            break;
        case KStorageControllerType_NVMe:
            pszCtrl = "NVME";
            break;
        default:
            AssertFailed();
            pszCtrl = "<INVALID>";
            break;
    }

    return pszCtrl;
}

void UIInformationDataStorageStatistics::sltProcessStatistics()
{
    /* Get machine debugger: */
    CMachineDebugger dbg = m_console.GetDebugger();
    QString strInfo;

    /* Process selected VM statistics: */
    for (DataMapType::const_iterator it = m_names.begin(); it != m_names.end(); ++it)
    {
        strInfo = dbg.GetStats(it.key(), true);
        m_values[it.key()] = parseStatistics(strInfo);
    }

    QModelIndex index = m_pModel->index(1,0);
    m_pModel->updateData(index);
}

