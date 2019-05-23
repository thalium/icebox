/* $Id: mp.h $ */
/** @file
 * IPRT - Internal RTMp header
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
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

#ifndef ___internal_mp_h
#define ___internal_mp_h

#include <iprt/assert.h>
#include <iprt/mp.h>

RT_C_DECLS_BEGIN


#ifdef RT_OS_WINDOWS
/** @todo Return the processor group + number instead.
 * Unfortunately, DTrace and HM makes the impossible for the time being as it
 * seems to be making the stupid assumption that idCpu == iCpuSet. */
#if 0
# define IPRT_WITH_RTCPUID_AS_GROUP_AND_NUMBER
#endif

# ifdef IPRT_WITH_RTCPUID_AS_GROUP_AND_NUMBER

/** @def RTMPCPUID_FROM_GROUP_AND_NUMBER
 * Creates the RTCPUID value.
 *
 * @remarks We Increment a_uGroup by 1 to make sure the ID is never the same as
 *          the CPU set index.
 *
 * @remarks We put the group in the top to make it easy to construct the MAX ID.
 *          For that reason we also just use 8 bits for the processor number, as
 *          it keeps the range small.
 */
#  define RTMPCPUID_FROM_GROUP_AND_NUMBER(a_uGroup, a_uGroupMember)  \
    ( (uint8_t)(a_uGroupMember) | (((uint32_t)(a_uGroup) + 1) << 8) )

/** Extracts the group number from a RTCPUID value.  */
DECLINLINE(uint16_t) rtMpCpuIdGetGroup(RTCPUID idCpu)
{
    Assert(idCpu != NIL_RTCPUID);
    uint16_t idxGroup = idCpu >> 8;
    Assert(idxGroup != 0);
    return idxGroup - 1;
}

/** Extracts the group member number from a RTCPUID value.   */
DECLINLINE(uint8_t) rtMpCpuIdGetGroupMember(RTCPUID idCpu)
{
    Assert(idCpu != NIL_RTCPUID);
    return (uint8_t)idCpu;
}

# endif /* IPRT_WITH_RTCPUID_AS_GROUP_AND_NUMBER */
#endif /* RT_OS_WINDOWS */


RT_C_DECLS_END

#endif

