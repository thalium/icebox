/* $Id: QIWidgetValidator.h $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIWidgetValidator class declaration.
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

#ifndef __QIWidgetValidator_h__
#define __QIWidgetValidator_h__

/* Qt includes: */
#include <QValidator>
#include <QPixmap>
#include <QMap>

/* External includes: */
#ifdef VBOX_WS_X11
#include <limits.h>
#endif /* VBOX_WS_X11 */

/* Forward declarations: */
class UISettingsPage;

/** QObject reimplementation,
  * providing passed QObject with validation routine. */
class QObjectValidator : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies listener(s) about validity change. */
    void sigValidityChange(QValidator::State state);

public:

    /** Constructor.
      * @param pParent    is passed on to the QObject constructor,
      * @param pValidator is passed on to the OObject children
      *                   and used to perform validation itself. */
    QObjectValidator(QValidator *pValidator, QObject *pParent = 0);

    /** Returns last validation state. */
    QValidator::State state() const { return m_state; }

public slots:

    /** Performs validation: */
    void sltValidate(QString strInput = QString());

private:

    /** Prepare routine. */
    void prepare();

    /** Holds validator. */
    QValidator *m_pValidator;
    /** Holds validation state. */
    QValidator::State m_state;
};

/** QObject reimplementation,
  * which can group various QObjectValidator instances to operate on. */
class QObjectValidatorGroup : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies listener(s) about validity change. */
    void sigValidityChange(bool fValid);

public:

    /** Constructor.
      * @param pParent is passed on to the QObject constructor. */
    QObjectValidatorGroup(QObject *pParent);

    /** Adds object-validator.
      * @note The ownership of @a pObjectValidator is transferred to the group,
      *       and it's the group's responsibility to delete it. */
    void addObjectValidator(QObjectValidator *pObjectValidator);

    /** Returns last validation result. */
    bool result() const { return m_fResult; }

private slots:

    /** Performs validation: */
    void sltValidate(QValidator::State state);

private:

    /** Converts QValidator::State to bool result. */
    static bool toResult(QValidator::State state);

    /** Holds object-validators and their states. */
    QMap<QObjectValidator*, bool> m_group;
    /** Holds validation result. */
    bool m_fResult;
};

/* Page validator prototype: */
class UIPageValidator : public QObject
{
    Q_OBJECT;

signals:

    /* Notifier: Validation stuff: */
    void sigValidityChanged(UIPageValidator *pValidator);

    /* Notifiers: Warning stuff: */
    void sigShowWarningIcon();
    void sigHideWarningIcon();

public:

    /* Constructor: */
    UIPageValidator(QObject *pParent, UISettingsPage *pPage);

    /* API: Page stuff: */
    UISettingsPage* page() const { return m_pPage; }
    QPixmap warningPixmap() const;
    QString internalName() const;

    /* API: Validity stuff: */
    bool isValid() const { return m_fIsValid; }
    void setValid(bool fIsValid) { m_fIsValid = fIsValid; }

    /* API: Message stuff: */
    QString lastMessage() const { return m_strLastMessage; }
    void setLastMessage(const QString &strLastMessage);

public slots:

    /* API/Handler: Validation stuff: */
    void revalidate();

private:

    /* Variables: */
    UISettingsPage *m_pPage;
    bool m_fIsValid;
    QString m_strLastMessage;
};

class QIULongValidator : public QValidator
{
public:

    QIULongValidator (QObject *aParent)
        : QValidator (aParent)
        , mBottom (0), mTop (ULONG_MAX) {}

    QIULongValidator (ulong aMinimum, ulong aMaximum,
                      QObject *aParent)
        : QValidator (aParent)
        , mBottom (aMinimum), mTop (aMaximum) {}

    ~QIULongValidator() {}

    State validate (QString &aInput, int &aPos) const;
    void setBottom (ulong aBottom) { setRange (aBottom, mTop); }
    void setTop (ulong aTop) { setRange (mBottom, aTop); }
    void setRange (ulong aBottom, ulong aTop) { mBottom = aBottom; mTop = aTop; }
    ulong bottom() const { return mBottom; }
    ulong top() const { return mTop; }

private:

    ulong mBottom;
    ulong mTop;
};

#endif // __QIWidgetValidator_h__
