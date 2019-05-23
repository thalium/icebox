/* $Id: kDbgBase.h 40 2010-02-02 16:02:15Z bird $ */
/** @file
 * kDbg - The Debug Info Reader, Base Definitions and Typedefs.
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

#ifndef ___kDbgBase_h___
#define ___kDbgBase_h___

#include <k/kDefs.h>
#include <k/kTypes.h>


/** @defgroup grp_kDbgBase  kDbgBase - Base Definitions And Typedefs
 * @{ */

/*
 * kDbg depend on size_t, [u]intNN_t, [u]intptr_t and some related constants.
 * If KDBG_ALREADY_INCLUDED_STD_TYPES or KCOMMON_ALREADY_INCLUDED_STD_TYPES
 * is defined, these has already been defined.
 */
#if !defined(KDBG_ALREADY_INCLUDED_STD_TYPES) && !defined(KCOMMON_ALREADY_INCLUDED_STD_TYPES)
# define KCOMMON_ALREADY_INCLUDED_STD_TYPES 1
# include <sys/types.h>
# include <stddef.h>
# ifdef _MSC_VER
   typedef signed char          int8_t;
   typedef unsigned char        uint8_t;
   typedef signed short         int16_t;
   typedef unsigned short       uint16_t;
   typedef signed int           int32_t;
   typedef unsigned int         uint32_t;
   typedef signed __int64       int64_t;
   typedef unsigned __int64     uint64_t;
   typedef int64_t              intmax_t;
   typedef uint64_t             uintmax_t;
#  define UINT8_C(c)            (c)
#  define UINT16_C(c)           (c)
#  define UINT32_C(c)           (c ## U)
#  define UINT64_C(c)           (c ## ULL)
#  define INT8_C(c)             (c)
#  define INT16_C(c)            (c)
#  define INT32_C(c)            (c)
#  define INT64_C(c)            (c ## LL)
#  define INT8_MIN              (INT8_C(-0x7f) - 1)
#  define INT16_MIN             (INT16_C(-0x7fff) - 1)
#  define INT32_MIN             (INT32_C(-0x7fffffff) - 1)
#  define INT64_MIN             (INT64_C(-0x7fffffffffffffff) - 1)
#  define INT8_MAX              INT8_C(0x7f)
#  define INT16_MAX             INT16_C(0x7fff)
#  define INT32_MAX             INT32_C(0x7fffffff)
#  define INT64_MAX             INT64_C(0x7fffffffffffffff)
#  define UINT8_MAX             UINT8_C(0xff)
#  define UINT16_MAX            UINT16_C(0xffff)
#  define UINT32_MAX            UINT32_C(0xffffffff)
#  define UINT64_MAX            UINT64_C(0xffffffffffffffff)
# else
#  include <stdint.h>
# endif
#endif /* !KDBG_ALREADY_INCLUDED_STD_TYPES && !KCOMMON_ALREADY_INCLUDED_STD_TYPES */


/** @def KDBG_CALL
 * The calling convention used by the kDbg functions. */
#if defined(_MSC_VER) || defined(__OS2__)
# define KDBG_CALL  __cdecl
#else
# define KDBG_CALL
#endif

#ifdef DOXYGEN_RUNNING
/** @def KDBG_BUILDING
 * Define KDBG_BUILDING to indicate that kDbg is being built.
 */
# define KDBG_BUILDING
/** @def KDBG_RESIDES_IN_DLL
 * Define KDBG_RESIDES_IN_DLL to indicate that kDbg resides in a DLL.
 */
# define KDBG_RESIDES_IN_DLL
#endif

/** @def KDBG_DECL
 * Macro for defining public functions. */
#if defined(KDBG_RESIDES_IN_DLL) \
 && (defined(_MSC_VER) || defined(__OS2__))
# ifdef KDBG_BUILDING
#  define KDBG_DECL(type) __declspec(dllexport) type
# else
#  define KDBG_DECL(type) __declspec(dllimport) type
# endif
#else
# define KDBG_DECL(type) type
#endif

/** @def KDBG_INLINE
 * Macro for defining an inline function. */
#ifdef __cplusplus
# if defined(__GNUC__)
#  define KDBG_INLINE(type) static inline type
# else
#  define KDBG_INLINE(type) inline type
# endif
#else
# if defined(__GNUC__)
#  define KDBG_INLINE(type) static __inline__ type
# elif defined(_MSC_VER)
#  define KDBG_INLINE(type) _inline type
# else
#  error "Port me"
# endif
#endif


/** The kDbg address type. */
typedef uint64_t KDBGADDR;
/** Pointer to a kLdr address. */
typedef KDBGADDR *PKDBGADDR;
/** Pointer to a const kLdr address. */
typedef const KDBGADDR *PCKDBGADDR;

/** NIL address. */
#define NIL_KDBGADDR    (~(uint64_t)0)

/** @def PRI_KDBGADDR
 * printf format type. */
#ifdef _MSC_VER
# define PRI_KDBGADDR    "I64x"
#else
# define PRI_KDBGADDR    "llx"
#endif


