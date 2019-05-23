/** @file
 * IPRT - Crypto - Cryptographic Hash / Message Digest.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
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

#ifndef ___iprt_crypto_digest_h
#define ___iprt_crypto_digest_h

#include <iprt/asn1.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_crdigest RTCrDigest - Crypographic Hash / Message Digest API.
 * @ingroup grp_rt
 * @{
 */

/**
 * Cryptographic hash / message digest provider descriptor.
 *
 * This gives the basic details and identifiers of the algorithm as well as
 * function pointers to the implementation.
 */
typedef struct RTCRDIGESTDESC
{
    /** The message digest provider name. */
    const char         *pszName;
    /** The object ID string. */
    const char         *pszObjId;
    /** Pointer to a NULL terminated table of alias object IDs (optional). */
    const char * const *papszObjIdAliases;
    /** The IPRT digest type. */
    RTDIGESTTYPE        enmType;
    /** The max size of the final hash (binary). */
    uint32_t            cbHash;
    /** The size of the state. */
    uint32_t            cbState;
    /** Reserved. */
    uint32_t            uReserved;

    /**
     * Allocates the digest data.
     */
    DECLCALLBACKMEMBER(void *, pfnNew)(void);

    /**
     * Frees the digest data.
     *
     * @param   pvState     The opaque message digest state.
     */
    DECLCALLBACKMEMBER(void, pfnFree)(void *pvState);

    /**
     * Updates the digest with more data.
     *
     * @param   pvState     The opaque message digest state.
     * @param   pvData      The data to add to the digest.
     * @param   cbData      The amount of data to add to the digest.
     */
    DECLCALLBACKMEMBER(void, pfnUpdate)(void *pvState, const void *pvData, size_t cbData);

    /**
     * Finalizes the digest calculation.
     *
     * @param   pvState     The opaque message digest state.
     * @param   pbHash      Where to store the output digest.  This buffer is at
     *                      least RTCRDIGESTDESC::cbHash bytes large.
     */
    DECLCALLBACKMEMBER(void, pfnFinal)(void *pvState, uint8_t *pbHash);

    /**
     * (Re-)Initializes the digest. Optional.
     *
     * Optional, RT_BZERO will be used if NULL.
     *
     * @returns IPRT status code.
     * @param   pvState     The opaque message digest state.
     * @param   pvOpaque    Opaque algortihm specific parameter.
     * @param   fReInit     Set if this is a re-init call.
     */
    DECLCALLBACKMEMBER(int, pfnInit)(void *pvState, void *pvOpaque, bool fReInit);

    /**
     * Deletes the message digest state.
     *
     * Optional, memset will be used if NULL.
     *
     * @param   pvState     The opaque message digest state.
     */
    DECLCALLBACKMEMBER(void, pfnDelete)(void *pvState);

    /**
     * Clones the message digest state.
     *
     * Optional, memcpy will be used if NULL.
     *
     * @returns IPRT status code.
     * @param   pvState     The opaque message digest state (destination).
     * @param   pvSrcState  The opaque message digest state to clone (source).
     */
    DECLCALLBACKMEMBER(int, pfnClone)(void *pvState, void const *pvSrcState);

    /**
     * Gets the hash size.
     *
     * Optional, if not provided RTCRDIGESTDESC::cbHash will be returned.  If
     * provided though, RTCRDIGESTDESC::cbHash must be set to the largest possible
     * hash size.
     *
     * @returns The hash size.
     * @param   pvState     The opaque message digest state.
     */
    DECLCALLBACKMEMBER(uint32_t, pfnGetHashSize)(void *pvState);

    /**
     * Gets the digest type (when enmType is RTDIGESTTYPE_UNKNOWN).
     *
     * @returns The hash size.
     * @param   pvState     The opaque message digest state.
     */
    DECLCALLBACKMEMBER(RTDIGESTTYPE, pfnGetDigestType)(void *pvState);
} RTCRDIGESTDESC;
/** Pointer to const message digest details and vtable. */
typedef RTCRDIGESTDESC const *PCRTCRDIGESTDESC;

/**
 * Finds a cryptographic hash / message digest descriptor by object identifier
 * string.
 *
 * @returns Pointer to the message digest details & vtable if found.  NULL if
 *          not found.
 * @param   pszObjId        The dotted object identifier string of the message
 *                          digest algorithm.
 * @param   ppvOpaque       Where to return an opaque implementation specfici
 *                          sub-type indicator that can be passed to
 *                          RTCrDigestCreate.  This is optional, fewer
 *                          algortihms are available if not specified.
 */
