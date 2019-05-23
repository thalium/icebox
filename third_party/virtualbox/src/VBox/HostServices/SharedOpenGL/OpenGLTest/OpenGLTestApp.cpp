/* $Id: OpenGLTestApp.cpp $ */
/** @file
 * VBox host opengl support test application.
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

#include <iprt/assert.h>
#include <iprt/buildconfig.h>
#include <iprt/err.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/stream.h>
#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h>
#endif
#if !defined(RT_OS_WINDOWS) && !defined(RT_OS_OS2)
# include <sys/resource.h>
# include <fcntl.h>
# include <unistd.h>
#endif

#include <string.h>

#define VBOXGLTEST_WITH_LOGGING

#ifdef VBOXGLTEST_WITH_LOGGING
#include "package-generated.h"

#include <iprt/log.h>
#include <iprt/param.h>
#include <iprt/time.h>
#include <iprt/system.h>
#include <iprt/process.h>
#include <iprt/env.h>

#include <VBox/log.h>
#include <VBox/version.h>
#endif

#ifdef VBOX_WITH_CROGL

extern "C"
{
  extern void * crSPULoad(void *, int, char *, char *, void *);
  extern void crSPUUnloadChain(void *);
}


static int vboxCheck3DAccelerationSupported()
{
    LogRel(("Testing 3D Support:\n"));
    void *spu = crSPULoad(NULL, 0, (char*)"render", NULL, NULL);
    if (spu)
    {
        crSPUUnloadChain(spu);
        LogRel(("Testing 3D Succeeded!\n"));
        return 0;
    }
    LogRel(("Testing 3D Failed\n"));
    return 1;
}
#endif

#ifdef VBOX_WITH_VIDEOHWACCEL
#include <QGLWidget>
#include <QApplication>
#include <VBox/VBoxGL2D.h>

static int vboxCheck2DVideoAccelerationSupported()
{
    LogRel(("Testing 2D Support:\n"));
    static int dummyArgc = 1;
    static char * dummyArgv = (char*)"GlTest";
    QApplication app (dummyArgc, &dummyArgv);

    VBoxGLTmpContext ctx;
    const QGLContext *pContext = ctx.makeCurrent();
    if(pContext)
    {
        VBoxVHWAInfo supportInfo;
        supportInfo.init(pContext);
        if(supportInfo.isVHWASupported())
        {
            LogRel(("Testing 2D Succeeded!\n"));
            return 0;
        }
    }
    else
    {
        LogRel(("Failed to create gl context\n"));
    }
    LogRel(("Testing 2D Failed\n"));
    return 1;
}
#endif

#ifdef VBOXGLTEST_WITH_LOGGING
static int vboxInitLogging(const char *pszFilename, bool bGenNameSuffix)
{
    PRTLOGGER loggerRelease;
    static const char * const s_apszGroups[] = VBOX_LOGGROUP_NAMES;
    RTUINT fFlags = RTLOGFLAGS_PREFIX_TIME_PROG;
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    fFlags |= RTLOGFLAGS_USECRLF;
#endif
    const char * pszFilenameFmt;
    RTLOGDEST enmLogDest;
    if(pszFilename)
    {
        if(bGenNameSuffix)
            pszFilenameFmt = "%s.%ld.log";
        else
            pszFilenameFmt = "%s";
        enmLogDest = RTLOGDEST_FILE;
    }
    else
    {
        pszFilenameFmt = NULL;
        enmLogDest = RTLOGDEST_STDOUT;
    }

    int vrc = RTLogCreateEx(&loggerRelease, fFlags, "all",
                            "VBOX_RELEASE_LOG", RT_ELEMENTS(s_apszGroups), s_apszGroups, enmLogDest,
                            NULL /* pfnBeginEnd */, 0 /* cHistory */, 0 /* cbHistoryFileMax */, 0 /* uHistoryTimeMax */,
                            NULL /* pErrInfo */, pszFilenameFmt, pszFilename, RTTimeMilliTS());
    if (RT_SUCCESS(vrc))
    {
        /* some introductory information */
        RTTIMESPEC timeSpec;
        char szTmp[256];
        RTTimeSpecToString(RTTimeNow(&timeSpec), szTmp, sizeof(szTmp));
        RTLogRelLogger(loggerRelease, 0, ~0U,
                       "VBoxTestGL %s r%u %s (%s %s) release log\n"
#ifdef VBOX_BLEEDING_EDGE
                       "EXPERIMENTAL build " VBOX_BLEEDING_EDGE "\n"
#endif
                       "Log opened %s\n",
                       VBOX_VERSION_STRING, RTBldCfgRevision(), VBOX_BUILD_TARGET,
                       __DATE__, __TIME__, szTmp);

        vrc = RTSystemQueryOSInfo(RTSYSOSINFO_PRODUCT, szTmp, sizeof(szTmp));
        if (RT_SUCCESS(vrc) || vrc == VERR_BUFFER_OVERFLOW)
            RTLogRelLogger(loggerRelease, 0, ~0U, "OS Product: %s\n", szTmp);
        vrc = RTSystemQueryOSInfo(RTSYSOSINFO_RELEASE, szTmp, sizeof(szTmp));
        if (RT_SUCCESS(vrc) || vrc == VERR_BUFFER_OVERFLOW)
            RTLogRelLogger(loggerRelease, 0, ~0U, "OS Release: %s\n", szTmp);
        vrc = RTSystemQueryOSInfo(RTSYSOSINFO_VERSION, szTmp, sizeof(szTmp));
        if (RT_SUCCESS(vrc) || vrc == VERR_BUFFER_OVERFLOW)
            RTLogRelLogger(loggerRelease, 0, ~0U, "OS Version: %s\n", szTmp);
        vrc = RTSystemQueryOSInfo(RTSYSOSINFO_SERVICE_PACK, szTmp, sizeof(szTmp));
        if (RT_SUCCESS(vrc) || vrc == VERR_BUFFER_OVERFLOW)
            RTLogRelLogger(loggerRelease, 0, ~0U, "OS Service Pack: %s\n", szTmp);
//        RTLogRelLogger(loggerRelease, 0, ~0U, "Host RAM: %uMB RAM, available: %uMB\n",
//                       uHostRamMb, uHostRamAvailMb);
        /* the package type is interesting for Linux distributions */
        char szExecName[RTPATH_MAX];
        char *pszExecName = RTProcGetExecutablePath(szExecName, sizeof(szExecName));
        RTLogRelLogger(loggerRelease, 0, ~0U,
                       "Executable: %s\n"
                       "Process ID: %u\n"
                       "Package type: %s"
#ifdef VBOX_OSE
                       " (OSE)"
#endif
                       "\n",
                       pszExecName ? pszExecName : "unknown",
                       RTProcSelf(),
                       VBOX_PACKAGE_STRING);

        /* register this logger as the release logger */
        RTLogRelSetDefaultInstance(loggerRelease);

        return VINF_SUCCESS;
    }

    return vrc;
}
#endif

