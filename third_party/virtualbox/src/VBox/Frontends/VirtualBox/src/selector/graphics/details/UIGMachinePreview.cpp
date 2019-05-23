/* $Id: UIGMachinePreview.cpp $ */
/** @file
 * VBox Qt GUI - UIGMachinePreview class implementation.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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
# include <QGraphicsSceneContextMenuEvent>
# include <QMenu>
# include <QPainter>
# include <QTimer>

/* GUI includes: */
# include "UIGMachinePreview.h"
# include "UIVirtualBoxEventHandler.h"
# include "UIExtraDataManager.h"
# include "UIImageTools.h"
# include "UIConverter.h"
# include "UIIconPool.h"
# include "VBoxGlobal.h"

/* COM includes: */
# include "CConsole.h"
# include "CDisplay.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* VirtualBox interface declarations: */
#ifndef VBOX_WITH_XPCOM
# include "VirtualBox.h"
#else /* !VBOX_WITH_XPCOM */
# include "VirtualBox_XPCOM.h"
#endif /* VBOX_WITH_XPCOM */

UIGMachinePreview::UIGMachinePreview(QIGraphicsWidget *pParent)
    : QIWithRetranslateUI4<QIGraphicsWidget>(pParent)
    , m_pUpdateTimer(new QTimer(this))
    , m_pUpdateTimerMenu(0)
    , m_dRatio((double)QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 16)
    , m_iMargin(0)
    , m_preset(AspectRatioPreset_16x9)
    , m_pPreviewImg(0)
{
    /* Create session instance: */
    m_session.createInstance(CLSID_Session);

    /* Cache aspect-ratio preset settings: */
    const QIcon empty16x10 = UIIconPool::iconSet(":/preview_empty_16to10_242x167px.png");
    const QIcon empty16x9 = UIIconPool::iconSet(":/preview_empty_16to9_242x155px.png");
    const QIcon empty4x3 = UIIconPool::iconSet(":/preview_empty_4to3_242x192px.png");
    const QIcon full16x10 = UIIconPool::iconSet(":/preview_full_16to10_242x167px.png");
    const QIcon full16x9 = UIIconPool::iconSet(":/preview_full_16to9_242x155px.png");
    const QIcon full4x3 = UIIconPool::iconSet(":/preview_full_4to3_242x192px.png");

    // WORKAROUND:
    // Since we don't have x3 and x4 HiDPI icons yet,
    // and we hadn't enabled automatic up-scaling for now,
    // we have to make sure m_dRatio is within possible bounds.
    const QList<QSize> sizes = empty16x10.availableSizes();
    if (sizes.size() >= 2)
        m_dRatio = qMin(m_dRatio, (double)sizes.last().width() / sizes.first().width());

    m_sizes.insert(AspectRatioPreset_16x10, QSize(242 * m_dRatio, 167 * m_dRatio));
    m_sizes.insert(AspectRatioPreset_16x9, QSize(242 * m_dRatio, 155 * m_dRatio));
    m_sizes.insert(AspectRatioPreset_4x3, QSize(242 * m_dRatio, 192 * m_dRatio));
    m_ratios.insert(AspectRatioPreset_16x10, (double)16/10);
    m_ratios.insert(AspectRatioPreset_16x9, (double)16/9);
    m_ratios.insert(AspectRatioPreset_4x3, (double)4/3);
    m_emptyPixmaps.insert(AspectRatioPreset_16x10, new QPixmap(empty16x10.pixmap(m_sizes.value(AspectRatioPreset_16x10))));
    m_emptyPixmaps.insert(AspectRatioPreset_16x9, new QPixmap(empty16x9.pixmap(m_sizes.value(AspectRatioPreset_16x9))));
    m_emptyPixmaps.insert(AspectRatioPreset_4x3, new QPixmap(empty4x3.pixmap(m_sizes.value(AspectRatioPreset_4x3))));
    m_fullPixmaps.insert(AspectRatioPreset_16x10, new QPixmap(full16x10.pixmap(m_sizes.value(AspectRatioPreset_16x10))));
    m_fullPixmaps.insert(AspectRatioPreset_16x9, new QPixmap(full16x9.pixmap(m_sizes.value(AspectRatioPreset_16x9))));
    m_fullPixmaps.insert(AspectRatioPreset_4x3, new QPixmap(full4x3.pixmap(m_sizes.value(AspectRatioPreset_4x3))));

    /* Setup contents (depends on presets above!): */
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    /* Create the context menu: */
    m_pUpdateTimerMenu = new QMenu;
    QActionGroup *pUpdateTimeG = new QActionGroup(this);
    pUpdateTimeG->setExclusive(true);
    for(int i = 0; i < PreviewUpdateIntervalType_Max; ++i)
    {
        QAction *pUpdateTime = new QAction(pUpdateTimeG);
        pUpdateTime->setData(i);
        pUpdateTime->setCheckable(true);
        pUpdateTimeG->addAction(pUpdateTime);
        m_pUpdateTimerMenu->addAction(pUpdateTime);
        m_actions[static_cast<PreviewUpdateIntervalType>(i)] = pUpdateTime;
    }
    m_pUpdateTimerMenu->insertSeparator(m_actions[static_cast<PreviewUpdateIntervalType>(PreviewUpdateIntervalType_500ms)]);

    /* Initialize with the new update interval: */
    setUpdateInterval(gEDataManager->selectorWindowPreviewUpdateInterval(), false);

    /* Setup connections: */
    connect(m_pUpdateTimer, SIGNAL(timeout()), this, SLOT(sltRecreatePreview()));
    connect(gVBoxEvents, SIGNAL(sigMachineStateChange(QString, KMachineState)),
            this, SLOT(sltMachineStateChange(QString)));

    /* Retranslate the UI */
    retranslateUi();
}

