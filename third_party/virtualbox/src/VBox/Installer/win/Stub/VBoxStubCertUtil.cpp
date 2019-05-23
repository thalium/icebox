/* $Id: VBoxStubCertUtil.cpp $ */
/** @file
 * VBoxStub - VirtualBox's Windows installer stub (certificate manipulations).
 *
 * NOTE: The content of this file is partly
 *       grabbed from src/VBox/Additions/WINNT/tools/VBoxCertUtil.cpp
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
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
#include <iprt/win/windows.h>
#include <Wincrypt.h>

#include <iprt/string.h>
#include <iprt/message.h>
#include <iprt/err.h>


/**
 * Reads a certificate from a (const char []) buffer, returning a context
 * or a the handle to a temporary memory store.
 *
 * @returns true on success, false on failure (error message written).
 * @param   kpCertBuf           The pointer to the buffer containing the
 *                              certificates.
 * @param   cbCertBuf           Size of @param kpCertBuf in bytes.
 * @param   ppOutCtx            Where to return the handle to the temporary
 *                              memory store.
 */
static bool readCertBuf(const unsigned char kpCertBuf[], DWORD cbCertBuf, PCCERT_CONTEXT *ppOutCtx)
{
    *ppOutCtx = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                             (PBYTE)kpCertBuf, cbCertBuf);
    if (*ppOutCtx)
        return true;

    return false;
}

/**
 * Opens a certificate store.
 *
 * @returns true on success, false on failure (error message written).
 * @param   dwDst           The destination, like
 *                          CERT_SYSTEM_STORE_LOCAL_MACHINE or
 *                          CERT_SYSTEM_STORE_CURRENT_USER.
 * @param   pszStoreNm      The store name.
 */
static HCERTSTORE openCertStore(DWORD dwDst, const char *pszStoreNm)
{
    HCERTSTORE hStore = NULL;
    PRTUTF16   pwszStoreNm;
    int rc = RTStrToUtf16(pszStoreNm, &pwszStoreNm);
    if (RT_SUCCESS(rc))
    {
        /*
         * Make sure CERT_STORE_OPEN_EXISTING_FLAG is not set. This causes Windows XP
         * to return ACCESS_DENIED when installing TrustedPublisher certificates via
         * CertAddCertificateContextToStore() if the TrustedPublisher store never has
         * been used (through certmgr.exe and friends) yet.
         *
         * According to MSDN, if neither CERT_STORE_OPEN_EXISTING_FLAG nor
         * CERT_STORE_CREATE_NEW_FLAG is set, the store will be either opened or
         * created accordingly.
         */
        dwDst &= ~CERT_STORE_OPEN_EXISTING_FLAG;

        hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                               PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                               NULL /* hCryptProv = default */,
                               dwDst,
                               pwszStoreNm);

        RTUtf16Free(pwszStoreNm);
    }
    return hStore;
}

/**
 * Adds a certificate to a store.
 *
 * @returns true on success, false on failure (error message written).
 * @param   dwDst           The destination, like
 *                          CERT_SYSTEM_STORE_LOCAL_MACHINE or
 *                          CERT_SYSTEM_STORE_CURRENT_USER.
 * @param   pszStoreNm      The store name.
 * @param   kpCertBuf       Buffer that contains a certificate
 * @param   cbCertBuf       Size of @param kpCertBuf in bytes
 */
bool addCertToStore(DWORD dwDst, const char *pszStoreNm, const unsigned char kpCertBuf[], DWORD cbCertBuf)
{
    /*
     * Get certificate from buffer.
     */
    PCCERT_CONTEXT  pSrcCtx = NULL;
    bool            fRc     = false;

    if (!readCertBuf(kpCertBuf, cbCertBuf, &pSrcCtx))
    {
        RTMsgError("Unable to get certificate context: %d", GetLastError());
        return fRc;
    }

    /*
     * Open the certificates store.
     */
    HCERTSTORE hDstStore = openCertStore(dwDst, pszStoreNm);
    if (hDstStore)
    {
        /*
         * Finally, add certificate to store
         */
        if (CertAddCertificateContextToStore(hDstStore, pSrcCtx, CERT_STORE_ADD_REPLACE_EXISTING, NULL))
            fRc = true;
        else
            RTMsgError("Unable to install certificate: %d", GetLastError());

        CertCloseStore(hDstStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    else
        RTMsgError("Unable to open certificates store: %d", GetLastError());

    /* Release resources */
    CertFreeCertificateContext(pSrcCtx);

    return fRc;
}
