/* $Id: UIMenuBarEditorWindow.cpp $ */
/** @file
 * VBox Qt GUI - UIMenuBarEditorWindow class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QAccessibleWidget>
# include <QHBoxLayout>
# include <QMenu>
# include <QMenuBar>
# include <QMetaEnum>
# include <QPainter>
# include <QPaintEvent>
# include <QStyleOptionToolButton>
# ifndef VBOX_WS_MAC
#  include <QCheckBox>
# endif /* !VBOX_WS_MAC */

/* GUI includes: */
# include "UIMenuBarEditorWindow.h"
# include "UIActionPoolRuntime.h"
# include "UIExtraDataManager.h"
# include "UIMachineWindow.h"
# include "UIConverter.h"
# include "UIIconPool.h"
# include "UIToolBar.h"
# include "QIToolButton.h"
# include "VBoxGlobal.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Forward declarations: */
class UIAccessibilityInterfaceForUIMenuBarEditorButton;


/** Menu-bar editor button segment types. */
enum UIMenuBarEditorSegment
{
    UIMenuBarEditorSegment_Button,
    UIMenuBarEditorSegment_Menu,
    UIMenuBarEditorSegment_Max,
};


/** QAccessibleWidget extension used as an accessibility interface for UIMenuBarEditor button segments. */
class UIAccessibilityInterfaceForUIMenuBarEditorButtonSegment : public QAccessibleInterface
{
public:

    /** Constructs an accessibility interface.
      * @param  pParent   Brings the parent interface we are linked to.
      * @param  enmIndex  Brings the index of segment we are referring to. */
    UIAccessibilityInterfaceForUIMenuBarEditorButtonSegment(UIAccessibilityInterfaceForUIMenuBarEditorButton *pParent,
                                                            UIMenuBarEditorSegment enmIndex);

    /** Returns whether the interface is valid. */
    virtual bool isValid() const /* override */ { return true; }

    /** Returns the wrapped object. */
    virtual QObject *object() const /* override */ { return 0; }
    /** Returns the parent. */
    virtual QAccessibleInterface *parent() const /* override */;

    /** Returns the number of children. */
    virtual int childCount() const /* override */ { return 0; }
    /** Returns the child with the passed @a iIndex. */
    virtual QAccessibleInterface *child(int /* iIndex */) const /* override */ { return 0; }
    /** Returns the child at position QPoint(@a x, @a y). */
    virtual QAccessibleInterface *childAt(int /* x */, int /* y */) const /* override */ { return 0; }
    /** Returns the index of the passed @a pChild. */
    virtual int indexOfChild(const QAccessibleInterface * /* pChild */) const /* override */ { return -1; }

    /** Returns the rect. */
    virtual QRect rect() const /* override */;

    /** Defines a @a strText for the passed @a enmTextRole. */
    virtual void setText(QAccessible::Text /* enmTextRole */, const QString & /* strText */) /* override */ {}
    /** Returns a text for the passed @a enmTextRole. */
    virtual QString text(QAccessible::Text /* enmTextRole */) const /* override */;

    /** Returns the role. */
    virtual QAccessible::Role role() const /* override */ { return QAccessible::Button; }
    /** Returns the state. */
    virtual QAccessible::State state() const /* override */ { return QAccessible::State(); }

private:

    /** Holds the parent interface we are linked to. */
    UIAccessibilityInterfaceForUIMenuBarEditorButton *m_pParent;
    /** Holds the index of segment we are referring to. */
    const UIMenuBarEditorSegment m_enmIndex;
};


/** QAccessibleWidget extension used as an accessibility interface for UIMenuBarEditor buttons. */
class UIAccessibilityInterfaceForUIMenuBarEditorButton : public QAccessibleWidget
{
public:

    /** Returns an accessibility interface for passed @a strClassname and @a pObject. */
    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject);

    /** Constructs an accessibility interface passing @a pWidget to the base-class. */
    UIAccessibilityInterfaceForUIMenuBarEditorButton(QWidget *pWidget);
    /** Destructs an accessibility interface. */
    ~UIAccessibilityInterfaceForUIMenuBarEditorButton();

    /** Returns the number of children. */
    virtual int childCount() const /* override */;
    /** Returns the child with the passed @a iIndex. */
    virtual QAccessibleInterface *child(int iIndex) const /* override */;

    /** Returns the role. */
    virtual QAccessible::Role role() const /* override */;

    /** Returns the rect of sub-element @a enmSegment. */
    QRect subRect(UIMenuBarEditorSegment enmSegment) const;
    /** Returns the text of sub-element @a enmSegment. */
    QString subText(UIMenuBarEditorSegment enmSegment) const;

private:

    /** Returns corresponding toolbar button. */
    QToolButton *button() const { return qobject_cast<QToolButton*>(widget()); }

    /** Holds the map of instances of sub-element interfaces. */
    QMap<UIMenuBarEditorSegment, UIAccessibilityInterfaceForUIMenuBarEditorButtonSegment*> m_elements;
};


/*********************************************************************************************************************************
*   Class UIAccessibilityInterfaceForUIMenuBarEditorButtonSegment implementation.                                                *
*********************************************************************************************************************************/

UIAccessibilityInterfaceForUIMenuBarEditorButtonSegment::UIAccessibilityInterfaceForUIMenuBarEditorButtonSegment(
        UIAccessibilityInterfaceForUIMenuBarEditorButton *pParent,
        UIMenuBarEditorSegment enmIndex)
    : m_pParent(pParent)
    , m_enmIndex(enmIndex)
{
}

QAccessibleInterface *UIAccessibilityInterfaceForUIMenuBarEditorButtonSegment::parent() const
{
    return m_pParent;
}

QRect UIAccessibilityInterfaceForUIMenuBarEditorButtonSegment::rect() const
{
    return m_pParent->subRect(m_enmIndex);
}

QString UIAccessibilityInterfaceForUIMenuBarEditorButtonSegment::text(QAccessible::Text /* enmTextRole */) const
{
    return m_pParent->subText(m_enmIndex);
}


/*********************************************************************************************************************************
*   Class UIAccessibilityInterfaceForUIMenuBarEditorButton implementation.                                                       *
*********************************************************************************************************************************/

/* static */
QAccessibleInterface *UIAccessibilityInterfaceForUIMenuBarEditorButton::pFactory(const QString &strClassname, QObject *pObject)
{
    /* Creating toolbar button accessibility interface: */
    if (   pObject
        && strClassname == QLatin1String("QToolButton")
        && pObject->property("Belongs to") == "UIMenuBarEditorWidget")
        return new UIAccessibilityInterfaceForUIMenuBarEditorButton(qobject_cast<QWidget*>(pObject));

    /* Null by default: */
    return 0;
}

UIAccessibilityInterfaceForUIMenuBarEditorButton::UIAccessibilityInterfaceForUIMenuBarEditorButton(QWidget *pWidget)
    : QAccessibleWidget(pWidget, QAccessible::Button)
{
    /* Prepare button with popup menu: */
    if (button()->popupMode() == QToolButton::MenuButtonPopup)
    {
        for (int iIndex = 0; iIndex < (int)UIMenuBarEditorSegment_Max; ++iIndex)
            m_elements[(UIMenuBarEditorSegment)iIndex] =
                new UIAccessibilityInterfaceForUIMenuBarEditorButtonSegment(this, (UIMenuBarEditorSegment)iIndex);
    }
}

UIAccessibilityInterfaceForUIMenuBarEditorButton::~UIAccessibilityInterfaceForUIMenuBarEditorButton()
{
    /* Cleanup button with popup menu: */
    qDeleteAll(m_elements);
    m_elements.clear();
}

int UIAccessibilityInterfaceForUIMenuBarEditorButton::childCount() const
{
    /* Sanity check: */
    AssertPtrReturn(button(), 0);

    /* Return child count for a button with popup menu: */
    if (button()->popupMode() == QToolButton::MenuButtonPopup)
        return UIMenuBarEditorSegment_Max;

    /* Call to base-class: */
    return QAccessibleWidget::childCount();
}

QAccessibleInterface *UIAccessibilityInterfaceForUIMenuBarEditorButton::child(int iIndex) const
{
    /* Sanity check: */
    AssertPtrReturn(button(), 0);
    AssertReturn(iIndex >= 0 && iIndex < childCount(), 0);

    /* Return the child with the passed iIndex for a button with popup menu: */
    if (button()->popupMode() == QToolButton::MenuButtonPopup)
        return m_elements.value((UIMenuBarEditorSegment)iIndex);

    /* Call to base-class: */
    return QAccessibleWidget::child(iIndex);
}

QAccessible::Role UIAccessibilityInterfaceForUIMenuBarEditorButton::role() const
{
    /* Sanity check: */
    AssertPtrReturn(button(), QAccessibleWidget::role());

    /* Return role for button with popup menu: */
    if (button()->popupMode() == QToolButton::MenuButtonPopup)
        return QAccessible::ToolBar;

    /* Call to base-class: */
    return QAccessibleWidget::role();
}

QRect UIAccessibilityInterfaceForUIMenuBarEditorButton::subRect(UIMenuBarEditorSegment enmSegment) const
{
    /* Sanity check: */
    AssertReturn(button()->popupMode() == QToolButton::MenuButtonPopup, QRect());

    /* Return the rect of segment with the passed enmIndex for a button with popup menu: */
    switch (enmSegment)
    {
        case UIMenuBarEditorSegment_Button:
        {
            QStyleOptionToolButton options;
            options.initFrom(button());
            options.features |= QStyleOptionToolButton::MenuButtonPopup;
            QRect rect = button()->style()->subControlRect(QStyle::CC_ToolButton, &options, QStyle::SC_ToolButton);
            rect.moveTo(button()->mapToGlobal(rect.topLeft()));
            return rect;
        }
        case UIMenuBarEditorSegment_Menu:
        {
            QStyleOptionToolButton options;
            options.initFrom(button());
            options.features |= QStyleOptionToolButton::MenuButtonPopup;
            QRect rect = button()->style()->subControlRect(QStyle::CC_ToolButton, &options, QStyle::SC_ToolButtonMenu);
            rect.moveTo(button()->mapToGlobal(rect.topLeft()));
            return rect;
        }
        default:
            break;
    }

    /* Null rect by default: */
    return QRect();
}

QString UIAccessibilityInterfaceForUIMenuBarEditorButton::subText(UIMenuBarEditorSegment enmSegment) const
{
    /* Sanity check: */
    AssertReturn(button()->popupMode() == QToolButton::MenuButtonPopup, QString());

    /* Return the text of segment with the passed enmIndex for a button with popup menu: */
    switch (enmSegment)
    {
        case UIMenuBarEditorSegment_Button:
            return UIMenuBarEditorWidget::tr("Toggle menu %1").arg(QAccessibleWidget::text(QAccessible::Description));
        case UIMenuBarEditorSegment_Menu:
            return UIMenuBarEditorWidget::tr("Popup menu %1").arg(QAccessibleWidget::text(QAccessible::Description));
        default:
            break;
    }

    /* Null string by default: */
    return QString();
}


/*********************************************************************************************************************************
*   Class UIMenuBarEditorWidget implementation.                                                                                  *
*********************************************************************************************************************************/

UIMenuBarEditorWidget::UIMenuBarEditorWidget(QWidget *pParent,
                                             bool fStartedFromVMSettings /* = true */,
                                             const QString &strMachineID /* = QString() */,
                                             UIActionPool *pActionPool /* = 0 */)
    : QIWithRetranslateUI2<QWidget>(pParent)
    , m_fPrepared(false)
    , m_fStartedFromVMSettings(fStartedFromVMSettings)
    , m_strMachineID(strMachineID)
    , m_pActionPool(pActionPool)
    , m_pMainLayout(0)
    , m_pToolBar(0)
    , m_pButtonClose(0)
#ifndef VBOX_WS_MAC
    , m_pCheckBoxEnable(0)
#endif /* !VBOX_WS_MAC */
    , m_restrictionsOfMenuBar(UIExtraDataMetaDefs::MenuType_Invalid)
    , m_restrictionsOfMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType_Invalid)
    , m_restrictionsOfMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Invalid)
    , m_restrictionsOfMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Invalid)
    , m_restrictionsOfMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType_Invalid)
    , m_restrictionsOfMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Invalid)
#ifdef VBOX_WITH_DEBUGGER_GUI
    , m_restrictionsOfMenuDebug(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Invalid)
#endif
#ifdef VBOX_WS_MAC
    , m_restrictionsOfMenuWindow(UIExtraDataMetaDefs::MenuWindowActionType_Invalid)
#endif
    , m_restrictionsOfMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType_Invalid)

{
    /* Prepare: */
    prepare();
}

void UIMenuBarEditorWidget::setMachineID(const QString &strMachineID)
{
    /* Remember new machine ID: */
    m_strMachineID = strMachineID;
    /* Prepare: */
    prepare();
}

void UIMenuBarEditorWidget::setActionPool(UIActionPool *pActionPool)
{
    /* Remember new action-pool: */
    m_pActionPool = pActionPool;
    /* Prepare: */
    prepare();
}

#ifndef VBOX_WS_MAC
bool UIMenuBarEditorWidget::isMenuBarEnabled() const
{
    /* For VM settings only: */
    AssertReturn(m_fStartedFromVMSettings, false);

    /* Acquire enable-checkbox if possible: */
    AssertPtrReturn(m_pCheckBoxEnable, false);
    return m_pCheckBoxEnable->isChecked();
}

void UIMenuBarEditorWidget::setMenuBarEnabled(bool fEnabled)
{
    /* For VM settings only: */
    AssertReturnVoid(m_fStartedFromVMSettings);

    /* Update enable-checkbox if possible: */
    AssertPtrReturnVoid(m_pCheckBoxEnable);
    m_pCheckBoxEnable->setChecked(fEnabled);
}
#endif /* !VBOX_WS_MAC */

void UIMenuBarEditorWidget::setRestrictionsOfMenuBar(UIExtraDataMetaDefs::MenuType restrictions)
{
    /* Cache passed restrictions: */
    m_restrictionsOfMenuBar = restrictions;
    /* Get static meta-object: */
    const QMetaObject &smo = UIExtraDataMetaDefs::staticMetaObject;

    /* We have UIExtraDataMetaDefs::MenuType enum registered, so we can enumerate it: */
    const int iEnumIndex = smo.indexOfEnumerator("MenuType");
    const QMetaEnum metaEnum = smo.enumerator(iEnumIndex);
    /* Handle other enum-values: */
    for (int iKeyIndex = 0; iKeyIndex < metaEnum.keyCount(); ++iKeyIndex)
    {
        /* Get iterated enum-value: */
        const UIExtraDataMetaDefs::MenuType enumValue =
            static_cast<const UIExtraDataMetaDefs::MenuType>(metaEnum.keyToValue(metaEnum.key(iKeyIndex)));
        /* Skip MenuType_Invalid & MenuType_All enum-value: */
        if (enumValue == UIExtraDataMetaDefs::MenuType_Invalid ||
            enumValue == UIExtraDataMetaDefs::MenuType_All)
            continue;
        /* Which key required action registered under? */
        const QString strKey = gpConverter->toInternalString(enumValue);
        if (!m_actions.contains(strKey))
            continue;
        /* Update action 'checked' state: */
        m_actions.value(strKey)->setChecked(!(m_restrictionsOfMenuBar & enumValue));
    }
}

