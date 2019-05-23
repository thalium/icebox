/* $Id: UIShortcutPool.cpp $ */
/** @file
 * VBox Qt GUI - UIShortcutPool class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* GUI includes: */
# include "UIShortcutPool.h"
# include "UIActionPool.h"
# include "UIExtraDataManager.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* Namespaces: */
using namespace UIExtraDataDefs;


void UIShortcut::setDescription(const QString &strDescription)
{
    m_strDescription = strDescription;
}

const QString& UIShortcut::description() const
{
    return m_strDescription;
}

void UIShortcut::setSequence(const QKeySequence &sequence)
{
    m_sequence = sequence;
}

const QKeySequence& UIShortcut::sequence() const
{
    return m_sequence;
}

void UIShortcut::setDefaultSequence(const QKeySequence &defaultSequence)
{
    m_defaultSequence = defaultSequence;
}

const QKeySequence& UIShortcut::defaultSequence() const
{
    return m_defaultSequence;
}

QString UIShortcut::toString() const
{
    return m_sequence.toString();
}

UIShortcutPool* UIShortcutPool::m_pInstance = 0;

const QString UIShortcutPool::m_sstrShortcutKeyTemplate = QString("%1/%2");
const QString UIShortcutPool::m_sstrShortcutKeyTemplateRuntime = m_sstrShortcutKeyTemplate.arg(GUI_Input_MachineShortcuts);

UIShortcutPool* UIShortcutPool::instance()
{
    return m_pInstance;
}

void UIShortcutPool::create()
{
    /* Check that instance do NOT exists: */
    if (m_pInstance)
        return;

    /* Create instance: */
    new UIShortcutPool;

    /* Prepare instance: */
    m_pInstance->prepare();
}

void UIShortcutPool::destroy()
{
    /* Check that instance exists: */
    if (!m_pInstance)
        return;

    /* Cleanup instance: */
    m_pInstance->cleanup();

    /* Delete instance: */
    delete m_pInstance;
}

UIShortcut& UIShortcutPool::shortcut(UIActionPool *pActionPool, UIAction *pAction)
{
    /* Compose shortcut key: */
    const QString strShortcutKey(m_sstrShortcutKeyTemplate.arg(pActionPool->shortcutsExtraDataID(),
                                                               pAction->shortcutExtraDataID()));
    /* Return existing if any: */
    if (m_shortcuts.contains(strShortcutKey))
        return shortcut(strShortcutKey);
    /* Create and return new one: */
    UIShortcut &newShortcut = m_shortcuts[strShortcutKey];
    newShortcut.setDescription(pAction->name());
    newShortcut.setSequence(pAction->defaultShortcut(pActionPool->type()));
    newShortcut.setDefaultSequence(pAction->defaultShortcut(pActionPool->type()));
    return newShortcut;
}

UIShortcut& UIShortcutPool::shortcut(const QString &strPoolID, const QString &strActionID)
{
    /* Return if present, autocreate if necessary: */
    return shortcut(m_sstrShortcutKeyTemplate.arg(strPoolID, strActionID));
}

void UIShortcutPool::setOverrides(const QMap<QString, QString> &overrides)
{
    /* Iterate over all the overrides: */
    const QList<QString> shortcutKeys = overrides.keys();
    foreach (const QString &strShortcutKey, shortcutKeys)
    {
        /* Make no changes if there is no such shortcut: */
        if (!m_shortcuts.contains(strShortcutKey))
            continue;
        /* Assign overridden sequence to the shortcut: */
        m_shortcuts[strShortcutKey].setSequence(overrides[strShortcutKey]);
    }
    /* Save overrides: */
    saveOverrides();
}

