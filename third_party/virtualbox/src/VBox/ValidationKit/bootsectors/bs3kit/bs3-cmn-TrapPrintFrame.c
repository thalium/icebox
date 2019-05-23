/* $Id: bs3-cmn-TrapPrintFrame.c $ */
/** @file
 * BS3Kit - Bs3TrapPrintFrame
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


#undef Bs3TrapPrintFrame
BS3_CMN_DEF(void, Bs3TrapPrintFrame,(PCBS3TRAPFRAME pTrapFrame))
{
#if 1
    Bs3TestPrintf("Trap %#04x errcd=%#06RX64 at %04x:%016RX64 (by %04x/%04x) - test step %d (%#x)\n",
                  pTrapFrame->bXcpt,
                  pTrapFrame->uErrCd,
                  pTrapFrame->Ctx.cs,
                  pTrapFrame->Ctx.rip.u64,
                  pTrapFrame->uHandlerCs,
                  pTrapFrame->uHandlerSs,
                  g_usBs3TestStep, g_usBs3TestStep);
    Bs3RegCtxPrint(&pTrapFrame->Ctx);
#else
    /* This is useful if having trouble returning from real mode. */
    PCBS3REGCTX pRegCtx = &pTrapFrame->Ctx;
    Bs3TestPrintf("Trap %#04x errcd=%#06RX64 at %04x:%016RX64 - test step %d (%#x)\n"
                  "eax=%08RX32 ebx=%08RX32 ecx=%08RX32 edx=%08RX32 esi=%08RX32 edi=%08RX32\n"
                  "eip=%08RX32 esp=%08RX32 ebp=%08RX32 efl=%08RX32 cr0=%08RX32 cr2=%08RX32\n"
                  "cs=%04RX16   ds=%04RX16 es=%04RX16 fs=%04RX16 gs=%04RX16   ss=%04RX16 cr3=%08RX32 cr4=%08RX32\n"
                  "tr=%04RX16 ldtr=%04RX16 cpl=%d   mode=%#x fbFlags=%#x\n"
                  ,
                  pTrapFrame->bXcpt,
                  pTrapFrame->uErrCd,
                  pTrapFrame->Ctx.cs,
                  pTrapFrame->Ctx.rip.u64,
                  g_usBs3TestStep, g_usBs3TestStep
                  ,
                  pRegCtx->rax.u32, pRegCtx->rbx.u32, pRegCtx->rcx.u32, pRegCtx->rdx.u32, pRegCtx->rsi.u32, pRegCtx->rdi.u32
                  ,
                  pRegCtx->rip.u32, pRegCtx->rsp.u32, pRegCtx->rbp.u32, pRegCtx->rflags.u32,
                  pRegCtx->cr0.u32, pRegCtx->cr2.u32
                  ,
                  pRegCtx->cs, pRegCtx->ds, pRegCtx->es, pRegCtx->fs, pRegCtx->gs, pRegCtx->ss,
                  pRegCtx->cr3.u32, pRegCtx->cr4.u32
                  ,
                  pRegCtx->tr, pRegCtx->ldtr, pRegCtx->bCpl, pRegCtx->bMode, pRegCtx->fbFlags);

#endif
}

