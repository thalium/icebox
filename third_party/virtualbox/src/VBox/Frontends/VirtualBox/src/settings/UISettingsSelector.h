/* $Id: UISettingsSelector.h $ */
/** @file
 * VBox Qt GUI - UISettingsSelector class declaration.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UISettingsSelector_h___
#define ___UISettingsSelector_h___

/* Qt includes: */
#include <QObject>

/* Forward declarations: */
class QAction;
class QActionGroup;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QITreeWidget;
class UISelectorItem;
class UISelectorActionItem;
class UISettingsPage;
class UIToolBar;


/** QObject subclass providing settings dialog
  * with the means to switch between settings pages. */
class UISettingsSelector : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies listeners about selector @a iCategory changed. */
    void sigCategoryChanged(int iCategory);

public:

    /** Constructs settings selector passing @a pParent to the base-class. */
    UISettingsSelector(QWidget *pParent = 0);
    /** Destructs settings selector. */
    ~UISettingsSelector();

    /** Returns the widget selector operates on. */
    virtual QWidget *widget() const = 0;

    /** Adds a new selector item.
      * @param  strBigIcon     Brings the big icon reference.
      * @param  strMediumIcon  Brings the medium icon reference.
      * @param  strSmallIcon   Brings the small icon reference.
      * @param  iID            Brings the selector section ID.
      * @param  strLink        Brings the selector section link.
      * @param  pPage          Brings the selector section page reference.
      * @param  iParentID      Brings the parent section ID or -1 if there is no parent. */
    virtual QWidget *addItem(const QString &strBigIcon, const QString &strMediumIcon, const QString &strSmallIcon,
                             int iID, const QString &strLink, UISettingsPage *pPage = 0, int iParentID = -1) = 0;

    /** Defines the @a strText for section with @a iID. */
    virtual void setItemText(int iID, const QString &strText);
    /** Returns the text for section with @a iID. */
    virtual QString itemText(int iID) const = 0;
    /** Returns the text for section with @a pPage. */
    virtual QString itemTextByPage(UISettingsPage *pPage) const;

    /** Returns the current selector ID. */
    virtual int currentId() const = 0;

    /** Returns the section ID for passed @a strLink. */
    virtual int linkToId(const QString &strLink) const = 0;

    /** Returns the section page for passed @a iID. */
    virtual QWidget *idToPage(int iID) const;
    /** Returns the section root-page for passed @a iID. */
    virtual QWidget *rootPage(int iID) const { return idToPage(iID); }

    /** Make the section with @a iID current. */
    virtual void selectById(int iID) = 0;
    /** Make the section with @a strLink current. */
    virtual void selectByLink(const QString &strLink) { selectById(linkToId(strLink)); }

    /** Make the section with @a iID @a fVisible. */
    virtual void setVisibleById(int iID, bool fVisible) = 0;

    /** Returns the list of all selector pages. */
    virtual QList<UISettingsPage*> settingPages() const;
    /** Returns the list of all root pages. */
    virtual QList<QWidget*> rootPages() const;

    /** Performs selector polishing. */
    virtual void polish() {}

    /** Returns minimum selector width. */
    virtual int minWidth() const { return 0; }

protected:

    /** Clears selector of all the items. */
    virtual void clear() = 0;

    /** Searches for item with passed @a iID. */
    UISelectorItem *findItem(int iID) const;
    /** Searches for item with passed @a strLink. */
    UISelectorItem *findItemByLink(const QString &strLink) const;
    /** Searches for item with passed @a pPage. */
    UISelectorItem *findItemByPage(UISettingsPage *pPage) const;

    /** Holds the selector item instances. */
    QList<UISelectorItem*> m_list;
};


/** UISettingsSelector subclass providing settings dialog
  * with the means to switch between settings pages.
  * This one represented as tree-widget. */
class UISettingsSelectorTreeView : public UISettingsSelector
{
    Q_OBJECT;

public:

    /** Constructs settings selector passing @a pParent to the base-class. */
    UISettingsSelectorTreeView(QWidget *pParent = 0);
    /** Destructs settings selector. */
    ~UISettingsSelectorTreeView();

    /** Returns the widget selector operates on. */
    virtual QWidget *widget() const /* override */;

    /** Adds a new selector item.
      * @param  strBigIcon     Brings the big icon reference.
      * @param  strMediumIcon  Brings the medium icon reference.
      * @param  strSmallIcon   Brings the small icon reference.
      * @param  iID            Brings the selector section ID.
      * @param  strLink        Brings the selector section link.
      * @param  pPage          Brings the selector section page reference.
      * @param  iParentID      Brings the parent section ID or -1 if there is no parent. */
    virtual QWidget *addItem(const QString &strBigIcon, const QString &strMediumIcon, const QString &strSmallIcon,
                             int iID, const QString &strLink, UISettingsPage *pPage = 0, int iParentID = -1) /* override */;

