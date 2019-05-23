/* $Id: UIShortcutPool.h $ */
/** @file
 * VBox Qt GUI - UIShortcutPool class declaration.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIShortcutPool_h__
#define __UIShortcutPool_h__

/* Qt includes: */
#include <QMap>

/* GUI includes: */
#include "VBoxGlobal.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class UIActionPool;
class UIAction;

/* Shortcut descriptor: */
class UIShortcut
{
public:

    /* Constructors: */
    UIShortcut()
        : m_strDescription(QString())
        , m_sequence(QKeySequence())
        , m_defaultSequence(QKeySequence())
    {}
    UIShortcut(const QString &strDescription,
               const QKeySequence &sequence,
               const QKeySequence &defaultSequence)
        : m_strDescription(strDescription)
        , m_sequence(sequence)
        , m_defaultSequence(defaultSequence)
    {}

    /* API: Description stuff: */
    void setDescription(const QString &strDescription);
    const QString& description() const;

    /* API: Sequence stuff: */
    void setSequence(const QKeySequence &sequence);
    const QKeySequence& sequence() const;

    /* API: Default sequence stuff: */
    void setDefaultSequence(const QKeySequence &defaultSequence);
    const QKeySequence& defaultSequence() const;

    /* API: Conversion stuff: */
    QString toString() const;

private:

    /* Variables: */
    QString m_strDescription;
    QKeySequence m_sequence;
    QKeySequence m_defaultSequence;
};

/* Singleton shortcut pool: */
class UIShortcutPool : public QIWithRetranslateUI3<QObject>
{
    Q_OBJECT;

signals:

    /* Notifiers: Extra-data stuff: */
    void sigSelectorShortcutsReloaded();
    void sigMachineShortcutsReloaded();

public:

    /* API: Singleton stuff: */
    static UIShortcutPool* instance();
    static void create();
    static void destroy();

    /* API: Shortcut stuff: */
    UIShortcut& shortcut(UIActionPool *pActionPool, UIAction *pAction);
    UIShortcut& shortcut(const QString &strPoolID, const QString &strActionID);
    const QMap<QString, UIShortcut>& shortcuts() const { return m_shortcuts; }
    void setOverrides(const QMap<QString, QString> &overrides);

    /* API: Action-pool stuff: */
    void applyShortcuts(UIActionPool *pActionPool);

private slots:

    /* Handlers: Extra-data stuff: */
    void sltReloadSelectorShortcuts();
    void sltReloadMachineShortcuts();

private:

    /* Constructor/destructor: */
    UIShortcutPool();
    ~UIShortcutPool();

    /* Helpers: Prepare stuff: */
    void prepare();
    void prepareConnections();

    /* Helper: Cleanup stuff: */
    void cleanup() {}

    /** Translation handler. */
    void retranslateUi();

    /* Helpers: Shortcuts stuff: */
    void loadDefaults();
    void loadDefaultsFor(const QString &strPoolExtraDataID);
    void loadOverrides();
    void loadOverridesFor(const QString &strPoolExtraDataID);
    void saveOverrides();
    void saveOverridesFor(const QString &strPoolExtraDataID);

    /* Helper: Shortcut stuff: */
    UIShortcut& shortcut(const QString &strShortcutKey);

    /* Variables: */
    static UIShortcutPool *m_pInstance;
    /** Shortcut key template. */
    static const QString m_sstrShortcutKeyTemplate;
    /** Shortcut key template for Runtime UI. */
    static const QString m_sstrShortcutKeyTemplateRuntime;
    QMap<QString, UIShortcut> m_shortcuts;
};

#define gShortcutPool UIShortcutPool::instance()

#endif /* __UIShortcutPool_h__ */

