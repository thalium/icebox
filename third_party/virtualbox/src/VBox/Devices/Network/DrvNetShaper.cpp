/* $Id: DrvNetShaper.cpp $ */
/** @file
 * NetShaperFilter - Network shaper filter driver.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_NET_SHAPER

#include <VBox/vmm/pdmdrv.h>
#include <VBox/vmm/pdmnetifs.h>
#include <VBox/vmm/pdmnetshaper.h>

#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/string.h>
#include <iprt/uuid.h>

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Block driver instance data.
 *
 * @implements  PDMINETWORKUP
 * @implements  PDMINETWORKDOWN
 * @implements  PDMINETWORKCONFIG
 */
typedef struct DRVNETSHAPER
{
    /** Pointer to the driver instance. */
    PPDMDRVINS              pDrvInsR3;
    /** The network interface. */
    PDMINETWORKUP           INetworkUpR3;
    /** The connector that's attached to us. */
    PPDMINETWORKUP          pIBelowNetR3;

    /** Pointer to the driver instance. */
    PPDMDRVINSR0            pDrvInsR0;
    /** The network interface. */
    PDMINETWORKUPR0         INetworkUpR0;
    /** The connector that's attached to us. */
    PPDMINETWORKUPR0        pIBelowNetR0;

    /** Ring-3 base interface for the ring-0 context. */
    PDMIBASER0              IBaseR0;
    /** Ring-3 base interface for the raw-mode context. */
    PDMIBASERC              IBaseRC;

    /** For when we're the leaf driver. */
    PDMCRITSECT             XmitLock;

    /** The network interface. */
    PDMINETWORKDOWN         INetworkDown;
    /** The network config interface.
     * @todo this is a main interface and shouldn't be here...  */
    PDMINETWORKCONFIG       INetworkConfig;
    /** The port we're attached to. */
    PPDMINETWORKDOWN        pIAboveNet;
    /** The config port interface we're attached to. */
    PPDMINETWORKCONFIG      pIAboveConfig;
    /** The filter that represents us at bandwidth group. */
    PDMNSFILTER             Filter;
    /** The name of bandwidth group we are attached to. */
    char *                  pszBwGroup;

    /** TX: Total number of bytes to allocate. */
    STAMCOUNTER             StatXmitBytesRequested;
    /** TX: Number of bytes delayed. */
    STAMCOUNTER             StatXmitBytesDenied;
    /** TX: Number of bytes allowed to pass. */
    STAMCOUNTER             StatXmitBytesGranted;
    /** TX: Total number of packets being sent. */
    STAMCOUNTER             StatXmitPktsRequested;
    /** TX: Number of packets delayed. */
    STAMCOUNTER             StatXmitPktsDenied;
    /** TX: Number of packets allowed to pass. */
    STAMCOUNTER             StatXmitPktsGranted;
    /** TX: Number of calls to pfnXmitPending. */
    STAMCOUNTER             StatXmitPendingCalled;
} DRVNETSHAPER, *PDRVNETSHAPER;


/**
 * @interface_method_impl{PDMINETWORKUP,pfnBeginXmit}
 */
PDMBOTHCBDECL(int) drvNetShaperUp_BeginXmit(PPDMINETWORKUP pInterface, bool fOnWorkerThread)
{
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, CTX_SUFF(INetworkUp));
    if (RT_UNLIKELY(!pThis->CTX_SUFF(pIBelowNet)))
    {
        int rc = PDMCritSectTryEnter(&pThis->XmitLock);
        if (RT_UNLIKELY(rc == VERR_SEM_BUSY))
            rc = VERR_TRY_AGAIN;
        return rc;
    }
    return pThis->CTX_SUFF(pIBelowNet)->pfnBeginXmit(pThis->CTX_SUFF(pIBelowNet), fOnWorkerThread);
}


/**
 * @interface_method_impl{PDMINETWORKUP,pfnAllocBuf}
 */
