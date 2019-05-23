/* $Id: UIMediumManager.cpp $ */
/** @file
 * VBox Qt GUI - UIMediumManager class implementation.
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

/* Qt includes: */
# include <QFrame>
# include <QLabel>
# include <QMenuBar>
# include <QHeaderView>
# include <QPushButton>
# include <QProgressBar>

/* GUI includes: */
# include "VBoxGlobal.h"
# include "UIExtraDataManager.h"
# include "UIMediumDetailsWidget.h"
# include "UIMediumManager.h"
# include "UIWizardCloneVD.h"
# include "UIMessageCenter.h"
# include "QIFileDialog.h"
# include "QITabWidget.h"
# include "QITreeWidget.h"
# include "QILabel.h"
# include "QIDialogButtonBox.h"
# include "UIToolBar.h"
# include "UIIconPool.h"
# include "UIMedium.h"

/* COM includes: */
# include "COMEnums.h"
# include "CMachine.h"
# include "CMediumFormat.h"
# include "CStorageController.h"
# include "CMediumAttachment.h"

# ifdef VBOX_WS_MAC
#  include "UIWindowMenuManager.h"
# endif /* VBOX_WS_MAC */

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** QITreeWidgetItem extension representing Media Manager item. */
class UIMediumItem : public QITreeWidgetItem, public UIDataMedium
{
public:

    /** Constructs top-level item.
      * @param  guiMedium  Brings the medium to wrap around.
      * @param  pParent    Brings the parent tree reference. */
    UIMediumItem(const UIMedium &guiMedium, QITreeWidget *pParent);
    /** Constructs sub-level item.
      * @param  guiMedium  Brings the medium to wrap around.
      * @param  pParent    Brings the parent item reference. */
    UIMediumItem(const UIMedium &guiMedium, UIMediumItem *pParent);

    /** Copies UIMedium wrapped by <i>this</i> item. */
    virtual bool copy();
    /** Moves UIMedium wrapped by <i>this</i> item. */
    virtual bool move();
    /** Removes UIMedium wrapped by <i>this</i> item. */
    virtual bool remove() = 0;
    /** Releases UIMedium wrapped by <i>this</i> item.
      * @param  fInduced  Brings whether this action is caused by other user's action,
      *                   not a direct order to release particularly selected medium. */
    virtual bool release(bool fInduced = false);

    /** Refreshes item fully. */
    void refreshAll();

    /** Returns UIMedium wrapped by <i>this</i> item. */
    const UIMedium &medium() const { return m_guiMedium; }
    /** Defines UIMedium wrapped by <i>this</i> item. */
    void setMedium(const UIMedium &guiMedium);

    /** Returns UIMediumType of the wrapped UIMedium. */
    UIMediumType mediumType() const { return m_guiMedium.type(); }

    /** Returns KMediumState of the wrapped UIMedium. */
    KMediumState state() const { return m_guiMedium.state(); }

    /** Returns QString <i>ID</i> of the wrapped UIMedium. */
    QString id() const { return m_guiMedium.id(); }
    /** Returns QString <i>location</i> of the wrapped UIMedium. */
    QString location() const { return m_guiMedium.location(); }

    /** Returns QString <i>hard-disk format</i> of the wrapped UIMedium. */
    QString hardDiskFormat() const { return m_guiMedium.hardDiskFormat(); }
    /** Returns QString <i>hard-disk type</i> of the wrapped UIMedium. */
    QString hardDiskType() const { return m_guiMedium.hardDiskType(); }

    /** Returns QString <i>storage details</i> of the wrapped UIMedium. */
    QString details() const { return m_guiMedium.storageDetails(); }
    /** Returns QString <i>encryption password ID</i> of the wrapped UIMedium. */
    QString encryptionPasswordID() const { return m_guiMedium.encryptionPasswordID(); }

    /** Returns QString <i>tool-tip</i> of the wrapped UIMedium. */
    QString toolTip() const { return m_guiMedium.toolTip(); }

    /** Returns a vector of IDs of all machines wrapped UIMedium is attached to. */
    const QList<QString> &machineIds() const { return m_guiMedium.machineIds(); }
    /** Returns QString <i>usage</i> of the wrapped UIMedium. */
    QString usage() const { return m_guiMedium.usage(); }
    /** Returns whether wrapped UIMedium is used. */
    bool isUsed() const { return m_guiMedium.isUsed(); }
    /** Returns whether wrapped UIMedium is used in snapshots. */
    bool isUsedInSnapshots() const { return m_guiMedium.isUsedInSnapshots(); }

    /** Returns whether <i>this</i> item is less than @a other one. */
    bool operator<(const QTreeWidgetItem &other) const;

protected:

    /** Release UIMedium wrapped by <i>this</i> item from virtual @a comMachine. */
    virtual bool releaseFrom(CMachine comMachine) = 0;

    /** Returns default text. */
    virtual QString defaultText() const /* override */;

private:

    /** Refreshes item information such as icon, text and tool-tip. */
    void refresh();

    /** Releases UIMedium wrapped by <i>this</i> item from virtual machine with @a strMachineId. */
    bool releaseFrom(const QString &strMachineId);

    /** Formats field text. */
    static QString formatFieldText(const QString &strText, bool fCompact = true, const QString &strElipsis = "middle");

    /** Holds the UIMedium wrapped by <i>this</i> item. */
    UIMedium m_guiMedium;
};


/** UIMediumItem extension representing hard-disk item. */
class UIMediumItemHD : public UIMediumItem
{
public:

    /** Constructs top-level item.
      * @param  guiMedium  Brings the medium to wrap around.
      * @param  pParent    Brings the parent tree reference. */
    UIMediumItemHD(const UIMedium &guiMedium, QITreeWidget *pParent);
    /** Constructs sub-level item.
      * @param  guiMedium  Brings the medium to wrap around.
      * @param  pParent    Brings the parent item reference. */
    UIMediumItemHD(const UIMedium &guiMedium, UIMediumItem *pParent);

protected:

    /** Removes UIMedium wrapped by <i>this</i> item. */
    virtual bool remove() /* override */;
    /** Releases UIMedium wrapped by <i>this</i> item from virtual @a comMachine. */
    virtual bool releaseFrom(CMachine comMachine) /* override */;

private:

    /** Proposes user to remove CMedium storage wrapped by <i>this</i> item. */
    bool maybeRemoveStorage();
};

/** UIMediumItem extension representing optical-disk item. */
class UIMediumItemCD : public UIMediumItem
{
public:

    /** Constructs top-level item.
      * @param  guiMedium  Brings the medium to wrap around.
      * @param  pParent    Brings the parent tree reference. */
    UIMediumItemCD(const UIMedium &guiMedium, QITreeWidget *pParent);

protected:

    /** Removes UIMedium wrapped by <i>this</i> item. */
    virtual bool remove() /* override */;
    /** Releases UIMedium wrapped by <i>this</i> item from virtual @a comMachine. */
    virtual bool releaseFrom(CMachine comMachine) /* override */;
};

/** UIMediumItem extension representing floppy-disk item. */
class UIMediumItemFD : public UIMediumItem
{
public:

    /** Constructs top-level item.
      * @param  guiMedium  Brings the medium to wrap around.
      * @param  pParent    Brings the parent tree reference. */
    UIMediumItemFD(const UIMedium &guiMedium, QITreeWidget *pParent);

protected:

    /** Removes UIMedium wrapped by <i>this</i> item. */
    virtual bool remove() /* override */;
    /** Releases UIMedium wrapped by <i>this</i> item from virtual @a comMachine. */
    virtual bool releaseFrom(CMachine comMachine) /* override */;
};


/** Functor allowing to check if passed UIMediumItem is suitable by @a strID. */
class CheckIfSuitableByID : public CheckIfSuitableBy
{
public:
    /** Constructor accepting @a strID to compare with. */
    CheckIfSuitableByID(const QString &strID) : m_strID(strID) {}

private:
    /** Determines whether passed UIMediumItem is suitable by @a strID. */
    bool isItSuitable(UIMediumItem *pItem) const { return pItem->id() == m_strID; }
    /** Holds the @a strID to compare to. */
    QString m_strID;
};

/** Functor allowing to check if passed UIMediumItem is suitable by @a state. */
class CheckIfSuitableByState : public CheckIfSuitableBy
{
public:
    /** Constructor accepting @a state to compare with. */
    CheckIfSuitableByState(KMediumState state) : m_state(state) {}

private:
    /** Determines whether passed UIMediumItem is suitable by @a state. */
    bool isItSuitable(UIMediumItem *pItem) const { return pItem->state() == m_state; }
    /** Holds the @a state to compare to. */
    KMediumState m_state;
};


/*********************************************************************************************************************************
*   Class UIMediumItem implementation.                                                                                           *
*********************************************************************************************************************************/

UIMediumItem::UIMediumItem(const UIMedium &guiMedium, QITreeWidget *pParent)
    : QITreeWidgetItem(pParent)
    , m_guiMedium(guiMedium)
{
    refresh();
}

UIMediumItem::UIMediumItem(const UIMedium &guiMedium, UIMediumItem *pParent)
    : QITreeWidgetItem(pParent)
    , m_guiMedium(guiMedium)
{
    refresh();
}

bool UIMediumItem::move()
{
    /* Open file-save dialog to choose location for current medium: */
    const QString strFileName = QIFileDialog::getSaveFileName(location(),
                                                              UIMediumManager::tr("Current extension (*.%1)")
                                                                 .arg(QFileInfo(location()).suffix()),
                                                              treeWidget(),
                                                              UIMediumManager::tr("Choose the location of this medium"),
                                                              0, true, true);
    /* Negative if nothing changed: */
    if (strFileName.isNull())
        return false;

    /* Search for corresponding medium: */
    CMedium comMedium = medium().medium();

    /* Try to assign new medium location: */
    if (   comMedium.isOk()
        && strFileName != location())
    {
        /* Prepare move storage progress: */
        CProgress comProgress = comMedium.SetLocation(strFileName);

        /* Show error message if necessary: */
        if (!comMedium.isOk())
        {
            msgCenter().cannotMoveMediumStorage(comMedium, location(),
                                                strFileName, treeWidget());
            /* Negative if failed: */
            return false;
        }
        else
        {
            /* Show move storage progress: */
            msgCenter().showModalProgressDialog(comProgress, UIMediumManager::tr("Moving medium..."),
                                                ":/progress_media_move_90px.png", treeWidget());

            /* Show error message if necessary: */
            if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
            {
                msgCenter().cannotMoveMediumStorage(comProgress, location(),
                                                    strFileName, treeWidget());
                /* Negative if failed: */
                return false;
            }
        }
    }

    /* Recache item: */
    refreshAll();

    /* Positive: */
    return true;
}

bool UIMediumItem::copy()
{
    /* Show Clone VD wizard: */
    UISafePointerWizard pWizard = new UIWizardCloneVD(treeWidget(), medium().medium());
    pWizard->prepare();
    pWizard->exec();

    /* Delete if still exists: */
    if (pWizard)
        delete pWizard;

    /* True by default: */
    return true;
}

