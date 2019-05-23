/* $Id: kRdrInternal.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kRdr - Internal Header.
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

#ifndef ___kRdrInternal_h___
#define ___kRdrInternal_h___

#include <k/kHlpAssert.h>
#include <k/kMagics.h>
#include <k/kRdrAll.h>
#include <k/kErrors.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup grp_kRdrInternal - Internals
 * @internal
 * @addtogroup grp_kRdr
 * @{
 */

/** @def KRDR_STRICT
 * If defined the kRdr assertions and other runtime checks will be enabled. */
#ifdef K_ALL_STRICT
# undef KRDR_STRICT
# define KRDR_STRICT
#endif

/** @name Our Assert macros
 * @{ */
#ifdef KRDR_STRICT
# define kRdrAssert(expr)                       kHlpAssert(expr)
# define kRdrAssertReturn(expr, rcRet)          kHlpAssertReturn(expr, rcRet)
# define kRdrAssertMsg(expr, msg)               kHlpAssertMsg(expr, msg)
# define kRdrAssertMsgReturn(expr, msg, rcRet)  kHlpAssertMsgReturn(expr, msg, rcRet)
#else   /* !KRDR_STRICT */
# define kRdrAssert(expr)                       do { } while (0)
# define kRdrAssertReturn(expr, rcRet)          do { if (!(expr)) return (rcRet); } while (0)
# define kRdrAssertMsg(expr, msg)               do { } while (0)
# define kRdrAssertMsgReturn(expr, msg, rcRet)  do { if (!(expr)) return (rcRet); } while (0)
#endif  /* !KRDR_STRICT */

#define kRdrAssertPtr(ptr)                      kRdrAssertMsg(K_VALID_PTR(ptr), ("%s = %p\n", #ptr, (ptr)))
#define kRdrAssertPtrReturn(ptr, rcRet)         kRdrAssertMsgReturn(K_VALID_PTR(ptr), ("%s = %p -> %d\n", #ptr, (ptr), (rcRet)), (rcRet))
#define kRdrAssertPtrNull(ptr)                  kRdrAssertMsg(!(ptr) || K_VALID_PTR(ptr), ("%s = %p\n", #ptr, (ptr)))
#define kRdrAssertPtrNullReturn(ptr, rcRet)     kRdrAssertMsgReturn(!(ptr) || K_VALID_PTR(ptr), ("%s = %p -> %d\n", #ptr, (ptr), (rcRet)), (rcRet))
#define kRdrAssertRC(rc)                        kRdrAssertMsg((rc) == 0, ("%s = %d\n", #rc, (rc)))
#define kRdrAssertRCReturn(rc, rcRet)           kRdrAssertMsgReturn((rc) == 0, ("%s = %d -> %d\n", #rc, (rc), (rcRet)), (rcRet))
#define kRdrAssertFailed()                      kRdrAssert(0)
#define kRdrAssertFailedReturn(rcRet)           kRdrAssertReturn(0, (rcRet))
#define kRdrAssertMsgFailed(msg)                kRdrAssertMsg(0, msg)
#define kRdrAssertMsgFailedReturn(msg, rcRet)   kRdrAssertMsgReturn(0, msg, (rcRet))
/** @} */

/** Return / crash validation of a reader argument. */
#define KRDR_VALIDATE_EX(pRdr, rc) \
    do  { \
        if (    (pRdr)->u32Magic != KRDR_MAGIC \
            ||  (pRdr)->pOps == NULL \
           )\
        { \
            return (rc); \
        } \
    } while (0)

/** Return / crash validation of a reader argument. */
#define KRDR_VALIDATE(pRdr) \
    KRDR_VALIDATE_EX(pRdr, KERR_INVALID_PARAMETER)

/** Return / crash validation of a reader argument. */
#define KRDR_VALIDATE_VOID(pRdr) \
    do  { \
        if (    !K_VALID_PTR(pRdr) \
            ||  (pRdr)->u32Magic != KRDR_MAGIC \
            ||  (pRdr)->pOps == NULL \
           )\
        { \
            return; \
        } \
    } while (0)


/** @name Built-in Providers
 * @{ */
extern const KRDROPS g_kRdrFileOps;
/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif

