/* $Id: UIWizardExportAppPageBasic3.h $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageBasic3 class declaration.
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

#ifndef __UIWizardExportAppPageBasic3_h__
#define __UIWizardExportAppPageBasic3_h__

/* Global includes: */
#include <QVariant>

/* Local includes: */
#include "UIWizardPage.h"

/* Forward declarations: */
class QLabel;
class QLineEdit;
class UIEmptyFilePathSelector;
class QComboBox;
class QCheckBox;
class QIRichTextLabel;

/* 3rd page of the Export Appliance wizard (base part): */
class UIWizardExportAppPage3 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardExportAppPage3();

    /* Helpers: */
    void chooseDefaultSettings();
    virtual void refreshCurrentSettings();
    virtual void updateFormatComboToolTip();

    /* Stuff for 'format' field: */
    QString format() const;
    void setFormat(const QString &strFormat);
    /* Stuff for 'manifestSelected' field: */
    bool isManifestSelected() const;
    void setManifestSelected(bool fChecked);
    /* Stuff for 'username' field: */
    QString username() const;
    void setUserName(const QString &strUserName);
    /* Stuff for 'password' field: */
    QString password() const;
    void setPassword(const QString &strPassword);
    /* Stuff for 'hostname' field: */
    QString hostname() const;
    void setHostname(const QString &strHostname);
    /* Stuff for 'bucket' field: */
    QString bucket() const;
    void setBucket(const QString &strBucket);
    /* Stuff for 'path' field: */
    QString path() const;
    void setPath(const QString &strPath);

    /* Variables: */
    QString m_strDefaultApplianceName;

    /* Widgets: */
    QLabel *m_pUsernameLabel;
    QLineEdit *m_pUsernameEditor;
    QLabel *m_pPasswordLabel;
    QLineEdit *m_pPasswordEditor;
    QLabel *m_pHostnameLabel;
    QLineEdit *m_pHostnameEditor;
    QLabel *m_pBucketLabel;
    QLineEdit *m_pBucketEditor;
    QLabel *m_pFileSelectorLabel;
    UIEmptyFilePathSelector *m_pFileSelector;
    QLabel *m_pFormatComboBoxLabel;
    QComboBox *m_pFormatComboBox;
    QCheckBox *m_pManifestCheckbox;
};

/* 3rd page of the Export Appliance wizard (basic extension): */
class UIWizardExportAppPageBasic3 : public UIWizardPage, public UIWizardExportAppPage3
{
    Q_OBJECT;
    Q_PROPERTY(QString format READ format WRITE setFormat);
    Q_PROPERTY(bool manifestSelected READ isManifestSelected WRITE setManifestSelected);
    Q_PROPERTY(QString username READ username WRITE setUserName);
    Q_PROPERTY(QString password READ password WRITE setPassword);
    Q_PROPERTY(QString hostname READ hostname WRITE setHostname);
    Q_PROPERTY(QString bucket READ bucket WRITE setBucket);
    Q_PROPERTY(QString path READ path WRITE setPath);

public:

    /* Constructor: */
    UIWizardExportAppPageBasic3();

protected:

    /* Wrapper to access 'wizard-field' from base part: */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

private slots:

    /* Format combo change handler: */
    void sltHandleFormatComboChange();

private:

    /* Translate stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;

    /* Helpers: */
    void refreshCurrentSettings();

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif /* __UIWizardExportAppPageBasic3_h__ */

