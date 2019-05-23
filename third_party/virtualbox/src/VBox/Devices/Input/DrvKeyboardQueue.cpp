/* $Id: DrvKeyboardQueue.cpp $ */
/** @file
 * VBox input devices: Keyboard queue driver
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DRV_KBD_QUEUE
#include <VBox/vmm/pdmdrv.h>
#include <iprt/assert.h>
#include <iprt/uuid.h>

#include "VBoxDD.h"



/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/** Scancode translator state.  */
typedef enum {
    SS_IDLE,    /**< Starting state. */
    SS_EXT,     /**< E0 byte was received. */
    SS_EXT1     /**< E1 byte was received. */
} scan_state_t;

/**
 * Keyboard queue driver instance data.
 *
 * @implements  PDMIKEYBOARDCONNECTOR
 * @implements  PDMIKEYBOARDPORT
 */
typedef struct DRVKBDQUEUE
{
    /** Pointer to the driver instance structure. */
    PPDMDRVINS                  pDrvIns;
    /** Pointer to the keyboard port interface of the driver/device above us. */
    PPDMIKEYBOARDPORT           pUpPort;
    /** Pointer to the keyboard port interface of the driver/device below us. */
    PPDMIKEYBOARDCONNECTOR      pDownConnector;
    /** Our keyboard connector interface. */
    PDMIKEYBOARDCONNECTOR       IConnector;
    /** Our keyboard port interface. */
    PDMIKEYBOARDPORT            IPort;
    /** The queue handle. */
    PPDMQUEUE                   pQueue;
    /** State of the scancode translation. */
    scan_state_t                XlatState;
    /** Discard input when this flag is set. */
    bool                        fInactive;
    /** When VM is suspended, queue full errors are not fatal. */
    bool                        fSuspended;
} DRVKBDQUEUE, *PDRVKBDQUEUE;


/**
 * Keyboard queue item.
 */
typedef struct DRVKBDQUEUEITEM
{
    /** The core part owned by the queue manager. */
    PDMQUEUEITEMCORE    Core;
    /** The keycode. */
    uint32_t            u32UsageCode;
} DRVKBDQUEUEITEM, *PDRVKBDQUEUEITEM;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/

/** Lookup table for converting PC/XT scan codes to USB HID usage codes. */
static const uint8_t aScancode2Hid[] =
{
    0x00, 0x29, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, /* 00-07 */
    0x24, 0x25, 0x26, 0x27, 0x2d, 0x2e, 0x2a, 0x2b, /* 08-1F */
    0x14, 0x1a, 0x08, 0x15, 0x17, 0x1c, 0x18, 0x0c, /* 10-17 */
    0x12, 0x13, 0x2f, 0x30, 0x28, 0xe0, 0x04, 0x16, /* 18-1F */
    0x07, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f, 0x33, /* 20-27 */
    0x34, 0x35, 0xe1, 0x31, 0x1d, 0x1b, 0x06, 0x19, /* 28-2F */
    0x05, 0x11, 0x10, 0x36, 0x37, 0x38, 0xe5, 0x55, /* 30-37 */
    0xe2, 0x2c, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, /* 38-3F */
    0x3f, 0x40, 0x41, 0x42, 0x43, 0x53, 0x47, 0x5f, /* 40-47 */
    0x60, 0x61, 0x56, 0x5c, 0x5d, 0x5e, 0x57, 0x59, /* 48-4F */
    0x5a, 0x5b, 0x62, 0x63, 0x46, 0x00, 0x64, 0x44, /* 50-57 */
    0x45, 0x67, 0x00, 0x00, 0x8c, 0x00, 0x00, 0x00, /* 58-5F */
    0x00, 0x00, 0x00, 0x00, 0x68, 0x69, 0x6a, 0x6b, /* 60-67 */
    0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x00, /* 68-6F */
    0x88, 0x91, 0x90, 0x87, 0x00, 0x00, 0x00, 0x00, /* 70-77 */
    0x00, 0x8a, 0x00, 0x8b, 0x00, 0x89, 0x85, 0x00  /* 78-7F */
};

/** Lookup table for extended scancodes (arrow keys etc.). */
static const uint8_t aExtScan2Hid[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 00-07 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 08-1F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 10-17 */
    0x00, 0x00, 0x00, 0x00, 0x58, 0xe4, 0x00, 0x00, /* 18-1F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 20-27 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 28-2F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x00, 0x46, /* 30-37 */
    /* Sun-specific keys.  Most of the XT codes are made up  */
    0xe6, 0x00, 0x00, 0x75, 0x76, 0x77, 0xA3, 0x78, /* 38-3F */
    0x80, 0x81, 0x82, 0x79, 0x00, 0x00, 0x48, 0x4a, /* 40-47 */
    0x52, 0x4b, 0x00, 0x50, 0x00, 0x4f, 0x00, 0x4d, /* 48-4F */
    0x51, 0x4e, 0x49, 0x4c, 0x00, 0x00, 0x00, 0x00, /* 50-57 */
    0x00, 0x00, 0x00, 0xe3, 0xe7, 0x65, 0x66, 0x00, /* 58-5F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 60-67 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 68-6F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 70-77 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /* 78-7F */
};

