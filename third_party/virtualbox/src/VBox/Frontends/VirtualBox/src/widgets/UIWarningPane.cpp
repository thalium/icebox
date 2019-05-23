/* $Id: UIWarningPane.cpp $ */
/** @file
 * VBox Qt GUI - UIWarningPane class implementation.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
# include <QHBoxLayout>
# include <QLabel>
# include <QEvent>
# include <QTimer>

/* GUI includes: */
# include "UIWarningPane.h"
# include "QIWidgetValidator.h"

/* Other VBox includes: */
# include <VBox/sup.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWarningPane::UIWarningPane(QWidget *pParent)
    : QWidget(pParent)
    , m_pIconLayout(0)
    , m_pTextLabel(0)
    , m_pHoverTimer(0)
    , m_iHoveredIconLabelPosition(-1)
{
    /* Prepare: */
    prepare();
}

void UIWarningPane::setWarningLabel(const QString &strWarningLabel)
{
    /* Assign passed text directly to warning-label: */
    m_pTextLabel->setText(strWarningLabel);
}

void UIWarningPane::registerValidator(UIPageValidator *pValidator)
{
    /* Make sure validator exists: */
    AssertPtrReturnVoid(pValidator);

    /* Make sure validator is not registered yet: */
    if (m_validators.contains(pValidator))
    {
        AssertMsgFailed(("Validator is registered already!\n"));
        return;
    }

    /* Register validator: */
    m_validators << pValidator;

    /* Create icon-label for newly registered validator: */
    QLabel *pIconLabel = new QLabel;
    {
        /* Add icon-label into list: */
        m_icons << pIconLabel;
        /* Add icon-label into layout: */
        m_pIconLayout->addWidget(pIconLabel);
        /* Configure icon-label: */
        pIconLabel->setMouseTracking(true);
        pIconLabel->installEventFilter(this);
        pIconLabel->setPixmap(pValidator->warningPixmap());
        connect(pValidator, SIGNAL(sigShowWarningIcon()), pIconLabel, SLOT(show()));
        connect(pValidator, SIGNAL(sigHideWarningIcon()), pIconLabel, SLOT(hide()));
    }

    /* Mark icon as 'unhovered': */
    m_hovered << false;
}

void UIWarningPane::sltHandleHoverTimer()
{
    /* Notify listeners about hovering: */
    if (m_iHoveredIconLabelPosition >= 0 && m_iHoveredIconLabelPosition < m_validators.size())
        emit sigHoverEnter(m_validators[m_iHoveredIconLabelPosition]);
}

void UIWarningPane::prepare()
{
    /* Prepare content: */
    prepareContent();
}

void UIWarningPane::prepareContent()
{
    /* Create main-layout: */
    QHBoxLayout *pMainLayout = new QHBoxLayout(this);
    {
        /* Configure layout: */
        pMainLayout->setContentsMargins(0, 0, 0, 0);
        /* Add left stretch: */
        pMainLayout->addStretch();
        /* Create text-label: */
        m_pTextLabel = new QLabel;
        {
            /* Add into main-layout: */
            pMainLayout->addWidget(m_pTextLabel);
        }
        /* Create layout: */
        m_pIconLayout = new QHBoxLayout;
        {
            /* Configure layout: */
            m_pIconLayout->setContentsMargins(0, 0, 0, 0);
            /* Add into main-layout: */
            pMainLayout->addLayout(m_pIconLayout);
        }
        /* Create hover-timer: */
        m_pHoverTimer = new QTimer(this);
        {
            /* Configure timer: */
            m_pHoverTimer->setInterval(200);
            m_pHoverTimer->setSingleShot(true);
            connect(m_pHoverTimer, SIGNAL(timeout()), this, SLOT(sltHandleHoverTimer()));
        }
        /* Add right stretch: */
        pMainLayout->addStretch();
    }
}

bool UIWarningPane::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    /* Depending on event-type: */
    switch (pEvent->type())
    {
        /* One of icons hovered: */
        case QEvent::MouseMove:
        {
            /* Cast object to label: */
            if (QLabel *pIconLabel = qobject_cast<QLabel*>(pWatched))
            {
                /* Search for the corresponding icon: */
                if (m_icons.contains(pIconLabel))
                {
                    /* Mark icon-label hovered if not yet: */
                    int iIconLabelPosition = m_icons.indexOf(pIconLabel);
                    if (!m_hovered[iIconLabelPosition])
                    {
                        m_hovered[iIconLabelPosition] = true;
                        m_iHoveredIconLabelPosition = iIconLabelPosition;
                        m_pHoverTimer->start();
                    }
                }
            }
            break;
        }
        /* One of icons unhovered: */
        case QEvent::Leave:
        {
            /* Cast object to label: */
            if (QLabel *pIconLabel = qobject_cast<QLabel*>(pWatched))
            {
                /* Search for the corresponding icon: */
                if (m_icons.contains(pIconLabel))
                {
                    /* Mark icon-label unhovered if not yet: */
                    int iIconLabelPosition = m_icons.indexOf(pIconLabel);
                    if (m_hovered[iIconLabelPosition])
                    {
                        m_hovered[iIconLabelPosition] = false;
                        if (m_pHoverTimer->isActive())
                        {
                            m_pHoverTimer->stop();
                            m_iHoveredIconLabelPosition = -1;
                        }
                        else
                            emit sigHoverLeave(m_validators[iIconLabelPosition]);
                    }
                }
            }
            break;
        }
        /* Default case: */
        default: break;
    }
    /* Call to base-class: */
    return QWidget::eventFilter(pWatched, pEvent);
}

