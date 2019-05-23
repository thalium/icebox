/* $Id: VBoxAutostartUtils.cpp $ */
/** @file
 * VBoxAutostart - VirtualBox Autostart service, start machines during system boot.
 *                 Utils used by the windows and posix frontends.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/Guid.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>

#include <VBox/err.h>

#include <iprt/message.h>
#include <iprt/thread.h>
#include <iprt/stream.h>
#include <iprt/log.h>
#include <iprt/path.h>

#include <algorithm>
#include <list>
#include <string>

#include "VBoxAutostart.h"

using namespace com;

DECLHIDDEN(const char *) machineStateToName(MachineState_T machineState, bool fShort)
{
    switch (machineState)
    {
        case MachineState_PoweredOff:
            return fShort ? "poweroff"             : "powered off";
        case MachineState_Saved:
            return "saved";
        case MachineState_Teleported:
            return "teleported";
        case MachineState_Aborted:
            return "aborted";
        case MachineState_Running:
            return "running";
        case MachineState_Paused:
            return "paused";
        case MachineState_Stuck:
            return fShort ? "gurumeditation"       : "guru meditation";
        case MachineState_Teleporting:
            return "teleporting";
        case MachineState_LiveSnapshotting:
            return fShort ? "livesnapshotting"     : "live snapshotting";
        case MachineState_Starting:
            return "starting";
        case MachineState_Stopping:
            return "stopping";
        case MachineState_Saving:
            return "saving";
        case MachineState_Restoring:
            return "restoring";
        case MachineState_TeleportingPausedVM:
            return fShort ? "teleportingpausedvm"  : "teleporting paused vm";
        case MachineState_TeleportingIn:
            return fShort ? "teleportingin"        : "teleporting (incoming)";
        case MachineState_DeletingSnapshotOnline:
            return fShort ? "deletingsnapshotlive" : "deleting snapshot live";
        case MachineState_DeletingSnapshotPaused:
            return fShort ? "deletingsnapshotlivepaused" : "deleting snapshot live paused";
        case MachineState_OnlineSnapshotting:
            return fShort ? "onlinesnapshotting"   : "online snapshotting";
        case MachineState_RestoringSnapshot:
            return fShort ? "restoringsnapshot"    : "restoring snapshot";
        case MachineState_DeletingSnapshot:
            return fShort ? "deletingsnapshot"     : "deleting snapshot";
        case MachineState_SettingUp:
            return fShort ? "settingup"           : "setting up";
        case MachineState_Snapshotting:
            return "snapshotting";
        default:
            break;
    }
    return "unknown";
}

DECLHIDDEN(void) autostartSvcLogErrorV(const char *pszFormat, va_list va)
{
    if (*pszFormat)
    {
        char *pszMsg = NULL;
        if (RTStrAPrintfV(&pszMsg, pszFormat, va) != -1)
        {
            autostartSvcOsLogStr(pszMsg, AUTOSTARTLOGTYPE_ERROR);
            RTStrFree(pszMsg);
        }
        else
            autostartSvcOsLogStr(pszFormat, AUTOSTARTLOGTYPE_ERROR);
    }
}

DECLHIDDEN(void) autostartSvcLogError(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    autostartSvcLogErrorV(pszFormat, va);
    va_end(va);
}

DECLHIDDEN(void) autostartSvcLogVerboseV(const char *pszFormat, va_list va)
{
    if (*pszFormat)
    {
        char *pszMsg = NULL;
        if (RTStrAPrintfV(&pszMsg, pszFormat, va) != -1)
        {
            autostartSvcOsLogStr(pszMsg, AUTOSTARTLOGTYPE_VERBOSE);
            RTStrFree(pszMsg);
        }
        else
            autostartSvcOsLogStr(pszFormat, AUTOSTARTLOGTYPE_VERBOSE);
    }
}

DECLHIDDEN(void) autostartSvcLogVerbose(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    autostartSvcLogVerboseV(pszFormat, va);
    va_end(va);
}

DECLHIDDEN(void) autostartSvcLogWarningV(const char *pszFormat, va_list va)
{
    if (*pszFormat)
    {
        char *pszMsg = NULL;
        if (RTStrAPrintfV(&pszMsg, pszFormat, va) != -1)
        {
            autostartSvcOsLogStr(pszMsg, AUTOSTARTLOGTYPE_WARNING);
            RTStrFree(pszMsg);
        }
        else
            autostartSvcOsLogStr(pszFormat, AUTOSTARTLOGTYPE_WARNING);
    }
}

DECLHIDDEN(void) autostartSvcLogInfo(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    autostartSvcLogInfoV(pszFormat, va);
    va_end(va);
}

DECLHIDDEN(void) autostartSvcLogInfoV(const char *pszFormat, va_list va)
{
    if (*pszFormat)
    {
        char *pszMsg = NULL;
        if (RTStrAPrintfV(&pszMsg, pszFormat, va) != -1)
        {
            autostartSvcOsLogStr(pszMsg, AUTOSTARTLOGTYPE_INFO);
            RTStrFree(pszMsg);
        }
        else
            autostartSvcOsLogStr(pszFormat, AUTOSTARTLOGTYPE_INFO);
    }
}

DECLHIDDEN(void) autostartSvcLogWarning(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    autostartSvcLogWarningV(pszFormat, va);
    va_end(va);
}

DECLHIDDEN(RTEXITCODE) autostartSvcLogGetOptError(const char *pszAction, int rc, int argc, char **argv, int iArg, PCRTGETOPTUNION pValue)
{
    NOREF(pValue);
    autostartSvcLogError("%s - RTGetOpt failure, %Rrc (%d): %s",
                   pszAction, rc, rc, iArg < argc ? argv[iArg] : "<null>");
    return RTEXITCODE_FAILURE;
}

DECLHIDDEN(RTEXITCODE) autostartSvcLogTooManyArgsError(const char *pszAction, int argc, char **argv, int iArg)
{
    Assert(iArg < argc);
    autostartSvcLogError("%s - Too many arguments: %s", pszAction, argv[iArg]);
    for ( ; iArg < argc; iArg++)
        LogRel(("arg#%i: %s\n", iArg, argv[iArg]));
    return RTEXITCODE_FAILURE;
}

DECLHIDDEN(void) autostartSvcDisplayErrorV(const char *pszFormat, va_list va)
{
    RTStrmPrintf(g_pStdErr, "VBoxSupSvc error: ");
    RTStrmPrintfV(g_pStdErr, pszFormat, va);
    Log(("autostartSvcDisplayErrorV: %s", pszFormat)); /** @todo format it! */
}

