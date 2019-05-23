/* $Id: UIMachineSettingsDisplay.cpp $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsDisplay class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* GUI includes: */
# include "QIWidgetValidator.h"
# include "UIConverter.h"
# include "UIDesktopWidgetWatchdog.h"
# include "UIExtraDataManager.h"
# include "UIMachineSettingsDisplay.h"
# include "UIErrorString.h"
# include "VBoxGlobal.h"

/* COM includes: */
# include "CExtPack.h"
# include "CExtPackManager.h"
# include "CVRDEServer.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Machine settings: Display page data structure. */
struct UIDataSettingsMachineDisplay
{
    /** Constructs data. */
    UIDataSettingsMachineDisplay()
        : m_iCurrentVRAM(0)
        , m_cGuestScreenCount(0)
        , m_dScaleFactor(1.0)
#ifdef VBOX_WS_MAC
        , m_fUseUnscaledHiDPIOutput(false)
#endif /* VBOX_WS_MAC */
        , m_f3dAccelerationEnabled(false)
#ifdef VBOX_WITH_VIDEOHWACCEL
        , m_f2dAccelerationEnabled(false)
#endif /* VBOX_WITH_VIDEOHWACCEL */
        , m_fRemoteDisplayServerSupported(false)
        , m_fRemoteDisplayServerEnabled(false)
        , m_strRemoteDisplayPort(QString())
        , m_remoteDisplayAuthType(KAuthType_Null)
        , m_uRemoteDisplayTimeout(0)
        , m_fRemoteDisplayMultiConnAllowed(false)
        , m_fVideoCaptureEnabled(false)
        , m_strVideoCaptureFolder(QString())
        , m_strVideoCaptureFilePath(QString())
        , m_iVideoCaptureFrameWidth(0)
        , m_iVideoCaptureFrameHeight(0)
        , m_iVideoCaptureFrameRate(0)
        , m_iVideoCaptureBitRate(0)
        , m_strVideoCaptureOptions(QString())
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineDisplay &other) const
    {
        return true
               && (m_iCurrentVRAM == other.m_iCurrentVRAM)
               && (m_cGuestScreenCount == other.m_cGuestScreenCount)
               && (m_dScaleFactor == other.m_dScaleFactor)
#ifdef VBOX_WS_MAC
               && (m_fUseUnscaledHiDPIOutput == other.m_fUseUnscaledHiDPIOutput)
#endif /* VBOX_WS_MAC */
               && (m_f3dAccelerationEnabled == other.m_f3dAccelerationEnabled)
#ifdef VBOX_WITH_VIDEOHWACCEL
               && (m_f2dAccelerationEnabled == other.m_f2dAccelerationEnabled)
#endif /* VBOX_WITH_VIDEOHWACCEL */
               && (m_fRemoteDisplayServerSupported == other.m_fRemoteDisplayServerSupported)
               && (m_fRemoteDisplayServerEnabled == other.m_fRemoteDisplayServerEnabled)
               && (m_strRemoteDisplayPort == other.m_strRemoteDisplayPort)
               && (m_remoteDisplayAuthType == other.m_remoteDisplayAuthType)
               && (m_uRemoteDisplayTimeout == other.m_uRemoteDisplayTimeout)
               && (m_fRemoteDisplayMultiConnAllowed == other.m_fRemoteDisplayMultiConnAllowed)
               && (m_fVideoCaptureEnabled == other.m_fVideoCaptureEnabled)
               && (m_strVideoCaptureFilePath == other.m_strVideoCaptureFilePath)
               && (m_iVideoCaptureFrameWidth == other.m_iVideoCaptureFrameWidth)
               && (m_iVideoCaptureFrameHeight == other.m_iVideoCaptureFrameHeight)
               && (m_iVideoCaptureFrameRate == other.m_iVideoCaptureFrameRate)
               && (m_iVideoCaptureBitRate == other.m_iVideoCaptureBitRate)
               && (m_screens == other.m_screens)
               && (m_strVideoCaptureOptions == other.m_strVideoCaptureOptions)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineDisplay &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineDisplay &other) const { return !equal(other); }

    /** Video Capture Options. */
    enum VideoCaptureOption
    {
        VideoCaptureOption_Unknown,
        VideoCaptureOption_AC,
    };

    /** Returns enum value corresponding to passed @a strKey. */
    static VideoCaptureOption toVideoCaptureOptionKey(const QString &strKey)
    {
        /* Compare case-sensitive: */
        QMap<QString, VideoCaptureOption> keys;
        keys["ac_enabled"] = VideoCaptureOption_AC;
        /* Return known value or VideoCaptureOption_Unknown otherwise: */
        return keys.value(strKey, VideoCaptureOption_Unknown);
    }

    /** Returns bool value corresponding to passed @a strValue. */
    static bool toVideoCaptureOptionValue(const QString &strValue)
    {
        /* Compare case-sensitive: */
        QMap<QString, bool> values;
        values["true"] = true;
        values["false"] = false;
        /* Return known value or true otherwise: */
        return values.value(strValue, false);
    }

    /** Returns string representation for passed enum @a enmKey. */
    static QString fromVideoCaptureOptionKey(VideoCaptureOption enmKey)
    {
        /* Compare case-sensitive: */
        QMap<VideoCaptureOption, QString> values;
        values[VideoCaptureOption_AC] = "ac_enabled";
        /* Return known value or QString() otherwise: */
        return values.value(enmKey);
    }

    /** Returns string representation for passed bool @a fValue. */
    static QString fromVideoCaptureOptionValue(bool fValue)
    {
        /* Compare case-sensitive: */
        QMap<bool, QString> values;
        values[true] = "true";
        values[false] = "false";
        /* Return known value or "false" otherwise: */
        return values.value(fValue, "false");
    }

    /** Parses Video Capture Options. */
    static void parseVideoCaptureOptions(const QString &strOptions,
                                         QList<VideoCaptureOption> &outKeys,
                                         QList<bool> &outValues)
    {
        outKeys.clear();
        outValues.clear();
        const QStringList aPairs = strOptions.split(',');
        foreach (const QString &strPair, aPairs)
        {
            const QStringList aPair = strPair.split('=');
            const VideoCaptureOption enmKey = toVideoCaptureOptionKey(aPair.value(0));
            const bool fValue = toVideoCaptureOptionValue(aPair.value(1));
            if (enmKey == VideoCaptureOption_Unknown)
                continue;
            outKeys << enmKey;
            outValues << fValue;
        }
    }

    /** Serializes Video Capture Options. */
    static void serializeVideoCaptureOptions(const QList<VideoCaptureOption> &inKeys,
                                             const QList<bool> &inValues,
                                             QString &strOptions)
    {
        QStringList aPairs;
        for (int i = 0; i < inKeys.size(); ++i)
        {
            QStringList aPair;
            aPair << fromVideoCaptureOptionKey(inKeys.value(i));
            aPair << fromVideoCaptureOptionValue(inValues.value(i));
            aPairs << aPair.join('=');
        }
        strOptions = aPairs.join(',');
    }

    /** Returns whether passed Video Capture @a enmOption is enabled. */
    static bool isVideoCaptureOptionEnabled(const QString &strOptions,
                                            VideoCaptureOption enmOption)
    {
        QList<VideoCaptureOption> aKeys;
        QList<bool> aValues;
        parseVideoCaptureOptions(strOptions, aKeys, aValues);
        int iIndex = aKeys.indexOf(enmOption);
        if (iIndex == -1)
            return false; /* If option is missing, assume disabled (false). */
        return aValues.value(iIndex);
    }

    /** Defines whether passed Video Capture @a enmOption is @a fEnabled. */
    static QString setVideoCaptureOptionEnabled(const QString &strOptions,
                                                VideoCaptureOption enmOption,
                                                bool fEnabled)
    {
        QList<VideoCaptureOption> aKeys;
        QList<bool> aValues;
        parseVideoCaptureOptions(strOptions, aKeys, aValues);
        int iIndex = aKeys.indexOf(enmOption);
        if (iIndex == -1)
        {
            aKeys << enmOption;
            aValues << fEnabled;
        }
        else
        {
            aValues[iIndex] = fEnabled;
        }
        QString strResult;
        serializeVideoCaptureOptions(aKeys, aValues, strResult);
        return strResult;
    }

    /** Holds the video RAM amount. */
    int     m_iCurrentVRAM;
    /** Holds the guest screen count. */
    int     m_cGuestScreenCount;
    /** Holds the guest screen scale-factor. */
    double  m_dScaleFactor;
#ifdef VBOX_WS_MAC
    /** Holds whether automatic Retina scaling is disabled. */
    bool    m_fUseUnscaledHiDPIOutput;
#endif /* VBOX_WS_MAC */
    /** Holds whether the 3D acceleration is enabled. */
    bool    m_f3dAccelerationEnabled;
#ifdef VBOX_WITH_VIDEOHWACCEL
    /** Holds whether the 2D video acceleration is enabled. */
    bool    m_f2dAccelerationEnabled;
#endif /* VBOX_WITH_VIDEOHWACCEL */

    /** Holds whether the remote display server is supported. */
    bool       m_fRemoteDisplayServerSupported;
    /** Holds whether the remote display server is enabled. */
    bool       m_fRemoteDisplayServerEnabled;
    /** Holds the remote display server port. */
    QString    m_strRemoteDisplayPort;
    /** Holds the remote display server auth type. */
    KAuthType  m_remoteDisplayAuthType;
    /** Holds the remote display server timeout. */
    ulong      m_uRemoteDisplayTimeout;
    /** Holds whether the remote display server allows multiple connections. */
    bool       m_fRemoteDisplayMultiConnAllowed;

    /** Holds whether the video capture is enabled. */
    bool m_fVideoCaptureEnabled;
    /** Holds the video capture folder. */
    QString m_strVideoCaptureFolder;
    /** Holds the video capture file path. */
    QString m_strVideoCaptureFilePath;
    /** Holds the video capture frame width. */
    int m_iVideoCaptureFrameWidth;
    /** Holds the video capture frame height. */
    int m_iVideoCaptureFrameHeight;
    /** Holds the video capture frame rate. */
    int m_iVideoCaptureFrameRate;
    /** Holds the video capture bit rate. */
    int m_iVideoCaptureBitRate;
    /** Holds which of the guest screens should be captured. */
    QVector<BOOL> m_screens;
    /** Holds the video capture options. */
    QString m_strVideoCaptureOptions;
};


UIMachineSettingsDisplay::UIMachineSettingsDisplay()
    : m_iMinVRAM(0)
    , m_iMaxVRAM(0)
    , m_iMaxVRAMVisible(0)
    , m_iInitialVRAM(0)
#ifdef VBOX_WITH_VIDEOHWACCEL
    , m_f2DVideoAccelerationSupported(false)
#endif /* VBOX_WITH_VIDEOHWACCEL */
#ifdef VBOX_WITH_CRHGSMI
    , m_fWddmModeSupported(false)
#endif /* VBOX_WITH_CRHGSMI */
    , m_pCache(0)
{
    /* Prepare: */
    prepare();
}

UIMachineSettingsDisplay::~UIMachineSettingsDisplay()
{
    /* Cleanup: */
    cleanup();
}

void UIMachineSettingsDisplay::setGuestOSType(CGuestOSType comGuestOSType)
{
    /* Check if guest os type changed: */
    if (m_comGuestOSType == comGuestOSType)
        return;

    /* Remember new guest os type: */
    m_comGuestOSType = comGuestOSType;

#ifdef VBOX_WITH_VIDEOHWACCEL
    /* Check if 2D video acceleration supported by the guest OS type: */
    const QString strGuestOSTypeFamily = m_comGuestOSType.GetFamilyId();
    m_f2DVideoAccelerationSupported = strGuestOSTypeFamily == "Windows";
#endif
#ifdef VBOX_WITH_CRHGSMI
    /* Check if WDDM mode supported by the guest OS type: */
    const QString strGuestOSTypeId = m_comGuestOSType.GetId();
    m_fWddmModeSupported = VBoxGlobal::isWddmCompatibleOsType(strGuestOSTypeId);
#endif

    /* Recheck video RAM requirement: */
    checkVRAMRequirements();

    /* Revalidate: */
    revalidate();
}

#ifdef VBOX_WITH_VIDEOHWACCEL
bool UIMachineSettingsDisplay::isAcceleration2DVideoSelected() const
{
    return m_pCheckbox2DVideo->isChecked();
}
#endif /* VBOX_WITH_VIDEOHWACCEL */

bool UIMachineSettingsDisplay::changed() const
{
    return m_pCache->wasChanged();
}

void UIMachineSettingsDisplay::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old display data: */
    UIDataSettingsMachineDisplay oldDisplayData;

    /* Gather old 'Screen' data: */
    oldDisplayData.m_iCurrentVRAM = m_machine.GetVRAMSize();
    oldDisplayData.m_cGuestScreenCount = m_machine.GetMonitorCount();
    oldDisplayData.m_dScaleFactor = gEDataManager->scaleFactor(m_machine.GetId());
#ifdef VBOX_WS_MAC
    oldDisplayData.m_fUseUnscaledHiDPIOutput = gEDataManager->useUnscaledHiDPIOutput(m_machine.GetId());
#endif
    oldDisplayData.m_f3dAccelerationEnabled = m_machine.GetAccelerate3DEnabled();
#ifdef VBOX_WITH_VIDEOHWACCEL
    oldDisplayData.m_f2dAccelerationEnabled = m_machine.GetAccelerate2DVideoEnabled();
