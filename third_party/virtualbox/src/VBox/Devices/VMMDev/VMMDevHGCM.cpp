/* $Id: VMMDevHGCM.cpp $ */
/** @file
 * VMMDev - HGCM - Host-Guest Communication Manager Device.
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
#define LOG_GROUP LOG_GROUP_DEV_VMM
#include <iprt/alloc.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/param.h>
#include <iprt/string.h>

#include <VBox/err.h>
#include <VBox/hgcmsvc.h>

#include <VBox/log.h>

#include "VMMDevHGCM.h"

#ifdef VBOX_WITH_DTRACE
# include "dtrace/VBoxDD.h"
#else
# define VBOXDD_HGCMCALL_ENTER(a,b,c,d)             do { } while (0)
# define VBOXDD_HGCMCALL_COMPLETED_REQ(a,b)         do { } while (0)
# define VBOXDD_HGCMCALL_COMPLETED_EMT(a,b)         do { } while (0)
# define VBOXDD_HGCMCALL_COMPLETED_DONE(a,b,c,d)    do { } while (0)
#endif


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef enum VBOXHGCMCMDTYPE
{
    VBOXHGCMCMDTYPE_LOADSTATE = 0,
    VBOXHGCMCMDTYPE_CONNECT,
    VBOXHGCMCMDTYPE_DISCONNECT,
    VBOXHGCMCMDTYPE_CALL,
    VBOXHGCMCMDTYPE_SizeHack = 0x7fffffff
} VBOXHGCMCMDTYPE;

/**
 * Information about a linear ptr parameter.
 */
typedef struct VBOXHGCMLINPTR
{
    /** Index of the parameter. */
    uint32_t iParm;

    /** Offset in the first physical page of the region. */
    uint32_t offFirstPage;

    /** How many pages. */
    uint32_t cPages;

    /** Pointer to array of the GC physical addresses for these pages.
     * It is assumed that the physical address of the locked resident guest page
     * does not change.
     */
    RTGCPHYS *paPages;

} VBOXHGCMLINPTR;

struct VBOXHGCMCMD
{
    /** Active commands, list is protected by critsectHGCMCmdList. */
    struct VBOXHGCMCMD *pNext;
    struct VBOXHGCMCMD *pPrev;

    /** The type of the command. */
    VBOXHGCMCMDTYPE enmCmdType;

    /** Whether the command was cancelled by the guest. */
    bool fCancelled;

    /** Whether the command is in the active commands list. */
    bool fInList;

    /** GC physical address of the guest request. */
    RTGCPHYS        GCPhys;

    /** Request packet size */
    uint32_t        cbSize;

    /** Pointer to converted host parameters in case of a Call request.
     * Parameters follow this structure in the same memory block.
     */
    VBOXHGCMSVCPARM *paHostParms;

    /* Number of elements in paHostParms */
    uint32_t cHostParms;

    /** Linear pointer parameters information. */
    int cLinPtrs;

    /** How many pages for all linptrs of this command.
     * Only valid if cLinPtrs > 0. This field simplifies loading of saved state.
     */
    int cLinPtrPages;

    /** Pointer to descriptions of linear pointers.  */
    VBOXHGCMLINPTR *paLinPtrs;
};



static int vmmdevHGCMCmdListLock (PVMMDEV pThis)
{
    int rc = RTCritSectEnter (&pThis->critsectHGCMCmdList);
    AssertRC (rc);
    return rc;
}

static void vmmdevHGCMCmdListUnlock (PVMMDEV pThis)
{
    int rc = RTCritSectLeave (&pThis->critsectHGCMCmdList);
    AssertRC (rc);
}

static int vmmdevHGCMAddCommand (PVMMDEV pThis, PVBOXHGCMCMD pCmd, RTGCPHYS GCPhys, uint32_t cbSize, VBOXHGCMCMDTYPE enmCmdType)
{
    /* PPDMDEVINS pDevIns = pThis->pDevIns; */

    int rc = vmmdevHGCMCmdListLock (pThis);

    if (RT_SUCCESS (rc))
    {
        LogFlowFunc(("%p type %d\n", pCmd, enmCmdType));

        /* Insert at the head of the list. The vmmdevHGCMLoadStateDone depends on this. */
        pCmd->pNext = pThis->pHGCMCmdList;
        pCmd->pPrev = NULL;

        if (pThis->pHGCMCmdList)
        {
            pThis->pHGCMCmdList->pPrev = pCmd;
        }

        pThis->pHGCMCmdList = pCmd;

        pCmd->fInList = true;

        if (enmCmdType != VBOXHGCMCMDTYPE_LOADSTATE)
        {
            /* Loaded commands already have the right type. */
            pCmd->enmCmdType = enmCmdType;
        }
        pCmd->GCPhys = GCPhys;
        pCmd->cbSize = cbSize;

        /* Automatically enable HGCM events, if there are HGCM commands. */
        if (   enmCmdType == VBOXHGCMCMDTYPE_CONNECT
            || enmCmdType == VBOXHGCMCMDTYPE_DISCONNECT
            || enmCmdType == VBOXHGCMCMDTYPE_CALL)
        {
            Log(("vmmdevHGCMAddCommand: u32HGCMEnabled = %d\n", pThis->u32HGCMEnabled));
            if (ASMAtomicCmpXchgU32(&pThis->u32HGCMEnabled, 1, 0))
            {
                 VMMDevCtlSetGuestFilterMask (pThis, VMMDEV_EVENT_HGCM, 0);
            }
        }

        vmmdevHGCMCmdListUnlock (pThis);
    }

    return rc;
}

static int vmmdevHGCMRemoveCommand (PVMMDEV pThis, PVBOXHGCMCMD pCmd)
{
    /* PPDMDEVINS pDevIns = pThis->pDevIns; */

    int rc = vmmdevHGCMCmdListLock (pThis);

    if (RT_SUCCESS (rc))
    {
        LogFlowFunc(("%p\n", pCmd));

        if (!pCmd->fInList)
        {
            LogFlowFunc(("%p not in the list\n", pCmd));
            vmmdevHGCMCmdListUnlock (pThis);
            return VINF_SUCCESS;
        }

        if (pCmd->pNext)
        {
            pCmd->pNext->pPrev = pCmd->pPrev;
        }
        else
        {
           /* Tail, do nothing. */
        }

        if (pCmd->pPrev)
        {
            pCmd->pPrev->pNext = pCmd->pNext;
        }
        else
        {
            pThis->pHGCMCmdList = pCmd->pNext;
        }

        pCmd->pNext = NULL;
        pCmd->pPrev = NULL;
        pCmd->fInList = false;

        vmmdevHGCMCmdListUnlock (pThis);
    }

    return rc;
}


/**
 * Find a HGCM command by its physical address.
 *
 * The caller is responsible for taking the command list lock before calling
 * this function.
 *
 * @returns Pointer to the command on success, NULL otherwise.
 * @param   pThis           The VMMDev instance data.
 * @param   GCPhys          The physical address of the command we're looking
 *                          for.
 */
DECLINLINE(PVBOXHGCMCMD) vmmdevHGCMFindCommandLocked (PVMMDEV pThis, RTGCPHYS GCPhys)
{
    for (PVBOXHGCMCMD pCmd = pThis->pHGCMCmdList;
         pCmd;
         pCmd = pCmd->pNext)
    {
         if (pCmd->GCPhys == GCPhys)
             return pCmd;
    }
    return NULL;
}

static int vmmdevHGCMSaveLinPtr (PPDMDEVINS pDevIns,
                                 uint32_t iParm,
                                 RTGCPTR GCPtr,
                                 uint32_t u32Size,
                                 uint32_t iLinPtr,
                                 VBOXHGCMLINPTR *paLinPtrs,
                                 RTGCPHYS **ppPages)
{
    int rc = VINF_SUCCESS;

    VBOXHGCMLINPTR *pLinPtr = &paLinPtrs[iLinPtr];

    /* Take the offset into the current page also into account! */
    u32Size += GCPtr & PAGE_OFFSET_MASK;

    uint32_t cPages = (u32Size + PAGE_SIZE - 1) / PAGE_SIZE;

    Log(("vmmdevHGCMSaveLinPtr: parm %d: %RGv %d = %d pages\n", iParm, GCPtr, u32Size, cPages));

    pLinPtr->iParm          = iParm;
    pLinPtr->offFirstPage   = GCPtr & PAGE_OFFSET_MASK;
    pLinPtr->cPages         = cPages;
    pLinPtr->paPages        = *ppPages;

    *ppPages += cPages;

    uint32_t iPage = 0;

    GCPtr &= PAGE_BASE_GC_MASK;

    /* Gonvert the guest linear pointers of pages to HC addresses. */
    while (iPage < cPages)
    {
        /* convert */
        RTGCPHYS GCPhys;

        rc = PDMDevHlpPhysGCPtr2GCPhys(pDevIns, GCPtr, &GCPhys);

        Log(("vmmdevHGCMSaveLinPtr: Page %d: %RGv -> %RGp. %Rrc\n", iPage, GCPtr, GCPhys, rc));

        if (RT_FAILURE (rc))
        {
            break;
        }

        /* store */
        pLinPtr->paPages[iPage++] = GCPhys;

        /* next */
        GCPtr += PAGE_SIZE;
    }

    return rc;
}

static int vmmdevHGCMWriteLinPtr (PPDMDEVINS pDevIns,
                                  uint32_t iParm,
                                  void *pvHost,
                                  uint32_t u32Size,
                                  uint32_t iLinPtr,
                                  VBOXHGCMLINPTR *paLinPtrs)
{
    int rc = VINF_SUCCESS;

    VBOXHGCMLINPTR *pLinPtr = &paLinPtrs[iLinPtr];

    AssertLogRelReturn(u32Size > 0 && iParm == (uint32_t)pLinPtr->iParm, VERR_INVALID_PARAMETER);

    RTGCPHYS GCPhysDst = pLinPtr->paPages[0] + pLinPtr->offFirstPage;
    uint8_t *pu8Src    = (uint8_t *)pvHost;

    Log(("vmmdevHGCMWriteLinPtr: parm %d: size %d, cPages = %d\n", iParm, u32Size, pLinPtr->cPages));

    uint32_t iPage = 0;

    while (iPage < pLinPtr->cPages)
    {
        /* copy */
        uint32_t cbWrite = iPage == 0?
                               PAGE_SIZE - pLinPtr->offFirstPage:
                               PAGE_SIZE;

        Log(("vmmdevHGCMWriteLinPtr: page %d: dst %RGp, src %p, cbWrite %d\n", iPage, GCPhysDst, pu8Src, cbWrite));

        iPage++;

        if (cbWrite >= u32Size)
        {
            rc = PDMDevHlpPhysWrite(pDevIns, GCPhysDst, pu8Src, u32Size);
            if (RT_FAILURE(rc))
                break;

            u32Size = 0;
            break;
        }

        rc = PDMDevHlpPhysWrite(pDevIns, GCPhysDst, pu8Src, cbWrite);
        if (RT_FAILURE(rc))
            break;

        /* next */
        u32Size    -= cbWrite;
        pu8Src     += cbWrite;

        GCPhysDst   = pLinPtr->paPages[iPage];
    }

    if (RT_SUCCESS(rc))
    {
        AssertLogRelReturn(iPage == pLinPtr->cPages, VERR_INVALID_PARAMETER);
    }

    return rc;
}

DECLINLINE(bool) vmmdevHGCMPageListIsContiguous(const HGCMPageListInfo *pPgLst)
{
    if (pPgLst->cPages == 1)
        return true;
    RTGCPHYS64 Phys = pPgLst->aPages[0] + PAGE_SIZE;
    if (Phys != pPgLst->aPages[1])
        return false;
    if (pPgLst->cPages > 2)
    {
        uint32_t iPage = 2;
        do
        {
            Phys += PAGE_SIZE;
            if (Phys != pPgLst->aPages[iPage])
                return false;
            iPage++;
        } while (iPage < pPgLst->cPages);
    }
    return true;
}

static int vmmdevHGCMPageListRead(PPDMDEVINSR3 pDevIns, void *pvDst, uint32_t cbDst, const HGCMPageListInfo *pPageListInfo)
{
    /*
     * Try detect contiguous buffers.
     */
    /** @todo We need a flag for indicating this. */
    if (vmmdevHGCMPageListIsContiguous(pPageListInfo))
        return PDMDevHlpPhysRead(pDevIns, pPageListInfo->aPages[0] | pPageListInfo->offFirstPage, pvDst, cbDst);

    /*
     * Page by page fallback
     */
    int rc = VINF_SUCCESS;

    uint8_t *pu8Dst = (uint8_t *)pvDst;
    uint32_t offPage = pPageListInfo->offFirstPage;
    size_t cbRemaining = (size_t)cbDst;

    uint32_t iPage;

    for (iPage = 0; iPage < pPageListInfo->cPages; iPage++)
    {
        if (cbRemaining == 0)
        {
            break;
        }

        size_t cbChunk = PAGE_SIZE - offPage;

        if (cbChunk > cbRemaining)
        {
            cbChunk = cbRemaining;
        }

        rc = PDMDevHlpPhysRead(pDevIns,
                               pPageListInfo->aPages[iPage] + offPage,
                               pu8Dst, cbChunk);

        AssertRCBreak(rc);

        offPage = 0; /* A next page is read from 0 offset. */
        cbRemaining -= cbChunk;
        pu8Dst += cbChunk;
    }

    return rc;
}

