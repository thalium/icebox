/* $Id: pipe.h $ */
/** @file
 * IPRT - Internal RTPipe header.
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

#ifndef ___internal_pipe_h
#define ___internal_pipe_h

#include <iprt/pipe.h>

RT_C_DECLS_BEGIN

/**
 * Internal RTPollSetAdd helper that returns the handle that should be added to
 * the pollset.
 *
 * @returns Valid handle on success, INVALID_HANDLE_VALUE on failure.
 * @param   hPipe               The pipe handle.
 * @param   fEvents             The events we're polling for.
 * @param   phNative            Where to put the primary handle.
 */
int         rtPipePollGetHandle(RTPIPE hPipe, uint32_t fEvents, PRTHCINTPTR phNative);

/**
 * Internal RTPoll helper that polls the pipe handle and, if @a fNoWait is
 * clear, starts whatever actions we've got running during the poll call.
 *
 * @returns 0 if no pending events, actions initiated if @a fNoWait is clear.
 *          Event mask (in @a fEvents) and no actions if the handle is ready
 *          already.
 *          UINT32_MAX (asserted) if the pipe handle is busy in I/O or a
 *          different poll set.
 *
 * @param   hPipe               The pipe handle.
 * @param   hPollSet            The poll set handle (for access checks).
 * @param   fEvents             The events we're polling for.
 * @param   fFinalEntry         Set if this is the final entry for this handle
 *                              in this poll set.  This can be used for dealing
 *                              with duplicate entries.
 * @param   fNoWait             Set if it's a zero-wait poll call.  Clear if
 *                              we'll wait for an event to occur.
 */
uint32_t    rtPipePollStart(RTPIPE hPipe, RTPOLLSET hPollSet, uint32_t fEvents, bool fFinalEntry, bool fNoWait);

/**
 * Called after a WaitForMultipleObjects returned in order to check for pending
 * events and stop whatever actions that rtPipePollStart() initiated.
 *
 * @returns Event mask or 0.
 *
 * @param   hPipe               The pipe handle.
 * @param   fEvents             The events we're polling for.
 * @param   fFinalEntry         Set if this is the final entry for this handle
 *                              in this poll set.  This can be used for dealing
 *                              with duplicate entries.  Only keep in mind that
 *                              this method is called in reverse order, so the
 *                              first call will have this set (when the entire
 *                              set was processed).
 * @param   fHarvestEvents      Set if we should check for pending events.
 */
uint32_t    rtPipePollDone(RTPIPE hPipe, uint32_t fEvents, bool fFinalEntry, bool fHarvestEvents);


/**
 * Fakes basic query info data for RTPipeQueryInfo.
 *
 * @param   pObjInfo            The output structure.
 * @param   enmAddAttr          The extra attribute.
 * @param   fReadPipe           Set if read pipe, clear if write pipe.
 */
DECLINLINE(void) rtPipeFakeQueryInfo(PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr, bool fReadPipe)
{
    RT_ZERO(*pObjInfo);
    if (fReadPipe)
        pObjInfo->Attr.fMode     = RTFS_TYPE_FIFO | RTFS_UNIX_IRUSR | RTFS_DOS_READONLY;
    else
        pObjInfo->Attr.fMode     = RTFS_TYPE_FIFO | RTFS_UNIX_IWUSR;
    pObjInfo->Attr.enmAdditional = enmAddAttr;
    switch (enmAddAttr)
    {
        case RTFSOBJATTRADD_UNIX:
            pObjInfo->Attr.u.Unix.cHardlinks = 1;
            break;
        case RTFSOBJATTRADD_UNIX_OWNER:
            pObjInfo->Attr.u.UnixOwner.uid = NIL_RTUID;
            break;
        case RTFSOBJATTRADD_UNIX_GROUP:
            pObjInfo->Attr.u.UnixGroup.gid = NIL_RTGID;
            break;
        case RTFSOBJATTRADD_EASIZE:
            break;
        case RTFSOBJATTRADD_32BIT_SIZE_HACK:
        case RTFSOBJATTRADD_NOTHING:
            /* shut up gcc. */
            break;
        /* no default, want warnings. */
    }
}


RT_C_DECLS_END

#endif

