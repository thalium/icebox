/* $Id: UIWizardCloneVDPageExpert.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPageExpert class declaration.
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

#ifndef __UIWizardCloneVDPageExpert_h__
#define __UIWizardCloneVDPageExpert_h__

/* Local includes: */
#include "UIWizardCloneVDPageBasic1.h"
#include "UIWizardCloneVDPageBasic2.h"
#include "UIWizardCloneVDPageBasic3.h"
#include "UIWizardCloneVDPageBasic4.h"

/* Forward declarations: */
class QGroupBox;

/* Expert page of the Clone Virtual Hard Drive wizard: */
class UIWizardCloneVDPageExpert : public UIWizardPage,
                                  public UIWizardCloneVDPage1,
                                  public UIWizardCloneVDPage2,
                                  public UIWizardCloneVDPage3,
                                  public UIWizardCloneVDPage4
{
    Q_OBJECT;
    Q_PROPERTY(CMedium sourceVirtualDisk READ sourceVirtualDisk WRITE setSourceVirtualDisk);
    Q_PROPERTY(CMediumFormat mediumFormat READ mediumFormat WRITE setMediumFormat);
    Q_PROPERTY(qulonglong mediumVariant READ mediumVariant WRITE setMediumVariant);
    Q_PROPERTY(QString mediumPath READ mediumPath);
    Q_PROPERTY(qulonglong mediumSize READ mediumSize);

public:

    /* Constructor: */
    UIWizardCloneVDPageExpert(const CMedium &sourceVirtualDisk);

protected:

    /* Wrapper to access 'this' from base part: */
    UIWizardPage* thisImp() { return this; }
    /* Wrapper to access 'wizard-field' from base part: */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

private slots:

    /* Source virtual-disk stuff: */
    void sltHandleSourceDiskChange();
    void sltHandleOpenSourceDiskClick();

    /* Medium format stuff: */
    void sltMediumFormatChanged();

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
    QGroupBox *m_pSourceDiskCnt;
    QGroupBox *m_pFormatCnt;
    QGroupBox *m_pVariantCnt;
    QGroupBox *m_pDestinationCnt;
};

#endif // __UIWizardCloneVDPageExpert_h__

