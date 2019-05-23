/* $Id: DevOHCI.cpp $ */
/** @file
 * DevOHCI - Open Host Controller Interface for USB.
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

/** @page pg_dev_ohci   OHCI - Open Host Controller Interface Emulation.
 *
 * This component implements an OHCI USB controller. It is split roughly in
 * to two main parts, the first part implements the register level
 * specification of USB OHCI and the second part maintains the root hub (which
 * is an integrated component of the device).
 *
 * The OHCI registers are used for the usual stuff like enabling and disabling
 * interrupts. Since the USB time is divided in to 1ms frames and various
 * interrupts may need to be triggered at frame boundary time, a timer-based
 * approach was taken. Whenever the bus is enabled ohci->eof_timer will be set.
 *
 * The actual USB transfers are stored in main memory (along with endpoint and
 * transfer descriptors). The ED's for all the control and bulk endpoints are
 * found by consulting the HcControlHeadED and HcBulkHeadED registers
 * respectively. Interrupt ED's are different, they are found by looking
 * in the HCCA (another communication area in main memory).
 *
 * At the start of every frame (in function ohci_sof) we traverse all enabled
 * ED lists and queue up as many transfers as possible. No attention is paid
 * to control/bulk service ratios or bandwidth requirements since our USB
 * could conceivably contain a dozen high speed busses so this would
 * artificially limit the performance.
 *
 * Once we have a transfer ready to go (in function ohciServiceTd) we
 * allocate an URB on the stack,  fill in all the relevant fields and submit
 * it using the VUSBIRhSubmitUrb function. The roothub device and the virtual
 * USB core code (vusb.c) coordinates everything else from this point onwards.
 *
 * When the URB has been successfully handed to the lower level driver, our
 * prepare callback gets called and we can remove the TD from the ED transfer
 * list. This stops us queueing it twice while it completes.
 *  bird: no, we don't remove it because that confuses the guest! (=> crashes)
 *
 * Completed URBs are reaped at the end of every frame (in function
 * ohci_frame_boundary). Our completion routine makes use of the ED and TD
 * fields in the URB to store the physical addresses of the descriptors so
 * that they may be modified in the roothub callbacks. Our completion
 * routine (ohciRhXferComplete) carries out a number of tasks:
 *      -# Retires the TD associated with the transfer, setting the
 *         relevant error code etc.
 *      -# Updates done-queue interrupt timer and potentially causes
 *         a writeback of the done-queue.
 *      -# If the transfer was device-to-host, we copy the data in to
 *         the host memory.
 *
 * As for error handling OHCI allows for 3 retries before failing a transfer,
 * an error count is stored in each transfer descriptor. A halt flag is also
 * stored in the transfer descriptor. That allows for ED's to be disabled
 * without stopping the bus and de-queuing them.
 *
 * When the bus is started and stopped we call VUSBIDevPowerOn/Off() on our
 * roothub to indicate it's powering up and powering down. Whenever we power
 * down, the  USB core makes sure to synchronously complete all outstanding
 * requests so  that the OHCI is never seen in an inconsistent state by the
 * guest OS (Transfers are not meant to be unlinked until they've actually
 * completed, but we can't do that unless we work synchronously, so we just
 * have to fake it).
 *  bird: we do work synchronously now, anything causes guest crashes.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_OHCI
#include <VBox/pci.h>
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/mm.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/asm.h>
#include <iprt/asm-math.h>
#include <iprt/semaphore.h>
#include <iprt/critsect.h>
#include <iprt/param.h>
#ifdef IN_RING3
# include <iprt/alloca.h>
# include <iprt/mem.h>
# include <iprt/thread.h>
# include <iprt/uuid.h>
#endif
#include <VBox/vusb.h>
#include "VBoxDD.h"


#define VBOX_WITH_OHCI_PHYS_READ_CACHE
//#define VBOX_WITH_OHCI_PHYS_READ_STATS


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** The saved state version. */
#define OHCI_SAVED_STATE_VERSION            5
// The saved state with support of 8 ports
#define OHCI_SAVED_STATE_VERSION_8PORTS     4
/** The saved state version used in 3.0 and earlier.
 *
 * @remarks Because of the SSMR3MemPut/Get laziness we ended up with an
 *          accidental format change between 2.0 and 2.1 that didn't get its own
 *          version number.  It is therefore not possible to restore states from
 *          2.0 and earlier with 2.1 and later. */
#define OHCI_SAVED_STATE_VERSION_MEM_HELL   3


/** Maximum supported number of Downstream Ports on the root hub. 15 ports
 * is the maximum defined by the OHCI spec.
 * If you change this you need to add more status register words to the 'opreg'
 * array.
 */
#define OHCI_NDP_MAX        15

/** Default NDP, chosen to be compatible with everything. */
#define OHCI_NDP_DEFAULT    12

/* Macro to query the number of currently configured ports. */
#define OHCI_NDP_CFG(pohci) ((pohci)->RootHub.desc_a & OHCI_RHA_NDP)

/** Pointer to OHCI device data. */
typedef struct OHCI *POHCI;
/** Read-only pointer to the OHCI device data. */
typedef struct OHCI const *PCOHCI;

#ifndef VBOX_DEVICE_STRUCT_TESTCASE
/**
 * Host controller transfer descriptor data.
 */
typedef struct VUSBURBHCITDINT
{
    /** Type of TD. */
    uint32_t        TdType;
    /** The address of the */
    RTGCPHYS32      TdAddr;
    /** A copy of the TD. */
    uint32_t        TdCopy[16];
} VUSBURBHCITDINT;

/**
 * The host controller data associated with each URB.
 */
typedef struct VUSBURBHCIINT
{
    /** The endpoint descriptor address. */
    RTGCPHYS32      EdAddr;
    /** Number of Tds in the array. */
    uint32_t        cTds;
    /** When this URB was created.
     * (Used for isochronous frames and for logging.) */
    uint32_t        u32FrameNo;
    /** Flag indicating that the TDs have been unlinked. */
    bool            fUnlinked;
} VUSBURBHCIINT;
#endif

/**
 * An OHCI root hub port.
 */
typedef struct OHCIHUBPORT
{
    /** The port register. */
    uint32_t                fReg;
#if HC_ARCH_BITS == 64
    uint32_t                Alignment0; /**< Align the pointer correctly. */
#endif
    /** The device attached to the port. */
    R3PTRTYPE(PVUSBIDEVICE) pDev;
} OHCIHUBPORT;
#if HC_ARCH_BITS == 64
AssertCompile(sizeof(OHCIHUBPORT) == 16); /* saved state */
#endif
/** Pointer to an OHCI hub port. */
typedef OHCIHUBPORT *POHCIHUBPORT;

/**
 * The OHCI root hub.
 *
 * @implements  PDMIBASE
 * @implements  VUSBIROOTHUBPORT
 * @implements  PDMILEDPORTS
 */
typedef struct ohci_roothub
{
    /** Pointer to the base interface of the VUSB RootHub. */
    R3PTRTYPE(PPDMIBASE)                pIBase;
    /** Pointer to the connector interface of the VUSB RootHub. */
    R3PTRTYPE(PVUSBIROOTHUBCONNECTOR)   pIRhConn;
    /** Pointer to the device interface of the VUSB RootHub. */
    R3PTRTYPE(PVUSBIDEVICE)             pIDev;
    /** The base interface exposed to the roothub driver. */
    PDMIBASE                            IBase;
    /** The roothub port interface exposed to the roothub driver. */
    VUSBIROOTHUBPORT                    IRhPort;

    /** The LED. */
    PDMLED                              Led;
    /** The LED ports. */
    PDMILEDPORTS                        ILeds;
    /** Partner of ILeds. */
    R3PTRTYPE(PPDMILEDCONNECTORS)       pLedsConnector;

    uint32_t                            status;
    uint32_t                            desc_a;
    uint32_t                            desc_b;
#if HC_ARCH_BITS == 64
    uint32_t                            Alignment0; /**< Align aPorts on a 8 byte boundary. */
#endif
    OHCIHUBPORT                         aPorts[OHCI_NDP_MAX];
    R3PTRTYPE(POHCI)                    pOhci;
} OHCIROOTHUB;
/** Pointer to the OHCI root hub. */
typedef OHCIROOTHUB *POHCIROOTHUB;


/**
 * Data used for reattaching devices on a state load.
 */
typedef struct ohci_load {
    /** Timer used once after state load to inform the guest about new devices.
     * We do this to be sure the guest get any disconnect / reconnect on the
     * same port. */
    PTMTIMERR3 pTimer;
    /** Number of detached devices. */
    unsigned cDevs;
    /** Array of devices which were detached. */
    PVUSBIDEVICE apDevs[OHCI_NDP_MAX];
} OHCILOAD;
/** Pointer to an OHCILOAD structure. */
typedef OHCILOAD *POHCILOAD;

#ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
typedef struct OHCIPAGECACHE
{
    /** Last read physical page address. */
    RTGCPHYS            GCPhysReadCacheAddr;
    /** Copy of last read physical page. */
    uint8_t             au8PhysReadCache[PAGE_SIZE];
} OHCIPAGECACHE, *POHCIPAGECACHE;
#endif

/**
 * OHCI device data.
 */
typedef struct OHCI
{
    /** The PCI device. */
    PDMPCIDEV           PciDev;

    /** Pointer to the device instance - R3 ptr. */
    PPDMDEVINSR3        pDevInsR3;
    /** The End-Of-Frame timer - R3 Ptr. */
    PTMTIMERR3          pEndOfFrameTimerR3;

    /** Pointer to the device instance - R0 ptr */
    PPDMDEVINSR0        pDevInsR0;
    /** The End-Of-Frame timer - R0 Ptr. */
    PTMTIMERR0          pEndOfFrameTimerR0;

    /** Pointer to the device instance - RC ptr. */
    PPDMDEVINSRC        pDevInsRC;
    /** The End-Of-Frame timer - RC Ptr. */
    PTMTIMERRC          pEndOfFrameTimerRC;

    /** Start of current frame. */
    uint64_t            SofTime;
    /* done queue interrupt counter */
    uint32_t            dqic : 3;
    /** frame number overflow. */
    uint32_t            fno : 1;
    /** Address of the MMIO region assigned by PCI. */
    RTGCPHYS32          MMIOBase;

    /* Root hub device */
    OHCIROOTHUB         RootHub;

    /* OHCI registers */

    /** @name Control partition
     * @{ */
    /** HcControl. */
    uint32_t            ctl;
    /** HcCommandStatus. */
    uint32_t            status;
    /** HcInterruptStatus. */
    uint32_t            intr_status;
    /** HcInterruptEnabled. */
    uint32_t            intr;
    /** @} */

    /** @name Memory pointer partition
     * @{ */
    /** HcHCCA. */
    uint32_t            hcca;
    /** HcPeriodCurrentEd. */
    uint32_t            per_cur;
    /** HcControlCurrentED. */
    uint32_t            ctrl_cur;
    /** HcControlHeadED. */
    uint32_t            ctrl_head;
    /** HcBlockCurrendED. */
    uint32_t            bulk_cur;
    /** HcBlockHeadED. */
    uint32_t            bulk_head;
    /** HcDoneHead. */
    uint32_t            done;
    /** @} */

    /** @name Frame counter partition
     * @{ */
    /** HcFmInterval.FSMPS - FSLargestDataPacket */
    uint32_t            fsmps : 15;
    /** HcFmInterval.FIT - FrameItervalToggle */
    uint32_t            fit : 1;
    /** HcFmInterval.FI - FrameInterval */
    uint32_t            fi : 14;
    /** HcFmRemaining.FRT - toggle bit. */
    uint32_t            frt : 1;
    /** HcFmNumber.
     * @remark The register size is 16-bit, but for debugging and performance
     *         reasons we maintain a 32-bit counter. */
    uint32_t            HcFmNumber;
    /** HcPeriodicStart */
    uint32_t            pstart;
    /** @} */

    /** The number of virtual time ticks per frame. */
    uint64_t            cTicksPerFrame;
    /** The number of virtual time ticks per USB bus tick. */
    uint64_t            cTicksPerUsbTick;

    /** Number of in-flight TDs. */
    unsigned            cInFlight;
    unsigned            Alignment0;    /**< Align aInFlight on a 8 byte boundary. */
    /** Array of in-flight TDs. */
    struct ohci_td_in_flight
    {
        /** Address of the transport descriptor. */
        uint32_t    GCPhysTD;
        /** Flag indicating an inactive (not-linked) URB. */
        bool        fInactive;
        /** Pointer to the URB. */
        R3PTRTYPE(PVUSBURB) pUrb;
    } aInFlight[257];

#if HC_ARCH_BITS == 32
    uint32_t            Alignment1;
#endif

    /** Number of in-done-queue TDs. */
    unsigned            cInDoneQueue;
    /** Array of in-done-queue TDs. */
    struct ohci_td_in_done_queue
    {
        /** Address of the transport descriptor. */
        uint32_t GCPhysTD;
    } aInDoneQueue[64];
    /** When the tail of the done queue was added.
     * Used to calculate the age of the done queue. */
    uint32_t            u32FmDoneQueueTail;
#if R3_ARCH_BITS == 32
    /** Align pLoad, the stats and the struct size correctly. */
    uint32_t            Alignment2;
#endif
    /** Pointer to state load data. */
    R3PTRTYPE(POHCILOAD) pLoad;

    /** Detected canceled isochronous URBs. */
    STAMCOUNTER         StatCanceledIsocUrbs;
    /** Detected canceled general URBs. */
    STAMCOUNTER         StatCanceledGenUrbs;
    /** Dropped URBs (endpoint halted, or URB canceled). */
    STAMCOUNTER         StatDroppedUrbs;
    /** Profiling ohciFrameBoundaryTimer. */
    STAMPROFILE         StatTimer;

    /** This member and all the following are not part of saved state. */
    uint64_t            SavedStateEnd;

    /** VM timer frequency used for frame timer calculations. */
    uint64_t            u64TimerHz;
    /** Idle detection flag; must be cleared at start of frame */
    bool                fIdle;
    /** A flag indicating that the bulk list may have in-flight URBs. */
    bool                fBulkNeedsCleaning;

    /** Whether RC/R0 is enabled. */
    bool                fRZEnabled;

    uint32_t            Alignment3;     /**< Align size on a 8 byte boundary. */

    /** Critical section synchronising interrupt handling. */
    PDMCRITSECT         CsIrq;
    /** Critical section to synchronize the framer and URB completion handler. */
    RTCRITSECT                 CritSect;
#ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
    /** Last read physical page for caching ED reads in the framer thread. */
    R3PTRTYPE(POHCIPAGECACHE)  pCacheED;
    /** Last read physical page for caching TD reads in the framer thread. */
    R3PTRTYPE(POHCIPAGECACHE)  pCacheTD;
#endif

} OHCI;

/* Standard OHCI bus speed */
#define OHCI_DEFAULT_TIMER_FREQ     1000

/* Host Controller Communications Area */
#define OHCI_HCCA_NUM_INTR  32
#define OHCI_HCCA_OFS       (OHCI_HCCA_NUM_INTR * sizeof(uint32_t))
struct ohci_hcca
{
    uint16_t frame;
    uint16_t pad;
    uint32_t done;
};
AssertCompileSize(ohci_hcca, 8);

/** @name OHCI Endpoint Descriptor
 * @{ */

#define ED_PTR_MASK         (~(uint32_t)0xf)
#define ED_HWINFO_MPS       0x07ff0000
#define ED_HWINFO_ISO       RT_BIT(15)
#define ED_HWINFO_SKIP      RT_BIT(14)
#define ED_HWINFO_LOWSPEED  RT_BIT(13)
#define ED_HWINFO_IN        RT_BIT(12)
#define ED_HWINFO_OUT       RT_BIT(11)
#define ED_HWINFO_DIR       (RT_BIT(11) | RT_BIT(12))
#define ED_HWINFO_ENDPOINT  0x780  /* 4 bits */
#define ED_HWINFO_ENDPOINT_SHIFT 7
#define ED_HWINFO_FUNCTION  0x7f /* 7 bits */
#define ED_HEAD_CARRY       RT_BIT(1)
#define ED_HEAD_HALTED      RT_BIT(0)

/**
 * OHCI Endpoint Descriptor.
 */
typedef struct OHCIED
{
    /** Flags and stuff. */
    uint32_t hwinfo;
    /** TailP - TD Queue Tail pointer. Bits 0-3 ignored / preserved. */
    uint32_t TailP;
    /** HeadP - TD Queue head pointer. Bit 0 - Halted, Bit 1 - toggleCarry. Bit 2&3 - 0. */
    uint32_t HeadP;
    /** NextED - Next Endpoint Descriptor. Bits 0-3 ignored / preserved. */
    uint32_t NextED;
} OHCIED, *POHCIED;
typedef const OHCIED *PCOHCIED;
/** @} */
AssertCompileSize(OHCIED, 16);


/** @name Completion Codes
 * @{ */
#define OHCI_CC_NO_ERROR                (UINT32_C(0x00) << 28)
#define OHCI_CC_CRC                     (UINT32_C(0x01) << 28)
#define OHCI_CC_STALL                   (UINT32_C(0x04) << 28)
#define OHCI_CC_DEVICE_NOT_RESPONDING   (UINT32_C(0x05) << 28)
#define OHCI_CC_DNR                     OHCI_CC_DEVICE_NOT_RESPONDING
#define OHCI_CC_PID_CHECK_FAILURE       (UINT32_C(0x06) << 28)
#define OHCI_CC_UNEXPECTED_PID          (UINT32_C(0x07) << 28)
#define OHCI_CC_DATA_OVERRUN            (UINT32_C(0x08) << 28)
#define OHCI_CC_DATA_UNDERRUN           (UINT32_C(0x09) << 28)
/* 0x0a..0x0b - reserved */
#define OHCI_CC_BUFFER_OVERRUN          (UINT32_C(0x0c) << 28)
#define OHCI_CC_BUFFER_UNDERRUN         (UINT32_C(0x0d) << 28)
#define OHCI_CC_NOT_ACCESSED_0          (UINT32_C(0x0e) << 28)
#define OHCI_CC_NOT_ACCESSED_1          (UINT32_C(0x0f) << 28)
/** @} */


/** @name OHCI General transfer descriptor
 * @{ */

/** Error count (EC) shift. */
#define TD_ERRORS_SHIFT         26
/** Error count max. (One greater than what the EC field can hold.) */
#define TD_ERRORS_MAX           4

/** CC - Condition code mask. */
#define TD_HWINFO_CC            (UINT32_C(0xf0000000))
#define TD_HWINFO_CC_SHIFT      28
/** EC - Error count. */
#define TD_HWINFO_ERRORS        (RT_BIT(26) | RT_BIT(27))
/** T  - Data toggle. */
#define TD_HWINFO_TOGGLE        (RT_BIT(24) | RT_BIT(25))
#define TD_HWINFO_TOGGLE_HI     (RT_BIT(25))
#define TD_HWINFO_TOGGLE_LO     (RT_BIT(24))
/** DI - Delay interrupt. */
#define TD_HWINFO_DI            (RT_BIT(21) | RT_BIT(22) | RT_BIT(23))
#define TD_HWINFO_IN            (RT_BIT(20))
#define TD_HWINFO_OUT           (RT_BIT(19))
/** DP - Direction / PID. */
#define TD_HWINFO_DIR           (RT_BIT(19) | RT_BIT(20))
/** R  - Buffer rounding. */
#define TD_HWINFO_ROUNDING      (RT_BIT(18))
/** Bits that are reserved / unknown. */
#define TD_HWINFO_UNKNOWN_MASK  (UINT32_C(0x0003ffff))

/** SETUP - to endpoint. */
#define OHCI_TD_DIR_SETUP       0x0
/** OUT - to endpoint. */
#define OHCI_TD_DIR_OUT         0x1
/** IN - from endpoint. */
#define OHCI_TD_DIR_IN          0x2
/** Reserved. */
#define OHCI_TD_DIR_RESERVED    0x3

/**
 * OHCI general transfer descriptor
 */
typedef struct OHCITD
{
    uint32_t hwinfo;
    /** CBP - Current Buffer Pointer. (32-bit physical address) */
    uint32_t cbp;
    /** NextTD - Link to the next transfer descriptor. (32-bit physical address, dword aligned) */
    uint32_t NextTD;
    /** BE - Buffer End (inclusive). (32-bit physical address) */
    uint32_t be;
} OHCITD, *POHCITD;
typedef const OHCITD *PCOHCITD;
/** @} */
AssertCompileSize(OHCIED, 16);


/** @name OHCI isochronous transfer descriptor.
 * @{ */
/** SF - Start frame number. */
#define ITD_HWINFO_SF       0xffff
/** DI - Delay interrupt. (TD_HWINFO_DI) */
#define ITD_HWINFO_DI       (RT_BIT(21) | RT_BIT(22) | RT_BIT(23))
#define ITD_HWINFO_DI_SHIFT 21
/** FC - Frame count. */
#define ITD_HWINFO_FC       (RT_BIT(24) | RT_BIT(25) | RT_BIT(26))
#define ITD_HWINFO_FC_SHIFT 24
/** CC - Condition code mask. (=TD_HWINFO_CC)  */
#define ITD_HWINFO_CC       UINT32_C(0xf0000000)
#define ITD_HWINFO_CC_SHIFT 28
/** The buffer page 0 mask (lower 12 bits are ignored). */
#define ITD_BP0_MASK        UINT32_C(0xfffff000)

#define ITD_NUM_PSW 8
/** OFFSET - offset of the package into the buffer page.
 * (Only valid when CC set to Not Accessed.)
 *
 * Note that the top bit of the OFFSET field is overlapping with the
 * first bit in the CC field. This is ok because both 0xf and 0xe are
 * defined as "Not Accessed".
 */
#define ITD_PSW_OFFSET      0x1fff
/** SIZE field mask for IN bound transfers.
 * (Only valid when CC isn't Not Accessed.)*/
#define ITD_PSW_SIZE        0x07ff
/** CC field mask.
 * USed to indicate the format of SIZE (Not Accessed -> OFFSET). */
#define ITD_PSW_CC          0xf000
#define ITD_PSW_CC_SHIFT    12

/**
 * OHCI isochronous transfer descriptor.
 */
typedef struct OHCIITD
{
    uint32_t HwInfo;
    /** BP0 - Buffer Page 0. The lower 12 bits are ignored. */
    uint32_t BP0;
    /** NextTD - Link to the next transfer descriptor. (32-bit physical address, dword aligned) */
    uint32_t NextTD;
    /** BE - Buffer End (inclusive). (32-bit physical address) */
    uint32_t BE;
    /** (OffsetN/)PSWN - package status word array (0..7).
     * The format varies depending on whether the package has been completed or not. */
    uint16_t aPSW[ITD_NUM_PSW];
} OHCIITD, *POHCIITD;
typedef const OHCIITD *PCOHCIITD;
/** @} */
AssertCompileSize(OHCIITD, 32);

/**
 * OHCI register operator.
 */
typedef struct ohci_opreg
{
    const char *pszName;
    int (*pfnRead )(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value);
    int (*pfnWrite)(POHCI pThis, uint32_t iReg, uint32_t u32Value);
} OHCIOPREG;


/* OHCI Local stuff */
#define OHCI_CTL_CBSR       ((1<<0)|(1<<1))
#define OHCI_CTL_PLE        (1<<2)
#define OHCI_CTL_IE         (1<<3)
#define OHCI_CTL_CLE        (1<<4)
#define OHCI_CTL_BLE        (1<<5)
#define OHCI_CTL_HCFS       ((1<<6)|(1<<7))
#define  OHCI_USB_RESET         0x00
#define  OHCI_USB_RESUME        0x40
#define  OHCI_USB_OPERATIONAL   0x80
#define  OHCI_USB_SUSPEND       0xc0
#define OHCI_CTL_IR         (1<<8)
#define OHCI_CTL_RWC        (1<<9)
#define OHCI_CTL_RWE        (1<<10)

#define OHCI_STATUS_HCR     (1<<0)
#define OHCI_STATUS_CLF     (1<<1)
#define OHCI_STATUS_BLF     (1<<2)
#define OHCI_STATUS_OCR     (1<<3)
#define OHCI_STATUS_SOC     ((1<<6)|(1<<7))

/** @name Interrupt Status and Enabled/Disabled Flags
 * @{ */
/** SO  - Scheduling overrun. */
#define OHCI_INTR_SCHEDULEING_OVERRUN       RT_BIT(0)
/** WDH - HcDoneHead writeback. */
#define OHCI_INTR_WRITE_DONE_HEAD           RT_BIT(1)
/** SF  - Start of frame. */
#define OHCI_INTR_START_OF_FRAME            RT_BIT(2)
/** RD  - Resume detect. */
#define OHCI_INTR_RESUME_DETECT             RT_BIT(3)
/** UE  - Unrecoverable error. */
#define OHCI_INTR_UNRECOVERABLE_ERROR       RT_BIT(4)
/** FNO - Frame number overflow. */
#define OHCI_INTR_FRAMENUMBER_OVERFLOW      RT_BIT(5)
/** RHSC- Root hub status change. */
#define OHCI_INTR_ROOT_HUB_STATUS_CHANGE    RT_BIT(6)
/** OC  - Ownership change. */
#define OHCI_INTR_OWNERSHIP_CHANGE          RT_BIT(30)
/** MIE - Master interrupt enable. */
#define OHCI_INTR_MASTER_INTERRUPT_ENABLED  RT_BIT(31)
/** @} */

#define OHCI_HCCA_SIZE      0x100
#define OHCI_HCCA_MASK      UINT32_C(0xffffff00)

#define OHCI_FMI_FI         UINT32_C(0x00003fff)
#define OHCI_FMI_FSMPS      UINT32_C(0x7fff0000)
#define OHCI_FMI_FSMPS_SHIFT 16
#define OHCI_FMI_FIT        UINT32_C(0x80000000)
#define OHCI_FMI_FIT_SHIFT  31

#define OHCI_FR_RT          RT_BIT_32(31)

#define OHCI_LS_THRESH      0x628

#define OHCI_RHA_NDP        (0xff)
#define OHCI_RHA_PSM        RT_BIT_32(8)
#define OHCI_RHA_NPS        RT_BIT_32(9)
#define OHCI_RHA_DT         RT_BIT_32(10)
#define OHCI_RHA_OCPM       RT_BIT_32(11)
#define OHCI_RHA_NOCP       RT_BIT_32(12)
#define OHCI_RHA_POTPGP     UINT32_C(0xff000000)

#define OHCI_RHS_LPS        RT_BIT_32(0)
#define OHCI_RHS_OCI        RT_BIT_32(1)
#define OHCI_RHS_DRWE       RT_BIT_32(15)
#define OHCI_RHS_LPSC       RT_BIT_32(16)
#define OHCI_RHS_OCIC       RT_BIT_32(17)
#define OHCI_RHS_CRWE       RT_BIT_32(31)

/** @name HcRhPortStatus[n] - RH Port Status register (read).
 * @{ */
/** CCS - CurrentConnectionStatus - 0 = no device, 1 = device. */
#define OHCI_PORT_CCS       RT_BIT(0)
/** PES - PortEnableStatus. */
#define OHCI_PORT_PES       RT_BIT(1)
/** PSS - PortSuspendStatus */
#define OHCI_PORT_PSS       RT_BIT(2)
/** POCI- PortOverCurrentIndicator. */
#define OHCI_PORT_POCI      RT_BIT(3)
/** PRS - PortResetStatus */
#define OHCI_PORT_PRS       RT_BIT(4)
/** PPS - PortPowerStatus */
#define OHCI_PORT_PPS       RT_BIT(8)
/** LSDA - LowSpeedDeviceAttached */
#define OHCI_PORT_LSDA      RT_BIT(9)
/** CSC  - ConnectStatusChange */
#define OHCI_PORT_CSC       RT_BIT(16)
/** PESC - PortEnableStatusChange */
#define OHCI_PORT_PESC      RT_BIT(17)
/** PSSC - PortSuspendStatusChange */
#define OHCI_PORT_PSSC      RT_BIT(18)
/** OCIC - OverCurrentIndicatorChange */
#define OHCI_PORT_OCIC      RT_BIT(19)
/** PRSC - PortResetStatusChange */
#define OHCI_PORT_PRSC      RT_BIT(20)
/** @} */


/** @name HcRhPortStatus[n] - Root Hub Port Status Registers - read.
 * @{ */
/** CCS - CurrentConnectStatus - 0 = no device, 1 = device. */
#define OHCI_PORT_R_CURRENT_CONNECT_STATUS      RT_BIT(0)
/** PES - PortEnableStatus. */
#define OHCI_PORT_R_ENABLE_STATUS               RT_BIT(1)
/** PSS - PortSuspendStatus */
#define OHCI_PORT_R_SUSPEND_STATUS              RT_BIT(2)
/** POCI- PortOverCurrentIndicator. */
#define OHCI_PORT_R_OVER_CURRENT_INDICATOR      RT_BIT(3)
/** PRS - PortResetStatus */
#define OHCI_PORT_R_RESET_STATUS                RT_BIT(4)
/** PPS - PortPowerStatus */
#define OHCI_PORT_R_POWER_STATUS                RT_BIT(8)
/** LSDA - LowSpeedDeviceAttached */
#define OHCI_PORT_R_LOW_SPEED_DEVICE_ATTACHED   RT_BIT(9)
/** CSC  - ConnectStatusChange */
#define OHCI_PORT_R_CONNECT_STATUS_CHANGE       RT_BIT(16)
/** PESC - PortEnableStatusChange */
#define OHCI_PORT_R_ENABLE_STATUS_CHANGE        RT_BIT(17)
/** PSSC - PortSuspendStatusChange */
#define OHCI_PORT_R_SUSPEND_STATUS_CHANGE       RT_BIT(18)
/** OCIC - OverCurrentIndicatorChange */
#define OHCI_PORT_R_OVER_CURRENT_INDICATOR_CHANGE   RT_BIT(19)
/** PRSC - PortResetStatusChange */
#define OHCI_PORT_R_RESET_STATUS_CHANGE         RT_BIT(20)
/** @} */

/** @name HcRhPortStatus[n] - Root Hub Port Status Registers - write.
 * @{ */
/** CCS - ClearPortEnable. */
#define OHCI_PORT_W_CLEAR_ENABLE                RT_BIT(0)
/** PES - SetPortEnable. */
#define OHCI_PORT_W_SET_ENABLE                  RT_BIT(1)
/** PSS - SetPortSuspend */
#define OHCI_PORT_W_SET_SUSPEND                 RT_BIT(2)
/** POCI- ClearSuspendStatus. */
#define OHCI_PORT_W_CLEAR_SUSPEND_STATUS        RT_BIT(3)
/** PRS - SetPortReset */
#define OHCI_PORT_W_SET_RESET                   RT_BIT(4)
/** PPS - SetPortPower */
#define OHCI_PORT_W_SET_POWER                   RT_BIT(8)
/** LSDA - ClearPortPower */
#define OHCI_PORT_W_CLEAR_POWER                 RT_BIT(9)
/** CSC  - ClearConnectStatusChange */
#define OHCI_PORT_W_CLEAR_CSC                   RT_BIT(16)
/** PESC - PortEnableStatusChange */
#define OHCI_PORT_W_CLEAR_PESC                  RT_BIT(17)
/** PSSC - PortSuspendStatusChange */
#define OHCI_PORT_W_CLEAR_PSSC                  RT_BIT(18)
/** OCIC - OverCurrentIndicatorChange */
#define OHCI_PORT_W_CLEAR_OCIC                  RT_BIT(19)
/** PRSC - PortResetStatusChange */
#define OHCI_PORT_W_CLEAR_PRSC                  RT_BIT(20)
/** The mask of bit which are used to clear themselves. */
#define OHCI_PORT_W_CLEAR_CHANGE_MASK           (  OHCI_PORT_W_CLEAR_CSC  | OHCI_PORT_W_CLEAR_PESC | OHCI_PORT_W_CLEAR_PSSC \
                                                 | OHCI_PORT_W_CLEAR_OCIC | OHCI_PORT_W_CLEAR_PRSC)
/** @} */


#ifndef VBOX_DEVICE_STRUCT_TESTCASE

#ifdef VBOX_WITH_OHCI_PHYS_READ_STATS
/*
 * Explain
 */
typedef struct OHCIDESCREADSTATS
{
    uint32_t cReads;
    uint32_t cPageChange;
    uint32_t cMinReadsPerPage;
    uint32_t cMaxReadsPerPage;

    uint32_t cReadsLastPage;
    uint32_t u32LastPageAddr;
} OHCIDESCREADSTATS;
typedef OHCIDESCREADSTATS *POHCIDESCREADSTATS;

typedef struct OHCIPHYSREADSTATS
{
    OHCIDESCREADSTATS ed;
    OHCIDESCREADSTATS td;
    OHCIDESCREADSTATS all;

    uint32_t cCrossReads;
    uint32_t cCacheReads;
    uint32_t cPageReads;
} OHCIPHYSREADSTATS;
typedef OHCIPHYSREADSTATS *POHCIPHYSREADSTATS;
typedef OHCIPHYSREADSTATS const *PCOHCIPHYSREADSTATS;
#endif /* VBOX_WITH_OHCI_PHYS_READ_STATS */


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#if defined(VBOX_WITH_OHCI_PHYS_READ_STATS) && defined(IN_RING3)
static OHCIPHYSREADSTATS g_PhysReadState;
#endif

#if defined(LOG_ENABLED) && defined(IN_RING3)
static bool g_fLogBulkEPs = false;
static bool g_fLogControlEPs = false;
static bool g_fLogInterruptEPs = false;
#endif
#ifdef IN_RING3
/**
 * SSM descriptor table for the OHCI structure.
 */
static SSMFIELD const g_aOhciFields[] =
{
    SSMFIELD_ENTRY(         OHCI, SofTime),
    SSMFIELD_ENTRY_CUSTOM(        dpic+fno, RT_OFFSETOF(OHCI, SofTime) + RT_SIZEOFMEMB(OHCI, SofTime), 4),
    SSMFIELD_ENTRY(         OHCI, RootHub.status),
    SSMFIELD_ENTRY(         OHCI, RootHub.desc_a),
    SSMFIELD_ENTRY(         OHCI, RootHub.desc_b),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[0].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[1].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[2].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[3].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[4].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[5].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[6].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[7].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[8].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[9].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[10].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[11].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[12].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[13].fReg),
    SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[14].fReg),
    SSMFIELD_ENTRY(         OHCI, ctl),
    SSMFIELD_ENTRY(         OHCI, status),
    SSMFIELD_ENTRY(         OHCI, intr_status),
    SSMFIELD_ENTRY(         OHCI, intr),
    SSMFIELD_ENTRY(         OHCI, hcca),
    SSMFIELD_ENTRY(         OHCI, per_cur),
    SSMFIELD_ENTRY(         OHCI, ctrl_cur),
    SSMFIELD_ENTRY(         OHCI, ctrl_head),
    SSMFIELD_ENTRY(         OHCI, bulk_cur),
    SSMFIELD_ENTRY(         OHCI, bulk_head),
    SSMFIELD_ENTRY(         OHCI, done),
    SSMFIELD_ENTRY_CUSTOM(        fsmps+fit+fi+frt, RT_OFFSETOF(OHCI, done) + RT_SIZEOFMEMB(OHCI, done), 4),
    SSMFIELD_ENTRY(         OHCI, HcFmNumber),
    SSMFIELD_ENTRY(         OHCI, pstart),
    SSMFIELD_ENTRY_TERM()
};
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
#ifdef IN_RING3
/* Update host controller state to reflect a device attach */
static void                 rhport_power(POHCIROOTHUB pRh, unsigned iPort, bool fPowerUp);
static void                 ohciBusResume(POHCI ohci, bool fHardware);
static void                 ohciBusStop(POHCI pThis);
#ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
static void                 ohciPhysReadCacheClear(POHCIPAGECACHE pPageCache);
#endif

static DECLCALLBACK(void)   ohciRhXferCompletion(PVUSBIROOTHUBPORT pInterface, PVUSBURB pUrb);
static DECLCALLBACK(bool)   ohciRhXferError(PVUSBIROOTHUBPORT pInterface, PVUSBURB pUrb);

static int                  ohci_in_flight_find(POHCI pThis, uint32_t GCPhysTD);
# if defined(VBOX_STRICT) || defined(LOG_ENABLED)
static int                  ohci_in_done_queue_find(POHCI pThis, uint32_t GCPhysTD);
# endif
static DECLCALLBACK(void)   ohciR3LoadReattachDevices(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser);
#endif /* IN_RING3 */
RT_C_DECLS_END


/**
 * Update PCI IRQ levels
 */
static void ohciUpdateInterruptLocked(POHCI ohci, const char *msg)
{
    int level = 0;

    if (    (ohci->intr & OHCI_INTR_MASTER_INTERRUPT_ENABLED)
        &&  (ohci->intr_status & ohci->intr)
        && !(ohci->ctl & OHCI_CTL_IR))
        level = 1;

    PDMDevHlpPCISetIrq(ohci->CTX_SUFF(pDevIns), 0, level);
    if (level)
    {
        uint32_t val = ohci->intr_status & ohci->intr;
        Log2(("ohci: Fired off interrupt %#010x - SO=%d WDH=%d SF=%d RD=%d UE=%d FNO=%d RHSC=%d OC=%d - %s\n",
              val, val & 1, (val >> 1) & 1, (val >> 2) & 1, (val >> 3) & 1, (val >> 4) & 1, (val >> 5) & 1,
              (val >> 6) & 1, (val >> 30) & 1, msg)); NOREF(val); NOREF(msg);
    }
}

#ifdef IN_RING3

/**
 * Set an interrupt, use the wrapper ohciSetInterrupt.
 */
DECLINLINE(int) ohciSetInterruptInt(POHCI ohci, int rcBusy, uint32_t intr, const char *msg)
{
    int rc = PDMCritSectEnter(&ohci->CsIrq, rcBusy);
    if (rc != VINF_SUCCESS)
        return rc;

    if ( (ohci->intr_status & intr) != intr )
    {
        ohci->intr_status |= intr;
        ohciUpdateInterruptLocked(ohci, msg);
    }

    PDMCritSectLeave(&ohci->CsIrq);
    return rc;
}

/**
 * Set an interrupt wrapper macro for logging purposes.
 */
# define ohciR3SetInterrupt(ohci, intr) ohciSetInterruptInt(ohci, VERR_IGNORED, intr, #intr)


/* Carry out a hardware remote wakeup */
static void ohci_remote_wakeup(POHCI pThis)
{
    if ((pThis->ctl & OHCI_CTL_HCFS) != OHCI_USB_SUSPEND)
        return;
    if (!(pThis->RootHub.status & OHCI_RHS_DRWE))
        return;
    ohciBusResume(pThis, true /* hardware */);
}


/**
 * Query interface method for the roothub LUN.
 */
static DECLCALLBACK(void *) ohciRhQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    POHCI pThis = RT_FROM_MEMBER(pInterface, OHCI, RootHub.IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->RootHub.IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, VUSBIROOTHUBPORT, &pThis->RootHub.IRhPort);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMILEDPORTS, &pThis->RootHub.ILeds);
    return NULL;
}

/**
 * Gets the pointer to the status LED of a unit.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 * @param   iLUN            The unit which status LED we desire.
 * @param   ppLed           Where to store the LED pointer.
 */
static DECLCALLBACK(int) ohciRhQueryStatusLed(PPDMILEDPORTS pInterface, unsigned iLUN, PPDMLED *ppLed)
{
    POHCI pThis = (POHCI)((uintptr_t)pInterface - RT_OFFSETOF(OHCI, RootHub.ILeds));
    if (iLUN == 0)
    {
        *ppLed = &pThis->RootHub.Led;
        return VINF_SUCCESS;
    }
    return VERR_PDM_LUN_NOT_FOUND;
}


/** Converts a OHCI.roothub.IRhPort pointer to a POHCI. */
#define VUSBIROOTHUBPORT_2_OHCI(pInterface) ((POHCI)( (uintptr_t)(pInterface) - RT_OFFSETOF(OHCI, RootHub.IRhPort) ))


/**
 * Get the number of available ports in the hub.
 *
 * @returns The number of ports available.
 * @param   pInterface      Pointer to this structure.
 * @param   pAvailable      Bitmap indicating the available ports. Set bit == available port.
 */
static DECLCALLBACK(unsigned) ohciRhGetAvailablePorts(PVUSBIROOTHUBPORT pInterface, PVUSBPORTBITMAP pAvailable)
{
    POHCI pThis = VUSBIROOTHUBPORT_2_OHCI(pInterface);
    unsigned iPort;
    unsigned cPorts = 0;

    memset(pAvailable, 0, sizeof(*pAvailable));

    PDMCritSectEnter(pThis->pDevInsR3->pCritSectRoR3, VERR_IGNORED);
    for (iPort = 0; iPort < OHCI_NDP_CFG(pThis); iPort++)
    {
        if (!pThis->RootHub.aPorts[iPort].pDev)
        {
            cPorts++;
            ASMBitSet(pAvailable, iPort + 1);
        }
    }
    PDMCritSectLeave(pThis->pDevInsR3->pCritSectRoR3);

    return cPorts;
}


/**
 * Gets the supported USB versions.
 *
 * @returns The mask of supported USB versions.
 * @param   pInterface      Pointer to this structure.
 */
static DECLCALLBACK(uint32_t) ohciRhGetUSBVersions(PVUSBIROOTHUBPORT pInterface)
{
    RT_NOREF(pInterface);
    return VUSB_STDVER_11;
}


/**
 * A device is being attached to a port in the roothub.
 *
 * @param   pInterface      Pointer to this structure.
 * @param   pDev            Pointer to the device being attached.
 * @param   uPort           The port number assigned to the device.
 */
static DECLCALLBACK(int) ohciRhAttach(PVUSBIROOTHUBPORT pInterface, PVUSBIDEVICE pDev, unsigned uPort)
{
    POHCI pThis = VUSBIROOTHUBPORT_2_OHCI(pInterface);
    LogFlow(("ohciRhAttach: pDev=%p uPort=%u\n", pDev, uPort));
    PDMCritSectEnter(pThis->pDevInsR3->pCritSectRoR3, VERR_IGNORED);

    /*
     * Validate and adjust input.
     */
    Assert(uPort >= 1 && uPort <= OHCI_NDP_CFG(pThis));
    uPort--;
    Assert(!pThis->RootHub.aPorts[uPort].pDev);
    /* Only LS/FS devices can end up here. */
    Assert(pDev->pfnGetSpeed(pDev) == VUSB_SPEED_LOW || pDev->pfnGetSpeed(pDev) == VUSB_SPEED_FULL);

    /*
     * Attach it.
     */
    pThis->RootHub.aPorts[uPort].fReg = OHCI_PORT_R_CURRENT_CONNECT_STATUS | OHCI_PORT_R_CONNECT_STATUS_CHANGE;
    pThis->RootHub.aPorts[uPort].pDev = pDev;
    rhport_power(&pThis->RootHub, uPort, 1 /* power on */);

    ohci_remote_wakeup(pThis);
    ohciR3SetInterrupt(pThis, OHCI_INTR_ROOT_HUB_STATUS_CHANGE);

    PDMCritSectLeave(pThis->pDevInsR3->pCritSectRoR3);
    return VINF_SUCCESS;
}


/**
 * A device is being detached from a port in the roothub.
 *
 * @param   pInterface      Pointer to this structure.
 * @param   pDev            Pointer to the device being detached.
 * @param   uPort           The port number assigned to the device.
 */
static DECLCALLBACK(void) ohciRhDetach(PVUSBIROOTHUBPORT pInterface, PVUSBIDEVICE pDev, unsigned uPort)
{
    RT_NOREF(pDev);
    POHCI pThis = VUSBIROOTHUBPORT_2_OHCI(pInterface);
    LogFlow(("ohciRhDetach: pDev=%p uPort=%u\n", pDev, uPort));
    PDMCritSectEnter(pThis->pDevInsR3->pCritSectRoR3, VERR_IGNORED);

    /*
     * Validate and adjust input.
     */
    Assert(uPort >= 1 && uPort <= OHCI_NDP_CFG(pThis));
    uPort--;
    Assert(pThis->RootHub.aPorts[uPort].pDev == pDev);

    /*
     * Detach it.
     */
    pThis->RootHub.aPorts[uPort].pDev = NULL;
    if (pThis->RootHub.aPorts[uPort].fReg & OHCI_PORT_PES)
        pThis->RootHub.aPorts[uPort].fReg = OHCI_PORT_R_CONNECT_STATUS_CHANGE | OHCI_PORT_PESC;
    else
        pThis->RootHub.aPorts[uPort].fReg = OHCI_PORT_R_CONNECT_STATUS_CHANGE;

    ohci_remote_wakeup(pThis);
    ohciR3SetInterrupt(pThis, OHCI_INTR_ROOT_HUB_STATUS_CHANGE);

    PDMCritSectLeave(pThis->pDevInsR3->pCritSectRoR3);
}


#ifdef IN_RING3
/**
 * One of the roothub devices has completed its reset operation.
 *
 * Currently, we don't think anything is required to be done here
 * so it's just a stub for forcing async resetting of the devices
 * during a root hub reset.
 *
 * @param pDev      The root hub device.
 * @param rc        The result of the operation.
 * @param pvUser    Pointer to the controller.
 */
static DECLCALLBACK(void) ohciRhResetDoneOneDev(PVUSBIDEVICE pDev, int rc, void *pvUser)
{
    LogRel(("OHCI: root hub reset completed with %Rrc\n", rc));
    NOREF(pDev); NOREF(rc); NOREF(pvUser);
}
#endif


/**
 * Reset the root hub.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to this structure.
 * @param   fResetOnLinux   This is used to indicate whether we're at VM reset time and
 *                          can do real resets or if we're at any other time where that
 *                          isn't such a good idea.
 * @remark  Do NOT call VUSBIDevReset on the root hub in an async fashion!
 * @thread  EMT
 */
static DECLCALLBACK(int) ohciRhReset(PVUSBIROOTHUBPORT pInterface, bool fResetOnLinux)
{
    POHCI pThis = VUSBIROOTHUBPORT_2_OHCI(pInterface);
    PDMCritSectEnter(pThis->pDevInsR3->pCritSectRoR3, VERR_IGNORED);

    Log(("ohci: root hub reset%s\n", fResetOnLinux ? " (reset on linux)" : ""));

    pThis->RootHub.status = 0;
    pThis->RootHub.desc_a = OHCI_RHA_NPS | OHCI_NDP_CFG(pThis); /* Preserve NDP value. */
    pThis->RootHub.desc_b = 0x0; /* Impl. specific */

    /*
     * We're pending to _reattach_ the device without resetting them.
     * Except, during VM reset where we use the opportunity to do a proper
     * reset before the guest comes along and expect things.
     *
     * However, it's very very likely that we're not doing the right thing
     * here if coming from the guest (USB Reset state). The docs talks about
     * root hub resetting, however what exact behaviour in terms of root hub
     * status and changed bits, and HC interrupts aren't stated clearly. IF we
     * get trouble and see the guest doing "USB Resets" we will have to look
     * into this. For the time being we stick with simple.
     */
    for (unsigned iPort = 0; iPort < OHCI_NDP_CFG(pThis); iPort++)
    {
        if (pThis->RootHub.aPorts[iPort].pDev)
        {
            pThis->RootHub.aPorts[iPort].fReg = OHCI_PORT_R_CURRENT_CONNECT_STATUS | OHCI_PORT_R_CONNECT_STATUS_CHANGE | OHCI_PORT_PPS;
            if (fResetOnLinux)
            {
                PVM pVM = PDMDevHlpGetVM(pThis->CTX_SUFF(pDevIns));
                VUSBIDevReset(pThis->RootHub.aPorts[iPort].pDev, fResetOnLinux, ohciRhResetDoneOneDev, pThis, pVM);
            }
        }
        else
            pThis->RootHub.aPorts[iPort].fReg = 0;
    }
    ohciR3SetInterrupt(pThis, OHCI_INTR_ROOT_HUB_STATUS_CHANGE);

    PDMCritSectLeave(pThis->pDevInsR3->pCritSectRoR3);
    return VINF_SUCCESS;
}


/**
 * Does a software or hardware reset of the controller.
 *
 * This is called in response to setting HcCommandStatus.HCR, hardware reset,
 * and device construction.
 *
 * @param   pThis           The ohci instance data.
 * @param   fNewMode        The new mode of operation. This is UsbSuspend if it's a
 *                          software reset, and UsbReset if it's a hardware reset / cold boot.
 * @param   fResetOnLinux   Set if we can do a real reset of the devices attached to the root hub.
 *                          This is really a just a hack for the non-working linux device reset.
 *                          Linux has this feature called 'logical disconnect' if device reset fails
 *                          which prevents us from doing resets when the guest asks for it - the guest
 *                          will get confused when the device seems to be reconnected everytime it tries
 *                          to reset it. But if we're at hardware reset time, we can allow a device to
 *                          be 'reconnected' without upsetting the guest.
 *
 * @remark  This hasn't got anything to do with software setting the mode to UsbReset.
 */
static void ohciDoReset(POHCI pThis, uint32_t fNewMode, bool fResetOnLinux)
{
    Log(("ohci: %s reset%s\n", fNewMode == OHCI_USB_RESET ? "hardware" : "software",
         fResetOnLinux ? " (reset on linux)" : ""));

    /* Stop the bus in any case, disabling walking the lists. */
    ohciBusStop(pThis);

    /*
     * Cancel all outstanding URBs.
     *
     * We can't, and won't, deal with URBs until we're moved out of the
     * suspend/reset state. Also, a real HC isn't going to send anything
     * any more when a reset has been signaled.
     */
    pThis->RootHub.pIRhConn->pfnCancelAllUrbs(pThis->RootHub.pIRhConn);

    /*
     * Reset the hardware registers.
     */
    if (fNewMode == OHCI_USB_RESET)
        pThis->ctl  = OHCI_CTL_RWC;                     /* We're the firmware, set RemoteWakeupConnected. */
    else
        pThis->ctl &= OHCI_CTL_IR | OHCI_CTL_RWC;       /* IR and RWC are preserved on software reset. */

    /* Clear the HCFS bits first to make setting the new state work. */
    pThis->ctl &= ~OHCI_CTL_HCFS;
    pThis->ctl |= fNewMode;
    pThis->status = 0;
    pThis->intr_status = 0;
    pThis->intr = 0;
    PDMDevHlpPCISetIrq(pThis->CTX_SUFF(pDevIns), 0, 0);

    pThis->hcca = 0;
    pThis->per_cur = 0;
    pThis->ctrl_head = pThis->ctrl_cur = 0;
    pThis->bulk_head = pThis->bulk_cur = 0;
    pThis->done = 0;

    pThis->fsmps = 0x2778;                              /* To-Be-Defined, use the value linux sets...*/
    pThis->fit = 0;
    pThis->fi = 11999;                                  /* (12MHz ticks, one frame is 1ms) */
    pThis->frt = 0;
    pThis->HcFmNumber = 0;
    pThis->pstart = 0;

    pThis->dqic = 0x7;
    pThis->fno = 0;

#ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
    ohciPhysReadCacheClear(pThis->pCacheED);
    ohciPhysReadCacheClear(pThis->pCacheTD);
#endif

    /*
     * If this is a hardware reset, we will initialize the root hub too.
     * Software resets doesn't do this according to the specs.
     * (It's not possible to have device connected at the time of the
     * device construction, so nothing to worry about there.)
     */
    if (fNewMode == OHCI_USB_RESET)
        VUSBIDevReset(pThis->RootHub.pIDev, fResetOnLinux, NULL, NULL, NULL);
}

/**
 * Reads physical memory.
 */
DECLINLINE(void) ohciPhysRead(POHCI pThis, uint32_t Addr, void *pvBuf, size_t cbBuf)
{
    if (cbBuf)
        PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), Addr, pvBuf, cbBuf);
}

/**
 * Writes physical memory.
 */
DECLINLINE(void) ohciPhysWrite(POHCI pThis, uint32_t Addr, const void *pvBuf, size_t cbBuf)
{
    if (cbBuf)
        PDMDevHlpPCIPhysWrite(pThis->CTX_SUFF(pDevIns), Addr, pvBuf, cbBuf);
}

/**
 * Read an array of dwords from physical memory and correct endianness.
 */
DECLINLINE(void) ohciGetDWords(POHCI pThis, uint32_t Addr, uint32_t *pau32s, int c32s)
{
    ohciPhysRead(pThis, Addr, pau32s, c32s * sizeof(uint32_t));
# ifndef RT_LITTLE_ENDIAN
    for(int i = 0; i < c32s; i++)
        pau32s[i] = RT_H2LE_U32(pau32s[i]);
# endif
}

/**
 * Write an array of dwords from physical memory and correct endianness.
 */
DECLINLINE(void) ohciPutDWords(POHCI pThis, uint32_t Addr, const uint32_t *pau32s, int cu32s)
{
# ifdef RT_LITTLE_ENDIAN
    ohciPhysWrite(pThis, Addr, pau32s, cu32s << 2);
# else
    for (int i = 0; i < c32s; i++, pau32s++, Addr += sizeof(*pau32s))
    {
        uint32_t u32Tmp = RT_H2LE_U32(*pau32s);
        ohciPhysWrite(pThis, Addr, (uint8_t *)&u32Tmp, sizeof(u32Tmp));
    }
# endif
}



# ifdef VBOX_WITH_OHCI_PHYS_READ_STATS

static void descReadStatsReset(POHCIDESCREADSTATS p)
{
    p->cReads = 0;
    p->cPageChange = 0;
    p->cMinReadsPerPage = UINT32_MAX;
    p->cMaxReadsPerPage = 0;

    p->cReadsLastPage = 0;
    p->u32LastPageAddr = 0;
}

static void physReadStatsReset(POHCIPHYSREADSTATS p)
{
    descReadStatsReset(&p->ed);
    descReadStatsReset(&p->td);
    descReadStatsReset(&p->all);

    p->cCrossReads = 0;
    p->cCacheReads = 0;
    p->cPageReads = 0;
}

static void physReadStatsUpdateDesc(POHCIDESCREADSTATS p, uint32_t u32Addr)
{
    const uint32_t u32PageAddr = u32Addr & ~UINT32_C(0xFFF);

    ++p->cReads;

    if (p->u32LastPageAddr == 0)
    {
       /* First call. */
       ++p->cReadsLastPage;
       p->u32LastPageAddr = u32PageAddr;
    }
    else if (u32PageAddr != p->u32LastPageAddr)
    {
       /* New page. */
       ++p->cPageChange;

       p->cMinReadsPerPage = RT_MIN(p->cMinReadsPerPage, p->cReadsLastPage);
       p->cMaxReadsPerPage = RT_MAX(p->cMaxReadsPerPage, p->cReadsLastPage);;

       p->cReadsLastPage = 1;
       p->u32LastPageAddr = u32PageAddr;
    }
    else
    {
        /* Read on the same page. */
       ++p->cReadsLastPage;
    }
}

static void physReadStatsPrint(POHCIPHYSREADSTATS p)
{
    p->ed.cMinReadsPerPage = RT_MIN(p->ed.cMinReadsPerPage, p->ed.cReadsLastPage);
    p->ed.cMaxReadsPerPage = RT_MAX(p->ed.cMaxReadsPerPage, p->ed.cReadsLastPage);;

    p->td.cMinReadsPerPage = RT_MIN(p->td.cMinReadsPerPage, p->td.cReadsLastPage);
    p->td.cMaxReadsPerPage = RT_MAX(p->td.cMaxReadsPerPage, p->td.cReadsLastPage);;

    p->all.cMinReadsPerPage = RT_MIN(p->all.cMinReadsPerPage, p->all.cReadsLastPage);
    p->all.cMaxReadsPerPage = RT_MAX(p->all.cMaxReadsPerPage, p->all.cReadsLastPage);;

    LogRel(("PHYSREAD:\n"
            "  ED: %d, %d, %d/%d\n"
            "  TD: %d, %d, %d/%d\n"
            " ALL: %d, %d, %d/%d\n"
            "   C: %d, %d, %d\n"
            "",
            p->ed.cReads, p->ed.cPageChange, p->ed.cMinReadsPerPage, p->ed.cMaxReadsPerPage,
            p->td.cReads, p->td.cPageChange, p->td.cMinReadsPerPage, p->td.cMaxReadsPerPage,
            p->all.cReads, p->all.cPageChange, p->all.cMinReadsPerPage, p->all.cMaxReadsPerPage,
            p->cCrossReads, p->cCacheReads, p->cPageReads
          ));

    physReadStatsReset(p);
}

# endif /* VBOX_WITH_OHCI_PHYS_READ_STATS */
# ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE

static POHCIPAGECACHE ohciPhysReadCacheAlloc(void)
{
    return (POHCIPAGECACHE)RTMemAlloc(sizeof(OHCIPAGECACHE));
}

static void ohciPhysReadCacheFree(POHCIPAGECACHE pPageCache)
{
    RTMemFree(pPageCache);
}

static void ohciPhysReadCacheClear(POHCIPAGECACHE pPageCache)
{
    pPageCache->GCPhysReadCacheAddr = NIL_RTGCPHYS;
}

static void ohciPhysReadCacheRead(POHCI pThis, POHCIPAGECACHE pPageCache, RTGCPHYS GCPhys, void *pvBuf, size_t cbBuf)
{
    const RTGCPHYS PageAddr = PAGE_ADDRESS(GCPhys);

    if (PageAddr == PAGE_ADDRESS(GCPhys + cbBuf))
    {
        if (PageAddr != pPageCache->GCPhysReadCacheAddr)
        {
            PDMDevHlpPhysRead(pThis->pDevInsR3, PageAddr,
                              pPageCache->au8PhysReadCache, sizeof(pPageCache->au8PhysReadCache));
            pPageCache->GCPhysReadCacheAddr = PageAddr;
#  ifdef VBOX_WITH_OHCI_PHYS_READ_STATS
            ++g_PhysReadState.cPageReads;
#  endif
        }

        memcpy(pvBuf, &pPageCache->au8PhysReadCache[GCPhys & PAGE_OFFSET_MASK], cbBuf);
#  ifdef VBOX_WITH_OHCI_PHYS_READ_STATS
        ++g_PhysReadState.cCacheReads;
#  endif
    }
    else
    {
        PDMDevHlpPhysRead(pThis->pDevInsR3, GCPhys, pvBuf, cbBuf);
#  ifdef VBOX_WITH_OHCI_PHYS_READ_STATS
        ++g_PhysReadState.cCrossReads;
#  endif
    }
}


/**
 * Updates the data in the given page cache if the given guest physical address is currently contained
 * in the cache.
 *
 * @returns nothing.
 * @param   pPageCache  The page cache to update.
 * @param   GCPhys      The guest physical address needing the update.
 * @param   pvBuf       Pointer to the buffer to update the page cache with.
 * @param   cbBuf       Number of bytes to update.
 */
static void ohciPhysCacheUpdate(POHCIPAGECACHE pPageCache, RTGCPHYS GCPhys, const void *pvBuf, size_t cbBuf)
{
    const RTGCPHYS GCPhysPage = PAGE_ADDRESS(GCPhys);

    if (GCPhysPage == pPageCache->GCPhysReadCacheAddr)
    {
        uint32_t offPage = GCPhys & PAGE_OFFSET_MASK;
        memcpy(&pPageCache->au8PhysReadCache[offPage], pvBuf, RT_MIN(PAGE_SIZE - offPage, cbBuf));
    }
}

/**
 * Update any cached ED data with the given endpoint descriptor at the given address.
 *
 * @returns nothing.
 * @param   pThis       The OHCI instance data.
 * @param   EdAddr      Endpoint descriptor address.
 * @param   pEd         The endpoint descriptor which got updated.
 */
DECLINLINE(void) ohciCacheEdUpdate(POHCI pThis, RTGCPHYS32 EdAddr, PCOHCIED pEd)
{
    ohciPhysCacheUpdate(pThis->pCacheED, EdAddr + RT_OFFSETOF(OHCIED, HeadP), &pEd->HeadP, sizeof(uint32_t));
}


/**
 * Update any cached TD data with the given transfer descriptor at the given address.
 *
 * @returns nothing.
 * @param   pThis       The OHCI instance data.
 * @param   TdAddr      Transfer descriptor address.
 * @param   pTd         The transfer descriptor which got updated.
 */
DECLINLINE(void) ohciCacheTdUpdate(POHCI pThis, RTGCPHYS32 TdAddr, PCOHCITD pTd)
{
    ohciPhysCacheUpdate(pThis->pCacheTD, TdAddr, pTd, sizeof(*pTd));
}

# endif /* VBOX_WITH_OHCI_PHYS_READ_CACHE */

/**
 * Reads an OHCIED.
 */
DECLINLINE(void) ohciReadEd(POHCI pThis, uint32_t EdAddr, POHCIED pEd)
{
# ifdef VBOX_WITH_OHCI_PHYS_READ_STATS
    physReadStatsUpdateDesc(&g_PhysReadState.ed, EdAddr);
    physReadStatsUpdateDesc(&g_PhysReadState.all, EdAddr);
# endif
#ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
    ohciPhysReadCacheRead(pThis, pThis->pCacheED, EdAddr, pEd, sizeof(*pEd));
#else
    ohciGetDWords(pThis, EdAddr, (uint32_t *)pEd, sizeof(*pEd) >> 2);
#endif
}

/**
 * Reads an OHCITD.
 */
DECLINLINE(void) ohciReadTd(POHCI pThis, uint32_t TdAddr, POHCITD pTd)
{
# ifdef VBOX_WITH_OHCI_PHYS_READ_STATS
    physReadStatsUpdateDesc(&g_PhysReadState.td, TdAddr);
    physReadStatsUpdateDesc(&g_PhysReadState.all, TdAddr);
# endif
#ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
    ohciPhysReadCacheRead(pThis, pThis->pCacheTD, TdAddr, pTd, sizeof(*pTd));
#else
    ohciGetDWords(pThis, TdAddr, (uint32_t *)pTd, sizeof(*pTd) >> 2);
#endif
# ifdef LOG_ENABLED
    if (LogIs3Enabled())
    {
        uint32_t hichg;
        hichg = pTd->hwinfo;
        Log3(("ohciReadTd(,%#010x,): R=%d DP=%d DI=%d T=%d EC=%d CC=%#x CBP=%#010x NextTD=%#010x BE=%#010x UNK=%#x\n",
              TdAddr,
              (pTd->hwinfo >> 18) & 1,
              (pTd->hwinfo >> 19) & 3,
              (pTd->hwinfo >> 21) & 7,
              (pTd->hwinfo >> 24) & 3,
              (pTd->hwinfo >> 26) & 3,
              (pTd->hwinfo >> 28) &15,
              pTd->cbp,
              pTd->NextTD,
              pTd->be,
              pTd->hwinfo & TD_HWINFO_UNKNOWN_MASK));
#  if 0
        if (LogIs3Enabled())
        {
            /*
             * usbohci.sys (32-bit XP) allocates 0x80 bytes per TD:
             *  0x00-0x0f is the OHCI TD.
             *  0x10-0x1f for isochronous TDs
             *  0x20 is the physical address of this TD.
             *  0x24 is initialized with 0x64745948, probably a magic.
             *  0x28 is some kind of flags. the first bit begin the allocated / not allocated indicator.
             *  0x30 is a pointer to something. endpoint? interface? device?
             *  0x38 is initialized to 0xdeadface. but is changed into a pointer or something.
             *  0x40 looks like a pointer.
             * The rest is unknown and initialized with zeros.
             */
            uint8_t abXpTd[0x80];
            ohciPhysRead(pThis, TdAddr, abXpTd, sizeof(abXpTd));
            Log3(("WinXpTd: alloc=%d PhysSelf=%RX32 s2=%RX32 magic=%RX32 s4=%RX32 s5=%RX32\n"
                  "%.*Rhxd\n",
                  abXpTd[28] & RT_BIT(0),
                  *((uint32_t *)&abXpTd[0x20]), *((uint32_t *)&abXpTd[0x30]),
                  *((uint32_t *)&abXpTd[0x24]), *((uint32_t *)&abXpTd[0x38]),
                  *((uint32_t *)&abXpTd[0x40]),
                  sizeof(abXpTd), &abXpTd[0]));
        }
#  endif
    }
# endif
}

/**
 * Reads an OHCIITD.
 */
DECLINLINE(void) ohciReadITd(POHCI pThis, uint32_t ITdAddr, POHCIITD pITd)
{
    ohciGetDWords(pThis, ITdAddr, (uint32_t *)pITd, sizeof(*pITd) / sizeof(uint32_t));
# ifdef LOG_ENABLED
    if (LogIs3Enabled())
    {
        Log3(("ohciReadITd(,%#010x,): SF=%#06x (%#RX32) DI=%#x FC=%d CC=%#x BP0=%#010x NextTD=%#010x BE=%#010x\n",
              ITdAddr,
              pITd->HwInfo & 0xffff, pThis->HcFmNumber,
              (pITd->HwInfo >> 21) & 7,
              (pITd->HwInfo >> 24) & 7,
              (pITd->HwInfo >> 28) &15,
              pITd->BP0,
              pITd->NextTD,
              pITd->BE));
        Log3(("psw0=%x:%03x psw1=%x:%03x psw2=%x:%03x psw3=%x:%03x psw4=%x:%03x psw5=%x:%03x psw6=%x:%03x psw7=%x:%03x\n",
              pITd->aPSW[0] >> 12, pITd->aPSW[0] & 0xfff,
              pITd->aPSW[1] >> 12, pITd->aPSW[1] & 0xfff,
              pITd->aPSW[2] >> 12, pITd->aPSW[2] & 0xfff,
              pITd->aPSW[3] >> 12, pITd->aPSW[3] & 0xfff,
              pITd->aPSW[4] >> 12, pITd->aPSW[4] & 0xfff,
              pITd->aPSW[5] >> 12, pITd->aPSW[5] & 0xfff,
              pITd->aPSW[6] >> 12, pITd->aPSW[6] & 0xfff,
              pITd->aPSW[7] >> 12, pITd->aPSW[7] & 0xfff));
    }
# endif
}


/**
 * Writes an OHCIED.
 */
DECLINLINE(void) ohciWriteEd(POHCI pThis, uint32_t EdAddr, PCOHCIED pEd)
{
# ifdef LOG_ENABLED
    if (LogIs3Enabled())
    {
        OHCIED      EdOld;
        uint32_t    hichg;

        ohciGetDWords(pThis, EdAddr, (uint32_t *)&EdOld, sizeof(EdOld) >> 2);
        hichg = EdOld.hwinfo ^ pEd->hwinfo;
        Log3(("ohciWriteEd(,%#010x,): %sFA=%#x %sEN=%#x %sD=%#x %sS=%d %sK=%d %sF=%d %sMPS=%#x %sTailP=%#010x %sHeadP=%#010x %sH=%d %sC=%d %sNextED=%#010x\n",
              EdAddr,
              (hichg >>  0) & 0x7f ? "*" : "", (pEd->hwinfo >>  0) & 0x7f,
              (hichg >>  7) &  0xf ? "*" : "", (pEd->hwinfo >>  7) &  0xf,
              (hichg >> 11) &    3 ? "*" : "", (pEd->hwinfo >> 11) &    3,
              (hichg >> 13) &    1 ? "*" : "", (pEd->hwinfo >> 13) &    1,
              (hichg >> 14) &    1 ? "*" : "", (pEd->hwinfo >> 14) &    1,
              (hichg >> 15) &    1 ? "*" : "", (pEd->hwinfo >> 15) &    1,
              (hichg >> 24) &0x3ff ? "*" : "", (pEd->hwinfo >> 16) &0x3ff,
              EdOld.TailP != pEd->TailP ? "*" : "", pEd->TailP,
              (EdOld.HeadP & ~3) != (pEd->HeadP & ~3) ? "*" : "", pEd->HeadP & ~3,
              (EdOld.HeadP ^ pEd->HeadP) & 1 ? "*" : "", pEd->HeadP & 1,
              (EdOld.HeadP ^ pEd->HeadP) & 2 ? "*" : "", (pEd->HeadP >> 1) & 1,
              EdOld.NextED != pEd->NextED ? "*" : "", pEd->NextED));
    }
# endif

    ohciPutDWords(pThis, EdAddr + RT_OFFSETOF(OHCIED, HeadP), &pEd->HeadP, 1);
#ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
    ohciCacheEdUpdate(pThis, EdAddr, pEd);
#endif
}


