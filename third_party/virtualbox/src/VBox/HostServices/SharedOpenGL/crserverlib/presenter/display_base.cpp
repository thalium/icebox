/* $Id: display_base.cpp $ */

/** @file
 * Presenter API: display base class implementation.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "server_presenter.h"

CrFbDisplayBase::CrFbDisplayBase() :
        mpContainer(NULL),
        mpFb(NULL),
        mcUpdates(0),
        mhSlot(CRHTABLE_HANDLE_INVALID)
{
    mFlags.u32Value = 0;
}


CrFbDisplayBase::~CrFbDisplayBase()
{
    Assert(!mcUpdates);

    if (mpContainer)
        mpContainer->remove(this);
}


bool CrFbDisplayBase::isComposite()
{
    return false;
}


class CrFbDisplayComposite* CrFbDisplayBase::getContainer()
{
    return mpContainer;
}


bool CrFbDisplayBase::isInList()
{
    return !!mpContainer;
}


bool CrFbDisplayBase::isUpdating()
{
    return !!mcUpdates;
}


int CrFbDisplayBase::setRegionsChanged()
{
    if (!mcUpdates)
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }

    mFlags.fRegionsShanged = 1;
    return VINF_SUCCESS;
}


int CrFbDisplayBase::setFramebuffer(struct CR_FRAMEBUFFER *pFb)
{
    if (mcUpdates)
    {
        WARN(("trying to set framebuffer while update is in progress"));
        return VERR_INVALID_STATE;
    }

    if (mpFb == pFb)
        return VINF_SUCCESS;

    int rc = setFramebufferBegin(pFb);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    if (mpFb)
    {
        rc = fbCleanup();
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            setFramebufferEnd(pFb);
            return rc;
        }
    }

     mpFb = pFb;

    if (mpFb)
    {
        rc = fbSync();
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            setFramebufferEnd(pFb);
            return rc;
        }
    }

    setFramebufferEnd(pFb);
    return VINF_SUCCESS;
}


struct CR_FRAMEBUFFER* CrFbDisplayBase::getFramebuffer()
{
    return mpFb;
}


int CrFbDisplayBase::UpdateBegin(struct CR_FRAMEBUFFER *pFb)
{
    ++mcUpdates;
    Assert(!mFlags.fRegionsShanged || mcUpdates > 1);
    return VINF_SUCCESS;
}


void CrFbDisplayBase::UpdateEnd(struct CR_FRAMEBUFFER *pFb)
{
    --mcUpdates;
    Assert(mcUpdates < UINT32_MAX/2);
    if (!mcUpdates)
        onUpdateEnd();
}


int CrFbDisplayBase::EntryCreated(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    if (!mcUpdates)
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }
    return VINF_SUCCESS;
}


int CrFbDisplayBase::EntryAdded(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    if (!mcUpdates)
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }
    mFlags.fRegionsShanged = 1;
    return VINF_SUCCESS;
}


int CrFbDisplayBase::EntryReplaced(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hNewEntry,
    HCR_FRAMEBUFFER_ENTRY hReplacedEntry)
{
    if (!mcUpdates)
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }
    return VINF_SUCCESS;
}


int CrFbDisplayBase::EntryTexChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    if (!mcUpdates)
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }
    return VINF_SUCCESS;
}


int CrFbDisplayBase::EntryRemoved(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    if (!mcUpdates)
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }
    mFlags.fRegionsShanged = 1;
    return VINF_SUCCESS;
}


int CrFbDisplayBase::EntryDestroyed(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    return VINF_SUCCESS;
}


int CrFbDisplayBase::EntryPosChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    if (!mcUpdates)
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }
    mFlags.fRegionsShanged = 1;
    return VINF_SUCCESS;
}


int CrFbDisplayBase::RegionsChanged(struct CR_FRAMEBUFFER *pFb)
{
    if (!mcUpdates)
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }
    mFlags.fRegionsShanged = 1;
    return VINF_SUCCESS;
}


int CrFbDisplayBase::FramebufferChanged(struct CR_FRAMEBUFFER *pFb)
{
    if (!mcUpdates)
    {
        WARN(("err"));
        return VERR_INVALID_STATE;
    }
    return VINF_SUCCESS;
}


void CrFbDisplayBase::onUpdateEnd()
{
    if (mFlags.fRegionsShanged)
    {
        mFlags.fRegionsShanged = 0;
        if (getFramebuffer()) /*<-dont't do anything on cleanup*/
            ueRegions();
    }
}


