/* $Id: UIMachineSettingsSF.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsSF class implementation.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
# include <QTimer>

/* GUI includes: */
# include "UIIconPool.h"
# include "UIMachineSettingsSF.h"
# include "UIMachineSettingsSFDetails.h"
# include "UIErrorString.h"
# include "VBoxGlobal.h"
# include "VBoxUtils.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Machine settings: Shared Folder data structure. */
struct UIDataSettingsSharedFolder
{
    /** Constructs data. */
    UIDataSettingsSharedFolder()
        : m_enmType(MachineType)
        , m_strName(QString())
        , m_strPath(QString())
        , m_fAutoMount(false)
        , m_fWritable(false)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsSharedFolder &other) const
    {
        return true
               && (m_enmType == other.m_enmType)
               && (m_strName == other.m_strName)
               && (m_strPath == other.m_strPath)
               && (m_fAutoMount == other.m_fAutoMount)
               && (m_fWritable == other.m_fWritable)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsSharedFolder &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsSharedFolder &other) const { return !equal(other); }

    /** Holds the shared folder type. */
    UISharedFolderType  m_enmType;
    /** Holds the shared folder name. */
    QString             m_strName;
    /** Holds the shared folder path. */
    QString             m_strPath;
    /** Holds whether the shared folder should be auto-mounted at startup. */
    bool                m_fAutoMount;
    /** Holds whether the shared folder should be writeable. */
    bool                m_fWritable;
};


/** Machine settings: Shared Folders page data structure. */
struct UIDataSettingsSharedFolders
{
    /** Constructs data. */
    UIDataSettingsSharedFolders() {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsSharedFolders & /* other */) const { return true; }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsSharedFolders & /* other */) const { return false; }
};


/** Machine settings: Shared Folder tree-widget item. */
class SFTreeViewItem : public QITreeWidgetItem, public UIDataSettingsSharedFolder
{
public:

    /** Format type. */
    enum FormatType
    {
        FormatType_Invalid,
        FormatType_EllipsisStart,
        FormatType_EllipsisMiddle,
        FormatType_EllipsisEnd,
        FormatType_EllipsisFile,
    };

    /** Constructs shared folder type (root) item.
      * @param  pParent    Brings the item parent.
      * @param  enmFormat  Brings the item format type. */
    SFTreeViewItem(QITreeWidget *pParent, FormatType enmFormat)
        : QITreeWidgetItem(pParent)
        , m_enmFormat(enmFormat)
    {
        setFirstColumnSpanned(true);
        setFlags(flags() ^ Qt::ItemIsSelectable);
    }

    /** Constructs shared folder (child) item.
      * @param  pParent    Brings the item parent.
      * @param  enmFormat  Brings the item format type. */
    SFTreeViewItem(SFTreeViewItem *pParent, FormatType enmFormat)
        : QITreeWidgetItem(pParent)
        , m_enmFormat(enmFormat)
    {
    }

    /** Returns whether this item is less than the @a other one. */
    bool operator<(const QTreeWidgetItem &other) const
    {
        /* Root items should always been sorted by type field: */
        return parentItem() ? text(0) < other.text(0) :
                              text(1) < other.text(1);
    }

    /** Returns child item number @a iIndex. */
    SFTreeViewItem *child(int iIndex) const
    {
        QTreeWidgetItem *pItem = QTreeWidgetItem::child(iIndex);
        return pItem ? static_cast<SFTreeViewItem*>(pItem) : 0;
    }

    /** Returns text of item number @a iIndex. */
    QString getText(int iIndex) const
    {
        return iIndex >= 0 && iIndex < m_fields.size() ? m_fields.at(iIndex) : QString();
    }

    /** Updates item fields. */
    void updateFields()
    {
        /* Clear fields: */
        m_fields.clear();

        /* For root items: */
        if (!parentItem())
            m_fields << m_strName
                     << QString::number((int)m_enmType);
        /* For child items: */
        else
            m_fields << m_strName
                     << m_strPath
                     << (m_fAutoMount ? UIMachineSettingsSF::tr("Yes") : "")
                     << (m_fWritable ? UIMachineSettingsSF::tr("Full") : UIMachineSettingsSF::tr("Read-only"));

        /* Adjust item layout: */
        adjustText();
    }

    /** Adjusts item layout. */
    void adjustText()
    {
        for (int i = 0; i < treeWidget()->columnCount(); ++i)
            processColumn(i);
    }

protected:

