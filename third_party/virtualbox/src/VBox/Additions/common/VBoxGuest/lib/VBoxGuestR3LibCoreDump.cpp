/* $Id: VBoxGuestR3LibCoreDump.cpp $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, Core Dumps.
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
#include "VBoxGuestR3LibInternal.h"


/**
 * Write guest core dump.
 *
 * @returns IPRT status code.
 */
VBGLR3DECL(int) VbglR3WriteCoreDump(void)
{
    VBGLIOCWRITECOREDUMP Req;
    VBGLREQHDR_INIT(&Req.Hdr, WRITE_CORE_DUMP);
    Req.u.In.fFlags = 0;
    return vbglR3DoIOCtl(VBGL_IOCTL_WRITE_CORE_DUMP, &Req.Hdr, sizeof(Req));
}

