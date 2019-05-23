/* $Id: VBoxNetDHCP.cpp $ */
/** @file
 * VBoxNetDHCP - DHCP Service for connecting to IntNet.
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

/** @page pg_net_dhcp       VBoxNetDHCP
 *
 * Write a few words...
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/com/com.h>
#include <VBox/com/listeners.h>
#include <VBox/com/string.h>
#include <VBox/com/Guid.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/EventQueue.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/alloca.h>
#include <iprt/buildconfig.h>
#include <iprt/err.h>
#include <iprt/net.h>                   /* must come before getopt */
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/message.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/time.h>
#include <iprt/string.h>
#ifdef RT_OS_WINDOWS
# include <iprt/thread.h>
#endif

#include <VBox/sup.h>
#include <VBox/intnet.h>
#include <VBox/intnetinline.h>
#include <VBox/vmm/vmm.h>
#include <VBox/version.h>

#include "../NetLib/VBoxNetLib.h"
#include "../NetLib/shared_ptr.h"

#include <vector>
#include <list>
#include <string>
#include <map>

#include "../NetLib/VBoxNetBaseService.h"
#include "../NetLib/utils.h"

#ifdef RT_OS_WINDOWS /* WinMain */
# include <iprt/win/windows.h>
# include <stdlib.h>
# ifdef INET_ADDRSTRLEN
/* On Windows INET_ADDRSTRLEN defined as 22 Ws2ipdef.h, because it include port number */
#  undef INET_ADDRSTRLEN
# endif
# define INET_ADDRSTRLEN 16
#else
# include <netinet/in.h>
#endif


#include "Config.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * DHCP server instance.
 */
class VBoxNetDhcp : public VBoxNetBaseService, public NATNetworkEventAdapter
{
public:
    VBoxNetDhcp();
    virtual ~VBoxNetDhcp();

    int                 init();
    void                done();
    void                usage(void) { /* XXX: document options */ };
    int                 parseOpt(int rc, const RTGETOPTUNION& getOptVal);
    int                 processFrame(void *, size_t) {return VERR_IGNORED; };
    int                 processGSO(PCPDMNETWORKGSO, size_t) {return VERR_IGNORED; };
    int                 processUDP(void *, size_t);

protected:
    bool                handleDhcpMsg(uint8_t uMsgType, PCRTNETBOOTP pDhcpMsg, size_t cb);

    void                debugPrintV(int32_t iMinLevel, bool fMsg,  const char *pszFmt, va_list va) const;
    static const char  *debugDhcpName(uint8_t uMsgType);

private:
    int initNoMain();
    int initWithMain();
    HRESULT HandleEvent(VBoxEventType_T aEventType, IEvent *pEvent);

    static int hostDnsServers(const ComHostPtr& host,
                              const RTNETADDRIPV4& networkid,
                              const AddressToOffsetMapping& mapping,
                              AddressList& servers);
    int fetchAndUpdateDnsInfo();

protected:
    /** @name The DHCP server specific configuration data members.
     * @{ */
    /*
     * XXX: what was the plan? SQL3 or plain text file?
     * How it will coexists with managment from VBoxManagement, who should manage db
     * in that case (VBoxManage, VBoxSVC ???)
     */
    std::string         m_LeaseDBName;

    /** @} */

    /* corresponding dhcp server description in Main */
    ComPtr<IDHCPServer> m_DhcpServer;

    ComPtr<INATNetwork> m_NATNetwork;

    /** Listener for Host DNS changes */
    ComNatListenerPtr m_VBoxListener;
    ComNatListenerPtr m_VBoxClientListener;

    NetworkManager *m_NetworkManager;

    /*
     * We will ignore cmd line parameters IFF there will be some DHCP specific arguments
     * otherwise all paramters will come from Main.
     */
    bool m_fIgnoreCmdLineParameters;

    /*
     * -b -n 10.0.1.2 -m 255.255.255.0 -> to the list processing in
     */
    typedef struct
    {
        char Key;
        std::string strValue;
    } CMDLNPRM;
    std::list<CMDLNPRM> CmdParameterll;
    typedef std::list<CMDLNPRM>::iterator CmdParameterIterator;

