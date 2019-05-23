/* $Id: VBoxManage.cpp $ */
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
# include <VBox/com/NativeEventQueue.h>

# include <VBox/com/VirtualBox.h>
#endif /* !VBOX_ONLY_DOCS */

#include <VBox/err.h>
#include <VBox/version.h>

#include <iprt/asm.h>
#include <iprt/buildconfig.h>
#include <iprt/ctype.h>
#include <iprt/file.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>

#include <signal.h>

#include "VBoxManage.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The command doesn't need the COM stuff. */
#define VBMG_CMD_F_NO_COM       RT_BIT_32(0)

#define VBMG_CMD_TODO HELP_CMD_VBOXMANAGE_INVALID


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
#ifndef VBOX_ONLY_DOCS
/**
 * VBoxManage command descriptor.
 */
typedef struct VBMGCMD
{
    /** The command.   */
    const char                 *pszCommand;
    /** The help category. */
    USAGECATEGORY               enmHelpCat;
    /** The new help command. */
    enum HELP_CMD_VBOXMANAGE    enmCmdHelp;
    /** The handler. */
    RTEXITCODE                (*pfnHandler)(HandlerArg *pArg);
    /** VBMG_CMD_F_XXX,    */
    uint32_t                    fFlags;
} VBMGCMD;
/** Pointer to a const VBoxManage command descriptor. */
typedef VBMGCMD const *PCVBMGCMD;
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/*extern*/ bool         g_fDetailedProgress = false;

#ifndef VBOX_ONLY_DOCS
/** Set by the signal handler. */
static volatile bool    g_fCanceled = false;


/**
 * All registered command handlers
 */
