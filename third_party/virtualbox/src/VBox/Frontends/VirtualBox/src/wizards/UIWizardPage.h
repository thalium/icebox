/* $Id: UIWizardPage.h $ */
/** @file
 * VBox Qt GUI - UIWizardPage class declaration.
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

#ifndef ___UIWizardPage_h___
#define ___UIWizardPage_h___

/* Global includes: */
#include <QVariant>
#include <QWizardPage>

/* Local includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class UIWizard;
class UIWizardPage;

/* One of interfaces for wizard page,
 * providing API for basic/expert pages. */
class UIWizardPageBase
{
public:
    virtual ~UIWizardPageBase() { /* Makes MSC happy. */ }

protected:

    /* Helpers: */
    virtual UIWizard* wizardImp();
    virtual UIWizardPage* thisImp();
    virtual QVariant fieldImp(const QString &strFieldName) const;
};

/* One of interfaces for wizard page,
 * QWizardPage class reimplementation with extended funtionality. */
class UIWizardPage : public QIWithRetranslateUI<QWizardPage>
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIWizardPage();

    /* Translation stuff: */
    void retranslate() { retranslateUi(); }

    /* Prepare stuff: */
    void markReady();

    /* Title stuff: */
    void setTitle(const QString &strTitle);

protected:

    /* Helpers: */
    UIWizard* wizard() const;
    void startProcessing();
    void endProcessing();

    /* Variables: */
    bool m_fReady;
    QString m_strTitle;
};

#endif