bool UIMediumItem::release(bool fInduced /* = false */)
{
    /* Refresh medium and item: */
    m_guiMedium.refresh();
    refresh();

    /* Make sure medium was not released yet: */
    if (medium().curStateMachineIds().isEmpty())
        return true;

    /* Confirm release: */
    if (!msgCenter().confirmMediumRelease(medium(), fInduced, treeWidget()))
        return false;

    /* Release: */
    foreach (const QString &strMachineId, medium().curStateMachineIds())
        if (!releaseFrom(strMachineId))
            return false;

    /* True by default: */
    return true;
}

void UIMediumItem::refreshAll()
{
    m_guiMedium.blockAndQueryState();
    refresh();
}

void UIMediumItem::setMedium(const UIMedium &guiMedium)
{
    m_guiMedium = guiMedium;
    refresh();
}

bool UIMediumItem::operator<(const QTreeWidgetItem &other) const
{
    int iColumn = treeWidget()->sortColumn();
    ULONG64 uThisValue = vboxGlobal().parseSize(      text(iColumn));
    ULONG64 uThatValue = vboxGlobal().parseSize(other.text(iColumn));
    return uThisValue && uThatValue ? uThisValue < uThatValue : QTreeWidgetItem::operator<(other);
}

QString UIMediumItem::defaultText() const
{
    return UIMediumManager::tr("%1, %2: %3, %4: %5", "col.1 text, col.2 name: col.2 text, col.3 name: col.3 text")
                               .arg(text(0))
                               .arg(parentTree()->headerItem()->text(1)).arg(text(1))
                               .arg(parentTree()->headerItem()->text(2)).arg(text(2));
}

void UIMediumItem::refresh()
{
    /* Fill-in columns: */
    setIcon(0, m_guiMedium.icon());
    setText(0, m_guiMedium.name());
    setText(1, m_guiMedium.logicalSize());
    setText(2, m_guiMedium.size());
    /* All columns get the same tooltip: */
    QString strToolTip = m_guiMedium.toolTip();
    for (int i = 0; i < treeWidget()->columnCount(); ++i)
        setToolTip(i, strToolTip);

    /* Gather medium data: */
    m_fValid =    !m_guiMedium.isNull()
               && m_guiMedium.state() != KMediumState_Inaccessible;
    m_enmType = m_guiMedium.type();
    m_enmVariant = m_guiMedium.mediumVariant();
    m_fHasChildren = m_guiMedium.hasChildren();
    /* Gather medium options data: */
    m_options.m_enmType = m_guiMedium.mediumType();
    m_options.m_strLocation = m_guiMedium.location();
    m_options.m_uLogicalSize = m_guiMedium.logicalSizeInBytes();
    m_options.m_strDescription = m_guiMedium.description();
    /* Gather medium details data: */
    m_details.m_aFields.clear();
    switch (m_enmType)
    {
        case UIMediumType_HardDisk:
        {
            m_details.m_aLabels << UIMediumManager::tr("Format:");
            m_details.m_aLabels << UIMediumManager::tr("Storage details:");
            m_details.m_aLabels << UIMediumManager::tr("Attached to:");
            m_details.m_aLabels << UIMediumManager::tr("Encrypted with key:");
            m_details.m_aLabels << UIMediumManager::tr("UUID:");

            m_details.m_aFields << hardDiskFormat();
            m_details.m_aFields << details();
            m_details.m_aFields << (usage().isNull() ?
                                    formatFieldText(UIMediumManager::tr("<i>Not&nbsp;Attached</i>"), false) :
                                    formatFieldText(usage()));
            m_details.m_aFields << (encryptionPasswordID().isNull() ?
                                    formatFieldText(UIMediumManager::tr("<i>Not&nbsp;Encrypted</i>"), false) :
                                    formatFieldText(encryptionPasswordID()));
            m_details.m_aFields << id();

            break;
        }
        case UIMediumType_DVD:
        case UIMediumType_Floppy:
        {
            m_details.m_aLabels << UIMediumManager::tr("Attached to:");
            m_details.m_aLabels << UIMediumManager::tr("UUID:");

            m_details.m_aFields << (usage().isNull() ?
                                    formatFieldText(UIMediumManager::tr("<i>Not&nbsp;Attached</i>"), false) :
                                    formatFieldText(usage()));
            m_details.m_aFields << id();
            break;
        }
        default:
            break;
    }
}

bool UIMediumItem::releaseFrom(const QString &strMachineId)
{
    /* Open session: */
    CSession session = vboxGlobal().openSession(strMachineId);
    if (session.isNull())
        return false;

    /* Get machine: */
    CMachine machine = session.GetMachine();

    /* Prepare result: */
    bool fSuccess = false;

    /* Release medium from machine: */
    if (releaseFrom(machine))
    {
        /* Save machine settings: */
        machine.SaveSettings();
        if (!machine.isOk())
            msgCenter().cannotSaveMachineSettings(machine, treeWidget());
        else
            fSuccess = true;
    }

    /* Close session: */
    session.UnlockMachine();

    /* Return result: */
    return fSuccess;
}

/* static */
QString UIMediumItem::formatFieldText(const QString &strText, bool fCompact /* = true */,
                                      const QString &strElipsis /* = "middle" */)
{
    QString strCompactString = QString("<compact elipsis=\"%1\">").arg(strElipsis);
    QString strInfo = QString("<nobr>%1%2%3</nobr>")
                              .arg(fCompact ? strCompactString : "")
                              .arg(strText.isEmpty() ? UIMediumManager::tr("--", "no info") : strText)
                              .arg(fCompact ? "</compact>" : "");
    return strInfo;
}


/*********************************************************************************************************************************
*   Class UIMediumItemHD implementation.                                                                                         *
*********************************************************************************************************************************/

UIMediumItemHD::UIMediumItemHD(const UIMedium &guiMedium, QITreeWidget *pParent)
    : UIMediumItem(guiMedium, pParent)
{
}

UIMediumItemHD::UIMediumItemHD(const UIMedium &guiMedium, UIMediumItem *pParent)
    : UIMediumItem(guiMedium, pParent)
{
}

bool UIMediumItemHD::remove()
{
    /* Confirm medium removal: */
    if (!msgCenter().confirmMediumRemoval(medium(), treeWidget()))
        return false;

    /* Remember some of hard-disk attributes: */
    CMedium hardDisk = medium().medium();
    QString strMediumID = id();

    /* Propose to remove medium storage: */
    if (!maybeRemoveStorage())
        return false;

    /* Close hard-disk: */
    hardDisk.Close();
    if (!hardDisk.isOk())
    {
        msgCenter().cannotCloseMedium(medium(), hardDisk, treeWidget());
        return false;
    }

    /* Remove UIMedium finally: */
    vboxGlobal().deleteMedium(strMediumID);

    /* True by default: */
    return true;
}

bool UIMediumItemHD::releaseFrom(CMachine comMachine)
{
    /* Enumerate attachments: */
    CMediumAttachmentVector attachments = comMachine.GetMediumAttachments();
    foreach (const CMediumAttachment &attachment, attachments)
    {
        /* Skip non-hard-disks: */
        if (attachment.GetType() != KDeviceType_HardDisk)
            continue;

        /* Skip unrelated hard-disks: */
        if (attachment.GetMedium().GetId() != id())
            continue;

        /* Remember controller: */
        CStorageController controller = comMachine.GetStorageControllerByName(attachment.GetController());

        /* Try to detach device: */
        comMachine.DetachDevice(attachment.GetController(), attachment.GetPort(), attachment.GetDevice());
        if (!comMachine.isOk())
        {
            /* Return failure: */
            msgCenter().cannotDetachDevice(comMachine, UIMediumType_HardDisk, location(),
                                           StorageSlot(controller.GetBus(), attachment.GetPort(), attachment.GetDevice()),
                                           treeWidget());
            return false;
        }

        /* Return success: */
        return true;
    }

    /* False by default: */
    return false;
}

bool UIMediumItemHD::maybeRemoveStorage()
{
    /* Remember some of hard-disk attributes: */
    CMedium hardDisk = medium().medium();
    QString strLocation = location();

    /* We don't want to try to delete inaccessible storage as it will most likely fail.
     * Note that UIMessageCenter::confirmMediumRemoval() is aware of that and
     * will give a corresponding hint. Therefore, once the code is changed below,
     * the hint should be re-checked for validity. */
    bool fDeleteStorage = false;
    qulonglong uCapability = 0;
    QVector<KMediumFormatCapabilities> capabilities = hardDisk.GetMediumFormat().GetCapabilities();
    foreach (KMediumFormatCapabilities capability, capabilities)
        uCapability |= capability;
    if (state() != KMediumState_Inaccessible && uCapability & KMediumFormatCapabilities_File)
    {
        int rc = msgCenter().confirmDeleteHardDiskStorage(strLocation, treeWidget());
        if (rc == AlertButton_Cancel)
            return false;
        fDeleteStorage = rc == AlertButton_Choice1;
    }

    /* If user wish to delete storage: */
    if (fDeleteStorage)
    {
        /* Prepare delete storage progress: */
        CProgress progress = hardDisk.DeleteStorage();
        if (!hardDisk.isOk())
        {
            msgCenter().cannotDeleteHardDiskStorage(hardDisk, strLocation, treeWidget());
            return false;
        }
        /* Show delete storage progress: */
        msgCenter().showModalProgressDialog(progress, UIMediumManager::tr("Removing medium..."),
                                            ":/progress_media_delete_90px.png", treeWidget());
        if (!progress.isOk() || progress.GetResultCode() != 0)
        {
            msgCenter().cannotDeleteHardDiskStorage(progress, strLocation, treeWidget());
            return false;
        }
    }

    /* True by default: */
    return true;
}


/*********************************************************************************************************************************
*   Class UIMediumItemCD implementation.                                                                                         *
*********************************************************************************************************************************/

UIMediumItemCD::UIMediumItemCD(const UIMedium &guiMedium, QITreeWidget *pParent)
    : UIMediumItem(guiMedium, pParent)
{
}

bool UIMediumItemCD::remove()
{
    /* Confirm medium removal: */
    if (!msgCenter().confirmMediumRemoval(medium(), treeWidget()))
        return false;

    /* Remember some of optical-disk attributes: */
    CMedium image = medium().medium();
    QString strMediumID = id();

    /* Close optical-disk: */
    image.Close();
    if (!image.isOk())
    {
        msgCenter().cannotCloseMedium(medium(), image, treeWidget());
        return false;
    }

    /* Remove UIMedium finally: */
    vboxGlobal().deleteMedium(strMediumID);

    /* True by default: */
    return true;
}

