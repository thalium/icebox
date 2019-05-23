/** @file
 * IPRT - Public Key Infrastructure APIs.
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

#ifndef ___iprt_crypto_pkix_h
#define ___iprt_crypto_pkix_h

#include <iprt/asn1.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_crpkix RTCrPkix - Public Key Infrastructure APIs
 * @ingroup grp_rt_crypto
 * @{
 */

/**
 * Verifies the signature (@a pSignatureValue) of the give data (@a pvData)
 * using the specfied public key (@a pPublicKey) and algorithm.
 *
 * @returns IPRT status code.
 * @param   pAlgorithm      The signature algorithm (digest w/ cipher).
 * @param   pParameters     Parameter to the public key algorithm. Optional.
 * @param   pPublicKey      The public key.
 * @param   pSignatureValue The signature value.
 * @param   pvData          The signed data.
 * @param   cbData          The amount of signed data.
 * @param   pErrInfo        Where to return extended error info. Optional.
 *
 * @remarks Depending on the IPRT build configuration, the verficiation may be
 *          performed more than once using all available crypto implementations.
 */
RTDECL(int) RTCrPkixPubKeyVerifySignature(PCRTASN1OBJID pAlgorithm, PCRTASN1DYNTYPE pParameters, PCRTASN1BITSTRING pPublicKey,
                                          PCRTASN1BITSTRING pSignatureValue, const void *pvData, size_t cbData,
                                          PRTERRINFO pErrInfo);


/**
 * Verifies the signed digest (@a pvSignedDigest) against our digest (@a
 * hDigest) using the specfied public key (@a pPublicKey) and algorithm.
 *
 * @returns IPRT status code.
 * @param   pAlgorithm      The signature algorithm (digest w/ cipher).
 * @param   pParameters     Parameter to the public key algorithm. Optional.
 * @param   pPublicKey      The public key.
 * @param   pvSignedDigest  The signed digest.
 * @param   cbSignedDigest  The signed digest size.
 * @param   hDigest         The digest of the data to compare @a pvSignedDigest
 *                          with.
 * @param   pErrInfo        Where to return extended error info. Optional.
 *
 * @remarks Depending on the IPRT build configuration, the verficiation may be
 *          performed more than once using all available crypto implementations.
 */
RTDECL(int) RTCrPkixPubKeyVerifySignedDigest(PCRTASN1OBJID pAlgorithm, PCRTASN1DYNTYPE pParameters,
                                             PCRTASN1BITSTRING pPublicKey, void const *pvSignedDigest, size_t cbSignedDigest,
                                             RTCRDIGEST hDigest, PRTERRINFO pErrInfo);


/**
 * Gets the cipher OID matching the given signature algorithm.
 *
 * @returns Cipher OID string on success, NULL on failure.
 * @param   pAlgorithm      The signature algorithm (digest w/ cipher).
 */
RTDECL(const char *) RTCrPkixGetCiperOidFromSignatureAlgorithm(PCRTASN1OBJID pAlgorithm);


/** @name PKCS-1 Object Identifiers (OIDs)
 * @{ */
#define RTCR_PKCS1_OID                              "1.2.840.113549.1.1"
#define RTCR_PKCS1_RSA_OID                          "1.2.840.113549.1.1.1"
#define RTCR_PKCS1_MD2_WITH_RSA_OID                 "1.2.840.113549.1.1.2"
#define RTCR_PKCS1_MD4_WITH_RSA_OID                 "1.2.840.113549.1.1.3"
#define RTCR_PKCS1_MD5_WITH_RSA_OID                 "1.2.840.113549.1.1.4"
#define RTCR_PKCS1_SHA1_WITH_RSA_OID                "1.2.840.113549.1.1.5"
#define RTCR_PKCS1_RSA_OAEP_ENCRYPTION_SET_OID      "1.2.840.113549.1.1.6"
#define RTCR_PKCS1_RSA_AES_OAEP_OID                 "1.2.840.113549.1.1.7"
#define RTCR_PKCS1_MSGF1_OID                        "1.2.840.113549.1.1.8"
#define RTCR_PKCS1_P_SPECIFIED_OID                  "1.2.840.113549.1.1.9"
#define RTCR_PKCS1_RSASSA_PSS_OID                   "1.2.840.113549.1.1.10"
#define RTCR_PKCS1_SHA256_WITH_RSA_OID              "1.2.840.113549.1.1.11"
#define RTCR_PKCS1_SHA384_WITH_RSA_OID              "1.2.840.113549.1.1.12"
#define RTCR_PKCS1_SHA512_WITH_RSA_OID              "1.2.840.113549.1.1.13"
#define RTCR_PKCS1_SHA224_WITH_RSA_OID              "1.2.840.113549.1.1.14"
/** @}  */