void CrFbDisplayBase::ueRegions()
{
}


DECLCALLBACK(bool) CrFbDisplayBase::entriesCreateCb(HCR_FRAMEBUFFER hFb, HCR_FRAMEBUFFER_ENTRY hEntry, void *pvContext)
{
    int rc = ((ICrFbDisplay*)(pvContext))->EntryCreated(hFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
    }
    return true;
}


DECLCALLBACK(bool) CrFbDisplayBase::entriesDestroyCb(HCR_FRAMEBUFFER hFb, HCR_FRAMEBUFFER_ENTRY hEntry, void *pvContext)
{
    int rc = ((ICrFbDisplay*)(pvContext))->EntryDestroyed(hFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
    }
    return true;
}


int CrFbDisplayBase::fbSynchAddAllEntries()
{
    VBOXVR_SCR_COMPOSITOR_CONST_ITERATOR Iter;
    const VBOXVR_SCR_COMPOSITOR_ENTRY *pEntry;

    CrVrScrCompositorConstIterInit(CrFbGetCompositor(mpFb), &Iter);
    int rc = VINF_SUCCESS;

    CrFbVisitCreatedEntries(mpFb, entriesCreateCb, this);

    while ((pEntry = CrVrScrCompositorConstIterNext(&Iter)) != NULL)
    {
        HCR_FRAMEBUFFER_ENTRY hEntry = CrFbEntryFromCompositorEntry(pEntry);

        rc = EntryAdded(mpFb, hEntry);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            EntryDestroyed(mpFb, hEntry);
            break;
        }
    }

    return rc;
}


int CrFbDisplayBase::fbCleanupRemoveAllEntries()
{
    VBOXVR_SCR_COMPOSITOR_CONST_ITERATOR Iter;
    const VBOXVR_SCR_COMPOSITOR_ENTRY *pEntry;

    CrVrScrCompositorConstIterInit(CrFbGetCompositor(mpFb), &Iter);

    int rc = VINF_SUCCESS;

    while ((pEntry = CrVrScrCompositorConstIterNext(&Iter)) != NULL)
    {
        HCR_FRAMEBUFFER_ENTRY hEntry = CrFbEntryFromCompositorEntry(pEntry);
        rc = EntryRemoved(mpFb, hEntry);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            break;
        }

        CrFbVisitCreatedEntries(mpFb, entriesDestroyCb, this);
    }

    return rc;
}


int CrFbDisplayBase::setFramebufferBegin(struct CR_FRAMEBUFFER *pFb)
{
    return UpdateBegin(pFb);
}


void CrFbDisplayBase::setFramebufferEnd(struct CR_FRAMEBUFFER *pFb)
{
    UpdateEnd(pFb);
}


DECLCALLBACK(void) CrFbDisplayBase::slotEntryReleaseCB(HCR_FRAMEBUFFER hFb, HCR_FRAMEBUFFER_ENTRY hEntry, void *pvContext)
{
}


void CrFbDisplayBase::slotRelease()
{
    Assert(mhSlot);
    CrFbDDataReleaseSlot(mpFb, mhSlot, slotEntryReleaseCB, this);
}


int CrFbDisplayBase::fbCleanup()
{
    if (mhSlot)
    {
        slotRelease();
        mhSlot = 0;
    }

    mpFb = NULL;
    return VINF_SUCCESS;
}


int CrFbDisplayBase::fbSync()
{
    return VINF_SUCCESS;
}


CRHTABLE_HANDLE CrFbDisplayBase::slotGet()
{
    if (!mhSlot)
    {
        if (mpFb)
            mhSlot = CrFbDDataAllocSlot(mpFb);
    }

    return mhSlot;
}

