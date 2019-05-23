/* $Id: inifile.h $ */
/** @file
 * IPRT - INI-file parser.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
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

#ifndef ___iprt_inifile_h
#define ___iprt_inifile_h


#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_inifile    RTIniFile - INI-file parser
 * @ingroup grp_rt
 * @{
 */

/** @name RTINIFILE_F_XXX - INI-file open flags.
 * @{ */
/** Readonly. */
#define RTINIFILE_F_READONLY        RT_BIT(0)
/** Valid mask. */
#define RTINIFILE_F_VALID_MASK      UINT32_C(0x00000001)
/** @} */



/**
 * Creates a INI-file instance from a VFS file handle.
 *
 * @returns IPRT status code
 * @param   phIniFile       Where to return the INI-file handle.
 * @param   hVfsFile        The VFS file handle (not consumed, additional
 *                          reference is retained).
 * @param   fFlags          Flags, RTINIFILE_F_XXX.
 */
RTDECL(int)      RTIniFileCreateFromVfsFile(PRTINIFILE phIniFile, RTVFSFILE hVfsFile, uint32_t fFlags);

/**
 * Retains a reference to an INI-file instance.
 *
 * @returns New reference count, UINT32_MAX on failure.
 * @param   hIniFile        The INI-file handle.
 */
RTDECL(uint32_t) RTIniFileRetain(RTINIFILE hIniFile);

/**
 * Releases a reference to an INI-file instance, destroying it if the count
 * reaches zero.
 *
 * @returns New reference count, UINT32_MAX on failure.
 * @param   hIniFile        The INI-file handle.  NIL is ignored.
 */
RTDECL(uint32_t) RTIniFileRelease(RTINIFILE hIniFile);

/**
 * Queries a named value in a section.
 *
 * The first matching value is returned.  The matching is by default case
 * insensitive.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if section or key not found.
 *
 * @param   hIniFile        The INI-file handle.
 * @param   pszSection      The section name.  Pass NULL to refer to the
 *                          unsectioned key space at the top of the file.
 * @param   pszKey          The key name.
 * @param   pszValue        Where to return the value.
 * @param   cbValue         Size of the buffer @a pszValue points to.
 * @param   pcbActual       Where to return the actual value size excluding
 *                          terminator on success.  On VERR_BUFFER_OVERFLOW this
 *                          will be set to the buffer size needed to hold the
 *                          value, terminator included.  Optional.
 */
RTDECL(int)      RTIniFileQueryValue(RTINIFILE hIniFile, const char *pszSection, const char *pszKey,
                                     char *pszValue, size_t cbValue, size_t *pcbActual);

/**
 * Queries a key-value pair in a section by ordinal.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if the section wasn't found or if it contains no pair
 *          with the given ordinal value.
 *
 * @param   hIniFile        The INI-file handle.
 * @param   pszSection      The section name.  Pass NULL to refer to the
 *                          unsectioned key space at the top of the file.
 * @param   idxPair         The pair to fetch (counting from 0).
 *
 * @param   pszKey          Where to return the key name.
 * @param   cbKey           Size of the buffer @a pszKey points to.
 * @param   pcbKeyActual    Where to return the actual key size excluding
 *                          terminator on success.  On VERR_BUFFER_OVERFLOW this
 *                          will be set to the buffer size needed to hold the
 *                          value, terminator included.  Optional.
 *
 * @param   pszValue        Where to return the value.
 * @param   cbValue         Size of the buffer @a pszValue points to.
 * @param   pcbValueActual  Where to return the actual value size excluding
 *                          terminator on success.  On VERR_BUFFER_OVERFLOW this
 *                          will be set to the buffer size needed to hold the
 *                          value, terminator included. Optional.
 */
RTDECL(int)      RTIniFileQueryPair(RTINIFILE hIniFile, const char *pszSection, uint32_t idxPair,
                                    char *pszKey, size_t cbKey, size_t *pcbKeyActual,
                                    char *pszValue, size_t cbValue, size_t *pcbValueActual);


/** @} */

RT_C_DECLS_END

#endif

