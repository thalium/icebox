/* $Id: UIWizardNewVMPageExpert.h $ */
/** @file
 * VBox Qt GUI - UIWizardNewVMPageExpert class declaration.
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

#ifndef __UIWizardNewVMPageExpert_h__
#define __UIWizardNewVMPageExpert_h__

/* Local includes: */
#include "UIWizardNewVMPageBasic1.h"
#include "UIWizardNewVMPageBasic2.h"
#include "UIWizardNewVMPageBasic3.h"

/* Forward declarations: */
class QGroupBox;

/* Expert page of the New Virtual Machine wizard: */
class UIWizardNewVMPageExpert : public UIWizardPage,
                                public UIWizardNewVMPage1,
                                public UIWizardNewVMPage2,
                                public UIWizardNewVMPage3
{
    Q_OBJECT;
    Q_PROPERTY(QString machineFolder READ machineFolder WRITE setMachineFolder);
    Q_PROPERTY(QString machineBaseName READ machineBaseName WRITE setMachineBaseName);
    Q_PROPERTY(CMedium virtualDisk READ virtualDisk WRITE setVirtualDisk);
    Q_PROPERTY(QString virtualDiskId READ virtualDiskId WRITE setVirtualDiskId);
    Q_PROPERTY(QString virtualDiskLocation READ virtualDiskLocation WRITE setVirtualDiskLocation);

public:

    /* Constructor: */
    UIWizardNewVMPageExpert(const QString &strGroup);

protected:

    /* Wrapper to access 'wizard' from base part: */
    UIWizard* wizardImp() { return wizard(); }
    /* Wrapper to access 'this' from base part: */
    UIWizardPage* thisImp() { return this; }
    /* Wrapper to access 'wizard-field' from base part: */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

private slots:

    /* Handlers: */
    void sltNameChanged(const QString &strNewText);
    void sltOsTypeChanged();
    void sltRamSliderValueChanged();
    void sltRamEditorValueChanged();
    void sltVirtualDiskSourceChanged();
    void sltGetWithFileOpenDialog();

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();
    void cleanupPage();

    /* Validation stuff: */
    bool isComplete() const;
    bool validatePage();

    /* Widgets: */
    QGroupBox *m_pNameAndSystemCnt;
    QGroupBox *m_pMemoryCnt;
    QGroupBox *m_pDiskCnt;
};

#endif // __UIWizardNewVMPageExpert_h__

