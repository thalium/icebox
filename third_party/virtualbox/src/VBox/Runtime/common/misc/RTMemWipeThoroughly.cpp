/* $Id: RTMemWipeThoroughly.cpp $ */
/** @file
 * IPRT - RTMemWipeThoroughly.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
#include <iprt/mem.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#include <iprt/rand.h>
#include <iprt/string.h>


RTDECL(void) RTMemWipeThoroughly(void *pv, size_t cb, size_t cMinPasses) RT_NO_THROW_DEF
{
    size_t cPasses = RT_MIN(cMinPasses, 6);

    do
    {
        memset(pv, 0xff, cb);
        ASMMemoryFence();

        memset(pv, 0x00, cb);
        ASMMemoryFence();

        RTRandBytes(pv, cb);
        ASMMemoryFence();
    } while (cPasses-- > 0);
}