void UIShortcutPool::applyShortcuts(UIActionPool *pActionPool)
{
    /* For each the action of the passed action-pool: */
    foreach (UIAction *pAction, pActionPool->actions())
    {
        /* Skip menu actions: */
        if (pAction->type() == UIActionType_Menu)
            continue;

        /* Compose shortcut key: */
        const QString strShortcutKey = m_sstrShortcutKeyTemplate.arg(pActionPool->shortcutsExtraDataID(),
                                                                     pAction->shortcutExtraDataID());
        /* If shortcut key is already known: */
        if (m_shortcuts.contains(strShortcutKey))
        {
            /* Get corresponding shortcut: */
            UIShortcut &existingShortcut = m_shortcuts[strShortcutKey];
            /* Copy the description from the action to the shortcut: */
            existingShortcut.setDescription(pAction->name());
            /* Copy the sequence from the shortcut to the action: */
            pAction->setShortcut(existingShortcut.sequence());
            /* Copy the default sequence from the action to the shortcut: */
            existingShortcut.setDefaultSequence(pAction->defaultShortcut(pActionPool->type()));
        }
        /* If shortcut key is NOT known yet: */
        else
        {
            /* Create corresponding shortcut: */
            UIShortcut &newShortcut = m_shortcuts[strShortcutKey];
            /* Copy the action's default to both the shortcut & the action: */
            newShortcut.setSequence(pAction->defaultShortcut(pActionPool->type()));
            newShortcut.setDefaultSequence(pAction->defaultShortcut(pActionPool->type()));
            pAction->setShortcut(newShortcut.sequence());
            /* Copy the description from the action to the shortcut: */
            newShortcut.setDescription(pAction->name());
        }
    }
}

void UIShortcutPool::sltReloadSelectorShortcuts()
{
    /* Clear selector shortcuts first: */
    const QList<QString> shortcutKeyList = m_shortcuts.keys();
    foreach (const QString &strShortcutKey, shortcutKeyList)
        if (strShortcutKey.startsWith(GUI_Input_SelectorShortcuts))
            m_shortcuts.remove(strShortcutKey);

    /* Load selector defaults: */
    loadDefaultsFor(GUI_Input_SelectorShortcuts);
    /* Load selector overrides: */
    loadOverridesFor(GUI_Input_SelectorShortcuts);

    /* Notify selector shortcuts reloaded: */
    emit sigSelectorShortcutsReloaded();
}

void UIShortcutPool::sltReloadMachineShortcuts()
{
    /* Clear machine shortcuts first: */
    const QList<QString> shortcutKeyList = m_shortcuts.keys();
    foreach (const QString &strShortcutKey, shortcutKeyList)
        if (strShortcutKey.startsWith(GUI_Input_MachineShortcuts))
            m_shortcuts.remove(strShortcutKey);

    /* Load machine defaults: */
    loadDefaultsFor(GUI_Input_MachineShortcuts);
    /* Load machine overrides: */
    loadOverridesFor(GUI_Input_MachineShortcuts);

    /* Notify machine shortcuts reloaded: */
    emit sigMachineShortcutsReloaded();
}

UIShortcutPool::UIShortcutPool()
{
    /* Prepare instance: */
    if (!m_pInstance)
        m_pInstance = this;
}

UIShortcutPool::~UIShortcutPool()
{
    /* Cleanup instance: */
    if (m_pInstance == this)
        m_pInstance = 0;
}

void UIShortcutPool::prepare()
{
    /* Load defaults: */
    loadDefaults();
    /* Load overrides: */
    loadOverrides();
    /* Prepare connections: */
    prepareConnections();
}

void UIShortcutPool::prepareConnections()
{
    /* Connect to extra-data signals: */
    connect(gEDataManager, &UIExtraDataManager::sigSelectorUIShortcutChange,
            this, &UIShortcutPool::sltReloadSelectorShortcuts);
    connect(gEDataManager, &UIExtraDataManager::sigRuntimeUIShortcutChange,
            this, &UIShortcutPool::sltReloadMachineShortcuts);
}

void UIShortcutPool::retranslateUi()
{
    /* Translate own defaults: */
    m_shortcuts[m_sstrShortcutKeyTemplateRuntime.arg("PopupMenu")]
        .setDescription(QApplication::translate("UIActionPool", "Popup Menu"));
}

void UIShortcutPool::loadDefaults()
{
    /* Load selector defaults: */
    loadDefaultsFor(GUI_Input_SelectorShortcuts);
    /* Load machine defaults: */
    loadDefaultsFor(GUI_Input_MachineShortcuts);
}

