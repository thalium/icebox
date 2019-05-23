/* $Id: VBoxManageMisc.cpp $ */
/** @file
 * VBoxManage - VirtualBox's command-line interface.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#ifndef VBOX_ONLY_DOCS
# include <VBox/com/com.h>
# include <VBox/com/string.h>
# include <VBox/com/Guid.h>
# include <VBox/com/array.h>
# include <VBox/com/ErrorInfo.h>
# include <VBox/com/errorprint.h>
# include <VBox/com/VirtualBox.h>
#endif /* !VBOX_ONLY_DOCS */

#include <iprt/asm.h>
#include <iprt/buildconfig.h>
#include <iprt/cidr.h>
#include <iprt/ctype.h>
#include <iprt/dir.h>
#include <iprt/env.h>
#include <VBox/err.h>
#include <iprt/file.h>
#include <iprt/sha.h>
#include <iprt/initterm.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/cpp/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/stdarg.h>
#include <iprt/thread.h>
#include <iprt/uuid.h>
#include <iprt/getopt.h>
#include <iprt/ctype.h>
#include <VBox/version.h>
#include <VBox/log.h>

#include "VBoxManage.h"

#include <list>

using namespace com;



RTEXITCODE handleRegisterVM(HandlerArg *a)
{
    HRESULT rc;

    if (a->argc != 1)
        return errorSyntax(USAGE_REGISTERVM, "Incorrect number of parameters");

    ComPtr<IMachine> machine;
    /** @todo Ugly hack to get both the API interpretation of relative paths
     * and the client's interpretation of relative paths. Remove after the API
     * has been redesigned. */
    rc = a->virtualBox->OpenMachine(Bstr(a->argv[0]).raw(),
                                    machine.asOutParam());
    if (rc == VBOX_E_FILE_ERROR)
    {
        char szVMFileAbs[RTPATH_MAX] = "";
        int vrc = RTPathAbs(a->argv[0], szVMFileAbs, sizeof(szVMFileAbs));
        if (RT_FAILURE(vrc))
            return RTMsgErrorExit(RTEXITCODE_FAILURE, "Cannot convert filename \"%s\" to absolute path: %Rrc", a->argv[0], vrc);
        CHECK_ERROR(a->virtualBox, OpenMachine(Bstr(szVMFileAbs).raw(),
                                               machine.asOutParam()));
    }
    else if (FAILED(rc))
        CHECK_ERROR(a->virtualBox, OpenMachine(Bstr(a->argv[0]).raw(),
                                               machine.asOutParam()));
    if (SUCCEEDED(rc))
    {
        ASSERT(machine);
        CHECK_ERROR(a->virtualBox, RegisterMachine(machine));
    }
    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static const RTGETOPTDEF g_aUnregisterVMOptions[] =
{
    { "--delete",       'd', RTGETOPT_REQ_NOTHING },
    { "-delete",        'd', RTGETOPT_REQ_NOTHING },    // deprecated
};

RTEXITCODE handleUnregisterVM(HandlerArg *a)
{
    HRESULT rc;
    const char *VMName = NULL;
    bool fDelete = false;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aUnregisterVMOptions, RT_ELEMENTS(g_aUnregisterVMOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'd':   // --delete
                fDelete = true;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!VMName)
                    VMName = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_UNREGISTERVM, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_UNREGISTERVM, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_UNREGISTERVM, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_UNREGISTERVM, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_UNREGISTERVM, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_UNREGISTERVM, "error: %Rrs", c);
        }
    }

    /* check for required options */
    if (!VMName)
        return errorSyntax(USAGE_UNREGISTERVM, "VM name required");

    ComPtr<IMachine> machine;
    CHECK_ERROR_RET(a->virtualBox, FindMachine(Bstr(VMName).raw(),
                                               machine.asOutParam()),
                    RTEXITCODE_FAILURE);
    SafeIfaceArray<IMedium> aMedia;
    CHECK_ERROR_RET(machine, Unregister(CleanupMode_DetachAllReturnHardDisksOnly,
                                        ComSafeArrayAsOutParam(aMedia)),
                    RTEXITCODE_FAILURE);
    if (fDelete)
    {
        ComPtr<IProgress> pProgress;
        CHECK_ERROR_RET(machine, DeleteConfig(ComSafeArrayAsInParam(aMedia), pProgress.asOutParam()),
                        RTEXITCODE_FAILURE);

        rc = showProgress(pProgress);
        CHECK_PROGRESS_ERROR_RET(pProgress, ("Machine delete failed"), RTEXITCODE_FAILURE);
    }
    else
    {
        /* Note that the IMachine::Unregister method will return the medium
         * reference in a sane order, which means that closing will normally
         * succeed, unless there is still another machine which uses the
         * medium. No harm done if we ignore the error. */
        for (size_t i = 0; i < aMedia.size(); i++)
        {
            IMedium *pMedium = aMedia[i];
            if (pMedium)
                rc = pMedium->Close();
        }
        rc = S_OK;
    }
    return RTEXITCODE_SUCCESS;
}

static const RTGETOPTDEF g_aCreateVMOptions[] =
{
    { "--name",           'n', RTGETOPT_REQ_STRING },
    { "-name",            'n', RTGETOPT_REQ_STRING },
    { "--groups",         'g', RTGETOPT_REQ_STRING },
    { "--basefolder",     'p', RTGETOPT_REQ_STRING },
    { "-basefolder",      'p', RTGETOPT_REQ_STRING },
    { "--ostype",         'o', RTGETOPT_REQ_STRING },
    { "-ostype",          'o', RTGETOPT_REQ_STRING },
    { "--uuid",           'u', RTGETOPT_REQ_UUID },
    { "-uuid",            'u', RTGETOPT_REQ_UUID },
    { "--register",       'r', RTGETOPT_REQ_NOTHING },
    { "-register",        'r', RTGETOPT_REQ_NOTHING },
};

RTEXITCODE handleCreateVM(HandlerArg *a)
{
    HRESULT rc;
    Bstr bstrBaseFolder;
    Bstr bstrName;
    Bstr bstrOsTypeId;
    Bstr bstrUuid;
    bool fRegister = false;
    com::SafeArray<BSTR> groups;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aCreateVMOptions, RT_ELEMENTS(g_aCreateVMOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'n':   // --name
                bstrName = ValueUnion.psz;
                break;

            case 'g':   // --groups
                parseGroups(ValueUnion.psz, &groups);
                break;

            case 'p':   // --basefolder
                bstrBaseFolder = ValueUnion.psz;
                break;

            case 'o':   // --ostype
                bstrOsTypeId = ValueUnion.psz;
                break;

            case 'u':   // --uuid
                bstrUuid = Guid(ValueUnion.Uuid).toUtf16().raw();
                break;

            case 'r':   // --register
                fRegister = true;
                break;

            default:
                return errorGetOpt(USAGE_CREATEVM, c, &ValueUnion);
        }
    }

    /* check for required options */
    if (bstrName.isEmpty())
        return errorSyntax(USAGE_CREATEVM, "Parameter --name is required");

    do
    {
        Bstr createFlags;
        if (!bstrUuid.isEmpty())
            createFlags = BstrFmt("UUID=%ls", bstrUuid.raw());
        Bstr bstrPrimaryGroup;
        if (groups.size())
            bstrPrimaryGroup = groups[0];
        Bstr bstrSettingsFile;
        CHECK_ERROR_BREAK(a->virtualBox,
                          ComposeMachineFilename(bstrName.raw(),
                                                 bstrPrimaryGroup.raw(),
                                                 createFlags.raw(),
                                                 bstrBaseFolder.raw(),
                                                 bstrSettingsFile.asOutParam()));
        ComPtr<IMachine> machine;
        CHECK_ERROR_BREAK(a->virtualBox,
                          CreateMachine(bstrSettingsFile.raw(),
                                        bstrName.raw(),
                                        ComSafeArrayAsInParam(groups),
                                        bstrOsTypeId.raw(),
                                        createFlags.raw(),
                                        machine.asOutParam()));

        CHECK_ERROR_BREAK(machine, SaveSettings());
        if (fRegister)
        {
            CHECK_ERROR_BREAK(a->virtualBox, RegisterMachine(machine));
        }
        Bstr uuid;
        CHECK_ERROR_BREAK(machine, COMGETTER(Id)(uuid.asOutParam()));
        Bstr settingsFile;
        CHECK_ERROR_BREAK(machine, COMGETTER(SettingsFilePath)(settingsFile.asOutParam()));
        RTPrintf("Virtual machine '%ls' is created%s.\n"
                 "UUID: %s\n"
                 "Settings file: '%ls'\n",
                 bstrName.raw(), fRegister ? " and registered" : "",
                 Utf8Str(uuid).c_str(), settingsFile.raw());
    }
    while (0);

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static const RTGETOPTDEF g_aCloneVMOptions[] =
{
    { "--snapshot",       's', RTGETOPT_REQ_STRING },
    { "--name",           'n', RTGETOPT_REQ_STRING },
    { "--groups",         'g', RTGETOPT_REQ_STRING },
    { "--mode",           'm', RTGETOPT_REQ_STRING },
    { "--options",        'o', RTGETOPT_REQ_STRING },
    { "--register",       'r', RTGETOPT_REQ_NOTHING },
    { "--basefolder",     'p', RTGETOPT_REQ_STRING },
    { "--uuid",           'u', RTGETOPT_REQ_UUID },
};

static int parseCloneMode(const char *psz, CloneMode_T *pMode)
{
    if (!RTStrICmp(psz, "machine"))
        *pMode = CloneMode_MachineState;
    else if (!RTStrICmp(psz, "machineandchildren"))
        *pMode = CloneMode_MachineAndChildStates;
    else if (!RTStrICmp(psz, "all"))
        *pMode = CloneMode_AllStates;
    else
        return VERR_PARSE_ERROR;

    return VINF_SUCCESS;
}

static int parseCloneOptions(const char *psz, com::SafeArray<CloneOptions_T> *options)
{
    int rc = VINF_SUCCESS;
    while (psz && *psz && RT_SUCCESS(rc))
    {
        size_t len;
        const char *pszComma = strchr(psz, ',');
        if (pszComma)
            len = pszComma - psz;
        else
            len = strlen(psz);
        if (len > 0)
        {
            if (!RTStrNICmp(psz, "KeepAllMACs", len))
                options->push_back(CloneOptions_KeepAllMACs);
            else if (!RTStrNICmp(psz, "KeepNATMACs", len))
                options->push_back(CloneOptions_KeepNATMACs);
            else if (!RTStrNICmp(psz, "KeepDiskNames", len))
                options->push_back(CloneOptions_KeepDiskNames);
            else if (   !RTStrNICmp(psz, "Link", len)
                     || !RTStrNICmp(psz, "Linked", len))
                options->push_back(CloneOptions_Link);
            else
                rc = VERR_PARSE_ERROR;
        }
        if (pszComma)
            psz += len + 1;
        else
            psz += len;
    }

    return rc;
}

RTEXITCODE handleCloneVM(HandlerArg *a)
{
    HRESULT                        rc;
    const char                    *pszSrcName       = NULL;
    const char                    *pszSnapshotName  = NULL;
    CloneMode_T                    mode             = CloneMode_MachineState;
    com::SafeArray<CloneOptions_T> options;
    const char                    *pszTrgName       = NULL;
    const char                    *pszTrgBaseFolder = NULL;
    bool                           fRegister        = false;
    Bstr                           bstrUuid;
    com::SafeArray<BSTR> groups;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aCloneVMOptions, RT_ELEMENTS(g_aCloneVMOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 's':   // --snapshot
                pszSnapshotName = ValueUnion.psz;
                break;

            case 'n':   // --name
                pszTrgName = ValueUnion.psz;
                break;

            case 'g':   // --groups
                parseGroups(ValueUnion.psz, &groups);
                break;

            case 'p':   // --basefolder
                pszTrgBaseFolder = ValueUnion.psz;
                break;

            case 'm':   // --mode
                if (RT_FAILURE(parseCloneMode(ValueUnion.psz, &mode)))
                    return errorArgument("Invalid clone mode '%s'\n", ValueUnion.psz);
                break;

            case 'o':   // --options
                if (RT_FAILURE(parseCloneOptions(ValueUnion.psz, &options)))
                    return errorArgument("Invalid clone options '%s'\n", ValueUnion.psz);
                break;

            case 'u':   // --uuid
                bstrUuid = Guid(ValueUnion.Uuid).toUtf16().raw();
                break;

            case 'r':   // --register
                fRegister = true;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!pszSrcName)
                    pszSrcName = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_CLONEVM, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                return errorGetOpt(USAGE_CLONEVM, c, &ValueUnion);
        }
    }

    /* Check for required options */
    if (!pszSrcName)
        return errorSyntax(USAGE_CLONEVM, "VM name required");

    /* Get the machine object */
    ComPtr<IMachine> srcMachine;
    CHECK_ERROR_RET(a->virtualBox, FindMachine(Bstr(pszSrcName).raw(),
                                               srcMachine.asOutParam()),
                    RTEXITCODE_FAILURE);

    /* If a snapshot name/uuid was given, get the particular machine of this
     * snapshot. */
    if (pszSnapshotName)
    {
        ComPtr<ISnapshot> srcSnapshot;
        CHECK_ERROR_RET(srcMachine, FindSnapshot(Bstr(pszSnapshotName).raw(),
                                                 srcSnapshot.asOutParam()),
                        RTEXITCODE_FAILURE);
        CHECK_ERROR_RET(srcSnapshot, COMGETTER(Machine)(srcMachine.asOutParam()),
                        RTEXITCODE_FAILURE);
    }

    /* Default name necessary? */
    if (!pszTrgName)
        pszTrgName = RTStrAPrintf2("%s Clone", pszSrcName);

    Bstr createFlags;
    if (!bstrUuid.isEmpty())
        createFlags = BstrFmt("UUID=%ls", bstrUuid.raw());
    Bstr bstrPrimaryGroup;
    if (groups.size())
        bstrPrimaryGroup = groups[0];
    Bstr bstrSettingsFile;
    CHECK_ERROR_RET(a->virtualBox,
                    ComposeMachineFilename(Bstr(pszTrgName).raw(),
                                           bstrPrimaryGroup.raw(),
                                           createFlags.raw(),
                                           Bstr(pszTrgBaseFolder).raw(),
                                           bstrSettingsFile.asOutParam()),
                    RTEXITCODE_FAILURE);

    ComPtr<IMachine> trgMachine;
    CHECK_ERROR_RET(a->virtualBox, CreateMachine(bstrSettingsFile.raw(),
                                                 Bstr(pszTrgName).raw(),
                                                 ComSafeArrayAsInParam(groups),
                                                 NULL,
                                                 createFlags.raw(),
                                                 trgMachine.asOutParam()),
                    RTEXITCODE_FAILURE);

    /* Start the cloning */
    ComPtr<IProgress> progress;
    CHECK_ERROR_RET(srcMachine, CloneTo(trgMachine,
                                        mode,
                                        ComSafeArrayAsInParam(options),
                                        progress.asOutParam()),
                    RTEXITCODE_FAILURE);
    rc = showProgress(progress);
    CHECK_PROGRESS_ERROR_RET(progress, ("Clone VM failed"), RTEXITCODE_FAILURE);

    if (fRegister)
        CHECK_ERROR_RET(a->virtualBox, RegisterMachine(trgMachine), RTEXITCODE_FAILURE);

    Bstr bstrNewName;
    CHECK_ERROR_RET(trgMachine, COMGETTER(Name)(bstrNewName.asOutParam()), RTEXITCODE_FAILURE);
    RTPrintf("Machine has been successfully cloned as \"%ls\"\n", bstrNewName.raw());

    return RTEXITCODE_SUCCESS;
}

