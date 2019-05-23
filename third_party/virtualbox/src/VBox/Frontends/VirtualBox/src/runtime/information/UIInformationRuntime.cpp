/* $Id: UIInformationRuntime.cpp $ */
/** @file
 * VBox Qt GUI - UIInformationRuntime class implementation.
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
# include "UIInformationRuntime.h"
# include "UIInformationDataItem.h"
# include "UIInformationItem.h"
# include "UIInformationView.h"
# include "UIExtraDataManager.h"
# include "UIInformationModel.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIInformationRuntime::UIInformationRuntime(QWidget *pParent, const CMachine &machine, const CConsole &console)
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

void UIInformationRuntime::prepareLayout()
{
    /* Create layout: */
    m_pMainLayout = new QVBoxLayout(this);
    AssertPtrReturnVoid(m_pMainLayout);
    {
        /* Configure layout: */
        m_pMainLayout->setSpacing(0);
    }
}

void UIInformationRuntime::prepareModel()
{
    /* Create information-model: */
    m_pModel = new UIInformationModel(this, m_machine, m_console);
    AssertPtrReturnVoid(m_pModel);
    {
        /* Create runtime-attributes data-item: */
        UIInformationDataRuntimeAttributes *pGeneral = new UIInformationDataRuntimeAttributes(m_machine, m_console, m_pModel);
        AssertPtrReturnVoid(pGeneral);
        {
            /* Add runtime-attributes data-item to model: */
            m_pModel->addItem(pGeneral);
        }

        /* Create network-statistics data-item: */
        UIInformationDataNetworkStatistics *pNetwork = new UIInformationDataNetworkStatistics(m_machine, m_console, m_pModel);
        AssertPtrReturnVoid(pNetwork);
        {
            /* Add network-statistics data-item to model: */
            m_pModel->addItem(pNetwork);
        }

        /* Create storage-statistics data-item: */
        UIInformationDataStorageStatistics *pStorage = new UIInformationDataStorageStatistics(m_machine, m_console, m_pModel);
        AssertPtrReturnVoid(pStorage);
        {
            /* Add storage-statistics data-item to model: */
            m_pModel->addItem(pStorage);
        }
    }
}

void UIInformationRuntime::prepareView()
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