/**
 * Writes an OHCITD.
 */
DECLINLINE(void) ohciWriteTd(POHCI pThis, uint32_t TdAddr, PCOHCITD pTd, const char *pszLogMsg)
{
# ifdef LOG_ENABLED
    if (LogIs3Enabled())
    {
        OHCITD TdOld;
        ohciGetDWords(pThis, TdAddr, (uint32_t *)&TdOld, sizeof(TdOld) >> 2);
        uint32_t hichg = TdOld.hwinfo ^ pTd->hwinfo;
        Log3(("ohciWriteTd(,%#010x,): %sR=%d %sDP=%d %sDI=%#x %sT=%d %sEC=%d %sCC=%#x %sCBP=%#010x %sNextTD=%#010x %sBE=%#010x (%s)\n",
              TdAddr,
              (hichg >> 18) & 1 ? "*" : "", (pTd->hwinfo >> 18) & 1,
              (hichg >> 19) & 3 ? "*" : "", (pTd->hwinfo >> 19) & 3,
              (hichg >> 21) & 7 ? "*" : "", (pTd->hwinfo >> 21) & 7,
              (hichg >> 24) & 3 ? "*" : "", (pTd->hwinfo >> 24) & 3,
              (hichg >> 26) & 3 ? "*" : "", (pTd->hwinfo >> 26) & 3,
              (hichg >> 28) &15 ? "*" : "", (pTd->hwinfo >> 28) &15,
              TdOld.cbp  != pTd->cbp  ? "*" : "", pTd->cbp,
              TdOld.NextTD != pTd->NextTD ? "*" : "", pTd->NextTD,
              TdOld.be   != pTd->be   ? "*" : "", pTd->be,
              pszLogMsg));
    }
# else
    RT_NOREF(pszLogMsg);
# endif
    ohciPutDWords(pThis, TdAddr, (uint32_t *)pTd, sizeof(*pTd) >> 2);
#ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
    ohciCacheTdUpdate(pThis, TdAddr, pTd);
#endif
}

/**
 * Writes an OHCIITD.
 */
DECLINLINE(void) ohciWriteITd(POHCI pThis, uint32_t ITdAddr, PCOHCIITD pITd, const char *pszLogMsg)
{
# ifdef LOG_ENABLED
    if (LogIs3Enabled())
    {
        OHCIITD ITdOld;
        ohciGetDWords(pThis, ITdAddr, (uint32_t *)&ITdOld, sizeof(ITdOld) / sizeof(uint32_t));
        uint32_t HIChg = ITdOld.HwInfo ^ pITd->HwInfo;
        Log3(("ohciWriteITd(,%#010x,): %sSF=%#x (now=%#RX32) %sDI=%#x %sFC=%d %sCC=%#x %sBP0=%#010x %sNextTD=%#010x %sBE=%#010x (%s)\n",
              ITdAddr,
              (HIChg & 0xffff) & 1 ? "*" : "", pITd->HwInfo & 0xffff, pThis->HcFmNumber,
              (HIChg >> 21)    & 7 ? "*" : "", (pITd->HwInfo >> 21) & 7,
              (HIChg >> 24)    & 7 ? "*" : "", (pITd->HwInfo >> 24) & 7,
              (HIChg >> 28)    &15 ? "*" : "", (pITd->HwInfo >> 28) &15,
              ITdOld.BP0    != pITd->BP0    ? "*" : "", pITd->BP0,
              ITdOld.NextTD != pITd->NextTD ? "*" : "", pITd->NextTD,
              ITdOld.BE     != pITd->BE     ? "*" : "", pITd->BE,
              pszLogMsg));
        Log3(("psw0=%s%x:%s%03x psw1=%s%x:%s%03x psw2=%s%x:%s%03x psw3=%s%x:%s%03x psw4=%s%x:%s%03x psw5=%s%x:%s%03x psw6=%s%x:%s%03x psw7=%s%x:%s%03x\n",
              (ITdOld.aPSW[0] >> 12) != (pITd->aPSW[0] >> 12) ? "*" : "", pITd->aPSW[0] >> 12,  (ITdOld.aPSW[0] & 0xfff) != (pITd->aPSW[0] & 0xfff) ? "*" : "", pITd->aPSW[0] & 0xfff,
              (ITdOld.aPSW[1] >> 12) != (pITd->aPSW[1] >> 12) ? "*" : "", pITd->aPSW[1] >> 12,  (ITdOld.aPSW[1] & 0xfff) != (pITd->aPSW[1] & 0xfff) ? "*" : "", pITd->aPSW[1] & 0xfff,
              (ITdOld.aPSW[2] >> 12) != (pITd->aPSW[2] >> 12) ? "*" : "", pITd->aPSW[2] >> 12,  (ITdOld.aPSW[2] & 0xfff) != (pITd->aPSW[2] & 0xfff) ? "*" : "", pITd->aPSW[2] & 0xfff,
              (ITdOld.aPSW[3] >> 12) != (pITd->aPSW[3] >> 12) ? "*" : "", pITd->aPSW[3] >> 12,  (ITdOld.aPSW[3] & 0xfff) != (pITd->aPSW[3] & 0xfff) ? "*" : "", pITd->aPSW[3] & 0xfff,
              (ITdOld.aPSW[4] >> 12) != (pITd->aPSW[4] >> 12) ? "*" : "", pITd->aPSW[4] >> 12,  (ITdOld.aPSW[4] & 0xfff) != (pITd->aPSW[4] & 0xfff) ? "*" : "", pITd->aPSW[4] & 0xfff,
              (ITdOld.aPSW[5] >> 12) != (pITd->aPSW[5] >> 12) ? "*" : "", pITd->aPSW[5] >> 12,  (ITdOld.aPSW[5] & 0xfff) != (pITd->aPSW[5] & 0xfff) ? "*" : "", pITd->aPSW[5] & 0xfff,
              (ITdOld.aPSW[6] >> 12) != (pITd->aPSW[6] >> 12) ? "*" : "", pITd->aPSW[6] >> 12,  (ITdOld.aPSW[6] & 0xfff) != (pITd->aPSW[6] & 0xfff) ? "*" : "", pITd->aPSW[6] & 0xfff,
              (ITdOld.aPSW[7] >> 12) != (pITd->aPSW[7] >> 12) ? "*" : "", pITd->aPSW[7] >> 12,  (ITdOld.aPSW[7] & 0xfff) != (pITd->aPSW[7] & 0xfff) ? "*" : "", pITd->aPSW[7] & 0xfff));
    }
# else
    RT_NOREF(pszLogMsg);
# endif
    ohciPutDWords(pThis, ITdAddr, (uint32_t *)pITd, sizeof(*pITd) / sizeof(uint32_t));
}


# ifdef LOG_ENABLED

/**
 * Core TD queue dumper. LOG_ENABLED builds only.
 */
DECLINLINE(void) ohciDumpTdQueueCore(POHCI pThis, uint32_t GCPhysHead, uint32_t GCPhysTail, bool fFull)
{
    uint32_t GCPhys = GCPhysHead;
    int cMax = 100;
    for (;;)
    {
        OHCITD Td;
        Log4(("%#010x%s%s", GCPhys,
              GCPhys && ohci_in_flight_find(pThis, GCPhys) >= 0 ? "~" : "",
              GCPhys && ohci_in_done_queue_find(pThis, GCPhys) >= 0 ? "^" : ""));
        if (GCPhys == 0 || GCPhys == GCPhysTail)
            break;

        /* can't use ohciReadTd() because of Log4. */
        ohciGetDWords(pThis, GCPhys, (uint32_t *)&Td, sizeof(Td) >> 2);
        if (fFull)
            Log4((" [R=%d DP=%d DI=%d T=%d EC=%d CC=%#x CBP=%#010x NextTD=%#010x BE=%#010x] -> ",
                  (Td.hwinfo >> 18) & 1,
                  (Td.hwinfo >> 19) & 3,
                  (Td.hwinfo >> 21) & 7,
                  (Td.hwinfo >> 24) & 3,
                  (Td.hwinfo >> 26) & 3,
                  (Td.hwinfo >> 28) &15,
                  Td.cbp,
                  Td.NextTD,
                  Td.be));
        else
            Log4((" -> "));
        GCPhys = Td.NextTD & ED_PTR_MASK;
        Assert(GCPhys != GCPhysHead);
        Assert(cMax-- > 0); NOREF(cMax);
    }
}

/**
 * Dumps a TD queue. LOG_ENABLED builds only.
 */
DECLINLINE(void) ohciDumpTdQueue(POHCI pThis, uint32_t GCPhysHead, const char *pszMsg)
{
    if (pszMsg)
        Log4(("%s: ", pszMsg));
    ohciDumpTdQueueCore(pThis, GCPhysHead, 0, true);
    Log4(("\n"));
}

/**
 * Core ITD queue dumper. LOG_ENABLED builds only.
 */
DECLINLINE(void) ohciDumpITdQueueCore(POHCI pThis, uint32_t GCPhysHead, uint32_t GCPhysTail, bool fFull)
{
    RT_NOREF(fFull);
    uint32_t GCPhys = GCPhysHead;
    int cMax = 100;
    for (;;)
    {
        OHCIITD ITd;
        Log4(("%#010x%s%s", GCPhys,
              GCPhys && ohci_in_flight_find(pThis, GCPhys) >= 0 ? "~" : "",
              GCPhys && ohci_in_done_queue_find(pThis, GCPhys) >= 0 ? "^" : ""));
        if (GCPhys == 0 || GCPhys == GCPhysTail)
            break;

        /* can't use ohciReadTd() because of Log4. */
        ohciGetDWords(pThis, GCPhys, (uint32_t *)&ITd, sizeof(ITd) / sizeof(uint32_t));
        /*if (fFull)
            Log4((" [R=%d DP=%d DI=%d T=%d EC=%d CC=%#x CBP=%#010x NextTD=%#010x BE=%#010x] -> ",
                  (Td.hwinfo >> 18) & 1,
                  (Td.hwinfo >> 19) & 3,
                  (Td.hwinfo >> 21) & 7,
                  (Td.hwinfo >> 24) & 3,
                  (Td.hwinfo >> 26) & 3,
                  (Td.hwinfo >> 28) &15,
                  Td.cbp,
                  Td.NextTD,
                  Td.be));
        else*/
            Log4((" -> "));
        GCPhys = ITd.NextTD & ED_PTR_MASK;
        Assert(GCPhys != GCPhysHead);
        Assert(cMax-- > 0); NOREF(cMax);
    }
}

/**
 * Dumps a ED list. LOG_ENABLED builds only.
 */
DECLINLINE(void) ohciDumpEdList(POHCI pThis, uint32_t GCPhysHead, const char *pszMsg, bool fTDs)
{
    RT_NOREF(fTDs);
    uint32_t GCPhys = GCPhysHead;
    if (pszMsg)
        Log4(("%s:", pszMsg));
    for (;;)
    {
        OHCIED Ed;

        /* ED */
        Log4((" %#010x={", GCPhys));
        if (!GCPhys)
        {
            Log4(("END}\n"));
            return;
        }

        /* TDs */
        ohciReadEd(pThis, GCPhys, &Ed);
        if (Ed.hwinfo & ED_HWINFO_ISO)
            Log4(("[I]"));
        if ((Ed.HeadP & ED_HEAD_HALTED) || (Ed.hwinfo & ED_HWINFO_SKIP))
        {
            if ((Ed.HeadP & ED_HEAD_HALTED) && (Ed.hwinfo & ED_HWINFO_SKIP))
                Log4(("SH}"));
            else if (Ed.hwinfo & ED_HWINFO_SKIP)
                Log4(("S-}"));
            else
                Log4(("-H}"));
        }
        else
        {
            if (Ed.hwinfo & ED_HWINFO_ISO)
                ohciDumpITdQueueCore(pThis, Ed.HeadP & ED_PTR_MASK, Ed.TailP & ED_PTR_MASK, false);
            else
                ohciDumpTdQueueCore(pThis, Ed.HeadP & ED_PTR_MASK, Ed.TailP & ED_PTR_MASK, false);
            Log4(("}"));
        }

        /* next */
        GCPhys = Ed.NextED & ED_PTR_MASK;
        Assert(GCPhys != GCPhysHead);
    }
    /* not reached */
}

# endif /* LOG_ENABLED */


DECLINLINE(int) ohci_in_flight_find_free(POHCI pThis, const int iStart)
{
    unsigned i = iStart;
    while (i < RT_ELEMENTS(pThis->aInFlight))
    {
        if (pThis->aInFlight[i].GCPhysTD == 0)
            return i;
        i++;
    }
    i = iStart;
    while (i-- > 0)
    {
        if (pThis->aInFlight[i].GCPhysTD == 0)
            return i;
    }
    return -1;
}


/**
 * Record an in-flight TD.
 *
 * @param   pThis       OHCI instance data.
 * @param   GCPhysTD    Physical address of the TD.
 * @param   pUrb        The URB.
 */
static void ohci_in_flight_add(POHCI pThis, uint32_t GCPhysTD, PVUSBURB pUrb)
{
    int i = ohci_in_flight_find_free(pThis, (GCPhysTD >> 4) % RT_ELEMENTS(pThis->aInFlight));
    if (i >= 0)
    {
# ifdef LOG_ENABLED
        pUrb->pHci->u32FrameNo = pThis->HcFmNumber;
# endif
        pThis->aInFlight[i].GCPhysTD = GCPhysTD;
        pThis->aInFlight[i].pUrb = pUrb;
        pThis->cInFlight++;
        return;
    }
    AssertMsgFailed(("Out of space cInFlight=%d!\n", pThis->cInFlight));
}


/**
 * Record in-flight TDs for an URB.
 *
 * @param   pThis       OHCI instance data.
 * @param   pUrb        The URB.
 */
static void ohci_in_flight_add_urb(POHCI pThis, PVUSBURB pUrb)
{
    for (unsigned iTd = 0; iTd < pUrb->pHci->cTds; iTd++)
        ohci_in_flight_add(pThis, pUrb->paTds[iTd].TdAddr, pUrb);
}


/**
 * Finds a in-flight TD.
 *
 * @returns Index of the record.
 * @returns -1 if not found.
 * @param   pThis       OHCI instance data.
 * @param   GCPhysTD    Physical address of the TD.
 * @remark  This has to be fast.
 */
static int ohci_in_flight_find(POHCI pThis, uint32_t GCPhysTD)
{
    unsigned cLeft = pThis->cInFlight;
    unsigned i = (GCPhysTD >> 4) % RT_ELEMENTS(pThis->aInFlight);
    const int iLast = i;
    while (i < RT_ELEMENTS(pThis->aInFlight))
    {
        if (pThis->aInFlight[i].GCPhysTD == GCPhysTD)
            return i;
        if (pThis->aInFlight[i].GCPhysTD)
            if (cLeft-- <= 1)
                return -1;
        i++;
    }
    i = iLast;
    while (i-- > 0)
    {
        if (pThis->aInFlight[i].GCPhysTD == GCPhysTD)
            return i;
        if (pThis->aInFlight[i].GCPhysTD)
            if (cLeft-- <= 1)
                return -1;
    }
    return -1;
}


/**
 * Checks if a TD is in-flight.
 *
 * @returns true if in flight, false if not.
 * @param   pThis       OHCI instance data.
 * @param   GCPhysTD    Physical address of the TD.
 */
static bool ohciIsTdInFlight(POHCI pThis, uint32_t GCPhysTD)
{
    return ohci_in_flight_find(pThis, GCPhysTD) >= 0;
}

/**
 * Returns a URB associated with an in-flight TD, if any.
 *
 * @returns pointer to URB if TD is in flight.
 * @returns NULL if not in flight.
 * @param   pThis       OHCI instance data.
 * @param   GCPhysTD    Physical address of the TD.
 */
static PVUSBURB ohciTdInFlightUrb(POHCI pThis, uint32_t GCPhysTD)
{
    int i;

    i = ohci_in_flight_find(pThis, GCPhysTD);
    if ( i >= 0 )
        return pThis->aInFlight[i].pUrb;
    return NULL;
}

/**
 * Removes a in-flight TD.
 *
 * @returns 0 if found. For logged builds this is the number of frames the TD has been in-flight.
 * @returns -1 if not found.
 * @param   pThis       OHCI instance data.
 * @param   GCPhysTD    Physical address of the TD.
 */
static int ohci_in_flight_remove(POHCI pThis, uint32_t GCPhysTD)
{
    int i = ohci_in_flight_find(pThis, GCPhysTD);
    if (i >= 0)
    {
# ifdef LOG_ENABLED
        const int cFramesInFlight = pThis->HcFmNumber - pThis->aInFlight[i].pUrb->pHci->u32FrameNo;
# else
        const int cFramesInFlight = 0;
# endif
        Log2(("ohci_in_flight_remove: reaping TD=%#010x %d frames (%#010x-%#010x)\n",
              GCPhysTD, cFramesInFlight, pThis->aInFlight[i].pUrb->pHci->u32FrameNo, pThis->HcFmNumber));
        pThis->aInFlight[i].GCPhysTD = 0;
        pThis->aInFlight[i].pUrb = NULL;
        pThis->cInFlight--;
        return cFramesInFlight;
    }
    AssertMsgFailed(("TD %#010x is not in flight\n", GCPhysTD));
    return -1;
}


/**
 * Removes all TDs associated with a URB from the in-flight tracking.
 *
 * @returns 0 if found. For logged builds this is the number of frames the TD has been in-flight.
 * @returns -1 if not found.
 * @param   pThis       OHCI instance data.
 * @param   pUrb        The URB.
 */
static int ohci_in_flight_remove_urb(POHCI pThis, PVUSBURB pUrb)
{
    int cFramesInFlight = ohci_in_flight_remove(pThis, pUrb->paTds[0].TdAddr);
    if (pUrb->pHci->cTds > 1)
    {
        for (unsigned iTd = 1; iTd < pUrb->pHci->cTds; iTd++)
            if (ohci_in_flight_remove(pThis, pUrb->paTds[iTd].TdAddr) < 0)
                cFramesInFlight = -1;
    }
    return cFramesInFlight;
}


# if defined(VBOX_STRICT) || defined(LOG_ENABLED)

/**
 * Empties the in-done-queue.
 * @param   pThis       OHCI instance data.
 */
static void ohci_in_done_queue_zap(POHCI pThis)
{
    pThis->cInDoneQueue = 0;
}

/**
 * Finds a TD in the in-done-queue.
 * @returns >= 0 on success.
 * @returns -1 if not found.
 * @param   pThis       OHCI instance data.
 * @param   GCPhysTD    Physical address of the TD.
 */
static int ohci_in_done_queue_find(POHCI pThis, uint32_t GCPhysTD)
{
    unsigned i = pThis->cInDoneQueue;
    while (i-- > 0)
        if (pThis->aInDoneQueue[i].GCPhysTD == GCPhysTD)
            return i;
    return -1;
}

/**
 * Checks that the specified TD is not in the done queue.
 * @param   pThis       OHCI instance data.
 * @param   GCPhysTD    Physical address of the TD.
 */
static bool ohci_in_done_queue_check(POHCI pThis, uint32_t GCPhysTD)
{
    int i = ohci_in_done_queue_find(pThis, GCPhysTD);
#  if 0
    /* This condition has been observed with the USB tablet emulation or with
     * a real USB mouse and an SMP XP guest.  I am also not sure if this is
     * really a problem for us.  The assertion checks that the guest doesn't
     * re-submit a TD which is still in the done queue.  It seems to me that
     * this should only be a problem if we either keep track of TDs in the done
     * queue somewhere else as well (in which case we should also free those
     * references in time, and I can't see any code doing that) or if we
     * manipulate TDs in the done queue in some way that might fail if they are
     * re-submitted (can't see anything like that either).
     */
    AssertMsg(i < 0, ("TD %#010x (i=%d)\n", GCPhysTD, i));
#  endif
    return i < 0;
}


#  if defined(VBOX_STRICT) && defined(LOG_ENABLED)
/**
 * Adds a TD to the in-done-queue tracking, checking that it's not there already.
 * @param   pThis       OHCI instance data.
 * @param   GCPhysTD    Physical address of the TD.
 */
static void ohci_in_done_queue_add(POHCI pThis, uint32_t GCPhysTD)
{
    Assert(pThis->cInDoneQueue + 1 <= RT_ELEMENTS(pThis->aInDoneQueue));
    if (ohci_in_done_queue_check(pThis, GCPhysTD))
        pThis->aInDoneQueue[pThis->cInDoneQueue++].GCPhysTD = GCPhysTD;
}
#  endif /* VBOX_STRICT */
# endif /* defined(VBOX_STRICT) || defined(LOG_ENABLED) */


/**
 * OHCI Transport Buffer - represents a OHCI Transport Descriptor (TD).
 * A TD may be split over max 2 pages.
 */
typedef struct OHCIBUF
{
    /** Pages involved. */
    struct OHCIBUFVEC
    {
        /** The 32-bit physical address of this part. */
        uint32_t Addr;
        /** The length. */
        uint32_t cb;
    } aVecs[2];
    /** Number of valid entries in aVecs. */
    uint32_t    cVecs;
    /** The total length. */
    uint32_t    cbTotal;
} OHCIBUF, *POHCIBUF;


/**
 * Sets up a OHCI transport buffer.
 *
 * @param   pBuf    Ohci buffer.
 * @param   cbp     Current buffer pointer. 32-bit physical address.
 * @param   be      Last byte in buffer (BufferEnd). 32-bit physical address.
 */
static void ohciBufInit(POHCIBUF pBuf, uint32_t cbp, uint32_t be)
{
    if (!cbp || !be)
    {
        pBuf->cVecs = 0;
        pBuf->cbTotal = 0;
        Log2(("ohci: cbp=%#010x be=%#010x cbTotal=0 EMPTY\n", cbp, be));
    }
    else if ((cbp & ~0xfff) == (be & ~0xfff))
    {
        pBuf->aVecs[0].Addr = cbp;
        pBuf->aVecs[0].cb = (be - cbp) + 1;
        pBuf->cVecs   = 1;
        pBuf->cbTotal = pBuf->aVecs[0].cb;
        Log2(("ohci: cbp=%#010x be=%#010x cbTotal=%u\n", cbp, be, pBuf->cbTotal));
    }
    else
    {
        pBuf->aVecs[0].Addr = cbp;
        pBuf->aVecs[0].cb   = 0x1000 - (cbp & 0xfff);
        pBuf->aVecs[1].Addr = be & ~0xfff;
        pBuf->aVecs[1].cb   = (be & 0xfff) + 1;
        pBuf->cVecs   = 2;
        pBuf->cbTotal = pBuf->aVecs[0].cb + pBuf->aVecs[1].cb;
        Log2(("ohci: cbp=%#010x be=%#010x cbTotal=%u PAGE FLIP\n", cbp, be, pBuf->cbTotal));
    }
}

/**
 * Updates a OHCI transport buffer.
 *
 * This is called upon completion to adjust the sector lengths if
 * the total length has changed. (received less then we had space for
 * or a partial transfer.)
 *
 * @param   pBuf        The buffer to update. cbTotal contains the new total on input.
 *                      While the aVecs[*].cb members is updated upon return.
 */
static void ohciBufUpdate(POHCIBUF pBuf)
{
    for (uint32_t i = 0, cbCur = 0; i < pBuf->cVecs; i++)
    {
        if (cbCur + pBuf->aVecs[i].cb > pBuf->cbTotal)
        {
            pBuf->aVecs[i].cb = pBuf->cbTotal - cbCur;
            pBuf->cVecs = i + 1;
            return;
        }
        cbCur += pBuf->aVecs[i].cb;
    }
}


/** A worker for ohciUnlinkTds(). */
static bool ohciUnlinkIsochronousTdInList(POHCI pThis, uint32_t TdAddr, POHCIITD pITd, POHCIED pEd)
{
    const uint32_t  LastTdAddr = pEd->TailP & ED_PTR_MASK;
    Log(("ohciUnlinkIsocTdInList: Unlinking non-head ITD! TdAddr=%#010RX32 HeadTdAddr=%#010RX32 LastEdAddr=%#010RX32\n",
         TdAddr, pEd->HeadP & ED_PTR_MASK, LastTdAddr));
    AssertMsgReturn(LastTdAddr != TdAddr, ("TdAddr=%#010RX32\n", TdAddr), false);

    uint32_t cMax = 256;
    uint32_t CurTdAddr = pEd->HeadP & ED_PTR_MASK;
    while (     CurTdAddr != LastTdAddr
           &&   cMax-- > 0)
    {
        OHCIITD ITd;
        ohciReadITd(pThis, CurTdAddr, &ITd);
        if ((ITd.NextTD & ED_PTR_MASK) == TdAddr)
        {
            ITd.NextTD = (pITd->NextTD & ED_PTR_MASK) | (ITd.NextTD & ~ED_PTR_MASK);
            ohciWriteITd(pThis, CurTdAddr, &ITd, "ohciUnlinkIsocTdInList");
            pITd->NextTD &= ~ED_PTR_MASK;
            return true;
        }

        /* next */
        CurTdAddr = ITd.NextTD & ED_PTR_MASK;
    }

    Log(("ohciUnlinkIsocTdInList: TdAddr=%#010RX32 wasn't found in the list!!! (cMax=%d)\n", TdAddr, cMax));
    return false;
}


/** A worker for ohciUnlinkTds(). */
static bool ohciUnlinkGeneralTdInList(POHCI pThis, uint32_t TdAddr, POHCITD pTd, POHCIED pEd)
{
    const uint32_t  LastTdAddr = pEd->TailP & ED_PTR_MASK;
    Log(("ohciUnlinkGeneralTdInList: Unlinking non-head TD! TdAddr=%#010RX32 HeadTdAddr=%#010RX32 LastEdAddr=%#010RX32\n",
         TdAddr, pEd->HeadP & ED_PTR_MASK, LastTdAddr));
    AssertMsgReturn(LastTdAddr != TdAddr, ("TdAddr=%#010RX32\n", TdAddr), false);

    uint32_t cMax = 256;
    uint32_t CurTdAddr = pEd->HeadP & ED_PTR_MASK;
    while (     CurTdAddr != LastTdAddr
           &&   cMax-- > 0)
    {
        OHCITD Td;
        ohciReadTd(pThis, CurTdAddr, &Td);
        if ((Td.NextTD & ED_PTR_MASK) == TdAddr)
        {
            Td.NextTD = (pTd->NextTD & ED_PTR_MASK) | (Td.NextTD & ~ED_PTR_MASK);
            ohciWriteTd(pThis, CurTdAddr, &Td, "ohciUnlinkGeneralTdInList");
            pTd->NextTD &= ~ED_PTR_MASK;
            return true;
        }

        /* next */
        CurTdAddr = Td.NextTD & ED_PTR_MASK;
    }

    Log(("ohciUnlinkGeneralTdInList: TdAddr=%#010RX32 wasn't found in the list!!! (cMax=%d)\n", TdAddr, cMax));
    return false;
}


/**
 * Unlinks the TDs that makes up the URB from the ED.
 *
 * @returns success indicator. true if successfully unlinked.
 * @returns false if the TD was not found in the list.
 */
static bool ohciUnlinkTds(POHCI pThis, PVUSBURB pUrb, POHCIED pEd)
{
    /*
     * Don't unlink more than once.
     */
    if (pUrb->pHci->fUnlinked)
        return true;
    pUrb->pHci->fUnlinked = true;

    if (pUrb->enmType == VUSBXFERTYPE_ISOC)
    {
        for (unsigned iTd = 0; iTd < pUrb->pHci->cTds; iTd++)
        {
            POHCIITD pITd = (POHCIITD)&pUrb->paTds[iTd].TdCopy[0];
            const uint32_t ITdAddr = pUrb->paTds[iTd].TdAddr;

            /*
             * Unlink the TD from the ED list.
             * The normal case is that it's at the head of the list.
             */
            Assert((ITdAddr & ED_PTR_MASK) == ITdAddr);
            if ((pEd->HeadP & ED_PTR_MASK) == ITdAddr)
            {
                pEd->HeadP = (pITd->NextTD & ED_PTR_MASK) | (pEd->HeadP & ~ED_PTR_MASK);
                pITd->NextTD &= ~ED_PTR_MASK;
            }
            else
            {
                /*
                 * It's probably somewhere in the list, not a unlikely situation with
                 * the current isochronous code.
                 */
                if (!ohciUnlinkIsochronousTdInList(pThis, ITdAddr, pITd, pEd))
                    return false;
            }
        }
    }
    else
    {
        for (unsigned iTd = 0; iTd < pUrb->pHci->cTds; iTd++)
        {
            POHCITD pTd = (POHCITD)&pUrb->paTds[iTd].TdCopy[0];
            const uint32_t TdAddr = pUrb->paTds[iTd].TdAddr;

            /** @todo r=bird: Messing with the toggle flag in prepare is probably not correct
             * when we encounter a STALL error, 4.3.1.3.7.2: ''If an endpoint returns a STALL
             * PID, the  Host Controller retires the General TD with the ConditionCode set
             * to STALL and halts the endpoint. The CurrentBufferPointer, ErrorCount, and
             * dataToggle fields retain the values that they had at the start of the
             * transaction.'' */

            /* update toggle and set data toggle carry */
            pTd->hwinfo &= ~TD_HWINFO_TOGGLE;
            if ( pTd->hwinfo & TD_HWINFO_TOGGLE_HI )
            {
                if ( !!(pTd->hwinfo & TD_HWINFO_TOGGLE_LO) ) /** @todo r=bird: is it just me or doesn't this make sense at all? */
                    pTd->hwinfo |= TD_HWINFO_TOGGLE_LO;
                else
                    pTd->hwinfo &= ~TD_HWINFO_TOGGLE_LO;
            }
            else
            {
                if ( !!(pEd->HeadP & ED_HEAD_CARRY) )        /** @todo r=bird: is it just me or doesn't this make sense at all? */
                    pEd->HeadP |= ED_HEAD_CARRY;
                else
                    pEd->HeadP &= ~ED_HEAD_CARRY;
            }

            /*
             * Unlink the TD from the ED list.
             * The normal case is that it's at the head of the list.
             */
            Assert((TdAddr & ED_PTR_MASK) == TdAddr);
            if ((pEd->HeadP & ED_PTR_MASK) == TdAddr)
            {
                pEd->HeadP = (pTd->NextTD & ED_PTR_MASK) | (pEd->HeadP & ~ED_PTR_MASK);
                pTd->NextTD &= ~ED_PTR_MASK;
            }
            else
            {
                /*
                 * The TD is probably somewhere in the list.
                 *
                 * This shouldn't ever happen unless there was a failure! Even on failure,
                 * we can screw up the HCD state by picking out a TD from within the list
                 * like this! If this turns out to be a problem, we have to find a better
                 * solution. For now we'll hope the HCD handles it...
                 */
                if (!ohciUnlinkGeneralTdInList(pThis, TdAddr, pTd, pEd))
                    return false;
            }

            /*
             * Only unlink the first TD on error.
             * See comment in ohciRhXferCompleteGeneralURB().
             */
            if (pUrb->enmStatus != VUSBSTATUS_OK)
                break;
        }
    }

    return true;
}


/**
 * Checks that the transport descriptors associated with the URB
 * hasn't been changed in any way indicating that they may have been canceled.
 *
 * This rountine also updates the TD copies contained within the URB.
 *
 * @returns true if the URB has been canceled, otherwise false.
 * @param   pThis       The OHCI instance.
 * @param   pUrb        The URB in question.
 * @param   pEd         The ED pointer (optional).
 */