RTEXITCODE handleStartVM(HandlerArg *a)
{
    HRESULT rc = S_OK;
    std::list<const char *> VMs;
    Bstr sessionType;
    Utf8Str strEnv;

#if defined(RT_OS_LINUX) || defined(RT_OS_SOLARIS)
    /* make sure the VM process will by default start on the same display as VBoxManage */
    {
        const char *pszDisplay = RTEnvGet("DISPLAY");
        if (pszDisplay)
            strEnv = Utf8StrFmt("DISPLAY=%s\n", pszDisplay);
        const char *pszXAuth = RTEnvGet("XAUTHORITY");
        if (pszXAuth)
            strEnv.append(Utf8StrFmt("XAUTHORITY=%s\n", pszXAuth));
    }
#endif

    static const RTGETOPTDEF s_aStartVMOptions[] =
    {
        { "--type",         't', RTGETOPT_REQ_STRING },
        { "-type",          't', RTGETOPT_REQ_STRING },     // deprecated
        { "--putenv",       'E', RTGETOPT_REQ_STRING },
    };
    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, s_aStartVMOptions, RT_ELEMENTS(s_aStartVMOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 't':   // --type
                if (!RTStrICmp(ValueUnion.psz, "gui"))
                {
                    sessionType = "gui";
                }
#ifdef VBOX_WITH_VBOXSDL
                else if (!RTStrICmp(ValueUnion.psz, "sdl"))
                {
                    sessionType = "sdl";
                }
#endif
#ifdef VBOX_WITH_HEADLESS
                else if (!RTStrICmp(ValueUnion.psz, "capture"))
                {
                    sessionType = "capture";
                }
                else if (!RTStrICmp(ValueUnion.psz, "headless"))
                {
                    sessionType = "headless";
                }
#endif
                else
                    sessionType = ValueUnion.psz;
                break;

            case 'E':   // --putenv
                if (!RTStrStr(ValueUnion.psz, "\n"))
                    strEnv.append(Utf8StrFmt("%s\n", ValueUnion.psz));
                else
                    return errorSyntax(USAGE_STARTVM, "Parameter to option --putenv must not contain any newline character");
                break;

            case VINF_GETOPT_NOT_OPTION:
                VMs.push_back(ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_STARTVM, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_STARTVM, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_STARTVM, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_STARTVM, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_STARTVM, "error: %Rrs", c);
        }
    }

    /* check for required options */
    if (VMs.empty())
        return errorSyntax(USAGE_STARTVM, "at least one VM name or uuid required");

    for (std::list<const char *>::const_iterator it = VMs.begin();
         it != VMs.end();
         ++it)
    {
        HRESULT rc2 = rc;
        const char *pszVM = *it;
        ComPtr<IMachine> machine;
        CHECK_ERROR(a->virtualBox, FindMachine(Bstr(pszVM).raw(),
                                               machine.asOutParam()));
        if (machine)
        {
            ComPtr<IProgress> progress;
            CHECK_ERROR(machine, LaunchVMProcess(a->session, sessionType.raw(),
                                                 Bstr(strEnv).raw(), progress.asOutParam()));
            if (SUCCEEDED(rc) && !progress.isNull())
            {
                RTPrintf("Waiting for VM \"%s\" to power on...\n", pszVM);
                CHECK_ERROR(progress, WaitForCompletion(-1));
                if (SUCCEEDED(rc))
                {
                    BOOL completed = true;
                    CHECK_ERROR(progress, COMGETTER(Completed)(&completed));
                    if (SUCCEEDED(rc))
                    {
                        ASSERT(completed);

                        LONG iRc;
                        CHECK_ERROR(progress, COMGETTER(ResultCode)(&iRc));
                        if (SUCCEEDED(rc))
                        {
                            if (SUCCEEDED(iRc))
                                RTPrintf("VM \"%s\" has been successfully started.\n", pszVM);
                            else
                            {
                                ProgressErrorInfo info(progress);
                                com::GluePrintErrorInfo(info);
                            }
                            rc = iRc;
                        }
                    }
                }
            }
        }

        /* it's important to always close sessions */
        a->session->UnlockMachine();

        /* make sure that we remember the failed state */
        if (FAILED(rc2))
            rc = rc2;
    }

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

RTEXITCODE handleDiscardState(HandlerArg *a)
{
    HRESULT rc;

    if (a->argc != 1)
        return errorSyntax(USAGE_DISCARDSTATE, "Incorrect number of parameters");

    ComPtr<IMachine> machine;
    CHECK_ERROR(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                           machine.asOutParam()));
    if (machine)
    {
        do
        {
            /* we have to open a session for this task */
            CHECK_ERROR_BREAK(machine, LockMachine(a->session, LockType_Write));
            do
            {
                ComPtr<IMachine> sessionMachine;
                CHECK_ERROR_BREAK(a->session, COMGETTER(Machine)(sessionMachine.asOutParam()));
                CHECK_ERROR_BREAK(sessionMachine, DiscardSavedState(true /* fDeleteFile */));
            } while (0);
            CHECK_ERROR_BREAK(a->session, UnlockMachine());
        } while (0);
    }

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

RTEXITCODE handleAdoptState(HandlerArg *a)
{
    HRESULT rc;

    if (a->argc != 2)
        return errorSyntax(USAGE_ADOPTSTATE, "Incorrect number of parameters");

    ComPtr<IMachine> machine;
    CHECK_ERROR(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                           machine.asOutParam()));
    if (machine)
    {
        char szStateFileAbs[RTPATH_MAX] = "";
        int vrc = RTPathAbs(a->argv[1], szStateFileAbs, sizeof(szStateFileAbs));
        if (RT_FAILURE(vrc))
            return RTMsgErrorExit(RTEXITCODE_FAILURE, "Cannot convert filename \"%s\" to absolute path: %Rrc", a->argv[0], vrc);

        do
        {
            /* we have to open a session for this task */
            CHECK_ERROR_BREAK(machine, LockMachine(a->session, LockType_Write));
            do
            {
                ComPtr<IMachine> sessionMachine;
                CHECK_ERROR_BREAK(a->session, COMGETTER(Machine)(sessionMachine.asOutParam()));
                CHECK_ERROR_BREAK(sessionMachine, AdoptSavedState(Bstr(szStateFileAbs).raw()));
            } while (0);
            CHECK_ERROR_BREAK(a->session, UnlockMachine());
        } while (0);
    }

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

RTEXITCODE handleGetExtraData(HandlerArg *a)
{
    HRESULT rc = S_OK;

    if (a->argc != 2)
        return errorSyntax(USAGE_GETEXTRADATA, "Incorrect number of parameters");

    /* global data? */
    if (!strcmp(a->argv[0], "global"))
    {
        /* enumeration? */
        if (!strcmp(a->argv[1], "enumerate"))
        {
            SafeArray<BSTR> aKeys;
            CHECK_ERROR(a->virtualBox, GetExtraDataKeys(ComSafeArrayAsOutParam(aKeys)));

            for (size_t i = 0;
                 i < aKeys.size();
                 ++i)
            {
                Bstr bstrKey(aKeys[i]);
                Bstr bstrValue;
                CHECK_ERROR(a->virtualBox, GetExtraData(bstrKey.raw(),
                                                        bstrValue.asOutParam()));

                RTPrintf("Key: %ls, Value: %ls\n", bstrKey.raw(), bstrValue.raw());
            }
        }
        else
        {
            Bstr value;
            CHECK_ERROR(a->virtualBox, GetExtraData(Bstr(a->argv[1]).raw(),
                                                    value.asOutParam()));
            if (!value.isEmpty())
                RTPrintf("Value: %ls\n", value.raw());
            else
                RTPrintf("No value set!\n");
        }
    }
    else
    {
        ComPtr<IMachine> machine;
        CHECK_ERROR(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                               machine.asOutParam()));
        if (machine)
        {
            /* enumeration? */
            if (!strcmp(a->argv[1], "enumerate"))
            {
                SafeArray<BSTR> aKeys;
                CHECK_ERROR(machine, GetExtraDataKeys(ComSafeArrayAsOutParam(aKeys)));

                for (size_t i = 0;
                    i < aKeys.size();
                    ++i)
                {
                    Bstr bstrKey(aKeys[i]);
                    Bstr bstrValue;
                    CHECK_ERROR(machine, GetExtraData(bstrKey.raw(),
                                                      bstrValue.asOutParam()));

                    RTPrintf("Key: %ls, Value: %ls\n", bstrKey.raw(), bstrValue.raw());
                }
            }
            else
            {
                Bstr value;
                CHECK_ERROR(machine, GetExtraData(Bstr(a->argv[1]).raw(),
                                                  value.asOutParam()));
                if (!value.isEmpty())
                    RTPrintf("Value: %ls\n", value.raw());
                else
                    RTPrintf("No value set!\n");
            }
        }
    }
    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

RTEXITCODE handleSetExtraData(HandlerArg *a)
{
    HRESULT rc = S_OK;

    if (a->argc < 2)
        return errorSyntax(USAGE_SETEXTRADATA, "Not enough parameters");

    /* global data? */
    if (!strcmp(a->argv[0], "global"))
    {
        /** @todo passing NULL is deprecated */
        if (a->argc < 3)
            CHECK_ERROR(a->virtualBox, SetExtraData(Bstr(a->argv[1]).raw(),
                                                    NULL));
        else if (a->argc == 3)
            CHECK_ERROR(a->virtualBox, SetExtraData(Bstr(a->argv[1]).raw(),
                                                    Bstr(a->argv[2]).raw()));
        else
            return errorSyntax(USAGE_SETEXTRADATA, "Too many parameters");
    }
    else
    {
        ComPtr<IMachine> machine;
        CHECK_ERROR(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                               machine.asOutParam()));
        if (machine)
        {
            /* open an existing session for the VM */
            CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Shared), RTEXITCODE_FAILURE);
            /* get the session machine */
            ComPtr<IMachine> sessionMachine;
            CHECK_ERROR_RET(a->session, COMGETTER(Machine)(sessionMachine.asOutParam()), RTEXITCODE_FAILURE);
            /** @todo passing NULL is deprecated */
            if (a->argc < 3)
                CHECK_ERROR(sessionMachine, SetExtraData(Bstr(a->argv[1]).raw(),
                                                         NULL));
            else if (a->argc == 3)
                CHECK_ERROR(sessionMachine, SetExtraData(Bstr(a->argv[1]).raw(),
                                                         Bstr(a->argv[2]).raw()));
            else
                return errorSyntax(USAGE_SETEXTRADATA, "Too many parameters");
        }
    }
    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

