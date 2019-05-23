/* $Id: DevLsiLogicSCSI.cpp $ */
/** @file
 * DevLsiLogicSCSI - LsiLogic LSI53c1030 SCSI controller.
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
#define LOG_GROUP LOG_GROUP_DEV_LSILOGICSCSI
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmstorageifs.h>
#include <VBox/vmm/pdmqueue.h>
#include <VBox/vmm/pdmthread.h>
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/scsi.h>
#include <VBox/sup.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/string.h>
#include <iprt/list.h>
#ifdef IN_RING3
# include <iprt/memcache.h>
# include <iprt/mem.h>
# include <iprt/param.h>
# include <iprt/uuid.h>
# include <iprt/time.h>
#endif

#include "DevLsiLogicSCSI.h"
#include "VBoxSCSI.h"

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The current saved state version. */
#define LSILOGIC_SAVED_STATE_VERSION                5
/** The saved state version used by VirtualBox before the diagnostic
 * memory access was implemented. */
#define LSILOGIC_SAVED_STATE_VERSION_PRE_DIAG_MEM   4
/** The saved state version used by VirtualBox before the doorbell status flag
 * was changed from bool to a 32bit enum. */
#define LSILOGIC_SAVED_STATE_VERSION_BOOL_DOORBELL  3
/** The saved state version used by VirtualBox before SAS support was added. */
#define LSILOGIC_SAVED_STATE_VERSION_PRE_SAS        2
/** The saved state version used by VirtualBox 3.0 and earlier.  It does not
 * include the device config part. */
#define LSILOGIC_SAVED_STATE_VERSION_VBOX_30        1

/** Maximum number of entries in the release log. */
#define MAX_REL_LOG_ERRORS 1024

#define LSILOGIC_RTGCPHYS_FROM_U32(Hi, Lo)         ( (RTGCPHYS)RT_MAKE_U64(Lo, Hi) )

/** Upper number a buffer is freed if it was too big before. */
#define LSILOGIC_MAX_ALLOC_TOO_MUCH 20

/** Maximum size of the memory regions (prevents teh guest from DOSing the host by
 * allocating loadds of memory). */
#define LSILOGIC_MEMORY_REGIONS_MAX (_1M)


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/** Pointer to the device instance data of the LsiLogic emulation. */
typedef struct LSILOGICSCSI *PLSILOGICSCSI;

#ifdef IN_RING3
/**
 * Memory buffer callback.
 *
 * @returns nothing.
 * @param   pThis    The LsiLogic controller instance.
 * @param   GCPhys   The guest physical address of the memory buffer.
 * @param   pSgBuf   The pointer to the host R3 S/G buffer.
 * @param   cbCopy   How many bytes to copy between the two buffers.
 * @param   pcbSkip  Initially contains the amount of bytes to skip
 *                   starting from the guest physical address before
 *                   accessing the S/G buffer and start copying data.
 *                   On return this contains the remaining amount if
 *                   cbCopy < *pcbSkip or 0 otherwise.
 */
typedef DECLCALLBACK(void) LSILOGICR3MEMCOPYCALLBACK(PLSILOGICSCSI pThis, RTGCPHYS GCPhys, PRTSGBUF pSgBuf, size_t cbCopy,
                                                     size_t *pcbSkip);
/** Pointer to a memory copy buffer callback. */
typedef LSILOGICR3MEMCOPYCALLBACK *PLSILOGICR3MEMCOPYCALLBACK;
#endif

/**
 * Reply data.
 */
typedef struct LSILOGICSCSIREPLY
{
    /** Lower 32 bits of the reply address in memory. */
    uint32_t      u32HostMFALowAddress;
    /** Full address of the reply in guest memory. */
    RTGCPHYS      GCPhysReplyAddress;
    /** Size of the reply. */
    uint32_t      cbReply;
    /** Different views to the reply depending on the request type. */
    MptReplyUnion Reply;
} LSILOGICSCSIREPLY;
/** Pointer to reply data. */
typedef LSILOGICSCSIREPLY *PLSILOGICSCSIREPLY;

/**
 * Memory region of the IOC.
 */
typedef struct LSILOGICMEMREGN
{
    /** List node. */
    RTLISTNODE    NodeList;
    /** 32bit address the region starts to describe. */
    uint32_t      u32AddrStart;
    /** 32bit address the region ends (inclusive). */
    uint32_t      u32AddrEnd;
    /** Data for this region - variable. */
    uint32_t      au32Data[1];
} LSILOGICMEMREGN;
/** Pointer to a memory region. */
typedef LSILOGICMEMREGN *PLSILOGICMEMREGN;

/**
 * State of a device attached to the buslogic host adapter.
 *
 * @implements  PDMIBASE
 * @implements  PDMISCSIPORT
 * @implements  PDMILEDPORTS
 */
typedef struct LSILOGICDEVICE
{
    /** Pointer to the owning lsilogic device instance. - R3 pointer */
    R3PTRTYPE(PLSILOGICSCSI)      pLsiLogicR3;

    /** LUN of the device. */
    uint32_t                      iLUN;
    /** Number of outstanding tasks on the port. */
    volatile uint32_t             cOutstandingRequests;

#if HC_ARCH_BITS == 64
    uint32_t                      Alignment0;
#endif

    /** Our base interface. */
    PDMIBASE                      IBase;
    /** Media port interface. */
    PDMIMEDIAPORT                 IMediaPort;
    /** Extended media port interface. */
    PDMIMEDIAEXPORT               IMediaExPort;
    /** Led interface. */
    PDMILEDPORTS                  ILed;
    /** Pointer to the attached driver's base interface. */
    R3PTRTYPE(PPDMIBASE)          pDrvBase;
    /** Pointer to the attached driver's media interface. */
    R3PTRTYPE(PPDMIMEDIA)         pDrvMedia;
    /** Pointer to the attached driver's extended media interface. */
    R3PTRTYPE(PPDMIMEDIAEX)       pDrvMediaEx;
    /** The status LED state for this device. */
    PDMLED                        Led;

} LSILOGICDEVICE;
/** Pointer to a device state. */
typedef LSILOGICDEVICE *PLSILOGICDEVICE;

/** Pointer to a task state. */
typedef struct LSILOGICREQ *PLSILOGICREQ;

/**
 * Device instance data for the emulated SCSI controller.
 */
typedef struct LSILOGICSCSI
{
    /** PCI device structure. */
    PDMPCIDEV            PciDev;
    /** Pointer to the device instance. - R3 ptr. */
    PPDMDEVINSR3         pDevInsR3;
    /** Pointer to the device instance. - R0 ptr. */
    PPDMDEVINSR0         pDevInsR0;
    /** Pointer to the device instance. - RC ptr. */
    PPDMDEVINSRC         pDevInsRC;

    /** Flag whether the GC part of the device is enabled. */
    bool                 fGCEnabled;
    /** Flag whether the R0 part of the device is enabled. */
    bool                 fR0Enabled;

    /** The state the controller is currently in. */
    LSILOGICSTATE        enmState;
    /** Who needs to init the driver to get into operational state. */
    LSILOGICWHOINIT      enmWhoInit;
    /** Flag whether we are in doorbell function. */
    LSILOGICDOORBELLSTATE enmDoorbellState;
    /** Flag whether diagnostic access is enabled. */
    bool                 fDiagnosticEnabled;
    /** Flag whether a notification was send to R3. */
    bool                 fNotificationSent;
    /** Flag whether the guest enabled event notification from the IOC. */
    bool                 fEventNotificationEnabled;
    /** Flag whether the diagnostic address and RW registers are enabled. */
    bool                 fDiagRegsEnabled;

    /** Queue to send tasks to R3. - R3 ptr */
    R3PTRTYPE(PPDMQUEUE) pNotificationQueueR3;
    /** Queue to send tasks to R3. - R0 ptr */
    R0PTRTYPE(PPDMQUEUE) pNotificationQueueR0;
    /** Queue to send tasks to R3. - RC ptr */
    RCPTRTYPE(PPDMQUEUE) pNotificationQueueRC;

    /** Number of device states allocated. */
    uint32_t             cDeviceStates;

    /** States for attached devices. */
    R3PTRTYPE(PLSILOGICDEVICE) paDeviceStates;
#if HC_ARCH_BITS == 32
    RTR3PTR                 R3PtrPadding0;
#endif

    /** Interrupt mask. */
    volatile uint32_t     uInterruptMask;
    /** Interrupt status register. */
    volatile uint32_t     uInterruptStatus;

    /** Buffer for messages which are passed through the doorbell using the
     * handshake method. */
    uint32_t              aMessage[sizeof(MptConfigurationRequest)]; /** @todo r=bird: Looks like 4 tims the required size? Please explain in comment if this correct... */
    /** Actual position in the buffer. */
    uint32_t              iMessage;
    /** Size of the message which is given in the doorbell message in dwords. */
    uint32_t              cMessage;

    /** Reply buffer.
     * @note 60 bytes  */
    MptReplyUnion         ReplyBuffer;
    /** Next entry to read. */
    uint32_t              uNextReplyEntryRead;
    /** Size of the reply in the buffer in 16bit words. */
    uint32_t              cReplySize;

    /** The fault code of the I/O controller if we are in the fault state. */
    uint16_t              u16IOCFaultCode;

    /** I/O port address the device is mapped to. */
    RTIOPORT              IOPortBase;
    /** MMIO address the device is mapped to. */
    RTGCPHYS              GCPhysMMIOBase;

    /** Upper 32 bits of the message frame address to locate requests in guest memory. */
    uint32_t              u32HostMFAHighAddr;
    /** Upper 32 bits of the sense buffer address. */
    uint32_t              u32SenseBufferHighAddr;
    /** Maximum number of devices the driver reported he can handle. */
    uint8_t               cMaxDevices;
    /** Maximum number of buses the driver reported he can handle. */
    uint8_t               cMaxBuses;
    /** Current size of reply message frames in the guest. */
    uint16_t              cbReplyFrame;

    /** Next key to write in the sequence to get access
     *  to diagnostic memory. */
    uint32_t              iDiagnosticAccess;

    /** Number entries allocated for the reply queue. */
    uint32_t              cReplyQueueEntries;
    /** Number entries allocated for the outstanding request queue. */
    uint32_t              cRequestQueueEntries;


    /** Critical section protecting the reply post queue. */
    PDMCRITSECT           ReplyPostQueueCritSect;
    /** Critical section protecting the reply free queue. */
    PDMCRITSECT           ReplyFreeQueueCritSect;
    /** Critical section protecting the request queue against
     * concurrent access from the guest. */
    PDMCRITSECT           RequestQueueCritSect;
    /** Critical section protecting the reply free queue against
     * concurrent write access from the guest. */
    PDMCRITSECT           ReplyFreeQueueWriteCritSect;

    /** Pointer to the start of the reply free queue - R3. */
    R3PTRTYPE(volatile uint32_t *) pReplyFreeQueueBaseR3;
    /** Pointer to the start of the reply post queue - R3. */
    R3PTRTYPE(volatile uint32_t *) pReplyPostQueueBaseR3;
    /** Pointer to the start of the request queue - R3. */
    R3PTRTYPE(volatile uint32_t *) pRequestQueueBaseR3;

    /** Pointer to the start of the reply queue - R0. */
    R0PTRTYPE(volatile uint32_t *) pReplyFreeQueueBaseR0;
    /** Pointer to the start of the reply queue - R0. */
    R0PTRTYPE(volatile uint32_t *) pReplyPostQueueBaseR0;
    /** Pointer to the start of the request queue - R0. */
    R0PTRTYPE(volatile uint32_t *) pRequestQueueBaseR0;

    /** Pointer to the start of the reply queue - RC. */
    RCPTRTYPE(volatile uint32_t *) pReplyFreeQueueBaseRC;
    /** Pointer to the start of the reply queue - RC. */
    RCPTRTYPE(volatile uint32_t *) pReplyPostQueueBaseRC;
    /** Pointer to the start of the request queue - RC. */
    RCPTRTYPE(volatile uint32_t *) pRequestQueueBaseRC;
    /** End these RC pointers on a 64-bit boundrary. */
    RTRCPTR                        RCPtrPadding1;

    /** Next free entry in the reply queue the guest can write a address to. */
    volatile uint32_t              uReplyFreeQueueNextEntryFreeWrite;
    /** Next valid entry the controller can read a valid address for reply frames from. */
    volatile uint32_t              uReplyFreeQueueNextAddressRead;

    /** Next free entry in the reply queue the guest can write a address to. */
    volatile uint32_t              uReplyPostQueueNextEntryFreeWrite;
    /** Next valid entry the controller can read a valid address for reply frames from. */
    volatile uint32_t              uReplyPostQueueNextAddressRead;

    /** Next free entry the guest can write a address to a request frame to. */
    volatile uint32_t              uRequestQueueNextEntryFreeWrite;
    /** Next valid entry the controller can read a valid address for request frames from. */
    volatile uint32_t              uRequestQueueNextAddressRead;

    /** Emulated controller type */
    LSILOGICCTRLTYPE               enmCtrlType;
    /** Handle counter */
    uint16_t                       u16NextHandle;

    /** Number of ports this controller has. */
    uint8_t                        cPorts;

    /** BIOS emulation. */
    VBOXSCSI                       VBoxSCSI;

    /** Status LUN: The base interface. */
    PDMIBASE                       IBase;
    /** Status LUN: Leds interface. */
    PDMILEDPORTS                   ILeds;
    /** Status LUN: Partner of ILeds. */
    R3PTRTYPE(PPDMILEDCONNECTORS)  pLedsConnector;
    /** Status LUN: Media Notifys. */
    R3PTRTYPE(PPDMIMEDIANOTIFY)    pMediaNotify;
    /** Pointer to the configuration page area. */
    R3PTRTYPE(PMptConfigurationPagesSupported) pConfigurationPages;

    /** Indicates that PDMDevHlpAsyncNotificationCompleted should be called when
     * a port is entering the idle state. */
    bool volatile                    fSignalIdle;
    /** Flag whether we have tasks which need to be processed again- */
    bool volatile                    fRedo;
    /** Flag whether the worker thread is sleeping. */
    volatile bool                    fWrkThreadSleeping;
    /** Flag whether a request from the BIOS is pending which the
     * worker thread needs to process. */
    volatile bool                    fBiosReqPending;
#if HC_ARCH_BITS == 64
    /** Alignment padding. */
    bool                             afPadding2[4];
#endif
    /** List of tasks which can be redone. */
    R3PTRTYPE(volatile PLSILOGICREQ) pTasksRedoHead;

    /** Current address to read from or write to in the diagnostic memory region. */
    uint32_t                         u32DiagMemAddr;
    /** Current size of the memory regions. */
    uint32_t                         cbMemRegns;

#if HC_ARCH_BITS ==32
    uint32_t                         u32Padding3;
#endif

    union
    {
        /** List of memory regions - PLSILOGICMEMREGN. */
        RTLISTANCHOR                 ListMemRegns;
        uint8_t                      u8Padding[2 * sizeof(RTUINTPTR)];
    };

    /** The support driver session handle. */
    R3R0PTRTYPE(PSUPDRVSESSION)      pSupDrvSession;
    /** Worker thread. */
    R3PTRTYPE(PPDMTHREAD)            pThreadWrk;
    /** The event semaphore the processing thread waits on. */
    SUPSEMEVENT                      hEvtProcess;

} LSILOGISCSI;

/**
 * Task state object which holds all necessary data while
 * processing the request from the guest.
 */
typedef struct LSILOGICREQ
{
    /** I/O request handle. */
    PDMMEDIAEXIOREQ            hIoReq;
    /** Next in the redo list. */
    PLSILOGICREQ               pRedoNext;
    /** Target device. */
    PLSILOGICDEVICE            pTargetDevice;
    /** The message request from the guest. */
    MptRequestUnion            GuestRequest;
    /** Address of the message request frame in guests memory.
     *  Used to read the S/G entries in the second step. */
    RTGCPHYS                   GCPhysMessageFrameAddr;
    /** Physical start address of the S/G list. */
    RTGCPHYS                   GCPhysSgStart;
    /** Chain offset */
    uint32_t                   cChainOffset;
    /** Pointer to the sense buffer. */
    uint8_t                    abSenseBuffer[18];
    /** Flag whether the request was issued from the BIOS. */
    bool                       fBIOS;
    /** SCSI status code. */
    uint8_t                    u8ScsiSts;
} LSILOGICREQ;


#ifndef VBOX_DEVICE_STRUCT_TESTCASE


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
#ifdef IN_RING3
static void lsilogicR3InitializeConfigurationPages(PLSILOGICSCSI pThis);
static void lsilogicR3ConfigurationPagesFree(PLSILOGICSCSI pThis);
static int  lsilogicR3ProcessConfigurationRequest(PLSILOGICSCSI pThis, PMptConfigurationRequest pConfigurationReq,
                                                  PMptConfigurationReply pReply);
#endif
RT_C_DECLS_END


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Key sequence the guest has to write to enable access
 * to diagnostic memory. */
static const uint8_t g_lsilogicDiagnosticAccess[] = {0x04, 0x0b, 0x02, 0x07, 0x0d};

/**
 * Updates the status of the interrupt pin of the device.
 *
 * @returns nothing.
 * @param   pThis       Pointer to the LsiLogic device state.
 */
static void lsilogicUpdateInterrupt(PLSILOGICSCSI pThis)
{
    uint32_t uIntSts;

    LogFlowFunc(("Updating interrupts\n"));

    /* Mask out doorbell status so that it does not affect interrupt updating. */
    uIntSts = (ASMAtomicReadU32(&pThis->uInterruptStatus) & ~LSILOGIC_REG_HOST_INTR_STATUS_DOORBELL_STS);
    /* Check maskable interrupts. */
    uIntSts &= ~(ASMAtomicReadU32(&pThis->uInterruptMask) & ~LSILOGIC_REG_HOST_INTR_MASK_IRQ_ROUTING);

    if (uIntSts)
    {
        LogFlowFunc(("Setting interrupt\n"));
        PDMDevHlpPCISetIrq(pThis->CTX_SUFF(pDevIns), 0, 1);
    }
    else
    {
        LogFlowFunc(("Clearing interrupt\n"));
        PDMDevHlpPCISetIrq(pThis->CTX_SUFF(pDevIns), 0, 0);
    }
}

/**
 * Sets a given interrupt status bit in the status register and
 * updates the interrupt status.
 *
 * @returns nothing.
 * @param   pThis       Pointer to the LsiLogic device state.
 * @param   uStatus     The status bit to set.
 */
DECLINLINE(void) lsilogicSetInterrupt(PLSILOGICSCSI pThis, uint32_t uStatus)
{
    ASMAtomicOrU32(&pThis->uInterruptStatus, uStatus);
    lsilogicUpdateInterrupt(pThis);
}

/**
 * Clears a given interrupt status bit in the status register and
 * updates the interrupt status.
 *
 * @returns nothing.
 * @param   pThis       Pointer to the LsiLogic device state.
 * @param   uStatus     The status bit to set.
 */
DECLINLINE(void) lsilogicClearInterrupt(PLSILOGICSCSI pThis, uint32_t uStatus)
{
    ASMAtomicAndU32(&pThis->uInterruptStatus, ~uStatus);
    lsilogicUpdateInterrupt(pThis);
}


#ifdef IN_RING3
/**
 * Sets the I/O controller into fault state and sets the fault code.
 *
 * @returns nothing
 * @param   pThis           Pointer to the LsiLogic device state.
 * @param   uIOCFaultCode   Fault code to set.
 */
DECLINLINE(void) lsilogicSetIOCFaultCode(PLSILOGICSCSI pThis, uint16_t uIOCFaultCode)
{
    if (pThis->enmState != LSILOGICSTATE_FAULT)
    {
        LogFunc(("Setting I/O controller into FAULT state: uIOCFaultCode=%u\n", uIOCFaultCode));
        pThis->enmState        = LSILOGICSTATE_FAULT;
        pThis->u16IOCFaultCode = uIOCFaultCode;
    }
    else
        LogFunc(("We are already in FAULT state\n"));
}
#endif /* IN_RING3 */


/**
 * Returns the number of frames in the reply free queue.
 *
 * @returns Number of frames in the reply free queue.
 * @param   pThis    Pointer to the LsiLogic device state.
 */
DECLINLINE(uint32_t) lsilogicReplyFreeQueueGetFrameCount(PLSILOGICSCSI pThis)
{
    uint32_t cReplyFrames = 0;

    if (pThis->uReplyFreeQueueNextAddressRead <= pThis->uReplyFreeQueueNextEntryFreeWrite)
        cReplyFrames = pThis->uReplyFreeQueueNextEntryFreeWrite - pThis->uReplyFreeQueueNextAddressRead;
    else
        cReplyFrames = pThis->cReplyQueueEntries - pThis->uReplyFreeQueueNextAddressRead + pThis->uReplyFreeQueueNextEntryFreeWrite;

    return cReplyFrames;
}

#ifdef IN_RING3

/**
 * Returns the number of free entries in the reply post queue.
 *
 * @returns Number of frames in the reply free queue.
 * @param   pThis    Pointer to the LsiLogic device state.
 */
DECLINLINE(uint32_t) lsilogicReplyPostQueueGetFrameCount(PLSILOGICSCSI pThis)
{
    uint32_t cReplyFrames = 0;

    if (pThis->uReplyPostQueueNextAddressRead <= pThis->uReplyPostQueueNextEntryFreeWrite)
        cReplyFrames = pThis->cReplyQueueEntries - pThis->uReplyPostQueueNextEntryFreeWrite + pThis->uReplyPostQueueNextAddressRead;
    else
        cReplyFrames = pThis->uReplyPostQueueNextEntryFreeWrite - pThis->uReplyPostQueueNextAddressRead;

    return cReplyFrames;
}


/**
 * Performs a hard reset on the controller.
 *
 * @returns VBox status code.
 * @param   pThis       Pointer to the LsiLogic device state.
 */
static int lsilogicR3HardReset(PLSILOGICSCSI pThis)
{
    pThis->enmState = LSILOGICSTATE_RESET;
    pThis->enmDoorbellState = LSILOGICDOORBELLSTATE_NOT_IN_USE;

    /* The interrupts are masked out. */
    pThis->uInterruptMask |= LSILOGIC_REG_HOST_INTR_MASK_DOORBELL
                           | LSILOGIC_REG_HOST_INTR_MASK_REPLY;
    /* Reset interrupt states. */
    pThis->uInterruptStatus = 0;
    lsilogicUpdateInterrupt(pThis);

    /* Reset the queues. */
    pThis->uReplyFreeQueueNextEntryFreeWrite = 0;
    pThis->uReplyFreeQueueNextAddressRead    = 0;
    pThis->uReplyPostQueueNextEntryFreeWrite = 0;
    pThis->uReplyPostQueueNextAddressRead    = 0;
    pThis->uRequestQueueNextEntryFreeWrite   = 0;
    pThis->uRequestQueueNextAddressRead      = 0;

    /* Disable diagnostic access. */
    pThis->iDiagnosticAccess  = 0;
    pThis->fDiagnosticEnabled = false;
    pThis->fDiagRegsEnabled   = false;

    /* Set default values. */
    pThis->cMaxDevices    = pThis->cDeviceStates;
    pThis->cMaxBuses      = 1;
    pThis->cbReplyFrame   = 128; /** @todo Figure out where it is needed. */
    pThis->u16NextHandle  = 1;
    pThis->u32DiagMemAddr = 0;

    lsilogicR3ConfigurationPagesFree(pThis);
    lsilogicR3InitializeConfigurationPages(pThis);

    /* Mark that we finished performing the reset. */
    pThis->enmState = LSILOGICSTATE_READY;
    return VINF_SUCCESS;
}

/**
 * Frees the configuration pages if allocated.
 *
 * @returns nothing.
 * @param pThis    The LsiLogic controller instance
 */
static void lsilogicR3ConfigurationPagesFree(PLSILOGICSCSI pThis)
{

    if (pThis->pConfigurationPages)
    {
        /* Destroy device list if we emulate a SAS controller. */
        if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
        {
            PMptConfigurationPagesSas pSasPages = &pThis->pConfigurationPages->u.SasPages;
            PMptSASDevice pSASDeviceCurr = pSasPages->pSASDeviceHead;

            while (pSASDeviceCurr)
            {
                PMptSASDevice pFree = pSASDeviceCurr;

                pSASDeviceCurr = pSASDeviceCurr->pNext;
                RTMemFree(pFree);
            }
            if (pSasPages->paPHYs)
                RTMemFree(pSasPages->paPHYs);
            if (pSasPages->pManufacturingPage7)
                RTMemFree(pSasPages->pManufacturingPage7);
            if (pSasPages->pSASIOUnitPage0)
                RTMemFree(pSasPages->pSASIOUnitPage0);
            if (pSasPages->pSASIOUnitPage1)
                RTMemFree(pSasPages->pSASIOUnitPage1);
        }

        RTMemFree(pThis->pConfigurationPages);
    }
}

/**
 * Finishes a context reply.
 *
 * @returns nothing
 * @param   pThis               Pointer to the LsiLogic device state.
 * @param   u32MessageContext   The message context ID to post.
 */
static void lsilogicR3FinishContextReply(PLSILOGICSCSI pThis, uint32_t u32MessageContext)
{
    int rc;

    LogFlowFunc(("pThis=%#p u32MessageContext=%#x\n", pThis, u32MessageContext));

    AssertMsg(pThis->enmDoorbellState == LSILOGICDOORBELLSTATE_NOT_IN_USE, ("We are in a doorbell function\n"));

    /* Write message context ID into reply post queue. */
    rc = PDMCritSectEnter(&pThis->ReplyPostQueueCritSect, VINF_SUCCESS);
    AssertRC(rc);

    /* Check for a entry in the queue. */
    if (!lsilogicReplyPostQueueGetFrameCount(pThis))
    {
        /* Set error code. */
        lsilogicSetIOCFaultCode(pThis, LSILOGIC_IOCSTATUS_INSUFFICIENT_RESOURCES);
        PDMCritSectLeave(&pThis->ReplyPostQueueCritSect);
        return;
    }

    /* We have a context reply. */
    ASMAtomicWriteU32(&pThis->CTX_SUFF(pReplyPostQueueBase)[pThis->uReplyPostQueueNextEntryFreeWrite], u32MessageContext);
    ASMAtomicIncU32(&pThis->uReplyPostQueueNextEntryFreeWrite);
    pThis->uReplyPostQueueNextEntryFreeWrite %= pThis->cReplyQueueEntries;

    /* Set interrupt. */
    lsilogicSetInterrupt(pThis, LSILOGIC_REG_HOST_INTR_STATUS_REPLY_INTR);

    PDMCritSectLeave(&pThis->ReplyPostQueueCritSect);
}


/**
 * Takes necessary steps to finish a reply frame.
 *
 * @returns nothing
 * @param   pThis           Pointer to the LsiLogic device state.
 * @param   pReply          Pointer to the reply message.
 * @param   fForceReplyFifo Flag whether the use of the reply post fifo is forced.
 */
static void lsilogicFinishAddressReply(PLSILOGICSCSI pThis, PMptReplyUnion pReply, bool fForceReplyFifo)
{
    /*
     * If we are in a doorbell function we set the reply size now and
     * set the system doorbell status interrupt to notify the guest that
     * we are ready to send the reply.
     */
    if (pThis->enmDoorbellState != LSILOGICDOORBELLSTATE_NOT_IN_USE && !fForceReplyFifo)
    {
        /* Set size of the reply in 16bit words. The size in the reply is in 32bit dwords. */
        pThis->cReplySize = pReply->Header.u8MessageLength * 2;
        Log(("%s: cReplySize=%u\n", __FUNCTION__, pThis->cReplySize));
        pThis->uNextReplyEntryRead = 0;
        lsilogicSetInterrupt(pThis, LSILOGIC_REG_HOST_INTR_STATUS_SYSTEM_DOORBELL);
    }
    else
    {
        /*
         * The reply queues are only used if the request was fetched from the request queue.
         * Requests from the request queue are always transferred to R3. So it is not possible
         * that this case happens in R0 or GC.
         */
# ifdef IN_RING3
        int rc;
        /* Grab a free reply message from the queue. */
        rc = PDMCritSectEnter(&pThis->ReplyFreeQueueCritSect, VINF_SUCCESS);
        AssertRC(rc);

        /* Check for a free reply frame. */
        if (!lsilogicReplyFreeQueueGetFrameCount(pThis))
        {
            /* Set error code. */
            lsilogicSetIOCFaultCode(pThis, LSILOGIC_IOCSTATUS_INSUFFICIENT_RESOURCES);
            PDMCritSectLeave(&pThis->ReplyFreeQueueCritSect);
            return;
        }

        uint32_t u32ReplyFrameAddressLow = pThis->CTX_SUFF(pReplyFreeQueueBase)[pThis->uReplyFreeQueueNextAddressRead];

        pThis->uReplyFreeQueueNextAddressRead++;
        pThis->uReplyFreeQueueNextAddressRead %= pThis->cReplyQueueEntries;

        PDMCritSectLeave(&pThis->ReplyFreeQueueCritSect);

        /* Build 64bit physical address. */
        RTGCPHYS GCPhysReplyMessage = LSILOGIC_RTGCPHYS_FROM_U32(pThis->u32HostMFAHighAddr, u32ReplyFrameAddressLow);
        size_t cbReplyCopied = (pThis->cbReplyFrame < sizeof(MptReplyUnion)) ? pThis->cbReplyFrame : sizeof(MptReplyUnion);

        /* Write reply to guest memory. */
        PDMDevHlpPCIPhysWrite(pThis->CTX_SUFF(pDevIns), GCPhysReplyMessage, pReply, cbReplyCopied);

        /* Write low 32bits of reply frame into post reply queue. */
        rc = PDMCritSectEnter(&pThis->ReplyPostQueueCritSect, VINF_SUCCESS);
        AssertRC(rc);

        /* Check for a entry in the queue. */
        if (!lsilogicReplyPostQueueGetFrameCount(pThis))
        {
            /* Set error code. */
            lsilogicSetIOCFaultCode(pThis, LSILOGIC_IOCSTATUS_INSUFFICIENT_RESOURCES);
            PDMCritSectLeave(&pThis->ReplyPostQueueCritSect);
            return;
        }

        /* We have a address reply. Set the 31th bit to indicate that. */
        ASMAtomicWriteU32(&pThis->CTX_SUFF(pReplyPostQueueBase)[pThis->uReplyPostQueueNextEntryFreeWrite],
                          RT_BIT(31) | (u32ReplyFrameAddressLow >> 1));
        ASMAtomicIncU32(&pThis->uReplyPostQueueNextEntryFreeWrite);
        pThis->uReplyPostQueueNextEntryFreeWrite %= pThis->cReplyQueueEntries;

        if (fForceReplyFifo)
        {
            pThis->enmDoorbellState = LSILOGICDOORBELLSTATE_NOT_IN_USE;
            lsilogicSetInterrupt(pThis, LSILOGIC_REG_HOST_INTR_STATUS_SYSTEM_DOORBELL);
        }

        /* Set interrupt. */
        lsilogicSetInterrupt(pThis, LSILOGIC_REG_HOST_INTR_STATUS_REPLY_INTR);

        PDMCritSectLeave(&pThis->ReplyPostQueueCritSect);
# else
        AssertMsgFailed(("This is not allowed to happen.\n"));
# endif
    }
}


/**
 * Tries to find a memory region which covers the given address.
 *
 * @returns Pointer to memory region or NULL if not found.
 * @param   pThis           Pointer to the LsiLogic device state.
 * @param   u32Addr         The 32bit address to search for.
 */
static PLSILOGICMEMREGN lsilogicR3MemRegionFindByAddr(PLSILOGICSCSI pThis, uint32_t u32Addr)
{
    PLSILOGICMEMREGN pRegion = NULL;

    PLSILOGICMEMREGN pIt;
    RTListForEach(&pThis->ListMemRegns, pIt, LSILOGICMEMREGN, NodeList)
    {
        if (   u32Addr >= pIt->u32AddrStart
            && u32Addr <= pIt->u32AddrEnd)
        {
            pRegion = pIt;
            break;
        }
    }

    return pRegion;
}

/**
 * Frees all allocated memory regions.
 *
 * @returns nothing.
 * @param   pThis           Pointer to the LsiLogic device state.
 */
static void lsilogicR3MemRegionsFree(PLSILOGICSCSI pThis)
{
    PLSILOGICMEMREGN pItNext;

    PLSILOGICMEMREGN pIt;
    RTListForEachSafe(&pThis->ListMemRegns, pIt, pItNext, LSILOGICMEMREGN, NodeList)
    {
        RTListNodeRemove(&pIt->NodeList);
        RTMemFree(pIt);
    }
    pThis->cbMemRegns = 0;
}

/**
 * Inserts a given memory region into the list.
 *
 * @returns nothing.
 * @param   pThis           Pointer to the LsiLogic device state.
 * @param   pRegion         The region to insert.
 */
static void lsilogicR3MemRegionInsert(PLSILOGICSCSI pThis, PLSILOGICMEMREGN pRegion)
{
    bool fInserted = false;

    /* Insert at the right position. */
    PLSILOGICMEMREGN pIt;
    RTListForEach(&pThis->ListMemRegns, pIt, LSILOGICMEMREGN, NodeList)
    {
        if (pRegion->u32AddrEnd < pIt->u32AddrStart)
        {
            RTListNodeInsertBefore(&pIt->NodeList, &pRegion->NodeList);
            fInserted = true;
            break;
        }
    }
    if (!fInserted)
        RTListAppend(&pThis->ListMemRegns, &pRegion->NodeList);
}

/**
 * Count number of memory regions.
 *
 * @returns Number of memory regions.
 * @param   pThis           Pointer to the LsiLogic device state.
 */
static uint32_t lsilogicR3MemRegionsCount(PLSILOGICSCSI pThis)
{
    uint32_t cRegions = 0;

    PLSILOGICMEMREGN pIt;
    RTListForEach(&pThis->ListMemRegns, pIt, LSILOGICMEMREGN, NodeList)
    {
        cRegions++;
    }

    return cRegions;
}

/**
 * Handles a write to the diagnostic data register.
 *
 * @returns nothing.
 * @param   pThis           Pointer to the LsiLogic device state.
 * @param   u32Data         Data to write.
 */
