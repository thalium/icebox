/* $Id: kHlpBareProcess.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpBare - Process Management
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
#include <k/kHlpProcess.h>
#include <k/kHlpAssert.h>

#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
# include <k/kHlpSys.h>

#elif K_OS == K_OS_OS2
# define INCL_BASE
# define INCL_ERRORS
# include <os2.h>
#elif  K_OS == K_OS_WINDOWS
# include <Windows.h>
#else
# error "port me"
#endif


/**
 * Terminate the process.
 *
 * @param   rc      The exit status.
 */
void    kHlpExit(int rc)
{
    for (;;)
    {
#if K_OS == K_OS_DARWIN \
 || K_OS == K_OS_FREEBSD \
 || K_OS == K_OS_LINUX \
 || K_OS == K_OS_NETBSD \
 || K_OS == K_OS_OPENBSD \
 || K_OS == K_OS_SOLARIS
        kHlpSys_exit(rc);

#elif K_OS == K_OS_OS2
        DosExit(EXIT_PROCESS, rc);

#elif  K_OS == K_OS_WINDOWS
        TerminateProcess(GetCurrentProcess(), rc);

#else
# error "Port me"
#endif
        kHlpAssert(!"Impossible");
    }
}

