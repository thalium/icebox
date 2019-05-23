/* $Id: tstRTProcWait.cpp $ */
/** @file
 * IPRT Testcase - RTProcWait.
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
#include <iprt/initterm.h>
#include <iprt/process.h>
#include <iprt/thread.h>
#include <iprt/stream.h>
#include <iprt/semaphore.h>
#include <iprt/env.h>
#include <iprt/err.h>
#include <iprt/string.h>




typedef struct SpawnerArgs
{
    RTPROCESS Process;
    const char *pszExe;
} SPAWNERARGS, *PSPAWNERARGS;


DECLCALLBACK(int) SpawnerThread(RTTHREAD Thread, void *pvUser)
{
    RT_NOREF1(Thread);
    PSPAWNERARGS pArgs = (PSPAWNERARGS)pvUser;
    pArgs->Process = NIL_RTPROCESS;
    const char *apszArgs[3] = { pArgs->pszExe, "child", NULL };
    return RTProcCreate(apszArgs[0], apszArgs, RTENV_DEFAULT, 0, &pArgs->Process);
}


int main(int argc, char **argv)
{
    RTR3InitExe(argc, &argv, 0);
    if (argc == 2 && !strcmp(argv[1], "child"))
        return 42;

    RTPrintf("tstRTWait: spawning a child in a separate thread and waits for it in the main thread...\n");
    RTTHREAD  Thread;
    SPAWNERARGS Args = { NIL_RTPROCESS, argv[0] };
    int rc = RTThreadCreate(&Thread, SpawnerThread, &Args, 0, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "SPAWNER");
    if (RT_SUCCESS(rc))
    {
        /* Wait for it to complete. */
        int rc2;
        rc = RTThreadWait(Thread, RT_INDEFINITE_WAIT, &rc2);
        if (RT_SUCCESS(rc))
            rc = rc2;
        if (RT_SUCCESS(rc))
        {
            /* wait for the process to complete */
            RTPROCSTATUS Status;
            rc = RTProcWait(Args.Process, 0, &Status);
            if (RT_SUCCESS(rc))
            {
                if (    Status.enmReason == RTPROCEXITREASON_NORMAL
                    &&  Status.iStatus == 42)
                    RTPrintf("tstRTWait: Success!\n");
                else
                {
                    rc = VERR_GENERAL_FAILURE;
                    if (Status.enmReason != RTPROCEXITREASON_NORMAL)
                        RTPrintf("tstRTWait: Expected exit reason RTPROCEXITREASON_NORMAL, got %d.\n", Status.enmReason);
                    else
                        RTPrintf("tstRTWait: Expected exit status 42, got %d.\n", Status.iStatus);
                }
            }
            else
                RTPrintf("tstRTWait: RTProcWait failed with rc=%Rrc!\n", rc);
        }
        else
            RTPrintf("tstRTWait: RTThreadWait or SpawnerThread failed with rc=%Rrc!\n", rc);
    }
    else
        RTPrintf("tstRTWait: RTThreadCreate failed with rc=%Rrc!\n", rc);

    return RT_SUCCESS(rc) ? 0 : 1;
}
