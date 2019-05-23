/* $Id: Virtio.h $ */
/** @file
 * Virtio.h - Virtio Declarations
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

#ifndef ___VBox_Virtio_h
#define ___VBox_Virtio_h

#include <iprt/ctype.h>


/** @name Saved state versions.
 * The saved state version is changed if either common or any of specific
 * parts are changed. That is, it is perfectly possible that the version
 * of saved vnet state will increase as a result of change in vblk structure
 * for example.
 */
#define VIRTIO_SAVEDSTATE_VERSION_3_1_BETA1 1
#define VIRTIO_SAVEDSTATE_VERSION           2
/** @} */

#define DEVICE_PCI_VENDOR_ID                0x1AF4
#define DEVICE_PCI_BASE_ID                  0x1000
#define DEVICE_PCI_SUBSYSTEM_VENDOR_ID      0x1AF4
#define DEVICE_PCI_SUBSYSTEM_BASE_ID       1

#define VIRTIO_MAX_NQUEUES                  3

#define VPCI_HOST_FEATURES                  0x0
#define VPCI_GUEST_FEATURES                 0x4
#define VPCI_QUEUE_PFN                      0x8
#define VPCI_QUEUE_NUM                      0xC
#define VPCI_QUEUE_SEL                      0xE
#define VPCI_QUEUE_NOTIFY                   0x10
#define VPCI_STATUS                         0x12
#define VPCI_ISR                            0x13
#define VPCI_CONFIG                         0x14

#define VPCI_ISR_QUEUE                      0x1
#define VPCI_ISR_CONFIG                     0x3

#define VPCI_STATUS_ACK                     0x01
#define VPCI_STATUS_DRV                     0x02
#define VPCI_STATUS_DRV_OK                  0x04
#define VPCI_STATUS_FAILED                  0x80

#define VPCI_F_NOTIFY_ON_EMPTY              0x01000000
#define VPCI_F_ANY_LAYOUT                   0x08000000
#define VPCI_F_RING_INDIRECT_DESC           0x10000000
#define VPCI_F_RING_EVENT_IDX               0x20000000
#define VPCI_F_BAD_FEATURE                  0x40000000

#define VRINGDESC_MAX_SIZE                  (2 * 1024 * 1024)
#define VRINGDESC_F_NEXT                    0x01
#define VRINGDESC_F_WRITE                   0x02
#define VRINGDESC_F_INDIRECT                0x04

typedef struct VRingDesc
{
    uint64_t u64Addr;
    uint32_t uLen;
    uint16_t u16Flags;
    uint16_t u16Next;
} VRINGDESC;
typedef VRINGDESC *PVRINGDESC;

#define VRINGAVAIL_F_NO_INTERRUPT 0x01

typedef struct VRingAvail
{
    uint16_t uFlags;
    uint16_t uNextFreeIndex;
    uint16_t auRing[1];
} VRINGAVAIL;

typedef struct VRingUsedElem
{
    uint32_t uId;
    uint32_t uLen;
} VRINGUSEDELEM;

#define VRINGUSED_F_NO_NOTIFY 0x01

typedef struct VRingUsed
{
    uint16_t      uFlags;
    uint16_t      uIndex;
    VRINGUSEDELEM aRing[1];
} VRINGUSED;
typedef VRINGUSED *PVRINGUSED;

#define VRING_MAX_SIZE 1024

typedef struct VRing
{
    uint16_t   uSize;
    uint16_t   padding[3];
    RTGCPHYS   addrDescriptors;
    RTGCPHYS   addrAvail;
    RTGCPHYS   addrUsed;
} VRING;
typedef VRING *PVRING;

/**
 * Queue callback (consumer?).
 *
 * @param   pvState         Pointer to the VirtIO PCI core state, VPCISTATE.
 * @param   pQueue          Pointer to the queue structure.
 */
typedef DECLCALLBACK(void) FNVPCIQUEUECALLBACK(void *pvState, struct VQueue *pQueue);
/** Pointer to a VQUEUE callback function. */
typedef FNVPCIQUEUECALLBACK *PFNVPCIQUEUECALLBACK;

typedef struct VQueue
{
    VRING    VRing;
    uint16_t uNextAvailIndex;
    uint16_t uNextUsedIndex;
    uint32_t uPageNumber;
    R3PTRTYPE(PFNVPCIQUEUECALLBACK) pfnCallback;
    R3PTRTYPE(const char *)         pcszName;
} VQUEUE;
typedef VQUEUE *PVQUEUE;

typedef struct VQueueElemSeg
{
    RTGCPHYS addr;
    void    *pv;
    uint32_t cb;
} VQUEUESEG;

typedef struct VQueueElem
{
    uint32_t  uIndex;
    uint32_t  nIn;
    uint32_t  nOut;
    VQUEUESEG aSegsIn[VRING_MAX_SIZE];
    VQUEUESEG aSegsOut[VRING_MAX_SIZE];
} VQUEUEELEM;
typedef VQUEUEELEM *PVQUEUEELEM;


enum VirtioDeviceType
{
    VIRTIO_NET_ID = 0,
    VIRTIO_BLK_ID = 1,
    VIRTIO_32BIT_HACK = 0x7fffffff
};


/**
 * The core (/common) state of the VirtIO PCI device
 *
 * @implements  PDMILEDPORTS
 */
typedef struct VPCIState_st
{
    PDMCRITSECT            cs;      /**< Critical section - what is it protecting? */
    /* Read-only part, never changes after initialization. */
    char                   szInstance[8];         /**< Instance name, e.g. VNet#1. */

#if HC_ARCH_BITS != 64
    uint32_t               padding1;
#endif

    /** Status LUN: Base interface. */
    PDMIBASE               IBase;
    /** Status LUN: LED port interface. */
    PDMILEDPORTS           ILeds;
    /** Status LUN: LED connector (peer). */
    R3PTRTYPE(PPDMILEDCONNECTORS) pLedsConnector;

    PPDMDEVINSR3           pDevInsR3;                   /**< Device instance - R3. */
    PPDMDEVINSR0           pDevInsR0;                   /**< Device instance - R0. */
    PPDMDEVINSRC           pDevInsRC;                   /**< Device instance - RC. */

#if HC_ARCH_BITS == 64
    uint32_t               padding2;
#endif

    /** TODO */
    PDMPCIDEV              pciDevice;
    /** Base port of I/O space region. */
    RTIOPORT               IOPortBase;

    /* Read/write part, protected with critical section. */
    /** Status LED. */
    PDMLED                 led;

    uint32_t               uGuestFeatures;
    uint16_t               uQueueSelector;         /**< An index in aQueues array. */
    uint8_t                uStatus; /**< Device Status (bits are device-specific). */
    uint8_t                uISR;                   /**< Interrupt Status Register. */

#if HC_ARCH_BITS != 64
    uint32_t               padding3;
#endif

    uint32_t               nQueues;       /**< Actual number of queues used. */
    VQUEUE                 Queues[VIRTIO_MAX_NQUEUES];

#if defined(VBOX_WITH_STATISTICS)
    STAMPROFILEADV         StatIOReadGC;
    STAMPROFILEADV         StatIOReadHC;
    STAMPROFILEADV         StatIOWriteGC;
    STAMPROFILEADV         StatIOWriteHC;
    STAMCOUNTER            StatIntsRaised;
    STAMCOUNTER            StatIntsSkipped;
    STAMPROFILE            StatCsGC;
    STAMPROFILE            StatCsHC;
#endif /* VBOX_WITH_STATISTICS */
} VPCISTATE;
/** Pointer to the core (/common) state of a VirtIO PCI device. */
typedef VPCISTATE *PVPCISTATE;

typedef DECLCALLBACK(uint32_t) FNGETHOSTFEATURES(void *pvState);
typedef FNGETHOSTFEATURES *PFNGETHOSTFEATURES;

/** @name VirtIO port I/O callbacks.
 * @{ */
typedef struct VPCIIOCALLBACKS
{
     DECLCALLBACKMEMBER(uint32_t, pfnGetHostFeatures)(void *pvState);
     DECLCALLBACKMEMBER(uint32_t, pfnGetHostMinimalFeatures)(void *pvState);
     DECLCALLBACKMEMBER(void,     pfnSetHostFeatures)(void *pvState, uint32_t fFeatures);
     DECLCALLBACKMEMBER(int,      pfnGetConfig)(void *pvState, uint32_t offCfg, uint32_t cb, void *pvData);
     DECLCALLBACKMEMBER(int,      pfnSetConfig)(void *pvState, uint32_t offCfg, uint32_t cb, void *pvData);
     DECLCALLBACKMEMBER(int,      pfnReset)(void *pvState);
     DECLCALLBACKMEMBER(void,     pfnReady)(void *pvState);
} VPCIIOCALLBACKS;
/** Pointer to a const VirtIO port I/O callback structure. */
typedef const VPCIIOCALLBACKS *PCVPCIIOCALLBACKS;
/** @} */

int vpciRaiseInterrupt(VPCISTATE *pState, int rcBusy, uint8_t u8IntCause);
int vpciIOPortIn(PPDMDEVINS         pDevIns,
                 void              *pvUser,
                 RTIOPORT           port,
                 uint32_t          *pu32,
                 unsigned           cb,
                 PCVPCIIOCALLBACKS  pCallbacks);

int vpciIOPortOut(PPDMDEVINS                pDevIns,
                  void                     *pvUser,
                  RTIOPORT                  port,
                  uint32_t                  u32,
                  unsigned                  cb,
                  PCVPCIIOCALLBACKS         pCallbacks);

void  vpciSetWriteLed(PVPCISTATE pState, bool fOn);
void  vpciSetReadLed(PVPCISTATE pState, bool fOn);
int   vpciSaveExec(PVPCISTATE pState, PSSMHANDLE pSSM);
int   vpciLoadExec(PVPCISTATE pState, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass, uint32_t nQueues);
int   vpciConstruct(PPDMDEVINS pDevIns, VPCISTATE *pState, int iInstance, const char *pcszNameFmt,
                    uint16_t uDeviceId, uint16_t uClass, uint32_t nQueues);
int   vpciDestruct(VPCISTATE* pState);
void  vpciRelocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta);
void  vpciReset(PVPCISTATE pState);
void *vpciQueryInterface(struct PDMIBASE *pInterface, const char *pszIID);
PVQUEUE vpciAddQueue(VPCISTATE* pState, unsigned uSize, PFNVPCIQUEUECALLBACK pfnCallback, const char *pcszName);

#define VPCI_CS
DECLINLINE(int) vpciCsEnter(VPCISTATE *pState, int rcBusy)
{
#ifdef VPCI_CS
    STAM_PROFILE_START(&pState->CTXSUFF(StatCs), a);
    int rc = PDMCritSectEnter(&pState->cs, rcBusy);
    STAM_PROFILE_STOP(&pState->CTXSUFF(StatCs), a);
    return rc;
#else
    return VINF_SUCCESS;
#endif
}

DECLINLINE(void) vpciCsLeave(VPCISTATE *pState)
{
#ifdef VPCI_CS
    PDMCritSectLeave(&pState->cs);
#endif
}

void vringSetNotification(PVPCISTATE pState, PVRING pVRing, bool fEnabled);

DECLINLINE(uint16_t) vringReadAvailIndex(PVPCISTATE pState, PVRING pVRing)
{
    uint16_t tmp;

    PDMDevHlpPhysRead(pState->CTX_SUFF(pDevIns),
                      pVRing->addrAvail + RT_OFFSETOF(VRINGAVAIL, uNextFreeIndex),
                      &tmp, sizeof(tmp));
    return tmp;
}

bool vqueueSkip(PVPCISTATE pState, PVQUEUE pQueue);
bool vqueueGet(PVPCISTATE pState, PVQUEUE pQueue, PVQUEUEELEM pElem, bool fRemove = true);
void vqueuePut(PVPCISTATE pState, PVQUEUE pQueue, PVQUEUEELEM pElem, uint32_t uLen, uint32_t uReserved = 0);
void vqueueNotify(PVPCISTATE pState, PVQUEUE pQueue);
void vqueueSync(PVPCISTATE pState, PVQUEUE pQueue);

DECLINLINE(bool) vqueuePeek(PVPCISTATE pState, PVQUEUE pQueue, PVQUEUEELEM pElem)
{
    return vqueueGet(pState, pQueue, pElem, /* fRemove */ false);
}

DECLINLINE(bool) vqueueIsReady(PVPCISTATE pState, PVQUEUE pQueue)
{
    NOREF(pState);
    return !!pQueue->VRing.addrAvail;
}

DECLINLINE(bool) vqueueIsEmpty(PVPCISTATE pState, PVQUEUE pQueue)
{
    return (vringReadAvailIndex(pState, &pQueue->VRing) == pQueue->uNextAvailIndex);
}

#endif /* !___VBox_Virtio_h */