    /** @name Debug stuff
     * @{  */
    int32_t             m_cVerbosity;
    uint8_t             m_uCurMsgType;
    size_t              m_cbCurMsg;
    PCRTNETBOOTP        m_pCurMsg;
    VBOXNETUDPHDRS      m_CurHdrs;
    /** @} */
};


static inline int configGetBoundryAddress(const ComDhcpServerPtr& dhcp, bool fUpperBoundry, RTNETADDRIPV4& boundryAddress)
{
    boundryAddress.u = INADDR_ANY;

    HRESULT hrc;
    com::Bstr strAddress;
    if (fUpperBoundry)
        hrc = dhcp->COMGETTER(UpperIP)(strAddress.asOutParam());
    else
        hrc = dhcp->COMGETTER(LowerIP)(strAddress.asOutParam());
    AssertComRCReturn(hrc, VERR_INTERNAL_ERROR);

    return RTNetStrToIPv4Addr(com::Utf8Str(strAddress).c_str(), &boundryAddress);
}


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Pointer to the DHCP server. */
static VBoxNetDhcp *g_pDhcp;

/* DHCP server specific options */
static RTGETOPTDEF g_aOptionDefs[] =
{
  { "--lease-db",       'D',   RTGETOPT_REQ_STRING },
  { "--begin-config",   'b',   RTGETOPT_REQ_NOTHING },
  { "--gateway",        'g',   RTGETOPT_REQ_IPV4ADDR },
  { "--lower-ip",       'l',   RTGETOPT_REQ_IPV4ADDR },
  { "--upper-ip",       'u',   RTGETOPT_REQ_IPV4ADDR },
};

/**
 * Construct a DHCP server with a default configuration.
 */
VBoxNetDhcp::VBoxNetDhcp()
  : VBoxNetBaseService("VBoxNetDhcp", "VBoxNetDhcp"),
    m_NetworkManager(NULL)
{
    /*   m_enmTrunkType          = kIntNetTrunkType_WhateverNone; */
    RTMAC mac;
    mac.au8[0]     = 0x08;
    mac.au8[1]     = 0x00;
    mac.au8[2]     = 0x27;
    mac.au8[3]     = 0x40;
    mac.au8[4]     = 0x41;
    mac.au8[5]     = 0x42;
    setMacAddress(mac);

    RTNETADDRIPV4 address;
    address.u = RT_H2N_U32_C(RT_BSWAP_U32_C(RT_MAKE_U32_FROM_U8( 10,  0,  2,  5)));
    setIpv4Address(address);

    setSendBufSize(8 * _1K);
    setRecvBufSize(50 * _1K);

    m_uCurMsgType           = UINT8_MAX;
    m_cbCurMsg              = 0;
    m_pCurMsg               = NULL;
    RT_ZERO(m_CurHdrs);

    m_fIgnoreCmdLineParameters = true;

    for(unsigned int i = 0; i < RT_ELEMENTS(g_aOptionDefs); ++i)
        addCommandLineOption(&g_aOptionDefs[i]);
}


/**
 * Destruct a DHCP server.
 */
VBoxNetDhcp::~VBoxNetDhcp()
{
}




/**
 * Parse the DHCP specific arguments.
 *
 * This callback caled for each paramenter so
 * ....
 * we nee post analisys of the parameters, at least
 *    for -b, -g, -l, -u, -m
 */
int VBoxNetDhcp::parseOpt(int rc, const RTGETOPTUNION& Val)
{
    CMDLNPRM prm;

    /* Ok, we've entered here, thus we can't ignore cmd line parameters anymore */
    m_fIgnoreCmdLineParameters = false;

    prm.Key = rc;

    switch (rc)
    {
        case 'l':
        case 'u':
        case 'g':
        {
            char buf[17];
            RTStrPrintf(buf, 17, "%RTnaipv4", Val.IPv4Addr.u);
            prm.strValue = buf;
            CmdParameterll.push_back(prm);
        }
        break;

        case 'b': // ignore
        case 'D': // ignore
            break;

        default:
            rc = RTGetOptPrintError(rc, &Val);
            RTPrintf("Use --help for more information.\n");
            return rc;
    }

    return VINF_SUCCESS;
}

