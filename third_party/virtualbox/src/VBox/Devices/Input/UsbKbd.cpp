/* $Id: UsbKbd.cpp $ */
/** @file
 * UsbKbd - USB Human Interface Device Emulation, Keyboard.
 */

/*
 * Copyright (C) 2007-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/** @page pg_usb_kbd    USB Keyboard Device Emulation.
 *
 * This module implements a standard USB keyboard which uses the boot
 * interface. The keyboard sends reports which have room for up to six
 * normal keys and all standard modifier keys. A report always reflects the
 * current state of the keyboard and indicates which keys are held down.
 *
 * Software normally utilizes the keyboard's interrupt endpoint to request
 * reports to be sent whenever a state change occurs. However, reports can
 * also be sent whenever an interrupt transfer is initiated (the keyboard is
 * not "idle") or requested via the control endpoint (polling).
 *
 * Because turnaround on USB is relatively slow, the keyboard often ends up
 * in a situation where new input arrived but there is no URB available
 * where a report could be written to. The PDM queue maintained by the
 * keyboard driver is utilized to provide buffering and hold incoming events
 * until they can be passed along. The USB keyboard can effectively buffer
 * up to one event.
 *
 * If there is a pending event and a new URB becomes available, a report is
 * built and the keyboard queue is flushed. This ensures that queued events
 * are processed as quickly as possible.
 *
 *
 * References:
 *
 * Device Class Definition for Human Interface Devices (HID), Version 1.11
 *
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP   LOG_GROUP_USB_KBD
#include <VBox/vmm/pdmusb.h>
#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/mem.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>
#include <iprt/uuid.h>
#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** @name USB HID string IDs
 * @{ */
#define USBHID_STR_ID_MANUFACTURER  1
#define USBHID_STR_ID_PRODUCT       2
/** @} */

/** @name USB HID specific descriptor types
 * @{ */
#define DT_IF_HID_DESCRIPTOR        0x21
#define DT_IF_HID_REPORT            0x22
/** @} */

/** @name USB HID vendor and product IDs
 * @{ */
#define VBOX_USB_VENDOR             0x80EE
#define USBHID_PID_KEYBOARD         0x0010
/** @} */

/** @name USB HID class specific requests
 * @{ */
#define HID_REQ_GET_REPORT          0x01
#define HID_REQ_GET_IDLE            0x02
#define HID_REQ_SET_REPORT          0x09
#define HID_REQ_SET_IDLE            0x0A
/** @} */

/** @name USB HID additional constants
 * @{ */
/** The highest USB usage code reported by the VBox emulated keyboard */
#define VBOX_USB_MAX_USAGE_CODE     0xE7
/** The size of an array needed to store all USB usage codes */
#define VBOX_USB_USAGE_ARRAY_SIZE   (VBOX_USB_MAX_USAGE_CODE + 1)
#define USBHID_USAGE_ROLL_OVER      1
/** The usage code of the first modifier key. */
#define USBHID_MODIFIER_FIRST       0xE0
/** The usage code of the last modifier key. */
#define USBHID_MODIFIER_LAST        0xE7
/** @} */


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * The USB HID request state.
 */
typedef enum USBHIDREQSTATE
{
    /** Invalid status. */
    USBHIDREQSTATE_INVALID = 0,
    /** Ready to receive a new read request. */
    USBHIDREQSTATE_READY,
    /** Have (more) data for the host. */
    USBHIDREQSTATE_DATA_TO_HOST,
    /** Waiting to supply status information to the host. */
    USBHIDREQSTATE_STATUS,
    /** The end of the valid states. */
    USBHIDREQSTATE_END
} USBHIDREQSTATE;


/**
 * Endpoint status data.
 */
typedef struct USBHIDEP
{
    bool                fHalted;
} USBHIDEP;
/** Pointer to the endpoint status. */
typedef USBHIDEP *PUSBHIDEP;


/**
 * A URB queue.
 */
typedef struct USBHIDURBQUEUE
{
    /** The head pointer. */
    PVUSBURB            pHead;
    /** Where to insert the next entry. */
    PVUSBURB           *ppTail;
} USBHIDURBQUEUE;
/** Pointer to a URB queue. */
typedef USBHIDURBQUEUE *PUSBHIDURBQUEUE;
/** Pointer to a const URB queue. */
typedef USBHIDURBQUEUE const *PCUSBHIDURBQUEUE;


/**
 * The USB HID report structure for regular keys.
 */
typedef struct USBHIDK_REPORT
{
    uint8_t     ShiftState;     /**< Modifier keys bitfield */
    uint8_t     Reserved;       /**< Currently unused */
    uint8_t     aKeys[6];       /**< Normal keys */
} USBHIDK_REPORT, *PUSBHIDK_REPORT;

/**
 * The USB HID instance data.
 */
typedef struct USBHID
{
    /** Pointer back to the PDM USB Device instance structure. */
    PPDMUSBINS          pUsbIns;
    /** Critical section protecting the device state. */
    RTCRITSECT          CritSect;

    /** The current configuration.
     * (0 - default, 1 - the one supported configuration, i.e configured.) */
    uint8_t             bConfigurationValue;
    /** USB HID Idle value.
     * (0 - only report state change, !=0 - report in bIdle * 4ms intervals.) */
    uint8_t             bIdle;
    /** Endpoint 0 is the default control pipe, 1 is the dev->host interrupt one. */
    USBHIDEP            aEps[2];
    /** The state of the HID (state machine).*/
    USBHIDREQSTATE      enmState;

    /** Pending to-host queue.
     * The URBs waiting here are waiting for data to become available.
     */
    USBHIDURBQUEUE      ToHostQueue;

    /** Done queue
     * The URBs stashed here are waiting to be reaped. */
    USBHIDURBQUEUE      DoneQueue;
    /** Signalled when adding an URB to the done queue and fHaveDoneQueueWaiter
     *  is set. */
    RTSEMEVENT          hEvtDoneQueue;
    /** Someone is waiting on the done queue. */
    bool                fHaveDoneQueueWaiter;
    /** If device has pending changes. */
    bool                fHasPendingChanges;
    /** Currently depressed keys */
    uint8_t             abDepressedKeys[VBOX_USB_USAGE_ARRAY_SIZE];

    /**
     * Keyboard port - LUN#0.
     *
     * @implements  PDMIBASE
     * @implements  PDMIKEYBOARDPORT
     */
    struct
    {
        /** The base interface for the keyboard port. */
        PDMIBASE                            IBase;
        /** The keyboard port base interface. */
        PDMIKEYBOARDPORT                    IPort;

        /** The base interface of the attached keyboard driver. */
        R3PTRTYPE(PPDMIBASE)                pDrvBase;
        /** The keyboard interface of the attached keyboard driver. */
        R3PTRTYPE(PPDMIKEYBOARDCONNECTOR)   pDrv;
    } Lun0;
} USBHID;
/** Pointer to the USB HID instance data. */
typedef USBHID *PUSBHID;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const PDMUSBDESCCACHESTRING g_aUsbHidStrings_en_US[] =
{
    { USBHID_STR_ID_MANUFACTURER,   "VirtualBox"    },
    { USBHID_STR_ID_PRODUCT,        "USB Keyboard"  },
};