UIGMachinePreview::~UIGMachinePreview()
{
    /* Close any open session: */
    if (m_session.GetState() == KSessionState_Locked)
        m_session.UnlockMachine();

    /* Destroy background images: */
    foreach (const AspectRatioPreset &preset, m_emptyPixmaps.keys())
    {
        delete m_emptyPixmaps.value(preset);
        m_emptyPixmaps.remove(preset);
    }
    foreach (const AspectRatioPreset &preset, m_fullPixmaps.keys())
    {
        delete m_fullPixmaps.value(preset);
        m_fullPixmaps.remove(preset);
    }

    /* Destroy preview image: */
    if (m_pPreviewImg)
        delete m_pPreviewImg;

    /* Destroy update timer: */
    if (m_pUpdateTimerMenu)
        delete m_pUpdateTimerMenu;
}

void UIGMachinePreview::setMachine(const CMachine& machine)
{
    /* Pause: */
    stop();

    /* Assign new machine: */
    m_machine = machine;

    /* Fetch machine data: */
    m_strPreviewName = tr("No preview");
    if (!m_machine.isNull())
        m_strPreviewName = m_machine.GetAccessible() ? m_machine.GetName() :
                           QApplication::translate("UIVMListView", "Inaccessible");

    /* Resume: */
    restart();
}

CMachine UIGMachinePreview::machine() const
{
    return m_machine;
}

void UIGMachinePreview::sltMachineStateChange(QString strId)
{
    /* Make sure its the event for our machine: */
    if (m_machine.isNull() || m_machine.GetId() != strId)
        return;

    /* Restart the preview: */
    restart();
}

