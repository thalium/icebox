/* $Id: UIMediumManager.h $ */
/** @file
 * VBox Qt GUI - UIMediumManager class declaration.
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

#ifndef ___UIMediumManager_h___
#define ___UIMediumManager_h___

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"
#include "UIMediumDefs.h"

/* Forward declarations: */
class QAbstractButton;
class QLabel;
class QProgressBar;
class QTabWidget;
class QTreeWidgetItem;
class QIDialogButtonBox;
class QILabel;
class QITreeWidget;
class UIMedium;
class UIMediumDetailsWidget;
class UIMediumItem;
class UIToolBar;


/** Functor interface allowing to check if passed UIMediumItem is suitable. */
class CheckIfSuitableBy
{
public:

    /** Destructs functor. */
    virtual ~CheckIfSuitableBy() { /* Makes MSC happy. */ }

    /** Determines whether passed @a pItem is suitable. */
    virtual bool isItSuitable(UIMediumItem *pItem) const = 0;
};


/** Medium manager progress-bar.
  * Reflects medium-enumeration progress, stays hidden otherwise. */
class UIEnumerationProgressBar : public QWidget
{
    Q_OBJECT;

public:

    /** Constructor on the basis of passed @a pParent. */
    UIEnumerationProgressBar(QWidget *pParent = 0);

    /** Defines progress-bar label-text. */
    void setText(const QString &strText);

    /** Returns progress-bar current-value. */
    int value() const;
    /** Defines progress-bar current-value. */
    void setValue(int iValue);
    /** Defines progress-bar maximum-value. */
    void setMaximum(int iValue);

private:

    /** Prepares progress-bar content. */
    void prepare();

    /** Progress-bar label. */
    QLabel       *m_pLabel;
    /** Progress-bar itself. */
    QProgressBar *m_pProgressBar;
};


/** QWidget extension providing GUI with the pane to control media related functionality. */
class UIMediumManagerWidget : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

    /** Item action types. */
    enum Action { Action_Add, Action_Edit, Action_Copy, Action_Remove, Action_Release };

signals:

    /** Notifies listeners about medium details-widget @a fVisible. */
    void sigMediumDetailsVisibilityChanged(bool fVisible);
    /** Notifies listeners about accept is @a fAllowed. */
    void sigAcceptAllowed(bool fAllowed);
    /** Notifies listeners about reject is @a fAllowed. */
    void sigRejectAllowed(bool fAllowed);

public:

    /** Constructs Virtual Media Manager widget. */
    UIMediumManagerWidget(EmbedTo enmEmbedding, QWidget *pParent = 0);

    /** Returns the menu. */
    QMenu *menu() const { return m_pMenu; }

#ifdef VBOX_WS_MAC
    /** Returns the toolbar. */
    UIToolBar *toolbar() const { return m_pToolBar; }
#endif

    /** Defines @a pProgressBar reference. */
    void setProgressBar(UIEnumerationProgressBar *pProgressBar);

protected:

    /** @name Event-handling stuff.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() /* override */;

        /** Handles @a pShow event. */
        virtual void showEvent(QShowEvent *pEvent) /* override */;
    /** @} */

public slots:

    /** @name Details-widget stuff.
      * @{ */
        /** Handles command to reset medium details changes. */
        void sltResetMediumDetailsChanges();
        /** Handles command to apply medium details changes. */
        void sltApplyMediumDetailsChanges();
    /** @} */

private slots:

    /** @name Medium operation stuff.
      * @{ */
        /** Handles VBoxGlobal::sigMediumCreated signal. */
        void sltHandleMediumCreated(const QString &strMediumID);
        /** Handles VBoxGlobal::sigMediumDeleted signal. */
        void sltHandleMediumDeleted(const QString &strMediumID);
    /** @} */

    /** @name Medium enumeration stuff.
      * @{ */
        /** Handles VBoxGlobal::sigMediumEnumerationStarted signal. */
        void sltHandleMediumEnumerationStart();
        /** Handles VBoxGlobal::sigMediumEnumerated signal. */
        void sltHandleMediumEnumerated(const QString &strMediumID);
        /** Handles VBoxGlobal::sigMediumEnumerationFinished signal. */
        void sltHandleMediumEnumerationFinish();
    /** @} */

    /** @name Menu/action stuff.
      * @{ */
        /** Handles command to copy medium. */
        void sltCopyMedium();
        /** Handles command to move medium. */
        void sltMoveMedium();
        /** Handles command to remove medium. */
        void sltRemoveMedium();
        /** Handles command to release medium. */
        void sltReleaseMedium();
        /** Handles command to make medium details @a fVisible. */
        void sltToggleMediumDetailsVisibility(bool fVisible);
        /** Handles command to refresh medium. */
        void sltRefreshAll();
    /** @} */

    /** @name Tab-widget stuff.
      * @{ */
        /** Handles tab change case. */
        void sltHandleCurrentTabChanged();
        /** Handles item change case. */
        void sltHandleCurrentItemChanged();
        /** Handles item context-menu-call case. */
        void sltHandleContextMenuCall(const QPoint &position);
    /** @} */

    /** @name Tree-widget stuff.
      * @{ */
        /** Adjusts tree-widgets according content. */
        void sltPerformTablesAdjustment();
    /** @} */

private:

    /** @name Prepare/cleanup cascade.
      * @{ */
        /** Prepares all. */
        void prepare();
        /** Prepares this. */
        void prepareThis();
        /** Prepares connections. */
        void prepareConnections();
        /** Prepares actions. */
        void prepareActions();
        /** Prepares menu. */
        void prepareMenu();
        /** Prepares context-menu. */
        void prepareContextMenu();
        /** Prepares widgets. */
        void prepareWidgets();
        /** Prepares toolbar. */
        void prepareToolBar();
        /** Prepares tab-widget. */
        void prepareTabWidget();
        /** Prepares tab-widget's tab. */
        void prepareTab(UIMediumType type);
        /** Prepares tab-widget's tree-widget. */
        void prepareTreeWidget(UIMediumType type, int iColumns);
        /** Prepares details-widget. */
        void prepareDetailsWidget();
        /** Load settings: */
        void loadSettings();

        /** Repopulates tree-widgets content. */
        void repopulateTreeWidgets();

        /** Updates details according latest changes in current item of passed @a type. */
        void refetchCurrentMediumItem(UIMediumType type);
        /** Updates details according latest changes in current item of chosen type. */
        void refetchCurrentChosenMediumItem();
        /** Updates details according latest changes in all current items. */
        void refetchCurrentMediumItems();

        /** Updates actions according currently chosen item. */
        void updateActions();
        /** Updates action icons according currently chosen tab. */
        void updateActionIcons();
        /** Updates tab icons according last @a action happened with @a pItem. */
        void updateTabIcons(UIMediumItem *pItem, Action action);
    /** @} */

    /** @name Widget operation stuff.
      * @{ */
        /** Creates UIMediumItem for corresponding @a medium. */
        UIMediumItem *createMediumItem(const UIMedium &medium);
        /** Creates UIMediumItemHD for corresponding @a medium. */
        UIMediumItem *createHardDiskItem(const UIMedium &medium);
        /** Updates UIMediumItem for corresponding @a medium. */
        void updateMediumItem(const UIMedium &medium);
        /** Deletes UIMediumItem for corresponding @a strMediumID. */
        void deleteMediumItem(const QString &strMediumID);

        /** Returns tab for passed medium @a type. */
        QWidget *tab(UIMediumType type) const;
        /** Returns tree-widget for passed medium @a type. */
        QITreeWidget *treeWidget(UIMediumType type) const;
        /** Returns item for passed medium @a type. */
        UIMediumItem *mediumItem(UIMediumType type) const;

        /** Returns medium type for passed @a pTreeWidget. */
        UIMediumType mediumType(QITreeWidget *pTreeWidget) const;

        /** Returns current medium type. */
        UIMediumType currentMediumType() const;
        /** Returns current tree-widget. */
        QITreeWidget *currentTreeWidget() const;
        /** Returns current item. */
        UIMediumItem *currentMediumItem() const;

        /** Defines current item for passed @a pTreeWidget as @a pItem. */
        void setCurrentItem(QITreeWidget *pTreeWidget, QTreeWidgetItem *pItem);
    /** @} */

    /** @name Helper stuff.
      * @{ */
        /** Returns tab index for passed UIMediumType. */
        static int tabIndex(UIMediumType type);

        /** Performs search for the @a pTree child which corresponds to the @a condition but not @a pException. */
        static UIMediumItem *searchItem(QITreeWidget *pTree,
                                        const CheckIfSuitableBy &condition,
                                        CheckIfSuitableBy *pException = 0);
        /** Performs search for the @a pParentItem child which corresponds to the @a condition but not @a pException. */
        static UIMediumItem *searchItem(QTreeWidgetItem *pParentItem,
                                        const CheckIfSuitableBy &condition,
                                        CheckIfSuitableBy *pException = 0);

        /** Checks if @a action can be used for @a pItem. */
        static bool checkMediumFor(UIMediumItem *pItem, Action action);

        /** Casts passed QTreeWidgetItem @a pItem to UIMediumItem if possible. */
        static UIMediumItem *toMediumItem(QTreeWidgetItem *pItem);
    /** @} */

    /** @name General variables.
      * @{ */
        /** Holds the widget embedding type. */
        const EmbedTo m_enmEmbedding;

        /** Holds whether Virtual Media Manager should preserve current item change. */
        bool m_fPreventChangeCurrentItem;
    /** @} */

    /** @name Tab-widget variables.
      * @{ */
        /** Holds the tab-widget instance. */
        QTabWidget                  *m_pTabWidget;
        /** Holds the tab-widget tab-count. */
        const int                    m_iTabCount;
        /** Holds the map of tree-widget instances. */
        QMap<int, QITreeWidget*>     m_trees;
        /** Holds whether hard-drive tab-widget have inaccessible item. */
        bool                         m_fInaccessibleHD;
        /** Holds whether optical-disk tab-widget have inaccessible item. */
        bool                         m_fInaccessibleCD;
        /** Holds whether floppy-disk tab-widget have inaccessible item. */
        bool                         m_fInaccessibleFD;
        /** Holds cached hard-drive tab-widget icon. */
        const QIcon                  m_iconHD;
        /** Holds cached optical-disk tab-widget icon. */
        const QIcon                  m_iconCD;
        /** Holds cached floppy-disk tab-widget icon. */
        const QIcon                  m_iconFD;
        /** Holds current hard-drive tree-view item ID. */
        QString                      m_strCurrentIdHD;
        /** Holds current optical-disk tree-view item ID. */
        QString                      m_strCurrentIdCD;
        /** Holds current floppy-disk tree-view item ID. */
        QString                      m_strCurrentIdFD;
    /** @} */

    /** @name Details-widget variables.
      * @{ */
        /** Holds the medium details-widget instance. */
        UIMediumDetailsWidget *m_pDetailsWidget;
    /** @} */

    /** @name Toolbar and menu variables.
      * @{ */
        /** Holds the toolbar widget instance. */
        UIToolBar *m_pToolBar;
        /** Holds the context-menu object instance. */
        QMenu     *m_pContextMenu;
        /** Holds the menu object instance. */
        QMenu     *m_pMenu;
        /** Holds the Copy action instance. */
        QAction   *m_pActionCopy;
        /** Holds the Move action instance. */
        QAction   *m_pActionMove;
        /** Holds the Remove action instance. */
        QAction   *m_pActionRemove;
        /** Holds the Release action instance. */
        QAction   *m_pActionRelease;
        /** Holds the Details action instance. */
        QAction   *m_pActionDetails;
        /** Holds the Refresh action instance. */
        QAction   *m_pActionRefresh;
    /** @} */

    /** @name Progress-bar variables.
      * @{ */
        /** Holds the progress-bar widget reference. */
        UIEnumerationProgressBar *m_pProgressBar;
    /** @} */
};


