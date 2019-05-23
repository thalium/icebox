/* $Id: UIWizardCloneVDPageBasic4.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPageBasic4 class declaration.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIWizardCloneVDPageBasic4_h__
#define __UIWizardCloneVDPageBasic4_h__

/* Qt includes: */
#include <QVariant>

/* GUI includes: */
#include "UIWizardPage.h"

/* Forward declarations: */
class CMediumFormat;
class QLineEdit;
class QIToolButton;
class QIRichTextLabel;

/* 4th page of the Clone Virtual Hard Drive wizard (base part): */
class UIWizardCloneVDPage4 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardCloneVDPage4();

    /* Handlers: */
    void onSelectLocationButtonClicked();

    /* Location-editors stuff: */
    static QString toFileName(const QString &strName, const QString &strExtension);
    static QString absoluteFilePath(const QString &strFileName, const QString &strDefaultPath);
    static QString defaultExtension(const CMediumFormat &mediumFormatRef);

    /* Stuff for 'mediumPath' field: */
    QString mediumPath() const;

    /* Stuff for 'mediumSize' field: */
    qulonglong mediumSize() const;

    /* Variables: */
    QString m_strDefaultPath;
    QString m_strDefaultExtension;

    /* Widgets: */
    QLineEdit *m_pDestinationDiskEditor;
    QIToolButton *m_pDestinationDiskOpenButton;
};

/* 4th page of the Clone Virtual Hard Drive wizard (basic extension): */
class UIWizardCloneVDPageBasic4 : public UIWizardPage, public UIWizardCloneVDPage4
{
    Q_OBJECT;
    Q_PROPERTY(QString mediumPath READ mediumPath);
    Q_PROPERTY(qulonglong mediumSize READ mediumSize);

public:

    /* Constructor: */
    UIWizardCloneVDPageBasic4();

protected:

    /* Wrapper to access 'this' from base part: */
    UIWizardPage* thisImp() { return this; }
    /* Wrapper to access 'wizard-field' from base part: */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

private slots:

    /* Location editor stuff: */
    void sltSelectLocationButtonClicked();

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;
    bool validatePage();

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif // __UIWizardCloneVDPageBasic4_h__

