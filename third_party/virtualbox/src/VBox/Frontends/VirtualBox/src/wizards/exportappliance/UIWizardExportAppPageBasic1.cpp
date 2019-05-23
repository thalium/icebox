/* $Id: UIWizardExportAppPageBasic1.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageBasic1 class implementation.
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
# include <QVBoxLayout>

/* Local includes: */
# include "UIWizardExportAppPageBasic1.h"
# include "UIWizardExportApp.h"
# include "UIWizardExportAppDefs.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"
# include "QILabelSeparator.h"
# include "QIRichTextLabel.h"

/* COM includes: */
# include "CMachine.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardExportAppPage1::UIWizardExportAppPage1()
{
}

void UIWizardExportAppPage1::populateVMSelectorItems(const QStringList &selectedVMNames)
{
    /* Add all VM items into 'VM Selector': */
    foreach (const CMachine &machine, vboxGlobal().virtualBox().GetMachines())
    {
        QPixmap pixIcon;
        QString strName;
        QString strUuid;
        bool fInSaveState = false;
        bool fEnabled = false;
        const QStyle *pStyle = QApplication::style();
        const int iIconMetric = pStyle->pixelMetric(QStyle::PM_SmallIconSize);
        if (machine.GetAccessible())
        {
            pixIcon = vboxGlobal().vmUserPixmapDefault(machine);
            if (pixIcon.isNull())
                pixIcon = vboxGlobal().vmGuestOSTypePixmapDefault(machine.GetOSTypeId());
            strName = machine.GetName();
            strUuid = machine.GetId();
            fEnabled = machine.GetSessionState() == KSessionState_Unlocked;
            fInSaveState = machine.GetState() == KMachineState_Saved;
        }
        else
        {
            QString settingsFile = machine.GetSettingsFilePath();
            QFileInfo fi(settingsFile);
            strName = VBoxGlobal::hasAllowedExtension(fi.completeSuffix(), VBoxFileExts) ? fi.completeBaseName() : fi.fileName();
            pixIcon = QPixmap(":/os_other.png").scaled(iIconMetric, iIconMetric, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        QListWidgetItem *pItem = new VMListWidgetItem(pixIcon, strName, strUuid, fInSaveState, m_pVMSelector);
        if (!fEnabled)
            pItem->setFlags(0);
        m_pVMSelector->addItem(pItem);
    }
    m_pVMSelector->sortItems();

    /* Choose initially selected items (if passed): */
    for (int i = 0; i < selectedVMNames.size(); ++i)
    {
        QList<QListWidgetItem*> list = m_pVMSelector->findItems(selectedVMNames[i], Qt::MatchExactly);
        if (list.size() > 0)
        {
            if (m_pVMSelector->selectedItems().isEmpty())
                m_pVMSelector->setCurrentItem(list.first());
            else
                list.first()->setSelected(true);
        }
    }
}

QStringList UIWizardExportAppPage1::machineNames() const
{
    /* Prepare list: */
    QStringList machineNames;
    /* Iterate over all the selected items: */
    foreach (QListWidgetItem *pItem, m_pVMSelector->selectedItems())
        machineNames << pItem->text();
    /* Return result list: */
    return machineNames;
}

QStringList UIWizardExportAppPage1::machineIDs() const
{
    /* Prepare list: */
    QStringList machineIDs;
    /* Iterate over all the selected items: */
    foreach (QListWidgetItem *pItem, m_pVMSelector->selectedItems())
        machineIDs << static_cast<VMListWidgetItem*>(pItem)->uuid();
    /* Return result list: */
    return machineIDs;
}

UIWizardExportAppPageBasic1::UIWizardExportAppPageBasic1(const QStringList &selectedVMNames)
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        m_pVMSelector = new QListWidget(this);
        {
            m_pVMSelector->setAlternatingRowColors(true);
            m_pVMSelector->setSelectionMode(QAbstractItemView::ExtendedSelection);
        }
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addWidget(m_pVMSelector);
        populateVMSelectorItems(selectedVMNames);
    }

    /* Setup connections: */
    connect(m_pVMSelector, SIGNAL(itemSelectionChanged()), this, SIGNAL(completeChanged()));

    /* Register fields: */
    registerField("machineNames", this, "machineNames");
    registerField("machineIDs", this, "machineIDs");
}

void UIWizardExportAppPageBasic1::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardExportApp::tr("Virtual machines to export"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardExportApp::tr("<p>Please select the virtual machines that should be added to the appliance. "
                                            "You can select more than one. Please note that these machines have to be "
                                            "turned off before they can be exported.</p>"));
}

void UIWizardExportAppPageBasic1::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardExportAppPageBasic1::isComplete() const
{
    /* There should be at least one vm selected: */
    return m_pVMSelector->selectedItems().size() > 0;
}

bool UIWizardExportAppPageBasic1::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Ask user about machines which are in save state currently: */
    QStringList savedMachines;
    QList<QListWidgetItem*> pItems = m_pVMSelector->selectedItems();
    for (int i=0; i < pItems.size(); ++i)
    {
        if (static_cast<VMListWidgetItem*>(pItems.at(i))->isInSaveState())
            savedMachines << pItems.at(i)->text();
    }
    if (!savedMachines.isEmpty())
        fResult = msgCenter().confirmExportMachinesInSaveState(savedMachines, this);

    /* Return result: */
    return fResult;
}

int UIWizardExportAppPageBasic1::nextId() const
{
    /* Skip next (2nd, storage-type) page for now! */
    return UIWizardExportApp::Page3;
}

