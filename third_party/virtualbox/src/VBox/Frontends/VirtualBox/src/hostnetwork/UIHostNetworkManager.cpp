/* $Id: UIHostNetworkManager.cpp $ */
/** @file
 * VBox Qt GUI - UIHostNetworkManager class implementation.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
# include <QHeaderView>
# include <QMenuBar>
# include <QPushButton>

/* GUI includes: */
# include "QIDialogButtonBox.h"
# include "QITreeWidget.h"
# include "UIExtraDataManager.h"
# include "UIIconPool.h"
# include "UIHostNetworkDetailsWidget.h"
# include "UIHostNetworkManager.h"
# include "UIHostNetworkUtils.h"
# include "UIMessageCenter.h"
# include "UIToolBar.h"
# ifdef VBOX_WS_MAC
#  include "UIWindowMenuManager.h"
# endif /* VBOX_WS_MAC */
# include "VBoxGlobal.h"

/* COM includes: */
# include "CDHCPServer.h"
# include "CHost.h"
# include "CHostNetworkInterface.h"

/* Other VBox includes: */
# include <iprt/cidr.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Tree-widget column tags. */
enum
{
    Column_Name,
    Column_IPv4,
    Column_IPv6,
    Column_DHCP,
    Column_Max,
};


/** Host Network Manager: Tree-widget item. */
class UIItemHostNetwork : public QITreeWidgetItem, public UIDataHostNetwork
{
public:

    /** Updates item fields from data. */
    void updateFields();

    /** Returns item name. */
    QString name() const { return m_interface.m_strName; }

private:

    /** Returns CIDR for a passed @a strMask. */
    static int maskToCidr(const QString &strMask);
};


/*********************************************************************************************************************************
*   Class UIItemHostNetwork implementation.                                                                                      *
*********************************************************************************************************************************/

void UIItemHostNetwork::updateFields()
{
    /* Compose item fields: */
    setText(Column_Name, m_interface.m_strName);
    setText(Column_IPv4, m_interface.m_strAddress.isEmpty() ? QString() :
                         QString("%1/%2").arg(m_interface.m_strAddress).arg(maskToCidr(m_interface.m_strMask)));
    setText(Column_IPv6, m_interface.m_strAddress6.isEmpty() || !m_interface.m_fSupportedIPv6 ? QString() :
                         QString("%1/%2").arg(m_interface.m_strAddress6).arg(m_interface.m_strPrefixLength6.toInt()));
    setText(Column_DHCP, UIHostNetworkManager::tr("Enable", "DHCP Server"));
    setCheckState(Column_DHCP, m_dhcpserver.m_fEnabled ? Qt::Checked : Qt::Unchecked);

    /* Compose item tool-tip: */
    const QString strTable("<table cellspacing=5>%1</table>");
    const QString strHeader("<tr><td><nobr>%1:&nbsp;</nobr></td><td><nobr>%2</nobr></td></tr>");
    const QString strSubHeader("<tr><td><nobr>&nbsp;&nbsp;%1:&nbsp;</nobr></td><td><nobr>%2</nobr></td></tr>");
    QString strToolTip;

    /* Interface information: */
    strToolTip += strHeader.arg(UIHostNetworkManager::tr("Adapter"))
                           .arg(m_interface.m_fDHCPEnabled ?
                                UIHostNetworkManager::tr("Automatically configured", "interface") :
                                UIHostNetworkManager::tr("Manually configured", "interface"));
    strToolTip += strSubHeader.arg(UIHostNetworkManager::tr("IPv4 Address"))
                              .arg(m_interface.m_strAddress.isEmpty() ?
                                   UIHostNetworkManager::tr ("Not set", "address") :
                                   m_interface.m_strAddress) +
                  strSubHeader.arg(UIHostNetworkManager::tr("IPv4 Network Mask"))
                              .arg(m_interface.m_strMask.isEmpty() ?
                                   UIHostNetworkManager::tr ("Not set", "mask") :
                                   m_interface.m_strMask);
    if (m_interface.m_fSupportedIPv6)
    {
        strToolTip += strSubHeader.arg(UIHostNetworkManager::tr("IPv6 Address"))
                                  .arg(m_interface.m_strAddress6.isEmpty() ?
                                       UIHostNetworkManager::tr("Not set", "address") :
                                       m_interface.m_strAddress6) +
                      strSubHeader.arg(UIHostNetworkManager::tr("IPv6 Prefix Length"))
                                  .arg(m_interface.m_strPrefixLength6.isEmpty() ?
                                       UIHostNetworkManager::tr("Not set", "length") :
                                       m_interface.m_strPrefixLength6);
    }

    /* DHCP server information: */
    strToolTip += strHeader.arg(UIHostNetworkManager::tr("DHCP Server"))
                           .arg(m_dhcpserver.m_fEnabled ?
                                UIHostNetworkManager::tr("Enabled", "server") :
                                UIHostNetworkManager::tr("Disabled", "server"));
    if (m_dhcpserver.m_fEnabled)
    {
        strToolTip += strSubHeader.arg(UIHostNetworkManager::tr("Address"))
                                  .arg(m_dhcpserver.m_strAddress.isEmpty() ?
                                       UIHostNetworkManager::tr("Not set", "address") :
                                       m_dhcpserver.m_strAddress) +
                      strSubHeader.arg(UIHostNetworkManager::tr("Network Mask"))
                                  .arg(m_dhcpserver.m_strMask.isEmpty() ?
                                       UIHostNetworkManager::tr("Not set", "mask") :
                                       m_dhcpserver.m_strMask) +
                      strSubHeader.arg(UIHostNetworkManager::tr("Lower Bound"))
                                  .arg(m_dhcpserver.m_strLowerAddress.isEmpty() ?
                                       UIHostNetworkManager::tr("Not set", "bound") :
                                       m_dhcpserver.m_strLowerAddress) +
                      strSubHeader.arg(UIHostNetworkManager::tr("Upper Bound"))
                                  .arg(m_dhcpserver.m_strUpperAddress.isEmpty() ?
                                       UIHostNetworkManager::tr("Not set", "bound") :
                                       m_dhcpserver.m_strUpperAddress);
    }

    /* Assign tool-tip finally: */
    setToolTip(Column_Name, strTable.arg(strToolTip));
}

