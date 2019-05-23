/* $Id: VBoxManageBandwidthControl.cpp $ */
/** @file
 * VBoxManage - The bandwidth control related commands.
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

#ifndef VBOX_ONLY_DOCS


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/com/com.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/path.h>
#include <iprt/param.h>
#include <iprt/string.h>
#include <iprt/ctype.h>
#include <iprt/stream.h>
#include <iprt/getopt.h>
#include <VBox/log.h>

#include "VBoxManage.h"
using namespace com;


// funcs
///////////////////////////////////////////////////////////////////////////////


/**
 * Parses a string in the following format "n[k|m|g|K|M|G]". Stores the value
 * of n expressed in bytes to *pLimit. k meas kilobit, while K means kilobyte.
 *
 * @returns Error message or NULL if successful.
 * @param   pcszLimit       The string to parse.
 * @param   pLimit          Where to store the result.
 */
static const char *parseLimit(const char *pcszLimit, int64_t *pLimit)
{
    int iMultiplier = _1M;
    char *pszNext = NULL;
    int rc = RTStrToInt64Ex(pcszLimit, &pszNext, 10, pLimit);

    switch (rc)
    {
        case VINF_SUCCESS:
            break;
        case VWRN_NUMBER_TOO_BIG:
            return "Limit is too big\n";
        case VWRN_TRAILING_CHARS:
            switch (*pszNext)
            {
                case 'G': iMultiplier = _1G;       break;
                case 'M': iMultiplier = _1M;       break;
                case 'K': iMultiplier = _1K;       break;
                case 'g': iMultiplier = 125000000; break;
                case 'm': iMultiplier = 125000;    break;
                case 'k': iMultiplier = 125;       break;
                default:  return "Invalid unit suffix. Valid suffixes are: k, m, g, K, M, G\n";
            }
            break;
        case VWRN_TRAILING_SPACES:
            return "Trailing spaces in limit!\n";
        case VERR_NO_DIGITS:
            return "No digits in limit specifier\n";
        default:
            return "Invalid limit specifier\n";
    }
    if (*pLimit < 0)
        return "Limit cannot be negative\n";
    if (*pLimit > INT64_MAX / iMultiplier)
        return "Limit is too big\n";
    *pLimit *= iMultiplier;

    return NULL;
}

/**
 * Handles the 'bandwidthctl myvm add' sub-command.
 * @returns Exit code.
 * @param   a               The handler argument package.
 * @param   bwCtrl          Reference to the bandwidth control interface.
 */
static RTEXITCODE handleBandwidthControlAdd(HandlerArg *a, ComPtr<IBandwidthControl> &bwCtrl)
{
    HRESULT rc = S_OK;
    static const RTGETOPTDEF g_aBWCtlAddOptions[] =
        {
            { "--type",   't', RTGETOPT_REQ_STRING },
            { "--limit",  'l', RTGETOPT_REQ_STRING }
        };


    Bstr name(a->argv[2]);
    if (name.isEmpty())
    {
        errorArgument("Bandwidth group name must not be empty!\n");
        return RTEXITCODE_FAILURE;
    }

    const char *pszType  = NULL;
    int64_t cMaxBytesPerSec = INT64_MAX;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, a->argc, a->argv, g_aBWCtlAddOptions,
                 RT_ELEMENTS(g_aBWCtlAddOptions), 3, RTGETOPTINIT_FLAGS_NO_STD_OPTS);

    while (   SUCCEEDED(rc)
           && (c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 't':   // bandwidth group type
            {
                if (ValueUnion.psz)
                    pszType = ValueUnion.psz;
                else
                    rc = E_FAIL;
                break;
            }

            case 'l': // limit
            {
                if (ValueUnion.psz)
                {
                    const char *pcszError = parseLimit(ValueUnion.psz, &cMaxBytesPerSec);
                    if (pcszError)
                    {
                        errorArgument(pcszError);
                        return RTEXITCODE_FAILURE;
                    }
                }
                else
                    rc = E_FAIL;
                break;
            }

            default:
            {
                errorGetOpt(USAGE_BANDWIDTHCONTROL, c, &ValueUnion);
                rc = E_FAIL;
                break;
            }
        }
    }

    BandwidthGroupType_T enmType;

    if (!RTStrICmp(pszType, "disk"))
        enmType = BandwidthGroupType_Disk;
    else if (!RTStrICmp(pszType, "network"))
        enmType = BandwidthGroupType_Network;
    else
    {
        errorArgument("Invalid bandwidth group type\n");
        return RTEXITCODE_FAILURE;
    }

    CHECK_ERROR2I_RET(bwCtrl, CreateBandwidthGroup(name.raw(), enmType, (LONG64)cMaxBytesPerSec), RTEXITCODE_FAILURE);

    return RTEXITCODE_SUCCESS;
}

/**
 * Handles the 'bandwidthctl myvm set' sub-command.
 * @returns Exit code.
 * @param   a               The handler argument package.
 * @param   bwCtrl          Reference to the bandwidth control interface.
 */
static RTEXITCODE handleBandwidthControlSet(HandlerArg *a, ComPtr<IBandwidthControl> &bwCtrl)
{
    HRESULT rc = S_OK;
    static const RTGETOPTDEF g_aBWCtlAddOptions[] =
        {
            { "--limit",  'l', RTGETOPT_REQ_STRING }
        };


    Bstr name(a->argv[2]);
    int64_t cMaxBytesPerSec = INT64_MAX;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, a->argc, a->argv, g_aBWCtlAddOptions,
                 RT_ELEMENTS(g_aBWCtlAddOptions), 3, RTGETOPTINIT_FLAGS_NO_STD_OPTS);

    while (   SUCCEEDED(rc)
           && (c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'l': // limit
            {
                if (ValueUnion.psz)
                {
                    const char *pcszError = parseLimit(ValueUnion.psz, &cMaxBytesPerSec);
                    if (pcszError)
                    {
                        errorArgument(pcszError);
                        return RTEXITCODE_FAILURE;
                    }
                }
                else
                    rc = E_FAIL;
                break;
            }

            default:
            {
                errorGetOpt(USAGE_BANDWIDTHCONTROL, c, &ValueUnion);
                rc = E_FAIL;
                break;
            }
        }
    }


    if (cMaxBytesPerSec != INT64_MAX)
    {
        ComPtr<IBandwidthGroup> bwGroup;
        CHECK_ERROR2I_RET(bwCtrl, GetBandwidthGroup(name.raw(), bwGroup.asOutParam()), RTEXITCODE_FAILURE);
        if (SUCCEEDED(rc))
        {
            CHECK_ERROR2I_RET(bwGroup, COMSETTER(MaxBytesPerSec)((LONG64)cMaxBytesPerSec), RTEXITCODE_FAILURE);
        }
    }

    return RTEXITCODE_SUCCESS;
}

/**
 * Handles the 'bandwidthctl myvm remove' sub-command.
 * @returns Exit code.
 * @param   a               The handler argument package.
 * @param   bwCtrl          Reference to the bandwidth control interface.
 */
static RTEXITCODE handleBandwidthControlRemove(HandlerArg *a, ComPtr<IBandwidthControl> &bwCtrl)
{
    Bstr name(a->argv[2]);
    CHECK_ERROR2I_RET(bwCtrl, DeleteBandwidthGroup(name.raw()), RTEXITCODE_FAILURE);
    return RTEXITCODE_SUCCESS;
}

/**
 * Handles the 'bandwidthctl myvm list' sub-command.
 * @returns Exit code.
 * @param   a               The handler argument package.
 * @param   bwCtrl          Reference to the bandwidth control interface.
 */
static RTEXITCODE handleBandwidthControlList(HandlerArg *pArgs, ComPtr<IBandwidthControl> &rptrBWControl)
{
    static const RTGETOPTDEF g_aOptions[] =
    {
        { "--machinereadable",  'M', RTGETOPT_REQ_NOTHING },
    };

    VMINFO_DETAILS enmDetails = VMINFO_STANDARD;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, pArgs->argc, pArgs->argv, g_aOptions, RT_ELEMENTS(g_aOptions), 2 /*iArg*/, 0 /*fFlags*/);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'M':
                enmDetails = VMINFO_MACHINEREADABLE;
                break;
            default:
                return errorGetOpt(USAGE_BANDWIDTHCONTROL, c, &ValueUnion);
        }
    }

    if (FAILED(showBandwidthGroups(rptrBWControl, enmDetails)))
        return RTEXITCODE_FAILURE;

    return RTEXITCODE_SUCCESS;
}


/**
 * Handles the 'bandwidthctl' command.
 * @returns Exit code.
 * @param   a               The handler argument package.
 */
RTEXITCODE handleBandwidthControl(HandlerArg *a)
{
    HRESULT rc = S_OK;
    ComPtr<IMachine> machine;
    ComPtr<IBandwidthControl> bwCtrl;

    if (a->argc < 2)
        return errorSyntax(USAGE_BANDWIDTHCONTROL, "Too few parameters");
    else if (a->argc > 7)
        return errorSyntax(USAGE_BANDWIDTHCONTROL, "Too many parameters");

    /* try to find the given machine */
    CHECK_ERROR_RET(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                               machine.asOutParam()), RTEXITCODE_FAILURE);

    /* open a session for the VM (new or shared) */
    CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Shared), RTEXITCODE_FAILURE);
    SessionType_T st;
    CHECK_ERROR_RET(a->session, COMGETTER(Type)(&st), RTEXITCODE_FAILURE);
    bool fRunTime = (st == SessionType_Shared);

    /* get the mutable session machine */
    a->session->COMGETTER(Machine)(machine.asOutParam());
    rc = machine->COMGETTER(BandwidthControl)(bwCtrl.asOutParam());
    if (FAILED(rc)) goto leave;

    if (!strcmp(a->argv[1], "add"))
    {
        if (fRunTime)
        {
            errorArgument("Bandwidth groups cannot be created while the VM is running\n");
            goto leave;
        }
        rc = handleBandwidthControlAdd(a, bwCtrl) == RTEXITCODE_SUCCESS ? S_OK : E_FAIL;
    }
    else if (!strcmp(a->argv[1], "remove"))
    {
        if (fRunTime)
        {
            errorArgument("Bandwidth groups cannot be deleted while the VM is running\n");
            goto leave;
        }
        rc = handleBandwidthControlRemove(a, bwCtrl) == RTEXITCODE_SUCCESS ? S_OK : E_FAIL;
    }
    else if (!strcmp(a->argv[1], "set"))
        rc = handleBandwidthControlSet(a, bwCtrl) == RTEXITCODE_SUCCESS ? S_OK : E_FAIL;
    else if (!strcmp(a->argv[1], "list"))
        rc = handleBandwidthControlList(a, bwCtrl) == RTEXITCODE_SUCCESS ? S_OK : E_FAIL;
    else
    {
        errorSyntax(USAGE_BANDWIDTHCONTROL, "Invalid parameter '%s'", Utf8Str(a->argv[1]).c_str());
        rc = E_FAIL;
    }

    /* commit changes */
    if (SUCCEEDED(rc))
        CHECK_ERROR(machine, SaveSettings());

leave:
    /* it's important to always close sessions */
    a->session->UnlockMachine();

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

#endif /* !VBOX_ONLY_DOCS */