PDMBOTHCBDECL(int) drvNetShaperUp_AllocBuf(PPDMINETWORKUP pInterface, size_t cbMin,
                                                  PCPDMNETWORKGSO pGso, PPPDMSCATTERGATHER ppSgBuf)
{
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, CTX_SUFF(INetworkUp));
    if (RT_UNLIKELY(!pThis->CTX_SUFF(pIBelowNet)))
        return VERR_NET_DOWN;
    //LogFlow(("drvNetShaperUp_AllocBuf: cb=%d\n", cbMin));
    STAM_REL_COUNTER_ADD(&pThis->StatXmitBytesRequested, cbMin);
    STAM_REL_COUNTER_INC(&pThis->StatXmitPktsRequested);
#if defined(IN_RING3) || defined(IN_RING0)
    if (!PDMNsAllocateBandwidth(&pThis->Filter, cbMin))
    {
        STAM_REL_COUNTER_ADD(&pThis->StatXmitBytesDenied, cbMin);
        STAM_REL_COUNTER_INC(&pThis->StatXmitPktsDenied);
        return VERR_TRY_AGAIN;
    }
#endif
    STAM_REL_COUNTER_ADD(&pThis->StatXmitBytesGranted, cbMin);
    STAM_REL_COUNTER_INC(&pThis->StatXmitPktsGranted);
    //LogFlow(("drvNetShaperUp_AllocBuf: got cb=%d\n", cbMin));
    return pThis->CTX_SUFF(pIBelowNet)->pfnAllocBuf(pThis->CTX_SUFF(pIBelowNet), cbMin, pGso, ppSgBuf);
}


/**
 * @interface_method_impl{PDMINETWORKUP,pfnFreeBuf}
 */
PDMBOTHCBDECL(int) drvNetShaperUp_FreeBuf(PPDMINETWORKUP pInterface, PPDMSCATTERGATHER pSgBuf)
{
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, CTX_SUFF(INetworkUp));
    if (RT_UNLIKELY(!pThis->CTX_SUFF(pIBelowNet)))
        return VERR_NET_DOWN;
    return pThis->CTX_SUFF(pIBelowNet)->pfnFreeBuf(pThis->CTX_SUFF(pIBelowNet), pSgBuf);
}


/**
 * @interface_method_impl{PDMINETWORKUP,pfnSendBuf}
 */
PDMBOTHCBDECL(int) drvNetShaperUp_SendBuf(PPDMINETWORKUP pInterface, PPDMSCATTERGATHER pSgBuf, bool fOnWorkerThread)
{
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, CTX_SUFF(INetworkUp));
    if (RT_UNLIKELY(!pThis->CTX_SUFF(pIBelowNet)))
        return VERR_NET_DOWN;

    return pThis->CTX_SUFF(pIBelowNet)->pfnSendBuf(pThis->CTX_SUFF(pIBelowNet), pSgBuf, fOnWorkerThread);
}


/**
 * @interface_method_impl{PDMINETWORKUP,pfnEndXmit}
 */
PDMBOTHCBDECL(void) drvNetShaperUp_EndXmit(PPDMINETWORKUP pInterface)
{
    //LogFlow(("drvNetShaperUp_EndXmit:\n"));
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, CTX_SUFF(INetworkUp));
    if (RT_LIKELY(pThis->CTX_SUFF(pIBelowNet)))
        pThis->CTX_SUFF(pIBelowNet)->pfnEndXmit(pThis->CTX_SUFF(pIBelowNet));
    else
        PDMCritSectLeave(&pThis->XmitLock);
}


/**
 * @interface_method_impl{PDMINETWORKUP,pfnSetPromiscuousMode}
 */
PDMBOTHCBDECL(void) drvNetShaperUp_SetPromiscuousMode(PPDMINETWORKUP pInterface, bool fPromiscuous)
{
    LogFlow(("drvNetShaperUp_SetPromiscuousMode: fPromiscuous=%d\n", fPromiscuous));
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, CTX_SUFF(INetworkUp));
    if (pThis->CTX_SUFF(pIBelowNet))
        pThis->CTX_SUFF(pIBelowNet)->pfnSetPromiscuousMode(pThis->CTX_SUFF(pIBelowNet), fPromiscuous);
}


#ifdef IN_RING3
/**
 * @interface_method_impl{PDMINETWORKUP,pfnNotifyLinkChanged}
 */
static DECLCALLBACK(void) drvR3NetShaperUp_NotifyLinkChanged(PPDMINETWORKUP pInterface, PDMNETWORKLINKSTATE enmLinkState)
{
    LogFlow(("drvNetShaperUp_NotifyLinkChanged: enmLinkState=%d\n", enmLinkState));
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, CTX_SUFF(INetworkUp));
    if (pThis->pIBelowNetR3)
        pThis->pIBelowNetR3->pfnNotifyLinkChanged(pThis->pIBelowNetR3, enmLinkState);
}

/**
 * @interface_method_impl{PDMINETWORKDOWN,pfnWaitReceiveAvail}
 */
static DECLCALLBACK(int) drvR3NetShaperDown_WaitReceiveAvail(PPDMINETWORKDOWN pInterface, RTMSINTERVAL cMillies)
{
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, INetworkDown);
    return pThis->pIAboveNet->pfnWaitReceiveAvail(pThis->pIAboveNet, cMillies);
}


/**
 * @interface_method_impl{PDMINETWORKDOWN,pfnReceive}
 */
static DECLCALLBACK(int) drvR3NetShaperDown_Receive(PPDMINETWORKDOWN pInterface, const void *pvBuf, size_t cb)
{
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, INetworkDown);
    return pThis->pIAboveNet->pfnReceive(pThis->pIAboveNet, pvBuf, cb);
}


/**
 * @interface_method_impl{PDMINETWORKDOWN,pfnReceiveGso}
 */
static DECLCALLBACK(int) drvR3NetShaperDown_ReceiveGso(PPDMINETWORKDOWN pInterface, const void *pvBuf, size_t cb, PCPDMNETWORKGSO pGso)
{
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, INetworkDown);
    if (pThis->pIAboveNet->pfnReceiveGso)
        return pThis->pIAboveNet->pfnReceiveGso(pThis->pIAboveNet, pvBuf, cb, pGso);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{PDMINETWORKDOWN,pfnXmitPending}
 */
static DECLCALLBACK(void) drvR3NetShaperDown_XmitPending(PPDMINETWORKDOWN pInterface)
{
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, INetworkDown);
    STAM_REL_COUNTER_INC(&pThis->StatXmitPendingCalled);
    pThis->pIAboveNet->pfnXmitPending(pThis->pIAboveNet);
}


/**
 * Gets the current Media Access Control (MAC) address.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @param   pMac            Where to store the MAC address.
 * @thread  EMT
 */
static DECLCALLBACK(int) drvR3NetShaperDownCfg_GetMac(PPDMINETWORKCONFIG pInterface, PRTMAC pMac)
{
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, INetworkConfig);
    return pThis->pIAboveConfig->pfnGetMac(pThis->pIAboveConfig, pMac);
}

/**
 * Gets the new link state.
 *
 * @returns The current link state.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @thread  EMT
 */
static DECLCALLBACK(PDMNETWORKLINKSTATE) drvR3NetShaperDownCfg_GetLinkState(PPDMINETWORKCONFIG pInterface)
{
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, INetworkConfig);
    return pThis->pIAboveConfig->pfnGetLinkState(pThis->pIAboveConfig);
}

/**
 * Sets the new link state.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @param   enmState        The new link state
 * @thread  EMT
 */
static DECLCALLBACK(int) drvR3NetShaperDownCfg_SetLinkState(PPDMINETWORKCONFIG pInterface, PDMNETWORKLINKSTATE enmState)
{
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, INetworkConfig);
    return pThis->pIAboveConfig->pfnSetLinkState(pThis->pIAboveConfig, enmState);
}


/**
 * @interface_method_impl{PDMIBASER0,pfnQueryInterface}
 */
static DECLCALLBACK(RTR0PTR) drvR3NetShaperIBaseR0_QueryInterface(PPDMIBASER0 pInterface, const char *pszIID)
{
    PDRVNETSHAPER pThis = RT_FROM_MEMBER(pInterface, DRVNETSHAPER, IBaseR0);
    /*
     * We need to check if the underlying driver supports R0. If it does not,
     * then it is useless and even harmful to support R0 here, as we will end up
     * returning errors when a network adapter tries to allocate a buffer in R0.
     */
    if (pThis->pIBelowNetR0)
        PDMIBASER0_RETURN_INTERFACE(pThis->pDrvInsR3, pszIID, PDMINETWORKUP, &pThis->INetworkUpR0);
    return NIL_RTR0PTR;
}