int VBoxNetDhcp::init()
{
    int rc = this->VBoxNetBaseService::init();
    AssertRCReturn(rc, rc);

    if (isMainNeeded())
        rc = initWithMain();
    else
        rc = initNoMain();
    AssertRCReturn(rc, rc);

    m_NetworkManager = NetworkManager::getNetworkManager(m_DhcpServer);
    AssertPtrReturn(m_NetworkManager, VERR_INTERNAL_ERROR);

    m_NetworkManager->setOurAddress(getIpv4Address());
    m_NetworkManager->setOurNetmask(getIpv4Netmask());
    m_NetworkManager->setOurMac(getMacAddress());
    m_NetworkManager->setService(this);

    return VINF_SUCCESS;
}

void VBoxNetDhcp::done()
{
    destroyNatListener(m_VBoxListener, virtualbox);
    destroyClientListener(m_VBoxClientListener, virtualboxClient);
}

int  VBoxNetDhcp::processUDP(void *pv, size_t cbPv)
{
    PCRTNETBOOTP pDhcpMsg = (PCRTNETBOOTP)pv;
    m_pCurMsg  = pDhcpMsg;
    m_cbCurMsg = cbPv;

    uint8_t uMsgType;
    if (RTNetIPv4IsDHCPValid(NULL /* why is this here? */, pDhcpMsg, cbPv, &uMsgType))
    {
        m_uCurMsgType = uMsgType;
        {
            /* To avoid fight with event processing thread */
            VBoxNetALock(this);
            handleDhcpMsg(uMsgType, pDhcpMsg, cbPv);
        }
        m_uCurMsgType = UINT8_MAX;
    }
    else
        debugPrint(1, true, "VBoxNetDHCP: Skipping invalid DHCP packet.\n"); /** @todo handle pure bootp clients too? */

    m_pCurMsg = NULL;
    m_cbCurMsg = 0;

    return VINF_SUCCESS;
}


/**
 * Handles a DHCP message.
 *
 * @returns true if handled, false if not.  (IGNORED BY CALLER)
 * @param   uMsgType        The message type.
 * @param   pDhcpMsg        The DHCP message.
 * @param   cb              The size of the DHCP message.
 */
bool VBoxNetDhcp::handleDhcpMsg(uint8_t uMsgType, PCRTNETBOOTP pDhcpMsg, size_t cb)
{
    if (pDhcpMsg->bp_op == RTNETBOOTP_OP_REQUEST)
    {
        AssertPtrReturn(m_NetworkManager, false);

        switch (uMsgType)
        {
            case RTNET_DHCP_MT_DISCOVER:
                return m_NetworkManager->handleDhcpReqDiscover(pDhcpMsg, cb);

            case RTNET_DHCP_MT_REQUEST:
                return m_NetworkManager->handleDhcpReqRequest(pDhcpMsg, cb);

            case RTNET_DHCP_MT_DECLINE:
                return m_NetworkManager->handleDhcpReqDecline(pDhcpMsg, cb);

            case RTNET_DHCP_MT_RELEASE:
                return m_NetworkManager->handleDhcpReqRelease(pDhcpMsg, cb);

            case RTNET_DHCP_MT_INFORM:
                debugPrint(0, true, "Should we handle this?");
                break;

            default:
                debugPrint(0, true, "Unexpected.");
                break;
        }
    }
    return false;
}

/**
 * Print debug message depending on the m_cVerbosity level.
 *
 * @param   iMinLevel       The minimum m_cVerbosity level for this message.
 * @param   fMsg            Whether to dump parts for the current DHCP message.
 * @param   pszFmt          The message format string.
 * @param   va              Optional arguments.
 */