RTEXITCODE handleSetProperty(HandlerArg *a)
{
    HRESULT rc;

    /* there must be two arguments: property name and value */
    if (a->argc != 2)
        return errorSyntax(USAGE_SETPROPERTY, "Incorrect number of parameters");

    ComPtr<ISystemProperties> systemProperties;
    a->virtualBox->COMGETTER(SystemProperties)(systemProperties.asOutParam());

    if (!strcmp(a->argv[0], "machinefolder"))
    {
        /* reset to default? */
        if (!strcmp(a->argv[1], "default"))
            CHECK_ERROR(systemProperties, COMSETTER(DefaultMachineFolder)(NULL));
        else
            CHECK_ERROR(systemProperties, COMSETTER(DefaultMachineFolder)(Bstr(a->argv[1]).raw()));
    }
    else if (!strcmp(a->argv[0], "hwvirtexclusive"))
    {
        bool   fHwVirtExclusive;

        if (!strcmp(a->argv[1], "on"))
            fHwVirtExclusive = true;
        else if (!strcmp(a->argv[1], "off"))
            fHwVirtExclusive = false;
        else
            return errorArgument("Invalid hwvirtexclusive argument '%s'", a->argv[1]);
        CHECK_ERROR(systemProperties, COMSETTER(ExclusiveHwVirt)(fHwVirtExclusive));
    }
    else if (   !strcmp(a->argv[0], "vrdeauthlibrary")
             || !strcmp(a->argv[0], "vrdpauthlibrary"))
    {
        if (!strcmp(a->argv[0], "vrdpauthlibrary"))
            RTStrmPrintf(g_pStdErr, "Warning: 'vrdpauthlibrary' is deprecated. Use 'vrdeauthlibrary'.\n");

        /* reset to default? */
        if (!strcmp(a->argv[1], "default"))
            CHECK_ERROR(systemProperties, COMSETTER(VRDEAuthLibrary)(NULL));
        else
            CHECK_ERROR(systemProperties, COMSETTER(VRDEAuthLibrary)(Bstr(a->argv[1]).raw()));
    }
    else if (!strcmp(a->argv[0], "websrvauthlibrary"))
    {
        /* reset to default? */
        if (!strcmp(a->argv[1], "default"))
            CHECK_ERROR(systemProperties, COMSETTER(WebServiceAuthLibrary)(NULL));
        else
            CHECK_ERROR(systemProperties, COMSETTER(WebServiceAuthLibrary)(Bstr(a->argv[1]).raw()));
    }
    else if (!strcmp(a->argv[0], "vrdeextpack"))
    {
        /* disable? */
        if (!strcmp(a->argv[1], "null"))
            CHECK_ERROR(systemProperties, COMSETTER(DefaultVRDEExtPack)(NULL));
        else
            CHECK_ERROR(systemProperties, COMSETTER(DefaultVRDEExtPack)(Bstr(a->argv[1]).raw()));
    }
    else if (!strcmp(a->argv[0], "loghistorycount"))
    {
        uint32_t uVal;
        int vrc;
        vrc = RTStrToUInt32Ex(a->argv[1], NULL, 0, &uVal);
        if (vrc != VINF_SUCCESS)
            return errorArgument("Error parsing Log history count '%s'", a->argv[1]);
        CHECK_ERROR(systemProperties, COMSETTER(LogHistoryCount)(uVal));
    }
    else if (!strcmp(a->argv[0], "autostartdbpath"))
    {
        /* disable? */
        if (!strcmp(a->argv[1], "null"))
            CHECK_ERROR(systemProperties, COMSETTER(AutostartDatabasePath)(NULL));
        else
            CHECK_ERROR(systemProperties, COMSETTER(AutostartDatabasePath)(Bstr(a->argv[1]).raw()));
    }
    else if (!strcmp(a->argv[0], "defaultfrontend"))
    {
        Bstr bstrDefaultFrontend(a->argv[1]);
        if (!strcmp(a->argv[1], "default"))
            bstrDefaultFrontend.setNull();
        CHECK_ERROR(systemProperties, COMSETTER(DefaultFrontend)(bstrDefaultFrontend.raw()));
    }
    else if (!strcmp(a->argv[0], "logginglevel"))
    {
        Bstr bstrLoggingLevel(a->argv[1]);
        if (!strcmp(a->argv[1], "default"))
            bstrLoggingLevel.setNull();
        CHECK_ERROR(systemProperties, COMSETTER(LoggingLevel)(bstrLoggingLevel.raw()));
    }
    else
        return errorSyntax(USAGE_SETPROPERTY, "Invalid parameter '%s'", a->argv[0]);

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

RTEXITCODE handleSharedFolder(HandlerArg *a)
{
    HRESULT rc;

    /* we need at least a command and target */
    if (a->argc < 2)
        return errorSyntax(USAGE_SHAREDFOLDER, "Not enough parameters");

    const char *pszMachineName = a->argv[1];
    ComPtr<IMachine> machine;
    CHECK_ERROR(a->virtualBox, FindMachine(Bstr(pszMachineName).raw(), machine.asOutParam()));
    if (!machine)
        return RTEXITCODE_FAILURE;

    if (!strcmp(a->argv[0], "add"))
    {
        /* we need at least four more parameters */
        if (a->argc < 5)
            return errorSyntax(USAGE_SHAREDFOLDER_ADD, "Not enough parameters");

        char *name = NULL;
        char *hostpath = NULL;
        bool fTransient = false;
        bool fWritable = true;
        bool fAutoMount = false;

        for (int i = 2; i < a->argc; i++)
        {
            if (   !strcmp(a->argv[i], "--name")
                || !strcmp(a->argv[i], "-name"))
            {
                if (a->argc <= i + 1 || !*a->argv[i+1])
                    return errorArgument("Missing argument to '%s'", a->argv[i]);
                i++;
                name = a->argv[i];
            }
            else if (   !strcmp(a->argv[i], "--hostpath")
                     || !strcmp(a->argv[i], "-hostpath"))
            {
                if (a->argc <= i + 1 || !*a->argv[i+1])
                    return errorArgument("Missing argument to '%s'", a->argv[i]);
                i++;
                hostpath = a->argv[i];
            }
            else if (   !strcmp(a->argv[i], "--readonly")
                     || !strcmp(a->argv[i], "-readonly"))
            {
                fWritable = false;
            }
            else if (   !strcmp(a->argv[i], "--transient")
                     || !strcmp(a->argv[i], "-transient"))
            {
                fTransient = true;
            }
            else if (   !strcmp(a->argv[i], "--automount")
                     || !strcmp(a->argv[i], "-automount"))
            {
                fAutoMount = true;
            }
            else
                return errorSyntax(USAGE_SHAREDFOLDER_ADD, "Invalid parameter '%s'", Utf8Str(a->argv[i]).c_str());
        }

        if (NULL != strstr(name, " "))
            return errorSyntax(USAGE_SHAREDFOLDER_ADD, "No spaces allowed in parameter '-name'!");

        /* required arguments */
        if (!name || !hostpath)
        {
            return errorSyntax(USAGE_SHAREDFOLDER_ADD, "Parameters --name and --hostpath are required");
        }

        if (fTransient)
        {
            ComPtr<IConsole> console;

            /* open an existing session for the VM */
            CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Shared), RTEXITCODE_FAILURE);

            /* get the session machine */
            ComPtr<IMachine> sessionMachine;
            CHECK_ERROR_RET(a->session, COMGETTER(Machine)(sessionMachine.asOutParam()), RTEXITCODE_FAILURE);

            /* get the session console */
            CHECK_ERROR_RET(a->session, COMGETTER(Console)(console.asOutParam()), RTEXITCODE_FAILURE);
            if (console.isNull())
                return RTMsgErrorExit(RTEXITCODE_FAILURE,
                                      "Machine '%s' is not currently running.\n", pszMachineName);

            CHECK_ERROR(console, CreateSharedFolder(Bstr(name).raw(),
                                                    Bstr(hostpath).raw(),
                                                    fWritable, fAutoMount));
            a->session->UnlockMachine();
        }
        else
        {
            /* open a session for the VM */
            CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Write), RTEXITCODE_FAILURE);

            /* get the mutable session machine */
            ComPtr<IMachine> sessionMachine;
            a->session->COMGETTER(Machine)(sessionMachine.asOutParam());

            CHECK_ERROR(sessionMachine, CreateSharedFolder(Bstr(name).raw(),
                                                           Bstr(hostpath).raw(),
                                                           fWritable, fAutoMount));
            if (SUCCEEDED(rc))
                CHECK_ERROR(sessionMachine, SaveSettings());

            a->session->UnlockMachine();
        }
    }
    else if (!strcmp(a->argv[0], "remove"))
    {
        /* we need at least two more parameters */
        if (a->argc < 3)
            return errorSyntax(USAGE_SHAREDFOLDER_REMOVE, "Not enough parameters");

        char *name = NULL;
        bool fTransient = false;

        for (int i = 2; i < a->argc; i++)
        {
            if (   !strcmp(a->argv[i], "--name")
                || !strcmp(a->argv[i], "-name"))
            {
                if (a->argc <= i + 1 || !*a->argv[i+1])
                    return errorArgument("Missing argument to '%s'", a->argv[i]);
                i++;
                name = a->argv[i];
            }
            else if (   !strcmp(a->argv[i], "--transient")
                     || !strcmp(a->argv[i], "-transient"))
            {
                fTransient = true;
            }
            else
                return errorSyntax(USAGE_SHAREDFOLDER_REMOVE, "Invalid parameter '%s'", Utf8Str(a->argv[i]).c_str());
        }

        /* required arguments */
        if (!name)
            return errorSyntax(USAGE_SHAREDFOLDER_REMOVE, "Parameter --name is required");

        if (fTransient)
        {
            /* open an existing session for the VM */
            CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Shared), RTEXITCODE_FAILURE);
            /* get the session machine */
            ComPtr<IMachine> sessionMachine;
            CHECK_ERROR_RET(a->session, COMGETTER(Machine)(sessionMachine.asOutParam()), RTEXITCODE_FAILURE);
            /* get the session console */
            ComPtr<IConsole> console;
            CHECK_ERROR_RET(a->session, COMGETTER(Console)(console.asOutParam()), RTEXITCODE_FAILURE);
            if (console.isNull())
                return RTMsgErrorExit(RTEXITCODE_FAILURE,
                                      "Machine '%s' is not currently running.\n", pszMachineName);

            CHECK_ERROR(console, RemoveSharedFolder(Bstr(name).raw()));

            a->session->UnlockMachine();
        }
        else
        {
            /* open a session for the VM */
            CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Write), RTEXITCODE_FAILURE);

            /* get the mutable session machine */
            ComPtr<IMachine> sessionMachine;
            a->session->COMGETTER(Machine)(sessionMachine.asOutParam());

            CHECK_ERROR(sessionMachine, RemoveSharedFolder(Bstr(name).raw()));

            /* commit and close the session */
            CHECK_ERROR(sessionMachine, SaveSettings());
            a->session->UnlockMachine();
        }
    }
    else
        return errorSyntax(USAGE_SHAREDFOLDER, "Invalid parameter '%s'", Utf8Str(a->argv[0]).c_str());

    return RTEXITCODE_SUCCESS;
}