void UIMenuBarEditorWidget::setRestrictionsOfMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType restrictions)
{
    /* Cache passed restrictions: */
    m_restrictionsOfMenuApplication = restrictions;
    /* Get static meta-object: */
    const QMetaObject &smo = UIExtraDataMetaDefs::staticMetaObject;

    /* We have UIExtraDataMetaDefs::MenuApplicationActionType enum registered, so we can enumerate it: */
    const int iEnumIndex = smo.indexOfEnumerator("MenuApplicationActionType");
    QMetaEnum metaEnum = smo.enumerator(iEnumIndex);
    /* Handle other enum-values: */
    for (int iKeyIndex = 0; iKeyIndex < metaEnum.keyCount(); ++iKeyIndex)
    {
        /* Get iterated enum-value: */
        const UIExtraDataMetaDefs::MenuApplicationActionType enumValue =
            static_cast<const UIExtraDataMetaDefs::MenuApplicationActionType>(metaEnum.keyToValue(metaEnum.key(iKeyIndex)));
        /* Skip MenuApplicationActionType_Invalid & MenuApplicationActionType_All enum-value: */
        if (enumValue == UIExtraDataMetaDefs::MenuApplicationActionType_Invalid ||
            enumValue == UIExtraDataMetaDefs::MenuApplicationActionType_All)
            continue;
        /* Which key required action registered under? */
        const QString strKey = gpConverter->toInternalString(enumValue);
        if (!m_actions.contains(strKey))
            continue;
        /* Update action 'checked' state: */
        m_actions.value(strKey)->setChecked(!(m_restrictionsOfMenuApplication & enumValue));
    }
}

void UIMenuBarEditorWidget::setRestrictionsOfMenuMachine(UIExtraDataMetaDefs::RuntimeMenuMachineActionType restrictions)
{
    /* Cache passed restrictions: */
    m_restrictionsOfMenuMachine = restrictions;
    /* Get static meta-object: */
    const QMetaObject &smo = UIExtraDataMetaDefs::staticMetaObject;

    /* We have UIExtraDataMetaDefs::RuntimeMenuMachineActionType enum registered, so we can enumerate it: */
    const int iEnumIndex = smo.indexOfEnumerator("RuntimeMenuMachineActionType");
    QMetaEnum metaEnum = smo.enumerator(iEnumIndex);
    /* Handle other enum-values: */
    for (int iKeyIndex = 0; iKeyIndex < metaEnum.keyCount(); ++iKeyIndex)
    {
        /* Get iterated enum-value: */
        const UIExtraDataMetaDefs::RuntimeMenuMachineActionType enumValue =
            static_cast<const UIExtraDataMetaDefs::RuntimeMenuMachineActionType>(metaEnum.keyToValue(metaEnum.key(iKeyIndex)));
        /* Skip RuntimeMenuMachineActionType_Invalid & RuntimeMenuMachineActionType_All enum-value: */
        if (enumValue == UIExtraDataMetaDefs::RuntimeMenuMachineActionType_Invalid ||
            enumValue == UIExtraDataMetaDefs::RuntimeMenuMachineActionType_All)
            continue;
        /* Which key required action registered under? */
        const QString strKey = gpConverter->toInternalString(enumValue);
        if (!m_actions.contains(strKey))
            continue;
        /* Update action 'checked' state: */
        m_actions.value(strKey)->setChecked(!(m_restrictionsOfMenuMachine & enumValue));
    }
}

void UIMenuBarEditorWidget::setRestrictionsOfMenuView(UIExtraDataMetaDefs::RuntimeMenuViewActionType restrictions)
{
    /* Cache passed restrictions: */
    m_restrictionsOfMenuView = restrictions;
    /* Get static meta-object: */
    const QMetaObject &smo = UIExtraDataMetaDefs::staticMetaObject;

    /* We have UIExtraDataMetaDefs::RuntimeMenuViewActionType enum registered, so we can enumerate it: */
    const int iEnumIndex = smo.indexOfEnumerator("RuntimeMenuViewActionType");
    QMetaEnum metaEnum = smo.enumerator(iEnumIndex);
    /* Handle other enum-values: */
    for (int iKeyIndex = 0; iKeyIndex < metaEnum.keyCount(); ++iKeyIndex)
    {
        /* Get iterated enum-value: */
        const UIExtraDataMetaDefs::RuntimeMenuViewActionType enumValue =
            static_cast<const UIExtraDataMetaDefs::RuntimeMenuViewActionType>(metaEnum.keyToValue(metaEnum.key(iKeyIndex)));
        /* Skip RuntimeMenuViewActionType_Invalid & RuntimeMenuViewActionType_All enum-value: */
        if (enumValue == UIExtraDataMetaDefs::RuntimeMenuViewActionType_Invalid ||
            enumValue == UIExtraDataMetaDefs::RuntimeMenuViewActionType_All)
            continue;
        /* Which key required action registered under? */
        const QString strKey = gpConverter->toInternalString(enumValue);
        if (!m_actions.contains(strKey))
            continue;
        /* Update action 'checked' state: */
        m_actions.value(strKey)->setChecked(!(m_restrictionsOfMenuView & enumValue));
    }
}

void UIMenuBarEditorWidget::setRestrictionsOfMenuInput(UIExtraDataMetaDefs::RuntimeMenuInputActionType restrictions)
{
    /* Cache passed restrictions: */
    m_restrictionsOfMenuInput = restrictions;
    /* Get static meta-object: */
    const QMetaObject &smo = UIExtraDataMetaDefs::staticMetaObject;

    /* We have UIExtraDataMetaDefs::RuntimeMenuInputActionType enum registered, so we can enumerate it: */
    const int iEnumIndex = smo.indexOfEnumerator("RuntimeMenuInputActionType");
    QMetaEnum metaEnum = smo.enumerator(iEnumIndex);
    /* Handle other enum-values: */
    for (int iKeyIndex = 0; iKeyIndex < metaEnum.keyCount(); ++iKeyIndex)
    {
        /* Get iterated enum-value: */
        const UIExtraDataMetaDefs::RuntimeMenuInputActionType enumValue =
            static_cast<const UIExtraDataMetaDefs::RuntimeMenuInputActionType>(metaEnum.keyToValue(metaEnum.key(iKeyIndex)));
        /* Skip RuntimeMenuInputActionType_Invalid & RuntimeMenuInputActionType_All enum-value: */
        if (enumValue == UIExtraDataMetaDefs::RuntimeMenuInputActionType_Invalid ||
            enumValue == UIExtraDataMetaDefs::RuntimeMenuInputActionType_All)
            continue;
        /* Which key required action registered under? */
        const QString strKey = gpConverter->toInternalString(enumValue);
        if (!m_actions.contains(strKey))
            continue;
        /* Update action 'checked' state: */
        m_actions.value(strKey)->setChecked(!(m_restrictionsOfMenuInput & enumValue));
    }
}

void UIMenuBarEditorWidget::setRestrictionsOfMenuDevices(UIExtraDataMetaDefs::RuntimeMenuDevicesActionType restrictions)
{
    /* Cache passed restrictions: */
    m_restrictionsOfMenuDevices = restrictions;
    /* Get static meta-object: */
    const QMetaObject &smo = UIExtraDataMetaDefs::staticMetaObject;

    /* We have UIExtraDataMetaDefs::RuntimeMenuDevicesActionType enum registered, so we can enumerate it: */
    const int iEnumIndex = smo.indexOfEnumerator("RuntimeMenuDevicesActionType");
    QMetaEnum metaEnum = smo.enumerator(iEnumIndex);
    /* Handle other enum-values: */
    for (int iKeyIndex = 0; iKeyIndex < metaEnum.keyCount(); ++iKeyIndex)
    {
        /* Get iterated enum-value: */
        const UIExtraDataMetaDefs::RuntimeMenuDevicesActionType enumValue =
            static_cast<const UIExtraDataMetaDefs::RuntimeMenuDevicesActionType>(metaEnum.keyToValue(metaEnum.key(iKeyIndex)));
        /* Skip RuntimeMenuDevicesActionType_Invalid & RuntimeMenuDevicesActionType_All enum-value: */
        if (enumValue == UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_Invalid ||
            enumValue == UIExtraDataMetaDefs::RuntimeMenuDevicesActionType_All)
            continue;
        /* Which key required action registered under? */
        const QString strKey = gpConverter->toInternalString(enumValue);
        if (!m_actions.contains(strKey))
            continue;
        /* Update action 'checked' state: */
        m_actions.value(strKey)->setChecked(!(m_restrictionsOfMenuDevices & enumValue));
    }
}

