/* $Id: UIModalWindowManager.cpp $ */
/** @file
 * VBox Qt GUI - UIModalWindowManager class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* GUI includes: */
# include "UIModalWindowManager.h"
# ifdef VBOX_GUI_WITH_NETWORK_MANAGER
#  include "UINetworkManagerDialog.h"
#  include "UINetworkManager.h"
# endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
# include "UISelectorWindow.h"
# include "UIProgressDialog.h"
# include "VBoxGlobal.h"

/* Other VBox includes: */
# include <VBox/sup.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/* static */
UIModalWindowManager* UIModalWindowManager::m_spInstance = 0;
UIModalWindowManager* UIModalWindowManager::instance() { return m_spInstance; }

/* static */
void UIModalWindowManager::create()
{
    /* Make sure instance is NOT created yet: */
    if (m_spInstance)
    {
        AssertMsgFailed(("UIModalWindowManager instance is already created!"));
        return;
    }

    /* Create instance: */
    new UIModalWindowManager;
}

/* static */
void UIModalWindowManager::destroy()
{
    /* Make sure instance is NOT destroyed yet: */
    if (!m_spInstance)
    {
        AssertMsgFailed(("UIModalWindowManager instance is already destroyed!"));
        return;
    }

    /* Destroy instance: */
    delete m_spInstance;
}

UIModalWindowManager::UIModalWindowManager()
{
    /* Assign instance: */
    m_spInstance = this;
}

UIModalWindowManager::~UIModalWindowManager()
{
    /* Unassign instance: */
    m_spInstance = 0;
}

QWidget* UIModalWindowManager::mainWindowShown() const
{
    /* It may happen that this method is called before VBoxGlobal initialization
     * or after initialization had failed (for example, to show some message).
     * Return NULL pointer in such cases: */
    if (!VBoxGlobal::instance() || !vboxGlobal().isValid())
        return 0;

    /* For VM console process: */
    if (vboxGlobal().isVMConsoleProcess())
    {
        /* It will be currently active machine-window if visible: */
        if (vboxGlobal().activeMachineWindow() &&
            vboxGlobal().activeMachineWindow()->isVisible())
            return vboxGlobal().activeMachineWindow();
    }
    /* For VM selector process: */
    else
    {
        /* It will be the selector window if visible: */
        if (gpSelectorWindow &&
            gpSelectorWindow->isVisible())
            return gpSelectorWindow;
    }

    /* NULL by default: */
    return 0;
}

#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
QWidget* UIModalWindowManager::networkManagerOrMainWindowShown() const
{
    /* It may happen that this method is called before network-manager initialization
     * or when the network-manager is hidden, return main application window in this case: */
    return gNetworkManager && gNetworkManager->window()->isVisible() ? gNetworkManager->window() : mainWindowShown();
}
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */

QWidget* UIModalWindowManager::realParentWindow(QWidget *pWidget)
{
    /* Null if widget pointer is null: */
    if (!pWidget)
        return 0;

    /* Get the top-level window for the passed-widget: */
    QWidget *pTopLevelWindow = pWidget->window();

    /* Search through all the stack(s) we have: */
    foreach (const QList<QWidget*> &iteratedWindowStack, m_windows)
    {
        /* Search through all the window(s) iterated-stack contains: */
        foreach (QWidget *pIteratedWindow, iteratedWindowStack)
        {
            /* If possible-parent-window found: */
            if (pIteratedWindow == pTopLevelWindow)
            {
                /* Return the 'top' of the iterated-window-stack as the result: */
                QWidget *pTopWindow = iteratedWindowStack.last();
                preprocessRealParent(pTopWindow);
                return pTopWindow;
            }
        }
    }

    /* If we unable to found the possible-parent-window among all ours,
     * we have to add it as the new-window-stack only element: */
    registerNewParent(pTopLevelWindow);
    /* And return as the result: */
    return pTopLevelWindow;
}

bool UIModalWindowManager::isWindowInTheModalWindowStack(QWidget *pWindow)
{
    return contains(pWindow);
}

bool UIModalWindowManager::isWindowOnTheTopOfTheModalWindowStack(QWidget *pWindow)
{
    return contains(pWindow, true);
}

