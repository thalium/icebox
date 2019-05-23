/* $Id: UIWizardFirstRunPageBasic.h $ */
/** @file
 * VBox Qt GUI - UIWizardFirstRunPageBasic class declaration.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIWizardFirstRunPageBasic_h__
#define __UIWizardFirstRunPageBasic_h__

/* Local includes: */
#include "UIWizardPage.h"

/* Forward declarations: */
class CMachine;
class VBoxMediaComboBox;
class QIToolButton;
class QIRichTextLabel;

/* Single page of the First Run wizard (base part): */
class UIWizardFirstRunPage : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardFirstRunPage(bool fBootHardDiskWasSet);

    /* Handlers: */
    void onOpenMediumWithFileOpenDialog();

    /* Stuff for 'id' field: */
    QString id() const;
    void setId(const QString &strId);

    /* Variables: */
    bool m_fBootHardDiskWasSet;

    /* Widgets: */
    VBoxMediaComboBox *m_pMediaSelector;
    QIToolButton *m_pSelectMediaButton;
};

/* Single page of the First Run wizard (basic extension): */
class UIWizardFirstRunPageBasic : public UIWizardPage, public UIWizardFirstRunPage
{
    Q_OBJECT;
    Q_PROPERTY(QString source READ source);
    Q_PROPERTY(QString id READ id WRITE setId);

public:

    /* Constructor: */
    UIWizardFirstRunPageBasic(const QString &strMachineId, bool fBootHardDiskWasSet);

protected:

    /* Wrapper to access 'this' from base part: */
    UIWizardPage* thisImp() { return this; }

private slots:

    /* Open with file-open dialog: */
    void sltOpenMediumWithFileOpenDialog();

private:

    /* Translate stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;

    /* Validation stuff: */
    bool validatePage();

    /* Stuff for 'source' field: */
    QString source() const;

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif // __UIWizardFirstRunPageBasic_h__

