/* $Id: UIMachineSettingsUSB.h $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsUSB class declaration.
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

#ifndef ___UIMachineSettingsUSB_h___
#define ___UIMachineSettingsUSB_h___

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIMachineSettingsUSB.gen.h"

/* Forward declarations: */
class VBoxUSBMenu;
class UIToolBar;
struct UIDataSettingsMachineUSB;
struct UIDataSettingsMachineUSBFilter;
typedef UISettingsCache<UIDataSettingsMachineUSBFilter> UISettingsCacheMachineUSBFilter;
typedef UISettingsCachePool<UIDataSettingsMachineUSB, UISettingsCacheMachineUSBFilter> UISettingsCacheMachineUSB;


/** Machine settings: USB page. */
class UIMachineSettingsUSB : public UISettingsPageMachine,
                             public Ui::UIMachineSettingsUSB
{
    Q_OBJECT;

public:

    /** Remote mode types. */
    enum { ModeAny, ModeOn, ModeOff };

    /** Constructs USB settings page. */
    UIMachineSettingsUSB();
    /** Destructs USB settings page. */
    ~UIMachineSettingsUSB();

    /** Returns whether the USB is enabled. */
    bool isUSBEnabled() const;

protected:

    /** Returns whether the page content was changed. */
    virtual bool changed() const /* override */;

    /** Loads data into the cache from corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    virtual void loadToCacheFrom(QVariant &data) /* override */;
    /** Loads data into corresponding widgets from the cache,
      * this task SHOULD be performed in the GUI thread only. */
    virtual void getFromCache() /* override */;

    /** Saves data from corresponding widgets to the cache,
      * this task SHOULD be performed in the GUI thread only. */
    virtual void putToCache() /* override */;
    /** Saves data from the cache to corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    virtual void saveFromCacheTo(QVariant &data) /* overrride */;

    /** Performs validation, updates @a messages list if something is wrong. */
    virtual bool validate(QList<UIValidationMessage> &messages) /* override */;

    /** Defines TAB order for passed @a pWidget. */
    virtual void setOrderAfter(QWidget *pWidget) /* override */;

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs final page polishing. */
    virtual void polishPage() /* override */;

private slots:

    /** Handles whether USB adapted is @a fEnabled. */
    void sltHandleUsbAdapterToggle(bool fEnabled);

    /** Handles USB filter tree @a pCurrentItem change. */
    void sltHandleCurrentItemChange(QTreeWidgetItem *pCurrentItem);
    /** Handles context menu request for @a position of USB filter tree. */
    void sltHandleContextMenuRequest(const QPoint &position);
    /** Handles USB filter tree activity state change for @a pChangedItem. */
    void sltHandleActivityStateChange(QTreeWidgetItem *pChangedItem);

    /** Handles command to add new USB filter. */
    void sltNewFilter();
    /** Handles command to add existing USB filter. */
    void sltAddFilter();
    /** Handles command to edit USB filter. */
    void sltEditFilter();
    /** Handles command to confirm add of existing USB filter defined by @a pAction. */
    void sltAddFilterConfirmed(QAction *pAction);
    /** Handles command to remove chosen USB filter. */
    void sltRemoveFilter();
    /** Handles command to move chosen USB filter up. */
    void sltMoveFilterUp();
    /** Handles command to move chosen USB filter down. */
    void sltMoveFilterDown();

private:

    /** Prepares all. */
    void prepare();
    /** Prepares USB filters tree. */
    void prepareFiltersTree();
    /** Prepares USB filters toolbar. */
    void prepareFiltersToolbar();
    /** Prepares connections. */
    void prepareConnections();
    /** Cleanups all. */
    void cleanup();

    /** Adds USB filter item based on a given @a filterData, fChoose if requested. */
    void addUSBFilterItem(const UIDataSettingsMachineUSBFilter &filterData, bool fChoose);

    /** Saves existing USB data from the cache. */
    bool saveUSBData();
    /** Removes USB controllers of passed @a types. */
    bool removeUSBControllers(const QSet<KUSBControllerType> &types = QSet<KUSBControllerType>());
    /** Creates USB controllers of passed @a enmType. */
    bool createUSBControllers(KUSBControllerType enmType);
    /** Removes USB filter at passed @a iPosition of the @a filtersObject. */
    bool removeUSBFilter(CUSBDeviceFilters &comFiltersObject, int iPosition);
    /** Creates USB filter at passed @a iPosition of the @a filtersObject using the @a filterData. */
    bool createUSBFilter(CUSBDeviceFilters &comFiltersObject, int iPosition, const UIDataSettingsMachineUSBFilter &filterData);

    /** Holds the toolbar instance. */
    UIToolBar   *m_pToolBar;
    /** Holds the New action instance. */
    QAction     *m_pActionNew;
    /** Holds the Add action instance. */
    QAction     *m_pActionAdd;
    /** Holds the Edit action instance. */
    QAction     *m_pActionEdit;
    /** Holds the Remove action instance. */
    QAction     *m_pActionRemove;
    /** Holds the Move Up action instance. */
    QAction     *m_pActionMoveUp;
    /** Holds the Move Down action instance. */
    QAction     *m_pActionMoveDown;
    /** Holds the USB devices menu instance. */
    VBoxUSBMenu *m_pMenuUSBDevices;

    /** Holds the "New Filter %1" translation tag. */
    QString  m_strTrUSBFilterName;

    /** Holds the page data cache instance. */
    UISettingsCacheMachineUSB *m_pCache;
};

#endif /* !___UIMachineSettingsUSB_h___ */

