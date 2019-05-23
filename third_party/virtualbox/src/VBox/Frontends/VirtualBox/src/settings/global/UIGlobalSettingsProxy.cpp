/* $Id: UIGlobalSettingsProxy.cpp $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsProxy class implementation.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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
# include <QRegExpValidator>

/* GUI includes: */
# include "QIWidgetValidator.h"
# include "UIGlobalSettingsProxy.h"
# include "UIExtraDataManager.h"
# include "UIMessageCenter.h"
# include "VBoxUtils.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Global settings: Proxy page data structure. */
struct UIDataSettingsGlobalProxy
{
    /** Constructs data. */
    UIDataSettingsGlobalProxy()
        : m_enmProxyState(UIProxyManager::ProxyState_Auto)
        , m_strProxyHost(QString())
        , m_strProxyPort(QString())
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsGlobalProxy &other) const
    {
        return true
               && (m_enmProxyState == other.m_enmProxyState)
               && (m_strProxyHost == other.m_strProxyHost)
               && (m_strProxyPort == other.m_strProxyPort)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsGlobalProxy &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsGlobalProxy &other) const { return !equal(other); }

    /** Holds the proxy state. */
    UIProxyManager::ProxyState m_enmProxyState;
    /** Holds the proxy host. */
    QString m_strProxyHost;
    /** Holds the proxy port. */
    QString m_strProxyPort;
};


UIGlobalSettingsProxy::UIGlobalSettingsProxy()
    : m_pCache(0)
{
    /* Prepare: */
    prepare();
}

UIGlobalSettingsProxy::~UIGlobalSettingsProxy()
{
    /* Cleanup: */
    cleanup();
}

void UIGlobalSettingsProxy::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to properties: */
    UISettingsPageGlobal::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old proxy data: */
    UIDataSettingsGlobalProxy oldProxyData;

    /* Gather old proxy data: */
    UIProxyManager proxyManager(gEDataManager->proxySettings());
    oldProxyData.m_enmProxyState = proxyManager.proxyState();
    oldProxyData.m_strProxyHost = proxyManager.proxyHost();
    oldProxyData.m_strProxyPort = proxyManager.proxyPort();

    /* Cache old proxy data: */
    m_pCache->cacheInitialData(oldProxyData);

    /* Upload properties to data: */
    UISettingsPageGlobal::uploadData(data);
}

void UIGlobalSettingsProxy::getFromCache()
{
    /* Get old proxy data from the cache: */
    const UIDataSettingsGlobalProxy &oldProxyData = m_pCache->base();

    /* Load old proxy data from the cache: */
    switch (oldProxyData.m_enmProxyState)
    {
        case UIProxyManager::ProxyState_Auto:     m_pRadioProxyAuto->setChecked(true); break;
        case UIProxyManager::ProxyState_Disabled: m_pRadioProxyDisabled->setChecked(true); break;
        case UIProxyManager::ProxyState_Enabled:  m_pRadioProxyEnabled->setChecked(true); break;
    }
    m_pHostEditor->setText(oldProxyData.m_strProxyHost);
    m_pPortEditor->setText(oldProxyData.m_strProxyPort);
    sltHandleProxyToggle();

    /* Revalidate: */
    revalidate();
}

void UIGlobalSettingsProxy::putToCache()
{
    /* Prepare new proxy data: */
    UIDataSettingsGlobalProxy newProxyData = m_pCache->base();

    /* Gather new proxy data: */
    newProxyData.m_enmProxyState = m_pRadioProxyEnabled->isChecked()  ? UIProxyManager::ProxyState_Enabled :
                                   m_pRadioProxyDisabled->isChecked() ? UIProxyManager::ProxyState_Disabled :
                                                                        UIProxyManager::ProxyState_Auto;
    newProxyData.m_strProxyHost = m_pHostEditor->text();
    newProxyData.m_strProxyPort = m_pPortEditor->text();

    /* Cache new proxy data: */
    m_pCache->cacheCurrentData(newProxyData);
}

