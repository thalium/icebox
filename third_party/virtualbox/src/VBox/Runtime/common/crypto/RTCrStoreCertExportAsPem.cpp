/* $Id: RTCrStoreCertExportAsPem.cpp $ */
/** @file
 * IPRT - Cryptographic (Certificate) Store, RTCrStoreCertExportAsPem.
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

#include <iprt/assert.h>
#include <iprt/base64.h>
#include <iprt/dir.h>
#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/stream.h>



RTDECL(int) RTCrStoreCertExportAsPem(RTCRSTORE hStore, uint32_t fFlags, const char *pszFilename)
{
    /*
     * Validate input.
     */
    AssertReturn(!fFlags, VERR_INVALID_FLAGS);

    /*
     * Start the enumeration first as this validates the store handle.
     */
    RTCRSTORECERTSEARCH Search;
    int rc = RTCrStoreCertFindAll(hStore, &Search);
    if (RT_SUCCESS(rc))
    {
        /*
         * Open the file for writing.
         *
         * Note! We must use text and no binary here, because the base-64 API
         *       below will use host specific EOL markers, not CRLF as PEM
         *       specifies.
         */
        PRTSTREAM hStrm;
        rc = RTStrmOpen(pszFilename, "w", &hStrm);
        if (RT_SUCCESS(rc))
        {
            /*
             * Enumerate the certificates in the store, writing them out one by one.
             */
            size_t          cbBase64  = 0;
            char           *pszBase64 = NULL;
            PCRTCRCERTCTX   pCertCtx;
            while ((pCertCtx = RTCrStoreCertSearchNext(hStore, &Search)) != NULL)
            {
                const char *pszMarker;
                switch (pCertCtx->fFlags & RTCRCERTCTX_F_ENC_MASK)
                {
                    case RTCRCERTCTX_F_ENC_X509_DER:    pszMarker = "CERTIFICATE";  break;
                    case RTCRCERTCTX_F_ENC_TAF_DER:     pszMarker = "TRUST ANCHOR"; break;
                    default:                            pszMarker = NULL;           break;
                }
                if (pszMarker && pCertCtx->cbEncoded > 0)
                {
                    /*
                     * Do the base64 conversion first.
                     */
                    size_t cchEncoded = RTBase64EncodedLength(pCertCtx->cbEncoded);
                    if (cchEncoded < cbBase64)
                    { /* likely */ }
                    else
                    {
                        size_t cbNew = RT_ALIGN(cchEncoded + 64, 128);
                        void *pvNew = RTMemRealloc(pszBase64, cbNew);
                        if (!pvNew)
                        {
                            rc = VERR_NO_MEMORY;
                            break;
                        }
                        cbBase64  = cbNew;
                        pszBase64 = (char *)pvNew;
                    }
                    rc = RTBase64Encode(pCertCtx->pabEncoded, pCertCtx->cbEncoded, pszBase64, cbBase64, &cchEncoded);
                    if (RT_FAILURE(rc))
                        break;

                    RTStrmPrintf(hStrm, "-----BEGIN %s-----\n", pszMarker);
                    RTStrmWrite(hStrm, pszBase64, cchEncoded);
                    rc = RTStrmPrintf(hStrm, "\n-----END %s-----\n", pszMarker);
                    if (RT_FAILURE(rc))
                        break;
                }

                RTCrCertCtxRelease(pCertCtx);
            }
            if (pCertCtx)
                RTCrCertCtxRelease(pCertCtx);
            RTMemFree(pszBase64);

            /*
             * Flush the output file before closing.
             */
            int rc2 = RTStrmFlush(hStrm);
            if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                rc = rc2;
            RTStrmClearError(hStrm); /** @todo fix RTStrmClose... */
            rc2 = RTStrmClose(hStrm);
            if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                rc = rc2;
        }

        int rc2 = RTCrStoreCertSearchDestroy(hStore, &Search); AssertRC(rc2);
    }
    return rc;
}
RT_EXPORT_SYMBOL(RTCrStoreCertExportAsPem);

