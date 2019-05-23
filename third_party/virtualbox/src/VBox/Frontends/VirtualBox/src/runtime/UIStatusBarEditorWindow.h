/* $Id: UIStatusBarEditorWindow.h $ */
/** @file
 * VBox Qt GUI - UIStatusBarEditorWindow class declaration.
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

#ifndef ___UIStatusBarEditorWindow_h___
#define ___UIStatusBarEditorWindow_h___

/* Qt includes: */
#include <QMap>
#include <QList>

/* GUI includes: */
#include "UIExtraDataDefs.h"
#include "UISlidingToolBar.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class UIStatusBarEditorButton;
class UIMachineWindow;
class QIToolButton;
class QHBoxLayout;
class QCheckBox;

/** UISlidingToolBar wrapper
  * providing user with possibility to edit status-bar layout. */
class UIStatusBarEditorWindow : public UISlidingToolBar
{
    Q_OBJECT;

public:

    /** Constructor, passes @a pParent to the UISlidingToolBar constructor. */
    UIStatusBarEditorWindow(UIMachineWindow *pParent);
};

/** QWidget reimplementation
  * used as status-bar editor widget. */
class UIStatusBarEditorWidget : public QIWithRetranslateUI2<QWidget>
{
    Q_OBJECT;

signals:

    /** Notifies about Cancel button click. */
    void sigCancelClicked();

public:

    /** Constructor,
      * @param pParent                is passed to QWidget constructor,
      * @param fStartedFromVMSettings determines whether 'this' is a part of VM settings,
      * @param strMachineID           brings the machine ID to be used by the editor. */
    UIStatusBarEditorWidget(QWidget *pParent,
                            bool fStartedFromVMSettings = true,
                            const QString &strMachineID = QString());

    /** Returns the machine ID instance. */
    const QString& machineID() const { return m_strMachineID; }
    /** Defines the @a strMachineID instance. */
    void setMachineID(const QString &strMachineID);

    /** Returns whether the status-bar enabled. */
    bool isStatusBarEnabled() const;
    /** Defines whether the status-bar @a fEnabled. */
    void setStatusBarEnabled(bool fEnabled);

    /** Returns status-bar indicator restrictions. */
    const QList<IndicatorType>& statusBarIndicatorRestrictions() const { return m_restrictions; }
    /** Returns status-bar indicator order. */
    const QList<IndicatorType>& statusBarIndicatorOrder() const { return m_order; }
    /** Defines status-bar indicator @a restrictions and @a order. */
    void setStatusBarConfiguration(const QList<IndicatorType> &restrictions, const QList<IndicatorType> &order);

private slots:

    /** Handles configuration change. */
    void sltHandleConfigurationChange(const QString &strMachineID);

    /** Handles button click. */
    void sltHandleButtonClick();

    /** Handles drag object destroy. */
    void sltHandleDragObjectDestroy();

private:

    /** Prepare routine. */
    void prepare();
    /** Prepare status buttons routine. */
    void prepareStatusButtons();
    /** Prepare status button routine. */
    void prepareStatusButton(IndicatorType type);

    /** Retranslation routine. */
    virtual void retranslateUi();

    /** Paint event handler. */
    virtual void paintEvent(QPaintEvent *pEvent);

    /** Drag-enter event handler. */
    virtual void dragEnterEvent(QDragEnterEvent *pEvent);
    /** Drag-move event handler. */
    virtual void dragMoveEvent(QDragMoveEvent *pEvent);
    /** Drag-leave event handler. */
    virtual void dragLeaveEvent(QDragLeaveEvent *pEvent);
    /** Drop event handler. */
    virtual void dropEvent(QDropEvent *pEvent);

    /** Returns position for passed @a type. */
    int position(IndicatorType type) const;

    /** @name General
      * @{ */
        /** Holds whether 'this' is prepared. */
        bool m_fPrepared;
        /** Holds whether 'this' is a part of VM settings. */
        bool m_fStartedFromVMSettings;
        /** Holds the machine ID instance. */
        QString m_strMachineID;
    /** @} */

    /** @name Contents
      * @{ */
        /** Holds the main-layout instance. */
        QHBoxLayout *m_pMainLayout;
        /** Holds the button-layout instance. */
        QHBoxLayout *m_pButtonLayout;
        /** Holds the close-button instance. */
        QIToolButton *m_pButtonClose;
        /** Holds the enable-checkbox instance. */
        QCheckBox *m_pCheckBoxEnable;
        /** Holds status-bar buttons. */
        QMap<IndicatorType, UIStatusBarEditorButton*> m_buttons;
    /** @} */

    /** @name Contents: Restrictions
      * @{ */
        /** Holds the cached status-bar button restrictions. */
        QList<IndicatorType> m_restrictions;
    /** @} */

    /** @name Contents: Order
      * @{ */
        /** Holds the cached status-bar button order. */
        QList<IndicatorType> m_order;
        /** Holds the token-button to drop dragged-button nearby. */
        UIStatusBarEditorButton *m_pButtonDropToken;
        /** Holds whether dragged-button should be dropped <b>after</b> the token-button. */
        bool m_fDropAfterTokenButton;
    /** @} */
};

#endif /* !___UIStatusBarEditorWindow_h___ */
