/* $Id: RTLogWriteDebugger-r0drv-solaris.c $ */
/** @file
 * IPRT - Log To Debugger, Ring-0 Driver, Solaris.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
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
#include "the-solaris-kernel.h"
#include "internal/iprt.h"
#include <iprt/log.h>

#include <iprt/asm.h>
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
# include <iprt/asm-amd64-x86.h>
#endif
#include <iprt/assert.h>
#include <iprt/thread.h>



RTDECL(void) RTLogWriteDebugger(const char *pch, size_t cb)
{
    if (pch[cb] != '\0')
        AssertBreakpoint();

    /* cmn_err() acquires adaptive mutexes. Not preemption safe, see @bugref{6657}. */
    if (!RTThreadPreemptIsEnabled(NIL_RTTHREAD))
        return;

    if (    !g_frtSolSplSetsEIF
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
        ||  ASMIntAreEnabled()
#else
/* PORTME: Check if interrupts are enabled, if applicable. */
#endif
        )
    {
        cmn_err(CE_CONT, pch);
    }

    return;
}

