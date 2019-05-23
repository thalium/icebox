/* $Id: VBoxCertUtil.cpp $ */
/** @file
 * VBoxCertUtil - VBox Certificate Utility - Windows Only.
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

#include <iprt/buildconfig.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/message.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/time.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** The verbosity level. */
static unsigned  g_cVerbosityLevel = 1;


static const char *errorToString(DWORD dwErr)
{
    switch (dwErr)
    {
#define MY_CASE(a_uConst)       case a_uConst: return #a_uConst;
        MY_CASE(CRYPT_E_MSG_ERROR);
        MY_CASE(CRYPT_E_UNKNOWN_ALGO);
        MY_CASE(CRYPT_E_OID_FORMAT);
        MY_CASE(CRYPT_E_INVALID_MSG_TYPE);
        MY_CASE(CRYPT_E_UNEXPECTED_ENCODING);
        MY_CASE(CRYPT_E_AUTH_ATTR_MISSING);
        MY_CASE(CRYPT_E_HASH_VALUE);
        MY_CASE(CRYPT_E_INVALID_INDEX);
        MY_CASE(CRYPT_E_ALREADY_DECRYPTED);
        MY_CASE(CRYPT_E_NOT_DECRYPTED);
        MY_CASE(CRYPT_E_RECIPIENT_NOT_FOUND);
        MY_CASE(CRYPT_E_CONTROL_TYPE);
        MY_CASE(CRYPT_E_ISSUER_SERIALNUMBER);
        MY_CASE(CRYPT_E_SIGNER_NOT_FOUND);
        MY_CASE(CRYPT_E_ATTRIBUTES_MISSING);
        MY_CASE(CRYPT_E_STREAM_MSG_NOT_READY);
        MY_CASE(CRYPT_E_STREAM_INSUFFICIENT_DATA);
        MY_CASE(CRYPT_I_NEW_PROTECTION_REQUIRED);
        MY_CASE(CRYPT_E_BAD_LEN);
        MY_CASE(CRYPT_E_BAD_ENCODE);
        MY_CASE(CRYPT_E_FILE_ERROR);
        MY_CASE(CRYPT_E_NOT_FOUND);
        MY_CASE(CRYPT_E_EXISTS);
        MY_CASE(CRYPT_E_NO_PROVIDER);
        MY_CASE(CRYPT_E_SELF_SIGNED);
        MY_CASE(CRYPT_E_DELETED_PREV);
        MY_CASE(CRYPT_E_NO_MATCH);
        MY_CASE(CRYPT_E_UNEXPECTED_MSG_TYPE);
        MY_CASE(CRYPT_E_NO_KEY_PROPERTY);
        MY_CASE(CRYPT_E_NO_DECRYPT_CERT);
        MY_CASE(CRYPT_E_BAD_MSG);
        MY_CASE(CRYPT_E_NO_SIGNER);
        MY_CASE(CRYPT_E_PENDING_CLOSE);
        MY_CASE(CRYPT_E_REVOKED);
        MY_CASE(CRYPT_E_NO_REVOCATION_DLL);
        MY_CASE(CRYPT_E_NO_REVOCATION_CHECK);
        MY_CASE(CRYPT_E_REVOCATION_OFFLINE);
        MY_CASE(CRYPT_E_NOT_IN_REVOCATION_DATABASE);
        MY_CASE(CRYPT_E_INVALID_NUMERIC_STRING);
        MY_CASE(CRYPT_E_INVALID_PRINTABLE_STRING);
        MY_CASE(CRYPT_E_INVALID_IA5_STRING);
        MY_CASE(CRYPT_E_INVALID_X500_STRING);
        MY_CASE(CRYPT_E_NOT_CHAR_STRING);
        MY_CASE(CRYPT_E_FILERESIZED);
        MY_CASE(CRYPT_E_SECURITY_SETTINGS);
        MY_CASE(CRYPT_E_NO_VERIFY_USAGE_DLL);
        MY_CASE(CRYPT_E_NO_VERIFY_USAGE_CHECK);
        MY_CASE(CRYPT_E_VERIFY_USAGE_OFFLINE);
        MY_CASE(CRYPT_E_NOT_IN_CTL);
        MY_CASE(CRYPT_E_NO_TRUSTED_SIGNER);
        MY_CASE(CRYPT_E_MISSING_PUBKEY_PARA);
        MY_CASE(CRYPT_E_OSS_ERROR);
        default:
        {
            PCRTCOMERRMSG pWinComMsg = RTErrCOMGet(dwErr);
            if (pWinComMsg)
                return pWinComMsg->pszDefine;

            static char s_szErr[32];
            RTStrPrintf(s_szErr, sizeof(s_szErr), "%#x (%d)", dwErr, dwErr);
            return s_szErr;
        }
    }
}

