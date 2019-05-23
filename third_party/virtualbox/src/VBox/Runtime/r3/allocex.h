/* $Id: allocex.h $ */
/** @file
 * IPRT - Memory Allocation, Extended Alloc and Free Functions for Ring-3.
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


#ifndef ___r3_allocex_h
#define ___r3_allocex_h

#include <iprt/types.h>

/**
 * Heading for extended memory allocations in ring-3.
 */
typedef struct RTMEMHDRR3
{
    /** Magic (RTMEMHDR_MAGIC). */
    uint32_t    u32Magic;
    /** Block flags (RTMEMALLOCEX_FLAGS_*). */
    uint32_t    fFlags;
    /** The actual size of the block, header not included. */
    uint32_t    cb;
    /** The requested allocation size. */
    uint32_t    cbReq;
} RTMEMHDRR3;
/** Pointer to a ring-3 extended memory header. */
typedef RTMEMHDRR3 *PRTMEMHDRR3;


/**
 * Allocate memory in below 64KB.
 *
 * @returns IPRT status code.
 * @param   cbAlloc             Number of bytes to allocate (including the
 *                              header if the caller needs one).
 * @param   fFlags              Allocation flags.
 * @param   ppv                 Where to return the pointer to the memory.
 */
DECLHIDDEN(int) rtMemAllocEx16BitReach(size_t cbAlloc, uint32_t fFlags, void **ppv);

/**
 * Allocate memory in below 4GB.
 *
 * @returns IPRT status code.
 * @param   cbAlloc             Number of bytes to allocate (including the
 *                              header if the caller needs one).
 * @param   fFlags              Allocation flags.
 * @param   ppv                 Where to return the pointer to the memory.
 */
DECLHIDDEN(int) rtMemAllocEx32BitReach(size_t cbAlloc, uint32_t fFlags, void **ppv);


/**
 * Frees memory allocated by rtMemAllocEx16BitReach and rtMemAllocEx32BitReach.
 *
 * @param   pv                  Start of allocation.
 * @param   cb                  Allocation size.
 * @param   fFlags              Allocation flags.
 */
DECLHIDDEN(void) rtMemFreeExYyBitReach(void *pv, size_t cb, uint32_t fFlags);

#endif

