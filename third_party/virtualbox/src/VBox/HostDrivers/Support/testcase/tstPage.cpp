/* $Id: tstPage.cpp $ */
/** @file
 * SUP Testcase - Page allocation interface (ring 3).
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/sup.h>
#include <VBox/param.h>
#include <iprt/initterm.h>
#include <iprt/stream.h>
#include <string.h>


int main(int argc, char **argv)
{
    int cErrors = 0;

    RTR3InitExe(argc, &argv, RTR3INIT_FLAGS_SUPLIB);
    int rc = SUPR3Init(NULL);
    cErrors += rc != 0;
    if (!rc)
    {
        void *pv;
        rc = SUPR3PageAlloc(1, &pv);
        cErrors += rc != 0;
        if (!rc)
        {
            memset(pv, 0xff, PAGE_SIZE);
            rc = SUPR3PageFree(pv, 1);
            cErrors += rc != 0;
            if (rc)
                RTPrintf("tstPage: SUPR3PageFree() failed rc=%Rrc\n", rc);
        }
        else
            RTPrintf("tstPage: SUPR3PageAlloc(1,) failed rc=%Rrc\n", rc);

        /*
         * Big chunk.
         */
        rc = SUPR3PageAlloc(1023, &pv);
        cErrors += rc != 0;
        if (!rc)
        {
            memset(pv, 0xfe, 1023 << PAGE_SHIFT);
            rc = SUPR3PageFree(pv, 1023);
            cErrors += rc != 0;
            if (rc)
                RTPrintf("tstPage: SUPR3PageFree() failed rc=%Rrc\n", rc);
        }
        else
            RTPrintf("tstPage: SUPR3PageAlloc(1,) failed rc=%Rrc\n", rc);


        //rc = SUPR3Term();
        cErrors += rc != 0;
        if (rc)
            RTPrintf("tstPage: SUPR3Term failed rc=%Rrc\n", rc);
    }
    else
        RTPrintf("tstPage: SUPR3Init failed rc=%Rrc\n", rc);

    if (!cErrors)
        RTPrintf("tstPage: SUCCESS\n");
    else
        RTPrintf("tstPage: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}
