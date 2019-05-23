/* $Id: UIWizardImportAppPageBasic1.h $ */
/** @file
 * VBox Qt GUI - UIWizardImportAppPageBasic1 class declaration.
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

#ifndef __UIWizardImportAppPageBasic1_h__
#define __UIWizardImportAppPageBasic1_h__

/* Local includes: */
#include "UIWizardPage.h"

/* Forward declarations: */
class UIEmptyFilePathSelector;
class QIRichTextLabel;

/* 1st page of the Import Appliance wizard (base part): */
class UIWizardImportAppPage1 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardImportAppPage1();

    /* Widgets: */
    UIEmptyFilePathSelector *m_pFileSelector;
};

/* 1st page of the Import Appliance wizard (basic extension): */
class UIWizardImportAppPageBasic1 : public UIWizardPage, public UIWizardImportAppPage1
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIWizardImportAppPageBasic1();

private:

    /* Translate stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;
    bool validatePage();

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif /* __UIWizardImportAppPageBasic1_h__ */

