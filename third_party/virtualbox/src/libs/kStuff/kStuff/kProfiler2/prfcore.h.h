/* $Id: prfcore.h.h 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kProfiler Mark 2 - Core Header Template.
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


/** @def KPRF_NAME
 * Mixed case name macro.
 */
#ifndef KPRF_NAME
# define KPRF_NAME(Name)        Name
#endif

/** @def KPRF_TYPE
 * Upper case type name macro.
 */
#ifndef KPRF_TYPE
# define KPRF_TYPE(Prefix,Name) Prefix##Name
#endif

/** @type KPRF_DECL_FUNC
 * The calling convention used.
 */
#ifndef KPRF_DECL_FUNC
# define KPRF_DECL_FUNC(type, name) type name
#endif

/** @def KPRF_BITS
 * The bitsize of the format.
 */
#ifndef KPRF_BITS
# define KPRF_BITS  32
#endif

/** @type UPTR
 * The basic unsigned interger pointer type.
 */
/** @type IPTR
 * The basic signed interger pointer type.
 */
#if KPRF_BITS == 16
typedef KU16    KPRF_TYPE(,UPTR);
typedef KI16    KPRF_TYPE(,IPTR);
#elif KPRF_BITS == 32
typedef KU32    KPRF_TYPE(,UPTR);
typedef KI32    KPRF_TYPE(,IPTR);
#elif KPRF_BITS == 64
typedef KU64    KPRF_TYPE(,UPTR);
typedef KI64    KPRF_TYPE(,IPTR);
#else
# error "KPRF_BITS has an invalid value. Supported values are 16, 32 and 64."
#endif
/** @type KPRF_TYPE(P,UPTR)
 * Pointer to the basic pointer type.
 */
typedef KPRF_TYPE(,UPTR) *KPRF_TYPE(P,UPTR);


/**
 * Various constants.
 */
enum KPRF_TYPE(,CONSTANTS)
{
    /** Magic for the profiler header. (Unix Epoc) */
    KPRF_TYPE(,HDR_MAGIC) = 0x19700101
};


/**
 * The profile data header.
 */
typedef struct KPRF_TYPE(,HDR)
{
    /** [0] The magic number for file data. (KPRF_TYPE(,HDR_MAGIC)) */
    KU32                    u32Magic;
    /** [4] KPRF_BITS. */
    KU32                    cFormatBits;
    /** [8] The base address which all pointers should be relative to. */
    KPRF_TYPE(,UPTR)        uBasePtr;
#if KPRF_BITS <= 16
    /** [a] Reserved. */
    KU16                    u16Reserved;
#endif
#if KPRF_BITS <= 32
    /** [c] Reserved. */
    KU32                    u32Reserved;
#endif
    /** [10] The size of this data set. */
    KU32                    cb;
    /** [10] The allocated data set size. */
    KU32                    cbAllocated;

    /** [18] The max number of functions the function table can hold. */
    KU32                    cMaxFunctions;
    /** [1c] The current number of functions in the function table. */
    KU32                    cFunctions;
    /** [20] The offset of the function table (relative to this header). */
    KU32                    offFunctions;
    /** [24] The size of a function entry. */
    KU32                    cbFunction;

    /** [28] The max number of bytes the module segments can occupy. */
    KU32                    cbMaxModSegs;
    /** [2c] The current size of the module segment records. */
    KU32                    cbModSegs;
    /** [30] The offset of the module segment records (relative to this header). */
    KU32                    offModSegs;

    /** [34] The max number of threads the thread table can contain. */
    KU32                    cMaxThreads;
    /** [38] The current number of threads in the thread table. */
    KU32                    cThreads;
    /** [3c] The offset of the thread table (relative to this header). */
    KU32                    offThreads;
    /** [40] The size of a thread entry. */
    KU32                    cbThread;

    /** [44] The max number of stacks the stack table can contain. */
    KU32                    cMaxStacks;
    /** [48] The max number of stacks.
     * Unlike the other members, the stacks can be reused. It follows that
     * this count doesn't specify the number of used slots from the start. */
    KU32                    cStacks;
    /** [4c] The offset of the thread table (relative to this header).
     * This is usually 0 in a stored data set. */
    KU32                    offStacks;
    /** [50] The size of a stack. */
    KU32                    cbStack;
    /** [54] The maxium stack depth. */
    KU32                    cMaxStackFrames;

    /** [58] The process commandline.
     * Might not always apply is will be 0 in those cases. This is normally written
     * where the stacks used to be.
     */
    KU32                    offCommandLine;
    /** [5c] The length of the command line. (excludes the terminator). */
    KU32                    cchCommandLine;

    /** [60]  The function lookup table (it contains indexes).
     * This is sorted by address so that a binary search can be performed.
     * Access to this table is managed externally, but generally a read/write lock is employed. */
    KU32                    aiFunctions[1];
} KPRF_TYPE(,HDR);
/** Pointer to a profiler data header. */
typedef KPRF_TYPE(,HDR) *KPRF_TYPE(P,HDR);
/** Pointer to a const profiler data header. */
typedef const KPRF_TYPE(,HDR) *KPRF_TYPE(PC,HDR);


