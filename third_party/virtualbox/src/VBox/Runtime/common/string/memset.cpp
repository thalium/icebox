/* $Id: memset.cpp $ */
/** @file
 * IPRT - CRT Strings, memset().
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
#include <iprt/string.h>


/**
 * Fill a memory block with specific byte.
 *
 * @returns pvDst.
 * @param   pvDst      Pointer to the block.
 * @param   ch      The filler char.
 * @param   cb      The size of the block.
 */
#ifdef _MSC_VER
# if _MSC_VER >= 1400
void *  __cdecl memset(__out_bcount_full_opt(_Size) void *pvDst, __in int ch, __in size_t cb)
# else
void *memset(void *pvDst, int ch, size_t cb)
# endif
#else
void *memset(void *pvDst, int ch, size_t cb)
#endif
{
    register union
    {
        uint8_t  *pu8;
        uint32_t *pu32;
        void     *pvDst;
    } u;
    u.pvDst = pvDst;

    /* 32-bit word moves. */
    register uint32_t u32 = ch | (ch << 8);
    u32 |= u32 << 16;
    register size_t c = cb >> 2;
    while (c-- > 0)
        *u.pu32++ = u32;

    /* Remaining byte moves. */
    c = cb & 3;
    while (c-- > 0)
        *u.pu8++ = (uint8_t)u32;

    return pvDst;
}