void UIGlobalSettingsProxy::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to properties: */
    UISettingsPageGlobal::fetchData(data);

    /* Update proxy data and failing state: */
    setFailed(!saveProxyData());

    /* Upload properties to data: */
    UISettingsPageGlobal::uploadData(data);
}

bool UIGlobalSettingsProxy::validate(QList<UIValidationMessage> &messages)
{
    /* Pass if proxy is disabled: */
    if (!m_pRadioProxyEnabled->isChecked())
        return true;

    /* Pass by default: */
    bool fPass = true;

    /* Prepare message: */
    UIValidationMessage message;

    /* Check for host value: */
    if (m_pHostEditor->text().trimmed().isEmpty())
    {
        message.second << tr("No proxy host is currently specified.");
        fPass = false;
    }

    /* Check for port value: */
    if (m_pPortEditor->text().trimmed().isEmpty())
    {
        message.second << tr("No proxy port is currently specified.");
        fPass = false;
    }

    /* Serialize message: */
    if (!message.second.isEmpty())
        messages << message;

    /* Return result: */
    return fPass;
}

void UIGlobalSettingsProxy::retranslateUi()
{
    /* Translate uic generated strings: */
    Ui::UIGlobalSettingsProxy::retranslateUi(this);
}

void UIGlobalSettingsProxy::sltHandleProxyToggle()
{
    /* Update widgets availability: */
    m_pContainerProxy->setEnabled(m_pRadioProxyEnabled->isChecked());

    /* Revalidate: */
    revalidate();
}

void UIGlobalSettingsProxy::prepare()
{
    /* Apply UI decorations: */
    Ui::UIGlobalSettingsProxy::setupUi(this);

    /* Prepare cache: */
    m_pCache = new UISettingsCacheGlobalProxy;
    AssertPtrReturnVoid(m_pCache);

    /* Layout created in the .ui file. */
    {
        /* Create button-group: */
        QButtonGroup *pButtonGroup = new QButtonGroup(this);
        AssertPtrReturnVoid(pButtonGroup);
        {
            /* Configure button-group: */
            pButtonGroup->addButton(m_pRadioProxyAuto);
            pButtonGroup->addButton(m_pRadioProxyDisabled);
            pButtonGroup->addButton(m_pRadioProxyEnabled);
            connect(pButtonGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(sltHandleProxyToggle()));
        }

        /* Host editor created in the .ui file. */
        AssertPtrReturnVoid(m_pHostEditor);
        {
            /* Configure editor: */
            m_pHostEditor->setValidator(new QRegExpValidator(QRegExp("\\S+"), m_pHostEditor));
            connect(m_pHostEditor, SIGNAL(textEdited(const QString &)), this, SLOT(revalidate()));
        }

        /* Port editor created in the .ui file. */
        AssertPtrReturnVoid(m_pPortEditor);
        {
            /* Configure editor: */
            m_pPortEditor->setFixedWidthByText(QString().fill('0', 6));
            m_pPortEditor->setValidator(new QRegExpValidator(QRegExp("\\d+"), m_pPortEditor));
            connect(m_pPortEditor, SIGNAL(textEdited(const QString &)), this, SLOT(revalidate()));
        }
    }

    /* Apply language settings: */
    retranslateUi();
}

void UIGlobalSettingsProxy::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

bool UIGlobalSettingsProxy::saveProxyData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save proxy settings from the cache: */
    if (fSuccess && m_pCache->wasChanged())
    {
        /* Get old proxy data from the cache: */
        //const UIDataSettingsGlobalProxy &oldProxyData = m_pCache->base();
        /* Get new proxy data from the cache: */
        const UIDataSettingsGlobalProxy &newProxyData = m_pCache->data();

        /* Save new proxy data from the cache: */
        UIProxyManager proxyManager;
        proxyManager.setProxyState(newProxyData.m_enmProxyState);
        proxyManager.setProxyHost(newProxyData.m_strProxyHost);
        proxyManager.setProxyPort(newProxyData.m_strProxyPort);
        gEDataManager->setProxySettings(proxyManager.toString());
    }
    /* Return result: */
    return fSuccess;
}

