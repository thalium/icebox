/* $Id: UIWizardExportAppPageBasic2.h $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageBasic2 class declaration.
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

#ifndef __UIWizardExportAppPageBasic2_h__
#define __UIWizardExportAppPageBasic2_h__

/* Local includes: */
#include "UIWizardPage.h"
#include "UIWizardExportAppDefs.h"

/* Forward declarations: */
class QGroupBox;
class QRadioButton;
class QIRichTextLabel;

/* 2nd page of the Export Appliance wizard (base part): */
class UIWizardExportAppPage2 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardExportAppPage2();

    /* Helpers: */
    void chooseDefaultStorageType();

    /* Stuff for 'storageType' field: */
    StorageType storageType() const;
    void setStorageType(StorageType storageType);

    /* Widgets: */
    QGroupBox *m_pTypeCnt;
    QRadioButton *m_pTypeLocalFilesystem;
    QRadioButton *m_pTypeSunCloud;
    QRadioButton *m_pTypeSimpleStorageSystem;
};

/* 2nd page of the Export Appliance wizard (basic extension): */
class UIWizardExportAppPageBasic2 : public UIWizardPage, public UIWizardExportAppPage2
{
    Q_OBJECT;
    Q_PROPERTY(StorageType storageType READ storageType WRITE setStorageType);

public:

    /* Constructor: */
    UIWizardExportAppPageBasic2();

private:

    /* Translate stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif /* __UIWizardExportAppPageBasic2_h__ */

