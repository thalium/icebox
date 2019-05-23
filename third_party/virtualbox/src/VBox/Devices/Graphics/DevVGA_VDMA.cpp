/* $Id: DevVGA_VDMA.cpp $ */
/** @file
 * Video DMA (VDMA) support.
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
#define LOG_GROUP LOG_GROUP_DEV_VGA
#include <VBox/VMMDev.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pgm.h>
#include <VBoxVideo.h>
#include <VBox/AssertGuest.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>
#include <iprt/mem.h>
#include <iprt/asm.h>
#include <iprt/list.h>
#include <iprt/param.h>

#include "DevVGA.h"
#include "HGSMI/SHGSMIHost.h"

#include <VBoxVideo3D.h>
#include <VBoxVideoHost3D.h>

#ifdef DEBUG_misha
# define VBOXVDBG_MEMCACHE_DISABLE
#endif

#ifndef VBOXVDBG_MEMCACHE_DISABLE
# include <iprt/memcache.h>
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#ifdef DEBUG_misha
# define WARN_BP() do { AssertFailed(); } while (0)
#else
# define WARN_BP() do { } while (0)
#endif
#define WARN(_msg) do { \
        LogRel(_msg); \
        WARN_BP(); \
    } while (0)

#define VBOXVDMATHREAD_STATE_TERMINATED             0
#define VBOXVDMATHREAD_STATE_CREATING               1
#define VBOXVDMATHREAD_STATE_CREATED                3
#define VBOXVDMATHREAD_STATE_TERMINATING            4


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
struct VBOXVDMATHREAD;

typedef DECLCALLBACKPTR(void, PFNVBOXVDMATHREAD_CHANGED)(struct VBOXVDMATHREAD *pThread, int rc, void *pvThreadContext, void *pvChangeContext);

#ifdef VBOX_WITH_CRHGSMI
static DECLCALLBACK(int) vboxCmdVBVACmdCallout(struct VBOXVDMAHOST *pVdma, struct VBOXCRCMDCTL* pCmd, VBOXCRCMDCTL_CALLOUT_LISTENTRY *pEntry, PFNVBOXCRCMDCTL_CALLOUT_CB pfnCb);
#endif


typedef struct VBOXVDMATHREAD
{
    RTTHREAD hWorkerThread;
    RTSEMEVENT hEvent;
    volatile uint32_t u32State;
    PFNVBOXVDMATHREAD_CHANGED pfnChanged;
    void *pvChanged;
} VBOXVDMATHREAD, *PVBOXVDMATHREAD;


/* state transformations:
 *
 *   submitter   |    processor
 *
 *  LISTENING   --->  PROCESSING
 *
 *  */
#define VBVAEXHOSTCONTEXT_STATE_LISTENING      0
#define VBVAEXHOSTCONTEXT_STATE_PROCESSING     1

#define VBVAEXHOSTCONTEXT_ESTATE_DISABLED     -1
#define VBVAEXHOSTCONTEXT_ESTATE_PAUSED        0
#define VBVAEXHOSTCONTEXT_ESTATE_ENABLED       1

typedef struct VBVAEXHOSTCONTEXT
{
    VBVABUFFER RT_UNTRUSTED_VOLATILE_GUEST *pVBVA;
    /** Maximum number of data bytes addressible relative to pVBVA. */
    uint32_t                                cbMaxData;
    volatile int32_t i32State;
    volatile int32_t i32EnableState;
    volatile uint32_t u32cCtls;
    /* critical section for accessing ctl lists */
    RTCRITSECT CltCritSect;
    RTLISTANCHOR GuestCtlList;
    RTLISTANCHOR HostCtlList;
#ifndef VBOXVDBG_MEMCACHE_DISABLE
    RTMEMCACHE CtlCache;
#endif
} VBVAEXHOSTCONTEXT;

typedef enum
{
    VBVAEXHOSTCTL_TYPE_UNDEFINED = 0,
    VBVAEXHOSTCTL_TYPE_HH_INTERNAL_PAUSE,
    VBVAEXHOSTCTL_TYPE_HH_INTERNAL_RESUME,
    VBVAEXHOSTCTL_TYPE_HH_SAVESTATE,
    VBVAEXHOSTCTL_TYPE_HH_LOADSTATE,
    VBVAEXHOSTCTL_TYPE_HH_LOADSTATE_DONE,
    VBVAEXHOSTCTL_TYPE_HH_BE_OPAQUE,
    VBVAEXHOSTCTL_TYPE_HH_ON_HGCM_UNLOAD,
    VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE,
    VBVAEXHOSTCTL_TYPE_GHH_ENABLE,
    VBVAEXHOSTCTL_TYPE_GHH_ENABLE_PAUSED,
    VBVAEXHOSTCTL_TYPE_GHH_DISABLE,
    VBVAEXHOSTCTL_TYPE_GHH_RESIZE
} VBVAEXHOSTCTL_TYPE;

struct VBVAEXHOSTCTL;

typedef DECLCALLBACK(void) FNVBVAEXHOSTCTL_COMPLETE(VBVAEXHOSTCONTEXT *pVbva, struct VBVAEXHOSTCTL *pCtl, int rc, void *pvComplete);
typedef FNVBVAEXHOSTCTL_COMPLETE *PFNVBVAEXHOSTCTL_COMPLETE;

typedef struct VBVAEXHOSTCTL
{
    RTLISTNODE Node;
    VBVAEXHOSTCTL_TYPE enmType;
    union
    {
        struct
        {
            void RT_UNTRUSTED_VOLATILE_GUEST *pvCmd;
            uint32_t cbCmd;
        } cmd;

        struct
        {
            PSSMHANDLE pSSM;
            uint32_t u32Version;
        } state;
    } u;
    PFNVBVAEXHOSTCTL_COMPLETE pfnComplete;
    void *pvComplete;
} VBVAEXHOSTCTL;

/* VBoxVBVAExHP**, i.e. processor functions, can NOT be called concurrently with each other,
 * but can be called with other VBoxVBVAExS** (submitter) functions except Init/Start/Term aparently.
 * Can only be called be the processor, i.e. the entity that acquired the processor state by direct or indirect call to the VBoxVBVAExHSCheckCommands
 * see mor edetailed comments in headers for function definitions */
typedef enum
{
    VBVAEXHOST_DATA_TYPE_NO_DATA = 0,
    VBVAEXHOST_DATA_TYPE_CMD,
    VBVAEXHOST_DATA_TYPE_HOSTCTL,
    VBVAEXHOST_DATA_TYPE_GUESTCTL
} VBVAEXHOST_DATA_TYPE;


#ifdef VBOX_WITH_CRHGSMI
typedef struct VBOXVDMA_SOURCE
{
    VBVAINFOSCREEN Screen;
    VBOXCMDVBVA_SCREENMAP_DECL(uint32_t, aTargetMap);
} VBOXVDMA_SOURCE;
#endif

typedef struct VBOXVDMAHOST
{
    PHGSMIINSTANCE pHgsmi; /**< Same as VGASTATE::pHgsmi. */
    PVGASTATE pVGAState;
#ifdef VBOX_WITH_CRHGSMI
    VBVAEXHOSTCONTEXT CmdVbva;
    VBOXVDMATHREAD Thread;
    VBOXCRCMD_SVRINFO CrSrvInfo;
    VBVAEXHOSTCTL* pCurRemainingHostCtl;
    RTSEMEVENTMULTI HostCrCtlCompleteEvent;
    int32_t volatile i32cHostCrCtlCompleted;
    RTCRITSECT CalloutCritSect;
//    VBOXVDMA_SOURCE aSources[VBOX_VIDEO_MAX_SCREENS];
#endif
#ifdef VBOX_VDMA_WITH_WATCHDOG
    PTMTIMERR3 WatchDogTimer;
#endif
} VBOXVDMAHOST, *PVBOXVDMAHOST;


/**
 * List selector for VBoxVBVAExHCtlSubmit(), vdmaVBVACtlSubmit().
 */
typedef enum
{
    VBVAEXHOSTCTL_SOURCE_GUEST = 0,
    VBVAEXHOSTCTL_SOURCE_HOST
} VBVAEXHOSTCTL_SOURCE;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
#ifdef VBOX_WITH_CRHGSMI
static int  vdmaVBVANotifyDisable(PVGASTATE pVGAState);
static void VBoxVBVAExHPDataCompleteCmd(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint32_t cbCmd);
static void VBoxVBVAExHPDataCompleteCtl(struct VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL *pCtl, int rc);
static int  VBoxVDMAThreadEventNotify(PVBOXVDMATHREAD pThread);
static int  vboxVDMACmdExecBpbTransfer(PVBOXVDMAHOST pVdma, VBOXVDMACMD_DMA_BPB_TRANSFER RT_UNTRUSTED_VOLATILE_GUEST *pTransfer,
                                       uint32_t cbBuffer);
static int  vdmaVBVACtlSubmitSync(PVBOXVDMAHOST pVdma, VBVAEXHOSTCTL *pCtl, VBVAEXHOSTCTL_SOURCE enmSource);
static DECLCALLBACK(void) vdmaVBVACtlSubmitSyncCompletion(VBVAEXHOSTCONTEXT *pVbva, struct VBVAEXHOSTCTL *pCtl,
                                                          int rc, void *pvContext);

/* VBoxVBVAExHP**, i.e. processor functions, can NOT be called concurrently with each other,
 * can be called concurrently with istelf as well as with other VBoxVBVAEx** functions except Init/Start/Term aparently */
#endif /* VBOX_WITH_CRHGSMI */



#ifdef VBOX_WITH_CRHGSMI

/**
 * Creates a host control command.
 */
static VBVAEXHOSTCTL *VBoxVBVAExHCtlCreate(VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL_TYPE enmType)
{
# ifndef VBOXVDBG_MEMCACHE_DISABLE
    VBVAEXHOSTCTL *pCtl = (VBVAEXHOSTCTL *)RTMemCacheAlloc(pCmdVbva->CtlCache);
# else
    VBVAEXHOSTCTL *pCtl = (VBVAEXHOSTCTL *)RTMemAlloc(sizeof(VBVAEXHOSTCTL));
# endif
    if (pCtl)
    {
        RT_ZERO(*pCtl);
        pCtl->enmType = enmType;
    }
    else
        WARN(("VBoxVBVAExHCtlAlloc failed\n"));
    return pCtl;
}

/**
 * Destroys a host control command.
 */
static void VBoxVBVAExHCtlFree(VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL *pCtl)
{
# ifndef VBOXVDBG_MEMCACHE_DISABLE
    RTMemCacheFree(pCmdVbva->CtlCache, pCtl);
# else
    RTMemFree(pCtl);
# endif
}



/**
 * Works the VBVA state.
 */
static int vboxVBVAExHSProcessorAcquire(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    Assert(pCmdVbva->i32State >= VBVAEXHOSTCONTEXT_STATE_LISTENING);

    if (ASMAtomicCmpXchgS32(&pCmdVbva->i32State, VBVAEXHOSTCONTEXT_STATE_PROCESSING, VBVAEXHOSTCONTEXT_STATE_LISTENING))
        return VINF_SUCCESS;
    return VERR_SEM_BUSY;
}

/**
 * Worker for vboxVBVAExHPDataGetInner() and VBoxVBVAExHPCheckHostCtlOnDisable()
 * that gets the next control command.
 *
 * @returns Pointer to command if found, NULL if not.
 * @param   pCmdVbva        The VBVA command context.
 * @param   pfHostCtl       Where to indicate whether it's a host or guest
 *                          control command.
 * @param   fHostOnlyMode   Whether to only fetch host commands, or both.
 */
static VBVAEXHOSTCTL *vboxVBVAExHPCheckCtl(struct VBVAEXHOSTCONTEXT *pCmdVbva, bool *pfHostCtl, bool fHostOnlyMode)
{
    Assert(pCmdVbva->i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);

    if (!fHostOnlyMode && !ASMAtomicUoReadU32(&pCmdVbva->u32cCtls))
        return NULL;

    int rc = RTCritSectEnter(&pCmdVbva->CltCritSect);
    if (RT_SUCCESS(rc))
    {
        VBVAEXHOSTCTL *pCtl = RTListGetFirst(&pCmdVbva->HostCtlList, VBVAEXHOSTCTL, Node);
        if (pCtl)
            *pfHostCtl = true;
        else if (!fHostOnlyMode)
        {
            if (ASMAtomicUoReadS32(&pCmdVbva->i32EnableState) != VBVAEXHOSTCONTEXT_ESTATE_PAUSED)
            {
                pCtl = RTListGetFirst(&pCmdVbva->GuestCtlList, VBVAEXHOSTCTL, Node);
                /* pCtl can not be null here since pCmdVbva->u32cCtls is not null,
                 * and there are no HostCtl commands*/
                Assert(pCtl);
                *pfHostCtl = false;
            }
        }

        if (pCtl)
        {
            RTListNodeRemove(&pCtl->Node);
            ASMAtomicDecU32(&pCmdVbva->u32cCtls);
        }

        RTCritSectLeave(&pCmdVbva->CltCritSect);

        return pCtl;
    }
    else
        WARN(("RTCritSectEnter failed %Rrc\n", rc));

    return NULL;
}

/**
 * Worker for vboxVDMACrHgcmHandleEnableRemainingHostCommand().
 */
static VBVAEXHOSTCTL *VBoxVBVAExHPCheckHostCtlOnDisable(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    bool fHostCtl = false;
    VBVAEXHOSTCTL *pCtl = vboxVBVAExHPCheckCtl(pCmdVbva, &fHostCtl, true);
    Assert(!pCtl || fHostCtl);
    return pCtl;
}

/**
 * Worker for vboxVBVAExHPCheckProcessCtlInternal() and
 * vboxVDMACrGuestCtlProcess() / VBVAEXHOSTCTL_TYPE_GHH_ENABLE_PAUSED.
 */
static int VBoxVBVAExHPPause(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    if (pCmdVbva->i32EnableState < VBVAEXHOSTCONTEXT_ESTATE_PAUSED)
    {
        WARN(("Invalid state\n"));
        return VERR_INVALID_STATE;
    }

    ASMAtomicWriteS32(&pCmdVbva->i32EnableState, VBVAEXHOSTCONTEXT_ESTATE_PAUSED);
    return VINF_SUCCESS;
}

/**
 * Works the VBVA state in response to VBVAEXHOSTCTL_TYPE_HH_INTERNAL_RESUME.
 */
static int VBoxVBVAExHPResume(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    if (pCmdVbva->i32EnableState != VBVAEXHOSTCONTEXT_ESTATE_PAUSED)
    {
        WARN(("Invalid state\n"));
        return VERR_INVALID_STATE;
    }

    ASMAtomicWriteS32(&pCmdVbva->i32EnableState, VBVAEXHOSTCONTEXT_ESTATE_ENABLED);
    return VINF_SUCCESS;
}

/**
 * Worker for vboxVBVAExHPDataGetInner that processes PAUSE and RESUME requests.
 *
 * Unclear why these cannot be handled the normal way.
 *
 * @returns true if handled, false if not.
 * @param   pCmdVbva            The VBVA context.
 * @param   pCtl                The host control command.
 */
static bool vboxVBVAExHPCheckProcessCtlInternal(struct VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL* pCtl)
{
    switch (pCtl->enmType)
    {
        case VBVAEXHOSTCTL_TYPE_HH_INTERNAL_PAUSE:
            VBoxVBVAExHPPause(pCmdVbva);
            VBoxVBVAExHPDataCompleteCtl(pCmdVbva, pCtl, VINF_SUCCESS);
            return true;

        case VBVAEXHOSTCTL_TYPE_HH_INTERNAL_RESUME:
            VBoxVBVAExHPResume(pCmdVbva);
            VBoxVBVAExHPDataCompleteCtl(pCmdVbva, pCtl, VINF_SUCCESS);
            return true;

        default:
            return false;
    }
}

/**
 * Works the VBVA state.
 */
static void vboxVBVAExHPProcessorRelease(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    Assert(pCmdVbva->i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);

    ASMAtomicWriteS32(&pCmdVbva->i32State, VBVAEXHOSTCONTEXT_STATE_LISTENING);
}

/**
 * Works the VBVA state.
 */
static void vboxVBVAExHPHgEventSet(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    Assert(pCmdVbva->i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);
    if (pCmdVbva->pVBVA)
        ASMAtomicOrU32(&pCmdVbva->pVBVA->hostFlags.u32HostEvents, VBVA_F_STATE_PROCESSING);
}

/**
 * Works the VBVA state.
 */
static void vboxVBVAExHPHgEventClear(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    Assert(pCmdVbva->i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);
    if (pCmdVbva->pVBVA)
        ASMAtomicAndU32(&pCmdVbva->pVBVA->hostFlags.u32HostEvents, ~VBVA_F_STATE_PROCESSING);
}

/**
 * Worker for vboxVBVAExHPDataGetInner.
 *
 * @retval VINF_SUCCESS
 * @retval VINF_EOF
 * @retval VINF_TRY_AGAIN
 * @retval VERR_INVALID_STATE
 *
 * @thread VDMA
 */
static int vboxVBVAExHPCmdGet(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t RT_UNTRUSTED_VOLATILE_GUEST **ppbCmd, uint32_t *pcbCmd)
{
    Assert(pCmdVbva->i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);
    Assert(pCmdVbva->i32EnableState > VBVAEXHOSTCONTEXT_ESTATE_PAUSED);

    VBVABUFFER RT_UNTRUSTED_VOLATILE_GUEST *pVBVA = pCmdVbva->pVBVA; /* This is shared with the guest, so careful! */

    /*
     * Inspect records.
     */
    uint32_t idxRecordFirst = ASMAtomicUoReadU32(&pVBVA->indexRecordFirst);
    uint32_t idxRecordFree  = ASMAtomicReadU32(&pVBVA->indexRecordFree);
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
    Log(("first = %d, free = %d\n", idxRecordFirst, idxRecordFree));
    if (idxRecordFirst == idxRecordFree)
        return VINF_EOF; /* No records to process. Return without assigning output variables. */
    AssertReturn(idxRecordFirst < VBVA_MAX_RECORDS, VERR_INVALID_STATE);
    RT_UNTRUSTED_VALIDATED_FENCE();

    /*
     * Read the record size and check that it has been completly recorded.
     */
    uint32_t const cbRecordCurrent = ASMAtomicReadU32(&pVBVA->aRecords[idxRecordFirst].cbRecord);
    uint32_t const cbRecord        = cbRecordCurrent & ~VBVA_F_RECORD_PARTIAL;
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
    if (   (cbRecordCurrent & VBVA_F_RECORD_PARTIAL)
        || !cbRecord)
        return VINF_TRY_AGAIN; /* The record is being recorded, try again. */
    Assert(cbRecord);

    /*
     * Get and validate the data area.
     */
    uint32_t const offData   = ASMAtomicReadU32(&pVBVA->off32Data);
    uint32_t       cbMaxData = ASMAtomicReadU32(&pVBVA->cbData);
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
    AssertLogRelMsgStmt(cbMaxData <= pCmdVbva->cbMaxData, ("%#x vs %#x\n", cbMaxData, pCmdVbva->cbMaxData),
                        cbMaxData = pCmdVbva->cbMaxData);
    AssertLogRelMsgReturn(   cbRecord <= cbMaxData
                          && offData  <= cbMaxData - cbRecord,
                          ("offData=%#x cbRecord=%#x cbMaxData=%#x cbRecord\n", offData, cbRecord, cbMaxData),
                          VERR_INVALID_STATE);
    RT_UNTRUSTED_VALIDATED_FENCE();

    /*
     * Just set the return values and we're done.
     */
    *ppbCmd = (uint8_t RT_UNTRUSTED_VOLATILE_GUEST *)&pVBVA->au8Data[offData];
    *pcbCmd = cbRecord;
    return VINF_SUCCESS;
}

/**
 * Completion routine advancing our end of the ring and data buffers forward.
 *
 * @param   pCmdVbva            The VBVA context.
 * @param   cbCmd               The size of the data.
 */
static void VBoxVBVAExHPDataCompleteCmd(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint32_t cbCmd)
{
    VBVABUFFER RT_UNTRUSTED_VOLATILE_GUEST *pVBVA = pCmdVbva->pVBVA;
    if (pVBVA)
    {
        /* Move data head. */
        uint32_t const  cbData      = pVBVA->cbData;
        uint32_t const  offData     = pVBVA->off32Data;
        RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
        if (cbData > 0)
            ASMAtomicWriteU32(&pVBVA->off32Data, (offData + cbCmd) % cbData);
        else
            ASMAtomicWriteU32(&pVBVA->off32Data, 0);

        /* Increment record pointer. */
        uint32_t const  idxRecFirst = pVBVA->indexRecordFirst;
        RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
        ASMAtomicWriteU32(&pVBVA->indexRecordFirst, (idxRecFirst + 1) % RT_ELEMENTS(pVBVA->aRecords));
    }
}

/**
 * Control command completion routine used by many.
 */
static void VBoxVBVAExHPDataCompleteCtl(struct VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL *pCtl, int rc)
{
    if (pCtl->pfnComplete)
        pCtl->pfnComplete(pCmdVbva, pCtl, rc, pCtl->pvComplete);
    else
        VBoxVBVAExHCtlFree(pCmdVbva, pCtl);
}


/**
 * Worker for VBoxVBVAExHPDataGet.
 * @thread VDMA
 */
static VBVAEXHOST_DATA_TYPE vboxVBVAExHPDataGetInner(struct VBVAEXHOSTCONTEXT *pCmdVbva,
                                                     uint8_t RT_UNTRUSTED_VOLATILE_GUEST **ppbCmd, uint32_t *pcbCmd)
{
    Assert(pCmdVbva->i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);
    VBVAEXHOSTCTL *pCtl;
    bool fHostClt;

    for (;;)
    {
        pCtl = vboxVBVAExHPCheckCtl(pCmdVbva, &fHostClt, false);
        if (pCtl)
        {
            if (fHostClt)
            {
                if (!vboxVBVAExHPCheckProcessCtlInternal(pCmdVbva, pCtl))
                {
                    *ppbCmd = (uint8_t RT_UNTRUSTED_VOLATILE_GUEST *)pCtl; /* Note! pCtl is host data, so trusted */
                    *pcbCmd = sizeof (*pCtl);
                    return VBVAEXHOST_DATA_TYPE_HOSTCTL;
                }
                continue; /* Processed by vboxVBVAExHPCheckProcessCtlInternal, get next. */
            }
            *ppbCmd = (uint8_t RT_UNTRUSTED_VOLATILE_GUEST *)pCtl; /* Note! pCtl is host data, so trusted */
            *pcbCmd = sizeof (*pCtl);
            return VBVAEXHOST_DATA_TYPE_GUESTCTL;
        }

        if (ASMAtomicUoReadS32(&pCmdVbva->i32EnableState) <= VBVAEXHOSTCONTEXT_ESTATE_PAUSED)
            return VBVAEXHOST_DATA_TYPE_NO_DATA;

        int rc = vboxVBVAExHPCmdGet(pCmdVbva, ppbCmd, pcbCmd);
        switch (rc)
        {
            case VINF_SUCCESS:
                return VBVAEXHOST_DATA_TYPE_CMD;
            case VINF_EOF:
                return VBVAEXHOST_DATA_TYPE_NO_DATA;
            case VINF_TRY_AGAIN:
                RTThreadSleep(1);
                continue;
            default:
                /* this is something really unexpected, i.e. most likely guest has written something incorrect to the VBVA buffer */
                WARN(("Warning: vboxVBVAExHCmdGet returned unexpected status %Rrc\n", rc));
                return VBVAEXHOST_DATA_TYPE_NO_DATA;
        }
    }
    /* not reached */
}

/**
 * Called by vboxVDMAWorkerThread to get the next command to process.
 * @thread VDMA
 */
static VBVAEXHOST_DATA_TYPE VBoxVBVAExHPDataGet(struct VBVAEXHOSTCONTEXT *pCmdVbva,
                                                uint8_t RT_UNTRUSTED_VOLATILE_GUEST **ppbCmd, uint32_t *pcbCmd)
{
    VBVAEXHOST_DATA_TYPE enmType = vboxVBVAExHPDataGetInner(pCmdVbva, ppbCmd, pcbCmd);
    if (enmType == VBVAEXHOST_DATA_TYPE_NO_DATA)
    {
        vboxVBVAExHPHgEventClear(pCmdVbva);
        vboxVBVAExHPProcessorRelease(pCmdVbva);

        /*
         * We need to prevent racing between us clearing the flag and command check/submission thread, i.e.
         * 1. we check the queue -> and it is empty
         * 2. submitter adds command to the queue
         * 3. submitter checks the "processing" -> and it is true , thus it does not submit a notification
         * 4. we clear the "processing" state
         * 5. ->here we need to re-check the queue state to ensure we do not leak the notification of the above command
         * 6. if the queue appears to be not-empty set the "processing" state back to "true"
         */
        int rc = vboxVBVAExHSProcessorAcquire(pCmdVbva);
        if (RT_SUCCESS(rc))
        {
            /* we are the processor now */
            enmType = vboxVBVAExHPDataGetInner(pCmdVbva, ppbCmd, pcbCmd);
            if (enmType == VBVAEXHOST_DATA_TYPE_NO_DATA)
            {
                vboxVBVAExHPProcessorRelease(pCmdVbva);
                return VBVAEXHOST_DATA_TYPE_NO_DATA;
            }

            vboxVBVAExHPHgEventSet(pCmdVbva);
        }
    }

    return enmType;
}

/**
 * Checks for pending VBVA command or (internal) control command.
 */
DECLINLINE(bool) vboxVBVAExHSHasCommands(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    VBVABUFFER RT_UNTRUSTED_VOLATILE_GUEST *pVBVA = pCmdVbva->pVBVA;
    if (pVBVA)
    {
        uint32_t indexRecordFirst = pVBVA->indexRecordFirst;
        uint32_t indexRecordFree  = pVBVA->indexRecordFree;
        RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

        if (indexRecordFirst != indexRecordFree)
            return true;
    }

    return ASMAtomicReadU32(&pCmdVbva->u32cCtls) > 0;
}

/** Checks whether the new commands are ready for processing
 * @returns
 *   VINF_SUCCESS - there are commands are in a queue, and the given thread is now the processor (i.e. typically it would delegate processing to a worker thread)
 *   VINF_EOF - no commands in a queue
 *   VINF_ALREADY_INITIALIZED - another thread already processing the commands
 *   VERR_INVALID_STATE - the VBVA is paused or pausing */
static int VBoxVBVAExHSCheckCommands(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    int rc = vboxVBVAExHSProcessorAcquire(pCmdVbva);
    if (RT_SUCCESS(rc))
    {
        /* we are the processor now */
        if (vboxVBVAExHSHasCommands(pCmdVbva))
        {
            vboxVBVAExHPHgEventSet(pCmdVbva);
            return VINF_SUCCESS;
        }

        vboxVBVAExHPProcessorRelease(pCmdVbva);
        return VINF_EOF;
    }
    if (rc == VERR_SEM_BUSY)
        return VINF_ALREADY_INITIALIZED;
    return VERR_INVALID_STATE;
}

/**
 * Worker for vboxVDMAConstruct() that initializes the give VBVA host context.
 */
static int VBoxVBVAExHSInit(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    RT_ZERO(*pCmdVbva);
    int rc = RTCritSectInit(&pCmdVbva->CltCritSect);
    if (RT_SUCCESS(rc))
    {
# ifndef VBOXVDBG_MEMCACHE_DISABLE
        rc = RTMemCacheCreate(&pCmdVbva->CtlCache, sizeof (VBVAEXHOSTCTL),
                              0, /* size_t cbAlignment */
                              UINT32_MAX, /* uint32_t cMaxObjects */
                              NULL, /* PFNMEMCACHECTOR pfnCtor*/
                              NULL, /* PFNMEMCACHEDTOR pfnDtor*/
                              NULL, /* void *pvUser*/
                              0 /* uint32_t fFlags*/
                              );
        if (RT_SUCCESS(rc))
# endif
        {
            RTListInit(&pCmdVbva->GuestCtlList);
            RTListInit(&pCmdVbva->HostCtlList);
            pCmdVbva->i32State       = VBVAEXHOSTCONTEXT_STATE_PROCESSING;
            pCmdVbva->i32EnableState = VBVAEXHOSTCONTEXT_ESTATE_DISABLED;
            return VINF_SUCCESS;
        }
# ifndef VBOXVDBG_MEMCACHE_DISABLE
        WARN(("RTMemCacheCreate failed %Rrc\n", rc));
# endif
    }
    else
        WARN(("RTCritSectInit failed %Rrc\n", rc));

    return rc;
}

/**
 * Checks if VBVA state is some form of enabled.
 */
DECLINLINE(bool) VBoxVBVAExHSIsEnabled(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    return ASMAtomicUoReadS32(&pCmdVbva->i32EnableState) >= VBVAEXHOSTCONTEXT_ESTATE_PAUSED;
}

/**
 * Checks if VBVA state is disabled.
 */
DECLINLINE(bool) VBoxVBVAExHSIsDisabled(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    return ASMAtomicUoReadS32(&pCmdVbva->i32EnableState) == VBVAEXHOSTCONTEXT_ESTATE_DISABLED;
}

/**
 * Worker for vdmaVBVAEnableProcess().
 *
 * @thread VDMA
 */
static int VBoxVBVAExHSEnable(struct VBVAEXHOSTCONTEXT *pCmdVbva, VBVABUFFER RT_UNTRUSTED_VOLATILE_GUEST *pVBVA,
                              uint8_t *pbVRam, uint32_t cbVRam)
{
    if (VBoxVBVAExHSIsEnabled(pCmdVbva))
    {
        WARN(("VBVAEx is enabled already\n"));
        return VERR_INVALID_STATE;
    }

    uintptr_t offVRam = (uintptr_t)pVBVA - (uintptr_t)pbVRam;
    AssertLogRelMsgReturn(offVRam < cbVRam - sizeof(*pVBVA), ("%#p cbVRam=%#x\n", offVRam, cbVRam), VERR_OUT_OF_RANGE);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pCmdVbva->pVBVA     = pVBVA;
    pCmdVbva->cbMaxData = cbVRam - offVRam - RT_UOFFSETOF(VBVABUFFER, au8Data);
    pVBVA->hostFlags.u32HostEvents = 0;
    ASMAtomicWriteS32(&pCmdVbva->i32EnableState, VBVAEXHOSTCONTEXT_ESTATE_ENABLED);
    return VINF_SUCCESS;
}

/**
 * Works the enable state.
 * @thread VDMA, CR, EMT, ...
 */
static int VBoxVBVAExHSDisable(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    if (VBoxVBVAExHSIsDisabled(pCmdVbva))
        return VINF_SUCCESS;

    ASMAtomicWriteS32(&pCmdVbva->i32EnableState, VBVAEXHOSTCONTEXT_ESTATE_DISABLED);
    return VINF_SUCCESS;
}

/**
 * Worker for vboxVDMADestruct() and vboxVDMAConstruct().
 */
static void VBoxVBVAExHSTerm(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    /* ensure the processor is stopped */
    Assert(pCmdVbva->i32State >= VBVAEXHOSTCONTEXT_STATE_LISTENING);

    /* ensure no one tries to submit the command */
    if (pCmdVbva->pVBVA)
        pCmdVbva->pVBVA->hostFlags.u32HostEvents = 0;

    Assert(RTListIsEmpty(&pCmdVbva->GuestCtlList));
    Assert(RTListIsEmpty(&pCmdVbva->HostCtlList));

    RTCritSectDelete(&pCmdVbva->CltCritSect);

# ifndef VBOXVDBG_MEMCACHE_DISABLE
    RTMemCacheDestroy(pCmdVbva->CtlCache);
# endif

    RT_ZERO(*pCmdVbva);
}


/**
 * Worker for vboxVBVAExHSSaveStateLocked().
 * @thread VDMA
 */
static int vboxVBVAExHSSaveGuestCtl(struct VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL* pCtl, uint8_t* pu8VramBase, PSSMHANDLE pSSM)
{
    RT_NOREF(pCmdVbva);
    int rc = SSMR3PutU32(pSSM, pCtl->enmType);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, pCtl->u.cmd.cbCmd);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, (uint32_t)((uintptr_t)pCtl->u.cmd.pvCmd - (uintptr_t)pu8VramBase));
    AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}

/**
 * Worker for VBoxVBVAExHSSaveState().
 * @thread VDMA
 */
static int vboxVBVAExHSSaveStateLocked(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t* pu8VramBase, PSSMHANDLE pSSM)
{
    if (ASMAtomicUoReadS32(&pCmdVbva->i32EnableState) != VBVAEXHOSTCONTEXT_ESTATE_PAUSED)
    {
        WARN(("vbva not paused\n"));
        return VERR_INVALID_STATE;
    }

    int rc;
    VBVAEXHOSTCTL* pCtl;
    RTListForEach(&pCmdVbva->GuestCtlList, pCtl, VBVAEXHOSTCTL, Node)
    {
        rc = vboxVBVAExHSSaveGuestCtl(pCmdVbva, pCtl, pu8VramBase, pSSM);
        AssertRCReturn(rc, rc);
    }

    rc = SSMR3PutU32(pSSM, 0);
    AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}

/**
 * Handles VBVAEXHOSTCTL_TYPE_HH_SAVESTATE for vboxVDMACrHostCtlProcess, saving
 * state on the VDMA thread.
 *
 * @returns - same as VBoxVBVAExHSCheckCommands, or failure on load state fail
 * @thread VDMA
 */
static int VBoxVBVAExHSSaveState(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t* pu8VramBase, PSSMHANDLE pSSM)
{
    int rc = RTCritSectEnter(&pCmdVbva->CltCritSect);
    AssertRCReturn(rc, rc);

    rc = vboxVBVAExHSSaveStateLocked(pCmdVbva, pu8VramBase, pSSM);
    if (RT_FAILURE(rc))
        WARN(("vboxVBVAExHSSaveStateLocked failed %Rrc\n", rc));

    RTCritSectLeave(&pCmdVbva->CltCritSect);
    return rc;
}


/**
 * Worker for vboxVBVAExHSLoadStateLocked.
 * @retval VINF_EOF if end stuff to load.
 * @thread VDMA
 */
static int vboxVBVAExHSLoadGuestCtl(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t* pu8VramBase, PSSMHANDLE pSSM, uint32_t u32Version)
{
    RT_NOREF(u32Version);
    uint32_t u32;
    int rc = SSMR3GetU32(pSSM, &u32);
    AssertLogRelRCReturn(rc, rc);

    if (!u32)
        return VINF_EOF;

    VBVAEXHOSTCTL *pHCtl = VBoxVBVAExHCtlCreate(pCmdVbva, (VBVAEXHOSTCTL_TYPE)u32);
    if (!pHCtl)
    {
        WARN(("VBoxVBVAExHCtlCreate failed\n"));
        return VERR_NO_MEMORY;
    }

    rc = SSMR3GetU32(pSSM, &u32);
    AssertLogRelRCReturn(rc, rc);
    pHCtl->u.cmd.cbCmd = u32;

    rc = SSMR3GetU32(pSSM, &u32);
    AssertLogRelRCReturn(rc, rc);
    pHCtl->u.cmd.pvCmd = pu8VramBase + u32;

    RTListAppend(&pCmdVbva->GuestCtlList, &pHCtl->Node);
    ++pCmdVbva->u32cCtls;

    return VINF_SUCCESS;
}

/**
 * Worker for VBoxVBVAExHSLoadState.
 * @thread VDMA
 */
static int vboxVBVAExHSLoadStateLocked(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t* pu8VramBase, PSSMHANDLE pSSM, uint32_t u32Version)
{
    if (ASMAtomicUoReadS32(&pCmdVbva->i32EnableState) != VBVAEXHOSTCONTEXT_ESTATE_PAUSED)
    {
        WARN(("vbva not stopped\n"));
        return VERR_INVALID_STATE;
    }

    int rc;
    do
    {
        rc = vboxVBVAExHSLoadGuestCtl(pCmdVbva, pu8VramBase, pSSM, u32Version);
        AssertLogRelRCReturn(rc, rc);
    } while (rc != VINF_EOF);

    return VINF_SUCCESS;
}

/**
 * Handles VBVAEXHOSTCTL_TYPE_HH_LOADSTATE for vboxVDMACrHostCtlProcess(),
 * loading state on the VDMA thread.
 *
 * @returns - same as VBoxVBVAExHSCheckCommands, or failure on load state fail
 * @thread VDMA
 */
static int VBoxVBVAExHSLoadState(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t* pu8VramBase, PSSMHANDLE pSSM, uint32_t u32Version)
{
    Assert(VGA_SAVEDSTATE_VERSION_3D <= u32Version);
    int rc = RTCritSectEnter(&pCmdVbva->CltCritSect);
    AssertRCReturn(rc, rc);

    rc = vboxVBVAExHSLoadStateLocked(pCmdVbva, pu8VramBase, pSSM, u32Version);
    if (RT_FAILURE(rc))
        WARN(("vboxVBVAExHSSaveStateLocked failed %Rrc\n", rc));

    RTCritSectLeave(&pCmdVbva->CltCritSect);
    return rc;
}



/**
 * Queues a control command to the VDMA worker thread.
 *
 * The @a enmSource argument decides which list (guest/host) it's queued on.
 *
 */
static int VBoxVBVAExHCtlSubmit(VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL *pCtl, VBVAEXHOSTCTL_SOURCE enmSource,
                                PFNVBVAEXHOSTCTL_COMPLETE pfnComplete, void *pvComplete)
{
    int rc;
    if (VBoxVBVAExHSIsEnabled(pCmdVbva))
    {
        pCtl->pfnComplete = pfnComplete;
        pCtl->pvComplete  = pvComplete;

        rc = RTCritSectEnter(&pCmdVbva->CltCritSect);
        if (RT_SUCCESS(rc))
        {
            /* Recheck that we're enabled after we've got the lock. */
            if (VBoxVBVAExHSIsEnabled(pCmdVbva))
            {
                /* Queue it. */
                if (enmSource > VBVAEXHOSTCTL_SOURCE_GUEST)
                    RTListAppend(&pCmdVbva->HostCtlList, &pCtl->Node);
                else
                    RTListAppend(&pCmdVbva->GuestCtlList, &pCtl->Node);
                ASMAtomicIncU32(&pCmdVbva->u32cCtls);

                RTCritSectLeave(&pCmdVbva->CltCritSect);

                /* Work the state or something. */
                rc = VBoxVBVAExHSCheckCommands(pCmdVbva);
            }
            else
            {
                RTCritSectLeave(&pCmdVbva->CltCritSect);
                Log(("cmd vbva not enabled (race)\n"));
                rc = VERR_INVALID_STATE;
            }
        }
        else
            AssertRC(rc);
    }
    else
    {
        Log(("cmd vbva not enabled\n"));
        rc = VERR_INVALID_STATE;
    }
    return rc;
}

/**
 * Submits the control command and notifies the VDMA thread.
 */
static int vdmaVBVACtlSubmit(PVBOXVDMAHOST pVdma, VBVAEXHOSTCTL *pCtl, VBVAEXHOSTCTL_SOURCE enmSource,
                             PFNVBVAEXHOSTCTL_COMPLETE pfnComplete, void *pvComplete)
{
    int rc = VBoxVBVAExHCtlSubmit(&pVdma->CmdVbva, pCtl, enmSource, pfnComplete, pvComplete);
    if (RT_SUCCESS(rc))
    {
        if (rc == VINF_SUCCESS)
            return VBoxVDMAThreadEventNotify(&pVdma->Thread);
        Assert(rc == VINF_ALREADY_INITIALIZED);
    }
    else
        Log(("VBoxVBVAExHCtlSubmit failed %Rrc\n", rc));

    return rc;
}


/**
 * Call VDMA thread creation notification callback.
 */
void VBoxVDMAThreadNotifyConstructSucceeded(PVBOXVDMATHREAD pThread, void *pvThreadContext)
{
    Assert(pThread->u32State == VBOXVDMATHREAD_STATE_CREATING);
    PFNVBOXVDMATHREAD_CHANGED pfnChanged = pThread->pfnChanged;
    void                     *pvChanged  = pThread->pvChanged;

    pThread->pfnChanged = NULL;
    pThread->pvChanged  = NULL;

    ASMAtomicWriteU32(&pThread->u32State, VBOXVDMATHREAD_STATE_CREATED);

    if (pfnChanged)
        pfnChanged(pThread, VINF_SUCCESS, pvThreadContext, pvChanged);
}

/**
 * Call VDMA thread termination notification callback.
 */
void VBoxVDMAThreadNotifyTerminatingSucceeded(PVBOXVDMATHREAD pThread, void *pvThreadContext)
{
    Assert(pThread->u32State == VBOXVDMATHREAD_STATE_TERMINATING);
    PFNVBOXVDMATHREAD_CHANGED pfnChanged = pThread->pfnChanged;
    void                     *pvChanged  = pThread->pvChanged;

    pThread->pfnChanged = NULL;
    pThread->pvChanged  = NULL;

    if (pfnChanged)
        pfnChanged(pThread, VINF_SUCCESS, pvThreadContext, pvChanged);
}

/**
 * Check if VDMA thread is terminating.
 */
DECLINLINE(bool) VBoxVDMAThreadIsTerminating(PVBOXVDMATHREAD pThread)
{
    return ASMAtomicUoReadU32(&pThread->u32State) == VBOXVDMATHREAD_STATE_TERMINATING;
}

/**
 * Init VDMA thread.
 */
void VBoxVDMAThreadInit(PVBOXVDMATHREAD pThread)
{
    RT_ZERO(*pThread);
    pThread->u32State = VBOXVDMATHREAD_STATE_TERMINATED;
}

/**
 * Clean up VDMA thread.
 */
int VBoxVDMAThreadCleanup(PVBOXVDMATHREAD pThread)
{
    uint32_t u32State = ASMAtomicUoReadU32(&pThread->u32State);
    switch (u32State)
    {
        case VBOXVDMATHREAD_STATE_TERMINATED:
            return VINF_SUCCESS;

        case VBOXVDMATHREAD_STATE_TERMINATING:
        {
            int rc = RTThreadWait(pThread->hWorkerThread, RT_INDEFINITE_WAIT, NULL);
            if (RT_SUCCESS(rc))
            {
                RTSemEventDestroy(pThread->hEvent);
                pThread->hEvent        = NIL_RTSEMEVENT;
                pThread->hWorkerThread = NIL_RTTHREAD;
                ASMAtomicWriteU32(&pThread->u32State, VBOXVDMATHREAD_STATE_TERMINATED);
            }
            else
                WARN(("RTThreadWait failed %Rrc\n", rc));
            return rc;
        }

        default:
            WARN(("invalid state"));
            return VERR_INVALID_STATE;
    }
}

/**
 * Start VDMA thread.
 */
int VBoxVDMAThreadCreate(PVBOXVDMATHREAD pThread, PFNRTTHREAD pfnThread, void *pvThread,
                         PFNVBOXVDMATHREAD_CHANGED pfnCreated, void *pvCreated)
{
    int rc = VBoxVDMAThreadCleanup(pThread);
    if (RT_SUCCESS(rc))
    {
        rc = RTSemEventCreate(&pThread->hEvent);
        pThread->u32State   = VBOXVDMATHREAD_STATE_CREATING;
        pThread->pfnChanged = pfnCreated;
        pThread->pvChanged  = pvCreated;
        rc = RTThreadCreate(&pThread->hWorkerThread, pfnThread, pvThread, 0, RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, "VDMA");
        if (RT_SUCCESS(rc))
            return VINF_SUCCESS;

        WARN(("RTThreadCreate failed %Rrc\n", rc));
        RTSemEventDestroy(pThread->hEvent);
        pThread->hEvent        = NIL_RTSEMEVENT;
        pThread->hWorkerThread = NIL_RTTHREAD;
        pThread->u32State      = VBOXVDMATHREAD_STATE_TERMINATED;
    }
    else
        WARN(("VBoxVDMAThreadCleanup failed %Rrc\n", rc));
    return rc;
}

/**
 * Notifies the VDMA thread.
 * @thread !VDMA
 */
static int VBoxVDMAThreadEventNotify(PVBOXVDMATHREAD pThread)
{
    int rc = RTSemEventSignal(pThread->hEvent);
    AssertRC(rc);
    return rc;
}

/**
 * State worker for VBVAEXHOSTCTL_TYPE_HH_ON_HGCM_UNLOAD &
 * VBVAEXHOSTCTL_TYPE_GHH_DISABLE in vboxVDMACrHostCtlProcess(), and
 * VBVAEXHOSTCTL_TYPE_GHH_DISABLE in vboxVDMACrGuestCtlProcess().
 *
 * @thread VDMA
 */
static int VBoxVDMAThreadTerm(PVBOXVDMATHREAD pThread, PFNVBOXVDMATHREAD_CHANGED pfnTerminated, void *pvTerminated, bool fNotify)
{
    for (;;)
    {
        uint32_t u32State = ASMAtomicUoReadU32(&pThread->u32State);
        switch (u32State)
        {
            case VBOXVDMATHREAD_STATE_CREATED:
                pThread->pfnChanged = pfnTerminated;
                pThread->pvChanged  = pvTerminated;
                ASMAtomicWriteU32(&pThread->u32State, VBOXVDMATHREAD_STATE_TERMINATING);
                if (fNotify)
                {
                    int rc = VBoxVDMAThreadEventNotify(pThread);
                    AssertRC(rc);
                }
                return VINF_SUCCESS;

            case VBOXVDMATHREAD_STATE_TERMINATING:
            case VBOXVDMATHREAD_STATE_TERMINATED:
                WARN(("thread is marked to termination or terminated\nn"));
                return VERR_INVALID_STATE;

            case VBOXVDMATHREAD_STATE_CREATING:
                /* wait till the thread creation is completed */
                WARN(("concurrent thread create/destron\n"));
                RTThreadYield();
                continue;

            default:
                WARN(("invalid state"));
                return VERR_INVALID_STATE;
        }
    }
}



/*
 *
 *
 * vboxVDMACrCtlPost / vboxVDMACrCtlPostAsync
 * vboxVDMACrCtlPost / vboxVDMACrCtlPostAsync
 * vboxVDMACrCtlPost / vboxVDMACrCtlPostAsync
 *
 *
 */

/** Completion callback for vboxVDMACrCtlPostAsync(). */
typedef DECLCALLBACK(void) FNVBOXVDMACRCTL_CALLBACK(PVGASTATE pVGAState, PVBOXVDMACMD_CHROMIUM_CTL pCmd, void* pvContext);
/** Pointer to a vboxVDMACrCtlPostAsync completion callback. */
typedef FNVBOXVDMACRCTL_CALLBACK *PFNVBOXVDMACRCTL_CALLBACK;

/**
 * Private wrapper around VBOXVDMACMD_CHROMIUM_CTL.
 */
typedef struct VBOXVDMACMD_CHROMIUM_CTL_PRIVATE
{
    uint32_t                    uMagic; /**< VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_MAGIC */
    uint32_t                    cRefs;
    int32_t volatile            rc;
    PFNVBOXVDMACRCTL_CALLBACK   pfnCompletion;
    void                       *pvCompletion;
    RTSEMEVENT                  hEvtDone;
    VBOXVDMACMD_CHROMIUM_CTL    Cmd;
} VBOXVDMACMD_CHROMIUM_CTL_PRIVATE, *PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE;
/** Magic number for VBOXVDMACMD_CHROMIUM_CTL_PRIVATE (Michael Wolff). */
# define VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_MAGIC         UINT32_C(0x19530827)

/** Converts from a VBOXVDMACMD_CHROMIUM_CTL::Cmd pointer to a pointer to the
 * containing structure. */
# define VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(_p)  RT_FROM_MEMBER(pCmd, VBOXVDMACMD_CHROMIUM_CTL_PRIVATE, Cmd)

/**
 * Creates a VBOXVDMACMD_CHROMIUM_CTL_PRIVATE instance.
 */
static PVBOXVDMACMD_CHROMIUM_CTL vboxVDMACrCtlCreate(VBOXVDMACMD_CHROMIUM_CTL_TYPE enmCmd, uint32_t cbCmd)
{
    PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pHdr;
    pHdr = (PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE)RTMemAllocZ(cbCmd + RT_OFFSETOF(VBOXVDMACMD_CHROMIUM_CTL_PRIVATE, Cmd));
    if (pHdr)
    {
        pHdr->uMagic      = VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_MAGIC;
        pHdr->cRefs       = 1;
        pHdr->rc          = VERR_NOT_IMPLEMENTED;
        pHdr->hEvtDone    = NIL_RTSEMEVENT;
        pHdr->Cmd.enmType = enmCmd;
        pHdr->Cmd.cbCmd   = cbCmd;
        return &pHdr->Cmd;
    }
    return NULL;
}

/**
 * Releases a reference to a VBOXVDMACMD_CHROMIUM_CTL_PRIVATE instance.
 */
DECLINLINE(void) vboxVDMACrCtlRelease(PVBOXVDMACMD_CHROMIUM_CTL pCmd)
{
    PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pHdr = VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(pCmd);
    Assert(pHdr->uMagic == VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_MAGIC);

    uint32_t cRefs = ASMAtomicDecU32(&pHdr->cRefs);
    if (!cRefs)
    {
        pHdr->uMagic = ~VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_MAGIC;
        if (pHdr->hEvtDone != NIL_RTSEMEVENT)
        {
            RTSemEventDestroy(pHdr->hEvtDone);
            pHdr->hEvtDone = NIL_RTSEMEVENT;
        }
        RTMemFree(pHdr);
    }
}

/**
 * Releases a reference to a VBOXVDMACMD_CHROMIUM_CTL_PRIVATE instance.
 */
DECLINLINE(void) vboxVDMACrCtlRetain(PVBOXVDMACMD_CHROMIUM_CTL pCmd)
{
    PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pHdr = VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(pCmd);
    Assert(pHdr->uMagic == VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_MAGIC);

    uint32_t cRefs = ASMAtomicIncU32(&pHdr->cRefs);
    Assert(cRefs > 1);
    Assert(cRefs < _1K);
    RT_NOREF_PV(cRefs);
}

/**
 * Gets the result from our private chromium control command.
 *
 * @returns status code.
 * @param   pCmd                The command.
 */
DECLINLINE(int) vboxVDMACrCtlGetRc(PVBOXVDMACMD_CHROMIUM_CTL pCmd)
{
    PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pHdr = VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(pCmd);
    Assert(pHdr->uMagic == VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_MAGIC);
    return pHdr->rc;
}

/**
 * @interface_method_impl{PDMIDISPLAYVBVACALLBACKS,pfnCrHgsmiControlCompleteAsync,
 *      Some indirect completion magic, you gotta love this code! }
 */
DECLCALLBACK(int) vboxVDMACrHgsmiControlCompleteAsync(PPDMIDISPLAYVBVACALLBACKS pInterface, PVBOXVDMACMD_CHROMIUM_CTL pCmd, int rc)
{
    PVGASTATE                           pVGAState = PPDMIDISPLAYVBVACALLBACKS_2_PVGASTATE(pInterface);
    PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE   pHdr      = VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(pCmd);
    Assert(pHdr->uMagic == VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_MAGIC);

    pHdr->rc = rc;
    if (pHdr->pfnCompletion)
        pHdr->pfnCompletion(pVGAState, pCmd, pHdr->pvCompletion);
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNCRCTLCOMPLETION,
 *      Completion callback for vboxVDMACrCtlPost. }
 */
static DECLCALLBACK(void) vboxVDMACrCtlCbSetEvent(PVGASTATE pVGAState, PVBOXVDMACMD_CHROMIUM_CTL pCmd, void *pvContext)
{
    PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pHdr = (PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE)pvContext;
    Assert(pHdr == VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(pCmd));
    Assert(pHdr->uMagic == VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_MAGIC);
    RT_NOREF(pVGAState, pCmd);

    int rc = RTSemEventSignal(pHdr->hEvtDone);
    AssertRC(rc);

    vboxVDMACrCtlRelease(&pHdr->Cmd);
}

/**
 * Worker for vboxVDMACrCtlPost().
 */
static int vboxVDMACrCtlPostAsync(PVGASTATE pVGAState, PVBOXVDMACMD_CHROMIUM_CTL pCmd, uint32_t cbCmd,
                                  PFNVBOXVDMACRCTL_CALLBACK pfnCompletion, void *pvCompletion)
{
    if (   pVGAState->pDrv
        && pVGAState->pDrv->pfnCrHgsmiControlProcess)
    {
        PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pHdr = VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(pCmd);
        pHdr->pfnCompletion = pfnCompletion;
        pHdr->pvCompletion  = pvCompletion;
        pVGAState->pDrv->pfnCrHgsmiControlProcess(pVGAState->pDrv, pCmd, cbCmd);
        return VINF_SUCCESS;
    }
    return VERR_NOT_SUPPORTED;
}

/**
 * Posts stuff and waits.
 */
static int vboxVDMACrCtlPost(PVGASTATE pVGAState, PVBOXVDMACMD_CHROMIUM_CTL pCmd, uint32_t cbCmd)
{
    PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pHdr = VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(pCmd);

    /* Allocate the semaphore. */
    Assert(pHdr->hEvtDone == NIL_RTSEMEVENT);
    int rc = RTSemEventCreate(&pHdr->hEvtDone);
    AssertRCReturn(rc, rc);

    /* Grab a reference for the completion routine. */
    vboxVDMACrCtlRetain(&pHdr->Cmd);

    /* Submit and wait for it. */
    rc = vboxVDMACrCtlPostAsync(pVGAState, pCmd, cbCmd, vboxVDMACrCtlCbSetEvent, pHdr);
    if (RT_SUCCESS(rc))
        rc = RTSemEventWaitNoResume(pHdr->hEvtDone, RT_INDEFINITE_WAIT);
    else
    {
        if (rc != VERR_NOT_SUPPORTED)
            AssertRC(rc);
        vboxVDMACrCtlRelease(pCmd);
    }
    return rc;
}


/**
 * Structure for passing data between vboxVDMACrHgcmSubmitSync() and the
 * completion routine vboxVDMACrHgcmSubmitSyncCompletion().
 */
typedef struct VDMA_VBVA_CTL_CYNC_COMPLETION
{
    int volatile rc;
    RTSEMEVENT hEvent;
} VDMA_VBVA_CTL_CYNC_COMPLETION;

/**
 * @callback_method_impl{FNCRCTLCOMPLETION,
 *      Completion callback for vboxVDMACrHgcmSubmitSync() that signals the
 *      waiting thread.}
 */
static DECLCALLBACK(void) vboxVDMACrHgcmSubmitSyncCompletion(struct VBOXCRCMDCTL *pCmd, uint32_t cbCmd, int rc, void *pvCompletion)
{
    VDMA_VBVA_CTL_CYNC_COMPLETION *pData = (VDMA_VBVA_CTL_CYNC_COMPLETION*)pvCompletion;
    pData->rc = rc;
    rc = RTSemEventSignal(pData->hEvent);
    AssertLogRelRC(rc);

    RT_NOREF(pCmd, cbCmd);
}

/**
 * Worker for vboxVDMACrHgcmHandleEnable() and vdmaVBVAEnableProcess() that
 * works pVGAState->pDrv->pfnCrHgcmCtlSubmit.
 *
 * @thread VDMA
 */
static int vboxVDMACrHgcmSubmitSync(struct VBOXVDMAHOST *pVdma, VBOXCRCMDCTL* pCtl, uint32_t cbCtl)
{
    VDMA_VBVA_CTL_CYNC_COMPLETION Data;
    Data.rc = VERR_NOT_IMPLEMENTED;
    int rc = RTSemEventCreate(&Data.hEvent);
    if (!RT_SUCCESS(rc))
    {
        WARN(("RTSemEventCreate failed %Rrc\n", rc));
        return rc;
    }

    pCtl->CalloutList.List.pNext = NULL;

    PVGASTATE pVGAState = pVdma->pVGAState;
    rc = pVGAState->pDrv->pfnCrHgcmCtlSubmit(pVGAState->pDrv, pCtl, cbCtl, vboxVDMACrHgcmSubmitSyncCompletion, &Data);
    if (RT_SUCCESS(rc))
    {
        rc = RTSemEventWait(Data.hEvent, RT_INDEFINITE_WAIT);
        if (RT_SUCCESS(rc))
        {
            rc = Data.rc;
            if (!RT_SUCCESS(rc))
            {
                WARN(("pfnCrHgcmCtlSubmit command failed %Rrc\n", rc));
            }

        }
        else
            WARN(("RTSemEventWait failed %Rrc\n", rc));
    }
    else
        WARN(("pfnCrHgcmCtlSubmit failed %Rrc\n", rc));


    RTSemEventDestroy(Data.hEvent);

    return rc;
}


/**
 * Worker for vboxVDMAReset().
 */
static int vdmaVBVACtlDisableSync(PVBOXVDMAHOST pVdma)
{
    VBVAEXHOSTCTL HCtl;
    RT_ZERO(HCtl);
    HCtl.enmType = VBVAEXHOSTCTL_TYPE_GHH_DISABLE;
    int rc = vdmaVBVACtlSubmitSync(pVdma, &HCtl, VBVAEXHOSTCTL_SOURCE_HOST);
    if (RT_SUCCESS(rc))
        vgaUpdateDisplayAll(pVdma->pVGAState, /* fFailOnResize = */ false);
    else
        Log(("vdmaVBVACtlSubmitSync failed %Rrc\n", rc));
    return rc;
}


/**
 * @interface_method_impl{VBOXCRCMDCTL_HGCMENABLE_DATA,pfnRHCmd,
 *      Used by vboxVDMACrHgcmNotifyTerminatingCb() and called by
 *      crVBoxServerCrCmdDisablePostProcess() during crServerTearDown() to drain
 *      command queues or something.}
 */
static DECLCALLBACK(uint8_t *)
vboxVDMACrHgcmHandleEnableRemainingHostCommand(HVBOXCRCMDCTL_REMAINING_HOST_COMMAND hClient, uint32_t *pcbCtl, int prevCmdRc)
{
    struct VBOXVDMAHOST *pVdma = hClient;

    if (!pVdma->pCurRemainingHostCtl)
        VBoxVBVAExHSDisable(&pVdma->CmdVbva); /* disable VBVA, all subsequent host commands will go HGCM way */
    else
        VBoxVBVAExHPDataCompleteCtl(&pVdma->CmdVbva, pVdma->pCurRemainingHostCtl, prevCmdRc);

    pVdma->pCurRemainingHostCtl = VBoxVBVAExHPCheckHostCtlOnDisable(&pVdma->CmdVbva);
    if (pVdma->pCurRemainingHostCtl)
    {
        *pcbCtl = pVdma->pCurRemainingHostCtl->u.cmd.cbCmd;
        return (uint8_t *)pVdma->pCurRemainingHostCtl->u.cmd.pvCmd;
    }

    *pcbCtl = 0;
    return NULL;
}

/**
 * @interface_method_impl{VBOXCRCMDCTL_HGCMDISABLE_DATA,pfnNotifyTermDone,
 *      Called by crServerTearDown().}
 */
static DECLCALLBACK(void) vboxVDMACrHgcmNotifyTerminatingDoneCb(HVBOXCRCMDCTL_NOTIFY_TERMINATING hClient)
{
# ifdef VBOX_STRICT
    struct VBOXVDMAHOST *pVdma = hClient;
    Assert(pVdma->CmdVbva.i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);
    Assert(pVdma->Thread.u32State == VBOXVDMATHREAD_STATE_TERMINATING);
# else
    RT_NOREF(hClient);
# endif
}

/**
 * @interface_method_impl{VBOXCRCMDCTL_HGCMDISABLE_DATA,pfnNotifyTerm,
 *      Called by crServerTearDown().}
 */
static DECLCALLBACK(int) vboxVDMACrHgcmNotifyTerminatingCb(HVBOXCRCMDCTL_NOTIFY_TERMINATING hClient,
                                                           VBOXCRCMDCTL_HGCMENABLE_DATA *pHgcmEnableData)
{
    struct VBOXVDMAHOST *pVdma = hClient;

    VBVAEXHOSTCTL HCtl;
    RT_ZERO(HCtl);
    HCtl.enmType = VBVAEXHOSTCTL_TYPE_HH_ON_HGCM_UNLOAD;
    int rc = vdmaVBVACtlSubmitSync(pVdma, &HCtl, VBVAEXHOSTCTL_SOURCE_HOST);

    pHgcmEnableData->hRHCmd   = pVdma;
    pHgcmEnableData->pfnRHCmd = vboxVDMACrHgcmHandleEnableRemainingHostCommand;

    if (rc == VERR_INVALID_STATE)
        rc = VINF_SUCCESS;
    else if (RT_FAILURE(rc))
        WARN(("vdmaVBVACtlSubmitSync failed %Rrc\n", rc));

    return rc;
}

/**
 * Worker for vdmaVBVAEnableProcess() and vdmaVBVADisableProcess().
 *
 * @thread VDMA
 */
static int vboxVDMACrHgcmHandleEnable(struct VBOXVDMAHOST *pVdma)
{
    VBOXCRCMDCTL_ENABLE Enable;
    RT_ZERO(Enable);
    Enable.Hdr.enmType   = VBOXCRCMDCTL_TYPE_ENABLE;
    Enable.Data.hRHCmd   = pVdma;
    Enable.Data.pfnRHCmd = vboxVDMACrHgcmHandleEnableRemainingHostCommand;

    int rc = vboxVDMACrHgcmSubmitSync(pVdma, &Enable.Hdr, sizeof (Enable));
    Assert(!pVdma->pCurRemainingHostCtl);
    if (RT_SUCCESS(rc))
    {
        Assert(!VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva));
        return VINF_SUCCESS;
    }

    Assert(VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva));
    WARN(("vboxVDMACrHgcmSubmitSync failed %Rrc\n", rc));
    return rc;
}

/**
 * Handles VBVAEXHOSTCTL_TYPE_GHH_ENABLE and VBVAEXHOSTCTL_TYPE_GHH_ENABLE_PAUSED
 * for vboxVDMACrGuestCtlProcess().
 *
 * @thread VDMA
 */
static int vdmaVBVAEnableProcess(struct VBOXVDMAHOST *pVdma, uint32_t u32Offset)
{
    if (VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva))
    {
        WARN(("vdma VBVA is already enabled\n"));
        return VERR_INVALID_STATE;
    }

    VBVABUFFER RT_UNTRUSTED_VOLATILE_GUEST *pVBVA
        = (VBVABUFFER RT_UNTRUSTED_VOLATILE_GUEST *)HGSMIOffsetToPointerHost(pVdma->pHgsmi, u32Offset);
    if (!pVBVA)
    {
        WARN(("invalid offset %d (%#x)\n", u32Offset, u32Offset));
        return VERR_INVALID_PARAMETER;
    }

    int rc = VBoxVBVAExHSEnable(&pVdma->CmdVbva, pVBVA, pVdma->pVGAState->vram_ptrR3, pVdma->pVGAState->vram_size);
    if (RT_SUCCESS(rc))
    {
        if (!pVdma->CrSrvInfo.pfnEnable)
        {
            /* "HGCM-less" mode. All inited. */
            return VINF_SUCCESS;
        }

        VBOXCRCMDCTL_DISABLE Disable;
        Disable.Hdr.enmType            = VBOXCRCMDCTL_TYPE_DISABLE;
        Disable.Data.hNotifyTerm       = pVdma;
        Disable.Data.pfnNotifyTerm     = vboxVDMACrHgcmNotifyTerminatingCb;
        Disable.Data.pfnNotifyTermDone = vboxVDMACrHgcmNotifyTerminatingDoneCb;
        rc = vboxVDMACrHgcmSubmitSync(pVdma, &Disable.Hdr, sizeof (Disable));
        if (RT_SUCCESS(rc))
        {
            PVGASTATE pVGAState = pVdma->pVGAState;
            VBOXCRCMD_SVRENABLE_INFO Info;
            Info.hCltScr                = pVGAState->pDrv;
            Info.pfnCltScrUpdateBegin   = pVGAState->pDrv->pfnVBVAUpdateBegin;
            Info.pfnCltScrUpdateProcess = pVGAState->pDrv->pfnVBVAUpdateProcess;
            Info.pfnCltScrUpdateEnd     = pVGAState->pDrv->pfnVBVAUpdateEnd;
            rc = pVdma->CrSrvInfo.pfnEnable(pVdma->CrSrvInfo.hSvr, &Info);
            if (RT_SUCCESS(rc))
                return VINF_SUCCESS;

            WARN(("pfnEnable failed %Rrc\n", rc));
            vboxVDMACrHgcmHandleEnable(pVdma);
        }
        else
            WARN(("vboxVDMACrHgcmSubmitSync failed %Rrc\n", rc));

        VBoxVBVAExHSDisable(&pVdma->CmdVbva);
    }
    else
        WARN(("VBoxVBVAExHSEnable failed %Rrc\n", rc));

    return rc;
}

/**
 * Worker for several vboxVDMACrHostCtlProcess() commands.
 *
 * @returns IPRT status code.
 * @param   pVdma           The VDMA channel.
 * @param   fDoHgcmEnable   ???
 * @thread  VDMA
 */
static int vdmaVBVADisableProcess(struct VBOXVDMAHOST *pVdma, bool fDoHgcmEnable)
{
    if (!VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva))
    {
        Log(("vdma VBVA is already disabled\n"));
        return VINF_SUCCESS;
    }

    if (!pVdma->CrSrvInfo.pfnDisable)
    {
        /* "HGCM-less" mode. Just undo what vdmaVBVAEnableProcess did. */
        VBoxVBVAExHSDisable(&pVdma->CmdVbva);
        return VINF_SUCCESS;
    }

    int rc = pVdma->CrSrvInfo.pfnDisable(pVdma->CrSrvInfo.hSvr);
    if (RT_SUCCESS(rc))
    {
        if (fDoHgcmEnable)
        {
            PVGASTATE pVGAState = pVdma->pVGAState;

            /* disable is a bit tricky
             * we need to ensure the host ctl commands do not come out of order
             * and do not come over HGCM channel until after it is enabled */
            rc = vboxVDMACrHgcmHandleEnable(pVdma);
            if (RT_SUCCESS(rc))
            {
                vdmaVBVANotifyDisable(pVGAState);
                return VINF_SUCCESS;
            }

            VBOXCRCMD_SVRENABLE_INFO Info;
            Info.hCltScr                = pVGAState->pDrv;
            Info.pfnCltScrUpdateBegin   = pVGAState->pDrv->pfnVBVAUpdateBegin;
            Info.pfnCltScrUpdateProcess = pVGAState->pDrv->pfnVBVAUpdateProcess;
            Info.pfnCltScrUpdateEnd     = pVGAState->pDrv->pfnVBVAUpdateEnd;
            pVdma->CrSrvInfo.pfnEnable(pVdma->CrSrvInfo.hSvr, &Info); /** @todo ignoring return code */
        }
    }
    else
        WARN(("pfnDisable failed %Rrc\n", rc));

    return rc;
}

/**
 * Handles VBVAEXHOST_DATA_TYPE_HOSTCTL for vboxVDMAWorkerThread.
 *
 * @returns VBox status code.
 * @param   pVdma                   The VDMA channel.
 * @param   pCmd                    The control command to process.  Should be
 *                                  safe, i.e. not shared with guest.
 * @param   pfContinue              Where to return whether to continue or not.
 * @thread  VDMA
 */
static int vboxVDMACrHostCtlProcess(struct VBOXVDMAHOST *pVdma, VBVAEXHOSTCTL *pCmd, bool *pfContinue)
{
    *pfContinue = true;

    int rc;
    switch (pCmd->enmType)
    {
        /*
         * See vdmaVBVACtlOpaqueHostSubmit() and its callers.
         */
        case VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE:
            if (VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva))
            {
                if (pVdma->CrSrvInfo.pfnHostCtl)
                    return pVdma->CrSrvInfo.pfnHostCtl(pVdma->CrSrvInfo.hSvr, (uint8_t *)pCmd->u.cmd.pvCmd, pCmd->u.cmd.cbCmd);
                WARN(("VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE for disabled vdma VBVA\n"));
            }
            else
                WARN(("VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE for HGCM-less mode\n"));
            return VERR_INVALID_STATE;

        /*
         * See vdmaVBVACtlDisableSync().
         */
        case VBVAEXHOSTCTL_TYPE_GHH_DISABLE:
            rc = vdmaVBVADisableProcess(pVdma, true /* fDoHgcmEnable */);
            if (RT_SUCCESS(rc))
                rc = VBoxVDMAThreadTerm(&pVdma->Thread, NULL, NULL, false /* fNotify */ );
            else
                WARN(("vdmaVBVADisableProcess failed %Rrc\n", rc));
            return rc;

        /*
         * See vboxVDMACrHgcmNotifyTerminatingCb().
         */
        case VBVAEXHOSTCTL_TYPE_HH_ON_HGCM_UNLOAD:
            rc = vdmaVBVADisableProcess(pVdma, false /* fDoHgcmEnable */);
            if (RT_SUCCESS(rc))
            {
                rc = VBoxVDMAThreadTerm(&pVdma->Thread, NULL, NULL, true /* fNotify */);
                if (RT_SUCCESS(rc))
                    *pfContinue = false;
                else
                    WARN(("VBoxVDMAThreadTerm failed %Rrc\n", rc));
            }
            else
                WARN(("vdmaVBVADisableProcess failed %Rrc\n", rc));
            return rc;

        /*
         * See vboxVDMASaveStateExecPerform().
         */
        case VBVAEXHOSTCTL_TYPE_HH_SAVESTATE:
            rc = VBoxVBVAExHSSaveState(&pVdma->CmdVbva, pVdma->pVGAState->vram_ptrR3, pCmd->u.state.pSSM);
            if (RT_SUCCESS(rc))
            {
                VGA_SAVED_STATE_PUT_MARKER(pCmd->u.state.pSSM, 4);
                if (pVdma->CrSrvInfo.pfnSaveState)
                    rc = pVdma->CrSrvInfo.pfnSaveState(pVdma->CrSrvInfo.hSvr, pCmd->u.state.pSSM);
            }
            else
                WARN(("VBoxVBVAExHSSaveState failed %Rrc\n", rc));
            return rc;

        /*
         * See vboxVDMASaveLoadExecPerform().
         */
        case VBVAEXHOSTCTL_TYPE_HH_LOADSTATE:
            rc = VBoxVBVAExHSLoadState(&pVdma->CmdVbva, pVdma->pVGAState->vram_ptrR3, pCmd->u.state.pSSM, pCmd->u.state.u32Version);
            if (RT_SUCCESS(rc))
            {
                VGA_SAVED_STATE_GET_MARKER_RETURN_ON_MISMATCH(pCmd->u.state.pSSM, pCmd->u.state.u32Version, 4);
                if (pVdma->CrSrvInfo.pfnLoadState)
                {
                    rc = pVdma->CrSrvInfo.pfnLoadState(pVdma->CrSrvInfo.hSvr, pCmd->u.state.pSSM, pCmd->u.state.u32Version);
                    if (RT_FAILURE(rc))
                        WARN(("pfnLoadState failed %Rrc\n", rc));
                }
            }
            else
                WARN(("VBoxVBVAExHSLoadState failed %Rrc\n", rc));
            return rc;

        /*
         * See vboxVDMASaveLoadDone().
         */
        case VBVAEXHOSTCTL_TYPE_HH_LOADSTATE_DONE:
        {
            PVGASTATE pVGAState = pVdma->pVGAState;
            for (uint32_t i = 0; i < pVGAState->cMonitors; ++i)
            {
                VBVAINFOSCREEN CurScreen;
                VBVAINFOVIEW   CurView;
                rc = VBVAGetInfoViewAndScreen(pVGAState, i, &CurView, &CurScreen);
                AssertLogRelMsgRCReturn(rc, ("VBVAGetInfoViewAndScreen [screen #%u] -> %#x\n", i, rc), rc);

                rc = VBVAInfoScreen(pVGAState, &CurScreen);
                AssertLogRelMsgRCReturn(rc, ("VBVAInfoScreen [screen #%u] -> %#x\n", i, rc), rc);
            }

            return VINF_SUCCESS;
        }

        default:
            WARN(("unexpected host ctl type %d\n", pCmd->enmType));
            return VERR_INVALID_PARAMETER;
    }
}

/**
 * Worker for vboxVDMACrGuestCtlResizeEntryProcess.
 *
 * @returns VINF_SUCCESS or VERR_INVALID_PARAMETER.
 * @param   pVGAState           The VGA device state.
 * @param   pScreen             The screen info (safe copy).
 */
static int vboxVDMASetupScreenInfo(PVGASTATE pVGAState, VBVAINFOSCREEN *pScreen)
{
    const uint32_t idxView = pScreen->u32ViewIndex;
    const uint16_t fFlags  = pScreen->u16Flags;

    if (fFlags & VBVA_SCREEN_F_DISABLED)
    {
        if (   idxView < pVGAState->cMonitors
            || idxView == UINT32_C(0xFFFFFFFF))
        {
            RT_UNTRUSTED_VALIDATED_FENCE();

            RT_ZERO(*pScreen);
            pScreen->u32ViewIndex = idxView;
            pScreen->u16Flags     = VBVA_SCREEN_F_ACTIVE | VBVA_SCREEN_F_DISABLED;
            return VINF_SUCCESS;
        }
    }
    else
    {
        if (fFlags & VBVA_SCREEN_F_BLANK2)
        {
            if (   idxView >= pVGAState->cMonitors
                && idxView != UINT32_C(0xFFFFFFFF))
                return VERR_INVALID_PARAMETER;
            RT_UNTRUSTED_VALIDATED_FENCE();

            /* Special case for blanking using current video mode.
             * Only 'u16Flags' and 'u32ViewIndex' field are relevant.
             */
            RT_ZERO(*pScreen);
            pScreen->u32ViewIndex = idxView;
            pScreen->u16Flags     = fFlags;
            return VINF_SUCCESS;
        }

        if (   idxView < pVGAState->cMonitors
            && pScreen->u16BitsPerPixel <= 32
            && pScreen->u32Width <= UINT16_MAX
            && pScreen->u32Height <= UINT16_MAX
            && pScreen->u32LineSize <= UINT16_MAX * 4)
        {
            const uint32_t u32BytesPerPixel = (pScreen->u16BitsPerPixel + 7) / 8;
            if (pScreen->u32Width <= pScreen->u32LineSize / (u32BytesPerPixel? u32BytesPerPixel: 1))
            {
                const uint64_t u64ScreenSize = (uint64_t)pScreen->u32LineSize * pScreen->u32Height;
                if (   pScreen->u32StartOffset <= pVGAState->vram_size
                    && u64ScreenSize           <= pVGAState->vram_size
                    && pScreen->u32StartOffset <= pVGAState->vram_size - (uint32_t)u64ScreenSize)
                    return VINF_SUCCESS;
            }
        }
    }

    LogFunc(("Failed\n"));
    return VERR_INVALID_PARAMETER;
}

/**
 * Handles on entry in a VBVAEXHOSTCTL_TYPE_GHH_RESIZE command.
 *
 * @returns IPRT status code.
 * @param   pVdma               The VDMA channel
 * @param   pEntry              The entry to handle.  Considered volatile.
 *
 * @thread  VDMA
 */
static int vboxVDMACrGuestCtlResizeEntryProcess(struct VBOXVDMAHOST *pVdma,
                                                VBOXCMDVBVA_RESIZE_ENTRY RT_UNTRUSTED_VOLATILE_GUEST *pEntry)
{
    PVGASTATE pVGAState = pVdma->pVGAState;

    VBVAINFOSCREEN Screen;
    RT_COPY_VOLATILE(Screen, pEntry->Screen);
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

    /* Verify and cleanup local copy of the input data. */
    int rc = vboxVDMASetupScreenInfo(pVGAState, &Screen);
    if (RT_FAILURE(rc))
    {
        WARN(("invalid screen data\n"));
        return rc;
    }
    RT_UNTRUSTED_VALIDATED_FENCE();

    VBOXCMDVBVA_SCREENMAP_DECL(uint32_t, aTargetMap);
    RT_BCOPY_VOLATILE(aTargetMap, pEntry->aTargetMap, sizeof(aTargetMap));
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

    ASMBitClearRange(aTargetMap, pVGAState->cMonitors, VBOX_VIDEO_MAX_SCREENS);

    if (pVdma->CrSrvInfo.pfnResize)
    {
        /* Also inform the HGCM service, if it is there. */
        rc = pVdma->CrSrvInfo.pfnResize(pVdma->CrSrvInfo.hSvr, &Screen, aTargetMap);
        if (RT_FAILURE(rc))
        {
            WARN(("pfnResize failed %Rrc\n", rc));
            return rc;
        }
    }

    /* A fake view which contains the current screen for the 2D VBVAInfoView. */
    VBVAINFOVIEW View;
    View.u32ViewOffset    = 0;
    View.u32ViewSize      = Screen.u32LineSize * Screen.u32Height + Screen.u32StartOffset;
    View.u32MaxScreenSize = Screen.u32LineSize * Screen.u32Height;

    const bool fDisable = RT_BOOL(Screen.u16Flags & VBVA_SCREEN_F_DISABLED);

    for (int i = ASMBitFirstSet(aTargetMap, pVGAState->cMonitors);
         i >= 0;
         i = ASMBitNextSet(aTargetMap, pVGAState->cMonitors, i))
    {
        Screen.u32ViewIndex = i;

        VBVAINFOSCREEN CurScreen;
        VBVAINFOVIEW CurView;

        rc = VBVAGetInfoViewAndScreen(pVGAState, i, &CurView, &CurScreen);
        AssertRC(rc);

        if (!memcmp(&Screen, &CurScreen, sizeof (CurScreen)))
            continue;

        /* The view does not change if _BLANK2 is set. */
        if (   (!fDisable || !CurView.u32ViewSize)
            && !RT_BOOL(Screen.u16Flags & VBVA_SCREEN_F_BLANK2))
        {
            View.u32ViewIndex = Screen.u32ViewIndex;

            rc = VBVAInfoView(pVGAState, &View);
            if (RT_FAILURE(rc))
            {
                WARN(("VBVAInfoView failed %Rrc\n", rc));
                break;
            }
        }

        rc = VBVAInfoScreen(pVGAState, &Screen);
        if (RT_FAILURE(rc))
        {
            WARN(("VBVAInfoScreen failed %Rrc\n", rc));
            break;
        }
    }

    return rc;
}


/**
 * Processes VBVAEXHOST_DATA_TYPE_GUESTCTL for vboxVDMAWorkerThread and
 * vdmaVBVACtlThreadCreatedEnable.
 *
 * @returns VBox status code.
 * @param   pVdma               The VDMA channel.
 * @param   pCmd                The command to process.  Maybe safe (not shared
 *                              with guest).
 *
 * @thread  VDMA
 */
static int vboxVDMACrGuestCtlProcess(struct VBOXVDMAHOST *pVdma, VBVAEXHOSTCTL *pCmd)
{
    VBVAEXHOSTCTL_TYPE enmType = pCmd->enmType;
    switch (enmType)
    {
        /*
         * See handling of VBOXCMDVBVACTL_TYPE_3DCTL in vboxCmdVBVACmdCtl().
         */
        case VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE:
            ASSERT_GUEST_LOGREL_RETURN(VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva), VERR_INVALID_STATE);
            ASSERT_GUEST_LOGREL_RETURN(pVdma->CrSrvInfo.pfnGuestCtl, VERR_INVALID_STATE);
            return pVdma->CrSrvInfo.pfnGuestCtl(pVdma->CrSrvInfo.hSvr,
                                                (uint8_t RT_UNTRUSTED_VOLATILE_GUEST *)pCmd->u.cmd.pvCmd,
                                                pCmd->u.cmd.cbCmd);

        /*
         * See handling of VBOXCMDVBVACTL_TYPE_RESIZE in vboxCmdVBVACmdCtl().
         */
        case VBVAEXHOSTCTL_TYPE_GHH_RESIZE:
        {
            ASSERT_GUEST_RETURN(VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva), VERR_INVALID_STATE);
            uint32_t cbCmd = pCmd->u.cmd.cbCmd;
            ASSERT_GUEST_LOGREL_MSG_RETURN(   !(cbCmd % sizeof(VBOXCMDVBVA_RESIZE_ENTRY))
                                           && cbCmd > 0,
                                           ("cbCmd=%#x\n", cbCmd), VERR_INVALID_PARAMETER);

            uint32_t const cElements = cbCmd / sizeof(VBOXCMDVBVA_RESIZE_ENTRY);
            VBOXCMDVBVA_RESIZE RT_UNTRUSTED_VOLATILE_GUEST *pResize
                = (VBOXCMDVBVA_RESIZE RT_UNTRUSTED_VOLATILE_GUEST *)pCmd->u.cmd.pvCmd;
            for (uint32_t i = 0; i < cElements; ++i)
            {
                VBOXCMDVBVA_RESIZE_ENTRY RT_UNTRUSTED_VOLATILE_GUEST *pEntry = &pResize->aEntries[i];
                int rc = vboxVDMACrGuestCtlResizeEntryProcess(pVdma, pEntry);
                ASSERT_GUEST_LOGREL_MSG_RC_RETURN(rc, ("vboxVDMACrGuestCtlResizeEntryProcess failed for #%u: %Rrc\n", i, rc), rc);
            }
            return VINF_SUCCESS;
        }

        /*
         * See vdmaVBVACtlEnableSubmitInternal().
         */
        case VBVAEXHOSTCTL_TYPE_GHH_ENABLE:
        case VBVAEXHOSTCTL_TYPE_GHH_ENABLE_PAUSED:
        {
            ASSERT_GUEST(pCmd->u.cmd.cbCmd == sizeof(VBVAENABLE));

            VBVAENABLE RT_UNTRUSTED_VOLATILE_GUEST *pEnable = (VBVAENABLE RT_UNTRUSTED_VOLATILE_GUEST *)pCmd->u.cmd.pvCmd;
            uint32_t const u32Offset = pEnable->u32Offset;
            RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

            int rc = vdmaVBVAEnableProcess(pVdma, u32Offset);
            ASSERT_GUEST_MSG_RC_RETURN(rc, ("vdmaVBVAEnableProcess -> %Rrc\n", rc), rc);

            if (enmType == VBVAEXHOSTCTL_TYPE_GHH_ENABLE_PAUSED)
            {
                rc = VBoxVBVAExHPPause(&pVdma->CmdVbva);
                ASSERT_GUEST_MSG_RC_RETURN(rc, ("VBoxVBVAExHPPause -> %Rrc\n", rc), rc);
            }
            return VINF_SUCCESS;
        }

        /*
         * See vdmaVBVACtlDisableSubmitInternal().
         */
        case VBVAEXHOSTCTL_TYPE_GHH_DISABLE:
        {
            int rc = vdmaVBVADisableProcess(pVdma, true /* fDoHgcmEnable */);
            ASSERT_GUEST_MSG_RC_RETURN(rc, ("vdmaVBVADisableProcess -> %Rrc\n", rc), rc);

            /* do vgaUpdateDisplayAll right away */
            VMR3ReqCallNoWait(PDMDevHlpGetVM(pVdma->pVGAState->pDevInsR3), VMCPUID_ANY,
                              (PFNRT)vgaUpdateDisplayAll, 2, pVdma->pVGAState, /* fFailOnResize = */ false);

            return VBoxVDMAThreadTerm(&pVdma->Thread, NULL, NULL, false /* fNotify */);
        }

        default:
            ASSERT_GUEST_LOGREL_MSG_FAILED(("unexpected ctl type %d\n", enmType));
            return VERR_INVALID_PARAMETER;
    }
}


/**
 * Copies one page in a VBOXCMDVBVA_OPTYPE_PAGING_TRANSFER command.
 *
 * @param fIn - whether this is a page in or out op.
 * @thread VDMA
 *
 * the direction is VRA#M - related, so fIn == true - transfer to VRAM); false - transfer from VRAM
 */
static int vboxVDMACrCmdVbvaProcessPagingEl(PPDMDEVINS pDevIns, VBOXCMDVBVAPAGEIDX uPageNo, uint8_t *pbVram, bool fIn)
{
    RTGCPHYS       GCPhysPage = (RTGCPHYS)uPageNo << X86_PAGE_SHIFT;
    PGMPAGEMAPLOCK Lock;

    if (fIn)
    {
        const void *pvPage;
        int rc = PDMDevHlpPhysGCPhys2CCPtrReadOnly(pDevIns, GCPhysPage, 0, &pvPage, &Lock);
        ASSERT_GUEST_LOGREL_MSG_RC_RETURN(rc, ("PDMDevHlpPhysGCPhys2CCPtrReadOnly %RGp -> %Rrc\n", GCPhysPage, rc), rc);

        memcpy(pbVram, pvPage, PAGE_SIZE);
        PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);
    }
    else
    {
        void *pvPage;
        int rc = PDMDevHlpPhysGCPhys2CCPtr(pDevIns, GCPhysPage, 0, &pvPage, &Lock);
        ASSERT_GUEST_LOGREL_MSG_RC_RETURN(rc, ("PDMDevHlpPhysGCPhys2CCPtr %RGp -> %Rrc\n", GCPhysPage, rc), rc);

        memcpy(pvPage, pbVram, PAGE_SIZE);
        PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);
    }

    return VINF_SUCCESS;
}

/**
 * Handles a VBOXCMDVBVA_OPTYPE_PAGING_TRANSFER command.
 *
 * @return 0 on success, -1 on failure.
 *
 * @thread VDMA
 */
static int8_t vboxVDMACrCmdVbvaPageTransfer(PVGASTATE pVGAState, VBOXCMDVBVA_HDR const RT_UNTRUSTED_VOLATILE_GUEST *pHdr,
                                            uint32_t cbCmd, const VBOXCMDVBVA_PAGING_TRANSFER_DATA RT_UNTRUSTED_VOLATILE_GUEST *pData)
{
    /*
     * Extract and validate information.
     */
    ASSERT_GUEST_MSG_RETURN(cbCmd >= sizeof(VBOXCMDVBVA_PAGING_TRANSFER), ("%#x\n", cbCmd), -1);

    bool const fIn = RT_BOOL(pHdr->u8Flags & VBOXCMDVBVA_OPF_PAGING_TRANSFER_IN);
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

    uint32_t cbPageNumbers = cbCmd - RT_OFFSETOF(VBOXCMDVBVA_PAGING_TRANSFER, Data.aPageNumbers);
    ASSERT_GUEST_MSG_RETURN(!(cbPageNumbers % sizeof(VBOXCMDVBVAPAGEIDX)), ("%#x\n", cbPageNumbers), -1);
    VBOXCMDVBVAPAGEIDX const cPages = cbPageNumbers / sizeof(VBOXCMDVBVAPAGEIDX);

    VBOXCMDVBVAOFFSET offVRam = pData->Alloc.u.offVRAM;
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
    ASSERT_GUEST_MSG_RETURN(!(offVRam & X86_PAGE_OFFSET_MASK), ("%#x\n", offVRam), -1);
    ASSERT_GUEST_MSG_RETURN(offVRam < pVGAState->vram_size, ("%#x vs %#x\n", offVRam, pVGAState->vram_size), -1);
    uint32_t cVRamPages = (pVGAState->vram_size - offVRam) >> X86_PAGE_SHIFT;
    ASSERT_GUEST_MSG_RETURN(cPages <= cVRamPages, ("cPages=%#x vs cVRamPages=%#x @ offVRam=%#x\n", cPages, cVRamPages, offVRam), -1);

    RT_UNTRUSTED_VALIDATED_FENCE();

    /*
     * Execute the command.
     */
    uint8_t *pbVRam = (uint8_t *)pVGAState->vram_ptrR3 + offVRam;
    for (uint32_t iPage = 0; iPage < cPages; iPage++, pbVRam += X86_PAGE_SIZE)
    {
        uint32_t uPageNo = pData->aPageNumbers[iPage];
        RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
        int rc = vboxVDMACrCmdVbvaProcessPagingEl(pVGAState->pDevInsR3, uPageNo, pbVRam, fIn);
        ASSERT_GUEST_MSG_RETURN(RT_SUCCESS(rc), ("#%#x: uPageNo=%#x rc=%Rrc\n", iPage, uPageNo, rc), -1);
    }
    return 0;
}


/**
 * Handles VBOXCMDVBVA_OPTYPE_PAGING_FILL.
 *
 * @returns 0 on success, -1 on failure.
 * @param   pVGAState           The VGA state.
 * @param   pFill               The fill command (volatile).
 *
 * @thread VDMA
 */
static int8_t vboxVDMACrCmdVbvaPagingFill(PVGASTATE pVGAState, VBOXCMDVBVA_PAGING_FILL RT_UNTRUSTED_VOLATILE_GUEST *pFill)
{
    /*
     * Copy and validate input.
     */
    VBOXCMDVBVA_PAGING_FILL FillSafe;
    RT_COPY_VOLATILE(FillSafe, *pFill);
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

    VBOXCMDVBVAOFFSET offVRAM = FillSafe.offVRAM;
    ASSERT_GUEST_MSG_RETURN(!(offVRAM & X86_PAGE_OFFSET_MASK), ("offVRAM=%#x\n", offVRAM), -1);
    ASSERT_GUEST_MSG_RETURN(offVRAM <= pVGAState->vram_size, ("offVRAM=%#x\n", offVRAM), -1);

    uint32_t cbFill = FillSafe.u32CbFill;
    ASSERT_GUEST_STMT(!(cbFill & 3), cbFill &= ~(uint32_t)3);
    ASSERT_GUEST_MSG_RETURN(   cbFill < pVGAState->vram_size
                            && offVRAM <= pVGAState->vram_size - cbFill,
                            ("offVRAM=%#x cbFill=%#x\n", offVRAM, cbFill), -1);

    RT_UNTRUSTED_VALIDATED_FENCE();

    /*
     * Execute.
     */
    uint32_t      *pu32Vram = (uint32_t *)((uint8_t *)pVGAState->vram_ptrR3 + offVRAM);
    uint32_t const u32Color = FillSafe.u32Pattern;

    uint32_t cLoops = cbFill / 4;
    while (cLoops-- > 0)
        pu32Vram[cLoops] = u32Color;

    return 0;
}

/**
 * Process command data.
 *
 * @returns zero or positive is success, negative failure.
 * @param   pVdma               The VDMA channel.
 * @param   pCmd                The command data to process. Assume volatile.
 * @param   cbCmd               The amount of command data.
 *
 * @thread VDMA
 */
static int8_t vboxVDMACrCmdVbvaProcessCmdData(struct VBOXVDMAHOST *pVdma,
                                              const VBOXCMDVBVA_HDR RT_UNTRUSTED_VOLATILE_GUEST *pCmd, uint32_t cbCmd)
{
    uint8_t bOpCode = pCmd->u8OpCode;
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
    switch (bOpCode)
    {
        case VBOXCMDVBVA_OPTYPE_NOPCMD:
            return 0;

        case VBOXCMDVBVA_OPTYPE_PAGING_TRANSFER:
            return vboxVDMACrCmdVbvaPageTransfer(pVdma->pVGAState, pCmd, cbCmd,
                                                 &((VBOXCMDVBVA_PAGING_TRANSFER RT_UNTRUSTED_VOLATILE_GUEST *)pCmd)->Data);

        case VBOXCMDVBVA_OPTYPE_PAGING_FILL:
            ASSERT_GUEST_RETURN(cbCmd == sizeof(VBOXCMDVBVA_PAGING_FILL), -1);
            return vboxVDMACrCmdVbvaPagingFill(pVdma->pVGAState, (VBOXCMDVBVA_PAGING_FILL RT_UNTRUSTED_VOLATILE_GUEST *)pCmd);

        default:
            ASSERT_GUEST_RETURN(pVdma->CrSrvInfo.pfnCmd != NULL, -1);
            return pVdma->CrSrvInfo.pfnCmd(pVdma->CrSrvInfo.hSvr, pCmd, cbCmd);
    }
}

# if 0
typedef struct VBOXCMDVBVA_PAGING_TRANSFER
{
    VBOXCMDVBVA_HDR Hdr;
    /* for now can only contain offVRAM.
     * paging transfer can NOT be initiated for allocations having host 3D object (hostID) associated */
    VBOXCMDVBVA_ALLOCINFO Alloc;
    uint32_t u32Reserved;
    VBOXCMDVBVA_SYSMEMEL aSysMem[1];
} VBOXCMDVBVA_PAGING_TRANSFER;
# endif

AssertCompile(sizeof (VBOXCMDVBVA_HDR) == 8);
AssertCompile(sizeof (VBOXCMDVBVA_ALLOCINFO) == 4);
AssertCompile(sizeof (VBOXCMDVBVAPAGEIDX) == 4);
AssertCompile(!(X86_PAGE_SIZE % sizeof (VBOXCMDVBVAPAGEIDX)));

# define VBOXCMDVBVA_NUM_SYSMEMEL_PER_PAGE (X86_PAGE_SIZE / sizeof (VBOXCMDVBVA_SYSMEMEL))

/**
 * Worker for vboxVDMACrCmdProcess.
 *
 * @returns 8-bit result.
 * @param   pVdma       The VDMA channel.
 * @param   pCmd        The command.  Consider volatile!
 * @param   cbCmd       The size of what @a pCmd points to.  At least
 *                      sizeof(VBOXCMDVBVA_HDR).
 * @param   fRecursion  Set if recursive call, false if not.
 *
 * @thread VDMA
 */
static int8_t vboxVDMACrCmdVbvaProcess(struct VBOXVDMAHOST *pVdma, const VBOXCMDVBVA_HDR RT_UNTRUSTED_VOLATILE_GUEST *pCmd,
                                       uint32_t cbCmd, bool fRecursion)
{
    int8_t        i8Result = 0;
    uint8_t const bOpCode  = pCmd->u8OpCode;
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
    LogRelFlow(("VDMA: vboxVDMACrCmdVbvaProcess: ENTER, bOpCode=%u\n", bOpCode));
    switch (bOpCode)
    {
        case VBOXCMDVBVA_OPTYPE_SYSMEMCMD:
        {
            /*
             * Extract the command physical address and size.
             */
            ASSERT_GUEST_MSG_RETURN(cbCmd >= sizeof(VBOXCMDVBVA_SYSMEMCMD), ("%#x\n", cbCmd), -1);
            RTGCPHYS GCPhysCmd  = ((VBOXCMDVBVA_SYSMEMCMD RT_UNTRUSTED_VOLATILE_GUEST *)pCmd)->phCmd;
            RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
            uint32_t cbCmdPart  = X86_PAGE_SIZE - (uint32_t)(GCPhysCmd & X86_PAGE_OFFSET_MASK);

            uint32_t cbRealCmd  = pCmd->u8Flags;
            cbRealCmd |= (uint32_t)pCmd->u.u8PrimaryID << 8;
            ASSERT_GUEST_MSG_RETURN(cbRealCmd >= sizeof(VBOXCMDVBVA_HDR), ("%#x\n", cbRealCmd), -1);
            ASSERT_GUEST_MSG_RETURN(cbRealCmd <= _1M, ("%#x\n", cbRealCmd), -1);

            /*
             * Lock down the first page of the memory specified by the command.
             */
            PGMPAGEMAPLOCK Lock;
            PVGASTATE pVGAState = pVdma->pVGAState;
            PPDMDEVINS pDevIns = pVGAState->pDevInsR3;
            VBOXCMDVBVA_HDR const *pRealCmdHdr = NULL;
            int rc = PDMDevHlpPhysGCPhys2CCPtrReadOnly(pDevIns, GCPhysCmd, 0, (const void **)&pRealCmdHdr, &Lock);
            ASSERT_GUEST_LOGREL_MSG_RC_RETURN(rc, ("VDMA: %RGp -> %Rrc\n", GCPhysCmd, rc), -1);
            Assert((GCPhysCmd & PAGE_OFFSET_MASK) == (((uintptr_t)pRealCmdHdr) & PAGE_OFFSET_MASK));

            /*
             * All fits within one page?  We can handle that pretty efficiently.
             */
            if (cbRealCmd <= cbCmdPart)
            {
                i8Result = vboxVDMACrCmdVbvaProcessCmdData(pVdma, pRealCmdHdr, cbRealCmd);
                PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);
            }
            else
            {
                /*
                 * To keep things damn simple, just double buffer cross page or
                 * multipage requests.
                 */
                uint8_t *pbCmdBuf = (uint8_t *)RTMemTmpAllocZ(RT_ALIGN_Z(cbRealCmd, 16));
                if (pbCmdBuf)
                {
                    memcpy(pbCmdBuf, pRealCmdHdr, cbCmdPart);
                    PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);
                    pRealCmdHdr = NULL;

                    rc = PDMDevHlpPhysRead(pDevIns, GCPhysCmd + cbCmdPart, &pbCmdBuf[cbCmdPart], cbRealCmd - cbCmdPart);
                    if (RT_SUCCESS(rc))
                        i8Result = vboxVDMACrCmdVbvaProcessCmdData(pVdma, (VBOXCMDVBVA_HDR const *)pbCmdBuf, cbRealCmd);
                    else
                        LogRelMax(200, ("VDMA: Error reading %#x bytes of guest memory %#RGp!\n", cbRealCmd, GCPhysCmd));
                    RTMemTmpFree(pbCmdBuf);
                }
                else
                {
                    PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);
                    LogRelMax(200, ("VDMA: Out of temporary memory! %#x\n", cbRealCmd));
                    i8Result = -1;
                }
            }
            return i8Result;
        }

        case VBOXCMDVBVA_OPTYPE_COMPLEXCMD:
        {
            Assert(cbCmd >= sizeof(VBOXCMDVBVA_HDR)); /* caller already checked this */
            ASSERT_GUEST_RETURN(!fRecursion, -1);

            /* Skip current command. */
            cbCmd -= sizeof(*pCmd);
            pCmd++;

            /* Process subcommands. */
            while (cbCmd > 0)
            {
                ASSERT_GUEST_MSG_RETURN(cbCmd >= sizeof(VBOXCMDVBVA_HDR), ("%#x\n", cbCmd), -1);

                uint16_t cbCurCmd = pCmd->u2.complexCmdEl.u16CbCmdHost;
                RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
                ASSERT_GUEST_MSG_RETURN(cbCurCmd <= cbCmd, ("cbCurCmd=%#x, cbCmd=%#x\n", cbCurCmd, cbCmd), -1);

                i8Result = vboxVDMACrCmdVbvaProcess(pVdma, pCmd, cbCurCmd, true /*fRecursive*/);
                ASSERT_GUEST_MSG_RETURN(i8Result >= 0, ("vboxVDMACrCmdVbvaProcess -> %d\n", i8Result), i8Result);

                /* Advance to the next command. */
                pCmd  = (VBOXCMDVBVA_HDR RT_UNTRUSTED_VOLATILE_GUEST *)((uintptr_t)pCmd + cbCurCmd);
                cbCmd -= cbCurCmd;
            }
            return 0;
        }

        default:
            i8Result = vboxVDMACrCmdVbvaProcessCmdData(pVdma, pCmd, cbCmd);
            LogRelFlow(("VDMA: vboxVDMACrCmdVbvaProcess: LEAVE, opCode(%i)\n", pCmd->u8OpCode));
            return i8Result;
    }
}

/**
 * Worker for vboxVDMAWorkerThread handling VBVAEXHOST_DATA_TYPE_CMD.
 *
 * @thread VDMA
 */
static void vboxVDMACrCmdProcess(struct VBOXVDMAHOST *pVdma, uint8_t RT_UNTRUSTED_VOLATILE_GUEST *pbCmd, uint32_t cbCmd)
{
    if (   cbCmd > 0
        && *pbCmd == VBOXCMDVBVA_OPTYPE_NOP)
    { /* nop */ }
    else
    {
        ASSERT_GUEST_RETURN_VOID(cbCmd >= sizeof(VBOXCMDVBVA_HDR));
        VBOXCMDVBVA_HDR RT_UNTRUSTED_VOLATILE_GUEST *pCmd = (VBOXCMDVBVA_HDR RT_UNTRUSTED_VOLATILE_GUEST *)pbCmd;

        /* check if the command is cancelled */
        if (ASMAtomicCmpXchgU8(&pCmd->u8State, VBOXCMDVBVA_STATE_IN_PROGRESS, VBOXCMDVBVA_STATE_SUBMITTED))
        {
            /* Process it. */
            pCmd->u.i8Result = vboxVDMACrCmdVbvaProcess(pVdma, pCmd, cbCmd, false /*fRecursion*/);
        }
        else
            Assert(pCmd->u8State == VBOXCMDVBVA_STATE_CANCELLED);
    }

}

/**
 * Worker for vboxVDMAConstruct().
 */
static int vboxVDMACrCtlHgsmiSetup(struct VBOXVDMAHOST *pVdma)
{
    PVBOXVDMACMD_CHROMIUM_CTL_CRHGSMI_SETUP pCmd;
    pCmd = (PVBOXVDMACMD_CHROMIUM_CTL_CRHGSMI_SETUP)vboxVDMACrCtlCreate(VBOXVDMACMD_CHROMIUM_CTL_TYPE_CRHGSMI_SETUP, sizeof(*pCmd));
    int rc;
    if (pCmd)
    {
        PVGASTATE pVGAState = pVdma->pVGAState;
        pCmd->pvVRamBase = pVGAState->vram_ptrR3;
        pCmd->cbVRam = pVGAState->vram_size;
        pCmd->pLed = &pVGAState->Led3D;
        pCmd->CrClientInfo.hClient = pVdma;
        pCmd->CrClientInfo.pfnCallout = vboxCmdVBVACmdCallout;
        rc = vboxVDMACrCtlPost(pVGAState, &pCmd->Hdr, sizeof (*pCmd));
        if (RT_SUCCESS(rc))
        {
            rc = vboxVDMACrCtlGetRc(&pCmd->Hdr);
            if (RT_SUCCESS(rc))
                pVdma->CrSrvInfo = pCmd->CrCmdServerInfo;
            else if (rc != VERR_NOT_SUPPORTED)
                WARN(("vboxVDMACrCtlGetRc returned %Rrc\n", rc));
        }
        else
            WARN(("vboxVDMACrCtlPost failed %Rrc\n", rc));

        vboxVDMACrCtlRelease(&pCmd->Hdr);
    }
    else
        rc = VERR_NO_MEMORY;

    if (!RT_SUCCESS(rc))
        memset(&pVdma->CrSrvInfo, 0, sizeof (pVdma->CrSrvInfo));

    return rc;
}

/**
 * @interface_method_impl{PDMIDISPLAYVBVACALLBACKS,pfnCrHgsmiControlCompleteAsync,
 *      Some indirect completion magic, you gotta love this code! }
 */
DECLCALLBACK(int) vboxVDMACrHgsmiCommandCompleteAsync(PPDMIDISPLAYVBVACALLBACKS pInterface, PVBOXVDMACMD_CHROMIUM_CMD pCmd, int rc)
{
    PVGASTATE                                    pVGAState = PPDMIDISPLAYVBVACALLBACKS_2_PVGASTATE(pInterface);
    PHGSMIINSTANCE                               pIns      = pVGAState->pHGSMI;
    VBOXVDMACMD RT_UNTRUSTED_VOLATILE_GUEST     *pDmaHdr   = VBOXVDMACMD_FROM_BODY(pCmd);
    VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_GUEST *pDr       = VBOXVDMACBUF_DR_FROM_TAIL(pDmaHdr);

    AssertRC(rc);
    pDr->rc = rc;

    Assert(pVGAState->fGuestCaps & VBVACAPS_COMPLETEGCMD_BY_IOREAD);
    rc = VBoxSHGSMICommandComplete(pIns, pDr);
    AssertRC(rc);

    return rc;
}

/**
 * Worker for vboxVDMACmdExecBlt().
 */
static int vboxVDMACmdExecBltPerform(PVBOXVDMAHOST pVdma, const VBOXVIDEOOFFSET offDst, const VBOXVIDEOOFFSET offSrc,
                                     const PVBOXVDMA_SURF_DESC pDstDesc, const PVBOXVDMA_SURF_DESC pSrcDesc,
                                     const VBOXVDMA_RECTL *pDstRectl, const VBOXVDMA_RECTL *pSrcRectl)
{
    /*
     * We do not support color conversion.
     */
    AssertReturn(pDstDesc->format == pSrcDesc->format, VERR_INVALID_FUNCTION);

    /* we do not support stretching (checked by caller) */
    Assert(pDstRectl->height == pSrcRectl->height);
    Assert(pDstRectl->width  == pSrcRectl->width);

    uint8_t *pbRam = pVdma->pVGAState->vram_ptrR3;
    AssertCompileSize(pVdma->pVGAState->vram_size, sizeof(uint32_t));
    uint32_t cbVRamSize = pVdma->pVGAState->vram_size;
    uint8_t *pbDstSurf = pbRam + offDst;
    uint8_t *pbSrcSurf = pbRam + offSrc;

    if (   pDstDesc->width == pDstRectl->width
        && pSrcDesc->width == pSrcRectl->width
        && pSrcDesc->width == pDstDesc->width
        && pSrcDesc->pitch == pDstDesc->pitch)
    {
        Assert(!pDstRectl->left);
        Assert(!pSrcRectl->left);
        uint32_t offBoth  = pDstDesc->pitch * pDstRectl->top;
        uint32_t cbToCopy = pDstDesc->pitch * pDstRectl->height;

        if (   cbToCopy <= cbVRamSize
            && (uintptr_t)(pbDstSurf + offBoth) - (uintptr_t)pbRam <= cbVRamSize - cbToCopy
            && (uintptr_t)(pbSrcSurf + offBoth) - (uintptr_t)pbRam <= cbVRamSize - cbToCopy)
        {
            RT_UNTRUSTED_VALIDATED_FENCE();
            memcpy(pbDstSurf + offBoth, pbSrcSurf + offBoth, cbToCopy);
        }
        else
            return VERR_INVALID_PARAMETER;
    }
    else
    {
        uint32_t offDstLineStart =   pDstRectl->left * pDstDesc->bpp >> 3;
        uint32_t offDstLineEnd   = ((pDstRectl->left * pDstDesc->bpp + 7) >> 3) + ((pDstDesc->bpp * pDstRectl->width + 7) >> 3);
        uint32_t cbDstLine       = offDstLineEnd - offDstLineStart;
        uint32_t offDstStart     = pDstDesc->pitch * pDstRectl->top + offDstLineStart;
        Assert(cbDstLine <= pDstDesc->pitch);
        uint32_t cbDstSkip       = pDstDesc->pitch;
        uint8_t *pbDstStart      = pbDstSurf + offDstStart;

        uint32_t offSrcLineStart =   pSrcRectl->left * pSrcDesc->bpp >> 3;
# ifdef VBOX_STRICT
        uint32_t offSrcLineEnd   = ((pSrcRectl->left * pSrcDesc->bpp + 7) >> 3) + ((pSrcDesc->bpp * pSrcRectl->width + 7) >> 3);
        uint32_t cbSrcLine       = offSrcLineEnd - offSrcLineStart;
# endif
        uint32_t offSrcStart     = pSrcDesc->pitch * pSrcRectl->top + offSrcLineStart;
        Assert(cbSrcLine <= pSrcDesc->pitch);
        uint32_t cbSrcSkip       = pSrcDesc->pitch;
        const uint8_t *pbSrcStart = pbSrcSurf + offSrcStart;

        Assert(cbDstLine == cbSrcLine);

        for (uint32_t i = 0; ; ++i)
        {
            if (   cbDstLine <= cbVRamSize
                && (uintptr_t)pbDstStart - (uintptr_t)pbRam <= cbVRamSize - cbDstLine
                && (uintptr_t)pbSrcStart - (uintptr_t)pbRam <= cbVRamSize - cbDstLine)
            {
                RT_UNTRUSTED_VALIDATED_FENCE(); /** @todo this could potentially be buzzkiller. */
                memcpy(pbDstStart, pbSrcStart, cbDstLine);
            }
            else
                return VERR_INVALID_PARAMETER;
            if (i == pDstRectl->height)
                break;
            pbDstStart += cbDstSkip;
            pbSrcStart += cbSrcSkip;
        }
    }
    return VINF_SUCCESS;
}

#if 0 /* unused */
static void vboxVDMARectlUnite(VBOXVDMA_RECTL * pRectl1, const VBOXVDMA_RECTL * pRectl2)
{
    if (!pRectl1->width)
        *pRectl1 = *pRectl2;
    else
    {
        int16_t x21 = pRectl1->left + pRectl1->width;
        int16_t x22 = pRectl2->left + pRectl2->width;
        if (pRectl1->left > pRectl2->left)
        {
            pRectl1->left = pRectl2->left;
            pRectl1->width = x21 < x22 ? x22 - pRectl1->left : x21 - pRectl1->left;
        }
        else if (x21 < x22)
            pRectl1->width = x22 - pRectl1->left;

        x21 = pRectl1->top + pRectl1->height;
        x22 = pRectl2->top + pRectl2->height;
        if (pRectl1->top > pRectl2->top)
        {
            pRectl1->top = pRectl2->top;
            pRectl1->height = x21 < x22 ? x22 - pRectl1->top : x21 - pRectl1->top;
        }
        else if (x21 < x22)
            pRectl1->height = x22 - pRectl1->top;
    }
}
#endif /* unused */

/**
 * Handles VBOXVDMACMD_TYPE_DMA_PRESENT_BLT for vboxVDMACmdExec().
 *
 * @returns number of bytes (positive) of the full command on success,
 *          otherwise a negative error status (VERR_XXX).
 *
 * @param   pVdma           The VDMA channel.
 * @param   pBlt            Blit command buffer.  This is to be considered
 *                          volatile!
 * @param   cbBuffer        Number of bytes accessible at @a pBtl.
 */
static int vboxVDMACmdExecBlt(PVBOXVDMAHOST pVdma, const VBOXVDMACMD_DMA_PRESENT_BLT RT_UNTRUSTED_VOLATILE_GUEST *pBlt,
                              uint32_t cbBuffer)
{
    /*
     * Validate and make a local copy of the blt command up to the rectangle array.
     */
    AssertReturn(cbBuffer >= RT_UOFFSETOF(VBOXVDMACMD_DMA_PRESENT_BLT, aDstSubRects), VERR_INVALID_PARAMETER);
    VBOXVDMACMD_DMA_PRESENT_BLT BltSafe;
    RT_BCOPY_VOLATILE(&BltSafe, (void const *)pBlt, RT_UOFFSETOF(VBOXVDMACMD_DMA_PRESENT_BLT, aDstSubRects));
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

    AssertReturn(BltSafe.cDstSubRects < _8M, VERR_INVALID_PARAMETER);
    uint32_t const cbBlt = RT_UOFFSETOF_DYN(VBOXVDMACMD_DMA_PRESENT_BLT, aDstSubRects[BltSafe.cDstSubRects]);
    AssertReturn(cbBuffer >= cbBlt, VERR_INVALID_PARAMETER);

    /*
     * We do not support stretching.
     */
    AssertReturn(BltSafe.srcRectl.width  == BltSafe.dstRectl.width,  VERR_INVALID_FUNCTION);
    AssertReturn(BltSafe.srcRectl.height == BltSafe.dstRectl.height, VERR_INVALID_FUNCTION);

    Assert(BltSafe.cDstSubRects);

    RT_UNTRUSTED_VALIDATED_FENCE();

    /*
     * Do the work.
     */
    //VBOXVDMA_RECTL updateRectl = {0, 0, 0, 0}; - pointless
    if (BltSafe.cDstSubRects)
    {
        for (uint32_t i = 0; i < BltSafe.cDstSubRects; ++i)
        {
            VBOXVDMA_RECTL dstSubRectl;
            dstSubRectl.left   = pBlt->aDstSubRects[i].left;
            dstSubRectl.top    = pBlt->aDstSubRects[i].top;
            dstSubRectl.width  = pBlt->aDstSubRects[i].width;
            dstSubRectl.height = pBlt->aDstSubRects[i].height;
            RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

            VBOXVDMA_RECTL srcSubRectl = dstSubRectl;

            dstSubRectl.left += BltSafe.dstRectl.left;
            dstSubRectl.top  += BltSafe.dstRectl.top;

            srcSubRectl.left += BltSafe.srcRectl.left;
            srcSubRectl.top  += BltSafe.srcRectl.top;

            int rc = vboxVDMACmdExecBltPerform(pVdma, BltSafe.offDst, BltSafe.offSrc, &BltSafe.dstDesc, &BltSafe.srcDesc,
                                               &dstSubRectl, &srcSubRectl);
            AssertRCReturn(rc, rc);

            //vboxVDMARectlUnite(&updateRectl, &dstSubRectl); - pointless
        }
    }
    else
    {
        int rc = vboxVDMACmdExecBltPerform(pVdma, BltSafe.offDst, BltSafe.offSrc, &BltSafe.dstDesc, &BltSafe.srcDesc,
                                           &BltSafe.dstRectl, &BltSafe.srcRectl);
        AssertRCReturn(rc, rc);

        //vboxVDMARectlUnite(&updateRectl, &BltSafe.dstRectl); - pointless
    }

    return cbBlt;
}


/**
 * Handles VBOXVDMACMD_TYPE_DMA_BPB_TRANSFER for vboxVDMACmdCheckCrCmd() and
 * vboxVDMACmdExec().
 *
 * @returns number of bytes (positive) of the full command on success,
 *          otherwise a negative error status (VERR_XXX).
 *
 * @param   pVdma           The VDMA channel.
 * @param   pTransfer       Transfer command buffer.  This is to be considered
 *                          volatile!
 * @param   cbBuffer        Number of bytes accessible at @a pTransfer.
 */
static int vboxVDMACmdExecBpbTransfer(PVBOXVDMAHOST pVdma, VBOXVDMACMD_DMA_BPB_TRANSFER RT_UNTRUSTED_VOLATILE_GUEST *pTransfer,
                                      uint32_t cbBuffer)
{
    /*
     * Make a copy of the command (it's volatile).
     */
    AssertReturn(cbBuffer >= sizeof(*pTransfer), VERR_INVALID_PARAMETER);
    VBOXVDMACMD_DMA_BPB_TRANSFER TransferSafeCopy;
    RT_COPY_VOLATILE(TransferSafeCopy, *pTransfer);
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

    PVGASTATE   pVGAState    = pVdma->pVGAState;
    PPDMDEVINS  pDevIns      = pVGAState->pDevInsR3;
    uint8_t    *pbRam        = pVGAState->vram_ptrR3;
    uint32_t    cbTransfer   = TransferSafeCopy.cbTransferSize;

    /*
     * Validate VRAM offset.
     */
    if (TransferSafeCopy.fFlags & VBOXVDMACMD_DMA_BPB_TRANSFER_F_SRC_VRAMOFFSET)
        AssertReturn(   cbTransfer <= pVGAState->vram_size
                     && TransferSafeCopy.Src.offVramBuf <= pVGAState->vram_size - cbTransfer,
                     VERR_INVALID_PARAMETER);

    if (TransferSafeCopy.fFlags & VBOXVDMACMD_DMA_BPB_TRANSFER_F_DST_VRAMOFFSET)
        AssertReturn(   cbTransfer <= pVGAState->vram_size
                     && TransferSafeCopy.Dst.offVramBuf <= pVGAState->vram_size - cbTransfer,
                     VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    /*
     * Transfer loop.
     */
    uint32_t    cbTransfered = 0;
    int         rc           = VINF_SUCCESS;
    do
    {
        uint32_t cbSubTransfer = cbTransfer;

        const void     *pvSrc;
        bool            fSrcLocked = false;
        PGMPAGEMAPLOCK  SrcLock;
        if (TransferSafeCopy.fFlags & VBOXVDMACMD_DMA_BPB_TRANSFER_F_SRC_VRAMOFFSET)
            pvSrc = pbRam + TransferSafeCopy.Src.offVramBuf + cbTransfered;
        else
        {
            RTGCPHYS GCPhysSrcPage = TransferSafeCopy.Src.phBuf + cbTransfered;
            rc = PDMDevHlpPhysGCPhys2CCPtrReadOnly(pDevIns, GCPhysSrcPage, 0, &pvSrc, &SrcLock);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                fSrcLocked = true;
                cbSubTransfer = RT_MIN(cbSubTransfer, X86_PAGE_SIZE - (uint32_t)(GCPhysSrcPage & X86_PAGE_OFFSET_MASK));
            }
            else
                break;
        }

        void           *pvDst;
        PGMPAGEMAPLOCK  DstLock;
        bool            fDstLocked = false;
        if (TransferSafeCopy.fFlags & VBOXVDMACMD_DMA_BPB_TRANSFER_F_DST_VRAMOFFSET)
            pvDst = pbRam + TransferSafeCopy.Dst.offVramBuf + cbTransfered;
        else
        {
            RTGCPHYS GCPhysDstPage = TransferSafeCopy.Dst.phBuf + cbTransfered;
            rc = PDMDevHlpPhysGCPhys2CCPtr(pDevIns, GCPhysDstPage, 0, &pvDst, &DstLock);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                fDstLocked = true;
                cbSubTransfer = RT_MIN(cbSubTransfer, X86_PAGE_SIZE - (uint32_t)(GCPhysDstPage & X86_PAGE_OFFSET_MASK));
            }
        }

        if (RT_SUCCESS(rc))
        {
            memcpy(pvDst, pvSrc, cbSubTransfer);
            cbTransfered += cbSubTransfer;
            cbTransfer   -= cbSubTransfer;
        }
        else
            cbTransfer = 0; /* force break below */

        if (fSrcLocked)
            PDMDevHlpPhysReleasePageMappingLock(pDevIns, &SrcLock);
        if (fDstLocked)
            PDMDevHlpPhysReleasePageMappingLock(pDevIns, &DstLock);
    } while (cbTransfer);

    if (RT_SUCCESS(rc))
        return sizeof(TransferSafeCopy);
    return rc;
}

/**
 * Worker for vboxVDMACommandProcess().
 *
 * @param   pVdma       Tthe VDMA channel.
 * @param   pbBuffer    Command buffer, considered volatile.
 * @param   cbBuffer    The number of bytes at @a pbBuffer.
 * @param   pCmdDr      The command.  For setting the async flag on chromium
 *                      requests.
 * @param   pfAsyncCmd  Flag to set if async command completion on chromium
 *                      requests.  Input stat is false, so it only ever need to
 *                      be set to true.
 */
static int vboxVDMACmdExec(PVBOXVDMAHOST pVdma, uint8_t const RT_UNTRUSTED_VOLATILE_GUEST *pbBuffer, uint32_t cbBuffer,
                           VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_GUEST *pCmdDr, bool *pfAsyncCmd)
{
    AssertReturn(pbBuffer, VERR_INVALID_POINTER);

    for (;;)
    {
        AssertReturn(cbBuffer >= VBOXVDMACMD_HEADER_SIZE(), VERR_INVALID_PARAMETER);

        VBOXVDMACMD const RT_UNTRUSTED_VOLATILE_GUEST  *pCmd       = (VBOXVDMACMD const RT_UNTRUSTED_VOLATILE_GUEST *)pbBuffer;
        VBOXVDMACMD_TYPE                                enmCmdType = pCmd->enmType;
        RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

        ASSERT_GUEST_MSG_RETURN(   enmCmdType == VBOXVDMACMD_TYPE_CHROMIUM_CMD
                                || enmCmdType == VBOXVDMACMD_TYPE_DMA_PRESENT_BLT
                                || enmCmdType == VBOXVDMACMD_TYPE_DMA_BPB_TRANSFER
                                || enmCmdType == VBOXVDMACMD_TYPE_DMA_NOP
                                || enmCmdType == VBOXVDMACMD_TYPE_CHILD_STATUS_IRQ,
                                ("enmCmdType=%d\n", enmCmdType),
                                VERR_INVALID_FUNCTION);
        RT_UNTRUSTED_VALIDATED_FENCE();

        int cbProcessed;
        switch (enmCmdType)
        {
            case VBOXVDMACMD_TYPE_CHROMIUM_CMD:
            {
                VBOXVDMACMD_CHROMIUM_CMD RT_UNTRUSTED_VOLATILE_GUEST *pCrCmd = VBOXVDMACMD_BODY(pCmd, VBOXVDMACMD_CHROMIUM_CMD);
                uint32_t const cbBody = VBOXVDMACMD_BODY_SIZE(cbBuffer);
                AssertReturn(cbBody >= sizeof(*pCrCmd), VERR_INVALID_PARAMETER);

                PVGASTATE pVGAState = pVdma->pVGAState;
                AssertReturn(pVGAState->pDrv->pfnCrHgsmiCommandProcess, VERR_NOT_SUPPORTED);

                VBoxSHGSMICommandMarkAsynchCompletion(pCmdDr);
                pVGAState->pDrv->pfnCrHgsmiCommandProcess(pVGAState->pDrv, pCrCmd, cbBody);
                *pfAsyncCmd = true;
                return VINF_SUCCESS;
            }

            case VBOXVDMACMD_TYPE_DMA_PRESENT_BLT:
            {
                VBOXVDMACMD_DMA_PRESENT_BLT RT_UNTRUSTED_VOLATILE_GUEST *pBlt = VBOXVDMACMD_BODY(pCmd, VBOXVDMACMD_DMA_PRESENT_BLT);
                cbProcessed = vboxVDMACmdExecBlt(pVdma, pBlt, cbBuffer - VBOXVDMACMD_HEADER_SIZE());
                Assert(cbProcessed >= 0);
                break;
            }

            case VBOXVDMACMD_TYPE_DMA_BPB_TRANSFER:
            {
                VBOXVDMACMD_DMA_BPB_TRANSFER RT_UNTRUSTED_VOLATILE_GUEST *pTransfer
                    = VBOXVDMACMD_BODY(pCmd, VBOXVDMACMD_DMA_BPB_TRANSFER);
                cbProcessed = vboxVDMACmdExecBpbTransfer(pVdma, pTransfer, cbBuffer - VBOXVDMACMD_HEADER_SIZE());
                Assert(cbProcessed >= 0);
                break;
            }

            case VBOXVDMACMD_TYPE_DMA_NOP:
                return VINF_SUCCESS;

            case VBOXVDMACMD_TYPE_CHILD_STATUS_IRQ:
                return VINF_SUCCESS;

            default:
                AssertFailedReturn(VERR_INVALID_FUNCTION);
        }

        /* Advance buffer or return. */
        if (cbProcessed >= 0)
        {
            Assert(cbProcessed > 0);
            cbProcessed += VBOXVDMACMD_HEADER_SIZE();
            if ((uint32_t)cbProcessed >= cbBuffer)
            {
                Assert((uint32_t)cbProcessed == cbBuffer);
                return VINF_SUCCESS;
            }

            cbBuffer -= cbProcessed;
            pbBuffer += cbProcessed;
        }
        else
        {
            RT_UNTRUSTED_VALIDATED_FENCE();
            return cbProcessed; /* error status */
        }
    }
}

/**
 * VDMA worker thread procedure, see vdmaVBVACtlEnableSubmitInternal().
 *
 * @thread VDMA
 */
static DECLCALLBACK(int) vboxVDMAWorkerThread(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF(hThreadSelf);
    PVBOXVDMAHOST       pVdma     = (PVBOXVDMAHOST)pvUser;
    PVGASTATE           pVGAState = pVdma->pVGAState;
    VBVAEXHOSTCONTEXT  *pCmdVbva  = &pVdma->CmdVbva;
    int rc;

    VBoxVDMAThreadNotifyConstructSucceeded(&pVdma->Thread, pvUser);

    while (!VBoxVDMAThreadIsTerminating(&pVdma->Thread))
    {
        uint8_t RT_UNTRUSTED_VOLATILE_GUEST *pbCmd = NULL;
        uint32_t                             cbCmd = 0;
        VBVAEXHOST_DATA_TYPE enmType = VBoxVBVAExHPDataGet(pCmdVbva, &pbCmd, &cbCmd);
        switch (enmType)
        {
            case VBVAEXHOST_DATA_TYPE_CMD:
                vboxVDMACrCmdProcess(pVdma, pbCmd, cbCmd);
                VBoxVBVAExHPDataCompleteCmd(pCmdVbva, cbCmd);
                VBVARaiseIrq(pVGAState, 0);
                break;

            case VBVAEXHOST_DATA_TYPE_GUESTCTL:
                rc = vboxVDMACrGuestCtlProcess(pVdma, (VBVAEXHOSTCTL *)pbCmd);
                VBoxVBVAExHPDataCompleteCtl(pCmdVbva, (VBVAEXHOSTCTL *)pbCmd, rc);
                break;

            case VBVAEXHOST_DATA_TYPE_HOSTCTL:
            {
                bool fContinue = true;
                rc = vboxVDMACrHostCtlProcess(pVdma, (VBVAEXHOSTCTL *)pbCmd, &fContinue);
                VBoxVBVAExHPDataCompleteCtl(pCmdVbva, (VBVAEXHOSTCTL *)pbCmd, rc);
                if (fContinue)
                    break;
            }
            RT_FALL_THRU();

            case VBVAEXHOST_DATA_TYPE_NO_DATA:
                rc = RTSemEventWaitNoResume(pVdma->Thread.hEvent, RT_INDEFINITE_WAIT);
                AssertMsg(RT_SUCCESS(rc) || rc == VERR_INTERRUPTED, ("%Rrc\n", rc));
                break;

            default:
                WARN(("unexpected type %d\n", enmType));
                break;
        }
    }

    VBoxVDMAThreadNotifyTerminatingSucceeded(&pVdma->Thread, pvUser);

    return VINF_SUCCESS;
}

/**
 * Worker for vboxVDMACommand.
 *
 * @returns VBox status code of the operation.
 * @param   pVdma       VDMA instance data.
 * @param   pCmd        The command to process.  Consider content volatile.
 * @param   cbCmd       Number of valid bytes at @a pCmd.  This is at least
 *                      sizeof(VBOXVDMACBUF_DR).
 * @param   pfAsyncCmd  Flag to set if async command completion on chromium
 *                      requests.  Input stat is false, so it only ever need to
 *                      be set to true.
 * @thread  EMT
 */
static int vboxVDMACommandProcess(PVBOXVDMAHOST pVdma, VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_GUEST *pCmd,
                                  uint32_t cbCmd, bool *pfAsyncCmd)
{
    /*
     * Get the command buffer (volatile).
     */
    uint16_t const cbCmdBuf                = pCmd->cbBuf;
    uint16_t const fCmdFlags               = pCmd->fFlags;
    uint64_t const offVramBuf_or_GCPhysBuf = pCmd->Location.offVramBuf;
    AssertCompile(sizeof(pCmd->Location.offVramBuf) == sizeof(pCmd->Location.phBuf));
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

    const uint8_t RT_UNTRUSTED_VOLATILE_GUEST  *pbCmdBuf;
    PGMPAGEMAPLOCK                              Lock;
    bool                                        fReleaseLocked = false;
    if (fCmdFlags & VBOXVDMACBUF_FLAG_BUF_FOLLOWS_DR)
    {
        pbCmdBuf = VBOXVDMACBUF_DR_TAIL(pCmd, const uint8_t);
        AssertReturn((uintptr_t)&pbCmdBuf[cbCmdBuf] <= (uintptr_t)&((uint8_t *)pCmd)[cbCmd],
                     VERR_INVALID_PARAMETER);
        RT_UNTRUSTED_VALIDATED_FENCE();
    }
    else if (fCmdFlags & VBOXVDMACBUF_FLAG_BUF_VRAM_OFFSET)
    {
        AssertReturn(   offVramBuf_or_GCPhysBuf <= pVdma->pVGAState->vram_size
                     && offVramBuf_or_GCPhysBuf + cbCmdBuf <= pVdma->pVGAState->vram_size,
                     VERR_INVALID_PARAMETER);
        RT_UNTRUSTED_VALIDATED_FENCE();

        pbCmdBuf = (uint8_t const RT_UNTRUSTED_VOLATILE_GUEST *)pVdma->pVGAState->vram_ptrR3 + offVramBuf_or_GCPhysBuf;
    }
    else
    {
        /* Make sure it doesn't cross a page. */
        AssertReturn((uint32_t)(offVramBuf_or_GCPhysBuf & X86_PAGE_OFFSET_MASK) + cbCmdBuf <= (uint32_t)X86_PAGE_SIZE,
                     VERR_INVALID_PARAMETER);
        RT_UNTRUSTED_VALIDATED_FENCE();

        int rc = PDMDevHlpPhysGCPhys2CCPtrReadOnly(pVdma->pVGAState->pDevInsR3, offVramBuf_or_GCPhysBuf, 0 /*fFlags*/,
                                                   (const void **)&pbCmdBuf, &Lock);
        AssertRCReturn(rc, rc); /* if (rc == VERR_PGM_PHYS_PAGE_RESERVED) -> fall back on using PGMPhysRead ?? */
        fReleaseLocked = true;
    }

    /*
     * Process the command.
     */
    int rc = vboxVDMACmdExec(pVdma, pbCmdBuf, cbCmdBuf, pCmd, pfAsyncCmd);
    AssertRC(rc);

    /* Clean up comand buffer. */
    if (fReleaseLocked)
        PDMDevHlpPhysReleasePageMappingLock(pVdma->pVGAState->pDevInsR3, &Lock);
    return rc;
}

# if 0 /** @todo vboxVDMAControlProcess is unused */
static void vboxVDMAControlProcess(PVBOXVDMAHOST pVdma, PVBOXVDMA_CTL pCmd)
{
    PHGSMIINSTANCE pHgsmi = pVdma->pHgsmi;
    pCmd->i32Result = VINF_SUCCESS;
    int rc = VBoxSHGSMICommandComplete (pHgsmi, pCmd);
    AssertRC(rc);
}
# endif

#endif /* VBOX_WITH_CRHGSMI */
#ifdef VBOX_VDMA_WITH_WATCHDOG

/**
 * @callback_method_impl{TMTIMER, VDMA watchdog timer.}
 */
static DECLCALLBACK(void) vboxVDMAWatchDogTimer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    VBOXVDMAHOST *pVdma = (VBOXVDMAHOST *)pvUser;
    PVGASTATE pVGAState = pVdma->pVGAState;
    VBVARaiseIrq(pVGAState, HGSMIHOSTFLAGS_WATCHDOG);
}

/**
 * Handles VBOXVDMA_CTL_TYPE_WATCHDOG for vboxVDMAControl.
 */
static int vboxVDMAWatchDogCtl(struct VBOXVDMAHOST *pVdma, uint32_t cMillis)
{
    PPDMDEVINS pDevIns = pVdma->pVGAState->pDevInsR3;
    if (cMillis)
        TMTimerSetMillies(pVdma->WatchDogTimer, cMillis);
    else
        TMTimerStop(pVdma->WatchDogTimer);
    return VINF_SUCCESS;
}

#endif /* VBOX_VDMA_WITH_WATCHDOG */

/**
 * Called by vgaR3Construct() to initialize the state.
 *
 * @returns VBox status code.
 */
int vboxVDMAConstruct(PVGASTATE pVGAState, uint32_t cPipeElements)
{
    RT_NOREF(cPipeElements);
    int rc;
    PVBOXVDMAHOST pVdma = (PVBOXVDMAHOST)RTMemAllocZ(sizeof(*pVdma));
    Assert(pVdma);
    if (pVdma)
    {
        pVdma->pHgsmi    = pVGAState->pHGSMI;
        pVdma->pVGAState = pVGAState;

#ifdef VBOX_VDMA_WITH_WATCHDOG
        rc = PDMDevHlpTMTimerCreate(pVGAState->pDevInsR3, TMCLOCK_REAL, vboxVDMAWatchDogTimer,
                                    pVdma, TMTIMER_FLAGS_NO_CRIT_SECT,
                                    "VDMA WatchDog Timer", &pVdma->WatchDogTimer);
        AssertRC(rc);
#else
        rc = VINF_SUCCESS;
#endif
        if (RT_SUCCESS(rc))
        {
#ifdef VBOX_WITH_CRHGSMI
            VBoxVDMAThreadInit(&pVdma->Thread);

            rc = RTSemEventMultiCreate(&pVdma->HostCrCtlCompleteEvent);
            if (RT_SUCCESS(rc))
            {
                rc = VBoxVBVAExHSInit(&pVdma->CmdVbva);
                if (RT_SUCCESS(rc))
                {
                    rc = RTCritSectInit(&pVdma->CalloutCritSect);
                    if (RT_SUCCESS(rc))
                    {
#endif
                        pVGAState->pVdma = pVdma;

#ifdef VBOX_WITH_CRHGSMI
                        /* No HGCM service if VMSVGA is enabled. */
                        if (!pVGAState->fVMSVGAEnabled)
                        {
                            int rcIgnored = vboxVDMACrCtlHgsmiSetup(pVdma); NOREF(rcIgnored); /** @todo is this ignoring intentional? */
                        }
#endif
                        return VINF_SUCCESS;

#ifdef VBOX_WITH_CRHGSMI
                    }

                    WARN(("RTCritSectInit failed %Rrc\n", rc));
                    VBoxVBVAExHSTerm(&pVdma->CmdVbva);
                }
                else
                    WARN(("VBoxVBVAExHSInit failed %Rrc\n", rc));
                RTSemEventMultiDestroy(pVdma->HostCrCtlCompleteEvent);
            }
            else
                WARN(("RTSemEventMultiCreate failed %Rrc\n", rc));
#endif
            /* the timer is cleaned up automatically */
        }
        RTMemFree(pVdma);
    }
    else
        rc = VERR_OUT_OF_RESOURCES;
    return rc;
}

/**
 * Called by vgaR3Reset() to do reset.
 */
void  vboxVDMAReset(struct VBOXVDMAHOST *pVdma)
{
#ifdef VBOX_WITH_CRHGSMI
    vdmaVBVACtlDisableSync(pVdma);
#else
    RT_NOREF(pVdma);
#endif
}

/**
 * Called by vgaR3Destruct() to do cleanup.
 */
void vboxVDMADestruct(struct VBOXVDMAHOST *pVdma)
{
    if (!pVdma)
        return;
#ifdef VBOX_WITH_CRHGSMI
    if (pVdma->pVGAState->fVMSVGAEnabled)
        VBoxVBVAExHSDisable(&pVdma->CmdVbva);
    else
    {
        /** @todo Remove. It does nothing because pVdma->CmdVbva is already disabled at this point
         *        as the result of the SharedOpenGL HGCM service unloading.
         */
        vdmaVBVACtlDisableSync(pVdma);
    }
    VBoxVDMAThreadCleanup(&pVdma->Thread);
    VBoxVBVAExHSTerm(&pVdma->CmdVbva);
    RTSemEventMultiDestroy(pVdma->HostCrCtlCompleteEvent);
    RTCritSectDelete(&pVdma->CalloutCritSect);
#endif
    RTMemFree(pVdma);
}

/**
 * Handle VBVA_VDMA_CTL, see vbvaChannelHandler
 *
 * @param   pVdma   The VDMA channel.
 * @param   pCmd    The control command to handle.  Considered volatile.
 * @param   cbCmd   The size of the command.  At least sizeof(VBOXVDMA_CTL).
 */
void vboxVDMAControl(struct VBOXVDMAHOST *pVdma, VBOXVDMA_CTL RT_UNTRUSTED_VOLATILE_GUEST *pCmd, uint32_t cbCmd)
{
    RT_NOREF(cbCmd);
    PHGSMIINSTANCE pIns = pVdma->pHgsmi;

    VBOXVDMA_CTL_TYPE enmCtl = pCmd->enmCtl;
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

    int rc;
    if (enmCtl < VBOXVDMA_CTL_TYPE_END)
    {
        RT_UNTRUSTED_VALIDATED_FENCE();

        switch (enmCtl)
        {
            case VBOXVDMA_CTL_TYPE_ENABLE:
                rc = VINF_SUCCESS;
                break;
            case VBOXVDMA_CTL_TYPE_DISABLE:
                rc = VINF_SUCCESS;
                break;
            case VBOXVDMA_CTL_TYPE_FLUSH:
                rc = VINF_SUCCESS;
                break;
            case VBOXVDMA_CTL_TYPE_WATCHDOG:
#ifdef VBOX_VDMA_WITH_WATCHDOG
                rc = vboxVDMAWatchDogCtl(pVdma, pCmd->u32Offset);
#else
                rc = VERR_NOT_SUPPORTED;
#endif
                break;
            default:
                AssertFailedBreakStmt(rc = VERR_IPE_NOT_REACHED_DEFAULT_CASE);
        }
    }
    else
    {
        RT_UNTRUSTED_VALIDATED_FENCE();
        ASSERT_GUEST_FAILED();
        rc = VERR_NOT_SUPPORTED;
    }

    pCmd->i32Result = rc;
    rc = VBoxSHGSMICommandComplete(pIns, pCmd);
    AssertRC(rc);
}

/**
 * Handle VBVA_VDMA_CMD, see vbvaChannelHandler().
 *
 * @param   pVdma   The VDMA channel.
 * @param   pCmd    The command to handle.  Considered volatile.
 * @param   cbCmd   The size of the command.  At least sizeof(VBOXVDMACBUF_DR).
 * @thread  EMT
 */
void vboxVDMACommand(struct VBOXVDMAHOST *pVdma, VBOXVDMACBUF_DR RT_UNTRUSTED_VOLATILE_GUEST *pCmd, uint32_t cbCmd)
{
    /*
     * Process the command.
     */
    bool fAsyncCmd = false;
#ifdef VBOX_WITH_CRHGSMI
    int rc = vboxVDMACommandProcess(pVdma, pCmd, cbCmd, &fAsyncCmd);
#else
    RT_NOREF(cbCmd);
    int rc = VERR_NOT_IMPLEMENTED;
#endif

    /*
     * Complete the command unless it's asynchronous (e.g. chromium).
     */
    if (!fAsyncCmd)
    {
        pCmd->rc = rc;
        int rc2 = VBoxSHGSMICommandComplete(pVdma->pHgsmi, pCmd);
        AssertRC(rc2);
    }
}

#ifdef VBOX_WITH_CRHGSMI

/**
 * @callback_method_impl{FNVBVAEXHOSTCTL_COMPLETE,
 *      Used by vdmaVBVACtlEnableDisableSubmit() and vdmaVBVACtlEnableDisableSubmit() }
 */
static DECLCALLBACK(void) vboxCmdVBVACmdCtlGuestCompletion(VBVAEXHOSTCONTEXT *pVbva, struct VBVAEXHOSTCTL *pCtl,
                                                           int rc, void *pvContext)
{
    PVBOXVDMAHOST pVdma = (PVBOXVDMAHOST)pvContext;
    VBOXCMDVBVA_CTL RT_UNTRUSTED_VOLATILE_GUEST *pGCtl
        = (VBOXCMDVBVA_CTL RT_UNTRUSTED_VOLATILE_GUEST *)((uintptr_t)pCtl->u.cmd.pvCmd - sizeof(VBOXCMDVBVA_CTL));
    AssertRC(rc);
    pGCtl->i32Result = rc;

    Assert(pVdma->pVGAState->fGuestCaps & VBVACAPS_COMPLETEGCMD_BY_IOREAD);
    rc = VBoxSHGSMICommandComplete(pVdma->pHgsmi, pGCtl);
    AssertRC(rc);

    VBoxVBVAExHCtlFree(pVbva, pCtl);
}

/**
 * Worker for vdmaVBVACtlGenericGuestSubmit() and vdmaVBVACtlOpaqueHostSubmit().
 */
static int vdmaVBVACtlGenericSubmit(PVBOXVDMAHOST pVdma, VBVAEXHOSTCTL_SOURCE enmSource, VBVAEXHOSTCTL_TYPE enmType,
                                    uint8_t RT_UNTRUSTED_VOLATILE_GUEST *pbCmd, uint32_t cbCmd,
                                    PFNVBVAEXHOSTCTL_COMPLETE pfnComplete, void *pvComplete)
{
    int            rc;
    VBVAEXHOSTCTL *pHCtl = VBoxVBVAExHCtlCreate(&pVdma->CmdVbva, enmType);
    if (pHCtl)
    {
        pHCtl->u.cmd.pvCmd = pbCmd;
        pHCtl->u.cmd.cbCmd = cbCmd;
        rc = vdmaVBVACtlSubmit(pVdma, pHCtl, enmSource, pfnComplete, pvComplete);
        if (RT_SUCCESS(rc))
            return VINF_SUCCESS;

        VBoxVBVAExHCtlFree(&pVdma->CmdVbva, pHCtl);
        Log(("vdmaVBVACtlSubmit failed %Rrc\n", rc));
    }
    else
    {
        WARN(("VBoxVBVAExHCtlCreate failed\n"));
        rc = VERR_NO_MEMORY;
    }
    return rc;
}

/**
 * Handler for vboxCmdVBVACmdCtl()/VBOXCMDVBVACTL_TYPE_3DCTL.
 */
static int vdmaVBVACtlGenericGuestSubmit(PVBOXVDMAHOST pVdma, VBVAEXHOSTCTL_TYPE enmType,
                                         VBOXCMDVBVA_CTL RT_UNTRUSTED_VOLATILE_GUEST *pCtl, uint32_t cbCtl)
{
    Assert(cbCtl >= sizeof(VBOXCMDVBVA_CTL)); /* Checked by callers caller, vbvaChannelHandler(). */

    VBoxSHGSMICommandMarkAsynchCompletion(pCtl);
    int rc = vdmaVBVACtlGenericSubmit(pVdma, VBVAEXHOSTCTL_SOURCE_GUEST, enmType,
                                      (uint8_t RT_UNTRUSTED_VOLATILE_GUEST *)(pCtl + 1),
                                      cbCtl - sizeof(VBOXCMDVBVA_CTL),
                                      vboxCmdVBVACmdCtlGuestCompletion, pVdma);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    WARN(("vdmaVBVACtlGenericSubmit failed %Rrc\n", rc));
    pCtl->i32Result = rc;
    rc = VBoxSHGSMICommandComplete(pVdma->pHgsmi, pCtl);
    AssertRC(rc);
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNVBVAEXHOSTCTL_COMPLETE, Used by vdmaVBVACtlOpaqueHostSubmit()}
 */
static DECLCALLBACK(void) vboxCmdVBVACmdCtlHostCompletion(VBVAEXHOSTCONTEXT *pVbva, struct VBVAEXHOSTCTL *pCtl,
                                                          int rc, void *pvCompletion)
{
    VBOXCRCMDCTL *pVboxCtl = (VBOXCRCMDCTL *)pCtl->u.cmd.pvCmd;
    if (pVboxCtl->u.pfnInternal)
        ((PFNCRCTLCOMPLETION)pVboxCtl->u.pfnInternal)(pVboxCtl, pCtl->u.cmd.cbCmd, rc, pvCompletion);
    VBoxVBVAExHCtlFree(pVbva, pCtl);
}

/**
 * Worker for vboxCmdVBVACmdHostCtl() and vboxCmdVBVACmdHostCtlSync().
 */
static int vdmaVBVACtlOpaqueHostSubmit(PVBOXVDMAHOST pVdma, struct VBOXCRCMDCTL* pCmd, uint32_t cbCmd,
                                       PFNCRCTLCOMPLETION pfnCompletion, void *pvCompletion)
{
    pCmd->u.pfnInternal = (PFNRT)pfnCompletion;
    int rc = vdmaVBVACtlGenericSubmit(pVdma, VBVAEXHOSTCTL_SOURCE_HOST, VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE,
                                      (uint8_t *)pCmd, cbCmd, vboxCmdVBVACmdCtlHostCompletion, pvCompletion);
    if (RT_FAILURE(rc))
    {
        if (rc == VERR_INVALID_STATE)
        {
            pCmd->u.pfnInternal = NULL;
            PVGASTATE pVGAState = pVdma->pVGAState;
            rc = pVGAState->pDrv->pfnCrHgcmCtlSubmit(pVGAState->pDrv, pCmd, cbCmd, pfnCompletion, pvCompletion);
            if (!RT_SUCCESS(rc))
                WARN(("pfnCrHgsmiControlProcess failed %Rrc\n", rc));

            return rc;
        }
        WARN(("vdmaVBVACtlGenericSubmit failed %Rrc\n", rc));
        return rc;
    }

    return VINF_SUCCESS;
}

/**
 * Called from vdmaVBVACtlThreadCreatedEnable().
 */
static int vdmaVBVANotifyEnable(PVGASTATE pVGAState)
{
    for (uint32_t i = 0; i < pVGAState->cMonitors; i++)
    {
        int rc = pVGAState->pDrv->pfnVBVAEnable (pVGAState->pDrv, i, NULL, true);
        if (!RT_SUCCESS(rc))
        {
            WARN(("pfnVBVAEnable failed %Rrc\n", rc));
            for (uint32_t j = 0; j < i; j++)
            {
                pVGAState->pDrv->pfnVBVADisable (pVGAState->pDrv, j);
            }

            return rc;
        }
    }
    return VINF_SUCCESS;
}

/**
 * Called from vdmaVBVACtlThreadCreatedEnable() and vdmaVBVADisableProcess().
 */
static int vdmaVBVANotifyDisable(PVGASTATE pVGAState)
{
    for (uint32_t i = 0; i < pVGAState->cMonitors; i++)
        pVGAState->pDrv->pfnVBVADisable(pVGAState->pDrv, i);
    return VINF_SUCCESS;
}

/**
 * Hook that is called by vboxVDMAWorkerThread when it starts.
 *
 * @thread VDMA
 */
static DECLCALLBACK(void) vdmaVBVACtlThreadCreatedEnable(struct VBOXVDMATHREAD *pThread, int rc,
                                                         void *pvThreadContext, void *pvContext)
{
    RT_NOREF(pThread);
    PVBOXVDMAHOST pVdma = (PVBOXVDMAHOST)pvThreadContext;
    VBVAEXHOSTCTL* pHCtl = (VBVAEXHOSTCTL*)pvContext;

    if (RT_SUCCESS(rc))
    {
        rc = vboxVDMACrGuestCtlProcess(pVdma, pHCtl);
        /* rc == VINF_SUCCESS would mean the actual state change has occcured */
        if (rc == VINF_SUCCESS)
        {
            /* we need to inform Main about VBVA enable/disable
             * main expects notifications to be done from the main thread
             * submit it there */
            PVGASTATE pVGAState = pVdma->pVGAState;

            if (VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva))
                vdmaVBVANotifyEnable(pVGAState);
            else
                vdmaVBVANotifyDisable(pVGAState);
        }
        else if (RT_FAILURE(rc))
            WARN(("vboxVDMACrGuestCtlProcess failed %Rrc\n", rc));
    }
    else
        WARN(("vdmaVBVACtlThreadCreatedEnable is passed %Rrc\n", rc));

    VBoxVBVAExHPDataCompleteCtl(&pVdma->CmdVbva, pHCtl, rc);
}

/**
 * Worker for vdmaVBVACtlEnableDisableSubmitInternal() and vdmaVBVACtlEnableSubmitSync().
 */
static int vdmaVBVACtlEnableSubmitInternal(PVBOXVDMAHOST pVdma, VBVAENABLE RT_UNTRUSTED_VOLATILE_GUEST *pEnable, bool fPaused,
                                           PFNVBVAEXHOSTCTL_COMPLETE pfnComplete, void *pvComplete)
{
    int rc;
    VBVAEXHOSTCTL *pHCtl = VBoxVBVAExHCtlCreate(&pVdma->CmdVbva,
                                                fPaused ? VBVAEXHOSTCTL_TYPE_GHH_ENABLE_PAUSED : VBVAEXHOSTCTL_TYPE_GHH_ENABLE);
    if (pHCtl)
    {
        pHCtl->u.cmd.pvCmd  = pEnable;
        pHCtl->u.cmd.cbCmd  = sizeof(*pEnable);
        pHCtl->pfnComplete  = pfnComplete;
        pHCtl->pvComplete   = pvComplete;

        rc = VBoxVDMAThreadCreate(&pVdma->Thread, vboxVDMAWorkerThread, pVdma, vdmaVBVACtlThreadCreatedEnable, pHCtl);
        if (RT_SUCCESS(rc))
            return VINF_SUCCESS;

        WARN(("VBoxVDMAThreadCreate failed %Rrc\n", rc));
        VBoxVBVAExHCtlFree(&pVdma->CmdVbva, pHCtl);
    }
    else
    {
        WARN(("VBoxVBVAExHCtlCreate failed\n"));
        rc = VERR_NO_MEMORY;
    }

    return rc;
}

/**
 * Worker for vboxVDMASaveLoadExecPerform().
 */
static int vdmaVBVACtlEnableSubmitSync(PVBOXVDMAHOST pVdma, uint32_t offVram, bool fPaused)
{
    VBVAENABLE Enable = {0};
    Enable.u32Flags = VBVA_F_ENABLE;
    Enable.u32Offset = offVram;

    VDMA_VBVA_CTL_CYNC_COMPLETION Data;
    Data.rc = VERR_NOT_IMPLEMENTED;
    int rc = RTSemEventCreate(&Data.hEvent);
    if (!RT_SUCCESS(rc))
    {
        WARN(("RTSemEventCreate failed %Rrc\n", rc));
        return rc;
    }

    rc = vdmaVBVACtlEnableSubmitInternal(pVdma, &Enable, fPaused, vdmaVBVACtlSubmitSyncCompletion, &Data);
    if (RT_SUCCESS(rc))
    {
        rc = RTSemEventWait(Data.hEvent, RT_INDEFINITE_WAIT);
        if (RT_SUCCESS(rc))
        {
            rc = Data.rc;
            if (!RT_SUCCESS(rc))
                WARN(("vdmaVBVACtlSubmitSyncCompletion returned %Rrc\n", rc));
        }
        else
            WARN(("RTSemEventWait failed %Rrc\n", rc));
    }
    else
        WARN(("vdmaVBVACtlSubmit failed %Rrc\n", rc));

    RTSemEventDestroy(Data.hEvent);

    return rc;
}

/**
 * Worker for vdmaVBVACtlEnableDisableSubmitInternal().
 */
static int vdmaVBVACtlDisableSubmitInternal(PVBOXVDMAHOST pVdma, VBVAENABLE RT_UNTRUSTED_VOLATILE_GUEST *pEnable,
                                            PFNVBVAEXHOSTCTL_COMPLETE pfnComplete, void *pvComplete)
{
    int rc;
    if (VBoxVBVAExHSIsDisabled(&pVdma->CmdVbva))
    {
        WARN(("VBoxVBVAExHSIsDisabled: disabled"));
        return VINF_SUCCESS;
    }

    VBVAEXHOSTCTL *pHCtl = VBoxVBVAExHCtlCreate(&pVdma->CmdVbva, VBVAEXHOSTCTL_TYPE_GHH_DISABLE);
    if (!pHCtl)
    {
        WARN(("VBoxVBVAExHCtlCreate failed\n"));
        return VERR_NO_MEMORY;
    }

    pHCtl->u.cmd.pvCmd = pEnable;
    pHCtl->u.cmd.cbCmd = sizeof(*pEnable);
    rc = vdmaVBVACtlSubmit(pVdma, pHCtl, VBVAEXHOSTCTL_SOURCE_GUEST, pfnComplete, pvComplete);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    WARN(("vdmaVBVACtlSubmit failed rc %Rrc\n", rc));
    VBoxVBVAExHCtlFree(&pVdma->CmdVbva, pHCtl);
    return rc;
}

/**
 * Worker for vdmaVBVACtlEnableDisableSubmit().
 */
static int vdmaVBVACtlEnableDisableSubmitInternal(PVBOXVDMAHOST pVdma, VBVAENABLE RT_UNTRUSTED_VOLATILE_GUEST *pEnable,
                                                  PFNVBVAEXHOSTCTL_COMPLETE pfnComplete, void *pvComplete)
{
    bool fEnable = (pEnable->u32Flags & (VBVA_F_ENABLE | VBVA_F_DISABLE)) == VBVA_F_ENABLE;
    if (fEnable)
        return vdmaVBVACtlEnableSubmitInternal(pVdma, pEnable, false, pfnComplete, pvComplete);
    return vdmaVBVACtlDisableSubmitInternal(pVdma, pEnable, pfnComplete, pvComplete);
}

/**
 * Handler for vboxCmdVBVACmdCtl/VBOXCMDVBVACTL_TYPE_ENABLE.
 */
static int vdmaVBVACtlEnableDisableSubmit(PVBOXVDMAHOST pVdma, VBOXCMDVBVA_CTL_ENABLE RT_UNTRUSTED_VOLATILE_GUEST *pEnable)
{
    VBoxSHGSMICommandMarkAsynchCompletion(&pEnable->Hdr);
    int rc = vdmaVBVACtlEnableDisableSubmitInternal(pVdma, &pEnable->Enable, vboxCmdVBVACmdCtlGuestCompletion, pVdma);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    WARN(("vdmaVBVACtlEnableDisableSubmitInternal failed %Rrc\n", rc));
    pEnable->Hdr.i32Result = rc;
    rc = VBoxSHGSMICommandComplete(pVdma->pHgsmi, &pEnable->Hdr);
    AssertRC(rc);
    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNVBVAEXHOSTCTL_COMPLETE,
 *      Used by vdmaVBVACtlSubmitSync() and vdmaVBVACtlEnableSubmitSync().}
 */
static DECLCALLBACK(void) vdmaVBVACtlSubmitSyncCompletion(VBVAEXHOSTCONTEXT *pVbva, struct VBVAEXHOSTCTL *pCtl,
                                                          int rc, void *pvContext)
{
    RT_NOREF(pVbva, pCtl);
    VDMA_VBVA_CTL_CYNC_COMPLETION *pData = (VDMA_VBVA_CTL_CYNC_COMPLETION *)pvContext;
    pData->rc = rc;
    rc = RTSemEventSignal(pData->hEvent);
    if (!RT_SUCCESS(rc))
        WARN(("RTSemEventSignal failed %Rrc\n", rc));
}


/**
 *
 */
static int vdmaVBVACtlSubmitSync(PVBOXVDMAHOST pVdma, VBVAEXHOSTCTL *pCtl, VBVAEXHOSTCTL_SOURCE enmSource)
{
    VDMA_VBVA_CTL_CYNC_COMPLETION Data;
    Data.rc     = VERR_NOT_IMPLEMENTED;
    Data.hEvent = NIL_RTSEMEVENT;
    int rc = RTSemEventCreate(&Data.hEvent);
    if (RT_SUCCESS(rc))
    {
        rc = vdmaVBVACtlSubmit(pVdma, pCtl, enmSource, vdmaVBVACtlSubmitSyncCompletion, &Data);
        if (RT_SUCCESS(rc))
        {
            rc = RTSemEventWait(Data.hEvent, RT_INDEFINITE_WAIT);
            if (RT_SUCCESS(rc))
            {
                rc = Data.rc;
                if (!RT_SUCCESS(rc))
                    WARN(("vdmaVBVACtlSubmitSyncCompletion returned %Rrc\n", rc));
            }
            else
                WARN(("RTSemEventWait failed %Rrc\n", rc));
        }
        else
            Log(("vdmaVBVACtlSubmit failed %Rrc\n", rc));

        RTSemEventDestroy(Data.hEvent);
    }
    else
        WARN(("RTSemEventCreate failed %Rrc\n", rc));
    return rc;
}

/**
 * Worker for vboxVDMASaveStateExecPrep().
 */
static int vdmaVBVAPause(PVBOXVDMAHOST pVdma)
{
    VBVAEXHOSTCTL Ctl;
    Ctl.enmType = VBVAEXHOSTCTL_TYPE_HH_INTERNAL_PAUSE;
    return vdmaVBVACtlSubmitSync(pVdma, &Ctl, VBVAEXHOSTCTL_SOURCE_HOST);
}

/**
 * Worker for vboxVDMASaveLoadExecPerform() and vboxVDMASaveStateExecDone().
 */
static int vdmaVBVAResume(PVBOXVDMAHOST pVdma)
{
    VBVAEXHOSTCTL Ctl;
    Ctl.enmType = VBVAEXHOSTCTL_TYPE_HH_INTERNAL_RESUME;
    return vdmaVBVACtlSubmitSync(pVdma, &Ctl, VBVAEXHOSTCTL_SOURCE_HOST);
}

/**
 * Worker for vboxCmdVBVACmdSubmit(), vboxCmdVBVACmdFlush() and vboxCmdVBVATimerRefresh().
 */
static int vboxVDMACmdSubmitPerform(struct VBOXVDMAHOST *pVdma)
{
    int rc = VBoxVBVAExHSCheckCommands(&pVdma->CmdVbva);
    switch (rc)
    {
        case VINF_SUCCESS:
            return VBoxVDMAThreadEventNotify(&pVdma->Thread);
        case VINF_ALREADY_INITIALIZED:
        case VINF_EOF:
        case VERR_INVALID_STATE:
            return VINF_SUCCESS;
        default:
            Assert(!RT_FAILURE(rc));
            return RT_FAILURE(rc) ? rc : VERR_INTERNAL_ERROR;
    }
}


/**
 * @interface_method_impl{PDMIDISPLAYVBVACALLBACKS,pfnCrCtlSubmit}
 */
int vboxCmdVBVACmdHostCtl(PPDMIDISPLAYVBVACALLBACKS pInterface,
                          struct VBOXCRCMDCTL *pCmd,
                          uint32_t cbCmd,
                          PFNCRCTLCOMPLETION pfnCompletion,
                          void *pvCompletion)
{
    PVGASTATE pVGAState = PPDMIDISPLAYVBVACALLBACKS_2_PVGASTATE(pInterface);
    struct VBOXVDMAHOST *pVdma = pVGAState->pVdma;
    if (pVdma == NULL)
        return VERR_INVALID_STATE;
    pCmd->CalloutList.List.pNext = NULL;
    return vdmaVBVACtlOpaqueHostSubmit(pVdma, pCmd, cbCmd, pfnCompletion, pvCompletion);
}

/**
 * Argument package from vboxCmdVBVACmdHostCtlSync to vboxCmdVBVACmdHostCtlSyncCb.
 */
typedef struct VBOXCMDVBVA_CMDHOSTCTL_SYNC
{
    struct VBOXVDMAHOST *pVdma;
    uint32_t fProcessing;
    int rc;
} VBOXCMDVBVA_CMDHOSTCTL_SYNC;

/**
 * @interface_method_impl{FNCRCTLCOMPLETION, Used by vboxCmdVBVACmdHostCtlSync.}
 */
static DECLCALLBACK(void) vboxCmdVBVACmdHostCtlSyncCb(struct VBOXCRCMDCTL *pCmd, uint32_t cbCmd, int rc, void *pvCompletion)
{
    RT_NOREF(pCmd, cbCmd);
    VBOXCMDVBVA_CMDHOSTCTL_SYNC *pData = (VBOXCMDVBVA_CMDHOSTCTL_SYNC *)pvCompletion;

    pData->rc = rc;

    struct VBOXVDMAHOST *pVdma = pData->pVdma;

    ASMAtomicIncS32(&pVdma->i32cHostCrCtlCompleted);

    pData->fProcessing = 0;

    RTSemEventMultiSignal(pVdma->HostCrCtlCompleteEvent);
}

/**
 * @callback_method_impl{FNVBOXCRCLIENT_CALLOUT, Worker for vboxVDMACrCtlHgsmiSetup }
 *
 * @note r=bird: not to be confused with the callout function below. sigh.
 */
static DECLCALLBACK(int) vboxCmdVBVACmdCallout(struct VBOXVDMAHOST *pVdma, struct VBOXCRCMDCTL* pCmd,
                                               VBOXCRCMDCTL_CALLOUT_LISTENTRY *pEntry, PFNVBOXCRCMDCTL_CALLOUT_CB pfnCb)
{
    pEntry->pfnCb = pfnCb;
    int rc = RTCritSectEnter(&pVdma->CalloutCritSect);
    if (RT_SUCCESS(rc))
    {
        RTListAppend(&pCmd->CalloutList.List, &pEntry->Node);
        RTCritSectLeave(&pVdma->CalloutCritSect);

        RTSemEventMultiSignal(pVdma->HostCrCtlCompleteEvent);
    }
    else
        WARN(("RTCritSectEnter failed %Rrc\n", rc));

    return rc;
}


/**
 * Worker for vboxCmdVBVACmdHostCtlSync.
 */
static int vboxCmdVBVACmdCalloutProcess(struct VBOXVDMAHOST *pVdma, struct VBOXCRCMDCTL* pCmd)
{
    int rc = VINF_SUCCESS;
    for (;;)
    {
        rc = RTCritSectEnter(&pVdma->CalloutCritSect);
        if (RT_SUCCESS(rc))
        {
            VBOXCRCMDCTL_CALLOUT_LISTENTRY* pEntry = RTListGetFirst(&pCmd->CalloutList.List, VBOXCRCMDCTL_CALLOUT_LISTENTRY, Node);
            if (pEntry)
                RTListNodeRemove(&pEntry->Node);
            RTCritSectLeave(&pVdma->CalloutCritSect);

            if (!pEntry)
                break;

            pEntry->pfnCb(pEntry);
        }
        else
        {
            WARN(("RTCritSectEnter failed %Rrc\n", rc));
            break;
        }
    }

    return rc;
}

/**
 * @interface_method_impl{PDMIDISPLAYVBVACALLBACKS,pfnCrCtlSubmitSync}
 */
DECLCALLBACK(int) vboxCmdVBVACmdHostCtlSync(PPDMIDISPLAYVBVACALLBACKS pInterface, struct VBOXCRCMDCTL *pCmd, uint32_t cbCmd)
{
    PVGASTATE               pVGAState = PPDMIDISPLAYVBVACALLBACKS_2_PVGASTATE(pInterface);
    struct VBOXVDMAHOST    *pVdma     = pVGAState->pVdma;
    if (pVdma == NULL)
        return VERR_INVALID_STATE;

    VBOXCMDVBVA_CMDHOSTCTL_SYNC Data;
    Data.pVdma = pVdma;
    Data.fProcessing = 1;
    Data.rc = VERR_INTERNAL_ERROR;
    RTListInit(&pCmd->CalloutList.List);
    int rc = vdmaVBVACtlOpaqueHostSubmit(pVdma, pCmd, cbCmd, vboxCmdVBVACmdHostCtlSyncCb, &Data);
    if (!RT_SUCCESS(rc))
    {
        WARN(("vdmaVBVACtlOpaqueHostSubmit failed %Rrc", rc));
        return rc;
    }

    while (Data.fProcessing)
    {
        /* Poll infrequently to make sure no completed message has been missed. */
        RTSemEventMultiWait(pVdma->HostCrCtlCompleteEvent, 500);

        vboxCmdVBVACmdCalloutProcess(pVdma, pCmd);

        if (Data.fProcessing)
            RTThreadYield();
    }

    /* extra check callouts */
    vboxCmdVBVACmdCalloutProcess(pVdma, pCmd);

    /* 'Our' message has been processed, so should reset the semaphore.
     * There is still possible that another message has been processed
     * and the semaphore has been signalled again.
     * Reset only if there are no other messages completed.
     */
    int32_t c = ASMAtomicDecS32(&pVdma->i32cHostCrCtlCompleted);
    Assert(c >= 0);
    if (!c)
        RTSemEventMultiReset(pVdma->HostCrCtlCompleteEvent);

    rc = Data.rc;
    if (!RT_SUCCESS(rc))
        WARN(("host call failed %Rrc", rc));

    return rc;
}

/**
 * Handler for VBVA_CMDVBVA_CTL, see vbvaChannelHandler().
 *
 * @returns VBox status code
 * @param   pVGAState           The VGA state.
 * @param   pCtl                The control command.
 * @param   cbCtl               The size of it.  This is at least
 *                              sizeof(VBOXCMDVBVA_CTL).
 * @thread  EMT
 */
int vboxCmdVBVACmdCtl(PVGASTATE pVGAState, VBOXCMDVBVA_CTL RT_UNTRUSTED_VOLATILE_GUEST *pCtl, uint32_t cbCtl)
{
    struct VBOXVDMAHOST *pVdma = pVGAState->pVdma;
    uint32_t uType = pCtl->u32Type;
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

    if (   uType == VBOXCMDVBVACTL_TYPE_3DCTL
        || uType == VBOXCMDVBVACTL_TYPE_RESIZE
        || uType == VBOXCMDVBVACTL_TYPE_ENABLE)
    {
        RT_UNTRUSTED_VALIDATED_FENCE();

        switch (uType)
        {
            case VBOXCMDVBVACTL_TYPE_3DCTL:
                return vdmaVBVACtlGenericGuestSubmit(pVdma, VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE, pCtl, cbCtl);

            case VBOXCMDVBVACTL_TYPE_RESIZE:
                return vdmaVBVACtlGenericGuestSubmit(pVdma, VBVAEXHOSTCTL_TYPE_GHH_RESIZE, pCtl, cbCtl);

            case VBOXCMDVBVACTL_TYPE_ENABLE:
                ASSERT_GUEST_BREAK(cbCtl == sizeof(VBOXCMDVBVA_CTL_ENABLE));
                return vdmaVBVACtlEnableDisableSubmit(pVdma, (VBOXCMDVBVA_CTL_ENABLE RT_UNTRUSTED_VOLATILE_GUEST *)pCtl);

            default:
                AssertFailed();
        }
    }

    pCtl->i32Result = VERR_INVALID_PARAMETER;
    int rc = VBoxSHGSMICommandComplete(pVdma->pHgsmi, pCtl);
    AssertRC(rc);
    return VINF_SUCCESS;
}

/**
 * Handler for VBVA_CMDVBVA_SUBMIT, see vbvaChannelHandler().
 *
 * @thread  EMT
 */
int vboxCmdVBVACmdSubmit(PVGASTATE pVGAState)
{
    if (!VBoxVBVAExHSIsEnabled(&pVGAState->pVdma->CmdVbva))
    {
        WARN(("vdma VBVA is disabled\n"));
        return VERR_INVALID_STATE;
    }

    return vboxVDMACmdSubmitPerform(pVGAState->pVdma);
}

/**
 * Handler for VBVA_CMDVBVA_FLUSH, see vbvaChannelHandler().
 *
 * @thread  EMT
 */
int vboxCmdVBVACmdFlush(PVGASTATE pVGAState)
{
    WARN(("flush\n"));
    if (!VBoxVBVAExHSIsEnabled(&pVGAState->pVdma->CmdVbva))
    {
        WARN(("vdma VBVA is disabled\n"));
        return VERR_INVALID_STATE;
    }
    return vboxVDMACmdSubmitPerform(pVGAState->pVdma);
}

/**
 * Called from vgaTimerRefresh().
 */
void vboxCmdVBVATimerRefresh(PVGASTATE pVGAState)
{
    if (!VBoxVBVAExHSIsEnabled(&pVGAState->pVdma->CmdVbva))
        return;
    vboxVDMACmdSubmitPerform(pVGAState->pVdma);
}

bool vboxCmdVBVAIsEnabled(PVGASTATE pVGAState)
{
    return VBoxVBVAExHSIsEnabled(&pVGAState->pVdma->CmdVbva);
}

#endif /* VBOX_WITH_CRHGSMI */


/*
 *
 *
 * Saved state.
 * Saved state.
 * Saved state.
 *
 *
 */

int vboxVDMASaveStateExecPrep(struct VBOXVDMAHOST *pVdma)
{
#ifdef VBOX_WITH_CRHGSMI
    int rc = vdmaVBVAPause(pVdma);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    if (rc != VERR_INVALID_STATE)
    {
        WARN(("vdmaVBVAPause failed %Rrc\n", rc));
        return rc;
    }

# ifdef DEBUG_misha
    WARN(("debug prep"));
# endif

    PVGASTATE pVGAState = pVdma->pVGAState;
    PVBOXVDMACMD_CHROMIUM_CTL pCmd;
    pCmd = (PVBOXVDMACMD_CHROMIUM_CTL)vboxVDMACrCtlCreate(VBOXVDMACMD_CHROMIUM_CTL_TYPE_SAVESTATE_BEGIN, sizeof(*pCmd));
    if (pCmd)
    {
        rc = vboxVDMACrCtlPost(pVGAState, pCmd, sizeof (*pCmd));
        AssertRC(rc);
        if (RT_SUCCESS(rc))
            rc = vboxVDMACrCtlGetRc(pCmd);
        vboxVDMACrCtlRelease(pCmd);
        return rc;
    }
    return VERR_NO_MEMORY;
#else
    RT_NOREF(pVdma);
    return VINF_SUCCESS;
#endif
}

int vboxVDMASaveStateExecDone(struct VBOXVDMAHOST *pVdma)
{
#ifdef VBOX_WITH_CRHGSMI
    int rc = vdmaVBVAResume(pVdma);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    if (rc != VERR_INVALID_STATE)
    {
        WARN(("vdmaVBVAResume failed %Rrc\n", rc));
        return rc;
    }

# ifdef DEBUG_misha
    WARN(("debug done"));
# endif

    PVGASTATE pVGAState = pVdma->pVGAState;
    PVBOXVDMACMD_CHROMIUM_CTL pCmd;
    pCmd = (PVBOXVDMACMD_CHROMIUM_CTL)vboxVDMACrCtlCreate(VBOXVDMACMD_CHROMIUM_CTL_TYPE_SAVESTATE_END, sizeof(*pCmd));
    Assert(pCmd);
    if (pCmd)
    {
        rc = vboxVDMACrCtlPost(pVGAState, pCmd, sizeof (*pCmd));
        AssertRC(rc);
        if (RT_SUCCESS(rc))
            rc = vboxVDMACrCtlGetRc(pCmd);
        vboxVDMACrCtlRelease(pCmd);
        return rc;
    }
    return VERR_NO_MEMORY;
#else
    RT_NOREF(pVdma);
    return VINF_SUCCESS;
#endif
}

int vboxVDMASaveStateExecPerform(struct VBOXVDMAHOST *pVdma, PSSMHANDLE pSSM)
{
    int rc;
#ifndef VBOX_WITH_CRHGSMI
    RT_NOREF(pVdma, pSSM);

#else
    if (!VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva))
#endif
    {
        rc = SSMR3PutU32(pSSM, UINT32_MAX);
        AssertRCReturn(rc, rc);
        return VINF_SUCCESS;
    }

#ifdef VBOX_WITH_CRHGSMI
    PVGASTATE pVGAState = pVdma->pVGAState;
    uint8_t * pu8VramBase = pVGAState->vram_ptrR3;

    rc = SSMR3PutU32(pSSM, (uint32_t)((uintptr_t)pVdma->CmdVbva.pVBVA - (uintptr_t)pu8VramBase));
    AssertRCReturn(rc, rc);

    VBVAEXHOSTCTL HCtl;
    HCtl.enmType = VBVAEXHOSTCTL_TYPE_HH_SAVESTATE;
    HCtl.u.state.pSSM = pSSM;
    HCtl.u.state.u32Version = 0;
    return vdmaVBVACtlSubmitSync(pVdma, &HCtl, VBVAEXHOSTCTL_SOURCE_HOST);
#endif
}

int vboxVDMASaveLoadExecPerform(struct VBOXVDMAHOST *pVdma, PSSMHANDLE pSSM, uint32_t u32Version)
{
    uint32_t u32;
    int rc = SSMR3GetU32(pSSM, &u32);
    AssertLogRelRCReturn(rc, rc);

    if (u32 != UINT32_MAX)
    {
#ifdef VBOX_WITH_CRHGSMI
        rc = vdmaVBVACtlEnableSubmitSync(pVdma, u32, true);
        AssertLogRelRCReturn(rc, rc);

        Assert(pVdma->CmdVbva.i32State == VBVAEXHOSTCONTEXT_ESTATE_PAUSED);

        VBVAEXHOSTCTL HCtl;
        HCtl.enmType = VBVAEXHOSTCTL_TYPE_HH_LOADSTATE;
        HCtl.u.state.pSSM = pSSM;
        HCtl.u.state.u32Version = u32Version;
        rc = vdmaVBVACtlSubmitSync(pVdma, &HCtl, VBVAEXHOSTCTL_SOURCE_HOST);
        AssertLogRelRCReturn(rc, rc);

        rc = vdmaVBVAResume(pVdma);
        AssertLogRelRCReturn(rc, rc);

        return VINF_SUCCESS;
#else
        RT_NOREF(pVdma, u32Version);
        WARN(("Unsupported VBVACtl info!\n"));
        return VERR_VERSION_MISMATCH;
#endif
    }

    return VINF_SUCCESS;
}

int vboxVDMASaveLoadDone(struct VBOXVDMAHOST *pVdma)
{
#ifdef VBOX_WITH_CRHGSMI
    if (!VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva))
        return VINF_SUCCESS;

/** @todo r=bird: BTW. would be great if you put in a couple of comments here and there explaining what
       * the purpose of this code is. */
    VBVAEXHOSTCTL *pHCtl = VBoxVBVAExHCtlCreate(&pVdma->CmdVbva, VBVAEXHOSTCTL_TYPE_HH_LOADSTATE_DONE);
    if (!pHCtl)
    {
        WARN(("VBoxVBVAExHCtlCreate failed\n"));
        return VERR_NO_MEMORY;
    }

    /* sanity */
    pHCtl->u.cmd.pvCmd = NULL;
    pHCtl->u.cmd.cbCmd = 0;

    /* NULL completion will just free the ctl up */
    int rc = vdmaVBVACtlSubmit(pVdma, pHCtl, VBVAEXHOSTCTL_SOURCE_HOST, NULL, NULL);
    if (RT_FAILURE(rc))
    {
        Log(("vdmaVBVACtlSubmit failed %Rrc\n", rc));
        VBoxVBVAExHCtlFree(&pVdma->CmdVbva, pHCtl);
        return rc;
    }
#else
    RT_NOREF(pVdma);
#endif
    return VINF_SUCCESS;
}