#endif

    /* Check whether remote display server is valid: */
    const CVRDEServer &comServer = m_machine.GetVRDEServer();
    oldDisplayData.m_fRemoteDisplayServerSupported = !comServer.isNull();
    if (!comServer.isNull())
    {
        /* Gather old 'Remote Display' data: */
        oldDisplayData.m_fRemoteDisplayServerEnabled = comServer.GetEnabled();
        oldDisplayData.m_strRemoteDisplayPort = comServer.GetVRDEProperty("TCP/Ports");
        oldDisplayData.m_remoteDisplayAuthType = comServer.GetAuthType();
        oldDisplayData.m_uRemoteDisplayTimeout = comServer.GetAuthTimeout();
        oldDisplayData.m_fRemoteDisplayMultiConnAllowed = comServer.GetAllowMultiConnection();
    }

    /* Gather old 'Video Capture' data: */
    oldDisplayData.m_fVideoCaptureEnabled = m_machine.GetVideoCaptureEnabled();
    oldDisplayData.m_strVideoCaptureFolder = QFileInfo(m_machine.GetSettingsFilePath()).absolutePath();
    oldDisplayData.m_strVideoCaptureFilePath = m_machine.GetVideoCaptureFile();
    oldDisplayData.m_iVideoCaptureFrameWidth = m_machine.GetVideoCaptureWidth();
    oldDisplayData.m_iVideoCaptureFrameHeight = m_machine.GetVideoCaptureHeight();
    oldDisplayData.m_iVideoCaptureFrameRate = m_machine.GetVideoCaptureFPS();
    oldDisplayData.m_iVideoCaptureBitRate = m_machine.GetVideoCaptureRate();
    oldDisplayData.m_screens = m_machine.GetVideoCaptureScreens();
    oldDisplayData.m_strVideoCaptureOptions = m_machine.GetVideoCaptureOptions();

    /* Gather other old display data: */
    m_iInitialVRAM = RT_MIN(oldDisplayData.m_iCurrentVRAM, m_iMaxVRAM);

    /* Cache old display data: */
    m_pCache->cacheInitialData(oldDisplayData);

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsDisplay::getFromCache()
{
    /* Get old display data from the cache: */
    const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();

    /* Load old 'Screen' data from the cache: */
    m_pEditorVideoScreenCount->setValue(oldDisplayData.m_cGuestScreenCount);
    m_pEditorGuestScreenScale->setValue((int)(oldDisplayData.m_dScaleFactor * 100));
#ifdef VBOX_WS_MAC
    m_pCheckBoxUnscaledHiDPIOutput->setChecked(oldDisplayData.m_fUseUnscaledHiDPIOutput);
#endif
    m_pCheckbox3D->setChecked(oldDisplayData.m_f3dAccelerationEnabled);
#ifdef VBOX_WITH_VIDEOHWACCEL
    m_pCheckbox2DVideo->setChecked(oldDisplayData.m_f2dAccelerationEnabled);
#endif
    // Should be the last one for this tab:
    m_pEditorVideoMemorySize->setValue(oldDisplayData.m_iCurrentVRAM);

    /* If remote display server is supported: */
    if (oldDisplayData.m_fRemoteDisplayServerSupported)
    {
        /* Load old 'Remote Display' data from the cache: */
        m_pCheckboxRemoteDisplay->setChecked(oldDisplayData.m_fRemoteDisplayServerEnabled);
        m_pEditorRemoteDisplayPort->setText(oldDisplayData.m_strRemoteDisplayPort);
        m_pComboRemoteDisplayAuthMethod->setCurrentIndex(m_pComboRemoteDisplayAuthMethod->findText(gpConverter->toString(oldDisplayData.m_remoteDisplayAuthType)));
        m_pEditorRemoteDisplayTimeout->setText(QString::number(oldDisplayData.m_uRemoteDisplayTimeout));
        m_pCheckboxMultipleConn->setChecked(oldDisplayData.m_fRemoteDisplayMultiConnAllowed);
    }

    /* Load old 'Video Capture' data from the cache: */
    m_pCheckboxVideoCapture->setChecked(oldDisplayData.m_fVideoCaptureEnabled);
    m_pEditorVideoCapturePath->setHomeDir(oldDisplayData.m_strVideoCaptureFolder);
    m_pEditorVideoCapturePath->setPath(oldDisplayData.m_strVideoCaptureFilePath);
    m_pEditorVideoCaptureWidth->setValue(oldDisplayData.m_iVideoCaptureFrameWidth);
    m_pEditorVideoCaptureHeight->setValue(oldDisplayData.m_iVideoCaptureFrameHeight);
    m_pEditorVideoCaptureFrameRate->setValue(oldDisplayData.m_iVideoCaptureFrameRate);
    m_pEditorVideoCaptureBitRate->setValue(oldDisplayData.m_iVideoCaptureBitRate);
    m_pScrollerVideoCaptureScreens->setValue(oldDisplayData.m_screens);
    m_pCheckBoxVideoCaptureWithAudio->setChecked(UIDataSettingsMachineDisplay::isVideoCaptureOptionEnabled(oldDisplayData.m_strVideoCaptureOptions,
                                                                                                           UIDataSettingsMachineDisplay::VideoCaptureOption_AC));

    /* Polish page finally: */
    polishPage();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsDisplay::putToCache()
{
    /* Prepare new display data: */
    UIDataSettingsMachineDisplay newDisplayData;

    /* Gather new 'Screen' data: */
    newDisplayData.m_iCurrentVRAM = m_pEditorVideoMemorySize->value();
    newDisplayData.m_cGuestScreenCount = m_pEditorVideoScreenCount->value();
    newDisplayData.m_dScaleFactor = (double)m_pEditorGuestScreenScale->value() / 100;
#ifdef VBOX_WS_MAC
    newDisplayData.m_fUseUnscaledHiDPIOutput = m_pCheckBoxUnscaledHiDPIOutput->isChecked();
#endif
    newDisplayData.m_f3dAccelerationEnabled = m_pCheckbox3D->isChecked();
#ifdef VBOX_WITH_VIDEOHWACCEL
    newDisplayData.m_f2dAccelerationEnabled = m_pCheckbox2DVideo->isChecked();
#endif

    /* If remote display server is supported: */
    newDisplayData.m_fRemoteDisplayServerSupported = m_pCache->base().m_fRemoteDisplayServerSupported;
    if (newDisplayData.m_fRemoteDisplayServerSupported)
    {
        /* Gather new 'Remote Display' data: */
        newDisplayData.m_fRemoteDisplayServerEnabled = m_pCheckboxRemoteDisplay->isChecked();
        newDisplayData.m_strRemoteDisplayPort = m_pEditorRemoteDisplayPort->text();
        newDisplayData.m_remoteDisplayAuthType = gpConverter->fromString<KAuthType>(m_pComboRemoteDisplayAuthMethod->currentText());
        newDisplayData.m_uRemoteDisplayTimeout = m_pEditorRemoteDisplayTimeout->text().toULong();
        newDisplayData.m_fRemoteDisplayMultiConnAllowed = m_pCheckboxMultipleConn->isChecked();
    }

    /* Gather new 'Video Capture' data: */
    newDisplayData.m_fVideoCaptureEnabled = m_pCheckboxVideoCapture->isChecked();
    newDisplayData.m_strVideoCaptureFolder = m_pCache->base().m_strVideoCaptureFolder;
    newDisplayData.m_strVideoCaptureFilePath = m_pEditorVideoCapturePath->path();
    newDisplayData.m_iVideoCaptureFrameWidth = m_pEditorVideoCaptureWidth->value();
    newDisplayData.m_iVideoCaptureFrameHeight = m_pEditorVideoCaptureHeight->value();
    newDisplayData.m_iVideoCaptureFrameRate = m_pEditorVideoCaptureFrameRate->value();
    newDisplayData.m_iVideoCaptureBitRate = m_pEditorVideoCaptureBitRate->value();
    newDisplayData.m_screens = m_pScrollerVideoCaptureScreens->value();
    newDisplayData.m_strVideoCaptureOptions = UIDataSettingsMachineDisplay::setVideoCaptureOptionEnabled(m_pCache->base().m_strVideoCaptureOptions,
                                                                                                         UIDataSettingsMachineDisplay::VideoCaptureOption_AC,
                                                                                                         m_pCheckBoxVideoCaptureWithAudio->isChecked());

    /* Cache new display data: */
    m_pCache->cacheCurrentData(newDisplayData);
}

void UIMachineSettingsDisplay::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Update display data and failing state: */
    setFailed(!saveDisplayData());

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

bool UIMachineSettingsDisplay::validate(QList<UIValidationMessage> &messages)
{
    /* Check if video RAM requirement changed first: */
    checkVRAMRequirements();

    /* Pass by default: */
    bool fPass = true;

    /* Screen tab: */
    {
        /* Prepare message: */
        UIValidationMessage message;
        message.first = VBoxGlobal::removeAccelMark(m_pTabWidget->tabText(0));

        /* 3D acceleration test: */
        if (m_pCheckbox3D->isChecked() && !vboxGlobal().is3DAvailable())
        {
            message.second << tr("The virtual machine is set up to use hardware graphics acceleration. "
                                 "However the host system does not currently provide this, "
                                 "so you will not be able to start the machine.");
        }

        /* Video RAM amount test: */
        if (shouldWeWarnAboutLowVRAM() && !m_comGuestOSType.isNull())
        {
            quint64 uNeedBytes = VBoxGlobal::requiredVideoMemory(m_comGuestOSType.GetId(), m_pEditorVideoScreenCount->value());

            /* Basic video RAM amount test: */
            if ((quint64)m_pEditorVideoMemorySize->value() * _1M < uNeedBytes)
            {
                message.second << tr("The virtual machine is currently assigned less than <b>%1</b> of video memory "
                                     "which is the minimum amount required to switch to full-screen or seamless mode.")
                                     .arg(vboxGlobal().formatSize(uNeedBytes, 0, FormatSize_RoundUp));
            }
#ifdef VBOX_WITH_VIDEOHWACCEL
            /* 2D acceleration video RAM amount test: */
            else if (m_pCheckbox2DVideo->isChecked() && m_f2DVideoAccelerationSupported)
            {
                uNeedBytes += VBoxGlobal::required2DOffscreenVideoMemory();
                if ((quint64)m_pEditorVideoMemorySize->value() * _1M < uNeedBytes)
                {
                    message.second << tr("The virtual machine is currently assigned less than <b>%1</b> of video memory "
                                         "which is the minimum amount required for High Definition Video to be played efficiently.")
                                         .arg(vboxGlobal().formatSize(uNeedBytes, 0, FormatSize_RoundUp));
                }
            }
#endif /* VBOX_WITH_VIDEOHWACCEL */
#ifdef VBOX_WITH_CRHGSMI
            /* 3D acceleration video RAM amount test: */
            else if (m_pCheckbox3D->isChecked() && m_fWddmModeSupported)
            {
                uNeedBytes = qMax(uNeedBytes, (quint64) 128 * _1M);
                if ((quint64)m_pEditorVideoMemorySize->value() * _1M < uNeedBytes)
                {
                    message.second << tr("The virtual machine is set up to use hardware graphics acceleration "
                                         "and the operating system hint is set to Windows Vista or later. "
                                         "For best performance you should set the machine's video memory to at least <b>%1</b>.")
                                         .arg(vboxGlobal().formatSize(uNeedBytes, 0, FormatSize_RoundUp));
                }
            }
#endif /* VBOX_WITH_CRHGSMI */
        }

#ifdef VBOX_WITH_VIDEOHWACCEL
        /* 2D video acceleration is available for Windows guests only: */
        if (m_pCheckbox2DVideo->isChecked() && !m_f2DVideoAccelerationSupported)
        {
            message.second << tr("The virtual machine is set up to use Video Stream Acceleration. "
                                 "As this feature only works with Windows guest systems it will be disabled.");
        }
#endif /* VBOX_WITH_VIDEOHWACCEL */

        /* Serialize message: */
        if (!message.second.isEmpty())
            messages << message;
    }

    /* Remote Display tab: */
    {
        /* Prepare message: */
        UIValidationMessage message;
        message.first = VBoxGlobal::removeAccelMark(m_pTabWidget->tabText(1));

#ifdef VBOX_WITH_EXTPACK
        /* VRDE Extension Pack presence test: */
        CExtPack extPack = vboxGlobal().virtualBox().GetExtensionPackManager().Find(GUI_ExtPackName);
        if (m_pCheckboxRemoteDisplay->isChecked() && (extPack.isNull() || !extPack.GetUsable()))
        {
            message.second << tr("Remote Display is currently enabled for this virtual machine. "
                                 "However, this requires the <i>%1</i> to be installed. "
                                 "Please install the Extension Pack from the VirtualBox download site as "
                                 "otherwise your VM will be started with Remote Display disabled.")
                                 .arg(GUI_ExtPackName);
        }
#endif /* VBOX_WITH_EXTPACK */

        /* Check VRDE server port: */
        if (m_pEditorRemoteDisplayPort->text().trimmed().isEmpty())
        {
            message.second << tr("The VRDE server port value is not currently specified.");
            fPass = false;
        }

        /* Check VRDE server timeout: */
        if (m_pEditorRemoteDisplayTimeout->text().trimmed().isEmpty())
        {
            message.second << tr("The VRDE authentication timeout value is not currently specified.");
            fPass = false;
        }

        /* Serialize message: */
        if (!message.second.isEmpty())
            messages << message;
    }

    /* Return result: */
    return fPass;
}

void UIMachineSettingsDisplay::setOrderAfter(QWidget *pWidget)
{
    /* Screen tab-order: */
    setTabOrder(pWidget, m_pTabWidget->focusProxy());
    setTabOrder(m_pTabWidget->focusProxy(), m_pSliderVideoMemorySize);
    setTabOrder(m_pSliderVideoMemorySize, m_pEditorVideoMemorySize);
    setTabOrder(m_pEditorVideoMemorySize, m_pSliderVideoScreenCount);
    setTabOrder(m_pSliderVideoScreenCount, m_pEditorVideoScreenCount);
    setTabOrder(m_pEditorVideoScreenCount, m_pSliderGuestScreenScale);
    setTabOrder(m_pSliderGuestScreenScale, m_pEditorGuestScreenScale);
    setTabOrder(m_pEditorGuestScreenScale, m_pCheckBoxUnscaledHiDPIOutput);
    setTabOrder(m_pCheckBoxUnscaledHiDPIOutput, m_pCheckbox3D);
#ifdef VBOX_WITH_VIDEOHWACCEL
    setTabOrder(m_pCheckbox3D, m_pCheckbox2DVideo);
    setTabOrder(m_pCheckbox2DVideo, m_pCheckboxRemoteDisplay);
#else /* VBOX_WITH_VIDEOHWACCEL */
    setTabOrder(m_pCheckbox3D, m_pCheckboxRemoteDisplay);
#endif /* !VBOX_WITH_VIDEOHWACCEL */

    /* Remote Display tab-order: */
    setTabOrder(m_pCheckboxRemoteDisplay, m_pEditorRemoteDisplayPort);
    setTabOrder(m_pEditorRemoteDisplayPort, m_pComboRemoteDisplayAuthMethod);
    setTabOrder(m_pComboRemoteDisplayAuthMethod, m_pEditorRemoteDisplayTimeout);
    setTabOrder(m_pEditorRemoteDisplayTimeout, m_pCheckboxMultipleConn);

    /* Video Capture tab-order: */
    setTabOrder(m_pCheckboxMultipleConn, m_pCheckboxVideoCapture);
    setTabOrder(m_pCheckboxVideoCapture, m_pEditorVideoCapturePath);
    setTabOrder(m_pEditorVideoCapturePath, m_pComboVideoCaptureSize);
    setTabOrder(m_pComboVideoCaptureSize, m_pEditorVideoCaptureWidth);
    setTabOrder(m_pEditorVideoCaptureWidth, m_pEditorVideoCaptureHeight);
    setTabOrder(m_pEditorVideoCaptureHeight, m_pSliderVideoCaptureFrameRate);
    setTabOrder(m_pSliderVideoCaptureFrameRate, m_pEditorVideoCaptureFrameRate);
    setTabOrder(m_pEditorVideoCaptureFrameRate, m_pSliderVideoCaptureQuality);
    setTabOrder(m_pSliderVideoCaptureQuality, m_pEditorVideoCaptureBitRate);
}

void UIMachineSettingsDisplay::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIMachineSettingsDisplay::retranslateUi(this);

    /* Screen stuff: */
    CSystemProperties sys = vboxGlobal().virtualBox().GetSystemProperties();
    m_pEditorVideoMemorySize->setSuffix(QString(" %1").arg(tr("MB")));
    m_pLabelVideoMemorySizeMin->setText(tr("%1 MB").arg(m_iMinVRAM));
    m_pLabelVideoMemorySizeMax->setText(tr("%1 MB").arg(m_iMaxVRAMVisible));
    m_pLabelVideoScreenCountMin->setText(QString::number(1));
    m_pLabelVideoScreenCountMax->setText(QString::number(qMin(sys.GetMaxGuestMonitors(), (ULONG)8)));
    m_pLabelGuestScreenScaleMin->setText(tr("%1%").arg(100));
    m_pLabelGuestScreenScaleMax->setText(tr("%1%").arg(200));

    /* Remote Display stuff: */
    m_pComboRemoteDisplayAuthMethod->setItemText(0, gpConverter->toString(KAuthType_Null));
    m_pComboRemoteDisplayAuthMethod->setItemText(1, gpConverter->toString(KAuthType_External));
    m_pComboRemoteDisplayAuthMethod->setItemText(2, gpConverter->toString(KAuthType_Guest));

    /* Video Capture stuff: */
    m_pEditorVideoCaptureFrameRate->setSuffix(QString(" %1").arg(tr("fps")));
    m_pEditorVideoCaptureBitRate->setSuffix(QString(" %1").arg(tr("kbps")));
    m_pComboVideoCaptureSize->setItemText(0, tr("User Defined"));
    m_pLabelVideoCaptureFrameRateMin->setText(tr("%1 fps").arg(m_pSliderVideoCaptureFrameRate->minimum()));
    m_pLabelVideoCaptureFrameRateMax->setText(tr("%1 fps").arg(m_pSliderVideoCaptureFrameRate->maximum()));
    m_pLabelVideoCaptureQualityMin->setText(tr("low", "quality"));
    m_pLabelVideoCaptureQualityMed->setText(tr("medium", "quality"));
    m_pLabelVideoCaptureQualityMax->setText(tr("high", "quality"));

    updateVideoCaptureFileSizeHint();
}

