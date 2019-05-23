/* $Id: store.cpp $ */
/** @file
 * IPRT - Cryptographic (Certificate) Store.
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

#include <iprt/asm.h>
#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/string.h>

#include <iprt/crypto/x509.h>

#ifdef IPRT_WITH_OPENSSL
# include <openssl/x509.h>
#endif

#include "store-internal.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Internal representation of a (certificate,++) store.
 */
typedef struct RTCRSTOREINT
{
    /** Magic number (RTCRSTOREINT_MAGIC). */
    uint32_t                    u32Magic;
    /** Reference counter. */
    uint32_t volatile           cRefs;
    /** Pointer to the store provider. */
    PCRTCRSTOREPROVIDER         pProvider;
    /** Provider specific data. */
    void                       *pvProvider;
} RTCRSTOREINT;
/** Pointer to the internal representation of a store. */
typedef RTCRSTOREINT *PRTCRSTOREINT;

/** Magic value for RTCRSTOREINT::u32Magic (Alfred Dillwyn "Dilly" Knox). */
#define RTCRSTOREINT_MAGIC          UINT32_C(0x18840723)
/** Dead magic value for RTCRSTOREINT::u32Magic. */
#define RTCRSTOREINT_MAGIC_DEAD     UINT32_C(0x19430227)



/**
 * Internal method a store provider uses to create a store handle.
 *
 * @returns IPRT status code
 * @param   pProvider       Pointer to the store provider callback table.
 * @param   pvProvider      Pointer to the provider specific instance data.
 * @param   phStore         Where to return the store handle.
 */
DECLHIDDEN(int) rtCrStoreCreate(PCRTCRSTOREPROVIDER pProvider, void *pvProvider, PRTCRSTORE phStore)
{
    PRTCRSTOREINT pThis = (PRTCRSTOREINT)RTMemAlloc(sizeof(*pThis));
    if (pThis)
    {
        pThis->pvProvider   = pvProvider;
        pThis->pProvider    = pProvider;
        pThis->cRefs        = 1;
        pThis->u32Magic     = RTCRSTOREINT_MAGIC;
        *phStore = pThis;
        return VINF_SUCCESS;
    }
    return VERR_NO_MEMORY;
}



RTDECL(uint32_t) RTCrStoreRetain(RTCRSTORE hStore)
{
    PRTCRSTOREINT pThis = (PRTCRSTOREINT)hStore;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->u32Magic == RTCRSTOREINT_MAGIC, UINT32_MAX);

    uint32_t cRet = ASMAtomicIncU32(&pThis->cRefs);
    Assert(cRet < 8192);
    return cRet;
}


RTDECL(uint32_t) RTCrStoreRelease(RTCRSTORE hStore)
{
    if (hStore == NIL_RTCRSTORE)
        return 0;

    PRTCRSTOREINT pThis = (PRTCRSTOREINT)hStore;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->u32Magic == RTCRSTOREINT_MAGIC, UINT32_MAX);

    uint32_t cStore = ASMAtomicDecU32(&pThis->cRefs);
    if (!cStore)
    {
        ASMAtomicWriteU32(&pThis->u32Magic, RTCRSTOREINT_MAGIC_DEAD);
        pThis->pProvider->pfnDestroyStore(pThis->pvProvider);
        RTMemFree(pThis);
    }
    return cStore;
}


RTDECL(PCRTCRCERTCTX) RTCrStoreCertByIssuerAndSerialNo(RTCRSTORE hStore, PCRTCRX509NAME pIssuer, PCRTASN1INTEGER pSerialNo)
{
    PRTCRSTOREINT pThis = (PRTCRSTOREINT)hStore;
    AssertPtrReturn(pThis, NULL);
    AssertReturn(pThis->u32Magic == RTCRSTOREINT_MAGIC, NULL);
    AssertPtrReturn(pIssuer, NULL);

    int rc;
    RTCRSTORECERTSEARCH Search;
    if (pThis->pProvider->pfnCertFindByIssuerAndSerialNo)
        rc = pThis->pProvider->pfnCertFindByIssuerAndSerialNo(pThis->pvProvider, pIssuer, pSerialNo, &Search);
    else
        rc = pThis->pProvider->pfnCertFindAll(pThis->pvProvider, &Search);

    PCRTCRCERTCTX pCertCtx = NULL;
    if (RT_SUCCESS(rc))
    {
        for (;;)
        {
            pCertCtx = pThis->pProvider->pfnCertSearchNext(pThis->pvProvider, &Search);
            if (!pCertCtx)
                break;

            if (   pCertCtx->pCert
                && RTCrX509Certificate_MatchIssuerAndSerialNumber(pCertCtx->pCert, pIssuer, pSerialNo))
                break;
            RTCrCertCtxRelease(pCertCtx);
        }

        pThis->pProvider->pfnCertSearchDestroy(pThis->pvProvider, &Search);
    }
    else
        AssertMsg(rc == VERR_NOT_FOUND, ("%Rrc\n", rc));
    return pCertCtx;
}


RTDECL(int) RTCrStoreCertAddEncoded(RTCRSTORE hStore, uint32_t fFlags, void const *pvSrc, size_t cbSrc, PRTERRINFO pErrInfo)
{
    PRTCRSTOREINT pThis = (PRTCRSTOREINT)hStore;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTCRSTOREINT_MAGIC, VERR_INVALID_HANDLE);
    AssertPtrReturn(pvSrc, VERR_INVALID_POINTER);
    AssertReturn(cbSrc > 16 && cbSrc < _1M, VERR_OUT_OF_RANGE);
    AssertReturn(!(fFlags & ~(RTCRCERTCTX_F_ADD_IF_NOT_FOUND | RTCRCERTCTX_F_ENC_MASK)), VERR_INVALID_FLAGS);
    AssertMsgReturn(   (fFlags & RTCRCERTCTX_F_ENC_MASK) == RTCRCERTCTX_F_ENC_X509_DER
                    || (fFlags & RTCRCERTCTX_F_ENC_MASK) == RTCRCERTCTX_F_ENC_TAF_DER
                    , ("Only X.509 and TAF DER supported: %#x\n", fFlags), VERR_INVALID_FLAGS);

    int rc;
    if (pThis->pProvider->pfnCertAddEncoded)
        rc = pThis->pProvider->pfnCertAddEncoded(pThis->pvProvider, fFlags, (uint8_t const *)pvSrc, (uint32_t)cbSrc, pErrInfo);
    else
        rc = VERR_WRITE_PROTECT;

    return rc;
}



/*
 * Searching.
 * Searching.
 * Searching.
 */

RTDECL(int) RTCrStoreCertFindAll(RTCRSTORE hStore, PRTCRSTORECERTSEARCH pSearch)
{
    PRTCRSTOREINT pThis = (PRTCRSTOREINT)hStore;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTCRSTOREINT_MAGIC, VERR_INVALID_HANDLE);
    AssertPtrReturn(pSearch, VERR_INVALID_POINTER);

    return pThis->pProvider->pfnCertFindAll(pThis->pvProvider, pSearch);
}


/** Indicator for RTCrStoreCertFindBySubjectOrAltSubjectByRfc5280 searches
 *  implemented by this front-end code. */
#define RTCRSTORECERTSEARCH_BY_SUBECT_OR_ALT_SUBJECT_BY_RFC5280     UINT32_C(0x5be9145d)

RTDECL(int) RTCrStoreCertFindBySubjectOrAltSubjectByRfc5280(RTCRSTORE hStore, PCRTCRX509NAME pSubject,
                                                            PRTCRSTORECERTSEARCH pSearch)
{
    PRTCRSTOREINT pThis = (PRTCRSTOREINT)hStore;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTCRSTOREINT_MAGIC, VERR_INVALID_HANDLE);
    AssertPtrReturn(pSearch, VERR_INVALID_POINTER);

    int rc = pThis->pProvider->pfnCertFindAll(pThis->pvProvider, pSearch);
    if (RT_SUCCESS(rc))
    {
        pSearch->auOpaque[2] = RTCRSTORECERTSEARCH_BY_SUBECT_OR_ALT_SUBJECT_BY_RFC5280;
        pSearch->auOpaque[3] = (uintptr_t)pSubject;
    }
    return rc;
}