/* static */
int UIItemHostNetwork::maskToCidr(const QString &strMask)
{
    /* Parse passed mask: */
    QList<int> address;
    foreach (const QString &strValue, strMask.split('.'))
        address << strValue.toInt();

    /* Calculate CIDR: */
    int iCidr = 0;
    for (int i = 0; i < 4 || i < address.size(); ++i)
    {
        switch(address.at(i))
        {
            case 0x80: iCidr += 1; break;
            case 0xC0: iCidr += 2; break;
            case 0xE0: iCidr += 3; break;
            case 0xF0: iCidr += 4; break;
            case 0xF8: iCidr += 5; break;
            case 0xFC: iCidr += 6; break;
            case 0xFE: iCidr += 7; break;
            case 0xFF: iCidr += 8; break;
            /* Return CIDR prematurelly: */
            default: return iCidr;
        }
    }

    /* Return CIDR: */
    return iCidr;
}


/*********************************************************************************************************************************
*   Class UIHostNetworkManagerWidget implementation.                                                                             *
*********************************************************************************************************************************/

UIHostNetworkManagerWidget::UIHostNetworkManagerWidget(EmbedTo enmEmbedding, QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_enmEmbedding(enmEmbedding)
    , m_pToolBar(0)
    , m_pMenu(0)
    , m_pActionAdd(0)
    , m_pActionRemove(0)
    , m_pActionDetails(0)
    , m_pActionRefresh(0)
    , m_pTreeWidget(0)
    , m_pDetailsWidget(0)
{
    /* Prepare: */
    prepare();
}

void UIHostNetworkManagerWidget::retranslateUi()
{
    /* Translate menu: */
    if (m_pMenu)
        m_pMenu->setTitle(UIHostNetworkManager::tr("&Network"));

    /* Translate actions: */
    if (m_pActionAdd)
    {
        m_pActionAdd->setText(UIHostNetworkManager::tr("&Create"));
        m_pActionAdd->setToolTip(UIHostNetworkManager::tr("Create Host-only Network (%1)").arg(m_pActionAdd->shortcut().toString()));
        m_pActionAdd->setStatusTip(UIHostNetworkManager::tr("Create new host-only network"));
    }
    if (m_pActionRemove)
    {
        m_pActionRemove->setText(UIHostNetworkManager::tr("&Remove..."));
        m_pActionRemove->setToolTip(UIHostNetworkManager::tr("Remove Host-only Network (%1)").arg(m_pActionRemove->shortcut().toString()));
        m_pActionRemove->setStatusTip(UIHostNetworkManager::tr("Remove selected host-only network"));
    }
    if (m_pActionDetails)
    {
        m_pActionDetails->setText(UIHostNetworkManager::tr("&Properties..."));
        m_pActionDetails->setToolTip(UIHostNetworkManager::tr("Open Host-only Network Properties (%1)").arg(m_pActionDetails->shortcut().toString()));
        m_pActionDetails->setStatusTip(UIHostNetworkManager::tr("Open pane with selected host-only network properties"));
    }
    if (m_pActionRefresh)
    {
        m_pActionRefresh->setText(UIHostNetworkManager::tr("&Refresh..."));
        m_pActionRefresh->setToolTip(UIHostNetworkManager::tr("Refresh Host-only Networks (%1)").arg(m_pActionRefresh->shortcut().toString()));
        m_pActionRefresh->setStatusTip(UIHostNetworkManager::tr("Refresh the list of host-only networks"));
    }

    /* Adjust toolbar: */
#ifdef VBOX_WS_MAC
    // WORKAROUND:
    // There is a bug in Qt Cocoa which result in showing a "more arrow" when
    // the necessary size of the toolbar is increased. Also for some languages
    // the with doesn't match if the text increase. So manually adjust the size
    // after changing the text.
    if (m_pToolBar)
        m_pToolBar->updateLayout();
#endif

    /* Translate tree-widget: */
    const QStringList fields = QStringList()
                               << UIHostNetworkManager::tr("Name")
                               << UIHostNetworkManager::tr("IPv4 Address/Mask")
                               << UIHostNetworkManager::tr("IPv6 Address/Mask")
                               << UIHostNetworkManager::tr("DHCP Server");
    m_pTreeWidget->setHeaderLabels(fields);
}

void UIHostNetworkManagerWidget::resizeEvent(QResizeEvent *pEvent)
{
    /* Call to base-class: */
    QIWithRetranslateUI<QWidget>::resizeEvent(pEvent);

    /* Adjust tree-widget: */
    sltAdjustTreeWidget();
}

void UIHostNetworkManagerWidget::showEvent(QShowEvent *pEvent)
{
    /* Call to base-class: */
    QIWithRetranslateUI<QWidget>::showEvent(pEvent);

    /* Adjust tree-widget: */
    sltAdjustTreeWidget();
}

void UIHostNetworkManagerWidget::sltResetHostNetworkDetailsChanges()
{
    /* Just push the current item data there again: */
    sltHandleCurrentItemChange();
}

void UIHostNetworkManagerWidget::sltApplyHostNetworkDetailsChanges()
{
    /* Get network item: */
    UIItemHostNetwork *pItem = static_cast<UIItemHostNetwork*>(m_pTreeWidget->currentItem());
    AssertMsgReturnVoid(pItem, ("Current item must not be null!\n"));

    /* Get item data: */
    UIDataHostNetwork oldData = *pItem;
    UIDataHostNetwork newData = m_pDetailsWidget->data();

    /* Get host for further activities: */
    CHost comHost = vboxGlobal().host();

    /* Find corresponding interface: */
    CHostNetworkInterface comInterface = comHost.FindHostNetworkInterfaceByName(oldData.m_interface.m_strName);

    /* Show error message if necessary: */
    if (!comHost.isOk() || comInterface.isNull())
        msgCenter().cannotFindHostNetworkInterface(comHost, oldData.m_interface.m_strName, this);
    else
    {
        /* Save automatic interface configuration: */
        if (newData.m_interface.m_fDHCPEnabled)
        {
            if (   comInterface.isOk()
                && !oldData.m_interface.m_fDHCPEnabled)
                comInterface.EnableDynamicIPConfig();
        }
        /* Save manual interface configuration: */
        else
        {
            /* Save IPv4 interface configuration: */
            if (   comInterface.isOk()
                && (   oldData.m_interface.m_fDHCPEnabled
                    || newData.m_interface.m_strAddress != oldData.m_interface.m_strAddress
                    || newData.m_interface.m_strMask != oldData.m_interface.m_strMask))
                comInterface.EnableStaticIPConfig(newData.m_interface.m_strAddress, newData.m_interface.m_strMask);
            /* Save IPv6 interface configuration: */
            if (   comInterface.isOk()
                && newData.m_interface.m_fSupportedIPv6
                && (   oldData.m_interface.m_fDHCPEnabled
                    || newData.m_interface.m_strAddress6 != oldData.m_interface.m_strAddress6
                    || newData.m_interface.m_strPrefixLength6 != oldData.m_interface.m_strPrefixLength6))
                comInterface.EnableStaticIPConfigV6(newData.m_interface.m_strAddress6, newData.m_interface.m_strPrefixLength6.toULong());
        }

        /* Show error message if necessary: */
        if (!comInterface.isOk())
            msgCenter().cannotSaveHostNetworkInterfaceParameter(comInterface, this);
        else
        {
            /* Get network name for further activities: */
            const QString strNetworkName = comInterface.GetNetworkName();

            /* Show error message if necessary: */
            if (!comInterface.isOk())
                msgCenter().cannotAcquireHostNetworkInterfaceParameter(comInterface, this);
            else
            {
                /* Get VBox for further activities: */
                CVirtualBox comVBox = vboxGlobal().virtualBox();

                /* Find corresponding DHCP server (create if necessary): */
                CDHCPServer comServer = comVBox.FindDHCPServerByNetworkName(strNetworkName);
                if (!comVBox.isOk() || comServer.isNull())
                    comServer = comVBox.CreateDHCPServer(strNetworkName);

                /* Show error message if necessary: */
                if (!comVBox.isOk() || comServer.isNull())
                    msgCenter().cannotCreateDHCPServer(comVBox, strNetworkName, this);
                else
                {
                    /* Save whether DHCP server is enabled: */
                    if (   comServer.isOk()
                        && newData.m_dhcpserver.m_fEnabled != oldData.m_dhcpserver.m_fEnabled)
                        comServer.SetEnabled(newData.m_dhcpserver.m_fEnabled);
                    /* Save DHCP server configuration: */
                    if (   comServer.isOk()
                        && newData.m_dhcpserver.m_fEnabled
                        && (   newData.m_dhcpserver.m_strAddress != oldData.m_dhcpserver.m_strAddress
                            || newData.m_dhcpserver.m_strMask != oldData.m_dhcpserver.m_strMask
                            || newData.m_dhcpserver.m_strLowerAddress != oldData.m_dhcpserver.m_strLowerAddress
                            || newData.m_dhcpserver.m_strUpperAddress != oldData.m_dhcpserver.m_strUpperAddress))
                        comServer.SetConfiguration(newData.m_dhcpserver.m_strAddress, newData.m_dhcpserver.m_strMask,
                                                   newData.m_dhcpserver.m_strLowerAddress, newData.m_dhcpserver.m_strUpperAddress);

                    /* Show error message if necessary: */
                    if (!comServer.isOk())
                        msgCenter().cannotSaveDHCPServerParameter(comServer, this);
                }
            }
        }

        /* Find corresponding interface again (if necessary): */
        if (!comInterface.isOk())
        {
            comInterface = comHost.FindHostNetworkInterfaceByName(oldData.m_interface.m_strName);

            /* Show error message if necessary: */
            if (!comHost.isOk() || comInterface.isNull())
                msgCenter().cannotFindHostNetworkInterface(comHost, oldData.m_interface.m_strName, this);
        }

        /* If interface is Ok now: */
        if (comInterface.isNotNull() && comInterface.isOk())
        {
            /* Update interface in the tree: */
            UIDataHostNetwork data;
            loadHostNetwork(comInterface, data);
            updateItemForNetworkHost(data, true, pItem);

            /* Make sure current item fetched: */
            sltHandleCurrentItemChange();

            /* Adjust tree-widget: */
            sltAdjustTreeWidget();
        }
    }
}

void UIHostNetworkManagerWidget::sltAddHostNetwork()
{
    /* Get host for further activities: */
    CHost comHost = vboxGlobal().host();

    /* Create interface: */
    CHostNetworkInterface comInterface;
    CProgress progress = comHost.CreateHostOnlyNetworkInterface(comInterface);

    /* Show error message if necessary: */
    if (!comHost.isOk() || progress.isNull())
        msgCenter().cannotCreateHostNetworkInterface(comHost, this);
    else
    {
        /* Show interface creation progress: */
        msgCenter().showModalProgressDialog(progress, tr("Adding network..."), ":/progress_network_interface_90px.png", this, 0);

        /* Show error message if necessary: */
        if (!progress.isOk() || progress.GetResultCode() != 0)
            msgCenter().cannotCreateHostNetworkInterface(progress, this);
        else
        {
            /* Get network name for further activities: */
            const QString strNetworkName = comInterface.GetNetworkName();

            /* Show error message if necessary: */
            if (!comInterface.isOk())
                msgCenter().cannotAcquireHostNetworkInterfaceParameter(comInterface, this);
            else
            {
                /* Get VBox for further activities: */
                CVirtualBox comVBox = vboxGlobal().virtualBox();

                /* Find corresponding DHCP server (create if necessary): */
                CDHCPServer comServer = comVBox.FindDHCPServerByNetworkName(strNetworkName);
                if (!comVBox.isOk() || comServer.isNull())
                    comServer = comVBox.CreateDHCPServer(strNetworkName);

                /* Show error message if necessary: */
                if (!comVBox.isOk() || comServer.isNull())
                    msgCenter().cannotCreateDHCPServer(comVBox, strNetworkName, this);
            }

            /* Add interface to the tree: */
            UIDataHostNetwork data;
            loadHostNetwork(comInterface, data);
            createItemForNetworkHost(data, true);

            /* Adjust tree-widget: */
            sltAdjustTreeWidget();
        }
    }
}

void UIHostNetworkManagerWidget::sltRemoveHostNetwork()
{
    /* Get network item: */
    UIItemHostNetwork *pItem = static_cast<UIItemHostNetwork*>(m_pTreeWidget->currentItem());
    AssertMsgReturnVoid(pItem, ("Current item must not be null!\n"));

    /* Get interface name: */
    const QString strInterfaceName(pItem->name());

    /* Confirm host network removal: */
    if (!msgCenter().confirmHostOnlyInterfaceRemoval(strInterfaceName, this))
        return;

    /* Get host for further activities: */
    CHost comHost = vboxGlobal().host();

    /* Find corresponding interface: */
    const CHostNetworkInterface &comInterface = comHost.FindHostNetworkInterfaceByName(strInterfaceName);

    /* Show error message if necessary: */
    if (!comHost.isOk() || comInterface.isNull())
        msgCenter().cannotFindHostNetworkInterface(comHost, strInterfaceName, this);
    else
    {
        /* Get network name for further activities: */
        QString strNetworkName;
        if (comInterface.isOk())
            strNetworkName = comInterface.GetNetworkName();
        /* Get interface id for further activities: */
        QString strInterfaceId;
        if (comInterface.isOk())
            strInterfaceId = comInterface.GetId();

        /* Show error message if necessary: */
        if (!comInterface.isOk())
            msgCenter().cannotAcquireHostNetworkInterfaceParameter(comInterface, this);
        else
        {
            /* Get VBox for further activities: */
            CVirtualBox comVBox = vboxGlobal().virtualBox();

            /* Find corresponding DHCP server: */
            const CDHCPServer &comServer = comVBox.FindDHCPServerByNetworkName(strNetworkName);
            if (comVBox.isOk() && comServer.isNotNull())
            {
                /* Remove server if any: */
                comVBox.RemoveDHCPServer(comServer);

                /* Show error message if necessary: */
                if (!comVBox.isOk())
                    msgCenter().cannotRemoveDHCPServer(comVBox, strInterfaceName, this);
            }

            /* Remove interface finally: */
            CProgress progress = comHost.RemoveHostOnlyNetworkInterface(strInterfaceId);

            /* Show error message if necessary: */
            if (!comHost.isOk() || progress.isNull())
                msgCenter().cannotRemoveHostNetworkInterface(comHost, strInterfaceName, this);
            else
            {
                /* Show interface removal progress: */
                msgCenter().showModalProgressDialog(progress, tr("Removing network..."), ":/progress_network_interface_90px.png", this, 0);

                /* Show error message if necessary: */
                if (!progress.isOk() || progress.GetResultCode() != 0)
                    return msgCenter().cannotRemoveHostNetworkInterface(progress, strInterfaceName, this);
                else
                {
                    /* Remove interface from the tree: */
                    delete pItem;

                    /* Adjust tree-widget: */
                    sltAdjustTreeWidget();
                }
            }
        }
    }
}

void UIHostNetworkManagerWidget::sltToggleHostNetworkDetailsVisibility(bool fVisible)
{
    /* Save the setting: */
    gEDataManager->setHostNetworkManagerDetailsExpanded(fVisible);
    /* Show/hide details area and Apply button: */
    m_pDetailsWidget->setVisible(fVisible);
    /* Notify external lsiteners: */
    emit sigHostNetworkDetailsVisibilityChanged(fVisible);
}

void UIHostNetworkManagerWidget::sltRefreshHostNetworks()
{
    // Not implemented.
    AssertMsgFailed(("Not implemented!"));
}

void UIHostNetworkManagerWidget::sltAdjustTreeWidget()
{
    /* Get the tree-widget abstract interface: */
    QAbstractItemView *pItemView = m_pTreeWidget;
    /* Get the tree-widget header-view: */
    QHeaderView *pItemHeader = m_pTreeWidget->header();

    /* Calculate the total tree-widget width: */
    const int iTotal = m_pTreeWidget->viewport()->width();
    /* Look for a minimum width hints for non-important columns: */
    const int iMinWidth1 = qMax(pItemView->sizeHintForColumn(Column_IPv4), pItemHeader->sectionSizeHint(Column_IPv4));
    const int iMinWidth2 = qMax(pItemView->sizeHintForColumn(Column_IPv6), pItemHeader->sectionSizeHint(Column_IPv6));
    const int iMinWidth3 = qMax(pItemView->sizeHintForColumn(Column_DHCP), pItemHeader->sectionSizeHint(Column_DHCP));
    /* Propose suitable width hints for non-important columns: */
    const int iWidth1 = iMinWidth1 < iTotal / Column_Max ? iMinWidth1 : iTotal / Column_Max;
    const int iWidth2 = iMinWidth2 < iTotal / Column_Max ? iMinWidth2 : iTotal / Column_Max;
    const int iWidth3 = iMinWidth3 < iTotal / Column_Max ? iMinWidth3 : iTotal / Column_Max;
    /* Apply the proposal: */
    m_pTreeWidget->setColumnWidth(Column_IPv4, iWidth1);
    m_pTreeWidget->setColumnWidth(Column_IPv6, iWidth2);
    m_pTreeWidget->setColumnWidth(Column_DHCP, iWidth3);
    m_pTreeWidget->setColumnWidth(Column_Name, iTotal - iWidth1 - iWidth2 - iWidth3);
}

void UIHostNetworkManagerWidget::sltHandleItemChange(QTreeWidgetItem *pItem)
{
    /* Get network item: */
    UIItemHostNetwork *pChangedItem = static_cast<UIItemHostNetwork*>(pItem);
    AssertMsgReturnVoid(pChangedItem, ("Changed item must not be null!\n"));

    /* Get item data: */
    UIDataHostNetwork oldData = *pChangedItem;

    /* Make sure dhcp server status changed: */
    if (   (   oldData.m_dhcpserver.m_fEnabled
            && pChangedItem->checkState(Column_DHCP) == Qt::Checked)
        || (   !oldData.m_dhcpserver.m_fEnabled
            && pChangedItem->checkState(Column_DHCP) == Qt::Unchecked))
        return;

    /* Get host for further activities: */
    CHost comHost = vboxGlobal().host();

    /* Find corresponding interface: */
    CHostNetworkInterface comInterface = comHost.FindHostNetworkInterfaceByName(oldData.m_interface.m_strName);

    /* Show error message if necessary: */
    if (!comHost.isOk() || comInterface.isNull())
        msgCenter().cannotFindHostNetworkInterface(comHost, oldData.m_interface.m_strName, this);
    else
    {
        /* Get network name for further activities: */
        const QString strNetworkName = comInterface.GetNetworkName();

        /* Show error message if necessary: */
        if (!comInterface.isOk())
            msgCenter().cannotAcquireHostNetworkInterfaceParameter(comInterface, this);
        else
        {
            /* Get VBox for further activities: */
            CVirtualBox comVBox = vboxGlobal().virtualBox();

            /* Find corresponding DHCP server (create if necessary): */
            CDHCPServer comServer = comVBox.FindDHCPServerByNetworkName(strNetworkName);
            if (!comVBox.isOk() || comServer.isNull())
                comServer = comVBox.CreateDHCPServer(strNetworkName);

            /* Show error message if necessary: */
            if (!comVBox.isOk() || comServer.isNull())
                msgCenter().cannotCreateDHCPServer(comVBox, strNetworkName, this);
            else
            {
                /* Save whether DHCP server is enabled: */
                if (comServer.isOk())
                    comServer.SetEnabled(!oldData.m_dhcpserver.m_fEnabled);
                /* Save default DHCP server configuration if current is invalid: */
                if (   comServer.isOk()
                    && !oldData.m_dhcpserver.m_fEnabled
                    && (   oldData.m_dhcpserver.m_strAddress == "0.0.0.0"
                        || oldData.m_dhcpserver.m_strMask == "0.0.0.0"
                        || oldData.m_dhcpserver.m_strLowerAddress == "0.0.0.0"
                        || oldData.m_dhcpserver.m_strUpperAddress == "0.0.0.0"))
                {
                    const QStringList &proposal = makeDhcpServerProposal(oldData.m_interface.m_strAddress,
                                                                         oldData.m_interface.m_strMask);
                    comServer.SetConfiguration(proposal.at(0), proposal.at(1), proposal.at(2), proposal.at(3));
                }

                /* Show error message if necessary: */
                if (!comServer.isOk())
                    msgCenter().cannotSaveDHCPServerParameter(comServer, this);
                {
                    /* Update interface in the tree: */
                    UIDataHostNetwork data;
                    loadHostNetwork(comInterface, data);
                    updateItemForNetworkHost(data, true, pChangedItem);

                    /* Make sure current item fetched: */
                    sltHandleCurrentItemChange();

                    /* Adjust tree-widget: */
                    sltAdjustTreeWidget();
                }
            }
        }
    }
}

void UIHostNetworkManagerWidget::sltHandleCurrentItemChange()
{
    /* Get network item: */
    UIItemHostNetwork *pItem = static_cast<UIItemHostNetwork*>(m_pTreeWidget->currentItem());

    /* Update actions availability: */
    if (m_pActionRemove)
        m_pActionRemove->setEnabled(pItem);
    if (m_pActionDetails)
        m_pActionDetails->setEnabled(pItem);

    /* If there is an item => update details data: */
    if (pItem)
        m_pDetailsWidget->setData(*pItem);
    else
    {
        /* Otherwise => clear details and close the area: */
        m_pDetailsWidget->setData(UIDataHostNetwork());
        sltToggleHostNetworkDetailsVisibility(false);
    }
}

void UIHostNetworkManagerWidget::sltHandleContextMenuRequest(const QPoint &pos)
{
    /* Compose temporary context-menu: */
    QMenu menu;
    if (m_pTreeWidget->itemAt(pos))
    {
        if (m_pActionRemove)
            menu.addAction(m_pActionRemove);
        if (m_pActionDetails)
            menu.addAction(m_pActionDetails);
    }
    else
    {
        if (m_pActionAdd)
            menu.addAction(m_pActionAdd);
    }
    /* And show it: */
    menu.exec(m_pTreeWidget->mapToGlobal(pos));
}

void UIHostNetworkManagerWidget::prepare()
{
    /* Prepare this: */
    prepareThis();

    /* Load settings: */
    loadSettings();

    /* Apply language settings: */
    retranslateUi();

    /* Load host networks: */
    loadHostNetworks();
}

void UIHostNetworkManagerWidget::prepareThis()
{
    /* Prepare actions: */
    prepareActions();
    /* Prepare widget: */
    prepareWidgets();
}

void UIHostNetworkManagerWidget::prepareActions()
{
    /* Create 'Add' action: */
    m_pActionAdd = new QAction(this);
    AssertPtrReturnVoid(m_pActionAdd);
    {
        /* Configure 'Add' action: */
        m_pActionAdd->setShortcut(QKeySequence("Ctrl+A"));
        m_pActionAdd->setIcon(UIIconPool::iconSetFull(":/add_host_iface_22px.png",
                                                      ":/add_host_iface_16px.png",
                                                      ":/add_host_iface_disabled_22px.png",
                                                      ":/add_host_iface_disabled_16px.png"));
        connect(m_pActionAdd, &QAction::triggered, this, &UIHostNetworkManagerWidget::sltAddHostNetwork);
    }

    /* Create 'Remove' action: */
    m_pActionRemove = new QAction(this);
    AssertPtrReturnVoid(m_pActionRemove);
    {
        /* Configure 'Remove' action: */
        m_pActionRemove->setShortcut(QKeySequence("Del"));
        m_pActionRemove->setIcon(UIIconPool::iconSetFull(":/remove_host_iface_22px.png",
                                                         ":/remove_host_iface_16px.png",
                                                         ":/remove_host_iface_disabled_22px.png",
                                                         ":/remove_host_iface_disabled_16px.png"));
        connect(m_pActionRemove, &QAction::triggered, this, &UIHostNetworkManagerWidget::sltRemoveHostNetwork);
    }

    /* Create 'Details' action: */
    m_pActionDetails = new QAction(this);
    AssertPtrReturnVoid(m_pActionDetails);
    {
        /* Configure 'Details' action: */
        m_pActionDetails->setCheckable(true);
        m_pActionDetails->setShortcut(QKeySequence("Ctrl+Space"));
        m_pActionDetails->setIcon(UIIconPool::iconSetFull(":/edit_host_iface_22px.png",
                                                          ":/edit_host_iface_16px.png",
                                                          ":/edit_host_iface_disabled_22px.png",
                                                          ":/edit_host_iface_disabled_16px.png"));
        connect(m_pActionDetails, &QAction::toggled, this, &UIHostNetworkManagerWidget::sltToggleHostNetworkDetailsVisibility);
    }

    /* Create 'Refresh' action: */
    m_pActionRefresh = new QAction(this);
    AssertPtrReturnVoid(m_pActionRefresh);
    {
        /* Configure 'Details' action: */
        m_pActionRefresh->setCheckable(true);
        m_pActionRefresh->setShortcut(QKeySequence(QKeySequence::Refresh));
        m_pActionRefresh->setIcon(UIIconPool::iconSetFull(":/refresh_22px.png",
                                                          ":/refresh_16px.png",
                                                          ":/refresh_disabled_22px.png",
                                                          ":/refresh_disabled_16px.png"));
        connect(m_pActionRefresh, &QAction::toggled, this, &UIHostNetworkManagerWidget::sltRefreshHostNetworks);
    }

    /* Prepare menu: */
    prepareMenu();
}

void UIHostNetworkManagerWidget::prepareMenu()
{
    /* Create 'Network' menu: */
    m_pMenu = new QMenu(this);
    AssertPtrReturnVoid(m_pMenu);
    {
        /* Configure menu: */
        if (m_pActionAdd)
            m_pMenu->addAction(m_pActionAdd);
        if (m_pActionRemove)
            m_pMenu->addAction(m_pActionRemove);
        if (m_pActionDetails)
            m_pMenu->addAction(m_pActionDetails);
//        if (m_pActionRefresh)
//            m_pMenu->addAction(m_pActionRefresh);
    }
}

void UIHostNetworkManagerWidget::prepareWidgets()
{
    /* Create main-layout: */
    new QVBoxLayout(this);
    AssertPtrReturnVoid(layout());
    {
        /* Configure layout: */
        layout()->setContentsMargins(0, 0, 0, 0);
#ifdef VBOX_WS_MAC
        layout()->setSpacing(10);
#else
        layout()->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing) / 2);
#endif

        /* Prepare toolbar: */
        prepareToolBar();
        /* Prepare tree-widget: */
        prepareTreeWidget();
        /* Prepare details-widget: */
        prepareDetailsWidget();
    }
}