void UIShortcutPool::loadDefaultsFor(const QString &strPoolExtraDataID)
{
    /* Default shortcuts for Selector UI: */
    if (strPoolExtraDataID == GUI_Input_SelectorShortcuts)
    {
        /* Nothing for now.. */
    }
    /* Default shortcuts for Runtime UI: */
    else if (strPoolExtraDataID == GUI_Input_MachineShortcuts)
    {
        /* Default shortcut for the Runtime Popup Menu: */
        m_shortcuts.insert(m_sstrShortcutKeyTemplateRuntime.arg("PopupMenu"),
                           UIShortcut(QApplication::translate("UIActionPool", "Popup Menu"),
                                      QString("Home"), QString("Home")));
    }
}

void UIShortcutPool::loadOverrides()
{
    /* Load selector overrides: */
    loadOverridesFor(GUI_Input_SelectorShortcuts);
    /* Load machine overrides: */
    loadOverridesFor(GUI_Input_MachineShortcuts);
}

void UIShortcutPool::loadOverridesFor(const QString &strPoolExtraDataID)
{
    /* Compose shortcut key template: */
    const QString strShortcutKeyTemplate(m_sstrShortcutKeyTemplate.arg(strPoolExtraDataID));
    /* Iterate over all the overrides: */
    const QStringList overrides = gEDataManager->shortcutOverrides(strPoolExtraDataID);
    foreach (const QString &strKeyValuePair, overrides)
    {
        /* Make sure override structure is valid: */
        int iDelimiterPosition = strKeyValuePair.indexOf('=');
        if (iDelimiterPosition < 0)
            continue;

        /* Get shortcut ID/sequence: */
        QString strShortcutExtraDataID = strKeyValuePair.left(iDelimiterPosition);
        const QString strShortcutSequence = strKeyValuePair.right(strKeyValuePair.length() - iDelimiterPosition - 1);

        // Hack for handling "Save" as "SaveState":
        if (strShortcutExtraDataID == "Save")
            strShortcutExtraDataID = "SaveState";

        /* Compose corresponding shortcut key: */
        const QString strShortcutKey(strShortcutKeyTemplate.arg(strShortcutExtraDataID));
        /* Modify map with composed key/value: */
        if (!m_shortcuts.contains(strShortcutKey))
            m_shortcuts.insert(strShortcutKey, UIShortcut(QString(), strShortcutSequence, QString()));
        else
        {
            /* Get corresponding value: */
            UIShortcut &shortcut = m_shortcuts[strShortcutKey];
            /* Check if corresponding shortcut overridden by value: */
            if (shortcut.toString().compare(strShortcutSequence, Qt::CaseInsensitive) != 0)
            {
                /* Shortcut unassigned? */
                if (strShortcutSequence.compare("None", Qt::CaseInsensitive) == 0)
                    shortcut.setSequence(QKeySequence());
                /* Or reassigned? */
                else
                    shortcut.setSequence(QKeySequence(strShortcutSequence));
            }
        }
    }
}

void UIShortcutPool::saveOverrides()
{
    /* Load selector overrides: */
    saveOverridesFor(GUI_Input_SelectorShortcuts);
    /* Load machine overrides: */
    saveOverridesFor(GUI_Input_MachineShortcuts);
}

void UIShortcutPool::saveOverridesFor(const QString &strPoolExtraDataID)
{
    /* Compose shortcut prefix: */
    const QString strShortcutPrefix(m_sstrShortcutKeyTemplate.arg(strPoolExtraDataID, QString()));
    /* Populate the list of all the known overrides: */
    QStringList overrides;
    const QList<QString> shortcutKeys = m_shortcuts.keys();
    foreach (const QString &strShortcutKey, shortcutKeys)
    {
        /* Check if the key starts from the proper prefix: */
        if (!strShortcutKey.startsWith(strShortcutPrefix))
            continue;
        /* Get corresponding shortcut: */
        const UIShortcut &shortcut = m_shortcuts[strShortcutKey];
        /* Check if the sequence for that shortcut differs from default: */
        if (shortcut.sequence() == shortcut.defaultSequence())
            continue;
        /* Add the shortcut sequence into overrides list: */
        overrides << QString("%1=%2").arg(QString(strShortcutKey).remove(strShortcutPrefix),
                                          shortcut.sequence().toString());
    }
    /* Save overrides into the extra-data: */
    vboxGlobal().virtualBox().SetExtraDataStringList(strPoolExtraDataID, overrides);
}

UIShortcut& UIShortcutPool::shortcut(const QString &strShortcutKey)
{
    return m_shortcuts[strShortcutKey];
}