static void lsilogicR3DiagRegDataWrite(PLSILOGICSCSI pThis, uint32_t u32Data)
{
    PLSILOGICMEMREGN pRegion = lsilogicR3MemRegionFindByAddr(pThis, pThis->u32DiagMemAddr);

    if (pRegion)
    {
        uint32_t offRegion = pThis->u32DiagMemAddr - pRegion->u32AddrStart;

        AssertMsg(   offRegion % 4 == 0
                  && pThis->u32DiagMemAddr <= pRegion->u32AddrEnd,
                  ("Region offset not on a word boundary or crosses memory region\n"));

        offRegion /= 4;
        pRegion->au32Data[offRegion] = u32Data;
    }
    else
    {
        pRegion = NULL;

        /* Create new region, first check whether we can extend another region. */
        PLSILOGICMEMREGN pIt;
        RTListForEach(&pThis->ListMemRegns, pIt, LSILOGICMEMREGN, NodeList)
        {
            if (pThis->u32DiagMemAddr == pIt->u32AddrEnd + sizeof(uint32_t))
            {
                pRegion = pIt;
                break;
            }
        }

        if (pRegion)
        {
            /* Reallocate. */
            RTListNodeRemove(&pRegion->NodeList);

            uint32_t cRegionSizeOld = (pRegion->u32AddrEnd - pRegion->u32AddrStart) / 4 + 1;
            uint32_t cRegionSizeNew = cRegionSizeOld + 512;

            if (pThis->cbMemRegns + 512 * sizeof(uint32_t) < LSILOGIC_MEMORY_REGIONS_MAX)
            {
                PLSILOGICMEMREGN pRegionNew = (PLSILOGICMEMREGN)RTMemRealloc(pRegion, RT_OFFSETOF(LSILOGICMEMREGN, au32Data[cRegionSizeNew]));

                if (pRegionNew)
                {
                    pRegion = pRegionNew;
                    memset(&pRegion->au32Data[cRegionSizeOld], 0, 512 * sizeof(uint32_t));
                    pRegion->au32Data[cRegionSizeOld] = u32Data;
                    pRegion->u32AddrEnd = pRegion->u32AddrStart + (cRegionSizeNew - 1) * sizeof(uint32_t);
                    pThis->cbMemRegns += 512 * sizeof(uint32_t);
                }
                /* else: Silently fail, there is nothing we can do here and the guest might work nevertheless. */

                lsilogicR3MemRegionInsert(pThis, pRegion);
            }
        }
        else
        {
            if (pThis->cbMemRegns + 512 * sizeof(uint32_t) < LSILOGIC_MEMORY_REGIONS_MAX)
            {
                /* Create completely new. */
                pRegion = (PLSILOGICMEMREGN)RTMemAllocZ(RT_OFFSETOF(LSILOGICMEMREGN, au32Data[512]));
                if (pRegion)
                {
                    pRegion->u32AddrStart = pThis->u32DiagMemAddr;
                    pRegion->u32AddrEnd   = pRegion->u32AddrStart + (512 - 1) * sizeof(uint32_t);
                    pRegion->au32Data[0]  = u32Data;
                    pThis->cbMemRegns += 512 * sizeof(uint32_t);

                    lsilogicR3MemRegionInsert(pThis, pRegion);
                }
                /* else: Silently fail, there is nothing we can do here and the guest might work nevertheless. */
            }
        }

    }

    /* Memory access is always 32bit big. */
    pThis->u32DiagMemAddr += sizeof(uint32_t);
}

/**
 * Handles a read from the diagnostic data register.
 *
 * @returns nothing.
 * @param   pThis           Pointer to the LsiLogic device state.
 * @param   pu32Data        Where to store the data.
 */
static void lsilogicR3DiagRegDataRead(PLSILOGICSCSI pThis, uint32_t *pu32Data)
{
    PLSILOGICMEMREGN pRegion = lsilogicR3MemRegionFindByAddr(pThis, pThis->u32DiagMemAddr);

    if (pRegion)
    {
        uint32_t offRegion = pThis->u32DiagMemAddr - pRegion->u32AddrStart;

        AssertMsg(   offRegion % 4 == 0
                  && pThis->u32DiagMemAddr <= pRegion->u32AddrEnd,
                  ("Region offset not on a word boundary or crosses memory region\n"));

        offRegion /= 4;
        *pu32Data = pRegion->au32Data[offRegion];
    }
    else /* No region, default value 0. */
        *pu32Data = 0;

    /* Memory access is always 32bit big. */
    pThis->u32DiagMemAddr += sizeof(uint32_t);
}

/**
 * Handles a write to the diagnostic memory address register.
 *
 * @returns nothing.
 * @param   pThis           Pointer to the LsiLogic device state.
 * @param   u32Addr         Address to write.
 */
static void lsilogicR3DiagRegAddressWrite(PLSILOGICSCSI pThis, uint32_t u32Addr)
{
    pThis->u32DiagMemAddr = u32Addr & ~UINT32_C(0x3); /* 32bit alignment. */
}

/**
 * Handles a read from the diagnostic memory address register.
 *
 * @returns nothing.
 * @param   pThis           Pointer to the LsiLogic device state.
 * @param   pu32Addr        Where to store the current address.
 */
static void lsilogicR3DiagRegAddressRead(PLSILOGICSCSI pThis, uint32_t *pu32Addr)
{
    *pu32Addr = pThis->u32DiagMemAddr;
}

/**
 * Processes a given Request from the guest
 *
 * @returns VBox status code.
 * @param   pThis       Pointer to the LsiLogic device state.
 * @param   pMessageHdr Pointer to the message header of the request.
 * @param   pReply      Pointer to the reply.
 */
static int lsilogicR3ProcessMessageRequest(PLSILOGICSCSI pThis, PMptMessageHdr pMessageHdr, PMptReplyUnion pReply)
{
    int rc = VINF_SUCCESS;
    bool fForceReplyPostFifo = false;

# ifdef LOG_ENABLED
    if (pMessageHdr->u8Function < RT_ELEMENTS(g_apszMPTFunctionNames))
        Log(("Message request function: %s\n", g_apszMPTFunctionNames[pMessageHdr->u8Function]));
    else
        Log(("Message request function: <unknown>\n"));
# endif

    memset(pReply, 0, sizeof(MptReplyUnion));

    switch (pMessageHdr->u8Function)
    {
        case MPT_MESSAGE_HDR_FUNCTION_SCSI_TASK_MGMT:
        {
            PMptSCSITaskManagementRequest pTaskMgmtReq = (PMptSCSITaskManagementRequest)pMessageHdr;

            LogFlow(("u8TaskType=%u\n", pTaskMgmtReq->u8TaskType));
            LogFlow(("u32TaskMessageContext=%#x\n", pTaskMgmtReq->u32TaskMessageContext));

            pReply->SCSITaskManagement.u8MessageLength     = 6;     /* 6 32bit dwords. */
            pReply->SCSITaskManagement.u8TaskType          = pTaskMgmtReq->u8TaskType;
            pReply->SCSITaskManagement.u32TerminationCount = 0;
            fForceReplyPostFifo = true;
            break;
        }
        case MPT_MESSAGE_HDR_FUNCTION_IOC_INIT:
        {
            /*
             * This request sets the I/O controller to the
             * operational state.
             */
            PMptIOCInitRequest pIOCInitReq = (PMptIOCInitRequest)pMessageHdr;

            /* Update configuration values. */
            pThis->enmWhoInit             = (LSILOGICWHOINIT)pIOCInitReq->u8WhoInit;
            pThis->cbReplyFrame           = pIOCInitReq->u16ReplyFrameSize;
            pThis->cMaxBuses              = pIOCInitReq->u8MaxBuses;
            pThis->cMaxDevices            = pIOCInitReq->u8MaxDevices;
            pThis->u32HostMFAHighAddr     = pIOCInitReq->u32HostMfaHighAddr;
            pThis->u32SenseBufferHighAddr = pIOCInitReq->u32SenseBufferHighAddr;

            if (pThis->enmState == LSILOGICSTATE_READY)
            {
                pThis->enmState = LSILOGICSTATE_OPERATIONAL;
            }

            /* Return reply. */
            pReply->IOCInit.u8MessageLength = 5;
            pReply->IOCInit.u8WhoInit       = pThis->enmWhoInit;
            pReply->IOCInit.u8MaxDevices    = pThis->cMaxDevices;
            pReply->IOCInit.u8MaxBuses      = pThis->cMaxBuses;
            break;
        }
        case MPT_MESSAGE_HDR_FUNCTION_IOC_FACTS:
        {
            pReply->IOCFacts.u8MessageLength      = 15;     /* 15 32bit dwords. */

            if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI)
            {
                pReply->IOCFacts.u16MessageVersion    = 0x0102; /* Version from the specification. */
                pReply->IOCFacts.u8NumberOfPorts      = pThis->cPorts;
            }
            else if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
            {
                pReply->IOCFacts.u16MessageVersion    = 0x0105; /* Version from the specification. */
                pReply->IOCFacts.u8NumberOfPorts      = pThis->cPorts;
            }
            else
                AssertMsgFailed(("Invalid controller type %d\n", pThis->enmCtrlType));

            pReply->IOCFacts.u8IOCNumber          = 0;      /* PCI function number. */
            pReply->IOCFacts.u16IOCExceptions     = 0;
            pReply->IOCFacts.u8MaxChainDepth      = LSILOGICSCSI_MAXIMUM_CHAIN_DEPTH;
            pReply->IOCFacts.u8WhoInit            = pThis->enmWhoInit;
            pReply->IOCFacts.u8BlockSize          = 12;     /* Block size in 32bit dwords. This is the largest request we can get (SCSI I/O). */
            pReply->IOCFacts.u8Flags              = 0;      /* Bit 0 is set if the guest must upload the FW prior to using the controller. Obviously not needed here. */
            pReply->IOCFacts.u16ReplyQueueDepth   = pThis->cReplyQueueEntries - 1; /* One entry is always free. */
            pReply->IOCFacts.u16RequestFrameSize  = 128;    /** @todo Figure out where it is needed. */
            pReply->IOCFacts.u32CurrentHostMFAHighAddr = pThis->u32HostMFAHighAddr;
            pReply->IOCFacts.u16GlobalCredits     = pThis->cRequestQueueEntries - 1; /* One entry is always free. */

            pReply->IOCFacts.u8EventState         = 0; /* Event notifications not enabled. */
            pReply->IOCFacts.u32CurrentSenseBufferHighAddr = pThis->u32SenseBufferHighAddr;
            pReply->IOCFacts.u16CurReplyFrameSize = pThis->cbReplyFrame;
            pReply->IOCFacts.u8MaxDevices         = pThis->cMaxDevices;
            pReply->IOCFacts.u8MaxBuses           = pThis->cMaxBuses;

            /* Check for a valid firmware image in the IOC memory which was downlaoded by tzhe guest earlier. */
            PLSILOGICMEMREGN pRegion = lsilogicR3MemRegionFindByAddr(pThis, LSILOGIC_FWIMGHDR_LOAD_ADDRESS);

            if (pRegion)
            {
                uint32_t offImgHdr = (LSILOGIC_FWIMGHDR_LOAD_ADDRESS - pRegion->u32AddrStart) / 4;
                PFwImageHdr pFwImgHdr = (PFwImageHdr)&pRegion->au32Data[offImgHdr];

                /* Check for the signature. */
                /** @todo Checksum validation. */
                if (   pFwImgHdr->u32Signature1 == LSILOGIC_FWIMGHDR_SIGNATURE1
                    && pFwImgHdr->u32Signature2 == LSILOGIC_FWIMGHDR_SIGNATURE2
                    && pFwImgHdr->u32Signature3 == LSILOGIC_FWIMGHDR_SIGNATURE3)
                {
                    LogFlowFunc(("IOC Facts: Found valid firmware image header in memory, using version (%#x), size (%d) and product ID (%#x) from there\n",
                                 pFwImgHdr->u32FwVersion, pFwImgHdr->u32ImageSize, pFwImgHdr->u16ProductId));

                    pReply->IOCFacts.u16ProductID         = pFwImgHdr->u16ProductId;
                    pReply->IOCFacts.u32FwImageSize       = pFwImgHdr->u32ImageSize;
                    pReply->IOCFacts.u32FWVersion         = pFwImgHdr->u32FwVersion;
                }
            }
            else
            {
                pReply->IOCFacts.u16ProductID         = 0xcafe; /* Our own product ID :) */
                pReply->IOCFacts.u32FwImageSize       = 0; /* No image needed. */
                pReply->IOCFacts.u32FWVersion         = 0;
            }
            break;
        }
        case MPT_MESSAGE_HDR_FUNCTION_PORT_FACTS:
        {
            PMptPortFactsRequest pPortFactsReq = (PMptPortFactsRequest)pMessageHdr;

            pReply->PortFacts.u8MessageLength = 10;
            pReply->PortFacts.u8PortNumber    = pPortFactsReq->u8PortNumber;

            if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI)
            {
                /* This controller only supports one bus with bus number 0. */
                if (pPortFactsReq->u8PortNumber >= pThis->cPorts)
                {
                    pReply->PortFacts.u8PortType = 0; /* Not existant. */
                }
                else
                {
                    pReply->PortFacts.u8PortType             = 0x01; /* SCSI Port. */
                    pReply->PortFacts.u16MaxDevices          = LSILOGICSCSI_PCI_SPI_DEVICES_PER_BUS_MAX;
                    pReply->PortFacts.u16ProtocolFlags       = RT_BIT(3) | RT_BIT(0); /* SCSI initiator and LUN supported. */
                    pReply->PortFacts.u16PortSCSIID          = 7; /* Default */
                    pReply->PortFacts.u16MaxPersistentIDs    = 0;
                    pReply->PortFacts.u16MaxPostedCmdBuffers = 0; /* Only applies for target mode which we dont support. */
                    pReply->PortFacts.u16MaxLANBuckets       = 0; /* Only for the LAN controller. */
                }
            }
            else if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
            {
                if (pPortFactsReq->u8PortNumber >= pThis->cPorts)
                {
                    pReply->PortFacts.u8PortType = 0; /* Not existant. */
                }
                else
                {
                    pReply->PortFacts.u8PortType             = 0x30; /* SAS Port. */
                    pReply->PortFacts.u16MaxDevices          = pThis->cPorts;
                    pReply->PortFacts.u16ProtocolFlags       = RT_BIT(3) | RT_BIT(0); /* SCSI initiator and LUN supported. */
                    pReply->PortFacts.u16PortSCSIID          = pThis->cPorts;
                    pReply->PortFacts.u16MaxPersistentIDs    = 0;
                    pReply->PortFacts.u16MaxPostedCmdBuffers = 0; /* Only applies for target mode which we dont support. */
                    pReply->PortFacts.u16MaxLANBuckets       = 0; /* Only for the LAN controller. */
                }
            }
            else
                AssertMsgFailed(("Invalid controller type %d\n", pThis->enmCtrlType));
            break;
        }
        case MPT_MESSAGE_HDR_FUNCTION_PORT_ENABLE:
        {
            /*
             * The port enable request notifies the IOC to make the port available and perform
             * appropriate discovery on the associated link.
             */
            PMptPortEnableRequest pPortEnableReq = (PMptPortEnableRequest)pMessageHdr;

            pReply->PortEnable.u8MessageLength = 5;
            pReply->PortEnable.u8PortNumber    = pPortEnableReq->u8PortNumber;
            break;
        }
        case MPT_MESSAGE_HDR_FUNCTION_EVENT_NOTIFICATION:
        {
            PMptEventNotificationRequest pEventNotificationReq = (PMptEventNotificationRequest)pMessageHdr;

            if (pEventNotificationReq->u8Switch)
                pThis->fEventNotificationEnabled = true;
            else
                pThis->fEventNotificationEnabled = false;

            pReply->EventNotification.u16EventDataLength = 1; /* 1 32bit D-Word. */
            pReply->EventNotification.u8MessageLength    = 8;
            pReply->EventNotification.u8MessageFlags     = (1 << 7);
            pReply->EventNotification.u8AckRequired      = 0;
            pReply->EventNotification.u32Event           = MPT_EVENT_EVENT_CHANGE;
            pReply->EventNotification.u32EventContext    = 0;
            pReply->EventNotification.u32EventData       = pThis->fEventNotificationEnabled ? 1 : 0;

            break;
        }
        case MPT_MESSAGE_HDR_FUNCTION_EVENT_ACK:
        {
            AssertMsgFailed(("todo"));
            break;
        }
        case MPT_MESSAGE_HDR_FUNCTION_CONFIG:
        {
            PMptConfigurationRequest pConfigurationReq = (PMptConfigurationRequest)pMessageHdr;

            rc = lsilogicR3ProcessConfigurationRequest(pThis, pConfigurationReq, &pReply->Configuration);
            AssertRC(rc);
            break;
        }
        case MPT_MESSAGE_HDR_FUNCTION_FW_UPLOAD:
        {
            PMptFWUploadRequest pFWUploadReq = (PMptFWUploadRequest)pMessageHdr;

            pReply->FWUpload.u8ImageType        = pFWUploadReq->u8ImageType;
            pReply->FWUpload.u8MessageLength    = 6;
            pReply->FWUpload.u32ActualImageSize = 0;
            break;
        }
        case MPT_MESSAGE_HDR_FUNCTION_FW_DOWNLOAD:
        {
            //PMptFWDownloadRequest pFWDownloadReq = (PMptFWDownloadRequest)pMessageHdr;

            pReply->FWDownload.u8MessageLength    = 5;
            LogFlowFunc(("FW Download request issued\n"));
            break;
        }
        case MPT_MESSAGE_HDR_FUNCTION_SCSI_IO_REQUEST: /* Should be handled already. */
        default:
            AssertMsgFailed(("Invalid request function %#x\n", pMessageHdr->u8Function));
    }

    /* Copy common bits from request message frame to reply. */
    pReply->Header.u8Function        = pMessageHdr->u8Function;
    pReply->Header.u32MessageContext = pMessageHdr->u32MessageContext;

    lsilogicFinishAddressReply(pThis, pReply, fForceReplyPostFifo);
    return rc;
}

#endif /* IN_RING3 */

/**
 * Writes a value to a register at a given offset.
 *
 * @returns VBox status code.
 * @param   pThis       Pointer to the LsiLogic device state.
 * @param   offReg      Offset of the register to write.
 * @param   u32         The value being written.
 */
static int lsilogicRegisterWrite(PLSILOGICSCSI pThis, uint32_t offReg, uint32_t u32)
{
    LogFlowFunc(("pThis=%#p offReg=%#x u32=%#x\n", pThis, offReg, u32));
    switch (offReg)
    {
        case LSILOGIC_REG_REPLY_QUEUE:
        {
            int rc = PDMCritSectEnter(&pThis->ReplyFreeQueueWriteCritSect, VINF_IOM_R3_MMIO_WRITE);
            if (rc != VINF_SUCCESS)
                return rc;
            /* Add the entry to the reply free queue. */
            ASMAtomicWriteU32(&pThis->CTX_SUFF(pReplyFreeQueueBase)[pThis->uReplyFreeQueueNextEntryFreeWrite], u32);
            pThis->uReplyFreeQueueNextEntryFreeWrite++;
            pThis->uReplyFreeQueueNextEntryFreeWrite %= pThis->cReplyQueueEntries;
            PDMCritSectLeave(&pThis->ReplyFreeQueueWriteCritSect);
            break;
        }
        case LSILOGIC_REG_REQUEST_QUEUE:
        {
            int rc = PDMCritSectEnter(&pThis->RequestQueueCritSect, VINF_IOM_R3_MMIO_WRITE);
            if (rc != VINF_SUCCESS)
                return rc;

            uint32_t uNextWrite = ASMAtomicReadU32(&pThis->uRequestQueueNextEntryFreeWrite);

            ASMAtomicWriteU32(&pThis->CTX_SUFF(pRequestQueueBase)[uNextWrite], u32);

            /*
             * Don't update the value in place. It can happen that we get preempted
             * after the increment but before the modulo.
             * Another EMT will read the wrong value when processing the queues
             * and hang in an endless loop creating thousands of requests.
             */
            uNextWrite++;
            uNextWrite %= pThis->cRequestQueueEntries;
            ASMAtomicWriteU32(&pThis->uRequestQueueNextEntryFreeWrite, uNextWrite);
            PDMCritSectLeave(&pThis->RequestQueueCritSect);

            /* Send notification to R3 if there is not one sent already. Do this
             * only if the worker thread is not sleeping or might go sleeping. */
            if (!ASMAtomicXchgBool(&pThis->fNotificationSent, true))
            {
                if (ASMAtomicReadBool(&pThis->fWrkThreadSleeping))
                {
#ifdef IN_RC
                    PPDMQUEUEITEMCORE pNotificationItem = PDMQueueAlloc(pThis->CTX_SUFF(pNotificationQueue));
                    AssertPtr(pNotificationItem);
                    PDMQueueInsert(pThis->CTX_SUFF(pNotificationQueue), pNotificationItem);
#else
                    LogFlowFunc(("Signal event semaphore\n"));
                    rc = SUPSemEventSignal(pThis->pSupDrvSession, pThis->hEvtProcess);
                    AssertRC(rc);
#endif
                }
            }
            break;
        }
        case LSILOGIC_REG_DOORBELL:
        {
            /*
             * When the guest writes to this register a real device would set the
             * doorbell status bit in the interrupt status register to indicate that the IOP
             * has still to process the message.
             * The guest needs to wait with posting new messages here until the bit is cleared.
             * Because the guest is not continuing execution while we are here we can skip this.
             */
            if (pThis->enmDoorbellState == LSILOGICDOORBELLSTATE_NOT_IN_USE)
            {
                uint32_t uFunction = LSILOGIC_REG_DOORBELL_GET_FUNCTION(u32);

                switch (uFunction)
                {
                    case LSILOGIC_DOORBELL_FUNCTION_IO_UNIT_RESET:
                    case LSILOGIC_DOORBELL_FUNCTION_IOC_MSG_UNIT_RESET:
                    {
                        /*
                         * The I/O unit reset does much more on real hardware like
                         * reloading the firmware, nothing we need to do here,
                         * so this is like the IOC message unit reset.
                         */
                        pThis->enmState = LSILOGICSTATE_RESET;

                        /* Reset interrupt status. */
                        pThis->uInterruptStatus = 0;
                        lsilogicUpdateInterrupt(pThis);

                        /* Reset the queues. */
                        pThis->uReplyFreeQueueNextEntryFreeWrite = 0;
                        pThis->uReplyFreeQueueNextAddressRead = 0;
                        pThis->uReplyPostQueueNextEntryFreeWrite = 0;
                        pThis->uReplyPostQueueNextAddressRead = 0;
                        pThis->uRequestQueueNextEntryFreeWrite = 0;
                        pThis->uRequestQueueNextAddressRead = 0;

                        /* Only the IOC message unit reset transisionts to the ready state. */
                        if (uFunction == LSILOGIC_DOORBELL_FUNCTION_IOC_MSG_UNIT_RESET)
                            pThis->enmState = LSILOGICSTATE_READY;
                        break;
                    }
                    case LSILOGIC_DOORBELL_FUNCTION_HANDSHAKE:
                    {
                        pThis->cMessage = LSILOGIC_REG_DOORBELL_GET_SIZE(u32);
                        pThis->iMessage = 0;
                        AssertMsg(pThis->cMessage <= RT_ELEMENTS(pThis->aMessage),
                                  ("Message doesn't fit into the buffer, cMessage=%u", pThis->cMessage));
                        pThis->enmDoorbellState = LSILOGICDOORBELLSTATE_FN_HANDSHAKE;
                        /* Update the interrupt status to notify the guest that a doorbell function was started. */
                        lsilogicSetInterrupt(pThis, LSILOGIC_REG_HOST_INTR_STATUS_SYSTEM_DOORBELL);
                        break;
                    }
                    case LSILOGIC_DOORBELL_FUNCTION_REPLY_FRAME_REMOVAL:
                    {
                        pThis->enmDoorbellState = LSILOGICDOORBELLSTATE_RFR_FRAME_COUNT_LOW;
                        /* Update the interrupt status to notify the guest that a doorbell function was started. */
                        lsilogicSetInterrupt(pThis, LSILOGIC_REG_HOST_INTR_STATUS_SYSTEM_DOORBELL);
                        break;
                    }
                    default:
                        AssertMsgFailed(("Unknown function %u to perform\n", uFunction));
                }
            }
            else if (pThis->enmDoorbellState == LSILOGICDOORBELLSTATE_FN_HANDSHAKE)
            {
                /*
                 * We are already performing a doorbell function.
                 * Get the remaining parameters.
                 */
                AssertMsg(pThis->iMessage < RT_ELEMENTS(pThis->aMessage), ("Message is too big to fit into the buffer\n"));
                /*
                 * If the last byte of the message is written, force a switch to R3 because some requests might force
                 * a reply through the FIFO which cannot be handled in GC or R0.
                 */
#ifndef IN_RING3
                if (pThis->iMessage == pThis->cMessage - 1)
                    return VINF_IOM_R3_MMIO_WRITE;
#endif
                pThis->aMessage[pThis->iMessage++] = u32;
#ifdef IN_RING3
                if (pThis->iMessage == pThis->cMessage)
                {
                    int rc = lsilogicR3ProcessMessageRequest(pThis, (PMptMessageHdr)pThis->aMessage, &pThis->ReplyBuffer);
                    AssertRC(rc);
                }
#endif
            }
            break;
        }
        case LSILOGIC_REG_HOST_INTR_STATUS:
        {
            /*
             * Clear the bits the guest wants except the system doorbell interrupt and the IO controller
             * status bit.
             * The former bit is always cleared no matter what the guest writes to the register and
             * the latter one is read only.
             */
            ASMAtomicAndU32(&pThis->uInterruptStatus, ~LSILOGIC_REG_HOST_INTR_STATUS_SYSTEM_DOORBELL);

            /*
             * Check if there is still a doorbell function in progress. Set the
             * system doorbell interrupt bit again if it is.
             * We do not use lsilogicSetInterrupt here because the interrupt status
             * is updated afterwards anyway.
             */
            if (   (pThis->enmDoorbellState == LSILOGICDOORBELLSTATE_FN_HANDSHAKE)
                && (pThis->cMessage == pThis->iMessage))
            {
                if (pThis->uNextReplyEntryRead == pThis->cReplySize)
                {
                    /* Reply finished. Reset doorbell in progress status. */
                    Log(("%s: Doorbell function finished\n", __FUNCTION__));
                    pThis->enmDoorbellState = LSILOGICDOORBELLSTATE_NOT_IN_USE;
                }
                ASMAtomicOrU32(&pThis->uInterruptStatus, LSILOGIC_REG_HOST_INTR_STATUS_SYSTEM_DOORBELL);
            }
            else if (   pThis->enmDoorbellState != LSILOGICDOORBELLSTATE_NOT_IN_USE
                     && pThis->enmDoorbellState != LSILOGICDOORBELLSTATE_FN_HANDSHAKE)
            {
                /* Reply frame removal, check whether the reply free queue is empty. */
                if (   pThis->uReplyFreeQueueNextAddressRead == pThis->uReplyFreeQueueNextEntryFreeWrite
                    && pThis->enmDoorbellState == LSILOGICDOORBELLSTATE_RFR_NEXT_FRAME_LOW)
                    pThis->enmDoorbellState = LSILOGICDOORBELLSTATE_NOT_IN_USE;
                ASMAtomicOrU32(&pThis->uInterruptStatus, LSILOGIC_REG_HOST_INTR_STATUS_SYSTEM_DOORBELL);
            }

            lsilogicUpdateInterrupt(pThis);
            break;
        }
        case LSILOGIC_REG_HOST_INTR_MASK:
        {
            ASMAtomicWriteU32(&pThis->uInterruptMask, u32 & LSILOGIC_REG_HOST_INTR_MASK_W_MASK);
            lsilogicUpdateInterrupt(pThis);
            break;
        }
        case LSILOGIC_REG_WRITE_SEQUENCE:
        {
            if (pThis->fDiagnosticEnabled)
            {
                /* Any value will cause a reset and disabling access. */
                pThis->fDiagnosticEnabled = false;
                pThis->iDiagnosticAccess  = 0;
                pThis->fDiagRegsEnabled   = false;
            }
            else if ((u32 & 0xf) == g_lsilogicDiagnosticAccess[pThis->iDiagnosticAccess])
            {
                pThis->iDiagnosticAccess++;
                if (pThis->iDiagnosticAccess == RT_ELEMENTS(g_lsilogicDiagnosticAccess))
                {
                    /*
                     * Key sequence successfully written. Enable access to diagnostic
                     * memory and register.
                     */
                    pThis->fDiagnosticEnabled = true;
                }
            }
            else
            {
                /* Wrong value written - reset to beginning. */
                pThis->iDiagnosticAccess = 0;
            }
            break;
        }
        case LSILOGIC_REG_HOST_DIAGNOSTIC:
        {
            if (pThis->fDiagnosticEnabled)
            {
#ifndef IN_RING3
                return VINF_IOM_R3_MMIO_WRITE;
#else
                if (u32 & LSILOGIC_REG_HOST_DIAGNOSTIC_RESET_ADAPTER)
                    lsilogicR3HardReset(pThis);
                else if (u32 & LSILOGIC_REG_HOST_DIAGNOSTIC_DIAG_RW_ENABLE)
                    pThis->fDiagRegsEnabled = true;
#endif
            }
            break;
        }
        case LSILOGIC_REG_DIAG_RW_DATA:
        {
            if (pThis->fDiagRegsEnabled)
            {
#ifndef IN_RING3
                return VINF_IOM_R3_MMIO_WRITE;
#else
                lsilogicR3DiagRegDataWrite(pThis, u32);
#endif
            }
            break;
        }
        case LSILOGIC_REG_DIAG_RW_ADDRESS:
        {
            if (pThis->fDiagRegsEnabled)
            {
#ifndef IN_RING3
                return VINF_IOM_R3_MMIO_WRITE;
#else
                lsilogicR3DiagRegAddressWrite(pThis, u32);
#endif
            }
            break;
        }
        default: /* Ignore. */
        {
            break;
        }
    }
    return VINF_SUCCESS;
}

/**
 * Reads the content of a register at a given offset.
 *
 * @returns VBox status code.
 * @param   pThis       Pointer to the LsiLogic device state.
 * @param   offReg      Offset of the register to read.
 * @param   pu32        Where to store the content of the register.
 */
static int lsilogicRegisterRead(PLSILOGICSCSI pThis, uint32_t offReg, uint32_t *pu32)
{
    int rc = VINF_SUCCESS;
    uint32_t u32 = 0;
    Assert(!(offReg & 3));

    /* Align to a 4 byte offset. */
    switch (offReg)
    {
        case LSILOGIC_REG_REPLY_QUEUE:
        {
            rc = PDMCritSectEnter(&pThis->ReplyPostQueueCritSect, VINF_IOM_R3_MMIO_READ);
            if (rc != VINF_SUCCESS)
                break;

            uint32_t idxReplyPostQueueWrite = ASMAtomicUoReadU32(&pThis->uReplyPostQueueNextEntryFreeWrite);
            uint32_t idxReplyPostQueueRead  = ASMAtomicUoReadU32(&pThis->uReplyPostQueueNextAddressRead);

            if (idxReplyPostQueueWrite != idxReplyPostQueueRead)
            {
                u32 = pThis->CTX_SUFF(pReplyPostQueueBase)[idxReplyPostQueueRead];
                idxReplyPostQueueRead++;
                idxReplyPostQueueRead %= pThis->cReplyQueueEntries;
                ASMAtomicWriteU32(&pThis->uReplyPostQueueNextAddressRead, idxReplyPostQueueRead);
            }
            else
            {
                /* The reply post queue is empty. Reset interrupt. */
                u32 = UINT32_C(0xffffffff);
                lsilogicClearInterrupt(pThis, LSILOGIC_REG_HOST_INTR_STATUS_REPLY_INTR);
            }
            PDMCritSectLeave(&pThis->ReplyPostQueueCritSect);

            Log(("%s: Returning address %#x\n", __FUNCTION__, u32));
            break;
        }
        case LSILOGIC_REG_DOORBELL:
        {
            u32  = LSILOGIC_REG_DOORBELL_SET_STATE(pThis->enmState);
            u32 |= LSILOGIC_REG_DOORBELL_SET_USED(pThis->enmDoorbellState);
            u32 |= LSILOGIC_REG_DOORBELL_SET_WHOINIT(pThis->enmWhoInit);
            /*
             * If there is a doorbell function in progress we pass the return value
             * instead of the status code. We transfer 16bit of the reply
             * during one read.
             */
            switch (pThis->enmDoorbellState)
            {
                case LSILOGICDOORBELLSTATE_NOT_IN_USE:
                    /* We return the status code of the I/O controller. */
                    u32 |= pThis->u16IOCFaultCode;
                    break;
                case LSILOGICDOORBELLSTATE_FN_HANDSHAKE:
                    /* Return next 16bit value. */
                    if (pThis->uNextReplyEntryRead < pThis->cReplySize)
                        u32 |= pThis->ReplyBuffer.au16Reply[pThis->uNextReplyEntryRead++];
                    lsilogicSetInterrupt(pThis, LSILOGIC_REG_HOST_INTR_STATUS_SYSTEM_DOORBELL);
                    break;
                case LSILOGICDOORBELLSTATE_RFR_FRAME_COUNT_LOW:
                {
                    uint32_t cReplyFrames = lsilogicReplyFreeQueueGetFrameCount(pThis);

                    u32 |= cReplyFrames & UINT32_C(0xffff);
                    pThis->enmDoorbellState = LSILOGICDOORBELLSTATE_RFR_FRAME_COUNT_HIGH;
                    lsilogicSetInterrupt(pThis, LSILOGIC_REG_HOST_INTR_STATUS_SYSTEM_DOORBELL);
                    break;
                }
                case LSILOGICDOORBELLSTATE_RFR_FRAME_COUNT_HIGH:
                {
                    uint32_t cReplyFrames = lsilogicReplyFreeQueueGetFrameCount(pThis);

                    u32 |= cReplyFrames >> 16;
                    pThis->enmDoorbellState = LSILOGICDOORBELLSTATE_RFR_NEXT_FRAME_LOW;
                    lsilogicSetInterrupt(pThis, LSILOGIC_REG_HOST_INTR_STATUS_SYSTEM_DOORBELL);
                    break;
                }
                case LSILOGICDOORBELLSTATE_RFR_NEXT_FRAME_LOW:
                    if (pThis->uReplyFreeQueueNextEntryFreeWrite != pThis->uReplyFreeQueueNextAddressRead)
                    {
                        u32 |= pThis->CTX_SUFF(pReplyFreeQueueBase)[pThis->uReplyFreeQueueNextAddressRead] & UINT32_C(0xffff);
                        pThis->enmDoorbellState = LSILOGICDOORBELLSTATE_RFR_NEXT_FRAME_HIGH;
                        lsilogicSetInterrupt(pThis, LSILOGIC_REG_HOST_INTR_STATUS_SYSTEM_DOORBELL);
                    }
                    break;
                case LSILOGICDOORBELLSTATE_RFR_NEXT_FRAME_HIGH:
                    u32 |= pThis->CTX_SUFF(pReplyFreeQueueBase)[pThis->uReplyFreeQueueNextAddressRead] >> 16;
                    pThis->uReplyFreeQueueNextAddressRead++;
                    pThis->uReplyFreeQueueNextAddressRead %= pThis->cReplyQueueEntries;
                    pThis->enmDoorbellState = LSILOGICDOORBELLSTATE_RFR_NEXT_FRAME_LOW;
                    lsilogicSetInterrupt(pThis, LSILOGIC_REG_HOST_INTR_STATUS_SYSTEM_DOORBELL);
                    break;
                default:
                    AssertMsgFailed(("Invalid doorbell state %d\n", pThis->enmDoorbellState));
            }

            break;
        }
        case LSILOGIC_REG_HOST_INTR_STATUS:
        {
            u32 = ASMAtomicReadU32(&pThis->uInterruptStatus);
            break;
        }
        case LSILOGIC_REG_HOST_INTR_MASK:
        {
            u32 = ASMAtomicReadU32(&pThis->uInterruptMask);
            break;
        }
        case LSILOGIC_REG_HOST_DIAGNOSTIC:
        {
            if (pThis->fDiagnosticEnabled)
                u32 |= LSILOGIC_REG_HOST_DIAGNOSTIC_DRWE;
            if (pThis->fDiagRegsEnabled)
                u32 |= LSILOGIC_REG_HOST_DIAGNOSTIC_DIAG_RW_ENABLE;
            break;
        }
        case LSILOGIC_REG_DIAG_RW_DATA:
        {
            if (pThis->fDiagRegsEnabled)
            {
#ifndef IN_RING3
                return VINF_IOM_R3_MMIO_READ;
#else
                lsilogicR3DiagRegDataRead(pThis, &u32);
#endif
            }
        }
        RT_FALL_THRU();
        case LSILOGIC_REG_DIAG_RW_ADDRESS:
        {
            if (pThis->fDiagRegsEnabled)
            {
#ifndef IN_RING3
                return VINF_IOM_R3_MMIO_READ;
#else
                lsilogicR3DiagRegAddressRead(pThis, &u32);
#endif
            }
        }
        RT_FALL_THRU();
        case LSILOGIC_REG_TEST_BASE_ADDRESS: /* The spec doesn't say anything about these registers, so we just ignore them */
        default: /* Ignore. */
        {
            /** @todo LSILOGIC_REG_DIAG_* should return all F's when accessed by MMIO. We
             *        return 0.  Likely to apply to undefined offsets as well. */
            break;
        }
    }

    *pu32 = u32;
    LogFlowFunc(("pThis=%#p offReg=%#x u32=%#x\n", pThis, offReg, u32));
    return rc;
}

