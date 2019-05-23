/* $Id: UIWizardImportAppPageBasic2.cpp $ */
/** @file
 * VBox Qt GUI - UIWizardImportAppPageBasic2 class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Global includes: */
# include <QVBoxLayout>
# include <QTextBrowser>
# include <QPushButton>
# include <QPointer>
# include <QLabel>

/* Local includes: */
# include "UIWizardImportAppPageBasic2.h"
# include "UIWizardImportApp.h"
# include "QIRichTextLabel.h"
# include "QIDialogButtonBox.h"

/* COM includes: */
# include "CAppliance.h"
# include "CCertificate.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/*********************************************************************************************************************************
*   Class UIWizardImportAppPage2 implementation.                                                                                 *
*********************************************************************************************************************************/

UIWizardImportAppPage2::UIWizardImportAppPage2()
{
}


/*********************************************************************************************************************************
*   Class UIWizardImportAppPageBasic2 implementation.                                                                            *
*********************************************************************************************************************************/

UIWizardImportAppPageBasic2::UIWizardImportAppPageBasic2(const QString &strFileName)
    : m_enmCertText(kCertText_Uninitialized)
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        m_pApplianceWidget = new UIApplianceImportEditorWidget(this);
        {
            m_pApplianceWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
            m_pApplianceWidget->setFile(strFileName);
        }
        m_pCertLabel = new QLabel("<cert label>", this);
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addWidget(m_pApplianceWidget);
        pMainLayout->addWidget(m_pCertLabel);
    }

    /* Register classes: */
    qRegisterMetaType<ImportAppliancePointer>();
    /* Register fields: */
    registerField("applianceWidget", this, "applianceWidget");
}

void UIWizardImportAppPageBasic2::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardImportApp::tr("Appliance settings"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardImportApp::tr("These are the virtual machines contained in the appliance "
                                            "and the suggested settings of the imported VirtualBox machines. "
                                            "You can change many of the properties shown by double-clicking "
                                            "on the items and disable others using the check boxes below."));
    switch (m_enmCertText)
    {
        case kCertText_Unsigned:
            m_pCertLabel->setText(UIWizardImportApp::tr("Appliance is not signed"));
            break;
        case kCertText_IssuedTrusted:
            m_pCertLabel->setText(UIWizardImportApp::tr("Appliance signed by %1 (trusted)").arg(m_strSignedBy));
            break;
        case kCertText_IssuedExpired:
            m_pCertLabel->setText(UIWizardImportApp::tr("Appliance signed by %1 (expired!)").arg(m_strSignedBy));
            break;
        case kCertText_IssuedUnverified:
            m_pCertLabel->setText(UIWizardImportApp::tr("Unverified signature by %1!").arg(m_strSignedBy));
            break;
        case kCertText_SelfSignedTrusted:
            m_pCertLabel->setText(UIWizardImportApp::tr("Self signed by %1 (trusted)").arg(m_strSignedBy));
            break;
        case kCertText_SelfSignedExpired:
            m_pCertLabel->setText(UIWizardImportApp::tr("Self signed by %1 (expired!)").arg(m_strSignedBy));
            break;
        case kCertText_SelfSignedUnverified:
            m_pCertLabel->setText(UIWizardImportApp::tr("Unverified self signed signature by %1!").arg(m_strSignedBy));
            break;
        default:
            AssertFailed();
            RT_FALL_THRU();
        case kCertText_Uninitialized:
            m_pCertLabel->setText("<uninitialized page>");
            break;
    }
}

void UIWizardImportAppPageBasic2::initializePage()
{
    /* Acquire appliance and certificate: */
    CAppliance *pAppliance = m_pApplianceWidget->appliance();
    /* Check if pAppliance is alive. If not just return here. This
       prevents crashes when an invalid ova file is supllied: */
    if (!pAppliance)
    {
        if (wizard())
            wizard()->reject();
        return;
    }
    CCertificate certificate = pAppliance->GetCertificate();
    if (certificate.isNull())
        m_enmCertText = kCertText_Unsigned;
    else
    {
        /* Pick a 'signed-by' name. */
        m_strSignedBy = certificate.GetFriendlyName();

        /* If trusted, just select the right message: */
        if (certificate.GetTrusted())
        {
            if (certificate.GetSelfSigned())
                m_enmCertText = !certificate.GetExpired() ? kCertText_SelfSignedTrusted : kCertText_SelfSignedExpired;
            else
                m_enmCertText = !certificate.GetExpired() ? kCertText_IssuedTrusted     : kCertText_IssuedExpired;
        }
        else
        {
            /* Not trusted!  Must ask the user whether to continue in this case: */
            m_enmCertText = certificate.GetSelfSigned() ? kCertText_SelfSignedUnverified : kCertText_IssuedUnverified;

            /* Translate page early: */
            retranslateUi();

            /* Instantiate the dialog: */
            QPointer<UIApplianceUnverifiedCertificateViewer> pDialog = new UIApplianceUnverifiedCertificateViewer(this, certificate);
            AssertPtrReturnVoid(pDialog.data());

            /* Show viewer in modal mode: */
            const int iResultCode = pDialog->exec();

            /* Leave if viewer destroyed prematurely: */
            if (!pDialog)
                return;
            /* Delete viewer finally: */
            delete pDialog;
            pDialog = 0;

            /* Dismiss the entire import-appliance wizard if user rejects certificate: */
            if (iResultCode == QDialog::Rejected)
                wizard()->reject();
        }
    }

    /* Translate page: */
    retranslateUi();
}

