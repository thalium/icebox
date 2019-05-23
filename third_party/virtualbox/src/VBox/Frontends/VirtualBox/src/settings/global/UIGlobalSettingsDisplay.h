/* $Id: UIGlobalSettingsDisplay.h $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsDisplay class declaration.
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

#ifndef ___UIGlobalSettingsDisplay_h___
#define ___UIGlobalSettingsDisplay_h___

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIGlobalSettingsDisplay.gen.h"

/* Forward declarations: */
struct UIDataSettingsGlobalDisplay;
typedef UISettingsCache<UIDataSettingsGlobalDisplay> UISettingsCacheGlobalDisplay;


/** Global settings: Display page. */
class UIGlobalSettingsDisplay : public UISettingsPageGlobal,
                                public Ui::UIGlobalSettingsDisplay
{
    Q_OBJECT;

public:

    /** Constructs Display settings page. */
    UIGlobalSettingsDisplay();
    /** Destructs Display settings page. */
    ~UIGlobalSettingsDisplay();

protected:

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

private slots:

    /** Handles maximum guest-screen size policy change. */
    void sltHandleMaximumGuestScreenSizePolicyChange();

private:

    /** Prepares all. */
    void prepare();
    /** Cleanups all. */
    void cleanup();

    /** Reloads maximum guest-screen size policy combo-box. */
    void reloadMaximumGuestScreenSizePolicyComboBox();

    /** Saves existing display data from the cache. */
    bool saveDisplayData();

    /** Holds the page data cache instance. */
    UISettingsCacheGlobalDisplay *m_pCache;
};

#endif /* !___UIGlobalSettingsDisplay_h___ */

