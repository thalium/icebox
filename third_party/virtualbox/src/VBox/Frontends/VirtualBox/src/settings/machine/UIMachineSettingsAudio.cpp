/* $Id: UIMachineSettingsAudio.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsAudio class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* GUI includes: */
# include "UIConverter.h"
# include "UIMachineSettingsAudio.h"
# include "UIErrorString.h"

/* COM includes: */
# include "CAudioAdapter.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Machine settings: Audio page data structure. */
struct UIDataSettingsMachineAudio
{
    /** Constructs data. */
    UIDataSettingsMachineAudio()
        : m_fAudioEnabled(false)
        , m_audioDriverType(KAudioDriverType_Null)
        , m_audioControllerType(KAudioControllerType_AC97)
        , m_fAudioOutputEnabled(false)
        , m_fAudioInputEnabled(false)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineAudio &other) const
    {
        return true
               && (m_fAudioEnabled == other.m_fAudioEnabled)
               && (m_audioDriverType == other.m_audioDriverType)
               && (m_audioControllerType == other.m_audioControllerType)
               && (m_fAudioOutputEnabled == other.m_fAudioOutputEnabled)
               && (m_fAudioInputEnabled == other.m_fAudioInputEnabled)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineAudio &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineAudio &other) const { return !equal(other); }

    /** Holds whether the audio is enabled. */
    bool                  m_fAudioEnabled;
    /** Holds the audio driver type. */
    KAudioDriverType      m_audioDriverType;
    /** Holds the audio controller type. */
    KAudioControllerType  m_audioControllerType;
    /** Holds whether the audio output is enabled. */
    bool                  m_fAudioOutputEnabled;
    /** Holds whether the audio input is enabled. */
    bool                  m_fAudioInputEnabled;
};


UIMachineSettingsAudio::UIMachineSettingsAudio()
    : m_pCache(0)
{
    /* Prepare: */
    prepare();
}

UIMachineSettingsAudio::~UIMachineSettingsAudio()
{
    /* Cleanup: */
    cleanup();
}

bool UIMachineSettingsAudio::changed() const
{
    return m_pCache->wasChanged();
}

void UIMachineSettingsAudio::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old audio data: */
    UIDataSettingsMachineAudio oldAudioData;

    /* Check whether adapter is valid: */
    const CAudioAdapter &comAdapter = m_machine.GetAudioAdapter();
    if (!comAdapter.isNull())
    {
        /* Gather old audio data: */
        oldAudioData.m_fAudioEnabled = comAdapter.GetEnabled();
        oldAudioData.m_audioDriverType = comAdapter.GetAudioDriver();
        oldAudioData.m_audioControllerType = comAdapter.GetAudioController();
        oldAudioData.m_fAudioOutputEnabled = comAdapter.GetEnabledOut();
        oldAudioData.m_fAudioInputEnabled = comAdapter.GetEnabledIn();
    }

    /* Cache old audio data: */
    m_pCache->cacheInitialData(oldAudioData);

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsAudio::getFromCache()
{
    /* Get old audio data from the cache: */
    const UIDataSettingsMachineAudio &oldAudioData = m_pCache->base();

    /* Load old audio data from the cache: */
    m_pCheckBoxAudio->setChecked(oldAudioData.m_fAudioEnabled);
    m_pComboAudioDriver->setCurrentIndex(m_pComboAudioDriver->findData((int)oldAudioData.m_audioDriverType));
    m_pComboAudioController->setCurrentIndex(m_pComboAudioController->findData((int)oldAudioData.m_audioControllerType));
    m_pCheckBoxAudioOutput->setChecked(oldAudioData.m_fAudioOutputEnabled);
    m_pCheckBoxAudioInput->setChecked(oldAudioData.m_fAudioInputEnabled);

    /* Polish page finally: */
    polishPage();
}