void UIHostNetworkManagerWidget::prepareToolBar()
{
    /* Create toolbar: */
    m_pToolBar = new UIToolBar(parentWidget());
    AssertPtrReturnVoid(m_pToolBar);
    {
        /* Configure toolbar: */
        const int iIconMetric = (int)(QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) * 1.375);
        m_pToolBar->setIconSize(QSize(iIconMetric, iIconMetric));
        m_pToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        /* Add toolbar actions: */
        if (m_pActionAdd)
            m_pToolBar->addAction(m_pActionAdd);
        if (m_pActionRemove)
            m_pToolBar->addAction(m_pActionRemove);
        if ((m_pActionAdd || m_pActionRemove) && m_pActionDetails)
            m_pToolBar->addSeparator();
        if (m_pActionDetails)
            m_pToolBar->addAction(m_pActionDetails);
//        if (m_pActionDetails && m_pActionRefresh)
//            m_pToolBar->addSeparator();
//        if (m_pActionRefresh)
//            m_pToolBar->addAction(m_pActionRefresh);
#ifdef VBOX_WS_MAC
        /* Check whether we are embedded into a stack: */
        if (m_enmEmbedding == EmbedTo_Stack)
        {
            /* Add into layout: */
            layout()->addWidget(m_pToolBar);
        }
#else
        /* Add into layout: */
        layout()->addWidget(m_pToolBar);
#endif
    }
}

