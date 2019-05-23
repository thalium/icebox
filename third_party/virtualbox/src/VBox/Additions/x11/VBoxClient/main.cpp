/* $Id: main.cpp $ */
/** @file
 * VirtualBox Guest Additions - X11 Client.
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
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>       /* For exit */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <iprt/buildconfig.h>
#include <iprt/critsect.h>
#include <iprt/env.h>
#include <iprt/file.h>
#include <iprt/initterm.h>
#include <iprt/message.h>
#include <iprt/path.h>
#include <iprt/param.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/types.h>
#include <VBox/VBoxGuestLib.h>
#include <VBox/log.h>

#include "VBoxClient.h"

/*static int (*gpfnOldIOErrorHandler)(Display *) = NULL; - unused */

/** Object representing the service we are running.  This has to be global
 * so that the cleanup routine can access it. */
struct VBCLSERVICE **g_pService;
/** The name of our pidfile.  It is global for the benefit of the cleanup
 * routine. */
static char g_szPidFile[RTPATH_MAX] = "";
/** The file handle of our pidfile.  It is global for the benefit of the
 * cleanup routine. */
static RTFILE g_hPidFile;
/** Global critical section held during the clean-up routine (to prevent it
 * being called on multiple threads at once) or things which may not happen
 * during clean-up (e.g. pausing and resuming the service).
 */
RTCRITSECT g_critSect;
/** Counter of how often our deamon has been respawned. */
unsigned cRespawn = 0;

/**
 * Exit with a fatal error.
 *
 * This is used by the VBClFatalError macro and thus needs to be external.
 */
void vbclFatalError(char *pszMessage)
{
    char *pszCommand;
    int status;
    if (pszMessage && cRespawn == 0)
    {
        pszCommand = RTStrAPrintf2("notify-send \"VBoxClient: %s\"", pszMessage);
        if (pszCommand)
        {
            status = system(pszCommand);
            if (WEXITSTATUS(status) != 0)  /* Utility or extension not available. */
            {
                pszCommand = RTStrAPrintf2("xmessage -buttons OK:0 -center \"VBoxClient: %s\"",
                                           pszMessage);
                if (pszCommand)
                {
                    status = system(pszCommand);
                    if (WEXITSTATUS(status) != 0)  /* Utility or extension not available. */
                    {
                        RTPrintf("VBoxClient: %s", pszMessage);
                    }
                }
            }
        }
    }
    _exit(RTEXITCODE_FAILURE);
}

/**
 * Clean up if we get a signal or something.
 *
 * This is extern so that we can call it from other compilation units.
 */
void VBClCleanUp(bool fExit /*=true*/)
{
    /* We never release this, as we end up with a call to exit(3) which is not
     * async-safe.  Unless we fix this application properly, we should be sure
     * never to exit from anywhere except from this method. */
    int rc = RTCritSectEnter(&g_critSect);
    if (RT_FAILURE(rc))
        VBClFatalError(("VBoxClient: Failure while acquiring the global critical section, rc=%Rrc\n", rc));
    if (g_pService)
        (*g_pService)->cleanup(g_pService);
    if (g_szPidFile[0] && g_hPidFile)
        VbglR3ClosePidFile(g_szPidFile, g_hPidFile);
    if (fExit)
        exit(RTEXITCODE_SUCCESS);
}

/**
 * A standard signal handler which cleans up and exits.
 */
static void vboxClientSignalHandler(int cSignal)
{
    LogRel(("VBoxClient: terminated with signal %d\n", cSignal));
    /** Disable seamless mode */
    RTPrintf(("VBoxClient: terminating...\n"));
    VBClCleanUp();
}

/**
 * Xlib error handler for certain errors that we can't avoid.
 */
static int vboxClientXLibErrorHandler(Display *pDisplay, XErrorEvent *pError)
{
    char errorText[1024];

    XGetErrorText(pDisplay, pError->error_code, errorText, sizeof(errorText));
    LogRelFlow(("VBoxClient: an X Window protocol error occurred: %s (error code %d).  Request code: %d, minor code: %d, serial number: %d\n", errorText, pError->error_code, pError->request_code, pError->minor_code, pError->serial));
    return 0;
}

/**
 * Xlib error handler for fatal errors.  This often means that the programme is still running
 * when X exits.
 */
static int vboxClientXLibIOErrorHandler(Display *pDisplay)
{
    RT_NOREF1(pDisplay);
    LogRel(("VBoxClient: a fatal guest X Window error occurred.  This may just mean that the Window system was shut down while the client was still running.\n"));
    VBClCleanUp();
    return 0;  /* We should never reach this. */
}

/**
 * Reset all standard termination signals to call our signal handler, which
 * cleans up and exits.
 */
