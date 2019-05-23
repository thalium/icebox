/* $Id: UIApplianceExportEditorWidget.cpp $ */
/** @file
 * VBox Qt GUI - UIApplianceExportEditorWidget class implementation.
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
# include <QTextEdit>

/* GUI includes: */
# include "QITreeView.h"
# include "UIApplianceExportEditorWidget.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"

/* COM includes: */
# include "CAppliance.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


////////////////////////////////////////////////////////////////////////////////
// ExportSortProxyModel

class ExportSortProxyModel: public UIApplianceSortProxyModel
{
public:
    ExportSortProxyModel(QObject *pParent = NULL)
      : UIApplianceSortProxyModel(pParent)
    {
        m_aFilteredList
            << KVirtualSystemDescriptionType_OS
            << KVirtualSystemDescriptionType_CPU
            << KVirtualSystemDescriptionType_Memory
            << KVirtualSystemDescriptionType_Floppy
            << KVirtualSystemDescriptionType_CDROM
            << KVirtualSystemDescriptionType_USBController
            << KVirtualSystemDescriptionType_SoundCard
            << KVirtualSystemDescriptionType_NetworkAdapter
            << KVirtualSystemDescriptionType_HardDiskControllerIDE
            << KVirtualSystemDescriptionType_HardDiskControllerSATA
            << KVirtualSystemDescriptionType_HardDiskControllerSCSI
            << KVirtualSystemDescriptionType_HardDiskControllerSAS;
    }
};

////////////////////////////////////////////////////////////////////////////////
// UIApplianceExportEditorWidget

UIApplianceExportEditorWidget::UIApplianceExportEditorWidget(QWidget *pParent /* = NULL */)
  : UIApplianceEditorWidget(pParent)
{
}

CAppliance* UIApplianceExportEditorWidget::init()
{
    if (m_pAppliance)
        delete m_pAppliance;
    CVirtualBox vbox = vboxGlobal().virtualBox();
    /* Create a appliance object */
    m_pAppliance = new CAppliance(vbox.CreateAppliance());
//    bool fResult = m_pAppliance->isOk();
    return m_pAppliance;
}

void UIApplianceExportEditorWidget::populate()
{
    if (m_pModel)
        delete m_pModel;

    QVector<CVirtualSystemDescription> vsds = m_pAppliance->GetVirtualSystemDescriptions();

    m_pModel = new UIApplianceModel(vsds, m_pTreeViewSettings);

    ExportSortProxyModel *pProxy = new ExportSortProxyModel(this);
    pProxy->setSourceModel(m_pModel);
    pProxy->sort(ApplianceViewSection_Description, Qt::DescendingOrder);

    UIApplianceDelegate *pDelegate = new UIApplianceDelegate(pProxy, this);

    /* Set our own model */
    m_pTreeViewSettings->setModel(pProxy);
    /* Set our own delegate */
    m_pTreeViewSettings->setItemDelegate(pDelegate);
    /* For now we hide the original column. This data is displayed as tooltip
       also. */
    m_pTreeViewSettings->setColumnHidden(ApplianceViewSection_OriginalValue, true);
    m_pTreeViewSettings->expandAll();
    /* Set model root index and make it current: */
    m_pTreeViewSettings->setRootIndex(pProxy->mapFromSource(m_pModel->root()));
    m_pTreeViewSettings->setCurrentIndex(pProxy->mapFromSource(m_pModel->root()));

    /* Check for warnings & if there are one display them. */
    bool fWarningsEnabled = false;
    QVector<QString> warnings = m_pAppliance->GetWarnings();
    if (warnings.size() > 0)
    {
        foreach (const QString& text, warnings)
            m_pTextEditWarning->append("- " + text);
        fWarningsEnabled = true;
    }
    m_pPaneWarning->setVisible(fWarningsEnabled);
}

void UIApplianceExportEditorWidget::prepareExport()
{
    if (m_pAppliance)
        m_pModel->putBack();
}

