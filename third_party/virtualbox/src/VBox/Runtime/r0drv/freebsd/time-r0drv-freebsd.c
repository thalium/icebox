/* $Id: time-r0drv-freebsd.c $ */
/** @file
 * IPRT - Time, Ring-0 Driver, FreeBSD.
 */

/*
 * Copyright (c) 2007 knut st. osmundsen <bird-src-spam@anduin.net>
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "the-freebsd-kernel.h"
#define RTTIME_INCL_TIMESPEC

#include <iprt/time.h>


RTDECL(uint64_t) RTTimeNanoTS(void)
{
    struct timespec tsp;
    nanouptime(&tsp);
    return tsp.tv_sec * RT_NS_1SEC_64
         + tsp.tv_nsec;
}


RTDECL(uint64_t) RTTimeMilliTS(void)
{
    return RTTimeNanoTS() / RT_NS_1MS;
}


RTDECL(uint64_t) RTTimeSystemNanoTS(void)
{
    return RTTimeNanoTS();
}


RTDECL(uint64_t) RTTimeSystemMilliTS(void)
{
    return RTTimeMilliTS();
}


RTDECL(PRTTIMESPEC) RTTimeNow(PRTTIMESPEC pTime)
{
    struct timespec tsp;
    nanotime(&tsp);
    return RTTimeSpecSetTimespec(pTime, &tsp);
}

