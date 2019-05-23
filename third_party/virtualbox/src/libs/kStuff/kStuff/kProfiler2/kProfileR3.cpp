/* $Id: kProfileR3.cpp 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kProfiler Mark 2 - The Ring-3 Implementation.
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kDefs.h>
#if K_OS == K_OS_WINDOWS
# include <windows.h>
# include <psapi.h>
# include <malloc.h>
# if _MSC_VER >= 1400
#  include <intrin.h>
#  define HAVE_INTRIN
# endif

#elif K_OS == K_OS_LINUX || K_OS == K_OS_FREEBSD
# define KPRF_USE_PTHREAD
# include <pthread.h>
# include <stdint.h>
# define KPRF_USE_MMAN
# include <sys/mman.h>
# include <sys/fcntl.h>
# include <unistd.h>
# include <stdlib.h>
# ifndef O_BINARY
#  define O_BINARY 0
# endif

#elif K_OS == K_OS_OS2
# define INCL_BASE
# include <os2.h>
# include <stdint.h>
# include <sys/fmutex.h>

#else
# error "not ported to this OS..."
#endif

#include <k/kDefs.h>
#include <k/kTypes.h>

/*
 * Instantiate the header.
 */
#define KPRF_NAME(Suffix)               KPrf##Suffix
#define KPRF_TYPE(Prefix,Suffix)        Prefix##KPRF##Suffix
#if K_OS == K_OS_WINDOWS || K_OS == K_OS_OS2
# define KPRF_DECL_FUNC(type, name)     extern "C"  __declspec(dllexport) type __cdecl KPRF_NAME(name)
#else
# define KPRF_DECL_FUNC(type, name)     extern "C" type KPRF_NAME(name)
#endif
#if 1
# ifdef __GNUC__
#  define KPRF_ASSERT(expr) do { if (!(expr)) { __asm__ __volatile__("int3\n\tnop\n\t");} } while (0)
# else
#  define KPRF_ASSERT(expr) do { if (!(expr)) { __debugbreak(); } } while (0)
# endif
#else
# define KPRF_ASSERT(expr) do { } while (0)
#endif

#include "prfcore.h.h"



/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** Mutex lock type. */
#if defined(KPRF_USE_PTHREAD)
typedef pthread_mutex_t     KPRF_TYPE(,MUTEX);
#elif K_OS == K_OS_WINDOWS
typedef CRITICAL_SECTION    KPRF_TYPE(,MUTEX);
#elif K_OS == K_OS_OS2
typedef struct _fmutex      KPRF_TYPE(,MUTEX);
#endif
/** Pointer to a mutex lock. */
typedef KPRF_TYPE(,MUTEX)  *KPRF_TYPE(P,MUTEX);


#if defined(KPRF_USE_PTHREAD)
/** Read/Write lock type. */
typedef pthread_rwlock_t    KPRF_TYPE(,RWLOCK);
#elif K_OS == K_OS_WINDOWS || K_OS == K_OS_OS2
/** Read/Write lock state. */
typedef enum KPRF_TYPE(,RWLOCKSTATE)
{
    RWLOCK_STATE_UNINITIALIZED = 0,
    RWLOCK_STATE_SHARED,
    RWLOCK_STATE_LOCKING,
    RWLOCK_STATE_EXCLUSIVE,
    RWLOCK_STATE_32BIT_HACK = 0x7fffffff
} KPRF_TYPE(,RWLOCKSTATE);
/** Update the state. */
#define KPRF_RWLOCK_SETSTATE(pRWLock, enmNewState) \
    kPrfAtomicSet32((volatile KU32 *)&(pRWLock)->enmState, (KU32)(enmNewState))

/** Read/Write lock type. */
typedef struct KPRF_TYPE(,RWLOCK)
{
    /** This mutex serialize the access and updating of the members
     * of this structure. */
    KPRF_TYPE(,MUTEX)       Mutex;
    /** The current number of readers. */
    KU32                    cReaders;
    /** The number of readers waiting. */
    KU32                    cReadersWaiting;
    /** The current number of waiting writers. */
    KU32                    cWritersWaiting;
# if K_OS == K_OS_WINDOWS
    /** The handle of the event object on which the waiting readers block. (manual reset). */
    HANDLE                  hevReaders;
    /** The handle of the event object on which the waiting writers block. (manual reset). */
    HANDLE                  hevWriters;
# elif K_OS == K_OS_OS2
    /** The handle of the event semaphore on which the waiting readers block. */
    HEV                     hevReaders;
    /** The handle of the event semaphore on which the waiting writers block. */
    HEV                     hevWriters;
# endif
    /** The current state of the read-write lock. */
    KPRF_TYPE(,RWLOCKSTATE) enmState;
} KPRF_TYPE(,RWLOCK);
#endif
/** Pointer to a Read/Write lock. */
typedef KPRF_TYPE(,RWLOCK) *KPRF_TYPE(P,RWLOCK);



/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** The TLS index / key. */
#if K_OS == K_OS_WINDOWS
static DWORD                g_dwThreadTLS = TLS_OUT_OF_INDEXES;

#elif defined(KPRF_USE_PTHREAD)
static pthread_key_t        g_ThreadKey = (pthread_key_t)-1;

#elif K_OS == K_OS_OS2
static KPRF_TYPE(P,THREAD) *g_ppThread = NULL;

#else
# error "Not ported to your OS - or you're missing the OS define(s)."
#endif

/** Pointer to the profiler header. */
static KPRF_TYPE(P,HDR)     g_pHdr = NULL;
#define KPRF_GET_HDR()                  g_pHdr

/** Whether the profiler is enabled or not. */
static bool                 g_fEnabled = false;
#define KPRF_IS_ACTIVE()                g_fEnabled


/** The mutex protecting the threads in g_pHdr. */
static KPRF_TYPE(,MUTEX)   g_ThreadsMutex;

/** The mutex protecting the module segments in g_pHdr. */
static KPRF_TYPE(,MUTEX)   g_ModSegsMutex;

/** The read-write lock protecting the functions in g_pHdr. */
static KPRF_TYPE(,RWLOCK)  g_FunctionsRWLock;



/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static KPRF_TYPE(P,THREAD) kPrfGetThreadAutoReg(void);
#ifdef KPRF_USE_PTHREAD
static void kPrfPThreadKeyDtor(void *pvThread);
#endif


/**
 * Gets the pointer to the profiler data for the current thread.
 *
 * This implementation automatically adds unknown threads.
 *
 * @returns Pointer to the profiler thread data.
 * @returns NULL if we're out of thread space.
 */
