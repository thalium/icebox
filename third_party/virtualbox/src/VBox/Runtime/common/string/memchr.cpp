/* $Id: memchr.cpp $ */
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
 * Search a memory block for a character.
 *
 * @returns Pointer to the first instance of ch in pv.
 * @returns NULL if ch wasn't found.
 * @param   pv          Pointer to the block to search.
 * @param   ch          The character to search for.
 * @param   cb          The size of the block.
 */
#ifdef _MSC_VER /* Silly 'safeness' from MS. */
# if _MSC_VER >= 1400
_CRTIMP __checkReturn _CONST_RETURN void *  __cdecl memchr( __in_bcount_opt(_MaxCount) const void * pv, __in int ch, __in size_t cb)
# else
void *memchr(const void *pv, int ch, size_t cb)
# endif
#else
void *memchr(const void *pv, int ch, size_t cb)
#endif
{
    register uint8_t const *pu8 = (uint8_t const *)pv;
    register size_t cb2 = cb;
    while (cb2-- > 0)
    {
        if (*pu8 == ch)
            return (void *)pu8;
        pu8++;
    }
    return NULL;
}

