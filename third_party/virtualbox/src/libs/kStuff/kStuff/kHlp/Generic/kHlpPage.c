/* $Id: kHlpPage.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlp - Generic Page Memory Functions.
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
#include <k/kHlpAlloc.h>
#include <k/kHlpAssert.h>

#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
# include <k/kHlpSys.h>
# include <sys/mman.h>

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
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
/* nothing */
#elif K_OS == K_OS_OS2
/** The base of the loader stub object. <kLdr Hack>
 * The OS/2 exe stub consists of a single data object. When allocating memory
 * for an executable, we'll have to reuse this. */
static void            *g_pvStub = NULL;
/** The size of the stub object - 0 if no stub. <kLdr Hack> */
static KSIZE            g_cbStub = 0;

#elif  K_OS == K_OS_WINDOWS
/** The system info. */
static SYSTEM_INFO      g_SystemInfo;
#else
# error "port me"
#endif



#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
static int kHlpPageProtToNative(KPROT enmProt)
{
    switch (enmProt)
    {
        case KPROT_NOACCESS:             return PROT_NONE;
        case KPROT_READONLY:             return PROT_READ;
        case KPROT_READWRITE:            return PROT_READ | PROT_WRITE;
        case KPROT_EXECUTE:              return PROT_EXEC;
        case KPROT_EXECUTE_READ:         return PROT_EXEC | PROT_READ;
        case KPROT_EXECUTE_READWRITE:    return PROT_EXEC | PROT_READ | PROT_WRITE;
        default:
            kHlpAssert(0);
            return ~0U;
    }
}

#elif K_OS == K_OS_OS2
static ULONG kHlpPageProtToNative(KPROT enmProt)
{
    switch (enmProt)
    {
        case KPROT_NOACCESS:             return PAG_EXECUTE | PAG_READ | PAG_WRITE;
        case KPROT_READONLY:             return PAG_COMMIT | PAG_READ;
        case KPROT_READWRITE:            return PAG_COMMIT | PAG_READ | PAG_WRITE;
        case KPROT_EXECUTE:              return PAG_COMMIT | PAG_EXECUTE;
        case KPROT_EXECUTE_READ:         return PAG_COMMIT | PAG_EXECUTE | PAG_READ;
        case KPROT_EXECUTE_READWRITE:    return PAG_COMMIT | PAG_EXECUTE | PAG_READ | PAG_WRITE;
        default:
            kHlpAssert(0);
            return ~0U;
    }
}
#elif  K_OS == K_OS_WINDOWS
static DWORD kHlpPageProtToNative(KPROT enmProt)
{
    switch (enmProt)
    {
        case KPROT_NOACCESS:             return PAGE_NOACCESS;
        case KPROT_READONLY:             return PAGE_READONLY;
        case KPROT_READWRITE:            return PAGE_READWRITE;
        case KPROT_EXECUTE:              return PAGE_EXECUTE;
        case KPROT_EXECUTE_READ:         return PAGE_EXECUTE_READ;
        case KPROT_EXECUTE_READWRITE:    return PAGE_EXECUTE_READWRITE;
        default:
            kHlpAssert(0);
            return ~0U;
    }
}
#endif



/**
 * Allocate a chunk of memory with page granularity.
 *
 * @returns 0 on success, non-zero OS status code on failure.
 * @param   ppv         Where to store the address of the allocated memory.
 *                      If fFixed is set, *ppv will on entry contain the desired address (page aligned).
 * @param   cb          Number of bytes. Page aligned.
 * @param   enmProt     The new protection. Copy-on-write is invalid.
 */
KHLP_DECL(int) kHlpPageAlloc(void **ppv, KSIZE cb, KPROT enmProt, KBOOL fFixed)
{
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
    void *pv;

    pv = kHlpSys_mmap(fFixed ? *ppv : NULL, cb, kHlpPageProtToNative(enmProt),
                      fFixed ? MAP_FIXED | MAP_ANON: MAP_ANON, -1, 0);
    if ((KIPTR)pv < 256)
    {
        kHlpAssert(0);
        return (int)(KIPTR)pv; /** @todo convert errno to kErrors */
    }
    *ppv = pv;
    return 0;

#elif K_OS == K_OS_OS2
    APIRET  rc;
    ULONG   fFlags = kHlpPageProtToNative(enmProt);

    if (!fFixed)
    {
        /* simple */
        rc = DosAllocMem(ppv, cb, fFlags | OBJ_ANY);
        if (rc == ERROR_INVALID_PARAMETER)
            rc = DosAllocMem(ppv, cb, fFlags);
    }
    else
    {
        /* not so simple. */
        /** @todo I've got code for this in libc somewhere. */
        rc = -1;
    }
    if (!rc)
        return 0;
    kHlpAssert(0);
    return rc;

#elif  K_OS == K_OS_WINDOWS
    /* (We don't have to care about the stub here, because the stub will be unmapped before we get here.) */
    int     rc;
    DWORD   fProt = kHlpPageProtToNative(enmProt);

    if (!g_SystemInfo.dwPageSize)
        GetSystemInfo(&g_SystemInfo);

    *ppv = VirtualAlloc(fFixed ? *ppv : NULL, cb, MEM_COMMIT, fProt);
    if (*ppv != NULL)
        return 0;
    rc = GetLastError();
    kHlpAssert(0);
    return rc;

#else
# error "port me"
#endif
}


