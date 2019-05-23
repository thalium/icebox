/* $Id: UIGlobalSettingsExtension.cpp $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsExtension class implementation.
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
# include <QHeaderView>

/* GUI includes: */
# include "QIFileDialog.h"
# include "UIGlobalSettingsExtension.h"
# include "UIIconPool.h"
# include "UIMessageCenter.h"
# include "VBoxGlobal.h"
# include "VBoxLicenseViewer.h"

/* COM includes: */
# include "CExtPack.h"
# include "CExtPackFile.h"
# include "CExtPackManager.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Global settings: Extension page item data structure. */
struct UIDataSettingsGlobalExtensionItem
{
    /** Constructs data. */
    UIDataSettingsGlobalExtensionItem()
        : m_strName(QString())
        , m_strDescription(QString())
        , m_strVersion(QString())
        , m_uRevision(0)
        , m_fIsUsable(false)
        , m_strWhyUnusable(QString())
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsGlobalExtensionItem &other) const
    {
        return true
               && (m_strName == other.m_strName)
               && (m_strDescription == other.m_strDescription)
               && (m_strVersion == other.m_strVersion)
               && (m_uRevision == other.m_uRevision)
               && (m_fIsUsable == other.m_fIsUsable)
               && (m_strWhyUnusable == other.m_strWhyUnusable)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsGlobalExtensionItem &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsGlobalExtensionItem &other) const { return !equal(other); }

    /** Holds the extension item name. */
    QString m_strName;
    /** Holds the extension item description. */
    QString m_strDescription;
    /** Holds the extension item version. */
    QString m_strVersion;
    /** Holds the extension item revision. */
    ULONG m_uRevision;
    /** Holds whether the extension item usable. */
    bool m_fIsUsable;
    /** Holds why the extension item is unusable. */
    QString m_strWhyUnusable;
};


/** Global settings: Extension page data structure. */
struct UIDataSettingsGlobalExtension
{
    /** Constructs data. */
    UIDataSettingsGlobalExtension()
        : m_items(QList<UIDataSettingsGlobalExtensionItem>())
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsGlobalExtension &other) const
    {
        return true
               && (m_items == other.m_items)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsGlobalExtension &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsGlobalExtension &other) const { return !equal(other); }

    /** Holds the extension items. */
    QList<UIDataSettingsGlobalExtensionItem> m_items;
};


/* Extension package item: */
class UIExtensionPackageItem : public QITreeWidgetItem
{
public:

    /* Extension package item constructor: */
    UIExtensionPackageItem(QITreeWidget *pParent, const UIDataSettingsGlobalExtensionItem &data)
        : QITreeWidgetItem(pParent)
        , m_data(data)
    {
        /* Icon: */
        setIcon(0, UIIconPool::iconSet(m_data.m_fIsUsable ?
                                       ":/status_check_16px.png" :
                                       ":/status_error_16px.png"));

        /* Name: */
        setText(1, m_data.m_strName);

        /* Version, Revision, Edition: */
        QString strVersion(m_data.m_strVersion.section(QRegExp("[-_]"), 0, 0));
        QString strAppend;
        /* workaround for http://qt.gitorious.org/qt/qt/commit/7fc63dd0ff368a637dcd17e692b9d6b26278b538 */
        if (m_data.m_strVersion.contains(QRegExp("[-_]")))
            strAppend = m_data.m_strVersion.section(QRegExp("[-_]"), 1, -1, QString::SectionIncludeLeadingSep);
        setText(2, QString("%1r%2%3").arg(strVersion).arg(m_data.m_uRevision).arg(strAppend));

        /* Tool-tip: */
        QString strTip = m_data.m_strDescription;
        if (!m_data.m_fIsUsable)
        {
            strTip += QString("<hr>");
            strTip += m_data.m_strWhyUnusable;
        }
        setToolTip(0, strTip);
        setToolTip(1, strTip);
        setToolTip(2, strTip);
    }

    QString name() const { return m_data.m_strName; }

    /** Returns default text. */
    virtual QString defaultText() const /* override */
    {
        return m_data.m_fIsUsable ?
               tr("%1, %2: %3, %4", "col.2 text, col.3 name: col.3 text, col.1 name")
                 .arg(text(1))
                 .arg(parentTree()->headerItem()->text(2)).arg(text(2))
                 .arg(parentTree()->headerItem()->text(0)) :
               tr("%1, %2: %3",     "col.2 text, col.3 name: col.3 text")
                 .arg(text(1))
                 .arg(parentTree()->headerItem()->text(2)).arg(text(2));
    }

private:

    UIDataSettingsGlobalExtensionItem m_data;
};


UIGlobalSettingsExtension::UIGlobalSettingsExtension()
    : m_pActionAdd(0), m_pActionRemove(0)
    , m_pCache(0)
{
    /* Prepare: */
    prepare();
}