RTDECL(PCRTCRCERTCTX) RTCrStoreCertSearchNext(RTCRSTORE hStore, PRTCRSTORECERTSEARCH pSearch)
{
    PRTCRSTOREINT pThis = (PRTCRSTOREINT)hStore;
    AssertPtrReturn(pThis, NULL);
    AssertReturn(pThis->u32Magic == RTCRSTOREINT_MAGIC, NULL);
    AssertPtrReturn(pSearch, NULL);

    PCRTCRCERTCTX pRet;
    switch (pSearch->auOpaque[2])
    {
        default:
            pRet = pThis->pProvider->pfnCertSearchNext(pThis->pvProvider, pSearch);
            break;

        case RTCRSTORECERTSEARCH_BY_SUBECT_OR_ALT_SUBJECT_BY_RFC5280:
        {
            PCRTCRX509NAME pSubject = (PCRTCRX509NAME)pSearch->auOpaque[3];
            AssertPtrReturn(pSubject, NULL);

            for (;;)
            {
                pRet = pThis->pProvider->pfnCertSearchNext(pThis->pvProvider, pSearch);
                if (!pRet)
                    break;
                if (pRet->pCert)
                {
                    if (RTCrX509Certificate_MatchSubjectOrAltSubjectByRfc5280(pRet->pCert, pSubject))
                        break;
                }
                else if (pRet->pTaInfo)
                {
                    if (   RTCrTafCertPathControls_IsPresent(&pRet->pTaInfo->CertPath)
                        && RTCrX509Name_MatchByRfc5280(&pRet->pTaInfo->CertPath.TaName, pSubject))
                        break;
                }
                RTCrCertCtxRelease(pRet);
            }
            break;
        }
    }
    return pRet;
}


RTDECL(int) RTCrStoreCertSearchDestroy(RTCRSTORE hStore, PRTCRSTORECERTSEARCH pSearch)
{
    PRTCRSTOREINT pThis = (PRTCRSTOREINT)hStore;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTCRSTOREINT_MAGIC, VERR_INVALID_HANDLE);
    if (pSearch)
    {
        AssertPtrReturn(pSearch, VERR_INVALID_POINTER);
        pThis->pProvider->pfnCertSearchDestroy(pThis->pvProvider, pSearch);
    }
    return VINF_SUCCESS;
}



RTDECL(uint32_t) RTCrStoreCertCount(RTCRSTORE hStore)
{
    PRTCRSTOREINT pThis = (PRTCRSTOREINT)hStore;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->u32Magic == RTCRSTOREINT_MAGIC, UINT32_MAX);

    RTCRSTORECERTSEARCH Search;
    int rc = pThis->pProvider->pfnCertFindAll(pThis->pvProvider, &Search);
    AssertRCReturn(rc, UINT32_MAX);


    uint32_t cCerts = 0;
    PCRTCRCERTCTX pCur;
    while ((pCur = pThis->pProvider->pfnCertSearchNext(pThis->pvProvider, &Search)) != NULL)
    {
        RTCrCertCtxRelease(pCur);
        cCerts++;
    }

    return cCerts;
}


#ifdef IPRT_WITH_OPENSSL
/*
 * OpenSSL helper.
 * OpenSSL helper.
 * OpenSSL helper.
 */

RTDECL(int) RTCrStoreConvertToOpenSslCertStore(RTCRSTORE hStore, uint32_t fFlags, void **ppvOpenSslStore)
{
    PRTCRSTOREINT pThis = (PRTCRSTOREINT)hStore;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTCRSTOREINT_MAGIC, VERR_INVALID_HANDLE);
    RT_NOREF_PV(fFlags);

    /*
     * Use the pfnCertFindAll method to add all certificates to the store we're returning.
     */
    int rc;
    X509_STORE *pOsslStore = X509_STORE_new();
    if (pOsslStore)
    {
        RTCRSTORECERTSEARCH Search;
        rc = pThis->pProvider->pfnCertFindAll(pThis->pvProvider, &Search);
        if (RT_SUCCESS(rc))
        {
            do
            {
                PCRTCRCERTCTX pCertCtx = pThis->pProvider->pfnCertSearchNext(pThis->pvProvider, &Search);
                if (!pCertCtx)
                    break;

                if (pCertCtx->fFlags & RTCRCERTCTX_F_ENC_X509_DER)
                {
                    X509 *pOsslCert = NULL;
                    const unsigned char *pabEncoded = (const unsigned char *)pCertCtx->pabEncoded;
                    if (d2i_X509(&pOsslCert, &pabEncoded, pCertCtx->cbEncoded) == pOsslCert)
                    {
                        if (!X509_STORE_add_cert(pOsslStore, pOsslCert))
                            rc = VERR_NO_MEMORY;
                        X509_free(pOsslCert);
                    }
                }

                RTCrCertCtxRelease(pCertCtx);
            } while (RT_SUCCESS(rc));

            pThis->pProvider->pfnCertSearchDestroy(pThis->pvProvider, &Search);
            if (RT_SUCCESS(rc))
            {
                *ppvOpenSslStore = pOsslStore;
                return VINF_SUCCESS;
            }
        }
        X509_STORE_free(pOsslStore);
    }
    else
        rc = VERR_NO_MEMORY;
    return rc;
}


