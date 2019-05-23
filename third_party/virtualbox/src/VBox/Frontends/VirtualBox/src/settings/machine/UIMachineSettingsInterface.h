/* $Id: UIMachineSettingsInterface.h $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsInterface class declaration.
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

#ifndef ___UIMachineSettingsInterface_h___
#define ___UIMachineSettingsInterface_h___

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIMachineSettingsInterface.gen.h"

/* Forward declarations: */
class UIActionPool;
struct UIDataSettingsMachineInterface;
typedef UISettingsCache<UIDataSettingsMachineInterface> UISettingsCacheMachineInterface;


/** Machine settings: User Interface page. */
class UIMachineSettingsInterface : public UISettingsPageMachine,
                                   public Ui::UIMachineSettingsInterface
{
    Q_OBJECT;

public:

    /** Constructs User Interface settings page. */
    UIMachineSettingsInterface(const QString strMachineId);
    /** Destructs User Interface settings page. */
    ~UIMachineSettingsInterface();

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

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs final page polishing. */
    virtual void polishPage() /* override */;

private:

    /** Prepares all. */
    void prepare();
    /** Cleanups all. */
    void cleanup();

    /** Saves existing interface data from the cache. */
    bool saveInterfaceData();
    /** Saves existing 'Menu-bar' data from the cache. */
    bool saveMenuBarData();
    /** Saves existing 'Status-bar' data from the cache. */
    bool saveStatusBarData();
    /** Saves existing 'Mini-toolbar' data from the cache. */
    bool saveMiniToolbarData();

    /** Holds the machine ID copy. */
    const QString  m_strMachineId;
    /** Holds the action-pool instance. */
    UIActionPool  *m_pActionPool;

    /** Holds the page data cache instance. */
    UISettingsCacheMachineInterface *m_pCache;
};

#endif /* !___UIMachineSettingsInterface_h___ */