bool UIMediumItemCD::releaseFrom(CMachine comMachine)
{
    /* Enumerate attachments: */
    CMediumAttachmentVector attachments = comMachine.GetMediumAttachments();
    foreach (const CMediumAttachment &attachment, attachments)
    {
        /* Skip non-optical-disks: */
        if (attachment.GetType() != KDeviceType_DVD)
            continue;

        /* Skip unrelated optical-disks: */
        if (attachment.GetMedium().GetId() != id())
            continue;

        /* Try to unmount device: */
        comMachine.MountMedium(attachment.GetController(), attachment.GetPort(), attachment.GetDevice(), CMedium(), false /* force */);
        if (!comMachine.isOk())
        {
            /* Return failure: */
            msgCenter().cannotRemountMedium(comMachine, medium(), false /* mount? */, false /* retry? */, treeWidget());
            return false;
        }

        /* Return success: */
        return true;
    }

    /* Return failure: */
    return false;
}


/*********************************************************************************************************************************
*   Class UIMediumItemFD implementation.                                                                                         *
*********************************************************************************************************************************/

UIMediumItemFD::UIMediumItemFD(const UIMedium &guiMedium, QITreeWidget *pParent)
    : UIMediumItem(guiMedium, pParent)
{
}

bool UIMediumItemFD::remove()
{
    /* Confirm medium removal: */
    if (!msgCenter().confirmMediumRemoval(medium(), treeWidget()))
        return false;

    /* Remember some of floppy-disk attributes: */
    CMedium image = medium().medium();
    QString strMediumID = id();

    /* Close floppy-disk: */
    image.Close();
    if (!image.isOk())
    {
        msgCenter().cannotCloseMedium(medium(), image, treeWidget());
        return false;
    }

    /* Remove UIMedium finally: */
    vboxGlobal().deleteMedium(strMediumID);

    /* True by default: */
    return true;
}

bool UIMediumItemFD::releaseFrom(CMachine comMachine)
{
    /* Enumerate attachments: */
    CMediumAttachmentVector attachments = comMachine.GetMediumAttachments();
    foreach (const CMediumAttachment &attachment, attachments)
    {
        /* Skip non-floppy-disks: */
        if (attachment.GetType() != KDeviceType_Floppy)
            continue;

        /* Skip unrelated floppy-disks: */
        if (attachment.GetMedium().GetId() != id())
            continue;

        /* Try to unmount device: */
        comMachine.MountMedium(attachment.GetController(), attachment.GetPort(), attachment.GetDevice(), CMedium(), false /* force */);
        if (!comMachine.isOk())
        {
            /* Return failure: */
            msgCenter().cannotRemountMedium(comMachine, medium(), false /* mount? */, false /* retry? */, treeWidget());
            return false;
        }

        /* Return success: */
        return true;
    }

    /* Return failure: */
    return false;
}


/*********************************************************************************************************************************
*   Class UIEnumerationProgressBar implementation.                                                                               *
*********************************************************************************************************************************/

UIEnumerationProgressBar::UIEnumerationProgressBar(QWidget *pParent /* = 0 */)
    : QWidget(pParent)
{
    /* Prepare: */
    prepare();
}

void UIEnumerationProgressBar::setText(const QString &strText)
{
    m_pLabel->setText(strText);
}

int UIEnumerationProgressBar::value() const
{
    return m_pProgressBar->value();
}

void UIEnumerationProgressBar::setValue(int iValue)
{
    m_pProgressBar->setValue(iValue);
}

void UIEnumerationProgressBar::setMaximum(int iValue)
{
    m_pProgressBar->setMaximum(iValue);
}

void UIEnumerationProgressBar::prepare()
{
    /* Create layout: */
    QHBoxLayout *pLayout = new QHBoxLayout(this);
    {
        /* Configure layout: */
        pLayout->setContentsMargins(0, 0, 0, 0);
        /* Create label: */
        m_pLabel = new QLabel;
        /* Create progress-bar: */
        m_pProgressBar = new QProgressBar;
        {
            /* Configure progress-bar: */
            m_pProgressBar->setTextVisible(false);
        }
        /* Add widgets into layout: */
        pLayout->addWidget(m_pLabel);
        pLayout->addWidget(m_pProgressBar);
    }
}


/*********************************************************************************************************************************
*   Class UIMediumManagerWidget implementation.                                                                                  *
*********************************************************************************************************************************/

UIMediumManagerWidget::UIMediumManagerWidget(EmbedTo enmEmbedding, QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_enmEmbedding(enmEmbedding)
    , m_fPreventChangeCurrentItem(false)
    , m_pTabWidget(0)
    , m_iTabCount(3)
    , m_fInaccessibleHD(false)
    , m_fInaccessibleCD(false)
    , m_fInaccessibleFD(false)
    , m_iconHD(UIIconPool::iconSet(":/hd_16px.png", ":/hd_disabled_16px.png"))
    , m_iconCD(UIIconPool::iconSet(":/cd_16px.png", ":/cd_disabled_16px.png"))
    , m_iconFD(UIIconPool::iconSet(":/fd_16px.png", ":/fd_disabled_16px.png"))
    , m_pDetailsWidget(0)
    , m_pToolBar(0)
    , m_pContextMenu(0)
    , m_pMenu(0)
    , m_pActionCopy(0), m_pActionMove(0), m_pActionRemove(0)
    , m_pActionRelease(0), m_pActionDetails(0)
    , m_pActionRefresh(0)
    , m_pProgressBar(0)
{
    /* Prepare: */
    prepare();
}

void UIMediumManagerWidget::setProgressBar(UIEnumerationProgressBar *pProgressBar)
{
    /* Cache progress-bar reference:*/
    m_pProgressBar = pProgressBar;

    /* Update translation: */
    retranslateUi();
}

void UIMediumManagerWidget::retranslateUi()
{
    /* Translate menu: */
    if (m_pMenu)
        m_pMenu->setTitle(UIMediumManager::tr("&Medium"));

    /* Translate actions: */
    if (m_pActionCopy)
    {
        m_pActionCopy->setText(UIMediumManager::tr("&Copy..."));
        m_pActionCopy->setToolTip(UIMediumManager::tr("Copy Disk Image File (%1)").arg(m_pActionCopy->shortcut().toString()));
        m_pActionCopy->setStatusTip(UIMediumManager::tr("Copy selected disk image file"));
    }
    if (m_pActionMove)
    {
        m_pActionMove->setText(UIMediumManager::tr("&Move..."));
        m_pActionMove->setToolTip(UIMediumManager::tr("Move Disk Image File (%1)").arg(m_pActionMove->shortcut().toString()));
        m_pActionMove->setStatusTip(UIMediumManager::tr("Move selected disk image file"));
    }
    if (m_pActionRemove)
    {
        m_pActionRemove->setText(UIMediumManager::tr("&Remove..."));
        m_pActionRemove->setToolTip(UIMediumManager::tr("Remove Disk Image File (%1)").arg(m_pActionRemove->shortcut().toString()));
        m_pActionRemove->setStatusTip(UIMediumManager::tr("Remove selected disk image file"));
    }
    if (m_pActionRelease)
    {
        m_pActionRelease->setText(UIMediumManager::tr("Re&lease..."));
        m_pActionRelease->setToolTip(UIMediumManager::tr("Release Disk Image File (%1)").arg(m_pActionRelease->shortcut().toString()));
        m_pActionRelease->setStatusTip(UIMediumManager::tr("Release selected disk image file by detaching it from machines"));
    }
    if (m_pActionDetails)
    {
        m_pActionDetails->setText(UIMediumManager::tr("&Properties..."));
        m_pActionDetails->setToolTip(UIMediumManager::tr("Open Disk Image File Properties (%1)").arg(m_pActionDetails->shortcut().toString()));
        m_pActionDetails->setStatusTip(UIMediumManager::tr("Open pane with selected disk image file properties"));
    }
    if (m_pActionRefresh)
    {
        m_pActionRefresh->setText(UIMediumManager::tr("Re&fresh"));
        m_pActionRefresh->setToolTip(UIMediumManager::tr("Refresh Disk Image Files (%1)").arg(m_pActionRefresh->shortcut().toString()));
        m_pActionRefresh->setStatusTip(UIMediumManager::tr("Refresh the list of disk image files"));
    }

    /* Translate toolbar: */
#ifdef VBOX_WS_MAC
    // WORKAROUND:
    // There is a bug in Qt Cocoa which result in showing a "more arrow" when
    // the necessary size of the toolbar is increased. Also for some languages
    // the with doesn't match if the text increase. So manually adjust the size
    // after changing the text. */
    if (m_pToolBar)
        m_pToolBar->updateLayout();
#endif

    /* Translate tab-widget: */
    if (m_pTabWidget)
    {
        m_pTabWidget->setTabText(tabIndex(UIMediumType_HardDisk), UIMediumManager::tr("&Hard disks"));
        m_pTabWidget->setTabText(tabIndex(UIMediumType_DVD), UIMediumManager::tr("&Optical disks"));
        m_pTabWidget->setTabText(tabIndex(UIMediumType_Floppy), UIMediumManager::tr("&Floppy disks"));
    }

    /* Translate HD tree-widget: */
    QITreeWidget *pTreeWidgetHD = treeWidget(UIMediumType_HardDisk);
    if (pTreeWidgetHD)
    {
        pTreeWidgetHD->headerItem()->setText(0, UIMediumManager::tr("Name"));
        pTreeWidgetHD->headerItem()->setText(1, UIMediumManager::tr("Virtual Size"));
        pTreeWidgetHD->headerItem()->setText(2, UIMediumManager::tr("Actual Size"));
    }

    /* Translate CD tree-widget: */
    QITreeWidget *pTreeWidgetCD = treeWidget(UIMediumType_DVD);
    if (pTreeWidgetCD)
    {
        pTreeWidgetCD->headerItem()->setText(0, UIMediumManager::tr("Name"));
        pTreeWidgetCD->headerItem()->setText(1, UIMediumManager::tr("Size"));
    }

    /* Translate FD tree-widget: */
    QITreeWidget *pTreeWidgetFD = treeWidget(UIMediumType_Floppy);
    if (pTreeWidgetFD)
    {
        pTreeWidgetFD->headerItem()->setText(0, UIMediumManager::tr("Name"));
        pTreeWidgetFD->headerItem()->setText(1, UIMediumManager::tr("Size"));
    }

    /* Translate progress-bar: */
    if (m_pProgressBar)
    {
        m_pProgressBar->setText(UIMediumManager::tr("Checking accessibility"));
#ifdef VBOX_WS_MAC
        /* Make sure that the widgets aren't jumping around
         * while the progress-bar get visible. */
        m_pProgressBar->adjustSize();
        //int h = m_pProgressBar->height();
        //if (m_pButtonBox)
        //    m_pButtonBox->setMinimumHeight(h + 12);
#endif
    }

    /* Full refresh if there is at least one item present: */
    if (   (pTreeWidgetHD && pTreeWidgetHD->topLevelItemCount())
        || (pTreeWidgetCD && pTreeWidgetCD->topLevelItemCount())
        || (pTreeWidgetFD && pTreeWidgetFD->topLevelItemCount()))
        sltRefreshAll();
}

void UIMediumManagerWidget::showEvent(QShowEvent *pEvent)
{
    /* Call to base-class: */
    QIWithRetranslateUI<QWidget>::showEvent(pEvent);

    /* Focus current tree-widget: */
    if (currentTreeWidget())
        currentTreeWidget()->setFocus();
}

void UIMediumManagerWidget::sltResetMediumDetailsChanges()
{
    /* Push the current item data into details-widget: */
    sltHandleCurrentTabChanged();
}

void UIMediumManagerWidget::sltApplyMediumDetailsChanges()
{
    /* Get current medium-item: */
    UIMediumItem *pMediumItem = currentMediumItem();
    AssertMsgReturnVoid(pMediumItem, ("Current item must not be null"));
    AssertReturnVoid(!pMediumItem->id().isNull());

    /* Get item data: */
    UIDataMedium oldData = *pMediumItem;
    UIDataMedium newData = m_pDetailsWidget->data();

    /* Search for corresponding medium: */
    CMedium comMedium = vboxGlobal().medium(pMediumItem->id()).medium();

    /* Try to assign new medium type: */
    if (   comMedium.isOk()
        && newData.m_options.m_enmType != oldData.m_options.m_enmType)
    {
        /* Check if we need to release medium first: */
        bool fDo = true;
        if (   pMediumItem->machineIds().size() > 1
            || (   (   newData.m_options.m_enmType == KMediumType_Immutable
                    || newData.m_options.m_enmType == KMediumType_MultiAttach)
                && pMediumItem->machineIds().size() > 0))
            fDo = pMediumItem->release(true);

        if (fDo)
        {
            comMedium.SetType(newData.m_options.m_enmType);

            /* Show error message if necessary: */
            if (!comMedium.isOk())
                msgCenter().cannotChangeMediumType(comMedium, oldData.m_options.m_enmType, newData.m_options.m_enmType, this);
        }
    }

    /* Try to assign new medium location: */
    if (   comMedium.isOk()
        && newData.m_options.m_strLocation != oldData.m_options.m_strLocation)
    {
        /* Prepare move storage progress: */
        CProgress comProgress = comMedium.SetLocation(newData.m_options.m_strLocation);

        /* Show error message if necessary: */
        if (!comMedium.isOk())
            msgCenter().cannotMoveMediumStorage(comMedium,
                                                oldData.m_options.m_strLocation,
                                                newData.m_options.m_strLocation,
                                                this);
        else
        {
            /* Show move storage progress: */
            msgCenter().showModalProgressDialog(comProgress, UIMediumManager::tr("Moving medium..."),
                                                ":/progress_media_move_90px.png", this);

            /* Show error message if necessary: */
            if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
                msgCenter().cannotMoveMediumStorage(comProgress,
                                                    oldData.m_options.m_strLocation,
                                                    newData.m_options.m_strLocation,
                                                    this);
        }
    }

    /* Try to assign new medium size: */
    if (   comMedium.isOk()
        && newData.m_options.m_uLogicalSize != oldData.m_options.m_uLogicalSize)
    {
        /* Prepare resize storage progress: */
        CProgress comProgress = comMedium.Resize(newData.m_options.m_uLogicalSize);

        /* Show error message if necessary: */
        if (!comMedium.isOk())
            msgCenter().cannotResizeHardDiskStorage(comMedium,
                                                    oldData.m_options.m_strLocation,
                                                    vboxGlobal().formatSize(oldData.m_options.m_uLogicalSize),
                                                    vboxGlobal().formatSize(newData.m_options.m_uLogicalSize),
                                                    this);
        else
        {
            /* Show resize storage progress: */
            msgCenter().showModalProgressDialog(comProgress, UIMediumManager::tr("Moving medium..."),
                                                ":/progress_media_move_90px.png", this);

            /* Show error message if necessary: */
            if (!comProgress.isOk() || comProgress.GetResultCode() != 0)
                msgCenter().cannotResizeHardDiskStorage(comProgress,
                                                        oldData.m_options.m_strLocation,
                                                        vboxGlobal().formatSize(oldData.m_options.m_uLogicalSize),
                                                        vboxGlobal().formatSize(newData.m_options.m_uLogicalSize),
                                                        this);
        }
    }

    /* Try to assign new medium description: */
    if (   comMedium.isOk()
        && newData.m_options.m_strDescription != oldData.m_options.m_strDescription)
    {
        comMedium.SetDescription(newData.m_options.m_strDescription);

        /* Show error message if necessary: */
        if (!comMedium.isOk())
            msgCenter().cannotChangeMediumDescription(comMedium,
                                                      oldData.m_options.m_strLocation,
                                                      this);
    }

    /* Recache current item: */
    pMediumItem->refreshAll();

    /* Push the current item data into details-widget: */
    sltHandleCurrentTabChanged();
}

void UIMediumManagerWidget::sltHandleMediumCreated(const QString &strMediumID)
{
    /* Search for corresponding medium: */
    UIMedium medium = vboxGlobal().medium(strMediumID);

    /* Ignore non-interesting mediums: */
    if (medium.isNull() || medium.isHostDrive())
        return;

    /* Ignore mediums (and their children) which are
     * marked as hidden or attached to hidden machines only: */
    if (UIMedium::isMediumAttachedToHiddenMachinesOnly(medium))
        return;

    /* Create medium-item for corresponding medium: */
    UIMediumItem *pMediumItem = createMediumItem(medium);

    /* Make sure medium-item was created: */
    if (!pMediumItem)
        return;

    /* If medium-item change allowed and
     * 1. medium-enumeration is not currently in progress or
     * 2. if there is no currently medium-item selected
     * we have to choose newly added medium-item as current one: */
    if (   !m_fPreventChangeCurrentItem
        && (   !vboxGlobal().isMediumEnumerationInProgress()
            || !mediumItem(medium.type())))
        setCurrentItem(treeWidget(medium.type()), pMediumItem);
}

void UIMediumManagerWidget::sltHandleMediumDeleted(const QString &strMediumID)
{
    /* Make sure corresponding medium-item deleted: */
    deleteMediumItem(strMediumID);
}

void UIMediumManagerWidget::sltHandleMediumEnumerationStart()
{
    /* Disable 'refresh' action: */
    if (m_pActionRefresh)
        m_pActionRefresh->setEnabled(false);

    /* Disable details-widget: */
    if (m_pDetailsWidget)
        m_pDetailsWidget->setOptionsEnabled(false);

    /* Reset and show progress-bar: */
    if (m_pProgressBar)
    {
        m_pProgressBar->setMaximum(vboxGlobal().mediumIDs().size());
        m_pProgressBar->setValue(0);
        m_pProgressBar->show();
    }

    /* Reset inaccessibility flags: */
    m_fInaccessibleHD =
        m_fInaccessibleCD =
            m_fInaccessibleFD = false;

    /* Reset tab-widget icons: */
    if (m_pTabWidget)
    {
        m_pTabWidget->setTabIcon(tabIndex(UIMediumType_HardDisk), m_iconHD);
        m_pTabWidget->setTabIcon(tabIndex(UIMediumType_DVD), m_iconCD);
        m_pTabWidget->setTabIcon(tabIndex(UIMediumType_Floppy), m_iconFD);
    }

    /* Repopulate tree-widgets content: */
    repopulateTreeWidgets();

    /* Re-fetch all current medium-items: */
    refetchCurrentMediumItems();
    refetchCurrentChosenMediumItem();
}

void UIMediumManagerWidget::sltHandleMediumEnumerated(const QString &strMediumID)
{
    /* Search for corresponding medium: */
    UIMedium medium = vboxGlobal().medium(strMediumID);

    /* Ignore non-interesting mediums: */
    if (medium.isNull() || medium.isHostDrive())
        return;

    /* Ignore mediums (and their children) which are
     * marked as hidden or attached to hidden machines only: */
    if (UIMedium::isMediumAttachedToHiddenMachinesOnly(medium))
        return;

    /* Update medium-item for corresponding medium: */
    updateMediumItem(medium);

    /* Advance progress-bar: */
    if (m_pProgressBar)
        m_pProgressBar->setValue(m_pProgressBar->value() + 1);
}

void UIMediumManagerWidget::sltHandleMediumEnumerationFinish()
{
    /* Hide progress-bar: */
    if (m_pProgressBar)
        m_pProgressBar->hide();

    /* Enable details-widget: */
    if (m_pDetailsWidget)
        m_pDetailsWidget->setOptionsEnabled(true);

    /* Enable 'refresh' action: */
    if (m_pActionRefresh)
        m_pActionRefresh->setEnabled(true);

    /* Re-fetch all current medium-items: */
    refetchCurrentMediumItems();
    refetchCurrentChosenMediumItem();
}

void UIMediumManagerWidget::sltCopyMedium()
{
    /* Get current medium-item: */
    UIMediumItem *pMediumItem = currentMediumItem();
    AssertMsgReturnVoid(pMediumItem, ("Current item must not be null"));
    AssertReturnVoid(!pMediumItem->id().isNull());

    /* Copy current medium-item: */
    pMediumItem->copy();
}

void UIMediumManagerWidget::sltMoveMedium()
{
    /* Get current medium-item: */
    UIMediumItem *pMediumItem = currentMediumItem();
    AssertMsgReturnVoid(pMediumItem, ("Current item must not be null"));
    AssertReturnVoid(!pMediumItem->id().isNull());

    /* Copy current medium-item: */
    pMediumItem->move();

    /* Push the current item data into details-widget: */
    sltHandleCurrentTabChanged();
}

void UIMediumManagerWidget::sltRemoveMedium()
{
    /* Get current medium-item: */
    UIMediumItem *pMediumItem = currentMediumItem();
    AssertMsgReturnVoid(pMediumItem, ("Current item must not be null"));
    AssertReturnVoid(!pMediumItem->id().isNull());

    /* Remove current medium-item: */
    pMediumItem->remove();
}

void UIMediumManagerWidget::sltReleaseMedium()
{
    /* Get current medium-item: */
    UIMediumItem *pMediumItem = currentMediumItem();
    AssertMsgReturnVoid(pMediumItem, ("Current item must not be null"));
    AssertReturnVoid(!pMediumItem->id().isNull());

    /* Remove current medium-item: */
    bool fResult = pMediumItem->release();

    /* Refetch currently chosen medium-item: */
    if (fResult)
        refetchCurrentChosenMediumItem();
}

void UIMediumManagerWidget::sltToggleMediumDetailsVisibility(bool fVisible)
{
    /* Save the setting: */
    gEDataManager->setVirtualMediaManagerDetailsExpanded(fVisible);
    /* Toggle medium details visibility: */
    if (m_pDetailsWidget)
        m_pDetailsWidget->setVisible(fVisible);
    /* Notify external lsiteners: */
    emit sigMediumDetailsVisibilityChanged(fVisible);
}

void UIMediumManagerWidget::sltRefreshAll()
{
    /* Start medium-enumeration: */
    vboxGlobal().startMediumEnumeration();
}

void UIMediumManagerWidget::sltHandleCurrentTabChanged()
{
    /* Get current tree-widget: */
    QITreeWidget *pTreeWidget = currentTreeWidget();
    if (pTreeWidget)
    {
        /* If another tree-widget was focused before,
         * move focus to current tree-widget: */
        if (qobject_cast<QITreeWidget*>(focusWidget()))
            pTreeWidget->setFocus();
    }

    /* Update action icons: */
    updateActionIcons();

    /* Raise the required information-container: */
    if (m_pDetailsWidget)
        m_pDetailsWidget->setCurrentType(currentMediumType());
    /* Re-fetch currently chosen medium-item: */
    refetchCurrentChosenMediumItem();
}

void UIMediumManagerWidget::sltHandleCurrentItemChanged()
{
    /* Get sender() tree-widget: */
    QITreeWidget *pTreeWidget = qobject_cast<QITreeWidget*>(sender());
    AssertMsgReturnVoid(pTreeWidget, ("This slot should be called by tree-widget only!\n"));

    /* Re-fetch current medium-item of required type: */
    refetchCurrentMediumItem(mediumType(pTreeWidget));
}

void UIMediumManagerWidget::sltHandleContextMenuCall(const QPoint &position)
{
    /* Get current tree-widget: */
    QITreeWidget *pTreeWidget = currentTreeWidget();
    AssertPtrReturnVoid(pTreeWidget);

    /* Make sure underlaying item was found: */
    QTreeWidgetItem *pItem = pTreeWidget->itemAt(position);
    if (!pItem)
        return;

    /* Make sure that item is current one: */
    setCurrentItem(pTreeWidget, pItem);

    /* Show item context menu: */
    if (m_pContextMenu)
        m_pContextMenu->exec(pTreeWidget->viewport()->mapToGlobal(position));
}

void UIMediumManagerWidget::sltPerformTablesAdjustment()
{
    /* Get all the tree-widgets: */
    const QList<QITreeWidget*> trees = m_trees.values();

    /* Calculate deduction for every header: */
    QList<int> deductions;
    foreach (QITreeWidget *pTreeWidget, trees)
    {
        int iDeduction = 0;
        for (int iHeaderIndex = 1; iHeaderIndex < pTreeWidget->header()->count(); ++iHeaderIndex)
            iDeduction += pTreeWidget->header()->sectionSize(iHeaderIndex);
        deductions << iDeduction;
    }

    /* Adjust the table's first column: */
    for (int iTreeIndex = 0; iTreeIndex < trees.size(); ++iTreeIndex)
    {
        QITreeWidget *pTreeWidget = trees[iTreeIndex];
        int iSize0 = pTreeWidget->viewport()->width() - deductions[iTreeIndex];
        if (pTreeWidget->header()->sectionSize(0) != iSize0)
            pTreeWidget->header()->resizeSection(0, iSize0);
    }
}

void UIMediumManagerWidget::prepare()
{
    /* Prepare this: */
    prepareThis();

    /* Load settings: */
    loadSettings();

    /* Apply language settings: */
    retranslateUi();

    /* Start medium-enumeration (if necessary): */
    if (!vboxGlobal().isMediumEnumerationInProgress())
        vboxGlobal().startMediumEnumeration();
    /* Emulate medium-enumeration otherwise: */
    else
    {
        /* Start medium-enumeration: */
        sltHandleMediumEnumerationStart();

        /* Finish medium-enumeration (if necessary): */
        if (!vboxGlobal().isMediumEnumerationInProgress())
            sltHandleMediumEnumerationFinish();
    }
}

void UIMediumManagerWidget::prepareThis()
{
    /* Prepare connections: */
    prepareConnections();
    /* Prepare actions: */
    prepareActions();
    /* Prepare central-widget: */
    prepareWidgets();
}

void UIMediumManagerWidget::prepareConnections()
{
    /* Configure medium-processing connections: */
    connect(&vboxGlobal(), &VBoxGlobal::sigMediumCreated,
            this, &UIMediumManagerWidget::sltHandleMediumCreated);
    connect(&vboxGlobal(), &VBoxGlobal::sigMediumDeleted,
            this, &UIMediumManagerWidget::sltHandleMediumDeleted);

    /* Configure medium-enumeration connections: */
    connect(&vboxGlobal(), &VBoxGlobal::sigMediumEnumerationStarted,
            this, &UIMediumManagerWidget::sltHandleMediumEnumerationStart);
    connect(&vboxGlobal(), &VBoxGlobal::sigMediumEnumerated,
            this, &UIMediumManagerWidget::sltHandleMediumEnumerated);
    connect(&vboxGlobal(), &VBoxGlobal::sigMediumEnumerationFinished,
            this, &UIMediumManagerWidget::sltHandleMediumEnumerationFinish);
}

void UIMediumManagerWidget::prepareActions()
{
    /* Create 'Copy' action: */
    m_pActionCopy = new QAction(this);
    AssertPtrReturnVoid(m_pActionCopy);
    {
        /* Configure copy-action: */
        m_pActionCopy->setShortcut(QKeySequence("Ctrl+C"));
        connect(m_pActionCopy, &QAction::triggered, this, &UIMediumManagerWidget::sltCopyMedium);
    }

    /* Create 'Move' action: */
    m_pActionMove = new QAction(this);
    AssertPtrReturnVoid(m_pActionMove);
    {
        /* Configure move-action: */
        m_pActionMove->setShortcut(QKeySequence("Ctrl+M"));
        connect(m_pActionMove, &QAction::triggered, this, &UIMediumManagerWidget::sltMoveMedium);
    }

    /* Create 'Remove' action: */
    m_pActionRemove  = new QAction(this);
    AssertPtrReturnVoid(m_pActionRemove);
    {
        /* Configure remove-action: */
        m_pActionRemove->setShortcut(QKeySequence("Ctrl+R"));
        connect(m_pActionRemove, &QAction::triggered, this, &UIMediumManagerWidget::sltRemoveMedium);
    }

    /* Create 'Release' action: */
    m_pActionRelease = new QAction(this);
    AssertPtrReturnVoid(m_pActionRelease);
    {
        /* Configure release-action: */
        m_pActionRelease->setShortcut(QKeySequence("Ctrl+L"));
        connect(m_pActionRelease, &QAction::triggered, this, &UIMediumManagerWidget::sltReleaseMedium);
    }

    /* Create 'Details' action: */
    m_pActionDetails = new QAction(this);
    AssertPtrReturnVoid(m_pActionDetails);
    {
        /* Configure modify-action: */
        m_pActionDetails->setCheckable(true);
        m_pActionDetails->setShortcut(QKeySequence("Ctrl+Space"));
        connect(m_pActionDetails, &QAction::toggled, this, &UIMediumManagerWidget::sltToggleMediumDetailsVisibility);
    }

    /* Create 'Refresh' action: */
    m_pActionRefresh = new QAction(this);
    AssertPtrReturnVoid(m_pActionRefresh);
    {
        /* Configure refresh-action: */
        m_pActionRefresh->setShortcut(QKeySequence(QKeySequence::Refresh));
        connect(m_pActionRefresh, &QAction::triggered, this, &UIMediumManagerWidget::sltRefreshAll);
    }

    /* Update action icons: */
    updateActionIcons();

    /* Prepare menu: */
    prepareMenu();
    /* Prepare context-menu: */
    prepareContextMenu();
}

void UIMediumManagerWidget::prepareMenu()
{
    /* Create 'Medium' menu: */
    m_pMenu = new QMenu(this);
    AssertPtrReturnVoid(m_pMenu);
    {
        /* Configure 'Medium' menu: */
        if (m_pActionCopy)
            m_pMenu->addAction(m_pActionCopy);
        if (m_pActionMove)
            m_pMenu->addAction(m_pActionMove);
        if (m_pActionRemove)
            m_pMenu->addAction(m_pActionRemove);
        if (   (m_pActionCopy || m_pActionMove || m_pActionRemove)
            && (m_pActionRelease || m_pActionDetails))
            m_pMenu->addSeparator();
        if (m_pActionRelease)
            m_pMenu->addAction(m_pActionRelease);
        if (m_pActionDetails)
            m_pMenu->addAction(m_pActionDetails);
        if (   (m_pActionRelease || m_pActionDetails)
            && (m_pActionRefresh))
            m_pMenu->addSeparator();
        if (m_pActionRefresh)
            m_pMenu->addAction(m_pActionRefresh);
    }
}

void UIMediumManagerWidget::prepareContextMenu()
{
    /* Create context-menu: */
    m_pContextMenu = new QMenu(this);
    AssertPtrReturnVoid(m_pContextMenu);
    {
        /* Configure contex-menu: */
        if (m_pActionCopy)
            m_pContextMenu->addAction(m_pActionCopy);
        if (m_pActionMove)
            m_pContextMenu->addAction(m_pActionMove);
        if (m_pActionRemove)
            m_pContextMenu->addAction(m_pActionRemove);
        if (   (m_pActionCopy || m_pActionMove || m_pActionRemove)
            && (m_pActionRelease || m_pActionDetails))
            m_pContextMenu->addSeparator();
        if (m_pActionRelease)
            m_pContextMenu->addAction(m_pActionRelease);
        if (m_pActionDetails)
            m_pContextMenu->addAction(m_pActionDetails);
    }
}

void UIMediumManagerWidget::prepareWidgets()
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
        /* Prepare tab-widget: */
        prepareTabWidget();
        /* Prepare details-widget: */
        prepareDetailsWidget();
    }
}

