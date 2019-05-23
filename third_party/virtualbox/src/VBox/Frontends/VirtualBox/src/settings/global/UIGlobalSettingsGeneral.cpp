/* $Id: UIGlobalSettingsGeneral.cpp $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsGeneral class implementation.
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

/* Qt includes: */
# include <QDir>

/* GUI includes: */
# include "UIGlobalSettingsGeneral.h"
# include "UIExtraDataManager.h"
# include "UIErrorString.h"
# include "VBoxGlobal.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Global settings: General page data structure. */
struct UIDataSettingsGlobalGeneral
{
    /** Constructs data. */
    UIDataSettingsGlobalGeneral()
        : m_strDefaultMachineFolder(QString())
        , m_strVRDEAuthLibrary(QString())
        , m_fHostScreenSaverDisabled(false)
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsGlobalGeneral &other) const
    {
        return true
               && (m_strDefaultMachineFolder == other.m_strDefaultMachineFolder)
               && (m_strVRDEAuthLibrary == other.m_strVRDEAuthLibrary)
               && (m_fHostScreenSaverDisabled == other.m_fHostScreenSaverDisabled)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsGlobalGeneral &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsGlobalGeneral &other) const { return !equal(other); }

    /** Holds the default machine folder path. */
    QString m_strDefaultMachineFolder;
    /** Holds the VRDE authentication library name. */
    QString m_strVRDEAuthLibrary;
    /** Holds whether host screen-saver should be disabled. */
    bool m_fHostScreenSaverDisabled;
};


UIGlobalSettingsGeneral::UIGlobalSettingsGeneral()
    : m_pCache(0)
{
    /* Prepare: */
    prepare();
}

UIGlobalSettingsGeneral::~UIGlobalSettingsGeneral()
{
    /* Cleanup: */
    cleanup();
}

void UIGlobalSettingsGeneral::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to properties: */
    UISettingsPageGlobal::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old general data: */
    UIDataSettingsGlobalGeneral oldGeneralData;

    /* Gather old general data: */
    oldGeneralData.m_strDefaultMachineFolder = m_properties.GetDefaultMachineFolder();
    oldGeneralData.m_strVRDEAuthLibrary = m_properties.GetVRDEAuthLibrary();
    oldGeneralData.m_fHostScreenSaverDisabled = gEDataManager->hostScreenSaverDisabled();

    /* Cache old general data: */
    m_pCache->cacheInitialData(oldGeneralData);

    /* Upload properties to data: */
    UISettingsPageGlobal::uploadData(data);
}

void UIGlobalSettingsGeneral::getFromCache()
{
    /* Get old general data from the cache: */
    const UIDataSettingsGlobalGeneral &oldGeneralData = m_pCache->base();

    /* Load old general data from the cache: */
    m_pSelectorMachineFolder->setPath(oldGeneralData.m_strDefaultMachineFolder);
    m_pSelectorVRDPLibName->setPath(oldGeneralData.m_strVRDEAuthLibrary);
    m_pCheckBoxHostScreenSaver->setChecked(oldGeneralData.m_fHostScreenSaverDisabled);
}

void UIGlobalSettingsGeneral::putToCache()
{
    /* Prepare new general data: */
    UIDataSettingsGlobalGeneral newGeneralData = m_pCache->base();

    /* Gather new general data: */
    newGeneralData.m_strDefaultMachineFolder = m_pSelectorMachineFolder->path();
    newGeneralData.m_strVRDEAuthLibrary = m_pSelectorVRDPLibName->path();
    newGeneralData.m_fHostScreenSaverDisabled = m_pCheckBoxHostScreenSaver->isChecked();

    /* Cache new general data: */
    m_pCache->cacheCurrentData(newGeneralData);
}

void UIGlobalSettingsGeneral::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to properties: */
    UISettingsPageGlobal::fetchData(data);

    /* Update general data and failing state: */
    setFailed(!saveGeneralData());

    /* Upload properties to data: */
    UISettingsPageGlobal::uploadData(data);
}

void UIGlobalSettingsGeneral::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIGlobalSettingsGeneral::retranslateUi(this);
}

void UIGlobalSettingsGeneral::prepare()
{
    /* Apply UI decorations: */
    Ui::UIGlobalSettingsGeneral::setupUi(this);

    /* Prepare cache: */
    m_pCache = new UISettingsCacheGlobalGeneral;
    AssertPtrReturnVoid(m_pCache);

    /* Layout/widgets created in the .ui file. */
    AssertPtrReturnVoid(m_pLabelHostScreenSaver);
    AssertPtrReturnVoid(m_pCheckBoxHostScreenSaver);
    AssertPtrReturnVoid(m_pSelectorMachineFolder);
    AssertPtrReturnVoid(m_pSelectorVRDPLibName);
    {
        /* Configure host screen-saver check-box: */
        // Hide checkbox for now.
        m_pLabelHostScreenSaver->hide();
        m_pCheckBoxHostScreenSaver->hide();

        /* Configure other widgets: */
        m_pSelectorMachineFolder->setHomeDir(vboxGlobal().homeFolder());
        m_pSelectorVRDPLibName->setHomeDir(vboxGlobal().homeFolder());
        m_pSelectorVRDPLibName->setMode(UIFilePathSelector::Mode_File_Open);
    }

    /* Apply language settings: */
    retranslateUi();
}

void UIGlobalSettingsGeneral::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

bool UIGlobalSettingsGeneral::saveGeneralData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save general settings from the cache: */
    if (fSuccess && m_pCache->wasChanged())
    {
        /* Get old general data from the cache: */
        const UIDataSettingsGlobalGeneral &oldGeneralData = m_pCache->base();
        /* Get new general data from the cache: */
        const UIDataSettingsGlobalGeneral &newGeneralData = m_pCache->data();

        /* Save default machine folder: */
        if (   fSuccess
            && newGeneralData.m_strDefaultMachineFolder != oldGeneralData.m_strDefaultMachineFolder)
        {
            m_properties.SetDefaultMachineFolder(newGeneralData.m_strDefaultMachineFolder);
            fSuccess = m_properties.isOk();
        }
        /* Save VRDE auth library: */
        if (   fSuccess
            && newGeneralData.m_strVRDEAuthLibrary != oldGeneralData.m_strVRDEAuthLibrary)
        {
            m_properties.SetVRDEAuthLibrary(newGeneralData.m_strVRDEAuthLibrary);
            fSuccess = m_properties.isOk();
        }

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_properties));

        /* Save new general data from the cache: */
        if (newGeneralData.m_fHostScreenSaverDisabled != oldGeneralData.m_fHostScreenSaverDisabled)
            gEDataManager->setHostScreenSaverDisabled(newGeneralData.m_fHostScreenSaverDisabled);
    }
    /* Return result: */
    return fSuccess;
}