/**
 * Convert a PC scan code to a USB HID usage byte.
 *
 * @param state         Current state of the translator (scan_state_t).
 * @param scanCode      Incoming scan code.
 * @param pUsage        Pointer to usage; high bit set for key up events. The
 *                      contents are only valid if returned state is SS_IDLE.
 *
 * @return scan_state_t New state of the translator.
 */
static scan_state_t ScancodeToHidUsage(scan_state_t state, uint8_t scanCode, uint32_t *pUsage)
{
    uint32_t    keyUp;
    uint8_t     usage;

    Assert(pUsage);

    /* Isolate the scan code and key break flag. */
    keyUp = (scanCode & 0x80) << 24;

    switch (state) {
    case SS_IDLE:
        if (scanCode == 0xE0) {
            state = SS_EXT;
        } else if (scanCode == 0xE1) {
            state = SS_EXT1;
        } else {
            usage = aScancode2Hid[scanCode & 0x7F];
            *pUsage = usage | keyUp;
            /* Remain in SS_IDLE state. */
        }
        break;
    case SS_EXT:
        usage = aExtScan2Hid[scanCode & 0x7F];
        *pUsage = usage | keyUp;
        state = SS_IDLE;
        break;
    case SS_EXT1:
        /* The sequence is E1 1D 45 E1 9D C5. We take the easy way out and remain
         * in the SS_EXT1 state until 45 or C5 is received.
         */
        if ((scanCode & 0x7F) == 0x45) {
            *pUsage = 0x48;
            if (scanCode == 0xC5)
                *pUsage |= keyUp;
            state = SS_IDLE;
        }
        /* Else remain in SS_EXT1 state. */
        break;
    }
    return state;
}


/* -=-=-=-=- IBase -=-=-=-=- */

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *)  drvKbdQueueQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS      pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVKBDQUEUE    pThis   = PDMINS_2_DATA(pDrvIns, PDRVKBDQUEUE);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIKEYBOARDCONNECTOR, &pThis->IConnector);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIKEYBOARDPORT, &pThis->IPort);
    return NULL;
}


/* -=-=-=-=- IKeyboardPort -=-=-=-=- */

/** Converts a pointer to DRVKBDQUEUE::IPort to a DRVKBDQUEUE pointer. */
#define IKEYBOARDPORT_2_DRVKBDQUEUE(pInterface) ( (PDRVKBDQUEUE)((char *)(pInterface) - RT_OFFSETOF(DRVKBDQUEUE, IPort)) )


/**
 * Queues a keyboard event.
 * Because of the event queueing the EMT context requirement is lifted.
 *
 * @returns VBox status code.
 * @param   pInterface          Pointer to this interface structure.
 * @param   u8ScanCode          The scan code to translate/queue.
 * @thread  Any thread.
 */
static DECLCALLBACK(int) drvKbdQueuePutEventScan(PPDMIKEYBOARDPORT pInterface, uint8_t u8ScanCode)
{
    PDRVKBDQUEUE pDrv = IKEYBOARDPORT_2_DRVKBDQUEUE(pInterface);
    /* Ignore any attempt to send events if queue is inactive. */
    if (pDrv->fInactive)
        return VINF_SUCCESS;

    uint32_t    u32Usage = 0;
    pDrv->XlatState = ScancodeToHidUsage(pDrv->XlatState, u8ScanCode, &u32Usage);

    if (pDrv->XlatState == SS_IDLE) {
        PDRVKBDQUEUEITEM pItem = (PDRVKBDQUEUEITEM)PDMQueueAlloc(pDrv->pQueue);
        if (pItem)
        {
            /*
             * Work around incredibly poorly desgined Korean keyboards which
             * only send break events for Hangul/Hanja keys -- convert a lone
             * key up into a key up/key down sequence.
             */
            if (u32Usage == 0x80000090 || u32Usage == 0x80000091)
            {
                PDRVKBDQUEUEITEM pItem2 = (PDRVKBDQUEUEITEM)PDMQueueAlloc(pDrv->pQueue);
                /*
                 * NB: If there's no room in the queue, we will drop the faked
                 * key down event. Probably less bad than the alternatives.
                 */
                if (pItem2)
                {
                    /* Manufacture a key down event. */
                    pItem2->u32UsageCode = u32Usage & ~0x80000000;
                    PDMQueueInsert(pDrv->pQueue, &pItem2->Core);
                }
            }

            pItem->u32UsageCode = u32Usage;
            PDMQueueInsert(pDrv->pQueue, &pItem->Core);

            return VINF_SUCCESS;
        }
        if (!pDrv->fSuspended)
            AssertMsgFailed(("drvKbdQueuePutEventScan: Queue is full!!!!\n"));
        return VERR_PDM_NO_QUEUE_ITEMS;
    }
    else
        return VINF_SUCCESS;
}


