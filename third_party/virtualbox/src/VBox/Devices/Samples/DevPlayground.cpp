/* $Id: DevPlayground.cpp $ */
/** @file
 * DevPlayground - Device for making PDM/PCI/... experiments.
 *
 * This device uses big PCI BAR64 resources, which needs the ICH9 chipset.
 * The device works without any PCI config (because the default setup with the
 * ICH9 chipset doesn't have anything at bus=0, device=0, function=0.
 *
 * To enable this device for a particular VM:
 * VBoxManage setextradata vmname VBoxInternal/PDM/Devices/playground/Path .../obj/VBoxPlaygroundDevice/VBoxPlaygroundDevice
 * VBoxManage setextradata vmname VBoxInternal/Devices/playground/0/Config/Whatever1 0
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

#include <VBox/com/assert.h>
#include <VBox/com/defs.h>
#include <VBox/com/string.h>
#include <VBox/com/Guid.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/assert.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Playground device per function (sub-device) data.
 */
typedef struct VBOXPLAYGROUNDDEVICEFUNCTION
{
    /** The PCI devices. */
    PDMPCIDEV   PciDev;
    /** The function number. */
    uint8_t     iFun;
    /** Device function name. */
    char        szName[31];
} VBOXPLAYGROUNDDEVICEFUNCTION;
/** Pointer to a PCI function of the playground device. */
typedef VBOXPLAYGROUNDDEVICEFUNCTION *PVBOXPLAYGROUNDDEVICEFUNCTION;

/**
 * Playground device instance data.
 */
typedef struct VBOXPLAYGROUNDDEVICE
{
    /** PCI device functions. */
    VBOXPLAYGROUNDDEVICEFUNCTION aPciFuns[8];
} VBOXPLAYGROUNDDEVICE;
/** Pointer to the instance data of a playground device instance. */
typedef VBOXPLAYGROUNDDEVICE *PVBOXPLAYGROUNDDEVICE;


#define PLAYGROUND_SSM_VERSION 3


/*********************************************************************************************************************************
*   Device Functions                                                                                                             *
*********************************************************************************************************************************/

PDMBOTHCBDECL(int) devPlaygroundMMIORead(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb)
{
    NOREF(pDevIns);
    NOREF(pvUser);
    NOREF(GCPhysAddr);
    NOREF(pv);
    NOREF(cb);
    return VINF_SUCCESS;
}


PDMBOTHCBDECL(int) devPlaygroundMMIOWrite(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void const *pv, unsigned cb)
{
    NOREF(pDevIns);
    NOREF(pvUser);
    NOREF(GCPhysAddr);
    NOREF(pv);
    NOREF(cb);
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNPCIIOREGIONMAP}
 */
static DECLCALLBACK(int) devPlaygroundMap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                          RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType)
{
    RT_NOREF(pPciDev, enmType, cb);

    switch (iRegion)
    {
        case 0:
        case 2:
            Assert(   enmType == (PCIADDRESSSPACE)(PCI_ADDRESS_SPACE_MEM | PCI_ADDRESS_SPACE_BAR64)
                   || enmType == (PCIADDRESSSPACE)(PCI_ADDRESS_SPACE_MEM_PREFETCH | PCI_ADDRESS_SPACE_BAR64));
            if (GCPhysAddress == NIL_RTGCPHYS)
                return VINF_SUCCESS; /* We ignore the unmap notification. */
            return PDMDevHlpMMIOExMap(pDevIns, pPciDev, iRegion, GCPhysAddress);

        default:
            /* We should never get here */
            AssertMsgFailedReturn(("Invalid PCI region param in map callback"), VERR_INTERNAL_ERROR);
    }
}


/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) devPlaygroundSaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PVBOXPLAYGROUNDDEVICE pThis = PDMINS_2_DATA(pDevIns, PVBOXPLAYGROUNDDEVICE);

    /* dummy (real devices would need to save their state here) */
    RT_NOREF(pThis);

    /* Demo of some API stuff - very unusual, think twice if there's no better
     * solution which doesn't need API interaction. */
    HRESULT hrc = S_OK;
    com::Bstr bstrSnapName;
    com::Guid uuid(COM_IIDOF(ISnapshot));
    ISnapshot *pSnap = (ISnapshot *)PDMDevHlpQueryGenericUserObject(pDevIns, uuid.raw());
    if (pSnap)
    {
        hrc = pSnap->COMGETTER(Name)(bstrSnapName.asOutParam());
        AssertComRCReturn(hrc, VERR_INVALID_STATE);
    }
    com::Utf8Str strSnapName(bstrSnapName);
    SSMR3PutStrZ(pSSM, strSnapName.c_str());
    LogRel(("Playground: saving state of snapshot '%s', hrc=%Rhrc\n", strSnapName.c_str(), hrc));

    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) devPlaygroundLoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PVBOXPLAYGROUNDDEVICE pThis = PDMINS_2_DATA(pDevIns, PVBOXPLAYGROUNDDEVICE);

    if (uVersion > PLAYGROUND_SSM_VERSION)
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    /* dummy (real devices would need to load their state here) */
    RT_NOREF(pThis);

    /* Reading the stuff written to saved state, just a demo. */
    char szSnapName[256];
    int rc = SSMR3GetStrZ(pSSM, szSnapName, sizeof(szSnapName));
    AssertRCReturn(rc, rc);
    LogRel(("Playground: loading state of snapshot '%s'\n", szSnapName));

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) devPlaygroundConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    RT_NOREF(iInstance, pCfg);
    int rc = VINF_SUCCESS;

    /*
     * Check that the device instance and device helper structures are compatible.
     */
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Initialize the instance data so that the destructor won't mess up.
     */
    PVBOXPLAYGROUNDDEVICE pThis = PDMINS_2_DATA(pDevIns, PVBOXPLAYGROUNDDEVICE);

    /*
     * Validate and read the configuration.
     */
    PDMDEV_VALIDATE_CONFIG_RETURN(pDevIns, "Whatever1|Whatever2", "");

    /*
     * PCI device setup.
     */
    uint32_t iPciDevNo = PDMPCIDEVREG_DEV_NO_FIRST_UNUSED;
    for (uint32_t iPciFun = 0; iPciFun < RT_ELEMENTS(pThis->aPciFuns); iPciFun++)
    {
        PVBOXPLAYGROUNDDEVICEFUNCTION pFun = &pThis->aPciFuns[iPciFun];
        RTStrPrintf(pFun->szName, sizeof(pThis->aPciFuns[iPciFun].PciDev), "playground%u", iPciFun);
        pFun->iFun = iPciFun;

        PCIDevSetVendorId( &pFun->PciDev, 0x80ee);
        PCIDevSetDeviceId( &pFun->PciDev, 0xde4e);
        PCIDevSetClassBase(&pFun->PciDev, 0x07);   /* communications device */
        PCIDevSetClassSub( &pFun->PciDev, 0x80);   /* other communications device */
        if (iPciFun == 0)       /* only for the primary function */
            PCIDevSetHeaderType(&pFun->PciDev, 0x80); /* normal, multifunction device */

        rc = PDMDevHlpPCIRegisterEx(pDevIns, &pFun->PciDev, iPciFun, 0 /*fFlags*/, iPciDevNo, iPciFun,
                                        pThis->aPciFuns[iPciFun].szName);
        AssertLogRelRCReturn(rc, rc);

        /* First region. */
        RTGCPHYS const cbFirst = iPciFun == 0 ? 8*_1M : iPciFun * _4K;
        rc = PDMDevHlpPCIIORegionRegisterEx(pDevIns, &pFun->PciDev, 0, cbFirst,
                                            (PCIADDRESSSPACE)(  PCI_ADDRESS_SPACE_MEM | PCI_ADDRESS_SPACE_BAR64
                                                              | (iPciFun == 0 ? PCI_ADDRESS_SPACE_MEM_PREFETCH : 0)),
                                            devPlaygroundMap);
        AssertLogRelRCReturn(rc, rc);
        char *pszRegionName = NULL;
        RTStrAPrintf(&pszRegionName, "PG-F%d-BAR0", iPciFun);
        Assert(pszRegionName);
        rc = PDMDevHlpMMIOExPreRegister(pDevIns, &pFun->PciDev, 0, cbFirst,
                                        IOMMMIO_FLAGS_READ_PASSTHRU | IOMMMIO_FLAGS_WRITE_PASSTHRU, pszRegionName,
                                        NULL /*pvUser*/,  devPlaygroundMMIOWrite, devPlaygroundMMIORead, NULL /*pfnFill*/,
                                        NIL_RTR0PTR /*pvUserR0*/, NULL /*pszWriteR0*/, NULL /*pszReadR0*/, NULL /*pszFillR0*/,
                                        NIL_RTRCPTR /*pvUserRC*/, NULL /*pszWriteRC*/, NULL /*pszReadRC*/, NULL /*pszFillRC*/);
        AssertLogRelRCReturn(rc, rc);

        /* Second region. */
        RTGCPHYS const cbSecond = iPciFun == 0  ? 32*_1M : iPciFun * _32K;
        rc = PDMDevHlpPCIIORegionRegisterEx(pDevIns, &pFun->PciDev, 2, cbSecond,
                                            (PCIADDRESSSPACE)(  PCI_ADDRESS_SPACE_MEM | PCI_ADDRESS_SPACE_BAR64
                                                              | (iPciFun == 0 ? PCI_ADDRESS_SPACE_MEM_PREFETCH : 0)),
                                            devPlaygroundMap);
        AssertLogRelRCReturn(rc, rc);
        pszRegionName = NULL;
        RTStrAPrintf(&pszRegionName, "PG-F%d-BAR2", iPciFun);
        Assert(pszRegionName);
        rc = PDMDevHlpMMIOExPreRegister(pDevIns, &pFun->PciDev, 2, cbSecond,
                                        IOMMMIO_FLAGS_READ_PASSTHRU | IOMMMIO_FLAGS_WRITE_PASSTHRU, pszRegionName,
                                        NULL /*pvUser*/,  devPlaygroundMMIOWrite, devPlaygroundMMIORead, NULL /*pfnFill*/,
                                        NIL_RTR0PTR /*pvUserR0*/, NULL /*pszWriteR0*/, NULL /*pszReadR0*/, NULL /*pszFillR0*/,
                                        NIL_RTRCPTR /*pvUserRC*/, NULL /*pszWriteRC*/, NULL /*pszReadRC*/, NULL /*pszFillRC*/);
        AssertLogRelRCReturn(rc, rc);

        /* Subsequent function should use the same major as the previous one. */
        iPciDevNo = PDMPCIDEVREG_DEV_NO_SAME_AS_PREV;
    }

    /*
     * Save state handling.
     */
    rc = PDMDevHlpSSMRegister(pDevIns, PLAYGROUND_SSM_VERSION, sizeof(*pThis), devPlaygroundSaveExec, devPlaygroundLoadExec);
    if (RT_FAILURE(rc))
        return rc;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) devPlaygroundDestruct(PPDMDEVINS pDevIns)
{
    /*
     * Check the versions here as well since the destructor is *always* called.
     */
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);

    return VINF_SUCCESS;
}


/**
 * The device registration structure.
 */
static const PDMDEVREG g_DevicePlayground =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "playground",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "VBox Playground Device.",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS,
    /* fClass */
    PDM_DEVREG_CLASS_MISC,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(VBOXPLAYGROUNDDEVICE),
    /* pfnConstruct */
    devPlaygroundConstruct,
    /* pfnDestruct */
    devPlaygroundDestruct,
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
    LogFlow(("VBoxPlaygroundDevice::VBoxDevicesRegister: u32Version=%#x pCallbacks->u32Version=%#x\n", u32Version, pCallbacks->u32Version));

    AssertLogRelMsgReturn(u32Version >= VBOX_VERSION,
                          ("VirtualBox version %#x, expected %#x or higher\n", u32Version, VBOX_VERSION),
                          VERR_VERSION_MISMATCH);
    AssertLogRelMsgReturn(pCallbacks->u32Version == PDM_DEVREG_CB_VERSION,
                          ("callback version %#x, expected %#x\n", pCallbacks->u32Version, PDM_DEVREG_CB_VERSION),
                          VERR_VERSION_MISMATCH);

    return pCallbacks->pfnRegister(pCallbacks, &g_DevicePlayground);
}