static inline KPRF_TYPE(P,THREAD) kPrfGetThread(void)
{
    KPRF_TYPE(P,THREAD) pThread;

/* Win32/64 */
#if K_OS == K_OS_WINDOWS
    pThread = (KPRF_TYPE(P,THREAD))TlsGetValue(g_dwThreadTLS);

/* Posix Threads */
#elif defined(KPRF_USE_PTHREAD)
    pThread = (KPRF_TYPE(P,THREAD))pthread_getspecific(g_ThreadKey);

#elif K_OS == K_OS_OS2
    pThread = *g_ppThread;

#else
# error not implemented
#endif
    if (!pThread)
        pThread = kPrfGetThreadAutoReg();
    return pThread;
}
#define KPRF_GET_THREAD()               kPrfGetThread()


/**
 * The the ID of the current thread.
 *
 * @returns The thread id.
 */
static inline KUPTR kPrfGetThreadId(void)
{
/* Win32/64 */
#if K_OS == K_OS_WINDOWS
    KUPTR ThreadId = (KUPTR)GetCurrentThreadId();

/* Posix Threads */
#elif defined(KPRF_USE_PTHREAD)
    KUPTR ThreadId = (KUPTR)pthread_self();

#elif K_OS == K_OS_OS2
    PTIB pTib;
    PPIB pPib;
    DosGetInfoBlocks(&pTib, &pPib);
    ThreadId = pTib->tib_ptib2->tib2_ultid;

#else
# error not implemented
#endif

    return ThreadId;
}
#define KPRF_GET_THREADID()             kPrfGetThreadId()


/**
 * The the ID of the current process.
 *
 * @returns The process id.
 */
static inline KUPTR kPrfGetProcessId(void)
{
/* Win32/64 */
#if K_OS == K_OS_WINDOWS
    KUPTR ThreadId = (KUPTR)GetProcessId(GetCurrentProcess());

#elif K_OS == K_OS_OS2
    PTIB pTib;
    PPIB pPib;
    DosGetInfoBlocks(&pTib, &pPib);
    ThreadId = pPib->pib_pid;

#else
    KUPTR ThreadId = (KUPTR)getpid();
#endif

    return ThreadId;
}
#define KPRF_GET_PROCESSID()            kPrfGetProcessId()


/**
 * Sets the pointer to the profiler data for the current thread.
 *
 * We require fast access to the profiler thread data, so we store
 * it in a TLS (thread local storage) item/key where the implementation
 * allows that.
 *
 * @param   pThread         The pointer to the profiler thread data for the current thread.
 */
static inline void kPrfSetThread(KPRF_TYPE(P,THREAD) pThread)
{
/* Win32/64 */
#if K_OS == K_OS_WINDOWS
    BOOL fRc = TlsSetValue(g_dwThreadTLS, pThread);

/* Posix Threads */
#elif defined(KPRF_USE_PTHREAD)
    int rc = pthread_setspecific(g_ThreadKey, pThread);

#elif K_OS == K_OS_OS2
    *g_ppThread = pThread;

#else
# error not implemented
#endif
}
#define KPRF_SET_THREAD(pThread)        kPrfSetThread(pThread)


/**
 * Get the now timestamp.
 * This must correspond to what the assembly code are doing.
 */
static inline KU64 kPrfNow(void)
{
#if defined(HAVE_INTRIN)
    return __rdtsc();
# else
    union
    {
        KU64 u64;
        struct
        {
            KU32 u32Lo;
            KU32 u32Hi;
        } s;
    } u;
# if defined(__GNUC__)
    __asm__ __volatile__ ("rdtsc\n\t" : "=a" (u.s.u32Lo), "=d" (u.s.u32Hi));
# else
    __asm
    {
        rdtsc
        mov     [u.s.u32Lo], eax
        mov     [u.s.u32Hi], edx
    }

# endif
    return u.u64;
#endif
}
#define KPRF_NOW()                      kPrfNow()


/**
 * Atomically set a 32-bit value.
 */
static inline void kPrfAtomicSet32(volatile KU32 *pu32, const KU32 u32)
{
#if defined(HAVE_INTRIN)
    _InterlockedExchange((long volatile *)pu32, (const long)u32);

#elif defined(__GNUC__)
    __asm__ __volatile__("xchgl %0, %1\n\t"
                         : "=m" (*pu32)
                         : "r" (u32));

#elif _MSC_VER
    __asm
    {
        mov     edx, [pu32]
        mov     eax, [u32]
        xchg    [edx], eax
    }

#else
    *pu32 = u32;
#endif
}
#define KPRF_ATOMIC_SET32(a,b)          kPrfAtomicSet32(a, b)



/**
 * Atomically set a 64-bit value.
 */
static inline void kPrfAtomicSet64(volatile KU64 *pu64, KU64 u64)
{
#if defined(HAVE_INTRIN) && KPRF_BITS == 64
    _InterlockedExchange64((KI64 *)pu64, (const KI64)u64);

#elif defined(__GNUC__) && KPRF_BITS == 64
    __asm__ __volatile__("xchgq %0, %1\n\t"
                         : "=m" (*pu64)
                         : "r" (u64));

#elif defined(__GNUC__) && KPRF_BITS == 32
    __asm__ __volatile__("1:\n\t"
                         "lock; cmpxchg8b %1\n\t"
                         "jnz 1b\n\t"
                         : "=A" (u64),
                           "=m" (*pu64)
                         : "0" (*pu64),
                           "b" ( (KU32)u64 ),
                           "c" ( (KU32)(u64 >> 32) ));

#elif _MSC_VER
    __asm
    {
        mov     ebx, dword ptr [u64]
        mov     ecx, dword ptr [u64 + 4]
        mov     esi, pu64
        mov     eax, dword ptr [esi]
        mov     edx, dword ptr [esi + 4]
    retry:
        lock cmpxchg8b [esi]
        jnz retry
    }
#else
    *pu64 = u64;
#endif
}
#define KPRF_ATOMIC_SET64(a,b)          kPrfAtomicSet64(a, b)


/**
 * Atomically add a 32-bit integer to another.
 */
static inline void kPrfAtomicAdd32(volatile KU32 *pu32, const KU32 u32)
{
#if defined(HAVE_INTRIN)
    _InterlockedExchangeAdd((volatile long *)pu32, (const long)u32);

#elif defined(__GNUC__)
    __asm__ __volatile__("lock; addl %0, %1\n\t"
                         : "=m" (*pu32)
                         : "r" (u32));

#elif _MSC_VER
    __asm
    {
        mov     edx, [pu32]
        mov     eax, dword ptr [u32]
        lock add [edx], eax
    }

#else
    *pu32 += u32;
#endif
}
#define KPRF_ATOMIC_ADD32(a,b)          kPrfAtomicAdd32(a, b)
#define KPRF_ATOMIC_INC32(a)            kPrfAtomicAdd32(a, 1);
#define KPRF_ATOMIC_DEC32(a)            kPrfAtomicAdd32(a, (KU32)-1);