static int vmmdevHGCMPageListWrite(PPDMDEVINSR3 pDevIns, const HGCMPageListInfo *pPageListInfo, const void *pvSrc, uint32_t cbSrc)
{
    int rc = VINF_SUCCESS;

    uint8_t *pu8Src = (uint8_t *)pvSrc;
    uint32_t offPage = pPageListInfo->offFirstPage;
    size_t cbRemaining = (size_t)cbSrc;

    uint32_t iPage;
    for (iPage = 0; iPage < pPageListInfo->cPages; iPage++)
    {
        if (cbRemaining == 0)
        {
            break;
        }

        size_t cbChunk = PAGE_SIZE - offPage;

        if (cbChunk > cbRemaining)
        {
            cbChunk = cbRemaining;
        }

        rc = PDMDevHlpPhysWrite(pDevIns,
                                pPageListInfo->aPages[iPage] + offPage,
                                pu8Src, cbChunk);

        AssertRCBreak(rc);

        offPage = 0; /* A next page is read from 0 offset. */
        cbRemaining -= cbChunk;
        pu8Src += cbChunk;
    }

    return rc;
}

static void vmmdevRestoreSavedCommand(VBOXHGCMCMD *pCmd, VBOXHGCMCMD *pSavedCmd)
{
    /* Copy relevant saved command information to the new allocated structure. */
    pCmd->enmCmdType   = pSavedCmd->enmCmdType;
    pCmd->fCancelled   = pSavedCmd->fCancelled;
    pCmd->GCPhys       = pSavedCmd->GCPhys;
    pCmd->cbSize       = pSavedCmd->cbSize;
    pCmd->cLinPtrs     = pSavedCmd->cLinPtrs;
    pCmd->cLinPtrPages = pSavedCmd->cLinPtrPages;
    pCmd->paLinPtrs    = pSavedCmd->paLinPtrs;

    /* The new allocated command owns the 'paLinPtrs' pointer. */
    pSavedCmd->paLinPtrs = NULL;
}

int vmmdevHGCMConnect (PVMMDEV pThis, VMMDevHGCMConnect *pHGCMConnect, RTGCPHYS GCPhys)
{
    int rc = VINF_SUCCESS;

    uint32_t cbCmdSize = sizeof (struct VBOXHGCMCMD) + pHGCMConnect->header.header.size;

    PVBOXHGCMCMD pCmd = (PVBOXHGCMCMD)RTMemAllocZ (cbCmdSize);

    if (pCmd)
    {
        VMMDevHGCMConnect *pHGCMConnectCopy = (VMMDevHGCMConnect *)(pCmd+1);

        vmmdevHGCMAddCommand (pThis, pCmd, GCPhys, pHGCMConnect->header.header.size, VBOXHGCMCMDTYPE_CONNECT);

        memcpy(pHGCMConnectCopy, pHGCMConnect, pHGCMConnect->header.header.size);

        pCmd->paHostParms = NULL;
        pCmd->cLinPtrs = 0;
        pCmd->paLinPtrs = NULL;

        /* Only allow the guest to use existing services! */
        Assert(pHGCMConnectCopy->loc.type == VMMDevHGCMLoc_LocalHost_Existing);
        pHGCMConnectCopy->loc.type = VMMDevHGCMLoc_LocalHost_Existing;

        rc = pThis->pHGCMDrv->pfnConnect (pThis->pHGCMDrv, pCmd, &pHGCMConnectCopy->loc, &pHGCMConnectCopy->u32ClientID);

        if (RT_FAILURE(rc))
            vmmdevHGCMRemoveCommand(pThis, pCmd);
    }
    else
    {
        rc = VERR_NO_MEMORY;
    }

    return rc;
}

static int vmmdevHGCMConnectSaved (PVMMDEV pThis, VMMDevHGCMConnect *pHGCMConnect, RTGCPHYS GCPhys, bool *pfHGCMCalled, VBOXHGCMCMD *pSavedCmd, VBOXHGCMCMD **ppCmd)
{
    int rc = VINF_SUCCESS;

    /* Allocate buffer for the new command. */
    uint32_t cbCmdSize = sizeof (struct VBOXHGCMCMD) + pHGCMConnect->header.header.size;

    PVBOXHGCMCMD pCmd = (PVBOXHGCMCMD)RTMemAllocZ (cbCmdSize);

    if (pCmd)
    {
        vmmdevRestoreSavedCommand(pCmd, pSavedCmd);
        *ppCmd = pCmd;

        VMMDevHGCMConnect *pHGCMConnectCopy = (VMMDevHGCMConnect *)(pCmd+1);

        vmmdevHGCMAddCommand (pThis, pCmd, GCPhys, pHGCMConnect->header.header.size, VBOXHGCMCMDTYPE_CONNECT);

        memcpy(pHGCMConnectCopy, pHGCMConnect, pHGCMConnect->header.header.size);

        /* Only allow the guest to use existing services! */
        Assert(pHGCMConnectCopy->loc.type == VMMDevHGCMLoc_LocalHost_Existing);
        pHGCMConnectCopy->loc.type = VMMDevHGCMLoc_LocalHost_Existing;

        rc = pThis->pHGCMDrv->pfnConnect (pThis->pHGCMDrv, pCmd, &pHGCMConnectCopy->loc, &pHGCMConnectCopy->u32ClientID);

        if (RT_SUCCESS (rc))
        {
            *pfHGCMCalled = true;
        }
        /* else
         *  ... the caller will also execute vmmdevHGCMRemoveCommand() for us */
    }
    else
    {
        rc = VERR_NO_MEMORY;
    }

    return rc;
}

int vmmdevHGCMDisconnect (PVMMDEV pThis, VMMDevHGCMDisconnect *pHGCMDisconnect, RTGCPHYS GCPhys)
{
    int rc = VINF_SUCCESS;

    uint32_t cbCmdSize = sizeof (struct VBOXHGCMCMD);

    PVBOXHGCMCMD pCmd = (PVBOXHGCMCMD)RTMemAllocZ (cbCmdSize);

    if (pCmd)
    {
        vmmdevHGCMAddCommand (pThis, pCmd, GCPhys, pHGCMDisconnect->header.header.size, VBOXHGCMCMDTYPE_DISCONNECT);

        pCmd->paHostParms = NULL;
        pCmd->cLinPtrs = 0;
        pCmd->paLinPtrs = NULL;

        rc = pThis->pHGCMDrv->pfnDisconnect (pThis->pHGCMDrv, pCmd, pHGCMDisconnect->u32ClientID);

        if (RT_FAILURE(rc))
            vmmdevHGCMRemoveCommand(pThis, pCmd);
    }
    else
    {
        rc = VERR_NO_MEMORY;
    }

    return rc;
}

static int vmmdevHGCMDisconnectSaved (PVMMDEV pThis, VMMDevHGCMDisconnect *pHGCMDisconnect, RTGCPHYS GCPhys, bool *pfHGCMCalled, VBOXHGCMCMD *pSavedCmd, VBOXHGCMCMD **ppCmd)
{
    int rc = VINF_SUCCESS;

    uint32_t cbCmdSize = sizeof (struct VBOXHGCMCMD);

    PVBOXHGCMCMD pCmd = (PVBOXHGCMCMD)RTMemAllocZ (cbCmdSize);

    if (pCmd)
    {
        vmmdevRestoreSavedCommand(pCmd, pSavedCmd);
        *ppCmd = pCmd;

        vmmdevHGCMAddCommand (pThis, pCmd, GCPhys, pHGCMDisconnect->header.header.size, VBOXHGCMCMDTYPE_DISCONNECT);

        pCmd->paHostParms = NULL;
        pCmd->cLinPtrs = 0;
        pCmd->paLinPtrs = NULL;

        rc = pThis->pHGCMDrv->pfnDisconnect (pThis->pHGCMDrv, pCmd, pHGCMDisconnect->u32ClientID);

        if (RT_SUCCESS (rc))
        {
            *pfHGCMCalled = true;
        }
        /* else
         *  ... the caller will also execute vmmdevHGCMRemoveCommand() for us */
    }
    else
    {
        rc = VERR_NO_MEMORY;
    }

    return rc;
}

