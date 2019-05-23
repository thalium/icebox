/* $Id: UIGDetails.cpp $ */
/** @file
 * VBox Qt GUI - UIGDetails class implementation.
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
# include <QApplication>
# include <QStyle>
# include <QVBoxLayout>

/* GUI includes: */
# include "UIGDetails.h"
# include "UIGDetailsModel.h"
# include "UIGDetailsView.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIGDetails::UIGDetails(QWidget *pParent /* = 0 */)
    : QWidget(pParent)
    , m_pMainLayout(0)
    , m_pDetailsModel(0)
    , m_pDetailsView(0)
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
}

void UIGDetails::setItems(const QList<UIVMItem*> &items)
{
    /* Propagate to details-model: */
    m_pDetailsModel->setItems(items);
}

void UIGDetails::preparePalette()
{
    /* Setup palette: */
    setAutoFillBackground(true);
    QPalette pal = qApp->palette();
    pal.setColor(QPalette::Window, pal.color(QPalette::Active, QPalette::Window));
    setPalette(pal);
}

void UIGDetails::prepareLayout()
{
    /* Setup main-layout: */
    m_pMainLayout = new QVBoxLayout(this);
    const int iL = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 9;
    m_pMainLayout->setContentsMargins(iL, 0, 0, 0);
    m_pMainLayout->setSpacing(0);
}

void UIGDetails::prepareModel()
{
    /* Setup details-model: */
    m_pDetailsModel = new UIGDetailsModel(this);
}

void UIGDetails::prepareView()
{
    /* Setup details-view: */
    m_pDetailsView = new UIGDetailsView(this);
    m_pDetailsView->setScene(m_pDetailsModel->scene());
    m_pDetailsView->show();
    setFocusProxy(m_pDetailsView);
    m_pMainLayout->addWidget(m_pDetailsView);
}

void UIGDetails::prepareConnections()
{
    /* Setup details-model connections: */
    connect(m_pDetailsModel, SIGNAL(sigRootItemMinimumWidthHintChanged(int)),
            m_pDetailsView, SLOT(sltMinimumWidthHintChanged(int)));
    connect(m_pDetailsModel, SIGNAL(sigRootItemMinimumHeightHintChanged(int)),
            m_pDetailsView, SLOT(sltMinimumHeightHintChanged(int)));
    connect(m_pDetailsModel, SIGNAL(sigLinkClicked(const QString&, const QString&, const QString&)),
            this, SIGNAL(sigLinkClicked(const QString&, const QString&, const QString&)));
    connect(this, SIGNAL(sigSlidingStarted()),
            m_pDetailsModel, SLOT(sltHandleSlidingStarted()));
    connect(this, SIGNAL(sigToggleStarted()),
            m_pDetailsModel, SLOT(sltHandleToggleStarted()));
    connect(this, SIGNAL(sigToggleFinished()),
            m_pDetailsModel, SLOT(sltHandleToggleFinished()));

    /* Setup details-view connections: */
    connect(m_pDetailsView, SIGNAL(sigResized()),
            m_pDetailsModel, SLOT(sltHandleViewResize()));
}

