/* $Id: handle.cpp $ */
/** @file
 * IPRT - Generic Handle Manipulation.
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
#include <iprt/handle.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/pipe.h>


RTDECL(int) RTHandleClose(PRTHANDLE ph)
{
    int rc = VINF_SUCCESS;
    if (ph)
    {
        switch (ph->enmType)
        {
            case RTHANDLETYPE_FILE:
                rc = RTFileClose(ph->u.hFile);
                ph->u.hFile = NIL_RTFILE;
                break;

            case RTHANDLETYPE_PIPE:
                rc = RTPipeClose(ph->u.hPipe);
                ph->u.hPipe = NIL_RTPIPE;
                break;

            case RTHANDLETYPE_SOCKET:
                AssertMsgFailed(("Socket not supported\n"));
                rc = VERR_NOT_SUPPORTED;
                break;

            case RTHANDLETYPE_THREAD:
                AssertMsgFailed(("Thread not supported\n"));
                rc = VERR_NOT_SUPPORTED;
                break;

            default:
                AssertMsgFailed(("Invalid type %d\n", ph->enmType));
                rc = VERR_INVALID_PARAMETER;
                break;
        }
    }
    return rc;
}