DECLHIDDEN(void) autostartSvcDisplayError(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    autostartSvcDisplayErrorV(pszFormat, va);
    va_end(va);
}

DECLHIDDEN(RTEXITCODE) autostartSvcDisplayGetOptError(const char *pszAction, int rc, int argc, char **argv, int iArg,
                                                      PCRTGETOPTUNION pValue)
{
    RT_NOREF(pValue);
    autostartSvcDisplayError("%s - RTGetOpt failure, %Rrc (%d): %s\n",
                       pszAction, rc, rc, iArg < argc ? argv[iArg] : "<null>");
    return RTEXITCODE_FAILURE;
}

DECLHIDDEN(RTEXITCODE) autostartSvcDisplayTooManyArgsError(const char *pszAction, int argc, char **argv, int iArg)
{
    RT_NOREF(argc);
    Assert(iArg < argc);
    autostartSvcDisplayError("%s - Too many arguments: %s\n", pszAction, argv[iArg]);
    return RTEXITCODE_FAILURE;
}

DECLHIDDEN(int) autostartSetup()
{
    autostartSvcOsLogStr("Setting up ...\n", AUTOSTARTLOGTYPE_VERBOSE);

    /*
     * Initialize COM.
     */
    using namespace com;
    HRESULT hrc = com::Initialize();
# ifdef VBOX_WITH_XPCOM
    if (hrc == NS_ERROR_FILE_ACCESS_DENIED)
    {
        char szHome[RTPATH_MAX] = "";
        com::GetVBoxUserHomeDirectory(szHome, sizeof(szHome));
        return RTMsgErrorExit(RTEXITCODE_FAILURE,
               "Failed to initialize COM because the global settings directory '%s' is not accessible!", szHome);
    }
# endif
    if (FAILED(hrc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed to initialize COM (%Rhrc)!", hrc);

    hrc = g_pVirtualBoxClient.createInprocObject(CLSID_VirtualBoxClient);
    if (FAILED(hrc))
    {
        RTMsgError("Failed to create the VirtualBoxClient object (%Rhrc)!", hrc);
        com::ErrorInfo info;
        if (!info.isFullAvailable() && !info.isBasicAvailable())
        {
            com::GluePrintRCMessage(hrc);
            RTMsgError("Most likely, the VirtualBox COM server is not running or failed to start.");
        }
        else
            com::GluePrintErrorInfo(info);
        return RTEXITCODE_FAILURE;
    }

    /*
     * Setup VirtualBox + session interfaces.
     */
    HRESULT rc = g_pVirtualBoxClient->COMGETTER(VirtualBox)(g_pVirtualBox.asOutParam());
    if (SUCCEEDED(rc))
    {
        rc = g_pSession.createInprocObject(CLSID_Session);
        if (FAILED(rc))
            RTMsgError("Failed to create a session object (rc=%Rhrc)!", rc);
    }
    else
        RTMsgError("Failed to get VirtualBox object (rc=%Rhrc)!", rc);

    if (FAILED(rc))
        return VERR_COM_OBJECT_NOT_FOUND;

    return VINF_SUCCESS;
}

DECLHIDDEN(void) autostartShutdown()
{
    autostartSvcOsLogStr("Shutting down ...\n", AUTOSTARTLOGTYPE_VERBOSE);

    g_pSession.setNull();
    g_pVirtualBox.setNull();
    g_pVirtualBoxClient.setNull();
    com::Shutdown();
}