void UIMachineSettingsDisplay::polishPage()
{
    /* Get old display data from the cache: */
    const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();

    /* Polish 'Screen' availability: */
    m_pLabelVideoMemorySize->setEnabled(isMachineOffline());
    m_pSliderVideoMemorySize->setEnabled(isMachineOffline());
    m_pLabelVideoMemorySizeMin->setEnabled(isMachineOffline());
    m_pLabelVideoMemorySizeMax->setEnabled(isMachineOffline());
    m_pEditorVideoMemorySize->setEnabled(isMachineOffline());
    m_pLabelVideoScreenCount->setEnabled(isMachineOffline());
    m_pSliderVideoScreenCount->setEnabled(isMachineOffline());
    m_pLabelVideoScreenCountMin->setEnabled(isMachineOffline());
    m_pLabelVideoScreenCountMax->setEnabled(isMachineOffline());
    m_pEditorVideoScreenCount->setEnabled(isMachineOffline());
    m_pLabelGuestScreenScale->setEnabled(isMachineInValidMode());
    m_pSliderGuestScreenScale->setEnabled(isMachineInValidMode());
    m_pLabelGuestScreenScaleMin->setEnabled(isMachineInValidMode());
    m_pLabelGuestScreenScaleMax->setEnabled(isMachineInValidMode());
    m_pEditorGuestScreenScale->setEnabled(isMachineInValidMode());
#ifdef VBOX_WS_MAC
    m_pLabelHiDPI->setEnabled(isMachineInValidMode());
    m_pCheckBoxUnscaledHiDPIOutput->setEnabled(isMachineInValidMode());
#else /* !VBOX_WS_MAC */
    m_pLabelHiDPI->hide();
    m_pCheckBoxUnscaledHiDPIOutput->hide();
#endif /* !VBOX_WS_MAC */
    m_pLabelVideoOptions->setEnabled(isMachineOffline());
    m_pCheckbox3D->setEnabled(isMachineOffline());
#ifdef VBOX_WITH_VIDEOHWACCEL
    m_pCheckbox2DVideo->setEnabled(isMachineOffline() && VBoxGlobal::isAcceleration2DVideoAvailable());
#else /* !VBOX_WITH_VIDEOHWACCEL */
    m_pCheckbox2DVideo->hide();
#endif /* !VBOX_WITH_VIDEOHWACCEL */

    /* Polish 'Remote Display' availability: */
    m_pTabWidget->setTabEnabled(1, oldDisplayData.m_fRemoteDisplayServerSupported);
    m_pContainerRemoteDisplay->setEnabled(isMachineInValidMode());
    m_pContainerRemoteDisplayOptions->setEnabled(m_pCheckboxRemoteDisplay->isChecked());
    m_pLabelRemoteDisplayOptions->setEnabled(isMachineOffline() || isMachineSaved());
    m_pCheckboxMultipleConn->setEnabled(isMachineOffline() || isMachineSaved());

    /* Polish 'Video Capture' availability: */
    m_pContainerVideoCapture->setEnabled(isMachineInValidMode());
    sltHandleVideoCaptureCheckboxToggle();
}