UIGlobalSettingsExtension::~UIGlobalSettingsExtension()
{
    /* Cleanup: */
    cleanup();
}

/* static */
void UIGlobalSettingsExtension::doInstallation(QString const &strFilePath, QString const &strDigest,
                                               QWidget *pParent, QString *pstrExtPackName)
{
    /* Open the extpack tarball via IExtPackManager: */
    CExtPackManager manager = vboxGlobal().virtualBox().GetExtensionPackManager();
    CExtPackFile extPackFile;
    if (strDigest.isEmpty())
        extPackFile = manager.OpenExtPackFile(strFilePath);
    else
    {
        QString strFileAndHash = QString("%1::SHA-256=%2").arg(strFilePath).arg(strDigest);
        extPackFile = manager.OpenExtPackFile(strFileAndHash);
    }
    if (!manager.isOk())
    {
        msgCenter().cannotOpenExtPack(strFilePath, manager, pParent);
        return;
    }

    if (!extPackFile.GetUsable())
    {
        msgCenter().warnAboutBadExtPackFile(strFilePath, extPackFile, pParent);
        return;
    }

    const QString strPackName = extPackFile.GetName();
    const QString strPackDescription = extPackFile.GetDescription();
    const QString strPackVersion = QString("%1r%2%3").arg(extPackFile.GetVersion()).arg(extPackFile.GetRevision()).arg(extPackFile.GetEdition());

    /* Check if there is a version of the extension pack already
     * installed on the system and let the user decide what to do about it. */
    CExtPack extPackCur = manager.Find(strPackName);
    bool fReplaceIt = extPackCur.isOk();
    if (fReplaceIt)
    {
        QString strPackVersionCur = QString("%1r%2%3").arg(extPackCur.GetVersion()).arg(extPackCur.GetRevision()).arg(extPackCur.GetEdition());
        if (!msgCenter().confirmReplaceExtensionPack(strPackName, strPackVersion, strPackVersionCur, strPackDescription, pParent))
            return;
    }
    /* If it's a new package just ask for general confirmation. */
    else
    {
        if (!msgCenter().confirmInstallExtensionPack(strPackName, strPackVersion, strPackDescription, pParent))
            return;
    }

    /* Display the license dialog if required by the extension pack. */
    if (extPackFile.GetShowLicense())
    {
        QString strLicense = extPackFile.GetLicense();
        VBoxLicenseViewer licenseViewer(pParent);
        if (licenseViewer.showLicenseFromString(strLicense) != QDialog::Accepted)
            return;
    }

    /* Install the selected package.
     * Set the package name return value before doing
     * this as the caller should do a refresh even on failure. */
    QString displayInfo;
#ifdef VBOX_WS_WIN
    if (pParent)
        displayInfo.sprintf("hwnd=%#llx", (uint64_t)(uintptr_t)pParent->winId());
#endif
    /* Prepare installation progress: */
    CProgress progress = extPackFile.Install(fReplaceIt, displayInfo);
    if (extPackFile.isOk())
    {
        /* Show installation progress: */
        msgCenter().showModalProgressDialog(progress, tr("Extensions"), ":/progress_install_guest_additions_90px.png", pParent);
        if (!progress.GetCanceled())
        {
            if (progress.isOk() && progress.GetResultCode() == 0)
                msgCenter().warnAboutExtPackInstalled(strPackName, pParent);
            else
                msgCenter().cannotInstallExtPack(progress, strFilePath, pParent);
        }
    }
    else
        msgCenter().cannotInstallExtPack(extPackFile, strFilePath, pParent);

    if (pstrExtPackName)
        *pstrExtPackName = strPackName;
}

void UIGlobalSettingsExtension::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to properties: */
    UISettingsPageGlobal::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old extension data: */
    UIDataSettingsGlobalExtension oldExtensionData;

    /* Gather old extension data: */
    const CExtPackVector &packages = vboxGlobal().virtualBox().
                                     GetExtensionPackManager().GetInstalledExtPacks();
    foreach (const CExtPack &package, packages)
    {
        UIDataSettingsGlobalExtensionItem item;
        loadData(package, item);
        oldExtensionData.m_items << item;
    }

    /* Cache old extension data: */
    m_pCache->cacheInitialData(oldExtensionData);

    /* Upload properties to data: */
    UISettingsPageGlobal::uploadData(data);
}