static int vboxInitQuietMode()
{
#if !defined(RT_OS_WINDOWS) && !defined(RT_OS_OS2)
    /* This small test application might crash on some hosts. Do never
     * generate a core dump as most likely some OpenGL library is
     * responsible. */
    struct rlimit lim = { 0, 0 };
    setrlimit(RLIMIT_CORE, &lim);

    /* Redirect stderr to /dev/null */
    int fd = open("/dev/null", O_WRONLY);
    if (fd != -1)
        dup2(fd, STDERR_FILENO);
#endif
    return 0;
}

int main(int argc, char **argv)
{
    int rc = 0;

    RTR3InitExe(argc, &argv, 0);

    if(argc < 2)
    {
#ifdef VBOX_WITH_CROGL
        /* backwards compatibility: check 3D */
        rc = vboxCheck3DAccelerationSupported();
#endif
    }
    else
    {
        static const RTGETOPTDEF s_aOptionDefs[] =
        {
            { "--test",           't',   RTGETOPT_REQ_STRING },
            { "-test",            't',   RTGETOPT_REQ_STRING },
#ifdef VBOXGLTEST_WITH_LOGGING
            { "--log",            'l',   RTGETOPT_REQ_STRING },
#endif
        };

        RTGETOPTSTATE State;
        rc = RTGetOptInit(&State, argc-1, argv+1, &s_aOptionDefs[0], RT_ELEMENTS(s_aOptionDefs), 0, 0);
        AssertRCReturn(rc, 49);

#ifdef VBOX_WITH_VIDEOHWACCEL
        bool bTest2D = false;
#endif
#ifdef VBOX_WITH_CROGL
        bool bTest3D = false;
#endif
#ifdef VBOXGLTEST_WITH_LOGGING
        bool bLog = false;
        bool bLogSuffix = false;
        const char * pLog = NULL;
#endif

        for (;;)
        {
            RTGETOPTUNION Val;
            rc = RTGetOpt(&State, &Val);
            if (!rc)
                break;
            switch (rc)
            {
                case 't':
#ifdef VBOX_WITH_CROGL
                    if (!strcmp(Val.psz, "3D") || !strcmp(Val.psz, "3d"))
                    {
                        bTest3D = true;
                        rc = 0;
                        break;
                    }
#endif
#ifdef VBOX_WITH_VIDEOHWACCEL
                    if (!strcmp(Val.psz, "2D") || !strcmp(Val.psz, "2d"))
                    {
                        bTest2D = true;
                        rc = 0;
                        break;
                    }
#endif
                    rc = 1;
                    break;
#ifdef VBOXGLTEST_WITH_LOGGING
                case 'l':
                    bLog = true;
                    pLog = Val.psz;
                    rc = 0;
                    break;
#endif
                case 'h':
                    RTPrintf(VBOX_PRODUCT " Helper for testing 2D/3D OpenGL capabilities %u.%u.%u\n"
                             "(C) 2009-" VBOX_C_YEAR " " VBOX_VENDOR "\n"
                             "All rights reserved.\n"
                             "\n"
                             "Parameters:\n"
#ifdef VBOX_WITH_VIDEOHWACCEL
                             "  --test 2D             test for 2D (video) OpenGL capabilities\n"
#endif
#ifdef VBOX_WITH_CROGL
                             "  --test 3D             test for 3D OpenGL capabilities\n"
#endif
#ifdef VBOXGLTEST_WITH_LOGGING
                             "  --log <log_file_name> log the GL test result to the given file\n"
                             "\n"
                             "Logging can alternatively be enabled by specifying the VBOXGLTEST_LOG=<log_file_name> env variable\n"

#endif
                             "\n",
                            RTBldCfgVersionMajor(), RTBldCfgVersionMinor(), RTBldCfgVersionBuild());
                    break;

                case 'V':
                    RTPrintf("$Revision: 119149 $\n");
                    return 0;

                case VERR_GETOPT_UNKNOWN_OPTION:
                case VINF_GETOPT_NOT_OPTION:
                    rc = 1;

                default:
                    /* complain? RTGetOptPrintError(rc, &Val); */
                    break;
            }

            if (rc)
                break;
        }

        if(!rc)
        {
#ifdef VBOXGLTEST_WITH_LOGGING
            if(!bLog)
            {
                /* check the VBOXGLTEST_LOG env var */
                pLog = RTEnvGet("VBOXGLTEST_LOG");
                if(pLog)
                    bLog = true;
                bLogSuffix = true;
            }
            if(bLog)
                rc = vboxInitLogging(pLog, bLogSuffix);
            else
#endif
                rc = vboxInitQuietMode();

#ifdef VBOX_WITH_CROGL
            if(!rc && bTest3D)
                rc = vboxCheck3DAccelerationSupported();
#endif

#ifdef VBOX_WITH_VIDEOHWACCEL
            if(!rc && bTest2D)
                rc = vboxCheck2DVideoAccelerationSupported();
#endif

        }
    }

    /*RTR3Term();*/
    return rc;

}

#ifdef RT_OS_WINDOWS
extern "C" int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    RT_NOREF(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
    return main(__argc, __argv);
}
#endif