RTEXITCODE handleExtPack(HandlerArg *a)
{
    if (a->argc < 1)
        return errorNoSubcommand();

    ComObjPtr<IExtPackManager> ptrExtPackMgr;
    CHECK_ERROR2I_RET(a->virtualBox, COMGETTER(ExtensionPackManager)(ptrExtPackMgr.asOutParam()), RTEXITCODE_FAILURE);

    RTGETOPTSTATE   GetState;
    RTGETOPTUNION   ValueUnion;
    int             ch;
    HRESULT         hrc = S_OK;

    if (!strcmp(a->argv[0], "install"))
    {
        setCurrentSubcommand(HELP_SCOPE_EXTPACK_INSTALL);
        const char *pszName  = NULL;
        bool        fReplace = false;

        static const RTGETOPTDEF s_aInstallOptions[] =
        {
            { "--replace",        'r', RTGETOPT_REQ_NOTHING },
            { "--accept-license", 'a', RTGETOPT_REQ_STRING },
        };

        RTCList<RTCString> lstLicenseHashes;
        RTGetOptInit(&GetState, a->argc, a->argv, s_aInstallOptions, RT_ELEMENTS(s_aInstallOptions), 1, 0 /*fFlags*/);
        while ((ch = RTGetOpt(&GetState, &ValueUnion)))
        {
            switch (ch)
            {
                case 'r':
                    fReplace = true;
                    break;

                case 'a':
                    lstLicenseHashes.append(ValueUnion.psz);
                    lstLicenseHashes[lstLicenseHashes.size() - 1].toLower();
                    break;

                case VINF_GETOPT_NOT_OPTION:
                    if (pszName)
                        return errorSyntax("Too many extension pack names given to \"extpack uninstall\"");
                    pszName = ValueUnion.psz;
                    break;

                default:
                    return errorGetOpt(ch, &ValueUnion);
            }
        }
        if (!pszName)
            return errorSyntax("No extension pack name was given to \"extpack install\"");

        char szPath[RTPATH_MAX];
        int vrc = RTPathAbs(pszName, szPath, sizeof(szPath));
        if (RT_FAILURE(vrc))
            return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTPathAbs(%s,,) failed with rc=%Rrc", pszName, vrc);

        Bstr bstrTarball(szPath);
        Bstr bstrName;
        ComPtr<IExtPackFile> ptrExtPackFile;
        CHECK_ERROR2I_RET(ptrExtPackMgr, OpenExtPackFile(bstrTarball.raw(), ptrExtPackFile.asOutParam()), RTEXITCODE_FAILURE);
        CHECK_ERROR2I_RET(ptrExtPackFile, COMGETTER(Name)(bstrName.asOutParam()), RTEXITCODE_FAILURE);
        BOOL fShowLicense = true;
        CHECK_ERROR2I_RET(ptrExtPackFile, COMGETTER(ShowLicense)(&fShowLicense), RTEXITCODE_FAILURE);
        if (fShowLicense)
        {
            Bstr bstrLicense;
            CHECK_ERROR2I_RET(ptrExtPackFile,
                              QueryLicense(Bstr("").raw() /* PreferredLocale */,
                                           Bstr("").raw() /* PreferredLanguage */,
                                           Bstr("txt").raw() /* Format */,
                                           bstrLicense.asOutParam()), RTEXITCODE_FAILURE);
            Utf8Str strLicense(bstrLicense);
            uint8_t abHash[RTSHA256_HASH_SIZE];
            char    szDigest[RTSHA256_DIGEST_LEN + 1];
            RTSha256(strLicense.c_str(), strLicense.length(), abHash);
            vrc = RTSha256ToString(abHash, szDigest, sizeof(szDigest));
            AssertRCStmt(vrc, szDigest[0] = '\0');
            if (lstLicenseHashes.contains(szDigest))
                RTPrintf("License accepted.\n");
            else
            {
                RTPrintf("%s\n", strLicense.c_str());
                RTPrintf("Do you agree to these license terms and conditions (y/n)? " );
                ch = RTStrmGetCh(g_pStdIn);
                RTPrintf("\n");
                if (ch != 'y' && ch != 'Y')
                {
                    RTPrintf("Installation of \"%ls\" aborted.\n", bstrName.raw());
                    return RTEXITCODE_FAILURE;
                }
                if (szDigest[0])
                    RTPrintf("License accepted. For batch installaltion add\n"
                             "--accept-license=%s\n"
                             "to the VBoxManage command line.\n\n", szDigest);
            }
        }
        ComPtr<IProgress> ptrProgress;
        CHECK_ERROR2I_RET(ptrExtPackFile, Install(fReplace, NULL, ptrProgress.asOutParam()), RTEXITCODE_FAILURE);
        hrc = showProgress(ptrProgress);
        CHECK_PROGRESS_ERROR_RET(ptrProgress, ("Failed to install \"%s\"", szPath), RTEXITCODE_FAILURE);

        RTPrintf("Successfully installed \"%ls\".\n", bstrName.raw());
    }
    else if (!strcmp(a->argv[0], "uninstall"))
    {
        setCurrentSubcommand(HELP_SCOPE_EXTPACK_UNINSTALL);
        const char *pszName = NULL;
        bool        fForced = false;

        static const RTGETOPTDEF s_aUninstallOptions[] =
        {
            { "--force",  'f', RTGETOPT_REQ_NOTHING },
        };

        RTGetOptInit(&GetState, a->argc, a->argv, s_aUninstallOptions, RT_ELEMENTS(s_aUninstallOptions), 1, 0);
        while ((ch = RTGetOpt(&GetState, &ValueUnion)))
        {
            switch (ch)
            {
                case 'f':
                    fForced = true;
                    break;

                case VINF_GETOPT_NOT_OPTION:
                    if (pszName)
                        return errorSyntax("Too many extension pack names given to \"extpack uninstall\"");
                    pszName = ValueUnion.psz;
                    break;

                default:
                    return errorGetOpt(ch, &ValueUnion);
            }
        }
        if (!pszName)
            return errorSyntax("No extension pack name was given to \"extpack uninstall\"");

        Bstr bstrName(pszName);
        ComPtr<IProgress> ptrProgress;
        CHECK_ERROR2I_RET(ptrExtPackMgr, Uninstall(bstrName.raw(), fForced, NULL, ptrProgress.asOutParam()), RTEXITCODE_FAILURE);
        hrc = showProgress(ptrProgress);
        CHECK_PROGRESS_ERROR_RET(ptrProgress, ("Failed to uninstall \"%s\"", pszName), RTEXITCODE_FAILURE);

        RTPrintf("Successfully uninstalled \"%s\".\n", pszName);
    }
    else if (!strcmp(a->argv[0], "cleanup"))
    {
        setCurrentSubcommand(HELP_SCOPE_EXTPACK_CLEANUP);
        if (a->argc > 1)
            return errorTooManyParameters(&a->argv[1]);
        CHECK_ERROR2I_RET(ptrExtPackMgr, Cleanup(), RTEXITCODE_FAILURE);
        RTPrintf("Successfully performed extension pack cleanup\n");
    }
    else
        return errorUnknownSubcommand(a->argv[0]);

    return RTEXITCODE_SUCCESS;
}

