/* $Id: UIMachineSettingsDisplay.h $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsDisplay class declaration.
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

#ifndef ___UIMachineSettingsDisplay_h___
#define ___UIMachineSettingsDisplay_h___

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIMachineSettingsDisplay.gen.h"

/* COM includes: */
#include "CGuestOSType.h"

/* Forward declarations: */
class UIActionPool;
struct UIDataSettingsMachineDisplay;
typedef UISettingsCache<UIDataSettingsMachineDisplay> UISettingsCacheMachineDisplay;


/** Machine settings: Display page. */
class UIMachineSettingsDisplay : public UISettingsPageMachine,
                                 public Ui::UIMachineSettingsDisplay
{
    Q_OBJECT;

public:

    /** Constructs Display settings page. */
    UIMachineSettingsDisplay();
    /** Destructs Display settings page. */
    ~UIMachineSettingsDisplay();

    /** Defines @a comGuestOSType. */
    void setGuestOSType(CGuestOSType comGuestOSType);

#ifdef VBOX_WITH_VIDEOHWACCEL
    /** Returns whether 2D Video Acceleration is enabled. */
    bool isAcceleration2DVideoSelected() const;
#endif

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

    /** Handles Video Memory size slider change. */
    void sltHandleVideoMemorySizeSliderChange();
    /** Handles Video Memory size editor change. */
    void sltHandleVideoMemorySizeEditorChange();
    /** Handles Guest Screen count slider change. */
    void sltHandleGuestScreenCountSliderChange();
    /** Handles Guest Screen count editor change. */
    void sltHandleGuestScreenCountEditorChange();
    /** Handles Guest Screen scale-factor slider change. */
    void sltHandleGuestScreenScaleSliderChange();
    /** Handles Guest Screen scale-factor editor change. */
    void sltHandleGuestScreenScaleEditorChange();

    /** Handles Video Capture toggle. */
    void sltHandleVideoCaptureCheckboxToggle();
    /** Handles Video Capture frame size change. */
    void sltHandleVideoCaptureFrameSizeComboboxChange();
    /** Handles Video Capture frame width change. */
    void sltHandleVideoCaptureFrameWidthEditorChange();
    /** Handles Video Capture frame height change. */
    void sltHandleVideoCaptureFrameHeightEditorChange();
    /** Handles Video Capture frame rate slider change. */
    void sltHandleVideoCaptureFrameRateSliderChange();
    /** Handles Video Capture frame rate editor change. */
    void sltHandleVideoCaptureFrameRateEditorChange();
    /** Handles Video Capture quality slider change. */
    void sltHandleVideoCaptureQualitySliderChange();
    /** Handles Video Capture bit-rate editor change. */
    void sltHandleVideoCaptureBitRateEditorChange();

private:

    /** Prepares all. */
    void prepare();
    /** Prepares 'Screen' tab. */
    void prepareTabScreen();
    /** Prepares 'Remote Display' tab. */
    void prepareTabRemoteDisplay();
    /** Prepares 'Video Capture' tab. */
    void prepareTabVideoCapture();
    /** Prepares connections. */
    void prepareConnections();
    /** Cleanups all. */
    void cleanup();

    /** Checks the VRAM requirements. */
    void checkVRAMRequirements();
    /** Returns whether the VRAM requirements are important. */
    bool shouldWeWarnAboutLowVRAM();
    /** Calculates the reasonably sane slider page step. */
    static int calculatePageStep(int iMax);

    /** Searches for corresponding frame size preset. */
    void lookForCorrespondingFrameSizePreset();
    /** Updates guest-screen count. */
    void updateGuestScreenCount();
    /** Updates video capture file size hint. */
    void updateVideoCaptureFileSizeHint();
    /** Searches for the @a data field in corresponding @a pComboBox. */
    static void lookForCorrespondingPreset(QComboBox *pComboBox, const QVariant &data);
    /** Calculates Video Capture bit-rate for passed @a iFrameWidth, @a iFrameHeight, @a iFrameRate and @a iQuality. */
    static int calculateBitRate(int iFrameWidth, int iFrameHeight, int iFrameRate, int iQuality);
    /** Calculates Video Capture quality for passed @a iFrameWidth, @a iFrameHeight, @a iFrameRate and @a iBitRate. */
    static int calculateQuality(int iFrameWidth, int iFrameHeight, int iFrameRate, int iBitRate);

    /** Saves existing display data from the cache. */
    bool saveDisplayData();
    /** Saves existing 'Screen' data from the cache. */
    bool saveScreenData();
    /** Saves existing 'Remote Display' data from the cache. */
    bool saveRemoteDisplayData();
    /** Saves existing 'Video Capture' data from the cache. */
    bool saveVideoCaptureData();

    /** Holds the guest OS type ID. */
    CGuestOSType  m_comGuestOSType;
    /** Holds the minimum lower limit of VRAM (MiB). */
    int           m_iMinVRAM;
    /** Holds the maximum upper limit of VRAM (MiB). */
    int           m_iMaxVRAM;
    /** Holds the upper limit of VRAM (MiB) for this dialog.
      * This value is lower than m_iMaxVRAM to save careless
      * users from setting useless big values. */
    int           m_iMaxVRAMVisible;
    /** Holds the initial VRAM value when the dialog is opened. */
    int           m_iInitialVRAM;
#ifdef VBOX_WITH_VIDEOHWACCEL
    /** Holds whether the guest OS supports 2D Video Acceleration. */
    bool          m_f2DVideoAccelerationSupported;
#endif
#ifdef VBOX_WITH_CRHGSMI
    /** Holds whether the guest OS supports WDDM. */
    bool          m_fWddmModeSupported;
#endif

    /** Holds the page data cache instance. */
    UISettingsCacheMachineDisplay *m_pCache;
};

#endif /* !___UIMachineSettingsDisplay_h___ */

