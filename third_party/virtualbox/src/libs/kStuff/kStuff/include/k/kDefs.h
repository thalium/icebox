/* $Id: kDefs.h 105 2017-11-21 23:55:40Z bird $ */
/** @file
 * kTypes - Defines and Macros.
 */

/*
 * Copyright (c) 2006-2017 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
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

#ifndef ___k_kDefs_h___
#define ___k_kDefs_h___

/** @defgroup grp_kDefs  kDefs - Defines and Macros
 * @{ */

/** @name Operative System Identifiers.
 * These are the value that the K_OS \#define can take.
 * @{
 */
/** Unknown OS. */
#define K_OS_UNKNOWN    0
/** Darwin - aka Mac OS X. */
#define K_OS_DARWIN     1
/** DragonFly BSD. */
#define K_OS_DRAGONFLY  2
/** FreeBSD. */
#define K_OS_FREEBSD    3
/** GNU/Hurd. */
#define K_OS_GNU_HURD   4
/** GNU/kFreeBSD. */
#define K_OS_GNU_KFBSD  5
/** GNU/kNetBSD or GNU/NetBSD or whatever the decide to call it. */
#define K_OS_GNU_KNBSD  6
/** Haiku. */
#define K_OS_HAIKU      7
/** Linux. */
#define K_OS_LINUX      8
/** NetBSD. */
#define K_OS_NETBSD     9
/** NT (native). */
#define K_OS_NT         10
/** OpenBSD*/
#define K_OS_OPENBSD    11
/** OS/2 */
#define K_OS_OS2        12
/** Solaris */
#define K_OS_SOLARIS    13
/** Windows. */
#define K_OS_WINDOWS    14
/** The max K_OS_* value (exclusive). */
#define K_OS_MAX        15
/** @} */

/** @def K_OS
 * Indicates which OS we're targetting. It's a \#define with is
 * assigned one of the K_OS_* defines above.
 *
 * So to test if we're on FreeBSD do the following:
 * @code
 *  #if K_OS == K_OS_FREEBSD
 *  some_funky_freebsd_specific_stuff();
 *  #endif
 * @endcode
 */
#ifndef K_OS
# if defined(__APPLE__)
#  define K_OS      K_OS_DARWIN
# elif defined(__DragonFly__)
#  define K_OS      K_OS_DRAGONFLY
# elif defined(__FreeBSD__)
#  define K_OS      K_OS_FREEBSD
# elif defined(__FreeBSD_kernel__)
#  define K_OS      K_OS_GNU_KFBSD
# elif defined(__gnu_hurd__)
#  define K_OS      K_OS_GNU_HURD
# elif defined(__gnu_linux__)
#  define K_OS      K_OS_LINUX
# elif defined(__NetBSD__) /*??*/
#  define K_OS      K_OS_NETBSD
# elif defined(__NetBSD_kernel__)
#  define K_OS      K_OS_GNU_KNBSD
# elif defined(__OpenBSD__) /*??*/
#  define K_OS      K_OS_OPENBSD
# elif defined(__OS2__)
#  define K_OS      K_OS_OS2
# elif defined(__sun__) || defined(__SunOS__) || defined(__sun) || defined(__SunOS)
#  define K_OS      K_OS_SOLARIS
# elif defined(_WIN32) || defined(_WIN64)
#  define K_OS      K_OS_WINDOWS
# elif defined(__haiku__) || defined(__HAIKU__)
#  define K_OS      K_OS_HAIKU
# else
#  error "Port Me"
# endif
#endif
#if K_OS < K_OS_UNKNOWN || K_OS >= K_OS_MAX
# error "Invalid K_OS value."
#endif



/** @name   Architecture bit width.
 * @{ */
#define K_ARCH_BIT_8            0x0100  /**< 8-bit */
#define K_ARCH_BIT_16           0x0200  /**< 16-bit */
#define K_ARCH_BIT_32           0x0400  /**< 32-bit */
#define K_ARCH_BIT_64           0x0800  /**< 64-bit */
#define K_ARCH_BIT_128          0x1000  /**< 128-bit */
#define K_ARCH_BIT_MASK         0x1f00  /**< The bit mask. */
#define K_ARCH_BIT_SHIFT        5       /**< Shift count for producing the width in bits. */
#define K_ARCH_BYTE_SHIFT       8       /**< Shift count for producing the width in bytes. */
/** @} */

