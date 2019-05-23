/* $Id: VBoxGuestR3LibGuestCtrl.cpp $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, guest control.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/assert.h>
#include <iprt/cpp/autores.h>
#include <iprt/stdarg.h>
#include <VBox/log.h>
#include <VBox/HostServices/GuestControlSvc.h>

#include "VBoxGuestR3LibInternal.h"

using namespace guestControl;


/**
 * Connects to the guest control service.
 *
 * @returns VBox status code
 * @param   pidClient     Where to put The client ID on success. The client ID
 *                        must be passed to all the other calls to the service.
 */
VBGLR3DECL(int) VbglR3GuestCtrlConnect(uint32_t *pidClient)
{
    return VbglR3HGCMConnect("VBoxGuestControlSvc", pidClient);
}


/**
 * Disconnect from the guest control service.
 *
 * @returns VBox status code.
 * @param   idClient        The client ID returned by VbglR3GuestCtrlConnect().
 */
VBGLR3DECL(int) VbglR3GuestCtrlDisconnect(uint32_t idClient)
{
    return VbglR3HGCMDisconnect(idClient);
}


/**
 * Waits until a new host message arrives.
 * This will block until a message becomes available.
 *
 * @returns VBox status code.
 * @param   uClientId       The client ID returned by VbglR3GuestCtrlConnect().
 * @param   pidMsg          Where to store the message id.
 * @param   pcParameters    Where to store the number  of parameters which will
 *                          be received in a second call to the host.
 */
VBGLR3DECL(int) VbglR3GuestCtrlMsgWaitFor(uint32_t uClientId, uint32_t *pidMsg, uint32_t *pcParameters)
{
    AssertPtrReturn(pidMsg, VERR_INVALID_POINTER);
    AssertPtrReturn(pcParameters, VERR_INVALID_POINTER);

    HGCMMsgCmdWaitFor Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, uClientId,
                       GUEST_MSG_WAIT,      /* Tell the host we want our next command. */
                       2);                  /* Just peek for the next message! */
    VbglHGCMParmUInt32Set(&Msg.msg, 0);
    VbglHGCMParmUInt32Set(&Msg.num_parms, 0);

    /*
     * We should always get a VERR_TOO_MUCH_DATA response here, see
     * guestControl::HostCommand::Peek() and its caller ClientState::SendReply().
     * We accept success too here, in case someone decide to make the protocol
     * slightly more sane.
     *
     * Note! A really sane protocol design would have a separate call for getting
     *       info about a pending message (returning VINF_SUCCESS), and a separate
     *       one for retriving the actual message parameters.  Not this weird
     *       stuff, to put it rather bluntly.
     */
    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (   rc == VERR_TOO_MUCH_DATA
        || RT_SUCCESS(rc))
    {
        int rc2 = VbglHGCMParmUInt32Get(&Msg.msg, pidMsg);
        if (RT_SUCCESS(rc2))
        {
            rc2 = VbglHGCMParmUInt32Get(&Msg.num_parms, pcParameters);
            if (RT_SUCCESS(rc2))
            {
                /* Ok, so now we know what message type and how much parameters there are. */
                return rc;
            }
        }
        rc = rc2;
    }
    *pidMsg       = UINT32_MAX - 1;
    *pcParameters = UINT32_MAX - 2;
    return rc;
}


/**
 * Asks the host guest control service to set a command filter to this
 * client so that it only will receive certain commands in the future.
 * The filter(s) are a bitmask for the context IDs, served from the host.
 *
 * @return  IPRT status code.
 * @param   uClientId       The client ID returned by VbglR3GuestCtrlConnect().
 * @param   uValue          The value to filter messages for.
 * @param   uMaskAdd        Filter mask to add.
 * @param   uMaskRemove     Filter mask to remove.
 */