RTDECL(PCRTCRDIGESTDESC) RTCrDigestFindByObjIdString(const char *pszObjId, void **ppvOpaque);

/**
 * Finds a cryptographic hash / message digest descriptor by object identifier
 * ASN.1 object.
 *
 * @returns Pointer to the message digest details & vtable if found.  NULL if
 *          not found.
 * @param   pObjId          The ASN.1 object ID of the message digest algorithm.
 * @param   ppvOpaque       Where to return an opaque implementation specfici
 *                          sub-type indicator that can be passed to
 *                          RTCrDigestCreate.  This is optional, fewer
 *                          algortihms are available if not specified.
 */
RTDECL(PCRTCRDIGESTDESC) RTCrDigestFindByObjId(PCRTASN1OBJID pObjId, void **ppvOpaque);

RTDECL(PCRTCRDIGESTDESC) RTCrDigestFindByType(RTDIGESTTYPE enmDigestType);
RTDECL(int) RTCrDigestCreateByObjIdString(PRTCRDIGEST phDigest, const char *pszObjId);
RTDECL(int) RTCrDigestCreateByObjId(PRTCRDIGEST phDigest, PCRTASN1OBJID pObjId);
RTDECL(int) RTCrDigestCreateByType(PRTCRDIGEST phDigest, RTDIGESTTYPE enmDigestType);


RTDECL(int) RTCrDigestCreate(PRTCRDIGEST phDigest, PCRTCRDIGESTDESC pDesc, void *pvOpaque);
RTDECL(int) RTCrDigestClone(PRTCRDIGEST phDigest, RTCRDIGEST hSrc);
RTDECL(int) RTCrDigestReset(RTCRDIGEST hDigest);
RTDECL(uint32_t) RTCrDigestRetain(RTCRDIGEST hDigest);
RTDECL(uint32_t) RTCrDigestRelease(RTCRDIGEST hDigest);
RTDECL(int) RTCrDigestUpdate(RTCRDIGEST hDigest, void const *pvData, size_t cbData);
RTDECL(int) RTCrDigestUpdateFromVfsFile(RTCRDIGEST hDigest, RTVFSFILE hVfsFile, bool fRewindFile);
RTDECL(int) RTCrDigestFinal(RTCRDIGEST hDigest, void *pvHash, size_t cbHash);
RTDECL(bool) RTCrDigestMatch(RTCRDIGEST hDigest, void const *pvHash, size_t cbHash);
RTDECL(uint8_t const *) RTCrDigestGetHash(RTCRDIGEST hDigest);
RTDECL(uint32_t) RTCrDigestGetHashSize(RTCRDIGEST hDigest);
RTDECL(uint64_t) RTCrDigestGetConsumedSize(RTCRDIGEST hDigest);
RTDECL(bool) RTCrDigestIsFinalized(RTCRDIGEST hDigest);
RTDECL(RTDIGESTTYPE) RTCrDigestGetType(RTCRDIGEST hDigest);
RTDECL(const char *) RTCrDigestGetAlgorithmOid(RTCRDIGEST hDigest);


/**
 * Translates an IPRT digest type value to an OID.
 *
 * @returns Dotted OID string on success, NULL if not translatable.
 * @param       enmDigestType       The IPRT digest type value to convert.
 */
RTDECL(const char *) RTCrDigestTypeToAlgorithmOid(RTDIGESTTYPE enmDigestType);

/**
 * Translates an IPRT digest type value to a name/descriptive string.
 *
 * The purpose here is for human readable output rather than machine readable
 * output, i.e. the names aren't set in stone.
 *
 * @returns Pointer to read-only string, NULL if unknown type.
 * @param       enmDigestType       The IPRT digest type value to convert.
 */
RTDECL(const char *) RTCrDigestTypeToName(RTDIGESTTYPE enmDigestType);

/**
 * Translates an IPRT digest type value to a hash size.
 *
 * @returns Hash size (in bytes).
 * @param       enmDigestType       The IPRT digest type value to convert.
 */
RTDECL(uint32_t) RTCrDigestTypeToHashSize(RTDIGESTTYPE enmDigestType);

/** @} */

RT_C_DECLS_END

#endif

