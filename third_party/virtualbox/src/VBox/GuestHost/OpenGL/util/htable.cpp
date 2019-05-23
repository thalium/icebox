/* $Id: htable.cpp $ */
/** @file
 * uint32_t handle to void simple table impl
 *
 * @todo Why couldn't you simply use iprt/handletable.h?
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

#include <iprt/cdefs.h>
#include <iprt/asm.h>
#include "cr_spu.h"
#include "cr_vreg.h"

#include "cr_htable.h"
#include "cr_spu.h"
#include "chromium.h"
#include "cr_error.h"
#include "cr_net.h"
#include "cr_rand.h"
#include "cr_mem.h"
#include "cr_string.h"

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/mem.h>
#include <iprt/err.h>

VBOXHTABLEDECL(int) CrHTableCreate(PCRHTABLE pTbl, uint32_t cSize)
{
    memset(pTbl, 0, sizeof (*pTbl));
    if (!cSize)
        return VINF_SUCCESS;
    pTbl->paData = (void**)RTMemAllocZ(sizeof (pTbl->paData[0]) * cSize);
    if (pTbl->paData)
    {
        pTbl->cSize = cSize;
        return VINF_SUCCESS;
    }
    WARN(("RTMemAllocZ failed!"));
    return VERR_NO_MEMORY;
}

VBOXHTABLEDECL(void) CrHTableDestroy(PCRHTABLE pTbl)
{
    if (!pTbl->paData)
        return;

    RTMemFree(pTbl->paData);
}

int crHTableRealloc(PCRHTABLE pTbl, uint32_t cNewSize)
{
    Assert(cNewSize > pTbl->cSize);
    if (cNewSize > pTbl->cSize)
    {
        void **pvNewData = (void**)RTMemAllocZ(sizeof (pTbl->paData[0]) * cNewSize);
        if (!pvNewData)
        {
            WARN(("RTMemAllocZ failed for size (%d)", (int)(sizeof (pTbl->paData[0]) * cNewSize)));
            return VERR_NO_MEMORY;
        }
        memcpy(pvNewData, pTbl->paData, sizeof (pTbl->paData[0]) * pTbl->cSize);
        RTMemFree(pTbl->paData);
        pTbl->iNext2Search = pTbl->cSize;
        pTbl->cSize = cNewSize;
        pTbl->paData = pvNewData;
        return VINF_SUCCESS;
    }
    else if (cNewSize >= pTbl->cData)
    {
        WARN(("not implemented"));
        return VERR_NOT_IMPLEMENTED;
    }
    WARN(("invalid parameter"));
    return VERR_INVALID_PARAMETER;

}

VBOXHTABLEDECL(int) CrHTableRealloc(PCRHTABLE pTbl, uint32_t cNewSize)
{
    return crHTableRealloc(pTbl, cNewSize);
}

VBOXHTABLEDECL(void) CrHTableEmpty(PCRHTABLE pTbl)
{
    pTbl->cData = 0;
    pTbl->iNext2Search = 0;
    if (pTbl->cSize)
        memset(pTbl->paData, 0, sizeof (pTbl->paData[0]) * pTbl->cSize);
}

static void* crHTablePutToSlot(PCRHTABLE pTbl, uint32_t iSlot, void* pvData)
{
    Assert(pvData);
    void* pvOld = pTbl->paData[iSlot];
    pTbl->paData[iSlot] = pvData;
    if (!pvOld)
        ++pTbl->cData;
    Assert(pTbl->cData <= pTbl->cSize);
    return pvOld;
}

VBOXHTABLEDECL(int) CrHTablePutToSlot(PCRHTABLE pTbl, CRHTABLE_HANDLE hHandle, void* pvData)
{
    AssertReturn(pvData, VERR_INVALID_PARAMETER);
    uint32_t iIndex = crHTableHandle2Index(hHandle);
    if (iIndex >= pTbl->cSize)
    {
        int rc = crHTableRealloc(pTbl, iIndex + RT_MAX(10, pTbl->cSize/4));
        if (!RT_SUCCESS(rc))
        {
            WARN(("crHTableRealloc failed rc %d", rc));
            return CRHTABLE_HANDLE_INVALID;
        }
    }

    crHTablePutToSlot(pTbl, iIndex, pvData);

    return VINF_SUCCESS;
}

VBOXHTABLEDECL(CRHTABLE_HANDLE) CrHTablePut(PCRHTABLE pTbl, void* pvData)
{
    AssertReturn(pvData, CRHTABLE_HANDLE_INVALID);

    if (pTbl->cSize == pTbl->cData)
    {
        int rc = crHTableRealloc(pTbl, pTbl->cSize + RT_MAX(10, pTbl->cSize/4));
        if (!RT_SUCCESS(rc))
        {
            WARN(("crHTableRealloc failed rc %d", rc));
            return CRHTABLE_HANDLE_INVALID;
        }
    }
    for (uint32_t i = pTbl->iNext2Search; ; ++i, i %= pTbl->cSize)
    {
        Assert(i < pTbl->cSize);
        if (!pTbl->paData[i])
        {
            void *pvOld = crHTablePutToSlot(pTbl, i, pvData);
            Assert(!pvOld); NOREF(pvOld);
            pTbl->iNext2Search = i+1;
            pTbl->iNext2Search %= pTbl->cSize;
            return crHTableIndex2Handle(i);
        }
    }
    /* not reached */
}

VBOXHTABLEDECL(void*) CrHTableRemove(PCRHTABLE pTbl, CRHTABLE_HANDLE hHandle)
{
    uint32_t iIndex = crHTableHandle2Index(hHandle);
    Assert(iIndex < pTbl->cSize);
    if (iIndex < pTbl->cSize)
    {
        void* pvData = pTbl->paData[iIndex];
        if (pvData)
        {
            pTbl->paData[iIndex] = NULL;
            --pTbl->cData;
            Assert(pTbl->cData <= pTbl->cSize);
            pTbl->iNext2Search = iIndex;
        }
        return pvData;
    }
    WARN(("invalid handle supplied %d", hHandle));
    return NULL;
}

VBOXHTABLEDECL(void*) CrHTableGet(PCRHTABLE pTbl, CRHTABLE_HANDLE hHandle)
{
    uint32_t iIndex = crHTableHandle2Index(hHandle);
    if (iIndex < pTbl->cSize)
        return pTbl->paData[iIndex];
    LOG(("invalid handle supplied %d", hHandle));
    return NULL;
}
