/* $Id: UIWizardCloneVDPageBasic3.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPageBasic3 class declaration.
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

#ifndef ___UIWizardCloneVDPageBasic3_h___
#define ___UIWizardCloneVDPageBasic3_h___

/* GUI includes: */
#include "UIWizardPage.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class QButtonGroup;
class QRadioButton;
class QCheckBox;
class QIRichTextLabel;


/** 3rd page of the Clone Virtual Disk Image wizard (base part): */
class UIWizardCloneVDPage3 : public UIWizardPageBase
{
protected:

    /** Constructs page basis. */
    UIWizardCloneVDPage3();

    /** Returns 'mediumVariant' field value. */
    qulonglong mediumVariant() const;
    /** Defines 'mediumVariant' field value. */
    void setMediumVariant(qulonglong uMediumVariant);

    /** Holds the variant button-group instance. */
    QButtonGroup *m_pVariantButtonGroup;
    /** Holds the 'Dynamical' button instance. */
    QRadioButton *m_pDynamicalButton;
    /** Holds the 'Fixed' button instance. */
    QRadioButton *m_pFixedButton;
    /** Holds the 'Split to 2GB files' check-box instance. */
    QCheckBox    *m_pSplitBox;
};


/** 3rd page of the Clone Virtual Disk Image wizard (basic extension): */
class UIWizardCloneVDPageBasic3 : public UIWizardPage, public UIWizardCloneVDPage3
{
    Q_OBJECT;
    Q_PROPERTY(qulonglong mediumVariant READ mediumVariant WRITE setMediumVariant);

public:

    /** Constructs basic page. */
    UIWizardCloneVDPageBasic3(KDeviceType enmDeviceType);

private:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Prepares the page. */
    virtual void initializePage() /* override */;

    /** Returns whether the page is complete. */
    virtual bool isComplete() const /* override */;

    /** Holds the description label instance. */
    QIRichTextLabel *m_pDescriptionLabel;
    /** Holds the 'Dynamic' description label instance. */
    QIRichTextLabel *m_pDynamicLabel;
    /** Holds the 'Fixed' description label instance. */
    QIRichTextLabel *m_pFixedLabel;
    /** Holds the 'Split to 2GB files' description label instance. */
    QIRichTextLabel *m_pSplitLabel;
};

#endif /* !___UIWizardCloneVDPageBasic3_h___ */

