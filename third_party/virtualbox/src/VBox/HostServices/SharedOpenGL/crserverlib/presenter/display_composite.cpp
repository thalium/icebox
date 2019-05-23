/* $Id: display_composite.cpp $ */

/** @file
 * Presenter API: display composite class implementation.
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

CrFbDisplayComposite::CrFbDisplayComposite() :
        mcDisplays(0)
{
    RTListInit(&mDisplays);
}


bool CrFbDisplayComposite::isComposite()
{
    return true;
}


uint32_t CrFbDisplayComposite::getDisplayCount()
{
    return mcDisplays;
}


bool CrFbDisplayComposite::add(CrFbDisplayBase *pDisplay)
{
    if (pDisplay->isInList())
    {
        WARN(("entry in list already"));
        return false;
    }

    RTListAppend(&mDisplays, &pDisplay->mNode);
    pDisplay->mpContainer = this;
    pDisplay->setFramebuffer(getFramebuffer());
    ++mcDisplays;
    return true;
}


bool CrFbDisplayComposite::remove(CrFbDisplayBase *pDisplay, bool fCleanupDisplay)
{
    if (pDisplay->getContainer() != this)
    {
        WARN(("invalid entry container"));
        return false;
    }

    RTListNodeRemove(&pDisplay->mNode);
    pDisplay->mpContainer = NULL;
    if (fCleanupDisplay)
        pDisplay->setFramebuffer(NULL);
    --mcDisplays;
    return true;
}


CrFbDisplayBase* CrFbDisplayComposite::first()
{
    return RTListGetFirstCpp(&mDisplays, CrFbDisplayBase, mNode);
}


CrFbDisplayBase* CrFbDisplayComposite::next(CrFbDisplayBase* pDisplay)
{
    if (pDisplay->getContainer() != this)
    {
        WARN(("invalid entry container"));
        return NULL;
    }

    return RTListGetNextCpp(&mDisplays, pDisplay, CrFbDisplayBase, mNode);
}


int CrFbDisplayComposite::setFramebuffer(struct CR_FRAMEBUFFER *pFb)
{
    CrFbDisplayBase::setFramebuffer(pFb);

    CrFbDisplayBase *pIter;
    RTListForEachCpp(&mDisplays, pIter, CrFbDisplayBase, mNode)
    {
        pIter->setFramebuffer(pFb);
    }

    return VINF_SUCCESS;
}


int CrFbDisplayComposite::UpdateBegin(struct CR_FRAMEBUFFER *pFb)
{
    int rc = CrFbDisplayBase::UpdateBegin(pFb);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    CrFbDisplayBase *pIter;
    RTListForEachCpp(&mDisplays, pIter, CrFbDisplayBase, mNode)
    {
        rc = pIter->UpdateBegin(pFb);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }
    return VINF_SUCCESS;
}


void CrFbDisplayComposite::UpdateEnd(struct CR_FRAMEBUFFER *pFb)
{
    CrFbDisplayBase *pIter;
    RTListForEachCpp(&mDisplays, pIter, CrFbDisplayBase, mNode)
    {
        pIter->UpdateEnd(pFb);
    }

    CrFbDisplayBase::UpdateEnd(pFb);
}


int CrFbDisplayComposite::EntryAdded(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayBase::EntryAdded(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    CrFbDisplayBase *pIter;
    RTListForEachCpp(&mDisplays, pIter, CrFbDisplayBase, mNode)
    {
        int rc = pIter->EntryAdded(pFb, hEntry);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }
    return VINF_SUCCESS;
}


int CrFbDisplayComposite::EntryCreated(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayBase::EntryAdded(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    CrFbDisplayBase *pIter;
    RTListForEachCpp(&mDisplays, pIter, CrFbDisplayBase, mNode)
    {
        int rc = pIter->EntryCreated(pFb, hEntry);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }
    return VINF_SUCCESS;
}


int CrFbDisplayComposite::EntryReplaced(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hNewEntry, HCR_FRAMEBUFFER_ENTRY hReplacedEntry)
{
    int rc = CrFbDisplayBase::EntryReplaced(pFb, hNewEntry, hReplacedEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    CrFbDisplayBase *pIter;
    RTListForEachCpp(&mDisplays, pIter, CrFbDisplayBase, mNode)
    {
        int rc = pIter->EntryReplaced(pFb, hNewEntry, hReplacedEntry);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }

    return VINF_SUCCESS;
}


int CrFbDisplayComposite::EntryTexChanged(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayBase::EntryTexChanged(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    CrFbDisplayBase *pIter;
    RTListForEachCpp(&mDisplays, pIter, CrFbDisplayBase, mNode)
    {
        int rc = pIter->EntryTexChanged(pFb, hEntry);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }
    return VINF_SUCCESS;
}


int CrFbDisplayComposite::EntryRemoved(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayBase::EntryRemoved(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    CrFbDisplayBase *pIter;
    RTListForEachCpp(&mDisplays, pIter, CrFbDisplayBase, mNode)
    {
        int rc = pIter->EntryRemoved(pFb, hEntry);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }
    return VINF_SUCCESS;
}


int CrFbDisplayComposite::EntryDestroyed(struct CR_FRAMEBUFFER *pFb, HCR_FRAMEBUFFER_ENTRY hEntry)
{
    int rc = CrFbDisplayBase::EntryDestroyed(pFb, hEntry);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    CrFbDisplayBase *pIter;
    RTListForEachCpp(&mDisplays, pIter, CrFbDisplayBase, mNode)
    {
        int rc = pIter->EntryDestroyed(pFb, hEntry);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }

    return VINF_SUCCESS;
}


int CrFbDisplayComposite::RegionsChanged(struct CR_FRAMEBUFFER *pFb)
{
    int rc = CrFbDisplayBase::RegionsChanged(pFb);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    CrFbDisplayBase *pIter;
    RTListForEachCpp(&mDisplays, pIter, CrFbDisplayBase, mNode)
    {
        int rc = pIter->RegionsChanged(pFb);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }
    return VINF_SUCCESS;
}


int CrFbDisplayComposite::FramebufferChanged(struct CR_FRAMEBUFFER *pFb)
{
    int rc = CrFbDisplayBase::FramebufferChanged(pFb);
    if (!RT_SUCCESS(rc))
    {
        WARN(("err"));
        return rc;
    }

    CrFbDisplayBase *pIter;
    RTListForEachCpp(&mDisplays, pIter, CrFbDisplayBase, mNode)
    {
        int rc = pIter->FramebufferChanged(pFb);
        if (!RT_SUCCESS(rc))
        {
            WARN(("err"));
            return rc;
        }
    }

    return VINF_SUCCESS;
}


CrFbDisplayComposite::~CrFbDisplayComposite()
{
    cleanup();
}


void CrFbDisplayComposite::cleanup(bool fCleanupDisplays)
{
    CrFbDisplayBase *pIter, *pIterNext;
    RTListForEachSafeCpp(&mDisplays, pIter, pIterNext, CrFbDisplayBase, mNode)
    {
        remove(pIter, fCleanupDisplays);
    }
}

