/* $Id: OpenGLTest.cpp $ */
/** @file
 * VBox host opengl support test - generic implementation.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <VBox/err.h>
#include <iprt/assert.h>
#include <iprt/env.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/process.h>
#include <iprt/string.h>
#include <iprt/time.h>
#include <iprt/thread.h>
#include <iprt/env.h>
#include <iprt/log.h>

#include <VBox/VBoxOGL.h>

bool RTCALL VBoxOglIs3DAccelerationSupported(void)
{
    if (RTEnvExist("VBOX_CROGL_FORCE_SUPPORTED"))
    {
        LogRel(("VBOX_CROGL_FORCE_SUPPORTED is specified, skipping 3D test, and treating as supported\n"));
        return true;
    }

    static char pszVBoxPath[RTPATH_MAX];
    const char *papszArgs[4] = { NULL, "-test", "3D", NULL};
    int rc;
    RTPROCESS Process;
    RTPROCSTATUS ProcStatus;
    uint64_t StartTS;

#ifdef __SANITIZE_ADDRESS__
    /* The OpenGL test tool contains a number of memory leaks which cause it to
     * return failure when run with ASAN unless we disable the leak detector. */
    RTENV env;
    if (RT_FAILURE(RTEnvClone(&env, RTENV_DEFAULT)))
        return false;
    RTEnvPutEx(env, "ASAN_OPTIONS=detect_leaks=0");  /* If this fails we will notice later */
#endif
    rc = RTPathExecDir(pszVBoxPath, RTPATH_MAX); AssertRCReturn(rc, false);
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    rc = RTPathAppend(pszVBoxPath, RTPATH_MAX, "VBoxTestOGL.exe");
#else
    rc = RTPathAppend(pszVBoxPath, RTPATH_MAX, "VBoxTestOGL");
#endif
    papszArgs[0] = pszVBoxPath;         /* argv[0] */
    AssertRCReturn(rc, false);

#ifndef __SANITIZE_ADDRESS__
    rc = RTProcCreate(pszVBoxPath, papszArgs, RTENV_DEFAULT, 0, &Process);
#else
    rc = RTProcCreate(pszVBoxPath, papszArgs, env, 0, &Process);
    RTEnvDestroy(env);
#endif
    if (RT_FAILURE(rc))
        return false;

    StartTS = RTTimeMilliTS();

    while (1)
    {
        rc = RTProcWait(Process, RTPROCWAIT_FLAGS_NOBLOCK, &ProcStatus);
        if (rc != VERR_PROCESS_RUNNING)
            break;

#ifndef DEBUG_misha
        if (RTTimeMilliTS() - StartTS > 30*1000 /* 30 sec */)
        {
            RTProcTerminate(Process);
            RTThreadSleep(100);
            RTProcWait(Process, RTPROCWAIT_FLAGS_NOBLOCK, &ProcStatus);
            return false;
        }
#endif
        RTThreadSleep(100);
    }

    if (RT_SUCCESS(rc))
    {
        if ((ProcStatus.enmReason==RTPROCEXITREASON_NORMAL) && (ProcStatus.iStatus==0))
        {
            return true;
        }
    }

    return false;
}

