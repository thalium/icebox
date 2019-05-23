/* $Id: UIMenuBarEditorWindow.h $ */
/** @file
 * VBox Qt GUI - UIMenuBarEditorWindow class declaration.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIMenuBarEditorWindow_h___
#define ___UIMenuBarEditorWindow_h___

/* Qt includes: */
#include <QMap>

/* GUI includes: */
#include "UIExtraDataDefs.h"
#include "UISlidingToolBar.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class UIMachineWindow;
class UIActionPool;
class UIToolBar;
class UIAction;
class QIToolButton;
#ifndef VBOX_WS_MAC
class QCheckBox;
#endif /* !VBOX_WS_MAC */
class QHBoxLayout;
class QAction;
class QMenu;

/** UISlidingToolBar wrapper
  * providing user with possibility to edit menu-bar layout. */
class UIMenuBarEditorWindow : public UISlidingToolBar
{
    Q_OBJECT;

public:

    /** Constructor, passes @a pParent to the UISlidingToolBar constructor.
      * @param pActionPool brings the action-pool reference for internal use. */
    UIMenuBarEditorWindow(UIMachineWindow *pParent, UIActionPool *pActionPool);
};

/** QWidget reimplementation
  * used as menu-bar editor widget. */
class UIMenuBarEditorWidget : public QIWithRetranslateUI2<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies about Cancel button click. */
    void sigCancelClicked();

public:

    /** Constructor.
      * @param pParent                is passed to QWidget constructor,
      * @param fStartedFromVMSettings determines whether 'this' is a part of VM settings,
      * @param strMachineID           brings the machine ID to be used by the editor,
      * @param pActionPool            brings the action-pool to be used by the editor. */
    UIMenuBarEditorWidget(QWidget *pParent,
                          bool fStartedFromVMSettings = true,
                          const QString &strMachineID = QString(),
                          UIActionPool *pActionPool = 0);

    /** Returns the machine ID instance. */
    const QString& machineID() const { return m_strMachineID; }
    /** Defines the @a strMachineID instance. */
    void setMachineID(const QString &strMachineID);

    /** Returns the action-pool reference. */
    const UIActionPool* actionPool() const { return m_pActionPool; }
    /** Defines the @a pActionPool reference. */
    void setActionPool(UIActionPool *pActionPool);

#ifndef VBOX_WS_MAC
    /** Returns whether the menu-bar enabled. */
    bool isMenuBarEnabled() const;
    /** Defines whether the menu-bar @a fEnabled. */
    void setMenuBarEnabled(bool fEnabled);
#endif /* !VBOX_WS_MAC */

    /** Returns the cached restrictions of menu-bar. */
    UIExtraDataMetaDefs::MenuType restrictionsOfMenuBar() const { return m_restrictionsOfMenuBar; }
    /** Returns the cached restrictions of menu 'Application'. */
    UIExtraDataMetaDefs::MenuApplicationActionType restrictionsOfMenuApplication() const { return m_restrictionsOfMenuApplication; }
    /** Returns the cached restrictions of menu 'Machine'. */
    UIExtraDataMetaDefs::RuntimeMenuMachineActionType restrictionsOfMenuMachine() const { return m_restrictionsOfMenuMachine; }
    /** Returns the cached restrictions of menu 'View'. */
    UIExtraDataMetaDefs::RuntimeMenuViewActionType restrictionsOfMenuView() const { return m_restrictionsOfMenuView; }
    /** Returns the cached restrictions of menu 'Input'. */
    UIExtraDataMetaDefs::RuntimeMenuInputActionType restrictionsOfMenuInput() const { return m_restrictionsOfMenuInput; }
    /** Returns the cached restrictions of menu 'Devices'. */
    UIExtraDataMetaDefs::RuntimeMenuDevicesActionType restrictionsOfMenuDevices() const { return m_restrictionsOfMenuDevices; }
#ifdef VBOX_WITH_DEBUGGER_GUI
    /** Returns the cached restrictions of menu 'Debug'. */
    UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType restrictionsOfMenuDebug() const { return m_restrictionsOfMenuDebug; }
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    /** Mac OS X: Returns the cached restrictions of menu 'Window'. */
    UIExtraDataMetaDefs::MenuWindowActionType restrictionsOfMenuWindow() const { return m_restrictionsOfMenuWindow; }
#endif /* VBOX_WS_MAC */
    /** Returns the cached restrictions of menu 'Help'. */
    UIExtraDataMetaDefs::MenuHelpActionType restrictionsOfMenuHelp() const { return m_restrictionsOfMenuHelp; }

    /** Defines the cached @a restrictions of menu-bar. */
    void setRestrictionsOfMenuBar(UIExtraDataMetaDefs::MenuType restrictions);
    /** Defines the cached @a restrictions of menu 'Application'. */
    void setRestrictionsOfMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType restrictions);
    /** Defines the cached @a restrictions of menu 'Machine'. */
    void setRestrictionsOfMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType restrictions);
    /** Defines the cached @a restrictions of menu 'View'. */
    void setRestrictionsOfMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType restrictions);
    /** Defines the cached @a restrictions of menu 'Input'. */
    void setRestrictionsOfMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType restrictions);
    /** Defines the cached @a restrictions of menu 'Devices'. */
    void setRestrictionsOfMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType restrictions);
