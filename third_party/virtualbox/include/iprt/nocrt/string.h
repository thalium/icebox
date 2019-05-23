/** @file
 * IPRT / No-CRT - string.h.
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

#ifndef ___iprt_nocrt_string_h
#define ___iprt_nocrt_string_h

#include <iprt/types.h>

RT_C_DECLS_BEGIN

void *  RT_NOCRT(memchr)(const void *pv, int ch, size_t cb);
int     RT_NOCRT(memcmp)(const void *pv1, const void *pv2, size_t cb);
void *  RT_NOCRT(memcpy)(void *pvDst, const void *pvSrc, size_t cb);
void *  RT_NOCRT(mempcpy)(void *pvDst, const void *pvSrc, size_t cb);
void *  RT_NOCRT(memmove)(void *pvDst, const void *pvSrc, size_t cb);
void *  RT_NOCRT(memset)(void *pvDst, int ch, size_t cb);

char *  RT_NOCRT(strcat)(char *pszDst, const char *pszSrc);
char *  RT_NOCRT(strncat)(char *pszDst, const char *pszSrc, size_t cch);
char *  RT_NOCRT(strchr)(const char *psz, int ch);
int     RT_NOCRT(strcmp)(const char *psz1, const char *psz2);
int     RT_NOCRT(strncmp)(const char *psz1, const char *psz2, size_t cch);
int     RT_NOCRT(stricmp)(const char *psz1, const char *psz2);
int     RT_NOCRT(strnicmp)(const char *psz1, const char *psz2, size_t cch);
char *  RT_NOCRT(strcpy)(char *pszDst, const char *pszSrc);
char *  RT_NOCRT(strncpy)(char *pszDst, const char *pszSrc, size_t cch);
char *  RT_NOCRT(strcat)(char *pszDst, const char *pszSrc);
char *  RT_NOCRT(strncat)(char *pszDst, const char *pszSrc, size_t cch);
size_t  RT_NOCRT(strlen)(const char *psz);
size_t  RT_NOCRT(strnlen)(const char *psz, size_t cch);
char *  RT_NOCRT(strstr)(const char *psz, const char *pszSub);

#if !defined(RT_WITHOUT_NOCRT_WRAPPERS) && !defined(RT_WITHOUT_NOCRT_WRAPPER_ALIASES)
# define memchr   RT_NOCRT(memchr)
# define memcmp   RT_NOCRT(memcmp)
# define memcpy   RT_NOCRT(memcpy)
# define mempcpy  RT_NOCRT(mempcpy)
# define memmove  RT_NOCRT(memmove)
# define memset   RT_NOCRT(memset)
# define strcat   RT_NOCRT(strcat)
# define strncat  RT_NOCRT(strncat)
# define strchr   RT_NOCRT(strchr)
# define strcmp   RT_NOCRT(strcmp)
# define strncmp  RT_NOCRT(strncmp)
# define stricmp  RT_NOCRT(stricmp)
# define strnicmp RT_NOCRT(strnicmp)
# define strcpy   RT_NOCRT(strcpy)
# define strncpy  RT_NOCRT(strncpy)
# define strcat   RT_NOCRT(strcat)
# define strncat  RT_NOCRT(strncat)
# define strlen   RT_NOCRT(strlen)
# define strnlen  RT_NOCRT(strnlen)
# define strstr   RT_NOCRT(strstr)
#endif

RT_C_DECLS_END

#endif
