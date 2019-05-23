/* $Id: UIWizardExportAppPageExpert.h $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageExpert class declaration.
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

#ifndef __UIWizardExportAppPageExpert_h__
#define __UIWizardExportAppPageExpert_h__

/* Local includes: */
#include "UIWizardExportAppPageBasic1.h"
#include "UIWizardExportAppPageBasic2.h"
#include "UIWizardExportAppPageBasic3.h"
#include "UIWizardExportAppPageBasic4.h"

/* Forward declarations: */
class QGroupBox;

/* Expert page of the Export Appliance wizard: */
class UIWizardExportAppPageExpert : public UIWizardPage,
                                    public UIWizardExportAppPage1,
                                    public UIWizardExportAppPage2,
                                    public UIWizardExportAppPage3,
                                    public UIWizardExportAppPage4
{
    Q_OBJECT;
    Q_PROPERTY(QStringList machineNames READ machineNames);
    Q_PROPERTY(QStringList machineIDs READ machineIDs);
    Q_PROPERTY(StorageType storageType READ storageType WRITE setStorageType);
    Q_PROPERTY(QString format READ format WRITE setFormat);
    Q_PROPERTY(bool manifestSelected READ isManifestSelected WRITE setManifestSelected);
    Q_PROPERTY(QString username READ username WRITE setUserName);
    Q_PROPERTY(QString password READ password WRITE setPassword);
    Q_PROPERTY(QString hostname READ hostname WRITE setHostname);
    Q_PROPERTY(QString bucket READ bucket WRITE setBucket);
    Q_PROPERTY(QString path READ path WRITE setPath);
    Q_PROPERTY(ExportAppliancePointer applianceWidget READ applianceWidget);

public:

    /* Constructor: */
    UIWizardExportAppPageExpert(const QStringList &selectedVMNames);

protected:

    /* Wrapper to access 'wizard' from base part: */
    UIWizard* wizardImp() { return UIWizardPage::wizard(); }
    /* Wrapper to access 'this' from base part: */
    UIWizardPage* thisImp() { return this; }
    /* Wrapper to access 'wizard-field' from base part: */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

private slots:

    /* VM Selector selection change handler: */
    void sltVMSelectionChangeHandler();

    /* Storage-type change handler: */
    void sltStorageTypeChangeHandler();

    /* Format combo change handler: */
    void sltHandleFormatComboChange();

private:

    /* Field API: */
    QVariant field(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

    /* Translate stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;
    bool validatePage();

    /* Widgets: */
    QGroupBox *m_pSelectorCnt;
    QGroupBox *m_pApplianceCnt;
    QGroupBox *m_pSettingsCnt;
};

#endif /* __UIWizardExportAppPageExpert_h__ */

