/* $Id: UIWizardCloneVMPageBasic3.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVMPageBasic3 class declaration.
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

#ifndef __UIWizardCloneVMPageBasic3_h__
#define __UIWizardCloneVMPageBasic3_h__

/* GUI includes: */
#include "UIWizardPage.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declaration: */
class QRadioButton;
class QIRichTextLabel;

/* 3rd page of the Clone Virtual Machine wizard (base part): */
class UIWizardCloneVMPage3 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardCloneVMPage3(bool fShowChildsOption);

    /* Stuff for 'cloneMode' field: */
    KCloneMode cloneMode() const;
    void setCloneMode(KCloneMode cloneMode);

    /* Variables: */
    bool m_fShowChildsOption;

    /* Widgets: */
    QRadioButton *m_pMachineRadio;
    QRadioButton *m_pMachineAndChildsRadio;
    QRadioButton *m_pAllRadio;
};

/* 3rd page of the Clone Virtual Machine wizard (basic extension): */
class UIWizardCloneVMPageBasic3 : public UIWizardPage, public UIWizardCloneVMPage3
{
    Q_OBJECT;
    Q_PROPERTY(KCloneMode cloneMode READ cloneMode WRITE setCloneMode);

public:

    /* Constructor: */
    UIWizardCloneVMPageBasic3(bool fShowChildsOption);

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool validatePage();

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif // __UIWizardCloneVMPageBasic3_h__