static const VBMGCMD g_aCommands[] =
{
    { "internalcommands",   0,                      VBMG_CMD_TODO, handleInternalCommands,     0 },
    { "list",               USAGE_LIST,             VBMG_CMD_TODO, handleList,                 0 },
    { "showvminfo",         USAGE_SHOWVMINFO,       VBMG_CMD_TODO, handleShowVMInfo,           0 },
    { "registervm",         USAGE_REGISTERVM,       VBMG_CMD_TODO, handleRegisterVM,           0 },
    { "unregistervm",       USAGE_UNREGISTERVM,     VBMG_CMD_TODO, handleUnregisterVM,         0 },
    { "clonevm",            USAGE_CLONEVM,          VBMG_CMD_TODO, handleCloneVM,              0 },
    { "mediumproperty",     USAGE_MEDIUMPROPERTY,   VBMG_CMD_TODO, handleMediumProperty,       0 },
    { "hdproperty",         USAGE_MEDIUMPROPERTY,   VBMG_CMD_TODO, handleMediumProperty,       0 }, /* backward compatibility */
    { "createmedium",       USAGE_CREATEMEDIUM,     VBMG_CMD_TODO, handleCreateMedium,         0 },
    { "createhd",           USAGE_CREATEMEDIUM,     VBMG_CMD_TODO, handleCreateMedium,         0 }, /* backward compatibility */
    { "createvdi",          USAGE_CREATEMEDIUM,     VBMG_CMD_TODO, handleCreateMedium,         0 }, /* backward compatibility */
    { "modifymedium",       USAGE_MODIFYMEDIUM,     VBMG_CMD_TODO, handleModifyMedium,         0 },
    { "modifyhd",           USAGE_MODIFYMEDIUM,     VBMG_CMD_TODO, handleModifyMedium,         0 }, /* backward compatibility */
    { "modifyvdi",          USAGE_MODIFYMEDIUM,     VBMG_CMD_TODO, handleModifyMedium,         0 }, /* backward compatibility */
    { "clonemedium",        USAGE_CLONEMEDIUM,      VBMG_CMD_TODO, handleCloneMedium,          0 },
    { "clonehd",            USAGE_CLONEMEDIUM,      VBMG_CMD_TODO, handleCloneMedium,          0 }, /* backward compatibility */
    { "clonevdi",           USAGE_CLONEMEDIUM,      VBMG_CMD_TODO, handleCloneMedium,          0 }, /* backward compatibility */
    { "encryptmedium",      USAGE_ENCRYPTMEDIUM,    VBMG_CMD_TODO, handleEncryptMedium,        0 },
    { "checkmediumpwd",     USAGE_MEDIUMENCCHKPWD,  VBMG_CMD_TODO, handleCheckMediumPassword,  0 },
    { "createvm",           USAGE_CREATEVM,         VBMG_CMD_TODO, handleCreateVM,             0 },
    { "modifyvm",           USAGE_MODIFYVM,         VBMG_CMD_TODO, handleModifyVM,             0 },
    { "startvm",            USAGE_STARTVM,          VBMG_CMD_TODO, handleStartVM,              0 },
    { "controlvm",          USAGE_CONTROLVM,        VBMG_CMD_TODO, handleControlVM,            0 },
    { "unattended",         USAGE_UNATTENDEDINSTALL, HELP_CMD_UNATTENDED, handleUnattended,    0 },
    { "discardstate",       USAGE_DISCARDSTATE,     VBMG_CMD_TODO, handleDiscardState,         0 },
    { "adoptstate",         USAGE_ADOPTSTATE,       VBMG_CMD_TODO, handleAdoptState,           0 },
    { "snapshot",           USAGE_SNAPSHOT,         VBMG_CMD_TODO, handleSnapshot,             0 },
    { "closemedium",        USAGE_CLOSEMEDIUM,      VBMG_CMD_TODO, handleCloseMedium,          0 },
    { "storageattach",      USAGE_STORAGEATTACH,    VBMG_CMD_TODO, handleStorageAttach,        0 },
    { "storagectl",         USAGE_STORAGECONTROLLER,VBMG_CMD_TODO, handleStorageController,    0 },
    { "showmediuminfo",     USAGE_SHOWMEDIUMINFO,   VBMG_CMD_TODO, handleShowMediumInfo,       0 },
    { "showhdinfo",         USAGE_SHOWMEDIUMINFO,   VBMG_CMD_TODO, handleShowMediumInfo,       0 }, /* backward compatibility */
    { "showvdiinfo",        USAGE_SHOWMEDIUMINFO,   VBMG_CMD_TODO, handleShowMediumInfo,       0 }, /* backward compatibility */
    { "getextradata",       USAGE_GETEXTRADATA,     VBMG_CMD_TODO, handleGetExtraData,         0 },
    { "setextradata",       USAGE_SETEXTRADATA,     VBMG_CMD_TODO, handleSetExtraData,         0 },
    { "setproperty",        USAGE_SETPROPERTY,      VBMG_CMD_TODO, handleSetProperty,          0 },
    { "usbfilter",          USAGE_USBFILTER,        VBMG_CMD_TODO, handleUSBFilter,            0 },
    { "sharedfolder",       USAGE_SHAREDFOLDER,     VBMG_CMD_TODO, handleSharedFolder,         0 },
#ifdef VBOX_WITH_GUEST_PROPS
    { "guestproperty",      USAGE_GUESTPROPERTY,    VBMG_CMD_TODO, handleGuestProperty,        0 },
#endif
#ifdef VBOX_WITH_GUEST_CONTROL
    { "guestcontrol",       USAGE_GUESTCONTROL,     VBMG_CMD_TODO, handleGuestControl,         0 },
#endif
    { "metrics",            USAGE_METRICS,          VBMG_CMD_TODO, handleMetrics,              0 },
    { "import",             USAGE_IMPORTAPPLIANCE,  VBMG_CMD_TODO, handleImportAppliance,      0 },
    { "export",             USAGE_EXPORTAPPLIANCE,  VBMG_CMD_TODO, handleExportAppliance,      0 },
#ifdef VBOX_WITH_NETFLT
    { "hostonlyif",         USAGE_HOSTONLYIFS,      VBMG_CMD_TODO, handleHostonlyIf,           0 },
#endif
    { "dhcpserver",         USAGE_DHCPSERVER,       VBMG_CMD_TODO, handleDHCPServer,           0 },
#ifdef VBOX_WITH_NAT_SERVICE
    { "natnetwork",         USAGE_NATNETWORK,       VBMG_CMD_TODO, handleNATNetwork,           0 },
#endif
    { "extpack",            USAGE_EXTPACK,       HELP_CMD_EXTPACK, handleExtPack,              0 },
    { "bandwidthctl",       USAGE_BANDWIDTHCONTROL, VBMG_CMD_TODO, handleBandwidthControl,     0 },
    { "debugvm",            USAGE_DEBUGVM,       HELP_CMD_DEBUGVM, handleDebugVM,              0 },
    { "convertfromraw",     USAGE_CONVERTFROMRAW,   VBMG_CMD_TODO, handleConvertFromRaw,       VBMG_CMD_F_NO_COM },
    { "convertdd",          USAGE_CONVERTFROMRAW,   VBMG_CMD_TODO, handleConvertFromRaw,       VBMG_CMD_F_NO_COM },
    { "usbdevsource",       USAGE_USBDEVSOURCE,     VBMG_CMD_TODO, handleUSBDevSource,         0 }
};


