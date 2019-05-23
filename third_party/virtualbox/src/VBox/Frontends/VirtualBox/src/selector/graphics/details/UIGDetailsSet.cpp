/* $Id: UIGDetailsSet.cpp $ */
/** @file
 * VBox Qt GUI - UIGDetailsSet class implementation.
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

/* GUI includes: */
# include "UIGDetailsSet.h"
# include "UIGDetailsModel.h"
# include "UIGDetailsElements.h"
# include "UIVMItem.h"
# include "UIVirtualBoxEventHandler.h"
# include "VBoxGlobal.h"

/* COM includes: */
# include "CUSBController.h"
# include "CUSBDeviceFilters.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIGDetailsSet::UIGDetailsSet(UIGDetailsItem *pParent)
    : UIGDetailsItem(pParent)
    , m_pMachineItem(0)
    , m_fHasDetails(false)
    , m_configurationAccessLevel(ConfigurationAccessLevel_Null)
    , m_fFullSet(true)
    , m_pBuildStep(0)
    , m_iLastStepNumber(-1)
{
    /* Add set to the parent group: */
    parentItem()->addItem(this);

    /* Prepare set: */
    prepareSet();

    /* Prepare connections: */
    prepareConnections();
}

UIGDetailsSet::~UIGDetailsSet()
{
    /* Cleanup items: */
    clearItems();

    /* Remove set from the parent group: */
    parentItem()->removeItem(this);
}

void UIGDetailsSet::buildSet(UIVMItem *pMachineItem, bool fFullSet, const QMap<DetailsElementType, bool> &settings)
{
    /* Remember passed arguments: */
    m_pMachineItem = pMachineItem;
    m_machine = m_pMachineItem->machine();
    m_fHasDetails = m_pMachineItem->hasDetails();
    m_fFullSet = fFullSet;
    m_settings = settings;

    /* Cleanup superfluous items: */
    if (!m_fFullSet || !m_fHasDetails)
    {
        int iFirstItem = m_fHasDetails ? DetailsElementType_Display : DetailsElementType_General;
        int iLastItem = DetailsElementType_Description;
        bool fCleanupPerformed = false;
        for (int i = iFirstItem; i <= iLastItem; ++i)
            if (m_elements.contains(i))
            {
                delete m_elements[i];
                fCleanupPerformed = true;
            }
        if (fCleanupPerformed)
            updateGeometry();
    }

    /* Make sure we have details: */
    if (!m_fHasDetails)
    {
        /* Reset last-step number: */
        m_iLastStepNumber = -1;
        /* Notify parent group we are built: */
        emit sigBuildDone();
        return;
    }

    /* Choose last-step number: */
    m_iLastStepNumber = m_fFullSet ? DetailsElementType_Description : DetailsElementType_Preview;

    /* Fetch USB controller restrictions: */
    const CUSBDeviceFilters &filters = m_machine.GetUSBDeviceFilters();
    if (filters.isNull() || !m_machine.GetUSBProxyAvailable())
        m_settings.remove(DetailsElementType_USB);

    /* Start building set: */
    rebuildSet();
}

void UIGDetailsSet::sltBuildStep(QString strStepId, int iStepNumber)
{
    /* Cleanup build-step: */
    delete m_pBuildStep;
    m_pBuildStep = 0;

    /* Is step id valid? */
    if (strStepId != m_strSetId)
        return;

    /* Step number feats the bounds: */
    if (iStepNumber >= 0 && iStepNumber <= m_iLastStepNumber)
    {
        /* Load details settings: */
        DetailsElementType elementType = (DetailsElementType)iStepNumber;
        /* Should the element be visible? */
        bool fVisible = m_settings.contains(elementType);
        /* Should the element be opened? */
        bool fOpen = fVisible && m_settings[elementType];

        /* Check if element is present already: */
        UIGDetailsElement *pElement = element(elementType);
        if (pElement && fOpen)
            pElement->open(false);
        /* Create element if necessary: */
        bool fJustCreated = false;
        if (!pElement)
        {
            fJustCreated = true;
            pElement = createElement(elementType, fOpen);
        }

        /* Show element if necessary: */
        if (fVisible && !pElement->isVisible())
        {
            /* Show the element: */
            pElement->show();
            /* Recursively update size-hint: */
            pElement->updateGeometry();
            /* Update layout: */
            model()->updateLayout();
        }
        /* Hide element if necessary: */
        else if (!fVisible && pElement->isVisible())
        {
            /* Hide the element: */
            pElement->hide();
            /* Recursively update size-hint: */
            updateGeometry();
            /* Update layout: */
            model()->updateLayout();
        }
        /* Update model if necessary: */
        else if (fJustCreated)
            model()->updateLayout();

        /* For visible element: */
        if (pElement->isVisible())
        {
            /* Create next build-step: */
            m_pBuildStep = new UIBuildStep(this, pElement, strStepId, iStepNumber + 1);

            /* Build element: */
            pElement->updateAppearance();
        }
        /* For invisible element: */
        else
        {
            /* Just build next step: */
            sltBuildStep(strStepId, iStepNumber + 1);
        }
    }
    /* Step number out of bounds: */
    else
    {
        /* Update model: */
        model()->updateLayout();
        /* Repaint all the items: */
        foreach (UIGDetailsItem *pItem, items())
            pItem->update();
        /* Notify listener about build done: */
        emit sigBuildDone();
    }
}