/**
 * Atomically add a 64-bit integer to another.
 * Atomically isn't quite required, just a non-corruptive manner, assuming all updates are adds.
 */
static inline void kPrfAtomicAdd64(volatile KU64 *pu64, const KU64 u64)
{
#if defined(HAVE_INTRIN) && KPRF_BITS == 64
    _InterlockedExchangeAdd64((volatile KI64 *)pu64, (const KI64)u64);

#elif defined(__GNUC__) && KPRF_BITS == 64
    __asm__ __volatile__("lock; addq %0, %1\n\t"
                         : "=m" (*pu64)
                         : "r" (u64));

#elif defined(__GNUC__) && KPRF_BITS == 32
    __asm__ __volatile__("lock; addl %0, %2\n\t"
                         "lock; adcl %1, %3\n\t"
                         : "=m" (*(volatile KU32 *)pu64),
                           "=m" (*((volatile KU32 *)pu64 + 1))
                         : "r" ((KU32)u64),
                           "r" ((KU32)(u64 >> 32)));

#elif _MSC_VER
    __asm
    {
        mov     edx, [pu64]
        mov     eax, dword ptr [u64]
        mov     ecx, dword ptr [u64 + 4]
        lock add [edx], eax
        lock adc [edx + 4], ecx
    }

#else
    *pu64 += u64;
#endif
}
#define KPRF_ATOMIC_ADD64(a,b)          kPrfAtomicAdd64(a, b)
#define KPRF_ATOMIC_INC64(a)            kPrfAtomicAdd64(a, 1);


/**
 * Initializes a mutex.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pMutex      The mutex to init.
 */
static int kPrfMutexInit(KPRF_TYPE(P,MUTEX) pMutex)
{
#if defined(KPRF_USE_PTHREAD)
    if (!pthread_mutex_init(pMutex, NULL));
        return 0;
    return -1;

#elif K_OS == K_OS_WINDOWS
    InitializeCriticalSection(pMutex);
    return 0;

#elif K_OS == K_OS_OS2
    if (!_fmutex_create(pMutex, 0))
        return 0;
    return -1;
#endif
}

/**
 * Deletes a mutex.
 *
 * @param   pMutex      The mutex to delete.
 */
static void kPrfMutexDelete(KPRF_TYPE(P,MUTEX) pMutex)
{
#if defined(KPRF_USE_PTHREAD)
    pthread_mutex_destroy(pMutex);

#elif K_OS == K_OS_WINDOWS
    DeleteCriticalSection(pMutex);

#elif K_OS == K_OS_OS2
    _fmutex_close(pMutex);
#endif
}

/**
 * Locks a mutex.
 * @param   pMutex      The mutex lock.
 */
static inline void kPrfMutexAcquire(KPRF_TYPE(P,MUTEX) pMutex)
{
#if K_OS == K_OS_WINDOWS
    EnterCriticalSection(pMutex);

#elif defined(KPRF_USE_PTHREAD)
    pthread_mutex_lock(pMutex);

#elif K_OS == K_OS_OS2
    fmutex_request(pMutex);
#endif
}


/**
 * Unlocks a mutex.
 * @param   pMutex      The mutex lock.
 */
static inline void kPrfMutexRelease(KPRF_TYPE(P,MUTEX) pMutex)
{
#if K_OS == K_OS_WINDOWS
    LeaveCriticalSection(pMutex);

#elif defined(KPRF_USE_PTHREAD)
    pthread_mutex_lock(pMutex);

#elif K_OS == K_OS_OS2
    fmutex_request(pMutex);
#endif
}


#define KPRF_THREADS_LOCK()             kPrfMutexAcquire(&g_ThreadsMutex)
#define KPRF_THREADS_UNLOCK()           kPrfMutexRelease(&g_ThreadsMutex)

#define KPRF_MODSEGS_LOCK()             kPrfMutexAcquire(&g_ModSegsMutex)
#define KPRF_MODSEGS_UNLOCK()           kPrfMutexRelease(&g_ModSegsMutex)


/**
 * Initializes a read-write lock.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pRWLock     The read-write lock to initialize.
 */
static inline int kPrfRWLockInit(KPRF_TYPE(P,RWLOCK) pRWLock)
{
#if defined(KPRF_USE_PTHREAD)
    if (!pthread_rwlock_init(pRWLock, NULL))
        return 0;
    return -1;

#elif K_OS == K_OS_WINDOWS || K_OS == K_OS_OS2
    if (kPrfMutexInit(&pRWLock->Mutex))
        return -1;
    pRWLock->cReaders = 0;
    pRWLock->cReadersWaiting = 0;
    pRWLock->cWritersWaiting = 0;
    pRWLock->enmState = RWLOCK_STATE_SHARED;
# if K_OS == K_OS_WINDOWS
    pRWLock->hevReaders = CreateEvent(NULL, TRUE, TRUE, NULL);
    pRWLock->hevWriters = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (    pRWLock->hevReaders != INVALID_HANDLE_VALUE
        &&  pRWLock->hevWriters != INVALID_HANDLE_VALUE)
        return 0;
    CloseHandle(pRWLock->hevReaders);
    CloseHandle(pRWLock->hevWriters);

# elif K_OS == K_OS_OS2
    APIRET rc = DosCreateEventSem(NULL, &pRWLock->hevReaders, 0, TRUE);
    if (!rc)
    {
        rc = DosCreateEventSem(NULL, &pRWLock->hevWriters, 0, TRUE);
        if (!rc)
            return 0;
        pRWLock->hevWriters = NULLHANDLE;
        DosCloseEventSem(pRWLock->hevReaders);
    }
    pRWLock->hevReaders = NULLHANDLE;
# endif

    pRWLock->enmState = RWLOCK_STATE_UNINITIALIZED;
    kPrfMutexDelete(&pRWLock->Mutex);
    return -1;
#endif
}


/**
 * Deleters a read-write lock.
 *
 * @param   pRWLock     The read-write lock to delete.
 */