    /** Returns default text. */
    virtual QString defaultText() const /* override */
    {
        return parentItem() ?
               tr("%1, %2: %3, %4: %5, %6: %7",
                  "col.1 text, col.2 name: col.2 text, col.3 name: col.3 text, col.4 name: col.4 text")
                  .arg(text(0))
                  .arg(parentTree()->headerItem()->text(1)).arg(text(1))
                  .arg(parentTree()->headerItem()->text(2)).arg(text(2))
                  .arg(parentTree()->headerItem()->text(3)).arg(text(3)) :
               text(0);
    }

private:

    /** Performs item @a iColumn processing. */
    void processColumn(int iColumn)
    {
        QString strOneString = getText(iColumn);
        if (strOneString.isNull())
            return;
        const QFontMetrics fm = treeWidget()->fontMetrics();
        const int iOldSize = fm.width(strOneString);
        const int iItemIndent = parentItem() ? treeWidget()->indentation() * 2 : treeWidget()->indentation();
        int iIndentSize = fm.width(" ... ");
        if (iColumn == 0)
            iIndentSize += iItemIndent;
        const int cWidth = !parentItem() ? treeWidget()->viewport()->width() : treeWidget()->columnWidth(iColumn);

        /* Compress text: */
        int iStart = 0;
        int iFinish = 0;
        int iPosition = 0;
        int iTextWidth = 0;
        do
        {
            iTextWidth = fm.width(strOneString);
            if (   iTextWidth
                && (iTextWidth + iIndentSize > cWidth))
            {
                iStart = 0;
                iFinish = strOneString.length();

                /* Selecting remove position: */
                switch (m_enmFormat)
                {
                    case FormatType_EllipsisStart:
                        iPosition = iStart;
                        break;
                    case FormatType_EllipsisMiddle:
                        iPosition = (iFinish - iStart) / 2;
                        break;
                    case FormatType_EllipsisEnd:
                        iPosition = iFinish - 1;
                        break;
                    case FormatType_EllipsisFile:
                    {
                        const QRegExp regExp("([\\\\/][^\\\\^/]+[\\\\/]?$)");
                        const int iNewFinish = regExp.indexIn(strOneString);
                        if (iNewFinish != -1)
                            iFinish = iNewFinish;
                        iPosition = (iFinish - iStart) / 2;
                        break;
                    }
                    default:
                        AssertMsgFailed(("Invalid format type\n"));
                }

                if (iPosition == iFinish)
                   break;

                strOneString.remove(iPosition, 1);
            }
        }
        while (   iTextWidth
               && (iTextWidth + iIndentSize > cWidth));

        if (iPosition || m_enmFormat == FormatType_EllipsisFile)
            strOneString.insert(iPosition, "...");
        const int iNewSize = fm.width(strOneString);
        setText(iColumn, iNewSize < iOldSize ? strOneString : m_fields.at(iColumn));
        setToolTip(iColumn, text(iColumn) == getText(iColumn) ? QString() : getText(iColumn));

        /* Calculate item's size-hint: */
        setSizeHint(iColumn, QSize(fm.width(QString("  %1  ").arg(getText(iColumn))), fm.height()));
    }

    /** Holds the item format type. */
    FormatType   m_enmFormat;
    /** Holds the item text fields. */
    QStringList  m_fields;
};


UIMachineSettingsSF::UIMachineSettingsSF()
    : m_pActionAdd(0), m_pActionEdit(0), m_pActionRemove(0)
    , m_pCache(0)
{
    /* Prepare: */
    prepare();
}

UIMachineSettingsSF::~UIMachineSettingsSF()
{
    /* Cleanup: */
    cleanup();
}

bool UIMachineSettingsSF::changed() const
{
    return m_pCache->wasChanged();
}

