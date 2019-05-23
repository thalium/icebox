/* $Id: UIWizardImportAppPageBasic2.h $ */
/** @file
 * VBox Qt GUI - UIWizardImportAppPageBasic2 class declaration.
 */

/*
 * Copyright (C) 2009-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIWizardImportAppPageBasic2_h___
#define ___UIWizardImportAppPageBasic2_h___

/* GUI includes: */
#include "QIDialog.h"
#include "QIWithRetranslateUI.h"
#include "UIWizardPage.h"
#include "UIWizardImportAppDefs.h"

/* Forward declarations: */
class QLabel;
class QTextBrowser;
class QDialogButtonBox;
class QIRichTextLabel;
class CCertificate;


/** 2nd page of the Import Appliance wizard (base part): */
class UIWizardImportAppPage2 : public UIWizardPageBase
{
protected:

    /* Constructor: */
    UIWizardImportAppPage2();

    /* Stuff for 'applianceWidget' field: */
    ImportAppliancePointer applianceWidget() const { return m_pApplianceWidget; }

    /* Widgets: */
    ImportAppliancePointer m_pApplianceWidget;
};

/** 2nd page of the Import Appliance wizard (basic extension): */
class UIWizardImportAppPageBasic2 : public UIWizardPage, public UIWizardImportAppPage2
{
    Q_OBJECT;
    Q_PROPERTY(ImportAppliancePointer applianceWidget READ applianceWidget);

public:

    /* Constructor: */
    UIWizardImportAppPageBasic2(const QString &strFileName);

private:

    /* Translate stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();
    void cleanupPage();

    /* Validation stuff: */
    bool validatePage();

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
    QLabel *m_pCertLabel;       /**< Signature/certificate info label. */
    enum {
        kCertText_Uninitialized = 0, kCertText_Unsigned,
        kCertText_IssuedTrusted,     kCertText_IssuedExpired,     kCertText_IssuedUnverified,
        kCertText_SelfSignedTrusted, kCertText_SelfSignedExpired, kCertText_SelfSignedUnverified
    } m_enmCertText;
    QString m_strSignedBy;
};

/** QIDialog extension
  * asking for consent to continue with unverifiable certificate. */
class UIApplianceUnverifiedCertificateViewer : public QIWithRetranslateUI<QIDialog>
{
    Q_OBJECT;

public:

    /** Constructs appliance @a certificate viewer for passed @a pParent. */
    UIApplianceUnverifiedCertificateViewer(QWidget *pParent, const CCertificate &certificate);

protected:

    /** Prepares all. */
    void prepare();

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private:

    /** Holds the certificate reference. */
    const CCertificate &m_certificate;

    /** Holds the text-label instance. */
    QLabel *m_pTextLabel;
    /** Holds the text-browser instance. */
    QTextBrowser *m_pTextBrowser;
};

#endif /* !___UIWizardImportAppPageBasic2_h___ */

