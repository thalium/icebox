/* $Id: UIHotKeyEditor.h $ */
/** @file
 * VBox Qt GUI - VirtualBox Qt extensions: UIHotKeyEditor class declaration.
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

#ifndef ___UIHotKeyEditor_h___
#define ___UIHotKeyEditor_h___

/* Qt includes: */
#include <QMetaType>
#include <QWidget>
#include <QSet>

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QHBoxLayout;
class QIToolButton;
class UIHotKeyLineEdit;

/* Host key type enumerator: */
enum UIHotKeyType
{
    UIHotKeyType_Simple,
    UIHotKeyType_WithModifiers
};

/* A string pair wrapper for hot-key sequence: */
class UIHotKey
{
public:

    /* Constructors: */
    UIHotKey()
        : m_type(UIHotKeyType_Simple)
    {}
    UIHotKey(UIHotKeyType type,
             const QString &strSequence,
             const QString &strDefaultSequence)
        : m_type(type)
        , m_strSequence(strSequence)
        , m_strDefaultSequence(strDefaultSequence)
    {}
    UIHotKey(const UIHotKey &other)
        : m_type(other.type())
        , m_strSequence(other.sequence())
        , m_strDefaultSequence(other.defaultSequence())
    {}

    /* API: Operators stuff: */
    UIHotKey& operator=(const UIHotKey &other)
    {
        m_type = other.type();
        m_strSequence = other.sequence();
        m_strDefaultSequence = other.defaultSequence();
        return *this;
    }

    /* API: Type access stuff: */
    UIHotKeyType type() const { return m_type; }

    /* API: Sequence access stuff: */
    const QString& sequence() const { return m_strSequence; }
    const QString& defaultSequence() const { return m_strDefaultSequence; }
    void setSequence(const QString &strSequence) { m_strSequence = strSequence; }

private:

    /* Variables: */
    UIHotKeyType m_type;
    QString m_strSequence;
    QString m_strDefaultSequence;
};
Q_DECLARE_METATYPE(UIHotKey);

/* A widget wrapper for real hot-key editor: */
class UIHotKeyEditor : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;
    Q_PROPERTY(UIHotKey hotKey READ hotKey WRITE setHotKey USER true);

signals:

    /** Notifies listener about data should be committed. */
    void sigCommitData(QWidget *pThis);

public:

    /* Constructor: */
    UIHotKeyEditor(QWidget *pParent);

private slots:

    /* Handlers: Tool-button stuff: */
    void sltReset();
    void sltClear();

private:

    /* Helper: Translate stuff: */
    void retranslateUi();

    /* Handlers: Line-edit key event pre-processing stuff: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);
    bool shouldWeSkipKeyEventToLineEdit(QKeyEvent *pEvent);

    /* Handlers: Key event processing stuff: */
    void keyPressEvent(QKeyEvent *pEvent);
    void keyReleaseEvent(QKeyEvent *pEvent);
    bool isKeyEventIgnored(QKeyEvent *pEvent);

    /* Helpers: Modifier stuff: */
    void fetchModifiersState();
    void checkIfHostModifierNeeded();

    /* Handlers: Sequence stuff: */
    bool approvedKeyPressed(QKeyEvent *pKeyEvent);
    void handleKeyPress(QKeyEvent *pKeyEvent);
    void handleKeyRelease(QKeyEvent *pKeyEvent);
    void reflectSequence();
    void drawSequence();

    /* API: Editor stuff: */
    UIHotKey hotKey() const;
    void setHotKey(const UIHotKey &hotKey);

    /* Variables: */
    UIHotKey m_hotKey;
    bool m_fIsModifiersAllowed;
    QHBoxLayout *m_pMainLayout;
    QHBoxLayout *m_pButtonLayout;
    UIHotKeyLineEdit *m_pLineEdit;
    QIToolButton *m_pResetButton;
    QIToolButton *m_pClearButton;
    QSet<int> m_takenModifiers;
    int m_iTakenKey;
    bool m_fSequenceTaken;
};

#endif /* !___UIHotKeyEditor_h___ */

