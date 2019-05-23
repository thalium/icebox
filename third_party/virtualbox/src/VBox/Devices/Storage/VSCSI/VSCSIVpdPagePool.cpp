/* $Id: VSCSIVpdPagePool.cpp $ */
/** @file
 * Virtual SCSI driver: VPD page pool
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_VSCSI
#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/mem.h>
#include <iprt/assert.h>

#include "VSCSIInternal.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * A VSCSI VPD page.
 */
typedef struct VSCSIVPDPAGE
{
    /** List node. */
    RTLISTNODE    NodePages;
    /** Page size. */
    size_t        cbPage;
    /** Page data - variable size. */
    uint8_t       abPage[1];
} VSCSIVPDPAGE;
/** Pointer to a VPD page. */
typedef VSCSIVPDPAGE *PVSCSIVPDPAGE;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

int vscsiVpdPagePoolInit(PVSCSIVPDPOOL pVScsiVpdPool)
{
    RTListInit(&pVScsiVpdPool->ListPages);
    return VINF_SUCCESS;
}


void vscsiVpdPagePoolDestroy(PVSCSIVPDPOOL pVScsiVpdPool)
{
    PVSCSIVPDPAGE pIt, pItNext;

    RTListForEachSafe(&pVScsiVpdPool->ListPages, pIt, pItNext, VSCSIVPDPAGE, NodePages)
    {
        RTListNodeRemove(&pIt->NodePages);
        RTMemFree(pIt);
    }
}


int vscsiVpdPagePoolAllocNewPage(PVSCSIVPDPOOL pVScsiVpdPool, uint8_t uPage, size_t cbPage, uint8_t **ppbPage)
{
    int rc = VINF_SUCCESS;
    PVSCSIVPDPAGE pPage;

    /* Check that the page doesn't exist already. */
    RTListForEach(&pVScsiVpdPool->ListPages, pPage, VSCSIVPDPAGE, NodePages)
    {
        if (pPage->abPage[1] == uPage)
            return VERR_ALREADY_EXISTS;
    }

    pPage = (PVSCSIVPDPAGE)RTMemAllocZ(RT_OFFSETOF(VSCSIVPDPAGE, abPage[cbPage]));
    if (pPage)
    {
        pPage->cbPage    = cbPage;
        pPage->abPage[1] = uPage;
        RTListAppend(&pVScsiVpdPool->ListPages, &pPage->NodePages);
        *ppbPage = &pPage->abPage[0];
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}


int vscsiVpdPagePoolQueryPage(PVSCSIVPDPOOL pVScsiVpdPool, PVSCSIREQINT pVScsiReq, uint8_t uPage)
{
    PVSCSIVPDPAGE pPage;

    /* Check that the page doesn't exist already. */
    RTListForEach(&pVScsiVpdPool->ListPages, pPage, VSCSIVPDPAGE, NodePages)
    {
        if (pPage->abPage[1] == uPage)
        {
            RTSgBufCopyFromBuf(&pVScsiReq->SgBuf, &pPage->abPage[0], pPage->cbPage);
            return VINF_SUCCESS;
        }
    }

    return VERR_NOT_FOUND;
}

