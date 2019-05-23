/* $Id: iprt-openssl.cpp $ */
/** @file
 * IPRT - Crypto - OpenSSL Helpers.
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

#ifdef IPRT_WITH_OPENSSL    /* Whole file. */
# include <iprt/err.h>
# include <iprt/string.h>

# include "internal/iprt-openssl.h"
# include <openssl/x509.h>
# include <openssl/err.h>


DECLHIDDEN(void) rtCrOpenSslInit(void)
{
    static bool s_fOssInitalized;
    if (!s_fOssInitalized)
    {
        OpenSSL_add_all_algorithms();
        ERR_load_ERR_strings();
        ERR_load_crypto_strings();

        s_fOssInitalized = true;
    }
}


DECLHIDDEN(int) rtCrOpenSslErrInfoCallback(const char *pach, size_t cch, void *pvUser)
{
    PRTERRINFO pErrInfo = (PRTERRINFO)pvUser;
    size_t cchAlready = pErrInfo->fFlags & RTERRINFO_FLAGS_SET ? strlen(pErrInfo->pszMsg) : 0;
    if (cchAlready + 1 < pErrInfo->cbMsg)
        RTStrCopyEx(pErrInfo->pszMsg + cchAlready, pErrInfo->cbMsg - cchAlready, pach, cch);
    return -1;
}


DECLHIDDEN(int) rtCrOpenSslAddX509CertToStack(void *pvOsslStack, PCRTCRX509CERTIFICATE pCert)
{
    int                  rc;
    const unsigned char *pabEncoded = (const unsigned char *)RTASN1CORE_GET_RAW_ASN1_PTR(&pCert->SeqCore.Asn1Core);
    uint32_t             cbEncoded  = RTASN1CORE_GET_RAW_ASN1_SIZE(&pCert->SeqCore.Asn1Core);
    X509                *pOsslCert  = NULL;
    if (d2i_X509(&pOsslCert, &pabEncoded, cbEncoded) == pOsslCert)
    {
        if (sk_X509_push((STACK_OF(X509) *)pvOsslStack, pOsslCert))
            rc = VINF_SUCCESS;
        else
        {
            rc = VERR_NO_MEMORY;
            X509_free(pOsslCert);
        }
    }
    else
        rc = VERR_CR_X509_OSSL_D2I_FAILED;
    return rc;
}

#endif /* IPRT_WITH_OPENSSL */

