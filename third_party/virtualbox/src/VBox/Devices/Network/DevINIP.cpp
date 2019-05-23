/* $Id: DevINIP.cpp $ */
/** @file
 * DevINIP - Internal Network IP stack device/service.
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DEV_INIP
#include <iprt/cdefs.h>     /* include early to allow RT_C_DECLS_BEGIN hack */
#include <iprt/mem.h>       /* include anything of ours that the lwip headers use. */
#include <iprt/semaphore.h>
#include <iprt/thread.h>
#include <iprt/alloca.h>
/* All lwip header files are not C++ safe. So hack around this. */
RT_C_DECLS_BEGIN
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/api.h"
#include "lwip/tcp_impl.h"
# if LWIP_IPV6
#  include "ipv6/lwip/ethip6.h"
# endif
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "netif/etharp.h"
RT_C_DECLS_END
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmnetifs.h>
#include <VBox/vmm/tm.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/uuid.h>

#include "VBoxDD.h"
#include "VBoxLwipCore.h"


/*********************************************************************************************************************************
*   Macros and Defines                                                                                                           *
*********************************************************************************************************************************/

/** Maximum frame size this device can handle. */
#define DEVINIP_MAX_FRAME 1514


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * Internal Network IP stack device instance data.
 *
 * @implements PDMIBASE
 * @implements PDMINETWORKDOWN
 */
typedef struct DEVINTNETIP
{
    /** The base interface for LUN\#0. */
    PDMIBASE                IBase;
    /** The network port this device provides (LUN\#0). */
    PDMINETWORKDOWN         INetworkDown;
    /** The network configuration port this device provides (LUN\#0). */
    PDMINETWORKCONFIG       INetworkConfig;
    /** The base interface of the network driver below us. */
    PPDMIBASE               pDrvBase;
    /** The connector of the network driver below us. */
    PPDMINETWORKUP          pDrv;
    /** Pointer to the device instance. */
    PPDMDEVINSR3            pDevIns;
    /** MAC address. */
    RTMAC                   MAC;
    /** Static IP address of the interface. */
    char                   *pszIP;
    /** Netmask of the interface. */
    char                   *pszNetmask;
    /** Gateway for the interface. */
    char                   *pszGateway;
    /** lwIP network interface description. */
    struct netif            IntNetIF;
    /** lwIP ARP timer. */
    PTMTIMERR3              ARPTimer;
    /** lwIP TCP fast timer. */
    PTMTIMERR3              TCPFastTimer;
    /** lwIP TCP slow timer. */
    PTMTIMERR3              TCPSlowTimer;
    /** lwIP semaphore to coordinate TCPIP init/terminate. */
    sys_sem_t               LWIPTcpInitSem;
    /** hack: get linking right. remove this eventually, once the device
     * provides a proper interface to all IP stack functions. */
    const void             *pLinkHack;
    /** Flag whether the link is up. */
    bool                    fLnkUp;
    /**
     * In callback we're getting status of interface adding operation (TCPIP thread),
     * but we need inform constructing routine whether it was success or not(EMT thread).
     */
    int rcInitialization;
} DEVINTNETIP, *PDEVINTNETIP;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/

/**
 * Pointer to the (only) instance data in this device.
 */
static PDEVINTNETIP g_pDevINIPData = NULL;

/*
 * really ugly hack to avoid linking problems on unix style platforms
 * using .a libraries for now.
 */
static const PFNRT g_pDevINILinkHack[] =
{
    (PFNRT)lwip_socket,
    (PFNRT)lwip_close,
    (PFNRT)lwip_setsockopt,
    (PFNRT)lwip_recv,
    (PFNRT)lwip_send,
    (PFNRT)lwip_select
};


#if 0 /* unused */
/**
 * Output a TCP/IP packet on the interface. Uses the generic lwIP ARP
 * code to resolve the address and call the link-level packet function.
 *
 * @returns lwIP error code
 * @param   netif   Interface on which to send IP packet.
 * @param   p       Packet data.
 * @param   ipaddr  Destination IP address.
 */
