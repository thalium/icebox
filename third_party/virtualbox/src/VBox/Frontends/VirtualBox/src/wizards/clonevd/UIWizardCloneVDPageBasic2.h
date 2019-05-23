/* $Id: UIWizardCloneVDPageBasic2.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPageBasic2 class declaration.
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

#ifndef __UIWizardCloneVDPageBasic2_h__
#define __UIWizardCloneVDPageBasic2_h__

/* GUI includes: */
#include "UIWizardPage.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMediumFormat.h"

/* Forward declarations: */
class QVBoxLayout;
class QButtonGroup;
class QRadioButton;
class QIRichTextLabel;

/* 2nd page of the Clone Virtual Hard Drive wizard (base part): */
class UIWizardCloneVDPage2 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardCloneVDPage2();

    /* Helping stuff: */
    void addFormatButton(QWidget *pParent, QVBoxLayout *pFormatsLayout, CMediumFormat medFormat, bool fPreferred = false);

    /* Stuff for 'mediumFormat' field: */
    CMediumFormat mediumFormat() const;
    void setMediumFormat(const CMediumFormat &mediumFormat);

    /* Variables: */
    QButtonGroup *m_pFormatButtonGroup;
    QList<CMediumFormat> m_formats;
    QStringList m_formatNames;
};

/* 2nd page of the Clone Virtual Hard Drive wizard (basic extension): */
class UIWizardCloneVDPageBasic2 : public UIWizardPage, public UIWizardCloneVDPage2
{
    Q_OBJECT;
    Q_PROPERTY(CMediumFormat mediumFormat READ mediumFormat WRITE setMediumFormat);

public:

    /* Constructor: */
    UIWizardCloneVDPageBasic2();

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;

    /* Navigation stuff: */
    int nextId() const;

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif // __UIWizardCloneVDPageBasic2_h__