RTEXITCODE handleUnattendedDetect(HandlerArg *a)
{
    HRESULT hrc;

    /*
     * Options.  We work directly on an IUnattended instace while parsing
     * the options.  This saves a lot of extra clutter.
     */
    bool    fMachineReadable = false;
    char    szIsoPath[RTPATH_MAX];
    szIsoPath[0] = '\0';

    /*
     * Parse options.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--iso",                              'i', RTGETOPT_REQ_STRING },
        { "--machine-readable",                 'M', RTGETOPT_REQ_NOTHING },
    };

    RTGETOPTSTATE GetState;
    int vrc = RTGetOptInit(&GetState, a->argc, a->argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, RTGETOPTINIT_FLAGS_OPTS_FIRST);
    AssertRCReturn(vrc, RTEXITCODE_FAILURE);

    int c;
    RTGETOPTUNION ValueUnion;
    while ((c = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        switch (c)
        {
            case 'i': // --iso
                vrc = RTPathAbs(ValueUnion.psz, szIsoPath, sizeof(szIsoPath));
                if (RT_FAILURE(vrc))
                    return errorSyntax("RTPathAbs failed on '%s': %Rrc", ValueUnion.psz, vrc);
                break;

            case 'M': // --machine-readable.
                fMachineReadable = true;
                break;

            default:
                return errorGetOpt(c, &ValueUnion);
        }
    }

    /*
     * Check for required stuff.
     */
    if (szIsoPath[0] == '\0')
        return errorSyntax("No ISO specified");

    /*
     * Do the job.
     */
    ComPtr<IUnattended> ptrUnattended;
    CHECK_ERROR2_RET(hrc, a->virtualBox, CreateUnattendedInstaller(ptrUnattended.asOutParam()), RTEXITCODE_FAILURE);
    CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(IsoPath)(Bstr(szIsoPath).raw()), RTEXITCODE_FAILURE);
    CHECK_ERROR2(hrc, ptrUnattended, DetectIsoOS());
    RTEXITCODE rcExit = SUCCEEDED(hrc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;

    /*
     * Retrieve the results.
     */
    Bstr bstrDetectedOSTypeId;
    CHECK_ERROR2_RET(hrc, ptrUnattended, COMGETTER(DetectedOSTypeId)(bstrDetectedOSTypeId.asOutParam()), RTEXITCODE_FAILURE);
    Bstr bstrDetectedVersion;
    CHECK_ERROR2_RET(hrc, ptrUnattended, COMGETTER(DetectedOSVersion)(bstrDetectedVersion.asOutParam()), RTEXITCODE_FAILURE);
    Bstr bstrDetectedFlavor;
    CHECK_ERROR2_RET(hrc, ptrUnattended, COMGETTER(DetectedOSFlavor)(bstrDetectedFlavor.asOutParam()), RTEXITCODE_FAILURE);
    Bstr bstrDetectedLanguages;
    CHECK_ERROR2_RET(hrc, ptrUnattended, COMGETTER(DetectedOSLanguages)(bstrDetectedLanguages.asOutParam()), RTEXITCODE_FAILURE);
    Bstr bstrDetectedHints;
    CHECK_ERROR2_RET(hrc, ptrUnattended, COMGETTER(DetectedOSHints)(bstrDetectedHints.asOutParam()), RTEXITCODE_FAILURE);
    if (fMachineReadable)
        RTPrintf("OSTypeId=\"%ls\"\n"
                 "OSVersion=\"%ls\"\n"
                 "OSFlavor=\"%ls\"\n"
                 "OSLanguages=\"%ls\"\n"
                 "OSHints=\"%ls\"\n",
                 bstrDetectedOSTypeId.raw(),
                 bstrDetectedVersion.raw(),
                 bstrDetectedFlavor.raw(),
                 bstrDetectedLanguages.raw(),
                 bstrDetectedHints.raw());
    else
    {
        RTMsgInfo("Detected '%s' to be:\n", szIsoPath);
        RTPrintf("    OS TypeId    = %ls\n"
                 "    OS Version   = %ls\n"
                 "    OS Flavor    = %ls\n"
                 "    OS Languages = %ls\n"
                 "    OS Hints     = %ls\n",
                 bstrDetectedOSTypeId.raw(),
                 bstrDetectedVersion.raw(),
                 bstrDetectedFlavor.raw(),
                 bstrDetectedLanguages.raw(),
                 bstrDetectedHints.raw());
    }

    return rcExit;
}

