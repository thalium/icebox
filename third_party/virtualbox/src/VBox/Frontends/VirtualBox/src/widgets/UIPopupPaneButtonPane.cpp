/* $Id: UIPopupPaneButtonPane.cpp $ */
/** @file
 * VBox Qt GUI - UIPopupPaneButtonPane class implementation.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
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
# include <QApplication>
# include <QHBoxLayout>
# include <QVBoxLayout>
# include <QKeyEvent>

/* GUI includes: */
# include "UIPopupPaneButtonPane.h"
# include "UIIconPool.h"
# include "QIToolButton.h"
# include "QIMessageBox.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIPopupPaneButtonPane::UIPopupPaneButtonPane(QWidget *pParent /* = 0*/)
    : QWidget(pParent)
    , m_iDefaultButton(0)
    , m_iEscapeButton(0)
{
    /* Prepare: */
    prepare();
}

void UIPopupPaneButtonPane::setButtons(const QMap<int, QString> &buttonDescriptions)
{
    /* Make sure something changed: */
    if (m_buttonDescriptions == buttonDescriptions)
        return;

    /* Assign new button-descriptions: */
    m_buttonDescriptions = buttonDescriptions;
    /* Recreate buttons: */
    cleanupButtons();
    prepareButtons();
}

void UIPopupPaneButtonPane::sltButtonClicked()
{
    /* Make sure the slot is called by the button: */
    QIToolButton *pButton = qobject_cast<QIToolButton*>(sender());
    if (!pButton)
        return;

    /* Make sure we still have that button: */
    int iButtonID = m_buttons.key(pButton, 0);
    if (!iButtonID)
        return;

    /* Notify listeners button was clicked: */
    emit sigButtonClicked(iButtonID);
}

void UIPopupPaneButtonPane::prepare()
{
    /* Prepare layouts: */
    prepareLayouts();
}

void UIPopupPaneButtonPane::prepareLayouts()
{
    /* Create layouts: */
    m_pButtonLayout = new QHBoxLayout;
    m_pButtonLayout->setContentsMargins(0, 0, 0, 0);
    m_pButtonLayout->setSpacing(0);
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    pMainLayout->setContentsMargins(0, 0, 0, 0);
    pMainLayout->setSpacing(0);
    pMainLayout->addLayout(m_pButtonLayout);
    pMainLayout->addStretch();
}

void UIPopupPaneButtonPane::prepareButtons()
{
    /* Add all the buttons: */
    const QList<int> &buttonsIDs = m_buttonDescriptions.keys();
    foreach (int iButtonID, buttonsIDs)
        if (QIToolButton *pButton = addButton(iButtonID, m_buttonDescriptions[iButtonID]))
        {
            m_pButtonLayout->addWidget(pButton);
            m_buttons[iButtonID] = pButton;
            connect(pButton, SIGNAL(clicked(bool)), this, SLOT(sltButtonClicked()));
            if (pButton->property("default").toBool())
                m_iDefaultButton = iButtonID;
            if (pButton->property("escape").toBool())
                m_iEscapeButton = iButtonID;
        }
}

void UIPopupPaneButtonPane::cleanupButtons()
{
    /* Remove all the buttons: */
    const QList<int> &buttonsIDs = m_buttons.keys();
    foreach (int iButtonID, buttonsIDs)
    {
        delete m_buttons[iButtonID];
        m_buttons.remove(iButtonID);
    }
}

void UIPopupPaneButtonPane::keyPressEvent(QKeyEvent *pEvent)
{
    /* Depending on pressed key: */
    switch (pEvent->key())
    {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        {
            if (m_iDefaultButton)
            {
                pEvent->accept();
                emit sigButtonClicked(m_iDefaultButton);
                return;
            }
            break;
        }
        case Qt::Key_Escape:
        {
            if (m_iEscapeButton)
            {
                pEvent->accept();
                emit sigButtonClicked(m_iEscapeButton);
                return;
            }
            break;
        }
        default:
            break;
    }
    /* Call to base-class: */
    QWidget::keyPressEvent(pEvent);
}

/* static */
QIToolButton* UIPopupPaneButtonPane::addButton(int iButtonID, const QString &strToolTip)
{
    /* Create button: */
    QIToolButton *pButton = new QIToolButton;
    pButton->removeBorder();
    pButton->setToolTip(strToolTip.isEmpty() ? defaultToolTip(iButtonID) : strToolTip);
    pButton->setIcon(defaultIcon(iButtonID));

    /* Sign the 'default' button: */
    if (iButtonID & AlertButtonOption_Default)
        pButton->setProperty("default", true);
    /* Sign the 'escape' button: */
    if (iButtonID & AlertButtonOption_Escape)
        pButton->setProperty("escape", true);

    /* Return button: */
    return pButton;
}

/* static */
QString UIPopupPaneButtonPane::defaultToolTip(int iButtonID)
{
    QString strToolTip;
    switch (iButtonID & AlertButtonMask)
    {
        case AlertButton_Ok: strToolTip = QIMessageBox::tr("OK"); break;
        case AlertButton_Cancel:
        {
            switch (iButtonID & AlertOptionMask)
            {
                case AlertOption_AutoConfirmed: strToolTip = QApplication::translate("UIMessageCenter", "Do not show this message again"); break;
                default: strToolTip = QIMessageBox::tr("Cancel"); break;
            }
            break;
        }
        case AlertButton_Choice1: strToolTip = QIMessageBox::tr("Yes"); break;
        case AlertButton_Choice2: strToolTip = QIMessageBox::tr("No"); break;
        default: strToolTip = QString(); break;
    }
    return strToolTip;
}

/* static */
QIcon UIPopupPaneButtonPane::defaultIcon(int iButtonID)
{
    QIcon icon;
    switch (iButtonID & AlertButtonMask)
    {
        case AlertButton_Ok: icon = UIIconPool::iconSet(":/ok_16px.png"); break;
        case AlertButton_Cancel:
        {
            switch (iButtonID & AlertOptionMask)
            {
                case AlertOption_AutoConfirmed: icon = UIIconPool::iconSet(":/close_popup_16px.png"); break;
                default: icon = UIIconPool::iconSet(":/cancel_16px.png"); break;
            }
            break;
        }
        case AlertButton_Choice1: break;
        case AlertButton_Choice2: break;
        default: break;
    }
    return icon;
}