    /** Defines the @a strText for section with @a iID. */
    virtual void setItemText(int iID, const QString &strText) /* override */;
    /** Returns the text for section with @a iID. */
    virtual QString itemText(int iID) const /* override */;

    /** Returns the current selector ID. */
    virtual int currentId() const /* override */;

    /** Returns the section ID for passed @a strLink. */
    virtual int linkToId(const QString &strLink) const /* override */;

    /** Make the section with @a iID current. */
    virtual void selectById(int iID) /* override */;

    /** Make the section with @a iID @a fVisible. */
    virtual void setVisibleById(int iID, bool fVisible) /* override */;

    /** Performs selector polishing. */
    virtual void polish() /* override */;

private slots:

    /** Handles selector section change from @a pPrevItem to @a pItem. */
    void sltSettingsGroupChanged(QTreeWidgetItem *pItem, QTreeWidgetItem *pPrevItem);

private:

    /** Clears selector of all the items. */
    virtual void clear() /* override */;

    /** Returns page path for passed @a strMatch. */
    QString pagePath(const QString &strMatch) const;
    /** Find item within the passed @a pView and @a iColumn matching @a strMatch. */
    QTreeWidgetItem *findItem(QTreeWidget *pView, const QString &strMatch, int iColumn) const;
    /** Performs @a iID to QString serialization. */
    QString idToString(int iID) const;

    /** Forges the full path for passed @a pItem. */
    static QString path(const QTreeWidgetItem *pItem);

    /** Holds the tree-widget instance. */
    QITreeWidget *m_pTreeWidget;
};


/** UISettingsSelector subclass providing settings dialog
  * with the means to switch between settings pages.
  * This one represented as tab-widget. */
class UISettingsSelectorToolBar : public UISettingsSelector
{
    Q_OBJECT;

public:

    /** Constructs settings selector passing @a pParent to the base-class. */
    UISettingsSelectorToolBar(QWidget *pParent = 0);
    /** Destructs settings selector. */
    ~UISettingsSelectorToolBar();

    /** Returns the widget selector operates on. */
    virtual QWidget *widget() const /* override */;

    /** Adds a new selector item.
      * @param  strBigIcon     Brings the big icon reference.
      * @param  strMediumIcon  Brings the medium icon reference.
      * @param  strSmallIcon   Brings the small icon reference.
      * @param  iID            Brings the selector section ID.
      * @param  strLink        Brings the selector section link.
      * @param  pPage          Brings the selector section page reference.
      * @param  iParentID      Brings the parent section ID or -1 if there is no parent. */
    virtual QWidget *addItem(const QString &strBigIcon, const QString &strMediumIcon, const QString &strSmallIcon,
                             int iID, const QString &strLink, UISettingsPage *pPage = 0, int iParentID = -1) /* override */;

    /** Defines the @a strText for section with @a iID. */
    virtual void setItemText(int iID, const QString &strText) /* override */;
    /** Returns the text for section with @a iID. */
    virtual QString itemText(int iID) const /* override */;

    /** Returns the current selector ID. */
    virtual int currentId() const /* override */;

    /** Returns the section ID for passed @a strLink. */
    virtual int linkToId(const QString &strLink) const /* override */;

    /** Returns the section page for passed @a iID. */
    virtual QWidget *idToPage(int iID) const /* override */;
    /** Returns the section root-page for passed @a iID. */
    virtual QWidget *rootPage(int iID) const /* override */;

    /** Make the section with @a iID current. */
    virtual void selectById(int iID) /* override */;

    /** Make the section with @a iID @a fVisible. */
    virtual void setVisibleById(int iID, bool fVisible) /* override */;

    /** Returns the list of all root pages. */
    virtual QList<QWidget*> rootPages() const /* override */;

    /** Returns minimum selector width. */
    virtual int minWidth() const /* override */;

private slots:

    /** Handles selector section change to @a pAction. */
    void sltSettingsGroupChanged(QAction *pAction);
    /** Handles selector section change to @a iIndex. */
    void sltSettingsGroupChanged(int iIndex);

private:

    /** Clears selector of all the items. */
    virtual void clear() /* override */;

    /** Searches for action item with passed @a iID. */
    UISelectorActionItem *findActionItem(int iID) const;
    /** Searches for action item with passed @a pAction. */
    UISelectorActionItem *findActionItemByAction(QAction *pAction) const;
    /** Searches for action item with passed @a pTabWidget and @a iIndex. */
    UISelectorActionItem *findActionItemByTabWidget(QTabWidget *pTabWidget, int iIndex) const;

    /** Holds the toolbar instance. */
    UIToolBar *m_pToolBar;
    /** Holds the action group instance. */
    QActionGroup *m_pActionGroup;
};

#endif /* !___UISettingsSelector_h___ */