/**
 * Time statistics.
 */
typedef struct KPRF_TYPE(,TIMESTAT)      /** @todo bad names and descriptions! */
{
    /** The minimum period */
    KU64 volatile           MinTicks;
    /** The maximum period */
    KU64 volatile           MaxTicks;
    /** The sum of all periods. */
    KU64 volatile           SumTicks;
} KPRF_TYPE(,TIMESTAT);
/** Pointer to time statistics. */
typedef KPRF_TYPE(,TIMESTAT) *KPRF_TYPE(P,TIMESTAT);
/** Pointer to const time statistics. */
typedef const KPRF_TYPE(,TIMESTAT) *KPRF_TYPE(PC,TIMESTAT);


/**
 * A Module Segment.
 */
typedef struct KPRF_TYPE(,MODSEG)
{
    /** The address of the segment. (relative address) */
    KPRF_TYPE(,UPTR)        uBasePtr;
    /** The size of the segment minus one (so the entire address space can be covered). */
    KPRF_TYPE(,UPTR)        cbSegmentMinusOne;
    /** The segment number. (0 based) */
    KU32                    iSegment;
    /** Flag indicating whether this segment is loaded or not.
     * (A 16-bit value was choosen out of convenience, all that's stored is 0 or 1 anyway.) */
    KU16                    fLoaded;
    /** The length of the path.
     * This is used to calculate the length of the record: offsetof(MODSEG, szPath) + cchPath + 1 */
    KU16                    cchPath;
    /** The module name. */
    char                    szPath[1];
} KPRF_TYPE(,MODSEG);
/** Pointer to a module segment. */
typedef KPRF_TYPE(,MODSEG) *KPRF_TYPE(P,MODSEG);
/** Pointer to a const module segment. */
typedef const KPRF_TYPE(,MODSEG) *KPRF_TYPE(PC,MODSEG);


/**
 * The profiler data for a function.
 */
typedef struct KPRF_TYPE(,FUNC)
{
    /** The entry address of the function. (relative address)
     * This is the return address of the entry hook (_mcount, _penter, _ProfileHook32, ...). */
    KPRF_TYPE(,UPTR)        uEntryPtr;
    /** Offset (relative to the profiler header) of the module segment to which this function belongs. */
    KU32                    offModSeg;

    /** The number times on the stack. */
    KU64 volatile           cOnStack;
    /** The number of calls made from this function. */
    KU64 volatile           cCalls;

    /** Time on stack. */
    KPRF_TYPE(,TIMESTAT)    OnStack;
    /** Time on top of the stack, i.e. executing. */
    KPRF_TYPE(,TIMESTAT)    OnTopOfStack;

    /** @todo recursion */

} KPRF_TYPE(,FUNC);
/** Pointer to the profiler data for a function. */
typedef KPRF_TYPE(,FUNC)       *KPRF_TYPE(P,FUNC);
/** Pointer to the const profiler data for a function. */
typedef const KPRF_TYPE(,FUNC) *KPRF_TYPE(PC,FUNC);


/**
 * Stack frame.
 */
typedef struct KPRF_TYPE(,FRAME)
{
    /** The accumulated overhead.
     * Over head is accumulated by the parent frame when a child is poped off the stack. */
    KU64                    OverheadTicks;
    /** The current (top of stack) overhead. */
    KU64                    CurOverheadTicks;
    /** The accumulated sleep ticks.
     * It's possible to notify the profiler that the thread is being put into a wait/sleep/yield
     * state. The time spent sleeping is transfered to the parent frame when poping of a child one. */
    KU64                    SleepTicks;
    /** The start of the on-stack period. */
    KU64                    OnStackStart;
    /** The accumulated time on top (excludes overhead (sleep doesn't apply here obviously)). */
    KU64                    OnTopOfStackTicks;
    /** The start of the current on-top-of-stack period.
     * This is also to mark the start of a sleeping period, the ResumeThread function will always
     * treat it as the start of the suspend period. */
    KU64                    OnTopOfStackStart;
    /** The number of calls made from this stack frame. */
    KU64                    cCalls;
    /** Stack address of this frame.
     * This is used to detect throw and longjmp, and is also used to deal with overflow. (relative address) */
    KPRF_TYPE(,UPTR)        uFramePtr;
    /** Offset (relative to the profiler header) to the function record.
     * This is 0 if we're out of function space. */
    KU32                    offFunction;
} KPRF_TYPE(,FRAME);
/** Pointer to a stack frame. */
typedef KPRF_TYPE(,FRAME) *KPRF_TYPE(P,FRAME);
/** Pointer to a const stack frame. */
typedef const KPRF_TYPE(,FRAME) *KPRF_TYPE(PC,FRAME);