int vmmdevHGCMCall (PVMMDEV pThis, VMMDevHGCMCall *pHGCMCall, uint32_t cbHGCMCall, RTGCPHYS GCPhys, bool f64Bits)
{
    int rc = VINF_SUCCESS;

    Log(("vmmdevHGCMCall: client id = %d, function = %d, %s bit\n", pHGCMCall->u32ClientID, pHGCMCall->u32Function, f64Bits? "64": "32"));

    /* Compute size and allocate memory block to hold:
     *    struct VBOXHGCMCMD
     *    VBOXHGCMSVCPARM[cParms]
     *    memory buffers for pointer parameters.
     */

    uint32_t cParms = pHGCMCall->cParms;

    Log(("vmmdevHGCMCall: cParms = %d\n", cParms));

    /*
     * Sane upper limit.
     */
    if (cParms > VMMDEV_MAX_HGCM_PARMS)
    {
        LogRelMax(50, ("VMMDev: request packet with too many parameters (%d). Refusing operation.\n", cParms));
        return VERR_INVALID_PARAMETER;
    }

    /*
     * Compute size of required memory buffer.
     */

    uint32_t cbCmdSize = sizeof (struct VBOXHGCMCMD) + cParms * sizeof (VBOXHGCMSVCPARM);

    uint32_t i;

    uint32_t cLinPtrs = 0;
    uint32_t cLinPtrPages  = 0;

    if (f64Bits)
    {
#ifdef VBOX_WITH_64_BITS_GUESTS
        HGCMFunctionParameter64 *pGuestParm = VMMDEV_HGCM_CALL_PARMS64(pHGCMCall);
#else
        HGCMFunctionParameter *pGuestParm = VMMDEV_HGCM_CALL_PARMS(pHGCMCall);
        AssertFailed (); /* This code should not be called in this case */
#endif /* VBOX_WITH_64_BITS_GUESTS */

        /* Look for pointer parameters, which require a host buffer. */
        for (i = 0; i < cParms && RT_SUCCESS(rc); i++, pGuestParm++)
        {
            switch (pGuestParm->type)
            {
                case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
                case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
                case VMMDevHGCMParmType_LinAddr:     /* In & Out */
                {
                    if (pGuestParm->u.Pointer.size > 0)
                    {
                        /* Only pointers with some actual data are counted. */
                        if (pGuestParm->u.Pointer.size > VMMDEV_MAX_HGCM_DATA_SIZE - cbCmdSize)
                        {
                            rc = VERR_INVALID_PARAMETER;
                            break;
                        }

                        cbCmdSize += pGuestParm->u.Pointer.size;

                        cLinPtrs++;
                        /* Take the offset into the current page also into account! */
                        cLinPtrPages += ((pGuestParm->u.Pointer.u.linearAddr & PAGE_OFFSET_MASK)
                                          + pGuestParm->u.Pointer.size + PAGE_SIZE - 1) / PAGE_SIZE;
                    }

                    Log(("vmmdevHGCMCall: linptr size = %d\n", pGuestParm->u.Pointer.size));
                } break;

                case VMMDevHGCMParmType_PageList:
                {
                    if (pGuestParm->u.PageList.size > VMMDEV_MAX_HGCM_DATA_SIZE - cbCmdSize)
                    {
                        rc = VERR_INVALID_PARAMETER;
                        break;
                    }

                    cbCmdSize += pGuestParm->u.PageList.size;
                    Log(("vmmdevHGCMCall: pagelist size = %d\n", pGuestParm->u.PageList.size));
                } break;

                case VMMDevHGCMParmType_32bit:
                case VMMDevHGCMParmType_64bit:
                {
                } break;

                default:
                case VMMDevHGCMParmType_PhysAddr:
                {
                    AssertMsgFailed(("vmmdevHGCMCall: invalid parameter type %x\n", pGuestParm->type));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }
            }
        }
    }
    else
    {
#ifdef VBOX_WITH_64_BITS_GUESTS
        HGCMFunctionParameter32 *pGuestParm = VMMDEV_HGCM_CALL_PARMS32(pHGCMCall);
#else
        HGCMFunctionParameter *pGuestParm = VMMDEV_HGCM_CALL_PARMS(pHGCMCall);
#endif /* VBOX_WITH_64_BITS_GUESTS */

        /* Look for pointer parameters, which require a host buffer. */
        for (i = 0; i < cParms && RT_SUCCESS(rc); i++, pGuestParm++)
        {
            switch (pGuestParm->type)
            {
                case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
                case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
                case VMMDevHGCMParmType_LinAddr:     /* In & Out */
                {
                    if (pGuestParm->u.Pointer.size > 0)
                    {
                        /* Only pointers with some actual data are counted. */
                        if (pGuestParm->u.Pointer.size > VMMDEV_MAX_HGCM_DATA_SIZE - cbCmdSize)
                        {
                            rc = VERR_INVALID_PARAMETER;
                            break;
                        }

                        cbCmdSize += pGuestParm->u.Pointer.size;

                        cLinPtrs++;
                        /* Take the offset into the current page also into account! */
                        cLinPtrPages += ((pGuestParm->u.Pointer.u.linearAddr & PAGE_OFFSET_MASK)
                                          + pGuestParm->u.Pointer.size + PAGE_SIZE - 1) / PAGE_SIZE;
                    }

                    Log(("vmmdevHGCMCall: linptr size = %d\n", pGuestParm->u.Pointer.size));
                } break;

                case VMMDevHGCMParmType_PageList:
                {
                    if (pGuestParm->u.PageList.size > VMMDEV_MAX_HGCM_DATA_SIZE - cbCmdSize)
                    {
                        rc = VERR_INVALID_PARAMETER;
                        break;
                    }

                    cbCmdSize += pGuestParm->u.PageList.size;
                    Log(("vmmdevHGCMCall: pagelist size = %d\n", pGuestParm->u.PageList.size));
                } break;

                case VMMDevHGCMParmType_32bit:
                case VMMDevHGCMParmType_64bit:
                {
                } break;

                default:
                {
                    AssertMsgFailed(("vmmdevHGCMCall: invalid parameter type %x\n", pGuestParm->type));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }
            }
        }
    }

    if (RT_FAILURE (rc))
    {
        return rc;
    }

    PVBOXHGCMCMD pCmd = (PVBOXHGCMCMD)RTMemAllocZ(cbCmdSize);

    if (pCmd == NULL)
    {
        return VERR_NO_MEMORY;
    }

    pCmd->paHostParms = NULL;
    pCmd->cLinPtrs    = cLinPtrs;
    pCmd->cLinPtrPages = cLinPtrPages;

    if (cLinPtrs > 0)
    {
        pCmd->paLinPtrs = (VBOXHGCMLINPTR *)RTMemAllocZ(  sizeof (VBOXHGCMLINPTR) * cLinPtrs
                                                        + sizeof (RTGCPHYS) * cLinPtrPages);

        if (pCmd->paLinPtrs == NULL)
        {
            RTMemFree (pCmd);
            return VERR_NO_MEMORY;
        }
    }
    else
    {
        pCmd->paLinPtrs = NULL;
    }

    VBOXDD_HGCMCALL_ENTER(pCmd, pHGCMCall->u32Function, pHGCMCall->u32ClientID, cbCmdSize);

    /* Process parameters, changing them to host context pointers for easy
     * processing by connector. Guest must insure that the pointed data is actually
     * in the guest RAM and remains locked there for entire request processing.
     */

    if (cParms != 0)
    {
        /* Compute addresses of host parms array and first memory buffer. */
        VBOXHGCMSVCPARM *pHostParm = (VBOXHGCMSVCPARM *)((char *)pCmd + sizeof (struct VBOXHGCMCMD));

        uint8_t *pcBuf = (uint8_t *)pHostParm + cParms * sizeof (VBOXHGCMSVCPARM);

        pCmd->paHostParms = pHostParm;
        pCmd->cHostParms  = cParms;

        uint32_t iLinPtr = 0;
        RTGCPHYS *pPages  = (RTGCPHYS *)((uint8_t *)pCmd->paLinPtrs + sizeof (VBOXHGCMLINPTR) *cLinPtrs);

        if (f64Bits)
        {
#ifdef VBOX_WITH_64_BITS_GUESTS
            HGCMFunctionParameter64 *pGuestParm = VMMDEV_HGCM_CALL_PARMS64(pHGCMCall);
#else
            HGCMFunctionParameter *pGuestParm = VMMDEV_HGCM_CALL_PARMS(pHGCMCall);
            AssertFailed (); /* This code should not be called in this case */
#endif /* VBOX_WITH_64_BITS_GUESTS */


            for (i = 0; i < cParms && RT_SUCCESS(rc); i++, pGuestParm++, pHostParm++)
            {
                switch (pGuestParm->type)
                {
                     case VMMDevHGCMParmType_32bit:
                     {
                         uint32_t u32 = pGuestParm->u.value32;

                         pHostParm->type = VBOX_HGCM_SVC_PARM_32BIT;
                         pHostParm->u.uint32 = u32;

                         Log(("vmmdevHGCMCall: uint32 guest parameter %u\n", u32));
                         break;
                     }

                     case VMMDevHGCMParmType_64bit:
                     {
                         uint64_t u64 = pGuestParm->u.value64;

                         pHostParm->type = VBOX_HGCM_SVC_PARM_64BIT;
                         pHostParm->u.uint64 = u64;

                         Log(("vmmdevHGCMCall: uint64 guest parameter %llu\n", u64));
                         break;
                     }

                     case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
                     case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
                     case VMMDevHGCMParmType_LinAddr:     /* In & Out */
                     {
                         uint32_t size = pGuestParm->u.Pointer.size;
                         RTGCPTR linearAddr = pGuestParm->u.Pointer.u.linearAddr;

                         pHostParm->type = VBOX_HGCM_SVC_PARM_PTR;
                         pHostParm->u.pointer.size = size;

                         /* Copy guest data to an allocated buffer, so
                          * services can use the data.
                          */

                         if (size == 0)
                         {
                             pHostParm->u.pointer.addr = NULL;
                         }
                         else
                         {
                             /* Don't overdo it */
                             if (pGuestParm->type != VMMDevHGCMParmType_LinAddr_Out)
                                 rc = PDMDevHlpPhysReadGCVirt(pThis->pDevIns, pcBuf, linearAddr, size);
                             else
                                 rc = VINF_SUCCESS;

                             if (RT_SUCCESS(rc))
                             {
                                 pHostParm->u.pointer.addr = pcBuf;
                                 pcBuf += size;

                                 /* Remember the guest physical pages that belong to the virtual address region.
                                  * Do it for all linear pointers because load state will require In pointer info too.
                                  */
                                 rc = vmmdevHGCMSaveLinPtr (pThis->pDevIns, i, linearAddr, size, iLinPtr, pCmd->paLinPtrs, &pPages);

                                 iLinPtr++;
                             }
                         }

                         Log(("vmmdevHGCMCall: LinAddr guest parameter %RGv, rc = %Rrc\n", linearAddr, rc));
                         break;
                     }

                     case VMMDevHGCMParmType_PageList:
                     {
                         uint32_t size = pGuestParm->u.PageList.size;

                         /* Check that the page list info is within the request. */
                         if (   cbHGCMCall < sizeof (HGCMPageListInfo)
                             || pGuestParm->u.PageList.offset > cbHGCMCall - sizeof (HGCMPageListInfo))
                         {
                             rc = VERR_INVALID_PARAMETER;
                             break;
                         }

                         /* At least the structure is within. */
                         HGCMPageListInfo *pPageListInfo = (HGCMPageListInfo *)((uint8_t *)pHGCMCall + pGuestParm->u.PageList.offset);

                         uint32_t cbPageListInfo = sizeof (HGCMPageListInfo) + (pPageListInfo->cPages - 1) * sizeof (pPageListInfo->aPages[0]);

                         if (   pPageListInfo->cPages == 0
                             || cbHGCMCall < pGuestParm->u.PageList.offset + cbPageListInfo)
                         {
                             rc = VERR_INVALID_PARAMETER;
                             break;
                         }

                         pHostParm->type = VBOX_HGCM_SVC_PARM_PTR;
                         pHostParm->u.pointer.size = size;

                         /* Copy guest data to an allocated buffer, so
                          * services can use the data.
                          */

                         if (size == 0)
                         {
                             pHostParm->u.pointer.addr = NULL;
                         }
                         else
                         {
                             if (pPageListInfo->flags & VBOX_HGCM_F_PARM_DIRECTION_TO_HOST)
                             {
                                 /* Copy pages to the pcBuf[size]. */
                                 rc = vmmdevHGCMPageListRead(pThis->pDevIns, pcBuf, size, pPageListInfo);
                             }
                             else
                                 rc = VINF_SUCCESS;

                             if (RT_SUCCESS(rc))
                             {
                                 pHostParm->u.pointer.addr = pcBuf;
                                 pcBuf += size;
                             }
                         }

                         Log(("vmmdevHGCMCall: PageList guest parameter rc = %Rrc\n", rc));
                         break;
                     }

                    /* just to shut up gcc */
                    default:
                        AssertFailed();
                        break;
                }
            }
        }
        else
        {
#ifdef VBOX_WITH_64_BITS_GUESTS
            HGCMFunctionParameter32 *pGuestParm = VMMDEV_HGCM_CALL_PARMS32(pHGCMCall);
#else
            HGCMFunctionParameter *pGuestParm = VMMDEV_HGCM_CALL_PARMS(pHGCMCall);
#endif /* VBOX_WITH_64_BITS_GUESTS */

            for (i = 0; i < cParms && RT_SUCCESS(rc); i++, pGuestParm++, pHostParm++)
            {
                switch (pGuestParm->type)
                {
                     case VMMDevHGCMParmType_32bit:
                     {
                         uint32_t u32 = pGuestParm->u.value32;

                         pHostParm->type = VBOX_HGCM_SVC_PARM_32BIT;
                         pHostParm->u.uint32 = u32;

                         Log(("vmmdevHGCMCall: uint32 guest parameter %u\n", u32));
                         break;
                     }

                     case VMMDevHGCMParmType_64bit:
                     {
                         uint64_t u64 = pGuestParm->u.value64;

                         pHostParm->type = VBOX_HGCM_SVC_PARM_64BIT;
                         pHostParm->u.uint64 = u64;

                         Log(("vmmdevHGCMCall: uint64 guest parameter %llu\n", u64));
                         break;
                     }

                     case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
                     case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
                     case VMMDevHGCMParmType_LinAddr:     /* In & Out */
                     {
                         uint32_t size = pGuestParm->u.Pointer.size;
                         RTGCPTR linearAddr = pGuestParm->u.Pointer.u.linearAddr;

                         pHostParm->type = VBOX_HGCM_SVC_PARM_PTR;
                         pHostParm->u.pointer.size = size;

                         /* Copy guest data to an allocated buffer, so
                          * services can use the data.
                          */

                         if (size == 0)
                         {
                             pHostParm->u.pointer.addr = NULL;
                         }
                         else
                         {
                             /* Don't overdo it */
                             if (pGuestParm->type != VMMDevHGCMParmType_LinAddr_Out)
                                 rc = PDMDevHlpPhysReadGCVirt(pThis->pDevIns, pcBuf, linearAddr, size);
                             else
                                 rc = VINF_SUCCESS;

                             if (RT_SUCCESS(rc))
                             {
                                 pHostParm->u.pointer.addr = pcBuf;
                                 pcBuf += size;

                                 /* Remember the guest physical pages that belong to the virtual address region.
                                  * Do it for all linear pointers because load state will require In pointer info too.
                                  */
                                 rc = vmmdevHGCMSaveLinPtr (pThis->pDevIns, i, linearAddr, size, iLinPtr, pCmd->paLinPtrs, &pPages);

                                 iLinPtr++;
                             }
                         }

                         Log(("vmmdevHGCMCall: LinAddr guest parameter %RGv, rc = %Rrc\n", linearAddr, rc));
                         break;
                     }

                     case VMMDevHGCMParmType_PageList:
                     {
                         uint32_t size = pGuestParm->u.PageList.size;

                         /* Check that the page list info is within the request. */
                         if (   cbHGCMCall < sizeof (HGCMPageListInfo)
                             || pGuestParm->u.PageList.offset > cbHGCMCall - sizeof (HGCMPageListInfo))
                         {
                             rc = VERR_INVALID_PARAMETER;
                             break;
                         }

                         /* At least the structure is within. */
                         HGCMPageListInfo *pPageListInfo = (HGCMPageListInfo *)((uint8_t *)pHGCMCall + pGuestParm->u.PageList.offset);

                         uint32_t cbPageListInfo = sizeof (HGCMPageListInfo) + (pPageListInfo->cPages - 1) * sizeof (pPageListInfo->aPages[0]);

                         if (   pPageListInfo->cPages == 0
                             || cbHGCMCall < pGuestParm->u.PageList.offset + cbPageListInfo)
                         {
                             rc = VERR_INVALID_PARAMETER;
                             break;
                         }

                         pHostParm->type = VBOX_HGCM_SVC_PARM_PTR;
                         pHostParm->u.pointer.size = size;

                         /* Copy guest data to an allocated buffer, so
                          * services can use the data.
                          */

                         if (size == 0)
                         {
                             pHostParm->u.pointer.addr = NULL;
                         }
                         else
                         {
                             if (pPageListInfo->flags & VBOX_HGCM_F_PARM_DIRECTION_TO_HOST)
                             {
                                 /* Copy pages to the pcBuf[size]. */
                                 rc = vmmdevHGCMPageListRead(pThis->pDevIns, pcBuf, size, pPageListInfo);
                             }
                             else
                                 rc = VINF_SUCCESS;

                             if (RT_SUCCESS(rc))
                             {
                                 pHostParm->u.pointer.addr = pcBuf;
                                 pcBuf += size;
                             }
                         }

                         Log(("vmmdevHGCMCall: PageList guest parameter rc = %Rrc\n", rc));
                         break;
                     }

                    /* just to shut up gcc */
                    default:
                        AssertFailed();
                        break;
                }
            }
        }
    }

    if (RT_SUCCESS (rc))
    {
        vmmdevHGCMAddCommand (pThis, pCmd, GCPhys, pHGCMCall->header.header.size, VBOXHGCMCMDTYPE_CALL);

        /* Pass the function call to HGCM connector for actual processing */
        rc = pThis->pHGCMDrv->pfnCall (pThis->pHGCMDrv, pCmd, pHGCMCall->u32ClientID,
                                              pHGCMCall->u32Function, cParms, pCmd->paHostParms);

        if (RT_FAILURE (rc))
        {
            Log(("vmmdevHGCMCall: pfnCall failed rc = %Rrc\n", rc));
            vmmdevHGCMRemoveCommand (pThis, pCmd);
        }
    }

    if (RT_FAILURE (rc))
    {
        if (pCmd->paLinPtrs)
        {
            RTMemFree (pCmd->paLinPtrs);
        }

        RTMemFree (pCmd);
    }

    return rc;
}