void UIHostNetworkManagerWidget::prepareTreeWidget()
{
    /* Create tree-widget: */
    m_pTreeWidget = new QITreeWidget;
    AssertPtrReturnVoid(m_pTreeWidget);
    {
        /* Configure tree-widget: */
        m_pTreeWidget->setRootIsDecorated(false);
        m_pTreeWidget->setAlternatingRowColors(true);
        m_pTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        m_pTreeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_pTreeWidget->setColumnCount(Column_Max);
        m_pTreeWidget->setSortingEnabled(true);
        m_pTreeWidget->sortByColumn(Column_Name, Qt::AscendingOrder);
        m_pTreeWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
        connect(m_pTreeWidget, &QITreeWidget::currentItemChanged,
                this, &UIHostNetworkManagerWidget::sltHandleCurrentItemChange);
        connect(m_pTreeWidget, &QITreeWidget::customContextMenuRequested,
                this, &UIHostNetworkManagerWidget::sltHandleContextMenuRequest);
        connect(m_pTreeWidget, &QITreeWidget::itemChanged,
                this, &UIHostNetworkManagerWidget::sltHandleItemChange);
        if (m_pActionDetails)
            connect(m_pTreeWidget, &QITreeWidget::itemDoubleClicked,
                    m_pActionDetails, &QAction::setChecked);

        /* Add into layout: */
        layout()->addWidget(m_pTreeWidget);
    }
}

