/* $Id: x509-verify.cpp $ */
/** @file
 * IPRT - Crypto - X.509, Signature verficiation.
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
#include <iprt/crypto/x509.h>
#include <iprt/crypto/pkix.h>

#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/string.h>


RTDECL(int) RTCrX509Certificate_VerifySignature(PCRTCRX509CERTIFICATE pThis, PCRTASN1OBJID pAlgorithm,
                                                PCRTASN1DYNTYPE pParameters, PCRTASN1BITSTRING pPublicKey,
                                                PRTERRINFO pErrInfo)
{
    /*
     * Validate the input a little.
     */
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);
    AssertReturn(RTCrX509Certificate_IsPresent(pThis), VERR_INVALID_PARAMETER);

    AssertPtrReturn(pAlgorithm, VERR_INVALID_POINTER);
    AssertReturn(RTAsn1ObjId_IsPresent(pAlgorithm), VERR_INVALID_POINTER);

    if (pParameters)
    {
        AssertPtrReturn(pParameters, VERR_INVALID_POINTER);
        if (pParameters->enmType == RTASN1TYPE_NULL)
            pParameters = NULL;
    }

    AssertPtrReturn(pPublicKey, VERR_INVALID_POINTER);
    AssertReturn(RTAsn1BitString_IsPresent(pPublicKey), VERR_INVALID_POINTER);

    /*
     * Check if the algorithm matches.
     */
    const char *pszCipherOid = RTCrPkixGetCiperOidFromSignatureAlgorithm(&pThis->SignatureAlgorithm.Algorithm);
    if (!pszCipherOid)
        return RTErrInfoSetF(pErrInfo, VERR_CR_X509_UNKNOWN_CERT_SIGN_ALGO,
                             "Certificate signature algorithm not known: %s",
                             pThis->SignatureAlgorithm.Algorithm.szObjId);

    if (RTAsn1ObjId_CompareWithString(pAlgorithm, pszCipherOid) != 0)
        return RTErrInfoSetF(pErrInfo, VERR_CR_X509_CERT_SIGN_ALGO_MISMATCH,
                             "Certificate signature cipher algorithm mismatch: cert uses %s (%s) while key uses %s",
                             pszCipherOid, pThis->SignatureAlgorithm.Algorithm.szObjId, pAlgorithm->szObjId);

    /*
     * Here we should recode the to-be-signed part as DER, but we'll ASSUME
     * that it's already in DER encoding and only does this if there the
     * encoded bits are missing.
     */
    if (   pThis->TbsCertificate.SeqCore.Asn1Core.uData.pu8
        && pThis->TbsCertificate.SeqCore.Asn1Core.cb > 0)
        return RTCrPkixPubKeyVerifySignature(&pThis->SignatureAlgorithm.Algorithm, pParameters, pPublicKey, &pThis->SignatureValue,
                                             RTASN1CORE_GET_RAW_ASN1_PTR(&pThis->TbsCertificate.SeqCore.Asn1Core),
                                             RTASN1CORE_GET_RAW_ASN1_SIZE(&pThis->TbsCertificate.SeqCore.Asn1Core),
                                             pErrInfo);

    uint32_t cbEncoded;
    int rc = RTAsn1EncodePrepare((PRTASN1CORE)&pThis->TbsCertificate.SeqCore.Asn1Core, RTASN1ENCODE_F_DER, &cbEncoded, pErrInfo);
    if (RT_SUCCESS(rc))
    {
        void *pvTbsBits = RTMemTmpAlloc(cbEncoded);
        if (pvTbsBits)
        {
            rc = RTAsn1EncodeToBuffer(&pThis->TbsCertificate.SeqCore.Asn1Core, RTASN1ENCODE_F_DER,
                                      pvTbsBits, cbEncoded, pErrInfo);
            if (RT_SUCCESS(rc))
                rc = RTCrPkixPubKeyVerifySignature(&pThis->SignatureAlgorithm.Algorithm, pParameters, pPublicKey,
                                                   &pThis->SignatureValue, pvTbsBits, cbEncoded, pErrInfo);
            else
                AssertRC(rc);
            RTMemTmpFree(pvTbsBits);
        }
        else
            rc = VERR_NO_TMP_MEMORY;
    }
    return rc;
}


RTDECL(int) RTCrX509Certificate_VerifySignatureSelfSigned(PCRTCRX509CERTIFICATE pThis, PRTERRINFO pErrInfo)
{
    /*
     * Validate the input a little.
     */
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);
    AssertReturn(RTCrX509Certificate_IsPresent(pThis), VERR_INVALID_PARAMETER);

    /*
     * Assemble parameters for the generic verification call.
     */
    PCRTCRX509TBSCERTIFICATE const pTbsCert    = &pThis->TbsCertificate;
    PCRTASN1DYNTYPE                pParameters = NULL;
    if (   RTASN1CORE_IS_PRESENT(&pTbsCert->SubjectPublicKeyInfo.Algorithm.Parameters.u.Core)
        && pTbsCert->SubjectPublicKeyInfo.Algorithm.Parameters.enmType != RTASN1TYPE_NULL)
        pParameters = &pTbsCert->SubjectPublicKeyInfo.Algorithm.Parameters;
    return RTCrX509Certificate_VerifySignature(pThis, &pTbsCert->SubjectPublicKeyInfo.Algorithm.Algorithm, pParameters,
                                               &pTbsCert->SubjectPublicKeyInfo.SubjectPublicKey, pErrInfo);
}