static const PDMUSBDESCCACHELANG g_aUsbHidLanguages[] =
{
    { 0x0409, RT_ELEMENTS(g_aUsbHidStrings_en_US), g_aUsbHidStrings_en_US }
};

static const VUSBDESCENDPOINTEX g_aUsbHidEndpointDescs[] =
{
    {
        {
            /* .bLength = */            sizeof(VUSBDESCENDPOINT),
            /* .bDescriptorType = */    VUSB_DT_ENDPOINT,
            /* .bEndpointAddress = */   0x81 /* ep=1, in */,
            /* .bmAttributes = */       3 /* interrupt */,
            /* .wMaxPacketSize = */     8,
            /* .bInterval = */          10,
        },
        /* .pvMore = */     NULL,
        /* .pvClass = */    NULL,
        /* .cbClass = */    0
    },
};

/** HID report descriptor. */
static const uint8_t g_UsbHidReportDesc[] =
{
    /* Usage Page */                0x05, 0x01,     /* Generic Desktop */
    /* Usage */                     0x09, 0x06,     /* Keyboard */
    /* Collection */                0xA1, 0x01,     /* Application */
    /* Usage Page */                0x05, 0x07,     /* Keyboard */
    /* Usage Minimum */             0x19, 0xE0,     /* Left Ctrl Key */
    /* Usage Maximum */             0x29, 0xE7,     /* Right GUI Key */
    /* Logical Minimum */           0x15, 0x00,     /* 0 */
    /* Logical Maximum */           0x25, 0x01,     /* 1 */
    /* Report Count */              0x95, 0x08,     /* 8 */
    /* Report Size */               0x75, 0x01,     /* 1 */
    /* Input */                     0x81, 0x02,     /* Data, Value, Absolute, Bit field */
    /* Report Count */              0x95, 0x01,     /* 1 */
    /* Report Size */               0x75, 0x08,     /* 8 (padding bits) */
    /* Input */                     0x81, 0x01,     /* Constant, Array, Absolute, Bit field */
    /* Report Count */              0x95, 0x05,     /* 5 */
    /* Report Size */               0x75, 0x01,     /* 1 */
    /* Usage Page */                0x05, 0x08,     /* LEDs */
    /* Usage Minimum */             0x19, 0x01,     /* Num Lock */
    /* Usage Maximum */             0x29, 0x05,     /* Kana */
    /* Output */                    0x91, 0x02,     /* Data, Value, Absolute, Non-volatile,Bit field */
    /* Report Count */              0x95, 0x01,     /* 1 */
    /* Report Size */               0x75, 0x03,     /* 3 */
    /* Output */                    0x91, 0x01,     /* Constant, Value, Absolute, Non-volatile, Bit field */
    /* Report Count */              0x95, 0x06,     /* 6 */
    /* Report Size */               0x75, 0x08,     /* 8 */
    /* Logical Minimum */           0x15, 0x00,     /* 0 */
    /* Logical Maximum */           0x26, 0xFF,0x00,/* 255 */
    /* Usage Page */                0x05, 0x07,     /* Keyboard */
    /* Usage Minimum */             0x19, 0x00,     /* 0 */
    /* Usage Maximum */             0x29, 0xFF,     /* 255 */
    /* Input */                     0x81, 0x00,     /* Data, Array, Absolute, Bit field */
    /* End Collection */            0xC0,
};

/** Additional HID class interface descriptor. */
static const uint8_t g_UsbHidIfHidDesc[] =
{
    /* .bLength = */                0x09,
    /* .bDescriptorType = */        0x21,       /* HID */
    /* .bcdHID = */                 0x10, 0x01, /* 1.1 */
    /* .bCountryCode = */           0x0D,       /* International (ISO) */
    /* .bNumDescriptors = */        1,
    /* .bDescriptorType = */        0x22,       /* Report */
    /* .wDescriptorLength = */      sizeof(g_UsbHidReportDesc), 0x00
};

static const VUSBDESCINTERFACEEX g_UsbHidInterfaceDesc =
{
    {
        /* .bLength = */                sizeof(VUSBDESCINTERFACE),
        /* .bDescriptorType = */        VUSB_DT_INTERFACE,
        /* .bInterfaceNumber = */       0,
        /* .bAlternateSetting = */      0,
        /* .bNumEndpoints = */          1,
        /* .bInterfaceClass = */        3 /* HID */,
        /* .bInterfaceSubClass = */     1 /* Boot Interface */,
        /* .bInterfaceProtocol = */     1 /* Keyboard */,
        /* .iInterface = */             0
    },
    /* .pvMore = */     NULL,
    /* .pvClass = */    &g_UsbHidIfHidDesc,
    /* .cbClass = */    sizeof(g_UsbHidIfHidDesc),
    &g_aUsbHidEndpointDescs[0],
    /* .pIAD = */ NULL,
    /* .cbIAD = */ 0
};

static const VUSBINTERFACE g_aUsbHidInterfaces[] =
{
    { &g_UsbHidInterfaceDesc, /* .cSettings = */ 1 },
};

static const VUSBDESCCONFIGEX g_UsbHidConfigDesc =
{
    {
        /* .bLength = */            sizeof(VUSBDESCCONFIG),
        /* .bDescriptorType = */    VUSB_DT_CONFIG,
        /* .wTotalLength = */       0 /* recalculated on read */,
        /* .bNumInterfaces = */     RT_ELEMENTS(g_aUsbHidInterfaces),
        /* .bConfigurationValue =*/ 1,
        /* .iConfiguration = */     0,
        /* .bmAttributes = */       RT_BIT(7),
        /* .MaxPower = */           50 /* 100mA */
    },
    NULL,                           /* pvMore */
    NULL,                           /* pvClass */
    0,                              /* cbClass */
    &g_aUsbHidInterfaces[0],
    NULL                            /* pvOriginal */
};

