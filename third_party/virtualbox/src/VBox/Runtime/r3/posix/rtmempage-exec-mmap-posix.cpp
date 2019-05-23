/* $Id: rtmempage-exec-mmap-posix.cpp $ */
/** @file
 * IPRT - RTMemPage*, POSIX with mmap only.
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
#include "internal/iprt.h"
#include <iprt/mem.h>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/param.h>
#include <iprt/string.h>

#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
# define MAP_ANONYMOUS MAP_ANON
#endif


/**
 * Allocates memory from the specified heap.
 *
 * @returns Address of the allocated memory.
 * @param   cb                  The number of bytes to allocate.
 * @param   pszTag              The tag.
 * @param   fZero               Whether to zero the memory or not.
 * @param   fProtExec           PROT_EXEC or 0.
 */
static void *rtMemPagePosixAlloc(size_t cb, const char *pszTag, bool fZero, int fProtExec)
{
    /*
     * Validate & adjust the input.
     */
    Assert(cb > 0);
    NOREF(pszTag);
    cb = RT_ALIGN_Z(cb, PAGE_SIZE);

    /*
     * Do the allocation.
     */
    void *pv = mmap(NULL, cb,
                    PROT_READ | PROT_WRITE | fProtExec,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (pv != MAP_FAILED)
    {
        AssertPtr(pv);
        if (fZero)
            RT_BZERO(pv, cb);
    }
    else
        pv = NULL;

    return pv;
}


/**
 * Free memory allocated by rtMemPagePosixAlloc.
 *
 * @param   pv                  The address of the memory to free.
 * @param   cb                  The size.
 */
static void rtMemPagePosixFree(void *pv, size_t cb)
{
    /*
     * Validate & adjust the input.
     */
    if (!pv)
        return;
    AssertPtr(pv);
    Assert(cb > 0);
    Assert(!((uintptr_t)pv & PAGE_OFFSET_MASK));
    cb = RT_ALIGN_Z(cb, PAGE_SIZE);

    /*
     * Free the memory.
     */
    int rc = munmap(pv, cb);
    AssertMsg(rc == 0, ("rc=%d pv=%p cb=%#zx\n", rc, pv, cb)); NOREF(rc);
}





RTDECL(void *) RTMemPageAllocTag(size_t cb, const char *pszTag) RT_NO_THROW_DEF
{
    return rtMemPagePosixAlloc(cb, pszTag, false /*fZero*/, 0);
}


RTDECL(void *) RTMemPageAllocZTag(size_t cb, const char *pszTag) RT_NO_THROW_DEF
{
    return rtMemPagePosixAlloc(cb, pszTag, true /*fZero*/, 0);
}


RTDECL(void) RTMemPageFree(void *pv, size_t cb) RT_NO_THROW_DEF
{
    return rtMemPagePosixFree(pv, cb);
}





RTDECL(void *) RTMemExecAllocTag(size_t cb, const char *pszTag) RT_NO_THROW_DEF
{
    return rtMemPagePosixAlloc(cb, pszTag, false /*fZero*/, PROT_EXEC);
}


RTDECL(void) RTMemExecFree(void *pv, size_t cb) RT_NO_THROW_DEF
{
    return rtMemPagePosixFree(pv, cb);
}