void UIHostNetworkManagerWidget::prepareDetailsWidget()
{
    /* Create details-widget: */
    m_pDetailsWidget = new UIHostNetworkDetailsWidget(m_enmEmbedding);
    AssertPtrReturnVoid(m_pDetailsWidget);
    {
        /* Configure details-widget: */
        m_pDetailsWidget->setVisible(false);
        m_pDetailsWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        connect(m_pDetailsWidget, &UIHostNetworkDetailsWidget::sigDataChanged,
                this, &UIHostNetworkManagerWidget::sigHostNetworkDetailsDataChanged);
        connect(m_pDetailsWidget, &UIHostNetworkDetailsWidget::sigDataChangeRejected,
                this, &UIHostNetworkManagerWidget::sltResetHostNetworkDetailsChanges);
        connect(m_pDetailsWidget, &UIHostNetworkDetailsWidget::sigDataChangeAccepted,
                this, &UIHostNetworkManagerWidget::sltApplyHostNetworkDetailsChanges);

        /* Add into layout: */
        layout()->addWidget(m_pDetailsWidget);
    }
}

void UIHostNetworkManagerWidget::loadSettings()
{
    /* Details action/widget: */
    m_pActionDetails->setChecked(gEDataManager->hostNetworkManagerDetailsExpanded());
}