VBGLR3DECL(int) VbglR3GuestCtrlMsgFilterSet(uint32_t uClientId, uint32_t uValue,
                                            uint32_t uMaskAdd, uint32_t uMaskRemove)
{
    HGCMMsgCmdFilterSet Msg;

    /* Tell the host we want to set a filter. */
    VBGL_HGCM_HDR_INIT(&Msg.hdr, uClientId, GUEST_MSG_FILTER_SET, 4);
    VbglHGCMParmUInt32Set(&Msg.value, uValue);
    VbglHGCMParmUInt32Set(&Msg.mask_add, uMaskAdd);
    VbglHGCMParmUInt32Set(&Msg.mask_remove, uMaskRemove);
    VbglHGCMParmUInt32Set(&Msg.flags, 0 /* Flags, unused */);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


/**
 * Disables a previously set message filter.
 *
 * @return  IPRT status code.
 * @param   uClientId       The client ID returned by VbglR3GuestCtrlConnect().
 */
VBGLR3DECL(int) VbglR3GuestCtrlMsgFilterUnset(uint32_t uClientId)
{
    /* Tell the host we want to unset the filter. */
    HGCMMsgCmdFilterUnset Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, uClientId, GUEST_MSG_FILTER_UNSET, 1);
    VbglHGCMParmUInt32Set(&Msg.flags, 0 /* Flags, unused */);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


VBGLR3DECL(int) VbglR3GuestCtrlMsgReply(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                        int rc)
{
    return VbglR3GuestCtrlMsgReplyEx(pCtx, rc, 0 /* uType */,
                                     NULL /* pvPayload */, 0 /* cbPayload */);
}


VBGLR3DECL(int) VbglR3GuestCtrlMsgReplyEx(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                          int rc, uint32_t uType,
                                          void *pvPayload, uint32_t cbPayload)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    /* Everything else is optional. */

    HGCMMsgCmdReply Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_REPLY, 4);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.rc, (uint32_t)rc); /* int vs. uint32_t */
    VbglHGCMParmUInt32Set(&Msg.type, uType);
    VbglHGCMParmPtrSet(&Msg.payload, pvPayload, cbPayload);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


/**
 * Tells the host service to skip the current message returned by
 * VbglR3GuestCtrlMsgWaitFor().
 *
 * @return  IPRT status code.
 * @param   uClientId       The client ID returned by VbglR3GuestCtrlConnect().
 */
VBGLR3DECL(int) VbglR3GuestCtrlMsgSkip(uint32_t uClientId)
{
    HGCMMsgCmdSkip Msg;

    /* Tell the host we want to skip the current assigned command. */
    VBGL_HGCM_HDR_INIT(&Msg.hdr, uClientId, GUEST_MSG_SKIP, 1);
    VbglHGCMParmUInt32Set(&Msg.flags, 0 /* Flags, unused */);
    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


/**
 * Asks the host to cancel (release) all pending waits which were deferred.
 *
 * @returns VBox status code.
 * @param   uClientId     The client ID returned by VbglR3GuestCtrlConnect().
 */
VBGLR3DECL(int) VbglR3GuestCtrlCancelPendingWaits(uint32_t uClientId)
{
    HGCMMsgCancelPendingWaits Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, uClientId, GUEST_CANCEL_PENDING_WAITS, 0);
    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


/**
 * Asks a specific guest session to close.
 *
 * @return  IPRT status code.
 * @param   pCtx                    Host context.
 * @param   fFlags                  Some kind of flag. Figure it out yourself.
 ** @todo Docs!
 */
VBGLR3DECL(int) VbglR3GuestCtrlSessionClose(PVBGLR3GUESTCTRLCMDCTX pCtx, uint32_t fFlags)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertReturn(pCtx->uNumParms == 2, VERR_INVALID_PARAMETER);

    HGCMMsgSessionClose Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_SESSION_CLOSE, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.flags, fFlags);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


VBGLR3DECL(int) VbglR3GuestCtrlSessionNotify(PVBGLR3GUESTCTRLCMDCTX pCtx, uint32_t uType, uint32_t uResult)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    HGCMMsgSessionNotify Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_SESSION_NOTIFY, 3);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.type, uType);
    VbglHGCMParmUInt32Set(&Msg.result, uResult);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