/* -=-=-=-=- IConnector -=-=-=-=- */

#define PPDMIKEYBOARDCONNECTOR_2_DRVKBDQUEUE(pInterface) ( (PDRVKBDQUEUE)((char *)(pInterface) - RT_OFFSETOF(DRVKBDQUEUE, IConnector)) )


/**
 * Pass LED status changes from the guest thru to the frontend driver.
 *
 * @param   pInterface  Pointer to the keyboard connector interface structure.
 * @param   enmLeds     The new LED mask.
 */
static DECLCALLBACK(void) drvKbdPassThruLedsChange(PPDMIKEYBOARDCONNECTOR pInterface, PDMKEYBLEDS enmLeds)
{
    PDRVKBDQUEUE pDrv = PPDMIKEYBOARDCONNECTOR_2_DRVKBDQUEUE(pInterface);
    pDrv->pDownConnector->pfnLedStatusChange(pDrv->pDownConnector, enmLeds);
}

/**
 * Pass keyboard state changes from the guest thru to the frontend driver.
 *
 * @param   pInterface  Pointer to the keyboard connector interface structure.
 * @param   fActive     The new active/inactive state.
 */
static DECLCALLBACK(void) drvKbdPassThruSetActive(PPDMIKEYBOARDCONNECTOR pInterface, bool fActive)
{
    PDRVKBDQUEUE pDrv = PPDMIKEYBOARDCONNECTOR_2_DRVKBDQUEUE(pInterface);

    AssertPtr(pDrv->pDownConnector->pfnSetActive);
    pDrv->pDownConnector->pfnSetActive(pDrv->pDownConnector, fActive);
}

/**
 * Flush the keyboard queue if there are pending events.
 *
 * @param   pInterface  Pointer to the keyboard connector interface structure.
 */
static DECLCALLBACK(void) drvKbdFlushQueue(PPDMIKEYBOARDCONNECTOR pInterface)
{
    PDRVKBDQUEUE pDrv = PPDMIKEYBOARDCONNECTOR_2_DRVKBDQUEUE(pInterface);

    AssertPtr(pDrv->pQueue);
    PDMQueueFlushIfNecessary(pDrv->pQueue);
}


/* -=-=-=-=- queue -=-=-=-=- */

/**
 * Queue callback for processing a queued item.
 *
 * @returns Success indicator.
 *          If false the item will not be removed and the flushing will stop.
 * @param   pDrvIns         The driver instance.
 * @param   pItemCore       Pointer to the queue item to process.
 */
static DECLCALLBACK(bool) drvKbdQueueConsumer(PPDMDRVINS pDrvIns, PPDMQUEUEITEMCORE pItemCore)
{
    PDRVKBDQUEUE        pThis = PDMINS_2_DATA(pDrvIns, PDRVKBDQUEUE);
    PDRVKBDQUEUEITEM    pItem = (PDRVKBDQUEUEITEM)pItemCore;
    int rc = pThis->pUpPort->pfnPutEventHid(pThis->pUpPort, pItem->u32UsageCode);
    return RT_SUCCESS(rc);
}


/* -=-=-=-=- driver interface -=-=-=-=- */

/**
 * Power On notification.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The drive instance data.
 */
static DECLCALLBACK(void) drvKbdQueuePowerOn(PPDMDRVINS pDrvIns)
{
    PDRVKBDQUEUE    pThis = PDMINS_2_DATA(pDrvIns, PDRVKBDQUEUE);
    pThis->fInactive = false;
}


/**
 * Reset notification.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The drive instance data.
 */
static DECLCALLBACK(void)  drvKbdQueueReset(PPDMDRVINS pDrvIns)
{
    //PDRVKBDQUEUE        pThis = PDMINS_2_DATA(pDrvIns, PDRVKBDQUEUE);
    /** @todo purge the queue on reset. */
    RT_NOREF(pDrvIns);
}


/**
 * Suspend notification.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The drive instance data.
 */
static DECLCALLBACK(void)  drvKbdQueueSuspend(PPDMDRVINS pDrvIns)
{
    PDRVKBDQUEUE        pThis = PDMINS_2_DATA(pDrvIns, PDRVKBDQUEUE);
    pThis->fSuspended = true;
}


