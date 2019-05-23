/* $Id: tst-3.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kLdr - Dynamic Loader testcase no. 3, Driver.
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

#include "tst.h"


int g_i1 = 1;
int g_i2 = 2;
int *g_pi1 = &g_i1;

extern int Tst3Sub(int);
int (*g_pfnTst3Sub)(int) = &Tst3Sub;

MY_IMPORT(int) Tst3Ext(int);
int (*g_pfnTst3Ext)(int) = &Tst3Ext;

char g_achBss[256];


MY_EXPORT(int) Tst3(int iFortyTwo)
{
    int rc;

    if (iFortyTwo != 42)
        return 0;
    if (g_i1 != 1)
        return 1;
    if (g_i2 != 2)
        return 2;
    if (g_pi1 != &g_i1)
        return 3;
    if (g_pfnTst3Sub != &Tst3Sub)
        return 4;
    rc = Tst3Sub(iFortyTwo);
    if (rc != g_pfnTst3Sub(iFortyTwo))
        return 5;
    rc = Tst3Ext(iFortyTwo);
    if (rc != 42)
        return 6;
    rc = g_pfnTst3Ext(iFortyTwo);
    if (rc != 42)
        return 7;
    for (rc = 0; rc < sizeof(g_achBss); rc++)
        if (g_achBss[rc])
            return 8;
    if (g_achBss[0] || g_achBss[1] || g_achBss[255])
        return 9;

    return 42;
}