/**
 * Retrieves the request to create a new guest session.
 *
 * @return  IPRT status code.
 ** @todo Docs!
 */
VBGLR3DECL(int) VbglR3GuestCtrlSessionGetOpen(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                              uint32_t *puProtocol,
                                              char     *pszUser,     uint32_t  cbUser,
                                              char     *pszPassword, uint32_t  cbPassword,
                                              char     *pszDomain,   uint32_t  cbDomain,
                                              uint32_t *pfFlags,     uint32_t *pidSession)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertReturn(pCtx->uNumParms == 6, VERR_INVALID_PARAMETER);

    AssertPtrReturn(puProtocol, VERR_INVALID_POINTER);
    AssertPtrReturn(pszUser, VERR_INVALID_POINTER);
    AssertPtrReturn(pszPassword, VERR_INVALID_POINTER);
    AssertPtrReturn(pszDomain, VERR_INVALID_POINTER);
    AssertPtrReturn(pfFlags, VERR_INVALID_POINTER);

    HGCMMsgSessionOpen Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.protocol, 0);
    VbglHGCMParmPtrSet(&Msg.username, pszUser, cbUser);
    VbglHGCMParmPtrSet(&Msg.password, pszPassword, cbPassword);
    VbglHGCMParmPtrSet(&Msg.domain, pszDomain, cbDomain);
    VbglHGCMParmUInt32Set(&Msg.flags, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.protocol.GetUInt32(puProtocol);
        Msg.flags.GetUInt32(pfFlags);

        if (pidSession)
            *pidSession = VBOX_GUESTCTRL_CONTEXTID_GET_SESSION(pCtx->uContextID);
    }

    return rc;
}


/**
 * Retrieves the request to terminate an existing guest session.
 *
 * @return  IPRT status code.
 ** @todo Docs!
 */
VBGLR3DECL(int) VbglR3GuestCtrlSessionGetClose(PVBGLR3GUESTCTRLCMDCTX pCtx, uint32_t *pfFlags, uint32_t *pidSession)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertReturn(pCtx->uNumParms == 2, VERR_INVALID_PARAMETER);

    AssertPtrReturn(pfFlags, VERR_INVALID_POINTER);

    HGCMMsgSessionClose Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.flags, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.flags.GetUInt32(pfFlags);

        if (pidSession)
            *pidSession = VBOX_GUESTCTRL_CONTEXTID_GET_SESSION(pCtx->uContextID);
    }

    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlPathGetRename(PVBGLR3GUESTCTRLCMDCTX     pCtx,
                                             char     *pszSource,       uint32_t cbSource,
                                             char     *pszDest,         uint32_t cbDest,
                                             uint32_t *pfFlags)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertReturn(pCtx->uNumParms == 4, VERR_INVALID_PARAMETER);

    AssertPtrReturn(pszSource, VERR_INVALID_POINTER);
    AssertReturn(cbSource, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszDest, VERR_INVALID_POINTER);
    AssertReturn(cbDest, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pfFlags, VERR_INVALID_POINTER);

    HGCMMsgPathRename Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmPtrSet(&Msg.source, pszSource, cbSource);
    VbglHGCMParmPtrSet(&Msg.dest, pszDest, cbDest);
    VbglHGCMParmUInt32Set(&Msg.flags, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.flags.GetUInt32(pfFlags);
    }
    return rc;
}


/**
 * Allocates and gets host data, based on the message id.
 *
 * This will block until data becomes available.
 *
 * @returns VBox status code.
 ** @todo Docs!
 ** @todo Move the parameters in an own struct!
 */