static void vboxClientSetSignalHandlers(void)
{
    struct sigaction sigAction;

    LogRelFlowFunc(("\n"));
    sigAction.sa_handler = vboxClientSignalHandler;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags = 0;
    sigaction(SIGHUP, &sigAction, NULL);
    sigaction(SIGINT, &sigAction, NULL);
    sigaction(SIGQUIT, &sigAction, NULL);
    sigaction(SIGPIPE, &sigAction, NULL);
    sigaction(SIGALRM, &sigAction, NULL);
    sigaction(SIGTERM, &sigAction, NULL);
    sigaction(SIGUSR1, &sigAction, NULL);
    sigaction(SIGUSR2, &sigAction, NULL);
    LogRelFlowFunc(("returning\n"));
}

/**
 * Print out a usage message and exit with success.
 */
static void vboxClientUsage(const char *pcszFileName)
{
    RTPrintf("Usage: %s --clipboard|"
#ifdef VBOX_WITH_DRAG_AND_DROP
             "--draganddrop|"
#endif
             "--display|"
# ifdef VBOX_WITH_GUEST_PROPS
             "--checkhostversion|"
#endif
             "--seamless|check3d|"
             "--vmsvga [-d|--nodaemon]\n", pcszFileName);
    RTPrintf("Starts the VirtualBox DRM/X Window System guest services.\n\n");
    RTPrintf("Options:\n");
    RTPrintf("  --clipboard        starts the shared clipboard service\n");
#ifdef VBOX_WITH_DRAG_AND_DROP
    RTPrintf("  --draganddrop      starts the drag and drop service\n");
#endif
    RTPrintf("  --display          starts the display management service\n");
#ifdef VBOX_WITH_GUEST_PROPS
    RTPrintf("  --checkhostversion starts the host version notifier service\n");
#endif
    RTPrintf("  --check3d          tests whether 3D pass-through is enabled\n");
    RTPrintf("  --seamless         starts the seamless windows service\n");
    RTPrintf("  --vmsvga           starts VMSVGA dynamic resizing for DRM or for X11\n");
    RTPrintf("  -f, --foreground   run in the foreground (no daemonizing)\n");
    RTPrintf("  -d, --nodaemon     continues running as a system service\n");
    RTPrintf("  -h, --help         shows this help text\n");
    RTPrintf("  -V, --version      shows version information\n");
    RTPrintf("\n");
}

/**
 * Complains about seeing more than one service specification.
 *
 * @returns RTEXITCODE_SYNTAX.
 */
static int vbclSyntaxOnlyOneService(void)
{
    RTMsgError("More than one service specified! Only one, please.");
    return RTEXITCODE_SYNTAX;
}

/**
 * The main loop for the VBoxClient daemon.
 * @todo Clean up for readability.
 */