void UIMachineSettingsAudio::putToCache()
{
    /* Prepare new audio data: */
    UIDataSettingsMachineAudio newAudioData;

    /* Gather new audio data: */
    newAudioData.m_fAudioEnabled = m_pCheckBoxAudio->isChecked();
    newAudioData.m_audioDriverType = static_cast<KAudioDriverType>(m_pComboAudioDriver->itemData(m_pComboAudioDriver->currentIndex()).toInt());
    newAudioData.m_audioControllerType = static_cast<KAudioControllerType>(m_pComboAudioController->itemData(m_pComboAudioController->currentIndex()).toInt());
    newAudioData.m_fAudioOutputEnabled = m_pCheckBoxAudioOutput->isChecked();
    newAudioData.m_fAudioInputEnabled = m_pCheckBoxAudioInput->isChecked();

    /* Cache new audio data: */
    m_pCache->cacheCurrentData(newAudioData);
}

void UIMachineSettingsAudio::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Update audio data and failing state: */
    setFailed(!saveAudioData());

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsAudio::retranslateUi()
{
    /* Translate generated strings: */
    Ui::UIMachineSettingsAudio::retranslateUi(this);

    /* Translate audio-driver combo.
     * Make sure this order corresponds the same in prepare(): */
    int iIndex = -1;
    m_pComboAudioDriver->setItemText(++iIndex, gpConverter->toString(KAudioDriverType_Null));
#ifdef Q_OS_WIN
    m_pComboAudioDriver->setItemText(++iIndex, gpConverter->toString(KAudioDriverType_DirectSound));
# ifdef VBOX_WITH_WINMM
    m_pComboAudioDriver->setItemText(++iIndex, gpConverter->toString(KAudioDriverType_WinMM));
# endif
#endif
#ifdef VBOX_WITH_AUDIO_OSS
    m_pComboAudioDriver->setItemText(++iIndex, gpConverter->toString(KAudioDriverType_OSS));
#endif
#ifdef VBOX_WITH_AUDIO_ALSA
    m_pComboAudioDriver->setItemText(++iIndex, gpConverter->toString(KAudioDriverType_ALSA));
#endif
#ifdef VBOX_WITH_AUDIO_PULSE
    m_pComboAudioDriver->setItemText(++iIndex, gpConverter->toString(KAudioDriverType_Pulse));
#endif
#ifdef Q_OS_MACX
    m_pComboAudioDriver->setItemText(++iIndex, gpConverter->toString(KAudioDriverType_CoreAudio));
#endif

    /* Translate audio-controller combo.
     * Make sure this order corresponds the same in prepare(): */
    iIndex = -1;
    m_pComboAudioController->setItemText(++iIndex, gpConverter->toString(KAudioControllerType_HDA));
    m_pComboAudioController->setItemText(++iIndex, gpConverter->toString(KAudioControllerType_AC97));
    m_pComboAudioController->setItemText(++iIndex, gpConverter->toString(KAudioControllerType_SB16));
}

void UIMachineSettingsAudio::polishPage()
{
    /* Polish audio page availability: */
    m_pCheckBoxAudio->setEnabled(isMachineOffline());
    m_pLabelAudioDriver->setEnabled(isMachineOffline());
    m_pComboAudioDriver->setEnabled(isMachineOffline());
    m_pLabelAudioController->setEnabled(isMachineOffline());
    m_pComboAudioController->setEnabled(isMachineOffline());
    m_pLabelAudioExtended->setEnabled(isMachineInValidMode());
    m_pCheckBoxAudioOutput->setEnabled(isMachineInValidMode());
    m_pCheckBoxAudioInput->setEnabled(isMachineInValidMode());
    m_pContainerAudioSubOptions->setEnabled(m_pCheckBoxAudio->isChecked());
}

void UIMachineSettingsAudio::prepare()
{
    /* Apply UI decorations: */
    Ui::UIMachineSettingsAudio::setupUi(this);

    /* Prepare cache: */
    m_pCache = new UISettingsCacheMachineAudio;
    AssertPtrReturnVoid(m_pCache);

    /* Layout created in the .ui file. */
    {
        /* Audio-driver combo-box created in the .ui file. */
        AssertPtrReturnVoid(m_pComboAudioDriver);
        {
            /* Configure combo-box.
             * Make sure this order corresponds the same in retranslateUi(): */
            int iIndex = -1;
            m_pComboAudioDriver->insertItem(++iIndex, "", KAudioDriverType_Null);
#ifdef Q_OS_WIN
            m_pComboAudioDriver->insertItem(++iIndex, "", KAudioDriverType_DirectSound);
# ifdef VBOX_WITH_WINMM
            m_pComboAudioDriver->insertItem(++iIndex, "", KAudioDriverType_WinMM);
# endif
#endif
#ifdef VBOX_WITH_AUDIO_OSS
            m_pComboAudioDriver->insertItem(++iIndex, "", KAudioDriverType_OSS);
#endif
#ifdef VBOX_WITH_AUDIO_ALSA
            m_pComboAudioDriver->insertItem(++iIndex, "", KAudioDriverType_ALSA);
#endif
#ifdef VBOX_WITH_AUDIO_PULSE
            m_pComboAudioDriver->insertItem(++iIndex, "", KAudioDriverType_Pulse);
#endif
#ifdef Q_OS_MACX
            m_pComboAudioDriver->insertItem(++iIndex, "", KAudioDriverType_CoreAudio);
#endif
        }

        /* Audio-controller combo-box created in the .ui file. */
        AssertPtrReturnVoid(m_pComboAudioController);
        {
            /* Configure combo-box.
             * Make sure this order corresponds the same in retranslateUi(): */
            int iIndex = -1;
            m_pComboAudioController->insertItem(++iIndex, "", KAudioControllerType_HDA);
            m_pComboAudioController->insertItem(++iIndex, "", KAudioControllerType_AC97);
            m_pComboAudioController->insertItem(++iIndex, "", KAudioControllerType_SB16);
        }
    }

    /* Apply language settings: */
    retranslateUi();
}

void UIMachineSettingsAudio::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

bool UIMachineSettingsAudio::saveAudioData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save audio settings from the cache: */
    if (fSuccess && isMachineInValidMode() && m_pCache->wasChanged())
    {
        /* Get old audio data from the cache: */
        const UIDataSettingsMachineAudio &oldAudioData = m_pCache->base();
        /* Get new audio data from the cache: */
        const UIDataSettingsMachineAudio &newAudioData = m_pCache->data();

        /* Get audio adapter for further activities: */
        CAudioAdapter comAdapter = m_machine.GetAudioAdapter();
        fSuccess = m_machine.isOk() && comAdapter.isNotNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            /* Save whether audio is enabled: */
            if (fSuccess && isMachineOffline() && newAudioData.m_fAudioEnabled != oldAudioData.m_fAudioEnabled)
            {
                comAdapter.SetEnabled(newAudioData.m_fAudioEnabled);
                fSuccess = comAdapter.isOk();
            }
            /* Save audio driver type: */
            if (fSuccess && isMachineOffline() && newAudioData.m_audioDriverType != oldAudioData.m_audioDriverType)
            {
                comAdapter.SetAudioDriver(newAudioData.m_audioDriverType);
                fSuccess = comAdapter.isOk();
            }
            /* Save audio controller type: */
            if (fSuccess && isMachineOffline() && newAudioData.m_audioControllerType != oldAudioData.m_audioControllerType)
            {
                comAdapter.SetAudioController(newAudioData.m_audioControllerType);
                fSuccess = comAdapter.isOk();
            }
            /* Save whether audio output is enabled: */
            if (fSuccess && isMachineInValidMode() && newAudioData.m_fAudioOutputEnabled != oldAudioData.m_fAudioOutputEnabled)
            {
                comAdapter.SetEnabledOut(newAudioData.m_fAudioOutputEnabled);
                fSuccess = comAdapter.isOk();
            }
            /* Save whether audio input is enabled: */
            if (fSuccess && isMachineInValidMode() && newAudioData.m_fAudioInputEnabled != oldAudioData.m_fAudioInputEnabled)
            {
                comAdapter.SetEnabledIn(newAudioData.m_fAudioInputEnabled);
                fSuccess = comAdapter.isOk();
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comAdapter));
        }
    }
    /* Return result: */
    return fSuccess;
}