VBGLR3DECL(int) VbglR3GuestCtrlProcGetStart(PVBGLR3GUESTCTRLCMDCTX    pCtx,
                                            char     *pszCmd,         uint32_t  cbCmd,
                                            uint32_t *pfFlags,
                                            char     *pszArgs,        uint32_t  cbArgs,     uint32_t *pcArgs,
                                            char     *pszEnv,         uint32_t *pcbEnv,     uint32_t *pcEnvVars,
                                            char     *pszUser,        uint32_t  cbUser,
                                            char     *pszPassword,    uint32_t  cbPassword,
                                            uint32_t *puTimeoutMS,
                                            uint32_t *puPriority,
                                            uint64_t *puAffinity,     uint32_t  cbAffinity, uint32_t *pcAffinity)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    AssertPtrReturn(pszCmd, VERR_INVALID_POINTER);
    AssertPtrReturn(pfFlags, VERR_INVALID_POINTER);
    AssertPtrReturn(pszArgs, VERR_INVALID_POINTER);
    AssertPtrReturn(pcArgs, VERR_INVALID_POINTER);
    AssertPtrReturn(pszEnv, VERR_INVALID_POINTER);
    AssertPtrReturn(pcbEnv, VERR_INVALID_POINTER);
    AssertPtrReturn(pcEnvVars, VERR_INVALID_POINTER);
    AssertPtrReturn(puTimeoutMS, VERR_INVALID_POINTER);

    HGCMMsgProcExec Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmPtrSet(&Msg.cmd, pszCmd, cbCmd);
    VbglHGCMParmUInt32Set(&Msg.flags, 0);
    VbglHGCMParmUInt32Set(&Msg.num_args, 0);
    VbglHGCMParmPtrSet(&Msg.args, pszArgs, cbArgs);
    VbglHGCMParmUInt32Set(&Msg.num_env, 0);
    VbglHGCMParmUInt32Set(&Msg.cb_env, 0);
    VbglHGCMParmPtrSet(&Msg.env, pszEnv, *pcbEnv);
    if (pCtx->uProtocol < 2)
    {
        AssertPtrReturn(pszUser, VERR_INVALID_POINTER);
        AssertReturn(cbUser, VERR_INVALID_PARAMETER);
        AssertPtrReturn(pszPassword, VERR_INVALID_POINTER);
        AssertReturn(pszPassword, VERR_INVALID_PARAMETER);

        VbglHGCMParmPtrSet(&Msg.u.v1.username, pszUser, cbUser);
        VbglHGCMParmPtrSet(&Msg.u.v1.password, pszPassword, cbPassword);
        VbglHGCMParmUInt32Set(&Msg.u.v1.timeout, 0);
    }
    else
    {
        AssertPtrReturn(puAffinity, VERR_INVALID_POINTER);
        AssertReturn(cbAffinity, VERR_INVALID_PARAMETER);

        VbglHGCMParmUInt32Set(&Msg.u.v2.timeout, 0);
        VbglHGCMParmUInt32Set(&Msg.u.v2.priority, 0);
        VbglHGCMParmUInt32Set(&Msg.u.v2.num_affinity, 0);
        VbglHGCMParmPtrSet(&Msg.u.v2.affinity, puAffinity, cbAffinity);
    }

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.flags.GetUInt32(pfFlags);
        Msg.num_args.GetUInt32(pcArgs);
        Msg.num_env.GetUInt32(pcEnvVars);
        Msg.cb_env.GetUInt32(pcbEnv);
        if (pCtx->uProtocol < 2)
        {
            Msg.u.v1.timeout.GetUInt32(puTimeoutMS);
        }
        else
        {
            Msg.u.v2.timeout.GetUInt32(puTimeoutMS);
            Msg.u.v2.priority.GetUInt32(puPriority);
            Msg.u.v2.num_affinity.GetUInt32(pcAffinity);
        }
    }
    return rc;
}


/**
 * Allocates and gets host data, based on the message id.
 *
 * This will block until data becomes available.
 *
 * @returns VBox status code.
 ** @todo Docs!
 */
VBGLR3DECL(int) VbglR3GuestCtrlProcGetOutput(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                             uint32_t *puPID, uint32_t *puHandle, uint32_t *pfFlags)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertReturn(pCtx->uNumParms == 4, VERR_INVALID_PARAMETER);

    AssertPtrReturn(puPID, VERR_INVALID_POINTER);
    AssertPtrReturn(puHandle, VERR_INVALID_POINTER);
    AssertPtrReturn(pfFlags, VERR_INVALID_POINTER);

    HGCMMsgProcOutput Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.pid, 0);
    VbglHGCMParmUInt32Set(&Msg.handle, 0);
    VbglHGCMParmUInt32Set(&Msg.flags, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.pid.GetUInt32(puPID);
        Msg.handle.GetUInt32(puHandle);
        Msg.flags.GetUInt32(pfFlags);
    }
    return rc;
}


/**
 * Retrieves the input data from host which then gets sent to the
 * started process.
 *
 * This will block until data becomes available.
 *
 * @returns VBox status code.
 ** @todo Docs!
 */
VBGLR3DECL(int) VbglR3GuestCtrlProcGetInput(PVBGLR3GUESTCTRLCMDCTX  pCtx,
                                            uint32_t  *puPID,       uint32_t *pfFlags,
                                            void      *pvData,      uint32_t  cbData,
                                            uint32_t  *pcbSize)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertReturn(pCtx->uNumParms == 5, VERR_INVALID_PARAMETER);

    AssertPtrReturn(puPID, VERR_INVALID_POINTER);
    AssertPtrReturn(pfFlags, VERR_INVALID_POINTER);
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertPtrReturn(pcbSize, VERR_INVALID_POINTER);

    HGCMMsgProcInput Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.pid, 0);
    VbglHGCMParmUInt32Set(&Msg.flags, 0);
    VbglHGCMParmPtrSet(&Msg.data, pvData, cbData);
    VbglHGCMParmUInt32Set(&Msg.size, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.pid.GetUInt32(puPID);
        Msg.flags.GetUInt32(pfFlags);
        Msg.size.GetUInt32(pcbSize);
    }
    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlDirGetRemove(PVBGLR3GUESTCTRLCMDCTX     pCtx,
                                            char     *pszPath,         uint32_t cbPath,
                                            uint32_t *pfFlags)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertReturn(pCtx->uNumParms == 3, VERR_INVALID_PARAMETER);

    AssertPtrReturn(pszPath, VERR_INVALID_POINTER);
    AssertReturn(cbPath, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pfFlags, VERR_INVALID_POINTER);

    HGCMMsgDirRemove Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmPtrSet(&Msg.path, pszPath, cbPath);
    VbglHGCMParmUInt32Set(&Msg.flags, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.flags.GetUInt32(pfFlags);
    }
    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlFileGetOpen(PVBGLR3GUESTCTRLCMDCTX      pCtx,
                                           char     *pszFileName,      uint32_t cbFileName,
                                           char     *pszAccess,        uint32_t cbAccess,
                                           char     *pszDisposition,   uint32_t cbDisposition,
                                           char     *pszSharing,       uint32_t cbSharing,
                                           uint32_t *puCreationMode,
                                           uint64_t *poffAt)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);
    AssertReturn(pCtx->uNumParms == 7, VERR_INVALID_PARAMETER);

    AssertPtrReturn(pszFileName, VERR_INVALID_POINTER);
    AssertReturn(cbFileName, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszAccess, VERR_INVALID_POINTER);
    AssertReturn(cbAccess, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszDisposition, VERR_INVALID_POINTER);
    AssertReturn(cbDisposition, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszSharing, VERR_INVALID_POINTER);
    AssertReturn(cbSharing, VERR_INVALID_PARAMETER);
    AssertPtrReturn(puCreationMode, VERR_INVALID_POINTER);
    AssertPtrReturn(poffAt, VERR_INVALID_POINTER);

    HGCMMsgFileOpen Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmPtrSet(&Msg.filename, pszFileName, cbFileName);
    VbglHGCMParmPtrSet(&Msg.openmode, pszAccess, cbAccess);
    VbglHGCMParmPtrSet(&Msg.disposition, pszDisposition, cbDisposition);
    VbglHGCMParmPtrSet(&Msg.sharing, pszSharing, cbSharing);
    VbglHGCMParmUInt32Set(&Msg.creationmode, 0);
    VbglHGCMParmUInt64Set(&Msg.offset, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.creationmode.GetUInt32(puCreationMode);
        Msg.offset.GetUInt64(poffAt);
    }
    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlFileGetClose(PVBGLR3GUESTCTRLCMDCTX pCtx, uint32_t *puHandle)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    AssertReturn(pCtx->uNumParms == 2, VERR_INVALID_PARAMETER);
    AssertPtrReturn(puHandle, VERR_INVALID_POINTER);

    HGCMMsgFileClose Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.handle, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.handle.GetUInt32(puHandle);
    }
    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlFileGetRead(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                           uint32_t *puHandle, uint32_t *puToRead)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    AssertReturn(pCtx->uNumParms == 3, VERR_INVALID_PARAMETER);
    AssertPtrReturn(puHandle, VERR_INVALID_POINTER);
    AssertPtrReturn(puToRead, VERR_INVALID_POINTER);

    HGCMMsgFileRead Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.handle, 0);
    VbglHGCMParmUInt32Set(&Msg.size, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.handle.GetUInt32(puHandle);
        Msg.size.GetUInt32(puToRead);
    }
    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlFileGetReadAt(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                             uint32_t *puHandle, uint32_t *puToRead, uint64_t *poffAt)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    AssertReturn(pCtx->uNumParms == 4, VERR_INVALID_PARAMETER);
    AssertPtrReturn(puHandle, VERR_INVALID_POINTER);
    AssertPtrReturn(puToRead, VERR_INVALID_POINTER);

    HGCMMsgFileReadAt Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.handle, 0);
    VbglHGCMParmUInt32Set(&Msg.offset, 0);
    VbglHGCMParmUInt32Set(&Msg.size, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.handle.GetUInt32(puHandle);
        Msg.offset.GetUInt64(poffAt);
        Msg.size.GetUInt32(puToRead);
    }
    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlFileGetWrite(PVBGLR3GUESTCTRLCMDCTX pCtx, uint32_t *puHandle,
                                            void *pvData, uint32_t cbData, uint32_t *pcbSize)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    AssertReturn(pCtx->uNumParms == 4, VERR_INVALID_PARAMETER);
    AssertPtrReturn(puHandle, VERR_INVALID_POINTER);
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcbSize, VERR_INVALID_POINTER);

    HGCMMsgFileWrite Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.handle, 0);
    VbglHGCMParmPtrSet(&Msg.data, pvData, cbData);
    VbglHGCMParmUInt32Set(&Msg.size, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.handle.GetUInt32(puHandle);
        Msg.size.GetUInt32(pcbSize);
    }
    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlFileGetWriteAt(PVBGLR3GUESTCTRLCMDCTX pCtx, uint32_t *puHandle,
                                              void *pvData, uint32_t cbData, uint32_t *pcbSize, uint64_t *poffAt)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    AssertReturn(pCtx->uNumParms == 5, VERR_INVALID_PARAMETER);
    AssertPtrReturn(puHandle, VERR_INVALID_POINTER);
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcbSize, VERR_INVALID_POINTER);

    HGCMMsgFileWriteAt Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.handle, 0);
    VbglHGCMParmPtrSet(&Msg.data, pvData, cbData);
    VbglHGCMParmUInt32Set(&Msg.size, 0);
    VbglHGCMParmUInt32Set(&Msg.offset, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.handle.GetUInt32(puHandle);
        Msg.size.GetUInt32(pcbSize);
        Msg.offset.GetUInt64(poffAt);
    }
    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlFileGetSeek(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                           uint32_t *puHandle, uint32_t *puSeekMethod, uint64_t *poffAt)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    AssertReturn(pCtx->uNumParms == 4, VERR_INVALID_PARAMETER);
    AssertPtrReturn(puHandle, VERR_INVALID_POINTER);
    AssertPtrReturn(puSeekMethod, VERR_INVALID_POINTER);
    AssertPtrReturn(poffAt, VERR_INVALID_POINTER);

    HGCMMsgFileSeek Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.handle, 0);
    VbglHGCMParmUInt32Set(&Msg.method, 0);
    VbglHGCMParmUInt64Set(&Msg.offset, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.handle.GetUInt32(puHandle);
        Msg.method.GetUInt32(puSeekMethod);
        Msg.offset.GetUInt64(poffAt);
    }
    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlFileGetTell(PVBGLR3GUESTCTRLCMDCTX pCtx, uint32_t *puHandle)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    AssertReturn(pCtx->uNumParms == 2, VERR_INVALID_PARAMETER);
    AssertPtrReturn(puHandle, VERR_INVALID_POINTER);

    HGCMMsgFileTell Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.handle, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.handle.GetUInt32(puHandle);
    }
    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlProcGetTerminate(PVBGLR3GUESTCTRLCMDCTX pCtx, uint32_t *puPID)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    AssertReturn(pCtx->uNumParms == 2, VERR_INVALID_PARAMETER);
    AssertPtrReturn(puPID, VERR_INVALID_POINTER);

    HGCMMsgProcTerminate Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.pid, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.pid.GetUInt32(puPID);
    }
    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlProcGetWaitFor(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                              uint32_t *puPID, uint32_t *puWaitFlags, uint32_t *puTimeoutMS)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    AssertReturn(pCtx->uNumParms == 5, VERR_INVALID_PARAMETER);
    AssertPtrReturn(puPID, VERR_INVALID_POINTER);

    HGCMMsgProcWaitFor Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_MSG_WAIT, pCtx->uNumParms);
    VbglHGCMParmUInt32Set(&Msg.context, 0);
    VbglHGCMParmUInt32Set(&Msg.pid, 0);
    VbglHGCMParmUInt32Set(&Msg.flags, 0);
    VbglHGCMParmUInt32Set(&Msg.timeout, 0);

    int rc = VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
    if (RT_SUCCESS(rc))
    {
        Msg.context.GetUInt32(&pCtx->uContextID);
        Msg.pid.GetUInt32(puPID);
        Msg.flags.GetUInt32(puWaitFlags);
        Msg.timeout.GetUInt32(puTimeoutMS);
    }
    return rc;
}