static const VUSBDESCDEVICE g_UsbHidDeviceDesc =
{
    /* .bLength = */                sizeof(g_UsbHidDeviceDesc),
    /* .bDescriptorType = */        VUSB_DT_DEVICE,
    /* .bcdUsb = */                 0x110,  /* 1.1 */
    /* .bDeviceClass = */           0 /* Class specified in the interface desc. */,
    /* .bDeviceSubClass = */        0 /* Subclass specified in the interface desc. */,
    /* .bDeviceProtocol = */        0 /* Protocol specified in the interface desc. */,
    /* .bMaxPacketSize0 = */        8,
    /* .idVendor = */               VBOX_USB_VENDOR,
    /* .idProduct = */              USBHID_PID_KEYBOARD,
    /* .bcdDevice = */              0x0100, /* 1.0 */
    /* .iManufacturer = */          USBHID_STR_ID_MANUFACTURER,
    /* .iProduct = */               USBHID_STR_ID_PRODUCT,
    /* .iSerialNumber = */          0,
    /* .bNumConfigurations = */     1
};

static const PDMUSBDESCCACHE g_UsbHidDescCache =
{
    /* .pDevice = */                &g_UsbHidDeviceDesc,
    /* .paConfigs = */              &g_UsbHidConfigDesc,
    /* .paLanguages = */            g_aUsbHidLanguages,
    /* .cLanguages = */             RT_ELEMENTS(g_aUsbHidLanguages),
    /* .fUseCachedDescriptors = */  true,
    /* .fUseCachedStringsDescriptors = */ true
};


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/


/**
 * Initializes an URB queue.
 *
 * @param   pQueue              The URB queue.
 */
static void usbHidQueueInit(PUSBHIDURBQUEUE pQueue)
{
    pQueue->pHead = NULL;
    pQueue->ppTail = &pQueue->pHead;
}

/**
 * Inserts an URB at the end of the queue.
 *
 * @param   pQueue              The URB queue.
 * @param   pUrb                The URB to insert.
 */
DECLINLINE(void) usbHidQueueAddTail(PUSBHIDURBQUEUE pQueue, PVUSBURB pUrb)
{
    pUrb->Dev.pNext = NULL;
    *pQueue->ppTail = pUrb;
    pQueue->ppTail  = &pUrb->Dev.pNext;
}


/**
 * Unlinks the head of the queue and returns it.
 *
 * @returns The head entry.
 * @param   pQueue              The URB queue.
 */
DECLINLINE(PVUSBURB) usbHidQueueRemoveHead(PUSBHIDURBQUEUE pQueue)
{
    PVUSBURB pUrb = pQueue->pHead;
    if (pUrb)
    {
        PVUSBURB pNext = pUrb->Dev.pNext;
        pQueue->pHead = pNext;
        if (!pNext)
            pQueue->ppTail = &pQueue->pHead;
        else
            pUrb->Dev.pNext = NULL;
    }
    return pUrb;
}


/**
 * Removes an URB from anywhere in the queue.
 *
 * @returns true if found, false if not.
 * @param   pQueue              The URB queue.
 * @param   pUrb                The URB to remove.
 */
DECLINLINE(bool) usbHidQueueRemove(PUSBHIDURBQUEUE pQueue, PVUSBURB pUrb)
{
    PVUSBURB pCur = pQueue->pHead;
    if (pCur == pUrb)
        pQueue->pHead = pUrb->Dev.pNext;
    else
    {
        while (pCur)
        {
            if (pCur->Dev.pNext == pUrb)
            {
                pCur->Dev.pNext = pUrb->Dev.pNext;
                break;
            }
            pCur = pCur->Dev.pNext;
        }
        if (!pCur)
            return false;
    }
    if (!pUrb->Dev.pNext)
        pQueue->ppTail = &pQueue->pHead;
    return true;
}


#if 0 /* unused */
/**
 * Checks if the queue is empty or not.
 *
 * @returns true if it is, false if it isn't.
 * @param   pQueue              The URB queue.
 */
DECLINLINE(bool) usbHidQueueIsEmpty(PCUSBHIDURBQUEUE pQueue)
{
    return pQueue->pHead == NULL;
}
#endif /* unused */


/**
 * Links an URB into the done queue.
 *
 * @param   pThis               The HID instance.
 * @param   pUrb                The URB.
 */
static void usbHidLinkDone(PUSBHID pThis, PVUSBURB pUrb)
{
    usbHidQueueAddTail(&pThis->DoneQueue, pUrb);

    if (pThis->fHaveDoneQueueWaiter)
    {
        int rc = RTSemEventSignal(pThis->hEvtDoneQueue);
        AssertRC(rc);
    }
}


/**
 * Completes the URB with a stalled state, halting the pipe.
 */
static int usbHidCompleteStall(PUSBHID pThis, PUSBHIDEP pEp, PVUSBURB pUrb, const char *pszWhy)
{
    RT_NOREF1(pszWhy);
    Log(("usbHidCompleteStall/#%u: pUrb=%p:%s: %s\n", pThis->pUsbIns->iInstance, pUrb, pUrb->pszDesc, pszWhy));

    pUrb->enmStatus = VUSBSTATUS_STALL;

    /** @todo figure out if the stall is global or pipe-specific or both. */
    if (pEp)
        pEp->fHalted = true;
    else
    {
        pThis->aEps[0].fHalted = true;
        pThis->aEps[1].fHalted = true;
    }

    usbHidLinkDone(pThis, pUrb);
    return VINF_SUCCESS;
}


/**
 * Completes the URB with a OK state.
 */
static int usbHidCompleteOk(PUSBHID pThis, PVUSBURB pUrb, size_t cbData)
{
    Log(("usbHidCompleteOk/#%u: pUrb=%p:%s cbData=%#zx\n", pThis->pUsbIns->iInstance, pUrb, pUrb->pszDesc, cbData));

    pUrb->enmStatus = VUSBSTATUS_OK;
    pUrb->cbData    = (uint32_t)cbData;

    usbHidLinkDone(pThis, pUrb);
    return VINF_SUCCESS;
}


/**
 * Reset worker for usbHidUsbReset, usbHidUsbSetConfiguration and
 * usbHidHandleDefaultPipe.
 *
 * @returns VBox status code.
 * @param   pThis               The HID instance.
 * @param   pUrb                Set when usbHidHandleDefaultPipe is the
 *                              caller.
 * @param   fSetConfig          Set when usbHidUsbSetConfiguration is the
 *                              caller.
 */
