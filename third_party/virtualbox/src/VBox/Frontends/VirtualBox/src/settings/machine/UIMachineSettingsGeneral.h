/* $Id: UIMachineSettingsGeneral.h $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsGeneral class declaration.
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

#ifndef ___UIMachineSettingsGeneral_h___
#define ___UIMachineSettingsGeneral_h___

/* GUI includes: */
#include "UIAddDiskEncryptionPasswordDialog.h"
#include "UISettingsPage.h"
#include "UIMachineSettingsGeneral.gen.h"

/* Forward declarations: */
struct UIDataSettingsMachineGeneral;
typedef UISettingsCache<UIDataSettingsMachineGeneral> UISettingsCacheMachineGeneral;


/** Machine settings: General page. */
class UIMachineSettingsGeneral : public UISettingsPageMachine,
                                 public Ui::UIMachineSettingsGeneral
{
    Q_OBJECT;

public:

    /** Constructs General settings page. */
    UIMachineSettingsGeneral();
    /** Destructs General settings page. */
    ~UIMachineSettingsGeneral();

    /** Returns the VM OS type ID. */
    CGuestOSType guestOSType() const;
    /** Returns whether 64bit OS type ID is selected. */
    bool is64BitOSTypeSelected() const;
#ifdef VBOX_WITH_VIDEOHWACCEL
    /** Returns whether Windows OS type ID is selected. */
    bool isWindowsOSTypeSelected() const;
#endif /* VBOX_WITH_VIDEOHWACCEL */

    /** Defines whether HW virtualization extension is enabled. */
    void setHWVirtExEnabled(bool fEnabled);

protected:

    /** Returns whether the page content was changed. */
    virtual bool changed() const /* override */;

    /** Loads data into the cache from the corresponding external object(s).
      * @note This task COULD be performed in other than GUI thread. */
    virtual void loadToCacheFrom(QVariant &data) /* override */;
    /** Loads data into the corresponding widgets from the cache,
      * @note This task SHOULD be performed in GUI thread only! */
    virtual void getFromCache() /* override */;

    /** Saves the data from the corresponding widgets into the cache,
      * @note This task SHOULD be performed in GUI thread only! */
    virtual void putToCache() /* override */;
    /** Save data from the cache into the corresponding external object(s).
      * @note This task COULD be performed in other than GUI thread. */
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

    /** Marks the encryption cipher as changed. */
    void sltMarkEncryptionCipherChanged() { m_fEncryptionCipherChanged = true; }
    /** Marks the encryption cipher and password as changed. */
    void sltMarkEncryptionPasswordChanged() { m_fEncryptionCipherChanged = true; m_fEncryptionPasswordChanged = true; }

private:

    /** Prepares all. */
    void prepare();
    /** Prepares 'Basic' tab. */
    void prepareTabBasic();
    /** Prepares 'Advanced' tab. */
    void prepareTabAdvanced();
    /** Prepares 'Description' tab. */
    void prepareTabDescription();
    /** Prepares 'Encryption' tab. */
    void prepareTabEncryption();
    /** Prepares connections. */
    void prepareConnections();
    /** Cleanups all. */
    void cleanup();

    /** Saves existing general data from the cache. */
    bool saveGeneralData();
    /** Saves existing 'Basic' data from the cache. */
    bool saveBasicData();
    /** Saves existing 'Advanced' data from the cache. */
    bool saveAdvancedData();
    /** Saves existing 'Description' data from the cache. */
    bool saveDescriptionData();
    /** Saves existing 'Encryption' data from the cache. */
    bool saveEncryptionData();

    /** Holds whether HW virtualization extension is enabled. */
    bool  m_fHWVirtExEnabled;

    /** Holds whether the encryption cipher was changed.
      * We are holding that argument here because we do not know
      * the old <i>cipher</i> for sure to compare the new one with. */
    bool  m_fEncryptionCipherChanged;
    /** Holds whether the encryption password was changed.
      * We are holding that argument here because we do not know
      * the old <i>password</i> at all to compare the new one with. */
    bool  m_fEncryptionPasswordChanged;

    /** Holds the hard-coded encryption cipher list.
      * We are hard-coding it because there is no place we can get it from. */
    QStringList  m_encryptionCiphers;

    /** Holds the page data cache instance. */
    UISettingsCacheMachineGeneral *m_pCache;
};

#endif /* !___UIMachineSettingsGeneral_h___ */