static void logRelLoadStatePointerIndexMismatch (uint32_t iParm, uint32_t iSavedParm, int iLinPtr, int cLinPtrs)
{
   LogRel(("Warning: VMMDev load state: a pointer parameter index mismatch %d (expected %d) (%d/%d)\n",
           (int)iParm, (int)iSavedParm, iLinPtr, cLinPtrs));
}

static void logRelLoadStateBufferSizeMismatch (uint32_t size, uint32_t iPage, uint32_t cPages)
{
    LogRel(("Warning: VMMDev load state: buffer size mismatch: size %d, page %d/%d\n",
            (int)size, (int)iPage, (int)cPages));
}


static int vmmdevHGCMCallSaved (PVMMDEV pThis, VMMDevHGCMCall *pHGCMCall, RTGCPHYS GCPhys, uint32_t cbHGCMCall, bool f64Bits, bool *pfHGCMCalled, VBOXHGCMCMD *pSavedCmd, VBOXHGCMCMD **ppCmd)
{
    int rc = VINF_SUCCESS;

    Log(("vmmdevHGCMCallSaved: client id = %d, function = %d, %s bit\n", pHGCMCall->u32ClientID, pHGCMCall->u32Function, f64Bits? "64": "32"));

    /* Compute size and allocate memory block to hold:
     *    struct VBOXHGCMCMD
     *    VBOXHGCMSVCPARM[cParms]
     *    memory buffers for pointer parameters.
     */

    uint32_t cParms = pHGCMCall->cParms;

    Log(("vmmdevHGCMCall: cParms = %d\n", cParms));

    /*
     * Sane upper limit.
     */
    if (cParms > VMMDEV_MAX_HGCM_PARMS)
    {
        LogRelMax(50, ("VMMDev: request packet with too many parameters (%d). Refusing operation.\n", cParms));
        return VERR_INVALID_PARAMETER;
    }

    /*
     * Compute size of required memory buffer.
     */

    uint32_t cbCmdSize = sizeof (struct VBOXHGCMCMD) + cParms * sizeof (VBOXHGCMSVCPARM);

    uint32_t i;

    int32_t cLinPtrs = 0;
    int32_t cLinPtrPages = 0;

    if (f64Bits)
    {
#ifdef VBOX_WITH_64_BITS_GUESTS
        HGCMFunctionParameter64 *pGuestParm = VMMDEV_HGCM_CALL_PARMS64(pHGCMCall);
#else
        HGCMFunctionParameter *pGuestParm = VMMDEV_HGCM_CALL_PARMS(pHGCMCall);
        AssertFailed (); /* This code should not be called in this case */
#endif /* VBOX_WITH_64_BITS_GUESTS */

        /* Look for pointer parameters, which require a host buffer. */
        for (i = 0; i < cParms && RT_SUCCESS(rc); i++, pGuestParm++)
        {
            switch (pGuestParm->type)
            {
                case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
                case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
                case VMMDevHGCMParmType_LinAddr:     /* In & Out */
                {
                    if (pGuestParm->u.Pointer.size > 0)
                    {
                        /* Only pointers with some actual data are counted. */
                        cbCmdSize += pGuestParm->u.Pointer.size;

                        cLinPtrs++;
                        /* Take the offset into the current page also into account! */
                        cLinPtrPages += ((pGuestParm->u.Pointer.u.linearAddr & PAGE_OFFSET_MASK)
                                          + pGuestParm->u.Pointer.size + PAGE_SIZE - 1) / PAGE_SIZE;
                    }

                    Log(("vmmdevHGCMCall: linptr size = %d\n", pGuestParm->u.Pointer.size));
                } break;

                case VMMDevHGCMParmType_PageList:
                {
                    cbCmdSize += pGuestParm->u.PageList.size;
                    Log(("vmmdevHGCMCall: pagelist size = %d\n", pGuestParm->u.PageList.size));
                } break;

                case VMMDevHGCMParmType_32bit:
                case VMMDevHGCMParmType_64bit:
                {
                } break;

                default:
                case VMMDevHGCMParmType_PhysAddr:
                {
                    AssertMsgFailed(("vmmdevHGCMCall: invalid parameter type %x\n", pGuestParm->type));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }
            }
        }
    }
    else
    {
#ifdef VBOX_WITH_64_BITS_GUESTS
        HGCMFunctionParameter32 *pGuestParm = VMMDEV_HGCM_CALL_PARMS32(pHGCMCall);
#else
        HGCMFunctionParameter *pGuestParm = VMMDEV_HGCM_CALL_PARMS(pHGCMCall);
#endif /* VBOX_WITH_64_BITS_GUESTS */

        /* Look for pointer parameters, which require a host buffer. */
        for (i = 0; i < cParms && RT_SUCCESS(rc); i++, pGuestParm++)
        {
            switch (pGuestParm->type)
            {
                case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
                case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
                case VMMDevHGCMParmType_LinAddr:     /* In & Out */
                {
                    if (pGuestParm->u.Pointer.size > 0)
                    {
                        /* Only pointers with some actual data are counted. */
                        cbCmdSize += pGuestParm->u.Pointer.size;

                        cLinPtrs++;
                        /* Take the offset into the current page also into account! */
                        cLinPtrPages += ((pGuestParm->u.Pointer.u.linearAddr & PAGE_OFFSET_MASK)
                                          + pGuestParm->u.Pointer.size + PAGE_SIZE - 1) / PAGE_SIZE;
                    }

                    Log(("vmmdevHGCMCall: linptr size = %d\n", pGuestParm->u.Pointer.size));
                } break;

                case VMMDevHGCMParmType_PageList:
                {
                    cbCmdSize += pGuestParm->u.PageList.size;
                    Log(("vmmdevHGCMCall: pagelist size = %d\n", pGuestParm->u.PageList.size));
                } break;

                case VMMDevHGCMParmType_32bit:
                case VMMDevHGCMParmType_64bit:
                {
                } break;

                default:
                {
                    AssertMsgFailed(("vmmdevHGCMCall: invalid parameter type %x\n", pGuestParm->type));
                    rc = VERR_INVALID_PARAMETER;
                    break;
                }
            }
        }
    }

    if (RT_FAILURE (rc))
    {
        return rc;
    }

    if (   pSavedCmd->cLinPtrs     != cLinPtrs
        || pSavedCmd->cLinPtrPages != cLinPtrPages)
    {
        LogRel(("VMMDev: invalid saved command ptrs: %d/%d, pages %d/%d\n",
                pSavedCmd->cLinPtrs, cLinPtrs, pSavedCmd->cLinPtrPages, cLinPtrPages));
        AssertFailed();
        return VERR_INVALID_PARAMETER;
    }

    PVBOXHGCMCMD pCmd = (PVBOXHGCMCMD)RTMemAllocZ (cbCmdSize);

    if (pCmd == NULL)
    {
        return VERR_NO_MEMORY;
    }

    vmmdevRestoreSavedCommand(pCmd, pSavedCmd);
    *ppCmd = pCmd;

    vmmdevHGCMAddCommand (pThis, pCmd, GCPhys, pHGCMCall->header.header.size, VBOXHGCMCMDTYPE_CALL);

    /* Process parameters, changing them to host context pointers for easy
     * processing by connector. Guest must insure that the pointed data is actually
     * in the guest RAM and remains locked there for entire request processing.
     */

    if (cParms != 0)
    {
        /* Compute addresses of host parms array and first memory buffer. */
        VBOXHGCMSVCPARM *pHostParm = (VBOXHGCMSVCPARM *)((uint8_t *)pCmd + sizeof (struct VBOXHGCMCMD));

        uint8_t *pu8Buf = (uint8_t *)pHostParm + cParms * sizeof (VBOXHGCMSVCPARM);

        pCmd->paHostParms = pHostParm;
        pCmd->cHostParms = cParms;

        uint32_t iParm;
        int iLinPtr = 0;

        if (f64Bits)
        {
#ifdef VBOX_WITH_64_BITS_GUESTS
            HGCMFunctionParameter64 *pGuestParm = VMMDEV_HGCM_CALL_PARMS64(pHGCMCall);
#else
            HGCMFunctionParameter *pGuestParm = VMMDEV_HGCM_CALL_PARMS(pHGCMCall);
            AssertFailed (); /* This code should not be called in this case */
#endif /* VBOX_WITH_64_BITS_GUESTS */

            for (iParm = 0; iParm < cParms && RT_SUCCESS(rc); iParm++, pGuestParm++, pHostParm++)
            {
                switch (pGuestParm->type)
                {
                     case VMMDevHGCMParmType_32bit:
                     {
                         uint32_t u32 = pGuestParm->u.value32;

                         pHostParm->type = VBOX_HGCM_SVC_PARM_32BIT;
                         pHostParm->u.uint32 = u32;

                         Log(("vmmdevHGCMCall: uint32 guest parameter %u\n", u32));
                         break;
                     }

                     case VMMDevHGCMParmType_64bit:
                     {
                         uint64_t u64 = pGuestParm->u.value64;

                         pHostParm->type = VBOX_HGCM_SVC_PARM_64BIT;
                         pHostParm->u.uint64 = u64;

                         Log(("vmmdevHGCMCall: uint64 guest parameter %llu\n", u64));
                         break;
                     }

                     case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
                     case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
                     case VMMDevHGCMParmType_LinAddr:     /* In & Out */
                     {
                         uint32_t size = pGuestParm->u.Pointer.size;

                         pHostParm->type = VBOX_HGCM_SVC_PARM_PTR;
                         pHostParm->u.pointer.size = size;

                         /* Copy guest data to an allocated buffer, so
                          * services can use the data.
                          */

                         if (size == 0)
                         {
                             pHostParm->u.pointer.addr = NULL;
                         }
                         else
                         {
                             /* The saved command already have the page list in pCmd->paLinPtrs.
                              * Read data from guest pages.
                              */
                             /* Don't overdo it */
                             if (pGuestParm->type != VMMDevHGCMParmType_LinAddr_Out)
                             {
                                 if (   iLinPtr >= pCmd->cLinPtrs
                                     || pCmd->paLinPtrs[iLinPtr].iParm != iParm)
                                 {
                                     logRelLoadStatePointerIndexMismatch (iParm, pCmd->paLinPtrs[iLinPtr].iParm, iLinPtr, pCmd->cLinPtrs);
                                     rc = VERR_INVALID_PARAMETER;
                                 }
                                 else
                                 {
                                     VBOXHGCMLINPTR *pLinPtr = &pCmd->paLinPtrs[iLinPtr];

                                     uint32_t iPage;
                                     uint32_t offPage = pLinPtr->offFirstPage;
                                     size_t cbRemaining = size;
                                     uint8_t *pu8Dst = pu8Buf;
                                     for (iPage = 0; iPage < pLinPtr->cPages; iPage++)
                                     {
                                         if (cbRemaining == 0)
                                         {
                                             logRelLoadStateBufferSizeMismatch (size, iPage, pLinPtr->cPages);
                                             break;
                                         }

                                         size_t cbChunk = PAGE_SIZE - offPage;

                                         if (cbChunk > cbRemaining)
                                         {
                                             cbChunk = cbRemaining;
                                         }

                                         rc = PDMDevHlpPhysRead(pThis->pDevIns,
                                                                pLinPtr->paPages[iPage] + offPage,
                                                                pu8Dst, cbChunk);

                                         AssertRCBreak(rc);

                                         offPage = 0; /* A next page is read from 0 offset. */
                                         cbRemaining -= cbChunk;
                                         pu8Dst += cbChunk;
                                     }
                                 }
                             }
                             else
                                 rc = VINF_SUCCESS;

                             if (RT_SUCCESS(rc))
                             {
                                 pHostParm->u.pointer.addr = pu8Buf;
                                 pu8Buf += size;

                                 iLinPtr++;
                             }
                         }

                         Log(("vmmdevHGCMCall: LinAddr guest parameter %RGv, rc = %Rrc\n",
                              pGuestParm->u.Pointer.u.linearAddr, rc));
                         break;
                     }

                     case VMMDevHGCMParmType_PageList:
                     {
                         uint32_t size = pGuestParm->u.PageList.size;

                         /* Check that the page list info is within the request. */
                         if (   cbHGCMCall < sizeof (HGCMPageListInfo)
                             || pGuestParm->u.PageList.offset > cbHGCMCall - sizeof (HGCMPageListInfo))
                         {
                             rc = VERR_INVALID_PARAMETER;
                             break;
                         }

                         /* At least the structure is within. */
                         HGCMPageListInfo *pPageListInfo = (HGCMPageListInfo *)((uint8_t *)pHGCMCall + pGuestParm->u.PageList.offset);

                         uint32_t cbPageListInfo = sizeof (HGCMPageListInfo) + (pPageListInfo->cPages - 1) * sizeof (pPageListInfo->aPages[0]);

                         if (   pPageListInfo->cPages == 0
                             || cbHGCMCall < pGuestParm->u.PageList.offset + cbPageListInfo)
                         {
                             rc = VERR_INVALID_PARAMETER;
                             break;
                         }

                         pHostParm->type = VBOX_HGCM_SVC_PARM_PTR;
                         pHostParm->u.pointer.size = size;

                         /* Copy guest data to an allocated buffer, so
                          * services can use the data.
                          */

                         if (size == 0)
                         {
                             pHostParm->u.pointer.addr = NULL;
                         }
                         else
                         {
                             if (pPageListInfo->flags & VBOX_HGCM_F_PARM_DIRECTION_TO_HOST)
                             {
                                 /* Copy pages to the pcBuf[size]. */
                                 rc = vmmdevHGCMPageListRead(pThis->pDevIns, pu8Buf, size, pPageListInfo);
                             }
                             else
                                 rc = VINF_SUCCESS;

                             if (RT_SUCCESS(rc))
                             {
                                 pHostParm->u.pointer.addr = pu8Buf;
                                 pu8Buf += size;
                             }
                         }

                         Log(("vmmdevHGCMCall: PageList guest parameter rc = %Rrc\n", rc));
                         break;
                     }

                    /* just to shut up gcc */
                    default:
                        AssertFailed();
                        break;
                }
            }
        }
        else
        {
#ifdef VBOX_WITH_64_BITS_GUESTS
            HGCMFunctionParameter32 *pGuestParm = VMMDEV_HGCM_CALL_PARMS32(pHGCMCall);
#else
            HGCMFunctionParameter *pGuestParm = VMMDEV_HGCM_CALL_PARMS(pHGCMCall);
#endif /* VBOX_WITH_64_BITS_GUESTS */

            for (iParm = 0; iParm < cParms && RT_SUCCESS(rc); iParm++, pGuestParm++, pHostParm++)
            {
                switch (pGuestParm->type)
                {
                     case VMMDevHGCMParmType_32bit:
                     {
                         uint32_t u32 = pGuestParm->u.value32;

                         pHostParm->type = VBOX_HGCM_SVC_PARM_32BIT;
                         pHostParm->u.uint32 = u32;

                         Log(("vmmdevHGCMCall: uint32 guest parameter %u\n", u32));
                         break;
                     }

                     case VMMDevHGCMParmType_64bit:
                     {
                         uint64_t u64 = pGuestParm->u.value64;

                         pHostParm->type = VBOX_HGCM_SVC_PARM_64BIT;
                         pHostParm->u.uint64 = u64;

                         Log(("vmmdevHGCMCall: uint64 guest parameter %llu\n", u64));
                         break;
                     }

                     case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
                     case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
                     case VMMDevHGCMParmType_LinAddr:     /* In & Out */
                     {
                         uint32_t size = pGuestParm->u.Pointer.size;

                         pHostParm->type = VBOX_HGCM_SVC_PARM_PTR;
                         pHostParm->u.pointer.size = size;

                         /* Copy guest data to an allocated buffer, so
                          * services can use the data.
                          */

                         if (size == 0)
                         {
                             pHostParm->u.pointer.addr = NULL;
                         }
                         else
                         {
                             /* The saved command already have the page list in pCmd->paLinPtrs.
                              * Read data from guest pages.
                              */
                             /* Don't overdo it */
                             if (pGuestParm->type != VMMDevHGCMParmType_LinAddr_Out)
                             {
                                 if (   iLinPtr >= pCmd->cLinPtrs
                                     || pCmd->paLinPtrs[iLinPtr].iParm != iParm)
                                 {
                                     logRelLoadStatePointerIndexMismatch (iParm, pCmd->paLinPtrs[iLinPtr].iParm, iLinPtr, pCmd->cLinPtrs);
                                     rc = VERR_INVALID_PARAMETER;
                                 }
                                 else
                                 {
                                     VBOXHGCMLINPTR *pLinPtr = &pCmd->paLinPtrs[iLinPtr];

                                     uint32_t iPage;
                                     uint32_t offPage = pLinPtr->offFirstPage;
                                     size_t cbRemaining = size;
                                     uint8_t *pu8Dst = pu8Buf;
                                     for (iPage = 0; iPage < pLinPtr->cPages; iPage++)
                                     {
                                         if (cbRemaining == 0)
                                         {
                                             logRelLoadStateBufferSizeMismatch (size, iPage, pLinPtr->cPages);
                                             break;
                                         }

                                         size_t cbChunk = PAGE_SIZE - offPage;

                                         if (cbChunk > cbRemaining)
                                         {
                                             cbChunk = cbRemaining;
                                         }

                                         rc = PDMDevHlpPhysRead(pThis->pDevIns,
                                                                pLinPtr->paPages[iPage] + offPage,
                                                                pu8Dst, cbChunk);

                                         AssertRCBreak(rc);

                                         offPage = 0; /* A next page is read from 0 offset. */
                                         cbRemaining -= cbChunk;
                                         pu8Dst += cbChunk;
                                     }
                                 }
                             }
                             else
                                 rc = VINF_SUCCESS;

                             if (RT_SUCCESS(rc))
                             {
                                 pHostParm->u.pointer.addr = pu8Buf;
                                 pu8Buf += size;

                                 iLinPtr++;
                             }
                         }

                         Log(("vmmdevHGCMCall: LinAddr guest parameter %RGv, rc = %Rrc\n",
                              pGuestParm->u.Pointer.u.linearAddr, rc));
                         break;
                     }

                     case VMMDevHGCMParmType_PageList:
                     {
                         uint32_t size = pGuestParm->u.PageList.size;

                         /* Check that the page list info is within the request. */
                         if (   cbHGCMCall < sizeof (HGCMPageListInfo)
                             || pGuestParm->u.PageList.offset > cbHGCMCall - sizeof (HGCMPageListInfo))
                         {
                             rc = VERR_INVALID_PARAMETER;
                             break;
                         }

                         /* At least the structure is within. */
                         HGCMPageListInfo *pPageListInfo = (HGCMPageListInfo *)((uint8_t *)pHGCMCall + pGuestParm->u.PageList.offset);

                         uint32_t cbPageListInfo = sizeof (HGCMPageListInfo) + (pPageListInfo->cPages - 1) * sizeof (pPageListInfo->aPages[0]);

                         if (   pPageListInfo->cPages == 0
                             || cbHGCMCall < pGuestParm->u.PageList.offset + cbPageListInfo)
                         {
                             rc = VERR_INVALID_PARAMETER;
                             break;
                         }

                         pHostParm->type = VBOX_HGCM_SVC_PARM_PTR;
                         pHostParm->u.pointer.size = size;

                         /* Copy guest data to an allocated buffer, so
                          * services can use the data.
                          */

                         if (size == 0)
                         {
                             pHostParm->u.pointer.addr = NULL;
                         }
                         else
                         {
                             if (pPageListInfo->flags & VBOX_HGCM_F_PARM_DIRECTION_TO_HOST)
                             {
                                 /* Copy pages to the pcBuf[size]. */
                                 rc = vmmdevHGCMPageListRead(pThis->pDevIns, pu8Buf, size, pPageListInfo);
                             }
                             else
                                 rc = VINF_SUCCESS;

                             if (RT_SUCCESS(rc))
                             {
                                 pHostParm->u.pointer.addr = pu8Buf;
                                 pu8Buf += size;
                             }
                         }

                         Log(("vmmdevHGCMCall: PageList guest parameter rc = %Rrc\n", rc));
                         break;
                     }

                    /* just to shut up gcc */
                    default:
                        AssertFailed();
                        break;
                }
            }
        }
    }

    if (RT_SUCCESS (rc))
    {
        /* Pass the function call to HGCM connector for actual processing */
        rc = pThis->pHGCMDrv->pfnCall (pThis->pHGCMDrv, pCmd, pHGCMCall->u32ClientID, pHGCMCall->u32Function, cParms, pCmd->paHostParms);
        if (RT_SUCCESS (rc))
        {
            *pfHGCMCalled = true;
        }
        /* else
         *  ... the caller will also execute vmmdevHGCMRemoveCommand() for us */
    }

    return rc;
}

