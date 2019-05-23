/* $Id: DevATA.cpp $ */
/** @file
 * VBox storage devices: ATA/ATAPI controller device (disk and cdrom).
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
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Temporary instrumentation for tracking down potential virtual disk
 * write performance issues. */
#undef VBOX_INSTRUMENT_DMA_WRITES

/** @name The SSM saved state versions.
 * @{
 */
/** The current saved state version. */
#define ATA_SAVED_STATE_VERSION                         20
/** The saved state version used by VirtualBox 3.0.
 * This lacks the config part and has the type at the and.  */
#define ATA_SAVED_STATE_VERSION_VBOX_30                 19
#define ATA_SAVED_STATE_VERSION_WITH_BOOL_TYPE          18
#define ATA_SAVED_STATE_VERSION_WITHOUT_FULL_SENSE      16
#define ATA_SAVED_STATE_VERSION_WITHOUT_EVENT_STATUS    17
/** @} */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_IDE
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmstorageifs.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#ifdef IN_RING3
# include <iprt/uuid.h>
# include <iprt/semaphore.h>
# include <iprt/thread.h>
# include <iprt/time.h>
# include <iprt/alloc.h>
#endif /* IN_RING3 */
#include <iprt/critsect.h>
#include <iprt/asm.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pgm.h>

#include <VBox/sup.h>
#include <VBox/scsi.h>
#include <VBox/scsiinline.h>
#include <VBox/ata.h>

#include "ATAPIPassthrough.h"
#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/**
 * Maximum number of sectors to transfer in a READ/WRITE MULTIPLE request.
 * Set to 1 to disable multi-sector read support. According to the ATA
 * specification this must be a power of 2 and it must fit in an 8 bit
 * value. Thus the only valid values are 1, 2, 4, 8, 16, 32, 64 and 128.
 */
#define ATA_MAX_MULT_SECTORS 128

/**
 * Fastest PIO mode supported by the drive.
 */
#define ATA_PIO_MODE_MAX 4
/**
 * Fastest MDMA mode supported by the drive.
 */
#define ATA_MDMA_MODE_MAX 2
/**
 * Fastest UDMA mode supported by the drive.
 */
#define ATA_UDMA_MODE_MAX 6

/** ATAPI sense info size. */
#define ATAPI_SENSE_SIZE 64

/** The maximum number of release log entries per device. */
#define MAX_LOG_REL_ERRORS  1024

/* MediaEventStatus */
#define ATA_EVENT_STATUS_UNCHANGED              0    /**< medium event status not changed */
#define ATA_EVENT_STATUS_MEDIA_NEW              1    /**< new medium inserted */
#define ATA_EVENT_STATUS_MEDIA_REMOVED          2    /**< medium removed */
#define ATA_EVENT_STATUS_MEDIA_CHANGED          3    /**< medium was removed + new medium was inserted */
#define ATA_EVENT_STATUS_MEDIA_EJECT_REQUESTED  4    /**< medium eject requested (eject button pressed) */

/* Media track type */
#define ATA_MEDIA_TYPE_UNKNOWN                  0    /**< unknown CD type */
#define ATA_MEDIA_NO_DISC                    0x70    /**< Door closed, no medium */

/** @defgroup grp_piix3atabmdma     PIIX3 ATA Bus Master DMA
 * @{
 */

/** @name BM_STATUS
 * @{
 */
/** Currently performing a DMA operation. */
#define BM_STATUS_DMAING 0x01
/** An error occurred during the DMA operation. */
#define BM_STATUS_ERROR  0x02
/** The DMA unit has raised the IDE interrupt line. */
#define BM_STATUS_INT    0x04
/** User-defined bit 0, commonly used to signal that drive 0 supports DMA. */
#define BM_STATUS_D0DMA  0x20
/** User-defined bit 1, commonly used to signal that drive 1 supports DMA. */
#define BM_STATUS_D1DMA  0x40
/** @} */

/** @name BM_CMD
 * @{
 */
/** Start the DMA operation. */
#define BM_CMD_START     0x01
/** Data transfer direction: from device to memory if set. */
#define BM_CMD_WRITE     0x08
/** @} */

/** @} */


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** @defgroup grp_piix3atabmdma     PIIX3 ATA Bus Master DMA
 * @{
 */
/** PIIX3 Bus Master DMA unit state. */
typedef struct BMDMAState
{
    /** Command register. */
    uint8_t    u8Cmd;
    /** Status register. */
    uint8_t    u8Status;
    /** Address of the MMIO region in the guest's memory space. */
    RTGCPHYS32 GCPhysAddr;
} BMDMAState;

/** PIIX3 Bus Master DMA descriptor entry. */
typedef struct BMDMADesc
{
    /** Address of the DMA source/target buffer. */
    RTGCPHYS32 GCPhysBuffer;
    /** Size of the DMA source/target buffer. */
    uint32_t   cbBuffer;
} BMDMADesc;
/** @} */


/**
 * The state of an ATA device.
 *
 * @implements PDMIBASE
 * @implements PDMIBLOCKPORT
 * @implements PDMIMOUNTNOTIFY
 */
typedef struct ATADevState
{
    /** Flag indicating whether the current command uses LBA48 mode. */
    bool                                fLBA48;
    /** Flag indicating whether this drive implements the ATAPI command set. */
    bool                                fATAPI;
    /** Set if this interface has asserted the IRQ. */
    bool                                fIrqPending;
    /** Currently configured number of sectors in a multi-sector transfer. */
    uint8_t                             cMultSectors;
    /** PCHS disk geometry. */
    PDMMEDIAGEOMETRY                    PCHSGeometry;
    /** Total number of sectors on this disk. */
    uint64_t                            cTotalSectors;
    /** Sector size of the medium. */
    uint32_t                            cbSector;
    /** Number of sectors to transfer per IRQ. */
    uint32_t                            cSectorsPerIRQ;

    /** ATA/ATAPI register 1: feature (write-only). */
    uint8_t                             uATARegFeature;
    /** ATA/ATAPI register 1: feature, high order byte. */
    uint8_t                             uATARegFeatureHOB;
    /** ATA/ATAPI register 1: error (read-only). */
    uint8_t                             uATARegError;
    /** ATA/ATAPI register 2: sector count (read/write). */
    uint8_t                             uATARegNSector;
    /** ATA/ATAPI register 2: sector count, high order byte. */
    uint8_t                             uATARegNSectorHOB;
    /** ATA/ATAPI register 3: sector (read/write). */
    uint8_t                             uATARegSector;
    /** ATA/ATAPI register 3: sector, high order byte. */
    uint8_t                             uATARegSectorHOB;
    /** ATA/ATAPI register 4: cylinder low (read/write). */
    uint8_t                             uATARegLCyl;
    /** ATA/ATAPI register 4: cylinder low, high order byte. */
    uint8_t                             uATARegLCylHOB;
    /** ATA/ATAPI register 5: cylinder high (read/write). */
    uint8_t                             uATARegHCyl;
    /** ATA/ATAPI register 5: cylinder high, high order byte. */
    uint8_t                             uATARegHCylHOB;
    /** ATA/ATAPI register 6: select drive/head (read/write). */
    uint8_t                             uATARegSelect;
    /** ATA/ATAPI register 7: status (read-only). */
    uint8_t                             uATARegStatus;
    /** ATA/ATAPI register 7: command (write-only). */
    uint8_t                             uATARegCommand;
    /** ATA/ATAPI drive control register (write-only). */
    uint8_t                             uATARegDevCtl;

    /** Currently active transfer mode (MDMA/UDMA) and speed. */
    uint8_t                             uATATransferMode;
    /** Current transfer direction. */
    uint8_t                             uTxDir;
    /** Index of callback for begin transfer. */
    uint8_t                             iBeginTransfer;
    /** Index of callback for source/sink of data. */
    uint8_t                             iSourceSink;
    /** Flag indicating whether the current command transfers data in DMA mode. */
    bool                                fDMA;
    /** Set to indicate that ATAPI transfer semantics must be used. */
    bool                                fATAPITransfer;

    /** Total ATA/ATAPI transfer size, shared PIO/DMA. */
    uint32_t                            cbTotalTransfer;
    /** Elementary ATA/ATAPI transfer size, shared PIO/DMA. */
    uint32_t                            cbElementaryTransfer;
    /** Maximum ATAPI elementary transfer size, PIO only. */
    uint32_t                            cbPIOTransferLimit;
    /** ATAPI passthrough transfer size, shared PIO/DMA */
    uint32_t                            cbAtapiPassthroughTransfer;
    /** Current read/write buffer position, shared PIO/DMA. */
    uint32_t                            iIOBufferCur;
    /** First element beyond end of valid buffer content, shared PIO/DMA. */
    uint32_t                            iIOBufferEnd;
    /** Align the following fields correctly. */
    uint32_t                            Alignment0;

    /** ATA/ATAPI current PIO read/write transfer position. Not shared with DMA for safety reasons. */
    uint32_t                            iIOBufferPIODataStart;
    /** ATA/ATAPI current PIO read/write transfer end. Not shared with DMA for safety reasons. */
    uint32_t                            iIOBufferPIODataEnd;

    /** ATAPI current LBA position. */
    uint32_t                            iATAPILBA;
    /** ATAPI current sector size. */
    uint32_t                            cbATAPISector;
    /** ATAPI current command. */
    uint8_t                             aATAPICmd[ATAPI_PACKET_SIZE];
    /** ATAPI sense data. */
    uint8_t                             abATAPISense[ATAPI_SENSE_SIZE];
    /** HACK: Countdown till we report a newly unmounted drive as mounted. */
    uint8_t                             cNotifiedMediaChange;
    /** The same for GET_EVENT_STATUS for mechanism */
    volatile uint32_t                   MediaEventStatus;

    /** Media type if known. */
    volatile uint32_t                   MediaTrackType;

    /** The status LED state for this drive. */
    PDMLED                              Led;

    /** Size of I/O buffer. */
    uint32_t                            cbIOBuffer;
    /** Pointer to the I/O buffer. */
    R3PTRTYPE(uint8_t *)                pbIOBufferR3;
    /** Pointer to the I/O buffer. */
    R0PTRTYPE(uint8_t *)                pbIOBufferR0;
    /** Pointer to the I/O buffer. */
    RCPTRTYPE(uint8_t *)                pbIOBufferRC;

    RTRCPTR                             Aligmnent1; /**< Align the statistics at an 8-byte boundary. */

    /*
     * No data that is part of the saved state after this point!!!!!
     */

    /* Release statistics: number of ATA DMA commands. */
    STAMCOUNTER                         StatATADMA;
    /* Release statistics: number of ATA PIO commands. */
    STAMCOUNTER                         StatATAPIO;
    /* Release statistics: number of ATAPI PIO commands. */
    STAMCOUNTER                         StatATAPIDMA;
    /* Release statistics: number of ATAPI PIO commands. */
    STAMCOUNTER                         StatATAPIPIO;
#ifdef VBOX_INSTRUMENT_DMA_WRITES
    /* Release statistics: number of DMA sector writes and the time spent. */
    STAMPROFILEADV                      StatInstrVDWrites;
#endif

    /** Statistics: number of read operations and the time spent reading. */
    STAMPROFILEADV                      StatReads;
    /** Statistics: number of bytes read. */
    STAMCOUNTER                         StatBytesRead;
    /** Statistics: number of write operations and the time spent writing. */
    STAMPROFILEADV                      StatWrites;
    /** Statistics: number of bytes written. */
    STAMCOUNTER                         StatBytesWritten;
    /** Statistics: number of flush operations and the time spend flushing. */
    STAMPROFILE                         StatFlushes;

    /** Enable passing through commands directly to the ATAPI drive. */
    bool                                fATAPIPassthrough;
    /** Flag whether to overwrite inquiry data in passthrough mode. */
    bool                                fOverwriteInquiry;
    /** Number of errors we've reported to the release log.
     * This is to prevent flooding caused by something going horribly wrong.
     * this value against MAX_LOG_REL_ERRORS in places likely to cause floods
     * like the ones we currently seeing on the linux smoke tests (2006-11-10). */
    uint32_t                            cErrors;
    /** Timestamp of last started command. 0 if no command pending. */
    uint64_t                            u64CmdTS;

    /** Pointer to the attached driver's base interface. */
    R3PTRTYPE(PPDMIBASE)                pDrvBase;
    /** Pointer to the attached driver's block interface. */
    R3PTRTYPE(PPDMIMEDIA)               pDrvMedia;
    /** Pointer to the attached driver's mount interface.
     * This is NULL if the driver isn't a removable unit. */
    R3PTRTYPE(PPDMIMOUNT)               pDrvMount;
    /** The base interface. */
    PDMIBASE                            IBase;
    /** The block port interface. */
    PDMIMEDIAPORT                       IPort;
    /** The mount notify interface. */
    PDMIMOUNTNOTIFY                     IMountNotify;
    /** The LUN #. */
    RTUINT                              iLUN;

    RTUINT                              Alignment2; /**< Align pDevInsR3 correctly. */

    /** Pointer to device instance. */
    PPDMDEVINSR3                        pDevInsR3;
    /** Pointer to controller instance. */
    R3PTRTYPE(struct ATACONTROLLER *)   pControllerR3;
    /** Pointer to device instance. */
    PPDMDEVINSR0                        pDevInsR0;
    /** Pointer to controller instance. */
    R0PTRTYPE(struct ATACONTROLLER *)   pControllerR0;
    /** Pointer to device instance. */
    PPDMDEVINSRC                        pDevInsRC;
    /** Pointer to controller instance. */
    RCPTRTYPE(struct ATACONTROLLER *)   pControllerRC;

    /** The serial number to use for IDENTIFY DEVICE commands. */
    char                                szSerialNumber[ATA_SERIAL_NUMBER_LENGTH+1];
    /** The firmware revision to use for IDENTIFY DEVICE commands. */
    char                                szFirmwareRevision[ATA_FIRMWARE_REVISION_LENGTH+1];
    /** The model number to use for IDENTIFY DEVICE commands. */
    char                                szModelNumber[ATA_MODEL_NUMBER_LENGTH+1];
    /** The vendor identification string for SCSI INQUIRY commands. */
    char                                szInquiryVendorId[SCSI_INQUIRY_VENDOR_ID_LENGTH+1];
    /** The product identification string for SCSI INQUIRY commands. */
    char                                szInquiryProductId[SCSI_INQUIRY_PRODUCT_ID_LENGTH+1];
    /** The revision string for SCSI INQUIRY commands. */
    char                                szInquiryRevision[SCSI_INQUIRY_REVISION_LENGTH+1];
    /** The current tracklist of the loaded medium if passthrough is used. */
    R3PTRTYPE(PTRACKLIST)               pTrackList;

    uint8_t                             abAlignment4[HC_ARCH_BITS == 64 ? 7 : 3];
} ATADevState;
AssertCompileMemberAlignment(ATADevState, cTotalSectors, 8);
AssertCompileMemberAlignment(ATADevState, StatATADMA, 8);
AssertCompileMemberAlignment(ATADevState, u64CmdTS, 8);
AssertCompileMemberAlignment(ATADevState, pDevInsR3, 8);
AssertCompileMemberAlignment(ATADevState, szSerialNumber, 8);
AssertCompileSizeAlignment(ATADevState, 8);


/**
 * Transfer request forwarded to the async I/O thread.
 */
typedef struct ATATransferRequest
{
    /** The interface index the request is for. */
    uint8_t  iIf;
    /** The index of the begin transfer callback to call. */
    uint8_t  iBeginTransfer;
    /** The index of the source sink callback to call for doing the transfer. */
    uint8_t  iSourceSink;
    /** How many bytes to transfer. */
    uint32_t cbTotalTransfer;
    /** Transfer direction. */
    uint8_t  uTxDir;
} ATATransferRequest;


/**
 * Abort request forwarded to the async I/O thread.
 */
typedef struct ATAAbortRequest
{
    /** The interface index the request is for. */
    uint8_t iIf;
    /** Flag whether to reset the drive. */
    bool    fResetDrive;
} ATAAbortRequest;


/**
 * Request type indicator.
 */
typedef enum
{
    /** Begin a new transfer. */
    ATA_AIO_NEW = 0,
    /** Continue a DMA transfer. */
    ATA_AIO_DMA,
    /** Continue a PIO transfer. */
    ATA_AIO_PIO,
    /** Reset the drives on current controller, stop all transfer activity. */
    ATA_AIO_RESET_ASSERTED,
    /** Reset the drives on current controller, resume operation. */
    ATA_AIO_RESET_CLEARED,
    /** Abort the current transfer of a particular drive. */
    ATA_AIO_ABORT
} ATAAIO;


/**
 * Combining structure for an ATA request to the async I/O thread
 * started with the request type insicator.
 */
typedef struct ATARequest
{
    /** Request type. */
    ATAAIO                 ReqType;
    /** Request type dependent data. */
    union
    {
        /** Transfer request specific data. */
        ATATransferRequest t;
        /** Abort request specific data. */
        ATAAbortRequest    a;
    } u;
} ATARequest;


/**
 * The state of an ATA controller containing to devices (master and slave).
 */
typedef struct ATACONTROLLER
{
    /** The base of the first I/O Port range. */
    RTIOPORT            IOPortBase1;
    /** The base of the second I/O Port range. (0 if none) */
    RTIOPORT            IOPortBase2;
    /** The assigned IRQ. */
    RTUINT              irq;
    /** Access critical section */
    PDMCRITSECT         lock;

    /** Selected drive. */
    uint8_t             iSelectedIf;
    /** The interface on which to handle async I/O. */
    uint8_t             iAIOIf;
    /** The state of the async I/O thread. */
    uint8_t             uAsyncIOState;
    /** Flag indicating whether the next transfer is part of the current command. */
    bool                fChainedTransfer;
    /** Set when the reset processing is currently active on this controller. */
    bool                fReset;
    /** Flag whether the current transfer needs to be redone. */
    bool                fRedo;
    /** Flag whether the redo suspend has been finished. */
    bool                fRedoIdle;
    /** Flag whether the DMA operation to be redone is the final transfer. */
    bool                fRedoDMALastDesc;
    /** The BusMaster DMA state. */
    BMDMAState          BmDma;
    /** Pointer to first DMA descriptor. */
    RTGCPHYS32          GCPhysFirstDMADesc;
    /** Pointer to last DMA descriptor. */
    RTGCPHYS32          GCPhysLastDMADesc;
    /** Pointer to current DMA buffer (for redo operations). */
    RTGCPHYS32          GCPhysRedoDMABuffer;
    /** Size of current DMA buffer (for redo operations). */
    uint32_t            cbRedoDMABuffer;

    /** The ATA/ATAPI interfaces of this controller. */
    ATADevState         aIfs[2];

    /** Pointer to device instance. */
    PPDMDEVINSR3        pDevInsR3;
    /** Pointer to device instance. */
    PPDMDEVINSR0        pDevInsR0;
    /** Pointer to device instance. */
    PPDMDEVINSRC        pDevInsRC;

    /** Set when the destroying the device instance and the thread must exit. */
    uint32_t volatile   fShutdown;
    /** The async I/O thread handle. NIL_RTTHREAD if no thread. */
    RTTHREAD            AsyncIOThread;
    /** The event semaphore the thread is waiting on for requests. */
    SUPSEMEVENT         hAsyncIOSem;
    /** The support driver session handle. */
    PSUPDRVSESSION      pSupDrvSession;
    /** The request queue for the AIO thread. One element is always unused. */
    ATARequest          aAsyncIORequests[4];
    /** The position at which to insert a new request for the AIO thread. */
    volatile uint8_t    AsyncIOReqHead;
    /** The position at which to get a new request for the AIO thread. */
    volatile uint8_t    AsyncIOReqTail;
    /** Whether to call PDMDevHlpAsyncNotificationCompleted when idle. */
    bool volatile       fSignalIdle;
    uint8_t             Alignment3[1]; /**< Explicit padding of the 1 byte gap. */
    /** Magic delay before triggering interrupts in DMA mode. */
    uint32_t            DelayIRQMillies;
    /** The event semaphore the thread is waiting on during suspended I/O. */
    RTSEMEVENT          SuspendIOSem;
    /** The lock protecting the request queue. */
    PDMCRITSECT         AsyncIORequestLock;

    /** Timestamp we started the reset. */
    uint64_t            u64ResetTime;

    /* Statistics */
    STAMCOUNTER         StatAsyncOps;
    uint64_t            StatAsyncMinWait;
    uint64_t            StatAsyncMaxWait;
    STAMCOUNTER         StatAsyncTimeUS;
    STAMPROFILEADV      StatAsyncTime;
    STAMPROFILE         StatLockWait;
} ATACONTROLLER, *PATACONTROLLER;
AssertCompileMemberAlignment(ATACONTROLLER, lock, 8);
AssertCompileMemberAlignment(ATACONTROLLER, aIfs, 8);
AssertCompileMemberAlignment(ATACONTROLLER, u64ResetTime, 8);
AssertCompileMemberAlignment(ATACONTROLLER, StatAsyncOps, 8);
AssertCompileMemberAlignment(ATACONTROLLER, AsyncIORequestLock, 8);
AssertCompileSizeAlignment(ATACONTROLLER, 8);

typedef enum CHIPSET
{
    /** PIIX3 chipset, must be 0 for saved state compatibility */
    CHIPSET_PIIX3 = 0,
    /** PIIX4 chipset, must be 1 for saved state compatibility */
    CHIPSET_PIIX4 = 1,
    /** ICH6 chipset */
    CHIPSET_ICH6 = 2
} CHIPSET;

/**
 * The state of the ATA PCI device.
 *
 * @extends     PDMPCIDEV
 * @implements  PDMILEDPORTS
 */
typedef struct PCIATAState
{
    PDMPCIDEV                       dev;
    /** The controllers. */
    ATACONTROLLER                   aCts[2];
    /** Pointer to device instance. */
    PPDMDEVINSR3                    pDevIns;
    /** Status LUN: Base interface. */
    PDMIBASE                        IBase;
    /** Status LUN: Leds interface. */
    PDMILEDPORTS                    ILeds;
    /** Status LUN: Partner of ILeds. */
    R3PTRTYPE(PPDMILEDCONNECTORS)   pLedsConnector;
    /** Status LUN: Media Notify. */
    R3PTRTYPE(PPDMIMEDIANOTIFY)     pMediaNotify;
    /** Flag whether RC is enabled. */
    bool                            fRCEnabled;
    /** Flag whether R0 is enabled. */
    bool                            fR0Enabled;
    /** Flag indicating chipset being emulated. */
    uint8_t                         u8Type;
    bool                            Alignment0[HC_ARCH_BITS == 64 ? 5 : 1 ]; /**< Align the struct size. */
} PCIATAState;

#define ATACONTROLLER_IDX(pController) ( (pController) - PDMINS_2_DATA(CONTROLLER_2_DEVINS(pController), PCIATAState *)->aCts )

#define ATADEVSTATE_2_CONTROLLER(pIf)          ( (pIf)->CTX_SUFF(pController) )
#define ATADEVSTATE_2_DEVINS(pIf)              ( (pIf)->CTX_SUFF(pDevIns) )
#define CONTROLLER_2_DEVINS(pController)       ( (pController)->CTX_SUFF(pDevIns) )

#ifndef VBOX_DEVICE_STRUCT_TESTCASE


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN

PDMBOTHCBDECL(int) ataIOPortWrite1Data(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb);
PDMBOTHCBDECL(int) ataIOPortRead1Data(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *u32, unsigned cb);
PDMBOTHCBDECL(int) ataIOPortWriteStr1Data(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint8_t const *pbSrc,
                                          uint32_t *pcTransfers, unsigned cb);
PDMBOTHCBDECL(int) ataIOPortReadStr1Data(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint8_t *pbDst,
                                         uint32_t *pcTransfers, unsigned cb);
PDMBOTHCBDECL(int) ataIOPortWrite1Other(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb);
PDMBOTHCBDECL(int) ataIOPortRead1Other(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *u32, unsigned cb);
PDMBOTHCBDECL(int) ataIOPortWrite2(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb);
PDMBOTHCBDECL(int) ataIOPortRead2(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *u32, unsigned cb);
PDMBOTHCBDECL(int) ataBMDMAIOPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb);
PDMBOTHCBDECL(int) ataBMDMAIOPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb);
RT_C_DECLS_END



#ifdef IN_RING3
DECLINLINE(void) ataSetStatusValue(ATADevState *s, uint8_t stat)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);

    /* Freeze status register contents while processing RESET. */
    if (!pCtl->fReset)
    {
        s->uATARegStatus = stat;
        Log2(("%s: LUN#%d status %#04x\n", __FUNCTION__, s->iLUN, s->uATARegStatus));
    }
}
#endif /* IN_RING3 */


DECLINLINE(void) ataSetStatus(ATADevState *s, uint8_t stat)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);

    /* Freeze status register contents while processing RESET. */
    if (!pCtl->fReset)
    {
        s->uATARegStatus |= stat;
        Log2(("%s: LUN#%d status %#04x\n", __FUNCTION__, s->iLUN, s->uATARegStatus));
    }
}


DECLINLINE(void) ataUnsetStatus(ATADevState *s, uint8_t stat)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);

    /* Freeze status register contents while processing RESET. */
    if (!pCtl->fReset)
    {
        s->uATARegStatus &= ~stat;
        Log2(("%s: LUN#%d status %#04x\n", __FUNCTION__, s->iLUN, s->uATARegStatus));
    }
}

#if defined(IN_RING3) || defined(IN_RING0)

# ifdef IN_RING3
typedef void (*PBeginTransferFunc)(ATADevState *);
typedef bool (*PSourceSinkFunc)(ATADevState *);

static void ataR3ReadWriteSectorsBT(ATADevState *);
static void ataR3PacketBT(ATADevState *);
static void atapiR3CmdBT(ATADevState *);
static void atapiR3PassthroughCmdBT(ATADevState *);

static bool ataR3IdentifySS(ATADevState *);
static bool ataR3FlushSS(ATADevState *);
static bool ataR3ReadSectorsSS(ATADevState *);
static bool ataR3WriteSectorsSS(ATADevState *);
static bool ataR3ExecuteDeviceDiagnosticSS(ATADevState *);
static bool ataR3TrimSS(ATADevState *);
static bool ataR3PacketSS(ATADevState *);
static bool atapiR3GetConfigurationSS(ATADevState *);
static bool atapiR3GetEventStatusNotificationSS(ATADevState *);
static bool atapiR3IdentifySS(ATADevState *);
static bool atapiR3InquirySS(ATADevState *);
static bool atapiR3MechanismStatusSS(ATADevState *);
static bool atapiR3ModeSenseErrorRecoverySS(ATADevState *);
static bool atapiR3ModeSenseCDStatusSS(ATADevState *);
static bool atapiR3ReadSS(ATADevState *);
static bool atapiR3ReadCapacitySS(ATADevState *);
static bool atapiR3ReadDiscInformationSS(ATADevState *);
static bool atapiR3ReadTOCNormalSS(ATADevState *);
static bool atapiR3ReadTOCMultiSS(ATADevState *);
static bool atapiR3ReadTOCRawSS(ATADevState *);
static bool atapiR3ReadTrackInformationSS(ATADevState *);
static bool atapiR3RequestSenseSS(ATADevState *);
static bool atapiR3PassthroughSS(ATADevState *);
static bool atapiR3ReadDVDStructureSS(ATADevState *);
# endif /* IN_RING3 */

/**
 * Begin of transfer function indexes for g_apfnBeginTransFuncs.
 */
typedef enum ATAFNBT
{
    ATAFN_BT_NULL = 0,
    ATAFN_BT_READ_WRITE_SECTORS,
    ATAFN_BT_PACKET,
    ATAFN_BT_ATAPI_CMD,
    ATAFN_BT_ATAPI_PASSTHROUGH_CMD,
    ATAFN_BT_MAX
} ATAFNBT;

# ifdef IN_RING3
/**
 * Array of end transfer functions, the index is ATAFNET.
 * Make sure ATAFNET and this array match!
 */
static const PBeginTransferFunc g_apfnBeginTransFuncs[ATAFN_BT_MAX] =
{
    NULL,
    ataR3ReadWriteSectorsBT,
    ataR3PacketBT,
    atapiR3CmdBT,
    atapiR3PassthroughCmdBT,
};
# endif /* IN_RING3 */

/**
 * Source/sink function indexes for g_apfnSourceSinkFuncs.
 */
typedef enum ATAFNSS
{
    ATAFN_SS_NULL = 0,
    ATAFN_SS_IDENTIFY,
    ATAFN_SS_FLUSH,
    ATAFN_SS_READ_SECTORS,
    ATAFN_SS_WRITE_SECTORS,
    ATAFN_SS_EXECUTE_DEVICE_DIAGNOSTIC,
    ATAFN_SS_TRIM,
    ATAFN_SS_PACKET,
    ATAFN_SS_ATAPI_GET_CONFIGURATION,
    ATAFN_SS_ATAPI_GET_EVENT_STATUS_NOTIFICATION,
    ATAFN_SS_ATAPI_IDENTIFY,
    ATAFN_SS_ATAPI_INQUIRY,
    ATAFN_SS_ATAPI_MECHANISM_STATUS,
    ATAFN_SS_ATAPI_MODE_SENSE_ERROR_RECOVERY,
    ATAFN_SS_ATAPI_MODE_SENSE_CD_STATUS,
    ATAFN_SS_ATAPI_READ,
    ATAFN_SS_ATAPI_READ_CAPACITY,
    ATAFN_SS_ATAPI_READ_DISC_INFORMATION,
    ATAFN_SS_ATAPI_READ_TOC_NORMAL,
    ATAFN_SS_ATAPI_READ_TOC_MULTI,
    ATAFN_SS_ATAPI_READ_TOC_RAW,
    ATAFN_SS_ATAPI_READ_TRACK_INFORMATION,
    ATAFN_SS_ATAPI_REQUEST_SENSE,
    ATAFN_SS_ATAPI_PASSTHROUGH,
    ATAFN_SS_ATAPI_READ_DVD_STRUCTURE,
    ATAFN_SS_MAX
} ATAFNSS;

# ifdef IN_RING3
/**
 * Array of source/sink functions, the index is ATAFNSS.
 * Make sure ATAFNSS and this array match!
 */
static const PSourceSinkFunc g_apfnSourceSinkFuncs[ATAFN_SS_MAX] =
{
    NULL,
    ataR3IdentifySS,
    ataR3FlushSS,
    ataR3ReadSectorsSS,
    ataR3WriteSectorsSS,
    ataR3ExecuteDeviceDiagnosticSS,
    ataR3TrimSS,
    ataR3PacketSS,
    atapiR3GetConfigurationSS,
    atapiR3GetEventStatusNotificationSS,
    atapiR3IdentifySS,
    atapiR3InquirySS,
    atapiR3MechanismStatusSS,
    atapiR3ModeSenseErrorRecoverySS,
    atapiR3ModeSenseCDStatusSS,
    atapiR3ReadSS,
    atapiR3ReadCapacitySS,
    atapiR3ReadDiscInformationSS,
    atapiR3ReadTOCNormalSS,
    atapiR3ReadTOCMultiSS,
    atapiR3ReadTOCRawSS,
    atapiR3ReadTrackInformationSS,
    atapiR3RequestSenseSS,
    atapiR3PassthroughSS,
    atapiR3ReadDVDStructureSS
};
# endif /* IN_RING3 */


static const ATARequest g_ataDMARequest    = { ATA_AIO_DMA,            { { 0, 0, 0, 0, 0 } } };
static const ATARequest g_ataPIORequest    = { ATA_AIO_PIO,            { { 0, 0, 0, 0, 0 } } };
# ifdef IN_RING3
static const ATARequest g_ataResetARequest = { ATA_AIO_RESET_ASSERTED, { { 0, 0, 0, 0, 0 } } };
static const ATARequest g_ataResetCRequest = { ATA_AIO_RESET_CLEARED,  { { 0, 0, 0, 0, 0 } } };
# endif

# ifdef IN_RING3
static void ataR3AsyncIOClearRequests(PATACONTROLLER pCtl)
{
    int rc = PDMCritSectEnter(&pCtl->AsyncIORequestLock, VINF_SUCCESS);
    AssertRC(rc);

    pCtl->AsyncIOReqHead = 0;
    pCtl->AsyncIOReqTail = 0;

    rc = PDMCritSectLeave(&pCtl->AsyncIORequestLock);
    AssertRC(rc);
}
# endif /* IN_RING3 */

static void ataHCAsyncIOPutRequest(PATACONTROLLER pCtl, const ATARequest *pReq)
{
    int rc = PDMCritSectEnter(&pCtl->AsyncIORequestLock, VINF_SUCCESS);
    AssertRC(rc);

    Assert((pCtl->AsyncIOReqHead + 1) % RT_ELEMENTS(pCtl->aAsyncIORequests) != pCtl->AsyncIOReqTail);
    memcpy(&pCtl->aAsyncIORequests[pCtl->AsyncIOReqHead], pReq, sizeof(*pReq));
    pCtl->AsyncIOReqHead++;
    pCtl->AsyncIOReqHead %= RT_ELEMENTS(pCtl->aAsyncIORequests);

    rc = PDMCritSectLeave(&pCtl->AsyncIORequestLock);
    AssertRC(rc);

    rc = PDMHCCritSectScheduleExitEvent(&pCtl->lock, pCtl->hAsyncIOSem);
    if (RT_FAILURE(rc))
    {
        rc = SUPSemEventSignal(pCtl->pSupDrvSession, pCtl->hAsyncIOSem);
        AssertRC(rc);
    }
}

# ifdef IN_RING3

static const ATARequest *ataR3AsyncIOGetCurrentRequest(PATACONTROLLER pCtl)
{
    const ATARequest *pReq;

    int rc = PDMCritSectEnter(&pCtl->AsyncIORequestLock, VINF_SUCCESS);
    AssertRC(rc);

    if (pCtl->AsyncIOReqHead != pCtl->AsyncIOReqTail)
        pReq = &pCtl->aAsyncIORequests[pCtl->AsyncIOReqTail];
    else
        pReq = NULL;

    rc = PDMCritSectLeave(&pCtl->AsyncIORequestLock);
    AssertRC(rc);
    return pReq;
}


/**
 * Remove the request with the given type, as it's finished. The request
 * is not removed blindly, as this could mean a RESET request that is not
 * yet processed (but has cleared the request queue) is lost.
 *
 * @param pCtl      Controller for which to remove the request.
 * @param ReqType   Type of the request to remove.
 */
static void ataR3AsyncIORemoveCurrentRequest(PATACONTROLLER pCtl, ATAAIO ReqType)
{
    int rc = PDMCritSectEnter(&pCtl->AsyncIORequestLock, VINF_SUCCESS);
    AssertRC(rc);

    if (pCtl->AsyncIOReqHead != pCtl->AsyncIOReqTail && pCtl->aAsyncIORequests[pCtl->AsyncIOReqTail].ReqType == ReqType)
    {
        pCtl->AsyncIOReqTail++;
        pCtl->AsyncIOReqTail %= RT_ELEMENTS(pCtl->aAsyncIORequests);
    }

    rc = PDMCritSectLeave(&pCtl->AsyncIORequestLock);
    AssertRC(rc);
}


/**
 * Dump the request queue for a particular controller. First dump the queue
 * contents, then the already processed entries, as long as they haven't been
 * overwritten.
 *
 * @param pCtl      Controller for which to dump the queue.
 */
static void ataR3AsyncIODumpRequests(PATACONTROLLER pCtl)
{
    int rc = PDMCritSectEnter(&pCtl->AsyncIORequestLock, VINF_SUCCESS);
    AssertRC(rc);

    LogRel(("PIIX3 ATA: Ctl#%d: request queue dump (topmost is current):\n", ATACONTROLLER_IDX(pCtl)));
    uint8_t curr = pCtl->AsyncIOReqTail;
    do
    {
        if (curr == pCtl->AsyncIOReqHead)
            LogRel(("PIIX3 ATA: Ctl#%d: processed requests (topmost is oldest):\n", ATACONTROLLER_IDX(pCtl)));
        switch (pCtl->aAsyncIORequests[curr].ReqType)
        {
            case ATA_AIO_NEW:
                LogRel(("new transfer request, iIf=%d iBeginTransfer=%d iSourceSink=%d cbTotalTransfer=%d uTxDir=%d\n",
                        pCtl->aAsyncIORequests[curr].u.t.iIf, pCtl->aAsyncIORequests[curr].u.t.iBeginTransfer,
                        pCtl->aAsyncIORequests[curr].u.t.iSourceSink, pCtl->aAsyncIORequests[curr].u.t.cbTotalTransfer,
                        pCtl->aAsyncIORequests[curr].u.t.uTxDir));
                break;
            case ATA_AIO_DMA:
                LogRel(("dma transfer continuation\n"));
                break;
            case ATA_AIO_PIO:
                LogRel(("pio transfer continuation\n"));
                break;
            case ATA_AIO_RESET_ASSERTED:
                LogRel(("reset asserted request\n"));
                break;
            case ATA_AIO_RESET_CLEARED:
                LogRel(("reset cleared request\n"));
                break;
            case ATA_AIO_ABORT:
                LogRel(("abort request, iIf=%d fResetDrive=%d\n", pCtl->aAsyncIORequests[curr].u.a.iIf,
                                                                  pCtl->aAsyncIORequests[curr].u.a.fResetDrive));
                break;
            default:
                LogRel(("unknown request %d\n", pCtl->aAsyncIORequests[curr].ReqType));
        }
        curr = (curr + 1) % RT_ELEMENTS(pCtl->aAsyncIORequests);
    } while (curr != pCtl->AsyncIOReqTail);

    rc = PDMCritSectLeave(&pCtl->AsyncIORequestLock);
    AssertRC(rc);
}


/**
 * Checks whether the request queue for a particular controller is empty
 * or whether a particular controller is idle.
 *
 * @param pCtl      Controller for which to check the queue.
 * @param fStrict   If set then the controller is checked to be idle.
 */
static bool ataR3AsyncIOIsIdle(PATACONTROLLER pCtl, bool fStrict)
{
    int rc = PDMCritSectEnter(&pCtl->AsyncIORequestLock, VINF_SUCCESS);
    AssertRC(rc);

    bool fIdle = pCtl->fRedoIdle;
    if (!fIdle)
        fIdle = (pCtl->AsyncIOReqHead == pCtl->AsyncIOReqTail);
    if (fStrict)
        fIdle &= (pCtl->uAsyncIOState == ATA_AIO_NEW);

    rc = PDMCritSectLeave(&pCtl->AsyncIORequestLock);
    AssertRC(rc);
    return fIdle;
}


/**
 * Send a transfer request to the async I/O thread.
 *
 * @param   s                   Pointer to the ATA device state data.
 * @param   cbTotalTransfer     Data transfer size.
 * @param   uTxDir              Data transfer direction.
 * @param   iBeginTransfer      Index of BeginTransfer callback.
 * @param   iSourceSink         Index of SourceSink callback.
 * @param   fChainedTransfer    Whether this is a transfer that is part of the previous command/transfer.
 */
static void ataR3StartTransfer(ATADevState *s, uint32_t cbTotalTransfer, uint8_t uTxDir, ATAFNBT iBeginTransfer,
                               ATAFNSS iSourceSink, bool fChainedTransfer)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);
    ATARequest Req;

    Assert(PDMCritSectIsOwner(&pCtl->lock));

    /* Do not issue new requests while the RESET line is asserted. */
    if (pCtl->fReset)
    {
        Log2(("%s: Ctl#%d: suppressed new request as RESET is active\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
        return;
    }

    /* If the controller is already doing something else right now, ignore
     * the command that is being submitted. Some broken guests issue commands
     * twice (e.g. the Linux kernel that comes with Acronis True Image 8). */
    if (!fChainedTransfer && !ataR3AsyncIOIsIdle(pCtl, true /*fStrict*/))
    {
        Log(("%s: Ctl#%d: ignored command %#04x, controller state %d\n",
             __FUNCTION__, ATACONTROLLER_IDX(pCtl), s->uATARegCommand, pCtl->uAsyncIOState));
        LogRel(("PIIX3 IDE: guest issued command %#04x while controller busy\n", s->uATARegCommand));
        return;
    }

    Req.ReqType = ATA_AIO_NEW;
    if (fChainedTransfer)
        Req.u.t.iIf = pCtl->iAIOIf;
    else
        Req.u.t.iIf = pCtl->iSelectedIf;
    Req.u.t.cbTotalTransfer = cbTotalTransfer;
    Req.u.t.uTxDir = uTxDir;
    Req.u.t.iBeginTransfer = iBeginTransfer;
    Req.u.t.iSourceSink = iSourceSink;
    ataSetStatusValue(s, ATA_STAT_BUSY);
    pCtl->fChainedTransfer = fChainedTransfer;

    /*
     * Kick the worker thread into action.
     */
    Log2(("%s: Ctl#%d: message to async I/O thread, new request\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
    ataHCAsyncIOPutRequest(pCtl, &Req);
}


/**
 * Send an abort command request to the async I/O thread.
 *
 * @param   s           Pointer to the ATA device state data.
 * @param   fResetDrive Whether to reset the drive or just abort a command.
 */
static void ataR3AbortCurrentCommand(ATADevState *s, bool fResetDrive)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);
    ATARequest Req;

    Assert(PDMCritSectIsOwner(&pCtl->lock));

    /* Do not issue new requests while the RESET line is asserted. */
    if (pCtl->fReset)
    {
        Log2(("%s: Ctl#%d: suppressed aborting command as RESET is active\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
        return;
    }

    Req.ReqType = ATA_AIO_ABORT;
    Req.u.a.iIf = pCtl->iSelectedIf;
    Req.u.a.fResetDrive = fResetDrive;
    ataSetStatus(s, ATA_STAT_BUSY);
    Log2(("%s: Ctl#%d: message to async I/O thread, abort command on LUN#%d\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl), s->iLUN));
    ataHCAsyncIOPutRequest(pCtl, &Req);
}
# endif /* IN_RING3 */

static void ataHCSetIRQ(ATADevState *s)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);
    PPDMDEVINS pDevIns = ATADEVSTATE_2_DEVINS(s);

    if (!(s->uATARegDevCtl & ATA_DEVCTL_DISABLE_IRQ))
    {
        Log2(("%s: LUN#%d asserting IRQ\n", __FUNCTION__, s->iLUN));
        /* The BMDMA unit unconditionally sets BM_STATUS_INT if the interrupt
         * line is asserted. It monitors the line for a rising edge. */
        if (!s->fIrqPending)
            pCtl->BmDma.u8Status |= BM_STATUS_INT;
        /* Only actually set the IRQ line if updating the currently selected drive. */
        if (s == &pCtl->aIfs[pCtl->iSelectedIf])
        {
            /** @todo experiment with adaptive IRQ delivery: for reads it is
             * better to wait for IRQ delivery, as it reduces latency. */
            if (pCtl->irq == 16)
                PDMDevHlpPCISetIrq(pDevIns, 0, 1);
            else
                PDMDevHlpISASetIrq(pDevIns, pCtl->irq, 1);
        }
    }
    s->fIrqPending = true;
}

#endif /* IN_RING0 || IN_RING3 */

static void ataUnsetIRQ(ATADevState *s)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);
    PPDMDEVINS pDevIns = ATADEVSTATE_2_DEVINS(s);

    if (!(s->uATARegDevCtl & ATA_DEVCTL_DISABLE_IRQ))
    {
        Log2(("%s: LUN#%d deasserting IRQ\n", __FUNCTION__, s->iLUN));
        /* Only actually unset the IRQ line if updating the currently selected drive. */
        if (s == &pCtl->aIfs[pCtl->iSelectedIf])
        {
            if (pCtl->irq == 16)
                PDMDevHlpPCISetIrq(pDevIns, 0, 0);
            else
                PDMDevHlpISASetIrq(pDevIns, pCtl->irq, 0);
        }
    }
    s->fIrqPending = false;
}

#if defined(IN_RING0) || defined(IN_RING3)

static void ataHCPIOTransferStart(ATADevState *s, uint32_t start, uint32_t size)
{
    Log2(("%s: LUN#%d start %d size %d\n", __FUNCTION__, s->iLUN, start, size));
    s->iIOBufferPIODataStart = start;
    s->iIOBufferPIODataEnd = start + size;
    ataSetStatus(s, ATA_STAT_DRQ | ATA_STAT_SEEK);
    ataUnsetStatus(s, ATA_STAT_BUSY);
}


static void ataHCPIOTransferStop(ATADevState *s)
{
    Log2(("%s: LUN#%d\n", __FUNCTION__, s->iLUN));
    if (s->fATAPITransfer)
    {
        s->uATARegNSector = (s->uATARegNSector & ~7) | ATAPI_INT_REASON_IO | ATAPI_INT_REASON_CD;
        Log2(("%s: interrupt reason %#04x\n", __FUNCTION__, s->uATARegNSector));
        ataHCSetIRQ(s);
        s->fATAPITransfer = false;
    }
    s->cbTotalTransfer = 0;
    s->cbElementaryTransfer = 0;
    s->iIOBufferPIODataStart = 0;
    s->iIOBufferPIODataEnd = 0;
    s->iBeginTransfer = ATAFN_BT_NULL;
    s->iSourceSink = ATAFN_SS_NULL;
}


static void ataHCPIOTransferLimitATAPI(ATADevState *s)
{
    uint32_t cbLimit, cbTransfer;

    cbLimit = s->cbPIOTransferLimit;
    /* Use maximum transfer size if the guest requested 0. Avoids a hang. */
    if (cbLimit == 0)
        cbLimit = 0xfffe;
    Log2(("%s: byte count limit=%d\n", __FUNCTION__, cbLimit));
    if (cbLimit == 0xffff)
        cbLimit--;
    cbTransfer = RT_MIN(s->cbTotalTransfer, s->iIOBufferEnd - s->iIOBufferCur);
    if (cbTransfer > cbLimit)
    {
        /* Byte count limit for clipping must be even in this case */
        if (cbLimit & 1)
            cbLimit--;
        cbTransfer = cbLimit;
    }
    s->uATARegLCyl = cbTransfer;
    s->uATARegHCyl = cbTransfer >> 8;
    s->cbElementaryTransfer = cbTransfer;
}

# ifdef IN_RING3

/**
 * Enters the lock protecting the controller data against concurrent access.
 *
 * @returns nothing.
 * @param   pCtl        The controller to lock.
 */
DECLINLINE(void) ataR3LockEnter(PATACONTROLLER pCtl)
{
    STAM_PROFILE_START(&pCtl->StatLockWait, a);
    PDMCritSectEnter(&pCtl->lock, VINF_SUCCESS);
    STAM_PROFILE_STOP(&pCtl->StatLockWait, a);
}

/**
 * Leaves the lock protecting the controller against concurrent data access.
 *
 * @returns nothing.
 * @param   pCtl        The controller to unlock.
 */
DECLINLINE(void) ataR3LockLeave(PATACONTROLLER pCtl)
{
    PDMCritSectLeave(&pCtl->lock);
}

static uint32_t ataR3GetNSectors(ATADevState *s)
{
    /* 0 means either 256 (LBA28) or 65536 (LBA48) sectors. */
    if (s->fLBA48)
    {
        if (!s->uATARegNSector && !s->uATARegNSectorHOB)
            return 65536;
        else
            return s->uATARegNSectorHOB << 8 | s->uATARegNSector;
    }
    else
    {
        if (!s->uATARegNSector)
            return 256;
        else
            return s->uATARegNSector;
    }
}


static void ataR3PadString(uint8_t *pbDst, const char *pbSrc, uint32_t cbSize)
{
    for (uint32_t i = 0; i < cbSize; i++)
    {
        if (*pbSrc)
            pbDst[i ^ 1] = *pbSrc++;
        else
            pbDst[i ^ 1] = ' ';
    }
}


#if 0 /* unused */
/**
 * Compares two MSF values.
 *
 * @returns 1  if the first value is greater than the second value.
 *          0  if both are equal
 *          -1 if the first value is smaller than the second value.
 */
DECLINLINE(int) atapiCmpMSF(const uint8_t *pbMSF1, const uint8_t *pbMSF2)
{
    int iRes = 0;

    for (unsigned i = 0; i < 3; i++)
    {
        if (pbMSF1[i] < pbMSF2[i])
        {
            iRes = -1;
            break;
        }
        else if (pbMSF1[i] > pbMSF2[i])
        {
            iRes = 1;
            break;
        }
    }

    return iRes;
}
#endif /* unused */

static void ataR3CmdOK(ATADevState *s, uint8_t status)
{
    s->uATARegError = 0; /* Not needed by ATA spec, but cannot hurt. */
    ataSetStatusValue(s, ATA_STAT_READY | status);
}


static void ataR3CmdError(ATADevState *s, uint8_t uErrorCode)
{
    Log(("%s: code=%#x\n", __FUNCTION__, uErrorCode));
    Assert(uErrorCode);
    s->uATARegError = uErrorCode;
    ataSetStatusValue(s, ATA_STAT_READY | ATA_STAT_ERR);
    s->cbTotalTransfer = 0;
    s->cbElementaryTransfer = 0;
    s->iIOBufferCur = 0;
    s->iIOBufferEnd = 0;
    s->uTxDir = PDMMEDIATXDIR_NONE;
    s->iBeginTransfer = ATAFN_BT_NULL;
    s->iSourceSink = ATAFN_SS_NULL;
}

static uint32_t ataR3Checksum(void* ptr, size_t count)
{
    uint8_t u8Sum = 0xa5, *p = (uint8_t*)ptr;
    size_t i;

    for (i = 0; i < count; i++)
    {
      u8Sum += *p++;
    }

    return (uint8_t)-(int32_t)u8Sum;
}

static bool ataR3IdentifySS(ATADevState *s)
{
    uint16_t *p;

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    Assert(s->cbElementaryTransfer == 512);

    p = (uint16_t *)s->CTX_SUFF(pbIOBuffer);
    memset(p, 0, 512);
    p[0] = RT_H2LE_U16(0x0040);
    p[1] = RT_H2LE_U16(RT_MIN(s->PCHSGeometry.cCylinders, 16383));
    p[3] = RT_H2LE_U16(s->PCHSGeometry.cHeads);
    /* Block size; obsolete, but required for the BIOS. */
    p[5] = RT_H2LE_U16(s->cbSector);
    p[6] = RT_H2LE_U16(s->PCHSGeometry.cSectors);
    ataR3PadString((uint8_t *)(p + 10), s->szSerialNumber, ATA_SERIAL_NUMBER_LENGTH); /* serial number */
    p[20] = RT_H2LE_U16(3); /* XXX: retired, cache type */
    p[21] = RT_H2LE_U16(512); /* XXX: retired, cache size in sectors */
    p[22] = RT_H2LE_U16(0); /* ECC bytes per sector */
    ataR3PadString((uint8_t *)(p + 23), s->szFirmwareRevision, ATA_FIRMWARE_REVISION_LENGTH); /* firmware version */
    ataR3PadString((uint8_t *)(p + 27), s->szModelNumber, ATA_MODEL_NUMBER_LENGTH); /* model */
# if ATA_MAX_MULT_SECTORS > 1
    p[47] = RT_H2LE_U16(0x8000 | ATA_MAX_MULT_SECTORS);
# endif
    p[48] = RT_H2LE_U16(1); /* dword I/O, used by the BIOS */
    p[49] = RT_H2LE_U16(1 << 11 | 1 << 9 | 1 << 8); /* DMA and LBA supported */
    p[50] = RT_H2LE_U16(1 << 14); /* No drive specific standby timer minimum */
    p[51] = RT_H2LE_U16(240); /* PIO transfer cycle */
    p[52] = RT_H2LE_U16(240); /* DMA transfer cycle */
    p[53] = RT_H2LE_U16(1 | 1 << 1 | 1 << 2); /* words 54-58,64-70,88 valid */
    p[54] = RT_H2LE_U16(RT_MIN(s->PCHSGeometry.cCylinders, 16383));
    p[55] = RT_H2LE_U16(s->PCHSGeometry.cHeads);
    p[56] = RT_H2LE_U16(s->PCHSGeometry.cSectors);
    p[57] = RT_H2LE_U16(  RT_MIN(s->PCHSGeometry.cCylinders, 16383)
                        * s->PCHSGeometry.cHeads
                        * s->PCHSGeometry.cSectors);
    p[58] = RT_H2LE_U16(  RT_MIN(s->PCHSGeometry.cCylinders, 16383)
                        * s->PCHSGeometry.cHeads
                        * s->PCHSGeometry.cSectors >> 16);
    if (s->cMultSectors)
        p[59] = RT_H2LE_U16(0x100 | s->cMultSectors);
    if (s->cTotalSectors <= (1 << 28) - 1)
    {
        p[60] = RT_H2LE_U16(s->cTotalSectors);
        p[61] = RT_H2LE_U16(s->cTotalSectors >> 16);
    }
    else
    {
        /* Report maximum number of sectors possible with LBA28 */
        p[60] = RT_H2LE_U16(((1 << 28) - 1) & 0xffff);
        p[61] = RT_H2LE_U16(((1 << 28) - 1) >> 16);
    }
    p[63] = RT_H2LE_U16(ATA_TRANSFER_ID(ATA_MODE_MDMA, ATA_MDMA_MODE_MAX, s->uATATransferMode)); /* MDMA modes supported / mode enabled */
    p[64] = RT_H2LE_U16(ATA_PIO_MODE_MAX > 2 ? (1 << (ATA_PIO_MODE_MAX - 2)) - 1 : 0); /* PIO modes beyond PIO2 supported */
    p[65] = RT_H2LE_U16(120); /* minimum DMA multiword tx cycle time */
    p[66] = RT_H2LE_U16(120); /* recommended DMA multiword tx cycle time */
    p[67] = RT_H2LE_U16(120); /* minimum PIO cycle time without flow control */
    p[68] = RT_H2LE_U16(120); /* minimum PIO cycle time with IORDY flow control */
    if (   s->pDrvMedia->pfnDiscard
        || s->cbSector != 512
        || s->pDrvMedia->pfnIsNonRotational(s->pDrvMedia))
    {
        p[80] = RT_H2LE_U16(0x1f0); /* support everything up to ATA/ATAPI-8 ACS */
        p[81] = RT_H2LE_U16(0x28); /* conforms to ATA/ATAPI-8 ACS */
    }
    else
    {
        p[80] = RT_H2LE_U16(0x7e); /* support everything up to ATA/ATAPI-6 */
        p[81] = RT_H2LE_U16(0x22); /* conforms to ATA/ATAPI-6 */
    }
    p[82] = RT_H2LE_U16(1 << 3 | 1 << 5 | 1 << 6); /* supports power management,  write cache and look-ahead */
    if (s->cTotalSectors <= (1 << 28) - 1)
        p[83] = RT_H2LE_U16(1 << 14 | 1 << 12); /* supports FLUSH CACHE */
    else
        p[83] = RT_H2LE_U16(1 << 14 | 1 << 10 | 1 << 12 | 1 << 13); /* supports LBA48, FLUSH CACHE and FLUSH CACHE EXT */
    p[84] = RT_H2LE_U16(1 << 14);
    p[85] = RT_H2LE_U16(1 << 3 | 1 << 5 | 1 << 6); /* enabled power management,  write cache and look-ahead */
    if (s->cTotalSectors <= (1 << 28) - 1)
        p[86] = RT_H2LE_U16(1 << 12); /* enabled FLUSH CACHE */
    else
        p[86] = RT_H2LE_U16(1 << 10 | 1 << 12 | 1 << 13); /* enabled LBA48, FLUSH CACHE and FLUSH CACHE EXT */
    p[87] = RT_H2LE_U16(1 << 14);
    p[88] = RT_H2LE_U16(ATA_TRANSFER_ID(ATA_MODE_UDMA, ATA_UDMA_MODE_MAX, s->uATATransferMode)); /* UDMA modes supported / mode enabled */
    p[93] = RT_H2LE_U16((1 | 1 << 1) << ((s->iLUN & 1) == 0 ? 0 : 8) | 1 << 13 | 1 << 14);
    if (s->cTotalSectors > (1 << 28) - 1)
    {
        p[100] = RT_H2LE_U16(s->cTotalSectors);
        p[101] = RT_H2LE_U16(s->cTotalSectors >> 16);
        p[102] = RT_H2LE_U16(s->cTotalSectors >> 32);
        p[103] = RT_H2LE_U16(s->cTotalSectors >> 48);
    }

    if (s->cbSector != 512)
    {
        uint32_t cSectorSizeInWords = s->cbSector / sizeof(uint16_t);
        /* Enable reporting of logical sector size. */
        p[106] |= RT_H2LE_U16(RT_BIT(12) | RT_BIT(14));
        p[117] = RT_H2LE_U16(cSectorSizeInWords);
        p[118] = RT_H2LE_U16(cSectorSizeInWords >> 16);
    }

    if (s->pDrvMedia->pfnDiscard) /** @todo Set bit 14 in word 69 too? (Deterministic read after TRIM). */
        p[169] = RT_H2LE_U16(1); /* DATA SET MANAGEMENT command supported. */
    if (s->pDrvMedia->pfnIsNonRotational(s->pDrvMedia))
        p[217] = RT_H2LE_U16(1); /* Non-rotational medium */
    uint32_t uCsum = ataR3Checksum(p, 510);
    p[255] = RT_H2LE_U16(0xa5 | (uCsum << 8)); /* Integrity word */
    s->iSourceSink = ATAFN_SS_NULL;
    ataR3CmdOK(s, ATA_STAT_SEEK);
    return false;
}


static bool ataR3FlushSS(ATADevState *s)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);
    int rc;

    Assert(s->uTxDir == PDMMEDIATXDIR_NONE);
    Assert(!s->cbElementaryTransfer);

    ataR3LockLeave(pCtl);

    STAM_PROFILE_START(&s->StatFlushes, f);
    rc = s->pDrvMedia->pfnFlush(s->pDrvMedia);
    AssertRC(rc);
    STAM_PROFILE_STOP(&s->StatFlushes, f);

    ataR3LockEnter(pCtl);
    ataR3CmdOK(s, 0);
    return false;
}

static bool atapiR3IdentifySS(ATADevState *s)
{
    uint16_t *p;

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    Assert(s->cbElementaryTransfer == 512);

    p = (uint16_t *)s->CTX_SUFF(pbIOBuffer);
    memset(p, 0, 512);
    /* Removable CDROM, 3ms response, 12 byte packets */
    p[0] = RT_H2LE_U16(2 << 14 | 5 << 8 | 1 << 7 | 0 << 5 | 0 << 0);
    ataR3PadString((uint8_t *)(p + 10), s->szSerialNumber, ATA_SERIAL_NUMBER_LENGTH); /* serial number */
    p[20] = RT_H2LE_U16(3); /* XXX: retired, cache type */
    p[21] = RT_H2LE_U16(512); /* XXX: retired, cache size in sectors */
    ataR3PadString((uint8_t *)(p + 23), s->szFirmwareRevision, ATA_FIRMWARE_REVISION_LENGTH); /* firmware version */
    ataR3PadString((uint8_t *)(p + 27), s->szModelNumber, ATA_MODEL_NUMBER_LENGTH); /* model */
    p[49] = RT_H2LE_U16(1 << 11 | 1 << 9 | 1 << 8); /* DMA and LBA supported */
    p[50] = RT_H2LE_U16(1 << 14);  /* No drive specific standby timer minimum */
    p[51] = RT_H2LE_U16(240); /* PIO transfer cycle */
    p[52] = RT_H2LE_U16(240); /* DMA transfer cycle */
    p[53] = RT_H2LE_U16(1 << 1 | 1 << 2); /* words 64-70,88 are valid */
    p[63] = RT_H2LE_U16(ATA_TRANSFER_ID(ATA_MODE_MDMA, ATA_MDMA_MODE_MAX, s->uATATransferMode)); /* MDMA modes supported / mode enabled */
    p[64] = RT_H2LE_U16(ATA_PIO_MODE_MAX > 2 ? (1 << (ATA_PIO_MODE_MAX - 2)) - 1 : 0); /* PIO modes beyond PIO2 supported */
    p[65] = RT_H2LE_U16(120); /* minimum DMA multiword tx cycle time */
    p[66] = RT_H2LE_U16(120); /* recommended DMA multiword tx cycle time */
    p[67] = RT_H2LE_U16(120); /* minimum PIO cycle time without flow control */
    p[68] = RT_H2LE_U16(120); /* minimum PIO cycle time with IORDY flow control */
    p[73] = RT_H2LE_U16(0x003e); /* ATAPI CDROM major */
    p[74] = RT_H2LE_U16(9); /* ATAPI CDROM minor */
    p[75] = RT_H2LE_U16(1); /* queue depth 1 */
    p[80] = RT_H2LE_U16(0x7e); /* support everything up to ATA/ATAPI-6 */
    p[81] = RT_H2LE_U16(0x22); /* conforms to ATA/ATAPI-6 */
    p[82] = RT_H2LE_U16(1 << 4 | 1 << 9); /* supports packet command set and DEVICE RESET */
    p[83] = RT_H2LE_U16(1 << 14);
    p[84] = RT_H2LE_U16(1 << 14);
    p[85] = RT_H2LE_U16(1 << 4 | 1 << 9); /* enabled packet command set and DEVICE RESET */
    p[86] = RT_H2LE_U16(0);
    p[87] = RT_H2LE_U16(1 << 14);
    p[88] = RT_H2LE_U16(ATA_TRANSFER_ID(ATA_MODE_UDMA, ATA_UDMA_MODE_MAX, s->uATATransferMode)); /* UDMA modes supported / mode enabled */
    p[93] = RT_H2LE_U16((1 | 1 << 1) << ((s->iLUN & 1) == 0 ? 0 : 8) | 1 << 13 | 1 << 14);
    /* According to ATAPI-5 spec:
     *
     * The use of this word is optional.
     * If bits 7:0 of this word contain the signature A5h, bits 15:8
     * contain the data
     * structure checksum.
     * The data structure checksum is the twos complement of the sum of
     * all bytes in words 0 through 254 and the byte consisting of
     * bits 7:0 in word 255.
     * Each byte shall be added with unsigned arithmetic,
     * and overflow shall be ignored.
     * The sum of all 512 bytes is zero when the checksum is correct.
     */
    uint32_t uCsum = ataR3Checksum(p, 510);
    p[255] = RT_H2LE_U16(0xa5 | (uCsum << 8)); /* Integrity word */

    s->iSourceSink = ATAFN_SS_NULL;
    ataR3CmdOK(s, ATA_STAT_SEEK);
    return false;
}


static void ataR3SetSignature(ATADevState *s)
{
    s->uATARegSelect &= 0xf0; /* clear head */
    /* put signature */
    s->uATARegNSector = 1;
    s->uATARegSector = 1;
    if (s->fATAPI)
    {
        s->uATARegLCyl = 0x14;
        s->uATARegHCyl = 0xeb;
    }
    else if (s->pDrvMedia)
    {
        s->uATARegLCyl = 0;
        s->uATARegHCyl = 0;
    }
    else
    {
        s->uATARegLCyl = 0xff;
        s->uATARegHCyl = 0xff;
    }
}


static uint64_t ataR3GetSector(ATADevState *s)
{
    uint64_t iLBA;
    if (s->uATARegSelect & 0x40)
    {
        /* any LBA variant */
        if (s->fLBA48)
        {
            /* LBA48 */
            iLBA = ((uint64_t)s->uATARegHCylHOB << 40) |
                ((uint64_t)s->uATARegLCylHOB << 32) |
                ((uint64_t)s->uATARegSectorHOB << 24) |
                ((uint64_t)s->uATARegHCyl << 16) |
                ((uint64_t)s->uATARegLCyl << 8) |
                s->uATARegSector;
        }
        else
        {
            /* LBA */
            iLBA = ((s->uATARegSelect & 0x0f) << 24) | (s->uATARegHCyl << 16) |
                (s->uATARegLCyl << 8) | s->uATARegSector;
        }
    }
    else
    {
        /* CHS */
        iLBA = ((s->uATARegHCyl << 8) | s->uATARegLCyl) * s->PCHSGeometry.cHeads * s->PCHSGeometry.cSectors +
            (s->uATARegSelect & 0x0f) * s->PCHSGeometry.cSectors +
            (s->uATARegSector - 1);
    }
    return iLBA;
}

static void ataR3SetSector(ATADevState *s, uint64_t iLBA)
{
    uint32_t cyl, r;
    if (s->uATARegSelect & 0x40)
    {
        /* any LBA variant */
        if (s->fLBA48)
        {
            /* LBA48 */
            s->uATARegHCylHOB = iLBA >> 40;
            s->uATARegLCylHOB = iLBA >> 32;
            s->uATARegSectorHOB = iLBA >> 24;
            s->uATARegHCyl = iLBA >> 16;
            s->uATARegLCyl = iLBA >> 8;
            s->uATARegSector = iLBA;
        }
        else
        {
            /* LBA */
            s->uATARegSelect = (s->uATARegSelect & 0xf0) | (iLBA >> 24);
            s->uATARegHCyl = (iLBA >> 16);
            s->uATARegLCyl = (iLBA >> 8);
            s->uATARegSector = (iLBA);
        }
    }
    else
    {
        /* CHS */
        cyl = iLBA / (s->PCHSGeometry.cHeads * s->PCHSGeometry.cSectors);
        r = iLBA % (s->PCHSGeometry.cHeads * s->PCHSGeometry.cSectors);
        s->uATARegHCyl = cyl >> 8;
        s->uATARegLCyl = cyl;
        s->uATARegSelect = (s->uATARegSelect & 0xf0) | ((r / s->PCHSGeometry.cSectors) & 0x0f);
        s->uATARegSector = (r % s->PCHSGeometry.cSectors) + 1;
    }
}


static void ataR3WarningDiskFull(PPDMDEVINS pDevIns)
{
    int rc;
    LogRel(("PIIX3 ATA: Host disk full\n"));
    rc = PDMDevHlpVMSetRuntimeError(pDevIns, VMSETRTERR_FLAGS_SUSPEND | VMSETRTERR_FLAGS_NO_WAIT, "DevATA_DISKFULL",
                                    N_("Host system reported disk full. VM execution is suspended. You can resume after freeing some space"));
    AssertRC(rc);
}

static void ataR3WarningFileTooBig(PPDMDEVINS pDevIns)
{
    int rc;
    LogRel(("PIIX3 ATA: File too big\n"));
    rc = PDMDevHlpVMSetRuntimeError(pDevIns, VMSETRTERR_FLAGS_SUSPEND | VMSETRTERR_FLAGS_NO_WAIT, "DevATA_FILETOOBIG",
                                    N_("Host system reported that the file size limit of the host file system has been exceeded. VM execution is suspended. You need to move your virtual hard disk to a filesystem which allows bigger files"));
    AssertRC(rc);
}

static void ataR3WarningISCSI(PPDMDEVINS pDevIns)
{
    int rc;
    LogRel(("PIIX3 ATA: iSCSI target unavailable\n"));
    rc = PDMDevHlpVMSetRuntimeError(pDevIns, VMSETRTERR_FLAGS_SUSPEND | VMSETRTERR_FLAGS_NO_WAIT, "DevATA_ISCSIDOWN",
                                    N_("The iSCSI target has stopped responding. VM execution is suspended. You can resume when it is available again"));
    AssertRC(rc);
}

static bool ataR3IsRedoSetWarning(ATADevState *s, int rc)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);
    Assert(!PDMCritSectIsOwner(&pCtl->lock));
    if (rc == VERR_DISK_FULL)
    {
        pCtl->fRedoIdle = true;
        ataR3WarningDiskFull(ATADEVSTATE_2_DEVINS(s));
        return true;
    }
    if (rc == VERR_FILE_TOO_BIG)
    {
        pCtl->fRedoIdle = true;
        ataR3WarningFileTooBig(ATADEVSTATE_2_DEVINS(s));
        return true;
    }
    if (rc == VERR_BROKEN_PIPE || rc == VERR_NET_CONNECTION_REFUSED)
    {
        pCtl->fRedoIdle = true;
        /* iSCSI connection abort (first error) or failure to reestablish
         * connection (second error). Pause VM. On resume we'll retry. */
        ataR3WarningISCSI(ATADEVSTATE_2_DEVINS(s));
        return true;
    }
    if (rc == VERR_VD_DEK_MISSING)
    {
        /* Error message already set. */
        pCtl->fRedoIdle = true;
        return true;
    }

    return false;
}


static int ataR3ReadSectors(ATADevState *s, uint64_t u64Sector, void *pvBuf,
                            uint32_t cSectors, bool *pfRedo)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);
    int rc;

    ataR3LockLeave(pCtl);

    STAM_PROFILE_ADV_START(&s->StatReads, r);
    s->Led.Asserted.s.fReading = s->Led.Actual.s.fReading = 1;
    rc = s->pDrvMedia->pfnRead(s->pDrvMedia, u64Sector * s->cbSector, pvBuf, cSectors * s->cbSector);
    s->Led.Actual.s.fReading = 0;
    STAM_PROFILE_ADV_STOP(&s->StatReads, r);
    Log4(("ataR3ReadSectors: rc=%Rrc cSectors=%#x u64Sector=%llu\n%.*Rhxd\n",
          rc, cSectors, u64Sector, cSectors * s->cbSector, pvBuf));

    STAM_REL_COUNTER_ADD(&s->StatBytesRead, cSectors * s->cbSector);

    if (RT_SUCCESS(rc))
        *pfRedo = false;
    else
        *pfRedo = ataR3IsRedoSetWarning(s, rc);

    ataR3LockEnter(pCtl);
    return rc;
}


static int ataR3WriteSectors(ATADevState *s, uint64_t u64Sector,
                             const void *pvBuf, uint32_t cSectors, bool *pfRedo)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);
    int rc;

    ataR3LockLeave(pCtl);

    STAM_PROFILE_ADV_START(&s->StatWrites, w);
    s->Led.Asserted.s.fWriting = s->Led.Actual.s.fWriting = 1;
# ifdef VBOX_INSTRUMENT_DMA_WRITES
    if (s->fDMA)
        STAM_PROFILE_ADV_START(&s->StatInstrVDWrites, vw);
# endif
    rc = s->pDrvMedia->pfnWrite(s->pDrvMedia, u64Sector * s->cbSector, pvBuf, cSectors * s->cbSector);
# ifdef VBOX_INSTRUMENT_DMA_WRITES
    if (s->fDMA)
        STAM_PROFILE_ADV_STOP(&s->StatInstrVDWrites, vw);
# endif
    s->Led.Actual.s.fWriting = 0;
    STAM_PROFILE_ADV_STOP(&s->StatWrites, w);
    Log4(("ataR3WriteSectors: rc=%Rrc cSectors=%#x u64Sector=%llu\n%.*Rhxd\n",
          rc, cSectors, u64Sector, cSectors * s->cbSector, pvBuf));

    STAM_REL_COUNTER_ADD(&s->StatBytesWritten, cSectors * s->cbSector);

    if (RT_SUCCESS(rc))
        *pfRedo = false;
    else
        *pfRedo = ataR3IsRedoSetWarning(s, rc);

    ataR3LockEnter(pCtl);
    return rc;
}


static void ataR3ReadWriteSectorsBT(ATADevState *s)
{
    uint32_t cSectors;

    cSectors = s->cbTotalTransfer / s->cbSector;
    if (cSectors > s->cSectorsPerIRQ)
        s->cbElementaryTransfer = s->cSectorsPerIRQ * s->cbSector;
    else
        s->cbElementaryTransfer = cSectors * s->cbSector;
    if (s->uTxDir == PDMMEDIATXDIR_TO_DEVICE)
        ataR3CmdOK(s, 0);
}


static bool ataR3ReadSectorsSS(ATADevState *s)
{
    int rc;
    uint32_t cSectors;
    uint64_t iLBA;
    bool fRedo;

    cSectors = s->cbElementaryTransfer / s->cbSector;
    Assert(cSectors);
    iLBA = ataR3GetSector(s);
    Log(("%s: %d sectors at LBA %d\n", __FUNCTION__, cSectors, iLBA));
    rc = ataR3ReadSectors(s, iLBA, s->CTX_SUFF(pbIOBuffer), cSectors, &fRedo);
    if (RT_SUCCESS(rc))
    {
        ataR3SetSector(s, iLBA + cSectors);
        if (s->cbElementaryTransfer == s->cbTotalTransfer)
            s->iSourceSink = ATAFN_SS_NULL;
        ataR3CmdOK(s, ATA_STAT_SEEK);
    }
    else
    {
        if (fRedo)
            return fRedo;
        if (s->cErrors++ < MAX_LOG_REL_ERRORS)
            LogRel(("PIIX3 ATA: LUN#%d: disk read error (rc=%Rrc iSector=%#RX64 cSectors=%#RX32)\n",
                    s->iLUN, rc, iLBA, cSectors));

        /*
         * Check if we got interrupted. We don't need to set status variables
         * because the request was aborted.
         */
        if (rc != VERR_INTERRUPTED)
            ataR3CmdError(s, ID_ERR);
    }
    return false;
}


static bool ataR3WriteSectorsSS(ATADevState *s)
{
    int rc;
    uint32_t cSectors;
    uint64_t iLBA;
    bool fRedo;

    cSectors = s->cbElementaryTransfer / s->cbSector;
    Assert(cSectors);
    iLBA = ataR3GetSector(s);
    Log(("%s: %d sectors at LBA %d\n", __FUNCTION__, cSectors, iLBA));
    rc = ataR3WriteSectors(s, iLBA, s->CTX_SUFF(pbIOBuffer), cSectors, &fRedo);
    if (RT_SUCCESS(rc))
    {
        ataR3SetSector(s, iLBA + cSectors);
        if (!s->cbTotalTransfer)
            s->iSourceSink = ATAFN_SS_NULL;
        ataR3CmdOK(s, ATA_STAT_SEEK);
    }
    else
    {
        if (fRedo)
            return fRedo;
        if (s->cErrors++ < MAX_LOG_REL_ERRORS)
            LogRel(("PIIX3 ATA: LUN#%d: disk write error (rc=%Rrc iSector=%#RX64 cSectors=%#RX32)\n",
                    s->iLUN, rc, iLBA, cSectors));

        /*
         * Check if we got interrupted. We don't need to set status variables
         * because the request was aborted.
         */
        if (rc != VERR_INTERRUPTED)
            ataR3CmdError(s, ID_ERR);
    }
    return false;
}


static void atapiR3CmdOK(ATADevState *s)
{
    s->uATARegError = 0;
    ataSetStatusValue(s, ATA_STAT_READY);
    s->uATARegNSector = (s->uATARegNSector & ~7)
        | ((s->uTxDir != PDMMEDIATXDIR_TO_DEVICE) ? ATAPI_INT_REASON_IO : 0)
        | (!s->cbTotalTransfer ? ATAPI_INT_REASON_CD : 0);
    Log2(("%s: interrupt reason %#04x\n", __FUNCTION__, s->uATARegNSector));

    memset(s->abATAPISense, '\0', sizeof(s->abATAPISense));
    s->abATAPISense[0] = 0x70 | (1 << 7);
    s->abATAPISense[7] = 10;
}


static void atapiR3CmdError(ATADevState *s, const uint8_t *pabATAPISense, size_t cbATAPISense)
{
    Log(("%s: sense=%#x (%s) asc=%#x ascq=%#x (%s)\n", __FUNCTION__, pabATAPISense[2] & 0x0f, SCSISenseText(pabATAPISense[2] & 0x0f),
             pabATAPISense[12], pabATAPISense[13], SCSISenseExtText(pabATAPISense[12], pabATAPISense[13])));
    s->uATARegError = pabATAPISense[2] << 4;
    ataSetStatusValue(s, ATA_STAT_READY | ATA_STAT_ERR);
    s->uATARegNSector = (s->uATARegNSector & ~7) | ATAPI_INT_REASON_IO | ATAPI_INT_REASON_CD;
    Log2(("%s: interrupt reason %#04x\n", __FUNCTION__, s->uATARegNSector));
    memset(s->abATAPISense, '\0', sizeof(s->abATAPISense));
    memcpy(s->abATAPISense, pabATAPISense, RT_MIN(cbATAPISense, sizeof(s->abATAPISense)));
    s->cbTotalTransfer = 0;
    s->cbElementaryTransfer = 0;
    s->cbAtapiPassthroughTransfer = 0;
    s->iIOBufferCur = 0;
    s->iIOBufferEnd = 0;
    s->uTxDir = PDMMEDIATXDIR_NONE;
    s->iBeginTransfer = ATAFN_BT_NULL;
    s->iSourceSink = ATAFN_SS_NULL;
}


/** @todo deprecated function - doesn't provide enough info. Replace by direct
 * calls to atapiR3CmdError()  with full data. */
static void atapiR3CmdErrorSimple(ATADevState *s, uint8_t uATAPISenseKey, uint8_t uATAPIASC)
{
    uint8_t abATAPISense[ATAPI_SENSE_SIZE];
    memset(abATAPISense, '\0', sizeof(abATAPISense));
    abATAPISense[0] = 0x70 | (1 << 7);
    abATAPISense[2] = uATAPISenseKey & 0x0f;
    abATAPISense[7] = 10;
    abATAPISense[12] = uATAPIASC;
    atapiR3CmdError(s, abATAPISense, sizeof(abATAPISense));
}


static void atapiR3CmdBT(ATADevState *s)
{
    s->fATAPITransfer = true;
    s->cbElementaryTransfer = s->cbTotalTransfer;
    s->cbAtapiPassthroughTransfer = s->cbTotalTransfer;
    s->cbPIOTransferLimit = s->uATARegLCyl | (s->uATARegHCyl << 8);
    if (s->uTxDir == PDMMEDIATXDIR_TO_DEVICE)
        atapiR3CmdOK(s);
}


static void atapiR3PassthroughCmdBT(ATADevState *s)
{
    atapiR3CmdBT(s);
}

static bool atapiR3ReadSS(ATADevState *s)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);
    int rc = VINF_SUCCESS;
    uint32_t cbTransfer, cSectors;
    uint64_t cbBlockRegion = 0;

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    cbTransfer = RT_MIN(s->cbTotalTransfer, s->cbIOBuffer);
    cSectors = cbTransfer / s->cbATAPISector;
    Assert(cSectors * s->cbATAPISector <= cbTransfer);
    Log(("%s: %d sectors at LBA %d\n", __FUNCTION__, cSectors, s->iATAPILBA));

    ataR3LockLeave(pCtl);

    rc = s->pDrvMedia->pfnQueryRegionPropertiesForLba(s->pDrvMedia, s->iATAPILBA, NULL, NULL,
                                                      &cbBlockRegion, NULL);
    if (RT_SUCCESS(rc))
    {
        STAM_PROFILE_ADV_START(&s->StatReads, r);
        s->Led.Asserted.s.fReading = s->Led.Actual.s.fReading = 1;

        /* If the region block size and requested sector matches we can just pass the request through. */
        if (cbBlockRegion == s->cbATAPISector)
            rc = s->pDrvMedia->pfnRead(s->pDrvMedia, (uint64_t)s->iATAPILBA * s->cbATAPISector,
                                       s->CTX_SUFF(pbIOBuffer), s->cbATAPISector * cSectors);
        else
        {
            if (cbBlockRegion == 2048 && s->cbATAPISector == 2352)
            {
                /* Generate the sync bytes. */
                uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);

                for (uint32_t i = s->iATAPILBA; i < s->iATAPILBA + cSectors; i++)
                {
                    /* Sync bytes, see 4.2.3.8 CD Main Channel Block Formats */
                    *pbBuf++ = 0x00;
                    memset(pbBuf, 0xff, 10);
                    pbBuf += 10;
                    *pbBuf++ = 0x00;
                    /* MSF */
                    scsiLBA2MSF(pbBuf, i);
                    pbBuf += 3;
                    *pbBuf++ = 0x01; /* mode 1 data */
                    /* data */
                    rc = s->pDrvMedia->pfnRead(s->pDrvMedia, (uint64_t)i * 2048, pbBuf, 2048);
                    if (RT_FAILURE(rc))
                        break;
                    pbBuf += 2048;
                    /**
                     * @todo: maybe compute ECC and parity, layout is:
                     * 2072 4   EDC
                     * 2076 172 P parity symbols
                     * 2248 104 Q parity symbols
                     */
                    memset(pbBuf, 0, 280);
                    pbBuf += 280;
                }
            }
            else if (cbBlockRegion == 2352 && s->cbATAPISector == 2048)
            {
                /* Read only the user data portion. */
                uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);

                for (uint32_t i = s->iATAPILBA; i < s->iATAPILBA + cSectors; i++)
                {
                    uint8_t abTmp[2352];
                    rc = s->pDrvMedia->pfnRead(s->pDrvMedia, (uint64_t)i * 2352, &abTmp[0], 2352);
                    if (RT_FAILURE(rc))
                        break;

                    memcpy(pbBuf, &abTmp[16], 2048);
                    pbBuf += 2048;
                }
            }
        }
        s->Led.Actual.s.fReading = 0;
        STAM_PROFILE_ADV_STOP(&s->StatReads, r);
    }

    ataR3LockEnter(pCtl);

    if (RT_SUCCESS(rc))
    {
        STAM_REL_COUNTER_ADD(&s->StatBytesRead, s->cbATAPISector * cSectors);

        /* The initial buffer end value has been set up based on the total
         * transfer size. But the I/O buffer size limits what can actually be
         * done in one transfer, so set the actual value of the buffer end. */
        s->cbElementaryTransfer = cbTransfer;
        if (cbTransfer >= s->cbTotalTransfer)
            s->iSourceSink = ATAFN_SS_NULL;
        atapiR3CmdOK(s);
        s->iATAPILBA += cSectors;
    }
    else
    {
        if (s->cErrors++ < MAX_LOG_REL_ERRORS)
            LogRel(("PIIX3 ATA: LUN#%d: CD-ROM read error, %d sectors at LBA %d\n", s->iLUN, cSectors, s->iATAPILBA));

        /*
         * Check if we got interrupted. We don't need to set status variables
         * because the request was aborted.
         */
        if (rc != VERR_INTERRUPTED)
            atapiR3CmdErrorSimple(s, SCSI_SENSE_MEDIUM_ERROR, SCSI_ASC_READ_ERROR);
    }
    return false;
}

/**
 * Sets the given media track type.
 */
static uint32_t ataR3MediumTypeSet(ATADevState *s, uint32_t MediaTrackType)
{
    return ASMAtomicXchgU32(&s->MediaTrackType, MediaTrackType);
}

static bool atapiR3PassthroughSS(ATADevState *s)
{
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);
    int rc = VINF_SUCCESS;
    uint8_t abATAPISense[ATAPI_SENSE_SIZE];
    uint32_t cbTransfer;
    PSTAMPROFILEADV pProf = NULL;

    cbTransfer = RT_MIN(s->cbAtapiPassthroughTransfer, s->cbIOBuffer);

    if (s->uTxDir == PDMMEDIATXDIR_TO_DEVICE)
        Log3(("ATAPI PT data write (%d): %.*Rhxs\n", cbTransfer, cbTransfer, s->CTX_SUFF(pbIOBuffer)));

    /* Simple heuristics: if there is at least one sector of data
     * to transfer, it's worth updating the LEDs. */
    if (cbTransfer >= 2048)
    {
        if (s->uTxDir != PDMMEDIATXDIR_TO_DEVICE)
        {
            s->Led.Asserted.s.fReading = s->Led.Actual.s.fReading = 1;
            pProf = &s->StatReads;
        }
        else
        {
            s->Led.Asserted.s.fWriting = s->Led.Actual.s.fWriting = 1;
            pProf = &s->StatWrites;
        }
    }

    ataR3LockLeave(pCtl);

# if defined(LOG_ENABLED)
    char szBuf[1024];

    memset(szBuf, 0, sizeof(szBuf));

    switch (s->aATAPICmd[0])
    {
        case SCSI_MODE_SELECT_10:
        {
            size_t cbBlkDescLength = scsiBE2H_U16(&s->CTX_SUFF(pbIOBuffer)[6]);

            SCSILogModePage(szBuf, sizeof(szBuf) - 1,
                            s->CTX_SUFF(pbIOBuffer) + 8 + cbBlkDescLength,
                            cbTransfer - 8 - cbBlkDescLength);
            break;
        }
        case SCSI_SEND_CUE_SHEET:
        {
            SCSILogCueSheet(szBuf, sizeof(szBuf) - 1,
                            s->CTX_SUFF(pbIOBuffer), cbTransfer);
            break;
        }
        default:
            break;
    }

    Log2(("%s\n", szBuf));
# endif

    if (pProf) { STAM_PROFILE_ADV_START(pProf, b); }
    if (   cbTransfer > SCSI_MAX_BUFFER_SIZE
        || s->cbElementaryTransfer > s->cbIOBuffer)
    {
        /* Linux accepts commands with up to 100KB of data, but expects
         * us to handle commands with up to 128KB of data. The usual
         * imbalance of powers. */
        uint8_t aATAPICmd[ATAPI_PACKET_SIZE];
        uint32_t iATAPILBA, cSectors, cReqSectors, cbCurrTX;
        uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);
        uint32_t cSectorsMax; /**< Maximum amount of sectors to read without exceeding the I/O buffer. */

        Assert(s->cbATAPISector);
        cSectorsMax = cbTransfer / s->cbATAPISector;
        Assert(cSectorsMax * s->cbATAPISector <= s->cbIOBuffer);

        switch (s->aATAPICmd[0])
        {
            case SCSI_READ_10:
            case SCSI_WRITE_10:
            case SCSI_WRITE_AND_VERIFY_10:
                iATAPILBA = scsiBE2H_U32(s->aATAPICmd + 2);
                cSectors = scsiBE2H_U16(s->aATAPICmd + 7);
                break;
            case SCSI_READ_12:
            case SCSI_WRITE_12:
                iATAPILBA = scsiBE2H_U32(s->aATAPICmd + 2);
                cSectors = scsiBE2H_U32(s->aATAPICmd + 6);
                break;
            case SCSI_READ_CD:
                iATAPILBA = scsiBE2H_U32(s->aATAPICmd + 2);
                cSectors = scsiBE2H_U24(s->aATAPICmd + 6);
                break;
            case SCSI_READ_CD_MSF:
                iATAPILBA = scsiMSF2LBA(s->aATAPICmd + 3);
                cSectors = scsiMSF2LBA(s->aATAPICmd + 6) - iATAPILBA;
                break;
            default:
                AssertMsgFailed(("Don't know how to split command %#04x\n", s->aATAPICmd[0]));
                if (s->cErrors++ < MAX_LOG_REL_ERRORS)
                    LogRel(("PIIX3 ATA: LUN#%d: CD-ROM passthrough split error\n", s->iLUN));
                atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_ILLEGAL_OPCODE);
                ataR3LockEnter(pCtl);
                return false;
        }
        cSectorsMax = RT_MIN(cSectorsMax, cSectors);
        memcpy(aATAPICmd, s->aATAPICmd, ATAPI_PACKET_SIZE);
        cReqSectors = 0;
        for (uint32_t i = cSectorsMax; i > 0; i -= cReqSectors)
        {
            if (i * s->cbATAPISector > SCSI_MAX_BUFFER_SIZE)
                cReqSectors = SCSI_MAX_BUFFER_SIZE / s->cbATAPISector;
            else
                cReqSectors = i;
            cbCurrTX = s->cbATAPISector * cReqSectors;
            switch (s->aATAPICmd[0])
            {
                case SCSI_READ_10:
                case SCSI_WRITE_10:
                case SCSI_WRITE_AND_VERIFY_10:
                    scsiH2BE_U32(aATAPICmd + 2, iATAPILBA);
                    scsiH2BE_U16(aATAPICmd + 7, cReqSectors);
                    break;
                case SCSI_READ_12:
                case SCSI_WRITE_12:
                    scsiH2BE_U32(aATAPICmd + 2, iATAPILBA);
                    scsiH2BE_U32(aATAPICmd + 6, cReqSectors);
                    break;
                case SCSI_READ_CD:
                    scsiH2BE_U32(aATAPICmd + 2, iATAPILBA);
                    scsiH2BE_U24(aATAPICmd + 6, cReqSectors);
                    break;
                case SCSI_READ_CD_MSF:
                    scsiLBA2MSF(aATAPICmd + 3, iATAPILBA);
                    scsiLBA2MSF(aATAPICmd + 6, iATAPILBA + cReqSectors);
                    break;
            }
            rc = s->pDrvMedia->pfnSendCmd(s->pDrvMedia, aATAPICmd, ATAPI_PACKET_SIZE, (PDMMEDIATXDIR)s->uTxDir,
                                          pbBuf, &cbCurrTX, abATAPISense, sizeof(abATAPISense), 30000 /**< @todo timeout */);
            if (rc != VINF_SUCCESS)
                break;
            iATAPILBA += cReqSectors;
            pbBuf += s->cbATAPISector * cReqSectors;
        }

        if (RT_SUCCESS(rc))
        {
            /* Adjust ATAPI command for the next call. */
            switch (s->aATAPICmd[0])
            {
                case SCSI_READ_10:
                case SCSI_WRITE_10:
                case SCSI_WRITE_AND_VERIFY_10:
                    scsiH2BE_U32(s->aATAPICmd + 2, iATAPILBA);
                    scsiH2BE_U16(s->aATAPICmd + 7, cSectors - cSectorsMax);
                    break;
                case SCSI_READ_12:
                case SCSI_WRITE_12:
                    scsiH2BE_U32(s->aATAPICmd + 2, iATAPILBA);
                    scsiH2BE_U32(s->aATAPICmd + 6, cSectors - cSectorsMax);
                    break;
                case SCSI_READ_CD:
                    scsiH2BE_U32(s->aATAPICmd + 2, iATAPILBA);
                    scsiH2BE_U24(s->aATAPICmd + 6, cSectors - cSectorsMax);
                    break;
                case SCSI_READ_CD_MSF:
                    scsiLBA2MSF(s->aATAPICmd + 3, iATAPILBA);
                    scsiLBA2MSF(s->aATAPICmd + 6, iATAPILBA + cSectors - cSectorsMax);
                    break;
                default:
                    AssertMsgFailed(("Don't know how to split command %#04x\n", s->aATAPICmd[0]));
                    if (s->cErrors++ < MAX_LOG_REL_ERRORS)
                        LogRel(("PIIX3 ATA: LUN#%d: CD-ROM passthrough split error\n", s->iLUN));
                    atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_ILLEGAL_OPCODE);
                    return false;
            }
        }
    }
    else
        rc = s->pDrvMedia->pfnSendCmd(s->pDrvMedia, s->aATAPICmd, ATAPI_PACKET_SIZE, (PDMMEDIATXDIR)s->uTxDir,
                                      s->CTX_SUFF(pbIOBuffer), &cbTransfer, abATAPISense, sizeof(abATAPISense), 30000 /**< @todo timeout */);
    if (pProf) { STAM_PROFILE_ADV_STOP(pProf, b); }

    ataR3LockEnter(pCtl);

    /* Update the LEDs and the read/write statistics. */
    if (cbTransfer >= 2048)
    {
        if (s->uTxDir != PDMMEDIATXDIR_TO_DEVICE)
        {
            s->Led.Actual.s.fReading = 0;
            STAM_REL_COUNTER_ADD(&s->StatBytesRead, cbTransfer);
        }
        else
        {
            s->Led.Actual.s.fWriting = 0;
            STAM_REL_COUNTER_ADD(&s->StatBytesWritten, cbTransfer);
        }
    }

    if (RT_SUCCESS(rc))
    {
        /* Do post processing for certain commands. */
        switch (s->aATAPICmd[0])
        {
            case SCSI_SEND_CUE_SHEET:
            case SCSI_READ_TOC_PMA_ATIP:
            {
                if (!s->pTrackList)
                    rc = ATAPIPassthroughTrackListCreateEmpty(&s->pTrackList);

                if (RT_SUCCESS(rc))
                    rc = ATAPIPassthroughTrackListUpdate(s->pTrackList, s->aATAPICmd, s->CTX_SUFF(pbIOBuffer));

                if (   RT_FAILURE(rc)
                    && s->cErrors++ < MAX_LOG_REL_ERRORS)
                    LogRel(("ATA: Error (%Rrc) while updating the tracklist during %s, burning the disc might fail\n",
                            rc, s->aATAPICmd[0] == SCSI_SEND_CUE_SHEET ? "SEND CUE SHEET" : "READ TOC/PMA/ATIP"));
                break;
            }
            case SCSI_SYNCHRONIZE_CACHE:
            {
                if (s->pTrackList)
                    ATAPIPassthroughTrackListClear(s->pTrackList);
                break;
            }
        }

        if (s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE)
        {
            /*
             * Reply with the same amount of data as the real drive
             * but only if the command wasn't split.
             */
            if (s->cbAtapiPassthroughTransfer < s->cbIOBuffer)
                s->cbTotalTransfer = cbTransfer;

            if (   s->aATAPICmd[0] == SCSI_INQUIRY
                && s->fOverwriteInquiry)
            {
                /* Make sure that the real drive cannot be identified.
                 * Motivation: changing the VM configuration should be as
                 *             invisible as possible to the guest. */
                Log3(("ATAPI PT inquiry data before (%d): %.*Rhxs\n", cbTransfer, cbTransfer, s->CTX_SUFF(pbIOBuffer)));
                scsiPadStr(s->CTX_SUFF(pbIOBuffer) + 8, "VBOX", 8);
                scsiPadStr(s->CTX_SUFF(pbIOBuffer) + 16, "CD-ROM", 16);
                scsiPadStr(s->CTX_SUFF(pbIOBuffer) + 32, "1.0", 4);
            }

            if (cbTransfer)
                Log3(("ATAPI PT data read (%d):\n%.*Rhxd\n", cbTransfer, cbTransfer, s->CTX_SUFF(pbIOBuffer)));
        }

        /* The initial buffer end value has been set up based on the total
         * transfer size. But the I/O buffer size limits what can actually be
         * done in one transfer, so set the actual value of the buffer end. */
        Assert(cbTransfer <= s->cbAtapiPassthroughTransfer);
        s->cbElementaryTransfer        = cbTransfer;
        s->cbAtapiPassthroughTransfer -= cbTransfer;
        if (!s->cbAtapiPassthroughTransfer)
        {
            s->iSourceSink = ATAFN_SS_NULL;
            atapiR3CmdOK(s);
        }
    }
    else
    {
        if (s->cErrors < MAX_LOG_REL_ERRORS)
        {
            uint8_t u8Cmd = s->aATAPICmd[0];
            do
            {
                /* don't log superfluous errors */
                if (    rc == VERR_DEV_IO_ERROR
                    && (   u8Cmd == SCSI_TEST_UNIT_READY
                        || u8Cmd == SCSI_READ_CAPACITY
                        || u8Cmd == SCSI_READ_DVD_STRUCTURE
                        || u8Cmd == SCSI_READ_TOC_PMA_ATIP))
                    break;
                s->cErrors++;
                LogRel(("PIIX3 ATA: LUN#%d: CD-ROM passthrough cmd=%#04x sense=%d ASC=%#02x ASCQ=%#02x %Rrc\n",
                            s->iLUN, u8Cmd, abATAPISense[2] & 0x0f, abATAPISense[12], abATAPISense[13], rc));
            } while (0);
        }
        atapiR3CmdError(s, abATAPISense, sizeof(abATAPISense));
    }
    return false;
}

/** @todo Revise ASAP. */
static bool atapiR3ReadDVDStructureSS(ATADevState *s)
{
    uint8_t *buf = s->CTX_SUFF(pbIOBuffer);
    int media = s->aATAPICmd[1];
    int format = s->aATAPICmd[7];

    uint16_t max_len = scsiBE2H_U16(&s->aATAPICmd[8]);

    memset(buf, 0, max_len);

    switch (format) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b:
        case 0x0c:
        case 0x0d:
        case 0x0e:
        case 0x0f:
        case 0x10:
        case 0x11:
        case 0x30:
        case 0x31:
        case 0xff:
            if (media == 0)
            {
                int uASC = SCSI_ASC_NONE;

                switch (format)
                {
                    case 0x0: /* Physical format information */
                    {
                        int layer = s->aATAPICmd[6];
                        uint64_t total_sectors;

                        if (layer != 0)
                        {
                            uASC = -SCSI_ASC_INV_FIELD_IN_CMD_PACKET;
                            break;
                        }

                        total_sectors = s->cTotalSectors;
                        total_sectors >>= 2;
                        if (total_sectors == 0)
                        {
                            uASC = -SCSI_ASC_MEDIUM_NOT_PRESENT;
                            break;
                        }

                        buf[4] = 1;   /* DVD-ROM, part version 1 */
                        buf[5] = 0xf; /* 120mm disc, minimum rate unspecified */
                        buf[6] = 1;   /* one layer, read-only (per MMC-2 spec) */
                        buf[7] = 0;   /* default densities */

                        /* FIXME: 0x30000 per spec? */
                        scsiH2BE_U32(buf + 8, 0); /* start sector */
                        scsiH2BE_U32(buf + 12, total_sectors - 1); /* end sector */
                        scsiH2BE_U32(buf + 16, total_sectors - 1); /* l0 end sector */

                        /* Size of buffer, not including 2 byte size field */
                        scsiH2BE_U32(&buf[0], 2048 + 2);

                        /* 2k data + 4 byte header */
                        uASC = (2048 + 4);
                        break;
                    }
                    case 0x01: /* DVD copyright information */
                        buf[4] = 0; /* no copyright data */
                        buf[5] = 0; /* no region restrictions */

                        /* Size of buffer, not including 2 byte size field */
                        scsiH2BE_U16(buf, 4 + 2);

                        /* 4 byte header + 4 byte data */
                        uASC = (4 + 4);
                        break;

                    case 0x03: /* BCA information - invalid field for no BCA info */
                        uASC = -SCSI_ASC_INV_FIELD_IN_CMD_PACKET;
                        break;

                    case 0x04: /* DVD disc manufacturing information */
                        /* Size of buffer, not including 2 byte size field */
                        scsiH2BE_U16(buf, 2048 + 2);

                        /* 2k data + 4 byte header */
                        uASC = (2048 + 4);
                        break;
                    case 0xff:
                        /*
                         * This lists all the command capabilities above.  Add new ones
                         * in order and update the length and buffer return values.
                         */

                        buf[4] = 0x00; /* Physical format */
                        buf[5] = 0x40; /* Not writable, is readable */
                        scsiH2BE_U16((buf + 6), 2048 + 4);

                        buf[8] = 0x01; /* Copyright info */
                        buf[9] = 0x40; /* Not writable, is readable */
                        scsiH2BE_U16((buf + 10), 4 + 4);

                        buf[12] = 0x03; /* BCA info */
                        buf[13] = 0x40; /* Not writable, is readable */
                        scsiH2BE_U16((buf + 14), 188 + 4);

                        buf[16] = 0x04; /* Manufacturing info */
                        buf[17] = 0x40; /* Not writable, is readable */
                        scsiH2BE_U16((buf + 18), 2048 + 4);

                        /* Size of buffer, not including 2 byte size field */
                        scsiH2BE_U16(buf, 16 + 2);

                        /* data written + 4 byte header */
                        uASC = (16 + 4);
                        break;
                    default: /** @todo formats beyond DVD-ROM requires */
                        uASC = -SCSI_ASC_INV_FIELD_IN_CMD_PACKET;
                }

                if (uASC < 0)
                {
                    s->iSourceSink = ATAFN_SS_NULL;
                    atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, -uASC);
                    return false;
                }
                break;
            }
            /** @todo BD support, fall through for now */
            RT_FALL_THRU();

        /* Generic disk structures */
        case 0x80: /** @todo AACS volume identifier */
        case 0x81: /** @todo AACS media serial number */
        case 0x82: /** @todo AACS media identifier */
        case 0x83: /** @todo AACS media key block */
        case 0x90: /** @todo List of recognized format layers */
        case 0xc0: /** @todo Write protection status */
        default:
            s->iSourceSink = ATAFN_SS_NULL;
            atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST,
                                SCSI_ASC_INV_FIELD_IN_CMD_PACKET);
            return false;
    }

    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}

static bool atapiR3ReadSectors(ATADevState *s, uint32_t iATAPILBA, uint32_t cSectors, uint32_t cbSector)
{
    Assert(cSectors > 0);
    s->iATAPILBA = iATAPILBA;
    s->cbATAPISector = cbSector;
    ataR3StartTransfer(s, cSectors * cbSector, PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_READ, true);
    return false;
}


static bool atapiR3ReadCapacitySS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    Assert(s->cbElementaryTransfer <= 8);
    scsiH2BE_U32(pbBuf, s->cTotalSectors - 1);
    scsiH2BE_U32(pbBuf + 4, 2048);
    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}


static bool atapiR3ReadDiscInformationSS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    Assert(s->cbElementaryTransfer <= 34);
    memset(pbBuf, '\0', 34);
    scsiH2BE_U16(pbBuf, 32);
    pbBuf[2] = (0 << 4) | (3 << 2) | (2 << 0); /* not erasable, complete session, complete disc */
    pbBuf[3] = 1; /* number of first track */
    pbBuf[4] = 1; /* number of sessions (LSB) */
    pbBuf[5] = 1; /* first track number in last session (LSB) */
    pbBuf[6] = (uint8_t)s->pDrvMedia->pfnGetRegionCount(s->pDrvMedia); /* last track number in last session (LSB) */
    pbBuf[7] = (0 << 7) | (0 << 6) | (1 << 5) | (0 << 2) | (0 << 0); /* disc id not valid, disc bar code not valid, unrestricted use, not dirty, not RW medium */
    pbBuf[8] = 0; /* disc type = CD-ROM */
    pbBuf[9] = 0; /* number of sessions (MSB) */
    pbBuf[10] = 0; /* number of sessions (MSB) */
    pbBuf[11] = 0; /* number of sessions (MSB) */
    scsiH2BE_U32(pbBuf + 16, 0xffffffff); /* last session lead-in start time is not available */
    scsiH2BE_U32(pbBuf + 20, 0xffffffff); /* last possible start time for lead-out is not available */
    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}


static bool atapiR3ReadTrackInformationSS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);
    uint32_t u32LogAddr = scsiBE2H_U32(&s->aATAPICmd[2]);
    uint8_t u8LogAddrType = s->aATAPICmd[1] & 0x03;

    int rc = VINF_SUCCESS;
    uint64_t u64LbaStart = 0;
    uint32_t uRegion = 0;
    uint64_t cBlocks = 0;
    uint64_t cbBlock = 0;
    uint8_t u8DataMode = 0xf; /* Unknown data mode. */
    uint8_t u8TrackMode = 0;
    VDREGIONDATAFORM enmDataForm = VDREGIONDATAFORM_INVALID;

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    Assert(s->cbElementaryTransfer <= 36);

    switch (u8LogAddrType)
    {
        case 0x00:
            rc = s->pDrvMedia->pfnQueryRegionPropertiesForLba(s->pDrvMedia, u32LogAddr, &uRegion,
                                                              NULL, NULL, NULL);
            if (RT_SUCCESS(rc))
                rc = s->pDrvMedia->pfnQueryRegionProperties(s->pDrvMedia, uRegion, &u64LbaStart,
                                                            &cBlocks, &cbBlock, &enmDataForm);
            break;
        case 0x01:
        {
            if (u32LogAddr >= 1)
            {
                uRegion = u32LogAddr - 1;
                rc = s->pDrvMedia->pfnQueryRegionProperties(s->pDrvMedia, uRegion, &u64LbaStart,
                                                            &cBlocks, &cbBlock, &enmDataForm);
            }
            else
                rc = VERR_NOT_FOUND; /** @todo Return lead-in information. */
            break;
        }
        case 0x02:
        default:
            atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_INV_FIELD_IN_CMD_PACKET);
            return false;
    }

    if (RT_FAILURE(rc))
    {
        atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_INV_FIELD_IN_CMD_PACKET);
        return false;
    }

    switch (enmDataForm)
    {
        case VDREGIONDATAFORM_MODE1_2048:
        case VDREGIONDATAFORM_MODE1_2352:
        case VDREGIONDATAFORM_MODE1_0:
            u8DataMode = 1;
            break;
        case VDREGIONDATAFORM_XA_2336:
        case VDREGIONDATAFORM_XA_2352:
        case VDREGIONDATAFORM_XA_0:
        case VDREGIONDATAFORM_MODE2_2336:
        case VDREGIONDATAFORM_MODE2_2352:
        case VDREGIONDATAFORM_MODE2_0:
            u8DataMode = 2;
            break;
        default:
            u8DataMode = 0xf;
    }

    if (enmDataForm == VDREGIONDATAFORM_CDDA)
        u8TrackMode = 0x0;
    else
        u8TrackMode = 0x4;

    memset(pbBuf, '\0', 36);
    scsiH2BE_U16(pbBuf, 34);
    pbBuf[2] = uRegion + 1;                                            /* track number (LSB) */
    pbBuf[3] = 1;                                                      /* session number (LSB) */
    pbBuf[5] = (0 << 5) | (0 << 4) | u8TrackMode;                      /* not damaged, primary copy, data track */
    pbBuf[6] = (0 << 7) | (0 << 6) | (0 << 5) | (0 << 6) | u8DataMode; /* not reserved track, not blank, not packet writing, not fixed packet */
    pbBuf[7] = (0 << 1) | (0 << 0);                                    /* last recorded address not valid, next recordable address not valid */
    scsiH2BE_U32(pbBuf + 8, (uint32_t)u64LbaStart);                    /* track start address is 0 */
    scsiH2BE_U32(pbBuf + 24, (uint32_t)cBlocks);                       /* track size */
    pbBuf[32] = 0;                                                     /* track number (MSB) */
    pbBuf[33] = 0;                                                     /* session number (MSB) */
    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}

static DECLCALLBACK(uint32_t) atapiR3GetConfigurationFillFeatureListProfiles(ATADevState *s, uint8_t *pbBuf, size_t cbBuf)
{
    RT_NOREF1(s);
    if (cbBuf < 3*4)
        return 0;

    scsiH2BE_U16(pbBuf, 0x0); /* feature 0: list of profiles supported */
    pbBuf[2] = (0 << 2) | (1 << 1) | (1 << 0); /* version 0, persistent, current */
    pbBuf[3] = 8; /* additional bytes for profiles */
    /* The MMC-3 spec says that DVD-ROM read capability should be reported
     * before CD-ROM read capability. */
    scsiH2BE_U16(pbBuf + 4, 0x10); /* profile: read-only DVD */
    pbBuf[6] = (0 << 0); /* NOT current profile */
    scsiH2BE_U16(pbBuf + 8, 0x08); /* profile: read only CD */
    pbBuf[10] = (1 << 0); /* current profile */

    return 3*4; /* Header + 2 profiles entries */
}

static DECLCALLBACK(uint32_t) atapiR3GetConfigurationFillFeatureCore(ATADevState *s, uint8_t *pbBuf, size_t cbBuf)
{
    RT_NOREF1(s);
    if (cbBuf < 12)
        return 0;

    scsiH2BE_U16(pbBuf, 0x1); /* feature 0001h: Core Feature */
    pbBuf[2] = (0x2 << 2) | RT_BIT(1) | RT_BIT(0); /* Version | Persistent | Current */
    pbBuf[3] = 8; /* Additional length */
    scsiH2BE_U16(pbBuf + 4, 0x00000002); /* Physical interface ATAPI. */
    pbBuf[8] = RT_BIT(0); /* DBE */
    /* Rest is reserved. */

    return 12;
}

static DECLCALLBACK(uint32_t) atapiR3GetConfigurationFillFeatureMorphing(ATADevState *s, uint8_t *pbBuf, size_t cbBuf)
{
    RT_NOREF1(s);
    if (cbBuf < 8)
        return 0;

    scsiH2BE_U16(pbBuf, 0x2); /* feature 0002h: Morphing Feature */
    pbBuf[2] = (0x1 << 2) | RT_BIT(1) | RT_BIT(0); /* Version | Persistent | Current */
    pbBuf[3] = 4; /* Additional length */
    pbBuf[4] = RT_BIT(1) | 0x0; /* OCEvent | !ASYNC */
    /* Rest is reserved. */

    return 8;
}

static DECLCALLBACK(uint32_t) atapiR3GetConfigurationFillFeatureRemovableMedium(ATADevState *s, uint8_t *pbBuf, size_t cbBuf)
{
    RT_NOREF1(s);
    if (cbBuf < 8)
        return 0;

    scsiH2BE_U16(pbBuf, 0x3); /* feature 0003h: Removable Medium Feature */
    pbBuf[2] = (0x2 << 2) | RT_BIT(1) | RT_BIT(0); /* Version | Persistent | Current */
    pbBuf[3] = 4; /* Additional length */
    /* Tray type loading | Load | Eject | !Pvnt Jmpr | !DBML | Lock */
    pbBuf[4] = (0x2 << 5) | RT_BIT(4) | RT_BIT(3) | (0x0 << 2) | (0x0 << 1) | RT_BIT(0);
    /* Rest is reserved. */

    return 8;
}

static DECLCALLBACK(uint32_t) atapiR3GetConfigurationFillFeatureRandomReadable (ATADevState *s, uint8_t *pbBuf, size_t cbBuf)
{
    RT_NOREF1(s);
    if (cbBuf < 12)
        return 0;

    scsiH2BE_U16(pbBuf, 0x10); /* feature 0010h: Random Readable Feature */
    pbBuf[2] = (0x0 << 2) | RT_BIT(1) | RT_BIT(0); /* Version | Persistent | Current */
    pbBuf[3] = 8; /* Additional length */
    scsiH2BE_U32(pbBuf + 4, 2048); /* Logical block size. */
    scsiH2BE_U16(pbBuf + 8, 0x10); /* Blocking (0x10 for DVD, CD is not defined). */
    pbBuf[10] = 0; /* PP not present */
    /* Rest is reserved. */

    return 12;
}

static DECLCALLBACK(uint32_t) atapiR3GetConfigurationFillFeatureCDRead(ATADevState *s, uint8_t *pbBuf, size_t cbBuf)
{
    RT_NOREF1(s);
    if (cbBuf < 8)
        return 0;

    scsiH2BE_U16(pbBuf, 0x1e); /* feature 001Eh: CD Read Feature */
    pbBuf[2] = (0x2 << 2) | RT_BIT(1) | RT_BIT(0); /* Version | Persistent | Current */
    pbBuf[3] = 0; /* Additional length */
    pbBuf[4] = (0x0 << 7) | (0x0 << 1) | 0x0; /* !DAP | !C2-Flags | !CD-Text. */
    /* Rest is reserved. */

    return 8;
}

static DECLCALLBACK(uint32_t) atapiR3GetConfigurationFillFeaturePowerManagement(ATADevState *s, uint8_t *pbBuf, size_t cbBuf)
{
    RT_NOREF1(s);
    if (cbBuf < 4)
        return 0;

    scsiH2BE_U16(pbBuf, 0x100); /* feature 0100h: Power Management Feature */
    pbBuf[2] = (0x0 << 2) | RT_BIT(1) | RT_BIT(0); /* Version | Persistent | Current */
    pbBuf[3] = 0; /* Additional length */

    return 4;
}

static DECLCALLBACK(uint32_t) atapiR3GetConfigurationFillFeatureTimeout(ATADevState *s, uint8_t *pbBuf, size_t cbBuf)
{
    RT_NOREF1(s);
    if (cbBuf < 8)
        return 0;

    scsiH2BE_U16(pbBuf, 0x105); /* feature 0105h: Timeout Feature */
    pbBuf[2] = (0x0 << 2) | RT_BIT(1) | RT_BIT(0); /* Version | Persistent | Current */
    pbBuf[3] = 4; /* Additional length */
    pbBuf[4] = 0x0; /* !Group3 */

    return 8;
}

/**
 * Callback to fill in the correct data for a feature.
 *
 * @returns Number of bytes written into the buffer.
 * @param   s       The ATA device state.
 * @param   pbBuf   The buffer to fill the data with.
 * @param   cbBuf   Size of the buffer.
 */
typedef DECLCALLBACK(uint32_t) FNATAPIR3FEATUREFILL(ATADevState *s, uint8_t *pbBuf, size_t cbBuf);
/** Pointer to a feature fill callback. */
typedef FNATAPIR3FEATUREFILL *PFNATAPIR3FEATUREFILL;

/**
 * ATAPI feature descriptor.
 */
typedef struct ATAPIR3FEATDESC
{
    /** The feature number. */
    uint16_t u16Feat;
    /** The callback to fill in the correct data. */
    PFNATAPIR3FEATUREFILL pfnFeatureFill;
} ATAPIR3FEATDESC;

/**
 * Array of known ATAPI feature descriptors.
 */
static const ATAPIR3FEATDESC s_aAtapiR3Features[] =
{
    { 0x0000, atapiR3GetConfigurationFillFeatureListProfiles},
    { 0x0001, atapiR3GetConfigurationFillFeatureCore},
    { 0x0002, atapiR3GetConfigurationFillFeatureMorphing},
    { 0x0003, atapiR3GetConfigurationFillFeatureRemovableMedium},
    { 0x0010, atapiR3GetConfigurationFillFeatureRandomReadable},
    { 0x001e, atapiR3GetConfigurationFillFeatureCDRead},
    { 0x0100, atapiR3GetConfigurationFillFeaturePowerManagement},
    { 0x0105, atapiR3GetConfigurationFillFeatureTimeout}
};

static bool atapiR3GetConfigurationSS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);
    uint32_t cbBuf = s->cbIOBuffer;
    uint32_t cbCopied = 0;
    uint16_t u16Sfn = scsiBE2H_U16(&s->aATAPICmd[2]);
    uint8_t u8Rt = s->aATAPICmd[1] & 0x03;

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    Assert(s->cbElementaryTransfer <= 80);
    /* Accept valid request types only. */
    if (u8Rt == 3)
    {
        atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_INV_FIELD_IN_CMD_PACKET);
        return false;
    }
    memset(pbBuf, '\0', cbBuf);
    /** @todo implement switching between CD-ROM and DVD-ROM profile (the only
     * way to differentiate them right now is based on the image size). */
    if (s->cTotalSectors)
        scsiH2BE_U16(pbBuf + 6, 0x08); /* current profile: read-only CD */
    else
        scsiH2BE_U16(pbBuf + 6, 0x00); /* current profile: none -> no media */
    cbBuf    -= 8;
    pbBuf    += 8;

    if (u8Rt == 0x2)
    {
        for (uint32_t i = 0; i < RT_ELEMENTS(s_aAtapiR3Features); i++)
        {
            if (s_aAtapiR3Features[i].u16Feat == u16Sfn)
            {
                cbCopied = s_aAtapiR3Features[i].pfnFeatureFill(s, pbBuf, cbBuf);
                cbBuf -= cbCopied;
                pbBuf += cbCopied;
                break;
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i < RT_ELEMENTS(s_aAtapiR3Features); i++)
        {
            if (s_aAtapiR3Features[i].u16Feat > u16Sfn)
            {
                cbCopied = s_aAtapiR3Features[i].pfnFeatureFill(s, pbBuf, cbBuf);
                cbBuf -= cbCopied;
                pbBuf += cbCopied;
            }
        }
    }

    /* Set data length now - the field is not included in the final length. */
    scsiH2BE_U32(s->CTX_SUFF(pbIOBuffer), s->cbIOBuffer - cbBuf - 4);

    /* Other profiles we might want to add in the future: 0x40 (BD-ROM) and 0x50 (HDDVD-ROM) */
    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}


static bool atapiR3GetEventStatusNotificationSS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    Assert(s->cbElementaryTransfer <= 8);

    if (!(s->aATAPICmd[1] & 1))
    {
        /* no asynchronous operation supported */
        atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_INV_FIELD_IN_CMD_PACKET);
        return false;
    }

    uint32_t OldStatus, NewStatus;
    do
    {
        OldStatus = ASMAtomicReadU32(&s->MediaEventStatus);
        NewStatus = ATA_EVENT_STATUS_UNCHANGED;
        switch (OldStatus)
        {
            case ATA_EVENT_STATUS_MEDIA_NEW:
                /* mount */
                scsiH2BE_U16(pbBuf + 0, 6);
                pbBuf[2] = 0x04; /* media */
                pbBuf[3] = 0x5e; /* supported = busy|media|external|power|operational */
                pbBuf[4] = 0x02; /* new medium */
                pbBuf[5] = 0x02; /* medium present / door closed */
                pbBuf[6] = 0x00;
                pbBuf[7] = 0x00;
                break;

            case ATA_EVENT_STATUS_MEDIA_CHANGED:
            case ATA_EVENT_STATUS_MEDIA_REMOVED:
                /* umount */
                scsiH2BE_U16(pbBuf + 0, 6);
                pbBuf[2] = 0x04; /* media */
                pbBuf[3] = 0x5e; /* supported = busy|media|external|power|operational */
                pbBuf[4] = 0x03; /* media removal */
                pbBuf[5] = 0x00; /* medium absent / door closed */
                pbBuf[6] = 0x00;
                pbBuf[7] = 0x00;
                if (OldStatus == ATA_EVENT_STATUS_MEDIA_CHANGED)
                    NewStatus = ATA_EVENT_STATUS_MEDIA_NEW;
                break;

            case ATA_EVENT_STATUS_MEDIA_EJECT_REQUESTED: /* currently unused */
                scsiH2BE_U16(pbBuf + 0, 6);
                pbBuf[2] = 0x04; /* media */
                pbBuf[3] = 0x5e; /* supported = busy|media|external|power|operational */
                pbBuf[4] = 0x01; /* eject requested (eject button pressed) */
                pbBuf[5] = 0x02; /* medium present / door closed */
                pbBuf[6] = 0x00;
                pbBuf[7] = 0x00;
                break;

            case ATA_EVENT_STATUS_UNCHANGED:
            default:
                scsiH2BE_U16(pbBuf + 0, 6);
                pbBuf[2] = 0x01; /* operational change request / notification */
                pbBuf[3] = 0x5e; /* supported = busy|media|external|power|operational */
                pbBuf[4] = 0x00;
                pbBuf[5] = 0x00;
                pbBuf[6] = 0x00;
                pbBuf[7] = 0x00;
                break;
        }
    } while (!ASMAtomicCmpXchgU32(&s->MediaEventStatus, NewStatus, OldStatus));

    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}


static bool atapiR3InquirySS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    Assert(s->cbElementaryTransfer <= 36);
    pbBuf[0] = 0x05; /* CD-ROM */
    pbBuf[1] = 0x80; /* removable */
# if 1/*ndef VBOX*/  /** @todo implement MESN + AENC. (async notification on removal and stuff.) */
    pbBuf[2] = 0x00; /* ISO */
    pbBuf[3] = 0x21; /* ATAPI-2 (XXX: put ATAPI-4 ?) */
# else
    pbBuf[2] = 0x00; /* ISO */
    pbBuf[3] = 0x91; /* format 1, MESN=1, AENC=9 ??? */
# endif
    pbBuf[4] = 31; /* additional length */
    pbBuf[5] = 0; /* reserved */
    pbBuf[6] = 0; /* reserved */
    pbBuf[7] = 0; /* reserved */
    scsiPadStr(pbBuf + 8, s->szInquiryVendorId, 8);
    scsiPadStr(pbBuf + 16, s->szInquiryProductId, 16);
    scsiPadStr(pbBuf + 32, s->szInquiryRevision, 4);
    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}


static bool atapiR3ModeSenseErrorRecoverySS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    Assert(s->cbElementaryTransfer <= 16);
    scsiH2BE_U16(&pbBuf[0], 16 + 6);
    pbBuf[2] = (uint8_t)s->MediaTrackType;
    pbBuf[3] = 0;
    pbBuf[4] = 0;
    pbBuf[5] = 0;
    pbBuf[6] = 0;
    pbBuf[7] = 0;

    pbBuf[8] = 0x01;
    pbBuf[9] = 0x06;
    pbBuf[10] = 0x00;   /* Maximum error recovery */
    pbBuf[11] = 0x05;   /* 5 retries */
    pbBuf[12] = 0x00;
    pbBuf[13] = 0x00;
    pbBuf[14] = 0x00;
    pbBuf[15] = 0x00;
    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}


static bool atapiR3ModeSenseCDStatusSS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    Assert(s->cbElementaryTransfer <= 40);
    scsiH2BE_U16(&pbBuf[0], 38);
    pbBuf[2] = (uint8_t)s->MediaTrackType;
    pbBuf[3] = 0;
    pbBuf[4] = 0;
    pbBuf[5] = 0;
    pbBuf[6] = 0;
    pbBuf[7] = 0;

    pbBuf[8] = 0x2a;
    pbBuf[9] = 30; /* page length */
    pbBuf[10] = 0x08; /* DVD-ROM read support */
    pbBuf[11] = 0x00; /* no write support */
    /* The following claims we support audio play. This is obviously false,
     * but the Linux generic CDROM support makes many features depend on this
     * capability. If it's not set, this causes many things to be disabled. */
    pbBuf[12] = 0x71; /* multisession support, mode 2 form 1/2 support, audio play */
    pbBuf[13] = 0x00; /* no subchannel reads supported */
    pbBuf[14] = (1 << 0) | (1 << 3) | (1 << 5); /* lock supported, eject supported, tray type loading mechanism */
    if (s->pDrvMount->pfnIsLocked(s->pDrvMount))
        pbBuf[14] |= 1 << 1; /* report lock state */
    pbBuf[15] = 0; /* no subchannel reads supported, no separate audio volume control, no changer etc. */
    scsiH2BE_U16(&pbBuf[16], 5632); /* (obsolete) claim 32x speed support */
    scsiH2BE_U16(&pbBuf[18], 2); /* number of audio volume levels */
    scsiH2BE_U16(&pbBuf[20], s->cbIOBuffer / _1K); /* buffer size supported in Kbyte */
    scsiH2BE_U16(&pbBuf[22], 5632); /* (obsolete) current read speed 32x */
    pbBuf[24] = 0; /* reserved */
    pbBuf[25] = 0; /* reserved for digital audio (see idx 15) */
    scsiH2BE_U16(&pbBuf[26], 0); /* (obsolete) maximum write speed */
    scsiH2BE_U16(&pbBuf[28], 0); /* (obsolete) current write speed */
    scsiH2BE_U16(&pbBuf[30], 0); /* copy management revision supported 0=no CSS */
    pbBuf[32] = 0; /* reserved */
    pbBuf[33] = 0; /* reserved */
    pbBuf[34] = 0; /* reserved */
    pbBuf[35] = 1; /* rotation control CAV */
    scsiH2BE_U16(&pbBuf[36], 0); /* current write speed */
    scsiH2BE_U16(&pbBuf[38], 0); /* number of write speed performance descriptors */
    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}


static bool atapiR3RequestSenseSS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    memset(pbBuf, '\0', s->cbElementaryTransfer);
    memcpy(pbBuf, s->abATAPISense, RT_MIN(s->cbElementaryTransfer, sizeof(s->abATAPISense)));
    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}


static bool atapiR3MechanismStatusSS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    Assert(s->cbElementaryTransfer <= 8);
    scsiH2BE_U16(pbBuf, 0);
    /* no current LBA */
    pbBuf[2] = 0;
    pbBuf[3] = 0;
    pbBuf[4] = 0;
    pbBuf[5] = 1;
    scsiH2BE_U16(pbBuf + 6, 0);
    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}


static bool atapiR3ReadTOCNormalSS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer), *q, iStartTrack;
    bool fMSF;
    uint32_t cbSize;
    uint32_t cTracks = s->pDrvMedia->pfnGetRegionCount(s->pDrvMedia);

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    fMSF = (s->aATAPICmd[1] >> 1) & 1;
    iStartTrack = s->aATAPICmd[6];
    if (iStartTrack == 0)
        iStartTrack = 1;

    if (iStartTrack > cTracks && iStartTrack != 0xaa)
    {
        atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_INV_FIELD_IN_CMD_PACKET);
        return false;
    }
    q = pbBuf + 2;
    *q++ = iStartTrack; /* first track number */
    *q++ = cTracks;     /* last track number */
    for (uint32_t iTrack = iStartTrack; iTrack <= cTracks; iTrack++)
    {
        uint64_t uLbaStart = 0;
        VDREGIONDATAFORM enmDataForm = VDREGIONDATAFORM_MODE1_2048;

        int rc = s->pDrvMedia->pfnQueryRegionProperties(s->pDrvMedia, iTrack - 1, &uLbaStart,
                                                        NULL, NULL, &enmDataForm);
        AssertRC(rc);

        *q++ = 0;                  /* reserved */

        if (enmDataForm == VDREGIONDATAFORM_CDDA)
            *q++ = 0x10;           /* ADR, control */
        else
            *q++ = 0x14;           /* ADR, control */

        *q++ = (uint8_t)iTrack;    /* track number */
        *q++ = 0;                  /* reserved */
        if (fMSF)
        {
            *q++ = 0; /* reserved */
            scsiLBA2MSF(q, (uint32_t)uLbaStart);
            q += 3;
        }
        else
        {
            /* sector 0 */
            scsiH2BE_U32(q, (uint32_t)uLbaStart);
            q += 4;
        }
    }
    /* lead out track */
    *q++ = 0; /* reserved */
    *q++ = 0x14; /* ADR, control */
    *q++ = 0xaa; /* track number */
    *q++ = 0; /* reserved */

    /* Query start and length of last track to get the start of the lead out track. */
    uint64_t uLbaStart = 0;
    uint64_t cBlocks = 0;

    int rc = s->pDrvMedia->pfnQueryRegionProperties(s->pDrvMedia, cTracks - 1, &uLbaStart,
                                                    &cBlocks, NULL, NULL);
    AssertRC(rc);

    uLbaStart += cBlocks;
    if (fMSF)
    {
        *q++ = 0; /* reserved */
        scsiLBA2MSF(q, (uint32_t)uLbaStart);
        q += 3;
    }
    else
    {
        scsiH2BE_U32(q, (uint32_t)uLbaStart);
        q += 4;
    }
    cbSize = q - pbBuf;
    scsiH2BE_U16(pbBuf, cbSize - 2);
    if (cbSize < s->cbTotalTransfer)
        s->cbTotalTransfer = cbSize;
    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}


static bool atapiR3ReadTOCMultiSS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer);
    bool fMSF;

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    Assert(s->cbElementaryTransfer <= 12);
    fMSF = (s->aATAPICmd[1] >> 1) & 1;
    /* multi session: only a single session defined */
    /** @todo double-check this stuff against what a real drive says for a CD-ROM (not a CD-R)
     * with only a single data session. Maybe solve the problem with "cdrdao read-toc" not being
     * able to figure out whether numbers are in BCD or hex. */
    memset(pbBuf, 0, 12);
    pbBuf[1] = 0x0a;
    pbBuf[2] = 0x01;
    pbBuf[3] = 0x01;

    VDREGIONDATAFORM enmDataForm = VDREGIONDATAFORM_MODE1_2048;
    int rc = s->pDrvMedia->pfnQueryRegionProperties(s->pDrvMedia, 0, NULL,
                                                    NULL, NULL, &enmDataForm);
    AssertRC(rc);

    if (enmDataForm == VDREGIONDATAFORM_CDDA)
        pbBuf[5] = 0x10;           /* ADR, control */
    else
        pbBuf[5] = 0x14;           /* ADR, control */

    pbBuf[6] = 1; /* first track in last complete session */
    if (fMSF)
    {
        pbBuf[8] = 0; /* reserved */
        scsiLBA2MSF(&pbBuf[9], 0);
    }
    else
    {
        /* sector 0 */
        scsiH2BE_U32(pbBuf + 8, 0);
    }
    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}


static bool atapiR3ReadTOCRawSS(ATADevState *s)
{
    uint8_t *pbBuf = s->CTX_SUFF(pbIOBuffer), *q, iStartTrack;
    bool fMSF;
    uint32_t cbSize;

    Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
    fMSF = (s->aATAPICmd[1] >> 1) & 1;
    iStartTrack = s->aATAPICmd[6];

    q = pbBuf + 2;
    *q++ = 1; /* first session */
    *q++ = 1; /* last session */

    *q++ = 1; /* session number */
    *q++ = 0x14; /* data track */
    *q++ = 0; /* track number */
    *q++ = 0xa0; /* first track in program area */
    *q++ = 0; /* min */
    *q++ = 0; /* sec */
    *q++ = 0; /* frame */
    *q++ = 0;
    *q++ = 1; /* first track */
    *q++ = 0x00; /* disk type CD-DA or CD data */
    *q++ = 0;

    *q++ = 1; /* session number */
    *q++ = 0x14; /* data track */
    *q++ = 0; /* track number */
    *q++ = 0xa1; /* last track in program area */
    *q++ = 0; /* min */
    *q++ = 0; /* sec */
    *q++ = 0; /* frame */
    *q++ = 0;
    *q++ = 1; /* last track */
    *q++ = 0;
    *q++ = 0;

    *q++ = 1; /* session number */
    *q++ = 0x14; /* data track */
    *q++ = 0; /* track number */
    *q++ = 0xa2; /* lead-out */
    *q++ = 0; /* min */
    *q++ = 0; /* sec */
    *q++ = 0; /* frame */
    if (fMSF)
    {
        *q++ = 0; /* reserved */
        scsiLBA2MSF(q, s->cTotalSectors);
        q += 3;
    }
    else
    {
        scsiH2BE_U32(q, s->cTotalSectors);
        q += 4;
    }

    *q++ = 1; /* session number */
    *q++ = 0x14; /* ADR, control */
    *q++ = 0;    /* track number */
    *q++ = 1;    /* point */
    *q++ = 0; /* min */
    *q++ = 0; /* sec */
    *q++ = 0; /* frame */
    if (fMSF)
    {
        *q++ = 0; /* reserved */
        scsiLBA2MSF(q, 0);
        q += 3;
    }
    else
    {
        /* sector 0 */
        scsiH2BE_U32(q, 0);
        q += 4;
    }

    cbSize = q - pbBuf;
    scsiH2BE_U16(pbBuf, cbSize - 2);
    if (cbSize < s->cbTotalTransfer)
        s->cbTotalTransfer = cbSize;
    s->iSourceSink = ATAFN_SS_NULL;
    atapiR3CmdOK(s);
    return false;
}


static void atapiR3ParseCmdVirtualATAPI(ATADevState *s)
{
    const uint8_t *pbPacket;
    uint8_t *pbBuf;
    uint32_t cbMax;

    pbPacket = s->aATAPICmd;
    pbBuf = s->CTX_SUFF(pbIOBuffer);
    switch (pbPacket[0])
    {
        case SCSI_TEST_UNIT_READY:
            if (s->cNotifiedMediaChange > 0)
            {
                if (s->cNotifiedMediaChange-- > 2)
                    atapiR3CmdErrorSimple(s, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT);
                else
                    atapiR3CmdErrorSimple(s, SCSI_SENSE_UNIT_ATTENTION, SCSI_ASC_MEDIUM_MAY_HAVE_CHANGED); /* media changed */
            }
            else if (s->pDrvMount->pfnIsMounted(s->pDrvMount))
                atapiR3CmdOK(s);
            else
                atapiR3CmdErrorSimple(s, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT);
            break;
        case SCSI_GET_EVENT_STATUS_NOTIFICATION:
            cbMax = scsiBE2H_U16(pbPacket + 7);
            ataR3StartTransfer(s, RT_MIN(cbMax, 8), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_GET_EVENT_STATUS_NOTIFICATION, true);
            break;
        case SCSI_MODE_SENSE_10:
        {
            uint8_t uPageControl, uPageCode;
            cbMax = scsiBE2H_U16(pbPacket + 7);
            uPageControl = pbPacket[2] >> 6;
            uPageCode = pbPacket[2] & 0x3f;
            switch (uPageControl)
            {
                case SCSI_PAGECONTROL_CURRENT:
                    switch (uPageCode)
                    {
                        case SCSI_MODEPAGE_ERROR_RECOVERY:
                            ataR3StartTransfer(s, RT_MIN(cbMax, 16), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_MODE_SENSE_ERROR_RECOVERY, true);
                            break;
                        case SCSI_MODEPAGE_CD_STATUS:
                            ataR3StartTransfer(s, RT_MIN(cbMax, 40), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_MODE_SENSE_CD_STATUS, true);
                            break;
                        default:
                            goto error_cmd;
                    }
                    break;
                case SCSI_PAGECONTROL_CHANGEABLE:
                    goto error_cmd;
                case SCSI_PAGECONTROL_DEFAULT:
                    goto error_cmd;
                default:
                case SCSI_PAGECONTROL_SAVED:
                    atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_SAVING_PARAMETERS_NOT_SUPPORTED);
                    break;
            }
            break;
        }
        case SCSI_REQUEST_SENSE:
            cbMax = pbPacket[4];
            ataR3StartTransfer(s, RT_MIN(cbMax, 18), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_REQUEST_SENSE, true);
            break;
        case SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL:
            if (s->pDrvMount->pfnIsMounted(s->pDrvMount))
            {
                if (pbPacket[4] & 1)
                    s->pDrvMount->pfnLock(s->pDrvMount);
                else
                    s->pDrvMount->pfnUnlock(s->pDrvMount);
                atapiR3CmdOK(s);
            }
            else
                atapiR3CmdErrorSimple(s, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT);
            break;
        case SCSI_READ_10:
        case SCSI_READ_12:
        {
            uint32_t cSectors, iATAPILBA;

            if (s->cNotifiedMediaChange > 0)
            {
                s->cNotifiedMediaChange-- ;
                atapiR3CmdErrorSimple(s, SCSI_SENSE_UNIT_ATTENTION, SCSI_ASC_MEDIUM_MAY_HAVE_CHANGED); /* media changed */
                break;
            }
            else if (!s->pDrvMount->pfnIsMounted(s->pDrvMount))
            {
                atapiR3CmdErrorSimple(s, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT);
                break;
            }
            if (pbPacket[0] == SCSI_READ_10)
                cSectors = scsiBE2H_U16(pbPacket + 7);
            else
                cSectors = scsiBE2H_U32(pbPacket + 6);
            iATAPILBA = scsiBE2H_U32(pbPacket + 2);

            /* Check that the sector size is valid. */
            VDREGIONDATAFORM enmDataForm = VDREGIONDATAFORM_INVALID;
            int rc = s->pDrvMedia->pfnQueryRegionPropertiesForLba(s->pDrvMedia, iATAPILBA,
                                                                  NULL, NULL, NULL, &enmDataForm);
            AssertRC(rc);
            if (   enmDataForm != VDREGIONDATAFORM_MODE1_2048
                && enmDataForm != VDREGIONDATAFORM_MODE1_2352
                && enmDataForm != VDREGIONDATAFORM_MODE2_2336
                && enmDataForm != VDREGIONDATAFORM_MODE2_2352
                && enmDataForm != VDREGIONDATAFORM_RAW)
            {
                uint8_t abATAPISense[ATAPI_SENSE_SIZE];
                RT_ZERO(abATAPISense);

                abATAPISense[0] = 0x70 | (1 << 7);
                abATAPISense[2] = (SCSI_SENSE_ILLEGAL_REQUEST & 0x0f) | SCSI_SENSE_FLAG_ILI;
                scsiH2BE_U32(&abATAPISense[3], iATAPILBA);
                abATAPISense[7] = 10;
                abATAPISense[12] = SCSI_ASC_ILLEGAL_MODE_FOR_THIS_TRACK;
                atapiR3CmdError(s, &abATAPISense[0], sizeof(abATAPISense));
                break;
            }

            if (cSectors == 0)
            {
                atapiR3CmdOK(s);
                break;
            }
            if ((uint64_t)iATAPILBA + cSectors > s->cTotalSectors)
            {
                /* Rate limited logging, one log line per second. For
                 * guests that insist on reading from places outside the
                 * valid area this often generates too many release log
                 * entries otherwise. */
                static uint64_t uLastLogTS = 0;
                if (RTTimeMilliTS() >= uLastLogTS + 1000)
                {
                    LogRel(("PIIX3 ATA: LUN#%d: CD-ROM block number %Ld invalid (READ)\n", s->iLUN, (uint64_t)iATAPILBA + cSectors));
                    uLastLogTS = RTTimeMilliTS();
                }
                atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_LOGICAL_BLOCK_OOR);
                break;
            }
            atapiR3ReadSectors(s, iATAPILBA, cSectors, 2048);
            break;
        }
        case SCSI_READ_CD:
        {
            uint32_t cSectors, iATAPILBA;

            if (s->cNotifiedMediaChange > 0)
            {
                s->cNotifiedMediaChange-- ;
                atapiR3CmdErrorSimple(s, SCSI_SENSE_UNIT_ATTENTION, SCSI_ASC_MEDIUM_MAY_HAVE_CHANGED); /* media changed */
                break;
            }
            else if (!s->pDrvMount->pfnIsMounted(s->pDrvMount))
            {
                atapiR3CmdErrorSimple(s, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT);
                break;
            }
            if ((pbPacket[10] & 0x7) != 0)
            {
                atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_INV_FIELD_IN_CMD_PACKET);
                break;
            }
            cSectors = (pbPacket[6] << 16) | (pbPacket[7] << 8) | pbPacket[8];
            iATAPILBA = scsiBE2H_U32(pbPacket + 2);
            if (cSectors == 0)
            {
                atapiR3CmdOK(s);
                break;
            }
            if ((uint64_t)iATAPILBA + cSectors > s->cTotalSectors)
            {
                /* Rate limited logging, one log line per second. For
                 * guests that insist on reading from places outside the
                 * valid area this often generates too many release log
                 * entries otherwise. */
                static uint64_t uLastLogTS = 0;
                if (RTTimeMilliTS() >= uLastLogTS + 1000)
                {
                    LogRel(("PIIX3 ATA: LUN#%d: CD-ROM block number %Ld invalid (READ CD)\n", s->iLUN, (uint64_t)iATAPILBA + cSectors));
                    uLastLogTS = RTTimeMilliTS();
                }
                atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_LOGICAL_BLOCK_OOR);
                break;
            }
            /*
             * If the LBA is in an audio track we are required to ignore pretty much all
             * of the channel selection values (except 0x00) and map everything to 0x10
             * which means read user data with a sector size of 2352 bytes.
             *
             * (MMC-6 chapter 6.19.2.6)
             */
            uint8_t uChnSel = pbPacket[9] & 0xf8;
            VDREGIONDATAFORM enmDataForm;
            int rc = s->pDrvMedia->pfnQueryRegionPropertiesForLba(s->pDrvMedia, iATAPILBA,
                                                                  NULL, NULL, NULL, &enmDataForm);
            AssertRC(rc);

            if (enmDataForm == VDREGIONDATAFORM_CDDA)
            {
                if (uChnSel == 0)
                {
                    /* nothing */
                    atapiR3CmdOK(s);
                }
                else
                    atapiR3ReadSectors(s, iATAPILBA, cSectors, 2352);
            }
            else
            {
                switch (uChnSel)
                {
                    case 0x00:
                        /* nothing */
                        atapiR3CmdOK(s);
                        break;
                    case 0x10:
                        /* normal read */
                        atapiR3ReadSectors(s, iATAPILBA, cSectors, 2048);
                        break;
                    case 0xf8:
                        /* read all data */
                        atapiR3ReadSectors(s, iATAPILBA, cSectors, 2352);
                        break;
                    default:
                        LogRel(("PIIX3 ATA: LUN#%d: CD-ROM sector format not supported (%#x)\n", s->iLUN, pbPacket[9] & 0xf8));
                        atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_INV_FIELD_IN_CMD_PACKET);
                        break;
                }
            }
            break;
        }
        case SCSI_SEEK_10:
        {
            uint32_t iATAPILBA;
            if (s->cNotifiedMediaChange > 0)
            {
                s->cNotifiedMediaChange-- ;
                atapiR3CmdErrorSimple(s, SCSI_SENSE_UNIT_ATTENTION, SCSI_ASC_MEDIUM_MAY_HAVE_CHANGED); /* media changed */
                break;
            }
            else if (!s->pDrvMount->pfnIsMounted(s->pDrvMount))
            {
                atapiR3CmdErrorSimple(s, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT);
                break;
            }
            iATAPILBA = scsiBE2H_U32(pbPacket + 2);
            if (iATAPILBA > s->cTotalSectors)
            {
                /* Rate limited logging, one log line per second. For
                 * guests that insist on seeking to places outside the
                 * valid area this often generates too many release log
                 * entries otherwise. */
                static uint64_t uLastLogTS = 0;
                if (RTTimeMilliTS() >= uLastLogTS + 1000)
                {
                    LogRel(("PIIX3 ATA: LUN#%d: CD-ROM block number %Ld invalid (SEEK)\n", s->iLUN, (uint64_t)iATAPILBA));
                    uLastLogTS = RTTimeMilliTS();
                }
                atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_LOGICAL_BLOCK_OOR);
                break;
            }
            atapiR3CmdOK(s);
            ataSetStatus(s, ATA_STAT_SEEK); /* Linux expects this. */
            break;
        }
        case SCSI_START_STOP_UNIT:
        {
            int rc = VINF_SUCCESS;
            switch (pbPacket[4] & 3)
            {
                case 0: /* 00 - Stop motor */
                case 1: /* 01 - Start motor */
                    break;
                case 2: /* 10 - Eject media */
                {
                    /* This must be done from EMT. */
                    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);
                    PPDMDEVINS pDevIns = ATADEVSTATE_2_DEVINS(s);
                    PCIATAState *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);

                    ataR3LockLeave(pCtl);
                    rc = VMR3ReqPriorityCallWait(PDMDevHlpGetVM(pDevIns), VMCPUID_ANY,
                                                 (PFNRT)s->pDrvMount->pfnUnmount, 3,
                                                 s->pDrvMount, false /*=fForce*/, true /*=fEject*/);
                    Assert(RT_SUCCESS(rc) || rc == VERR_PDM_MEDIA_LOCKED || rc == VERR_PDM_MEDIA_NOT_MOUNTED);
                    if (RT_SUCCESS(rc) && pThis->pMediaNotify)
                    {
                        rc = VMR3ReqCallNoWait(PDMDevHlpGetVM(pDevIns), VMCPUID_ANY,
                                               (PFNRT)pThis->pMediaNotify->pfnEjected, 2,
                                               pThis->pMediaNotify, s->iLUN);
                        AssertRC(rc);
                    }

                    ataR3LockEnter(pCtl);
                    break;
                }
                case 3: /* 11 - Load media */
                    /** @todo rc = s->pDrvMount->pfnLoadMedia(s->pDrvMount) */
                    break;
            }
            if (RT_SUCCESS(rc))
                atapiR3CmdOK(s);
            else
                atapiR3CmdErrorSimple(s, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIA_LOAD_OR_EJECT_FAILED);
            break;
        }
        case SCSI_MECHANISM_STATUS:
        {
            cbMax = scsiBE2H_U16(pbPacket + 8);
            ataR3StartTransfer(s, RT_MIN(cbMax, 8), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_MECHANISM_STATUS, true);
            break;
        }
        case SCSI_READ_TOC_PMA_ATIP:
        {
            uint8_t format;

            if (s->cNotifiedMediaChange > 0)
            {
                s->cNotifiedMediaChange-- ;
                atapiR3CmdErrorSimple(s, SCSI_SENSE_UNIT_ATTENTION, SCSI_ASC_MEDIUM_MAY_HAVE_CHANGED); /* media changed */
                break;
            }
            else if (!s->pDrvMount->pfnIsMounted(s->pDrvMount))
            {
                atapiR3CmdErrorSimple(s, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT);
                break;
            }
            cbMax = scsiBE2H_U16(pbPacket + 7);
            /* SCSI MMC-3 spec says format is at offset 2 (lower 4 bits),
             * but Linux kernel uses offset 9 (topmost 2 bits). Hope that
             * the other field is clear... */
            format = (pbPacket[2] & 0xf) | (pbPacket[9] >> 6);
            switch (format)
            {
                case 0:
                    ataR3StartTransfer(s, cbMax, PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_READ_TOC_NORMAL, true);
                    break;
                case 1:
                    ataR3StartTransfer(s, RT_MIN(cbMax, 12), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_READ_TOC_MULTI, true);
                    break;
                case 2:
                    ataR3StartTransfer(s, cbMax, PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_READ_TOC_RAW, true);
                    break;
                default:
                  error_cmd:
                    atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_INV_FIELD_IN_CMD_PACKET);
                    break;
            }
            break;
        }
        case SCSI_READ_CAPACITY:
            if (s->cNotifiedMediaChange > 0)
            {
                s->cNotifiedMediaChange-- ;
                atapiR3CmdErrorSimple(s, SCSI_SENSE_UNIT_ATTENTION, SCSI_ASC_MEDIUM_MAY_HAVE_CHANGED); /* media changed */
                break;
            }
            else if (!s->pDrvMount->pfnIsMounted(s->pDrvMount))
            {
                atapiR3CmdErrorSimple(s, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT);
                break;
            }
            ataR3StartTransfer(s, 8, PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_READ_CAPACITY, true);
            break;
        case SCSI_READ_DISC_INFORMATION:
            if (s->cNotifiedMediaChange > 0)
            {
                s->cNotifiedMediaChange-- ;
                atapiR3CmdErrorSimple(s, SCSI_SENSE_UNIT_ATTENTION, SCSI_ASC_MEDIUM_MAY_HAVE_CHANGED); /* media changed */
                break;
            }
            else if (!s->pDrvMount->pfnIsMounted(s->pDrvMount))
            {
                atapiR3CmdErrorSimple(s, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT);
                break;
            }
            cbMax = scsiBE2H_U16(pbPacket + 7);
            ataR3StartTransfer(s, RT_MIN(cbMax, 34), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_READ_DISC_INFORMATION, true);
            break;
        case SCSI_READ_TRACK_INFORMATION:
            if (s->cNotifiedMediaChange > 0)
            {
                s->cNotifiedMediaChange-- ;
                atapiR3CmdErrorSimple(s, SCSI_SENSE_UNIT_ATTENTION, SCSI_ASC_MEDIUM_MAY_HAVE_CHANGED); /* media changed */
                break;
            }
            else if (!s->pDrvMount->pfnIsMounted(s->pDrvMount))
            {
                atapiR3CmdErrorSimple(s, SCSI_SENSE_NOT_READY, SCSI_ASC_MEDIUM_NOT_PRESENT);
                break;
            }
            cbMax = scsiBE2H_U16(pbPacket + 7);
            ataR3StartTransfer(s, RT_MIN(cbMax, 36), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_READ_TRACK_INFORMATION, true);
            break;
        case SCSI_GET_CONFIGURATION:
            /* No media change stuff here, it can confuse Linux guests. */
            cbMax = scsiBE2H_U16(pbPacket + 7);
            ataR3StartTransfer(s, RT_MIN(cbMax, 80), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_GET_CONFIGURATION, true);
            break;
        case SCSI_INQUIRY:
            cbMax = scsiBE2H_U16(pbPacket + 3);
            ataR3StartTransfer(s, RT_MIN(cbMax, 36), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_INQUIRY, true);
            break;
        case SCSI_READ_DVD_STRUCTURE:
        {
            cbMax = scsiBE2H_U16(pbPacket + 8);
            ataR3StartTransfer(s, RT_MIN(cbMax, 4), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_READ_DVD_STRUCTURE, true);
            break;
        }
        default:
            atapiR3CmdErrorSimple(s, SCSI_SENSE_ILLEGAL_REQUEST, SCSI_ASC_ILLEGAL_OPCODE);
            break;
    }
}


/*
 * Parse ATAPI commands, passing them directly to the CD/DVD drive.
 */
static void atapiR3ParseCmdPassthrough(ATADevState *s)
{
    const uint8_t *pbPacket = &s->aATAPICmd[0];

    /* Some cases we have to handle here. */
    if (   pbPacket[0] == SCSI_GET_EVENT_STATUS_NOTIFICATION
        && ASMAtomicReadU32(&s->MediaEventStatus) != ATA_EVENT_STATUS_UNCHANGED)
    {
        uint32_t cbTransfer = scsiBE2H_U16(pbPacket + 7);
        ataR3StartTransfer(s, RT_MIN(cbTransfer, 8), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_GET_EVENT_STATUS_NOTIFICATION, true);
    }
    else if (   pbPacket[0] == SCSI_REQUEST_SENSE
             && (s->abATAPISense[2] & 0x0f) != SCSI_SENSE_NONE)
        ataR3StartTransfer(s, RT_MIN(pbPacket[4], 18), PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_ATAPI_CMD, ATAFN_SS_ATAPI_REQUEST_SENSE, true);
    else
    {
        size_t cbBuf = 0;
        size_t cbATAPISector = 0;
        size_t cbTransfer = 0;
        PDMMEDIATXDIR uTxDir = PDMMEDIATXDIR_NONE;
        uint8_t u8ScsiSts = SCSI_STATUS_OK;

        if (pbPacket[0] == SCSI_FORMAT_UNIT || pbPacket[0] == SCSI_GET_PERFORMANCE)
            cbBuf = s->uATARegLCyl | (s->uATARegHCyl << 8); /* use ATAPI transfer length */

        bool fPassthrough = ATAPIPassthroughParseCdb(pbPacket, sizeof(s->aATAPICmd), cbBuf, s->pTrackList,
                                                     &s->abATAPISense[0], sizeof(s->abATAPISense), &uTxDir, &cbTransfer,
                                                     &cbATAPISector, &u8ScsiSts);
        if (fPassthrough)
        {
            s->cbATAPISector = (uint32_t)cbATAPISector;
            Assert(s->cbATAPISector == (uint32_t)cbATAPISector);
            Assert(cbTransfer == (uint32_t)cbTransfer);

            /*
             * Send a command to the drive, passing data in/out as required.
             * Commands which exceed the I/O buffer size are split below
             * or aborted if splitting is not implemented.
             */
            Log2(("ATAPI PT: max size %d\n", cbTransfer));
            if (cbTransfer == 0)
                uTxDir = PDMMEDIATXDIR_NONE;
            ataR3StartTransfer(s, (uint32_t)cbTransfer, uTxDir, ATAFN_BT_ATAPI_PASSTHROUGH_CMD, ATAFN_SS_ATAPI_PASSTHROUGH, true);
        }
        else if (u8ScsiSts == SCSI_STATUS_CHECK_CONDITION)
        {
            /* Sense data is already set, end the request and notify the guest. */
            Log(("%s: sense=%#x (%s) asc=%#x ascq=%#x (%s)\n", __FUNCTION__, s->abATAPISense[2] & 0x0f, SCSISenseText(s->abATAPISense[2] & 0x0f),
                     s->abATAPISense[12], s->abATAPISense[13], SCSISenseExtText(s->abATAPISense[12], s->abATAPISense[13])));
            s->uATARegError = s->abATAPISense[2] << 4;
            ataSetStatusValue(s, ATA_STAT_READY | ATA_STAT_ERR);
            s->uATARegNSector = (s->uATARegNSector & ~7) | ATAPI_INT_REASON_IO | ATAPI_INT_REASON_CD;
            Log2(("%s: interrupt reason %#04x\n", __FUNCTION__, s->uATARegNSector));
            s->cbTotalTransfer = 0;
            s->cbElementaryTransfer = 0;
            s->cbAtapiPassthroughTransfer = 0;
            s->iIOBufferCur = 0;
            s->iIOBufferEnd = 0;
            s->uTxDir = PDMMEDIATXDIR_NONE;
            s->iBeginTransfer = ATAFN_BT_NULL;
            s->iSourceSink = ATAFN_SS_NULL;
        }
        else if (u8ScsiSts == SCSI_STATUS_OK)
            atapiR3CmdOK(s);
    }
}


static void atapiR3ParseCmd(ATADevState *s)
{
    const uint8_t *pbPacket;

    pbPacket = s->aATAPICmd;
# ifdef DEBUG
    Log(("%s: LUN#%d DMA=%d CMD=%#04x \"%s\"\n", __FUNCTION__, s->iLUN, s->fDMA, pbPacket[0], SCSICmdText(pbPacket[0])));
# else /* !DEBUG */
    Log(("%s: LUN#%d DMA=%d CMD=%#04x\n", __FUNCTION__, s->iLUN, s->fDMA, pbPacket[0]));
# endif /* !DEBUG */
    Log2(("%s: limit=%#x packet: %.*Rhxs\n", __FUNCTION__, s->uATARegLCyl | (s->uATARegHCyl << 8), ATAPI_PACKET_SIZE, pbPacket));

    if (s->fATAPIPassthrough)
        atapiR3ParseCmdPassthrough(s);
    else
        atapiR3ParseCmdVirtualATAPI(s);
}


static bool ataR3PacketSS(ATADevState *s)
{
    s->fDMA = !!(s->uATARegFeature & 1);
    memcpy(s->aATAPICmd, s->CTX_SUFF(pbIOBuffer), ATAPI_PACKET_SIZE);
    s->uTxDir = PDMMEDIATXDIR_NONE;
    s->cbTotalTransfer = 0;
    s->cbElementaryTransfer = 0;
    s->cbAtapiPassthroughTransfer = 0;
    atapiR3ParseCmd(s);
    return false;
}


/**
 * SCSI_GET_EVENT_STATUS_NOTIFICATION should return "medium removed" event
 * from now on, regardless if there was a medium inserted or not.
 */
static void ataR3MediumRemoved(ATADevState *s)
{
    ASMAtomicWriteU32(&s->MediaEventStatus, ATA_EVENT_STATUS_MEDIA_REMOVED);
}


/**
 * SCSI_GET_EVENT_STATUS_NOTIFICATION should return "medium inserted". If
 * there was already a medium inserted, don't forget to send the "medium
 * removed" event first.
 */
static void ataR3MediumInserted(ATADevState *s)
{
    uint32_t OldStatus, NewStatus;
    do
    {
        OldStatus = ASMAtomicReadU32(&s->MediaEventStatus);
        switch (OldStatus)
        {
            case ATA_EVENT_STATUS_MEDIA_CHANGED:
            case ATA_EVENT_STATUS_MEDIA_REMOVED:
                /* no change, we will send "medium removed" + "medium inserted" */
                NewStatus = ATA_EVENT_STATUS_MEDIA_CHANGED;
                break;
            default:
                NewStatus = ATA_EVENT_STATUS_MEDIA_NEW;
                break;
        }
    } while (!ASMAtomicCmpXchgU32(&s->MediaEventStatus, NewStatus, OldStatus));
}


/**
 * @interface_method_impl{PDMIMOUNTNOTIFY,pfnMountNotify}
 */
static DECLCALLBACK(void) ataR3MountNotify(PPDMIMOUNTNOTIFY pInterface)
{
    ATADevState *pIf = RT_FROM_MEMBER(pInterface, ATADevState, IMountNotify);
    Log(("%s: changing LUN#%d\n", __FUNCTION__, pIf->iLUN));

    /* Ignore the call if we're called while being attached. */
    if (!pIf->pDrvMedia)
        return;

    uint32_t cRegions = pIf->pDrvMedia->pfnGetRegionCount(pIf->pDrvMedia);
    for (uint32_t i = 0; i < cRegions; i++)
    {
        uint64_t cBlocks = 0;
        int rc = pIf->pDrvMedia->pfnQueryRegionProperties(pIf->pDrvMedia, i, NULL, &cBlocks,
                                                          NULL, NULL);
        AssertRC(rc);
        pIf->cTotalSectors += cBlocks;
    }

    LogRel(("PIIX3 ATA: LUN#%d: CD/DVD, total number of sectors %Ld, passthrough unchanged\n", pIf->iLUN, pIf->cTotalSectors));

    /* Report media changed in TEST UNIT and other (probably incorrect) places. */
    if (pIf->cNotifiedMediaChange < 2)
        pIf->cNotifiedMediaChange = 1;
    ataR3MediumInserted(pIf);
    ataR3MediumTypeSet(pIf, ATA_MEDIA_TYPE_UNKNOWN);
}

/**
 * @interface_method_impl{PDMIMOUNTNOTIFY,pfnUnmountNotify}
 */
static DECLCALLBACK(void) ataR3UnmountNotify(PPDMIMOUNTNOTIFY pInterface)
{
    ATADevState *pIf = RT_FROM_MEMBER(pInterface, ATADevState, IMountNotify);
    Log(("%s:\n", __FUNCTION__));
    pIf->cTotalSectors = 0;

    /*
     * Whatever I do, XP will not use the GET MEDIA STATUS nor the EVENT stuff.
     * However, it will respond to TEST UNIT with a 0x6 0x28 (media changed) sense code.
     * So, we'll give it 4 TEST UNIT command to catch up, two which the media is not
     * present and 2 in which it is changed.
     */
    pIf->cNotifiedMediaChange = 1;
    ataR3MediumRemoved(pIf);
    ataR3MediumTypeSet(pIf, ATA_MEDIA_NO_DISC);
}

static void ataR3PacketBT(ATADevState *s)
{
    s->cbElementaryTransfer = s->cbTotalTransfer;
    s->cbAtapiPassthroughTransfer = s->cbTotalTransfer;
    s->uATARegNSector = (s->uATARegNSector & ~7) | ATAPI_INT_REASON_CD;
    Log2(("%s: interrupt reason %#04x\n", __FUNCTION__, s->uATARegNSector));
    ataSetStatusValue(s, ATA_STAT_READY);
}


static void ataR3ResetDevice(ATADevState *s)
{
    s->cMultSectors = ATA_MAX_MULT_SECTORS;
    s->cNotifiedMediaChange = 0;
    ASMAtomicWriteU32(&s->MediaEventStatus, ATA_EVENT_STATUS_UNCHANGED);
    ASMAtomicWriteU32(&s->MediaTrackType, ATA_MEDIA_TYPE_UNKNOWN);
    ataUnsetIRQ(s);

    s->uATARegSelect = 0x20;
    ataSetStatusValue(s, ATA_STAT_READY);
    ataR3SetSignature(s);
    s->cbTotalTransfer = 0;
    s->cbElementaryTransfer = 0;
    s->cbAtapiPassthroughTransfer = 0;
    s->iIOBufferPIODataStart = 0;
    s->iIOBufferPIODataEnd = 0;
    s->iBeginTransfer = ATAFN_BT_NULL;
    s->iSourceSink = ATAFN_SS_NULL;
    s->fDMA = false;
    s->fATAPITransfer = false;
    s->uATATransferMode = ATA_MODE_UDMA | 2; /* PIIX3 supports only up to UDMA2 */

    s->uATARegFeature = 0;
}


static bool ataR3ExecuteDeviceDiagnosticSS(ATADevState *s)
{
    ataR3SetSignature(s);
    if (s->fATAPI)
        ataSetStatusValue(s, 0); /* NOTE: READY is _not_ set */
    else
        ataSetStatusValue(s, ATA_STAT_READY | ATA_STAT_SEEK);
    s->uATARegError = 0x01;
    return false;
}


static int ataR3TrimSectors(ATADevState *s, uint64_t u64Sector, uint32_t cSectors,
                            bool *pfRedo)
{
    RTRANGE TrimRange;
    PATACONTROLLER pCtl = ATADEVSTATE_2_CONTROLLER(s);
    int rc;

    ataR3LockLeave(pCtl);

    TrimRange.offStart = u64Sector * s->cbSector;
    TrimRange.cbRange  = cSectors * s->cbSector;

    s->Led.Asserted.s.fWriting = s->Led.Actual.s.fWriting = 1;
    rc = s->pDrvMedia->pfnDiscard(s->pDrvMedia, &TrimRange, 1);
    s->Led.Actual.s.fWriting = 0;

    if (RT_SUCCESS(rc))
        *pfRedo = false;
    else
        *pfRedo = ataR3IsRedoSetWarning(s, rc);

    ataR3LockEnter(pCtl);
    return rc;
}


static bool ataR3TrimSS(ATADevState *s)
{
    int rc = VERR_GENERAL_FAILURE;
    uint32_t cRangesMax;
    uint64_t *pu64Range = (uint64_t *)s->CTX_SUFF(pbIOBuffer);
    bool fRedo = false;

    cRangesMax = s->cbElementaryTransfer / sizeof(uint64_t);
    Assert(cRangesMax);

    while (cRangesMax-- > 0)
    {
        if (ATA_RANGE_LENGTH_GET(*pu64Range) == 0)
            break;

        rc = ataR3TrimSectors(s, *pu64Range & ATA_RANGE_LBA_MASK,
                            ATA_RANGE_LENGTH_GET(*pu64Range), &fRedo);
        if (RT_FAILURE(rc))
            break;

        pu64Range++;
    }

    if (RT_SUCCESS(rc))
    {
        s->iSourceSink = ATAFN_SS_NULL;
        ataR3CmdOK(s, ATA_STAT_SEEK);
    }
    else
    {
        if (fRedo)
            return fRedo;
        if (s->cErrors++ < MAX_LOG_REL_ERRORS)
            LogRel(("PIIX3 ATA: LUN#%d: disk trim error (rc=%Rrc iSector=%#RX64 cSectors=%#RX32)\n",
                    s->iLUN, rc, *pu64Range & ATA_RANGE_LBA_MASK, ATA_RANGE_LENGTH_GET(*pu64Range)));

        /*
         * Check if we got interrupted. We don't need to set status variables
         * because the request was aborted.
         */
        if (rc != VERR_INTERRUPTED)
            ataR3CmdError(s, ID_ERR);
    }

    return false;
}


static void ataR3ParseCmd(ATADevState *s, uint8_t cmd)
{
# ifdef DEBUG
    Log(("%s: LUN#%d CMD=%#04x \"%s\"\n", __FUNCTION__, s->iLUN, cmd, ATACmdText(cmd)));
# else /* !DEBUG */
    Log(("%s: LUN#%d CMD=%#04x\n", __FUNCTION__, s->iLUN, cmd));
# endif /* !DEBUG */
    s->fLBA48 = false;
    s->fDMA = false;
    if (cmd == ATA_IDLE_IMMEDIATE)
    {
        /* Detect Linux timeout recovery, first tries IDLE IMMEDIATE (which
         * would overwrite the failing command unfortunately), then RESET. */
        int32_t uCmdWait = -1;
        uint64_t uNow = RTTimeNanoTS();
        if (s->u64CmdTS)
            uCmdWait = (uNow - s->u64CmdTS) / 1000;
        LogRel(("PIIX3 ATA: LUN#%d: IDLE IMMEDIATE, CmdIf=%#04x (%d usec ago)\n",
                s->iLUN, s->uATARegCommand, uCmdWait));
    }
    s->uATARegCommand = cmd;
    switch (cmd)
    {
        case ATA_IDENTIFY_DEVICE:
            if (s->pDrvMedia && !s->fATAPI)
                ataR3StartTransfer(s, 512, PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_NULL, ATAFN_SS_IDENTIFY, false);
            else
            {
                if (s->fATAPI)
                    ataR3SetSignature(s);
                ataR3CmdError(s, ABRT_ERR);
                ataUnsetStatus(s, ATA_STAT_READY);
                ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
            }
            break;
        case ATA_RECALIBRATE:
            if (s->fATAPI)
                goto abort_cmd;
            RT_FALL_THRU();
        case ATA_INITIALIZE_DEVICE_PARAMETERS:
            ataR3CmdOK(s, ATA_STAT_SEEK);
            ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
            break;
        case ATA_SET_MULTIPLE_MODE:
            if (    s->uATARegNSector != 0
                &&  (   s->uATARegNSector > ATA_MAX_MULT_SECTORS
                     || (s->uATARegNSector & (s->uATARegNSector - 1)) != 0))
            {
                ataR3CmdError(s, ABRT_ERR);
            }
            else
            {
                Log2(("%s: set multi sector count to %d\n", __FUNCTION__, s->uATARegNSector));
                s->cMultSectors = s->uATARegNSector;
                ataR3CmdOK(s, 0);
            }
            ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
            break;
        case ATA_READ_VERIFY_SECTORS_EXT:
            s->fLBA48 = true;
            RT_FALL_THRU();
        case ATA_READ_VERIFY_SECTORS:
        case ATA_READ_VERIFY_SECTORS_WITHOUT_RETRIES:
            /* do sector number check ? */
            ataR3CmdOK(s, ATA_STAT_SEEK);
            ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
            break;
        case ATA_READ_SECTORS_EXT:
            s->fLBA48 = true;
            RT_FALL_THRU();
        case ATA_READ_SECTORS:
        case ATA_READ_SECTORS_WITHOUT_RETRIES:
            if (!s->pDrvMedia || s->fATAPI)
                goto abort_cmd;
            s->cSectorsPerIRQ = 1;
            ataR3StartTransfer(s, ataR3GetNSectors(s) * s->cbSector, PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_READ_WRITE_SECTORS, ATAFN_SS_READ_SECTORS, false);
            break;
        case ATA_WRITE_SECTORS_EXT:
            s->fLBA48 = true;
            RT_FALL_THRU();
        case ATA_WRITE_SECTORS:
        case ATA_WRITE_SECTORS_WITHOUT_RETRIES:
            if (!s->pDrvMedia || s->fATAPI)
                goto abort_cmd;
            s->cSectorsPerIRQ = 1;
            ataR3StartTransfer(s, ataR3GetNSectors(s) * s->cbSector, PDMMEDIATXDIR_TO_DEVICE, ATAFN_BT_READ_WRITE_SECTORS, ATAFN_SS_WRITE_SECTORS, false);
            break;
        case ATA_READ_MULTIPLE_EXT:
            s->fLBA48 = true;
            RT_FALL_THRU();
        case ATA_READ_MULTIPLE:
            if (!s->pDrvMedia || !s->cMultSectors || s->fATAPI)
                goto abort_cmd;
            s->cSectorsPerIRQ = s->cMultSectors;
            ataR3StartTransfer(s, ataR3GetNSectors(s) * s->cbSector, PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_READ_WRITE_SECTORS, ATAFN_SS_READ_SECTORS, false);
            break;
        case ATA_WRITE_MULTIPLE_EXT:
            s->fLBA48 = true;
            RT_FALL_THRU();
        case ATA_WRITE_MULTIPLE:
            if (!s->pDrvMedia || !s->cMultSectors || s->fATAPI)
                goto abort_cmd;
            s->cSectorsPerIRQ = s->cMultSectors;
            ataR3StartTransfer(s, ataR3GetNSectors(s) * s->cbSector, PDMMEDIATXDIR_TO_DEVICE, ATAFN_BT_READ_WRITE_SECTORS, ATAFN_SS_WRITE_SECTORS, false);
            break;
        case ATA_READ_DMA_EXT:
            s->fLBA48 = true;
            RT_FALL_THRU();
        case ATA_READ_DMA:
        case ATA_READ_DMA_WITHOUT_RETRIES:
            if (!s->pDrvMedia || s->fATAPI)
                goto abort_cmd;
            s->cSectorsPerIRQ = ATA_MAX_MULT_SECTORS;
            s->fDMA = true;
            ataR3StartTransfer(s, ataR3GetNSectors(s) * s->cbSector, PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_READ_WRITE_SECTORS, ATAFN_SS_READ_SECTORS, false);
            break;
        case ATA_WRITE_DMA_EXT:
            s->fLBA48 = true;
            RT_FALL_THRU();
        case ATA_WRITE_DMA:
        case ATA_WRITE_DMA_WITHOUT_RETRIES:
            if (!s->pDrvMedia || s->fATAPI)
                goto abort_cmd;
            s->cSectorsPerIRQ = ATA_MAX_MULT_SECTORS;
            s->fDMA = true;
            ataR3StartTransfer(s, ataR3GetNSectors(s) * s->cbSector, PDMMEDIATXDIR_TO_DEVICE, ATAFN_BT_READ_WRITE_SECTORS, ATAFN_SS_WRITE_SECTORS, false);
            break;
        case ATA_READ_NATIVE_MAX_ADDRESS_EXT:
            s->fLBA48 = true;
            ataR3SetSector(s, s->cTotalSectors - 1);
            ataR3CmdOK(s, 0);
            ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
            break;
        case ATA_SEEK: /* Used by the SCO OpenServer. Command is marked as obsolete */
            ataR3CmdOK(s, 0);
            ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
            break;
        case ATA_READ_NATIVE_MAX_ADDRESS:
            ataR3SetSector(s, RT_MIN(s->cTotalSectors, 1 << 28) - 1);
            ataR3CmdOK(s, 0);
            ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
            break;
        case ATA_CHECK_POWER_MODE:
            s->uATARegNSector = 0xff; /* drive active or idle */
            ataR3CmdOK(s, 0);
            ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
            break;
        case ATA_SET_FEATURES:
            Log2(("%s: feature=%#x\n", __FUNCTION__, s->uATARegFeature));
            if (!s->pDrvMedia)
                goto abort_cmd;
            switch (s->uATARegFeature)
            {
                case 0x02: /* write cache enable */
                    Log2(("%s: write cache enable\n", __FUNCTION__));
                    ataR3CmdOK(s, ATA_STAT_SEEK);
                    ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
                    break;
                case 0xaa: /* read look-ahead enable */
                    Log2(("%s: read look-ahead enable\n", __FUNCTION__));
                    ataR3CmdOK(s, ATA_STAT_SEEK);
                    ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
                    break;
                case 0x55: /* read look-ahead disable */
                    Log2(("%s: read look-ahead disable\n", __FUNCTION__));
                    ataR3CmdOK(s, ATA_STAT_SEEK);
                    ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
                    break;
                case 0xcc: /* reverting to power-on defaults enable */
                    Log2(("%s: revert to power-on defaults enable\n", __FUNCTION__));
                    ataR3CmdOK(s, ATA_STAT_SEEK);
                    ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
                    break;
                case 0x66: /* reverting to power-on defaults disable */
                    Log2(("%s: revert to power-on defaults disable\n", __FUNCTION__));
                    ataR3CmdOK(s, ATA_STAT_SEEK);
                    ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
                    break;
                case 0x82: /* write cache disable */
                    Log2(("%s: write cache disable\n", __FUNCTION__));
                    /* As per the ATA/ATAPI-6 specs, a write cache disable
                     * command MUST flush the write buffers to disc. */
                    ataR3StartTransfer(s, 0, PDMMEDIATXDIR_NONE, ATAFN_BT_NULL, ATAFN_SS_FLUSH, false);
                    break;
                case 0x03: { /* set transfer mode */
                    Log2(("%s: transfer mode %#04x\n", __FUNCTION__, s->uATARegNSector));
                    switch (s->uATARegNSector & 0xf8)
                    {
                        case 0x00: /* PIO default */
                        case 0x08: /* PIO mode */
                            break;
                        case ATA_MODE_MDMA: /* MDMA mode */
                            s->uATATransferMode = (s->uATARegNSector & 0xf8) | RT_MIN(s->uATARegNSector & 0x07, ATA_MDMA_MODE_MAX);
                            break;
                        case ATA_MODE_UDMA: /* UDMA mode */
                            s->uATATransferMode = (s->uATARegNSector & 0xf8) | RT_MIN(s->uATARegNSector & 0x07, ATA_UDMA_MODE_MAX);
                            break;
                        default:
                            goto abort_cmd;
                    }
                    ataR3CmdOK(s, ATA_STAT_SEEK);
                    ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
                    break;
                }
                default:
                    goto abort_cmd;
            }
            /*
             * OS/2 workarond:
             * The OS/2 IDE driver from MCP2 appears to rely on the feature register being
             * reset here. According to the specification, this is a driver bug as the register
             * contents are undefined after the call. This means we can just as well reset it.
             */
            s->uATARegFeature = 0;
            break;
        case ATA_FLUSH_CACHE_EXT:
        case ATA_FLUSH_CACHE:
            if (!s->pDrvMedia || s->fATAPI)
                goto abort_cmd;
            ataR3StartTransfer(s, 0, PDMMEDIATXDIR_NONE, ATAFN_BT_NULL, ATAFN_SS_FLUSH, false);
            break;
        case ATA_STANDBY_IMMEDIATE:
            ataR3CmdOK(s, 0);
            ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
            break;
        case ATA_IDLE_IMMEDIATE:
            LogRel(("PIIX3 ATA: LUN#%d: aborting current command\n", s->iLUN));
            ataR3AbortCurrentCommand(s, false);
            break;
        case ATA_SLEEP:
            ataR3CmdOK(s, 0);
            ataHCSetIRQ(s);
            break;
            /* ATAPI commands */
        case ATA_IDENTIFY_PACKET_DEVICE:
            if (s->fATAPI)
                ataR3StartTransfer(s, 512, PDMMEDIATXDIR_FROM_DEVICE, ATAFN_BT_NULL, ATAFN_SS_ATAPI_IDENTIFY, false);
            else
            {
                ataR3CmdError(s, ABRT_ERR);
                ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
            }
            break;
        case ATA_EXECUTE_DEVICE_DIAGNOSTIC:
            ataR3StartTransfer(s, 0, PDMMEDIATXDIR_NONE, ATAFN_BT_NULL, ATAFN_SS_EXECUTE_DEVICE_DIAGNOSTIC, false);
            break;
        case ATA_DEVICE_RESET:
            if (!s->fATAPI)
                goto abort_cmd;
            LogRel(("PIIX3 ATA: LUN#%d: performing device RESET\n", s->iLUN));
            ataR3AbortCurrentCommand(s, true);
            break;
        case ATA_PACKET:
            if (!s->fATAPI)
                goto abort_cmd;
            /* overlapping commands not supported */
            if (s->uATARegFeature & 0x02)
                goto abort_cmd;
            ataR3StartTransfer(s, ATAPI_PACKET_SIZE, PDMMEDIATXDIR_TO_DEVICE, ATAFN_BT_PACKET, ATAFN_SS_PACKET, false);
            break;
        case ATA_DATA_SET_MANAGEMENT:
            if (!s->pDrvMedia || !s->pDrvMedia->pfnDiscard)
                goto abort_cmd;
            if (   !(s->uATARegFeature & UINT8_C(0x01))
                || (s->uATARegFeature & ~UINT8_C(0x01)))
                goto abort_cmd;
            s->fDMA = true;
            ataR3StartTransfer(s, (s->uATARegNSectorHOB << 8 | s->uATARegNSector) * s->cbSector, PDMMEDIATXDIR_TO_DEVICE, ATAFN_BT_NULL, ATAFN_SS_TRIM, false);
            break;
        default:
        abort_cmd:
            ataR3CmdError(s, ABRT_ERR);
            if (s->fATAPI)
                ataUnsetStatus(s, ATA_STAT_READY);
            ataHCSetIRQ(s); /* Shortcut, do not use AIO thread. */
            break;
    }
}

# endif /* IN_RING3 */
#endif /* IN_RING0 || IN_RING3 */

/*
 * Note: There are four distinct cases of port I/O handling depending on
 * which devices (if any) are attached to an IDE channel:
 *
 *  1) No device attached. No response to writes or reads (i.e. reads return
 *     all bits set).
 *
 *  2) Both devices attached. Reads and writes are processed normally.
 *
 *  3) Device 0 only. If device 0 is selected, normal behavior applies. But
 *     if Device 1 is selected, writes are still directed to Device 0 (except
 *     commands are not executed), reads from control/command registers are
 *     directed to Device 0, but status/alt status reads return 0. If Device 1
 *     is a PACKET device, all reads return 0. See ATAPI-6 clause 9.16.1 and
 *     Table 18 in clause 7.1.
 *
 *  4) Device 1 only - non-standard(!). Device 1 can't tell if Device 0 is
 *     present or not and behaves the same. That means if Device 0 is selected,
 *     Device 1 responds to writes (except commands are not executed) but does
 *     not respond to reads. If Device 1 selected, normal behavior applies.
 *     See ATAPI-6 clause 9.16.2 and Table 15 in clause 7.1.
 */

static int ataIOPortWriteU8(PATACONTROLLER pCtl, uint32_t addr, uint32_t val)
{
    Log2(("%s: LUN#%d write addr=%#x val=%#04x\n", __FUNCTION__, pCtl->aIfs[pCtl->iSelectedIf].iLUN, addr, val));
    addr &= 7;
    switch (addr)
    {
        case 0:
            break;
        case 1: /* feature register */
            /* NOTE: data is written to the two drives */
            pCtl->aIfs[0].uATARegDevCtl &= ~ATA_DEVCTL_HOB;
            pCtl->aIfs[1].uATARegDevCtl &= ~ATA_DEVCTL_HOB;
            pCtl->aIfs[0].uATARegFeatureHOB = pCtl->aIfs[0].uATARegFeature;
            pCtl->aIfs[1].uATARegFeatureHOB = pCtl->aIfs[1].uATARegFeature;
            pCtl->aIfs[0].uATARegFeature = val;
            pCtl->aIfs[1].uATARegFeature = val;
            break;
        case 2: /* sector count */
            pCtl->aIfs[0].uATARegDevCtl &= ~ATA_DEVCTL_HOB;
            pCtl->aIfs[1].uATARegDevCtl &= ~ATA_DEVCTL_HOB;
            pCtl->aIfs[0].uATARegNSectorHOB = pCtl->aIfs[0].uATARegNSector;
            pCtl->aIfs[1].uATARegNSectorHOB = pCtl->aIfs[1].uATARegNSector;
            pCtl->aIfs[0].uATARegNSector = val;
            pCtl->aIfs[1].uATARegNSector = val;
            break;
        case 3: /* sector number */
            pCtl->aIfs[0].uATARegDevCtl &= ~ATA_DEVCTL_HOB;
            pCtl->aIfs[1].uATARegDevCtl &= ~ATA_DEVCTL_HOB;
            pCtl->aIfs[0].uATARegSectorHOB = pCtl->aIfs[0].uATARegSector;
            pCtl->aIfs[1].uATARegSectorHOB = pCtl->aIfs[1].uATARegSector;
            pCtl->aIfs[0].uATARegSector = val;
            pCtl->aIfs[1].uATARegSector = val;
            break;
        case 4: /* cylinder low */
            pCtl->aIfs[0].uATARegDevCtl &= ~ATA_DEVCTL_HOB;
            pCtl->aIfs[1].uATARegDevCtl &= ~ATA_DEVCTL_HOB;
            pCtl->aIfs[0].uATARegLCylHOB = pCtl->aIfs[0].uATARegLCyl;
            pCtl->aIfs[1].uATARegLCylHOB = pCtl->aIfs[1].uATARegLCyl;
            pCtl->aIfs[0].uATARegLCyl = val;
            pCtl->aIfs[1].uATARegLCyl = val;
            break;
        case 5: /* cylinder high */
            pCtl->aIfs[0].uATARegDevCtl &= ~ATA_DEVCTL_HOB;
            pCtl->aIfs[1].uATARegDevCtl &= ~ATA_DEVCTL_HOB;
            pCtl->aIfs[0].uATARegHCylHOB = pCtl->aIfs[0].uATARegHCyl;
            pCtl->aIfs[1].uATARegHCylHOB = pCtl->aIfs[1].uATARegHCyl;
            pCtl->aIfs[0].uATARegHCyl = val;
            pCtl->aIfs[1].uATARegHCyl = val;
            break;
        case 6: /* drive/head */
            pCtl->aIfs[0].uATARegSelect = (val & ~0x10) | 0xa0;
            pCtl->aIfs[1].uATARegSelect = (val | 0x10) | 0xa0;
            if (((val >> 4) & 1) != pCtl->iSelectedIf)
            {
                PPDMDEVINS pDevIns = CONTROLLER_2_DEVINS(pCtl);

                /* select another drive */
                pCtl->iSelectedIf = (val >> 4) & 1;
                /* The IRQ line is multiplexed between the two drives, so
                 * update the state when switching to another drive. Only need
                 * to update interrupt line if it is enabled and there is a
                 * state change. */
                if (    !(pCtl->aIfs[pCtl->iSelectedIf].uATARegDevCtl & ATA_DEVCTL_DISABLE_IRQ)
                    &&  (   pCtl->aIfs[pCtl->iSelectedIf].fIrqPending
                         !=  pCtl->aIfs[pCtl->iSelectedIf ^ 1].fIrqPending))
                {
                    if (pCtl->aIfs[pCtl->iSelectedIf].fIrqPending)
                    {
                        Log2(("%s: LUN#%d asserting IRQ (drive select change)\n", __FUNCTION__, pCtl->aIfs[pCtl->iSelectedIf].iLUN));
                        /* The BMDMA unit unconditionally sets BM_STATUS_INT if
                         * the interrupt line is asserted. It monitors the line
                         * for a rising edge. */
                        pCtl->BmDma.u8Status |= BM_STATUS_INT;
                        if (pCtl->irq == 16)
                            PDMDevHlpPCISetIrq(pDevIns, 0, 1);
                        else
                            PDMDevHlpISASetIrq(pDevIns, pCtl->irq, 1);
                    }
                    else
                    {
                        Log2(("%s: LUN#%d deasserting IRQ (drive select change)\n", __FUNCTION__, pCtl->aIfs[pCtl->iSelectedIf].iLUN));
                        if (pCtl->irq == 16)
                            PDMDevHlpPCISetIrq(pDevIns, 0, 0);
                        else
                            PDMDevHlpISASetIrq(pDevIns, pCtl->irq, 0);
                    }
                }
            }
            break;
        default:
        case 7: /* command */
            /* ignore commands to non-existent device */
            if (pCtl->iSelectedIf && !pCtl->aIfs[pCtl->iSelectedIf].pDrvMedia)
                break;
#ifndef IN_RING3
            /* Don't do anything complicated in GC */
            return VINF_IOM_R3_IOPORT_WRITE;
#else /* IN_RING3 */
            ataR3ParseCmd(&pCtl->aIfs[pCtl->iSelectedIf], val);
#endif /* !IN_RING3 */
    }
    return VINF_SUCCESS;
}


static int ataIOPortReadU8(PATACONTROLLER pCtl, uint32_t addr, uint32_t *pu32)
{
    ATADevState *s = &pCtl->aIfs[pCtl->iSelectedIf];
    uint32_t    val;
    bool        fHOB;

    /* Check if the guest is reading from a non-existent device. */
    if (!s->pDrvMedia)
    {
        if (pCtl->iSelectedIf)  /* Device 1 selected, Device 0 responding for it. */
        {
            if (!pCtl->aIfs[0].pDrvMedia)   /** @todo this case should never get here! */
            {
                Log2(("%s: addr=%#x: no device on channel\n", __FUNCTION__, addr));
                return VERR_IOM_IOPORT_UNUSED;
            }
            if (((addr & 7) != 1) && pCtl->aIfs[0].fATAPI) {
                Log2(("%s: addr=%#x, val=0: LUN#%d not attached/LUN#%d ATAPI\n", __FUNCTION__, addr,
                                s->iLUN, pCtl->aIfs[0].iLUN));
                *pu32 = 0;
                return VINF_SUCCESS;
            }
            /* Else handle normally. */
        }
        else                    /* Device 0 selected (but not present). */
        {
            Log2(("%s: addr=%#x: LUN#%d not attached\n", __FUNCTION__, addr, s->iLUN));
            return VERR_IOM_IOPORT_UNUSED;
        }
    }
    fHOB = !!(s->uATARegDevCtl & (1 << 7));
    switch (addr & 7)
    {
        case 0: /* data register */
            val = 0xff;
            break;
        case 1: /* error register */
            /* The ATA specification is very terse when it comes to specifying
             * the precise effects of reading back the error/feature register.
             * The error register (read-only) shares the register number with
             * the feature register (write-only), so it seems that it's not
             * necessary to support the usual HOB readback here. */
            if (!s->pDrvMedia)
                val = 0;
            else
                val = s->uATARegError;
            break;
        case 2: /* sector count */
            if (fHOB)
                val = s->uATARegNSectorHOB;
            else
                val = s->uATARegNSector;
            break;
        case 3: /* sector number */
            if (fHOB)
                val = s->uATARegSectorHOB;
            else
                val = s->uATARegSector;
            break;
        case 4: /* cylinder low */
            if (fHOB)
                val = s->uATARegLCylHOB;
            else
                val = s->uATARegLCyl;
            break;
        case 5: /* cylinder high */
            if (fHOB)
                val = s->uATARegHCylHOB;
            else
                val = s->uATARegHCyl;
            break;
        case 6: /* drive/head */
            /* This register must always work as long as there is at least
             * one drive attached to the controller. It is common between
             * both drives anyway (completely identical content). */
            if (!pCtl->aIfs[0].pDrvMedia && !pCtl->aIfs[1].pDrvMedia)
                val = 0;
            else
                val = s->uATARegSelect;
            break;
        default:
        case 7: /* primary status */
        {
            /* Counter for number of busy status seen in GC in a row. */
            static unsigned cBusy = 0;

            if (!s->pDrvMedia)
                val = 0;
            else
                val = s->uATARegStatus;

            /* Give the async I/O thread an opportunity to make progress,
             * don't let it starve by guests polling frequently. EMT has a
             * lower priority than the async I/O thread, but sometimes the
             * host OS doesn't care. With some guests we are only allowed to
             * be busy for about 5 milliseconds in some situations. Note that
             * this is no guarantee for any other VBox thread getting
             * scheduled, so this just lowers the CPU load a bit when drives
             * are busy. It cannot help with timing problems. */
            if (val & ATA_STAT_BUSY)
            {
#ifdef IN_RING3
                cBusy = 0;
                ataR3LockLeave(pCtl);

#ifndef RT_OS_WINDOWS
                /*
                 * The thread might be stuck in an I/O operation
                 * due to a high I/O load on the host. (see @bugref{3301})
                 * To perform the reset successfully
                 * we interrupt the operation by sending a signal to the thread
                 * if the thread didn't responded in 10ms.
                 * This works only on POSIX hosts (Windows has a CancelSynchronousIo function which
                 * does the same but it was introduced with Vista) but so far
                 * this hang was only observed on Linux and Mac OS X.
                 *
                 * This is a workaround and needs to be solved properly.
                 */
                if (pCtl->fReset)
                {
                    uint64_t u64ResetTimeStop = RTTimeMilliTS();

                    if ((u64ResetTimeStop - pCtl->u64ResetTime) >= 10)
                    {
                        LogRel(("PIIX3 ATA LUN#%d: Async I/O thread probably stuck in operation, interrupting\n", s->iLUN));
                        pCtl->u64ResetTime = u64ResetTimeStop;
                        RTThreadPoke(pCtl->AsyncIOThread);
                    }
                }
#endif

                RTThreadYield();

                ataR3LockEnter(pCtl);

                val = s->uATARegStatus;
#else /* !IN_RING3 */
                /* Cannot yield CPU in raw-mode and ring-0 context.  And switching
                 * to host context for each and every busy status is too costly,
                 * especially on SMP systems where we don't gain much by
                 * yielding the CPU to someone else. */
                if (++cBusy >= 20)
                {
                    cBusy = 0;
                    return VINF_IOM_R3_IOPORT_READ;
                }
#endif /* !IN_RING3 */
            }
            else
                cBusy = 0;
            ataUnsetIRQ(s);
            break;
        }
    }
    Log2(("%s: LUN#%d addr=%#x val=%#04x\n", __FUNCTION__, s->iLUN, addr, val));
    *pu32 = val;
    return VINF_SUCCESS;
}


static uint32_t ataStatusRead(PATACONTROLLER pCtl, uint32_t addr)
{
    ATADevState *s = &pCtl->aIfs[pCtl->iSelectedIf];
    uint32_t val;
    RT_NOREF1(addr);

    /// @todo The handler should not be even registered if there
    // is no device on an IDE channel.
    if (!pCtl->aIfs[0].pDrvMedia && !pCtl->aIfs[1].pDrvMedia)
        val = 0xff;
    else if (pCtl->iSelectedIf == 1 && !s->pDrvMedia)
        val = 0;    /* Device 1 selected, Device 0 responding for it. */
    else
        val = s->uATARegStatus;
    Log2(("%s: addr=%#x val=%#04x\n", __FUNCTION__, addr, val));
    return val;
}

static int ataControlWrite(PATACONTROLLER pCtl, uint32_t addr, uint32_t val)
{
    RT_NOREF1(addr);
#ifndef IN_RING3
    if ((val ^ pCtl->aIfs[0].uATARegDevCtl) & ATA_DEVCTL_RESET)
        return VINF_IOM_R3_IOPORT_WRITE; /* The RESET stuff is too complicated for RC+R0. */
#endif /* !IN_RING3 */

    Log2(("%s: addr=%#x val=%#04x\n", __FUNCTION__, addr, val));
    /* RESET is common for both drives attached to a controller. */
    if (   !(pCtl->aIfs[0].uATARegDevCtl & ATA_DEVCTL_RESET)
        && (val & ATA_DEVCTL_RESET))
    {
#ifdef IN_RING3
        /* Software RESET low to high */
        int32_t uCmdWait0 = -1;
        int32_t uCmdWait1 = -1;
        uint64_t uNow = RTTimeNanoTS();
        if (pCtl->aIfs[0].u64CmdTS)
            uCmdWait0 = (uNow - pCtl->aIfs[0].u64CmdTS) / 1000;
        if (pCtl->aIfs[1].u64CmdTS)
            uCmdWait1 = (uNow - pCtl->aIfs[1].u64CmdTS) / 1000;
        LogRel(("PIIX3 ATA: Ctl#%d: RESET, DevSel=%d AIOIf=%d CmdIf0=%#04x (%d usec ago) CmdIf1=%#04x (%d usec ago)\n",
                ATACONTROLLER_IDX(pCtl), pCtl->iSelectedIf, pCtl->iAIOIf,
                pCtl->aIfs[0].uATARegCommand, uCmdWait0,
                pCtl->aIfs[1].uATARegCommand, uCmdWait1));
        pCtl->fReset = true;
        /* Everything must be done after the reset flag is set, otherwise
         * there are unavoidable races with the currently executing request
         * (which might just finish in the mean time). */
        pCtl->fChainedTransfer = false;
        for (uint32_t i = 0; i < RT_ELEMENTS(pCtl->aIfs); i++)
        {
            ataR3ResetDevice(&pCtl->aIfs[i]);
            /* The following cannot be done using ataSetStatusValue() since the
             * reset flag is already set, which suppresses all status changes. */
            pCtl->aIfs[i].uATARegStatus = ATA_STAT_BUSY | ATA_STAT_SEEK;
            Log2(("%s: LUN#%d status %#04x\n", __FUNCTION__, pCtl->aIfs[i].iLUN, pCtl->aIfs[i].uATARegStatus));
            pCtl->aIfs[i].uATARegError = 0x01;
        }
        pCtl->iSelectedIf = 0;
        ataR3AsyncIOClearRequests(pCtl);
        Log2(("%s: Ctl#%d: message to async I/O thread, resetA\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
        if (val & ATA_DEVCTL_HOB)
        {
            val &= ~ATA_DEVCTL_HOB;
            Log2(("%s: ignored setting HOB\n", __FUNCTION__));
        }

        /* Save the timestamp we started the reset. */
        pCtl->u64ResetTime = RTTimeMilliTS();

        /* Issue the reset request now. */
        ataHCAsyncIOPutRequest(pCtl, &g_ataResetARequest);
#else /* !IN_RING3 */
        AssertMsgFailed(("RESET handling is too complicated for GC\n"));
#endif /* IN_RING3 */
    }
    else if (   (pCtl->aIfs[0].uATARegDevCtl & ATA_DEVCTL_RESET)
             && !(val & ATA_DEVCTL_RESET))
    {
#ifdef IN_RING3
        /* Software RESET high to low */
        Log(("%s: deasserting RESET\n", __FUNCTION__));
        Log2(("%s: Ctl#%d: message to async I/O thread, resetC\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
        if (val & ATA_DEVCTL_HOB)
        {
            val &= ~ATA_DEVCTL_HOB;
            Log2(("%s: ignored setting HOB\n", __FUNCTION__));
        }
        ataHCAsyncIOPutRequest(pCtl, &g_ataResetCRequest);
#else /* !IN_RING3 */
        AssertMsgFailed(("RESET handling is too complicated for GC\n"));
#endif /* IN_RING3 */
    }

    /* Change of interrupt disable flag. Update interrupt line if interrupt
     * is pending on the current interface. */
    if (   ((val ^ pCtl->aIfs[0].uATARegDevCtl) & ATA_DEVCTL_DISABLE_IRQ)
        && pCtl->aIfs[pCtl->iSelectedIf].fIrqPending)
    {
        if (!(val & ATA_DEVCTL_DISABLE_IRQ))
        {
            Log2(("%s: LUN#%d asserting IRQ (interrupt disable change)\n", __FUNCTION__, pCtl->aIfs[pCtl->iSelectedIf].iLUN));
            /* The BMDMA unit unconditionally sets BM_STATUS_INT if the
             * interrupt line is asserted. It monitors the line for a rising
             * edge. */
            pCtl->BmDma.u8Status |= BM_STATUS_INT;
            if (pCtl->irq == 16)
                PDMDevHlpPCISetIrq(CONTROLLER_2_DEVINS(pCtl), 0, 1);
            else
                PDMDevHlpISASetIrq(CONTROLLER_2_DEVINS(pCtl), pCtl->irq, 1);
        }
        else
        {
            Log2(("%s: LUN#%d deasserting IRQ (interrupt disable change)\n", __FUNCTION__, pCtl->aIfs[pCtl->iSelectedIf].iLUN));
            if (pCtl->irq == 16)
                PDMDevHlpPCISetIrq(CONTROLLER_2_DEVINS(pCtl), 0, 0);
            else
                PDMDevHlpISASetIrq(CONTROLLER_2_DEVINS(pCtl), pCtl->irq, 0);
        }
    }

    if (val & ATA_DEVCTL_HOB)
        Log2(("%s: set HOB\n", __FUNCTION__));

    pCtl->aIfs[0].uATARegDevCtl = val;
    pCtl->aIfs[1].uATARegDevCtl = val;

    return VINF_SUCCESS;
}

#if defined(IN_RING0) || defined(IN_RING3)

static void ataHCPIOTransfer(PATACONTROLLER pCtl)
{
    ATADevState *s;

    s = &pCtl->aIfs[pCtl->iAIOIf];
    Log3(("%s: if=%p\n", __FUNCTION__, s));

    if (s->cbTotalTransfer && s->iIOBufferCur > s->iIOBufferEnd)
    {
# ifdef IN_RING3
        LogRel(("PIIX3 ATA: LUN#%d: %s data in the middle of a PIO transfer - VERY SLOW\n",
                s->iLUN, s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE ? "loading" : "storing"));
        /* Any guest OS that triggers this case has a pathetic ATA driver.
         * In a real system it would block the CPU via IORDY, here we do it
         * very similarly by not continuing with the current instruction
         * until the transfer to/from the storage medium is completed. */
        if (s->iSourceSink != ATAFN_SS_NULL)
        {
            bool fRedo;
            uint8_t status = s->uATARegStatus;
            ataSetStatusValue(s, ATA_STAT_BUSY);
            Log2(("%s: calling source/sink function\n", __FUNCTION__));
            fRedo = g_apfnSourceSinkFuncs[s->iSourceSink](s);
            pCtl->fRedo = fRedo;
            if (RT_UNLIKELY(fRedo))
                return;
            ataSetStatusValue(s, status);
            s->iIOBufferCur = 0;
            s->iIOBufferEnd = s->cbElementaryTransfer;
        }
# else
        AssertReleaseFailed();
# endif
    }
    if (s->cbTotalTransfer)
    {
        if (s->fATAPITransfer)
            ataHCPIOTransferLimitATAPI(s);

        if (s->uTxDir == PDMMEDIATXDIR_TO_DEVICE && s->cbElementaryTransfer > s->cbTotalTransfer)
            s->cbElementaryTransfer = s->cbTotalTransfer;

        Log2(("%s: %s tx_size=%d elem_tx_size=%d index=%d end=%d\n",
             __FUNCTION__, s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE ? "T2I" : "I2T",
             s->cbTotalTransfer, s->cbElementaryTransfer,
             s->iIOBufferCur, s->iIOBufferEnd));
        ataHCPIOTransferStart(s, s->iIOBufferCur, s->cbElementaryTransfer);
        s->cbTotalTransfer -= s->cbElementaryTransfer;
        s->iIOBufferCur += s->cbElementaryTransfer;

        if (s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE && s->cbElementaryTransfer > s->cbTotalTransfer)
            s->cbElementaryTransfer = s->cbTotalTransfer;
    }
    else
        ataHCPIOTransferStop(s);
}


DECLINLINE(void) ataHCPIOTransferFinish(PATACONTROLLER pCtl, ATADevState *s)
{
    /* Do not interfere with RESET processing if the PIO transfer finishes
     * while the RESET line is asserted. */
    if (pCtl->fReset)
    {
        Log2(("%s: Ctl#%d: suppressed continuing PIO transfer as RESET is active\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
        return;
    }

    if (   s->uTxDir == PDMMEDIATXDIR_TO_DEVICE
        || (   s->iSourceSink != ATAFN_SS_NULL
            && s->iIOBufferCur >= s->iIOBufferEnd))
    {
        /* Need to continue the transfer in the async I/O thread. This is
         * the case for write operations or generally for not yet finished
         * transfers (some data might need to be read). */
        ataUnsetStatus(s, ATA_STAT_READY | ATA_STAT_DRQ);
        ataSetStatus(s, ATA_STAT_BUSY);

        Log2(("%s: Ctl#%d: message to async I/O thread, continuing PIO transfer\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
        ataHCAsyncIOPutRequest(pCtl, &g_ataPIORequest);
    }
    else
    {
        /* Either everything finished (though some data might still be pending)
         * or some data is pending before the next read is due. */

        /* Continue a previously started transfer. */
        ataUnsetStatus(s, ATA_STAT_DRQ);
        ataSetStatus(s, ATA_STAT_READY);

        if (s->cbTotalTransfer)
        {
            /* There is more to transfer, happens usually for large ATAPI
             * reads - the protocol limits the chunk size to 65534 bytes. */
            ataHCPIOTransfer(pCtl);
            ataHCSetIRQ(s);
        }
        else
        {
            Log2(("%s: Ctl#%d: skipping message to async I/O thread, ending PIO transfer\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
            /* Finish PIO transfer. */
            ataHCPIOTransfer(pCtl);
            Assert(!pCtl->fRedo);
        }
    }
}

#endif /* IN_RING0 || IN_RING3 */

/**
 * Fallback for ataCopyPioData124 that handles unaligned and out of bounds cases.
 *
 * @param   pIf         The device interface to work with.
 * @param   pbDst       The destination buffer.
 * @param   pbSrc       The source buffer.
 * @param   cbCopy      The number of bytes to copy, either 1, 2 or 4 bytes.
 */
DECL_NO_INLINE(static, void) ataCopyPioData124Slow(ATADevState *pIf, uint8_t *pbDst, const uint8_t *pbSrc, uint32_t cbCopy)
{
    uint32_t const offStart = pIf->iIOBufferPIODataStart;
    uint32_t const offNext  = offStart + cbCopy;

    if (offStart + cbCopy > pIf->cbIOBuffer)
    {
        Log(("%s: cbCopy=%#x offStart=%#x cbIOBuffer=%#x offNext=%#x (iIOBufferPIODataEnd=%#x)\n",
             __FUNCTION__, cbCopy, offStart, pIf->cbIOBuffer, offNext, pIf->iIOBufferPIODataEnd));
        if (offStart < pIf->cbIOBuffer)
            cbCopy = pIf->cbIOBuffer - offStart;
        else
            cbCopy = 0;
    }

    switch (cbCopy)
    {
        case 4: pbDst[3] = pbSrc[3]; RT_FALL_THRU();
        case 3: pbDst[2] = pbSrc[2]; RT_FALL_THRU();
        case 2: pbDst[1] = pbSrc[1]; RT_FALL_THRU();
        case 1: pbDst[0] = pbSrc[0]; RT_FALL_THRU();
        case 0: break;
        default: AssertFailed(); /* impossible */
    }

    pIf->iIOBufferPIODataStart = offNext;

}


/**
 * Work for ataDataWrite & ataDataRead that copies data without using memcpy.
 *
 * This also updates pIf->iIOBufferPIODataStart.
 *
 * The two buffers are either stack (32-bit aligned) or somewhere within
 * pIf->pbIOBuffer.
 *
 * @param   pIf         The device interface to work with.
 * @param   pbDst       The destination buffer.
 * @param   pbSrc       The source buffer.
 * @param   cbCopy      The number of bytes to copy, either 1, 2 or 4 bytes.
 */
DECLINLINE(void) ataCopyPioData124(ATADevState *pIf, uint8_t *pbDst, const uint8_t *pbSrc, uint32_t cbCopy)
{
    /*
     * Quick bounds checking can be done by checking that the pbIOBuffer offset
     * (iIOBufferPIODataStart) is aligned at the transfer size (which is ASSUMED
     * to be 1, 2 or 4).  However, since we're paranoid and don't currently
     * trust iIOBufferPIODataEnd to be within bounds, we current check against the
     * IO buffer size too.
     */
    Assert(cbCopy == 1 || cbCopy == 2 || cbCopy == 4);
    uint32_t const offStart = pIf->iIOBufferPIODataStart;
    if (RT_LIKELY(   !(offStart & (cbCopy - 1))
                  && offStart + cbCopy <= pIf->cbIOBuffer))
    {
        switch (cbCopy)
        {
            case 4: *(uint32_t *)pbDst = *(uint32_t const *)pbSrc; break;
            case 2: *(uint16_t *)pbDst = *(uint16_t const *)pbSrc; break;
            case 1: *pbDst = *pbSrc; break;
        }
        pIf->iIOBufferPIODataStart = offStart + cbCopy;
    }
    else
        ataCopyPioData124Slow(pIf, pbDst, pbSrc, cbCopy);
}


/**
 * Port I/O Handler for primary port range OUT operations.
 * @see FNIOMIOPORTOUT for details.
 */
PDMBOTHCBDECL(int) ataIOPortWrite1Data(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    uint32_t       i = (uint32_t)(uintptr_t)pvUser;
    PCIATAState   *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    PATACONTROLLER pCtl = &pThis->aCts[i];
    RT_NOREF1(Port);

    Assert(i < 2);
    Assert(Port == pCtl->IOPortBase1);
    Assert(cb == 2 || cb == 4); /* Writes to the data port may be 16-bit or 32-bit. */

    int rc = PDMCritSectEnter(&pCtl->lock, VINF_IOM_R3_IOPORT_WRITE);
    if (rc == VINF_SUCCESS)
    {
        ATADevState *s = &pCtl->aIfs[pCtl->iSelectedIf];

        if (s->iIOBufferPIODataStart < s->iIOBufferPIODataEnd)
        {
            Assert(s->uTxDir == PDMMEDIATXDIR_TO_DEVICE);
            uint8_t       *pbDst = s->CTX_SUFF(pbIOBuffer) + s->iIOBufferPIODataStart;
            uint8_t const *pbSrc = (uint8_t const *)&u32;

#ifdef IN_RC
            /* Raw-mode: The ataHCPIOTransfer following the last transfer unit
               requires I/O thread signalling, we must go to ring-3 for that. */
            if (s->iIOBufferPIODataStart + cb < s->iIOBufferPIODataEnd)
                ataCopyPioData124(s, pbDst, pbSrc, cb);
            else
                rc = VINF_IOM_R3_IOPORT_WRITE;

#elif defined(IN_RING0)
            /* Ring-0: We can do I/O thread signalling here, however for paranoid reasons
               triggered by a special case in ataHCPIOTransferFinish, we take extra care here. */
            if (s->iIOBufferPIODataStart + cb < s->iIOBufferPIODataEnd)
                ataCopyPioData124(s, pbDst, pbSrc, cb);
            else if (s->uTxDir == PDMMEDIATXDIR_TO_DEVICE) /* paranoia */
            {
                ataCopyPioData124(s, pbDst, pbSrc, cb);
                ataHCPIOTransferFinish(pCtl, s);
            }
            else
            {
                Log(("%s: Unexpected\n",__FUNCTION__));
                rc = VINF_IOM_R3_IOPORT_WRITE;
            }

#else  /* IN_RING 3*/
            ataCopyPioData124(s, pbDst, pbSrc, cb);
            if (s->iIOBufferPIODataStart >= s->iIOBufferPIODataEnd)
                ataHCPIOTransferFinish(pCtl, s);
#endif /* IN_RING 3*/
        }
        else
            Log2(("%s: DUMMY data\n", __FUNCTION__));

        Log3(("%s: addr=%#x val=%.*Rhxs rc=%d\n", __FUNCTION__, Port, cb, &u32, rc));
        PDMCritSectLeave(&pCtl->lock);
    }
    else
        Log3(("%s: addr=%#x -> %d\n", __FUNCTION__, Port, rc));
    return rc;
}


/**
 * Port I/O Handler for primary port range IN operations.
 * @see FNIOMIOPORTIN for details.
 */
PDMBOTHCBDECL(int) ataIOPortRead1Data(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    uint32_t       i = (uint32_t)(uintptr_t)pvUser;
    PCIATAState   *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    PATACONTROLLER pCtl = &pThis->aCts[i];
    RT_NOREF1(Port);

    Assert(i < 2);
    Assert(Port == pCtl->IOPortBase1);

    /* Reads from the data register may be 16-bit or 32-bit. Byte accesses are
       upgraded to word. */
    Assert(cb == 1 || cb == 2 || cb == 4);
    uint32_t cbActual = cb != 1 ? cb : 2;
    *pu32 = 0;

    int rc = PDMCritSectEnter(&pCtl->lock, VINF_IOM_R3_IOPORT_READ);
    if (rc == VINF_SUCCESS)
    {
        ATADevState *s = &pCtl->aIfs[pCtl->iSelectedIf];

        if (s->iIOBufferPIODataStart < s->iIOBufferPIODataEnd)
        {
            Assert(s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE);
            uint8_t const *pbSrc = s->CTX_SUFF(pbIOBuffer) + s->iIOBufferPIODataStart;
            uint8_t       *pbDst = (uint8_t *)pu32;

#ifdef IN_RC
            /* All but the last transfer unit is simple enough for RC, but
             * sending a request to the async IO thread is too complicated. */
            if (s->iIOBufferPIODataStart + cbActual < s->iIOBufferPIODataEnd)
                ataCopyPioData124(s, pbDst, pbSrc, cbActual);
            else
                rc = VINF_IOM_R3_IOPORT_READ;

#elif defined(IN_RING0)
            /* Ring-0: We can do I/O thread signalling here.  However there is one
               case in ataHCPIOTransfer that does a LogRel and would (but not from
               here) call directly into the driver code.  We detect that odd case
               here cand return to ring-3 to handle it. */
            if (s->iIOBufferPIODataStart + cbActual < s->iIOBufferPIODataEnd)
                ataCopyPioData124(s, pbDst, pbSrc, cbActual);
            else if (   s->cbTotalTransfer == 0
                     || s->iSourceSink != ATAFN_SS_NULL
                     || s->iIOBufferCur <= s->iIOBufferEnd)
            {
                ataCopyPioData124(s, pbDst, pbSrc, cbActual);
                ataHCPIOTransferFinish(pCtl, s);
            }
            else
            {
                Log(("%s: Unexpected\n",__FUNCTION__));
                rc = VINF_IOM_R3_IOPORT_READ;
            }

#else  /* IN_RING3 */
            ataCopyPioData124(s, pbDst, pbSrc, cbActual);
            if (s->iIOBufferPIODataStart >= s->iIOBufferPIODataEnd)
                ataHCPIOTransferFinish(pCtl, s);
#endif /* IN_RING3 */

            /* Just to be on the safe side (caller takes care of this, really). */
            if (cb == 1)
                *pu32 &= 0xff;
        }
        else
        {
            Log2(("%s: DUMMY data\n", __FUNCTION__));
            memset(pu32, 0xff, cb);
        }
        Log3(("%s: addr=%#x val=%.*Rhxs rc=%d\n", __FUNCTION__, Port, cb, pu32, rc));

        PDMCritSectLeave(&pCtl->lock);
    }
    else
        Log3(("%s: addr=%#x -> %d\n", __FUNCTION__, Port, rc));

    return rc;
}


/**
 * Port I/O Handler for primary port range IN string operations.
 * @see FNIOMIOPORTINSTRING for details.
 */
PDMBOTHCBDECL(int) ataIOPortReadStr1Data(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint8_t *pbDst,
                                         uint32_t *pcTransfers, unsigned cb)
{
    uint32_t       i     = (uint32_t)(uintptr_t)pvUser;
    PCIATAState   *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    PATACONTROLLER pCtl  = &pThis->aCts[i];
    RT_NOREF1(Port);

    Assert(i < 2);
    Assert(Port == pCtl->IOPortBase1);
    Assert(*pcTransfers > 0);

    int rc;
    if (cb == 2 || cb == 4)
    {
        rc = PDMCritSectEnter(&pCtl->lock, VINF_IOM_R3_IOPORT_READ);
        if (rc == VINF_SUCCESS)
        {
            ATADevState *s = &pCtl->aIfs[pCtl->iSelectedIf];

            uint32_t const offStart = s->iIOBufferPIODataStart;
            if (offStart < s->iIOBufferPIODataEnd)
            {
                /*
                 * Figure how much we can copy.  Usually it's the same as the request.
                 * The last transfer unit cannot be handled in RC, as it involves
                 * thread communication.  In R0 we let the non-string callback handle it,
                 * and ditto for overflows/dummy data.
                 */
                uint32_t cAvailable = (s->iIOBufferPIODataEnd - offStart) / cb;
#ifndef IN_RING3
                if (cAvailable > 0)
                    cAvailable--;
#endif
                uint32_t const cRequested = *pcTransfers;
                if (cAvailable > cRequested)
                    cAvailable = cRequested;
                uint32_t const cbTransfer = cAvailable * cb;
                if (   offStart + cbTransfer <= s->cbIOBuffer
                    && cbTransfer > 0)
                {
                    /*
                     * Do the transfer.
                     */
                    uint8_t const *pbSrc = s->CTX_SUFF(pbIOBuffer) + offStart;
                    memcpy(pbDst, pbSrc, cbTransfer);
                    Log3(("%s: addr=%#x cb=%#x cbTransfer=%#x val=%.*Rhxd\n",
                          __FUNCTION__, Port, cb, cbTransfer, cbTransfer, pbSrc));
                    s->iIOBufferPIODataStart = offStart + cbTransfer;

#ifdef IN_RING3
                    if (s->iIOBufferPIODataStart >= s->iIOBufferPIODataEnd)
                        ataHCPIOTransferFinish(pCtl, s);
#endif
                    *pcTransfers = cRequested - cAvailable;
                }
                else
                    Log2(("ataIOPortReadStr1Data: DUMMY/Overflow!\n"));
            }
            else
            {
                /*
                 * Dummy read (shouldn't happen) return 0xff like the non-string handler.
                 */
                Log2(("ataIOPortReadStr1Data: DUMMY data (%#x bytes)\n", *pcTransfers * cb));
                memset(pbDst, 0xff, *pcTransfers * cb);
                *pcTransfers = 0;
            }

            PDMCritSectLeave(&pCtl->lock);
        }
    }
    /*
     * Let the non-string I/O callback handle 1 byte reads.
     */
    else
    {
        Log2(("ataIOPortReadStr1Data: 1 byte read (%#x transfers)\n", *pcTransfers));
        AssertFailed();
        rc = VINF_SUCCESS;
    }
    return rc;
}


/**
 * Port I/O Handler for primary port range OUT string operations.
 * @see FNIOMIOPORTOUTSTRING for details.
 */
PDMBOTHCBDECL(int) ataIOPortWriteStr1Data(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint8_t const *pbSrc,
                                          uint32_t *pcTransfers, unsigned cb)
{
    uint32_t       i     = (uint32_t)(uintptr_t)pvUser;
    PCIATAState   *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    PATACONTROLLER pCtl  = &pThis->aCts[i];
    RT_NOREF1(Port);

    Assert(i < 2);
    Assert(Port == pCtl->IOPortBase1);
    Assert(*pcTransfers > 0);

    int rc;
    if (cb == 2 || cb == 4)
    {
        rc = PDMCritSectEnter(&pCtl->lock, VINF_IOM_R3_IOPORT_WRITE);
        if (rc == VINF_SUCCESS)
        {
            ATADevState *s = &pCtl->aIfs[pCtl->iSelectedIf];

            uint32_t const offStart = s->iIOBufferPIODataStart;
            if (offStart < s->iIOBufferPIODataEnd)
            {
                /*
                 * Figure how much we can copy.  Usually it's the same as the request.
                 * The last transfer unit cannot be handled in RC, as it involves
                 * thread communication.  In R0 we let the non-string callback handle it,
                 * and ditto for overflows/dummy data.
                 */
                uint32_t cAvailable = (s->iIOBufferPIODataEnd - offStart) / cb;
#ifndef IN_RING3
                if (cAvailable)
                    cAvailable--;
#endif
                uint32_t const cRequested = *pcTransfers;
                if (cAvailable > cRequested)
                    cAvailable = cRequested;
                uint32_t const cbTransfer = cAvailable * cb;
                if (   offStart + cbTransfer <= s->cbIOBuffer
                    && cbTransfer)
                {
                    /*
                     * Do the transfer.
                     */
                    void *pvDst = s->CTX_SUFF(pbIOBuffer) + offStart;
                    memcpy(pvDst, pbSrc, cbTransfer);
                    Log3(("%s: addr=%#x val=%.*Rhxs\n", __FUNCTION__, Port, cbTransfer, pvDst));
                    s->iIOBufferPIODataStart = offStart + cbTransfer;

#ifdef IN_RING3
                    if (s->iIOBufferPIODataStart >= s->iIOBufferPIODataEnd)
                        ataHCPIOTransferFinish(pCtl, s);
#endif
                    *pcTransfers = cRequested - cAvailable;
                }
                else
                    Log2(("ataIOPortWriteStr1Data: DUMMY/Overflow!\n"));
            }
            else
            {
                Log2(("ataIOPortWriteStr1Data: DUMMY data (%#x bytes)\n", *pcTransfers * cb));
                *pcTransfers = 0;
            }

            PDMCritSectLeave(&pCtl->lock);
        }
    }
    /*
     * Let the non-string I/O callback handle 1 byte reads.
     */
    else
    {
        Log2(("ataIOPortWriteStr1Data: 1 byte write (%#x transfers)\n", *pcTransfers));
        AssertFailed();
        rc = VINF_SUCCESS;
    }

    return rc;
}


#ifdef IN_RING3

static void ataR3DMATransferStop(ATADevState *s)
{
    s->cbTotalTransfer = 0;
    s->cbElementaryTransfer = 0;
    s->iBeginTransfer = ATAFN_BT_NULL;
    s->iSourceSink = ATAFN_SS_NULL;
}


/**
 * Perform the entire DMA transfer in one go (unless a source/sink operation
 * has to be redone or a RESET comes in between). Unlike the PIO counterpart
 * this function cannot handle empty transfers.
 *
 * @param pCtl      Controller for which to perform the transfer.
 */
static void ataR3DMATransfer(PATACONTROLLER pCtl)
{
    PPDMDEVINS pDevIns = CONTROLLER_2_DEVINS(pCtl);
    ATADevState *s = &pCtl->aIfs[pCtl->iAIOIf];
    bool fRedo;
    RTGCPHYS32 GCPhysDesc;
    uint32_t cbTotalTransfer, cbElementaryTransfer;
    uint32_t iIOBufferCur, iIOBufferEnd;
    PDMMEDIATXDIR uTxDir;
    bool fLastDesc = false;

    Assert(sizeof(BMDMADesc) == 8);

    fRedo = pCtl->fRedo;
    if (RT_LIKELY(!fRedo))
        Assert(s->cbTotalTransfer);
    uTxDir = (PDMMEDIATXDIR)s->uTxDir;
    cbTotalTransfer = s->cbTotalTransfer;
    cbElementaryTransfer = s->cbElementaryTransfer;
    iIOBufferCur = s->iIOBufferCur;
    iIOBufferEnd = s->iIOBufferEnd;

    /* The DMA loop is designed to hold the lock only when absolutely
     * necessary. This avoids long freezes should the guest access the
     * ATA registers etc. for some reason. */
    ataR3LockLeave(pCtl);

    Log2(("%s: %s tx_size=%d elem_tx_size=%d index=%d end=%d\n",
         __FUNCTION__, uTxDir == PDMMEDIATXDIR_FROM_DEVICE ? "T2I" : "I2T",
         cbTotalTransfer, cbElementaryTransfer,
         iIOBufferCur, iIOBufferEnd));
    for (GCPhysDesc = pCtl->GCPhysFirstDMADesc;
         GCPhysDesc <= pCtl->GCPhysLastDMADesc;
         GCPhysDesc += sizeof(BMDMADesc))
    {
        BMDMADesc DMADesc;
        RTGCPHYS32 GCPhysBuffer;
        uint32_t cbBuffer;

        if (RT_UNLIKELY(fRedo))
        {
            GCPhysBuffer = pCtl->GCPhysRedoDMABuffer;
            cbBuffer = pCtl->cbRedoDMABuffer;
            fLastDesc = pCtl->fRedoDMALastDesc;
            DMADesc.GCPhysBuffer = DMADesc.cbBuffer = 0; /* Shut up MSC. */
        }
        else
        {
            PDMDevHlpPhysRead(pDevIns, GCPhysDesc, &DMADesc, sizeof(BMDMADesc));
            GCPhysBuffer = RT_LE2H_U32(DMADesc.GCPhysBuffer);
            cbBuffer = RT_LE2H_U32(DMADesc.cbBuffer);
            fLastDesc = !!(cbBuffer & 0x80000000);
            cbBuffer &= 0xfffe;
            if (cbBuffer == 0)
                cbBuffer = 0x10000;
            if (cbBuffer > cbTotalTransfer)
                cbBuffer = cbTotalTransfer;
        }

        while (RT_UNLIKELY(fRedo) || (cbBuffer && cbTotalTransfer))
        {
            if (RT_LIKELY(!fRedo))
            {
                uint32_t cbXfer = RT_MIN(cbBuffer, iIOBufferEnd - iIOBufferCur);
                Log2(("%s: DMA desc %#010x: addr=%#010x size=%#010x orig_size=%#010x\n", __FUNCTION__,
                       (int)GCPhysDesc, GCPhysBuffer, cbBuffer, RT_LE2H_U32(DMADesc.cbBuffer) & 0xfffe));

                if (uTxDir == PDMMEDIATXDIR_FROM_DEVICE)
                    PDMDevHlpPCIPhysWrite(pDevIns, GCPhysBuffer, s->CTX_SUFF(pbIOBuffer) + iIOBufferCur, cbXfer);
                else
                    PDMDevHlpPCIPhysRead(pDevIns, GCPhysBuffer, s->CTX_SUFF(pbIOBuffer) + iIOBufferCur, cbXfer);

                iIOBufferCur    += cbXfer;
                cbTotalTransfer -= cbXfer;
                cbBuffer        -= cbXfer;
                GCPhysBuffer    += cbXfer;
            }
            if (    iIOBufferCur == iIOBufferEnd
                &&  (uTxDir == PDMMEDIATXDIR_TO_DEVICE || cbTotalTransfer))
            {
                if (uTxDir == PDMMEDIATXDIR_FROM_DEVICE && cbElementaryTransfer > cbTotalTransfer)
                    cbElementaryTransfer = cbTotalTransfer;

                ataR3LockEnter(pCtl);

                /* The RESET handler could have cleared the DMA transfer
                 * state (since we didn't hold the lock until just now
                 * the guest can continue in parallel). If so, the state
                 * is already set up so the loop is exited immediately. */
                if (s->iSourceSink != ATAFN_SS_NULL)
                {
                    s->iIOBufferCur = iIOBufferCur;
                    s->iIOBufferEnd = iIOBufferEnd;
                    s->cbElementaryTransfer = cbElementaryTransfer;
                    s->cbTotalTransfer = cbTotalTransfer;
                    Log2(("%s: calling source/sink function\n", __FUNCTION__));
                    fRedo = g_apfnSourceSinkFuncs[s->iSourceSink](s);
                    if (RT_UNLIKELY(fRedo))
                    {
                        pCtl->GCPhysFirstDMADesc = GCPhysDesc;
                        pCtl->GCPhysRedoDMABuffer = GCPhysBuffer;
                        pCtl->cbRedoDMABuffer = cbBuffer;
                        pCtl->fRedoDMALastDesc = fLastDesc;
                    }
                    else
                    {
                        cbTotalTransfer = s->cbTotalTransfer;
                        cbElementaryTransfer = s->cbElementaryTransfer;

                        if (uTxDir == PDMMEDIATXDIR_TO_DEVICE && cbElementaryTransfer > cbTotalTransfer)
                            cbElementaryTransfer = cbTotalTransfer;
                        iIOBufferCur = 0;
                        iIOBufferEnd = cbElementaryTransfer;
                    }
                    pCtl->fRedo = fRedo;
                }
                else
                {
                    /* This forces the loop to exit immediately. */
                    GCPhysDesc = pCtl->GCPhysLastDMADesc + 1;
                }

                ataR3LockLeave(pCtl);
                if (RT_UNLIKELY(fRedo))
                    break;
            }
        }

        if (RT_UNLIKELY(fRedo))
            break;

        /* end of transfer */
        if (!cbTotalTransfer || fLastDesc)
            break;

        ataR3LockEnter(pCtl);

        if (!(pCtl->BmDma.u8Cmd & BM_CMD_START) || pCtl->fReset)
        {
            LogRel(("PIIX3 ATA: Ctl#%d: ABORT DMA%s\n", ATACONTROLLER_IDX(pCtl), pCtl->fReset ? " due to RESET" : ""));
            if (!pCtl->fReset)
                ataR3DMATransferStop(s);
            /* This forces the loop to exit immediately. */
            GCPhysDesc = pCtl->GCPhysLastDMADesc + 1;
        }

        ataR3LockLeave(pCtl);
    }

    ataR3LockEnter(pCtl);
    if (RT_UNLIKELY(fRedo))
        return;

    if (fLastDesc)
        pCtl->BmDma.u8Status &= ~BM_STATUS_DMAING;
    s->cbTotalTransfer = cbTotalTransfer;
    s->cbElementaryTransfer = cbElementaryTransfer;
    s->iIOBufferCur = iIOBufferCur;
    s->iIOBufferEnd = iIOBufferEnd;
}

/**
 * Signal PDM that we're idle (if we actually are).
 *
 * @param   pCtl        The controller.
 */
static void ataR3AsyncSignalIdle(PATACONTROLLER pCtl)
{
    /*
     * Take the lock here and recheck the idle indicator to avoid
     * unnecessary work and racing ataR3WaitForAsyncIOIsIdle.
     */
    int rc = PDMCritSectEnter(&pCtl->AsyncIORequestLock, VINF_SUCCESS);
    AssertRC(rc);

    if (    pCtl->fSignalIdle
        &&  ataR3AsyncIOIsIdle(pCtl, false /*fStrict*/))
    {
        PDMDevHlpAsyncNotificationCompleted(pCtl->pDevInsR3);
        RTThreadUserSignal(pCtl->AsyncIOThread); /* for ataR3Construct/ataR3ResetCommon. */
    }

    rc = PDMCritSectLeave(&pCtl->AsyncIORequestLock);
    AssertRC(rc);
}

/**
 * Async I/O thread for an interface.
 *
 * Once upon a time this was readable code with several loops and a different
 * semaphore for each purpose. But then came the "how can one save the state in
 * the middle of a PIO transfer" question.  The solution was to use an ASM,
 * which is what's there now.
 */
static DECLCALLBACK(int) ataR3AsyncIOThread(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF1(hThreadSelf);
    const ATARequest *pReq;
    uint64_t        u64TS = 0; /* shut up gcc */
    uint64_t        uWait;
    int             rc   = VINF_SUCCESS;
    PATACONTROLLER  pCtl = (PATACONTROLLER)pvUser;
    ATADevState     *s;

    pReq = NULL;
    pCtl->fChainedTransfer = false;
    while (!pCtl->fShutdown)
    {
        /* Keep this thread from doing anything as long as EMT is suspended. */
        while (pCtl->fRedoIdle)
        {
            if (pCtl->fSignalIdle)
                ataR3AsyncSignalIdle(pCtl);
            rc = RTSemEventWait(pCtl->SuspendIOSem, RT_INDEFINITE_WAIT);
            /* Continue if we got a signal by RTThreadPoke().
             * We will get notified if there is a request to process.
             */
            if (RT_UNLIKELY(rc == VERR_INTERRUPTED))
                continue;
            if (RT_FAILURE(rc) || pCtl->fShutdown)
                break;

            pCtl->fRedoIdle = false;
        }

        /* Wait for work.  */
        while (pReq == NULL)
        {
            if (pCtl->fSignalIdle)
                ataR3AsyncSignalIdle(pCtl);
            rc = SUPSemEventWaitNoResume(pCtl->pSupDrvSession, pCtl->hAsyncIOSem, RT_INDEFINITE_WAIT);
            /* Continue if we got a signal by RTThreadPoke().
             * We will get notified if there is a request to process.
             */
            if (RT_UNLIKELY(rc == VERR_INTERRUPTED))
                continue;
            if (RT_FAILURE(rc) || RT_UNLIKELY(pCtl->fShutdown))
                break;

            pReq = ataR3AsyncIOGetCurrentRequest(pCtl);
        }

        if (RT_FAILURE(rc) || pCtl->fShutdown)
            break;

        if (pReq == NULL)
            continue;

        ATAAIO ReqType = pReq->ReqType;

        Log2(("%s: Ctl#%d: state=%d, req=%d\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl), pCtl->uAsyncIOState, ReqType));
        if (pCtl->uAsyncIOState != ReqType)
        {
            /* The new state is not the state that was expected by the normal
             * state changes. This is either a RESET/ABORT or there's something
             * really strange going on. */
            if (    (pCtl->uAsyncIOState == ATA_AIO_PIO || pCtl->uAsyncIOState == ATA_AIO_DMA)
                &&  (ReqType == ATA_AIO_PIO || ReqType == ATA_AIO_DMA))
            {
                /* Incorrect sequence of PIO/DMA states. Dump request queue. */
                ataR3AsyncIODumpRequests(pCtl);
            }
            AssertReleaseMsg(   ReqType == ATA_AIO_RESET_ASSERTED
                             || ReqType == ATA_AIO_RESET_CLEARED
                             || ReqType == ATA_AIO_ABORT
                             || pCtl->uAsyncIOState == ReqType,
                             ("I/O state inconsistent: state=%d request=%d\n", pCtl->uAsyncIOState, ReqType));
        }

        /* Do our work.  */
        ataR3LockEnter(pCtl);

        if (pCtl->uAsyncIOState == ATA_AIO_NEW && !pCtl->fChainedTransfer)
        {
            u64TS = RTTimeNanoTS();
#if defined(DEBUG) || defined(VBOX_WITH_STATISTICS)
            STAM_PROFILE_ADV_START(&pCtl->StatAsyncTime, a);
#endif
        }

        switch (ReqType)
        {
            case ATA_AIO_NEW:

                pCtl->iAIOIf = pReq->u.t.iIf;
                s = &pCtl->aIfs[pCtl->iAIOIf];
                s->cbTotalTransfer = pReq->u.t.cbTotalTransfer;
                s->uTxDir = pReq->u.t.uTxDir;
                s->iBeginTransfer = pReq->u.t.iBeginTransfer;
                s->iSourceSink = pReq->u.t.iSourceSink;
                s->iIOBufferEnd = 0;
                s->u64CmdTS = u64TS;

                if (s->fATAPI)
                {
                    if (pCtl->fChainedTransfer)
                    {
                        /* Only count the actual transfers, not the PIO
                         * transfer of the ATAPI command bytes. */
                        if (s->fDMA)
                            STAM_REL_COUNTER_INC(&s->StatATAPIDMA);
                        else
                            STAM_REL_COUNTER_INC(&s->StatATAPIPIO);
                    }
                }
                else
                {
                    if (s->fDMA)
                        STAM_REL_COUNTER_INC(&s->StatATADMA);
                    else
                        STAM_REL_COUNTER_INC(&s->StatATAPIO);
                }

                pCtl->fChainedTransfer = false;

                if (s->iBeginTransfer != ATAFN_BT_NULL)
                {
                    Log2(("%s: Ctl#%d: calling begin transfer function\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
                    g_apfnBeginTransFuncs[s->iBeginTransfer](s);
                    s->iBeginTransfer = ATAFN_BT_NULL;
                    if (s->uTxDir != PDMMEDIATXDIR_FROM_DEVICE)
                        s->iIOBufferEnd = s->cbElementaryTransfer;
                }
                else
                {
                    s->cbElementaryTransfer = s->cbTotalTransfer;
                    s->iIOBufferEnd = s->cbTotalTransfer;
                }
                s->iIOBufferCur = 0;

                if (s->uTxDir != PDMMEDIATXDIR_TO_DEVICE)
                {
                    if (s->iSourceSink != ATAFN_SS_NULL)
                    {
                        bool fRedo;
                        Log2(("%s: Ctl#%d: calling source/sink function\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
                        fRedo = g_apfnSourceSinkFuncs[s->iSourceSink](s);
                        pCtl->fRedo = fRedo;
                        if (RT_UNLIKELY(fRedo && !pCtl->fReset))
                        {
                            /* Operation failed at the initial transfer, restart
                             * everything from scratch by resending the current
                             * request. Occurs very rarely, not worth optimizing. */
                            LogRel(("%s: Ctl#%d: redo entire operation\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
                            ataHCAsyncIOPutRequest(pCtl, pReq);
                            break;
                        }
                    }
                    else
                        ataR3CmdOK(s, 0);
                    s->iIOBufferEnd = s->cbElementaryTransfer;

                }

                /* Do not go into the transfer phase if RESET is asserted.
                 * The CritSect is released while waiting for the host OS
                 * to finish the I/O, thus RESET is possible here. Most
                 * important: do not change uAsyncIOState. */
                if (pCtl->fReset)
                    break;

                if (s->fDMA)
                {
                    if (s->cbTotalTransfer)
                    {
                        ataSetStatus(s, ATA_STAT_DRQ);

                        pCtl->uAsyncIOState = ATA_AIO_DMA;
                        /* If BMDMA is already started, do the transfer now. */
                        if (pCtl->BmDma.u8Cmd & BM_CMD_START)
                        {
                            Log2(("%s: Ctl#%d: message to async I/O thread, continuing DMA transfer immediately\n",
                                  __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
                            ataHCAsyncIOPutRequest(pCtl, &g_ataDMARequest);
                        }
                    }
                    else
                    {
                        Assert(s->uTxDir == PDMMEDIATXDIR_NONE); /* Any transfer which has an initial transfer size of 0 must be marked as such. */
                        /* Finish DMA transfer. */
                        ataR3DMATransferStop(s);
                        ataHCSetIRQ(s);
                        pCtl->uAsyncIOState = ATA_AIO_NEW;
                    }
                }
                else
                {
                    if (s->cbTotalTransfer)
                    {
                        ataHCPIOTransfer(pCtl);
                        Assert(!pCtl->fRedo);
                        if (s->fATAPITransfer || s->uTxDir != PDMMEDIATXDIR_TO_DEVICE)
                            ataHCSetIRQ(s);

                        if (s->uTxDir == PDMMEDIATXDIR_TO_DEVICE || s->iSourceSink != ATAFN_SS_NULL)
                        {
                            /* Write operations and not yet finished transfers
                             * must be completed in the async I/O thread. */
                            pCtl->uAsyncIOState = ATA_AIO_PIO;
                        }
                        else
                        {
                            /* Finished read operation can be handled inline
                             * in the end of PIO transfer handling code. Linux
                             * depends on this, as it waits only briefly for
                             * devices to become ready after incoming data
                             * transfer. Cannot find anything in the ATA spec
                             * that backs this assumption, but as all kernels
                             * are affected (though most of the time it does
                             * not cause any harm) this must work. */
                            pCtl->uAsyncIOState = ATA_AIO_NEW;
                        }
                    }
                    else
                    {
                        Assert(s->uTxDir == PDMMEDIATXDIR_NONE); /* Any transfer which has an initial transfer size of 0 must be marked as such. */
                        /* Finish PIO transfer. */
                        ataHCPIOTransfer(pCtl);
                        Assert(!pCtl->fRedo);
                        if (!s->fATAPITransfer)
                            ataHCSetIRQ(s);
                        pCtl->uAsyncIOState = ATA_AIO_NEW;
                    }
                }
                break;

            case ATA_AIO_DMA:
            {
                BMDMAState *bm = &pCtl->BmDma;
                s = &pCtl->aIfs[pCtl->iAIOIf]; /* Do not remove or there's an instant crash after loading the saved state */
                ATAFNSS iOriginalSourceSink = (ATAFNSS)s->iSourceSink; /* Used by the hack below, but gets reset by then. */

                if (s->uTxDir == PDMMEDIATXDIR_FROM_DEVICE)
                    AssertRelease(bm->u8Cmd & BM_CMD_WRITE);
                else
                    AssertRelease(!(bm->u8Cmd & BM_CMD_WRITE));

                if (RT_LIKELY(!pCtl->fRedo))
                {
                    /* The specs say that the descriptor table must not cross a
                     * 4K boundary. */
                    pCtl->GCPhysFirstDMADesc = bm->GCPhysAddr;
                    pCtl->GCPhysLastDMADesc = RT_ALIGN_32(bm->GCPhysAddr + 1, _4K) - sizeof(BMDMADesc);
                }
                ataR3DMATransfer(pCtl);

                if (RT_UNLIKELY(pCtl->fRedo && !pCtl->fReset))
                {
                    LogRel(("PIIX3 ATA: Ctl#%d: redo DMA operation\n", ATACONTROLLER_IDX(pCtl)));
                    ataHCAsyncIOPutRequest(pCtl, &g_ataDMARequest);
                    break;
                }

                /* The infamous delay IRQ hack. */
                if (   iOriginalSourceSink == ATAFN_SS_WRITE_SECTORS
                    && s->cbTotalTransfer == 0
                    && pCtl->DelayIRQMillies)
                {
                    /* Delay IRQ for writing. Required to get the Win2K
                     * installation work reliably (otherwise it crashes,
                     * usually during component install). So far no better
                     * solution has been found. */
                    Log(("%s: delay IRQ hack\n", __FUNCTION__));
                    ataR3LockLeave(pCtl);
                    RTThreadSleep(pCtl->DelayIRQMillies);
                    ataR3LockEnter(pCtl);
                }

                ataUnsetStatus(s, ATA_STAT_DRQ);
                Assert(!pCtl->fChainedTransfer);
                Assert(s->iSourceSink == ATAFN_SS_NULL);
                if (s->fATAPITransfer)
                {
                    s->uATARegNSector = (s->uATARegNSector & ~7) | ATAPI_INT_REASON_IO | ATAPI_INT_REASON_CD;
                    Log2(("%s: Ctl#%d: interrupt reason %#04x\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl), s->uATARegNSector));
                    s->fATAPITransfer = false;
                }
                ataHCSetIRQ(s);
                pCtl->uAsyncIOState = ATA_AIO_NEW;
                break;
            }

            case ATA_AIO_PIO:
                s = &pCtl->aIfs[pCtl->iAIOIf]; /* Do not remove or there's an instant crash after loading the saved state */

                if (s->iSourceSink != ATAFN_SS_NULL)
                {
                    bool fRedo;
                    Log2(("%s: Ctl#%d: calling source/sink function\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
                    fRedo = g_apfnSourceSinkFuncs[s->iSourceSink](s);
                    pCtl->fRedo = fRedo;
                    if (RT_UNLIKELY(fRedo && !pCtl->fReset))
                    {
                        LogRel(("PIIX3 ATA: Ctl#%d: redo PIO operation\n", ATACONTROLLER_IDX(pCtl)));
                        ataHCAsyncIOPutRequest(pCtl, &g_ataPIORequest);
                        break;
                    }
                    s->iIOBufferCur = 0;
                    s->iIOBufferEnd = s->cbElementaryTransfer;
                }
                else
                {
                    /* Continue a previously started transfer. */
                    ataUnsetStatus(s, ATA_STAT_BUSY);
                    ataSetStatus(s, ATA_STAT_READY);
                }

                /* It is possible that the drives on this controller get RESET
                 * during the above call to the source/sink function. If that's
                 * the case, don't restart the transfer and don't finish it the
                 * usual way. RESET handling took care of all that already.
                 * Most important: do not change uAsyncIOState. */
                if (pCtl->fReset)
                    break;

                if (s->cbTotalTransfer)
                {
                    ataHCPIOTransfer(pCtl);
                    ataHCSetIRQ(s);

                    if (s->uTxDir == PDMMEDIATXDIR_TO_DEVICE || s->iSourceSink != ATAFN_SS_NULL)
                    {
                        /* Write operations and not yet finished transfers
                         * must be completed in the async I/O thread. */
                        pCtl->uAsyncIOState = ATA_AIO_PIO;
                    }
                    else
                    {
                        /* Finished read operation can be handled inline
                         * in the end of PIO transfer handling code. Linux
                         * depends on this, as it waits only briefly for
                         * devices to become ready after incoming data
                         * transfer. Cannot find anything in the ATA spec
                         * that backs this assumption, but as all kernels
                         * are affected (though most of the time it does
                         * not cause any harm) this must work. */
                        pCtl->uAsyncIOState = ATA_AIO_NEW;
                    }
                }
                else
                {
                    /* Finish PIO transfer. */
                    ataHCPIOTransfer(pCtl);
                    if (    !pCtl->fChainedTransfer
                        &&  !s->fATAPITransfer
                        &&  s->uTxDir != PDMMEDIATXDIR_FROM_DEVICE)
                    {
                            ataHCSetIRQ(s);
                    }
                    pCtl->uAsyncIOState = ATA_AIO_NEW;
                }
                break;

            case ATA_AIO_RESET_ASSERTED:
                pCtl->uAsyncIOState = ATA_AIO_RESET_CLEARED;
                ataHCPIOTransferStop(&pCtl->aIfs[0]);
                ataHCPIOTransferStop(&pCtl->aIfs[1]);
                /* Do not change the DMA registers, they are not affected by the
                 * ATA controller reset logic. It should be sufficient to issue a
                 * new command, which is now possible as the state is cleared. */
                break;

            case ATA_AIO_RESET_CLEARED:
                pCtl->uAsyncIOState = ATA_AIO_NEW;
                pCtl->fReset = false;
                /* Ensure that half-completed transfers are not redone. A reset
                 * cancels the entire transfer, so continuing is wrong. */
                pCtl->fRedo = false;
                pCtl->fRedoDMALastDesc = false;
                LogRel(("PIIX3 ATA: Ctl#%d: finished processing RESET\n",
                        ATACONTROLLER_IDX(pCtl)));
                for (uint32_t i = 0; i < RT_ELEMENTS(pCtl->aIfs); i++)
                {
                    if (pCtl->aIfs[i].fATAPI)
                        ataSetStatusValue(&pCtl->aIfs[i], 0); /* NOTE: READY is _not_ set */
                    else
                        ataSetStatusValue(&pCtl->aIfs[i], ATA_STAT_READY | ATA_STAT_SEEK);
                    ataR3SetSignature(&pCtl->aIfs[i]);
                }
                break;

            case ATA_AIO_ABORT:
                /* Abort the current command no matter what. There cannot be
                 * any command activity on the other drive otherwise using
                 * one thread per controller wouldn't work at all. */
                s = &pCtl->aIfs[pReq->u.a.iIf];

                pCtl->uAsyncIOState = ATA_AIO_NEW;
                /* Do not change the DMA registers, they are not affected by the
                 * ATA controller reset logic. It should be sufficient to issue a
                 * new command, which is now possible as the state is cleared. */
                if (pReq->u.a.fResetDrive)
                {
                    ataR3ResetDevice(s);
                    ataR3ExecuteDeviceDiagnosticSS(s);
                }
                else
                {
                    /* Stop any pending DMA transfer. */
                    s->fDMA = false;
                    ataHCPIOTransferStop(s);
                    ataUnsetStatus(s, ATA_STAT_BUSY | ATA_STAT_DRQ | ATA_STAT_SEEK | ATA_STAT_ERR);
                    ataSetStatus(s, ATA_STAT_READY);
                    ataHCSetIRQ(s);
                }
                break;

            default:
                AssertMsgFailed(("Undefined async I/O state %d\n", pCtl->uAsyncIOState));
        }

        ataR3AsyncIORemoveCurrentRequest(pCtl, ReqType);
        pReq = ataR3AsyncIOGetCurrentRequest(pCtl);

        if (pCtl->uAsyncIOState == ATA_AIO_NEW && !pCtl->fChainedTransfer)
        {
# if defined(DEBUG) || defined(VBOX_WITH_STATISTICS)
            STAM_PROFILE_ADV_STOP(&pCtl->StatAsyncTime, a);
# endif

            u64TS = RTTimeNanoTS() - u64TS;
            uWait = u64TS / 1000;
            Log(("%s: Ctl#%d: LUN#%d finished I/O transaction in %d microseconds\n",
                 __FUNCTION__, ATACONTROLLER_IDX(pCtl), pCtl->aIfs[pCtl->iAIOIf].iLUN, (uint32_t)(uWait)));
            /* Mark command as finished. */
            pCtl->aIfs[pCtl->iAIOIf].u64CmdTS = 0;

            /*
             * Release logging of command execution times depends on the
             * command type. ATAPI commands often take longer (due to CD/DVD
             * spin up time etc.) so the threshold is different.
             */
            if (pCtl->aIfs[pCtl->iAIOIf].uATARegCommand != ATA_PACKET)
            {
                if (uWait > 8 * 1000 * 1000)
                {
                    /*
                     * Command took longer than 8 seconds. This is close
                     * enough or over the guest's command timeout, so place
                     * an entry in the release log to allow tracking such
                     * timing errors (which are often caused by the host).
                     */
                    LogRel(("PIIX3 ATA: execution time for ATA command %#04x was %d seconds\n",
                            pCtl->aIfs[pCtl->iAIOIf].uATARegCommand, uWait / (1000 * 1000)));
                }
            }
            else
            {
                if (uWait > 20 * 1000 * 1000)
                {
                    /*
                     * Command took longer than 20 seconds. This is close
                     * enough or over the guest's command timeout, so place
                     * an entry in the release log to allow tracking such
                     * timing errors (which are often caused by the host).
                     */
                    LogRel(("PIIX3 ATA: execution time for ATAPI command %#04x was %d seconds\n",
                            pCtl->aIfs[pCtl->iAIOIf].aATAPICmd[0], uWait / (1000 * 1000)));
                }
            }

# if defined(DEBUG) || defined(VBOX_WITH_STATISTICS)
            if (uWait < pCtl->StatAsyncMinWait || !pCtl->StatAsyncMinWait)
                pCtl->StatAsyncMinWait = uWait;
            if (uWait > pCtl->StatAsyncMaxWait)
                pCtl->StatAsyncMaxWait = uWait;

            STAM_COUNTER_ADD(&pCtl->StatAsyncTimeUS, uWait);
            STAM_COUNTER_INC(&pCtl->StatAsyncOps);
# endif /* DEBUG || VBOX_WITH_STATISTICS */
        }

        ataR3LockLeave(pCtl);
    }

    /* Signal the ultimate idleness. */
    RTThreadUserSignal(pCtl->AsyncIOThread);
    if (pCtl->fSignalIdle)
        PDMDevHlpAsyncNotificationCompleted(pCtl->pDevInsR3);

    /* Cleanup the state.  */
    /* Do not destroy request lock yet, still needed for proper shutdown. */
    pCtl->fShutdown = false;

    Log2(("%s: Ctl#%d: return %Rrc\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl), rc));
    return rc;
}

#endif /* IN_RING3 */

static uint32_t ataBMDMACmdReadB(PATACONTROLLER pCtl, uint32_t addr)
{
    uint32_t val = pCtl->BmDma.u8Cmd;
    RT_NOREF1(addr);
    Log2(("%s: addr=%#06x val=%#04x\n", __FUNCTION__, addr, val));
    return val;
}


static void ataBMDMACmdWriteB(PATACONTROLLER pCtl, uint32_t addr, uint32_t val)
{
    RT_NOREF1(addr);
    Log2(("%s: addr=%#06x val=%#04x\n", __FUNCTION__, addr, val));
    if (!(val & BM_CMD_START))
    {
        pCtl->BmDma.u8Status &= ~BM_STATUS_DMAING;
        pCtl->BmDma.u8Cmd = val & (BM_CMD_START | BM_CMD_WRITE);
    }
    else
    {
#ifndef IN_RC
        /* Check whether the guest OS wants to change DMA direction in
         * mid-flight. Not allowed, according to the PIIX3 specs. */
        Assert(!(pCtl->BmDma.u8Status & BM_STATUS_DMAING) || !((val ^ pCtl->BmDma.u8Cmd) & 0x04));
        uint8_t uOldBmDmaStatus = pCtl->BmDma.u8Status;
        pCtl->BmDma.u8Status |= BM_STATUS_DMAING;
        pCtl->BmDma.u8Cmd = val & (BM_CMD_START | BM_CMD_WRITE);

        /* Do not continue DMA transfers while the RESET line is asserted. */
        if (pCtl->fReset)
        {
            Log2(("%s: Ctl#%d: suppressed continuing DMA transfer as RESET is active\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
            return;
        }

        /* Do not start DMA transfers if there's a PIO transfer going on,
         * or if there is already a transfer started on this controller. */
        if (   !pCtl->aIfs[pCtl->iSelectedIf].fDMA
            || (uOldBmDmaStatus & BM_STATUS_DMAING))
            return;

        if (pCtl->aIfs[pCtl->iAIOIf].uATARegStatus & ATA_STAT_DRQ)
        {
            Log2(("%s: Ctl#%d: message to async I/O thread, continuing DMA transfer\n", __FUNCTION__, ATACONTROLLER_IDX(pCtl)));
            ataHCAsyncIOPutRequest(pCtl, &g_ataDMARequest);
        }
#else /* !IN_RING3 */
        AssertMsgFailed(("DMA START handling is too complicated for RC\n"));
#endif /* IN_RING3 */
    }
}

static uint32_t ataBMDMAStatusReadB(PATACONTROLLER pCtl, uint32_t addr)
{
    uint32_t val = pCtl->BmDma.u8Status;
    RT_NOREF1(addr);
    Log2(("%s: addr=%#06x val=%#04x\n", __FUNCTION__, addr, val));
    return val;
}

static void ataBMDMAStatusWriteB(PATACONTROLLER pCtl, uint32_t addr, uint32_t val)
{
    RT_NOREF1(addr);
    Log2(("%s: addr=%#06x val=%#04x\n", __FUNCTION__, addr, val));
    pCtl->BmDma.u8Status =    (val & (BM_STATUS_D0DMA | BM_STATUS_D1DMA))
                           |  (pCtl->BmDma.u8Status & BM_STATUS_DMAING)
                           |  (pCtl->BmDma.u8Status & ~val & (BM_STATUS_ERROR | BM_STATUS_INT));
}

static uint32_t ataBMDMAAddrReadL(PATACONTROLLER pCtl, uint32_t addr)
{
    uint32_t val = (uint32_t)pCtl->BmDma.GCPhysAddr;
    RT_NOREF1(addr);
    Log2(("%s: addr=%#06x val=%#010x\n", __FUNCTION__, addr, val));
    return val;
}

static void ataBMDMAAddrWriteL(PATACONTROLLER pCtl, uint32_t addr, uint32_t val)
{
    RT_NOREF1(addr);
    Log2(("%s: addr=%#06x val=%#010x\n", __FUNCTION__, addr, val));
    pCtl->BmDma.GCPhysAddr = val & ~3;
}

static void ataBMDMAAddrWriteLowWord(PATACONTROLLER pCtl, uint32_t addr, uint32_t val)
{
    RT_NOREF1(addr);
    Log2(("%s: addr=%#06x val=%#010x\n", __FUNCTION__, addr, val));
    pCtl->BmDma.GCPhysAddr = (pCtl->BmDma.GCPhysAddr & 0xFFFF0000) | RT_LOWORD(val & ~3);

}

static void ataBMDMAAddrWriteHighWord(PATACONTROLLER pCtl, uint32_t addr, uint32_t val)
{
    Log2(("%s: addr=%#06x val=%#010x\n", __FUNCTION__, addr, val));
    RT_NOREF1(addr);
    pCtl->BmDma.GCPhysAddr = (RT_LOWORD(val) << 16) | RT_LOWORD(pCtl->BmDma.GCPhysAddr);
}

#define VAL(port, size)   ( ((port) & 7) | ((size) << 3) )

/**
 * Port I/O Handler for bus master DMA IN operations.
 * @see FNIOMIOPORTIN for details.
 */
PDMBOTHCBDECL(int) ataBMDMAIOPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    uint32_t       i = (uint32_t)(uintptr_t)pvUser;
    PCIATAState   *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    PATACONTROLLER pCtl = &pThis->aCts[i];

    int rc = PDMCritSectEnter(&pCtl->lock, VINF_IOM_R3_IOPORT_READ);
    if (rc != VINF_SUCCESS)
        return rc;
    switch (VAL(Port, cb))
    {
        case VAL(0, 1): *pu32 = ataBMDMACmdReadB(pCtl, Port); break;
        case VAL(0, 2): *pu32 = ataBMDMACmdReadB(pCtl, Port); break;
        case VAL(2, 1): *pu32 = ataBMDMAStatusReadB(pCtl, Port); break;
        case VAL(2, 2): *pu32 = ataBMDMAStatusReadB(pCtl, Port); break;
        case VAL(4, 4): *pu32 = ataBMDMAAddrReadL(pCtl, Port); break;
        case VAL(0, 4):
            /* The SCO OpenServer tries to read 4 bytes starting from offset 0. */
            *pu32 = ataBMDMACmdReadB(pCtl, Port) | (ataBMDMAStatusReadB(pCtl, Port) << 16);
            break;
        default:
            AssertMsgFailed(("%s: Unsupported read from port %x size=%d\n", __FUNCTION__, Port, cb));
            rc = VERR_IOM_IOPORT_UNUSED;
            break;
    }
    PDMCritSectLeave(&pCtl->lock);
    return rc;
}

/**
 * Port I/O Handler for bus master DMA OUT operations.
 * @see FNIOMIOPORTOUT for details.
 */
PDMBOTHCBDECL(int) ataBMDMAIOPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    uint32_t       i = (uint32_t)(uintptr_t)pvUser;
    PCIATAState   *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    PATACONTROLLER pCtl = &pThis->aCts[i];

    int rc = PDMCritSectEnter(&pCtl->lock, VINF_IOM_R3_IOPORT_WRITE);
    if (rc != VINF_SUCCESS)
        return rc;
    switch (VAL(Port, cb))
    {
        case VAL(0, 1):
#ifdef IN_RC
            if (u32 & BM_CMD_START)
            {
                rc = VINF_IOM_R3_IOPORT_WRITE;
                break;
            }
#endif
            ataBMDMACmdWriteB(pCtl, Port, u32);
            break;
        case VAL(2, 1): ataBMDMAStatusWriteB(pCtl, Port, u32); break;
        case VAL(4, 4): ataBMDMAAddrWriteL(pCtl, Port, u32); break;
        case VAL(4, 2): ataBMDMAAddrWriteLowWord(pCtl, Port, u32); break;
        case VAL(6, 2): ataBMDMAAddrWriteHighWord(pCtl, Port, u32); break;
        default:        AssertMsgFailed(("%s: Unsupported write to port %x size=%d val=%x\n", __FUNCTION__, Port, cb, u32)); break;
    }
    PDMCritSectLeave(&pCtl->lock);
    return rc;
}

#undef VAL

#ifdef IN_RING3

/**
 * @callback_method_impl{FNPCIIOREGIONMAP}
 */
static DECLCALLBACK(int) ataR3BMDMAIORangeMap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                              RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType)
{
    RT_NOREF(iRegion, cb, enmType, pPciDev);
    PCIATAState *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    int         rc = VINF_SUCCESS;
    Assert(enmType == PCI_ADDRESS_SPACE_IO);
    Assert(iRegion == 4);
    AssertMsg(RT_ALIGN(GCPhysAddress, 8) == GCPhysAddress, ("Expected 8 byte alignment. GCPhysAddress=%#x\n", GCPhysAddress));

    /* Register the port range. */
    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        int rc2 = PDMDevHlpIOPortRegister(pDevIns, (RTIOPORT)GCPhysAddress + i * 8, 8,
                                          (RTHCPTR)(uintptr_t)i, ataBMDMAIOPortWrite, ataBMDMAIOPortRead,
                                          NULL, NULL, "ATA Bus Master DMA");
        AssertRC(rc2);
        if (rc2 < rc)
            rc = rc2;

        if (pThis->fRCEnabled)
        {
            rc2 = PDMDevHlpIOPortRegisterRC(pDevIns, (RTIOPORT)GCPhysAddress + i * 8, 8,
                                            (RTGCPTR)i, "ataBMDMAIOPortWrite", "ataBMDMAIOPortRead",
                                            NULL, NULL, "ATA Bus Master DMA");
            AssertRC(rc2);
            if (rc2 < rc)
                rc = rc2;
        }
        if (pThis->fR0Enabled)
        {
            rc2 = PDMDevHlpIOPortRegisterR0(pDevIns, (RTIOPORT)GCPhysAddress + i * 8, 8,
                                            (RTR0PTR)i, "ataBMDMAIOPortWrite", "ataBMDMAIOPortRead",
                                            NULL, NULL, "ATA Bus Master DMA");
            AssertRC(rc2);
            if (rc2 < rc)
                rc = rc2;
        }
    }
    return rc;
}


/* -=-=-=-=-=- PCIATAState::IBase  -=-=-=-=-=- */

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) ataR3Status_QueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PCIATAState *pThis = RT_FROM_MEMBER(pInterface, PCIATAState, IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMILEDPORTS, &pThis->ILeds);
    return NULL;
}


/* -=-=-=-=-=- PCIATAState::ILeds  -=-=-=-=-=- */

/**
 * Gets the pointer to the status LED of a unit.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @param   iLUN            The unit which status LED we desire.
 * @param   ppLed           Where to store the LED pointer.
 */
static DECLCALLBACK(int) ataR3Status_QueryStatusLed(PPDMILEDPORTS pInterface, unsigned iLUN, PPDMLED *ppLed)
{
    PCIATAState *pThis = RT_FROM_MEMBER(pInterface, PCIATAState, ILeds);
    if (iLUN < 4)
    {
        switch (iLUN)
        {
            case 0: *ppLed = &pThis->aCts[0].aIfs[0].Led; break;
            case 1: *ppLed = &pThis->aCts[0].aIfs[1].Led; break;
            case 2: *ppLed = &pThis->aCts[1].aIfs[0].Led; break;
            case 3: *ppLed = &pThis->aCts[1].aIfs[1].Led; break;
        }
        Assert((*ppLed)->u32Magic == PDMLED_MAGIC);
        return VINF_SUCCESS;
    }
    return VERR_PDM_LUN_NOT_FOUND;
}


/* -=-=-=-=-=- ATADevState::IBase   -=-=-=-=-=- */

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) ataR3QueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    ATADevState *pIf = RT_FROM_MEMBER(pInterface, ATADevState, IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pIf->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMEDIAPORT, &pIf->IPort);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMOUNTNOTIFY, &pIf->IMountNotify);
    return NULL;
}


/* -=-=-=-=-=- ATADevState::IPort  -=-=-=-=-=- */

/**
 * @interface_method_impl{PDMIMEDIAPORT,pfnQueryDeviceLocation}
 */
static DECLCALLBACK(int) ataR3QueryDeviceLocation(PPDMIMEDIAPORT pInterface, const char **ppcszController,
                                                  uint32_t *piInstance, uint32_t *piLUN)
{
    ATADevState *pIf = RT_FROM_MEMBER(pInterface, ATADevState, IPort);
    PPDMDEVINS pDevIns = pIf->CTX_SUFF(pDevIns);

    AssertPtrReturn(ppcszController, VERR_INVALID_POINTER);
    AssertPtrReturn(piInstance, VERR_INVALID_POINTER);
    AssertPtrReturn(piLUN, VERR_INVALID_POINTER);

    *ppcszController = pDevIns->pReg->szName;
    *piInstance = pDevIns->iInstance;
    *piLUN = pIf->iLUN;

    return VINF_SUCCESS;
}

#endif /* IN_RING3 */

/* -=-=-=-=-=- Wrappers  -=-=-=-=-=- */


/**
 * Port I/O Handler for primary port range OUT operations.
 * @see FNIOMIOPORTOUT for details.
 */
PDMBOTHCBDECL(int) ataIOPortWrite1Other(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    uint32_t       i = (uint32_t)(uintptr_t)pvUser;
    PCIATAState   *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    PATACONTROLLER pCtl = &pThis->aCts[i];

    Assert(i < 2);
    Assert(Port != pCtl->IOPortBase1);

    int rc = PDMCritSectEnter(&pCtl->lock, VINF_IOM_R3_IOPORT_WRITE);
    if (rc == VINF_SUCCESS)
    {
        /* Writes to the other command block ports should be 8-bit only. If they
         * are not, the high bits are simply discarded. Undocumented, but observed
         * on a real PIIX4 system.
         */
        if (cb > 1)
            Log(("ataIOPortWrite1: suspect write to port %x val=%x size=%d\n", Port, u32, cb));

        rc = ataIOPortWriteU8(pCtl, Port, u32);

        PDMCritSectLeave(&pCtl->lock);
    }
    return rc;
}


/**
 * Port I/O Handler for primary port range IN operations.
 * @see FNIOMIOPORTIN for details.
 */
PDMBOTHCBDECL(int) ataIOPortRead1Other(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    uint32_t       i = (uint32_t)(uintptr_t)pvUser;
    PCIATAState   *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    PATACONTROLLER pCtl = &pThis->aCts[i];

    Assert(i < 2);
    Assert(Port != pCtl->IOPortBase1);

    int rc = PDMCritSectEnter(&pCtl->lock, VINF_IOM_R3_IOPORT_READ);
    if (rc == VINF_SUCCESS)
    {
        /* Reads from the other command block registers should be 8-bit only.
         * If they are not, the low byte is propagated to the high bits.
         * Undocumented, but observed on a real PIIX4 system.
         */
        rc = ataIOPortReadU8(pCtl, Port, pu32);
        if (cb > 1)
        {
            uint32_t    pad;

            /* Replicate the 8-bit result into the upper three bytes. */
            pad = *pu32 & 0xff;
            pad = pad | (pad << 8);
            pad = pad | (pad << 16);
            *pu32 = pad;
            Log(("ataIOPortRead1: suspect read from port %x size=%d\n", Port, cb));
        }
        PDMCritSectLeave(&pCtl->lock);
    }
    return rc;
}


/**
 * Port I/O Handler for secondary port range OUT operations.
 * @see FNIOMIOPORTOUT for details.
 */
PDMBOTHCBDECL(int) ataIOPortWrite2(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    uint32_t       i = (uint32_t)(uintptr_t)pvUser;
    PCIATAState   *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    PATACONTROLLER pCtl = &pThis->aCts[i];
    int rc;

    Assert(i < 2);

    if (cb == 1)
    {
        rc = PDMCritSectEnter(&pCtl->lock, VINF_IOM_R3_IOPORT_WRITE);
        if (rc == VINF_SUCCESS)
        {
            rc = ataControlWrite(pCtl, Port, u32);
            PDMCritSectLeave(&pCtl->lock);
        }
    }
    else
        rc = VINF_SUCCESS;
    return rc;
}


/**
 * Port I/O Handler for secondary port range IN operations.
 * @see FNIOMIOPORTIN for details.
 */
PDMBOTHCBDECL(int) ataIOPortRead2(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    uint32_t       i = (uint32_t)(uintptr_t)pvUser;
    PCIATAState   *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    PATACONTROLLER pCtl = &pThis->aCts[i];
    int            rc;

    Assert(i < 2);

    if (cb == 1)
    {
        rc = PDMCritSectEnter(&pCtl->lock, VINF_IOM_R3_IOPORT_READ);
        if (rc == VINF_SUCCESS)
        {
            *pu32 = ataStatusRead(pCtl, Port);
            PDMCritSectLeave(&pCtl->lock);
        }
    }
    else
        rc = VERR_IOM_IOPORT_UNUSED;
    return rc;
}

#ifdef IN_RING3


DECLINLINE(void) ataR3RelocBuffer(PPDMDEVINS pDevIns, ATADevState *s)
{
    if (s->pbIOBufferR3)
        s->pbIOBufferRC = MMHyperR3ToRC(PDMDevHlpGetVM(pDevIns), s->pbIOBufferR3);
}


/**
 * Detach notification.
 *
 * The DVD drive has been unplugged.
 *
 * @param   pDevIns     The device instance.
 * @param   iLUN        The logical unit which is being detached.
 * @param   fFlags      Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 */
static DECLCALLBACK(void) ataR3Detach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
    PCIATAState    *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    AssertMsg(fFlags & PDM_TACH_FLAGS_NOT_HOT_PLUG,
              ("PIIX3IDE: Device does not support hotplugging\n")); RT_NOREF(fFlags);

    /*
     * Locate the controller and stuff.
     */
    unsigned iController = iLUN / RT_ELEMENTS(pThis->aCts[0].aIfs);
    AssertReleaseMsg(iController < RT_ELEMENTS(pThis->aCts), ("iController=%d iLUN=%d\n", iController, iLUN));
    PATACONTROLLER  pCtl = &pThis->aCts[iController];

    unsigned iInterface  = iLUN % RT_ELEMENTS(pThis->aCts[0].aIfs);
    ATADevState *pIf = &pCtl->aIfs[iInterface];

    /*
     * Zero some important members.
     */
    pIf->pDrvBase = NULL;
    pIf->pDrvMedia = NULL;
    pIf->pDrvMount = NULL;

    /*
     * In case there was a medium inserted.
     */
    ataR3MediumRemoved(pIf);
}


/**
 * Configure a LUN.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pIf         The ATA unit state.
 */
static int ataR3ConfigLun(PPDMDEVINS pDevIns, ATADevState *pIf)
{
    int             rc = VINF_SUCCESS;
    PDMMEDIATYPE    enmType;

    /*
     * Query Block, Bios and Mount interfaces.
     */
    pIf->pDrvMedia = PDMIBASE_QUERY_INTERFACE(pIf->pDrvBase, PDMIMEDIA);
    if (!pIf->pDrvMedia)
    {
        AssertMsgFailed(("Configuration error: LUN#%d hasn't a block interface!\n", pIf->iLUN));
        return VERR_PDM_MISSING_INTERFACE;
    }

    pIf->pDrvMount = PDMIBASE_QUERY_INTERFACE(pIf->pDrvBase, PDMIMOUNT);

    /*
     * Validate type.
     */
    enmType = pIf->pDrvMedia->pfnGetType(pIf->pDrvMedia);
    if (    enmType != PDMMEDIATYPE_CDROM
        &&  enmType != PDMMEDIATYPE_DVD
        &&  enmType != PDMMEDIATYPE_HARD_DISK)
    {
        AssertMsgFailed(("Configuration error: LUN#%d isn't a disk or cd/dvd-rom. enmType=%d\n", pIf->iLUN, enmType));
        return VERR_PDM_UNSUPPORTED_BLOCK_TYPE;
    }
    if (    (   enmType == PDMMEDIATYPE_DVD
             || enmType == PDMMEDIATYPE_CDROM)
        &&  !pIf->pDrvMount)
    {
        AssertMsgFailed(("Internal error: cdrom without a mountable interface, WTF???!\n"));
        return VERR_INTERNAL_ERROR;
    }
    pIf->fATAPI = enmType == PDMMEDIATYPE_DVD || enmType == PDMMEDIATYPE_CDROM;
    pIf->fATAPIPassthrough = pIf->fATAPI ? (pIf->pDrvMedia->pfnSendCmd != NULL) : false;

    /*
     * Allocate I/O buffer.
     */
    if (pIf->fATAPI)
        pIf->cbSector = 2048; /* Not required for ATAPI, one medium can have multiple sector sizes. */
    else
        pIf->cbSector = pIf->pDrvMedia->pfnGetSectorSize(pIf->pDrvMedia);

    PVM pVM = PDMDevHlpGetVM(pDevIns);
    if (pIf->cbIOBuffer)
    {
        /* Buffer is (probably) already allocated. Validate the fields,
         * because memory corruption can also overwrite pIf->cbIOBuffer. */
        if (pIf->fATAPI)
            AssertRelease(pIf->cbIOBuffer == _128K);
        else
            AssertRelease(pIf->cbIOBuffer == ATA_MAX_MULT_SECTORS * pIf->cbSector);
        Assert(pIf->pbIOBufferR3);
        Assert(pIf->pbIOBufferR0 == MMHyperR3ToR0(pVM, pIf->pbIOBufferR3));
        Assert(pIf->pbIOBufferRC == MMHyperR3ToRC(pVM, pIf->pbIOBufferR3));
    }
    else
    {
        if (pIf->fATAPI)
            pIf->cbIOBuffer = _128K;
        else
            pIf->cbIOBuffer = ATA_MAX_MULT_SECTORS * pIf->cbSector;
        Assert(!pIf->pbIOBufferR3);
        rc = MMR3HyperAllocOnceNoRel(pVM, pIf->cbIOBuffer, 0, MM_TAG_PDM_DEVICE_USER, (void **)&pIf->pbIOBufferR3);
        if (RT_FAILURE(rc))
            return VERR_NO_MEMORY;
        pIf->pbIOBufferR0 = MMHyperR3ToR0(pVM, pIf->pbIOBufferR3);
        pIf->pbIOBufferRC = MMHyperR3ToRC(pVM, pIf->pbIOBufferR3);
    }

    /*
     * Init geometry (only for non-CD/DVD media).
     */
    uint32_t cRegions = pIf->pDrvMedia->pfnGetRegionCount(pIf->pDrvMedia);
    pIf->cTotalSectors = 0;
    for (uint32_t i = 0; i < cRegions; i++)
    {
        uint64_t cBlocks = 0;
        rc = pIf->pDrvMedia->pfnQueryRegionProperties(pIf->pDrvMedia, i, NULL, &cBlocks,
                                                      NULL, NULL);
        AssertRC(rc);
        pIf->cTotalSectors += cBlocks;
    }

    if (pIf->fATAPI)
    {
        pIf->PCHSGeometry.cCylinders = 0; /* dummy */
        pIf->PCHSGeometry.cHeads     = 0; /* dummy */
        pIf->PCHSGeometry.cSectors   = 0; /* dummy */
        LogRel(("PIIX3 ATA: LUN#%d: CD/DVD, total number of sectors %Ld, passthrough %s\n",
                pIf->iLUN, pIf->cTotalSectors, (pIf->fATAPIPassthrough ? "enabled" : "disabled")));
    }
    else
    {
        rc = pIf->pDrvMedia->pfnBiosGetPCHSGeometry(pIf->pDrvMedia, &pIf->PCHSGeometry);
        if (rc == VERR_PDM_MEDIA_NOT_MOUNTED)
        {
            pIf->PCHSGeometry.cCylinders = 0;
            pIf->PCHSGeometry.cHeads     = 16; /*??*/
            pIf->PCHSGeometry.cSectors   = 63; /*??*/
        }
        else if (rc == VERR_PDM_GEOMETRY_NOT_SET)
        {
            pIf->PCHSGeometry.cCylinders = 0; /* autodetect marker */
            rc = VINF_SUCCESS;
        }
        AssertRC(rc);

        if (   pIf->PCHSGeometry.cCylinders == 0
            || pIf->PCHSGeometry.cHeads == 0
            || pIf->PCHSGeometry.cSectors == 0
           )
        {
            uint64_t cCylinders = pIf->cTotalSectors / (16 * 63);
            pIf->PCHSGeometry.cCylinders = RT_MAX(RT_MIN(cCylinders, 16383), 1);
            pIf->PCHSGeometry.cHeads = 16;
            pIf->PCHSGeometry.cSectors = 63;
            /* Set the disk geometry information. Ignore errors. */
            pIf->pDrvMedia->pfnBiosSetPCHSGeometry(pIf->pDrvMedia, &pIf->PCHSGeometry);
            rc = VINF_SUCCESS;
        }
        LogRel(("PIIX3 ATA: LUN#%d: disk, PCHS=%u/%u/%u, total number of sectors %Ld\n",
                pIf->iLUN, pIf->PCHSGeometry.cCylinders, pIf->PCHSGeometry.cHeads, pIf->PCHSGeometry.cSectors,
                pIf->cTotalSectors));

        if (pIf->pDrvMedia->pfnDiscard)
            LogRel(("PIIX3 ATA: LUN#%d: TRIM enabled\n", pIf->iLUN));
    }
    return rc;
}


/**
 * Attach command.
 *
 * This is called when we change block driver for the DVD drive.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   iLUN        The logical unit which is being detached.
 * @param   fFlags      Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 */
static DECLCALLBACK(int)  ataR3Attach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
    PCIATAState    *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    PATACONTROLLER  pCtl;
    ATADevState    *pIf;
    int             rc;
    unsigned        iController;
    unsigned        iInterface;

    AssertMsgReturn(fFlags & PDM_TACH_FLAGS_NOT_HOT_PLUG,
                    ("PIIX3IDE: Device does not support hotplugging\n"),
                    VERR_INVALID_PARAMETER);

    /*
     * Locate the controller and stuff.
     */
    iController = iLUN / RT_ELEMENTS(pThis->aCts[0].aIfs);
    AssertReleaseMsg(iController < RT_ELEMENTS(pThis->aCts), ("iController=%d iLUN=%d\n", iController, iLUN));
    pCtl = &pThis->aCts[iController];

    iInterface  = iLUN % RT_ELEMENTS(pThis->aCts[0].aIfs);
    pIf = &pCtl->aIfs[iInterface];

    /* the usual paranoia */
    AssertRelease(!pIf->pDrvBase);
    AssertRelease(!pIf->pDrvMedia);
    Assert(ATADEVSTATE_2_CONTROLLER(pIf) == pCtl);
    Assert(pIf->iLUN == iLUN);

    /*
     * Try attach the block device and get the interfaces,
     * required as well as optional.
     */
    rc = PDMDevHlpDriverAttach(pDevIns, pIf->iLUN, &pIf->IBase, &pIf->pDrvBase, NULL);
    if (RT_SUCCESS(rc))
    {
        rc = ataR3ConfigLun(pDevIns, pIf);
        /*
         * In case there is a medium inserted.
         */
        ataR3MediumInserted(pIf);
        ataR3MediumTypeSet(pIf, ATA_MEDIA_TYPE_UNKNOWN);
    }
    else
        AssertMsgFailed(("Failed to attach LUN#%d. rc=%Rrc\n", pIf->iLUN, rc));

    if (RT_FAILURE(rc))
    {
        pIf->pDrvBase = NULL;
        pIf->pDrvMedia = NULL;
    }
    return rc;
}


/**
 * Resume notification.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance data.
 */
static DECLCALLBACK(void) ataR3Resume(PPDMDEVINS pDevIns)
{
    PCIATAState    *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    int             rc;

    Log(("%s:\n", __FUNCTION__));
    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        if (pThis->aCts[i].fRedo && pThis->aCts[i].fRedoIdle)
        {
            rc = RTSemEventSignal(pThis->aCts[i].SuspendIOSem);
            AssertRC(rc);
        }
    }
    return;
}


/**
 * Checks if all (both) the async I/O threads have quiesced.
 *
 * @returns true on success.
 * @returns false when one or more threads is still processing.
 * @param   pDevIns               Pointer to the PDM device instance.
 */
static bool ataR3AllAsyncIOIsIdle(PPDMDEVINS pDevIns)
{
    PCIATAState *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);

    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
        if (pThis->aCts[i].AsyncIOThread != NIL_RTTHREAD)
        {
            bool fRc = ataR3AsyncIOIsIdle(&pThis->aCts[i], false /*fStrict*/);
            if (!fRc)
            {
                /* Make it signal PDM & itself when its done */
                PDMCritSectEnter(&pThis->aCts[i].AsyncIORequestLock, VERR_IGNORED);
                ASMAtomicWriteBool(&pThis->aCts[i].fSignalIdle, true);
                PDMCritSectLeave(&pThis->aCts[i].AsyncIORequestLock);

                fRc = ataR3AsyncIOIsIdle(&pThis->aCts[i], false /*fStrict*/);
                if (!fRc)
                {
#if 0  /** @todo Need to do some time tracking here... */
                    LogRel(("PIIX3 ATA: Ctl#%u is still executing, DevSel=%d AIOIf=%d CmdIf0=%#04x CmdIf1=%#04x\n",
                            i, pThis->aCts[i].iSelectedIf, pThis->aCts[i].iAIOIf,
                            pThis->aCts[i].aIfs[0].uATARegCommand, pThis->aCts[i].aIfs[1].uATARegCommand));
#endif
                    return false;
                }
            }
            ASMAtomicWriteBool(&pThis->aCts[i].fSignalIdle, false);
        }
    return true;
}

/**
 * Prepare state save and load operation.
 *
 * @returns VBox status code.
 * @param   pDevIns         Device instance of the device which registered the data unit.
 * @param   pSSM            SSM operation handle.
 */
static DECLCALLBACK(int) ataR3SaveLoadPrep(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    RT_NOREF1(pSSM);
    PCIATAState *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);

    /* sanity - the suspend notification will wait on the async stuff. */
    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
        AssertLogRelMsgReturn(ataR3AsyncIOIsIdle(&pThis->aCts[i], false /*fStrict*/),
                              ("i=%u\n", i),
                              VERR_SSM_IDE_ASYNC_TIMEOUT);
    return VINF_SUCCESS;
}

/**
 * @copydoc FNSSMDEVLIVEEXEC
 */
static DECLCALLBACK(int) ataR3LiveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uPass)
{
    RT_NOREF1(uPass);
    PCIATAState *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);

    SSMR3PutU8(pSSM, pThis->u8Type);
    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        SSMR3PutBool(pSSM, true);       /* For controller enabled / disabled. */
        for (uint32_t j = 0; j < RT_ELEMENTS(pThis->aCts[i].aIfs); j++)
        {
            SSMR3PutBool(pSSM, pThis->aCts[i].aIfs[j].pDrvBase != NULL);
            SSMR3PutStrZ(pSSM, pThis->aCts[i].aIfs[j].szSerialNumber);
            SSMR3PutStrZ(pSSM, pThis->aCts[i].aIfs[j].szFirmwareRevision);
            SSMR3PutStrZ(pSSM, pThis->aCts[i].aIfs[j].szModelNumber);
        }
    }

    return VINF_SSM_DONT_CALL_AGAIN;
}

/**
 * @copydoc FNSSMDEVSAVEEXEC
 */
static DECLCALLBACK(int) ataR3SaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PCIATAState    *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);

    ataR3LiveExec(pDevIns, pSSM, SSM_PASS_FINAL);

    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        SSMR3PutU8(pSSM, pThis->aCts[i].iSelectedIf);
        SSMR3PutU8(pSSM, pThis->aCts[i].iAIOIf);
        SSMR3PutU8(pSSM, pThis->aCts[i].uAsyncIOState);
        SSMR3PutBool(pSSM, pThis->aCts[i].fChainedTransfer);
        SSMR3PutBool(pSSM, pThis->aCts[i].fReset);
        SSMR3PutBool(pSSM, pThis->aCts[i].fRedo);
        SSMR3PutBool(pSSM, pThis->aCts[i].fRedoIdle);
        SSMR3PutBool(pSSM, pThis->aCts[i].fRedoDMALastDesc);
        SSMR3PutMem(pSSM, &pThis->aCts[i].BmDma, sizeof(pThis->aCts[i].BmDma));
        SSMR3PutGCPhys32(pSSM, pThis->aCts[i].GCPhysFirstDMADesc);
        SSMR3PutGCPhys32(pSSM, pThis->aCts[i].GCPhysLastDMADesc);
        SSMR3PutGCPhys32(pSSM, pThis->aCts[i].GCPhysRedoDMABuffer);
        SSMR3PutU32(pSSM, pThis->aCts[i].cbRedoDMABuffer);

        for (uint32_t j = 0; j < RT_ELEMENTS(pThis->aCts[i].aIfs); j++)
        {
            SSMR3PutBool(pSSM, pThis->aCts[i].aIfs[j].fLBA48);
            SSMR3PutBool(pSSM, pThis->aCts[i].aIfs[j].fATAPI);
            SSMR3PutBool(pSSM, pThis->aCts[i].aIfs[j].fIrqPending);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].cMultSectors);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].PCHSGeometry.cCylinders);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].PCHSGeometry.cHeads);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].PCHSGeometry.cSectors);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].cSectorsPerIRQ);
            SSMR3PutU64(pSSM, pThis->aCts[i].aIfs[j].cTotalSectors);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegFeature);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegFeatureHOB);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegError);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegNSector);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegNSectorHOB);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegSector);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegSectorHOB);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegLCyl);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegLCylHOB);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegHCyl);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegHCylHOB);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegSelect);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegStatus);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegCommand);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATARegDevCtl);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uATATransferMode);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].uTxDir);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].iBeginTransfer);
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].iSourceSink);
            SSMR3PutBool(pSSM, pThis->aCts[i].aIfs[j].fDMA);
            SSMR3PutBool(pSSM, pThis->aCts[i].aIfs[j].fATAPITransfer);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].cbTotalTransfer);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].cbElementaryTransfer);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].iIOBufferCur);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].iIOBufferEnd);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].iIOBufferPIODataStart);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].iIOBufferPIODataEnd);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].iATAPILBA);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].cbATAPISector);
            SSMR3PutMem(pSSM, &pThis->aCts[i].aIfs[j].aATAPICmd, sizeof(pThis->aCts[i].aIfs[j].aATAPICmd));
            SSMR3PutMem(pSSM, &pThis->aCts[i].aIfs[j].abATAPISense, sizeof(pThis->aCts[i].aIfs[j].abATAPISense));
            SSMR3PutU8(pSSM, pThis->aCts[i].aIfs[j].cNotifiedMediaChange);
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].MediaEventStatus);
            SSMR3PutMem(pSSM, &pThis->aCts[i].aIfs[j].Led, sizeof(pThis->aCts[i].aIfs[j].Led));
            SSMR3PutU32(pSSM, pThis->aCts[i].aIfs[j].cbIOBuffer);
            if (pThis->aCts[i].aIfs[j].cbIOBuffer)
                SSMR3PutMem(pSSM, pThis->aCts[i].aIfs[j].CTX_SUFF(pbIOBuffer), pThis->aCts[i].aIfs[j].cbIOBuffer);
            else
                Assert(pThis->aCts[i].aIfs[j].CTX_SUFF(pbIOBuffer) == NULL);
        }
    }

    return SSMR3PutU32(pSSM, UINT32_MAX); /* sanity/terminator */
}

/**
 * Converts the LUN number into a message string.
 */
static const char *ataR3StringifyLun(unsigned iLun)
{
    switch (iLun)
    {
        case 0:  return "primary master";
        case 1:  return "primary slave";
        case 2:  return "secondary master";
        case 3:  return "secondary slave";
        default: AssertFailedReturn("unknown lun");
    }
}

/**
 * FNSSMDEVLOADEXEC
 */
static DECLCALLBACK(int) ataR3LoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PCIATAState    *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    int             rc;
    uint32_t        u32;

    if (   uVersion != ATA_SAVED_STATE_VERSION
        && uVersion != ATA_SAVED_STATE_VERSION_VBOX_30
        && uVersion != ATA_SAVED_STATE_VERSION_WITHOUT_FULL_SENSE
        && uVersion != ATA_SAVED_STATE_VERSION_WITHOUT_EVENT_STATUS
        && uVersion != ATA_SAVED_STATE_VERSION_WITH_BOOL_TYPE)
    {
        AssertMsgFailed(("uVersion=%d\n", uVersion));
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;
    }

    /*
     * Verify the configuration.
     */
    if (uVersion > ATA_SAVED_STATE_VERSION_VBOX_30)
    {
        uint8_t u8Type;
        rc = SSMR3GetU8(pSSM, &u8Type);
        AssertRCReturn(rc, rc);
        if (u8Type != pThis->u8Type)
            return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Config mismatch: u8Type - saved=%u config=%u"), u8Type, pThis->u8Type);

        for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
        {
            bool fEnabled;
            rc = SSMR3GetBool(pSSM, &fEnabled);
            AssertRCReturn(rc, rc);
            if (!fEnabled)
                return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Ctr#%u onfig mismatch: fEnabled != true"), i);

            for (uint32_t j = 0; j < RT_ELEMENTS(pThis->aCts[i].aIfs); j++)
            {
                ATADevState const *pIf = &pThis->aCts[i].aIfs[j];

                bool fInUse;
                rc = SSMR3GetBool(pSSM, &fInUse);
                AssertRCReturn(rc, rc);
                if (fInUse != (pIf->pDrvBase != NULL))
                    return SSMR3SetCfgError(pSSM, RT_SRC_POS,
                                            N_("The %s VM is missing a %s device. Please make sure the source and target VMs have compatible storage configurations"),
                                            fInUse ? "target" : "source", ataR3StringifyLun(pIf->iLUN) );

                char szSerialNumber[ATA_SERIAL_NUMBER_LENGTH+1];
                rc = SSMR3GetStrZ(pSSM, szSerialNumber,     sizeof(szSerialNumber));
                AssertRCReturn(rc, rc);
                if (strcmp(szSerialNumber, pIf->szSerialNumber))
                    LogRel(("PIIX3 ATA: LUN#%u config mismatch: Serial number - saved='%s' config='%s'\n",
                            pIf->iLUN, szSerialNumber, pIf->szSerialNumber));

                char szFirmwareRevision[ATA_FIRMWARE_REVISION_LENGTH+1];
                rc = SSMR3GetStrZ(pSSM, szFirmwareRevision, sizeof(szFirmwareRevision));
                AssertRCReturn(rc, rc);
                if (strcmp(szFirmwareRevision, pIf->szFirmwareRevision))
                    LogRel(("PIIX3 ATA: LUN#%u config mismatch: Firmware revision - saved='%s' config='%s'\n",
                            pIf->iLUN, szFirmwareRevision, pIf->szFirmwareRevision));

                char szModelNumber[ATA_MODEL_NUMBER_LENGTH+1];
                rc = SSMR3GetStrZ(pSSM, szModelNumber,      sizeof(szModelNumber));
                AssertRCReturn(rc, rc);
                if (strcmp(szModelNumber, pIf->szModelNumber))
                    LogRel(("PIIX3 ATA: LUN#%u config mismatch: Model number - saved='%s' config='%s'\n",
                            pIf->iLUN, szModelNumber, pIf->szModelNumber));
            }
        }
    }
    if (uPass != SSM_PASS_FINAL)
        return VINF_SUCCESS;

    /*
     * Restore valid parts of the PCIATAState structure
     */
    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        /* integrity check */
        if (!ataR3AsyncIOIsIdle(&pThis->aCts[i], false))
        {
            AssertMsgFailed(("Async I/O for controller %d is active\n", i));
            return VERR_INTERNAL_ERROR_4;
        }

        SSMR3GetU8(pSSM, &pThis->aCts[i].iSelectedIf);
        SSMR3GetU8(pSSM, &pThis->aCts[i].iAIOIf);
        SSMR3GetU8(pSSM, &pThis->aCts[i].uAsyncIOState);
        SSMR3GetBool(pSSM, &pThis->aCts[i].fChainedTransfer);
        SSMR3GetBool(pSSM, (bool *)&pThis->aCts[i].fReset);
        SSMR3GetBool(pSSM, (bool *)&pThis->aCts[i].fRedo);
        SSMR3GetBool(pSSM, (bool *)&pThis->aCts[i].fRedoIdle);
        SSMR3GetBool(pSSM, (bool *)&pThis->aCts[i].fRedoDMALastDesc);
        SSMR3GetMem(pSSM, &pThis->aCts[i].BmDma, sizeof(pThis->aCts[i].BmDma));
        SSMR3GetGCPhys32(pSSM, &pThis->aCts[i].GCPhysFirstDMADesc);
        SSMR3GetGCPhys32(pSSM, &pThis->aCts[i].GCPhysLastDMADesc);
        SSMR3GetGCPhys32(pSSM, &pThis->aCts[i].GCPhysRedoDMABuffer);
        SSMR3GetU32(pSSM, &pThis->aCts[i].cbRedoDMABuffer);

        for (uint32_t j = 0; j < RT_ELEMENTS(pThis->aCts[i].aIfs); j++)
        {
            SSMR3GetBool(pSSM, &pThis->aCts[i].aIfs[j].fLBA48);
            SSMR3GetBool(pSSM, &pThis->aCts[i].aIfs[j].fATAPI);
            SSMR3GetBool(pSSM, &pThis->aCts[i].aIfs[j].fIrqPending);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].cMultSectors);
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].PCHSGeometry.cCylinders);
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].PCHSGeometry.cHeads);
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].PCHSGeometry.cSectors);
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].cSectorsPerIRQ);
            SSMR3GetU64(pSSM, &pThis->aCts[i].aIfs[j].cTotalSectors);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegFeature);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegFeatureHOB);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegError);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegNSector);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegNSectorHOB);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegSector);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegSectorHOB);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegLCyl);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegLCylHOB);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegHCyl);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegHCylHOB);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegSelect);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegStatus);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegCommand);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATARegDevCtl);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uATATransferMode);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].uTxDir);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].iBeginTransfer);
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].iSourceSink);
            SSMR3GetBool(pSSM, &pThis->aCts[i].aIfs[j].fDMA);
            SSMR3GetBool(pSSM, &pThis->aCts[i].aIfs[j].fATAPITransfer);
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].cbTotalTransfer);
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].cbElementaryTransfer);
            /* NB: cbPIOTransferLimit could be saved/restored but it's sufficient
             * to re-calculate it here, with a tiny risk that it could be
             * unnecessarily low for the current transfer only. Could be changed
             * when changing the saved state in the future.
             */
            pThis->aCts[i].aIfs[j].cbPIOTransferLimit = (pThis->aCts[i].aIfs[j].uATARegHCyl << 8) | pThis->aCts[i].aIfs[j].uATARegLCyl;
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].iIOBufferCur);
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].iIOBufferEnd);
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].iIOBufferPIODataStart);
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].iIOBufferPIODataEnd);
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].iATAPILBA);
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].cbATAPISector);
            SSMR3GetMem(pSSM, &pThis->aCts[i].aIfs[j].aATAPICmd, sizeof(pThis->aCts[i].aIfs[j].aATAPICmd));
            if (uVersion > ATA_SAVED_STATE_VERSION_WITHOUT_FULL_SENSE)
            {
                SSMR3GetMem(pSSM, pThis->aCts[i].aIfs[j].abATAPISense, sizeof(pThis->aCts[i].aIfs[j].abATAPISense));
            }
            else
            {
                uint8_t uATAPISenseKey, uATAPIASC;
                memset(pThis->aCts[i].aIfs[j].abATAPISense, '\0', sizeof(pThis->aCts[i].aIfs[j].abATAPISense));
                pThis->aCts[i].aIfs[j].abATAPISense[0] = 0x70 | (1 << 7);
                pThis->aCts[i].aIfs[j].abATAPISense[7] = 10;
                SSMR3GetU8(pSSM, &uATAPISenseKey);
                SSMR3GetU8(pSSM, &uATAPIASC);
                pThis->aCts[i].aIfs[j].abATAPISense[2] = uATAPISenseKey & 0x0f;
                pThis->aCts[i].aIfs[j].abATAPISense[12] = uATAPIASC;
            }
            /** @todo triple-check this hack after passthrough is working */
            SSMR3GetU8(pSSM, &pThis->aCts[i].aIfs[j].cNotifiedMediaChange);
            if (uVersion > ATA_SAVED_STATE_VERSION_WITHOUT_EVENT_STATUS)
                SSMR3GetU32(pSSM, (uint32_t*)&pThis->aCts[i].aIfs[j].MediaEventStatus);
            else
                pThis->aCts[i].aIfs[j].MediaEventStatus = ATA_EVENT_STATUS_UNCHANGED;
            SSMR3GetMem(pSSM, &pThis->aCts[i].aIfs[j].Led, sizeof(pThis->aCts[i].aIfs[j].Led));
            SSMR3GetU32(pSSM, &pThis->aCts[i].aIfs[j].cbIOBuffer);
            if (pThis->aCts[i].aIfs[j].cbIOBuffer)
            {
                if (pThis->aCts[i].aIfs[j].CTX_SUFF(pbIOBuffer))
                    SSMR3GetMem(pSSM, pThis->aCts[i].aIfs[j].CTX_SUFF(pbIOBuffer), pThis->aCts[i].aIfs[j].cbIOBuffer);
                else
                {
                    LogRel(("ATA: No buffer for %d/%d\n", i, j));
                    if (SSMR3HandleGetAfter(pSSM) != SSMAFTER_DEBUG_IT)
                        return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("No buffer for %d/%d"), i, j);

                    /* skip the buffer if we're loading for the debugger / animator. */
                    uint8_t u8Ignored;
                    size_t cbLeft = pThis->aCts[i].aIfs[j].cbIOBuffer;
                    while (cbLeft-- > 0)
                        SSMR3GetU8(pSSM, &u8Ignored);
                }
            }
            else
                Assert(pThis->aCts[i].aIfs[j].CTX_SUFF(pbIOBuffer) == NULL);
        }
    }
    if (uVersion <= ATA_SAVED_STATE_VERSION_VBOX_30)
        SSMR3GetU8(pSSM, &pThis->u8Type);

    rc = SSMR3GetU32(pSSM, &u32);
    if (RT_FAILURE(rc))
        return rc;
    if (u32 != ~0U)
    {
        AssertMsgFailed(("u32=%#x expected ~0\n", u32));
        rc = VERR_SSM_DATA_UNIT_FORMAT_CHANGED;
        return rc;
    }

    return VINF_SUCCESS;
}


/**
 * Callback employed by ataSuspend and ataR3PowerOff.
 *
 * @returns true if we've quiesced, false if we're still working.
 * @param   pDevIns     The device instance.
 */
static DECLCALLBACK(bool) ataR3IsAsyncSuspendOrPowerOffDone(PPDMDEVINS pDevIns)
{
    return ataR3AllAsyncIOIsIdle(pDevIns);
}


/**
 * Common worker for ataSuspend and ataR3PowerOff.
 */
static void ataR3SuspendOrPowerOff(PPDMDEVINS pDevIns)
{
    if (!ataR3AllAsyncIOIsIdle(pDevIns))
        PDMDevHlpSetAsyncNotification(pDevIns, ataR3IsAsyncSuspendOrPowerOffDone);
}


/**
 * Power Off notification.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance data.
 */
static DECLCALLBACK(void) ataR3PowerOff(PPDMDEVINS pDevIns)
{
    Log(("%s:\n", __FUNCTION__));
    ataR3SuspendOrPowerOff(pDevIns);
}


/**
 * Suspend notification.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance data.
 */
static DECLCALLBACK(void) ataR3Suspend(PPDMDEVINS pDevIns)
{
    Log(("%s:\n", __FUNCTION__));
    ataR3SuspendOrPowerOff(pDevIns);
}


/**
 * Callback employed by ataR3Reset.
 *
 * @returns true if we've quiesced, false if we're still working.
 * @param   pDevIns     The device instance.
 */
static DECLCALLBACK(bool) ataR3IsAsyncResetDone(PPDMDEVINS pDevIns)
{
    PCIATAState *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);

    if (!ataR3AllAsyncIOIsIdle(pDevIns))
        return false;

    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        PDMCritSectEnter(&pThis->aCts[i].lock, VERR_INTERNAL_ERROR);
        for (uint32_t j = 0; j < RT_ELEMENTS(pThis->aCts[i].aIfs); j++)
            ataR3ResetDevice(&pThis->aCts[i].aIfs[j]);
        PDMCritSectLeave(&pThis->aCts[i].lock);
    }
    return true;
}


/**
 * Common reset worker for ataR3Reset and ataR3Construct.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance data.
 * @param   fConstruct  Indicates who is calling.
 */
static int ataR3ResetCommon(PPDMDEVINS pDevIns, bool fConstruct)
{
    PCIATAState *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);

    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        PDMCritSectEnter(&pThis->aCts[i].lock, VERR_INTERNAL_ERROR);

        pThis->aCts[i].iSelectedIf = 0;
        pThis->aCts[i].iAIOIf = 0;
        pThis->aCts[i].BmDma.u8Cmd = 0;
        /* Report that both drives present on the bus are in DMA mode. This
         * pretends that there is a BIOS that has set it up. Normal reset
         * default is 0x00. */
        pThis->aCts[i].BmDma.u8Status =   (pThis->aCts[i].aIfs[0].pDrvBase != NULL ? BM_STATUS_D0DMA : 0)
                                        | (pThis->aCts[i].aIfs[1].pDrvBase != NULL ? BM_STATUS_D1DMA : 0);
        pThis->aCts[i].BmDma.GCPhysAddr = 0;

        pThis->aCts[i].fReset = true;
        pThis->aCts[i].fRedo = false;
        pThis->aCts[i].fRedoIdle = false;
        ataR3AsyncIOClearRequests(&pThis->aCts[i]);
        Log2(("%s: Ctl#%d: message to async I/O thread, reset controller\n", __FUNCTION__, i));
        ataHCAsyncIOPutRequest(&pThis->aCts[i], &g_ataResetARequest);
        ataHCAsyncIOPutRequest(&pThis->aCts[i], &g_ataResetCRequest);

        PDMCritSectLeave(&pThis->aCts[i].lock);
    }

    int rcRet = VINF_SUCCESS;
    if (!fConstruct)
    {
        /*
         * Setup asynchronous notification completion if the requests haven't
         * completed yet.
         */
        if (!ataR3IsAsyncResetDone(pDevIns))
            PDMDevHlpSetAsyncNotification(pDevIns, ataR3IsAsyncResetDone);
    }
    else
    {
        /*
         * Wait for the requests for complete.
         *
         * Would be real nice if we could do it all from EMT(0) and not
         * involve the worker threads, then we could dispense with all the
         * waiting and semaphore ping-pong here...
         */
        for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
        {
            if (pThis->aCts[i].AsyncIOThread != NIL_RTTHREAD)
            {
                int rc = PDMCritSectEnter(&pThis->aCts[i].AsyncIORequestLock, VERR_IGNORED);
                AssertRC(rc);

                ASMAtomicWriteBool(&pThis->aCts[i].fSignalIdle, true);
                rc = RTThreadUserReset(pThis->aCts[i].AsyncIOThread);
                AssertRC(rc);

                rc = PDMCritSectLeave(&pThis->aCts[i].AsyncIORequestLock);
                AssertRC(rc);

                if (!ataR3AsyncIOIsIdle(&pThis->aCts[i], false /*fStrict*/))
                {
                    rc = RTThreadUserWait(pThis->aCts[i].AsyncIOThread,  30*1000 /*ms*/);
                    if (RT_FAILURE(rc))
                        rc = RTThreadUserWait(pThis->aCts[i].AsyncIOThread, 1000 /*ms*/);
                    if (RT_FAILURE(rc))
                    {
                        AssertRC(rc);
                        rcRet = rc;
                    }
                }
            }
            ASMAtomicWriteBool(&pThis->aCts[i].fSignalIdle, false);
        }
        if (RT_SUCCESS(rcRet))
        {
            rcRet = ataR3IsAsyncResetDone(pDevIns) ? VINF_SUCCESS : VERR_INTERNAL_ERROR;
            AssertRC(rcRet);
        }
    }
    return rcRet;
}

/**
 * Reset notification.
 *
 * @param   pDevIns     The device instance data.
 */
static DECLCALLBACK(void)  ataR3Reset(PPDMDEVINS pDevIns)
{
    ataR3ResetCommon(pDevIns, false /*fConstruct*/);
}

/**
 * @copydoc FNPDMDEVRELOCATE
 */
static DECLCALLBACK(void) ataR3Relocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta)
{
    PCIATAState *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);

    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        pThis->aCts[i].pDevInsRC += offDelta;
        pThis->aCts[i].aIfs[0].pDevInsRC += offDelta;
        pThis->aCts[i].aIfs[0].pControllerRC += offDelta;
        ataR3RelocBuffer(pDevIns, &pThis->aCts[i].aIfs[0]);
        pThis->aCts[i].aIfs[1].pDevInsRC += offDelta;
        pThis->aCts[i].aIfs[1].pControllerRC += offDelta;
        ataR3RelocBuffer(pDevIns, &pThis->aCts[i].aIfs[1]);
    }
}

/**
 * Destroy a driver instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that any non-VM
 * resources can be freed correctly.
 *
 * @param   pDevIns     The device instance data.
 */
static DECLCALLBACK(int) ataR3Destruct(PPDMDEVINS pDevIns)
{
    PCIATAState    *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    int             rc;

    Log(("ataR3Destruct\n"));
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);

    /*
     * Tell the async I/O threads to terminate.
     */
    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        if (pThis->aCts[i].AsyncIOThread != NIL_RTTHREAD)
        {
            ASMAtomicWriteU32(&pThis->aCts[i].fShutdown, true);
            rc = SUPSemEventSignal(pThis->aCts[i].pSupDrvSession, pThis->aCts[i].hAsyncIOSem);
            AssertRC(rc);
            rc = RTSemEventSignal(pThis->aCts[i].SuspendIOSem);
            AssertRC(rc);
        }
    }

    /*
     * Wait for the threads to terminate before destroying their resources.
     */
    for (unsigned i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        if (pThis->aCts[i].AsyncIOThread != NIL_RTTHREAD)
        {
            rc = RTThreadWait(pThis->aCts[i].AsyncIOThread, 30000 /* 30 s*/, NULL);
            if (RT_SUCCESS(rc))
                pThis->aCts[i].AsyncIOThread = NIL_RTTHREAD;
            else
                LogRel(("PIIX3 ATA Dtor: Ctl#%u is still executing, DevSel=%d AIOIf=%d CmdIf0=%#04x CmdIf1=%#04x rc=%Rrc\n",
                        i, pThis->aCts[i].iSelectedIf, pThis->aCts[i].iAIOIf,
                        pThis->aCts[i].aIfs[0].uATARegCommand, pThis->aCts[i].aIfs[1].uATARegCommand, rc));
        }
    }

    /*
     * Free resources.
     */
    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        if (PDMCritSectIsInitialized(&pThis->aCts[i].AsyncIORequestLock))
            PDMR3CritSectDelete(&pThis->aCts[i].AsyncIORequestLock);
        if (pThis->aCts[i].hAsyncIOSem != NIL_SUPSEMEVENT)
        {
            SUPSemEventClose(pThis->aCts[i].pSupDrvSession, pThis->aCts[i].hAsyncIOSem);
            pThis->aCts[i].hAsyncIOSem = NIL_SUPSEMEVENT;
        }
        if (pThis->aCts[i].SuspendIOSem != NIL_RTSEMEVENT)
        {
            RTSemEventDestroy(pThis->aCts[i].SuspendIOSem);
            pThis->aCts[i].SuspendIOSem = NIL_RTSEMEVENT;
        }

        /* try one final time */
        if (pThis->aCts[i].AsyncIOThread != NIL_RTTHREAD)
        {
            rc = RTThreadWait(pThis->aCts[i].AsyncIOThread, 1 /*ms*/, NULL);
            if (RT_SUCCESS(rc))
            {
                pThis->aCts[i].AsyncIOThread = NIL_RTTHREAD;
                LogRel(("PIIX3 ATA Dtor: Ctl#%u actually completed.\n", i));
            }
        }

        for (uint32_t iIf = 0; iIf < RT_ELEMENTS(pThis->aCts[i].aIfs); iIf++)
        {
            if (pThis->aCts[i].aIfs[iIf].pTrackList)
            {
                ATAPIPassthroughTrackListDestroy(pThis->aCts[i].aIfs[iIf].pTrackList);
                pThis->aCts[i].aIfs[iIf].pTrackList = NULL;
            }
        }
    }

    return VINF_SUCCESS;
}

/**
 * Convert config value to DEVPCBIOSBOOT.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance data.
 * @param   pCfg        Configuration handle.
 * @param   penmChipset Where to store the chipset type.
 */
static int ataR3ControllerFromCfg(PPDMDEVINS pDevIns, PCFGMNODE pCfg, CHIPSET *penmChipset)
{
    char szType[20];

    int rc = CFGMR3QueryStringDef(pCfg, "Type", &szType[0], sizeof(szType), "PIIX4");
    if (RT_FAILURE(rc))
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("Configuration error: Querying \"Type\" as a string failed"));
    if (!strcmp(szType, "PIIX3"))
        *penmChipset = CHIPSET_PIIX3;
    else if (!strcmp(szType, "PIIX4"))
        *penmChipset = CHIPSET_PIIX4;
    else if (!strcmp(szType, "ICH6"))
        *penmChipset = CHIPSET_ICH6;
    else
    {
        PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                            N_("Configuration error: The \"Type\" value \"%s\" is unknown"),
                            szType);
        rc = VERR_INTERNAL_ERROR;
    }
    return rc;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) ataR3Construct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    PCIATAState    *pThis = PDMINS_2_DATA(pDevIns, PCIATAState *);
    PPDMIBASE       pBase;
    int             rc;
    bool            fRCEnabled;
    bool            fR0Enabled;
    uint32_t        DelayIRQMillies;

    Assert(iInstance == 0);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Initialize NIL handle values (for the destructor).
     */
    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        pThis->aCts[i].hAsyncIOSem = NIL_SUPSEMEVENT;
        pThis->aCts[i].SuspendIOSem = NIL_RTSEMEVENT;
        pThis->aCts[i].AsyncIOThread = NIL_RTTHREAD;
    }

    /*
     * Validate and read configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg,
                              "GCEnabled\0"
                              "R0Enabled\0"
                              "IRQDelay\0"
                              "Type\0")
        /** @todo || invalid keys */)
        return PDMDEV_SET_ERROR(pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                N_("PIIX3 configuration error: unknown option specified"));

    rc = CFGMR3QueryBoolDef(pCfg, "GCEnabled", &fRCEnabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("PIIX3 configuration error: failed to read GCEnabled as boolean"));
    Log(("%s: fRCEnabled=%d\n", __FUNCTION__, fRCEnabled));

    rc = CFGMR3QueryBoolDef(pCfg, "R0Enabled", &fR0Enabled, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("PIIX3 configuration error: failed to read R0Enabled as boolean"));
    Log(("%s: fR0Enabled=%d\n", __FUNCTION__, fR0Enabled));

    rc = CFGMR3QueryU32Def(pCfg, "IRQDelay", &DelayIRQMillies, 0);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("PIIX3 configuration error: failed to read IRQDelay as integer"));
    Log(("%s: DelayIRQMillies=%d\n", __FUNCTION__, DelayIRQMillies));
    Assert(DelayIRQMillies < 50);

    CHIPSET enmChipset = CHIPSET_PIIX3;
    rc = ataR3ControllerFromCfg(pDevIns, pCfg, &enmChipset);
    if (RT_FAILURE(rc))
        return rc;
    pThis->u8Type = (uint8_t)enmChipset;

    /*
     * Initialize data (most of it anyway).
     */
    /* Status LUN. */
    pThis->IBase.pfnQueryInterface = ataR3Status_QueryInterface;
    pThis->ILeds.pfnQueryStatusLed = ataR3Status_QueryStatusLed;

    /* PCI configuration space. */
    PCIDevSetVendorId(&pThis->dev, 0x8086); /* Intel */

    /*
     * When adding more IDE chipsets, don't forget to update pci_bios_init_device()
     * as it explicitly checks for PCI id for IDE controllers.
     */
    switch (pThis->u8Type)
    {
        case CHIPSET_ICH6:
            PCIDevSetDeviceId(&pThis->dev, 0x269e); /* ICH6 IDE */
            /** @todo do we need it? Do we need anything else? */
            pThis->dev.abConfig[0x48] = 0x00; /* UDMACTL */
            pThis->dev.abConfig[0x4A] = 0x00; /* UDMATIM */
            pThis->dev.abConfig[0x4B] = 0x00;
            {
                /*
                 * See www.intel.com/Assets/PDF/manual/298600.pdf p. 30
                 * Report
                 *   WR_Ping-Pong_EN: must be set
                 *   PCR0, PCR1: 80-pin primary cable reporting for both disks
                 *   SCR0, SCR1: 80-pin secondary cable reporting for both disks
                 */
                uint16_t u16Config = (1<<10) | (1<<7)  | (1<<6) | (1<<5) | (1<<4) ;
                pThis->dev.abConfig[0x54] = u16Config & 0xff;
                pThis->dev.abConfig[0x55] = u16Config >> 8;
            }
            break;
        case CHIPSET_PIIX4:
            PCIDevSetDeviceId(&pThis->dev, 0x7111); /* PIIX4 IDE */
            PCIDevSetRevisionId(&pThis->dev, 0x01); /* PIIX4E */
            pThis->dev.abConfig[0x48] = 0x00; /* UDMACTL */
            pThis->dev.abConfig[0x4A] = 0x00; /* UDMATIM */
            pThis->dev.abConfig[0x4B] = 0x00;
            break;
        case CHIPSET_PIIX3:
            PCIDevSetDeviceId(&pThis->dev, 0x7010); /* PIIX3 IDE */
            break;
        default:
            AssertMsgFailed(("Unsupported IDE chipset type: %d\n", pThis->u8Type));
    }

    /** @todo
     * This is the job of the BIOS / EFI!
     *
     * The same is done in DevPCI.cpp / pci_bios_init_device() but there is no
     * corresponding function in DevPciIch9.cpp. The EFI has corresponding code
     * in OvmfPkg/Library/PlatformBdsLib/BdsPlatform.c: NotifyDev() but this
     * function assumes that the IDE controller is located at PCI 00:01.1 which
     * is not true if the ICH9 chipset is used.
     */
    PCIDevSetWord(&pThis->dev, 0x40, 0x8000); /* enable IDE0 */
    PCIDevSetWord(&pThis->dev, 0x42, 0x8000); /* enable IDE1 */

    PCIDevSetCommand(   &pThis->dev, PCI_COMMAND_IOACCESS | PCI_COMMAND_MEMACCESS | PCI_COMMAND_BUSMASTER);
    PCIDevSetClassProg( &pThis->dev, 0x8a); /* programming interface = PCI_IDE bus master is supported */
    PCIDevSetClassSub(  &pThis->dev, 0x01); /* class_sub = PCI_IDE */
    PCIDevSetClassBase( &pThis->dev, 0x01); /* class_base = PCI_mass_storage */
    PCIDevSetHeaderType(&pThis->dev, 0x00);

    pThis->pDevIns          = pDevIns;
    pThis->fRCEnabled       = fRCEnabled;
    pThis->fR0Enabled       = fR0Enabled;
    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        pThis->aCts[i].pDevInsR3 = pDevIns;
        pThis->aCts[i].pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
        pThis->aCts[i].pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
        pThis->aCts[i].DelayIRQMillies = (uint32_t)DelayIRQMillies;
        for (uint32_t j = 0; j < RT_ELEMENTS(pThis->aCts[i].aIfs); j++)
        {
            ATADevState *pIf = &pThis->aCts[i].aIfs[j];

            pIf->iLUN      = i * RT_ELEMENTS(pThis->aCts) + j;
            pIf->pDevInsR3 = pDevIns;
            pIf->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
            pIf->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
            pIf->pControllerR3 = &pThis->aCts[i];
            pIf->pControllerR0 = MMHyperR3ToR0(PDMDevHlpGetVM(pDevIns), &pThis->aCts[i]);
            pIf->pControllerRC = MMHyperR3ToRC(PDMDevHlpGetVM(pDevIns), &pThis->aCts[i]);
            pIf->IBase.pfnQueryInterface       = ataR3QueryInterface;
            pIf->IMountNotify.pfnMountNotify   = ataR3MountNotify;
            pIf->IMountNotify.pfnUnmountNotify = ataR3UnmountNotify;
            pIf->IPort.pfnQueryDeviceLocation  = ataR3QueryDeviceLocation;
            pIf->Led.u32Magic                  = PDMLED_MAGIC;
        }
    }

    Assert(RT_ELEMENTS(pThis->aCts) == 2);
    pThis->aCts[0].irq          = 14;
    pThis->aCts[0].IOPortBase1  = 0x1f0;
    pThis->aCts[0].IOPortBase2  = 0x3f6;
    pThis->aCts[1].irq          = 15;
    pThis->aCts[1].IOPortBase1  = 0x170;
    pThis->aCts[1].IOPortBase2  = 0x376;

    /*
     * Set the default critical section to NOP as we lock on controller level.
     */
    rc = PDMDevHlpSetDeviceCritSect(pDevIns, PDMDevHlpCritSectGetNop(pDevIns));
    AssertRCReturn(rc, rc);

    /*
     * Register the PCI device.
     */
    rc = PDMDevHlpPCIRegisterEx(pDevIns, &pThis->dev, PDMPCIDEVREG_CFG_PRIMARY, PDMPCIDEVREG_F_NOT_MANDATORY_NO,
                                1 /*uPciDevNo*/, 1 /*uPciDevFn*/, "piix3ide");
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("PIIX3 cannot register PCI device"));
    rc = PDMDevHlpPCIIORegionRegister(pDevIns, 4, 0x10, PCI_ADDRESS_SPACE_IO, ataR3BMDMAIORangeMap);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("PIIX3 cannot register PCI I/O region for BMDMA"));

    /*
     * Register the I/O ports.
     * The ports are all hardcoded and enforced by the PIIX3 host bridge controller.
     */
    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        rc = PDMDevHlpIOPortRegister(pDevIns, pThis->aCts[i].IOPortBase1, 1, (RTHCPTR)(uintptr_t)i,
                                     ataIOPortWrite1Data, ataIOPortRead1Data,
                                     ataIOPortWriteStr1Data, ataIOPortReadStr1Data, "ATA I/O Base 1 - Data");
        AssertLogRelRCReturn(rc, rc);
        rc = PDMDevHlpIOPortRegister(pDevIns, pThis->aCts[i].IOPortBase1 + 1, 7, (RTHCPTR)(uintptr_t)i,
                                     ataIOPortWrite1Other, ataIOPortRead1Other, NULL, NULL, "ATA I/O Base 1 - Other");

        AssertLogRelRCReturn(rc, rc);
        if (fRCEnabled)
        {
            rc = PDMDevHlpIOPortRegisterRC(pDevIns, pThis->aCts[i].IOPortBase1, 1, (RTGCPTR)i,
                                           "ataIOPortWrite1Data", "ataIOPortRead1Data",
                                           "ataIOPortWriteStr1Data", "ataIOPortReadStr1Data", "ATA I/O Base 1 - Data");
            AssertLogRelRCReturn(rc, rc);
            rc = PDMDevHlpIOPortRegisterRC(pDevIns, pThis->aCts[i].IOPortBase1 + 1, 7, (RTGCPTR)i,
                                           "ataIOPortWrite1Other", "ataIOPortRead1Other", NULL, NULL, "ATA I/O Base 1 - Other");
            AssertLogRelRCReturn(rc, rc);
        }

        if (fR0Enabled)
        {
#if 0
            rc = PDMDevHlpIOPortRegisterR0(pDevIns, pThis->aCts[i].IOPortBase1, 1, (RTR0PTR)i,
                                           "ataIOPortWrite1Data", "ataIOPortRead1Data", NULL, NULL, "ATA I/O Base 1 - Data");
#else
            rc = PDMDevHlpIOPortRegisterR0(pDevIns, pThis->aCts[i].IOPortBase1, 1, (RTR0PTR)i,
                                           "ataIOPortWrite1Data", "ataIOPortRead1Data",
                                           "ataIOPortWriteStr1Data", "ataIOPortReadStr1Data", "ATA I/O Base 1 - Data");
#endif
            AssertLogRelRCReturn(rc, rc);
            rc = PDMDevHlpIOPortRegisterR0(pDevIns, pThis->aCts[i].IOPortBase1 + 1, 7, (RTR0PTR)i,
                                           "ataIOPortWrite1Other", "ataIOPortRead1Other", NULL, NULL, "ATA I/O Base 1 - Other");
            AssertLogRelRCReturn(rc, rc);
        }

        rc = PDMDevHlpIOPortRegister(pDevIns, pThis->aCts[i].IOPortBase2, 1, (RTHCPTR)(uintptr_t)i,
                                     ataIOPortWrite2, ataIOPortRead2, NULL, NULL, "ATA I/O Base 2");
        if (RT_FAILURE(rc))
            return PDMDEV_SET_ERROR(pDevIns, rc, N_("PIIX3 cannot register base2 I/O handlers"));

        if (fRCEnabled)
        {
            rc = PDMDevHlpIOPortRegisterRC(pDevIns, pThis->aCts[i].IOPortBase2, 1, (RTGCPTR)i,
                                           "ataIOPortWrite2", "ataIOPortRead2", NULL, NULL, "ATA I/O Base 2");
            if (RT_FAILURE(rc))
                return PDMDEV_SET_ERROR(pDevIns, rc, N_("PIIX3 cannot register base2 I/O handlers (GC)"));
        }
        if (fR0Enabled)
        {
            rc = PDMDevHlpIOPortRegisterR0(pDevIns, pThis->aCts[i].IOPortBase2, 1, (RTR0PTR)i,
                                           "ataIOPortWrite2", "ataIOPortRead2", NULL, NULL, "ATA I/O Base 2");
            if (RT_FAILURE(rc))
                return PDMDEV_SET_ERROR(pDevIns, rc, N_("PIIX3 cannot register base2 I/O handlers (R0)"));
        }

        for (uint32_t j = 0; j < RT_ELEMENTS(pThis->aCts[i].aIfs); j++)
        {
            ATADevState *pIf = &pThis->aCts[i].aIfs[j];
            PDMDevHlpSTAMRegisterF(pDevIns, &pIf->StatATADMA,       STAMTYPE_COUNTER,    STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,
                                   "Number of ATA DMA transfers.",              "/Devices/IDE%d/ATA%d/Unit%d/DMA", iInstance, i, j);
            PDMDevHlpSTAMRegisterF(pDevIns, &pIf->StatATAPIO,       STAMTYPE_COUNTER,    STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,
                                   "Number of ATA PIO transfers.",              "/Devices/IDE%d/ATA%d/Unit%d/PIO", iInstance, i, j);
            PDMDevHlpSTAMRegisterF(pDevIns, &pIf->StatATAPIDMA,     STAMTYPE_COUNTER,    STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,
                                   "Number of ATAPI DMA transfers.",            "/Devices/IDE%d/ATA%d/Unit%d/AtapiDMA", iInstance, i, j);
            PDMDevHlpSTAMRegisterF(pDevIns, &pIf->StatATAPIPIO,     STAMTYPE_COUNTER,    STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,
                                   "Number of ATAPI PIO transfers.",            "/Devices/IDE%d/ATA%d/Unit%d/AtapiPIO", iInstance, i, j);
#ifdef VBOX_WITH_STATISTICS /** @todo release too. */
            PDMDevHlpSTAMRegisterF(pDevIns, &pIf->StatReads,        STAMTYPE_PROFILE_ADV, STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_CALL,
                                   "Profiling of the read operations.",         "/Devices/IDE%d/ATA%d/Unit%d/Reads", iInstance, i, j);
#endif
            PDMDevHlpSTAMRegisterF(pDevIns, &pIf->StatBytesRead,    STAMTYPE_COUNTER,     STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,
                                   "Amount of data read.",                      "/Devices/IDE%d/ATA%d/Unit%d/ReadBytes", iInstance, i, j);
#ifdef VBOX_INSTRUMENT_DMA_WRITES
            PDMDevHlpSTAMRegisterF(pDevIns, &pIf->StatInstrVDWrites,STAMTYPE_PROFILE_ADV, STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_CALL,
                                   "Profiling of the VD DMA write operations.", "/Devices/IDE%d/ATA%d/Unit%d/InstrVDWrites", iInstance, i, j);
#endif
#ifdef VBOX_WITH_STATISTICS
            PDMDevHlpSTAMRegisterF(pDevIns, &pIf->StatWrites,       STAMTYPE_PROFILE_ADV, STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_CALL,
                                   "Profiling of the write operations.",        "/Devices/IDE%d/ATA%d/Unit%d/Writes", iInstance, i, j);
#endif
            PDMDevHlpSTAMRegisterF(pDevIns, &pIf->StatBytesWritten, STAMTYPE_COUNTER,     STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,
                                   "Amount of data written.",                   "/Devices/IDE%d/ATA%d/Unit%d/WrittenBytes", iInstance, i, j);
#ifdef VBOX_WITH_STATISTICS
            PDMDevHlpSTAMRegisterF(pDevIns, &pIf->StatFlushes,      STAMTYPE_PROFILE,     STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_CALL,
                                   "Profiling of the flush operations.",        "/Devices/IDE%d/ATA%d/Unit%d/Flushes", iInstance, i, j);
#endif
        }
#ifdef VBOX_WITH_STATISTICS /** @todo release too. */
        PDMDevHlpSTAMRegisterF(pDevIns, &pThis->aCts[i].StatAsyncOps,     STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_OCCURENCES,
                                   "The number of async operations.",   "/Devices/IDE%d/ATA%d/Async/Operations", iInstance, i);
        /** @todo STAMUNIT_MICROSECS */
        PDMDevHlpSTAMRegisterF(pDevIns, &pThis->aCts[i].StatAsyncMinWait, STAMTYPE_U64_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                                   "Minimum wait in microseconds.",     "/Devices/IDE%d/ATA%d/Async/MinWait", iInstance, i);
        PDMDevHlpSTAMRegisterF(pDevIns, &pThis->aCts[i].StatAsyncMaxWait, STAMTYPE_U64_RESET, STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                                   "Maximum wait in microseconds.",     "/Devices/IDE%d/ATA%d/Async/MaxWait", iInstance, i);
        PDMDevHlpSTAMRegisterF(pDevIns, &pThis->aCts[i].StatAsyncTimeUS,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_NONE,
                                   "Total time spent in microseconds.", "/Devices/IDE%d/ATA%d/Async/TotalTimeUS", iInstance, i);
        PDMDevHlpSTAMRegisterF(pDevIns, &pThis->aCts[i].StatAsyncTime,    STAMTYPE_PROFILE_ADV, STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_CALL,
                                   "Profiling of async operations.",    "/Devices/IDE%d/ATA%d/Async/Time", iInstance, i);
        PDMDevHlpSTAMRegisterF(pDevIns, &pThis->aCts[i].StatLockWait,     STAMTYPE_PROFILE, STAMVISIBILITY_ALWAYS, STAMUNIT_TICKS_PER_CALL,
                                   "Profiling of locks.",               "/Devices/IDE%d/ATA%d/Async/LockWait", iInstance, i);
#endif /* VBOX_WITH_STATISTICS */

        /* Initialize per-controller critical section. */
        rc = PDMDevHlpCritSectInit(pDevIns, &pThis->aCts[i].lock,               RT_SRC_POS, "ATA#%u-Ctl", i);
        AssertLogRelRCReturn(rc, rc);

        /* Initialize per-controller async I/O request critical section. */
        rc = PDMDevHlpCritSectInit(pDevIns, &pThis->aCts[i].AsyncIORequestLock, RT_SRC_POS, "ATA#%u-Req", i);
        AssertLogRelRCReturn(rc, rc);
    }

    /*
     * Attach status driver (optional).
     */
    rc = PDMDevHlpDriverAttach(pDevIns, PDM_STATUS_LUN, &pThis->IBase, &pBase, "Status Port");
    if (RT_SUCCESS(rc))
    {
        pThis->pLedsConnector = PDMIBASE_QUERY_INTERFACE(pBase, PDMILEDCONNECTORS);
        pThis->pMediaNotify = PDMIBASE_QUERY_INTERFACE(pBase, PDMIMEDIANOTIFY);
    }
    else if (rc != VERR_PDM_NO_ATTACHED_DRIVER)
    {
        AssertMsgFailed(("Failed to attach to status driver. rc=%Rrc\n", rc));
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("PIIX3 cannot attach to status driver"));
    }

    /*
     * Attach the units.
     */
    uint32_t cbTotalBuffer = 0;
    for (uint32_t i = 0; i < RT_ELEMENTS(pThis->aCts); i++)
    {
        PATACONTROLLER pCtl = &pThis->aCts[i];

        /*
         * Start the worker thread.
         */
        pCtl->uAsyncIOState = ATA_AIO_NEW;
        pCtl->pSupDrvSession = PDMDevHlpGetSupDrvSession(pDevIns);
        rc = SUPSemEventCreate(pCtl->pSupDrvSession, &pCtl->hAsyncIOSem);
        AssertLogRelRCReturn(rc, rc);
        rc = RTSemEventCreate(&pCtl->SuspendIOSem);
        AssertLogRelRCReturn(rc, rc);

        ataR3AsyncIOClearRequests(pCtl);
        rc = RTThreadCreateF(&pCtl->AsyncIOThread, ataR3AsyncIOThread, (void *)pCtl, 128*1024 /*cbStack*/,
                             RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, "ATA-%u", i);
        AssertLogRelRCReturn(rc, rc);
        Assert(  pCtl->AsyncIOThread != NIL_RTTHREAD    && pCtl->hAsyncIOSem != NIL_SUPSEMEVENT
               && pCtl->SuspendIOSem != NIL_RTSEMEVENT  && PDMCritSectIsInitialized(&pCtl->AsyncIORequestLock));
        Log(("%s: controller %d AIO thread id %#x; sem %p susp_sem %p\n", __FUNCTION__, i, pCtl->AsyncIOThread, pCtl->hAsyncIOSem, pCtl->SuspendIOSem));

        for (uint32_t j = 0; j < RT_ELEMENTS(pCtl->aIfs); j++)
        {
            static const char *s_apszDescs[RT_ELEMENTS(pThis->aCts)][RT_ELEMENTS(pCtl->aIfs)] =
            {
                { "Primary Master", "Primary Slave" },
                { "Secondary Master", "Secondary Slave" }
            };

            /*
             * Try attach the block device and get the interfaces,
             * required as well as optional.
             */
            ATADevState *pIf = &pCtl->aIfs[j];

            rc = PDMDevHlpDriverAttach(pDevIns, pIf->iLUN, &pIf->IBase, &pIf->pDrvBase, s_apszDescs[i][j]);
            if (RT_SUCCESS(rc))
            {
                rc = ataR3ConfigLun(pDevIns, pIf);
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Init vendor product data.
                     */
                    static const char *s_apszCFGMKeys[RT_ELEMENTS(pThis->aCts)][RT_ELEMENTS(pCtl->aIfs)] =
                    {
                        { "PrimaryMaster", "PrimarySlave" },
                        { "SecondaryMaster", "SecondarySlave" }
                    };

                    /* Generate a default serial number. */
                    char szSerial[ATA_SERIAL_NUMBER_LENGTH+1];
                    RTUUID Uuid;
                    if (pIf->pDrvMedia)
                        rc = pIf->pDrvMedia->pfnGetUuid(pIf->pDrvMedia, &Uuid);
                    else
                        RTUuidClear(&Uuid);

                    if (RT_FAILURE(rc) || RTUuidIsNull(&Uuid))
                    {
                        /* Generate a predictable serial for drives which don't have a UUID. */
                        RTStrPrintf(szSerial, sizeof(szSerial), "VB%x-%04x%04x",
                                    pIf->iLUN + pDevIns->iInstance * 32,
                                    pThis->aCts[i].IOPortBase1, pThis->aCts[i].IOPortBase2);
                    }
                    else
                        RTStrPrintf(szSerial, sizeof(szSerial), "VB%08x-%08x", Uuid.au32[0], Uuid.au32[3]);

                    /* Get user config if present using defaults otherwise. */
                    PCFGMNODE pCfgNode = CFGMR3GetChild(pCfg, s_apszCFGMKeys[i][j]);
                    rc = CFGMR3QueryStringDef(pCfgNode, "SerialNumber", pIf->szSerialNumber, sizeof(pIf->szSerialNumber),
                                              szSerial);
                    if (RT_FAILURE(rc))
                    {
                        if (rc == VERR_CFGM_NOT_ENOUGH_SPACE)
                            return PDMDEV_SET_ERROR(pDevIns, VERR_INVALID_PARAMETER,
                                        N_("PIIX3 configuration error: \"SerialNumber\" is longer than 20 bytes"));
                        return PDMDEV_SET_ERROR(pDevIns, rc,
                                  N_("PIIX3 configuration error: failed to read \"SerialNumber\" as string"));
                    }

                    rc = CFGMR3QueryStringDef(pCfgNode, "FirmwareRevision", pIf->szFirmwareRevision, sizeof(pIf->szFirmwareRevision),
                                              "1.0");
                    if (RT_FAILURE(rc))
                    {
                        if (rc == VERR_CFGM_NOT_ENOUGH_SPACE)
                            return PDMDEV_SET_ERROR(pDevIns, VERR_INVALID_PARAMETER,
                                        N_("PIIX3 configuration error: \"FirmwareRevision\" is longer than 8 bytes"));
                        return PDMDEV_SET_ERROR(pDevIns, rc,
                                    N_("PIIX3 configuration error: failed to read \"FirmwareRevision\" as string"));
                    }

                    rc = CFGMR3QueryStringDef(pCfgNode, "ModelNumber", pIf->szModelNumber, sizeof(pIf->szModelNumber),
                                              pIf->fATAPI ? "VBOX CD-ROM" : "VBOX HARDDISK");
                    if (RT_FAILURE(rc))
                    {
                        if (rc == VERR_CFGM_NOT_ENOUGH_SPACE)
                            return PDMDEV_SET_ERROR(pDevIns, VERR_INVALID_PARAMETER,
                                       N_("PIIX3 configuration error: \"ModelNumber\" is longer than 40 bytes"));
                        return PDMDEV_SET_ERROR(pDevIns, rc,
                                    N_("PIIX3 configuration error: failed to read \"ModelNumber\" as string"));
                    }

                    /* There are three other identification strings for CD drives used for INQUIRY */
                    if (pIf->fATAPI)
                    {
                        rc = CFGMR3QueryStringDef(pCfgNode, "ATAPIVendorId", pIf->szInquiryVendorId, sizeof(pIf->szInquiryVendorId),
                                                  "VBOX");
                        if (RT_FAILURE(rc))
                        {
                            if (rc == VERR_CFGM_NOT_ENOUGH_SPACE)
                                return PDMDEV_SET_ERROR(pDevIns, VERR_INVALID_PARAMETER,
                                           N_("PIIX3 configuration error: \"ATAPIVendorId\" is longer than 16 bytes"));
                            return PDMDEV_SET_ERROR(pDevIns, rc,
                                        N_("PIIX3 configuration error: failed to read \"ATAPIVendorId\" as string"));
                        }

                        rc = CFGMR3QueryStringDef(pCfgNode, "ATAPIProductId", pIf->szInquiryProductId, sizeof(pIf->szInquiryProductId),
                                                  "CD-ROM");
                        if (RT_FAILURE(rc))
                        {
                            if (rc == VERR_CFGM_NOT_ENOUGH_SPACE)
                                return PDMDEV_SET_ERROR(pDevIns, VERR_INVALID_PARAMETER,
                                           N_("PIIX3 configuration error: \"ATAPIProductId\" is longer than 16 bytes"));
                            return PDMDEV_SET_ERROR(pDevIns, rc,
                                        N_("PIIX3 configuration error: failed to read \"ATAPIProductId\" as string"));
                        }

                        rc = CFGMR3QueryStringDef(pCfgNode, "ATAPIRevision", pIf->szInquiryRevision, sizeof(pIf->szInquiryRevision),
                                                  "1.0");
                        if (RT_FAILURE(rc))
                        {
                            if (rc == VERR_CFGM_NOT_ENOUGH_SPACE)
                                return PDMDEV_SET_ERROR(pDevIns, VERR_INVALID_PARAMETER,
                                           N_("PIIX3 configuration error: \"ATAPIRevision\" is longer than 4 bytes"));
                            return PDMDEV_SET_ERROR(pDevIns, rc,
                                        N_("PIIX3 configuration error: failed to read \"ATAPIRevision\" as string"));
                        }

                        rc = CFGMR3QueryBoolDef(pCfgNode, "OverwriteInquiry", &pIf->fOverwriteInquiry, true);
                        if (RT_FAILURE(rc))
                            return PDMDEV_SET_ERROR(pDevIns, rc,
                                        N_("PIIX3 configuration error: failed to read \"OverwriteInquiry\" as boolean"));
                    }
                }

            }
            else if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
            {
                pIf->pDrvBase = NULL;
                pIf->pDrvMedia = NULL;
                pIf->cbIOBuffer = 0;
                pIf->pbIOBufferR3 = NULL;
                pIf->pbIOBufferR0 = NIL_RTR0PTR;
                pIf->pbIOBufferRC = NIL_RTGCPTR;
                LogRel(("PIIX3 ATA: LUN#%d: no unit\n", pIf->iLUN));
            }
            else
            {
                switch (rc)
                {
                    case VERR_ACCESS_DENIED:
                        /* Error already cached by DrvHostBase */
                        return rc;
                    default:
                        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                                   N_("PIIX3 cannot attach drive to the %s"),
                                                   s_apszDescs[i][j]);
                }
            }
            cbTotalBuffer += pIf->cbIOBuffer;
        }
    }

    rc = PDMDevHlpSSMRegisterEx(pDevIns, ATA_SAVED_STATE_VERSION, sizeof(*pThis) + cbTotalBuffer, NULL,
                                NULL,            ataR3LiveExec, NULL,
                                ataR3SaveLoadPrep, ataR3SaveExec, NULL,
                                ataR3SaveLoadPrep, ataR3LoadExec, NULL);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("PIIX3 cannot register save state handlers"));

    /*
     * Initialize the device state.
     */
    return ataR3ResetCommon(pDevIns, true /*fConstruct*/);
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DevicePIIX3IDE =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "piix3ide",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "Intel PIIX3 ATA controller.\n"
    "  LUN #0 is primary master.\n"
    "  LUN #1 is primary slave.\n"
    "  LUN #2 is secondary master.\n"
    "  LUN #3 is secondary slave.\n"
    "  LUN #999 is the LED/Status connector.",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0 |
    PDM_DEVREG_FLAGS_FIRST_SUSPEND_NOTIFICATION | PDM_DEVREG_FLAGS_FIRST_POWEROFF_NOTIFICATION |
    PDM_DEVREG_FLAGS_FIRST_RESET_NOTIFICATION,
    /* fClass */
    PDM_DEVREG_CLASS_STORAGE,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(PCIATAState),
    /* pfnConstruct */
    ataR3Construct,
    /* pfnDestruct */
    ataR3Destruct,
    /* pfnRelocate */
    ataR3Relocate,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    ataR3Reset,
    /* pfnSuspend */
    ataR3Suspend,
    /* pfnResume */
    ataR3Resume,
    /* pfnAttach */
    ataR3Attach,
    /* pfnDetach */
    ataR3Detach,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    ataR3PowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};
#endif /* IN_RING3 */
#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */
