/* $Id: UIWizardNewVMPageBasic3.h $ */
/** @file
 * VBox Qt GUI - UIWizardNewVMPageBasic3 class declaration.
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

#ifndef __UIWizardNewVMPageBasic3_h__
#define __UIWizardNewVMPageBasic3_h__

/* Qt includes: */
#include <QVariant>

/* GUI includes: */
#include "UIWizardPage.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMedium.h"

/* Forward declarations: */
class QRadioButton;
class VBoxMediaComboBox;
class QIToolButton;
class QIRichTextLabel;

/* 3rd page of the New Virtual Machine wizard (base part): */
class UIWizardNewVMPage3 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardNewVMPage3();

    /* Handlers: */
    void updateVirtualDiskSource();
    void getWithFileOpenDialog();
    bool getWithNewVirtualDiskWizard();

    /* Stuff for 'virtualDisk' field: */
    CMedium virtualDisk() const { return m_virtualDisk; }
    void setVirtualDisk(const CMedium &virtualDisk) { m_virtualDisk = virtualDisk; }

    /* Stuff for 'virtualDiskId' field: */
    QString virtualDiskId() const { return m_strVirtualDiskId; }
    void setVirtualDiskId(const QString &strVirtualDiskId) { m_strVirtualDiskId = strVirtualDiskId; }

    /* Stuff for 'virtualDiskName' field: */
    QString virtualDiskName() const { return m_strVirtualDiskName; }
    void setVirtualDiskName(const QString &strVirtualDiskName) { m_strVirtualDiskName = strVirtualDiskName; }

    /* Stuff for 'virtualDiskLocation' field: */
    QString virtualDiskLocation() const { return m_strVirtualDiskLocation; }
    void setVirtualDiskLocation(const QString &strVirtualDiskLocation) { m_strVirtualDiskLocation = strVirtualDiskLocation; }

    /* Helpers: */
    void ensureNewVirtualDiskDeleted();

    /* Input: */
    bool m_fRecommendedNoDisk;

    /* Variables: */
    CMedium m_virtualDisk;
    QString m_strVirtualDiskId;
    QString m_strVirtualDiskName;
    QString m_strVirtualDiskLocation;

    /* Widgets: */
    QRadioButton *m_pDiskSkip;
    QRadioButton *m_pDiskCreate;
    QRadioButton *m_pDiskPresent;
    VBoxMediaComboBox *m_pDiskSelector;
    QIToolButton *m_pVMMButton;
};

/* 3rd page of the New Virtual Machine wizard (basic extension): */
class UIWizardNewVMPageBasic3 : public UIWizardPage, public UIWizardNewVMPage3
{
    Q_OBJECT;
    Q_PROPERTY(CMedium virtualDisk READ virtualDisk WRITE setVirtualDisk);
    Q_PROPERTY(QString virtualDiskId READ virtualDiskId WRITE setVirtualDiskId);
    Q_PROPERTY(QString virtualDiskName READ virtualDiskName WRITE setVirtualDiskName);
    Q_PROPERTY(QString virtualDiskLocation READ virtualDiskLocation WRITE setVirtualDiskLocation);

public:

    /* Constructor: */
    UIWizardNewVMPageBasic3();

protected:

    /* Wrapper to access 'wizard' from base part: */
    UIWizard* wizardImp() { return wizard(); }
    /* Wrapper to access 'this' from base part: */
    UIWizardPage* thisImp() { return this; }
    /* Wrapper to access 'wizard-field' from base part: */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

private slots:

    /* Handlers: */
    void sltVirtualDiskSourceChanged();
    void sltGetWithFileOpenDialog();

private:

    /* Translate stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();
    void cleanupPage();

    /* Validation stuff: */
    bool isComplete() const;
    bool validatePage();

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif // __UIWizardNewVMPageBasic3_h__

