/* $Id: VBoxNetLwipNAT.cpp $ */
/** @file
 * VBoxNetNAT - NAT Service for connecting to IntNet.
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

/* Must be included before winutils.h (lwip/def.h), otherwise Windows build breaks. */
#define LOG_GROUP LOG_GROUP_NAT_SERVICE

#include "winutils.h"

#include <VBox/com/assert.h>
#include <VBox/com/com.h>
#include <VBox/com/listeners.h>
#include <VBox/com/string.h>
#include <VBox/com/Guid.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/net.h>
#include <iprt/initterm.h>
#include <iprt/alloca.h>
#ifndef RT_OS_WINDOWS
# include <arpa/inet.h>
#endif
#include <iprt/err.h>
#include <iprt/time.h>
#include <iprt/timer.h>
#include <iprt/thread.h>
#include <iprt/stream.h>
#include <iprt/path.h>
#include <iprt/param.h>
#include <iprt/pipe.h>
#include <iprt/getopt.h>
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/message.h>
#include <iprt/req.h>
#include <iprt/file.h>
#include <iprt/semaphore.h>
#include <iprt/cpp/utils.h>
#include <VBox/log.h>

#include <VBox/sup.h>
#include <VBox/intnet.h>
#include <VBox/intnetinline.h>
#include <VBox/vmm/pdmnetinline.h>
#include <VBox/vmm/vmm.h>
#include <VBox/version.h>

#ifndef RT_OS_WINDOWS
# include <sys/poll.h>
# include <sys/socket.h>
# include <netinet/in.h>
# ifdef RT_OS_LINUX
#  include <linux/icmp.h>       /* ICMP_FILTER */
# endif
# include <netinet/icmp6.h>
#endif

#include <map>
#include <vector>
#include <string>

#include <stdio.h>

#include "../NetLib/VBoxNetLib.h"
#include "../NetLib/VBoxNetBaseService.h"
#include "../NetLib/utils.h"
#include "VBoxLwipCore.h"

extern "C"
{
/* bunch of LWIP headers */
#include "lwip/sys.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/ethip6.h"
#include "lwip/nd6.h"           // for proxy_na_hook
#include "lwip/mld6.h"
#include "lwip/tcpip.h"
#include "netif/etharp.h"

#include "proxy.h"
#include "pxremap.h"
#include "portfwd.h"
}


#if defined(VBOX_RAWSOCK_DEBUG_HELPER)          \
    && (defined(VBOX_WITH_HARDENING)            \
        || defined(RT_OS_WINDOWS)               \
        || defined(RT_OS_DARWIN))
# error Have you forgotten to turn off VBOX_RAWSOCK_DEBUG_HELPER?
#endif

#ifdef VBOX_RAWSOCK_DEBUG_HELPER
extern "C" int getrawsock(int type);
#endif

#include "../NetLib/VBoxPortForwardString.h"

static RTGETOPTDEF g_aGetOptDef[] =
{
    { "--port-forward4",           'p',   RTGETOPT_REQ_STRING },
    { "--port-forward6",           'P',   RTGETOPT_REQ_STRING }
};

typedef struct NATSEVICEPORTFORWARDRULE
{
    PORTFORWARDRULE Pfr;
    fwspec         FWSpec;
} NATSEVICEPORTFORWARDRULE, *PNATSEVICEPORTFORWARDRULE;

typedef std::vector<NATSEVICEPORTFORWARDRULE> VECNATSERVICEPF;
typedef VECNATSERVICEPF::iterator ITERATORNATSERVICEPF;
typedef VECNATSERVICEPF::const_iterator CITERATORNATSERVICEPF;

static int fetchNatPortForwardRules(const ComNatPtr&, bool, VECNATSERVICEPF&);

static int vboxNetNATLogInit(int argc, char **argv);


class VBoxNetLwipNAT: public VBoxNetBaseService, public NATNetworkEventAdapter
{
    friend class NATNetworkListener;
  public:
    VBoxNetLwipNAT(SOCKET icmpsock4, SOCKET icmpsock6);
    virtual ~VBoxNetLwipNAT();
    void usage(){                /** @todo should be implemented */ };
    int run();
    virtual int init(void);
    virtual int parseOpt(int rc, const RTGETOPTUNION& getOptVal);
    /* VBoxNetNAT always needs Main */
    virtual bool isMainNeeded() const { return true; }
    virtual int processFrame(void *, size_t);
    virtual int processGSO(PCPDMNETWORKGSO, size_t);
    virtual int processUDP(void *, size_t) { return VERR_IGNORED; }

   private:
    struct proxy_options m_ProxyOptions;
    struct sockaddr_in m_src4;
    struct sockaddr_in6 m_src6;
    /**
     * place for registered local interfaces.
     */
    ip4_lomap m_lo2off[10];
    ip4_lomap_desc m_loOptDescriptor;

    uint16_t m_u16Mtu;
    netif m_LwipNetIf;

    /* Our NAT network descriptor in Main */
    ComPtr<INATNetwork> m_net;
    ComPtr<IHost> m_host;

    ComNatListenerPtr m_NatListener;
    ComNatListenerPtr m_VBoxListener;
    ComNatListenerPtr m_VBoxClientListener;
    static INTNETSEG aXmitSeg[64];

    HRESULT HandleEvent(VBoxEventType_T aEventType, IEvent *pEvent);

    const char **getHostNameservers();

    /* Only for debug needs, by default NAT service should load rules from SVC
     * on startup, and then on sync them on events.
     */
    bool fDontLoadRulesOnStartup;
    static DECLCALLBACK(void) onLwipTcpIpInit(void *arg);
    static DECLCALLBACK(void) onLwipTcpIpFini(void *arg);
    static err_t netifInit(netif *pNetif);
    static err_t netifLinkoutput(netif *pNetif, pbuf *pBuf);
    /* static int intNetThreadRecv(RTTHREAD, void *); - unused */

    VECNATSERVICEPF m_vecPortForwardRule4;
    VECNATSERVICEPF m_vecPortForwardRule6;

    static int natServicePfRegister(NATSEVICEPORTFORWARDRULE& natServicePf);
    static int natServiceProcessRegisteredPf(VECNATSERVICEPF& vecPf);
};


static VBoxNetLwipNAT *g_pLwipNat;
INTNETSEG VBoxNetLwipNAT::aXmitSeg[64];

/**
 * @note: this work on Event thread.
 */
