/* $Id: UIWizardImportAppPageExpert.h $ */
/** @file
 * VBox Qt GUI - UIWizardImportAppPageExpert class declaration.
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

#ifndef __UIWizardImportAppPageExpert_h__
#define __UIWizardImportAppPageExpert_h__

/* Local includes: */
#include "UIWizardImportAppPageBasic1.h"
#include "UIWizardImportAppPageBasic2.h"

/* Forward declarations: */
class QGroupBox;

/* Expert page of the Import Appliance wizard: */
class UIWizardImportAppPageExpert : public UIWizardPage,
                                    public UIWizardImportAppPage1,
                                    public UIWizardImportAppPage2
{
    Q_OBJECT;
    Q_PROPERTY(ImportAppliancePointer applianceWidget READ applianceWidget);

public:

    /* Constructor: */
    UIWizardImportAppPageExpert(const QString &strFileName);

private slots:

    /* File-path change handler: */
    void sltFilePathChangeHandler();

private:

    /* Translate stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;
    bool validatePage();

    /* Widgets: */
    QGroupBox *m_pApplianceCnt;
    QGroupBox *m_pSettingsCnt;
};

#endif /* __UIWizardImportAppPageExpert_h__ */

