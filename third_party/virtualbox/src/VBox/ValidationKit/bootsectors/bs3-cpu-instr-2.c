/* $Id: bs3-cpu-instr-2.c $ */
/** @file
 * BS3Kit - bs3-cpu-instr-2, 16-bit C code.
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
#include <bs3kit.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_mul);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_imul);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_div);
BS3TESTMODE_PROTOTYPES_CMN(bs3CpuInstr2_idiv);
BS3TESTMODE_PROTOTYPES_CMN_64(bs3CpuInstr2_cmpxchg16b);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const BS3TESTMODEENTRY g_aModeTests[] =
{
    BS3TESTMODEENTRY_CMN("mul", bs3CpuInstr2_mul),
    BS3TESTMODEENTRY_CMN("imul", bs3CpuInstr2_imul),
    BS3TESTMODEENTRY_CMN("div", bs3CpuInstr2_div),
    BS3TESTMODEENTRY_CMN("idiv", bs3CpuInstr2_idiv),
    BS3TESTMODEENTRY_CMN_64("cmpxchg16b", bs3CpuInstr2_cmpxchg16b),
};


BS3_DECL(void) Main_rm()
{
    Bs3InitAll_rm();
    Bs3TestInit("bs3-cpu-instr-2");

    Bs3TestDoModes_rm(g_aModeTests, RT_ELEMENTS(g_aModeTests));

    Bs3TestTerm();
}