/**
 * VMMDevReq_HGCMCancel worker.
 *
 * @thread EMT
 */
int vmmdevHGCMCancel (PVMMDEV pThis, VMMDevHGCMCancel *pHGCMCancel, RTGCPHYS GCPhys)
{
    NOREF(pHGCMCancel);
    int rc = vmmdevHGCMCancel2(pThis, GCPhys);
    return rc == VERR_NOT_FOUND ? VERR_INVALID_PARAMETER : rc;
}

/**
 * VMMDevReq_HGCMCancel2 worker.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_NOT_FOUND if the request was not found.
 * @retval  VERR_INVALID_PARAMETER if the request address is invalid.
 *
 * @param   pThis       The VMMDev instance data.
 * @param   GCPhys      The address of the request that should be cancelled.
 *
 * @thread EMT
 */
int vmmdevHGCMCancel2 (PVMMDEV pThis, RTGCPHYS GCPhys)
{
    if (    GCPhys == 0
        ||  GCPhys == NIL_RTGCPHYS
        ||  GCPhys == NIL_RTGCPHYS32)
    {
        Log(("vmmdevHGCMCancel2: GCPhys=%#x\n", GCPhys));
        return VERR_INVALID_PARAMETER;
    }

    /*
     * Locate the command and cancel it while under the protection of
     * the lock. hgcmCompletedWorker makes assumptions about this.
     */
    int rc = vmmdevHGCMCmdListLock (pThis);
    AssertRCReturn(rc, rc);

    PVBOXHGCMCMD pCmd = vmmdevHGCMFindCommandLocked (pThis, GCPhys);
    if (pCmd)
    {
        pCmd->fCancelled = true;
        Log(("vmmdevHGCMCancel2: Cancelled pCmd=%p / GCPhys=%#x\n", pCmd, GCPhys));
    }
    else
        rc = VERR_NOT_FOUND;

    vmmdevHGCMCmdListUnlock (pThis);
    return rc;
}

static int vmmdevHGCMCmdVerify (PVBOXHGCMCMD pCmd, VMMDevHGCMRequestHeader *pHeader)
{
    switch (pCmd->enmCmdType)
    {
        case VBOXHGCMCMDTYPE_CONNECT:
            if (   pHeader->header.requestType == VMMDevReq_HGCMConnect
                || pHeader->header.requestType == VMMDevReq_HGCMCancel) return VINF_SUCCESS;
            break;

        case VBOXHGCMCMDTYPE_DISCONNECT:
            if (   pHeader->header.requestType == VMMDevReq_HGCMDisconnect
                || pHeader->header.requestType == VMMDevReq_HGCMCancel) return VINF_SUCCESS;
            break;

        case VBOXHGCMCMDTYPE_CALL:
#ifdef VBOX_WITH_64_BITS_GUESTS
            if (   pHeader->header.requestType == VMMDevReq_HGCMCall32
                || pHeader->header.requestType == VMMDevReq_HGCMCall64
                || pHeader->header.requestType == VMMDevReq_HGCMCancel) return VINF_SUCCESS;
#else
            if (   pHeader->header.requestType == VMMDevReq_HGCMCall
                || pHeader->header.requestType == VMMDevReq_HGCMCancel) return VINF_SUCCESS;
#endif /* VBOX_WITH_64_BITS_GUESTS */

            break;

        default:
            AssertFailed ();
    }

    LogRel(("VMMDEV: Invalid HGCM command: pCmd->enmCmdType = 0x%08X, pHeader->header.requestType = 0x%08X\n",
          pCmd->enmCmdType, pHeader->header.requestType));
    return VERR_INVALID_PARAMETER;
}

#ifdef VBOX_WITH_64_BITS_GUESTS
static int vmmdevHGCMParmVerify64(HGCMFunctionParameter64 *pGuestParm, VBOXHGCMSVCPARM *pHostParm)
{
    int rc = VERR_INVALID_PARAMETER;

    switch (pGuestParm->type)
    {
        case VMMDevHGCMParmType_32bit:
            if (pHostParm->type == VBOX_HGCM_SVC_PARM_32BIT)
                rc = VINF_SUCCESS;
            break;

        case VMMDevHGCMParmType_64bit:
            if (pHostParm->type == VBOX_HGCM_SVC_PARM_64BIT)
                rc = VINF_SUCCESS;
            break;

        case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
        case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
        case VMMDevHGCMParmType_LinAddr:     /* In & Out */
            if (   pHostParm->type == VBOX_HGCM_SVC_PARM_PTR
                && pGuestParm->u.Pointer.size >= pHostParm->u.pointer.size)
                rc = VINF_SUCCESS;
            break;

        case VMMDevHGCMParmType_PageList:
            if (   pHostParm->type == VBOX_HGCM_SVC_PARM_PTR
                && pGuestParm->u.PageList.size >= pHostParm->u.pointer.size)
                rc = VINF_SUCCESS;
            break;

        default:
            AssertLogRelMsgFailed(("hgcmCompleted: invalid parameter type %08X\n", pGuestParm->type));
            break;
    }

    return rc;
}
#endif /* VBOX_WITH_64_BITS_GUESTS */

#ifdef VBOX_WITH_64_BITS_GUESTS
static int vmmdevHGCMParmVerify32(HGCMFunctionParameter32 *pGuestParm, VBOXHGCMSVCPARM *pHostParm)
#else
static int vmmdevHGCMParmVerify32(HGCMFunctionParameter *pGuestParm, VBOXHGCMSVCPARM *pHostParm)
#endif
{
    int rc = VERR_INVALID_PARAMETER;

    switch (pGuestParm->type)
    {
        case VMMDevHGCMParmType_32bit:
            if (pHostParm->type == VBOX_HGCM_SVC_PARM_32BIT)
                rc = VINF_SUCCESS;
            break;

        case VMMDevHGCMParmType_64bit:
            if (pHostParm->type == VBOX_HGCM_SVC_PARM_64BIT)
                rc = VINF_SUCCESS;
            break;

        case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
        case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
        case VMMDevHGCMParmType_LinAddr:     /* In & Out */
            if (   pHostParm->type == VBOX_HGCM_SVC_PARM_PTR
                && pGuestParm->u.Pointer.size >= pHostParm->u.pointer.size)
                rc = VINF_SUCCESS;
            break;

        case VMMDevHGCMParmType_PageList:
            if (   pHostParm->type == VBOX_HGCM_SVC_PARM_PTR
                && pGuestParm->u.PageList.size >= pHostParm->u.pointer.size)
                rc = VINF_SUCCESS;
            break;

        default:
            AssertLogRelMsgFailed(("hgcmCompleted: invalid parameter type %08X\n", pGuestParm->type));
            break;
    }

    return rc;
}