static inline void kPrfRWLockDelete(KPRF_TYPE(P,RWLOCK) pRWLock)
{
#if defined(KPRF_USE_PTHREAD)
    pthread_rwlock_destroy(pRWLock);

#elif K_OS == K_OS_WINDOWS || K_OS == K_OS_OS2
    if (pRWLock->enmState == RWLOCK_STATE_UNINITIALIZED)
        return;

    pRWLock->enmState = RWLOCK_STATE_UNINITIALIZED;
    kPrfMutexDelete(&pRWLock->Mutex);
    pRWLock->cReaders = 0;
    pRWLock->cReadersWaiting = 0;
    pRWLock->cWritersWaiting = 0;
# if K_OS == K_OS_WINDOWS
    CloseHandle(pRWLock->hevReaders);
    pRWLock->hevReaders = INVALID_HANDLE_VALUE;
    CloseHandle(pRWLock->hevWriters);
    pRWLock->hevWriters = INVALID_HANDLE_VALUE;

# elif K_OS == K_OS_OS2
    DosCloseEventSem(pRWLock->hevReaders);
    pRWLock->hevReaders = NULLHANDLE;
    DosCloseEventSem(pRWLock->hevWriters);
    pRWLock->hevWriters = NULLHANDLE;
# endif
#endif
}


/**
 * Acquires read access to the read-write lock.
 * @param   pRWLock     The read-write lock.
 */
static inline void kPrfRWLockAcquireRead(KPRF_TYPE(P,RWLOCK) pRWLock)
{
#if defined(KPRF_USE_PTHREAD)
    pthread_rwlock_rdlock(pRWLock);

#elif K_OS == K_OS_WINDOWS || K_OS == K_OS_OS2
    if (pRWLock->enmState == RWLOCK_STATE_UNINITIALIZED)
        return;

    kPrfMutexAcquire(&pRWLock->Mutex);
    if (pRWLock->enmState == RWLOCK_STATE_SHARED)
    {
        KPRF_ATOMIC_INC32(&pRWLock->cReaders);
        kPrfMutexRelease(&pRWLock->Mutex);
        return;
    }

    for (;;)
    {
        /* have to wait */
        KPRF_ATOMIC_INC32(&pRWLock->cReadersWaiting);
# if K_OS == K_OS_WINDOWS
        HANDLE hev = pRWLock->hevReaders;
        ResetEvent(hev);

# elif K_OS == K_OS_OS2
        HEV    hev = pRWLock->hevReaders;
        ULONG cIgnored;
        DosResetEventSem(hev, &cIgnored);

# endif
        kPrfMutexRelease(&pRWLock->Mutex);

# if K_OS == K_OS_WINDOWS
        switch (WaitForSingleObject(hev, INFINITE))
        {
            case WAIT_IO_COMPLETION:
            case WAIT_TIMEOUT:
            case WAIT_OBJECT_0:
                break;
            case WAIT_ABANDONED:
            default:
                return;
        }

# elif K_OS == K_OS_OS2
        switch (DosWaitEventSem(hev, SEM_INDEFINITE_WAIT))
        {
            case NO_ERROR:
            case ERROR_SEM_TIMEOUT:
            case ERROR_TIMEOUT:
            case ERROR_INTERRUPT:
                break;
            default:
                return;
        }
# endif

        kPrfMutexAcquire(&pRWLock->Mutex);
        if (pRWLock->enmState == RWLOCK_STATE_SHARED)
        {
            KPRF_ATOMIC_INC32(&pRWLock->cReaders);
            KPRF_ATOMIC_DEC32(&pRWLock->cReadersWaiting);
            kPrfMutexRelease(&pRWLock->Mutex);
            return;
        }
    }
#endif
}


/**
 * Releases read access to the read-write lock.
 * @param   pRWLock     The read-write lock.
 */
static inline void kPrfRWLockReleaseRead(KPRF_TYPE(P,RWLOCK) pRWLock)
{
#if defined(KPRF_USE_PTHREAD)
    pthread_rwlock_unlock(pRWLock);

#elif K_OS == K_OS_WINDOWS || K_OS == K_OS_OS2
    if (pRWLock->enmState == RWLOCK_STATE_UNINITIALIZED)
        return;

    /*
     * If we're still in the shared state, or if there
     * are more readers out there, or if there are no
     * waiting writers, all we have to do is decrement an leave.
     *
     * That's the most frequent, thing and should be fast.
     */
    kPrfMutexAcquire(&pRWLock->Mutex);
    KPRF_ATOMIC_DEC32(&pRWLock->cReaders);
    if (    pRWLock->enmState == RWLOCK_STATE_SHARED
        ||  pRWLock->cReaders
        ||  !pRWLock->cWritersWaiting)
    {
        kPrfMutexRelease(&pRWLock->Mutex);
        return;
    }

    /*
     * Wake up one (or more on OS/2) waiting writers.
     */
# if K_OS == K_OS_WINDOWS
    SetEvent(pRWLock->hevWriters);
# elif K_OS == K_OS_OS2
    DosPostEvent(pRWLock->hevwriters);
# endif
    kPrfMutexRelease(&pRWLock->Mutex);

#endif
}


/**
 * Acquires write access to the read-write lock.
 * @param   pRWLock     The read-write lock.
 */
static inline void kPrfRWLockAcquireWrite(KPRF_TYPE(P,RWLOCK) pRWLock)
{
#if defined(KPRF_USE_PTHREAD)
    pthread_rwlock_wrlock(pRWLock);

#elif K_OS == K_OS_WINDOWS || K_OS == K_OS_OS2
    if (pRWLock->enmState == RWLOCK_STATE_UNINITIALIZED)
        return;

    kPrfMutexAcquire(&pRWLock->Mutex);
    if (    !pRWLock->cReaders
        &&  (   pRWLock->enmState == RWLOCK_STATE_SHARED
             || pRWLock->enmState == RWLOCK_STATE_LOCKING)
        )
    {
        KPRF_RWLOCK_SETSTATE(pRWLock, RWLOCK_STATE_EXCLUSIVE);
        kPrfMutexRelease(&pRWLock->Mutex);
        return;
    }

    /*
     * We'll have to wait.
     */
    if (pRWLock->enmState == RWLOCK_STATE_SHARED)
        KPRF_RWLOCK_SETSTATE(pRWLock, RWLOCK_STATE_LOCKING);
    KPRF_ATOMIC_INC32(&pRWLock->cWritersWaiting);
    for (;;)
    {
# if K_OS == K_OS_WINDOWS
        HANDLE hev = pRWLock->hevWriters;
# elif K_OS == K_OS_OS2
        HEV    hev = pRWLock->hevWriters;
# endif
        kPrfMutexRelease(&pRWLock->Mutex);
# if K_OS == K_OS_WINDOWS
        switch (WaitForSingleObject(hev, INFINITE))
        {
            case WAIT_IO_COMPLETION:
            case WAIT_TIMEOUT:
            case WAIT_OBJECT_0:
                break;
            case WAIT_ABANDONED:
            default:
                KPRF_ATOMIC_DEC32(&pRWLock->cWritersWaiting);
                return;
        }

# elif K_OS == K_OS_OS2
        switch (DosWaitEventSem(hev, SEM_INDEFINITE_WAIT))
        {
            case NO_ERROR:
            case ERROR_SEM_TIMEOUT:
            case ERROR_TIMEOUT:
            case ERROR_INTERRUPT:
                break;
            default:
                KPRF_ATOMIC_DEC32(&pRWLock->cWritersWaiting);
                return;
        }
        ULONG cIgnored;
        DosResetEventSem(hev, &cIgnored);
# endif

        /*
         * Try acquire the lock.
         */
        kPrfMutexAcquire(&pRWLock->Mutex);
        if (    !pRWLock->cReaders
            &&  (   pRWLock->enmState == RWLOCK_STATE_SHARED
                 || pRWLock->enmState == RWLOCK_STATE_LOCKING)
            )
        {
            KPRF_RWLOCK_SETSTATE(pRWLock, RWLOCK_STATE_EXCLUSIVE);
            KPRF_ATOMIC_DEC32(&pRWLock->cWritersWaiting);
            kPrfMutexRelease(&pRWLock->Mutex);
            return;
        }
    }
#endif
}