static err_t devINIPOutput(struct netif *netif, struct pbuf *p, struct ip_addr *ipaddr)
{
    err_t lrc;
    LogFlow(("%s: netif=%p p=%p ipaddr=%#04x\n", __FUNCTION__, netif, p,
             ipaddr->addr));

    lrc = lwip_etharp_output(netif, p, ipaddr);

    LogFlow(("%s: return %d\n", __FUNCTION__, lrc));
    return lrc;
}
#endif

/**
 * Output a raw packet on the interface.
 *
 * @returns lwIP error code
 * @param   netif   Interface on which to send frame.
 * @param   p       Frame data.
 */
static err_t devINIPOutputRaw(struct netif *netif, struct pbuf *p)
{
    NOREF(netif);
    int rc = VINF_SUCCESS;

    LogFlow(("%s: netif=%p p=%p\n", __FUNCTION__, netif, p));
    Assert(g_pDevINIPData);
    Assert(g_pDevINIPData->pDrv);

    /* Silently ignore packets being sent while lwIP isn't set up. */
    if (g_pDevINIPData)
    {
        PPDMSCATTERGATHER pSgBuf;

        rc = g_pDevINIPData->pDrv->pfnBeginXmit(g_pDevINIPData->pDrv, true /* fOnWorkerThread */);
        if (RT_FAILURE(rc))
            return ERR_IF;

        rc = g_pDevINIPData->pDrv->pfnAllocBuf(g_pDevINIPData->pDrv, DEVINIP_MAX_FRAME, NULL /*pGso*/, &pSgBuf);
        if (RT_SUCCESS(rc))
        {
#if ETH_PAD_SIZE
            lwip_pbuf_header(p, -ETH_PAD_SIZE);      /* drop the padding word */
#endif

            uint8_t *pbBuf = pSgBuf ? (uint8_t *)pSgBuf->aSegs[0].pvSeg : NULL;
            size_t   cbBuf = 0;
            for (struct pbuf *q = p; q != NULL; q = q->next)
            {
                if (cbBuf + q->len <= DEVINIP_MAX_FRAME)
                {
                    if (RT_LIKELY(pbBuf))
                    {
                        memcpy(pbBuf, q->payload, q->len);
                        pbBuf += q->len;
                    }
                    cbBuf += q->len;
                }
                else
                {
                    LogRel(("INIP: exceeded frame size\n"));
                    break;
                }
            }
            if (cbBuf)
            {
                pSgBuf->cbUsed = cbBuf;
                rc = g_pDevINIPData->pDrv->pfnSendBuf(g_pDevINIPData->pDrv, pSgBuf, true /* fOnWorkerThread */);
            }
            else
                rc = g_pDevINIPData->pDrv->pfnFreeBuf(g_pDevINIPData->pDrv, pSgBuf);

#if ETH_PAD_SIZE
            lwip_pbuf_header(p, ETH_PAD_SIZE);       /* reclaim the padding word */
#endif
        }

        g_pDevINIPData->pDrv->pfnEndXmit(g_pDevINIPData->pDrv);
    }

    err_t lrc = ERR_OK;
    if (RT_FAILURE(rc))
        lrc = ERR_IF;
    LogFlow(("%s: return %d (vbox: %Rrc)\n", __FUNCTION__, rc, lrc));
    return lrc;
}

/**
 * Implements the ethernet interface backend initialization for lwIP.
 *
 * @returns lwIP error code
 * @param   netif   Interface to configure.
 */
static err_t devINIPInterface(struct netif *netif)
{
    LogFlow(("%s: netif=%p\n", __FUNCTION__, netif));
    Assert(g_pDevINIPData != NULL);
    netif->state = g_pDevINIPData;
    netif->hwaddr_len = sizeof(g_pDevINIPData->MAC);
    memcpy(netif->hwaddr, &g_pDevINIPData->MAC, sizeof(g_pDevINIPData->MAC));
    netif->mtu = DEVINIP_MAX_FRAME;
    netif->flags = NETIF_FLAG_BROADCAST;
    netif->flags |= NETIF_FLAG_ETHARP;
    netif->flags |= NETIF_FLAG_ETHERNET;

#if LWIP_IPV6
    netif_create_ip6_linklocal_address(netif, 0);
    netif_ip6_addr_set_state(netif, 0, IP6_ADDR_VALID);
    netif->output_ip6 = ethip6_output;
    netif->ip6_autoconfig_enabled=1;
    LogFunc(("netif: ipv6:%RTnaipv6\n", &netif->ip6_addr[0].addr[0]));
#endif

    netif->output = lwip_etharp_output;
    netif->linkoutput = devINIPOutputRaw;

    LogFlow(("%s: success\n", __FUNCTION__));
    return ERR_OK;
}