int main(int argc, char *argv[])
{
    /* Initialise our runtime before all else. */
    int rc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(rc))
        return RTMsgInitFailure(rc);

    /* This should never be called twice in one process - in fact one Display
     * object should probably never be used from multiple threads anyway. */
    if (!XInitThreads())
        VBClFatalError(("Failed to initialize X11 threads\n"));

    /* Get our file name for usage info and hints. */
    const char *pcszFileName = RTPathFilename(argv[0]);
    if (!pcszFileName)
        pcszFileName = "VBoxClient";

    /* Parse our option(s) */
    /** @todo Use RTGetOpt() if the arguments become more complex. */
    bool fDaemonise = true;
    bool fRespawn = true;
    for (int i = 1; i < argc; ++i)
    {
        if (   !strcmp(argv[i], "-f")
            || !strcmp(argv[i], "--foreground")
            || !strcmp(argv[i], "-d")
            || !strcmp(argv[i], "--nodaemon"))
        {
            /* If the user is running in "no daemon" mode anyway, send critical
             * logging to stdout as well. */
            /** @todo r=bird: Since the release logger isn't created until the service
             *        calls VbglR3InitUser or VbglR3Init or RTLogCreate, this whole
             *        exercise is pointless.  Added --init-vbgl-user and --init-vbgl-full
             *        for getting some work done. */
            PRTLOGGER pReleaseLog = RTLogRelGetDefaultInstance();
            if (pReleaseLog)
                rc = RTLogDestinations(pReleaseLog, "stdout");
            if (pReleaseLog && RT_FAILURE(rc))
                return RTMsgErrorExitFailure("failed to redivert error output, rc=%Rrc", rc);

            fDaemonise = false;
            if (   !strcmp(argv[i], "-f")
                || !strcmp(argv[i], "--foreground"))
                fRespawn = false;
        }
        else if (!strcmp(argv[i], "--no-respawn"))
        {
            fRespawn = false;
        }
        else if (!strcmp(argv[i], "--clipboard"))
        {
            if (g_pService)
                return vbclSyntaxOnlyOneService();
            g_pService = VBClGetClipboardService();
        }
        else if (!strcmp(argv[i], "--display"))
        {
            if (g_pService)
                return vbclSyntaxOnlyOneService();
            g_pService = VBClGetDisplayService();
        }
        else if (!strcmp(argv[i], "--seamless"))
        {
            if (g_pService)
                return vbclSyntaxOnlyOneService();
            g_pService = VBClGetSeamlessService();
        }
        else if (!strcmp(argv[i], "--checkhostversion"))
        {
            if (g_pService)
                return vbclSyntaxOnlyOneService();
            g_pService = VBClGetHostVersionService();
        }
#ifdef VBOX_WITH_DRAG_AND_DROP
        else if (!strcmp(argv[i], "--draganddrop"))
        {
            if (g_pService)
                return vbclSyntaxOnlyOneService();
            g_pService = VBClGetDragAndDropService();
        }
#endif /* VBOX_WITH_DRAG_AND_DROP */
        else if (!strcmp(argv[i], "--check3d"))
        {
            if (g_pService)
                return vbclSyntaxOnlyOneService();
            g_pService = VBClCheck3DService();
        }
        else if (!strcmp(argv[i], "--vmsvga"))
        {
            if (g_pService)
                return vbclSyntaxOnlyOneService();
            g_pService = VBClDisplaySVGAService();
        }
        /* bird: this is just a quick hack to get something out of the LogRel statements in the code. */
        else if (!strcmp(argv[i], "--init-vbgl-user"))
        {
            rc = VbglR3InitUser();
            if (RT_FAILURE(rc))
                return RTMsgErrorExitFailure("VbglR3InitUser failed: %Rrc", rc);
        }
        else if (!strcmp(argv[i], "--init-vbgl-full"))
        {
            rc = VbglR3Init();
            if (RT_FAILURE(rc))
                return RTMsgErrorExitFailure("VbglR3Init failed: %Rrc", rc);
        }
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
        {
            vboxClientUsage(pcszFileName);
            return RTEXITCODE_SUCCESS;
        }
        else if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version"))
        {
            RTPrintf("%sr%s\n", RTBldCfgVersion(), RTBldCfgRevisionStr());
            return RTEXITCODE_SUCCESS;
        }
        else
        {
            RTMsgError("unrecognized option `%s'", argv[i]);
            RTMsgInfo("Try `%s --help' for more information", pcszFileName);
            return RTEXITCODE_SYNTAX;
        }
    }
    if (!g_pService)
    {
        RTMsgError("No service specified. Quitting because nothing to do!");
        return RTEXITCODE_SYNTAX;
    }

    rc = RTCritSectInit(&g_critSect);
    if (RT_FAILURE(rc))
        VBClFatalError(("Initialising critical section failed: %Rrc\n", rc));
    if ((*g_pService)->getPidFilePath)
    {
        rc = RTPathUserHome(g_szPidFile, sizeof(g_szPidFile));
        if (RT_FAILURE(rc))
            VBClFatalError(("Getting home directory for PID file failed: %Rrc\n", rc));
        rc = RTPathAppend(g_szPidFile, sizeof(g_szPidFile),
                          (*g_pService)->getPidFilePath());
        if (RT_FAILURE(rc))
            VBClFatalError(("Creating PID file path failed: %Rrc\n", rc));
        if (fDaemonise)
            rc = VbglR3Daemonize(false /* fNoChDir */, false /* fNoClose */, fRespawn, &cRespawn);
        if (RT_FAILURE(rc))
            VBClFatalError(("Daemonizing failed: %Rrc\n", rc));
        if (g_szPidFile[0])
            rc = VbglR3PidFile(g_szPidFile, &g_hPidFile);
        if (rc == VERR_FILE_LOCK_VIOLATION)  /* Already running. */
            return RTEXITCODE_SUCCESS;
        if (RT_FAILURE(rc))
            VBClFatalError(("Creating PID file failed: %Rrc\n", rc));
    }
    /* Set signal handlers to clean up on exit. */
    vboxClientSetSignalHandlers();
#ifndef VBOXCLIENT_WITHOUT_X11
    /* Set an X11 error handler, so that we don't die when we get unavoidable
     * errors. */
    XSetErrorHandler(vboxClientXLibErrorHandler);
    /* Set an X11 I/O error handler, so that we can shutdown properly on
     * fatal errors. */
    XSetIOErrorHandler(vboxClientXLibIOErrorHandler);
#endif
    rc = (*g_pService)->init(g_pService);
    if (RT_SUCCESS(rc))
    {
        rc = (*g_pService)->run(g_pService, fDaemonise);
        if (RT_FAILURE(rc))
            LogRel2(("Running service failed: %Rrc\n", rc));
    }
    else
    {
        /** @todo r=andy Should we return an appropriate exit code if the service failed to init?
         *               Must be tested carefully with our init scripts first. */
        LogRel2(("Initializing service failed: %Rrc\n", rc));
    }
    VBClCleanUp(false /*fExit*/);
    return RTEXITCODE_SUCCESS;
}