#if 0 /* hacking */
static RTEXITCODE addToStore(const char *pszFilename, PCRTUTF16 pwszStore)
{
    /*
     * Open the source.
     */
    void   *pvFile;
    size_t  cbFile;
    int rc = RTFileReadAll(pszFilename, &pvFile, &cbFile);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTFileReadAll failed on '%s': %Rrc", pszFilename, rc);

    RTEXITCODE rcExit = RTEXITCODE_FAILURE;

    PCCERT_CONTEXT pCertCtx = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                           (PBYTE)pvFile,
                                                           (DWORD)cbFile);
    if (pCertCtx)
    {
        /*
         * Open the destination.
         */
        HCERTSTORE hDstStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                             PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                             NULL /* hCryptProv = default */,
                                             /*CERT_SYSTEM_STORE_LOCAL_MACHINE*/ CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG,
                                             pwszStore);
        if (hDstStore != NULL)
        {
#if 0
            DWORD dwContextType;
            if (CertAddSerializedElementToStore(hDstStore,
                                                pCertCtx->pbCertEncoded,
                                                pCertCtx->cbCertEncoded,
                                                CERT_STORE_ADD_NEW,
                                                0 /* dwFlags (reserved) */,
                                                CERT_STORE_ALL_CONTEXT_FLAG,
                                                &dwContextType,
                                                NULL))
            {
                RTMsgInfo("Successfully added '%s' to the '%ls' store (ctx type %u)", pszFilename, pwszStore, dwContextType);
                rcExit = RTEXITCODE_SUCCESS;
            }
            else
                RTMsgError("CertAddSerializedElementToStore returned %s", errorToString(GetLastError()));
#else
            if (CertAddCertificateContextToStore(hDstStore, pCertCtx, CERT_STORE_ADD_NEW, NULL))
            {
                RTMsgInfo("Successfully added '%s' to the '%ls' store", pszFilename, pwszStore);
                rcExit = RTEXITCODE_SUCCESS;
            }
            else
                RTMsgError("CertAddCertificateContextToStore returned %s", errorToString(GetLastError()));
#endif

            CertCloseStore(hDstStore, CERT_CLOSE_STORE_CHECK_FLAG);
        }
        else
            RTMsgError("CertOpenStore returned %s", errorToString(GetLastError()));
        CertFreeCertificateContext(pCertCtx);
    }
    else
        RTMsgError("CertCreateCertificateContext returned %s", errorToString(GetLastError()));
    RTFileReadAllFree(pvFile, cbFile);
    return rcExit;

#if 0

    CRYPT_DATA_BLOB Blob;
    Blob.cbData = (DWORD)cbData;
    Blob.pbData = (PBYTE)pvData;
    HCERTSTORE hSrcStore = PFXImportCertStore(&Blob, L"", )

#endif
}
#endif /* hacking */


/**
 * Reads a certificate from a file, returning a context or a the handle to a
 * temporary memory store.
 *
 * @returns true on success, false on failure (error message written).
 * @param   pszCertFile         The name of the file containing the
 *                              certificates.
 * @param   ppOutCtx            Where to return the certificate context.
 * @param   phSrcStore          Where to return the handle to the temporary
 *                              memory store.
 */
