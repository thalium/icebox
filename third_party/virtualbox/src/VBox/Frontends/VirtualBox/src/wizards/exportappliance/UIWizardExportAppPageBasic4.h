/* $Id: UIWizardExportAppPageBasic4.h $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageBasic4 class declaration.
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

#ifndef __UIWizardExportAppPageBasic4_h__
#define __UIWizardExportAppPageBasic4_h__

/* Global includes: */
#include <QVariant>

/* Local includes: */
#include "UIWizardPage.h"
#include "UIWizardExportAppDefs.h"

/* Forward declarations: */
class UIWizardExportApp;
class QIRichTextLabel;

/* 4th page of the Export Appliance wizard (base part): */
class UIWizardExportAppPage4 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardExportAppPage4();

    /* Helpers: */
    void refreshApplianceSettingsWidget(    );

    /* Stuff for 'applianceWidget' field: */
    ExportAppliancePointer applianceWidget() const { return m_pApplianceWidget; }

    /* Widgets: */
    UIApplianceExportEditorWidget *m_pApplianceWidget;
};

/* 4th page of the Export Appliance wizard (basic extension): */
class UIWizardExportAppPageBasic4 : public UIWizardPage, public UIWizardExportAppPage4
{
    Q_OBJECT;
    Q_PROPERTY(ExportAppliancePointer applianceWidget READ applianceWidget);

public:

    /* Constructor: */
    UIWizardExportAppPageBasic4();

protected:

    /* Wrapper to access 'wizard' from base part: */
    UIWizard* wizardImp() { return UIWizardPage::wizard(); }
    /* Wrapper to access 'this' from base part: */
    UIWizardPage* thisImp() { return this; }
    /* Wrapper to access 'wizard-field' from base part: */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

private:

    /* Translate stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool validatePage();

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif /* __UIWizardExportAppPageBasic4_h__ */