/**
 * Releases write access to the read-write lock.
 * @param   pRWLock     The read-write lock.
 */
static inline void kPrfRWLockReleaseWrite(KPRF_TYPE(P,RWLOCK) pRWLock)
{
#if defined(KPRF_USE_PTHREAD)
    pthread_rwlock_unlock(pRWLock);

#elif K_OS == K_OS_WINDOWS || K_OS == K_OS_OS2
    if (pRWLock->enmState == RWLOCK_STATE_UNINITIALIZED)
        return;

    /*
     * The common thing is that there are noone waiting.
     * But, before that usual paranoia.
     */
    kPrfMutexAcquire(&pRWLock->Mutex);
    if (pRWLock->enmState != RWLOCK_STATE_EXCLUSIVE)
    {
        kPrfMutexRelease(&pRWLock->Mutex);
        return;
    }
    if (    !pRWLock->cReadersWaiting
        &&  !pRWLock->cWritersWaiting)
    {
        KPRF_RWLOCK_SETSTATE(pRWLock, RWLOCK_STATE_SHARED);
        kPrfMutexRelease(&pRWLock->Mutex);
        return;
    }

    /*
     * Someone is waiting, wake them up as we change the state.
     */
# if K_OS == K_OS_WINDOWS
    HANDLE hev = INVALID_HANDLE_VALUE;
# elif K_OS == K_OS_OS2
    HEV    hev = NULLHANDLE;
# endif

    if (pRWLock->cWritersWaiting)
    {
        KPRF_RWLOCK_SETSTATE(pRWLock, RWLOCK_STATE_LOCKING);
        hev = pRWLock->hevWriters;
    }
    else
    {
        KPRF_RWLOCK_SETSTATE(pRWLock, RWLOCK_STATE_SHARED);
        hev = pRWLock->hevReaders;
    }
# if K_OS == K_OS_WINDOWS
    SetEvent(hev);
# elif K_OS == K_OS_OS2
    DosPostEvent(pRWLock->hevwriters);
# endif
    kPrfMutexRelease(&pRWLock->Mutex);

#endif
}

#define KPRF_FUNCS_WRITE_LOCK()         kPrfRWLockAcquireWrite(&g_FunctionsRWLock)
#define KPRF_FUNCS_WRITE_UNLOCK()       kPrfRWLockReleaseWrite(&g_FunctionsRWLock)
#define KPRF_FUNCS_READ_LOCK()          kPrfRWLockAcquireRead(&g_FunctionsRWLock)
#define KPRF_FUNCS_READ_UNLOCK()        kPrfRWLockReleaseRead(&g_FunctionsRWLock)




/**
 * Finds the module segment which the address belongs to.
 *
 */
static int kPrfGetModSeg(KPRF_TYPE(,UPTR) uAddress, char *pszPath, KU32 cchPath, KU32 *piSegment,
                         KPRF_TYPE(P,UPTR) puBasePtr, KPRF_TYPE(P,UPTR) pcbSegmentMinusOne)
{
#if K_OS == K_OS_WINDOWS
    /*
     * Enumerate the module handles.
     */
    HANDLE      hProcess = GetCurrentProcess();
    DWORD       cbNeeded = 0;
    HMODULE     hModIgnored;
    if (    !EnumProcessModules(hProcess, &hModIgnored, sizeof(hModIgnored), &cbNeeded)
        &&  GetLastError() != ERROR_BUFFER_OVERFLOW) /** figure out what this actually returns */
        cbNeeded = 256 * sizeof(HMODULE);

    cbNeeded += sizeof(HMODULE) * 32;
    HMODULE *pahModules = (HMODULE *)alloca(cbNeeded);
    if (EnumProcessModules(hProcess, pahModules, cbNeeded, &cbNeeded))
    {
        const unsigned cModules = cbNeeded / sizeof(HMODULE);
        for (unsigned i = 0; i < cModules; i++)
        {
            __try
            {
                const KUPTR uImageBase = (KUPTR)pahModules[i];
                union
                {
                    KU8                *pu8;
                    PIMAGE_DOS_HEADER   pDos;
                    PIMAGE_NT_HEADERS   pNt;
                    PIMAGE_NT_HEADERS32 pNt32;
                    PIMAGE_NT_HEADERS64 pNt64;
                    KUPTR               u;
                } u;
                u.u = uImageBase;

                /* reject modules higher than the address. */
                if (uAddress < u.u)
                    continue;

                /* Skip past the MZ header */
                if (u.pDos->e_magic == IMAGE_DOS_SIGNATURE)
                    u.pu8 += u.pDos->e_lfanew;

                /* Ignore anything which isn't an NT header. */
                if (u.pNt->Signature != IMAGE_NT_SIGNATURE)
                    continue;

                /* Extract necessary info from the optional header (comes in 32-bit and 64-bit variations, we simplify a bit). */
                KU32                    cbImage;
                PIMAGE_SECTION_HEADER   paSHs;
                if (u.pNt->FileHeader.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32))
                {
                    paSHs = (PIMAGE_SECTION_HEADER)(u.pNt32 + 1);
                    cbImage = u.pNt32->OptionalHeader.SizeOfImage;
                }
                else if (u.pNt->FileHeader.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER64))
                {
                    paSHs = (PIMAGE_SECTION_HEADER)(u.pNt64 + 1);
                    cbImage = u.pNt64->OptionalHeader.SizeOfImage;
                }
                else
                    continue;

                /* Is our address within the image size */
                KUPTR uRVA = uAddress - (KUPTR)pahModules[i];
                if (uRVA >= cbImage)
                    continue;

                /*
                 * Iterate the section headers and figure which section we're in.
                 * (segment == section + 1)
                 */
                const KU32 cSHs = u.pNt->FileHeader.NumberOfSections;
                if (uRVA < paSHs[0].VirtualAddress)
                {
                    /* the implicit header section */
                    *puBasePtr          = uImageBase;
                    *pcbSegmentMinusOne = paSHs[0].VirtualAddress - 1;
                    *piSegment          = 0;
                }
                else
                {
                    KU32 iSH = 0;
                    for (;;)
                    {
                        if (iSH >= cSHs)
                        {
                            /* this shouldn't happen, but in case it does simply deal with it. */
                            *puBasePtr          = paSHs[iSH - 1].VirtualAddress + paSHs[iSH - 1].Misc.VirtualSize + uImageBase;
                            *pcbSegmentMinusOne = cbImage - *puBasePtr;
                            *piSegment          = iSH + 1;
                            break;
                        }
                        if (uRVA - paSHs[iSH].VirtualAddress < paSHs[iSH].Misc.VirtualSize)
                        {
                            *puBasePtr          = paSHs[iSH].VirtualAddress + uImageBase;
                            *pcbSegmentMinusOne = paSHs[iSH].Misc.VirtualSize;
                            *piSegment          = iSH + 1;
                            break;
                        }
                        iSH++;
                    }
                }

                /*
                 * Finally, get the module name.
                 * There are multiple ways, try them all before giving up.
                 */
                if (    !GetModuleFileNameEx(hProcess, pahModules[i], pszPath, cchPath)
                    &&  !GetModuleFileName(pahModules[i], pszPath, cchPath)
                    &&  !GetMappedFileName(hProcess, (PVOID)uAddress, pszPath, cchPath)
                    &&  !GetModuleBaseName(hProcess, pahModules[i], pszPath, cchPath))
                    *pszPath = '\0';
                return 0;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }
    }