/** @name   Architecture Endianness.
 * @{ */
#define K_ARCH_END_LITTLE       0x2000  /**< Little-endian. */
#define K_ARCH_END_BIG          0x4000  /**< Big-endian. */
#define K_ARCH_END_BI           0x6000  /**< Bi-endian, can be switched. */
#define K_ARCH_END_MASK         0x6000  /**< The endian mask. */
#define K_ARCH_END_SHIFT        13      /**< Shift count for converting between this K_ENDIAN_*. */
/** @} */

/** @name Architecture Identifiers.
 * These are the value that the K_ARCH \#define can take.
 *@{ */
/** Unknown CPU architecture. */
#define K_ARCH_UNKNOWN          ( 0 )
/** Clone or Intel 16-bit x86. */
#define K_ARCH_X86_16           ( 1 | K_ARCH_BIT_16 | K_ARCH_END_LITTLE)
/** Clone or Intel 32-bit x86. */
#define K_ARCH_X86_32           ( 1 | K_ARCH_BIT_32 | K_ARCH_END_LITTLE)
/** AMD64 (including clones). */
#define K_ARCH_AMD64            ( 2 | K_ARCH_BIT_64 | K_ARCH_END_LITTLE)
/** Itanic (64-bit). */
#define K_ARCH_IA64             ( 3 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** ALPHA (64-bit). */
#define K_ARCH_ALPHA            ( 4 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** ALPHA limited to 32-bit. */
#define K_ARCH_ALPHA_32         ( 4 | K_ARCH_BIT_32 | K_ARCH_END_BI)
/** 32-bit ARM. */
#define K_ARCH_ARM_32           ( 5 | K_ARCH_BIT_32 | K_ARCH_END_BI)
/** 64-bit ARM. */
#define K_ARCH_ARM_64           ( 5 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** Motorola 68000 (32-bit). */
#define K_ARCH_M68K             ( 6 | K_ARCH_BIT_32 | K_ARCH_END_BIG)
/** 32-bit MIPS. */
#define K_ARCH_MIPS_32          ( 7 | K_ARCH_BIT_32 | K_ARCH_END_BI)
/** 64-bit MIPS. */
#define K_ARCH_MIPS_64          ( 7 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** 32-bit PA-RISC. */
#define K_ARCH_PARISC_32        ( 8 | K_ARCH_BIT_32 | K_ARCH_END_BI)
/** 64-bit PA-RISC. */
#define K_ARCH_PARISC_64        ( 8 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** 32-bit PowerPC. */
#define K_ARCH_POWERPC_32       ( 9 | K_ARCH_BIT_32 | K_ARCH_END_BI)
/** 64-bit PowerPC. */
#define K_ARCH_POWERPC_64       ( 9 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** 32(31)-bit S390. */
#define K_ARCH_S390_32          (10 | K_ARCH_BIT_32 | K_ARCH_END_BIG)
/** 64-bit S390. */
#define K_ARCH_S390_64          (10 | K_ARCH_BIT_64 | K_ARCH_END_BIG)
/** 32-bit SuperH. */
#define K_ARCH_SH_32            (11 | K_ARCH_BIT_32 | K_ARCH_END_BI)
/** 64-bit SuperH. */
#define K_ARCH_SH_64            (11 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** 32-bit SPARC. */
#define K_ARCH_SPARC_32         (12 | K_ARCH_BIT_32 | K_ARCH_END_BIG)
/** 64-bit SPARC. */
#define K_ARCH_SPARC_64         (12 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** The end of the valid architecture values (exclusive). */
#define K_ARCH_MAX              (12+1)
/** @} */


/** @def K_ARCH
 * The value of this \#define indicates which architecture we're targetting.
 */
#ifndef K_ARCH
  /* detection based on compiler defines. */
