/* $Id: VBoxManageDHCPServer.cpp $ */
/** @file
 * VBoxManage - Implementation of dhcpserver command.
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

#include <string>
#include <vector>
#include <map>

#ifndef VBOX_ONLY_DOCS
using namespace com;

typedef enum enMainOpCodes
{
    OP_ADD = 1000,
    OP_REMOVE,
    OP_MODIFY
} OPCODE;

typedef std::pair<DhcpOpt_T, std::string> DhcpOptSpec;
typedef std::vector<DhcpOptSpec> DhcpOpts;
typedef DhcpOpts::iterator DhcpOptIterator;

struct VmNameSlotKey
{
    const std::string VmName;
    uint8_t u8Slot;

    VmNameSlotKey(const std::string &aVmName, uint8_t aSlot)
      : VmName(aVmName)
      , u8Slot(aSlot)
    {}

    bool operator< (const VmNameSlotKey& that) const
    {
        if (VmName == that.VmName)
            return u8Slot < that.u8Slot;
        else
            return VmName < that.VmName;
    }
};

typedef std::map<VmNameSlotKey, DhcpOpts> VmSlot2OptionsM;
typedef VmSlot2OptionsM::iterator VmSlot2OptionsIterator;
typedef VmSlot2OptionsM::value_type VmSlot2OptionsPair;

typedef std::vector<VmNameSlotKey> VmConfigs;

static const RTGETOPTDEF g_aDHCPIPOptions[] =
{
    { "--netname",          't', RTGETOPT_REQ_STRING },  /* we use 't' instead of 'n' to avoid
                                                          * 1. the misspelled "-enable" long option to be treated as 'e' (for -enable) + 'n' (for -netname) + "<the_rest_opt>" (for net name)
                                                          * 2. the misspelled "-netmask" to be treated as 'n' (for -netname) + "<the_rest_opt>" (for net name)
                                                          */
    { "-netname",           't', RTGETOPT_REQ_STRING },     // deprecated (if removed check below)
    { "--ifname",           'f', RTGETOPT_REQ_STRING },  /* we use 'f' instead of 'i' to avoid
                                                          * 1. the misspelled "-disable" long option to be treated as 'd' (for -disable) + 'i' (for -ifname) + "<the_rest_opt>" (for if name)
                                                          */
    { "-ifname",            'f', RTGETOPT_REQ_STRING },     // deprecated
    { "--ip",               'a', RTGETOPT_REQ_STRING },
    { "-ip",                'a', RTGETOPT_REQ_STRING },     // deprecated
    { "--netmask",          'm', RTGETOPT_REQ_STRING },
    { "-netmask",           'm', RTGETOPT_REQ_STRING },     // deprecated
    { "--lowerip",          'l', RTGETOPT_REQ_STRING },
    { "-lowerip",           'l', RTGETOPT_REQ_STRING },     // deprecated
    { "--upperip",          'u', RTGETOPT_REQ_STRING },
    { "-upperip",           'u', RTGETOPT_REQ_STRING },     // deprecated
    { "--enable",           'e', RTGETOPT_REQ_NOTHING },
    { "-enable",            'e', RTGETOPT_REQ_NOTHING },    // deprecated
    { "--disable",          'd', RTGETOPT_REQ_NOTHING },
    { "-disable",           'd', RTGETOPT_REQ_NOTHING },     // deprecated
    { "--options",          'o', RTGETOPT_REQ_NOTHING },
    { "--vm",               'n', RTGETOPT_REQ_STRING}, /* only with -o */
    { "--slot",             's', RTGETOPT_REQ_UINT8}, /* only with -o and -n */
    { "--id",               'i', RTGETOPT_REQ_UINT8}, /* only with -o */
    { "--value",            'p', RTGETOPT_REQ_STRING} /* only with -i */
};