void UIHostNetworkManagerWidget::loadHostNetworks()
{
    /* Clear tree first of all: */
    m_pTreeWidget->clear();

    /* Get host for further activities: */
    const CHost comHost = vboxGlobal().host();

    /* Get interfaces for further activities: */
    const QVector<CHostNetworkInterface> interfaces = comHost.GetNetworkInterfaces();

    /* Show error message if necessary: */
    if (!comHost.isOk())
        msgCenter().cannotAcquireHostNetworkInterfaces(comHost, this);
    else
    {
        /* For each host-only interface => load it to the tree: */
        foreach (const CHostNetworkInterface &comInterface, interfaces)
            if (comInterface.GetInterfaceType() == KHostNetworkInterfaceType_HostOnly)
            {
                UIDataHostNetwork data;
                loadHostNetwork(comInterface, data);
                createItemForNetworkHost(data, false);
            }

        /* Choose the 1st item as current initially: */
        m_pTreeWidget->setCurrentItem(m_pTreeWidget->topLevelItem(0));
        sltHandleCurrentItemChange();

        /* Adjust tree-widget: */
        sltAdjustTreeWidget();
    }
}

void UIHostNetworkManagerWidget::loadHostNetwork(const CHostNetworkInterface &comInterface, UIDataHostNetwork &data)
{
    /* Gather interface settings: */
    if (comInterface.isOk())
        data.m_interface.m_strName = comInterface.GetName();
    if (comInterface.isOk())
        data.m_interface.m_fDHCPEnabled = comInterface.GetDHCPEnabled();
    if (comInterface.isOk())
        data.m_interface.m_strAddress = comInterface.GetIPAddress();
    if (comInterface.isOk())
        data.m_interface.m_strMask = comInterface.GetNetworkMask();
    if (comInterface.isOk())
        data.m_interface.m_fSupportedIPv6 = comInterface.GetIPV6Supported();
    if (comInterface.isOk())
        data.m_interface.m_strAddress6 = comInterface.GetIPV6Address();
    if (comInterface.isOk())
        data.m_interface.m_strPrefixLength6 = QString::number(comInterface.GetIPV6NetworkMaskPrefixLength());

    /* Get host interface network name for further activities: */
    QString strNetworkName;
    if (comInterface.isOk())
        strNetworkName = comInterface.GetNetworkName();

    /* Show error message if necessary: */
    if (!comInterface.isOk())
        msgCenter().cannotAcquireHostNetworkInterfaceParameter(comInterface, this);

    /* Get VBox for further activities: */
    CVirtualBox comVBox = vboxGlobal().virtualBox();

    /* Find corresponding DHCP server (create if necessary): */
    CDHCPServer comServer = comVBox.FindDHCPServerByNetworkName(strNetworkName);
    if (!comVBox.isOk() || comServer.isNull())
        comServer = comVBox.CreateDHCPServer(strNetworkName);

    /* Show error message if necessary: */
    if (!comVBox.isOk() || comServer.isNull())
        msgCenter().cannotCreateDHCPServer(comVBox, strNetworkName, this);
    else
    {
        /* Gather DHCP server settings: */
        if (comServer.isOk())
            data.m_dhcpserver.m_fEnabled = comServer.GetEnabled();
        if (comServer.isOk())
            data.m_dhcpserver.m_strAddress = comServer.GetIPAddress();
        if (comServer.isOk())
            data.m_dhcpserver.m_strMask = comServer.GetNetworkMask();
        if (comServer.isOk())
            data.m_dhcpserver.m_strLowerAddress = comServer.GetLowerIP();
        if (comServer.isOk())
            data.m_dhcpserver.m_strUpperAddress = comServer.GetUpperIP();

        /* Show error message if necessary: */
        if (!comServer.isOk())
            return msgCenter().cannotAcquireDHCPServerParameter(comServer, this);
    }
}

