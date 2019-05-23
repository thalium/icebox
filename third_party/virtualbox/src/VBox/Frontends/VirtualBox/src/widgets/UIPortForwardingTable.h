/* $Id: UIPortForwardingTable.h $ */
/** @file
 * VBox Qt GUI - UIPortForwardingTable class declaration.
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

#ifndef __UIPortForwardingTable_h__
#define __UIPortForwardingTable_h__

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class QITableView;
class UIToolBar;
class QIDialogButtonBox;
class UIPortForwardingModel;

/* Name data: */
class NameData : public QString
{
public:

    NameData() : QString() {}
    NameData(const QString &strName) : QString(strName) {}
};
Q_DECLARE_METATYPE(NameData);

/* Ip data: */
class IpData : public QString
{
public:

    IpData() : QString() {}
    IpData(const QString &strIP) : QString(strIP) {}
};
Q_DECLARE_METATYPE(IpData);

/* Port data: */
class PortData
{
public:

    PortData() : m_uValue(0) {}
    PortData(ushort uValue) : m_uValue(uValue) {}
    PortData(const PortData &other) : m_uValue(other.value()) {}
    bool operator==(const PortData &other) const { return m_uValue == other.m_uValue; }
    ushort value() const { return m_uValue; }

private:

    ushort m_uValue;
};
Q_DECLARE_METATYPE(PortData);

/** Port Forwarding Rule data structure. */
struct UIDataPortForwardingRule
{
    /** Constructs data. */
    UIDataPortForwardingRule()
        : name(QString())
        , protocol(KNATProtocol_UDP)
        , hostIp(IpData())
        , hostPort(PortData())
        , guestIp(IpData())
        , guestPort(PortData())
    {}

    /** Constructs data on the basis of passed arguments.
      * @param  strName      Brings the rule name.
      * @param  enmProtocol  Brings the rule protocol.
      * @param  strHostIP    Brings the rule host IP.
      * @param  uHostPort    Brings the rule host port.
      * @param  strGuestIP   Brings the rule guest IP.
      * @param  uGuestPort   Brings the rule guest port. */
    UIDataPortForwardingRule(const NameData &strName,
                             KNATProtocol enmProtocol,
                             const IpData &strHostIP,
                             PortData uHostPort,
                             const IpData &strGuestIP,
                             PortData uGuestPort)
        : name(strName)
        , protocol(enmProtocol)
        , hostIp(strHostIP)
        , hostPort(uHostPort)
        , guestIp(strGuestIP)
        , guestPort(uGuestPort)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataPortForwardingRule &other) const
    {
        return true
               && (name == other.name)
               && (protocol == other.protocol)
               && (hostIp == other.hostIp)
               && (hostPort == other.hostPort)
               && (guestIp == other.guestIp)
               && (guestPort == other.guestPort)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataPortForwardingRule &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataPortForwardingRule &other) const { return !equal(other); }

    /** Holds the rule name. */
    NameData name;
    /** Holds the rule protocol. */
    KNATProtocol protocol;
    /** Holds the rule host IP. */
    IpData hostIp;
    /** Holds the rule host port. */
    PortData hostPort;
    /** Holds the rule guest IP. */
    IpData guestIp;
    /** Holds the rule guest port. */
    PortData guestPort;
};

/* Port forwarding data, unique part: */
struct UIPortForwardingDataUnique
{
    UIPortForwardingDataUnique(KNATProtocol enmProtocol,
                               PortData uHostPort,
                               const IpData &strHostIp)
        : protocol(enmProtocol)
        , hostPort(uHostPort)
        , hostIp(strHostIp) {}
    bool operator==(const UIPortForwardingDataUnique &other)
    {
        return    protocol == other.protocol
               && hostPort == other.hostPort
               && (   hostIp.isEmpty()    || other.hostIp.isEmpty()
                   || hostIp == "0.0.0.0" || other.hostIp == "0.0.0.0"
                   || hostIp              == other.hostIp);
    }
    KNATProtocol protocol;
    PortData hostPort;
    IpData hostIp;
};

/* Port forwarding data list: */
typedef QList<UIDataPortForwardingRule> UIPortForwardingDataList;

/* Port forwarding dialog: */
class UIPortForwardingTable : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIPortForwardingTable(const UIPortForwardingDataList &rules, bool fIPv6, bool fAllowEmptyGuestIPs);

    /* API: Rules stuff: */
    const UIPortForwardingDataList rules() const;
    bool validate() const;

    /** Returns whether the table data was changed. */
    bool isChanged() const { return m_fIsTableDataChanged; }

    /** Makes sure current editor data committed. */
    void makeSureEditorDataCommitted();

private slots:

    /* Handlers: Table operation stuff: */
    void sltAddRule();
    void sltCopyRule();
    void sltDelRule();

    /** Marks table data as changed. */
    void sltTableDataChanged() { m_fIsTableDataChanged = true; }

    /* Handlers: Table stuff: */
    void sltCurrentChanged();
    void sltShowTableContexMenu(const QPoint &position);
    void sltAdjustTable();

private:

    /* Handler: Translation stuff: */
    void retranslateUi();

    /* Handlers: Event-processing stuff: */
    bool eventFilter(QObject *pObject, QEvent *pEvent);

    /* Flags: */
    bool m_fAllowEmptyGuestIPs;

    /** Holds whether the table data was changed. */
    bool m_fIsTableDataChanged;

    /* Widgets: */
    QITableView *m_pTableView;
    UIToolBar *m_pToolBar;

    /* Model: */
    UIPortForwardingModel *m_pModel;

    /* Actions: */
    QAction *m_pAddAction;
    QAction *m_pCopyAction;
    QAction *m_pDelAction;
};

#endif // __UIPortForwardingTable_h__
