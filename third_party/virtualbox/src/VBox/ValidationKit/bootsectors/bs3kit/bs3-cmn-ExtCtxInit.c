/* $Id: bs3-cmn-ExtCtxInit.c $ */
/** @file
 * BS3Kit - Bs3ExtCtxInit
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
#include <iprt/asm-amd64-x86.h>


#undef Bs3ExtCtxInit
BS3_CMN_DEF(PBS3EXTCTX, Bs3ExtCtxInit,(PBS3EXTCTX pExtCtx, uint16_t cbExtCtx, uint64_t fFlags))
{
    Bs3MemSet(pExtCtx, 0, cbExtCtx);
    if (cbExtCtx >= RT_OFFSETOF(BS3EXTCTX, Ctx) + sizeof(X86FXSTATE) + sizeof(X86XSAVEHDR))
    {
        BS3_ASSERT(fFlags & XSAVE_C_X87);
        pExtCtx->enmMethod = BS3EXTCTXMETHOD_XSAVE;
        pExtCtx->Ctx.x.Hdr.bmXState = fFlags;
    }
    else if (cbExtCtx >= RT_OFFSETOF(BS3EXTCTX, Ctx) + sizeof(X86FXSTATE))
    {
        BS3_ASSERT(fFlags == 0);
        pExtCtx->enmMethod = BS3EXTCTXMETHOD_FXSAVE;
    }
    else
    {
        BS3_ASSERT(fFlags == 0);
        BS3_ASSERT(cbExtCtx >= RT_OFFSETOF(BS3EXTCTX, Ctx) + sizeof(X86FPUSTATE));
        pExtCtx->enmMethod = BS3EXTCTXMETHOD_ANCIENT;
    }
    pExtCtx->cb             = cbExtCtx;
    pExtCtx->u16Magic       = BS3EXTCTX_MAGIC;
    pExtCtx->fXcr0Nominal   = fFlags;
    pExtCtx->fXcr0Saved     = fFlags;
    return pExtCtx;
}

