/* $Id: UIWizardExportAppPageBasic4.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageBasic4 class implementation.
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

/* GUI includes: */
# include "UIWizardExportAppPageBasic4.h"
# include "UIWizardExportApp.h"
# include "VBoxGlobal.h"
# include "UIMessageCenter.h"
# include "QILabelSeparator.h"
# include "QIRichTextLabel.h"

/* COM includes: */
# include "CAppliance.h"
# include "CMachine.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardExportAppPage4::UIWizardExportAppPage4()
{
}

void UIWizardExportAppPage4::refreshApplianceSettingsWidget()
{
    /* Refresh settings widget: */
    CVirtualBox vbox = vboxGlobal().virtualBox();
    CAppliance *pAppliance = m_pApplianceWidget->init();
    bool fResult = pAppliance->isOk();
    if (fResult)
    {
        /* Iterate over all the selected machine ids: */
        QStringList uuids = fieldImp("machineIDs").toStringList();
        foreach (const QString &uuid, uuids)
        {
            /* Get the machine with the uuid: */
            CMachine machine = vbox.FindMachine(uuid);
            fResult = machine.isOk();
            if (fResult)
            {
                /* Add the export description to our appliance object: */
                CVirtualSystemDescription vsd = machine.ExportTo(*pAppliance, qobject_cast<UIWizardExportApp*>(wizardImp())->uri());
                fResult = machine.isOk();
                if (!fResult)
                {
                    msgCenter().cannotExportAppliance(machine, pAppliance->GetPath(), thisImp());
                    return;
                }
                /* Now add some new fields the user may change: */
                vsd.AddDescription(KVirtualSystemDescriptionType_Product, "", "");
                vsd.AddDescription(KVirtualSystemDescriptionType_ProductUrl, "", "");
                vsd.AddDescription(KVirtualSystemDescriptionType_Vendor, "", "");
                vsd.AddDescription(KVirtualSystemDescriptionType_VendorUrl, "", "");
                vsd.AddDescription(KVirtualSystemDescriptionType_Version, "", "");
                vsd.AddDescription(KVirtualSystemDescriptionType_License, "", "");
            }
            else
                break;
        }
        /* Make sure the settings widget get the new descriptions: */
        m_pApplianceWidget->populate();
    }
    if (!fResult)
        msgCenter().cannotExportAppliance(*pAppliance, thisImp());
}

UIWizardExportAppPageBasic4::UIWizardExportAppPageBasic4()
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        m_pApplianceWidget = new UIApplianceExportEditorWidget(this);
        {
            m_pApplianceWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
        }
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addWidget(m_pApplianceWidget);
    }

    /* Register classes: */
    qRegisterMetaType<ExportAppliancePointer>();
    /* Register fields: */
    registerField("applianceWidget", this, "applianceWidget");
}

void UIWizardExportAppPageBasic4::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardExportApp::tr("Appliance settings"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardExportApp::tr("This is the descriptive information which will be added "
                                            "to the virtual appliance.  You can change it by double "
                                            "clicking on individual lines."));
}

void UIWizardExportAppPageBasic4::initializePage()
{
    /* Translate page: */
    retranslateUi();

    /* Refresh appliance settings widget: */
    refreshApplianceSettingsWidget();
}

bool UIWizardExportAppPageBasic4::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Lock finish button: */
    startProcessing();

    /* Try to export appliance: */
    if (fResult)
        fResult = qobject_cast<UIWizardExportApp*>(wizard())->exportAppliance();

    /* Unlock finish button: */
    endProcessing();

    /* Return result: */
    return fResult;
}

