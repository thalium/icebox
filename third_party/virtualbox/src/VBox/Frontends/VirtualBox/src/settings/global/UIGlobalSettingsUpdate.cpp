/* $Id: UIGlobalSettingsUpdate.cpp $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsUpdate class implementation.
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
# include "UIGlobalSettingsUpdate.h"
# include "UIExtraDataManager.h"
# include "UIMessageCenter.h"
# include "VBoxGlobal.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Global settings: Update page data structure. */
struct UIDataSettingsGlobalUpdate
{
    /** Constructs data. */
    UIDataSettingsGlobalUpdate()
        : m_fCheckEnabled(false)
        , m_periodIndex(VBoxUpdateData::PeriodUndefined)
        , m_branchIndex(VBoxUpdateData::BranchStable)
        , m_strDate(QString())
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsGlobalUpdate &other) const
    {
        return true
               && (m_fCheckEnabled == other.m_fCheckEnabled)
               && (m_periodIndex == other.m_periodIndex)
               && (m_branchIndex == other.m_branchIndex)
               && (m_strDate == other.m_strDate)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsGlobalUpdate &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsGlobalUpdate &other) const { return !equal(other); }

    /** Holds whether the update check is enabled. */
    bool m_fCheckEnabled;
    /** Holds the update check period. */
    VBoxUpdateData::PeriodType m_periodIndex;
    /** Holds the update branch type. */
    VBoxUpdateData::BranchType m_branchIndex;
    /** Holds the next update date. */
    QString m_strDate;
};


UIGlobalSettingsUpdate::UIGlobalSettingsUpdate()
    : m_pLastChosenRadio(0)
    , m_pCache(0)
{
    /* Prepare: */
    prepare();
}

UIGlobalSettingsUpdate::~UIGlobalSettingsUpdate()
{
    /* Cleanup: */
    cleanup();
}

void UIGlobalSettingsUpdate::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to properties: */
    UISettingsPageGlobal::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old update data: */
    UIDataSettingsGlobalUpdate oldUpdateData;

    /* Gather old update data: */
    const VBoxUpdateData updateData(gEDataManager->applicationUpdateData());
    oldUpdateData.m_fCheckEnabled = !updateData.isNoNeedToCheck();
    oldUpdateData.m_periodIndex = updateData.periodIndex();
    oldUpdateData.m_branchIndex = updateData.branchIndex();
    oldUpdateData.m_strDate = updateData.date();

    /* Cache old update data: */
    m_pCache->cacheInitialData(oldUpdateData);

    /* Upload properties to data: */
    UISettingsPageGlobal::uploadData(data);
}

void UIGlobalSettingsUpdate::getFromCache()
{
    /* Get old update data from the cache: */
    const UIDataSettingsGlobalUpdate &oldUpdateData = m_pCache->base();

    /* Load old update data from the cache: */
    m_pCheckBoxUpdate->setChecked(oldUpdateData.m_fCheckEnabled);
    if (m_pCheckBoxUpdate->isChecked())
    {
        m_pComboBoxUpdatePeriod->setCurrentIndex(oldUpdateData.m_periodIndex);
        if (oldUpdateData.m_branchIndex == VBoxUpdateData::BranchWithBetas)
            m_pRadioUpdateFilterBetas->setChecked(true);
        else if (oldUpdateData.m_branchIndex == VBoxUpdateData::BranchAllRelease)
            m_pRadioUpdateFilterEvery->setChecked(true);
        else
            m_pRadioUpdateFilterStable->setChecked(true);
    }
    m_pUpdateDateText->setText(oldUpdateData.m_strDate);
    sltHandleUpdateToggle(oldUpdateData.m_fCheckEnabled);
}

void UIGlobalSettingsUpdate::putToCache()
{
    /* Prepare new update data: */
    UIDataSettingsGlobalUpdate newUpdateData = m_pCache->base();

    /* Gather new update data: */
    newUpdateData.m_periodIndex = periodType();
    newUpdateData.m_branchIndex = branchType();

    /* Cache new update data: */
    m_pCache->cacheCurrentData(newUpdateData);
}

void UIGlobalSettingsUpdate::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to properties: */
    UISettingsPageGlobal::fetchData(data);

    /* Update update data and failing state: */
    setFailed(!saveUpdateData());

    /* Upload properties to data: */
    UISettingsPageGlobal::uploadData(data);
}

