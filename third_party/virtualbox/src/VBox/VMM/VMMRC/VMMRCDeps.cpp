/* $Id: VMMRCDeps.cpp $ */
/** @file
 * VMMRC Runtime Dependencies.
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
 */

#include <iprt/crc.h>
#include <iprt/string.h>

#if defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
RT_C_DECLS_BEGIN
extern uint64_t __udivdi3(uint64_t, uint64_t);
extern uint64_t __umoddi3(uint64_t, uint64_t);
RT_C_DECLS_END
#endif // RT_OS_SOLARIS || RT_OS_FREEBSD

PFNRT g_VMMRCDeps[] =
{
    (PFNRT)memset,
    (PFNRT)memcpy,
    (PFNRT)memchr,
    (PFNRT)memcmp,
    (PFNRT)RTCrc32,
#if defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
    (PFNRT)__udivdi3,
    (PFNRT)__umoddi3,
#endif // RT_OS_SOLARIS || RT_OS_FREEBSD
    NULL
};