#ifdef VBOX_WITH_DEBUGGER_GUI
void UIMenuBarEditorWidget::setRestrictionsOfMenuDebug(UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType restrictions)
{
    /* Cache passed restrictions: */
    m_restrictionsOfMenuDebug = restrictions;
    /* Get static meta-object: */
    const QMetaObject &smo = UIExtraDataMetaDefs::staticMetaObject;

    /* We have UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType enum registered, so we can enumerate it: */
    const int iEnumIndex = smo.indexOfEnumerator("RuntimeMenuDebuggerActionType");
    QMetaEnum metaEnum = smo.enumerator(iEnumIndex);
    /* Handle other enum-values: */
    for (int iKeyIndex = 0; iKeyIndex < metaEnum.keyCount(); ++iKeyIndex)
    {
        /* Get iterated enum-value: */
        const UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType enumValue =
            static_cast<const UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType>(metaEnum.keyToValue(metaEnum.key(iKeyIndex)));
        /* Skip RuntimeMenuDebuggerActionType_Invalid & RuntimeMenuDebuggerActionType_All enum-value: */
        if (enumValue == UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_Invalid ||
            enumValue == UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType_All)
            continue;
        /* Which key required action registered under? */
        const QString strKey = gpConverter->toInternalString(enumValue);
        if (!m_actions.contains(strKey))
            continue;
        /* Update action 'checked' state: */
        m_actions.value(strKey)->setChecked(!(m_restrictionsOfMenuDebug & enumValue));
    }
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
void UIMenuBarEditorWidget::setRestrictionsOfMenuWindow(UIExtraDataMetaDefs::MenuWindowActionType restrictions)
{
    /* Cache passed restrictions: */
    m_restrictionsOfMenuWindow = restrictions;
    /* Get static meta-object: */
    const QMetaObject &smo = UIExtraDataMetaDefs::staticMetaObject;

    /* We have UIExtraDataMetaDefs::MenuWindowActionType enum registered, so we can enumerate it: */
    const int iEnumIndex = smo.indexOfEnumerator("MenuWindowActionType");
    QMetaEnum metaEnum = smo.enumerator(iEnumIndex);
    /* Handle other enum-values: */
    for (int iKeyIndex = 0; iKeyIndex < metaEnum.keyCount(); ++iKeyIndex)
    {
        /* Get iterated enum-value: */
        const UIExtraDataMetaDefs::MenuWindowActionType enumValue =
            static_cast<const UIExtraDataMetaDefs::MenuWindowActionType>(metaEnum.keyToValue(metaEnum.key(iKeyIndex)));
        /* Skip MenuWindowActionType_Invalid & MenuWindowActionType_All enum-value: */
        if (enumValue == UIExtraDataMetaDefs::MenuWindowActionType_Invalid ||
            enumValue == UIExtraDataMetaDefs::MenuWindowActionType_All)
            continue;
        /* Which key required action registered under? */
        const QString strKey = gpConverter->toInternalString(enumValue);
        if (!m_actions.contains(strKey))
            continue;
        /* Update action 'checked' state: */
        m_actions.value(strKey)->setChecked(!(m_restrictionsOfMenuWindow & enumValue));
    }
}
#endif /* VBOX_WS_MAC */

void UIMenuBarEditorWidget::setRestrictionsOfMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType restrictions)
{
    /* Cache passed restrictions: */
    m_restrictionsOfMenuHelp = restrictions;
    /* Get static meta-object: */
    const QMetaObject &smo = UIExtraDataMetaDefs::staticMetaObject;

    /* We have UIExtraDataMetaDefs::MenuHelpActionType enum registered, so we can enumerate it: */
    const int iEnumIndex = smo.indexOfEnumerator("MenuHelpActionType");
    QMetaEnum metaEnum = smo.enumerator(iEnumIndex);
    /* Handle other enum-values: */
    for (int iKeyIndex = 0; iKeyIndex < metaEnum.keyCount(); ++iKeyIndex)
    {
        /* Get iterated enum-value: */
        const UIExtraDataMetaDefs::MenuHelpActionType enumValue =
            static_cast<const UIExtraDataMetaDefs::MenuHelpActionType>(metaEnum.keyToValue(metaEnum.key(iKeyIndex)));
        /* Skip MenuHelpActionType_Invalid & MenuHelpActionType_All enum-value: */
        if (enumValue == UIExtraDataMetaDefs::MenuHelpActionType_Invalid ||
            enumValue == UIExtraDataMetaDefs::MenuHelpActionType_All)
            continue;
        /* Which key required action registered under? */
        const QString strKey = gpConverter->toInternalString(enumValue);
        if (!m_actions.contains(strKey))
            continue;
        /* Update action 'checked' state: */
        m_actions.value(strKey)->setChecked(!(m_restrictionsOfMenuHelp & enumValue));
    }
}

void UIMenuBarEditorWidget::sltHandleConfigurationChange(const QString &strMachineID)
{
    /* Skip unrelated machine IDs: */
    if (machineID() != strMachineID)
        return;

    /* Recache menu-bar configuration: */
    setRestrictionsOfMenuBar(gEDataManager->restrictedRuntimeMenuTypes(machineID()));
    setRestrictionsOfMenuApplication(gEDataManager->restrictedRuntimeMenuApplicationActionTypes(machineID()));
    setRestrictionsOfMenuMachine(gEDataManager->restrictedRuntimeMenuMachineActionTypes(machineID()));
    setRestrictionsOfMenuView(gEDataManager->restrictedRuntimeMenuViewActionTypes(machineID()));
    setRestrictionsOfMenuInput(gEDataManager->restrictedRuntimeMenuInputActionTypes(machineID()));
    setRestrictionsOfMenuDevices(gEDataManager->restrictedRuntimeMenuDevicesActionTypes(machineID()));
#ifdef VBOX_WITH_DEBUGGER_GUI
    setRestrictionsOfMenuDebug(gEDataManager->restrictedRuntimeMenuDebuggerActionTypes(machineID()));
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    setRestrictionsOfMenuWindow(gEDataManager->restrictedRuntimeMenuWindowActionTypes(machineID()));
#endif /* VBOX_WS_MAC */
    setRestrictionsOfMenuHelp(gEDataManager->restrictedRuntimeMenuHelpActionTypes(machineID()));
}

