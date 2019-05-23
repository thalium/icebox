/* $Id: UIMachineSettingsSystem.h $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsSystem class declaration.
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

#ifndef ___UIMachineSettingsSystem_h___
#define ___UIMachineSettingsSystem_h___

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIMachineSettingsSystem.gen.h"

/* Forward declarations: */
struct UIDataSettingsMachineSystem;
typedef UISettingsCache<UIDataSettingsMachineSystem> UISettingsCacheMachineSystem;


/** Machine settings: System page. */
class UIMachineSettingsSystem : public UISettingsPageMachine,
                                public Ui::UIMachineSettingsSystem
{
    Q_OBJECT;

public:

    /** Constructs System settings page. */
    UIMachineSettingsSystem();
    /** Destructs System settings page. */
    ~UIMachineSettingsSystem();

    /** Returns whether the HW Virt Ex is enabled. */
    bool isHWVirtExEnabled() const;

    /** Returns whether the HID is enabled. */
    bool isHIDEnabled() const;

    /** Returns the chipset type. */
    KChipsetType chipsetType() const;

    /** Defines whether the USB is enabled. */
    void setUSBEnabled(bool fEnabled);

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

    /** Preprocesses any Qt @a pEvent for passed @a pObject. */
    virtual bool eventFilter(QObject *pObject, QEvent *pEvent) /* override */;

private slots:

    /** Handles memory size slider change. */
    void sltHandleMemorySizeSliderChange();
    /** Handles memory size editor change. */
    void sltHandleMemorySizeEditorChange();

    /** Handle current boot item change to @a iCurrentIndex. */
    void sltHandleCurrentBootItemChange(int iCurrentIndex);

    /** Handles CPU count slider change. */
    void sltHandleCPUCountSliderChange();
    /** Handles CPU count editor change. */
    void sltHandleCPUCountEditorChange();
    /** Handles CPU execution cap slider change. */
    void sltHandleCPUExecCapSliderChange();
    /** Handles CPU execution cap editor change. */
    void sltHandleCPUExecCapEditorChange();

private:

    /** Prepares all. */
    void prepare();
    /** Prepares 'Motherboard' tab. */
    void prepareTabMotherboard();
    /** Prepares 'Processor' tab. */
    void prepareTabProcessor();
    /** Prepares 'Acceleration' tab. */
    void prepareTabAcceleration();
    /** Prepares connections. */
    void prepareConnections();
    /** Cleanups all. */
    void cleanup();

    /** Repopulates Pointing HID type combo-box. */
    void repopulateComboPointingHIDType();

    /** Retranslates Chipset type combo-box. */
    void retranslateComboChipsetType();
    /** Retranslates Pointing HID type combo-box. */
    void retranslateComboPointingHIDType();
    /** Retranslates Paravirtualization providers combo-box. */
    void retranslateComboParavirtProvider();

    /** Adjusts boot-order tree-widget size. */
    void adjustBootOrderTWSize();

    /** Saves existing system data from the cache. */
    bool saveSystemData();
    /** Saves existing 'Motherboard' data from the cache. */
    bool saveMotherboardData();
    /** Saves existing 'Processor' data from the cache. */
    bool saveProcessorData();
    /** Saves existing 'Acceleration' data from the cache. */
    bool saveAccelerationData();

    /** Holds the list of all possible boot items. */
    QList<KDeviceType>  m_possibleBootItems;

    /** Holds the minimum guest CPU count. */
    uint  m_uMinGuestCPU;
    /** Holds the maximum guest CPU count. */
    uint  m_uMaxGuestCPU;
    /** Holds the minimum guest CPU execution cap. */
    uint  m_uMinGuestCPUExecCap;
    /** Holds the medium guest CPU execution cap. */
    uint  m_uMedGuestCPUExecCap;
    /** Holds the maximum guest CPU execution cap. */
    uint  m_uMaxGuestCPUExecCap;

    /** Holds whether the USB is enabled. */
    bool m_fIsUSBEnabled;

    /** Holds the page data cache instance. */
    UISettingsCacheMachineSystem *m_pCache;
};

#endif /* !___UIMachineSettingsSystem_h___ */

