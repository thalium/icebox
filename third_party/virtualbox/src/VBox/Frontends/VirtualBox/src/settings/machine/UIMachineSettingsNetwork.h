/* $Id: UIMachineSettingsNetwork.h $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsNetwork class declaration.
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

#ifndef ___UIMachineSettingsNetwork_h___
#define ___UIMachineSettingsNetwork_h___

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIMachineSettingsNetwork.gen.h"
#include "UIMachineSettingsPortForwardingDlg.h"

/* Forward declarations: */
class QITabWidget;
struct UIDataSettingsMachineNetwork;
struct UIDataSettingsMachineNetworkAdapter;
typedef UISettingsCache<UIDataPortForwardingRule> UISettingsCachePortForwardingRule;
typedef UISettingsCachePool<UIDataSettingsMachineNetworkAdapter, UISettingsCachePortForwardingRule> UISettingsCacheMachineNetworkAdapter;
typedef UISettingsCachePool<UIDataSettingsMachineNetwork, UISettingsCacheMachineNetworkAdapter> UISettingsCacheMachineNetwork;


/** Machine settings: Network page. */
class UIMachineSettingsNetworkPage : public UISettingsPageMachine
{
    Q_OBJECT;

public:

    /** Constructs Network settings page. */
    UIMachineSettingsNetworkPage();
    /** Destructs Network settings page. */
    ~UIMachineSettingsNetworkPage();

    /** Returns the bridged adapter list. */
    const QStringList &bridgedAdapterList() const { return m_bridgedAdapterList; }
    /** Returns the internal network list. */
    const QStringList &internalNetworkList() const { return m_internalNetworkList; }
    /** Returns the host-only interface list. */
    const QStringList &hostInterfaceList() const { return m_hostInterfaceList; }
    /** Returns the generic driver list. */
    const QStringList &genericDriverList() const { return m_genericDriverList; }
    /** Returns the NAT network list. */
    const QStringList &natNetworkList() const { return m_natNetworkList; }

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

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs final page polishing. */
    virtual void polishPage() /* override */;

private slots:

    /** Handles particular tab update. */
    void sltHandleTabUpdate();

    /** Handles whether the advanced button is @a fExpanded. */
    void sltHandleAdvancedButtonStateChange(bool fExpanded);

private:

    /** Prepares all. */
    void prepare();
    /** Cleanups all. */
    void cleanup();

    /** Repopulates bridged adapter list. */
    void refreshBridgedAdapterList();
    /** Repopulates internal network list. */
    void refreshInternalNetworkList(bool fFullRefresh = false);
    /** Repopulates host-only interface list. */
    void refreshHostInterfaceList();
    /** Repopulates generic driver list. */
    void refreshGenericDriverList(bool fFullRefresh = false);
    /** Repopulates NAT network list. */
    void refreshNATNetworkList();

    /** Populates a list of known internal networks. */
    static QStringList otherInternalNetworkList();
    /** Populates a list of known generic drivers. */
    static QStringList otherGenericDriverList();
    /** Loads generic properties from passed @a adapter. */
    static QString loadGenericProperties(const CNetworkAdapter &adapter);
    /** Saves generic @a strProperties to passed @a adapter. */
    static bool saveGenericProperties(CNetworkAdapter &comAdapter, const QString &strProperties);

    /** Saves existing network data from the cache. */
    bool saveNetworkData();
    /** Saves existing adapter data from the cache. */
    bool saveAdapterData(int iSlot);

    /** Holds the tab-widget instance. */
    QITabWidget *m_pTabWidget;

    /** Holds the bridged adapter list. */
    QStringList  m_bridgedAdapterList;
    /** Holds the internal network list. */
    QStringList  m_internalNetworkList;
    /** Holds the host-only interface list. */
    QStringList  m_hostInterfaceList;
    /** Holds the generic driver list. */
    QStringList  m_genericDriverList;
    /** Holds the NAT network list. */
    QStringList  m_natNetworkList;

    /** Holds the page data cache instance. */
    UISettingsCacheMachineNetwork *m_pCache;
};

#endif /* !___UIMachineSettingsNetwork_h___ */

