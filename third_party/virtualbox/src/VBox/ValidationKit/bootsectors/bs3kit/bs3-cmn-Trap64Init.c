/* $Id: bs3-cmn-Trap64Init.c $ */
/** @file
 * BS3Kit - Bs3Trap64Init
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


#undef Bs3Trap64Init
BS3_CMN_DEF(void, Bs3Trap64Init,(void))
{
     X86TSS64 BS3_FAR *pTss;
     unsigned iIdt;

    /*
     * IDT entries, except the system call gate.
     * The #DF entry get IST=1, all others IST=0.
     */
    for (iIdt = 0; iIdt < BS3_TRAP_SYSCALL; iIdt++)
        Bs3Trap64SetGate(iIdt, AMD64_SEL_TYPE_SYS_INT_GATE, 0 /*bDpl*/,
                         BS3_SEL_R0_CS64, g_Bs3Trap64GenericEntriesFlatAddr + iIdt * 8, iIdt == X86_XCPT_DF /*bIst*/);
    for (iIdt = BS3_TRAP_SYSCALL + 1; iIdt < 256; iIdt++)
        Bs3Trap64SetGate(iIdt, AMD64_SEL_TYPE_SYS_INT_GATE, 0 /*bDpl*/,
                         BS3_SEL_R0_CS64, g_Bs3Trap64GenericEntriesFlatAddr + iIdt * 8, 0 /*bIst*/);

    /*
     * Initialize the normal TSS so we can do ring transitions via the IDT.
     */
    pTss = &Bs3Tss64;
    Bs3MemZero(pTss, sizeof(*pTss));
    pTss->rsp0      = BS3_ADDR_STACK_R0;
    pTss->rsp1      = BS3_ADDR_STACK_R1;
    pTss->rsp2      = BS3_ADDR_STACK_R2;
    pTss->ist1      = BS3_ADDR_STACK_R0_IST1;
    pTss->ist2      = BS3_ADDR_STACK_R0_IST2;
    pTss->ist3      = BS3_ADDR_STACK_R0_IST3;
    pTss->ist4      = BS3_ADDR_STACK_R0_IST4;
    pTss->ist5      = BS3_ADDR_STACK_R0_IST5;
    pTss->ist6      = BS3_ADDR_STACK_R0_IST6;
    pTss->ist7      = BS3_ADDR_STACK_R0_IST7;
}