/**
 * Stack.
 */
typedef struct KPRF_TYPE(,STACK)
{
    /** The offset (relative to the profiler header) of the thread owning the stack.
     * This is zero if not in use, and non-zero if in use. */
    KU32                        offThread;
    /** The number of active stack frames. */
    KU32                        cFrames;
    /** The stack frames.
     * The actual size of this array is specified in the header. */
    KPRF_TYPE(,FRAME)           aFrames[1];
} KPRF_TYPE(,STACK);
/** Pointer to a stack. */
typedef KPRF_TYPE(,STACK) *KPRF_TYPE(P,STACK);
/** Pointer to a const stack. */
typedef const KPRF_TYPE(,STACK) *KPRF_TYPE(PC,STACK);


/**
 * The thread state.
 */
typedef enum KPRF_TYPE(,THREADSTATE)
{
    /** The thread hasn't been used yet. */
    KPRF_TYPE(,THREADSTATE_UNUSED) = 0,
    /** The thread is activly being profiled.
     * A thread is added in the suspended state and then activated when
     * starting to execute the first function.
     */
    KPRF_TYPE(,THREADSTATE_ACTIVE),
    /** The thread is currently suspended from profiling.
     * Upon entering profiler code the thread is suspended, it's reactivated
     * upon normal return.
     */
    KPRF_TYPE(,THREADSTATE_SUSPENDED),
    /** The thread is currently suspended due of stack overflow.
     * When we overflow the stack frame array, the thread enter the overflow state. In this
     * state nothing is profiled but we keep looking for the exit of the top frame. */
    KPRF_TYPE(,THREADSTATE_OVERFLOWED),
    /** The thread is terminated.
     * When we received a thread termination notification the thread is unwinded, statistics
     * updated and the state changed to terminated. A terminated thread cannot be revivied. */
    KPRF_TYPE(,THREADSTATE_TERMINATED),

    /** Ensure 32-bit size. */
    KPRF_TYPE(,THREADSTATE_32BIT_HACK) = 0x7fffffff
} KPRF_TYPE(,THREADSTATE);


/**
 * Thread statistics and stack.
 */
typedef struct KPRF_TYPE(,THREAD)
{
    /** The native thread id. */
    KU64                        ThreadId;
    /** The thread name. (optional) */
    char                        szName[32];
    /** The thread current thread state. */
    KPRF_TYPE(,THREADSTATE)     enmState;
    /** Alignment. */
    KPRF_TYPE(,THREADSTATE)     Reserved0;
    /** The base pointer of the thread stack. (relative address) */
    KPRF_TYPE(,UPTR)            uStackBasePtr;
    /** The maximum depth of the thread stack (bytes). */
    KPRF_TYPE(,UPTR)            cbMaxStack;
    /** The number of calls done by this thread. */
    KU64                        cCalls;
    /** The number of times the stack overflowed. */
    KU64                        cOverflows;
    /** The number of times stack entries has been rejected because of a stack switch. */
    KU64                        cStackSwitchRejects;
    /** The number of times the stack has been unwinded more than one frame. */
    KU64                        cUnwinds;

    /** The profiled ticks. (This does not include sleep or overhead ticks.)
     * This is the accumulated on-stack values for the final stack frames. */
    KU64                        ProfiledTicks;
    /** The accumulated overhead of this thread. */
    KU64                        OverheadTicks;
    /** The accumulated sleep ticks for this thread.
     * See KPRF_TYPE(,FRAME)::SleepTicks for details. */
    KU64                        SleepTicks;

    /** The offset of the stack. */
    KU32                        offStack;
} KPRF_TYPE(,THREAD);
/** Pointer to a thread. */
typedef KPRF_TYPE(,THREAD) *KPRF_TYPE(P,THREAD);
/** Pointer to a const thread. */
typedef const KPRF_TYPE(,THREAD) *KPRF_TYPE(PC,THREAD);


