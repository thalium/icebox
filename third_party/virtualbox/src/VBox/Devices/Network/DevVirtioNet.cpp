/* $Id: DevVirtioNet.cpp $ */
/** @file
 * DevVirtioNet - Virtio Network Device
 */

/*
 * Copyright (C) 2009-2016 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DEV_VIRTIO_NET
#define VNET_GC_SUPPORT
#define VNET_WITH_GSO
#define VNET_WITH_MERGEABLE_RX_BUFS

#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmnetifs.h>
#include <iprt/asm.h>
#include <iprt/net.h>
#include <iprt/semaphore.h>
#ifdef IN_RING3
# include <iprt/mem.h>
# include <iprt/uuid.h>
#endif /* IN_RING3 */
#include <VBox/VBoxPktDmp.h>
#include "VBoxDD.h"
#include "../VirtIO/Virtio.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#ifndef VBOX_DEVICE_STRUCT_TESTCASE

#define INSTANCE(pThis) pThis->VPCI.szInstance
#define STATUS pThis->config.uStatus

#ifdef IN_RING3

#define VNET_PCI_CLASS               0x0200
#define VNET_N_QUEUES                3
#define VNET_NAME_FMT                "VNet%d"

#if 0
/* Virtio Block Device */
#define VNET_PCI_CLASS               0x0180
#define VNET_N_QUEUES                2
#define VNET_NAME_FMT                "VBlk%d"
#endif

#endif /* IN_RING3 */

#endif /* VBOX_DEVICE_STRUCT_TESTCASE */


#define VNET_TX_DELAY           150   /**< 150 microseconds */
#define VNET_MAX_FRAME_SIZE     65535 + 18  /**< Max IP packet size + Ethernet header with VLAN tag */
#define VNET_MAC_FILTER_LEN     32
#define VNET_MAX_VID            (1 << 12)

/** @name Virtio net features
 * @{  */
#define VNET_F_CSUM       0x00000001  /**< Host handles pkts w/ partial csum */
#define VNET_F_GUEST_CSUM 0x00000002  /**< Guest handles pkts w/ partial csum */
#define VNET_F_MAC        0x00000020  /**< Host has given MAC address. */
#define VNET_F_GSO        0x00000040  /**< Host handles pkts w/ any GSO type */
#define VNET_F_GUEST_TSO4 0x00000080  /**< Guest can handle TSOv4 in. */
#define VNET_F_GUEST_TSO6 0x00000100  /**< Guest can handle TSOv6 in. */
#define VNET_F_GUEST_ECN  0x00000200  /**< Guest can handle TSO[6] w/ ECN in. */
#define VNET_F_GUEST_UFO  0x00000400  /**< Guest can handle UFO in. */
#define VNET_F_HOST_TSO4  0x00000800  /**< Host can handle TSOv4 in. */
#define VNET_F_HOST_TSO6  0x00001000  /**< Host can handle TSOv6 in. */
#define VNET_F_HOST_ECN   0x00002000  /**< Host can handle TSO[6] w/ ECN in. */
#define VNET_F_HOST_UFO   0x00004000  /**< Host can handle UFO in. */
#define VNET_F_MRG_RXBUF  0x00008000  /**< Host can merge receive buffers. */
#define VNET_F_STATUS     0x00010000  /**< virtio_net_config.status available */
#define VNET_F_CTRL_VQ    0x00020000  /**< Control channel available */
#define VNET_F_CTRL_RX    0x00040000  /**< Control channel RX mode support */
#define VNET_F_CTRL_VLAN  0x00080000  /**< Control channel VLAN filtering */
/** @} */

#define VNET_S_LINK_UP    1


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
#ifdef _MSC_VER
struct VNetPCIConfig
#else /* !_MSC_VER */
struct __attribute__ ((__packed__)) VNetPCIConfig /** @todo r=bird: Use #pragma pack if necessary, that's portable! */
#endif /* !_MSC_VER */
{
    RTMAC    mac;
    uint16_t uStatus;
};
AssertCompileMemberOffset(struct VNetPCIConfig, uStatus, 6);

/**
 * Device state structure. Holds the current state of device.
 *
 * @extends     VPCISTATE
 * @implements  PDMINETWORKDOWN
 * @implements  PDMINETWORKCONFIG
 */
typedef struct VNetState_st
{
    /* VPCISTATE must be the first member! */
    VPCISTATE               VPCI;

//    PDMCRITSECT             csRx;                           /**< Protects RX queue. */

    PDMINETWORKDOWN         INetworkDown;
    PDMINETWORKCONFIG       INetworkConfig;
    R3PTRTYPE(PPDMIBASE)    pDrvBase;                 /**< Attached network driver. */
    R3PTRTYPE(PPDMINETWORKUP) pDrv;    /**< Connector of attached network driver. */

    R3PTRTYPE(PPDMQUEUE)    pCanRxQueueR3;           /**< Rx wakeup signaller - R3. */
    R0PTRTYPE(PPDMQUEUE)    pCanRxQueueR0;           /**< Rx wakeup signaller - R0. */
    RCPTRTYPE(PPDMQUEUE)    pCanRxQueueRC;           /**< Rx wakeup signaller - RC. */
# if HC_ARCH_BITS == 64
    uint32_t                padding;
# endif

    /**< Link Up(/Restore) Timer. */
    PTMTIMERR3              pLinkUpTimer;

#ifdef VNET_TX_DELAY
    /**< Transmit Delay Timer - R3. */
    PTMTIMERR3              pTxTimerR3;
    /**< Transmit Delay Timer - R0. */
    PTMTIMERR0              pTxTimerR0;
    /**< Transmit Delay Timer - GC. */
    PTMTIMERRC              pTxTimerRC;

# if HC_ARCH_BITS == 64
    uint32_t                padding2;
# endif

    uint32_t                u32i;
    uint32_t                u32AvgDiff;
    uint32_t                u32MinDiff;
    uint32_t                u32MaxDiff;
    uint64_t                u64NanoTS;
#endif /* VNET_TX_DELAY */

    /** Indicates transmission in progress -- only one thread is allowed. */
    uint32_t                uIsTransmitting;

    /** PCI config area holding MAC address as well as TBD. */
    struct VNetPCIConfig    config;
    /** MAC address obtained from the configuration. */
    RTMAC                   macConfigured;
    /** True if physical cable is attached in configuration. */
    bool                    fCableConnected;
    /** Link up delay (in milliseconds). */
    uint32_t                cMsLinkUpDelay;

    uint32_t                alignment;

    /** Number of packet being sent/received to show in debug log. */
    uint32_t                u32PktNo;

    /** N/A: */
    bool volatile           fMaybeOutOfSpace;

    /** Promiscuous mode -- RX filter accepts all packets. */
    bool                    fPromiscuous;
    /** AllMulti mode -- RX filter accepts all multicast packets. */
    bool                    fAllMulti;
    /** The number of actually used slots in aMacTable. */
    uint32_t                nMacFilterEntries;
    /** Array of MAC addresses accepted by RX filter. */
    RTMAC                   aMacFilter[VNET_MAC_FILTER_LEN];
    /** Bit array of VLAN filter, one bit per VLAN ID. */
    uint8_t                 aVlanFilter[VNET_MAX_VID / sizeof(uint8_t)];

    R3PTRTYPE(PVQUEUE)      pRxQueue;
    R3PTRTYPE(PVQUEUE)      pTxQueue;
    R3PTRTYPE(PVQUEUE)      pCtlQueue;
    /* Receive-blocking-related fields ***************************************/

    /** EMT: Gets signalled when more RX descriptors become available. */
    RTSEMEVENT              hEventMoreRxDescAvail;

    /** @name Statistic
     * @{ */
    STAMCOUNTER             StatReceiveBytes;
    STAMCOUNTER             StatTransmitBytes;
    STAMCOUNTER             StatReceiveGSO;
    STAMCOUNTER             StatTransmitPackets;
    STAMCOUNTER             StatTransmitGSO;
    STAMCOUNTER             StatTransmitCSum;
#if defined(VBOX_WITH_STATISTICS)
    STAMPROFILE             StatReceive;
    STAMPROFILE             StatReceiveStore;
    STAMPROFILEADV          StatTransmit;
    STAMPROFILE             StatTransmitSend;
    STAMPROFILE             StatRxOverflow;
    STAMCOUNTER             StatRxOverflowWakeup;
#endif /* VBOX_WITH_STATISTICS */
    /** @}  */
} VNETSTATE;
/** Pointer to a virtual I/O network device state. */
typedef VNETSTATE *PVNETSTATE;

#ifndef VBOX_DEVICE_STRUCT_TESTCASE

#define VNETHDR_F_NEEDS_CSUM     1       // Use u16CSumStart, u16CSumOffset

#define VNETHDR_GSO_NONE         0       // Not a GSO frame
#define VNETHDR_GSO_TCPV4        1       // GSO frame, IPv4 TCP (TSO)
#define VNETHDR_GSO_UDP          3       // GSO frame, IPv4 UDP (UFO)
#define VNETHDR_GSO_TCPV6        4       // GSO frame, IPv6 TCP
#define VNETHDR_GSO_ECN          0x80    // TCP has ECN set

struct VNetHdr
{
    uint8_t  u8Flags;
    uint8_t  u8GSOType;
    uint16_t u16HdrLen;
    uint16_t u16GSOSize;
    uint16_t u16CSumStart;
    uint16_t u16CSumOffset;
};
typedef struct VNetHdr VNETHDR;
typedef VNETHDR *PVNETHDR;
AssertCompileSize(VNETHDR, 10);

struct VNetHdrMrx
{
    VNETHDR  Hdr;
    uint16_t u16NumBufs;
};
typedef struct VNetHdrMrx VNETHDRMRX;
typedef VNETHDRMRX *PVNETHDRMRX;
AssertCompileSize(VNETHDRMRX, 12);

AssertCompileMemberOffset(VNETSTATE, VPCI, 0);

#define VNET_OK                    0
#define VNET_ERROR                 1
typedef uint8_t VNETCTLACK;

#define VNET_CTRL_CLS_RX_MODE          0
#define VNET_CTRL_CMD_RX_MODE_PROMISC  0
#define VNET_CTRL_CMD_RX_MODE_ALLMULTI 1

#define VNET_CTRL_CLS_MAC              1
#define VNET_CTRL_CMD_MAC_TABLE_SET    0

#define VNET_CTRL_CLS_VLAN             2
#define VNET_CTRL_CMD_VLAN_ADD         0
#define VNET_CTRL_CMD_VLAN_DEL         1


struct VNetCtlHdr
{
    uint8_t  u8Class;
    uint8_t  u8Command;
};
typedef struct VNetCtlHdr VNETCTLHDR;
typedef VNETCTLHDR *PVNETCTLHDR;
AssertCompileSize(VNETCTLHDR, 2);

#ifdef IN_RING3

/** Returns true if large packets are written into several RX buffers. */
DECLINLINE(bool) vnetMergeableRxBuffers(PVNETSTATE pThis)
{
    return !!(pThis->VPCI.uGuestFeatures & VNET_F_MRG_RXBUF);
}

DECLINLINE(int) vnetCsEnter(PVNETSTATE pThis, int rcBusy)
{
    return vpciCsEnter(&pThis->VPCI, rcBusy);
}

DECLINLINE(void) vnetCsLeave(PVNETSTATE pThis)
{
    vpciCsLeave(&pThis->VPCI);
}

#endif /* IN_RING3 */

DECLINLINE(int) vnetCsRxEnter(PVNETSTATE pThis, int rcBusy)
{
    RT_NOREF_PV(pThis);
    RT_NOREF_PV(rcBusy);
    // STAM_PROFILE_START(&pThis->CTXSUFF(StatCsRx), a);
    // int rc = PDMCritSectEnter(&pThis->csRx, rcBusy);
    // STAM_PROFILE_STOP(&pThis->CTXSUFF(StatCsRx), a);
    // return rc;
    return VINF_SUCCESS;
}

