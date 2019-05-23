/* $Id: kLdr.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kLdr - The Dynamic Loader.
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


/** @mainpage   kLdr - The Dynamic Loader
 *
 * The purpose of kLdr is to provide a generic interface for querying
 * information about and loading executable image modules.
 *
 * kLdr defines the term executable image to include all kinds of files that contains
 * binary code that can be executed on a CPU - linker objects (OBJs/Os), shared
 * objects (SOs), dynamic link libraries (DLLs), executables (EXEs), and all kinds
 * of kernel modules / device drivers (SYSs).
 *
 * kLdr provides two types of services:
 *      -# Inspect or/and load individual modules (kLdrMod).
 *      -# Work as a dynamic loader - construct and maintain an address space (kLdrDy).
 *
 * The kLdrMod API works on KLDRMOD structures where all the internals are exposed, while
 * the kLdrDy API works opque KLDRDY structures. KLDRDY are in reality simple wrappers
 * around KLDRMOD with some extra linking and attributes.
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kLdr.h>
#include "kLdrInternal.h"


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Flag indicating whether we've initialized the loader or not.
 *
 * 0 if not initialized.
 * -1 if we're initializing or terminating.
 * 1 if we've successfully initialized it.
 * -2 if initialization failed.
 */
static int volatile g_fInitialized;



/**
 * Initializes the loader.
 * @returns 0 on success, non-zero OS status code on failure.
 */
int kldrInit(void)
{
    int rc;

    /* check we're already good. */
    if (g_fInitialized == 1)
        return 0;

    /* a tiny serialization effort. */
    for (;;)
    {
        if (g_fInitialized == 1)
            return 0;
        if (g_fInitialized == -2)
            return -1;
        /** @todo atomic test and set if we care. */
        if (g_fInitialized == 0)
        {
            g_fInitialized = -1;
            break;
        }
        kHlpSleep(1);
    }

    /*
     * Do the initialization.
     */
    rc = kHlpHeapInit();
    if (!rc)
    {
        rc = kLdrDyldSemInit();
        if (!rc)
        {
            rc = kldrDyldInit();
            if (!rc)
            {
                g_fInitialized = 1;
                return 0;
            }
            kLdrDyldSemTerm();
        }
        kHlpHeapTerm();
    }
    g_fInitialized = -2;
    return rc;
}


/**
 * Terminates the loader.
 */
void kldrTerm(void)
{
    /* can't terminate unless it's initialized. */
    if (g_fInitialized != 1)
        return;
    g_fInitialized = -1;

    /*
     * Do the termination.
     */
    kLdrDyldSemTerm();
    kHlpHeapTerm();

    /* done */
    g_fInitialized = 0;
}