HRESULT VBoxNetLwipNAT::HandleEvent(VBoxEventType_T aEventType, IEvent *pEvent)
{
    HRESULT hrc = S_OK;
    switch (aEventType)
    {
        case VBoxEventType_OnNATNetworkSetting:
        {
            ComPtr<INATNetworkSettingEvent> pSettingsEvent(pEvent);

            com::Bstr networkName;
            hrc = pSettingsEvent->COMGETTER(NetworkName)(networkName.asOutParam());
            AssertComRCReturn(hrc, hrc);
            if (networkName.compare(getNetworkName().c_str()))
                break; /* change not for our network */

            // XXX: only handle IPv6 default route for now
            if (!m_ProxyOptions.ipv6_enabled)
                break;

            BOOL fIPv6DefaultRoute = FALSE;
            hrc = pSettingsEvent->COMGETTER(AdvertiseDefaultIPv6RouteEnabled)(&fIPv6DefaultRoute);
            AssertComRCReturn(hrc, hrc);

            if (m_ProxyOptions.ipv6_defroute == fIPv6DefaultRoute)
                break;

            m_ProxyOptions.ipv6_defroute = fIPv6DefaultRoute;
            tcpip_callback_with_block(proxy_rtadvd_do_quick, &m_LwipNetIf, 0);
            break;
        }

        case VBoxEventType_OnNATNetworkPortForward:
        {
            ComPtr<INATNetworkPortForwardEvent> pForwardEvent = pEvent;

            com::Bstr networkName;
            hrc = pForwardEvent->COMGETTER(NetworkName)(networkName.asOutParam());
            AssertComRCReturn(hrc, hrc);
            if (networkName.compare(getNetworkName().c_str()))
                break; /* change not for our network */

            BOOL fCreateFW;
            hrc = pForwardEvent->COMGETTER(Create)(&fCreateFW);
            AssertComRCReturn(hrc, hrc);

            BOOL  fIPv6FW;
            hrc = pForwardEvent->COMGETTER(Ipv6)(&fIPv6FW);
            AssertComRCReturn(hrc, hrc);

            com::Bstr name;
            hrc = pForwardEvent->COMGETTER(Name)(name.asOutParam());
            AssertComRCReturn(hrc, hrc);

            NATProtocol_T proto = NATProtocol_TCP;
            hrc = pForwardEvent->COMGETTER(Proto)(&proto);
            AssertComRCReturn(hrc, hrc);

            com::Bstr strHostAddr;
            hrc = pForwardEvent->COMGETTER(HostIp)(strHostAddr.asOutParam());
            AssertComRCReturn(hrc, hrc);

            LONG lHostPort;
            hrc = pForwardEvent->COMGETTER(HostPort)(&lHostPort);
            AssertComRCReturn(hrc, hrc);

            com::Bstr strGuestAddr;
            hrc = pForwardEvent->COMGETTER(GuestIp)(strGuestAddr.asOutParam());
            AssertComRCReturn(hrc, hrc);

            LONG lGuestPort;
            hrc = pForwardEvent->COMGETTER(GuestPort)(&lGuestPort);
            AssertComRCReturn(hrc, hrc);

            VECNATSERVICEPF& rules = fIPv6FW ? m_vecPortForwardRule6
                                             : m_vecPortForwardRule4;

            NATSEVICEPORTFORWARDRULE r;
            RT_ZERO(r);

            r.Pfr.fPfrIPv6 = fIPv6FW;

            switch (proto)
            {
                case NATProtocol_TCP:
                    r.Pfr.iPfrProto = IPPROTO_TCP;
                    break;
                case NATProtocol_UDP:
                    r.Pfr.iPfrProto = IPPROTO_UDP;
                    break;

                default:
                    LogRel(("Event: %s %s port-forwarding rule \"%s\": invalid protocol %d\n",
                            fCreateFW ? "Add" : "Remove",
                            fIPv6FW ? "IPv6" : "IPv4",
                            com::Utf8Str(name).c_str(),
                            (int)proto));
                    goto port_forward_done;
            }

            LogRel(("Event: %s %s port-forwarding rule \"%s\": %s %s%s%s:%d -> %s%s%s:%d\n",
                    fCreateFW ? "Add" : "Remove",
                    fIPv6FW ? "IPv6" : "IPv4",
                    com::Utf8Str(name).c_str(),
                    proto == NATProtocol_TCP ? "TCP" : "UDP",
                    /* from */
                    fIPv6FW ? "[" : "",
                    com::Utf8Str(strHostAddr).c_str(),
                    fIPv6FW ? "]" : "",
                    lHostPort,
                    /* to */
                    fIPv6FW ? "[" : "",
                    com::Utf8Str(strGuestAddr).c_str(),
                    fIPv6FW ? "]" : "",
                    lGuestPort));

            if (name.length() > sizeof(r.Pfr.szPfrName))
            {
                hrc = E_INVALIDARG;
                goto port_forward_done;
            }

            RTStrPrintf(r.Pfr.szPfrName, sizeof(r.Pfr.szPfrName),
                        "%s", com::Utf8Str(name).c_str());

            RTStrPrintf(r.Pfr.szPfrHostAddr, sizeof(r.Pfr.szPfrHostAddr),
                        "%s", com::Utf8Str(strHostAddr).c_str());

            /* XXX: limits should be checked */
            r.Pfr.u16PfrHostPort = (uint16_t)lHostPort;

            RTStrPrintf(r.Pfr.szPfrGuestAddr, sizeof(r.Pfr.szPfrGuestAddr),
                        "%s", com::Utf8Str(strGuestAddr).c_str());

            /* XXX: limits should be checked */
            r.Pfr.u16PfrGuestPort = (uint16_t)lGuestPort;

            if (fCreateFW) /* Addition */
            {
                int rc = natServicePfRegister(r);
                if (RT_SUCCESS(rc))
                    rules.push_back(r);
            }
            else /* Deletion */
            {
                ITERATORNATSERVICEPF it;
                for (it = rules.begin(); it != rules.end(); ++it)
                {
                    /* compare */
                    NATSEVICEPORTFORWARDRULE &natFw = *it;
                    if (   natFw.Pfr.iPfrProto == r.Pfr.iPfrProto
                        && natFw.Pfr.u16PfrHostPort == r.Pfr.u16PfrHostPort
                        && strncmp(natFw.Pfr.szPfrHostAddr, r.Pfr.szPfrHostAddr, INET6_ADDRSTRLEN) == 0
                        && natFw.Pfr.u16PfrGuestPort == r.Pfr.u16PfrGuestPort
                        && strncmp(natFw.Pfr.szPfrGuestAddr, r.Pfr.szPfrGuestAddr, INET6_ADDRSTRLEN) == 0)
                    {
                        fwspec *pFwCopy = (fwspec *)RTMemDup(&natFw.FWSpec, sizeof(natFw.FWSpec));
                        if (pFwCopy)
                        {
                            int status = portfwd_rule_del(pFwCopy);
                            if (status == 0)
                                rules.erase(it);   /* (pFwCopy is owned by lwip thread now.) */
                            else
                                RTMemFree(pFwCopy);
                        }
                        break;
                    }
                } /* loop over vector elements */
            } /* condition add or delete */
        port_forward_done:
            /* clean up strings */
            name.setNull();
            strHostAddr.setNull();
            strGuestAddr.setNull();
            break;
        }

        case VBoxEventType_OnHostNameResolutionConfigurationChange:
        {
            const char **ppcszNameServers = getHostNameservers();
            err_t error;

            error = tcpip_callback_with_block(pxdns_set_nameservers,
                                              ppcszNameServers,
                                              /* :block */ 0);
            if (error != ERR_OK && ppcszNameServers != NULL)
                RTMemFree(ppcszNameServers);
            break;
        }

        case VBoxEventType_OnNATNetworkStartStop:
        {
            ComPtr <INATNetworkStartStopEvent> pStartStopEvent = pEvent;

            com::Bstr networkName;
            hrc = pStartStopEvent->COMGETTER(NetworkName)(networkName.asOutParam());
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
            LogRel(("VBoxSVC became unavailable, exiting.\n"));
            shutdown();
            break;
        }

        default: break; /* Shut up MSC. */
    }
    return hrc;
}


/*static*/ DECLCALLBACK(void) VBoxNetLwipNAT::onLwipTcpIpInit(void *arg)
{
    AssertPtrReturnVoid(arg);
    VBoxNetLwipNAT *pNat = static_cast<VBoxNetLwipNAT *>(arg);

    HRESULT hrc = com::Initialize();
    Assert(!FAILED(hrc)); NOREF(hrc);

    proxy_arp_hook = pxremap_proxy_arp;
    proxy_ip4_divert_hook = pxremap_ip4_divert;

    proxy_na_hook = pxremap_proxy_na;
    proxy_ip6_divert_hook = pxremap_ip6_divert;

    /* lwip thread */
    RTNETADDRIPV4 network;
    RTNETADDRIPV4 address = g_pLwipNat->getIpv4Address();
    RTNETADDRIPV4 netmask = g_pLwipNat->getIpv4Netmask();
    network.u = address.u & netmask.u;

    ip_addr LwipIpAddr, LwipIpNetMask, LwipIpNetwork;

    memcpy(&LwipIpAddr, &address, sizeof(ip_addr));
    memcpy(&LwipIpNetMask, &netmask, sizeof(ip_addr));
    memcpy(&LwipIpNetwork, &network, sizeof(ip_addr));

    netif *pNetif = netif_add(&g_pLwipNat->m_LwipNetIf /* Lwip Interface */,
                              &LwipIpAddr /* IP address*/,
                              &LwipIpNetMask /* Network mask */,
                              &LwipIpAddr /* gateway address, @todo: is self IP acceptable? */,
                              g_pLwipNat /* state */,
                              VBoxNetLwipNAT::netifInit /* netif_init_fn */,
                              tcpip_input /* netif_input_fn */);

    AssertPtrReturnVoid(pNetif);

    LogRel(("netif %c%c%d: mac %RTmac\n",
            pNetif->name[0], pNetif->name[1], pNetif->num,
            pNetif->hwaddr));
    LogRel(("netif %c%c%d: inet %RTnaipv4 netmask %RTnaipv4\n",
            pNetif->name[0], pNetif->name[1], pNetif->num,
            pNetif->ip_addr, pNetif->netmask));
    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
        if (!ip6_addr_isinvalid(netif_ip6_addr_state(pNetif, i))) {
            LogRel(("netif %c%c%d: inet6 %RTnaipv6\n",
                    pNetif->name[0], pNetif->name[1], pNetif->num,
                    netif_ip6_addr(pNetif, i)));
        }
    }

    netif_set_up(pNetif);
    netif_set_link_up(pNetif);

    if (pNat->m_ProxyOptions.ipv6_enabled) {
        /*
         * XXX: lwIP currently only ever calls mld6_joingroup() in
         * nd6_tmr() for fresh tentative addresses, which is a wrong place
         * to do it - but I'm not keen on fixing this properly for now
         * (with correct handling of interface up and down transitions,
         * etc).  So stick it here as a kludge.
         */
        for (int i = 0; i <= 1; ++i) {
            ip6_addr_t *paddr = netif_ip6_addr(pNetif, i);

            ip6_addr_t solicited_node_multicast_address;
            ip6_addr_set_solicitednode(&solicited_node_multicast_address,
                                       paddr->addr[3]);
            mld6_joingroup(paddr, &solicited_node_multicast_address);
        }

        /*
         * XXX: We must join the solicited-node multicast for the
         * addresses we do IPv6 NA-proxy for.  We map IPv6 loopback to
         * proxy address + 1.  We only need the low 24 bits, and those are
         * fixed.
         */
        {
            ip6_addr_t solicited_node_multicast_address;

            ip6_addr_set_solicitednode(&solicited_node_multicast_address,
                                       /* last 24 bits of the address */
                                       PP_HTONL(0x00000002));
            mld6_netif_joingroup(pNetif,  &solicited_node_multicast_address);
        }
    }

    proxy_init(&g_pLwipNat->m_LwipNetIf, &g_pLwipNat->m_ProxyOptions);

    natServiceProcessRegisteredPf(g_pLwipNat->m_vecPortForwardRule4);
    natServiceProcessRegisteredPf(g_pLwipNat->m_vecPortForwardRule6);
}


/*static*/ DECLCALLBACK(void) VBoxNetLwipNAT::onLwipTcpIpFini(void* arg)
{
    AssertPtrReturnVoid(arg);

    /* XXX: proxy finalization */
    netif_set_link_down(&g_pLwipNat->m_LwipNetIf);
    netif_set_down(&g_pLwipNat->m_LwipNetIf);
    netif_remove(&g_pLwipNat->m_LwipNetIf);

}

/*
 * Callback for netif_add() to initialize the interface.
 */
/*static*/ err_t VBoxNetLwipNAT::netifInit(netif *pNetif)
{
    err_t rcLwip = ERR_OK;

    AssertPtrReturn(pNetif, ERR_ARG);

    VBoxNetLwipNAT *pNat = static_cast<VBoxNetLwipNAT *>(pNetif->state);
    AssertPtrReturn(pNat, ERR_ARG);

    LogFlowFunc(("ENTER: pNetif[%c%c%d]\n", pNetif->name[0], pNetif->name[1], pNetif->num));
    /* validity */
    AssertReturn(   pNetif->name[0] == 'N'
                 && pNetif->name[1] == 'T', ERR_ARG);


    pNetif->hwaddr_len = sizeof(RTMAC);
    RTMAC mac = g_pLwipNat->getMacAddress();
    memcpy(pNetif->hwaddr, &mac, sizeof(RTMAC));

    pNat->m_u16Mtu = 1500; // XXX: FIXME
    pNetif->mtu = pNat->m_u16Mtu;

    pNetif->flags = NETIF_FLAG_BROADCAST
      | NETIF_FLAG_ETHARP                /* Don't bother driver with ARP and let Lwip resolve ARP handling */
      | NETIF_FLAG_ETHERNET;             /* Lwip works with ethernet too */

    pNetif->linkoutput = netifLinkoutput; /* ether-level-pipe */
    pNetif->output = etharp_output;       /* ip-pipe */

    if (pNat->m_ProxyOptions.ipv6_enabled) {
        pNetif->output_ip6 = ethip6_output;

        /* IPv6 link-local address in slot 0 */
        netif_create_ip6_linklocal_address(pNetif, /* :from_mac_48bit */ 1);
        netif_ip6_addr_set_state(pNetif, 0, IP6_ADDR_PREFERRED); // skip DAD

        /*
         * RFC 4193 Locally Assigned Global ID (ULA) in slot 1
         * [fd17:625c:f037:XXXX::1] where XXXX, 16 bit Subnet ID, are two
         * bytes from the middle of the IPv4 address, e.g. :dead: for
         * 10.222.173.1
         */
        u8_t nethi = ip4_addr2(&pNetif->ip_addr);
        u8_t netlo = ip4_addr3(&pNetif->ip_addr);

        ip6_addr_t *paddr = netif_ip6_addr(pNetif, 1);
        IP6_ADDR(paddr, 0,   0xFD, 0x17,   0x62, 0x5C);
        IP6_ADDR(paddr, 1,   0xF0, 0x37,  nethi, netlo);
        IP6_ADDR(paddr, 2,   0x00, 0x00,   0x00, 0x00);
        IP6_ADDR(paddr, 3,   0x00, 0x00,   0x00, 0x01);
        netif_ip6_addr_set_state(pNetif, 1, IP6_ADDR_PREFERRED);

#if LWIP_IPV6_SEND_ROUTER_SOLICIT
        pNetif->rs_count = 0;
#endif
    }

    LogFlowFunc(("LEAVE: %d\n", rcLwip));
    return rcLwip;
}


/*static*/ err_t VBoxNetLwipNAT::netifLinkoutput(netif *pNetif, pbuf *pPBuf)
{
    AssertPtrReturn(pNetif, ERR_ARG);
    AssertPtrReturn(pPBuf, ERR_ARG);

    VBoxNetLwipNAT *self = static_cast<VBoxNetLwipNAT *>(pNetif->state);
    AssertPtrReturn(self, ERR_IF);
    AssertReturn(self == g_pLwipNat, ERR_ARG);

    LogFlowFunc(("ENTER: pNetif[%c%c%d], pPbuf:%p\n",
                 pNetif->name[0],
                 pNetif->name[1],
                 pNetif->num,
                 pPBuf));

    RT_ZERO(VBoxNetLwipNAT::aXmitSeg);

    size_t idx = 0;
    for (struct pbuf *q = pPBuf; q != NULL; q = q->next, ++idx)
    {
        AssertReturn(idx < RT_ELEMENTS(VBoxNetLwipNAT::aXmitSeg), ERR_MEM);

#if ETH_PAD_SIZE
        if (q == pPBuf)
        {
            VBoxNetLwipNAT::aXmitSeg[idx].pv = (uint8_t *)q->payload + ETH_PAD_SIZE;
            VBoxNetLwipNAT::aXmitSeg[idx].cb = q->len - ETH_PAD_SIZE;
        }
        else
#endif
        {
            VBoxNetLwipNAT::aXmitSeg[idx].pv = q->payload;
            VBoxNetLwipNAT::aXmitSeg[idx].cb = q->len;
        }
    }

    int rc = self->sendBufferOnWire(VBoxNetLwipNAT::aXmitSeg, idx,
                                    pPBuf->tot_len - ETH_PAD_SIZE);
    AssertRCReturn(rc, ERR_IF);

    self->flushWire();

    LogFlowFunc(("LEAVE: %d\n", ERR_OK));
    return ERR_OK;
}


VBoxNetLwipNAT::VBoxNetLwipNAT(SOCKET icmpsock4, SOCKET icmpsock6) : VBoxNetBaseService("VBoxNetNAT", "nat-network")
{
    LogFlowFuncEnter();

    m_ProxyOptions.ipv6_enabled = 0;
    m_ProxyOptions.ipv6_defroute = 0;
    m_ProxyOptions.icmpsock4 = icmpsock4;
    m_ProxyOptions.icmpsock6 = icmpsock6;
    m_ProxyOptions.tftp_root = NULL;
    m_ProxyOptions.src4 = NULL;
    m_ProxyOptions.src6 = NULL;
    RT_ZERO(m_src4);
    RT_ZERO(m_src6);
    m_src4.sin_family = AF_INET;
    m_src6.sin6_family = AF_INET6;
#if HAVE_SA_LEN
    m_src4.sin_len = sizeof(m_src4);
    m_src6.sin6_len = sizeof(m_src6);
#endif
    m_ProxyOptions.nameservers = NULL;

    m_LwipNetIf.name[0] = 'N';
    m_LwipNetIf.name[1] = 'T';

    RTMAC mac;
    mac.au8[0] = 0x52;
    mac.au8[1] = 0x54;
    mac.au8[2] = 0;
    mac.au8[3] = 0x12;
    mac.au8[4] = 0x35;
    mac.au8[5] = 0;
    setMacAddress(mac);

    RTNETADDRIPV4 address;
    address.u     = RT_MAKE_U32_FROM_U8( 10,  0,  2,  2); // NB: big-endian
    setIpv4Address(address);

    address.u     = RT_H2N_U32_C(0xffffff00);
    setIpv4Netmask(address);

    fDontLoadRulesOnStartup = false;

    for(unsigned int i = 0; i < RT_ELEMENTS(g_aGetOptDef); ++i)
        addCommandLineOption(&g_aGetOptDef[i]);

    LogFlowFuncLeave();
}


VBoxNetLwipNAT::~VBoxNetLwipNAT()
{
    if (m_ProxyOptions.tftp_root)
    {
        RTStrFree((char *)m_ProxyOptions.tftp_root);
        m_ProxyOptions.tftp_root = NULL;
    }
    if (m_ProxyOptions.nameservers)
    {
        const char **pv = m_ProxyOptions.nameservers;
        while (*pv)
        {
            RTStrFree((char*)*pv);
            pv++;
        }
        RTMemFree(m_ProxyOptions.nameservers);
        m_ProxyOptions.nameservers = NULL;
    }
}


/*static*/ int VBoxNetLwipNAT::natServicePfRegister(NATSEVICEPORTFORWARDRULE& natPf)
{
    int lrc;

    int sockFamily = (natPf.Pfr.fPfrIPv6 ? PF_INET6 : PF_INET);
    int socketSpec;
    switch(natPf.Pfr.iPfrProto)
    {
        case IPPROTO_TCP:
            socketSpec = SOCK_STREAM;
            break;
        case IPPROTO_UDP:
            socketSpec = SOCK_DGRAM;
            break;
        default:
            return VERR_IGNORED;
    }

    const char *pszHostAddr = natPf.Pfr.szPfrHostAddr;
    if (pszHostAddr[0] == '\0')
    {
        if (sockFamily == PF_INET)
            pszHostAddr = "0.0.0.0";
        else
            pszHostAddr = "::";
    }

    lrc = fwspec_set(&natPf.FWSpec,
                     sockFamily,
                     socketSpec,
                     pszHostAddr,
                     natPf.Pfr.u16PfrHostPort,
                     natPf.Pfr.szPfrGuestAddr,
                     natPf.Pfr.u16PfrGuestPort);
    if (lrc != 0)
        return VERR_IGNORED;

    fwspec *pFwCopy = (fwspec *)RTMemDup(&natPf.FWSpec, sizeof(natPf.FWSpec));
    if (pFwCopy)
    {
        lrc = portfwd_rule_add(pFwCopy);
        if (lrc == 0)
            return VINF_SUCCESS; /* (pFwCopy is owned by lwip thread now.) */
        RTMemFree(pFwCopy);
    }
    else
        LogRel(("Unable to allocate memory for %s rule \"%s\"\n",
                natPf.Pfr.fPfrIPv6 ? "IPv6" : "IPv4",
                natPf.Pfr.szPfrName));
    return VERR_IGNORED;
}


/*static*/ int VBoxNetLwipNAT::natServiceProcessRegisteredPf(VECNATSERVICEPF& vecRules)
{
    ITERATORNATSERVICEPF it;
    for (it = vecRules.begin(); it != vecRules.end(); ++it)
    {
        NATSEVICEPORTFORWARDRULE &natPf = *it;

        LogRel(("Loading %s port-forwarding rule \"%s\": %s %s%s%s:%d -> %s%s%s:%d\n",
                natPf.Pfr.fPfrIPv6 ? "IPv6" : "IPv4",
                natPf.Pfr.szPfrName,
                natPf.Pfr.iPfrProto == IPPROTO_TCP ? "TCP" : "UDP",
                /* from */
                natPf.Pfr.fPfrIPv6 ? "[" : "",
                natPf.Pfr.szPfrHostAddr,
                natPf.Pfr.fPfrIPv6 ? "]" : "",
                natPf.Pfr.u16PfrHostPort,
                /* to */
                natPf.Pfr.fPfrIPv6 ? "[" : "",
                natPf.Pfr.szPfrGuestAddr,
                natPf.Pfr.fPfrIPv6 ? "]" : "",
                natPf.Pfr.u16PfrGuestPort));

        natServicePfRegister(natPf);
    }

    return VINF_SUCCESS;
}


/**
 * Main thread. Starts also the LWIP thread.
 */
int VBoxNetLwipNAT::init()
{
    LogFlowFuncEnter();

    /* virtualbox initialized in super class */
    int rc = ::VBoxNetBaseService::init();
    AssertRCReturn(rc, rc);

    std::string networkName = getNetworkName();
    rc = findNatNetwork(virtualbox, networkName, m_net);
    AssertRCReturn(rc, rc);

    {
        ComEventTypeArray eventTypes;
        eventTypes.push_back(VBoxEventType_OnNATNetworkPortForward);
        eventTypes.push_back(VBoxEventType_OnNATNetworkSetting);
        rc = createNatListener(m_NatListener, virtualbox, this, eventTypes);
        AssertRCReturn(rc, rc);
    }


    // resolver changes are reported on vbox but are retrieved from
    // host so stash a pointer for future lookups
    HRESULT hrc = virtualbox->COMGETTER(Host)(m_host.asOutParam());
    AssertComRCReturn(hrc, VERR_INTERNAL_ERROR);

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

    BOOL fIPv6Enabled = FALSE;
    hrc = m_net->COMGETTER(IPv6Enabled)(&fIPv6Enabled);
    AssertComRCReturn(hrc, VERR_NOT_FOUND);

    BOOL fIPv6DefaultRoute = FALSE;
    if (fIPv6Enabled)
    {
        hrc = m_net->COMGETTER(AdvertiseDefaultIPv6RouteEnabled)(&fIPv6DefaultRoute);
        AssertComRCReturn(hrc, VERR_NOT_FOUND);
    }

    m_ProxyOptions.ipv6_enabled = fIPv6Enabled;
    m_ProxyOptions.ipv6_defroute = fIPv6DefaultRoute;


    /*
     * Bind outgoing connections to the specified IP.
     */
    com::Bstr bstrSourceIpX;

    /* IPv4 */
    com::Bstr bstrSourceIp4Key = com::BstrFmt("NAT/%s/SourceIp4", networkName.c_str());
    hrc = virtualbox->GetExtraData(bstrSourceIp4Key.raw(), bstrSourceIpX.asOutParam());
    if (SUCCEEDED(hrc) && bstrSourceIpX.isNotEmpty())
    {
        RTNETADDRIPV4 addr;
        rc = RTNetStrToIPv4Addr(com::Utf8Str(bstrSourceIpX).c_str(), &addr);
        if (RT_SUCCESS(rc))
        {
            m_src4.sin_addr.s_addr = addr.u;
            m_ProxyOptions.src4 = &m_src4;

            LogRel(("Will use %RTnaipv4 as IPv4 source address\n",
                    m_src4.sin_addr.s_addr));
        }
        else
        {
            LogRel(("Failed to parse \"%s\" IPv4 source address specification\n",
                    com::Utf8Str(bstrSourceIpX).c_str()));
        }

        bstrSourceIpX.setNull();
    }

    /* IPv6 */
    com::Bstr bstrSourceIp6Key = com::BstrFmt("NAT/%s/SourceIp6", networkName.c_str());
    hrc = virtualbox->GetExtraData(bstrSourceIp6Key.raw(), bstrSourceIpX.asOutParam());
    if (SUCCEEDED(hrc) && bstrSourceIpX.isNotEmpty())
    {
        RTNETADDRIPV6 addr;
        char *pszZone = NULL;
        rc = RTNetStrToIPv6Addr(com::Utf8Str(bstrSourceIpX).c_str(), &addr, &pszZone);
        if (RT_SUCCESS(rc))
        {
            memcpy(&m_src6.sin6_addr, &addr, sizeof(addr));
            m_ProxyOptions.src6 = &m_src6;

            LogRel(("Will use %RTnaipv6 as IPv6 source address\n",
                    &m_src6.sin6_addr));
        }
        else
        {
            LogRel(("Failed to parse \"%s\" IPv6 source address specification\n",
                    com::Utf8Str(bstrSourceIpX).c_str()));
        }

        bstrSourceIpX.setNull();
    }


    if (!fDontLoadRulesOnStartup)
    {
        fetchNatPortForwardRules(m_net, false, m_vecPortForwardRule4);
        fetchNatPortForwardRules(m_net, true, m_vecPortForwardRule6);
    } /* if (!fDontLoadRulesOnStartup) */

    AddressToOffsetMapping tmp;
    rc = localMappings(m_net, tmp);
    if (RT_SUCCESS(rc) && !tmp.empty())
    {
        unsigned long i = 0;
        for (AddressToOffsetMapping::iterator it = tmp.begin();
             it != tmp.end() && i < RT_ELEMENTS(m_lo2off);
             ++it, ++i)
        {
            ip4_addr_set_u32(&m_lo2off[i].loaddr, it->first.u);
            m_lo2off[i].off = it->second;
        }

        m_loOptDescriptor.lomap = m_lo2off;
        m_loOptDescriptor.num_lomap = i;
        m_ProxyOptions.lomap_desc = &m_loOptDescriptor;
    }

    com::Bstr bstr;
    hrc = virtualbox->COMGETTER(HomeFolder)(bstr.asOutParam());
    AssertComRCReturn(hrc, VERR_NOT_FOUND);
    if (!bstr.isEmpty())
    {
        com::Utf8Str strTftpRoot(com::Utf8StrFmt("%ls%c%s",
                                     bstr.raw(), RTPATH_DELIMITER, "TFTP"));
        char *pszStrTemp;       // avoid const char ** vs char **
        rc = RTStrUtf8ToCurrentCP(&pszStrTemp, strTftpRoot.c_str());
        AssertRC(rc);
        m_ProxyOptions.tftp_root = pszStrTemp;
    }

    m_ProxyOptions.nameservers = getHostNameservers();

    /* end of COM initialization */

    rc = g_pLwipNat->tryGoOnline();
    if (RT_FAILURE(rc))
        return rc;

    /* this starts LWIP thread */
    vboxLwipCoreInitialize(VBoxNetLwipNAT::onLwipTcpIpInit, this);

    LogFlowFuncLeaveRC(rc);
    return rc;
}


const char **VBoxNetLwipNAT::getHostNameservers()
{
    if (m_host.isNull())
        return NULL;

    com::SafeArray<BSTR> aNameServers;
    HRESULT hrc = m_host->COMGETTER(NameServers)(ComSafeArrayAsOutParam(aNameServers));
    if (FAILED(hrc))
        return NULL;

    const size_t cNameServers = aNameServers.size();
    if (cNameServers == 0)
        return NULL;

    const char **ppcszNameServers =
        (const char **)RTMemAllocZ(sizeof(char *) * (cNameServers + 1));
    if (ppcszNameServers == NULL)
        return NULL;

    size_t idxLast = 0;
    for (size_t i = 0; i < cNameServers; ++i)
    {
        com::Utf8Str strNameServer(aNameServers[i]);
        ppcszNameServers[idxLast] = RTStrDup(strNameServer.c_str());
        if (ppcszNameServers[idxLast] != NULL)
            ++idxLast;
    }

    if (idxLast == 0)
    {
        RTMemFree(ppcszNameServers);
        return NULL;
    }

    return ppcszNameServers;
}


int VBoxNetLwipNAT::parseOpt(int rc, const RTGETOPTUNION& Val)
{
    switch (rc)
    {
        case 'p':
        case 'P':
        {
            NATSEVICEPORTFORWARDRULE Rule;
            VECNATSERVICEPF& rules = (rc == 'P'?
                                        m_vecPortForwardRule6
                                      : m_vecPortForwardRule4);

            fDontLoadRulesOnStartup = true;

            RT_ZERO(Rule);

            int rc2 = netPfStrToPf(Val.psz, (rc == 'P'), &Rule.Pfr);
            RT_NOREF_PV(rc2);
            rules.push_back(Rule);
            return VINF_SUCCESS;
        }
        default:;
    }
    return VERR_NOT_FOUND;
}


int VBoxNetLwipNAT::processFrame(void *pvFrame, size_t cbFrame)
{
    AssertPtrReturn(pvFrame, VERR_INVALID_PARAMETER);
    AssertReturn(cbFrame != 0, VERR_INVALID_PARAMETER);

    struct pbuf *p = pbuf_alloc(PBUF_RAW, (u16_t)cbFrame + ETH_PAD_SIZE, PBUF_POOL);
    if (RT_UNLIKELY(p == NULL))
        return VERR_NO_MEMORY;

    /*
     * The code below is inlined version of:
     *
     *   pbuf_header(p, -ETH_PAD_SIZE); // hide padding
     *   pbuf_take(p, pvFrame, cbFrame);
     *   pbuf_header(p, ETH_PAD_SIZE);  // reveal padding
     */
    struct pbuf *q = p;
    uint8_t *pu8Chunk = (uint8_t *)pvFrame;
    do {
        uint8_t *payload = (uint8_t *)q->payload;
        size_t len = q->len;

#if ETH_PAD_SIZE
        if (RT_LIKELY(q == p))  // single pbuf is large enough
        {
            payload += ETH_PAD_SIZE;
            len -= ETH_PAD_SIZE;
        }
#endif
        memcpy(payload, pu8Chunk, len);
        pu8Chunk += len;
        q = q->next;
    } while (RT_UNLIKELY(q != NULL));

    m_LwipNetIf.input(p, &m_LwipNetIf);
    return VINF_SUCCESS;
}


int VBoxNetLwipNAT::processGSO(PCPDMNETWORKGSO pGso, size_t cbFrame)
{
    if (!PDMNetGsoIsValid(pGso, cbFrame, cbFrame - sizeof(PDMNETWORKGSO)))
        return VERR_INVALID_PARAMETER;

    cbFrame -= sizeof(PDMNETWORKGSO);
    uint8_t         abHdrScratch[256];
    uint32_t const  cSegs = PDMNetGsoCalcSegmentCount(pGso,
                                                      cbFrame);
    for (uint32_t iSeg = 0; iSeg < cSegs; iSeg++)
    {
        uint32_t cbSegFrame;
        void    *pvSegFrame = PDMNetGsoCarveSegmentQD(pGso,
                                                      (uint8_t *)(pGso + 1),
                                                      cbFrame,
                                                      abHdrScratch,
                                                      iSeg,
                                                      cSegs,
                                                      &cbSegFrame);

        int rc = processFrame(pvSegFrame, cbSegFrame);
        if (RT_FAILURE(rc))
        {
            return rc;
        }
    }

    return VINF_SUCCESS;
}