DECLINLINE(void) vnetCsRxLeave(PVNETSTATE pThis)
{
    RT_NOREF_PV(pThis);
    // PDMCritSectLeave(&pThis->csRx);
}

#ifdef IN_RING3
/**
 * Dump a packet to debug log.
 *
 * @param   pThis       The device state structure.
 * @param   pbPacket    The packet.
 * @param   cb          The size of the packet.
 * @param   pszText     A string denoting direction of packet transfer.
 */
DECLINLINE(void) vnetPacketDump(PVNETSTATE pThis, const uint8_t *pbPacket, size_t cb, const char *pszText)
{
# ifdef DEBUG
#  if 0
    Log(("%s %s packet #%d (%d bytes):\n",
         INSTANCE(pThis), pszText, ++pThis->u32PktNo, cb));
    Log3(("%.*Rhxd\n", cb, pbPacket));
#  else
    vboxEthPacketDump(INSTANCE(pThis), pszText, pbPacket, (uint32_t)cb);
#  endif
# else
    RT_NOREF4(pThis, pbPacket, cb, pszText);
# endif
}
#endif /* IN_RING3 */

/**
 * Print features given in uFeatures to debug log.
 *
 * @param   pThis      The device state structure.
 * @param   fFeatures   Descriptions of which features to print.
 * @param   pcszText    A string to print before the list of features.
 */
DECLINLINE(void) vnetPrintFeatures(PVNETSTATE pThis, uint32_t fFeatures, const char *pcszText)
{
#ifdef DEBUG
    static struct
    {
        uint32_t uMask;
        const char *pcszDesc;
    } const s_aFeatures[] =
    {
        { VNET_F_CSUM,       "host handles pkts w/ partial csum" },
        { VNET_F_GUEST_CSUM, "guest handles pkts w/ partial csum" },
        { VNET_F_MAC,        "host has given MAC address" },
        { VNET_F_GSO,        "host handles pkts w/ any GSO type" },
        { VNET_F_GUEST_TSO4, "guest can handle TSOv4 in" },
        { VNET_F_GUEST_TSO6, "guest can handle TSOv6 in" },
        { VNET_F_GUEST_ECN,  "guest can handle TSO[6] w/ ECN in" },
        { VNET_F_GUEST_UFO,  "guest can handle UFO in" },
        { VNET_F_HOST_TSO4,  "host can handle TSOv4 in" },
        { VNET_F_HOST_TSO6,  "host can handle TSOv6 in" },
        { VNET_F_HOST_ECN,   "host can handle TSO[6] w/ ECN in" },
        { VNET_F_HOST_UFO,   "host can handle UFO in" },
        { VNET_F_MRG_RXBUF,  "host can merge receive buffers" },
        { VNET_F_STATUS,     "virtio_net_config.status available" },
        { VNET_F_CTRL_VQ,    "control channel available" },
        { VNET_F_CTRL_RX,    "control channel RX mode support" },
        { VNET_F_CTRL_VLAN,  "control channel VLAN filtering" }
    };

    Log3(("%s %s:\n", INSTANCE(pThis), pcszText));
    for (unsigned i = 0; i < RT_ELEMENTS(s_aFeatures); ++i)
    {
        if (s_aFeatures[i].uMask & fFeatures)
            Log3(("%s --> %s\n", INSTANCE(pThis), s_aFeatures[i].pcszDesc));
    }
#else  /* !DEBUG */
    RT_NOREF3(pThis, fFeatures, pcszText);
#endif /* !DEBUG */
}

static DECLCALLBACK(uint32_t) vnetIoCb_GetHostFeatures(void *pvState)
{
    RT_NOREF_PV(pvState);

    /* We support:
     * - Host-provided MAC address
     * - Link status reporting in config space
     * - Control queue
     * - RX mode setting
     * - MAC filter table
     * - VLAN filter
     */
    return VNET_F_MAC
        | VNET_F_STATUS
        | VNET_F_CTRL_VQ
        | VNET_F_CTRL_RX
        | VNET_F_CTRL_VLAN
#ifdef VNET_WITH_GSO
        | VNET_F_CSUM
        | VNET_F_HOST_TSO4
        | VNET_F_HOST_TSO6
        | VNET_F_HOST_UFO
        | VNET_F_GUEST_CSUM   /* We expect the guest to accept partial TCP checksums (see @bugref{4796}) */
        | VNET_F_GUEST_TSO4
        | VNET_F_GUEST_TSO6
        | VNET_F_GUEST_UFO
#endif
#ifdef VNET_WITH_MERGEABLE_RX_BUFS
        | VNET_F_MRG_RXBUF
#endif
        ;
}

static DECLCALLBACK(uint32_t) vnetIoCb_GetHostMinimalFeatures(void *pvState)
{
    RT_NOREF_PV(pvState);
    return VNET_F_MAC;
}

static DECLCALLBACK(void) vnetIoCb_SetHostFeatures(void *pvState, uint32_t fFeatures)
{
    /** @todo Nothing to do here yet */
    PVNETSTATE pThis = (PVNETSTATE)pvState;
    LogFlow(("%s vnetIoCb_SetHostFeatures: uFeatures=%x\n", INSTANCE(pThis), fFeatures));
    vnetPrintFeatures(pThis, fFeatures, "The guest negotiated the following features");
}

static DECLCALLBACK(int) vnetIoCb_GetConfig(void *pvState, uint32_t offCfg, uint32_t cb, void *data)
{
    PVNETSTATE pThis = (PVNETSTATE)pvState;
    if (offCfg + cb > sizeof(struct VNetPCIConfig))
    {
        Log(("%s vnetIoCb_GetConfig: Read beyond the config structure is attempted (offCfg=%#x cb=%x).\n", INSTANCE(pThis), offCfg, cb));
        return VERR_IOM_IOPORT_UNUSED;
    }
    memcpy(data, (uint8_t *)&pThis->config + offCfg, cb);
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) vnetIoCb_SetConfig(void *pvState, uint32_t offCfg, uint32_t cb, void *data)
{
    PVNETSTATE pThis = (PVNETSTATE)pvState;
    if (offCfg + cb > sizeof(struct VNetPCIConfig))
    {
        Log(("%s vnetIoCb_SetConfig: Write beyond the config structure is attempted (offCfg=%#x cb=%x).\n", INSTANCE(pThis), offCfg, cb));
        if (offCfg < sizeof(struct VNetPCIConfig))
            memcpy((uint8_t *)&pThis->config + offCfg, data,
                   sizeof(struct VNetPCIConfig) - offCfg);
        return VINF_SUCCESS;
    }
    memcpy((uint8_t *)&pThis->config + offCfg, data, cb);
    return VINF_SUCCESS;
}

/**
 * Hardware reset. Revert all registers to initial values.
 *
 * @param   pThis      The device state structure.
 */
static DECLCALLBACK(int) vnetIoCb_Reset(void *pvState)
{
    PVNETSTATE pThis = (PVNETSTATE)pvState;
    Log(("%s Reset triggered\n", INSTANCE(pThis)));

    int rc = vnetCsRxEnter(pThis, VERR_SEM_BUSY);
    if (RT_UNLIKELY(rc != VINF_SUCCESS))
    {
        LogRel(("vnetIoCb_Reset failed to enter RX critical section!\n"));
        return rc;
    }
    vpciReset(&pThis->VPCI);
    vnetCsRxLeave(pThis);

    /// @todo Implement reset
    if (pThis->fCableConnected)
        STATUS = VNET_S_LINK_UP;
    else
        STATUS = 0;
    Log(("%s vnetIoCb_Reset: Link is %s\n", INSTANCE(pThis), pThis->fCableConnected ? "up" : "down"));

    /*
     * By default we pass all packets up since the older guests cannot control
     * virtio mode.
     */
    pThis->fPromiscuous      = true;
    pThis->fAllMulti         = false;
    pThis->nMacFilterEntries = 0;
    memset(pThis->aMacFilter,  0, VNET_MAC_FILTER_LEN * sizeof(RTMAC));
    memset(pThis->aVlanFilter, 0, sizeof(pThis->aVlanFilter));
    pThis->uIsTransmitting   = 0;
#ifndef IN_RING3
    return VINF_IOM_R3_IOPORT_WRITE;
#else
    if (pThis->pDrv)
        pThis->pDrv->pfnSetPromiscuousMode(pThis->pDrv, true);
    return VINF_SUCCESS;
#endif
}

#ifdef IN_RING3

/**
 * Wakeup the RX thread.
 */
static void vnetWakeupReceive(PPDMDEVINS pDevIns)
{
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);
    if (    pThis->fMaybeOutOfSpace
        &&  pThis->hEventMoreRxDescAvail != NIL_RTSEMEVENT)
    {
        STAM_COUNTER_INC(&pThis->StatRxOverflowWakeup);
        Log(("%s Waking up Out-of-RX-space semaphore\n",  INSTANCE(pThis)));
        RTSemEventSignal(pThis->hEventMoreRxDescAvail);
    }
}


/**
 * Takes down the link temporarily if it's current status is up.
 *
 * This is used during restore and when replumbing the network link.
 *
 * The temporary link outage is supposed to indicate to the OS that all network
 * connections have been lost and that it for instance is appropriate to
 * renegotiate any DHCP lease.
 *
 * @param  pThis        The Virtual I/O network device state.
 */
static void vnetTempLinkDown(PVNETSTATE pThis)
{
    if (STATUS & VNET_S_LINK_UP)
    {
        STATUS &= ~VNET_S_LINK_UP;
        vpciRaiseInterrupt(&pThis->VPCI, VERR_SEM_BUSY, VPCI_ISR_CONFIG);
        /* Restore the link back in 5 seconds. */
        int rc = TMTimerSetMillies(pThis->pLinkUpTimer, pThis->cMsLinkUpDelay);
        AssertRC(rc);
        Log(("%s vnetTempLinkDown: Link is down temporarily\n", INSTANCE(pThis)));
    }
}


/**
 * @callback_method_impl{FNTMTIMERDEV, Link Up Timer handler.}
 */
static DECLCALLBACK(void) vnetLinkUpTimer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF(pTimer);
    PVNETSTATE pThis = (PVNETSTATE)pvUser;

    int rc = vnetCsEnter(pThis, VERR_SEM_BUSY);
    if (RT_UNLIKELY(rc != VINF_SUCCESS))
        return;
    STATUS |= VNET_S_LINK_UP;
    vpciRaiseInterrupt(&pThis->VPCI, VERR_SEM_BUSY, VPCI_ISR_CONFIG);
    vnetWakeupReceive(pDevIns);
    vnetCsLeave(pThis);
    Log(("%s vnetLinkUpTimer: Link is up\n", INSTANCE(pThis)));
    if (pThis->pDrv)
        pThis->pDrv->pfnNotifyLinkChanged(pThis->pDrv, PDMNETWORKLINKSTATE_UP);
}


/**
 * @callback_method_impl{FNPDMQUEUEDEV, Handler for the wakeup signaller queue.}
 */
static DECLCALLBACK(bool) vnetCanRxQueueConsumer(PPDMDEVINS pDevIns, PPDMQUEUEITEMCORE pItem)
{
    RT_NOREF(pItem);
    vnetWakeupReceive(pDevIns);
    return true;
}

#endif /* IN_RING3 */

/**
 * This function is called when the driver becomes ready.
 *
 * @param   pThis      The device state structure.
 */
static DECLCALLBACK(void) vnetIoCb_Ready(void *pvState)
{
    PVNETSTATE pThis = (PVNETSTATE)pvState;
    Log(("%s Driver became ready, waking up RX thread...\n", INSTANCE(pThis)));
#ifdef IN_RING3
    vnetWakeupReceive(pThis->VPCI.CTX_SUFF(pDevIns));
#else
    PPDMQUEUEITEMCORE pItem = PDMQueueAlloc(pThis->CTX_SUFF(pCanRxQueue));
    if (pItem)
        PDMQueueInsert(pThis->CTX_SUFF(pCanRxQueue), pItem);
#endif
}