DECLCALLBACK(void) hgcmCompletedWorker (PPDMIHGCMPORT pInterface, int32_t result, PVBOXHGCMCMD pCmd)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDevState, IHGCMPort);
#ifdef VBOX_WITH_DTRACE
    uint32_t idFunction = 0;
    uint32_t idClient   = 0;
#endif

    int rc = VINF_SUCCESS;

    if (result == VINF_HGCM_SAVE_STATE)
    {
        /* If the completion routine was called because HGCM saves its state,
         * then currently nothing to be done here. The pCmd stays in the list
         * and will be saved later when the VMMDev state will be saved.
         *
         * It it assumed that VMMDev saves state after the HGCM services,
         * and, therefore, VBOXHGCMCMD structures are not removed by
         * vmmdevHGCMSaveState from the list, while HGCM uses them.
         */
        LogFlowFunc(("VINF_HGCM_SAVE_STATE for command %p\n", pCmd));
        return;
    }

    /*
     * The cancellation protocol requires us to remove the command here
     * and then check the flag. Cancelled commands must not be written
     * back to guest memory.
     */
    VBOXDD_HGCMCALL_COMPLETED_EMT(pCmd, result);
    vmmdevHGCMRemoveCommand (pThis, pCmd);

    if (pCmd->fCancelled)
    {
        LogFlowFunc(("A cancelled command %p: %d\n", pCmd, pCmd->fCancelled));
    }
    else
    {
        /* Preallocated block for requests which have up to 8 parameters (most of requests). */
#ifdef VBOX_WITH_64_BITS_GUESTS
        uint8_t au8Prealloc[sizeof (VMMDevHGCMCall) + 8 * sizeof (HGCMFunctionParameter64)];
#else
        uint8_t au8Prealloc[sizeof (VMMDevHGCMCall) + 8 * sizeof (HGCMFunctionParameter)];
#endif /* VBOX_WITH_64_BITS_GUESTS */

        VMMDevHGCMRequestHeader *pHeader;

        if (pCmd->cbSize <= sizeof (au8Prealloc))
        {
            pHeader = (VMMDevHGCMRequestHeader *)&au8Prealloc[0];
        }
        else
        {
            pHeader = (VMMDevHGCMRequestHeader *)RTMemAlloc (pCmd->cbSize);
            if (pHeader == NULL)
            {
                LogRel(("VMMDev: Failed to allocate %u bytes for HGCM request completion!!!\n", pCmd->cbSize));

                /* Free it. The command have to be excluded from list of active commands anyway. */
                RTMemFree (pCmd);
                return;
            }
        }

        /*
         * Enter and leave the critical section here so we make sure
         * vmmdevRequestHandler has completed before we read & write
         * the request. (This isn't 100% optimal, but it solves the
         * 3.0 blocker.)
         */
        /** @todo It would be faster if this interface would use MMIO2 memory and we
         *        didn't have to mess around with PDMDevHlpPhysRead/Write. We're
         *        reading the header 3 times now and writing the request back twice. */

        PDMCritSectEnter(&pThis->CritSect, VERR_SEM_BUSY);
        PDMCritSectLeave(&pThis->CritSect);

        PDMDevHlpPhysRead(pThis->pDevIns, pCmd->GCPhys, pHeader, pCmd->cbSize);

        /* Setup return codes. */
        pHeader->result = result;

        /* Verify the request type. */
        rc = vmmdevHGCMCmdVerify (pCmd, pHeader);

        if (RT_SUCCESS (rc))
        {
            /* Update parameters and data buffers. */

            switch (pHeader->header.requestType)
            {
#ifdef VBOX_WITH_64_BITS_GUESTS
            case VMMDevReq_HGCMCall64:
            {
                VMMDevHGCMCall *pHGCMCall = (VMMDevHGCMCall *)pHeader;

                uint32_t cParms = pHGCMCall->cParms;
                if (cParms != pCmd->cHostParms)
                    rc = VERR_INVALID_PARAMETER;

                VBOXHGCMSVCPARM *pHostParm = pCmd->paHostParms;

                uint32_t i;
                uint32_t iLinPtr = 0;

                HGCMFunctionParameter64 *pGuestParm = VMMDEV_HGCM_CALL_PARMS64(pHGCMCall);

                for (i = 0; i < cParms && RT_SUCCESS(rc); i++, pGuestParm++, pHostParm++)
                {
                    rc = vmmdevHGCMParmVerify64(pGuestParm, pHostParm);
                    if (RT_FAILURE(rc))
                        break;

                    switch (pGuestParm->type)
                    {
                        case VMMDevHGCMParmType_32bit:
                        {
                            pGuestParm->u.value32 = pHostParm->u.uint32;
                        } break;

                        case VMMDevHGCMParmType_64bit:
                        {
                            pGuestParm->u.value64 = pHostParm->u.uint64;
                        } break;

                        case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
                        case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
                        case VMMDevHGCMParmType_LinAddr:     /* In & Out */
                        {
                            /* Copy buffer back to guest memory. */
                            uint32_t size = pGuestParm->u.Pointer.size;

                            if (size > 0)
                            {
                                if (pGuestParm->type != VMMDevHGCMParmType_LinAddr_In)
                                {
                                    /* Use the saved page list to write data back to the guest RAM. */
                                    rc = vmmdevHGCMWriteLinPtr (pThis->pDevIns, i, pHostParm->u.pointer.addr,
                                                                size, iLinPtr, pCmd->paLinPtrs);
                                }

                                /* All linptrs with size > 0 were saved. Advance the index to the next linptr. */
                                iLinPtr++;
                            }
                        } break;

                        case VMMDevHGCMParmType_PageList:
                        {
                            uint32_t cbHGCMCall = pCmd->cbSize; /* Size of the request. */

                            uint32_t size = pGuestParm->u.PageList.size;

                            /* Check that the page list info is within the request. */
                            if (   cbHGCMCall < sizeof (HGCMPageListInfo)
                                || pGuestParm->u.PageList.offset > cbHGCMCall - sizeof (HGCMPageListInfo))
                            {
                                rc = VERR_INVALID_PARAMETER;
                                break;
                            }

                            /* At least the structure is within. */
                            HGCMPageListInfo *pPageListInfo = (HGCMPageListInfo *)((uint8_t *)pHGCMCall + pGuestParm->u.PageList.offset);

                            uint32_t cbPageListInfo = sizeof (HGCMPageListInfo) + (pPageListInfo->cPages - 1) * sizeof (pPageListInfo->aPages[0]);

                            if (   pPageListInfo->cPages == 0
                                || cbHGCMCall < pGuestParm->u.PageList.offset + cbPageListInfo)
                            {
                                rc = VERR_INVALID_PARAMETER;
                                break;
                            }

                            if (size > 0)
                            {
                                if (pPageListInfo->flags & VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST)
                                {
                                    /* Copy pHostParm->u.pointer.addr[pHostParm->u.pointer.size] to pages. */
                                    rc = vmmdevHGCMPageListWrite(pThis->pDevIns, pPageListInfo, pHostParm->u.pointer.addr, size);
                                }
                                else
                                    rc = VINF_SUCCESS;
                            }

                            Log(("vmmdevHGCMCall: PageList guest parameter rc = %Rrc\n", rc));
                        } break;

                        default:
                        {
                            /* This indicates that the guest request memory was corrupted. */
                            rc = VERR_INVALID_PARAMETER;
                            break;
                        }
                    }
                }
# ifdef VBOX_WITH_DTRACE
                idFunction = pHGCMCall->u32Function;
                idClient   = pHGCMCall->u32ClientID;
# endif
                break;
            }

            case VMMDevReq_HGCMCall32:
            {
                VMMDevHGCMCall *pHGCMCall = (VMMDevHGCMCall *)pHeader;

                uint32_t cParms = pHGCMCall->cParms;
                if (cParms != pCmd->cHostParms)
                    rc = VERR_INVALID_PARAMETER;

                VBOXHGCMSVCPARM *pHostParm = pCmd->paHostParms;

                uint32_t i;
                uint32_t iLinPtr = 0;

                HGCMFunctionParameter32 *pGuestParm = VMMDEV_HGCM_CALL_PARMS32(pHGCMCall);

                for (i = 0; i < cParms && RT_SUCCESS(rc); i++, pGuestParm++, pHostParm++)
                {
                    rc = vmmdevHGCMParmVerify32(pGuestParm, pHostParm);
                    if (RT_FAILURE(rc))
                        break;

                    switch (pGuestParm->type)
                    {
                        case VMMDevHGCMParmType_32bit:
                        {
                            pGuestParm->u.value32 = pHostParm->u.uint32;
                        } break;

                        case VMMDevHGCMParmType_64bit:
                        {
                            pGuestParm->u.value64 = pHostParm->u.uint64;
                        } break;

                        case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
                        case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
                        case VMMDevHGCMParmType_LinAddr:     /* In & Out */
                        {
                            /* Copy buffer back to guest memory. */
                            uint32_t size = pGuestParm->u.Pointer.size;

                            if (size > 0)
                            {
                                if (pGuestParm->type != VMMDevHGCMParmType_LinAddr_In)
                                {
                                    /* Use the saved page list to write data back to the guest RAM. */
                                    rc = vmmdevHGCMWriteLinPtr (pThis->pDevIns, i, pHostParm->u.pointer.addr, size, iLinPtr, pCmd->paLinPtrs);
                                }

                                /* All linptrs with size > 0 were saved. Advance the index to the next linptr. */
                                iLinPtr++;
                            }
                        } break;

                        case VMMDevHGCMParmType_PageList:
                        {
                            uint32_t cbHGCMCall = pCmd->cbSize; /* Size of the request. */

                            uint32_t size = pGuestParm->u.PageList.size;

                            /* Check that the page list info is within the request. */
                            if (   cbHGCMCall < sizeof (HGCMPageListInfo)
                                || pGuestParm->u.PageList.offset > cbHGCMCall - sizeof (HGCMPageListInfo))
                            {
                                rc = VERR_INVALID_PARAMETER;
                                break;
                            }

                            /* At least the structure is within. */
                            HGCMPageListInfo *pPageListInfo = (HGCMPageListInfo *)((uint8_t *)pHGCMCall + pGuestParm->u.PageList.offset);

                            uint32_t cbPageListInfo = sizeof (HGCMPageListInfo) + (pPageListInfo->cPages - 1) * sizeof (pPageListInfo->aPages[0]);

                            if (   pPageListInfo->cPages == 0
                                || cbHGCMCall < pGuestParm->u.PageList.offset + cbPageListInfo)
                            {
                                rc = VERR_INVALID_PARAMETER;
                                break;
                            }

                            if (size > 0)
                            {
                                if (pPageListInfo->flags & VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST)
                                {
                                    /* Copy pHostParm->u.pointer.addr[pHostParm->u.pointer.size] to pages. */
                                    rc = vmmdevHGCMPageListWrite(pThis->pDevIns, pPageListInfo, pHostParm->u.pointer.addr, size);
                                }
                                else
                                    rc = VINF_SUCCESS;
                            }

                            Log(("vmmdevHGCMCall: PageList guest parameter rc = %Rrc\n", rc));
                        } break;

                        default:
                        {
                            /* This indicates that the guest request memory was corrupted. */
                            rc = VERR_INVALID_PARAMETER;
                            break;
                        }
                    }
                }
# ifdef VBOX_WITH_DTRACE
                idFunction = pHGCMCall->u32Function;
                idClient   = pHGCMCall->u32ClientID;
# endif
                break;
            }
#else
            case VMMDevReq_HGCMCall:
            {
                VMMDevHGCMCall *pHGCMCall = (VMMDevHGCMCall *)pHeader;

                uint32_t cParms = pHGCMCall->cParms;
                if (cParms != pCmd->cHostParms)
                    rc = VERR_INVALID_PARAMETER;

                VBOXHGCMSVCPARM *pHostParm = pCmd->paHostParms;

                uint32_t i;
                uint32_t iLinPtr = 0;

                HGCMFunctionParameter *pGuestParm = VMMDEV_HGCM_CALL_PARMS(pHGCMCall);

                for (i = 0; i < cParms && RT_SUCCESS(rc); i++, pGuestParm++, pHostParm++)
                {
                    rc = vmmdevHGCMParmVerify32(pGuestParm, pHostParm);
                    if (RT_FAILURE(rc))
                        break;

                    switch (pGuestParm->type)
                    {
                        case VMMDevHGCMParmType_32bit:
                        {
                            pGuestParm->u.value32 = pHostParm->u.uint32;
                        } break;

                        case VMMDevHGCMParmType_64bit:
                        {
                            pGuestParm->u.value64 = pHostParm->u.uint64;
                        } break;

                        case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
                        case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
                        case VMMDevHGCMParmType_LinAddr:     /* In & Out */
                        {
                            /* Copy buffer back to guest memory. */
                            uint32_t size = pGuestParm->u.Pointer.size;

                            if (size > 0)
                            {
                                if (pGuestParm->type != VMMDevHGCMParmType_LinAddr_In)
                                {
                                    /* Use the saved page list to write data back to the guest RAM. */
                                    rc = vmmdevHGCMWriteLinPtr (pThis->pDevIns, i, pHostParm->u.pointer.addr, size, iLinPtr, pCmd->paLinPtrs);
                                }

                                /* All linptrs with size > 0 were saved. Advance the index to the next linptr. */
                                iLinPtr++;
                            }
                        } break;

                        case VMMDevHGCMParmType_PageList:
                        {
                            uint32_t cbHGCMCall = pCmd->cbSize; /* Size of the request. */

                            uint32_t size = pGuestParm->u.PageList.size;

                            /* Check that the page list info is within the request. */
                            if (   cbHGCMCall < sizeof (HGCMPageListInfo)
                                || pGuestParm->u.PageList.offset > cbHGCMCall - sizeof (HGCMPageListInfo))
                            {
                                rc = VERR_INVALID_PARAMETER;
                                break;
                            }

                            /* At least the structure is within. */
                            HGCMPageListInfo *pPageListInfo = (HGCMPageListInfo *)((uint8_t *)pHGCMCall + pGuestParm->u.PageList.offset);

                            uint32_t cbPageListInfo = sizeof (HGCMPageListInfo) + (pPageListInfo->cPages - 1) * sizeof (pPageListInfo->aPages[0]);

                            if (   pPageListInfo->cPages == 0
                                || cbHGCMCall < pGuestParm->u.PageList.offset + cbPageListInfo)
                            {
                                rc = VERR_INVALID_PARAMETER;
                                break;
                            }

                            if (size > 0)
                            {
                                if (pPageListInfo->flags & VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST)
                                {
                                    /* Copy pHostParm->u.pointer.addr[pHostParm->u.pointer.size] to pages. */
                                    rc = vmmdevHGCMPageListWrite(pThis->pDevIns, pPageListInfo, pHostParm->u.pointer.addr, size);
                                }
                                else
                                    rc = VINF_SUCCESS;
                            }

                            Log(("vmmdevHGCMCall: PageList guest parameter rc = %Rrc\n", rc));
                        } break;

                        default:
                        {
                            /* This indicates that the guest request memory was corrupted. */
                            rc = VERR_INVALID_PARAMETER;
                            break;
                        }
                    }
                }
# ifdef VBOX_WITH_DTRACE
                idFunction = pHGCMCall->u32Function;
                idClient   = pHGCMCall->u32ClientID;
# endif
                break;
            }
#endif /* VBOX_WITH_64_BITS_GUESTS */
            case VMMDevReq_HGCMConnect:
            {
                VMMDevHGCMConnect *pHGCMConnectCopy = (VMMDevHGCMConnect *)(pCmd+1);

                /* save the client id in the guest request packet */
                VMMDevHGCMConnect *pHGCMConnect = (VMMDevHGCMConnect *)pHeader;
                pHGCMConnect->u32ClientID = pHGCMConnectCopy->u32ClientID;
                break;
            }

            default:
                /* make gcc happy */
                break;
            }
        }

        if (RT_FAILURE(rc))
        {
            /* Command is wrong. Return HGCM error result to the guest. */
            pHeader->result = rc;
        }

        /* Mark request as processed. */
        pHeader->fu32Flags |= VBOX_HGCM_REQ_DONE;

        /* Write back the request */
        PDMDevHlpPhysWrite(pThis->pDevIns, pCmd->GCPhys, pHeader, pCmd->cbSize);

        /* Now, when the command was removed from the internal list, notify the guest. */
        VMMDevNotifyGuest (pThis, VMMDEV_EVENT_HGCM);

        if ((uint8_t *)pHeader != &au8Prealloc[0])
        {
            /* Only if it was allocated from heap. */
            RTMemFree (pHeader);
        }
    }

    /* Deallocate the command memory. */
    if (pCmd->paLinPtrs)
    {
        RTMemFree (pCmd->paLinPtrs);
    }

    RTMemFree (pCmd);

    VBOXDD_HGCMCALL_COMPLETED_DONE(pCmd, idFunction, idClient, result);
    return;
}