void UIMachineSettingsSF::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old folders data: */
    UIDataSettingsSharedFolders oldFoldersData;

    /* Get actual folders: */
    QMap<UISharedFolderType, CSharedFolder> folders;
    /* Load machine (permanent) folders if allowed: */
    if (isSharedFolderTypeSupported(MachineType))
        foreach (const CSharedFolder &folder, getSharedFolders(MachineType))
            folders.insertMulti(MachineType, folder);
    /* Load console (temporary) folders if allowed: */
    if (isSharedFolderTypeSupported(ConsoleType))
        foreach (const CSharedFolder &folder, getSharedFolders(ConsoleType))
            folders.insertMulti(ConsoleType, folder);

    /* For each folder type: */
    foreach (const UISharedFolderType &enmFolderType, folders.keys())
    {
        /* For each folder of current type: */
        const QList<CSharedFolder> &currentTypeFolders = folders.values(enmFolderType);
        for (int iFolderIndex = 0; iFolderIndex < currentTypeFolders.size(); ++iFolderIndex)
        {
            /* Prepare old folder data & cache key: */
            UIDataSettingsSharedFolder oldFolderData;
            QString strFolderKey = QString::number(iFolderIndex);

            /* Check whether folder is valid:  */
            const CSharedFolder &comFolder = currentTypeFolders.at(iFolderIndex);
            if (!comFolder.isNull())
            {
                /* Gather old folder data: */
                oldFolderData.m_enmType = enmFolderType;
                oldFolderData.m_strName = comFolder.GetName();
                oldFolderData.m_strPath = comFolder.GetHostPath();
                oldFolderData.m_fAutoMount = comFolder.GetAutoMount();
                oldFolderData.m_fWritable = comFolder.GetWritable();
                /* Override folder cache key: */
                strFolderKey = oldFolderData.m_strName;
            }

            /* Cache old folder data: */
            m_pCache->child(strFolderKey).cacheInitialData(oldFolderData);
        }
    }

    /* Cache old folders data: */
    m_pCache->cacheInitialData(oldFoldersData);

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsSF::getFromCache()
{
    /* Clear list initially: */
    mTwFolders->clear();

    /* Update root items visibility: */
    updateRootItemsVisibility();

    /* For each folder => load it from the cache: */
    for (int iFolderIndex = 0; iFolderIndex < m_pCache->childCount(); ++iFolderIndex)
        addSharedFolderItem(m_pCache->child(iFolderIndex).base(), false /* its new? */);

    /* Choose first folder as current: */
    mTwFolders->setCurrentItem(mTwFolders->topLevelItem(0));
    sltHandleCurrentItemChange(mTwFolders->currentItem());

    /* Polish page finally: */
    polishPage();
}

void UIMachineSettingsSF::putToCache()
{
    /* Prepare new folders data: */
    UIDataSettingsSharedFolders newFoldersData;

    /* For each folder type: */
    QTreeWidgetItem *pMainRootItem = mTwFolders->invisibleRootItem();
    for (int iFolderTypeIndex = 0; iFolderTypeIndex < pMainRootItem->childCount(); ++iFolderTypeIndex)
    {
        /* Get folder root item: */
        const SFTreeViewItem *pFolderTypeRoot = static_cast<SFTreeViewItem*>(pMainRootItem->child(iFolderTypeIndex));

        /* For each folder of current type: */
        for (int iFolderIndex = 0; iFolderIndex < pFolderTypeRoot->childCount(); ++iFolderIndex)
        {
            /* Gather and cache new folder data: */
            const SFTreeViewItem *pItem = static_cast<SFTreeViewItem*>(pFolderTypeRoot->child(iFolderIndex));
            m_pCache->child(pItem->m_strName).cacheCurrentData(*pItem);
        }
    }

    /* Cache new folders data: */
    m_pCache->cacheCurrentData(newFoldersData);
}

void UIMachineSettingsSF::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Update folders data and failing state: */
    setFailed(!saveFoldersData());

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsSF::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIMachineSettingsSF::retranslateUi(this);

    m_pActionAdd->setText(tr("Add Shared Folder"));
    m_pActionEdit->setText(tr("Edit Shared Folder"));
    m_pActionRemove->setText(tr("Remove Shared Folder"));

    m_pActionAdd->setWhatsThis(tr("Adds new shared folder."));
    m_pActionEdit->setWhatsThis(tr("Edits selected shared folder."));
    m_pActionRemove->setWhatsThis(tr("Removes selected shared folder."));

    m_pActionAdd->setToolTip(m_pActionAdd->whatsThis());
    m_pActionEdit->setToolTip(m_pActionEdit->whatsThis());
    m_pActionRemove->setToolTip(m_pActionRemove->whatsThis());
}

void UIMachineSettingsSF::polishPage()
{
    /* Polish shared folders page availability: */
    mNameSeparator->setEnabled(isMachineInValidMode());
    m_pFoldersToolBar->setEnabled(isMachineInValidMode());
    m_pFoldersToolBar->setEnabled(isMachineInValidMode());

    /* Update root items visibility: */
    updateRootItemsVisibility();
}

void UIMachineSettingsSF::showEvent(QShowEvent *pEvent)
{
    /* Call to base-class: */
    UISettingsPageMachine::showEvent(pEvent);

    /* Connect header-resize signal just before widget is shown after all the items properly loaded and initialized: */
    connect(mTwFolders->header(), SIGNAL(sectionResized(int, int, int)), this, SLOT(sltAdjustTreeFields()));

    /* Adjusting size after all pending show events are processed: */
    QTimer::singleShot(0, this, SLOT(sltAdjustTree()));
}

