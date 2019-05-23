/* $Id: tsp-template.h $ */
/** @file
 * IPRT - Crypto - Time-Stamp Protocol (RFC-3161), Code Generator Template.
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

#define RTASN1TMPL_DECL         RTDECL

/*
 * MessageImprint
 */
#define RTASN1TMPL_TYPE         RTCRTSPMESSAGEIMPRINT
#define RTASN1TMPL_EXT_NAME     RTCrTspMessageImprint
#define RTASN1TMPL_INT_NAME     rtCrTspMessageImprint
RTASN1TMPL_BEGIN_SEQCORE();
RTASN1TMPL_MEMBER(              HashAlgorithm,      RTCRX509ALGORITHMIDENTIFIER,    RTCrX509AlgorithmIdentifier);
RTASN1TMPL_MEMBER(              HashedMessage,      RTASN1OCTETSTRING,              RTAsn1OctetString);
RTASN1TMPL_END_SEQCORE();
#undef RTASN1TMPL_TYPE
#undef RTASN1TMPL_EXT_NAME
#undef RTASN1TMPL_INT_NAME

/*
 * TimeStampReq
 */

/*
 * PKIStatusInfo
 */

/*
 * TimeStampResp
 */

/*
 * Accuracy
 */
#define RTASN1TMPL_TYPE         RTCRTSPACCURACY
#define RTASN1TMPL_EXT_NAME     RTCrTspAccuracy
#define RTASN1TMPL_INT_NAME     rtCrTspAccuracy
RTASN1TMPL_BEGIN_SEQCORE();
RTASN1TMPL_MEMBER_OPT_ITAG_EX(  Seconds, RTASN1INTEGER, RTAsn1Integer, ASN1_TAG_INTEGER, RTASN1TMPL_ITAG_F_UP,
                                RTASN1TMPL_MEMBER_CONSTR_U64_MIN_MAX(Seconds, 0, UINT64_MAX, RT_NOTHING));
/** @todo The Millis and Micros fields makes no sense if Seconds > 1, while the
 *        Micros field makes no sense with Millis > 1.  Add constraints
 *        expressing this dependency. */
RTASN1TMPL_MEMBER_OPT_ITAG_EX(  Millis,     RTASN1INTEGER,  RTAsn1Integer, 0, RTASN1TMPL_ITAG_F_CP,
                                RTASN1TMPL_MEMBER_CONSTR_U64_MIN_MAX(Millis, 1, 999, RT_NOTHING));
RTASN1TMPL_MEMBER_OPT_ITAG_EX(  Micros,     RTASN1INTEGER,  RTAsn1Integer, 1, RTASN1TMPL_ITAG_F_CP,
                                RTASN1TMPL_MEMBER_CONSTR_U64_MIN_MAX(Micros, 1, 999, RT_NOTHING));
RTASN1TMPL_END_SEQCORE();
#undef RTASN1TMPL_TYPE
#undef RTASN1TMPL_EXT_NAME
#undef RTASN1TMPL_INT_NAME


/*
 * TSTInfo
 */
#define RTASN1TMPL_TYPE         RTCRTSPTSTINFO
#define RTASN1TMPL_EXT_NAME     RTCrTspTstInfo
#define RTASN1TMPL_INT_NAME     rtCrTspTstInfo
RTASN1TMPL_BEGIN_SEQCORE();
RTASN1TMPL_MEMBER(              Version,            RTASN1INTEGER,                  RTAsn1Integer);
RTASN1TMPL_MEMBER(              Policy,             RTASN1OBJID,                    RTAsn1ObjId);
RTASN1TMPL_MEMBER(              MessageImprint,     RTCRTSPMESSAGEIMPRINT,          RTCrTspMessageImprint);
RTASN1TMPL_MEMBER(              SerialNumber,       RTASN1INTEGER,                  RTAsn1Integer);
RTASN1TMPL_MEMBER(              GenTime,            RTASN1TIME,                     RTAsn1GeneralizedTime);
RTASN1TMPL_MEMBER_OPT_ITAG_UC(  Accuracy,           RTCRTSPACCURACY,                RTCrTspAccuracy,                ASN1_TAG_SEQUENCE);
RTASN1TMPL_MEMBER_DEF_ITAG_UP(  Ordering,           RTASN1BOOLEAN,                  RTAsn1Boolean,                  ASN1_TAG_BOOLEAN, 0 /*False*/);
RTASN1TMPL_MEMBER_OPT_ITAG_UP(  Nonce,              RTASN1INTEGER,                  RTAsn1Integer,                  ASN1_TAG_INTEGER);
RTASN1TMPL_MEMBER_OPT_XTAG(     T0, CtxTag0, Tsa,   RTCRX509GENERALNAME,            RTCrX509GeneralName,            0);
RTASN1TMPL_MEMBER_OPT_ITAG(     Extensions,         RTCRX509EXTENSION,              RTCrX509Extension,              1);
RTASN1TMPL_END_SEQCORE();
#undef RTASN1TMPL_TYPE
#undef RTASN1TMPL_EXT_NAME
#undef RTASN1TMPL_INT_NAME

