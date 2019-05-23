/** @file
 * IPRT - Universal Unique Identifiers (UUID).
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

#ifndef ___iprt_uuid_h
#define ___iprt_uuid_h

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_uuid       RTUuid - Universally Unique Identifiers
 * @ingroup grp_rt
 * @{
 */

/**
 * Generates new UUID value.
 *
 * @note IPRT uses little endian byte ordering in the UUID integer fields. If
 * you want to pass IPRT UUIDs in binary representation to other UUID libraries
 * and expect to get exactly the same string representation as in IPRT, you
 * need to convert the first three integer fields (one 32 bit value, two 16 bit
 * values) separately to big endian (also called network byte order).
 *
 * @sa RTUUID::Gen
 *
 * @returns iprt status code.
 * @param   pUuid           Where to store generated uuid.
 */
RTDECL(int)  RTUuidCreate(PRTUUID pUuid);

/**
 * Makes null UUID value.
 *
 * @returns iprt status code.
 * @param   pUuid           Where to store generated null uuid.
 */
RTDECL(int)  RTUuidClear(PRTUUID pUuid);

/**
 * Checks if UUID is null.
 *
 * @returns true if UUID is null.
 * @param   pUuid           uuid to check.
 */
RTDECL(bool)  RTUuidIsNull(PCRTUUID pUuid);

/**
 * Compares two UUID values.
 *
 * @returns 0 if eq, < 0 or > 0.
 * @param   pUuid1          First value to compare.  NULL is treated like if
 *                          RTUuidIsNull() return true.
 * @param   pUuid2          Second value to compare.  NULL is treated like if
 *                          RTUuidIsNull() return true.
 */
RTDECL(int)  RTUuidCompare(PCRTUUID pUuid1, PCRTUUID pUuid2);

/**
 * Compares a UUID value with a UUID string.
 *
 * @note IPRT uses little endian byte ordering in the UUID integer fields. If
 * you want to pass IPRT UUIDs in binary representation to other UUID libraries
 * and expect to get exactly the same string representation as in IPRT, you need
 * to convert the first three integer fields (one 32 bit value, two 16 bit
 * values) separately to big endian (also called network byte order).
 * Correspondingly, if you want to get the right result with UUIDs which are in
 * big endian format, you need to convert them before using this function.
 *
 * @sa RTUUID::Gen
 *
 * @returns 0 if eq, < 0 or > 0.
 * @param   pUuid1          First value to compare.  NULL is not allowed.
 * @param   pszString2      The 2nd UUID in string form.  NULL or malformed
 *                          string is not permitted.
 */
RTDECL(int)  RTUuidCompareStr(PCRTUUID pUuid1, const char *pszString2);

/**
 * Compares two UUID strings.
 *
 * @returns 0 if eq, < 0 or > 0.
 * @param   pszString1      The 1st UUID in string from.  NULL or malformed
 *                          string is not permitted.
 * @param   pszString2      The 2nd UUID in string form.  NULL or malformed
 *                          string is not permitted.
 */
RTDECL(int)  RTUuidCompare2Strs(const char *pszString1, const char *pszString2);

/**
 * Converts binary UUID to its string representation.
 *
 * @note IPRT uses little endian byte ordering in the UUID integer fields. If
 * you want to pass IPRT UUIDs in binary representation to other UUID libraries
 * and expect to get exactly the same string representation as in IPRT, you
 * need to convert the first three integer fields (one 32 bit value, two 16 bit
 * values) separately to big endian (also called network byte order).
 * Correspondingly, if you want to get the right result with UUIDs which are in
 * big endian format, you need to convert them before using this function.
 *
 * @sa RTUUID::Gen
 *
 * @returns iprt status code.
 * @param   pUuid           Uuid to convert.
 * @param   pszString       Where to store result string.
 * @param   cchString       pszString buffer length, must be >= RTUUID_STR_LENGTH.
 */
RTDECL(int)  RTUuidToStr(PCRTUUID pUuid, char *pszString, size_t cchString);

/**
 * Converts UUID from its string representation to binary format.
 *
 * @note IPRT uses little endian byte ordering in the UUID integer fields. If
 * you want to pass IPRT UUIDs in binary representation to other UUID libraries
 * and expect to get exactly the same string representation as in IPRT, you
 * need to convert the first three integer fields (one 32 bit value, two 16 bit
 * values) separately to big endian (also called network byte order).
 * Correspondingly, if you want to get the right result with UUIDs which are in
 * big endian format, you need to convert them before using this function.
 *
 * @sa RTUUID::Gen
 *
 * @returns iprt status code.
 * @param   pUuid           Where to store result Uuid.
 * @param   pszString       String with UUID text data.
 */
RTDECL(int)  RTUuidFromStr(PRTUUID pUuid, const char *pszString);

/**
 * Converts binary UUID to its UTF-16 string representation.
 *
 * @note See note in RTUuidToStr.
 *
 * @sa RTUUID::Gen
 *
 * @returns iprt status code.
 * @param   pUuid           Uuid to convert.
 * @param   pwszString      Where to store result string.
 * @param   cwcString       pszString buffer length, must be >=
 *                          RTUUID_STR_LENGTH.
 */
RTDECL(int)  RTUuidToUtf16(PCRTUUID pUuid, PRTUTF16 pwszString, size_t cwcString);

/**
 * Converts UUID from its UTF-16 string representation to binary format.
 *
 * @note See note in RTUuidFromStr.
 *
 * @sa RTUUID::Gen
 *
 * @returns iprt status code.
 * @param   pUuid           Where to store result Uuid.
 * @param   pwszString      String with UUID text data.
 */
RTDECL(int)  RTUuidFromUtf16(PRTUUID pUuid, PCRTUTF16 pwszString);

/** @} */

RT_C_DECLS_END

#endif