void UIMachineSettingsSF::resizeEvent(QResizeEvent * /* pEvent */)
{
    sltAdjustTree();
}

void UIMachineSettingsSF::sltAddFolder()
{
    /* Configure folder details dialog: */
    UIMachineSettingsSFDetails dlgFolderDetails(UIMachineSettingsSFDetails::AddType,
                                                isSharedFolderTypeSupported(ConsoleType),
                                                usedList(true),
                                                this);

    /* Run folder details dialog: */
    if (dlgFolderDetails.exec() == QDialog::Accepted)
    {
        const QString strName = dlgFolderDetails.name();
        const QString strPath = dlgFolderDetails.path();
        const UISharedFolderType enmType = dlgFolderDetails.isPermanent() ? MachineType : ConsoleType;
        /* Shared folder's name & path could not be empty: */
        Assert(!strName.isEmpty() && !strPath.isEmpty());

        /* Prepare new folder data: */
        UIDataSettingsSharedFolder newFolderData;
        newFolderData.m_enmType = enmType;
        newFolderData.m_strName = strName;
        newFolderData.m_strPath = strPath;
        newFolderData.m_fAutoMount = dlgFolderDetails.isAutoMounted();
        newFolderData.m_fWritable = dlgFolderDetails.isWriteable();

        /* Add new folder item: */
        addSharedFolderItem(newFolderData, true /* its new? */);

        /* Sort tree-widget before adjusting: */
        mTwFolders->sortItems(0, Qt::AscendingOrder);
        /* Adjust tree-widget finally: */
        sltAdjustTree();
    }
}

void UIMachineSettingsSF::sltEditFolder()
{
    /* Check current folder item: */
    SFTreeViewItem *pItem = static_cast<SFTreeViewItem*>(mTwFolders->currentItem());
    AssertPtrReturnVoid(pItem);
    AssertPtrReturnVoid(pItem->parentItem());

    /* Configure folder details dialog: */
    UIMachineSettingsSFDetails dlgFolderDetails(UIMachineSettingsSFDetails::EditType,
                                                isSharedFolderTypeSupported(ConsoleType),
                                                usedList(false),
                                                this);
    dlgFolderDetails.setPath(pItem->m_strPath);
    dlgFolderDetails.setName(pItem->m_strName);
    dlgFolderDetails.setPermanent(pItem->m_enmType == MachineType);
    dlgFolderDetails.setAutoMount(pItem->m_fAutoMount);
    dlgFolderDetails.setWriteable(pItem->m_fWritable);

    /* Run folder details dialog: */
    if (dlgFolderDetails.exec() == QDialog::Accepted)
    {
        const QString strName = dlgFolderDetails.name();
        const QString strPath = dlgFolderDetails.path();
        const UISharedFolderType enmType = dlgFolderDetails.isPermanent() ? MachineType : ConsoleType;
        /* Shared folder's name & path could not be empty: */
        Assert(!strName.isEmpty() && !strPath.isEmpty());

        /* Update edited tree-widget item: */
        pItem->m_enmType = enmType;
        pItem->m_strName = strName;
        pItem->m_strPath = strPath;
        pItem->m_fAutoMount = dlgFolderDetails.isAutoMounted();
        pItem->m_fWritable = dlgFolderDetails.isWriteable();
        pItem->updateFields();

        /* Searching for a root of the edited tree-widget item: */
        SFTreeViewItem *pRoot = root(enmType);
        if (pItem->parentItem() != pRoot)
        {
            /* Move the tree-widget item to a new location: */
            pItem->parentItem()->takeChild(pItem->parentItem()->indexOfChild(pItem));
            pRoot->insertChild(pRoot->childCount(), pItem);

            /* Update tree-widget: */
            mTwFolders->scrollToItem(pItem);
            mTwFolders->setCurrentItem(pItem);
            sltHandleCurrentItemChange(pItem);
        }

        /* Sort tree-widget before adjusting: */
        mTwFolders->sortItems(0, Qt::AscendingOrder);
        /* Adjust tree-widget finally: */
        sltAdjustTree();
    }
}

void UIMachineSettingsSF::sltRemoveFolder()
{
    /* Check current folder item: */
    QTreeWidgetItem *pItem = mTwFolders->currentItem();
    AssertPtrReturnVoid(pItem);

    /* Delete corresponding item: */
    delete pItem;

    /* Adjust tree-widget finally: */
    sltAdjustTree();
}

