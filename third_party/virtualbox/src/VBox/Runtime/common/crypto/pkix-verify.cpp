/* $Id: pkix-verify.cpp $ */
/** @file
 * IPRT - Crypto - Public Key Infrastructure API, Verification.
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
#include <iprt/crypto/pkix.h>

#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/crypto/digest.h>

#ifdef IPRT_WITH_OPENSSL
# include "internal/iprt-openssl.h"
# include "openssl/evp.h"
# ifndef OPENSSL_VERSION_NUMBER
#  error "Missing OPENSSL_VERSION_NUMBER!"
# endif
#endif


RTDECL(int) RTCrPkixPubKeyVerifySignature(PCRTASN1OBJID pAlgorithm, PCRTASN1DYNTYPE pParameters, PCRTASN1BITSTRING pPublicKey,
                                          PCRTASN1BITSTRING pSignatureValue, const void *pvData, size_t cbData,
                                          PRTERRINFO pErrInfo)
{
    /*
     * Valid input.
     */
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

    AssertPtrReturn(pSignatureValue, VERR_INVALID_POINTER);
    AssertReturn(RTAsn1BitString_IsPresent(pSignatureValue), VERR_INVALID_POINTER);

    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData > 0, VERR_INVALID_PARAMETER);

    /*
     * Parameters are not currently supported (openssl code path).
     */
    if (pParameters)
        return RTErrInfoSet(pErrInfo, VERR_CR_PKIX_CIPHER_ALGO_PARAMS_NOT_IMPL,
                            "Cipher algorithm parameters are not yet supported.");

    /*
     * Validate using IPRT.
     */
    RTCRPKIXSIGNATURE hSignature;
    int rcIprt = RTCrPkixSignatureCreateByObjId(&hSignature, pAlgorithm, false /*fSigning*/, pPublicKey, pParameters);
    if (RT_FAILURE(rcIprt))
        return RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_CIPHER_ALGO_NOT_KNOWN,
                             "Unknown public key algorithm [IPRT]: %s", pAlgorithm->szObjId);

    RTCRDIGEST hDigest;
    rcIprt = RTCrDigestCreateByObjId(&hDigest, pAlgorithm);
    if (RT_SUCCESS(rcIprt))
    {
        /* Calculate the digest. */
        rcIprt = RTCrDigestUpdate(hDigest, pvData, cbData);
        if (RT_SUCCESS(rcIprt))
        {
            rcIprt = RTCrPkixSignatureVerifyBitString(hSignature, hDigest, pSignatureValue);
            if (RT_FAILURE(rcIprt))
                RTErrInfoSet(pErrInfo, rcIprt, "RTCrPkixSignatureVerifyBitString failed");
        }
        else
            RTErrInfoSet(pErrInfo, rcIprt, "RTCrDigestUpdate failed");
        RTCrDigestRelease(hDigest);
    }
    else
        RTErrInfoSetF(pErrInfo, rcIprt, "Unknown digest algorithm [IPRT]: %s", pAlgorithm->szObjId);
    RTCrPkixSignatureRelease(hSignature);

#ifdef IPRT_WITH_OPENSSL
    /*
     * Validate using OpenSSL EVP.
     */
    rtCrOpenSslInit();

    /* Translate the algorithm ID into a EVP message digest type pointer. */
    int iAlgoNid = OBJ_txt2nid(pAlgorithm->szObjId);
    if (iAlgoNid == NID_undef)
        return RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_CIPHER_ALGO_NOT_KNOWN,
                             "Unknown public key algorithm [OpenSSL]: %s", pAlgorithm->szObjId);
    const char *pszAlgoSn = OBJ_nid2sn(iAlgoNid);

# if OPENSSL_VERSION_NUMBER >= 0x10001000 && !defined(LIBRESSL_VERSION_NUMBER)
    int idAlgoPkey = 0;
    int idAlgoMd = 0;
    if (!OBJ_find_sigid_algs(iAlgoNid, &idAlgoMd, &idAlgoPkey))
        return RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_CIPHER_ALGO_NOT_KNOWN_EVP,
                             "OBJ_find_sigid_algs failed on %u (%s, %s)", iAlgoNid, pszAlgoSn, pAlgorithm->szObjId);
    const EVP_MD *pEvpMdType = EVP_get_digestbynid(idAlgoMd);
    if (!pEvpMdType)
        return RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_CIPHER_ALGO_NOT_KNOWN_EVP,
                             "EVP_get_digestbynid failed on %d (%s, %s)", idAlgoMd, pszAlgoSn, pAlgorithm->szObjId);
