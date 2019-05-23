/* $Id: crc32-zlib.cpp $ */
/** @file
 * IPRT - CRC-32 on top of zlib (very fast).
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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
#include <iprt/crc.h>

#include <zlib.h>

/** @todo Check if we can't just use the zlib code directly here. */

/**
 * Deal with blocks that are too big for the uInt type.
 */
static uint32_t rtCrc32ProcessTooBig(uint32_t uCRC32, const void *pv, size_t cb)
{
    const Bytef *pb = (const Bytef *)pv;
    do
    {
        uInt const cbChunk = cb <= ~(uInt)0 ? (uInt)cb : ~(uInt)0;
        uCRC32 = crc32(uCRC32, pb, cbChunk);
        pb += cbChunk;
        cb -= cbChunk;
    } while (!cb);
    return uCRC32;
}

RTDECL(uint32_t) RTCrc32(const void *pv, register size_t cb)
{
    uint32_t uCrc = crc32(0, NULL, 0);
    if (RT_UNLIKELY((uInt)cb == cb))
        uCrc = crc32(uCrc, (const Bytef *)pv, (uInt)cb);
    else
        uCrc = rtCrc32ProcessTooBig(uCrc, pv, cb);
    return uCrc;
}
RT_EXPORT_SYMBOL(RTCrc32);


RTDECL(uint32_t) RTCrc32Start(void)
{
    return crc32(0, NULL, 0);
}
RT_EXPORT_SYMBOL(RTCrc32Start);


RTDECL(uint32_t) RTCrc32Process(uint32_t uCRC32, const void *pv, size_t cb)
{
    if (RT_UNLIKELY((uInt)cb == cb))
        uCRC32 = crc32(uCRC32, (const Bytef *)pv, (uInt)cb);
    else
        uCRC32 = rtCrc32ProcessTooBig(uCRC32, pv, cb);
    return uCRC32;
}
RT_EXPORT_SYMBOL(RTCrc32Process);


RTDECL(uint32_t) RTCrc32Finish(uint32_t uCRC32)
{
    return uCRC32;
}
RT_EXPORT_SYMBOL(RTCrc32Finish);