/**
 * @callback_method_impl{FNIOMIOPORTOUT}
 */
PDMBOTHCBDECL(int) lsilogicIOPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t u32, unsigned cb)
{
    PLSILOGICSCSI   pThis  = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    uint32_t        offReg = uPort - pThis->IOPortBase;
    int             rc;
    RT_NOREF2(pvUser, cb);

    if (!(offReg & 3))
    {
        rc = lsilogicRegisterWrite(pThis, offReg, u32);
        if (rc == VINF_IOM_R3_MMIO_WRITE)
            rc = VINF_IOM_R3_IOPORT_WRITE;
    }
    else
    {
        Log(("lsilogicIOPortWrite: Ignoring misaligned write - offReg=%#x u32=%#x cb=%#x\n", offReg, u32, cb));
        rc = VINF_SUCCESS;
    }

    return rc;
}

/**
 * @callback_method_impl{FNIOMIOPORTIN}
 */
PDMBOTHCBDECL(int) lsilogicIOPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t *pu32, unsigned cb)
{
    PLSILOGICSCSI   pThis   = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    uint32_t        offReg  = uPort - pThis->IOPortBase;
    RT_NOREF_PV(pvUser);
    RT_NOREF_PV(cb);

    int rc = lsilogicRegisterRead(pThis, offReg & ~(uint32_t)3, pu32);
    if (rc == VINF_IOM_R3_MMIO_READ)
        rc = VINF_IOM_R3_IOPORT_READ;

    return rc;
}

/**
 * @callback_method_impl{FNIOMMMIOWRITE}
 */
PDMBOTHCBDECL(int) lsilogicMMIOWrite(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void const *pv, unsigned cb)
{
    PLSILOGICSCSI   pThis  = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    uint32_t        offReg = GCPhysAddr - pThis->GCPhysMMIOBase;
    uint32_t        u32;
    int             rc;
    RT_NOREF_PV(pvUser);

    /* See comments in lsilogicR3Map regarding size and alignment. */
    if (cb == 4)
        u32 = *(uint32_t const *)pv;
    else
    {
        if (cb > 4)
            u32 = *(uint32_t const *)pv;
        else if (cb >= 2)
            u32 = *(uint16_t const *)pv;
        else
            u32 = *(uint8_t const *)pv;
        Log(("lsilogicMMIOWrite: Non-DWORD write access - offReg=%#x u32=%#x cb=%#x\n", offReg, u32, cb));
    }

    if (!(offReg & 3))
        rc = lsilogicRegisterWrite(pThis, offReg, u32);
    else
    {
        Log(("lsilogicIOPortWrite: Ignoring misaligned write - offReg=%#x u32=%#x cb=%#x\n", offReg, u32, cb));
        rc = VINF_SUCCESS;
    }
    return rc;
}

/**
 * @callback_method_impl{FNIOMMMIOREAD}
 */
PDMBOTHCBDECL(int) lsilogicMMIORead(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb)
{
    PLSILOGICSCSI   pThis  = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    uint32_t        offReg = GCPhysAddr - pThis->GCPhysMMIOBase;
    Assert(!(offReg & 3)); Assert(cb == 4);
    RT_NOREF2(pvUser, cb);

    return lsilogicRegisterRead(pThis, offReg, (uint32_t *)pv);
}

PDMBOTHCBDECL(int) lsilogicDiagnosticWrite(PPDMDEVINS pDevIns, void *pvUser,
                                           RTGCPHYS GCPhysAddr, void const *pv, unsigned cb)
{
#ifdef LOG_ENABLED
    PLSILOGICSCSI  pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    LogFlowFunc(("pThis=%#p GCPhysAddr=%RGp pv=%#p{%.*Rhxs} cb=%u\n", pThis, GCPhysAddr, pv, cb, pv, cb));
#endif

    RT_NOREF_PV(pDevIns); RT_NOREF_PV(pvUser); RT_NOREF_PV(GCPhysAddr); RT_NOREF_PV(pv); RT_NOREF_PV(cb);
    return VINF_SUCCESS;
}

PDMBOTHCBDECL(int) lsilogicDiagnosticRead(PPDMDEVINS pDevIns, void *pvUser,
                                          RTGCPHYS GCPhysAddr, void *pv, unsigned cb)
{
#ifdef LOG_ENABLED
    PLSILOGICSCSI  pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    LogFlowFunc(("pThis=%#p GCPhysAddr=%RGp pv=%#p{%.*Rhxs} cb=%u\n", pThis, GCPhysAddr, pv, cb, pv, cb));
#endif

    RT_NOREF_PV(pDevIns); RT_NOREF_PV(pvUser); RT_NOREF_PV(GCPhysAddr); RT_NOREF_PV(pv); RT_NOREF_PV(cb);
    return VINF_SUCCESS;
}

#ifdef IN_RING3

# ifdef LOG_ENABLED
/**
 * Dump an SG entry.
 *
 * @returns nothing.
 * @param   pSGEntry    Pointer to the SG entry to dump
 */
static void lsilogicDumpSGEntry(PMptSGEntryUnion pSGEntry)
{
    if (LogIsEnabled())
    {
        switch (pSGEntry->Simple32.u2ElementType)
        {
            case MPTSGENTRYTYPE_SIMPLE:
            {
                Log(("%s: Dumping info for SIMPLE SG entry:\n", __FUNCTION__));
                Log(("%s: u24Length=%u\n", __FUNCTION__, pSGEntry->Simple32.u24Length));
                Log(("%s: fEndOfList=%d\n", __FUNCTION__, pSGEntry->Simple32.fEndOfList));
                Log(("%s: f64BitAddress=%d\n", __FUNCTION__, pSGEntry->Simple32.f64BitAddress));
                Log(("%s: fBufferContainsData=%d\n", __FUNCTION__, pSGEntry->Simple32.fBufferContainsData));
                Log(("%s: fLocalAddress=%d\n", __FUNCTION__, pSGEntry->Simple32.fLocalAddress));
                Log(("%s: fEndOfBuffer=%d\n", __FUNCTION__, pSGEntry->Simple32.fEndOfBuffer));
                Log(("%s: fLastElement=%d\n", __FUNCTION__, pSGEntry->Simple32.fLastElement));
                Log(("%s: u32DataBufferAddressLow=%u\n", __FUNCTION__, pSGEntry->Simple32.u32DataBufferAddressLow));
                if (pSGEntry->Simple32.f64BitAddress)
                {
                    Log(("%s: u32DataBufferAddressHigh=%u\n", __FUNCTION__, pSGEntry->Simple64.u32DataBufferAddressHigh));
                    Log(("%s: GCDataBufferAddress=%RGp\n", __FUNCTION__,
                         ((uint64_t)pSGEntry->Simple64.u32DataBufferAddressHigh << 32)
                         | pSGEntry->Simple64.u32DataBufferAddressLow));
                }
                else
                    Log(("%s: GCDataBufferAddress=%RGp\n", __FUNCTION__, pSGEntry->Simple32.u32DataBufferAddressLow));

                break;
            }
            case MPTSGENTRYTYPE_CHAIN:
            {
                Log(("%s: Dumping info for CHAIN SG entry:\n", __FUNCTION__));
                Log(("%s: u16Length=%u\n", __FUNCTION__, pSGEntry->Chain.u16Length));
                Log(("%s: u8NExtChainOffset=%d\n", __FUNCTION__, pSGEntry->Chain.u8NextChainOffset));
                Log(("%s: f64BitAddress=%d\n", __FUNCTION__, pSGEntry->Chain.f64BitAddress));
                Log(("%s: fLocalAddress=%d\n", __FUNCTION__, pSGEntry->Chain.fLocalAddress));
                Log(("%s: u32SegmentAddressLow=%u\n", __FUNCTION__, pSGEntry->Chain.u32SegmentAddressLow));
                Log(("%s: u32SegmentAddressHigh=%u\n", __FUNCTION__, pSGEntry->Chain.u32SegmentAddressHigh));
                if (pSGEntry->Chain.f64BitAddress)
                    Log(("%s: GCSegmentAddress=%RGp\n", __FUNCTION__,
                        ((uint64_t)pSGEntry->Chain.u32SegmentAddressHigh << 32) | pSGEntry->Chain.u32SegmentAddressLow));
                else
                    Log(("%s: GCSegmentAddress=%RGp\n", __FUNCTION__, pSGEntry->Chain.u32SegmentAddressLow));
                break;
            }
        }
    }
}
# endif /* LOG_ENABLED */

/**
 * Copy from guest to host memory worker.
 *
 * @copydoc LSILOGICR3MEMCOPYCALLBACK
 */
static DECLCALLBACK(void) lsilogicR3CopyBufferFromGuestWorker(PLSILOGICSCSI pThis, RTGCPHYS GCPhys, PRTSGBUF pSgBuf,
                                                              size_t cbCopy, size_t *pcbSkip)
{
    size_t cbSkipped = RT_MIN(cbCopy, *pcbSkip);
    cbCopy   -= cbSkipped;
    GCPhys   += cbSkipped;
    *pcbSkip -= cbSkipped;

    while (cbCopy)
    {
        size_t cbSeg = cbCopy;
        void *pvSeg = RTSgBufGetNextSegment(pSgBuf, &cbSeg);

        AssertPtr(pvSeg);
        PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), GCPhys, pvSeg, cbSeg);
        GCPhys += cbSeg;
        cbCopy -= cbSeg;
    }
}

/**
 * Copy from host to guest memory worker.
 *
 * @copydoc LSILOGICR3MEMCOPYCALLBACK
 */
static DECLCALLBACK(void) lsilogicR3CopyBufferToGuestWorker(PLSILOGICSCSI pThis, RTGCPHYS GCPhys, PRTSGBUF pSgBuf,
                                                            size_t cbCopy, size_t *pcbSkip)
{
    size_t cbSkipped = RT_MIN(cbCopy, *pcbSkip);
    cbCopy   -= cbSkipped;
    GCPhys   += cbSkipped;
    *pcbSkip -= cbSkipped;

    while (cbCopy)
    {
        size_t cbSeg = cbCopy;
        void *pvSeg = RTSgBufGetNextSegment(pSgBuf, &cbSeg);

        AssertPtr(pvSeg);
        PDMDevHlpPCIPhysWrite(pThis->CTX_SUFF(pDevIns), GCPhys, pvSeg, cbSeg);
        GCPhys += cbSeg;
        cbCopy -= cbSeg;
    }
}

/**
 * Walks the guest S/G buffer calling the given copy worker for every buffer.
 *
 * @returns The amout of bytes actually copied.
 * @param   pThis                  Pointer to the LsiLogic device state.
 * @param   pLsiReq                LSI request state.
 * @param   pfnCopyWorker          The copy method to apply for each guest buffer.
 * @param   pSgBuf                 The host S/G buffer.
 * @param   cbSkip                 How many bytes to skip in advance before starting to copy.
 * @param   cbCopy                 How many bytes to copy.
 */
static size_t lsilogicSgBufWalker(PLSILOGICSCSI pThis, PLSILOGICREQ pLsiReq,
                                  PLSILOGICR3MEMCOPYCALLBACK pfnCopyWorker,
                                  PRTSGBUF pSgBuf, size_t cbSkip, size_t cbCopy)
{
    bool     fEndOfList = false;
    RTGCPHYS GCPhysSgEntryNext = pLsiReq->GCPhysSgStart;
    RTGCPHYS GCPhysSegmentStart = pLsiReq->GCPhysSgStart;
    uint32_t cChainOffsetNext = pLsiReq->cChainOffset;
    PPDMDEVINS pDevIns = pThis->CTX_SUFF(pDevIns);
    size_t cbCopied = 0;

    /*
     * Add the amount to skip to the host buffer size to avoid a
     * few conditionals later on.
     */
    cbCopy += cbSkip;

    /* Go through the list until we reach the end. */
    while (   !fEndOfList
           && cbCopy)
    {
        bool fEndOfSegment = false;

        while (   !fEndOfSegment
               && cbCopy)
        {
            MptSGEntryUnion SGEntry;

            Log(("%s: Reading SG entry from %RGp\n", __FUNCTION__, GCPhysSgEntryNext));

            /* Read the entry. */
            PDMDevHlpPhysRead(pDevIns, GCPhysSgEntryNext, &SGEntry, sizeof(MptSGEntryUnion));

# ifdef LOG_ENABLED
            lsilogicDumpSGEntry(&SGEntry);
# endif

            AssertMsg(SGEntry.Simple32.u2ElementType == MPTSGENTRYTYPE_SIMPLE, ("Invalid SG entry type\n"));

            /* Check if this is a zero element and abort. */
            if (   !SGEntry.Simple32.u24Length
                && SGEntry.Simple32.fEndOfList
                && SGEntry.Simple32.fEndOfBuffer)
                return cbCopied - RT_MIN(cbSkip, cbCopied);

            uint32_t cbCopyThis           = SGEntry.Simple32.u24Length;
            RTGCPHYS GCPhysAddrDataBuffer = SGEntry.Simple32.u32DataBufferAddressLow;

            if (SGEntry.Simple32.f64BitAddress)
            {
                GCPhysAddrDataBuffer |= ((uint64_t)SGEntry.Simple64.u32DataBufferAddressHigh) << 32;
                GCPhysSgEntryNext += sizeof(MptSGEntrySimple64);
            }
            else
                GCPhysSgEntryNext += sizeof(MptSGEntrySimple32);

            pfnCopyWorker(pThis, GCPhysAddrDataBuffer, pSgBuf, cbCopyThis, &cbSkip);
            cbCopy -= cbCopyThis;
            cbCopied += cbCopyThis;

            /* Check if we reached the end of the list. */
            if (SGEntry.Simple32.fEndOfList)
            {
                /* We finished. */
                fEndOfSegment = true;
                fEndOfList = true;
            }
            else if (SGEntry.Simple32.fLastElement)
                fEndOfSegment = true;
        } /* while (!fEndOfSegment) */

        /* Get next chain element. */
        if (cChainOffsetNext)
        {
            MptSGEntryChain SGEntryChain;

            PDMDevHlpPhysRead(pDevIns, GCPhysSegmentStart + cChainOffsetNext, &SGEntryChain, sizeof(MptSGEntryChain));

            AssertMsg(SGEntryChain.u2ElementType == MPTSGENTRYTYPE_CHAIN, ("Invalid SG entry type\n"));

           /* Set the next address now. */
            GCPhysSgEntryNext = SGEntryChain.u32SegmentAddressLow;
            if (SGEntryChain.f64BitAddress)
                GCPhysSgEntryNext |= ((uint64_t)SGEntryChain.u32SegmentAddressHigh) << 32;

            GCPhysSegmentStart = GCPhysSgEntryNext;
            cChainOffsetNext   = SGEntryChain.u8NextChainOffset * sizeof(uint32_t);
        }
    } /* while (!fEndOfList) */

    return cbCopied - RT_MIN(cbSkip, cbCopied);
}

/**
 * Copies a data buffer into the S/G buffer set up by the guest.
 *
 * @returns Amount of bytes copied to the guest.
 * @param   pThis          The LsiLogic controller device instance.
 * @param   pReq           Request structure.
 * @param   pSgBuf         The S/G buffer to copy from.
 * @param   cbSkip         How many bytes to skip in advance before starting to copy.
 * @param   cbCopy         How many bytes to copy.
 */
static size_t lsilogicR3CopySgBufToGuest(PLSILOGICSCSI pThis, PLSILOGICREQ pReq, PRTSGBUF pSgBuf,
                                         size_t cbSkip, size_t cbCopy)
{
    return lsilogicSgBufWalker(pThis, pReq, lsilogicR3CopyBufferToGuestWorker,
                               pSgBuf, cbSkip, cbCopy);
}

/**
 * Copies the guest S/G buffer into a host data buffer.
 *
 * @returns Amount of bytes copied from the guest.
 * @param   pThis          The LsiLogic controller device instance.
 * @param   pReq           Request structure.
 * @param   pSgBuf         The S/G buffer to copy into.
 * @param   cbSkip         How many bytes to skip in advance before starting to copy.
 * @param   cbCopy         How many bytes to copy.
 */
static size_t lsilogicR3CopySgBufFromGuest(PLSILOGICSCSI pThis, PLSILOGICREQ pReq, PRTSGBUF pSgBuf,
                                           size_t cbSkip, size_t cbCopy)
{
    return lsilogicSgBufWalker(pThis, pReq, lsilogicR3CopyBufferFromGuestWorker,
                               pSgBuf, cbSkip, cbCopy);
}

#if 0 /* unused */
/**
 * Copy a simple memory buffer to the guest memory buffer.
 *
 * @returns Amount of bytes copied to the guest.
 * @param   pThis          The LsiLogic controller device instance.
 * @param   pReq           Request structure.
 * @param   pvSrc          The buffer to copy from.
 * @param   cbSrc          How many bytes to copy.
 * @param   cbSkip         How many bytes to skip initially.
 */
static size_t lsilogicR3CopyBufferToGuest(PLSILOGICSCSI pThis, PLSILOGICREQ pReq, const void *pvSrc,
                                          size_t cbSrc, size_t cbSkip)
{
    RTSGSEG Seg;
    RTSGBUF SgBuf;
    Seg.pvSeg = (void *)pvSrc;
    Seg.cbSeg = cbSrc;
    RTSgBufInit(&SgBuf, &Seg, 1);
    return lsilogicR3CopySgBufToGuest(pThis, pReq, &SgBuf, cbSkip, cbSrc);
}

/**
 * Copy a guest memry buffe into simple host memory buffer.
 *
 * @returns Amount of bytes copied to the guest.
 * @param   pThis          The LsiLogic controller device instance.
 * @param   pReq           Request structure.
 * @param   pvSrc          The buffer to copy from.
 * @param   cbSrc          How many bytes to copy.
 * @param   cbSkip         How many bytes to skip initially.
 */
static size_t lsilogicR3CopyBufferFromGuest(PLSILOGICSCSI pThis, PLSILOGICREQ pReq, void *pvDst,
                                            size_t cbDst, size_t cbSkip)
{
    RTSGSEG Seg;
    RTSGBUF SgBuf;
    Seg.pvSeg = (void *)pvDst;
    Seg.cbSeg = cbDst;
    RTSgBufInit(&SgBuf, &Seg, 1);
    return lsilogicR3CopySgBufFromGuest(pThis, pReq, &SgBuf, cbSkip, cbDst);
}
#endif

# ifdef LOG_ENABLED
static void lsilogicR3DumpSCSIIORequest(PMptSCSIIORequest pSCSIIORequest)
{
    if (LogIsEnabled())
    {
        Log(("%s: u8TargetID=%d\n", __FUNCTION__, pSCSIIORequest->u8TargetID));
        Log(("%s: u8Bus=%d\n", __FUNCTION__, pSCSIIORequest->u8Bus));
        Log(("%s: u8ChainOffset=%d\n", __FUNCTION__, pSCSIIORequest->u8ChainOffset));
        Log(("%s: u8Function=%d\n", __FUNCTION__, pSCSIIORequest->u8Function));
        Log(("%s: u8CDBLength=%d\n", __FUNCTION__, pSCSIIORequest->u8CDBLength));
        Log(("%s: u8SenseBufferLength=%d\n", __FUNCTION__, pSCSIIORequest->u8SenseBufferLength));
        Log(("%s: u8MessageFlags=%d\n", __FUNCTION__, pSCSIIORequest->u8MessageFlags));
        Log(("%s: u32MessageContext=%#x\n", __FUNCTION__, pSCSIIORequest->u32MessageContext));
        for (unsigned i = 0; i < RT_ELEMENTS(pSCSIIORequest->au8LUN); i++)
            Log(("%s: u8LUN[%d]=%d\n", __FUNCTION__, i, pSCSIIORequest->au8LUN[i]));
        Log(("%s: u32Control=%#x\n", __FUNCTION__, pSCSIIORequest->u32Control));
        for (unsigned i = 0; i < RT_ELEMENTS(pSCSIIORequest->au8CDB); i++)
            Log(("%s: u8CDB[%d]=%d\n", __FUNCTION__, i, pSCSIIORequest->au8CDB[i]));
        Log(("%s: u32DataLength=%#x\n", __FUNCTION__, pSCSIIORequest->u32DataLength));
        Log(("%s: u32SenseBufferLowAddress=%#x\n", __FUNCTION__, pSCSIIORequest->u32SenseBufferLowAddress));
    }
}
# endif

/**
 * Handles the completion of th given request.
 *
 * @returns nothing.
 * @param   pThis                  Pointer to the LsiLogic device state.
 * @param   pReq                   The request to complete.
 * @param   rcReq                  Status code of the request.
 */
static void lsilogicR3ReqComplete(PLSILOGICSCSI pThis, PLSILOGICREQ pReq, int rcReq)
{
    PLSILOGICDEVICE pTgtDev = pReq->pTargetDevice;

    if (RT_UNLIKELY(pReq->fBIOS))
    {
        uint8_t u8ScsiSts = pReq->u8ScsiSts;
        pTgtDev->pDrvMediaEx->pfnIoReqFree(pTgtDev->pDrvMediaEx, pReq->hIoReq);
        int rc = vboxscsiRequestFinished(&pThis->VBoxSCSI, u8ScsiSts);
        AssertMsgRC(rc, ("Finishing BIOS SCSI request failed rc=%Rrc\n", rc));
    }
    else
    {
        RTGCPHYS GCPhysAddrSenseBuffer;

        GCPhysAddrSenseBuffer = pReq->GuestRequest.SCSIIO.u32SenseBufferLowAddress;
        GCPhysAddrSenseBuffer |= ((uint64_t)pThis->u32SenseBufferHighAddr << 32);

        /* Copy the sense buffer over. */
        PDMDevHlpPCIPhysWrite(pThis->CTX_SUFF(pDevIns), GCPhysAddrSenseBuffer, pReq->abSenseBuffer,
                              RT_UNLIKELY(  pReq->GuestRequest.SCSIIO.u8SenseBufferLength
                                          < sizeof(pReq->abSenseBuffer))
                              ? pReq->GuestRequest.SCSIIO.u8SenseBufferLength
                              : sizeof(pReq->abSenseBuffer));

        if (RT_SUCCESS(rcReq) && RT_LIKELY(pReq->u8ScsiSts == SCSI_STATUS_OK))
        {
            uint32_t u32MsgCtx = pReq->GuestRequest.SCSIIO.u32MessageContext;

            /* Free the request before posting completion. */
            pTgtDev->pDrvMediaEx->pfnIoReqFree(pTgtDev->pDrvMediaEx, pReq->hIoReq);
            lsilogicR3FinishContextReply(pThis, u32MsgCtx);
        }
        else
        {
            MptReplyUnion IOCReply;
            RT_ZERO(IOCReply);

            /* The SCSI target encountered an error during processing post a reply. */
            IOCReply.SCSIIOError.u8TargetID          = pReq->GuestRequest.SCSIIO.u8TargetID;
            IOCReply.SCSIIOError.u8Bus               = pReq->GuestRequest.SCSIIO.u8Bus;
            IOCReply.SCSIIOError.u8MessageLength     = 8;
            IOCReply.SCSIIOError.u8Function          = pReq->GuestRequest.SCSIIO.u8Function;
            IOCReply.SCSIIOError.u8CDBLength         = pReq->GuestRequest.SCSIIO.u8CDBLength;
            IOCReply.SCSIIOError.u8SenseBufferLength = pReq->GuestRequest.SCSIIO.u8SenseBufferLength;
            IOCReply.SCSIIOError.u8MessageFlags      = pReq->GuestRequest.SCSIIO.u8MessageFlags;
            IOCReply.SCSIIOError.u32MessageContext   = pReq->GuestRequest.SCSIIO.u32MessageContext;
            IOCReply.SCSIIOError.u8SCSIStatus        = pReq->u8ScsiSts;
            IOCReply.SCSIIOError.u8SCSIState         = MPT_SCSI_IO_ERROR_SCSI_STATE_AUTOSENSE_VALID;
            IOCReply.SCSIIOError.u16IOCStatus        = 0;
            IOCReply.SCSIIOError.u32IOCLogInfo       = 0;
            IOCReply.SCSIIOError.u32TransferCount    = 0;
            IOCReply.SCSIIOError.u32SenseCount       = sizeof(pReq->abSenseBuffer);
            IOCReply.SCSIIOError.u32ResponseInfo     = 0;

            /* Free the request before posting completion. */
            pTgtDev->pDrvMediaEx->pfnIoReqFree(pTgtDev->pDrvMediaEx, pReq->hIoReq);
            lsilogicFinishAddressReply(pThis, &IOCReply, false);
        }
    }

    ASMAtomicDecU32(&pTgtDev->cOutstandingRequests);

    if (pTgtDev->cOutstandingRequests == 0 && pThis->fSignalIdle)
        PDMDevHlpAsyncNotificationCompleted(pThis->pDevInsR3);
}

/**
 * Processes a SCSI I/O request by setting up the request
 * and sending it to the underlying SCSI driver.
 * Steps needed to complete request are done in the
 * callback called by the driver below upon completion of
 * the request.
 *
 * @returns VBox status code.
 * @param   pThis                  Pointer to the LsiLogic device state.
 * @param   GCPhysMessageFrameAddr Guest physical address where the request is located.
 * @param   pGuestReq              The request read fro th guest memory.
 */
static int lsilogicR3ProcessSCSIIORequest(PLSILOGICSCSI pThis, RTGCPHYS GCPhysMessageFrameAddr,
                                          PMptRequestUnion pGuestReq)
{
    MptReplyUnion IOCReply;
    int rc = VINF_SUCCESS;

# ifdef LOG_ENABLED
    lsilogicR3DumpSCSIIORequest(&pGuestReq->SCSIIO);
# endif

    if (RT_LIKELY(   (pGuestReq->SCSIIO.u8TargetID < pThis->cDeviceStates)
                  && (pGuestReq->SCSIIO.u8Bus == 0)))
    {
        PLSILOGICDEVICE pTgtDev = &pThis->paDeviceStates[pGuestReq->SCSIIO.u8TargetID];

        if (pTgtDev->pDrvBase)
        {
            /* Allocate and prepare a new request. */
            PDMMEDIAEXIOREQ hIoReq;
            PLSILOGICREQ pLsiReq = NULL;
            rc = pTgtDev->pDrvMediaEx->pfnIoReqAlloc(pTgtDev->pDrvMediaEx, &hIoReq, (void **)&pLsiReq,
                                                     pGuestReq->SCSIIO.u32MessageContext,
                                                     PDMIMEDIAEX_F_SUSPEND_ON_RECOVERABLE_ERR);
            if (RT_SUCCESS(rc))
            {
                pLsiReq->hIoReq                 = hIoReq;
                pLsiReq->pTargetDevice          = pTgtDev;
                pLsiReq->GCPhysMessageFrameAddr = GCPhysMessageFrameAddr;
                pLsiReq->fBIOS                  = false;
                pLsiReq->GCPhysSgStart          = GCPhysMessageFrameAddr + sizeof(MptSCSIIORequest);
                pLsiReq->cChainOffset           = pGuestReq->SCSIIO.u8ChainOffset;
                if (pLsiReq->cChainOffset)
                    pLsiReq->cChainOffset = pLsiReq->cChainOffset * sizeof(uint32_t) - sizeof(MptSCSIIORequest);
                memcpy(&pLsiReq->GuestRequest, pGuestReq, sizeof(MptRequestUnion));
                RT_BZERO(&pLsiReq->abSenseBuffer[0], sizeof(pLsiReq->abSenseBuffer));

                PDMMEDIAEXIOREQSCSITXDIR enmXferDir = PDMMEDIAEXIOREQSCSITXDIR_UNKNOWN;
                uint8_t uDataDirection = MPT_SCSIIO_REQUEST_CONTROL_TXDIR_GET(pLsiReq->GuestRequest.SCSIIO.u32Control);

                 /*
                  * Keep the direction to unknown if there is a mismatch between the data length
                  * and the transfer direction bit.
                  * The Solaris 9 driver is buggy and sets it to none for INQUIRY requests.
                  */
                if (   uDataDirection == MPT_SCSIIO_REQUEST_CONTROL_TXDIR_NONE
                    && pLsiReq->GuestRequest.SCSIIO.u32DataLength == 0)
                    enmXferDir = PDMMEDIAEXIOREQSCSITXDIR_NONE;
                else if (uDataDirection == MPT_SCSIIO_REQUEST_CONTROL_TXDIR_WRITE)
                    enmXferDir = PDMMEDIAEXIOREQSCSITXDIR_TO_DEVICE;
                else if (uDataDirection == MPT_SCSIIO_REQUEST_CONTROL_TXDIR_READ)
                    enmXferDir = PDMMEDIAEXIOREQSCSITXDIR_FROM_DEVICE;

                ASMAtomicIncU32(&pTgtDev->cOutstandingRequests);
                rc = pTgtDev->pDrvMediaEx->pfnIoReqSendScsiCmd(pTgtDev->pDrvMediaEx, pLsiReq->hIoReq, pLsiReq->GuestRequest.SCSIIO.au8LUN[1],
                                                               &pLsiReq->GuestRequest.SCSIIO.au8CDB[0], pLsiReq->GuestRequest.SCSIIO.u8CDBLength,
                                                               enmXferDir, pLsiReq->GuestRequest.SCSIIO.u32DataLength,
                                                               &pLsiReq->abSenseBuffer[0], sizeof(pLsiReq->abSenseBuffer), &pLsiReq->u8ScsiSts,
                                                               30 * RT_MS_1SEC);
                if (rc != VINF_PDM_MEDIAEX_IOREQ_IN_PROGRESS)
                    lsilogicR3ReqComplete(pThis, pLsiReq, rc);

                return VINF_SUCCESS;
            }
            else
                IOCReply.SCSIIOError.u16IOCStatus = MPT_SCSI_IO_ERROR_IOCSTATUS_DEVICE_NOT_THERE;
        }
        else
        {
            /* Device is not present report SCSI selection timeout. */
            IOCReply.SCSIIOError.u16IOCStatus = MPT_SCSI_IO_ERROR_IOCSTATUS_DEVICE_NOT_THERE;
        }
    }
    else
    {
        /* Report out of bounds target ID or bus. */
        if (pGuestReq->SCSIIO.u8Bus != 0)
            IOCReply.SCSIIOError.u16IOCStatus = MPT_SCSI_IO_ERROR_IOCSTATUS_INVALID_BUS;
        else
            IOCReply.SCSIIOError.u16IOCStatus = MPT_SCSI_IO_ERROR_IOCSTATUS_INVALID_TARGETID;
    }

    static int g_cLogged = 0;

    if (g_cLogged++ < MAX_REL_LOG_ERRORS)
    {
        LogRel(("LsiLogic#%d: %d/%d (Bus/Target) doesn't exist\n", pThis->CTX_SUFF(pDevIns)->iInstance,
                pGuestReq->SCSIIO.u8TargetID, pGuestReq->SCSIIO.u8Bus));
        /* Log the CDB too  */
        LogRel(("LsiLogic#%d: Guest issued CDB {%#x",
                pThis->CTX_SUFF(pDevIns)->iInstance, pGuestReq->SCSIIO.au8CDB[0]));
        for (unsigned i = 1; i < pGuestReq->SCSIIO.u8CDBLength; i++)
            LogRel((", %#x", pGuestReq->SCSIIO.au8CDB[i]));
        LogRel(("}\n"));
    }

    /* The rest is equal to both errors. */
    IOCReply.SCSIIOError.u8TargetID          = pGuestReq->SCSIIO.u8TargetID;
    IOCReply.SCSIIOError.u8Bus               = pGuestReq->SCSIIO.u8Bus;
    IOCReply.SCSIIOError.u8MessageLength     = sizeof(MptSCSIIOErrorReply) / 4;
    IOCReply.SCSIIOError.u8Function          = pGuestReq->SCSIIO.u8Function;
    IOCReply.SCSIIOError.u8CDBLength         = pGuestReq->SCSIIO.u8CDBLength;
    IOCReply.SCSIIOError.u8SenseBufferLength = pGuestReq->SCSIIO.u8SenseBufferLength;
    IOCReply.SCSIIOError.u32MessageContext   = pGuestReq->SCSIIO.u32MessageContext;
    IOCReply.SCSIIOError.u8SCSIStatus        = SCSI_STATUS_OK;
    IOCReply.SCSIIOError.u8SCSIState         = MPT_SCSI_IO_ERROR_SCSI_STATE_TERMINATED;
    IOCReply.SCSIIOError.u32IOCLogInfo       = 0;
    IOCReply.SCSIIOError.u32TransferCount    = 0;
    IOCReply.SCSIIOError.u32SenseCount       = 0;
    IOCReply.SCSIIOError.u32ResponseInfo     = 0;

    lsilogicFinishAddressReply(pThis, &IOCReply, false);

    return rc;
}


/**
 * @interface_method_impl{PDMIMEDIAPORT,pfnQueryDeviceLocation}
 */
static DECLCALLBACK(int) lsilogicR3QueryDeviceLocation(PPDMIMEDIAPORT pInterface, const char **ppcszController,
                                                       uint32_t *piInstance, uint32_t *piLUN)
{
    PLSILOGICDEVICE pTgtDev = RT_FROM_MEMBER(pInterface, LSILOGICDEVICE, IMediaPort);
    PPDMDEVINS pDevIns = pTgtDev->CTX_SUFF(pLsiLogic)->CTX_SUFF(pDevIns);

    AssertPtrReturn(ppcszController, VERR_INVALID_POINTER);
    AssertPtrReturn(piInstance, VERR_INVALID_POINTER);
    AssertPtrReturn(piLUN, VERR_INVALID_POINTER);

    *ppcszController = pDevIns->pReg->szName;
    *piInstance = pDevIns->iInstance;
    *piLUN = pTgtDev->iLUN;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqCopyFromBuf}
 */
