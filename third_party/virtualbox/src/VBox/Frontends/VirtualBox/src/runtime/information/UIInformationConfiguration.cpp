/* $Id: UIInformationConfiguration.cpp $ */
/** @file
 * VBox Qt GUI - UIInformationConfiguration class implementation.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
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
# include <QVBoxLayout>
# include <QApplication>

/* GUI includes: */
# include "UIInformationConfiguration.h"
# include "UIInformationDataItem.h"
# include "UIInformationItem.h"
# include "UIInformationView.h"
# include "UIExtraDataManager.h"
# include "UIInformationModel.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIInformationConfiguration::UIInformationConfiguration(QWidget *pParent, const CMachine &machine, const CConsole &console)
    : QWidget(pParent)
    , m_machine(machine)
    , m_console(console)
    , m_pMainLayout(0)
    , m_pModel(0)
    , m_pView(0)
{
    /* Prepare layout: */
    prepareLayout();

    /* Prepare model: */
    prepareModel();

    /* Prepare view: */
    prepareView();
}

void UIInformationConfiguration::prepareLayout()
{
    /* Create layout: */
    m_pMainLayout = new QVBoxLayout(this);
    AssertPtrReturnVoid(m_pMainLayout);
    {
        /* Configure layout: */
        m_pMainLayout->setSpacing(0);
    }
}

void UIInformationConfiguration::prepareModel()
{
    /* Create information-model: */
    m_pModel = new UIInformationModel(this, m_machine, m_console);
    AssertPtrReturnVoid(m_pModel);
    {
        /* Create general data-item: */
        UIInformationDataItem *pGeneral = new UIInformationDataGeneral(m_machine, m_console, m_pModel);
        AssertPtrReturnVoid(pGeneral);
        {
            /* Add general data-item to model: */
            m_pModel->addItem(pGeneral);
        }

        /* Create system data-item: */
        UIInformationDataItem *pSystem = new UIInformationDataSystem(m_machine, m_console, m_pModel);
        AssertPtrReturnVoid(pSystem);
        {
            /* Add system data-item to model: */
            m_pModel->addItem(pSystem);
        }

        /* Create display data-item: */
        UIInformationDataItem *pDisplay = new UIInformationDataDisplay(m_machine, m_console, m_pModel);
        AssertPtrReturnVoid(pDisplay);
        {
            /* Add display data-item to model: */
            m_pModel->addItem(pDisplay);
        }

        /* Create storage data-item: */
        UIInformationDataItem *pStorage = new UIInformationDataStorage(m_machine, m_console, m_pModel);
        AssertPtrReturnVoid(pStorage);
        {
            /* Add storage data-item to model: */
            m_pModel->addItem(pStorage);
        }

        /* Create audio data-item: */
        UIInformationDataItem *pAudio = new UIInformationDataAudio(m_machine, m_console, m_pModel);
        AssertPtrReturnVoid(pAudio);
        {
            /* Add audio data-item to model: */
            m_pModel->addItem(pAudio);
        }

        /* Create network data-item: */
        UIInformationDataItem *pNetwork = new UIInformationDataNetwork(m_machine, m_console, m_pModel);
        AssertPtrReturnVoid(pNetwork);
        {
            /* Add network data-item to model: */
            m_pModel->addItem(pNetwork);
        }

        /* Create serial-ports data-item: */
        UIInformationDataItem *pSerialPorts = new UIInformationDataSerialPorts(m_machine, m_console, m_pModel);
        AssertPtrReturnVoid(pSerialPorts);
        {
            /* Add serial-ports data-item to model: */
            m_pModel->addItem(pSerialPorts);
        }

        /* Create usb data-item: */
        UIInformationDataItem *pUSB = new UIInformationDataUSB(m_machine, m_console, m_pModel);
        AssertPtrReturnVoid(pUSB);
        {
            /* Add usb data-item to model: */
            m_pModel->addItem(pUSB);
        }

        /* Create shared-folders data-item: */
        UIInformationDataItem *pSharedFolders = new UIInformationDataSharedFolders(m_machine, m_console, m_pModel);
        AssertPtrReturnVoid(pSharedFolders);
        {
            /* Add shared-folders data-item to model: */
            m_pModel->addItem(pSharedFolders);
        }
    }
}

void UIInformationConfiguration::prepareView()
{
    /* Create information-view: */
    m_pView = new UIInformationView;
    AssertPtrReturnVoid(m_pView);
    {
        /* Configure information-view: */
        m_pView->setResizeMode(QListView::Adjust);

        /* Create information-delegate item: */
        UIInformationItem *pItem = new UIInformationItem(m_pView);
        AssertPtrReturnVoid(pItem);
        {
            /* Set item-delegate for information-view: */
            m_pView->setItemDelegate(pItem);
        }
        /* Connect data changed signal: */
        connect(m_pModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
                m_pView, SLOT(updateData(const QModelIndex &, const QModelIndex &)));

        /* Set model for view: */
        m_pView->setModel(m_pModel);
        /* Add information-view to the layout: */
        m_pMainLayout->addWidget(m_pView);
    }
}

