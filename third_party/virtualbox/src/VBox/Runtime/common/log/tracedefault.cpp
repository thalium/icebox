/* $Id: tracedefault.cpp $ */
/** @file
 * IPRT - Tracebuffer common functions.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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
#include "internal/iprt.h"
#include <iprt/trace.h>

#include <iprt/asm.h>
#include <iprt/err.h>
#include <iprt/thread.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** The default trace buffer handle. */
static RTTRACEBUF   g_hDefaultTraceBuf = NIL_RTTRACEBUF;



RTDECL(int)         RTTraceSetDefaultBuf(RTTRACEBUF hTraceBuf)
{
    /* Retain the new buffer. */
    if (hTraceBuf != NIL_RTTRACEBUF)
    {
        uint32_t cRefs = RTTraceBufRetain(hTraceBuf);
        if (cRefs >= _1M)
            return VERR_INVALID_HANDLE;
    }

    RTTRACEBUF hOldTraceBuf;
#ifdef IN_RC
    hOldTraceBuf = (RTTRACEBUF)ASMAtomicXchgPtr((void **)&g_hDefaultTraceBuf, hTraceBuf);
#else
    ASMAtomicXchgHandle(&g_hDefaultTraceBuf, hTraceBuf, &hOldTraceBuf);
#endif

    if (    hOldTraceBuf != NIL_RTTRACEBUF
        &&  hOldTraceBuf != hTraceBuf)
    {
        /* Race prevention kludge. */
#ifndef IN_RC
        RTThreadSleep(33);
#endif
        RTTraceBufRelease(hOldTraceBuf);
    }

    return VINF_SUCCESS;
}


RTDECL(RTTRACEBUF)  RTTraceGetDefaultBuf(void)
{
    return g_hDefaultTraceBuf;
}