# if defined(__amd64__) || defined(__x86_64__) || defined(__AMD64__) || defined(_M_X64) || defined(__amd64)
#  define K_ARCH    K_ARCH_AMD64
# elif defined(__i386__) || defined(__x86__) || defined(__X86__) || defined(_M_IX86) || defined(__i386)
#  define K_ARCH    K_ARCH_X86_32
# elif defined(__ia64__) || defined(__IA64__) || defined(_M_IA64)
#  define K_ARCH    K_ARCH_IA64
# elif defined(__alpha__)
#  define K_ARCH    K_ARCH_ALPHA
# elif defined(__arm__) || defined(__arm32__)
#  define K_ARCH    K_ARCH_ARM_32
# elif defined(__aarch64__) || defined(__arm64__)
#  define K_ARCH    K_ARCH_ARM_64
# elif defined(__hppa__) && defined(__LP64__)
#  define K_ARCH    K_ARCH_PARISC_64
# elif defined(__hppa__)
#  define K_ARCH    K_ARCH_PARISC_32
# elif defined(__m68k__)
#  define K_ARCH    K_ARCH_M68K
# elif defined(__mips64)
#  define K_ARCH    K_ARCH_MIPS_64
# elif defined(__mips__)
#  define K_ARCH    K_ARCH_MIPS_32
# elif defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__)
#  define K_ARCH    K_ARCH_POWERPC_64
# elif defined(__powerpc__) || defined(__ppc__) || defined(__PPC__)
#  define K_ARCH    K_ARCH_POWERPC_32
# elif defined(__sparcv9__) || defined(__sparcv9)
#  define K_ARCH    K_ARCH_SPARC_64
# elif defined(__sparc__) || defined(__sparc)
#  define K_ARCH    K_ARCH_SPARC_32
# elif defined(__s390x__)
#  define K_ARCH    K_ARCH_S390_64
# elif defined(__s390__)
#  define K_ARCH    K_ARCH_S390_32
# elif defined(__sh__)
#  if !defined(__SH5__)
#   define K_ARCH    K_ARCH_SH_32
#  else
#   if __SH5__ == 64
#    define K_ARCH   K_ARCH_SH_64
#   else
#    define K_ARCH   K_ARCH_SH_32
#   endif
#  endif
# else
#  error "Port Me"
# endif
#else
  /* validate the user specified value. */
# if (K_ARCH & K_ARCH_BIT_MASK) != K_ARCH_BIT_8 \
  && (K_ARCH & K_ARCH_BIT_MASK) != K_ARCH_BIT_16 \
  && (K_ARCH & K_ARCH_BIT_MASK) != K_ARCH_BIT_32 \
  && (K_ARCH & K_ARCH_BIT_MASK) != K_ARCH_BIT_64 \
  && (K_ARCH & K_ARCH_BIT_MASK) != K_ARCH_BIT_128
#  error "Invalid K_ARCH value (bit)"
# endif
# if (K_ARCH & K_ARCH_END_MASK) != K_ARCH_END_LITTLE \
  && (K_ARCH & K_ARCH_END_MASK) != K_ARCH_END_BIG \
  && (K_ARCH & K_ARCH_END_MASK) != K_ARCH_END_BI
#  error "Invalid K_ARCH value (endian)"
# endif
# if (K_ARCH & ~(K_ARCH_BIT_MASK | K_ARCH_BIT_END_MASK)) < K_ARCH_UNKNOWN \
  || (K_ARCH & ~(K_ARCH_BIT_MASK | K_ARCH_BIT_END_MASK)) >= K_ARCH_MAX
#  error "Invalid K_ARCH value"
# endif
#endif

/** @def K_ARCH_IS_VALID
 * Check if the architecture identifier is valid.
 * @param   arch            The K_ARCH_* define to examin.
 */
