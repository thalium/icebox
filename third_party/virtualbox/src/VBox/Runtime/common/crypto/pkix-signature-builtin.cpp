/* $Id: pkix-signature-builtin.cpp $ */
/** @file
 * IPRT - Crypto - Public Key Signature Schemas, Built-in providers.
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

#ifdef IPRT_WITH_OPENSSL
# include "internal/iprt-openssl.h"
# include <openssl/evp.h>
#endif

#include "pkix-signature-builtin.h"


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Array of built in message digest vtables.
 */
static PCRTCRPKIXSIGNATUREDESC const g_apPkixSignatureDescriptors[] =
{
    &g_rtCrPkixSigningHashWithRsaDesc,
};



PCRTCRPKIXSIGNATUREDESC RTCrPkixSignatureFindByObjIdString(const char *pszObjId, void **ppvOpaque)
{
    if (ppvOpaque)
        *ppvOpaque = NULL;

    /*
     * Primary OIDs.
     */
    uint32_t i = RT_ELEMENTS(g_apPkixSignatureDescriptors);
    while (i-- > 0)
        if (strcmp(g_apPkixSignatureDescriptors[i]->pszObjId, pszObjId) == 0)
            return g_apPkixSignatureDescriptors[i];

    /*
     * Alias OIDs.
     */
    i = RT_ELEMENTS(g_apPkixSignatureDescriptors);
    while (i-- > 0)
    {
        const char * const *ppszAliases = g_apPkixSignatureDescriptors[i]->papszObjIdAliases;
        if (ppszAliases)
            for (; *ppszAliases; ppszAliases++)
                if (strcmp(*ppszAliases, pszObjId) == 0)
                    return g_apPkixSignatureDescriptors[i];
    }

#if 0//def IPRT_WITH_OPENSSL
    /*
     * Try EVP and see if it knows the algorithm.
     */
    if (ppvOpaque)
    {
        rtCrOpenSslInit();
        int iAlgoNid = OBJ_txt2nid(pszObjId);
        if (iAlgoNid != NID_undef)
        {
            const char *pszAlogSn = OBJ_nid2sn(iAlgoNid);
            const EVP_MD *pEvpMdType = EVP_get_digestbyname(pszAlogSn);
            if (pEvpMdType)
            {
                /*
                 * Return the OpenSSL provider descriptor and the EVP_MD address.
                 */
                Assert(pEvpMdType->md_size);
                *ppvOpaque = (void *)pEvpMdType;
                return &g_rtCrPkixSignatureOpenSslDesc;
            }
        }
    }
#endif
    return NULL;
}


PCRTCRPKIXSIGNATUREDESC RTCrPkixSignatureFindByObjId(PCRTASN1OBJID pObjId, void **ppvOpaque)
{
    return RTCrPkixSignatureFindByObjIdString(pObjId->szObjId, ppvOpaque);
}


RTDECL(int) RTCrPkixSignatureCreateByObjIdString(PRTCRPKIXSIGNATURE phSignature, const char *pszObjId, bool fSigning,
                                                 PCRTASN1BITSTRING pKey,PCRTASN1DYNTYPE pParams)
{
    void *pvOpaque;
    PCRTCRPKIXSIGNATUREDESC pDesc = RTCrPkixSignatureFindByObjIdString(pszObjId, &pvOpaque);
    if (pDesc)
        return RTCrPkixSignatureCreate(phSignature, pDesc, pvOpaque, fSigning, pKey, pParams);
    return VERR_NOT_FOUND;
}


RTDECL(int) RTCrPkixSignatureCreateByObjId(PRTCRPKIXSIGNATURE phSignature, PCRTASN1OBJID pObjId, bool fSigning,
                                           PCRTASN1BITSTRING pKey, PCRTASN1DYNTYPE pParams)
{
    void *pvOpaque;
    PCRTCRPKIXSIGNATUREDESC pDesc = RTCrPkixSignatureFindByObjId(pObjId, &pvOpaque);
    if (pDesc)
        return RTCrPkixSignatureCreate(phSignature, pDesc, pvOpaque, fSigning, pKey, pParams);
    return VERR_NOT_FOUND;
}

