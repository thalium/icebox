/* $Id: QIWidgetValidator.cpp $ */
/** @file
 * VBox Qt GUI - VirtualBox Qt extensions: QIWidgetValidator class implementation.
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

/* GUI includes: */
# include "QIWidgetValidator.h"
# include "UISettingsPage.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */



QObjectValidator::QObjectValidator(QValidator *pValidator, QObject *pParent /* = 0 */)
    : QObject(pParent)
    , m_pValidator(pValidator)
    , m_state(QValidator::Invalid)
{
    /* Prepare: */
    prepare();
}

void QObjectValidator::sltValidate(QString strInput /* = QString() */)
{
    /* Make sure validator assigned: */
    AssertPtrReturnVoid(m_pValidator);

    /* Validate: */
    int iPosition = 0;
    const QValidator::State state = m_pValidator->validate(strInput, iPosition);

    /* If validity state changed: */
    if (m_state != state)
    {
        /* Update last validity state: */
        m_state = state;

        /* Notifies listener(s) about validity change: */
        emit sigValidityChange(m_state);
    }
}

void QObjectValidator::prepare()
{
    /* Make sure validator assigned: */
    AssertPtrReturnVoid(m_pValidator);

    /* Register validator as child: */
    m_pValidator->setParent(this);

    /* Validate: */
    sltValidate();
}


QObjectValidatorGroup::QObjectValidatorGroup(QObject *pParent)
    : QObject(pParent)
    , m_fResult(false)
{
}

void QObjectValidatorGroup::addObjectValidator(QObjectValidator *pObjectValidator)
{
    /* Make sure object-validator passed: */
    AssertPtrReturnVoid(pObjectValidator);

    /* Register object-validator as child: */
    pObjectValidator->setParent(this);

    /* Insert object-validator to internal map: */
    m_group.insert(pObjectValidator, toResult(pObjectValidator->state()));

    /* Attach object-validator to group: */
    connect(pObjectValidator, &QObjectValidator::sigValidityChange,
            this, &QObjectValidatorGroup::sltValidate);
}

void QObjectValidatorGroup::sltValidate(QValidator::State state)
{
    /* Determine sender object-validator: */
    QObjectValidator *pObjectValidatorSender = qobject_cast<QObjectValidator*>(sender());
    /* Make sure that is one of our senders: */
    AssertReturnVoid(pObjectValidatorSender && m_group.contains(pObjectValidatorSender));

    /* Update internal map: */
    m_group[pObjectValidatorSender] = toResult(state);

    /* Enumerate all the registered object-validators: */
    bool fResult = true;
    foreach (QObjectValidator *pObjectValidator, m_group.keys())
        if (!toResult(pObjectValidator->state()))
        {
            fResult = false;
            break;
        }

    /* If validity state changed: */
    if (m_fResult != fResult)
    {
        /* Update last validity state: */
        m_fResult = fResult;

        /* Notifies listener(s) about validity change: */
        emit sigValidityChange(m_fResult);
    }
}

/* static */
bool QObjectValidatorGroup::toResult(QValidator::State state)
{
    return state == QValidator::Acceptable;
}


UIPageValidator::UIPageValidator(QObject *pParent, UISettingsPage *pPage)
    : QObject(pParent)
    , m_pPage(pPage)
    , m_fIsValid(true)
{
}

QPixmap UIPageValidator::warningPixmap() const
{
    return m_pPage->warningPixmap();
}

QString UIPageValidator::internalName() const
{
    return m_pPage->internalName();
}

void UIPageValidator::setLastMessage(const QString &strLastMessage)
{
    /* Remember new message: */
    m_strLastMessage = strLastMessage;

    /* Should we show corresponding warning icon? */
    if (m_strLastMessage.isEmpty())
        emit sigHideWarningIcon();
    else
        emit sigShowWarningIcon();
}

void UIPageValidator::revalidate()
{
    /* Notify listener(s) about validity change: */
    emit sigValidityChanged(this);
}


QValidator::State QIULongValidator::validate (QString &aInput, int &aPos) const
{
    Q_UNUSED (aPos);

    QString stripped = aInput.trimmed();

    if (stripped.isEmpty() ||
        stripped.toUpper() == QString ("0x").toUpper())
        return Intermediate;

    bool ok;
    ulong entered = aInput.toULong (&ok, 0);

    if (!ok)
        return Invalid;

    if (entered >= mBottom && entered <= mTop)
        return Acceptable;

    return (entered > mTop ) ? Invalid : Intermediate;
}

