/* $Id: UIPortForwardingTable.cpp $ */
/** @file
 * VBox Qt GUI - UIPortForwardingTable class implementation.
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
# include <QHBoxLayout>
# include <QMenu>
# include <QAction>
# include <QHeaderView>
# include <QStyledItemDelegate>
# include <QItemEditorFactory>
# include <QComboBox>
# include <QLineEdit>
# include <QSpinBox>

/* GUI includes: */
# include "UIDesktopWidgetWatchdog.h"
# include "UIPortForwardingTable.h"
# include "UIMessageCenter.h"
# include "UIConverter.h"
# include "UIIconPool.h"
# include "UIToolBar.h"
# include "QITableView.h"

/* Other VBox includes: */
# include <iprt/cidr.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* External includes: */
#include <math.h>


#if 0 /* Decided to not use it for now. */
/* IPv4 validator: */
class IPv4Validator : public QValidator
{
    Q_OBJECT;

public:

    /* Constructor/destructor: */
    IPv4Validator(QObject *pParent) : QValidator(pParent) {}
    ~IPv4Validator() {}

    /* Handler: Validation stuff: */
    QValidator::State validate(QString &strInput, int& /*iPos*/) const
    {
        QString strStringToValidate(strInput);
        strStringToValidate.remove(' ');
        QString strDot("\\.");
        QString strDigits("(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]?|0)");
        QRegExp intRegExp(QString("^(%1?(%2(%1?(%2(%1?(%2%1?)?)?)?)?)?)?$").arg(strDigits).arg(strDot));
        RTNETADDRIPV4 Network, Mask;
        if (strStringToValidate == "..." || RTCidrStrToIPv4(strStringToValidate.toLatin1().constData(), &Network, &Mask) == VINF_SUCCESS)
            return QValidator::Acceptable;
        else if (intRegExp.indexIn(strStringToValidate) != -1)
            return QValidator::Intermediate;
        else
            return QValidator::Invalid;
    }
};

/* IPv6 validator: */
class IPv6Validator : public QValidator
{
    Q_OBJECT;

public:

    /* Constructor/destructor: */
    IPv6Validator(QObject *pParent) : QValidator(pParent) {}
    ~IPv6Validator() {}

    /* Handler: Validation stuff: */
    QValidator::State validate(QString &strInput, int& /*iPos*/) const
    {
        QString strStringToValidate(strInput);
        strStringToValidate.remove(' ');
        QString strDigits("([0-9a-fA-F]{0,4})");
        QRegExp intRegExp(QString("^%1(:%1(:%1(:%1(:%1(:%1(:%1(:%1)?)?)?)?)?)?)?$").arg(strDigits));
        if (intRegExp.indexIn(strStringToValidate) != -1)
            return QValidator::Acceptable;
        else
            return QValidator::Invalid;
    }
};
#endif /* Decided to not use it for now. */


/** Port Forwarding data types. */
enum UIPortForwardingDataType
{
    UIPortForwardingDataType_Name,
    UIPortForwardingDataType_Protocol,
    UIPortForwardingDataType_HostIp,
    UIPortForwardingDataType_HostPort,
    UIPortForwardingDataType_GuestIp,
    UIPortForwardingDataType_GuestPort,
    UIPortForwardingDataType_Max
};


/* Name editor: */
class NameEditor : public QLineEdit
{
    Q_OBJECT;
    Q_PROPERTY(NameData name READ name WRITE setName USER true);

public:

    /* Constructor: */
    NameEditor(QWidget *pParent = 0) : QLineEdit(pParent)
    {
        setFrame(false);
        setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        setValidator(new QRegExpValidator(QRegExp("[^,:]*"), this));
    }

private:

    /* API: Name stuff: */
    void setName(NameData name)
    {
        setText(name);
    }

    /* API: Name stuff: */
    NameData name() const
    {
        return text();
    }
};


/* Protocol editor: */
class ProtocolEditor : public QComboBox
{
    Q_OBJECT;
    Q_PROPERTY(KNATProtocol protocol READ protocol WRITE setProtocol USER true);

public:

    /* Constructor: */
    ProtocolEditor(QWidget *pParent = 0) : QComboBox(pParent)
    {
        addItem(gpConverter->toString(KNATProtocol_UDP), QVariant::fromValue(KNATProtocol_UDP));
        addItem(gpConverter->toString(KNATProtocol_TCP), QVariant::fromValue(KNATProtocol_TCP));
    }

private:

    /* API: Protocol stuff: */
    void setProtocol(KNATProtocol p)
    {
        for (int i = 0; i < count(); ++i)
        {
            if (itemData(i).value<KNATProtocol>() == p)
            {
                setCurrentIndex(i);
                break;
            }
        }
    }

    /* API: Protocol stuff: */
    KNATProtocol protocol() const
    {
        return itemData(currentIndex()).value<KNATProtocol>();
    }
};


/* IPv4 editor: */
class IPv4Editor : public QLineEdit
{
    Q_OBJECT;
    Q_PROPERTY(IpData ip READ ip WRITE setIp USER true);

public:

    /* Constructor: */
    IPv4Editor(QWidget *pParent = 0) : QLineEdit(pParent)
    {
        setFrame(false);
        setAlignment(Qt::AlignCenter);
        // Decided to not use it for now:
        // setValidator(new IPv4Validator(this));
    }

private:

    /* API: IP stuff: */
    void setIp(IpData ip)
    {
        setText(ip);
    }

    /* API: IP stuff: */
    IpData ip() const
    {
        return text() == "..." ? QString() : text();
    }
};


/* IPv6 editor: */
class IPv6Editor : public QLineEdit
{
    Q_OBJECT;
    Q_PROPERTY(IpData ip READ ip WRITE setIp USER true);

public:

    /* Constructor: */
    IPv6Editor(QWidget *pParent = 0) : QLineEdit(pParent)
    {
        setFrame(false);
        setAlignment(Qt::AlignCenter);
        // Decided to not use it for now:
        // setValidator(new IPv6Validator(this));
    }

private:

    /* API: IP stuff: */
    void setIp(IpData ip)
    {
        setText(ip);
    }

    /* API: IP stuff: */
    IpData ip() const
    {
        return text() == "..." ? QString() : text();
    }
};


/* Port editor: */
class PortEditor : public QSpinBox
{
    Q_OBJECT;
    Q_PROPERTY(PortData port READ port WRITE setPort USER true);

public:

    /* Constructor: */
    PortEditor(QWidget *pParent = 0) : QSpinBox(pParent)
    {
        setFrame(false);
        setRange(0, (1 << (8 * sizeof(ushort))) - 1);
    }

private:

    /* API: Port stuff: */
    void setPort(PortData port)
    {
        setValue(port.value());
    }

    /* API: Port stuff: */
    PortData port() const
    {
        return value();
    }
};


/** QITableViewCell extension used as Port Forwarding table-view cell. */
class UIPortForwardingCell : public QITableViewCell
{
    Q_OBJECT;

public:

    /** Constructs table cell passing @a pParent to the base-class.
      * @param  strName  Brings the name. */
    UIPortForwardingCell(QITableViewRow *pParent, const NameData &strName)
        : QITableViewCell(pParent)
        , m_strText(strName)
    {}

    /** Constructs table cell passing @a pParent to the base-class.
      * @param  enmProtocol  Brings the protocol type. */
    UIPortForwardingCell(QITableViewRow *pParent, KNATProtocol enmProtocol)
        : QITableViewCell(pParent)
        , m_strText(gpConverter->toString(enmProtocol))
    {}

    /** Constructs table cell passing @a pParent to the base-class.
      * @param  strIp  Brings the IP address. */
    UIPortForwardingCell(QITableViewRow *pParent, const IpData &strIP)
        : QITableViewCell(pParent)
        , m_strText(strIP)
    {}

    /** Constructs table cell passing @a pParent to the base-class.
      * @param  uHostPort  Brings the port. */
    UIPortForwardingCell(QITableViewRow *pParent, PortData uPort)
        : QITableViewCell(pParent)
        , m_strText(QString::number(uPort.value()))
    {}

    /** Returns the cell text. */
    virtual QString text() const /* override */ { return m_strText; }

private:

    /** Holds the cell text. */
    QString m_strText;
};


/** QITableViewRow extension used as Port Forwarding table-view row. */
class UIPortForwardingRow : public QITableViewRow
{
    Q_OBJECT;

public:

    /** Constructs table row passing @a pParent to the base-class.
      * @param  strName      Brings the unique rule name.
      * @param  enmProtocol  Brings the rule protocol type.
      * @param  strHostIp    Brings the rule host IP address.
      * @param  uHostPort    Brings the rule host port.
      * @param  strGuestIp   Brings the rule guest IP address.
      * @param  uGuestPort   Brings the rule guest port. */
    UIPortForwardingRow(QITableView *pParent,
                        const NameData &strName, KNATProtocol enmProtocol,
                        const IpData &strHostIp, PortData uHostPort,
                        const IpData &strGuestIp, PortData uGuestPort)
        : QITableViewRow(pParent)
        , m_strName(strName), m_enmProtocol(enmProtocol)
        , m_strHostIp(strHostIp), m_uHostPort(uHostPort)
        , m_strGuestIp(strGuestIp), m_uGuestPort(uGuestPort)
    {
        /* Create cells: */
        createCells();
    }

    /** Destructs table row. */
    ~UIPortForwardingRow()
    {
        /* Destroy cells: */
        destroyCells();
    }

    /** Returns the unique rule name. */
    NameData name() const { return m_strName; }
    /** Defines the unique rule name. */
    void setName(const NameData &strName)
    {
        m_strName = strName;
        delete m_cells[UIPortForwardingDataType_Name];
        m_cells[UIPortForwardingDataType_Name] = new UIPortForwardingCell(this, m_strName);
    }