/**
 * Parses CFGM parameters related to network connection
 */
static DECLCALLBACK(int) devINIPNetworkConfiguration(PPDMDEVINS pDevIns, PDEVINTNETIP pThis, PCFGMNODE pCfg)
{
    int rc = VINF_SUCCESS;
    rc = CFGMR3QueryStringAlloc(pCfg, "IP", &pThis->pszIP);
    if (RT_FAILURE(rc))
    {
        PDMDEV_SET_ERROR(pDevIns, rc,
                         N_("Configuration error: Failed to get the \"IP\" value"));
        /** @todo perhaps we should panic if IPv4 address isn't specify, with assumtion that
         * ISCSI target specified in IPv6 form.
         */
        return rc;
    }

    rc = CFGMR3QueryStringAlloc(pCfg, "Netmask", &pThis->pszNetmask);
    if (RT_FAILURE(rc))
    {
        PDMDEV_SET_ERROR(pDevIns, rc,
                         N_("Configuration error: Failed to get the \"Netmask\" value"));
        return rc;
    }
    rc = CFGMR3QueryStringAlloc(pCfg, "Gateway", &pThis->pszGateway);
    if (   RT_FAILURE(rc)
        && rc != VERR_CFGM_VALUE_NOT_FOUND)
    {
        PDMDEV_SET_ERROR(pDevIns, rc,
                         N_("Configuration error: Failed to get the \"Gateway\" value"));
        return rc;
    }
    return VINF_SUCCESS;
}

/**
 * Wait until data can be received.
 *
 * @returns VBox status code. VINF_SUCCESS means there is at least one receive descriptor available.
 * @param   pInterface  PDM network port interface pointer.
 * @param   cMillies    Number of milliseconds to wait. 0 means return immediately.
 */
static DECLCALLBACK(int) devINIPNetworkDown_WaitInputAvail(PPDMINETWORKDOWN pInterface, RTMSINTERVAL cMillies)
{
    RT_NOREF(pInterface, cMillies);
    LogFlow(("%s: pInterface=%p\n", __FUNCTION__, pInterface));
    LogFlow(("%s: return VINF_SUCCESS\n", __FUNCTION__));
    return VINF_SUCCESS;
}

/**
 * Receive data and pass it to lwIP for processing.
 *
 * @returns VBox status code
 * @param   pInterface  PDM network port interface pointer.
 * @param   pvBuf       Pointer to frame data.
 * @param   cb          Frame size.
 */
static DECLCALLBACK(int) devINIPNetworkDown_Input(PPDMINETWORKDOWN pInterface, const void *pvBuf, size_t cb)
{
    RT_NOREF(pInterface);
    const uint8_t *pbBuf = (const uint8_t *)pvBuf;
    size_t len = cb;
    const struct eth_hdr *ethhdr;
    struct pbuf *p, *q;

    LogFlow(("%s: pInterface=%p pvBuf=%p cb=%lu\n", __FUNCTION__, pInterface, pvBuf, cb));
    Assert(g_pDevINIPData);
    Assert(g_pDevINIPData->pDrv);

    /* Silently ignore packets being received while lwIP isn't set up. */
    if (!g_pDevINIPData)
    {
        LogFlow(("%s: return %Rrc (no global)\n", __FUNCTION__, VINF_SUCCESS));
        return VINF_SUCCESS;
    }

#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE;        /* allow room for Ethernet padding */
#endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    Assert((u16_t)len == len);
    p = lwip_pbuf_alloc(PBUF_RAW, (u16_t)len, PBUF_POOL);
    if (p != NULL)
    {
#if ETH_PAD_SIZE
        lwip_pbuf_header(p, -ETH_PAD_SIZE);      /* drop the padding word */
#endif

        for (q = p; q != NULL; q = q->next)
        {
            /* Fill the buffers, and clean out unused buffer space. */
            memcpy(q->payload, pbBuf, RT_MIN(cb, q->len));
            pbBuf += RT_MIN(cb, q->len);
            if (q->len > cb)
                memset(((uint8_t *)q->payload) + cb, '\0', q->len - cb);
            cb -= RT_MIN(cb, q->len);
        }

        ethhdr = (const struct eth_hdr *)p->payload;
        struct netif *iface = &g_pDevINIPData->IntNetIF;

        /* We've setup flags NETIF_FLAG_ETHARP and NETIF_FLAG_ETHERNET
          so this should be thread-safe. */
        tcpip_input(p,iface);
    }

    LogFlow(("%s: return %Rrc\n", __FUNCTION__, VINF_SUCCESS));
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMINETWORKDOWN,pfnXmitPending}
 */
static DECLCALLBACK(void) devINIPNetworkDown_XmitPending(PPDMINETWORKDOWN pInterface)
{
    NOREF(pInterface);
}


/**
 * Signals the end of lwIP TCPIP initialization.
 *
 * @note: TCPIP thread, corresponding EMT waiting on semaphore.
 * @param   arg     opaque argument, here the pointer to the PDEVINTNETIP.
 */
static DECLCALLBACK(void) devINIPTcpipInitDone(void *arg)
{
    PDEVINTNETIP pThis = (PDEVINTNETIP)arg;
    AssertPtrReturnVoid(arg);

    pThis->rcInitialization = VINF_SUCCESS;
    {
        struct netif *ret;
        struct ip_addr ipaddr, netmask, gw;
        struct in_addr ip;

        if (!inet_aton(pThis->pszIP, &ip))
        {
            pThis->rcInitialization = VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES;
            PDMDEV_SET_ERROR(pThis->pDevIns,
                             pThis->rcInitialization,
                             N_("Configuration error: Invalid \"IP\" value"));
            goto done;
        }
        memcpy(&ipaddr, &ip, sizeof(ipaddr));

        if (!inet_aton(pThis->pszNetmask, &ip))
        {
            pThis->rcInitialization = VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES;
            PDMDEV_SET_ERROR(pThis->pDevIns,
                             pThis->rcInitialization,
                             N_("Configuration error: Invalid \"Netmask\" value"));
            goto done;
        }
        memcpy(&netmask, &ip, sizeof(netmask));

        if (pThis->pszGateway)
        {
            if (!inet_aton(pThis->pszGateway, &ip))
            {
                pThis->rcInitialization = VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES;
                PDMDEV_SET_ERROR(pThis->pDevIns,
                                 pThis->rcInitialization,
                                 N_("Configuration error: Invalid \"Gateway\" value"));
                goto done;
            }

        }
        else
        {
            inet_aton(pThis->pszIP, &ip);
        }
        memcpy(&gw, &ip, sizeof(gw));

        pThis->IntNetIF.name[0] = 'I';
        pThis->IntNetIF.name[1] = 'N';

        ret = netif_add(&pThis->IntNetIF, &ipaddr, &netmask, &gw, NULL,
                        devINIPInterface, lwip_tcpip_input);

        if (!ret)
        {

            pThis->rcInitialization = VERR_NET_NO_NETWORK;
            PDMDEV_SET_ERROR(pThis->pDevIns,
                             pThis->rcInitialization,
                             N_("netif_add failed"));
            goto done;
        }

        lwip_netif_set_default(&pThis->IntNetIF);
        lwip_netif_set_up(&pThis->IntNetIF);
    }
    done:
    return;
}


/**
 * This callback is for finitializing our activity on TCPIP thread.
 * XXX: We do it only for new LWIP, old LWIP will stay broken for now.
 */
static DECLCALLBACK(void) devINIPTcpipFiniDone(void *arg)
{
    PDEVINTNETIP pThis = (PDEVINTNETIP)arg;
    AssertPtrReturnVoid(arg);

    netif_set_link_down(&pThis->IntNetIF);
    netif_set_down(&pThis->IntNetIF);
    netif_remove(&pThis->IntNetIF);
}


/**
 * Gets the current Media Access Control (MAC) address.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @param   pMac            Where to store the MAC address.
 * @thread  EMT
 */
static DECLCALLBACK(int) devINIPGetMac(PPDMINETWORKCONFIG pInterface, PRTMAC pMac)
{
    PDEVINTNETIP pThis = RT_FROM_MEMBER(pInterface, DEVINTNETIP, INetworkConfig);
    memcpy(pMac, pThis->MAC.au8, sizeof(RTMAC));
    return VINF_SUCCESS;
}

/**
 * Gets the new link state.
 *
 * @returns The current link state.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @thread  EMT
 */
static DECLCALLBACK(PDMNETWORKLINKSTATE) devINIPGetLinkState(PPDMINETWORKCONFIG pInterface)
{
    PDEVINTNETIP pThis = RT_FROM_MEMBER(pInterface, DEVINTNETIP, INetworkConfig);
    if (pThis->fLnkUp)
        return PDMNETWORKLINKSTATE_UP;
    return PDMNETWORKLINKSTATE_DOWN;
}


/**
 * Sets the new link state.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @param   enmState        The new link state
 */
static DECLCALLBACK(int) devINIPSetLinkState(PPDMINETWORKCONFIG pInterface, PDMNETWORKLINKSTATE enmState)
{
    PDEVINTNETIP pThis = RT_FROM_MEMBER(pInterface, DEVINTNETIP, INetworkConfig);
    bool fNewUp = enmState == PDMNETWORKLINKSTATE_UP;

    if (fNewUp != pThis->fLnkUp)
    {
        if (fNewUp)
        {
            LogFlowFunc(("Link is up\n"));
            pThis->fLnkUp = true;
        }
        else
        {
            LogFlowFunc(("Link is down\n"));
            pThis->fLnkUp = false;
        }
        if (pThis->pDrv)
            pThis->pDrv->pfnNotifyLinkChanged(pThis->pDrv, enmState);
    }
    return VINF_SUCCESS;
}

/* -=-=-=-=- PDMIBASE -=-=-=-=- */

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) devINIPQueryInterface(PPDMIBASE pInterface,
                                                  const char *pszIID)
{
    PDEVINTNETIP pThis = RT_FROM_MEMBER(pInterface, DEVINTNETIP, IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMINETWORKDOWN, &pThis->INetworkDown);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMINETWORKCONFIG, &pThis->INetworkConfig);
    return NULL;
}

/* -=-=-=-=- PDMDEVREG -=-=-=-=- */

/**
 * Destruct a device instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that any non-VM
 * resources can be freed correctly.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance data.
 */
static DECLCALLBACK(int) devINIPDestruct(PPDMDEVINS pDevIns)
{
    PDEVINTNETIP pThis = PDMINS_2_DATA(pDevIns, PDEVINTNETIP);

    LogFlow(("%s: pDevIns=%p\n", __FUNCTION__, pDevIns));
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);

    if (g_pDevINIPData != NULL)
        vboxLwipCoreFinalize(devINIPTcpipFiniDone, pThis);

    MMR3HeapFree(pThis->pszIP);
    pThis->pszIP = NULL;
    MMR3HeapFree(pThis->pszNetmask);
    pThis->pszNetmask = NULL;
    MMR3HeapFree(pThis->pszGateway);
    pThis->pszGateway = NULL;

    LogFlow(("%s: success\n", __FUNCTION__));
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) devINIPConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    RT_NOREF(iInstance);
    PDEVINTNETIP pThis = PDMINS_2_DATA(pDevIns, PDEVINTNETIP);

    LogFlow(("%s: pDevIns=%p iInstance=%d pCfg=%p\n", __FUNCTION__,
             pDevIns, iInstance, pCfg));

    Assert(iInstance == 0);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Validate the config.
     */
    if (!CFGMR3AreValuesValid(pCfg, "MAC\0IP\0"
                                    "IPv6\0"
                                    "Netmask\0Gateway\0"))
        return PDMDEV_SET_ERROR(pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                N_("Unknown Internal Networking IP configuration option"));

    /*
     * Init the static parts.
     */
    pThis->pszIP                            = NULL;
    pThis->pszNetmask                       = NULL;
    pThis->pszGateway                       = NULL;
    /* Pointer to device instance */
    pThis->pDevIns                          = pDevIns;
    /* IBase */
    pThis->IBase.pfnQueryInterface          = devINIPQueryInterface;
    /* INetworkDown */
    pThis->INetworkDown.pfnWaitReceiveAvail = devINIPNetworkDown_WaitInputAvail;
    pThis->INetworkDown.pfnReceive          = devINIPNetworkDown_Input;
    pThis->INetworkDown.pfnXmitPending      = devINIPNetworkDown_XmitPending;
    /* INetworkConfig */
    pThis->INetworkConfig.pfnGetMac         = devINIPGetMac;
    pThis->INetworkConfig.pfnGetLinkState   = devINIPGetLinkState;
    pThis->INetworkConfig.pfnSetLinkState   = devINIPSetLinkState;

    /*
     * Get the configuration settings.
     */
    int rc = CFGMR3QueryBytes(pCfg, "MAC", &pThis->MAC, sizeof(pThis->MAC));
    if (rc == VERR_CFGM_NOT_BYTES)
    {
        char szMAC[64];
        rc = CFGMR3QueryString(pCfg, "MAC", &szMAC[0], sizeof(szMAC));
        if (RT_SUCCESS(rc))
        {
            char *macStr = &szMAC[0];
            char *pMac = (char *)&pThis->MAC;
            for (uint32_t i = 0; i < 6; i++)
            {
                if (   !*macStr || !*(macStr + 1)
                    || *macStr == ':' || *(macStr + 1) == ':')
                    return PDMDEV_SET_ERROR(pDevIns,
                                            VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                            N_("Configuration error: Invalid \"MAC\" value"));
                char c1 = *macStr++ - '0';
                if (c1 > 9)
                    c1 -= 7;
                char c2 = *macStr++ - '0';
                if (c2 > 9)
                    c2 -= 7;
                *pMac++ = ((c1 & 0x0f) << 4) | (c2 & 0x0f);
                if (i != 5 && *macStr == ':')
                    macStr++;
            }
        }
    }
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the \"MAC\" value"));
    rc = devINIPNetworkConfiguration(pDevIns, pThis, pCfg);
    AssertLogRelRCReturn(rc, rc);

    /*
     * Attach driver and query the network connector interface.
     */
    rc = PDMDevHlpDriverAttach(pDevIns, 0, &pThis->IBase, &pThis->pDrvBase, "Network Port");
    if (RT_FAILURE(rc))
    {
        pThis->pDrvBase = NULL;
        pThis->pDrv = NULL;
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Error attaching device below us"));
    }
    pThis->pDrv = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMINETWORKUP);
    AssertMsgReturn(pThis->pDrv, ("Failed to obtain the PDMINETWORKUP interface!\n"), VERR_PDM_MISSING_INTERFACE_BELOW);


    /*
     * Set up global pointer to interface data.
     */
    g_pDevINIPData = pThis;


    /* link hack */
    pThis->pLinkHack = g_pDevINILinkHack;

    /*
     * Initialize lwIP.
     */
    vboxLwipCoreInitialize(devINIPTcpipInitDone, pThis);

    /* this rc could be updated in devINIPTcpInitDone thread */
    AssertRCReturn(pThis->rcInitialization, pThis->rcInitialization);


    LogFlow(("%s: return %Rrc\n", __FUNCTION__, rc));
    return rc;
}


/**
 * Query whether lwIP is initialized or not. Since there is only a single
 * instance of this device ever for a VM, it can be a global function.
 *
 * @returns True if lwIP is initialized.
 */
bool DevINIPConfigured(void)
{
    return g_pDevINIPData != NULL;
}


/**
 * Internal network IP stack device registration record.
 */
const PDMDEVREG g_DeviceINIP =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "IntNetIP",
    /* szRCMod/szR0Mod */
    "",
    "",
    /* pszDescription */
    "Internal Network IP stack device",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS,
    /* fClass. As this is used by the storage devices, it must come earlier. */
    PDM_DEVREG_CLASS_VMM_DEV,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(DEVINTNETIP),
    /* pfnConstruct */
    devINIPConstruct,
    /* pfnDestruct */
    devINIPDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnQueryInterface */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};
