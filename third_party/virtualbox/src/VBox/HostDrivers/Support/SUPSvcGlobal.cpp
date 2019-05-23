/* $Id: SUPSvcGlobal.cpp $ */
/** @file
 * VirtualBox Support Service - The Global Service.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
#include "SUPSvcInternal.h"

#include <iprt/err.h>
#include <iprt/assert.h>


/** @copydoc SUPSVCSERVICE::pfnCreate */
DECLCALLBACK(int)  supSvcGlobalCreate(void **ppvInstance)
{
    *ppvInstance = (void *)1;
    return VINF_SUCCESS;
}


/** @copydoc SUPSVCSERVICE::pfnStart */
DECLCALLBACK(void) supSvcGlobalStart(void *pvInstance)
{
    Assert(pvInstance == (void *)1);
    NOREF(pvInstance);
}


/** @copydoc SUPSVCSERVICE::pfnTryStop */
DECLCALLBACK(int)  supSvcGlobalTryStop(void *pvInstance)
{
    Assert(pvInstance == (void *)1);
    NOREF(pvInstance);
    return VINF_SUCCESS;
}


/** @copydoc SUPSVCSERVICE::pfnStopAndDestroy */
DECLCALLBACK(void) supSvcGlobalStopAndDestroy(void *pvInstance, bool fRunning)
{
    Assert(pvInstance == (void *)1);
    NOREF(pvInstance);
    NOREF(fRunning);
}