    /** Returns the rule protocol type. */
    KNATProtocol protocol() const { return m_enmProtocol; }
    /** Defines the rule protocol type. */
    void setProtocol(KNATProtocol enmProtocol)
    {
        m_enmProtocol = enmProtocol;
        delete m_cells[UIPortForwardingDataType_Protocol];
        m_cells[UIPortForwardingDataType_Protocol] = new UIPortForwardingCell(this, m_enmProtocol);
    }

    /** Returns the rule host IP address. */
    IpData hostIp() const { return m_strHostIp; }
    /** Defines the rule host IP address. */
    void setHostIp(const IpData &strHostIp)
    {
        m_strHostIp = strHostIp;
        delete m_cells[UIPortForwardingDataType_HostIp];
        m_cells[UIPortForwardingDataType_HostIp] = new UIPortForwardingCell(this, m_strHostIp);
    }

    /** Returns the rule host port. */
    PortData hostPort() const { return m_uHostPort; }
    /** Defines the rule host port. */
    void setHostPort(PortData uHostPort)
    {
        m_uHostPort = uHostPort;
        delete m_cells[UIPortForwardingDataType_HostPort];
        m_cells[UIPortForwardingDataType_HostPort] = new UIPortForwardingCell(this, m_uHostPort);
    }

    /** Returns the rule guest IP address. */
    IpData guestIp() const { return m_strGuestIp; }
    /** Defines the rule guest IP address. */
    void setGuestIp(const IpData &strGuestIp)
    {
        m_strGuestIp = strGuestIp;
        delete m_cells[UIPortForwardingDataType_GuestIp];
        m_cells[UIPortForwardingDataType_GuestIp] = new UIPortForwardingCell(this, m_strGuestIp);
    }

    /** Returns the rule guest port. */
    PortData guestPort() const { return m_uGuestPort; }
    /** Defines the rule guest port. */
    void setGuestPort(PortData uGuestPort)
    {
        m_uGuestPort = uGuestPort;
        delete m_cells[UIPortForwardingDataType_GuestPort];
        m_cells[UIPortForwardingDataType_GuestPort] = new UIPortForwardingCell(this, m_uGuestPort);
    }

protected:

    /** Returns the number of children. */
    virtual int childCount() const /* override */
    {
        /* Return cell count: */
        return UIPortForwardingDataType_Max;
    }

    /** Returns the child item with @a iIndex. */
    virtual QITableViewCell *childItem(int iIndex) const /* override */
    {
        /* Make sure index within the bounds: */
        AssertReturn(iIndex >= 0 && iIndex < m_cells.size(), 0);
        /* Return corresponding cell: */
        return m_cells[iIndex];
    }

private:

    /** Creates cells. */
    void createCells()
    {
        /* Create cells on the basis of variables we have: */
        m_cells.resize(UIPortForwardingDataType_Max);
        m_cells[UIPortForwardingDataType_Name] = new UIPortForwardingCell(this, m_strName);
        m_cells[UIPortForwardingDataType_Protocol] = new UIPortForwardingCell(this, m_enmProtocol);
        m_cells[UIPortForwardingDataType_HostIp] = new UIPortForwardingCell(this, m_strHostIp);
        m_cells[UIPortForwardingDataType_HostPort] = new UIPortForwardingCell(this, m_uHostPort);
        m_cells[UIPortForwardingDataType_GuestIp] = new UIPortForwardingCell(this, m_strGuestIp);
        m_cells[UIPortForwardingDataType_GuestPort] = new UIPortForwardingCell(this, m_uGuestPort);
    }

    /** Destroys cells. */
    void destroyCells()
    {
        /* Destroy cells: */
        qDeleteAll(m_cells);
        m_cells.clear();
    }

    /** Holds the unique rule name. */
    NameData m_strName;
    /** Holds the rule protocol type. */
    KNATProtocol m_enmProtocol;
    /** Holds the rule host IP address. */
    IpData m_strHostIp;
    /** Holds the rule host port. */
    PortData m_uHostPort;
    /** Holds the rule guest IP address. */
    IpData m_strGuestIp;
    /** Holds the rule guest port. */
    PortData m_uGuestPort;

    /** Holds the cell instances. */
    QVector<UIPortForwardingCell*> m_cells;
};


/* Port forwarding data model: */
class UIPortForwardingModel : public QAbstractTableModel
{
    Q_OBJECT;

public:

    /** Constructs Port Forwarding model passing @a pParent to the base-class.
      * @param  rules  Brings the list of port forwarding rules to load initially. */
    UIPortForwardingModel(QITableView *pParent, const UIPortForwardingDataList &rules = UIPortForwardingDataList());
    /** Destructs Port Forwarding model. */
    ~UIPortForwardingModel();

    /** Returns the number of children. */
    int childCount() const;
    /** Returns the child item with @a iIndex. */
    QITableViewRow *childItem(int iIndex) const;

    /* API: Rule stuff: */
    const UIPortForwardingDataList rules() const;
    void addRule(const QModelIndex &index);
    void delRule(const QModelIndex &index);

    /* API: Index flag stuff: */
    Qt::ItemFlags flags(const QModelIndex &index) const;