void UIHostNetworkManagerWidget::createItemForNetworkHost(const UIDataHostNetwork &data, bool fChooseItem)
{
    /* Create new item: */
    UIItemHostNetwork *pItem = new UIItemHostNetwork;
    AssertPtrReturnVoid(pItem);
    {
        /* Configure item: */
        pItem->UIDataHostNetwork::operator=(data);
        pItem->updateFields();
        /* Add item to the tree: */
        m_pTreeWidget->addTopLevelItem(pItem);
        /* And choose it as current if necessary: */
        if (fChooseItem)
            m_pTreeWidget->setCurrentItem(pItem);
    }
}

void UIHostNetworkManagerWidget::updateItemForNetworkHost(const UIDataHostNetwork &data, bool fChooseItem, UIItemHostNetwork *pItem)
{
    /* Update passed item: */
    AssertPtrReturnVoid(pItem);
    {
        /* Configure item: */
        pItem->UIDataHostNetwork::operator=(data);
        pItem->updateFields();
        /* And choose it as current if necessary: */
        if (fChooseItem)
            m_pTreeWidget->setCurrentItem(pItem);
    }
}


/*********************************************************************************************************************************
*   Class UIHostNetworkManagerFactory implementation.                                                                            *
*********************************************************************************************************************************/

void UIHostNetworkManagerFactory::create(QIManagerDialog *&pDialog, QWidget *pCenterWidget)
{
    pDialog = new UIHostNetworkManager(pCenterWidget);
}