static bool readCertFile(const char *pszCertFile, PCCERT_CONTEXT *ppOutCtx, HCERTSTORE *phSrcStore)
{
    *ppOutCtx   = NULL;
    *phSrcStore = NULL;

    bool    fRc = false;
    void   *pvFile;
    size_t  cbFile;
    int rc = RTFileReadAll(pszCertFile, &pvFile, &cbFile);
    if (RT_SUCCESS(rc))
    {
        *ppOutCtx = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                 (PBYTE)pvFile, (DWORD)cbFile);
        if (*ppOutCtx)
            fRc = true;
        else
        {
            /** @todo figure out if it's some other format... */
            RTMsgError("CertCreateCertificateContext returned %s parsing the content of '%s'",
                       errorToString(GetLastError()), pszCertFile);
        }
        RTFileReadAllFree(pvFile, cbFile);
    }
    else
        RTMsgError("RTFileReadAll failed on '%s': %Rrc", pszCertFile, rc);
    return fRc;
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
        if (g_cVerbosityLevel > 1)
            RTMsgInfo("Opening store %#x:'%s'", dwDst, pszStoreNm);

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
        if (hStore == NULL)
            RTMsgError("CertOpenStore failed opening %#x:'%s': %s",
                       dwDst, pszStoreNm, errorToString(GetLastError()));

        RTUtf16Free(pwszStoreNm);
    }
    return hStore;
}

/**
 * Removes a certificate, given by file, from a store
 *
 * @returns true on success, false on failure (error message written).
 * @param   dwDst           The destination, like
 *                          CERT_SYSTEM_STORE_LOCAL_MACHINE or
 *                          CERT_SYSTEM_STORE_CURRENT_USER.
 * @param   pszStoreNm      The store name.
 * @param   pszCertFile     The file containing the certificate to add.
 */