void UIMachineSettingsSF::sltHandleCurrentItemChange(QTreeWidgetItem *pCurrentItem)
{
    if (pCurrentItem && pCurrentItem->parent() && !pCurrentItem->isSelected())
        pCurrentItem->setSelected(true);
    const bool fAddEnabled = pCurrentItem;
    const bool fRemoveEnabled = fAddEnabled && pCurrentItem->parent();
    m_pActionAdd->setEnabled(fAddEnabled);
    m_pActionEdit->setEnabled(fRemoveEnabled);
    m_pActionRemove->setEnabled(fRemoveEnabled);
}

void UIMachineSettingsSF::sltHandleDoubleClick(QTreeWidgetItem *pItem)
{
    const bool fEditEnabled = pItem && pItem->parent();
    if (fEditEnabled)
        sltEditFolder();
}

void UIMachineSettingsSF::sltHandleContextMenuRequest(const QPoint &position)
{
    QMenu menu;
    QTreeWidgetItem *pItem = mTwFolders->itemAt(position);
    if (mTwFolders->isEnabled() && pItem && pItem->flags() & Qt::ItemIsSelectable)
    {
        menu.addAction(m_pActionEdit);
        menu.addAction(m_pActionRemove);
    }
    else
    {
        menu.addAction(m_pActionAdd);
    }
    if (!menu.isEmpty())
        menu.exec(mTwFolders->viewport()->mapToGlobal(position));
}

void UIMachineSettingsSF::sltAdjustTree()
{
    /*
     * Calculates required columns sizes to max out column 2
     * and let all other columns stay at their minimum sizes.
     *
     * Columns
     * 0 = Tree view
     * 1 = Shared Folder name
     * 2 = Auto-mount flag
     * 3 = Writable flag
     */
    QAbstractItemView *pItemView = mTwFolders;
    QHeaderView *pItemHeader = mTwFolders->header();
    const int iTotal = mTwFolders->viewport()->width();

    const int mw0 = qMax(pItemView->sizeHintForColumn(0), pItemHeader->sectionSizeHint(0));
    const int mw2 = qMax(pItemView->sizeHintForColumn(2), pItemHeader->sectionSizeHint(2));
    const int mw3 = qMax(pItemView->sizeHintForColumn(3), pItemHeader->sectionSizeHint(3));

    const int w0 = mw0 < iTotal / 4 ? mw0 : iTotal / 4;
    const int w2 = mw2 < iTotal / 4 ? mw2 : iTotal / 4;
    const int w3 = mw3 < iTotal / 4 ? mw3 : iTotal / 4;

    /* Giving 1st column all the available space. */
    mTwFolders->setColumnWidth(0, w0);
    mTwFolders->setColumnWidth(1, iTotal - w0 - w2 - w3);
    mTwFolders->setColumnWidth(2, w2);
    mTwFolders->setColumnWidth(3, w3);
}

void UIMachineSettingsSF::sltAdjustTreeFields()
{
    QTreeWidgetItem *pMainRoot = mTwFolders->invisibleRootItem();
    for (int i = 0; i < pMainRoot->childCount(); ++i)
    {
        SFTreeViewItem *pSubRoot = static_cast<SFTreeViewItem*>(pMainRoot->child(i));
        pSubRoot->adjustText();
        for (int j = 0; j < pSubRoot->childCount(); ++j)
        {
            SFTreeViewItem *pItem = static_cast<SFTreeViewItem*>(pSubRoot->child(j));
            pItem->adjustText();
        }
    }
}

void UIMachineSettingsSF::prepare()
{
    /* Apply UI decorations: */
    Ui::UIMachineSettingsSF::setupUi(this);

    /* Prepare cache: */
    m_pCache = new UISettingsCacheSharedFolders;
    AssertPtrReturnVoid(m_pCache);

    /* Layout created in the .ui file. */
    {
        /* Prepare shared folders tree: */
        prepareFoldersTree();
        /* Prepare shared folders toolbar: */
        prepareFoldersToolbar();
        /* Prepare connections: */
        prepareConnections();
    }

    /* Apply language settings: */
    retranslateUi();
}

void UIMachineSettingsSF::prepareFoldersTree()
{
    /* Shared Folders tree-widget created in the .ui file. */
    AssertPtrReturnVoid(mTwFolders);
    {
        /* Configure tree-widget: */
        mTwFolders->header()->setSectionsMovable(false);
    }
}

