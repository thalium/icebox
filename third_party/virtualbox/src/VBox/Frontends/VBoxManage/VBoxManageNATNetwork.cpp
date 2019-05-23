/* $Id: VBoxManageNATNetwork.cpp $ */
/** @file
 * VBoxManage - Implementation of NAT Network command command.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
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

#ifndef RT_OS_WINDOWS
# include <netinet/in.h>
#else
/* from  <ws2ipdef.h> */
# define INET6_ADDRSTRLEN 65
#endif

#define IPv6

#include <iprt/cdefs.h>
#include <iprt/cidr.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/net.h>
#include <iprt/getopt.h>
#include <iprt/ctype.h>

#include <VBox/log.h>

#include <vector>
#include <string>

#include "VBoxManage.h"
#include "VBoxPortForwardString.h"

#ifndef VBOX_ONLY_DOCS

using namespace com;

typedef enum
{
    OP_ADD = 1000,
    OP_REMOVE,
    OP_MODIFY,
    OP_START,
    OP_STOP
} OPCODE;

typedef struct PFNAME2DELETE
{
    char szName[PF_NAMELEN];
    bool fIPv6;
} PFNAME2DELETE, *PPFNAME2DELETE;

typedef std::vector<PFNAME2DELETE> VPF2DELETE;
typedef VPF2DELETE::const_iterator VPF2DELETEITERATOR;

typedef std::vector<PORTFORWARDRULE> VPF2ADD;
typedef VPF2ADD::const_iterator VPF2ADDITERATOR;

typedef std::vector<std::string>  LOOPBACK2DELETEADD;
typedef LOOPBACK2DELETEADD::iterator LOOPBACK2DELETEADDITERATOR;

static HRESULT printNATNetwork(const ComPtr<INATNetwork> &pNATNet)
{
    HRESULT rc;

    do
    {
        Bstr strVal;
        CHECK_ERROR_BREAK(pNATNet, COMGETTER(NetworkName)(strVal.asOutParam()));
        RTPrintf("Name:        %ls\n", strVal.raw());
        CHECK_ERROR_BREAK(pNATNet, COMGETTER(Network)(strVal.asOutParam()));
        RTPrintf("Network:     %ls\n", strVal.raw());
        CHECK_ERROR_BREAK(pNATNet, COMGETTER(Gateway)(strVal.asOutParam()));
        RTPrintf("Gateway:     %ls\n", strVal.raw());
        BOOL fVal;
        CHECK_ERROR_BREAK(pNATNet, COMGETTER(IPv6Enabled)(&fVal));
        RTPrintf("IPv6:        %s\n",  fVal ? "Yes" : "No");
        if (fVal)
        {
            CHECK_ERROR_BREAK(pNATNet, COMGETTER(IPv6Prefix)(strVal.asOutParam()));
            RTPrintf("IPv6 Prefix: %s\n", strVal.raw());
        }
        CHECK_ERROR_BREAK(pNATNet, COMGETTER(Enabled)(&fVal));
        RTPrintf("Enabled:     %s\n",  fVal ? "Yes" : "No");
        /** @todo Add more information here. */
        RTPrintf("\n");

    } while (0);

    return rc;
}

static RTEXITCODE handleNATList(HandlerArg *a)
{
    HRESULT rc;

    RTPrintf("NAT Networks:\n\n");

    const char *pszFilter = NULL;
    if (a->argc > 1)
        pszFilter = a->argv[1];

    size_t cFound = 0;

    com::SafeIfaceArray<INATNetwork> arrNetNets;
    CHECK_ERROR(a->virtualBox, COMGETTER(NATNetworks)(ComSafeArrayAsOutParam(arrNetNets)));
    for (size_t i = 0; i < arrNetNets.size(); ++i)
    {
        ComPtr<INATNetwork> pNATNet = arrNetNets[i];

        if (pszFilter)
        {
            Bstr strVal;
            CHECK_ERROR_BREAK(pNATNet, COMGETTER(NetworkName)(strVal.asOutParam()));

            Utf8Str strValUTF8 = Utf8Str(strVal);
            if (!RTStrSimplePatternMatch(pszFilter,  strValUTF8.c_str()))
                continue;
        }

        if (i > 0)
            RTPrintf("\n");
        rc = printNATNetwork(pNATNet);
        if (FAILED(rc))
            break;

        cFound++;
    }

    if (SUCCEEDED(rc))
        RTPrintf("%zu %s found\n", cFound, cFound == 1 ? "network" : "networks");

    return SUCCEEDED(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}

static RTEXITCODE handleOp(HandlerArg *a, OPCODE enmCode)
{
    if (a->argc - 1 <= 1)
        return errorSyntax(USAGE_NATNETWORK, "Not enough parameters");

    const char *pNetName = NULL;
    const char *pNetworkCidr = NULL;
    int enable = -1;
    int dhcp = -1;
    int ipv6 = -1;

    VPF2DELETE vPfName2Delete;
    VPF2ADD vPf2Add;

    LOOPBACK2DELETEADD vLoopback2Delete;
    LOOPBACK2DELETEADD vLoopback2Add;

    LONG loopback6Offset = 0; /* ignore me */

    static const RTGETOPTDEF g_aNATNetworkIPOptions[] =
    {
        { "--netname",          't', RTGETOPT_REQ_STRING  },
        { "--network",          'n', RTGETOPT_REQ_STRING  },
        { "--dhcp",             'h', RTGETOPT_REQ_BOOL    },
        { "--ipv6",             '6', RTGETOPT_REQ_BOOL    },
        { "--enable",           'e', RTGETOPT_REQ_NOTHING },
        { "--disable",          'd', RTGETOPT_REQ_NOTHING },
        { "--port-forward-4",   'p', RTGETOPT_REQ_STRING  },
        { "--port-forward-6",   'P', RTGETOPT_REQ_STRING  },
        { "--loopback-4",       'l', RTGETOPT_REQ_STRING  },
        { "--loopback-6",       'L', RTGETOPT_REQ_STRING  },
    };

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, a->argc, a->argv, g_aNATNetworkIPOptions,
                 enmCode != OP_REMOVE ? RT_ELEMENTS(g_aNATNetworkIPOptions) : 4, /* we use only --netname and --ifname for remove*/
                 1, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        switch (c)
        {
            case 't':   // --netname
                if (pNetName)
                    return errorSyntax(USAGE_NATNETWORK, "You can only specify --netname only once.");
                pNetName = ValueUnion.psz;
                break;

            case 'n':   // --network
                if (pNetworkCidr)
                    return errorSyntax(USAGE_NATNETWORK, "You can only specify --network only once.");
                pNetworkCidr = ValueUnion.psz;
                break;

            case 'e':   // --enable
                if (enable >= 0)
                    return errorSyntax(USAGE_NATNETWORK, "You can specify either --enable or --disable once.");
                enable = 1;
                break;

            case 'd':   // --disable
                if (enable >= 0)
                    return errorSyntax(USAGE_NATNETWORK, "You can specify either --enable or --disable once.");
                enable = 0;
                break;

            case 'h':
                if (dhcp != -1)
                    return errorSyntax(USAGE_NATNETWORK, "You can specify --dhcp only once.");
                dhcp = ValueUnion.f;
                break;

            case '6':
                if (ipv6 != -1)
                    return errorSyntax(USAGE_NATNETWORK, "You can specify --ipv6 only once.");
                ipv6 = ValueUnion.f;
                break;

            case 'L': /* ipv6 loopback */
            case 'l': /* ipv4 loopback */
                if (RTStrCmp(ValueUnion.psz, "delete") == 0)
                {
                    /* deletion */
                    if (enmCode != OP_MODIFY)
                      errorSyntax(USAGE_NATNETWORK,
                                  "loopback couldn't be deleted on modified\n");
                    if (c == 'L')
                        loopback6Offset = -1;
                    else
                    {
                        int vrc;
                        RTGETOPTUNION Addr2Delete;
                        vrc = RTGetOptFetchValue(&GetState,
                                                 &Addr2Delete,
                                                 RTGETOPT_REQ_STRING);
                        if (RT_FAILURE(vrc))
                          return errorSyntax(USAGE_NATNETWORK,
                                             "Not enough parmaters\n");

                        vLoopback2Delete.push_back(std::string(Addr2Delete.psz));
                    }
                }
                else
                {
                    /* addition */
                    if (c == 'L')
                        loopback6Offset = ValueUnion.u32;
                    else
                        vLoopback2Add.push_back(std::string(ValueUnion.psz));
                }
                break;

            case 'P': /* ipv6 portforwarding*/
            case 'p': /* ipv4 portforwarding */
            {
                if (RTStrCmp(ValueUnion.psz, "delete") != 0)
                {
                    /* addition */
                    /* netPfStrToPf will clean up the Pfr */
                    PORTFORWARDRULE Pfr;
                    int irc = netPfStrToPf(ValueUnion.psz, (c == 'P'), &Pfr);
                    if (RT_FAILURE(irc))
                        return errorSyntax(USAGE_NATNETWORK, "Invalid port-forward rule %s\n", ValueUnion.psz);

                    vPf2Add.push_back(Pfr);
                }
                else
                {
                    /* deletion */
                    if (enmCode != OP_MODIFY)
                        return errorSyntax(USAGE_NATNETWORK,
                                           "Port-forward could be deleted on modify \n");

                    RTGETOPTUNION NamePf2DeleteUnion;
                    int vrc = RTGetOptFetchValue(&GetState, &NamePf2DeleteUnion, RTGETOPT_REQ_STRING);
                    if (RT_FAILURE(vrc))
                        return errorSyntax(USAGE_NATNETWORK, "Not enough parmaters\n");

                    if (strlen(NamePf2DeleteUnion.psz) > PF_NAMELEN)
                        return errorSyntax(USAGE_NATNETWORK, "Port-forward rule name is too long\n");

                    PFNAME2DELETE Name2Delete;
                    RT_ZERO(Name2Delete);
                    RTStrCopy(Name2Delete.szName, PF_NAMELEN, NamePf2DeleteUnion.psz);
                    Name2Delete.fIPv6 = (c == 'P');
                    vPfName2Delete.push_back(Name2Delete);
                }
                break;
            }

            default:
                return errorGetOpt(USAGE_NATNETWORK, c, &ValueUnion);
        }
    }

    if (!pNetName)
        return errorSyntax(USAGE_NATNETWORK, "You need to specify the --netname option");
    /* verification */
    switch (enmCode)
    {
        case OP_ADD:
            if (!pNetworkCidr)
                return errorSyntax(USAGE_NATNETWORK, "You need to specify the --network option");
            break;
        case OP_MODIFY:
        case OP_REMOVE:
        case OP_START:
        case OP_STOP:
            break;
        default:
            AssertMsgFailedReturn(("Unknown operation (:%d)", enmCode), RTEXITCODE_FAILURE);
    }

    HRESULT rc;
    Bstr NetName;
    NetName = Bstr(pNetName);

    ComPtr<INATNetwork> net;
    rc = a->virtualBox->FindNATNetworkByName(NetName.mutableRaw(), net.asOutParam());
    if (enmCode == OP_ADD)
    {
        if (SUCCEEDED(rc))
            return errorArgument("NATNetwork server already exists");

        CHECK_ERROR(a->virtualBox, CreateNATNetwork(NetName.raw(), net.asOutParam()));
        if (FAILED(rc))
            return errorArgument("Failed to create the NAT network service");
    }
    else if (FAILED(rc))
        return errorArgument("NATNetwork server does not exist");

    switch (enmCode)
    {
        case OP_ADD:
        case OP_MODIFY:
        {
            if (pNetworkCidr)
            {
                CHECK_ERROR(net, COMSETTER(Network)(Bstr(pNetworkCidr).raw()));
                if (FAILED(rc))
                    return errorArgument("Failed to set configuration");
            }
            if (dhcp >= 0)
            {
                CHECK_ERROR(net, COMSETTER(NeedDhcpServer) ((BOOL)dhcp));
                if (FAILED(rc))
                    return errorArgument("Failed to set configuration");
            }

            if (ipv6 >= 0)
            {
                CHECK_ERROR(net, COMSETTER(IPv6Enabled) ((BOOL)ipv6));
                if (FAILED(rc))
                    return errorArgument("Failed to set configuration");
            }

            if (!vPfName2Delete.empty())
            {
                VPF2DELETEITERATOR it;
                for (it = vPfName2Delete.begin(); it != vPfName2Delete.end(); ++it)
                {
                    CHECK_ERROR(net, RemovePortForwardRule((BOOL)(*it).fIPv6,
                                                           Bstr((*it).szName).raw()));
                    if (FAILED(rc))
                        return errorArgument("Failed to delete pf");
                }
            }

            if (!vPf2Add.empty())
            {
                VPF2ADDITERATOR it;
                for (it = vPf2Add.begin(); it != vPf2Add.end(); ++it)
                {
                    NATProtocol_T proto = NATProtocol_TCP;
                    if ((*it).iPfrProto == IPPROTO_TCP)
                        proto = NATProtocol_TCP;
                    else if ((*it).iPfrProto == IPPROTO_UDP)
                        proto = NATProtocol_UDP;
                    else
                        continue; /* XXX: warning here. */

                    CHECK_ERROR(net, AddPortForwardRule((BOOL)(*it).fPfrIPv6,
                                                        Bstr((*it).szPfrName).raw(),
                                                        proto,
                                                        Bstr((*it).szPfrHostAddr).raw(),
                                                        (*it).u16PfrHostPort,
                                                        Bstr((*it).szPfrGuestAddr).raw(),
                                                        (*it).u16PfrGuestPort));
                    if (FAILED(rc))
                        return errorArgument("Failed to add pf");
                }
            }

            if (loopback6Offset)
            {
                if (loopback6Offset == -1)
                    loopback6Offset = 0; /* deletion */

                CHECK_ERROR_RET(net, COMSETTER(LoopbackIp6)(loopback6Offset), RTEXITCODE_FAILURE);
            }

            /* addLocalMapping (hostid, offset) */
            if (!vLoopback2Add.empty())
            {
                /* we're expecting stings 127.0.0.1=5 */
                LOOPBACK2DELETEADDITERATOR it;
                for (it = vLoopback2Add.begin();
                     it != vLoopback2Add.end();
                     ++it)
                {
                    std::string address, strOffset;
                    size_t pos = it->find('=');
                    LONG lOffset = 0;
                    Bstr bstrAddress;

                    AssertReturn(pos != std::string::npos, errorArgument("invalid loopback string"));

                    address = it->substr(0, pos);
                    strOffset = it->substr(pos + 1);

                    lOffset = RTStrToUInt32(strOffset.c_str());
                    AssertReturn(lOffset > 0, errorArgument("invalid loopback string"));

                    bstrAddress = Bstr(address.c_str());

                    CHECK_ERROR_RET(net, AddLocalMapping(bstrAddress.raw(), lOffset), RTEXITCODE_FAILURE);
                }
            }

            if (!vLoopback2Delete.empty())
            {
                /* we're expecting stings 127.0.0.1 */
                LOOPBACK2DELETEADDITERATOR it;
                for (it = vLoopback2Add.begin();
                     it != vLoopback2Add.end();
                     ++it)
                {
                    Bstr bstrAddress;
                    bstrAddress = Bstr(it->c_str());

                    CHECK_ERROR_RET(net, AddLocalMapping(bstrAddress.raw(), 0), RTEXITCODE_FAILURE);
                }
            }

            if (enable >= 0)
            {
                CHECK_ERROR(net, COMSETTER(Enabled) ((BOOL)enable));
                if (FAILED(rc))
                    return errorArgument("Failed to set configuration");
            }
            break;
        }
        case OP_REMOVE:
        {
            CHECK_ERROR(a->virtualBox, RemoveNATNetwork(net));
            if (FAILED(rc))
                return errorArgument("Failed to remove nat network");
            break;
        }
        case OP_START:
        {
            CHECK_ERROR(net, Start(Bstr("whatever").raw()));
            if (FAILED(rc))
                return errorArgument("Failed to start network");
            break;
        }
        case OP_STOP:
        {
            CHECK_ERROR(net, Stop());
            if (FAILED(rc))
                return errorArgument("Failed to start network");
            break;
        }
        default:;
    }
    return RTEXITCODE_SUCCESS;
}


RTEXITCODE handleNATNetwork(HandlerArg *a)
{
    if (a->argc < 1)
        return errorSyntax(USAGE_NATNETWORK, "Not enough parameters");

    RTEXITCODE rcExit;
    if (strcmp(a->argv[0], "modify") == 0)
        rcExit = handleOp(a, OP_MODIFY);
    else if (strcmp(a->argv[0], "add") == 0)
        rcExit = handleOp(a, OP_ADD);
    else if (strcmp(a->argv[0], "remove") == 0)
        rcExit = handleOp(a, OP_REMOVE);
    else if (strcmp(a->argv[0], "start") == 0)
        rcExit = handleOp(a, OP_START);
    else if (strcmp(a->argv[0], "stop") == 0)
        rcExit = handleOp(a, OP_STOP);
    else if (strcmp(a->argv[0], "list") == 0)
        rcExit = handleNATList(a);
    else
        rcExit = errorSyntax(USAGE_NATNETWORK, "Invalid parameter '%s'", Utf8Str(a->argv[0]).c_str());
    return rcExit;
}

#endif /* !VBOX_ONLY_DOCS */