static int usbHidResetWorker(PUSBHID pThis, PVUSBURB pUrb, bool fSetConfig)
{
    /*
     * Deactivate the keyboard.
     */
    pThis->Lun0.pDrv->pfnSetActive(pThis->Lun0.pDrv, false);

    /*
     * Reset the device state.
     */
    pThis->enmState = USBHIDREQSTATE_READY;
    pThis->bIdle = 0;
    pThis->fHasPendingChanges = false;

    for (unsigned i = 0; i < RT_ELEMENTS(pThis->aEps); i++)
        pThis->aEps[i].fHalted = false;

    if (!pUrb && !fSetConfig) /* (only device reset) */
        pThis->bConfigurationValue = 0; /* default */

    /*
     * Ditch all pending URBs.
     */
    PVUSBURB pCurUrb;
    while ((pCurUrb = usbHidQueueRemoveHead(&pThis->ToHostQueue)) != NULL)
    {
        pCurUrb->enmStatus = VUSBSTATUS_CRC;
        usbHidLinkDone(pThis, pCurUrb);
    }

    if (pUrb)
        return usbHidCompleteOk(pThis, pUrb, 0);
    return VINF_SUCCESS;
}

/**
 * Returns true if the usage code corresponds to a keyboard modifier key
 * (left or right ctrl, shift, alt or GUI).  The usage codes for these keys
 * are the range 0xe0 to 0xe7.
 */
static bool usbHidUsageCodeIsModifier(uint8_t u8Usage)
{
    return u8Usage >= USBHID_MODIFIER_FIRST && u8Usage <= USBHID_MODIFIER_LAST;
}

/**
 * Convert a USB HID usage code to a keyboard modifier flag.  The arithmetic
 * is simple: the modifier keys have usage codes from 0xe0 to 0xe7, and the
 * lower nibble is the bit number of the flag.
 */
static uint8_t usbHidModifierToFlag(uint8_t u8Usage)
{
    Assert(usbHidUsageCodeIsModifier(u8Usage));
    return RT_BIT(u8Usage & 0xf);
}

/**
 * Create a USB HID keyboard report reflecting the current state of the
 * keyboard (up/down keys).
 */
static void usbHidBuildReport(PUSBHIDK_REPORT pReport, uint8_t *pabDepressedKeys)
{
    unsigned iBuf = 0;
    RT_ZERO(*pReport);
    for (unsigned iKey = 0; iKey < VBOX_USB_USAGE_ARRAY_SIZE; ++iKey)
    {
        Assert(iBuf <= RT_ELEMENTS(pReport->aKeys));
        if (pabDepressedKeys[iKey])
        {
            if (usbHidUsageCodeIsModifier(iKey))
                pReport->ShiftState |= usbHidModifierToFlag(iKey);
            else if (iBuf == RT_ELEMENTS(pReport->aKeys))
            {
                /* The USB HID spec says that the entire vector should be
                 * set to ErrorRollOver on overflow.  We don't mind if this
                 * path is taken several times for one report. */
                for (unsigned iBuf2 = 0;
                     iBuf2 < RT_ELEMENTS(pReport->aKeys); ++iBuf2)
                    pReport->aKeys[iBuf2] = USBHID_USAGE_ROLL_OVER;
            }
            else
            {
                pReport->aKeys[iBuf] = iKey;
                ++iBuf;
            }
        }
    }
}

/**
 * Handles a SET_REPORT request sent to the default control pipe. Note
 * that unrecognized requests are ignored without reporting an error.
 */
static void usbHidSetReport(PUSBHID pThis, PVUSBURB pUrb)
{
    PVUSBSETUP pSetup = (PVUSBSETUP)&pUrb->abData[0];
    Assert(pSetup->bRequest == HID_REQ_SET_REPORT);

    /* The LED report is the 3rd report, ID 0 (-> wValue 0x200). */
    if (pSetup->wIndex == 0 && pSetup->wLength == 1 && pSetup->wValue == 0x200)
    {
        PDMKEYBLEDS enmLeds = PDMKEYBLEDS_NONE;
        uint8_t     u8LEDs = pUrb->abData[sizeof(*pSetup)];
        LogFlowFunc(("Setting keybooard LEDs to u8LEDs=%02X\n", u8LEDs));

        /* Translate LED state to PDM format and send upstream. */
        if (u8LEDs & 0x01)
            enmLeds = (PDMKEYBLEDS)(enmLeds | PDMKEYBLEDS_NUMLOCK);
        if (u8LEDs & 0x02)
            enmLeds = (PDMKEYBLEDS)(enmLeds | PDMKEYBLEDS_CAPSLOCK);
        if (u8LEDs & 0x04)
            enmLeds = (PDMKEYBLEDS)(enmLeds | PDMKEYBLEDS_SCROLLLOCK);

        pThis->Lun0.pDrv->pfnLedStatusChange(pThis->Lun0.pDrv, enmLeds);
    }
}

/**
 * Sends a state report to the guest if there is a URB available.
 */
static void usbHidSendReport(PUSBHID pThis)
{
    PVUSBURB pUrb = usbHidQueueRemoveHead(&pThis->ToHostQueue);
    if (pUrb)
    {
        PUSBHIDK_REPORT pReport = (PUSBHIDK_REPORT)&pUrb->abData[0];

        usbHidBuildReport(pReport, pThis->abDepressedKeys);
        pThis->fHasPendingChanges = false;
        usbHidCompleteOk(pThis, pUrb, sizeof(*pReport));
    }
    else
    {
        Log2(("No available URB for USB kbd\n"));
        pThis->fHasPendingChanges = true;
    }
}

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) usbHidKeyboardQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PUSBHID pThis = RT_FROM_MEMBER(pInterface, USBHID, Lun0.IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->Lun0.IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIKEYBOARDPORT, &pThis->Lun0.IPort);
    return NULL;
}

/* See the PS2K device. */
#define KRSP_BAT_FAIL       0xFC    /* Also a 'release keys' signal. */

/**
 * Keyboard event handler.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the keyboard port interface (KBDState::Keyboard.IPort).
 * @param   u32UsageCode    The key usage ID.
 */