void UIMachineSettingsDisplay::sltHandleVideoMemorySizeSliderChange()
{
    /* Apply proposed memory-size: */
    m_pEditorVideoMemorySize->blockSignals(true);
    m_pEditorVideoMemorySize->setValue(m_pSliderVideoMemorySize->value());
    m_pEditorVideoMemorySize->blockSignals(false);

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsDisplay::sltHandleVideoMemorySizeEditorChange()
{
    /* Apply proposed memory-size: */
    m_pSliderVideoMemorySize->blockSignals(true);
    m_pSliderVideoMemorySize->setValue(m_pEditorVideoMemorySize->value());
    m_pSliderVideoMemorySize->blockSignals(false);

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsDisplay::sltHandleGuestScreenCountSliderChange()
{
    /* Apply proposed screen-count: */
    m_pEditorVideoScreenCount->blockSignals(true);
    m_pEditorVideoScreenCount->setValue(m_pSliderVideoScreenCount->value());
    m_pEditorVideoScreenCount->blockSignals(false);

    /* Update Video RAM requirements: */
    checkVRAMRequirements();

    /* Update Video Capture tab screen count: */
    updateGuestScreenCount();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsDisplay::sltHandleGuestScreenCountEditorChange()
{
    /* Apply proposed screen-count: */
    m_pSliderVideoScreenCount->blockSignals(true);
    m_pSliderVideoScreenCount->setValue(m_pEditorVideoScreenCount->value());
    m_pSliderVideoScreenCount->blockSignals(false);

    /* Update Video RAM requirements: */
    checkVRAMRequirements();

    /* Update Video Capture tab screen count: */
    updateGuestScreenCount();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsDisplay::sltHandleGuestScreenScaleSliderChange()
{
    /* Apply proposed scale-factor: */
    m_pEditorGuestScreenScale->blockSignals(true);
    m_pEditorGuestScreenScale->setValue(m_pSliderGuestScreenScale->value());
    m_pEditorGuestScreenScale->blockSignals(false);
}

void UIMachineSettingsDisplay::sltHandleGuestScreenScaleEditorChange()
{
    /* Apply proposed scale-factor: */
    m_pSliderGuestScreenScale->blockSignals(true);
    m_pSliderGuestScreenScale->setValue(m_pEditorGuestScreenScale->value());
    m_pSliderGuestScreenScale->blockSignals(false);
}

void UIMachineSettingsDisplay::sltHandleVideoCaptureCheckboxToggle()
{
    /* Video Capture options should be enabled only if:
     * 1. Machine is in 'offline' or 'saved' state and check-box is checked,
     * 2. Machine is in 'online' state, check-box is checked, and video recording is *disabled* currently. */
    const bool fIsVideoCaptureOptionsEnabled = ((isMachineOffline() || isMachineSaved()) && m_pCheckboxVideoCapture->isChecked()) ||
                                               (isMachineOnline() && !m_pCache->base().m_fVideoCaptureEnabled && m_pCheckboxVideoCapture->isChecked());

    /* Video Capture Screens option should be enabled only if:
     * Machine is in *any* valid state and check-box is checked. */
    const bool fIsVideoCaptureScreenOptionEnabled = isMachineInValidMode() && m_pCheckboxVideoCapture->isChecked();

    m_pLabelVideoCapturePath->setEnabled(fIsVideoCaptureOptionsEnabled);
    m_pEditorVideoCapturePath->setEnabled(fIsVideoCaptureOptionsEnabled);

    m_pLabelVideoCaptureSize->setEnabled(fIsVideoCaptureOptionsEnabled);
    m_pComboVideoCaptureSize->setEnabled(fIsVideoCaptureOptionsEnabled);
    m_pEditorVideoCaptureWidth->setEnabled(fIsVideoCaptureOptionsEnabled);
    m_pEditorVideoCaptureHeight->setEnabled(fIsVideoCaptureOptionsEnabled);

    m_pLabelVideoCaptureFrameRate->setEnabled(fIsVideoCaptureOptionsEnabled);
    m_pContainerSliderVideoCaptureFrameRate->setEnabled(fIsVideoCaptureOptionsEnabled);
    m_pEditorVideoCaptureFrameRate->setEnabled(fIsVideoCaptureOptionsEnabled);

    m_pLabelVideoCaptureRate->setEnabled(fIsVideoCaptureOptionsEnabled);
    m_pContainerSliderVideoCaptureQuality->setEnabled(fIsVideoCaptureOptionsEnabled);
    m_pEditorVideoCaptureBitRate->setEnabled(fIsVideoCaptureOptionsEnabled);

    m_pLabelVideoCaptureExtended->setEnabled(fIsVideoCaptureOptionsEnabled);
    m_pCheckBoxVideoCaptureWithAudio->setEnabled(fIsVideoCaptureOptionsEnabled);

    m_pLabelVideoCaptureScreens->setEnabled(fIsVideoCaptureScreenOptionEnabled);
    m_pLabelVideoCaptureSizeHint->setEnabled(fIsVideoCaptureScreenOptionEnabled);
    m_pScrollerVideoCaptureScreens->setEnabled(fIsVideoCaptureScreenOptionEnabled);
}

void UIMachineSettingsDisplay::sltHandleVideoCaptureFrameSizeComboboxChange()
{
    /* Get the proposed size: */
    const int iCurrentIndex = m_pComboVideoCaptureSize->currentIndex();
    const QSize videoCaptureSize = m_pComboVideoCaptureSize->itemData(iCurrentIndex).toSize();

    /* Make sure its valid: */
    if (!videoCaptureSize.isValid())
        return;

    /* Apply proposed size: */
    m_pEditorVideoCaptureWidth->setValue(videoCaptureSize.width());
    m_pEditorVideoCaptureHeight->setValue(videoCaptureSize.height());
}

void UIMachineSettingsDisplay::sltHandleVideoCaptureFrameWidthEditorChange()
{
    /* Look for preset: */
    lookForCorrespondingFrameSizePreset();
    /* Update quality and bit-rate: */
    sltHandleVideoCaptureQualitySliderChange();
}

void UIMachineSettingsDisplay::sltHandleVideoCaptureFrameHeightEditorChange()
{
    /* Look for preset: */
    lookForCorrespondingFrameSizePreset();
    /* Update quality and bit-rate: */
    sltHandleVideoCaptureQualitySliderChange();
}

void UIMachineSettingsDisplay::sltHandleVideoCaptureFrameRateSliderChange()
{
    /* Apply proposed frame-rate: */
    m_pEditorVideoCaptureFrameRate->blockSignals(true);
    m_pEditorVideoCaptureFrameRate->setValue(m_pSliderVideoCaptureFrameRate->value());
    m_pEditorVideoCaptureFrameRate->blockSignals(false);
    /* Update quality and bit-rate: */
    sltHandleVideoCaptureQualitySliderChange();
}

void UIMachineSettingsDisplay::sltHandleVideoCaptureFrameRateEditorChange()
{
    /* Apply proposed frame-rate: */
    m_pSliderVideoCaptureFrameRate->blockSignals(true);
    m_pSliderVideoCaptureFrameRate->setValue(m_pEditorVideoCaptureFrameRate->value());
    m_pSliderVideoCaptureFrameRate->blockSignals(false);
    /* Update quality and bit-rate: */
    sltHandleVideoCaptureQualitySliderChange();
}

void UIMachineSettingsDisplay::sltHandleVideoCaptureQualitySliderChange()
{
    /* Calculate/apply proposed bit-rate: */
    m_pEditorVideoCaptureBitRate->blockSignals(true);
    m_pEditorVideoCaptureBitRate->setValue(calculateBitRate(m_pEditorVideoCaptureWidth->value(),
                                                            m_pEditorVideoCaptureHeight->value(),
                                                            m_pEditorVideoCaptureFrameRate->value(),
                                                            m_pSliderVideoCaptureQuality->value()));
    m_pEditorVideoCaptureBitRate->blockSignals(false);
    updateVideoCaptureFileSizeHint();
}

void UIMachineSettingsDisplay::sltHandleVideoCaptureBitRateEditorChange()
{
    /* Calculate/apply proposed quality: */
    m_pSliderVideoCaptureQuality->blockSignals(true);
    m_pSliderVideoCaptureQuality->setValue(calculateQuality(m_pEditorVideoCaptureWidth->value(),
                                                            m_pEditorVideoCaptureHeight->value(),
                                                            m_pEditorVideoCaptureFrameRate->value(),
                                                            m_pEditorVideoCaptureBitRate->value()));
    m_pSliderVideoCaptureQuality->blockSignals(false);
    updateVideoCaptureFileSizeHint();
}

void UIMachineSettingsDisplay::prepare()
{
    /* Apply UI decorations: */
    Ui::UIMachineSettingsDisplay::setupUi(this);

    /* Prepare cache: */
    m_pCache = new UISettingsCacheMachineDisplay;
    AssertPtrReturnVoid(m_pCache);

    /* Tree-widget created in the .ui file. */
    {
        /* Prepare 'Screen' tab: */
        prepareTabScreen();
        /* Prepare 'Remote Display' tab: */
        prepareTabRemoteDisplay();
        /* Prepare 'Video Capture' tab: */
        prepareTabVideoCapture();
        /* Prepare connections: */
        prepareConnections();
    }

    /* Apply language settings: */
    retranslateUi();
}

void UIMachineSettingsDisplay::prepareTabScreen()
{
    /* Prepare common variables: */
    const CSystemProperties sys = vboxGlobal().virtualBox().GetSystemProperties();
    m_iMinVRAM = sys.GetMinGuestVRAM();
    m_iMaxVRAM = sys.GetMaxGuestVRAM();
    m_iMaxVRAMVisible = m_iMaxVRAM;

    /* Tab and it's layout created in the .ui file. */
    {
        /* Memory-size slider created in the .ui file. */
        AssertPtrReturnVoid(m_pSliderVideoMemorySize);
        {
            /* Configure slider: */
            m_pSliderVideoMemorySize->setMinimum(m_iMinVRAM);
            m_pSliderVideoMemorySize->setMaximum(m_iMaxVRAMVisible);
            m_pSliderVideoMemorySize->setPageStep(calculatePageStep(m_iMaxVRAMVisible));
            m_pSliderVideoMemorySize->setSingleStep(m_pSliderVideoMemorySize->pageStep() / 4);
            m_pSliderVideoMemorySize->setTickInterval(m_pSliderVideoMemorySize->pageStep());
            m_pSliderVideoMemorySize->setSnappingEnabled(true);
            m_pSliderVideoMemorySize->setErrorHint(0, 1);
        }

        /* Memory-size editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoMemorySize);
        {
            /* Configure editor: */
            vboxGlobal().setMinimumWidthAccordingSymbolCount(m_pEditorVideoMemorySize, 4);
            m_pEditorVideoMemorySize->setMinimum(m_iMinVRAM);
            m_pEditorVideoMemorySize->setMaximum(m_iMaxVRAMVisible);
        }

        /* Screen-count slider created in the .ui file. */
        AssertPtrReturnVoid(m_pSliderVideoScreenCount);
        {
            /* Configure slider: */
            const uint cHostScreens = gpDesktop->screenCount();
            const uint cMinGuestScreens = 1;
            const uint cMaxGuestScreens = sys.GetMaxGuestMonitors();
            const uint cMaxGuestScreensForSlider = qMin(cMaxGuestScreens, (uint)8);
            m_pSliderVideoScreenCount->setMinimum(cMinGuestScreens);
            m_pSliderVideoScreenCount->setMaximum(cMaxGuestScreensForSlider);
            m_pSliderVideoScreenCount->setPageStep(1);
            m_pSliderVideoScreenCount->setSingleStep(1);
            m_pSliderVideoScreenCount->setTickInterval(1);
            m_pSliderVideoScreenCount->setOptimalHint(cMinGuestScreens, cHostScreens);
            m_pSliderVideoScreenCount->setWarningHint(cHostScreens, cMaxGuestScreensForSlider);
        }

        /* Screen-count editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoScreenCount);
        {
            /* Configure editor: */
            const uint cMaxGuestScreens = sys.GetMaxGuestMonitors();
            vboxGlobal().setMinimumWidthAccordingSymbolCount(m_pEditorVideoScreenCount, 3);
            m_pEditorVideoScreenCount->setMinimum(1);
            m_pEditorVideoScreenCount->setMaximum(cMaxGuestScreens);
        }

        /* Scale-factor slider created in the .ui file. */
        AssertPtrReturnVoid(m_pSliderGuestScreenScale);
        {
            /* Configure slider: */
            m_pSliderGuestScreenScale->setMinimum(100);
            m_pSliderGuestScreenScale->setMaximum(200);
            m_pSliderGuestScreenScale->setPageStep(10);
            m_pSliderGuestScreenScale->setSingleStep(1);
            m_pSliderGuestScreenScale->setTickInterval(10);
            m_pSliderGuestScreenScale->setSnappingEnabled(true);
        }

        /* Scale-factor editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorGuestScreenScale);
        {
            /* Configure editor: */
            m_pEditorGuestScreenScale->setMinimum(100);
            m_pEditorGuestScreenScale->setMaximum(200);
            vboxGlobal().setMinimumWidthAccordingSymbolCount(m_pEditorGuestScreenScale, 5);
        }
    }
}

void UIMachineSettingsDisplay::prepareTabRemoteDisplay()
{
    /* Tab and it's layout created in the .ui file. */
    {
        /* Port editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorRemoteDisplayPort);
        {
            /* Configure editor: */
            m_pEditorRemoteDisplayPort->setValidator(new QRegExpValidator(
                QRegExp("(([0-9]{1,5}(\\-[0-9]{1,5}){0,1}),)*([0-9]{1,5}(\\-[0-9]{1,5}){0,1})"), this));
        }

        /* Timeout editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorRemoteDisplayTimeout);
        {
            /* Configure editor: */
            m_pEditorRemoteDisplayTimeout->setValidator(new QIntValidator(this));
        }

        /* Auth-method combo-box created in the .ui file. */
        AssertPtrReturnVoid(m_pComboRemoteDisplayAuthMethod);
        {
            /* Configure combo-box: */
            m_pComboRemoteDisplayAuthMethod->insertItem(0, ""); /* KAuthType_Null */
            m_pComboRemoteDisplayAuthMethod->insertItem(1, ""); /* KAuthType_External */
            m_pComboRemoteDisplayAuthMethod->insertItem(2, ""); /* KAuthType_Guest */
        }
    }
}

