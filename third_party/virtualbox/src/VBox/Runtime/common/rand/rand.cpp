/* $Id: rand.cpp $ */
/** @file
 * IPRT - Random Numbers.
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
#include <iprt/rand.h>
#include "internal/iprt.h"

#include <iprt/time.h>
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
# include <iprt/asm-amd64-x86.h>
#endif
#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/thread.h>
#include <iprt/once.h>
#include "internal/rand.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** For lazily initializing of the random generator. */
static RTONCE g_rtRandOnce = RTONCE_INITIALIZER;
/** The default random generator. */
static RTRAND g_hRand = NIL_RTRAND;


/**
 * Perform lazy initialization.
 *
 * @returns IPRT status code.
 * @param   pvUser      Ignored.
 */
static DECLCALLBACK(int) rtRandInitOnce(void *pvUser)
{
    RTRAND hRand;
    int rc = RTRandAdvCreateSystemFaster(&hRand);
    if (RT_FAILURE(rc))
        rc = RTRandAdvCreateParkMiller(&hRand);
    if (RT_SUCCESS(rc))
    {
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
        RTRandAdvSeed(hRand, ASMReadTSC() >> 8);
#else
        RTRandAdvSeed(hRand, RTTimeNanoTS() >> 8);
#endif
        g_hRand = hRand;
    }
    else
        AssertRC(rc);

    NOREF(pvUser);
    return rc;
}


/**
 * Termination counterpart to rtRandInitOnce.
 *
 * @returns IPRT status code.
 * @param   pvUser          Ignored.
 * @param   fLazyCleanUpOk  Set if we're terminating the process.
 */
static DECLCALLBACK(void) rtRandTermOnce(void *pvUser, bool fLazyCleanUpOk)
{
    if (!fLazyCleanUpOk)
    {
        RTRAND hRand = g_hRand;
        g_hRand = NIL_RTRAND;
        if (hRand != NIL_RTRAND)
        {
            int rc = RTRandAdvDestroy(hRand);
            AssertRC(rc);
        }
    }
    NOREF(pvUser);
}


RTDECL(void) RTRandBytes(void *pv, size_t cb) RT_NO_THROW_DEF
{
    RTOnceEx(&g_rtRandOnce, rtRandInitOnce, rtRandTermOnce, NULL);
    RTRandAdvBytes(g_hRand, pv, cb);
}
RT_EXPORT_SYMBOL(RTRandBytes);


RTDECL(uint32_t) RTRandU32Ex(uint32_t u32First, uint32_t u32Last) RT_NO_THROW_DEF
{
    RTOnceEx(&g_rtRandOnce, rtRandInitOnce, rtRandTermOnce, NULL);
    return RTRandAdvU32Ex(g_hRand, u32First, u32Last);
}
RT_EXPORT_SYMBOL(RTRandU32Ex);


RTDECL(uint32_t) RTRandU32(void) RT_NO_THROW_DEF
{
    RTOnceEx(&g_rtRandOnce, rtRandInitOnce, rtRandTermOnce, NULL);
    return RTRandAdvU32(g_hRand);
}
RT_EXPORT_SYMBOL(RTRandU32);


RTDECL(int32_t) RTRandS32Ex(int32_t i32First, int32_t i32Last) RT_NO_THROW_DEF
{
    RTOnceEx(&g_rtRandOnce, rtRandInitOnce, rtRandTermOnce, NULL);
    return RTRandAdvS32Ex(g_hRand, i32First, i32Last);
}
RT_EXPORT_SYMBOL(RTRandS32Ex);


RTDECL(int32_t) RTRandS32(void) RT_NO_THROW_DEF
{
    RTOnceEx(&g_rtRandOnce, rtRandInitOnce, rtRandTermOnce, NULL);
    return RTRandAdvS32(g_hRand);
}
RT_EXPORT_SYMBOL(RTRandS32);


RTDECL(uint64_t) RTRandU64Ex(uint64_t u64First, uint64_t u64Last) RT_NO_THROW_DEF
{
    RTOnceEx(&g_rtRandOnce, rtRandInitOnce, rtRandTermOnce, NULL);
    return RTRandAdvU64Ex(g_hRand, u64First, u64Last);
}
RT_EXPORT_SYMBOL(RTRandU64Ex);


RTDECL(uint64_t) RTRandU64(void) RT_NO_THROW_DEF
{
    RTOnceEx(&g_rtRandOnce, rtRandInitOnce, rtRandTermOnce, NULL);
    return RTRandAdvU64(g_hRand);
}
RT_EXPORT_SYMBOL(RTRandU64);


RTDECL(int64_t) RTRandS64Ex(int64_t i64First, int64_t i64Last) RT_NO_THROW_DEF
{
    RTOnceEx(&g_rtRandOnce, rtRandInitOnce, rtRandTermOnce, NULL);
    return RTRandAdvS64Ex(g_hRand, i64First, i64Last);
}
RT_EXPORT_SYMBOL(RTRandS64Ex);


RTDECL(int64_t) RTRandS64(void) RT_NO_THROW_DEF
{
    RTOnceEx(&g_rtRandOnce, rtRandInitOnce, rtRandTermOnce, NULL);
    return RTRandAdvS32(g_hRand);
}
RT_EXPORT_SYMBOL(RTRandS64);