/**
 * Looks up a command by name.
 *
 * @returns Pointer to the command structure.
 * @param   pszCommand          Name of the command.
 */
static PCVBMGCMD lookupCommand(const char *pszCommand)
{
    if (pszCommand)
        for (uint32_t i = 0; i < RT_ELEMENTS(g_aCommands); i++)
            if (!strcmp(g_aCommands[i].pszCommand, pszCommand))
                return &g_aCommands[i];
    return NULL;
}


/**
 * Signal handler that sets g_fCanceled.
 *
 * This can be executed on any thread in the process, on Windows it may even be
 * a thread dedicated to delivering this signal.  Do not doing anything
 * unnecessary here.
 */
static void showProgressSignalHandler(int iSignal)
{
    NOREF(iSignal);
    ASMAtomicWriteBool(&g_fCanceled, true);
}

/**
 * Print out progress on the console.
 *
 * This runs the main event queue every now and then to prevent piling up
 * unhandled things (which doesn't cause real problems, just makes things
 * react a little slower than in the ideal case).
 */
HRESULT showProgress(ComPtr<IProgress> progress)
{
    using namespace com;

    BOOL fCompleted = FALSE;
    ULONG ulCurrentPercent = 0;
    ULONG ulLastPercent = 0;

    ULONG ulLastOperationPercent = (ULONG)-1;

    ULONG ulLastOperation = (ULONG)-1;
    Bstr bstrOperationDescription;

    NativeEventQueue::getMainEventQueue()->processEventQueue(0);

    ULONG cOperations = 1;
    HRESULT hrc = progress->COMGETTER(OperationCount)(&cOperations);
    if (FAILED(hrc))
    {
        RTStrmPrintf(g_pStdErr, "Progress object failure: %Rhrc\n", hrc);
        RTStrmFlush(g_pStdErr);
        return hrc;
    }

    /*
     * Note: Outputting the progress info to stderr (g_pStdErr) is intentional
     *       to not get intermixed with other (raw) stdout data which might get
     *       written in the meanwhile.
     */

    if (!g_fDetailedProgress)
    {
        RTStrmPrintf(g_pStdErr, "0%%...");
        RTStrmFlush(g_pStdErr);
    }

    /* setup signal handling if cancelable */
    bool fCanceledAlready = false;
    BOOL fCancelable;
    hrc = progress->COMGETTER(Cancelable)(&fCancelable);
    if (FAILED(hrc))
        fCancelable = FALSE;
    if (fCancelable)
    {
        signal(SIGINT,   showProgressSignalHandler);
        signal(SIGTERM,  showProgressSignalHandler);
#ifdef SIGBREAK
        signal(SIGBREAK, showProgressSignalHandler);
#endif
    }

    hrc = progress->COMGETTER(Completed(&fCompleted));
    while (SUCCEEDED(hrc))
    {
        progress->COMGETTER(Percent(&ulCurrentPercent));

        if (g_fDetailedProgress)
        {
            ULONG ulOperation = 1;
            hrc = progress->COMGETTER(Operation)(&ulOperation);
            if (FAILED(hrc))
                break;
            ULONG ulCurrentOperationPercent = 0;
            hrc = progress->COMGETTER(OperationPercent(&ulCurrentOperationPercent));
            if (FAILED(hrc))
                break;

            if (ulLastOperation != ulOperation)
            {
                hrc = progress->COMGETTER(OperationDescription(bstrOperationDescription.asOutParam()));
                if (FAILED(hrc))
                    break;
                ulLastPercent = (ULONG)-1;        // force print
                ulLastOperation = ulOperation;
            }

            if (    ulCurrentPercent != ulLastPercent
                 || ulCurrentOperationPercent != ulLastOperationPercent
               )
            {
                LONG lSecsRem = 0;
                progress->COMGETTER(TimeRemaining)(&lSecsRem);

                RTStrmPrintf(g_pStdErr, "(%u/%u) %ls %02u%% => %02u%% (%d s remaining)\n", ulOperation + 1, cOperations,
                             bstrOperationDescription.raw(), ulCurrentOperationPercent, ulCurrentPercent, lSecsRem);
                ulLastPercent = ulCurrentPercent;
                ulLastOperationPercent = ulCurrentOperationPercent;
            }
        }
        else
        {
            /* did we cross a 10% mark? */
            if (ulCurrentPercent / 10  >  ulLastPercent / 10)
            {
                /* make sure to also print out missed steps */
                for (ULONG curVal = (ulLastPercent / 10) * 10 + 10; curVal <= (ulCurrentPercent / 10) * 10; curVal += 10)
                {
                    if (curVal < 100)
                    {
                        RTStrmPrintf(g_pStdErr, "%u%%...", curVal);
                        RTStrmFlush(g_pStdErr);
                    }
                }
                ulLastPercent = (ulCurrentPercent / 10) * 10;
            }
        }
        if (fCompleted)
            break;

        /* process async cancelation */
        if (g_fCanceled && !fCanceledAlready)
        {
            hrc = progress->Cancel();
            if (SUCCEEDED(hrc))
                fCanceledAlready = true;
            else
                g_fCanceled = false;
        }

        /* make sure the loop is not too tight */
        progress->WaitForCompletion(100);

        NativeEventQueue::getMainEventQueue()->processEventQueue(0);
        hrc = progress->COMGETTER(Completed(&fCompleted));
    }

    /* undo signal handling */
    if (fCancelable)
    {
        signal(SIGINT,   SIG_DFL);
        signal(SIGTERM,  SIG_DFL);
# ifdef SIGBREAK
        signal(SIGBREAK, SIG_DFL);
# endif
    }

    /* complete the line. */
    LONG iRc = E_FAIL;
    hrc = progress->COMGETTER(ResultCode)(&iRc);
    if (SUCCEEDED(hrc))
    {
        if (SUCCEEDED(iRc))
            RTStrmPrintf(g_pStdErr, "100%%\n");
        else if (g_fCanceled)
            RTStrmPrintf(g_pStdErr, "CANCELED\n");
        else
        {
            if (!g_fDetailedProgress)
                RTStrmPrintf(g_pStdErr, "\n");
            RTStrmPrintf(g_pStdErr, "Progress state: %Rhrc\n", iRc);
        }
        hrc = iRc;
    }
    else
    {
        if (!g_fDetailedProgress)
            RTStrmPrintf(g_pStdErr, "\n");
        RTStrmPrintf(g_pStdErr, "Progress object failure: %Rhrc\n", hrc);
    }
    RTStrmFlush(g_pStdErr);
    return hrc;
}

