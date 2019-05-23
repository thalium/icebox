/* $Id: SUPDrv.d $ */
/** @file
 * SUPDrv - Static dtrace probes.
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


provider vboxdrv
{
    probe session__create(struct SUPDRVSESSION *pSession, int fUser);
    probe session__close(struct SUPDRVSESSION *pSession);
    probe ioctl__entry(struct SUPDRVSESSION *pSession, uintptr_t uIOCtl, void *pvReqHdr);
    probe ioctl__return(struct SUPDRVSESSION *pSession, uintptr_t uIOCtl, void *pvReqHdr, int rc, int rcReq);
};

#pragma D attributes Evolving/Evolving/Common provider vboxdrv provider
#pragma D attributes Private/Private/Unknown  provider vboxdrv module
#pragma D attributes Private/Private/Unknown  provider vboxdrv function
#pragma D attributes Evolving/Evolving/Common provider vboxdrv name
#pragma D attributes Evolving/Evolving/Common provider vboxdrv args