    /* API: Index row-count stuff: */
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    /* API: Index column-count stuff: */
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    /* API: Header data stuff: */
    QVariant headerData(int iSection, Qt::Orientation orientation, int iRole) const;

    /* API: Index data stuff: */
    QVariant data(const QModelIndex &index, int iRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int iRole = Qt::EditRole);

private:

    /** Return the parent table-view reference. */
    QITableView *parentTable() const;

    /* Variable: Data stuff: */
    QList<UIPortForwardingRow*> m_dataList;
};


/** QITableView extension used as Port Forwarding table-view. */
class UIPortForwardingView : public QITableView
{
    Q_OBJECT;

public:

    /** Constructs Port Forwarding table-view. */
    UIPortForwardingView() {}

protected:

    /** Returns the number of children. */
    virtual int childCount() const /* override */;
    /** Returns the child item with @a iIndex. */
    virtual QITableViewRow *childItem(int iIndex) const /* override */;
};


/*********************************************************************************************************************************
*   Class UIPortForwardingModel implementation.                                                                                  *
*********************************************************************************************************************************/

UIPortForwardingModel::UIPortForwardingModel(QITableView *pParent, const UIPortForwardingDataList &rules /* = UIPortForwardingDataList() */)
    : QAbstractTableModel(pParent)
{
    /* Fetch the incoming data: */
    foreach (const UIDataPortForwardingRule &rule, rules)
        m_dataList << new UIPortForwardingRow(pParent,
                                              rule.name, rule.protocol,
                                              rule.hostIp, rule.hostPort,
                                              rule.guestIp, rule.guestPort);
}

UIPortForwardingModel::~UIPortForwardingModel()
{
    /* Delete the cached data: */
    qDeleteAll(m_dataList);
    m_dataList.clear();
}

int UIPortForwardingModel::childCount() const
{
    /* Return row count: */
    return rowCount();
}

QITableViewRow *UIPortForwardingModel::childItem(int iIndex) const
{
    /* Make sure index within the bounds: */
    AssertReturn(iIndex >= 0 && iIndex < m_dataList.size(), 0);
    /* Return corresponding row: */
    return m_dataList[iIndex];
}

const UIPortForwardingDataList UIPortForwardingModel::rules() const
{
    /* Return the cached data: */
    UIPortForwardingDataList data;
    foreach (const UIPortForwardingRow *pRow, m_dataList)
        data << UIDataPortForwardingRule(pRow->name(), pRow->protocol(),
                                         pRow->hostIp(), pRow->hostPort(),
                                         pRow->guestIp(), pRow->guestPort());
    return data;
}

void UIPortForwardingModel::addRule(const QModelIndex &index)
{
    beginInsertRows(QModelIndex(), m_dataList.size(), m_dataList.size());
    /* Search for existing "Rule [NUMBER]" record: */
    uint uMaxIndex = 0;
    QString strTemplate("Rule %1");
    QRegExp regExp(strTemplate.arg("(\\d+)"));
    for (int i = 0; i < m_dataList.size(); ++i)
        if (regExp.indexIn(m_dataList[i]->name()) > -1)
            uMaxIndex = regExp.cap(1).toUInt() > uMaxIndex ? regExp.cap(1).toUInt() : uMaxIndex;
    /* If index is valid => copy data: */
    if (index.isValid())
        m_dataList << new UIPortForwardingRow(parentTable(),
                                              strTemplate.arg(++uMaxIndex), m_dataList[index.row()]->protocol(),
                                              m_dataList[index.row()]->hostIp(), m_dataList[index.row()]->hostPort(),
                                              m_dataList[index.row()]->guestIp(), m_dataList[index.row()]->guestPort());
    /* If index is NOT valid => use default values: */
    else
        m_dataList << new UIPortForwardingRow(parentTable(),
                                              strTemplate.arg(++uMaxIndex), KNATProtocol_TCP,
                                              QString(""), 0, QString(""), 0);
    endInsertRows();
}

void UIPortForwardingModel::delRule(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    delete m_dataList.at(index.row());
    m_dataList.removeAt(index.row());
    endRemoveRows();
}

Qt::ItemFlags UIPortForwardingModel::flags(const QModelIndex &index) const
{
    /* Check index validness: */
    if (!index.isValid())
        return Qt::NoItemFlags;
    /* All columns have similar flags: */
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

int UIPortForwardingModel::rowCount(const QModelIndex&) const
{
    return m_dataList.size();
}

int UIPortForwardingModel::columnCount(const QModelIndex&) const
{
    return UIPortForwardingDataType_Max;
}

QVariant UIPortForwardingModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
    /* Display role for horizontal header: */
    if (iRole == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        /* Switch for different columns: */
        switch (iSection)
        {
            case UIPortForwardingDataType_Name: return UIPortForwardingTable::tr("Name");
            case UIPortForwardingDataType_Protocol: return UIPortForwardingTable::tr("Protocol");
            case UIPortForwardingDataType_HostIp: return UIPortForwardingTable::tr("Host IP");
            case UIPortForwardingDataType_HostPort: return UIPortForwardingTable::tr("Host Port");
            case UIPortForwardingDataType_GuestIp: return UIPortForwardingTable::tr("Guest IP");
            case UIPortForwardingDataType_GuestPort: return UIPortForwardingTable::tr("Guest Port");
            default: break;
        }
    }
    /* Return wrong value: */
    return QVariant();
}