void UIMediumManagerWidget::prepareToolBar()
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
        if (m_pActionCopy)
            m_pToolBar->addAction(m_pActionCopy);
        if (m_pActionMove)
            m_pToolBar->addAction(m_pActionMove);
        if (m_pActionRemove)
            m_pToolBar->addAction(m_pActionRemove);
        if (   (m_pActionCopy || m_pActionMove || m_pActionRemove)
            && (m_pActionRelease || m_pActionDetails))
            m_pToolBar->addSeparator();
        if (m_pActionRelease)
            m_pToolBar->addAction(m_pActionRelease);
        if (m_pActionDetails)
            m_pToolBar->addAction(m_pActionDetails);
        if (   (m_pActionRelease || m_pActionDetails)
            && (m_pActionRefresh))
            m_pToolBar->addSeparator();
        if (m_pActionRefresh)
            m_pToolBar->addAction(m_pActionRefresh);

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

void UIMediumManagerWidget::prepareTabWidget()
{
    /* Create tab-widget: */
    m_pTabWidget = new QITabWidget;
    AssertPtrReturnVoid(m_pTabWidget);
    {
        /* Create tabs: */
        for (int i = 0; i < m_iTabCount; ++i)
            prepareTab((UIMediumType)i);
        /* Configure tab-widget: */
        m_pTabWidget->setFocusPolicy(Qt::TabFocus);
        m_pTabWidget->setTabIcon(tabIndex(UIMediumType_HardDisk), m_iconHD);
        m_pTabWidget->setTabIcon(tabIndex(UIMediumType_DVD), m_iconCD);
        m_pTabWidget->setTabIcon(tabIndex(UIMediumType_Floppy), m_iconFD);
        connect(m_pTabWidget, &QITabWidget::currentChanged, this, &UIMediumManagerWidget::sltHandleCurrentTabChanged);

        /* Add tab-widget into central layout: */
        layout()->addWidget(m_pTabWidget);

        /* Update other widgets according chosen tab: */
        sltHandleCurrentTabChanged();
    }
}