void UIGlobalSettingsUpdate::setOrderAfter(QWidget *pWidget)
{
    /* Configure navigation: */
    setTabOrder(pWidget, m_pCheckBoxUpdate);
    setTabOrder(m_pCheckBoxUpdate, m_pComboBoxUpdatePeriod);
    setTabOrder(m_pComboBoxUpdatePeriod, m_pRadioUpdateFilterStable);
    setTabOrder(m_pRadioUpdateFilterStable, m_pRadioUpdateFilterEvery);
    setTabOrder(m_pRadioUpdateFilterEvery, m_pRadioUpdateFilterBetas);
}

void UIGlobalSettingsUpdate::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIGlobalSettingsUpdate::retranslateUi(this);

    /* Retranslate m_pComboBoxUpdatePeriod combobox: */
    int iCurrenIndex = m_pComboBoxUpdatePeriod->currentIndex();
    m_pComboBoxUpdatePeriod->clear();
    VBoxUpdateData::populate();
    m_pComboBoxUpdatePeriod->insertItems(0, VBoxUpdateData::list());
    m_pComboBoxUpdatePeriod->setCurrentIndex(iCurrenIndex == -1 ? 0 : iCurrenIndex);
}

void UIGlobalSettingsUpdate::sltHandleUpdateToggle(bool fEnabled)
{
    /* Update activity status: */
    m_pContainerUpdate->setEnabled(fEnabled);

    /* Update time of next check: */
    sltHandleUpdatePeriodChange();

    /* Temporary remember branch type if was switched off: */
    if (!fEnabled)
    {
        m_pLastChosenRadio = m_pRadioUpdateFilterBetas->isChecked() ? m_pRadioUpdateFilterBetas :
                             m_pRadioUpdateFilterEvery->isChecked() ? m_pRadioUpdateFilterEvery : m_pRadioUpdateFilterStable;
    }

    /* Check/uncheck last selected radio depending on activity status: */
    if (m_pLastChosenRadio)
        m_pLastChosenRadio->setChecked(fEnabled);
}

void UIGlobalSettingsUpdate::sltHandleUpdatePeriodChange()
{
    const VBoxUpdateData data(periodType(), branchType());
    m_pUpdateDateText->setText(data.date());
}

void UIGlobalSettingsUpdate::prepare()
{
    /* Apply UI decorations: */
    Ui::UIGlobalSettingsUpdate::setupUi(this);

    /* Prepare cache: */
    m_pCache = new UISettingsCacheGlobalUpdate;
    AssertPtrReturnVoid(m_pCache);

    /* Layout/widgets created in the .ui file. */
    AssertPtrReturnVoid(m_pCheckBoxUpdate);
    AssertPtrReturnVoid(m_pComboBoxUpdatePeriod);
    {
        /* Configure widgets: */
        connect(m_pCheckBoxUpdate, SIGNAL(toggled(bool)), this, SLOT(sltHandleUpdateToggle(bool)));
        connect(m_pComboBoxUpdatePeriod, SIGNAL(activated(int)), this, SLOT(sltHandleUpdatePeriodChange()));
    }

    /* Apply language settings: */
    retranslateUi();
}

void UIGlobalSettingsUpdate::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

VBoxUpdateData::PeriodType UIGlobalSettingsUpdate::periodType() const
{
    const VBoxUpdateData::PeriodType result = m_pCheckBoxUpdate->isChecked() ?
        (VBoxUpdateData::PeriodType)m_pComboBoxUpdatePeriod->currentIndex() : VBoxUpdateData::PeriodNever;
    return result == VBoxUpdateData::PeriodUndefined ? VBoxUpdateData::Period1Day : result;
}

VBoxUpdateData::BranchType UIGlobalSettingsUpdate::branchType() const
{
    if (m_pRadioUpdateFilterBetas->isChecked())
        return VBoxUpdateData::BranchWithBetas;
    else if (m_pRadioUpdateFilterEvery->isChecked())
        return VBoxUpdateData::BranchAllRelease;
    else
        return VBoxUpdateData::BranchStable;
}

bool UIGlobalSettingsUpdate::saveUpdateData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save update settings from the cache: */
    if (fSuccess && m_pCache->wasChanged())
    {
        /* Get old update data from the cache: */
        //const UIDataSettingsGlobalUpdate &oldUpdateData = m_pCache->base();
        /* Get new update data from the cache: */
        const UIDataSettingsGlobalUpdate &newUpdateData = m_pCache->data();

        /* Save new update data from the cache: */
        const VBoxUpdateData newData(newUpdateData.m_periodIndex, newUpdateData.m_branchIndex);
        gEDataManager->setApplicationUpdateData(newData.data());
    }
    /* Return result: */
    return fSuccess;
}