RTEXITCODE handleUnattendedInstall(HandlerArg *a)
{
    HRESULT hrc;
    char    szAbsPath[RTPATH_MAX];

    /*
     * Options.  We work directly on an IUnattended instace while parsing
     * the options.  This saves a lot of extra clutter.
     */
    ComPtr<IUnattended> ptrUnattended;
    CHECK_ERROR2_RET(hrc, a->virtualBox, CreateUnattendedInstaller(ptrUnattended.asOutParam()), RTEXITCODE_FAILURE);
    RTCList<RTCString>  arrPackageSelectionAdjustments;
    ComPtr<IMachine>    ptrMachine;
    bool                fDryRun = false;
    const char         *pszSessionType = "none";

    /*
     * Parse options.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--iso",                              'i', RTGETOPT_REQ_STRING },
        { "--user",                             'u', RTGETOPT_REQ_STRING },
        { "--password",                         'p', RTGETOPT_REQ_STRING },
        { "--password-file",                    'X', RTGETOPT_REQ_STRING },
        { "--full-user-name",                   'U', RTGETOPT_REQ_STRING },
        { "--key",                              'k', RTGETOPT_REQ_STRING },
        { "--install-additions",                'A', RTGETOPT_REQ_NOTHING },
        { "--no-install-additions",             'N', RTGETOPT_REQ_NOTHING },
        { "--additions-iso",                    'a', RTGETOPT_REQ_STRING },
        { "--install-txs",                      't', RTGETOPT_REQ_NOTHING },
        { "--no-install-txs",                   'T', RTGETOPT_REQ_NOTHING },
        { "--validation-kit-iso",               'K', RTGETOPT_REQ_STRING },
        { "--locale",                           'l', RTGETOPT_REQ_STRING },
        { "--country",                          'Y', RTGETOPT_REQ_STRING },
        { "--time-zone",                        'z', RTGETOPT_REQ_STRING },
        { "--proxy",                            'y', RTGETOPT_REQ_STRING },
        { "--hostname",                         'H', RTGETOPT_REQ_STRING },
        { "--package-selection-adjustment",     's', RTGETOPT_REQ_STRING },
        { "--dry-run",                          'D', RTGETOPT_REQ_NOTHING },
        // advance options:
        { "--auxiliary-base-path",              'x', RTGETOPT_REQ_STRING },
        { "--image-index",                      'm', RTGETOPT_REQ_UINT32 },
        { "--script-template",                  'c', RTGETOPT_REQ_STRING },
        { "--post-install-template",            'C', RTGETOPT_REQ_STRING },
        { "--post-install-command",             'P', RTGETOPT_REQ_STRING },
        { "--extra-install-kernel-parameters",  'I', RTGETOPT_REQ_STRING },
        { "--language",                         'L', RTGETOPT_REQ_STRING },
        // start vm related options:
        { "--start-vm",                         'S', RTGETOPT_REQ_STRING },
        /** @todo Add a --wait option too for waiting for the VM to shut down or
         *        something like that...? */
    };

    RTGETOPTSTATE GetState;
    int vrc = RTGetOptInit(&GetState, a->argc, a->argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, RTGETOPTINIT_FLAGS_OPTS_FIRST);
    AssertRCReturn(vrc, RTEXITCODE_FAILURE);

    int c;
    RTGETOPTUNION ValueUnion;
    while ((c = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        switch (c)
        {
            case VINF_GETOPT_NOT_OPTION:
                if (ptrMachine.isNotNull())
                    return errorSyntax("VM name/UUID given more than once!");
                CHECK_ERROR2_RET(hrc, a->virtualBox, FindMachine(Bstr(ValueUnion.psz).raw(), ptrMachine.asOutParam()), RTEXITCODE_FAILURE);
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(Machine)(ptrMachine), RTEXITCODE_FAILURE);
                break;

            case 'i':   // --iso
                vrc = RTPathAbs(ValueUnion.psz, szAbsPath, sizeof(szAbsPath));
                if (RT_FAILURE(vrc))
                    return errorSyntax("RTPathAbs failed on '%s': %Rrc", ValueUnion.psz, vrc);
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(IsoPath)(Bstr(szAbsPath).raw()), RTEXITCODE_FAILURE);
                break;

            case 'u':   // --user
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(User)(Bstr(ValueUnion.psz).raw()), RTEXITCODE_FAILURE);
                break;

            case 'p':   // --password
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(Password)(Bstr(ValueUnion.psz).raw()), RTEXITCODE_FAILURE);
                break;

            case 'X':   // --password-file
            {
                Utf8Str strPassword;
                RTEXITCODE rcExit = readPasswordFile(ValueUnion.psz, &strPassword);
                if (rcExit != RTEXITCODE_SUCCESS)
                    return rcExit;
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(Password)(Bstr(strPassword).raw()), RTEXITCODE_FAILURE);
                break;
            }

            case 'U':   // --full-user-name
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(FullUserName)(Bstr(ValueUnion.psz).raw()), RTEXITCODE_FAILURE);
                break;

            case 'k':   // --key
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(ProductKey)(Bstr(ValueUnion.psz).raw()), RTEXITCODE_FAILURE);
                break;

            case 'A':   // --install-additions
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(InstallGuestAdditions)(TRUE), RTEXITCODE_FAILURE);
                break;
            case 'N':   // --no-install-additions
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(InstallGuestAdditions)(FALSE), RTEXITCODE_FAILURE);
                break;
            case 'a':   // --additions-iso
                vrc = RTPathAbs(ValueUnion.psz, szAbsPath, sizeof(szAbsPath));
                if (RT_FAILURE(vrc))
                    return errorSyntax("RTPathAbs failed on '%s': %Rrc", ValueUnion.psz, vrc);
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(AdditionsIsoPath)(Bstr(szAbsPath).raw()), RTEXITCODE_FAILURE);
                break;

            case 't':   // --install-txs
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(InstallTestExecService)(TRUE), RTEXITCODE_FAILURE);
                break;
            case 'T':   // --no-install-txs
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(InstallTestExecService)(FALSE), RTEXITCODE_FAILURE);
                break;
            case 'K':   // --valiation-kit-iso
                vrc = RTPathAbs(ValueUnion.psz, szAbsPath, sizeof(szAbsPath));
                if (RT_FAILURE(vrc))
                    return errorSyntax("RTPathAbs failed on '%s': %Rrc", ValueUnion.psz, vrc);
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(ValidationKitIsoPath)(Bstr(szAbsPath).raw()), RTEXITCODE_FAILURE);
                break;

            case 'l':   // --locale
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(Locale)(Bstr(ValueUnion.psz).raw()), RTEXITCODE_FAILURE);
                break;

            case 'Y':   // --country
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(Country)(Bstr(ValueUnion.psz).raw()), RTEXITCODE_FAILURE);
                break;

            case 'z':   // --time-zone;
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(TimeZone)(Bstr(ValueUnion.psz).raw()), RTEXITCODE_FAILURE);
                break;

            case 'y':   // --proxy
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(Proxy)(Bstr(ValueUnion.psz).raw()), RTEXITCODE_FAILURE);
                break;

            case 'H':   // --hostname
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(Hostname)(Bstr(ValueUnion.psz).raw()), RTEXITCODE_FAILURE);
                break;

            case 's':   // --package-selection-adjustment
                arrPackageSelectionAdjustments.append(ValueUnion.psz);
                break;

            case 'D':
                fDryRun = true;
                break;

            case 'x':   // --auxiliary-base-path
                vrc = RTPathAbs(ValueUnion.psz, szAbsPath, sizeof(szAbsPath));
                if (RT_FAILURE(vrc))
                    return errorSyntax("RTPathAbs failed on '%s': %Rrc", ValueUnion.psz, vrc);
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(AuxiliaryBasePath)(Bstr(szAbsPath).raw()), RTEXITCODE_FAILURE);
                break;

            case 'm':   // --image-index
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(ImageIndex)(ValueUnion.u32), RTEXITCODE_FAILURE);
                break;

            case 'c':   // --script-template
                vrc = RTPathAbs(ValueUnion.psz, szAbsPath, sizeof(szAbsPath));
                if (RT_FAILURE(vrc))
                    return errorSyntax("RTPathAbs failed on '%s': %Rrc", ValueUnion.psz, vrc);
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(ScriptTemplatePath)(Bstr(szAbsPath).raw()), RTEXITCODE_FAILURE);
                break;

            case 'C':   // --post-install-script-template
                vrc = RTPathAbs(ValueUnion.psz, szAbsPath, sizeof(szAbsPath));
                if (RT_FAILURE(vrc))
                    return errorSyntax("RTPathAbs failed on '%s': %Rrc", ValueUnion.psz, vrc);
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(PostInstallScriptTemplatePath)(Bstr(szAbsPath).raw()), RTEXITCODE_FAILURE);
                break;

            case 'P':   // --post-install-command.
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(PostInstallCommand)(Bstr(ValueUnion.psz).raw()), RTEXITCODE_FAILURE);
                break;

            case 'I':   // --extra-install-kernel-parameters
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(ExtraInstallKernelParameters)(Bstr(ValueUnion.psz).raw()), RTEXITCODE_FAILURE);
                break;

            case 'L':   // --language
                CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(Language)(Bstr(ValueUnion.psz).raw()), RTEXITCODE_FAILURE);
                break;

            case 'S':   // --start-vm
                pszSessionType = ValueUnion.psz;
                break;

            default:
                return errorGetOpt(c, &ValueUnion);
        }
    }

    /*
     * Check for required stuff.
     */
    if (ptrMachine.isNull())
        return errorSyntax("Missing VM name/UUID");

    /*
     * Set accumulative attributes.
     */
    if (arrPackageSelectionAdjustments.size() == 1)
        CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(PackageSelectionAdjustments)(Bstr(arrPackageSelectionAdjustments[0]).raw()),
                         RTEXITCODE_FAILURE);
    else if (arrPackageSelectionAdjustments.size() > 1)
    {
        RTCString strAdjustments;
        strAdjustments.join(arrPackageSelectionAdjustments, ";");
        CHECK_ERROR2_RET(hrc, ptrUnattended, COMSETTER(PackageSelectionAdjustments)(Bstr(strAdjustments).raw()), RTEXITCODE_FAILURE);
    }

    /*
     * Get details about the machine so we can display them below.
     */
    Bstr bstrMachineName;
    CHECK_ERROR2_RET(hrc, ptrMachine, COMGETTER(Name)(bstrMachineName.asOutParam()), RTEXITCODE_FAILURE);
    Bstr bstrUuid;
    CHECK_ERROR2_RET(hrc, ptrMachine, COMGETTER(Id)(bstrUuid.asOutParam()), RTEXITCODE_FAILURE);
    BSTR bstrInstalledOS;
    CHECK_ERROR2_RET(hrc, ptrMachine, COMGETTER(OSTypeId)(&bstrInstalledOS), RTEXITCODE_FAILURE);
    Utf8Str strInstalledOS(bstrInstalledOS);

    /*
     * Temporarily lock the machine to check whether it's running or not.
     * We take this opportunity to disable the first run wizard.
     */
    CHECK_ERROR2_RET(hrc, ptrMachine, LockMachine(a->session, LockType_Shared), RTEXITCODE_FAILURE);
    {
        ComPtr<IConsole> ptrConsole;
        CHECK_ERROR2(hrc, a->session, COMGETTER(Console)(ptrConsole.asOutParam()));

        if (   ptrConsole.isNull()
            && SUCCEEDED(hrc)
            && (   RTStrICmp(pszSessionType, "gui") == 0
                || RTStrICmp(pszSessionType, "none") == 0))
        {
            ComPtr<IMachine> ptrSessonMachine;
            CHECK_ERROR2(hrc, a->session, COMGETTER(Machine)(ptrSessonMachine.asOutParam()));
            if (ptrSessonMachine.isNotNull())
            {
                CHECK_ERROR2(hrc, ptrSessonMachine, SetExtraData(Bstr("GUI/FirstRun").raw(), Bstr("0").raw()));
            }
        }

        a->session->UnlockMachine();
        if (FAILED(hrc))
            return RTEXITCODE_FAILURE;
        if (ptrConsole.isNotNull())
            return RTMsgErrorExit(RTEXITCODE_FAILURE, "Machine '%ls' is currently running", bstrMachineName.raw());
    }

    /*
     * Do the work.
     */
    RTMsgInfo("%s unattended installation of %s in machine '%ls' (%ls).\n",
              RTStrICmp(pszSessionType, "none") == 0 ? "Preparing" : "Starting",
              strInstalledOS.c_str(), bstrMachineName.raw(), bstrUuid.raw());

    CHECK_ERROR2_RET(hrc, ptrUnattended,Prepare(), RTEXITCODE_FAILURE);
    if (!fDryRun)
    {
        CHECK_ERROR2_RET(hrc, ptrUnattended, ConstructMedia(), RTEXITCODE_FAILURE);
        CHECK_ERROR2_RET(hrc, ptrUnattended,ReconfigureVM(), RTEXITCODE_FAILURE);
    }

    /*
     * Retrieve and display the parameters actually used.
     */
    RTMsgInfo("Using values:\n");
