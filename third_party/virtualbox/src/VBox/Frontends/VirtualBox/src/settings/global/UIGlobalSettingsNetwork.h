/* $Id: UIGlobalSettingsNetwork.h $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsNetwork class declaration.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIGlobalSettingsNetwork_h___
#define ___UIGlobalSettingsNetwork_h___

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIGlobalSettingsNetwork.gen.h"
#include "UIPortForwardingTable.h"

/* Forward declarations: */
class UIItemNetworkNAT;
struct UIDataSettingsGlobalNetwork;
struct UIDataSettingsGlobalNetworkNAT;
typedef UISettingsCache<UIDataPortForwardingRule> UISettingsCachePortForwardingRule;
typedef UISettingsCachePoolOfTwo<UIDataSettingsGlobalNetworkNAT, UISettingsCachePortForwardingRule, UISettingsCachePortForwardingRule> UISettingsCacheGlobalNetworkNAT;
typedef UISettingsCachePool<UIDataSettingsGlobalNetwork, UISettingsCacheGlobalNetworkNAT> UISettingsCacheGlobalNetwork;


/** Global settings: Network page. */
class UIGlobalSettingsNetwork : public UISettingsPageGlobal,
                                public Ui::UIGlobalSettingsNetwork
{
    Q_OBJECT;

public:

    /** Constructs Network settings page. */
    UIGlobalSettingsNetwork();
    /** Destructs Network settings page. */
    ~UIGlobalSettingsNetwork();

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

    /** Handles command to add NAT network. */
    void sltAddNATNetwork();
    /** Handles command to remove NAT network. */
    void sltRemoveNATNetwork();
    /** Handles command to edit NAT network. */
    void sltEditNATNetwork();

    /** Handles @a pChangedItem change for NAT network tree. */
    void sltHandleItemChangeNATNetwork(QTreeWidgetItem *pChangedItem);
    /** Handles NAT network tree current item change. */
    void sltHandleCurrentItemChangeNATNetwork();
    /** Handles context menu request for @a position of NAT network tree. */
    void sltHandleContextMenuRequestNATNetwork(const QPoint &position);

private:

    /** Prepares all. */
    void prepare();
    /** Prepares NAT network tree. */
    void prepareNATNetworkTree();
    /** Prepares NAT network toolbar. */
    void prepareNATNetworkToolbar();
    /** Prepares connections. */
    void prepareConnections();
    /** Cleanups all. */
    void cleanup();

    /** Saves existing network data from the cache. */
    bool saveNetworkData();

    /** Uploads NAT @a network data into passed @a cache storage unit. */
    void loadToCacheFromNATNetwork(const CNATNetwork &network, UISettingsCacheGlobalNetworkNAT &cache);
    /** Removes corresponding NAT network on the basis of @a cache. */
    bool removeNATNetwork(const UISettingsCacheGlobalNetworkNAT &cache);
    /** Creates corresponding NAT network on the basis of @a cache. */
    bool createNATNetwork(const UISettingsCacheGlobalNetworkNAT &cache);
    /** Updates @a cache of corresponding NAT network. */
    bool updateNATNetwork(const UISettingsCacheGlobalNetworkNAT &cache);
    /** Creates a new item in the NAT network tree on the basis of passed @a cache. */
    void createTreeWidgetItemForNATNetwork(const UISettingsCacheGlobalNetworkNAT &cache);
    /** Creates a new item in the NAT network tree on the basis of passed
      * @a data, @a ipv4rules, @a ipv6rules, @a fChooseItem if requested. */
    void createTreeWidgetItemForNATNetwork(const UIDataSettingsGlobalNetworkNAT &data,
                                           const UIPortForwardingDataList &ipv4rules,
                                           const UIPortForwardingDataList &ipv6rules,
                                           bool fChooseItem = false);
    /** Removes existing @a pItem from the NAT network tree. */
    void removeTreeWidgetItemOfNATNetwork(UIItemNetworkNAT *pItem);
    /** Returns whether the NAT network described by the @a cache could be updated or recreated otherwise. */
    bool isNetworkCouldBeUpdated(const UISettingsCacheGlobalNetworkNAT &cache) const;

    /** Holds the Add NAT network action instance. */
    QAction *m_pActionAddNATNetwork;
    /** Holds the Remove NAT network action instance. */
    QAction *m_pActionRemoveNATNetwork;
    /** Holds the Edit NAT network action instance. */
    QAction *m_pActionEditNATNetwork;

    /** Holds the page data cache instance. */
    UISettingsCacheGlobalNetwork *m_pCache;
};

#endif /* !___UIGlobalSettingsNetwork_h___ */

