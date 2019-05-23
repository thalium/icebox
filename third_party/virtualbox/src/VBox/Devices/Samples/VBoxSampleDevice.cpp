/* $Id: VBoxSampleDevice.cpp $ */
/** @file
 * VBox Sample Device.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_MISC
#include <VBox/vmm/pdmdev.h>
#include <VBox/version.h>
#include <VBox/err.h>
#include <VBox/log.h>

#include <iprt/assert.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Device Instance Data.
 */
typedef struct VBOXSAMPLEDEVICE
{
    uint32_t    Whatever;
} VBOXSAMPLEDEVICE;
typedef VBOXSAMPLEDEVICE *PVBOXSAMPLEDEVICE;



static DECLCALLBACK(int) devSampleDestruct(PPDMDEVINS pDevIns)
{
    /*
     * Check the versions here as well since the destructor is *always* called.
     */
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);

    return VINF_SUCCESS;
}


static DECLCALLBACK(int) devSampleConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{

    /*
     * Check that the device instance and device helper structures are compatible.
     */
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    NOREF(pCfg);

    /*
     * Initialize the instance data so that the destructor won't mess up.
     */
    PVBOXSAMPLEDEVICE pThis = PDMINS_2_DATA(pDevIns, PVBOXSAMPLEDEVICE);
    pThis->Whatever = 0;

    /*
     * Validate and read the configuration.
     */
    PDMDEV_VALIDATE_CONFIG_RETURN(pDevIns, "Whatever1|Whatever2", "");

    /*
     * Use the instance number if necessary (not for this device, which in
     * g_DeviceSample below declares a maximum instance count of 1).
     */
    NOREF(iInstance);

    return VINF_SUCCESS;
}


/**
 * The device registration structure.
 */
static const PDMDEVREG g_DeviceSample =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "sample",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "VBox Sample Device.",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS,
    /* fClass */
    PDM_DEVREG_CLASS_MISC,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(VBOXSAMPLEDEVICE),
    /* pfnConstruct */
    devSampleConstruct,
    /* pfnDestruct */
    devSampleDestruct,
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


/**
 * Register devices provided by the plugin.
 *
 * @returns VBox status code.
 * @param   pCallbacks      Pointer to the callback table.
 * @param   u32Version      VBox version number.
 */
extern "C" DECLEXPORT(int) VBoxDevicesRegister(PPDMDEVREGCB pCallbacks, uint32_t u32Version)
{
    LogFlow(("VBoxSampleDevice::VBoxDevicesRegister: u32Version=%#x pCallbacks->u32Version=%#x\n", u32Version, pCallbacks->u32Version));

    AssertLogRelMsgReturn(u32Version >= VBOX_VERSION,
                          ("VirtualBox version %#x, expected %#x or higher\n", u32Version, VBOX_VERSION),
                          VERR_VERSION_MISMATCH);
    AssertLogRelMsgReturn(pCallbacks->u32Version == PDM_DEVREG_CB_VERSION,
                          ("callback version %#x, expected %#x\n", pCallbacks->u32Version, PDM_DEVREG_CB_VERSION),
                          VERR_VERSION_MISMATCH);

    return pCallbacks->pfnRegister(pCallbacks, &g_DeviceSample);
}

