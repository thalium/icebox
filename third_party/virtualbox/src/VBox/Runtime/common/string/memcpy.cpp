/* $Id: memcpy.cpp $ */
/** @file
 * IPRT - CRT Strings, memcpy().
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
 * Copies a memory.
 *
 * @returns pvDst.
 * @param   pvDst       Pointer to the target block.
 * @param   pvSrc       Pointer to the source block.
 * @param   cb          The size of the block.
 */
#ifdef _MSC_VER
# if _MSC_VER >= 1400
_CRT_INSECURE_DEPRECATE_MEMORY(memcpy_s) void *  __cdecl memcpy(__out_bcount_full_opt(_Size) void * pvDst, __in_bcount_opt(_Size) const void * pvSrc, __in size_t cb)
# else
void *memcpy(void *pvDst, const void *pvSrc, size_t cb)
# endif
#else
void *memcpy(void *pvDst, const void *pvSrc, size_t cb)
#endif
{
    register union
    {
        uint8_t  *pu8;
        uint32_t *pu32;
        void     *pv;
    } uTrg;
    uTrg.pv = pvDst;

    register union
    {
        uint8_t const  *pu8;
        uint32_t const *pu32;
        void const     *pv;
    } uSrc;
    uSrc.pv = pvSrc;

    /* 32-bit word moves. */
    register size_t c = cb >> 2;
    while (c-- > 0)
        *uTrg.pu32++ = *uSrc.pu32++;

    /* Remaining byte moves. */
    c = cb & 3;
    while (c-- > 0)
        *uTrg.pu8++ = *uSrc.pu8++;

    return pvDst;
}