#define K_ARCH_IS_VALID(arch)   (   (   ((arch) & K_ARCH_BIT_MASK) == K_ARCH_BIT_8 \
                                     || ((arch) & K_ARCH_BIT_MASK) == K_ARCH_BIT_16 \
                                     || ((arch) & K_ARCH_BIT_MASK) == K_ARCH_BIT_32 \
                                     || ((arch) & K_ARCH_BIT_MASK) == K_ARCH_BIT_64 \
                                     || ((arch) & K_ARCH_BIT_MASK) == K_ARCH_BIT_128) \
                                 && \
                                    (   ((arch) & K_ARCH_END_MASK) == K_ARCH_END_LITTLE \
                                     || ((arch) & K_ARCH_END_MASK) == K_ARCH_END_BIG \
                                     || ((arch) & K_ARCH_END_MASK) == K_ARCH_END_BI) \
                                 && \
                                    (   ((arch) & ~(K_ARCH_BIT_MASK | K_ARCH_END_MASK)) >= K_ARCH_UNKNOWN \
                                     && ((arch) & ~(K_ARCH_BIT_MASK | K_ARCH_END_MASK)) < K_ARCH_MAX) \
                                )

/** @def K_ARCH_BITS_EX
 * Determin the architure byte width of the specified architecture.
 * @param   arch            The K_ARCH_* define to examin.
 */
#define K_ARCH_BITS_EX(arch)    ( ((arch) & K_ARCH_BIT_MASK) >> K_ARCH_BIT_SHIFT )

/** @def K_ARCH_BYTES_EX
 * Determin the architure byte width of the specified architecture.
 * @param   arch            The K_ARCH_* define to examin.
 */
#define K_ARCH_BYTES_EX(arch)   ( ((arch) & K_ARCH_BIT_MASK) >> K_ARCH_BYTE_SHIFT )

/** @def K_ARCH_ENDIAN_EX
 * Determin the K_ENDIAN value for the specified architecture.
 * @param   arch            The K_ARCH_* define to examin.
 */
#define K_ARCH_ENDIAN_EX(arch)  ( ((arch) & K_ARCH_END_MASK) >> K_ARCH_END_SHIFT )

/** @def K_ARCH_BITS
 * Determin the target architure bit width.
 */
#define K_ARCH_BITS             K_ARCH_BITS_EX(K_ARCH)

/** @def K_ARCH_BYTES
 * Determin the target architure byte width.
 */
#define K_ARCH_BYTES            K_ARCH_BYTES_EX(K_ARCH)

/** @def K_ARCH_ENDIAN
 * Determin the target K_ENDIAN value.
 */
#define K_ARCH_ENDIAN           K_ARCH_ENDIAN_EX(K_ARCH)



/** @name Endianness Identifiers.
 * These are the value that the K_ENDIAN \#define can take.
 * @{ */
#define K_ENDIAN_LITTLE         1       /**< Little-endian. */
#define K_ENDIAN_BIG            2       /**< Big-endian. */
#define K_ENDIAN_BI             3       /**< Bi-endian, can be switched. Only used with K_ARCH. */
/** @} */

/** @def K_ENDIAN
 * The value of this \#define indicates the target endianness.
 *
 * @remark  It's necessary to define this (or add the necessary deduction here)
 *          on bi-endian architectures.
 */
#ifndef K_ENDIAN
  /* use K_ARCH if possible. */
# if K_ARCH_ENDIAN != K_ENDIAN_BI
#  define K_ENDIAN K_ARCH_ENDIAN
# elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
#  if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define K_ENDIAN K_ARCH_LITTLE
#  elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  define K_ENDIAN K_ARCH_BIG
#  else
#   error "Port Me or define K_ENDIAN."
#  endif
# else
#  error "Port Me or define K_ENDIAN."
# endif
#else
  /* validate the user defined value. */
# if K_ENDIAN != K_ENDIAN_LITTLE
  && K_ENDIAN != K_ENDIAN_BIG
#  error "K_ENDIAN must either be defined as K_ENDIAN_LITTLE or as K_ENDIAN_BIG."
# endif
#endif

/** @name Endian Conversion
 * @{ */

/** @def K_E2E_U16
 * Convert the endian of an unsigned 16-bit value. */
# define K_E2E_U16(u16)         ( (KU16) (((u16) >> 8) | ((u16) << 8)) )
/** @def K_E2E_U32
 * Convert the endian of an unsigned 32-bit value. */
