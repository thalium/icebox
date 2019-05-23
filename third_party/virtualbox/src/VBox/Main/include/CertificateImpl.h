/* $Id: CertificateImpl.h $ */
/** @file
 * VirtualBox COM ICertificate implementation.
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
 */

#ifndef ____H_CERTIFICATEIMPL
#define ____H_CERTIFICATEIMPL

/* VBox includes */
#include <iprt/crypto/x509.h>
#include "CertificateWrap.h"

#include <vector>

using namespace std;

/**
 * Implemenation of ICertificate.
 *
 * This implemenation is a very thin wrapper around an immutable
 * RTCRX509CERTIFICATE and a few caller stated views.
 *
 * The views are whether the caller thinks the certificate is trustworthly, and
 * whether the caller thinks it's expired or not.  The caller could be sitting
 * on more information, like timestamp and intermediate certificates, that helps
 * inform the caller's view on these two topics.
 *
 * @remarks It could be helpful to let the caller also add certificate paths
 *          showing how this certificate ends up being trusted.  However, that's
 *          possibly quite some work and will have to wait till required...
 */
class ATL_NO_VTABLE Certificate
    : public CertificateWrap
{

public:

    DECLARE_EMPTY_CTOR_DTOR(Certificate)

    HRESULT initCertificate(PCRTCRX509CERTIFICATE a_pCert, bool a_fTrusted, bool a_fExpired);
    void uninit();

    HRESULT FinalConstruct();
    void FinalRelease();

private:
    // Wrapped ICertificate properties
    HRESULT getVersionNumber(CertificateVersion_T *aVersionNumber);
    HRESULT getSerialNumber(com::Utf8Str &aSerialNumber);
    HRESULT getSignatureAlgorithmOID(com::Utf8Str &aSignatureAlgorithmOID);
    HRESULT getSignatureAlgorithmName(com::Utf8Str &aSignatureAlgorithmName);
    HRESULT getPublicKeyAlgorithmOID(com::Utf8Str &aPublicKeyAlgorithmOID);
    HRESULT getPublicKeyAlgorithm(com::Utf8Str &aPublicKeyAlgorithm);
    HRESULT getIssuerName(std::vector<com::Utf8Str> &aIssuerName);
    HRESULT getSubjectName(std::vector<com::Utf8Str> &aSubjectName);
    HRESULT getFriendlyName(com::Utf8Str &aFriendlyName);
    HRESULT getValidityPeriodNotBefore(com::Utf8Str &aValidityPeriodNotBefore);
    HRESULT getValidityPeriodNotAfter(com::Utf8Str &aValidityPeriodNotAfter);
    HRESULT getSubjectPublicKey(std::vector<BYTE> &aSubjectPublicKey);
    HRESULT getIssuerUniqueIdentifier(com::Utf8Str &aIssuerUniqueIdentifier);
    HRESULT getSubjectUniqueIdentifier(com::Utf8Str &aSubjectUniqueIdentifier);
    HRESULT getCertificateAuthority(BOOL *aCertificateAuthority);
    HRESULT getKeyUsage(ULONG *aKeyUsage);
    HRESULT getExtendedKeyUsage(std::vector<com::Utf8Str> &aExtendedKeyUsage);
    HRESULT getRawCertData(std::vector<BYTE> &aRawCertData);
    HRESULT getSelfSigned(BOOL *aSelfSigned);
    HRESULT getTrusted(BOOL *aTrusted);
    HRESULT getExpired(BOOL *aExpired);

    // Wrapped ICertificate methods
    HRESULT isCurrentlyExpired(BOOL *aResult);
    HRESULT queryInfo(LONG aWhat, com::Utf8Str &aResult);

    // Methods extracting COM data from the certificate object
    HRESULT i_getAlgorithmName(PCRTCRX509ALGORITHMIDENTIFIER a_pAlgId, com::Utf8Str &a_rReturn);
    HRESULT i_getX509Name(PCRTCRX509NAME a_pName, std::vector<com::Utf8Str> &a_rReturn);
    HRESULT i_getTime(PCRTASN1TIME a_pTime, com::Utf8Str &a_rReturn);
    HRESULT i_getUniqueIdentifier(PCRTCRX509UNIQUEIDENTIFIER a_pUniqueId, com::Utf8Str &a_rReturn);
    HRESULT i_getEncodedBytes(PRTASN1CORE a_pAsn1Obj, std::vector<BYTE> &a_rReturn);

    struct Data;
    /** Pointer to the private instance data   */
    Data *m;
};

#endif // !____H_CERTIFICATEIMPL