void UIMediumManagerWidget::prepareTab(UIMediumType type)
{
    /* Create tab: */
    m_pTabWidget->addTab(new QWidget, QString());
    QWidget *pTab = tab(type);
    AssertPtrReturnVoid(pTab);
    {
        /* Create tab layout: */
        QVBoxLayout *pLayout = new QVBoxLayout(pTab);
        AssertPtrReturnVoid(pLayout);
        {
#ifdef VBOX_WS_MAC
            /* Configure layout: */
            pLayout->setContentsMargins(10, 10, 10, 10);
#endif

            /* Prepare tree-widget: */
            prepareTreeWidget(type, type == UIMediumType_HardDisk ? 3 : 2);
        }
    }
}

void UIMediumManagerWidget::prepareTreeWidget(UIMediumType type, int iColumns)
{
    /* Create tree-widget: */
    m_trees.insert(tabIndex(type), new QITreeWidget);
    QITreeWidget *pTreeWidget = treeWidget(type);
    AssertPtrReturnVoid(pTreeWidget);
    {
        /* Configure tree-widget: */
        pTreeWidget->setExpandsOnDoubleClick(false);
        pTreeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        pTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        pTreeWidget->setAlternatingRowColors(true);
        pTreeWidget->setAllColumnsShowFocus(true);
        pTreeWidget->setAcceptDrops(true);
        pTreeWidget->setColumnCount(iColumns);
        pTreeWidget->sortItems(0, Qt::AscendingOrder);
        if (iColumns > 0)
            pTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Fixed);
        if (iColumns > 1)
            pTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        if (iColumns > 2)
            pTreeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        pTreeWidget->header()->setStretchLastSection(false);
        pTreeWidget->setSortingEnabled(true);
        connect(pTreeWidget, &QITreeWidget::currentItemChanged,
                this, &UIMediumManagerWidget::sltHandleCurrentItemChanged);
        if (m_pActionDetails)
            connect(pTreeWidget, &QITreeWidget::itemDoubleClicked,
                    m_pActionDetails, &QAction::setChecked);
        connect(pTreeWidget, &QITreeWidget::customContextMenuRequested,
                this, &UIMediumManagerWidget::sltHandleContextMenuCall);
        connect(pTreeWidget, &QITreeWidget::resized,
                this, &UIMediumManagerWidget::sltPerformTablesAdjustment, Qt::QueuedConnection);
        connect(pTreeWidget->header(), &QHeaderView::sectionResized,
                this, &UIMediumManagerWidget::sltPerformTablesAdjustment, Qt::QueuedConnection);
        /* Add tree-widget into tab layout: */
        tab(type)->layout()->addWidget(pTreeWidget);
    }
}

void UIMediumManagerWidget::prepareDetailsWidget()
{
    /* Create details-widget: */
    m_pDetailsWidget = new UIMediumDetailsWidget(this, m_enmEmbedding);
    AssertPtrReturnVoid(m_pDetailsWidget);
    {
        /* Configure details-widget: */
        m_pDetailsWidget->setVisible(false);
        m_pDetailsWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        connect(m_pDetailsWidget, &UIMediumDetailsWidget::sigAcceptAllowed,
                this, &UIMediumManagerWidget::sigAcceptAllowed);
        connect(m_pDetailsWidget, &UIMediumDetailsWidget::sigRejectAllowed,
                this, &UIMediumManagerWidget::sigRejectAllowed);
        connect(m_pDetailsWidget, &UIMediumDetailsWidget::sigDataChangeRejected,
                this, &UIMediumManagerWidget::sltResetMediumDetailsChanges);
        connect(m_pDetailsWidget, &UIMediumDetailsWidget::sigDataChangeAccepted,
                this, &UIMediumManagerWidget::sltApplyMediumDetailsChanges);

        /* Add into layout: */
        layout()->addWidget(m_pDetailsWidget);
    }
}

void UIMediumManagerWidget::loadSettings()
{
    /* Details action/widget: */
    m_pActionDetails->setChecked(gEDataManager->virtualMediaManagerDetailsExpanded());
}

void UIMediumManagerWidget::repopulateTreeWidgets()
{
    /* Remember current medium-items: */
    if (UIMediumItem *pMediumItem = mediumItem(UIMediumType_HardDisk))
        m_strCurrentIdHD = pMediumItem->id();
    if (UIMediumItem *pMediumItem = mediumItem(UIMediumType_DVD))
        m_strCurrentIdCD = pMediumItem->id();
    if (UIMediumItem *pMediumItem = mediumItem(UIMediumType_Floppy))
        m_strCurrentIdFD = pMediumItem->id();

    /* Clear tree-widgets: */
    QITreeWidget *pTreeWidgetHD = treeWidget(UIMediumType_HardDisk);
    if (pTreeWidgetHD)
    {
        setCurrentItem(pTreeWidgetHD, 0);
        pTreeWidgetHD->clear();
    }
    QITreeWidget *pTreeWidgetCD = treeWidget(UIMediumType_DVD);
    if (pTreeWidgetCD)
    {
        setCurrentItem(pTreeWidgetCD, 0);
        pTreeWidgetCD->clear();
    }
    QITreeWidget *pTreeWidgetFD = treeWidget(UIMediumType_Floppy);
    if (pTreeWidgetFD)
    {
        setCurrentItem(pTreeWidgetFD, 0);
        pTreeWidgetFD->clear();
    }

    /* Create medium-items (do not change current one): */
    m_fPreventChangeCurrentItem = true;
    foreach (const QString &strMediumID, vboxGlobal().mediumIDs())
        sltHandleMediumCreated(strMediumID);
    m_fPreventChangeCurrentItem = false;

    /* Select first item as current one if nothing selected: */
    if (pTreeWidgetHD && !mediumItem(UIMediumType_HardDisk))
        if (QTreeWidgetItem *pItem = pTreeWidgetHD->topLevelItem(0))
            setCurrentItem(pTreeWidgetHD, pItem);
    if (pTreeWidgetCD && !mediumItem(UIMediumType_DVD))
        if (QTreeWidgetItem *pItem = pTreeWidgetCD->topLevelItem(0))
            setCurrentItem(pTreeWidgetCD, pItem);
    if (pTreeWidgetFD && !mediumItem(UIMediumType_Floppy))
        if (QTreeWidgetItem *pItem = pTreeWidgetFD->topLevelItem(0))
            setCurrentItem(pTreeWidgetFD, pItem);
}

void UIMediumManagerWidget::refetchCurrentMediumItem(UIMediumType type)
{
    /* Get corresponding medium-item: */
    UIMediumItem *pMediumItem = mediumItem(type);

#ifdef VBOX_WS_MAC
    /* Set the file for the proxy icon: */
    if (pMediumItem == currentMediumItem())
        setWindowFilePath(pMediumItem ? pMediumItem->location() : QString());
#endif /* VBOX_WS_MAC */

    /* Make sure current medium-item visible: */
    if (pMediumItem)
        treeWidget(type)->scrollToItem(pMediumItem, QAbstractItemView::EnsureVisible);

    /* Update actions: */
    updateActions();

    /* Update details-widget: */
    if (m_pDetailsWidget)
        m_pDetailsWidget->setData(pMediumItem ? *pMediumItem : UIDataMedium(type));
}

void UIMediumManagerWidget::refetchCurrentChosenMediumItem()
{
    refetchCurrentMediumItem(currentMediumType());
}

void UIMediumManagerWidget::refetchCurrentMediumItems()
{
    refetchCurrentMediumItem(UIMediumType_HardDisk);
    refetchCurrentMediumItem(UIMediumType_DVD);
    refetchCurrentMediumItem(UIMediumType_Floppy);
}

void UIMediumManagerWidget::updateActions()
{
    /* Get current medium-item: */
    UIMediumItem *pMediumItem = currentMediumItem();

    /* Calculate actions accessibility: */
    bool fNotInEnumeration = !vboxGlobal().isMediumEnumerationInProgress();

    /* Apply actions accessibility: */
    if (m_pActionCopy)
    {
        bool fActionEnabledCopy = fNotInEnumeration && pMediumItem && checkMediumFor(pMediumItem, Action_Copy);
        m_pActionCopy->setEnabled(fActionEnabledCopy);
    }
    if (m_pActionMove)
    {
        bool fActionEnabledMove = fNotInEnumeration && pMediumItem && checkMediumFor(pMediumItem, Action_Edit);
        m_pActionMove->setEnabled(fActionEnabledMove);
    }
    if (m_pActionRemove)
    {
        bool fActionEnabledRemove = fNotInEnumeration && pMediumItem && checkMediumFor(pMediumItem, Action_Remove);
        m_pActionRemove->setEnabled(fActionEnabledRemove);
    }
    if (m_pActionRelease)
    {
        bool fActionEnabledRelease = fNotInEnumeration && pMediumItem && checkMediumFor(pMediumItem, Action_Release);
        m_pActionRelease->setEnabled(fActionEnabledRelease);
    }
    if (m_pActionDetails)
    {
        bool fActionEnabledDetails = true;
        m_pActionDetails->setEnabled(fActionEnabledDetails);
    }
}

void UIMediumManagerWidget::updateActionIcons()
{
    QString strPrefix = "hd";
    if (m_pTabWidget)
    {
        switch (currentMediumType())
        {
            case UIMediumType_HardDisk: strPrefix = "hd"; break;
            case UIMediumType_DVD:      strPrefix = "cd"; break;
            case UIMediumType_Floppy:   strPrefix = "fd"; break;
            default: break;
        }
    }
    if (m_pActionCopy)
        m_pActionCopy->setIcon(UIIconPool::iconSetFull(QString(":/%1_copy_22px.png").arg(strPrefix),
                                                       QString(":/%1_copy_16px.png").arg(strPrefix),
                                                       QString(":/%1_copy_disabled_22px.png").arg(strPrefix),
                                                       QString(":/%1_copy_disabled_16px.png").arg(strPrefix)));
    if (m_pActionMove)
        m_pActionMove->setIcon(UIIconPool::iconSetFull(QString(":/%1_move_22px.png").arg(strPrefix),
                                                       QString(":/%1_move_16px.png").arg(strPrefix),
                                                       QString(":/%1_move_disabled_22px.png").arg(strPrefix),
                                                       QString(":/%1_move_disabled_16px.png").arg(strPrefix)));
    if (m_pActionRemove)
        m_pActionRemove->setIcon(UIIconPool::iconSetFull(QString(":/%1_remove_22px.png").arg(strPrefix),
                                                         QString(":/%1_remove_16px.png").arg(strPrefix),
                                                         QString(":/%1_remove_disabled_22px.png").arg(strPrefix),
                                                         QString(":/%1_remove_disabled_16px.png").arg(strPrefix)));
    if (m_pActionRelease)
        m_pActionRelease->setIcon(UIIconPool::iconSetFull(QString(":/%1_release_22px.png").arg(strPrefix),
                                                          QString(":/%1_release_16px.png").arg(strPrefix),
                                                          QString(":/%1_release_disabled_22px.png").arg(strPrefix),
                                                          QString(":/%1_release_disabled_16px.png").arg(strPrefix)));
    if (m_pActionDetails)
        m_pActionDetails->setIcon(UIIconPool::iconSetFull(QString(":/%1_modify_22px.png").arg(strPrefix),
                                                          QString(":/%1_modify_16px.png").arg(strPrefix),
                                                          QString(":/%1_modify_disabled_22px.png").arg(strPrefix),
                                                          QString(":/%1_modify_disabled_16px.png").arg(strPrefix)));
    if (m_pActionRefresh && m_pActionRefresh->icon().isNull())
        m_pActionRefresh->setIcon(UIIconPool::iconSetFull(":/refresh_22px.png", ":/refresh_16px.png",
                                                          ":/refresh_disabled_22px.png", ":/refresh_disabled_16px.png"));
}