# else
    const EVP_MD *pEvpMdType = EVP_get_digestbyname(pszAlgoSn);
    if (!pEvpMdType)
        return RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_CIPHER_ALGO_NOT_KNOWN_EVP,
                             "EVP_get_digestbyname failed on %s (%s)", pszAlgoSn, pAlgorithm->szObjId);
# endif

    EVP_MD_CTX *pEvpMdCtx = EVP_MD_CTX_create();
    if (!pEvpMdCtx)
        return RTErrInfoSetF(pErrInfo, VERR_NO_MEMORY, "EVP_MD_CTX_create failed");
    int rcOssl;
    if (EVP_VerifyInit_ex(pEvpMdCtx, pEvpMdType, NULL /*engine*/))
    {
        /* Create an EVP public key. */
        EVP_PKEY *pEvpPublicKey = EVP_PKEY_new();
        if (pEvpPublicKey)
        {
# if OPENSSL_VERSION_NUMBER >= 0x10001000 && !defined(LIBRESSL_VERSION_NUMBER)
            if (EVP_PKEY_set_type(pEvpPublicKey, idAlgoPkey))
            {
                int idKeyType = EVP_PKEY_base_id(pEvpPublicKey);
# else
                int idKeyType = pEvpPublicKey->type = EVP_PKEY_type(pEvpMdType->required_pkey_type[0]);
# endif
                if (idKeyType != NID_undef)
                {
                    const unsigned char *puchPublicKey = RTASN1BITSTRING_GET_BIT0_PTR(pPublicKey);
                    if (d2i_PublicKey(idKeyType, &pEvpPublicKey, &puchPublicKey, RTASN1BITSTRING_GET_BYTE_SIZE(pPublicKey)))
                    {
                        /* Digest the data. */
                        EVP_VerifyUpdate(pEvpMdCtx, pvData, cbData);

                        /* Verify the signature. */
                        if (EVP_VerifyFinal(pEvpMdCtx,
                                            RTASN1BITSTRING_GET_BIT0_PTR(pSignatureValue),
                                            RTASN1BITSTRING_GET_BYTE_SIZE(pSignatureValue),
                                            pEvpPublicKey) > 0)
                            rcOssl = VINF_SUCCESS;
                        else
                            rcOssl = RTErrInfoSet(pErrInfo, VERR_CR_PKIX_OSSL_VERIFY_FINAL_FAILED, "EVP_VerifyFinal failed");
                    }
                    else
                        rcOssl = RTErrInfoSet(pErrInfo, VERR_CR_PKIX_OSSL_D2I_PUBLIC_KEY_FAILED, "d2i_PublicKey failed");
                }
                else
# if OPENSSL_VERSION_NUMBER < 0x10001000 || defined(LIBRESSL_VERSION_NUMBER)
                    rcOssl = RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_EVP_PKEY_TYPE_ERROR, "EVP_PKEY_type() failed");
# else
                    rcOssl = RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_EVP_PKEY_TYPE_ERROR, "EVP_PKEY_base_id() failed");
            }
            else
                rcOssl = RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_EVP_PKEY_TYPE_ERROR,
                                       "EVP_PKEY_set_type(%u) failed (sig algo %s)", idAlgoPkey, pszAlgoSn);
# endif
            /* Cleanup and return.*/
            EVP_PKEY_free(pEvpPublicKey);
        }
        else
            rcOssl = RTErrInfoSetF(pErrInfo, VERR_NO_MEMORY, "EVP_PKEY_new(%d) failed", iAlgoNid);
    }
    else
        rcOssl = RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_CIPHER_ALOG_INIT_FAILED,
                               "EVP_VerifyInit_ex failed (algorithm type is %s / %s)", pszAlgoSn, pAlgorithm->szObjId);
    EVP_MD_CTX_destroy(pEvpMdCtx);

    /*
     * Check the result.
     */
    if (RT_SUCCESS(rcIprt) && RT_SUCCESS(rcOssl))
        return VINF_SUCCESS;
    if (RT_FAILURE_NP(rcIprt) && RT_FAILURE_NP(rcOssl))
        return rcIprt;
    AssertMsgFailed(("rcIprt=%Rrc rcOssl=%Rrc\n", rcIprt, rcOssl));
    if (RT_FAILURE_NP(rcOssl))
        return rcOssl;
#endif /* IPRT_WITH_OPENSSL */

    return rcIprt;
}