static bool ohciHasUrbBeenCanceled(POHCI pThis, PVUSBURB pUrb, PCOHCIED pEd)
{
    if (!pUrb)
        return true;

    /*
     * Make sure we've got an endpoint descriptor so we can
     * check for tail TDs.
     */
    OHCIED Ed;
    if (!pEd)
    {
        ohciReadEd(pThis, pUrb->pHci->EdAddr, &Ed);
        pEd = &Ed;
    }

    if (pUrb->enmType == VUSBXFERTYPE_ISOC)
    {
        for (unsigned iTd = 0; iTd < pUrb->pHci->cTds; iTd++)
        {
            union
            {
                OHCIITD     ITd;
                uint32_t    au32[8];
            } u;
            if (    (pUrb->paTds[iTd].TdAddr & ED_PTR_MASK)
                ==  (pEd->TailP & ED_PTR_MASK))
            {
                Log(("%s: ohciHasUrbBeenCanceled: iTd=%d cTds=%d TdAddr=%#010RX32 canceled (tail)! [iso]\n",
                     pUrb->pszDesc, iTd, pUrb->pHci->cTds, pUrb->paTds[iTd].TdAddr));
                STAM_COUNTER_INC(&pThis->StatCanceledIsocUrbs);
                return true;
            }
            ohciReadITd(pThis, pUrb->paTds[iTd].TdAddr, &u.ITd);
            if (    u.au32[0] != pUrb->paTds[iTd].TdCopy[0]     /* hwinfo */
                ||  u.au32[1] != pUrb->paTds[iTd].TdCopy[1]     /* bp0 */
                ||  u.au32[3] != pUrb->paTds[iTd].TdCopy[3]     /* be */
                ||  (   u.au32[2] != pUrb->paTds[iTd].TdCopy[2] /* NextTD */
                     && iTd + 1 < pUrb->pHci->cTds /* ignore the last one */)
                ||  u.au32[4] != pUrb->paTds[iTd].TdCopy[4]     /* psw0&1 */
                ||  u.au32[5] != pUrb->paTds[iTd].TdCopy[5]     /* psw2&3 */
                ||  u.au32[6] != pUrb->paTds[iTd].TdCopy[6]     /* psw4&5 */
                ||  u.au32[7] != pUrb->paTds[iTd].TdCopy[7]     /* psw6&7 */
               )
            {
                Log(("%s: ohciHasUrbBeenCanceled: iTd=%d cTds=%d TdAddr=%#010RX32 canceled! [iso]\n",
                     pUrb->pszDesc, iTd, pUrb->pHci->cTds, pUrb->paTds[iTd].TdAddr));
                Log2(("   %.*Rhxs (cur)\n"
                      "!= %.*Rhxs (copy)\n",
                      sizeof(u.ITd), &u.ITd, sizeof(u.ITd), &pUrb->paTds[iTd].TdCopy[0]));
                STAM_COUNTER_INC(&pThis->StatCanceledIsocUrbs);
                return true;
            }
            pUrb->paTds[iTd].TdCopy[2] = u.au32[2];
        }
    }
    else
    {
        for (unsigned iTd = 0; iTd < pUrb->pHci->cTds; iTd++)
        {
            union
            {
                OHCITD      Td;
                uint32_t    au32[4];
            } u;
            if (    (pUrb->paTds[iTd].TdAddr & ED_PTR_MASK)
                ==  (pEd->TailP & ED_PTR_MASK))
            {
                Log(("%s: ohciHasUrbBeenCanceled: iTd=%d cTds=%d TdAddr=%#010RX32 canceled (tail)!\n",
                     pUrb->pszDesc, iTd, pUrb->pHci->cTds, pUrb->paTds[iTd].TdAddr));
                STAM_COUNTER_INC(&pThis->StatCanceledGenUrbs);
                return true;
            }
            ohciReadTd(pThis, pUrb->paTds[iTd].TdAddr, &u.Td);
            if (    u.au32[0] != pUrb->paTds[iTd].TdCopy[0]     /* hwinfo */
                ||  u.au32[1] != pUrb->paTds[iTd].TdCopy[1]     /* cbp */
                ||  u.au32[3] != pUrb->paTds[iTd].TdCopy[3]     /* be */
                ||  (   u.au32[2] != pUrb->paTds[iTd].TdCopy[2] /* NextTD */
                     && iTd + 1 < pUrb->pHci->cTds /* ignore the last one */)
               )
            {
                Log(("%s: ohciHasUrbBeenCanceled: iTd=%d cTds=%d TdAddr=%#010RX32 canceled!\n",
                     pUrb->pszDesc, iTd, pUrb->pHci->cTds, pUrb->paTds[iTd].TdAddr));
                Log2(("   %.*Rhxs (cur)\n"
                      "!= %.*Rhxs (copy)\n",
                      sizeof(u.Td), &u.Td, sizeof(u.Td), &pUrb->paTds[iTd].TdCopy[0]));
                STAM_COUNTER_INC(&pThis->StatCanceledGenUrbs);
                return true;
            }
            pUrb->paTds[iTd].TdCopy[2] = u.au32[2];
        }
    }
    return false;
}


/**
 * Returns the OHCI_CC_* corresponding to the VUSB status code.
 *
 * @returns OHCI_CC_* value.
 * @param   enmStatus   The VUSB status code.
 */
static uint32_t ohciVUsbStatus2OhciStatus(VUSBSTATUS enmStatus)
{
    switch (enmStatus)
    {
        case VUSBSTATUS_OK:             return OHCI_CC_NO_ERROR;
        case VUSBSTATUS_STALL:          return OHCI_CC_STALL;
        case VUSBSTATUS_CRC:            return OHCI_CC_CRC;
        case VUSBSTATUS_DATA_UNDERRUN:  return OHCI_CC_DATA_UNDERRUN;
        case VUSBSTATUS_DATA_OVERRUN:   return OHCI_CC_DATA_OVERRUN;
        case VUSBSTATUS_DNR:            return OHCI_CC_DNR;
        case VUSBSTATUS_NOT_ACCESSED:   return OHCI_CC_NOT_ACCESSED_1;
        default:
            Log(("pUrb->enmStatus=%#x!!!\n", enmStatus));
            return OHCI_CC_DNR;
    }
}


/**
 * Lock the given OHCI controller instance.
 *
 * @returns nothing.
 * @param   pThis               The OHCI controller instance to lock.
 */
DECLINLINE(void) ohciR3Lock(POHCI pThis)
{
    RTCritSectEnter(&pThis->CritSect);

# ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
    /* Clear all caches here to avoid reading stale data from previous lock holders. */
    ohciPhysReadCacheClear(pThis->pCacheED);
    ohciPhysReadCacheClear(pThis->pCacheTD);
# endif
}


/**
 * Unlocks the given OHCI controller instance.
 *
 * @returns nothing.
 * @param   pThis               The OHCI controller instance to unlock.
 */
DECLINLINE(void) ohciR3Unlock(POHCI pThis)
{
# ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
    /*
     * Clear all caches here to avoid leaving stale data behind (paranoia^2,
     * already done in ohciR3Lock).
     */
    ohciPhysReadCacheClear(pThis->pCacheED);
    ohciPhysReadCacheClear(pThis->pCacheTD);
# endif

    RTCritSectLeave(&pThis->CritSect);
}


/**
 * Worker for ohciR3RhXferCompletion that handles the completion of
 * a URB made up of isochronous TDs.
 *
 * In general, all URBs should have status OK.
 */
static void ohciRhXferCompleteIsochronousURB(POHCI pThis, PVUSBURB pUrb /*, POHCIED pEd , int cFmAge*/)
{
    /*
     * Copy the data back (if IN operation) and update the TDs.
     */
    for (unsigned iTd = 0; iTd < pUrb->pHci->cTds; iTd++)
    {
        POHCIITD pITd = (POHCIITD)&pUrb->paTds[iTd].TdCopy[0];
        const uint32_t ITdAddr = pUrb->paTds[iTd].TdAddr;
        const unsigned cFrames = ((pITd->HwInfo & ITD_HWINFO_FC) >> ITD_HWINFO_FC_SHIFT) + 1;
        unsigned       R = (pUrb->pHci->u32FrameNo & ITD_HWINFO_SF) - (pITd->HwInfo & ITD_HWINFO_SF);
        if (R >= 8)
            R = 0; /* submitted ahead of time. */

        /*
         * Only one case of TD level condition code is document, so
         * just set NO_ERROR here to reduce number duplicate code.
         */
        pITd->HwInfo &= ~TD_HWINFO_CC;
        AssertCompile(OHCI_CC_NO_ERROR == 0);

        if (pUrb->enmStatus == VUSBSTATUS_OK)
        {
            /*
             * Update the frames and copy back the data.
             * We assume that we don't get incorrect lengths here.
             */
            for (unsigned i = 0; i < cFrames; i++)
            {
                if (   i < R
                    || pUrb->aIsocPkts[i - R].enmStatus == VUSBSTATUS_NOT_ACCESSED)
                {
                    /* It should already be NotAccessed. */
                    pITd->aPSW[i] |= 0xe000; /* (Don't touch the 12th bit.) */
                    continue;
                }

                /* Update the PSW (save the offset first in case of a IN). */
                uint32_t off = pITd->aPSW[i] & ITD_PSW_OFFSET;
                pITd->aPSW[i] = ohciVUsbStatus2OhciStatus(pUrb->aIsocPkts[i - R].enmStatus)
                              >> (TD_HWINFO_CC_SHIFT - ITD_PSW_CC_SHIFT);

                if (    pUrb->enmDir == VUSBDIRECTION_IN
                    &&  (   pUrb->aIsocPkts[i - R].enmStatus == VUSBSTATUS_OK
                         || pUrb->aIsocPkts[i - R].enmStatus == VUSBSTATUS_DATA_UNDERRUN
                         || pUrb->aIsocPkts[i - R].enmStatus == VUSBSTATUS_DATA_OVERRUN))
                {
                    /* Set the size. */
                    const unsigned   cb = pUrb->aIsocPkts[i - R].cb;
                    pITd->aPSW[i] |= cb & ITD_PSW_SIZE;
                    /* Copy data. */
                    if (cb)
                    {
                        uint8_t *pb = &pUrb->abData[pUrb->aIsocPkts[i - R].off];
                        if (off + cb > 0x1000)
                        {
                            if (off < 0x1000)
                            {
                                /* both */
                                const unsigned cb0 = 0x1000 - off;
                                ohciPhysWrite(pThis, (pITd->BP0 & ITD_BP0_MASK) + off, pb, cb0);
                                ohciPhysWrite(pThis, pITd->BE & ITD_BP0_MASK, pb + cb0, cb - cb0);
                            }
                            else /* only in the 2nd page */
                                ohciPhysWrite(pThis, (pITd->BE & ITD_BP0_MASK) + (off & ITD_BP0_MASK), pb, cb);
                        }
                        else /* only in the 1st page */
                            ohciPhysWrite(pThis, (pITd->BP0 & ITD_BP0_MASK) + off, pb, cb);
                        Log5(("packet %d: off=%#x cb=%#x pb=%p (%#x)\n"
                              "%.*Rhxd\n",
                              i + R, off, cb, pb, pb - &pUrb->abData[0], cb, pb));
                        //off += cb;
                    }
                }
            }

            /*
             * If the last package ended with a NotAccessed status, set ITD CC
             * to DataOverrun to indicate scheduling overrun.
             */
            if (pUrb->aIsocPkts[pUrb->cIsocPkts - 1].enmStatus == VUSBSTATUS_NOT_ACCESSED)
                pITd->HwInfo |= OHCI_CC_DATA_OVERRUN;
        }
        else
        {
            Log(("DevOHCI: Taking untested code path at line %d...\n", __LINE__));
            /*
             * Most status codes only applies to the individual packets.
             *
             * If we get a URB level error code of this kind, we'll distribute
             * it to all the packages unless some other status is available for
             * a package. This is a bit fuzzy, and we will get rid of this code
             * before long!
             */
            //if (pUrb->enmStatus != VUSBSTATUS_DATA_OVERRUN)
            {
                const unsigned uCC = ohciVUsbStatus2OhciStatus(pUrb->enmStatus)
                                   >> (TD_HWINFO_CC_SHIFT - ITD_PSW_CC_SHIFT);
                for (unsigned i = 0; i < cFrames; i++)
                    pITd->aPSW[i] = uCC;
            }
            //else
            //    pITd->HwInfo |= ohciVUsbStatus2OhciStatus(pUrb->enmStatus);
        }

        /*
         * Update the done queue interrupt timer.
         */
        uint32_t DoneInt = (pITd->HwInfo & ITD_HWINFO_DI) >> ITD_HWINFO_DI_SHIFT;
        if ((pITd->HwInfo & TD_HWINFO_CC) != OHCI_CC_NO_ERROR)
            DoneInt = 0; /* It's cleared on error. */
        if (    DoneInt != 0x7
            &&  DoneInt < pThis->dqic)
            pThis->dqic = DoneInt;

        /*
         * Move on to the done list and write back the modified TD.
         */
# ifdef LOG_ENABLED
        if (!pThis->done)
            pThis->u32FmDoneQueueTail = pThis->HcFmNumber;
#  ifdef VBOX_STRICT
        ohci_in_done_queue_add(pThis, ITdAddr);
#  endif
# endif
        pITd->NextTD = pThis->done;
        pThis->done = ITdAddr;

        Log(("%s: ohciRhXferCompleteIsochronousURB: ITdAddr=%#010x EdAddr=%#010x SF=%#x (%#x) CC=%#x FC=%d "
             "psw0=%x:%x psw1=%x:%x psw2=%x:%x psw3=%x:%x psw4=%x:%x psw5=%x:%x psw6=%x:%x psw7=%x:%x R=%d\n",
             pUrb->pszDesc, ITdAddr,
             pUrb->pHci->EdAddr,
             pITd->HwInfo & ITD_HWINFO_SF, pThis->HcFmNumber,
             (pITd->HwInfo & ITD_HWINFO_CC) >> ITD_HWINFO_CC_SHIFT,
             (pITd->HwInfo & ITD_HWINFO_FC) >> ITD_HWINFO_FC_SHIFT,
             pITd->aPSW[0] >> ITD_PSW_CC_SHIFT, pITd->aPSW[0] & ITD_PSW_SIZE,
             pITd->aPSW[1] >> ITD_PSW_CC_SHIFT, pITd->aPSW[1] & ITD_PSW_SIZE,
             pITd->aPSW[2] >> ITD_PSW_CC_SHIFT, pITd->aPSW[2] & ITD_PSW_SIZE,
             pITd->aPSW[3] >> ITD_PSW_CC_SHIFT, pITd->aPSW[3] & ITD_PSW_SIZE,
             pITd->aPSW[4] >> ITD_PSW_CC_SHIFT, pITd->aPSW[4] & ITD_PSW_SIZE,
             pITd->aPSW[5] >> ITD_PSW_CC_SHIFT, pITd->aPSW[5] & ITD_PSW_SIZE,
             pITd->aPSW[6] >> ITD_PSW_CC_SHIFT, pITd->aPSW[6] & ITD_PSW_SIZE,
             pITd->aPSW[7] >> ITD_PSW_CC_SHIFT, pITd->aPSW[7] & ITD_PSW_SIZE,
             R));
        ohciWriteITd(pThis, ITdAddr, pITd, "retired");
    }
}


/**
 * Worker for ohciRhXferCompletion that handles the completion of
 * a URB made up of general TDs.
 */
static void ohciRhXferCompleteGeneralURB(POHCI pThis, PVUSBURB pUrb, POHCIED pEd, int cFmAge)
{
    RT_NOREF(cFmAge);

    /*
     * Copy the data back (if IN operation) and update the TDs.
     */
    unsigned cbLeft = pUrb->cbData;
    uint8_t *pb     = &pUrb->abData[0];
    for (unsigned iTd = 0; iTd < pUrb->pHci->cTds; iTd++)
    {
        POHCITD pTd = (POHCITD)&pUrb->paTds[iTd].TdCopy[0];
        const uint32_t TdAddr = pUrb->paTds[iTd].TdAddr;

        /*
         * Setup a ohci transfer buffer and calc the new cbp value.
         */
        OHCIBUF Buf;
        ohciBufInit(&Buf, pTd->cbp, pTd->be);
        uint32_t NewCbp;
        if (cbLeft >= Buf.cbTotal)
            NewCbp = 0;
        else
        {
            /* (len may have changed for short transfers) */
            Buf.cbTotal = cbLeft;
            ohciBufUpdate(&Buf);
            Assert(Buf.cVecs >= 1);
            NewCbp = Buf.aVecs[Buf.cVecs-1].Addr + Buf.aVecs[Buf.cVecs-1].cb;
        }

        /*
         * Write back IN buffers.
         */
        if (    pUrb->enmDir == VUSBDIRECTION_IN
            &&  (   pUrb->enmStatus == VUSBSTATUS_OK
                 || pUrb->enmStatus == VUSBSTATUS_DATA_OVERRUN
                 || pUrb->enmStatus == VUSBSTATUS_DATA_UNDERRUN)
            &&  Buf.cbTotal > 0)
        {
            Assert(Buf.cVecs > 0);
            ohciPhysWrite(pThis, Buf.aVecs[0].Addr, pb, Buf.aVecs[0].cb);
            if (Buf.cVecs > 1)
                ohciPhysWrite(pThis, Buf.aVecs[1].Addr, pb + Buf.aVecs[0].cb, Buf.aVecs[1].cb);
        }

        /* advance the data buffer. */
        cbLeft -= Buf.cbTotal;
        pb += Buf.cbTotal;

        /*
         * Set writeback field.
         */
        /* zero out writeback fields for retirement */
        pTd->hwinfo &= ~TD_HWINFO_CC;
        /* always update the CurrentBufferPointer; essential for underrun/overrun errors */
        pTd->cbp = NewCbp;

        if (pUrb->enmStatus == VUSBSTATUS_OK)
        {
            pTd->hwinfo &= ~TD_HWINFO_ERRORS;

            /* update done queue interrupt timer */
            uint32_t DoneInt = (pTd->hwinfo & TD_HWINFO_DI) >> 21;
            if (    DoneInt != 0x7
                &&  DoneInt < pThis->dqic)
                pThis->dqic = DoneInt;
            Log(("%s: ohciRhXferCompleteGeneralURB: ED=%#010x TD=%#010x Age=%d enmStatus=%d cbTotal=%#x NewCbp=%#010RX32 dqic=%d\n",
                 pUrb->pszDesc, pUrb->pHci->EdAddr, TdAddr, cFmAge, pUrb->enmStatus, Buf.cbTotal, NewCbp, pThis->dqic));
        }
        else
        {
            Log(("%s: ohciRhXferCompleteGeneralURB: HALTED ED=%#010x TD=%#010x (age %d) pUrb->enmStatus=%d\n",
                 pUrb->pszDesc, pUrb->pHci->EdAddr, TdAddr, cFmAge, pUrb->enmStatus));
            pEd->HeadP |= ED_HEAD_HALTED;
            pThis->dqic = 0; /* "If the Transfer Descriptor is being retired with an error,
                             *  then the Done Queue Interrupt Counter is cleared as if the
                             *  InterruptDelay field were zero."
                             */
            switch (pUrb->enmStatus)
            {
                case VUSBSTATUS_STALL:
                    pTd->hwinfo |= OHCI_CC_STALL;
                    break;
                case VUSBSTATUS_CRC:
                    pTd->hwinfo |= OHCI_CC_CRC;
                    break;
                case VUSBSTATUS_DATA_UNDERRUN:
                    pTd->hwinfo |= OHCI_CC_DATA_UNDERRUN;
                    break;
                case VUSBSTATUS_DATA_OVERRUN:
                    pTd->hwinfo |= OHCI_CC_DATA_OVERRUN;
                    break;
                default: /* what the hell */
                    Log(("pUrb->enmStatus=%#x!!!\n", pUrb->enmStatus));
                    RT_FALL_THRU();
                case VUSBSTATUS_DNR:
                    pTd->hwinfo |= OHCI_CC_DNR;
                    break;
            }
        }

        /*
         * Move on to the done list and write back the modified TD.
         */
# ifdef LOG_ENABLED
        if (!pThis->done)
            pThis->u32FmDoneQueueTail = pThis->HcFmNumber;
#  ifdef VBOX_STRICT
        ohci_in_done_queue_add(pThis, TdAddr);
#  endif
# endif
        pTd->NextTD = pThis->done;
        pThis->done = TdAddr;

        ohciWriteTd(pThis, TdAddr, pTd, "retired");

        /*
         * If we've halted the endpoint, we stop here.
         * ohciUnlinkTds() will make sure we've only unliked the first TD.
         *
         * The reason for this is that while we can have more than one TD in a URB, real
         * OHCI hardware will only deal with one TD at the time and it's therefore incorrect
         * to retire TDs after the endpoint has been halted. Win2k will crash or enter infinite
         * kernel loop if we don't behave correctly. (See @bugref{1646}.)
         */
        if (pEd->HeadP & ED_HEAD_HALTED)
            break;
    }
}


/**
 * Transfer completion callback routine.
 *
 * VUSB will call this when a transfer have been completed
 * in a one or another way.
 *
 * @param   pInterface      Pointer to OHCI::ROOTHUB::IRhPort.
 * @param   pUrb            Pointer to the URB in question.
 */
static DECLCALLBACK(void) ohciRhXferCompletion(PVUSBIROOTHUBPORT pInterface, PVUSBURB pUrb)
{
    POHCI pThis = VUSBIROOTHUBPORT_2_OHCI(pInterface);
    LogFlow(("%s: ohciRhXferCompletion: EdAddr=%#010RX32 cTds=%d TdAddr0=%#010RX32\n",
             pUrb->pszDesc, pUrb->pHci->EdAddr, pUrb->pHci->cTds, pUrb->paTds[0].TdAddr));

    ohciR3Lock(pThis);
    pThis->fIdle = false;   /* Mark as active */

    /* get the current end point descriptor. */
    OHCIED Ed;
    ohciReadEd(pThis, pUrb->pHci->EdAddr, &Ed);

    /*
     * Check that the URB hasn't been canceled and then try unlink the TDs.
     *
     * We drop the URB if the ED is marked halted/skip ASSUMING that this
     * means the HCD has canceled the URB.
     *
     * If we succeed here (i.e. not dropping the URB), the TdCopy members will
     * be updated but not yet written. We will delay the writing till we're done
     * with the data copying, buffer pointer advancing and error handling.
     */
    int cFmAge = ohci_in_flight_remove_urb(pThis, pUrb);
    if (pUrb->enmStatus == VUSBSTATUS_UNDO)
    {
        /* Leave the TD alone - the HCD doesn't want us talking to the device. */
        Log(("%s: ohciRhXferCompletion: CANCELED {ED=%#010x cTds=%d TD0=%#010x age %d}\n",
             pUrb->pszDesc, pUrb->pHci->EdAddr, pUrb->pHci->cTds, pUrb->paTds[0].TdAddr, cFmAge));
        STAM_COUNTER_INC(&pThis->StatDroppedUrbs);
        ohciR3Unlock(pThis);
        return;
    }
    bool fHasBeenCanceled = false;
    if (    (Ed.HeadP & ED_HEAD_HALTED)
        ||  (Ed.hwinfo & ED_HWINFO_SKIP)
        ||  cFmAge < 0
        ||  (fHasBeenCanceled = ohciHasUrbBeenCanceled(pThis, pUrb, &Ed))
        ||  !ohciUnlinkTds(pThis, pUrb, &Ed)
       )
    {
        Log(("%s: ohciRhXferCompletion: DROPPED {ED=%#010x cTds=%d TD0=%#010x age %d} because:%s%s%s%s%s!!!\n",
             pUrb->pszDesc, pUrb->pHci->EdAddr, pUrb->pHci->cTds, pUrb->paTds[0].TdAddr, cFmAge,
             (Ed.HeadP & ED_HEAD_HALTED)                            ? " ep halted" : "",
             (Ed.hwinfo & ED_HWINFO_SKIP)                           ? " ep skip" : "",
             (Ed.HeadP & ED_PTR_MASK) != pUrb->paTds[0].TdAddr      ? " ep head-changed" : "",
             cFmAge < 0                                             ? " td not-in-flight" : "",
             fHasBeenCanceled                                       ? " td canceled" : ""));
        NOREF(fHasBeenCanceled);
        STAM_COUNTER_INC(&pThis->StatDroppedUrbs);
        ohciR3Unlock(pThis);
        return;
    }

    /*
     * Complete the TD updating and write the back.
     * When appropriate also copy data back to the guest memory.
     */
    if (pUrb->enmType == VUSBXFERTYPE_ISOC)
        ohciRhXferCompleteIsochronousURB(pThis, pUrb /*, &Ed , cFmAge*/);
    else
        ohciRhXferCompleteGeneralURB(pThis, pUrb, &Ed, cFmAge);

    /* finally write back the endpoint descriptor. */
    ohciWriteEd(pThis, pUrb->pHci->EdAddr, &Ed);
    ohciR3Unlock(pThis);
}


/**
 * Handle transfer errors.
 *
 * VUSB calls this when a transfer attempt failed. This function will respond
 * indicating whether to retry or complete the URB with failure.
 *
 * @returns true if the URB should be retired.
 * @returns false if the URB should be retried.
 * @param   pInterface      Pointer to OHCI::ROOTHUB::IRhPort.
 * @param   pUrb            Pointer to the URB in question.
 */
static DECLCALLBACK(bool) ohciRhXferError(PVUSBIROOTHUBPORT pInterface, PVUSBURB pUrb)
{
    POHCI pThis = VUSBIROOTHUBPORT_2_OHCI(pInterface);

    /*
     * Isochronous URBs can't be retried.
     */
    if (pUrb->enmType == VUSBXFERTYPE_ISOC)
        return true;

    /*
     * Don't retry on stall.
     */
    if (pUrb->enmStatus == VUSBSTATUS_STALL)
    {
        Log2(("%s: ohciRhXferError: STALL, giving up.\n", pUrb->pszDesc));
        return true;
    }

    ohciR3Lock(pThis);
    bool fRetire = false;
    /*
     * Check if the TDs still are valid.
     * This will make sure the TdCopy is up to date.
     */
    const uint32_t  TdAddr = pUrb->paTds[0].TdAddr;
/** @todo IMPORTANT! we must check if the ED is still valid at this point!!! */
    if (ohciHasUrbBeenCanceled(pThis, pUrb, NULL))
    {
        Log(("%s: ohciRhXferError: TdAddr0=%#x canceled!\n", pUrb->pszDesc, TdAddr));
        fRetire = true;
    }
    else
    {
        /*
         * Get and update the error counter.
         */
        POHCITD     pTd = (POHCITD)&pUrb->paTds[0].TdCopy[0];
        unsigned    cErrs = (pTd->hwinfo & TD_HWINFO_ERRORS) >> TD_ERRORS_SHIFT;
        pTd->hwinfo &= ~TD_HWINFO_ERRORS;
        cErrs++;
        pTd->hwinfo |= (cErrs % TD_ERRORS_MAX) << TD_ERRORS_SHIFT;
        ohciWriteTd(pThis, TdAddr, pTd, "ohciRhXferError");

        if (cErrs >= TD_ERRORS_MAX - 1)
        {
            Log2(("%s: ohciRhXferError: too many errors, giving up!\n", pUrb->pszDesc));
            fRetire = true;
        }
        else
            Log2(("%s: ohciRhXferError: cErrs=%d: retrying...\n", pUrb->pszDesc, cErrs));
    }

    ohciR3Unlock(pThis);
    return fRetire;
}


/**
 * Service a general transport descriptor.
 */
static bool ohciServiceTd(POHCI pThis, VUSBXFERTYPE enmType, PCOHCIED pEd, uint32_t EdAddr, uint32_t TdAddr,
                          uint32_t *pNextTdAddr, const char *pszListName)
{
    RT_NOREF(pszListName);

    /*
     * Read the TD and setup the buffer data.
     */
    OHCITD Td;
    ohciReadTd(pThis, TdAddr, &Td);
    OHCIBUF Buf;
    ohciBufInit(&Buf, Td.cbp, Td.be);

    *pNextTdAddr = Td.NextTD & ED_PTR_MASK;

    /*
     * Determine the direction.
     */
    VUSBDIRECTION enmDir;
    switch (pEd->hwinfo & ED_HWINFO_DIR)
    {
        case ED_HWINFO_OUT: enmDir = VUSBDIRECTION_OUT; break;
        case ED_HWINFO_IN:  enmDir = VUSBDIRECTION_IN;  break;
        default:
            switch (Td.hwinfo & TD_HWINFO_DIR)
            {
                case TD_HWINFO_OUT: enmDir = VUSBDIRECTION_OUT; break;
                case TD_HWINFO_IN:  enmDir = VUSBDIRECTION_IN; break;
                case 0:             enmDir = VUSBDIRECTION_SETUP; break;
                default:
                    Log(("ohciServiceTd: Invalid direction!!!! Td.hwinfo=%#x Ed.hwdinfo=%#x\n", Td.hwinfo, pEd->hwinfo));
                    /** @todo Do the correct thing here */
                    return false;
            }
            break;
    }

    pThis->fIdle = false;   /* Mark as active */

    /*
     * Allocate and initialize a new URB.
     */
    PVUSBURB pUrb = VUSBIRhNewUrb(pThis->RootHub.pIRhConn, pEd->hwinfo & ED_HWINFO_FUNCTION, NULL,
                                  enmType, enmDir, Buf.cbTotal, 1, NULL);
    if (!pUrb)
        return false;                   /* retry later... */

    pUrb->EndPt = (pEd->hwinfo & ED_HWINFO_ENDPOINT) >> ED_HWINFO_ENDPOINT_SHIFT;
    pUrb->fShortNotOk = !(Td.hwinfo & TD_HWINFO_ROUNDING);
    pUrb->enmStatus = VUSBSTATUS_OK;
    pUrb->pHci->EdAddr = EdAddr;
    pUrb->pHci->fUnlinked = false;
    pUrb->pHci->cTds = 1;
    pUrb->paTds[0].TdAddr = TdAddr;
    pUrb->pHci->u32FrameNo = pThis->HcFmNumber;
    AssertCompile(sizeof(pUrb->paTds[0].TdCopy) >= sizeof(Td));
    memcpy(pUrb->paTds[0].TdCopy, &Td, sizeof(Td));

    /* copy data if out bound transfer. */
    pUrb->cbData = Buf.cbTotal;
    if (    Buf.cbTotal
        &&  Buf.cVecs > 0
        &&  enmDir != VUSBDIRECTION_IN)
    {
        ohciPhysRead(pThis, Buf.aVecs[0].Addr, pUrb->abData, Buf.aVecs[0].cb);
        if (Buf.cVecs > 1)
            ohciPhysRead(pThis, Buf.aVecs[1].Addr, &pUrb->abData[Buf.aVecs[0].cb], Buf.aVecs[1].cb);
    }

    /*
     * Submit the URB.
     */
    ohci_in_flight_add(pThis, TdAddr, pUrb);
    Log(("%s: ohciServiceTd: submitting TdAddr=%#010x EdAddr=%#010x cbData=%#x\n",
         pUrb->pszDesc, TdAddr, EdAddr, pUrb->cbData));

    ohciR3Unlock(pThis);
    int rc = VUSBIRhSubmitUrb(pThis->RootHub.pIRhConn, pUrb, &pThis->RootHub.Led);
    ohciR3Lock(pThis);
    if (RT_SUCCESS(rc))
        return true;

    /* Failure cleanup. Can happen if we're still resetting the device or out of resources. */
    Log(("ohciServiceTd: failed submitting TdAddr=%#010x EdAddr=%#010x pUrb=%p!!\n",
         TdAddr, EdAddr, pUrb));
    ohci_in_flight_remove(pThis, TdAddr);
    return false;
}


/**
 * Service a the head TD of an endpoint.
 */
static bool ohciServiceHeadTd(POHCI pThis, VUSBXFERTYPE enmType, PCOHCIED pEd, uint32_t EdAddr, const char *pszListName)
{
    /*
     * Read the TD, after first checking if it's already in-flight.
     */
    uint32_t TdAddr = pEd->HeadP & ED_PTR_MASK;
    if (ohciIsTdInFlight(pThis, TdAddr))
        return false;
# if defined(VBOX_STRICT) || defined(LOG_ENABLED)
    ohci_in_done_queue_check(pThis, TdAddr);
# endif
    return ohciServiceTd(pThis, enmType, pEd, EdAddr, TdAddr, &TdAddr, pszListName);
}


/**
 * Service one or more general transport descriptors (bulk or interrupt).
 */
static bool ohciServiceTdMultiple(POHCI pThis, VUSBXFERTYPE enmType, PCOHCIED pEd, uint32_t EdAddr,
                                  uint32_t TdAddr, uint32_t *pNextTdAddr, const char *pszListName)
{
    RT_NOREF(pszListName);

    /*
     * Read the TDs involved in this URB.
     */
    struct OHCITDENTRY
    {
        /** The TD. */
        OHCITD      Td;
        /** The associated OHCI buffer tracker. */
        OHCIBUF     Buf;
        /** The TD address. */
        uint32_t    TdAddr;
        /** Pointer to the next element in the chain (stack). */
        struct OHCITDENTRY *pNext;
    }   Head;

# ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
    ohciPhysReadCacheClear(pThis->pCacheTD);
# endif

    /* read the head */
    ohciReadTd(pThis, TdAddr, &Head.Td);
    ohciBufInit(&Head.Buf, Head.Td.cbp, Head.Td.be);
    Head.TdAddr = TdAddr;
    Head.pNext = NULL;

    /* combine with more TDs. */
    struct OHCITDENTRY *pTail   = &Head;
    unsigned            cbTotal = pTail->Buf.cbTotal;
    unsigned            cTds    = 1;
    while (     (pTail->Buf.cbTotal == 0x1000 || pTail->Buf.cbTotal == 0x2000)
           &&   !(pTail->Td.hwinfo & TD_HWINFO_ROUNDING) /* This isn't right for *BSD, but let's not . */
           &&   (pTail->Td.NextTD & ED_PTR_MASK) != (pEd->TailP & ED_PTR_MASK)
           &&   cTds < 128)
    {
        struct OHCITDENTRY *pCur = (struct OHCITDENTRY *)alloca(sizeof(*pCur));

        pCur->pNext = NULL;
        pCur->TdAddr = pTail->Td.NextTD & ED_PTR_MASK;
        ohciReadTd(pThis, pCur->TdAddr, &pCur->Td);
        ohciBufInit(&pCur->Buf, pCur->Td.cbp, pCur->Td.be);

        /* Don't combine if the direction doesn't match up. There can't actually be
         * a mismatch for bulk/interrupt EPs unless the guest is buggy.
         */
        if (    (pCur->Td.hwinfo & (TD_HWINFO_DIR))
            !=  (Head.Td.hwinfo & (TD_HWINFO_DIR)))
            break;

        pTail->pNext = pCur;
        pTail = pCur;
        cbTotal += pCur->Buf.cbTotal;
        cTds++;
    }

    /* calc next TD address */
    *pNextTdAddr = pTail->Td.NextTD & ED_PTR_MASK;

    /*
     * Determine the direction.
     */
    VUSBDIRECTION enmDir;
    switch (pEd->hwinfo & ED_HWINFO_DIR)
    {
        case ED_HWINFO_OUT: enmDir = VUSBDIRECTION_OUT; break;
        case ED_HWINFO_IN:  enmDir = VUSBDIRECTION_IN;  break;
        default:
            Log(("ohciServiceTdMultiple: WARNING! Ed.hwdinfo=%#x bulk or interrupt EP shouldn't rely on the TD for direction...\n", pEd->hwinfo));
            switch (Head.Td.hwinfo & TD_HWINFO_DIR)
            {
                case TD_HWINFO_OUT: enmDir = VUSBDIRECTION_OUT; break;
                case TD_HWINFO_IN:  enmDir = VUSBDIRECTION_IN; break;
                default:
                    Log(("ohciServiceTdMultiple: Invalid direction!!!! Head.Td.hwinfo=%#x Ed.hwdinfo=%#x\n", Head.Td.hwinfo, pEd->hwinfo));
                    /** @todo Do the correct thing here */
                    return false;
            }
            break;
    }

    pThis->fIdle = false;   /* Mark as active */

    /*
     * Allocate and initialize a new URB.
     */
    PVUSBURB pUrb = VUSBIRhNewUrb(pThis->RootHub.pIRhConn, pEd->hwinfo & ED_HWINFO_FUNCTION, NULL,
                                  enmType, enmDir, cbTotal, cTds, "ohciServiceTdMultiple");
    if (!pUrb)
        /* retry later... */
        return false;
    Assert(pUrb->cbData == cbTotal);

    pUrb->enmType = enmType;
    pUrb->EndPt = (pEd->hwinfo & ED_HWINFO_ENDPOINT) >> ED_HWINFO_ENDPOINT_SHIFT;
    pUrb->enmDir = enmDir;
    pUrb->fShortNotOk = !(pTail->Td.hwinfo & TD_HWINFO_ROUNDING);
    pUrb->enmStatus = VUSBSTATUS_OK;
    pUrb->pHci->cTds = cTds;
    pUrb->pHci->EdAddr = EdAddr;
    pUrb->pHci->fUnlinked = false;
    pUrb->pHci->u32FrameNo = pThis->HcFmNumber;

    /* Copy data and TD information. */
    unsigned iTd = 0;
    uint8_t *pb = &pUrb->abData[0];
    for (struct OHCITDENTRY *pCur = &Head; pCur; pCur = pCur->pNext, iTd++)
    {
        /* data */
        if (    cbTotal
            &&  enmDir != VUSBDIRECTION_IN
            &&  pCur->Buf.cVecs > 0)
        {
            ohciPhysRead(pThis, pCur->Buf.aVecs[0].Addr, pb, pCur->Buf.aVecs[0].cb);
            if (pCur->Buf.cVecs > 1)
                ohciPhysRead(pThis, pCur->Buf.aVecs[1].Addr, pb + pCur->Buf.aVecs[0].cb, pCur->Buf.aVecs[1].cb);
        }
        pb += pCur->Buf.cbTotal;

        /* TD info */
        pUrb->paTds[iTd].TdAddr = pCur->TdAddr;
        AssertCompile(sizeof(pUrb->paTds[iTd].TdCopy) >= sizeof(pCur->Td));
        memcpy(pUrb->paTds[iTd].TdCopy, &pCur->Td, sizeof(pCur->Td));
    }

    /*
     * Submit the URB.
     */
    ohci_in_flight_add_urb(pThis, pUrb);
    Log(("%s: ohciServiceTdMultiple: submitting cbData=%#x EdAddr=%#010x cTds=%d TdAddr0=%#010x\n",
         pUrb->pszDesc, pUrb->cbData, EdAddr, cTds, TdAddr));
    ohciR3Unlock(pThis);
    int rc = VUSBIRhSubmitUrb(pThis->RootHub.pIRhConn, pUrb, &pThis->RootHub.Led);
    ohciR3Lock(pThis);
    if (RT_SUCCESS(rc))
        return true;

    /* Failure cleanup. Can happen if we're still resetting the device or out of resources. */
    Log(("ohciServiceTdMultiple: failed submitting pUrb=%p cbData=%#x EdAddr=%#010x cTds=%d TdAddr0=%#010x - rc=%Rrc\n",
         pUrb, cbTotal, EdAddr, cTds, TdAddr, rc));
    for (struct OHCITDENTRY *pCur = &Head; pCur; pCur = pCur->pNext, iTd++)
        ohci_in_flight_remove(pThis, pCur->TdAddr);
    return false;
}


/**
 * Service the head TD of an endpoint.
 */
static bool ohciServiceHeadTdMultiple(POHCI pThis, VUSBXFERTYPE enmType, PCOHCIED pEd, uint32_t EdAddr, const char *pszListName)
{
    /*
     * First, check that it's not already in-flight.
     */
    uint32_t TdAddr = pEd->HeadP & ED_PTR_MASK;
    if (ohciIsTdInFlight(pThis, TdAddr))
        return false;
# if defined(VBOX_STRICT) || defined(LOG_ENABLED)
    ohci_in_done_queue_check(pThis, TdAddr);
# endif
    return ohciServiceTdMultiple(pThis, enmType, pEd, EdAddr, TdAddr, &TdAddr, pszListName);
}


/**
 * A worker for ohciServiceIsochronousEndpoint which unlinks a ITD
 * that belongs to the past.
 */
static bool ohciServiceIsochronousTdUnlink(POHCI pThis, POHCIITD pITd, uint32_t ITdAddr, uint32_t ITdAddrPrev,
                                           PVUSBURB pUrb, POHCIED pEd, uint32_t EdAddr)
{
    LogFlow(("%s%sohciServiceIsochronousTdUnlink: Unlinking ITD: ITdAddr=%#010x EdAddr=%#010x ITdAddrPrev=%#010x\n",
             pUrb ? pUrb->pszDesc : "", pUrb ? ": " : "", ITdAddr, EdAddr, ITdAddrPrev));

    /*
     * Do the unlinking.
     */
    const uint32_t ITdAddrNext = pITd->NextTD & ED_PTR_MASK;
    if (ITdAddrPrev)
    {
        /* Get validate the previous TD */
        int iInFlightPrev = ohci_in_flight_find(pThis, ITdAddrPrev);
        AssertMsgReturn(iInFlightPrev >= 0, ("ITdAddr=%#RX32\n", ITdAddrPrev), false);
        PVUSBURB pUrbPrev = pThis->aInFlight[iInFlightPrev].pUrb;
        if (ohciHasUrbBeenCanceled(pThis, pUrbPrev, pEd)) /* ensures the copy is correct. */
            return false;

        /* Update the copy and write it back. */
        POHCIITD pITdPrev = ((POHCIITD)pUrbPrev->paTds[0].TdCopy);
        pITdPrev->NextTD = (pITdPrev->NextTD & ~ED_PTR_MASK) | ITdAddrNext;
        ohciWriteITd(pThis, ITdAddrPrev, pITdPrev, "ohciServiceIsochronousEndpoint");
    }
    else
    {
        /* It's the head node. update the copy from the caller and write it back. */
        pEd->HeadP = (pEd->HeadP & ~ED_PTR_MASK) | ITdAddrNext;
        ohciWriteEd(pThis, EdAddr, pEd);
    }

    /*
     * If it's in flight, just mark the URB as unlinked (there is only one ITD per URB atm).
     * Otherwise, retire it to the done queue with an error and cause a done line interrupt (?).
     */
    if (pUrb)
    {
        pUrb->pHci->fUnlinked = true;
        if (ohciHasUrbBeenCanceled(pThis, pUrb, pEd)) /* ensures the copy is correct (paranoia). */
            return false;

        POHCIITD pITdCopy = ((POHCIITD)pUrb->paTds[0].TdCopy);
        pITd->NextTD = pITdCopy->NextTD &= ~ED_PTR_MASK;
    }
    else
    {
        pITd->HwInfo &= ~ITD_HWINFO_CC;
        pITd->HwInfo |= OHCI_CC_DATA_OVERRUN;

        pITd->NextTD = pThis->done;
        pThis->done = ITdAddr;

        pThis->dqic = 0;
    }

    ohciWriteITd(pThis, ITdAddr, pITd, "ohciServiceIsochronousTdUnlink");
    return true;
}


/**
 * A worker for ohciServiceIsochronousEndpoint which submits the specified TD.
 *
 * @returns true on success.
 * @returns false on failure to submit.
 * @param   pThis   The OHCI controller instance data.
 * @param   pITd    The transfer descriptor to service.
 * @param   ITdAddr The address of the transfer descriptor in gues memory.
 * @param   R       The start packet (frame) relative to the start of frame in HwInfo.
 * @param   pEd     The OHCI endpoint descriptor.
 * @param   EdAddr  The endpoint descriptor address in guest memory.
 */
static bool ohciServiceIsochronousTd(POHCI pThis, POHCIITD pITd, uint32_t ITdAddr, const unsigned R, PCOHCIED pEd, uint32_t EdAddr)
{
    /*
     * Determine the endpoint direction.
     */
    VUSBDIRECTION enmDir;
    switch (pEd->hwinfo & ED_HWINFO_DIR)
    {
        case ED_HWINFO_OUT: enmDir = VUSBDIRECTION_OUT; break;
        case ED_HWINFO_IN:  enmDir = VUSBDIRECTION_IN;  break;
        default:
            Log(("ohciServiceIsochronousTd: Invalid direction!!!! Ed.hwdinfo=%#x\n", pEd->hwinfo));
            /* Should probably raise an unrecoverable HC error here */
            return false;
    }

    /*
     * Extract the packet sizes and calc the total URB size.
     */
    struct
    {
        uint16_t cb;
        uint16_t off;
    } aPkts[ITD_NUM_PSW];

    /* first entry (R) */
    uint32_t cbTotal = 0;
    if (((uint32_t)pITd->aPSW[R] >> ITD_PSW_CC_SHIFT) < (OHCI_CC_NOT_ACCESSED_0 >> TD_HWINFO_CC_SHIFT))
        Log(("ITdAddr=%RX32 PSW%d.CC=%#x < 'Not Accessed'!\n", ITdAddr, R, pITd->aPSW[R] >> ITD_PSW_CC_SHIFT)); /* => Unrecoverable Error*/
    uint16_t offPrev = aPkts[0].off = (pITd->aPSW[R] & ITD_PSW_OFFSET);

    /* R+1..cFrames */
    const unsigned cFrames = ((pITd->HwInfo & ITD_HWINFO_FC) >> ITD_HWINFO_FC_SHIFT) + 1;
    for (unsigned iR = R + 1; iR < cFrames; iR++)
    {
        const uint16_t PSW = pITd->aPSW[iR];
        const uint16_t off = aPkts[iR - R].off = (PSW & ITD_PSW_OFFSET);
        cbTotal += aPkts[iR - R - 1].cb = off - offPrev;
        if (off < offPrev)
            Log(("ITdAddr=%RX32 PSW%d.offset=%#x < offPrev=%#x!\n", ITdAddr, iR, off, offPrev)); /* => Unrecoverable Error*/
        if (((uint32_t)PSW >> ITD_PSW_CC_SHIFT) < (OHCI_CC_NOT_ACCESSED_0 >> TD_HWINFO_CC_SHIFT))
            Log(("ITdAddr=%RX32 PSW%d.CC=%#x < 'Not Accessed'!\n", ITdAddr, iR, PSW >> ITD_PSW_CC_SHIFT)); /* => Unrecoverable Error*/
        offPrev = off;
    }

    /* calc offEnd and figure out the size of the last packet. */
    const uint32_t offEnd = (pITd->BE & 0xfff)
                          + (((pITd->BE & ITD_BP0_MASK) != (pITd->BP0 & ITD_BP0_MASK)) << 12)
                          + 1 /* BE is inclusive */;
    if (offEnd < offPrev)
        Log(("ITdAddr=%RX32 offEnd=%#x < offPrev=%#x!\n", ITdAddr, offEnd, offPrev)); /* => Unrecoverable Error*/
    cbTotal += aPkts[cFrames - 1 - R].cb = offEnd - offPrev;
    Assert(cbTotal <= 0x2000);

    pThis->fIdle = false;   /* Mark as active */

    /*
     * Allocate and initialize a new URB.
     */
    PVUSBURB pUrb = VUSBIRhNewUrb(pThis->RootHub.pIRhConn, pEd->hwinfo & ED_HWINFO_FUNCTION, NULL,
                                  VUSBXFERTYPE_ISOC, enmDir, cbTotal, 1, NULL);
    if (!pUrb)
        /* retry later... */
        return false;

    pUrb->EndPt           = (pEd->hwinfo & ED_HWINFO_ENDPOINT) >> ED_HWINFO_ENDPOINT_SHIFT;
    pUrb->fShortNotOk     = false;
    pUrb->enmStatus       = VUSBSTATUS_OK;
    pUrb->pHci->EdAddr    = EdAddr;
    pUrb->pHci->cTds      = 1;
    pUrb->pHci->fUnlinked = false;
    pUrb->pHci->u32FrameNo = pThis->HcFmNumber;
    pUrb->paTds[0].TdAddr = ITdAddr;
    AssertCompile(sizeof(pUrb->paTds[0].TdCopy) >= sizeof(*pITd));
    memcpy(pUrb->paTds[0].TdCopy, pITd, sizeof(*pITd));
# if 0 /* color the data */
    memset(pUrb->abData, 0xfe, cbTotal);
# endif

    /* copy the data */
    if (    cbTotal
        &&  enmDir != VUSBDIRECTION_IN)
    {
        const uint32_t off0 = pITd->aPSW[R] & ITD_PSW_OFFSET;
        if (off0 < 0x1000)
        {
            if (offEnd > 0x1000)
            {
                /* both pages. */
                const unsigned cb0 = 0x1000 - off0;
                ohciPhysRead(pThis, (pITd->BP0 & ITD_BP0_MASK) + off0, &pUrb->abData[0], cb0);
                ohciPhysRead(pThis, pITd->BE & ITD_BP0_MASK, &pUrb->abData[cb0], offEnd & 0xfff);
            }
            else /* a portion of the 1st page. */
                ohciPhysRead(pThis, (pITd->BP0 & ITD_BP0_MASK) + off0, pUrb->abData, offEnd - off0);
        }
        else /* a portion of the 2nd page. */
            ohciPhysRead(pThis, (pITd->BE & UINT32_C(0xfffff000)) + (off0 & 0xfff), pUrb->abData, cbTotal);
    }

    /* setup the packets */
    pUrb->cIsocPkts = cFrames - R;
    unsigned off = 0;
    for (unsigned i = 0; i < pUrb->cIsocPkts; i++)
    {
        pUrb->aIsocPkts[i].enmStatus = VUSBSTATUS_NOT_ACCESSED;
        pUrb->aIsocPkts[i].off = off;
        off += pUrb->aIsocPkts[i].cb = aPkts[i].cb;
    }
    Assert(off == cbTotal);

    /*
     * Submit the URB.
     */
    ohci_in_flight_add_urb(pThis, pUrb);
    Log(("%s: ohciServiceIsochronousTd: submitting cbData=%#x cIsocPkts=%d EdAddr=%#010x TdAddr=%#010x SF=%#x (%#x)\n",
         pUrb->pszDesc, pUrb->cbData, pUrb->cIsocPkts, EdAddr, ITdAddr, pITd->HwInfo & ITD_HWINFO_SF, pThis->HcFmNumber));
    ohciR3Unlock(pThis);
    int rc = VUSBIRhSubmitUrb(pThis->RootHub.pIRhConn, pUrb, &pThis->RootHub.Led);
    ohciR3Lock(pThis);
    if (RT_SUCCESS(rc))
        return true;

    /* Failure cleanup. Can happen if we're still resetting the device or out of resources. */
    Log(("ohciServiceIsochronousTd: failed submitting pUrb=%p cbData=%#x EdAddr=%#010x cTds=%d ITdAddr0=%#010x - rc=%Rrc\n",
         pUrb, cbTotal, EdAddr, 1, ITdAddr, rc));
    ohci_in_flight_remove(pThis, ITdAddr);
    return false;
}


/**
 * Service an isochronous endpoint.
 */
static void ohciServiceIsochronousEndpoint(POHCI pThis, POHCIED pEd, uint32_t EdAddr)
{
    /*
     * We currently process this as if the guest follows the interrupt end point chaining
     * hierarchy described in the documenation. This means that for an isochronous endpoint
     * with a 1 ms interval we expect to find in-flight TDs at the head of the list. We will
     * skip over all in-flight TDs which timeframe has been exceed. Those which aren't in
     * flight but which are too late will be retired (possibly out of order, but, we don't
     * care right now).
     *
     * When we reach a TD which still has a buffer which is due for take off, we will
     * stop iterating TDs. If it's in-flight, there isn't anything to be done. Otherwise
     * we will push it onto the runway for immediate take off. In this process we
     * might have to complete buffers which didn't make it on time, something which
     * complicates the kind of status info we need to keep around for the TD.
     *
     * Note: We're currently not making any attempt at reassembling ITDs into URBs.
     *       However, this will become necessary because of EMT scheduling and guest
     *       like linux using one TD for each frame (simple but inefficient for us).
     */
    OHCIITD ITd;
    uint32_t ITdAddr = pEd->HeadP & ED_PTR_MASK;
    uint32_t ITdAddrPrev = 0;
    uint32_t u32NextFrame = UINT32_MAX;
    const uint16_t u16CurFrame = pThis->HcFmNumber;
    for (;;)
    {
        /* check for end-of-chain. */
        if (    ITdAddr == (pEd->TailP & ED_PTR_MASK)
            ||  !ITdAddr)
            break;

        /*
         * If isochronous endpoints are around, don't slow down the timer. Getting the timing right
         * is difficult enough as it is.
         */
        pThis->fIdle = false;

        /*
         * Read the current ITD and check what we're supposed to do about it.
         */
        ohciReadITd(pThis, ITdAddr, &ITd);
        const uint32_t  ITdAddrNext = ITd.NextTD & ED_PTR_MASK;
        const int16_t   R = u16CurFrame - (uint16_t)(ITd.HwInfo & ITD_HWINFO_SF); /* 4.3.2.3 */
        const int16_t   cFrames = ((ITd.HwInfo & ITD_HWINFO_FC) >> ITD_HWINFO_FC_SHIFT) + 1;

        if (R < cFrames)
        {
            /*
             * It's inside the current or a future launch window.
             *
             * We will try maximize the TD in flight here to deal with EMT scheduling
             * issues and similar stuff which will screw up the time. So, we will only
             * stop submitting TD when we reach a gap (in time) or end of the list.
             */
            if (    R < 0   /* (a future frame) */
                &&  (uint16_t)u32NextFrame != (uint16_t)(ITd.HwInfo & ITD_HWINFO_SF))
                break;
            if (ohci_in_flight_find(pThis, ITdAddr) < 0)
                if (!ohciServiceIsochronousTd(pThis, &ITd, ITdAddr, R < 0 ? 0 : R, pEd, EdAddr))
                    break;

            ITdAddrPrev = ITdAddr;
        }
        else
        {
# if 1
            /*
             * Ok, the launch window for this TD has passed.
             * If it's not in flight it should be retired with a DataOverrun status (TD).
             *
             * Don't remove in-flight TDs before they complete.
             * Windows will, upon the completion of another ITD it seems, check for if
             * any other TDs has been unlinked. If we unlink them before they really
             * complete all the packet status codes will be NotAccessed and Windows
             * will fail the URB with status USBD_STATUS_ISOCH_REQUEST_FAILED.
             *
             * I don't know if unlinking TDs out of order could cause similar problems,
             * time will show.
             */
            int iInFlight = ohci_in_flight_find(pThis, ITdAddr);
            if (iInFlight >= 0)
                ITdAddrPrev = ITdAddr;
            else if (!ohciServiceIsochronousTdUnlink(pThis, &ITd, ITdAddr, ITdAddrPrev,
                                                     NULL, pEd, EdAddr))
            {
                Log(("ohciServiceIsochronousEndpoint: Failed unlinking old ITD.\n"));
                break;
            }
# else /* BAD IDEA: */
            /*
             * Ok, the launch window for this TD has passed.
             * If it's not in flight it should be retired with a DataOverrun status (TD).
             *
             * If it's in flight we will try unlink it from the list prematurely to
             * help the guest to move on and shorten the list we have to walk. We currently
             * are successful with the first URB but then it goes too slowly...
             */
            int iInFlight = ohci_in_flight_find(pThis, ITdAddr);
            if (!ohciServiceIsochronousTdUnlink(pThis, &ITd, ITdAddr, ITdAddrPrev,
                                                iInFlight < 0 ? NULL : pThis->aInFlight[iInFlight].pUrb,
                                                pEd, EdAddr))
            {
                Log(("ohciServiceIsochronousEndpoint: Failed unlinking old ITD.\n"));
                break;
            }
# endif
        }

        /* advance to the next ITD */
        ITdAddr = ITdAddrNext;
        u32NextFrame = (ITd.HwInfo & ITD_HWINFO_SF) + cFrames;
    }
}


/**
 * Checks if a endpoints has TDs queued and is ready to have them processed.
 *
 * @returns true if it's ok to process TDs.
 * @param   pEd     The endpoint data.
 */
DECLINLINE(bool) ohciIsEdReady(PCOHCIED pEd)
{
    return (pEd->HeadP & ED_PTR_MASK) != (pEd->TailP & ED_PTR_MASK)
         && !(pEd->HeadP & ED_HEAD_HALTED)
         && !(pEd->hwinfo & ED_HWINFO_SKIP);
}


/**
 * Checks if an endpoint has TDs queued (not necessarily ready to have them processed).
 *
 * @returns true if endpoint may have TDs queued.
 * @param   pEd     The endpoint data.
 */
DECLINLINE(bool) ohciIsEdPresent(PCOHCIED pEd)
{
    return (pEd->HeadP & ED_PTR_MASK) != (pEd->TailP & ED_PTR_MASK)
         && !(pEd->HeadP & ED_HEAD_HALTED);
}


/**
 * Services the bulk list.
 *
 * On the bulk list we must reassemble URBs from multiple TDs using heuristics
 * derived from USB tracing done in the guests and guest source code (when available).
 */
static void ohciServiceBulkList(POHCI pThis)
{
# ifdef LOG_ENABLED
    if (g_fLogBulkEPs)
        ohciDumpEdList(pThis, pThis->bulk_head, "Bulk before", true);
    if (pThis->bulk_cur)
        Log(("ohciServiceBulkList: bulk_cur=%#010x before listprocessing!!! HCD have positioned us!!!\n", pThis->bulk_cur));
# endif

    /*
     * ", HC will start processing the Bulk list and will set BF [BulkListFilled] to 0"
     * - We've simplified and are always starting at the head of the list and working
     *   our way thru to the end each time.
     */
    pThis->status &= ~OHCI_STATUS_BLF;
    pThis->fBulkNeedsCleaning = false;
    pThis->bulk_cur = 0;

    uint32_t EdAddr = pThis->bulk_head;
    while (EdAddr)
    {
        OHCIED Ed;
        ohciReadEd(pThis, EdAddr, &Ed);
        Assert(!(Ed.hwinfo & ED_HWINFO_ISO)); /* the guest is screwing us */
        if (ohciIsEdReady(&Ed))
        {
            pThis->status |= OHCI_STATUS_BLF;
            pThis->fBulkNeedsCleaning = true;

# if 1
            /*

             * After we figured out that all the TDs submitted for dealing with MSD
             * read/write data really makes up on single URB, and that we must
             * reassemble these TDs into an URB before submitting it, there is no
             * longer any need for servicing anything other than the head *URB*
             * on a bulk endpoint.
             */
            ohciServiceHeadTdMultiple(pThis, VUSBXFERTYPE_BULK, &Ed, EdAddr, "Bulk");
# else
            /*
             * This alternative code was used before we started reassembling URBs from
             * multiple TDs. We keep it handy for debugging.
             */
            uint32_t TdAddr = Ed.HeadP & ED_PTR_MASK;
            if (!ohciIsTdInFlight(pThis, TdAddr))
            {
                do
                {
                    if (!ohciServiceTdMultiple(pThis, VUSBXFERTYPE_BULK, &Ed, EdAddr, TdAddr, &TdAddr, "Bulk"))
                    {
                        LogFlow(("ohciServiceBulkList: ohciServiceTdMultiple -> false\n"));
                        break;
                    }
                    if (    (TdAddr & ED_PTR_MASK) == (Ed.TailP & ED_PTR_MASK)
                        ||  !TdAddr /* paranoia */)
                    {
                        LogFlow(("ohciServiceBulkList: TdAddr=%#010RX32 Ed.TailP=%#010RX32\n", TdAddr, Ed.TailP));
                        break;
                    }

                    ohciReadEd(pThis, EdAddr, &Ed); /* It might have been updated on URB completion. */
                } while (ohciIsEdReady(&Ed));
            }
# endif
        }
        else
        {
            if (Ed.hwinfo & ED_HWINFO_SKIP)
            {
                LogFlow(("ohciServiceBulkList: Ed=%#010RX32 Ed.TailP=%#010RX32 SKIP\n", EdAddr, Ed.TailP));
                /* If the ED is in 'skip' state, no transactions on it are allowed and we must
                 * cancel outstanding URBs, if any.
                 */
                uint32_t TdAddr = Ed.HeadP & ED_PTR_MASK;
                PVUSBURB pUrb = ohciTdInFlightUrb(pThis, TdAddr);
                if (pUrb)
                    pThis->RootHub.pIRhConn->pfnCancelUrbsEp(pThis->RootHub.pIRhConn, pUrb);
            }
        }

        /* next end point */
        EdAddr = Ed.NextED & ED_PTR_MASK;

    }

# ifdef LOG_ENABLED
    if (g_fLogBulkEPs)
        ohciDumpEdList(pThis, pThis->bulk_head, "Bulk after ", true);
# endif
}

/**
 * Abort outstanding transfers on the bulk list.
 *
 * If the guest disabled bulk list processing, we must abort any outstanding transfers
 * (that is, cancel in-flight URBs associated with the list). This is required because
 * there may be outstanding read URBs that will never get a response from the device
 * and would block further communication.
 */
static void ohciUndoBulkList(POHCI pThis)
{
# ifdef LOG_ENABLED
    if (g_fLogBulkEPs)
        ohciDumpEdList(pThis, pThis->bulk_head, "Bulk before", true);
    if (pThis->bulk_cur)
        Log(("ohciUndoBulkList: bulk_cur=%#010x before list processing!!! HCD has positioned us!!!\n", pThis->bulk_cur));
# endif

    /* This flag follows OHCI_STATUS_BLF, but BLF doesn't change when list processing is disabled. */
    pThis->fBulkNeedsCleaning = false;

    uint32_t EdAddr = pThis->bulk_head;
    while (EdAddr)
    {
        OHCIED Ed;

        ohciReadEd(pThis, EdAddr, &Ed);
        Assert(!(Ed.hwinfo & ED_HWINFO_ISO)); /* the guest is screwing us */
        if (ohciIsEdPresent(&Ed))
        {
            uint32_t TdAddr = Ed.HeadP & ED_PTR_MASK;
            if (ohciIsTdInFlight(pThis, TdAddr))
            {
                LogFlow(("ohciUndoBulkList: Ed=%#010RX32 Ed.TailP=%#010RX32 UNDO\n", EdAddr, Ed.TailP));
                PVUSBURB pUrb = ohciTdInFlightUrb(pThis, TdAddr);
                if (pUrb)
                    pThis->RootHub.pIRhConn->pfnCancelUrbsEp(pThis->RootHub.pIRhConn, pUrb);
            }
        }
        /* next endpoint */
        EdAddr = Ed.NextED & ED_PTR_MASK;
    }
}


/**
 * Services the control list.
 *
 * The control list has complex URB assembling, but that's taken
 * care of at VUSB level (unlike the other transfer types).
 */
static void ohciServiceCtrlList(POHCI pThis)
{
# ifdef LOG_ENABLED
    if (g_fLogControlEPs)
        ohciDumpEdList(pThis, pThis->ctrl_head, "Ctrl before", true);
    if (pThis->ctrl_cur)
        Log(("ohciServiceCtrlList: ctrl_cur=%010x before list processing!!! HCD have positioned us!!!\n", pThis->ctrl_cur));
# endif

    /*
     * ", HC will start processing the list and will set ControlListFilled to 0"
     * - We've simplified and are always starting at the head of the list and working
     *   our way thru to the end each time.
     */
    pThis->status &= ~OHCI_STATUS_CLF;
    pThis->ctrl_cur = 0;

    uint32_t EdAddr = pThis->ctrl_head;
    while (EdAddr)
    {
        OHCIED Ed;
        ohciReadEd(pThis, EdAddr, &Ed);
        Assert(!(Ed.hwinfo & ED_HWINFO_ISO)); /* the guest is screwing us */
        if (ohciIsEdReady(&Ed))
        {
# if 1
            /*
             * Control TDs depends on order and stage. Only one can be in-flight
             * at any given time. OTOH, some stages are completed immediately,
             * so we process the list until we've got a head which is in-flight
             * or reach the end of the list.
             */
            do
            {
                if (    !ohciServiceHeadTd(pThis, VUSBXFERTYPE_CTRL, &Ed, EdAddr, "Control")
                    ||  ohciIsTdInFlight(pThis, Ed.HeadP & ED_PTR_MASK))
                {
                    pThis->status |= OHCI_STATUS_CLF;
                    break;
                }
                ohciReadEd(pThis, EdAddr, &Ed); /* It might have been updated on URB completion. */
            } while (ohciIsEdReady(&Ed));
# else
            /* Simplistic, for debugging. */
            ohciServiceHeadTd(pThis, VUSBXFERTYPE_CTRL, &Ed, EdAddr, "Control");
            pThis->status |= OHCI_STATUS_CLF;
# endif
        }

        /* next end point */
        EdAddr = Ed.NextED & ED_PTR_MASK;
    }

# ifdef LOG_ENABLED
    if (g_fLogControlEPs)
        ohciDumpEdList(pThis, pThis->ctrl_head, "Ctrl after ", true);
# endif
}


/**
 * Services the periodic list.
 *
 * On the interrupt portion of the periodic list we must reassemble URBs from multiple
 * TDs using heuristics derived from USB tracing done in the guests and guest source
 * code (when available).
 */
static void ohciServicePeriodicList(POHCI pThis)
{
    /*
     * Read the list head from the HCCA.
     */
    const unsigned  iList = pThis->HcFmNumber % OHCI_HCCA_NUM_INTR;
    uint32_t        EdAddr;
    ohciGetDWords(pThis, pThis->hcca + iList * sizeof(EdAddr), &EdAddr, 1);

# ifdef LOG_ENABLED
    const uint32_t EdAddrHead = EdAddr;
    if (g_fLogInterruptEPs)
    {
        char sz[48];
        RTStrPrintf(sz, sizeof(sz), "Int%02x before", iList);
        ohciDumpEdList(pThis, EdAddrHead, sz, true);
    }
# endif

    /*
     * Iterate the endpoint list.
     */
    while (EdAddr)
    {
        OHCIED Ed;

        ohciReadEd(pThis, EdAddr, &Ed);
        if (ohciIsEdReady(&Ed))
        {
            /*
             * "There is no separate head pointer of isochronous transfers. The first
             * isochronous Endpoint Descriptor simply links to the last interrupt
             * Endpoint Descriptor."
             */
            if (!(Ed.hwinfo & ED_HWINFO_ISO))
            {
                /*
                 * Presently we will only process the head URB on an interrupt endpoint.
                 */
                ohciServiceHeadTdMultiple(pThis, VUSBXFERTYPE_INTR, &Ed, EdAddr, "Periodic");
            }
            else if (pThis->ctl & OHCI_CTL_IE)
            {
                /*
                 * Presently only the head ITD.
                 */
                ohciServiceIsochronousEndpoint(pThis, &Ed, EdAddr);
            }
            else
                break;
        }
        else
        {
            if (Ed.hwinfo & ED_HWINFO_SKIP)
            {
                Log3(("ohciServicePeriodicList: Ed=%#010RX32 Ed.TailP=%#010RX32 SKIP\n", EdAddr, Ed.TailP));
                /* If the ED is in 'skip' state, no transactions on it are allowed and we must
                 * cancel outstanding URBs, if any.
                 */
                uint32_t TdAddr = Ed.HeadP & ED_PTR_MASK;
                PVUSBURB pUrb = ohciTdInFlightUrb(pThis, TdAddr);
                if (pUrb)
                    pThis->RootHub.pIRhConn->pfnCancelUrbsEp(pThis->RootHub.pIRhConn, pUrb);
            }
        }
        /* next end point */
        EdAddr = Ed.NextED & ED_PTR_MASK;
    }

# ifdef LOG_ENABLED
    if (g_fLogInterruptEPs)
    {
        char sz[48];
        RTStrPrintf(sz, sizeof(sz), "Int%02x after ", iList);
        ohciDumpEdList(pThis, EdAddrHead, sz, true);
    }
# endif
}


/**
 * Update the HCCA.
 *
 * @param   pThis   The OHCI instance data.
 */
static void ohciUpdateHCCA(POHCI pThis)
{
    struct ohci_hcca hcca;
    ohciPhysRead(pThis, pThis->hcca + OHCI_HCCA_OFS, &hcca, sizeof(hcca));

    hcca.frame = RT_H2LE_U16((uint16_t)pThis->HcFmNumber);
    hcca.pad = 0;

    bool fWriteDoneHeadInterrupt = false;
    if (    pThis->dqic == 0
        &&  (pThis->intr_status & OHCI_INTR_WRITE_DONE_HEAD) == 0)
    {
        uint32_t done = pThis->done;

        if (pThis->intr_status & ~(  OHCI_INTR_MASTER_INTERRUPT_ENABLED | OHCI_INTR_OWNERSHIP_CHANGE
                                   | OHCI_INTR_WRITE_DONE_HEAD) )
            done |= 0x1;

        hcca.done = RT_H2LE_U32(done);
        pThis->done = 0;
        pThis->dqic = 0x7;

        Log(("ohci: Writeback Done (%#010x) on frame %#x (age %#x)\n", hcca.done,
             pThis->HcFmNumber, pThis->HcFmNumber - pThis->u32FmDoneQueueTail));
# ifdef LOG_ENABLED
        ohciDumpTdQueue(pThis, hcca.done & ED_PTR_MASK, "DoneQueue");
# endif
        Assert(RT_OFFSETOF(struct ohci_hcca, done) == 4);
# if defined(VBOX_STRICT) || defined(LOG_ENABLED)
        ohci_in_done_queue_zap(pThis);
# endif
        fWriteDoneHeadInterrupt = true;
    }

    Log3(("ohci: Updating HCCA on frame %#x\n", pThis->HcFmNumber));
    ohciPhysWrite(pThis, pThis->hcca + OHCI_HCCA_OFS, (uint8_t *)&hcca, sizeof(hcca));
    if (fWriteDoneHeadInterrupt)
        ohciR3SetInterrupt(pThis, OHCI_INTR_WRITE_DONE_HEAD);
}


/**
 * Go over the in-flight URB list and cancel any URBs that are no longer in use.
 * This occurs when the host removes EDs or TDs from the lists and we don't notice
 * the sKip bit. Such URBs must be promptly canceled, otherwise there is a risk
 * they might "steal" data destined for another URB.
 */
static void ohciCancelOrphanedURBs(POHCI pThis)
{
    bool fValidHCCA = !(    pThis->hcca >= OHCI_HCCA_MASK
                        ||  pThis->hcca < ~OHCI_HCCA_MASK);
    unsigned    i, cLeft;
    int         j;
    uint32_t    EdAddr;
    PVUSBURB    pUrb;

    /* If the HCCA is not currently valid, or there are no in-flight URBs,
     * there's nothing to do.
     */
    if (!fValidHCCA || !pThis->cInFlight)
        return;

    /* Initially mark all in-flight URBs as inactive. */
    for (i = 0, cLeft = pThis->cInFlight; cLeft && i < RT_ELEMENTS(pThis->aInFlight); i++)
    {
        if (pThis->aInFlight[i].pUrb)
        {
            pThis->aInFlight[i].fInactive = true;
            cLeft--;
        }
    }
    Assert(cLeft == 0);

# ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
    /* Get hcca data to minimize calls to ohciGetDWords/PDMDevHlpPhysRead. */
    uint32_t au32HCCA[OHCI_HCCA_NUM_INTR];
    ohciGetDWords(pThis, pThis->hcca, au32HCCA, OHCI_HCCA_NUM_INTR);
# endif

    /* Go over all bulk/control/interrupt endpoint lists; any URB found in these lists
     * is marked as active again.
     */
    for (i = 0; i < OHCI_HCCA_NUM_INTR + 2; i++)
    {
        switch (i)
        {
        case OHCI_HCCA_NUM_INTR:
            EdAddr = pThis->bulk_head;
            break;
        case OHCI_HCCA_NUM_INTR + 1:
            EdAddr = pThis->ctrl_head;
            break;
        default:
# ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
            EdAddr = au32HCCA[i];
# else
            ohciGetDWords(pThis, pThis->hcca + i * sizeof(EdAddr), &EdAddr, 1);
# endif
            break;
        }
        while (EdAddr)
        {
            OHCIED Ed;
            OHCITD Td;

            ohciReadEd(pThis, EdAddr, &Ed);
            uint32_t TdAddr = Ed.HeadP & ED_PTR_MASK;
            uint32_t TailP  = Ed.TailP & ED_PTR_MASK;
            unsigned k = 0;
            if (  !(Ed.hwinfo & ED_HWINFO_SKIP)
                && (TdAddr != TailP))
            {
# ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
                ohciPhysReadCacheClear(pThis->pCacheTD);
# endif
                do
                {
                    ohciReadTd(pThis, TdAddr, &Td);
                    j = ohci_in_flight_find(pThis, TdAddr);
                    if (j > -1)
                        pThis->aInFlight[j].fInactive = false;
                    TdAddr = Td.NextTD & ED_PTR_MASK;
                    /* See #8125.
                     * Sometimes the ED is changed by the guest between ohciReadEd above and here.
                     * Then the code reads TD pointed by the new TailP, which is not allowed.
                     * Luckily Windows guests have Td.NextTD = 0 in the tail TD.
                     * Also having a real TD at 0 is very unlikely.
                     * So do not continue.
                     */
                    if (TdAddr == 0)
                        break;
                    /* Failsafe for temporarily looped lists. */
                    if (++k == 128)
                        break;
                } while (TdAddr != (Ed.TailP & ED_PTR_MASK));
            }
            EdAddr = Ed.NextED & ED_PTR_MASK;
        }
    }

    /* In-flight URBs still marked as inactive are not used anymore and need
     * to be canceled.
     */
    for (i = 0, cLeft = pThis->cInFlight; cLeft && i < RT_ELEMENTS(pThis->aInFlight); i++)
    {
        if (pThis->aInFlight[i].pUrb)
        {
            cLeft--;
            pUrb = pThis->aInFlight[i].pUrb;
            if (pThis->aInFlight[i].fInactive
                && pUrb->enmState == VUSBURBSTATE_IN_FLIGHT
                && pUrb->enmType != VUSBXFERTYPE_CTRL)
                pThis->RootHub.pIRhConn->pfnCancelUrbsEp(pThis->RootHub.pIRhConn, pUrb);
        }
    }
    Assert(cLeft == 0);
}

/**
 * Generate a Start-Of-Frame event, and set a timer for End-Of-Frame.
 */
static void ohciStartOfFrame(POHCI pThis)
{
# ifdef LOG_ENABLED
    const uint32_t status_old = pThis->status;
# endif

    /*
     * Update HcFmRemaining.FRT and update start of frame time.
     */
    pThis->frt = pThis->fit;
    pThis->SofTime += pThis->cTicksPerFrame;

    /*
     * Check that the HCCA address isn't bogus. Linux 2.4.x is known to start
     * the bus with a hcca of 0 to work around problem with a specific controller.
     */
    bool fValidHCCA = !(    pThis->hcca >= OHCI_HCCA_MASK
                        ||  pThis->hcca < ~OHCI_HCCA_MASK);

# if 1
    /*
     * Update the HCCA.
     * Should be done after SOF but before HC read first ED in this frame.
     */
    if (fValidHCCA)
        ohciUpdateHCCA(pThis);
# endif

    /* "After writing to HCCA, HC will set SF in HcInterruptStatus" - guest isn't executing, so ignore the order! */
    ohciR3SetInterrupt(pThis, OHCI_INTR_START_OF_FRAME);

    if (pThis->fno)
    {
        ohciR3SetInterrupt(pThis, OHCI_INTR_FRAMENUMBER_OVERFLOW);
        pThis->fno = 0;
    }

    /* If the HCCA address is invalid, we're quitting here to avoid doing something which cannot be reported to the HCD. */
    if (!fValidHCCA)
    {
        Log(("ohciStartOfFrame: skipping hcca part because hcca=%RX32 (our 'valid' range: %RX32-%RX32)\n",
             pThis->hcca, ~OHCI_HCCA_MASK, OHCI_HCCA_MASK));
        return;
    }

    /*
     * Periodic EPs.
     */
    if (pThis->ctl & OHCI_CTL_PLE)
        ohciServicePeriodicList(pThis);

    /*
     * Control EPs.
     */
    if (    (pThis->ctl & OHCI_CTL_CLE)
        &&  (pThis->status & OHCI_STATUS_CLF) )
        ohciServiceCtrlList(pThis);

    /*
     * Bulk EPs.
     */
    if (    (pThis->ctl & OHCI_CTL_BLE)
        &&  (pThis->status & OHCI_STATUS_BLF))
        ohciServiceBulkList(pThis);
    else if ((pThis->status & OHCI_STATUS_BLF)
        &&    pThis->fBulkNeedsCleaning)
        ohciUndoBulkList(pThis);    /* If list disabled but not empty, abort endpoints. */

# if 0
    /*
     * Update the HCCA after processing the lists and everything. A bit experimental.
     *
     * ASSUME the guest won't be very upset if a TD is completed, retired and handed
     * back immediately. The idea is to be able to retire the data and/or status stages
     * of a control transfer together with the setup stage, thus saving a frame. This
     * behaviour is should be perfectly ok, since the setup (and maybe data) stages
     * have already taken at least one frame to complete.
     *
     * But, when implementing the first synchronous virtual USB devices, we'll have to
     * verify that the guest doesn't choke when having a TD returned in the same frame
     * as it was submitted.
     */
    ohciUpdateHCCA(pThis);
# endif

# ifdef LOG_ENABLED
    if (pThis->status ^ status_old)
    {
        uint32_t val = pThis->status;
        uint32_t chg = val ^ status_old; NOREF(chg);
        Log2(("ohciStartOfFrame: HcCommandStatus=%#010x: %sHCR=%d %sCLF=%d %sBLF=%d %sOCR=%d %sSOC=%d\n",
              val,
              chg & RT_BIT(0) ? "*" : "", val & 1,
              chg & RT_BIT(1) ? "*" : "", (val >> 1) & 1,
              chg & RT_BIT(2) ? "*" : "", (val >> 2) & 1,
              chg & RT_BIT(3) ? "*" : "", (val >> 3) & 1,
              chg & (3<<16)? "*" : "", (val >> 16) & 3));
    }
# endif
}

/**
 * Updates the HcFmNumber and FNO registers.
 */
static void bump_frame_number(POHCI pThis)
{
    const uint16_t u16OldFmNumber = pThis->HcFmNumber++;
    if ((u16OldFmNumber ^ pThis->HcFmNumber) & RT_BIT(15))
        pThis->fno = 1;
}

/**
 * Callback for periodic frame processing.
 */
static DECLCALLBACK(bool) ohciR3StartFrame(PVUSBIROOTHUBPORT pInterface, uint32_t u32FrameNo)
{
    RT_NOREF(u32FrameNo);
    POHCI pThis = VUSBIROOTHUBPORT_2_OHCI(pInterface);

    ohciR3Lock(pThis);

    /* Reset idle detection flag */
    pThis->fIdle = true;

# ifdef VBOX_WITH_OHCI_PHYS_READ_STATS
    physReadStatsReset(&g_PhysReadState);
# endif

    /* Frame boundary, so do EOF stuff here. */
    bump_frame_number(pThis);
    if ( (pThis->dqic != 0x7) && (pThis->dqic != 0))
        pThis->dqic--;

    /* Clean up any URBs that have been removed. */
    ohciCancelOrphanedURBs(pThis);

    /* Start the next frame. */
    ohciStartOfFrame(pThis);

# ifdef VBOX_WITH_OHCI_PHYS_READ_STATS
    physReadStatsPrint(&g_PhysReadState);
# endif

    ohciR3Unlock(pThis);
    return pThis->fIdle;
}

/** @interface_method_impl{VUSBIROOTHUBPORT,pfnFrameRateChanged} */
static DECLCALLBACK(void) ohciR3FrameRateChanged(PVUSBIROOTHUBPORT pInterface, uint32_t u32FrameRate)
{
    POHCI pThis = VUSBIROOTHUBPORT_2_OHCI(pInterface);

    Assert(u32FrameRate <= OHCI_DEFAULT_TIMER_FREQ);

    pThis->cTicksPerFrame = pThis->u64TimerHz / u32FrameRate;
    if (!pThis->cTicksPerFrame)
        pThis->cTicksPerFrame = 1;
    pThis->cTicksPerUsbTick = pThis->u64TimerHz >= VUSB_BUS_HZ ? pThis->u64TimerHz / VUSB_BUS_HZ : 1;
}

/**
 * Do frame processing on frame boundary
 */
static DECLCALLBACK(void) ohciFrameBoundaryTimer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF(pDevIns, pTimer, pvUser);
}

/**
 * Start sending SOF tokens across the USB bus, lists are processed in
 * next frame
 */
static void ohciBusStart(POHCI pThis)
{
    VUSBIDevPowerOn(pThis->RootHub.pIDev);
    pThis->dqic = 0x7;

    Log(("ohci: %s: Bus started\n", pThis->PciDev.pszNameR3));

    pThis->SofTime = PDMDevHlpTMTimeVirtGet(pThis->CTX_SUFF(pDevIns));
    int rc = pThis->RootHub.pIRhConn->pfnSetPeriodicFrameProcessing(pThis->RootHub.pIRhConn, OHCI_DEFAULT_TIMER_FREQ);
    AssertRC(rc);
}

/**
 * Stop sending SOF tokens on the bus
 */
static void ohciBusStop(POHCI pThis)
{
    int rc = pThis->RootHub.pIRhConn->pfnSetPeriodicFrameProcessing(pThis->RootHub.pIRhConn, 0);
    AssertRC(rc);
    VUSBIDevPowerOff(pThis->RootHub.pIDev);
}

/**
 * Move in to resume state
 */
static void ohciBusResume(POHCI pThis, bool fHardware)
{
    pThis->ctl &= ~OHCI_CTL_HCFS;
    pThis->ctl |= OHCI_USB_RESUME;

    Log(("pThis: ohciBusResume fHardware=%RTbool RWE=%s\n",
         fHardware, (pThis->ctl & OHCI_CTL_RWE) ? "on" : "off"));

    if (fHardware && (pThis->ctl & OHCI_CTL_RWE))
        ohciR3SetInterrupt(pThis, OHCI_INTR_RESUME_DETECT);

    ohciBusStart(pThis);
}


/* Power a port up or down */
static void rhport_power(POHCIROOTHUB pRh, unsigned iPort, bool fPowerUp)
{
    POHCIHUBPORT pPort = &pRh->aPorts[iPort];
    bool fOldPPS = !!(pPort->fReg & OHCI_PORT_PPS);

    LogFlowFunc(("iPort=%u fPowerUp=%RTbool\n", iPort, fPowerUp));

    if (fPowerUp)
    {
        /* power up */
        if (pPort->pDev)
            pPort->fReg |= OHCI_PORT_R_CURRENT_CONNECT_STATUS;
        if (pPort->fReg & OHCI_PORT_R_CURRENT_CONNECT_STATUS)
            pPort->fReg |= OHCI_PORT_R_POWER_STATUS;
        if (pPort->pDev && !fOldPPS)
            VUSBIDevPowerOn(pPort->pDev);
    }
    else
    {
        /* power down */
        pPort->fReg &= ~(  OHCI_PORT_R_POWER_STATUS
                         | OHCI_PORT_R_CURRENT_CONNECT_STATUS
                         | OHCI_PORT_R_SUSPEND_STATUS
                         | OHCI_PORT_R_RESET_STATUS);
        if (pPort->pDev && fOldPPS)
            VUSBIDevPowerOff(pPort->pDev);
    }
}

#endif /* IN_RING3 */

/**
 * Read the HcRevision register.
 */
static int HcRevision_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    RT_NOREF2(pThis, iReg);
    Log2(("HcRevision_r() -> 0x10\n"));
    *pu32Value = 0x10; /* OHCI revision 1.0, no emulation. */
    return VINF_SUCCESS;
}

/**
 * Write to the HcRevision register.
 */
static int HcRevision_w(POHCI pThis, uint32_t iReg, uint32_t u32Value)
{
    RT_NOREF3(pThis, iReg, u32Value);
    Log2(("HcRevision_w(%#010x) - denied\n", u32Value));
    AssertMsgFailed(("Invalid operation!!! u32Value=%#010x\n", u32Value));
    return VINF_SUCCESS;
}

/**
 * Read the HcControl register.
 */
static int HcControl_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    RT_NOREF1(iReg);
    uint32_t ctl = pThis->ctl;
    Log2(("HcControl_r -> %#010x - CBSR=%d PLE=%d IE=%d CLE=%d BLE=%d HCFS=%#x IR=%d RWC=%d RWE=%d\n",
          ctl, ctl & 3, (ctl >> 2) & 1, (ctl >> 3) & 1, (ctl >> 4) & 1, (ctl >> 5) & 1, (ctl >> 6) & 3, (ctl >> 8) & 1,
          (ctl >> 9) & 1, (ctl >> 10) & 1));
    *pu32Value = ctl;
    return VINF_SUCCESS;
}

/**
 * Write the HcControl register.
 */
static int HcControl_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF1(iReg);

    /* log it. */
    uint32_t chg = pThis->ctl ^ val; NOREF(chg);
    Log2(("HcControl_w(%#010x) => %sCBSR=%d %sPLE=%d %sIE=%d %sCLE=%d %sBLE=%d %sHCFS=%#x %sIR=%d %sRWC=%d %sRWE=%d\n",
          val,
          chg & 3       ? "*" : "",  val        & 3,
          chg & RT_BIT(2)  ? "*" : "", (val >>  2) & 1,
          chg & RT_BIT(3)  ? "*" : "", (val >>  3) & 1,
          chg & RT_BIT(4)  ? "*" : "", (val >>  4) & 1,
          chg & RT_BIT(5)  ? "*" : "", (val >>  5) & 1,
          chg & (3 << 6)? "*" : "", (val >>  6) & 3,
          chg & RT_BIT(8)  ? "*" : "", (val >>  8) & 1,
          chg & RT_BIT(9)  ? "*" : "", (val >>  9) & 1,
          chg & RT_BIT(10) ? "*" : "", (val >> 10) & 1));
    if (val & ~0x07ff)
        Log2(("Unknown bits %#x are set!!!\n", val & ~0x07ff));

    /* see what changed and take action on that. */
    uint32_t old_state = pThis->ctl & OHCI_CTL_HCFS;
    uint32_t new_state = val & OHCI_CTL_HCFS;

#ifdef IN_RING3
    pThis->ctl = val;
    if (new_state != old_state)
    {
        switch (new_state)
        {
            case OHCI_USB_OPERATIONAL:
                LogRel(("OHCI: USB Operational\n"));
                ohciBusStart(pThis);
                break;
            case OHCI_USB_SUSPEND:
                ohciBusStop(pThis);
                LogRel(("OHCI: USB Suspended\n"));
                break;
            case OHCI_USB_RESUME:
                LogRel(("OHCI: USB Resume\n"));
                ohciBusResume(pThis, false /* not hardware */);
                break;
            case OHCI_USB_RESET:
            {
                LogRel(("OHCI: USB Reset\n"));
                ohciBusStop(pThis);
                /** @todo This should probably do a real reset, but we don't implement
                 * that correctly in the roothub reset callback yet. check it's
                 * comments and argument for more details. */
                VUSBIDevReset(pThis->RootHub.pIDev, false /* don't do a real reset */, NULL, NULL, NULL);
                break;
            }
        }
    }
#else /* !IN_RING3 */
    if ( new_state != old_state )
    {
        Log2(("HcControl_w: state changed -> VINF_IOM_R3_MMIO_WRITE\n"));
        return VINF_IOM_R3_MMIO_WRITE;
    }
    pThis->ctl = val;
#endif /* !IN_RING3 */

    return VINF_SUCCESS;
}

/**
 * Read the HcCommandStatus register.
 */
static int HcCommandStatus_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    uint32_t status = pThis->status;
    Log2(("HcCommandStatus_r() -> %#010x - HCR=%d CLF=%d BLF=%d OCR=%d SOC=%d\n",
          status, status & 1, (status >> 1) & 1, (status >> 2) & 1, (status >> 3) & 1, (status >> 16) & 3));
    *pu32Value = status;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Write to the HcCommandStatus register.
 */
static int HcCommandStatus_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF1(iReg);

    /* log */
    uint32_t chg = pThis->status ^ val; NOREF(chg);
    Log2(("HcCommandStatus_w(%#010x) => %sHCR=%d %sCLF=%d %sBLF=%d %sOCR=%d %sSOC=%d\n",
          val,
          chg & RT_BIT(0) ? "*" : "", val & 1,
          chg & RT_BIT(1) ? "*" : "", (val >> 1) & 1,
          chg & RT_BIT(2) ? "*" : "", (val >> 2) & 1,
          chg & RT_BIT(3) ? "*" : "", (val >> 3) & 1,
          chg & (3<<16)? "!!!":"", (pThis->status >> 16) & 3));
    if (val & ~0x0003000f)
        Log2(("Unknown bits %#x are set!!!\n", val & ~0x0003000f));

    /* SOC is read-only */
    val = (val & ~OHCI_STATUS_SOC);

#ifdef IN_RING3
    /* "bits written as '0' remain unchanged in the register" */
    pThis->status |= val;
    if (pThis->status & OHCI_STATUS_HCR)
    {
        LogRel(("OHCI: Software reset\n"));
        ohciDoReset(pThis, OHCI_USB_SUSPEND, false /* N/A */);
    }
#else
    if ((pThis->status | val) & OHCI_STATUS_HCR)
    {
        LogFlow(("HcCommandStatus_w: reset -> VINF_IOM_R3_MMIO_WRITE\n"));
        return VINF_IOM_R3_MMIO_WRITE;
    }
    pThis->status |= val;
#endif
    return VINF_SUCCESS;
}

/**
 * Read the HcInterruptStatus register.
 */
static int HcInterruptStatus_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    uint32_t val = pThis->intr_status;
    Log2(("HcInterruptStatus_r() -> %#010x - SO=%d WDH=%d SF=%d RD=%d UE=%d FNO=%d RHSC=%d OC=%d\n",
          val, val & 1, (val >> 1) & 1, (val >> 2) & 1, (val >> 3) & 1, (val >> 4) & 1, (val >> 5) & 1,
          (val >> 6) & 1, (val >> 30) & 1));
    *pu32Value = val;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Write to the HcInterruptStatus register.
 */
static int HcInterruptStatus_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF1(iReg);

    uint32_t res = pThis->intr_status & ~val;
    uint32_t chg = pThis->intr_status ^ res; NOREF(chg);

    int rc = PDMCritSectEnter(&pThis->CsIrq, VINF_IOM_R3_MMIO_WRITE);
    if (rc != VINF_SUCCESS)
        return rc;

    Log2(("HcInterruptStatus_w(%#010x) => %sSO=%d %sWDH=%d %sSF=%d %sRD=%d %sUE=%d %sFNO=%d %sRHSC=%d %sOC=%d\n",
          val,
          chg & RT_BIT(0) ? "*" : "",  res       & 1,
          chg & RT_BIT(1) ? "*" : "", (res >> 1) & 1,
          chg & RT_BIT(2) ? "*" : "", (res >> 2) & 1,
          chg & RT_BIT(3) ? "*" : "", (res >> 3) & 1,
          chg & RT_BIT(4) ? "*" : "", (res >> 4) & 1,
          chg & RT_BIT(5) ? "*" : "", (res >> 5) & 1,
          chg & RT_BIT(6) ? "*" : "", (res >> 6) & 1,
          chg & RT_BIT(30)? "*" : "", (res >> 30) & 1));
    if (    (val & ~0xc000007f)
        &&  val != 0xffffffff /* ignore clear-all-like requests from xp. */)
        Log2(("Unknown bits %#x are set!!!\n", val & ~0xc000007f));

    /* "The Host Controller Driver may clear specific bits in this
     * register by writing '1' to bit positions to be cleared"
     */
    pThis->intr_status &= ~val;
    ohciUpdateInterruptLocked(pThis, "HcInterruptStatus_w");
    PDMCritSectLeave(&pThis->CsIrq);
    return VINF_SUCCESS;
}

/**
 * Read the HcInterruptEnable register
 */
static int HcInterruptEnable_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    uint32_t val = pThis->intr;
    Log2(("HcInterruptEnable_r() -> %#010x - SO=%d WDH=%d SF=%d RD=%d UE=%d FNO=%d RHSC=%d OC=%d MIE=%d\n",
          val, val & 1, (val >> 1) & 1, (val >> 2) & 1, (val >> 3) & 1, (val >> 4) & 1, (val >> 5) & 1,
          (val >> 6) & 1, (val >> 30) & 1, (val >> 31) & 1));
    *pu32Value = val;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Writes to the HcInterruptEnable register.
 */
static int HcInterruptEnable_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF1(iReg);
    uint32_t res = pThis->intr | val;
    uint32_t chg = pThis->intr ^ res; NOREF(chg);

    int rc = PDMCritSectEnter(&pThis->CsIrq, VINF_IOM_R3_MMIO_WRITE);
    if (rc != VINF_SUCCESS)
        return rc;

    Log2(("HcInterruptEnable_w(%#010x) => %sSO=%d %sWDH=%d %sSF=%d %sRD=%d %sUE=%d %sFNO=%d %sRHSC=%d %sOC=%d %sMIE=%d\n",
          val,
          chg & RT_BIT(0)  ? "*" : "",  res        & 1,
          chg & RT_BIT(1)  ? "*" : "", (res >>  1) & 1,
          chg & RT_BIT(2)  ? "*" : "", (res >>  2) & 1,
          chg & RT_BIT(3)  ? "*" : "", (res >>  3) & 1,
          chg & RT_BIT(4)  ? "*" : "", (res >>  4) & 1,
          chg & RT_BIT(5)  ? "*" : "", (res >>  5) & 1,
          chg & RT_BIT(6)  ? "*" : "", (res >>  6) & 1,
          chg & RT_BIT(30) ? "*" : "", (res >> 30) & 1,
          chg & RT_BIT(31) ? "*" : "", (res >> 31) & 1));
    if (val & ~0xc000007f)
        Log2(("Uknown bits %#x are set!!!\n", val & ~0xc000007f));

    pThis->intr |= val;
    ohciUpdateInterruptLocked(pThis, "HcInterruptEnable_w");
    PDMCritSectLeave(&pThis->CsIrq);
    return VINF_SUCCESS;
}

/**
 * Reads the HcInterruptDisable register.
 */
static int HcInterruptDisable_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
#if 1 /** @todo r=bird: "On read, the current value of the HcInterruptEnable register is returned." */
    uint32_t val = pThis->intr;
#else /* old code. */
    uint32_t val = ~pThis->intr;
#endif
    Log2(("HcInterruptDisable_r() -> %#010x - SO=%d WDH=%d SF=%d RD=%d UE=%d FNO=%d RHSC=%d OC=%d MIE=%d\n",
          val, val & 1, (val >> 1) & 1, (val >> 2) & 1, (val >> 3) & 1, (val >> 4) & 1, (val >> 5) & 1,
          (val >> 6) & 1, (val >> 30) & 1, (val >> 31) & 1));

    *pu32Value = val;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Writes to the HcInterruptDisable register.
 */
static int HcInterruptDisable_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF1(iReg);
    uint32_t res = pThis->intr & ~val;
    uint32_t chg = pThis->intr ^ res; NOREF(chg);

    int rc = PDMCritSectEnter(&pThis->CsIrq, VINF_IOM_R3_MMIO_WRITE);
    if (rc != VINF_SUCCESS)
        return rc;

    Log2(("HcInterruptDisable_w(%#010x) => %sSO=%d %sWDH=%d %sSF=%d %sRD=%d %sUE=%d %sFNO=%d %sRHSC=%d %sOC=%d %sMIE=%d\n",
          val,
          chg & RT_BIT(0)  ? "*" : "",  res        & 1,
          chg & RT_BIT(1)  ? "*" : "", (res >>  1) & 1,
          chg & RT_BIT(2)  ? "*" : "", (res >>  2) & 1,
          chg & RT_BIT(3)  ? "*" : "", (res >>  3) & 1,
          chg & RT_BIT(4)  ? "*" : "", (res >>  4) & 1,
          chg & RT_BIT(5)  ? "*" : "", (res >>  5) & 1,
          chg & RT_BIT(6)  ? "*" : "", (res >>  6) & 1,
          chg & RT_BIT(30) ? "*" : "", (res >> 30) & 1,
          chg & RT_BIT(31) ? "*" : "", (res >> 31) & 1));
    /* Don't bitch about invalid bits here since it makes sense to disable
     * interrupts you don't know about. */

    pThis->intr &= ~val;
    ohciUpdateInterruptLocked(pThis, "HcInterruptDisable_w");
    PDMCritSectLeave(&pThis->CsIrq);
    return VINF_SUCCESS;
}

/**
 * Read the HcHCCA register (Host Controller Communications Area physical address).
 */
static int HcHCCA_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    Log2(("HcHCCA_r() -> %#010x\n", pThis->hcca));
    *pu32Value = pThis->hcca;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Write to the HcHCCA register (Host Controller Communications Area physical address).
 */
static int HcHCCA_w(POHCI pThis, uint32_t iReg, uint32_t Value)
{
    Log2(("HcHCCA_w(%#010x) - old=%#010x new=%#010x\n", Value, pThis->hcca, Value & OHCI_HCCA_MASK));
    pThis->hcca = Value & OHCI_HCCA_MASK;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Read the HcPeriodCurrentED register.
 */
static int HcPeriodCurrentED_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    Log2(("HcPeriodCurrentED_r() -> %#010x\n", pThis->per_cur));
    *pu32Value = pThis->per_cur;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Write to the HcPeriodCurrentED register.
 */
static int HcPeriodCurrentED_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    Log(("HcPeriodCurrentED_w(%#010x) - old=%#010x new=%#010x (This is a read only register, only the linux guys don't respect that!)\n",
         val, pThis->per_cur, val & ~7));
    //AssertMsgFailed(("HCD (Host Controller Driver) should not write to HcPeriodCurrentED! val=%#010x (old=%#010x)\n", val, pThis->per_cur));
    AssertMsg(!(val & 7), ("Invalid alignment, val=%#010x\n", val));
    pThis->per_cur = val & ~7;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Read the HcControlHeadED register.
 */
static int HcControlHeadED_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    Log2(("HcControlHeadED_r() -> %#010x\n", pThis->ctrl_head));
    *pu32Value = pThis->ctrl_head;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Write to the HcControlHeadED register.
 */
static int HcControlHeadED_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    Log2(("HcControlHeadED_w(%#010x) - old=%#010x new=%#010x\n", val, pThis->ctrl_head, val & ~7));
    AssertMsg(!(val & 7), ("Invalid alignment, val=%#010x\n", val));
    pThis->ctrl_head = val & ~7;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Read the HcControlCurrentED register.
 */
static int HcControlCurrentED_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    Log2(("HcControlCurrentED_r() -> %#010x\n", pThis->ctrl_cur));
    *pu32Value = pThis->ctrl_cur;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Write to the HcControlCurrentED register.
 */
static int HcControlCurrentED_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    Log2(("HcControlCurrentED_w(%#010x) - old=%#010x new=%#010x\n", val, pThis->ctrl_cur, val & ~7));
    AssertMsg(!(pThis->ctl & OHCI_CTL_CLE), ("Illegal write! HcControl.ControlListEnabled is set! val=%#010x\n", val));
    AssertMsg(!(val & 7), ("Invalid alignment, val=%#010x\n", val));
    pThis->ctrl_cur = val & ~7;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Read the HcBulkHeadED register.
 */
static int HcBulkHeadED_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    Log2(("HcBulkHeadED_r() -> %#010x\n", pThis->bulk_head));
    *pu32Value = pThis->bulk_head;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Write to the HcBulkHeadED register.
 */
static int HcBulkHeadED_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    Log2(("HcBulkHeadED_w(%#010x) - old=%#010x new=%#010x\n", val, pThis->bulk_head, val & ~7));
    AssertMsg(!(val & 7), ("Invalid alignment, val=%#010x\n", val));
    pThis->bulk_head = val & ~7; /** @todo The ATI OHCI controller on my machine enforces 16-byte address alignment. */
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Read the HcBulkCurrentED register.
 */
static int HcBulkCurrentED_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    Log2(("HcBulkCurrentED_r() -> %#010x\n", pThis->bulk_cur));
    *pu32Value = pThis->bulk_cur;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Write to the HcBulkCurrentED register.
 */
static int HcBulkCurrentED_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    Log2(("HcBulkCurrentED_w(%#010x) - old=%#010x new=%#010x\n", val, pThis->bulk_cur, val & ~7));
    AssertMsg(!(pThis->ctl & OHCI_CTL_BLE), ("Illegal write! HcControl.BulkListEnabled is set! val=%#010x\n", val));
    AssertMsg(!(val & 7), ("Invalid alignment, val=%#010x\n", val));
    pThis->bulk_cur = val & ~7;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}


/**
 * Read the HcDoneHead register.
 */
static int HcDoneHead_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    Log2(("HcDoneHead_r() -> 0x%#08x\n", pThis->done));
    *pu32Value = pThis->done;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Write to the HcDoneHead register.
 */
static int HcDoneHead_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF3(pThis, iReg, val);
    Log2(("HcDoneHead_w(0x%#08x) - denied!!!\n", val));
    /*AssertMsgFailed(("Illegal operation!!! val=%#010x\n", val)); - OS/2 does this */
    return VINF_SUCCESS;
}


/**
 * Read the HcFmInterval (Fm=Frame) register.
 */
static int HcFmInterval_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    uint32_t val = (pThis->fit << 31) | (pThis->fsmps << 16) | (pThis->fi);
    Log2(("HcFmInterval_r() -> 0x%#08x - FI=%d FSMPS=%d FIT=%d\n",
          val, val & 0x3fff, (val >> 16) & 0x7fff, val >> 31));
    *pu32Value = val;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Write to the HcFmInterval (Fm = Frame) register.
 */
static int HcFmInterval_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF1(iReg);

    /* log */
    uint32_t chg = val ^ ((pThis->fit << 31) | (pThis->fsmps << 16) | pThis->fi); NOREF(chg);
    Log2(("HcFmInterval_w(%#010x) => %sFI=%d %sFSMPS=%d %sFIT=%d\n",
          val,
          chg & 0x00003fff ? "*" : "",  val        & 0x3fff,
          chg & 0x7fff0000 ? "*" : "", (val >> 16) & 0x7fff,
          chg >> 31        ? "*" : "", (val >> 31) & 1));
    if ( pThis->fi != (val & OHCI_FMI_FI) )
    {
        Log(("ohci: FrameInterval: %#010x -> %#010x\n", pThis->fi, val & OHCI_FMI_FI));
        AssertMsg(pThis->fit != ((val >> OHCI_FMI_FIT_SHIFT) & 1), ("HCD didn't toggle the FIT bit!!!\n"));
    }

    /* update */
    pThis->fi = val & OHCI_FMI_FI;
    pThis->fit = (val & OHCI_FMI_FIT) >> OHCI_FMI_FIT_SHIFT;
    pThis->fsmps = (val & OHCI_FMI_FSMPS) >> OHCI_FMI_FSMPS_SHIFT;
    return VINF_SUCCESS;
}

/**
 * Read the HcFmRemaining (Fm = Frame) register.
 */
static int HcFmRemaining_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    RT_NOREF1(iReg);
    uint32_t Value = pThis->frt << 31;
    if ((pThis->ctl & OHCI_CTL_HCFS) == OHCI_USB_OPERATIONAL)
    {
        /*
         * Being in USB operational state guarantees SofTime was set already.
         */
        uint64_t tks = PDMDevHlpTMTimeVirtGet(pThis->CTX_SUFF(pDevIns)) - pThis->SofTime;
        if (tks < pThis->cTicksPerFrame)  /* avoid muldiv if possible */
        {
            uint16_t fr;
            tks = ASMMultU64ByU32DivByU32(1, tks, pThis->cTicksPerUsbTick);
            fr = (uint16_t)(pThis->fi - tks);
            Value |= fr;
        }
    }

    Log2(("HcFmRemaining_r() -> %#010x - FR=%d FRT=%d\n", Value, Value & 0x3fff, Value >> 31));
    *pu32Value = Value;
    return VINF_SUCCESS;
}

/**
 * Write to the HcFmRemaining (Fm = Frame) register.
 */
static int HcFmRemaining_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF3(pThis, iReg, val);
    Log2(("HcFmRemaining_w(%#010x) - denied\n", val));
    AssertMsgFailed(("Invalid operation!!! val=%#010x\n", val));
    return VINF_SUCCESS;
}

/**
 * Read the HcFmNumber (Fm = Frame) register.
 */
static int HcFmNumber_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    RT_NOREF1(iReg);
    uint32_t val = (uint16_t)pThis->HcFmNumber;
    Log2(("HcFmNumber_r() -> %#010x - FN=%#x(%d) (32-bit=%#x(%d))\n", val, val, val, pThis->HcFmNumber, pThis->HcFmNumber));
    *pu32Value = val;
    return VINF_SUCCESS;
}

/**
 * Write to the HcFmNumber (Fm = Frame) register.
 */
static int HcFmNumber_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF3(pThis, iReg, val);
    Log2(("HcFmNumber_w(%#010x) - denied\n", val));
    AssertMsgFailed(("Invalid operation!!! val=%#010x\n", val));
    return VINF_SUCCESS;
}

/**
 * Read the HcPeriodicStart register.
 * The register determines when in a frame to switch from control&bulk to periodic lists.
 */
static int HcPeriodicStart_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    RT_NOREF1(iReg);
    Log2(("HcPeriodicStart_r() -> %#010x - PS=%d\n", pThis->pstart, pThis->pstart & 0x3fff));
    *pu32Value = pThis->pstart;
    return VINF_SUCCESS;
}

/**
 * Write to the HcPeriodicStart register.
 * The register determines when in a frame to switch from control&bulk to periodic lists.
 */
static int HcPeriodicStart_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF1(iReg);
    Log2(("HcPeriodicStart_w(%#010x) => PS=%d\n", val, val & 0x3fff));
    if (val & ~0x3fff)
        Log2(("Unknown bits %#x are set!!!\n", val & ~0x3fff));
    pThis->pstart = val; /** @todo r=bird: should we support setting the other bits? */
    return VINF_SUCCESS;
}

/**
 * Read the HcLSThreshold register.
 */
static int HcLSThreshold_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    RT_NOREF2(pThis, iReg);
    Log2(("HcLSThreshold_r() -> %#010x\n", OHCI_LS_THRESH));
    *pu32Value = OHCI_LS_THRESH;
    return VINF_SUCCESS;
}

/**
 * Write to the HcLSThreshold register.
 *
 * Docs are inconsistent here:
 *
 *      "Neither the Host Controller nor the Host Controller Driver are allowed to change this value."
 *
 *      "This value is calculated by HCD with the consideration of transmission and setup overhead."
 *
 *      The register is marked "R/W" the HCD column.
 *
 */
static int HcLSThreshold_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF3(pThis, iReg, val);
    Log2(("HcLSThreshold_w(%#010x) => LST=0x%03x(%d)\n", val, val & 0x0fff, val & 0x0fff));
    AssertMsg(val == OHCI_LS_THRESH,
              ("HCD tried to write bad LS threshold: 0x%x (see function header)\n", val));
    /** @todo the HCD can change this. */
    return VINF_SUCCESS;
}

/**
 * Read the HcRhDescriptorA register.
 */
static int HcRhDescriptorA_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    RT_NOREF1(iReg);
    uint32_t val = pThis->RootHub.desc_a;
#if 0 /* annoying */
    Log2(("HcRhDescriptorA_r() -> %#010x - NDP=%d PSM=%d NPS=%d DT=%d OCPM=%d NOCP=%d POTGT=%#x\n",
          val, val & 0xff, (val >> 8) & 1, (val >> 9) & 1, (val >> 10) & 1, (val >> 11) & 1,
          (val >> 12) & 1, (val >> 24) & 0xff));
#endif
    *pu32Value = val;
    return VINF_SUCCESS;
}

/**
 * Write to the HcRhDescriptorA register.
 */
static int HcRhDescriptorA_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF1(iReg);
    uint32_t chg = val ^ pThis->RootHub.desc_a; NOREF(chg);
    Log2(("HcRhDescriptorA_w(%#010x) => %sNDP=%d %sPSM=%d %sNPS=%d %sDT=%d %sOCPM=%d %sNOCP=%d %sPOTGT=%#x - %sPowerSwitching Set%sPower\n",
          val,
          chg & 0xff      ?"!!!": "", val & 0xff,
          (chg >>  8) & 1 ? "*" : "", (val >>  8) & 1,
          (chg >>  9) & 1 ? "*" : "", (val >>  9) & 1,
          (chg >> 10) & 1 ?"!!!": "", 0,
          (chg >> 11) & 1 ? "*" : "", (val >> 11) & 1,
          (chg >> 12) & 1 ? "*" : "", (val >> 12) & 1,
          (chg >> 24)&0xff? "*" : "", (val >> 24) & 0xff,
          val & OHCI_RHA_NPS ? "No"   : "",
          val & OHCI_RHA_PSM ? "Port" : "Global"));
    if (val & ~0xff001fff)
        Log2(("Unknown bits %#x are set!!!\n", val & ~0xff001fff));


    if ((val & (OHCI_RHA_NDP | OHCI_RHA_DT)) != OHCI_NDP_CFG(pThis))
    {
        Log(("ohci: %s: invalid write to NDP or DT in roothub descriptor A!!! val=0x%.8x\n",
             pThis->PciDev.pszNameR3, val));
        val &= ~(OHCI_RHA_NDP | OHCI_RHA_DT);
        val |= OHCI_NDP_CFG(pThis);
    }

    pThis->RootHub.desc_a = val;
    return VINF_SUCCESS;
}

/**
 * Read the HcRhDescriptorB register.
 */
static int HcRhDescriptorB_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    uint32_t val = pThis->RootHub.desc_b;
    Log2(("HcRhDescriptorB_r() -> %#010x - DR=0x%04x PPCM=0x%04x\n",
          val, val & 0xffff, val >> 16));
    *pu32Value = val;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Write to the HcRhDescriptorB register.
 */
static int HcRhDescriptorB_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
    RT_NOREF1(iReg);
    uint32_t chg = pThis->RootHub.desc_b ^ val; NOREF(chg);
    Log2(("HcRhDescriptorB_w(%#010x) => %sDR=0x%04x %sPPCM=0x%04x\n",
          val,
          chg & 0xffff ? "!!!" : "", val & 0xffff,
          chg >> 16    ? "!!!" : "", val >> 16));

    if ( pThis->RootHub.desc_b != val )
        Log(("ohci: %s: unsupported write to root descriptor B!!! 0x%.8x -> 0x%.8x\n",
             pThis->PciDev.pszNameR3, pThis->RootHub.desc_b, val));
    pThis->RootHub.desc_b = val;
    return VINF_SUCCESS;
}

/**
 * Read the HcRhStatus (Rh = Root Hub) register.
 */
static int HcRhStatus_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    uint32_t val = pThis->RootHub.status;
    if (val & (OHCI_RHS_LPSC | OHCI_RHS_OCIC))
        Log2(("HcRhStatus_r() -> %#010x - LPS=%d OCI=%d DRWE=%d LPSC=%d OCIC=%d CRWE=%d\n",
              val, val & 1, (val >> 1) & 1, (val >> 15) & 1, (val >> 16) & 1, (val >> 17) & 1, (val >> 31) & 1));
    *pu32Value = val;
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
}

/**
 * Write to the HcRhStatus (Rh = Root Hub) register.
 */
static int HcRhStatus_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
#ifdef IN_RING3
    /* log */
    uint32_t old = pThis->RootHub.status;
    uint32_t chg;
    if (val & ~0x80038003)
        Log2(("HcRhStatus_w: Unknown bits %#x are set!!!\n", val & ~0x80038003));
    if ( (val & OHCI_RHS_LPSC) && (val & OHCI_RHS_LPS) )
        Log2(("HcRhStatus_w: Warning both CGP and SGP are set! (Clear/Set Global Power)\n"));
    if ( (val & OHCI_RHS_DRWE) && (val & OHCI_RHS_CRWE) )
        Log2(("HcRhStatus_w: Warning both CRWE and SRWE are set! (Clear/Set Remote Wakeup Enable)\n"));


    /* write 1 to clear OCIC */
    if ( val & OHCI_RHS_OCIC )
        pThis->RootHub.status &= ~OHCI_RHS_OCIC;

    /* SetGlobalPower */
    if ( val & OHCI_RHS_LPSC )
    {
        unsigned i;
        Log2(("ohci: %s: global power up\n", pThis->PciDev.pszNameR3));
        for (i = 0; i < OHCI_NDP_CFG(pThis); i++)
            rhport_power(&pThis->RootHub, i, true /* power up */);
    }

    /* ClearGlobalPower */
    if ( val & OHCI_RHS_LPS )
    {
        unsigned i;
        Log2(("ohci: %s: global power down\n", pThis->PciDev.pszNameR3));
        for (i = 0; i < OHCI_NDP_CFG(pThis); i++)
            rhport_power(&pThis->RootHub, i, false /* power down */);
    }

    if ( val & OHCI_RHS_DRWE )
        pThis->RootHub.status |= OHCI_RHS_DRWE;

    if ( val & OHCI_RHS_CRWE )
        pThis->RootHub.status &= ~OHCI_RHS_DRWE;

    chg = pThis->RootHub.status ^ old;
    Log2(("HcRhStatus_w(%#010x) => %sCGP=%d %sOCI=%d %sSRWE=%d %sSGP=%d %sOCIC=%d %sCRWE=%d\n",
          val,
           chg        & 1 ? "*" : "", val        & 1,
          (chg >>  1) & 1 ?"!!!": "", (val >>  1) & 1,
          (chg >> 15) & 1 ? "*" : "", (val >> 15) & 1,
          (chg >> 16) & 1 ? "*" : "", (val >> 16) & 1,
          (chg >> 17) & 1 ? "*" : "", (val >> 17) & 1,
          (chg >> 31) & 1 ? "*" : "", (val >> 31) & 1));
    RT_NOREF1(iReg);
    return VINF_SUCCESS;
#else  /* !IN_RING3 */
    RT_NOREF3(pThis, iReg, val);
    return VINF_IOM_R3_MMIO_WRITE;
#endif /* !IN_RING3 */
}

/**
 * Read the HcRhPortStatus register of a port.
 */
static int HcRhPortStatus_r(PCOHCI pThis, uint32_t iReg, uint32_t *pu32Value)
{
    const unsigned i = iReg - 21;
    uint32_t val = pThis->RootHub.aPorts[i].fReg | OHCI_PORT_R_POWER_STATUS; /* PortPowerStatus: see todo on power in _w function. */
    if (val & OHCI_PORT_R_RESET_STATUS)
    {
#ifdef IN_RING3
        RTThreadYield();
#else
        Log2(("HcRhPortStatus_r: yield -> VINF_IOM_R3_MMIO_READ\n"));
        return VINF_IOM_R3_MMIO_READ;
#endif
    }
    if (val & (OHCI_PORT_R_RESET_STATUS | OHCI_PORT_CSC | OHCI_PORT_PESC | OHCI_PORT_PSSC | OHCI_PORT_OCIC | OHCI_PORT_PRSC))
        Log2(("HcRhPortStatus_r(): port %u: -> %#010x - CCS=%d PES=%d PSS=%d POCI=%d RRS=%d PPS=%d LSDA=%d CSC=%d PESC=%d PSSC=%d OCIC=%d PRSC=%d\n",
              i, val, val & 1, (val >> 1) & 1, (val >> 2) & 1, (val >> 3) & 1, (val >> 4) & 1, (val >> 8) & 1, (val >> 9) & 1,
              (val >> 16) & 1, (val >> 17) & 1, (val >> 18) & 1, (val >> 19) & 1, (val >> 20) & 1));
    *pu32Value = val;
    return VINF_SUCCESS;
}

#ifdef IN_RING3
/**
 * Completion callback for the vusb_dev_reset() operation.
 * @thread EMT.
 */
static DECLCALLBACK(void) uchi_port_reset_done(PVUSBIDEVICE pDev, int rc, void *pvUser)
{
    POHCI pThis = (POHCI)pvUser;

    /*
     * Find the port in question
     */
    POHCIHUBPORT pPort = NULL;
    unsigned iPort;
    for (iPort = 0; iPort < OHCI_NDP_CFG(pThis); iPort++) /* lazy bird */
        if (pThis->RootHub.aPorts[iPort].pDev == pDev)
        {
            pPort = &pThis->RootHub.aPorts[iPort];
            break;
        }
    if (!pPort)
    {
        Assert(pPort); /* sometimes happens because of @bugref{1510} */
        return;
    }

    if (RT_SUCCESS(rc))
    {
        /*
         * Successful reset.
         */
        Log2(("uchi_port_reset_done: Reset completed.\n"));
        pPort->fReg &= ~(OHCI_PORT_R_RESET_STATUS | OHCI_PORT_R_SUSPEND_STATUS | OHCI_PORT_R_SUSPEND_STATUS_CHANGE);
        pPort->fReg |= OHCI_PORT_R_ENABLE_STATUS | OHCI_PORT_R_RESET_STATUS_CHANGE;
    }
    else
    {
        /* desperate measures. */
        if (    pPort->pDev
            &&  VUSBIDevGetState(pPort->pDev) == VUSB_DEVICE_STATE_ATTACHED)
        {
            /*
             * Damn, something weird happened during reset. We'll pretend the user did an
             * incredible fast reconnect or something. (probably not gonna work)
             */
            Log2(("uchi_port_reset_done: The reset failed (rc=%Rrc)!!! Pretending reconnect at the speed of light.\n", rc));
            pPort->fReg = OHCI_PORT_R_CURRENT_CONNECT_STATUS | OHCI_PORT_R_CONNECT_STATUS_CHANGE;
        }
        else
        {
            /*
             * The device have / will be disconnected.
             */
            Log2(("uchi_port_reset_done: Disconnected (rc=%Rrc)!!!\n", rc));
            pPort->fReg &= ~(OHCI_PORT_R_RESET_STATUS | OHCI_PORT_R_SUSPEND_STATUS | OHCI_PORT_R_SUSPEND_STATUS_CHANGE | OHCI_PORT_R_RESET_STATUS_CHANGE);
            pPort->fReg |= OHCI_PORT_R_CONNECT_STATUS_CHANGE;
        }
    }

    /* Raise roothub status change interrupt. */
    ohciR3SetInterrupt(pThis, OHCI_INTR_ROOT_HUB_STATUS_CHANGE);
}

/**
 * Sets a flag in a port status register but only set it if a device is
 * connected, if not set ConnectStatusChange flag to force HCD to reevaluate
 * connect status.
 *
 * @returns true if device was connected and the flag was cleared.
 */
static bool rhport_set_if_connected(POHCIROOTHUB pRh, int iPort, uint32_t fValue)
{
    /*
     * Writing a 0 has no effect
     */
    if (fValue == 0)
        return false;

    /*
     * If CurrentConnectStatus is cleared we set ConnectStatusChange.
     */
    if (!(pRh->aPorts[iPort].fReg & OHCI_PORT_R_CURRENT_CONNECT_STATUS))
    {
        pRh->aPorts[iPort].fReg |= OHCI_PORT_R_CONNECT_STATUS_CHANGE;
        ohciR3SetInterrupt(pRh->pOhci, OHCI_INTR_ROOT_HUB_STATUS_CHANGE);
        return false;
    }

    bool fRc = !(pRh->aPorts[iPort].fReg & fValue);

    /* set the bit */
    pRh->aPorts[iPort].fReg |= fValue;

    return fRc;
}
#endif /* IN_RING3 */

/**
 * Write to the HcRhPortStatus register of a port.
 */
static int HcRhPortStatus_w(POHCI pThis, uint32_t iReg, uint32_t val)
{
#ifdef IN_RING3
    const unsigned  i = iReg - 21;
    POHCIHUBPORT    p = &pThis->RootHub.aPorts[i];
    uint32_t        old_state = p->fReg;

# ifdef LOG_ENABLED
    /*
     * Log it.
     */
    static const char *apszCmdNames[32] =
    {
        "ClearPortEnable",      "SetPortEnable",    "SetPortSuspend",   "!!!ClearSuspendStatus",
        "SetPortReset",         "!!!5",             "!!!6",             "!!!7",
        "SetPortPower",         "ClearPortPower",   "!!!10",            "!!!11",
        "!!!12",                "!!!13",            "!!!14",            "!!!15",
        "ClearCSC",             "ClearPESC",        "ClearPSSC",        "ClearOCIC",
        "ClearPRSC",            "!!!21",            "!!!22",            "!!!23",
        "!!!24",                "!!!25",            "!!!26",            "!!!27",
        "!!!28",                "!!!29",            "!!!30",            "!!!31"
    };
    Log2(("HcRhPortStatus_w(%#010x): port %u:", val, i));
    for (unsigned j = 0; j < RT_ELEMENTS(apszCmdNames); j++)
        if (val & (1 << j))
            Log2((" %s", apszCmdNames[j]));
    Log2(("\n"));
# endif

    /* Write to clear any of the change bits: CSC, PESC, PSSC, OCIC and PRSC */
    if (val & OHCI_PORT_W_CLEAR_CHANGE_MASK)
        p->fReg &= ~(val & OHCI_PORT_W_CLEAR_CHANGE_MASK);

    if (val & OHCI_PORT_W_CLEAR_ENABLE)
    {
        p->fReg &= ~OHCI_PORT_R_ENABLE_STATUS;
        Log2(("HcRhPortStatus_w(): port %u: DISABLE\n", i));
    }

    if (rhport_set_if_connected(&pThis->RootHub, i, val & OHCI_PORT_W_SET_ENABLE))
        Log2(("HcRhPortStatus_w(): port %u: ENABLE\n", i));

    if (rhport_set_if_connected(&pThis->RootHub, i, val & OHCI_PORT_W_SET_SUSPEND))
        Log2(("HcRhPortStatus_w(): port %u: SUSPEND - not implemented correctly!!!\n", i));

    if (val & OHCI_PORT_W_SET_RESET)
    {
        if (rhport_set_if_connected(&pThis->RootHub, i, val & OHCI_PORT_W_SET_RESET))
        {
            PVM pVM = PDMDevHlpGetVM(pThis->CTX_SUFF(pDevIns));
            p->fReg &= ~OHCI_PORT_R_RESET_STATUS_CHANGE;
            VUSBIDevReset(p->pDev, false /* don't reset on linux */, uchi_port_reset_done, pThis, pVM);
        }
        else if (p->fReg & OHCI_PORT_R_RESET_STATUS)
        {
            /* the guest is getting impatient. */
            Log2(("HcRhPortStatus_w(): port %u: Impatient guest!\n", i));
            RTThreadYield();
        }
    }

    if (!(pThis->RootHub.desc_a & OHCI_RHA_NPS))
    {
        /** @todo To implement per-device power-switching
         * we need to check PortPowerControlMask to make
         * sure it isn't gang powered
         */
        if (val & OHCI_PORT_W_CLEAR_POWER)
            rhport_power(&pThis->RootHub, i, false /* power down */);
        if (val & OHCI_PORT_W_SET_POWER)
            rhport_power(&pThis->RootHub, i, true /* power up */);
    }

    /** @todo r=frank:  ClearSuspendStatus. Timing? */
    if (val & OHCI_PORT_W_CLEAR_SUSPEND_STATUS)
    {
        rhport_power(&pThis->RootHub, i, true /* power up */);
        pThis->RootHub.aPorts[i].fReg &= ~OHCI_PORT_R_SUSPEND_STATUS;
        pThis->RootHub.aPorts[i].fReg |= OHCI_PORT_R_SUSPEND_STATUS_CHANGE;
        ohciR3SetInterrupt(pThis, OHCI_INTR_ROOT_HUB_STATUS_CHANGE);
    }

    if (p->fReg != old_state)
    {
        uint32_t res = p->fReg;
        uint32_t chg = res ^ old_state; NOREF(chg);
        Log2(("HcRhPortStatus_w(%#010x): port %u: => %sCCS=%d %sPES=%d %sPSS=%d %sPOCI=%d %sRRS=%d %sPPS=%d %sLSDA=%d %sCSC=%d %sPESC=%d %sPSSC=%d %sOCIC=%d %sPRSC=%d\n",
              val, i,
              chg         & 1 ? "*" : "",  res        & 1,
              (chg >>  1) & 1 ? "*" : "", (res >>  1) & 1,
              (chg >>  2) & 1 ? "*" : "", (res >>  2) & 1,
              (chg >>  3) & 1 ? "*" : "", (res >>  3) & 1,
              (chg >>  4) & 1 ? "*" : "", (res >>  4) & 1,
              (chg >>  8) & 1 ? "*" : "", (res >>  8) & 1,
              (chg >>  9) & 1 ? "*" : "", (res >>  9) & 1,
              (chg >> 16) & 1 ? "*" : "", (res >> 16) & 1,
              (chg >> 17) & 1 ? "*" : "", (res >> 17) & 1,
              (chg >> 18) & 1 ? "*" : "", (res >> 18) & 1,
              (chg >> 19) & 1 ? "*" : "", (res >> 19) & 1,
              (chg >> 20) & 1 ? "*" : "", (res >> 20) & 1));
    }
    return VINF_SUCCESS;
#else /* !IN_RING3 */
    RT_NOREF3(pThis, iReg, val);
    return VINF_IOM_R3_MMIO_WRITE;
#endif /* !IN_RING3 */
}

/**
 * Register descriptor table
 */
static const OHCIOPREG g_aOpRegs[] =
{
    { "HcRevision",          HcRevision_r,           HcRevision_w },            /*  0 */
    { "HcControl",           HcControl_r,            HcControl_w },             /*  1 */
    { "HcCommandStatus",     HcCommandStatus_r,      HcCommandStatus_w },       /*  2 */
    { "HcInterruptStatus",   HcInterruptStatus_r,    HcInterruptStatus_w },     /*  3 */
    { "HcInterruptEnable",   HcInterruptEnable_r,    HcInterruptEnable_w },     /*  4 */
    { "HcInterruptDisable",  HcInterruptDisable_r,   HcInterruptDisable_w },    /*  5 */
    { "HcHCCA",              HcHCCA_r,               HcHCCA_w },                /*  6 */
    { "HcPeriodCurrentED",   HcPeriodCurrentED_r,    HcPeriodCurrentED_w },     /*  7 */
    { "HcControlHeadED",     HcControlHeadED_r,      HcControlHeadED_w },       /*  8 */
    { "HcControlCurrentED",  HcControlCurrentED_r,   HcControlCurrentED_w },    /*  9 */
    { "HcBulkHeadED",        HcBulkHeadED_r,         HcBulkHeadED_w },          /* 10 */
    { "HcBulkCurrentED",     HcBulkCurrentED_r,      HcBulkCurrentED_w },       /* 11 */
    { "HcDoneHead",          HcDoneHead_r,           HcDoneHead_w },            /* 12 */
    { "HcFmInterval",        HcFmInterval_r,         HcFmInterval_w },          /* 13 */
    { "HcFmRemaining",       HcFmRemaining_r,        HcFmRemaining_w },         /* 14 */
    { "HcFmNumber",          HcFmNumber_r,           HcFmNumber_w },            /* 15 */
    { "HcPeriodicStart",     HcPeriodicStart_r,      HcPeriodicStart_w },       /* 16 */
    { "HcLSThreshold",       HcLSThreshold_r,        HcLSThreshold_w },         /* 17 */
    { "HcRhDescriptorA",     HcRhDescriptorA_r,      HcRhDescriptorA_w },       /* 18 */
    { "HcRhDescriptorB",     HcRhDescriptorB_r,      HcRhDescriptorB_w },       /* 19 */
    { "HcRhStatus",          HcRhStatus_r,           HcRhStatus_w },            /* 20 */

    /* The number of port status register depends on the definition
     * of OHCI_NDP_MAX macro
     */
    { "HcRhPortStatus[0]",   HcRhPortStatus_r,       HcRhPortStatus_w },        /* 21 */
    { "HcRhPortStatus[1]",   HcRhPortStatus_r,       HcRhPortStatus_w },        /* 22 */
    { "HcRhPortStatus[2]",   HcRhPortStatus_r,       HcRhPortStatus_w },        /* 23 */
    { "HcRhPortStatus[3]",   HcRhPortStatus_r,       HcRhPortStatus_w },        /* 24 */
    { "HcRhPortStatus[4]",   HcRhPortStatus_r,       HcRhPortStatus_w },        /* 25 */
    { "HcRhPortStatus[5]",   HcRhPortStatus_r,       HcRhPortStatus_w },        /* 26 */
    { "HcRhPortStatus[6]",   HcRhPortStatus_r,       HcRhPortStatus_w },        /* 27 */
    { "HcRhPortStatus[7]",   HcRhPortStatus_r,       HcRhPortStatus_w },        /* 28 */
    { "HcRhPortStatus[8]",   HcRhPortStatus_r,       HcRhPortStatus_w },        /* 29 */
    { "HcRhPortStatus[9]",   HcRhPortStatus_r,       HcRhPortStatus_w },        /* 30 */
    { "HcRhPortStatus[10]",  HcRhPortStatus_r,       HcRhPortStatus_w },        /* 31 */
    { "HcRhPortStatus[11]",  HcRhPortStatus_r,       HcRhPortStatus_w },        /* 32 */
    { "HcRhPortStatus[12]",  HcRhPortStatus_r,       HcRhPortStatus_w },        /* 33 */
    { "HcRhPortStatus[13]",  HcRhPortStatus_r,       HcRhPortStatus_w },        /* 34 */
    { "HcRhPortStatus[14]",  HcRhPortStatus_r,       HcRhPortStatus_w },        /* 35 */
};

/* Quick way to determine how many op regs are valid. Since at least one port must
 * be configured (and no more than 15), there will be between 22 and 36 registers.
 */
#define NUM_OP_REGS(pohci)  (21 + OHCI_NDP_CFG(pohci))

AssertCompile(RT_ELEMENTS(g_aOpRegs) > 21);
AssertCompile(RT_ELEMENTS(g_aOpRegs) <= 36);

/**
 * @callback_method_impl{FNIOMMMIOREAD}
 */
PDMBOTHCBDECL(int) ohciMmioRead(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb)
{
    POHCI pThis = PDMINS_2_DATA(pDevIns, POHCI);
    RT_NOREF1(pvUser);

    /* Paranoia: Assert that IOMMMIO_FLAGS_READ_DWORD works. */
    AssertReturn(cb == sizeof(uint32_t), VERR_INTERNAL_ERROR_3);
    AssertReturn(!(GCPhysAddr & 0x3), VERR_INTERNAL_ERROR_4);

    /*
     * Validate the register and call the read operator.
     */
    int rc;
    const uint32_t iReg = (GCPhysAddr - pThis->MMIOBase) >> 2;
    if (iReg < NUM_OP_REGS(pThis))
    {
        const OHCIOPREG *pReg = &g_aOpRegs[iReg];
        rc = pReg->pfnRead(pThis, iReg, (uint32_t *)pv);
    }
    else
    {
        Log(("ohci: Trying to read register %u/%u!!!\n", iReg, NUM_OP_REGS(pThis)));
        rc = VINF_IOM_MMIO_UNUSED_FF;
    }
    return rc;
}


/**
 * @callback_method_impl{FNIOMMMIOWRITE}
 */
PDMBOTHCBDECL(int) ohciMmioWrite(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void const *pv, unsigned cb)
{
    POHCI pThis = PDMINS_2_DATA(pDevIns, POHCI);
    RT_NOREF1(pvUser);

    /* Paranoia: Assert that IOMMMIO_FLAGS_WRITE_DWORD_ZEROED works. */
    AssertReturn(cb == sizeof(uint32_t), VERR_INTERNAL_ERROR_3);
    AssertReturn(!(GCPhysAddr & 0x3), VERR_INTERNAL_ERROR_4);

    /*
     * Validate the register and call the read operator.
     */
    int rc;
    const uint32_t iReg = (GCPhysAddr - pThis->MMIOBase) >> 2;
    if (iReg < NUM_OP_REGS(pThis))
    {
        const OHCIOPREG *pReg = &g_aOpRegs[iReg];
        rc = pReg->pfnWrite(pThis, iReg, *(uint32_t const *)pv);
    }
    else
    {
        Log(("ohci: Trying to write to register %u/%u!!!\n", iReg, NUM_OP_REGS(pThis)));
        rc = VINF_SUCCESS;
    }
    return rc;
}

#ifdef IN_RING3

/**
 * @callback_method_impl{FNPCIIOREGIONMAP}
 */
static DECLCALLBACK(int) ohciR3Map(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                   RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType)
{
    RT_NOREF(iRegion, enmType);
    POHCI pThis = (POHCI)pPciDev;
    int rc = PDMDevHlpMMIORegister(pDevIns, GCPhysAddress, cb, NULL /*pvUser*/,
                                   IOMMMIO_FLAGS_READ_DWORD | IOMMMIO_FLAGS_WRITE_DWORD_ZEROED
                                   | IOMMMIO_FLAGS_DBGSTOP_ON_COMPLICATED_WRITE,
                                   ohciMmioWrite, ohciMmioRead, "USB OHCI");
    if (RT_FAILURE(rc))
        return rc;

    if (pThis->fRZEnabled)
    {
        rc = PDMDevHlpMMIORegisterRC(pDevIns, GCPhysAddress, cb, NIL_RTRCPTR /*pvUser*/, "ohciMmioWrite", "ohciMmioRead");
        if (RT_FAILURE(rc))
            return rc;

        rc = PDMDevHlpMMIORegisterR0(pDevIns, GCPhysAddress, cb, NIL_RTR0PTR /*pvUser*/, "ohciMmioWrite", "ohciMmioRead");
        if (RT_FAILURE(rc))
            return rc;
    }

    pThis->MMIOBase = GCPhysAddress;
    return VINF_SUCCESS;
}


/**
 * Prepares for state saving.
 * All URBs needs to be canceled.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pSSM        The handle to save the state to.
 */
static DECLCALLBACK(int) ohciR3SavePrep(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    RT_NOREF(pSSM);
    POHCI pThis = PDMINS_2_DATA(pDevIns, POHCI);
    POHCIROOTHUB pRh = &pThis->RootHub;
    LogFlow(("ohciR3SavePrep: \n"));

    /*
     * Detach all proxied devices.
     */
    PDMCritSectEnter(pThis->pDevInsR3->pCritSectRoR3, VERR_IGNORED);
    /** @todo this won't work well when continuing after saving! */
    for (unsigned i = 0; i < RT_ELEMENTS(pRh->aPorts); i++)
    {
        PVUSBIDEVICE pDev = pRh->aPorts[i].pDev;
        if (pDev)
        {
            if (!VUSBIDevIsSavedStateSupported(pDev))
            {
                VUSBIRhDetachDevice(pRh->pIRhConn, pDev);
                /*
                 * Save the device pointers here so we can reattach them afterwards.
                 * This will work fine even if the save fails since the Done handler is
                 * called unconditionally if the Prep handler was called.
                 */
                pRh->aPorts[i].pDev = pDev;
            }
        }
    }

    /*
     * If the bus was started set the timer. This is ugly but avoids changing the
     * saved state version for now so we can backport the changes to other branches.
     */
    /** @todo Do it properly for 4.4 by changing the saved state. */
    if (VUSBIRhGetPeriodicFrameRate(pRh->pIRhConn) != 0)
    {
        /* Calculate a new timer expiration so this saved state works with older releases. */
        uint64_t u64Expire = PDMDevHlpTMTimeVirtGet(pThis->CTX_SUFF(pDevIns)) + pThis->cTicksPerFrame;

        LogFlowFunc(("Bus is active, setting timer to %llu\n", u64Expire));
        int rc = TMTimerSet(pThis->pEndOfFrameTimerR3, u64Expire);
        AssertRC(rc);
    }

    PDMCritSectLeave(pThis->pDevInsR3->pCritSectRoR3);

    /*
     * Kill old load data which might be hanging around.
     */
    if (pThis->pLoad)
    {
        TMR3TimerDestroy(pThis->pLoad->pTimer);
        MMR3HeapFree(pThis->pLoad);
        pThis->pLoad = NULL;
    }
    return VINF_SUCCESS;
}


/**
 * Saves the state of the OHCI device.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pSSM        The handle to save the state to.
 */