/**
 * I/O port callbacks.
 */
static const VPCIIOCALLBACKS g_IOCallbacks =
{
     vnetIoCb_GetHostFeatures,
     vnetIoCb_GetHostMinimalFeatures,
     vnetIoCb_SetHostFeatures,
     vnetIoCb_GetConfig,
     vnetIoCb_SetConfig,
     vnetIoCb_Reset,
     vnetIoCb_Ready,
};


/**
 * @callback_method_impl{FNIOMIOPORTIN}
 */
PDMBOTHCBDECL(int) vnetIOPortIn(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT port, uint32_t *pu32, unsigned cb)
{
    return vpciIOPortIn(pDevIns, pvUser, port, pu32, cb, &g_IOCallbacks);
}


/**
 * @callback_method_impl{FNIOMIOPORTOUT}
 */
PDMBOTHCBDECL(int) vnetIOPortOut(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT port, uint32_t u32, unsigned cb)
{
    return vpciIOPortOut(pDevIns, pvUser, port, u32, cb, &g_IOCallbacks);
}


#ifdef IN_RING3

/**
 * Check if the device can receive data now.
 * This must be called before the pfnRecieve() method is called.
 *
 * @remarks As a side effect this function enables queue notification
 *          if it cannot receive because the queue is empty.
 *          It disables notification if it can receive.
 *
 * @returns VERR_NET_NO_BUFFER_SPACE if it cannot.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @thread  RX
 */
static int vnetCanReceive(PVNETSTATE pThis)
{
    int rc = vnetCsRxEnter(pThis, VERR_SEM_BUSY);
    AssertRCReturn(rc, rc);

    LogFlow(("%s vnetCanReceive\n", INSTANCE(pThis)));
    if (!(pThis->VPCI.uStatus & VPCI_STATUS_DRV_OK))
        rc = VERR_NET_NO_BUFFER_SPACE;
    else if (!vqueueIsReady(&pThis->VPCI, pThis->pRxQueue))
        rc = VERR_NET_NO_BUFFER_SPACE;
    else if (vqueueIsEmpty(&pThis->VPCI, pThis->pRxQueue))
    {
        vringSetNotification(&pThis->VPCI, &pThis->pRxQueue->VRing, true);
        rc = VERR_NET_NO_BUFFER_SPACE;
    }
    else
    {
        vringSetNotification(&pThis->VPCI, &pThis->pRxQueue->VRing, false);
        rc = VINF_SUCCESS;
    }

    LogFlow(("%s vnetCanReceive -> %Rrc\n", INSTANCE(pThis), rc));
    vnetCsRxLeave(pThis);
    return rc;
}

/**
 * @interface_method_impl{PDMINETWORKDOWN,pfnWaitReceiveAvail}
 */
static DECLCALLBACK(int) vnetNetworkDown_WaitReceiveAvail(PPDMINETWORKDOWN pInterface, RTMSINTERVAL cMillies)
{
    PVNETSTATE pThis = RT_FROM_MEMBER(pInterface, VNETSTATE, INetworkDown);
    LogFlow(("%s vnetNetworkDown_WaitReceiveAvail(cMillies=%u)\n", INSTANCE(pThis), cMillies));
    int rc = vnetCanReceive(pThis);

    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;
    if (RT_UNLIKELY(cMillies == 0))
        return VERR_NET_NO_BUFFER_SPACE;

    rc = VERR_INTERRUPTED;
    ASMAtomicXchgBool(&pThis->fMaybeOutOfSpace, true);
    STAM_PROFILE_START(&pThis->StatRxOverflow, a);

    VMSTATE enmVMState;
    while (RT_LIKELY(   (enmVMState = PDMDevHlpVMState(pThis->VPCI.CTX_SUFF(pDevIns))) == VMSTATE_RUNNING
                     ||  enmVMState == VMSTATE_RUNNING_LS))
    {
        int rc2 = vnetCanReceive(pThis);
        if (RT_SUCCESS(rc2))
        {
            rc = VINF_SUCCESS;
            break;
        }
        Log(("%s vnetNetworkDown_WaitReceiveAvail: waiting cMillies=%u...\n", INSTANCE(pThis), cMillies));
        RTSemEventWait(pThis->hEventMoreRxDescAvail, cMillies);
    }
    STAM_PROFILE_STOP(&pThis->StatRxOverflow, a);
    ASMAtomicXchgBool(&pThis->fMaybeOutOfSpace, false);

    LogFlow(("%s vnetNetworkDown_WaitReceiveAvail -> %d\n", INSTANCE(pThis), rc));
    return rc;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) vnetQueryInterface(struct PDMIBASE *pInterface, const char *pszIID)
{
    PVNETSTATE pThis = RT_FROM_MEMBER(pInterface, VNETSTATE, VPCI.IBase);
    Assert(&pThis->VPCI.IBase == pInterface);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMINETWORKDOWN, &pThis->INetworkDown);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMINETWORKCONFIG, &pThis->INetworkConfig);
    return vpciQueryInterface(pInterface, pszIID);
}

/**
 * Returns true if it is a broadcast packet.
 *
 * @returns true if destination address indicates broadcast.
 * @param   pvBuf           The ethernet packet.
 */
DECLINLINE(bool) vnetIsBroadcast(const void *pvBuf)
{
    static const uint8_t s_abBcastAddr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    return memcmp(pvBuf, s_abBcastAddr, sizeof(s_abBcastAddr)) == 0;
}

/**
 * Returns true if it is a multicast packet.
 *
 * @remarks returns true for broadcast packets as well.
 * @returns true if destination address indicates multicast.
 * @param   pvBuf           The ethernet packet.
 */
DECLINLINE(bool) vnetIsMulticast(const void *pvBuf)
{
    return (*(char*)pvBuf) & 1;
}

/**
 * Determines if the packet is to be delivered to upper layer.
 *
 * @returns true if packet is intended for this node.
 * @param   pThis          Pointer to the state structure.
 * @param   pvBuf           The ethernet packet.
 * @param   cb              Number of bytes available in the packet.
 */
static bool vnetAddressFilter(PVNETSTATE pThis, const void *pvBuf, size_t cb)
{
    if (pThis->fPromiscuous)
        return true;

    /* Ignore everything outside of our VLANs */
    uint16_t *u16Ptr = (uint16_t*)pvBuf;
    /* Compare TPID with VLAN Ether Type */
    if (   u16Ptr[6] == RT_H2BE_U16(0x8100)
        && !ASMBitTest(pThis->aVlanFilter, RT_BE2H_U16(u16Ptr[7]) & 0xFFF))
    {
        Log4(("%s vnetAddressFilter: not our VLAN, returning false\n", INSTANCE(pThis)));
        return false;
    }

    if (vnetIsBroadcast(pvBuf))
        return true;

    if (pThis->fAllMulti && vnetIsMulticast(pvBuf))
        return true;

    if (!memcmp(pThis->config.mac.au8, pvBuf, sizeof(RTMAC)))
        return true;
    Log4(("%s vnetAddressFilter: %RTmac (conf) != %RTmac (dest)\n", INSTANCE(pThis), pThis->config.mac.au8, pvBuf));

    for (unsigned i = 0; i < pThis->nMacFilterEntries; i++)
        if (!memcmp(&pThis->aMacFilter[i], pvBuf, sizeof(RTMAC)))
            return true;

    Log2(("%s vnetAddressFilter: failed all tests, returning false, packet dump follows:\n", INSTANCE(pThis)));
    vnetPacketDump(pThis, (const uint8_t *)pvBuf, cb, "<-- Incoming");

    return false;
}

/**
 * Pad and store received packet.
 *
 * @remarks Make sure that the packet appears to upper layer as one coming
 *          from real Ethernet: pad it and insert FCS.
 *
 * @returns VBox status code.
 * @param   pThis          The device state structure.
 * @param   pvBuf           The available data.
 * @param   cb              Number of bytes available in the buffer.
 * @thread  RX
 */
static int vnetHandleRxPacket(PVNETSTATE pThis, const void *pvBuf, size_t cb,
                              PCPDMNETWORKGSO pGso)
{
    VNETHDRMRX   Hdr;
    unsigned    uHdrLen;
    RTGCPHYS     addrHdrMrx = 0;

    if (pGso)
    {
        Log2(("%s vnetHandleRxPacket: gso type=%x cbHdrsTotal=%u cbHdrsSeg=%u mss=%u off1=0x%x off2=0x%x\n",
              INSTANCE(pThis), pGso->u8Type, pGso->cbHdrsTotal, pGso->cbHdrsSeg, pGso->cbMaxSeg, pGso->offHdr1, pGso->offHdr2));
        Hdr.Hdr.u8Flags = VNETHDR_F_NEEDS_CSUM;
        switch (pGso->u8Type)
        {
            case PDMNETWORKGSOTYPE_IPV4_TCP:
                Hdr.Hdr.u8GSOType = VNETHDR_GSO_TCPV4;
                Hdr.Hdr.u16CSumOffset = RT_OFFSETOF(RTNETTCP, th_sum);
                break;
            case PDMNETWORKGSOTYPE_IPV6_TCP:
                Hdr.Hdr.u8GSOType = VNETHDR_GSO_TCPV6;
                Hdr.Hdr.u16CSumOffset = RT_OFFSETOF(RTNETTCP, th_sum);
                break;
            case PDMNETWORKGSOTYPE_IPV4_UDP:
                Hdr.Hdr.u8GSOType = VNETHDR_GSO_UDP;
                Hdr.Hdr.u16CSumOffset = RT_OFFSETOF(RTNETUDP, uh_sum);
                break;
            default:
                return VERR_INVALID_PARAMETER;
        }
        Hdr.Hdr.u16HdrLen = pGso->cbHdrsTotal;
        Hdr.Hdr.u16GSOSize = pGso->cbMaxSeg;
        Hdr.Hdr.u16CSumStart = pGso->offHdr2;
        STAM_REL_COUNTER_INC(&pThis->StatReceiveGSO);
    }
    else
    {
        Hdr.Hdr.u8Flags   = 0;
        Hdr.Hdr.u8GSOType = VNETHDR_GSO_NONE;
    }

    if (vnetMergeableRxBuffers(pThis))
        uHdrLen = sizeof(VNETHDRMRX);
    else
        uHdrLen = sizeof(VNETHDR);

    vnetPacketDump(pThis, (const uint8_t *)pvBuf, cb, "<-- Incoming");

    unsigned int uOffset = 0;
    unsigned int nElem;
    for (nElem = 0; uOffset < cb; nElem++)
    {
        VQUEUEELEM elem;
        unsigned int nSeg = 0, uElemSize = 0, cbReserved = 0;

        if (!vqueueGet(&pThis->VPCI, pThis->pRxQueue, &elem))
        {
            /*
             * @todo: It is possible to run out of RX buffers if only a few
             * were added and we received a big packet.
             */
            Log(("%s vnetHandleRxPacket: Suddenly there is no space in receive queue!\n", INSTANCE(pThis)));
            return VERR_INTERNAL_ERROR;
        }

        if (elem.nIn < 1)
        {
            Log(("%s vnetHandleRxPacket: No writable descriptors in receive queue!\n", INSTANCE(pThis)));
            return VERR_INTERNAL_ERROR;
        }

        if (nElem == 0)
        {
            if (vnetMergeableRxBuffers(pThis))
            {
                addrHdrMrx = elem.aSegsIn[nSeg].addr;
                cbReserved = uHdrLen;
            }
            else
            {
                /* The very first segment of the very first element gets the header. */
                if (elem.aSegsIn[nSeg].cb != sizeof(VNETHDR))
                {
                    Log(("%s vnetHandleRxPacket: The first descriptor does match the header size!\n", INSTANCE(pThis)));
                    return VERR_INTERNAL_ERROR;
                }
                elem.aSegsIn[nSeg++].pv = &Hdr;
            }
            uElemSize += uHdrLen;
        }
        while (nSeg < elem.nIn && uOffset < cb)
        {
            unsigned int uSize = (unsigned int)RT_MIN(elem.aSegsIn[nSeg].cb - (nSeg?0:cbReserved),
                                                      cb - uOffset);
            elem.aSegsIn[nSeg++].pv = (uint8_t*)pvBuf + uOffset;
            uOffset += uSize;
            uElemSize += uSize;
        }
        STAM_PROFILE_START(&pThis->StatReceiveStore, a);
        vqueuePut(&pThis->VPCI, pThis->pRxQueue, &elem, uElemSize, cbReserved);
        STAM_PROFILE_STOP(&pThis->StatReceiveStore, a);
        if (!vnetMergeableRxBuffers(pThis))
            break;
        cbReserved = 0;
    }
    if (vnetMergeableRxBuffers(pThis))
    {
        Hdr.u16NumBufs = nElem;
        int rc = PDMDevHlpPCIPhysWrite(pThis->VPCI.CTX_SUFF(pDevIns), addrHdrMrx,
                                       &Hdr, sizeof(Hdr));
        if (RT_FAILURE(rc))
        {
            Log(("%s vnetHandleRxPacket: Failed to write merged RX buf header: %Rrc\n", INSTANCE(pThis), rc));
            return rc;
        }
    }
    vqueueSync(&pThis->VPCI, pThis->pRxQueue);
    if (uOffset < cb)
    {
        Log(("%s vnetHandleRxPacket: Packet did not fit into RX queue (packet size=%u)!\n", INSTANCE(pThis), cb));
        return VERR_TOO_MUCH_DATA;
    }

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMINETWORKDOWN,pfnReceiveGso}
 */