RTDECL(int) RTCrPkixPubKeyVerifySignedDigest(PCRTASN1OBJID pAlgorithm, PCRTASN1DYNTYPE pParameters, PCRTASN1BITSTRING pPublicKey,
                                             void const *pvSignedDigest, size_t cbSignedDigest, RTCRDIGEST hDigest,
                                             PRTERRINFO pErrInfo)
{
    /*
     * Valid input.
     */
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

    AssertPtrReturn(pvSignedDigest, VERR_INVALID_POINTER);
    AssertReturn(cbSignedDigest, VERR_INVALID_PARAMETER);

    AssertPtrReturn(hDigest, VERR_INVALID_HANDLE);

    /*
     * Parameters are not currently supported (openssl code path).
     */
    if (pParameters)
        return RTErrInfoSet(pErrInfo, VERR_CR_PKIX_CIPHER_ALGO_PARAMS_NOT_IMPL,
                            "Cipher algorithm parameters are not yet supported.");

    /*
     * Validate using IPRT.
     */
    RTCRPKIXSIGNATURE hSignature;
    int rcIprt = RTCrPkixSignatureCreateByObjId(&hSignature, pAlgorithm, false /*fSigning*/, pPublicKey, pParameters);
    if (RT_FAILURE(rcIprt))
        return RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_CIPHER_ALGO_NOT_KNOWN,
                             "Unknown public key algorithm [IPRT]: %s", pAlgorithm->szObjId);

    rcIprt = RTCrPkixSignatureVerify(hSignature, hDigest, pvSignedDigest, cbSignedDigest);
    if (RT_FAILURE(rcIprt))
        RTErrInfoSet(pErrInfo, rcIprt, "RTCrPkixSignatureVerifyBitString failed");

    RTCrPkixSignatureRelease(hSignature);

#if defined(IPRT_WITH_OPENSSL) \
  && (OPENSSL_VERSION_NUMBER > 0x10000000L) /* 0.9.8 doesn't seem to have EVP_PKEY_CTX_set_signature_md. */
    /*
     * Validate using OpenSSL EVP.
     */
    rtCrOpenSslInit();

    const char *pszAlgObjId = pAlgorithm->szObjId;
    if (!strcmp(pszAlgObjId, RTCRX509ALGORITHMIDENTIFIERID_RSA))
    {
        pszAlgObjId = RTCrX509AlgorithmIdentifier_CombineEncryptionOidAndDigestOid(pszAlgObjId,
                                                                                   RTCrDigestGetAlgorithmOid(hDigest));
        AssertMsgStmt(pszAlgObjId, ("enc=%s hash=%s\n", pAlgorithm->szObjId, RTCrDigestGetAlgorithmOid(hDigest)),
                      pszAlgObjId = RTCrDigestGetAlgorithmOid(hDigest));
    }

    /* Translate the algorithm ID into a EVP message digest type pointer. */
    int iAlgoNid = OBJ_txt2nid(pszAlgObjId);
    if (iAlgoNid == NID_undef)
        return RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_CIPHER_ALGO_NOT_KNOWN,
                             "Unknown public key algorithm [OpenSSL]: %s", pszAlgObjId);
    const char *pszAlgoSn = OBJ_nid2sn(iAlgoNid);

# if OPENSSL_VERSION_NUMBER >= 0x10001000 && !defined(LIBRESSL_VERSION_NUMBER)
    int idAlgoPkey = 0;
    int idAlgoMd = 0;
    if (!OBJ_find_sigid_algs(iAlgoNid, &idAlgoMd, &idAlgoPkey))
        return RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_CIPHER_ALGO_NOT_KNOWN_EVP,
                             "OBJ_find_sigid_algs failed on %u (%s, %s)", iAlgoNid, pszAlgoSn, pAlgorithm->szObjId);
    const EVP_MD *pEvpMdType = EVP_get_digestbynid(idAlgoMd);
    if (!pEvpMdType)
        return RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_CIPHER_ALGO_NOT_KNOWN_EVP,
                             "EVP_get_digestbynid failed on %d (%s, %s)", idAlgoMd, pszAlgoSn, pAlgorithm->szObjId);
# else
    const EVP_MD *pEvpMdType = EVP_get_digestbyname(pszAlgoSn);
    if (!pEvpMdType)
        return RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_CIPHER_ALGO_NOT_KNOWN_EVP,
                             "EVP_get_digestbyname failed on %s (%s)", pszAlgoSn, pszAlgObjId);
