/* $Id: kDbgInternal.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kDbg - The Debug Info Reader, Internal Header.
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

#ifndef ___kDbgInternal_h___
#define ___kDbgInternal_h___

#include <k/kHlpAssert.h>
#include <k/kMagics.h>
#include <k/kErrors.h>
#include <k/kDbgAll.h>


/** @defgroup grp_kDbgInternal  Internal
 * @internal
 * @addtogroup grp_kDbg
 * @{
 */

/** @def KDBG_STRICT
 * If defined the kDbg assertions and other runtime checks will be enabled. */
#ifdef K_ALL_STRICT
# undef KDBG_STRICT
# define KDBG_STRICT
#endif

/** @name Our Assert macros
 * @{ */
#ifdef KDBG_STRICT
# define kDbgAssert(expr)                       kHlpAssert(expr)
# define kDbgAssertReturn(expr, rcRet)          kHlpAssertReturn(expr, rcRet)
# define kDbgAssertReturnVoid(expr)             kHlpAssertReturnVoid(expr)
# define kDbgAssertMsg(expr, msg)               kHlpAssertMsg(expr, msg)
# define kDbgAssertMsgReturn(expr, msg, rcRet)  kHlpAssertMsgReturn(expr, msg, rcRet)
# define kDbgAssertMsgReturnVoid(expr, msg)     kHlpAssertMsgReturnVoid(expr, msg)
#else   /* !KDBG_STRICT */
# define kDbgAssert(expr)                       do { } while (0)
# define kDbgAssertReturn(expr, rcRet)          do { if (!(expr)) return (rcRet); } while (0)
# define kDbgAssertMsg(expr, msg)               do { } while (0)
# define kDbgAssertMsgReturn(expr, msg, rcRet)  do { if (!(expr)) return (rcRet); } while (0)
#endif  /* !KDBG_STRICT */

#define kDbgAssertPtr(ptr)                      kDbgAssertMsg(K_VALID_PTR(ptr), ("%s = %p\n", #ptr, (ptr)))
#define kDbgAssertPtrReturn(ptr, rcRet)         kDbgAssertMsgReturn(K_VALID_PTR(ptr), ("%s = %p -> %d\n", #ptr, (ptr), (rcRet)), (rcRet))
#define kDbgAssertPtrReturnVoid(ptr)            kDbgAssertMsgReturnVoid(K_VALID_PTR(ptr), ("%s = %p -> %d\n", #ptr, (ptr), (rcRet)))
#define kDbgAssertPtrNull(ptr)                  kDbgAssertMsg(!(ptr) || K_VALID_PTR(ptr), ("%s = %p\n", #ptr, (ptr)))
#define kDbgAssertPtrNullReturn(ptr, rcRet)     kDbgAssertMsgReturn(!(ptr) || K_VALID_PTR(ptr), ("%s = %p -> %d\n", #ptr, (ptr), (rcRet)), (rcRet))
#define kDbgAssertPtrNullReturnVoid(ptr)        kDbgAssertMsgReturnVoid(!(ptr) || K_VALID_PTR(ptr), ("%s = %p -> %d\n", #ptr, (ptr), (rcRet)))
#define kDbgAssertRC(rc)                        kDbgAssertMsg((rc) == 0, ("%s = %d\n", #rc, (rc)))
#define kDbgAssertRCReturn(rc, rcRet)           kDbgAssertMsgReturn((rc) == 0, ("%s = %d -> %d\n", #rc, (rc), (rcRet)), (rcRet))
#define kDbgAssertRCReturnVoid(rc)              kDbgAssertMsgReturnVoid((rc) == 0, ("%s = %d -> %d\n", #rc, (rc), (rcRet)))
#define kDbgAssertFailed()                      kDbgAssert(0)
#define kDbgAssertFailedReturn(rcRet)           kDbgAssertReturn(0, (rcRet))
#define kDbgAssertFailedReturnVoid()            kDbgAssertReturnVoid(0)
#define kDbgAssertMsgFailed(msg)                kDbgAssertMsg(0, msg)
#define kDbgAssertMsgFailedReturn(msg, rcRet)   kDbgAssertMsgReturn(0, msg, (rcRet))
#define kDbgAssertMsgFailedReturnVoid(msg)      kDbgAssertMsgReturnVoid(0, msg)
/** @} */

/** Return / crash validation of a reader argument. */
#define KDBGMOD_VALIDATE_EX(pDbgMod, rc) \
    do  { \
        kDbgAssertPtrReturn((pDbgMod), (rc)); \
        kDbgAssertReturn((pDbgMod)->u32Magic == KDBGMOD_MAGIC, (rc)); \
        kDbgAssertReturn((pDbgMod)->pOps != NULL, (rc)); \
    } while (0)

/** Return / crash validation of a reader argument. */
#define KDBGMOD_VALIDATE(pDbgMod) \
    do  { \
        kDbgAssertPtrReturn((pDbgMod), KERR_INVALID_POINTER); \
        kDbgAssertReturn((pDbgMod)->u32Magic == KDBGMOD_MAGIC, KERR_INVALID_HANDLE); \
        kDbgAssertReturn((pDbgMod)->pOps != NULL, KERR_INVALID_HANDLE); \
    } while (0)

/** Return / crash validation of a reader argument. */
#define KDBGMOD_VALIDATE_VOID(pDbgMod) \
    do  { \
        kDbgAssertPtrReturnVoid((pDbgMod)); \
        kDbgAssertReturnVoid((pDbgMod)->u32Magic == KDBGMOD_MAGIC); \
        kDbgAssertReturnVoid((pDbgMod)->pOps != NULL); \
    } while (0)


#ifdef __cplusplus
extern "C" {
#endif

/** @name Built-in Debug Module Readers
 * @{ */
extern KDBGMODOPS const g_kDbgModWinDbgHelpOpen;
extern KDBGMODOPS const g_kDbgModLdr;
extern KDBGMODOPS const g_kDbgModCv8;
extern KDBGMODOPS const g_kDbgModDwarf;
extern KDBGMODOPS const g_kDbgModHll;
extern KDBGMODOPS const g_kDbgModStabs;
extern KDBGMODOPS const g_kDbgModSym;
extern KDBGMODOPS const g_kDbgModMapILink;
extern KDBGMODOPS const g_kDbgModMapMSLink;
extern KDBGMODOPS const g_kDbgModMapNm;
extern KDBGMODOPS const g_kDbgModMapWLink;
/** @} */

#ifdef __cplusplus
}
#endif

/** @} */

#endif

