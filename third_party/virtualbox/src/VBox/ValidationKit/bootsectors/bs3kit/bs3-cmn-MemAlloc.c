/* $Id: bs3-cmn-MemAlloc.c $ */
/** @file
 * BS3Kit - Bs3MemAlloc
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "bs3kit-template-header.h"
#include "bs3-cmn-memory.h"
#include <iprt/asm.h>


#undef Bs3MemAlloc
BS3_CMN_DEF(void BS3_FAR *, Bs3MemAlloc,(BS3MEMKIND enmKind, size_t cb))
{
    void BS3_FAR   *pvRet;
    uint8_t         idxSlabList;

#if ARCH_BITS == 16
    /* Don't try allocate memory which address we cannot return. */
    if (   enmKind != BS3MEMKIND_REAL
        && BS3_MODE_IS_RM_OR_V86(g_bBs3CurrentMode))
        enmKind = BS3MEMKIND_REAL;
#endif

    idxSlabList = bs3MemSizeToSlabListIndex(cb);
    if (idxSlabList < BS3_MEM_SLAB_LIST_COUNT)
    {
        /*
         * Try allocate a chunk from the list.
         */
        PBS3SLABHEAD pHead = enmKind == BS3MEMKIND_REAL
                           ? &g_aBs3LowSlabLists[idxSlabList]
                           : &g_aBs3UpperTiledSlabLists[idxSlabList];

        BS3_ASSERT(g_aBs3LowSlabLists[idxSlabList].cbChunk >= cb);
        pvRet = Bs3SlabListAlloc(pHead);
        if (pvRet)
        { /* likely */ }
        else
        {
            /*
             * Grow the list.
             */
            PBS3SLABCTL pNew = (PBS3SLABCTL)Bs3SlabAlloc(  enmKind == BS3MEMKIND_REAL
                                                         ? &g_Bs3Mem4KLow.Core
                                                         : &g_Bs3Mem4KUpperTiled.Core);
            BS3_ASSERT(((uintptr_t)pNew & 0xfff) == 0);
            if (pNew)
            {
                uint16_t const      cbHdr = g_cbBs3SlabCtlSizesforLists[idxSlabList];
                BS3_XPTR_AUTO(void, pvNew);
                BS3_XPTR_SET(void, pvNew, pNew);

                Bs3SlabInit(pNew, cbHdr, BS3_XPTR_GET_FLAT(void, pvNew) + cbHdr, _4K - cbHdr, pHead->cbChunk);
                Bs3SlabListAdd(pHead, pNew);

                pvRet = Bs3SlabListAlloc(pHead);
            }
        }
    }
    else
    {
        /*
         * Allocate one or more pages.
         */
        size_t   const cbAligned = RT_ALIGN_Z(cb, _4K);
        uint16_t const cPages    = cbAligned >> 12 /* div _4K */;
        PBS3SLABCTL    pSlabCtl  = enmKind == BS3MEMKIND_REAL
                                 ? &g_Bs3Mem4KLow.Core : &g_Bs3Mem4KUpperTiled.Core;

        pvRet = Bs3SlabAllocEx(pSlabCtl,
                               cPages,
                               cPages <= _64K / _4K ? BS3_SLAB_ALLOC_F_SAME_TILE : 0);
    }
    return pvRet;
}

