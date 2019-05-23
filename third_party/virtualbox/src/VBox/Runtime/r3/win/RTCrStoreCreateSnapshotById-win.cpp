/* $Id: RTCrStoreCreateSnapshotById-win.cpp $ */
/** @file
 * IPRT - RTCrStoreCreateSnapshotById, Windows.
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
#include <iprt/crypto/store.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/once.h>
#include <iprt/ldr.h>

#include <iprt/win/windows.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef HCERTSTORE (WINAPI *PFNCERTOPENSTORE)(PCSTR pszStoreProvider, DWORD dwEncodingType, HCRYPTPROV_LEGACY hCryptProv,
                                              DWORD dwFlags, const void *pvParam);
typedef BOOL (WINAPI *PFNCERTCLOSESTORE)(HCERTSTORE hCertStore, DWORD dwFlags);
typedef PCCERT_CONTEXT (WINAPI *PFNCERTENUMCERTIFICATESINSTORE)(HCERTSTORE hCertStore, PCCERT_CONTEXT pPrevCertContext);



static int rtCrStoreAddCertsFromNative(RTCRSTORE hStore, DWORD fStore, PCRTUTF16 pwszStoreName,
                                       PFNCERTOPENSTORE pfnOpenStore, PFNCERTCLOSESTORE pfnCloseStore,
                                       PFNCERTENUMCERTIFICATESINSTORE pfnEnumCerts, int rc, PRTERRINFO pErrInfo)
{
    DWORD fOpenStore = CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG;
    HCERTSTORE hNativeStore = pfnOpenStore(CERT_STORE_PROV_SYSTEM_W, PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                           NULL /* hCryptProv = default */, fStore | fOpenStore, pwszStoreName);
    if (hStore)
    {
        PCCERT_CONTEXT pCurCtx = NULL;
        while ((pCurCtx = pfnEnumCerts(hNativeStore, pCurCtx)) != NULL)
        {
            if (pCurCtx->dwCertEncodingType & X509_ASN_ENCODING)
            {
                RTERRINFOSTATIC StaticErrInfo;
                RTASN1CURSORPRIMARY PrimaryCursor;
                RTAsn1CursorInitPrimary(&PrimaryCursor, pCurCtx->pbCertEncoded, pCurCtx->cbCertEncoded,
                                        RTErrInfoInitStatic(&StaticErrInfo),
                                        &g_RTAsn1DefaultAllocator, RTASN1CURSOR_FLAGS_DER, "CurCtx");
                RTCRX509CERTIFICATE MyCert;
                int rc2 = RTCrX509Certificate_DecodeAsn1(&PrimaryCursor.Cursor, 0, &MyCert, "Cert");
                if (RT_SUCCESS(rc2))
                {
                    rc2 = RTCrStoreCertAddEncoded(hStore, RTCRCERTCTX_F_ENC_X509_DER | RTCRCERTCTX_F_ADD_IF_NOT_FOUND,
                                                  pCurCtx->pbCertEncoded, pCurCtx->cbCertEncoded,
                                                  RTErrInfoInitStatic(&StaticErrInfo));
                    RTCrX509Certificate_Delete(&MyCert);
                }
                if (RT_FAILURE(rc2))
                {
                    if (RTErrInfoIsSet(&StaticErrInfo.Core))
                        RTErrInfoAddF(pErrInfo, -rc2, "  %s", StaticErrInfo.Core.pszMsg);
                    else
                        RTErrInfoAddF(pErrInfo, -rc2, "  %Rrc adding cert", rc2);
                    rc = -rc2;
                }
            }
        }
        pfnCloseStore(hNativeStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    else
    {
        DWORD uLastErr = GetLastError();
        if (uLastErr != ERROR_FILE_NOT_FOUND)
            rc = RTErrInfoAddF(pErrInfo, -RTErrConvertFromWin32(uLastErr),
                               " CertOpenStore(%#x,'%ls') failed: %u", fStore, pwszStoreName);
    }
    return rc;
}



RTDECL(int) RTCrStoreCreateSnapshotById(PRTCRSTORE phStore, RTCRSTOREID enmStoreId, PRTERRINFO pErrInfo)
{
    AssertReturn(enmStoreId > RTCRSTOREID_INVALID && enmStoreId < RTCRSTOREID_END, VERR_INVALID_PARAMETER);

    /*
     * Create an empty in-memory store.
     */
    RTCRSTORE hStore;
    int rc = RTCrStoreCreateInMem(&hStore, 128);
    if (RT_SUCCESS(rc))
    {
        *phStore = hStore;

        /*
         * Resolve the APIs we need to do this job.
         */
        RTLDRMOD hLdrMod;
        int rc2 = RTLdrLoadSystem("crypt32.dll", false /*NoUnload*/, &hLdrMod);
        if (RT_SUCCESS(rc2))
        {
            PFNCERTOPENSTORE                pfnOpenStore  = NULL;
            rc2 = RTLdrGetSymbol(hLdrMod, "CertOpenStore", (void **)&pfnOpenStore);

            PFNCERTCLOSESTORE               pfnCloseStore = NULL;
            if (RT_SUCCESS(rc2))
                rc2 = RTLdrGetSymbol(hLdrMod, "CertCloseStore", (void **)&pfnCloseStore);

            PFNCERTENUMCERTIFICATESINSTORE  pfnEnumCerts  = NULL;
            if (RT_SUCCESS(rc2))
                rc2 = RTLdrGetSymbol(hLdrMod, "CertEnumCertificatesInStore", (void **)&pfnEnumCerts);
            if (RT_SUCCESS(rc2))
            {
                /*
                 * Do the work.
                 */
                switch (enmStoreId)
                {
                    case RTCRSTOREID_USER_TRUSTED_CAS_AND_CERTIFICATES:
                    case RTCRSTOREID_SYSTEM_TRUSTED_CAS_AND_CERTIFICATES:
                    {
                        DWORD fStore = enmStoreId == RTCRSTOREID_USER_TRUSTED_CAS_AND_CERTIFICATES
                                     ? CERT_SYSTEM_STORE_CURRENT_USER : CERT_SYSTEM_STORE_LOCAL_MACHINE;
                        static PCRTUTF16 const s_apwszStores[] =  { L"AuthRoot", L"CA", L"MY", L"Root" };
                        for (uint32_t i = 0; i < RT_ELEMENTS(s_apwszStores); i++)
                            rc = rtCrStoreAddCertsFromNative(hStore, fStore, s_apwszStores[i], pfnOpenStore, pfnCloseStore,
                                                             pfnEnumCerts, rc, pErrInfo);
                        break;
                    }

                    default:
                        AssertFailed(); /* implement me */
                }
            }
            else
                rc = RTErrInfoSetF(pErrInfo, -rc2, "Error resolving crypt32.dll APIs");
            RTLdrClose(hLdrMod);
        }
        else
            rc = RTErrInfoSetF(pErrInfo, -rc2, "Error loading crypt32.dll");
    }
    else
        RTErrInfoSet(pErrInfo, rc, "RTCrStoreCreateInMem failed");
    return rc;
}
RT_EXPORT_SYMBOL(RTCrStoreCreateSnapshotById);

