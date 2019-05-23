/* $Id: USBLib.cpp $ */
/** @file
 * VirtualBox USB Library, Common Bits.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/usblib.h>


/**
 * Calculate the hash of the serial string.
 *
 * 64bit FNV1a, chosen because it is designed to hash in to a power of two
 * space, and is much quicker and simpler than, say, a half MD4.
 *
 * @returns the hash.
 * @param   pszSerial       The serial string.
 */
USBLIB_DECL(uint64_t) USBLibHashSerial(const char *pszSerial)
{
    if (!pszSerial)
        pszSerial = "";

    register const uint8_t *pu8 = (const uint8_t *)pszSerial;
    register uint64_t u64 = UINT64_C(14695981039346656037);
    for (;;)
    {
        register uint8_t u8 = *pu8;
        if (!u8)
            break;
        u64 = (u64 * UINT64_C(1099511628211)) ^ u8;
        pu8++;
    }

    return u64;
}

