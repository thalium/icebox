/* $Id: tstRTR0Timer.h $ */
/** @file
 * IPRT R0 Testcase - Timers, common header.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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


#include "tstRTR0CommonReq.h"

#ifdef IN_RING0
RT_C_DECLS_BEGIN
DECLEXPORT(int) TSTRTR0TimerSrvReqHandler(PSUPDRVSESSION pSession, uint32_t uOperation,
                                          uint64_t u64Arg, PSUPR0SERVICEREQHDR pReqHdr);
RT_C_DECLS_END
#endif

typedef enum TSTRTR0TIMER
{
    TSTRTR0TIMER_ONE_SHOT_BASIC = RTTSTR0REQ_FIRST_USER,
    TSTRTR0TIMER_ONE_SHOT_BASIC_HIRES,
    TSTRTR0TIMER_PERIODIC_BASIC,
    TSTRTR0TIMER_PERIODIC_BASIC_HIRES,
    TSTRTR0TIMER_ONE_SHOT_RESTART,
    TSTRTR0TIMER_ONE_SHOT_RESTART_HIRES,
    TSTRTR0TIMER_ONE_SHOT_DESTROY,
    TSTRTR0TIMER_ONE_SHOT_DESTROY_HIRES,
    TSTRTR0TIMER_ONE_SHOT_SPECIFIC,
    TSTRTR0TIMER_ONE_SHOT_SPECIFIC_HIRES,
    TSTRTR0TIMER_PERIODIC_CSSD_LOOPS,
    TSTRTR0TIMER_PERIODIC_CSSD_LOOPS_HIRES,
    TSTRTR0TIMER_PERIODIC_CHANGE_INTERVAL,
    TSTRTR0TIMER_PERIODIC_CHANGE_INTERVAL_HIRES,
    TSTRTR0TIMER_PERIODIC_SPECIFIC,
    TSTRTR0TIMER_PERIODIC_SPECIFIC_HIRES,
    TSTRTR0TIMER_PERIODIC_OMNI,
    TSTRTR0TIMER_PERIODIC_OMNI_HIRES,
    TSTRTR0TIMER_LATENCY_OMNI,
    TSTRTR0TIMER_LATENCY_OMNI_HIRES,
    TSTRTR0TIMER_END
} TSTRTR0TIMER;

/** Check if the operation is for a high resolution timer or not.  */
#define TSTRTR0TIMER_IS_HIRES(uOperation) \
    (   (uOperation) == TSTRTR0TIMER_ONE_SHOT_BASIC_HIRES \
     || (uOperation) == TSTRTR0TIMER_ONE_SHOT_RESTART_HIRES \
     || (uOperation) == TSTRTR0TIMER_ONE_SHOT_DESTROY_HIRES \
     || (uOperation) == TSTRTR0TIMER_ONE_SHOT_SPECIFIC_HIRES \
     || (uOperation) == TSTRTR0TIMER_PERIODIC_BASIC_HIRES \
     || (uOperation) == TSTRTR0TIMER_PERIODIC_CSSD_LOOPS_HIRES \
     || (uOperation) == TSTRTR0TIMER_PERIODIC_CHANGE_INTERVAL_HIRES \
     || (uOperation) == TSTRTR0TIMER_PERIODIC_SPECIFIC_HIRES \
     || (uOperation) == TSTRTR0TIMER_PERIODIC_OMNI_HIRES \
     || (uOperation) == TSTRTR0TIMER_LATENCY_OMNI_HIRES \
    )