/**
 * Public key signature scheme provider descriptor.
 */
typedef struct RTCRPKIXSIGNATUREDESC
{
    /** The signature scheme provider name. */
    const char         *pszName;
    /** The object ID string. */
    const char         *pszObjId;
    /** Pointer to a NULL terminated table of alias object IDs (optional). */
    const char * const *papszObjIdAliases;
    /** The size of the state. */
    uint32_t            cbState;
    /** Reserved for future / explicit padding. */
    uint32_t            uReserved;
    /** Provider specific field.  This generally indicates the kind of padding
     *  scheme to employ with the given OID. */
    uintptr_t           uProviderSpecific;

    /**
     * Initializes the state of the signature scheme provider.
     *
     * Optional, RT_BZERO will be used if NULL.
     *
     * @returns IPRT status code.
     * @param   pDesc           Pointer to this structure (for uProviderSpecific).
     * @param   pvState         The opaque provider state.
     * @param   pvOpaque        Opaque provider specific parameter.
     * @param   fSigning        Set if a signing operation is going to be performed,
     *                          clear if it is a verification.  This is a fixed
     *                          setting for the lifetime of the instance due to the
     *                          algorithm requiring different keys.
     * @param   pKey            The key to use (whether private or public depends on
     *                          the operation).
     * @param   pParams         Algorithm/key parameters, optional.  Will be NULL if
     *                          none.
     */
    DECLCALLBACKMEMBER(int, pfnInit)(struct RTCRPKIXSIGNATUREDESC const *pDesc, void *pvState, void *pvOpaque, bool fSigning,
                                     PCRTASN1BITSTRING pKey, PCRTASN1DYNTYPE pParams);

    /**
     * Resets the state before performing another signing or verification.
     *
     * Optional.  It is assumed that the provider does not have any state needing to
     * be re-initialized if this method is not implemented.
     *
     * @returns IPRT status code.
     * @param   pDesc           Pointer to this structure (for uProviderSpecific).
     * @param   pvState         The opaque provider state.
     * @param   fSigning        Exactly the same value as the init call.
     */
    DECLCALLBACKMEMBER(int, pfnReset)(struct RTCRPKIXSIGNATUREDESC const *pDesc, void *pvState, bool fSigning);

    /**
     * Deletes the provider state. Optional.
     *
     * The state will be securely wiped clean after the call, regardless of whether
     * the method is implemented or not.
     *
     * @param   pDesc           Pointer to this structure (for uProviderSpecific).
     * @param   pvState         The opaque provider state.
     * @param   fSigning        Exactly the same value as the init call.
     */
    DECLCALLBACKMEMBER(void, pfnDelete)(struct RTCRPKIXSIGNATUREDESC const *pDesc, void *pvState, bool fSigning);

    /**
     * Verifies a signed message digest (fSigning = false).
     *
     * @returns IPRT status code.
     * @retval  VINF_SUCCESS if the signature checked out correctly.
     * @retval  VERR_PKIX_KEY wrong key or some other key issue.
     *
     * @param   pDesc           Pointer to this structure (for uProviderSpecific).
     * @param   pvState         The opaque provider state.
     * @param   hDigest         The handle to the digest.  Call RTCrDigestFinal to
     *                          complete and retreive the final hash value.
     * @param   pvSignature     The signature to validate.
     * @param   cbSignature     The size of the signature (in bytes).
     */
    DECLCALLBACKMEMBER(int, pfnVerify)(struct RTCRPKIXSIGNATUREDESC const *pDesc, void *pvState,
                                       RTCRDIGEST hDigest, void const *pvSignature, size_t cbSignature);

    /**
     * Sign a message digest (fSigning = true).
     *
     * @returns IPRT status code.
     * @retval  VINF_SUCCESS on success.
     * @retval  VERR_PKIX_KEY wrong key or some other key issue.
     * @retval  VERR_BUFFER_OVERFLOW if the signature buffer is too small, the
     *          require buffer size will be available in @a *pcbSignature.
     *
     * @param   pDesc           Pointer to this structure (for uProviderSpecific).
     * @param   pvState         The opaque provider state.
     * @param   hDigest         The handle to the digest.  Call RTCrDigestFinal to
     *                          complete and retreive the final hash value.
     * @param   pvSignature     The output signature buffer.
     * @param   pcbSignature    On input the variable pointed to holds the size of
     *                          the buffer @a pvSignature points to.
     *                          On return the variable pointed to is set to the size
     *                          of the returned signature, or the required size in
     *                          case of VERR_BUFFER_OVERFLOW.
     */
    DECLCALLBACKMEMBER(int, pfnSign)(struct RTCRPKIXSIGNATUREDESC const *pDesc, void *pvState,
                                     RTCRDIGEST hDigest, void *pvSignature, size_t *pcbSignature);

} RTCRPKIXSIGNATUREDESC;
/** Pointer to a public key signature scheme provider descriptor. */
typedef RTCRPKIXSIGNATUREDESC const *PCRTCRPKIXSIGNATUREDESC;


PCRTCRPKIXSIGNATUREDESC RTCrPkixSignatureFindByObjIdString(const char *pszObjId, void *ppvOpaque);
PCRTCRPKIXSIGNATUREDESC RTCrPkixSignatureFindByObjId(PCRTASN1OBJID pObjId, void *ppvOpaque);
RTDECL(int) RTCrPkixSignatureCreateByObjIdString(PRTCRPKIXSIGNATURE phSignature, const char *pszObjId, bool fSigning,
                                                 PCRTASN1BITSTRING pKey,PCRTASN1DYNTYPE pParams);
RTDECL(int) RTCrPkixSignatureCreateByObjId(PRTCRPKIXSIGNATURE phSignature, PCRTASN1OBJID pObjId, bool fSigning,
                                           PCRTASN1BITSTRING pKey, PCRTASN1DYNTYPE pParams);


RTDECL(int) RTCrPkixSignatureCreate(PRTCRPKIXSIGNATURE phSignature, PCRTCRPKIXSIGNATUREDESC pDesc, void *pvOpaque,
                                    bool fSigning, PCRTASN1BITSTRING pKey, PCRTASN1DYNTYPE pParams);
RTDECL(uint32_t) RTCrPkixSignatureRetain(RTCRPKIXSIGNATURE hSignature);
RTDECL(uint32_t) RTCrPkixSignatureRelease(RTCRPKIXSIGNATURE hSignature);
RTDECL(int) RTCrPkixSignatureVerify(RTCRPKIXSIGNATURE hSignature, RTCRDIGEST hDigest,
                                    void const *pvSignature, size_t cbSignature);
RTDECL(int) RTCrPkixSignatureVerifyBitString(RTCRPKIXSIGNATURE hSignature, RTCRDIGEST hDigest, PCRTASN1BITSTRING pSignature);
RTDECL(int) RTCrPkixSignatureVerifyOctetString(RTCRPKIXSIGNATURE hSignature, RTCRDIGEST hDigest, PCRTASN1OCTETSTRING pSignature);
RTDECL(int) RTCrPkixSignatureSign(RTCRPKIXSIGNATURE hSignature, RTCRDIGEST hDigest,
                                  void *pvSignature, size_t *pcbSignature);


