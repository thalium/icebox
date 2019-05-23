/* $Id: UIWizardNewVMPageBasic1.h $ */
/** @file
 * VBox Qt GUI - UIWizardNewVMPageBasic1 class declaration.
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

#ifndef __UIWizardNewVMPageBasic1_h__
#define __UIWizardNewVMPageBasic1_h__

/* Local includes: */
#include "UIWizardPage.h"

/* Forward declarations: */
class UINameAndSystemEditor;
class QIRichTextLabel;

/* 1st page of the New Virtual Machine wizard (base part): */
class UIWizardNewVMPage1 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardNewVMPage1(const QString &strGroup);

    /* Handlers: */
    void onNameChanged(QString strNewName);
    void onOsTypeChanged();

    /* Helping stuff: */
    bool machineFolderCreated();
    bool createMachineFolder();
    bool cleanupMachineFolder();

    /** Returns the machine folder value. */
    QString machineFolder() const { return m_strMachineFolder; }
    /** Defines the @a strMachineFolder value. */
    void setMachineFolder(const QString &strMachineFolder) { m_strMachineFolder = strMachineFolder; }

    /** Returns the machine base-name value. */
    QString machineBaseName() const { return m_strMachineBaseName; }
    /** Defines the @a strMachineBaseName value. */
    void setMachineBaseName(const QString &strMachineBaseName) { m_strMachineBaseName = strMachineBaseName; }

    /** Holds the machine folder value. */
    QString m_strMachineFolder;
    /** Holds the machine base-name value. */
    QString m_strMachineBaseName;

    /* Widgets: */
    UINameAndSystemEditor *m_pNameAndSystemEditor;

    /* Variables: */
    QString m_strGroup;
    bool m_fSupportsHWVirtEx;
    bool m_fSupportsLongMode;
};

/* 1st page of the New Virtual Machine wizard (basic extension): */
class UIWizardNewVMPageBasic1 : public UIWizardPage, public UIWizardNewVMPage1
{
    Q_OBJECT;
    Q_PROPERTY(QString machineFolder READ machineFolder WRITE setMachineFolder);
    Q_PROPERTY(QString machineBaseName READ machineBaseName WRITE setMachineBaseName);

public:

    /* Constructor: */
    UIWizardNewVMPageBasic1(const QString &strGroup);

protected:

    /* Wrapper to access 'this' from base part: */
    UIWizardPage* thisImp() { return this; }

private slots:

    /* Handlers: */
    void sltNameChanged(const QString &strNewText);
    void sltOsTypeChanged();

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();
    void cleanupPage();

    /* Validation stuff: */
    bool validatePage();

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
};

#endif // __UIWizardNewVMPageBasic1_h__