void UIMachineSettingsDisplay::prepareTabVideoCapture()
{
    /* Tab and it's layout created in the .ui file. */
    {
        /* File-path selector created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoCapturePath);
        {
            /* Configure selector: */
            m_pEditorVideoCapturePath->setEditable(false);
            m_pEditorVideoCapturePath->setMode(UIFilePathSelector::Mode_File_Save);
        }

        /* Frame-size combo-box created in the .ui file. */
        AssertPtrReturnVoid(m_pComboVideoCaptureSize);
        {
            /* Configure combo-box: */
            m_pComboVideoCaptureSize->addItem(""); /* User Defined */
            m_pComboVideoCaptureSize->addItem("320 x 200 (16:10)",   QSize(320, 200));
            m_pComboVideoCaptureSize->addItem("640 x 480 (4:3)",     QSize(640, 480));
            m_pComboVideoCaptureSize->addItem("720 x 400 (9:5)",     QSize(720, 400));
            m_pComboVideoCaptureSize->addItem("720 x 480 (3:2)",     QSize(720, 480));
            m_pComboVideoCaptureSize->addItem("800 x 600 (4:3)",     QSize(800, 600));
            m_pComboVideoCaptureSize->addItem("1024 x 768 (4:3)",    QSize(1024, 768));
            m_pComboVideoCaptureSize->addItem("1152 x 864 (4:3)",    QSize(1152, 864));
            m_pComboVideoCaptureSize->addItem("1280 x 720 (16:9)",   QSize(1280, 720));
            m_pComboVideoCaptureSize->addItem("1280 x 800 (16:10)",  QSize(1280, 800));
            m_pComboVideoCaptureSize->addItem("1280 x 960 (4:3)",    QSize(1280, 960));
            m_pComboVideoCaptureSize->addItem("1280 x 1024 (5:4)",   QSize(1280, 1024));
            m_pComboVideoCaptureSize->addItem("1366 x 768 (16:9)",   QSize(1366, 768));
            m_pComboVideoCaptureSize->addItem("1440 x 900 (16:10)",  QSize(1440, 900));
            m_pComboVideoCaptureSize->addItem("1440 x 1080 (4:3)",   QSize(1440, 1080));
            m_pComboVideoCaptureSize->addItem("1600 x 900 (16:9)",   QSize(1600, 900));
            m_pComboVideoCaptureSize->addItem("1680 x 1050 (16:10)", QSize(1680, 1050));
            m_pComboVideoCaptureSize->addItem("1600 x 1200 (4:3)",   QSize(1600, 1200));
            m_pComboVideoCaptureSize->addItem("1920 x 1080 (16:9)",  QSize(1920, 1080));
            m_pComboVideoCaptureSize->addItem("1920 x 1200 (16:10)", QSize(1920, 1200));
            m_pComboVideoCaptureSize->addItem("1920 x 1440 (4:3)",   QSize(1920, 1440));
            m_pComboVideoCaptureSize->addItem("2880 x 1800 (16:10)", QSize(2880, 1800));
        }

        /* Frame-width editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoCaptureWidth);
        {
            /* Configure editor: */
            vboxGlobal().setMinimumWidthAccordingSymbolCount(m_pEditorVideoCaptureWidth, 5);
            m_pEditorVideoCaptureWidth->setMinimum(16);
            m_pEditorVideoCaptureWidth->setMaximum(2880);
        }

        /* Frame-height editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoCaptureHeight);
        {
            /* Configure editor: */
            vboxGlobal().setMinimumWidthAccordingSymbolCount(m_pEditorVideoCaptureHeight, 5);
            m_pEditorVideoCaptureHeight->setMinimum(16);
            m_pEditorVideoCaptureHeight->setMaximum(1800);
        }

        /* Frame-rate slider created in the .ui file. */
        AssertPtrReturnVoid(m_pSliderVideoCaptureFrameRate);
        {
            /* Configure slider: */
            m_pSliderVideoCaptureFrameRate->setMinimum(1);
            m_pSliderVideoCaptureFrameRate->setMaximum(30);
            m_pSliderVideoCaptureFrameRate->setPageStep(1);
            m_pSliderVideoCaptureFrameRate->setSingleStep(1);
            m_pSliderVideoCaptureFrameRate->setTickInterval(1);
            m_pSliderVideoCaptureFrameRate->setSnappingEnabled(true);
            m_pSliderVideoCaptureFrameRate->setOptimalHint(1, 25);
            m_pSliderVideoCaptureFrameRate->setWarningHint(25, 30);
        }

        /* Frame-rate editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoCaptureFrameRate);
        {
            /* Configure editor: */
            vboxGlobal().setMinimumWidthAccordingSymbolCount(m_pEditorVideoCaptureFrameRate, 3);
            m_pEditorVideoCaptureFrameRate->setMinimum(1);
            m_pEditorVideoCaptureFrameRate->setMaximum(30);
        }

        /* Frame quality combo-box created in the .ui file. */
        AssertPtrReturnVoid(m_pContainerLayoutSliderVideoCaptureQuality);
        AssertPtrReturnVoid(m_pSliderVideoCaptureQuality);
        {
            /* Configure combo-box: */
            m_pContainerLayoutSliderVideoCaptureQuality->setColumnStretch(1, 4);
            m_pContainerLayoutSliderVideoCaptureQuality->setColumnStretch(3, 5);
            m_pSliderVideoCaptureQuality->setMinimum(1);
            m_pSliderVideoCaptureQuality->setMaximum(10);
            m_pSliderVideoCaptureQuality->setPageStep(1);
            m_pSliderVideoCaptureQuality->setSingleStep(1);
            m_pSliderVideoCaptureQuality->setTickInterval(1);
            m_pSliderVideoCaptureQuality->setSnappingEnabled(true);
            m_pSliderVideoCaptureQuality->setOptimalHint(1, 5);
            m_pSliderVideoCaptureQuality->setWarningHint(5, 9);
            m_pSliderVideoCaptureQuality->setErrorHint(9, 10);
        }

        /* Bit-rate editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoCaptureBitRate);
        {
            /* Configure editor: */
            vboxGlobal().setMinimumWidthAccordingSymbolCount(m_pEditorVideoCaptureBitRate, 5);
            m_pEditorVideoCaptureBitRate->setMinimum(32);
            m_pEditorVideoCaptureBitRate->setMaximum(2048);
        }
    }
}

void UIMachineSettingsDisplay::prepareConnections()
{
    /* Configure 'Screen' connections: */
    connect(m_pSliderVideoMemorySize, SIGNAL(valueChanged(int)), this, SLOT(sltHandleVideoMemorySizeSliderChange()));
    connect(m_pEditorVideoMemorySize, SIGNAL(valueChanged(int)), this, SLOT(sltHandleVideoMemorySizeEditorChange()));
    connect(m_pSliderVideoScreenCount, SIGNAL(valueChanged(int)), this, SLOT(sltHandleGuestScreenCountSliderChange()));
    connect(m_pEditorVideoScreenCount, SIGNAL(valueChanged(int)), this, SLOT(sltHandleGuestScreenCountEditorChange()));
    connect(m_pSliderGuestScreenScale, SIGNAL(valueChanged(int)), this, SLOT(sltHandleGuestScreenScaleSliderChange()));
    connect(m_pEditorGuestScreenScale, SIGNAL(valueChanged(int)), this, SLOT(sltHandleGuestScreenScaleEditorChange()));
    connect(m_pCheckbox3D, SIGNAL(stateChanged(int)), this, SLOT(revalidate()));
#ifdef VBOX_WITH_VIDEOHWACCEL
    connect(m_pCheckbox2DVideo, SIGNAL(stateChanged(int)), this, SLOT(revalidate()));
#endif

    /* Configure 'Remote Display' connections: */
    connect(m_pCheckboxRemoteDisplay, SIGNAL(toggled(bool)), this, SLOT(revalidate()));
    connect(m_pEditorRemoteDisplayPort, SIGNAL(textChanged(const QString &)), this, SLOT(revalidate()));
    connect(m_pEditorRemoteDisplayTimeout, SIGNAL(textChanged(const QString &)), this, SLOT(revalidate()));

    /* Configure 'Video Capture' connections: */
    connect(m_pCheckboxVideoCapture, SIGNAL(toggled(bool)), this, SLOT(sltHandleVideoCaptureCheckboxToggle()));
    connect(m_pComboVideoCaptureSize, SIGNAL(currentIndexChanged(int)), this, SLOT(sltHandleVideoCaptureFrameSizeComboboxChange()));
    connect(m_pEditorVideoCaptureWidth, SIGNAL(valueChanged(int)), this, SLOT(sltHandleVideoCaptureFrameWidthEditorChange()));
    connect(m_pEditorVideoCaptureHeight, SIGNAL(valueChanged(int)), this, SLOT(sltHandleVideoCaptureFrameHeightEditorChange()));
    connect(m_pSliderVideoCaptureFrameRate, SIGNAL(valueChanged(int)), this, SLOT(sltHandleVideoCaptureFrameRateSliderChange()));
    connect(m_pEditorVideoCaptureFrameRate, SIGNAL(valueChanged(int)), this, SLOT(sltHandleVideoCaptureFrameRateEditorChange()));
    connect(m_pSliderVideoCaptureQuality, SIGNAL(valueChanged(int)), this, SLOT(sltHandleVideoCaptureQualitySliderChange()));
    connect(m_pEditorVideoCaptureBitRate, SIGNAL(valueChanged(int)), this, SLOT(sltHandleVideoCaptureBitRateEditorChange()));
}

