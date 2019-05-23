/* $Id: tstLdrLoad.cpp $ */
/** @file
 * IPRT Testcase - Native Loader.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


#include <iprt/ldr.h>
#include <iprt/stream.h>
#include <iprt/initterm.h>
#include <iprt/err.h>

int main(int argc, char **argv)
{
    int rcRet = 0;
    RTR3InitExe(argc, &argv, 0);

    /*
     * If no args, display usage.
     */
    if (argc <= 1)
    {
        RTPrintf("Syntax: %s [so/dll [so/dll [..]]\n", argv[0]);
        return 1;
    }

    /*
     * Iterate the arguments and treat all of them as so/dll paths.
     */
    for (int i = 1; i < argc; i++)
    {
        RTLDRMOD hLdrMod = (RTLDRMOD)(uintptr_t)0xbaadffaa;
        int rc = RTLdrLoad(argv[i], &hLdrMod);
        if (RT_SUCCESS(rc))
        {
            RTPrintf("tstLdrLoad: %d - %s\n", i, argv[i]);
            rc = RTLdrClose(hLdrMod);
            if (RT_FAILURE(rc))
            {
                RTPrintf("tstLdrLoad: rc=%Rrc RTLdrClose()\n", rc);
                rcRet++;
            }
        }
        else
        {
            RTPrintf("tstLdrLoad: rc=%Rrc RTLdrOpen('%s')\n", rc, argv[i]);
            rcRet++;
        }
    }

    /*
     * Summary.
     */
    if (!rcRet)
        RTPrintf("tstLdrLoad: SUCCESS\n");
    else
        RTPrintf("tstLdrLoad: FAILURE - %d errors\n", rcRet);

    return !!rcRet;
}
