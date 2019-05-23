/* $Id: UIGlobalSettingsDisplay.cpp $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsDisplay class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* GUI includes: */
# include "UIExtraDataManager.h"
# include "UIGlobalSettingsDisplay.h"
# include "UIMessageCenter.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Global settings: Display page data structure. */
struct UIDataSettingsGlobalDisplay
{
    /** Constructs data. */
    UIDataSettingsGlobalDisplay()
        : m_enmMaxGuestResolution(MaxGuestResolutionPolicy_Automatic)
        , m_maxGuestResolution(QSize())
        , m_fActivateHoveredMachineWindow(false)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsGlobalDisplay &other) const
    {
        return true
               && (m_enmMaxGuestResolution == other.m_enmMaxGuestResolution)
               && (m_maxGuestResolution == other.m_maxGuestResolution)
               && (m_fActivateHoveredMachineWindow == other.m_fActivateHoveredMachineWindow)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsGlobalDisplay &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsGlobalDisplay &other) const { return !equal(other); }

    /** Holds the maximum guest-screen resolution policy. */
    MaxGuestResolutionPolicy m_enmMaxGuestResolution;
    /** Holds the maximum guest-screen resolution. */
    QSize m_maxGuestResolution;
    /** Holds whether we should automatically activate machine window under the mouse cursor. */
    bool m_fActivateHoveredMachineWindow;
};


UIGlobalSettingsDisplay::UIGlobalSettingsDisplay()
    : m_pCache(0)
{
    /* Prepare: */
    prepare();
}

UIGlobalSettingsDisplay::~UIGlobalSettingsDisplay()
{
    /* Cleanup: */
    cleanup();
}

void UIGlobalSettingsDisplay::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to properties: */
    UISettingsPageGlobal::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old display data: */
    UIDataSettingsGlobalDisplay oldDisplayData;

    /* Gather old display data: */
    oldDisplayData.m_enmMaxGuestResolution = gEDataManager->maxGuestResolutionPolicy();
    if (oldDisplayData.m_enmMaxGuestResolution == MaxGuestResolutionPolicy_Fixed)
        oldDisplayData.m_maxGuestResolution = gEDataManager->maxGuestResolutionForPolicyFixed();
    oldDisplayData.m_fActivateHoveredMachineWindow = gEDataManager->activateHoveredMachineWindow();

    /* Cache old display data: */
    m_pCache->cacheInitialData(oldDisplayData);

    /* Upload properties to data: */
    UISettingsPageGlobal::uploadData(data);
}

void UIGlobalSettingsDisplay::getFromCache()
{
    /* Get old display data from the cache: */
    const UIDataSettingsGlobalDisplay &oldDisplayData = m_pCache->base();

    /* Load old display data from the cache: */
    m_pMaxResolutionCombo->setCurrentIndex(m_pMaxResolutionCombo->findData((int)oldDisplayData.m_enmMaxGuestResolution));
    if (oldDisplayData.m_enmMaxGuestResolution == MaxGuestResolutionPolicy_Fixed)
    {
        m_pResolutionWidthSpin->setValue(oldDisplayData.m_maxGuestResolution.width());
        m_pResolutionHeightSpin->setValue(oldDisplayData.m_maxGuestResolution.height());
    }
    m_pCheckBoxActivateOnMouseHover->setChecked(oldDisplayData.m_fActivateHoveredMachineWindow);
}

void UIGlobalSettingsDisplay::putToCache()
{
    /* Prepare new display data: */
    UIDataSettingsGlobalDisplay newDisplayData = m_pCache->base();

    /* Gather new display data: */
    newDisplayData.m_enmMaxGuestResolution = (MaxGuestResolutionPolicy)m_pMaxResolutionCombo->itemData(m_pMaxResolutionCombo->currentIndex()).toInt();
    if (newDisplayData.m_enmMaxGuestResolution == MaxGuestResolutionPolicy_Fixed)
        newDisplayData.m_maxGuestResolution = QSize(m_pResolutionWidthSpin->value(), m_pResolutionHeightSpin->value());
    newDisplayData.m_fActivateHoveredMachineWindow = m_pCheckBoxActivateOnMouseHover->isChecked();

    /* Cache new display data: */
    m_pCache->cacheCurrentData(newDisplayData);
}

void UIGlobalSettingsDisplay::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to properties: */
    UISettingsPageGlobal::fetchData(data);

    /* Update display data and failing state: */
    setFailed(!saveDisplayData());

    /* Upload properties to data: */
    UISettingsPageGlobal::uploadData(data);
}

void UIGlobalSettingsDisplay::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIGlobalSettingsDisplay::retranslateUi(this);

    /* Reload combo-box: */
    reloadMaximumGuestScreenSizePolicyComboBox();
}