/**
 * @interface_method_impl{PDMIBASERC,pfnQueryInterface}
 */
static DECLCALLBACK(RTRCPTR) drvR3NetShaperIBaseRC_QueryInterface(PPDMIBASERC pInterface, const char *pszIID)
{
    RT_NOREF(pInterface, pszIID);
    return NIL_RTRCPTR;
}

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvR3NetShaperIBase_QueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS     pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVNETSHAPER  pThis   = PDMINS_2_DATA(pDrvIns, PDRVNETSHAPER);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASER0, &pThis->IBaseR0);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASERC, &pThis->IBaseRC);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMINETWORKUP, &pThis->INetworkUpR3);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMINETWORKDOWN, &pThis->INetworkDown);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMINETWORKCONFIG, &pThis->INetworkConfig);
    return NULL;
}


/**
 * @interface_method_impl{PDMDRVREG,pfnDetach}
 */
static DECLCALLBACK(void) drvR3NetShaperDetach(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PDRVNETSHAPER pThis = PDMINS_2_DATA(pDrvIns, PDRVNETSHAPER);

    LogFlow(("drvNetShaperDetach: pDrvIns: %p, fFlags: %u\n", pDrvIns, fFlags));
    PDMCritSectEnter(&pThis->XmitLock, VERR_IGNORED);
    pThis->pIBelowNetR3 = NULL;
    pThis->pIBelowNetR0 = NIL_RTR0PTR;
    PDMCritSectLeave(&pThis->XmitLock);
}


/**
 * @interface_method_impl{PDMDRVREG,pfnAttach}
 */
static DECLCALLBACK(int) drvR3NetShaperAttach(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    PDRVNETSHAPER pThis = PDMINS_2_DATA(pDrvIns, PDRVNETSHAPER);
    LogFlow(("drvNetShaperAttach/#%#x: fFlags=%#x\n", pDrvIns->iInstance, fFlags));
    PDMCritSectEnter(&pThis->XmitLock, VERR_IGNORED);

    /*
     * Query the network connector interface.
     */
    PPDMIBASE   pBaseDown;
    int rc = PDMDrvHlpAttach(pDrvIns, fFlags, &pBaseDown);
    if (   rc == VERR_PDM_NO_ATTACHED_DRIVER
        || rc == VERR_PDM_CFG_MISSING_DRIVER_NAME)
    {
        pThis->pIBelowNetR3 = NULL;
        pThis->pIBelowNetR0 = NIL_RTR0PTR;
        rc = VINF_SUCCESS;
    }
    else if (RT_SUCCESS(rc))
    {
        pThis->pIBelowNetR3 = PDMIBASE_QUERY_INTERFACE(pBaseDown, PDMINETWORKUP);
        if (pThis->pIBelowNetR3)
        {
            PPDMIBASER0 pBaseR0  = PDMIBASE_QUERY_INTERFACE(pBaseDown, PDMIBASER0);
            pThis->pIBelowNetR0 = pBaseR0 ? pBaseR0->pfnQueryInterface(pBaseR0, PDMINETWORKUP_IID) : NIL_RTR0PTR;
            rc = VINF_SUCCESS;
        }
        else
        {
            AssertMsgFailed(("Configuration error: the driver below didn't export the network connector interface!\n"));
            rc = VERR_PDM_MISSING_INTERFACE_BELOW;
        }
    }
    else
        AssertMsgFailed(("Failed to attach to driver below! rc=%Rrc\n", rc));

    PDMCritSectLeave(&pThis->XmitLock);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDRVREG,pfnDestruct}
 */
static DECLCALLBACK(void) drvR3NetShaperDestruct(PPDMDRVINS pDrvIns)
{
    PDRVNETSHAPER pThis = PDMINS_2_DATA(pDrvIns, PDRVNETSHAPER);
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);

    PDMDrvHlpNetShaperDetach(pDrvIns, &pThis->Filter);

    if (PDMCritSectIsInitialized(&pThis->XmitLock))
        PDMR3CritSectDelete(&pThis->XmitLock);
}


/**
 * @interface_method_impl{Construct a NAT network transport driver instance,
 *                       PDMDRVREG,pfnDestruct}
 */
static DECLCALLBACK(int) drvR3NetShaperConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    PDRVNETSHAPER pThis = PDMINS_2_DATA(pDrvIns, PDRVNETSHAPER);
    LogFlow(("drvNetShaperConstruct:\n"));
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);

    /*
     * Init the static parts.
     */
    pThis->pDrvInsR3                                = pDrvIns;
    pThis->pDrvInsR0                                = PDMDRVINS_2_R0PTR(pDrvIns);
    /* IBase */
    pDrvIns->IBase.pfnQueryInterface                = drvR3NetShaperIBase_QueryInterface;
    pThis->IBaseR0.pfnQueryInterface                = drvR3NetShaperIBaseR0_QueryInterface;
    pThis->IBaseRC.pfnQueryInterface                = drvR3NetShaperIBaseRC_QueryInterface;
    /* INetworkUp */
    pThis->INetworkUpR3.pfnBeginXmit                = drvNetShaperUp_BeginXmit;
    pThis->INetworkUpR3.pfnAllocBuf                 = drvNetShaperUp_AllocBuf;
    pThis->INetworkUpR3.pfnFreeBuf                  = drvNetShaperUp_FreeBuf;
    pThis->INetworkUpR3.pfnSendBuf                  = drvNetShaperUp_SendBuf;
    pThis->INetworkUpR3.pfnEndXmit                  = drvNetShaperUp_EndXmit;
    pThis->INetworkUpR3.pfnSetPromiscuousMode       = drvNetShaperUp_SetPromiscuousMode;
    pThis->INetworkUpR3.pfnNotifyLinkChanged        = drvR3NetShaperUp_NotifyLinkChanged;
    /* Resolve the ring-0 context interface addresses. */
    int rc = pDrvIns->pHlpR3->pfnLdrGetR0InterfaceSymbols(pDrvIns, &pThis->INetworkUpR0,
                                                          sizeof(pThis->INetworkUpR0),
                                                          "drvNetShaperUp_", PDMINETWORKUP_SYM_LIST);
    AssertLogRelRCReturn(rc, rc);
    /* INetworkDown */
    pThis->INetworkDown.pfnWaitReceiveAvail         = drvR3NetShaperDown_WaitReceiveAvail;
    pThis->INetworkDown.pfnReceive                  = drvR3NetShaperDown_Receive;
    pThis->INetworkDown.pfnReceiveGso               = drvR3NetShaperDown_ReceiveGso;
    pThis->INetworkDown.pfnXmitPending              = drvR3NetShaperDown_XmitPending;
    /* INetworkConfig */
    pThis->INetworkConfig.pfnGetMac                 = drvR3NetShaperDownCfg_GetMac;
    pThis->INetworkConfig.pfnGetLinkState           = drvR3NetShaperDownCfg_GetLinkState;
    pThis->INetworkConfig.pfnSetLinkState           = drvR3NetShaperDownCfg_SetLinkState;

    /*
     * Create the locks.
     */
    rc = PDMDrvHlpCritSectInit(pDrvIns, &pThis->XmitLock, RT_SRC_POS, "NetShaper");
    AssertRCReturn(rc, rc);

    /*
     * Validate the config.
     */
    if (!CFGMR3AreValuesValid(pCfg, "BwGroup\0"))
        return VERR_PDM_DRVINS_UNKNOWN_CFG_VALUES;

    /*
     * Find the bandwidth group we have to attach to.
     */
    rc = CFGMR3QueryStringAlloc(pCfg, "BwGroup", &pThis->pszBwGroup);
    if (RT_FAILURE(rc) && rc != VERR_CFGM_VALUE_NOT_FOUND)
    {
        rc = PDMDRV_SET_ERROR(pDrvIns, rc,
                              N_("DrvNetShaper: Configuration error: Querying \"BwGroup\" as string failed"));
        return rc;
    }
    else
        rc = VINF_SUCCESS;

    pThis->Filter.pIDrvNetR3 = &pThis->INetworkDown;
    rc = PDMDrvHlpNetShaperAttach(pDrvIns, pThis->pszBwGroup, &pThis->Filter);
    if (RT_FAILURE(rc))
    {
        rc = PDMDRV_SET_ERROR(pDrvIns, rc,
                              N_("DrvNetShaper: Configuration error: Failed to attach to bandwidth group"));
        return rc;
    }

    /*
     * Query the network port interface.
     */
    pThis->pIAboveNet = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMINETWORKDOWN);
    if (!pThis->pIAboveNet)
    {
        AssertMsgFailed(("Configuration error: the above device/driver didn't export the network port interface!\n"));
        return VERR_PDM_MISSING_INTERFACE_ABOVE;
    }

    /*
     * Query the network config interface.
     */
    pThis->pIAboveConfig = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMINETWORKCONFIG);
    if (!pThis->pIAboveConfig)
    {
        AssertMsgFailed(("Configuration error: the above device/driver didn't export the network config interface!\n"));
        return VERR_PDM_MISSING_INTERFACE_ABOVE;
    }

    /*
     * Query the network connector interface.
     */
    PPDMIBASE   pBaseDown;
    rc = PDMDrvHlpAttach(pDrvIns, fFlags, &pBaseDown);
    if (   rc == VERR_PDM_NO_ATTACHED_DRIVER
        || rc == VERR_PDM_CFG_MISSING_DRIVER_NAME)
    {
        pThis->pIBelowNetR3 = NULL;
        pThis->pIBelowNetR0 = NIL_RTR0PTR;
    }
    else if (RT_SUCCESS(rc))
    {
        pThis->pIBelowNetR3 = PDMIBASE_QUERY_INTERFACE(pBaseDown, PDMINETWORKUP);
        if (!pThis->pIBelowNetR3)
        {
            AssertMsgFailed(("Configuration error: the driver below didn't export the network connector interface!\n"));
            return VERR_PDM_MISSING_INTERFACE_BELOW;
        }
        PPDMIBASER0 pBaseR0  = PDMIBASE_QUERY_INTERFACE(pBaseDown, PDMIBASER0);
        pThis->pIBelowNetR0 = pBaseR0 ? pBaseR0->pfnQueryInterface(pBaseR0, PDMINETWORKUP_IID) : NIL_RTR0PTR;
    }
    else
    {
        AssertMsgFailed(("Failed to attach to driver below! rc=%Rrc\n", rc));
        return rc;
    }

    /*
     * Register statistics.
     */
    PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->StatXmitBytesRequested, "Bytes/Tx/Requested",   STAMUNIT_BYTES, "Number of requested TX bytes.");
    PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->StatXmitBytesDenied,    "Bytes/Tx/Denied",      STAMUNIT_BYTES, "Number of denied TX bytes.");
    PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->StatXmitBytesGranted,   "Bytes/Tx/Granted",     STAMUNIT_BYTES, "Number of granted TX bytes.");

    PDMDrvHlpSTAMRegCounter(pDrvIns, &pThis->StatXmitPktsRequested,  "Packets/Tx/Requested", "Number of requested TX packets.");
    PDMDrvHlpSTAMRegCounter(pDrvIns, &pThis->StatXmitPktsDenied,     "Packets/Tx/Denied",    "Number of denied TX packets.");
    PDMDrvHlpSTAMRegCounter(pDrvIns, &pThis->StatXmitPktsGranted,    "Packets/Tx/Granted",   "Number of granted TX packets.");
    PDMDrvHlpSTAMRegCounter(pDrvIns, &pThis->StatXmitPendingCalled,  "Tx/WakeUp",            "Number of wakeup TX calls.");

    return VINF_SUCCESS;
}



/**
 * Network sniffer filter driver registration record.
 */
const PDMDRVREG g_DrvNetShaper =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "NetShaper",
    /* szRCMod */
    "",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "Network Shaper Filter Driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT | PDM_DRVREG_FLAGS_R0,
    /* fClass. */
    PDM_DRVREG_CLASS_NETWORK,
    /* cMaxInstances */
    UINT32_MAX,
    /* cbInstance */
    sizeof(DRVNETSHAPER),
    /* pfnConstruct */
    drvR3NetShaperConstruct,
    /* pfnDestruct */
    drvR3NetShaperDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
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
    drvR3NetShaperAttach,
    /* pfnDetach */
    drvR3NetShaperDetach,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};
#endif /* IN_RING3 */