void UIWizardImportAppPageBasic2::cleanupPage()
{
    /* Rollback settings: */
    m_pApplianceWidget->restoreDefaults();
    /* Call to base-class: */
    UIWizardPage::cleanupPage();
}

bool UIWizardImportAppPageBasic2::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Lock finish button: */
    startProcessing();

    /* Try to import appliance: */
    if (fResult)
        fResult = qobject_cast<UIWizardImportApp*>(wizard())->importAppliance();

    /* Unlock finish button: */
    endProcessing();

    /* Return result: */
    return fResult;
}


/*********************************************************************************************************************************
*   Class UIApplianceUnverifiedCertificateViewer implementation.                                                                 *
*********************************************************************************************************************************/

UIApplianceUnverifiedCertificateViewer::UIApplianceUnverifiedCertificateViewer(QWidget *pParent, const CCertificate &certificate)
    : QIWithRetranslateUI<QIDialog>(pParent)
    , m_certificate(certificate)
    , m_pTextLabel(0)
    , m_pTextBrowser(0)
{
    /* Prepare: */
    prepare();
}

void UIApplianceUnverifiedCertificateViewer::prepare()
{
    /* Create layout: */
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    AssertPtrReturnVoid(pLayout);
    {
        /* Create text-label: */
        m_pTextLabel = new QLabel;
        AssertPtrReturnVoid(m_pTextLabel);
        {
            /* Configure text-label: */
            m_pTextLabel->setWordWrap(true);
            /* Add text-label into layout: */
            pLayout->addWidget(m_pTextLabel);
        }

        /* Create text-browser: */
        m_pTextBrowser = new QTextBrowser;
        AssertPtrReturnVoid(m_pTextBrowser);
        {
            /* Configure text-browser: */
            m_pTextBrowser->setMinimumSize(500, 300);
            /* Add text-browser into layout: */
            pLayout->addWidget(m_pTextBrowser);
        }

        /* Create button-box: */
        QIDialogButtonBox *pButtonBox = new QIDialogButtonBox;
        AssertPtrReturnVoid(pButtonBox);
        {
            /* Configure button-box: */
            pButtonBox->setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
            pButtonBox->button(QDialogButtonBox::Yes)->setShortcut(Qt::Key_Enter);
            //pButtonBox->button(QDialogButtonBox::No)->setShortcut(Qt::Key_Esc);
            connect(pButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
            connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
            /* Add button-box into layout: */
            pLayout->addWidget(pButtonBox);
        }
    }
    /* Translate UI: */
    retranslateUi();
}

void UIApplianceUnverifiedCertificateViewer::retranslateUi()
{
    /* Translate dialog title: */
    setWindowTitle(tr("Unverifiable Certificate! Continue?"));

    /* Translate text-label caption: */
    if (m_certificate.GetSelfSigned())
        m_pTextLabel->setText(tr("<b>The appliance is signed by an unverified self signed certificate issued by '%1'. "
                                 "We recommend to only proceed with the importing if you are sure you should trust this entity.</b>"
                                 ).arg(m_certificate.GetFriendlyName()));
    else
        m_pTextLabel->setText(tr("<b>The appliance is signed by an unverified certificate issued to '%1'. "
                                 "We recommend to only proceed with the importing if you are sure you should trust this entity.</b>"
                                 ).arg(m_certificate.GetFriendlyName()));

    /* Translate text-browser contents: */
    const QString strTemplateRow = tr("<tr><td>%1:</td><td>%2</td></tr>", "key: value");
    QString strTableContent;
    strTableContent += strTemplateRow.arg(tr("Issuer"),               QStringList(m_certificate.GetIssuerName().toList()).join(", "));
    strTableContent += strTemplateRow.arg(tr("Subject"),              QStringList(m_certificate.GetSubjectName().toList()).join(", "));
    strTableContent += strTemplateRow.arg(tr("Not Valid Before"),     m_certificate.GetValidityPeriodNotBefore());
    strTableContent += strTemplateRow.arg(tr("Not Valid After"),      m_certificate.GetValidityPeriodNotAfter());
    strTableContent += strTemplateRow.arg(tr("Serial Number"),        m_certificate.GetSerialNumber());
    strTableContent += strTemplateRow.arg(tr("Self-Signed"),          m_certificate.GetSelfSigned() ? tr("True") : tr("False"));
    strTableContent += strTemplateRow.arg(tr("Authority (CA)"),       m_certificate.GetCertificateAuthority() ? tr("True") : tr("False"));
//    strTableContent += strTemplateRow.arg(tr("Trusted"),              m_certificate.GetTrusted() ? tr("True") : tr("False"));
    strTableContent += strTemplateRow.arg(tr("Public Algorithm"),     tr("%1 (%2)", "value (clarification)").arg(m_certificate.GetPublicKeyAlgorithm()).arg(m_certificate.GetPublicKeyAlgorithmOID()));
    strTableContent += strTemplateRow.arg(tr("Signature Algorithm"),  tr("%1 (%2)", "value (clarification)").arg(m_certificate.GetSignatureAlgorithmName()).arg(m_certificate.GetSignatureAlgorithmOID()));
    strTableContent += strTemplateRow.arg(tr("X.509 Version Number"), QString::number(m_certificate.GetVersionNumber()));
    m_pTextBrowser->setText(QString("<table>%1</table>").arg(strTableContent));
}

