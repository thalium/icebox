/* $Id: prfcorepre.cpp.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kProfiler Mark 2 - Core Pre-Code Template.
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


/** @def KPRF_OFF2PTR
 * Internal helper for converting a offset to a pointer.
 * @internal
 */
#define KPRF_OFF2PTR(TypePrefix, TypeName, off, pHdr) \
    ( (KPRF_TYPE(TypePrefix, TypeName)) ((off) + (KPRF_TYPE(,UPTR))pHdr) )

/** @def KPRF_PTR2OFF
 * Internal helper for converting a pointer to a offset.
 * @internal
 */
#define KPRF_PTR2OFF(ptr, pHdr) \
    ( (KPRF_TYPE(,UPTR))(ptr) - (KPRF_TYPE(,UPTR))(pHdr) )

/** @def KPRF_ALIGN
 * The usual align macro.
 * @internal
 */
#define KPRF_ALIGN(n, align) ( ((n) + ( (align) - 1)) & ~((align) - 1) )

/** @def KPRF_SETMIN_ALIGN
 * Ensures a minimum and aligned value.
 * @internal
 */
#define KPRF_SETMIN_ALIGN(n, min, align) \
    do { \
        if ((n) < (min)) \
            (n) = (min); \
        else { \
            const KU32 u32 = ((n) + ( (align) - 1)) & ~((align) - 1); \
            if (u32 >= (n)) \
                (n) = u32; \
        } \
    } while (0)

/** @def KPRF_OFFSETOF
 * My usual extended OFFSETOF macro, except this returns KU32 and mangles the type name.
 * @internal
 */
#define KPRF_OFFSETOF(kPrfType, Member) ( (KU32)(KUPTR)&((KPRF_TYPE(P,kPrfType))0)->Member )

/** @def PRF_SIZEOF
 * Size of a kPrf type.
 * @internal
 */
#define KPRF_SIZEOF(kPrfType)       sizeof(KPRF_TYPE(,kPrfType))


/** @def KPRF_NOW
 * Gets the current timestamp.
 */
#ifndef KPRF_NOW
# error "KPRF_NOW isn't defined!"
#endif

/** @def KRPF_IS_ACTIVE
 * Checks if profiling is activated or not.
 * The idea is to use some global variable for disabling and enabling
 * profiling in order to deal with init/term issues.
 */
#ifndef KPRF_IS_ACTIVE
# define KPRF_IS_ACTIVE()           1
#endif

/** @def KPRF_GET_HDR
 * Gets the pointer to the profiler data header.
 */
#ifndef KPRF_GET_HDR
# error "KPRF_GET_HDR isn't defined!"
#endif

/** @def KPRF_GET_THREADID
 * Gets native thread id. This must be unique.
 */
#ifndef KPRF_GET_THREADID
# error "KPRF_GET_THREADID isn't defined!"
#endif

/** @def KPRF_SET_THREAD
 * Sets the pointer to the current thread so we can get to it
 * without doing a linear search by thread id.
 */
#ifndef KPRF_SET_THREAD
# error "KPRF_SET_THREAD isn't defined!"
#endif

/** @def KPRF_GET_THREAD
 * Gets the pointer to the current thread as set by KPRF_SET_THREAD.
 */
#ifndef KPRF_GET_THREAD
# error "KPRF_GET_THREAD isn't defined!"
#endif

/** @def KPRF_MODSEGS_LOCK
 * Lock the module segment for updating.
 */
#ifndef KPRF_MODSEGS_LOCK
# define KPRF_MODSEGS_LOCK()            do { } while (0)
#endif

/** @def KPRF_MODSEGS_UNLOCK
 * Unlock the module segments.
 */
#ifndef KPRF_MODSEGS_UNLOCK
# define KPRF_MODSEGS_UNLOCK()          do { } while (0)
#endif

/** @def KPRF_THREADS_LOCK
 * Lock the threads for updating.
 */
#ifndef KPRF_THREADS_LOCK
# define KPRF_THREADS_LOCK()            do { } while (0)
#endif

/** @def KPRF_THREADS_UNLOCK
 * Unlock the threads.
 */
#ifndef KPRF_THREADS_UNLOCK
# define KPRF_THREADS_UNLOCK()          do { } while (0)
#endif

/** @def KPRF_FUNCS_READ_LOCK
 * Lock the functions for reading.
 */
#ifndef KPRF_FUNCS_READ_LOCK
# define KPRF_FUNCS_READ_LOCK()         do { } while (0)
#endif

/** @def KPRF_FUNCS_READ_UNLOCK
 * Releases a read lock on the functions.
 */
#ifndef KPRF_FUNCS_READ_UNLOCK
# define KPRF_FUNCS_READ_UNLOCK()       do { } while (0)
#endif

/** @def KPRF_FUNCS_WRITE_LOCK
 * Lock the functions for updating.
 */
#ifndef KPRF_FUNCS_WRITE_LOCK
# define KPRF_FUNCS_WRITE_LOCK()        do { } while (0)
#endif

/** @def KPRF_FUNCS_WRITE_UNLOCK
 * Releases a write lock on the functions.
 */
#ifndef KPRF_FUNCS_WRITE_UNLOCK
# define KPRF_FUNCS_WRITE_UNLOCK()      do { } while (0)
#endif


/** @def KPRF_ATOMIC_SET32
 * Atomically set a 32-bit value.
 */
#ifndef KPRF_ATOMIC_SET32
# define KPRF_ATOMIC_SET32(pu32, u32)   do { *(pu32) = (u32); } while (0)
#endif

/** @def KPRF_ATOMIC_SET64
 * Atomically (well, in a safe way) adds to a 64-bit value.
 */
#ifndef KPRF_ATOMIC_ADD64
# define KPRF_ATOMIC_ADD64(pu64, u64)   do { *(pu64) += (u64); } while (0)
#endif

/** @def KPRF_ATOMIC_SET64
 * Atomically (well, in a safe way) increments a 64-bit value.
 */
#ifndef KPRF_ATOMIC_INC64
# define KPRF_ATOMIC_INC64(pu64)        KPRF_ATOMIC_ADD64(pu64, 1)
#endif