void UIGMachinePreview::sltRecreatePreview()
{
    /* Skip invisible preview: */
    if (!isVisible())
        return;

    /* Cleanup previous image: */
    if (m_pPreviewImg)
    {
        delete m_pPreviewImg;
        m_pPreviewImg = 0;
    }

    /* Fetch actual machine-state: */
    const KMachineState machineState = m_machine.isNull() ? KMachineState_Null : m_machine.GetState();

    /* We are creating preview only for assigned and accessible VMs: */
    if (!m_machine.isNull() && machineState != KMachineState_Null &&
        m_vRect.width() > 0 && m_vRect.height() > 0)
    {
        /* Prepare image: */
        QImage image;

        /* Use 10x9 as the aspect-ratio preset by default: */
        AspectRatioPreset preset = AspectRatioPreset_16x9;

        /* Preview update enabled? */
        if (m_pUpdateTimer->interval() > 0)
        {
            /* Depending on machine state: */
            switch (machineState)
            {
                /* If machine is in SAVED/RESTORING state: */
                case KMachineState_Saved:
                case KMachineState_Restoring:
                {
                    /* Use the screenshot from saved-state if possible: */
                    ULONG uGuestWidth = 0, uGuestHeight = 0;
                    QVector<BYTE> screenData = m_machine.ReadSavedScreenshotToArray(0, KBitmapFormat_PNG, uGuestWidth, uGuestHeight);

                    /* Make sure screen-data is OK: */
                    if (!m_machine.isOk() || screenData.isEmpty())
                        break;

                    if (uGuestWidth > 0 && uGuestHeight > 0)
                    {
                        /* Calculate aspect-ratio: */
                        const double dAspectRatio = (double)uGuestWidth / uGuestHeight;
                        /* Look for the best aspect-ratio preset: */
                        preset = bestAspectRatioPreset(dAspectRatio, m_ratios);
                    }

                    /* Create image based on shallow copy or screenshot data,
                     * scale image down if necessary to the size possible to reflect: */
                    image = QImage::fromData(screenData.data(), screenData.size(), "PNG")
                            .scaled(imageAspectRatioSize(m_vRect.size(), QSize(uGuestWidth, uGuestHeight)),
                                    Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                    /* And detach that copy to make it deep: */
                    image.detach();
                    /* Dim image to give it required look: */
                    dimImage(image);
                    break;
                }
                /* If machine is in RUNNING/PAUSED state: */
                case KMachineState_Running:
                case KMachineState_Paused:
                {
                    /* Make sure session state is Locked: */
                    if (m_session.GetState() != KSessionState_Locked)
                        break;

                    /* Make sure console is OK: */
                    CConsole console = m_session.GetConsole();
                    if (!m_session.isOk() || console.isNull())
                        break;
                    /* Make sure display is OK: */
                    CDisplay display = console.GetDisplay();
                    if (!console.isOk() || display.isNull())
                        break;

                    /* Acquire guest-screen attributes: */
                    LONG iOriginX = 0, iOriginY = 0;
                    ULONG uGuestWidth = 0, uGuestHeight = 0, uBpp = 0;
                    KGuestMonitorStatus monitorStatus = KGuestMonitorStatus_Enabled;
                    display.GetScreenResolution(0, uGuestWidth, uGuestHeight, uBpp, iOriginX, iOriginY, monitorStatus);
                    if (uGuestWidth > 0 && uGuestHeight > 0)
                    {
                        /* Calculate aspect-ratio: */
                        const double dAspectRatio = (double)uGuestWidth / uGuestHeight;
                        /* Look for the best aspect-ratio preset: */
                        preset = bestAspectRatioPreset(dAspectRatio, m_ratios);
                    }

                    /* Calculate size corresponding to aspect-ratio: */
                    const QSize size = imageAspectRatioSize(m_vRect.size(), QSize(uGuestWidth, uGuestHeight));

                    /* Use direct VM content: */
                    QVector<BYTE> screenData = display.TakeScreenShotToArray(0, size.width(), size.height(), KBitmapFormat_BGR0);

                    /* Make sure screen-data is OK: */
                    if (!display.isOk() || screenData.isEmpty())
                        break;

                    /* Make sure screen-data size is valid: */
                    const int iExpectedSize = size.width() * size.height() * 4;
                    const int iActualSize = screenData.size();
                    if (iActualSize != iExpectedSize)
                    {
                        AssertMsgFailed(("Invalid screen-data size '%d', should be '%d'!\n", iActualSize, iExpectedSize));
                        break;
                    }

                    /* Create image based on shallow copy of acquired data: */
                    QImage tempImage(screenData.data(), size.width(), size.height(), QImage::Format_RGB32);
                    image = tempImage;
                    /* And detach that copy to make it deep: */
                    image.detach();
                    /* Dim image to give it required look for PAUSED state: */
                    if (machineState == KMachineState_Paused)
                        dimImage(image);
                    break;
                }
                default:
                    break;
            }
        }

        /* If image initialized: */
        if (!image.isNull())
        {
            /* Shallow copy that image: */
            m_pPreviewImg = new QImage(image);
            /* And detach that copy to make it deep: */
            m_pPreviewImg->detach();
        }

        /* If preset changed: */
        if (m_preset != preset)
        {
            /* Save new preset: */
            m_preset = preset;
            /* And update geometry: */
            updateGeometry();
            emit sigSizeHintChanged();
        }
    }

    /* Redraw preview in any case: */
    update();
}

void UIGMachinePreview::resizeEvent(QGraphicsSceneResizeEvent *pEvent)
{
    recalculatePreviewRectangle();
    sltRecreatePreview();
    QIGraphicsWidget::resizeEvent(pEvent);
}

void UIGMachinePreview::showEvent(QShowEvent *pEvent)
{
    restart();
    QIGraphicsWidget::showEvent(pEvent);
}

void UIGMachinePreview::hideEvent(QHideEvent *pEvent)
{
    stop();
    QIGraphicsWidget::hideEvent(pEvent);
}

void UIGMachinePreview::contextMenuEvent(QGraphicsSceneContextMenuEvent *pEvent)
{
    QAction *pReturn = m_pUpdateTimerMenu->exec(pEvent->screenPos(), 0);
    if (pReturn)
    {
        PreviewUpdateIntervalType interval = static_cast<PreviewUpdateIntervalType>(pReturn->data().toInt());
        setUpdateInterval(interval, true);
        restart();
    }
}

void UIGMachinePreview::retranslateUi()
{
    m_actions.value(PreviewUpdateIntervalType_Disabled)->setText(tr("Update disabled"));
    m_actions.value(PreviewUpdateIntervalType_500ms)->setText(tr("Every 0.5 s"));
    m_actions.value(PreviewUpdateIntervalType_1000ms)->setText(tr("Every 1 s"));
    m_actions.value(PreviewUpdateIntervalType_2000ms)->setText(tr("Every 2 s"));
    m_actions.value(PreviewUpdateIntervalType_5000ms)->setText(tr("Every 5 s"));
    m_actions.value(PreviewUpdateIntervalType_10000ms)->setText(tr("Every 10 s"));
}

QSizeF UIGMachinePreview::sizeHint(Qt::SizeHint which, const QSizeF &constraint /* = QSizeF() */) const
{
    if (which == Qt::MinimumSize)
    {
        AssertReturn(m_emptyPixmaps.contains(m_preset),
                     QIGraphicsWidget::sizeHint(which, constraint));
        QSize size = m_sizes.value(m_preset);
        if (m_iMargin != 0)
        {
            size.setWidth(size.width() - 2 * m_iMargin);
            size.setHeight(size.height() - 2 * m_iMargin);
        }
        return size;
    }
    return QIGraphicsWidget::sizeHint(which, constraint);
}

void UIGMachinePreview::paint(QPainter *pPainter, const QStyleOptionGraphicsItem*, QWidget*)
{
    /* Where should the content go: */
    QRect cr = contentsRect().toRect();
    if (!cr.isValid())
        return;

    /* If there is a preview image available: */
    if (m_pPreviewImg)
    {
        /* Draw empty monitor frame: */
        pPainter->drawPixmap(cr.x() + m_iMargin, cr.y() + m_iMargin, *m_emptyPixmaps.value(m_preset));

        /* Move image to viewport center: */
        QRect imageRect(QPoint(0, 0), m_pPreviewImg->size());
        imageRect.moveCenter(m_vRect.center());

#ifdef VBOX_WS_MAC
        /* Set composition-mode to opaque: */
        pPainter->setCompositionMode(QPainter::CompositionMode_Source);
        /* Replace translucent background with black one: */
        pPainter->fillRect(imageRect, QColor(Qt::black));
        /* Return default composition-mode back: */
        pPainter->setCompositionMode(QPainter::CompositionMode_SourceAtop);
#endif /* VBOX_WS_MAC */

        /* Draw preview image: */
        pPainter->drawImage(imageRect.topLeft(), *m_pPreviewImg);
    }
    else
    {
        /* Draw full monitor frame: */
        pPainter->drawPixmap(cr.x() + m_iMargin, cr.y() + m_iMargin, *m_fullPixmaps.value(m_preset));

        /* Paint preview name: */
        QFont font = pPainter->font();
        font.setBold(true);
        int fFlags = Qt::AlignCenter | Qt::TextWordWrap;
        float h = m_vRect.size().height() * .2;
        QRect r;
        /* Make a little magic to find out if the given text fits into our rectangle.
         * Decrease the font pixel size as long as it doesn't fit. */
        int cMax = 30;
        do
        {
            h = h * .8;
            font.setPixelSize((int)h);
            pPainter->setFont(font);
            r = pPainter->boundingRect(m_vRect, fFlags, m_strPreviewName);
        }
        while ((r.height() > m_vRect.height() || r.width() > m_vRect.width()) && cMax-- != 0);
        pPainter->setPen(Qt::white);
        pPainter->drawText(m_vRect, fFlags, m_strPreviewName);
    }
}

void UIGMachinePreview::setUpdateInterval(PreviewUpdateIntervalType interval, bool fSave)
{
    switch (interval)
    {
        case PreviewUpdateIntervalType_Disabled:
        {
            /* Stop the timer: */
            m_pUpdateTimer->stop();
            /* And continue with other cases: */
        }
        RT_FALL_THRU();
        case PreviewUpdateIntervalType_500ms:
        case PreviewUpdateIntervalType_1000ms:
        case PreviewUpdateIntervalType_2000ms:
        case PreviewUpdateIntervalType_5000ms:
        case PreviewUpdateIntervalType_10000ms:
        {
            /* Set the timer interval: */
            m_pUpdateTimer->setInterval(gpConverter->toInternalInteger(interval));
            /* Check corresponding action: */
            m_actions[interval]->setChecked(true);
            break;
        }
        case PreviewUpdateIntervalType_Max:
            break;
    }
    if (fSave)
        gEDataManager->setSelectorWindowPreviewUpdateInterval(interval);
}

void UIGMachinePreview::recalculatePreviewRectangle()
{
    /* Contents rectangle: */
    QRect cr = contentsRect().toRect();
    m_vRect = cr.adjusted( 21 * m_dRatio + m_iMargin,  21 * m_dRatio + m_iMargin,
                          -21 * m_dRatio - m_iMargin, -21 * m_dRatio - m_iMargin);
}

void UIGMachinePreview::restart()
{
    /* Fetch the latest machine-state: */
    KMachineState machineState = m_machine.isNull() ? KMachineState_Null : m_machine.GetState();

    /* Reopen session if necessary: */
    if (m_session.GetState() == KSessionState_Locked)
        m_session.UnlockMachine();
    if (!m_machine.isNull())
    {
        /* Lock the session for the current machine: */
        if (machineState == KMachineState_Running || machineState == KMachineState_Paused)
            m_machine.LockMachine(m_session, KLockType_Shared);
    }

    /* Recreate the preview image: */
    sltRecreatePreview();

    /* Start the timer if necessary: */
    if (!m_machine.isNull())
    {
        if (m_pUpdateTimer->interval() > 0 && machineState == KMachineState_Running)
            m_pUpdateTimer->start();
    }
}

void UIGMachinePreview::stop()
{
    /* Stop the timer: */
    m_pUpdateTimer->stop();
}

/* static */
UIGMachinePreview::AspectRatioPreset UIGMachinePreview::bestAspectRatioPreset(const double dAspectRatio,
                                                                              const QMap<AspectRatioPreset, double> &ratios)
{
    /* Use 16x9 preset as the 'best' by 'default': */
    AspectRatioPreset bestPreset = AspectRatioPreset_16x9;
    /* Calculate minimum diff based on 'default' preset: */
    double dMinimumDiff = qAbs(dAspectRatio - ratios.value(bestPreset));
    /* Now look for the 'best' aspect-ratio preset among existing: */
    for (AspectRatioPreset currentPreset = AspectRatioPreset_16x10;
         currentPreset <= AspectRatioPreset_4x3;
         currentPreset = (AspectRatioPreset)(currentPreset + 1))
    {
        /* Calculate current diff based on 'current' preset: */
        const double dDiff = qAbs(dAspectRatio - ratios.value(currentPreset));
        /* If new 'best' preset found: */
        if (dDiff < dMinimumDiff)
        {
            /* Remember new diff: */
            dMinimumDiff = dDiff;
            /* And new preset: */
            bestPreset = currentPreset;
        }
    }
    /* Return 'best' preset: */
    return bestPreset;
}

/* static */
QSize UIGMachinePreview::imageAspectRatioSize(const QSize &hostSize, const QSize &guestSize)
{
    /* Make sure host-size and guest-size are valid: */
    AssertReturn(!hostSize.isNull(), QSize());
    if (guestSize.isNull())
        return hostSize;

    /* Calculate host/guest aspect-ratio: */
    const double dHostAspectRatio = (double)hostSize.width() / hostSize.height();
    const double dGuestAspectRatio = (double)guestSize.width() / guestSize.height();
    int iWidth = 0, iHeight = 0;
    /* Guest-screen more thin by vertical than host-screen: */
    if (dGuestAspectRatio >= dHostAspectRatio)
    {
        /* Get host width: */
        iWidth = hostSize.width();
        /* And calculate height based on guest aspect ratio: */
        iHeight = (int)((double)iWidth / dGuestAspectRatio);
        /* But no more than host height: */
        iHeight = qMin(iHeight, hostSize.height());
    }
    /* Host-screen more thin by vertical than guest-screen: */
    else
    {
        /* Get host height: */
        iHeight = hostSize.height();
        /* And calculate width based on guest aspect ratio: */
        iWidth = (int)((double)iHeight * dGuestAspectRatio);
        /* But no more than host width: */
        iWidth = qMin(iWidth, hostSize.width());
    }
    /* Return actual size: */
    return QSize(iWidth, iHeight);
}

