/* $Id: kHlpString.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpString - String And Memory Routines.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ___k_kHlpString_h___
#define ___k_kHlpString_h___

#include <k/kHlpDefs.h>
#include <k/kTypes.h>

#if 0 /* optimize / fix this later */
#ifdef __GNUC__
/** memchr */
# define kHlpMemChr(a,b,c)      __builtin_memchr(a,b,c)
/** memcmp */
# define kHlpMemComp(a,b,c)     __builtin_memcmp(a,b,c)
/** memcpy */
# define kHlpMemCopy(a,b,c)     __builtin_memcpy(a,b,c)
/** memset */
# define kHlpMemSet(a,b,c)      __builtin_memset(a,b,c)
/** strchr */
# define kHlpStrChr(a, b)       __builtin_strchr(a, b)
/** strcmp */
# define kHlpStrComp(a, b)      __builtin_strcmp(a, b)
/** strncmp */
# define kHlpStrNComp(a,b,c)    __builtin_strncmp(a, b, c)
/** strlen */
# define kHlpStrLen(a)          __builtin_strlen(a)

#elif defined(_MSC_VER)
# pragma intrinsic(memcmp, memcpy, memset, strcmp, strlen)
/** memcmp */
# define kHlpMemComp(a,b,c)     memcmp(a,b,c)
/** memcpy */
# define kHlpMemCopy(a,b,c)     memcpy(a,b,c)
/** memset */
# define kHlpMemSet(a,b,c)      memset(a,b,c)
/** strcmp */
# define kHlpStrComp(a, b)      strcmp(a, b)
/** strlen */
# define kHlpStrLen(a)          strlen(a)
#else
# error "Port Me"
#endif
#endif /* disabled */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef kHlpMemChr
KHLP_DECL(void *)   kHlpMemChr(const void *pv, int ch, KSIZE cb);
#endif
#ifndef kHlpMemComp
KHLP_DECL(int)      kHlpMemComp(const void *pv1, const void *pv2, KSIZE cb);
#endif
#ifndef kHlpMemComp
KHLP_DECL(void *)   kHlpMemPComp(const void *pv1, const void *pv2, KSIZE cb);
#endif
#ifndef kHlpMemCopy
KHLP_DECL(void *)   kHlpMemCopy(void *pv1, const void *pv2, KSIZE cb);
#endif
#ifndef kHlpMemPCopy
KHLP_DECL(void *)   kHlpMemPCopy(void *pv1, const void *pv2, KSIZE cb);
#endif
#ifndef kHlpMemMove
KHLP_DECL(void *)   kHlpMemMove(void *pv1, const void *pv2, KSIZE cb);
#endif
#ifndef kHlpMemPMove
KHLP_DECL(void *)   kHlpMemPMove(void *pv1, const void *pv2, KSIZE cb);
#endif
#ifndef kHlpMemSet
KHLP_DECL(void *)   kHlpMemSet(void *pv1, int ch, KSIZE cb);
#endif
#ifndef kHlpMemPSet
KHLP_DECL(void *)   kHlpMemPSet(void *pv1, int ch, KSIZE cb);
#endif
KHLP_DECL(int)      kHlpMemICompAscii(const void *pv1, const void *pv2, KSIZE cb);

#ifndef kHlpStrCat
KHLP_DECL(char *)   kHlpStrCat(char *psz1, const char *psz2);
#endif
#ifndef kHlpStrPCat
KHLP_DECL(char *)   kHlpStrPCat(char *psz1, const char *psz2);
#endif
#ifndef kHlpStrNCat
KHLP_DECL(char *)   kHlpStrNCat(char *psz1, const char *psz2, KSIZE cb);
#endif
#ifndef kHlpStrPNCat
KHLP_DECL(char *)   kHlpStrNPCat(char *psz1, const char *psz2, KSIZE cb);
#endif
#ifndef kHlpStrChr
KHLP_DECL(char *)   kHlpStrChr(const char *psz, int ch);
#endif
#ifndef kHlpStrRChr
KHLP_DECL(char *)   kHlpStrRChr(const char *psz, int ch);
#endif
#ifndef kHlpStrComp
KHLP_DECL(int)      kHlpStrComp(const char *psz1, const char *psz2);
#endif
KHLP_DECL(char *)   kHlpStrPComp(const char *psz1, const char *psz2);
#ifndef kHlpStrNComp
KHLP_DECL(int)      kHlpStrNComp(const char *psz1, const char *psz2, KSIZE cch);
#endif
KHLP_DECL(char *)   kHlpStrNPComp(const char *psz1, const char *psz2, KSIZE cch);
KHLP_DECL(int)      kHlpStrICompAscii(const char *pv1, const char *pv2);
KHLP_DECL(char *)   kHlpStrIPCompAscii(const char *pv1, const char *pv2);
KHLP_DECL(int)      kHlpStrNICompAscii(const char *pv1, const char *pv2, KSIZE cch);
KHLP_DECL(char *)   kHlpStrNIPCompAscii(const char *pv1, const char *pv2, KSIZE cch);
#ifndef kHlpStrCopy
KHLP_DECL(char *)   kHlpStrCopy(char *psz1, const char *psz2);
#endif
#ifndef kHlpStrPCopy
KHLP_DECL(char *)   kHlpStrPCopy(char *psz1, const char *psz2);
#endif
#ifndef kHlpStrLen
KHLP_DECL(KSIZE)    kHlpStrLen(const char *psz1);
#endif
#ifndef kHlpStrNLen
KHLP_DECL(KSIZE)    kHlpStrNLen(const char *psz, KSIZE cchMax);
#endif

KHLP_DECL(char *)   kHlpInt2Ascii(char *psz, KSIZE cch, long lVal, unsigned iBase);

#ifdef __cplusplus
}
#endif

#endif