# endif

    /* Create an EVP public key. */
    int rcOssl;
    EVP_PKEY *pEvpPublicKey = EVP_PKEY_new();
    if (pEvpPublicKey)
    {
# if OPENSSL_VERSION_NUMBER >= 0x10001000 && !defined(LIBRESSL_VERSION_NUMBER)
        if (EVP_PKEY_set_type(pEvpPublicKey, idAlgoPkey))
        {
            int idKeyType = EVP_PKEY_base_id(pEvpPublicKey);
# else
            int idKeyType = pEvpPublicKey->type = EVP_PKEY_type(pEvpMdType->required_pkey_type[0]);
# endif
            if (idKeyType != NID_undef)

            {
                const unsigned char *puchPublicKey = RTASN1BITSTRING_GET_BIT0_PTR(pPublicKey);
                if (d2i_PublicKey(idKeyType, &pEvpPublicKey, &puchPublicKey, RTASN1BITSTRING_GET_BYTE_SIZE(pPublicKey)))
                {
                    /* Create an EVP public key context we can use to validate the digest. */
                    EVP_PKEY_CTX *pEvpPKeyCtx = EVP_PKEY_CTX_new(pEvpPublicKey, NULL);
                    if (pEvpPKeyCtx)
                    {
                        rcOssl = EVP_PKEY_verify_init(pEvpPKeyCtx);
                        if (rcOssl > 0)
                        {
                            rcOssl = EVP_PKEY_CTX_set_signature_md(pEvpPKeyCtx, pEvpMdType);
                            if (rcOssl > 0)
                            {
                                /* Get the digest from hDigest and verify it. */
                                rcOssl = EVP_PKEY_verify(pEvpPKeyCtx,
                                                         (uint8_t const *)pvSignedDigest,
                                                         cbSignedDigest,
                                                         RTCrDigestGetHash(hDigest),
                                                         RTCrDigestGetHashSize(hDigest));
                                if (rcOssl > 0)
                                    rcOssl = VINF_SUCCESS;
                                else
                                    rcOssl = RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_VERIFY_FINAL_FAILED,
                                                           "EVP_PKEY_verify failed (%d)", rcOssl);
                            }
                            else
                                rcOssl = RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_EVP_PKEY_TYPE_ERROR,
                                                       "EVP_PKEY_CTX_set_signature_md failed (%d)", rcOssl);
                        }
                        else
                            rcOssl = RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_EVP_PKEY_TYPE_ERROR,
                                                   "EVP_PKEY_verify_init failed (%d)", rcOssl);
                        EVP_PKEY_CTX_free(pEvpPKeyCtx);
                    }
                    else
                        rcOssl = RTErrInfoSet(pErrInfo, VERR_CR_PKIX_OSSL_EVP_PKEY_TYPE_ERROR, "EVP_PKEY_CTX_new failed");
                }
                else
                    rcOssl = RTErrInfoSet(pErrInfo, VERR_CR_PKIX_OSSL_D2I_PUBLIC_KEY_FAILED, "d2i_PublicKey failed");
            }
            else
# if OPENSSL_VERSION_NUMBER < 0x10001000 || defined(LIBRESSL_VERSION_NUMBER)
                rcOssl = RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_EVP_PKEY_TYPE_ERROR, "EVP_PKEY_type() failed");
# else
                rcOssl = RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_EVP_PKEY_TYPE_ERROR, "EVP_PKEY_base_id() failed");
        }
        else
            rcOssl = RTErrInfoSetF(pErrInfo, VERR_CR_PKIX_OSSL_EVP_PKEY_TYPE_ERROR,
                                   "EVP_PKEY_set_type(%u) failed (sig algo %s)", idAlgoPkey, pszAlgoSn);
# endif

        /* Cleanup and return.*/
        EVP_PKEY_free(pEvpPublicKey);
    }
    else
        rcOssl = RTErrInfoSetF(pErrInfo, VERR_NO_MEMORY, "EVP_PKEY_new(%d) failed", iAlgoNid);

    /*
     * Check the result.
     */
    if (RT_SUCCESS(rcIprt) && RT_SUCCESS(rcOssl))
        return VINF_SUCCESS;
    if (RT_FAILURE_NP(rcIprt) && RT_FAILURE_NP(rcOssl))
        return rcIprt;
    AssertMsgFailed(("rcIprt=%Rrc rcOssl=%Rrc\n", rcIprt, rcOssl));
    if (RT_FAILURE_NP(rcOssl))
        return rcOssl;
#endif /* IPRT_WITH_OPENSSL */

    return rcIprt;
}

