/* $Id: VBoxGuest-win-legacy.cpp $ */
/** @file
 * VBoxGuest-win-legacy - Windows NT4 specifics.
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
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "VBoxGuest-win.h"
#include "VBoxGuestInternal.h"
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/version.h>
#include <VBox/VBoxGuestLib.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#ifndef PCI_MAX_BUSES
# define PCI_MAX_BUSES 256
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
static NTSTATUS vgdrvNt4FindPciDevice(PULONG pulBusNumber, PPCI_SLOT_NUMBER pSlotNumber);
RT_C_DECLS_END

#ifdef ALLOC_PRAGMA
# pragma alloc_text(INIT, vgdrvNt4CreateDevice)
# pragma alloc_text(INIT, vgdrvNt4FindPciDevice)
#endif


/**
 * Legacy helper function to create the device object.
 *
 * @returns NT status code.
 *
 * @param   pDrvObj         The driver object.
 * @param   pRegPath        The driver registry path.
 */
NTSTATUS vgdrvNt4CreateDevice(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegPath)
{
    Log(("vgdrvNt4CreateDevice: pDrvObj=%p, pRegPath=%p\n", pDrvObj, pRegPath));

    /*
     * Find our virtual PCI device
     */
    ULONG uBusNumber;
    PCI_SLOT_NUMBER SlotNumber;
    NTSTATUS rc = vgdrvNt4FindPciDevice(&uBusNumber, &SlotNumber);
    if (NT_ERROR(rc))
    {
        Log(("vgdrvNt4CreateDevice: Device not found!\n"));
        return rc;
    }

    /*
     * Create device.
     */
    UNICODE_STRING szDevName;
    RtlInitUnicodeString(&szDevName, VBOXGUEST_DEVICE_NAME_NT);
    PDEVICE_OBJECT pDeviceObject = NULL;
    rc = IoCreateDevice(pDrvObj, sizeof(VBOXGUESTDEVEXTWIN), &szDevName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDeviceObject);
    if (NT_SUCCESS(rc))
    {
        Log(("vgdrvNt4CreateDevice: Device created\n"));

        UNICODE_STRING DosName;
        RtlInitUnicodeString(&DosName, VBOXGUEST_DEVICE_NAME_DOS);
        rc = IoCreateSymbolicLink(&DosName, &szDevName);
        if (NT_SUCCESS(rc))
        {
            Log(("vgdrvNt4CreateDevice: Symlink created\n"));

            /*
             * Setup the device extension.
             */
            Log(("vgdrvNt4CreateDevice: Setting up device extension ...\n"));

            PVBOXGUESTDEVEXTWIN pDevExt = (PVBOXGUESTDEVEXTWIN)pDeviceObject->DeviceExtension;
            RT_ZERO(*pDevExt);

            Log(("vgdrvNt4CreateDevice: Device extension created\n"));

            /* Store a reference to ourself. */
            pDevExt->pDeviceObject = pDeviceObject;

            /* Store bus and slot number we've queried before. */
            pDevExt->busNumber  = uBusNumber;
            pDevExt->slotNumber = SlotNumber.u.AsULONG;

#ifdef VBOX_WITH_GUEST_BUGCHECK_DETECTION
            rc = hlpRegisterBugCheckCallback(pDevExt);
#endif

            /* Do the actual VBox init ... */
            if (NT_SUCCESS(rc))
            {
                rc = vgdrvNtInit(pDrvObj, pDeviceObject, pRegPath);
                if (NT_SUCCESS(rc))
                {
                    Log(("vgdrvNt4CreateDevice: Returning rc = 0x%x (succcess)\n", rc));
                    return rc;
                }

                /* bail out */
            }
            IoDeleteSymbolicLink(&DosName);
        }
        else
            Log(("vgdrvNt4CreateDevice: IoCreateSymbolicLink failed with rc = %#x\n", rc));
        IoDeleteDevice(pDeviceObject);
    }
    else
        Log(("vgdrvNt4CreateDevice: IoCreateDevice failed with rc = %#x\n", rc));
    Log(("vgdrvNt4CreateDevice: Returning rc = 0x%x\n", rc));
    return rc;
}


/**
 * Helper function to handle the PCI device lookup.
 *
 * @returns NT status code.
 *
 * @param   pulBusNumber    Where to return the bus number on success.
 * @param   pSlotNumber     Where to return the slot number on success.
 */
static NTSTATUS vgdrvNt4FindPciDevice(PULONG pulBusNumber, PPCI_SLOT_NUMBER pSlotNumber)
{
    Log(("vgdrvNt4FindPciDevice\n"));

    PCI_SLOT_NUMBER SlotNumber;
    SlotNumber.u.AsULONG = 0;

    /* Scan each bus. */
    for (ULONG ulBusNumber = 0; ulBusNumber < PCI_MAX_BUSES; ulBusNumber++)
    {
        /* Scan each device. */
        for (ULONG deviceNumber = 0; deviceNumber < PCI_MAX_DEVICES; deviceNumber++)
        {
            SlotNumber.u.bits.DeviceNumber = deviceNumber;

            /* Scan each function (not really required...). */
            for (ULONG functionNumber = 0; functionNumber < PCI_MAX_FUNCTION; functionNumber++)
            {
                SlotNumber.u.bits.FunctionNumber = functionNumber;

                /* Have a look at what's in this slot. */
                PCI_COMMON_CONFIG PciData;
                if (!HalGetBusData(PCIConfiguration, ulBusNumber, SlotNumber.u.AsULONG, &PciData, sizeof(ULONG)))
                {
                    /* No such bus, we're done with it. */
                    deviceNumber = PCI_MAX_DEVICES;
                    break;
                }

                if (PciData.VendorID == PCI_INVALID_VENDORID)
                    /* We have to proceed to the next function. */
                    continue;

                /* Check if it's another device. */
                if (   PciData.VendorID != VMMDEV_VENDORID
                    || PciData.DeviceID != VMMDEV_DEVICEID)
                    continue;

                /* Hooray, we've found it! */
                Log(("vgdrvNt4FindPciDevice: Device found!\n"));

                *pulBusNumber = ulBusNumber;
                *pSlotNumber  = SlotNumber;
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_DEVICE_DOES_NOT_EXIST;
}

