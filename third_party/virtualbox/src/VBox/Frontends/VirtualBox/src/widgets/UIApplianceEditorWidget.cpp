/* $Id: UIApplianceEditorWidget.cpp $ */
/** @file
 * VBox Qt GUI - UIApplianceEditorWidget class implementation.
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
# include <QComboBox>
# include <QDir>
# include <QCheckBox>
# include <QHeaderView>
# include <QLabel>
# include <QLineEdit>
# include <QSpinBox>
# include <QTextEdit>

/* GUI includes: */
# include "QITreeView.h"
# include "VBoxGlobal.h"
# include "VBoxOSTypeSelectorButton.h"
# include "UIApplianceEditorWidget.h"
# include "UIConverter.h"
# include "UIIconPool.h"
# include "UILineTextEdit.h"
# include "UIMessageCenter.h"

/* COM includes: */
# include "CSystemProperties.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Describes the interface of Appliance item.
  * Represented as a tree structure with a parent & multiple children. */
class UIApplianceModelItem : public QITreeViewItem
{
public:

    /** Constructs root item with specified @a iNumber, @a enmType and @a pParent. */
    UIApplianceModelItem(int iNumber, ApplianceModelItemType enmType, QITreeView *pParent);
    /** Constructs non-root item with specified @a iNumber, @a enmType and @a pParentItem. */
    UIApplianceModelItem(int iNumber, ApplianceModelItemType enmType, UIApplianceModelItem *pParentItem);
    /** Destructs item. */
    virtual ~UIApplianceModelItem();

    /** Returns the item type. */
    ApplianceModelItemType type() const { return m_enmType; }

    /** Returns the parent of the item. */
    UIApplianceModelItem *parent() const { return m_pParentItem; }

    /** Appends the passed @a pChildItem to the item's list of children. */
    void appendChild(UIApplianceModelItem *pChildItem);
    /** Returns the child specified by the @a iIndex. */
    virtual UIApplianceModelItem *childItem(int iIndex) const /* override */;

    /** Returns the row of the item in the parent. */
    int row() const;

    /** Returns the number of children. */
    virtual int childCount() const /* override */;
    /** Returns the number of columns. */
    int columnCount() const { return 3; }

    /** Returns the item text. */
    virtual QString text() const /* override */;

    /** Returns the item flags for the given @a iColumn. */
    virtual Qt::ItemFlags itemFlags(int /* iColumn */) const { return 0; }

    /** Defines the @a iRole data for the item at @a iColumn to @a value. */
    virtual bool setData(int /* iColumn */, const QVariant & /* value */, int /* iRole */) { return false; }
    /** Returns the data stored under the given @a iRole for the item referred to by the @a iColumn. */
    virtual QVariant data(int /* iColumn */, int /* iRole */) const { return QVariant(); }

    /** Returns the widget used to edit the item specified by @a idx for editing.
      * @param  pParent      Brings the parent to be assigned for newly created editor.
      * @param  styleOption  Bring the style option set for the newly created editor. */
    virtual QWidget *createEditor(QWidget * /* pParent */, const QStyleOptionViewItem & /* styleOption */, const QModelIndex & /* idx */) const { return 0; }

    /** Defines the contents of the given @a pEditor to the data for the item at the given @a idx. */
    virtual bool setEditorData(QWidget * /* pEditor */, const QModelIndex & /* idx */) const { return false; }
    /** Defines the data for the item at the given @a idx in the @a pModel to the contents of the given @a pEditor. */
    virtual bool setModelData(QWidget * /* pEditor */, QAbstractItemModel * /* pModel */, const QModelIndex & /* idx */) { return false; }

    /** Restores the default values. */
    virtual void restoreDefaults() {}

    /** Cache currently stored values, such as @a finalStates, @a finalValues and @a finalExtraValues. */
    virtual void putBack(QVector<BOOL> &finalStates, QVector<QString> &finalValues, QVector<QString> &finalExtraValues);

protected:

    /** Holds the item number. */
    int                     m_iNumber;
    /** Holds the item type. */
    ApplianceModelItemType  m_enmType;

    /** Holds the parent item reference. */
    UIApplianceModelItem         *m_pParentItem;
    /** Holds the list of children item instances. */
    QList<UIApplianceModelItem*>  m_childItems;
};


/** UIApplianceModelItem subclass representing Appliance Virtual System item. */
class UIVirtualSystemItem : public UIApplianceModelItem
{
public:

    /** Constructs item passing @a iNumber and @a pParentItem to the base-class.
      * @param  comDescription  Brings the Virtual System Description. */
    UIVirtualSystemItem(int iNumber, CVirtualSystemDescription comDescription, UIApplianceModelItem *pParentItem);

    /** Returns the data stored under the given @a iRole for the item referred to by the @a iColumn. */
    virtual QVariant data(int iColumn, int iRole) const /* override */;

    /** Cache currently stored values, such as @a finalStates, @a finalValues and @a finalExtraValues. */
    virtual void putBack(QVector<BOOL> &finalStates, QVector<QString> &finalValues, QVector<QString> &finalExtraValues) /* override */;

private:

    /** Holds the Virtual System Description. */
    CVirtualSystemDescription  m_comDescription;
};


/** UIApplianceModelItem subclass representing Appliance Virtual Hardware item. */
class UIVirtualHardwareItem : public UIApplianceModelItem
{
    friend class UIApplianceSortProxyModel;

    /** Data roles. */
    enum
    {
        TypeRole = Qt::UserRole,
        ModifiedRole
    };

public:

    /** Constructs item passing @a iNumber and @a pParentItem to the base-class.
      * @param  enmVSDType           Brings the Virtual System Description type.
      * @param  strRef               Brings something totally useless.
      * @param  strOrigValue         Brings the original value.
      * @param  strConfigValue       Brings the configuration value.
      * @param  strExtraConfigValue  Brings the extra configuration value. */
    UIVirtualHardwareItem(int iNumber,
                          KVirtualSystemDescriptionType enmVSDType,
                          const QString &strRef,
                          const QString &strOrigValue,
                          const QString &strConfigValue,
                          const QString &strExtraConfigValue,
                          UIApplianceModelItem *pParentItem);

    /** Returns the item flags for the given @a iColumn. */
    virtual Qt::ItemFlags itemFlags(int iColumn) const /* override */;

    /** Defines the @a iRole data for the item at @a iColumn to @a value. */
    virtual bool setData(int iColumn, const QVariant &value, int iRole) /* override */;
    /** Returns the data stored under the given @a iRole for the item referred to by the @a iColumn. */
    virtual QVariant data(int iColumn, int iRole) const /* override */;

    /** Returns the widget used to edit the item specified by @a idx for editing.
      * @param  pParent      Brings the parent to be assigned for newly created editor.
      * @param  styleOption  Bring the style option set for the newly created editor. */
    virtual QWidget *createEditor(QWidget *pParent, const QStyleOptionViewItem &styleOption, const QModelIndex &idx) const /* override */;

    /** Defines the contents of the given @a pEditor to the data for the item at the given @a idx. */
    virtual bool setEditorData(QWidget *pEditor, const QModelIndex &idx) const /* override */;
    /** Defines the data for the item at the given @a idx in the @a pModel to the contents of the given @a pEditor. */
    virtual bool setModelData(QWidget *pEditor, QAbstractItemModel *pModel, const QModelIndex &idx) /* override */;

