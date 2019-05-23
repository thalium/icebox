/* $Id: VBoxManageHostonly.cpp $ */
/** @file
 * VBoxManage - Implementation of hostonlyif command.
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
#include <VBox/com/com.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/VirtualBox.h>
#endif /* !VBOX_ONLY_DOCS */

#include <iprt/cidr.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/net.h>
#include <iprt/getopt.h>
#include <iprt/ctype.h>

#include <VBox/log.h>

#include "VBoxManage.h"

#ifndef VBOX_ONLY_DOCS
using namespace com;

static const RTGETOPTDEF g_aHostOnlyCreateOptions[] =
{
    { "--machinereadable",  'M', RTGETOPT_REQ_NOTHING },
};

#if defined(VBOX_WITH_NETFLT) && !defined(RT_OS_SOLARIS)
static RTEXITCODE handleCreate(HandlerArg *a)
{
    /*
     * Parse input.
     */
    bool fMachineReadable = false;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, a->argc, a->argv, g_aHostOnlyCreateOptions,
                 RT_ELEMENTS(g_aHostOnlyCreateOptions), 1, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    int c;
    while ((c = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        switch (c)
        {
            case 'M':   // --machinereadable
                fMachineReadable = true;
                break;

            default:
                return errorGetOpt(USAGE_HOSTONLYIFS, c, &ValueUnion);
        }
    }

    /*
     * Do the work.
     */
    ComPtr<IHost> host;
    CHECK_ERROR2I_RET(a->virtualBox, COMGETTER(Host)(host.asOutParam()), RTEXITCODE_FAILURE);

    ComPtr<IHostNetworkInterface> hif;
    ComPtr<IProgress> progress;

    CHECK_ERROR2I_RET(host, CreateHostOnlyNetworkInterface(hif.asOutParam(), progress.asOutParam()), RTEXITCODE_FAILURE);

    if (fMachineReadable)
    {
        CHECK_PROGRESS_ERROR_RET(progress, (""), RTEXITCODE_FAILURE);
    }
    else
    {
        /*HRESULT hrc =*/ showProgress(progress);
        CHECK_PROGRESS_ERROR_RET(progress, ("Failed to create the host-only adapter"), RTEXITCODE_FAILURE);
    }

    Bstr bstrName;
    CHECK_ERROR2I(hif, COMGETTER(Name)(bstrName.asOutParam()));

    if (fMachineReadable)
        RTPrintf("%ls", bstrName.raw());
    else
        RTPrintf("Interface '%ls' was successfully created\n", bstrName.raw());
    return RTEXITCODE_SUCCESS;
}

static RTEXITCODE  handleRemove(HandlerArg *a)
{
    /*
     * Parse input.
     */
    const char *pszName = NULL;
    int ch;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, a->argc, a->argv, NULL, 0, 1, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((ch = RTGetOpt(&GetState, &ValueUnion)) != 0)
        switch (ch)
        {
            case VINF_GETOPT_NOT_OPTION:
                if (pszName)
                    return errorSyntax(USAGE_HOSTONLYIFS, "Only one interface name can be specified");
                pszName = ValueUnion.psz;
                break;

            default:
                return errorGetOpt(USAGE_HOSTONLYIFS, ch, &ValueUnion);
        }
    if (!pszName)
        return errorSyntax(USAGE_HOSTONLYIFS, "No interface name was specified");

    /*
     * Do the work.
     */
    ComPtr<IHost> host;
    CHECK_ERROR2I_RET(a->virtualBox, COMGETTER(Host)(host.asOutParam()), RTEXITCODE_FAILURE);

    ComPtr<IHostNetworkInterface> hif;
    CHECK_ERROR2I_RET(host, FindHostNetworkInterfaceByName(Bstr(pszName).raw(), hif.asOutParam()), RTEXITCODE_FAILURE);

    Bstr guid;
    CHECK_ERROR2I_RET(hif, COMGETTER(Id)(guid.asOutParam()), RTEXITCODE_FAILURE);

    ComPtr<IProgress> progress;
    CHECK_ERROR2I_RET(host, RemoveHostOnlyNetworkInterface(guid.raw(), progress.asOutParam()), RTEXITCODE_FAILURE);

    /*HRESULT hrc =*/ showProgress(progress);
    CHECK_PROGRESS_ERROR_RET(progress, ("Failed to remove the host-only adapter"), RTEXITCODE_FAILURE);

    return RTEXITCODE_SUCCESS;
}
#endif

static const RTGETOPTDEF g_aHostOnlyIPOptions[]
    = {
        { "--dhcp",             'd', RTGETOPT_REQ_NOTHING },
        { "-dhcp",              'd', RTGETOPT_REQ_NOTHING },    // deprecated
        { "--ip",               'a', RTGETOPT_REQ_STRING },
        { "-ip",                'a', RTGETOPT_REQ_STRING },     // deprecated
        { "--netmask",          'm', RTGETOPT_REQ_STRING },
        { "-netmask",           'm', RTGETOPT_REQ_STRING },     // deprecated
        { "--ipv6",             'b', RTGETOPT_REQ_STRING },
        { "-ipv6",              'b', RTGETOPT_REQ_STRING },     // deprecated
        { "--netmasklengthv6",  'l', RTGETOPT_REQ_UINT8 },
        { "-netmasklengthv6",   'l', RTGETOPT_REQ_UINT8 }       // deprecated
      };

static RTEXITCODE handleIpConfig(HandlerArg *a)
{
    bool        fDhcp = false;
    bool        fNetmasklengthv6 = false;
    uint32_t    uNetmasklengthv6 = UINT32_MAX;
    const char *pszIpv6 = NULL;
    const char *pszIp = NULL;
    const char *pszNetmask = NULL;
    const char *pszName = NULL;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, a->argc, a->argv, g_aHostOnlyIPOptions, RT_ELEMENTS(g_aHostOnlyIPOptions),
                 1, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        switch (c)
        {
            case 'd':   // --dhcp
                fDhcp = true;
                break;
            case 'a':   // --ip
                if (pszIp)
                    RTMsgWarning("The --ip option is specified more than once");
                pszIp = ValueUnion.psz;
                break;
            case 'm':   // --netmask
                if (pszNetmask)
                    RTMsgWarning("The --netmask option is specified more than once");
                pszNetmask = ValueUnion.psz;
                break;
            case 'b':   // --ipv6
                if (pszIpv6)
                    RTMsgWarning("The --ipv6 option is specified more than once");
                pszIpv6 = ValueUnion.psz;
                break;
            case 'l':   // --netmasklengthv6
                if (fNetmasklengthv6)
                    RTMsgWarning("The --netmasklengthv6 option is specified more than once");
                fNetmasklengthv6 = true;
                uNetmasklengthv6 = ValueUnion.u8;
                break;
            case VINF_GETOPT_NOT_OPTION:
                if (pszName)
                    return errorSyntax(USAGE_HOSTONLYIFS, "Only one interface name can be specified");
                pszName = ValueUnion.psz;
                break;
            default:
                return errorGetOpt(USAGE_HOSTONLYIFS, c, &ValueUnion);
        }
    }

    /* parameter sanity check */
    if (fDhcp && (fNetmasklengthv6 || pszIpv6 || pszIp || pszNetmask))
        return errorSyntax(USAGE_HOSTONLYIFS, "You can not use --dhcp with static ip configuration parameters: --ip, --netmask, --ipv6 and --netmasklengthv6.");
    if ((pszIp || pszNetmask) && (fNetmasklengthv6 || pszIpv6))
        return errorSyntax(USAGE_HOSTONLYIFS, "You can not use ipv4 configuration (--ip and --netmask) with ipv6 (--ipv6 and --netmasklengthv6) simultaneously.");

    ComPtr<IHost> host;
    CHECK_ERROR2I_RET(a->virtualBox, COMGETTER(Host)(host.asOutParam()), RTEXITCODE_FAILURE);

    ComPtr<IHostNetworkInterface> hif;
    CHECK_ERROR2I_RET(host, FindHostNetworkInterfaceByName(Bstr(pszName).raw(), hif.asOutParam()), RTEXITCODE_FAILURE);
    if (hif.isNull())
        return errorArgument("Could not find interface '%s'", pszName);

    if (fDhcp)
        CHECK_ERROR2I_RET(hif, EnableDynamicIPConfig(), RTEXITCODE_FAILURE);
    else if (pszIp)
    {
        if (!pszNetmask)
            pszNetmask = "255.255.255.0"; /* ?? */
        CHECK_ERROR2I_RET(hif, EnableStaticIPConfig(Bstr(pszIp).raw(), Bstr(pszNetmask).raw()), RTEXITCODE_FAILURE);
    }
    else if (pszIpv6)
    {
        BOOL fIpV6Supported;
        CHECK_ERROR2I_RET(hif, COMGETTER(IPV6Supported)(&fIpV6Supported), RTEXITCODE_FAILURE);
        if (!fIpV6Supported)
        {
            RTMsgError("IPv6 setting is not supported for this adapter");
            return RTEXITCODE_FAILURE;
        }

        if (uNetmasklengthv6 == UINT32_MAX)
            uNetmasklengthv6 = 64; /* ?? */
        CHECK_ERROR2I_RET(hif, EnableStaticIPConfigV6(Bstr(pszIpv6).raw(), (ULONG)uNetmasklengthv6), RTEXITCODE_FAILURE);
    }
    else
        return errorSyntax(USAGE_HOSTONLYIFS, "Neither -dhcp nor -ip nor -ipv6 was specfified");

    return RTEXITCODE_SUCCESS;
}


RTEXITCODE handleHostonlyIf(HandlerArg *a)
{
    if (a->argc < 1)
        return errorSyntax(USAGE_HOSTONLYIFS, "No sub-command specified");

    RTEXITCODE rcExit;
    if (!strcmp(a->argv[0], "ipconfig"))
        rcExit = handleIpConfig(a);
#if defined(VBOX_WITH_NETFLT) && !defined(RT_OS_SOLARIS)
    else if (!strcmp(a->argv[0], "create"))
        rcExit = handleCreate(a);
    else if (!strcmp(a->argv[0], "remove"))
        rcExit = handleRemove(a);
#endif
    else
        rcExit = errorSyntax(USAGE_HOSTONLYIFS, "Unknown sub-command '%s'", a->argv[0]);
    return rcExit;
}

#endif /* !VBOX_ONLY_DOCS */
