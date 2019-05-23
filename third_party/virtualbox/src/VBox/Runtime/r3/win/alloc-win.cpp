/* $Id: alloc-win.cpp $ */
/** @file
 * IPRT - Memory Allocation, Windows.
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
/*#define USE_VIRTUAL_ALLOC*/
#define LOG_GROUP RTLOGGROUP_MEM
#include <iprt/win/windows.h>

#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/param.h>
#include <iprt/string.h>
#include <iprt/err.h>

#ifndef USE_VIRTUAL_ALLOC
# include <malloc.h>
#endif


RTDECL(void *) RTMemExecAllocTag(size_t cb, const char *pszTag) RT_NO_THROW_DEF
{
    RT_NOREF_PV(pszTag);

    /*
     * Allocate first.
     */
    AssertMsg(cb, ("Allocating ZERO bytes is really not a good idea! Good luck with the next assertion!\n"));
    cb = RT_ALIGN_Z(cb, 32);
    void *pv = malloc(cb);
    AssertMsg(pv, ("malloc(%d) failed!!!\n", cb));
    if (pv)
    {
        memset(pv, 0xcc, cb);
        void   *pvProt = (void *)((uintptr_t)pv & ~(uintptr_t)PAGE_OFFSET_MASK);
        size_t  cbProt = ((uintptr_t)pv & PAGE_OFFSET_MASK) + cb;
        cbProt = RT_ALIGN_Z(cbProt, PAGE_SIZE);
        DWORD fFlags = 0;
        if (!VirtualProtect(pvProt, cbProt, PAGE_EXECUTE_READWRITE, &fFlags))
        {
            AssertMsgFailed(("VirtualProtect(%p, %#x,,) -> lasterr=%d\n", pvProt, cbProt, GetLastError()));
            free(pv);
            pv = NULL;
        }
    }
    return pv;
}


RTDECL(void)    RTMemExecFree(void *pv, size_t cb) RT_NO_THROW_DEF
{
    RT_NOREF_PV(cb);

    if (pv)
        free(pv);
}


RTDECL(void *) RTMemPageAllocTag(size_t cb, const char *pszTag) RT_NO_THROW_DEF
{
    RT_NOREF_PV(pszTag);

#ifdef USE_VIRTUAL_ALLOC
    void *pv = VirtualAlloc(NULL, RT_ALIGN_Z(cb, PAGE_SIZE), MEM_COMMIT, PAGE_READWRITE);
#else
    void *pv = _aligned_malloc(RT_ALIGN_Z(cb, PAGE_SIZE), PAGE_SIZE);
#endif
    AssertMsg(pv, ("cb=%d lasterr=%d\n", cb, GetLastError()));
    return pv;
}


RTDECL(void *) RTMemPageAllocZTag(size_t cb, const char *pszTag) RT_NO_THROW_DEF
{
    RT_NOREF_PV(pszTag);

#ifdef USE_VIRTUAL_ALLOC
    void *pv = VirtualAlloc(NULL, RT_ALIGN_Z(cb, PAGE_SIZE), MEM_COMMIT, PAGE_READWRITE);
#else
    void *pv = _aligned_malloc(RT_ALIGN_Z(cb, PAGE_SIZE), PAGE_SIZE);
#endif
    if (pv)
    {
        memset(pv, 0, RT_ALIGN_Z(cb, PAGE_SIZE));
        return pv;
    }
    AssertMsgFailed(("cb=%d lasterr=%d\n", cb, GetLastError()));
    return NULL;
}


RTDECL(void) RTMemPageFree(void *pv, size_t cb) RT_NO_THROW_DEF
{
    RT_NOREF_PV(cb);

    if (pv)
    {
#ifdef USE_VIRTUAL_ALLOC
        if (!VirtualFree(pv, 0, MEM_RELEASE))
            AssertMsgFailed(("pv=%p lasterr=%d\n", pv, GetLastError()));
#else
        _aligned_free(pv);
#endif
    }
}


RTDECL(int) RTMemProtect(void *pv, size_t cb, unsigned fProtect) RT_NO_THROW_DEF
{
    /*
     * Validate input.
     */
    if (cb == 0)
    {
        AssertMsgFailed(("!cb\n"));
        return VERR_INVALID_PARAMETER;
    }
    if (fProtect & ~(RTMEM_PROT_NONE | RTMEM_PROT_READ | RTMEM_PROT_WRITE | RTMEM_PROT_EXEC))
    {
        AssertMsgFailed(("fProtect=%#x\n", fProtect));
        return VERR_INVALID_PARAMETER;
    }

    /*
     * Convert the flags.
     */
    int fProt;
    Assert(!RTMEM_PROT_NONE);
    switch (fProtect & (RTMEM_PROT_NONE | RTMEM_PROT_READ | RTMEM_PROT_WRITE | RTMEM_PROT_EXEC))
    {
        case RTMEM_PROT_NONE:
            fProt = PAGE_NOACCESS;
            break;

        case RTMEM_PROT_READ:
            fProt = PAGE_READONLY;
            break;

        case RTMEM_PROT_READ | RTMEM_PROT_WRITE:
            fProt = PAGE_READWRITE;
            break;

        case RTMEM_PROT_READ | RTMEM_PROT_WRITE | RTMEM_PROT_EXEC:
            fProt = PAGE_EXECUTE_READWRITE;
            break;

        case RTMEM_PROT_READ | RTMEM_PROT_EXEC:
            fProt = PAGE_EXECUTE_READWRITE;
            break;

        case RTMEM_PROT_WRITE:
            fProt = PAGE_READWRITE;
            break;

        case RTMEM_PROT_WRITE | RTMEM_PROT_EXEC:
            fProt = PAGE_EXECUTE_READWRITE;
            break;

        case RTMEM_PROT_EXEC:
            fProt = PAGE_EXECUTE_READWRITE;
            break;

        /* If the compiler had any brains it would warn about this case. */
        default:
            AssertMsgFailed(("fProtect=%#x\n", fProtect));
            return VERR_INTERNAL_ERROR;
    }

    /*
     * Align the request.
     */
    cb += (uintptr_t)pv & PAGE_OFFSET_MASK;
    pv = (void *)((uintptr_t)pv & ~(uintptr_t)PAGE_OFFSET_MASK);

    /*
     * Change the page attributes.
     */
    DWORD fFlags = 0;
    if (VirtualProtect(pv, cb, fProt, &fFlags))
        return VINF_SUCCESS;
    return RTErrConvertFromWin32(GetLastError());
}

