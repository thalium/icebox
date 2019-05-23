/* $Id: PCIRawDevImpl.cpp $ */
/** @file
 * VirtualBox Driver Interface to raw PCI device.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_GROUP LOG_GROUP_DEV_PCI_RAW
#include "LoggingNew.h"

#include "PCIRawDevImpl.h"
#include "PCIDeviceAttachmentImpl.h"
#include "ConsoleImpl.h"

// generated header for events
#include "VBoxEvents.h"

/**
 * PCI raw driver instance data.
 */
typedef struct DRVMAINPCIRAWDEV
{
    /** Pointer to the real PCI raw object. */
    PCIRawDev                   *pPCIRawDev;
    /** Pointer to the driver instance structure. */
    PPDMDRVINS                  pDrvIns;
    /** Our PCI device connector interface. */
    PDMIPCIRAWCONNECTOR         IConnector;

} DRVMAINPCIRAWDEV, *PDRVMAINPCIRAWDEV;

//
// constructor / destructor
//
PCIRawDev::PCIRawDev(Console *console)
  : mpDrv(NULL),
    mParent(console)
{
}

PCIRawDev::~PCIRawDev()
{
}

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
DECLCALLBACK(void *) PCIRawDev::drvQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS         pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVMAINPCIRAWDEV  pThis   = PDMINS_2_DATA(pDrvIns, PDRVMAINPCIRAWDEV);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE,            &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIPCIRAWCONNECTOR, &pThis->IConnector);

    return NULL;
}


/**
 * @interface_method_impl{PDMIPCIRAWCONNECTOR,pfnDeviceConstructComplete}
 */
DECLCALLBACK(int) PCIRawDev::drvDeviceConstructComplete(PPDMIPCIRAWCONNECTOR pInterface, const char *pcszName,
                                                        uint32_t uHostPCIAddress, uint32_t uGuestPCIAddress,
                                                        int rc)
{
    PDRVMAINPCIRAWDEV pThis = RT_FROM_CPP_MEMBER(pInterface, DRVMAINPCIRAWDEV, IConnector);
    Console *pConsole = pThis->pPCIRawDev->getParent();
    const ComPtr<IMachine>& machine = pConsole->i_machine();
    ComPtr<IVirtualBox> vbox;

    HRESULT hrc = machine->COMGETTER(Parent)(vbox.asOutParam());
    Assert(SUCCEEDED(hrc)); NOREF(hrc);

    ComPtr<IEventSource> es;
    hrc = vbox->COMGETTER(EventSource)(es.asOutParam());
    Assert(SUCCEEDED(hrc));

    Bstr bstrId;
    hrc = machine->COMGETTER(Id)(bstrId.asOutParam());
    Assert(SUCCEEDED(hrc));

    ComObjPtr<PCIDeviceAttachment> pda;
    BstrFmt bstrName(pcszName);
    pda.createObject();
    pda->init(machine, bstrName, uHostPCIAddress, uGuestPCIAddress, TRUE);

    Bstr msg("");
    if (RT_FAILURE(rc))
        msg = BstrFmt("runtime error %Rrc", rc);

    fireHostPCIDevicePlugEvent(es, bstrId.raw(), true /* plugged */, RT_SUCCESS_NP(rc) /* success */, pda, msg.raw());

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDRVREG,pfnReset}
 */
DECLCALLBACK(void) PCIRawDev::drvDestruct(PPDMDRVINS pDrvIns)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);
    PDRVMAINPCIRAWDEV pThis = PDMINS_2_DATA(pDrvIns, PDRVMAINPCIRAWDEV);

    if (pThis->pPCIRawDev)
        pThis->pPCIRawDev->mpDrv = NULL;
}


/**
 * @interface_method_impl{PDMDRVREG,pfnConstruct}
 */
DECLCALLBACK(int) PCIRawDev::drvConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfgHandle, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVMAINPCIRAWDEV pThis = PDMINS_2_DATA(pDrvIns, PDRVMAINPCIRAWDEV);

    /*
     * Validate configuration.
     */
    if (!CFGMR3AreValuesValid(pCfgHandle, "Object\0"))
        return VERR_PDM_DRVINS_UNKNOWN_CFG_VALUES;

    AssertMsgReturn(PDMDrvHlpNoAttach(pDrvIns) == VERR_PDM_NO_ATTACHED_DRIVER,
                    ("Configuration error: Not possible to attach anything to this driver!\n"),
                    VERR_PDM_DRVINS_NO_ATTACH);

    /*
     * IBase.
     */
    pDrvIns->IBase.pfnQueryInterface = PCIRawDev::drvQueryInterface;

    /*
     * IConnector.
     */
    pThis->IConnector.pfnDeviceConstructComplete = PCIRawDev::drvDeviceConstructComplete;

    /*
     * Get the object pointer and update the mpDrv member.
     */
    void *pv;
    int rc = CFGMR3QueryPtr(pCfgHandle, "Object", &pv);
    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Configuration error: No \"Object\" value! rc=%Rrc\n", rc));
        return rc;
    }

    pThis->pPCIRawDev = (PCIRawDev *)pv;
    pThis->pPCIRawDev->mpDrv = pThis;

    return VINF_SUCCESS;
}

/**
 * Main raw PCI driver registration record.
 */
const PDMDRVREG PCIRawDev::DrvReg =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "MainPciRaw",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Main PCI raw driver (Main as in the API).",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_PCIRAW,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVMAINPCIRAWDEV),
    /* pfnConstruct */
    PCIRawDev::drvConstruct,
    /* pfnDestruct */
    PCIRawDev::drvDestruct,
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
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};