    /** Restores the default values. */
    virtual void restoreDefaults() /* override */;

    /** Cache currently stored values, such as @a finalStates, @a finalValues and @a finalExtraValues. */
    virtual void putBack(QVector<BOOL> &finalStates, QVector<QString> &finalValues, QVector<QString> &finalExtraValues) /* override */;

private:

    /** Holds the Virtual System Description type. */
    KVirtualSystemDescriptionType  m_enmVSDType;
    /** Holds something totally useless. */
    QString                        m_strRef;
    /** Holds the original value. */
    QString                        m_strOrigValue;
    /** Holds the configuration value. */
    QString                        m_strConfigValue;
    /** Holds the default configuration value. */
    QString                        m_strConfigDefaultValue;
    /** Holds the extra configuration value. */
    QString                        m_strExtraConfigValue;
    /** Holds the item check state. */
    Qt::CheckState                 m_checkState;
    /** Holds whether item was modified. */
    bool                           m_fModified;
};


/*********************************************************************************************************************************
*   Class UIApplianceModelItem implementation.                                                                                   *
*********************************************************************************************************************************/

UIApplianceModelItem::UIApplianceModelItem(int iNumber, ApplianceModelItemType enmType, QITreeView *pParent)
    : QITreeViewItem(pParent)
    , m_iNumber(iNumber)
    , m_enmType(enmType)
    , m_pParentItem(0)
{
}

UIApplianceModelItem::UIApplianceModelItem(int iNumber, ApplianceModelItemType enmType, UIApplianceModelItem *pParentItem)
    : QITreeViewItem(pParentItem)
    , m_iNumber(iNumber)
    , m_enmType(enmType)
    , m_pParentItem(pParentItem)
{
}

UIApplianceModelItem::~UIApplianceModelItem()
{
    qDeleteAll(m_childItems);
}

void UIApplianceModelItem::appendChild(UIApplianceModelItem *pChildItem)
{
    AssertPtr(pChildItem);
    m_childItems << pChildItem;
}

UIApplianceModelItem *UIApplianceModelItem::childItem(int iIndex) const
{
    return m_childItems.value(iIndex);
}

int UIApplianceModelItem::row() const
{
    if (m_pParentItem)
        return m_pParentItem->m_childItems.indexOf(const_cast<UIApplianceModelItem*>(this));

    return 0;
}

int UIApplianceModelItem::childCount() const
{
    return m_childItems.count();
}

QString UIApplianceModelItem::text() const
{
    switch (type())
    {
        case ApplianceModelItemType_VirtualSystem:
            return tr("%1", "col.1 text")
                     .arg(data(ApplianceViewSection_Description, Qt::DisplayRole).toString());
        case ApplianceModelItemType_VirtualHardware:
            return tr("%1: %2", "col.1 text: col.2 text")
                     .arg(data(ApplianceViewSection_Description, Qt::DisplayRole).toString())
                     .arg(data(ApplianceViewSection_ConfigValue, Qt::DisplayRole).toString());
        default:
            break;
    }
    return QString();
}

void UIApplianceModelItem::putBack(QVector<BOOL> &finalStates, QVector<QString> &finalValues, QVector<QString> &finalExtraValues)
{
    for (int i = 0; i < childCount(); ++i)
        childItem(i)->putBack(finalStates, finalValues, finalExtraValues);
}


/*********************************************************************************************************************************
*   Class UIVirtualSystemItem implementation.                                                                                    *
*********************************************************************************************************************************/

UIVirtualSystemItem::UIVirtualSystemItem(int iNumber, CVirtualSystemDescription comDescription, UIApplianceModelItem *pParentItem)
    : UIApplianceModelItem(iNumber, ApplianceModelItemType_VirtualSystem, pParentItem)
    , m_comDescription(comDescription)
{
}

QVariant UIVirtualSystemItem::data(int iColumn, int iRole) const
{
    QVariant value;
    if (iColumn == ApplianceViewSection_Description &&
        iRole == Qt::DisplayRole)
        value = UIApplianceEditorWidget::tr("Virtual System %1").arg(m_iNumber + 1);
    return value;
}

void UIVirtualSystemItem::putBack(QVector<BOOL> &finalStates, QVector<QString> &finalValues, QVector<QString> &finalExtraValues)
{
    /* Resize the vectors */
    unsigned long iCount = m_comDescription.GetCount();
    finalStates.resize(iCount);
    finalValues.resize(iCount);
    finalExtraValues.resize(iCount);
    /* Recursively fill the vectors */
    UIApplianceModelItem::putBack(finalStates, finalValues, finalExtraValues);
    /* Set all final values at once */
    m_comDescription.SetFinalValues(finalStates, finalValues, finalExtraValues);
}


/*********************************************************************************************************************************
*   Class UIVirtualHardwareItem implementation.                                                                                  *
*********************************************************************************************************************************/

UIVirtualHardwareItem::UIVirtualHardwareItem(int iNumber,
                                             KVirtualSystemDescriptionType enmVSDType,
                                             const QString &strRef,
                                             const QString &aOrigValue,
                                             const QString &strConfigValue,
                                             const QString &strExtraConfigValue,
                                             UIApplianceModelItem *pParentItem)
    : UIApplianceModelItem(iNumber, ApplianceModelItemType_VirtualHardware, pParentItem)
    , m_enmVSDType(enmVSDType)
    , m_strRef(strRef)
    , m_strOrigValue(aOrigValue)
    , m_strConfigValue(strConfigValue)
    , m_strConfigDefaultValue(strConfigValue)
    , m_strExtraConfigValue(strExtraConfigValue)
    , m_checkState(Qt::Checked)
    , m_fModified(false)
{
}

Qt::ItemFlags UIVirtualHardwareItem::itemFlags(int iColumn) const
{
    Qt::ItemFlags enmFlags = 0;
    if (iColumn == ApplianceViewSection_ConfigValue)
    {
        /* Some items are checkable */
        if (m_enmVSDType == KVirtualSystemDescriptionType_Floppy ||
            m_enmVSDType == KVirtualSystemDescriptionType_CDROM ||
            m_enmVSDType == KVirtualSystemDescriptionType_USBController ||
            m_enmVSDType == KVirtualSystemDescriptionType_SoundCard ||
            m_enmVSDType == KVirtualSystemDescriptionType_NetworkAdapter)
            enmFlags |= Qt::ItemIsUserCheckable;
        /* Some items are editable */
        if ((m_enmVSDType == KVirtualSystemDescriptionType_Name ||
             m_enmVSDType == KVirtualSystemDescriptionType_Product ||
             m_enmVSDType == KVirtualSystemDescriptionType_ProductUrl ||
             m_enmVSDType == KVirtualSystemDescriptionType_Vendor ||
             m_enmVSDType == KVirtualSystemDescriptionType_VendorUrl ||
             m_enmVSDType == KVirtualSystemDescriptionType_Version ||
             m_enmVSDType == KVirtualSystemDescriptionType_Description ||
             m_enmVSDType == KVirtualSystemDescriptionType_License ||
             m_enmVSDType == KVirtualSystemDescriptionType_OS ||
             m_enmVSDType == KVirtualSystemDescriptionType_CPU ||
             m_enmVSDType == KVirtualSystemDescriptionType_Memory ||
             m_enmVSDType == KVirtualSystemDescriptionType_SoundCard ||
             m_enmVSDType == KVirtualSystemDescriptionType_NetworkAdapter ||
             m_enmVSDType == KVirtualSystemDescriptionType_HardDiskControllerIDE ||
             m_enmVSDType == KVirtualSystemDescriptionType_HardDiskImage) &&
            m_checkState == Qt::Checked) /* Item has to be enabled */
            enmFlags |= Qt::ItemIsEditable;
    }
    return enmFlags;
}

bool UIVirtualHardwareItem::setData(int iColumn, const QVariant &value, int iRole)
{
    bool fDone = false;
    switch (iRole)
    {
        case Qt::CheckStateRole:
        {
            if (iColumn == ApplianceViewSection_ConfigValue &&
                (m_enmVSDType == KVirtualSystemDescriptionType_Floppy ||
                 m_enmVSDType == KVirtualSystemDescriptionType_CDROM ||
                 m_enmVSDType == KVirtualSystemDescriptionType_USBController ||
                 m_enmVSDType == KVirtualSystemDescriptionType_SoundCard ||
                 m_enmVSDType == KVirtualSystemDescriptionType_NetworkAdapter))
            {
                m_checkState = static_cast<Qt::CheckState>(value.toInt());
                fDone = true;
            }
            break;
        }
        case Qt::EditRole:
        {
            if (iColumn == ApplianceViewSection_OriginalValue)
                m_strOrigValue = value.toString();
            else if (iColumn == ApplianceViewSection_ConfigValue)
                m_strConfigValue = value.toString();
            break;
        }
        default: break;
    }
    return fDone;
}

QVariant UIVirtualHardwareItem::data(int iColumn, int iRole) const
{
    QVariant value;
    switch (iRole)
    {
        case Qt::EditRole:
        {
            if (iColumn == ApplianceViewSection_OriginalValue)
                value = m_strOrigValue;
            else if (iColumn == ApplianceViewSection_ConfigValue)
                value = m_strConfigValue;
            break;
        }
        case Qt::DisplayRole:
        {
            if (iColumn == ApplianceViewSection_Description)
            {
                switch (m_enmVSDType)
                {
                    case KVirtualSystemDescriptionType_Name:                   value = UIApplianceEditorWidget::tr("Name"); break;
                    case KVirtualSystemDescriptionType_Product:                value = UIApplianceEditorWidget::tr("Product"); break;
                    case KVirtualSystemDescriptionType_ProductUrl:             value = UIApplianceEditorWidget::tr("Product-URL"); break;
                    case KVirtualSystemDescriptionType_Vendor:                 value = UIApplianceEditorWidget::tr("Vendor"); break;
                    case KVirtualSystemDescriptionType_VendorUrl:              value = UIApplianceEditorWidget::tr("Vendor-URL"); break;
                    case KVirtualSystemDescriptionType_Version:                value = UIApplianceEditorWidget::tr("Version"); break;
                    case KVirtualSystemDescriptionType_Description:            value = UIApplianceEditorWidget::tr("Description"); break;
                    case KVirtualSystemDescriptionType_License:                value = UIApplianceEditorWidget::tr("License"); break;
                    case KVirtualSystemDescriptionType_OS:                     value = UIApplianceEditorWidget::tr("Guest OS Type"); break;
                    case KVirtualSystemDescriptionType_CPU:                    value = UIApplianceEditorWidget::tr("CPU"); break;
                    case KVirtualSystemDescriptionType_Memory:                 value = UIApplianceEditorWidget::tr("RAM"); break;
                    case KVirtualSystemDescriptionType_HardDiskControllerIDE:  value = UIApplianceEditorWidget::tr("Storage Controller (IDE)"); break;
                    case KVirtualSystemDescriptionType_HardDiskControllerSATA: value = UIApplianceEditorWidget::tr("Storage Controller (SATA)"); break;
                    case KVirtualSystemDescriptionType_HardDiskControllerSCSI: value = UIApplianceEditorWidget::tr("Storage Controller (SCSI)"); break;
                    case KVirtualSystemDescriptionType_HardDiskControllerSAS:  value = UIApplianceEditorWidget::tr("Storage Controller (SAS)"); break;
                    case KVirtualSystemDescriptionType_CDROM:                  value = UIApplianceEditorWidget::tr("DVD"); break;
                    case KVirtualSystemDescriptionType_Floppy:                 value = UIApplianceEditorWidget::tr("Floppy"); break;
                    case KVirtualSystemDescriptionType_NetworkAdapter:         value = UIApplianceEditorWidget::tr("Network Adapter"); break;
                    case KVirtualSystemDescriptionType_USBController:          value = UIApplianceEditorWidget::tr("USB Controller"); break;
                    case KVirtualSystemDescriptionType_SoundCard:              value = UIApplianceEditorWidget::tr("Sound Card"); break;
                    case KVirtualSystemDescriptionType_HardDiskImage:          value = UIApplianceEditorWidget::tr("Virtual Disk Image"); break;
                    default:                                                   value = UIApplianceEditorWidget::tr("Unknown Hardware Item"); break;
                }
            }
            else if (iColumn == ApplianceViewSection_OriginalValue)
                value = m_strOrigValue;
            else if (iColumn == ApplianceViewSection_ConfigValue)
            {
                switch (m_enmVSDType)
                {
                    case KVirtualSystemDescriptionType_Description:
                    case KVirtualSystemDescriptionType_License:
                    {
                        /* Shorten the big text if there is more than
                         * one line */
                        QString strTmp(m_strConfigValue);
                        int i = strTmp.indexOf('\n');
                        if (i > -1)
                            strTmp.replace(i, strTmp.length(), "...");
                        value = strTmp; break;
                    }
                    case KVirtualSystemDescriptionType_OS:             value = vboxGlobal().vmGuestOSTypeDescription(m_strConfigValue); break;
                    case KVirtualSystemDescriptionType_Memory:         value = m_strConfigValue + " " + VBoxGlobal::tr("MB", "size suffix MBytes=1024 KBytes"); break;
                    case KVirtualSystemDescriptionType_SoundCard:      value = gpConverter->toString(static_cast<KAudioControllerType>(m_strConfigValue.toInt())); break;
                    case KVirtualSystemDescriptionType_NetworkAdapter: value = gpConverter->toString(static_cast<KNetworkAdapterType>(m_strConfigValue.toInt())); break;
                    default:                                           value = m_strConfigValue; break;
                }
            }
            break;
        }
        case Qt::ToolTipRole:
        {
            if (iColumn == ApplianceViewSection_ConfigValue)
            {
                if (!m_strOrigValue.isEmpty())
                    value = UIApplianceEditorWidget::tr("<b>Original Value:</b> %1").arg(m_strOrigValue);
            }
            break;
        }
        case Qt::DecorationRole:
        {
            if (iColumn == ApplianceViewSection_Description)
            {
                switch (m_enmVSDType)
                {
                    case KVirtualSystemDescriptionType_Name:                   value = UIIconPool::iconSet(":/name_16px.png"); break;
                    case KVirtualSystemDescriptionType_Product:
                    case KVirtualSystemDescriptionType_ProductUrl:
                    case KVirtualSystemDescriptionType_Vendor:
                    case KVirtualSystemDescriptionType_VendorUrl:
                    case KVirtualSystemDescriptionType_Version:
                    case KVirtualSystemDescriptionType_Description:
                    case KVirtualSystemDescriptionType_License:                value = UIIconPool::iconSet(":/description_16px.png"); break;
                    case KVirtualSystemDescriptionType_OS:                     value = UIIconPool::iconSet(":/os_type_16px.png"); break;
                    case KVirtualSystemDescriptionType_CPU:                    value = UIIconPool::iconSet(":/cpu_16px.png"); break;
                    case KVirtualSystemDescriptionType_Memory:                 value = UIIconPool::iconSet(":/ram_16px.png"); break;
                    case KVirtualSystemDescriptionType_HardDiskControllerIDE:  value = UIIconPool::iconSet(":/ide_16px.png"); break;
                    case KVirtualSystemDescriptionType_HardDiskControllerSATA: value = UIIconPool::iconSet(":/sata_16px.png"); break;
                    case KVirtualSystemDescriptionType_HardDiskControllerSCSI: value = UIIconPool::iconSet(":/scsi_16px.png"); break;
                    case KVirtualSystemDescriptionType_HardDiskControllerSAS:  value = UIIconPool::iconSet(":/scsi_16px.png"); break;
                    case KVirtualSystemDescriptionType_HardDiskImage:          value = UIIconPool::iconSet(":/hd_16px.png"); break;
                    case KVirtualSystemDescriptionType_CDROM:                  value = UIIconPool::iconSet(":/cd_16px.png"); break;
                    case KVirtualSystemDescriptionType_Floppy:                 value = UIIconPool::iconSet(":/fd_16px.png"); break;
                    case KVirtualSystemDescriptionType_NetworkAdapter:         value = UIIconPool::iconSet(":/nw_16px.png"); break;
                    case KVirtualSystemDescriptionType_USBController:          value = UIIconPool::iconSet(":/usb_16px.png"); break;
                    case KVirtualSystemDescriptionType_SoundCard:              value = UIIconPool::iconSet(":/sound_16px.png"); break;
                    default: break;
                }
            }
            else if (iColumn == ApplianceViewSection_ConfigValue && m_enmVSDType == KVirtualSystemDescriptionType_OS)
                value = vboxGlobal().vmGuestOSTypeIcon(m_strConfigValue);
            break;
        }
        case Qt::FontRole:
        {
            /* If the item is unchecked mark it with italic text. */
            if (iColumn == ApplianceViewSection_ConfigValue &&
                m_checkState == Qt::Unchecked)
            {
                QFont font = qApp->font();
                font.setItalic(true);
                value = font;
            }
            break;
        }
        case Qt::ForegroundRole:
        {
            /* If the item is unchecked mark it with gray text. */
            if (iColumn == ApplianceViewSection_ConfigValue &&
                m_checkState == Qt::Unchecked)
            {
                QPalette pal = qApp->palette();
                value = pal.brush(QPalette::Disabled, QPalette::WindowText);
            }
            break;
        }
        case Qt::CheckStateRole:
        {
            if (iColumn == ApplianceViewSection_ConfigValue &&
                (m_enmVSDType == KVirtualSystemDescriptionType_Floppy ||
                 m_enmVSDType == KVirtualSystemDescriptionType_CDROM ||
                 m_enmVSDType == KVirtualSystemDescriptionType_USBController ||
                 m_enmVSDType == KVirtualSystemDescriptionType_SoundCard ||
                 m_enmVSDType == KVirtualSystemDescriptionType_NetworkAdapter))
                value = m_checkState;
            break;
        }
        case UIVirtualHardwareItem::TypeRole:
        {
            value = m_enmVSDType;
            break;
        }
        case UIVirtualHardwareItem::ModifiedRole:
        {
            if (iColumn == ApplianceViewSection_ConfigValue)
                value = m_fModified;
            break;
        }
    }
    return value;
}