void UIGDetailsSet::sltMachineStateChange(QString strId)
{
    /* Is this our VM changed? */
    if (m_machine.GetId() != strId)
        return;

    /* Update appearance: */
    rebuildSet();
}

void UIGDetailsSet::sltMachineAttributesChange(QString strId)
{
    /* Is this our VM changed? */
    if (m_machine.GetId() != strId)
        return;

    /* Update appearance: */
    rebuildSet();
}

void UIGDetailsSet::sltUpdateAppearance()
{
    /* Update appearance: */
    rebuildSet();
}

QString UIGDetailsSet::description() const
{
    return tr("Contains the details of virtual machine '%1'").arg(m_pMachineItem->name());
}

QVariant UIGDetailsSet::data(int iKey) const
{
    /* Provide other members with required data: */
    switch (iKey)
    {
        /* Layout hints: */
        case SetData_Margin: return 0;
        case SetData_Spacing: return QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) / 5;
        /* Default: */
        default: break;
    }
    return QVariant();
}

void UIGDetailsSet::addItem(UIGDetailsItem *pItem)
{
    switch (pItem->type())
    {
        case UIGDetailsItemType_Element:
        {
            UIGDetailsElement *pElement = pItem->toElement();
            DetailsElementType type = pElement->elementType();
            AssertMsg(!m_elements.contains(type), ("Element already added!"));
            m_elements.insert(type, pItem);
            break;
        }
        default:
        {
            AssertMsgFailed(("Invalid item type!"));
            break;
        }
    }
}

void UIGDetailsSet::removeItem(UIGDetailsItem *pItem)
{
    switch (pItem->type())
    {
        case UIGDetailsItemType_Element:
        {
            UIGDetailsElement *pElement = pItem->toElement();
            DetailsElementType type = pElement->elementType();
            AssertMsg(m_elements.contains(type), ("Element do not present (type = %d)!", (int)type));
            m_elements.remove(type);
            break;
        }
        default:
        {
            AssertMsgFailed(("Invalid item type!"));
            break;
        }
    }
}

QList<UIGDetailsItem*> UIGDetailsSet::items(UIGDetailsItemType type /* = UIGDetailsItemType_Element */) const
{
    switch (type)
    {
        case UIGDetailsItemType_Element: return m_elements.values();
        case UIGDetailsItemType_Any: return items(UIGDetailsItemType_Element);
        default: AssertMsgFailed(("Invalid item type!")); break;
    }
    return QList<UIGDetailsItem*>();
}

bool UIGDetailsSet::hasItems(UIGDetailsItemType type /* = UIGDetailsItemType_Element */) const
{
    switch (type)
    {
        case UIGDetailsItemType_Element: return !m_elements.isEmpty();
        case UIGDetailsItemType_Any: return hasItems(UIGDetailsItemType_Element);
        default: AssertMsgFailed(("Invalid item type!")); break;
    }
    return false;
}

void UIGDetailsSet::clearItems(UIGDetailsItemType type /* = UIGDetailsItemType_Element */)
{
    switch (type)
    {
        case UIGDetailsItemType_Element:
        {
            foreach (int iKey, m_elements.keys())
                delete m_elements[iKey];
            AssertMsg(m_elements.isEmpty(), ("Set items cleanup failed!"));
            break;
        }
        case UIGDetailsItemType_Any:
        {
            clearItems(UIGDetailsItemType_Element);
            break;
        }
        default:
        {
            AssertMsgFailed(("Invalid item type!"));
            break;
        }
    }
}

UIGDetailsElement* UIGDetailsSet::element(DetailsElementType elementType) const
{
    UIGDetailsItem *pItem = m_elements.value(elementType, 0);
    if (pItem)
        return pItem->toElement();
    return 0;
}