DECLCALLBACK(void) hgcmCompleted (PPDMIHGCMPORT pInterface, int32_t result, PVBOXHGCMCMD pCmd)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDevState, IHGCMPort);

    VBOXDD_HGCMCALL_COMPLETED_REQ(pCmd, result);

/** @todo no longer necessary to forward to EMT, but it might be more
 *        efficient...? */
    /* Not safe to execute asynchronously; forward to EMT */
    int rc = VMR3ReqCallVoidNoWait(PDMDevHlpGetVM(pThis->pDevIns), VMCPUID_ANY,
                                   (PFNRT)hgcmCompletedWorker, 3, pInterface, result, pCmd);
    AssertRC(rc);
}

/** @thread EMT */
int vmmdevHGCMSaveState(PVMMDEV pThis, PSSMHANDLE pSSM)
{
    /* Save information about pending requests.
     * Only GCPtrs are of interest.
     */
    int rc = VINF_SUCCESS;

    LogFlowFunc(("\n"));

    /* Compute how many commands are pending. */
    uint32_t cCmds = 0;

    PVBOXHGCMCMD pIter = pThis->pHGCMCmdList;

    while (pIter)
    {
        LogFlowFunc (("pIter %p\n", pIter));
        cCmds++;
        pIter = pIter->pNext;
    }

    LogFlowFunc(("cCmds = %d\n", cCmds));

    /* Save number of commands. */
    rc = SSMR3PutU32(pSSM, cCmds);
    AssertRCReturn(rc, rc);

    if (cCmds > 0)
    {
        pIter = pThis->pHGCMCmdList;

        while (pIter)
        {
            PVBOXHGCMCMD pNext = pIter->pNext;

            LogFlowFunc (("Saving %RGp, size %d\n", pIter->GCPhys, pIter->cbSize));

            /* GC physical address of the guest request. */
            rc = SSMR3PutGCPhys(pSSM, pIter->GCPhys);
            AssertRCReturn(rc, rc);

            /* Request packet size */
            rc = SSMR3PutU32(pSSM, pIter->cbSize);
            AssertRCReturn(rc, rc);

            /*
             * Version 9+: save complete information about commands.
             */

            /* The type of the command. */
            rc = SSMR3PutU32(pSSM, (uint32_t)pIter->enmCmdType);
            AssertRCReturn(rc, rc);

            /* Whether the command was cancelled by the guest. */
            rc = SSMR3PutBool(pSSM, pIter->fCancelled);
            AssertRCReturn(rc, rc);

            /* Linear pointer parameters information. How many pointers. Always 0 if not a call command. */
            rc = SSMR3PutU32(pSSM, (uint32_t)pIter->cLinPtrs);
            AssertRCReturn(rc, rc);

            if (pIter->cLinPtrs > 0)
            {
                /* How many pages for all linptrs in this command. */
                rc = SSMR3PutU32(pSSM, (uint32_t)pIter->cLinPtrPages);
                AssertRCReturn(rc, rc);
            }

            int i;
            for (i = 0; i < pIter->cLinPtrs; i++)
            {
                /* Pointer to descriptions of linear pointers.  */
                VBOXHGCMLINPTR *pLinPtr = &pIter->paLinPtrs[i];

                /* Index of the parameter. */
                rc = SSMR3PutU32(pSSM, (uint32_t)pLinPtr->iParm);
                AssertRCReturn(rc, rc);

                /* Offset in the first physical page of the region. */
                rc = SSMR3PutU32(pSSM, pLinPtr->offFirstPage);
                AssertRCReturn(rc, rc);

                /* How many pages. */
                rc = SSMR3PutU32(pSSM, pLinPtr->cPages);
                AssertRCReturn(rc, rc);

                uint32_t iPage;
                for (iPage = 0; iPage < pLinPtr->cPages; iPage++)
                {
                    /* Array of the GC physical addresses for these pages.
                     * It is assumed that the physical address of the locked resident
                     * guest page does not change.
                     */
                    rc = SSMR3PutGCPhys(pSSM, pLinPtr->paPages[iPage]);
                    AssertRCReturn(rc, rc);
                }
            }

            /* A reserved field, will allow to extend saved data for a command. */
            rc = SSMR3PutU32(pSSM, 0);
            AssertRCReturn(rc, rc);

            pIter = pNext;
        }
    }

    /* A reserved field, will allow to extend saved data for VMMDevHGCM. */
    rc = SSMR3PutU32(pSSM, 0);
    AssertRCReturn(rc, rc);

    return rc;
}

/** @thread EMT(0) */
int vmmdevHGCMLoadState(PVMMDEV pThis, PSSMHANDLE pSSM, uint32_t uVersion)
{
    int rc = VINF_SUCCESS;

    LogFlowFunc(("\n"));

    /* Read how many commands were pending. */
    uint32_t cCmds = 0;
    rc = SSMR3GetU32(pSSM, &cCmds);
    AssertRCReturn(rc, rc);

    LogFlowFunc(("cCmds = %d\n", cCmds));

    if (uVersion < 9)
    {
        /* Only the guest physical address is saved. */
        while (cCmds--)
        {
            RTGCPHYS GCPhys;
            uint32_t cbSize;

            rc = SSMR3GetGCPhys(pSSM, &GCPhys);
            AssertRCReturn(rc, rc);

            rc = SSMR3GetU32(pSSM, &cbSize);
            AssertRCReturn(rc, rc);

            LogFlowFunc (("Restoring %RGp size %x bytes\n", GCPhys, cbSize));

            PVBOXHGCMCMD pCmd = (PVBOXHGCMCMD)RTMemAllocZ (sizeof (struct VBOXHGCMCMD));
            AssertReturn(pCmd, VERR_NO_MEMORY);

            pCmd->enmCmdType = VBOXHGCMCMDTYPE_LOADSTATE; /* This marks the "old" saved command. */

            vmmdevHGCMAddCommand (pThis, pCmd, GCPhys, cbSize, VBOXHGCMCMDTYPE_LOADSTATE);
        }
    }
    else
    {
        /*
         * Version 9+: Load complete information about commands.
         */
        uint32_t u32;
        bool f;

        while (cCmds--)
        {
            RTGCPHYS GCPhys;
            uint32_t cbSize;

            /* GC physical address of the guest request. */
            rc = SSMR3GetGCPhys(pSSM, &GCPhys);
            AssertRCReturn(rc, rc);

            /* The request packet size */
            rc = SSMR3GetU32(pSSM, &cbSize);
            AssertRCReturn(rc, rc);

            LogFlowFunc (("Restoring %RGp size %x bytes\n", GCPhys, cbSize));

            /* For uVersion <= 12, this was the size of entire command.
             * Now the size is recalculated in vmmdevHGCMLoadStateDone.
             */
            if (uVersion <= 12)
            {
                rc = SSMR3Skip(pSSM, sizeof (uint32_t));
                AssertRCReturn(rc, rc);
            }

            /* Allocate only VBOXHGCMCMD structure. vmmdevHGCMLoadStateDone will reallocate the command
             * with additional space for parameters and for pointer/pagelists buffer.
             */
            PVBOXHGCMCMD pCmd = (PVBOXHGCMCMD)RTMemAllocZ (sizeof (VBOXHGCMCMD));
            AssertReturn(pCmd, VERR_NO_MEMORY);

            /* The type of the command. */
            rc = SSMR3GetU32(pSSM, &u32);
            AssertRCReturn(rc, rc);
            pCmd->enmCmdType = (VBOXHGCMCMDTYPE)u32;

            /* Whether the command was cancelled by the guest. */
            rc = SSMR3GetBool(pSSM, &f);
            AssertRCReturn(rc, rc);
            pCmd->fCancelled = f;

            /* Linear pointer parameters information. How many pointers. Always 0 if not a call command. */
            rc = SSMR3GetU32(pSSM, &u32);
            AssertRCReturn(rc, rc);
            pCmd->cLinPtrs = u32;

            if (pCmd->cLinPtrs > 0)
            {
                /* How many pages for all linptrs in this command. */
                rc = SSMR3GetU32(pSSM, &u32);
                AssertRCReturn(rc, rc);
                pCmd->cLinPtrPages = u32;

                pCmd->paLinPtrs = (VBOXHGCMLINPTR *)RTMemAllocZ (  sizeof (VBOXHGCMLINPTR) * pCmd->cLinPtrs
                                                                 + sizeof (RTGCPHYS) * pCmd->cLinPtrPages);
                AssertReturn(pCmd->paLinPtrs, VERR_NO_MEMORY);

                RTGCPHYS *pPages = (RTGCPHYS *)((uint8_t *)pCmd->paLinPtrs + sizeof (VBOXHGCMLINPTR) * pCmd->cLinPtrs);
                int cPages = 0;

                int i;
                for (i = 0; i < pCmd->cLinPtrs; i++)
                {
                    /* Pointer to descriptions of linear pointers. */
                    VBOXHGCMLINPTR *pLinPtr = &pCmd->paLinPtrs[i];

                    pLinPtr->paPages = pPages;

                    /* Index of the parameter. */
                    rc = SSMR3GetU32(pSSM, &u32);
                    AssertRCReturn(rc, rc);
                    pLinPtr->iParm = u32;

                    /* Offset in the first physical page of the region. */
                    rc = SSMR3GetU32(pSSM, &u32);
                    AssertRCReturn(rc, rc);
                    pLinPtr->offFirstPage = u32;

                    /* How many pages. */
                    rc = SSMR3GetU32(pSSM, &u32);
                    AssertRCReturn(rc, rc);
                    pLinPtr->cPages = u32;

                    uint32_t iPage;
                    for (iPage = 0; iPage < pLinPtr->cPages; iPage++)
                    {
                        /* Array of the GC physical addresses for these pages.
                         * It is assumed that the physical address of the locked resident
                         * guest page does not change.
                         */
                        RTGCPHYS GCPhysPage;
                        rc = SSMR3GetGCPhys(pSSM, &GCPhysPage);
                        AssertRCReturn(rc, rc);

                        /* Verify that the number of loaded pages is valid. */
                        cPages++;
                        if (cPages > pCmd->cLinPtrPages)
                        {
                            LogRel(("VMMDevHGCM load state failure: cPages %d, expected %d, ptr %d/%d\n",
                                    cPages, pCmd->cLinPtrPages, i, pCmd->cLinPtrs));
                            return VERR_SSM_UNEXPECTED_DATA;
                        }

                        *pPages++ = GCPhysPage;
                    }
                }
            }

            /* A reserved field, will allow to extend saved data for a command. */
            rc = SSMR3GetU32(pSSM, &u32);
            AssertRCReturn(rc, rc);

            vmmdevHGCMAddCommand (pThis, pCmd, GCPhys, cbSize, VBOXHGCMCMDTYPE_LOADSTATE);
        }

        /* A reserved field, will allow to extend saved data for VMMDevHGCM. */
        rc = SSMR3GetU32(pSSM, &u32);
        AssertRCReturn(rc, rc);
    }

    return rc;
}

