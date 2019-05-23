/* $Id: VBoxAboutDlg.cpp $ */
/** @file
 * VBox Qt GUI - VBoxAboutDlg class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QDir>
# include <QDialogButtonBox>
# include <QEvent>
# include <QLabel>
# include <QPainter>
# include <QPushButton>

/* GUI includes: */
# include "UIConverter.h"
# include "UIExtraDataManager.h"
# include "UIIconPool.h"
# include "VBoxAboutDlg.h"
# include "VBoxGlobal.h"

/* Other VBox includes: */
# include <iprt/path.h>
# include <VBox/version.h> /* VBOX_VENDOR */

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

VBoxAboutDlg::VBoxAboutDlg(QWidget *pParent, const QString &strVersion)
#ifdef VBOX_WS_MAC
    // No need for About dialog parent on macOS.
    // First of all, non of other native apps (Safari, App Store, iTunes) centers About dialog according the app itself, they do
    // it according to screen instead, we should do it as well.  Besides that since About dialog is not modal, it will be in
    // conflict with modal dialogs if there will be a parent passed, because the dialog will not have own event-loop in that case.
    : QIWithRetranslateUI2<QIDialog>(0)
    , m_pPseudoParent(pParent)
#else
    // On other hosts we will keep the current behavior for now.
    // First of all it's quite difficult to find native (Metro UI) Windows app which have About dialog at all.  But non-native
    // cross-platform apps (Qt Creator, VLC) centers About dialog according the app exactly.
    : QIWithRetranslateUI2<QIDialog>(pParent)
    , m_pPseudoParent(0)
#endif
    , m_strVersion(strVersion)
    , m_pLabel(0)
{
    /* Prepare: */
    prepare();
}

bool VBoxAboutDlg::event(QEvent *pEvent)
{
    /* Set fixed-size for dialog: */
    if (pEvent->type() == QEvent::Polish)
        setFixedSize(m_size);
    /* Call to base-class: */
    return QIDialog::event(pEvent);
}

void VBoxAboutDlg::paintEvent(QPaintEvent * /* pEvent */)
{
    QPainter painter(this);
    /* Draw About-VirtualBox background image: */
    painter.drawPixmap(0, 0, m_pixmap);
}

void VBoxAboutDlg::retranslateUi()
{
    setWindowTitle(tr("VirtualBox - About"));
    const QString strAboutText = tr("VirtualBox Graphical User Interface");
#ifdef VBOX_BLEEDING_EDGE
    const QString strVersionText = "EXPERIMENTAL build %1 - " + QString(VBOX_BLEEDING_EDGE);
#else
    const QString strVersionText = tr("Version %1");
#endif
#ifdef VBOX_OSE
    m_strAboutText = strAboutText + " " + strVersionText.arg(m_strVersion) + "\n"
                   + QString("%1 2004-" VBOX_C_YEAR " " VBOX_VENDOR).arg(QChar(0xa9));
#else
    m_strAboutText = strAboutText + "\n" + strVersionText.arg(m_strVersion);
#endif
    m_strAboutText = m_strAboutText + QString(" (Qt%1)").arg(qVersion());
    m_strAboutText = m_strAboutText + "\n" + QString("Copyright %1 %2 %3 and/or its affiliates. All rights reserved.")
                                                     .arg(QChar(0xa9)).arg(VBOX_C_YEAR).arg(VBOX_VENDOR);
    AssertPtrReturnVoid(m_pLabel);
    m_pLabel->setText(m_strAboutText);
}

void VBoxAboutDlg::prepare()
{
    /* Delete dialog on close: */
    setAttribute(Qt::WA_DeleteOnClose);

    /* Make sure the dialog is deleted on pseudo-parent destruction: */
    if (m_pPseudoParent)
        connect(m_pPseudoParent, &QObject::destroyed, this, &VBoxAboutDlg::close);

    /* Choose default image: */
    QString strPath(":/about.png");

    /* Branding: Use a custom about splash picture if set: */
    const QString strSplash = vboxGlobal().brandingGetKey("UI/AboutSplash");
    if (vboxGlobal().brandingIsActive() && !strSplash.isEmpty())
    {
        char szExecPath[1024];
        RTPathExecDir(szExecPath, 1024);
        QString strTmpPath = QString("%1/%2").arg(szExecPath).arg(strSplash);
        if (QFile::exists(strTmpPath))
            strPath = strTmpPath;
    }

    /* Load image: */
    const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize);
    const double dRatio = (double)iIconMetric / 32;
    const QIcon icon = UIIconPool::iconSet(strPath);
    m_size = icon.availableSizes().value(0, QSize(640, 480));
    m_size *= dRatio;
    m_pixmap = icon.pixmap(m_size);

    // WORKAROUND:
    // Since we don't have x3 and x4 HiDPI icons yet,
    // and we hadn't enabled automatic up-scaling for now,
    // we have to make sure m_pixmap is upscaled to required size.
    const QSize actualSize = m_pixmap.size() / m_pixmap.devicePixelRatio();
    if (   actualSize.width() < m_size.width()
        || actualSize.height() < m_size.height())
        m_pixmap = m_pixmap.scaled(m_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    /* Prepare main-layout: */
    prepareMainLayout();

    /* Translate: */
    retranslateUi();
}

void VBoxAboutDlg::prepareMainLayout()
{
    /* Create main-layout: */
    m_pMainLayout = new QVBoxLayout(this);
    AssertPtrReturnVoid(m_pMainLayout);
    {
        /* Prepare label: */
        prepareLabel();

        /* Prepare close-button: */
        prepareCloseButton();
    }
}

void VBoxAboutDlg::prepareLabel()
{
    /* Create label for version text: */
    m_pLabel = new QLabel;
    AssertPtrReturnVoid(m_pLabel);
    {
        /* Prepare label for version text: */
        QPalette palette;
        /* Branding: Set a different text color (because splash also could be white),
         * otherwise use white as default color: */
        const QString strColor = vboxGlobal().brandingGetKey("UI/AboutTextColor");
        if (!strColor.isEmpty())
            palette.setColor(QPalette::WindowText, QColor(strColor).name());
        else
            palette.setColor(QPalette::WindowText, Qt::black);
        m_pLabel->setPalette(palette);
        m_pLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_pLabel->setFont(font());

        /* Add label to the main-layout: */
        m_pMainLayout->addWidget(m_pLabel);
        m_pMainLayout->setAlignment(m_pLabel, Qt::AlignRight | Qt::AlignBottom);
    }
}

void VBoxAboutDlg::prepareCloseButton()
{
    /* Create button-box: */
    QDialogButtonBox *pButtonBox = new QDialogButtonBox;
    AssertPtrReturnVoid(pButtonBox);
    {
        /* Create close-button: */
        QPushButton *pCloseButton = pButtonBox->addButton(QDialogButtonBox::Close);
        AssertPtrReturnVoid(pCloseButton);
        /* Prepare close-button: */
        connect(pButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

        /* Add button-box to the main-layout: */
        m_pMainLayout->addWidget(pButtonBox);
    }
}

