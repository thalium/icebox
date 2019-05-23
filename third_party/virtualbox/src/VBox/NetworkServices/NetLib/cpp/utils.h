/* $Id: utils.h $ */
/** @file
 * NetLib/cpp/utils.h
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___NETLIB_CPP_UTILS_H___
#define ___NETLIB_CPP_UTILS_H___

#include <iprt/types.h>

/** less operator for IPv4 addresess */
DECLINLINE(bool) operator <(const RTNETADDRIPV4 &lhs, const RTNETADDRIPV4 &rhs)
{
    return RT_N2H_U32(lhs.u) < RT_N2H_U32(rhs.u);
}

/** greater operator for IPv4 addresess */
DECLINLINE(bool) operator >(const RTNETADDRIPV4 &lhs, const RTNETADDRIPV4 &rhs)
{
    return RT_N2H_U32(lhs.u) > RT_N2H_U32(rhs.u);
}

/**  Compares MAC addresses */
DECLINLINE(bool) operator== (const RTMAC &lhs, const RTMAC &rhs)
{
    return lhs.au16[0] == rhs.au16[0]
        && lhs.au16[1] == rhs.au16[1]
        && lhs.au16[2] == rhs.au16[2];
}

#endif