static DECLCALLBACK(int) lsilogicR3IoReqCopyFromBuf(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                    void *pvIoReqAlloc, uint32_t offDst, PRTSGBUF pSgBuf,
                                                    size_t cbCopy)
{
    RT_NOREF1(hIoReq);
    PLSILOGICDEVICE pTgtDev = RT_FROM_MEMBER(pInterface, LSILOGICDEVICE, IMediaExPort);
    PLSILOGICREQ pReq = (PLSILOGICREQ)pvIoReqAlloc;

    size_t cbCopied = 0;
    if (RT_UNLIKELY(pReq->fBIOS))
        cbCopied = vboxscsiCopyToBuf(&pTgtDev->CTX_SUFF(pLsiLogic)->VBoxSCSI, pSgBuf, offDst, cbCopy);
    else
        cbCopied = lsilogicR3CopySgBufToGuest(pTgtDev->CTX_SUFF(pLsiLogic), pReq, pSgBuf, offDst, cbCopy);
    return cbCopied == cbCopy ? VINF_SUCCESS : VERR_PDM_MEDIAEX_IOBUF_OVERFLOW;
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqCopyToBuf}
 */
static DECLCALLBACK(int) lsilogicR3IoReqCopyToBuf(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                  void *pvIoReqAlloc, uint32_t offSrc, PRTSGBUF pSgBuf,
                                                  size_t cbCopy)
{
    RT_NOREF1(hIoReq);
    PLSILOGICDEVICE pTgtDev = RT_FROM_MEMBER(pInterface, LSILOGICDEVICE, IMediaExPort);
    PLSILOGICREQ pReq = (PLSILOGICREQ)pvIoReqAlloc;

    size_t cbCopied = 0;
    if (RT_UNLIKELY(pReq->fBIOS))
        cbCopied = vboxscsiCopyFromBuf(&pTgtDev->CTX_SUFF(pLsiLogic)->VBoxSCSI, pSgBuf, offSrc, cbCopy);
    else
        cbCopied = lsilogicR3CopySgBufFromGuest(pTgtDev->CTX_SUFF(pLsiLogic), pReq, pSgBuf, offSrc, cbCopy);
    return cbCopied == cbCopy ? VINF_SUCCESS : VERR_PDM_MEDIAEX_IOBUF_UNDERRUN;
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqCompleteNotify}
 */
static DECLCALLBACK(int) lsilogicR3IoReqCompleteNotify(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                       void *pvIoReqAlloc, int rcReq)
{
    RT_NOREF(hIoReq);
    PLSILOGICDEVICE pTgtDev = RT_FROM_MEMBER(pInterface, LSILOGICDEVICE, IMediaExPort);
    lsilogicR3ReqComplete(pTgtDev->CTX_SUFF(pLsiLogic), (PLSILOGICREQ)pvIoReqAlloc, rcReq);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqStateChanged}
 */
static DECLCALLBACK(void) lsilogicR3IoReqStateChanged(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                      void *pvIoReqAlloc, PDMMEDIAEXIOREQSTATE enmState)
{
    RT_NOREF3(hIoReq, pvIoReqAlloc, enmState);
    PLSILOGICDEVICE pTgtDev = RT_FROM_MEMBER(pInterface, LSILOGICDEVICE, IMediaExPort);

    switch (enmState)
    {
        case PDMMEDIAEXIOREQSTATE_SUSPENDED:
        {
            /* Make sure the request is not accounted for so the VM can suspend successfully. */
            uint32_t cTasksActive = ASMAtomicDecU32(&pTgtDev->cOutstandingRequests);
            if (!cTasksActive && pTgtDev->CTX_SUFF(pLsiLogic)->fSignalIdle)
                PDMDevHlpAsyncNotificationCompleted(pTgtDev->CTX_SUFF(pLsiLogic)->pDevInsR3);
            break;
        }
        case PDMMEDIAEXIOREQSTATE_ACTIVE:
            /* Make sure the request is accounted for so the VM suspends only when the request is complete. */
            ASMAtomicIncU32(&pTgtDev->cOutstandingRequests);
            break;
        default:
            AssertMsgFailed(("Invalid request state given %u\n", enmState));
    }
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnMediumEjected}
 */
static DECLCALLBACK(void) lsilogicR3MediumEjected(PPDMIMEDIAEXPORT pInterface)
{
    PLSILOGICDEVICE pTgtDev = RT_FROM_MEMBER(pInterface, LSILOGICDEVICE, IMediaExPort);
    PLSILOGICSCSI pThis = pTgtDev->CTX_SUFF(pLsiLogic);

    if (pThis->pMediaNotify)
    {
        int rc = VMR3ReqCallNoWait(PDMDevHlpGetVM(pThis->CTX_SUFF(pDevIns)), VMCPUID_ANY,
                                   (PFNRT)pThis->pMediaNotify->pfnEjected, 2,
                                   pThis->pMediaNotify, pTgtDev->iLUN);
        AssertRC(rc);
    }
}


/**
 * Return the configuration page header and data
 * which matches the given page type and number.
 *
 * @returns VINF_SUCCESS if successful
 *          VERR_NOT_FOUND if the requested page could be found.
 * @param   pThis         The LsiLogic controller instance data.
 * @param   pPages        The pages supported by the controller.
 * @param   u8PageNumber  Number of the page to get.
 * @param   ppPageHeader  Where to store the pointer to the page header.
 * @param   ppbPageData   Where to store the pointer to the page data.
 * @param   pcbPage       Where to store the size of the page data in bytes on success.
 */
static int lsilogicR3ConfigurationIOUnitPageGetFromNumber(PLSILOGICSCSI pThis,
                                                          PMptConfigurationPagesSupported pPages,
                                                          uint8_t u8PageNumber,
                                                          PMptConfigurationPageHeader *ppPageHeader,
                                                          uint8_t **ppbPageData, size_t *pcbPage)
{
    RT_NOREF(pThis);
    int rc = VINF_SUCCESS;

    AssertPtr(ppPageHeader); Assert(ppbPageData);

    switch (u8PageNumber)
    {
        case 0:
            *ppPageHeader = &pPages->IOUnitPage0.u.fields.Header;
            *ppbPageData  =  pPages->IOUnitPage0.u.abPageData;
            *pcbPage      = sizeof(pPages->IOUnitPage0);
            break;
        case 1:
            *ppPageHeader = &pPages->IOUnitPage1.u.fields.Header;
            *ppbPageData  =  pPages->IOUnitPage1.u.abPageData;
            *pcbPage      = sizeof(pPages->IOUnitPage1);
            break;
        case 2:
            *ppPageHeader = &pPages->IOUnitPage2.u.fields.Header;
            *ppbPageData  =  pPages->IOUnitPage2.u.abPageData;
            *pcbPage      = sizeof(pPages->IOUnitPage2);
            break;
        case 3:
            *ppPageHeader = &pPages->IOUnitPage3.u.fields.Header;
            *ppbPageData  =  pPages->IOUnitPage3.u.abPageData;
            *pcbPage      = sizeof(pPages->IOUnitPage3);
            break;
        case 4:
            *ppPageHeader = &pPages->IOUnitPage4.u.fields.Header;
            *ppbPageData  =  pPages->IOUnitPage4.u.abPageData;
            *pcbPage      = sizeof(pPages->IOUnitPage4);
            break;
        default:
            rc = VERR_NOT_FOUND;
    }

    return rc;
}

/**
 * Return the configuration page header and data
 * which matches the given page type and number.
 *
 * @returns VINF_SUCCESS if successful
 *          VERR_NOT_FOUND if the requested page could be found.
 * @param   pThis         The LsiLogic controller instance data.
 * @param   pPages        The pages supported by the controller.
 * @param   u8PageNumber  Number of the page to get.
 * @param   ppPageHeader  Where to store the pointer to the page header.
 * @param   ppbPageData   Where to store the pointer to the page data.
 * @param   pcbPage       Where to store the size of the page data in bytes on success.
 */
static int lsilogicR3ConfigurationIOCPageGetFromNumber(PLSILOGICSCSI pThis,
                                                       PMptConfigurationPagesSupported pPages,
                                                       uint8_t u8PageNumber,
                                                       PMptConfigurationPageHeader *ppPageHeader,
                                                       uint8_t **ppbPageData, size_t *pcbPage)
{
    RT_NOREF(pThis);
    int rc = VINF_SUCCESS;

    AssertPtr(ppPageHeader); Assert(ppbPageData);

    switch (u8PageNumber)
    {
        case 0:
            *ppPageHeader = &pPages->IOCPage0.u.fields.Header;
            *ppbPageData  =  pPages->IOCPage0.u.abPageData;
            *pcbPage      = sizeof(pPages->IOCPage0);
            break;
        case 1:
            *ppPageHeader = &pPages->IOCPage1.u.fields.Header;
            *ppbPageData  =  pPages->IOCPage1.u.abPageData;
            *pcbPage      = sizeof(pPages->IOCPage1);
            break;
        case 2:
            *ppPageHeader = &pPages->IOCPage2.u.fields.Header;
            *ppbPageData  =  pPages->IOCPage2.u.abPageData;
            *pcbPage      = sizeof(pPages->IOCPage2);
            break;
        case 3:
            *ppPageHeader = &pPages->IOCPage3.u.fields.Header;
            *ppbPageData  =  pPages->IOCPage3.u.abPageData;
            *pcbPage      = sizeof(pPages->IOCPage3);
            break;
        case 4:
            *ppPageHeader = &pPages->IOCPage4.u.fields.Header;
            *ppbPageData  =  pPages->IOCPage4.u.abPageData;
            *pcbPage      = sizeof(pPages->IOCPage4);
            break;
        case 6:
            *ppPageHeader = &pPages->IOCPage6.u.fields.Header;
            *ppbPageData  =  pPages->IOCPage6.u.abPageData;
            *pcbPage      = sizeof(pPages->IOCPage6);
            break;
        default:
            rc = VERR_NOT_FOUND;
    }

    return rc;
}

/**
 * Return the configuration page header and data
 * which matches the given page type and number.
 *
 * @returns VINF_SUCCESS if successful
 *          VERR_NOT_FOUND if the requested page could be found.
 * @param   pThis         The LsiLogic controller instance data.
 * @param   pPages        The pages supported by the controller.
 * @param   u8PageNumber  Number of the page to get.
 * @param   ppPageHeader  Where to store the pointer to the page header.
 * @param   ppbPageData   Where to store the pointer to the page data.
 * @param   pcbPage       Where to store the size of the page data in bytes on success.
 */
static int lsilogicR3ConfigurationManufacturingPageGetFromNumber(PLSILOGICSCSI pThis,
                                                                 PMptConfigurationPagesSupported pPages,
                                                                 uint8_t u8PageNumber,
                                                                 PMptConfigurationPageHeader *ppPageHeader,
                                                                 uint8_t **ppbPageData, size_t *pcbPage)
{
    int rc = VINF_SUCCESS;

    AssertPtr(ppPageHeader); Assert(ppbPageData);

    switch (u8PageNumber)
    {
        case 0:
            *ppPageHeader = &pPages->ManufacturingPage0.u.fields.Header;
            *ppbPageData  =  pPages->ManufacturingPage0.u.abPageData;
            *pcbPage      = sizeof(pPages->ManufacturingPage0);
            break;
        case 1:
            *ppPageHeader = &pPages->ManufacturingPage1.u.fields.Header;
            *ppbPageData  =  pPages->ManufacturingPage1.u.abPageData;
            *pcbPage      = sizeof(pPages->ManufacturingPage1);
            break;
        case 2:
            *ppPageHeader = &pPages->ManufacturingPage2.u.fields.Header;
            *ppbPageData  =  pPages->ManufacturingPage2.u.abPageData;
            *pcbPage      = sizeof(pPages->ManufacturingPage2);
            break;
        case 3:
            *ppPageHeader = &pPages->ManufacturingPage3.u.fields.Header;
            *ppbPageData  =  pPages->ManufacturingPage3.u.abPageData;
            *pcbPage      = sizeof(pPages->ManufacturingPage3);
            break;
        case 4:
            *ppPageHeader = &pPages->ManufacturingPage4.u.fields.Header;
            *ppbPageData  =  pPages->ManufacturingPage4.u.abPageData;
            *pcbPage      = sizeof(pPages->ManufacturingPage4);
            break;
        case 5:
            *ppPageHeader = &pPages->ManufacturingPage5.u.fields.Header;
            *ppbPageData  =  pPages->ManufacturingPage5.u.abPageData;
            *pcbPage      = sizeof(pPages->ManufacturingPage5);
            break;
        case 6:
            *ppPageHeader = &pPages->ManufacturingPage6.u.fields.Header;
            *ppbPageData  =  pPages->ManufacturingPage6.u.abPageData;
            *pcbPage      = sizeof(pPages->ManufacturingPage6);
            break;
        case 7:
            if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
            {
                *ppPageHeader = &pPages->u.SasPages.pManufacturingPage7->u.fields.Header;
                *ppbPageData  =  pPages->u.SasPages.pManufacturingPage7->u.abPageData;
                *pcbPage      = pPages->u.SasPages.cbManufacturingPage7;
            }
            else
                rc = VERR_NOT_FOUND;
            break;
        case 8:
            *ppPageHeader = &pPages->ManufacturingPage8.u.fields.Header;
            *ppbPageData  =  pPages->ManufacturingPage8.u.abPageData;
            *pcbPage      = sizeof(pPages->ManufacturingPage8);
            break;
        case 9:
            *ppPageHeader = &pPages->ManufacturingPage9.u.fields.Header;
            *ppbPageData  =  pPages->ManufacturingPage9.u.abPageData;
            *pcbPage      = sizeof(pPages->ManufacturingPage9);
            break;
        case 10:
            *ppPageHeader = &pPages->ManufacturingPage10.u.fields.Header;
            *ppbPageData  =  pPages->ManufacturingPage10.u.abPageData;
            *pcbPage      = sizeof(pPages->ManufacturingPage10);
            break;
        default:
            rc = VERR_NOT_FOUND;
    }

    return rc;
}

/**
 * Return the configuration page header and data
 * which matches the given page type and number.
 *
 * @returns VINF_SUCCESS if successful
 *          VERR_NOT_FOUND if the requested page could be found.
 * @param   pThis         The LsiLogic controller instance data.
 * @param   pPages        The pages supported by the controller.
 * @param   u8PageNumber  Number of the page to get.
 * @param   ppPageHeader  Where to store the pointer to the page header.
 * @param   ppbPageData   Where to store the pointer to the page data.
 * @param   pcbPage       Where to store the size of the page data in bytes on success.
 */
static int lsilogicR3ConfigurationBiosPageGetFromNumber(PLSILOGICSCSI pThis,
                                                        PMptConfigurationPagesSupported pPages,
                                                        uint8_t u8PageNumber,
                                                        PMptConfigurationPageHeader *ppPageHeader,
                                                        uint8_t **ppbPageData, size_t *pcbPage)
{
    RT_NOREF(pThis);
    int rc = VINF_SUCCESS;

    AssertPtr(ppPageHeader); Assert(ppbPageData);

    switch (u8PageNumber)
    {
        case 1:
            *ppPageHeader = &pPages->BIOSPage1.u.fields.Header;
            *ppbPageData  =  pPages->BIOSPage1.u.abPageData;
            *pcbPage      = sizeof(pPages->BIOSPage1);
            break;
        case 2:
            *ppPageHeader = &pPages->BIOSPage2.u.fields.Header;
            *ppbPageData  =  pPages->BIOSPage2.u.abPageData;
            *pcbPage      = sizeof(pPages->BIOSPage2);
            break;
        case 4:
            *ppPageHeader = &pPages->BIOSPage4.u.fields.Header;
            *ppbPageData  =  pPages->BIOSPage4.u.abPageData;
            *pcbPage      = sizeof(pPages->BIOSPage4);
            break;
        default:
            rc = VERR_NOT_FOUND;
    }

    return rc;
}

/**
 * Return the configuration page header and data
 * which matches the given page type and number.
 *
 * @returns VINF_SUCCESS if successful
 *          VERR_NOT_FOUND if the requested page could be found.
 * @param   pThis         The LsiLogic controller instance data.
 * @param   pPages        The pages supported by the controller.
 * @param   u8Port        The port to retrieve the page for.
 * @param   u8PageNumber  Number of the page to get.
 * @param   ppPageHeader  Where to store the pointer to the page header.
 * @param   ppbPageData   Where to store the pointer to the page data.
 * @param   pcbPage       Where to store the size of the page data in bytes on success.
 */
static int lsilogicR3ConfigurationSCSISPIPortPageGetFromNumber(PLSILOGICSCSI pThis,
                                                               PMptConfigurationPagesSupported pPages,
                                                               uint8_t u8Port,
                                                               uint8_t u8PageNumber,
                                                               PMptConfigurationPageHeader *ppPageHeader,
                                                               uint8_t **ppbPageData, size_t *pcbPage)
{
    RT_NOREF(pThis);
    int rc = VINF_SUCCESS;
    AssertPtr(ppPageHeader); Assert(ppbPageData);


    if (u8Port >= RT_ELEMENTS(pPages->u.SpiPages.aPortPages))
        return VERR_NOT_FOUND;

    switch (u8PageNumber)
    {
        case 0:
            *ppPageHeader = &pPages->u.SpiPages.aPortPages[u8Port].SCSISPIPortPage0.u.fields.Header;
            *ppbPageData  =  pPages->u.SpiPages.aPortPages[u8Port].SCSISPIPortPage0.u.abPageData;
            *pcbPage      = sizeof(pPages->u.SpiPages.aPortPages[u8Port].SCSISPIPortPage0);
            break;
        case 1:
            *ppPageHeader = &pPages->u.SpiPages.aPortPages[u8Port].SCSISPIPortPage1.u.fields.Header;
            *ppbPageData  =  pPages->u.SpiPages.aPortPages[u8Port].SCSISPIPortPage1.u.abPageData;
            *pcbPage      = sizeof(pPages->u.SpiPages.aPortPages[u8Port].SCSISPIPortPage1);
            break;
        case 2:
            *ppPageHeader = &pPages->u.SpiPages.aPortPages[u8Port].SCSISPIPortPage2.u.fields.Header;
            *ppbPageData  =  pPages->u.SpiPages.aPortPages[u8Port].SCSISPIPortPage2.u.abPageData;
            *pcbPage      = sizeof(pPages->u.SpiPages.aPortPages[u8Port].SCSISPIPortPage2);
            break;
        default:
            rc = VERR_NOT_FOUND;
    }

    return rc;
}

/**
 * Return the configuration page header and data
 * which matches the given page type and number.
 *
 * @returns VINF_SUCCESS if successful
 *          VERR_NOT_FOUND if the requested page could be found.
 * @param   pThis         The LsiLogic controller instance data.
 * @param   pPages        The pages supported by the controller.
 * @param   u8Bus         The bus the device is on the page should be returned.
 * @param   u8TargetID    The target ID of the device to return the page for.
 * @param   u8PageNumber  Number of the page to get.
 * @param   ppPageHeader  Where to store the pointer to the page header.
 * @param   ppbPageData   Where to store the pointer to the page data.
 * @param   pcbPage       Where to store the size of the page data in bytes on success.
 */
static int lsilogicR3ConfigurationSCSISPIDevicePageGetFromNumber(PLSILOGICSCSI pThis,
                                                                 PMptConfigurationPagesSupported pPages,
                                                                 uint8_t u8Bus,
                                                                 uint8_t u8TargetID, uint8_t u8PageNumber,
                                                                 PMptConfigurationPageHeader *ppPageHeader,
                                                                 uint8_t **ppbPageData, size_t *pcbPage)
{
    RT_NOREF(pThis);
    int rc = VINF_SUCCESS;
    AssertPtr(ppPageHeader); Assert(ppbPageData);

    if (u8Bus >= RT_ELEMENTS(pPages->u.SpiPages.aBuses))
        return VERR_NOT_FOUND;

    if (u8TargetID >= RT_ELEMENTS(pPages->u.SpiPages.aBuses[u8Bus].aDevicePages))
        return VERR_NOT_FOUND;

    switch (u8PageNumber)
    {
        case 0:
            *ppPageHeader = &pPages->u.SpiPages.aBuses[u8Bus].aDevicePages[u8TargetID].SCSISPIDevicePage0.u.fields.Header;
            *ppbPageData  =  pPages->u.SpiPages.aBuses[u8Bus].aDevicePages[u8TargetID].SCSISPIDevicePage0.u.abPageData;
            *pcbPage      = sizeof(pPages->u.SpiPages.aBuses[u8Bus].aDevicePages[u8TargetID].SCSISPIDevicePage0);
            break;
        case 1:
            *ppPageHeader = &pPages->u.SpiPages.aBuses[u8Bus].aDevicePages[u8TargetID].SCSISPIDevicePage1.u.fields.Header;
            *ppbPageData  =  pPages->u.SpiPages.aBuses[u8Bus].aDevicePages[u8TargetID].SCSISPIDevicePage1.u.abPageData;
            *pcbPage      = sizeof(pPages->u.SpiPages.aBuses[u8Bus].aDevicePages[u8TargetID].SCSISPIDevicePage1);
            break;
        case 2:
            *ppPageHeader = &pPages->u.SpiPages.aBuses[u8Bus].aDevicePages[u8TargetID].SCSISPIDevicePage2.u.fields.Header;
            *ppbPageData  =  pPages->u.SpiPages.aBuses[u8Bus].aDevicePages[u8TargetID].SCSISPIDevicePage2.u.abPageData;
            *pcbPage      = sizeof(pPages->u.SpiPages.aBuses[u8Bus].aDevicePages[u8TargetID].SCSISPIDevicePage2);
            break;
        case 3:
            *ppPageHeader = &pPages->u.SpiPages.aBuses[u8Bus].aDevicePages[u8TargetID].SCSISPIDevicePage3.u.fields.Header;
            *ppbPageData  =  pPages->u.SpiPages.aBuses[u8Bus].aDevicePages[u8TargetID].SCSISPIDevicePage3.u.abPageData;
            *pcbPage      = sizeof(pPages->u.SpiPages.aBuses[u8Bus].aDevicePages[u8TargetID].SCSISPIDevicePage3);
            break;
        default:
            rc = VERR_NOT_FOUND;
    }

    return rc;
}

static int lsilogicR3ConfigurationSASIOUnitPageGetFromNumber(PLSILOGICSCSI pThis,
                                                             PMptConfigurationPagesSupported pPages,
                                                             uint8_t u8PageNumber,
                                                             PMptExtendedConfigurationPageHeader *ppPageHeader,
                                                             uint8_t **ppbPageData, size_t *pcbPage)
{
    RT_NOREF(pThis);
    int rc = VINF_SUCCESS;

    switch (u8PageNumber)
    {
        case 0:
            *ppPageHeader = &pPages->u.SasPages.pSASIOUnitPage0->u.fields.ExtHeader;
            *ppbPageData  = pPages->u.SasPages.pSASIOUnitPage0->u.abPageData;
            *pcbPage      = pPages->u.SasPages.cbSASIOUnitPage0;
            break;
        case 1:
            *ppPageHeader = &pPages->u.SasPages.pSASIOUnitPage1->u.fields.ExtHeader;
            *ppbPageData  =  pPages->u.SasPages.pSASIOUnitPage1->u.abPageData;
            *pcbPage      = pPages->u.SasPages.cbSASIOUnitPage1;
            break;
        case 2:
            *ppPageHeader = &pPages->u.SasPages.SASIOUnitPage2.u.fields.ExtHeader;
            *ppbPageData  =  pPages->u.SasPages.SASIOUnitPage2.u.abPageData;
            *pcbPage      = sizeof(pPages->u.SasPages.SASIOUnitPage2);
            break;
        case 3:
            *ppPageHeader = &pPages->u.SasPages.SASIOUnitPage3.u.fields.ExtHeader;
            *ppbPageData  =  pPages->u.SasPages.SASIOUnitPage3.u.abPageData;
            *pcbPage      = sizeof(pPages->u.SasPages.SASIOUnitPage3);
            break;
        default:
            rc = VERR_NOT_FOUND;
    }

    return rc;
}

static int lsilogicR3ConfigurationSASPHYPageGetFromNumber(PLSILOGICSCSI pThis,
                                                          PMptConfigurationPagesSupported pPages,
                                                          uint8_t u8PageNumber,
                                                          MptConfigurationPageAddress PageAddress,
                                                          PMptExtendedConfigurationPageHeader *ppPageHeader,
                                                          uint8_t **ppbPageData, size_t *pcbPage)
{
    RT_NOREF(pThis);
    int rc = VINF_SUCCESS;
    uint8_t uAddressForm = MPT_CONFIGURATION_PAGE_ADDRESS_GET_SAS_FORM(PageAddress);
    PMptConfigurationPagesSas pPagesSas = &pPages->u.SasPages;
    PMptPHY pPHYPages = NULL;

    Log(("Address form %d\n", uAddressForm));

    if (uAddressForm == 0) /* PHY number */
    {
        uint8_t u8PhyNumber = PageAddress.SASPHY.Form0.u8PhyNumber;

        Log(("PHY number %d\n", u8PhyNumber));

        if (u8PhyNumber >= pPagesSas->cPHYs)
            return VERR_NOT_FOUND;

        pPHYPages = &pPagesSas->paPHYs[u8PhyNumber];
    }
    else if (uAddressForm == 1) /* Index form */
    {
        uint16_t u16Index = PageAddress.SASPHY.Form1.u16Index;

        Log(("PHY index %d\n", u16Index));

        if (u16Index >= pPagesSas->cPHYs)
            return VERR_NOT_FOUND;

        pPHYPages = &pPagesSas->paPHYs[u16Index];
    }
    else
        rc = VERR_NOT_FOUND; /* Correct? */

    if (pPHYPages)
    {
        switch (u8PageNumber)
        {
            case 0:
                *ppPageHeader = &pPHYPages->SASPHYPage0.u.fields.ExtHeader;
                *ppbPageData  = pPHYPages->SASPHYPage0.u.abPageData;
                *pcbPage      = sizeof(pPHYPages->SASPHYPage0);
                break;
            case 1:
                *ppPageHeader = &pPHYPages->SASPHYPage1.u.fields.ExtHeader;
                *ppbPageData  =  pPHYPages->SASPHYPage1.u.abPageData;
                *pcbPage      = sizeof(pPHYPages->SASPHYPage1);
                break;
            default:
                rc = VERR_NOT_FOUND;
        }
    }
    else
        rc = VERR_NOT_FOUND;

    return rc;
}

static int lsilogicR3ConfigurationSASDevicePageGetFromNumber(PLSILOGICSCSI pThis,
                                                             PMptConfigurationPagesSupported pPages,
                                                             uint8_t u8PageNumber,
                                                             MptConfigurationPageAddress PageAddress,
                                                             PMptExtendedConfigurationPageHeader *ppPageHeader,
                                                             uint8_t **ppbPageData, size_t *pcbPage)
{
    RT_NOREF(pThis);
    int rc = VINF_SUCCESS;
    uint8_t uAddressForm = MPT_CONFIGURATION_PAGE_ADDRESS_GET_SAS_FORM(PageAddress);
    PMptConfigurationPagesSas pPagesSas = &pPages->u.SasPages;
    PMptSASDevice pSASDevice = NULL;

    Log(("Address form %d\n", uAddressForm));

    if (uAddressForm == 0)
    {
        uint16_t u16Handle = PageAddress.SASDevice.Form0And2.u16Handle;

        Log(("Get next handle %#x\n", u16Handle));

        pSASDevice = pPagesSas->pSASDeviceHead;

        /* Get the first device? */
        if (u16Handle != 0xffff)
        {
            /* No, search for the right one. */

            while (   pSASDevice
                   && pSASDevice->SASDevicePage0.u.fields.u16DevHandle != u16Handle)
                pSASDevice = pSASDevice->pNext;

            if (pSASDevice)
                pSASDevice = pSASDevice->pNext;
        }
    }
    else if (uAddressForm == 1)
    {
        uint8_t u8TargetID = PageAddress.SASDevice.Form1.u8TargetID;
        uint8_t u8Bus      = PageAddress.SASDevice.Form1.u8Bus;

        Log(("u8TargetID=%d u8Bus=%d\n", u8TargetID, u8Bus));

        pSASDevice = pPagesSas->pSASDeviceHead;

        while (   pSASDevice
               && (   pSASDevice->SASDevicePage0.u.fields.u8TargetID != u8TargetID
                   || pSASDevice->SASDevicePage0.u.fields.u8Bus != u8Bus))
            pSASDevice = pSASDevice->pNext;
    }
    else if (uAddressForm == 2)
    {
        uint16_t u16Handle = PageAddress.SASDevice.Form0And2.u16Handle;

        Log(("Handle %#x\n", u16Handle));

        pSASDevice = pPagesSas->pSASDeviceHead;

        while (   pSASDevice
               && pSASDevice->SASDevicePage0.u.fields.u16DevHandle != u16Handle)
            pSASDevice = pSASDevice->pNext;
    }

    if (pSASDevice)
    {
        switch (u8PageNumber)
        {
            case 0:
                *ppPageHeader = &pSASDevice->SASDevicePage0.u.fields.ExtHeader;
                *ppbPageData  =  pSASDevice->SASDevicePage0.u.abPageData;
                *pcbPage      = sizeof(pSASDevice->SASDevicePage0);
                break;
            case 1:
                *ppPageHeader = &pSASDevice->SASDevicePage1.u.fields.ExtHeader;
                *ppbPageData  =  pSASDevice->SASDevicePage1.u.abPageData;
                *pcbPage      = sizeof(pSASDevice->SASDevicePage1);
                break;
            case 2:
                *ppPageHeader = &pSASDevice->SASDevicePage2.u.fields.ExtHeader;
                *ppbPageData  =  pSASDevice->SASDevicePage2.u.abPageData;
                *pcbPage      = sizeof(pSASDevice->SASDevicePage2);
                break;
            default:
                rc = VERR_NOT_FOUND;
        }
    }
    else
        rc = VERR_NOT_FOUND;

    return rc;
}

/**
 * Returns the extended configuration page header and data.
 * @returns VINF_SUCCESS if successful
 *          VERR_NOT_FOUND if the requested page could be found.
 * @param   pThis               Pointer to the LsiLogic device state.
 * @param   pConfigurationReq   The configuration request.
 * @param   ppPageHeader        Where to return the pointer to the page header on success.
 * @param   ppbPageData         Where to store the pointer to the page data.
 * @param   pcbPage             Where to store the size of the page in bytes.
 */
static int lsilogicR3ConfigurationPageGetExtended(PLSILOGICSCSI pThis, PMptConfigurationRequest pConfigurationReq,
                                                  PMptExtendedConfigurationPageHeader *ppPageHeader,
                                                  uint8_t **ppbPageData, size_t *pcbPage)
{
    int rc = VINF_SUCCESS;

    Log(("Extended page requested:\n"));
    Log(("u8ExtPageType=%#x\n", pConfigurationReq->u8ExtPageType));
    Log(("u8ExtPageLength=%d\n", pConfigurationReq->u16ExtPageLength));

    switch (pConfigurationReq->u8ExtPageType)
    {
        case MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASIOUNIT:
        {
            rc = lsilogicR3ConfigurationSASIOUnitPageGetFromNumber(pThis,
                                                                 pThis->pConfigurationPages,
                                                                 pConfigurationReq->u8PageNumber,
                                                                 ppPageHeader, ppbPageData, pcbPage);
            break;
        }
        case MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASPHYS:
        {
            rc = lsilogicR3ConfigurationSASPHYPageGetFromNumber(pThis,
                                                              pThis->pConfigurationPages,
                                                              pConfigurationReq->u8PageNumber,
                                                              pConfigurationReq->PageAddress,
                                                              ppPageHeader, ppbPageData, pcbPage);
            break;
        }
        case MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASDEVICE:
        {
            rc = lsilogicR3ConfigurationSASDevicePageGetFromNumber(pThis,
                                                                 pThis->pConfigurationPages,
                                                                 pConfigurationReq->u8PageNumber,
                                                                 pConfigurationReq->PageAddress,
                                                                 ppPageHeader, ppbPageData, pcbPage);
            break;
        }
        case MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASEXPANDER: /* No expanders supported */
        case MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_ENCLOSURE: /* No enclosures supported */
        default:
            rc = VERR_NOT_FOUND;
    }

    return rc;
}

/**
 * Processes a Configuration request.
 *
 * @returns VBox status code.
 * @param   pThis               Pointer to the LsiLogic device state.
 * @param   pConfigurationReq   Pointer to the request structure.
 * @param   pReply              Pointer to the reply message frame
 */