static bool removeCertFromStoreByFile(DWORD dwDst, const char *pszStoreNm, const char *pszCertFile)
{
    /*
     * Read the certificate file first.
     */
    PCCERT_CONTEXT  pSrcCtx   = NULL;
    HCERTSTORE      hSrcStore = NULL;
    if (!readCertFile(pszCertFile, &pSrcCtx, &hSrcStore))
        return false;

    WCHAR wszName[1024];
    if (!CertGetNameStringW(pSrcCtx, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0 /*dwFlags*/, NULL /*pvTypePara*/,
                            wszName, sizeof(wszName)))
    {
        RTMsgError("CertGetNameStringW(Subject) failed: %s\n", errorToString(GetLastError()));
        wszName[0] = '\0';
    }

    /*
     * Open the destination store.
     */
    bool fRc = false;
    HCERTSTORE hDstStore = openCertStore(dwDst, pszStoreNm);
    if (hDstStore)
    {
        if (pSrcCtx)
        {
            fRc = true;
            unsigned        cDeleted = 0;
            PCCERT_CONTEXT  pCurCtx  = NULL;
            while ((pCurCtx = CertEnumCertificatesInStore(hDstStore, pCurCtx)) != NULL)
            {
                if (CertCompareCertificate(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, pCurCtx->pCertInfo, pSrcCtx->pCertInfo))
                {
                    if (g_cVerbosityLevel > 1)
                        RTMsgInfo("Removing '%ls'...", wszName);
                    PCCERT_CONTEXT pDeleteCtx = CertDuplicateCertificateContext(pCurCtx);
                    if (pDeleteCtx)
                    {
                        if (CertDeleteCertificateFromStore(pDeleteCtx))
                            cDeleted++;
                        else
                            RTMsgError("CertDeleteFromStore('%ls') failed: %s\n", wszName, errorToString(GetLastError()));
                    }
                    else
                        RTMsgError("CertDuplicateCertificateContext('%ls') failed: %s\n", wszName, errorToString(GetLastError()));
                }
            }

            if (!cDeleted)
                RTMsgInfo("Found no matching certificates to remove.");
        }
        else
        {
            RTMsgError("Path not implemented at line %d\n",  __LINE__);
        }

        CertCloseStore(hDstStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (pSrcCtx)
        CertFreeCertificateContext(pSrcCtx);
    if (hSrcStore)
        CertCloseStore(hSrcStore, CERT_CLOSE_STORE_CHECK_FLAG);
    return fRc;
}

/**
 * Adds a certificate to a store.
 *
 * @returns true on success, false on failure (error message written).
 * @param   dwDst           The destination, like
 *                          CERT_SYSTEM_STORE_LOCAL_MACHINE or
 *                          CERT_SYSTEM_STORE_CURRENT_USER.
 * @param   pszStoreNm      The store name.
 * @param   pszCertFile     The file containing the certificate to add.
 * @param   dwDisposition   The disposition towards existing certificates when
 *                          adding it.  CERT_STORE_ADD_NEW is a safe one.
 */
static bool addCertToStore(DWORD dwDst, const char *pszStoreNm, const char *pszCertFile, DWORD dwDisposition)
{
    /*
     * Read the certificate file first.
     */
    PCCERT_CONTEXT  pSrcCtx   = NULL;
    HCERTSTORE      hSrcStore = NULL;
    if (!readCertFile(pszCertFile, &pSrcCtx, &hSrcStore))
        return false;

    /*
     * Open the destination store.
     */
    bool fRc = false;
    HCERTSTORE hDstStore = openCertStore(dwDst, pszStoreNm);
    if (hDstStore)
    {
        if (pSrcCtx)
        {
            if (g_cVerbosityLevel > 1)
                RTMsgInfo("Adding '%s' to %#x:'%s'... (disp %d)", pszCertFile, dwDst, pszStoreNm, dwDisposition);

            if (CertAddCertificateContextToStore(hDstStore, pSrcCtx, dwDisposition, NULL))
                fRc = true;
            else
                RTMsgError("CertAddCertificateContextToStore returned %s", errorToString(GetLastError()));
        }
        else
            RTMsgError("Path not implemented at line %d\n",  __LINE__);

        CertCloseStore(hDstStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (pSrcCtx)
        CertFreeCertificateContext(pSrcCtx);
    if (hSrcStore)
        CertCloseStore(hSrcStore, CERT_CLOSE_STORE_CHECK_FLAG);
    return fRc;
}

/**
 * Worker for cmdDisplayAll.
 */
static BOOL WINAPI displaySystemStoreCallback(const void *pvSystemStore, DWORD dwFlags, PCERT_SYSTEM_STORE_INFO pStoreInfo,
                                              void *pvReserved, void *pvArg)
{
    RT_NOREF(pvArg);
    if (g_cVerbosityLevel > 1)
        RTPrintf("    pvSystemStore=%p dwFlags=%#x pStoreInfo=%p pvReserved=%p\n", pvSystemStore, dwFlags, pStoreInfo, pvReserved);
    LPCWSTR pwszStoreNm = NULL;
    if (dwFlags & CERT_SYSTEM_STORE_RELOCATE_FLAG)
    {
        const CERT_SYSTEM_STORE_RELOCATE_PARA *pRelPara = (const CERT_SYSTEM_STORE_RELOCATE_PARA *)pvSystemStore;
        pwszStoreNm = pRelPara->pwszSystemStore;
        RTPrintf("    %#010x '%ls' hKeyBase=%p\n", dwFlags, pwszStoreNm, pRelPara->hKeyBase);
    }
    else
    {
        pwszStoreNm = (LPCWSTR)pvSystemStore;
        RTPrintf("    %#010x '%ls'\n", dwFlags, pwszStoreNm);
    }

    /*
     * Open the store and list the certificates within.
     */
    DWORD      dwDst  = (dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK);
    HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                      PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                      NULL /* hCryptProv = default */,
                                      dwDst | CERT_STORE_OPEN_EXISTING_FLAG,
                                      pwszStoreNm);
    if (hStore)
    {
        PCCERT_CONTEXT pCertCtx = NULL;
        while ((pCertCtx = CertEnumCertificatesInStore(hStore, pCertCtx)) != NULL)
        {
            if (g_cVerbosityLevel > 1)
                RTPrintf("        pCertCtx=%p dwCertEncodingType=%#x cbCertEncoded=%#x pCertInfo=%p\n",
                         pCertCtx, pCertCtx->dwCertEncodingType, pCertCtx->cbCertEncoded, pCertCtx->pCertInfo);
            WCHAR wszName[1024];
            if (CertGetNameStringW(pCertCtx, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0 /*dwFlags*/, NULL /*pvTypePara*/,
                                   wszName, sizeof(wszName)))
            {
                RTPrintf("        '%ls'\n", wszName);
                if (pCertCtx->pCertInfo)
                {
                    RTTIMESPEC TmpTS;
                    char  szNotBefore[80];
                    RTTimeSpecToString(RTTimeSpecSetNtFileTime(&TmpTS, &pCertCtx->pCertInfo->NotBefore),
                                       szNotBefore, sizeof(szNotBefore));
                    char  szNotAfter[80];
                    RTTimeSpecToString(RTTimeSpecSetNtFileTime(&TmpTS, &pCertCtx->pCertInfo->NotAfter),
                                       szNotAfter, sizeof(szNotAfter));

                    RTPrintf("            NotBefore='%s'\n", szNotBefore);
                    RTPrintf("            NotAfter ='%s'\n", szNotAfter);
                    if (pCertCtx->pCertInfo->Issuer.cbData)
                    {
                        if (CertGetNameStringW(pCertCtx, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL /*pvTypePara*/,
                                               wszName, sizeof(wszName)))
                            RTPrintf("            Issuer='%ls'\n", wszName);
                        else
                            RTMsgError("CertGetNameStringW(Issuer) failed: %s\n", errorToString(GetLastError()));
                    }
                }
            }
            else
                RTMsgError("CertGetNameStringW(Subject) failed: %s\n", errorToString(GetLastError()));

        }

        CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    else
        RTMsgError("CertOpenStore failed opening %#x:'%ls': %s\n", dwDst, pwszStoreNm, errorToString(GetLastError()));

    return TRUE;
}

/**
 * Worker for cmdDisplayAll.
 */
static BOOL WINAPI displaySystemStoreLocation(LPCWSTR pwszStoreLocation, DWORD dwFlags, void *pvReserved, void *pvArg)
{
    NOREF(pvReserved); NOREF(pvArg);
    RTPrintf("System store location: %#010x '%ls'\n", dwFlags, pwszStoreLocation);
    if (!CertEnumSystemStore(dwFlags, NULL, NULL /*pvArg*/, displaySystemStoreCallback))
        RTMsgError("CertEnumSystemStore failed on %#x:'%ls': %s\n",
                   dwFlags, pwszStoreLocation, errorToString(GetLastError()));

    return TRUE;
}

/**
 * Handler for the 'display-all' command.
 */
static RTEXITCODE cmdDisplayAll(int argc, char **argv)
{
    RT_NOREF(argv);
    if (argc != 1)
        return RTMsgErrorExit(RTEXITCODE_SYNTAX, "the display-all command takes no arguments\n");

    if (!CertEnumSystemStoreLocation(0, NULL /*pvArg*/, displaySystemStoreLocation))
        return RTMsgErrorExit(RTEXITCODE_SYNTAX, "CertEnumSystemStoreLocation failed: %s\n", errorToString(GetLastError()));

    return RTEXITCODE_SUCCESS;
}

/**
 * Handler for the 'remove-trusted-publisher' command.
 */
static RTEXITCODE cmdRemoveTrustedPublisher(int argc, char **argv)
{
    /*
     * Parse arguments.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--root",     'r',    RTGETOPT_REQ_STRING },
    };

    const char *pszRootCert    = NULL;
    const char *pszTrustedCert = NULL;

    int             rc;
    RTGETOPTUNION   ValueUnion;
    RTGETOPTSTATE   GetState;
    RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, 0);
    while ((rc = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (rc)
        {
            case 'h':
                RTPrintf("Usage: VBoxCertUtil remove-trusted-publisher [--root <root-cert>] <trusted-cert>\n");
                break;

            case 'V':
                RTPrintf("%sr%d\n", RTBldCfgVersion(), RTBldCfgRevision());
                return RTEXITCODE_SUCCESS;

            case 'r':
                if (pszRootCert)
                    return RTMsgErrorExit(RTEXITCODE_SUCCESS,
                                          "You've already specified '%s' as root certificate.",
                                          pszRootCert);
                pszRootCert = ValueUnion.psz;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (pszTrustedCert)
                    return RTMsgErrorExit(RTEXITCODE_SUCCESS,
                                          "You've already specified '%s' as trusted certificate.",
                                          pszRootCert);
                pszTrustedCert = ValueUnion.psz;
                break;

            default:
                return RTGetOptPrintError(rc, &ValueUnion);
        }
    }
    if (!pszTrustedCert)
        return RTMsgErrorExit(RTEXITCODE_SUCCESS, "No trusted certificate specified.");

    /*
     * Do the job.
     */
    if (   pszRootCert
        && !removeCertFromStoreByFile(CERT_SYSTEM_STORE_LOCAL_MACHINE, "Root", pszRootCert))
        return RTEXITCODE_FAILURE;
    if (!removeCertFromStoreByFile(CERT_SYSTEM_STORE_LOCAL_MACHINE, "TrustedPublisher", pszTrustedCert))
        return RTEXITCODE_FAILURE;

    if (g_cVerbosityLevel > 0)
    {
        if (pszRootCert)
            RTMsgInfo("Successfully removed '%s' as root and '%s' as trusted publisher", pszRootCert, pszTrustedCert);
        else
            RTMsgInfo("Successfully removed '%s' as trusted publisher", pszTrustedCert);
    }
    return RTEXITCODE_SUCCESS;
}


/**
 * Handler for the 'add-trusted-publisher' command.
 */
static RTEXITCODE cmdAddTrustedPublisher(int argc, char **argv)
{
    /*
     * Parse arguments and execute imports as we move along.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--root",     'r',    RTGETOPT_REQ_STRING },
    };

    RTEXITCODE          rcExit = RTEXITCODE_SUCCESS;
    unsigned            cImports = 0;
    RTGETOPTUNION       ValueUnion;
    RTGETOPTSTATE       GetState;
    int rc = RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, 0);
    AssertRC(rc);
    while ((rc = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (rc)
        {
            case 'h':
                RTPrintf("Usage: VBoxCertUtil add-trusted-publisher [--root <root-cert>] <trusted-cert>\n");
                break;

            case 'V':
                RTPrintf("%sr%d\n", RTBldCfgVersion(), RTBldCfgRevision());
                return RTEXITCODE_SUCCESS;

            case 'r':
            case VINF_GETOPT_NOT_OPTION:
            {
                const char * const  pszStoreNm   = rc == 'r' ? "Root" : "TrustedPublisher";
                const char * const  pszStoreDesc = rc == 'r' ? "root" : "trusted publisher";
                PCRTPATHGLOBENTRY   pResultHead;
                rc = RTPathGlob(ValueUnion.psz, RTPATHGLOB_F_NO_DIRS, &pResultHead, NULL);
                if (RT_SUCCESS(rc))
                {
                    for (PCRTPATHGLOBENTRY pCur = pResultHead; pCur; pCur = pCur->pNext)
                    {
                        if (addCertToStore(CERT_SYSTEM_STORE_LOCAL_MACHINE, pszStoreNm, pCur->szPath, CERT_STORE_ADD_NEW))
                            RTMsgInfo("Successfully added '%s' as %s", pCur->szPath, pszStoreDesc);
                        else
                            rcExit = RTEXITCODE_FAILURE;
                        cImports++;
                    }
                    RTPathGlobFree(pResultHead);
                }
                else
                {
                    rcExit = RTMsgErrorExit(RTEXITCODE_SUCCESS, "glob failed on '%s': %Rrc", ValueUnion.psz, rc);
                    cImports++;
                }
                break;
            }

            default:
                return RTGetOptPrintError(rc, &ValueUnion);
        }
    }
    if (cImports == 0)
        return RTMsgErrorExit(RTEXITCODE_SUCCESS, "No trusted or root certificates specified.");
    return rcExit;
}


/**
 * Displays the usage info.
 * @param   argv0               Program name.
 */
static void showUsage(const char *argv0)
{
    RTPrintf("Usage: %Rbn [-v[v]] <command>\n"
             "   or  %Rbn <-V|--version>\n"
             "   or  %Rbn <-h|--help>\n"
             "\n"
             "Available commands:\n"
             "    add-trusted-publisher, remove-trusted-publisher,\n"
             "    display-all\n"
             , argv0, argv0, argv0);
}


int main(int argc, char **argv)
{
    int rc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(rc))
        return RTMsgInitFailure(rc);

    /*
     * Parse arguments up to the command and pass it on to the command handlers.
     */
    typedef enum
    {
        VCUACTION_ADD_TRUSTED_PUBLISHER = 1000,
        VCUACTION_REMOVE_TRUSTED_PUBLISHER,
        VCUACTION_DISPLAY_ALL,
        VCUACTION_END
    } VCUACTION;

    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--verbose",                  'v',                                RTGETOPT_REQ_NOTHING },
        { "--quiet",                    'q',                                RTGETOPT_REQ_NOTHING },
        { "add-trusted-publisher",      VCUACTION_ADD_TRUSTED_PUBLISHER,    RTGETOPT_REQ_NOTHING },
        { "remove-trusted-publisher",   VCUACTION_REMOVE_TRUSTED_PUBLISHER, RTGETOPT_REQ_NOTHING },
        { "display-all",                VCUACTION_DISPLAY_ALL,              RTGETOPT_REQ_NOTHING },
    };

    RTGETOPTUNION   ValueUnion;
    RTGETOPTSTATE   GetState;
    RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, 0);
    while ((rc = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (rc)
        {
            case 'v':
                g_cVerbosityLevel++;
                break;

            case 'q':
                if (g_cVerbosityLevel > 0)
                    g_cVerbosityLevel--;
                break;

            case 'h':
                showUsage(argv[0]);
                return RTEXITCODE_SUCCESS;

            case 'V':
                RTPrintf("%sr%d\n", RTBldCfgVersion(), RTBldCfgRevision());
                return RTEXITCODE_SUCCESS;

            case VCUACTION_ADD_TRUSTED_PUBLISHER:
                return cmdAddTrustedPublisher(argc - GetState.iNext + 1, argv + GetState.iNext - 1);

            case VCUACTION_REMOVE_TRUSTED_PUBLISHER:
                return cmdRemoveTrustedPublisher(argc - GetState.iNext + 1, argv + GetState.iNext - 1);

            case VCUACTION_DISPLAY_ALL:
                return cmdDisplayAll(argc - GetState.iNext + 1, argv + GetState.iNext - 1);

            default:
                return RTGetOptPrintError(rc, &ValueUnion);
        }
    }

    RTMsgError("Missing command...");
    showUsage(argv[0]);
    return RTEXITCODE_SYNTAX;
}

