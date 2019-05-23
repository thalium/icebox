/* $Id: kLdrDyldOS.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kLdr - The Dynamic Loader, OS specific operations.
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
#include <k/kLdr.h>
#include "kLdrInternal.h"

#if K_OS == K_OS_OS2
# define INCL_BASE
# define INCL_ERRORS
# include <os2.h>

#elif K_OS == K_OS_WINDOWS
# undef IMAGE_DOS_SIGNATURE
# undef IMAGE_NT_SIGNATURE
# include <Windows.h>

#else
# include <k/kHlpAlloc.h>

#endif


/**
 * Allocates a stack.
 *
 * @returns Pointer to the stack. NULL on allocation failure (assumes out of memory).
 * @param   cb      The size of the stack. This shall be page aligned.
 *                  If 0, a OS specific default stack size will be employed.
 */
void *kldrDyldOSAllocStack(KSIZE cb)
{
#if K_OS == K_OS_OS2
    APIRET rc;
    PVOID pv;

    if (!cb)
        cb = 1 * 1024*1024; /* 1MB */

    rc = DosAllocMem(&pv, cb, OBJ_TILE | PAG_COMMIT | PAG_WRITE | PAG_READ);
    if (rc == NO_ERROR)
        return pv;
    return NULL;

#elif K_OS == K_OS_WINDOWS

    if (!cb)
        cb = 1 *1024*1024; /* 1MB */

    return VirtualAlloc(NULL, cb, MEM_COMMIT, PAGE_READWRITE);

#else
    void *pv;

    if (!cb)
        cb = 1 * 1024*1024; /* 1MB */

    if (!kHlpPageAlloc(&pv, cb, KPROT_READWRITE, K_FALSE))
        return pv;
    return NULL;
#endif
}


/**
 * Invokes the main executable entry point with whatever
 * parameters specific to the host OS and/or module format.
 *
 * @returns
 * @param   uMainEPAddress  The address of the main entry point.
 * @param   pvStack         Pointer to the stack object.
 * @param   cbStack         The size of the stack object.
 */
int kldrDyldOSStartExe(KUPTR uMainEPAddress, void *pvStack, KSIZE cbStack)
{
#if K_OS == K_OS_WINDOWS
    /*
     * Invoke the entrypoint on the current stack for now.
     * Deal with other formats and stack switching another day.
     */
    int rc;
    int (*pfnEP)(void);
    pfnEP = (int (*)(void))uMainEPAddress;

    rc = pfnEP();

    TerminateProcess(GetCurrentProcess(), rc);
    kHlpAssert(!"TerminateProcess failed");
    for (;;)
        TerminateProcess(GetCurrentProcess(), rc);
#endif

    return -1;
}


void kldrDyldDoLoadExeStackSwitch(PKLDRDYLDMOD pExe, void *pvStack, KSIZE cbStack)
{
    /*kHlpAssert(!"not implemented");*/

    /** @todo implement this properly! */
    kldrDyldDoLoadExe(pExe);
}