#ifdef VBOX_WITH_DEBUGGER_GUI
    /** Defines the cached @a restrictions of menu 'Debug'. */
    void setRestrictionsOfMenuDebug(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType restrictions);
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    /** Mac OS X: Defines the cached @a restrictions of menu 'Window'. */
    void setRestrictionsOfMenuWindow(UIExtraDataMetaDefs::MenuWindowActionType restrictions);
#endif /* VBOX_WS_MAC */
    /** Defines the cached @a restrictions of menu 'Help'. */
    void setRestrictionsOfMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType restrictions);

private slots:

    /** Handles configuration change. */
    void sltHandleConfigurationChange(const QString &strMachineID);

    /** Handles menu-bar menu click. */
    void sltHandleMenuBarMenuClick();

private:

    /** Prepare routine. */
    void prepare();

#ifdef VBOX_WS_MAC
    /** Prepare named menu routine. */
    QMenu* prepareNamedMenu(const QString &strName);
#endif /* VBOX_WS_MAC */
    /** Prepare copied menu routine. */
    QMenu* prepareCopiedMenu(const UIAction *pAction);
#if 0
    /** Prepare copied sub-menu routine. */
    QMenu* prepareCopiedSubMenu(QMenu *pMenu, const UIAction *pAction);
#endif
    /** Prepare named action routine. */
    QAction* prepareNamedAction(QMenu *pMenu, const QString &strName,
                                int iExtraDataID, const QString &strExtraDataID);
    /** Prepare copied action routine. */
    QAction* prepareCopiedAction(QMenu *pMenu, const UIAction *pAction);

    /** Prepare menus routine. */
    void prepareMenus();
    /** Prepare 'Application' menu routine. */
    void prepareMenuApplication();
    /** Prepare 'Machine' menu routine. */
    void prepareMenuMachine();
    /** Prepare 'View' menu routine. */
    void prepareMenuView();
    /** Prepare 'Input' menu routine. */
    void prepareMenuInput();
    /** Prepare 'Devices' menu routine. */
    void prepareMenuDevices();
#ifdef VBOX_WITH_DEBUGGER_GUI
    /** Prepare 'Debug' menu routine. */
    void prepareMenuDebug();
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    /** Mac OS X: Prepare 'Window' menu routine. */
    void prepareMenuWindow();
#endif /* VBOX_WS_MAC */
    /** Prepare 'Help' menu routine. */
    void prepareMenuHelp();

    /** Retranslation routine. */
    virtual void retranslateUi();

    /** Paint event handler. */
    virtual void paintEvent(QPaintEvent *pEvent);

    /** @name General
      * @{ */
        /** Holds whether 'this' is prepared. */
        bool m_fPrepared;
        /** Holds whether 'this' is a part of VM settings. */
        bool m_fStartedFromVMSettings;
        /** Holds the machine ID instance. */
        QString m_strMachineID;
        /** Holds the action-pool reference. */
        const UIActionPool *m_pActionPool;
    /** @} */

    /** @name Contents
      * @{ */
        /** Holds the main-layout instance. */
        QHBoxLayout *m_pMainLayout;
        /** Holds the tool-bar instance. */
        UIToolBar *m_pToolBar;
        /** Holds the close-button instance. */
        QIToolButton *m_pButtonClose;
#ifndef VBOX_WS_MAC
        /** Non Mac OS X: Holds the enable-checkbox instance. */
        QCheckBox *m_pCheckBoxEnable;
#endif /* !VBOX_WS_MAC */
        /** Holds tool-bar action references. */
        QMap<QString, QAction*> m_actions;
    /** @} */

    /** @name Contents: Restrictions
      * @{ */
        /** Holds the cached restrictions of menu-bar. */
        UIExtraDataMetaDefs::MenuType m_restrictionsOfMenuBar;
        /** Holds the cached restrictions of menu 'Application'. */
        UIExtraDataMetaDefs::MenuApplicationActionType m_restrictionsOfMenuApplication;
        /** Holds the cached restrictions of menu 'Machine'. */
        UIExtraDataMetaDefs::RuntimeMenuMachineActionType m_restrictionsOfMenuMachine;
        /** Holds the cached restrictions of menu 'View'. */
        UIExtraDataMetaDefs::RuntimeMenuViewActionType m_restrictionsOfMenuView;
        /** Holds the cached restrictions of menu 'Input'. */
        UIExtraDataMetaDefs::RuntimeMenuInputActionType m_restrictionsOfMenuInput;
        /** Holds the cached restrictions of menu 'Devices'. */
        UIExtraDataMetaDefs::RuntimeMenuDevicesActionType m_restrictionsOfMenuDevices;
#ifdef VBOX_WITH_DEBUGGER_GUI
        /** Holds the cached restrictions of menu 'Debug'. */
        UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType m_restrictionsOfMenuDebug;
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
        /** Mac OS X: Holds the cached restrictions of menu 'Window'. */
        UIExtraDataMetaDefs::MenuWindowActionType m_restrictionsOfMenuWindow;
#endif /* VBOX_WS_MAC */
        /** Holds the cached restrictions of menu 'Help'. */
        UIExtraDataMetaDefs::MenuHelpActionType m_restrictionsOfMenuHelp;
    /** @} */
};

#endif /* !___UIMenuBarEditorWindow_h___ */
