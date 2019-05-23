/* $Id: UIApplianceImportEditorWidget.cpp $ */
/** @file
 * VBox Qt GUI - UIApplianceImportEditorWidget class implementation.
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
# include <QCheckBox>
# include <QTextEdit>

/* GUI includes: */
# include "QITreeView.h"
# include "UIApplianceImportEditorWidget.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"

/* COM includes: */
# include "CAppliance.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


////////////////////////////////////////////////////////////////////////////////
// ImportSortProxyModel

class ImportSortProxyModel: public UIApplianceSortProxyModel
{
public:
    ImportSortProxyModel(QObject *pParent = NULL)
      : UIApplianceSortProxyModel(pParent)
    {
        m_aFilteredList << KVirtualSystemDescriptionType_License;
    }
};

////////////////////////////////////////////////////////////////////////////////
// UIApplianceImportEditorWidget

UIApplianceImportEditorWidget::UIApplianceImportEditorWidget(QWidget *pParent)
    : UIApplianceEditorWidget(pParent)
{
    /* Show the MAC check box */
    m_pCheckBoxReinitMACs->setHidden(false);
}

bool UIApplianceImportEditorWidget::setFile(const QString& strFile)
{
    bool fResult = false;
    if (!strFile.isEmpty())
    {
        CProgress progress;
        CVirtualBox vbox = vboxGlobal().virtualBox();
        /* Create a appliance object */
        m_pAppliance = new CAppliance(vbox.CreateAppliance());
        fResult = m_pAppliance->isOk();
        if (fResult)
        {
            /* Read the appliance */
            progress = m_pAppliance->Read(strFile);
            fResult = m_pAppliance->isOk();
            if (fResult)
            {
                /* Show some progress, so the user know whats going on */
                msgCenter().showModalProgressDialog(progress, tr("Reading Appliance ..."), ":/progress_reading_appliance_90px.png", this);
                if (!progress.isOk() || progress.GetResultCode() != 0)
                    fResult = false;
                else
                {
                    /* Now we have to interpret that stuff */
                    m_pAppliance->Interpret();
                    fResult = m_pAppliance->isOk();
                    if (fResult)
                    {
                        if (m_pModel)
                            delete m_pModel;

                        QVector<CVirtualSystemDescription> vsds = m_pAppliance->GetVirtualSystemDescriptions();

                        m_pModel = new UIApplianceModel(vsds, m_pTreeViewSettings);

                        ImportSortProxyModel *pProxy = new ImportSortProxyModel(this);
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
                }
            }
        }
        if (!fResult)
        {
            if (!m_pAppliance->isOk())
                msgCenter().cannotImportAppliance(*m_pAppliance, this);
            else if (!progress.isNull() && (!progress.isOk() || progress.GetResultCode() != 0))
                msgCenter().cannotImportAppliance(progress, m_pAppliance->GetPath(), this);
            /* Delete the appliance in a case of an error */
            delete m_pAppliance;
            m_pAppliance = NULL;
        }
    }
    return fResult;
}

void UIApplianceImportEditorWidget::prepareImport()
{
    if (m_pAppliance)
        m_pModel->putBack();
}

bool UIApplianceImportEditorWidget::import()
{
    if (m_pAppliance)
    {
        /* Start the import asynchronously */
        CProgress progress;
        QVector<KImportOptions> options;
        if (!m_pCheckBoxReinitMACs->isChecked())
            options.append(KImportOptions_KeepAllMACs);
        progress = m_pAppliance->ImportMachines(options);
        bool fResult = m_pAppliance->isOk();
        if (fResult)
        {
            /* Show some progress, so the user know whats going on */
            msgCenter().showModalProgressDialog(progress, tr("Importing Appliance ..."), ":/progress_import_90px.png", this);
            if (progress.GetCanceled())
                return false;
            if (!progress.isOk() || progress.GetResultCode() != 0)
            {
                msgCenter().cannotImportAppliance(progress, m_pAppliance->GetPath(), this);
                return false;
            }
            else
                return true;
        }
        if (!fResult)
            msgCenter().cannotImportAppliance(*m_pAppliance, this);
    }
    return false;
}

QList<QPair<QString, QString> > UIApplianceImportEditorWidget::licenseAgreements() const
{
    QList<QPair<QString, QString> > list;

    CVirtualSystemDescriptionVector vsds = m_pAppliance->GetVirtualSystemDescriptions();
    for (int i = 0; i < vsds.size(); ++i)
    {
        QVector<QString> strLicense;
        strLicense = vsds[i].GetValuesByType(KVirtualSystemDescriptionType_License,
                                             KVirtualSystemDescriptionValueType_Original);
        if (!strLicense.isEmpty())
        {
            QVector<QString> strName;
            strName = vsds[i].GetValuesByType(KVirtualSystemDescriptionType_Name,
                                              KVirtualSystemDescriptionValueType_Auto);
            list << QPair<QString, QString>(strName.first(), strLicense.first());
        }
    }

    return list;
}

