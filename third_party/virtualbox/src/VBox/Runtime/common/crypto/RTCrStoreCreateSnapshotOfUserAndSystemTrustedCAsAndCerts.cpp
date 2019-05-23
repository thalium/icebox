/* $Id: RTCrStoreCreateSnapshotOfUserAndSystemTrustedCAsAndCerts.cpp $ */
/** @file
 * IPRT - Cryptographic (Certificate) Store, RTCrStoreCreateSnapshotOfUserAndSystemTrustedCAsAndCerts.
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
#include "internal/iprt.h"
#include <iprt/crypto/store.h>

#include <iprt/err.h>




RTR3DECL(int) RTCrStoreCreateSnapshotOfUserAndSystemTrustedCAsAndCerts(PRTCRSTORE phStore, PRTERRINFO pErrInfo)
{
    /*
     * Start by a snapshot of the user store.
     */
    RTCRSTORE hDstStore;
    int rc = RTCrStoreCreateSnapshotById(&hDstStore, RTCRSTOREID_USER_TRUSTED_CAS_AND_CERTIFICATES, pErrInfo);
    if (RT_SUCCESS(rc))
    {
        /*
         * Snapshot the system store and add that to the user snapshot.  Ignore errors.
         */
        RTCRSTORE hSrcStore;
        rc = RTCrStoreCreateSnapshotById(&hSrcStore, RTCRSTOREID_SYSTEM_TRUSTED_CAS_AND_CERTIFICATES, pErrInfo);
        if (RT_SUCCESS(rc))
        {
            rc = RTCrStoreCertAddFromStore(hDstStore, RTCRCERTCTX_F_ADD_IF_NOT_FOUND | RTCRCERTCTX_F_ADD_CONTINUE_ON_ERROR,
                                           hSrcStore);
            RTCrStoreRelease(hSrcStore);
        }
        if (RT_SUCCESS(rc))
        {
            *phStore = hDstStore;
            return rc;
        }

        /*
         * If we've got something in the store, return anyway despite of the error condition.
         */
        if (rc != VERR_NO_MEMORY && RTCrStoreCertCount(hDstStore) > 0)
        {
            *phStore = hDstStore;
            return -rc;
        }

        RTCrStoreRelease(hDstStore);
    }
    *phStore = NIL_RTCRSTORE;
    return rc;
}