QVariant UIPortForwardingModel::data(const QModelIndex &index, int iRole) const
{
    /* Check index validness: */
    if (!index.isValid())
        return QVariant();
    /* Switch for different roles: */
    switch (iRole)
    {
        /* Display role: */
        case Qt::DisplayRole:
        {
            /* Switch for different columns: */
            switch (index.column())
            {
                case UIPortForwardingDataType_Name: return m_dataList[index.row()]->name();
                case UIPortForwardingDataType_Protocol: return gpConverter->toString(m_dataList[index.row()]->protocol());
                case UIPortForwardingDataType_HostIp: return m_dataList[index.row()]->hostIp();
                case UIPortForwardingDataType_HostPort: return m_dataList[index.row()]->hostPort().value();
                case UIPortForwardingDataType_GuestIp: return m_dataList[index.row()]->guestIp();
                case UIPortForwardingDataType_GuestPort: return m_dataList[index.row()]->guestPort().value();
                default: return QVariant();
            }
        }
        /* Edit role: */
        case Qt::EditRole:
        {
            /* Switch for different columns: */
            switch (index.column())
            {
                case UIPortForwardingDataType_Name: return QVariant::fromValue(m_dataList[index.row()]->name());
                case UIPortForwardingDataType_Protocol: return QVariant::fromValue(m_dataList[index.row()]->protocol());
                case UIPortForwardingDataType_HostIp: return QVariant::fromValue(m_dataList[index.row()]->hostIp());
                case UIPortForwardingDataType_HostPort: return QVariant::fromValue(m_dataList[index.row()]->hostPort());
                case UIPortForwardingDataType_GuestIp: return QVariant::fromValue(m_dataList[index.row()]->guestIp());
                case UIPortForwardingDataType_GuestPort: return QVariant::fromValue(m_dataList[index.row()]->guestPort());
                default: return QVariant();
            }
        }
        /* Alignment role: */
        case Qt::TextAlignmentRole:
        {
            /* Switch for different columns: */
            switch (index.column())
            {
                case UIPortForwardingDataType_Name:
                case UIPortForwardingDataType_Protocol:
                case UIPortForwardingDataType_HostPort:
                case UIPortForwardingDataType_GuestPort:
                    return (int)(Qt::AlignLeft | Qt::AlignVCenter);
                case UIPortForwardingDataType_HostIp:
                case UIPortForwardingDataType_GuestIp:
                    return Qt::AlignCenter;
                default: return QVariant();
            }
        }
        case Qt::SizeHintRole:
        {
            /* Switch for different columns: */
            switch (index.column())
            {
                case UIPortForwardingDataType_HostIp:
                case UIPortForwardingDataType_GuestIp:
                    return QSize(QApplication::fontMetrics().width(" 888.888.888.888 "), QApplication::fontMetrics().height());
                default: return QVariant();
            }
        }
        default: break;
    }
    /* Return wrong value: */
    return QVariant();
}

bool UIPortForwardingModel::setData(const QModelIndex &index, const QVariant &value, int iRole /* = Qt::EditRole */)
{
    /* Check index validness: */
    if (!index.isValid() || iRole != Qt::EditRole)
        return false;
    /* Switch for different columns: */
    switch (index.column())
    {
        case UIPortForwardingDataType_Name:
            m_dataList[index.row()]->setName(value.value<NameData>());
            emit dataChanged(index, index);
            return true;
        case UIPortForwardingDataType_Protocol:
            m_dataList[index.row()]->setProtocol(value.value<KNATProtocol>());
            emit dataChanged(index, index);
            return true;
        case UIPortForwardingDataType_HostIp:
            m_dataList[index.row()]->setHostIp(value.value<IpData>());
            emit dataChanged(index, index);
            return true;
        case UIPortForwardingDataType_HostPort:
            m_dataList[index.row()]->setHostPort(value.value<PortData>());
            emit dataChanged(index, index);
            return true;
        case UIPortForwardingDataType_GuestIp:
            m_dataList[index.row()]->setGuestIp(value.value<IpData>());
            emit dataChanged(index, index);
            return true;
        case UIPortForwardingDataType_GuestPort:
            m_dataList[index.row()]->setGuestPort(value.value<PortData>());
            emit dataChanged(index, index);
            return true;
        default: return false;
    }
    /* not reached! */
}

QITableView *UIPortForwardingModel::parentTable() const
{
    return qobject_cast<QITableView*>(parent());
}


/*********************************************************************************************************************************
*   Class UIPortForwardingView implementation.                                                                                   *
*********************************************************************************************************************************/

int UIPortForwardingView::childCount() const
{
    /* Redirect request to table model: */
    return qobject_cast<UIPortForwardingModel*>(model())->childCount();
}