void VBoxNetDhcp::debugPrintV(int iMinLevel, bool fMsg, const char *pszFmt, va_list va) const
{
    if (iMinLevel <= m_cVerbosity)
    {
        va_list vaCopy;                 /* This dude is *very* special, thus the copy. */
        va_copy(vaCopy, va);
        RTStrmPrintf(g_pStdErr, "VBoxNetDHCP: %s: %N\n", iMinLevel >= 2 ? "debug" : "info", pszFmt, &vaCopy);
        va_end(vaCopy);

        if (    fMsg
            &&  m_cVerbosity >= 2
            &&  m_pCurMsg)
        {
            /* XXX: export this to debugPrinfDhcpMsg or variant and other method export
             *  to base class
             */
            const char *pszMsg = m_uCurMsgType != UINT8_MAX ? debugDhcpName(m_uCurMsgType) : "";
            RTStrmPrintf(g_pStdErr, "VBoxNetDHCP: debug: %8s chaddr=%.6Rhxs ciaddr=%d.%d.%d.%d yiaddr=%d.%d.%d.%d siaddr=%d.%d.%d.%d xid=%#x\n",
                         pszMsg,
                         &m_pCurMsg->bp_chaddr,
                         m_pCurMsg->bp_ciaddr.au8[0], m_pCurMsg->bp_ciaddr.au8[1], m_pCurMsg->bp_ciaddr.au8[2], m_pCurMsg->bp_ciaddr.au8[3],
                         m_pCurMsg->bp_yiaddr.au8[0], m_pCurMsg->bp_yiaddr.au8[1], m_pCurMsg->bp_yiaddr.au8[2], m_pCurMsg->bp_yiaddr.au8[3],
                         m_pCurMsg->bp_siaddr.au8[0], m_pCurMsg->bp_siaddr.au8[1], m_pCurMsg->bp_siaddr.au8[2], m_pCurMsg->bp_siaddr.au8[3],
                         m_pCurMsg->bp_xid);
        }
    }
}


/**
 * Gets the name of given DHCP message type.
 *
 * @returns Readonly name.
 * @param   uMsgType        The message number.
 */
/* static */ const char *VBoxNetDhcp::debugDhcpName(uint8_t uMsgType)
{
    switch (uMsgType)
    {
        case 0:                         return "MT_00";
        case RTNET_DHCP_MT_DISCOVER:    return "DISCOVER";
        case RTNET_DHCP_MT_OFFER:       return "OFFER";
        case RTNET_DHCP_MT_REQUEST:     return "REQUEST";
        case RTNET_DHCP_MT_DECLINE:     return "DECLINE";
        case RTNET_DHCP_MT_ACK:         return "ACK";
        case RTNET_DHCP_MT_NAC:         return "NAC";
        case RTNET_DHCP_MT_RELEASE:     return "RELEASE";
        case RTNET_DHCP_MT_INFORM:      return "INFORM";
        case 9:                         return "MT_09";
        case 10:                        return "MT_0a";
        case 11:                        return "MT_0b";
        case 12:                        return "MT_0c";
        case 13:                        return "MT_0d";
        case 14:                        return "MT_0e";
        case 15:                        return "MT_0f";
        case 16:                        return "MT_10";
        case 17:                        return "MT_11";
        case 18:                        return "MT_12";
        case 19:                        return "MT_13";
        case UINT8_MAX:                 return "MT_ff";
        default:                        return "UNKNOWN";
    }
}


int VBoxNetDhcp::initNoMain()
{
    CmdParameterIterator it;

    RTNETADDRIPV4 address = getIpv4Address();
    RTNETADDRIPV4 netmask = getIpv4Netmask();
    RTNETADDRIPV4 networkId;
    networkId.u = address.u & netmask.u;

    RTNETADDRIPV4 UpperAddress;
    RTNETADDRIPV4 LowerAddress = networkId;
    UpperAddress.u = RT_H2N_U32(RT_N2H_U32(LowerAddress.u) | RT_N2H_U32(netmask.u));

    for (it = CmdParameterll.begin(); it != CmdParameterll.end(); ++it)
    {
        switch(it->Key)
        {
            case 'l':
                RTNetStrToIPv4Addr(it->strValue.c_str(), &LowerAddress);
                break;

            case 'u':
                RTNetStrToIPv4Addr(it->strValue.c_str(), &UpperAddress);
                break;
            case 'b':
                break;

        }
    }

    ConfigurationManager *confManager = ConfigurationManager::getConfigurationManager();
    AssertPtrReturn(confManager, VERR_INTERNAL_ERROR);
    confManager->addNetwork(unconst(g_RootConfig),
                            networkId,
                            netmask,
                            LowerAddress,
                            UpperAddress);

    return VINF_SUCCESS;
}