void UIMenuBarEditorWidget::sltHandleMenuBarMenuClick()
{
    /* Make sure sender is valid: */
    QAction *pAction = qobject_cast<QAction*>(sender());
    AssertPtrReturnVoid(pAction);

    /* Depending on triggered action class: */
    switch (pAction->property("class").toInt())
    {
        case UIExtraDataMetaDefs::MenuType_All:
        {
            /* Get sender type: */
            const UIExtraDataMetaDefs::MenuType type =
                static_cast<UIExtraDataMetaDefs::MenuType>(pAction->property("type").toInt());
            /* Invert restriction for sender type: */
            m_restrictionsOfMenuBar = (UIExtraDataMetaDefs::MenuType)(m_restrictionsOfMenuBar ^ type);
            if (m_fStartedFromVMSettings)
            {
                /* Reapply menu-bar restrictions from cache: */
                setRestrictionsOfMenuBar(m_restrictionsOfMenuBar);
            }
            else
            {
                /* Save updated menu-bar restrictions: */
                gEDataManager->setRestrictedRuntimeMenuTypes(m_restrictionsOfMenuBar, machineID());
            }
            break;
        }
        case UIExtraDataMetaDefs::MenuType_Application:
        {
            /* Get sender type: */
            const UIExtraDataMetaDefs::MenuApplicationActionType type =
                static_cast<UIExtraDataMetaDefs::MenuApplicationActionType>(pAction->property("type").toInt());
            /* Invert restriction for sender type: */
            m_restrictionsOfMenuApplication = (UIExtraDataMetaDefs::MenuApplicationActionType)(m_restrictionsOfMenuApplication ^ type);
            if (m_fStartedFromVMSettings)
            {
                /* Reapply menu-bar restrictions from cache: */
                setRestrictionsOfMenuApplication(m_restrictionsOfMenuApplication);
            }
            else
            {
                /* Save updated menu-bar restrictions: */
                gEDataManager->setRestrictedRuntimeMenuApplicationActionTypes(m_restrictionsOfMenuApplication, machineID());
            }
            break;
        }
        case UIExtraDataMetaDefs::MenuType_Machine:
        {
            /* Get sender type: */
            const UIExtraDataMetaDefs::RuntimeMenuMachineActionType type =
                static_cast<UIExtraDataMetaDefs::RuntimeMenuMachineActionType>(pAction->property("type").toInt());
            /* Invert restriction for sender type: */
            m_restrictionsOfMenuMachine = (UIExtraDataMetaDefs::RuntimeMenuMachineActionType)(m_restrictionsOfMenuMachine ^ type);
            if (m_fStartedFromVMSettings)
            {
                /* Reapply menu-bar restrictions from cache: */
                setRestrictionsOfMenuMachine(m_restrictionsOfMenuMachine);
            }
            else
            {
                /* Save updated menu-bar restrictions: */
                gEDataManager->setRestrictedRuntimeMenuMachineActionTypes(m_restrictionsOfMenuMachine, machineID());
            }
            break;
        }
        case UIExtraDataMetaDefs::MenuType_View:
        {
            /* Get sender type: */
            const UIExtraDataMetaDefs::RuntimeMenuViewActionType type =
                static_cast<UIExtraDataMetaDefs::RuntimeMenuViewActionType>(pAction->property("type").toInt());
            /* Invert restriction for sender type: */
            m_restrictionsOfMenuView = (UIExtraDataMetaDefs::RuntimeMenuViewActionType)(m_restrictionsOfMenuView ^ type);
            if (m_fStartedFromVMSettings)
            {
                /* Reapply menu-bar restrictions from cache: */
                setRestrictionsOfMenuView(m_restrictionsOfMenuView);
            }
            else
            {
                /* Save updated menu-bar restrictions: */
                gEDataManager->setRestrictedRuntimeMenuViewActionTypes(m_restrictionsOfMenuView, machineID());
            }
            break;
        }
        case UIExtraDataMetaDefs::MenuType_Input:
        {
            /* Get sender type: */
            const UIExtraDataMetaDefs::RuntimeMenuInputActionType type =
                static_cast<UIExtraDataMetaDefs::RuntimeMenuInputActionType>(pAction->property("type").toInt());
            /* Invert restriction for sender type: */
            m_restrictionsOfMenuInput = (UIExtraDataMetaDefs::RuntimeMenuInputActionType)(m_restrictionsOfMenuInput ^ type);
            if (m_fStartedFromVMSettings)
            {
                /* Reapply menu-bar restrictions from cache: */
                setRestrictionsOfMenuInput(m_restrictionsOfMenuInput);
            }
            else
            {
                /* Save updated menu-bar restrictions: */
                gEDataManager->setRestrictedRuntimeMenuInputActionTypes(m_restrictionsOfMenuInput, machineID());
            }
            break;
        }
        case UIExtraDataMetaDefs::MenuType_Devices:
        {
            /* Get sender type: */
            const UIExtraDataMetaDefs::RuntimeMenuDevicesActionType type =
                static_cast<UIExtraDataMetaDefs::RuntimeMenuDevicesActionType>(pAction->property("type").toInt());
            /* Invert restriction for sender type: */
            m_restrictionsOfMenuDevices = (UIExtraDataMetaDefs::RuntimeMenuDevicesActionType)(m_restrictionsOfMenuDevices ^ type);
            if (m_fStartedFromVMSettings)
            {
                /* Reapply menu-bar restrictions from cache: */
                setRestrictionsOfMenuDevices(m_restrictionsOfMenuDevices);
            }
            else
            {
                /* Save updated menu-bar restrictions: */
                gEDataManager->setRestrictedRuntimeMenuDevicesActionTypes(m_restrictionsOfMenuDevices, machineID());
            }
            break;
        }
#ifdef VBOX_WITH_DEBUGGER_GUI
        case UIExtraDataMetaDefs::MenuType_Debug:
        {
            /* Get sender type: */
            const UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType type =
                static_cast<UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType>(pAction->property("type").toInt());
            /* Invert restriction for sender type: */
            m_restrictionsOfMenuDebug = (UIExtraDataMetaDefs::RuntimeMenuDebuggerActionType)(m_restrictionsOfMenuDebug ^ type);
            if (m_fStartedFromVMSettings)
            {
                /* Reapply menu-bar restrictions from cache: */
                setRestrictionsOfMenuDebug(m_restrictionsOfMenuDebug);
            }
            else
            {
                /* Save updated menu-bar restrictions: */
                gEDataManager->setRestrictedRuntimeMenuDebuggerActionTypes(m_restrictionsOfMenuDebug, machineID());
            }
            break;
        }
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
        case UIExtraDataMetaDefs::MenuType_Window:
        {
            /* Get sender type: */
            const UIExtraDataMetaDefs::MenuWindowActionType type =
                static_cast<UIExtraDataMetaDefs::MenuWindowActionType>(pAction->property("type").toInt());
            /* Invert restriction for sender type: */
            m_restrictionsOfMenuWindow = (UIExtraDataMetaDefs::MenuWindowActionType)(m_restrictionsOfMenuWindow ^ type);
            if (m_fStartedFromVMSettings)
            {
                /* Reapply menu-bar restrictions from cache: */
                setRestrictionsOfMenuWindow(m_restrictionsOfMenuWindow);
            }
            else
            {
                /* Save updated menu-bar restrictions: */
                gEDataManager->setRestrictedRuntimeMenuWindowActionTypes(m_restrictionsOfMenuWindow, machineID());
            }
            break;
        }
#endif /* VBOX_WS_MAC */
        case UIExtraDataMetaDefs::MenuType_Help:
        {
            /* Get sender type: */
            const UIExtraDataMetaDefs::MenuHelpActionType type =
                static_cast<UIExtraDataMetaDefs::MenuHelpActionType>(pAction->property("type").toInt());
            /* Invert restriction for sender type: */
            m_restrictionsOfMenuHelp = (UIExtraDataMetaDefs::MenuHelpActionType)(m_restrictionsOfMenuHelp ^ type);
            if (m_fStartedFromVMSettings)
            {
                /* Reapply menu-bar restrictions from cache: */
                setRestrictionsOfMenuHelp(m_restrictionsOfMenuHelp);
            }
            else
            {
                /* Save updated menu-bar restrictions: */
                gEDataManager->setRestrictedRuntimeMenuHelpActionTypes(m_restrictionsOfMenuHelp, machineID());
            }            break;
        }
        default: break;
    }
}

