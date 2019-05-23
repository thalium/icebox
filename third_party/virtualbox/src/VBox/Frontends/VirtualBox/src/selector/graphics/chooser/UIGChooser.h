/* $Id: UIGChooser.h $ */
/** @file
 * VBox Qt GUI - UIGChooser class declaration.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIGChooser_h__
#define __UIGChooser_h__

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "UIGChooserItem.h"

/* Forward declartions: */
class UIVMItem;
class QVBoxLayout;
class UISelectorWindow;
class UIActionPool;
class UIGChooserModel;
class UIGChooserView;
class QStatusBar;

/* Graphics selector widget: */
class UIGChooser : public QWidget
{
    Q_OBJECT;

signals:

    /* Notifier: Selection change: */
    void sigSelectionChanged();

    /* Notifier: Sliding stuff: */
    void sigSlidingStarted();

    /* Notifiers: Toggle stuff: */
    void sigToggleStarted();
    void sigToggleFinished();

    /* Notifier: Group-saving stuff: */
    void sigGroupSavingStateChanged();

public:

    /* Constructor/destructor: */
    UIGChooser(UISelectorWindow *pParent);
    ~UIGChooser();

    /** Returns the selector-window reference. */
    UISelectorWindow* selector() const { return m_pSelectorWindow; }
    /** Returns the action-pool reference. */
    UIActionPool* actionPool() const;

    /** Return the Chooser-model instance. */
    UIGChooserModel *model() const { return m_pChooserModel; }
    /** Return the Chooser-view instance. */
    UIGChooserView *view() const { return m_pChooserView; }

    /* API: Current-item stuff: */
    UIVMItem* currentItem() const;
    QList<UIVMItem*> currentItems() const;
    bool isSingleGroupSelected() const;
    bool isAllItemsOfOneGroupSelected() const;

    /* API: Group-saving stuff: */
    bool isGroupSavingInProgress() const;

private:

    /* Helpers: Prepare stuff: */
    void preparePalette();
    void prepareLayout();
    void prepareModel();
    void prepareView();
    void prepareConnections();
    void load();

    /* Helper: Cleanup stuff: */
    void save();

    /** Holds the selector-window reference. */
    UISelectorWindow* m_pSelectorWindow;

    /* Variables: */
    QVBoxLayout *m_pMainLayout;
    UIGChooserModel *m_pChooserModel;
    UIGChooserView *m_pChooserView;
};

#endif /* __UIGChooser_h__ */