QITableViewRow *UIPortForwardingView::childItem(int iIndex) const
{
    /* Redirect request to table model: */
    return qobject_cast<UIPortForwardingModel*>(model())->childItem(iIndex);
}


/*********************************************************************************************************************************
*   Class UIPortForwardingTable implementation.                                                                                  *
*********************************************************************************************************************************/

UIPortForwardingTable::UIPortForwardingTable(const UIPortForwardingDataList &rules, bool fIPv6, bool fAllowEmptyGuestIPs)
    : m_fAllowEmptyGuestIPs(fAllowEmptyGuestIPs)
    , m_fIsTableDataChanged(false)
    , m_pTableView(0)
    , m_pToolBar(0)
    , m_pModel(0)
    , m_pAddAction(0)
    , m_pCopyAction(0)
    , m_pDelAction(0)
{
    /* Create layout: */
    QHBoxLayout *pMainLayout = new QHBoxLayout(this);
    {
        /* Configure layout: */
#ifdef VBOX_WS_MAC
        /* On macOS we can do a bit of smoothness: */
        pMainLayout->setContentsMargins(0, 0, 0, 0);
        pMainLayout->setSpacing(3);
#else
        pMainLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) / 3);
#endif
        /* Create table: */
        m_pTableView = new UIPortForwardingView;
        {
            /* Configure table: */
            m_pTableView->setTabKeyNavigation(false);
            m_pTableView->verticalHeader()->hide();
            m_pTableView->verticalHeader()->setDefaultSectionSize((int)(m_pTableView->verticalHeader()->minimumSectionSize() * 1.33));
            m_pTableView->setSelectionMode(QAbstractItemView::SingleSelection);
            m_pTableView->setContextMenuPolicy(Qt::CustomContextMenu);
            m_pTableView->installEventFilter(this);
        }
        /* Create model: */
        m_pModel = new UIPortForwardingModel(m_pTableView, rules);
        {
            /* Configure model: */
            connect(m_pModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(sltTableDataChanged()));
            connect(m_pModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(sltTableDataChanged()));
            connect(m_pModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), this, SLOT(sltTableDataChanged()));
            /* Configure table (after model is configured): */
            m_pTableView->setModel(m_pModel);
            connect(m_pTableView, SIGNAL(sigCurrentChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(sltCurrentChanged()));
            connect(m_pTableView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(sltShowTableContexMenu(const QPoint &)));
        }
        /* Create toolbar: */
        m_pToolBar = new UIToolBar;
        {
            /* Determine icon metric: */
            const QStyle *pStyle = QApplication::style();
            const int iIconMetric = pStyle->pixelMetric(QStyle::PM_SmallIconSize);
            /* Configure toolbar: */
            m_pToolBar->setIconSize(QSize(iIconMetric, iIconMetric));
            m_pToolBar->setOrientation(Qt::Vertical);
            /* Create 'add' action: */
            m_pAddAction = new QAction(this);
            {
                /* Configure 'add' action: */
                m_pAddAction->setShortcut(QKeySequence("Ins"));
                m_pAddAction->setIcon(UIIconPool::iconSet(":/controller_add_16px.png", ":/controller_add_disabled_16px.png"));
                connect(m_pAddAction, SIGNAL(triggered(bool)), this, SLOT(sltAddRule()));
                m_pToolBar->addAction(m_pAddAction);
            }
            /* Create 'copy' action: */
            m_pCopyAction = new QAction(this);
            {
                /* Configure 'add' action: */
                m_pCopyAction->setIcon(UIIconPool::iconSet(":/controller_add_16px.png", ":/controller_add_disabled_16px.png"));
                connect(m_pCopyAction, SIGNAL(triggered(bool)), this, SLOT(sltCopyRule()));
            }
            /* Create 'del' action: */
            m_pDelAction = new QAction(this);
            {
                /* Configure 'del' action: */
                m_pDelAction->setShortcut(QKeySequence("Del"));
                m_pDelAction->setIcon(UIIconPool::iconSet(":/controller_remove_16px.png", ":/controller_remove_disabled_16px.png"));
                connect(m_pDelAction, SIGNAL(triggered(bool)), this, SLOT(sltDelRule()));
                m_pToolBar->addAction(m_pDelAction);
            }
        }
        /* Add widgets into layout: */
        pMainLayout->addWidget(m_pTableView);
        pMainLayout->addWidget(m_pToolBar);
    }

    /* We do have abstract item delegate: */
    QAbstractItemDelegate *pAbstractItemDelegate = m_pTableView->itemDelegate();
    if (pAbstractItemDelegate)
    {
        /* But do we have styled item delegate? */
        QStyledItemDelegate *pStyledItemDelegate = qobject_cast<QStyledItemDelegate*>(pAbstractItemDelegate);
        if (pStyledItemDelegate)
        {
            /* Create new item editor factory: */
            QItemEditorFactory *pNewItemEditorFactory = new QItemEditorFactory;
            {
                /* Register NameEditor as the NameData editor: */
                int iNameId = qRegisterMetaType<NameData>();
                QStandardItemEditorCreator<NameEditor> *pNameEditorItemCreator = new QStandardItemEditorCreator<NameEditor>();
                pNewItemEditorFactory->registerEditor((QVariant::Type)iNameId, pNameEditorItemCreator);

                /* Register ProtocolEditor as the KNATProtocol editor: */
                int iProtocolId = qRegisterMetaType<KNATProtocol>();
                QStandardItemEditorCreator<ProtocolEditor> *pProtocolEditorItemCreator = new QStandardItemEditorCreator<ProtocolEditor>();
                pNewItemEditorFactory->registerEditor((QVariant::Type)iProtocolId, pProtocolEditorItemCreator);

                /* Register IPv4Editor/IPv6Editor as the IpData editor: */
                int iIpId = qRegisterMetaType<IpData>();
                if (!fIPv6)
                {
                    QStandardItemEditorCreator<IPv4Editor> *pIPv4EditorItemCreator = new QStandardItemEditorCreator<IPv4Editor>();
                    pNewItemEditorFactory->registerEditor((QVariant::Type)iIpId, pIPv4EditorItemCreator);
                }
                else
                {
                    QStandardItemEditorCreator<IPv6Editor> *pIPv6EditorItemCreator = new QStandardItemEditorCreator<IPv6Editor>();
                    pNewItemEditorFactory->registerEditor((QVariant::Type)iIpId, pIPv6EditorItemCreator);
                }

                /* Register PortEditor as the PortData editor: */
                int iPortId = qRegisterMetaType<PortData>();
                QStandardItemEditorCreator<PortEditor> *pPortEditorItemCreator = new QStandardItemEditorCreator<PortEditor>();
                pNewItemEditorFactory->registerEditor((QVariant::Type)iPortId, pPortEditorItemCreator);

                /* Set newly created item editor factory for table delegate: */
                pStyledItemDelegate->setItemEditorFactory(pNewItemEditorFactory);
            }
        }
    }

    /* Retranslate dialog: */
    retranslateUi();

    /* Limit the minimum size to 33% of screen size: */
    setMinimumSize(gpDesktop->screenGeometry(this).size() / 3);
}

