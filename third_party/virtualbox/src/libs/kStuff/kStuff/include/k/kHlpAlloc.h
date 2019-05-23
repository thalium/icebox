/* $Id: kHlpAlloc.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpAlloc - Memory Allocation.
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

#ifndef ___k_kHlpAlloc_h___
#define ___k_kHlpAlloc_h___

#include <k/kHlpDefs.h>
#include <k/kTypes.h>

/** @defgroup grp_kHlpAlloc kHlpAlloc - Memory Allocation
 * @addtogroup grp_kHlp
 * @{*/

/** @def kHlpAllocA
 * The alloca() wrapper. */
#ifdef __GNUC__
# define kHlpAllocA(a)      __builtin_alloca(a)
#elif defined(_MSC_VER)
# include <malloc.h>
# define kHlpAllocA(a)      alloca(a)
#else
# error "Port Me."
#endif

#ifdef __cplusplus
extern "C" {
#endif

KHLP_DECL(void *)   kHlpAlloc(KSIZE cb);
KHLP_DECL(void *)   kHlpAllocZ(KSIZE cb);
KHLP_DECL(void *)   kHlpDup(const void *pv, KSIZE cb);
KHLP_DECL(char *)   kHlpStrDup(const char *psz);
KHLP_DECL(void *)   kHlpRealloc(void *pv, KSIZE cb);
KHLP_DECL(void)     kHlpFree(void *pv);

KHLP_DECL(int)      kHlpPageAlloc(void **ppv, KSIZE cb, KPROT enmProt, KBOOL fFixed);
KHLP_DECL(int)      kHlpPageProtect(void *pv, KSIZE cb, KPROT enmProt);
KHLP_DECL(int)      kHlpPageFree(void *pv, KSIZE cb);

KHLP_DECL(int)      kHlpHeapInit(void);
KHLP_DECL(void)     kHlpHeapTerm(void);
KHLP_DECL(void)     kHlpHeapDonate(void *pv, KSIZE cb);

#ifdef __cplusplus
}
#endif

/** @} */

#endif

