/* $Id: UISettingsPage.h $ */
/** @file
 * VBox Qt GUI - UISettingsPage class declaration.
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

#ifndef __UISettingsPage_h__
#define __UISettingsPage_h__

/* Qt includes: */
#include <QWidget>
#include <QVariant>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UISettingsDefs.h"
#include "UIExtraDataDefs.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMachine.h"
#include "CConsole.h"
#include "CSystemProperties.h"

/* Forward declarations: */
class UIPageValidator;
class QShowEvent;

/* Using declarations: */
using namespace UISettingsDefs;

/* Settings page types: */
enum UISettingsPageType
{
    UISettingsPageType_Global,
    UISettingsPageType_Machine
};

/* Global settings data wrapper: */
struct UISettingsDataGlobal
{
    UISettingsDataGlobal() {}
    UISettingsDataGlobal(const CSystemProperties &properties)
        : m_properties(properties) {}
    CSystemProperties m_properties;
};
Q_DECLARE_METATYPE(UISettingsDataGlobal);

/* Machine settings data wrapper: */
struct UISettingsDataMachine
{
    UISettingsDataMachine() {}
    UISettingsDataMachine(const CMachine &machine, const CConsole &console)
        : m_machine(machine), m_console(console) {}
    CMachine m_machine;
    CConsole m_console;
};
Q_DECLARE_METATYPE(UISettingsDataMachine);

/* Validation message type: */
typedef QPair<QString, QStringList> UIValidationMessage;

/* Settings page base class: */
class UISettingsPage : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about particular operation progress change.
      * @param iOperations  holds the number of operations CProgress have,
      * @param strOperation holds the description of the current CProgress operation,
      * @param iOperation   holds the index of the current CProgress operation,
      * @param iPercent     holds the percentage of the current CProgress operation. */
    void sigOperationProgressChange(ulong iOperations, QString strOperation,
                                    ulong iOperation, ulong iPercent);

    /** Notifies listeners about particular COM error.
      * @param strErrorInfo holds the details of the error happened. */
    void sigOperationProgressError(QString strErrorInfo);

public:

    /** Loads data into the cache from corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    virtual void loadToCacheFrom(QVariant &data) = 0;
    /** Loads data into corresponding widgets from the cache,
      * this task SHOULD be performed in the GUI thread only. */
    virtual void getFromCache() = 0;

    /** Saves data from corresponding widgets to the cache,
      * this task SHOULD be performed in the GUI thread only. */
    virtual void putToCache() = 0;
    /** Saves data from the cache to corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    virtual void saveFromCacheTo(QVariant &data) = 0;

    /** Notifies listeners about particular COM error.
      * @param  strErrorInfo  Brings the details of the error happened. */
    void notifyOperationProgressError(const QString &strErrorInfo);

    /* Validation stuff: */
    void setValidator(UIPageValidator *pValidator);
    void setValidatorBlocked(bool fIsValidatorBlocked) { m_fIsValidatorBlocked = fIsValidatorBlocked; }
    virtual bool validate(QList<UIValidationMessage>& /* messages */) { return true; }

    /* Navigation stuff: */
    QWidget* firstWidget() const { return m_pFirstWidget; }
    virtual void setOrderAfter(QWidget *pWidget) { m_pFirstWidget = pWidget; }

    /* Settings page type stuff: */
    UISettingsPageType pageType() const { return m_pageType; }

    /* Configuration access level stuff: */
    ConfigurationAccessLevel configurationAccessLevel() const { return m_configurationAccessLevel; }
    virtual void setConfigurationAccessLevel(ConfigurationAccessLevel newConfigurationAccessLevel) { m_configurationAccessLevel = newConfigurationAccessLevel; polishPage(); }
    bool isMachineOffline() const { return configurationAccessLevel() == ConfigurationAccessLevel_Full; }
    bool isMachinePoweredOff() const { return configurationAccessLevel() == ConfigurationAccessLevel_Partial_PoweredOff; }
    bool isMachineSaved() const { return configurationAccessLevel() == ConfigurationAccessLevel_Partial_Saved; }
    bool isMachineOnline() const { return configurationAccessLevel() == ConfigurationAccessLevel_Partial_Running; }
    bool isMachineInValidMode() const { return isMachineOffline() || isMachinePoweredOff() || isMachineSaved() || isMachineOnline(); }

    /** Returns whether the page content was changed. */
    virtual bool changed() const = 0;

    /* Page 'ID' stuff: */
    int id() const { return m_cId; }
    void setId(int cId) { m_cId = cId; }

    /* Page 'name' stuff: */
    virtual QString internalName() const = 0;

    /* Page 'warning pixmap' stuff: */
    virtual QPixmap warningPixmap() const = 0;

    /* Page 'processed' stuff: */
    bool processed() const { return m_fProcessed; }
    void setProcessed(bool fProcessed) { m_fProcessed = fProcessed; }

    /* Page 'failed' stuff: */
    bool failed() const { return m_fFailed; }
    void setFailed(bool fFailed) { m_fFailed = fFailed; }

    /* Virtual function to polish page content: */
    virtual void polishPage() {}

public slots:

    /* Handler: Validation stuff: */
    void revalidate();

protected:

    /* Settings page constructor, hidden: */
    UISettingsPage(UISettingsPageType type);

private:

    /* Variables: */
    UISettingsPageType m_pageType;
    ConfigurationAccessLevel m_configurationAccessLevel;
    int m_cId;
    bool m_fProcessed;
    bool m_fFailed;
    QWidget *m_pFirstWidget;
    UIPageValidator *m_pValidator;
    bool m_fIsValidatorBlocked;
};

/* Global settings page class: */
class UISettingsPageGlobal : public UISettingsPage
{
    Q_OBJECT;

protected:

    /* Global settings page constructor, hidden: */
    UISettingsPageGlobal();

    /* Page 'ID' stuff: */
    GlobalSettingsPageType internalID() const;

    /* Page 'name' stuff: */
    QString internalName() const;

    /* Page 'warning pixmap' stuff: */
    QPixmap warningPixmap() const;

    /* Fetch data to m_properties & m_settings: */
    void fetchData(const QVariant &data);

    /* Upload m_properties & m_settings to data: */
    void uploadData(QVariant &data) const;

    /** Returns whether the page content was changed. */
    bool changed() const { return false; }

    /* Global data source: */
    CSystemProperties m_properties;
};

/* Machine settings page class: */
class UISettingsPageMachine : public UISettingsPage
{
    Q_OBJECT;

protected:

    /* Machine settings page constructor, hidden: */
    UISettingsPageMachine();

    /* Page 'ID' stuff: */
    MachineSettingsPageType internalID() const;

    /* Page 'name' stuff: */
    QString internalName() const;

    /* Page 'warning pixmap' stuff: */
    QPixmap warningPixmap() const;

    /* Fetch data to m_machine: */
    void fetchData(const QVariant &data);

    /* Upload m_machine to data: */
    void uploadData(QVariant &data) const;

    /* Machine data source: */
    CMachine m_machine;
    CConsole m_console;
};

#endif // __UISettingsPage_h__

