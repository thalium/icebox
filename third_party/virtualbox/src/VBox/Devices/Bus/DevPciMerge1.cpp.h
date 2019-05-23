/* $Id: DevPciMerge1.cpp.h $ */
/** @file
 * DevPci - Early attempt at common code for DevPci and DevPciIch9.
 *
 * @note    Don't add more, add code to DevPciIch9.cpp instead.
 * @note    We'll keep this file like this for a little while longer
 *          because of 5.1.
 */

/*
 * Copyright (C) 2004-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/**
 * Search for a completely unused device entry (all 8 functions are unused).
 *
 * @returns VBox status code.
 * @param   pBus            The bus to register with.
 * @remarks Caller enters the PDM critical section.
 */
static uint8_t pciR3MergedFindUnusedDeviceNo(PDEVPCIBUS pBus)
{
    for (uint8_t uPciDevNo = pBus->iDevSearch >> VBOX_PCI_DEVFN_DEV_SHIFT; uPciDevNo < VBOX_PCI_MAX_DEVICES; uPciDevNo++)
        if (   !pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, 0)]
            && !pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, 1)]
            && !pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, 2)]
            && !pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, 3)]
            && !pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, 4)]
            && !pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, 5)]
            && !pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, 6)]
            && !pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, 7)])
            return uPciDevNo;
    return UINT8_MAX;
}



/**
 * Registers the device with the specified PCI bus.
 *
 * This is shared between the pci bus and pci bridge code.
 *
 * @returns VBox status code.
 * @param   pBus            The bus to register with.
 * @param   pPciDev         The PCI device structure.
 * @param   fFlags          Reserved for future use, PDMPCIDEVREG_F_MBZ.
 * @param   uPciDevNo       PDMPCIDEVREG_DEV_NO_FIRST_UNUSED, or a specific
 *                          device number (0-31).
 * @param   uPciFunNo       PDMPCIDEVREG_FUN_NO_FIRST_UNUSED, or a specific
 *                          function number (0-7).
 * @param   pszName         Device name (static but not unique).
 * @param   pfnConfigRead   The default config read method.
 * @param   pfnConfigWrite  The default config read method.
 *
 * @remarks Caller enters the PDM critical section.
 */
static int pciR3MergedRegisterDeviceOnBus(PDEVPCIBUS pBus, PPDMPCIDEV pPciDev, uint32_t fFlags,
                                          uint8_t uPciDevNo, uint8_t uPciFunNo, const char *pszName,
                                          PFNPCICONFIGREAD pfnConfigRead, PFNPCICONFIGWRITE pfnConfigWrite)
{
    /*
     * Validate input.
     */
    AssertPtrReturn(pszName, VERR_INVALID_POINTER);
    AssertPtrReturn(pPciDev, VERR_INVALID_POINTER);
    AssertReturn(!(fFlags & ~PDMPCIDEVREG_F_VALID_MASK), VERR_INVALID_FLAGS);
    AssertReturn(uPciDevNo < VBOX_PCI_MAX_DEVICES   || uPciDevNo == PDMPCIDEVREG_DEV_NO_FIRST_UNUSED, VERR_INVALID_PARAMETER);
    AssertReturn(uPciFunNo < VBOX_PCI_MAX_FUNCTIONS || uPciFunNo == PDMPCIDEVREG_FUN_NO_FIRST_UNUSED, VERR_INVALID_PARAMETER);

    /*
     * Assign device & function numbers.
     */

    /* Work the optional assignment flag. */
    if (fFlags & PDMPCIDEVREG_F_NOT_MANDATORY_NO)
    {
        AssertLogRelMsgReturn(uPciDevNo < VBOX_PCI_MAX_DEVICES && uPciFunNo < VBOX_PCI_MAX_FUNCTIONS,
                              ("PDMPCIDEVREG_F_NOT_MANDATORY_NO not implemented for #Dev=%#x / #Fun=%#x\n", uPciDevNo, uPciFunNo),
                              VERR_NOT_IMPLEMENTED);
        if (pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, uPciFunNo)])
        {
            uPciDevNo = PDMPCIDEVREG_DEV_NO_FIRST_UNUSED;
            uPciFunNo = PDMPCIDEVREG_FUN_NO_FIRST_UNUSED;
        }
    }

    if (uPciDevNo == PDMPCIDEVREG_DEV_NO_FIRST_UNUSED)
    {
        /* Just find the next unused device number and we're good. */
        uPciDevNo = pciR3MergedFindUnusedDeviceNo(pBus);
        AssertLogRelMsgReturn(uPciDevNo < VBOX_PCI_MAX_DEVICES, ("Couldn't find a free spot!\n"), VERR_PDM_TOO_PCI_MANY_DEVICES);
        if (uPciFunNo == PDMPCIDEVREG_FUN_NO_FIRST_UNUSED)
            uPciFunNo = 0;
    }
    else
    {
        /*
         * Direct assignment of device number can be more complicated.
         */
        PPDMPCIDEV pClash;
        if (uPciFunNo != PDMPCIDEVREG_FUN_NO_FIRST_UNUSED)
        {
            /* In the case of a specified function, we only relocate an existing
               device if it belongs to a different device instance.  Reasoning is
               that the device should figure out it's own function assignments.
               Note! We could make this more flexible by relocating functions assigned
                     via PDMPCIDEVREG_FUN_NO_FIRST_UNUSED, but it can wait till it's needed. */
            pClash = pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, uPciFunNo)];
            if (!pClash)
            { /* likely */ }
            else if (pClash->Int.s.pDevInsR3 == pPciDev->Int.s.pDevInsR3)
                AssertLogRelMsgFailedReturn(("PCI Configuration conflict at %u.%u: %s vs %s (same pDevIns)!\n",
                                             uPciDevNo, uPciFunNo, pClash->pszNameR3, pszName),
                                            VERR_PDM_TOO_PCI_MANY_DEVICES);
            else if (!pClash->Int.s.fReassignableDevNo)
                AssertLogRelMsgFailedReturn(("PCI Configuration conflict at %u.%u: %s vs %s (different pDevIns)!\n",
                                             uPciDevNo, uPciFunNo, pClash->pszNameR3, pszName),
                                            VERR_PDM_TOO_PCI_MANY_DEVICES);
        }
        else
        {
            /* First unused function slot.  Again, we only relocate the whole
               thing if all the device instance differs, because we otherwise
               reason that a device should manage its own functions correctly. */
            unsigned cSameDevInses = 0;
            for (uPciFunNo = 0, pClash = NULL; uPciFunNo < VBOX_PCI_MAX_FUNCTIONS; uPciFunNo++)
            {
                pClash = pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, uPciFunNo)];
                if (!pClash)
                    break;
                cSameDevInses += pClash->Int.s.pDevInsR3 == pPciDev->Int.s.pDevInsR3;
            }
            if (!pClash)
                Assert(uPciFunNo < VBOX_PCI_MAX_FUNCTIONS);
            else
                AssertLogRelMsgReturn(cSameDevInses == 0,
                                      ("PCI Configuration conflict at %u.* appending %s (%u of %u pDevIns matches)!\n",
                                       uPciDevNo, pszName, cSameDevInses, VBOX_PCI_MAX_FUNCTIONS),
                                      VERR_PDM_TOO_PCI_MANY_DEVICES);
        }
        if (pClash)
        {
            /*
             * Try relocate the existing device.
             */
            /* Check that all functions can be moved. */
            for (uint8_t uMoveFun = 0; uMoveFun < VBOX_PCI_MAX_FUNCTIONS; uMoveFun++)
            {
                PPDMPCIDEV pMovePciDev = pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, uMoveFun)];
                AssertLogRelMsgReturn(!pMovePciDev || pMovePciDev->Int.s.fReassignableDevNo,
                                      ("PCI Configuration conflict at %u.%u: %s vs %s\n",
                                       uPciDevNo, uMoveFun, pMovePciDev->pszNameR3, pszName),
                                      VERR_PDM_TOO_PCI_MANY_DEVICES);
            }

            /* Find a free device number to move it to. */
            uint8_t uMoveToDevNo = pciR3MergedFindUnusedDeviceNo(pBus);
            Assert(uMoveToDevNo != uPciFunNo);
            AssertLogRelMsgReturn(uMoveToDevNo < VBOX_PCI_MAX_DEVICES,
                                  ("No space to relocate device at %u so '%s' can be placed there instead!\n", uPciFunNo, pszName),
                                  VERR_PDM_TOO_PCI_MANY_DEVICES);

            /* Execute the move. */
            for (uint8_t uMoveFun = 0; uMoveFun < VBOX_PCI_MAX_FUNCTIONS; uMoveFun++)
            {
                PPDMPCIDEV pMovePciDev = pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, uMoveFun)];
                if (pMovePciDev)
                {
                    Log(("PCI: Relocating '%s' from %u.%u to %u.%u.\n", pMovePciDev->pszNameR3, uPciDevNo, uMoveFun, uMoveToDevNo, uMoveFun));
                    pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, uMoveFun)] = NULL;
                    pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uMoveToDevNo, uMoveFun)] = pMovePciDev;
                    pMovePciDev->uDevFn = VBOX_PCI_DEVFN_MAKE(uMoveToDevNo, uMoveFun);
                }
            }
        }
    }

    /*
     * Now, initialize the rest of the PCI device structure.
     */
    Assert(!pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, uPciFunNo)]);
    pBus->apDevices[VBOX_PCI_DEVFN_MAKE(uPciDevNo, uPciFunNo)] = pPciDev;

    pPciDev->uDevFn                 = VBOX_PCI_DEVFN_MAKE(uPciDevNo, uPciFunNo);
    pPciDev->Int.s.pBusR3           = pBus;
    pPciDev->Int.s.pBusR0           = MMHyperR3ToR0(PDMDevHlpGetVM(pBus->CTX_SUFF(pDevIns)), pBus);
    pPciDev->Int.s.pBusRC           = MMHyperR3ToRC(PDMDevHlpGetVM(pBus->CTX_SUFF(pDevIns)), pBus);
    pPciDev->Int.s.pfnConfigRead    = pfnConfigRead;
    pPciDev->Int.s.pfnConfigWrite   = pfnConfigWrite;

    /* Remember and mark bridges. */
    if (fFlags & PDMPCIDEVREG_F_PCI_BRIDGE)
    {
        AssertLogRelMsgReturn(pBus->cBridges < RT_ELEMENTS(pBus->apDevices),
                              ("Number of bridges exceeds the number of possible devices on the bus\n"),
                              VERR_INTERNAL_ERROR_3);
        pBus->papBridgesR3[pBus->cBridges++] = pPciDev;
        pciDevSetPci2PciBridge(pPciDev);
    }

    Log(("PCI: Registered device %d function %d (%#x) '%s'.\n",
         uPciDevNo, uPciFunNo, UINT32_C(0x80000000) | (pPciDev->uDevFn << 8), pszName));

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMPCIBUSREG,pfnRegisterR3}
 */