void UIMachineSettingsSF::prepareFoldersToolbar()
{
    /* Shared Folders toolbar created in the .ui file. */
    AssertPtrReturnVoid(m_pFoldersToolBar);
    {
        /* Configure toolbar: */
        const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
        m_pFoldersToolBar->setIconSize(QSize(iIconMetric, iIconMetric));
        m_pFoldersToolBar->setOrientation(Qt::Vertical);

        /* Create 'Add Shared Folder' action: */
        m_pActionAdd = m_pFoldersToolBar->addAction(UIIconPool::iconSet(":/sf_add_16px.png",
                                                                        ":/sf_add_disabled_16px.png"),
                                                    QString(), this, SLOT(sltAddFolder()));
        AssertPtrReturnVoid(m_pActionAdd);
        {
            /* Configure action: */
            m_pActionAdd->setShortcuts(QList<QKeySequence>() << QKeySequence("Ins") << QKeySequence("Ctrl+N"));
        }

        /* Create 'Edit Shared Folder' action: */
        m_pActionEdit = m_pFoldersToolBar->addAction(UIIconPool::iconSet(":/sf_edit_16px.png",
                                                                         ":/sf_edit_disabled_16px.png"),
                                                     QString(), this, SLOT(sltEditFolder()));
        AssertPtrReturnVoid(m_pActionEdit);
        {
            /* Configure action: */
            m_pActionEdit->setShortcuts(QList<QKeySequence>() << QKeySequence("Space") << QKeySequence("F2"));
        }

        /* Create 'Remove Shared Folder' action: */
        m_pActionRemove = m_pFoldersToolBar->addAction(UIIconPool::iconSet(":/sf_remove_16px.png",
                                                                           ":/sf_remove_disabled_16px.png"),
                                                       QString(), this, SLOT(sltRemoveFolder()));
        AssertPtrReturnVoid(m_pActionRemove);
        {
            /* Configure action: */
            m_pActionRemove->setShortcuts(QList<QKeySequence>() << QKeySequence("Del") << QKeySequence("Ctrl+R"));
        }
    }
}

void UIMachineSettingsSF::prepareConnections()
{
    /* Configure tree-widget connections: */
    connect(mTwFolders, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            this, SLOT(sltHandleCurrentItemChange(QTreeWidgetItem *)));
    connect(mTwFolders, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),
            this, SLOT(sltHandleDoubleClick(QTreeWidgetItem *)));
    connect(mTwFolders, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(sltHandleContextMenuRequest(const QPoint &)));
}

void UIMachineSettingsSF::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

SFTreeViewItem *UIMachineSettingsSF::root(UISharedFolderType enmSharedFolderType)
{
    /* Search for the corresponding root item among all the top-level items: */
    SFTreeViewItem *pRootItem = 0;
    QTreeWidgetItem *pMainRootItem = mTwFolders->invisibleRootItem();
    for (int iFolderTypeIndex = 0; iFolderTypeIndex < pMainRootItem->childCount(); ++iFolderTypeIndex)
    {
        /* Get iterated item: */
        SFTreeViewItem *pIteratedItem = static_cast<SFTreeViewItem*>(pMainRootItem->child(iFolderTypeIndex));
        /* If iterated item type is what we are looking for: */
        if (pIteratedItem->m_enmType == enmSharedFolderType)
        {
            /* Remember the item: */
            pRootItem = static_cast<SFTreeViewItem*>(pIteratedItem);
            /* And break further search: */
            break;
        }
    }
    /* Return root item: */
    return pRootItem;
}

QStringList UIMachineSettingsSF::usedList(bool fIncludeSelected)
{
    /* Make the used names list: */
    QStringList list;
    QTreeWidgetItemIterator it(mTwFolders);
    while (*it)
    {
        if ((*it)->parent() && (fIncludeSelected || !(*it)->isSelected()))
            list << static_cast<SFTreeViewItem*>(*it)->getText(0);
        ++it;
    }
    return list;
}

bool UIMachineSettingsSF::isSharedFolderTypeSupported(UISharedFolderType enmSharedFolderType) const
{
    bool fIsSharedFolderTypeSupported = false;
    switch (enmSharedFolderType)
    {
        case MachineType:
            fIsSharedFolderTypeSupported = isMachineInValidMode();
            break;
        case ConsoleType:
            fIsSharedFolderTypeSupported = isMachineOnline();
            break;
        default:
            break;
    }
    return fIsSharedFolderTypeSupported;
}

