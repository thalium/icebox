/* $Id: ctl.h $ */
/** @file
 * NAT - IP subnet constants.
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

#ifndef _SLIRP_CTL_H_
#define _SLIRP_CTL_H_

#define CTL_CMD         0
#define CTL_EXEC        1
#define CTL_ALIAS       2
#define CTL_DNS         3
#define CTL_TFTP        4
#define CTL_GUEST       15
#define CTL_BROADCAST   255


#define CTL_CHECK_NETWORK(x) (((x) & RT_H2N_U32(pData->netmask)) == pData->special_addr.s_addr)

#define CTL_CHECK(x, ctl) (   ((RT_N2H_U32((x)) & ~pData->netmask) == (ctl)) \
                           && CTL_CHECK_NETWORK(x))

#define CTL_CHECK_MINE(x) (   CTL_CHECK(x, CTL_ALIAS)      \
                           || CTL_CHECK(x, CTL_DNS)        \
                           || CTL_CHECK(x, CTL_TFTP))

#define CTL_CHECK_BROADCAST(x) CTL_CHECK((x), ~pData->netmask)


#endif /* _SLIRP_CTL_H_ */