static int lsilogicR3ProcessConfigurationRequest(PLSILOGICSCSI pThis, PMptConfigurationRequest pConfigurationReq,
                                                 PMptConfigurationReply pReply)
{
    int                                 rc             = VINF_SUCCESS;
    uint8_t                            *pbPageData     = NULL;
    PMptConfigurationPageHeader         pPageHeader    = NULL;
    PMptExtendedConfigurationPageHeader pExtPageHeader = NULL;
    uint8_t                             u8PageType;
    uint8_t                             u8PageAttribute;
    size_t                              cbPage = 0;

    LogFlowFunc(("pThis=%#p\n", pThis));

    u8PageType = MPT_CONFIGURATION_PAGE_TYPE_GET(pConfigurationReq->u8PageType);
    u8PageAttribute = MPT_CONFIGURATION_PAGE_ATTRIBUTE_GET(pConfigurationReq->u8PageType);

    Log(("GuestRequest:\n"));
    Log(("u8Action=%#x\n", pConfigurationReq->u8Action));
    Log(("u8PageType=%#x\n", u8PageType));
    Log(("u8PageNumber=%d\n", pConfigurationReq->u8PageNumber));
    Log(("u8PageLength=%d\n", pConfigurationReq->u8PageLength));
    Log(("u8PageVersion=%d\n", pConfigurationReq->u8PageVersion));

    /* Copy common bits from the request into the reply. */
    pReply->u8MessageLength   = 6; /* 6 32bit D-Words. */
    pReply->u8Action          = pConfigurationReq->u8Action;
    pReply->u8Function        = pConfigurationReq->u8Function;
    pReply->u32MessageContext = pConfigurationReq->u32MessageContext;

    switch (u8PageType)
    {
        case MPT_CONFIGURATION_PAGE_TYPE_IO_UNIT:
        {
            /* Get the page data. */
            rc = lsilogicR3ConfigurationIOUnitPageGetFromNumber(pThis,
                                                              pThis->pConfigurationPages,
                                                              pConfigurationReq->u8PageNumber,
                                                              &pPageHeader, &pbPageData, &cbPage);
            break;
        }
        case MPT_CONFIGURATION_PAGE_TYPE_IOC:
        {
            /* Get the page data. */
            rc = lsilogicR3ConfigurationIOCPageGetFromNumber(pThis,
                                                           pThis->pConfigurationPages,
                                                           pConfigurationReq->u8PageNumber,
                                                           &pPageHeader, &pbPageData, &cbPage);
            break;
        }
        case MPT_CONFIGURATION_PAGE_TYPE_MANUFACTURING:
        {
            /* Get the page data. */
            rc = lsilogicR3ConfigurationManufacturingPageGetFromNumber(pThis,
                                                                     pThis->pConfigurationPages,
                                                                     pConfigurationReq->u8PageNumber,
                                                                     &pPageHeader, &pbPageData, &cbPage);
            break;
        }
        case MPT_CONFIGURATION_PAGE_TYPE_SCSI_SPI_PORT:
        {
            /* Get the page data. */
            rc = lsilogicR3ConfigurationSCSISPIPortPageGetFromNumber(pThis,
                                                                   pThis->pConfigurationPages,
                                                                   pConfigurationReq->PageAddress.MPIPortNumber.u8PortNumber,
                                                                   pConfigurationReq->u8PageNumber,
                                                                   &pPageHeader, &pbPageData, &cbPage);
            break;
        }
        case MPT_CONFIGURATION_PAGE_TYPE_SCSI_SPI_DEVICE:
        {
            /* Get the page data. */
            rc = lsilogicR3ConfigurationSCSISPIDevicePageGetFromNumber(pThis,
                                                                     pThis->pConfigurationPages,
                                                                     pConfigurationReq->PageAddress.BusAndTargetId.u8Bus,
                                                                     pConfigurationReq->PageAddress.BusAndTargetId.u8TargetID,
                                                                     pConfigurationReq->u8PageNumber,
                                                                     &pPageHeader, &pbPageData, &cbPage);
            break;
        }
        case MPT_CONFIGURATION_PAGE_TYPE_BIOS:
        {
            rc = lsilogicR3ConfigurationBiosPageGetFromNumber(pThis,
                                                            pThis->pConfigurationPages,
                                                            pConfigurationReq->u8PageNumber,
                                                            &pPageHeader, &pbPageData, &cbPage);
            break;
        }
        case MPT_CONFIGURATION_PAGE_TYPE_EXTENDED:
        {
            rc = lsilogicR3ConfigurationPageGetExtended(pThis,
                                                      pConfigurationReq,
                                                      &pExtPageHeader, &pbPageData, &cbPage);
            break;
        }
        default:
            rc = VERR_NOT_FOUND;
    }

    if (rc == VERR_NOT_FOUND)
    {
        Log(("Page not found\n"));
        pReply->u8PageType    = pConfigurationReq->u8PageType;
        pReply->u8PageNumber  = pConfigurationReq->u8PageNumber;
        pReply->u8PageLength  = pConfigurationReq->u8PageLength;
        pReply->u8PageVersion = pConfigurationReq->u8PageVersion;
        pReply->u16IOCStatus  = MPT_IOCSTATUS_CONFIG_INVALID_PAGE;
        return VINF_SUCCESS;
    }

    if (u8PageType == MPT_CONFIGURATION_PAGE_TYPE_EXTENDED)
    {
        pReply->u8PageType       = pExtPageHeader->u8PageType;
        pReply->u8PageNumber     = pExtPageHeader->u8PageNumber;
        pReply->u8PageVersion    = pExtPageHeader->u8PageVersion;
        pReply->u8ExtPageType    = pExtPageHeader->u8ExtPageType;
        pReply->u16ExtPageLength = pExtPageHeader->u16ExtPageLength;

        for (int i = 0; i < pExtPageHeader->u16ExtPageLength; i++)
            LogFlowFunc(("PageData[%d]=%#x\n", i, ((uint32_t *)pbPageData)[i]));
    }
    else
    {
        pReply->u8PageType    = pPageHeader->u8PageType;
        pReply->u8PageNumber  = pPageHeader->u8PageNumber;
        pReply->u8PageLength  = pPageHeader->u8PageLength;
        pReply->u8PageVersion = pPageHeader->u8PageVersion;

        for (int i = 0; i < pReply->u8PageLength; i++)
            LogFlowFunc(("PageData[%d]=%#x\n", i, ((uint32_t *)pbPageData)[i]));
    }

    /*
     * Don't use the scatter gather handling code as the configuration request always have only one
     * simple element.
     */
    switch (pConfigurationReq->u8Action)
    {
        case MPT_CONFIGURATION_REQUEST_ACTION_DEFAULT: /* Nothing to do. We are always using the defaults. */
        case MPT_CONFIGURATION_REQUEST_ACTION_HEADER:
        {
            /* Already copied above nothing to do. */
            break;
        }
        case MPT_CONFIGURATION_REQUEST_ACTION_READ_NVRAM:
        case MPT_CONFIGURATION_REQUEST_ACTION_READ_CURRENT:
        case MPT_CONFIGURATION_REQUEST_ACTION_READ_DEFAULT:
        {
            uint32_t cbBuffer = pConfigurationReq->SimpleSGElement.u24Length;
            if (cbBuffer != 0)
            {
                RTGCPHYS GCPhysAddrPageBuffer = pConfigurationReq->SimpleSGElement.u32DataBufferAddressLow;
                if (pConfigurationReq->SimpleSGElement.f64BitAddress)
                    GCPhysAddrPageBuffer |= (uint64_t)pConfigurationReq->SimpleSGElement.u32DataBufferAddressHigh << 32;

                PDMDevHlpPCIPhysWrite(pThis->CTX_SUFF(pDevIns), GCPhysAddrPageBuffer, pbPageData, RT_MIN(cbBuffer, cbPage));
            }
            break;
        }
        case MPT_CONFIGURATION_REQUEST_ACTION_WRITE_CURRENT:
        case MPT_CONFIGURATION_REQUEST_ACTION_WRITE_NVRAM:
        {
            uint32_t cbBuffer = pConfigurationReq->SimpleSGElement.u24Length;
            if (cbBuffer != 0)
            {
                RTGCPHYS GCPhysAddrPageBuffer = pConfigurationReq->SimpleSGElement.u32DataBufferAddressLow;
                if (pConfigurationReq->SimpleSGElement.f64BitAddress)
                    GCPhysAddrPageBuffer |= (uint64_t)pConfigurationReq->SimpleSGElement.u32DataBufferAddressHigh << 32;

                LogFlow(("cbBuffer=%u cbPage=%u\n", cbBuffer, cbPage));

                PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), GCPhysAddrPageBuffer, pbPageData,
                                  RT_MIN(cbBuffer, cbPage));
            }
            break;
        }
        default:
            AssertMsgFailed(("todo\n"));
    }

    return VINF_SUCCESS;
}

/**
 * Initializes the configuration pages for the SPI SCSI controller.
 *
 * @returns nothing
 * @param   pThis       Pointer to the LsiLogic device state.
 */
static void lsilogicR3InitializeConfigurationPagesSpi(PLSILOGICSCSI pThis)
{
    PMptConfigurationPagesSpi pPages = &pThis->pConfigurationPages->u.SpiPages;

    AssertMsg(pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI, ("Controller is not the SPI SCSI one\n"));

    LogFlowFunc(("pThis=%#p\n", pThis));

    /* Clear everything first. */
    memset(pPages, 0, sizeof(MptConfigurationPagesSpi));

    for (unsigned i = 0; i < RT_ELEMENTS(pPages->aPortPages); i++)
    {
        /* SCSI-SPI port page 0. */
        pPages->aPortPages[i].SCSISPIPortPage0.u.fields.Header.u8PageType   =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY
                                                                              | MPT_CONFIGURATION_PAGE_TYPE_SCSI_SPI_PORT;
        pPages->aPortPages[i].SCSISPIPortPage0.u.fields.Header.u8PageNumber = 0;
        pPages->aPortPages[i].SCSISPIPortPage0.u.fields.Header.u8PageLength = sizeof(MptConfigurationPageSCSISPIPort0) / 4;
        pPages->aPortPages[i].SCSISPIPortPage0.u.fields.fInformationUnitTransfersCapable = true;
        pPages->aPortPages[i].SCSISPIPortPage0.u.fields.fDTCapable                       = true;
        pPages->aPortPages[i].SCSISPIPortPage0.u.fields.fQASCapable                      = true;
        pPages->aPortPages[i].SCSISPIPortPage0.u.fields.u8MinimumSynchronousTransferPeriod =  0;
        pPages->aPortPages[i].SCSISPIPortPage0.u.fields.u8MaximumSynchronousOffset         = 0xff;
        pPages->aPortPages[i].SCSISPIPortPage0.u.fields.fWide                              = true;
        pPages->aPortPages[i].SCSISPIPortPage0.u.fields.fAIPCapable                        = true;
        pPages->aPortPages[i].SCSISPIPortPage0.u.fields.u2SignalingType                    = 0x3; /* Single Ended. */

        /* SCSI-SPI port page 1. */
        pPages->aPortPages[i].SCSISPIPortPage1.u.fields.Header.u8PageType   =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE
                                                                              | MPT_CONFIGURATION_PAGE_TYPE_SCSI_SPI_PORT;
        pPages->aPortPages[i].SCSISPIPortPage1.u.fields.Header.u8PageNumber = 1;
        pPages->aPortPages[i].SCSISPIPortPage1.u.fields.Header.u8PageLength = sizeof(MptConfigurationPageSCSISPIPort1) / 4;
        pPages->aPortPages[i].SCSISPIPortPage1.u.fields.u8SCSIID                  = 7;
        pPages->aPortPages[i].SCSISPIPortPage1.u.fields.u16PortResponseIDsBitmask = (1 << 7);
        pPages->aPortPages[i].SCSISPIPortPage1.u.fields.u32OnBusTimerValue        =  0;

        /* SCSI-SPI port page 2. */
        pPages->aPortPages[i].SCSISPIPortPage2.u.fields.Header.u8PageType   =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE
                                                                              | MPT_CONFIGURATION_PAGE_TYPE_SCSI_SPI_PORT;
        pPages->aPortPages[i].SCSISPIPortPage2.u.fields.Header.u8PageNumber = 2;
        pPages->aPortPages[i].SCSISPIPortPage2.u.fields.Header.u8PageLength = sizeof(MptConfigurationPageSCSISPIPort2) / 4;
        pPages->aPortPages[i].SCSISPIPortPage2.u.fields.u4HostSCSIID           = 7;
        pPages->aPortPages[i].SCSISPIPortPage2.u.fields.u2InitializeHBA        = 0x3;
        pPages->aPortPages[i].SCSISPIPortPage2.u.fields.fTerminationDisabled   = true;
        for (unsigned iDevice = 0; iDevice < RT_ELEMENTS(pPages->aPortPages[i].SCSISPIPortPage2.u.fields.aDeviceSettings); iDevice++)
        {
            pPages->aPortPages[i].SCSISPIPortPage2.u.fields.aDeviceSettings[iDevice].fBootChoice   = true;
        }
        /* Everything else 0 for now. */
    }

    for (unsigned uBusCurr = 0; uBusCurr < RT_ELEMENTS(pPages->aBuses); uBusCurr++)
    {
        for (unsigned uDeviceCurr = 0; uDeviceCurr < RT_ELEMENTS(pPages->aBuses[uBusCurr].aDevicePages); uDeviceCurr++)
        {
            /* SCSI-SPI device page 0. */
            pPages->aBuses[uBusCurr].aDevicePages[uDeviceCurr].SCSISPIDevicePage0.u.fields.Header.u8PageType   =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY
                                                                                                                 | MPT_CONFIGURATION_PAGE_TYPE_SCSI_SPI_DEVICE;
            pPages->aBuses[uBusCurr].aDevicePages[uDeviceCurr].SCSISPIDevicePage0.u.fields.Header.u8PageNumber = 0;
            pPages->aBuses[uBusCurr].aDevicePages[uDeviceCurr].SCSISPIDevicePage0.u.fields.Header.u8PageLength = sizeof(MptConfigurationPageSCSISPIDevice0) / 4;
            /* Everything else 0 for now. */

            /* SCSI-SPI device page 1. */
            pPages->aBuses[uBusCurr].aDevicePages[uDeviceCurr].SCSISPIDevicePage1.u.fields.Header.u8PageType   =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE
                                                                                                                 | MPT_CONFIGURATION_PAGE_TYPE_SCSI_SPI_DEVICE;
            pPages->aBuses[uBusCurr].aDevicePages[uDeviceCurr].SCSISPIDevicePage1.u.fields.Header.u8PageNumber = 1;
            pPages->aBuses[uBusCurr].aDevicePages[uDeviceCurr].SCSISPIDevicePage1.u.fields.Header.u8PageLength = sizeof(MptConfigurationPageSCSISPIDevice1) / 4;
            /* Everything else 0 for now. */

            /* SCSI-SPI device page 2. */
            pPages->aBuses[uBusCurr].aDevicePages[uDeviceCurr].SCSISPIDevicePage2.u.fields.Header.u8PageType   =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE
                                                                                                                 | MPT_CONFIGURATION_PAGE_TYPE_SCSI_SPI_DEVICE;
            pPages->aBuses[uBusCurr].aDevicePages[uDeviceCurr].SCSISPIDevicePage2.u.fields.Header.u8PageNumber = 2;
            pPages->aBuses[uBusCurr].aDevicePages[uDeviceCurr].SCSISPIDevicePage2.u.fields.Header.u8PageLength = sizeof(MptConfigurationPageSCSISPIDevice2) / 4;
            /* Everything else 0 for now. */

            pPages->aBuses[uBusCurr].aDevicePages[uDeviceCurr].SCSISPIDevicePage3.u.fields.Header.u8PageType   =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY
                                                                                                                 | MPT_CONFIGURATION_PAGE_TYPE_SCSI_SPI_DEVICE;
            pPages->aBuses[uBusCurr].aDevicePages[uDeviceCurr].SCSISPIDevicePage3.u.fields.Header.u8PageNumber = 3;
            pPages->aBuses[uBusCurr].aDevicePages[uDeviceCurr].SCSISPIDevicePage3.u.fields.Header.u8PageLength = sizeof(MptConfigurationPageSCSISPIDevice3) / 4;
            /* Everything else 0 for now. */
        }
    }
}

/**
 * Generates a handle.
 *
 * @returns the handle.
 * @param   pThis       Pointer to the LsiLogic device state.
 */
DECLINLINE(uint16_t) lsilogicGetHandle(PLSILOGICSCSI pThis)
{
    uint16_t u16Handle = pThis->u16NextHandle++;
    return u16Handle;
}

/**
 * Generates a SAS address (WWID)
 *
 * @returns nothing.
 * @param   pSASAddress Pointer to an unitialised SAS address.
 * @param   iId         iId which will go into the address.
 *
 * @todo Generate better SAS addresses. (Request a block from SUN probably)
 */
void lsilogicSASAddressGenerate(PSASADDRESS pSASAddress, unsigned iId)
{
    pSASAddress->u8Address[0] = (0x5 << 5);
    pSASAddress->u8Address[1] = 0x01;
    pSASAddress->u8Address[2] = 0x02;
    pSASAddress->u8Address[3] = 0x03;
    pSASAddress->u8Address[4] = 0x04;
    pSASAddress->u8Address[5] = 0x05;
    pSASAddress->u8Address[6] = 0x06;
    pSASAddress->u8Address[7] = iId;
}

/**
 * Initializes the configuration pages for the SAS SCSI controller.
 *
 * @returns nothing
 * @param   pThis       Pointer to the LsiLogic device state.
 */
static void lsilogicR3InitializeConfigurationPagesSas(PLSILOGICSCSI pThis)
{
    PMptConfigurationPagesSas pPages = &pThis->pConfigurationPages->u.SasPages;

    AssertMsg(pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS, ("Controller is not the SAS SCSI one\n"));

    LogFlowFunc(("pThis=%#p\n", pThis));

    /* Manufacturing Page 7 - Connector settings. */
    pPages->cbManufacturingPage7 = LSILOGICSCSI_MANUFACTURING7_GET_SIZE(pThis->cPorts);
    PMptConfigurationPageManufacturing7 pManufacturingPage7 = (PMptConfigurationPageManufacturing7)RTMemAllocZ(pPages->cbManufacturingPage7);
    AssertPtr(pManufacturingPage7);
    MPT_CONFIG_PAGE_HEADER_INIT_MANUFACTURING(pManufacturingPage7,
                                              0, 7,
                                              MPT_CONFIGURATION_PAGE_ATTRIBUTE_PERSISTENT_READONLY);
    /* Set size manually. */
    if (pPages->cbManufacturingPage7 / 4 > 255)
        pManufacturingPage7->u.fields.Header.u8PageLength = 255;
    else
        pManufacturingPage7->u.fields.Header.u8PageLength = pPages->cbManufacturingPage7 / 4;
    pManufacturingPage7->u.fields.u8NumPhys = pThis->cPorts;
    pPages->pManufacturingPage7 = pManufacturingPage7;

    /* SAS I/O unit page 0 - Port specific information. */
    pPages->cbSASIOUnitPage0 = LSILOGICSCSI_SASIOUNIT0_GET_SIZE(pThis->cPorts);
    PMptConfigurationPageSASIOUnit0 pSASPage0 = (PMptConfigurationPageSASIOUnit0)RTMemAllocZ(pPages->cbSASIOUnitPage0);
    AssertPtr(pSASPage0);

    MPT_CONFIG_EXTENDED_PAGE_HEADER_INIT(pSASPage0, pPages->cbSASIOUnitPage0,
                                         0, MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY,
                                         MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASIOUNIT);
    pSASPage0->u.fields.u8NumPhys = pThis->cPorts;
    pPages->pSASIOUnitPage0 = pSASPage0;

    /* SAS I/O unit page 1 - Port specific settings. */
    pPages->cbSASIOUnitPage1 = LSILOGICSCSI_SASIOUNIT1_GET_SIZE(pThis->cPorts);
    PMptConfigurationPageSASIOUnit1 pSASPage1 = (PMptConfigurationPageSASIOUnit1)RTMemAllocZ(pPages->cbSASIOUnitPage1);
    AssertPtr(pSASPage1);

    MPT_CONFIG_EXTENDED_PAGE_HEADER_INIT(pSASPage1, pPages->cbSASIOUnitPage1,
                                         1, MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE,
                                         MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASIOUNIT);
    pSASPage1->u.fields.u8NumPhys = pSASPage0->u.fields.u8NumPhys;
    pSASPage1->u.fields.u16ControlFlags = 0;
    pSASPage1->u.fields.u16AdditionalControlFlags = 0;
    pPages->pSASIOUnitPage1 = pSASPage1;

    /* SAS I/O unit page 2 - Port specific information. */
    pPages->SASIOUnitPage2.u.fields.ExtHeader.u8PageType       =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY
                                                                 | MPT_CONFIGURATION_PAGE_TYPE_EXTENDED;
    pPages->SASIOUnitPage2.u.fields.ExtHeader.u8PageNumber     = 2;
    pPages->SASIOUnitPage2.u.fields.ExtHeader.u8ExtPageType    = MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASIOUNIT;
    pPages->SASIOUnitPage2.u.fields.ExtHeader.u16ExtPageLength = sizeof(MptConfigurationPageSASIOUnit2) / 4;

    /* SAS I/O unit page 3 - Port specific information. */
    pPages->SASIOUnitPage3.u.fields.ExtHeader.u8PageType       =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY
                                                                 | MPT_CONFIGURATION_PAGE_TYPE_EXTENDED;
    pPages->SASIOUnitPage3.u.fields.ExtHeader.u8PageNumber     = 3;
    pPages->SASIOUnitPage3.u.fields.ExtHeader.u8ExtPageType    = MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASIOUNIT;
    pPages->SASIOUnitPage3.u.fields.ExtHeader.u16ExtPageLength = sizeof(MptConfigurationPageSASIOUnit3) / 4;

    pPages->cPHYs  = pThis->cPorts;
    pPages->paPHYs = (PMptPHY)RTMemAllocZ(pPages->cPHYs * sizeof(MptPHY));
    AssertPtr(pPages->paPHYs);

    /* Initialize the PHY configuration */
    for (unsigned i = 0; i < pThis->cPorts; i++)
    {
        PMptPHY pPHYPages = &pPages->paPHYs[i];
        uint16_t u16ControllerHandle = lsilogicGetHandle(pThis);

        pManufacturingPage7->u.fields.aPHY[i].u8Location = LSILOGICSCSI_MANUFACTURING7_LOCATION_AUTO;

        pSASPage0->u.fields.aPHY[i].u8Port      = i;
        pSASPage0->u.fields.aPHY[i].u8PortFlags = 0;
        pSASPage0->u.fields.aPHY[i].u8PhyFlags  = 0;
        pSASPage0->u.fields.aPHY[i].u8NegotiatedLinkRate = LSILOGICSCSI_SASIOUNIT0_NEGOTIATED_RATE_FAILED;
        pSASPage0->u.fields.aPHY[i].u32ControllerPhyDeviceInfo = LSILOGICSCSI_SASIOUNIT0_DEVICE_TYPE_SET(LSILOGICSCSI_SASIOUNIT0_DEVICE_TYPE_NO);
        pSASPage0->u.fields.aPHY[i].u16ControllerDevHandle     = u16ControllerHandle;
        pSASPage0->u.fields.aPHY[i].u16AttachedDevHandle       = 0; /* No device attached. */
        pSASPage0->u.fields.aPHY[i].u32DiscoveryStatus         = 0; /* No errors */

        pSASPage1->u.fields.aPHY[i].u8Port           = i;
        pSASPage1->u.fields.aPHY[i].u8PortFlags      = 0;
        pSASPage1->u.fields.aPHY[i].u8PhyFlags       = 0;
        pSASPage1->u.fields.aPHY[i].u8MaxMinLinkRate =   LSILOGICSCSI_SASIOUNIT1_LINK_RATE_MIN_SET(LSILOGICSCSI_SASIOUNIT1_LINK_RATE_15GB)
                                                       | LSILOGICSCSI_SASIOUNIT1_LINK_RATE_MAX_SET(LSILOGICSCSI_SASIOUNIT1_LINK_RATE_30GB);
        pSASPage1->u.fields.aPHY[i].u32ControllerPhyDeviceInfo = LSILOGICSCSI_SASIOUNIT0_DEVICE_TYPE_SET(LSILOGICSCSI_SASIOUNIT0_DEVICE_TYPE_NO);

        /* SAS PHY page 0. */
        pPHYPages->SASPHYPage0.u.fields.ExtHeader.u8PageType       =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY
                                                                          | MPT_CONFIGURATION_PAGE_TYPE_EXTENDED;
        pPHYPages->SASPHYPage0.u.fields.ExtHeader.u8PageNumber     = 0;
        pPHYPages->SASPHYPage0.u.fields.ExtHeader.u8ExtPageType    = MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASPHYS;
        pPHYPages->SASPHYPage0.u.fields.ExtHeader.u16ExtPageLength = sizeof(MptConfigurationPageSASPHY0) / 4;
        pPHYPages->SASPHYPage0.u.fields.u8AttachedPhyIdentifier    = i;
        pPHYPages->SASPHYPage0.u.fields.u32AttachedDeviceInfo      = LSILOGICSCSI_SASPHY0_DEV_INFO_DEVICE_TYPE_SET(LSILOGICSCSI_SASPHY0_DEV_INFO_DEVICE_TYPE_NO);
        pPHYPages->SASPHYPage0.u.fields.u8ProgrammedLinkRate       =   LSILOGICSCSI_SASIOUNIT1_LINK_RATE_MIN_SET(LSILOGICSCSI_SASIOUNIT1_LINK_RATE_15GB)
                                                                     | LSILOGICSCSI_SASIOUNIT1_LINK_RATE_MAX_SET(LSILOGICSCSI_SASIOUNIT1_LINK_RATE_30GB);
        pPHYPages->SASPHYPage0.u.fields.u8HwLinkRate               =   LSILOGICSCSI_SASIOUNIT1_LINK_RATE_MIN_SET(LSILOGICSCSI_SASIOUNIT1_LINK_RATE_15GB)
                                                                     | LSILOGICSCSI_SASIOUNIT1_LINK_RATE_MAX_SET(LSILOGICSCSI_SASIOUNIT1_LINK_RATE_30GB);

        /* SAS PHY page 1. */
        pPHYPages->SASPHYPage1.u.fields.ExtHeader.u8PageType       =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY
                                                                     | MPT_CONFIGURATION_PAGE_TYPE_EXTENDED;
        pPHYPages->SASPHYPage1.u.fields.ExtHeader.u8PageNumber     = 1;
        pPHYPages->SASPHYPage1.u.fields.ExtHeader.u8ExtPageType    = MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASPHYS;
        pPHYPages->SASPHYPage1.u.fields.ExtHeader.u16ExtPageLength = sizeof(MptConfigurationPageSASPHY1) / 4;

        /* Settings for present devices. */
        if (pThis->paDeviceStates[i].pDrvBase)
        {
            uint16_t u16DeviceHandle = lsilogicGetHandle(pThis);
            SASADDRESS SASAddress;
            PMptSASDevice pSASDevice = (PMptSASDevice)RTMemAllocZ(sizeof(MptSASDevice));
            AssertPtr(pSASDevice);

            memset(&SASAddress, 0, sizeof(SASADDRESS));
            lsilogicSASAddressGenerate(&SASAddress, i);

            pSASPage0->u.fields.aPHY[i].u8NegotiatedLinkRate       = LSILOGICSCSI_SASIOUNIT0_NEGOTIATED_RATE_SET(LSILOGICSCSI_SASIOUNIT0_NEGOTIATED_RATE_30GB);
            pSASPage0->u.fields.aPHY[i].u32ControllerPhyDeviceInfo =   LSILOGICSCSI_SASIOUNIT0_DEVICE_TYPE_SET(LSILOGICSCSI_SASIOUNIT0_DEVICE_TYPE_END)
                                                                     | LSILOGICSCSI_SASIOUNIT0_DEVICE_SSP_TARGET;
            pSASPage0->u.fields.aPHY[i].u16AttachedDevHandle       = u16DeviceHandle;
            pSASPage1->u.fields.aPHY[i].u32ControllerPhyDeviceInfo =   LSILOGICSCSI_SASIOUNIT0_DEVICE_TYPE_SET(LSILOGICSCSI_SASIOUNIT0_DEVICE_TYPE_END)
                                                                     | LSILOGICSCSI_SASIOUNIT0_DEVICE_SSP_TARGET;
            pSASPage0->u.fields.aPHY[i].u16ControllerDevHandle     = u16DeviceHandle;

            pPHYPages->SASPHYPage0.u.fields.u32AttachedDeviceInfo  = LSILOGICSCSI_SASPHY0_DEV_INFO_DEVICE_TYPE_SET(LSILOGICSCSI_SASPHY0_DEV_INFO_DEVICE_TYPE_END);
            pPHYPages->SASPHYPage0.u.fields.SASAddress             = SASAddress;
            pPHYPages->SASPHYPage0.u.fields.u16OwnerDevHandle      = u16DeviceHandle;
            pPHYPages->SASPHYPage0.u.fields.u16AttachedDevHandle   = u16DeviceHandle;

            /* SAS device page 0. */
            pSASDevice->SASDevicePage0.u.fields.ExtHeader.u8PageType       =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY
                                                                             | MPT_CONFIGURATION_PAGE_TYPE_EXTENDED;
            pSASDevice->SASDevicePage0.u.fields.ExtHeader.u8PageNumber     = 0;
            pSASDevice->SASDevicePage0.u.fields.ExtHeader.u8ExtPageType    = MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASDEVICE;
            pSASDevice->SASDevicePage0.u.fields.ExtHeader.u16ExtPageLength = sizeof(MptConfigurationPageSASDevice0) / 4;
            pSASDevice->SASDevicePage0.u.fields.SASAddress                 = SASAddress;
            pSASDevice->SASDevicePage0.u.fields.u16ParentDevHandle         = u16ControllerHandle;
            pSASDevice->SASDevicePage0.u.fields.u8PhyNum                   = i;
            pSASDevice->SASDevicePage0.u.fields.u8AccessStatus             = LSILOGICSCSI_SASDEVICE0_STATUS_NO_ERRORS;
            pSASDevice->SASDevicePage0.u.fields.u16DevHandle               = u16DeviceHandle;
            pSASDevice->SASDevicePage0.u.fields.u8TargetID                 = i;
            pSASDevice->SASDevicePage0.u.fields.u8Bus                      = 0;
            pSASDevice->SASDevicePage0.u.fields.u32DeviceInfo              =   LSILOGICSCSI_SASPHY0_DEV_INFO_DEVICE_TYPE_SET(LSILOGICSCSI_SASPHY0_DEV_INFO_DEVICE_TYPE_END)
                                                                             | LSILOGICSCSI_SASIOUNIT0_DEVICE_SSP_TARGET;
            pSASDevice->SASDevicePage0.u.fields.u16Flags                   =   LSILOGICSCSI_SASDEVICE0_FLAGS_DEVICE_PRESENT
                                                                             | LSILOGICSCSI_SASDEVICE0_FLAGS_DEVICE_MAPPED_TO_BUS_AND_TARGET_ID
                                                                             | LSILOGICSCSI_SASDEVICE0_FLAGS_DEVICE_MAPPING_PERSISTENT;
            pSASDevice->SASDevicePage0.u.fields.u8PhysicalPort             = i;

            /* SAS device page 1. */
            pSASDevice->SASDevicePage1.u.fields.ExtHeader.u8PageType       =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY
                                                                             | MPT_CONFIGURATION_PAGE_TYPE_EXTENDED;
            pSASDevice->SASDevicePage1.u.fields.ExtHeader.u8PageNumber     = 1;
            pSASDevice->SASDevicePage1.u.fields.ExtHeader.u8ExtPageType    = MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASDEVICE;
            pSASDevice->SASDevicePage1.u.fields.ExtHeader.u16ExtPageLength = sizeof(MptConfigurationPageSASDevice1) / 4;
            pSASDevice->SASDevicePage1.u.fields.SASAddress                 = SASAddress;
            pSASDevice->SASDevicePage1.u.fields.u16DevHandle               = u16DeviceHandle;
            pSASDevice->SASDevicePage1.u.fields.u8TargetID                 = i;
            pSASDevice->SASDevicePage1.u.fields.u8Bus                      = 0;

            /* SAS device page 2. */
            pSASDevice->SASDevicePage2.u.fields.ExtHeader.u8PageType       =   MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY
                                                                              | MPT_CONFIGURATION_PAGE_TYPE_EXTENDED;
            pSASDevice->SASDevicePage2.u.fields.ExtHeader.u8PageNumber     = 2;
            pSASDevice->SASDevicePage2.u.fields.ExtHeader.u8ExtPageType    = MPT_CONFIGURATION_PAGE_TYPE_EXTENDED_SASDEVICE;
            pSASDevice->SASDevicePage2.u.fields.ExtHeader.u16ExtPageLength = sizeof(MptConfigurationPageSASDevice2) / 4;
            pSASDevice->SASDevicePage2.u.fields.SASAddress                 = SASAddress;

            /* Link into device list. */
            if (!pPages->cDevices)
            {
                pPages->pSASDeviceHead = pSASDevice;
                pPages->pSASDeviceTail = pSASDevice;
                pPages->cDevices = 1;
            }
            else
            {
                pSASDevice->pPrev = pPages->pSASDeviceTail;
                pPages->pSASDeviceTail->pNext = pSASDevice;
                pPages->pSASDeviceTail = pSASDevice;
                pPages->cDevices++;
            }
        }
    }
}

/**
 * Initializes the configuration pages.
 *
 * @returns nothing
 * @param   pThis       Pointer to the LsiLogic device state.
 */
static void lsilogicR3InitializeConfigurationPages(PLSILOGICSCSI pThis)
{
    /* Initialize the common pages. */
    PMptConfigurationPagesSupported pPages = (PMptConfigurationPagesSupported)RTMemAllocZ(sizeof(MptConfigurationPagesSupported));

    pThis->pConfigurationPages = pPages;

    LogFlowFunc(("pThis=%#p\n", pThis));

    /* Clear everything first. */
    memset(pPages, 0, sizeof(MptConfigurationPagesSupported));

    /* Manufacturing Page 0. */
    MPT_CONFIG_PAGE_HEADER_INIT_MANUFACTURING(&pPages->ManufacturingPage0,
                                              MptConfigurationPageManufacturing0, 0,
                                              MPT_CONFIGURATION_PAGE_ATTRIBUTE_PERSISTENT_READONLY);
    strncpy((char *)pPages->ManufacturingPage0.u.fields.abChipName,          "VBox MPT Fusion", 16);
    strncpy((char *)pPages->ManufacturingPage0.u.fields.abChipRevision,      "1.0", 8);
    strncpy((char *)pPages->ManufacturingPage0.u.fields.abBoardName,         "VBox MPT Fusion", 16);
    strncpy((char *)pPages->ManufacturingPage0.u.fields.abBoardAssembly,     "SUN", 8);
    strncpy((char *)pPages->ManufacturingPage0.u.fields.abBoardTracerNumber, "CAFECAFECAFECAFE", 16);

    /* Manufacturing Page 1 - I don't know what this contains so we leave it 0 for now. */
    MPT_CONFIG_PAGE_HEADER_INIT_MANUFACTURING(&pPages->ManufacturingPage1,
                                              MptConfigurationPageManufacturing1, 1,
                                              MPT_CONFIGURATION_PAGE_ATTRIBUTE_PERSISTENT_READONLY);

    /* Manufacturing Page 2. */
    MPT_CONFIG_PAGE_HEADER_INIT_MANUFACTURING(&pPages->ManufacturingPage2,
                                              MptConfigurationPageManufacturing2, 2,
                                              MPT_CONFIGURATION_PAGE_ATTRIBUTE_PERSISTENT_READONLY);

    if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI)
    {
        pPages->ManufacturingPage2.u.fields.u16PCIDeviceID = LSILOGICSCSI_PCI_SPI_DEVICE_ID;
        pPages->ManufacturingPage2.u.fields.u8PCIRevisionID = LSILOGICSCSI_PCI_SPI_REVISION_ID;
    }
    else if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
    {
        pPages->ManufacturingPage2.u.fields.u16PCIDeviceID = LSILOGICSCSI_PCI_SAS_DEVICE_ID;
        pPages->ManufacturingPage2.u.fields.u8PCIRevisionID = LSILOGICSCSI_PCI_SAS_REVISION_ID;
    }

    /* Manufacturing Page 3. */
    MPT_CONFIG_PAGE_HEADER_INIT_MANUFACTURING(&pPages->ManufacturingPage3,
                                              MptConfigurationPageManufacturing3, 3,
                                              MPT_CONFIGURATION_PAGE_ATTRIBUTE_PERSISTENT_READONLY);

    if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI)
    {
        pPages->ManufacturingPage3.u.fields.u16PCIDeviceID = LSILOGICSCSI_PCI_SPI_DEVICE_ID;
        pPages->ManufacturingPage3.u.fields.u8PCIRevisionID = LSILOGICSCSI_PCI_SPI_REVISION_ID;
    }
    else if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
    {
        pPages->ManufacturingPage3.u.fields.u16PCIDeviceID = LSILOGICSCSI_PCI_SAS_DEVICE_ID;
        pPages->ManufacturingPage3.u.fields.u8PCIRevisionID = LSILOGICSCSI_PCI_SAS_REVISION_ID;
    }

    /* Manufacturing Page 4 - I don't know what this contains so we leave it 0 for now. */
    MPT_CONFIG_PAGE_HEADER_INIT_MANUFACTURING(&pPages->ManufacturingPage4,
                                              MptConfigurationPageManufacturing4, 4,
                                              MPT_CONFIGURATION_PAGE_ATTRIBUTE_PERSISTENT_READONLY);

    /* Manufacturing Page 5 - WWID settings. */
    MPT_CONFIG_PAGE_HEADER_INIT_MANUFACTURING(&pPages->ManufacturingPage5,
                                              MptConfigurationPageManufacturing5, 5,
                                              MPT_CONFIGURATION_PAGE_ATTRIBUTE_PERSISTENT_READONLY);

    /* Manufacturing Page 6 - Product specific settings. */
    MPT_CONFIG_PAGE_HEADER_INIT_MANUFACTURING(&pPages->ManufacturingPage6,
                                              MptConfigurationPageManufacturing6, 6,
                                              MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE);

    /* Manufacturing Page 8 -  Product specific settings. */
    MPT_CONFIG_PAGE_HEADER_INIT_MANUFACTURING(&pPages->ManufacturingPage8,
                                              MptConfigurationPageManufacturing8, 8,
                                              MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE);

    /* Manufacturing Page 9 -  Product specific settings. */
    MPT_CONFIG_PAGE_HEADER_INIT_MANUFACTURING(&pPages->ManufacturingPage9,
                                              MptConfigurationPageManufacturing9, 9,
                                              MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE);

    /* Manufacturing Page 10 -  Product specific settings. */
    MPT_CONFIG_PAGE_HEADER_INIT_MANUFACTURING(&pPages->ManufacturingPage10,
                                              MptConfigurationPageManufacturing10, 10,
                                              MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE);

    /* I/O Unit page 0. */
    MPT_CONFIG_PAGE_HEADER_INIT_IO_UNIT(&pPages->IOUnitPage0,
                                        MptConfigurationPageIOUnit0, 0,
                                        MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY);
    pPages->IOUnitPage0.u.fields.u64UniqueIdentifier = 0xcafe;

    /* I/O Unit page 1. */
    MPT_CONFIG_PAGE_HEADER_INIT_IO_UNIT(&pPages->IOUnitPage1,
                                        MptConfigurationPageIOUnit1, 1,
                                        MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY);
    pPages->IOUnitPage1.u.fields.fSingleFunction         = true;
    pPages->IOUnitPage1.u.fields.fAllPathsMapped         = false;
    pPages->IOUnitPage1.u.fields.fIntegratedRAIDDisabled = true;
    pPages->IOUnitPage1.u.fields.f32BitAccessForced      = false;

    /* I/O Unit page 2. */
    MPT_CONFIG_PAGE_HEADER_INIT_IO_UNIT(&pPages->IOUnitPage2,
                                        MptConfigurationPageIOUnit2, 2,
                                        MPT_CONFIGURATION_PAGE_ATTRIBUTE_PERSISTENT);
    pPages->IOUnitPage2.u.fields.fPauseOnError       = false;
    pPages->IOUnitPage2.u.fields.fVerboseModeEnabled = false;
    pPages->IOUnitPage2.u.fields.fDisableColorVideo  = false;
    pPages->IOUnitPage2.u.fields.fNotHookInt40h      = false;
    pPages->IOUnitPage2.u.fields.u32BIOSVersion      = 0xcafecafe;
    pPages->IOUnitPage2.u.fields.aAdapterOrder[0].fAdapterEnabled = true;
    pPages->IOUnitPage2.u.fields.aAdapterOrder[0].fAdapterEmbedded = true;
    pPages->IOUnitPage2.u.fields.aAdapterOrder[0].u8PCIBusNumber = 0;
    pPages->IOUnitPage2.u.fields.aAdapterOrder[0].u8PCIDevFn     = pThis->PciDev.uDevFn;

    /* I/O Unit page 3. */
    MPT_CONFIG_PAGE_HEADER_INIT_IO_UNIT(&pPages->IOUnitPage3,
                                        MptConfigurationPageIOUnit3, 3,
                                        MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE);
    pPages->IOUnitPage3.u.fields.u8GPIOCount = 0;

    /* I/O Unit page 4. */
    MPT_CONFIG_PAGE_HEADER_INIT_IO_UNIT(&pPages->IOUnitPage4,
                                        MptConfigurationPageIOUnit4, 4,
                                        MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE);

    /* IOC page 0. */
    MPT_CONFIG_PAGE_HEADER_INIT_IOC(&pPages->IOCPage0,
                                    MptConfigurationPageIOC0, 0,
                                    MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY);
    pPages->IOCPage0.u.fields.u32TotalNVStore      = 0;
    pPages->IOCPage0.u.fields.u32FreeNVStore       = 0;

    if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI)
    {
        pPages->IOCPage0.u.fields.u16VendorId          = LSILOGICSCSI_PCI_VENDOR_ID;
        pPages->IOCPage0.u.fields.u16DeviceId          = LSILOGICSCSI_PCI_SPI_DEVICE_ID;
        pPages->IOCPage0.u.fields.u8RevisionId         = LSILOGICSCSI_PCI_SPI_REVISION_ID;
        pPages->IOCPage0.u.fields.u32ClassCode         = LSILOGICSCSI_PCI_SPI_CLASS_CODE;
        pPages->IOCPage0.u.fields.u16SubsystemVendorId = LSILOGICSCSI_PCI_SPI_SUBSYSTEM_VENDOR_ID;
        pPages->IOCPage0.u.fields.u16SubsystemId       = LSILOGICSCSI_PCI_SPI_SUBSYSTEM_ID;
    }
    else if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
    {
        pPages->IOCPage0.u.fields.u16VendorId          = LSILOGICSCSI_PCI_VENDOR_ID;
        pPages->IOCPage0.u.fields.u16DeviceId          = LSILOGICSCSI_PCI_SAS_DEVICE_ID;
        pPages->IOCPage0.u.fields.u8RevisionId         = LSILOGICSCSI_PCI_SAS_REVISION_ID;
        pPages->IOCPage0.u.fields.u32ClassCode         = LSILOGICSCSI_PCI_SAS_CLASS_CODE;
        pPages->IOCPage0.u.fields.u16SubsystemVendorId = LSILOGICSCSI_PCI_SAS_SUBSYSTEM_VENDOR_ID;
        pPages->IOCPage0.u.fields.u16SubsystemId       = LSILOGICSCSI_PCI_SAS_SUBSYSTEM_ID;
    }

    /* IOC page 1. */
    MPT_CONFIG_PAGE_HEADER_INIT_IOC(&pPages->IOCPage1,
                                    MptConfigurationPageIOC1, 1,
                                    MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE);
    pPages->IOCPage1.u.fields.fReplyCoalescingEnabled = false;
    pPages->IOCPage1.u.fields.u32CoalescingTimeout    = 0;
    pPages->IOCPage1.u.fields.u8CoalescingDepth       = 0;

    /* IOC page 2. */
    MPT_CONFIG_PAGE_HEADER_INIT_IOC(&pPages->IOCPage2,
                                    MptConfigurationPageIOC2, 2,
                                    MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY);
    /* Everything else here is 0. */

    /* IOC page 3. */
    MPT_CONFIG_PAGE_HEADER_INIT_IOC(&pPages->IOCPage3,
                                    MptConfigurationPageIOC3, 3,
                                    MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY);
    /* Everything else here is 0. */

    /* IOC page 4. */
    MPT_CONFIG_PAGE_HEADER_INIT_IOC(&pPages->IOCPage4,
                                    MptConfigurationPageIOC4, 4,
                                    MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY);
    /* Everything else here is 0. */

    /* IOC page 6. */
    MPT_CONFIG_PAGE_HEADER_INIT_IOC(&pPages->IOCPage6,
                                    MptConfigurationPageIOC6, 6,
                                    MPT_CONFIGURATION_PAGE_ATTRIBUTE_READONLY);
    /* Everything else here is 0. */

    /* BIOS page 1. */
    MPT_CONFIG_PAGE_HEADER_INIT_BIOS(&pPages->BIOSPage1,
                                     MptConfigurationPageBIOS1, 1,
                                     MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE);

    /* BIOS page 2. */
    MPT_CONFIG_PAGE_HEADER_INIT_BIOS(&pPages->BIOSPage2,
                                     MptConfigurationPageBIOS2, 2,
                                     MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE);

    /* BIOS page 4. */
    MPT_CONFIG_PAGE_HEADER_INIT_BIOS(&pPages->BIOSPage4,
                                     MptConfigurationPageBIOS4, 4,
                                     MPT_CONFIGURATION_PAGE_ATTRIBUTE_CHANGEABLE);

    if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI)
        lsilogicR3InitializeConfigurationPagesSpi(pThis);
    else if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
        lsilogicR3InitializeConfigurationPagesSas(pThis);
    else
        AssertMsgFailed(("Invalid controller type %d\n", pThis->enmCtrlType));
}

/**
 * @callback_method_impl{FNPDMQUEUEDEV, Transmit queue consumer.}
 */
static DECLCALLBACK(bool) lsilogicR3NotifyQueueConsumer(PPDMDEVINS pDevIns, PPDMQUEUEITEMCORE pItem)
{
    RT_NOREF(pItem);
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    int rc = VINF_SUCCESS;

    LogFlowFunc(("pDevIns=%#p pItem=%#p\n", pDevIns, pItem));

    rc = SUPSemEventSignal(pThis->pSupDrvSession, pThis->hEvtProcess);
    AssertRC(rc);

    return true;
}

/**
 * Sets the emulated controller type from a given string.
 *
 * @returns VBox status code.
 *
 * @param   pThis           Pointer to the LsiLogic device state.
 * @param   pcszCtrlType    The string to use.
 */
static int lsilogicR3GetCtrlTypeFromString(PLSILOGICSCSI pThis, const char *pcszCtrlType)
{
    int rc = VERR_INVALID_PARAMETER;

    if (!RTStrCmp(pcszCtrlType, LSILOGICSCSI_PCI_SPI_CTRLNAME))
    {
        pThis->enmCtrlType = LSILOGICCTRLTYPE_SCSI_SPI;
        rc = VINF_SUCCESS;
    }
    else if (!RTStrCmp(pcszCtrlType, LSILOGICSCSI_PCI_SAS_CTRLNAME))
    {
        pThis->enmCtrlType = LSILOGICCTRLTYPE_SCSI_SAS;
        rc = VINF_SUCCESS;
    }

    return rc;
}

/**
 * @callback_method_impl{FNIOMIOPORTIN, Legacy ISA port.}
 */
static DECLCALLBACK(int) lsilogicR3IsaIOPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    RT_NOREF(pvUser, cb);
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);

    Assert(cb == 1);

    uint8_t iRegister = pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI
                      ? Port - LSILOGIC_BIOS_IO_PORT
                      : Port - LSILOGIC_SAS_BIOS_IO_PORT;
    int rc = vboxscsiReadRegister(&pThis->VBoxSCSI, iRegister, pu32);

    Log2(("%s: pu32=%p:{%.*Rhxs} iRegister=%d rc=%Rrc\n",
          __FUNCTION__, pu32, 1, pu32, iRegister, rc));

    return rc;
}

/**
 * Prepares a request from the BIOS.
 *
 * @returns VBox status code.
 * @param   pThis       Pointer to the LsiLogic device state.
 */
static int lsilogicR3PrepareBiosScsiRequest(PLSILOGICSCSI pThis)
{
    int rc;
    uint32_t uTargetDevice;
    uint32_t uLun;
    uint8_t *pbCdb;
    size_t cbCdb;
    size_t cbBuf;

    rc = vboxscsiSetupRequest(&pThis->VBoxSCSI, &uLun, &pbCdb, &cbCdb, &cbBuf, &uTargetDevice);
    AssertMsgRCReturn(rc, ("Setting up SCSI request failed rc=%Rrc\n", rc), rc);

    if (   uTargetDevice < pThis->cDeviceStates
        && pThis->paDeviceStates[uTargetDevice].pDrvBase)
    {
        PLSILOGICDEVICE pTgtDev = &pThis->paDeviceStates[uTargetDevice];
        PDMMEDIAEXIOREQ hIoReq;
        PLSILOGICREQ pReq;

        rc = pTgtDev->pDrvMediaEx->pfnIoReqAlloc(pTgtDev->pDrvMediaEx, &hIoReq, (void **)&pReq,
                                                 0, PDMIMEDIAEX_F_SUSPEND_ON_RECOVERABLE_ERR);
        AssertMsgRCReturn(rc, ("Getting task from cache failed rc=%Rrc\n", rc), rc);

        pReq->fBIOS = true;
        pReq->hIoReq = hIoReq;
        pReq->pTargetDevice = pTgtDev;

        ASMAtomicIncU32(&pTgtDev->cOutstandingRequests);

        rc = pTgtDev->pDrvMediaEx->pfnIoReqSendScsiCmd(pTgtDev->pDrvMediaEx, pReq->hIoReq, uLun,
                                                       pbCdb, cbCdb, PDMMEDIAEXIOREQSCSITXDIR_UNKNOWN,
                                                       cbBuf, NULL, 0, &pReq->u8ScsiSts, 30 * RT_MS_1SEC);
        if (rc == VINF_SUCCESS || rc != VINF_PDM_MEDIAEX_IOREQ_IN_PROGRESS)
        {
            uint8_t u8ScsiSts = pReq->u8ScsiSts;
            pTgtDev->pDrvMediaEx->pfnIoReqFree(pTgtDev->pDrvMediaEx, pReq->hIoReq);
            rc = vboxscsiRequestFinished(&pThis->VBoxSCSI, u8ScsiSts);
        }
        else if (rc == VINF_PDM_MEDIAEX_IOREQ_IN_PROGRESS)
            rc = VINF_SUCCESS;

        return rc;
    }

    /* Device is not present. */
    AssertMsg(pbCdb[0] == SCSI_INQUIRY,
              ("Device is not present but command is not inquiry\n"));

    SCSIINQUIRYDATA ScsiInquiryData;

    memset(&ScsiInquiryData, 0, sizeof(SCSIINQUIRYDATA));
    ScsiInquiryData.u5PeripheralDeviceType = SCSI_INQUIRY_DATA_PERIPHERAL_DEVICE_TYPE_UNKNOWN;
    ScsiInquiryData.u3PeripheralQualifier = SCSI_INQUIRY_DATA_PERIPHERAL_QUALIFIER_NOT_CONNECTED_NOT_SUPPORTED;

    memcpy(pThis->VBoxSCSI.pbBuf, &ScsiInquiryData, 5);

    rc = vboxscsiRequestFinished(&pThis->VBoxSCSI, SCSI_STATUS_OK);
    AssertMsgRCReturn(rc, ("Finishing BIOS SCSI request failed rc=%Rrc\n", rc), rc);

    return rc;
}

/**
 * @callback_method_impl{FNIOMIOPORTOUT, Legacy ISA port.}
 */
static DECLCALLBACK(int) lsilogicR3IsaIOPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    RT_NOREF(pvUser, cb);
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    Log2(("#%d %s: pvUser=%#p cb=%d u32=%#x Port=%#x\n", pDevIns->iInstance, __FUNCTION__, pvUser, cb, u32, Port));

    Assert(cb == 1);

    /*
     * If there is already a request form the BIOS pending ignore this write
     * because it should not happen.
     */
    if (ASMAtomicReadBool(&pThis->fBiosReqPending))
        return VINF_SUCCESS;

    uint8_t iRegister = pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI
                      ? Port - LSILOGIC_BIOS_IO_PORT
                      : Port - LSILOGIC_SAS_BIOS_IO_PORT;
    int rc = vboxscsiWriteRegister(&pThis->VBoxSCSI, iRegister, (uint8_t)u32);
    if (rc == VERR_MORE_DATA)
    {
        ASMAtomicXchgBool(&pThis->fBiosReqPending, true);
        /* Send a notifier to the PDM queue that there are pending requests. */
        PPDMQUEUEITEMCORE pItem = PDMQueueAlloc(pThis->CTX_SUFF(pNotificationQueue));
        AssertMsg(pItem, ("Allocating item for queue failed\n"));
        PDMQueueInsert(pThis->CTX_SUFF(pNotificationQueue), (PPDMQUEUEITEMCORE)pItem);
    }
    else if (RT_FAILURE(rc))
        AssertMsgFailed(("Writing BIOS register failed %Rrc\n", rc));

    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNIOMIOPORTOUTSTRING,
 * Port I/O Handler for primary port range OUT string operations.}
 */
static DECLCALLBACK(int) lsilogicR3IsaIOPortWriteStr(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port,
                                                     uint8_t const *pbSrc, uint32_t *pcTransfers, unsigned cb)
{
    RT_NOREF(pvUser);
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    Log2(("#%d %s: pvUser=%#p cb=%d Port=%#x\n", pDevIns->iInstance, __FUNCTION__, pvUser, cb, Port));

    uint8_t iRegister = pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI
                      ? Port - LSILOGIC_BIOS_IO_PORT
                      : Port - LSILOGIC_SAS_BIOS_IO_PORT;
    int rc = vboxscsiWriteString(pDevIns, &pThis->VBoxSCSI, iRegister, pbSrc, pcTransfers, cb);
    if (rc == VERR_MORE_DATA)
    {
        ASMAtomicXchgBool(&pThis->fBiosReqPending, true);
        /* Send a notifier to the PDM queue that there are pending requests. */
        PPDMQUEUEITEMCORE pItem = PDMQueueAlloc(pThis->CTX_SUFF(pNotificationQueue));
        AssertMsg(pItem, ("Allocating item for queue failed\n"));
        PDMQueueInsert(pThis->CTX_SUFF(pNotificationQueue), (PPDMQUEUEITEMCORE)pItem);
    }
    else if (RT_FAILURE(rc))
        AssertMsgFailed(("Writing BIOS register failed %Rrc\n", rc));

    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNIOMIOPORTINSTRING,
 * Port I/O Handler for primary port range IN string operations.}
 */
static DECLCALLBACK(int) lsilogicR3IsaIOPortReadStr(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port,
                                                    uint8_t *pbDst, uint32_t *pcTransfers, unsigned cb)
{
    RT_NOREF(pvUser);
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    LogFlowFunc(("#%d %s: pvUser=%#p cb=%d Port=%#x\n", pDevIns->iInstance, __FUNCTION__, pvUser, cb, Port));

    uint8_t iRegister = pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI
                      ? Port - LSILOGIC_BIOS_IO_PORT
                      : Port - LSILOGIC_SAS_BIOS_IO_PORT;
    return vboxscsiReadString(pDevIns, &pThis->VBoxSCSI, iRegister, pbDst, pcTransfers, cb);
}

/**
 * @callback_method_impl{FNPCIIOREGIONMAP}
 */
static DECLCALLBACK(int) lsilogicR3Map(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                       RTGCPHYS GCPhysAddress, RTGCPHYS cb,
                                       PCIADDRESSSPACE enmType)
{
    RT_NOREF(pPciDev);
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    int         rc = VINF_SUCCESS;
    const char *pcszCtrl = pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI
                           ? "LsiLogic"
                           : "LsiLogicSas";
    const char *pcszDiag = pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI
                           ? "LsiLogicDiag"
                           : "LsiLogicSasDiag";

    Log2(("%s: registering area at GCPhysAddr=%RGp cb=%RGp\n", __FUNCTION__, GCPhysAddress, cb));

    AssertMsg(   (enmType == PCI_ADDRESS_SPACE_MEM && cb >= LSILOGIC_PCI_SPACE_MEM_SIZE)
              || (enmType == PCI_ADDRESS_SPACE_IO  && cb >= LSILOGIC_PCI_SPACE_IO_SIZE),
              ("PCI region type and size do not match\n"));

    if (enmType == PCI_ADDRESS_SPACE_MEM && iRegion == 1)
    {
        /*
         * Non-4-byte read access to LSILOGIC_REG_REPLY_QUEUE may cause real strange behavior
         * because the data is part of a physical guest address.  But some drivers use 1-byte
         * access to scan for SCSI controllers.  So, we simplify our code by telling IOM to
         * read DWORDs.
         *
         * Regarding writes, we couldn't find anything specific in the specs about what should
         * happen. So far we've ignored unaligned writes and assumed the missing bytes of
         * byte and word access to be zero. We suspect that IOMMMIO_FLAGS_WRITE_ONLY_DWORD
         * or IOMMMIO_FLAGS_WRITE_DWORD_ZEROED would be the most appropriate here, but since we
         * don't have real hw to test one, the old behavior is kept exactly like it used to be.
         */
        /** @todo Check out unaligned writes and non-dword writes on real LsiLogic
         *        hardware. */
        rc = PDMDevHlpMMIORegister(pDevIns, GCPhysAddress, cb, NULL /*pvUser*/,
                                   IOMMMIO_FLAGS_READ_DWORD | IOMMMIO_FLAGS_WRITE_PASSTHRU,
                                   lsilogicMMIOWrite, lsilogicMMIORead, pcszCtrl);
        if (RT_FAILURE(rc))
            return rc;

        if (pThis->fR0Enabled)
        {
            rc = PDMDevHlpMMIORegisterR0(pDevIns, GCPhysAddress, cb, NIL_RTR0PTR /*pvUser*/,
                                         "lsilogicMMIOWrite", "lsilogicMMIORead");
            if (RT_FAILURE(rc))
                return rc;
        }

        if (pThis->fGCEnabled)
        {
            rc = PDMDevHlpMMIORegisterRC(pDevIns, GCPhysAddress, cb, NIL_RTRCPTR /*pvUser*/,
                                         "lsilogicMMIOWrite", "lsilogicMMIORead");
            if (RT_FAILURE(rc))
                return rc;
        }

        pThis->GCPhysMMIOBase = GCPhysAddress;
    }
    else if (enmType == PCI_ADDRESS_SPACE_MEM && iRegion == 2)
    {
        /* We use the assigned size here, because we currently only support page aligned MMIO ranges. */
        rc = PDMDevHlpMMIORegister(pDevIns, GCPhysAddress, cb, NULL /*pvUser*/,
                                   IOMMMIO_FLAGS_READ_PASSTHRU | IOMMMIO_FLAGS_WRITE_PASSTHRU,
                                   lsilogicDiagnosticWrite, lsilogicDiagnosticRead, pcszDiag);
        if (RT_FAILURE(rc))
            return rc;

        if (pThis->fR0Enabled)
        {
            rc = PDMDevHlpMMIORegisterR0(pDevIns, GCPhysAddress, cb, NIL_RTR0PTR /*pvUser*/,
                                         "lsilogicDiagnosticWrite", "lsilogicDiagnosticRead");
            if (RT_FAILURE(rc))
                return rc;
        }

        if (pThis->fGCEnabled)
        {
            rc = PDMDevHlpMMIORegisterRC(pDevIns, GCPhysAddress, cb, NIL_RTRCPTR /*pvUser*/,
                                         "lsilogicDiagnosticWrite", "lsilogicDiagnosticRead");
            if (RT_FAILURE(rc))
                return rc;
        }
    }
    else if (enmType == PCI_ADDRESS_SPACE_IO)
    {
        rc = PDMDevHlpIOPortRegister(pDevIns, (RTIOPORT)GCPhysAddress, LSILOGIC_PCI_SPACE_IO_SIZE,
                                     NULL, lsilogicIOPortWrite, lsilogicIOPortRead, NULL, NULL, pcszCtrl);
        if (RT_FAILURE(rc))
            return rc;

        if (pThis->fR0Enabled)
        {
            rc = PDMDevHlpIOPortRegisterR0(pDevIns, (RTIOPORT)GCPhysAddress, LSILOGIC_PCI_SPACE_IO_SIZE,
                                           0, "lsilogicIOPortWrite", "lsilogicIOPortRead", NULL, NULL, pcszCtrl);
            if (RT_FAILURE(rc))
                return rc;
        }

        if (pThis->fGCEnabled)
        {
            rc = PDMDevHlpIOPortRegisterRC(pDevIns, (RTIOPORT)GCPhysAddress, LSILOGIC_PCI_SPACE_IO_SIZE,
                                           0, "lsilogicIOPortWrite", "lsilogicIOPortRead", NULL, NULL, pcszCtrl);
            if (RT_FAILURE(rc))
                return rc;
        }

        pThis->IOPortBase = (RTIOPORT)GCPhysAddress;
    }
    else
        AssertMsgFailed(("Invalid enmType=%d iRegion=%d\n", enmType, iRegion));

    return rc;
}

/**
 * @callback_method_impl{PFNDBGFHANDLERDEV}
 */
static DECLCALLBACK(void) lsilogicR3Info(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    bool          fVerbose = false;

    /*
     * Parse args.
     */
    if (pszArgs)
        fVerbose = strstr(pszArgs, "verbose") != NULL;

    /*
     * Show info.
     */
    pHlp->pfnPrintf(pHlp,
                    "%s#%d: port=%RTiop mmio=%RGp max-devices=%u GC=%RTbool R0=%RTbool\n",
                    pDevIns->pReg->szName,
                    pDevIns->iInstance,
                    pThis->IOPortBase, pThis->GCPhysMMIOBase,
                    pThis->cDeviceStates,
                    pThis->fGCEnabled ? true : false,
                    pThis->fR0Enabled ? true : false);

    /*
     * Show general state.
     */
    pHlp->pfnPrintf(pHlp, "enmState=%u\n", pThis->enmState);
    pHlp->pfnPrintf(pHlp, "enmWhoInit=%u\n", pThis->enmWhoInit);
    pHlp->pfnPrintf(pHlp, "enmDoorbellState=%d\n", pThis->enmDoorbellState);
    pHlp->pfnPrintf(pHlp, "fDiagnosticEnabled=%RTbool\n", pThis->fDiagnosticEnabled);
    pHlp->pfnPrintf(pHlp, "fNotificationSent=%RTbool\n", pThis->fNotificationSent);
    pHlp->pfnPrintf(pHlp, "fEventNotificationEnabled=%RTbool\n", pThis->fEventNotificationEnabled);
    pHlp->pfnPrintf(pHlp, "uInterruptMask=%#x\n", pThis->uInterruptMask);
    pHlp->pfnPrintf(pHlp, "uInterruptStatus=%#x\n", pThis->uInterruptStatus);
    pHlp->pfnPrintf(pHlp, "u16IOCFaultCode=%#06x\n", pThis->u16IOCFaultCode);
    pHlp->pfnPrintf(pHlp, "u32HostMFAHighAddr=%#x\n", pThis->u32HostMFAHighAddr);
    pHlp->pfnPrintf(pHlp, "u32SenseBufferHighAddr=%#x\n", pThis->u32SenseBufferHighAddr);
    pHlp->pfnPrintf(pHlp, "cMaxDevices=%u\n", pThis->cMaxDevices);
    pHlp->pfnPrintf(pHlp, "cMaxBuses=%u\n", pThis->cMaxBuses);
    pHlp->pfnPrintf(pHlp, "cbReplyFrame=%u\n", pThis->cbReplyFrame);
    pHlp->pfnPrintf(pHlp, "cReplyQueueEntries=%u\n", pThis->cReplyQueueEntries);
    pHlp->pfnPrintf(pHlp, "cRequestQueueEntries=%u\n", pThis->cRequestQueueEntries);
    pHlp->pfnPrintf(pHlp, "cPorts=%u\n", pThis->cPorts);

    /*
     * Show queue status.
     */
    pHlp->pfnPrintf(pHlp, "uReplyFreeQueueNextEntryFreeWrite=%u\n", pThis->uReplyFreeQueueNextEntryFreeWrite);
    pHlp->pfnPrintf(pHlp, "uReplyFreeQueueNextAddressRead=%u\n", pThis->uReplyFreeQueueNextAddressRead);
    pHlp->pfnPrintf(pHlp, "uReplyPostQueueNextEntryFreeWrite=%u\n", pThis->uReplyPostQueueNextEntryFreeWrite);
    pHlp->pfnPrintf(pHlp, "uReplyPostQueueNextAddressRead=%u\n", pThis->uReplyPostQueueNextAddressRead);
    pHlp->pfnPrintf(pHlp, "uRequestQueueNextEntryFreeWrite=%u\n", pThis->uRequestQueueNextEntryFreeWrite);
    pHlp->pfnPrintf(pHlp, "uRequestQueueNextAddressRead=%u\n", pThis->uRequestQueueNextAddressRead);

    /*
     * Show queue content if verbose
     */
    if (fVerbose)
    {
        for (unsigned i = 0; i < pThis->cReplyQueueEntries; i++)
            pHlp->pfnPrintf(pHlp, "RFQ[%u]=%#x\n", i, pThis->pReplyFreeQueueBaseR3[i]);

        pHlp->pfnPrintf(pHlp, "\n");

        for (unsigned i = 0; i < pThis->cReplyQueueEntries; i++)
            pHlp->pfnPrintf(pHlp, "RPQ[%u]=%#x\n", i, pThis->pReplyPostQueueBaseR3[i]);

        pHlp->pfnPrintf(pHlp, "\n");

        for (unsigned i = 0; i < pThis->cRequestQueueEntries; i++)
            pHlp->pfnPrintf(pHlp, "ReqQ[%u]=%#x\n", i, pThis->pRequestQueueBaseR3[i]);
    }

    /*
     * Print the device status.
     */
    for (unsigned i = 0; i < pThis->cDeviceStates; i++)
    {
        PLSILOGICDEVICE pDevice = &pThis->paDeviceStates[i];

        pHlp->pfnPrintf(pHlp, "\n");

        pHlp->pfnPrintf(pHlp, "Device[%u]: device-attached=%RTbool cOutstandingRequests=%u\n",
                        i, pDevice->pDrvBase != NULL, pDevice->cOutstandingRequests);
    }
}

/**
 * Allocate the queues.
 *
 * @returns VBox status code.
 *
 * @param   pThis       Pointer to the LsiLogic device state.
 */
static int lsilogicR3QueuesAlloc(PLSILOGICSCSI pThis)
{
    PVM pVM = PDMDevHlpGetVM(pThis->pDevInsR3);
    uint32_t cbQueues;

    Assert(!pThis->pReplyFreeQueueBaseR3);

    cbQueues  = 2*pThis->cReplyQueueEntries * sizeof(uint32_t);
    cbQueues += pThis->cRequestQueueEntries * sizeof(uint32_t);
    int rc = MMHyperAlloc(pVM, cbQueues, 1, MM_TAG_PDM_DEVICE_USER,
                          (void **)&pThis->pReplyFreeQueueBaseR3);
    if (RT_FAILURE(rc))
        return VERR_NO_MEMORY;
    pThis->pReplyFreeQueueBaseR0 = MMHyperR3ToR0(pVM, (void *)pThis->pReplyFreeQueueBaseR3);
    pThis->pReplyFreeQueueBaseRC = MMHyperR3ToRC(pVM, (void *)pThis->pReplyFreeQueueBaseR3);

    pThis->pReplyPostQueueBaseR3 = pThis->pReplyFreeQueueBaseR3 + pThis->cReplyQueueEntries;
    pThis->pReplyPostQueueBaseR0 = MMHyperR3ToR0(pVM, (void *)pThis->pReplyPostQueueBaseR3);
    pThis->pReplyPostQueueBaseRC = MMHyperR3ToRC(pVM, (void *)pThis->pReplyPostQueueBaseR3);

    pThis->pRequestQueueBaseR3   = pThis->pReplyPostQueueBaseR3 + pThis->cReplyQueueEntries;
    pThis->pRequestQueueBaseR0   = MMHyperR3ToR0(pVM, (void *)pThis->pRequestQueueBaseR3);
    pThis->pRequestQueueBaseRC   = MMHyperR3ToRC(pVM, (void *)pThis->pRequestQueueBaseR3);

    return VINF_SUCCESS;
}

/**
 * Free the hyper memory used or the queues.
 *
 * @returns nothing.
 *
 * @param   pThis       Pointer to the LsiLogic device state.
 */
static void lsilogicR3QueuesFree(PLSILOGICSCSI pThis)
{
    PVM pVM = PDMDevHlpGetVM(pThis->pDevInsR3);
    int rc = VINF_SUCCESS;

    AssertPtr(pThis->pReplyFreeQueueBaseR3);

    rc = MMHyperFree(pVM, (void *)pThis->pReplyFreeQueueBaseR3);
    AssertRC(rc);

    pThis->pReplyFreeQueueBaseR3 = NULL;
    pThis->pReplyPostQueueBaseR3 = NULL;
    pThis->pRequestQueueBaseR3   = NULL;
}


/* The worker thread. */
static DECLCALLBACK(int) lsilogicR3Worker(PPDMDEVINS pDevIns, PPDMTHREAD pThread)
{
    PLSILOGICSCSI pThis = (PLSILOGICSCSI)pThread->pvUser;
    int rc = VINF_SUCCESS;

    if (pThread->enmState == PDMTHREADSTATE_INITIALIZING)
        return VINF_SUCCESS;

    while (pThread->enmState == PDMTHREADSTATE_RUNNING)
    {
        ASMAtomicWriteBool(&pThis->fWrkThreadSleeping, true);
        bool fNotificationSent = ASMAtomicXchgBool(&pThis->fNotificationSent, false);
        if (!fNotificationSent)
        {
            Assert(ASMAtomicReadBool(&pThis->fWrkThreadSleeping));
            rc = SUPSemEventWaitNoResume(pThis->pSupDrvSession, pThis->hEvtProcess, RT_INDEFINITE_WAIT);
            AssertLogRelMsgReturn(RT_SUCCESS(rc) || rc == VERR_INTERRUPTED, ("%Rrc\n", rc), rc);
            if (RT_UNLIKELY(pThread->enmState != PDMTHREADSTATE_RUNNING))
                break;
            LogFlowFunc(("Woken up with rc=%Rrc\n", rc));
            ASMAtomicWriteBool(&pThis->fNotificationSent, false);
        }

        ASMAtomicWriteBool(&pThis->fWrkThreadSleeping, false);

        /* Check whether there is a BIOS request pending and process it first. */
        if (ASMAtomicReadBool(&pThis->fBiosReqPending))
        {
            rc = lsilogicR3PrepareBiosScsiRequest(pThis);
            AssertRC(rc);
            ASMAtomicXchgBool(&pThis->fBiosReqPending, false);
        }

        /* Only process request which arrived before we received the notification. */
        uint32_t uRequestQueueNextEntryWrite = ASMAtomicReadU32(&pThis->uRequestQueueNextEntryFreeWrite);

        /* Go through the messages now and process them. */
        while (   RT_LIKELY(pThis->enmState == LSILOGICSTATE_OPERATIONAL)
               && (pThis->uRequestQueueNextAddressRead != uRequestQueueNextEntryWrite))
        {
            MptRequestUnion GuestRequest;
            uint32_t  u32RequestMessageFrameDesc = pThis->CTX_SUFF(pRequestQueueBase)[pThis->uRequestQueueNextAddressRead];
            RTGCPHYS  GCPhysMessageFrameAddr = LSILOGIC_RTGCPHYS_FROM_U32(pThis->u32HostMFAHighAddr,
                                                                          (u32RequestMessageFrameDesc & ~0x07));

            /* Read the message header from the guest first. */
            PDMDevHlpPhysRead(pDevIns, GCPhysMessageFrameAddr, &GuestRequest, sizeof(MptMessageHdr));

            /* Determine the size of the request. */
            uint32_t cbRequest = 0;
            switch (GuestRequest.Header.u8Function)
            {
                case MPT_MESSAGE_HDR_FUNCTION_SCSI_IO_REQUEST:
                    cbRequest = sizeof(MptSCSIIORequest);
                    break;
                case MPT_MESSAGE_HDR_FUNCTION_SCSI_TASK_MGMT:
                    cbRequest = sizeof(MptSCSITaskManagementRequest);
                    break;
                case MPT_MESSAGE_HDR_FUNCTION_IOC_INIT:
                    cbRequest = sizeof(MptIOCInitRequest);
                    break;
                case MPT_MESSAGE_HDR_FUNCTION_IOC_FACTS:
                    cbRequest = sizeof(MptIOCFactsRequest);
                    break;
                case MPT_MESSAGE_HDR_FUNCTION_CONFIG:
                    cbRequest = sizeof(MptConfigurationRequest);
                    break;
                case MPT_MESSAGE_HDR_FUNCTION_PORT_FACTS:
                    cbRequest = sizeof(MptPortFactsRequest);
                    break;
                case MPT_MESSAGE_HDR_FUNCTION_PORT_ENABLE:
                    cbRequest = sizeof(MptPortEnableRequest);
                    break;
                case MPT_MESSAGE_HDR_FUNCTION_EVENT_NOTIFICATION:
                    cbRequest = sizeof(MptEventNotificationRequest);
                    break;
                case MPT_MESSAGE_HDR_FUNCTION_EVENT_ACK:
                    AssertMsgFailed(("todo\n"));
                    //cbRequest = sizeof(MptEventAckRequest);
                    break;
                case MPT_MESSAGE_HDR_FUNCTION_FW_DOWNLOAD:
                    cbRequest = sizeof(MptFWDownloadRequest);
                    break;
                case MPT_MESSAGE_HDR_FUNCTION_FW_UPLOAD:
                    cbRequest = sizeof(MptFWUploadRequest);
                    break;
                default:
                    AssertMsgFailed(("Unknown function issued %u\n", GuestRequest.Header.u8Function));
                    lsilogicSetIOCFaultCode(pThis, LSILOGIC_IOCSTATUS_INVALID_FUNCTION);
            }

            if (cbRequest != 0)
            {
                /* Read the complete message frame from guest memory now. */
                PDMDevHlpPhysRead(pDevIns, GCPhysMessageFrameAddr, &GuestRequest, cbRequest);

                /* Handle SCSI I/O requests now. */
                if (GuestRequest.Header.u8Function == MPT_MESSAGE_HDR_FUNCTION_SCSI_IO_REQUEST)
                {
                   rc = lsilogicR3ProcessSCSIIORequest(pThis, GCPhysMessageFrameAddr, &GuestRequest);
                   AssertRC(rc);
                }
                else
                {
                    MptReplyUnion Reply;
                    rc = lsilogicR3ProcessMessageRequest(pThis, &GuestRequest.Header, &Reply);
                    AssertRC(rc);
                }

                pThis->uRequestQueueNextAddressRead++;
                pThis->uRequestQueueNextAddressRead %= pThis->cRequestQueueEntries;
            }
        } /* While request frames available. */
    } /* While running */

    return VINF_SUCCESS;
}


/**
 * Unblock the worker thread so it can respond to a state change.
 *
 * @returns VBox status code.
 * @param   pDevIns     The pcnet device instance.
 * @param   pThread     The send thread.
 */
static DECLCALLBACK(int) lsilogicR3WorkerWakeUp(PPDMDEVINS pDevIns, PPDMTHREAD pThread)
{
    RT_NOREF(pThread);
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    return SUPSemEventSignal(pThis->pSupDrvSession, pThis->hEvtProcess);
}


/**
 * Kicks the controller to process pending tasks after the VM was resumed
 * or loaded from a saved state.
 *
 * @returns nothing.
 * @param   pThis       Pointer to the LsiLogic device state.
 */
static void lsilogicR3Kick(PLSILOGICSCSI pThis)
{
    if (pThis->fNotificationSent)
    {
        /* Send a notifier to the PDM queue that there are pending requests. */
        PPDMQUEUEITEMCORE pItem = PDMQueueAlloc(pThis->CTX_SUFF(pNotificationQueue));
        AssertMsg(pItem, ("Allocating item for queue failed\n"));
        PDMQueueInsert(pThis->CTX_SUFF(pNotificationQueue), (PPDMQUEUEITEMCORE)pItem);
    }
}


/*
 * Saved state.
 */

/**
 * @callback_method_impl{FNSSMDEVLIVEEXEC}
 */
static DECLCALLBACK(int) lsilogicR3LiveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uPass)
{
    RT_NOREF(uPass);
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);

    SSMR3PutU32(pSSM, pThis->enmCtrlType);
    SSMR3PutU32(pSSM, pThis->cDeviceStates);
    SSMR3PutU32(pSSM, pThis->cPorts);

    /* Save the device config. */
    for (unsigned i = 0; i < pThis->cDeviceStates; i++)
        SSMR3PutBool(pSSM, pThis->paDeviceStates[i].pDrvBase != NULL);

    return VINF_SSM_DONT_CALL_AGAIN;
}

/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) lsilogicR3SaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);

    /* Every device first. */
    lsilogicR3LiveExec(pDevIns, pSSM, SSM_PASS_FINAL);
    for (unsigned i = 0; i < pThis->cDeviceStates; i++)
    {
        PLSILOGICDEVICE pDevice = &pThis->paDeviceStates[i];

        AssertMsg(!pDevice->cOutstandingRequests,
                  ("There are still outstanding requests on this device\n"));
        SSMR3PutU32(pSSM, pDevice->cOutstandingRequests);

        /* Query all suspended requests and store them in the request queue. */
        if (pDevice->pDrvMediaEx)
        {
            uint32_t cReqsRedo = pDevice->pDrvMediaEx->pfnIoReqGetSuspendedCount(pDevice->pDrvMediaEx);
            if (cReqsRedo)
            {
                PDMMEDIAEXIOREQ hIoReq;
                PLSILOGICREQ pReq;
                int rc = pDevice->pDrvMediaEx->pfnIoReqQuerySuspendedStart(pDevice->pDrvMediaEx, &hIoReq,
                                                                           (void **)&pReq);
                AssertRCBreak(rc);

                for (;;)
                {
                    if (!pReq->fBIOS)
                    {
                        /* Write only the lower 32bit part of the address. */
                        ASMAtomicWriteU32(&pThis->CTX_SUFF(pRequestQueueBase)[pThis->uRequestQueueNextEntryFreeWrite],
                                          pReq->GCPhysMessageFrameAddr & UINT32_C(0xffffffff));

                        pThis->uRequestQueueNextEntryFreeWrite++;
                        pThis->uRequestQueueNextEntryFreeWrite %= pThis->cRequestQueueEntries;
                    }
                    else
                    {
                        AssertMsg(!pReq->pRedoNext, ("Only one BIOS task can be active!\n"));
                        vboxscsiSetRequestRedo(&pThis->VBoxSCSI);
                    }

                    cReqsRedo--;
                    if (!cReqsRedo)
                        break;

                    rc = pDevice->pDrvMediaEx->pfnIoReqQuerySuspendedNext(pDevice->pDrvMediaEx, hIoReq,
                                                                          &hIoReq, (void **)&pReq);
                    AssertRCBreak(rc);
                }
            }
        }
    }

    /* Now the main device state. */
    SSMR3PutU32   (pSSM, pThis->enmState);
    SSMR3PutU32   (pSSM, pThis->enmWhoInit);
    SSMR3PutU32   (pSSM, pThis->enmDoorbellState);
    SSMR3PutBool  (pSSM, pThis->fDiagnosticEnabled);
    SSMR3PutBool  (pSSM, pThis->fNotificationSent);
    SSMR3PutBool  (pSSM, pThis->fEventNotificationEnabled);
    SSMR3PutU32   (pSSM, pThis->uInterruptMask);
    SSMR3PutU32   (pSSM, pThis->uInterruptStatus);
    for (unsigned i = 0; i < RT_ELEMENTS(pThis->aMessage); i++)
        SSMR3PutU32   (pSSM, pThis->aMessage[i]);
    SSMR3PutU32   (pSSM, pThis->iMessage);
    SSMR3PutU32   (pSSM, pThis->cMessage);
    SSMR3PutMem   (pSSM, &pThis->ReplyBuffer, sizeof(pThis->ReplyBuffer));
    SSMR3PutU32   (pSSM, pThis->uNextReplyEntryRead);
    SSMR3PutU32   (pSSM, pThis->cReplySize);
    SSMR3PutU16   (pSSM, pThis->u16IOCFaultCode);
    SSMR3PutU32   (pSSM, pThis->u32HostMFAHighAddr);
    SSMR3PutU32   (pSSM, pThis->u32SenseBufferHighAddr);
    SSMR3PutU8    (pSSM, pThis->cMaxDevices);
    SSMR3PutU8    (pSSM, pThis->cMaxBuses);
    SSMR3PutU16   (pSSM, pThis->cbReplyFrame);
    SSMR3PutU32   (pSSM, pThis->iDiagnosticAccess);
    SSMR3PutU32   (pSSM, pThis->cReplyQueueEntries);
    SSMR3PutU32   (pSSM, pThis->cRequestQueueEntries);
    SSMR3PutU32   (pSSM, pThis->uReplyFreeQueueNextEntryFreeWrite);
    SSMR3PutU32   (pSSM, pThis->uReplyFreeQueueNextAddressRead);
    SSMR3PutU32   (pSSM, pThis->uReplyPostQueueNextEntryFreeWrite);
    SSMR3PutU32   (pSSM, pThis->uReplyPostQueueNextAddressRead);
    SSMR3PutU32   (pSSM, pThis->uRequestQueueNextEntryFreeWrite);
    SSMR3PutU32   (pSSM, pThis->uRequestQueueNextAddressRead);

    for (unsigned i = 0; i < pThis->cReplyQueueEntries; i++)
        SSMR3PutU32(pSSM, pThis->pReplyFreeQueueBaseR3[i]);
    for (unsigned i = 0; i < pThis->cReplyQueueEntries; i++)
        SSMR3PutU32(pSSM, pThis->pReplyPostQueueBaseR3[i]);
    for (unsigned i = 0; i < pThis->cRequestQueueEntries; i++)
        SSMR3PutU32(pSSM, pThis->pRequestQueueBaseR3[i]);

    SSMR3PutU16   (pSSM, pThis->u16NextHandle);

    /* Save diagnostic memory register and data regions. */
    SSMR3PutU32   (pSSM, pThis->u32DiagMemAddr);
    SSMR3PutU32   (pSSM, lsilogicR3MemRegionsCount(pThis));

    PLSILOGICMEMREGN pIt;
    RTListForEach(&pThis->ListMemRegns, pIt, LSILOGICMEMREGN, NodeList)
    {
        SSMR3PutU32(pSSM, pIt->u32AddrStart);
        SSMR3PutU32(pSSM, pIt->u32AddrEnd);
        SSMR3PutMem(pSSM, &pIt->au32Data[0], (pIt->u32AddrEnd - pIt->u32AddrStart + 1) * sizeof(uint32_t));
    }

    PMptConfigurationPagesSupported pPages = pThis->pConfigurationPages;

    SSMR3PutMem   (pSSM, &pPages->ManufacturingPage0, sizeof(MptConfigurationPageManufacturing0));
    SSMR3PutMem   (pSSM, &pPages->ManufacturingPage1, sizeof(MptConfigurationPageManufacturing1));
    SSMR3PutMem   (pSSM, &pPages->ManufacturingPage2, sizeof(MptConfigurationPageManufacturing2));
    SSMR3PutMem   (pSSM, &pPages->ManufacturingPage3, sizeof(MptConfigurationPageManufacturing3));
    SSMR3PutMem   (pSSM, &pPages->ManufacturingPage4, sizeof(MptConfigurationPageManufacturing4));
    SSMR3PutMem   (pSSM, &pPages->ManufacturingPage5, sizeof(MptConfigurationPageManufacturing5));
    SSMR3PutMem   (pSSM, &pPages->ManufacturingPage6, sizeof(MptConfigurationPageManufacturing6));
    SSMR3PutMem   (pSSM, &pPages->ManufacturingPage8, sizeof(MptConfigurationPageManufacturing8));
    SSMR3PutMem   (pSSM, &pPages->ManufacturingPage9, sizeof(MptConfigurationPageManufacturing9));
    SSMR3PutMem   (pSSM, &pPages->ManufacturingPage10, sizeof(MptConfigurationPageManufacturing10));
    SSMR3PutMem   (pSSM, &pPages->IOUnitPage0, sizeof(MptConfigurationPageIOUnit0));
    SSMR3PutMem   (pSSM, &pPages->IOUnitPage1, sizeof(MptConfigurationPageIOUnit1));
    SSMR3PutMem   (pSSM, &pPages->IOUnitPage2, sizeof(MptConfigurationPageIOUnit2));
    SSMR3PutMem   (pSSM, &pPages->IOUnitPage3, sizeof(MptConfigurationPageIOUnit3));
    SSMR3PutMem   (pSSM, &pPages->IOUnitPage4, sizeof(MptConfigurationPageIOUnit4));
    SSMR3PutMem   (pSSM, &pPages->IOCPage0, sizeof(MptConfigurationPageIOC0));
    SSMR3PutMem   (pSSM, &pPages->IOCPage1, sizeof(MptConfigurationPageIOC1));
    SSMR3PutMem   (pSSM, &pPages->IOCPage2, sizeof(MptConfigurationPageIOC2));
    SSMR3PutMem   (pSSM, &pPages->IOCPage3, sizeof(MptConfigurationPageIOC3));
    SSMR3PutMem   (pSSM, &pPages->IOCPage4, sizeof(MptConfigurationPageIOC4));
    SSMR3PutMem   (pSSM, &pPages->IOCPage6, sizeof(MptConfigurationPageIOC6));
    SSMR3PutMem   (pSSM, &pPages->BIOSPage1, sizeof(MptConfigurationPageBIOS1));
    SSMR3PutMem   (pSSM, &pPages->BIOSPage2, sizeof(MptConfigurationPageBIOS2));
    SSMR3PutMem   (pSSM, &pPages->BIOSPage4, sizeof(MptConfigurationPageBIOS4));

    /* Device dependent pages */
    if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI)
    {
        PMptConfigurationPagesSpi pSpiPages = &pPages->u.SpiPages;

        SSMR3PutMem(pSSM, &pSpiPages->aPortPages[0].SCSISPIPortPage0, sizeof(MptConfigurationPageSCSISPIPort0));
        SSMR3PutMem(pSSM, &pSpiPages->aPortPages[0].SCSISPIPortPage1, sizeof(MptConfigurationPageSCSISPIPort1));
        SSMR3PutMem(pSSM, &pSpiPages->aPortPages[0].SCSISPIPortPage2, sizeof(MptConfigurationPageSCSISPIPort2));

        for (unsigned i = 0; i < RT_ELEMENTS(pSpiPages->aBuses[0].aDevicePages); i++)
        {
            SSMR3PutMem(pSSM, &pSpiPages->aBuses[0].aDevicePages[i].SCSISPIDevicePage0, sizeof(MptConfigurationPageSCSISPIDevice0));
            SSMR3PutMem(pSSM, &pSpiPages->aBuses[0].aDevicePages[i].SCSISPIDevicePage1, sizeof(MptConfigurationPageSCSISPIDevice1));
            SSMR3PutMem(pSSM, &pSpiPages->aBuses[0].aDevicePages[i].SCSISPIDevicePage2, sizeof(MptConfigurationPageSCSISPIDevice2));
            SSMR3PutMem(pSSM, &pSpiPages->aBuses[0].aDevicePages[i].SCSISPIDevicePage3, sizeof(MptConfigurationPageSCSISPIDevice3));
        }
    }
    else if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
    {
        PMptConfigurationPagesSas pSasPages = &pPages->u.SasPages;

        SSMR3PutU32(pSSM, pSasPages->cbManufacturingPage7);
        SSMR3PutU32(pSSM, pSasPages->cbSASIOUnitPage0);
        SSMR3PutU32(pSSM, pSasPages->cbSASIOUnitPage1);

        SSMR3PutMem(pSSM, pSasPages->pManufacturingPage7, pSasPages->cbManufacturingPage7);
        SSMR3PutMem(pSSM, pSasPages->pSASIOUnitPage0, pSasPages->cbSASIOUnitPage0);
        SSMR3PutMem(pSSM, pSasPages->pSASIOUnitPage1, pSasPages->cbSASIOUnitPage1);

        SSMR3PutMem(pSSM, &pSasPages->SASIOUnitPage2, sizeof(MptConfigurationPageSASIOUnit2));
        SSMR3PutMem(pSSM, &pSasPages->SASIOUnitPage3, sizeof(MptConfigurationPageSASIOUnit3));

        SSMR3PutU32(pSSM, pSasPages->cPHYs);
        for (unsigned i = 0; i < pSasPages->cPHYs; i++)
        {
            SSMR3PutMem(pSSM, &pSasPages->paPHYs[i].SASPHYPage0, sizeof(MptConfigurationPageSASPHY0));
            SSMR3PutMem(pSSM, &pSasPages->paPHYs[i].SASPHYPage1, sizeof(MptConfigurationPageSASPHY1));
        }

        /* The number of devices first. */
        SSMR3PutU32(pSSM, pSasPages->cDevices);

        PMptSASDevice pCurr = pSasPages->pSASDeviceHead;

        while (pCurr)
        {
            SSMR3PutMem(pSSM, &pCurr->SASDevicePage0, sizeof(MptConfigurationPageSASDevice0));
            SSMR3PutMem(pSSM, &pCurr->SASDevicePage1, sizeof(MptConfigurationPageSASDevice1));
            SSMR3PutMem(pSSM, &pCurr->SASDevicePage2, sizeof(MptConfigurationPageSASDevice2));

            pCurr = pCurr->pNext;
        }
    }
    else
        AssertMsgFailed(("Invalid controller type %d\n", pThis->enmCtrlType));

    vboxscsiR3SaveExec(&pThis->VBoxSCSI, pSSM);
    return SSMR3PutU32(pSSM, UINT32_MAX);
}

/**
 * @callback_method_impl{FNSSMDEVLOADDONE}
 */
static DECLCALLBACK(int) lsilogicR3LoadDone(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    RT_NOREF(pSSM);
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);

    lsilogicR3Kick(pThis);
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) lsilogicR3LoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PLSILOGICSCSI   pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    int             rc;

    if (    uVersion != LSILOGIC_SAVED_STATE_VERSION
        &&  uVersion != LSILOGIC_SAVED_STATE_VERSION_PRE_DIAG_MEM
        &&  uVersion != LSILOGIC_SAVED_STATE_VERSION_BOOL_DOORBELL
        &&  uVersion != LSILOGIC_SAVED_STATE_VERSION_PRE_SAS
        &&  uVersion != LSILOGIC_SAVED_STATE_VERSION_VBOX_30)
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;

    /* device config */
    if (uVersion > LSILOGIC_SAVED_STATE_VERSION_PRE_SAS)
    {
        LSILOGICCTRLTYPE enmCtrlType;
        uint32_t cDeviceStates, cPorts;

        rc = SSMR3GetU32(pSSM, (uint32_t *)&enmCtrlType);
        AssertRCReturn(rc, rc);
        rc = SSMR3GetU32(pSSM, &cDeviceStates);
        AssertRCReturn(rc, rc);
        rc = SSMR3GetU32(pSSM, &cPorts);
        AssertRCReturn(rc, rc);

        if (enmCtrlType != pThis->enmCtrlType)
            return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Target config mismatch (Controller type): config=%d state=%d"),
                                    pThis->enmCtrlType, enmCtrlType);
        if (cDeviceStates != pThis->cDeviceStates)
            return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Target config mismatch (Device states): config=%u state=%u"),
                                    pThis->cDeviceStates, cDeviceStates);
        if (cPorts != pThis->cPorts)
            return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Target config mismatch (Ports): config=%u state=%u"),
                                    pThis->cPorts, cPorts);
    }
    if (uVersion > LSILOGIC_SAVED_STATE_VERSION_VBOX_30)
    {
        for (unsigned i = 0; i < pThis->cDeviceStates; i++)
        {
            bool fPresent;
            rc = SSMR3GetBool(pSSM, &fPresent);
            AssertRCReturn(rc, rc);
            if (fPresent != (pThis->paDeviceStates[i].pDrvBase != NULL))
                return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Target %u config mismatch: config=%RTbool state=%RTbool"),
                                         i, pThis->paDeviceStates[i].pDrvBase != NULL, fPresent);
        }
    }
    if (uPass != SSM_PASS_FINAL)
        return VINF_SUCCESS;

    /* Every device first. */
    for (unsigned i = 0; i < pThis->cDeviceStates; i++)
    {
        PLSILOGICDEVICE pDevice = &pThis->paDeviceStates[i];

        AssertMsg(!pDevice->cOutstandingRequests,
                  ("There are still outstanding requests on this device\n"));
        SSMR3GetU32(pSSM, (uint32_t *)&pDevice->cOutstandingRequests);
    }
    /* Now the main device state. */
    SSMR3GetU32   (pSSM, (uint32_t *)&pThis->enmState);
    SSMR3GetU32   (pSSM, (uint32_t *)&pThis->enmWhoInit);
    if (uVersion <= LSILOGIC_SAVED_STATE_VERSION_BOOL_DOORBELL)
    {
        bool fDoorbellInProgress = false;

        /*
         * The doorbell status flag distinguishes only between
         * doorbell not in use or a Function handshake is currently in progress.
         */
        SSMR3GetBool  (pSSM, &fDoorbellInProgress);
        if (fDoorbellInProgress)
            pThis->enmDoorbellState = LSILOGICDOORBELLSTATE_FN_HANDSHAKE;
        else
            pThis->enmDoorbellState = LSILOGICDOORBELLSTATE_NOT_IN_USE;
    }
    else
        SSMR3GetU32(pSSM, (uint32_t *)&pThis->enmDoorbellState);
    SSMR3GetBool  (pSSM, &pThis->fDiagnosticEnabled);
    SSMR3GetBool  (pSSM, &pThis->fNotificationSent);
    SSMR3GetBool  (pSSM, &pThis->fEventNotificationEnabled);
    SSMR3GetU32   (pSSM, (uint32_t *)&pThis->uInterruptMask);
    SSMR3GetU32   (pSSM, (uint32_t *)&pThis->uInterruptStatus);
    for (unsigned i = 0; i < RT_ELEMENTS(pThis->aMessage); i++)
        SSMR3GetU32   (pSSM, &pThis->aMessage[i]);
    SSMR3GetU32   (pSSM, &pThis->iMessage);
    SSMR3GetU32   (pSSM, &pThis->cMessage);
    SSMR3GetMem   (pSSM, &pThis->ReplyBuffer, sizeof(pThis->ReplyBuffer));
    SSMR3GetU32   (pSSM, &pThis->uNextReplyEntryRead);
    SSMR3GetU32   (pSSM, &pThis->cReplySize);
    SSMR3GetU16   (pSSM, &pThis->u16IOCFaultCode);
    SSMR3GetU32   (pSSM, &pThis->u32HostMFAHighAddr);
    SSMR3GetU32   (pSSM, &pThis->u32SenseBufferHighAddr);
    SSMR3GetU8    (pSSM, &pThis->cMaxDevices);
    SSMR3GetU8    (pSSM, &pThis->cMaxBuses);
    SSMR3GetU16   (pSSM, &pThis->cbReplyFrame);
    SSMR3GetU32   (pSSM, &pThis->iDiagnosticAccess);

    uint32_t cReplyQueueEntries, cRequestQueueEntries;
    SSMR3GetU32   (pSSM, &cReplyQueueEntries);
    SSMR3GetU32   (pSSM, &cRequestQueueEntries);

    if (   cReplyQueueEntries != pThis->cReplyQueueEntries
        || cRequestQueueEntries != pThis->cRequestQueueEntries)
    {
        LogFlow(("Reallocating queues cReplyQueueEntries=%u cRequestQueuEntries=%u\n",
                 cReplyQueueEntries, cRequestQueueEntries));
        lsilogicR3QueuesFree(pThis);
        pThis->cReplyQueueEntries = cReplyQueueEntries;
        pThis->cRequestQueueEntries = cRequestQueueEntries;
        rc = lsilogicR3QueuesAlloc(pThis);
        if (RT_FAILURE(rc))
            return rc;
    }

    SSMR3GetU32   (pSSM, (uint32_t *)&pThis->uReplyFreeQueueNextEntryFreeWrite);
    SSMR3GetU32   (pSSM, (uint32_t *)&pThis->uReplyFreeQueueNextAddressRead);
    SSMR3GetU32   (pSSM, (uint32_t *)&pThis->uReplyPostQueueNextEntryFreeWrite);
    SSMR3GetU32   (pSSM, (uint32_t *)&pThis->uReplyPostQueueNextAddressRead);
    SSMR3GetU32   (pSSM, (uint32_t *)&pThis->uRequestQueueNextEntryFreeWrite);
    SSMR3GetU32   (pSSM, (uint32_t *)&pThis->uRequestQueueNextAddressRead);

    PMptConfigurationPagesSupported pPages = pThis->pConfigurationPages;

    if (uVersion <= LSILOGIC_SAVED_STATE_VERSION_PRE_SAS)
    {
        PMptConfigurationPagesSpi pSpiPages = &pPages->u.SpiPages;
        MptConfigurationPagesSupported_SSM_V2 ConfigPagesV2;

        if (pThis->enmCtrlType != LSILOGICCTRLTYPE_SCSI_SPI)
            return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Config mismatch: Expected SPI SCSI controller"));

        SSMR3GetMem(pSSM, &ConfigPagesV2,
                    sizeof(MptConfigurationPagesSupported_SSM_V2));

        pPages->ManufacturingPage0 = ConfigPagesV2.ManufacturingPage0;
        pPages->ManufacturingPage1 = ConfigPagesV2.ManufacturingPage1;
        pPages->ManufacturingPage2 = ConfigPagesV2.ManufacturingPage2;
        pPages->ManufacturingPage3 = ConfigPagesV2.ManufacturingPage3;
        pPages->ManufacturingPage4 = ConfigPagesV2.ManufacturingPage4;
        pPages->IOUnitPage0 = ConfigPagesV2.IOUnitPage0;
        pPages->IOUnitPage1 = ConfigPagesV2.IOUnitPage1;
        pPages->IOUnitPage2 = ConfigPagesV2.IOUnitPage2;
        pPages->IOUnitPage3 = ConfigPagesV2.IOUnitPage3;
        pPages->IOCPage0 = ConfigPagesV2.IOCPage0;
        pPages->IOCPage1 = ConfigPagesV2.IOCPage1;
        pPages->IOCPage2 = ConfigPagesV2.IOCPage2;
        pPages->IOCPage3 = ConfigPagesV2.IOCPage3;
        pPages->IOCPage4 = ConfigPagesV2.IOCPage4;
        pPages->IOCPage6 = ConfigPagesV2.IOCPage6;

        pSpiPages->aPortPages[0].SCSISPIPortPage0 = ConfigPagesV2.aPortPages[0].SCSISPIPortPage0;
        pSpiPages->aPortPages[0].SCSISPIPortPage1 = ConfigPagesV2.aPortPages[0].SCSISPIPortPage1;
        pSpiPages->aPortPages[0].SCSISPIPortPage2 = ConfigPagesV2.aPortPages[0].SCSISPIPortPage2;

        for (unsigned i = 0; i < RT_ELEMENTS(pPages->u.SpiPages.aBuses[0].aDevicePages); i++)
        {
            pSpiPages->aBuses[0].aDevicePages[i].SCSISPIDevicePage0 = ConfigPagesV2.aBuses[0].aDevicePages[i].SCSISPIDevicePage0;
            pSpiPages->aBuses[0].aDevicePages[i].SCSISPIDevicePage1 = ConfigPagesV2.aBuses[0].aDevicePages[i].SCSISPIDevicePage1;
            pSpiPages->aBuses[0].aDevicePages[i].SCSISPIDevicePage2 = ConfigPagesV2.aBuses[0].aDevicePages[i].SCSISPIDevicePage2;
            pSpiPages->aBuses[0].aDevicePages[i].SCSISPIDevicePage3 = ConfigPagesV2.aBuses[0].aDevicePages[i].SCSISPIDevicePage3;
        }
    }
    else
    {
        /* Queue content */
        for (unsigned i = 0; i < pThis->cReplyQueueEntries; i++)
            SSMR3GetU32(pSSM, (uint32_t *)&pThis->pReplyFreeQueueBaseR3[i]);
        for (unsigned i = 0; i < pThis->cReplyQueueEntries; i++)
            SSMR3GetU32(pSSM, (uint32_t *)&pThis->pReplyPostQueueBaseR3[i]);
        for (unsigned i = 0; i < pThis->cRequestQueueEntries; i++)
            SSMR3GetU32(pSSM, (uint32_t *)&pThis->pRequestQueueBaseR3[i]);

        SSMR3GetU16(pSSM, &pThis->u16NextHandle);

        if (uVersion > LSILOGIC_SAVED_STATE_VERSION_PRE_DIAG_MEM)
        {
            uint32_t cMemRegions = 0;

            /* Save diagnostic memory register and data regions. */
            SSMR3GetU32   (pSSM, &pThis->u32DiagMemAddr);
            SSMR3GetU32   (pSSM, &cMemRegions);

            while (cMemRegions)
            {
                uint32_t u32AddrStart = 0;
                uint32_t u32AddrEnd = 0;
                uint32_t cRegion = 0;
                PLSILOGICMEMREGN pRegion = NULL;

                SSMR3GetU32(pSSM, &u32AddrStart);
                SSMR3GetU32(pSSM, &u32AddrEnd);

                cRegion = u32AddrEnd - u32AddrStart + 1;
                pRegion = (PLSILOGICMEMREGN)RTMemAllocZ(RT_OFFSETOF(LSILOGICMEMREGN, au32Data[cRegion]));
                if (pRegion)
                {
                    pRegion->u32AddrStart = u32AddrStart;
                    pRegion->u32AddrEnd = u32AddrEnd;
                    SSMR3GetMem(pSSM, &pRegion->au32Data[0], cRegion * sizeof(uint32_t));
                    lsilogicR3MemRegionInsert(pThis, pRegion);
                    pThis->cbMemRegns += cRegion * sizeof(uint32_t);
                }
                else
                {
                    /* Leave a log message but continue. */
                    LogRel(("LsiLogic: Out of memory while restoring the state, might not work as expected\n"));
                    SSMR3Skip(pSSM, cRegion * sizeof(uint32_t));
                }
                cMemRegions--;
            }
        }

        /* Configuration pages */
        SSMR3GetMem(pSSM, &pPages->ManufacturingPage0, sizeof(MptConfigurationPageManufacturing0));
        SSMR3GetMem(pSSM, &pPages->ManufacturingPage1, sizeof(MptConfigurationPageManufacturing1));
        SSMR3GetMem(pSSM, &pPages->ManufacturingPage2, sizeof(MptConfigurationPageManufacturing2));
        SSMR3GetMem(pSSM, &pPages->ManufacturingPage3, sizeof(MptConfigurationPageManufacturing3));
        SSMR3GetMem(pSSM, &pPages->ManufacturingPage4, sizeof(MptConfigurationPageManufacturing4));
        SSMR3GetMem(pSSM, &pPages->ManufacturingPage5, sizeof(MptConfigurationPageManufacturing5));
        SSMR3GetMem(pSSM, &pPages->ManufacturingPage6, sizeof(MptConfigurationPageManufacturing6));
        SSMR3GetMem(pSSM, &pPages->ManufacturingPage8, sizeof(MptConfigurationPageManufacturing8));
        SSMR3GetMem(pSSM, &pPages->ManufacturingPage9, sizeof(MptConfigurationPageManufacturing9));
        SSMR3GetMem(pSSM, &pPages->ManufacturingPage10, sizeof(MptConfigurationPageManufacturing10));
        SSMR3GetMem(pSSM, &pPages->IOUnitPage0, sizeof(MptConfigurationPageIOUnit0));
        SSMR3GetMem(pSSM, &pPages->IOUnitPage1, sizeof(MptConfigurationPageIOUnit1));
        SSMR3GetMem(pSSM, &pPages->IOUnitPage2, sizeof(MptConfigurationPageIOUnit2));
        SSMR3GetMem(pSSM, &pPages->IOUnitPage3, sizeof(MptConfigurationPageIOUnit3));
        SSMR3GetMem(pSSM, &pPages->IOUnitPage4, sizeof(MptConfigurationPageIOUnit4));
        SSMR3GetMem(pSSM, &pPages->IOCPage0, sizeof(MptConfigurationPageIOC0));
        SSMR3GetMem(pSSM, &pPages->IOCPage1, sizeof(MptConfigurationPageIOC1));
        SSMR3GetMem(pSSM, &pPages->IOCPage2, sizeof(MptConfigurationPageIOC2));
        SSMR3GetMem(pSSM, &pPages->IOCPage3, sizeof(MptConfigurationPageIOC3));
        SSMR3GetMem(pSSM, &pPages->IOCPage4, sizeof(MptConfigurationPageIOC4));
        SSMR3GetMem(pSSM, &pPages->IOCPage6, sizeof(MptConfigurationPageIOC6));
        SSMR3GetMem(pSSM, &pPages->BIOSPage1, sizeof(MptConfigurationPageBIOS1));
        SSMR3GetMem(pSSM, &pPages->BIOSPage2, sizeof(MptConfigurationPageBIOS2));
        SSMR3GetMem(pSSM, &pPages->BIOSPage4, sizeof(MptConfigurationPageBIOS4));

        /* Device dependent pages */
        if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI)
        {
            PMptConfigurationPagesSpi pSpiPages = &pPages->u.SpiPages;

            SSMR3GetMem(pSSM, &pSpiPages->aPortPages[0].SCSISPIPortPage0, sizeof(MptConfigurationPageSCSISPIPort0));
            SSMR3GetMem(pSSM, &pSpiPages->aPortPages[0].SCSISPIPortPage1, sizeof(MptConfigurationPageSCSISPIPort1));
            SSMR3GetMem(pSSM, &pSpiPages->aPortPages[0].SCSISPIPortPage2, sizeof(MptConfigurationPageSCSISPIPort2));

            for (unsigned i = 0; i < RT_ELEMENTS(pSpiPages->aBuses[0].aDevicePages); i++)
            {
                SSMR3GetMem(pSSM, &pSpiPages->aBuses[0].aDevicePages[i].SCSISPIDevicePage0, sizeof(MptConfigurationPageSCSISPIDevice0));
                SSMR3GetMem(pSSM, &pSpiPages->aBuses[0].aDevicePages[i].SCSISPIDevicePage1, sizeof(MptConfigurationPageSCSISPIDevice1));
                SSMR3GetMem(pSSM, &pSpiPages->aBuses[0].aDevicePages[i].SCSISPIDevicePage2, sizeof(MptConfigurationPageSCSISPIDevice2));
                SSMR3GetMem(pSSM, &pSpiPages->aBuses[0].aDevicePages[i].SCSISPIDevicePage3, sizeof(MptConfigurationPageSCSISPIDevice3));
            }
        }
        else if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
        {
            uint32_t cbPage0, cbPage1, cPHYs, cbManufacturingPage7;
            PMptConfigurationPagesSas pSasPages = &pPages->u.SasPages;

            SSMR3GetU32(pSSM, &cbManufacturingPage7);
            SSMR3GetU32(pSSM, &cbPage0);
            SSMR3GetU32(pSSM, &cbPage1);

            if (   (cbPage0 != pSasPages->cbSASIOUnitPage0)
                || (cbPage1 != pSasPages->cbSASIOUnitPage1)
                || (cbManufacturingPage7 != pSasPages->cbManufacturingPage7))
                return VERR_SSM_LOAD_CONFIG_MISMATCH;

            AssertPtr(pSasPages->pManufacturingPage7);
            AssertPtr(pSasPages->pSASIOUnitPage0);
            AssertPtr(pSasPages->pSASIOUnitPage1);

            SSMR3GetMem(pSSM, pSasPages->pManufacturingPage7, pSasPages->cbManufacturingPage7);
            SSMR3GetMem(pSSM, pSasPages->pSASIOUnitPage0, pSasPages->cbSASIOUnitPage0);
            SSMR3GetMem(pSSM, pSasPages->pSASIOUnitPage1, pSasPages->cbSASIOUnitPage1);

            SSMR3GetMem(pSSM, &pSasPages->SASIOUnitPage2, sizeof(MptConfigurationPageSASIOUnit2));
            SSMR3GetMem(pSSM, &pSasPages->SASIOUnitPage3, sizeof(MptConfigurationPageSASIOUnit3));

            SSMR3GetU32(pSSM, &cPHYs);
            if (cPHYs != pSasPages->cPHYs)
                return VERR_SSM_LOAD_CONFIG_MISMATCH;

            AssertPtr(pSasPages->paPHYs);
            for (unsigned i = 0; i < pSasPages->cPHYs; i++)
            {
                SSMR3GetMem(pSSM, &pSasPages->paPHYs[i].SASPHYPage0, sizeof(MptConfigurationPageSASPHY0));
                SSMR3GetMem(pSSM, &pSasPages->paPHYs[i].SASPHYPage1, sizeof(MptConfigurationPageSASPHY1));
            }

            /* The number of devices first. */
            SSMR3GetU32(pSSM, &pSasPages->cDevices);

            PMptSASDevice pCurr = pSasPages->pSASDeviceHead;

            for (unsigned i = 0; i < pSasPages->cDevices; i++)
            {
                SSMR3GetMem(pSSM, &pCurr->SASDevicePage0, sizeof(MptConfigurationPageSASDevice0));
                SSMR3GetMem(pSSM, &pCurr->SASDevicePage1, sizeof(MptConfigurationPageSASDevice1));
                SSMR3GetMem(pSSM, &pCurr->SASDevicePage2, sizeof(MptConfigurationPageSASDevice2));

                pCurr = pCurr->pNext;
            }

            Assert(!pCurr);
        }
        else
            AssertMsgFailed(("Invalid controller type %d\n", pThis->enmCtrlType));
    }

    rc = vboxscsiR3LoadExec(&pThis->VBoxSCSI, pSSM);
    if (RT_FAILURE(rc))
    {
        LogRel(("LsiLogic: Failed to restore BIOS state: %Rrc.\n", rc));
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("LsiLogic: Failed to restore BIOS state\n"));
    }

    uint32_t u32;
    rc = SSMR3GetU32(pSSM, &u32);
    if (RT_FAILURE(rc))
        return rc;
    AssertMsgReturn(u32 == UINT32_MAX, ("%#x\n", u32), VERR_SSM_DATA_UNIT_FORMAT_CHANGED);

    return VINF_SUCCESS;
}