static DECLCALLBACK(int) ohciR3SaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    POHCI pThis = PDMINS_2_DATA(pDevIns, POHCI);
    LogFlow(("ohciR3SaveExec: \n"));

    int rc = SSMR3PutStructEx(pSSM, pThis, sizeof(*pThis), 0 /*fFlags*/, &g_aOhciFields[0], NULL);
    if (RT_SUCCESS(rc))
        rc = TMR3TimerSave(pThis->CTX_SUFF(pEndOfFrameTimer), pSSM);
    return rc;
}


/**
 * Done state save operation.
 *
 * @returns VBox load code.
 * @param   pDevIns         Device instance of the device which registered the data unit.
 * @param   pSSM            SSM operation handle.
 */
static DECLCALLBACK(int) ohciR3SaveDone(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    RT_NOREF(pSSM);
    POHCI pThis = PDMINS_2_DATA(pDevIns, POHCI);
    LogFlow(("ohciR3SaveDone: \n"));

    /*
     * NULL the dev pointers.
     */
    POHCIROOTHUB pRh = &pThis->RootHub;
    OHCIROOTHUB Rh = *pRh;
    for (unsigned i = 0; i < RT_ELEMENTS(pRh->aPorts); i++)
    {
        if (   pRh->aPorts[i].pDev
            && !VUSBIDevIsSavedStateSupported(pRh->aPorts[i].pDev))
            pRh->aPorts[i].pDev = NULL;
    }

    /*
     * Attach the devices.
     */
    for (unsigned i = 0; i < RT_ELEMENTS(pRh->aPorts); i++)
    {
        PVUSBIDEVICE pDev = Rh.aPorts[i].pDev;
        if (   pDev
            && !VUSBIDevIsSavedStateSupported(pDev))
            VUSBIRhAttachDevice(pRh->pIRhConn, pDev);
    }

    return VINF_SUCCESS;
}


/**
 * Prepare loading the state of the OHCI device.
 * This must detach the devices currently attached and save
 * the up for reconnect after the state load have been completed
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pSSM        The handle to the saved state.
 */
static DECLCALLBACK(int) ohciR3LoadPrep(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    RT_NOREF(pSSM);
    POHCI pThis = PDMINS_2_DATA(pDevIns, POHCI);
    LogFlow(("ohciR3LoadPrep:\n"));
    if (!pThis->pLoad)
    {
        /*
         * Detach all devices which are present in this session. Save them in the load
         * structure so we can reattach them after restoring the guest.
         */
        POHCIROOTHUB pRh = &pThis->RootHub;
        OHCILOAD Load;
        Load.pTimer = NULL;
        Load.cDevs = 0;
        for (unsigned i = 0; i < RT_ELEMENTS(pRh->aPorts); i++)
        {
            PVUSBIDEVICE pDev = pRh->aPorts[i].pDev;
            if (   pDev
                && !VUSBIDevIsSavedStateSupported(pDev))
            {
                Load.apDevs[Load.cDevs++] = pDev;
                VUSBIRhDetachDevice(pRh->pIRhConn, pDev);
                Assert(!pRh->aPorts[i].pDev);
            }
        }

        /*
         * Any devices to reattach, if so duplicate the Load struct.
         */
        if (Load.cDevs)
        {
            pThis->pLoad = (POHCILOAD)PDMDevHlpMMHeapAlloc(pDevIns, sizeof(Load));
            if (!pThis->pLoad)
                return VERR_NO_MEMORY;
            *pThis->pLoad = Load;
        }
    }
    /* else: we ASSUME no device can be attached or detach in the period
     *       between a state load and the pLoad stuff is processed. */
    return VINF_SUCCESS;
}


/**
 * Loads the state of the OHCI device.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pSSM        The handle to the saved state.
 * @param   uVersion    The data unit version number.
 * @param   uPass       The data pass.
 */
static DECLCALLBACK(int) ohciR3LoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    POHCI       pThis = PDMINS_2_DATA(pDevIns, POHCI);
    int         rc;
    LogFlow(("ohciR3LoadExec:\n"));
    Assert(uPass == SSM_PASS_FINAL); NOREF(uPass);

    if (uVersion == OHCI_SAVED_STATE_VERSION)
    {
        rc = SSMR3GetStructEx(pSSM, pThis, sizeof(*pThis), 0 /*fFlags*/, &g_aOhciFields[0], NULL);
        if (RT_FAILURE(rc))
            return rc;
    }
    else if (uVersion == OHCI_SAVED_STATE_VERSION_8PORTS)
    {
        static SSMFIELD const s_aOhciFields8Ports[] =
        {
            SSMFIELD_ENTRY(         OHCI, SofTime),
            SSMFIELD_ENTRY_CUSTOM(        dpic+fno, RT_OFFSETOF(OHCI, SofTime) + RT_SIZEOFMEMB(OHCI, SofTime), 4),
            SSMFIELD_ENTRY(         OHCI, RootHub.status),
            SSMFIELD_ENTRY(         OHCI, RootHub.desc_a),
            SSMFIELD_ENTRY(         OHCI, RootHub.desc_b),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[0].fReg),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[1].fReg),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[2].fReg),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[3].fReg),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[4].fReg),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[5].fReg),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[6].fReg),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[7].fReg),
            SSMFIELD_ENTRY(         OHCI, ctl),
            SSMFIELD_ENTRY(         OHCI, status),
            SSMFIELD_ENTRY(         OHCI, intr_status),
            SSMFIELD_ENTRY(         OHCI, intr),
            SSMFIELD_ENTRY(         OHCI, hcca),
            SSMFIELD_ENTRY(         OHCI, per_cur),
            SSMFIELD_ENTRY(         OHCI, ctrl_cur),
            SSMFIELD_ENTRY(         OHCI, ctrl_head),
            SSMFIELD_ENTRY(         OHCI, bulk_cur),
            SSMFIELD_ENTRY(         OHCI, bulk_head),
            SSMFIELD_ENTRY(         OHCI, done),
            SSMFIELD_ENTRY_CUSTOM(        fsmps+fit+fi+frt, RT_OFFSETOF(OHCI, done) + RT_SIZEOFMEMB(OHCI, done), 4),
            SSMFIELD_ENTRY(         OHCI, HcFmNumber),
            SSMFIELD_ENTRY(         OHCI, pstart),
            SSMFIELD_ENTRY_TERM()
        };

        rc = SSMR3GetStructEx(pSSM, pThis, sizeof(*pThis), 0 /*fFlags*/, &s_aOhciFields8Ports[0], NULL);
        if (RT_FAILURE(rc))
            return rc;
    }
    else if (uVersion == OHCI_SAVED_STATE_VERSION_MEM_HELL)
    {
        static SSMFIELD const s_aOhciFields22[] =
        {
            SSMFIELD_ENTRY_OLD(           PciDev.abConfig,              256),   /* DevPCI restores this. */
            SSMFIELD_ENTRY_OLD(           PciDev.Int,                   224),
            SSMFIELD_ENTRY_OLD(           PciDev.uDevFn,                4),
            SSMFIELD_ENTRY_OLD(           PciDev.Alignment0,            4),
            SSMFIELD_ENTRY_OLD_HCPTR(     PciDev.pszNameR3),
            SSMFIELD_ENTRY_OLD_HCPTR(     PciDev.pvReserved),
            SSMFIELD_ENTRY_OLD_HCPTR(     pDevInsR3),
            SSMFIELD_ENTRY_OLD_HCPTR(     pEndOfFrameTimerR3),
            SSMFIELD_ENTRY_OLD_HCPTR(     pDevInsR0),
            SSMFIELD_ENTRY_OLD_HCPTR(     pEndOfFrameTimerR0),
            SSMFIELD_ENTRY_OLD_RCPTR(     pDevInsRC),
            SSMFIELD_ENTRY_OLD_RCPTR(     pEndOfFrameTimerRC),
            SSMFIELD_ENTRY(         OHCI, SofTime),
            SSMFIELD_ENTRY_CUSTOM(        dpic+fno, RT_OFFSETOF(OHCI, SofTime) + RT_SIZEOFMEMB(OHCI, SofTime), 4),
            SSMFIELD_ENTRY_OLD(           MMIOBase,                     4),     /* DevPCI implicitly restores this. */
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.pIBase),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.pIRhConn),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.pIDev),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.IBase.pfnQueryInterface),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.IRhPort.pfnGetAvailablePorts),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.IRhPort.pfnGetUSBVersions),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.IRhPort.pfnAttach),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.IRhPort.pfnDetach),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.IRhPort.pfnReset),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.IRhPort.pfnXferCompletion),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.IRhPort.pfnXferError),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.IRhPort.Alignment),
            SSMFIELD_ENTRY_OLD(           RootHub.Led,                  16),    /* No device restored. */
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.ILeds.pfnQueryStatusLed),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.pLedsConnector),
            SSMFIELD_ENTRY(         OHCI, RootHub.status),
            SSMFIELD_ENTRY(         OHCI, RootHub.desc_a),
            SSMFIELD_ENTRY(         OHCI, RootHub.desc_b),
            SSMFIELD_ENTRY_OLD_PAD_HC64(  RootHub.Alignment0,           4),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[0].fReg),
            SSMFIELD_ENTRY_OLD_PAD_HC64(  RootHub.aPorts[0].Alignment0, 4),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.aPorts[0].pDev),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[1].fReg),
            SSMFIELD_ENTRY_OLD_PAD_HC64(  RootHub.aPorts[1].Alignment0, 4),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.aPorts[1].pDev),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[2].fReg),
            SSMFIELD_ENTRY_OLD_PAD_HC64(  RootHub.aPorts[2].Alignment0, 4),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.aPorts[2].pDev),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[3].fReg),
            SSMFIELD_ENTRY_OLD_PAD_HC64(  RootHub.aPorts[3].Alignment0, 4),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.aPorts[3].pDev),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[4].fReg),
            SSMFIELD_ENTRY_OLD_PAD_HC64(  RootHub.aPorts[4].Alignment0, 4),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.aPorts[4].pDev),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[5].fReg),
            SSMFIELD_ENTRY_OLD_PAD_HC64(  RootHub.aPorts[5].Alignment0, 4),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.aPorts[5].pDev),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[6].fReg),
            SSMFIELD_ENTRY_OLD_PAD_HC64(  RootHub.aPorts[6].Alignment0, 4),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.aPorts[6].pDev),
            SSMFIELD_ENTRY(         OHCI, RootHub.aPorts[7].fReg),
            SSMFIELD_ENTRY_OLD_PAD_HC64(  RootHub.aPorts[7].Alignment0, 4),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.aPorts[7].pDev),
            SSMFIELD_ENTRY_OLD_HCPTR(     RootHub.pThis),
            SSMFIELD_ENTRY(         OHCI, ctl),
            SSMFIELD_ENTRY(         OHCI, status),
            SSMFIELD_ENTRY(         OHCI, intr_status),
            SSMFIELD_ENTRY(         OHCI, intr),
            SSMFIELD_ENTRY(         OHCI, hcca),
            SSMFIELD_ENTRY(         OHCI, per_cur),
            SSMFIELD_ENTRY(         OHCI, ctrl_cur),
            SSMFIELD_ENTRY(         OHCI, ctrl_head),
            SSMFIELD_ENTRY(         OHCI, bulk_cur),
            SSMFIELD_ENTRY(         OHCI, bulk_head),
            SSMFIELD_ENTRY(         OHCI, done),
            SSMFIELD_ENTRY_CUSTOM(        fsmps+fit+fi+frt, RT_OFFSETOF(OHCI, done) + RT_SIZEOFMEMB(OHCI, done), 4),
            SSMFIELD_ENTRY(         OHCI, HcFmNumber),
            SSMFIELD_ENTRY(         OHCI, pstart),
            SSMFIELD_ENTRY_OLD(           cTicksPerFrame,               8),     /* done by the constructor */
            SSMFIELD_ENTRY_OLD(           cTicksPerUsbTick,             8),     /* ditto */
            SSMFIELD_ENTRY_OLD(           cInFlight,                    4),     /* no in-flight stuff when saving. */
            SSMFIELD_ENTRY_OLD(           Alignment1,                   4),
            SSMFIELD_ENTRY_OLD(           aInFlight,                    257 * 8),
            SSMFIELD_ENTRY_OLD_PAD_HC64(  aInFlight,                    257 * 8),
            SSMFIELD_ENTRY_OLD(           cInDoneQueue,                 4),     /* strict builds only, so don't bother. */
            SSMFIELD_ENTRY_OLD(           aInDoneQueue,                 4*64),
            SSMFIELD_ENTRY_OLD(           u32FmDoneQueueTail,           4),     /* logging only */
            SSMFIELD_ENTRY_OLD_PAD_HC32(  Alignment2,                   4),
            SSMFIELD_ENTRY_OLD_HCPTR(     pLoad),
            SSMFIELD_ENTRY_OLD(           StatCanceledIsocUrbs,         8),
            SSMFIELD_ENTRY_OLD(           StatCanceledGenUrbs,          8),
            SSMFIELD_ENTRY_OLD(           StatDroppedUrbs,              8),
            SSMFIELD_ENTRY_OLD(           StatTimer,                    32),
            SSMFIELD_ENTRY_TERM()
        };

        /* deserialize the struct */
        rc = SSMR3GetStructEx(pSSM, pThis, sizeof(*pThis), SSMSTRUCT_FLAGS_NO_MARKERS /*fFlags*/, &s_aOhciFields22[0], NULL);
        if (RT_FAILURE(rc))
            return rc;

        /* check delimiter */
        uint32_t u32;
        rc = SSMR3GetU32(pSSM, &u32);
        if (RT_FAILURE(rc))
            return rc;
        AssertMsgReturn(u32 == ~0U, ("%#x\n", u32), VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
    }
    else
        AssertMsgFailedReturn(("%d\n", uVersion), VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION);

    /*
     * Finally restore the timer.
     */
    return TMR3TimerLoad(pThis->pEndOfFrameTimerR3, pSSM);
}


/**
 * Done state load operation.
 *
 * @returns VBox load code.
 * @param   pDevIns         Device instance of the device which registered the data unit.
 * @param   pSSM            SSM operation handle.
 */
static DECLCALLBACK(int) ohciR3LoadDone(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    RT_NOREF(pSSM);
    POHCI pThis = PDMINS_2_DATA(pDevIns, POHCI);
    LogFlow(("ohciR3LoadDone:\n"));

    /*
     * Start a timer if we've got devices to reattach
     */
    if (pThis->pLoad)
    {
        int rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL, ohciR3LoadReattachDevices, pThis,
                                        TMTIMER_FLAGS_NO_CRIT_SECT, "OHCI reattach devices on load",
                                        &pThis->pLoad->pTimer);
        if (RT_SUCCESS(rc))
            rc = TMTimerSetMillies(pThis->pLoad->pTimer, 250);
        return rc;
    }

    return VINF_SUCCESS;
}


/**
 * Reattaches devices after a saved state load.
 */
static DECLCALLBACK(void) ohciR3LoadReattachDevices(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    RT_NOREF(pDevIns);
    POHCI pThis = (POHCI)pvUser;
    POHCILOAD pLoad = pThis->pLoad;
    POHCIROOTHUB pRh = &pThis->RootHub;
    LogFlow(("ohciR3LoadReattachDevices:\n"));

    /*
     * Reattach devices.
     */
    for (unsigned i = 0; i < pLoad->cDevs; i++)
        VUSBIRhAttachDevice(pRh->pIRhConn, pLoad->apDevs[i]);

    /*
     * Cleanup.
     */
    TMR3TimerDestroy(pTimer);
    MMR3HeapFree(pLoad);
    pThis->pLoad = NULL;
}


/**
 * Reset notification.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance data.
 */
static DECLCALLBACK(void) ohciR3Reset(PPDMDEVINS pDevIns)
{
    POHCI pThis = PDMINS_2_DATA(pDevIns, POHCI);
    LogFlow(("ohciR3Reset:\n"));

    /*
     * There is no distinction between cold boot, warm reboot and software reboots,
     * all of these are treated as cold boots. We are also doing the initialization
     * job of a BIOS or SMM driver.
     *
     * Important: Don't confuse UsbReset with hardware reset. Hardware reset is
     *            just one way of getting into the UsbReset state.
     */
    ohciDoReset(pThis, OHCI_USB_RESET, true /* reset devices */);
}


/**
 * Resume notification.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance data.
 */
static DECLCALLBACK(void) ohciR3Resume(PPDMDEVINS pDevIns)
{
    POHCI pThis = PDMINS_2_DATA(pDevIns, POHCI);
    LogFlowFunc(("\n"));

    /* Restart the frame thread if the timer is active. */
    if (TMTimerIsActive(pThis->pEndOfFrameTimerR3))
    {
        int rc = TMTimerStop(pThis->pEndOfFrameTimerR3);
        AssertRC(rc);

        LogFlowFunc(("Bus was active, enable periodic frame processing\n"));
        rc = pThis->RootHub.pIRhConn->pfnSetPeriodicFrameProcessing(pThis->RootHub.pIRhConn, OHCI_DEFAULT_TIMER_FREQ);
        AssertRC(rc);
    }
}


/**
 * Info handler, device version. Dumps OHCI control registers.
 *
 * @param   pDevIns     Device instance which registered the info.
 * @param   pHlp        Callback functions for doing output.
 * @param   pszArgs     Argument string. Optional and specific to the handler.
 */
static DECLCALLBACK(void) ohciR3InfoRegs(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    RT_NOREF(pszArgs);
    POHCI pThis = PDMINS_2_DATA(pDevIns, POHCI);
    uint32_t val, ctl, status;

    /* Control register */
    ctl = pThis->ctl;
    pHlp->pfnPrintf(pHlp, "HcControl:          %08x - CBSR=%d PLE=%d IE=%d CLE=%d BLE=%d HCFS=%#x IR=%d RWC=%d RWE=%d\n",
          ctl, ctl & 3, (ctl >> 2) & 1, (ctl >> 3) & 1, (ctl >> 4) & 1, (ctl >> 5) & 1, (ctl >> 6) & 3, (ctl >> 8) & 1,
          (ctl >> 9) & 1, (ctl >> 10) & 1);

    /* Command status register */
    status = pThis->status;
    pHlp->pfnPrintf(pHlp, "HcCommandStatus:    %08x - HCR=%d CLF=%d BLF=%d OCR=%d SOC=%d\n",
          status, status & 1, (status >> 1) & 1, (status >> 2) & 1, (status >> 3) & 1, (status >> 16) & 3);

    /* Interrupt status register */
    val = pThis->intr_status;
    pHlp->pfnPrintf(pHlp, "HcInterruptStatus:  %08x - SO=%d WDH=%d SF=%d RD=%d UE=%d FNO=%d RHSC=%d OC=%d\n",
          val, val & 1, (val >> 1) & 1, (val >> 2) & 1, (val >> 3) & 1, (val >> 4) & 1, (val >> 5) & 1,
          (val >> 6) & 1, (val >> 30) & 1);

    /* Interrupt enable register */
    val = pThis->intr;
    pHlp->pfnPrintf(pHlp, "HcInterruptEnable:  %08x - SO=%d WDH=%d SF=%d RD=%d UE=%d FNO=%d RHSC=%d OC=%d MIE=%d\n",
          val, val & 1, (val >> 1) & 1, (val >> 2) & 1, (val >> 3) & 1, (val >> 4) & 1, (val >> 5) & 1,
          (val >> 6) & 1, (val >> 30) & 1, (val >> 31) & 1);

    /* HCCA address register */
    pHlp->pfnPrintf(pHlp, "HcHCCA:             %08x\n", pThis->hcca);

    /* Current periodic ED register */
    pHlp->pfnPrintf(pHlp, "HcPeriodCurrentED:  %08x\n", pThis->per_cur);

    /* Control ED registers */
    pHlp->pfnPrintf(pHlp, "HcControlHeadED:    %08x\n", pThis->ctrl_head);
    pHlp->pfnPrintf(pHlp, "HcControlCurrentED: %08x\n", pThis->ctrl_cur);

    /* Bulk ED registers */
    pHlp->pfnPrintf(pHlp, "HcBulkHeadED:       %08x\n", pThis->bulk_head);
    pHlp->pfnPrintf(pHlp, "HcBulkCurrentED:    %08x\n", pThis->bulk_cur);

    /* Done head register */
    pHlp->pfnPrintf(pHlp, "HcDoneHead:         %08x\n", pThis->done);

    /* Done head register */
    pHlp->pfnPrintf(pHlp, "HcDoneHead:         %08x\n", pThis->done);

    /* Root hub descriptor A */
    val = pThis->RootHub.desc_a;
    pHlp->pfnPrintf(pHlp, "HcRhDescriptorA:    %08x - NDP=%d PSM=%d NPS=%d DT=%d OCPM=%d NOCP=%d POTPGT=%d\n",
          val, (uint8_t)val, (val >> 8) & 1, (val >> 9) & 1, (val >> 10) & 1, (val >> 11) & 1, (val >> 12) & 1, (uint8_t)(val >> 24));

    /* Root hub descriptor B */
    val = pThis->RootHub.desc_b;
    pHlp->pfnPrintf(pHlp, "HcRhDescriptorB:    %08x - DR=%#04x PPCM=%#04x\n", val, (uint16_t)val, (uint16_t)(val >> 16));

    /* Root hub status register */
    val = pThis->RootHub.status;
    pHlp->pfnPrintf(pHlp, "HcRhStatus:         %08x - LPS=%d OCI=%d DRWE=%d  LPSC=%d OCIC=%d CRWE=%d\n\n",
          val, val & 1, (val >> 1) & 1, (val >> 15) & 1, (val >> 16) & 1, (val >> 17) & 1, (val >> 31) & 1);

    /* Port status registers */
    for (unsigned i = 0; i < OHCI_NDP_CFG(pThis); ++i)
    {
        val = pThis->RootHub.aPorts[i].fReg;
        pHlp->pfnPrintf(pHlp, "HcRhPortStatus%02d: CCS=%d PES =%d PSS =%d POCI=%d PRS =%d  PPS=%d LSDA=%d\n"
                              "      %08x -  CSC=%d PESC=%d PSSC=%d OCIC=%d PRSC=%d\n",
              i, val & 1, (val >> 1) & 1, (val >> 2) & 1,(val >> 3) & 1, (val >> 4) & 1, (val >> 8) & 1, (val >> 9) & 1,
              val, (val >> 16) & 1, (val >> 17) & 1, (val >> 18) & 1, (val >> 19) & 1, (val >> 20) & 1);
    }
}


/**
 * Relocate device instance data.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance data.
 * @param   offDelta    The relocation delta.
 */
static DECLCALLBACK(void) ohciR3Relocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta)
{
    RT_NOREF(offDelta);
    POHCI pThis = PDMINS_2_DATA(pDevIns, POHCI);
    pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    pThis->pEndOfFrameTimerRC = TMTimerRCPtr(pThis->pEndOfFrameTimerR3);
}


/**
 * Destruct a device instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that any non-VM
 * resources can be freed correctly.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance data.
 */
static DECLCALLBACK(int) ohciR3Destruct(PPDMDEVINS pDevIns)
{
    POHCI pThis = PDMINS_2_DATA(pDevIns, POHCI);
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);

#ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
    ohciPhysReadCacheFree(pThis->pCacheED);
    pThis->pCacheED = NULL;
    ohciPhysReadCacheFree(pThis->pCacheTD);
    pThis->pCacheTD = NULL;
#endif

    if (RTCritSectIsInitialized(&pThis->CritSect))
        RTCritSectDelete(&pThis->CritSect);
    PDMR3CritSectDelete(&pThis->CsIrq);

    /*
     * Tear down the per endpoint in-flight tracking...
     */

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct,OHCI constructor}
 */
static DECLCALLBACK(int) ohciR3Construct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    POHCI       pThis = PDMINS_2_DATA(pDevIns, POHCI);
    uint32_t    cPorts;
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Init instance data.
     */
    pThis->pDevInsR3 = pDevIns;
    pThis->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);

    PCIDevSetVendorId     (&pThis->PciDev, 0x106b);
    PCIDevSetDeviceId     (&pThis->PciDev, 0x003f);
    PCIDevSetClassProg    (&pThis->PciDev, 0x10); /* OHCI */
    PCIDevSetClassSub     (&pThis->PciDev, 0x03);
    PCIDevSetClassBase    (&pThis->PciDev, 0x0c);
    PCIDevSetInterruptPin (&pThis->PciDev, 0x01);
#ifdef VBOX_WITH_MSI_DEVICES
    PCIDevSetStatus       (&pThis->PciDev, VBOX_PCI_STATUS_CAP_LIST);
    PCIDevSetCapabilityList(&pThis->PciDev, 0x80);
#endif

    pThis->RootHub.pOhci                         = pThis;
    pThis->RootHub.IBase.pfnQueryInterface       = ohciRhQueryInterface;
    pThis->RootHub.IRhPort.pfnGetAvailablePorts  = ohciRhGetAvailablePorts;
    pThis->RootHub.IRhPort.pfnGetUSBVersions     = ohciRhGetUSBVersions;
    pThis->RootHub.IRhPort.pfnAttach             = ohciRhAttach;
    pThis->RootHub.IRhPort.pfnDetach             = ohciRhDetach;
    pThis->RootHub.IRhPort.pfnReset              = ohciRhReset;
    pThis->RootHub.IRhPort.pfnXferCompletion     = ohciRhXferCompletion;
    pThis->RootHub.IRhPort.pfnXferError          = ohciRhXferError;
    pThis->RootHub.IRhPort.pfnStartFrame         = ohciR3StartFrame;
    pThis->RootHub.IRhPort.pfnFrameRateChanged   = ohciR3FrameRateChanged;

    /* USB LED */
    pThis->RootHub.Led.u32Magic                  = PDMLED_MAGIC;
    pThis->RootHub.ILeds.pfnQueryStatusLed       = ohciRhQueryStatusLed;


    /*
     * Read configuration.
     */
    PDMDEV_VALIDATE_CONFIG_RETURN(pDevIns, "RZEnabled", "");
    int rc = CFGMR3QueryBoolDef(pCfg, "RZEnabled", &pThis->fRZEnabled, true);
    AssertLogRelRCReturn(rc, rc);

    /* Number of ports option. */
    rc = CFGMR3QueryU32Def(pCfg, "Ports", &cPorts, OHCI_NDP_DEFAULT);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("OHCI configuration error: failed to read Ports as integer"));

    if (cPorts == 0 || cPorts > OHCI_NDP_MAX)
        return PDMDevHlpVMSetError(pDevIns, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                   N_("OHCI configuration error: Ports must be in range [%u,%u]"),
                                   1, OHCI_NDP_MAX);

    /* Store the configured NDP; it will be used everywhere else from now on. */
    pThis->RootHub.desc_a = cPorts;

    /*
     * Register PCI device and I/O region.
     */
    rc = PDMDevHlpPCIRegister(pDevIns, &pThis->PciDev);
    if (RT_FAILURE(rc))
        return rc;

#ifdef VBOX_WITH_MSI_DEVICES
    PDMMSIREG MsiReg;
    RT_ZERO(MsiReg);
    MsiReg.cMsiVectors    = 1;
    MsiReg.iMsiCapOffset  = 0x80;
    MsiReg.iMsiNextOffset = 0x00;
    rc = PDMDevHlpPCIRegisterMsi(pDevIns, &MsiReg);
    if (RT_FAILURE(rc))
    {
        PCIDevSetCapabilityList(&pThis->PciDev, 0x0);
        /* That's OK, we can work without MSI */
    }
#endif

    rc = PDMDevHlpPCIIORegionRegister(pDevIns, 0, 4096, PCI_ADDRESS_SPACE_MEM, ohciR3Map);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Create the end-of-frame timer.
     */
    rc = PDMDevHlpTMTimerCreate(pDevIns, TMCLOCK_VIRTUAL, ohciFrameBoundaryTimer, pThis,
                                TMTIMER_FLAGS_DEFAULT_CRIT_SECT, "USB Frame Timer",
                                &pThis->pEndOfFrameTimerR3);
    if (RT_FAILURE(rc))
        return rc;
    pThis->pEndOfFrameTimerR0 = TMTimerR0Ptr(pThis->pEndOfFrameTimerR3);
    pThis->pEndOfFrameTimerRC = TMTimerRCPtr(pThis->pEndOfFrameTimerR3);

    /*
     * Register the saved state data unit.
     */
    rc = PDMDevHlpSSMRegisterEx(pDevIns, OHCI_SAVED_STATE_VERSION, sizeof(*pThis), NULL,
                                NULL, NULL, NULL,
                                ohciR3SavePrep, ohciR3SaveExec, ohciR3SaveDone,
                                ohciR3LoadPrep, ohciR3LoadExec, ohciR3LoadDone);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Attach to the VBox USB RootHub Driver on LUN #0.
     */
    rc = PDMDevHlpDriverAttach(pDevIns, 0, &pThis->RootHub.IBase, &pThis->RootHub.pIBase, "RootHub");
    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Configuration error: No roothub driver attached to LUN #0!\n"));
        return rc;
    }
    pThis->RootHub.pIRhConn = PDMIBASE_QUERY_INTERFACE(pThis->RootHub.pIBase, VUSBIROOTHUBCONNECTOR);
    AssertMsgReturn(pThis->RootHub.pIRhConn,
                    ("Configuration error: The driver doesn't provide the VUSBIROOTHUBCONNECTOR interface!\n"),
                    VERR_PDM_MISSING_INTERFACE);
    pThis->RootHub.pIDev = PDMIBASE_QUERY_INTERFACE(pThis->RootHub.pIBase, VUSBIDEVICE);
    AssertMsgReturn(pThis->RootHub.pIDev,
                    ("Configuration error: The driver doesn't provide the VUSBIDEVICE interface!\n"),
                    VERR_PDM_MISSING_INTERFACE);

    /*
     * Attach status driver (optional).
     */
    PPDMIBASE pBase;
    rc = PDMDevHlpDriverAttach(pDevIns, PDM_STATUS_LUN, &pThis->RootHub.IBase, &pBase, "Status Port");
    if (RT_SUCCESS(rc))
        pThis->RootHub.pLedsConnector = PDMIBASE_QUERY_INTERFACE(pBase, PDMILEDCONNECTORS);
    else if (rc != VERR_PDM_NO_ATTACHED_DRIVER)
    {
        AssertMsgFailed(("Failed to attach to status driver. rc=%Rrc\n", rc));
        return rc;
    }

    /* Set URB parameters. */
    rc = VUSBIRhSetUrbParams(pThis->RootHub.pIRhConn, sizeof(VUSBURBHCIINT), sizeof(VUSBURBHCITDINT));
    if (RT_FAILURE(rc))
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("OHCI: Failed to set URB parameters"));

    /*
     * Calculate the timer intervals.
     * This assumes that the VM timer doesn't change frequency during the run.
     */
    pThis->u64TimerHz = TMTimerGetFreq(pThis->CTX_SUFF(pEndOfFrameTimer));

    rc = PDMDevHlpCritSectInit(pDevIns, &pThis->CsIrq, RT_SRC_POS, "OHCI#%uIrq", iInstance);
    if (RT_FAILURE(rc))
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("OHCI: Failed to create critical section"));

    rc = RTCritSectInit(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("OHCI: Failed to create critical section"));

#ifdef VBOX_WITH_OHCI_PHYS_READ_CACHE
    pThis->pCacheED = ohciPhysReadCacheAlloc();
    pThis->pCacheTD = ohciPhysReadCacheAlloc();
    if (pThis->pCacheED == NULL || pThis->pCacheTD == NULL)
        return PDMDevHlpVMSetError(pDevIns, VERR_NO_MEMORY, RT_SRC_POS,
                                   N_("OHCI: Failed to allocate PhysRead cache"));
#endif

    /*
     * Do a hardware reset.
     */
    ohciDoReset(pThis, OHCI_USB_RESET, false /* don't reset devices */);

#ifdef VBOX_WITH_STATISTICS
    /*
     * Register statistics.
     */
    PDMDevHlpSTAMRegister(pDevIns, &pThis->StatCanceledIsocUrbs, STAMTYPE_COUNTER, "/Devices/OHCI/CanceledIsocUrbs", STAMUNIT_OCCURENCES,     "Detected canceled isochronous URBs.");
    PDMDevHlpSTAMRegister(pDevIns, &pThis->StatCanceledGenUrbs,  STAMTYPE_COUNTER, "/Devices/OHCI/CanceledGenUrbs",  STAMUNIT_OCCURENCES,     "Detected canceled general URBs.");
    PDMDevHlpSTAMRegister(pDevIns, &pThis->StatDroppedUrbs,      STAMTYPE_COUNTER, "/Devices/OHCI/DroppedUrbs",      STAMUNIT_OCCURENCES,     "Dropped URBs (endpoint halted, or URB canceled).");
#endif

    /*
     * Register debugger info callbacks.
     */
    PDMDevHlpDBGFInfoRegister(pDevIns, "ohci", "OHCI control registers.", ohciR3InfoRegs);

#if 0/*def DEBUG_bird*/
//  g_fLogInterruptEPs = true;
    g_fLogControlEPs = true;
    g_fLogBulkEPs = true;
#endif

    return VINF_SUCCESS;
}


const PDMDEVREG g_DeviceOHCI =
{
    /* u32version */
    PDM_DEVREG_VERSION,
    /* szName */
    "usb-ohci",
    /* szRCMod */
    "VBoxDDRC.rc",
    /* szR0Mod */
    "VBoxDDR0.r0",
    /* pszDescription */
    "OHCI USB controller.\n",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0,
    /* fClass */
    PDM_DEVREG_CLASS_BUS_USB,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(OHCI),
    /* pfnConstruct */
    ohciR3Construct,
    /* pfnDestruct */
    ohciR3Destruct,
    /* pfnRelocate */
    ohciR3Relocate,
    /* pfnMemSetup */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    ohciR3Reset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    ohciR3Resume,
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

#endif /* IN_RING3 */
#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */
