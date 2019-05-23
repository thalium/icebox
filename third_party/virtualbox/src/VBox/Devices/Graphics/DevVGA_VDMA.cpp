/* $Id: DevVGA_VDMA.cpp $ */
/** @file
 * Video DMA (VDMA) support.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_VGA
#include <VBox/VMMDev.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pgm.h>
#include <VBoxVideo.h>
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
    VBVABUFFER *pVBVA;
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

typedef DECLCALLBACKPTR(void, PFNVBVAEXHOSTCTL_COMPLETE)(VBVAEXHOSTCONTEXT *pVbva, struct VBVAEXHOSTCTL *pCtl, int rc, void *pvComplete);

typedef struct VBVAEXHOSTCTL
{
    RTLISTNODE Node;
    VBVAEXHOSTCTL_TYPE enmType;
    union
    {
        struct
        {
            uint8_t * pu8Cmd;
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
    PHGSMIINSTANCE pHgsmi;
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


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
#ifdef VBOX_WITH_CRHGSMI
static DECLCALLBACK(int) vdmaVBVANotifyDisable(PVGASTATE pVGAState);
static VBVAEXHOST_DATA_TYPE VBoxVBVAExHPDataGet(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t **ppCmd, uint32_t *pcbCmd);

static void VBoxVBVAExHPDataCompleteCmd(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint32_t cbCmd);
static void VBoxVBVAExHPDataCompleteCtl(struct VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL *pCtl, int rc);

/* VBoxVBVAExHP**, i.e. processor functions, can NOT be called concurrently with each other,
 * can be called concurrently with istelf as well as with other VBoxVBVAEx** functions except Init/Start/Term aparently */
static int VBoxVBVAExHSCheckCommands(struct VBVAEXHOSTCONTEXT *pCmdVbva);

static int VBoxVBVAExHSInit(struct VBVAEXHOSTCONTEXT *pCmdVbva);
static int VBoxVBVAExHSEnable(struct VBVAEXHOSTCONTEXT *pCmdVbva, VBVABUFFER *pVBVA);
static int VBoxVBVAExHSDisable(struct VBVAEXHOSTCONTEXT *pCmdVbva);
static void VBoxVBVAExHSTerm(struct VBVAEXHOSTCONTEXT *pCmdVbva);
static int VBoxVBVAExHSSaveState(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t* pu8VramBase, PSSMHANDLE pSSM);
static int VBoxVBVAExHSLoadState(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t* pu8VramBase, PSSMHANDLE pSSM, uint32_t u32Version);

#endif /* VBOX_WITH_CRHGSMI */



#ifdef VBOX_WITH_CRHGSMI

static VBVAEXHOSTCTL* VBoxVBVAExHCtlAlloc(VBVAEXHOSTCONTEXT *pCmdVbva)
{
# ifndef VBOXVDBG_MEMCACHE_DISABLE
    return (VBVAEXHOSTCTL*)RTMemCacheAlloc(pCmdVbva->CtlCache);
# else
    return (VBVAEXHOSTCTL*)RTMemAlloc(sizeof (VBVAEXHOSTCTL));
# endif
}

static void VBoxVBVAExHCtlFree(VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL *pCtl)
{
# ifndef VBOXVDBG_MEMCACHE_DISABLE
    RTMemCacheFree(pCmdVbva->CtlCache, pCtl);
# else
    RTMemFree(pCtl);
# endif
}

static VBVAEXHOSTCTL *VBoxVBVAExHCtlCreate(VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL_TYPE enmType)
{
    VBVAEXHOSTCTL* pCtl = VBoxVBVAExHCtlAlloc(pCmdVbva);
    if (!pCtl)
    {
        WARN(("VBoxVBVAExHCtlAlloc failed\n"));
        return NULL;
    }

    pCtl->enmType = enmType;
    return pCtl;
}

static int vboxVBVAExHSProcessorAcquire(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    Assert(pCmdVbva->i32State >= VBVAEXHOSTCONTEXT_STATE_LISTENING);

    if (ASMAtomicCmpXchgS32(&pCmdVbva->i32State, VBVAEXHOSTCONTEXT_STATE_PROCESSING, VBVAEXHOSTCONTEXT_STATE_LISTENING))
            return VINF_SUCCESS;
    return VERR_SEM_BUSY;
}

static VBVAEXHOSTCTL* vboxVBVAExHPCheckCtl(struct VBVAEXHOSTCONTEXT *pCmdVbva, bool *pfHostCtl, bool fHostOnlyMode)
{
    Assert(pCmdVbva->i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);

    if (!fHostOnlyMode && !ASMAtomicUoReadU32(&pCmdVbva->u32cCtls))
        return NULL;

    int rc = RTCritSectEnter(&pCmdVbva->CltCritSect);
    if (RT_SUCCESS(rc))
    {
        VBVAEXHOSTCTL* pCtl = RTListGetFirst(&pCmdVbva->HostCtlList, VBVAEXHOSTCTL, Node);
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
        WARN(("RTCritSectEnter failed %d\n", rc));

    return NULL;
}

static VBVAEXHOSTCTL* VBoxVBVAExHPCheckHostCtlOnDisable(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    bool fHostCtl = false;
    VBVAEXHOSTCTL* pCtl = vboxVBVAExHPCheckCtl(pCmdVbva, &fHostCtl, true);
    Assert(!pCtl || fHostCtl);
    return pCtl;
}

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

static bool vboxVBVAExHPCheckProcessCtlInternal(struct VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL* pCtl)
{
    switch (pCtl->enmType)
    {
        case VBVAEXHOSTCTL_TYPE_HH_INTERNAL_PAUSE:
        {
            VBoxVBVAExHPPause(pCmdVbva);
            VBoxVBVAExHPDataCompleteCtl(pCmdVbva, pCtl, VINF_SUCCESS);
            return true;
        }
        case VBVAEXHOSTCTL_TYPE_HH_INTERNAL_RESUME:
        {
            VBoxVBVAExHPResume(pCmdVbva);
            VBoxVBVAExHPDataCompleteCtl(pCmdVbva, pCtl, VINF_SUCCESS);
            return true;
        }
        default:
            return false;
    }
}

static void vboxVBVAExHPProcessorRelease(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    Assert(pCmdVbva->i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);

    ASMAtomicWriteS32(&pCmdVbva->i32State, VBVAEXHOSTCONTEXT_STATE_LISTENING);
}

static void vboxVBVAExHPHgEventSet(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    Assert(pCmdVbva->i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);
    if (pCmdVbva->pVBVA)
        ASMAtomicOrU32(&pCmdVbva->pVBVA->hostFlags.u32HostEvents, VBVA_F_STATE_PROCESSING);
}

static void vboxVBVAExHPHgEventClear(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    Assert(pCmdVbva->i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);
    if (pCmdVbva->pVBVA)
        ASMAtomicAndU32(&pCmdVbva->pVBVA->hostFlags.u32HostEvents, ~VBVA_F_STATE_PROCESSING);
}

static int vboxVBVAExHPCmdGet(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t **ppCmd, uint32_t *pcbCmd)
{
    Assert(pCmdVbva->i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);
    Assert(pCmdVbva->i32EnableState > VBVAEXHOSTCONTEXT_ESTATE_PAUSED);

    VBVABUFFER *pVBVA = pCmdVbva->pVBVA;

    uint32_t indexRecordFirst = pVBVA->indexRecordFirst;
    uint32_t indexRecordFree = pVBVA->indexRecordFree;

    Log(("first = %d, free = %d\n",
                   indexRecordFirst, indexRecordFree));

    if (indexRecordFirst == indexRecordFree)
    {
        /* No records to process. Return without assigning output variables. */
        return VINF_EOF;
    }

    uint32_t cbRecordCurrent = ASMAtomicReadU32(&pVBVA->aRecords[indexRecordFirst].cbRecord);

    /* A new record need to be processed. */
    if (cbRecordCurrent & VBVA_F_RECORD_PARTIAL)
    {
        /* the record is being recorded, try again */
        return VINF_TRY_AGAIN;
    }

    uint32_t cbRecord = cbRecordCurrent & ~VBVA_F_RECORD_PARTIAL;

    if (!cbRecord)
    {
        /* the record is being recorded, try again */
        return VINF_TRY_AGAIN;
    }

    /* we should not get partial commands here actually */
    Assert(cbRecord);

    /* The size of largest contiguous chunk in the ring biffer. */
    uint32_t u32BytesTillBoundary = pVBVA->cbData - pVBVA->off32Data;

    /* The pointer to data in the ring buffer. */
    uint8_t *pSrc = &pVBVA->au8Data[pVBVA->off32Data];

    /* Fetch or point the data. */
    if (u32BytesTillBoundary >= cbRecord)
    {
        /* The command does not cross buffer boundary. Return address in the buffer. */
        *ppCmd = pSrc;
        *pcbCmd = cbRecord;
        return VINF_SUCCESS;
    }

    LogRel(("CmdVbva: cross-bound writes unsupported\n"));
    return VERR_INVALID_STATE;
}

static void VBoxVBVAExHPDataCompleteCmd(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint32_t cbCmd)
{
    VBVABUFFER *pVBVA = pCmdVbva->pVBVA;
    pVBVA->off32Data = (pVBVA->off32Data + cbCmd) % pVBVA->cbData;

    pVBVA->indexRecordFirst = (pVBVA->indexRecordFirst + 1) % RT_ELEMENTS(pVBVA->aRecords);
}

static void VBoxVBVAExHPDataCompleteCtl(struct VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL *pCtl, int rc)
{
    if (pCtl->pfnComplete)
        pCtl->pfnComplete(pCmdVbva, pCtl, rc, pCtl->pvComplete);
    else
        VBoxVBVAExHCtlFree(pCmdVbva, pCtl);
}


static VBVAEXHOST_DATA_TYPE vboxVBVAExHPDataGet(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t **ppCmd, uint32_t *pcbCmd)
{
    Assert(pCmdVbva->i32State == VBVAEXHOSTCONTEXT_STATE_PROCESSING);
    VBVAEXHOSTCTL*pCtl;
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
                    *ppCmd = (uint8_t*)pCtl;
                    *pcbCmd = sizeof (*pCtl);
                    return VBVAEXHOST_DATA_TYPE_HOSTCTL;
                }
                continue;
            }
            else
            {
                *ppCmd = (uint8_t*)pCtl;
                *pcbCmd = sizeof (*pCtl);
                return VBVAEXHOST_DATA_TYPE_GUESTCTL;
            }
        }

        if (ASMAtomicUoReadS32(&pCmdVbva->i32EnableState) <= VBVAEXHOSTCONTEXT_ESTATE_PAUSED)
            return VBVAEXHOST_DATA_TYPE_NO_DATA;

        int rc = vboxVBVAExHPCmdGet(pCmdVbva, ppCmd, pcbCmd);
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
                WARN(("Warning: vboxVBVAExHCmdGet returned unexpected status %d\n", rc));
                return VBVAEXHOST_DATA_TYPE_NO_DATA;
        }
    }
    /* not reached */
}

static VBVAEXHOST_DATA_TYPE VBoxVBVAExHPDataGet(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t **ppCmd, uint32_t *pcbCmd)
{
    VBVAEXHOST_DATA_TYPE enmType = vboxVBVAExHPDataGet(pCmdVbva, ppCmd, pcbCmd);
    if (enmType == VBVAEXHOST_DATA_TYPE_NO_DATA)
    {
        vboxVBVAExHPHgEventClear(pCmdVbva);
        vboxVBVAExHPProcessorRelease(pCmdVbva);
        /* we need to prevent racing between us clearing the flag and command check/submission thread, i.e.
         * 1. we check the queue -> and it is empty
         * 2. submitter adds command to the queue
         * 3. submitter checks the "processing" -> and it is true , thus it does not submit a notification
         * 4. we clear the "processing" state
         * 5. ->here we need to re-check the queue state to ensure we do not leak the notification of the above command
         * 6. if the queue appears to be not-empty set the "processing" state back to "true"
         **/
        int rc = vboxVBVAExHSProcessorAcquire(pCmdVbva);
        if (RT_SUCCESS(rc))
        {
            /* we are the processor now */
            enmType = vboxVBVAExHPDataGet(pCmdVbva, ppCmd, pcbCmd);
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

DECLINLINE(bool) vboxVBVAExHSHasCommands(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    VBVABUFFER *pVBVA = pCmdVbva->pVBVA;

    if (pVBVA)
    {
        uint32_t indexRecordFirst = pVBVA->indexRecordFirst;
        uint32_t indexRecordFree = pVBVA->indexRecordFree;

        if (indexRecordFirst != indexRecordFree)
            return true;
    }

    return !!ASMAtomicReadU32(&pCmdVbva->u32cCtls);
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

static int VBoxVBVAExHSInit(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    memset(pCmdVbva, 0, sizeof (*pCmdVbva));
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
            pCmdVbva->i32State = VBVAEXHOSTCONTEXT_STATE_PROCESSING;
            pCmdVbva->i32EnableState = VBVAEXHOSTCONTEXT_ESTATE_DISABLED;
            return VINF_SUCCESS;
        }
# ifndef VBOXVDBG_MEMCACHE_DISABLE
        else
            WARN(("RTMemCacheCreate failed %d\n", rc));
# endif
    }
    else
        WARN(("RTCritSectInit failed %d\n", rc));

    return rc;
}

DECLINLINE(bool) VBoxVBVAExHSIsEnabled(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    return (ASMAtomicUoReadS32(&pCmdVbva->i32EnableState) >= VBVAEXHOSTCONTEXT_ESTATE_PAUSED);
}

DECLINLINE(bool) VBoxVBVAExHSIsDisabled(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    return (ASMAtomicUoReadS32(&pCmdVbva->i32EnableState) == VBVAEXHOSTCONTEXT_ESTATE_DISABLED);
}

static int VBoxVBVAExHSEnable(struct VBVAEXHOSTCONTEXT *pCmdVbva, VBVABUFFER *pVBVA)
{
    if (VBoxVBVAExHSIsEnabled(pCmdVbva))
    {
        WARN(("VBVAEx is enabled already\n"));
        return VERR_INVALID_STATE;
    }

    pCmdVbva->pVBVA = pVBVA;
    pCmdVbva->pVBVA->hostFlags.u32HostEvents = 0;
    ASMAtomicWriteS32(&pCmdVbva->i32EnableState, VBVAEXHOSTCONTEXT_ESTATE_ENABLED);
    return VINF_SUCCESS;
}

static int VBoxVBVAExHSDisable(struct VBVAEXHOSTCONTEXT *pCmdVbva)
{
    if (VBoxVBVAExHSIsDisabled(pCmdVbva))
        return VINF_SUCCESS;

    ASMAtomicWriteS32(&pCmdVbva->i32EnableState, VBVAEXHOSTCONTEXT_ESTATE_DISABLED);
    return VINF_SUCCESS;
}

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

    memset(pCmdVbva, 0, sizeof (*pCmdVbva));
}

static int vboxVBVAExHSSaveGuestCtl(struct VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL* pCtl, uint8_t* pu8VramBase, PSSMHANDLE pSSM)
{
    RT_NOREF(pCmdVbva);
    int rc = SSMR3PutU32(pSSM, pCtl->enmType);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, pCtl->u.cmd.cbCmd);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, (uint32_t)(pCtl->u.cmd.pu8Cmd - pu8VramBase));
    AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}

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