void UIMediumManagerWidget::updateTabIcons(UIMediumItem *pMediumItem, Action action)
{
    /* Make sure medium-item is valid: */
    AssertReturnVoid(pMediumItem);

    /* Prepare data for tab: */
    const QIcon *pIcon = 0;
    bool *pfInaccessible = 0;
    const UIMediumType mediumType = pMediumItem->mediumType();
    switch (mediumType)
    {
        case UIMediumType_HardDisk:
            pIcon = &m_iconHD;
            pfInaccessible = &m_fInaccessibleHD;
            break;
        case UIMediumType_DVD:
            pIcon = &m_iconCD;
            pfInaccessible = &m_fInaccessibleCD;
            break;
        case UIMediumType_Floppy:
            pIcon = &m_iconFD;
            pfInaccessible = &m_fInaccessibleFD;
            break;
        default:
            AssertFailed();
    }
    AssertReturnVoid(pIcon && pfInaccessible);

    switch (action)
    {
        case Action_Add:
        {
            /* Does it change the overall state? */
            if (*pfInaccessible || pMediumItem->state() != KMediumState_Inaccessible)
                break; /* no */

            *pfInaccessible = true;

            if (m_pTabWidget)
                m_pTabWidget->setTabIcon(tabIndex(mediumType), vboxGlobal().warningIcon());

            break;
        }
        case Action_Edit:
        case Action_Remove:
        {
            bool fCheckRest = false;

            if (action == Action_Edit)
            {
                /* Does it change the overall state? */
                if ((*pfInaccessible && pMediumItem->state() == KMediumState_Inaccessible) ||
                    (!*pfInaccessible && pMediumItem->state() != KMediumState_Inaccessible))
                    break; /* no */

                /* Is the given item in charge? */
                if (!*pfInaccessible && pMediumItem->state() == KMediumState_Inaccessible)
                    *pfInaccessible = true; /* yes */
                else
                    fCheckRest = true; /* no */
            }
            else
                fCheckRest = true;

            if (fCheckRest)
            {
                /* Find the first KMediumState_Inaccessible item to be in charge: */
                CheckIfSuitableByState lookForState(KMediumState_Inaccessible);
                CheckIfSuitableByID ignoreID(pMediumItem->id());
                UIMediumItem *pInaccessibleMediumItem = searchItem(pMediumItem->parentTree(), lookForState, &ignoreID);
                *pfInaccessible = !!pInaccessibleMediumItem;
            }

            if (m_pTabWidget)
            {
                if (*pfInaccessible)
                    m_pTabWidget->setTabIcon(tabIndex(mediumType), vboxGlobal().warningIcon());
                else
                    m_pTabWidget->setTabIcon(tabIndex(mediumType), *pIcon);
            }

            break;
        }

        default:
            break;
    }
}

UIMediumItem* UIMediumManagerWidget::createMediumItem(const UIMedium &medium)
{
    /* Get medium type: */
    UIMediumType type = medium.type();

    /* Create medium-item: */
    UIMediumItem *pMediumItem = 0;
    switch (type)
    {
        /* Of hard-drive type: */
        case UIMediumType_HardDisk:
        {
            /* Make sure corresponding tree-widget exists: */
            QITreeWidget *pTreeWidget = treeWidget(UIMediumType_HardDisk);
            if (pTreeWidget)
            {
                /* Recursively create hard-drive item: */
                pMediumItem = createHardDiskItem(medium);
                /* Make sure item was created: */
                if (!pMediumItem)
                    break;
                if (pMediumItem->id() == m_strCurrentIdHD)
                {
                    setCurrentItem(pTreeWidget, pMediumItem);
                    m_strCurrentIdHD = QString();
                }
            }
            break;
        }
        /* Of optical-image type: */
        case UIMediumType_DVD:
        {
            /* Make sure corresponding tree-widget exists: */
            QITreeWidget *pTreeWidget = treeWidget(UIMediumType_DVD);
            if (pTreeWidget)
            {
                /* Create optical-disk item: */
                pMediumItem = new UIMediumItemCD(medium, pTreeWidget);
                /* Make sure item was created: */
                if (!pMediumItem)
                    break;
                LogRel2(("UIMediumManager: Optical medium-item with ID={%s} created.\n", medium.id().toUtf8().constData()));
                if (pMediumItem->id() == m_strCurrentIdCD)
                {
                    setCurrentItem(pTreeWidget, pMediumItem);
                    m_strCurrentIdCD = QString();
                }
            }
            break;
        }
        /* Of floppy-image type: */
        case UIMediumType_Floppy:
        {
            /* Make sure corresponding tree-widget exists: */
            QITreeWidget *pTreeWidget = treeWidget(UIMediumType_Floppy);
            if (pTreeWidget)
            {
                /* Create floppy-disk item: */
                pMediumItem = new UIMediumItemFD(medium, pTreeWidget);
                /* Make sure item was created: */
                if (!pMediumItem)
                    break;
                LogRel2(("UIMediumManager: Floppy medium-item with ID={%s} created.\n", medium.id().toUtf8().constData()));
                if (pMediumItem->id() == m_strCurrentIdFD)
                {
                    setCurrentItem(pTreeWidget, pMediumItem);
                    m_strCurrentIdFD = QString();
                }
            }
            break;
        }
        default: AssertMsgFailed(("Medium-type unknown: %d\n", type)); break;
    }

    /* Make sure item was created: */
    if (!pMediumItem)
        return 0;

    /* Update tab-icons: */
    updateTabIcons(pMediumItem, Action_Add);

    /* Re-fetch medium-item if it is current one created: */
    if (pMediumItem == mediumItem(type))
        refetchCurrentMediumItem(type);

    /* Return created medium-item: */
    return pMediumItem;
}

UIMediumItem* UIMediumManagerWidget::createHardDiskItem(const UIMedium &medium)
{
    /* Make sure passed medium is valid: */
    AssertReturn(!medium.medium().isNull(), 0);

    /* Make sure corresponding tree-widget exists: */
    QITreeWidget *pTreeWidget = treeWidget(UIMediumType_HardDisk);
    if (pTreeWidget)
    {
        /* Search for existing medium-item: */
        UIMediumItem *pMediumItem = searchItem(pTreeWidget, CheckIfSuitableByID(medium.id()));

        /* If medium-item do not exists: */
        if (!pMediumItem)
        {
            /* If medium have a parent: */
            if (medium.parentID() != UIMedium::nullID())
            {
                /* Try to find parent medium-item: */
                UIMediumItem *pParentMediumItem = searchItem(pTreeWidget, CheckIfSuitableByID(medium.parentID()));
                /* If parent medium-item was not found: */
                if (!pParentMediumItem)
                {
                    /* Make sure corresponding parent medium is already cached! */
                    UIMedium parentMedium = vboxGlobal().medium(medium.parentID());
                    if (parentMedium.isNull())
                        AssertMsgFailed(("Parent medium with ID={%s} was not found!\n", medium.parentID().toUtf8().constData()));
                    /* Try to create parent medium-item: */
                    else
                        pParentMediumItem = createHardDiskItem(parentMedium);
                }
                /* If parent medium-item was found: */
                if (pParentMediumItem)
                {
                    pMediumItem = new UIMediumItemHD(medium, pParentMediumItem);
                    LogRel2(("UIMediumManager: Child hard-disk medium-item with ID={%s} created.\n", medium.id().toUtf8().constData()));
                }
            }
            /* Else just create item as top-level one: */
            if (!pMediumItem)
            {
                pMediumItem = new UIMediumItemHD(medium, pTreeWidget);
                LogRel2(("UIMediumManager: Root hard-disk medium-item with ID={%s} created.\n", medium.id().toUtf8().constData()));
            }
        }

        /* Return created medium-item: */
        return pMediumItem;
    }

    /* Return null by default: */
    return 0;
}

void UIMediumManagerWidget::updateMediumItem(const UIMedium &medium)
{
    /* Get medium type: */
    UIMediumType type = medium.type();

    /* Search for existing medium-item: */
    UIMediumItem *pMediumItem = searchItem(treeWidget(type), CheckIfSuitableByID(medium.id()));

    /* Create item if doesn't exists: */
    if (!pMediumItem)
        pMediumItem = createMediumItem(medium);

    /* Make sure item was created: */
    if (!pMediumItem)
        return;

    /* Update medium-item: */
    pMediumItem->setMedium(medium);
    LogRel2(("UIMediumManager: Medium-item with ID={%s} updated.\n", medium.id().toUtf8().constData()));

    /* Update tab-icons: */
    updateTabIcons(pMediumItem, Action_Edit);

    /* Re-fetch medium-item if it is current one updated: */
    if (pMediumItem == mediumItem(type))
        refetchCurrentMediumItem(type);
}

void UIMediumManagerWidget::deleteMediumItem(const QString &strMediumID)
{
    /* Search for corresponding tree-widget: */
    QList<UIMediumType> types;
    types << UIMediumType_HardDisk << UIMediumType_DVD << UIMediumType_Floppy;
    QITreeWidget *pTreeWidget = 0;
    UIMediumItem *pMediumItem = 0;
    foreach (UIMediumType type, types)
    {
        /* Get iterated tree-widget: */
        pTreeWidget = treeWidget(type);
        /* Search for existing medium-item: */
        pMediumItem = searchItem(pTreeWidget, CheckIfSuitableByID(strMediumID));
        if (pMediumItem)
            break;
    }

    /* Make sure item was found: */
    if (!pMediumItem)
        return;

    /* Update tab-icons: */
    updateTabIcons(pMediumItem, Action_Remove);

    /* Delete medium-item: */
    delete pMediumItem;
    LogRel2(("UIMediumManager: Medium-item with ID={%s} deleted.\n", strMediumID.toUtf8().constData()));

    /* If there is no current medium-item now selected
     * we have to choose first-available medium-item as current one: */
    if (!pTreeWidget->currentItem())
        setCurrentItem(pTreeWidget, pTreeWidget->topLevelItem(0));
}

QWidget* UIMediumManagerWidget::tab(UIMediumType type) const
{
    /* Determine tab index for passed medium type: */
    int iIndex = tabIndex(type);

    /* Return tab for known tab index: */
    if (iIndex >= 0 && iIndex < m_iTabCount)
        return iIndex < m_pTabWidget->count() ? m_pTabWidget->widget(iIndex) : 0;

    /* Null by default: */
    return 0;
}

QITreeWidget* UIMediumManagerWidget::treeWidget(UIMediumType type) const
{
    /* Determine tab index for passed medium type: */
    int iIndex = tabIndex(type);

    /* Return tree-widget for known tab index: */
    if (iIndex >= 0 && iIndex < m_iTabCount)
        return m_trees.value(iIndex, 0);

    /* Null by default: */
    return 0;
}

UIMediumItem* UIMediumManagerWidget::mediumItem(UIMediumType type) const
{
    /* Get corresponding tree-widget: */
    QITreeWidget *pTreeWidget = treeWidget(type);
    /* Return corresponding medium-item: */
    return pTreeWidget ? toMediumItem(pTreeWidget->currentItem()) : 0;
}

UIMediumType UIMediumManagerWidget::mediumType(QITreeWidget *pTreeWidget) const
{
    /* Determine tab index of passed tree-widget: */
    int iIndex = m_trees.key(pTreeWidget, -1);

    /* Return medium type for known tab index: */
    if (iIndex >= 0 && iIndex < m_iTabCount)
        return (UIMediumType)iIndex;

    /* Invalid by default: */
    AssertFailedReturn(UIMediumType_Invalid);
}

UIMediumType UIMediumManagerWidget::currentMediumType() const
{
    /* Invalid if tab-widget doesn't exists: */
    if (!m_pTabWidget)
        return UIMediumType_Invalid;

    /* Return current medium type: */
    return (UIMediumType)m_pTabWidget->currentIndex();
}

QITreeWidget* UIMediumManagerWidget::currentTreeWidget() const
{
    /* Return current tree-widget: */
    return treeWidget(currentMediumType());
}

UIMediumItem* UIMediumManagerWidget::currentMediumItem() const
{
    /* Return current medium-item: */
    return mediumItem(currentMediumType());
}

void UIMediumManagerWidget::setCurrentItem(QITreeWidget *pTreeWidget, QTreeWidgetItem *pItem)
{
    /* Make sure passed tree-widget is valid: */
    AssertPtrReturnVoid(pTreeWidget);

    /* Make passed item current for passed tree-widget: */
    pTreeWidget->setCurrentItem(pItem);

    /* If non NULL item was passed: */
    if (pItem)
    {
        /* Make sure it's also selected, and visible: */
        pItem->setSelected(true);
        pTreeWidget->scrollToItem(pItem, QAbstractItemView::EnsureVisible);
    }

    /* Re-fetch currently chosen medium-item: */
    refetchCurrentChosenMediumItem();
}

/* static */
int UIMediumManagerWidget::tabIndex(UIMediumType type)
{
    /* Return tab index corresponding to known medium type: */
    switch (type)
    {
        case UIMediumType_HardDisk: return 0;
        case UIMediumType_DVD:      return 1;
        case UIMediumType_Floppy:   return 2;
        default: break;
    }

    /* -1 by default: */
    return -1;
}

/* static */
UIMediumItem* UIMediumManagerWidget::searchItem(QITreeWidget *pTreeWidget, const CheckIfSuitableBy &condition, CheckIfSuitableBy *pException)
{
    /* Make sure argument is valid: */
    if (!pTreeWidget)
        return 0;

    /* Return wrapper: */
    return searchItem(pTreeWidget->invisibleRootItem(), condition, pException);
}

/* static */
UIMediumItem* UIMediumManagerWidget::searchItem(QTreeWidgetItem *pParentItem, const CheckIfSuitableBy &condition, CheckIfSuitableBy *pException)
{
    /* Make sure argument is valid: */
    if (!pParentItem)
        return 0;

    /* Verify passed item if it is of 'medium' type too: */
    if (UIMediumItem *pMediumParentItem = toMediumItem(pParentItem))
        if (   condition.isItSuitable(pMediumParentItem)
            && (!pException || !pException->isItSuitable(pMediumParentItem)))
            return pMediumParentItem;

    /* Iterate other all the children: */
    for (int iChildIndex = 0; iChildIndex < pParentItem->childCount(); ++iChildIndex)
        if (UIMediumItem *pMediumChildItem = toMediumItem(pParentItem->child(iChildIndex)))
            if (UIMediumItem *pRequiredMediumChildItem = searchItem(pMediumChildItem, condition, pException))
                return pRequiredMediumChildItem;

    /* Null by default: */
    return 0;
}

/* static */
bool UIMediumManagerWidget::checkMediumFor(UIMediumItem *pItem, Action action)
{
    /* Make sure passed ID is valid: */
    AssertReturn(pItem, false);

    switch (action)
    {
        case Action_Edit:
        {
            /* Edit means changing the description and alike; any media that is
             * not being read to or written from can be altered in these terms. */
            switch (pItem->state())
            {
                case KMediumState_NotCreated:
                case KMediumState_Inaccessible:
                case KMediumState_LockedRead:
                case KMediumState_LockedWrite:
                    return false;
                default:
                    break;
            }
            return true;
        }
        case Action_Copy:
        {
            /* False for children: */
            return pItem->medium().parentID() == UIMedium::nullID();
        }
        case Action_Remove:
        {
            /* Removable if not attached to anything: */
            return !pItem->isUsed();
        }
        case Action_Release:
        {
            /* Releasable if attached but not in snapshots: */
            return pItem->isUsed() && !pItem->isUsedInSnapshots();
        }

        default:
            break;
    }

    AssertFailedReturn(false);
}

/* static */
UIMediumItem* UIMediumManagerWidget::toMediumItem(QTreeWidgetItem *pItem)
{
    /* Cast passed QTreeWidgetItem to UIMediumItem if possible: */
    return pItem && pItem->type() == QITreeWidgetItem::ItemType ? static_cast<UIMediumItem*>(pItem) : 0;
}


/*********************************************************************************************************************************
*   Class UIMediumManagerFactory implementation.                                                                                 *
*********************************************************************************************************************************/

void UIMediumManagerFactory::create(QIManagerDialog *&pDialog, QWidget *pCenterWidget)
{
    pDialog = new UIMediumManager(pCenterWidget);
}


/*********************************************************************************************************************************
*   Class UIMediumManagerFactory implementation.                                                                                 *
*********************************************************************************************************************************/

UIMediumManager::UIMediumManager(QWidget *pCenterWidget)
    : QIWithRetranslateUI<QIManagerDialog>(pCenterWidget)
    , m_pProgressBar(0)
{
}

void UIMediumManager::sltHandleButtonBoxClick(QAbstractButton *pButton)
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

void UIMediumManager::retranslateUi()
{
    /* Translate window title: */
    setWindowTitle(tr("Virtual Media Manager"));

    /* Translate buttons: */
    button(ButtonType_Reset)->setText(tr("Reset"));
    button(ButtonType_Apply)->setText(tr("Apply"));
    button(ButtonType_Close)->setText(tr("Close"));
    button(ButtonType_Reset)->setStatusTip(tr("Reset changes in current medium details"));
    button(ButtonType_Apply)->setStatusTip(tr("Apply changes in current medium details"));
    button(ButtonType_Close)->setStatusTip(tr("Close dialog without saving"));
    button(ButtonType_Reset)->setShortcut(QString("Ctrl+Backspace"));
    button(ButtonType_Apply)->setShortcut(QString("Ctrl+Return"));
    button(ButtonType_Close)->setShortcut(Qt::Key_Escape);
    button(ButtonType_Reset)->setToolTip(tr("Reset Changes (%1)").arg(button(ButtonType_Reset)->shortcut().toString()));
    button(ButtonType_Apply)->setToolTip(tr("Apply Changes (%1)").arg(button(ButtonType_Apply)->shortcut().toString()));
    button(ButtonType_Close)->setToolTip(tr("Close Window (%1)").arg(button(ButtonType_Close)->shortcut().toString()));
}

void UIMediumManager::configure()
{
    /* Apply window icons: */
    setWindowIcon(UIIconPool::iconSetFull(":/diskimage_32px.png", ":/diskimage_16px.png"));
}

void UIMediumManager::configureCentralWidget()
{
    /* Create widget: */
    UIMediumManagerWidget *pWidget = new UIMediumManagerWidget(EmbedTo_Dialog, this);
    AssertPtrReturnVoid(pWidget);
    {
        /* Configure widget: */
        setWidget(pWidget);
        setWidgetMenu(pWidget->menu());
#ifdef VBOX_WS_MAC
        setWidgetToolbar(pWidget->toolbar());
#endif
        connect(this, &UIMediumManager::sigDataChangeRejected,
                pWidget, &UIMediumManagerWidget::sltResetMediumDetailsChanges);
        connect(this, &UIMediumManager::sigDataChangeAccepted,
                pWidget, &UIMediumManagerWidget::sltApplyMediumDetailsChanges);

        /* Add into layout: */
        centralWidget()->layout()->addWidget(pWidget);
    }
}

void UIMediumManager::configureButtonBox()
{
    /* Configure button-box: */
    connect(widget(), &UIMediumManagerWidget::sigMediumDetailsVisibilityChanged,
            button(ButtonType_Apply), &QPushButton::setVisible);
    connect(widget(), &UIMediumManagerWidget::sigMediumDetailsVisibilityChanged,
            button(ButtonType_Reset), &QPushButton::setVisible);
    connect(widget(), &UIMediumManagerWidget::sigAcceptAllowed,
            button(ButtonType_Apply), &QPushButton::setEnabled);
    connect(widget(), &UIMediumManagerWidget::sigRejectAllowed,
            button(ButtonType_Reset), &QPushButton::setEnabled);
    connect(buttonBox(), &QIDialogButtonBox::clicked,
            this, &UIMediumManager::sltHandleButtonBoxClick);
    // WORKAROUND:
    // Since we connected signals later than extra-data loaded
    // for signals above, we should handle that stuff here again:
    button(ButtonType_Apply)->setVisible(gEDataManager->virtualMediaManagerDetailsExpanded());
    button(ButtonType_Reset)->setVisible(gEDataManager->virtualMediaManagerDetailsExpanded());

    /* Create progress-bar: */
    m_pProgressBar = new UIEnumerationProgressBar;
    AssertPtrReturnVoid(m_pProgressBar);
    {
        /* Configure progress-bar: */
        m_pProgressBar->hide();
        /* Add progress-bar into button-box layout: */
        buttonBox()->addExtraWidget(m_pProgressBar);
        /* Notify widget it has progress-bar: */
        widget()->setProgressBar(m_pProgressBar);
    }
}

void UIMediumManager::finalize()
{
    /* Apply language settings: */
    retranslateUi();
}

UIMediumManagerWidget *UIMediumManager::widget()
{
    return qobject_cast<UIMediumManagerWidget*>(QIManagerDialog::widget());
}

