/* $Id: UIPopupStackViewport.cpp $ */
/** @file
 * VBox Qt GUI - UIPopupStackViewport class implementation.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
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
# include "UIPopupStackViewport.h"
# include "UIPopupPane.h"

/* Other VBox includes: */
# include <VBox/sup.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIPopupStackViewport::UIPopupStackViewport()
    : m_iLayoutMargin(1)
    , m_iLayoutSpacing(1)
{
}

bool UIPopupStackViewport::exists(const QString &strPopupPaneID) const
{
    /* Is there already popup-pane with the same ID? */
    return m_panes.contains(strPopupPaneID);
}

void UIPopupStackViewport::createPopupPane(const QString &strPopupPaneID,
                                           const QString &strMessage, const QString &strDetails,
                                           const QMap<int, QString> &buttonDescriptions)
{
    /* Make sure there is no such popup-pane already: */
    if (m_panes.contains(strPopupPaneID))
    {
        AssertMsgFailed(("Popup-pane already exists!"));
        return;
    }

    /* Create new popup-pane: */
    UIPopupPane *pPopupPane = m_panes[strPopupPaneID] = new UIPopupPane(this,
                                                                        strMessage, strDetails,
                                                                        buttonDescriptions);

    /* Attach popup-pane connection: */
    connect(this, &UIPopupStackViewport::sigProposePopupPaneSize, pPopupPane, &UIPopupPane::sltHandleProposalForSize);
    connect(pPopupPane, SIGNAL(sigSizeHintChanged()), this, SLOT(sltAdjustGeometry()));
    connect(pPopupPane, SIGNAL(sigDone(int)), this, SLOT(sltPopupPaneDone(int)));

    /* Show popup-pane: */
    pPopupPane->show();
}

void UIPopupStackViewport::updatePopupPane(const QString &strPopupPaneID,
                                           const QString &strMessage, const QString &strDetails)
{
    /* Make sure there is such popup-pane already: */
    if (!m_panes.contains(strPopupPaneID))
    {
        AssertMsgFailed(("Popup-pane doesn't exists!"));
        return;
    }

    /* Get existing popup-pane: */
    UIPopupPane *pPopupPane = m_panes[strPopupPaneID];

    /* Update message and details: */
    pPopupPane->setMessage(strMessage);
    pPopupPane->setDetails(strDetails);
}

void UIPopupStackViewport::recallPopupPane(const QString &strPopupPaneID)
{
    /* Make sure there is such popup-pane already: */
    if (!m_panes.contains(strPopupPaneID))
    {
        AssertMsgFailed(("Popup-pane doesn't exists!"));
        return;
    }

    /* Get existing popup-pane: */
    UIPopupPane *pPopupPane = m_panes[strPopupPaneID];

    /* Recall popup-pane: */
    pPopupPane->recall();
}

void UIPopupStackViewport::sltHandleProposalForSize(QSize newSize)
{
    /* Subtract layout margins: */
    newSize.setWidth(newSize.width() - 2 * m_iLayoutMargin);
    newSize.setHeight(newSize.height() - 2 * m_iLayoutMargin);

    /* Propagate resulting size to popups: */
    emit sigProposePopupPaneSize(newSize);
}

void UIPopupStackViewport::sltAdjustGeometry()
{
    /* Update size-hint: */
    updateSizeHint();

    /* Layout content: */
    layoutContent();

    /* Notify parent popup-stack: */
    emit sigSizeHintChanged();
}

void UIPopupStackViewport::sltPopupPaneDone(int iResultCode)
{
    /* Make sure the sender is the popup-pane: */
    UIPopupPane *pPopupPane = qobject_cast<UIPopupPane*>(sender());
    if (!pPopupPane)
    {
        AssertMsgFailed(("Should be called by popup-pane only!"));
        return;
    }

    /* Make sure the popup-pane still exists: */
    const QString strPopupPaneID(m_panes.key(pPopupPane, QString()));
    if (strPopupPaneID.isNull())
    {
        AssertMsgFailed(("Popup-pane already destroyed!"));
        return;
    }

    /* Notify listeners about popup-pane removal: */
    emit sigPopupPaneDone(strPopupPaneID, iResultCode);

    /* Delete popup-pane asyncronously.
     * To avoid issues with events which already posted: */
    m_panes.remove(strPopupPaneID);
    pPopupPane->deleteLater();

    /* Notify listeners about popup-pane removed: */
    emit sigPopupPaneRemoved(strPopupPaneID);

    /* Adjust geometry: */
    sltAdjustGeometry();

    /* Make sure this stack still contains popup-panes: */
    if (!m_panes.isEmpty())
        return;

    /* Notify listeners about popup-stack: */
    emit sigPopupPanesRemoved();
}

void UIPopupStackViewport::updateSizeHint()
{
    /* Calculate minimum width-hint: */
    int iMinimumWidthHint = 0;
    {
        /* Take into account all the panes: */
        foreach (UIPopupPane *pPane, m_panes)
            iMinimumWidthHint = qMax(iMinimumWidthHint, pPane->minimumSizeHint().width());

        /* And two margins finally: */
        iMinimumWidthHint += 2 * m_iLayoutMargin;
    }

    /* Calculate minimum height-hint: */
    int iMinimumHeightHint = 0;
    {
        /* Take into account all the panes: */
        foreach (UIPopupPane *pPane, m_panes)
            iMinimumHeightHint += pPane->minimumSizeHint().height();

        /* Take into account all the spacings, if any: */
        if (!m_panes.isEmpty())
            iMinimumHeightHint += (m_panes.size() - 1) * m_iLayoutSpacing;

        /* And two margins finally: */
        iMinimumHeightHint += 2 * m_iLayoutMargin;
    }

    /* Compose minimum size-hint: */
    m_minimumSizeHint = QSize(iMinimumWidthHint, iMinimumHeightHint);
}

void UIPopupStackViewport::layoutContent()
{
    /* Get attributes: */
    int iX = m_iLayoutMargin;
    int iY = m_iLayoutMargin;

    /* Layout every pane we have: */
    foreach (UIPopupPane *pPane, m_panes)
    {
        /* Get pane attributes: */
        QSize paneSize = pPane->minimumSizeHint();
        const int iPaneWidth = paneSize.width();
        const int iPaneHeight = paneSize.height();
        /* Adjust geometry for the pane: */
        pPane->setGeometry(iX, iY, iPaneWidth, iPaneHeight);
        pPane->layoutContent();
        /* Increment placeholder: */
        iY += (iPaneHeight + m_iLayoutSpacing);
    }
}