/** @thread EMT */
int vmmdevHGCMLoadStateDone(PVMMDEV pThis)
{
    LogFlowFunc(("\n"));

    /* Reissue pending requests. */
    PPDMDEVINS pDevIns = pThis->pDevIns;

    int rc = vmmdevHGCMCmdListLock (pThis);

    if (RT_SUCCESS (rc))
    {
        /* Start from the current list head and commands loaded from saved state.
         * New commands will be inserted at the list head, so they will not be seen by
         * this loop.
         *
         * Note: The list contains only VBOXHGCMCMD structures, place for HGCM parameters
         *       and for data buffers has not been allocated.
         *       Command handlers compute the command size and reallocate it before
         *       resubmitting the command to HGCM services.
         *       New commands will be inserted to the list.
         */
        PVBOXHGCMCMD pIter = pThis->pHGCMCmdList;

        pThis->pHGCMCmdList = NULL; /* Reset the list. Saved commands will be processed and deallocated. */

        while (pIter)
        {
            /* This will remove the command from the list if resubmitting fails. */
            bool fHGCMCalled = false;

            LogFlowFunc (("pIter %p\n", pIter));

            PVBOXHGCMCMD pNext = pIter->pNext;

            PVBOXHGCMCMD pCmd = NULL; /* Resubmitted command. */

            VMMDevHGCMRequestHeader *requestHeader = (VMMDevHGCMRequestHeader *)RTMemAllocZ (pIter->cbSize);
            Assert(requestHeader);
            if (requestHeader == NULL)
                return VERR_NO_MEMORY;

            PDMDevHlpPhysRead(pDevIns, pIter->GCPhys, requestHeader, pIter->cbSize);

            /* the structure size must be greater or equal to the header size */
            if (requestHeader->header.size < sizeof(VMMDevHGCMRequestHeader))
            {
                Log(("VMMDev request header size too small! size = %d\n", requestHeader->header.size));
            }
            else
            {
                /* check the version of the header structure */
                if (requestHeader->header.version != VMMDEV_REQUEST_HEADER_VERSION)
                {
                    Log(("VMMDev: guest header version (0x%08X) differs from ours (0x%08X)\n", requestHeader->header.version, VMMDEV_REQUEST_HEADER_VERSION));
                }
                else
                {
                    Log(("VMMDev request issued: %d, command type %d\n", requestHeader->header.requestType, pIter->enmCmdType));

                    /* Use the saved command type. Even if the guest has changed the memory already,
                     * HGCM should see the same command as it was before saving state.
                     */
                    switch (pIter->enmCmdType)
                    {
                        case VBOXHGCMCMDTYPE_CONNECT:
                        {
                            if (requestHeader->header.size < sizeof(VMMDevHGCMConnect))
                            {
                                AssertMsgFailed(("VMMDevReq_HGCMConnect structure has invalid size!\n"));
                                requestHeader->header.rc = VERR_INVALID_PARAMETER;
                            }
                            else if (!pThis->pHGCMDrv)
                            {
                                Log(("VMMDevReq_HGCMConnect HGCM Connector is NULL!\n"));
                                requestHeader->header.rc = VERR_NOT_SUPPORTED;
                            }
                            else
                            {
                                VMMDevHGCMConnect *pHGCMConnect = (VMMDevHGCMConnect *)requestHeader;

                                Log(("VMMDevReq_HGCMConnect\n"));

                                requestHeader->header.rc = vmmdevHGCMConnectSaved (pThis, pHGCMConnect, pIter->GCPhys, &fHGCMCalled, pIter, &pCmd);
                            }
                            break;
                        }

                        case VBOXHGCMCMDTYPE_DISCONNECT:
                        {
                            if (requestHeader->header.size < sizeof(VMMDevHGCMDisconnect))
                            {
                                AssertMsgFailed(("VMMDevReq_HGCMDisconnect structure has invalid size!\n"));
                                requestHeader->header.rc = VERR_INVALID_PARAMETER;
                            }
                            else if (!pThis->pHGCMDrv)
                            {
                                Log(("VMMDevReq_HGCMDisconnect HGCM Connector is NULL!\n"));
                                requestHeader->header.rc = VERR_NOT_SUPPORTED;
                            }
                            else
                            {
                                VMMDevHGCMDisconnect *pHGCMDisconnect = (VMMDevHGCMDisconnect *)requestHeader;

                                Log(("VMMDevReq_VMMDevHGCMDisconnect\n"));
                                requestHeader->header.rc = vmmdevHGCMDisconnectSaved (pThis, pHGCMDisconnect, pIter->GCPhys, &fHGCMCalled, pIter, &pCmd);
                            }
                            break;
                        }

                        case VBOXHGCMCMDTYPE_CALL:
                        {
                            if (requestHeader->header.size < sizeof(VMMDevHGCMCall))
                            {
                                AssertMsgFailed(("VMMDevReq_HGCMCall structure has invalid size!\n"));
                                requestHeader->header.rc = VERR_INVALID_PARAMETER;
                            }
                            else if (!pThis->pHGCMDrv)
                            {
                                Log(("VMMDevReq_HGCMCall HGCM Connector is NULL!\n"));
                                requestHeader->header.rc = VERR_NOT_SUPPORTED;
                            }
                            else
                            {
                                VMMDevHGCMCall *pHGCMCall = (VMMDevHGCMCall *)requestHeader;

                                Log(("VMMDevReq_HGCMCall: sizeof (VMMDevHGCMRequest) = %04X\n", sizeof (VMMDevHGCMCall)));

                                Log(("%.*Rhxd\n", requestHeader->header.size, requestHeader));

#ifdef VBOX_WITH_64_BITS_GUESTS
                                bool f64Bits = (requestHeader->header.requestType == VMMDevReq_HGCMCall64);
#else
                                bool f64Bits = false;
#endif /* VBOX_WITH_64_BITS_GUESTS */
                                requestHeader->header.rc = vmmdevHGCMCallSaved (pThis, pHGCMCall, pIter->GCPhys, requestHeader->header.size, f64Bits, &fHGCMCalled, pIter, &pCmd);
                            }
                            break;
                        }
                        case VBOXHGCMCMDTYPE_LOADSTATE:
                        {
                            /* Old saved state. */
                            switch (requestHeader->header.requestType)
                            {
                                case VMMDevReq_HGCMConnect:
                                {
                                    if (requestHeader->header.size < sizeof(VMMDevHGCMConnect))
                                    {
                                        AssertMsgFailed(("VMMDevReq_HGCMConnect structure has invalid size!\n"));
                                        requestHeader->header.rc = VERR_INVALID_PARAMETER;
                                    }
                                    else if (!pThis->pHGCMDrv)
                                    {
                                        Log(("VMMDevReq_HGCMConnect HGCM Connector is NULL!\n"));
                                        requestHeader->header.rc = VERR_NOT_SUPPORTED;
                                    }
                                    else
                                    {
                                        VMMDevHGCMConnect *pHGCMConnect = (VMMDevHGCMConnect *)requestHeader;

                                        Log(("VMMDevReq_HGCMConnect\n"));

                                        requestHeader->header.rc = vmmdevHGCMConnect (pThis, pHGCMConnect, pIter->GCPhys);
                                    }
                                    break;
                                }

                                case VMMDevReq_HGCMDisconnect:
                                {
                                    if (requestHeader->header.size < sizeof(VMMDevHGCMDisconnect))
                                    {
                                        AssertMsgFailed(("VMMDevReq_HGCMDisconnect structure has invalid size!\n"));
                                        requestHeader->header.rc = VERR_INVALID_PARAMETER;
                                    }
                                    else if (!pThis->pHGCMDrv)
                                    {
                                        Log(("VMMDevReq_HGCMDisconnect HGCM Connector is NULL!\n"));
                                        requestHeader->header.rc = VERR_NOT_SUPPORTED;
                                    }
                                    else
                                    {
                                        VMMDevHGCMDisconnect *pHGCMDisconnect = (VMMDevHGCMDisconnect *)requestHeader;

                                        Log(("VMMDevReq_VMMDevHGCMDisconnect\n"));
                                        requestHeader->header.rc = vmmdevHGCMDisconnect (pThis, pHGCMDisconnect, pIter->GCPhys);
                                    }
                                    break;
                                }

#ifdef VBOX_WITH_64_BITS_GUESTS
                                case VMMDevReq_HGCMCall64:
                                case VMMDevReq_HGCMCall32:
#else
                                case VMMDevReq_HGCMCall:
#endif /* VBOX_WITH_64_BITS_GUESTS */
                                {
                                    if (requestHeader->header.size < sizeof(VMMDevHGCMCall))
                                    {
                                        AssertMsgFailed(("VMMDevReq_HGCMCall structure has invalid size!\n"));
                                        requestHeader->header.rc = VERR_INVALID_PARAMETER;
                                    }
                                    else if (!pThis->pHGCMDrv)
                                    {
                                        Log(("VMMDevReq_HGCMCall HGCM Connector is NULL!\n"));
                                        requestHeader->header.rc = VERR_NOT_SUPPORTED;
                                    }
                                    else
                                    {
                                        VMMDevHGCMCall *pHGCMCall = (VMMDevHGCMCall *)requestHeader;

                                        Log(("VMMDevReq_HGCMCall: sizeof (VMMDevHGCMRequest) = %04X\n", sizeof (VMMDevHGCMCall)));

                                        Log(("%.*Rhxd\n", requestHeader->header.size, requestHeader));

#ifdef VBOX_WITH_64_BITS_GUESTS
                                        bool f64Bits = (requestHeader->header.requestType == VMMDevReq_HGCMCall64);
#else
                                        bool f64Bits = false;
#endif /* VBOX_WITH_64_BITS_GUESTS */
                                        requestHeader->header.rc = vmmdevHGCMCall (pThis, pHGCMCall, requestHeader->header.size, pIter->GCPhys, f64Bits);
                                    }
                                    break;
                                }
                                default:
                                    AssertMsgFailed(("Unknown request type %x during LoadState\n", requestHeader->header.requestType));
                                    LogRel(("VMMDEV: Ignoring unknown request type %x during LoadState\n", requestHeader->header.requestType));
                            }
                        } break;

                        default:
                            AssertMsgFailed(("Unknown request type %x during LoadState\n", requestHeader->header.requestType));
                            LogRel(("VMMDEV: Ignoring unknown request type %x during LoadState\n", requestHeader->header.requestType));
                    }
                }
            }

            if (pIter->enmCmdType == VBOXHGCMCMDTYPE_LOADSTATE)
            {
                /* Old saved state. */

                /* Write back the request */
                PDMDevHlpPhysWrite(pDevIns, pIter->GCPhys, requestHeader, pIter->cbSize);
                RTMemFree(requestHeader);
                requestHeader = NULL;
            }
            else
            {
                if (!fHGCMCalled)
                {
                   /* HGCM was not called. Return the error to the guest. Guest may try to repeat the call. */
                   requestHeader->header.rc = VERR_TRY_AGAIN;
                   requestHeader->fu32Flags |= VBOX_HGCM_REQ_DONE;
                }

                /* Write back the request */
                PDMDevHlpPhysWrite(pDevIns, pIter->GCPhys, requestHeader, pIter->cbSize);
                RTMemFree(requestHeader);
                requestHeader = NULL;

                if (!fHGCMCalled)
                {
                   /* HGCM was not called. Deallocate the current command and then notify guest. */
                   if (pCmd)
                   {
                       vmmdevHGCMRemoveCommand (pThis, pCmd);

                       if (pCmd->paLinPtrs != NULL)
                       {
                           RTMemFree(pCmd->paLinPtrs);
                       }

                       RTMemFree(pCmd);
                       pCmd = NULL;
                   }

                   VMMDevNotifyGuest (pThis, VMMDEV_EVENT_HGCM);
                }
            }

            /* Deallocate the saved command structure. */
            if (pIter->paLinPtrs != NULL)
            {
                RTMemFree(pIter->paLinPtrs);
            }

            RTMemFree(pIter);

            pIter = pNext;
        }

        vmmdevHGCMCmdListUnlock (pThis);
    }

    return rc;
}

void vmmdevHGCMDestroy(PVMMDEV pThis)
{
    LogFlowFunc(("\n"));

    PVBOXHGCMCMD pIter = pThis->pHGCMCmdList;

    while (pIter)
    {
        PVBOXHGCMCMD pNext = pIter->pNext;

        vmmdevHGCMRemoveCommand(pThis, pIter);

        /* Deallocate the command memory. */
        RTMemFree(pIter->paLinPtrs);
        RTMemFree(pIter);

        pIter = pNext;
    }

    return;
}