void UIMenuBarEditorWidget::prepare()
{
    /* Do nothing if already prepared: */
    if (m_fPrepared)
        return;

    /* Do not prepare if machine ID or action-pool is not set: */
    if (m_strMachineID.isEmpty() || !m_pActionPool)
        return;

    /* Install tool-bar button accessibility interface factory: */
    QAccessible::installFactory(UIAccessibilityInterfaceForUIMenuBarEditorButton::pFactory);

    /* Create main-layout: */
    m_pMainLayout = new QHBoxLayout(this);
    AssertPtrReturnVoid(m_pMainLayout);
    {
        /* Configure main-layout: */
        int iLeft, iTop, iRight, iBottom;
        m_pMainLayout->getContentsMargins(&iLeft, &iTop, &iRight, &iBottom);
        /* Acquire metric: */
        const int iStandardMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 2;
        const int iMinimumMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 4;
        /* Standard margins should not be too small/large: */
        iLeft   = iStandardMetric;
        iTop    = iStandardMetric;
        iRight  = iStandardMetric;
        iBottom = iStandardMetric;
        /* Top margin should be smaller for the common case: */
        if (iTop >= iMinimumMetric)
            iTop -= iMinimumMetric;
#ifndef VBOX_WS_MAC
        /* Right margin should be bigger for the settings case: */
        if (m_fStartedFromVMSettings)
            iRight += iMinimumMetric;
#endif /* !VBOX_WS_MAC */
        /* Apply margins/spacing finally: */
        m_pMainLayout->setContentsMargins(iLeft, iTop, iRight, iBottom);
        m_pMainLayout->setSpacing(0);
        /* Create tool-bar: */
        m_pToolBar = new UIToolBar;
        AssertPtrReturnVoid(m_pToolBar);
        {
            /* Prepare menus: */
            prepareMenus();
            /* Add tool-bar into main-layout: */
            m_pMainLayout->addWidget(m_pToolBar);
        }
        /* Insert stretch: */
        m_pMainLayout->addStretch();
        /* Create close-button if necessary: */
        if (!m_fStartedFromVMSettings)
        {
            m_pButtonClose = new QIToolButton;
            AssertPtrReturnVoid(m_pButtonClose);
            {
                /* Configure close-button: */
                m_pButtonClose->setFocusPolicy(Qt::StrongFocus);
                m_pButtonClose->setShortcut(Qt::Key_Escape);
                m_pButtonClose->setIcon(UIIconPool::iconSet(":/ok_16px.png"));
                connect(m_pButtonClose, SIGNAL(clicked(bool)), this, SIGNAL(sigCancelClicked()));
                /* Add close-button into main-layout: */
                m_pMainLayout->addWidget(m_pButtonClose);
            }
        }
#ifndef VBOX_WS_MAC
        /* Create enable-checkbox if necessary: */
        else
        {
            m_pCheckBoxEnable = new QCheckBox;
            AssertPtrReturnVoid(m_pCheckBoxEnable);
            {
                /* Configure enable-checkbox: */
                m_pCheckBoxEnable->setFocusPolicy(Qt::StrongFocus);
                /* Add enable-checkbox into main-layout: */
                m_pMainLayout->addWidget(m_pCheckBoxEnable);
            }
        }
#endif /* !VBOX_WS_MAC */
    }

    /* Mark as prepared: */
    m_fPrepared = true;

    /* Translate contents: */
    retranslateUi();
}

void UIMenuBarEditorWidget::prepareMenus()
{
    /* Create menus: */
    prepareMenuApplication();
    prepareMenuMachine();
    prepareMenuView();
    prepareMenuInput();
    prepareMenuDevices();
#ifdef VBOX_WITH_DEBUGGER_GUI
    prepareMenuDebug();
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
    prepareMenuWindow();
#endif /* VBOX_WS_MAC */
    prepareMenuHelp();

    if (!m_fStartedFromVMSettings)
    {
        /* Cache menu-bar configuration: */
        setRestrictionsOfMenuBar(gEDataManager->restrictedRuntimeMenuTypes(machineID()));
        setRestrictionsOfMenuApplication(gEDataManager->restrictedRuntimeMenuApplicationActionTypes(machineID()));
        setRestrictionsOfMenuMachine(gEDataManager->restrictedRuntimeMenuMachineActionTypes(machineID()));
        setRestrictionsOfMenuView(gEDataManager->restrictedRuntimeMenuViewActionTypes(machineID()));
        setRestrictionsOfMenuInput(gEDataManager->restrictedRuntimeMenuInputActionTypes(machineID()));
        setRestrictionsOfMenuDevices(gEDataManager->restrictedRuntimeMenuDevicesActionTypes(machineID()));
#ifdef VBOX_WITH_DEBUGGER_GUI
        setRestrictionsOfMenuDebug(gEDataManager->restrictedRuntimeMenuDebuggerActionTypes(machineID()));
#endif /* VBOX_WITH_DEBUGGER_GUI */
#ifdef VBOX_WS_MAC
        setRestrictionsOfMenuWindow(gEDataManager->restrictedRuntimeMenuWindowActionTypes(machineID()));
#endif /* VBOX_WS_MAC */
        setRestrictionsOfMenuHelp(gEDataManager->restrictedRuntimeMenuHelpActionTypes(machineID()));
        /* And listen for the menu-bar configuration changes after that: */
        connect(gEDataManager, SIGNAL(sigMenuBarConfigurationChange(const QString&)),
                this, SLOT(sltHandleConfigurationChange(const QString&)));
    }
}

#ifdef VBOX_WS_MAC
QMenu* UIMenuBarEditorWidget::prepareNamedMenu(const QString &strName)
{
    /* Create named menu: */
    QMenu *pNamedMenu = new QMenu(strName, m_pToolBar);
    AssertPtrReturn(pNamedMenu, 0);
    {
        /* Configure named menu: */
        pNamedMenu->setProperty("class", UIExtraDataMetaDefs::MenuType_Application);
        /* Get named menu action: */
        QAction *pNamedMenuAction = pNamedMenu->menuAction();
        AssertPtrReturn(pNamedMenuAction, 0);
        {
            /* Add menu action into tool-bar: */
            m_pToolBar->addAction(pNamedMenuAction);
            /* Get named menu tool-button: */
            QToolButton *pNamedMenuToolButton = qobject_cast<QToolButton*>(m_pToolBar->widgetForAction(pNamedMenuAction));
            AssertPtrReturn(pNamedMenuToolButton, 0);
            {
                /* Configure named menu tool-button: */
                pNamedMenuToolButton->setProperty("Belongs to", "UIMenuBarEditorWidget");
                pNamedMenuToolButton->setPopupMode(QToolButton::MenuButtonPopup);
                pNamedMenuToolButton->setAutoRaise(true);
                /* Update the accessibility interface to take "Belongs to" into account: */
                QAccessibleInterface *pInterface = QAccessible::queryAccessibleInterface(pNamedMenuToolButton);
                if (pInterface)
                {
                    QAccessible::deleteAccessibleInterface(QAccessible::uniqueId(pInterface));
                    QAccessible::queryAccessibleInterface(pNamedMenuToolButton); // <= new one, proper..
                }
                /* Create spacing after named menu tool-button: */
                QWidget *pSpacing = new QWidget;
                AssertPtrReturn(pSpacing, 0);
                {
                    /* Configure spacing: */
                    pSpacing->setFixedSize(5, 1);
                    /* Add spacing into tool-bar: */
                    m_pToolBar->addWidget(pSpacing);
                }
            }
        }
    }
    /* Return named menu: */
    return pNamedMenu;
}
#endif /* VBOX_WS_MAC */

