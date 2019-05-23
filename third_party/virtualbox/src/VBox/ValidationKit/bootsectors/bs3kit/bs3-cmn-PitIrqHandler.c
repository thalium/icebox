/* $Id: bs3-cmn-PitIrqHandler.c $ */
/** @file
 * BS3Kit - The PIT IRQ Handler and associated data.
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


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#if ARCH_BITS == 16
/** Nano seconds (approx) since last the PIT timer was started. */
uint64_t volatile   g_cBs3PitNs    = 0;
/** Milliseconds seconds (very approx) since last the PIT timer was started. */
uint64_t volatile   g_cBs3PitMs    = 0;
/** Number of ticks since last the PIT timer was started.  */
uint32_t volatile   g_cBs3PitTicks = 0;
/** The current interval in nanon seconds.  */
uint32_t            g_cBs3PitIntervalNs = 0;
/** The current interval in milliseconds (approximately).
 * This is 0 if not yet started (used for checking the state internally). */
uint16_t volatile   g_cBs3PitIntervalMs = 0;
/** The current PIT frequency (approximately).  0 if not yet started.  */
uint16_t            g_cBs3PitIntervalHz = 0;
#endif


BS3_DECL_NEAR_CALLBACK(void) BS3_CMN_NM(bs3PitIrqHandler)(PBS3TRAPFRAME pTrapFrame)
{
    uint16_t cMsIntercal = g_cBs3PitIntervalMs;
    if (cMsIntercal)
    {
        g_cBs3PitMs += cMsIntercal;
        g_cBs3PitNs += g_cBs3PitIntervalNs;
        g_cBs3PitTicks++;
    }
    NOREF(pTrapFrame);
    ASMOutU8(0x20, 0x20); /** @todo function! */
}