/**
 * Public key encryption scheme provider descriptor.
 *
 * @todo This is just a sketch left over from when the signature code was
 *       chiseled out.
 */
typedef struct RTCRPKIXENCRYPTIONDESC
{
    /** The encryption scheme provider name. */
    const char         *pszName;
    /** The object ID string. */
    const char         *pszObjId;
    /** Pointer to a NULL terminated table of alias object IDs (optional). */
    const char * const *papszObjIdAliases;
    /** The size of the state. */
    uint32_t            cbState;
    /** Reserved for future use / padding.  */
    uint32_t            uReserved;
    /** Provider specific field. */
    uintptr_t           uProviderSpecific;

    /**
     * Initializes the state for this encryption scheme.
     *
     * Optional, RT_BZERO will be used if NULL.
     *
     * @returns IPRT status code.
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   pvOpaque        Opaque provider specific parameter.
     * @param   fEncrypt        Set if the instance will be encrypting, clear if it
     *                          will be decrypting.  This aspect of the instance is
     *                          immutable due to the algorithm requiring different
     *                          keys for each of the operations.
     * @param   pKey            The key to use (whether private or public depends on
     *                          the operation type).
     * @param   pParams         Algorithm/key parameters, optional.  Will be NULL if
     *                          none.
     */
    DECLCALLBACKMEMBER(int, pfnInit)(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState, void *pvOpaque, bool fEncrypt,
                                     PCRTASN1BITSTRING pKey, PCRTASN1DYNTYPE pParams);

    /**
     * Re-initializes the provider state.
     *
     * Optional.  It is assumed that the provider does not have any state needing
     * to be re-initialized if this method is not implemented.  (Do not assume that
     * a final encrypt/decrypt call has been made prior to this call.)
     *
     * @returns IPRT status code.
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   enmOperation    Same as for the earlier pfnInit call.
     */
    DECLCALLBACKMEMBER(int, pfnReset)(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState, bool fEncrypt);

    /**
     * Deletes the provider state. Optional.
     *
     * The state will be securely wiped clean after the call, regardless of whether
     * the method is implemented or not.
     *
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   enmOperation    Same as for the earlier pfnInit call.
     */
    DECLCALLBACKMEMBER(void, pfnDelete)(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState, bool fEncrypt);

    /**
     * Encrypt using the public key (fEncrypt = true).
     *
     * @returns IPRT status code.
     * @retval  VINF_SUCCESS on success.
     * @retval  VERR_PKIX_KEY wrong key or some other key issue.
     * @retval  VERR_BUFFER_OVERFLOW if the output buffer is too small, the require
     *          buffer size will be available in @a *pcbCiphertext.  The caller can
     *          should retry the call with a larger buffer.
     *
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   pvPlaintext     The plaintext to encrypt.
     * @param   cbPlaintext     The number of bytes of plaintext.
     * @param   pvCiphertext    Where to return the ciphertext (if any).
     * @param   cbMaxCiphertext The size of the buffer @a pvCiphertext points to.
     * @param   pcbCiphertext   Where to return the actual number of bytes of
     *                          ciphertext returned.
     * @param   fFinal          Whether this is the final call.
     */
    DECLCALLBACKMEMBER(int, pfnEncrypt)(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState,
                                        void const *pvPlaintext, size_t cbPlaintext,
                                        void *pvCiphertext, size_t cbMaxCiphertext, size_t *pcbCiphertext, bool fFinal);

    /**
     * Calculate the output buffer size for the next pfnEncrypt call.
     *
     * @returns IPRT status code.
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   cbPlaintext     The number of bytes of plaintext.
     * @param   pcbCiphertext   Where to return the minimum buffer size.  This may
     *                          be larger than the actual number of bytes return.
     * @param   fFinal          Whether this is the final call.
     */
    DECLCALLBACKMEMBER(int, pfnEncryptLength)(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState,
                                              size_t cbPlaintext, size_t *pcbCiphertext, bool fFinal);

    /**
     * Decrypt using the private key (fEncrypt = false).
     *
     * @returns IPRT status code.
     * @retval  VINF_SUCCESS on success.
     * @retval  VERR_PKIX_KEY wrong key or some other key issue.
     * @retval  VERR_BUFFER_OVERFLOW if the output buffer is too small, the require
     *          buffer size will be available in @a *pcbCiphertext.  The caller can
     *          should retry the call with a larger buffer.
     *
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   pvCiphertext    The ciphertext to decrypt.
     * @param   cbCiphertext    The number of bytes of ciphertext.
     * @param   pvPlaintext     Where to return the plaintext (if any).
     * @param   cbMaxPlaintext  The size of the buffer @a pvPlaintext points to.
     * @param   pcbPlaintext    Where to return the actual number of bytes of
     *                          plaintext returned.
     * @param   fFinal          Whether this is the final call.
     */
    DECLCALLBACKMEMBER(int, pfnDecrypt)(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState,
                                        void const *pvCiphertext, size_t cbCiphertext,
                                        void *pvPlaintext, size_t cbMaxPlaintext, size_t *pcbPlaintext, bool fFinal);

    /**
     * Calculate the output buffer size for the next pfnDecrypt call.
     *
     * @returns IPRT status code.
     * @param   pDesc           Pointer to this structure (so uProviderSpecific can
     *                          be read).
     * @param   pvState         The opaque provider state.
     * @param   cbCiphertext    The number of bytes of ciphertext.
     * @param   pcbPlaintext    Where to return the minimum buffer size.  This may
     *                          be larger than the actual number of bytes return.
     * @param   fFinal          Whether this is the final call.
     */
    DECLCALLBACKMEMBER(int, pfnDecryptLength)(struct RTCRPKIXENCRYPTIONDESC const *pDesc, void *pvState,
                                              size_t cbCiphertext, size_t *pcbPlaintext, bool fFinal);
} RTCRPKIXENCRYPTIONDESC;
/** Pointer to a public key encryption schema provider descriptor. */
typedef RTCRPKIXENCRYPTIONDESC const *PCRTCRPKIXENCRYPTIONDESC;


