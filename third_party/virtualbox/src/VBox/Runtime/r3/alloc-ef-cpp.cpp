/* $Id: alloc-ef-cpp.cpp $ */
/** @file
 * IPRT - Memory Allocation, C++ electric fence.
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
#include "alloc-ef.h"

#include <iprt/asm.h>
#include <new>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** @todo test this on MSC */

/** MSC declares the operators as cdecl it seems. */
#ifdef _MSC_VER
# define RT_EF_CDECL    __cdecl
#else
# define RT_EF_CDECL
#endif

/** MSC doesn't use the standard namespace. */
#ifdef _MSC_VER
# define RT_EF_SIZE_T   size_t
#else
# define RT_EF_SIZE_T   std::size_t
#endif

/** The hint that we're throwing std::bad_alloc is not apprecitated by MSC. */
#ifdef RT_EXCEPTIONS_ENABLED
# ifdef _MSC_VER
#  define RT_EF_THROWS_BAD_ALLOC
#  define RT_EF_NOTHROW               RT_NO_THROW_DEF
# else
#  ifdef _GLIBCXX_THROW
#   define RT_EF_THROWS_BAD_ALLOC     _GLIBCXX_THROW(std::bad_alloc)
#  else
#   define RT_EF_THROWS_BAD_ALLOC     throw(std::bad_alloc)
#  endif
#  define RT_EF_NOTHROW               throw()
# endif
#else  /* !RT_EXCEPTIONS_ENABLED */
# define RT_EF_THROWS_BAD_ALLOC
# define RT_EF_NOTHROW
#endif /* !RT_EXCEPTIONS_ENABLED */


void *RT_EF_CDECL operator new(RT_EF_SIZE_T cb) RT_EF_THROWS_BAD_ALLOC
{
    void *pv = rtR3MemAlloc("new", RTMEMTYPE_NEW, cb, cb, NULL, ASMReturnAddress(), NULL, 0, NULL);
    if (!pv)
        throw std::bad_alloc();
    return pv;
}


void *RT_EF_CDECL operator new(RT_EF_SIZE_T cb, const std::nothrow_t &) RT_EF_NOTHROW
{
    void *pv = rtR3MemAlloc("new nothrow", RTMEMTYPE_NEW, cb, cb, NULL, ASMReturnAddress(), NULL, 0, NULL);
    return pv;
}


void RT_EF_CDECL operator delete(void *pv) RT_EF_NOTHROW
{
    rtR3MemFree("delete", RTMEMTYPE_DELETE, pv, ASMReturnAddress(), NULL, 0, NULL);
}


#ifdef __cpp_sized_deallocation
void RT_EF_CDECL operator delete(void *pv, RT_EF_SIZE_T cb) RT_EF_NOTHROW
{
    NOREF(cb);
    AssertMsgFailed(("cb ignored!\n"));
    rtR3MemFree("delete", RTMEMTYPE_DELETE, pv, ASMReturnAddress(), NULL, 0, NULL);
}
#endif


void RT_EF_CDECL operator delete(void * pv, const std::nothrow_t &) RT_EF_NOTHROW
{
    rtR3MemFree("delete nothrow", RTMEMTYPE_DELETE, pv, ASMReturnAddress(), NULL, 0, NULL);
}


/*
 *
 * Array object form.
 * Array object form.
 * Array object form.
 *
 */

void *RT_EF_CDECL operator new[](RT_EF_SIZE_T cb) RT_EF_THROWS_BAD_ALLOC
{
    void *pv = rtR3MemAlloc("new[]", RTMEMTYPE_NEW_ARRAY, cb, cb, NULL, ASMReturnAddress(), NULL, 0, NULL);
    if (!pv)
        throw std::bad_alloc();
    return pv;
}


void * RT_EF_CDECL operator new[](RT_EF_SIZE_T cb, const std::nothrow_t &) RT_EF_NOTHROW
{
    void *pv = rtR3MemAlloc("new[] nothrow", RTMEMTYPE_NEW_ARRAY, cb, cb, NULL, ASMReturnAddress(), NULL, 0, NULL);
    return pv;
}


void RT_EF_CDECL operator delete[](void * pv) RT_EF_NOTHROW
{
    rtR3MemFree("delete[]", RTMEMTYPE_DELETE_ARRAY, pv, ASMReturnAddress(), NULL, 0, NULL);
}


#ifdef __cpp_sized_deallocation
void RT_EF_CDECL operator delete[](void * pv, RT_EF_SIZE_T cb) RT_EF_NOTHROW
{
    NOREF(cb);
    AssertMsgFailed(("cb ignored!\n"));
    rtR3MemFree("delete[]", RTMEMTYPE_DELETE_ARRAY, pv, ASMReturnAddress(), NULL, 0, NULL);
}
#endif


void RT_EF_CDECL operator delete[](void *pv, const std::nothrow_t &) RT_EF_NOTHROW
{
    rtR3MemFree("delete[] nothrow", RTMEMTYPE_DELETE_ARRAY, pv, ASMReturnAddress(), NULL, 0, NULL);
}