/**
 * Change the protection of one or more pages in an allocation.
 *
 * (This will of course only work correctly on memory allocated by kHlpPageAlloc().)
 *
 * @returns 0 on success, non-zero OS status code on failure.
 * @param   pv          First page. Page aligned.
 * @param   cb          Number of bytes. Page aligned.
 * @param   enmProt     The new protection. Copy-on-write is invalid.
 */
KHLP_DECL(int) kHlpPageProtect(void *pv, KSIZE cb, KPROT enmProt)
{
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
    int rc;

    rc = kHlpSys_mprotect(pv, cb, kHlpPageProtToNative(enmProt));
    if (!rc)
        return 0;
    /** @todo convert errno -> kErrors */
    kHlpAssert(0);
    return rc;

#elif K_OS == K_OS_OS2
    APIRET      rc;
    ULONG       fFlags = kHlpPageProtToNative(enmProt);

    /*
     * The non-stub pages.
     */
    rc = DosSetMem(pv, cb, fFlags);
    if (rc && fFlags != PAG_DECOMMIT)
        rc = DosSetMem(pv, cb, fFlags | PAG_COMMIT);
    if (rc)
    {
        /* Try page by page. */
        while (cb > 0)
        {
            rc = DosSetMem(pv, 0x1000, fFlags);
            if (rc && fFlags != PAG_DECOMMIT)
                rc = DosSetMem(pv, 0x1000, fFlags | PAG_COMMIT);
            if (rc)
                return rc;
            pv = (void *)((KUPTR)pv + 0x1000);
            cb -= 0x1000;
        }
    }
    kHlpAssert(!rc);
    return rc;

#elif  K_OS == K_OS_WINDOWS
    DWORD fOldProt = 0;
    DWORD fProt = kHlpPageProtToNative(enmProt);
    int rc = 0;

    if (!VirtualProtect(pv, cb, fProt, &fOldProt))
    {
        rc = GetLastError();
        kHlpAssert(0);
    }
    return rc;
#else
# error "port me"
#endif
}


/**
 * Free memory allocated by kHlpPageAlloc().
 *
 * @returns 0 on success, non-zero OS status code on failure.
 * @param   pv          The address returned by kHlpPageAlloc().
 * @param   cb          The byte count requested from kHlpPageAlloc().
 */
KHLP_DECL(int) kHlpPageFree(void *pv, KSIZE cb)
{
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
    int rc;

    rc = kHlpSys_munmap(pv, cb);
    if (!rc)
        return 0;
    /** @todo convert errno -> kErrors */
    return rc;

#elif K_OS == K_OS_OS2
    APIRET rc;

    /*
     * Deal with any portion overlapping with the stub.
     */
    KUPTR offStub = (KUPTR)pv - (KUPTR)g_pvStub;
    if (offStub < g_cbStub)
    {
        /* decommit the pages in the stub. */
        KSIZE cbStub = K_MIN(g_cbStub - offStub, cb);
        rc = DosSetMem(pv, cbStub, PAG_DECOMMIT);
        if (rc)
        {
            /* Page by page, ignoring errors after the first success. */
            while (cbStub > 0)
            {
                if (!DosSetMem(pv, 0x1000, PAG_DECOMMIT))
                    rc = 0;
                pv = (void *)((KUPTR)pv + 0x1000);
                cbStub -= 0x1000;
                cb -= 0x1000;
            }
            if (rc)
            {
                kHlpAssert(!rc);
                return rc;
            }
        }
        else
        {
            cb -= cbStub;
            if (!cb)
                return 0;
            pv = (void *)((KUPTR)pv + cbStub);
        }
    }

    /*
     * Free the object.
     */
    rc = DosFreeMem(pv);
    kHlpAssert(!rc);
    return rc;

#elif  K_OS == K_OS_WINDOWS
    /*
     * Free the object.
     */
    int rc = 0;
    if (!VirtualFree(pv, 0 /*cb*/, MEM_RELEASE))
    {
        rc = GetLastError();
        kHlpAssert(0);
    }
    return rc;

#else
# error "port me"
#endif
}

