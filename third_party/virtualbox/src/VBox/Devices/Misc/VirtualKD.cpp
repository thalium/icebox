/* $Id: VirtualKD.cpp $ */
/** @file
 * VirtualKD - Device stub/loader for fast Windows kernel-mode debugging.
 *
 * Contributed by: Ivan Shcherbakov
 * Heavily modified after the contribution.
 */

/*
 * Copyright (C) 2010-2016 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DEV // LOG_GROUP_DEV_VIRTUALKD
#include <VBox/vmm/pdmdev.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/path.h>

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

#define IKDClient_InterfaceVersion 3


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

typedef struct VKDREQUESTHDR
{
    unsigned cbData;
    unsigned cbReplyMax;
} VKDREQUESTHDR;

#pragma pack(1)
typedef struct VKDREPLYHDR
{
    unsigned cbData;
    char chOne;
    char chSpace;
} VKDREPLYHDR;
#pragma pack()
AssertCompileSize(VKDREPLYHDR, 6);

class IKDClient
{
public:
    virtual unsigned OnRequest(const char *pRequestIncludingRpcHeader, unsigned RequestSizeWithRpcHeader, char **ppReply)=0;
    virtual ~IKDClient() {}
};

typedef IKDClient *(*PFNCreateVBoxKDClientEx)(unsigned version);

typedef struct VIRTUALKD
{
    bool fOpenChannelDetected;
    bool fChannelDetectSuccessful;
    RTLDRMOD hLib;
    IKDClient *pKDClient;
    char abCmdBody[_256K];
} VIRTUALKD;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

static DECLCALLBACK(int) vkdPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    RT_NOREF(pvUser, Port, cb);
    VIRTUALKD *pThis = PDMINS_2_DATA(pDevIns, VIRTUALKD *);

    if (pThis->fOpenChannelDetected)
    {
        *pu32 = RT_MAKE_U32_FROM_U8('V', 'B', 'O', 'X');    /* 'XOBV', checked in VMWRPC.H */
        pThis->fOpenChannelDetected = false;
        pThis->fChannelDetectSuccessful = true;
    }
    else
        *pu32 = UINT32_MAX;

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) vkdPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    RT_NOREF(pvUser, cb);
    VIRTUALKD *pThis = PDMINS_2_DATA(pDevIns, VIRTUALKD *);

    if (Port == 0x5659)
    {
        VKDREQUESTHDR RequestHeader = {0, };
        int rc = PDMDevHlpPhysRead(pDevIns, (RTGCPHYS)u32, &RequestHeader, sizeof(RequestHeader));
        if (   !RT_SUCCESS(rc)
            || !RequestHeader.cbData)
            return VINF_SUCCESS;

        unsigned cbData = RT_MIN(RequestHeader.cbData, sizeof(pThis->abCmdBody));
        rc = PDMDevHlpPhysRead(pDevIns, (RTGCPHYS)(u32 + sizeof(RequestHeader)), pThis->abCmdBody, cbData);
        if (!RT_SUCCESS(rc))
            return VINF_SUCCESS;

        char *pReply = NULL;
        unsigned cbReply = pThis->pKDClient->OnRequest(pThis->abCmdBody, cbData, &pReply);

        if (!pReply)
            cbReply = 0;

        VKDREPLYHDR ReplyHeader;
        ReplyHeader.cbData = cbReply + 2;
        ReplyHeader.chOne = '1';
        ReplyHeader.chSpace = ' ';
        rc = PDMDevHlpPhysWrite(pDevIns, (RTGCPHYS)u32, &ReplyHeader, sizeof(ReplyHeader));
        if (!RT_SUCCESS(rc))
            return VINF_SUCCESS;
        if (cbReply)
        {
            rc = PDMDevHlpPhysWrite(pDevIns, (RTGCPHYS)(u32 + sizeof(ReplyHeader)), pReply, cbReply);
            if (!RT_SUCCESS(rc))
                return VINF_SUCCESS;
        }
    }
    else if (Port == 0x5658)
    {
        if (u32 == 0x564D5868)
            pThis->fOpenChannelDetected = true;
        else
            pThis->fOpenChannelDetected = false;
    }

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) vkdDestruct(PPDMDEVINS pDevIns)
{
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    VIRTUALKD *pThis = PDMINS_2_DATA(pDevIns, VIRTUALKD *);

    delete pThis->pKDClient;
    if (pThis->hLib != NIL_RTLDRMOD)
        RTLdrClose(pThis->hLib);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) vkdConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    RT_NOREF(iInstance);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    VIRTUALKD *pThis = PDMINS_2_DATA(pDevIns, VIRTUALKD *);

    pThis->fOpenChannelDetected = false;
    pThis->fChannelDetectSuccessful = false;
    pThis->hLib = NIL_RTLDRMOD;
    pThis->pKDClient = NULL;

    if (!CFGMR3AreValuesValid(pCfg,
                              "Path\0"))
        return VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES;

    /* This device is a bit unusual, after this point it will not fail to be
     * constructed, but there will be a warning and it will not work. */

    char szPath[RTPATH_MAX] = "";
    CFGMR3QueryString(pCfg, "Path", szPath, sizeof(szPath));

    RTPathAppend(szPath, sizeof(szPath), HC_ARCH_BITS == 64 ?  "kdclient64.dll" : "kdclient.dll");
    int rc = RTLdrLoad(szPath, &pThis->hLib);
    if (RT_FAILURE(rc))
    {
        PDMDevHlpVMSetRuntimeError(pDevIns, 0 /* fFlags */, "VirtualKD_LOAD",
                                   N_("Failed to load VirtualKD library '%s'. Fast kernel-mode debugging will not work"), szPath);
        return VINF_SUCCESS;
    }

    PFNCreateVBoxKDClientEx pfnInit;
    rc = RTLdrGetSymbol(pThis->hLib, "CreateVBoxKDClientEx", (void **)&pfnInit);
    if (RT_FAILURE(rc))
    {
        RTLdrClose(pThis->hLib);
        pThis->hLib = NIL_RTLDRMOD;
        PDMDevHlpVMSetRuntimeError(pDevIns, 0 /* fFlags */, "VirtualKD_SYMBOL",
                                   N_("Failed to find entry point for VirtualKD library '%s'. Fast kernel-mode debugging will not work"), szPath);
        return VINF_SUCCESS;
    }

    pThis->pKDClient = pfnInit(IKDClient_InterfaceVersion);
    if (!pThis->pKDClient)
    {
        RTLdrClose(pThis->hLib);
        pThis->hLib = NIL_RTLDRMOD;
        PDMDevHlpVMSetRuntimeError(pDevIns, 0 /* fFlags */, "VirtualKD_INIT",
                                   N_("Failed to initialize VirtualKD library '%s'. Fast kernel-mode debugging will not work"), szPath);
        return VINF_SUCCESS;
    }

    PDMDevHlpIOPortRegister(pDevIns, 0x5658, 2, NULL, vkdPortWrite, vkdPortRead, NULL, NULL, "VirtualKD");

    return VINF_SUCCESS;
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceVirtualKD =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "VirtualKD",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Provides fast debugging interface when debugging Windows kernel",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS,
    /* fClass */
    PDM_DEVREG_CLASS_MISC,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(VIRTUALKD),
    /* pfnConstruct */
    vkdConstruct,
    /* pfnDestruct */
    vkdDestruct,
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