/** Get the minimum of two values. */
#define KDBG_MIN(a, b)              ((a) <= (b) ? (a) : (b))
/** Get the maximum of two values. */
#define KDBG_MAX(a, b)              ((a) >= (b) ? (a) : (b))
/** Calculate the offset of a structure member. */
#define KDBG_OFFSETOF(strct, memb)  ( (size_t)( &((strct *)0)->memb ) )
/** Align a size_t value. */
#define KDBG_ALIGN_Z(val, align)    ( ((val) + ((align) - 1)) & ~(size_t)((align) - 1) )
/** Align a void * value. */
#define KDBG_ALIGN_P(pv, align)     ( (void *)( ((uintptr_t)(pv) + ((align) - 1)) & ~(uintptr_t)((align) - 1) ) )
/** Align a size_t value. */
#define KDBG_ALIGN_ADDR(val, align) ( ((val) + ((align) - 1)) & ~(KDBGADDR)((align) - 1) )
/** Number of elements in an array. */
#define KDBG_ELEMENTS(a)            ( sizeof(a) / sizeof((a)[0]) )
/** @def KDBG_VALID_PTR
 * Checks if the specified pointer is a valid address or not. */
#define KDBG_VALID_PTR(ptr)         ( (uintptr_t)(ptr) + 0x1000U >= 0x2000U )


/** @def KDBG_LITTLE_ENDIAN
 * The kDbg build is for a little endian target. */
/** @def KDBG_BIG_ENDIAN
 * The kDbg build is for a big endian target. */
#if !defined(KDBG_LITTLE_ENDIAN) && !defined(KDBG_BIG_ENDIAN)
# define KDBG_LITTLE_ENDIAN
#endif
#ifdef DOXYGEN_RUNNING
# define KDBG_BIG_ENDIAN
#endif


/** @name Endian Conversion
 * @{ */

/** @def KDBG_E2E_U16
 * Convert the endian of an unsigned 16-bit value. */
# define KDBG_E2E_U16(u16)      ( (uint16_t) (((u16) >> 8) | ((u16) << 8)) )
/** @def KDBG_E2E_U32
 * Convert the endian of an unsigned 32-bit value. */
# define KDBG_E2E_U32(u32)      (   ( ((u32) & UINT32_C(0xff000000)) >> 24 ) \
                                  | ( ((u32) & UINT32_C(0x00ff0000)) >>  8 ) \
                                  | ( ((u32) & UINT32_C(0x0000ff00)) <<  8 ) \
                                  | ( ((u32) & UINT32_C(0x000000ff)) << 24 ) \
                                )
/** @def KDBG_E2E_U64
 * Convert the endian of an unsigned 64-bit value. */
# define KDBG_E2E_U64(u64)      (   ( ((u64) & UINT64_C(0xff00000000000000)) >> 56 ) \
                                  | ( ((u64) & UINT64_C(0x00ff000000000000)) >> 40 ) \
                                  | ( ((u64) & UINT64_C(0x0000ff0000000000)) >> 24 ) \
                                  | ( ((u64) & UINT64_C(0x000000ff00000000)) >>  8 ) \
                                  | ( ((u64) & UINT64_C(0x00000000ff000000)) <<  8 ) \
                                  | ( ((u64) & UINT64_C(0x0000000000ff0000)) << 24 ) \
                                  | ( ((u64) & UINT64_C(0x000000000000ff00)) << 40 ) \
                                  | ( ((u64) & UINT64_C(0x00000000000000ff)) << 56 ) \
                                )

/** @def KDBG_LE2H_U16
 * Unsigned 16-bit little-endian to host endian. */
/** @def KDBG_LE2H_U32
 * Unsigned 32-bit little-endian to host endian. */
/** @def KDBG_LE2H_U64
 * Unsigned 64-bit little-endian to host endian. */
/** @def KDBG_BE2H_U16
 * Unsigned 16-bit big-endian to host endian. */
/** @def KDBG_BE2H_U32
 * Unsigned 32-bit big-endian to host endian. */
/** @def KDBG_BE2H_U64
 * Unsigned 64-bit big-endian to host endian. */
#ifdef KDBG_LITTLE_ENDIAN
# define KDBG_LE2H_U16(u16)  ((uint16_t)(u16))
# define KDBG_LE2H_U32(u32)  ((uint32_t)(u32))
# define KDBG_LE2H_U64(u64)  ((uint32_t)(u32))
# define KDBG_BE2H_U16(u16)  KDBG_E2E_U16(u16)
# define KDBG_BE2H_U32(u32)  KDBG_E2E_U32(u32)
# define KDBG_BE2H_U64(u64)  KDBG_E2E_U64(u64)
#elif defined(KDBG_BIG_ENDIAN)
# define KDBG_LE2H_U16(u16)  KDBG_E2E_U16(u16)
# define KDBG_LE2H_U32(u32)  KDBG_E2E_U32(u32)
# define KDBG_LE2H_U32(u64)  KDBG_E2E_U64(u64)
# define KDBG_BE2H_U16(u16)  ((uint16_t)(u16))
# define KDBG_BE2H_U32(u32)  ((uint32_t)(u32))
# define KDBG_BE2H_U64(u64)  ((uint32_t)(u32))
#else
# error "KDBG_BIG_ENDIAN or KDBG_LITTLE_ENDIAN is supposed to be defined."
#endif

/** @} */

/** @} */

#endif

