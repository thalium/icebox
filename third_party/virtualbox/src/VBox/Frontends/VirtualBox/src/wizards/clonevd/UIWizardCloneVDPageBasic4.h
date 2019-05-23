/* $Id: UIWizardCloneVDPageBasic4.h $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVDPageBasic4 class declaration.
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

#ifndef ___UIWizardCloneVDPageBasic4_h___
#define ___UIWizardCloneVDPageBasic4_h___

/* Qt includes: */
#include <QVariant>

/* GUI includes: */
#include "UIWizardPage.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class CMediumFormat;
class QLineEdit;
class QIToolButton;
class QIRichTextLabel;


/** 4th page of the Clone Virtual Disk Image wizard (base part): */
class UIWizardCloneVDPage4 : public UIWizardPageBase
{
protected:

    /** Constructs page basis. */
    UIWizardCloneVDPage4();

    /** Handles command to open target disk. */
    void onSelectLocationButtonClicked();

    /** Helps to compose full file name on the basis of incoming @a strName and @a strExtension. */
    static QString toFileName(const QString &strName, const QString &strExtension);
    /** Converts the @a strFileName to absolute one if necessary using @a strDefaultPath as advice. */
    static QString absoluteFilePath(const QString &strFileName, const QString &strDefaultPath);
    /** Acquires the list of @a aAllowedExtensions and @a strDefaultExtension
      * on the basis of incoming @a comMediumFormat and @a enmDeviceType. */
    static void acquireExtensions(const CMediumFormat &comMediumFormat, KDeviceType enmDeviceType,
                                  QStringList &aAllowedExtensions, QString &strDefaultExtension);

    /** Returns 'mediumPath' field value. */
    QString mediumPath() const;

    /** Returns 'mediumSize' field value. */
    qulonglong mediumSize() const;

    /** Holds the default path. */
    QString      m_strDefaultPath;
    /** Holds the default extension. */
    QString      m_strDefaultExtension;
    /** Holds the allowed extensions. */
    QStringList  m_aAllowedExtensions;

    /** Holds the target disk path editor instance. */
    QLineEdit    *m_pDestinationDiskEditor;
    /** Holds the open-target-disk button instance. */
    QIToolButton *m_pDestinationDiskOpenButton;
};


/** 4th page of the Clone Virtual Disk Image wizard (basic extension): */
class UIWizardCloneVDPageBasic4 : public UIWizardPage, public UIWizardCloneVDPage4
{
    Q_OBJECT;
    Q_PROPERTY(QString mediumPath READ mediumPath);
    Q_PROPERTY(qulonglong mediumSize READ mediumSize);

public:

    /** Constructs basic page. */
    UIWizardCloneVDPageBasic4();

protected:

    /** Allows to access 'wizard()' from base part. */
    UIWizard* wizardImp() { return wizard(); }
    /** Allows to access 'this' from base part. */
    UIWizardPage* thisImp() { return this; }
    /** Allows to access 'field()' from base part. */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

private slots:

    /** Handles command to open target disk. */
    void sltSelectLocationButtonClicked();

private:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Prepares the page. */
    virtual void initializePage() /* override */;

    /** Returns whether the page is complete. */
    virtual bool isComplete() const /* override */;

    /** Returns whether the page is valid. */
    virtual bool validatePage() /* override */;

    /** Holds the description label instance. */
    QIRichTextLabel *m_pLabel;
};

#endif /* !___UIWizardCloneVDPageBasic4_h___ */

