/* $Id: VBoxAutostart.h $ */
/** @file
 * VBoxAutostart - VirtualBox Autostart service.
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

#ifndef __VBoxAutostart_h__
#define __VBoxAutostart_h__

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/getopt.h>
#include <iprt/types.h>

#include <VBox/cdefs.h>
#include <VBox/types.h>

#include <VBox/com/com.h>
#include <VBox/com/VirtualBox.h>

/*******************************************************************************
*   Constants And Macros, Structures and Typedefs                              *
*******************************************************************************/

/**
 * Config AST node types.
 */
typedef enum CFGASTNODETYPE
{
    /** Invalid. */
    CFGASTNODETYPE_INVALID = 0,
    /** Key/Value pair. */
    CFGASTNODETYPE_KEYVALUE,
    /** Compound type. */
    CFGASTNODETYPE_COMPOUND,
    /** List type. */
    CFGASTNODETYPE_LIST,
    /** 32bit hack. */
    CFGASTNODETYPE_32BIT_HACK = 0x7fffffff
} CFGASTNODETYPE;
/** Pointer to a config AST node type. */
typedef CFGASTNODETYPE *PCFGASTNODETYPE;
/** Pointer to a const config AST node type. */
typedef const CFGASTNODETYPE *PCCFGASTNODETYPE;

/**
 * Config AST.
 */
typedef struct CFGAST
{
    /** AST node type. */
    CFGASTNODETYPE        enmType;
    /** Key or scope id. */
    char                 *pszKey;
    /** Type dependent data. */
    union
    {
        /** Key value pair. */
        struct
        {
            /** Number of characters in the value - excluding terminator. */
            size_t        cchValue;
            /** Value string - variable in size. */
            char          aszValue[1];
        } KeyValue;
        /** Compound type. */
        struct
        {
            /** Number of AST node entries in the array. */
            unsigned       cAstNodes;
            /** AST node array - variable in size. */
            struct CFGAST *apAstNodes[1];
        } Compound;
        /** List type. */
        struct
        {
            /** Number of entries in the list. */
            unsigned       cListEntries;
            /** Array of list entries - variable in size. */
            char          *apszEntries[1];
        } List;
    } u;
} CFGAST, *PCFGAST;

/** Flag whether we are in verbose logging mode. */
extern bool                      g_fVerbose;
/** Handle to the VirtualBox interface. */
extern ComPtr<IVirtualBox>       g_pVirtualBox;
/** Handle to the session interface. */
extern ComPtr<ISession>          g_pSession;
/** handle to the VirtualBox interface. */
extern ComPtr<IVirtualBoxClient> g_pVirtualBoxClient;
/**
 * System log type.
 */
typedef enum AUTOSTARTLOGTYPE
{
    /** Invalid log type. */
    AUTOSTARTLOGTYPE_INVALID = 0,
    /** Log info message. */
    AUTOSTARTLOGTYPE_INFO,
    /** Log error message. */
    AUTOSTARTLOGTYPE_ERROR,
    /** Log warning message. */
    AUTOSTARTLOGTYPE_WARNING,
    /** Log verbose message, only if verbose mode is activated. */
    AUTOSTARTLOGTYPE_VERBOSE,
    /** Famous 32bit hack. */
    AUTOSTARTLOGTYPE_32BIT_HACK = 0x7fffffff
} AUTOSTARTLOGTYPE;

/**
 * Log messages to the system and release log.
 *
 * @returns nothing.
 * @param   pszMsg            Message to log.
 * @param   enmLogType        Log type to use.
 */
DECLHIDDEN(void) autostartSvcOsLogStr(const char *pszMsg, AUTOSTARTLOGTYPE enmLogType);

/**
 * Print out progress on the console.
 *
 * This runs the main event queue every now and then to prevent piling up
 * unhandled things (which doesn't cause real problems, just makes things
 * react a little slower than in the ideal case).
 */
DECLHIDDEN(HRESULT) showProgress(ComPtr<IProgress> progress);

/**
 * Converts the machine state to a human readable string.
 *
 * @returns Pointer to the human readable state.
 * @param   enmMachineState    Machine state to convert.
 * @param   fShort             Flag whether to return a short form.
 */
DECLHIDDEN(const char *) machineStateToName(MachineState_T enmMachineState, bool fShort);

/**
 * Parse the given configuration file and return the interesting config parameters.
 *
 * @returns VBox status code.
 * @param   pszFilename    The config file to parse.
 * @param   ppCfgAst       Where to store the pointer to the root AST node on success.
 */
DECLHIDDEN(int) autostartParseConfig(const char *pszFilename, PCFGAST *ppCfgAst);

/**
 * Destroys the config AST and frees all resources.
 *
 * @returns nothing.
 * @param   pCfgAst        The config AST.
 */
DECLHIDDEN(void) autostartConfigAstDestroy(PCFGAST pCfgAst);

/**
 * Return the config AST node with the given name or NULL if it doesn't exist.
 *
 * @returns Matching config AST node for the given name or NULL if not found.
 * @param   pCfgAst    The config ASt to search.
 * @param   pszName    The name to search for.
 */
DECLHIDDEN(PCFGAST) autostartConfigAstGetByName(PCFGAST pCfgAst, const char *pszName);

/**
 * Main routine for the autostart daemon.
 *
 * @returns exit status code.
 * @param   pCfgAst        Config AST for the startup part of the autostart daemon.
 */