# define K_E2E_U32(u32)         (   ( ((u32) & KU32_C(0xff000000)) >> 24 ) \
                                  | ( ((u32) & KU32_C(0x00ff0000)) >>  8 ) \
                                  | ( ((u32) & KU32_C(0x0000ff00)) <<  8 ) \
                                  | ( ((u32) & KU32_C(0x000000ff)) << 24 ) \
                                )
/** @def K_E2E_U64
 * Convert the endian of an unsigned 64-bit value. */
# define K_E2E_U64(u64)         (   ( ((u64) & KU64_C(0xff00000000000000)) >> 56 ) \
                                  | ( ((u64) & KU64_C(0x00ff000000000000)) >> 40 ) \
                                  | ( ((u64) & KU64_C(0x0000ff0000000000)) >> 24 ) \
                                  | ( ((u64) & KU64_C(0x000000ff00000000)) >>  8 ) \
                                  | ( ((u64) & KU64_C(0x00000000ff000000)) <<  8 ) \
                                  | ( ((u64) & KU64_C(0x0000000000ff0000)) << 24 ) \
                                  | ( ((u64) & KU64_C(0x000000000000ff00)) << 40 ) \
                                  | ( ((u64) & KU64_C(0x00000000000000ff)) << 56 ) \
                                )

/** @def K_LE2H_U16
 * Unsigned 16-bit little-endian to host endian. */
/** @def K_LE2H_U32
 * Unsigned 32-bit little-endian to host endian. */
/** @def K_LE2H_U64
 * Unsigned 64-bit little-endian to host endian. */
/** @def K_BE2H_U16
 * Unsigned 16-bit big-endian to host endian. */
/** @def K_BE2H_U32
 * Unsigned 32-bit big-endian to host endian. */
/** @def K_BE2H_U64
 * Unsigned 64-bit big-endian to host endian. */
#if K_ENDIAN == K_ENDIAN_LITTLE
# define K_LE2H_U16(u16)        ((KU16)(u16))
# define K_LE2H_U32(u32)        ((KU32)(u32))
# define K_LE2H_U64(u64)        ((KU64)(u32))
# define K_BE2H_U16(u16)        K_E2E_U16(u16)
# define K_BE2H_U32(u32)        K_E2E_U32(u32)
# define K_BE2H_U64(u64)        K_E2E_U64(u64)
#else
# define K_LE2H_U16(u16)        K_E2E_U16(u16)
# define K_LE2H_U32(u32)        K_E2E_U32(u32)
# define K_LE2H_U64(u64)        K_E2E_U64(u64)
# define K_BE2H_U16(u16)        ((KU16)(u16))
# define K_BE2H_U32(u32)        ((KU32)(u32))
# define K_BE2H_U64(u64)        ((KU64)(u32))
#endif



/** @def K_INLINE
 * How to say 'inline' in both C and C++ dialects.
 * @param   type        The return type.
 */
#ifdef __cplusplus
# if defined(__GNUC__)
#  define K_INLINE              static inline
# else
#  define K_INLINE              inline
# endif
#else
# if defined(__GNUC__)
#  define K_INLINE              static __inline__
# elif defined(_MSC_VER)
#  define K_INLINE              static __inline
# else
#  error "Port Me"
# endif
#endif

/** @def K_EXPORT
 * What to put in front of an exported function.
 */
#if K_OS == K_OS_OS2 || K_OS == K_OS_WINDOWS
# define K_EXPORT               __declspec(dllexport)
#else
# define K_EXPORT
#endif

/** @def K_IMPORT
 * What to put in front of an imported function.
 */
#if K_OS == K_OS_OS2 || K_OS == K_OS_WINDOWS
# define K_IMPORT               __declspec(dllimport)
#else
# define K_IMPORT               extern
#endif

/** @def K_DECL_EXPORT
 * Declare an exported function.
 * @param type      The return type.
 */
#define K_DECL_EXPORT(type)     K_EXPORT type

/** @def K_DECL_IMPORT
 * Declare an import function.
 * @param type      The return type.
 */
#define K_DECL_IMPORT(type)     K_IMPORT type

/** @def K_DECL_INLINE
 * Declare an inline function.
 * @param type      The return type.
 * @remark  Don't use on (class) methods.
 */
