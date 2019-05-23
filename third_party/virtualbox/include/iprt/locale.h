/** @file
 * IPRT - Locale and Related Info.
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

#ifndef ___iprt_locale_h
#define ___iprt_locale_h

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_locale RTLocale - Locale and Related Info
 * @ingroup grp_rt
 * @{
 */

/**
 * Returns the setlocale(LC_ALL,NULL) return value.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if not supported.
 * @param   pszName         Where to return the name.
 * @param   cbName          The size of the name buffer.
 */
RTDECL(int) RTLocaleQueryLocaleName(char *pszName, size_t cbName);

/**
 * Returns a normalized base locale name ('{ll}_{CC}' or 'C').
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if not supported.
 * @param   pszName         Where to return the name.
 * @param   cbName          The size of the name buffer.
 *
 * @sa      RTLOCALE_IS_LANGUAGE2_UNDERSCORE_COUNTRY2
 */
RTDECL(int) RTLocaleQueryNormalizedBaseLocaleName(char *pszName, size_t cbName);

/**
 * Gets the two letter country code (ISO 3166-1 alpha-2) for the current user.
 *
 * This is not necessarily the country from the locale name, when possible the
 * source is a different setting (host specific).
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_SUPPORTED if not supported.
 * @param   pszCountryCode  Pointer buffer that's at least three bytes in size.
 *                          The country code will be returned here on success.
 */
RTDECL(int) RTLocaleQueryUserCountryCode(char pszCountryCode[3]);


/**
 * Checks whether @a a_psz seems to start with a
 * language-code-underscore-country-code sequence.
 *
 * We perform a check for a likely ISO 639-1 language code, followed by an
 * underscore, followed by a likely ISO 3166-1 alpha-2 country code.
 *
 * @return true if probable '{ll}_{CC}' sequence, false if surely not.
 * @param  a_psz        The string to test the start of.
 *
 * @note User must include iprt/ctype.h separately.
 */
#define RTLOCALE_IS_LANGUAGE2_UNDERSCORE_COUNTRY2(a_psz) \
    (   RT_C_IS_LOWER((a_psz)[0]) \
     && RT_C_IS_LOWER((a_psz)[1]) \
     && (a_psz)[2] == '_' \
     && RT_C_IS_UPPER((a_psz)[3]) \
     && RT_C_IS_UPPER((a_psz)[4]) )


/** @} */

RT_C_DECLS_END

#endif