static DECLCALLBACK(int) vnetNetworkDown_ReceiveGso(PPDMINETWORKDOWN pInterface,
                                                    const void *pvBuf, size_t cb,
                                                    PCPDMNETWORKGSO pGso)
{
    PVNETSTATE pThis = RT_FROM_MEMBER(pInterface, VNETSTATE, INetworkDown);

    if (pGso)
    {
        uint32_t uFeatures = pThis->VPCI.uGuestFeatures;

        switch (pGso->u8Type)
        {
            case PDMNETWORKGSOTYPE_IPV4_TCP:
                uFeatures &= VNET_F_GUEST_TSO4;
                break;
            case PDMNETWORKGSOTYPE_IPV6_TCP:
                uFeatures &= VNET_F_GUEST_TSO6;
                break;
            case PDMNETWORKGSOTYPE_IPV4_UDP:
            case PDMNETWORKGSOTYPE_IPV6_UDP:
                uFeatures &= VNET_F_GUEST_UFO;
                break;
            default:
                uFeatures = 0;
                break;
        }
        if (!uFeatures)
        {
            Log2(("%s vnetNetworkDown_ReceiveGso: GSO type (0x%x) not supported\n", INSTANCE(pThis), pGso->u8Type));
            return VERR_NOT_SUPPORTED;
        }
    }

    Log2(("%s vnetNetworkDown_ReceiveGso: pvBuf=%p cb=%u pGso=%p\n", INSTANCE(pThis), pvBuf, cb, pGso));
    int rc = vnetCanReceive(pThis);
    if (RT_FAILURE(rc))
        return rc;

    /* Drop packets if VM is not running or cable is disconnected. */
    VMSTATE enmVMState = PDMDevHlpVMState(pThis->VPCI.CTX_SUFF(pDevIns));
    if ((   enmVMState != VMSTATE_RUNNING
         && enmVMState != VMSTATE_RUNNING_LS)
        || !(STATUS & VNET_S_LINK_UP))
        return VINF_SUCCESS;

    STAM_PROFILE_START(&pThis->StatReceive, a);
    vpciSetReadLed(&pThis->VPCI, true);
    if (vnetAddressFilter(pThis, pvBuf, cb))
    {
        rc = vnetCsRxEnter(pThis, VERR_SEM_BUSY);
        if (RT_SUCCESS(rc))
        {
            rc = vnetHandleRxPacket(pThis, pvBuf, cb, pGso);
            STAM_REL_COUNTER_ADD(&pThis->StatReceiveBytes, cb);
            vnetCsRxLeave(pThis);
        }
    }
    vpciSetReadLed(&pThis->VPCI, false);
    STAM_PROFILE_STOP(&pThis->StatReceive, a);
    return rc;
}

/**
 * @interface_method_impl{PDMINETWORKDOWN,pfnReceive}
 */
static DECLCALLBACK(int) vnetNetworkDown_Receive(PPDMINETWORKDOWN pInterface, const void *pvBuf, size_t cb)
{
    return vnetNetworkDown_ReceiveGso(pInterface, pvBuf, cb, NULL);
}

/**
 * Gets the current Media Access Control (MAC) address.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @param   pMac            Where to store the MAC address.
 * @thread  EMT
 */
static DECLCALLBACK(int) vnetGetMac(PPDMINETWORKCONFIG pInterface, PRTMAC pMac)
{
    PVNETSTATE pThis = RT_FROM_MEMBER(pInterface, VNETSTATE, INetworkConfig);
    memcpy(pMac, pThis->config.mac.au8, sizeof(RTMAC));
    return VINF_SUCCESS;
}

/**
 * Gets the new link state.
 *
 * @returns The current link state.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @thread  EMT
 */
static DECLCALLBACK(PDMNETWORKLINKSTATE) vnetGetLinkState(PPDMINETWORKCONFIG pInterface)
{
    PVNETSTATE pThis = RT_FROM_MEMBER(pInterface, VNETSTATE, INetworkConfig);
    if (STATUS & VNET_S_LINK_UP)
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
static DECLCALLBACK(int) vnetSetLinkState(PPDMINETWORKCONFIG pInterface, PDMNETWORKLINKSTATE enmState)
{
    PVNETSTATE pThis = RT_FROM_MEMBER(pInterface, VNETSTATE, INetworkConfig);
    bool fOldUp = !!(STATUS & VNET_S_LINK_UP);
    bool fNewUp = enmState == PDMNETWORKLINKSTATE_UP;

    Log(("%s vnetSetLinkState: enmState=%d\n", INSTANCE(pThis), enmState));
    if (enmState == PDMNETWORKLINKSTATE_DOWN_RESUME)
    {
        if (fOldUp)
        {
            /*
             * We bother to bring the link down only if it was up previously. The UP link state
             * notification will be sent when the link actually goes up in vnetLinkUpTimer().
             */
            vnetTempLinkDown(pThis);
            if (pThis->pDrv)
                pThis->pDrv->pfnNotifyLinkChanged(pThis->pDrv, enmState);
        }
    }
    else if (fNewUp != fOldUp)
    {
        if (fNewUp)
        {
            Log(("%s Link is up\n", INSTANCE(pThis)));
            STATUS |= VNET_S_LINK_UP;
            vpciRaiseInterrupt(&pThis->VPCI, VERR_SEM_BUSY, VPCI_ISR_CONFIG);
        }
        else
        {
            /* The link was brought down explicitly, make sure it won't come up by timer.  */
            TMTimerStop(pThis->pLinkUpTimer);
            Log(("%s Link is down\n", INSTANCE(pThis)));
            STATUS &= ~VNET_S_LINK_UP;
            vpciRaiseInterrupt(&pThis->VPCI, VERR_SEM_BUSY, VPCI_ISR_CONFIG);
        }
        if (pThis->pDrv)
            pThis->pDrv->pfnNotifyLinkChanged(pThis->pDrv, enmState);
    }
    return VINF_SUCCESS;
}

static DECLCALLBACK(void) vnetQueueReceive(void *pvState, PVQUEUE pQueue)
{
    RT_NOREF(pQueue);
    PVNETSTATE pThis = (PVNETSTATE)pvState;
    Log(("%s Receive buffers has been added, waking up receive thread.\n", INSTANCE(pThis)));
    vnetWakeupReceive(pThis->VPCI.CTX_SUFF(pDevIns));
}

/**
 * Sets up the GSO context according to the Virtio header.
 *
 * @param   pGso                The GSO context to setup.
 * @param   pCtx                The context descriptor.
 */
DECLINLINE(PPDMNETWORKGSO) vnetSetupGsoCtx(PPDMNETWORKGSO pGso, VNETHDR const *pHdr)
{
    pGso->u8Type = PDMNETWORKGSOTYPE_INVALID;

    if (pHdr->u8GSOType & VNETHDR_GSO_ECN)
    {
        AssertMsgFailed(("Unsupported flag in virtio header: ECN\n"));
        return NULL;
    }
    switch (pHdr->u8GSOType & ~VNETHDR_GSO_ECN)
    {
        case VNETHDR_GSO_TCPV4:
            pGso->u8Type = PDMNETWORKGSOTYPE_IPV4_TCP;
            pGso->cbHdrsSeg = pHdr->u16HdrLen;
            break;
        case VNETHDR_GSO_TCPV6:
            pGso->u8Type = PDMNETWORKGSOTYPE_IPV6_TCP;
            pGso->cbHdrsSeg = pHdr->u16HdrLen;
            break;
        case VNETHDR_GSO_UDP:
            pGso->u8Type = PDMNETWORKGSOTYPE_IPV4_UDP;
            pGso->cbHdrsSeg = pHdr->u16CSumStart;
            break;
        default:
            return NULL;
    }
    if (pHdr->u8Flags & VNETHDR_F_NEEDS_CSUM)
        pGso->offHdr2  = pHdr->u16CSumStart;
    else
    {
        AssertMsgFailed(("GSO without checksum offloading!\n"));
        return NULL;
    }
    pGso->offHdr1     = sizeof(RTNETETHERHDR);
    pGso->cbHdrsTotal = pHdr->u16HdrLen;
    pGso->cbMaxSeg    = pHdr->u16GSOSize;
    return pGso;
}

DECLINLINE(uint16_t) vnetCSum16(const void *pvBuf, size_t cb)
{
    uint32_t  csum = 0;
    uint16_t *pu16 = (uint16_t *)pvBuf;

    while (cb > 1)
    {
        csum += *pu16++;
        cb -= 2;
    }
    if (cb)
        csum += *(uint8_t*)pu16;
    while (csum >> 16)
        csum = (csum >> 16) + (csum & 0xFFFF);
    return ~csum;
}

DECLINLINE(void) vnetCompleteChecksum(uint8_t *pBuf, size_t cbSize, uint16_t uStart, uint16_t uOffset)
{
    AssertReturnVoid(uStart < cbSize);
    AssertReturnVoid(uStart + uOffset + sizeof(uint16_t) <= cbSize);
    *(uint16_t*)(pBuf + uStart + uOffset) = vnetCSum16(pBuf + uStart, cbSize - uStart);
}

static bool vnetReadHeader(PVNETSTATE pThis, RTGCPHYS GCPhys, PVNETHDR pHdr, uint32_t cbMax)
{
    int rc = PDMDevHlpPhysRead(pThis->VPCI.CTX_SUFF(pDevIns), GCPhys, pHdr, sizeof(*pHdr));
    if (RT_FAILURE(rc))
        return false;

    Log4(("virtio-net: header flags=%x gso-type=%x len=%x gso-size=%x csum-start=%x csum-offset=%x cb=%x\n",
          pHdr->u8Flags, pHdr->u8GSOType, pHdr->u16HdrLen, pHdr->u16GSOSize, pHdr->u16CSumStart, pHdr->u16CSumOffset, cbMax));

    if (pHdr->u8GSOType)
    {
        uint32_t u32MinHdrSize;

        /* Segmentation offloading cannot be done without checksumming. */
        if (RT_UNLIKELY(!(pHdr->u8Flags & VNETHDR_F_NEEDS_CSUM)))
            return false;
        /* We do not support ECN. */
        if (RT_UNLIKELY(pHdr->u8GSOType & VNETHDR_GSO_ECN))
            return false;
        switch (pHdr->u8GSOType)
        {
            case VNETHDR_GSO_TCPV4:
            case VNETHDR_GSO_TCPV6:
                u32MinHdrSize = sizeof(RTNETTCP);
                break;
            case VNETHDR_GSO_UDP:
                u32MinHdrSize = 0;
                break;
            default:
                return false;
        }
        /* Header+MSS must not exceed the packet size. */
        if (RT_UNLIKELY(u32MinHdrSize + pHdr->u16CSumStart + pHdr->u16GSOSize > cbMax))
            return false;
    }
    /* Checksum must fit into the frame (validating both checksum fields). */
    if (   (pHdr->u8Flags & VNETHDR_F_NEEDS_CSUM)
        && sizeof(uint16_t) + pHdr->u16CSumStart + pHdr->u16CSumOffset > cbMax)
        return false;
    Log4(("virtio-net: return true\n"));
    return true;
}

static int vnetTransmitFrame(PVNETSTATE pThis, PPDMSCATTERGATHER pSgBuf, PPDMNETWORKGSO pGso, PVNETHDR pHdr)
{
    vnetPacketDump(pThis, (uint8_t *)pSgBuf->aSegs[0].pvSeg, pSgBuf->cbUsed, "--> Outgoing");
    if (pGso)
    {
        /* Some guests (RHEL) may report HdrLen excluding transport layer header! */
        /*
         * We cannot use cdHdrs provided by the guest because of different ways
         * it gets filled out by different versions of kernels.
         */
        //if (pGso->cbHdrs < pHdr->u16CSumStart + pHdr->u16CSumOffset + 2)
        {
            Log4(("%s vnetTransmitPendingPackets: HdrLen before adjustment %d.\n",
                  INSTANCE(pThis), pGso->cbHdrsTotal));
            switch (pGso->u8Type)
            {
                case PDMNETWORKGSOTYPE_IPV4_TCP:
                case PDMNETWORKGSOTYPE_IPV6_TCP:
                    pGso->cbHdrsTotal = pHdr->u16CSumStart +
                        ((PRTNETTCP)(((uint8_t*)pSgBuf->aSegs[0].pvSeg) + pHdr->u16CSumStart))->th_off * 4;
                    pGso->cbHdrsSeg   = pGso->cbHdrsTotal;
                    break;
                case PDMNETWORKGSOTYPE_IPV4_UDP:
                    pGso->cbHdrsTotal = (uint8_t)(pHdr->u16CSumStart + sizeof(RTNETUDP));
                    pGso->cbHdrsSeg   = pHdr->u16CSumStart;
                    break;
            }
            /* Update GSO structure embedded into the frame */
            ((PPDMNETWORKGSO)pSgBuf->pvUser)->cbHdrsTotal = pGso->cbHdrsTotal;
            ((PPDMNETWORKGSO)pSgBuf->pvUser)->cbHdrsSeg   = pGso->cbHdrsSeg;
            Log4(("%s vnetTransmitPendingPackets: adjusted HdrLen to %d.\n",
                  INSTANCE(pThis), pGso->cbHdrsTotal));
        }
        Log2(("%s vnetTransmitPendingPackets: gso type=%x cbHdrsTotal=%u cbHdrsSeg=%u mss=%u off1=0x%x off2=0x%x\n",
              INSTANCE(pThis), pGso->u8Type, pGso->cbHdrsTotal, pGso->cbHdrsSeg, pGso->cbMaxSeg, pGso->offHdr1, pGso->offHdr2));
        STAM_REL_COUNTER_INC(&pThis->StatTransmitGSO);
    }
    else if (pHdr->u8Flags & VNETHDR_F_NEEDS_CSUM)
    {
        STAM_REL_COUNTER_INC(&pThis->StatTransmitCSum);
        /*
         * This is not GSO frame but checksum offloading is requested.
         */
        vnetCompleteChecksum((uint8_t*)pSgBuf->aSegs[0].pvSeg, pSgBuf->cbUsed,
                             pHdr->u16CSumStart, pHdr->u16CSumOffset);
    }

    return pThis->pDrv->pfnSendBuf(pThis->pDrv, pSgBuf, false);
}

static void vnetTransmitPendingPackets(PVNETSTATE pThis, PVQUEUE pQueue, bool fOnWorkerThread)
{
    /*
     * Only one thread is allowed to transmit at a time, others should skip
     * transmission as the packets will be picked up by the transmitting
     * thread.
     */
    if (!ASMAtomicCmpXchgU32(&pThis->uIsTransmitting, 1, 0))
        return;

    if ((pThis->VPCI.uStatus & VPCI_STATUS_DRV_OK) == 0)
    {
        Log(("%s Ignoring transmit requests from non-existent driver (status=0x%x).\n", INSTANCE(pThis), pThis->VPCI.uStatus));
        return;
    }

    PPDMINETWORKUP pDrv = pThis->pDrv;
    if (pDrv)
    {
        int rc = pDrv->pfnBeginXmit(pDrv, fOnWorkerThread);
        Assert(rc == VINF_SUCCESS || rc == VERR_TRY_AGAIN);
        if (rc == VERR_TRY_AGAIN)
        {
            ASMAtomicWriteU32(&pThis->uIsTransmitting, 0);
            return;
        }
    }

    unsigned int uHdrLen;
    if (vnetMergeableRxBuffers(pThis))
        uHdrLen = sizeof(VNETHDRMRX);
    else
        uHdrLen = sizeof(VNETHDR);

    Log3(("%s vnetTransmitPendingPackets: About to transmit %d pending packets\n",
          INSTANCE(pThis), vringReadAvailIndex(&pThis->VPCI, &pThis->pTxQueue->VRing) - pThis->pTxQueue->uNextAvailIndex));

    vpciSetWriteLed(&pThis->VPCI, true);

    VQUEUEELEM elem;
    /*
     * Do not remove descriptors from available ring yet, try to allocate the
     * buffer first.
     */
    while (vqueuePeek(&pThis->VPCI, pQueue, &elem))
    {
        unsigned int uOffset = 0;
        if (elem.nOut < 2 || elem.aSegsOut[0].cb != uHdrLen)
        {
            Log(("%s vnetQueueTransmit: The first segment is not the header! (%u < 2 || %u != %u).\n",
                 INSTANCE(pThis), elem.nOut, elem.aSegsOut[0].cb, uHdrLen));
            break; /* For now we simply ignore the header, but it must be there anyway! */
        }
        else
        {
            VNETHDR Hdr;
            unsigned int uSize = 0;
            STAM_PROFILE_ADV_START(&pThis->StatTransmit, a);
            /* Compute total frame size. */
            for (unsigned int i = 1; i < elem.nOut && uSize < VNET_MAX_FRAME_SIZE; i++)
                uSize += elem.aSegsOut[i].cb;
            Log5(("%s vnetTransmitPendingPackets: complete frame is %u bytes.\n", INSTANCE(pThis), uSize));
            Assert(uSize <= VNET_MAX_FRAME_SIZE);
            /* Truncate oversized frames. */
            if (uSize > VNET_MAX_FRAME_SIZE)
                uSize = VNET_MAX_FRAME_SIZE;
            if (pThis->pDrv && vnetReadHeader(pThis, elem.aSegsOut[0].addr, &Hdr, uSize))
            {
                PDMNETWORKGSO Gso, *pGso;

                STAM_REL_COUNTER_INC(&pThis->StatTransmitPackets);

                STAM_PROFILE_START(&pThis->StatTransmitSend, a);

                pGso = vnetSetupGsoCtx(&Gso, &Hdr);
                /** @todo Optimize away the extra copying! (lazy bird) */
                PPDMSCATTERGATHER pSgBuf;
                int rc = pThis->pDrv->pfnAllocBuf(pThis->pDrv, uSize, pGso, &pSgBuf);
                if (RT_SUCCESS(rc))
                {
                    Assert(pSgBuf->cSegs == 1);
                    pSgBuf->cbUsed = uSize;
                    /* Assemble a complete frame. */
                    for (unsigned int i = 1; i < elem.nOut && uSize > 0; i++)
                    {
                        unsigned int cbSegment = RT_MIN(uSize, elem.aSegsOut[i].cb);
                        PDMDevHlpPhysRead(pThis->VPCI.CTX_SUFF(pDevIns), elem.aSegsOut[i].addr,
                                          ((uint8_t*)pSgBuf->aSegs[0].pvSeg) + uOffset,
                                          cbSegment);
                        uOffset += cbSegment;
                        uSize -= cbSegment;
                    }
                    rc = vnetTransmitFrame(pThis, pSgBuf, pGso, &Hdr);
                }
                else
                {
                    Log4(("virtio-net: failed to allocate SG buffer: size=%u rc=%Rrc\n", uSize, rc));
                    STAM_PROFILE_STOP(&pThis->StatTransmitSend, a);
                    STAM_PROFILE_ADV_STOP(&pThis->StatTransmit, a);
                    /* Stop trying to fetch TX descriptors until we get more bandwidth. */
                    break;
                }

                STAM_PROFILE_STOP(&pThis->StatTransmitSend, a);
                STAM_REL_COUNTER_ADD(&pThis->StatTransmitBytes, uOffset);
            }
        }
        /* Remove this descriptor chain from the available ring */
        vqueueSkip(&pThis->VPCI, pQueue);
        vqueuePut(&pThis->VPCI, pQueue, &elem, sizeof(VNETHDR) + uOffset);
        vqueueSync(&pThis->VPCI, pQueue);
        STAM_PROFILE_ADV_STOP(&pThis->StatTransmit, a);
    }
    vpciSetWriteLed(&pThis->VPCI, false);

    if (pDrv)
        pDrv->pfnEndXmit(pDrv);
    ASMAtomicWriteU32(&pThis->uIsTransmitting, 0);
}

/**
 * @interface_method_impl{PDMINETWORKDOWN,pfnXmitPending}
 */
static DECLCALLBACK(void) vnetNetworkDown_XmitPending(PPDMINETWORKDOWN pInterface)
{
    PVNETSTATE pThis = RT_FROM_MEMBER(pInterface, VNETSTATE, INetworkDown);
    vnetTransmitPendingPackets(pThis, pThis->pTxQueue, false /*fOnWorkerThread*/);
}

#ifdef VNET_TX_DELAY

static DECLCALLBACK(void) vnetQueueTransmit(void *pvState, PVQUEUE pQueue)
{
    PVNETSTATE pThis = (PVNETSTATE)pvState;

    if (TMTimerIsActive(pThis->CTX_SUFF(pTxTimer)))
    {
        TMTimerStop(pThis->CTX_SUFF(pTxTimer));
        Log3(("%s vnetQueueTransmit: Got kicked with notification disabled, re-enable notification and flush TX queue\n", INSTANCE(pThis)));
        vnetTransmitPendingPackets(pThis, pQueue, false /*fOnWorkerThread*/);
        if (RT_FAILURE(vnetCsEnter(pThis, VERR_SEM_BUSY)))
            LogRel(("vnetQueueTransmit: Failed to enter critical section!/n"));
        else
        {
            vringSetNotification(&pThis->VPCI, &pThis->pTxQueue->VRing, true);
            vnetCsLeave(pThis);
        }
    }
    else
    {
        if (RT_FAILURE(vnetCsEnter(pThis, VERR_SEM_BUSY)))
            LogRel(("vnetQueueTransmit: Failed to enter critical section!/n"));
        else
        {
            vringSetNotification(&pThis->VPCI, &pThis->pTxQueue->VRing, false);
            TMTimerSetMicro(pThis->CTX_SUFF(pTxTimer), VNET_TX_DELAY);
            pThis->u64NanoTS = RTTimeNanoTS();
            vnetCsLeave(pThis);
        }
    }
}

/**
 * @callback_method_impl{FNTMTIMERDEV, Transmit Delay Timer handler.}
 */
static DECLCALLBACK(void) vnetTxTimer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF(pDevIns, pTimer);
    PVNETSTATE pThis = (PVNETSTATE)pvUser;

    uint32_t u32MicroDiff = (uint32_t)((RTTimeNanoTS() - pThis->u64NanoTS)/1000);
    if (u32MicroDiff < pThis->u32MinDiff)
        pThis->u32MinDiff = u32MicroDiff;
    if (u32MicroDiff > pThis->u32MaxDiff)
        pThis->u32MaxDiff = u32MicroDiff;
    pThis->u32AvgDiff = (pThis->u32AvgDiff * pThis->u32i + u32MicroDiff) / (pThis->u32i + 1);
    pThis->u32i++;
    Log3(("vnetTxTimer: Expired, diff %9d usec, avg %9d usec, min %9d usec, max %9d usec\n",
          u32MicroDiff, pThis->u32AvgDiff, pThis->u32MinDiff, pThis->u32MaxDiff));

//    Log3(("%s vnetTxTimer: Expired\n", INSTANCE(pThis)));
    vnetTransmitPendingPackets(pThis, pThis->pTxQueue, false /*fOnWorkerThread*/);
    if (RT_FAILURE(vnetCsEnter(pThis, VERR_SEM_BUSY)))
    {
        LogRel(("vnetTxTimer: Failed to enter critical section!/n"));
        return;
    }
    vringSetNotification(&pThis->VPCI, &pThis->pTxQueue->VRing, true);
    vnetCsLeave(pThis);
}

#else /* !VNET_TX_DELAY */

static DECLCALLBACK(void) vnetQueueTransmit(void *pvState, PVQUEUE pQueue)
{
    PVNETSTATE pThis = (PVNETSTATE)pvState;

    vnetTransmitPendingPackets(pThis, pQueue, false /*fOnWorkerThread*/);
}

#endif /* !VNET_TX_DELAY */

static uint8_t vnetControlRx(PVNETSTATE pThis, PVNETCTLHDR pCtlHdr, PVQUEUEELEM pElem)
{
    uint8_t u8Ack = VNET_OK;
    uint8_t fOn, fDrvWasPromisc = pThis->fPromiscuous | pThis->fAllMulti;
    PDMDevHlpPhysRead(pThis->VPCI.CTX_SUFF(pDevIns),
                      pElem->aSegsOut[1].addr,
                      &fOn, sizeof(fOn));
    Log(("%s vnetControlRx: uCommand=%u fOn=%u\n", INSTANCE(pThis), pCtlHdr->u8Command, fOn));
    switch (pCtlHdr->u8Command)
    {
        case VNET_CTRL_CMD_RX_MODE_PROMISC:
            pThis->fPromiscuous = !!fOn;
            break;
        case VNET_CTRL_CMD_RX_MODE_ALLMULTI:
            pThis->fAllMulti = !!fOn;
            break;
        default:
            u8Ack = VNET_ERROR;
    }
    if (fDrvWasPromisc != (pThis->fPromiscuous | pThis->fAllMulti) && pThis->pDrv)
        pThis->pDrv->pfnSetPromiscuousMode(pThis->pDrv,
            (pThis->fPromiscuous | pThis->fAllMulti));

    return u8Ack;
}

static uint8_t vnetControlMac(PVNETSTATE pThis, PVNETCTLHDR pCtlHdr, PVQUEUEELEM pElem)
{
    uint32_t nMacs = 0;

    if (pCtlHdr->u8Command != VNET_CTRL_CMD_MAC_TABLE_SET
        || pElem->nOut != 3
        || pElem->aSegsOut[1].cb < sizeof(nMacs)
        || pElem->aSegsOut[2].cb < sizeof(nMacs))
    {
        Log(("%s vnetControlMac: Segment layout is wrong (u8Command=%u nOut=%u cb1=%u cb2=%u)\n",
             INSTANCE(pThis), pCtlHdr->u8Command, pElem->nOut, pElem->aSegsOut[1].cb, pElem->aSegsOut[2].cb));
        return VNET_ERROR;
    }

    /* Load unicast addresses */
    PDMDevHlpPhysRead(pThis->VPCI.CTX_SUFF(pDevIns),
                      pElem->aSegsOut[1].addr,
                      &nMacs, sizeof(nMacs));

    if (pElem->aSegsOut[1].cb < nMacs * sizeof(RTMAC) + sizeof(nMacs))
    {
        Log(("%s vnetControlMac: The unicast mac segment is too small (nMacs=%u cb=%u)\n",
             INSTANCE(pThis), nMacs, pElem->aSegsOut[1].cb));
        return VNET_ERROR;
    }

    if (nMacs > VNET_MAC_FILTER_LEN)
    {
        Log(("%s vnetControlMac: MAC table is too big, have to use promiscuous mode (nMacs=%u)\n", INSTANCE(pThis), nMacs));
        pThis->fPromiscuous = true;
    }
    else
    {
        if (nMacs)
            PDMDevHlpPhysRead(pThis->VPCI.CTX_SUFF(pDevIns),
                              pElem->aSegsOut[1].addr + sizeof(nMacs),
                              pThis->aMacFilter, nMacs * sizeof(RTMAC));
        pThis->nMacFilterEntries = nMacs;
#ifdef DEBUG
        Log(("%s vnetControlMac: unicast macs:\n", INSTANCE(pThis)));
        for(unsigned i = 0; i < nMacs; i++)
            Log(("         %RTmac\n", &pThis->aMacFilter[i]));
#endif /* DEBUG */
    }

    /* Load multicast addresses */
    PDMDevHlpPhysRead(pThis->VPCI.CTX_SUFF(pDevIns),
                      pElem->aSegsOut[2].addr,
                      &nMacs, sizeof(nMacs));

    if (pElem->aSegsOut[2].cb < nMacs * sizeof(RTMAC) + sizeof(nMacs))
    {
        Log(("%s vnetControlMac: The multicast mac segment is too small (nMacs=%u cb=%u)\n",
             INSTANCE(pThis), nMacs, pElem->aSegsOut[2].cb));
        return VNET_ERROR;
    }

    if (nMacs > VNET_MAC_FILTER_LEN - pThis->nMacFilterEntries)
    {
        Log(("%s vnetControlMac: MAC table is too big, have to use allmulti mode (nMacs=%u)\n", INSTANCE(pThis), nMacs));
        pThis->fAllMulti = true;
    }
    else
    {
        if (nMacs)
            PDMDevHlpPhysRead(pThis->VPCI.CTX_SUFF(pDevIns),
                              pElem->aSegsOut[2].addr + sizeof(nMacs),
                              &pThis->aMacFilter[pThis->nMacFilterEntries],
                              nMacs * sizeof(RTMAC));
#ifdef DEBUG
        Log(("%s vnetControlMac: multicast macs:\n", INSTANCE(pThis)));
        for(unsigned i = 0; i < nMacs; i++)
            Log(("         %RTmac\n",
                 &pThis->aMacFilter[i+pThis->nMacFilterEntries]));
#endif /* DEBUG */
        pThis->nMacFilterEntries += nMacs;
    }

    return VNET_OK;
}

static uint8_t vnetControlVlan(PVNETSTATE pThis, PVNETCTLHDR pCtlHdr, PVQUEUEELEM pElem)
{
    uint8_t  u8Ack = VNET_OK;
    uint16_t u16Vid;

    if (pElem->nOut != 2 || pElem->aSegsOut[1].cb != sizeof(u16Vid))
    {
        Log(("%s vnetControlVlan: Segment layout is wrong (u8Command=%u nOut=%u cb=%u)\n",
             INSTANCE(pThis), pCtlHdr->u8Command, pElem->nOut, pElem->aSegsOut[1].cb));
        return VNET_ERROR;
    }

    PDMDevHlpPhysRead(pThis->VPCI.CTX_SUFF(pDevIns),
                      pElem->aSegsOut[1].addr,
                      &u16Vid, sizeof(u16Vid));

    if (u16Vid >= VNET_MAX_VID)
    {
        Log(("%s vnetControlVlan: VLAN ID is out of range (VID=%u)\n", INSTANCE(pThis), u16Vid));
        return VNET_ERROR;
    }

    Log(("%s vnetControlVlan: uCommand=%u VID=%u\n", INSTANCE(pThis), pCtlHdr->u8Command, u16Vid));

    switch (pCtlHdr->u8Command)
    {
        case VNET_CTRL_CMD_VLAN_ADD:
            ASMBitSet(pThis->aVlanFilter, u16Vid);
            break;
        case VNET_CTRL_CMD_VLAN_DEL:
            ASMBitClear(pThis->aVlanFilter, u16Vid);
            break;
        default:
            u8Ack = VNET_ERROR;
    }

    return u8Ack;
}


static DECLCALLBACK(void) vnetQueueControl(void *pvState, PVQUEUE pQueue)
{
    PVNETSTATE pThis = (PVNETSTATE)pvState;
    uint8_t u8Ack;
    VQUEUEELEM elem;
    while (vqueueGet(&pThis->VPCI, pQueue, &elem))
    {
        if (elem.nOut < 1 || elem.aSegsOut[0].cb < sizeof(VNETCTLHDR))
        {
            Log(("%s vnetQueueControl: The first 'out' segment is not the header! (%u < 1 || %u < %u).\n",
                 INSTANCE(pThis), elem.nOut, elem.aSegsOut[0].cb,sizeof(VNETCTLHDR)));
            break; /* Skip the element and hope the next one is good. */
        }
        else if (   elem.nIn < 1
                 || elem.aSegsIn[elem.nIn - 1].cb < sizeof(VNETCTLACK))
        {
            Log(("%s vnetQueueControl: The last 'in' segment is too small to hold the acknowledge! (%u < 1 || %u < %u).\n",
                 INSTANCE(pThis), elem.nIn, elem.aSegsIn[elem.nIn - 1].cb, sizeof(VNETCTLACK)));
            break; /* Skip the element and hope the next one is good. */
        }
        else
        {
            VNETCTLHDR CtlHdr;
            PDMDevHlpPhysRead(pThis->VPCI.CTX_SUFF(pDevIns),
                              elem.aSegsOut[0].addr,
                              &CtlHdr, sizeof(CtlHdr));
            switch (CtlHdr.u8Class)
            {
                case VNET_CTRL_CLS_RX_MODE:
                    u8Ack = vnetControlRx(pThis, &CtlHdr, &elem);
                    break;
                case VNET_CTRL_CLS_MAC:
                    u8Ack = vnetControlMac(pThis, &CtlHdr, &elem);
                    break;
                case VNET_CTRL_CLS_VLAN:
                    u8Ack = vnetControlVlan(pThis, &CtlHdr, &elem);
                    break;
                default:
                    u8Ack = VNET_ERROR;
            }
            Log(("%s Processed control message %u, ack=%u.\n", INSTANCE(pThis), CtlHdr.u8Class, u8Ack));
            PDMDevHlpPCIPhysWrite(pThis->VPCI.CTX_SUFF(pDevIns),
                                  elem.aSegsIn[elem.nIn - 1].addr,
                                  &u8Ack, sizeof(u8Ack));
        }
        vqueuePut(&pThis->VPCI, pQueue, &elem, sizeof(u8Ack));
        vqueueSync(&pThis->VPCI, pQueue);
    }
}


/* -=-=-=-=- Saved state -=-=-=-=- */

/**
 * Saves the configuration.
 *
 * @param   pThis      The VNET state.
 * @param   pSSM        The handle to the saved state.
 */
static void vnetSaveConfig(PVNETSTATE pThis, PSSMHANDLE pSSM)
{
    SSMR3PutMem(pSSM, &pThis->macConfigured, sizeof(pThis->macConfigured));
}


/**
 * @callback_method_impl{FNSSMDEVLIVEEXEC}
 */