PCRTCRPKIXENCRYPTIONDESC RTCrPkixEncryptionFindByObjIdString(const char *pszObjId, void *ppvOpaque);
PCRTCRPKIXENCRYPTIONDESC RTCrPkixEncryptionFindByObjId(PCRTASN1OBJID pObjId, void *ppvOpaque);
RTDECL(int) RTCrPkixEncryptionCreateByObjIdString(PRTCRPKIXENCRYPTION phEncryption, const char *pszObjId,
                                                  bool fEncrypt, PCRTASN1BITSTRING pKey,PCRTASN1DYNTYPE pParams);
RTDECL(int) RTCrPkixEncryptionCreateByObjId(PRTCRPKIXENCRYPTION phEncryption, PCRTASN1OBJID pObjId, bool fEncrypt,
                                            PCRTASN1BITSTRING pKey, PCRTASN1DYNTYPE pParams);


RTDECL(int) RTCrPkixEncryptionCreate(PRTCRPKIXENCRYPTION phEncryption, PCRTCRPKIXENCRYPTIONDESC pDesc, void *pvOpaque,
                                     bool fEncrypt, PCRTASN1BITSTRING pKey, PCRTASN1DYNTYPE pParams);
RTDECL(int) RTCrPkixEncryptionReset(RTCRPKIXENCRYPTION hEncryption);
RTDECL(uint32_t) RTCrPkixEncryptionRetain(RTCRPKIXENCRYPTION hEncryption);
RTDECL(uint32_t) RTCrPkixEncryptionRelease(RTCRPKIXENCRYPTION hEncryption);


/** @} */

RT_C_DECLS_END

#endif