void UIGDetailsSet::prepareSet()
{
    /* Setup size-policy: */
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

void UIGDetailsSet::prepareConnections()
{
    /* Global-events connections: */
    connect(gVBoxEvents, SIGNAL(sigMachineStateChange(QString, KMachineState)), this, SLOT(sltMachineStateChange(QString)));
    connect(gVBoxEvents, SIGNAL(sigMachineDataChange(QString)), this, SLOT(sltMachineAttributesChange(QString)));
    connect(gVBoxEvents, SIGNAL(sigSessionStateChange(QString, KSessionState)), this, SLOT(sltMachineAttributesChange(QString)));
    connect(gVBoxEvents, SIGNAL(sigSnapshotTake(QString, QString)), this, SLOT(sltMachineAttributesChange(QString)));
    connect(gVBoxEvents, SIGNAL(sigSnapshotDelete(QString, QString)), this, SLOT(sltMachineAttributesChange(QString)));
    connect(gVBoxEvents, SIGNAL(sigSnapshotChange(QString, QString)), this, SLOT(sltMachineAttributesChange(QString)));
    connect(gVBoxEvents, SIGNAL(sigSnapshotRestore(QString, QString)), this, SLOT(sltMachineAttributesChange(QString)));

    /* Meidum-enumeration connections: */
    connect(&vboxGlobal(), SIGNAL(sigMediumEnumerationStarted()), this, SLOT(sltUpdateAppearance()));
    connect(&vboxGlobal(), SIGNAL(sigMediumEnumerationFinished()), this, SLOT(sltUpdateAppearance()));
}

int UIGDetailsSet::minimumWidthHint() const
{
    /* Zero if has no details: */
    if (!hasDetails())
        return 0;

    /* Prepare variables: */
    int iMargin = data(SetData_Margin).toInt();
    int iSpacing = data(SetData_Spacing).toInt();
    int iMinimumWidthHint = 0;

    /* Take into account all the elements: */
    foreach (UIGDetailsItem *pItem, items())
    {
        /* Skip hidden: */
        if (!pItem->isVisible())
            continue;

        /* For each particular element: */
        UIGDetailsElement *pElement = pItem->toElement();
        switch (pElement->elementType())
        {
            case DetailsElementType_General:
            case DetailsElementType_System:
            case DetailsElementType_Display:
            case DetailsElementType_Storage:
            case DetailsElementType_Audio:
            case DetailsElementType_Network:
            case DetailsElementType_Serial:
            case DetailsElementType_USB:
            case DetailsElementType_SF:
            case DetailsElementType_UI:
            case DetailsElementType_Description:
            {
                iMinimumWidthHint = qMax(iMinimumWidthHint, pItem->minimumWidthHint());
                break;
            }
            case DetailsElementType_Preview:
            {
                UIGDetailsItem *pGeneralItem = element(DetailsElementType_General);
                UIGDetailsItem *pSystemItem = element(DetailsElementType_System);
                int iGeneralElementWidth = pGeneralItem ? pGeneralItem->minimumWidthHint() : 0;
                int iSystemElementWidth = pSystemItem ? pSystemItem->minimumWidthHint() : 0;
                int iFirstColumnWidth = qMax(iGeneralElementWidth, iSystemElementWidth);
                iMinimumWidthHint = qMax(iMinimumWidthHint, iFirstColumnWidth + iSpacing + pItem->minimumWidthHint());
                break;
            }
            case DetailsElementType_Invalid: AssertFailed(); break; /* Shut up, MSC! */
        }
    }

    /* And two margins finally: */
    iMinimumWidthHint += 2 * iMargin;

    /* Return result: */
    return iMinimumWidthHint;
}

int UIGDetailsSet::minimumHeightHint() const
{
    /* Zero if has no details: */
    if (!hasDetails())
        return 0;

    /* Prepare variables: */
    int iMargin = data(SetData_Margin).toInt();
    int iSpacing = data(SetData_Spacing).toInt();
    int iMinimumHeightHint = 0;

    /* Take into account all the elements: */
    foreach (UIGDetailsItem *pItem, items())
    {
        /* Skip hidden: */
        if (!pItem->isVisible())
            continue;

        /* For each particular element: */
        UIGDetailsElement *pElement = pItem->toElement();
        switch (pElement->elementType())
        {
            case DetailsElementType_General:
            case DetailsElementType_System:
            case DetailsElementType_Display:
            case DetailsElementType_Storage:
            case DetailsElementType_Audio:
            case DetailsElementType_Network:
            case DetailsElementType_Serial:
            case DetailsElementType_USB:
            case DetailsElementType_SF:
            case DetailsElementType_UI:
            case DetailsElementType_Description:
            {
                iMinimumHeightHint += (pItem->minimumHeightHint() + iSpacing);
                break;
            }
            case DetailsElementType_Preview:
            {
                iMinimumHeightHint = qMax(iMinimumHeightHint, pItem->minimumHeightHint() + iSpacing);
                break;
            }
            case DetailsElementType_Invalid: AssertFailed(); break; /* Shut up, MSC! */
        }
    }

    /* Minus last spacing: */
    iMinimumHeightHint -= iSpacing;

    /* And two margins finally: */
    iMinimumHeightHint += 2 * iMargin;

    /* Return result: */
    return iMinimumHeightHint;
}

void UIGDetailsSet::updateLayout()
{
    /* Prepare variables: */
    int iMargin = data(SetData_Margin).toInt();
    int iSpacing = data(SetData_Spacing).toInt();
    int iMaximumWidth = geometry().size().toSize().width();
    int iVerticalIndent = iMargin;

    /* Layout all the elements: */
    foreach (UIGDetailsItem *pItem, items())
    {
        /* Skip hidden: */
        if (!pItem->isVisible())
            continue;

        /* For each particular element: */
        UIGDetailsElement *pElement = pItem->toElement();
        switch (pElement->elementType())
        {
            case DetailsElementType_General:
            case DetailsElementType_System:
            case DetailsElementType_Display:
            case DetailsElementType_Storage:
            case DetailsElementType_Audio:
            case DetailsElementType_Network:
            case DetailsElementType_Serial:
            case DetailsElementType_USB:
            case DetailsElementType_SF:
            case DetailsElementType_UI:
            case DetailsElementType_Description:
            {
                /* Move element: */
                pElement->setPos(iMargin, iVerticalIndent);
                /* Calculate required width: */
                int iWidth = iMaximumWidth - 2 * iMargin;
                if (pElement->elementType() == DetailsElementType_General ||
                    pElement->elementType() == DetailsElementType_System)
                    if (UIGDetailsElement *pPreviewElement = element(DetailsElementType_Preview))
                        if (pPreviewElement->isVisible())
                            iWidth -= (iSpacing + pPreviewElement->minimumWidthHint());
                /* If element width is wrong: */
                if (pElement->geometry().width() != iWidth)
                {
                    /* Resize element to required width: */
                    pElement->resize(iWidth, pElement->geometry().height());
                }
                /* Acquire required height: */
                int iHeight = pElement->minimumHeightHint();
                /* If element height is wrong: */
                if (pElement->geometry().height() != iHeight)
                {
                    /* Resize element to required height: */
                    pElement->resize(pElement->geometry().width(), iHeight);
                }
                /* Layout element content: */
                pItem->updateLayout();
                /* Advance indent: */
                iVerticalIndent += (iHeight + iSpacing);
                break;
            }
            case DetailsElementType_Preview:
            {
                /* Prepare variables: */
                int iWidth = pElement->minimumWidthHint();
                int iHeight = pElement->minimumHeightHint();
                /* Move element: */
                pElement->setPos(iMaximumWidth - iMargin - iWidth, iMargin);
                /* Resize element: */
                pElement->resize(iWidth, iHeight);
                /* Layout element content: */
                pItem->updateLayout();
                /* Advance indent: */
                iVerticalIndent = qMax(iVerticalIndent, iHeight + iSpacing);
                break;
            }
            case DetailsElementType_Invalid: AssertFailed(); break; /* Shut up, MSC! */
        }
    }
}

void UIGDetailsSet::rebuildSet()
{
    /* Make sure we have details: */
    if (!m_fHasDetails)
        return;

    /* Recache properties: */
    m_configurationAccessLevel = m_pMachineItem->configurationAccessLevel();

    /* Cleanup build-step: */
    delete m_pBuildStep;
    m_pBuildStep = 0;

    /* Generate new set-id: */
    m_strSetId = QUuid::createUuid().toString();

    /* Request to build first step: */
    emit sigBuildStep(m_strSetId, DetailsElementType_General);
}

UIGDetailsElement* UIGDetailsSet::createElement(DetailsElementType elementType, bool fOpen)
{
    /* Element factory: */
    switch (elementType)
    {
        case DetailsElementType_General:     return new UIGDetailsElementGeneral(this, fOpen);
        case DetailsElementType_System:      return new UIGDetailsElementSystem(this, fOpen);
        case DetailsElementType_Preview:     return new UIGDetailsElementPreview(this, fOpen);
        case DetailsElementType_Display:     return new UIGDetailsElementDisplay(this, fOpen);
        case DetailsElementType_Storage:     return new UIGDetailsElementStorage(this, fOpen);
        case DetailsElementType_Audio:       return new UIGDetailsElementAudio(this, fOpen);
        case DetailsElementType_Network:     return new UIGDetailsElementNetwork(this, fOpen);
        case DetailsElementType_Serial:      return new UIGDetailsElementSerial(this, fOpen);
        case DetailsElementType_USB:         return new UIGDetailsElementUSB(this, fOpen);
        case DetailsElementType_SF:          return new UIGDetailsElementSF(this, fOpen);
        case DetailsElementType_UI:          return new UIGDetailsElementUI(this, fOpen);
        case DetailsElementType_Description: return new UIGDetailsElementDescription(this, fOpen);
        case DetailsElementType_Invalid:     AssertFailed(); break; /* Shut up, MSC! */
    }
    return 0;
}