QMenu* UIMenuBarEditorWidget::prepareCopiedMenu(const UIAction *pAction)
{
    /* Create copied menu: */
    QMenu *pCopiedMenu = new QMenu(pAction->name(), m_pToolBar);
    AssertPtrReturn(pCopiedMenu, 0);
    {
        /* Configure copied menu: */
        pCopiedMenu->setProperty("class", pAction->extraDataID());
        /* Get copied menu action: */
        QAction *pCopiedMenuAction = pCopiedMenu->menuAction();
        AssertPtrReturn(pCopiedMenuAction, 0);
        {
            /* Configure copied menu action: */
            pCopiedMenuAction->setCheckable(true);
            pCopiedMenuAction->setProperty("class", UIExtraDataMetaDefs::MenuType_All);
            pCopiedMenuAction->setProperty("type", pAction->extraDataID());
            connect(pCopiedMenuAction, SIGNAL(triggered(bool)), this, SLOT(sltHandleMenuBarMenuClick()));
            m_actions.insert(pAction->extraDataKey(), pCopiedMenuAction);
            /* Add menu action into tool-bar: */
            m_pToolBar->addAction(pCopiedMenuAction);
            /* Get copied menu tool-button: */
            QToolButton *pCopiedMenuToolButton = qobject_cast<QToolButton*>(m_pToolBar->widgetForAction(pCopiedMenuAction));
            AssertPtrReturn(pCopiedMenuToolButton, 0);
            {
                /* Configure copied menu tool-button: */
                pCopiedMenuToolButton->setProperty("Belongs to", "UIMenuBarEditorWidget");
                pCopiedMenuToolButton->setPopupMode(QToolButton::MenuButtonPopup);
                pCopiedMenuToolButton->setAutoRaise(true);
                /* Update the accessibility interface to take "Belongs to" into account: */
                QAccessibleInterface *pInterface = QAccessible::queryAccessibleInterface(pCopiedMenuToolButton);
                if (pInterface)
                {
                    QAccessible::deleteAccessibleInterface(QAccessible::uniqueId(pInterface));
                    QAccessible::queryAccessibleInterface(pCopiedMenuToolButton); // <= new one, proper..
                }
                /* Create spacing after copied menu tool-button: */
                QWidget *pSpacing = new QWidget;
                AssertPtrReturn(pSpacing, 0);
                {
                    /* Configure spacing: */
                    pSpacing->setFixedSize(5, 1);
                    /* Add spacing into tool-bar: */
                    m_pToolBar->addWidget(pSpacing);
                }
            }
        }
    }
    /* Return copied menu: */
    return pCopiedMenu;
}

#if 0
QMenu* UIMenuBarEditorWidget::prepareCopiedSubMenu(QMenu *pMenu, const UIAction *pAction)
{
    /* Create copied sub-menu: */
    QMenu *pCopiedMenu = pMenu->addMenu(pAction->name());
    AssertPtrReturn(pCopiedMenu, 0);
    {
        /* Configure copied sub-menu: */
        pCopiedMenu->setProperty("class", pMenu->property("class"));
        /* Get copied sub-menu action: */
        QAction *pCopiedMenuAction = pCopiedMenu->menuAction();
        AssertPtrReturn(pCopiedMenuAction, 0);
        {
            /* Configure copied sub-menu: */
            pCopiedMenuAction->setCheckable(true);
            pCopiedMenuAction->setProperty("class", pCopiedMenu->property("class"));
            pCopiedMenuAction->setProperty("type", pAction->extraDataID());
            connect(pCopiedMenuAction, SIGNAL(triggered(bool)), this, SLOT(sltHandleMenuBarMenuClick()));
            m_actions.insert(pAction->extraDataKey(), pCopiedMenuAction);
        }
    }
    /* Return copied sub-menu: */
    return pCopiedMenu;
}
#endif

QAction* UIMenuBarEditorWidget::prepareNamedAction(QMenu *pMenu, const QString &strName,
                                                   int iExtraDataID, const QString &strExtraDataID)
{
    /* Create copied action: */
    QAction *pCopiedAction = pMenu->addAction(strName);
    AssertPtrReturn(pCopiedAction, 0);
    {
        /* Configure copied action: */
        pCopiedAction->setCheckable(true);
        pCopiedAction->setProperty("class", pMenu->property("class"));
        pCopiedAction->setProperty("type", iExtraDataID);
        connect(pCopiedAction, SIGNAL(triggered(bool)), this, SLOT(sltHandleMenuBarMenuClick()));
        m_actions.insert(strExtraDataID, pCopiedAction);
    }
    /* Return copied action: */
    return pCopiedAction;
}

QAction* UIMenuBarEditorWidget::prepareCopiedAction(QMenu *pMenu, const UIAction *pAction)
{
    /* Create copied action: */
    QAction *pCopiedAction = pMenu->addAction(pAction->name());
    AssertPtrReturn(pCopiedAction, 0);
    {
        /* Configure copied action: */
        pCopiedAction->setCheckable(true);
        pCopiedAction->setProperty("class", pMenu->property("class"));
        pCopiedAction->setProperty("type", pAction->extraDataID());
        connect(pCopiedAction, SIGNAL(triggered(bool)), this, SLOT(sltHandleMenuBarMenuClick()));
        m_actions.insert(pAction->extraDataKey(), pCopiedAction);
    }
    /* Return copied action: */
    return pCopiedAction;
}

void UIMenuBarEditorWidget::prepareMenuApplication()
{
    /* Copy menu: */
#ifdef VBOX_WS_MAC
    QMenu *pMenu = prepareNamedMenu("Application");
#else /* !VBOX_WS_MAC */
    QMenu *pMenu = prepareCopiedMenu(actionPool()->action(UIActionIndex_M_Application));
#endif /* !VBOX_WS_MAC */
    AssertPtrReturnVoid(pMenu);
    {
#ifdef VBOX_WS_MAC
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_M_Application_S_About));
# ifdef VBOX_GUI_WITH_NETWORK_MANAGER
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_M_Application_S_NetworkAccessManager));
# endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_M_Application_S_ResetWarnings));
        pMenu->addSeparator();
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_M_Application_S_Preferences));
#else /* !VBOX_WS_MAC */
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_M_Application_S_Preferences));
        pMenu->addSeparator();
# ifdef VBOX_GUI_WITH_NETWORK_MANAGER
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_M_Application_S_NetworkAccessManager));
# endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_M_Application_S_ResetWarnings));
#endif /* !VBOX_WS_MAC */
    }
}

void UIMenuBarEditorWidget::prepareMenuMachine()
{
    /* Copy menu: */
    QMenu *pMenu = prepareCopiedMenu(actionPool()->action(UIActionIndexRT_M_Machine));
    AssertPtrReturnVoid(pMenu);
    {
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Machine_S_Settings));
        pMenu->addSeparator();
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Machine_S_TakeSnapshot));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Machine_S_ShowInformation));
        pMenu->addSeparator();
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Machine_T_Pause));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Machine_S_Reset));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Machine_S_Detach));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Machine_S_SaveState));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Machine_S_Shutdown));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Machine_S_PowerOff));
    }
}

void UIMenuBarEditorWidget::prepareMenuView()
{
    /* Copy menu: */
    QMenu *pMenu = prepareCopiedMenu(actionPool()->action(UIActionIndexRT_M_View));
    AssertPtrReturnVoid(pMenu);
    {
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_View_T_Fullscreen));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_View_T_Seamless));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_View_T_Scale));
        pMenu->addSeparator();
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_View_S_AdjustWindow));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_View_T_GuestAutoresize));
        pMenu->addSeparator();
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_View_S_TakeScreenshot));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_View_M_VideoCapture_T_Start));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_View_T_VRDEServer));
        pMenu->addSeparator();
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_View_M_MenuBar));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_View_M_StatusBar));
        pMenu->addSeparator();
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_View_M_ScaleFactor));
        prepareNamedAction(pMenu, tr("Virtual Screen Resize"),
                           UIExtraDataMetaDefs::RuntimeMenuViewActionType_Resize,
                           gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Resize));
        prepareNamedAction(pMenu, tr("Virtual Screen Mapping"),
                           UIExtraDataMetaDefs::RuntimeMenuViewActionType_Multiscreen,
                           gpConverter->toInternalString(UIExtraDataMetaDefs::RuntimeMenuViewActionType_Multiscreen));
    }
}