const UIPortForwardingDataList UIPortForwardingTable::rules() const
{
    return m_pModel->rules();
}

bool UIPortForwardingTable::validate() const
{
    /* Validate table: */
    QList<NameData> names;
    QList<UIPortForwardingDataUnique> rules;
    for (int i = 0; i < m_pModel->rowCount(); ++i)
    {
        /* Some of variables: */
        const NameData name = m_pModel->data(m_pModel->index(i, UIPortForwardingDataType_Name), Qt::EditRole).value<NameData>();
        const KNATProtocol protocol = m_pModel->data(m_pModel->index(i, UIPortForwardingDataType_Protocol), Qt::EditRole).value<KNATProtocol>();
        const PortData hostPort = m_pModel->data(m_pModel->index(i, UIPortForwardingDataType_HostPort), Qt::EditRole).value<PortData>().value();
        const PortData guestPort = m_pModel->data(m_pModel->index(i, UIPortForwardingDataType_GuestPort), Qt::EditRole).value<PortData>().value();
        const IpData hostIp = m_pModel->data(m_pModel->index(i, UIPortForwardingDataType_HostIp), Qt::EditRole).value<IpData>();
        const IpData guestIp = m_pModel->data(m_pModel->index(i, UIPortForwardingDataType_GuestIp), Qt::EditRole).value<IpData>();

        /* If at least one port is 'zero': */
        if (hostPort.value() == 0 || guestPort.value() == 0)
            return msgCenter().warnAboutIncorrectPort(window());
        /* If at least one address is incorrect: */
        if (!(   hostIp.trimmed().isEmpty()
              || RTNetIsIPv4AddrStr(hostIp.toUtf8().constData())
              || RTNetIsIPv6AddrStr(hostIp.toUtf8().constData())
              || RTNetStrIsIPv4AddrAny(hostIp.toUtf8().constData())
              || RTNetStrIsIPv6AddrAny(hostIp.toUtf8().constData())))
            return msgCenter().warnAboutIncorrectAddress(window());
        if (!(   guestIp.trimmed().isEmpty()
              || RTNetIsIPv4AddrStr(guestIp.toUtf8().constData())
              || RTNetIsIPv6AddrStr(guestIp.toUtf8().constData())
              || RTNetStrIsIPv4AddrAny(guestIp.toUtf8().constData())
              || RTNetStrIsIPv6AddrAny(guestIp.toUtf8().constData())))
            return msgCenter().warnAboutIncorrectAddress(window());
        /* If empty guest address is not allowed: */
        if (   !m_fAllowEmptyGuestIPs
            && guestIp.isEmpty())
            return msgCenter().warnAboutEmptyGuestAddress(window());

        /* Make sure non of the names were previosly used: */
        if (!names.contains(name))
            names << name;
        else
            return msgCenter().warnAboutNameShouldBeUnique(window());

        /* Make sure non of the rules were previosly used: */
        UIPortForwardingDataUnique rule(protocol, hostPort, hostIp);
        if (!rules.contains(rule))
            rules << rule;
        else
            return msgCenter().warnAboutRulesConflict(window());
    }
    /* True by default: */
    return true;
}