static RTEXITCODE handleOp(HandlerArg *a, OPCODE enmCode, int iStart)
{
    if (a->argc - iStart < 2)
        return errorSyntax(USAGE_DHCPSERVER, "Not enough parameters");

    int index = iStart;
    HRESULT rc;
    bool fOptionsRead = false;
    bool fVmOptionRead = false;

    const char *pszVmName = NULL;
    const char *pNetName = NULL;
    const char *pIfName = NULL;
    const char * pIp = NULL;
    const char * pNetmask = NULL;
    const char * pLowerIp = NULL;
    const char * pUpperIp = NULL;

    uint8_t u8OptId = (uint8_t)~0;
    uint8_t u8Slot = (uint8_t)~0;

    int enable = -1;

    DhcpOpts        GlobalDhcpOptions;
    VmSlot2OptionsM VmSlot2Options;
    VmConfigs       VmConfigs2Delete;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState,
                 a->argc,
                 a->argv,
                 g_aDHCPIPOptions,
                 enmCode != OP_REMOVE ? RT_ELEMENTS(g_aDHCPIPOptions) : 4, /* we use only --netname and --ifname for remove*/
                 index,
                 RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 't':   // --netname
                if(pNetName)
                    return errorSyntax(USAGE_DHCPSERVER, "You can only specify --netname once.");
                else if (pIfName)
                    return errorSyntax(USAGE_DHCPSERVER, "You can either use a --netname or --ifname for identifying the DHCP server.");
                else
                {
                    pNetName = ValueUnion.psz;
                }
            break;
            case 'f':   // --ifname
                if(pIfName)
                    return errorSyntax(USAGE_DHCPSERVER, "You can only specify --ifname once.");
                else if (pNetName)
                    return errorSyntax(USAGE_DHCPSERVER, "You can either use a --netname or --ipname for identifying the DHCP server.");
                else
                {
                    pIfName = ValueUnion.psz;
                }
            break;
            case 'a':   // -ip
                if(pIp)
                    return errorSyntax(USAGE_DHCPSERVER, "You can only specify --ip once.");
                else
                {
                    pIp = ValueUnion.psz;
                }
            break;
            case 'm':   // --netmask
                if(pNetmask)
                    return errorSyntax(USAGE_DHCPSERVER, "You can only specify --netmask once.");
                else
                {
                    pNetmask = ValueUnion.psz;
                }
            break;
            case 'l':   // --lowerip
                if(pLowerIp)
                    return errorSyntax(USAGE_DHCPSERVER, "You can only specify --lowerip once.");
                else
                {
                    pLowerIp = ValueUnion.psz;
                }
            break;
            case 'u':   // --upperip
                if(pUpperIp)
                    return errorSyntax(USAGE_DHCPSERVER, "You can only specify --upperip once.");
                else
                {
                    pUpperIp = ValueUnion.psz;
                }
            break;
            case 'e':   // --enable
                if(enable >= 0)
                    return errorSyntax(USAGE_DHCPSERVER, "You can specify either --enable or --disable once.");
                else
                {
                    enable = 1;
                }
            break;
            case 'd':   // --disable
                if(enable >= 0)
                    return errorSyntax(USAGE_DHCPSERVER, "You can specify either --enable or --disable once.");
                else
                {
                    enable = 0;
                }
            break;
            case VINF_GETOPT_NOT_OPTION:
                return errorSyntax(USAGE_DHCPSERVER, "unhandled parameter: %s", ValueUnion.psz);
            break;

            case 'o': // --options
                {
        // {"--vm",                'n', RTGETOPT_REQ_STRING}, /* only with -o */
        // {"--slot",              's', RTGETOPT_REQ_UINT8}, /* only with -o and -n*/
        // {"--id",                'i', RTGETOPT_REQ_UINT8}, /* only with -o */
        // {"--value",             'p', RTGETOPT_REQ_STRING} /* only with -i */
                    if (fOptionsRead)
                        return errorSyntax(USAGE_DHCPSERVER,
                                           "previos option edition  wasn't finished");
                    fOptionsRead = true;
                    fVmOptionRead = false; /* we want specify new global or vm option*/
                    u8Slot = (uint8_t)~0;
                    u8OptId = (uint8_t)~0;
                    pszVmName = NULL;
                } /* end of --options  */
                break;

            case 'n': // --vm-name
                {
                    if (fVmOptionRead)
                        return errorSyntax(USAGE_DHCPSERVER,
                                           "previous vm option edition wasn't finished");
                    else
                        fVmOptionRead = true;
                    u8Slot = (uint8_t)~0; /* clear slor */
                    pszVmName = RTStrDup(ValueUnion.psz);
                }
                break; /* end of --vm-name */

            case 's': // --slot
                {
                    if (!fVmOptionRead)
                        return errorSyntax(USAGE_DHCPSERVER,
                                           "vm name wasn't specified");

                    u8Slot = ValueUnion.u8;
                }
                break; /* end of --slot */

            case 'i': // --id
                {
                    if (!fOptionsRead)
                        return errorSyntax(USAGE_DHCPSERVER,
                                           "-o wasn't found");

                    u8OptId = ValueUnion.u8;
                }
                break; /* end of --id */

            case 'p': // --value
                {
                    if (!fOptionsRead)
                        return errorSyntax(USAGE_DHCPSERVER,
                                           "-o wasn't found");

                    if (u8OptId == (uint8_t)~0)
                        return errorSyntax(USAGE_DHCPSERVER,
                                           "--id wasn't found");
                    if (   fVmOptionRead
                        && u8Slot == (uint8_t)~0)
                        return errorSyntax(USAGE_DHCPSERVER,
                                           "--slot wasn't found");

                    DhcpOpts &opts = fVmOptionRead ? VmSlot2Options[VmNameSlotKey(pszVmName, u8Slot)]
                                                    : GlobalDhcpOptions;
                    std::string strVal = ValueUnion.psz;
                    opts.push_back(DhcpOptSpec((DhcpOpt_T)u8OptId, strVal));

                }
                break; // --end of value

            default:
                if (c > 0)
                {
                    if (RT_C_IS_GRAPH(c))
                        return errorSyntax(USAGE_DHCPSERVER, "unhandled option: -%c", c);
                    else
                        return errorSyntax(USAGE_DHCPSERVER, "unhandled option: %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_DHCPSERVER, "unknown option: %s", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_DHCPSERVER, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_DHCPSERVER, "%Rrs", c);
        }
    }

    if(! pNetName && !pIfName)
        return errorSyntax(USAGE_DHCPSERVER, "You need to specify either --netname or --ifname to identify the DHCP server");

    if(   enmCode != OP_REMOVE
       && GlobalDhcpOptions.size() == 0
       && VmSlot2Options.size() == 0)
    {
        if(enable < 0 || pIp || pNetmask || pLowerIp || pUpperIp)
        {
            if(!pIp)
                return errorSyntax(USAGE_DHCPSERVER, "You need to specify --ip option");

            if(!pNetmask)
                return errorSyntax(USAGE_DHCPSERVER, "You need to specify --netmask option");

            if(!pLowerIp)
                return errorSyntax(USAGE_DHCPSERVER, "You need to specify --lowerip option");

            if(!pUpperIp)
                return errorSyntax(USAGE_DHCPSERVER, "You need to specify --upperip option");
        }
    }

    Bstr NetName;
    if(!pNetName)
    {
        ComPtr<IHost> host;
        CHECK_ERROR(a->virtualBox, COMGETTER(Host)(host.asOutParam()));

        ComPtr<IHostNetworkInterface> hif;
        CHECK_ERROR(host, FindHostNetworkInterfaceByName(Bstr(pIfName).mutableRaw(), hif.asOutParam()));
        if (FAILED(rc))
            return errorArgument("Could not find interface '%s'", pIfName);

        CHECK_ERROR(hif, COMGETTER(NetworkName) (NetName.asOutParam()));
        if (FAILED(rc))
            return errorArgument("Could not get network name for the interface '%s'", pIfName);
    }
    else
    {
        NetName = Bstr(pNetName);
    }

    ComPtr<IDHCPServer> svr;
    rc = a->virtualBox->FindDHCPServerByNetworkName(NetName.mutableRaw(), svr.asOutParam());
    if(enmCode == OP_ADD)
    {
        if (SUCCEEDED(rc))
            return errorArgument("DHCP server already exists");

        CHECK_ERROR(a->virtualBox, CreateDHCPServer(NetName.mutableRaw(), svr.asOutParam()));
        if (FAILED(rc))
            return errorArgument("Failed to create the DHCP server");
    }
    else if (FAILED(rc))
    {
        return errorArgument("DHCP server does not exist");
    }

    if(enmCode != OP_REMOVE)
    {
        if (pIp || pNetmask || pLowerIp || pUpperIp)
        {
            CHECK_ERROR(svr, SetConfiguration (
                          Bstr(pIp).mutableRaw(),
                          Bstr(pNetmask).mutableRaw(),
                          Bstr(pLowerIp).mutableRaw(),
                          Bstr(pUpperIp).mutableRaw()));
            if(FAILED(rc))
                return errorArgument("Failed to set configuration");
        }

        if(enable >= 0)
        {
            CHECK_ERROR(svr, COMSETTER(Enabled) ((BOOL)enable));
        }

        /* option processing */
        DhcpOptIterator itOpt;
        VmSlot2OptionsIterator it;

        /* Global Options */
        for(itOpt = GlobalDhcpOptions.begin();
            itOpt != GlobalDhcpOptions.end();
            ++itOpt)
        {
            CHECK_ERROR(svr,
                        AddGlobalOption(
                          itOpt->first,
                          com::Bstr(itOpt->second.c_str()).raw()));
        }

        /* heh, vm slot options. */

        for (it = VmSlot2Options.begin();
             it != VmSlot2Options.end();
             ++it)
        {
            for(itOpt = it->second.begin();
                itOpt != it->second.end();
                ++itOpt)
            {
                CHECK_ERROR(svr,
                            AddVmSlotOption(Bstr(it->first.VmName.c_str()).raw(),
                                            it->first.u8Slot,
                                            itOpt->first,
                                            com::Bstr(itOpt->second.c_str()).raw()));
            }
        }
    }
    else
    {
        CHECK_ERROR(a->virtualBox, RemoveDHCPServer(svr));
        if(FAILED(rc))
            return errorArgument("Failed to remove server");
    }

    return RTEXITCODE_SUCCESS;
}


RTEXITCODE handleDHCPServer(HandlerArg *a)
{
    if (a->argc < 1)
        return errorSyntax(USAGE_DHCPSERVER, "Not enough parameters");

    RTEXITCODE rcExit;
    if (strcmp(a->argv[0], "modify") == 0)
        rcExit = handleOp(a, OP_MODIFY, 1);
    else if (strcmp(a->argv[0], "add") == 0)
        rcExit = handleOp(a, OP_ADD, 1);
    else if (strcmp(a->argv[0], "remove") == 0)
        rcExit = handleOp(a, OP_REMOVE, 1);
    else
        rcExit = errorSyntax(USAGE_DHCPSERVER, "Invalid parameter '%s'", Utf8Str(a->argv[0]).c_str());

    return rcExit;
}

#endif /* !VBOX_ONLY_DOCS */

