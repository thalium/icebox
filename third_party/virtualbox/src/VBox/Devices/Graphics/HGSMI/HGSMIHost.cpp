/* $Id: HGSMIHost.cpp $ */
/** @file
 *
 * VBox Host Guest Shared Memory Interface (HGSMI).
 * Host part:
 *  - virtual hardware IO handlers;
 *  - channel management;
 *  - low level interface for buffer transfer.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*
 * Async host->guest calls. Completion by an IO write from the guest or a timer timeout.
 *
 * Sync guest->host calls. Initiated by an IO write from the guest.
 *
 * Guest->Host
 * ___________
 *
 * Synchronous for the guest, an async result can be also reported later by a host->guest call:
 *
 * G: Alloc shared memory, fill the structure, issue an IO write (HGSMI_IO_GUEST) with the memory offset.
 * H: Verify the shared memory and call the handler.
 * G: Continue after the IO completion.
 *
 *
 * Host->Guest
 * __________
 *
 * H:      Alloc shared memory, fill in the info.
 *         Register in the FIFO with a callback, issue IRQ (on EMT).
 *         Wait on a sem with timeout if necessary.
 * G:      Read FIFO from HGSMI_IO_HOST_COMMAND.
 * H(EMT): Get the shared memory offset from FIFO to return to the guest.
 * G:      Get offset, process command, issue IO write to HGSMI_IO_HOST_COMMAND.
 * H(EMT): Find registered shared mem, run callback, which could post the sem.
 * H:      Get results and free shared mem (could be freed automatically on EMT too).
 *
 *
 * Implementation notes:
 *
 * Host->Guest
 *
 * * Shared memory allocation using a critsect.
 * * FIFO manipulation with a critsect.
 *
 */

#include <iprt/alloc.h>
#include <iprt/critsect.h>
#include <iprt/heap.h>
#include <iprt/list.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>

#include <VBox/err.h>
#define LOG_GROUP LOG_GROUP_HGSMI
#include <VBox/log.h>
#include <VBox/vmm/ssm.h>

#include "HGSMIHost.h"
#include <HGSMIChannels.h>

#include "../DevVGASavedState.h"

#ifdef DEBUG_sunlover
#define HGSMI_STRICT 1
#endif /* !DEBUG_sunlover */

#ifdef DEBUG_misha
//# define VBOXHGSMI_STATE_DEBUG
#endif

#ifdef VBOXHGSMI_STATE_DEBUG
#define VBOXHGSMI_STATE_START_MAGIC 0x12345678
#define VBOXHGSMI_STATE_STOP_MAGIC  0x87654321
#define VBOXHGSMI_STATE_FIFOSTART_MAGIC 0x9abcdef1
#define VBOXHGSMI_STATE_FIFOSTOP_MAGIC 0x1fedcba9

#define VBOXHGSMI_SAVE_START(_pSSM)     do{ int rc2 = SSMR3PutU32(_pSSM, VBOXHGSMI_STATE_START_MAGIC);     AssertRC(rc2); }while(0)
#define VBOXHGSMI_SAVE_STOP(_pSSM)      do{ int rc2 = SSMR3PutU32(_pSSM, VBOXHGSMI_STATE_STOP_MAGIC);      AssertRC(rc2); }while(0)
#define VBOXHGSMI_SAVE_FIFOSTART(_pSSM) do{ int rc2 = SSMR3PutU32(_pSSM, VBOXHGSMI_STATE_FIFOSTART_MAGIC); AssertRC(rc2); }while(0)
#define VBOXHGSMI_SAVE_FIFOSTOP(_pSSM)  do{ int rc2 = SSMR3PutU32(_pSSM, VBOXHGSMI_STATE_FIFOSTOP_MAGIC);  AssertRC(rc2); }while(0)

#define VBOXHGSMI_LOAD_CHECK(_pSSM, _v) \
    do{ \
        uint32_t u32; \
        int rc2 = SSMR3GetU32(_pSSM, &u32); AssertRC(rc2); \
        Assert(u32 == (_v)); \
    }while(0)

#define VBOXHGSMI_LOAD_START(_pSSM) VBOXHGSMI_LOAD_CHECK(_pSSM, VBOXHGSMI_STATE_START_MAGIC)
#define VBOXHGSMI_LOAD_FIFOSTART(_pSSM) VBOXHGSMI_LOAD_CHECK(_pSSM, VBOXHGSMI_STATE_FIFOSTART_MAGIC)
#define VBOXHGSMI_LOAD_FIFOSTOP(_pSSM) VBOXHGSMI_LOAD_CHECK(_pSSM, VBOXHGSMI_STATE_FIFOSTOP_MAGIC)
#define VBOXHGSMI_LOAD_STOP(_pSSM) VBOXHGSMI_LOAD_CHECK(_pSSM, VBOXHGSMI_STATE_STOP_MAGIC)
#else
#define VBOXHGSMI_SAVE_START(_pSSM) do{ }while(0)
#define VBOXHGSMI_SAVE_STOP(_pSSM) do{ }while(0)
#define VBOXHGSMI_SAVE_FIFOSTART(_pSSM) do{ }while(0)
#define VBOXHGSMI_SAVE_FIFOSTOP(_pSSM) do{ }while(0)


#define VBOXHGSMI_LOAD_START(_pSSM) do{ }while(0)
#define VBOXHGSMI_LOAD_FIFOSTART(_pSSM) do{ }while(0)
#define VBOXHGSMI_LOAD_FIFOSTOP(_pSSM) do{ }while(0)
#define VBOXHGSMI_LOAD_STOP(_pSSM) do{ }while(0)

#endif

/* Assertions for situations which could happen and normally must be processed properly
 * but must be investigated during development: guest misbehaving, etc.
 */
#ifdef HGSMI_STRICT
#define HGSMI_STRICT_ASSERT_FAILED() AssertFailed()
#define HGSMI_STRICT_ASSERT(expr) Assert(expr)
#else
#define HGSMI_STRICT_ASSERT_FAILED() do {} while (0)
#define HGSMI_STRICT_ASSERT(expr) do {} while (0)
#endif /* !HGSMI_STRICT */


/* Host heap types. */
#define HGSMI_HEAP_TYPE_NULL    0 /* Heap not initialized. */
#define HGSMI_HEAP_TYPE_POINTER 1 /* Deprecated, used only for old saved states. RTHEAPSIMPLE. */
#define HGSMI_HEAP_TYPE_OFFSET  2 /* Deprecated, used only for old saved states. RTHEAPOFFSET. */
#define HGSMI_HEAP_TYPE_MA      3 /* Memory allocator. */

typedef struct HGSMIHOSTHEAP
{
    uint32_t u32HeapType;   /* HGSMI_HEAP_TYPE_* */
    int32_t volatile cRefs; /* How many blocks allocated. */
    HGSMIAREA area;         /* Host heap location. */
    union
    {
        HGSMIMADATA ma;     /* Memory allocator for the default host heap implementation. */
        struct              /* Legacy heap implementations. For old saved states. */
        {
            union
            {
                RTHEAPSIMPLE hPtr;  /* Pointer based heap. */
                RTHEAPOFFSET hOff;  /* Offset based heap. */
            } u;
        } legacy;
    } u;
} HGSMIHOSTHEAP;

typedef struct HGSMIINSTANCE
{
    PVM pVM;                           /* The VM. */

    const char *pszName;               /* A name for the instance. Mostyl used in the log. */

    RTCRITSECT   instanceCritSect;     /* For updating the instance data: FIFO's, channels. */

    HGSMIAREA area; /* The shared memory description. */
    HGSMIHOSTHEAP hostHeap;            /* Host heap instance. */
    RTCRITSECT    hostHeapCritSect;    /* Heap serialization lock. */

    RTLISTANCHOR hostFIFO;             /* Pending host buffers. */
    RTLISTANCHOR hostFIFORead;         /* Host buffers read by the guest. */
    RTLISTANCHOR hostFIFOProcessed;    /* Processed by the guest. */
    RTLISTANCHOR hostFIFOFree;         /* Buffers for reuse. */
#ifdef VBOX_WITH_WDDM
    RTLISTANCHOR guestCmdCompleted;    /* list of completed guest commands to be returned to the guest*/
#endif
    RTCRITSECT hostFIFOCritSect;       /* FIFO serialization lock. */

    PFNHGSMINOTIFYGUEST pfnNotifyGuest; /* Guest notification callback. */
    void *pvNotifyGuest;                /* Guest notification callback context. */

    volatile HGSMIHOSTFLAGS *pHGFlags;

    HGSMICHANNELINFO channelInfo;      /* Channel handlers indexed by the channel id.
                                        * The array is accessed under the instance lock.
                                        */
} HGSMIINSTANCE;


typedef DECLCALLBACK(void) FNHGSMIHOSTFIFOCALLBACK(void *pvCallback);
typedef FNHGSMIHOSTFIFOCALLBACK *PFNHGSMIHOSTFIFOCALLBACK;

typedef struct HGSMIHOSTFIFOENTRY
{
    RTLISTNODE nodeEntry;

    HGSMIINSTANCE *pIns;               /* Backlink to the HGSMI instance. */

    volatile uint32_t fl;              /* Status flags of the entry. */

    HGSMIOFFSET offBuffer;             /* Offset of the HGSMI buffer header in the HGSMI host heap:
                                        * [pIns->hostHeap.area.offBase .. offLast]. */
} HGSMIHOSTFIFOENTRY;


#define HGSMI_F_HOST_FIFO_ALLOCATED 0x0001
#define HGSMI_F_HOST_FIFO_QUEUED    0x0002
#define HGSMI_F_HOST_FIFO_READ      0x0004
#define HGSMI_F_HOST_FIFO_PROCESSED 0x0008
#define HGSMI_F_HOST_FIFO_FREE      0x0010
#define HGSMI_F_HOST_FIFO_CANCELED  0x0020

static DECLCALLBACK(void) hgsmiHostCommandFreeCallback (void *pvCallback);

#ifdef VBOX_WITH_WDDM

typedef struct HGSMIGUESTCOMPLENTRY
{
    RTLISTNODE nodeEntry;
    HGSMIOFFSET offBuffer; /* Offset of the guest command buffer. */
} HGSMIGUESTCOMPLENTRY;


static void hgsmiGuestCompletionFIFOFree (HGSMIINSTANCE *pIns, HGSMIGUESTCOMPLENTRY *pEntry)
{
    NOREF (pIns);
    RTMemFree (pEntry);
}

static int hgsmiGuestCompletionFIFOAlloc (HGSMIINSTANCE *pIns, HGSMIGUESTCOMPLENTRY **ppEntry)
{
    int rc = VINF_SUCCESS;

    NOREF (pIns);

    HGSMIGUESTCOMPLENTRY *pEntry = (HGSMIGUESTCOMPLENTRY *)RTMemAllocZ (sizeof (HGSMIGUESTCOMPLENTRY));

    if (pEntry)
        *ppEntry = pEntry;
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

#endif

static int hgsmiLock (HGSMIINSTANCE *pIns)
{
    int rc = RTCritSectEnter (&pIns->instanceCritSect);
    AssertRC (rc);
    return rc;
}

static void hgsmiUnlock (HGSMIINSTANCE *pIns)
{
    int rc = RTCritSectLeave (&pIns->instanceCritSect);
    AssertRC (rc);
}

static int hgsmiFIFOLock (HGSMIINSTANCE *pIns)
{
    int rc = RTCritSectEnter (&pIns->hostFIFOCritSect);
    AssertRC (rc);
    return rc;
}

static void hgsmiFIFOUnlock (HGSMIINSTANCE *pIns)
{
    int rc = RTCritSectLeave (&pIns->hostFIFOCritSect);
    AssertRC (rc);
}

/*
 * Virtual hardware IO handlers.
 */

/* The guest submits a new buffer to the host.
 * Called from the HGSMI_IO_GUEST write handler.
 * @thread EMT
 */
void HGSMIGuestWrite (PHGSMIINSTANCE pIns,
                      HGSMIOFFSET offBuffer)
{
    HGSMIBufferProcess (&pIns->area, &pIns->channelInfo, offBuffer);
}

#ifdef VBOX_WITH_WDDM
static HGSMIOFFSET hgsmiProcessGuestCmdCompletion(HGSMIINSTANCE *pIns)
{
    HGSMIOFFSET offCmd = HGSMIOFFSET_VOID;
    int rc = hgsmiFIFOLock(pIns);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
    {
        HGSMIGUESTCOMPLENTRY *pEntry = RTListGetFirst(&pIns->guestCmdCompleted, HGSMIGUESTCOMPLENTRY, nodeEntry);
        if (pEntry)
        {
            RTListNodeRemove(&pEntry->nodeEntry);
        }

        if (RTListIsEmpty(&pIns->guestCmdCompleted))
        {
            if (pIns->pHGFlags)
                ASMAtomicAndU32(&pIns->pHGFlags->u32HostFlags, ~HGSMIHOSTFLAGS_GCOMMAND_COMPLETED);
        }

        hgsmiFIFOUnlock(pIns);

        if (pEntry)
        {
            offCmd = pEntry->offBuffer;

            LogFlowFunc(("host FIFO head %p.\n", pEntry));

            hgsmiGuestCompletionFIFOFree(pIns, pEntry);
        }
    }
    return offCmd;
}
#endif


/* Called from HGSMI_IO_GUEST read handler. */
HGSMIOFFSET HGSMIGuestRead (PHGSMIINSTANCE pIns)
{
    LogFlowFunc(("pIns %p\n", pIns));

    AssertPtr(pIns);

    VM_ASSERT_EMT(pIns->pVM);

#ifndef VBOX_WITH_WDDM
    /* Currently there is no functionality here. */
    NOREF(pIns);

    return HGSMIOFFSET_VOID;
#else
    /* use this to speedup guest cmd completion
     * this mechanism is alternative to submitting H->G command for notification */
    HGSMIOFFSET offCmd = hgsmiProcessGuestCmdCompletion(pIns);
    return offCmd;
#endif
}

static bool hgsmiProcessHostCmdCompletion(HGSMIINSTANCE *pIns,
                                          HGSMIOFFSET offBuffer,
                                          bool bCompleteFirst)
{
    VM_ASSERT_EMT(pIns->pVM);

    int rc = hgsmiFIFOLock(pIns);
    if (RT_SUCCESS(rc))
    {
        /* Search the Read list for the given buffer offset. */
        HGSMIHOSTFIFOENTRY *pEntry = NULL;

        HGSMIHOSTFIFOENTRY *pIter;
        RTListForEach(&pIns->hostFIFORead, pIter, HGSMIHOSTFIFOENTRY, nodeEntry)
        {
            Assert(pIter->fl == (HGSMI_F_HOST_FIFO_ALLOCATED | HGSMI_F_HOST_FIFO_READ));
            if (bCompleteFirst || pIter->offBuffer == offBuffer)
            {
                pEntry = pIter;
                break;
            }
        }

        LogFlowFunc(("read list entry: %p.\n", pEntry));

        Assert(pEntry || bCompleteFirst);

        if (pEntry)
        {
            RTListNodeRemove(&pEntry->nodeEntry);

            pEntry->fl &= ~HGSMI_F_HOST_FIFO_READ;
            pEntry->fl |= HGSMI_F_HOST_FIFO_PROCESSED;

            RTListAppend(&pIns->hostFIFOProcessed, &pEntry->nodeEntry);

            hgsmiFIFOUnlock(pIns);

            hgsmiHostCommandFreeCallback(pEntry);

            return true;
        }

        hgsmiFIFOUnlock(pIns);
        if (!bCompleteFirst)
            LogRel(("HGSMI[%s]: ignored invalid write to the host FIFO: 0x%08X!!!\n", pIns->pszName, offBuffer));
    }
    return false;
}

/* The guest has finished processing of a buffer previously submitted by the host.
 * Called from HGSMI_IO_HOST write handler.
 * @thread EMT
 */
void HGSMIHostWrite (HGSMIINSTANCE *pIns,
                     HGSMIOFFSET offBuffer)
{
    LogFlowFunc(("pIns %p offBuffer 0x%x\n", pIns, offBuffer));

    hgsmiProcessHostCmdCompletion (pIns, offBuffer, false);
}

/* The guest reads a new host buffer to be processed.
 * Called from the HGSMI_IO_HOST read handler.
 * @thread EMT
 */
HGSMIOFFSET HGSMIHostRead (HGSMIINSTANCE *pIns)
{
    LogFlowFunc(("pIns %p\n", pIns));

    VM_ASSERT_EMT(pIns->pVM);

    AssertPtrReturn(pIns->pHGFlags, HGSMIOFFSET_VOID);
    int rc = hgsmiFIFOLock(pIns);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
    {
        /* Get the host FIFO head entry. */
        HGSMIHOSTFIFOENTRY *pEntry = RTListGetFirst(&pIns->hostFIFO, HGSMIHOSTFIFOENTRY, nodeEntry);

        LogFlowFunc(("host FIFO head %p.\n", pEntry));

        if (pEntry != NULL)
        {
            Assert(pEntry->fl == (HGSMI_F_HOST_FIFO_ALLOCATED | HGSMI_F_HOST_FIFO_QUEUED));

            /*
             * Move the entry to the Read list.
             */
            RTListNodeRemove(&pEntry->nodeEntry);

            if (RTListIsEmpty(&pIns->hostFIFO))
            {
                ASMAtomicAndU32(&pIns->pHGFlags->u32HostFlags, (~HGSMIHOSTFLAGS_COMMANDS_PENDING));
            }

            pEntry->fl &= ~HGSMI_F_HOST_FIFO_QUEUED;
            pEntry->fl |= HGSMI_F_HOST_FIFO_READ;

            RTListAppend(&pIns->hostFIFORead, &pEntry->nodeEntry);

            hgsmiFIFOUnlock(pIns);

            /* Return the buffer offset of the host FIFO head. */
            return pEntry->offBuffer;
        }

        hgsmiFIFOUnlock(pIns);
    }
    /* Special value that means there is no host buffers to be processed. */
    return HGSMIOFFSET_VOID;
}


/* Tells the guest that a new buffer to be processed is available from the host. */
static void hgsmiNotifyGuest (HGSMIINSTANCE *pIns)
{
    if (pIns->pfnNotifyGuest)
    {
        pIns->pfnNotifyGuest (pIns->pvNotifyGuest);
    }
}

void HGSMISetHostGuestFlags(HGSMIINSTANCE *pIns, uint32_t flags)
{
    AssertPtrReturnVoid(pIns->pHGFlags);
    ASMAtomicOrU32(&pIns->pHGFlags->u32HostFlags, flags);
}

uint32_t HGSMIGetHostGuestFlags(HGSMIINSTANCE *pIns)
{
    return pIns->pHGFlags? ASMAtomicReadU32(&pIns->pHGFlags->u32HostFlags) : 0;
}

void HGSMIClearHostGuestFlags(HGSMIINSTANCE *pIns, uint32_t flags)
{
    AssertPtrReturnVoid(pIns->pHGFlags);
    ASMAtomicAndU32(&pIns->pHGFlags->u32HostFlags, (~flags));
}

/*
 * The host heap.
 *
 * Uses the RTHeap implementation.
 *
 */
static int hgsmiHostHeapLock (HGSMIINSTANCE *pIns)
{
    int rc = RTCritSectEnter (&pIns->hostHeapCritSect);
    AssertRC (rc);
    return rc;
}

static void hgsmiHostHeapUnlock (HGSMIINSTANCE *pIns)
{
    int rc = RTCritSectLeave (&pIns->hostHeapCritSect);
    AssertRC (rc);
}

static HGSMIOFFSET hgsmiHostHeapOffset(HGSMIHOSTHEAP *pHeap)
{
    return pHeap->area.offBase;
}

static HGSMISIZE hgsmiHostHeapSize(HGSMIHOSTHEAP *pHeap)
{
    return pHeap->area.cbArea;
}

static void *hgsmiHostHeapBufferAlloc(HGSMIHOSTHEAP *pHeap,
                                      HGSMISIZE cbBuffer)
{
    void *pvBuf = NULL;

    if (pHeap->u32HeapType == HGSMI_HEAP_TYPE_MA)
    {
        pvBuf = HGSMIMAAlloc(&pHeap->u.ma, cbBuffer);
    }
    else if (pHeap->u32HeapType == HGSMI_HEAP_TYPE_POINTER)
    {
        pvBuf = RTHeapSimpleAlloc(pHeap->u.legacy.u.hPtr, cbBuffer, 0);
    }
    else if (pHeap->u32HeapType == HGSMI_HEAP_TYPE_OFFSET)
    {
        pvBuf = RTHeapOffsetAlloc(pHeap->u.legacy.u.hOff, cbBuffer, 0);
    }

    if (pvBuf)
    {
        ++pHeap->cRefs;
    }

    return pvBuf;
}

static void hgsmiHostHeapBufferFree(HGSMIHOSTHEAP *pHeap,
                                    void *pvBuf)
{
    if (pHeap->u32HeapType == HGSMI_HEAP_TYPE_MA)
    {
        HGSMIMAFree(&pHeap->u.ma, pvBuf);
    }
    else if (pHeap->u32HeapType == HGSMI_HEAP_TYPE_POINTER)
    {
        RTHeapSimpleFree(pHeap->u.legacy.u.hPtr, pvBuf);
    }
    else if (pHeap->u32HeapType == HGSMI_HEAP_TYPE_OFFSET)
    {
        RTHeapOffsetFree(pHeap->u.legacy.u.hOff, pvBuf);
    }
    --pHeap->cRefs;
}

static void *hgsmiHostHeapDataAlloc(HGSMIHOSTHEAP *pHeap,
                                    HGSMISIZE cbData,
                                    uint8_t u8Channel,
                                    uint16_t u16ChannelInfo)
{
    HGSMISIZE cbAlloc = HGSMIBufferRequiredSize(cbData);
    HGSMIBUFFERHEADER *pHeader = (HGSMIBUFFERHEADER *)hgsmiHostHeapBufferAlloc(pHeap, cbAlloc);
    if (!pHeader)
        return NULL;

    HGSMIBufferInitializeSingle(&pHeap->area, pHeader, cbAlloc, u8Channel, u16ChannelInfo);

    return HGSMIBufferDataFromPtr(pHeader);
}

static void hgsmiHostHeapDataFree(HGSMIHOSTHEAP *pHeap,
                                  void *pvData)
{
    if (   pvData
        && pHeap->u32HeapType != HGSMI_HEAP_TYPE_NULL)
    {
        HGSMIBUFFERHEADER *pHeader = HGSMIBufferHeaderFromData(pvData);
        hgsmiHostHeapBufferFree(pHeap, pHeader);
    }
}

/* Needed for heap relocation: offset of the heap handle relative to the start of heap area. */
static HGSMIOFFSET hgsmiHostHeapHandleLocationOffset(HGSMIHOSTHEAP *pHeap)
{
    HGSMIOFFSET offHeapHandle;
    if (pHeap->u32HeapType == HGSMI_HEAP_TYPE_POINTER)
    {
        offHeapHandle = (HGSMIOFFSET)((uintptr_t)pHeap->u.legacy.u.hPtr - (uintptr_t)pHeap->area.pu8Base);
    }
    else if (pHeap->u32HeapType == HGSMI_HEAP_TYPE_OFFSET)
    {
        offHeapHandle = (HGSMIOFFSET)((uintptr_t)pHeap->u.legacy.u.hOff - (uintptr_t)pHeap->area.pu8Base);
    }
    else
    {
        offHeapHandle = HGSMIOFFSET_VOID;
    }
    return offHeapHandle;
}

static int hgsmiHostHeapRelocate(HGSMIHOSTHEAP *pHeap,
                                 uint32_t u32HeapType,
                                 void *pvBase,
                                 uint32_t offHeapHandle,
                                 uintptr_t offDelta,
                                 HGSMISIZE cbArea,
                                 HGSMIOFFSET offBase)
{
    int rc = HGSMIAreaInitialize(&pHeap->area, pvBase, cbArea, offBase);
    if (RT_SUCCESS(rc))
    {
        if (u32HeapType == HGSMI_HEAP_TYPE_OFFSET)
        {
            pHeap->u.legacy.u.hOff = (RTHEAPOFFSET)((uint8_t *)pvBase + offHeapHandle);
        }
        else if (u32HeapType == HGSMI_HEAP_TYPE_POINTER)
        {
            pHeap->u.legacy.u.hPtr = (RTHEAPSIMPLE)((uint8_t *)pvBase + offHeapHandle);
            rc = RTHeapSimpleRelocate(pHeap->u.legacy.u.hPtr, offDelta); AssertRC(rc);
        }
        else
        {
            /* HGSMI_HEAP_TYPE_MA does not need the relocation. */
            rc = VERR_NOT_SUPPORTED;
        }

        if (RT_SUCCESS(rc))
        {
            pHeap->u32HeapType = u32HeapType;
        }
        else
        {
            HGSMIAreaClear(&pHeap->area);
        }
    }

    return rc;
}

static int hgsmiHostHeapRestoreMA(HGSMIHOSTHEAP *pHeap,
                                  void *pvBase,
                                  HGSMISIZE cbArea,
                                  HGSMIOFFSET offBase,
                                  uint32_t cBlocks,
                                  HGSMIOFFSET *paDescriptors,
                                  HGSMISIZE cbMaxBlock,
                                  HGSMIENV *pEnv)
{
    int rc = HGSMIAreaInitialize(&pHeap->area, pvBase, cbArea, offBase);
    if (RT_SUCCESS(rc))
    {
        rc = HGSMIMAInit(&pHeap->u.ma, &pHeap->area, paDescriptors, cBlocks, cbMaxBlock, pEnv);

        if (RT_SUCCESS(rc))
        {
            pHeap->u32HeapType = HGSMI_HEAP_TYPE_MA;
        }
        else
        {
            HGSMIAreaClear(&pHeap->area);
        }
    }

    return rc;
}

static void hgsmiHostHeapSetupUninitialized(HGSMIHOSTHEAP *pHeap)
{
    RT_ZERO(*pHeap);
    pHeap->u32HeapType = HGSMI_HEAP_TYPE_NULL;
}

static void hgsmiHostHeapDestroy(HGSMIHOSTHEAP *pHeap)
{
    if (pHeap->u32HeapType == HGSMI_HEAP_TYPE_MA)
    {
        HGSMIMAUninit(&pHeap->u.ma);
    }
    hgsmiHostHeapSetupUninitialized(pHeap);
}

static int hgsmiHostFIFOAlloc (HGSMIINSTANCE *pIns, HGSMIHOSTFIFOENTRY **ppEntry)
{
    int rc = VINF_SUCCESS;

    NOREF (pIns);

    HGSMIHOSTFIFOENTRY *pEntry = (HGSMIHOSTFIFOENTRY *)RTMemAllocZ (sizeof (HGSMIHOSTFIFOENTRY));

    if (pEntry)
    {
        pEntry->fl = HGSMI_F_HOST_FIFO_ALLOCATED;
    }
    else
    {
        rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS (rc))
    {
        *ppEntry = pEntry;
    }

    return rc;
}

static void hgsmiHostFIFOFree (HGSMIINSTANCE *pIns, HGSMIHOSTFIFOENTRY *pEntry)
{
    NOREF (pIns);
    RTMemFree (pEntry);
}

static int hgsmiHostCommandFreeByEntry (HGSMIHOSTFIFOENTRY *pEntry)
{
    LogFlowFunc(("offBuffer 0x%08X\n", pEntry->offBuffer));

    HGSMIINSTANCE *pIns = pEntry->pIns;
    int rc = hgsmiFIFOLock (pIns);
    if (RT_SUCCESS(rc))
    {
        RTListNodeRemove(&pEntry->nodeEntry);
        hgsmiFIFOUnlock (pIns);

        void *pvData = HGSMIBufferDataFromOffset(&pIns->hostHeap.area, pEntry->offBuffer);

        rc = hgsmiHostHeapLock (pIns);
        if (RT_SUCCESS(rc))
        {
            /* Deallocate the host heap memory. */
            hgsmiHostHeapDataFree(&pIns->hostHeap, pvData);

            hgsmiHostHeapUnlock(pIns);
        }

        hgsmiHostFIFOFree (pIns, pEntry);
    }

    LogFlowFunc(("%Rrc\n", rc));
    return rc;
}

static int hgsmiHostCommandFree(HGSMIINSTANCE *pIns,
                                void *pvData)
{
    HGSMIOFFSET offBuffer = HGSMIBufferOffsetFromData(&pIns->hostHeap.area, pvData);
    HGSMIHOSTFIFOENTRY *pEntry = NULL;

    int rc = hgsmiFIFOLock(pIns);
    if (RT_SUCCESS(rc))
    {
        /* Search the Processed list for the given offBuffer. */
        HGSMIHOSTFIFOENTRY *pIter;
        RTListForEach(&pIns->hostFIFOProcessed, pIter, HGSMIHOSTFIFOENTRY, nodeEntry)
        {
            Assert(pIter->fl == (HGSMI_F_HOST_FIFO_ALLOCATED | HGSMI_F_HOST_FIFO_PROCESSED));

            if (pIter->offBuffer == offBuffer)
            {
                pEntry = pIter;
                break;
            }
        }

        if (pEntry)
        {
            RTListNodeRemove(&pEntry->nodeEntry);
        }
        else
        {
            AssertLogRelMsgFailed(("HGSMI[%s]: the host frees unprocessed FIFO entry: 0x%08X\n",
                                   pIns->pszName, offBuffer));
        }

        hgsmiFIFOUnlock (pIns);

        rc = hgsmiHostHeapLock (pIns);
        if (RT_SUCCESS(rc))
        {
            /* Deallocate the host heap memory. */
            hgsmiHostHeapDataFree(&pIns->hostHeap, pvData);

            hgsmiHostHeapUnlock(pIns);
        }

        if (pEntry)
        {
            /* Deallocate the entry. */
            hgsmiHostFIFOFree(pIns, pEntry);
        }
    }

    return rc;
}

static DECLCALLBACK(void) hgsmiHostCommandFreeCallback (void *pvCallback)
{
    /* Guest has processed the command. */
    HGSMIHOSTFIFOENTRY *pEntry = (HGSMIHOSTFIFOENTRY *)pvCallback;

    Assert(pEntry->fl == (HGSMI_F_HOST_FIFO_ALLOCATED | HGSMI_F_HOST_FIFO_PROCESSED));

    /* This is a simple callback, just signal the event. */
    hgsmiHostCommandFreeByEntry (pEntry);
}

static int hgsmiHostCommandWrite(HGSMIINSTANCE *pIns,
                                 HGSMIOFFSET offBuffer)
{
    AssertPtrReturn(pIns->pHGFlags, VERR_WRONG_ORDER);

    HGSMIHOSTFIFOENTRY *pEntry;
    int rc = hgsmiHostFIFOAlloc(pIns, &pEntry);

    if (RT_SUCCESS(rc))
    {
        /* Initialize the new entry and add it to the FIFO. */
        pEntry->fl |= HGSMI_F_HOST_FIFO_QUEUED;

        pEntry->pIns = pIns;
        pEntry->offBuffer = offBuffer;

        rc = hgsmiFIFOLock(pIns);
        if (RT_SUCCESS(rc))
        {
            ASMAtomicOrU32(&pIns->pHGFlags->u32HostFlags, HGSMIHOSTFLAGS_COMMANDS_PENDING);
            RTListAppend(&pIns->hostFIFO, &pEntry->nodeEntry);

            hgsmiFIFOUnlock(pIns);
        }
        else
        {
            hgsmiHostFIFOFree(pIns, pEntry);
        }
    }

    return rc;
}


/**
 * Append the shared memory block to the FIFO, inform the guest.
 *
 * @param pIns       Pointer to HGSMI instance.
 * @param pvData     The shared memory block data pointer.
 * @param fDoIrq     Whether the guest interrupt should be generated, i.e. if the command is not
 *                   urgent (e.g. some guest command completion notification that does not require
 *                   post-processing) the command could be submitted without raising an irq.
 * @thread EMT
 */
static int hgsmiHostCommandSubmit(HGSMIINSTANCE *pIns,
                                  void *pvData,
                                  bool fDoIrq)
{
    /* Append the command to FIFO. */
    HGSMIOFFSET offBuffer = HGSMIBufferOffsetFromData(&pIns->hostHeap.area, pvData);
    int rc = hgsmiHostCommandWrite(pIns, offBuffer);
    if (RT_SUCCESS(rc))
    {
        if (fDoIrq)
        {
            /* Now guest can read the FIFO, the notification is informational. */
            hgsmiNotifyGuest(pIns);
        }
    }

    return rc;
}

/**
 * Allocate a shared memory buffer. The host can write command/data to the memory.
 * The allocated buffer contains the 'header', 'data' and the 'tail', but *ppvData
 * will point to the 'data'.
 *
 * @return VBox status code. Pointer to the payload data in *ppvData.
 * @param pIns           HGSMI instance,
 * @param ppvData        Where to store the allocated memory pointer to data.
 * @param cbData         How many bytes of data to allocate.
 * @param u8Channel      HGSMI channel.
 * @param u16ChannelInfo Command parameter.
 */
int HGSMIHostCommandAlloc(HGSMIINSTANCE *pIns,
                          void **ppvData,
                          HGSMISIZE cbData,
                          uint8_t u8Channel,
                          uint16_t u16ChannelInfo)
{
    LogFlowFunc(("pIns = %p, cbData = %d, u8Channel %d, u16ChannelInfo 0x%04X\n",
                 pIns, cbData, u8Channel, u16ChannelInfo));

    int rc = hgsmiHostHeapLock(pIns);
    if (RT_SUCCESS(rc))
    {
        void *pvData = hgsmiHostHeapDataAlloc(&pIns->hostHeap,
                                              cbData,
                                              u8Channel,
                                              u16ChannelInfo);
        hgsmiHostHeapUnlock(pIns);

        if (pvData)
        {
            *ppvData = pvData;
        }
        else
        {
            LogRel(("HGSMI[%s]: host heap allocation failed %d bytes\n", pIns->pszName, cbData));
            rc = VERR_NO_MEMORY;
        }
    }

    LogFlowFunc(("%Rrc, pvData = %p\n", rc, *ppvData));
    return rc;
}

/**
 * Convenience function that allows posting the host command asynchronously
 * and make it freed on completion.
 * The caller does not get notified in any way on command completion,
 * on successful return the pvData buffer can not be used after being passed to this function.
 *
 * @param pIns   HGSMI instance,
 * @param pvData The pointer returned by 'HGSMIHostCommandAlloc'.
 * @param fDoIrq Specifies whether the guest interrupt should be generated.
 *               In case the command is not urgent (e.g. some guest command
 *               completion notification that does not require post-processing)
 *               the command could be posted without raising an irq.
 */
int HGSMIHostCommandSubmitAndFreeAsynch(PHGSMIINSTANCE pIns,
                                        void *pvData,
                                        bool fDoIrq)
{
    LogFlowFunc(("pIns = %p, pvData = %p, fDoIrq = %d\n", pIns, pvData, fDoIrq));

    int rc;
    if (HGSMIAreaContainsPointer(&pIns->hostHeap.area, pvData))
    {
        rc = hgsmiHostCommandSubmit(pIns, pvData, fDoIrq);
    }
    else
    {
        AssertLogRelMsgFailed(("HGSMI[%s]: host submits invalid command %p/%p\n",
                               pIns->pszName, pvData, pIns->hostHeap.area.pu8Base));
        rc = VERR_INVALID_POINTER;
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

/**
 * Free the shared memory block.
 *
 * @param pIns   Pointer to HGSMI instance,
 * @param pvData The pointer returned by 'HGSMIHostCommandAlloc'.
 */
int HGSMIHostCommandFree(HGSMIINSTANCE *pIns,
                         void *pvData)
{
    LogFlowFunc(("pIns = %p, pvData = %p\n", pIns, pvData));

    int rc;
    if (HGSMIAreaContainsPointer(&pIns->hostHeap.area, pvData))
    {
        rc = hgsmiHostCommandFree(pIns, pvData);
    }
    else
    {
        AssertLogRelMsgFailed(("HGSMI[%s]: the host frees invalid FIFO entry %p/%p\n",
                               pIns->pszName, pvData, pIns->hostHeap.area.pu8Base));
        rc = VERR_INVALID_POINTER;
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

static DECLCALLBACK(void *) hgsmiEnvAlloc(void *pvEnv, HGSMISIZE cb)
{
    NOREF(pvEnv);
    return RTMemAlloc(cb);
}

static DECLCALLBACK(void) hgsmiEnvFree(void *pvEnv, void *pv)
{
    NOREF(pvEnv);
    RTMemFree(pv);
}

static HGSMIENV g_hgsmiEnv =
{
    NULL,
    hgsmiEnvAlloc,
    hgsmiEnvFree
};

int HGSMIHostHeapSetup(PHGSMIINSTANCE pIns,
                       HGSMIOFFSET    offHeap,
                       HGSMISIZE      cbHeap)
{
    LogFlowFunc(("pIns %p, offHeap 0x%08X, cbHeap = 0x%08X\n", pIns, offHeap, cbHeap));

    int rc = VINF_SUCCESS;

    AssertPtrReturn(pIns, VERR_INVALID_PARAMETER);

    if (   offHeap >= pIns->area.cbArea
        || cbHeap > pIns->area.cbArea
        || offHeap > pIns->area.cbArea - cbHeap)
    {
        AssertLogRelMsgFailed(("offHeap 0x%08X, cbHeap = 0x%08X, pIns->area.cbArea 0x%08X\n",
                               offHeap, cbHeap, pIns->area.cbArea));
        rc = VERR_INVALID_PARAMETER;
    }
    else
    {
        rc = hgsmiHostHeapLock (pIns);

        if (RT_SUCCESS (rc))
        {
            if (pIns->hostHeap.cRefs)
            {
                AssertLogRelMsgFailed(("HGSMI[%s]: host heap setup ignored. %d allocated.\n",
                                       pIns->pszName, pIns->hostHeap.cRefs));
                /* It is possible to change the heap only if there is no pending allocations. */
                rc = VERR_ACCESS_DENIED;
            }
            else
            {
                rc = HGSMIAreaInitialize(&pIns->hostHeap.area, pIns->area.pu8Base + offHeap, cbHeap, offHeap);
                if (RT_SUCCESS(rc))
                {
                    rc = HGSMIMAInit(&pIns->hostHeap.u.ma, &pIns->hostHeap.area, NULL, 0, 0, &g_hgsmiEnv);
                }

                if (RT_SUCCESS(rc))
                {
                    pIns->hostHeap.u32HeapType = HGSMI_HEAP_TYPE_MA;
                }
                else
                {
                    HGSMIAreaClear(&pIns->hostHeap.area);
                }
            }

            hgsmiHostHeapUnlock (pIns);
        }
    }

    LogFlowFunc(("rc = %Rrc\n", rc));

    return rc;
}

static int hgsmiHostSaveFifoLocked(RTLISTANCHOR *pList, PSSMHANDLE pSSM)
{
    VBOXHGSMI_SAVE_FIFOSTART(pSSM);

    HGSMIHOSTFIFOENTRY *pIter;

    uint32_t cEntries = 0;
    RTListForEach(pList, pIter, HGSMIHOSTFIFOENTRY, nodeEntry)
    {
        ++cEntries;
    }

    int rc = SSMR3PutU32(pSSM, cEntries);
    if (RT_SUCCESS(rc))
    {
        RTListForEach(pList, pIter, HGSMIHOSTFIFOENTRY, nodeEntry)
        {
            SSMR3PutU32(pSSM, pIter->fl);
            rc = SSMR3PutU32(pSSM, pIter->offBuffer);
            if (RT_FAILURE(rc))
            {
                break;
            }
        }
    }

    VBOXHGSMI_SAVE_FIFOSTOP(pSSM);

    return rc;
}

static int hgsmiHostSaveGuestCmdCompletedFifoLocked(RTLISTANCHOR *pList, PSSMHANDLE pSSM)
{
    VBOXHGSMI_SAVE_FIFOSTART(pSSM);

    HGSMIGUESTCOMPLENTRY *pIter;

    uint32_t cEntries = 0;
    RTListForEach(pList, pIter, HGSMIGUESTCOMPLENTRY, nodeEntry)
    {
        ++cEntries;
    }
    int rc = SSMR3PutU32(pSSM, cEntries);
    if (RT_SUCCESS(rc))
    {
        RTListForEach(pList, pIter, HGSMIGUESTCOMPLENTRY, nodeEntry)
        {
            rc = SSMR3PutU32(pSSM, pIter->offBuffer);
            if (RT_FAILURE(rc))
            {
                break;
            }
        }
    }

    VBOXHGSMI_SAVE_FIFOSTOP(pSSM);

    return rc;
}

static int hgsmiHostLoadFifoEntryLocked (PHGSMIINSTANCE pIns, HGSMIHOSTFIFOENTRY **ppEntry, PSSMHANDLE pSSM)
{
    HGSMIHOSTFIFOENTRY *pEntry;
    int rc = hgsmiHostFIFOAlloc (pIns, &pEntry);  AssertRC(rc);
    if (RT_SUCCESS (rc))
    {
        uint32_t u32;
        pEntry->pIns = pIns;
        rc = SSMR3GetU32 (pSSM, &u32); AssertRC(rc);
        pEntry->fl = u32;
        rc = SSMR3GetU32 (pSSM, &pEntry->offBuffer); AssertRC(rc);
        if (RT_SUCCESS (rc))
            *ppEntry = pEntry;
        else
            hgsmiHostFIFOFree (pIns, pEntry);
    }

    return rc;
}

static int hgsmiHostLoadFifoLocked(PHGSMIINSTANCE pIns, RTLISTANCHOR *pList, PSSMHANDLE pSSM)
{
    VBOXHGSMI_LOAD_FIFOSTART(pSSM);

    uint32_t cEntries = 0;
    int rc = SSMR3GetU32(pSSM, &cEntries);
    if (RT_SUCCESS(rc) && cEntries)
    {
        uint32_t i;
        for (i = 0; i < cEntries; ++i)
        {
            HGSMIHOSTFIFOENTRY *pEntry = NULL;
            rc = hgsmiHostLoadFifoEntryLocked(pIns, &pEntry, pSSM);
            AssertRCBreak(rc);

            RTListAppend(pList, &pEntry->nodeEntry);
        }
    }

    VBOXHGSMI_LOAD_FIFOSTOP(pSSM);

    return rc;
}

static int hgsmiHostLoadGuestCmdCompletedFifoEntryLocked (PHGSMIINSTANCE pIns, HGSMIGUESTCOMPLENTRY **ppEntry, PSSMHANDLE pSSM)
{
    HGSMIGUESTCOMPLENTRY *pEntry;
    int rc = hgsmiGuestCompletionFIFOAlloc (pIns, &pEntry); AssertRC(rc);
    if (RT_SUCCESS (rc))
    {
        rc = SSMR3GetU32 (pSSM, &pEntry->offBuffer); AssertRC(rc);
        if (RT_SUCCESS (rc))
            *ppEntry = pEntry;
        else
            hgsmiGuestCompletionFIFOFree (pIns, pEntry);
    }
    return rc;
}

static int hgsmiHostLoadGuestCmdCompletedFifoLocked(PHGSMIINSTANCE pIns, RTLISTANCHOR *pList, PSSMHANDLE pSSM, uint32_t u32Version)
{
    VBOXHGSMI_LOAD_FIFOSTART(pSSM);

    uint32_t i;

    uint32_t cEntries = 0;
    int rc = SSMR3GetU32(pSSM, &cEntries);
    if (RT_SUCCESS(rc) && cEntries)
    {
        if (u32Version > VGA_SAVEDSTATE_VERSION_INV_GCMDFIFO)
        {
            for (i = 0; i < cEntries; ++i)
            {
                HGSMIGUESTCOMPLENTRY *pEntry = NULL;
                rc = hgsmiHostLoadGuestCmdCompletedFifoEntryLocked(pIns, &pEntry, pSSM);
                AssertRCBreak(rc);

                RTListAppend(pList, &pEntry->nodeEntry);
            }
        }
        else
        {
            LogRel(("WARNING: the current saved state version has some 3D support data missing, "
                    "which may lead to some guest applications function improperly"));

            /* Just read out all invalid data and discard it. */
            for (i = 0; i < cEntries; ++i)
            {
                HGSMIHOSTFIFOENTRY *pEntry = NULL;
                rc = hgsmiHostLoadFifoEntryLocked(pIns, &pEntry, pSSM);
                AssertRCBreak(rc);

                hgsmiHostFIFOFree(pIns, pEntry);
            }
        }
    }

    VBOXHGSMI_LOAD_FIFOSTOP(pSSM);

    return rc;
}

static int hgsmiHostSaveMA(PSSMHANDLE pSSM, HGSMIMADATA *pMA)
{
    int rc = SSMR3PutU32(pSSM, pMA->cBlocks);
    if (RT_SUCCESS(rc))
    {
        HGSMIMABLOCK *pIter;
        RTListForEach(&pMA->listBlocks, pIter, HGSMIMABLOCK, nodeBlock)
        {
            SSMR3PutU32(pSSM, pIter->descriptor);
        }

        rc = SSMR3PutU32(pSSM, pMA->cbMaxBlock);
    }

    return rc;
}

static int hgsmiHostLoadMA(PSSMHANDLE pSSM, uint32_t *pcBlocks, HGSMIOFFSET **ppaDescriptors, HGSMISIZE *pcbMaxBlock)
{
    int rc = SSMR3GetU32(pSSM, pcBlocks);
    if (RT_SUCCESS(rc))
    {
        HGSMIOFFSET *paDescriptors = NULL;
        if (*pcBlocks > 0)
        {
            paDescriptors = (HGSMIOFFSET *)RTMemAlloc(*pcBlocks * sizeof(HGSMIOFFSET));
            if (paDescriptors)
            {
                uint32_t i;
                for (i = 0; i < *pcBlocks; ++i)
                {
                    SSMR3GetU32(pSSM, &paDescriptors[i]);
                }
            }
            else
            {
                rc = VERR_NO_MEMORY;
            }
        }

        if (RT_SUCCESS(rc))
        {
            rc = SSMR3GetU32(pSSM, pcbMaxBlock);
        }

        if (RT_SUCCESS(rc))
        {
            *ppaDescriptors = paDescriptors;
        }
        else
        {
            RTMemFree(paDescriptors);
        }
    }

    return rc;
}

int HGSMIHostSaveStateExec (PHGSMIINSTANCE pIns, PSSMHANDLE pSSM)
{
    VBOXHGSMI_SAVE_START(pSSM);

    int rc;

    SSMR3PutU32(pSSM, pIns->hostHeap.u32HeapType);

    HGSMIOFFSET off = pIns->pHGFlags ? HGSMIPointerToOffset(&pIns->area, (const HGSMIBUFFERHEADER *)pIns->pHGFlags) : HGSMIOFFSET_VOID;
    SSMR3PutU32 (pSSM, off);

    off = pIns->hostHeap.u32HeapType == HGSMI_HEAP_TYPE_MA?
              0:
              hgsmiHostHeapHandleLocationOffset(&pIns->hostHeap);
    rc = SSMR3PutU32 (pSSM, off);
    if (off != HGSMIOFFSET_VOID)
    {
        SSMR3PutU32 (pSSM, hgsmiHostHeapOffset(&pIns->hostHeap));
        SSMR3PutU32 (pSSM, hgsmiHostHeapSize(&pIns->hostHeap));
        /* need save mem pointer to calculate offset on restore */
        SSMR3PutU64 (pSSM, (uint64_t)(uintptr_t)pIns->area.pu8Base);
        rc = hgsmiFIFOLock (pIns);
        if (RT_SUCCESS(rc))
        {
            rc = hgsmiHostSaveFifoLocked (&pIns->hostFIFO, pSSM); AssertRC(rc);
            rc = hgsmiHostSaveFifoLocked (&pIns->hostFIFORead, pSSM); AssertRC(rc);
            rc = hgsmiHostSaveFifoLocked (&pIns->hostFIFOProcessed, pSSM); AssertRC(rc);
#ifdef VBOX_WITH_WDDM
            rc = hgsmiHostSaveGuestCmdCompletedFifoLocked (&pIns->guestCmdCompleted, pSSM); AssertRC(rc);
#endif

            hgsmiFIFOUnlock (pIns);
        }

        if (RT_SUCCESS(rc))
        {
            if (pIns->hostHeap.u32HeapType == HGSMI_HEAP_TYPE_MA)
            {
                rc = hgsmiHostSaveMA(pSSM, &pIns->hostHeap.u.ma);
            }
        }
    }

    VBOXHGSMI_SAVE_STOP(pSSM);

    return rc;
}

int HGSMIHostLoadStateExec (PHGSMIINSTANCE pIns, PSSMHANDLE pSSM, uint32_t u32Version)
{
    if (u32Version < VGA_SAVEDSTATE_VERSION_HGSMI)
        return VINF_SUCCESS;

    VBOXHGSMI_LOAD_START(pSSM);

    int rc;
    HGSMIOFFSET off;
    uint32_t u32HeapType = HGSMI_HEAP_TYPE_NULL;

    if (u32Version >= VGA_SAVEDSTATE_VERSION_HGSMIMA)
    {
        rc = SSMR3GetU32(pSSM, &u32HeapType);
        AssertRCReturn(rc, rc);
    }

    rc = SSMR3GetU32(pSSM, &off);
    AssertLogRelRCReturn(rc, rc);
    pIns->pHGFlags = (off != HGSMIOFFSET_VOID) ? (HGSMIHOSTFLAGS*)HGSMIOffsetToPointer (&pIns->area, off) : NULL;

    rc = SSMR3GetU32(pSSM, &off);
    AssertLogRelRCReturn(rc, rc);
    if (off != HGSMIOFFSET_VOID)
    {
        /* There is a saved heap. */
        if (u32HeapType == HGSMI_HEAP_TYPE_NULL)
        {
            u32HeapType = u32Version > VGA_SAVEDSTATE_VERSION_HOST_HEAP?
                              HGSMI_HEAP_TYPE_OFFSET:
                              HGSMI_HEAP_TYPE_POINTER;
        }

        HGSMIOFFSET offHeap;
        SSMR3GetU32(pSSM, &offHeap);
        uint32_t cbHeap;
        SSMR3GetU32(pSSM, &cbHeap);
        uint64_t oldMem;
        rc = SSMR3GetU64(pSSM, &oldMem);
        AssertLogRelRCReturn(rc, rc);

        if (RT_SUCCESS(rc))
        {
            rc = hgsmiFIFOLock (pIns);
            if (RT_SUCCESS(rc))
            {
                rc = hgsmiHostLoadFifoLocked (pIns, &pIns->hostFIFO, pSSM);
                if (RT_SUCCESS(rc))
                    rc = hgsmiHostLoadFifoLocked (pIns, &pIns->hostFIFORead, pSSM);
                if (RT_SUCCESS(rc))
                    rc = hgsmiHostLoadFifoLocked (pIns, &pIns->hostFIFOProcessed, pSSM);
#ifdef VBOX_WITH_WDDM
                if (RT_SUCCESS(rc) && u32Version > VGA_SAVEDSTATE_VERSION_PRE_WDDM)
                    rc = hgsmiHostLoadGuestCmdCompletedFifoLocked (pIns, &pIns->guestCmdCompleted, pSSM, u32Version);
#endif

                hgsmiFIFOUnlock (pIns);
            }
        }

        if (RT_SUCCESS(rc))
        {
            if (u32HeapType == HGSMI_HEAP_TYPE_MA)
            {
                uint32_t cBlocks = 0;
                HGSMISIZE cbMaxBlock = 0;
                HGSMIOFFSET *paDescriptors = NULL;
                rc = hgsmiHostLoadMA(pSSM, &cBlocks, &paDescriptors, &cbMaxBlock);
                if (RT_SUCCESS(rc))
                {
                    rc = hgsmiHostHeapRestoreMA(&pIns->hostHeap,
                                                pIns->area.pu8Base+offHeap,
                                                cbHeap,
                                                offHeap,
                                                cBlocks,
                                                paDescriptors,
                                                cbMaxBlock,
                                                &g_hgsmiEnv);

                    RTMemFree(paDescriptors);
                }
            }
            else if (   u32HeapType == HGSMI_HEAP_TYPE_OFFSET
                     || u32HeapType == HGSMI_HEAP_TYPE_POINTER)
            {
                rc = hgsmiHostHeapLock (pIns);
                if (RT_SUCCESS (rc))
                {
                    Assert(!pIns->hostHeap.cRefs);
                    pIns->hostHeap.cRefs = 0;

                    rc = hgsmiHostHeapRelocate(&pIns->hostHeap,
                                               u32HeapType,
                                               pIns->area.pu8Base+offHeap,
                                               off,
                                               uintptr_t(pIns->area.pu8Base) - uintptr_t(oldMem),
                                               cbHeap,
                                               offHeap);

                    hgsmiHostHeapUnlock (pIns);
                }
            }
        }
    }

    VBOXHGSMI_LOAD_STOP(pSSM);

    return rc;
}

/*
 * Channels management.
 */

static int hgsmiChannelMapCreate(PHGSMIINSTANCE pIns, const char *pszChannel, uint8_t *pu8Channel)
{
    RT_NOREF(pIns, pszChannel, pu8Channel);
    /** @todo later */
    return VERR_NOT_SUPPORTED;
}

/* Register a new HGSMI channel by a predefined index.
 */
int HGSMIHostChannelRegister(PHGSMIINSTANCE pIns,
                             uint8_t u8Channel,
                             PFNHGSMICHANNELHANDLER pfnChannelHandler,
                             void *pvChannelHandler)
{
    LogFlowFunc(("pIns %p, u8Channel %x, pfnChannelHandler %p, pvChannelHandler %p\n",
                  pIns, u8Channel, pfnChannelHandler, pvChannelHandler));

    AssertReturn(!HGSMI_IS_DYNAMIC_CHANNEL(u8Channel), VERR_INVALID_PARAMETER);
    AssertPtrReturn(pIns, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pfnChannelHandler, VERR_INVALID_PARAMETER);

    int rc = hgsmiLock (pIns);

    if (RT_SUCCESS (rc))
    {
        rc = HGSMIChannelRegister (&pIns->channelInfo, u8Channel, NULL, pfnChannelHandler, pvChannelHandler);

        hgsmiUnlock (pIns);
    }

    LogFlowFunc(("leave rc = %Rrc\n", rc));

    return rc;
}

/* Register a new HGSMI channel by name.
 */
int HGSMIChannelRegisterName(PHGSMIINSTANCE pIns,
                             const char *pszChannel,
                             PFNHGSMICHANNELHANDLER pfnChannelHandler,
                             void *pvChannelHandler,
                             uint8_t *pu8Channel)
{
    LogFlowFunc(("pIns %p, pszChannel %s, pfnChannelHandler %p, pvChannelHandler %p, pu8Channel %p\n",
                  pIns, pszChannel, pfnChannelHandler, pvChannelHandler, pu8Channel));

    AssertPtrReturn(pIns, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszChannel, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pu8Channel, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pfnChannelHandler, VERR_INVALID_PARAMETER);

    int rc;

    /* The pointer to the copy will be saved in the channel description. */
    char *pszName = RTStrDup (pszChannel);

    if (pszName)
    {
        rc = hgsmiLock (pIns);

        if (RT_SUCCESS (rc))
        {
            rc = hgsmiChannelMapCreate (pIns, pszName, pu8Channel);

            if (RT_SUCCESS (rc))
            {
                rc = HGSMIChannelRegister (&pIns->channelInfo, *pu8Channel, pszName, pfnChannelHandler, pvChannelHandler);
            }

            hgsmiUnlock (pIns);
        }

        if (RT_FAILURE (rc))
        {
            RTStrFree (pszName);
        }
    }
    else
    {
        rc = VERR_NO_MEMORY;
    }

    LogFlowFunc(("leave rc = %Rrc\n", rc));

    return rc;
}

void *HGSMIOffsetToPointerHost (PHGSMIINSTANCE pIns,
                                HGSMIOFFSET offBuffer)
{
    const HGSMIAREA *pArea = &pIns->area;

    if (   offBuffer < pArea->offBase
        || offBuffer > pArea->offLast)
    {
        LogFunc(("offset 0x%x is outside the area [0x%x;0x%x]!!!\n", offBuffer, pArea->offBase, pArea->offLast));
        return NULL;
    }

    return HGSMIOffsetToPointer (pArea, offBuffer);
}


HGSMIOFFSET HGSMIPointerToOffsetHost (PHGSMIINSTANCE pIns,
                                      const void *pv)
{
    const HGSMIAREA *pArea = &pIns->area;

    uintptr_t pBegin = (uintptr_t)pArea->pu8Base;
    uintptr_t pEnd = (uintptr_t)pArea->pu8Base + (pArea->cbArea - 1);
    uintptr_t p = (uintptr_t)pv;

    if (   p < pBegin
        || p > pEnd)
    {
        LogFunc(("pointer %p is outside the area [%p;%p]!!!\n", pv, pBegin, pEnd));
        return HGSMIOFFSET_VOID;
    }

    return HGSMIPointerToOffset (pArea, (HGSMIBUFFERHEADER *)pv);
}


void *HGSMIContext (PHGSMIINSTANCE pIns)
{
    uint8_t *p = (uint8_t *)pIns;
    return p + sizeof (HGSMIINSTANCE);
}

/* The guest submitted a buffer. */
static DECLCALLBACK(int) hgsmiChannelHandler (void *pvHandler, uint16_t u16ChannelInfo, void *pvBuffer, HGSMISIZE cbBuffer)
{
    int rc = VINF_SUCCESS;

    LogFlowFunc(("pvHandler %p, u16ChannelInfo %d, pvBuffer %p, cbBuffer %u\n",
            pvHandler, u16ChannelInfo, pvBuffer, cbBuffer));

    PHGSMIINSTANCE pIns = (PHGSMIINSTANCE)pvHandler;

    switch (u16ChannelInfo)
    {
        case HGSMI_CC_HOST_FLAGS_LOCATION:
        {
            if (cbBuffer < sizeof (HGSMIBUFFERLOCATION))
            {
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            HGSMIBUFFERLOCATION *pLoc = (HGSMIBUFFERLOCATION *)pvBuffer;
            if (pLoc->cbLocation != sizeof(HGSMIHOSTFLAGS))
            {
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            pIns->pHGFlags = (HGSMIHOSTFLAGS*)HGSMIOffsetToPointer (&pIns->area, pLoc->offLocation);
        } break;

        default:
            Log(("Unsupported HGSMI guest command %d!!!\n",
                 u16ChannelInfo));
            break;
    }

    return rc;
}

int HGSMICreate (PHGSMIINSTANCE *ppIns,
                 PVM             pVM,
                 const char     *pszName,
                 HGSMIOFFSET     offBase,
                 uint8_t        *pu8MemBase,
                 HGSMISIZE       cbMem,
                 PFNHGSMINOTIFYGUEST pfnNotifyGuest,
                 void           *pvNotifyGuest,
                 size_t         cbContext)
{
    LogFlowFunc(("ppIns = %p, pVM = %p, pszName = [%s], offBase = 0x%08X, pu8MemBase = %p, cbMem = 0x%08X, "
                 "pfnNotifyGuest = %p, pvNotifyGuest = %p, cbContext = %d\n",
                 ppIns,
                 pVM,
                 pszName,
                 offBase,
                 pu8MemBase,
                 cbMem,
                 pfnNotifyGuest,
                 pvNotifyGuest,
                 cbContext
               ));

    AssertPtrReturn(ppIns, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pVM, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pu8MemBase, VERR_INVALID_PARAMETER);

    int rc = VINF_SUCCESS;

    PHGSMIINSTANCE pIns = (PHGSMIINSTANCE)RTMemAllocZ (sizeof (HGSMIINSTANCE) + cbContext);

    if (!pIns)
    {
        rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS (rc))
    {
        rc = HGSMIAreaInitialize (&pIns->area, pu8MemBase, cbMem, offBase);
    }

    if (RT_SUCCESS (rc))
    {
        rc = RTCritSectInit (&pIns->instanceCritSect);
    }

    if (RT_SUCCESS (rc))
    {
        rc = RTCritSectInit (&pIns->hostHeapCritSect);
    }

    if (RT_SUCCESS (rc))
    {
        rc = RTCritSectInit (&pIns->hostFIFOCritSect);
    }

    if (RT_SUCCESS (rc))
    {
        pIns->pVM            = pVM;

        pIns->pszName        = VALID_PTR(pszName)? pszName: "";

        hgsmiHostHeapSetupUninitialized(&pIns->hostHeap);

        pIns->pfnNotifyGuest = pfnNotifyGuest;
        pIns->pvNotifyGuest  = pvNotifyGuest;

        RTListInit(&pIns->hostFIFO);
        RTListInit(&pIns->hostFIFORead);
        RTListInit(&pIns->hostFIFOProcessed);
        RTListInit(&pIns->hostFIFOFree);
        RTListInit(&pIns->guestCmdCompleted);
    }

    rc = HGSMIHostChannelRegister (pIns,
                                   HGSMI_CH_HGSMI,
                                   hgsmiChannelHandler,
                                   pIns);

    if (RT_SUCCESS (rc))
    {
        *ppIns = pIns;
    }
    else
    {
        HGSMIDestroy (pIns);
    }

    LogFlowFunc(("leave rc = %Rrc, pIns = %p\n", rc, pIns));

    return rc;
}

uint32_t HGSMIReset (PHGSMIINSTANCE pIns)
{
    uint32_t flags = 0;
    if (pIns->pHGFlags)
    {
        /* treat the abandoned commands as read.. */
        while (HGSMIHostRead(pIns) != HGSMIOFFSET_VOID)
        {}
        flags = pIns->pHGFlags->u32HostFlags;
        pIns->pHGFlags->u32HostFlags = 0;
    }

    /* .. and complete them */
    while (hgsmiProcessHostCmdCompletion(pIns, 0, true))
    {}

#ifdef VBOX_WITH_WDDM
    while(hgsmiProcessGuestCmdCompletion(pIns) != HGSMIOFFSET_VOID)
    {}
#endif

    hgsmiHostHeapDestroy(&pIns->hostHeap);

    return flags;
}

void HGSMIDestroy (PHGSMIINSTANCE pIns)
{
    LogFlowFunc(("pIns = %p\n", pIns));

    if (pIns)
    {
        hgsmiHostHeapDestroy(&pIns->hostHeap);

        if (RTCritSectIsInitialized (&pIns->hostHeapCritSect))
        {
            RTCritSectDelete (&pIns->hostHeapCritSect);
        }

        if (RTCritSectIsInitialized (&pIns->instanceCritSect))
        {
            RTCritSectDelete (&pIns->instanceCritSect);
        }

        if (RTCritSectIsInitialized (&pIns->hostFIFOCritSect))
        {
            RTCritSectDelete (&pIns->hostFIFOCritSect);
        }

        memset (pIns, 0, sizeof (HGSMIINSTANCE));

        RTMemFree (pIns);
    }

    LogFlowFunc(("leave\n"));
}

#ifdef VBOX_WITH_WDDM

static int hgsmiGuestCommandComplete (HGSMIINSTANCE *pIns, HGSMIOFFSET offMem)
{
    HGSMIGUESTCOMPLENTRY *pEntry = NULL;

    AssertPtrReturn(pIns->pHGFlags, VERR_WRONG_ORDER);
    int rc = hgsmiGuestCompletionFIFOAlloc (pIns, &pEntry);
    AssertRC(rc);
    if (RT_SUCCESS (rc))
    {
        pEntry->offBuffer = offMem;

        rc = hgsmiFIFOLock(pIns);
        AssertRC(rc);
        if (RT_SUCCESS (rc))
        {
            RTListAppend(&pIns->guestCmdCompleted, &pEntry->nodeEntry);
            ASMAtomicOrU32(&pIns->pHGFlags->u32HostFlags, HGSMIHOSTFLAGS_GCOMMAND_COMPLETED);

            hgsmiFIFOUnlock(pIns);
        }
        else
        {
            hgsmiGuestCompletionFIFOFree(pIns, pEntry);
        }
    }

    return rc;
}

int hgsmiCompleteGuestCommand(PHGSMIINSTANCE pIns,
        HGSMIOFFSET offBuffer,
        bool bDoIrq)
{
    int rc = hgsmiGuestCommandComplete (pIns, offBuffer);
    if (RT_SUCCESS (rc))
    {
        if (bDoIrq)
        {
            /* Now guest can read the FIFO, the notification is informational. */
            hgsmiNotifyGuest (pIns);
        }
#ifdef DEBUG_misha
        else
        {
            Assert(0);
        }
#endif
    }
    return rc;
}

int HGSMICompleteGuestCommand(PHGSMIINSTANCE pIns,
        void *pvMem,
        bool bDoIrq)
{
    LogFlowFunc(("pIns = %p, pvMem = %p\n", pIns, pvMem));

    int rc = VINF_SUCCESS;

    HGSMIBUFFERHEADER *pHeader = HGSMIBufferHeaderFromData(pvMem);
    HGSMIOFFSET offBuffer = HGSMIPointerToOffset(&pIns->area, pHeader);

    Assert(offBuffer != HGSMIOFFSET_VOID);
    if (offBuffer != HGSMIOFFSET_VOID)
    {
        rc = hgsmiCompleteGuestCommand (pIns, offBuffer, bDoIrq);
        AssertRC (rc);
    }
    else
    {
        LogRel(("invalid cmd offset \n"));
        rc = VERR_INVALID_PARAMETER;
    }

    LogFlowFunc(("rc = %Rrc\n", rc));

    return rc;
}
#endif