DECLHIDDEN(RTEXITCODE) autostartStartMain(PCFGAST pCfgAst);

/**
 * Main routine for the autostart daemon when stopping virtual machines
 * during system shutdown.
 *
 * @returns exit status code.
 * @param   pCfgAst        Config AST for the shutdown part of the autostart daemon.
 */
DECLHIDDEN(RTEXITCODE) autostartStopMain(PCFGAST pCfgAst);

/**
 * Logs a verbose message to the appropriate system log.
 *
 * @param   pszFormat   The log string. No trailing newline.
 * @param   ...         Format arguments.
 */
DECLHIDDEN(void) autostartSvcLogVerboseV(const char *pszFormat, va_list va);

/**
 * Logs a verbose message to the appropriate system log.
 *
 * @param   pszFormat   The log string. No trailing newline.
 * @param   ...         Format arguments.
 */
DECLHIDDEN(void) autostartSvcLogVerbose(const char *pszFormat, ...);

/**
 * Logs a warning message to the appropriate system log.
 *
 * @param   pszFormat   The log string. No trailing newline.
 * @param   ...         Format arguments.
 */
DECLHIDDEN(void) autostartSvcLogWarningV(const char *pszFormat, va_list va);

/**
 * Logs a warning message to the appropriate system log.
 *
 * @param   pszFormat   The log string. No trailing newline.
 * @param   ...         Format arguments.
 */
DECLHIDDEN(void) autostartSvcLogWarning(const char *pszFormat, ...);

/**
 * Logs a info message to the appropriate system log.
 *
 * @param   pszFormat   The log string. No trailing newline.
 * @param   ...         Format arguments.
 */
DECLHIDDEN(void) autostartSvcLogInfoV(const char *pszFormat, va_list va);

/**
 * Logs a info message to the appropriate system log.
 *
 * @param   pszFormat   The log string. No trailing newline.
 * @param   ...         Format arguments.
 */
DECLHIDDEN(void) autostartSvcLogInfo(const char *pszFormat, ...);

/**
 * Logs the message to the appropriate system log.
 *
 * In debug builds this will also put it in the debug log.
 *
 * @param   pszFormat   The log string. No trailing newline.
 * @param   ...         Format arguments.
 *
 * @todo    This should later be replaced by the release logger and callback destination(s).
 */
DECLHIDDEN(void) autostartSvcLogErrorV(const char *pszFormat, va_list va);

/**
 * Logs the error message to the appropriate system log.
 *
 * In debug builds this will also put it in the debug log.
 *
 * @param   pszFormat   The log string. No trailing newline.
 * @param   ...         Format arguments.
 *
 * @todo    This should later be replaced by the release logger and callback destination(s).
 */
DECLHIDDEN(void) autostartSvcLogError(const char *pszFormat, ...);

/**
 * Deals with RTGetOpt failure, bitching in the system log.
 *
 * @returns 1
 * @param   pszAction       The action name.
 * @param   rc              The RTGetOpt return value.
 * @param   argc            The argument count.
 * @param   argv            The argument vector.
 * @param   iArg            The argument index.
 * @param   pValue          The value returned by RTGetOpt.
 */
DECLHIDDEN(RTEXITCODE) autostartSvcLogGetOptError(const char *pszAction, int rc, int argc, char **argv, int iArg, PCRTGETOPTUNION pValue);

/**
 * Bitch about too many arguments (after RTGetOpt stops) in the system log.
 *
 * @returns 1
 * @param   pszAction       The action name.
 * @param   argc            The argument count.
 * @param   argv            The argument vector.
 * @param   iArg            The argument index.
 */
DECLHIDDEN(RTEXITCODE) autostartSvcLogTooManyArgsError(const char *pszAction, int argc, char **argv, int iArg);

/**
 * Prints an error message to the screen.
 *
 * @param   pszFormat   The message format string.
 * @param   va          Format arguments.
 */
DECLHIDDEN(void) autostartSvcDisplayErrorV(const char *pszFormat, va_list va);

/**
 * Prints an error message to the screen.
 *
 * @param   pszFormat   The message format string.
 * @param   ...         Format arguments.
 */
DECLHIDDEN(void) autostartSvcDisplayError(const char *pszFormat, ...);

/**
 * Deals with RTGetOpt failure.
 *
 * @returns 1
 * @param   pszAction       The action name.
 * @param   rc              The RTGetOpt return value.
 * @param   argc            The argument count.
 * @param   argv            The argument vector.
 * @param   iArg            The argument index.
 * @param   pValue          The value returned by RTGetOpt.
 */
DECLHIDDEN(RTEXITCODE) autostartSvcDisplayGetOptError(const char *pszAction, int rc, int argc, char **argv, int iArg, PCRTGETOPTUNION pValue);

/**
 * Bitch about too many arguments (after RTGetOpt stops).
 *
 * @returns RTEXITCODE_FAILURE
 * @param   pszAction       The action name.
 * @param   argc            The argument count.
 * @param   argv            The argument vector.
 * @param   iArg            The argument index.
 */
DECLHIDDEN(RTEXITCODE) autostartSvcDisplayTooManyArgsError(const char *pszAction, int argc, char **argv, int iArg);

DECLHIDDEN(int) autostartSetup();

DECLHIDDEN(void) autostartShutdown();

#endif /* __VBoxAutostart_h__ */