/** QIManagerDialogFactory extension used as a factory for Virtual Media Manager dialog. */
class UIMediumManagerFactory : public QIManagerDialogFactory
{
protected:

    /** Creates derived @a pDialog instance.
      * @param  pCenterWidget  Brings the widget reference to center according to. */
    virtual void create(QIManagerDialog *&pDialog, QWidget *pCenterWidget) /* override */;
};


/** QIManagerDialog extension providing GUI with the dialog to control media related functionality. */
class UIMediumManager : public QIWithRetranslateUI<QIManagerDialog>
{
    Q_OBJECT;

signals:

    /** Notifies listeners about data change rejected and should be reseted. */
    void sigDataChangeRejected();
    /** Notifies listeners about data change accepted and should be applied. */
    void sigDataChangeAccepted();

private slots:

    /** @name Button-box stuff.
      * @{ */
        /** Handles button-box button click. */
        void sltHandleButtonBoxClick(QAbstractButton *pButton);
    /** @} */

private:

    /** Constructs Host Network Manager dialog.
      * @param  pCenterWidget  Brings the widget reference to center according to. */
    UIMediumManager(QWidget *pCenterWidget);

    /** @name Event-handling stuff.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() /* override */;
    /** @} */

    /** @name Prepare/cleanup cascade.
      * @{ */
        /** Configures all. */
        virtual void configure() /* override */;
        /** Configures central-widget. */
        virtual void configureCentralWidget() /* override */;
        /** Configures button-box. */
        virtual void configureButtonBox() /* override */;
        /** Perform final preparations. */
        virtual void finalize() /* override */;
    /** @} */

    /** @name Widget stuff.
      * @{ */
        /** Returns the widget. */
        virtual UIMediumManagerWidget *widget() /* override */;
    /** @} */

    /** @name Progress-bar variables.
      * @{ */
        /** Holds the progress-bar widget instance. */
        UIEnumerationProgressBar *m_pProgressBar;
    /** @} */

    /** Allow factory access to private/protected members: */
    friend class UIMediumManagerFactory;
};

#endif /* !___UIMediumManager_h___ */