void UIGlobalSettingsExtension::getFromCache()
{
    /* Get old extension data from the cache: */
    const UIDataSettingsGlobalExtension &oldExtensionData = m_pCache->base();

    /* Load old extension data from the cache: */
    foreach (const UIDataSettingsGlobalExtensionItem &item, oldExtensionData.m_items)
        new UIExtensionPackageItem(m_pPackagesTree, item);
    /* If at least one item present: */
    if (m_pPackagesTree->topLevelItemCount())
        m_pPackagesTree->setCurrentItem(m_pPackagesTree->topLevelItem(0));
    /* Update action's availability: */
    sltHandleCurrentItemChange(m_pPackagesTree->currentItem());
}

void UIGlobalSettingsExtension::putToCache()
{
    /* Nothing to cache... */
}

void UIGlobalSettingsExtension::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to properties: */
    UISettingsPageGlobal::fetchData(data);

    /* Nothing to save from the cache... */

    /* Upload properties to data: */
    UISettingsPageGlobal::uploadData(data);
}

void UIGlobalSettingsExtension::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIGlobalSettingsExtension::retranslateUi(this);

    /* Translate actions: */
    m_pActionAdd->setText(tr("Add Package"));
    m_pActionRemove->setText(tr("Remove Package"));

    m_pActionAdd->setWhatsThis(tr("Adds new package."));
    m_pActionRemove->setWhatsThis(tr("Removes selected package."));

    m_pActionAdd->setToolTip(m_pActionAdd->whatsThis());
    m_pActionRemove->setToolTip(m_pActionRemove->whatsThis());
}

void UIGlobalSettingsExtension::sltHandleCurrentItemChange(QTreeWidgetItem *pCurrentItem)
{
    /* Check action's availability: */
    m_pActionRemove->setEnabled(pCurrentItem);
}

void UIGlobalSettingsExtension::sltHandleContextMenuRequest(const QPoint &position)
{
    QMenu menu;
    if (m_pPackagesTree->itemAt(position))
    {
        menu.addAction(m_pActionAdd);
        menu.addAction(m_pActionRemove);
    }
    else
    {
        menu.addAction(m_pActionAdd);
    }
    menu.exec(m_pPackagesTree->viewport()->mapToGlobal(position));
}

void UIGlobalSettingsExtension::sltAddPackage()
{
    /* Open file-open window to let user to choose package file.
     * The default location is the user's Download or Downloads directory with
     * the user's home directory as a fallback. ExtPacks are downloaded. */
    QString strBaseFolder = QDir::homePath() + "/Downloads";
    if (!QDir(strBaseFolder).exists())
    {
        strBaseFolder = QDir::homePath() + "/Download";
        if (!QDir(strBaseFolder).exists())
            strBaseFolder = QDir::homePath();
    }
    const QString strTitle = tr("Select an extension package file");
    QStringList extensions;
    for (int i = 0; i < VBoxExtPackFileExts.size(); ++i)
        extensions << QString("*.%1").arg(VBoxExtPackFileExts[i]);
    const QString strFilter = tr("Extension package files (%1)").arg(extensions.join(" "));

    const QStringList fileNames = QIFileDialog::getOpenFileNames(strBaseFolder, strFilter, this, strTitle, 0, true, true);

    QString strFilePath;
    if (!fileNames.isEmpty())
        strFilePath = fileNames.at(0);

    /* Install the chosen package: */
    if (!strFilePath.isEmpty())
    {
        QString strExtPackName;
        doInstallation(strFilePath, QString(), this, &strExtPackName);

        /* Since we might be reinstalling an existing package, we have to
         * do a little refreshing regardless of what the user chose. */
        if (!strExtPackName.isNull())
        {
            /* Remove it from the cache: */
            for (int i = 0; i < m_pCache->data().m_items.size(); ++i)
            {
                if (!strExtPackName.compare(m_pCache->data().m_items.at(i).m_strName, Qt::CaseInsensitive))
                {
                    m_pCache->data().m_items.removeAt(i);
                    break;
                }
            }

            /* Remove it from the tree: */
            const int cItems = m_pPackagesTree->topLevelItemCount();
            for (int i = 0; i < cItems; i++)
            {
                UIExtensionPackageItem *pItem = static_cast<UIExtensionPackageItem*>(m_pPackagesTree->topLevelItem(i));
                if (!strExtPackName.compare(pItem->name(), Qt::CaseInsensitive))
                {
                    delete pItem;
                    break;
                }
            }

            /* Reinsert it into the cache and tree: */
            CExtPackManager manager = vboxGlobal().virtualBox().GetExtensionPackManager();
            const CExtPack &package = manager.Find(strExtPackName);
            if (package.isOk())
            {
                UIDataSettingsGlobalExtensionItem item;
                loadData(package, item);
                m_pCache->data().m_items << item;

                UIExtensionPackageItem *pItem = new UIExtensionPackageItem(m_pPackagesTree, m_pCache->data().m_items.last());
                m_pPackagesTree->setCurrentItem(pItem);
                m_pPackagesTree->sortByColumn(1, Qt::AscendingOrder);
            }
        }
    }
}