RTEXITCODE readPasswordFile(const char *pszFilename, com::Utf8Str *pPasswd)
{
    size_t cbFile;
    char szPasswd[512];
    int vrc = VINF_SUCCESS;
    RTEXITCODE rcExit = RTEXITCODE_SUCCESS;
    bool fStdIn = !strcmp(pszFilename, "stdin");
    PRTSTREAM pStrm;
    if (!fStdIn)
        vrc = RTStrmOpen(pszFilename, "r", &pStrm);
    else
        pStrm = g_pStdIn;
    if (RT_SUCCESS(vrc))
    {
        vrc = RTStrmReadEx(pStrm, szPasswd, sizeof(szPasswd)-1, &cbFile);
        if (RT_SUCCESS(vrc))
        {
            if (cbFile >= sizeof(szPasswd)-1)
                rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Provided password in file '%s' is too long", pszFilename);
            else
            {
                unsigned i;
                for (i = 0; i < cbFile && !RT_C_IS_CNTRL(szPasswd[i]); i++)
                    ;
                szPasswd[i] = '\0';
                *pPasswd = szPasswd;
            }
        }
        else
            rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Cannot read password from file '%s': %Rrc", pszFilename, vrc);
        if (!fStdIn)
            RTStrmClose(pStrm);
    }
    else
        rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Cannot open password file '%s' (%Rrc)", pszFilename, vrc);

    return rcExit;
}

static RTEXITCODE settingsPasswordFile(ComPtr<IVirtualBox> virtualBox, const char *pszFilename)
{
    com::Utf8Str passwd;
    RTEXITCODE rcExit = readPasswordFile(pszFilename, &passwd);
    if (rcExit == RTEXITCODE_SUCCESS)
    {
        CHECK_ERROR2I_STMT(virtualBox, SetSettingsSecret(com::Bstr(passwd).raw()), rcExit = RTEXITCODE_FAILURE);
    }

    return rcExit;
}

RTEXITCODE readPasswordFromConsole(com::Utf8Str *pPassword, const char *pszPrompt, ...)
{
    RTEXITCODE rcExit = RTEXITCODE_SUCCESS;
    char aszPwdInput[_1K] = { 0 };
    va_list vaArgs;

    va_start(vaArgs, pszPrompt);
    int vrc = RTStrmPrintfV(g_pStdOut, pszPrompt, vaArgs);
    if (RT_SUCCESS(vrc))
    {
        bool fEchoOld = false;
        vrc = RTStrmInputGetEchoChars(g_pStdIn, &fEchoOld);
        if (RT_SUCCESS(vrc))
        {
            vrc = RTStrmInputSetEchoChars(g_pStdIn, false);
            if (RT_SUCCESS(vrc))
            {
                vrc = RTStrmGetLine(g_pStdIn, &aszPwdInput[0], sizeof(aszPwdInput));
                if (RT_SUCCESS(vrc))
                    *pPassword = aszPwdInput;
                else
                    rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed read password from command line (%Rrc)", vrc);

                int vrc2 = RTStrmInputSetEchoChars(g_pStdIn, fEchoOld);
                AssertRC(vrc2);
            }
            else
                rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed to disable echoing typed characters (%Rrc)", vrc);
        }
        else
            rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed to retrieve echo setting (%Rrc)", vrc);

        RTStrmPutStr(g_pStdOut, "\n");
    }
    else
        rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed to print prompt (%Rrc)", vrc);
    va_end(vaArgs);

    return rcExit;
}

#endif /* !VBOX_ONLY_DOCS */


int main(int argc, char *argv[])
{
    /*
     * Before we do anything, init the runtime without loading
     * the support driver.
     */
    int vrc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(vrc))
        return RTMsgInitFailure(vrc);
#if defined(RT_OS_WINDOWS) && !defined(VBOX_ONLY_DOCS)
    ATL::CComModule _Module; /* Required internally by ATL (constructor records instance in global variable). */
#endif

    /*
     * Parse the global options
     */
    bool fShowLogo = false;
    bool fShowHelp = false;
    int  iCmd      = 1;
    int  iCmdArg;
    const char *pszSettingsPw = NULL;
    const char *pszSettingsPwFile = NULL;
#ifndef VBOX_ONLY_DOCS
    int         cResponseFileArgs     = 0;
    char      **papszResponseFileArgs = NULL;
    char      **papszNewArgv          = NULL;
#endif

    for (int i = 1; i < argc || argc <= iCmd; i++)
    {
        if (    argc <= iCmd
            ||  !strcmp(argv[i], "help")
            ||  !strcmp(argv[i], "--help")
            ||  !strcmp(argv[i], "-?")
            ||  !strcmp(argv[i], "-h")
            ||  !strcmp(argv[i], "-help"))
        {
            if (i >= argc - 1)
            {
                showLogo(g_pStdOut);
                printUsage(USAGE_ALL, ~0U, g_pStdOut);
                return 0;
            }
            fShowLogo = true;
            fShowHelp = true;
            iCmd++;
            continue;
        }

#ifndef VBOX_ONLY_DOCS
        if (   !strcmp(argv[i], "-V")
            || !strcmp(argv[i], "--version")
            || !strcmp(argv[i], "-v")       /* deprecated */
            || !strcmp(argv[i], "-version") /* deprecated */
            || !strcmp(argv[i], "-Version") /* deprecated */)
        {
            /* Print version number, and do nothing else. */
            RTPrintf("%sr%u\n", VBOX_VERSION_STRING, RTBldCfgRevision());
            return 0;
        }
#endif

        if (   !strcmp(argv[i], "--dumpopts")
            || !strcmp(argv[i], "-dumpopts") /* deprecated */)
        {
            /* Special option to dump really all commands,
             * even the ones not understood on this platform. */
            printUsage(USAGE_DUMPOPTS, ~0U, g_pStdOut);
            return 0;
        }

        if (   !strcmp(argv[i], "--nologo")
            || !strcmp(argv[i], "-q")
            || !strcmp(argv[i], "-nologo") /* deprecated */)
        {
            /* suppress the logo */
            fShowLogo = false;
            iCmd++;
        }
        else if (   !strcmp(argv[i], "--detailed-progress")
                 || !strcmp(argv[i], "-d"))
        {
            /* detailed progress report */
            g_fDetailedProgress = true;
            iCmd++;
        }
        else if (!strcmp(argv[i], "--settingspw"))
        {
            if (i >= argc - 1)
                return RTMsgErrorExit(RTEXITCODE_FAILURE, "Password expected");
            /* password for certain settings */
            pszSettingsPw = argv[i + 1];
            iCmd += 2;
        }
        else if (!strcmp(argv[i], "--settingspwfile"))
        {
            if (i >= argc-1)
                return RTMsgErrorExit(RTEXITCODE_FAILURE, "No password file specified");
            pszSettingsPwFile = argv[i+1];
            iCmd += 2;
        }
#ifndef VBOX_ONLY_DOCS
        else if (argv[i][0] == '@')
        {
            if (papszResponseFileArgs)
                return RTMsgErrorExitFailure("Only one response file allowed");

            /* Load response file, making sure it's valid UTF-8. */
            char  *pszResponseFile;
            size_t cbResponseFile;
            vrc = RTFileReadAllEx(&argv[i][1], 0, RTFOFF_MAX, RTFILE_RDALL_O_DENY_NONE | RTFILE_RDALL_F_TRAILING_ZERO_BYTE,
                                  (void **)&pszResponseFile, &cbResponseFile);
            if (RT_FAILURE(vrc))
                return RTMsgErrorExitFailure("Error reading response file '%s': %Rrc", &argv[i][1], vrc);
            vrc = RTStrValidateEncoding(pszResponseFile);
            if (RT_FAILURE(vrc))
            {
                RTFileReadAllFree(pszResponseFile, cbResponseFile);
                return RTMsgErrorExitFailure("Invalid response file ('%s') encoding: %Rrc", &argv[i][1], vrc);
            }

            /* Parse it. */
            vrc = RTGetOptArgvFromString(&papszResponseFileArgs, &cResponseFileArgs, pszResponseFile,
                                         RTGETOPTARGV_CNV_QUOTE_BOURNE_SH, NULL);
            RTFileReadAllFree(pszResponseFile, cbResponseFile);
            if (RT_FAILURE(vrc))
                return RTMsgErrorExitFailure("Failed to parse response file '%s' (bourne shell style): %Rrc", &argv[i][1], vrc);

            /* Construct new argv+argc with the response file arguments inserted. */
            int cNewArgs = argc + cResponseFileArgs;
            papszNewArgv = (char **)RTMemAllocZ((cNewArgs + 2) * sizeof(papszNewArgv[0]));
            if (!papszNewArgv)
                return RTMsgErrorExitFailure("out of memory");
            memcpy(&papszNewArgv[0], &argv[0], sizeof(argv[0]) * (i + 1));
            memcpy(&papszNewArgv[i + 1], papszResponseFileArgs, sizeof(argv[0]) * cResponseFileArgs);
            memcpy(&papszNewArgv[i + 1 + cResponseFileArgs], &argv[i + 1], sizeof(argv[0]) * (argc - i - 1 + 1));
            argv = papszNewArgv;
            argc = argc + cResponseFileArgs;

            iCmd++;
        }
#endif
        else
            break;
    }

    iCmdArg = iCmd + 1;

    /*
     * Show the logo and lookup the command and deal with fShowHelp = true.
     */
    if (fShowLogo)
        showLogo(g_pStdOut);