void UIMachineSettingsDisplay::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

void UIMachineSettingsDisplay::checkVRAMRequirements()
{
    /* Make sure guest OS type is set: */
    if (m_comGuestOSType.isNull())
        return;

    /* Get monitors count and base video memory requirements: */
    const int cGuestScreenCount = m_pEditorVideoScreenCount->value();
    quint64 uNeedMBytes = VBoxGlobal::requiredVideoMemory(m_comGuestOSType.GetId(), cGuestScreenCount) / _1M;

    /* Initial value: */
    m_iMaxVRAMVisible = cGuestScreenCount * 32;

    /* No more than m_iMaxVRAM: */
    if (m_iMaxVRAMVisible > m_iMaxVRAM)
        m_iMaxVRAMVisible = m_iMaxVRAM;

    /* No less than 128MB (if possible): */
    if (m_iMaxVRAMVisible < 128 && m_iMaxVRAM >= 128)
        m_iMaxVRAMVisible = 128;

    /* No less than initial VRAM size (wtf?): */
    if (m_iMaxVRAMVisible < m_iInitialVRAM)
        m_iMaxVRAMVisible = m_iInitialVRAM;

#ifdef VBOX_WITH_VIDEOHWACCEL
    if (m_pCheckbox2DVideo->isChecked() && m_f2DVideoAccelerationSupported)
    {
        uNeedMBytes += VBoxGlobal::required2DOffscreenVideoMemory() / _1M;
    }
#endif

#ifdef VBOX_WITH_CRHGSMI
    if (m_pCheckbox3D->isChecked() && m_fWddmModeSupported)
    {
        uNeedMBytes = qMax(uNeedMBytes, (quint64) 128);
        /* No less than 256MB (if possible): */
        if (m_iMaxVRAMVisible < 256 && m_iMaxVRAM >= 256)
            m_iMaxVRAMVisible = 256;
    }
#endif

    m_pEditorVideoMemorySize->setMaximum(m_iMaxVRAMVisible);
    m_pSliderVideoMemorySize->setMaximum(m_iMaxVRAMVisible);
    m_pSliderVideoMemorySize->setPageStep(calculatePageStep(m_iMaxVRAMVisible));
    m_pSliderVideoMemorySize->setWarningHint(1, qMin((int)uNeedMBytes, m_iMaxVRAMVisible));
    m_pSliderVideoMemorySize->setOptimalHint(qMin((int)uNeedMBytes, m_iMaxVRAMVisible), m_iMaxVRAMVisible);
    m_pLabelVideoMemorySizeMax->setText(tr("%1 MB").arg(m_iMaxVRAMVisible));
}

bool UIMachineSettingsDisplay::shouldWeWarnAboutLowVRAM()
{
    bool fResult = true;

    QStringList excludingOSList = QStringList()
        << "Other" << "DOS" << "Netware" << "L4" << "QNX" << "JRockitVE";
    if (excludingOSList.contains(m_comGuestOSType.GetId()))
        fResult = false;

    return fResult;
}

/* static */
int UIMachineSettingsDisplay::calculatePageStep(int iMax)
{
    /* Reasonable max. number of page steps is 32. */
    uint uPage = ((uint)iMax + 31) / 32;
    /* Make it a power of 2: */
    uint uP = uPage, p2 = 0x1;
    while ((uP >>= 1))
        p2 <<= 1;
    if (uPage != p2)
        p2 <<= 1;
    if (p2 < 4)
        p2 = 4;
    return (int)p2;
}

void UIMachineSettingsDisplay::lookForCorrespondingFrameSizePreset()
{
    /* Look for video-capture size preset: */
    lookForCorrespondingPreset(m_pComboVideoCaptureSize,
                               QSize(m_pEditorVideoCaptureWidth->value(),
                                     m_pEditorVideoCaptureHeight->value()));
}

void UIMachineSettingsDisplay::updateGuestScreenCount()
{
    /* Update copy of the cached item to get the desired result: */
    QVector<BOOL> screens = m_pCache->base().m_screens;
    screens.resize(m_pEditorVideoScreenCount->value());
    m_pScrollerVideoCaptureScreens->setValue(screens);
}

void UIMachineSettingsDisplay::updateVideoCaptureFileSizeHint()
{
    m_pLabelVideoCaptureSizeHint->setText(tr("<i>About %1MB per 5 minute video</i>").arg(m_pEditorVideoCaptureBitRate->value() * 300 / 8 / 1024));
}

/* static */
void UIMachineSettingsDisplay::lookForCorrespondingPreset(QComboBox *pComboBox, const QVariant &data)
{
    /* Use passed iterator to look for corresponding preset of passed combo-box: */
    const int iLookupResult = pComboBox->findData(data);
    if (iLookupResult != -1 && pComboBox->currentIndex() != iLookupResult)
        pComboBox->setCurrentIndex(iLookupResult);
    else if (iLookupResult == -1 && pComboBox->currentIndex() != 0)
        pComboBox->setCurrentIndex(0);
}

/* static */
int UIMachineSettingsDisplay::calculateBitRate(int iFrameWidth, int iFrameHeight, int iFrameRate, int iQuality)
{
    /* Linear quality<=>bit-rate scale-factor: */
    const double dResult = (double)iQuality
                         * (double)iFrameWidth * (double)iFrameHeight * (double)iFrameRate
                         / (double)10 /* translate quality to [%] */
                         / (double)1024 /* translate bit-rate to [kbps] */
                         / (double)18.75 /* linear scale factor */;
    return (int)dResult;
}

/* static */
int UIMachineSettingsDisplay::calculateQuality(int iFrameWidth, int iFrameHeight, int iFrameRate, int iBitRate)
{
    /* Linear bit-rate<=>quality scale-factor: */
    const double dResult = (double)iBitRate
                         / (double)iFrameWidth / (double)iFrameHeight / (double)iFrameRate
                         * (double)10 /* translate quality to [%] */
                         * (double)1024 /* translate bit-rate to [kbps] */
                         * (double)18.75 /* linear scale factor */;
    return (int)dResult;
}

bool UIMachineSettingsDisplay::saveDisplayData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save display settings from the cache: */
    if (fSuccess && isMachineInValidMode() && m_pCache->wasChanged())
    {
        /* Save 'Screen' data from the cache: */
        if (fSuccess)
            fSuccess = saveScreenData();
        /* Save 'Remote Display' data from the cache: */
        if (fSuccess)
            fSuccess = saveRemoteDisplayData();
        /* Save 'Video Capture' data from the cache: */
        if (fSuccess)
            fSuccess = saveVideoCaptureData();
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsDisplay::saveScreenData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save 'Screen' data from the cache: */
    if (fSuccess)
    {
        /* Get old display data from the cache: */
        const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();
        /* Get new display data from the cache: */
        const UIDataSettingsMachineDisplay &newDisplayData = m_pCache->data();

        /* Save video RAM size: */
        if (fSuccess && isMachineOffline() && newDisplayData.m_iCurrentVRAM != oldDisplayData.m_iCurrentVRAM)
        {
            m_machine.SetVRAMSize(newDisplayData.m_iCurrentVRAM);
            fSuccess = m_machine.isOk();
        }
        /* Save guest screen count: */
        if (fSuccess && isMachineOffline() && newDisplayData.m_cGuestScreenCount != oldDisplayData.m_cGuestScreenCount)
        {
            m_machine.SetMonitorCount(newDisplayData.m_cGuestScreenCount);
            fSuccess = m_machine.isOk();
        }
        /* Save whether 3D acceleration is enabled: */
        if (fSuccess && isMachineOffline() && newDisplayData.m_f3dAccelerationEnabled != oldDisplayData.m_f3dAccelerationEnabled)
        {
            m_machine.SetAccelerate3DEnabled(newDisplayData.m_f3dAccelerationEnabled);
            fSuccess = m_machine.isOk();
        }
#ifdef VBOX_WITH_VIDEOHWACCEL
        /* Save whether 2D video acceleration is enabled: */
        if (fSuccess && isMachineOffline() && newDisplayData.m_f2dAccelerationEnabled != oldDisplayData.m_f2dAccelerationEnabled)
        {
            m_machine.SetAccelerate2DVideoEnabled(newDisplayData.m_f2dAccelerationEnabled);
            fSuccess = m_machine.isOk();
        }
#endif

        /* Get machine ID for further activities: */
        QString strMachineId;
        if (fSuccess)
        {
            strMachineId = m_machine.GetId();
            fSuccess = m_machine.isOk();
        }

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));

        /* Save guest-screen scale-factor: */
        if (fSuccess && newDisplayData.m_dScaleFactor != oldDisplayData.m_dScaleFactor)
            /* fSuccess = */ gEDataManager->setScaleFactor(newDisplayData.m_dScaleFactor, strMachineId);
