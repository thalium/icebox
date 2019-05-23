/* $Id: bs3-cmn-SlabAllocEx.c $ */
/** @file
 * BS3Kit - Bs3SlabAllocEx
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
#include <iprt/asm.h>


#undef Bs3SlabAllocEx
BS3_CMN_DEF(void BS3_FAR *, Bs3SlabAllocEx,(PBS3SLABCTL pSlabCtl, uint16_t cChunks, uint16_t fFlags))
{
    BS3_ASSERT(cChunks > 0);
    if (pSlabCtl->cFreeChunks >= cChunks)
    {
        int32_t iBit = ASMBitFirstClear(&pSlabCtl->bmAllocated, pSlabCtl->cChunks);
        if (iBit >= 0)
        {
            BS3_ASSERT(!ASMBitTest(&pSlabCtl->bmAllocated, iBit));

            while ((uint32_t)iBit + cChunks <= pSlabCtl->cChunks)
            {
                /* Check that we've got the requested number of free chunks here. */
                uint16_t i;
                for (i = 1; i < cChunks; i++)
                    if (ASMBitTest(&pSlabCtl->bmAllocated, iBit + i))
                        break;
                if (i == cChunks)
                {
                    /* Check if the chunks are all in the same tiled segment. */
                    BS3_XPTR_AUTO(void, pvRet);
                    BS3_XPTR_SET_FLAT(void, pvRet,
                                      BS3_XPTR_GET_FLAT(uint8_t, pSlabCtl->pbStart) + ((uint32_t)iBit << pSlabCtl->cChunkShift));
                    if (   !(fFlags & BS3_SLAB_ALLOC_F_SAME_TILE)
                        ||    (BS3_XPTR_GET_FLAT(void, pvRet) >> 16)
                           == ((BS3_XPTR_GET_FLAT(void, pvRet) + ((uint32_t)cChunks << pSlabCtl->cChunkShift) - 1) >> 16) )
                    {
                        /* Complete the allocation. */
                        void *fpRet;
                        for (i = 0; i < cChunks; i++)
                            ASMBitSet(&pSlabCtl->bmAllocated, iBit + i);
                        pSlabCtl->cFreeChunks  -= cChunks;
                        fpRet = BS3_XPTR_GET(void, pvRet);
#if ARCH_BITS == 16
                        BS3_ASSERT(fpRet != NULL);
#endif
                        return fpRet;
                    }

                    /*
                     * We're crossing a tiled segment boundrary.
                     * Skip to the start of the next segment and retry there.
                     * (We already know that the first chunk in the next tiled
                     * segment is free, otherwise we wouldn't have a crossing.)
                     */
                    BS3_ASSERT(((uint32_t)cChunks << pSlabCtl->cChunkShift) <= _64K);
                    i = BS3_XPTR_GET_FLAT_LOW(void, pvRet);
                    i = UINT16_C(0) - i;
                    i >>= pSlabCtl->cChunkShift;
                    iBit += i;
                }
                else
                {
                    /*
                     * Continue searching.
                     */
                    iBit = ASMBitNextClear(&pSlabCtl->bmAllocated, pSlabCtl->cChunks, iBit + i);
                    if (iBit < 0)
                        break;
                }
            }
        }
    }
    return NULL;
}

