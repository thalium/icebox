/* $Id: bs3-cmn-SlabListFree.c $ */
/** @file
 * BS3Kit - Bs3SlabListFree
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

#include "bs3kit-template-header.h"


#undef Bs3SlabListFree
BS3_CMN_DEF(void, Bs3SlabListFree,(PBS3SLABHEAD pHead, void BS3_FAR *pvChunks, uint16_t cChunks))
{
    BS3_ASSERT(cChunks > 0);
    if (cChunks > 0)
    {
        PBS3SLABCTL         pCur;
        BS3_XPTR_AUTO(void, pvFlatChunk);
        BS3_XPTR_SET(void,  pvFlatChunk, pvChunks);

        for (pCur = BS3_XPTR_GET(BS3SLABCTL, pHead->pFirst);
             pCur != NULL;
             pCur = BS3_XPTR_GET(BS3SLABCTL, pCur->pNext))
        {
            if (  ((BS3_XPTR_GET_FLAT(void, pvFlatChunk) - BS3_XPTR_GET_FLAT(uint8_t, pCur->pbStart)) >> pCur->cChunkShift)
                < pCur->cChunks)
            {
                pHead->cFreeChunks += Bs3SlabFree(pCur, BS3_XPTR_GET_FLAT(void, pvFlatChunk), cChunks);
                return;
            }
        }
        BS3_ASSERT(0);
    }
}