#elif K_OS == K_OS_OS2
    /*
     * Just ask the loader.
     */
    ULONG   offObj = 0;
    ULONG   iObj = 0;
    HMODULE hmod = NULLHANDLE;
    APIRET rc = DosQueryModFromEIP(&hmod, &iObj, cchPath, pszPath, &offObj, uAddress);
    if (!rc)
    {
        *piSegment = iObj;
        *puBasePtr = uAddress - offObj;
        *pcbSegmentMinusOne = KPRF_ALIGN(offObj, 0x1000) - 1; /* minimum size */

        /*
         * Query the page attributes starting at the current page. The query will not enter
         * into the next object since PAG_BASE is requested.
         */
        ULONG cb     = ~0UL;
        ULONG fFlags = ~0UL;
        uAddress &= ~(KUPTR)0xfff;
        rc = DosQueryMem((PVOID)(uAddress, &cb, &fFlags);
        if (!rc)
        {
            *pcbSegmentMinusOne = (offObj & ~(KUPTR)0xfff) + KPRF_ALIGN(cb, 0x1000) - 1;
            if ((fFlags & PAG_BASE) && cb <= 0x1000) /* don't quite remember if PAG_BASE returns one page or not */
            {
                cb = ~0UL;
                fFlags = ~0UL;
                rc = DosQueryMem((PVOID)(uAddress + 0x1000), &cb, &fFlags);
                if (!rc & !(fFlags & (PAG_BASE | PAG_FREE)))
                    *pcbSegmentMinusOne += KPRF_ALIGN(cb, 0x1000);
            }
        }
        return 0;
    }

#endif
    /* The common fallback */
    *pszPath = '\0';
    *piSegment = 0;
    *puBasePtr = 0;
    *pcbSegmentMinusOne = ~(KPRF_TYPE(,UPTR))0;
    return -1;
}
#define KPRF_GET_MODSEG(uAddress, pszPath, cchPath, piSegment, puBasePtr, pcbSegmentMinusOne) \
                                        kPrfGetModSeg(uAddress, pszPath, cchPath, piSegment, puBasePtr, pcbSegmentMinusOne)




/*
 * Instantiate the implementation
 */
#include "prfcorepre.cpp.h"

#include "prfcoremodseg.cpp.h"
#include "prfcorefunction.cpp.h"
#include "prfcore.cpp.h"
#include "prfcoreinit.cpp.h"
#include "prfcoreterm.cpp.h"

#include "prfcorepost.cpp.h"





/**
 * Registers an unknown thread.
 *
 * @returns Pointer to the registered thread.
 */
static KPRF_TYPE(P,THREAD) kPrfGetThreadAutoReg(void)
{
    KUPTR uStackBasePtr;

#if 0
    /** @todo I'm sure Win32 has a way of obtaining the top and bottom of the stack, OS/2 did...
     * Some limit stuff in posix / ansi also comes to mind... */

#elif K_OS == K_OS_OS2
    PTIB pTib;
    PPIB pPib;
    DosGetInfoBlocks(&pTib, &pPib); /* never fails except if you give it bad input, thus 'Get' not 'Query'. */
    /* I never recall which of these is the right one... */
    uStackBasePtr = (KUPTR)pTib->tib_pstack < (KUPTR)pTib->tib_pstack_limit
        ? (KUPTR)pTib->tib_pstack
        : (KUPTR)pTib->tib_pstack_limit;

#else
    /* the default is top of the current stack page (assuming a page to be 4KB) */
    uStackBasePtr = (KUPTR)&uStackBasePtr;
    uStackBasePtr = (uStackBasePtr + 0xfff) & ~(KUPTR)0xfff;
#endif

    return KPRF_NAME(RegisterThread)(uStackBasePtr, "");
}


/**
 * Get a env.var. variable.
 *
 * @returns pszValue.
 * @param   pszVar      The variable name.
 * @param   pszValue    Where to store the value.
 * @param   cchValue    The size of the value buffer.
 * @param   pszDefault  The default value.
 */
static char *kPrfGetEnvString(const char *pszVar, char *pszValue, KU32 cchValue, const char *pszDefault)
{
#if K_OS == K_OS_WINDOWS
    if (GetEnvironmentVariable(pszVar, pszValue, cchValue))
        return pszValue;

#elif K_OS == K_OS_OS2
    PSZ pszValue;
    if (    !DosScanEnv((PCSZ)pszVar, &pszValue)
        &&  !*pszValue)
        pszDefault = pszValue;

#else
    const char *pszTmp = getenv(pszVar);
    if (pszTmp)
        pszDefault = pszTmp;

#endif

    /*
     * Copy the result into the buffer.
     */
    char *psz = pszValue;
    while (*pszDefault && cchValue-- > 1)
        *psz++ = *pszDefault++;
    *psz = '\0';

    return pszValue;
}


