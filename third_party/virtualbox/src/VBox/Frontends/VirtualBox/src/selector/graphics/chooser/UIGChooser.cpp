/* $Id: UIGChooser.cpp $ */
/** @file
 * VBox Qt GUI - UIGChooser class implementation.
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

/* Qt includes: */
# include <QVBoxLayout>
# include <QStatusBar>
# include <QStyle>

/* GUI includes: */
# include "UIGChooser.h"
# include "UIGChooserModel.h"
# include "UIGChooserView.h"
# include "UISelectorWindow.h"
# include "VBoxGlobal.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIGChooser::UIGChooser(UISelectorWindow *pParent)
    : QWidget(pParent)
    , m_pSelectorWindow(pParent)
    , m_pMainLayout(0)
    , m_pChooserModel(0)
    , m_pChooserView(0)
{
    /* Prepare palette: */
    preparePalette();

    /* Prepare layout: */
    prepareLayout();

    /* Prepare model: */
    prepareModel();

    /* Prepare view: */
    prepareView();

    /* Prepare connections: */
    prepareConnections();

    /* Load: */
    load();
}

UIGChooser::~UIGChooser()
{
    /* Save: */
    save();
}

UIActionPool* UIGChooser::actionPool() const
{
    return selector()->actionPool();
}

UIVMItem* UIGChooser::currentItem() const
{
    return m_pChooserModel->currentMachineItem();
}

QList<UIVMItem*> UIGChooser::currentItems() const
{
    return m_pChooserModel->currentMachineItems();
}

bool UIGChooser::isSingleGroupSelected() const
{
    return m_pChooserModel->isSingleGroupSelected();
}

bool UIGChooser::isAllItemsOfOneGroupSelected() const
{
    return m_pChooserModel->isAllItemsOfOneGroupSelected();
}

bool UIGChooser::isGroupSavingInProgress() const
{
    return m_pChooserModel->isGroupSavingInProgress();
}

void UIGChooser::preparePalette()
{
    /* Setup palette: */
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, pal.color(QPalette::Active, QPalette::Base));
    setPalette(pal);
}

void UIGChooser::prepareLayout()
{
    /* Setup main-layout: */
    m_pMainLayout = new QVBoxLayout(this);
    const int iR = qApp->style()->pixelMetric(QStyle::PM_LayoutRightMargin) / 9;
    m_pMainLayout->setContentsMargins(0, 0, iR, 0);
    m_pMainLayout->setSpacing(0);
}

void UIGChooser::prepareModel()
{
    /* Setup chooser-model: */
    m_pChooserModel = new UIGChooserModel(this);
}

void UIGChooser::prepareView()
{
    /* Setup chooser-view: */
    m_pChooserView = new UIGChooserView(this);
    m_pChooserView->setScene(m_pChooserModel->scene());
    m_pChooserView->show();
    setFocusProxy(m_pChooserView);
    m_pMainLayout->addWidget(m_pChooserView);
}

void UIGChooser::prepareConnections()
{
    /* Setup chooser-model connections: */
    connect(m_pChooserModel, SIGNAL(sigRootItemMinimumWidthHintChanged(int)),
            m_pChooserView, SLOT(sltMinimumWidthHintChanged(int)));
    connect(m_pChooserModel, SIGNAL(sigRootItemMinimumHeightHintChanged(int)),
            m_pChooserView, SLOT(sltMinimumHeightHintChanged(int)));
    connect(m_pChooserModel, SIGNAL(sigFocusChanged(UIGChooserItem*)),
            m_pChooserView, SLOT(sltFocusChanged(UIGChooserItem*)));

    /* Setup chooser-view connections: */
    connect(m_pChooserView, SIGNAL(sigResized()),
            m_pChooserModel, SLOT(sltHandleViewResized()));
}

void UIGChooser::load()
{
    /* Prepare model: */
    m_pChooserModel->prepare();
}

void UIGChooser::save()
{
    /* Cleanup model: */
    m_pChooserModel->cleanup();
}

