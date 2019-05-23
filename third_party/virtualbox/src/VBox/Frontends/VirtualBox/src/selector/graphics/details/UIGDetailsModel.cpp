/* $Id: UIGDetailsModel.cpp $ */
/** @file
 * VBox Qt GUI - UIGDetailsModel class implementation.
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
# include <QGraphicsScene>
# include <QGraphicsSceneContextMenuEvent>
# include <QGraphicsView>

/* GUI includes: */
# include "UIGDetails.h"
# include "UIGDetailsModel.h"
# include "UIGDetailsGroup.h"
# include "UIGDetailsElement.h"
# include "UIExtraDataManager.h"
# include "VBoxGlobal.h"
# include "UIConverter.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIGDetailsModel::UIGDetailsModel(UIGDetails *pParent)
    : QObject(pParent)
    , m_pDetails(pParent)
    , m_pScene(0)
    , m_pRoot(0)
    , m_pAnimationCallback(0)
{
    /* Prepare scene: */
    prepareScene();

    /* Prepare root: */
    prepareRoot();

    /* Load settings: */
    loadSettings();

    /* Register meta-type: */
    qRegisterMetaType<DetailsElementType>();
}

UIGDetailsModel::~UIGDetailsModel()
{
    /* Save settings: */
    saveSettings();

    /* Cleanup root: */
    cleanupRoot();

    /* Cleanup scene: */
    cleanupScene();
 }

QGraphicsScene* UIGDetailsModel::scene() const
{
    return m_pScene;
}

QGraphicsView* UIGDetailsModel::paintDevice() const
{
    if (!m_pScene || m_pScene->views().isEmpty())
        return 0;
    return m_pScene->views().first();
}

QGraphicsItem* UIGDetailsModel::itemAt(const QPointF &position) const
{
    return scene()->itemAt(position, QTransform());
}

UIGDetailsItem *UIGDetailsModel::root() const
{
    return m_pRoot;
}

void UIGDetailsModel::updateLayout()
{
    /* Prepare variables: */
    int iSceneMargin = data(DetailsModelData_Margin).toInt();
    QSize viewportSize = paintDevice()->viewport()->size();
    int iViewportWidth = viewportSize.width() - 2 * iSceneMargin;
    int iViewportHeight = viewportSize.height() - 2 * iSceneMargin;

    /* Move root: */
    m_pRoot->setPos(iSceneMargin, iSceneMargin);
    /* Resize root: */
    m_pRoot->resize(iViewportWidth, iViewportHeight);
    /* Layout root content: */
    m_pRoot->updateLayout();
}

void UIGDetailsModel::setItems(const QList<UIVMItem*> &items)
{
    m_pRoot->buildGroup(items);
}

void UIGDetailsModel::sltHandleViewResize()
{
    /* Relayout: */
    updateLayout();
}

void UIGDetailsModel::sltToggleElements(DetailsElementType type, bool fToggled)
{
    /* Make sure it is not started yet: */
    if (m_pAnimationCallback)
        return;

    /* Prepare/configure animation callback: */
    m_pAnimationCallback = new UIGDetailsElementAnimationCallback(this, type, fToggled);
    connect(m_pAnimationCallback, SIGNAL(sigAllAnimationFinished(DetailsElementType, bool)),
            this, SLOT(sltToggleAnimationFinished(DetailsElementType, bool)), Qt::QueuedConnection);
    /* For each the set of the group: */
    foreach (UIGDetailsItem *pSetItem, m_pRoot->items())
    {
        /* For each the element of the set: */
        foreach (UIGDetailsItem *pElementItem, pSetItem->items())
        {
            /* Get each element: */
            UIGDetailsElement *pElement = pElementItem->toElement();
            /* Check if this element is of required type: */
            if (pElement->elementType() == type)
            {
                if (fToggled && pElement->closed())
                {
                    m_pAnimationCallback->addNotifier(pElement);
                    pElement->open();
                }
                else if (!fToggled && pElement->opened())
                {
                    m_pAnimationCallback->addNotifier(pElement);
                    pElement->close();
                }
            }
        }
    }
    /* Update layout: */
    updateLayout();
}

void UIGDetailsModel::sltToggleAnimationFinished(DetailsElementType type, bool fToggled)
{
    /* Cleanup animation callback: */
    delete m_pAnimationCallback;
    m_pAnimationCallback = 0;

    /* Mark animation finished: */
    foreach (UIGDetailsItem *pSetItem, m_pRoot->items())
    {
        foreach (UIGDetailsItem *pElementItem, pSetItem->items())
        {
            UIGDetailsElement *pElement = pElementItem->toElement();
            if (pElement->elementType() == type)
                pElement->markAnimationFinished();
        }
    }
    /* Update layout: */
    updateLayout();

    /* Update element open/close status: */
    if (m_settings.contains(type))
        m_settings[type] = fToggled;
}