/**
 * The the value of an env.var.
 *
 * @returns The value of the env.var.
 * @returns The default if the value was not found.
 * @param   pszVar      The variable name.
 * @param   uDefault    The default value.
 */
static KU32 kPrfGetEnvValue(const char *pszVar, KU32 uDefault)
{
#if K_OS == K_OS_WINDOWS
    char szBuf[128];
    const char *pszValue = szBuf;
    if (!GetEnvironmentVariable(pszVar, szBuf, sizeof(szBuf)))
        pszValue = NULL;

#elif K_OS == K_OS_OS2
    PSZ pszValue;
    if (DosScanEnv((PCSZ)pszVar, &pszValue))
        pszValue = NULL;

#else
    const char *pszValue = getenv(pszVar);

#endif

    /*
     * Discard the obvious stuff.
     */
    if (!pszValue)
        return uDefault;
    while (*pszValue == ' ' || *pszValue == '\t')
        pszValue++;
    if (!*pszValue)
        return uDefault;

    /*
     * Interpret the value.
     */
    unsigned    uBase = 10;
    KU32        uValue = 0;
    const char *psz = pszValue;

    /* prefix - only hex */
    if (*psz == '0' && (psz[1] == 'x' || psz[1] == 'X'))
    {
        uBase = 16;
        psz += 2;
    }

    /* read the value */
    while (*psz)
    {
        unsigned char ch = (unsigned char)*psz;
        if (ch >= '0' && ch <= '9')
            ch -= '0';
        else if (   uBase > 10
                 && ch >= 'a' && ch <= 'f')
            ch -= 'a' + 10;
        else if (   uBase > 10
                 && ch >= 'a' && ch <= 'F')
            ch -= 'a' + 10;
        else
            break;
        uValue *= uBase;
        uValue += ch;
        psz++;
    }

    /* postfixes */
    switch (*psz)
    {
        case 'm':
        case 'M':
            uValue *= 1024*1024;
            break;

        case 'k':
        case 'K':
            uValue *= 1024;
            break;
    }

    /*
     * If the value is still 0, we return the default.
     */
    return uValue ? uValue : uDefault;
}


/**
 * Allocates memory.
 *
 * @returns Pointer to the allocated memory.
 * @returns NULL on failure.
 * @param   cb      The amount of memory (in bytes) to allocate.
 */
