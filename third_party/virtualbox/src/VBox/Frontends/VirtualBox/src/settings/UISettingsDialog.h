/* $Id: UISettingsDialog.h $ */
/** @file
 * VBox Qt GUI - UISettingsDialog class declaration.
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

#ifndef __UISettingsDialog_h__
#define __UISettingsDialog_h__

/* VBox includes: */
#include "QIMainDialog.h"
#include "QIWithRetranslateUI.h"
#include "UISettingsDialog.gen.h"
#include "UISettingsDefs.h"

/* Forward declarations: */
class UIPageValidator;
class QProgressBar;
class QStackedWidget;
class QTimer;
class UIWarningPane;
class UISettingsSelector;
class UISettingsPage;
class UISettingsSerializer;

/* Using declarations: */
using namespace UISettingsDefs;

/* Base dialog class for both Global & VM settings which encapsulates most of their similar functionalities */
class UISettingsDialog : public QIWithRetranslateUI<QIMainDialog>, public Ui::UISettingsDialog
{
    Q_OBJECT;

public:

    /* Settings Dialog Constructor/Destructor: */
    UISettingsDialog(QWidget *pParent);
   ~UISettingsDialog();

    /* Execute API: */
    void execute();

protected slots:

    /** Hides the modal dialog and sets the result code to Accepted. */
    virtual void accept();

    /* Category-change slot: */
    virtual void sltCategoryChanged(int cId);

    /* Mark dialog as loaded: */
    virtual void sltMarkLoaded();
    /* Mark dialog as saved: */
    virtual void sltMarkSaved();

    /* Handlers for process bar: */
    void sltHandleProcessStarted();
    void sltHandleProcessProgressChange(int iValue);

protected:

    /** Returns the serialize process instance. */
    UISettingsSerializer* serializeProcess() const { return m_pSerializeProcess; }
    /** Returns whether the serialization is in progress. */
    bool isSerializationInProgress() const { return m_fSerializationIsInProgress; }

    /** Loads the @a data. */
    void loadData(QVariant &data);
    /** Wrapper for the method above.
      * Loads the data from the corresponding source. */
    virtual void loadOwnData() = 0;

    /** Saves the @a data. */
    void saveData(QVariant &data);
    /** Wrapper for the method above.
      * Saves the data to the corresponding source. */
    virtual void saveOwnData() = 0;

    /* UI translator: */
    virtual void retranslateUi();

    /** Returns configuration access level. */
    ConfigurationAccessLevel configurationAccessLevel() { return m_configurationAccessLevel; }
    /** Defines configuration access level. */
    void setConfigurationAccessLevel(ConfigurationAccessLevel newConfigurationAccessLevel);

    /** Returns the dialog title extension. */
    virtual QString titleExtension() const = 0;
    /** Returns the dialog title. */
    virtual QString title() const = 0;

    /* Add settings page: */
    void addItem(const QString &strBigIcon, const QString &strMediumIcon, const QString &strSmallIcon,
                 int cId, const QString &strLink,
                 UISettingsPage* pSettingsPage = 0, int iParentId = -1);

    /* Helpers: Validation stuff: */
    virtual void recorrelate(UISettingsPage *pSettingsPage) { Q_UNUSED(pSettingsPage); }
    void revalidate(UIPageValidator *pValidator);
    void revalidate();

    /* Protected variables: */
    UISettingsSelector *m_pSelector;
    QStackedWidget *m_pStack;

private slots:

    /* Handlers: Validation stuff: */
    void sltHandleValidityChange(UIPageValidator *pValidator);
    void sltHandleWarningPaneHovered(UIPageValidator *pValidator);
    void sltHandleWarningPaneUnhovered(UIPageValidator *pValidator);

    /* Slot to update whats-this: */
    void sltUpdateWhatsThis(bool fGotFocus = false);

    /* Slot to handle reject: */
    void reject();

private:

    /* Event-handlers: */
    bool eventFilter(QObject *pObject, QEvent *pEvent);
    void showEvent(QShowEvent *pEvent);

    /* Helper: Validation stuff: */
    void assignValidator(UISettingsPage *pPage);

    /** Holds configuration access level. */
    ConfigurationAccessLevel m_configurationAccessLevel;

    /* Global Flags: */
    bool m_fPolished;

    /** Holds the serialize process instance. */
    UISettingsSerializer *m_pSerializeProcess;
    /** Holds whether the serialization is in progress. */
    bool m_fSerializationIsInProgress;
    /** Holds whether there were no serialization errors. */
    bool m_fSerializationClean;

    /* Status bar widget: */
    QStackedWidget *m_pStatusBar;

    /* Process bar widget: */
    QProgressBar *m_pProcessBar;

    /* Error & Warning stuff: */
    UIWarningPane *m_pWarningPane;
    bool m_fValid;
    bool m_fSilent;
    QString m_strWarningHint;

    /* Whats-This stuff: */
    QTimer *m_pWhatsThisTimer;
    QPointer<QWidget> m_pWhatsThisCandidate;

    QMap<int, int> m_pages;
#ifdef VBOX_WS_MAC
    QList<QSize> m_sizeList;
#endif /* VBOX_WS_MAC */
};

#endif // __UISettingsDialog_h__