void UIGlobalSettingsExtension::sltRemovePackage()
{
    /* Get current item: */
    UIExtensionPackageItem *pItem = m_pPackagesTree &&
                                    m_pPackagesTree->currentItem() ?
                                    static_cast<UIExtensionPackageItem*>(m_pPackagesTree->currentItem()) : 0;

    /* Uninstall chosen package: */
    if (pItem)
    {
        /* Get name of current package: */
        const QString strSelectedPackageName = pItem->name();
        /* Ask the user about package removing: */
        if (msgCenter().confirmRemoveExtensionPack(strSelectedPackageName, this))
        {
            /* Uninstall the package: */
            CExtPackManager manager = vboxGlobal().virtualBox().GetExtensionPackManager();
            /** @todo Refuse this if any VMs are running. */
            QString displayInfo;
#ifdef VBOX_WS_WIN
            displayInfo.sprintf("hwnd=%#llx", (uint64_t)(uintptr_t)this->winId());
#endif /* VBOX_WS_WIN */
            /* Prepare uninstallation progress: */
            CProgress progress = manager.Uninstall(strSelectedPackageName, false /* forced removal? */, displayInfo);
            if (manager.isOk())
            {
                /* Show uninstallation progress: */
                msgCenter().showModalProgressDialog(progress, tr("Extensions"), ":/progress_install_guest_additions_90px.png", this);
                if (progress.isOk() && progress.GetResultCode() == 0)
                {
                    /* Remove selected package from the cache: */
                    for (int i = 0; i < m_pCache->data().m_items.size(); ++i)
                    {
                        if (!strSelectedPackageName.compare(m_pCache->data().m_items.at(i).m_strName, Qt::CaseInsensitive))
                        {
                            m_pCache->data().m_items.removeAt(i);
                            break;
                        }
                    }
                    /* Remove selected package from tree: */
                    delete pItem;
                }
                else
                    msgCenter().cannotUninstallExtPack(progress, strSelectedPackageName, this);
            }
            else
                msgCenter().cannotUninstallExtPack(manager, strSelectedPackageName, this);
        }
    }
}

void UIGlobalSettingsExtension::prepare()
{
    /* Apply UI decorations: */
    Ui::UIGlobalSettingsExtension::setupUi(this);

    /* Prepare cache: */
    m_pCache = new UISettingsCacheGlobalExtension;
    AssertPtrReturnVoid(m_pCache);

    /* Layout created in the .ui file. */
    {
        /* Tree-widget created in the .ui file. */
        AssertPtrReturnVoid(m_pPackagesTree);
        {
            /* Configure tree-widget: */
            m_pPackagesTree->header()->setStretchLastSection(false);
            m_pPackagesTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
            m_pPackagesTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
            m_pPackagesTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
            m_pPackagesTree->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(m_pPackagesTree, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
                    this, SLOT(sltHandleCurrentItemChange(QTreeWidgetItem *)));
            connect(m_pPackagesTree, SIGNAL(customContextMenuRequested(const QPoint &)),
                    this, SLOT(sltHandleContextMenuRequest(const QPoint &)));
        }

        /* Tool-bar created in the .ui file. */
        AssertPtrReturnVoid(m_pPackagesToolbar);
        {
            /* Configure toolbar: */
            const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
            m_pPackagesToolbar->setOrientation(Qt::Vertical);
            m_pPackagesToolbar->setIconSize(QSize(iIconMetric, iIconMetric));

            /* Create 'Add Package' action: */
            m_pActionAdd = m_pPackagesToolbar->addAction(UIIconPool::iconSet(":/extension_pack_install_16px.png",
                                                                             ":/extension_pack_install_disabled_16px.png"),
                                                         QString(), this, SLOT(sltAddPackage()));

            /* Create 'Remove Package' action: */
            m_pActionRemove = m_pPackagesToolbar->addAction(UIIconPool::iconSet(":/extension_pack_uninstall_16px.png",
                                                                                ":/extension_pack_uninstall_disabled_16px.png"),
                                                            QString(), this, SLOT(sltRemovePackage()));
        }
    }

    /* Apply language settings: */
    retranslateUi();
}

void UIGlobalSettingsExtension::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

void UIGlobalSettingsExtension::loadData(const CExtPack &package, UIDataSettingsGlobalExtensionItem &item) const
{
    item.m_strName = package.GetName();
    item.m_strDescription = package.GetDescription();
    item.m_strVersion = package.GetVersion();
    item.m_uRevision = package.GetRevision();
    item.m_fIsUsable = package.GetUsable();
    if (!item.m_fIsUsable)
        item.m_strWhyUnusable = package.GetWhyUnusable();
}