void UIMachineSettingsSF::updateRootItemsVisibility()
{
    /* Update (show/hide) machine (permanent) root item: */
    setRootItemVisible(MachineType, isSharedFolderTypeSupported(MachineType));
    /* Update (show/hide) console (temporary) root item: */
    setRootItemVisible(ConsoleType, isSharedFolderTypeSupported(ConsoleType));
}

void UIMachineSettingsSF::setRootItemVisible(UISharedFolderType enmSharedFolderType, bool fVisible)
{
    /* Search for the corresponding root item among all the top-level items: */
    SFTreeViewItem *pRootItem = root(enmSharedFolderType);
    /* If root item, we are looking for, still not found: */
    if (!pRootItem)
    {
        /* Create new shared folder type item: */
        pRootItem = new SFTreeViewItem(mTwFolders, SFTreeViewItem::FormatType_EllipsisEnd);
        AssertPtrReturnVoid(pRootItem);
        {
            /* Configure item: */
            pRootItem->m_enmType = enmSharedFolderType;
            switch (enmSharedFolderType)
            {
                case MachineType: pRootItem->m_strName = tr(" Machine Folders"); break;
                case ConsoleType: pRootItem->m_strName = tr(" Transient Folders"); break;
                default: break;
            }
            pRootItem->updateFields();
        }
    }
    /* Expand/collaps it if necessary: */
    pRootItem->setExpanded(fVisible);
    /* And hide/show it if necessary: */
    pRootItem->setHidden(!fVisible);
}

void UIMachineSettingsSF::addSharedFolderItem(const UIDataSettingsSharedFolder &sharedFolderData, bool fChoose)
{
    /* Create shared folder item: */
    SFTreeViewItem *pItem = new SFTreeViewItem(root(sharedFolderData.m_enmType), SFTreeViewItem::FormatType_EllipsisFile);
    AssertPtrReturnVoid(pItem);
    {
        /* Configure item: */
        pItem->m_enmType = sharedFolderData.m_enmType;
        pItem->m_strName = sharedFolderData.m_strName;
        pItem->m_strPath = sharedFolderData.m_strPath;
        pItem->m_fAutoMount = sharedFolderData.m_fAutoMount;
        pItem->m_fWritable = sharedFolderData.m_fWritable;
        pItem->updateFields();

        /* Select this item if it's new: */
        if (fChoose)
        {
            mTwFolders->scrollToItem(pItem);
            mTwFolders->setCurrentItem(pItem);
            sltHandleCurrentItemChange(pItem);
        }
    }
}

CSharedFolderVector UIMachineSettingsSF::getSharedFolders(UISharedFolderType enmFoldersType)
{
    /* Wrap up the getter below: */
    CSharedFolderVector folders;
    getSharedFolders(enmFoldersType, folders);
    return folders;
}

