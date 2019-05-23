/* $Id: UIWizardCloneVMPageExpert.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVMPageExpert class declaration.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIWizardCloneVMPageExpert_h__
#define __UIWizardCloneVMPageExpert_h__

/* Local includes: */
#include "UIWizardCloneVMPageBasic1.h"
#include "UIWizardCloneVMPageBasic2.h"
#include "UIWizardCloneVMPageBasic3.h"

/* Forward declarations: */
class QGroupBox;

/* Expert page of the Clone Virtual Machine wizard: */
class UIWizardCloneVMPageExpert : public UIWizardPage,
                                  public UIWizardCloneVMPage1,
                                  public UIWizardCloneVMPage2,
                                  public UIWizardCloneVMPage3
{
    Q_OBJECT;
    Q_PROPERTY(QString cloneName READ cloneName WRITE setCloneName);
    Q_PROPERTY(bool reinitMACs READ isReinitMACsChecked);
    Q_PROPERTY(bool linkedClone READ isLinkedClone);
    Q_PROPERTY(KCloneMode cloneMode READ cloneMode WRITE setCloneMode);

public:

    /* Constructor: */
    UIWizardCloneVMPageExpert(const QString &strOriginalName, bool fAdditionalInfo, bool fShowChildsOption);

private slots:

    /* Button click handler: */
    void sltButtonClicked(QAbstractButton *pButton);

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;
    bool validatePage();

    /* Widgets: */
    QGroupBox *m_pNameCnt;
    QGroupBox *m_pCloneTypeCnt;
    QGroupBox *m_pCloneModeCnt;
};

#endif // __UIWizardCloneVMPageExpert_h__