void UIGlobalSettingsDisplay::sltHandleMaximumGuestScreenSizePolicyChange()
{
    /* Get current resolution-combo tool-tip data: */
    const QString strCurrentComboItemTip = m_pMaxResolutionCombo->itemData(m_pMaxResolutionCombo->currentIndex(), Qt::ToolTipRole).toString();
    m_pMaxResolutionCombo->setWhatsThis(strCurrentComboItemTip);

    /* Get current resolution-combo item data: */
    const MaxGuestResolutionPolicy enmPolicy = (MaxGuestResolutionPolicy)m_pMaxResolutionCombo->itemData(m_pMaxResolutionCombo->currentIndex()).toInt();
    /* Should be combo-level widgets enabled? */
    const bool fComboLevelWidgetsEnabled = enmPolicy == MaxGuestResolutionPolicy_Fixed;
    /* Enable/disable combo-level widgets: */
    m_pResolutionWidthLabel->setEnabled(fComboLevelWidgetsEnabled);
    m_pResolutionWidthSpin->setEnabled(fComboLevelWidgetsEnabled);
    m_pResolutionHeightLabel->setEnabled(fComboLevelWidgetsEnabled);
    m_pResolutionHeightSpin->setEnabled(fComboLevelWidgetsEnabled);
}

void UIGlobalSettingsDisplay::prepare()
{
    /* Apply UI decorations: */
    Ui::UIGlobalSettingsDisplay::setupUi(this);

    /* Prepare cache: */
    m_pCache = new UISettingsCacheGlobalDisplay;
    AssertPtrReturnVoid(m_pCache);

    /* Layout/widgets created in the .ui file. */
    AssertPtrReturnVoid(m_pResolutionWidthSpin);
    AssertPtrReturnVoid(m_pResolutionHeightSpin);
    AssertPtrReturnVoid(m_pMaxResolutionCombo);
    {
        /* Configure widgets: */
        const int iMinWidth = 640;
        const int iMinHeight = 480;
        const int iMaxSize = 16 * _1K;
        m_pResolutionWidthSpin->setMinimum(iMinWidth);
        m_pResolutionWidthSpin->setMaximum(iMaxSize);
        m_pResolutionHeightSpin->setMinimum(iMinHeight);
        m_pResolutionHeightSpin->setMaximum(iMaxSize);
        connect(m_pMaxResolutionCombo, SIGNAL(currentIndexChanged(int)),
                this, SLOT(sltHandleMaximumGuestScreenSizePolicyChange()));
    }

    /* Apply language settings: */
    retranslateUi();
}

void UIGlobalSettingsDisplay::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

void UIGlobalSettingsDisplay::reloadMaximumGuestScreenSizePolicyComboBox()
{
    /* Remember current position: */
    int iCurrentPosition = m_pMaxResolutionCombo->currentIndex();
    if (iCurrentPosition == -1)
        iCurrentPosition = 0;

    /* Clear combo-box: */
    m_pMaxResolutionCombo->clear();

    /* Create corresponding items: */
    m_pMaxResolutionCombo->addItem(tr("Automatic", "Maximum Guest Screen Size"),
                                   QVariant((int)MaxGuestResolutionPolicy_Automatic));
    m_pMaxResolutionCombo->setItemData(m_pMaxResolutionCombo->count() - 1,
                                       tr("Suggest a reasonable maximum screen size to the guest. "
                                          "The guest will only see this suggestion when guest additions are installed."),
                                       Qt::ToolTipRole);
    m_pMaxResolutionCombo->addItem(tr("None", "Maximum Guest Screen Size"),
                                   QVariant((int)MaxGuestResolutionPolicy_Any));
    m_pMaxResolutionCombo->setItemData(m_pMaxResolutionCombo->count() - 1,
                                       tr("Do not attempt to limit the size of the guest screen."),
                                       Qt::ToolTipRole);
    m_pMaxResolutionCombo->addItem(tr("Hint", "Maximum Guest Screen Size"),
                                   QVariant((int)MaxGuestResolutionPolicy_Fixed));
    m_pMaxResolutionCombo->setItemData(m_pMaxResolutionCombo->count() - 1,
                                       tr("Suggest a maximum screen size to the guest. "
                                          "The guest will only see this suggestion when guest additions are installed."),
                                       Qt::ToolTipRole);

    /* Choose previous position: */
    m_pMaxResolutionCombo->setCurrentIndex(iCurrentPosition);
    sltHandleMaximumGuestScreenSizePolicyChange();
}

bool UIGlobalSettingsDisplay::saveDisplayData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save display settings from the cache: */
    if (fSuccess && m_pCache->wasChanged())
    {
        /* Get old display data from the cache: */
        const UIDataSettingsGlobalDisplay &oldDisplayData = m_pCache->base();
        /* Get new display data from the cache: */
        const UIDataSettingsGlobalDisplay &newDisplayData = m_pCache->data();

        /* Save maximum guest resolution policy and/or value: */
        if (   fSuccess
            && (   newDisplayData.m_enmMaxGuestResolution != oldDisplayData.m_enmMaxGuestResolution
                || newDisplayData.m_maxGuestResolution != oldDisplayData.m_maxGuestResolution))
            gEDataManager->setMaxGuestScreenResolution(newDisplayData.m_enmMaxGuestResolution, newDisplayData.m_maxGuestResolution);
        /* Save whether hovered machine-window should be activated automatically: */
        if (fSuccess && newDisplayData.m_fActivateHoveredMachineWindow != oldDisplayData.m_fActivateHoveredMachineWindow)
            gEDataManager->setActivateHoveredMachineWindow(newDisplayData.m_fActivateHoveredMachineWindow);
    }
    /* Return result: */
    return fSuccess;
}