void UIMenuBarEditorWidget::prepareMenuInput()
{
    /* Copy menu: */
    QMenu *pMenu = prepareCopiedMenu(actionPool()->action(UIActionIndexRT_M_Input));
    AssertPtrReturnVoid(pMenu);
    {
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Input_M_Keyboard));
        pMenu->addSeparator();
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Input_M_Mouse_T_Integration));
    }
}

void UIMenuBarEditorWidget::prepareMenuDevices()
{
    /* Copy menu: */
    QMenu *pMenu = prepareCopiedMenu(actionPool()->action(UIActionIndexRT_M_Devices));
    AssertPtrReturnVoid(pMenu);
    {
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Devices_M_HardDrives));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Devices_M_OpticalDevices));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Devices_M_FloppyDevices));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Devices_M_Audio));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Devices_M_Network));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Devices_M_USBDevices));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Devices_M_WebCams));
        pMenu->addSeparator();
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Devices_M_SharedFolders));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Devices_M_SharedClipboard));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Devices_M_DragAndDrop));
        pMenu->addSeparator();
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Devices_S_InstallGuestTools));
    }
}

#ifdef VBOX_WITH_DEBUGGER_GUI
void UIMenuBarEditorWidget::prepareMenuDebug()
{
    /* Copy menu: */
    QMenu *pMenu = prepareCopiedMenu(actionPool()->action(UIActionIndexRT_M_Debug));
    AssertPtrReturnVoid(pMenu);
    {
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Debug_S_ShowStatistics));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Debug_S_ShowCommandLine));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Debug_T_Logging));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndexRT_M_Debug_S_ShowLogDialog));
    }
}
#endif /* VBOX_WITH_DEBUGGER_GUI */

#ifdef VBOX_WS_MAC
void UIMenuBarEditorWidget::prepareMenuWindow()
{
    /* Copy menu: */
    QMenu *pMenu = prepareCopiedMenu(actionPool()->action(UIActionIndex_M_Window));
    AssertPtrReturnVoid(pMenu);
    {
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_M_Window_S_Minimize));
        pMenu->addSeparator();
        prepareNamedAction(pMenu, tr("Switch"),
                           UIExtraDataMetaDefs::MenuWindowActionType_Switch,
                           gpConverter->toInternalString(UIExtraDataMetaDefs::MenuWindowActionType_Switch));
    }
}
#endif /* VBOX_WS_MAC */

void UIMenuBarEditorWidget::prepareMenuHelp()
{
    /* Copy menu: */
    QMenu *pMenu = prepareCopiedMenu(actionPool()->action(UIActionIndex_Menu_Help));
    AssertPtrReturnVoid(pMenu);
    {
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_Simple_Contents));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_Simple_WebSite));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_Simple_BugTracker));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_Simple_Forums));
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_Simple_Oracle));
        pMenu->addSeparator();
#ifndef VBOX_WS_MAC
        prepareCopiedAction(pMenu, actionPool()->action(UIActionIndex_Simple_About));
#endif /* !VBOX_WS_MAC */
    }
}

void UIMenuBarEditorWidget::retranslateUi()
{
    /* Translate close-button if necessary: */
    if (!m_fStartedFromVMSettings && m_pButtonClose)
        m_pButtonClose->setToolTip(tr("Close"));
#ifndef VBOX_WS_MAC
    /* Translate enable-checkbox if necessary: */
    if (m_fStartedFromVMSettings && m_pCheckBoxEnable)
        m_pCheckBoxEnable->setToolTip(tr("Enable Menu Bar"));
#endif /* !VBOX_WS_MAC */
}

void UIMenuBarEditorWidget::paintEvent(QPaintEvent*)
{
    /* Prepare painter: */
    QPainter painter(this);

    /* Prepare palette colors: */
    const QPalette pal = palette();
    QColor color0 = pal.color(QPalette::Window);
    QColor color1 = pal.color(QPalette::Window).lighter(110);
    color1.setAlpha(0);
    QColor color2 = pal.color(QPalette::Window).darker(200);
#if defined(VBOX_WS_WIN) || defined(VBOX_WS_X11)
    QColor color3 = pal.color(QPalette::Window).darker(120);
#endif /* VBOX_WS_WIN || VBOX_WS_X11 */

    /* Acquire metric: */
    const int iMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 4;

    /* Left corner: */
    QRadialGradient grad1(QPointF(iMetric, height() - iMetric), iMetric);
    {
        grad1.setColorAt(0, color2);
        grad1.setColorAt(1, color1);
    }
    /* Right corner: */
    QRadialGradient grad2(QPointF(width() - iMetric, height() - iMetric), iMetric);
    {
        grad2.setColorAt(0, color2);
        grad2.setColorAt(1, color1);
    }
    /* Bottom line: */
    QLinearGradient grad3(QPointF(iMetric, height()), QPointF(iMetric, height() - iMetric));
    {
        grad3.setColorAt(0, color1);
        grad3.setColorAt(1, color2);
    }
    /* Left line: */
    QLinearGradient grad4(QPointF(0, height() - iMetric), QPointF(iMetric, height() - iMetric));
    {
        grad4.setColorAt(0, color1);
        grad4.setColorAt(1, color2);
    }
    /* Right line: */
    QLinearGradient grad5(QPointF(width(), height() - iMetric), QPointF(width() - iMetric, height() - iMetric));
    {
        grad5.setColorAt(0, color1);
        grad5.setColorAt(1, color2);
    }

    /* Paint shape/shadow: */
    painter.fillRect(QRect(iMetric, 0, width() - iMetric * 2, height() - iMetric), color0); // background
    painter.fillRect(QRect(0,                 height() - iMetric, iMetric, iMetric), grad1); // left corner
    painter.fillRect(QRect(width() - iMetric, height() - iMetric, iMetric, iMetric), grad2); // right corner
    painter.fillRect(QRect(iMetric,           height() - iMetric, width() - iMetric * 2, iMetric), grad3); // bottom line
    painter.fillRect(QRect(0,                 0, iMetric, height() - iMetric), grad4); // left line
    painter.fillRect(QRect(width() - iMetric, 0, iMetric, height() - iMetric), grad5); // right line

#if defined(VBOX_WS_WIN) || defined(VBOX_WS_X11)
    /* Paint frames: */
    painter.save();
    painter.setPen(color3);
    painter.drawLine(QLine(QPoint(iMetric + 1,               0),
                           QPoint(iMetric + 1,               height() - 1 - iMetric - 1)));
    painter.drawLine(QLine(QPoint(iMetric + 1,               height() - 1 - iMetric - 1),
                           QPoint(width() - 1 - iMetric - 1, height() - 1 - iMetric - 1)));
    painter.drawLine(QLine(QPoint(width() - 1 - iMetric - 1, height() - 1 - iMetric - 1),
                           QPoint(width() - 1 - iMetric - 1, 0)));
    if (m_fStartedFromVMSettings)
        painter.drawLine(QLine(QPoint(width() - 1 - iMetric - 1, 0), QPoint(iMetric + 1, 0)));
    painter.restore();
#endif /* VBOX_WS_WIN || VBOX_WS_X11 */
}


UIMenuBarEditorWindow::UIMenuBarEditorWindow(UIMachineWindow *pParent, UIActionPool *pActionPool)
#ifndef VBOX_WS_MAC
    : UISlidingToolBar(pParent, pParent->menuBar(), new UIMenuBarEditorWidget(0, false, vboxGlobal().managedVMUuid(), pActionPool), UISlidingToolBar::Position_Top)
#else /* VBOX_WS_MAC */
    : UISlidingToolBar(pParent, 0, new UIMenuBarEditorWidget(0, false, vboxGlobal().managedVMUuid(), pActionPool), UISlidingToolBar::Position_Top)
#endif /* VBOX_WS_MAC */
{
}