QWidget *UIVirtualHardwareItem::createEditor(QWidget *pParent, const QStyleOptionViewItem & /* styleOption */, const QModelIndex &idx) const
{
    QWidget *pEditor = 0;
    if (idx.column() == ApplianceViewSection_ConfigValue)
    {
        switch (m_enmVSDType)
        {
            case KVirtualSystemDescriptionType_OS:
            {
                VBoxOSTypeSelectorButton *pButton = new VBoxOSTypeSelectorButton(pParent);
                /* Fill the background with the highlight color in the case
                 * the button hasn't a rectangle shape. This prevents the
                 * display of parts from the current text on the Mac. */
#ifdef VBOX_WS_MAC
                /* Use the palette from the tree view, not the one from the
                 * editor. */
                QPalette p = pButton->palette();
                p.setBrush(QPalette::Highlight, pParent->palette().brush(QPalette::Highlight));
                pButton->setPalette(p);
#endif /* VBOX_WS_MAC */
                pButton->setAutoFillBackground(true);
                pButton->setBackgroundRole(QPalette::Highlight);
                pEditor = pButton;
                break;
            }
            case KVirtualSystemDescriptionType_Name:
            case KVirtualSystemDescriptionType_Product:
            case KVirtualSystemDescriptionType_ProductUrl:
            case KVirtualSystemDescriptionType_Vendor:
            case KVirtualSystemDescriptionType_VendorUrl:
            case KVirtualSystemDescriptionType_Version:
            {
                QLineEdit *pLineEdit = new QLineEdit(pParent);
                pEditor = pLineEdit;
                break;
            }
            case KVirtualSystemDescriptionType_Description:
            case KVirtualSystemDescriptionType_License:
            {
                UILineTextEdit *pLineTextEdit = new UILineTextEdit(pParent);
                pEditor = pLineTextEdit;
                break;
            }
            case KVirtualSystemDescriptionType_CPU:
            {
                QSpinBox *pSpinBox = new QSpinBox(pParent);
                pSpinBox->setRange(UIApplianceEditorWidget::minGuestCPUCount(), UIApplianceEditorWidget::maxGuestCPUCount());
                pEditor = pSpinBox;
                break;
            }
            case KVirtualSystemDescriptionType_Memory:
            {
                QSpinBox *pSpinBox = new QSpinBox(pParent);
                pSpinBox->setRange(UIApplianceEditorWidget::minGuestRAM(), UIApplianceEditorWidget::maxGuestRAM());
                pSpinBox->setSuffix(" " + VBoxGlobal::tr("MB", "size suffix MBytes=1024 KBytes"));
                pEditor = pSpinBox;
                break;
            }
            case KVirtualSystemDescriptionType_SoundCard:
            {
                QComboBox *pComboBox = new QComboBox(pParent);
                pComboBox->addItem(gpConverter->toString(KAudioControllerType_AC97), KAudioControllerType_AC97);
                pComboBox->addItem(gpConverter->toString(KAudioControllerType_SB16), KAudioControllerType_SB16);
                pComboBox->addItem(gpConverter->toString(KAudioControllerType_HDA),  KAudioControllerType_HDA);
                pEditor = pComboBox;
                break;
            }
            case KVirtualSystemDescriptionType_NetworkAdapter:
            {
                QComboBox *pComboBox = new QComboBox(pParent);
                pComboBox->addItem(gpConverter->toString(KNetworkAdapterType_Am79C970A), KNetworkAdapterType_Am79C970A);
                pComboBox->addItem(gpConverter->toString(KNetworkAdapterType_Am79C973), KNetworkAdapterType_Am79C973);
#ifdef VBOX_WITH_E1000
                pComboBox->addItem(gpConverter->toString(KNetworkAdapterType_I82540EM), KNetworkAdapterType_I82540EM);
                pComboBox->addItem(gpConverter->toString(KNetworkAdapterType_I82543GC), KNetworkAdapterType_I82543GC);
                pComboBox->addItem(gpConverter->toString(KNetworkAdapterType_I82545EM), KNetworkAdapterType_I82545EM);
#endif /* VBOX_WITH_E1000 */
#ifdef VBOX_WITH_VIRTIO
                pComboBox->addItem(gpConverter->toString(KNetworkAdapterType_Virtio), KNetworkAdapterType_Virtio);
#endif /* VBOX_WITH_VIRTIO */
                pEditor = pComboBox;
                break;
            }
            case KVirtualSystemDescriptionType_HardDiskControllerIDE:
            {
                QComboBox *pComboBox = new QComboBox(pParent);
                pComboBox->addItem(gpConverter->toString(KStorageControllerType_PIIX3), "PIIX3");
                pComboBox->addItem(gpConverter->toString(KStorageControllerType_PIIX4), "PIIX4");
                pComboBox->addItem(gpConverter->toString(KStorageControllerType_ICH6),  "ICH6");
                pEditor = pComboBox;
                break;
            }
            case KVirtualSystemDescriptionType_HardDiskImage:
            {
                /* disabled for now
                   UIFilePathSelector *pFileChooser = new UIFilePathSelector(pParent);
                   pFileChooser->setMode(UIFilePathSelector::Mode_File);
                   pFileChooser->setResetEnabled(false);
                   */
                QLineEdit *pLineEdit = new QLineEdit(pParent);
                pEditor = pLineEdit;
                break;
            }
            default: break;
        }
    }
    return pEditor;
}