#ifdef VBOX_WS_MAC
        /* Save whether Unscaled HiDPI Output is enabled: : */
        if (fSuccess && newDisplayData.m_fUseUnscaledHiDPIOutput != oldDisplayData.m_fUseUnscaledHiDPIOutput)
            /* fSuccess = */ gEDataManager->setUseUnscaledHiDPIOutput(newDisplayData.m_fUseUnscaledHiDPIOutput, strMachineId);
#endif
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsDisplay::saveRemoteDisplayData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save 'Remote Display' data from the cache: */
    if (fSuccess)
    {
        /* Get old display data from the cache: */
        const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();
        /* Get new display data from the cache: */
        const UIDataSettingsMachineDisplay &newDisplayData = m_pCache->data();

        /* Get remote display server for further activities: */
        CVRDEServer comServer = m_machine.GetVRDEServer();
        fSuccess = m_machine.isOk() && comServer.isNotNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            /* Save whether remote display server is enabled: */
            if (fSuccess && newDisplayData.m_fRemoteDisplayServerEnabled != oldDisplayData.m_fRemoteDisplayServerEnabled)
            {
                comServer.SetEnabled(newDisplayData.m_fRemoteDisplayServerEnabled);
                fSuccess = comServer.isOk();
            }
            /* Save remote display server port: */
            if (fSuccess && newDisplayData.m_strRemoteDisplayPort != oldDisplayData.m_strRemoteDisplayPort)
            {
                comServer.SetVRDEProperty("TCP/Ports", newDisplayData.m_strRemoteDisplayPort);
                fSuccess = comServer.isOk();
            }
            /* Save remote display server auth type: */
            if (fSuccess && newDisplayData.m_remoteDisplayAuthType != oldDisplayData.m_remoteDisplayAuthType)
            {
                comServer.SetAuthType(newDisplayData.m_remoteDisplayAuthType);
                fSuccess = comServer.isOk();
            }
            /* Save remote display server timeout: */
            if (fSuccess && newDisplayData.m_uRemoteDisplayTimeout != oldDisplayData.m_uRemoteDisplayTimeout)
            {
                comServer.SetAuthTimeout(newDisplayData.m_uRemoteDisplayTimeout);
                fSuccess = comServer.isOk();
            }
            /* Save whether remote display server allows multiple connections: */
            if (   fSuccess
                && (isMachineOffline() || isMachineSaved())
                && (newDisplayData.m_fRemoteDisplayMultiConnAllowed != oldDisplayData.m_fRemoteDisplayMultiConnAllowed))
            {
                comServer.SetAllowMultiConnection(newDisplayData.m_fRemoteDisplayMultiConnAllowed);
                fSuccess = comServer.isOk();
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comServer));
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsDisplay::saveVideoCaptureData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save 'Video Capture' data from the cache: */
    if (fSuccess)
    {
        /* Get old display data from the cache: */
        const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();
        /* Get new display data from the cache: */
        const UIDataSettingsMachineDisplay &newDisplayData = m_pCache->data();

        /* Save new 'Video Capture' data for online case: */
        if (isMachineOnline())
        {
            /* If 'Video Capture' was *enabled*: */
            if (oldDisplayData.m_fVideoCaptureEnabled)
            {
                // We can still save the *screens* option.
                // And finally we should *disable* 'Video Capture' if necessary.
                /* Save video capture screens: */
                if (fSuccess && newDisplayData.m_screens != oldDisplayData.m_screens)
                {
                    m_machine.SetVideoCaptureScreens(newDisplayData.m_screens);
                    fSuccess = m_machine.isOk();
                }
                /* Save whether video capture is enabled: */
                if (fSuccess && newDisplayData.m_fVideoCaptureEnabled != oldDisplayData.m_fVideoCaptureEnabled)
                {
                    m_machine.SetVideoCaptureEnabled(newDisplayData.m_fVideoCaptureEnabled);
                    fSuccess = m_machine.isOk();
                }
            }
            /* If 'Video Capture' was *disabled*: */
            else
            {
                // We should save all the options *before* 'Video Capture' activation.
                // And finally we should *enable* Video Capture if necessary.
                /* Save video capture file path: */
                if (fSuccess && newDisplayData.m_strVideoCaptureFilePath != oldDisplayData.m_strVideoCaptureFilePath)
                {
                    m_machine.SetVideoCaptureFile(newDisplayData.m_strVideoCaptureFilePath);
                    fSuccess = m_machine.isOk();
                }
                /* Save video capture frame width: */
                if (fSuccess && newDisplayData.m_iVideoCaptureFrameWidth != oldDisplayData.m_iVideoCaptureFrameWidth)
                {
                    m_machine.SetVideoCaptureWidth(newDisplayData.m_iVideoCaptureFrameWidth);
                    fSuccess = m_machine.isOk();
                }
                /* Save video capture frame height: */
                if (fSuccess && newDisplayData.m_iVideoCaptureFrameHeight != oldDisplayData.m_iVideoCaptureFrameHeight)
                {
                    m_machine.SetVideoCaptureHeight(newDisplayData.m_iVideoCaptureFrameHeight);
                    fSuccess = m_machine.isOk();
                }
                /* Save video capture frame rate: */
                if (fSuccess && newDisplayData.m_iVideoCaptureFrameRate != oldDisplayData.m_iVideoCaptureFrameRate)
                {
                    m_machine.SetVideoCaptureFPS(newDisplayData.m_iVideoCaptureFrameRate);
                    fSuccess = m_machine.isOk();
                }
                /* Save video capture frame bit rate: */
                if (fSuccess && newDisplayData.m_iVideoCaptureBitRate != oldDisplayData.m_iVideoCaptureBitRate)
                {
                    m_machine.SetVideoCaptureRate(newDisplayData.m_iVideoCaptureBitRate);
                    fSuccess = m_machine.isOk();
                }
                /* Save video capture screens: */
                if (fSuccess && newDisplayData.m_screens != oldDisplayData.m_screens)
                {
                    m_machine.SetVideoCaptureScreens(newDisplayData.m_screens);
                    fSuccess = m_machine.isOk();
                }
                /* Save whether video capture is enabled: */
                if (fSuccess && newDisplayData.m_fVideoCaptureEnabled != oldDisplayData.m_fVideoCaptureEnabled)
                {
                    m_machine.SetVideoCaptureEnabled(newDisplayData.m_fVideoCaptureEnabled);
                    fSuccess = m_machine.isOk();
                }
            }
        }
        /* Save new 'Video Capture' data for offline case: */
        else
        {
            // For 'offline', 'powered off' and 'saved' states the order is irrelevant.
            /* Save whether video capture is enabled: */
            if (fSuccess && newDisplayData.m_fVideoCaptureEnabled != oldDisplayData.m_fVideoCaptureEnabled)
            {
                m_machine.SetVideoCaptureEnabled(newDisplayData.m_fVideoCaptureEnabled);
                fSuccess = m_machine.isOk();
            }
            /* Save video capture file path: */
            if (fSuccess && newDisplayData.m_strVideoCaptureFilePath != oldDisplayData.m_strVideoCaptureFilePath)
            {
                m_machine.SetVideoCaptureFile(newDisplayData.m_strVideoCaptureFilePath);
                fSuccess = m_machine.isOk();
            }
            /* Save video capture frame width: */
            if (fSuccess && newDisplayData.m_iVideoCaptureFrameWidth != oldDisplayData.m_iVideoCaptureFrameWidth)
            {
                m_machine.SetVideoCaptureWidth(newDisplayData.m_iVideoCaptureFrameWidth);
                fSuccess = m_machine.isOk();
            }
            /* Save video capture frame height: */
            if (fSuccess && newDisplayData.m_iVideoCaptureFrameHeight != oldDisplayData.m_iVideoCaptureFrameHeight)
            {
                m_machine.SetVideoCaptureHeight(newDisplayData.m_iVideoCaptureFrameHeight);
                fSuccess = m_machine.isOk();
            }
            /* Save video capture frame rate: */
            if (fSuccess && newDisplayData.m_iVideoCaptureFrameRate != oldDisplayData.m_iVideoCaptureFrameRate)
            {
                m_machine.SetVideoCaptureFPS(newDisplayData.m_iVideoCaptureFrameRate);
                fSuccess = m_machine.isOk();
            }
            /* Save video capture frame bit rate: */
            if (fSuccess && newDisplayData.m_iVideoCaptureBitRate != oldDisplayData.m_iVideoCaptureBitRate)
            {
                m_machine.SetVideoCaptureRate(newDisplayData.m_iVideoCaptureBitRate);
                fSuccess = m_machine.isOk();
            }
            /* Save video capture screens: */
            if (fSuccess && newDisplayData.m_screens != oldDisplayData.m_screens)
            {
                m_machine.SetVideoCaptureScreens(newDisplayData.m_screens);
                fSuccess = m_machine.isOk();
            }
            /* Save video capture options: */
            if (fSuccess && newDisplayData.m_strVideoCaptureOptions != oldDisplayData.m_strVideoCaptureOptions)
            {
                m_machine.SetVideoCaptureOptions(newDisplayData.m_strVideoCaptureOptions);
                fSuccess = m_machine.isOk();
            }
        }

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
    }
    /* Return result: */
    return fSuccess;
}