/*********************************************************************************************************************************
*   Class UIHostNetworkManager implementation.                                                                                   *
*********************************************************************************************************************************/

UIHostNetworkManager::UIHostNetworkManager(QWidget *pCenterWidget)
    : QIWithRetranslateUI<QIManagerDialog>(pCenterWidget)
{
}

void UIHostNetworkManager::sltHandleButtonBoxClick(QAbstractButton *pButton)
{
    /* Disable buttons first of all: */
    button(ButtonType_Reset)->setEnabled(false);
    button(ButtonType_Apply)->setEnabled(false);

    /* Compare with known buttons: */
    if (pButton == button(ButtonType_Reset))
        emit sigDataChangeRejected();
    else
    if (pButton == button(ButtonType_Apply))
        emit sigDataChangeAccepted();
}

void UIHostNetworkManager::retranslateUi()
{
    /* Translate window title: */
    setWindowTitle(tr("Host Network Manager"));

    /* Translate buttons: */
    button(ButtonType_Reset)->setText(tr("Reset"));
    button(ButtonType_Apply)->setText(tr("Apply"));
    button(ButtonType_Close)->setText(tr("Close"));
    button(ButtonType_Reset)->setStatusTip(tr("Reset changes in current host network details"));
    button(ButtonType_Apply)->setStatusTip(tr("Apply changes in current host network details"));
    button(ButtonType_Close)->setStatusTip(tr("Close dialog without saving"));
    button(ButtonType_Reset)->setShortcut(QString("Ctrl+Backspace"));
    button(ButtonType_Apply)->setShortcut(QString("Ctrl+Return"));
    button(ButtonType_Close)->setShortcut(Qt::Key_Escape);
    button(ButtonType_Reset)->setToolTip(tr("Reset Changes (%1)").arg(button(ButtonType_Reset)->shortcut().toString()));
    button(ButtonType_Apply)->setToolTip(tr("Apply Changes (%1)").arg(button(ButtonType_Apply)->shortcut().toString()));
    button(ButtonType_Close)->setToolTip(tr("Close Window (%1)").arg(button(ButtonType_Close)->shortcut().toString()));
}

void UIHostNetworkManager::configure()
{
    /* Apply window icons: */
    setWindowIcon(UIIconPool::iconSetFull(":/host_iface_manager_32px.png", ":/host_iface_manager_16px.png"));
}

void UIHostNetworkManager::configureCentralWidget()
{
    /* Create widget: */
    UIHostNetworkManagerWidget *pWidget = new UIHostNetworkManagerWidget(EmbedTo_Dialog, this);
    AssertPtrReturnVoid(pWidget);
    {
        /* Configure widget: */
        setWidget(pWidget);
        setWidgetMenu(pWidget->menu());
#ifdef VBOX_WS_MAC
        setWidgetToolbar(pWidget->toolbar());
#endif
        connect(this, &UIHostNetworkManager::sigDataChangeRejected,
                pWidget, &UIHostNetworkManagerWidget::sltResetHostNetworkDetailsChanges);
        connect(this, &UIHostNetworkManager::sigDataChangeAccepted,
                pWidget, &UIHostNetworkManagerWidget::sltApplyHostNetworkDetailsChanges);

        /* Add into layout: */
        centralWidget()->layout()->addWidget(pWidget);
    }
}

void UIHostNetworkManager::configureButtonBox()
{
    /* Configure button-box: */
    connect(widget(), &UIHostNetworkManagerWidget::sigHostNetworkDetailsVisibilityChanged,
            button(ButtonType_Apply), &QPushButton::setVisible);
    connect(widget(), &UIHostNetworkManagerWidget::sigHostNetworkDetailsVisibilityChanged,
            button(ButtonType_Reset), &QPushButton::setVisible);
    connect(widget(), &UIHostNetworkManagerWidget::sigHostNetworkDetailsDataChanged,
            button(ButtonType_Apply), &QPushButton::setEnabled);
    connect(widget(), &UIHostNetworkManagerWidget::sigHostNetworkDetailsDataChanged,
            button(ButtonType_Reset), &QPushButton::setEnabled);
    connect(buttonBox(), &QIDialogButtonBox::clicked,
            this, &UIHostNetworkManager::sltHandleButtonBoxClick);
    // WORKAROUND:
    // Since we connected signals later than extra-data loaded
    // for signals above, we should handle that stuff here again:
    button(ButtonType_Apply)->setVisible(gEDataManager->hostNetworkManagerDetailsExpanded());
    button(ButtonType_Reset)->setVisible(gEDataManager->hostNetworkManagerDetailsExpanded());
}

void UIHostNetworkManager::finalize()
{
    /* Apply language settings: */
    retranslateUi();
}

UIHostNetworkManagerWidget *UIHostNetworkManager::widget()
{
    return qobject_cast<UIHostNetworkManagerWidget*>(QIManagerDialog::widget());
}

