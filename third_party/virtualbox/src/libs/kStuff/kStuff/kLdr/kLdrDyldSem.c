/* $Id: kLdrDyldSem.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kLdr - The Dynamic Loader, Semaphore Helper Functions.
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
#include <k/kHlpSem.h>
#include <k/kHlpAssert.h>

#if K_OS == K_OS_DARWIN
# include <mach/mach.h>
# undef mach_task_self  /* don't use the macro (if we're using bare helpers ) */

#elif K_OS == K_OS_OS2
# define INCL_BASE
# define INCL_ERRORS
# include <os2.h>

#elif  K_OS == K_OS_WINDOWS
# include <Windows.h>

#else
# error "port me"
#endif



/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
#if K_OS == K_OS_DARWIN
/** The loader sempahore. */
static semaphore_t      g_Semaphore = MACH_PORT_NULL;

#elif K_OS == K_OS_OS2
/** The loader sempahore. */
static HMTX             g_hmtx;

#elif  K_OS == K_OS_WINDOWS
/** The loader sempahore. */
static CRITICAL_SECTION g_CritSect;

#else
# error "port me"
#endif


/**
 * Initializes the loader semaphore.
 *
 * @returns 0 on success, non-zero OS status code on failure.
 */
int kLdrDyldSemInit(void)
{
#if K_OS == K_OS_DARWIN
    kern_return_t krc;

    krc = semaphore_create(mach_task_self(), &g_Semaphore, SYNC_POLICY_FIFO, 0);
    if (krc != KERN_SUCCESS)
        return krc;

#elif K_OS == K_OS_OS2
    APIRET rc;
    g_hmtx = NULLHANDLE;
    rc = DosCreateMutexSem(NULL, &g_hmtx, 0, FALSE);
    if (rc)
        return rc;

#elif  K_OS == K_OS_WINDOWS
    InitializeCriticalSection(&g_CritSect);

#else
# error "port me"
#endif
    return 0;
}


/**
 * Terminates the loader semaphore.
 */
void kLdrDyldSemTerm(void)
{
#if K_OS == K_OS_DARWIN
    kern_return_t krc;
    semaphore_t Semaphore = g_Semaphore;
    g_Semaphore = MACH_PORT_NULL;
    krc = semaphore_destroy(mach_task_self(), Semaphore);
    kHlpAssert(krc == KERN_SUCCESS); (void)krc;

#elif K_OS == K_OS_OS2
    HMTX hmtx = g_hmtx;
    g_hmtx = NULLHANDLE;
    DosCloseMutexSem(hmtx);

#elif  K_OS == K_OS_WINDOWS
    DeleteCriticalSection(&g_CritSect);

#else
# error "port me"
#endif
}


/**
 * Requests the loader sempahore ownership.
 * This can be done recursivly.
 *
 * @returns 0 on success, non-zero OS status code on failure.
 */
int kLdrDyldSemRequest(void)
{
#if K_OS == K_OS_DARWIN
    /* not sure about this... */
    kern_return_t krc;
    do krc = semaphore_wait(g_Semaphore);
    while (krc == KERN_ABORTED);
    if (krc == KERN_SUCCESS)
        return 0;
    return krc;

#elif K_OS == K_OS_OS2
    APIRET rc = DosRequestMutexSem(g_hmtx, 5000);
    if (rc == ERROR_TIMEOUT || rc == ERROR_SEM_TIMEOUT || rc == ERROR_INTERRUPT)
    {
        unsigned i = 0;
        do
        {
            /** @todo check for deadlocks etc. */
            rc = DosRequestMutexSem(g_hmtx, 1000);
        } while (   (   rc == ERROR_TIMEOUT
                     || rc == ERROR_SEM_TIMEOUT
                     || rc == ERROR_INTERRUPT)
                 && i++ < 120);
    }
    return rc;

#elif  K_OS == K_OS_WINDOWS
    EnterCriticalSection(&g_CritSect);
    return 0;

#else
# error "port me"
#endif
}


/**
 * Releases the loader semaphore ownership.
 * The caller is responsible for making sure it's the semaphore owner!
 */
void kLdrDyldSemRelease(void)
{
#if K_OS == K_OS_DARWIN
    /* not too sure about this... */
    kern_return_t krc = semaphore_signal(g_Semaphore);
    kHlpAssert(krc == KERN_SUCCESS); (void)krc;

#elif K_OS == K_OS_OS2
    APIRET rc = DosReleaseMutexSem(g_hmtx);
    kHlpAssert(!rc); (void)rc;

#elif  K_OS == K_OS_WINDOWS
    LeaveCriticalSection(&g_CritSect);

#else
# error "port me"
#endif
}