#define SHOW_ATTR(a_Attr, a_szText, a_Type, a_szFmt) do { \
            a_Type Value; \
            HRESULT hrc2 = ptrUnattended->COMGETTER(a_Attr)(&Value); \
            if (SUCCEEDED(hrc2)) \
                RTPrintf("  %32s = " a_szFmt "\n", a_szText, Value); \
            else \
                RTPrintf("  %32s = failed: %Rhrc\n", a_szText, hrc2); \
        } while (0)
#define SHOW_STR_ATTR(a_Attr, a_szText) do { \
            Bstr bstrString; \
            HRESULT hrc2 = ptrUnattended->COMGETTER(a_Attr)(bstrString.asOutParam()); \
            if (SUCCEEDED(hrc2)) \
                RTPrintf("  %32s = %ls\n", a_szText, bstrString.raw()); \
            else \
                RTPrintf("  %32s = failed: %Rhrc\n", a_szText, hrc2); \
        } while (0)

    SHOW_STR_ATTR(IsoPath,                       "isoPath");
    SHOW_STR_ATTR(User,                          "user");
    SHOW_STR_ATTR(Password,                      "password");
    SHOW_STR_ATTR(FullUserName,                  "fullUserName");
    SHOW_STR_ATTR(ProductKey,                    "productKey");
    SHOW_STR_ATTR(AdditionsIsoPath,              "additionsIsoPath");
    SHOW_ATTR(    InstallGuestAdditions,         "installGuestAdditions",    BOOL, "%RTbool");
    SHOW_STR_ATTR(ValidationKitIsoPath,          "validationKitIsoPath");
    SHOW_ATTR(    InstallTestExecService,        "installTestExecService",   BOOL, "%RTbool");
    SHOW_STR_ATTR(Locale,                        "locale");
    SHOW_STR_ATTR(Country,                       "country");
    SHOW_STR_ATTR(TimeZone,                      "timeZone");
    SHOW_STR_ATTR(Proxy,                         "proxy");
    SHOW_STR_ATTR(Hostname,                      "hostname");
    SHOW_STR_ATTR(PackageSelectionAdjustments,   "packageSelectionAdjustments");
    SHOW_STR_ATTR(AuxiliaryBasePath,             "auxiliaryBasePath");
    SHOW_ATTR(    ImageIndex,                    "imageIndex",               ULONG, "%u");
    SHOW_STR_ATTR(ScriptTemplatePath,            "scriptTemplatePath");
    SHOW_STR_ATTR(PostInstallScriptTemplatePath, "postInstallScriptTemplatePath");
    SHOW_STR_ATTR(PostInstallCommand,            "postInstallCommand");
    SHOW_STR_ATTR(ExtraInstallKernelParameters,  "extraInstallKernelParameters");
    SHOW_STR_ATTR(Language,                      "language");
    SHOW_STR_ATTR(DetectedOSTypeId,              "detectedOSTypeId");
    SHOW_STR_ATTR(DetectedOSVersion,             "detectedOSVersion");
    SHOW_STR_ATTR(DetectedOSFlavor,              "detectedOSFlavor");
    SHOW_STR_ATTR(DetectedOSLanguages,           "detectedOSLanguages");
    SHOW_STR_ATTR(DetectedOSHints,               "detectedOSHints");

#undef SHOW_STR_ATTR
#undef SHOW_ATTR

    /* We can drop the IUnatteded object now. */
    ptrUnattended.setNull();

    /*
     * Start the VM if requested.
     */
    if (   fDryRun
        || RTStrICmp(pszSessionType, "none") == 0)
    {
        if (!fDryRun)
            RTMsgInfo("VM '%ls' (%ls) is ready to be started (e.g. VBoxManage startvm).\n", bstrMachineName.raw(), bstrUuid.raw());
        hrc = S_OK;
    }
    else
    {
        Bstr env;
#if defined(RT_OS_LINUX) || defined(RT_OS_SOLARIS)
        /* make sure the VM process will start on the same display as VBoxManage */
        Utf8Str str;
        const char *pszDisplay = RTEnvGet("DISPLAY");
        if (pszDisplay)
            str = Utf8StrFmt("DISPLAY=%s\n", pszDisplay);
        const char *pszXAuth = RTEnvGet("XAUTHORITY");
        if (pszXAuth)
            str.append(Utf8StrFmt("XAUTHORITY=%s\n", pszXAuth));
        env = str;
#endif
        ComPtr<IProgress> ptrProgress;
        CHECK_ERROR2(hrc, ptrMachine, LaunchVMProcess(a->session, Bstr(pszSessionType).raw(), env.raw(), ptrProgress.asOutParam()));
        if (SUCCEEDED(hrc) && !ptrProgress.isNull())
        {
            RTMsgInfo("Waiting for VM '%ls' to power on...\n", bstrMachineName.raw());
            CHECK_ERROR2(hrc, ptrProgress, WaitForCompletion(-1));
            if (SUCCEEDED(hrc))
            {
                BOOL fCompleted = true;
                CHECK_ERROR2(hrc, ptrProgress, COMGETTER(Completed)(&fCompleted));
                if (SUCCEEDED(hrc))
                {
                    ASSERT(fCompleted);

                    LONG iRc;
                    CHECK_ERROR2(hrc, ptrProgress, COMGETTER(ResultCode)(&iRc));
                    if (SUCCEEDED(hrc))
                    {
                        if (SUCCEEDED(iRc))
                            RTMsgInfo("VM '%ls' (%ls) has been successfully started.\n", bstrMachineName.raw(), bstrUuid.raw());
                        else
                        {
                            ProgressErrorInfo info(ptrProgress);
                            com::GluePrintErrorInfo(info);
                        }
                        hrc = iRc;
                    }
                }
            }
        }

        /*
         * Do we wait for the VM to power down?
         */
    }

    return SUCCEEDED(hrc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}


RTEXITCODE handleUnattended(HandlerArg *a)
{
    /*
     * Sub-command switch.
     */
    if (a->argc < 1)
        return errorNoSubcommand();

    if (!strcmp(a->argv[0], "detect"))
    {
        setCurrentSubcommand(HELP_SCOPE_UNATTENDED_DETECT);
        return handleUnattendedDetect(a);
    }

    if (!strcmp(a->argv[0], "install"))
    {
        setCurrentSubcommand(HELP_SCOPE_UNATTENDED_INSTALL);
        return handleUnattendedInstall(a);
    }

    /* Consider some kind of create-vm-and-install-guest-os command. */
    return errorUnknownSubcommand(a->argv[0]);
}