int VBoxNetLwipNAT::run()
{
    /* Father starts receiving thread and enter event loop. */
    VBoxNetBaseService::run();

    vboxLwipCoreFinalize(VBoxNetLwipNAT::onLwipTcpIpFini, this);

    m_vecPortForwardRule4.clear();
    m_vecPortForwardRule6.clear();

    destroyNatListener(m_NatListener, virtualbox);
    destroyNatListener(m_VBoxListener, virtualbox);
    destroyClientListener(m_VBoxClientListener, virtualboxClient);

    return VINF_SUCCESS;
}


/**
 *  Entry point.
 */
extern "C" DECLEXPORT(int) TrustedMain(int argc, char **argv, char **envp)
{
    int rc;

    LogFlowFuncEnter();

    NOREF(envp);

#ifdef RT_OS_WINDOWS
    WSADATA wsaData;
    int err;

    err = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (err)
    {
        fprintf(stderr, "wsastartup: failed (%d)\n", err);
        return 1;
    }
#endif

    SOCKET icmpsock4 = INVALID_SOCKET;
    SOCKET icmpsock6 = INVALID_SOCKET;
#ifndef RT_OS_DARWIN
    const int icmpstype = SOCK_RAW;
#else
    /* on OS X it's not privileged */
    const int icmpstype = SOCK_DGRAM;
#endif

    icmpsock4 = socket(AF_INET, icmpstype, IPPROTO_ICMP);
    if (icmpsock4 == INVALID_SOCKET)
    {
        perror("IPPROTO_ICMP");
#ifdef VBOX_RAWSOCK_DEBUG_HELPER
        icmpsock4 = getrawsock(AF_INET);
#endif
    }

    if (icmpsock4 != INVALID_SOCKET)
    {
#ifdef ICMP_FILTER              //  Linux specific
        struct icmp_filter flt = {
            ~(uint32_t)(
                  (1U << ICMP_ECHOREPLY)
                | (1U << ICMP_DEST_UNREACH)
                | (1U << ICMP_TIME_EXCEEDED)
            )
        };

        int status = setsockopt(icmpsock4, SOL_RAW, ICMP_FILTER,
                                &flt, sizeof(flt));
        if (status < 0)
        {
            perror("ICMP_FILTER");
        }
#endif
    }

    icmpsock6 = socket(AF_INET6, icmpstype, IPPROTO_ICMPV6);
    if (icmpsock6 == INVALID_SOCKET)
    {
        perror("IPPROTO_ICMPV6");
#ifdef VBOX_RAWSOCK_DEBUG_HELPER
        icmpsock6 = getrawsock(AF_INET6);
#endif
    }

    if (icmpsock6 != INVALID_SOCKET)
    {
#ifdef ICMP6_FILTER             // Windows doesn't support RFC 3542 API
        /*
         * XXX: We do this here for now, not in pxping.c, to avoid
         * name clashes between lwIP and system headers.
         */
        struct icmp6_filter flt;
        ICMP6_FILTER_SETBLOCKALL(&flt);

        ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY, &flt);

        ICMP6_FILTER_SETPASS(ICMP6_DST_UNREACH, &flt);
        ICMP6_FILTER_SETPASS(ICMP6_PACKET_TOO_BIG, &flt);
        ICMP6_FILTER_SETPASS(ICMP6_TIME_EXCEEDED, &flt);
        ICMP6_FILTER_SETPASS(ICMP6_PARAM_PROB, &flt);

        int status = setsockopt(icmpsock6, IPPROTO_ICMPV6, ICMP6_FILTER,
                                &flt, sizeof(flt));
        if (status < 0)
        {
            perror("ICMP6_FILTER");
        }
#endif
    }

    HRESULT hrc = com::Initialize();
    if (FAILED(hrc))
    {
#ifdef VBOX_WITH_XPCOM
        if (hrc == NS_ERROR_FILE_ACCESS_DENIED)
        {
            char szHome[RTPATH_MAX] = "";
            int vrc = com::GetVBoxUserHomeDirectory(szHome, sizeof(szHome), false);
            if (RT_SUCCESS(vrc))
            {
                closesocket(icmpsock4);
                closesocket(icmpsock6);
                return RTMsgErrorExit(RTEXITCODE_FAILURE,
                                      "Failed to initialize COM: %s: %Rhrf",
                                      szHome, hrc);
            }
        }
#endif  // VBOX_WITH_XPCOM
        closesocket(icmpsock4);
        closesocket(icmpsock6);
        return RTMsgErrorExit(RTEXITCODE_FAILURE,
                              "Failed to initialize COM: %Rhrf", hrc);
    }

    rc = vboxNetNATLogInit(argc, argv);
    // shall we bail if we failed to init logging?

    g_pLwipNat = new VBoxNetLwipNAT(icmpsock4, icmpsock6);

    Log2(("NAT: initialization\n"));
    rc = g_pLwipNat->parseArgs(argc - 1, argv + 1);
    rc = (rc == 0) ? VINF_SUCCESS : VERR_GENERAL_FAILURE; /* XXX: FIXME */

    if (RT_SUCCESS(rc))
        rc = g_pLwipNat->init();

    if (RT_SUCCESS(rc))
        g_pLwipNat->run();

    delete g_pLwipNat;
    return 0;
}


static int vboxNetNATLogInit(int argc, char **argv)
{
    size_t cch;
    int rc;

    char szHome[RTPATH_MAX];
    rc = com::GetVBoxUserHomeDirectory(szHome, sizeof(szHome), false);
    if (RT_FAILURE(rc))
        return rc;

    const char *pcszNetwork = NULL;

    // XXX: This duplicates information from VBoxNetBaseService.cpp.
    // Perhaps option definitions should be exported as public static
    // member of VBoxNetBaseService?
    static const RTGETOPTDEF s_aOptions[] = {
        { "--network", 'n', RTGETOPT_REQ_STRING }
    };

    RTGETOPTSTATE GetState;
    RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1,
                 RTGETOPTINIT_FLAGS_NO_STD_OPTS);

    RTGETOPTUNION ValueUnion;
    int ch;
    while ((ch = RTGetOpt(&GetState, &ValueUnion)))
    {
        if (ch == 'n')
        {
            pcszNetwork = ValueUnion.psz;
            break;
        }
    }

    if (pcszNetwork == NULL)
        return VERR_MISSING;

    char szNetwork[RTPATH_MAX];
    rc = RTStrCopy(szNetwork, sizeof(szNetwork), pcszNetwork);
    if (RT_FAILURE(rc))
        return rc;

    // sanitize network name to be usable as a path component
    for (char *p = szNetwork; *p != '\0'; ++p)
    {
        if (RTPATH_IS_SEP(*p))
            *p = '_';
    }

    char szLogFile[RTPATH_MAX];
    cch = RTStrPrintf(szLogFile, sizeof(szLogFile),
                      "%s%c%s.log", szHome, RTPATH_DELIMITER, szNetwork);
    if (cch >= sizeof(szLogFile))
    {
        return VERR_BUFFER_OVERFLOW;
    }

    // sanitize network name some more to be usable as environment variable
    for (char *p = szNetwork; *p != '\0'; ++p)
    {
        if (*p != '_'
            && (*p < '0' || '9' < *p)
            && (*p < 'a' || 'z' < *p)
            && (*p < 'A' || 'Z' < *p))
        {
            *p = '_';
        }
    }

    char szEnvVarBase[128];
    cch = RTStrPrintf(szEnvVarBase, sizeof(szEnvVarBase),
                      "VBOXNET_%s_RELEASE_LOG", szNetwork);
    if (cch >= sizeof(szEnvVarBase))
        return VERR_BUFFER_OVERFLOW;

    rc = com::VBoxLogRelCreate("NAT Network",
                               szLogFile,
                               RTLOGFLAGS_PREFIX_TIME_PROG,
                               "all all.restrict -default.restrict",
                               szEnvVarBase,
                               RTLOGDEST_FILE,
                               32768 /* cMaxEntriesPerGroup */,
                               0 /* cHistory */,
                               0 /* uHistoryFileTime */,
                               0 /* uHistoryFileSize */,
                               NULL /*pErrInfo*/);

    /*
     * Provide immediate feedback if corresponding LogRel level is
     * enabled.  It's frustrating when you chase some rare event and
     * discover you didn't actually have the corresponding log level
     * enabled because of a typo in the environment variable name or
     * its content.
     */
#define LOG_PING(_log) _log((#_log " enabled\n"))
    LOG_PING(LogRel2);
    LOG_PING(LogRel3);
    LOG_PING(LogRel4);
    LOG_PING(LogRel5);
    LOG_PING(LogRel6);
    LOG_PING(LogRel7);
    LOG_PING(LogRel8);
    LOG_PING(LogRel9);
    LOG_PING(LogRel10);
    LOG_PING(LogRel11);
    LOG_PING(LogRel12);

    return rc;
}


static int fetchNatPortForwardRules(const ComNatPtr& nat, bool fIsIPv6, VECNATSERVICEPF& vec)
{
    HRESULT hrc;
    com::SafeArray<BSTR> rules;
    if (fIsIPv6)
        hrc = nat->COMGETTER(PortForwardRules6)(ComSafeArrayAsOutParam(rules));
    else
        hrc = nat->COMGETTER(PortForwardRules4)(ComSafeArrayAsOutParam(rules));
    AssertComRCReturn(hrc, VERR_INTERNAL_ERROR);

    NATSEVICEPORTFORWARDRULE Rule;
    for (size_t idxRules = 0; idxRules < rules.size(); ++idxRules)
    {
        Log(("%d-%s rule: %ls\n", idxRules, (fIsIPv6 ? "IPv6" : "IPv4"), rules[idxRules]));
        RT_ZERO(Rule);

        int rc = netPfStrToPf(com::Utf8Str(rules[idxRules]).c_str(), fIsIPv6,
                              &Rule.Pfr);
        if (RT_FAILURE(rc))
            continue;

        vec.push_back(Rule);
    }

    return VINF_SUCCESS;
}


#ifndef VBOX_WITH_HARDENING

int main(int argc, char **argv, char **envp)
{
    int rc = RTR3InitExe(argc, &argv, RTR3INIT_FLAGS_SUPLIB);
    if (RT_FAILURE(rc))
        return RTMsgInitFailure(rc);

    return TrustedMain(argc, argv, envp);
}

# if defined(RT_OS_WINDOWS)

#  if 0 /* Some copy and paste from DHCP that nobody explained why was diabled. */
static LRESULT CALLBACK WindowProc(HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    if(uMsg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc (hwnd, uMsg, wParam, lParam);
}

static LPCWSTR g_WndClassName = L"VBoxNetNatLwipClass";

static DWORD WINAPI MsgThreadProc(__in  LPVOID lpParameter)
{
     HWND                 hwnd = 0;
     HINSTANCE hInstance = (HINSTANCE)GetModuleHandle (NULL);
     bool bExit = false;

     /* Register the Window Class. */
     WNDCLASS wc;
     wc.style         = 0;
     wc.lpfnWndProc   = WindowProc;
     wc.cbClsExtra    = 0;
     wc.cbWndExtra    = sizeof(void *);
     wc.hInstance     = hInstance;
     wc.hIcon         = NULL;
     wc.hCursor       = NULL;
     wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
     wc.lpszMenuName  = NULL;
     wc.lpszClassName = g_WndClassName;

     ATOM atomWindowClass = RegisterClass(&wc);

     if (atomWindowClass != 0)
     {
         /* Create the window. */
         hwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
                               g_WndClassName, g_WndClassName, WS_POPUPWINDOW,
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

             DestroyWindow (hwnd);

             bExit = true;
         }

         UnregisterClass (g_WndClassName, hInstance);
     }

     if(bExit)
     {
         /* no need any accuracy here, in anyway the DHCP server usually gets terminated with TerminateProcess */
         exit(0);
     }

     return 0;
}
#  endif


/** (We don't want a console usually.) */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    RT_NOREF(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
#  if 0 /* some copy and paste from DHCP that nobody explained why was diabled. */
    NOREF(hInstance); NOREF(hPrevInstance); NOREF(lpCmdLine); NOREF(nCmdShow);

    HANDLE hThread = CreateThread(
      NULL, /*__in_opt   LPSECURITY_ATTRIBUTES lpThreadAttributes, */
      0, /*__in       SIZE_T dwStackSize, */
      MsgThreadProc, /*__in       LPTHREAD_START_ROUTINE lpStartAddress,*/
      NULL, /*__in_opt   LPVOID lpParameter,*/
      0, /*__in       DWORD dwCreationFlags,*/
      NULL /*__out_opt  LPDWORD lpThreadId*/
    );

    if(hThread != NULL)
        CloseHandle(hThread);

#  endif
    return main(__argc, __argv, environ);
}
# endif /* RT_OS_WINDOWS */

#endif /* !VBOX_WITH_HARDENING */