#define K_DECL_INLINE(type)     K_INLINE type


/** Get the minimum of two values. */
#define K_MIN(a, b)             ( (a) <= (b) ? (a) : (b) )
/** Get the maximum of two values. */
#define K_MAX(a, b)             ( (a) >= (b) ? (a) : (b) )
/** Calculate the offset of a structure member. */
#define K_OFFSETOF(strct, memb) ( (KSIZE)( &((strct *)0)->memb ) )
/** Align a size_t value. */
#define K_ALIGN_Z(val, align)   ( ((val) + ((align) - 1)) & ~(KSIZE)((align) - 1) )
/** Align a void * value. */
#define K_ALIGN_P(pv, align)    ( (void *)( ((KUPTR)(pv) + ((align) - 1)) & ~(KUPTR)((align) - 1) ) )
/** Number of elements in an array. */
#define K_ELEMENTS(a)           ( sizeof(a) / sizeof((a)[0]) )
/** Checks if the specified pointer is a valid address or not. */
#define K_VALID_PTR(ptr)        ( (KUPTR)(ptr) + 0x1000U >= 0x2000U )
/** Makes a 32-bit bit mask. */
#define K_BIT32(bit)            ( KU32_C(1) << (bit))
/** Makes a 64-bit bit mask. */
#define K_BIT64(bit)            ( KU64_C(1) << (bit))
/** Shuts up unused parameter and unused variable warnings. */
#define K_NOREF(var)            ( (void)(var) )


/** @name Parameter validation macros
 * @{ */

/** Return/Crash validation of a string argument. */
#define K_VALIDATE_STRING(str) \
    do { \
        if (!K_VALID_PTR(str)) \
            return KERR_INVALID_POINTER; \
        kHlpStrLen(str); \
    } while (0)

/** Return/Crash validation of an optional string argument. */
#define K_VALIDATE_OPTIONAL_STRING(str) \
    do { \
        if (str) \
            K_VALIDATE_STRING(str); \
    } while (0)

/** Return/Crash validation of an output buffer. */
#define K_VALIDATE_BUFFER(buf, cb) \
    do { \
        if (!K_VALID_PTR(buf)) \
            return KERR_INVALID_POINTER; \
        if ((cb) != 0) \
        { \
            KU8             __b; \
            KU8 volatile   *__pb = (KU8 volatile *)(buf); \
            KSIZE           __cbPage1 = 0x1000 - ((KUPTR)(__pb) & 0xfff); /* ASSUMES page size! */ \
            __b = *__pb; *__pb = 0xff; *__pb = __b; \
            if ((cb) > __cbPage1) \
            { \
                KSIZE __cb = (cb) - __cbPage1; \
                __pb -= __cbPage1; \
                for (;;) \
                { \
                    __b = *__pb; *__pb = 0xff; *__pb = __b; \
                    if (__cb < 0x1000) \
                        break; \
                    __pb += 0x1000; \
                    __cb -= 0x1000; \
                } \
            } \
        } \
        else \
            return KERR_INVALID_PARAMETER; \
    } while (0)

/** Return/Crash validation of an optional output buffer. */
#define K_VALIDATE_OPTIONAL_BUFFER(buf, cb) \
    do { \
        if ((buf) && (cb) != 0) \
            K_VALIDATE_BUFFER(buf, cb); \
    } while (0)

/** Return validation of an enum argument. */
#define K_VALIDATE_ENUM(arg, enumname) \
    do { \
        if ((arg) <= enumname##_INVALID || (arg) >= enumname##_END) \
            return KERR_INVALID_PARAMETER; \
    } while (0)

/** Return validation of a flags argument. */
#define K_VALIDATE_FLAGS(arg, AllowedMask) \
    do { \
        if ((arg) & ~(AllowedMask)) \
            return KERR_INVALID_PARAMETER; \
    } while (0)

/** @} */

/** @def NULL
 * The nil pointer value. */
#ifndef NULL
# ifdef __cplusplus
#  define NULL          0
# else
#  define NULL          ((void *)0)
# endif
#endif

/** @} */

#endif