static DECLCALLBACK(int) vnetLiveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uPass)
{
    RT_NOREF(uPass);
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);
    vnetSaveConfig(pThis, pSSM);
    return VINF_SSM_DONT_CALL_AGAIN;
}


/**
 * @callback_method_impl{FNSSMDEVSAVEPREP}
 */
static DECLCALLBACK(int) vnetSavePrep(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    RT_NOREF(pSSM);
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);

    int rc = vnetCsRxEnter(pThis, VERR_SEM_BUSY);
    if (RT_UNLIKELY(rc != VINF_SUCCESS))
        return rc;
    vnetCsRxLeave(pThis);
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) vnetSaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);

    /* Save config first */
    vnetSaveConfig(pThis, pSSM);

    /* Save the common part */
    int rc = vpciSaveExec(&pThis->VPCI, pSSM);
    AssertRCReturn(rc, rc);
    /* Save device-specific part */
    rc = SSMR3PutMem( pSSM, pThis->config.mac.au8, sizeof(pThis->config.mac));
    AssertRCReturn(rc, rc);
    rc = SSMR3PutBool(pSSM, pThis->fPromiscuous);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutBool(pSSM, pThis->fAllMulti);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutU32( pSSM, pThis->nMacFilterEntries);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutMem( pSSM, pThis->aMacFilter,
                      pThis->nMacFilterEntries * sizeof(RTMAC));
    AssertRCReturn(rc, rc);
    rc = SSMR3PutMem( pSSM, pThis->aVlanFilter, sizeof(pThis->aVlanFilter));
    AssertRCReturn(rc, rc);
    Log(("%s State has been saved\n", INSTANCE(pThis)));
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNSSMDEVLOADPREP, Serializes the receive thread, it may
 *                      be working inside the critsect. }
 */
static DECLCALLBACK(int) vnetLoadPrep(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    RT_NOREF(pSSM);
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);

    int rc = vnetCsRxEnter(pThis, VERR_SEM_BUSY);
    if (RT_UNLIKELY(rc != VINF_SUCCESS))
        return rc;
    vnetCsRxLeave(pThis);
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) vnetLoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);
    int       rc;

    /* config checks */
    RTMAC macConfigured;
    rc = SSMR3GetMem(pSSM, &macConfigured, sizeof(macConfigured));
    AssertRCReturn(rc, rc);
    if (memcmp(&macConfigured, &pThis->macConfigured, sizeof(macConfigured))
        && (uPass == 0 || !PDMDevHlpVMTeleportedAndNotFullyResumedYet(pDevIns)))
        LogRel(("%s: The mac address differs: config=%RTmac saved=%RTmac\n", INSTANCE(pThis), &pThis->macConfigured, &macConfigured));

    rc = vpciLoadExec(&pThis->VPCI, pSSM, uVersion, uPass, VNET_N_QUEUES);
    AssertRCReturn(rc, rc);

    if (uPass == SSM_PASS_FINAL)
    {
        rc = SSMR3GetMem( pSSM, pThis->config.mac.au8,
                          sizeof(pThis->config.mac));
        AssertRCReturn(rc, rc);

        if (uVersion > VIRTIO_SAVEDSTATE_VERSION_3_1_BETA1)
        {
            rc = SSMR3GetBool(pSSM, &pThis->fPromiscuous);
            AssertRCReturn(rc, rc);
            rc = SSMR3GetBool(pSSM, &pThis->fAllMulti);
            AssertRCReturn(rc, rc);
            rc = SSMR3GetU32(pSSM, &pThis->nMacFilterEntries);
            AssertRCReturn(rc, rc);
            rc = SSMR3GetMem(pSSM, pThis->aMacFilter,
                             pThis->nMacFilterEntries * sizeof(RTMAC));
            AssertRCReturn(rc, rc);
            /* Clear the rest. */
            if (pThis->nMacFilterEntries < VNET_MAC_FILTER_LEN)
                memset(&pThis->aMacFilter[pThis->nMacFilterEntries],
                       0,
                       (VNET_MAC_FILTER_LEN - pThis->nMacFilterEntries)
                       * sizeof(RTMAC));
            rc = SSMR3GetMem(pSSM, pThis->aVlanFilter,
                             sizeof(pThis->aVlanFilter));
            AssertRCReturn(rc, rc);
        }
        else
        {
            pThis->fPromiscuous = true;
            pThis->fAllMulti = false;
            pThis->nMacFilterEntries = 0;
            memset(pThis->aMacFilter, 0, VNET_MAC_FILTER_LEN * sizeof(RTMAC));
            memset(pThis->aVlanFilter, 0, sizeof(pThis->aVlanFilter));
            if (pThis->pDrv)
                pThis->pDrv->pfnSetPromiscuousMode(pThis->pDrv, true);
        }
    }

    return rc;
}


/**
 * @callback_method_impl{FNSSMDEVLOADDONE, Link status adjustments after
 *                      loading.}
 */
static DECLCALLBACK(int) vnetLoadDone(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    RT_NOREF(pSSM);
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);

    if (pThis->pDrv)
        pThis->pDrv->pfnSetPromiscuousMode(pThis->pDrv,
            (pThis->fPromiscuous | pThis->fAllMulti));
    /*
     * Indicate link down to the guest OS that all network connections have
     * been lost, unless we've been teleported here.
     */
    if (!PDMDevHlpVMTeleportedAndNotFullyResumedYet(pDevIns))
        vnetTempLinkDown(pThis);

    return VINF_SUCCESS;
}


/* -=-=-=-=- PCI Device -=-=-=-=- */

/**
 * @callback_method_impl{FNPCIIOREGIONMAP}
 */
static DECLCALLBACK(int) vnetMap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                 RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType)
{
    RT_NOREF(pPciDev, iRegion);
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);
    int       rc;

    if (enmType != PCI_ADDRESS_SPACE_IO)
    {
        /* We should never get here */
        AssertMsgFailed(("Invalid PCI address space param in map callback"));
        return VERR_INTERNAL_ERROR;
    }

    pThis->VPCI.IOPortBase = (RTIOPORT)GCPhysAddress;
    rc = PDMDevHlpIOPortRegister(pDevIns, pThis->VPCI.IOPortBase,
                                 cb, 0, vnetIOPortOut, vnetIOPortIn,
                                 NULL, NULL, "VirtioNet");
#ifdef VNET_GC_SUPPORT
    AssertRCReturn(rc, rc);
    rc = PDMDevHlpIOPortRegisterR0(pDevIns, pThis->VPCI.IOPortBase,
                                   cb, 0, "vnetIOPortOut", "vnetIOPortIn",
                                   NULL, NULL, "VirtioNet");
    AssertRCReturn(rc, rc);
    rc = PDMDevHlpIOPortRegisterRC(pDevIns, pThis->VPCI.IOPortBase,
                                   cb, 0, "vnetIOPortOut", "vnetIOPortIn",
                                   NULL, NULL, "VirtioNet");
#endif
    AssertRC(rc);
    return rc;
}


/* -=-=-=-=- PDMDEVREG -=-=-=-=- */

/**
 * @interface_method_impl{PDMDEVREG,pfnDetach}
 */
static DECLCALLBACK(void) vnetDetach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);
    Log(("%s vnetDetach:\n", INSTANCE(pThis)));

    AssertLogRelReturnVoid(iLUN == 0);

    int rc = vnetCsEnter(pThis, VERR_SEM_BUSY);
    if (RT_FAILURE(rc))
    {
        LogRel(("vnetDetach failed to enter critical section!\n"));
        return;
    }

    /*
     * Zero some important members.
     */
    pThis->pDrvBase = NULL;
    pThis->pDrv = NULL;

    vnetCsLeave(pThis);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnAttach}
 */
static DECLCALLBACK(int) vnetAttach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);
    LogFlow(("%s vnetAttach:\n",  INSTANCE(pThis)));

    AssertLogRelReturn(iLUN == 0, VERR_PDM_NO_SUCH_LUN);

    int rc = vnetCsEnter(pThis, VERR_SEM_BUSY);
    if (RT_FAILURE(rc))
    {
        LogRel(("vnetAttach failed to enter critical section!\n"));
        return rc;
    }

    /*
     * Attach the driver.
     */
    rc = PDMDevHlpDriverAttach(pDevIns, 0, &pThis->VPCI.IBase, &pThis->pDrvBase, "Network Port");
    if (RT_SUCCESS(rc))
    {
        if (rc == VINF_NAT_DNS)
        {
#ifdef RT_OS_LINUX
            PDMDevHlpVMSetRuntimeError(pDevIns, 0 /*fFlags*/, "NoDNSforNAT",
                                       N_("A Domain Name Server (DNS) for NAT networking could not be determined. Please check your /etc/resolv.conf for <tt>nameserver</tt> entries. Either add one manually (<i>man resolv.conf</i>) or ensure that your host is correctly connected to an ISP. If you ignore this warning the guest will not be able to perform nameserver lookups and it will probably observe delays if trying so"));
#else
            PDMDevHlpVMSetRuntimeError(pDevIns, 0 /*fFlags*/, "NoDNSforNAT",
                                       N_("A Domain Name Server (DNS) for NAT networking could not be determined. Ensure that your host is correctly connected to an ISP. If you ignore this warning the guest will not be able to perform nameserver lookups and it will probably observe delays if trying so"));
#endif
        }
        pThis->pDrv = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMINETWORKUP);
        AssertMsgStmt(pThis->pDrv, ("Failed to obtain the PDMINETWORKUP interface!\n"),
                      rc = VERR_PDM_MISSING_INTERFACE_BELOW);
    }
    else if (   rc == VERR_PDM_NO_ATTACHED_DRIVER
             || rc == VERR_PDM_CFG_MISSING_DRIVER_NAME)
    {
        /* This should never happen because this function is not called
         * if there is no driver to attach! */
        Log(("%s No attached driver!\n", INSTANCE(pThis)));
    }

    /*
     * Temporary set the link down if it was up so that the guest
     * will know that we have change the configuration of the
     * network card
     */
    if (RT_SUCCESS(rc))
        vnetTempLinkDown(pThis);

    vnetCsLeave(pThis);
    return rc;

}


/**
 * @interface_method_impl{PDMDEVREG,pfnSuspend}
 */
static DECLCALLBACK(void) vnetSuspend(PPDMDEVINS pDevIns)
{
    /* Poke thread waiting for buffer space. */
    vnetWakeupReceive(pDevIns);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnPowerOff}
 */
static DECLCALLBACK(void) vnetPowerOff(PPDMDEVINS pDevIns)
{
    /* Poke thread waiting for buffer space. */
    vnetWakeupReceive(pDevIns);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnRelocate}
 */
static DECLCALLBACK(void) vnetRelocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta)
{
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);
    vpciRelocate(pDevIns, offDelta);
    pThis->pCanRxQueueRC = PDMQueueRCPtr(pThis->pCanRxQueueR3);
#ifdef VNET_TX_DELAY
    pThis->pTxTimerRC    = TMTimerRCPtr(pThis->pTxTimerR3);