/** Saves state
 * @returns - same as VBoxVBVAExHSCheckCommands, or failure on load state fail
 */
static int VBoxVBVAExHSSaveState(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t* pu8VramBase, PSSMHANDLE pSSM)
{
    int rc = RTCritSectEnter(&pCmdVbva->CltCritSect);
    if (RT_FAILURE(rc))
    {
        WARN(("RTCritSectEnter failed %d\n", rc));
        return rc;
    }

    rc = vboxVBVAExHSSaveStateLocked(pCmdVbva, pu8VramBase, pSSM);
    if (RT_FAILURE(rc))
        WARN(("vboxVBVAExHSSaveStateLocked failed %d\n", rc));

    RTCritSectLeave(&pCmdVbva->CltCritSect);

    return rc;
}

static int vboxVBVAExHSLoadGuestCtl(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t* pu8VramBase, PSSMHANDLE pSSM, uint32_t u32Version)
{
    RT_NOREF(u32Version);
    uint32_t u32;
    int rc = SSMR3GetU32(pSSM, &u32);
    AssertLogRelRCReturn(rc, rc);

    if (!u32)
        return VINF_EOF;

    VBVAEXHOSTCTL* pHCtl = VBoxVBVAExHCtlCreate(pCmdVbva, (VBVAEXHOSTCTL_TYPE)u32);
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
    pHCtl->u.cmd.pu8Cmd = pu8VramBase + u32;

    RTListAppend(&pCmdVbva->GuestCtlList, &pHCtl->Node);
    ++pCmdVbva->u32cCtls;

    return VINF_SUCCESS;
}


static int vboxVBVAExHSLoadStateLocked(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t* pu8VramBase, PSSMHANDLE pSSM, uint32_t u32Version)
{
    if (ASMAtomicUoReadS32(&pCmdVbva->i32EnableState) != VBVAEXHOSTCONTEXT_ESTATE_PAUSED)
    {
        WARN(("vbva not stopped\n"));
        return VERR_INVALID_STATE;
    }

    int rc;

    do {
        rc = vboxVBVAExHSLoadGuestCtl(pCmdVbva, pu8VramBase, pSSM, u32Version);
        AssertLogRelRCReturn(rc, rc);
    } while (VINF_EOF != rc);

    return VINF_SUCCESS;
}

/** Loads state
 * @returns - same as VBoxVBVAExHSCheckCommands, or failure on load state fail
 */
static int VBoxVBVAExHSLoadState(struct VBVAEXHOSTCONTEXT *pCmdVbva, uint8_t* pu8VramBase, PSSMHANDLE pSSM, uint32_t u32Version)
{
    Assert(VGA_SAVEDSTATE_VERSION_3D <= u32Version);
    int rc = RTCritSectEnter(&pCmdVbva->CltCritSect);
    if (RT_FAILURE(rc))
    {
        WARN(("RTCritSectEnter failed %d\n", rc));
        return rc;
    }

    rc = vboxVBVAExHSLoadStateLocked(pCmdVbva, pu8VramBase, pSSM, u32Version);
    if (RT_FAILURE(rc))
        WARN(("vboxVBVAExHSSaveStateLocked failed %d\n", rc));

    RTCritSectLeave(&pCmdVbva->CltCritSect);

    return rc;
}

typedef enum
{
    VBVAEXHOSTCTL_SOURCE_GUEST = 0,
    VBVAEXHOSTCTL_SOURCE_HOST
} VBVAEXHOSTCTL_SOURCE;


static int VBoxVBVAExHCtlSubmit(VBVAEXHOSTCONTEXT *pCmdVbva, VBVAEXHOSTCTL* pCtl, VBVAEXHOSTCTL_SOURCE enmSource, PFNVBVAEXHOSTCTL_COMPLETE pfnComplete, void *pvComplete)
{
    if (!VBoxVBVAExHSIsEnabled(pCmdVbva))
    {
        Log(("cmd vbva not enabled\n"));
        return VERR_INVALID_STATE;
    }

    pCtl->pfnComplete = pfnComplete;
    pCtl->pvComplete = pvComplete;

    int rc = RTCritSectEnter(&pCmdVbva->CltCritSect);
    if (RT_SUCCESS(rc))
    {
        if (!VBoxVBVAExHSIsEnabled(pCmdVbva))
        {
            Log(("cmd vbva not enabled\n"));
            RTCritSectLeave(&pCmdVbva->CltCritSect);
            return VERR_INVALID_STATE;
        }

        if (enmSource > VBVAEXHOSTCTL_SOURCE_GUEST)
        {
            RTListAppend(&pCmdVbva->HostCtlList, &pCtl->Node);
        }
        else
            RTListAppend(&pCmdVbva->GuestCtlList, &pCtl->Node);

        ASMAtomicIncU32(&pCmdVbva->u32cCtls);

        RTCritSectLeave(&pCmdVbva->CltCritSect);

        rc = VBoxVBVAExHSCheckCommands(pCmdVbva);
    }
    else
        WARN(("RTCritSectEnter failed %d\n", rc));

    return rc;
}

void VBoxVDMAThreadNotifyConstructSucceeded(PVBOXVDMATHREAD pThread, void *pvThreadContext)
{
    Assert(pThread->u32State == VBOXVDMATHREAD_STATE_CREATING);
    PFNVBOXVDMATHREAD_CHANGED pfnChanged = pThread->pfnChanged;
    void *pvChanged = pThread->pvChanged;

    pThread->pfnChanged = NULL;
    pThread->pvChanged = NULL;

    ASMAtomicWriteU32(&pThread->u32State, VBOXVDMATHREAD_STATE_CREATED);

    if (pfnChanged)
        pfnChanged(pThread, VINF_SUCCESS, pvThreadContext, pvChanged);
}

void VBoxVDMAThreadNotifyTerminatingSucceeded(PVBOXVDMATHREAD pThread, void *pvThreadContext)
{
    Assert(pThread->u32State == VBOXVDMATHREAD_STATE_TERMINATING);
    PFNVBOXVDMATHREAD_CHANGED pfnChanged = pThread->pfnChanged;
    void *pvChanged = pThread->pvChanged;

    pThread->pfnChanged = NULL;
    pThread->pvChanged = NULL;

    if (pfnChanged)
        pfnChanged(pThread, VINF_SUCCESS, pvThreadContext, pvChanged);
}

DECLINLINE(bool) VBoxVDMAThreadIsTerminating(PVBOXVDMATHREAD pThread)
{
    return ASMAtomicUoReadU32(&pThread->u32State) == VBOXVDMATHREAD_STATE_TERMINATING;
}

void VBoxVDMAThreadInit(PVBOXVDMATHREAD pThread)
{
    memset(pThread, 0, sizeof (*pThread));
    pThread->u32State = VBOXVDMATHREAD_STATE_TERMINATED;
}

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
            if (!RT_SUCCESS(rc))
            {
                WARN(("RTThreadWait failed %d\n", rc));
                return rc;
            }

            RTSemEventDestroy(pThread->hEvent);

            ASMAtomicWriteU32(&pThread->u32State, VBOXVDMATHREAD_STATE_TERMINATED);
            return VINF_SUCCESS;
        }
        default:
            WARN(("invalid state"));
            return VERR_INVALID_STATE;
    }
}

int VBoxVDMAThreadCreate(PVBOXVDMATHREAD pThread, PFNRTTHREAD pfnThread, void *pvThread, PFNVBOXVDMATHREAD_CHANGED pfnCreated, void*pvCreated)
{
    int rc = VBoxVDMAThreadCleanup(pThread);
    if (RT_FAILURE(rc))
    {
        WARN(("VBoxVDMAThreadCleanup failed %d\n", rc));
        return rc;
    }

    rc = RTSemEventCreate(&pThread->hEvent);
    if (RT_SUCCESS(rc))
    {
        pThread->u32State = VBOXVDMATHREAD_STATE_CREATING;
        pThread->pfnChanged = pfnCreated;
        pThread->pvChanged = pvCreated;
        rc = RTThreadCreate(&pThread->hWorkerThread, pfnThread, pvThread, 0, RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, "VDMA");
        if (RT_SUCCESS(rc))
            return VINF_SUCCESS;
        else
            WARN(("RTThreadCreate failed %d\n", rc));

        RTSemEventDestroy(pThread->hEvent);
    }
    else
        WARN(("RTSemEventCreate failed %d\n", rc));

    pThread->u32State = VBOXVDMATHREAD_STATE_TERMINATED;

    return rc;
}

DECLINLINE(int) VBoxVDMAThreadEventNotify(PVBOXVDMATHREAD pThread)
{
    int rc = RTSemEventSignal(pThread->hEvent);
    AssertRC(rc);
    return rc;
}

DECLINLINE(int) VBoxVDMAThreadEventWait(PVBOXVDMATHREAD pThread, RTMSINTERVAL cMillies)
{
    int rc = RTSemEventWait(pThread->hEvent, cMillies);
    AssertRC(rc);
    return rc;
}

int VBoxVDMAThreadTerm(PVBOXVDMATHREAD pThread, PFNVBOXVDMATHREAD_CHANGED pfnTerminated, void*pvTerminated, bool fNotify)
{
    int rc;
    do
    {
        uint32_t u32State = ASMAtomicUoReadU32(&pThread->u32State);
        switch (u32State)
        {
            case VBOXVDMATHREAD_STATE_CREATED:
                pThread->pfnChanged = pfnTerminated;
                pThread->pvChanged = pvTerminated;
                ASMAtomicWriteU32(&pThread->u32State, VBOXVDMATHREAD_STATE_TERMINATING);
                if (fNotify)
                {
                    rc = VBoxVDMAThreadEventNotify(pThread);
                    AssertRC(rc);
                }
                return VINF_SUCCESS;
            case VBOXVDMATHREAD_STATE_TERMINATING:
            case VBOXVDMATHREAD_STATE_TERMINATED:
            {
                WARN(("thread is marked to termination or terminated\nn"));
                return VERR_INVALID_STATE;
            }
            case VBOXVDMATHREAD_STATE_CREATING:
            {
                /* wait till the thread creation is completed */
                WARN(("concurrent thread create/destron\n"));
                RTThreadYield();
                continue;
            }
            default:
                WARN(("invalid state"));
                return VERR_INVALID_STATE;
        }
    } while (1);

    WARN(("should never be here\n"));
    return VERR_INTERNAL_ERROR;
}

static int vdmaVBVACtlSubmitSync(PVBOXVDMAHOST pVdma, VBVAEXHOSTCTL* pCtl, VBVAEXHOSTCTL_SOURCE enmSource);

typedef DECLCALLBACK(void) FNVBOXVDMACRCTL_CALLBACK(PVGASTATE pVGAState, PVBOXVDMACMD_CHROMIUM_CTL pCmd, void* pvContext);
typedef FNVBOXVDMACRCTL_CALLBACK *PFNVBOXVDMACRCTL_CALLBACK;

typedef struct VBOXVDMACMD_CHROMIUM_CTL_PRIVATE
{
    uint32_t cRefs;
    int32_t rc;
    PFNVBOXVDMACRCTL_CALLBACK pfnCompletion;
    void *pvCompletion;
    VBOXVDMACMD_CHROMIUM_CTL Cmd;
} VBOXVDMACMD_CHROMIUM_CTL_PRIVATE, *PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE;

# define VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(_p) ((PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE)(((uint8_t*)(_p)) - RT_OFFSETOF(VBOXVDMACMD_CHROMIUM_CTL_PRIVATE, Cmd)))

static PVBOXVDMACMD_CHROMIUM_CTL vboxVDMACrCtlCreate(VBOXVDMACMD_CHROMIUM_CTL_TYPE enmCmd, uint32_t cbCmd)
{
    PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pHdr = (PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE)RTMemAllocZ(cbCmd + RT_OFFSETOF(VBOXVDMACMD_CHROMIUM_CTL_PRIVATE, Cmd));
    Assert(pHdr);
    if (pHdr)
    {
        pHdr->cRefs = 1;
        pHdr->rc = VERR_NOT_IMPLEMENTED;
        pHdr->Cmd.enmType = enmCmd;
        pHdr->Cmd.cbCmd = cbCmd;
        return &pHdr->Cmd;
    }

    return NULL;
}

DECLINLINE(void) vboxVDMACrCtlRelease (PVBOXVDMACMD_CHROMIUM_CTL pCmd)
{
    PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pHdr = VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(pCmd);
    uint32_t cRefs = ASMAtomicDecU32(&pHdr->cRefs);
    if (!cRefs)
        RTMemFree(pHdr);
}

#if 0 /* unused */
DECLINLINE(void) vboxVDMACrCtlRetain(PVBOXVDMACMD_CHROMIUM_CTL pCmd)
{
    PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pHdr = VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(pCmd);
    ASMAtomicIncU32(&pHdr->cRefs);
}
#endif /* unused */

DECLINLINE(int) vboxVDMACrCtlGetRc (PVBOXVDMACMD_CHROMIUM_CTL pCmd)
{
    PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pHdr = VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(pCmd);
    return pHdr->rc;
}

static DECLCALLBACK(void) vboxVDMACrCtlCbSetEvent(PVGASTATE pVGAState, PVBOXVDMACMD_CHROMIUM_CTL pCmd, void* pvContext)
{
    RT_NOREF(pVGAState, pCmd);
    RTSemEventSignal((RTSEMEVENT)pvContext);
}

# if 0 /** @todo vboxVDMACrCtlCbReleaseCmd is unused */
static DECLCALLBACK(void) vboxVDMACrCtlCbReleaseCmd(PVGASTATE pVGAState, PVBOXVDMACMD_CHROMIUM_CTL pCmd, void* pvContext)
{
    RT_NOREF(pVGAState, pvContext);
    vboxVDMACrCtlRelease(pCmd);
}
# endif

static int vboxVDMACrCtlPostAsync (PVGASTATE pVGAState, PVBOXVDMACMD_CHROMIUM_CTL pCmd, uint32_t cbCmd, PFNVBOXVDMACRCTL_CALLBACK pfnCompletion, void *pvCompletion)
{
    if (   pVGAState->pDrv
        && pVGAState->pDrv->pfnCrHgsmiControlProcess)
    {
        PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pHdr = VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(pCmd);
        pHdr->pfnCompletion = pfnCompletion;
        pHdr->pvCompletion = pvCompletion;
        pVGAState->pDrv->pfnCrHgsmiControlProcess(pVGAState->pDrv, pCmd, cbCmd);
        return VINF_SUCCESS;
    }
# ifdef DEBUG_misha
    Assert(0);
# endif
    return VERR_NOT_SUPPORTED;
}

static int vboxVDMACrCtlPost(PVGASTATE pVGAState, PVBOXVDMACMD_CHROMIUM_CTL pCmd, uint32_t cbCmd)
{
    RTSEMEVENT hComplEvent;
    int rc = RTSemEventCreate(&hComplEvent);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
    {
        rc = vboxVDMACrCtlPostAsync(pVGAState, pCmd, cbCmd, vboxVDMACrCtlCbSetEvent, (void*)hComplEvent);
# ifdef DEBUG_misha
        AssertRC(rc);
# endif
        if (RT_SUCCESS(rc))
        {
            rc = RTSemEventWaitNoResume(hComplEvent, RT_INDEFINITE_WAIT);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                RTSemEventDestroy(hComplEvent);
            }
        }
        else
        {
            /* the command is completed */
            RTSemEventDestroy(hComplEvent);
        }
    }
    return rc;
}

typedef struct VDMA_VBVA_CTL_CYNC_COMPLETION
{
    int rc;
    RTSEMEVENT hEvent;
} VDMA_VBVA_CTL_CYNC_COMPLETION;

static DECLCALLBACK(void) vboxVDMACrHgcmSubmitSyncCompletion(struct VBOXCRCMDCTL* pCmd, uint32_t cbCmd, int rc, void *pvCompletion)
{
    RT_NOREF(pCmd, cbCmd);
    VDMA_VBVA_CTL_CYNC_COMPLETION *pData = (VDMA_VBVA_CTL_CYNC_COMPLETION*)pvCompletion;
    pData->rc = rc;
    rc = RTSemEventSignal(pData->hEvent);
    if (!RT_SUCCESS(rc))
        WARN(("RTSemEventSignal failed %d\n", rc));
}

static int vboxVDMACrHgcmSubmitSync(struct VBOXVDMAHOST *pVdma, VBOXCRCMDCTL* pCtl, uint32_t cbCtl)
{
    VDMA_VBVA_CTL_CYNC_COMPLETION Data;
    Data.rc = VERR_NOT_IMPLEMENTED;
    int rc = RTSemEventCreate(&Data.hEvent);
    if (!RT_SUCCESS(rc))
    {
        WARN(("RTSemEventCreate failed %d\n", rc));
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
                WARN(("pfnCrHgcmCtlSubmit command failed %d\n", rc));
            }

        }
        else
            WARN(("RTSemEventWait failed %d\n", rc));
    }
    else
        WARN(("pfnCrHgcmCtlSubmit failed %d\n", rc));


    RTSemEventDestroy(Data.hEvent);

    return rc;
}

static int vdmaVBVACtlDisableSync(PVBOXVDMAHOST pVdma)
{
    VBVAEXHOSTCTL HCtl;
    HCtl.enmType = VBVAEXHOSTCTL_TYPE_GHH_DISABLE;
    int rc = vdmaVBVACtlSubmitSync(pVdma, &HCtl, VBVAEXHOSTCTL_SOURCE_HOST);
    if (RT_FAILURE(rc))
    {
        Log(("vdmaVBVACtlSubmitSync failed %d\n", rc));
        return rc;
    }

    vgaUpdateDisplayAll(pVdma->pVGAState, /* fFailOnResize = */ false);

    return VINF_SUCCESS;
}

static DECLCALLBACK(uint8_t*) vboxVDMACrHgcmHandleEnableRemainingHostCommand(HVBOXCRCMDCTL_REMAINING_HOST_COMMAND hClient, uint32_t *pcbCtl, int prevCmdRc)
{
    struct VBOXVDMAHOST *pVdma = hClient;
    if (!pVdma->pCurRemainingHostCtl)
    {
        /* disable VBVA, all subsequent host commands will go HGCM way */
        VBoxVBVAExHSDisable(&pVdma->CmdVbva);
    }
    else
    {
        VBoxVBVAExHPDataCompleteCtl(&pVdma->CmdVbva, pVdma->pCurRemainingHostCtl, prevCmdRc);
    }

    pVdma->pCurRemainingHostCtl = VBoxVBVAExHPCheckHostCtlOnDisable(&pVdma->CmdVbva);
    if (pVdma->pCurRemainingHostCtl)
    {
        *pcbCtl = pVdma->pCurRemainingHostCtl->u.cmd.cbCmd;
        return pVdma->pCurRemainingHostCtl->u.cmd.pu8Cmd;
    }

    *pcbCtl = 0;
    return NULL;
}

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

static DECLCALLBACK(int) vboxVDMACrHgcmNotifyTerminatingCb(HVBOXCRCMDCTL_NOTIFY_TERMINATING hClient, VBOXCRCMDCTL_HGCMENABLE_DATA *pHgcmEnableData)
{
    struct VBOXVDMAHOST *pVdma = hClient;
    VBVAEXHOSTCTL HCtl;
    HCtl.enmType = VBVAEXHOSTCTL_TYPE_HH_ON_HGCM_UNLOAD;
    int rc = vdmaVBVACtlSubmitSync(pVdma, &HCtl, VBVAEXHOSTCTL_SOURCE_HOST);

    pHgcmEnableData->hRHCmd = pVdma;
    pHgcmEnableData->pfnRHCmd = vboxVDMACrHgcmHandleEnableRemainingHostCommand;

    if (RT_FAILURE(rc))
    {
        if (rc == VERR_INVALID_STATE)
            rc = VINF_SUCCESS;
        else
            WARN(("vdmaVBVACtlSubmitSync failed %d\n", rc));
    }

    return rc;
}

static int vboxVDMACrHgcmHandleEnable(struct VBOXVDMAHOST *pVdma)
{
    VBOXCRCMDCTL_ENABLE Enable;
    Enable.Hdr.enmType = VBOXCRCMDCTL_TYPE_ENABLE;
    Enable.Data.hRHCmd = pVdma;
    Enable.Data.pfnRHCmd = vboxVDMACrHgcmHandleEnableRemainingHostCommand;

    int rc = vboxVDMACrHgcmSubmitSync(pVdma, &Enable.Hdr, sizeof (Enable));
    Assert(!pVdma->pCurRemainingHostCtl);
    if (RT_SUCCESS(rc))
    {
        Assert(!VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva));
        return VINF_SUCCESS;
    }

    Assert(VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva));
    WARN(("vboxVDMACrHgcmSubmitSync failed %d\n", rc));

    return rc;
}

static int vdmaVBVAEnableProcess(struct VBOXVDMAHOST *pVdma, uint32_t u32Offset)
{
    if (VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva))
    {
        WARN(("vdma VBVA is already enabled\n"));
        return VERR_INVALID_STATE;
    }

    VBVABUFFER *pVBVA = (VBVABUFFER *)HGSMIOffsetToPointerHost(pVdma->pHgsmi, u32Offset);
    if (!pVBVA)
    {
        WARN(("invalid offset %d\n", u32Offset));
        return VERR_INVALID_PARAMETER;
    }

    if (!pVdma->CrSrvInfo.pfnEnable)
    {
# ifdef DEBUG_misha
        WARN(("pfnEnable is NULL\n"));
        return VERR_NOT_SUPPORTED;
# endif
    }

    int rc = VBoxVBVAExHSEnable(&pVdma->CmdVbva, pVBVA);
    if (RT_SUCCESS(rc))
    {
        VBOXCRCMDCTL_DISABLE Disable;
        Disable.Hdr.enmType = VBOXCRCMDCTL_TYPE_DISABLE;
        Disable.Data.hNotifyTerm = pVdma;
        Disable.Data.pfnNotifyTerm = vboxVDMACrHgcmNotifyTerminatingCb;
        Disable.Data.pfnNotifyTermDone = vboxVDMACrHgcmNotifyTerminatingDoneCb;
        rc = vboxVDMACrHgcmSubmitSync(pVdma, &Disable.Hdr, sizeof (Disable));
        if (RT_SUCCESS(rc))
        {
            PVGASTATE pVGAState = pVdma->pVGAState;
            VBOXCRCMD_SVRENABLE_INFO Info;
            Info.hCltScr = pVGAState->pDrv;
            Info.pfnCltScrUpdateBegin = pVGAState->pDrv->pfnVBVAUpdateBegin;
            Info.pfnCltScrUpdateProcess = pVGAState->pDrv->pfnVBVAUpdateProcess;
            Info.pfnCltScrUpdateEnd = pVGAState->pDrv->pfnVBVAUpdateEnd;
            rc = pVdma->CrSrvInfo.pfnEnable(pVdma->CrSrvInfo.hSvr, &Info);
            if (RT_SUCCESS(rc))
                return VINF_SUCCESS;
            else
                WARN(("pfnEnable failed %d\n", rc));

            vboxVDMACrHgcmHandleEnable(pVdma);
        }
        else
            WARN(("vboxVDMACrHgcmSubmitSync failed %d\n", rc));

        VBoxVBVAExHSDisable(&pVdma->CmdVbva);
    }
    else
        WARN(("VBoxVBVAExHSEnable failed %d\n", rc));

    return rc;
}

static int vdmaVBVADisableProcess(struct VBOXVDMAHOST *pVdma, bool fDoHgcmEnable)
{
    if (!VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva))
    {
        Log(("vdma VBVA is already disabled\n"));
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
            Info.hCltScr = pVGAState->pDrv;
            Info.pfnCltScrUpdateBegin = pVGAState->pDrv->pfnVBVAUpdateBegin;
            Info.pfnCltScrUpdateProcess = pVGAState->pDrv->pfnVBVAUpdateProcess;
            Info.pfnCltScrUpdateEnd = pVGAState->pDrv->pfnVBVAUpdateEnd;
            pVdma->CrSrvInfo.pfnEnable(pVdma->CrSrvInfo.hSvr, &Info);
        }
    }
    else
        WARN(("pfnDisable failed %d\n", rc));

    return rc;
}

static int vboxVDMACrHostCtlProcess(struct VBOXVDMAHOST *pVdma, VBVAEXHOSTCTL *pCmd, bool *pfContinue)
{
    *pfContinue = true;

    switch (pCmd->enmType)
    {
        case VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE:
        {
            if (!VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva))
            {
                WARN(("VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE for disabled vdma VBVA\n"));
                return VERR_INVALID_STATE;
            }
            return pVdma->CrSrvInfo.pfnHostCtl(pVdma->CrSrvInfo.hSvr, pCmd->u.cmd.pu8Cmd, pCmd->u.cmd.cbCmd);
        }
        case VBVAEXHOSTCTL_TYPE_GHH_DISABLE:
        {
            int rc = vdmaVBVADisableProcess(pVdma, true);
            if (RT_FAILURE(rc))
            {
                WARN(("vdmaVBVADisableProcess failed %d\n", rc));
                return rc;
            }

            return VBoxVDMAThreadTerm(&pVdma->Thread, NULL, NULL, false);
        }
        case VBVAEXHOSTCTL_TYPE_HH_ON_HGCM_UNLOAD:
        {
            int rc = vdmaVBVADisableProcess(pVdma, false);
            if (RT_FAILURE(rc))
            {
                WARN(("vdmaVBVADisableProcess failed %d\n", rc));
                return rc;
            }

            rc = VBoxVDMAThreadTerm(&pVdma->Thread, NULL, NULL, true);
            if (RT_FAILURE(rc))
            {
                WARN(("VBoxVDMAThreadTerm failed %d\n", rc));
                return rc;
            }

            *pfContinue = false;
            return VINF_SUCCESS;
        }
        case VBVAEXHOSTCTL_TYPE_HH_SAVESTATE:
        {
            PVGASTATE pVGAState = pVdma->pVGAState;
            uint8_t * pu8VramBase = pVGAState->vram_ptrR3;
            int rc = VBoxVBVAExHSSaveState(&pVdma->CmdVbva, pu8VramBase, pCmd->u.state.pSSM);
            if (RT_FAILURE(rc))
            {
                WARN(("VBoxVBVAExHSSaveState failed %d\n", rc));
                return rc;
            }
            VGA_SAVED_STATE_PUT_MARKER(pCmd->u.state.pSSM, 4);

            return pVdma->CrSrvInfo.pfnSaveState(pVdma->CrSrvInfo.hSvr, pCmd->u.state.pSSM);
        }
        case VBVAEXHOSTCTL_TYPE_HH_LOADSTATE:
        {
            PVGASTATE pVGAState = pVdma->pVGAState;
            uint8_t * pu8VramBase = pVGAState->vram_ptrR3;

            int rc = VBoxVBVAExHSLoadState(&pVdma->CmdVbva, pu8VramBase, pCmd->u.state.pSSM, pCmd->u.state.u32Version);
            if (RT_FAILURE(rc))
            {
                WARN(("VBoxVBVAExHSSaveState failed %d\n", rc));
                return rc;
            }

            VGA_SAVED_STATE_GET_MARKER_RETURN_ON_MISMATCH(pCmd->u.state.pSSM, pCmd->u.state.u32Version, 4);
            rc = pVdma->CrSrvInfo.pfnLoadState(pVdma->CrSrvInfo.hSvr, pCmd->u.state.pSSM, pCmd->u.state.u32Version);
            if (RT_FAILURE(rc))
            {
                WARN(("pfnLoadState failed %d\n", rc));
                return rc;
            }

            return VINF_SUCCESS;
        }
        case VBVAEXHOSTCTL_TYPE_HH_LOADSTATE_DONE:
        {
            PVGASTATE pVGAState = pVdma->pVGAState;

            for (uint32_t i = 0; i < pVGAState->cMonitors; ++i)
            {
                VBVAINFOSCREEN CurScreen;
                VBVAINFOVIEW CurView;

                int rc = VBVAGetInfoViewAndScreen(pVGAState, i, &CurView, &CurScreen);
                if (RT_FAILURE(rc))
                {
                    WARN(("VBVAGetInfoViewAndScreen failed %d\n", rc));
                    return rc;
                }

                rc = VBVAInfoScreen(pVGAState, &CurScreen);
                if (RT_FAILURE(rc))
                {
                    WARN(("VBVAInfoScreen failed %d\n", rc));
                    return rc;
                }
            }

            return VINF_SUCCESS;
        }
        default:
            WARN(("unexpected host ctl type %d\n", pCmd->enmType));
            return VERR_INVALID_PARAMETER;
    }
}

static int vboxVDMASetupScreenInfo(PVGASTATE pVGAState, VBVAINFOSCREEN *pScreen)
{
    const uint32_t u32ViewIndex = pScreen->u32ViewIndex;
    const uint16_t u16Flags = pScreen->u16Flags;

    if (u16Flags & VBVA_SCREEN_F_DISABLED)
    {
        if (   u32ViewIndex < pVGAState->cMonitors
            || u32ViewIndex == UINT32_C(0xFFFFFFFF))
        {
            RT_ZERO(*pScreen);
            pScreen->u32ViewIndex = u32ViewIndex;
            pScreen->u16Flags = VBVA_SCREEN_F_ACTIVE | VBVA_SCREEN_F_DISABLED;
            return VINF_SUCCESS;
        }
    }
    else
    {
        if (u16Flags & VBVA_SCREEN_F_BLANK2)
        {
            if (   u32ViewIndex >= pVGAState->cMonitors
                && u32ViewIndex != UINT32_C(0xFFFFFFFF))
            {
                return VERR_INVALID_PARAMETER;
            }

            /* Special case for blanking using current video mode.
             * Only 'u16Flags' and 'u32ViewIndex' field are relevant.
             */
            RT_ZERO(*pScreen);
            pScreen->u32ViewIndex = u32ViewIndex;
            pScreen->u16Flags = u16Flags;
            return VINF_SUCCESS;
        }

        if (   u32ViewIndex < pVGAState->cMonitors
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
                    && u64ScreenSize <= pVGAState->vram_size
                    && pScreen->u32StartOffset <= pVGAState->vram_size - (uint32_t)u64ScreenSize)
                {
                    return VINF_SUCCESS;
                }
            }
        }
    }

    return VERR_INVALID_PARAMETER;
}

static int vboxVDMACrGuestCtlResizeEntryProcess(struct VBOXVDMAHOST *pVdma, VBOXCMDVBVA_RESIZE_ENTRY *pEntry)
{
    PVGASTATE pVGAState = pVdma->pVGAState;
    VBVAINFOSCREEN Screen = pEntry->Screen;

    /* Verify and cleanup local copy of the input data. */
    int rc = vboxVDMASetupScreenInfo(pVGAState, &Screen);
    if (RT_FAILURE(rc))
    {
        WARN(("invalid screen data\n"));
        return rc;
    }

    VBOXCMDVBVA_SCREENMAP_DECL(uint32_t, aTargetMap);
    memcpy(aTargetMap, pEntry->aTargetMap, sizeof(aTargetMap));
    ASMBitClearRange(aTargetMap, pVGAState->cMonitors, VBOX_VIDEO_MAX_SCREENS);

    rc = pVdma->CrSrvInfo.pfnResize(pVdma->CrSrvInfo.hSvr, &Screen, aTargetMap);
    if (RT_FAILURE(rc))
    {
        WARN(("pfnResize failed %d\n", rc));
        return rc;
    }

    /* A fake view which contains the current screen for the 2D VBVAInfoView. */
    VBVAINFOVIEW View;
    View.u32ViewOffset = 0;
    View.u32ViewSize = Screen.u32LineSize * Screen.u32Height + Screen.u32StartOffset;
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
                WARN(("VBVAInfoView failed %d\n", rc));
                break;
            }
        }

        rc = VBVAInfoScreen(pVGAState, &Screen);
        if (RT_FAILURE(rc))
        {
            WARN(("VBVAInfoScreen failed %d\n", rc));
            break;
        }
    }

    return rc;
}

static int vboxVDMACrGuestCtlProcess(struct VBOXVDMAHOST *pVdma, VBVAEXHOSTCTL *pCmd)
{
    VBVAEXHOSTCTL_TYPE enmType = pCmd->enmType;
    switch (enmType)
    {
        case VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE:
        {
            if (!VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva))
            {
                WARN(("VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE for disabled vdma VBVA\n"));
                return VERR_INVALID_STATE;
            }
            return pVdma->CrSrvInfo.pfnGuestCtl(pVdma->CrSrvInfo.hSvr, pCmd->u.cmd.pu8Cmd, pCmd->u.cmd.cbCmd);
        }
        case VBVAEXHOSTCTL_TYPE_GHH_RESIZE:
        {
            if (!VBoxVBVAExHSIsEnabled(&pVdma->CmdVbva))
            {
                WARN(("VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE for disabled vdma VBVA\n"));
                return VERR_INVALID_STATE;
            }

            uint32_t cbCmd = pCmd->u.cmd.cbCmd;

            if (cbCmd % sizeof (VBOXCMDVBVA_RESIZE_ENTRY))
            {
                WARN(("invalid buffer size\n"));
                return VERR_INVALID_PARAMETER;
            }

            uint32_t cElements = cbCmd / sizeof (VBOXCMDVBVA_RESIZE_ENTRY);
            if (!cElements)
            {
                WARN(("invalid buffer size\n"));
                return VERR_INVALID_PARAMETER;
            }

            VBOXCMDVBVA_RESIZE *pResize = (VBOXCMDVBVA_RESIZE*)pCmd->u.cmd.pu8Cmd;

            int rc = VINF_SUCCESS;

            for (uint32_t i = 0; i < cElements; ++i)
            {
                VBOXCMDVBVA_RESIZE_ENTRY *pEntry = &pResize->aEntries[i];
                rc = vboxVDMACrGuestCtlResizeEntryProcess(pVdma, pEntry);
                if (RT_FAILURE(rc))
                {
                    WARN(("vboxVDMACrGuestCtlResizeEntryProcess failed %d\n", rc));
                    break;
                }
            }
            return rc;
        }
        case VBVAEXHOSTCTL_TYPE_GHH_ENABLE:
        case VBVAEXHOSTCTL_TYPE_GHH_ENABLE_PAUSED:
        {
            VBVAENABLE *pEnable = (VBVAENABLE *)pCmd->u.cmd.pu8Cmd;
            Assert(pCmd->u.cmd.cbCmd == sizeof (VBVAENABLE));
            uint32_t u32Offset = pEnable->u32Offset;
            int rc = vdmaVBVAEnableProcess(pVdma, u32Offset);
            if (!RT_SUCCESS(rc))
            {
                WARN(("vdmaVBVAEnableProcess failed %d\n", rc));
                return rc;
            }

            if (enmType == VBVAEXHOSTCTL_TYPE_GHH_ENABLE_PAUSED)
            {
                rc = VBoxVBVAExHPPause(&pVdma->CmdVbva);
                if (!RT_SUCCESS(rc))
                {
                    WARN(("VBoxVBVAExHPPause failed %d\n", rc));
                    return rc;
                }
            }

            return VINF_SUCCESS;
        }
        case VBVAEXHOSTCTL_TYPE_GHH_DISABLE:
        {
            int rc = vdmaVBVADisableProcess(pVdma, true);
            if (RT_FAILURE(rc))
            {
                WARN(("vdmaVBVADisableProcess failed %d\n", rc));
                return rc;
            }

            /* do vgaUpdateDisplayAll right away */
            VMR3ReqCallNoWait(PDMDevHlpGetVM(pVdma->pVGAState->pDevInsR3), VMCPUID_ANY,
                              (PFNRT)vgaUpdateDisplayAll, 2, pVdma->pVGAState, /* fFailOnResize = */ false);

            return VBoxVDMAThreadTerm(&pVdma->Thread, NULL, NULL, false);
        }
        default:
            WARN(("unexpected ctl type %d\n", pCmd->enmType));
            return VERR_INVALID_PARAMETER;
    }
}

/**
 * @param fIn - whether this is a page in or out op.
 * the direction is VRA#M - related, so fIn == true - transfer to VRAM); false - transfer from VRAM
 */
static int vboxVDMACrCmdVbvaProcessPagingEl(PPDMDEVINS pDevIns, VBOXCMDVBVAPAGEIDX iPage, uint8_t *pu8Vram, bool fIn)
{
    RTGCPHYS phPage = (RTGCPHYS)iPage << PAGE_SHIFT;
    PGMPAGEMAPLOCK Lock;
    int rc;

    if (fIn)
    {
        const void * pvPage;
        rc = PDMDevHlpPhysGCPhys2CCPtrReadOnly(pDevIns, phPage, 0, &pvPage, &Lock);
        if (!RT_SUCCESS(rc))
        {
            WARN(("PDMDevHlpPhysGCPhys2CCPtrReadOnly failed %d", rc));
            return rc;
        }

        memcpy(pu8Vram, pvPage, PAGE_SIZE);

        PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);
    }
    else
    {
        void * pvPage;
        rc = PDMDevHlpPhysGCPhys2CCPtr(pDevIns, phPage, 0, &pvPage, &Lock);
        if (!RT_SUCCESS(rc))
        {
            WARN(("PDMDevHlpPhysGCPhys2CCPtr failed %d", rc));
            return rc;
        }

        memcpy(pvPage, pu8Vram, PAGE_SIZE);

        PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);
    }

    return VINF_SUCCESS;
}

static int vboxVDMACrCmdVbvaProcessPagingEls(PPDMDEVINS pDevIns, const VBOXCMDVBVAPAGEIDX *piPages, uint32_t cPages, uint8_t *pu8Vram, bool fIn)
{
    for (uint32_t i = 0; i < cPages; ++i, pu8Vram += PAGE_SIZE)
    {
        int rc = vboxVDMACrCmdVbvaProcessPagingEl(pDevIns, piPages[i], pu8Vram, fIn);
        if (!RT_SUCCESS(rc))
        {
            WARN(("vboxVDMACrCmdVbvaProcessPagingEl failed %d", rc));
            return rc;
        }
    }

    return VINF_SUCCESS;
}

static int8_t vboxVDMACrCmdVbvaPagingDataInit(PVGASTATE pVGAState, const VBOXCMDVBVA_HDR *pHdr, const VBOXCMDVBVA_PAGING_TRANSFER_DATA *pData, uint32_t cbCmd,
                            const VBOXCMDVBVAPAGEIDX **ppPages, VBOXCMDVBVAPAGEIDX *pcPages,
                            uint8_t **ppu8Vram, bool *pfIn)
{
    if (cbCmd < sizeof (VBOXCMDVBVA_PAGING_TRANSFER))
    {
        WARN(("cmd too small"));
        return -1;
    }

    VBOXCMDVBVAPAGEIDX cPages = cbCmd - RT_OFFSETOF(VBOXCMDVBVA_PAGING_TRANSFER, Data.aPageNumbers);
    if (cPages % sizeof (VBOXCMDVBVAPAGEIDX))
    {
        WARN(("invalid cmd size"));
        return -1;
    }
    cPages /= sizeof (VBOXCMDVBVAPAGEIDX);

    VBOXCMDVBVAOFFSET offVRAM = pData->Alloc.u.offVRAM;
    if (offVRAM & PAGE_OFFSET_MASK)
    {
        WARN(("offVRAM address is not on page boundary\n"));
        return -1;
    }
    const VBOXCMDVBVAPAGEIDX *pPages = pData->aPageNumbers;

    uint8_t * pu8VramBase = pVGAState->vram_ptrR3;
    if (offVRAM >= pVGAState->vram_size)
    {
        WARN(("invalid vram offset"));
        return -1;
    }

    if (~(~(VBOXCMDVBVAPAGEIDX)0 >> PAGE_SHIFT) & cPages)
    {
        WARN(("invalid cPages %d", cPages));
        return -1;
    }

    if (offVRAM + ((VBOXCMDVBVAOFFSET)cPages << PAGE_SHIFT) >= pVGAState->vram_size)
    {
        WARN(("invalid cPages %d, exceeding vram size", cPages));
        return -1;
    }

    uint8_t *pu8Vram = pu8VramBase + offVRAM;
    bool fIn = !!(pHdr->u8Flags & VBOXCMDVBVA_OPF_PAGING_TRANSFER_IN);

    *ppPages = pPages;
    *pcPages = cPages;
    *ppu8Vram = pu8Vram;
    *pfIn = fIn;
    return 0;
}

static int8_t vboxVDMACrCmdVbvaPagingFill(PVGASTATE pVGAState, VBOXCMDVBVA_PAGING_FILL *pFill)
{
    VBOXCMDVBVAOFFSET offVRAM = pFill->offVRAM;
    if (offVRAM & PAGE_OFFSET_MASK)
    {
        WARN(("offVRAM address is not on page boundary\n"));
        return -1;
    }

    uint8_t * pu8VramBase = pVGAState->vram_ptrR3;
    if (offVRAM >= pVGAState->vram_size)
    {
        WARN(("invalid vram offset"));
        return -1;
    }

    uint32_t cbFill = pFill->u32CbFill;

    if (offVRAM + cbFill >= pVGAState->vram_size)
    {
        WARN(("invalid cPages"));
        return -1;
    }

    uint32_t *pu32Vram = (uint32_t*)(pu8VramBase + offVRAM);
    uint32_t u32Color = pFill->u32Pattern;

    Assert(!(cbFill % 4));
    for (uint32_t i = 0; i < cbFill / 4; ++i)
    {
        pu32Vram[i] = u32Color;
    }

    return 0;
}

static int8_t vboxVDMACrCmdVbvaProcessCmdData(struct VBOXVDMAHOST *pVdma, const VBOXCMDVBVA_HDR *pCmd, uint32_t cbCmd)
{
    switch (pCmd->u8OpCode)
    {
        case VBOXCMDVBVA_OPTYPE_NOPCMD:
            return 0;
        case VBOXCMDVBVA_OPTYPE_PAGING_TRANSFER:
        {
            PVGASTATE pVGAState = pVdma->pVGAState;
            const VBOXCMDVBVAPAGEIDX *pPages;
            uint32_t cPages;
            uint8_t *pu8Vram;
            bool fIn;
            int8_t i8Result = vboxVDMACrCmdVbvaPagingDataInit(pVGAState, pCmd, &((VBOXCMDVBVA_PAGING_TRANSFER*)pCmd)->Data, cbCmd,
                                                                &pPages, &cPages,
                                                                &pu8Vram, &fIn);
            if (i8Result < 0)
            {
                WARN(("vboxVDMACrCmdVbvaPagingDataInit failed %d", i8Result));
                return i8Result;
            }

            PPDMDEVINS pDevIns = pVGAState->pDevInsR3;
            int rc = vboxVDMACrCmdVbvaProcessPagingEls(pDevIns, pPages, cPages, pu8Vram, fIn);
            if (!RT_SUCCESS(rc))
            {
                WARN(("vboxVDMACrCmdVbvaProcessPagingEls failed %d", rc));
                return -1;
            }

            return 0;
        }
        case VBOXCMDVBVA_OPTYPE_PAGING_FILL:
        {
            PVGASTATE pVGAState = pVdma->pVGAState;
            if (cbCmd != sizeof (VBOXCMDVBVA_PAGING_FILL))
            {
                WARN(("cmd too small"));
                return -1;
            }

            return vboxVDMACrCmdVbvaPagingFill(pVGAState, (VBOXCMDVBVA_PAGING_FILL*)pCmd);
        }
        default:
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
AssertCompile(!(PAGE_SIZE % sizeof (VBOXCMDVBVAPAGEIDX)));

# define VBOXCMDVBVA_NUM_SYSMEMEL_PER_PAGE (PAGE_SIZE / sizeof (VBOXCMDVBVA_SYSMEMEL))

static int8_t vboxVDMACrCmdVbvaProcess(struct VBOXVDMAHOST *pVdma, const VBOXCMDVBVA_HDR *pCmd, uint32_t cbCmd)
{
    LogRelFlow(("VDMA: vboxVDMACrCmdVbvaProcess: ENTER, opCode(%i)\n", pCmd->u8OpCode));
    int8_t i8Result = 0;

    switch (pCmd->u8OpCode)
    {
        case VBOXCMDVBVA_OPTYPE_SYSMEMCMD:
        {
            if (cbCmd < sizeof (VBOXCMDVBVA_SYSMEMCMD))
            {
                WARN(("invalid command size"));
                return -1;
            }
            VBOXCMDVBVA_SYSMEMCMD *pSysmemCmd = (VBOXCMDVBVA_SYSMEMCMD*)pCmd;
            const VBOXCMDVBVA_HDR *pRealCmdHdr;
            uint32_t cbRealCmd = pCmd->u8Flags;
            cbRealCmd |= (uint32_t)pCmd->u.u8PrimaryID << 8;
            if (cbRealCmd < sizeof (VBOXCMDVBVA_HDR))
            {
                WARN(("invalid sysmem cmd size"));
                return -1;
            }

            RTGCPHYS phCmd = (RTGCPHYS)pSysmemCmd->phCmd;

            PGMPAGEMAPLOCK Lock;
            PVGASTATE pVGAState = pVdma->pVGAState;
            PPDMDEVINS pDevIns = pVGAState->pDevInsR3;
            const void * pvCmd;
            int rc = PDMDevHlpPhysGCPhys2CCPtrReadOnly(pDevIns, phCmd, 0, &pvCmd, &Lock);
            if (!RT_SUCCESS(rc))
            {
                WARN(("PDMDevHlpPhysGCPhys2CCPtrReadOnly failed %d\n", rc));
                return -1;
            }

            Assert((phCmd & PAGE_OFFSET_MASK) == (((uintptr_t)pvCmd) & PAGE_OFFSET_MASK));

            uint32_t cbCmdPart = PAGE_SIZE - (((uintptr_t)pvCmd) & PAGE_OFFSET_MASK);

            if (cbRealCmd <= cbCmdPart)
            {
                pRealCmdHdr = (const VBOXCMDVBVA_HDR *)pvCmd;
                i8Result = vboxVDMACrCmdVbvaProcessCmdData(pVdma, pRealCmdHdr, cbRealCmd);
                PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);
                return i8Result;
            }

            VBOXCMDVBVA_HDR Hdr;
            const void *pvCurCmdTail;
            uint32_t cbCurCmdTail;
            if (cbCmdPart >= sizeof (*pRealCmdHdr))
            {
                pRealCmdHdr = (const VBOXCMDVBVA_HDR *)pvCmd;
                pvCurCmdTail = (const void*)(pRealCmdHdr + 1);
                cbCurCmdTail = cbCmdPart - sizeof (*pRealCmdHdr);
            }
            else
            {
                memcpy(&Hdr, pvCmd, cbCmdPart);
                PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);
                phCmd += cbCmdPart;
                Assert(!(phCmd & PAGE_OFFSET_MASK));
                rc = PDMDevHlpPhysGCPhys2CCPtrReadOnly(pDevIns, phCmd, 0, &pvCmd, &Lock);
                if (!RT_SUCCESS(rc))
                {
                    WARN(("PDMDevHlpPhysGCPhys2CCPtrReadOnly failed %d\n", rc));
                    return -1;
                }

                cbCmdPart = sizeof (*pRealCmdHdr) - cbCmdPart;
                memcpy(((uint8_t*)(&Hdr)) + cbCmdPart, pvCmd, cbCmdPart);
                pRealCmdHdr = &Hdr;
                pvCurCmdTail = (const void*)(((uint8_t*)pvCmd) + cbCmdPart);
                cbCurCmdTail = PAGE_SIZE - cbCmdPart;
            }

            if (cbCurCmdTail > cbRealCmd - sizeof (*pRealCmdHdr))
                cbCurCmdTail = cbRealCmd - sizeof (*pRealCmdHdr);

            switch (pRealCmdHdr->u8OpCode)
            {
                case VBOXCMDVBVA_OPTYPE_PAGING_TRANSFER:
                {
                    const uint32_t *pPages;
                    uint32_t cPages;
                    uint8_t *pu8Vram;
                    bool fIn;
                    i8Result = vboxVDMACrCmdVbvaPagingDataInit(pVGAState, pRealCmdHdr, (const VBOXCMDVBVA_PAGING_TRANSFER_DATA*)pvCurCmdTail, cbRealCmd,
                                                                        &pPages, &cPages,
                                                                        &pu8Vram, &fIn);
                    if (i8Result < 0)
                    {
                        WARN(("vboxVDMACrCmdVbvaPagingDataInit failed %d", i8Result));
                        /* we need to break, not return, to ensure currently locked page is released */
                        break;
                    }

                    if (cbCurCmdTail & 3)
                    {
                        WARN(("command is not alligned properly %d", cbCurCmdTail));
                        i8Result = -1;
                        /* we need to break, not return, to ensure currently locked page is released */
                        break;
                    }

                    uint32_t cCurPages = cbCurCmdTail / sizeof (VBOXCMDVBVAPAGEIDX);
                    Assert(cCurPages < cPages);

                    do
                    {
                        rc = vboxVDMACrCmdVbvaProcessPagingEls(pDevIns, pPages, cCurPages, pu8Vram, fIn);
                        if (!RT_SUCCESS(rc))
                        {
                            WARN(("vboxVDMACrCmdVbvaProcessPagingEls failed %d", rc));
                            i8Result = -1;
                            /* we need to break, not return, to ensure currently locked page is released */
                            break;
                        }

                        Assert(cPages >= cCurPages);
                        cPages -= cCurPages;

                        if (!cPages)
                            break;

                        PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);

                        Assert(!(phCmd & PAGE_OFFSET_MASK));

                        phCmd += PAGE_SIZE;
                        pu8Vram += (VBOXCMDVBVAOFFSET)cCurPages << PAGE_SHIFT;

                        rc = PDMDevHlpPhysGCPhys2CCPtrReadOnly(pDevIns, phCmd, 0, &pvCmd, &Lock);
                        if (!RT_SUCCESS(rc))
                        {
                            WARN(("PDMDevHlpPhysGCPhys2CCPtrReadOnly failed %d\n", rc));
                            /* the page is not locked, return */
                            return -1;
                        }

                        cCurPages = PAGE_SIZE / sizeof (VBOXCMDVBVAPAGEIDX);
                        if (cCurPages > cPages)
                            cCurPages = cPages;
                    } while (1);
                    break;
                }
                default:
                    WARN(("command can not be splitted"));
                    i8Result = -1;
                    break;
            }

            PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);
            return i8Result;
        }
        case VBOXCMDVBVA_OPTYPE_COMPLEXCMD:
        {
            Assert(cbCmd >= sizeof (VBOXCMDVBVA_HDR));
            ++pCmd;
            cbCmd -= sizeof (*pCmd);
            uint32_t cbCurCmd = 0;
            for ( ; cbCmd; cbCmd -= cbCurCmd, pCmd = (VBOXCMDVBVA_HDR*)(((uint8_t*)pCmd) + cbCurCmd))
            {
                if (cbCmd < sizeof (VBOXCMDVBVA_HDR))
                {
                    WARN(("invalid command size"));
                    return -1;
                }

                cbCurCmd = pCmd->u2.complexCmdEl.u16CbCmdHost;
                if (cbCmd < cbCurCmd)
                {
                    WARN(("invalid command size"));
                    return -1;
                }

                i8Result = vboxVDMACrCmdVbvaProcess(pVdma, pCmd, cbCurCmd);
                if (i8Result < 0)
                {
                    WARN(("vboxVDMACrCmdVbvaProcess failed"));
                    return i8Result;
                }
            }
            return 0;
        }
        default:
            i8Result = vboxVDMACrCmdVbvaProcessCmdData(pVdma, pCmd, cbCmd);
            LogRelFlow(("VDMA: vboxVDMACrCmdVbvaProcess: LEAVE, opCode(%i)\n", pCmd->u8OpCode));
            return i8Result;
    }
}

static void vboxVDMACrCmdProcess(struct VBOXVDMAHOST *pVdma, uint8_t* pu8Cmd, uint32_t cbCmd)
{
    if (*pu8Cmd == VBOXCMDVBVA_OPTYPE_NOP)
        return;

    if (cbCmd < sizeof (VBOXCMDVBVA_HDR))
    {
        WARN(("invalid command size"));
        return;
    }

    PVBOXCMDVBVA_HDR pCmd = (PVBOXCMDVBVA_HDR)pu8Cmd;

    /* check if the command is cancelled */
    if (!ASMAtomicCmpXchgU8(&pCmd->u8State, VBOXCMDVBVA_STATE_IN_PROGRESS, VBOXCMDVBVA_STATE_SUBMITTED))
    {
        Assert(pCmd->u8State == VBOXCMDVBVA_STATE_CANCELLED);
        return;
    }

    pCmd->u.i8Result = vboxVDMACrCmdVbvaProcess(pVdma, pCmd, cbCmd);
}

static int vboxVDMACrCtlHgsmiSetup(struct VBOXVDMAHOST *pVdma)
{
    PVBOXVDMACMD_CHROMIUM_CTL_CRHGSMI_SETUP pCmd = (PVBOXVDMACMD_CHROMIUM_CTL_CRHGSMI_SETUP)
            vboxVDMACrCtlCreate (VBOXVDMACMD_CHROMIUM_CTL_TYPE_CRHGSMI_SETUP, sizeof (*pCmd));
    int rc = VERR_NO_MEMORY;
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
                WARN(("vboxVDMACrCtlGetRc returned %d\n", rc));
        }
        else
            WARN(("vboxVDMACrCtlPost failed %d\n", rc));

        vboxVDMACrCtlRelease(&pCmd->Hdr);
    }

    if (!RT_SUCCESS(rc))
        memset(&pVdma->CrSrvInfo, 0, sizeof (pVdma->CrSrvInfo));

    return rc;
}

static int vboxVDMACmdExecBpbTransfer(PVBOXVDMAHOST pVdma, const PVBOXVDMACMD_DMA_BPB_TRANSFER pTransfer, uint32_t cbBuffer);

/* check if this is external cmd to be passed to chromium backend */
static int vboxVDMACmdCheckCrCmd(struct VBOXVDMAHOST *pVdma, PVBOXVDMACBUF_DR pCmdDr, uint32_t cbCmdDr)
{
    PVBOXVDMACMD pDmaCmd = NULL;
    uint32_t cbDmaCmd = 0;
    uint8_t * pvRam = pVdma->pVGAState->vram_ptrR3;
    int rc = VINF_NOT_SUPPORTED;

    cbDmaCmd = pCmdDr->cbBuf;

    if (pCmdDr->fFlags & VBOXVDMACBUF_FLAG_BUF_FOLLOWS_DR)
    {
        if (cbCmdDr < sizeof (*pCmdDr) + VBOXVDMACMD_HEADER_SIZE())
        {
            AssertMsgFailed(("invalid buffer data!"));
            return VERR_INVALID_PARAMETER;
        }

        if (cbDmaCmd < cbCmdDr - sizeof (*pCmdDr) - VBOXVDMACMD_HEADER_SIZE())
        {
            AssertMsgFailed(("invalid command buffer data!"));
            return VERR_INVALID_PARAMETER;
        }

        pDmaCmd = VBOXVDMACBUF_DR_TAIL(pCmdDr, VBOXVDMACMD);
    }
    else if (pCmdDr->fFlags & VBOXVDMACBUF_FLAG_BUF_VRAM_OFFSET)
    {
        VBOXVIDEOOFFSET offBuf = pCmdDr->Location.offVramBuf;
        if (offBuf + cbDmaCmd > pVdma->pVGAState->vram_size)
        {
            AssertMsgFailed(("invalid command buffer data from offset!"));
            return VERR_INVALID_PARAMETER;
        }
        pDmaCmd = (VBOXVDMACMD*)(pvRam + offBuf);
    }

    if (pDmaCmd)
    {
        Assert(cbDmaCmd >= VBOXVDMACMD_HEADER_SIZE());
        uint32_t cbBody = VBOXVDMACMD_BODY_SIZE(cbDmaCmd);

        switch (pDmaCmd->enmType)
        {
            case VBOXVDMACMD_TYPE_CHROMIUM_CMD:
            {
                PVBOXVDMACMD_CHROMIUM_CMD pCrCmd = VBOXVDMACMD_BODY(pDmaCmd, VBOXVDMACMD_CHROMIUM_CMD);
                if (cbBody < sizeof (*pCrCmd))
                {
                    AssertMsgFailed(("invalid chromium command buffer size!"));
                    return VERR_INVALID_PARAMETER;
                }
                PVGASTATE pVGAState = pVdma->pVGAState;
                rc = VINF_SUCCESS;
                if (pVGAState->pDrv->pfnCrHgsmiCommandProcess)
                {
                    VBoxSHGSMICommandMarkAsynchCompletion(pCmdDr);
                    pVGAState->pDrv->pfnCrHgsmiCommandProcess(pVGAState->pDrv, pCrCmd, cbBody);
                    break;
                }
                else
                {
                    Assert(0);
                }

                int tmpRc = VBoxSHGSMICommandComplete (pVdma->pHgsmi, pCmdDr);
                AssertRC(tmpRc);
                break;
            }
            case VBOXVDMACMD_TYPE_DMA_BPB_TRANSFER:
            {
                PVBOXVDMACMD_DMA_BPB_TRANSFER pTransfer = VBOXVDMACMD_BODY(pDmaCmd, VBOXVDMACMD_DMA_BPB_TRANSFER);
                if (cbBody < sizeof (*pTransfer))
                {
                    AssertMsgFailed(("invalid bpb transfer buffer size!"));
                    return VERR_INVALID_PARAMETER;
                }

                rc = vboxVDMACmdExecBpbTransfer(pVdma, pTransfer, sizeof (*pTransfer));
                AssertRC(rc);
                if (RT_SUCCESS(rc))
                {
                    pCmdDr->rc = VINF_SUCCESS;
                    rc = VBoxSHGSMICommandComplete (pVdma->pHgsmi, pCmdDr);
                    AssertRC(rc);
                    rc = VINF_SUCCESS;
                }
                break;
            }
            default:
                break;
        }
    }
    return rc;
}

int vboxVDMACrHgsmiCommandCompleteAsync(PPDMIDISPLAYVBVACALLBACKS pInterface, PVBOXVDMACMD_CHROMIUM_CMD pCmd, int rc)
{
    PVGASTATE pVGAState = PPDMIDISPLAYVBVACALLBACKS_2_PVGASTATE(pInterface);
    PHGSMIINSTANCE pIns = pVGAState->pHGSMI;
    VBOXVDMACMD *pDmaHdr = VBOXVDMACMD_FROM_BODY(pCmd);
    VBOXVDMACBUF_DR *pDr = VBOXVDMACBUF_DR_FROM_TAIL(pDmaHdr);
    AssertRC(rc);
    pDr->rc = rc;

    Assert(pVGAState->fGuestCaps & VBVACAPS_COMPLETEGCMD_BY_IOREAD);
    rc = VBoxSHGSMICommandComplete(pIns, pDr);
    AssertRC(rc);
    return rc;
}

int vboxVDMACrHgsmiControlCompleteAsync(PPDMIDISPLAYVBVACALLBACKS pInterface, PVBOXVDMACMD_CHROMIUM_CTL pCmd, int rc)
{
    PVGASTATE pVGAState = PPDMIDISPLAYVBVACALLBACKS_2_PVGASTATE(pInterface);
    PVBOXVDMACMD_CHROMIUM_CTL_PRIVATE pCmdPrivate = VBOXVDMACMD_CHROMIUM_CTL_PRIVATE_FROM_CTL(pCmd);
    pCmdPrivate->rc = rc;
    if (pCmdPrivate->pfnCompletion)
    {
        pCmdPrivate->pfnCompletion(pVGAState, pCmd, pCmdPrivate->pvCompletion);
    }
    return VINF_SUCCESS;
}

static int vboxVDMACmdExecBltPerform(PVBOXVDMAHOST pVdma, uint8_t *pvDstSurf, const uint8_t *pvSrcSurf,
                                     const PVBOXVDMA_SURF_DESC pDstDesc, const PVBOXVDMA_SURF_DESC pSrcDesc,
                                     const VBOXVDMA_RECTL * pDstRectl, const VBOXVDMA_RECTL * pSrcRectl)
{
    RT_NOREF(pVdma);
    /* we do not support color conversion */
    Assert(pDstDesc->format == pSrcDesc->format);
    /* we do not support stretching */
    Assert(pDstRectl->height == pSrcRectl->height);
    Assert(pDstRectl->width == pSrcRectl->width);
    if (pDstDesc->format != pSrcDesc->format)
        return VERR_INVALID_FUNCTION;
    if (pDstDesc->width == pDstRectl->width
            && pSrcDesc->width == pSrcRectl->width
            && pSrcDesc->width == pDstDesc->width)
    {
        Assert(!pDstRectl->left);
        Assert(!pSrcRectl->left);
        uint32_t cbOff = pDstDesc->pitch * pDstRectl->top;
        uint32_t cbSize = pDstDesc->pitch * pDstRectl->height;
        memcpy(pvDstSurf + cbOff, pvSrcSurf + cbOff, cbSize);
    }
    else
    {
        uint32_t offDstLineStart = pDstRectl->left * pDstDesc->bpp >> 3;
        uint32_t offDstLineEnd = ((pDstRectl->left * pDstDesc->bpp + 7) >> 3) + ((pDstDesc->bpp * pDstRectl->width + 7) >> 3);
        uint32_t cbDstLine = offDstLineEnd - offDstLineStart;
        uint32_t offDstStart = pDstDesc->pitch * pDstRectl->top + offDstLineStart;
        Assert(cbDstLine <= pDstDesc->pitch);
        uint32_t cbDstSkip = pDstDesc->pitch;
        uint8_t * pvDstStart = pvDstSurf + offDstStart;

        uint32_t offSrcLineStart = pSrcRectl->left * pSrcDesc->bpp >> 3;
# ifdef VBOX_STRICT
        uint32_t offSrcLineEnd = ((pSrcRectl->left * pSrcDesc->bpp + 7) >> 3) + ((pSrcDesc->bpp * pSrcRectl->width + 7) >> 3);
        uint32_t cbSrcLine = offSrcLineEnd - offSrcLineStart;
# endif
        uint32_t offSrcStart = pSrcDesc->pitch * pSrcRectl->top + offSrcLineStart;
        Assert(cbSrcLine <= pSrcDesc->pitch);
        uint32_t cbSrcSkip = pSrcDesc->pitch;
        const uint8_t * pvSrcStart = pvSrcSurf + offSrcStart;

        Assert(cbDstLine == cbSrcLine);

        for (uint32_t i = 0; ; ++i)
        {
            memcpy (pvDstStart, pvSrcStart, cbDstLine);
            if (i == pDstRectl->height)
                break;
            pvDstStart += cbDstSkip;
            pvSrcStart += cbSrcSkip;
        }
    }
    return VINF_SUCCESS;
}

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

/*
 * @return on success the number of bytes the command contained, otherwise - VERR_xxx error code
 */
static int vboxVDMACmdExecBlt(PVBOXVDMAHOST pVdma, const PVBOXVDMACMD_DMA_PRESENT_BLT pBlt, uint32_t cbBuffer)
{
    const uint32_t cbBlt = VBOXVDMACMD_BODY_FIELD_OFFSET(uint32_t, VBOXVDMACMD_DMA_PRESENT_BLT, aDstSubRects[pBlt->cDstSubRects]);
    Assert(cbBlt <= cbBuffer);
    if (cbBuffer < cbBlt)
        return VERR_INVALID_FUNCTION;

    /* we do not support stretching for now */
    Assert(pBlt->srcRectl.width == pBlt->dstRectl.width);
    Assert(pBlt->srcRectl.height == pBlt->dstRectl.height);
    if (pBlt->srcRectl.width != pBlt->dstRectl.width)
        return VERR_INVALID_FUNCTION;
    if (pBlt->srcRectl.height != pBlt->dstRectl.height)
        return VERR_INVALID_FUNCTION;
    Assert(pBlt->cDstSubRects);

    uint8_t * pvRam = pVdma->pVGAState->vram_ptrR3;
    VBOXVDMA_RECTL updateRectl = {0, 0, 0, 0};

    if (pBlt->cDstSubRects)
    {
        VBOXVDMA_RECTL dstRectl, srcRectl;
        const VBOXVDMA_RECTL *pDstRectl, *pSrcRectl;
        for (uint32_t i = 0; i < pBlt->cDstSubRects; ++i)
        {
            pDstRectl = &pBlt->aDstSubRects[i];
            if (pBlt->dstRectl.left || pBlt->dstRectl.top)
            {
                dstRectl.left = pDstRectl->left + pBlt->dstRectl.left;
                dstRectl.top = pDstRectl->top + pBlt->dstRectl.top;
                dstRectl.width = pDstRectl->width;
                dstRectl.height = pDstRectl->height;
                pDstRectl = &dstRectl;
            }

            pSrcRectl = &pBlt->aDstSubRects[i];
            if (pBlt->srcRectl.left || pBlt->srcRectl.top)
            {
                srcRectl.left = pSrcRectl->left + pBlt->srcRectl.left;
                srcRectl.top = pSrcRectl->top + pBlt->srcRectl.top;
                srcRectl.width = pSrcRectl->width;
                srcRectl.height = pSrcRectl->height;
                pSrcRectl = &srcRectl;
            }

            int rc = vboxVDMACmdExecBltPerform(pVdma, pvRam + pBlt->offDst, pvRam + pBlt->offSrc,
                    &pBlt->dstDesc, &pBlt->srcDesc,
                    pDstRectl,
                    pSrcRectl);
            AssertRC(rc);
            if (!RT_SUCCESS(rc))
                return rc;

            vboxVDMARectlUnite(&updateRectl, pDstRectl);
        }
    }
    else
    {
        int rc = vboxVDMACmdExecBltPerform(pVdma, pvRam + pBlt->offDst, pvRam + pBlt->offSrc,
                &pBlt->dstDesc, &pBlt->srcDesc,
                &pBlt->dstRectl,
                &pBlt->srcRectl);
        AssertRC(rc);
        if (!RT_SUCCESS(rc))
            return rc;

        vboxVDMARectlUnite(&updateRectl, &pBlt->dstRectl);
    }

    return cbBlt;
}

static int vboxVDMACmdExecBpbTransfer(PVBOXVDMAHOST pVdma, const PVBOXVDMACMD_DMA_BPB_TRANSFER pTransfer, uint32_t cbBuffer)
{
    if (cbBuffer < sizeof (*pTransfer))
        return VERR_INVALID_PARAMETER;

    PVGASTATE pVGAState = pVdma->pVGAState;
    uint8_t * pvRam = pVGAState->vram_ptrR3;
    PGMPAGEMAPLOCK SrcLock;
    PGMPAGEMAPLOCK DstLock;
    PPDMDEVINS pDevIns = pVdma->pVGAState->pDevInsR3;
    const void * pvSrc;
    void * pvDst;
    int rc = VINF_SUCCESS;
    uint32_t cbTransfer = pTransfer->cbTransferSize;
    uint32_t cbTransfered = 0;
    bool bSrcLocked = false;
    bool bDstLocked = false;
    do
    {
        uint32_t cbSubTransfer = cbTransfer;
        if (pTransfer->fFlags & VBOXVDMACMD_DMA_BPB_TRANSFER_F_SRC_VRAMOFFSET)
        {
            pvSrc  = pvRam + pTransfer->Src.offVramBuf + cbTransfered;
        }
        else
        {
            RTGCPHYS phPage = pTransfer->Src.phBuf;
            phPage += cbTransfered;
            rc = PDMDevHlpPhysGCPhys2CCPtrReadOnly(pDevIns, phPage, 0, &pvSrc, &SrcLock);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                bSrcLocked = true;
                cbSubTransfer = RT_MIN(cbSubTransfer, 0x1000);
            }
            else
            {
                break;
            }
        }

        if (pTransfer->fFlags & VBOXVDMACMD_DMA_BPB_TRANSFER_F_DST_VRAMOFFSET)
        {
            pvDst  = pvRam + pTransfer->Dst.offVramBuf + cbTransfered;
        }
        else
        {
            RTGCPHYS phPage = pTransfer->Dst.phBuf;
            phPage += cbTransfered;
            rc = PDMDevHlpPhysGCPhys2CCPtr(pDevIns, phPage, 0, &pvDst, &DstLock);
            AssertRC(rc);
            if (RT_SUCCESS(rc))
            {
                bDstLocked = true;
                cbSubTransfer = RT_MIN(cbSubTransfer, 0x1000);
            }
            else
            {
                break;
            }
        }

        if (RT_SUCCESS(rc))
        {
            memcpy(pvDst, pvSrc, cbSubTransfer);
            cbTransfer -= cbSubTransfer;
            cbTransfered += cbSubTransfer;
        }
        else
        {
            cbTransfer = 0; /* to break */
        }

        if (bSrcLocked)
            PDMDevHlpPhysReleasePageMappingLock(pDevIns, &SrcLock);
        if (bDstLocked)
            PDMDevHlpPhysReleasePageMappingLock(pDevIns, &DstLock);
    } while (cbTransfer);

    if (RT_SUCCESS(rc))
        return sizeof (*pTransfer);
    return rc;
}

static int vboxVDMACmdExec(PVBOXVDMAHOST pVdma, const uint8_t *pvBuffer, uint32_t cbBuffer)
{
    do
    {
        Assert(pvBuffer);
        Assert(cbBuffer >= VBOXVDMACMD_HEADER_SIZE());

        if (!pvBuffer)
            return VERR_INVALID_PARAMETER;
        if (cbBuffer < VBOXVDMACMD_HEADER_SIZE())
            return VERR_INVALID_PARAMETER;

        PVBOXVDMACMD pCmd = (PVBOXVDMACMD)pvBuffer;
        switch (pCmd->enmType)
        {
            case VBOXVDMACMD_TYPE_CHROMIUM_CMD:
            {
# ifdef VBOXWDDM_TEST_UHGSMI
                static int count = 0;
                static uint64_t start, end;
                if (count==0)
                {
                    start = RTTimeNanoTS();
                }
                ++count;
                if (count==100000)
                {
                    end = RTTimeNanoTS();
                    float ems = (end-start)/1000000.f;
                    LogRel(("100000 calls took %i ms, %i cps\n", (int)ems, (int)(100000.f*1000.f/ems) ));
                }
# endif
                /** @todo post the buffer to chromium */
                return VINF_SUCCESS;
            }
            case VBOXVDMACMD_TYPE_DMA_PRESENT_BLT:
            {
                const PVBOXVDMACMD_DMA_PRESENT_BLT pBlt = VBOXVDMACMD_BODY(pCmd, VBOXVDMACMD_DMA_PRESENT_BLT);
                int cbBlt = vboxVDMACmdExecBlt(pVdma, pBlt, cbBuffer);
                Assert(cbBlt >= 0);
                Assert((uint32_t)cbBlt <= cbBuffer);
                if (cbBlt >= 0)
                {
                    if ((uint32_t)cbBlt == cbBuffer)
                        return VINF_SUCCESS;
                    else
                    {
                        cbBuffer -= (uint32_t)cbBlt;
                        pvBuffer -= cbBlt;
                    }
                }
                else
                    return cbBlt; /* error */
                break;
            }
            case VBOXVDMACMD_TYPE_DMA_BPB_TRANSFER:
            {
                const PVBOXVDMACMD_DMA_BPB_TRANSFER pTransfer = VBOXVDMACMD_BODY(pCmd, VBOXVDMACMD_DMA_BPB_TRANSFER);
                int cbTransfer = vboxVDMACmdExecBpbTransfer(pVdma, pTransfer, cbBuffer);
                Assert(cbTransfer >= 0);
                Assert((uint32_t)cbTransfer <= cbBuffer);
                if (cbTransfer >= 0)
                {
                    if ((uint32_t)cbTransfer == cbBuffer)
                        return VINF_SUCCESS;
                    else
                    {
                        cbBuffer -= (uint32_t)cbTransfer;
                        pvBuffer -= cbTransfer;
                    }
                }
                else
                    return cbTransfer; /* error */
                break;
            }
            case VBOXVDMACMD_TYPE_DMA_NOP:
                return VINF_SUCCESS;
            case VBOXVDMACMD_TYPE_CHILD_STATUS_IRQ:
                return VINF_SUCCESS;
            default:
                AssertBreakpoint();
                return VERR_INVALID_FUNCTION;
        }
    } while (1);

    /* we should not be here */
    AssertBreakpoint();
    return VERR_INVALID_STATE;
}

static DECLCALLBACK(int) vboxVDMAWorkerThread(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF(hThreadSelf);
    PVBOXVDMAHOST pVdma = (PVBOXVDMAHOST)pvUser;
    PVGASTATE pVGAState = pVdma->pVGAState;
    VBVAEXHOSTCONTEXT *pCmdVbva = &pVdma->CmdVbva;
    uint8_t *pCmd;
    uint32_t cbCmd;
    int rc;

    VBoxVDMAThreadNotifyConstructSucceeded(&pVdma->Thread, pvUser);

    while (!VBoxVDMAThreadIsTerminating(&pVdma->Thread))
    {
        VBVAEXHOST_DATA_TYPE enmType = VBoxVBVAExHPDataGet(pCmdVbva, &pCmd, &cbCmd);
        switch (enmType)
        {
            case VBVAEXHOST_DATA_TYPE_CMD:
                vboxVDMACrCmdProcess(pVdma, pCmd, cbCmd);
                VBoxVBVAExHPDataCompleteCmd(pCmdVbva, cbCmd);
                VBVARaiseIrq(pVGAState, 0);
                break;
            case VBVAEXHOST_DATA_TYPE_GUESTCTL:
                rc = vboxVDMACrGuestCtlProcess(pVdma, (VBVAEXHOSTCTL*)pCmd);
                VBoxVBVAExHPDataCompleteCtl(pCmdVbva, (VBVAEXHOSTCTL*)pCmd, rc);
                break;
            case VBVAEXHOST_DATA_TYPE_HOSTCTL:
            {
                bool fContinue = true;
                rc = vboxVDMACrHostCtlProcess(pVdma, (VBVAEXHOSTCTL*)pCmd, &fContinue);
                VBoxVBVAExHPDataCompleteCtl(pCmdVbva, (VBVAEXHOSTCTL*)pCmd, rc);
                if (fContinue)
                    break;
            }
            RT_FALL_THRU();
            case VBVAEXHOST_DATA_TYPE_NO_DATA:
                rc = VBoxVDMAThreadEventWait(&pVdma->Thread, RT_INDEFINITE_WAIT);
                AssertRC(rc);
                break;
            default:
                WARN(("unexpected type %d\n", enmType));
                break;
        }
    }

    VBoxVDMAThreadNotifyTerminatingSucceeded(&pVdma->Thread, pvUser);

    return VINF_SUCCESS;
}

static void vboxVDMACommandProcess(PVBOXVDMAHOST pVdma, PVBOXVDMACBUF_DR pCmd, uint32_t cbCmd)
{
    RT_NOREF(cbCmd);
    PHGSMIINSTANCE pHgsmi = pVdma->pHgsmi;
    const uint8_t * pvBuf;
    PGMPAGEMAPLOCK Lock;
    int rc;
    bool bReleaseLocked = false;

    do
    {
        PPDMDEVINS pDevIns = pVdma->pVGAState->pDevInsR3;

        if (pCmd->fFlags & VBOXVDMACBUF_FLAG_BUF_FOLLOWS_DR)
            pvBuf = VBOXVDMACBUF_DR_TAIL(pCmd, const uint8_t);
        else if (pCmd->fFlags & VBOXVDMACBUF_FLAG_BUF_VRAM_OFFSET)
        {
            uint8_t * pvRam = pVdma->pVGAState->vram_ptrR3;
            pvBuf = pvRam + pCmd->Location.offVramBuf;
        }
        else
        {
            RTGCPHYS phPage = pCmd->Location.phBuf & ~0xfffULL;
            uint32_t offset = pCmd->Location.phBuf & 0xfff;
            Assert(offset + pCmd->cbBuf <= 0x1000);
            if (offset + pCmd->cbBuf > 0x1000)
            {
                /** @todo more advanced mechanism of command buffer proc is actually needed */
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            const void * pvPageBuf;
            rc = PDMDevHlpPhysGCPhys2CCPtrReadOnly(pDevIns, phPage, 0, &pvPageBuf, &Lock);
            AssertRC(rc);
            if (!RT_SUCCESS(rc))
            {
                /** @todo if (rc == VERR_PGM_PHYS_PAGE_RESERVED) -> fall back on using PGMPhysRead ?? */
                break;
            }

            pvBuf = (const uint8_t *)pvPageBuf;
            pvBuf += offset;

            bReleaseLocked = true;
        }

        rc = vboxVDMACmdExec(pVdma, pvBuf, pCmd->cbBuf);
        AssertRC(rc);

        if (bReleaseLocked)
            PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);
    } while (0);

    pCmd->rc = rc;

    rc = VBoxSHGSMICommandComplete (pHgsmi, pCmd);
    AssertRC(rc);
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

static DECLCALLBACK(void) vboxVDMAWatchDogTimer(PPDMDEVINS pDevIns, PTMTIMER pTimer, void *pvUser)
{
    VBOXVDMAHOST *pVdma = (VBOXVDMAHOST *)pvUser;
    PVGASTATE pVGAState = pVdma->pVGAState;
    VBVARaiseIrq(pVGAState, HGSMIHOSTFLAGS_WATCHDOG);
}

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

int vboxVDMAConstruct(PVGASTATE pVGAState, uint32_t cPipeElements)
{
    RT_NOREF(cPipeElements);
    int rc;
    PVBOXVDMAHOST pVdma = (PVBOXVDMAHOST)RTMemAllocZ(sizeof(*pVdma));
    Assert(pVdma);
    if (pVdma)
    {
        pVdma->pHgsmi = pVGAState->pHGSMI;
        pVdma->pVGAState = pVGAState;

#ifdef VBOX_VDMA_WITH_WATCHDOG
        rc = PDMDevHlpTMTimerCreate(pVGAState->pDevInsR3, TMCLOCK_REAL, vboxVDMAWatchDogTimer,
                                        pVdma, TMTIMER_FLAGS_NO_CRIT_SECT,
                                        "VDMA WatchDog Timer", &pVdma->WatchDogTimer);
        AssertRC(rc);
#endif

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
                    pVGAState->pVdma = pVdma;
                    int rcIgnored = vboxVDMACrCtlHgsmiSetup(pVdma); NOREF(rcIgnored); /** @todo is this ignoring intentional? */
                    return VINF_SUCCESS;
                }
                WARN(("RTCritSectInit failed %d\n", rc));

                VBoxVBVAExHSTerm(&pVdma->CmdVbva);
            }
            else
                WARN(("VBoxVBVAExHSInit failed %d\n", rc));

            RTSemEventMultiDestroy(pVdma->HostCrCtlCompleteEvent);
        }
        else
            WARN(("RTSemEventMultiCreate failed %d\n", rc));


        RTMemFree(pVdma);
#else
        pVGAState->pVdma = pVdma;
        return VINF_SUCCESS;
#endif
    }
    else
        rc = VERR_OUT_OF_RESOURCES;

    return rc;
}

int vboxVDMAReset(struct VBOXVDMAHOST *pVdma)
{
#ifdef VBOX_WITH_CRHGSMI
    vdmaVBVACtlDisableSync(pVdma);
#else
    RT_NOREF(pVdma);
#endif
    return VINF_SUCCESS;
}

int vboxVDMADestruct(struct VBOXVDMAHOST *pVdma)
{
    if (!pVdma)
        return VINF_SUCCESS;
#ifdef VBOX_WITH_CRHGSMI
    vdmaVBVACtlDisableSync(pVdma);
    VBoxVDMAThreadCleanup(&pVdma->Thread);
    VBoxVBVAExHSTerm(&pVdma->CmdVbva);
    RTSemEventMultiDestroy(pVdma->HostCrCtlCompleteEvent);
    RTCritSectDelete(&pVdma->CalloutCritSect);
#endif
    RTMemFree(pVdma);
    return VINF_SUCCESS;
}

void vboxVDMAControl(struct VBOXVDMAHOST *pVdma, PVBOXVDMA_CTL pCmd, uint32_t cbCmd)
{
    RT_NOREF(cbCmd);
    PHGSMIINSTANCE pIns = pVdma->pHgsmi;

    switch (pCmd->enmCtl)
    {
        case VBOXVDMA_CTL_TYPE_ENABLE:
            pCmd->i32Result = VINF_SUCCESS;
            break;
        case VBOXVDMA_CTL_TYPE_DISABLE:
            pCmd->i32Result = VINF_SUCCESS;
            break;
        case VBOXVDMA_CTL_TYPE_FLUSH:
            pCmd->i32Result = VINF_SUCCESS;
            break;
#ifdef VBOX_VDMA_WITH_WATCHDOG
        case VBOXVDMA_CTL_TYPE_WATCHDOG:
            pCmd->i32Result = vboxVDMAWatchDogCtl(pVdma, pCmd->u32Offset);
            break;
#endif
        default:
            WARN(("cmd not supported"));
            pCmd->i32Result = VERR_NOT_SUPPORTED;
    }

    int rc = VBoxSHGSMICommandComplete (pIns, pCmd);
    AssertRC(rc);
}

void vboxVDMACommand(struct VBOXVDMAHOST *pVdma, PVBOXVDMACBUF_DR pCmd, uint32_t cbCmd)
{
    int rc = VERR_NOT_IMPLEMENTED;

#ifdef VBOX_WITH_CRHGSMI
    /* chromium commands are processed by crhomium hgcm thread independently from our internal cmd processing pipeline
     * this is why we process them specially */
    rc = vboxVDMACmdCheckCrCmd(pVdma, pCmd, cbCmd);
    if (rc == VINF_SUCCESS)
        return;

    if (RT_FAILURE(rc))
    {
        pCmd->rc = rc;
        rc = VBoxSHGSMICommandComplete (pVdma->pHgsmi, pCmd);
        AssertRC(rc);
        return;
    }

    vboxVDMACommandProcess(pVdma, pCmd, cbCmd);
#else
    RT_NOREF(cbCmd);
    pCmd->rc = rc;
    rc = VBoxSHGSMICommandComplete (pVdma->pHgsmi, pCmd);
    AssertRC(rc);
#endif
}

#ifdef VBOX_WITH_CRHGSMI

static DECLCALLBACK(void) vdmaVBVACtlSubmitSyncCompletion(VBVAEXHOSTCONTEXT *pVbva, struct VBVAEXHOSTCTL *pCtl, int rc, void *pvContext);

static int vdmaVBVACtlSubmit(PVBOXVDMAHOST pVdma, VBVAEXHOSTCTL* pCtl, VBVAEXHOSTCTL_SOURCE enmSource, PFNVBVAEXHOSTCTL_COMPLETE pfnComplete, void *pvComplete)
{
    int rc = VBoxVBVAExHCtlSubmit(&pVdma->CmdVbva, pCtl, enmSource, pfnComplete, pvComplete);
    if (RT_SUCCESS(rc))
    {
        if (rc == VINF_SUCCESS)
            return VBoxVDMAThreadEventNotify(&pVdma->Thread);
        else
            Assert(rc == VINF_ALREADY_INITIALIZED);
    }
    else
        Log(("VBoxVBVAExHCtlSubmit failed %d\n", rc));

    return rc;
}

static DECLCALLBACK(void) vboxCmdVBVACmdCtlGuestCompletion(VBVAEXHOSTCONTEXT *pVbva, struct VBVAEXHOSTCTL *pCtl, int rc, void *pvContext)
{
    PVBOXVDMAHOST pVdma = (PVBOXVDMAHOST)pvContext;
    VBOXCMDVBVA_CTL *pGCtl = (VBOXCMDVBVA_CTL*)(pCtl->u.cmd.pu8Cmd - sizeof (VBOXCMDVBVA_CTL));
    AssertRC(rc);
    pGCtl->i32Result = rc;

    Assert(pVdma->pVGAState->fGuestCaps & VBVACAPS_COMPLETEGCMD_BY_IOREAD);
    rc = VBoxSHGSMICommandComplete(pVdma->pHgsmi, pGCtl);
    AssertRC(rc);

    VBoxVBVAExHCtlFree(pVbva, pCtl);
}

static int vdmaVBVACtlGenericSubmit(PVBOXVDMAHOST pVdma, VBVAEXHOSTCTL_SOURCE enmSource, VBVAEXHOSTCTL_TYPE enmType, uint8_t* pu8Cmd, uint32_t cbCmd, PFNVBVAEXHOSTCTL_COMPLETE pfnComplete, void *pvComplete)
{
    VBVAEXHOSTCTL* pHCtl = VBoxVBVAExHCtlCreate(&pVdma->CmdVbva, enmType);
    if (!pHCtl)
    {
        WARN(("VBoxVBVAExHCtlCreate failed\n"));
        return VERR_NO_MEMORY;
    }

    pHCtl->u.cmd.pu8Cmd = pu8Cmd;
    pHCtl->u.cmd.cbCmd = cbCmd;
    int rc = vdmaVBVACtlSubmit(pVdma, pHCtl, enmSource, pfnComplete, pvComplete);
    if (RT_FAILURE(rc))
    {
        VBoxVBVAExHCtlFree(&pVdma->CmdVbva, pHCtl);
        Log(("vdmaVBVACtlSubmit failed rc %d\n", rc));
        return rc;;
    }
    return VINF_SUCCESS;
}

static int vdmaVBVACtlGenericGuestSubmit(PVBOXVDMAHOST pVdma, VBVAEXHOSTCTL_TYPE enmType, VBOXCMDVBVA_CTL *pCtl, uint32_t cbCtl)
{
    Assert(cbCtl >= sizeof (VBOXCMDVBVA_CTL));
    VBoxSHGSMICommandMarkAsynchCompletion(pCtl);
    int rc = vdmaVBVACtlGenericSubmit(pVdma, VBVAEXHOSTCTL_SOURCE_GUEST, enmType, (uint8_t*)(pCtl+1), cbCtl - sizeof (VBOXCMDVBVA_CTL), vboxCmdVBVACmdCtlGuestCompletion, pVdma);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    WARN(("vdmaVBVACtlGenericSubmit failed %d\n", rc));
    pCtl->i32Result = rc;
    rc = VBoxSHGSMICommandComplete(pVdma->pHgsmi, pCtl);
    AssertRC(rc);
    return VINF_SUCCESS;
}

static DECLCALLBACK(void) vboxCmdVBVACmdCtlHostCompletion(VBVAEXHOSTCONTEXT *pVbva, struct VBVAEXHOSTCTL *pCtl, int rc, void *pvCompletion)
{
    VBOXCRCMDCTL* pVboxCtl = (VBOXCRCMDCTL*)pCtl->u.cmd.pu8Cmd;
    if (pVboxCtl->u.pfnInternal)
        ((PFNCRCTLCOMPLETION)pVboxCtl->u.pfnInternal)(pVboxCtl, pCtl->u.cmd.cbCmd, rc, pvCompletion);
    VBoxVBVAExHCtlFree(pVbva, pCtl);
}

static int vdmaVBVACtlOpaqueHostSubmit(PVBOXVDMAHOST pVdma, struct VBOXCRCMDCTL* pCmd, uint32_t cbCmd,
        PFNCRCTLCOMPLETION pfnCompletion,
        void *pvCompletion)
{
    pCmd->u.pfnInternal = (void(*)())pfnCompletion;
    int rc = vdmaVBVACtlGenericSubmit(pVdma, VBVAEXHOSTCTL_SOURCE_HOST, VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE, (uint8_t*)pCmd, cbCmd, vboxCmdVBVACmdCtlHostCompletion, pvCompletion);
    if (RT_FAILURE(rc))
    {
        if (rc == VERR_INVALID_STATE)
        {
            pCmd->u.pfnInternal = NULL;
            PVGASTATE pVGAState = pVdma->pVGAState;
            rc = pVGAState->pDrv->pfnCrHgcmCtlSubmit(pVGAState->pDrv, pCmd, cbCmd, pfnCompletion, pvCompletion);
            if (!RT_SUCCESS(rc))
                WARN(("pfnCrHgsmiControlProcess failed %d\n", rc));

            return rc;
        }
        WARN(("vdmaVBVACtlGenericSubmit failed %d\n", rc));
        return rc;
    }

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) vdmaVBVANotifyEnable(PVGASTATE pVGAState)
{
    for (uint32_t i = 0; i < pVGAState->cMonitors; i++)
    {
        int rc = pVGAState->pDrv->pfnVBVAEnable (pVGAState->pDrv, i, NULL, true);
        if (!RT_SUCCESS(rc))
        {
            WARN(("pfnVBVAEnable failed %d\n", rc));
            for (uint32_t j = 0; j < i; j++)
            {
                pVGAState->pDrv->pfnVBVADisable (pVGAState->pDrv, j);
            }

            return rc;
        }
    }
    return VINF_SUCCESS;
}

static DECLCALLBACK(int) vdmaVBVANotifyDisable(PVGASTATE pVGAState)
{
    for (uint32_t i = 0; i < pVGAState->cMonitors; i++)
    {
        pVGAState->pDrv->pfnVBVADisable (pVGAState->pDrv, i);
    }
    return VINF_SUCCESS;
}

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
            WARN(("vboxVDMACrGuestCtlProcess failed %d\n", rc));
    }
    else
        WARN(("vdmaVBVACtlThreadCreatedEnable is passed %d\n", rc));

    VBoxVBVAExHPDataCompleteCtl(&pVdma->CmdVbva, pHCtl, rc);
}

static int vdmaVBVACtlEnableSubmitInternal(PVBOXVDMAHOST pVdma, VBVAENABLE *pEnable, bool fPaused, PFNVBVAEXHOSTCTL_COMPLETE pfnComplete, void *pvComplete)
{
    int rc;
    VBVAEXHOSTCTL* pHCtl = VBoxVBVAExHCtlCreate(&pVdma->CmdVbva, fPaused ? VBVAEXHOSTCTL_TYPE_GHH_ENABLE_PAUSED : VBVAEXHOSTCTL_TYPE_GHH_ENABLE);
    if (pHCtl)
    {
        pHCtl->u.cmd.pu8Cmd = (uint8_t*)pEnable;
        pHCtl->u.cmd.cbCmd = sizeof (*pEnable);
        pHCtl->pfnComplete = pfnComplete;
        pHCtl->pvComplete = pvComplete;

        rc = VBoxVDMAThreadCreate(&pVdma->Thread, vboxVDMAWorkerThread, pVdma, vdmaVBVACtlThreadCreatedEnable, pHCtl);
        if (RT_SUCCESS(rc))
            return VINF_SUCCESS;
        else
            WARN(("VBoxVDMAThreadCreate failed %d\n", rc));

        VBoxVBVAExHCtlFree(&pVdma->CmdVbva, pHCtl);
    }
    else
    {
        WARN(("VBoxVBVAExHCtlCreate failed\n"));
        rc = VERR_NO_MEMORY;
    }

    return rc;
}

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
        WARN(("RTSemEventCreate failed %d\n", rc));
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
                WARN(("vdmaVBVACtlSubmitSyncCompletion returned %d\n", rc));
        }
        else
            WARN(("RTSemEventWait failed %d\n", rc));
    }
    else
        WARN(("vdmaVBVACtlSubmit failed %d\n", rc));

    RTSemEventDestroy(Data.hEvent);

    return rc;
}

static int vdmaVBVACtlDisableSubmitInternal(PVBOXVDMAHOST pVdma, VBVAENABLE *pEnable, PFNVBVAEXHOSTCTL_COMPLETE pfnComplete, void *pvComplete)
{
    int rc;
    VBVAEXHOSTCTL* pHCtl;
    if (VBoxVBVAExHSIsDisabled(&pVdma->CmdVbva))
    {
        WARN(("VBoxVBVAExHSIsDisabled: disabled"));
        return VINF_SUCCESS;
    }

    pHCtl = VBoxVBVAExHCtlCreate(&pVdma->CmdVbva, VBVAEXHOSTCTL_TYPE_GHH_DISABLE);
    if (!pHCtl)
    {
        WARN(("VBoxVBVAExHCtlCreate failed\n"));
        return VERR_NO_MEMORY;
    }

    pHCtl->u.cmd.pu8Cmd = (uint8_t*)pEnable;
    pHCtl->u.cmd.cbCmd = sizeof (*pEnable);
    rc = vdmaVBVACtlSubmit(pVdma, pHCtl, VBVAEXHOSTCTL_SOURCE_GUEST, pfnComplete, pvComplete);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    WARN(("vdmaVBVACtlSubmit failed rc %d\n", rc));
    VBoxVBVAExHCtlFree(&pVdma->CmdVbva, pHCtl);
    return rc;
}

static int vdmaVBVACtlEnableDisableSubmitInternal(PVBOXVDMAHOST pVdma, VBVAENABLE *pEnable, PFNVBVAEXHOSTCTL_COMPLETE pfnComplete, void *pvComplete)
{
    bool fEnable = ((pEnable->u32Flags & (VBVA_F_ENABLE | VBVA_F_DISABLE)) == VBVA_F_ENABLE);
    if (fEnable)
        return vdmaVBVACtlEnableSubmitInternal(pVdma, pEnable, false, pfnComplete, pvComplete);
    return vdmaVBVACtlDisableSubmitInternal(pVdma, pEnable, pfnComplete, pvComplete);
}

static int vdmaVBVACtlEnableDisableSubmit(PVBOXVDMAHOST pVdma, VBOXCMDVBVA_CTL_ENABLE *pEnable)
{
    VBoxSHGSMICommandMarkAsynchCompletion(&pEnable->Hdr);
    int rc = vdmaVBVACtlEnableDisableSubmitInternal(pVdma, &pEnable->Enable, vboxCmdVBVACmdCtlGuestCompletion, pVdma);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    WARN(("vdmaVBVACtlEnableDisableSubmitInternal failed %d\n", rc));
    pEnable->Hdr.i32Result = rc;
    rc = VBoxSHGSMICommandComplete(pVdma->pHgsmi, &pEnable->Hdr);
    AssertRC(rc);
    return VINF_SUCCESS;
}

static DECLCALLBACK(void) vdmaVBVACtlSubmitSyncCompletion(VBVAEXHOSTCONTEXT *pVbva, struct VBVAEXHOSTCTL *pCtl,
                                                          int rc, void *pvContext)
{
    RT_NOREF(pVbva, pCtl);
    VDMA_VBVA_CTL_CYNC_COMPLETION *pData = (VDMA_VBVA_CTL_CYNC_COMPLETION*)pvContext;
    pData->rc = rc;
    rc = RTSemEventSignal(pData->hEvent);
    if (!RT_SUCCESS(rc))
        WARN(("RTSemEventSignal failed %d\n", rc));
}

static int vdmaVBVACtlSubmitSync(PVBOXVDMAHOST pVdma, VBVAEXHOSTCTL* pCtl, VBVAEXHOSTCTL_SOURCE enmSource)
{
    VDMA_VBVA_CTL_CYNC_COMPLETION Data;
    Data.rc = VERR_NOT_IMPLEMENTED;
    int rc = RTSemEventCreate(&Data.hEvent);
    if (!RT_SUCCESS(rc))
    {
        WARN(("RTSemEventCreate failed %d\n", rc));
        return rc;
    }

    rc = vdmaVBVACtlSubmit(pVdma, pCtl, enmSource, vdmaVBVACtlSubmitSyncCompletion, &Data);
    if (RT_SUCCESS(rc))
    {
        rc = RTSemEventWait(Data.hEvent, RT_INDEFINITE_WAIT);
        if (RT_SUCCESS(rc))
        {
            rc = Data.rc;
            if (!RT_SUCCESS(rc))
                WARN(("vdmaVBVACtlSubmitSyncCompletion returned %d\n", rc));
        }
        else
            WARN(("RTSemEventWait failed %d\n", rc));
    }
    else
        Log(("vdmaVBVACtlSubmit failed %d\n", rc));

    RTSemEventDestroy(Data.hEvent);

    return rc;
}

static int vdmaVBVAPause(PVBOXVDMAHOST pVdma)
{
    VBVAEXHOSTCTL Ctl;
    Ctl.enmType = VBVAEXHOSTCTL_TYPE_HH_INTERNAL_PAUSE;
    return vdmaVBVACtlSubmitSync(pVdma, &Ctl, VBVAEXHOSTCTL_SOURCE_HOST);
}

static int vdmaVBVAResume(PVBOXVDMAHOST pVdma)
{
    VBVAEXHOSTCTL Ctl;
    Ctl.enmType = VBVAEXHOSTCTL_TYPE_HH_INTERNAL_RESUME;
    return vdmaVBVACtlSubmitSync(pVdma, &Ctl, VBVAEXHOSTCTL_SOURCE_HOST);
}

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


int vboxCmdVBVACmdHostCtl(PPDMIDISPLAYVBVACALLBACKS pInterface,
                                                               struct VBOXCRCMDCTL* pCmd, uint32_t cbCmd,
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

typedef struct VBOXCMDVBVA_CMDHOSTCTL_SYNC
{
    struct VBOXVDMAHOST *pVdma;
    uint32_t fProcessing;
    int rc;
} VBOXCMDVBVA_CMDHOSTCTL_SYNC;

static DECLCALLBACK(void) vboxCmdVBVACmdHostCtlSyncCb(struct VBOXCRCMDCTL* pCmd, uint32_t cbCmd, int rc, void *pvCompletion)
{
    RT_NOREF(pCmd, cbCmd);
    VBOXCMDVBVA_CMDHOSTCTL_SYNC *pData = (VBOXCMDVBVA_CMDHOSTCTL_SYNC*)pvCompletion;

    pData->rc = rc;

    struct VBOXVDMAHOST *pVdma = pData->pVdma;

    ASMAtomicIncS32(&pVdma->i32cHostCrCtlCompleted);

    pData->fProcessing = 0;

    RTSemEventMultiSignal(pVdma->HostCrCtlCompleteEvent);
}

static DECLCALLBACK(int) vboxCmdVBVACmdCallout(struct VBOXVDMAHOST *pVdma, struct VBOXCRCMDCTL* pCmd, VBOXCRCMDCTL_CALLOUT_LISTENTRY *pEntry, PFNVBOXCRCMDCTL_CALLOUT_CB pfnCb)
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
        WARN(("RTCritSectEnter failed %d\n", rc));

    return rc;
}


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
            WARN(("RTCritSectEnter failed %d\n", rc));
            break;
        }
    }

    return rc;
}

DECLCALLBACK(int) vboxCmdVBVACmdHostCtlSync(PPDMIDISPLAYVBVACALLBACKS pInterface,
                                            struct VBOXCRCMDCTL* pCmd, uint32_t cbCmd)
{
    PVGASTATE pVGAState = PPDMIDISPLAYVBVACALLBACKS_2_PVGASTATE(pInterface);
    struct VBOXVDMAHOST *pVdma = pVGAState->pVdma;
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
        WARN(("vdmaVBVACtlOpaqueHostSubmit failed %d", rc));
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
        WARN(("host call failed %d", rc));

    return rc;
}

int vboxCmdVBVACmdCtl(PVGASTATE pVGAState, VBOXCMDVBVA_CTL *pCtl, uint32_t cbCtl)
{
    struct VBOXVDMAHOST *pVdma = pVGAState->pVdma;
    int rc = VINF_SUCCESS;
    switch (pCtl->u32Type)
    {
        case VBOXCMDVBVACTL_TYPE_3DCTL:
            return vdmaVBVACtlGenericGuestSubmit(pVdma, VBVAEXHOSTCTL_TYPE_GHH_BE_OPAQUE, pCtl, cbCtl);
        case VBOXCMDVBVACTL_TYPE_RESIZE:
            return vdmaVBVACtlGenericGuestSubmit(pVdma, VBVAEXHOSTCTL_TYPE_GHH_RESIZE, pCtl, cbCtl);
        case VBOXCMDVBVACTL_TYPE_ENABLE:
            if (cbCtl != sizeof (VBOXCMDVBVA_CTL_ENABLE))
            {
                WARN(("incorrect enable size\n"));
                rc = VERR_INVALID_PARAMETER;
                break;
            }
            return vdmaVBVACtlEnableDisableSubmit(pVdma, (VBOXCMDVBVA_CTL_ENABLE*)pCtl);
        default:
            WARN(("unsupported type\n"));
            rc = VERR_INVALID_PARAMETER;
            break;
    }

    pCtl->i32Result = rc;
    rc = VBoxSHGSMICommandComplete(pVdma->pHgsmi, pCtl);
    AssertRC(rc);
    return VINF_SUCCESS;
}

int vboxCmdVBVACmdSubmit(PVGASTATE pVGAState)
{
    if (!VBoxVBVAExHSIsEnabled(&pVGAState->pVdma->CmdVbva))
    {
        WARN(("vdma VBVA is disabled\n"));
        return VERR_INVALID_STATE;
    }

    return vboxVDMACmdSubmitPerform(pVGAState->pVdma);
}

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

void vboxCmdVBVACmdTimer(PVGASTATE pVGAState)
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

int vboxVDMASaveStateExecPrep(struct VBOXVDMAHOST *pVdma)
{
#ifdef VBOX_WITH_CRHGSMI
    int rc = vdmaVBVAPause(pVdma);
    if (RT_SUCCESS(rc))
        return VINF_SUCCESS;

    if (rc != VERR_INVALID_STATE)
    {
        WARN(("vdmaVBVAPause failed %d\n", rc));
        return rc;
    }

# ifdef DEBUG_misha
    WARN(("debug prep"));
# endif

    PVGASTATE pVGAState = pVdma->pVGAState;
    PVBOXVDMACMD_CHROMIUM_CTL pCmd = (PVBOXVDMACMD_CHROMIUM_CTL)vboxVDMACrCtlCreate(
            VBOXVDMACMD_CHROMIUM_CTL_TYPE_SAVESTATE_BEGIN, sizeof (*pCmd));
    Assert(pCmd);
    if (pCmd)
    {
        rc = vboxVDMACrCtlPost(pVGAState, pCmd, sizeof (*pCmd));
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            rc = vboxVDMACrCtlGetRc(pCmd);
        }
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
        WARN(("vdmaVBVAResume failed %d\n", rc));
        return rc;
    }

# ifdef DEBUG_misha
    WARN(("debug done"));
# endif

    PVGASTATE pVGAState = pVdma->pVGAState;
    PVBOXVDMACMD_CHROMIUM_CTL pCmd = (PVBOXVDMACMD_CHROMIUM_CTL)vboxVDMACrCtlCreate(
            VBOXVDMACMD_CHROMIUM_CTL_TYPE_SAVESTATE_END, sizeof (*pCmd));
    Assert(pCmd);
    if (pCmd)
    {
        rc = vboxVDMACrCtlPost(pVGAState, pCmd, sizeof (*pCmd));
        AssertRC(rc);
        if (RT_SUCCESS(rc))
        {
            rc = vboxVDMACrCtlGetRc(pCmd);
        }
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

    rc = SSMR3PutU32(pSSM, (uint32_t)(((uint8_t*)pVdma->CmdVbva.pVBVA) - pu8VramBase));
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
    VBVAEXHOSTCTL* pHCtl = VBoxVBVAExHCtlCreate(&pVdma->CmdVbva, VBVAEXHOSTCTL_TYPE_HH_LOADSTATE_DONE);
    if (!pHCtl)
    {
        WARN(("VBoxVBVAExHCtlCreate failed\n"));
        return VERR_NO_MEMORY;
    }

    /* sanity */
    pHCtl->u.cmd.pu8Cmd = NULL;
    pHCtl->u.cmd.cbCmd = 0;

    /* NULL completion will just free the ctl up */
    int rc = vdmaVBVACtlSubmit(pVdma, pHCtl, VBVAEXHOSTCTL_SOURCE_HOST, NULL, NULL);
    if (RT_FAILURE(rc))
    {
        Log(("vdmaVBVACtlSubmit failed rc %d\n", rc));
        VBoxVBVAExHCtlFree(&pVdma->CmdVbva, pHCtl);
        return rc;
    }
#else
    RT_NOREF(pVdma);
#endif
    return VINF_SUCCESS;
}

