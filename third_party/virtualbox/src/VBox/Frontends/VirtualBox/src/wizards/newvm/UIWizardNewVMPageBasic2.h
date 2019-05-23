/* $Id: UIWizardNewVMPageBasic2.h $ */
/** @file
 * VBox Qt GUI - UIWizardNewVMPageBasic2 class declaration.
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

#ifndef __UIWizardNewVMPageBasic2_h__
#define __UIWizardNewVMPageBasic2_h__

/* Local includes: */
#include "UIWizardPage.h"

/* Forward declarations: */
class VBoxGuestRAMSlider;
class QSpinBox;
class QLabel;
class QIRichTextLabel;

/* 2nd page of the New Virtual Machine wizard (base part): */
class UIWizardNewVMPage2 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardNewVMPage2();

    /* Handlers: */
    void onRamSliderValueChanged();
    void onRamEditorValueChanged();

    /* Widgets: */
    VBoxGuestRAMSlider *m_pRamSlider;
    QSpinBox *m_pRamEditor;
    QLabel *m_pRamMin;
    QLabel *m_pRamMax;
    QLabel *m_pRamUnits;
};

/* 2nd page of the New Virtual Machine wizard (basic extension): */
class UIWizardNewVMPageBasic2 : public UIWizardPage, public UIWizardNewVMPage2
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIWizardNewVMPageBasic2();

private slots:

    /* Handlers: */
    void sltRamSliderValueChanged();
    void sltRamEditorValueChanged();

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

#endif // __UINewVMWzd_h__