#endif /* VNET_TX_DELAY */
    // TBD
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) vnetDestruct(PPDMDEVINS pDevIns)
{
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);

    LogRel(("TxTimer stats (avg/min/max): %7d usec %7d usec %7d usec\n",
            pThis->u32AvgDiff, pThis->u32MinDiff, pThis->u32MaxDiff));
    Log(("%s Destroying instance\n", INSTANCE(pThis)));
    if (pThis->hEventMoreRxDescAvail != NIL_RTSEMEVENT)
    {
        RTSemEventSignal(pThis->hEventMoreRxDescAvail);
        RTSemEventDestroy(pThis->hEventMoreRxDescAvail);
        pThis->hEventMoreRxDescAvail = NIL_RTSEMEVENT;
    }

    // if (PDMCritSectIsInitialized(&pThis->csRx))
    //     PDMR3CritSectDelete(&pThis->csRx);

    return vpciDestruct(&pThis->VPCI);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) vnetConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    PVNETSTATE pThis = PDMINS_2_DATA(pDevIns, PVNETSTATE);
    int        rc;
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /* Initialize the instance data suffiencently for the destructor not to blow up. */
    pThis->hEventMoreRxDescAvail = NIL_RTSEMEVENT;

    /* Do our own locking. */
    rc = PDMDevHlpSetDeviceCritSect(pDevIns, PDMDevHlpCritSectGetNop(pDevIns));
    AssertRCReturn(rc, rc);

    /* Initialize PCI part. */
    pThis->VPCI.IBase.pfnQueryInterface    = vnetQueryInterface;
    rc = vpciConstruct(pDevIns, &pThis->VPCI, iInstance,
                       VNET_NAME_FMT, VIRTIO_NET_ID,
                       VNET_PCI_CLASS, VNET_N_QUEUES);
    pThis->pRxQueue  = vpciAddQueue(&pThis->VPCI, 256, vnetQueueReceive,  "RX ");
    pThis->pTxQueue  = vpciAddQueue(&pThis->VPCI, 256, vnetQueueTransmit, "TX ");
    pThis->pCtlQueue = vpciAddQueue(&pThis->VPCI, 16,  vnetQueueControl,  "CTL");

    Log(("%s Constructing new instance\n", INSTANCE(pThis)));

    /*
     * Validate configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, "MAC\0" "CableConnected\0" "LineSpeed\0" "LinkUpDelay\0"))
                    return PDMDEV_SET_ERROR(pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                            N_("Invalid configuration for VirtioNet device"));

    /* Get config params */
    rc = CFGMR3QueryBytes(pCfg, "MAC", pThis->macConfigured.au8,
                          sizeof(pThis->macConfigured));
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get MAC address"));
    rc = CFGMR3QueryBool(pCfg, "CableConnected", &pThis->fCableConnected);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the value of 'CableConnected'"));
    rc = CFGMR3QueryU32Def(pCfg, "LinkUpDelay", (uint32_t*)&pThis->cMsLinkUpDelay, 5000); /* ms */
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the value of 'LinkUpDelay'"));
    Assert(pThis->cMsLinkUpDelay <= 300000); /* less than 5 minutes */
    if (pThis->cMsLinkUpDelay > 5000 || pThis->cMsLinkUpDelay < 100)
    {
        LogRel(("%s WARNING! Link up delay is set to %u seconds!\n",
                INSTANCE(pThis), pThis->cMsLinkUpDelay / 1000));
    }
    Log(("%s Link up delay is set to %u seconds\n",
         INSTANCE(pThis), pThis->cMsLinkUpDelay / 1000));


    vnetPrintFeatures(pThis, vnetIoCb_GetHostFeatures(pThis), "Device supports the following features");

    /* Initialize PCI config space */
    memcpy(pThis->config.mac.au8, pThis->macConfigured.au8, sizeof(pThis->config.mac.au8));
    pThis->config.uStatus = 0;

    /* Initialize state structure */
    pThis->u32PktNo     = 1;

    /* Interfaces */
    pThis->INetworkDown.pfnWaitReceiveAvail = vnetNetworkDown_WaitReceiveAvail;
    pThis->INetworkDown.pfnReceive          = vnetNetworkDown_Receive;
    pThis->INetworkDown.pfnReceiveGso       = vnetNetworkDown_ReceiveGso;
    pThis->INetworkDown.pfnXmitPending      = vnetNetworkDown_XmitPending;

    pThis->INetworkConfig.pfnGetMac         = vnetGetMac;
    pThis->INetworkConfig.pfnGetLinkState   = vnetGetLinkState;
    pThis->INetworkConfig.pfnSetLinkState   = vnetSetLinkState;

    /* Initialize critical section. */
    // char szTmp[sizeof(pThis->VPCI.szInstance) + 2];
    // RTStrPrintf(szTmp, sizeof(szTmp), "%sRX", pThis->VPCI.szInstance);
    // rc = PDMDevHlpCritSectInit(pDevIns, &pThis->csRx, szTmp);
    // if (RT_FAILURE(rc))
    //     return rc;

    /* Map our ports to IO space. */
    rc = PDMDevHlpPCIIORegionRegister(pDevIns, 0,
                                      VPCI_CONFIG + sizeof(VNetPCIConfig),
                                      PCI_ADDRESS_SPACE_IO, vnetMap);
    if (RT_FAILURE(rc))
        return rc;


    /* Register save/restore state handlers. */
    rc = PDMDevHlpSSMRegisterEx(pDevIns, VIRTIO_SAVEDSTATE_VERSION, sizeof(VNETSTATE), NULL,
                                NULL,         vnetLiveExec, NULL,
                                vnetSavePrep, vnetSaveExec, NULL,
                                vnetLoadPrep, vnetLoadExec, vnetLoadDone);
    if (RT_FAILURE(rc))
        return rc;

    /* Create the RX notifier signaller. */
    rc = PDMDevHlpQueueCreate(pDevIns, sizeof(PDMQUEUEITEMCORE), 1, 0,
                              vnetCanRxQueueConsumer, true, "VNet-Rcv", &pThis->pCanRxQueueR3);
    if (RT_FAILURE(rc))
        return rc;
    pThis->pCanRxQueueR0 = PDMQueueR0Ptr(pThis->pCanRxQueueR3);
    pThis->pCanRxQueueRC = PDMQueueRCPtr(pThis->pCanRxQueueR3);

    /* Create Link Up Timer */
    rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL, vnetLinkUpTimer, pThis,
                                TMTIMER_FLAGS_NO_CRIT_SECT,
                                "VirtioNet Link Up Timer", &pThis->pLinkUpTimer);
    if (RT_FAILURE(rc))
        return rc;

#ifdef VNET_TX_DELAY
    /* Create Transmit Delay Timer */
    rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL, vnetTxTimer, pThis,
                                TMTIMER_FLAGS_NO_CRIT_SECT,
                                "VirtioNet TX Delay Timer", &pThis->pTxTimerR3);
    if (RT_FAILURE(rc))
        return rc;
    pThis->pTxTimerR0 = TMTimerR0Ptr(pThis->pTxTimerR3);
    pThis->pTxTimerRC = TMTimerRCPtr(pThis->pTxTimerR3);

    pThis->u32i = pThis->u32AvgDiff = pThis->u32MaxDiff = 0;
    pThis->u32MinDiff = UINT32_MAX;
#endif /* VNET_TX_DELAY */

    rc = PDMDevHlpDriverAttach(pDevIns, 0, &pThis->VPCI.IBase, &pThis->pDrvBase, "Network Port");
    if (RT_SUCCESS(rc))
    {
        if (rc == VINF_NAT_DNS)
        {
            PDMDevHlpVMSetRuntimeError(pDevIns, 0 /*fFlags*/, "NoDNSforNAT",
                                       N_("A Domain Name Server (DNS) for NAT networking could not be determined. Ensure that your host is correctly connected to an ISP. If you ignore this warning the guest will not be able to perform nameserver lookups and it will probably observe delays if trying so"));
        }
        pThis->pDrv = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMINETWORKUP);
        AssertMsgReturn(pThis->pDrv, ("Failed to obtain the PDMINETWORKUP interface!\n"),
                        VERR_PDM_MISSING_INTERFACE_BELOW);
    }
    else if (   rc == VERR_PDM_NO_ATTACHED_DRIVER
             || rc == VERR_PDM_CFG_MISSING_DRIVER_NAME )
    {
         /* No error! */
        Log(("%s This adapter is not attached to any network!\n", INSTANCE(pThis)));
    }
    else
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Failed to attach the network LUN"));

    rc = RTSemEventCreate(&pThis->hEventMoreRxDescAvail);
    if (RT_FAILURE(rc))
        return rc;

    rc = vnetIoCb_Reset(pThis);
    AssertRC(rc);

    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatReceiveBytes,       STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,          "Amount of data received",            "/Public/Net/VNet%u/BytesReceived", iInstance);
    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatTransmitBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,          "Amount of data transmitted",         "/Public/Net/VNet%u/BytesTransmitted", iInstance);

    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatReceiveBytes,       STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,          "Amount of data received",            "/Devices/VNet%d/ReceiveBytes", iInstance);
    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatTransmitBytes,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,          "Amount of data transmitted",         "/Devices/VNet%d/TransmitBytes", iInstance);
    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatReceiveGSO,         STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,          "Number of received GSO packets",     "/Devices/VNet%d/Packets/ReceiveGSO", iInstance);
    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatTransmitPackets,    STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,          "Number of sent packets",             "/Devices/VNet%d/Packets/Transmit", iInstance);
    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatTransmitGSO,        STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,          "Number of sent GSO packets",         "/Devices/VNet%d/Packets/Transmit-Gso", iInstance);
    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatTransmitCSum,       STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,          "Number of completed TX checksums",   "/Devices/VNet%d/Packets/Transmit-Csum", iInstance);
#if defined(VBOX_WITH_STATISTICS)
    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatReceive,            STAMTYPE_PROFILE, STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_CALL, "Profiling receive",                  "/Devices/VNet%d/Receive/Total", iInstance);
    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatReceiveStore,       STAMTYPE_PROFILE, STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_CALL, "Profiling receive storing",          "/Devices/VNet%d/Receive/Store", iInstance);
    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatRxOverflow,         STAMTYPE_PROFILE, STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_OCCURENCE, "Profiling RX overflows",        "/Devices/VNet%d/RxOverflow", iInstance);
    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatRxOverflowWakeup,   STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,     "Nr of RX overflow wakeups",          "/Devices/VNet%d/RxOverflowWakeup", iInstance);
    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatTransmit,           STAMTYPE_PROFILE, STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_CALL, "Profiling transmits in HC",          "/Devices/VNet%d/Transmit/Total", iInstance);
    PDMDevHlpSTAMRegisterF(pDevIns, &pThis->StatTransmitSend,       STAMTYPE_PROFILE, STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_CALL, "Profiling send transmit in HC",      "/Devices/VNet%d/Transmit/Send", iInstance);
#endif /* VBOX_WITH_STATISTICS */

    return VINF_SUCCESS;
}

/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceVirtioNet =
{
    /* Structure version. PDM_DEVREG_VERSION defines the current version. */
    PDM_DEVREG_VERSION,
    /* Device name. */
    "virtio-net",
    /* Name of guest context module (no path).
     * Only evalutated if PDM_DEVREG_FLAGS_RC is set. */
    "VBoxDDRC.rc",
    /* Name of ring-0 module (no path).
     * Only evalutated if PDM_DEVREG_FLAGS_RC is set. */
    "VBoxDDR0.r0",
    /* The description of the device. The UTF-8 string pointed to shall, like this structure,
     * remain unchanged from registration till VM destruction. */
    "Virtio Ethernet.\n",

    /* Flags, combination of the PDM_DEVREG_FLAGS_* \#defines. */
#ifdef VNET_GC_SUPPORT
    PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0,
#else
    PDM_DEVREG_FLAGS_DEFAULT_BITS,
#endif
    /* Device class(es), combination of the PDM_DEVREG_CLASS_* \#defines. */
    PDM_DEVREG_CLASS_NETWORK,
    /* Maximum number of instances (per VM). */
    ~0U,
    /* Size of the instance data. */
    sizeof(VNETSTATE),

    /* pfnConstruct */
    vnetConstruct,
    /* pfnDestruct */
    vnetDestruct,
    /* pfnRelocate */
    vnetRelocate,
    /* pfnMemSetup. */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    vnetSuspend,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    vnetAttach,
    /* pfnDetach */
    vnetDetach,
    /* pfnQueryInterface */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    vnetPowerOff,
    /* pfnSoftReset */
    NULL,

    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};

#endif /* IN_RING3 */
#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */
