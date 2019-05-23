/* $Id: VMMDevHGCM.cpp $ */
/** @file
 * VMMDev - HGCM - Host-Guest Communication Manager Device.
 */

/*
 * Copyright (C) 2006-2018 Oracle Corporation
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

#include <VBox/AssertGuest.h>
#include <VBox/err.h>
#include <VBox/hgcmsvc.h>
#include <VBox/log.h>

#include "VMMDevHGCM.h"

#ifdef DEBUG
# define VBOX_STRICT_GUEST
#endif

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
 * Information about a 32 or 64 bit parameter.
 */
typedef struct VBOXHGCMPARMVAL
{
    /** Actual value. Both 32 and 64 bit is saved here. */
    uint64_t u64Value;

    /** Offset from the start of the request where the value is stored. */
    uint32_t offValue;

    /** Size of the value: 4 for 32 bit and 8 for 64 bit. */
    uint32_t cbValue;

} VBOXHGCMPARMVAL;

/**
 * Information about a pointer parameter.
 */
typedef struct VBOXHGCMPARMPTR
{
    /** Size of the buffer described by the pointer parameter. */
    uint32_t cbData;

    /** Offset in the first physical page of the region. */
    uint32_t offFirstPage;

    /** How many pages. */
    uint32_t cPages;

    /** How the buffer should be copied VBOX_HGCM_F_PARM_*. */
    uint32_t fu32Direction;

    /** Pointer to array of the GC physical addresses for these pages.
     * It is assumed that the physical address of the locked resident guest page
     * does not change.
     */
    RTGCPHYS *paPages;

} VBOXHGCMPARMPTR;

/**
 * Information about a guest HGCM parameter.
 */
typedef struct VBOXHGCMGUESTPARM
{
    /** The parameter type. */
    HGCMFunctionParameterType enmType;

    union
    {
        VBOXHGCMPARMVAL       val;
        VBOXHGCMPARMPTR       ptr;
    } u;

} VBOXHGCMGUESTPARM;

typedef struct VBOXHGCMCMD
{
    /** Active commands, list is protected by critsectHGCMCmdList. */
    RTLISTNODE          node;

    /** The type of the command. */
    VBOXHGCMCMDTYPE     enmCmdType;

    /** Whether the command was cancelled by the guest. */
    bool                fCancelled;

    /** GC physical address of the guest request. */
    RTGCPHYS            GCPhys;

    /** Request packet size. */
    uint32_t            cbRequest;

    /** The type of the guest request. */
    VMMDevRequestType   enmRequestType;

    union {
        struct
        {
            uint32_t            u32ClientID;
            HGCMServiceLocation loc;
        } connect;

        struct
        {
            uint32_t            u32ClientID;
        } disconnect;

        struct
        {
            /* Number of elements in paGuestParms and paHostParms arrays. */
            uint32_t            cParms;

            uint32_t            u32ClientID;

            uint32_t            u32Function;

            /** Pointer to information about guest parameters in case of a Call request.
             * Follows this structure in the same memory block.
             */
            VBOXHGCMGUESTPARM  *paGuestParms;

            /** Pointer to converted host parameters in case of a Call request.
             * Follows this structure in the same memory block.
             */
            VBOXHGCMSVCPARM    *paHostParms;

            /* VBOXHGCMGUESTPARM[] */
            /* VBOXHGCMSVCPARM[] */
        } call;
    } u;
} VBOXHGCMCMD;


static int vmmdevHGCMCmdListLock(PVMMDEV pThis)
{
    int rc = RTCritSectEnter(&pThis->critsectHGCMCmdList);
    AssertRC(rc);
    return rc;
}

static void vmmdevHGCMCmdListUnlock(PVMMDEV pThis)
{
    int rc = RTCritSectLeave(&pThis->critsectHGCMCmdList);
    AssertRC(rc);
}

/** Allocate and initialize VBOXHGCMCMD structure for HGCM request.
 *
 * @returns Pointer to the command on success, NULL otherwise.
 * @param   enmCmdType      Type of the command.
 * @param   GCPhys          The guest physical address of the HGCM request.
 * @param   cbRequest       The size of the HGCM request.
 * @param   cParms          Number of HGCM parameters for VBOXHGCMCMDTYPE_CALL command.
 */
static PVBOXHGCMCMD vmmdevHGCMCmdAlloc(VBOXHGCMCMDTYPE enmCmdType, RTGCPHYS GCPhys, uint32_t cbRequest, uint32_t cParms)
{
    /* Size of required memory buffer. */
    const uint32_t cbCmd =   sizeof(struct VBOXHGCMCMD)
                           + cParms * sizeof(VBOXHGCMGUESTPARM)
                           + cParms * sizeof(VBOXHGCMSVCPARM);

    PVBOXHGCMCMD pCmd = (PVBOXHGCMCMD)RTMemAllocZ(cbCmd);
    if (pCmd)
    {
        pCmd->enmCmdType = enmCmdType;
        pCmd->GCPhys     = GCPhys;
        pCmd->cbRequest  = cbRequest;

        if (enmCmdType == VBOXHGCMCMDTYPE_CALL)
        {
            pCmd->u.call.cParms = cParms;
            if (cParms)
            {
                pCmd->u.call.paGuestParms = (VBOXHGCMGUESTPARM *)((uint8_t *)pCmd
                                                                  + sizeof(struct VBOXHGCMCMD));
                pCmd->u.call.paHostParms = (VBOXHGCMSVCPARM *)((uint8_t *)pCmd->u.call.paGuestParms
                                                               + cParms * sizeof(VBOXHGCMGUESTPARM));
            }
        }
    }
    return pCmd;
}

/** Deallocate VBOXHGCMCMD memory.
 *
 * @param   pCmd            Command to deallocate.
 */
static void vmmdevHGCMCmdFree(PVBOXHGCMCMD pCmd)
{
    if (pCmd)
    {
        if (pCmd->enmCmdType == VBOXHGCMCMDTYPE_CALL)
        {
            uint32_t i;
            for (i = 0; i < pCmd->u.call.cParms; ++i)
            {
                VBOXHGCMSVCPARM   * const pHostParm  = &pCmd->u.call.paHostParms[i];
                VBOXHGCMGUESTPARM * const pGuestParm = &pCmd->u.call.paGuestParms[i];

                if (pHostParm->type == VBOX_HGCM_SVC_PARM_PTR)
                    RTMemFree(pHostParm->u.pointer.addr);

                if (   pGuestParm->enmType == VMMDevHGCMParmType_LinAddr_In
                    || pGuestParm->enmType == VMMDevHGCMParmType_LinAddr_Out
                    || pGuestParm->enmType == VMMDevHGCMParmType_LinAddr
                    || pGuestParm->enmType == VMMDevHGCMParmType_PageList)
                    RTMemFree(pGuestParm->u.ptr.paPages);
            }
        }

        RTMemFree(pCmd);
    }
}

/** Add VBOXHGCMCMD to the list of pending commands.
 *
 * @returns VBox status code.
 * @param   pThis           The VMMDev instance data.
 * @param   pCmd            Command to add.
 */
static int vmmdevHGCMAddCommand(PVMMDEV pThis, PVBOXHGCMCMD pCmd)
{
    int rc = vmmdevHGCMCmdListLock(pThis);
    AssertRCReturn(rc, rc);

    LogFlowFunc(("%p type %d\n", pCmd, pCmd->enmCmdType));

    RTListPrepend(&pThis->listHGCMCmd, &pCmd->node);

    /* Automatically enable HGCM events, if there are HGCM commands. */
    if (   pCmd->enmCmdType == VBOXHGCMCMDTYPE_CONNECT
        || pCmd->enmCmdType == VBOXHGCMCMDTYPE_DISCONNECT
        || pCmd->enmCmdType == VBOXHGCMCMDTYPE_CALL)
    {
        LogFunc(("u32HGCMEnabled = %d\n", pThis->u32HGCMEnabled));
        if (ASMAtomicCmpXchgU32(&pThis->u32HGCMEnabled, 1, 0))
             VMMDevCtlSetGuestFilterMask(pThis, VMMDEV_EVENT_HGCM, 0);
    }

    vmmdevHGCMCmdListUnlock(pThis);
    return rc;
}

/** Remove VBOXHGCMCMD from the list of pending commands.
 *
 * @returns VBox status code.
 * @param   pThis           The VMMDev instance data.
 * @param   pCmd            Command to remove.
 */
static int vmmdevHGCMRemoveCommand(PVMMDEV pThis, PVBOXHGCMCMD pCmd)
{
    int rc = vmmdevHGCMCmdListLock(pThis);
    AssertRCReturn(rc, rc);

    LogFlowFunc(("%p\n", pCmd));

    RTListNodeRemove(&pCmd->node);

    vmmdevHGCMCmdListUnlock(pThis);
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
 * @param   GCPhys          The physical address of the command we're looking for.
 */
DECLINLINE(PVBOXHGCMCMD) vmmdevHGCMFindCommandLocked(PVMMDEV pThis, RTGCPHYS GCPhys)
{
    PVBOXHGCMCMD pCmd;
    RTListForEach(&pThis->listHGCMCmd, pCmd, VBOXHGCMCMD, node)
    {
        if (pCmd->GCPhys == GCPhys)
            return pCmd;
    }
    return NULL;
}

/** Copy VMMDevHGCMConnect request data from the guest to VBOXHGCMCMD command.
 *
 * @param   pHGCMConnect    The source guest request (cached in host memory).
 * @param   pCmd            Destination command.
 */
static void vmmdevHGCMConnectFetch(const VMMDevHGCMConnect *pHGCMConnect, PVBOXHGCMCMD pCmd)
{
    pCmd->enmRequestType        = pHGCMConnect->header.header.requestType;
    pCmd->u.connect.u32ClientID = pHGCMConnect->u32ClientID;
    pCmd->u.connect.loc         = pHGCMConnect->loc;
}

/** Handle VMMDevHGCMConnect request.
 *
 * @param   pThis           The VMMDev instance data.
 * @param   pHGCMConnect    The guest request (cached in host memory).
 * @param   GCPhys          The physical address of the request.
 */
int vmmdevHGCMConnect(PVMMDEV pThis, const VMMDevHGCMConnect *pHGCMConnect, RTGCPHYS GCPhys)
{
    int rc = VINF_SUCCESS;

    PVBOXHGCMCMD pCmd = vmmdevHGCMCmdAlloc(VBOXHGCMCMDTYPE_CONNECT, GCPhys, pHGCMConnect->header.header.size, 0);
    if (pCmd)
    {
        vmmdevHGCMConnectFetch(pHGCMConnect, pCmd);

        /* Only allow the guest to use existing services! */
        ASSERT_GUEST(pCmd->u.connect.loc.type == VMMDevHGCMLoc_LocalHost_Existing);
        pCmd->u.connect.loc.type = VMMDevHGCMLoc_LocalHost_Existing;

        vmmdevHGCMAddCommand(pThis, pCmd);
        rc = pThis->pHGCMDrv->pfnConnect(pThis->pHGCMDrv, pCmd, &pCmd->u.connect.loc, &pCmd->u.connect.u32ClientID);
        if (RT_FAILURE(rc))
            vmmdevHGCMRemoveCommand(pThis, pCmd);
    }
    else
    {
        rc = VERR_NO_MEMORY;
    }

    return rc;
}

/** Copy VMMDevHGCMDisconnect request data from the guest to VBOXHGCMCMD command.
 *
 * @param   pHGCMDisconnect The source guest request (cached in host memory).
 * @param   pCmd            Destination command.
 */
static void vmmdevHGCMDisconnectFetch(const VMMDevHGCMDisconnect *pHGCMDisconnect, PVBOXHGCMCMD pCmd)
{
    pCmd->enmRequestType = pHGCMDisconnect->header.header.requestType;
    pCmd->u.disconnect.u32ClientID = pHGCMDisconnect->u32ClientID;
}

/** Handle VMMDevHGCMDisconnect request.
 *
 * @param   pThis           The VMMDev instance data.
 * @param   pHGCMDisconnect The guest request (cached in host memory).
 * @param   GCPhys          The physical address of the request.
 */
int vmmdevHGCMDisconnect(PVMMDEV pThis, const VMMDevHGCMDisconnect *pHGCMDisconnect, RTGCPHYS GCPhys)
{
    int rc = VINF_SUCCESS;

    PVBOXHGCMCMD pCmd = vmmdevHGCMCmdAlloc(VBOXHGCMCMDTYPE_DISCONNECT, GCPhys, pHGCMDisconnect->header.header.size, 0);
    if (pCmd)
    {
        vmmdevHGCMDisconnectFetch(pHGCMDisconnect, pCmd);

        vmmdevHGCMAddCommand(pThis, pCmd);
        rc = pThis->pHGCMDrv->pfnDisconnect (pThis->pHGCMDrv, pCmd, pCmd->u.disconnect.u32ClientID);
        if (RT_FAILURE(rc))
            vmmdevHGCMRemoveCommand(pThis, pCmd);
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

/** Translate LinAddr parameter type to the direction of data transfer.
 *
 * @returns VBOX_HGCM_F_PARM_DIRECTION_* flags.
 * @param   enmType         Type of the LinAddr parameter.
 */
static uint32_t vmmdevHGCMParmTypeToDirection(HGCMFunctionParameterType enmType)
{
    if (enmType == VMMDevHGCMParmType_LinAddr_In)  return VBOX_HGCM_F_PARM_DIRECTION_TO_HOST;
    if (enmType == VMMDevHGCMParmType_LinAddr_Out) return VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST;
    return VBOX_HGCM_F_PARM_DIRECTION_BOTH;
}

/** Check if list of pages in a HGCM pointer parameter corresponds to a contiguous buffer.
 *
 * @returns true if pages are contiguous, false otherwise.
 * @param   pPtr            Information about a pointer HGCM parameter.
 */
DECLINLINE(bool) vmmdevHGCMGuestBufferIsContiguous(const VBOXHGCMPARMPTR *pPtr)
{
    if (pPtr->cPages == 1)
        return true;
    RTGCPHYS64 Phys = pPtr->paPages[0] + PAGE_SIZE;
    if (Phys != pPtr->paPages[1])
        return false;
    if (pPtr->cPages > 2)
    {
        uint32_t iPage = 2;
        do
        {
            Phys += PAGE_SIZE;
            if (Phys != pPtr->paPages[iPage])
                return false;
            ++iPage;
        } while (iPage < pPtr->cPages);
    }
    return true;
}

/** Copy data from guest memory to the host buffer.
 *
 * @returns VBox status code.
 * @param   pDevIns         The device instance for PDMDevHlp.
 * @param   pvDst           The destination host buffer.
 * @param   cbDst           Size of the destination host buffer.
 * @param   pPtr            Description of the source HGCM pointer parameter.
 */
static int vmmdevHGCMGuestBufferRead(PPDMDEVINSR3 pDevIns, void *pvDst, uint32_t cbDst,
                                     const VBOXHGCMPARMPTR *pPtr)
{
    /*
     * Try detect contiguous buffers.
     */
    /** @todo We need a flag for indicating this. */
    if (vmmdevHGCMGuestBufferIsContiguous(pPtr))
        return PDMDevHlpPhysRead(pDevIns, pPtr->paPages[0] | pPtr->offFirstPage, pvDst, cbDst);

    /*
     * Page by page fallback
     */
    int rc = VINF_SUCCESS;

    uint8_t *pu8Dst = (uint8_t *)pvDst;
    uint32_t offPage = pPtr->offFirstPage;
    uint32_t cbRemaining = cbDst;

    uint32_t iPage;
    for (iPage = 0; iPage < pPtr->cPages && cbRemaining > 0; ++iPage)
    {
        uint32_t cbToRead = PAGE_SIZE - offPage;
        if (cbToRead > cbRemaining)
            cbToRead = cbRemaining;

        /* Skip invalid pages. */
        const RTGCPHYS GCPhys = pPtr->paPages[iPage];
        if (GCPhys != NIL_RTGCPHYS)
        {
            rc = PDMDevHlpPhysRead(pDevIns, GCPhys + offPage, pu8Dst, cbToRead);
            AssertRCBreak(rc);
        }

        offPage = 0; /* A next page is read from 0 offset. */
        cbRemaining -= cbToRead;
        pu8Dst += cbToRead;
    }

    return rc;
}

/** Copy data from the host buffer to guest memory.
 *
 * @returns VBox status code.
 * @param   pDevIns         The device instance for PDMDevHlp.
 * @param   pPtr            Description of the destination HGCM pointer parameter.
 * @param   pvSrc           The source host buffer.
 * @param   cbSrc           Size of the source host buffer.
 */
static int vmmdevHGCMGuestBufferWrite(PPDMDEVINSR3 pDevIns, const VBOXHGCMPARMPTR *pPtr,
                                      const void *pvSrc,  uint32_t cbSrc)
{
    int rc = VINF_SUCCESS;

    uint8_t *pu8Src = (uint8_t *)pvSrc;
    uint32_t offPage = pPtr->offFirstPage;
    uint32_t cbRemaining = cbSrc;

    uint32_t iPage;
    for (iPage = 0; iPage < pPtr->cPages && cbRemaining > 0; ++iPage)
    {
        uint32_t cbToWrite = PAGE_SIZE - offPage;
        if (cbToWrite > cbRemaining)
            cbToWrite = cbRemaining;

        /* Skip invalid pages. */
        const RTGCPHYS GCPhys = pPtr->paPages[iPage];
        if (GCPhys != NIL_RTGCPHYS)
        {
            rc = PDMDevHlpPhysWrite(pDevIns, GCPhys + offPage, pu8Src, cbToWrite);
            AssertRCBreak(rc);
        }

        offPage = 0; /* A next page is written at 0 offset. */
        cbRemaining -= cbToWrite;
        pu8Src += cbToWrite;
    }

    return rc;
}

/** Initializes pCmd->paHostParms from already initialized pCmd->paGuestParms.
 * Allocates memory for pointer parameters and copies data from the guest.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pCmd            Command structure where host parameters needs initialization.
 */
static int vmmdevHGCMInitHostParameters(PVMMDEV pThis, PVBOXHGCMCMD pCmd)
{
    AssertReturn(pCmd->enmCmdType == VBOXHGCMCMDTYPE_CALL, VERR_INTERNAL_ERROR);

    int rc = VINF_SUCCESS;

    uint32_t i;
    for (i = 0; i < pCmd->u.call.cParms; ++i)
    {
        VBOXHGCMGUESTPARM * const pGuestParm = &pCmd->u.call.paGuestParms[i];
        VBOXHGCMSVCPARM   * const pHostParm  = &pCmd->u.call.paHostParms[i];

        switch (pGuestParm->enmType)
        {
            case VMMDevHGCMParmType_32bit:
            {
                pHostParm->type = VBOX_HGCM_SVC_PARM_32BIT;
                pHostParm->u.uint32 = (uint32_t)pGuestParm->u.val.u64Value;

                break;
            }

            case VMMDevHGCMParmType_64bit:
            {
                pHostParm->type = VBOX_HGCM_SVC_PARM_64BIT;
                pHostParm->u.uint64 = pGuestParm->u.val.u64Value;

                break;
            }

            case VMMDevHGCMParmType_LinAddr_In:
            case VMMDevHGCMParmType_LinAddr_Out:
            case VMMDevHGCMParmType_LinAddr:
            case VMMDevHGCMParmType_PageList:
            {
                const uint32_t cbData = pGuestParm->u.ptr.cbData;

                pHostParm->type = VBOX_HGCM_SVC_PARM_PTR;
                pHostParm->u.pointer.size = cbData;

                if (cbData)
                {
                    /* Zero memory, the buffer content is potentially copied to the guest. */
                    void *pv = RTMemAllocZ(cbData);
                    AssertBreakStmt(pv, rc = VERR_NO_MEMORY);
                    pHostParm->u.pointer.addr = pv;

                    if (pGuestParm->u.ptr.fu32Direction & VBOX_HGCM_F_PARM_DIRECTION_TO_HOST)
                    {
                        rc = vmmdevHGCMGuestBufferRead(pThis->pDevIns, pv, cbData, &pGuestParm->u.ptr);
                        ASSERT_GUEST_BREAK(RT_SUCCESS(rc));
                        RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();
                    }
                }
                else
                {
                    pHostParm->u.pointer.addr = NULL;
                }

                break;
            }

            default:
                ASSERT_GUEST_FAILED_STMT_BREAK(rc = VERR_INVALID_PARAMETER);
        }

        if (RT_FAILURE(rc))
            break;
    }

    return rc;
}

/** Allocate and initialize VBOXHGCMCMD structure for a HGCMCall request.
 *
 * @returns VBox status code that the guest should see.
 * @param   pHGCMCall       The HGCMCall request (cached in host memory).
 * @param   cbHGCMCall      Size of the request.
 * @param   GCPhys          Guest physical address of the request.
 * @param   enmRequestType  The request type. Distinguishes 64 and 32 bit calls.
 * @param   ppCmd           Where to store pointer to allocated command.
 * @param   pcbHGCMParmStruct Where to store size of used HGCM parameter structure.
 */
static int vmmdevHGCMCallAlloc(const VMMDevHGCMCall *pHGCMCall, uint32_t cbHGCMCall, RTGCPHYS GCPhys,
                               VMMDevRequestType enmRequestType, PVBOXHGCMCMD *ppCmd, uint32_t *pcbHGCMParmStruct)
{
#ifdef VBOX_WITH_64_BITS_GUESTS
    const uint32_t cbHGCMParmStruct = enmRequestType == VMMDevReq_HGCMCall64 ? sizeof(HGCMFunctionParameter64)
                                                                             : sizeof(HGCMFunctionParameter32);
#else
    const uint32_t cbHGCMParmStruct = sizeof(HGCMFunctionParameter);
#endif

    const uint32_t cParms = pHGCMCall->cParms;

    /* Whether there is enough space for parameters and sane upper limit. */
    ASSERT_GUEST_STMT_RETURN(   cParms <= (cbHGCMCall - sizeof(VMMDevHGCMCall)) / cbHGCMParmStruct
                             && cParms <= VMMDEV_MAX_HGCM_PARMS,
                             LogRelMax(50, ("VMMDev: request packet with invalid number of HGCM parameters: %d vs %d. Refusing operation.\n",
                                           (cbHGCMCall - sizeof(VMMDevHGCMCall)) / cbHGCMParmStruct, cParms)),
                             VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    PVBOXHGCMCMD pCmd = vmmdevHGCMCmdAlloc(VBOXHGCMCMDTYPE_CALL, GCPhys, cbHGCMCall, cParms);
    if (pCmd == NULL)
        return VERR_NO_MEMORY;

    /* Request type has been validated in vmmdevReqDispatcher. */
    pCmd->enmRequestType     = enmRequestType;
    pCmd->u.call.u32ClientID = pHGCMCall->u32ClientID;
    pCmd->u.call.u32Function = pHGCMCall->u32Function;

    *ppCmd = pCmd;
    *pcbHGCMParmStruct = cbHGCMParmStruct;
    return VINF_SUCCESS;
}

/** Copy VMMDevHGCMCall request data from the guest to VBOXHGCMCMD command.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pCmd            The destination command.
 * @param   pHGCMCall       The HGCMCall request (cached in host memory).
 * @param   cbHGCMCall      Size of the request.
 * @param   enmRequestType  The request type. Distinguishes 64 and 32 bit calls.
 * @param   cbHGCMParmStruct Size of used HGCM parameter structure.
 */
static int vmmdevHGCMCallFetchGuestParms(PVMMDEV pThis, PVBOXHGCMCMD pCmd,
                                         const VMMDevHGCMCall *pHGCMCall, uint32_t cbHGCMCall,
                                         VMMDevRequestType enmRequestType, uint32_t cbHGCMParmStruct)
{
#ifdef VBOX_WITH_64_BITS_GUESTS
    const bool f64Bits = (enmRequestType == VMMDevReq_HGCMCall64);
#endif

    int rc = VINF_SUCCESS;

    const uint32_t cParms = pCmd->u.call.cParms;

    /*
     * Go over all guest parameters and initialize relevant VBOXHGCMCMD fields.
     * VBOXHGCMCMD must contain all information about the request,
     * the request will be not read from the guest memory again.
     */

    /* Offsets in the request buffer to HGCM parameters and additional data. */
    const uint32_t offHGCMParms = sizeof(VMMDevHGCMCall);
    const uint32_t offExtra = offHGCMParms + cParms * cbHGCMParmStruct;

    /* Pointer to the next HGCM parameter of the request. */
    const uint8_t *pu8HGCMParm = (uint8_t *)pHGCMCall + offHGCMParms;

    uint32_t cbTotalData = 0;
    uint32_t i;
    for (i = 0; i < cParms && RT_SUCCESS(rc); ++i, pu8HGCMParm += cbHGCMParmStruct)
    {
        VBOXHGCMGUESTPARM * const pGuestParm = &pCmd->u.call.paGuestParms[i];

#ifdef VBOX_WITH_64_BITS_GUESTS
        pGuestParm->enmType = f64Bits ? ((HGCMFunctionParameter64 *)pu8HGCMParm)->type
                                      : ((HGCMFunctionParameter32 *)pu8HGCMParm)->type;
#else
        pGuestParm->enmType = ((HGCMFunctionParameter *)pu8HGCMParm)->type;
#endif

        switch (pGuestParm->enmType)
        {
            case VMMDevHGCMParmType_32bit:
            {
#ifdef VBOX_WITH_64_BITS_GUESTS
                uint32_t *pu32 = f64Bits ? &((HGCMFunctionParameter64 *)pu8HGCMParm)->u.value32
                                         : &((HGCMFunctionParameter32 *)pu8HGCMParm)->u.value32;
#else
                uint32_t *pu32 = &((HGCMFunctionParameter *)pu8HGCMParm)->u.value32;
#endif
                LogFunc(("uint32 guest parameter %RI32\n", *pu32));

                pGuestParm->u.val.u64Value = *pu32;
                pGuestParm->u.val.offValue = (uint32_t)((uintptr_t)pu32 - (uintptr_t)pHGCMCall);
                pGuestParm->u.val.cbValue = sizeof(uint32_t);

                break;
            }

            case VMMDevHGCMParmType_64bit:
            {
#ifdef VBOX_WITH_64_BITS_GUESTS
                uint64_t *pu64 = f64Bits ? &((HGCMFunctionParameter64 *)pu8HGCMParm)->u.value64
                                         : &((HGCMFunctionParameter32 *)pu8HGCMParm)->u.value64;
#else
                uint64_t *pu64 = &((HGCMFunctionParameter *)pu8HGCMParm)->u.value64;
#endif
                LogFunc(("uint64 guest parameter %RI64\n", *pu64));

                pGuestParm->u.val.u64Value = *pu64;
                pGuestParm->u.val.offValue = (uint32_t)((uintptr_t)pu64 - (uintptr_t)pHGCMCall);
                pGuestParm->u.val.cbValue = sizeof(uint64_t);

                break;
            }

            case VMMDevHGCMParmType_LinAddr_In:  /* In (read) */
            case VMMDevHGCMParmType_LinAddr_Out: /* Out (write) */
            case VMMDevHGCMParmType_LinAddr:     /* In & Out */
            {
#ifdef VBOX_WITH_64_BITS_GUESTS
                uint32_t cbData = f64Bits ? ((HGCMFunctionParameter64 *)pu8HGCMParm)->u.Pointer.size
                                          : ((HGCMFunctionParameter32 *)pu8HGCMParm)->u.Pointer.size;
                RTGCPTR GCPtr = f64Bits ? ((HGCMFunctionParameter64 *)pu8HGCMParm)->u.Pointer.u.linearAddr
                                        : ((HGCMFunctionParameter32 *)pu8HGCMParm)->u.Pointer.u.linearAddr;
#else
                uint32_t cbData = ((HGCMFunctionParameter *)pu8HGCMParm)->u.Pointer.size;
                RTGCPTR GCPtr = ((HGCMFunctionParameter *)pu8HGCMParm)->u.Pointer.u.linearAddr;
#endif
                LogFunc(("LinAddr guest parameter %RGv, cb %u\n", GCPtr, cbData));

                ASSERT_GUEST_STMT_BREAK(cbData <= VMMDEV_MAX_HGCM_DATA_SIZE - cbTotalData,
                                        rc = VERR_INVALID_PARAMETER);
                cbTotalData += cbData;

                const uint32_t offFirstPage = cbData > 0 ? GCPtr & PAGE_OFFSET_MASK : 0;
                const uint32_t cPages       = cbData > 0 ? (offFirstPage + cbData + PAGE_SIZE - 1) / PAGE_SIZE : 0;

                pGuestParm->u.ptr.cbData        = cbData;
                pGuestParm->u.ptr.offFirstPage  = offFirstPage;
                pGuestParm->u.ptr.cPages        = cPages;
                pGuestParm->u.ptr.fu32Direction = vmmdevHGCMParmTypeToDirection(pGuestParm->enmType);

                if (cbData > 0)
                {
                    pGuestParm->u.ptr.paPages = (RTGCPHYS *)RTMemAlloc(cPages * sizeof(RTGCPHYS));
                    AssertBreakStmt(pGuestParm->u.ptr.paPages, rc = VERR_NO_MEMORY);

                    /* Gonvert the guest linear pointers of pages to physical addresses. */
                    GCPtr &= PAGE_BASE_GC_MASK;
                    uint32_t iPage;
                    for (iPage = 0; iPage < cPages; ++iPage)
                    {
                        /* The guest might specify invalid GCPtr, just skip such addresses.
                         * Also if the guest parameters are fetched when restoring an old saved state,
                         * then GCPtr may become invalid and do not have a corresponding GCPhys.
                         * The command restoration routine will take care of this.
                         */
                        RTGCPHYS GCPhys;
                        int rc2 = PDMDevHlpPhysGCPtr2GCPhys(pThis->pDevIns, GCPtr, &GCPhys);
                        if (RT_FAILURE(rc2))
                            GCPhys = NIL_RTGCPHYS;
                        LogFunc(("Page %d: %RGv -> %RGp. %Rrc\n", iPage, GCPtr, GCPhys, rc2));

                        pGuestParm->u.ptr.paPages[iPage] = GCPhys;
                        GCPtr += PAGE_SIZE;
                    }
                }

                break;
            }

            case VMMDevHGCMParmType_PageList:
            {
#ifdef VBOX_WITH_64_BITS_GUESTS
                uint32_t cbData = f64Bits ? ((HGCMFunctionParameter64 *)pu8HGCMParm)->u.PageList.size
                                          : ((HGCMFunctionParameter32 *)pu8HGCMParm)->u.PageList.size;
                uint32_t offPageListInfo = f64Bits ? ((HGCMFunctionParameter64 *)pu8HGCMParm)->u.PageList.offset
                                                   : ((HGCMFunctionParameter32 *)pu8HGCMParm)->u.PageList.offset;
#else
                uint32_t cbData = ((HGCMFunctionParameter *)pu8HGCMParm)->u.PageList.size;
                uint32_t offPageListInfo = ((HGCMFunctionParameter *)pu8HGCMParm)->u.PageList.offset;
#endif
                LogFunc(("PageList guest parameter cb %u, offset %u\n", cbData, offPageListInfo));

                ASSERT_GUEST_STMT_BREAK(cbData <= VMMDEV_MAX_HGCM_DATA_SIZE - cbTotalData,
                                        rc = VERR_INVALID_PARAMETER);
                cbTotalData += cbData;

                /* Check that the page list info is within the request. */
                ASSERT_GUEST_STMT_BREAK(   offPageListInfo >= offExtra
                                        && cbHGCMCall >= sizeof(HGCMPageListInfo)
                                        && offPageListInfo <= cbHGCMCall - sizeof(HGCMPageListInfo),
                                        rc = VERR_INVALID_PARAMETER);
                RT_UNTRUSTED_VALIDATED_FENCE();

                /* The HGCMPageListInfo structure is within the request. */
                const HGCMPageListInfo *pPageListInfo = (HGCMPageListInfo *)((uint8_t *)pHGCMCall + offPageListInfo);

                /* Enough space for page pointers? */
                const uint32_t cMaxPages = 1 + (cbHGCMCall - offPageListInfo - sizeof(HGCMPageListInfo)) / sizeof(RTGCPHYS);
                ASSERT_GUEST_STMT_BREAK(   pPageListInfo->cPages > 0
                                        && pPageListInfo->cPages <= cMaxPages,
                                        rc = VERR_INVALID_PARAMETER);

                /* Other fields of PageListInfo. */
                ASSERT_GUEST_STMT_BREAK(   (pPageListInfo->flags & ~VBOX_HGCM_F_PARM_DIRECTION_BOTH) == 0
                                        && pPageListInfo->offFirstPage < PAGE_SIZE,
                                        rc = VERR_INVALID_PARAMETER);
                RT_UNTRUSTED_VALIDATED_FENCE();

                /* cbData is not checked to fit into the pages, because the host code does not access
                 * more than the provided number of pages.
                 */

                pGuestParm->u.ptr.cbData        = cbData;
                pGuestParm->u.ptr.offFirstPage  = pPageListInfo->offFirstPage;
                pGuestParm->u.ptr.cPages        = pPageListInfo->cPages;
                pGuestParm->u.ptr.fu32Direction = pPageListInfo->flags;
                pGuestParm->u.ptr.paPages       = (RTGCPHYS *)RTMemAlloc(pPageListInfo->cPages * sizeof(RTGCPHYS));
                AssertBreakStmt(pGuestParm->u.ptr.paPages, rc = VERR_NO_MEMORY);

                uint32_t iPage;
                for (iPage = 0; iPage < pGuestParm->u.ptr.cPages; ++iPage)
                    pGuestParm->u.ptr.paPages[iPage] = pPageListInfo->aPages[iPage];

                break;
            }

            default:
                ASSERT_GUEST_FAILED_STMT_BREAK(rc = VERR_INVALID_PARAMETER);
        }
    }

    return rc;
}

/**
 * Handles VMMDevHGCMCall request.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pHGCMCall       The request to handle (cached in host memory).
 * @param   cbHGCMCall      Size of the entire request (including HGCM parameters).
 * @param   GCPhys          The guest physical address of the request.
 * @param   enmRequestType  The request type. Distinguishes 64 and 32 bit calls.
 */
int vmmdevHGCMCall(PVMMDEV pThis, const VMMDevHGCMCall *pHGCMCall, uint32_t cbHGCMCall, RTGCPHYS GCPhys, VMMDevRequestType enmRequestType)
{
    LogFunc(("client id = %d, function = %d, cParms = %d, enmRequestType = %d\n",
             pHGCMCall->u32ClientID, pHGCMCall->u32Function, pHGCMCall->cParms, enmRequestType));

    ASSERT_GUEST_RETURN(cbHGCMCall >= sizeof(VMMDevHGCMCall), VERR_INVALID_PARAMETER);
#ifdef VBOX_WITH_64_BITS_GUESTS
    ASSERT_GUEST_RETURN(   enmRequestType == VMMDevReq_HGCMCall32
                        || enmRequestType == VMMDevReq_HGCMCall64, VERR_INVALID_PARAMETER);
#else
    ASSERT_GUEST_RETURN(enmRequestType == VMMDevReq_HGCMCall, VERR_INVALID_PARAMETER);
#endif
    RT_UNTRUSTED_VALIDATED_FENCE();

    PVBOXHGCMCMD pCmd;
    uint32_t cbHGCMParmStruct;
    int rc = vmmdevHGCMCallAlloc(pHGCMCall, cbHGCMCall, GCPhys, enmRequestType, &pCmd, &cbHGCMParmStruct);
    if (RT_SUCCESS(rc))
        /* likely */;
    else
        return rc;

    rc = vmmdevHGCMCallFetchGuestParms(pThis, pCmd, pHGCMCall, cbHGCMCall, enmRequestType, cbHGCMParmStruct);

    /* Copy guest data to host parameters, so HGCM services can use the data. */
    if (RT_SUCCESS(rc))
        rc = vmmdevHGCMInitHostParameters(pThis, pCmd);

    if (RT_SUCCESS(rc))
    {
        vmmdevHGCMAddCommand(pThis, pCmd);
        /* Pass the function call to HGCM connector for actual processing */
        rc = pThis->pHGCMDrv->pfnCall(pThis->pHGCMDrv, pCmd,
                                      pCmd->u.call.u32ClientID, pCmd->u.call.u32Function,
                                      pCmd->u.call.cParms, pCmd->u.call.paHostParms);
        if (RT_FAILURE(rc))
        {
            LogFunc(("pfnCall rc = %Rrc\n", rc));
            vmmdevHGCMRemoveCommand(pThis, pCmd);
        }
    }

    if (RT_FAILURE(rc))
    {
        /* Cleanup on failure. */
        vmmdevHGCMCmdFree(pCmd);
    }

    return rc;
}

/**
 * VMMDevReq_HGCMCancel worker.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pHGCMCancel     The request to handle (cached in host memory).
 * @param   GCPhys          The address of the request.
 *
 * @thread EMT
 */
int vmmdevHGCMCancel(PVMMDEV pThis, const VMMDevHGCMCancel *pHGCMCancel, RTGCPHYS GCPhys)
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
 * @param   pThis           The VMMDev instance data.
 * @param   GCPhys          The address of the request that should be cancelled.
 *
 * @thread EMT
 */
int vmmdevHGCMCancel2(PVMMDEV pThis, RTGCPHYS GCPhys)
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
    int rc = vmmdevHGCMCmdListLock(pThis);
    AssertRCReturn(rc, rc);

    PVBOXHGCMCMD pCmd = vmmdevHGCMFindCommandLocked(pThis, GCPhys);
    if (pCmd)
    {
        pCmd->fCancelled = true;
        Log(("vmmdevHGCMCancel2: Cancelled pCmd=%p / GCPhys=%#x\n", pCmd, GCPhys));
    }
    else
        rc = VERR_NOT_FOUND;

    vmmdevHGCMCmdListUnlock(pThis);
    return rc;
}

/** Write HGCM call parameters and buffers back to the guest request and memory.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pCmd            Completed call command.
 * @param   pHGCMCall       The guestrequest which needs updating (cached in the host memory).
 */
static int vmmdevHGCMCompleteCallRequest(PVMMDEV pThis, PVBOXHGCMCMD pCmd, VMMDevHGCMCall *pHGCMCall)
{
    AssertReturn(pCmd->enmCmdType == VBOXHGCMCMDTYPE_CALL, VERR_INTERNAL_ERROR);

    int rc = VINF_SUCCESS;

    /*
     * Go over parameter descriptions saved in pCmd.
     */
    uint32_t i;
    for (i = 0; i < pCmd->u.call.cParms; ++i)
    {
        VBOXHGCMGUESTPARM * const pGuestParm = &pCmd->u.call.paGuestParms[i];
        VBOXHGCMSVCPARM   * const pHostParm  = &pCmd->u.call.paHostParms[i];

        const HGCMFunctionParameterType enmType = pGuestParm->enmType;
        switch (enmType)
        {
            case VMMDevHGCMParmType_32bit:
            case VMMDevHGCMParmType_64bit:
            {
                const VBOXHGCMPARMVAL * const pVal = &pGuestParm->u.val;
                const void *pvSrc = enmType == VMMDevHGCMParmType_32bit ? (void *)&pHostParm->u.uint32
                                                                        : (void *)&pHostParm->u.uint64;
                memcpy((uint8_t *)pHGCMCall + pVal->offValue, pvSrc, pVal->cbValue);
                break;
            }

            case VMMDevHGCMParmType_LinAddr_In:
            case VMMDevHGCMParmType_LinAddr_Out:
            case VMMDevHGCMParmType_LinAddr:
            case VMMDevHGCMParmType_PageList:
            {
                const VBOXHGCMPARMPTR * const pPtr = &pGuestParm->u.ptr;
                if (   pPtr->cbData > 0
                    && pPtr->fu32Direction & VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST)
                {
                    const void *pvSrc = pHostParm->u.pointer.addr;
                    uint32_t cbSrc = pHostParm->u.pointer.size;
                    rc = vmmdevHGCMGuestBufferWrite(pThis->pDevIns, pPtr, pvSrc, cbSrc);
                }
                break;
            }

            default:
                break;
        }

        if (RT_FAILURE(rc))
            break;
    }

    return rc;
}

/** Update HGCM request in the guest memory and mark it as completed.
 *
 * @param   pInterface      Pointer to this PDM interface.
 * @param   result          HGCM completion status code (VBox status code).
 * @param   pCmd            Completed command, which contains updated host parameters.
 *
 * @thread EMT
 */
DECLCALLBACK(void) hgcmCompletedWorker(PPDMIHGCMPORT pInterface, int32_t result, PVBOXHGCMCMD pCmd)
{
    PVMMDEV pThis = RT_FROM_MEMBER(pInterface, VMMDevState, IHGCMPort);
#ifdef VBOX_WITH_DTRACE
    uint32_t idFunction = 0;
    uint32_t idClient   = 0;
#endif

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

    VBOXDD_HGCMCALL_COMPLETED_EMT(pCmd, result);

    int rc = VINF_SUCCESS;

    /*
     * The cancellation protocol requires us to remove the command here
     * and then check the flag. Cancelled commands must not be written
     * back to guest memory.
     */
    vmmdevHGCMRemoveCommand(pThis, pCmd);

    if (RT_LIKELY(!pCmd->fCancelled))
    {
        VMMDevHGCMRequestHeader *pHeader = (VMMDevHGCMRequestHeader *)RTMemAlloc(pCmd->cbRequest);
        if (pHeader)
        {
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

            /*
             * Read the request from the guest memory for updating.
             * The request data is not be used for anything but checking the request type.
             */
            PDMDevHlpPhysRead(pThis->pDevIns, pCmd->GCPhys, pHeader, pCmd->cbRequest);
            RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

            /* Verify the request type. This is the only field which is used from the guest memory. */
            const VMMDevRequestType enmRequestType = pHeader->header.requestType;
            if (   enmRequestType == pCmd->enmRequestType
                || enmRequestType == VMMDevReq_HGCMCancel)
            {
                RT_UNTRUSTED_VALIDATED_FENCE();

                /*
                 * Update parameters and data buffers.
                 */
                switch (enmRequestType)
                {
#ifdef VBOX_WITH_64_BITS_GUESTS
                    case VMMDevReq_HGCMCall64:
                    case VMMDevReq_HGCMCall32:
#else
                    case VMMDevReq_HGCMCall:
#endif
                    {
                        VMMDevHGCMCall *pHGCMCall = (VMMDevHGCMCall *)pHeader;
                        rc = vmmdevHGCMCompleteCallRequest(pThis, pCmd, pHGCMCall);
#ifdef VBOX_WITH_DTRACE
                        idFunction = pCmd->u.call.u32Function;
                        idClient   = pCmd->u.call.u32ClientID;
#endif
                        break;
                    }

                    case VMMDevReq_HGCMConnect:
                    {
                        /* save the client id in the guest request packet */
                        VMMDevHGCMConnect *pHGCMConnect = (VMMDevHGCMConnect *)pHeader;
                        pHGCMConnect->u32ClientID = pCmd->u.connect.u32ClientID;
                        break;
                    }

                    default:
                        /* make compiler happy */
                        break;
                }
            }
            else
            {
                /* Guest has changed the command type. */
                LogRelMax(50, ("VMMDEV: Invalid HGCM command: pCmd->enmCmdType = 0x%08X, pHeader->header.requestType = 0x%08X\n",
                               pCmd->enmCmdType, pHeader->header.requestType));

                ASSERT_GUEST_FAILED_STMT(rc = VERR_INVALID_PARAMETER);
            }

            /* Setup return code for the guest. */
            if (RT_SUCCESS(rc))
                pHeader->result = result;
            else
                pHeader->result = rc;

            /* First write back the request. */
            PDMDevHlpPhysWrite(pThis->pDevIns, pCmd->GCPhys, pHeader, pCmd->cbRequest);

            /* Mark request as processed. */
            pHeader->fu32Flags |= VBOX_HGCM_REQ_DONE;

            /* Second write the flags to mark the request as processed. */
            PDMDevHlpPhysWrite(pThis->pDevIns, pCmd->GCPhys + RT_UOFFSETOF(VMMDevHGCMRequestHeader, fu32Flags),
                               &pHeader->fu32Flags, sizeof(pHeader->fu32Flags));

            /* Now, when the command was removed from the internal list, notify the guest. */
            VMMDevNotifyGuest(pThis, VMMDEV_EVENT_HGCM);

            RTMemFree(pHeader);
        }
        else
        {
            LogRelMax(10, ("VMMDev: Failed to allocate %u bytes for HGCM request completion!!!\n", pCmd->cbRequest));
        }
    }
    else
    {
        LogFlowFunc(("Cancelled command %p\n", pCmd));
    }

    /* Deallocate the command memory. */
    vmmdevHGCMCmdFree(pCmd);

    VBOXDD_HGCMCALL_COMPLETED_DONE(pCmd, idFunction, idClient, result);
}

/** HGCM callback for request completion. Forwards to hgcmCompletedWorker.
 *
 * @param   pInterface      Pointer to this PDM interface.
 * @param   result          HGCM completion status code (VBox status code).
 * @param   pCmd            Completed command, which contains updated host parameters.
 */
DECLCALLBACK(void) hgcmCompleted(PPDMIHGCMPORT pInterface, int32_t result, PVBOXHGCMCMD pCmd)
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

/** Save information about pending HGCM requests from pThis->listHGCMCmd.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pSSM            SSM handle for SSM functions.
 *
 * @thread EMT
 */
int vmmdevHGCMSaveState(PVMMDEV pThis, PSSMHANDLE pSSM)
{
    LogFlowFunc(("\n"));

    /* Compute how many commands are pending. */
    uint32_t cCmds = 0;
    PVBOXHGCMCMD pCmd;
    RTListForEach(&pThis->listHGCMCmd, pCmd, VBOXHGCMCMD, node)
    {
        LogFlowFunc(("pCmd %p\n", pCmd));
        ++cCmds;
    }
    LogFlowFunc(("cCmds = %d\n", cCmds));

    /* Save number of commands. */
    int rc = SSMR3PutU32(pSSM, cCmds);
    AssertRCReturn(rc, rc);

    if (cCmds > 0)
    {
        RTListForEach(&pThis->listHGCMCmd, pCmd, VBOXHGCMCMD, node)
        {
            LogFlowFunc(("Saving %RGp, size %d\n", pCmd->GCPhys, pCmd->cbRequest));

            SSMR3PutU32     (pSSM, (uint32_t)pCmd->enmCmdType);
            SSMR3PutBool    (pSSM, pCmd->fCancelled);
            SSMR3PutGCPhys  (pSSM, pCmd->GCPhys);
            SSMR3PutU32     (pSSM, pCmd->cbRequest);
            SSMR3PutU32     (pSSM, (uint32_t)pCmd->enmRequestType);
            const uint32_t cParms = pCmd->enmCmdType == VBOXHGCMCMDTYPE_CALL ? pCmd->u.call.cParms : 0;
            rc = SSMR3PutU32(pSSM, cParms);
            AssertRCReturn(rc, rc);

            if (pCmd->enmCmdType == VBOXHGCMCMDTYPE_CALL)
            {
                SSMR3PutU32     (pSSM, pCmd->u.call.u32ClientID);
                rc = SSMR3PutU32(pSSM, pCmd->u.call.u32Function);
                AssertRCReturn(rc, rc);

                /* Guest parameters. */
                uint32_t i;
                for (i = 0; i < pCmd->u.call.cParms; ++i)
                {
                    VBOXHGCMGUESTPARM * const pGuestParm = &pCmd->u.call.paGuestParms[i];

                    rc = SSMR3PutU32(pSSM, (uint32_t)pGuestParm->enmType);
                    AssertRCReturn(rc, rc);

                    if (   pGuestParm->enmType == VMMDevHGCMParmType_32bit
                        || pGuestParm->enmType == VMMDevHGCMParmType_64bit)
                    {
                        const VBOXHGCMPARMVAL * const pVal = &pGuestParm->u.val;
                        SSMR3PutU64     (pSSM, pVal->u64Value);
                        SSMR3PutU32     (pSSM, pVal->offValue);
                        rc = SSMR3PutU32(pSSM, pVal->cbValue);
                    }
                    else if (   pGuestParm->enmType == VMMDevHGCMParmType_LinAddr_In
                             || pGuestParm->enmType == VMMDevHGCMParmType_LinAddr_Out
                             || pGuestParm->enmType == VMMDevHGCMParmType_LinAddr
                             || pGuestParm->enmType == VMMDevHGCMParmType_PageList)
                    {
                        const VBOXHGCMPARMPTR * const pPtr = &pGuestParm->u.ptr;
                        SSMR3PutU32     (pSSM, pPtr->cbData);
                        SSMR3PutU32     (pSSM, pPtr->offFirstPage);
                        SSMR3PutU32     (pSSM, pPtr->cPages);
                        rc = SSMR3PutU32(pSSM, pPtr->fu32Direction);

                        uint32_t iPage;
                        for (iPage = 0; iPage < pPtr->cPages; ++iPage)
                            rc = SSMR3PutGCPhys(pSSM, pPtr->paPages[iPage]);
                    }
                    else
                    {
                        AssertFailedStmt(rc = VERR_INTERNAL_ERROR);
                    }
                    AssertRCReturn(rc, rc);
                }
            }
            else if (pCmd->enmCmdType == VBOXHGCMCMDTYPE_CONNECT)
            {
                SSMR3PutU32(pSSM, pCmd->u.connect.u32ClientID);
                SSMR3PutMem(pSSM, &pCmd->u.connect.loc, sizeof(pCmd->u.connect.loc));
            }
            else if (pCmd->enmCmdType == VBOXHGCMCMDTYPE_DISCONNECT)
            {
                SSMR3PutU32(pSSM, pCmd->u.disconnect.u32ClientID);
            }
            else
            {
                AssertFailedReturn(VERR_INTERNAL_ERROR);
            }

            /* A reserved field, will allow to extend saved data for a command. */
            rc = SSMR3PutU32(pSSM, 0);
            AssertRCReturn(rc, rc);
        }
    }

    /* A reserved field, will allow to extend saved data for VMMDevHGCM. */
    rc = SSMR3PutU32(pSSM, 0);
    AssertRCReturn(rc, rc);

    return rc;
}

/** Load information about pending HGCM requests.
 *
 * Allocate VBOXHGCMCMD commands and add them to pThis->listHGCMCmd temporarily.
 * vmmdevHGCMLoadStateDone will process the temporary list.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   pSSM            SSM handle for SSM functions.
 * @param   uVersion        Saved state version.
 *
 * @thread EMT
 */
int vmmdevHGCMLoadState(PVMMDEV pThis, PSSMHANDLE pSSM, uint32_t uVersion)
{
    LogFlowFunc(("\n"));

    pThis->u32SSMVersion = uVersion; /* For vmmdevHGCMLoadStateDone */

    /* Read how many commands were pending. */
    uint32_t cCmds = 0;
    int rc = SSMR3GetU32(pSSM, &cCmds);
    AssertRCReturn(rc, rc);

    LogFlowFunc(("cCmds = %d\n", cCmds));

    if (uVersion >= VMMDEV_SAVED_STATE_VERSION_HGCM_PARAMS)
    {
        /* Saved information about all HGCM parameters. */
        uint32_t u32;

        uint32_t iCmd;
        for (iCmd = 0; iCmd < cCmds; ++iCmd)
        {
            /* Command fields. */
            VBOXHGCMCMDTYPE   enmCmdType;
            bool              fCancelled;
            RTGCPHYS          GCPhys;
            uint32_t          cbRequest;
            VMMDevRequestType enmRequestType;
            uint32_t          cParms;

            SSMR3GetU32     (pSSM, &u32);
            enmCmdType = (VBOXHGCMCMDTYPE)u32;
            SSMR3GetBool    (pSSM, &fCancelled);
            SSMR3GetGCPhys  (pSSM, &GCPhys);
            SSMR3GetU32     (pSSM, &cbRequest);
            SSMR3GetU32     (pSSM, &u32);
            enmRequestType = (VMMDevRequestType)u32;
            rc = SSMR3GetU32(pSSM, &cParms);
            AssertRCReturn(rc, rc);

            PVBOXHGCMCMD pCmd = vmmdevHGCMCmdAlloc(enmCmdType, GCPhys, cbRequest, cParms);
            AssertReturn(pCmd, VERR_NO_MEMORY);

            pCmd->fCancelled     = fCancelled;
            pCmd->GCPhys         = GCPhys;
            pCmd->cbRequest      = cbRequest;
            pCmd->enmRequestType = enmRequestType;

            if (enmCmdType == VBOXHGCMCMDTYPE_CALL)
            {
                SSMR3GetU32     (pSSM, &pCmd->u.call.u32ClientID);
                rc = SSMR3GetU32(pSSM, &pCmd->u.call.u32Function);
                AssertRCReturn(rc, rc);

                /* Guest parameters. */
                uint32_t i;
                for (i = 0; i < cParms; ++i)
                {
                    VBOXHGCMGUESTPARM * const pGuestParm = &pCmd->u.call.paGuestParms[i];

                    rc = SSMR3GetU32(pSSM, &u32);
                    AssertRCReturn(rc, rc);
                    pGuestParm->enmType = (HGCMFunctionParameterType)u32;

                    if (   pGuestParm->enmType == VMMDevHGCMParmType_32bit
                        || pGuestParm->enmType == VMMDevHGCMParmType_64bit)
                    {
                        VBOXHGCMPARMVAL * const pVal = &pGuestParm->u.val;
                        SSMR3GetU64     (pSSM, &pVal->u64Value);
                        SSMR3GetU32     (pSSM, &pVal->offValue);
                        rc = SSMR3GetU32(pSSM, &pVal->cbValue);
                    }
                    else if (   pGuestParm->enmType == VMMDevHGCMParmType_LinAddr_In
                             || pGuestParm->enmType == VMMDevHGCMParmType_LinAddr_Out
                             || pGuestParm->enmType == VMMDevHGCMParmType_LinAddr
                             || pGuestParm->enmType == VMMDevHGCMParmType_PageList)
                    {
                        VBOXHGCMPARMPTR * const pPtr = &pGuestParm->u.ptr;
                        SSMR3GetU32     (pSSM, &pPtr->cbData);
                        SSMR3GetU32     (pSSM, &pPtr->offFirstPage);
                        SSMR3GetU32     (pSSM, &pPtr->cPages);
                        rc = SSMR3GetU32(pSSM, &pPtr->fu32Direction);
                        if (RT_SUCCESS(rc))
                        {
                            pPtr->paPages = (RTGCPHYS *)RTMemAlloc(pPtr->cPages * sizeof(RTGCPHYS));
                            AssertStmt(pPtr->paPages, rc = VERR_NO_MEMORY);

                            if (RT_SUCCESS(rc))
                            {
                                uint32_t iPage;
                                for (iPage = 0; iPage < pPtr->cPages; ++iPage)
                                    rc = SSMR3GetGCPhys(pSSM, &pPtr->paPages[iPage]);
                            }
                        }
                    }
                    else
                    {
                        AssertFailedStmt(rc = VERR_INTERNAL_ERROR);
                    }
                    AssertRCReturn(rc, rc);
                }
            }
            else if (enmCmdType == VBOXHGCMCMDTYPE_CONNECT)
            {
                SSMR3GetU32(pSSM, &pCmd->u.connect.u32ClientID);
                rc = SSMR3GetMem(pSSM, &pCmd->u.connect.loc, sizeof(pCmd->u.connect.loc));
                AssertRCReturn(rc, rc);
            }
            else if (enmCmdType == VBOXHGCMCMDTYPE_DISCONNECT)
            {
                rc = SSMR3GetU32(pSSM, &pCmd->u.disconnect.u32ClientID);
                AssertRCReturn(rc, rc);
            }
            else
            {
                AssertFailedReturn(VERR_INTERNAL_ERROR);
            }

            /* A reserved field, will allow to extend saved data for a command. */
            rc = SSMR3GetU32(pSSM, &u32);
            AssertRCReturn(rc, rc);

            vmmdevHGCMAddCommand(pThis, pCmd);
        }

        /* A reserved field, will allow to extend saved data for VMMDevHGCM. */
        rc = SSMR3GetU32(pSSM, &u32);
        AssertRCReturn(rc, rc);
    }
    else if (uVersion >= 9)
    {
        /* Version 9+: Load information about commands. Pre-rewrite. */
        uint32_t u32;

        uint32_t iCmd;
        for (iCmd = 0; iCmd < cCmds; ++iCmd)
        {
            VBOXHGCMCMDTYPE   enmCmdType;
            bool              fCancelled;
            RTGCPHYS          GCPhys;
            uint32_t          cbRequest;
            uint32_t          cLinAddrs;

            SSMR3GetGCPhys  (pSSM, &GCPhys);
            rc = SSMR3GetU32(pSSM, &cbRequest);
            AssertRCReturn(rc, rc);

            LogFlowFunc(("Restoring %RGp size %x bytes\n", GCPhys, cbRequest));

            /* For uVersion <= 12, this was the size of entire command.
             * Now the command is reconstructed in vmmdevHGCMLoadStateDone.
             */
            if (uVersion <= 12)
                SSMR3Skip(pSSM, sizeof (uint32_t));

            SSMR3GetU32     (pSSM, &u32);
            enmCmdType = (VBOXHGCMCMDTYPE)u32;
            SSMR3GetBool    (pSSM, &fCancelled);
            /* How many linear pointers. Always 0 if not a call command. */
            rc = SSMR3GetU32(pSSM, &cLinAddrs);
            AssertRCReturn(rc, rc);

            PVBOXHGCMCMD pCmd = vmmdevHGCMCmdAlloc(enmCmdType, GCPhys, cbRequest, cLinAddrs);
            AssertReturn(pCmd, VERR_NO_MEMORY);

            pCmd->fCancelled = fCancelled;
            pCmd->GCPhys     = GCPhys;
            pCmd->cbRequest  = cbRequest;

            if (cLinAddrs > 0)
            {
                /* Skip number of pages for all LinAddrs in this command. */
                SSMR3Skip(pSSM, sizeof(uint32_t));

                uint32_t i;
                for (i = 0; i < cLinAddrs; ++i)
                {
                    VBOXHGCMPARMPTR * const pPtr = &pCmd->u.call.paGuestParms[i].u.ptr;

                    /* Index of the parameter. Use cbData field to store the index. */
                    SSMR3GetU32     (pSSM, &pPtr->cbData);
                    SSMR3GetU32     (pSSM, &pPtr->offFirstPage);
                    rc = SSMR3GetU32(pSSM, &pPtr->cPages);
                    AssertRCReturn(rc, rc);

                    pPtr->paPages = (RTGCPHYS *)RTMemAlloc(pPtr->cPages * sizeof(RTGCPHYS));
                    AssertReturn(pPtr->paPages, VERR_NO_MEMORY);

                    uint32_t iPage;
                    for (iPage = 0; iPage < pPtr->cPages; ++iPage)
                        rc = SSMR3GetGCPhys(pSSM, &pPtr->paPages[iPage]);
                }
            }

            /* A reserved field, will allow to extend saved data for a command. */
            rc = SSMR3GetU32(pSSM, &u32);
            AssertRCReturn(rc, rc);

            vmmdevHGCMAddCommand(pThis, pCmd);
        }

        /* A reserved field, will allow to extend saved data for VMMDevHGCM. */
        rc = SSMR3GetU32(pSSM, &u32);
        AssertRCReturn(rc, rc);
    }
    else
    {
        /* Ancient. Only the guest physical address is saved. */
        uint32_t iCmd;
        for (iCmd = 0; iCmd < cCmds; ++iCmd)
        {
            RTGCPHYS GCPhys;
            uint32_t cbRequest;

            SSMR3GetGCPhys(pSSM, &GCPhys);
            rc = SSMR3GetU32(pSSM, &cbRequest);
            AssertRCReturn(rc, rc);

            LogFlowFunc(("Restoring %RGp size %x bytes\n", GCPhys, cbRequest));

            PVBOXHGCMCMD pCmd = vmmdevHGCMCmdAlloc(VBOXHGCMCMDTYPE_LOADSTATE, GCPhys, cbRequest, 0);
            AssertReturn(pCmd, VERR_NO_MEMORY);

            vmmdevHGCMAddCommand(pThis, pCmd);
        }
    }

    return rc;
}

/** Restore HGCM connect command loaded from old saved state.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   u32SSMVersion   The saved state version the command has been loaded from.
 * @param   pLoadedCmd      Command loaded from saved state, it is imcomplete and needs restoration.
 * @param   pReq            The guest request (cached in host memory).
 * @param   cbReq           Size of the guest request.
 * @param   enmRequestType  Type of the HGCM request.
 * @param   ppRestoredCmd   Where to store pointer to newly allocated restored command.
 */
static int vmmdevHGCMRestoreConnect(PVMMDEV pThis, uint32_t u32SSMVersion, const VBOXHGCMCMD *pLoadedCmd,
                                    VMMDevHGCMConnect *pReq, uint32_t cbReq, VMMDevRequestType enmRequestType,
                                    VBOXHGCMCMD **ppRestoredCmd)
{
    RT_NOREF(pThis);

    int rc = VINF_SUCCESS;

    /* Verify the request.  */
    ASSERT_GUEST_RETURN(cbReq >= sizeof(*pReq), VERR_MISMATCH);
    if (u32SSMVersion >= 9)
    {
        ASSERT_GUEST_RETURN(pLoadedCmd->enmCmdType == VBOXHGCMCMDTYPE_CONNECT, VERR_MISMATCH);
    }

    PVBOXHGCMCMD pCmd = vmmdevHGCMCmdAlloc(VBOXHGCMCMDTYPE_CONNECT, pLoadedCmd->GCPhys, cbReq, 0);
    AssertReturn(pCmd, VERR_NO_MEMORY);

    if (u32SSMVersion >= 9)
        pCmd->fCancelled = pLoadedCmd->fCancelled;
    else
        pCmd->fCancelled = false;

    pCmd->enmRequestType = enmRequestType;

    vmmdevHGCMConnectFetch(pReq, pCmd);

    if (RT_SUCCESS(rc))
        *ppRestoredCmd = pCmd;

    return rc;
}

/** Restore HGCM disconnect command loaded from old saved state.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   u32SSMVersion   The saved state version the command has been loaded from.
 * @param   pLoadedCmd      Command loaded from saved state, it is imcomplete and needs restoration.
 * @param   pReq            The guest request (cached in host memory).
 * @param   cbReq           Size of the guest request.
 * @param   enmRequestType  Type of the HGCM request.
 * @param   ppRestoredCmd   Where to store pointer to newly allocated restored command.
 */
static int vmmdevHGCMRestoreDisconnect(PVMMDEV pThis, uint32_t u32SSMVersion, const VBOXHGCMCMD *pLoadedCmd,
                                       VMMDevHGCMDisconnect *pReq, uint32_t cbReq, VMMDevRequestType enmRequestType,
                                       VBOXHGCMCMD **ppRestoredCmd)
{
    RT_NOREF(pThis);

    int rc = VINF_SUCCESS;

    /* Verify the request.  */
    ASSERT_GUEST_RETURN(cbReq >= sizeof(*pReq), VERR_MISMATCH);
    if (u32SSMVersion >= 9)
    {
        ASSERT_GUEST_RETURN(pLoadedCmd->enmCmdType == VBOXHGCMCMDTYPE_DISCONNECT, VERR_MISMATCH);
    }

    PVBOXHGCMCMD pCmd = vmmdevHGCMCmdAlloc(VBOXHGCMCMDTYPE_DISCONNECT, pLoadedCmd->GCPhys, cbReq, 0);
    AssertReturn(pCmd, VERR_NO_MEMORY);

    if (u32SSMVersion >= 9)
        pCmd->fCancelled = pLoadedCmd->fCancelled;
    else
        pCmd->fCancelled = false;

    pCmd->enmRequestType = enmRequestType;

    vmmdevHGCMDisconnectFetch(pReq, pCmd);

    if (RT_SUCCESS(rc))
        *ppRestoredCmd = pCmd;

    return rc;
}

/** Restore HGCM call command loaded from old saved state.
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   u32SSMVersion   The saved state version the command has been loaded from.
 * @param   pLoadedCmd      Command loaded from saved state, it is imcomplete and needs restoration.
 * @param   pReq            The guest request (cached in host memory).
 * @param   cbReq           Size of the guest request.
 * @param   enmRequestType  Type of the HGCM request.
 * @param   ppRestoredCmd   Where to store pointer to newly allocated restored command.
 */
static int vmmdevHGCMRestoreCall(PVMMDEV pThis, uint32_t u32SSMVersion, const VBOXHGCMCMD *pLoadedCmd,
                                 VMMDevHGCMCall *pReq, uint32_t cbReq, VMMDevRequestType enmRequestType,
                                 VBOXHGCMCMD **ppRestoredCmd)
{
    int rc = VINF_SUCCESS;

    /* Verify the request.  */
    ASSERT_GUEST_RETURN(cbReq >= sizeof(*pReq), VERR_MISMATCH);
    if (u32SSMVersion >= 9)
    {
        ASSERT_GUEST_RETURN(pLoadedCmd->enmCmdType == VBOXHGCMCMDTYPE_CALL, VERR_MISMATCH);
    }

    PVBOXHGCMCMD pCmd;
    uint32_t cbHGCMParmStruct;
    rc = vmmdevHGCMCallAlloc(pReq, cbReq, pLoadedCmd->GCPhys, enmRequestType, &pCmd, &cbHGCMParmStruct);
    if (RT_FAILURE(rc))
        return rc;

    /* pLoadedCmd is fake, it does not contain actual call parameters. Only pagelists for LinAddr. */
    if (u32SSMVersion >= 9)
        pCmd->fCancelled = pLoadedCmd->fCancelled;
    else
        pCmd->fCancelled = false;

    pCmd->enmRequestType = enmRequestType;

    rc = vmmdevHGCMCallFetchGuestParms(pThis, pCmd, pReq, cbReq, enmRequestType, cbHGCMParmStruct);
    if (RT_SUCCESS(rc))
    {
        /* Update LinAddr parameters from pLoadedCmd.
         * pLoadedCmd->u.call.cParms is actually the number of LinAddrs, see vmmdevHGCMLoadState.
         */
        uint32_t iLinAddr;
        for (iLinAddr = 0; iLinAddr < pLoadedCmd->u.call.cParms; ++iLinAddr)
        {
            VBOXHGCMGUESTPARM * const pLoadedParm = &pLoadedCmd->u.call.paGuestParms[iLinAddr];
            /* pLoadedParm->cbData is actually index of the LinAddr parameter, see vmmdevHGCMLoadState. */
            const uint32_t iParm = pLoadedParm->u.ptr.cbData;
            ASSERT_GUEST_STMT_BREAK(iParm < pCmd->u.call.cParms, rc = VERR_MISMATCH);

            VBOXHGCMGUESTPARM * const pGuestParm = &pCmd->u.call.paGuestParms[iParm];
            ASSERT_GUEST_STMT_BREAK(   pGuestParm->enmType == VMMDevHGCMParmType_LinAddr_In
                                    || pGuestParm->enmType == VMMDevHGCMParmType_LinAddr_Out
                                    || pGuestParm->enmType == VMMDevHGCMParmType_LinAddr,
                                    rc = VERR_MISMATCH);
            ASSERT_GUEST_STMT_BREAK(   pLoadedParm->u.ptr.offFirstPage == pGuestParm->u.ptr.offFirstPage
                                    && pLoadedParm->u.ptr.cPages       == pGuestParm->u.ptr.cPages,
                                    rc = VERR_MISMATCH);
            memcpy(pGuestParm->u.ptr.paPages, pLoadedParm->u.ptr.paPages, pGuestParm->u.ptr.cPages * sizeof(RTGCPHYS));
        }
    }

    if (RT_SUCCESS(rc))
        *ppRestoredCmd = pCmd;
    else
        vmmdevHGCMCmdFree(pCmd);

    return rc;
}

/** Allocate and initialize a HGCM command using the given request (pReqHdr)
 * and command loaded from saved state (pCmd).
 *
 * @returns VBox status code that the guest should see.
 * @param   pThis           The VMMDev instance data.
 * @param   u32SSMVersion   Saved state version.
 * @param   pLoadedCmd      HGCM command which needs restoration.
 * @param   pReqHdr         The request (cached in host memory).
 * @param   cbReq           Size of the entire request (including HGCM parameters).
 * @param   ppRestoredCmd   Where to store pointer to restored command.
 */
static int vmmdevHGCMRestoreCommand(PVMMDEV pThis, uint32_t u32SSMVersion, const VBOXHGCMCMD *pLoadedCmd,
                                    const VMMDevHGCMRequestHeader *pReqHdr, uint32_t cbReq,
                                    VBOXHGCMCMD **ppRestoredCmd)
{
    int rc = VINF_SUCCESS;

    /* Verify the request.  */
    ASSERT_GUEST_RETURN(cbReq >= sizeof(VMMDevHGCMRequestHeader), VERR_MISMATCH);
    ASSERT_GUEST_RETURN(cbReq == pReqHdr->header.size, VERR_MISMATCH);

    const VMMDevRequestType enmRequestType = pReqHdr->header.requestType;
    switch (enmRequestType)
    {
        case VMMDevReq_HGCMConnect:
        {
            VMMDevHGCMConnect *pReq = (VMMDevHGCMConnect *)pReqHdr;
            rc = vmmdevHGCMRestoreConnect(pThis, u32SSMVersion, pLoadedCmd, pReq, cbReq, enmRequestType,
                                          ppRestoredCmd);
            break;
        }

        case VMMDevReq_HGCMDisconnect:
        {
            VMMDevHGCMDisconnect *pReq = (VMMDevHGCMDisconnect *)pReqHdr;
            rc = vmmdevHGCMRestoreDisconnect(pThis, u32SSMVersion, pLoadedCmd, pReq, cbReq, enmRequestType,
                                             ppRestoredCmd);
            break;
        }

#ifdef VBOX_WITH_64_BITS_GUESTS
        case VMMDevReq_HGCMCall32:
        case VMMDevReq_HGCMCall64:
#else
        case VMMDevReq_HGCMCall:
#endif
        {
            VMMDevHGCMCall *pReq = (VMMDevHGCMCall *)pReqHdr;
            rc = vmmdevHGCMRestoreCall(pThis, u32SSMVersion, pLoadedCmd, pReq, cbReq, enmRequestType,
                                       ppRestoredCmd);
            break;
        }

        default:
            ASSERT_GUEST_FAILED_RETURN(VERR_MISMATCH);
    }

    return rc;
}

/** Resubmit pending HGCM commands which were loaded form saved state.
 *
 * @returns VBox status code.
 * @param   pThis           The VMMDev instance data.
 *
 * @thread EMT
 */
int vmmdevHGCMLoadStateDone(PVMMDEV pThis)
{
    /*
     * Resubmit pending HGCM commands to services.
     *
     * pThis->pHGCMCmdList contains commands loaded by vmmdevHGCMLoadState.
     *
     * Legacy saved states (pre VMMDEV_SAVED_STATE_VERSION_HGCM_PARAMS)
     * do not have enough information about the command parameters,
     * therefore it is necessary to reload at least some data from the
     * guest memory to construct commands.
     *
     * There are two types of legacy saved states which contain:
     * 1) the guest physical address and size of request;
     * 2) additionally page lists for LinAddr parameters.
     *
     * Legacy commands have enmCmdType = VBOXHGCMCMDTYPE_LOADSTATE?
     */

    int rcFunc = VINF_SUCCESS; /* This status code will make the function fail. I.e. VM will not start. */

    /* Get local copy of the list of loaded commands. */
    RTLISTANCHOR listLoadedCommands;
    RTListInit(&listLoadedCommands); /* RTListMove does not touch the destination is the source list is empty. */
    RTListMove(&listLoadedCommands, &pThis->listHGCMCmd);

    /* Resubmit commands. */
    PVBOXHGCMCMD pCmd, pNext;
    RTListForEachSafe(&listLoadedCommands, pCmd, pNext, VBOXHGCMCMD, node)
    {
        int rcCmd = VINF_SUCCESS; /* This status code will make the HGCM command fail for the guest. */

        RTListNodeRemove(&pCmd->node);

        /*
         * Re-read the request from the guest memory.
         * It will be used to:
         *   * reconstruct commands if legacy saved state has been restored;
         *   * report an error to the guest if resubmit failed.
         */
        VMMDevHGCMRequestHeader *pReqHdr = (VMMDevHGCMRequestHeader *)RTMemAlloc(pCmd->cbRequest);
        AssertBreakStmt(pReqHdr, vmmdevHGCMCmdFree(pCmd); rcFunc = VERR_NO_MEMORY);

        PDMDevHlpPhysRead(pThis->pDevIns, pCmd->GCPhys, pReqHdr, pCmd->cbRequest);
        RT_UNTRUSTED_NONVOLATILE_COPY_FENCE();

        if (pThis->pHGCMDrv)
        {
            /*
             * Reconstruct legacy commands.
             */
            if (RT_LIKELY(pThis->u32SSMVersion >= VMMDEV_SAVED_STATE_VERSION_HGCM_PARAMS))
            { /* likely */ }
            else
            {
                PVBOXHGCMCMD pRestoredCmd = NULL;
                rcCmd = vmmdevHGCMRestoreCommand(pThis, pThis->u32SSMVersion, pCmd,
                                                 pReqHdr, pCmd->cbRequest, &pRestoredCmd);
                if (RT_SUCCESS(rcCmd))
                {
                    Assert(pCmd != pRestoredCmd); /* vmmdevHGCMRestoreCommand must allocate restored command. */
                    vmmdevHGCMCmdFree(pCmd);
                    pCmd = pRestoredCmd;
                }
            }

            /* Resubmit commands. */
            if (RT_SUCCESS(rcCmd))
            {
                switch (pCmd->enmCmdType)
                {
                    case VBOXHGCMCMDTYPE_CONNECT:
                    {
                        vmmdevHGCMAddCommand(pThis, pCmd);
                        rcCmd = pThis->pHGCMDrv->pfnConnect(pThis->pHGCMDrv, pCmd, &pCmd->u.connect.loc, &pCmd->u.connect.u32ClientID);
                        if (RT_FAILURE(rcCmd))
                            vmmdevHGCMRemoveCommand(pThis, pCmd);
                        break;
                    }

                    case VBOXHGCMCMDTYPE_DISCONNECT:
                    {
                        vmmdevHGCMAddCommand(pThis, pCmd);
                        rcCmd = pThis->pHGCMDrv->pfnDisconnect(pThis->pHGCMDrv, pCmd, pCmd->u.disconnect.u32ClientID);
                        if (RT_FAILURE(rcCmd))
                            vmmdevHGCMRemoveCommand(pThis, pCmd);
                        break;
                    }

                    case VBOXHGCMCMDTYPE_CALL:
                    {
                        rcCmd = vmmdevHGCMInitHostParameters(pThis, pCmd);
                        if (RT_SUCCESS(rcCmd))
                        {
                            vmmdevHGCMAddCommand(pThis, pCmd);

                            /* Pass the function call to HGCM connector for actual processing */
                            rcCmd = pThis->pHGCMDrv->pfnCall(pThis->pHGCMDrv, pCmd,
                                                             pCmd->u.call.u32ClientID, pCmd->u.call.u32Function,
                                                             pCmd->u.call.cParms, pCmd->u.call.paHostParms);
                            if (RT_FAILURE(rcCmd))
                            {
                                LogFunc(("pfnCall rc = %Rrc\n", rcCmd));
                                vmmdevHGCMRemoveCommand(pThis, pCmd);
                            }
                        }
                        break;
                    }

                    default:
                        AssertFailedStmt(rcCmd = VERR_INTERNAL_ERROR);
                }
            }
        }
        else
            AssertFailedStmt(rcCmd = VERR_INTERNAL_ERROR);

        if (RT_SUCCESS(rcCmd))
        { /* likely */ }
        else
        {
            /* Return the error to the guest. Guest may try to repeat the call. */
            pReqHdr->result = rcCmd;
            pReqHdr->header.rc = rcCmd;
            pReqHdr->fu32Flags |= VBOX_HGCM_REQ_DONE;

            /* Write back only the header. */
            PDMDevHlpPhysWrite(pThis->pDevIns, pCmd->GCPhys, pReqHdr, sizeof(*pReqHdr));

            VMMDevNotifyGuest(pThis, VMMDEV_EVENT_HGCM);

            /* Deallocate the command memory. */
            vmmdevHGCMCmdFree(pCmd);
        }

        RTMemFree(pReqHdr);
    }

    if (RT_FAILURE(rcFunc))
    {
        RTListForEachSafe(&listLoadedCommands, pCmd, pNext, VBOXHGCMCMD, node)
        {
            RTListNodeRemove(&pCmd->node);
            vmmdevHGCMCmdFree(pCmd);
        }
    }

    return rcFunc;
}

/** Deallocate HGCM commands.
 *
 * @param   pThis           The VMMDev instance data.
 */
void vmmdevHGCMDestroy(PVMMDEV pThis)
{
    LogFlowFunc(("\n"));

    PVBOXHGCMCMD pCmd, pNext;
    RTListForEachSafe(&pThis->listHGCMCmd, pCmd, pNext, VBOXHGCMCMD, node)
    {
        vmmdevHGCMRemoveCommand(pThis, pCmd);
        vmmdevHGCMCmdFree(pCmd);
    }
}