static DECLCALLBACK(int) usbHidKeyboardPutEvent(PPDMIKEYBOARDPORT pInterface, uint32_t u32UsageCode)
{
    PUSBHID pThis = RT_FROM_MEMBER(pInterface, USBHID, Lun0.IPort);
    uint8_t     u8HidCode;
    bool        fKeyDown;
    bool        fHaveEvent = true;
    int         rc = VINF_SUCCESS;

    RTCritSectEnter(&pThis->CritSect);

    /* Let's see what we got... */
    fKeyDown  = !(u32UsageCode & 0x80000000);
    u8HidCode = u32UsageCode & 0xFF;
    AssertReturn(u8HidCode <= VBOX_USB_MAX_USAGE_CODE, VERR_INTERNAL_ERROR);

    LogFlowFunc(("key %s: 0x%x\n", fKeyDown ? "down" : "up", u8HidCode));

    /*
     * Due to host key repeat, we can get key events for keys which are
     * already depressed. Drop those right here.
     */
    if (fKeyDown && pThis->abDepressedKeys[u8HidCode])
        fHaveEvent = false;

    /* If there is already a pending event, we won't accept a new one yet. */
    if (pThis->fHasPendingChanges && fHaveEvent)
    {
        rc = VERR_TRY_AGAIN;
    }
    else if (fHaveEvent)
    {
        if (RT_UNLIKELY(u32UsageCode == KRSP_BAT_FAIL))
        {
            /* Clear all currently depressed and unreported keys. */
            RT_ZERO(pThis->abDepressedKeys);
        }
        else
        {
            /* Regular key event - update keyboard state. */
            if (fKeyDown)
                pThis->abDepressedKeys[u8HidCode] = 1;
            else
                pThis->abDepressedKeys[u8HidCode] = 0;
        }

        /*
         * Try sending a report. Note that we already decided to consume the
         * event regardless of whether a URB is available or not. If it's not,
         * we will simply not accept any further events.
         */
        usbHidSendReport(pThis);
    }

    RTCritSectLeave(&pThis->CritSect);

    return rc;
}

/**
 * @interface_method_impl{PDMUSBREG,pfnUrbReap}
 */
static DECLCALLBACK(PVUSBURB) usbHidUrbReap(PPDMUSBINS pUsbIns, RTMSINTERVAL cMillies)
{
    PUSBHID pThis = PDMINS_2_DATA(pUsbIns, PUSBHID);
    //LogFlow(("usbHidUrbReap/#%u: cMillies=%u\n", pUsbIns->iInstance, cMillies));

    RTCritSectEnter(&pThis->CritSect);

    PVUSBURB pUrb = usbHidQueueRemoveHead(&pThis->DoneQueue);
    if (!pUrb && cMillies)
    {
        /* Wait */
        pThis->fHaveDoneQueueWaiter = true;
        RTCritSectLeave(&pThis->CritSect);

        RTSemEventWait(pThis->hEvtDoneQueue, cMillies);

        RTCritSectEnter(&pThis->CritSect);
        pThis->fHaveDoneQueueWaiter = false;

        pUrb = usbHidQueueRemoveHead(&pThis->DoneQueue);
    }

    RTCritSectLeave(&pThis->CritSect);

    if (pUrb)
        Log(("usbHidUrbReap/#%u: pUrb=%p:%s\n", pUsbIns->iInstance, pUrb, pUrb->pszDesc));
    return pUrb;
}


/**
 * @interface_method_impl{PDMUSBREG,pfnWakeup}
 */
static DECLCALLBACK(int) usbHidWakeup(PPDMUSBINS pUsbIns)
{
    PUSBHID pThis = PDMINS_2_DATA(pUsbIns, PUSBHID);

    return RTSemEventSignal(pThis->hEvtDoneQueue);
}


/**
 * @interface_method_impl{PDMUSBREG,pfnUrbCancel}
 */
static DECLCALLBACK(int) usbHidUrbCancel(PPDMUSBINS pUsbIns, PVUSBURB pUrb)
{
    PUSBHID pThis = PDMINS_2_DATA(pUsbIns, PUSBHID);
    LogFlow(("usbHidUrbCancel/#%u: pUrb=%p:%s\n", pUsbIns->iInstance, pUrb, pUrb->pszDesc));
    RTCritSectEnter(&pThis->CritSect);

    /*
     * Remove the URB from the to-host queue and move it onto the done queue.
     */
    if (usbHidQueueRemove(&pThis->ToHostQueue, pUrb))
        usbHidLinkDone(pThis, pUrb);

    RTCritSectLeave(&pThis->CritSect);
    return VINF_SUCCESS;
}


/**
 * Handles request sent to the inbound (device to host) interrupt pipe. This is
 * rather different from bulk requests because an interrupt read URB may complete
 * after arbitrarily long time.
 */
static int usbHidHandleIntrDevToHost(PUSBHID pThis, PUSBHIDEP pEp, PVUSBURB pUrb)
{
    /*
     * Stall the request if the pipe is halted.
     */
    if (RT_UNLIKELY(pEp->fHalted))
        return usbHidCompleteStall(pThis, NULL, pUrb, "Halted pipe");

    /*
     * Deal with the URB according to the state.
     */
    switch (pThis->enmState)
    {
        /*
         * We've data left to transfer to the host.
         */
        case USBHIDREQSTATE_DATA_TO_HOST:
        {
            AssertFailed();
            Log(("usbHidHandleIntrDevToHost: Entering STATUS\n"));
            return usbHidCompleteOk(pThis, pUrb, 0);
        }

        /*
         * Status transfer.
         */
        case USBHIDREQSTATE_STATUS:
        {
            AssertFailed();
            Log(("usbHidHandleIntrDevToHost: Entering READY\n"));
            pThis->enmState = USBHIDREQSTATE_READY;
            return usbHidCompleteOk(pThis, pUrb, 0);
        }

        case USBHIDREQSTATE_READY:
            usbHidQueueAddTail(&pThis->ToHostQueue, pUrb);
            /* If device was not set idle, send the current report right away. */
            if (pThis->bIdle != 0 || pThis->fHasPendingChanges)
            {
                usbHidSendReport(pThis);
                LogFlow(("usbHidHandleIntrDevToHost: Sent report via %p:%s\n", pUrb, pUrb->pszDesc));
                Assert(!pThis->fHasPendingChanges); /* Since we just got a URB... */
                /* There may be more input queued up. Ask for it now. */
                pThis->Lun0.pDrv->pfnFlushQueue(pThis->Lun0.pDrv);
            }
            return VINF_SUCCESS;

        /*
         * Bad states, stall.
         */
        default:
            Log(("usbHidHandleIntrDevToHost: enmState=%d cbData=%#x\n", pThis->enmState, pUrb->cbData));
            return usbHidCompleteStall(pThis, NULL, pUrb, "Really bad state (D2H)!");
    }
}


/**
 * Handles request sent to the default control pipe.
 */
