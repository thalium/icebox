/* $Id: bs3-fpustate-1.c $ */
/** @file
 * BS3Kit - bs3-fpustate-1, 16-bit C code.
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
#include <iprt/asm-amd64-x86.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
BS3TESTMODE_PROTOTYPES_MODE(bs3FpuState1_Corruption);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const BS3TESTMODEENTRY g_aModeTest[] =
{
    {
        /*pszSubTest =*/ "corruption",
        /*RM*/        bs3FpuState1_Corruption_rm,
        /*PE16*/      NULL, //bs3FpuState1_Corruption_pe16,
        /*PE16_32*/   NULL, //bs3FpuState1_Corruption_pe16_32,
        /*PE16_V86*/  NULL, //bs3FpuState1_Corruption_pe16_v86,
        /*PE32*/      bs3FpuState1_Corruption_pe32,
        /*PE32_16*/   NULL, //bs3FpuState1_Corruption_pe32_16,
        /*PEV86*/     NULL, //bs3FpuState1_Corruption_pev86,
        /*PP16*/      NULL, //bs3FpuState1_Corruption_pp16,
        /*PP16_32*/   NULL, //bs3FpuState1_Corruption_pp16_32,
        /*PP16_V86*/  NULL, //bs3FpuState1_Corruption_pp16_v86,
        /*PP32*/      bs3FpuState1_Corruption_pp32,
        /*PP32_16*/   NULL, //bs3FpuState1_Corruption_pp32_16,
        /*PPV86*/     NULL, //bs3FpuState1_Corruption_ppv86,
        /*PAE16*/     NULL, //bs3FpuState1_Corruption_pae16,
        /*PAE16_32*/  NULL, //bs3FpuState1_Corruption_pae16_32,
        /*PAE16_V86*/ NULL, //bs3FpuState1_Corruption_pae16_v86,
        /*PAE32*/     bs3FpuState1_Corruption_pae32,
        /*PAE32_16*/  NULL, //bs3FpuState1_Corruption_pae32_16,
        /*PAEV86*/    NULL, //bs3FpuState1_Corruption_paev86,
        /*LM16*/      NULL, //bs3FpuState1_Corruption_lm16,
        /*LM32*/      NULL, //bs3FpuState1_Corruption_lm32,
        /*LM64*/      bs3FpuState1_Corruption_lm64,
    }
};


BS3_DECL(void) Main_rm()
{
    Bs3InitAll_rm();
    Bs3TestInit("bs3-fpustate-1");
    Bs3TestPrintf("g_uBs3CpuDetected=%#x\n", g_uBs3CpuDetected);

    Bs3TestDoModes_rm(g_aModeTest, RT_ELEMENTS(g_aModeTest));

    Bs3TestTerm();
}