void UIModalWindowManager::registerNewParent(QWidget *pWindow, QWidget *pParentWindow /* = 0*/)
{
    /* Make sure passed-widget-pointer is not null: */
    if (!pWindow)
    {
        AssertMsgFailed(("Passed pointer is NULL!"));
        return;
    }

    /* Make sure passed-widget is of 'top-level window' type: */
    if (!pWindow->isWindow())
    {
        AssertMsgFailed(("Passed widget is NOT top-level window!"));
        return;
    }

    /* Make sure passed-parent-widget is of 'top-level window' type: */
    if (pParentWindow && !pParentWindow->isWindow())
    {
        AssertMsgFailed(("Passed parent widget is NOT top-level window!"));
        return;
    }

    /* If parent-window really passed: */
    if (pParentWindow)
    {
        /* Make sure we have passed-parent-window registered already.
         * If so, we have to make sure its the 'top' element in his stack also.
         * If so, we have to register passed-window as the new 'top' in that stack. */
        for (int iIteratedStackIndex = 0; iIteratedStackIndex < m_windows.size(); ++iIteratedStackIndex)
        {
            /* Get current-stack: */
            QList<QWidget*> &iteratedWindowStack = m_windows[iIteratedStackIndex];
            /* Search through all the window(s) iterated-stack contains: */
            int iIteratedWindwStackSize = iteratedWindowStack.size();
            for (int iIteratedWindowIndex = 0; iIteratedWindowIndex < iIteratedWindwStackSize; ++iIteratedWindowIndex)
            {
                /* Get iterated-window: */
                QWidget *pIteratedWindow = iteratedWindowStack[iIteratedWindowIndex];
                /* If passed-parent-window found: */
                if (pIteratedWindow == pParentWindow)
                {
                    /* Make sure it was the last one of the iterated-window(s): */
                    if (iIteratedWindowIndex != iIteratedWindwStackSize - 1)
                    {
                        AssertMsgFailed(("Passed parent window is not on the top of his current-stack!"));
                        return;
                    }
                    /* Register passed-window as the new 'top' in iterated-window-stack: */
                    iteratedWindowStack << pWindow;
                    connect(pWindow, &QWidget::destroyed, this, &UIModalWindowManager::sltRemoveFromStack);
                    return;
                }
            }
        }
        /* Passed-parent-window was not found: */
        AssertMsgFailed(("Passed parent window is not registered!"));
        return;
    }
    /* If no parent-window passed: */
    else
    {
        /* Register passed-window as the only one item in new-window-stack: */
        QList<QWidget*> newWindowStack(QList<QWidget*>() << pWindow);
        m_windows << newWindowStack;
        connect(pWindow, &QWidget::destroyed, this, &UIModalWindowManager::sltRemoveFromStack);
    }

    /* Notify listeners that their stack may have changed: */
    emit sigStackChanged();
}

void UIModalWindowManager::sltRemoveFromStack(QObject *pObject)
{
    /* Make sure passed-object still valid: */
    if (!pObject)
        return;

    /* Object is already of QObject type,
     * because inheritance wrapper(s) destructor(s) already called
     * so we can't search through the m_windows stack
     * using the standard algorithm functionality.
     * Lets do it manually: */
    for (int iIteratedStackIndex = 0; iIteratedStackIndex < m_windows.size(); ++iIteratedStackIndex)
    {
        /* Get iterated-stack: */
        QList<QWidget*> &iteratedWindowStack = m_windows[iIteratedStackIndex];
        /* Search through all the window(s) iterated-stack contains: */
        int iIteratedWindowStackSize = iteratedWindowStack.size();
        for (int iIteratedWindowIndex = 0; iIteratedWindowIndex < iIteratedWindowStackSize; ++iIteratedWindowIndex)
        {
            /* Get iterated-window: */
            QWidget *pIteratedWindow = iteratedWindowStack[iIteratedWindowIndex];
            /* If passed-object is almost-destroyed iterated-window: */
            if (pIteratedWindow == pObject)
            {
                /* Make sure it was the last added window: */
                AssertMsg(iIteratedWindowIndex == iIteratedWindowStackSize - 1, ("Removing element from the middle of the stack!"));
                /* Cleanup window pointer: */
                iteratedWindowStack.removeAt(iIteratedWindowIndex);
                /* And stack itself if necessary: */
                if (iteratedWindowStack.isEmpty())
                    m_windows.removeAt(iIteratedStackIndex);
            }
        }
    }

    /* Notify listeners that their stack may have changed: */
    emit sigStackChanged();
}

bool UIModalWindowManager::contains(QWidget *pParentWindow, bool fAsTheTopOfStack /* = false*/)
{
    /* False if passed-parent-widget pointer is null: */
    if (!pParentWindow)
    {
        AssertMsgFailed(("Passed pointer is NULL!"));
        return false;
    }

    /* False if passed-parent-widget is not of 'top-level window' type: */
    if (!pParentWindow->isWindow())
    {
        AssertMsgFailed(("Passed widget is NOT top-level window!"));
        return false;
    }

    /* Search through all the stack(s) we have: */
    foreach (const QList<QWidget*> &iteratedWindowStack, m_windows)
    {
        /* Search through all the window(s) iterated-stack contains: */
        int iIteratedWindowStackSize = iteratedWindowStack.size();
        for (int iIteratedWidnowIndex = 0; iIteratedWidnowIndex < iIteratedWindowStackSize; ++iIteratedWidnowIndex)
        {
            /* Get iterated-window: */
            QWidget *pIteratedWindow = iteratedWindowStack[iIteratedWidnowIndex];
            /* If passed-parent-window found: */
            if (pIteratedWindow == pParentWindow)
            {
                /* True if we are not looking for 'top' of the stack or its the 'top': */
                return !fAsTheTopOfStack || iIteratedWidnowIndex == iIteratedWindowStackSize - 1;
            }
        }
    }

    /* False by default: */
    return false;
}

/* static */
void UIModalWindowManager::preprocessRealParent(QWidget *pParent)
{
    /* Progress dialog can be hidden while we are trying to use it as top-most modal parent,
     * We should show it in such cases because else on MacOS X there will be a problem. */
    if (UIProgressDialog *pProgressDialog = qobject_cast<UIProgressDialog*>(pParent))
        pProgressDialog->show();
}