static void *kPrfAllocMem(KU32 cb)
{
#if K_OS == K_OS_WINDOWS
    void *pv = VirtualAlloc(NULL, cb, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

#elif defined(KPRF_USE_MMAN)
    void *pv = mmap(NULL, cb, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

#elif K_OS == K_OS_OS2
    void *pv;
# ifdef INCL_DOSEXAPIS
    if (DosAllocMemEx(&pv, cb, PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_COMMIT | OBJ_FORK))s
# else
    if (DosAllocMem(&pv, cb, PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_COMMIT))
# endif
        pvBuf = NULL;

#else
# error not implemented
#endif
    return pv;
}


/**
 * Frees memory.
 *
 * @param   pv      The memory to free.
 */
static void kPrfFreeMem(void *pv)
{
#if K_OS == K_OS_WINDOWS
    VirtualFree(pv, 0, MEM_RELEASE);

#elif defined(KPRF_USE_MMAN)
    munmap(pv, 0); /** @todo check if 0 is allowed here.. */

#elif K_OS == K_OS_OS2
# ifdef INCL_DOSEXAPIS
    DosFreeMemEx(&pv);
# else
    DosFreeMem(&pv);
# endif

#else
# error not implemented
#endif
}


/**
 * Writes a data buffer to a new file.
 *
 * Any existing file will be overwritten.
 *
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 *
 * @param   pszName     The name of the file.
 * @param   pvData      The data to write.
 * @param   cbData      The amount of data to write.
 */
static int kPrfWriteFile(const char *pszName, const void *pvData, KU32 cbData)
{
#if K_OS == K_OS_WINDOWS
    int rc = -1;
    HANDLE hFile = CreateFile(pszName,GENERIC_WRITE, FILE_SHARE_READ, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten;
        if (    WriteFile(hFile, pvData, cbData, &dwWritten, NULL)
            &&  dwWritten == cbData)
            rc = 0;
        CloseHandle(hFile);
    }
    return rc;

#elif K_OS == K_OS_OS2
    HFILE hFile;
    ULONG ulAction = 0;
    APIRET rc = DosOpen(pszName, &hFile, &ulAction, cbData, FILE_NORMAL,
                        OPEN_ACTION_REPLACE_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW,
                        OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYWRITE | OPEN_FLAGS_NOINHERIT | OPEN_FLAGS_SEQUENTIAL,
                        NULL);
    if (!rc)
    {
        ULONG cbWritten;
        rc = DosWrite(hFile, pvData, cbData, &cbWritten);
        if (!rc && cbWritten != cbData)
            rc = -1;
        DosClose(hFile);
    }
    return rc ? -1 : 0;

#else
    int rc = -1;
    int fd = open(pszName, O_WRONLY | O_CREAT | O_BINARY | O_TRUNC, 0666);
    if (fd >= 0)
    {
        if (write(fd, pvData, cbData) == cbData)
            rc = 0;
        close(fd);
    }
    return rc;

#endif
}



/**
 * Initializes and start the profiling.
 *
 * This should typically be called from some kind of module init
 * function, so we can start profiling upon/before entering main().
 *
 * @returns 0 on success
 * @returns -1 on failure.
 *
 */
int kPrfInitialize(void)
{
    /*
     * Only initialize once.
     */
    if (KPRF_GET_HDR())
        return 0;

    /*
     * Initial suggestions.
     */
    KU32    cbModSegs  = kPrfGetEnvValue("KPRF2_CBMODSEGS",  128*1024);
    KU32    cFunctions = kPrfGetEnvValue("KPRF2_CFUNCTIONS", 8192);
    KU32    cThreads   = kPrfGetEnvValue("KPRF2_CTHREADS",   256);
    KU32    cStacks    = kPrfGetEnvValue("KPRF2_CSTACKS",    48);
    KU32    cFrames    = kPrfGetEnvValue("KPRF2_CFRAMES",    448);
    KU32    fAffinity  = kPrfGetEnvValue("KPRF2_AFFINITY",   0);

    KU32    cb = KPRF_NAME(CalcSize)(cFunctions, cbModSegs, cThreads, cStacks, cFrames);

    /*
     * Allocate and initialize the data set.
     */
    void *pvBuf = kPrfAllocMem(cb);
    if (!pvBuf)
        return -1;

    KPRF_TYPE(P,HDR) pHdr = KPRF_NAME(Init)(pvBuf, cb, cFunctions, cbModSegs, cThreads, cStacks, cFrames);
    if (pHdr)
    {
        /*
         * Initialize semaphores.
         */
        if (!kPrfMutexInit(&g_ThreadsMutex))
        {
            if (!kPrfMutexInit(&g_ModSegsMutex))
            {
                if (!kPrfRWLockInit(&g_FunctionsRWLock))
                {
                    /*
                     * Allocate the TLS entry.
                     */
#if K_OS == K_OS_WINDOWS
                    g_dwThreadTLS = TlsAlloc();
                    if (g_dwThreadTLS != TLS_OUT_OF_INDEXES)

#elif defined(KPRF_USE_PTHREAD)
                    int rc = pthread_key_create(&g_ThreadKey, kPrfPThreadKeyDtor);
                    if (!rc)

#elif K_OS == K_OS_OS2
                    int rc = DosAllocThreadLocalMemory(sizeof(void *), (PULONG*)&g_ppThread); /** @todo check if this is a count or a size. */
                    if (!rc)

#endif
                    {
                        /*
                         * Apply the affinity mask, if specified.
                         */
                        if (fAffinity)
                        {
#if K_OS == K_OS_WINDOWS
                            SetProcessAffinityMask(GetCurrentProcess(), fAffinity);
#endif
                        }

                        g_pHdr = pHdr;
                        g_fEnabled = true;
                        return 0;
                    }
                    kPrfRWLockDelete(&g_FunctionsRWLock);
                }
                kPrfMutexDelete(&g_ModSegsMutex);
            }
            kPrfMutexDelete(&g_ThreadsMutex);
        }
    }
    kPrfFreeMem(pvBuf);
    return -1;
}


/**
 * Stops, dumps, and terminates the profiling.
 *
 * This should typically be called from some kind of module destruction
 * function, so we can profile parts of the termination sequence too.
 *
 * @returns 0 on success
 * @returns -1 on failure.
 *
 */
int kPrfTerminate(void)
{
    /*
     * Stop the profiling.
     * As a safety precaution, sleep a little bit to allow threads
     * still at large inside profiler code some time to get out.
     */
    g_fEnabled = false;
    KPRF_TYPE(P,HDR) pHdr = g_pHdr;
    g_pHdr = NULL;
    if (!pHdr)
        return -1;

#if K_OS == K_OS_WINDOWS
    Sleep(10);
#elif K_OS == K_OS_OS2
    DosSleep(10);
#else
    usleep(10000);
#endif

    /*
     * Unwind all active threads and so forth.
     */
    KPRF_NAME(TerminateAll)(pHdr);

    /*
     * Use the stack space to fill in process details.
     */
#if K_OS == K_OS_WINDOWS
    /* all is one single string */
    const char *pszCommandLine = GetCommandLine();
    if (pszCommandLine)
        KPRF_NAME(SetCommandLine)(pHdr, 1, &pszCommandLine);

#elif K_OS == K_OS_OS2 || K_OS == K_OS_OS2
    PTIB pTib;
    PPIB pPib;
    DosGetInfoBlocks(&pTib, &pPib);
    if (pPib->pib_pchcmd)
    {
        /* Tradition say that the commandline is made up of two zero terminate strings
         * - first the executable name, then the arguments. Similar to what unix does,
         *   only completely mocked up because of the CMD.EXE tradition.
         */
        const char *apszArgs[2];
        apszArgs[0] = pPib->pib_pchcmd;
        apszArgs[1] = pPib->pib_pchcmd;
        while (apszArgs[1][0])
            apszArgs[1]++;
        apszArgs[1]++;
        KPRF_NAME(SetCommandLine)(pHdr, 2, apszArgs);
    }

#else
    /* linux can read /proc/self/something I guess. Don't know about the rest... */

#endif

    /*
     * Write the file to disk.
     */
    char szName[260 + 16];
    kPrfGetEnvString("KPRF2_FILE", szName, sizeof(szName) - 16, "kPrf2-");

    /* append the process id */
    KUPTR pid = kPrfGetProcessId();
    char *psz = szName;
    while (*psz)
        psz++;

    static char s_szDigits[0x11] = "0123456789abcdef";
    KU32 uShift = KPRF_BITS - 4;
    while (     uShift > 0
           &&   !(pid & (0xf << uShift)))
        uShift -= 4;
    *psz++ = s_szDigits[(pid >> uShift) & 0xf];
     while (uShift > 0)
     {
         uShift -= 4;
         *psz++ = s_szDigits[(pid >> uShift) & 0xf];
     }

    /* .kPrf2 */
    *psz++ = '.';
    *psz++ = 'k';
    *psz++ = 'P';
    *psz++ = 'r';
    *psz++ = 'f';
    *psz++ = '2';
    *psz++ = '\0';

    /* write the file. */
    int rc = kPrfWriteFile(szName, pHdr, pHdr->cb);

    /*
     * Free resources.
     */
    kPrfFreeMem(pHdr);
#if K_OS == K_OS_WINDOWS
    TlsFree(g_dwThreadTLS);
    g_dwThreadTLS = TLS_OUT_OF_INDEXES;

#elif defined(KPRF_USE_PTHREAD)
    pthread_key_delete(g_ThreadKey);
    g_ThreadKey = (pthread_key_t)-1;

#elif K_OS == K_OS_OS2
    DosFreeThreadLocalMemory((PULONG)g_ppThread);
    g_ppThread = NULL;

#else
# error "port me!"
#endif

    kPrfMutexDelete(&g_ThreadsMutex);
    kPrfMutexDelete(&g_ModSegsMutex);
    kPrfRWLockDelete(&g_FunctionsRWLock);

    return rc;
}


/**
 * Terminate the current thread.
 */
void kPrfTerminateThread(void)
{
    KPRF_NAME(DeregisterThread)();
}


#ifdef KPRF_USE_PTHREAD
/**
 * TLS destructor.
 */
static void kPrfPThreadKeyDtor(void *pvThread)
{
    KPRF_TYPE(P,HDR) pHdr = KPRF_GET_HDR();
    if (pHdr)
    {
        KPRF_TYPE(P,THREAD) pThread = (KPRF_TYPE(P,THREAD))pvThread;
        pthread_setspecific(g_ThreadKey, pvThread);
        KPRF_NAME(TerminateThread)(pHdr, pThread, KPRF_NOW());
        pthread_setspecific(g_ThreadKey, NULL);
    }
}
#endif