void UIPortForwardingTable::makeSureEditorDataCommitted()
{
    m_pTableView->makeSureEditorDataCommitted();
}

void UIPortForwardingTable::sltAddRule()
{
    m_pModel->addRule(QModelIndex());
    m_pTableView->setFocus();
    m_pTableView->setCurrentIndex(m_pModel->index(m_pModel->rowCount() - 1, 0));
    sltCurrentChanged();
    sltAdjustTable();
}

void UIPortForwardingTable::sltCopyRule()
{
    m_pModel->addRule(m_pTableView->currentIndex());
    m_pTableView->setFocus();
    m_pTableView->setCurrentIndex(m_pModel->index(m_pModel->rowCount() - 1, 0));
    sltCurrentChanged();
    sltAdjustTable();
}

void UIPortForwardingTable::sltDelRule()
{
    m_pModel->delRule(m_pTableView->currentIndex());
    m_pTableView->setFocus();
    sltCurrentChanged();
    sltAdjustTable();
}

void UIPortForwardingTable::sltCurrentChanged()
{
    bool fTableFocused = m_pTableView->hasFocus();
    bool fTableChildFocused = m_pTableView->findChildren<QWidget*>().contains(QApplication::focusWidget());
    bool fTableOrChildFocused = fTableFocused || fTableChildFocused;
    m_pCopyAction->setEnabled(m_pTableView->currentIndex().isValid() && fTableOrChildFocused);
    m_pDelAction->setEnabled(m_pTableView->currentIndex().isValid() && fTableOrChildFocused);
}

void UIPortForwardingTable::sltShowTableContexMenu(const QPoint &pos)
{
    /* Prepare context menu: */
    QMenu menu(m_pTableView);
    /* If some index is currently selected: */
    if (m_pTableView->indexAt(pos).isValid())
    {
        menu.addAction(m_pCopyAction);
        menu.addAction(m_pDelAction);
    }
    /* If no valid index selected: */
    else
    {
        menu.addAction(m_pAddAction);
    }
    menu.exec(m_pTableView->viewport()->mapToGlobal(pos));
}

void UIPortForwardingTable::sltAdjustTable()
{
    m_pTableView->horizontalHeader()->setStretchLastSection(false);
    /* If table is NOT empty: */
    if (m_pModel->rowCount())
    {
        /* Resize table to contents size-hint and emit a spare place for first column: */
        m_pTableView->resizeColumnsToContents();
        uint uFullWidth = m_pTableView->viewport()->width();
        for (uint u = 1; u < UIPortForwardingDataType_Max; ++u)
            uFullWidth -= m_pTableView->horizontalHeader()->sectionSize(u);
        m_pTableView->horizontalHeader()->resizeSection(UIPortForwardingDataType_Name, uFullWidth);
    }
    /* If table is empty: */
    else
    {
        /* Resize table columns to be equal in size: */
        uint uFullWidth = m_pTableView->viewport()->width();
        for (uint u = 0; u < UIPortForwardingDataType_Max; ++u)
            m_pTableView->horizontalHeader()->resizeSection(u, uFullWidth / UIPortForwardingDataType_Max);
    }
    m_pTableView->horizontalHeader()->setStretchLastSection(true);
}

void UIPortForwardingTable::retranslateUi()
{
    /* Table translations: */
    m_pTableView->setWhatsThis(tr("Contains a list of port forwarding rules."));

    /* Set action's text: */
    m_pAddAction->setText(tr("Add New Rule"));
    m_pCopyAction->setText(tr("Copy Selected Rule"));
    m_pDelAction->setText(tr("Remove Selected Rule"));

    m_pAddAction->setWhatsThis(tr("Adds new port forwarding rule."));
    m_pCopyAction->setWhatsThis(tr("Copies selected port forwarding rule."));
    m_pDelAction->setWhatsThis(tr("Removes selected port forwarding rule."));

    m_pAddAction->setToolTip(m_pAddAction->whatsThis());
    m_pCopyAction->setToolTip(m_pCopyAction->whatsThis());
    m_pDelAction->setToolTip(m_pDelAction->whatsThis());
}

bool UIPortForwardingTable::eventFilter(QObject *pObject, QEvent *pEvent)
{
    /* Process table: */
    if (pObject == m_pTableView)
    {
        /* Process different event-types: */
        switch (pEvent->type())
        {
            case QEvent::Show:
            case QEvent::Resize:
            {
                /* Adjust table: */
                sltAdjustTable();
                break;
            }
            case QEvent::FocusIn:
            case QEvent::FocusOut:
            {
                /* Update actions: */
                sltCurrentChanged();
                break;
            }
            default:
                break;
        }
    }
    /* Call to base-class: */
    return QIWithRetranslateUI<QWidget>::eventFilter(pObject, pEvent);
}

#include "UIPortForwardingTable.moc"

