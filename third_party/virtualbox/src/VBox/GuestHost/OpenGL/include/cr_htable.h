/* $Id: cr_htable.h $ */

/** @file
 * uint32_t handle to void simple table API
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
#ifndef ___cr_htable_h_
#define ___cr_htable_h_

#include <iprt/types.h>
#include <iprt/cdefs.h>

#include <cr_error.h>

#ifndef IN_RING0
# define VBOXHTABLEDECL(_type) DECLEXPORT(_type)
#else
# define VBOXHTABLEDECL(_type) RTDECL(_type)
#endif



RT_C_DECLS_BEGIN

typedef uint32_t CRHTABLE_HANDLE;
#define CRHTABLE_HANDLE_INVALID 0UL

typedef struct CRHTABLE
{
    uint32_t cData;
    uint32_t iNext2Search;
    uint32_t cSize;
    void **paData;
} CRHTABLE, *PCRHTABLE;

typedef struct CRHTABLE_ITERATOR
{
    PCRHTABLE pTbl;
    uint32_t iCur;
    uint32_t cLeft;
} VCRHTABLE_ITERATOR, *PCRHTABLE_ITERATOR;

/*private stuff, not to be used directly */
DECLINLINE(CRHTABLE_HANDLE) crHTableIndex2Handle(uint32_t iIndex)
{
    return iIndex+1;
}

DECLINLINE(uint32_t) crHTableHandle2Index(CRHTABLE_HANDLE hHandle)
{
    return hHandle-1;
}

/* public API */
DECLINLINE(void) CrHTableIterInit(PCRHTABLE pTbl, PCRHTABLE_ITERATOR pIter)
{
    pIter->pTbl = pTbl;
    pIter->iCur = 0;
    pIter->cLeft = pTbl->cData;
}

DECLINLINE(void*) CrHTableIterNext(PCRHTABLE_ITERATOR pIter, CRHTABLE_HANDLE *phHandle)
{
    PCRHTABLE pTbl;
    uint32_t i;
    if (!pIter->cLeft)
    {
        if (phHandle)
            *phHandle = 0;
        return NULL;
    }

    pTbl = pIter->pTbl;

    for (i = pIter->iCur; i < pTbl->cSize; ++i)
    {
        if (pTbl->paData[i])
        {
            pIter->iCur = i+1;
            --(pIter->cLeft);
            if (phHandle)
                *phHandle = crHTableIndex2Handle(i);
            return pTbl->paData[i];
        }
    }

    crWarning("interator concurent modification!");
    return NULL;
}

VBOXHTABLEDECL(int) CrHTableCreate(PCRHTABLE pTbl, uint32_t cSize);
DECLINLINE(void) CrHTableMoveTo(PCRHTABLE pSrcTbl, PCRHTABLE pDstTbl)
{
    *pDstTbl = *pSrcTbl;
    CrHTableCreate(pSrcTbl, 0);
}
VBOXHTABLEDECL(void) CrHTableEmpty(PCRHTABLE pTbl);
VBOXHTABLEDECL(void) CrHTableDestroy(PCRHTABLE pTbl);
VBOXHTABLEDECL(int) CrHTableRealloc(PCRHTABLE pTbl, uint32_t cNewSize);
VBOXHTABLEDECL(CRHTABLE_HANDLE) CrHTablePut(PCRHTABLE pTbl, void *pvData);
VBOXHTABLEDECL(int) CrHTablePutToSlot(PCRHTABLE pTbl, CRHTABLE_HANDLE hHandle, void* pvData);

/* note: can be called for the element returned with CrHTableIterNext w/o corrupting the iterator */
VBOXHTABLEDECL(void*) CrHTableRemove(PCRHTABLE pTbl, CRHTABLE_HANDLE hHandle);
VBOXHTABLEDECL(void*) CrHTableGet(PCRHTABLE pTbl, CRHTABLE_HANDLE hHandle);

RT_C_DECLS_END

#endif /* #ifndef ___cr_htable_h_*/
