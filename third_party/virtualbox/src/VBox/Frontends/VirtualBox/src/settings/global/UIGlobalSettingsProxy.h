/* $Id: UIGlobalSettingsProxy.h $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsProxy class declaration.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIGlobalSettingsProxy_h___
#define ___UIGlobalSettingsProxy_h___

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIGlobalSettingsProxy.gen.h"
#include "VBoxUtils.h"

/* Forward declarations: */
struct UIDataSettingsGlobalProxy;
typedef UISettingsCache<UIDataSettingsGlobalProxy> UISettingsCacheGlobalProxy;


/** Global settings: Proxy page. */
class UIGlobalSettingsProxy : public UISettingsPageGlobal,
                              public Ui::UIGlobalSettingsProxy
{
    Q_OBJECT;

public:

    /** Constructs Proxy settings page. */
    UIGlobalSettingsProxy();
    /** Destructs Proxy settings page. */
    ~UIGlobalSettingsProxy();

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

    /** Performs validation, updates @a messages list if something is wrong. */
    virtual bool validate(QList<UIValidationMessage> &messages) /* override */;

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private slots:

    /** Handles proxy toggling. */
    void sltHandleProxyToggle();

private:

    /** Prepares all. */
    void prepare();
    /** Cleanups all. */
    void cleanup();

    /** Saves existing proxy data from the cache. */
    bool saveProxyData();

    /** Holds the page data cache instance. */
    UISettingsCacheGlobalProxy *m_pCache;
};

#endif /* !___UIGlobalSettingsProxy_h___ */

