/* $Id: bs3-cmn-MemFree.c $ */
/** @file
 * BS3Kit - Bs3MemFree
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


#undef Bs3MemFree
BS3_CMN_DEF(void, Bs3MemFree,(void BS3_FAR *pv, size_t cb))
{
    if (pv != NULL)
    {
        uint16_t    cChunks;
        PBS3SLABCTL pCtl;
        BS3_XPTR_AUTO(void, pvFlat);
        BS3_XPTR_SET(void, pvFlat, pv);

        if (BS3_XPTR_GET_FLAT(void, pvFlat) & 0xfffU)
        {
            /* Use an XPTR here in case we're in real mode and the caller has
               messed around with the pointer. */
            BS3_XPTR_AUTO(BS3SLABCTL, pTmp);
            BS3_XPTR_SET_FLAT(BS3SLABCTL, pTmp, BS3_XPTR_GET_FLAT(void, pvFlat) & ~(uint32_t)0xfff);
            pCtl = BS3_XPTR_GET(BS3SLABCTL, pTmp);
            BS3_ASSERT(pCtl->cbChunk >= cb);
            cChunks = 1;
        }
        else
        {
            pCtl = BS3_XPTR_GET_FLAT(void, pvFlat) < _1M ? &g_Bs3Mem4KLow.Core : &g_Bs3Mem4KUpperTiled.Core;
            cChunks = RT_ALIGN_Z(cb, _4K) >> 12;
        }
        Bs3SlabFree(pCtl, BS3_XPTR_GET_FLAT(void, pvFlat), cChunks);
    }
}