static int usbHidHandleDefaultPipe(PUSBHID pThis, PUSBHIDEP pEp, PVUSBURB pUrb)
{
    PVUSBSETUP pSetup = (PVUSBSETUP)&pUrb->abData[0];
    LogFlow(("usbHidHandleDefaultPipe: cbData=%d\n", pUrb->cbData));

    AssertReturn(pUrb->cbData >= sizeof(*pSetup), VERR_VUSB_FAILED_TO_QUEUE_URB);

    if ((pSetup->bmRequestType & VUSB_REQ_MASK) == VUSB_REQ_STANDARD)
    {
        switch (pSetup->bRequest)
        {
            case VUSB_REQ_GET_DESCRIPTOR:
            {
                switch (pSetup->bmRequestType)
                {
                    case VUSB_TO_DEVICE | VUSB_REQ_STANDARD | VUSB_DIR_TO_HOST:
                    {
                        switch (pSetup->wValue >> 8)
                        {
                            case VUSB_DT_STRING:
                                Log(("usbHid: GET_DESCRIPTOR DT_STRING wValue=%#x wIndex=%#x\n", pSetup->wValue, pSetup->wIndex));
                                break;
                            default:
                                Log(("usbHid: GET_DESCRIPTOR, huh? wValue=%#x wIndex=%#x\n", pSetup->wValue, pSetup->wIndex));
                                break;
                        }
                        break;
                    }

                    case VUSB_TO_INTERFACE | VUSB_REQ_STANDARD | VUSB_DIR_TO_HOST:
                    {
                        switch (pSetup->wValue >> 8)
                        {
                            case DT_IF_HID_DESCRIPTOR:
                            {
                                uint32_t    cbCopy;

                                /* Returned data is written after the setup message. */
                                cbCopy = pUrb->cbData - sizeof(*pSetup);
                                cbCopy = RT_MIN(cbCopy, sizeof(g_UsbHidIfHidDesc));
                                Log(("usbHidKbd: GET_DESCRIPTOR DT_IF_HID_DESCRIPTOR wValue=%#x wIndex=%#x cbCopy=%#x\n", pSetup->wValue, pSetup->wIndex, cbCopy));
                                memcpy(&pUrb->abData[sizeof(*pSetup)], &g_UsbHidIfHidDesc, cbCopy);
                                return usbHidCompleteOk(pThis, pUrb, cbCopy + sizeof(*pSetup));
                            }

                            case DT_IF_HID_REPORT:
                            {
                                uint32_t    cbCopy;

                                /* Returned data is written after the setup message. */
                                cbCopy = pUrb->cbData - sizeof(*pSetup);
                                cbCopy = RT_MIN(cbCopy, sizeof(g_UsbHidReportDesc));
                                Log(("usbHid: GET_DESCRIPTOR DT_IF_HID_REPORT wValue=%#x wIndex=%#x cbCopy=%#x\n", pSetup->wValue, pSetup->wIndex, cbCopy));
                                memcpy(&pUrb->abData[sizeof(*pSetup)], &g_UsbHidReportDesc, cbCopy);
                                return usbHidCompleteOk(pThis, pUrb, cbCopy + sizeof(*pSetup));
                            }

                            default:
                                Log(("usbHid: GET_DESCRIPTOR, huh? wValue=%#x wIndex=%#x\n", pSetup->wValue, pSetup->wIndex));
                                break;
                        }
                        break;
                    }

                    default:
                        Log(("usbHid: Bad GET_DESCRIPTOR req: bmRequestType=%#x\n", pSetup->bmRequestType));
                        return usbHidCompleteStall(pThis, pEp, pUrb, "Bad GET_DESCRIPTOR");
                }
                break;
            }

            case VUSB_REQ_GET_STATUS:
            {
                uint16_t    wRet = 0;

                if (pSetup->wLength != 2)
                {
                    Log(("usbHid: Bad GET_STATUS req: wLength=%#x\n", pSetup->wLength));
                    break;
                }
                Assert(pSetup->wValue == 0);
                switch (pSetup->bmRequestType)
                {
                    case VUSB_TO_DEVICE | VUSB_REQ_STANDARD | VUSB_DIR_TO_HOST:
                    {
                        Assert(pSetup->wIndex == 0);
                        Log(("usbHid: GET_STATUS (device)\n"));
                        wRet = 0;   /* Not self-powered, no remote wakeup. */
                        memcpy(&pUrb->abData[sizeof(*pSetup)], &wRet, sizeof(wRet));
                        return usbHidCompleteOk(pThis, pUrb, sizeof(wRet) + sizeof(*pSetup));
                    }

                    case VUSB_TO_INTERFACE | VUSB_REQ_STANDARD | VUSB_DIR_TO_HOST:
                    {
                        if (pSetup->wIndex == 0)
                        {
                            memcpy(&pUrb->abData[sizeof(*pSetup)], &wRet, sizeof(wRet));
                            return usbHidCompleteOk(pThis, pUrb, sizeof(wRet) + sizeof(*pSetup));
                        }
                        else
                        {
                            Log(("usbHid: GET_STATUS (interface) invalid, wIndex=%#x\n", pSetup->wIndex));
                        }
                        break;
                    }

                    case VUSB_TO_ENDPOINT | VUSB_REQ_STANDARD | VUSB_DIR_TO_HOST:
                    {
                        if (pSetup->wIndex < RT_ELEMENTS(pThis->aEps))
                        {
                            wRet = pThis->aEps[pSetup->wIndex].fHalted ? 1 : 0;
                            memcpy(&pUrb->abData[sizeof(*pSetup)], &wRet, sizeof(wRet));
                            return usbHidCompleteOk(pThis, pUrb, sizeof(wRet) + sizeof(*pSetup));
                        }
                        else
                        {
                            Log(("usbHid: GET_STATUS (endpoint) invalid, wIndex=%#x\n", pSetup->wIndex));
                        }
                        break;
                    }

                    default:
                        Log(("usbHid: Bad GET_STATUS req: bmRequestType=%#x\n", pSetup->bmRequestType));
                        return usbHidCompleteStall(pThis, pEp, pUrb, "Bad GET_STATUS");
                }
                break;
            }

            case VUSB_REQ_CLEAR_FEATURE:
                break;
        }

        /** @todo implement this. */
        Log(("usbHid: Implement standard request: bmRequestType=%#x bRequest=%#x wValue=%#x wIndex=%#x wLength=%#x\n",
             pSetup->bmRequestType, pSetup->bRequest, pSetup->wValue, pSetup->wIndex, pSetup->wLength));

        usbHidCompleteStall(pThis, pEp, pUrb, "TODO: standard request stuff");
    }
    else if ((pSetup->bmRequestType & VUSB_REQ_MASK) == VUSB_REQ_CLASS)
    {
        switch (pSetup->bRequest)
        {
            case HID_REQ_SET_IDLE:
            {
                switch (pSetup->bmRequestType)
                {
                    case VUSB_TO_INTERFACE | VUSB_REQ_CLASS | VUSB_DIR_TO_DEVICE:
                    {
                        Log(("usbHid: SET_IDLE wValue=%#x wIndex=%#x\n", pSetup->wValue, pSetup->wIndex));
                        pThis->bIdle = pSetup->wValue >> 8;
                        /* Consider 24ms to mean zero for keyboards (see IOUSBHIDDriver) */
                        if (pThis->bIdle == 6) pThis->bIdle = 0;
                        return usbHidCompleteOk(pThis, pUrb, 0);
                    }
                    break;
                }
                break;
            }
            case HID_REQ_GET_IDLE:
            {
                switch (pSetup->bmRequestType)
                {
                    case VUSB_TO_INTERFACE | VUSB_REQ_CLASS | VUSB_DIR_TO_HOST:
                    {
                        Log(("usbHid: GET_IDLE wValue=%#x wIndex=%#x, returning %#x\n", pSetup->wValue, pSetup->wIndex, pThis->bIdle));
                        pUrb->abData[sizeof(*pSetup)] = pThis->bIdle;
                        return usbHidCompleteOk(pThis, pUrb, 1);
                    }
                    break;
                }
                break;
            }
            case HID_REQ_SET_REPORT:
            {
                switch (pSetup->bmRequestType)
                {
                    case VUSB_TO_INTERFACE | VUSB_REQ_CLASS | VUSB_DIR_TO_DEVICE:
                    {
                        Log(("usbHid: SET_REPORT wValue=%#x wIndex=%#x wLength=%#x\n", pSetup->wValue, pSetup->wIndex, pSetup->wLength));
                        usbHidSetReport(pThis, pUrb);
                        return usbHidCompleteOk(pThis, pUrb, 0);
                    }
                    break;
                }
                break;
            }
        }
        Log(("usbHid: Unimplemented class request: bmRequestType=%#x bRequest=%#x wValue=%#x wIndex=%#x wLength=%#x\n",
             pSetup->bmRequestType, pSetup->bRequest, pSetup->wValue, pSetup->wIndex, pSetup->wLength));

        usbHidCompleteStall(pThis, pEp, pUrb, "TODO: class request stuff");
    }
    else
    {
        Log(("usbHid: Unknown control msg: bmRequestType=%#x bRequest=%#x wValue=%#x wIndex=%#x wLength=%#x\n",
             pSetup->bmRequestType, pSetup->bRequest, pSetup->wValue, pSetup->wIndex, pSetup->wLength));
        return usbHidCompleteStall(pThis, pEp, pUrb, "Unknown control msg");
    }

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMUSBREG,pfnUrbQueue}
 */
