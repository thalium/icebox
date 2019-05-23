/* $Id: UIWizardCloneVMPageBasic2.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVMPageBasic2 class declaration.
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

#ifndef __UIWizardCloneVMPageBasic2_h__
#define __UIWizardCloneVMPageBasic2_h__

/* Local includes: */
#include "UIWizardPage.h"

/* Forward declarations: */
class QButtonGroup;
class QRadioButton;
class QIRichTextLabel;

/* 2nd page of the Clone Virtual Machine wizard (base part): */
class UIWizardCloneVMPage2 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardCloneVMPage2(bool fAdditionalInfo);

    /* Stuff for 'linkedClone' field: */
    bool isLinkedClone() const;

    /* Variables: */
    bool m_fAdditionalInfo;

    /* Widgets: */
    QButtonGroup *m_pButtonGroup;
    QRadioButton *m_pFullCloneRadio;
    QRadioButton *m_pLinkedCloneRadio;
};

/* 2nd page of the Clone Virtual Machine wizard (basic extension): */
class UIWizardCloneVMPageBasic2 : public UIWizardPage, public UIWizardCloneVMPage2
{
    Q_OBJECT;
    Q_PROPERTY(bool linkedClone READ isLinkedClone);

public:

    /* Constructor: */
    UIWizardCloneVMPageBasic2(bool fAdditionalInfo);

private slots:

    /* Button click handler: */
    void sltButtonClicked(QAbstractButton *pButton);

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool validatePage();

    /* Navigation stuff: */
    int nextId() const;

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif // __UIWizardCloneVMPageBasic2_h__

