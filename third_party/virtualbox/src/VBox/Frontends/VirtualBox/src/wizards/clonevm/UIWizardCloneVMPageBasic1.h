/* $Id: UIWizardCloneVMPageBasic1.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVMPageBasic1 class declaration.
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

#ifndef __UIWizardCloneVMPageBasic1_h__
#define __UIWizardCloneVMPageBasic1_h__

/* Local includes: */
#include "UIWizardPage.h"

/* Forward declarations: */
class QLineEdit;
class QCheckBox;
class QIRichTextLabel;

/* 1st page of the Clone Virtual Machine wizard (base part): */
class UIWizardCloneVMPage1 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardCloneVMPage1(const QString &strOriginalName);

    /* Stuff for 'cloneName' field: */
    QString cloneName() const;
    void setCloneName(const QString &strName);

    /* Stuff for 'reinitMACs' field: */
    bool isReinitMACsChecked() const;

    /* Variables: */
    QString m_strOriginalName;

    /* Widgets: */
    QLineEdit *m_pNameEditor;
    QCheckBox *m_pReinitMACsCheckBox;
};

/* 1st page of the Clone Virtual Machine wizard (basic extension): */
class UIWizardCloneVMPageBasic1 : public UIWizardPage, public UIWizardCloneVMPage1
{
    Q_OBJECT;
    Q_PROPERTY(QString cloneName READ cloneName WRITE setCloneName);
    Q_PROPERTY(bool reinitMACs READ isReinitMACsChecked);

public:

    /* Constructor: */
    UIWizardCloneVMPageBasic1(const QString &strOriginalName);

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif // __UIWizardCloneVMPageBasic1_h__