/*
 * The device level IBASE and LED interfaces.
 */

/**
 * @interface_method_impl{PDMILEDPORTS,pfnQueryStatusLed, For a SCSI device.}
 *
 * @remarks Called by the scsi driver, proxying the main calls.
 */
static DECLCALLBACK(int) lsilogicR3DeviceQueryStatusLed(PPDMILEDPORTS pInterface, unsigned iLUN, PPDMLED *ppLed)
{
    PLSILOGICDEVICE pDevice = RT_FROM_MEMBER(pInterface, LSILOGICDEVICE, ILed);
    if (iLUN == 0)
    {
        *ppLed = &pDevice->Led;
        Assert((*ppLed)->u32Magic == PDMLED_MAGIC);
        return VINF_SUCCESS;
    }
    return VERR_PDM_LUN_NOT_FOUND;
}


/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) lsilogicR3DeviceQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PLSILOGICDEVICE pDevice = RT_FROM_MEMBER(pInterface, LSILOGICDEVICE, IBase);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDevice->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMEDIAPORT, &pDevice->IMediaPort);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMEDIAEXPORT, &pDevice->IMediaExPort);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMILEDPORTS, &pDevice->ILed);
    return NULL;
}


/*
 * The controller level IBASE and LED interfaces.
 */

/**
 * Gets the pointer to the status LED of a unit.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @param   iLUN            The unit which status LED we desire.
 * @param   ppLed           Where to store the LED pointer.
 */
static DECLCALLBACK(int) lsilogicR3StatusQueryStatusLed(PPDMILEDPORTS pInterface, unsigned iLUN, PPDMLED *ppLed)
{
    PLSILOGICSCSI pThis = RT_FROM_MEMBER(pInterface, LSILOGICSCSI, ILeds);
    if (iLUN < pThis->cDeviceStates)
    {
        *ppLed = &pThis->paDeviceStates[iLUN].Led;
        Assert((*ppLed)->u32Magic == PDMLED_MAGIC);
        return VINF_SUCCESS;
    }
    return VERR_PDM_LUN_NOT_FOUND;
}

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) lsilogicR3StatusQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PLSILOGICSCSI pThis = RT_FROM_MEMBER(pInterface, LSILOGICSCSI, IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMILEDPORTS, &pThis->ILeds);
    return NULL;
}


/*
 * The PDM device interface and some helpers.
 */

/**
 * Checks if all asynchronous I/O is finished.
 *
 * Used by lsilogicR3Reset, lsilogicR3Suspend and lsilogicR3PowerOff.
 *
 * @returns true if quiesced, false if busy.
 * @param   pDevIns         The device instance.
 */
static bool lsilogicR3AllAsyncIOIsFinished(PPDMDEVINS pDevIns)
{
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);

    for (uint32_t i = 0; i < pThis->cDeviceStates; i++)
    {
        PLSILOGICDEVICE pThisDevice = &pThis->paDeviceStates[i];
        if (pThisDevice->pDrvBase)
        {
            if (pThisDevice->cOutstandingRequests != 0)
                return false;
        }
    }

    return true;
}

/**
 * @callback_method_impl{FNPDMDEVASYNCNOTIFY,
 * Callback employed by lsilogicR3Suspend and lsilogicR3PowerOff.}
 */
static DECLCALLBACK(bool) lsilogicR3IsAsyncSuspendOrPowerOffDone(PPDMDEVINS pDevIns)
{
    if (!lsilogicR3AllAsyncIOIsFinished(pDevIns))
        return false;

    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    ASMAtomicWriteBool(&pThis->fSignalIdle, false);
    return true;
}

/**
 * Common worker for ahciR3Suspend and ahciR3PowerOff.
 */
static void lsilogicR3SuspendOrPowerOff(PPDMDEVINS pDevIns)
{
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);

    ASMAtomicWriteBool(&pThis->fSignalIdle, true);
    if (!lsilogicR3AllAsyncIOIsFinished(pDevIns))
        PDMDevHlpSetAsyncNotification(pDevIns, lsilogicR3IsAsyncSuspendOrPowerOffDone);
    else
    {
        ASMAtomicWriteBool(&pThis->fSignalIdle, false);

        AssertMsg(!pThis->fNotificationSent, ("The PDM Queue should be empty at this point\n"));
    }

    for (uint32_t i = 0; i < pThis->cDeviceStates; i++)
    {
        PLSILOGICDEVICE pThisDevice = &pThis->paDeviceStates[i];
        if (pThisDevice->pDrvMediaEx)
            pThisDevice->pDrvMediaEx->pfnNotifySuspend(pThisDevice->pDrvMediaEx);
    }
}

/**
 * @interface_method_impl{PDMDEVREG,pfnSuspend}
 */
static DECLCALLBACK(void) lsilogicR3Suspend(PPDMDEVINS pDevIns)
{
    Log(("lsilogicR3Suspend\n"));
    lsilogicR3SuspendOrPowerOff(pDevIns);
}

/**
 * @interface_method_impl{PDMDEVREG,pfnResume}
 */
static DECLCALLBACK(void) lsilogicR3Resume(PPDMDEVINS pDevIns)
{
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);

    Log(("lsilogicR3Resume\n"));

    lsilogicR3Kick(pThis);
}

/**
 * @interface_method_impl{PDMDEVREG,pfnDetach}
 *
 * One harddisk at one port has been unplugged.
 * The VM is suspended at this point.
 */
static DECLCALLBACK(void) lsilogicR3Detach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PLSILOGICSCSI   pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    PLSILOGICDEVICE pDevice = &pThis->paDeviceStates[iLUN];

    if (iLUN >= pThis->cDeviceStates)
        return;

    AssertMsg(fFlags & PDM_TACH_FLAGS_NOT_HOT_PLUG,
              ("LsiLogic: Device does not support hotplugging\n"));

    Log(("%s:\n", __FUNCTION__));

    /*
     * Zero some important members.
     */
    pDevice->pDrvBase = NULL;
    pDevice->pDrvMedia = NULL;
    pDevice->pDrvMediaEx = NULL;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnAttach}
 */
static DECLCALLBACK(int)  lsilogicR3Attach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
    PLSILOGICSCSI   pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    PLSILOGICDEVICE pDevice = &pThis->paDeviceStates[iLUN];
    int rc;

    if (iLUN >= pThis->cDeviceStates)
        return VERR_PDM_LUN_NOT_FOUND;

    AssertMsgReturn(fFlags & PDM_TACH_FLAGS_NOT_HOT_PLUG,
                    ("LsiLogic: Device does not support hotplugging\n"),
                    VERR_INVALID_PARAMETER);

    /* the usual paranoia */
    AssertRelease(!pDevice->pDrvBase);
    AssertRelease(!pDevice->pDrvMedia);
    AssertRelease(!pDevice->pDrvMediaEx);
    Assert(pDevice->iLUN == iLUN);

    /*
     * Try attach the block device and get the interfaces,
     * required as well as optional.
     */
    rc = PDMDevHlpDriverAttach(pDevIns, pDevice->iLUN, &pDevice->IBase, &pDevice->pDrvBase, NULL);
    if (RT_SUCCESS(rc))
    {
        /* Query the media interface. */
        pDevice->pDrvMedia = PDMIBASE_QUERY_INTERFACE(pDevice->pDrvBase, PDMIMEDIA);
        AssertMsgReturn(VALID_PTR(pDevice->pDrvMedia),
                        ("LsiLogic configuration error: LUN#%d misses the basic media interface!\n", pDevice->iLUN),
                        VERR_PDM_MISSING_INTERFACE);

        /* Get the extended media interface. */
        pDevice->pDrvMediaEx = PDMIBASE_QUERY_INTERFACE(pDevice->pDrvBase, PDMIMEDIAEX);
        AssertMsgReturn(VALID_PTR(pDevice->pDrvMediaEx),
                        ("LsiLogic configuration error: LUN#%d misses the extended media interface!\n", pDevice->iLUN),
                        VERR_PDM_MISSING_INTERFACE);

        rc = pDevice->pDrvMediaEx->pfnIoReqAllocSizeSet(pDevice->pDrvMediaEx, sizeof(LSILOGICREQ));
        AssertMsgRCReturn(rc, ("LsiLogic configuration error: LUN#%u: Failed to set I/O request size!", pDevice->iLUN),
                          rc);
    }
    else
        AssertMsgFailed(("Failed to attach LUN#%d. rc=%Rrc\n", pDevice->iLUN, rc));

    if (RT_FAILURE(rc))
    {
        pDevice->pDrvBase = NULL;
        pDevice->pDrvMedia = NULL;
        pDevice->pDrvMediaEx = NULL;
    }
    return rc;
}

/**
 * Common reset worker.
 *
 * @param   pDevIns     The device instance data.
 */
static void lsilogicR3ResetCommon(PPDMDEVINS pDevIns)
{
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    int rc;

    rc = lsilogicR3HardReset(pThis);
    AssertRC(rc);

    vboxscsiInitialize(&pThis->VBoxSCSI);
}

/**
 * @callback_method_impl{FNPDMDEVASYNCNOTIFY,
 * Callback employed by lsilogicR3Reset.}
 */
static DECLCALLBACK(bool) lsilogicR3IsAsyncResetDone(PPDMDEVINS pDevIns)
{
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);

    if (!lsilogicR3AllAsyncIOIsFinished(pDevIns))
        return false;
    ASMAtomicWriteBool(&pThis->fSignalIdle, false);

    lsilogicR3ResetCommon(pDevIns);
    return true;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 */
static DECLCALLBACK(void) lsilogicR3Reset(PPDMDEVINS pDevIns)
{
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);

    ASMAtomicWriteBool(&pThis->fSignalIdle, true);
    if (!lsilogicR3AllAsyncIOIsFinished(pDevIns))
        PDMDevHlpSetAsyncNotification(pDevIns, lsilogicR3IsAsyncResetDone);
    else
    {
        ASMAtomicWriteBool(&pThis->fSignalIdle, false);
        lsilogicR3ResetCommon(pDevIns);
    }
}

/**
 * @interface_method_impl{PDMDEVREG,pfnRelocate}
 */
static DECLCALLBACK(void) lsilogicR3Relocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta)
{
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);

    pThis->pDevInsRC        = PDMDEVINS_2_RCPTR(pDevIns);
    pThis->pNotificationQueueRC = PDMQueueRCPtr(pThis->pNotificationQueueR3);

    /* Relocate queues. */
    pThis->pReplyFreeQueueBaseRC += offDelta;
    pThis->pReplyPostQueueBaseRC += offDelta;
    pThis->pRequestQueueBaseRC   += offDelta;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnPowerOff}
 */
static DECLCALLBACK(void) lsilogicR3PowerOff(PPDMDEVINS pDevIns)
{
    Log(("lsilogicR3PowerOff\n"));
    lsilogicR3SuspendOrPowerOff(pDevIns);
}

/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) lsilogicR3Destruct(PPDMDEVINS pDevIns)
{
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);

    PDMR3CritSectDelete(&pThis->ReplyFreeQueueCritSect);
    PDMR3CritSectDelete(&pThis->ReplyPostQueueCritSect);
    PDMR3CritSectDelete(&pThis->RequestQueueCritSect);
    PDMR3CritSectDelete(&pThis->ReplyFreeQueueWriteCritSect);

    RTMemFree(pThis->paDeviceStates);
    pThis->paDeviceStates = NULL;

    if (pThis->hEvtProcess != NIL_SUPSEMEVENT)
    {
        SUPSemEventClose(pThis->pSupDrvSession, pThis->hEvtProcess);
        pThis->hEvtProcess = NIL_SUPSEMEVENT;
    }

    lsilogicR3ConfigurationPagesFree(pThis);
    lsilogicR3MemRegionsFree(pThis);

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) lsilogicR3Construct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    PLSILOGICSCSI pThis = PDMINS_2_DATA(pDevIns, PLSILOGICSCSI);
    int           rc    = VINF_SUCCESS;
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Initialize enought of the state to make the destructure not trip up.
     */
    pThis->hEvtProcess = NIL_SUPSEMEVENT;
    pThis->fBiosReqPending = false;
    RTListInit(&pThis->ListMemRegns);

    /*
     * Validate and read configuration.
     */
    rc = CFGMR3AreValuesValid(pCfg, "GCEnabled\0"
                                    "R0Enabled\0"
                                    "ReplyQueueDepth\0"
                                    "RequestQueueDepth\0"
                                    "ControllerType\0"
                                    "NumPorts\0"
                                    "Bootable\0");
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                N_("LsiLogic configuration error: unknown option specified"));
    rc = CFGMR3QueryBoolDef(pCfg, "GCEnabled", &pThis->fGCEnabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("LsiLogic configuration error: failed to read GCEnabled as boolean"));
    Log(("%s: fGCEnabled=%d\n", __FUNCTION__, pThis->fGCEnabled));

    rc = CFGMR3QueryBoolDef(pCfg, "R0Enabled", &pThis->fR0Enabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("LsiLogic configuration error: failed to read R0Enabled as boolean"));
    Log(("%s: fR0Enabled=%d\n", __FUNCTION__, pThis->fR0Enabled));

    rc = CFGMR3QueryU32Def(pCfg, "ReplyQueueDepth",
                           &pThis->cReplyQueueEntries,
                           LSILOGICSCSI_REPLY_QUEUE_DEPTH_DEFAULT);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("LsiLogic configuration error: failed to read ReplyQueue as integer"));
    Log(("%s: ReplyQueueDepth=%u\n", __FUNCTION__, pThis->cReplyQueueEntries));

    rc = CFGMR3QueryU32Def(pCfg, "RequestQueueDepth",
                           &pThis->cRequestQueueEntries,
                           LSILOGICSCSI_REQUEST_QUEUE_DEPTH_DEFAULT);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("LsiLogic configuration error: failed to read RequestQueue as integer"));
    Log(("%s: RequestQueueDepth=%u\n", __FUNCTION__, pThis->cRequestQueueEntries));

    char *pszCtrlType;
    rc = CFGMR3QueryStringAllocDef(pCfg, "ControllerType",
                                   &pszCtrlType, LSILOGICSCSI_PCI_SPI_CTRLNAME);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("LsiLogic configuration error: failed to read ControllerType as string"));
    Log(("%s: ControllerType=%s\n", __FUNCTION__, pszCtrlType));

    rc = lsilogicR3GetCtrlTypeFromString(pThis, pszCtrlType);
    MMR3HeapFree(pszCtrlType);

    char szDevTag[20];
    RTStrPrintf(szDevTag, sizeof(szDevTag), "LSILOGIC%s-%u",
                pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI ? "SPI" : "SAS",
                iInstance);


    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("LsiLogic configuration error: failed to determine controller type from string"));

    rc = CFGMR3QueryU8(pCfg, "NumPorts",
                       &pThis->cPorts);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
    {
        if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI)
            pThis->cPorts = LSILOGICSCSI_PCI_SPI_PORTS_MAX;
        else if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
            pThis->cPorts = LSILOGICSCSI_PCI_SAS_PORTS_DEFAULT;
        else
            AssertMsgFailed(("Invalid controller type: %d\n", pThis->enmCtrlType));
    }
    else if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("LsiLogic configuration error: failed to read NumPorts as integer"));

    bool fBootable;
    rc = CFGMR3QueryBoolDef(pCfg, "Bootable", &fBootable, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("LsiLogic configuration error: failed to read Bootable as boolean"));
    Log(("%s: Bootable=%RTbool\n", __FUNCTION__, fBootable));

    /* Init static parts. */
    PCIDevSetVendorId(&pThis->PciDev, LSILOGICSCSI_PCI_VENDOR_ID); /* LsiLogic */

    if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI)
    {
        PCIDevSetDeviceId         (&pThis->PciDev, LSILOGICSCSI_PCI_SPI_DEVICE_ID); /* LSI53C1030 */
        PCIDevSetSubSystemVendorId(&pThis->PciDev, LSILOGICSCSI_PCI_SPI_SUBSYSTEM_VENDOR_ID);
        PCIDevSetSubSystemId      (&pThis->PciDev, LSILOGICSCSI_PCI_SPI_SUBSYSTEM_ID);
    }
    else if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
    {
        PCIDevSetDeviceId         (&pThis->PciDev, LSILOGICSCSI_PCI_SAS_DEVICE_ID); /* SAS1068 */
        PCIDevSetSubSystemVendorId(&pThis->PciDev, LSILOGICSCSI_PCI_SAS_SUBSYSTEM_VENDOR_ID);
        PCIDevSetSubSystemId      (&pThis->PciDev, LSILOGICSCSI_PCI_SAS_SUBSYSTEM_ID);
    }
    else
        AssertMsgFailed(("Invalid controller type: %d\n", pThis->enmCtrlType));

    PCIDevSetClassProg   (&pThis->PciDev,   0x00); /* SCSI */
    PCIDevSetClassSub    (&pThis->PciDev,   0x00); /* SCSI */
    PCIDevSetClassBase   (&pThis->PciDev,   0x01); /* Mass storage */
    PCIDevSetInterruptPin(&pThis->PciDev,   0x01); /* Interrupt pin A */

# ifdef VBOX_WITH_MSI_DEVICES
    PCIDevSetStatus(&pThis->PciDev,   VBOX_PCI_STATUS_CAP_LIST);
    PCIDevSetCapabilityList(&pThis->PciDev, 0x80);
# endif

    pThis->pDevInsR3 = pDevIns;
    pThis->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    pThis->pSupDrvSession = PDMDevHlpGetSupDrvSession(pDevIns);
    pThis->IBase.pfnQueryInterface = lsilogicR3StatusQueryInterface;
    pThis->ILeds.pfnQueryStatusLed = lsilogicR3StatusQueryStatusLed;

    /*
     * Create critical sections protecting the reply post and free queues.
     * Note! We do our own syncronization, so NOP the default crit sect for the device.
     */
    rc = PDMDevHlpSetDeviceCritSect(pDevIns, PDMDevHlpCritSectGetNop(pDevIns));
    AssertRCReturn(rc, rc);

    rc = PDMDevHlpCritSectInit(pDevIns, &pThis->ReplyFreeQueueCritSect, RT_SRC_POS, "%sRFQ", szDevTag);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("LsiLogic: cannot create critical section for reply free queue"));

    rc = PDMDevHlpCritSectInit(pDevIns, &pThis->ReplyPostQueueCritSect, RT_SRC_POS, "%sRPQ", szDevTag);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("LsiLogic: cannot create critical section for reply post queue"));

    rc = PDMDevHlpCritSectInit(pDevIns, &pThis->RequestQueueCritSect, RT_SRC_POS, "%sRQ", szDevTag);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("LsiLogic: cannot create critical section for request queue"));

    rc = PDMDevHlpCritSectInit(pDevIns, &pThis->ReplyFreeQueueWriteCritSect, RT_SRC_POS, "%sRFQW", szDevTag);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("LsiLogic: cannot create critical section for reply free queue write access"));

    /*
     * Register the PCI device, it's I/O regions.
     */
    rc = PDMDevHlpPCIRegister(pDevIns, &pThis->PciDev);
    if (RT_FAILURE(rc))
        return rc;

# ifdef VBOX_WITH_MSI_DEVICES
    PDMMSIREG MsiReg;
    RT_ZERO(MsiReg);
    /* use this code for MSI-X support */
#  if 0
    MsiReg.cMsixVectors    = 1;
    MsiReg.iMsixCapOffset  = 0x80;
    MsiReg.iMsixNextOffset = 0x00;
    MsiReg.iMsixBar        = 3;
#  else
    MsiReg.cMsiVectors     = 1;
    MsiReg.iMsiCapOffset   = 0x80;
    MsiReg.iMsiNextOffset  = 0x00;
#  endif
    rc = PDMDevHlpPCIRegisterMsi(pDevIns, &MsiReg);
    if (RT_FAILURE (rc))
    {
        /* That's OK, we can work without MSI */
        PCIDevSetCapabilityList(&pThis->PciDev, 0x0);
    }
# endif

    rc = PDMDevHlpPCIIORegionRegister(pDevIns, 0, LSILOGIC_PCI_SPACE_IO_SIZE, PCI_ADDRESS_SPACE_IO, lsilogicR3Map);
    if (RT_FAILURE(rc))
        return rc;

    rc = PDMDevHlpPCIIORegionRegister(pDevIns, 1, LSILOGIC_PCI_SPACE_MEM_SIZE, PCI_ADDRESS_SPACE_MEM, lsilogicR3Map);
    if (RT_FAILURE(rc))
        return rc;

    rc = PDMDevHlpPCIIORegionRegister(pDevIns, 2, LSILOGIC_PCI_SPACE_MEM_SIZE, PCI_ADDRESS_SPACE_MEM, lsilogicR3Map);
    if (RT_FAILURE(rc))
        return rc;

    /* Initialize task queue. (Need two items to handle SMP guest concurrency.) */
    char szTaggedText[64];
    RTStrPrintf(szTaggedText, sizeof(szTaggedText), "%s-Task", szDevTag);
    rc = PDMDevHlpQueueCreate(pDevIns, sizeof(PDMQUEUEITEMCORE), 2, 0,
                              lsilogicR3NotifyQueueConsumer, true,
                              szTaggedText,
                              &pThis->pNotificationQueueR3);
    if (RT_FAILURE(rc))
        return rc;
    pThis->pNotificationQueueR0 = PDMQueueR0Ptr(pThis->pNotificationQueueR3);
    pThis->pNotificationQueueRC = PDMQueueRCPtr(pThis->pNotificationQueueR3);

    /*
     * We need one entry free in the queue.
     */
    pThis->cReplyQueueEntries++;
    pThis->cRequestQueueEntries++;

    /*
     * Allocate memory for the queues.
     */
    rc = lsilogicR3QueuesAlloc(pThis);
    if (RT_FAILURE(rc))
        return rc;

    if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI)
        pThis->cDeviceStates = pThis->cPorts * LSILOGICSCSI_PCI_SPI_DEVICES_PER_BUS_MAX;
    else if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
        pThis->cDeviceStates = pThis->cPorts * LSILOGICSCSI_PCI_SAS_DEVICES_PER_PORT_MAX;
    else
        AssertMsgFailed(("Invalid controller type: %d\n", pThis->enmCtrlType));

    /*
     * Create event semaphore and worker thread.
     */
    rc = PDMDevHlpThreadCreate(pDevIns, &pThis->pThreadWrk, pThis, lsilogicR3Worker,
                               lsilogicR3WorkerWakeUp, 0, RTTHREADTYPE_IO, szDevTag);
    if (RT_FAILURE(rc))
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("LsiLogic: Failed to create worker thread %s"), szDevTag);

    rc = SUPSemEventCreate(pThis->pSupDrvSession, &pThis->hEvtProcess);
    if (RT_FAILURE(rc))
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("LsiLogic: Failed to create SUP event semaphore"));

    /*
     * Allocate device states.
     */
    pThis->paDeviceStates = (PLSILOGICDEVICE)RTMemAllocZ(sizeof(LSILOGICDEVICE) * pThis->cDeviceStates);
    if (!pThis->paDeviceStates)
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Failed to allocate memory for device states"));

    for (unsigned i = 0; i < pThis->cDeviceStates; i++)
    {
        PLSILOGICDEVICE pDevice = &pThis->paDeviceStates[i];

        /* Initialize static parts of the device. */
        pDevice->iLUN                                    = i;
        pDevice->pLsiLogicR3                             = pThis;
        pDevice->Led.u32Magic                            = PDMLED_MAGIC;
        pDevice->IBase.pfnQueryInterface                 = lsilogicR3DeviceQueryInterface;
        pDevice->IMediaPort.pfnQueryDeviceLocation       = lsilogicR3QueryDeviceLocation;
        pDevice->IMediaExPort.pfnIoReqCompleteNotify     = lsilogicR3IoReqCompleteNotify;
        pDevice->IMediaExPort.pfnIoReqCopyFromBuf        = lsilogicR3IoReqCopyFromBuf;
        pDevice->IMediaExPort.pfnIoReqCopyToBuf          = lsilogicR3IoReqCopyToBuf;
        pDevice->IMediaExPort.pfnIoReqQueryBuf           = NULL;
        pDevice->IMediaExPort.pfnIoReqQueryDiscardRanges = NULL;
        pDevice->IMediaExPort.pfnIoReqStateChanged       = lsilogicR3IoReqStateChanged;
        pDevice->IMediaExPort.pfnMediumEjected           = lsilogicR3MediumEjected;
        pDevice->ILed.pfnQueryStatusLed                  = lsilogicR3DeviceQueryStatusLed;

        char *pszName;
        if (RTStrAPrintf(&pszName, "Device%u", i) <= 0)
            AssertLogRelFailedReturn(VERR_NO_MEMORY);

        /* Attach SCSI driver. */
        rc = PDMDevHlpDriverAttach(pDevIns, pDevice->iLUN, &pDevice->IBase, &pDevice->pDrvBase, pszName);
        if (RT_SUCCESS(rc))
        {
            /* Query the media interface. */
            pDevice->pDrvMedia = PDMIBASE_QUERY_INTERFACE(pDevice->pDrvBase, PDMIMEDIA);
            AssertMsgReturn(VALID_PTR(pDevice->pDrvMedia),
                            ("LsiLogic configuration error: LUN#%d misses the basic media interface!\n", pDevice->iLUN),
                            VERR_PDM_MISSING_INTERFACE);

            /* Get the extended media interface. */
            pDevice->pDrvMediaEx = PDMIBASE_QUERY_INTERFACE(pDevice->pDrvBase, PDMIMEDIAEX);
            AssertMsgReturn(VALID_PTR(pDevice->pDrvMediaEx),
                            ("LsiLogic configuration error: LUN#%d misses the extended media interface!\n", pDevice->iLUN),
                            VERR_PDM_MISSING_INTERFACE);

            rc = pDevice->pDrvMediaEx->pfnIoReqAllocSizeSet(pDevice->pDrvMediaEx, sizeof(LSILOGICREQ));
            if (RT_FAILURE(rc))
                return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                           N_("LsiLogic configuration error: LUN#%u: Failed to set I/O request size!"),
                                           pDevice->iLUN);
        }
        else if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
        {
            pDevice->pDrvBase = NULL;
            rc = VINF_SUCCESS;
            Log(("LsiLogic: no driver attached to device %s\n", pszName));
        }
        else
        {
            AssertLogRelMsgFailed(("LsiLogic: Failed to attach %s\n", pszName));
            return rc;
        }
    }

    /*
     * Attach status driver (optional).
     */
    PPDMIBASE pBase;
    rc = PDMDevHlpDriverAttach(pDevIns, PDM_STATUS_LUN, &pThis->IBase, &pBase, "Status Port");
    if (RT_SUCCESS(rc))
    {
        pThis->pLedsConnector = PDMIBASE_QUERY_INTERFACE(pBase, PDMILEDCONNECTORS);
        pThis->pMediaNotify = PDMIBASE_QUERY_INTERFACE(pBase, PDMIMEDIANOTIFY);
    }
    else if (rc != VERR_PDM_NO_ATTACHED_DRIVER)
    {
        AssertMsgFailed(("Failed to attach to status driver. rc=%Rrc\n", rc));
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("LsiLogic cannot attach to status driver"));
    }

    /* Initialize the SCSI emulation for the BIOS. */
    rc = vboxscsiInitialize(&pThis->VBoxSCSI);
    AssertRC(rc);

    /*
     * Register I/O port space in ISA region for BIOS access
     * if the controller is marked as bootable.
     */
    if (fBootable)
    {
        if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI)
            rc = PDMDevHlpIOPortRegister(pDevIns, LSILOGIC_BIOS_IO_PORT, 4, NULL,
                                         lsilogicR3IsaIOPortWrite, lsilogicR3IsaIOPortRead,
                                         lsilogicR3IsaIOPortWriteStr, lsilogicR3IsaIOPortReadStr,
                                         "LsiLogic BIOS");
        else if (pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SAS)
            rc = PDMDevHlpIOPortRegister(pDevIns, LSILOGIC_SAS_BIOS_IO_PORT, 4, NULL,
                                         lsilogicR3IsaIOPortWrite, lsilogicR3IsaIOPortRead,
                                         lsilogicR3IsaIOPortWriteStr, lsilogicR3IsaIOPortReadStr,
                                         "LsiLogic SAS BIOS");
        else
            AssertMsgFailed(("Invalid controller type %d\n", pThis->enmCtrlType));

        if (RT_FAILURE(rc))
            return PDMDEV_SET_ERROR(pDevIns, rc, N_("LsiLogic cannot register legacy I/O handlers"));
    }

    /* Register save state handlers. */
    rc = PDMDevHlpSSMRegisterEx(pDevIns, LSILOGIC_SAVED_STATE_VERSION, sizeof(*pThis), NULL,
                                NULL, lsilogicR3LiveExec, NULL,
                                NULL, lsilogicR3SaveExec, NULL,
                                NULL, lsilogicR3LoadExec, lsilogicR3LoadDone);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("LsiLogic cannot register save state handlers"));

    pThis->enmWhoInit = LSILOGICWHOINIT_SYSTEM_BIOS;

    /*
     * Register the info item.
     */
    char szTmp[128];
    RTStrPrintf(szTmp, sizeof(szTmp), "%s%u", pDevIns->pReg->szName, pDevIns->iInstance);
    PDMDevHlpDBGFInfoRegister(pDevIns, szTmp,
                              pThis->enmCtrlType == LSILOGICCTRLTYPE_SCSI_SPI
                              ? "LsiLogic SPI info."
                              : "LsiLogic SAS info.", lsilogicR3Info);

    /* Perform hard reset. */
    rc = lsilogicR3HardReset(pThis);
    AssertRC(rc);

    return rc;
}

/**
 * The device registration structure - SPI SCSI controller.
 */
const PDMDEVREG g_DeviceLsiLogicSCSI =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "lsilogicscsi",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "LSI Logic 53c1030 SCSI controller.\n",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0 |
    PDM_DEVREG_FLAGS_FIRST_SUSPEND_NOTIFICATION | PDM_DEVREG_FLAGS_FIRST_POWEROFF_NOTIFICATION,
    /* fClass */
    PDM_DEVREG_CLASS_STORAGE,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(LSILOGICSCSI),
    /* pfnConstruct */
    lsilogicR3Construct,
    /* pfnDestruct */
    lsilogicR3Destruct,
    /* pfnRelocate */
    lsilogicR3Relocate,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    lsilogicR3Reset,
    /* pfnSuspend */
    lsilogicR3Suspend,
    /* pfnResume */
    lsilogicR3Resume,
    /* pfnAttach */
    lsilogicR3Attach,
    /* pfnDetach */
    lsilogicR3Detach,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    lsilogicR3PowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};

/**
 * The device registration structure - SAS controller.
 */
const PDMDEVREG g_DeviceLsiLogicSAS =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "lsilogicsas",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "LSI Logic SAS1068 controller.\n",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0 |
    PDM_DEVREG_FLAGS_FIRST_SUSPEND_NOTIFICATION | PDM_DEVREG_FLAGS_FIRST_POWEROFF_NOTIFICATION |
    PDM_DEVREG_FLAGS_FIRST_RESET_NOTIFICATION,
    /* fClass */
    PDM_DEVREG_CLASS_STORAGE,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(LSILOGICSCSI),
    /* pfnConstruct */
    lsilogicR3Construct,
    /* pfnDestruct */
    lsilogicR3Destruct,
    /* pfnRelocate */
    lsilogicR3Relocate,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    lsilogicR3Reset,
    /* pfnSuspend */
    lsilogicR3Suspend,
    /* pfnResume */
    lsilogicR3Resume,
    /* pfnAttach */
    lsilogicR3Attach,
    /* pfnDetach */
    lsilogicR3Detach,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    lsilogicR3PowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};

#endif /* IN_RING3 */
#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */
