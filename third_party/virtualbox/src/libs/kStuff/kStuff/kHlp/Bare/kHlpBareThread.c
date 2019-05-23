/* $Id: kHlpBareThread.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpBare - Thread Manipulation.
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
#include <k/kHlpThread.h>

#if K_OS == K_OS_DARWIN
# include <mach/mach_time.h>

#elif K_OS == K_OS_LINUX
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
 * Sleep for a number of milliseconds.
 * @param   cMillies    Number of milliseconds to sleep.
 */
void kHlpSleep(unsigned cMillies)
{
#if K_OS == K_OS_DARWIN
    static struct mach_timebase_info   s_Info;
    static KBOOL                s_fNanoseconds = K_UNKNOWN;
    KU64 uNow = mach_absolute_time();
    KU64 uDeadline;
    KU64 uPeriod;

    if (s_fNanoseconds == K_UNKNOWN)
    {
        if (mach_timebase_info(&s_Info))
            s_fNanoseconds = K_TRUE; /* the easy way out */
        else if (s_Info.denom == s_Info.numer)
            s_fNanoseconds = K_TRUE;
        else
            s_fNanoseconds = K_FALSE;
    }

    uPeriod = (KU64)cMillies * 1000 * 1000;
    if (!s_fNanoseconds)
        uPeriod = (double)uPeriod * s_Info.denom / s_Info.numer; /* Use double to avoid 32-bit trouble. */
    uDeadline = uNow + uPeriod;
    mach_wait_until(uDeadline);

#elif K_OS == K_OS_LINUX
    /** @todo find the right syscall... */

#elif K_OS == K_OS_OS2
    DosSleep(cMillies);
#elif  K_OS == K_OS_WINDOWS
    Sleep(cMillies);
#else
    usleep(cMillies * 1000);
#endif
}

