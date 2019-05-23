/* $Id: tstRTR0CommonReq.h $ */
/** @file
 * IPRT R0 Testcase - Common header defining the request packet.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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


#ifndef ___testcase_tstRTR0CommonReq_h
#define ___testcase_tstRTR0CommonReq_h

#include <VBox/sup.h>

/**
 * Ring-0 test request packet.
 */
typedef struct RTTSTR0REQ
{
    SUPR0SERVICEREQHDR  Hdr;
    /** The message (output). */
    char                szMsg[2048];
} RTTSTR0REQ;
/** Pointer to a test request packet. */
typedef RTTSTR0REQ *PRTTSTR0REQ;


/** @name Standard requests.
 * @{ */
/** Positive sanity check. */
#define RTTSTR0REQ_SANITY_OK        1
/** Negative sanity check. */
#define RTTSTR0REQ_SANITY_FAILURE   2
/** The first user request.  */
#define RTTSTR0REQ_FIRST_USER       10
/** @}  */

#endif