bool UIVirtualHardwareItem::setEditorData(QWidget *pEditor, const QModelIndex & /* idx */) const
{
    bool fDone = false;
    switch (m_enmVSDType)
    {
        case KVirtualSystemDescriptionType_OS:
        {
            if (VBoxOSTypeSelectorButton *pButton = qobject_cast<VBoxOSTypeSelectorButton*>(pEditor))
            {
                pButton->setOSTypeId(m_strConfigValue);
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_HardDiskControllerIDE:
        {
            if (QComboBox *pComboBox = qobject_cast<QComboBox*>(pEditor))
            {
                int i = pComboBox->findData(m_strConfigValue);
                if (i != -1)
                    pComboBox->setCurrentIndex(i);
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_CPU:
        case KVirtualSystemDescriptionType_Memory:
        {
            if (QSpinBox *pSpinBox = qobject_cast<QSpinBox*>(pEditor))
            {
                pSpinBox->setValue(m_strConfigValue.toInt());
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_Name:
        case KVirtualSystemDescriptionType_Product:
        case KVirtualSystemDescriptionType_ProductUrl:
        case KVirtualSystemDescriptionType_Vendor:
        case KVirtualSystemDescriptionType_VendorUrl:
        case KVirtualSystemDescriptionType_Version:
        {
            if (QLineEdit *pLineEdit = qobject_cast<QLineEdit*>(pEditor))
            {
                pLineEdit->setText(m_strConfigValue);
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_Description:
        case KVirtualSystemDescriptionType_License:
        {
            if (UILineTextEdit *pLineTextEdit = qobject_cast<UILineTextEdit*>(pEditor))
            {
                pLineTextEdit->setText(m_strConfigValue);
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_SoundCard:
        case KVirtualSystemDescriptionType_NetworkAdapter:
        {
            if (QComboBox *pComboBox = qobject_cast<QComboBox*>(pEditor))
            {
                int i = pComboBox->findData(m_strConfigValue.toInt());
                if (i != -1)
                    pComboBox->setCurrentIndex(i);
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_HardDiskImage:
        {
            /* disabled for now
               if (UIFilePathSelector *pFileChooser = qobject_cast<UIFilePathSelector*>(pEditor))
               {
               pFileChooser->setPath(m_strConfigValue);
               }
               */
            if (QLineEdit *pLineEdit = qobject_cast<QLineEdit*>(pEditor))
            {
                pLineEdit->setText(m_strConfigValue);
                fDone = true;
            }
            break;
        }
        default: break;
    }
    return fDone;
}

bool UIVirtualHardwareItem::setModelData(QWidget *pEditor, QAbstractItemModel *pModel, const QModelIndex & idx)
{
    bool fDone = false;
    switch (m_enmVSDType)
    {
        case KVirtualSystemDescriptionType_OS:
        {
            if (VBoxOSTypeSelectorButton *pButton = qobject_cast<VBoxOSTypeSelectorButton*>(pEditor))
            {
                m_strConfigValue = pButton->osTypeId();
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_HardDiskControllerIDE:
        {
            if (QComboBox *pComboBox = qobject_cast<QComboBox*>(pEditor))
            {
                m_strConfigValue = pComboBox->itemData(pComboBox->currentIndex()).toString();
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_CPU:
        case KVirtualSystemDescriptionType_Memory:
        {
            if (QSpinBox *pSpinBox = qobject_cast<QSpinBox*>(pEditor))
            {
                m_strConfigValue = QString::number(pSpinBox->value());
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_Name:
        {
            if (QLineEdit *pLineEdit = qobject_cast<QLineEdit*>(pEditor))
            {
                /* When the VM name is changed the path of the disk images
                 * should be also changed. So first of all find all disk
                 * images corresponding to this appliance. Next check if
                 * they are modified by the user already. If not change the
                 * path to the new path. */
                /* Create an index of this position, but in column 0. */
                QModelIndex c0Index = pModel->index(idx.row(), 0, idx.parent());
                /* Query all items with the type HardDiskImage and which
                 * are child's of this item. */
                QModelIndexList list = pModel->match(c0Index,
                                                     UIVirtualHardwareItem::TypeRole,
                                                     KVirtualSystemDescriptionType_HardDiskImage,
                                                     -1,
                                                     Qt::MatchExactly | Qt::MatchWrap | Qt::MatchRecursive);
                for (int i = 0; i < list.count(); ++i)
                {
                    /* Get the index for the config value column. */
                    QModelIndex hdIndex = pModel->index(list.at(i).row(), ApplianceViewSection_ConfigValue, list.at(i).parent());
                    /* Ignore it if was already modified by the user. */
                    if (!hdIndex.data(ModifiedRole).toBool())
                        /* Replace any occurrence of the old VM name with
                         * the new VM name. */
                    {
                        QStringList splittedOriginalPath = hdIndex.data(Qt::EditRole).toString().split(QDir::separator());
                        QStringList splittedNewPath;

                        foreach (QString a, splittedOriginalPath)
                        {
                            (a.compare(m_strConfigValue) == 0) ? splittedNewPath << pLineEdit->text() : splittedNewPath << a;
                        }

                        QString newPath = splittedNewPath.join(QDir::separator());

                        pModel->setData(hdIndex,
                                        newPath,
                                        Qt::EditRole);
                    }
                }
                m_strConfigValue = pLineEdit->text();
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_Product:
        case KVirtualSystemDescriptionType_ProductUrl:
        case KVirtualSystemDescriptionType_Vendor:
        case KVirtualSystemDescriptionType_VendorUrl:
        case KVirtualSystemDescriptionType_Version:
        {
            if (QLineEdit *pLineEdit = qobject_cast<QLineEdit*>(pEditor))
            {
                m_strConfigValue = pLineEdit->text();
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_Description:
        case KVirtualSystemDescriptionType_License:
        {
            if (UILineTextEdit *pLineTextEdit = qobject_cast<UILineTextEdit*>(pEditor))
            {
                m_strConfigValue = pLineTextEdit->text();
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_SoundCard:
        case KVirtualSystemDescriptionType_NetworkAdapter:
        {
            if (QComboBox *pComboBox = qobject_cast<QComboBox*>(pEditor))
            {
                m_strConfigValue = pComboBox->itemData(pComboBox->currentIndex()).toString();
                fDone = true;
            }
            break;
        }
        case KVirtualSystemDescriptionType_HardDiskImage:
        {
            /* disabled for now
               if (UIFilePathSelector *pFileChooser = qobject_cast<UIFilePathSelector*>(pEditor))
               {
               m_strConfigValue = pFileChooser->path();
               }
               */
            if (QLineEdit *pLineEdit = qobject_cast<QLineEdit*>(pEditor))
            {
                m_strConfigValue = pLineEdit->text();
                fDone = true;
            }
            break;
        }
        default: break;
    }
    if (fDone)
        m_fModified = true;

    return fDone;
}

void UIVirtualHardwareItem::restoreDefaults()
{
    m_strConfigValue = m_strConfigDefaultValue;
    m_checkState = Qt::Checked;
}

void UIVirtualHardwareItem::putBack(QVector<BOOL> &finalStates, QVector<QString> &finalValues, QVector<QString> &finalExtraValues)
{
    finalStates[m_iNumber] = m_checkState == Qt::Checked;
    finalValues[m_iNumber] = m_strConfigValue;
    finalExtraValues[m_iNumber] = m_strExtraConfigValue;
    UIApplianceModelItem::putBack(finalStates, finalValues, finalExtraValues);
}


/*********************************************************************************************************************************
*   Class UIApplianceModel implementation.                                                                                       *
*********************************************************************************************************************************/

UIApplianceModel::UIApplianceModel(QVector<CVirtualSystemDescription>& aVSDs, QITreeView *pParent)
    : QAbstractItemModel(pParent)
    , m_pRootItem(new UIApplianceModelItem(0, ApplianceModelItemType_Root, pParent))
{
    for (int iVSDIndex = 0; iVSDIndex < aVSDs.size(); ++iVSDIndex)
    {
        CVirtualSystemDescription vsd = aVSDs[iVSDIndex];

        UIVirtualSystemItem *pVirtualSystemItem = new UIVirtualSystemItem(iVSDIndex, vsd, m_pRootItem);
        m_pRootItem->appendChild(pVirtualSystemItem);

        /** @todo ask Dmitry about include/COMDefs.h:232 */
        QVector<KVirtualSystemDescriptionType> types;
        QVector<QString> refs;
        QVector<QString> origValues;
        QVector<QString> configValues;
        QVector<QString> extraConfigValues;

        QList<int> hdIndexes;
        QMap<int, UIVirtualHardwareItem*> controllerMap;
        vsd.GetDescription(types, refs, origValues, configValues, extraConfigValues);
        for (int i = 0; i < types.size(); ++i)
        {
            /* We add the hard disk images in an second step, so save a
               reference to them. */
            if (types[i] == KVirtualSystemDescriptionType_HardDiskImage)
                hdIndexes << i;
            else
            {
                UIVirtualHardwareItem *pHardwareItem = new UIVirtualHardwareItem(i, types[i], refs[i], origValues[i], configValues[i], extraConfigValues[i], pVirtualSystemItem);
                pVirtualSystemItem->appendChild(pHardwareItem);
                /* Save the hard disk controller types in an extra map */
                if (types[i] == KVirtualSystemDescriptionType_HardDiskControllerIDE ||
                    types[i] == KVirtualSystemDescriptionType_HardDiskControllerSATA ||
                    types[i] == KVirtualSystemDescriptionType_HardDiskControllerSCSI ||
                    types[i] == KVirtualSystemDescriptionType_HardDiskControllerSAS)
                    controllerMap[i] = pHardwareItem;
            }
        }
        QRegExp rx("controller=(\\d+);?");
        /* Now process the hard disk images */
        for (int iHDIndex = 0; iHDIndex < hdIndexes.size(); ++iHDIndex)
        {
            int i = hdIndexes[iHDIndex];
            QString ecnf = extraConfigValues[i];
            if (rx.indexIn(ecnf) != -1)
            {
                /* Get the controller */
                UIVirtualHardwareItem *pControllerItem = controllerMap[rx.cap(1).toInt()];
                if (pControllerItem)
                {
                    /* New hardware item as child of the controller */
                    UIVirtualHardwareItem *pStorageItem = new UIVirtualHardwareItem(i, types[i], refs[i], origValues[i], configValues[i], extraConfigValues[i], pControllerItem);
                    pControllerItem->appendChild(pStorageItem);
                }
            }
        }
    }
}

UIApplianceModel::~UIApplianceModel()
{
    if (m_pRootItem)
        delete m_pRootItem;
}

QModelIndex UIApplianceModel::root() const
{
    return index(0, 0);
}

QModelIndex UIApplianceModel::index(int iRow, int iColumn, const QModelIndex &parentIdx /* = QModelIndex() */) const
{
    if (!hasIndex(iRow, iColumn, parentIdx))
        return QModelIndex();

    UIApplianceModelItem *pItem = !parentIdx.isValid() ? m_pRootItem :
                                  static_cast<UIApplianceModelItem*>(parentIdx.internalPointer())->childItem(iRow);

    return pItem ? createIndex(iRow, iColumn, pItem) : QModelIndex();
}

QModelIndex UIApplianceModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();

    UIApplianceModelItem *pItem = static_cast<UIApplianceModelItem*>(idx.internalPointer());
    UIApplianceModelItem *pParentItem = pItem->parent();

    if (pParentItem)
        return createIndex(pParentItem->row(), 0, pParentItem);
    else
        return QModelIndex();
}

int UIApplianceModel::rowCount(const QModelIndex &parentIdx /* = QModelIndex() */) const
{
    return !parentIdx.isValid() ? 1 /* only root item has invalid parent */ :
           static_cast<UIApplianceModelItem*>(parentIdx.internalPointer())->childCount();
}

int UIApplianceModel::columnCount(const QModelIndex &parentIdx /* = QModelIndex() */) const
{
    return !parentIdx.isValid() ? m_pRootItem->columnCount() :
           static_cast<UIApplianceModelItem*>(parentIdx.internalPointer())->columnCount();
}

Qt::ItemFlags UIApplianceModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return 0;

    UIApplianceModelItem *pItem = static_cast<UIApplianceModelItem*>(idx.internalPointer());

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | pItem->itemFlags(idx.column());
}

QVariant UIApplianceModel::headerData(int iSection, Qt::Orientation enmOrientation, int iRole) const
{
    if (iRole != Qt::DisplayRole ||
        enmOrientation != Qt::Horizontal)
        return QVariant();

    QString strTitle;
    switch (iSection)
    {
        case ApplianceViewSection_Description: strTitle = UIApplianceEditorWidget::tr("Description"); break;
        case ApplianceViewSection_ConfigValue: strTitle = UIApplianceEditorWidget::tr("Configuration"); break;
    }
    return strTitle;
}

bool UIApplianceModel::setData(const QModelIndex &idx, const QVariant &value, int iRole)
{
    if (!idx.isValid())
        return false;

    UIApplianceModelItem *pTtem = static_cast<UIApplianceModelItem*>(idx.internalPointer());

    return pTtem->setData(idx.column(), value, iRole);
}

QVariant UIApplianceModel::data(const QModelIndex &idx, int iRole /* = Qt::DisplayRole */) const
{
    if (!idx.isValid())
        return QVariant();

    UIApplianceModelItem *pTtem = static_cast<UIApplianceModelItem*>(idx.internalPointer());

    return pTtem->data(idx.column(), iRole);
}

QModelIndex UIApplianceModel::buddy(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();

    if (idx.column() == ApplianceViewSection_ConfigValue)
        return idx;
    else
        return index(idx.row(), ApplianceViewSection_ConfigValue, idx.parent());
}

void UIApplianceModel::restoreDefaults(QModelIndex parentIdx /* = QModelIndex() */)
{
    /* By default use the root: */
    if (!parentIdx.isValid())
        parentIdx = root();

    /* Get corresponding parent item and enumerate it's children: */
    UIApplianceModelItem *pParentItem = static_cast<UIApplianceModelItem*>(parentIdx.internalPointer());
    for (int i = 0; i < pParentItem->childCount(); ++i)
    {
        /* Reset children item data to default: */
        pParentItem->childItem(i)->restoreDefaults();
        /* Recursively process children item: */
        restoreDefaults(index(i, 0, parentIdx));
    }
    /* Notify the model about the changes: */
    emit dataChanged(index(0, 0, parentIdx), index(pParentItem->childCount() - 1, 0, parentIdx));
}

void UIApplianceModel::putBack()
{
    QVector<BOOL> v1;
    QVector<QString> v2;
    QVector<QString> v3;
    m_pRootItem->putBack(v1, v2, v3);
}


/*********************************************************************************************************************************
*   Class UIApplianceDelegate implementation.                                                                                    *
*********************************************************************************************************************************/

UIApplianceDelegate::UIApplianceDelegate(QAbstractProxyModel *pProxy, QObject *pParent /* = 0 */)
    : QItemDelegate(pParent)
    , m_pProxy(pProxy)
{
}

QWidget *UIApplianceDelegate::createEditor(QWidget *pParent, const QStyleOptionViewItem &styleOption, const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QItemDelegate::createEditor(pParent, styleOption, idx);

    QModelIndex index(idx);
    if (m_pProxy)
        index = m_pProxy->mapToSource(idx);

    UIApplianceModelItem *pItem = static_cast<UIApplianceModelItem*>(index.internalPointer());
    QWidget *pEditor = pItem->createEditor(pParent, styleOption, index);

    /* Allow UILineTextEdit to commit data early: */
    if (pEditor && qobject_cast<UILineTextEdit*>(pEditor))
        connect(pEditor, SIGNAL(sigFinished(QWidget*)), this, SIGNAL(commitData(QWidget*)));

    if (pEditor == 0)
        return QItemDelegate::createEditor(pParent, styleOption, index);
    else
        return pEditor;
}

void UIApplianceDelegate::setEditorData(QWidget *pEditor, const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QItemDelegate::setEditorData(pEditor, idx);

    QModelIndex index(idx);
    if (m_pProxy)
        index = m_pProxy->mapToSource(idx);

    UIApplianceModelItem *pItem = static_cast<UIApplianceModelItem*>(index.internalPointer());

    if (!pItem->setEditorData(pEditor, index))
        QItemDelegate::setEditorData(pEditor, index);
}

void UIApplianceDelegate::setModelData(QWidget *pEditor, QAbstractItemModel *pModel, const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QItemDelegate::setModelData(pEditor, pModel, idx);

    QModelIndex index = pModel->index(idx.row(), idx.column());
    if (m_pProxy)
        index = m_pProxy->mapToSource(idx);

    UIApplianceModelItem *pItem = static_cast<UIApplianceModelItem*>(index.internalPointer());
    if (!pItem->setModelData(pEditor, pModel, idx))
        QItemDelegate::setModelData(pEditor, pModel, idx);
}

void UIApplianceDelegate::updateEditorGeometry(QWidget *pEditor, const QStyleOptionViewItem &styleOption, const QModelIndex & /* idx */) const
{
    if (pEditor)
        pEditor->setGeometry(styleOption.rect);
}

QSize UIApplianceDelegate::sizeHint(const QStyleOptionViewItem &styleOption, const QModelIndex &idx) const
{
    QSize size = QItemDelegate::sizeHint(styleOption, idx);
#ifdef VBOX_WS_MAC
    int h = 28;
#else
    int h = 24;
#endif
    size.setHeight(RT_MAX(h, size.height()));
    return size;
}

#ifdef VBOX_WS_MAC
bool UIApplianceDelegate::eventFilter(QObject *pObject, QEvent *pEvent)
{
    if (pEvent->type() == QEvent::FocusOut)
    {
        /* On Mac OS X Cocoa the OS type selector widget loses it focus when
         * the popup menu is shown. Prevent this here, cause otherwise the new
         * selected OS will not be updated. */
        VBoxOSTypeSelectorButton *pButton = qobject_cast<VBoxOSTypeSelectorButton*>(pObject);
        if (pButton && pButton->isMenuShown())
            return false;
        /* The same counts for the text edit buttons of the license or
         * description fields. */
        else if (qobject_cast<UILineTextEdit*>(pObject))
            return false;
    }

    return QItemDelegate::eventFilter(pObject, pEvent);
}
#endif /* VBOX_WS_MAC */


/*********************************************************************************************************************************
*   Class UIApplianceSortProxyModel implementation.                                                                              *
*********************************************************************************************************************************/

/* static */
KVirtualSystemDescriptionType UIApplianceSortProxyModel::s_aSortList[] =
{
    KVirtualSystemDescriptionType_Name,
    KVirtualSystemDescriptionType_Product,
    KVirtualSystemDescriptionType_ProductUrl,
    KVirtualSystemDescriptionType_Vendor,
    KVirtualSystemDescriptionType_VendorUrl,
    KVirtualSystemDescriptionType_Version,
    KVirtualSystemDescriptionType_Description,
    KVirtualSystemDescriptionType_License,
    KVirtualSystemDescriptionType_OS,
    KVirtualSystemDescriptionType_CPU,
    KVirtualSystemDescriptionType_Memory,
    KVirtualSystemDescriptionType_Floppy,
    KVirtualSystemDescriptionType_CDROM,
    KVirtualSystemDescriptionType_USBController,
    KVirtualSystemDescriptionType_SoundCard,
    KVirtualSystemDescriptionType_NetworkAdapter,
    KVirtualSystemDescriptionType_HardDiskControllerIDE,
    KVirtualSystemDescriptionType_HardDiskControllerSATA,
    KVirtualSystemDescriptionType_HardDiskControllerSCSI,
    KVirtualSystemDescriptionType_HardDiskControllerSAS
};

UIApplianceSortProxyModel::UIApplianceSortProxyModel(QObject *pParent)
    : QSortFilterProxyModel(pParent)
{
}

bool UIApplianceSortProxyModel::filterAcceptsRow(int iSourceRow, const QModelIndex &srcParenIdx) const
{
    /* By default enable all, we will explicitly filter out below */
    if (srcParenIdx.isValid())
    {
        QModelIndex i = srcParenIdx.child(iSourceRow, 0);
        if (i.isValid())
        {
            UIApplianceModelItem *pItem = static_cast<UIApplianceModelItem*>(i.internalPointer());
            /* We filter hardware types only */
            if (pItem->type() == ApplianceModelItemType_VirtualHardware)
            {
                UIVirtualHardwareItem *hwItem = static_cast<UIVirtualHardwareItem*>(pItem);
                /* The license type shouldn't be displayed */
                if (m_aFilteredList.contains(hwItem->m_enmVSDType))
                    return false;
            }
        }
    }
    return true;
}

bool UIApplianceSortProxyModel::lessThan(const QModelIndex &leftIdx, const QModelIndex &rightIdx) const
{
    if (!leftIdx.isValid() ||
        !rightIdx.isValid())
        return false;

    UIApplianceModelItem *pLeftItem = static_cast<UIApplianceModelItem*>(leftIdx.internalPointer());
    UIApplianceModelItem *pRightItem = static_cast<UIApplianceModelItem*>(rightIdx.internalPointer());

    /* We sort hardware types only */
    if (!(pLeftItem->type() == ApplianceModelItemType_VirtualHardware &&
          pRightItem->type() == ApplianceModelItemType_VirtualHardware))
        return false;

    UIVirtualHardwareItem *pHwLeft = static_cast<UIVirtualHardwareItem*>(pLeftItem);
    UIVirtualHardwareItem *pHwRight = static_cast<UIVirtualHardwareItem*>(pRightItem);

    for (unsigned int i = 0; i < RT_ELEMENTS(s_aSortList); ++i)
        if (pHwLeft->m_enmVSDType == s_aSortList[i])
        {
            for (unsigned int a = 0; a <= i; ++a)
                if (pHwRight->m_enmVSDType == s_aSortList[a])
                    return true;
            return false;
        }

    return true;
}


/*********************************************************************************************************************************
*   Class UIApplianceEditorWidget implementation.                                                                                *
*********************************************************************************************************************************/

/* static */
int UIApplianceEditorWidget::m_minGuestRAM = -1;
int UIApplianceEditorWidget::m_maxGuestRAM = -1;
int UIApplianceEditorWidget::m_minGuestCPUCount = -1;
int UIApplianceEditorWidget::m_maxGuestCPUCount = -1;

UIApplianceEditorWidget::UIApplianceEditorWidget(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_pAppliance(0)
    , m_pModel(0)
{
    /* Make sure all static content is properly initialized */
    initSystemSettings();

    /* Create layout: */
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    {
        /* Configure information layout: */
        pLayout->setContentsMargins(0, 0, 0, 0);

        /* Create information pane: */
        m_pPaneInformation = new QWidget;
        {
            /* Create information layout: */
            QVBoxLayout *pLayoutInformation = new QVBoxLayout(m_pPaneInformation);
            {
                /* Configure information layout: */
                pLayoutInformation->setContentsMargins(0, 0, 0, 0);

                /* Create tree-view: */
                m_pTreeViewSettings = new QITreeView;
                {
                    /* Configure tree-view: */
                    m_pTreeViewSettings->setAlternatingRowColors(true);
                    m_pTreeViewSettings->setAllColumnsShowFocus(true);
                    m_pTreeViewSettings->header()->setStretchLastSection(true);
                    m_pTreeViewSettings->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
                    m_pTreeViewSettings->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

                    /* Add tree-view into information layout: */
                    pLayoutInformation->addWidget(m_pTreeViewSettings);
                }

                /* Create check-box: */
                m_pCheckBoxReinitMACs = new QCheckBox;
                {
                    /* Configure check-box: */
                    m_pCheckBoxReinitMACs->setHidden(true);

                    /* Add tree-view into information layout: */
                    pLayoutInformation->addWidget(m_pCheckBoxReinitMACs);
                }
            }

            /* Add information pane into layout: */
            pLayout->addWidget(m_pPaneInformation);
        }

        /* Create warning pane: */
        m_pPaneWarning  = new QWidget;
        {
            /* Configure warning pane: */
            m_pPaneWarning->hide();
            m_pPaneWarning->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

            /* Create warning layout: */
            QVBoxLayout *pLayoutWarning = new QVBoxLayout(m_pPaneWarning);
            {
                /* Configure warning layout: */
                pLayoutWarning->setContentsMargins(0, 0, 0, 0);

                /* Create label: */
                m_pLabelWarning = new QLabel;
                {
                    /* Add label into warning layout: */
                    pLayoutWarning->addWidget(m_pLabelWarning);
                }

                /* Create text-edit: */
                m_pTextEditWarning = new QTextEdit;
                {
                    /* Configure text-edit: */
                    m_pTextEditWarning->setReadOnly(true);
                    m_pTextEditWarning->setMaximumHeight(50);
                    m_pTextEditWarning->setAutoFormatting(QTextEdit::AutoBulletList);

                    /* Add text-edit into warning layout: */
                    pLayoutWarning->addWidget(m_pTextEditWarning);
                }
            }

            /* Add warning pane into layout: */
            pLayout->addWidget(m_pPaneWarning);
        }
    }

    /* Translate finally: */
    retranslateUi();
}

void UIApplianceEditorWidget::restoreDefaults()
{
    /* Make sure model exists, it's being created in sub-classes: */
    if (m_pModel)
        m_pModel->restoreDefaults();
}

void UIApplianceEditorWidget::retranslateUi()
{
    /* Translate information pane tree-view: */
    m_pTreeViewSettings->setWhatsThis(tr("Detailed list of all components of all virtual machines of the current appliance"));

    /* Translate information pane check-box: */
    m_pCheckBoxReinitMACs->setText(tr("&Reinitialize the MAC address of all network cards"));
    m_pCheckBoxReinitMACs->setToolTip(tr("When checked a new unique MAC address will assigned to all configured network cards."));

    /* Translate warning pane label: */
    m_pLabelWarning->setText(tr("Warnings:"));
}

/* static */
void UIApplianceEditorWidget::initSystemSettings()
{
    if (m_minGuestRAM == -1)
    {
        /* We need some global defaults from the current VirtualBox
           installation */
        CSystemProperties sp = vboxGlobal().virtualBox().GetSystemProperties();
        m_minGuestRAM        = sp.GetMinGuestRAM();
        m_maxGuestRAM        = sp.GetMaxGuestRAM();
        m_minGuestCPUCount   = sp.GetMinGuestCPUCount();
        m_maxGuestCPUCount   = sp.GetMaxGuestCPUCount();
    }
}

