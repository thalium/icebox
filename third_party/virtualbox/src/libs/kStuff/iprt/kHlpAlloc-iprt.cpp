/* $Id: kHlpAlloc-iprt.cpp $ */
/** @file
 * kHlpAlloc - Memory Allocation, IPRT based implementation.
 */

/*
 * Copyright (C) 2007-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kHlpAlloc.h>
#include <iprt/mem.h>
#include <iprt/string.h>


KHLP_DECL(void *) kHlpAlloc(KSIZE cb)
{
    return RTMemAlloc(cb);
}


KHLP_DECL(void *) kHlpAllocZ(KSIZE cb)
{
    return RTMemAllocZ(cb);
}


KHLP_DECL(void *) kHlpDup(const void *pv, KSIZE cb)
{
    return RTMemDup(pv, cb);
}


KHLP_DECL(char *) kHlpStrDup(const char *psz)
{
    return RTStrDup(psz);
}


KHLP_DECL(void *) kHlpRealloc(void *pv, KSIZE cb)
{
    return RTMemRealloc(pv, cb);
}


KHLP_DECL(void) kHlpFree(void *pv)
{
    RTMemFree(pv);
}