VBGLR3DECL(int) VbglR3GuestCtrlFileCbOpen(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                          uint32_t uRc, uint32_t uFileHandle)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    HGCMReplyFileNotify Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_FILE_NOTIFY, 4);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.type, GUEST_FILE_NOTIFYTYPE_OPEN);
    VbglHGCMParmUInt32Set(&Msg.rc, uRc);
    VbglHGCMParmUInt32Set(&Msg.u.open.handle, uFileHandle);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


VBGLR3DECL(int) VbglR3GuestCtrlFileCbClose(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                           uint32_t uRc)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    HGCMReplyFileNotify Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_FILE_NOTIFY, 3);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.type, GUEST_FILE_NOTIFYTYPE_CLOSE);
    VbglHGCMParmUInt32Set(&Msg.rc, uRc);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


VBGLR3DECL(int) VbglR3GuestCtrlFileCbError(PVBGLR3GUESTCTRLCMDCTX pCtx, uint32_t uRc)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    HGCMReplyFileNotify Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_FILE_NOTIFY, 3);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.type, GUEST_FILE_NOTIFYTYPE_ERROR);
    VbglHGCMParmUInt32Set(&Msg.rc, uRc);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


VBGLR3DECL(int) VbglR3GuestCtrlFileCbRead(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                          uint32_t uRc,
                                          void *pvData, uint32_t cbData)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    HGCMReplyFileNotify Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_FILE_NOTIFY, 4);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.type, GUEST_FILE_NOTIFYTYPE_READ);
    VbglHGCMParmUInt32Set(&Msg.rc, uRc);
    VbglHGCMParmPtrSet(&Msg.u.read.data, pvData, cbData);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


VBGLR3DECL(int) VbglR3GuestCtrlFileCbWrite(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                           uint32_t uRc, uint32_t uWritten)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    HGCMReplyFileNotify Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_FILE_NOTIFY, 4);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.type, GUEST_FILE_NOTIFYTYPE_WRITE);
    VbglHGCMParmUInt32Set(&Msg.rc, uRc);
    VbglHGCMParmUInt32Set(&Msg.u.write.written, uWritten);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


VBGLR3DECL(int) VbglR3GuestCtrlFileCbSeek(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                          uint32_t uRc, uint64_t uOffActual)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    HGCMReplyFileNotify Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_FILE_NOTIFY, 4);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.type, GUEST_FILE_NOTIFYTYPE_SEEK);
    VbglHGCMParmUInt32Set(&Msg.rc, uRc);
    VbglHGCMParmUInt64Set(&Msg.u.seek.offset, uOffActual);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


VBGLR3DECL(int) VbglR3GuestCtrlFileCbTell(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                          uint32_t uRc, uint64_t uOffActual)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    HGCMReplyFileNotify Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_FILE_NOTIFY, 4);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.type, GUEST_FILE_NOTIFYTYPE_TELL);
    VbglHGCMParmUInt32Set(&Msg.rc, uRc);
    VbglHGCMParmUInt64Set(&Msg.u.tell.offset, uOffActual);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


/**
 * Callback for reporting a guest process status (along with some other stuff) to the host.
 *
 * @returns VBox status code.
 ** @todo Docs!
 */
VBGLR3DECL(int) VbglR3GuestCtrlProcCbStatus(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                            uint32_t uPID, uint32_t uStatus, uint32_t fFlags,
                                            void  *pvData, uint32_t cbData)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    HGCMMsgProcStatus Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_EXEC_STATUS, 5);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.pid, uPID);
    VbglHGCMParmUInt32Set(&Msg.status, uStatus);
    VbglHGCMParmUInt32Set(&Msg.flags, fFlags);
    VbglHGCMParmPtrSet(&Msg.data, pvData, cbData);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


/**
 * Sends output (from stdout/stderr) from a running process.
 *
 * @returns VBox status code.
 ** @todo Docs!
 */
VBGLR3DECL(int) VbglR3GuestCtrlProcCbOutput(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                            uint32_t uPID,uint32_t uHandle, uint32_t fFlags,
                                            void *pvData, uint32_t cbData)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    HGCMMsgProcOutput Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_EXEC_OUTPUT, 5);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.pid, uPID);
    VbglHGCMParmUInt32Set(&Msg.handle, uHandle);
    VbglHGCMParmUInt32Set(&Msg.flags, fFlags);
    VbglHGCMParmPtrSet(&Msg.data, pvData, cbData);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}


/**
 * Callback for reporting back the input status of a guest process to the host.
 *
 * @returns VBox status code.
 ** @todo Docs!
 */
VBGLR3DECL(int) VbglR3GuestCtrlProcCbStatusInput(PVBGLR3GUESTCTRLCMDCTX pCtx,
                                                 uint32_t uPID, uint32_t uStatus,
                                                 uint32_t fFlags, uint32_t cbWritten)
{
    AssertPtrReturn(pCtx, VERR_INVALID_POINTER);

    HGCMMsgProcStatusInput Msg;
    VBGL_HGCM_HDR_INIT(&Msg.hdr, pCtx->uClientID, GUEST_EXEC_INPUT_STATUS, 5);
    VbglHGCMParmUInt32Set(&Msg.context, pCtx->uContextID);
    VbglHGCMParmUInt32Set(&Msg.pid, uPID);
    VbglHGCMParmUInt32Set(&Msg.status, uStatus);
    VbglHGCMParmUInt32Set(&Msg.flags, fFlags);
    VbglHGCMParmUInt32Set(&Msg.written, cbWritten);

    return VbglR3HGCMCall(&Msg.hdr, sizeof(Msg));
}