static DECLCALLBACK(int) usbHidQueue(PPDMUSBINS pUsbIns, PVUSBURB pUrb)
{
    PUSBHID pThis = PDMINS_2_DATA(pUsbIns, PUSBHID);
    LogFlow(("usbHidQueue/#%u: pUrb=%p:%s EndPt=%#x\n", pUsbIns->iInstance, pUrb, pUrb->pszDesc, pUrb->EndPt));
    RTCritSectEnter(&pThis->CritSect);

    /*
     * Parse on a per end-point basis.
     */
    int rc;
    switch (pUrb->EndPt)
    {
        case 0:
            rc = usbHidHandleDefaultPipe(pThis, &pThis->aEps[0], pUrb);
            break;

        case 0x81:
            AssertFailed();
            RT_FALL_THRU();
        case 0x01:
            rc = usbHidHandleIntrDevToHost(pThis, &pThis->aEps[1], pUrb);
            break;

        default:
            AssertMsgFailed(("EndPt=%d\n", pUrb->EndPt));
            rc = VERR_VUSB_FAILED_TO_QUEUE_URB;
            break;
    }

    RTCritSectLeave(&pThis->CritSect);
    return rc;
}


/**
 * @interface_method_impl{PDMUSBREG,pfnUsbClearHaltedEndpoint}
 */
static DECLCALLBACK(int) usbHidUsbClearHaltedEndpoint(PPDMUSBINS pUsbIns, unsigned uEndpoint)
{
    PUSBHID pThis = PDMINS_2_DATA(pUsbIns, PUSBHID);
    LogFlow(("usbHidUsbClearHaltedEndpoint/#%u: uEndpoint=%#x\n", pUsbIns->iInstance, uEndpoint));

    if ((uEndpoint & ~0x80) < RT_ELEMENTS(pThis->aEps))
    {
        RTCritSectEnter(&pThis->CritSect);
        pThis->aEps[(uEndpoint & ~0x80)].fHalted = false;
        RTCritSectLeave(&pThis->CritSect);
    }

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMUSBREG,pfnUsbSetInterface}
 */
static DECLCALLBACK(int) usbHidUsbSetInterface(PPDMUSBINS pUsbIns, uint8_t bInterfaceNumber, uint8_t bAlternateSetting)
{
    RT_NOREF3(pUsbIns, bInterfaceNumber, bAlternateSetting);
    LogFlow(("usbHidUsbSetInterface/#%u: bInterfaceNumber=%u bAlternateSetting=%u\n", pUsbIns->iInstance, bInterfaceNumber, bAlternateSetting));
    Assert(bAlternateSetting == 0);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMUSBREG,pfnUsbSetConfiguration}
 */
static DECLCALLBACK(int) usbHidUsbSetConfiguration(PPDMUSBINS pUsbIns, uint8_t bConfigurationValue,
                                                   const void *pvOldCfgDesc, const void *pvOldIfState, const void *pvNewCfgDesc)
{
    RT_NOREF3(pvOldCfgDesc, pvOldIfState, pvNewCfgDesc);
    PUSBHID pThis = PDMINS_2_DATA(pUsbIns, PUSBHID);
    LogFlow(("usbHidUsbSetConfiguration/#%u: bConfigurationValue=%u\n", pUsbIns->iInstance, bConfigurationValue));
    Assert(bConfigurationValue == 1);
    RTCritSectEnter(&pThis->CritSect);

    /*
     * If the same config is applied more than once, it's a kind of reset.
     */
    if (pThis->bConfigurationValue == bConfigurationValue)
        usbHidResetWorker(pThis, NULL, true /*fSetConfig*/); /** @todo figure out the exact difference */
    pThis->bConfigurationValue = bConfigurationValue;

    /*
     * Tell the other end that the keyboard is now enabled and wants
     * to receive keystrokes.
     */
    pThis->Lun0.pDrv->pfnSetActive(pThis->Lun0.pDrv, true);

    RTCritSectLeave(&pThis->CritSect);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMUSBREG,pfnUsbGetDescriptorCache}
 */