bool UIMachineSettingsSF::getSharedFolders(UISharedFolderType enmFoldersType, CSharedFolderVector &folders)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Load folders of passed type: */
    if (fSuccess)
    {
        /* Make sure folder type is supported: */
        AssertReturn(isSharedFolderTypeSupported(enmFoldersType), false);
        switch (enmFoldersType)
        {
            case MachineType:
            {
                /* Make sure machine was specified: */
                AssertReturn(!m_machine.isNull(), false);
                /* Load machine folders: */
                folders = m_machine.GetSharedFolders();
                fSuccess = m_machine.isOk();

                /* Show error message if necessary: */
                if (!fSuccess)
                    notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));

                break;
            }
            case ConsoleType:
            {
                /* Make sure console was specified: */
                AssertReturn(!m_console.isNull(), false);
                /* Load console folders: */
                folders = m_console.GetSharedFolders();
                fSuccess = m_console.isOk();

                /* Show error message if necessary: */
                if (!fSuccess)
                    notifyOperationProgressError(UIErrorString::formatErrorInfo(m_console));

                break;
            }
            default:
                AssertFailedReturn(false);
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsSF::getSharedFolder(const QString &strFolderName, const CSharedFolderVector &folders, CSharedFolder &comFolder)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Look for a folder with passed name: */
    for (int iFolderIndex = 0; fSuccess && iFolderIndex < folders.size(); ++iFolderIndex)
    {
        /* Get current folder: */
        const CSharedFolder &comCurrentFolder = folders.at(iFolderIndex);

        /* Get current folder name for further activities: */
        QString strCurrentFolderName;
        if (fSuccess)
        {
            strCurrentFolderName = comCurrentFolder.GetName();
            fSuccess = comCurrentFolder.isOk();
        }

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(comCurrentFolder));

        /* If that's the folder we are looking for => take it: */
        if (fSuccess && strCurrentFolderName == strFolderName)
            comFolder = folders[iFolderIndex];
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsSF::saveFoldersData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save folders settings from the cache: */
    if (fSuccess && isMachineInValidMode() && m_pCache->wasChanged())
    {
        /* For each folder: */
        for (int iFolderIndex = 0; fSuccess && iFolderIndex < m_pCache->childCount(); ++iFolderIndex)
        {
            /* Get folder cache: */
            const UISettingsCacheSharedFolder &folderCache = m_pCache->child(iFolderIndex);

            /* Remove folder marked for 'remove' or 'update': */
            if (fSuccess && (folderCache.wasRemoved() || folderCache.wasUpdated()))
                fSuccess = removeSharedFolder(folderCache);

            /* Create folder marked for 'create' or 'update': */
            if (fSuccess && (folderCache.wasCreated() || folderCache.wasUpdated()))
                fSuccess = createSharedFolder(folderCache);
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsSF::removeSharedFolder(const UISettingsCacheSharedFolder &folderCache)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Remove folder: */
    if (fSuccess)
    {
        /* Get folder data: */
        const UIDataSettingsSharedFolder &newFolderData = folderCache.base();
        const UISharedFolderType enmFoldersType = newFolderData.m_enmType;
        const QString strFolderName = newFolderData.m_strName;

        /* Get current folders: */
        CSharedFolderVector folders;
        if (fSuccess)
            fSuccess = getSharedFolders(enmFoldersType, folders);

        /* Search for a folder with the same name: */
        CSharedFolder comFolder;
        if (fSuccess)
            fSuccess = getSharedFolder(strFolderName, folders, comFolder);

        /* Make sure such folder really exists: */
        if (fSuccess && !comFolder.isNull())
        {
            /* Remove existing folder: */
            switch (enmFoldersType)
            {
                case MachineType:
                {
                    /* Remove existing folder: */
                    m_machine.RemoveSharedFolder(strFolderName);
                    /* Check that machine is OK: */
                    fSuccess = m_machine.isOk();
                    if (!fSuccess)
                    {
                        /* Show error message: */
                        notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
                    }
                    break;
                }
                case ConsoleType:
                {
                    /* Remove existing folder: */
                    m_console.RemoveSharedFolder(strFolderName);
                    /* Check that console is OK: */
                    fSuccess = m_console.isOk();
                    if (!fSuccess)
                    {
                        /* Show error message: */
                        notifyOperationProgressError(UIErrorString::formatErrorInfo(m_console));
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsSF::createSharedFolder(const UISettingsCacheSharedFolder &folderCache)
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Create folder: */
    if (fSuccess)
    {
        /* Get folder data: */
        const UIDataSettingsSharedFolder &newFolderData = folderCache.data();
        const UISharedFolderType enmFoldersType = newFolderData.m_enmType;
        const QString strFolderName = newFolderData.m_strName;
        const QString strFolderPath = newFolderData.m_strPath;
        const bool fIsAutoMount = newFolderData.m_fAutoMount;
        const bool fIsWritable = newFolderData.m_fWritable;

        /* Get current folders: */
        CSharedFolderVector folders;
        if (fSuccess)
            fSuccess = getSharedFolders(enmFoldersType, folders);

        /* Search for a folder with the same name: */
        CSharedFolder comFolder;
        if (fSuccess)
            fSuccess = getSharedFolder(strFolderName, folders, comFolder);

        /* Make sure such folder doesn't exist: */
        if (fSuccess && comFolder.isNull())
        {
            /* Create new folder: */
            switch (enmFoldersType)
            {
                case MachineType:
                {
                    /* Create new folder: */
                    m_machine.CreateSharedFolder(strFolderName, strFolderPath, fIsWritable, fIsAutoMount);
                    /* Check that machine is OK: */
                    fSuccess = m_machine.isOk();
                    if (!fSuccess)
                    {
                        /* Show error message: */
                        notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
                    }
                    break;
                }
                case ConsoleType:
                {
                    /* Create new folder: */
                    m_console.CreateSharedFolder(strFolderName, strFolderPath, fIsWritable, fIsAutoMount);
                    /* Check that console is OK: */
                    fSuccess = m_console.isOk();
                    if (!fSuccess)
                    {
                        /* Show error message: */
                        notifyOperationProgressError(UIErrorString::formatErrorInfo(m_console));
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
    /* Return result: */
    return fSuccess;
}

