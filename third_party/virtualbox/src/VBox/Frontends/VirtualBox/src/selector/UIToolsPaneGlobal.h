/* $Id: UIToolsPaneGlobal.h $ */
/** @file
 * VBox Qt GUI - UIToolsPaneGlobal class declaration.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIToolsPaneGlobal_h___
#define ___UIToolsPaneGlobal_h___

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UIExtraDataDefs.h"

/* Forward declarations: */
class QHBoxLayout;
class QStackedLayout;
class QVBoxLayout;
class UIActionPool;
class UIDesktopPane;
class UIHostNetworkManagerWidget;
class UIMediumManagerWidget;
class UIVMItem;
class CMachine;


/** QWidget subclass representing container for tool panes. */
class UIToolsPaneGlobal : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs tools pane passing @a pParent to the base-class. */
    UIToolsPaneGlobal(UIActionPool *pActionPool, QWidget *pParent = 0);
    /** Destructs tools pane. */
    virtual ~UIToolsPaneGlobal() /* override */;

    /** Returns type of tool currently opened. */
    ToolTypeGlobal currentTool() const;
    /** Returns whether tool of particular @a enmType is opened. */
    bool isToolOpened(ToolTypeGlobal enmType) const;
    /** Activates tool of passed @a enmType, creates new one if necessary. */
    void openTool(ToolTypeGlobal enmType);
    /** Closes tool of passed @a enmType, deletes one if exists. */
    void closeTool(ToolTypeGlobal enmType);

    /** Defines @a strError and switches to error details pane. */
    void setDetailsError(const QString &strError);

protected:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private:

    /** Prepares all. */
    void prepare();
    /** Prepares stacked-layout. */
    void prepareStackedLayout();
    /** Cleanups all. */
    void cleanup();

    /** Holds the action pool reference. */
    UIActionPool *m_pActionPool;

    /** Holds the stacked-layout instance. */
    QStackedLayout             *m_pLayout;
    /** Holds the Desktop pane instance. */
    UIDesktopPane              *m_pPaneDesktop;
    /** Holds the Virtual Media Manager instance. */
    UIMediumManagerWidget      *m_pPaneMedia;
    /** Holds the Host Network Manager instance. */
    UIHostNetworkManagerWidget *m_pPaneNetwork;
};

#endif /* !___UIToolsPaneGlobal_h___ */