/**
 * Resume notification.
 *
 * @returns VBox status code.
 * @param   pDrvIns     The drive instance data.
 */
static DECLCALLBACK(void)  drvKbdQueueResume(PPDMDRVINS pDrvIns)
{
    PDRVKBDQUEUE        pThis = PDMINS_2_DATA(pDrvIns, PDRVKBDQUEUE);
    pThis->fInactive = false;
    pThis->fSuspended = false;
}


/**
 * Power Off notification.
 *
 * @param   pDrvIns     The drive instance data.
 */
static DECLCALLBACK(void) drvKbdQueuePowerOff(PPDMDRVINS pDrvIns)
{
    PDRVKBDQUEUE        pThis = PDMINS_2_DATA(pDrvIns, PDRVKBDQUEUE);
    pThis->fInactive = true;
}


/**
 * Construct a keyboard driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvKbdQueueConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    PDRVKBDQUEUE pDrv = PDMINS_2_DATA(pDrvIns, PDRVKBDQUEUE);
    LogFlow(("drvKbdQueueConstruct: iInstance=%d\n", pDrvIns->iInstance));
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);

    /*
     * Validate configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, "QueueSize\0Interval\0"))
        return VERR_PDM_DRVINS_UNKNOWN_CFG_VALUES;

    /*
     * Init basic data members and interfaces.
     */
    pDrv->fInactive                         = true;
    pDrv->fSuspended                        = false;
    pDrv->XlatState                         = SS_IDLE;
    /* IBase. */
    pDrvIns->IBase.pfnQueryInterface        = drvKbdQueueQueryInterface;
    /* IKeyboardConnector. */
    pDrv->IConnector.pfnLedStatusChange     = drvKbdPassThruLedsChange;
    pDrv->IConnector.pfnSetActive           = drvKbdPassThruSetActive;
    pDrv->IConnector.pfnFlushQueue          = drvKbdFlushQueue;
    /* IKeyboardPort. */
    pDrv->IPort.pfnPutEventScan             = drvKbdQueuePutEventScan;

    /*
     * Get the IKeyboardPort interface of the above driver/device.
     */
    pDrv->pUpPort = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMIKEYBOARDPORT);
    if (!pDrv->pUpPort)
    {
        AssertMsgFailed(("Configuration error: No keyboard port interface above!\n"));
        return VERR_PDM_MISSING_INTERFACE_ABOVE;
    }

    /*
     * Attach driver below and query it's connector interface.
     */
    PPDMIBASE pDownBase;
    int rc = PDMDrvHlpAttach(pDrvIns, fFlags, &pDownBase);
    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Failed to attach driver below us! rc=%Rra\n", rc));
        return rc;
    }
    pDrv->pDownConnector = PDMIBASE_QUERY_INTERFACE(pDownBase, PDMIKEYBOARDCONNECTOR);
    if (!pDrv->pDownConnector)
    {
        AssertMsgFailed(("Configuration error: No keyboard connector interface below!\n"));
        return VERR_PDM_MISSING_INTERFACE_BELOW;
    }

    /*
     * Create the queue.
     */
    uint32_t cMilliesInterval = 0;
    rc = CFGMR3QueryU32(pCfg, "Interval", &cMilliesInterval);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
        cMilliesInterval = 0;
    else if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Configuration error: 32-bit \"Interval\" -> rc=%Rrc\n", rc));
        return rc;
    }

    uint32_t cItems = 0;
    rc = CFGMR3QueryU32(pCfg, "QueueSize", &cItems);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
        cItems = 128;
    else if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Configuration error: 32-bit \"QueueSize\" -> rc=%Rrc\n", rc));
        return rc;
    }

    rc = PDMDrvHlpQueueCreate(pDrvIns, sizeof(DRVKBDQUEUEITEM), cItems, cMilliesInterval, drvKbdQueueConsumer, "Keyboard", &pDrv->pQueue);
    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Failed to create driver: cItems=%d cMilliesInterval=%d rc=%Rrc\n", cItems, cMilliesInterval, rc));
        return rc;
    }

    return VINF_SUCCESS;
}


/**
 * Keyboard queue driver registration record.
 */
const PDMDRVREG g_DrvKeyboardQueue =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "KeyboardQueue",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Keyboard queue driver to plug in between the key source and the device to do queueing and inter-thread transport.",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_KEYBOARD,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVKBDQUEUE),
    /* pfnConstruct */
    drvKbdQueueConstruct,
    /* pfnRelocate */
    NULL,
    /* pfnDestruct */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    drvKbdQueuePowerOn,
    /* pfnReset */
    drvKbdQueueReset,
    /* pfnSuspend */
    drvKbdQueueSuspend,
    /* pfnResume */
    drvKbdQueueResume,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnPowerOff */
    drvKbdQueuePowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};

