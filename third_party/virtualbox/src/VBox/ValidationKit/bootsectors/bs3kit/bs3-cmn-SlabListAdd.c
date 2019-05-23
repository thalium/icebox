/* $Id: bs3-cmn-SlabListAdd.c $ */
/** @file
 * BS3Kit - Bs3SlabListAdd
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


#undef Bs3SlabListAdd
BS3_CMN_DEF(void, Bs3SlabListAdd,(PBS3SLABHEAD pHead, PBS3SLABCTL pSlabCtl))
{
    BS3_ASSERT(pHead->cbChunk == pSlabCtl->cbChunk);
    BS3_ASSERT(BS3_XPTR_IS_NULL(BS3SLABHEAD, pSlabCtl->pNext));

    BS3_XPTR_SET_FLAT(BS3SLABCTL, pSlabCtl->pNext, BS3_XPTR_GET_FLAT(BS3SLABCTL, pHead->pFirst));
    BS3_XPTR_SET(BS3SLABCTL, pHead->pFirst, pSlabCtl);

    pHead->cSlabs      += 1;
    pHead->cChunks     += pSlabCtl->cChunks;
    pHead->cFreeChunks += pSlabCtl->cFreeChunks;
}