int VBoxNetDhcp::initWithMain()
{
    /* ok, here we should initiate instance of dhcp server
     * and listener for Dhcp configuration events
     */
    AssertRCReturn(virtualbox.isNull(), VERR_INTERNAL_ERROR);
    std::string networkName = getNetworkName();

    int rc = findDhcpServer(virtualbox, networkName, m_DhcpServer);
    AssertRCReturn(rc, rc);

    rc = findNatNetwork(virtualbox, networkName, m_NATNetwork);
    AssertRCReturn(rc, rc);

    BOOL fNeedDhcpServer = isDhcpRequired(m_NATNetwork);
    if (!fNeedDhcpServer)
        return VERR_CANCELLED;

    RTNETADDRIPV4 gateway;
    com::Bstr strGateway;
    HRESULT hrc = m_NATNetwork->COMGETTER(Gateway)(strGateway.asOutParam());
    AssertComRCReturn(hrc, VERR_INTERNAL_ERROR);
    RTNetStrToIPv4Addr(com::Utf8Str(strGateway).c_str(), &gateway);

    ConfigurationManager *confManager = ConfigurationManager::getConfigurationManager();
    AssertPtrReturn(confManager, VERR_INTERNAL_ERROR);
    confManager->addToAddressList(RTNET_DHCP_OPT_ROUTERS, gateway);

    rc = fetchAndUpdateDnsInfo();
    AssertMsgRCReturn(rc, ("Wasn't able to fetch Dns info"), rc);

    {
        ComEventTypeArray eventTypes;
        eventTypes.push_back(VBoxEventType_OnHostNameResolutionConfigurationChange);
        eventTypes.push_back(VBoxEventType_OnNATNetworkStartStop);
        rc = createNatListener(m_VBoxListener, virtualbox, this, eventTypes);
        AssertRCReturn(rc, rc);
    }

    {
        ComEventTypeArray eventTypes;
        eventTypes.push_back(VBoxEventType_OnVBoxSVCAvailabilityChanged);
        rc = createClientListener(m_VBoxClientListener, virtualboxClient, this, eventTypes);
        AssertRCReturn(rc, rc);
    }

    RTNETADDRIPV4 LowerAddress;
    rc = configGetBoundryAddress(m_DhcpServer, false, LowerAddress);
    AssertMsgRCReturn(rc, ("can't get lower boundrary adderss'"),rc);

    RTNETADDRIPV4 UpperAddress;
    rc = configGetBoundryAddress(m_DhcpServer, true, UpperAddress);
    AssertMsgRCReturn(rc, ("can't get upper boundrary adderss'"),rc);

    RTNETADDRIPV4 address = getIpv4Address();
    RTNETADDRIPV4 netmask = getIpv4Netmask();
    RTNETADDRIPV4 networkId = networkid(address, netmask);
    std::string name = std::string("default");

    confManager->addNetwork(unconst(g_RootConfig),
                            networkId,
                            netmask,
                            LowerAddress,
                            UpperAddress);

    com::Bstr bstr;
    hrc = virtualbox->COMGETTER(HomeFolder)(bstr.asOutParam());
    com::Utf8StrFmt strXmlLeaseFile("%ls%c%s.leases",
                                    bstr.raw(), RTPATH_DELIMITER, networkName.c_str());
    confManager->loadFromFile(strXmlLeaseFile);

    return VINF_SUCCESS;
}


int VBoxNetDhcp::fetchAndUpdateDnsInfo()
{
    ComHostPtr host;
    if (SUCCEEDED(virtualbox->COMGETTER(Host)(host.asOutParam())))
    {
        AddressToOffsetMapping mapIp4Addr2Off;
        int rc = localMappings(m_NATNetwork, mapIp4Addr2Off);
        /* XXX: here could be several cases: 1. COM error, 2. not found (empty) 3. ? */
        AssertMsgRCReturn(rc, ("Can't fetch local mappings"), rc);

        RTNETADDRIPV4 address = getIpv4Address();
        RTNETADDRIPV4 netmask = getIpv4Netmask();

        AddressList nameservers;
        rc = hostDnsServers(host, networkid(address, netmask), mapIp4Addr2Off, nameservers);
        AssertMsgRCReturn(rc, ("Debug me!!!"), rc);
        /* XXX: Search strings */

        std::string domain;
        rc = hostDnsDomain(host, domain);
        AssertMsgRCReturn(rc, ("Debug me!!"), rc);

        {
            VBoxNetALock(this);
            ConfigurationManager *confManager = ConfigurationManager::getConfigurationManager();
            confManager->flushAddressList(RTNET_DHCP_OPT_DNS);

            for (AddressList::iterator it = nameservers.begin(); it != nameservers.end(); ++it)
                confManager->addToAddressList(RTNET_DHCP_OPT_DNS, *it);

            confManager->setString(RTNET_DHCP_OPT_DOMAIN_NAME, domain);
        }
    }

    return VINF_SUCCESS;
}


int VBoxNetDhcp::hostDnsServers(const ComHostPtr& host,
                                const RTNETADDRIPV4& networkid,
                                const AddressToOffsetMapping& mapping,
                                AddressList& servers)
{
    ComBstrArray strs;

    HRESULT hrc = host->COMGETTER(NameServers)(ComSafeArrayAsOutParam(strs));
    if (FAILED(hrc))
        return VERR_NOT_FOUND;

    /*
     * Recent fashion is to run dnsmasq on 127.0.1.1 which we
     * currently can't map.  If that's the only nameserver we've got,
     * we need to use DNS proxy for VMs to reach it.
     */
    bool fUnmappedLoopback = false;

    for (size_t i = 0; i < strs.size(); ++i)
    {
        RTNETADDRIPV4 addr;
        int rc;

        rc = RTNetStrToIPv4Addr(com::Utf8Str(strs[i]).c_str(), &addr);
        if (RT_FAILURE(rc))
            continue;

        if (addr.u == INADDR_ANY)
        {
            /*
             * This doesn't seem to be very well documented except for
             * RTFS of res_init.c, but INADDR_ANY is a valid value for
             * for "nameserver".
             */
            addr.u = RT_H2N_U32_C(INADDR_LOOPBACK);
        }

        if (addr.au8[0] == 127)
        {
            AddressToOffsetMapping::const_iterator remap(mapping.find(addr));

            if (remap != mapping.end())
            {
                int offset = remap->second;
                addr.u = RT_H2N_U32(RT_N2H_U32(networkid.u) + offset);
            }
            else
            {
                fUnmappedLoopback = true;
                continue;
            }
        }

        servers.push_back(addr);
    }

    if (servers.empty() && fUnmappedLoopback)
    {
        RTNETADDRIPV4 proxy;

        proxy.u = networkid.u | RT_H2N_U32_C(1U);
        servers.push_back(proxy);
    }

    return VINF_SUCCESS;
}


HRESULT VBoxNetDhcp::HandleEvent(VBoxEventType_T aEventType, IEvent *pEvent)
{
    switch (aEventType)
    {
        case VBoxEventType_OnHostNameResolutionConfigurationChange:
            fetchAndUpdateDnsInfo();
            break;

        case VBoxEventType_OnNATNetworkStartStop:
        {
            ComPtr <INATNetworkStartStopEvent> pStartStopEvent = pEvent;

            com::Bstr networkName;
            HRESULT hrc = pStartStopEvent->COMGETTER(NetworkName)(networkName.asOutParam());
            AssertComRCReturn(hrc, hrc);
            if (networkName.compare(getNetworkName().c_str()))
                break; /* change not for our network */

            BOOL fStart = TRUE;
            hrc = pStartStopEvent->COMGETTER(StartEvent)(&fStart);
            AssertComRCReturn(hrc, hrc);
            if (!fStart)
                shutdown();
            break;
        }

        case VBoxEventType_OnVBoxSVCAvailabilityChanged:
        {
            shutdown();
            break;
        }

        default: break; /* Shut up MSC. */
    }

    return S_OK;
}

#ifdef RT_OS_WINDOWS

/** The class name for the DIFx-killable window.    */
static WCHAR g_wszWndClassName[] = L"VBoxNetDHCPClass";
/** Whether to exit the process on quit.   */
static bool g_fExitProcessOnQuit = true;

/**
 * Window procedure for making us DIFx-killable.
 */
static LRESULT CALLBACK DIFxKillableWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/** @callback_method_impl{FNRTTHREAD,
 *      Thread that creates service a window the DIFx can destroy, thereby
 *      triggering process termination. }
 */
static DECLCALLBACK(int) DIFxKillableProcessThreadProc(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF(hThreadSelf, pvUser);
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    /* Register the Window Class. */
    WNDCLASSW WndCls;
    WndCls.style         = 0;
    WndCls.lpfnWndProc   = DIFxKillableWindowProc;
    WndCls.cbClsExtra    = 0;
    WndCls.cbWndExtra    = sizeof(void *);
    WndCls.hInstance     = hInstance;
    WndCls.hIcon         = NULL;
    WndCls.hCursor       = NULL;
    WndCls.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    WndCls.lpszMenuName  = NULL;
    WndCls.lpszClassName = g_wszWndClassName;

    ATOM atomWindowClass = RegisterClassW(&WndCls);
    if (atomWindowClass != 0)
    {
        /* Create the window. */
        HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
                                    g_wszWndClassName, g_wszWndClassName,
                                    WS_POPUPWINDOW,
                                    -200, -200, 100, 100, NULL, NULL, hInstance, NULL);
        if (hwnd)
        {
            SetWindowPos(hwnd, HWND_TOPMOST, -200, -200, 0, 0,
                         SWP_NOACTIVATE | SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOSIZE);

            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            DestroyWindow(hwnd);
        }

        UnregisterClassW(g_wszWndClassName, hInstance);

        if (hwnd && g_fExitProcessOnQuit)
            exit(0);
    }
    return 0;
}

#endif /* RT_OS_WINDOWS */

/**
 *  Entry point.
 */
extern "C" DECLEXPORT(int) TrustedMain(int argc, char **argv)
{
    /*
     * Instantiate the DHCP server and hand it the options.
     */
    VBoxNetDhcp *pDhcp = new VBoxNetDhcp();
    if (!pDhcp)
    {
        RTStrmPrintf(g_pStdErr, "VBoxNetDHCP: new VBoxNetDhcp failed!\n");
        return 1;
    }

    RTEXITCODE rcExit = (RTEXITCODE)pDhcp->parseArgs(argc - 1, argv + 1);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;

#ifdef RT_OS_WINDOWS
    /* DIFx hack. */
    RTTHREAD hMakeUseKillableThread = NIL_RTTHREAD;
    int rc2 = RTThreadCreate(&hMakeUseKillableThread, DIFxKillableProcessThreadProc, NULL, 0,
                             RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "DIFxKill");
    if (RT_FAILURE(rc2))
        hMakeUseKillableThread = NIL_RTTHREAD;
#endif

    pDhcp->init();

    /*
     * Try connect the server to the network.
     */
    int rc = pDhcp->tryGoOnline();
    if (RT_SUCCESS(rc))
    {
        /*
         * Process requests.
         */
        g_pDhcp = pDhcp;
        rc = pDhcp->run();
        pDhcp->done();

        g_pDhcp = NULL;
    }
    delete pDhcp;

#ifdef RT_OS_WINDOWS
    /* Kill DIFx hack. */
    if (hMakeUseKillableThread != NIL_RTTHREAD)
    {
        g_fExitProcessOnQuit = false;
        PostThreadMessage((DWORD)RTThreadGetNative(hMakeUseKillableThread), WM_QUIT, 0, 0);
        RTThreadWait(hMakeUseKillableThread, RT_MS_1SEC * 5U, NULL);
    }
#endif

    return RT_SUCCESS(rc) ? RTEXITCODE_SUCCESS : RTEXITCODE_FAILURE;
}


#ifndef VBOX_WITH_HARDENING

int main(int argc, char **argv)
{
    int rc = RTR3InitExe(argc, &argv, RTR3INIT_FLAGS_SUPLIB);
    if (RT_FAILURE(rc))
        return RTMsgInitFailure(rc);

    return TrustedMain(argc, argv);
}

# ifdef RT_OS_WINDOWS



/** (We don't want a console usually.) */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    NOREF(hInstance); NOREF(hPrevInstance); NOREF(lpCmdLine); NOREF(nCmdShow);
    return main(__argc, __argv);
}
# endif /* RT_OS_WINDOWS */

#endif /* !VBOX_WITH_HARDENING */

