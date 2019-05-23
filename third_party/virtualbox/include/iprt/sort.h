/** @file
 * IPRT - Sorting.
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

#ifndef ___iprt_sort_h
#define ___iprt_sort_h

#include <iprt/types.h>

/** @defgroup grp_rt_sort       RTSort - Sorting Algorithms
 * @ingroup grp_rt
 * @{ */

RT_C_DECLS_BEGIN

/**
 * Callback for comparing two array elements.
 *
 * @retval  0 if equal.
 * @retval  -1 if @a pvElement1 comes before @a pvElement2.
 * @retval  1  if @a pvElement1 comes after  @a pvElement2.
 *
 * @param   pvElement1      The 1st element.
 * @param   pvElement2      The 2nd element.
 * @param   pvUser          The user argument passed to the sorting function.
 */
typedef DECLCALLBACK(int) FNRTSORTCMP(void const *pvElement1, void const *pvElement2, void *pvUser);
/** Pointer to a compare function. */
typedef FNRTSORTCMP *PFNRTSORTCMP;

/**
 * Sorter function for an array of variable sized elementes.
 *
 * @param   papvArray       The array to sort.
 * @param   cElements       The number of elements in the array.
 * @param   cbElement       The size of an array element.
 * @param   pfnCmp          Callback function comparing two elements.
 * @param   pvUser          User argument for the callback.
 */
typedef DECLCALLBACK(void) FNRTSORT(void *pvArray, size_t cElements, size_t cbElement, PFNRTSORTCMP pfnCmp, void *pvUser);
/** Pointer to a sorter function for an array of variable sized elements. */
typedef FNRTSORT *PFNRTSORT;

/**
 * Pointer array sorter function.
 *
 * @param   papvArray       The array to sort.
 * @param   cElements       The number of elements in the array.
 * @param   pfnCmp          Callback function comparing two elements.
 * @param   pvUser          User argument for the callback.
 */
typedef DECLCALLBACK(void) FNRTSORTAPV(void **papvArray, size_t cElements, PFNRTSORTCMP pfnCmp, void *pvUser);
/** Pointer to a pointer array sorter function. */
typedef FNRTSORTAPV *PFNRTSORTAPV;

/**
 * Shell sort an array of variable sized elementes.
 *
 * @param   pvArray         The array to sort.
 * @param   cElements       The number of elements in the array.
 * @param   cbElement       The size of an array element.
 * @param   pfnCmp          Callback function comparing two elements.
 * @param   pvUser          User argument for the callback.
 */
RTDECL(void) RTSortShell(void *pvArray, size_t cElements, size_t cbElement, PFNRTSORTCMP pfnCmp, void *pvUser);

/**
 * Same as RTSortShell but speciallized for an array containing element
 * pointers.
 *
 * @param   papvArray       The array to sort.
 * @param   cElements       The number of elements in the array.
 * @param   pfnCmp          Callback function comparing two elements.
 * @param   pvUser          User argument for the callback.
 */
RTDECL(void) RTSortApvShell(void **papvArray, size_t cElements, PFNRTSORTCMP pfnCmp, void *pvUser);

/**
 * Checks if an array of variable sized elementes is sorted.
 *
 * @returns true if it is sorted, false if it isn't.
 * @param   pvArray         The array to check.
 * @param   cElements       The number of elements in the array.
 * @param   cbElement       The size of an array element.
 * @param   pfnCmp          Callback function comparing two elements.
 * @param   pvUser          User argument for the callback.
 */
RTDECL(bool) RTSortIsSorted(void const *pvArray, size_t cElements, size_t cbElement, PFNRTSORTCMP pfnCmp, void *pvUser);

/**
 * Same as RTSortShell but speciallized for an array containing element
 * pointers.
 *
 * @returns true if it is sorted, false if it isn't.
 * @param   papvArray       The array to check.
 * @param   cElements       The number of elements in the array.
 * @param   pfnCmp          Callback function comparing two elements.
 * @param   pvUser          User argument for the callback.
 */
RTDECL(bool) RTSortApvIsSorted(void const * const *papvArray, size_t cElements, PFNRTSORTCMP pfnCmp, void *pvUser);

RT_C_DECLS_END

/** @} */

#endif