void UIGDetailsModel::sltElementTypeToggled()
{
    /* Which item was toggled? */
    QAction *pAction = qobject_cast<QAction*>(sender());
    DetailsElementType type = pAction->data().value<DetailsElementType>();

    /* Toggle element visibility status: */
    if (m_settings.contains(type))
        m_settings.remove(type);
    else
        m_settings[type] = true;

    /* Rebuild group: */
    m_pRoot->rebuildGroup();
}

void UIGDetailsModel::sltHandleSlidingStarted()
{
    m_pRoot->stopBuildingGroup();
}

void UIGDetailsModel::sltHandleToggleStarted()
{
    m_pRoot->stopBuildingGroup();
}

void UIGDetailsModel::sltHandleToggleFinished()
{
    m_pRoot->rebuildGroup();
}

QVariant UIGDetailsModel::data(int iKey) const
{
    switch (iKey)
    {
        case DetailsModelData_Margin: return 0;
        default: break;
    }
    return QVariant();
}

void UIGDetailsModel::prepareScene()
{
    m_pScene = new QGraphicsScene(this);
    m_pScene->installEventFilter(this);
}

void UIGDetailsModel::prepareRoot()
{
    m_pRoot = new UIGDetailsGroup(scene());
}

void UIGDetailsModel::loadSettings()
{
    /* Load settings: */
    m_settings = gEDataManager->selectorWindowDetailsElements();
    /* If settings are empty: */
    if (m_settings.isEmpty())
    {
        /* Propose the defaults: */
        m_settings[DetailsElementType_General] = true;
        m_settings[DetailsElementType_Preview] = true;
        m_settings[DetailsElementType_System] = true;
        m_settings[DetailsElementType_Display] = true;
        m_settings[DetailsElementType_Storage] = true;
        m_settings[DetailsElementType_Audio] = true;
        m_settings[DetailsElementType_Network] = true;
        m_settings[DetailsElementType_USB] = true;
        m_settings[DetailsElementType_SF] = true;
        m_settings[DetailsElementType_Description] = true;
    }
}

void UIGDetailsModel::saveSettings()
{
    /* Save settings: */
    gEDataManager->setSelectorWindowDetailsElements(m_settings);
}

void UIGDetailsModel::cleanupRoot()
{
    delete m_pRoot;
    m_pRoot = 0;
}

void UIGDetailsModel::cleanupScene()
{
    delete m_pScene;
    m_pScene = 0;
}

bool UIGDetailsModel::eventFilter(QObject *pObject, QEvent *pEvent)
{
    /* Ignore if no scene object: */
    if (pObject != scene())
        return QObject::eventFilter(pObject, pEvent);

    /* Ignore if no context-menu event: */
    if (pEvent->type() != QEvent::GraphicsSceneContextMenu)
        return QObject::eventFilter(pObject, pEvent);

    /* Process context menu event: */
    return processContextMenuEvent(static_cast<QGraphicsSceneContextMenuEvent*>(pEvent));
}

bool UIGDetailsModel::processContextMenuEvent(QGraphicsSceneContextMenuEvent *pEvent)
{
    /* Pass preview context menu instead: */
    if (QGraphicsItem *pItem = itemAt(pEvent->scenePos()))
        if (pItem->type() == UIGDetailsItemType_Preview)
            return false;

    /* Prepare context-menu: */
    QMenu contextMenu;
    /* Enumerate elements settings: */
    for (int iType = DetailsElementType_General; iType <= DetailsElementType_Description; ++iType)
    {
        DetailsElementType currentElementType = (DetailsElementType)iType;
        QAction *pAction = contextMenu.addAction(gpConverter->toString(currentElementType), this, SLOT(sltElementTypeToggled()));
        pAction->setCheckable(true);
        pAction->setChecked(m_settings.contains(currentElementType));
        pAction->setData(QVariant::fromValue(currentElementType));
    }
    /* Exec context-menu: */
    contextMenu.exec(pEvent->screenPos());

    /* Filter: */
    return true;
}

UIGDetailsElementAnimationCallback::UIGDetailsElementAnimationCallback(QObject *pParent, DetailsElementType type, bool fToggled)
    : QObject(pParent)
    , m_type(type)
    , m_fToggled(fToggled)
{
}

void UIGDetailsElementAnimationCallback::addNotifier(UIGDetailsItem *pItem)
{
    /* Connect notifier: */
    connect(pItem, SIGNAL(sigToggleElementFinished()), this, SLOT(sltAnimationFinished()));
    /* Remember notifier: */
    m_notifiers << pItem;
}

void UIGDetailsElementAnimationCallback::sltAnimationFinished()
{
    /* Determine notifier: */
    UIGDetailsItem *pItem = qobject_cast<UIGDetailsItem*>(sender());
    /* Disconnect notifier: */
    disconnect(pItem, SIGNAL(sigToggleElementFinished()), this, SLOT(sltAnimationFinished()));
    /* Remove notifier: */
    m_notifiers.removeAll(pItem);
    /* Check if we finished: */
    if (m_notifiers.isEmpty())
        emit sigAllAnimationFinished(m_type, m_fToggled);
}