static DECLCALLBACK(PCPDMUSBDESCCACHE) usbHidUsbGetDescriptorCache(PPDMUSBINS pUsbIns)
{
    PUSBHID pThis = PDMINS_2_DATA(pUsbIns, PUSBHID); RT_NOREF_PV(pThis);
    LogFlow(("usbHidUsbGetDescriptorCache/#%u:\n", pUsbIns->iInstance));
    return &g_UsbHidDescCache;
}


/**
 * @interface_method_impl{PDMUSBREG,pfnUsbReset}
 */
static DECLCALLBACK(int) usbHidUsbReset(PPDMUSBINS pUsbIns, bool fResetOnLinux)
{
    RT_NOREF1(fResetOnLinux);
    PUSBHID pThis = PDMINS_2_DATA(pUsbIns, PUSBHID);
    LogFlow(("usbHidUsbReset/#%u:\n", pUsbIns->iInstance));
    RTCritSectEnter(&pThis->CritSect);

    int rc = usbHidResetWorker(pThis, NULL, false /*fSetConfig*/);

    RTCritSectLeave(&pThis->CritSect);
    return rc;
}


/**
 * @interface_method_impl{PDMUSBREG,pfnDestruct}
 */
static DECLCALLBACK(void) usbHidDestruct(PPDMUSBINS pUsbIns)
{
    PDMUSB_CHECK_VERSIONS_RETURN_VOID(pUsbIns);
    PUSBHID pThis = PDMINS_2_DATA(pUsbIns, PUSBHID);
    LogFlow(("usbHidDestruct/#%u:\n", pUsbIns->iInstance));

    if (RTCritSectIsInitialized(&pThis->CritSect))
    {
        /* Let whoever runs in this critical section complete. */
        RTCritSectEnter(&pThis->CritSect);
        RTCritSectLeave(&pThis->CritSect);
        RTCritSectDelete(&pThis->CritSect);
    }

    if (pThis->hEvtDoneQueue != NIL_RTSEMEVENT)
    {
        RTSemEventDestroy(pThis->hEvtDoneQueue);
        pThis->hEvtDoneQueue = NIL_RTSEMEVENT;
    }
}


/**
 * @interface_method_impl{PDMUSBREG,pfnConstruct}
 */
static DECLCALLBACK(int) usbHidConstruct(PPDMUSBINS pUsbIns, int iInstance, PCFGMNODE pCfg, PCFGMNODE pCfgGlobal)
{
    RT_NOREF1(pCfgGlobal);
    PDMUSB_CHECK_VERSIONS_RETURN(pUsbIns);
    PUSBHID pThis = PDMINS_2_DATA(pUsbIns, PUSBHID);
    Log(("usbHidConstruct/#%u:\n", iInstance));

    /*
     * Perform the basic structure initialization first so the destructor
     * will not misbehave.
     */
    pThis->pUsbIns                                  = pUsbIns;
    pThis->hEvtDoneQueue                            = NIL_RTSEMEVENT;
    usbHidQueueInit(&pThis->ToHostQueue);
    usbHidQueueInit(&pThis->DoneQueue);

    int rc = RTCritSectInit(&pThis->CritSect);
    AssertRCReturn(rc, rc);

    rc = RTSemEventCreate(&pThis->hEvtDoneQueue);
    AssertRCReturn(rc, rc);

    /*
     * Validate and read the configuration.
     */
    rc = CFGMR3ValidateConfig(pCfg, "/", "", "", "UsbHid", iInstance);
    if (RT_FAILURE(rc))
        return rc;

    pThis->Lun0.IBase.pfnQueryInterface = usbHidKeyboardQueryInterface;
    pThis->Lun0.IPort.pfnPutEventHid    = usbHidKeyboardPutEvent;

    /*
     * Attach the keyboard driver.
     */
    rc = PDMUsbHlpDriverAttach(pUsbIns, 0 /*iLun*/, &pThis->Lun0.IBase, &pThis->Lun0.pDrvBase, "Keyboard Port");
    if (RT_FAILURE(rc))
        return PDMUsbHlpVMSetError(pUsbIns, rc, RT_SRC_POS, N_("HID failed to attach keyboard driver"));

    pThis->Lun0.pDrv = PDMIBASE_QUERY_INTERFACE(pThis->Lun0.pDrvBase, PDMIKEYBOARDCONNECTOR);
    if (!pThis->Lun0.pDrv)
        return PDMUsbHlpVMSetError(pUsbIns, VERR_PDM_MISSING_INTERFACE, RT_SRC_POS, N_("HID failed to query keyboard interface"));

    return VINF_SUCCESS;
}


/**
 * The USB Human Interface Device (HID) Keyboard registration record.
 */
const PDMUSBREG g_UsbHidKbd =
{
    /* u32Version */
    PDM_USBREG_VERSION,
    /* szName */
    "HidKeyboard",
    /* pszDescription */
    "USB HID Keyboard.",
    /* fFlags */
    0,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(USBHID),
    /* pfnConstruct */
    usbHidConstruct,
    /* pfnDestruct */
    usbHidDestruct,
    /* pfnVMInitComplete */
    NULL,
    /* pfnVMPowerOn */
    NULL,
    /* pfnVMReset */
    NULL,
    /* pfnVMSuspend */
    NULL,
    /* pfnVMResume */
    NULL,
    /* pfnVMPowerOff */
    NULL,
    /* pfnHotPlugged */
    NULL,
    /* pfnHotUnplugged */
    NULL,
    /* pfnDriverAttach */
    NULL,
    /* pfnDriverDetach */
    NULL,
    /* pfnQueryInterface */
    NULL,
    /* pfnUsbReset */
    usbHidUsbReset,
    /* pfnUsbGetDescriptorCache */
    usbHidUsbGetDescriptorCache,
    /* pfnUsbSetConfiguration */
    usbHidUsbSetConfiguration,
    /* pfnUsbSetInterface */
    usbHidUsbSetInterface,
    /* pfnUsbClearHaltedEndpoint */
    usbHidUsbClearHaltedEndpoint,
    /* pfnUrbNew */
    NULL/*usbHidUrbNew*/,
    /* pfnUrbQueue */
    usbHidQueue,
    /* pfnUrbCancel */
    usbHidUrbCancel,
    /* pfnUrbReap */
    usbHidUrbReap,
    /* pfnWakeup */
    usbHidWakeup,
    /* u32TheEnd */
    PDM_USBREG_VERSION
};