static DECLCALLBACK(int) pciR3MergedRegister(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t fFlags,
                                             uint8_t uPciDevNo, uint8_t uPciFunNo, const char *pszName)
{
    PDEVPCIBUS pBus = PDMINS_2_DATA(pDevIns, PDEVPCIBUS);
    AssertCompileMemberOffset(DEVPCIROOT, PciBus, 0);
    return pciR3MergedRegisterDeviceOnBus(pBus, pPciDev, fFlags, uPciDevNo, uPciFunNo, pszName,
                                          devpciR3CommonDefaultConfigRead, devpciR3CommonDefaultConfigWrite);
}


/**
 * @interface_method_impl{PDMPCIBUSREG,pfnRegisterR3}
 */
static DECLCALLBACK(int) pcibridgeR3MergedRegisterDevice(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t fFlags,
                                                         uint8_t uPciDevNo, uint8_t uPciFunNo, const char *pszName)
{
    PDEVPCIBUS pBus = PDMINS_2_DATA(pDevIns, PDEVPCIBUS);
    return pciR3MergedRegisterDeviceOnBus(pBus, pPciDev, fFlags, uPciDevNo, uPciFunNo, pszName,
                                          devpciR3CommonDefaultConfigRead, devpciR3CommonDefaultConfigWrite);
}

