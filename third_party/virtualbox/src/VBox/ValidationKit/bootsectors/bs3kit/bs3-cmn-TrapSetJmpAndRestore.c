/* $Id: bs3-cmn-TrapSetJmpAndRestore.c $ */
/** @file
 * BS3Kit - Bs3TrapSetJmpAndRestore
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


#undef Bs3TrapSetJmpAndRestore
BS3_CMN_DEF(void, Bs3TrapSetJmpAndRestore,(PCBS3REGCTX pCtxRestore, PBS3TRAPFRAME pTrapFrame))
{
    if (Bs3TrapSetJmp(pTrapFrame))
    {
#if TMPL_BITS == 32
        g_uBs3TrapEipHint = pCtxRestore->rip.u32;
#endif
        Bs3RegCtxRestore(pCtxRestore, 0);
    }
}