RTDECL(int) RTCrStoreConvertToOpenSslCertStack(RTCRSTORE hStore, uint32_t fFlags, void **ppvOpenSslStack)
{
    PRTCRSTOREINT pThis = (PRTCRSTOREINT)hStore;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTCRSTOREINT_MAGIC, VERR_INVALID_HANDLE);
    RT_NOREF_PV(fFlags);

    /*
     * Use the pfnCertFindAll method to add all certificates to the store we're returning.
     */
    int rc;
    STACK_OF(X509) *pOsslStack = sk_X509_new_null();
    if (pOsslStack)
    {
        RTCRSTORECERTSEARCH Search;
        rc = pThis->pProvider->pfnCertFindAll(pThis->pvProvider, &Search);
        if (RT_SUCCESS(rc))
        {
            do
            {
                PCRTCRCERTCTX pCertCtx = pThis->pProvider->pfnCertSearchNext(pThis->pvProvider, &Search);
                if (!pCertCtx)
                    break;

                if (pCertCtx->fFlags & RTCRCERTCTX_F_ENC_X509_DER)
                {
                    X509 *pOsslCert = NULL;
                    const unsigned char *pabEncoded = (const unsigned char *)pCertCtx->pabEncoded;
                    if (d2i_X509(&pOsslCert, &pabEncoded, pCertCtx->cbEncoded) == pOsslCert)
                    {
                        if (!sk_X509_push(pOsslStack, pOsslCert))
                        {
                            rc = VERR_NO_MEMORY;
                            X509_free(pOsslCert);
                        }
                    }
                }

                RTCrCertCtxRelease(pCertCtx);
            } while (RT_SUCCESS(rc));

            pThis->pProvider->pfnCertSearchDestroy(pThis->pvProvider, &Search);
            if (RT_SUCCESS(rc))
            {
                *ppvOpenSslStack = pOsslStack;
                return VINF_SUCCESS;
            }
        }
        sk_X509_pop_free(pOsslStack, X509_free);
    }
    else
        rc = VERR_NO_MEMORY;
    return rc;
}

#endif /* IPRT_WITH_OPENSSL */


/*
 * Certificate context.
 * Certificate context.
 * Certificate context.
 */


RTDECL(uint32_t) RTCrCertCtxRetain(PCRTCRCERTCTX pCertCtx)
{
    AssertPtrReturn(pCertCtx, UINT32_MAX);
    PRTCRCERTCTXINT pThis = RT_FROM_MEMBER(pCertCtx, RTCRCERTCTXINT, Public);
    AssertReturn(pThis->u32Magic == RTCRCERTCTXINT_MAGIC, UINT32_MAX);
    uint32_t cRet = ASMAtomicIncU32(&pThis->cRefs);
    Assert(cRet < 64);
    return cRet;
}


RTDECL(uint32_t) RTCrCertCtxRelease(PCRTCRCERTCTX pCertCtx)
{
    if (!pCertCtx)
        return 0;

    AssertPtrReturn(pCertCtx, UINT32_MAX);
    PRTCRCERTCTXINT pThis = RT_FROM_MEMBER(pCertCtx, RTCRCERTCTXINT, Public);
    AssertReturn(pThis->u32Magic == RTCRCERTCTXINT_MAGIC, UINT32_MAX);
    uint32_t cRet = ASMAtomicDecU32(&pThis->cRefs);
    if (!cRet)
    {
        ASMAtomicWriteU32(&pThis->u32Magic, RTCRCERTCTXINT_MAGIC_DEAD);
        pThis->pfnDtor(pThis);
    }
    return cRet;
}