#ifndef VBOX_ONLY_DOCS
    PCVBMGCMD pCmd = lookupCommand(argv[iCmd]);
    if (pCmd && pCmd->enmCmdHelp != VBMG_CMD_TODO)
        setCurrentCommand(pCmd->enmCmdHelp);

    if (   pCmd
        && (   fShowHelp
            || (   argc - iCmdArg == 0
                && pCmd->enmHelpCat != 0)))
    {
        if (pCmd->enmCmdHelp == VBMG_CMD_TODO)
            printUsage(pCmd->enmHelpCat, ~0U, g_pStdOut);
        else if (fShowHelp)
            printHelp(g_pStdOut);
        else
            printUsage(g_pStdOut);
        return RTEXITCODE_FAILURE; /* error */
    }
    if (!pCmd)
    {
        if (!strcmp(argv[iCmd], "commands"))
        {
            RTPrintf("commands:\n");
            for (unsigned i = 0; i < RT_ELEMENTS(g_aCommands); i++)
                if (   i == 0  /* skip backwards compatibility entries */
                    || g_aCommands[i].enmHelpCat != g_aCommands[i - 1].enmHelpCat)
                    RTPrintf("    %s\n", g_aCommands[i].pszCommand);
            return RTEXITCODE_SUCCESS;
        }
        return errorSyntax(USAGE_ALL, "Invalid command '%s'", argv[iCmd]);
    }

    RTEXITCODE rcExit;
    if (!(pCmd->fFlags & VBMG_CMD_F_NO_COM))
    {
        /*
         * Initialize COM.
         */
        using namespace com;
        HRESULT hrc = com::Initialize();
        if (FAILED(hrc))
        {
# ifdef VBOX_WITH_XPCOM
            if (hrc == NS_ERROR_FILE_ACCESS_DENIED)
            {
                char szHome[RTPATH_MAX] = "";
                com::GetVBoxUserHomeDirectory(szHome, sizeof(szHome));
                return RTMsgErrorExit(RTEXITCODE_FAILURE,
                       "Failed to initialize COM because the global settings directory '%s' is not accessible!", szHome);
            }
# endif
            return RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed to initialize COM! (hrc=%Rhrc)", hrc);
        }


        /*
         * Get the remote VirtualBox object and create a local session object.
         */
        rcExit = RTEXITCODE_FAILURE;
        ComPtr<IVirtualBoxClient> virtualBoxClient;
        ComPtr<IVirtualBox> virtualBox;
        hrc = virtualBoxClient.createInprocObject(CLSID_VirtualBoxClient);
        if (SUCCEEDED(hrc))
            hrc = virtualBoxClient->COMGETTER(VirtualBox)(virtualBox.asOutParam());
        if (SUCCEEDED(hrc))
        {
            ComPtr<ISession> session;
            hrc = session.createInprocObject(CLSID_Session);
            if (SUCCEEDED(hrc))
            {
                /* Session secret. */
                if (pszSettingsPw)
                    CHECK_ERROR2I_STMT(virtualBox, SetSettingsSecret(Bstr(pszSettingsPw).raw()), rcExit = RTEXITCODE_FAILURE);
                else if (pszSettingsPwFile)
                    rcExit = settingsPasswordFile(virtualBox, pszSettingsPwFile);
                else
                    rcExit = RTEXITCODE_SUCCESS;
                if (rcExit == RTEXITCODE_SUCCESS)
                {
                    /*
                     * Call the handler.
                     */
                    HandlerArg handlerArg = { argc - iCmdArg, &argv[iCmdArg], virtualBox, session };
                    rcExit = pCmd->pfnHandler(&handlerArg);

                    /* Although all handlers should always close the session if they open it,
                     * we do it here just in case if some of the handlers contains a bug --
                     * leaving the direct session not closed will turn the machine state to
                     * Aborted which may have unwanted side effects like killing the saved
                     * state file (if the machine was in the Saved state before). */
                    session->UnlockMachine();
                }

                NativeEventQueue::getMainEventQueue()->processEventQueue(0);
            }
            else
            {
                com::ErrorInfo info;
                RTMsgError("Failed to create a session object!");
                if (!info.isFullAvailable() && !info.isBasicAvailable())
                    com::GluePrintRCMessage(hrc);
                else
                    com::GluePrintErrorInfo(info);
            }
        }
        else
        {
            com::ErrorInfo info;
            RTMsgError("Failed to create the VirtualBox object!");
            if (!info.isFullAvailable() && !info.isBasicAvailable())
            {
                com::GluePrintRCMessage(hrc);
                RTMsgError("Most likely, the VirtualBox COM server is not running or failed to start.");
            }
            else
                com::GluePrintErrorInfo(info);
        }

        /*
         * Terminate COM, make sure the virtualBox object has been released.
         */
        virtualBox.setNull();
        virtualBoxClient.setNull();
        NativeEventQueue::getMainEventQueue()->processEventQueue(0);
        com::Shutdown();
    }
    else
    {
        /*
         * The command needs no COM.
         */
        HandlerArg  handlerArg;
        handlerArg.argc = argc - iCmdArg;
        handlerArg.argv = &argv[iCmdArg];
        rcExit = pCmd->pfnHandler(&handlerArg);
    }

    if (papszResponseFileArgs)
    {
        RTGetOptArgvFree(papszResponseFileArgs);
        RTMemFree(papszNewArgv);
    }

    return rcExit;
#else  /* VBOX_ONLY_DOCS */
    return RTEXITCODE_SUCCESS;
#endif /* VBOX_ONLY_DOCS */
}
