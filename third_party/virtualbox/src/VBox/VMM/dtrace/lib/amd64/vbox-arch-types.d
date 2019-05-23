/** @file
 * VBox & DTrace - Types and Constants for AMD64.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


inline unsigned HC_ARCH_BITS = 64;
typedef uint64_t    RTR3PTR;
typedef void       *RTR0PTR;
typedef uint64_t    RTHCPTR;



typedef union RTFLOAT80U
{
    uint16_t    au16[5];
} RTFLOAT80U;

typedef union RTFLOAT80U2
{
    uint16_t    au16[5];
} RTFLOAT80U2;

typedef struct uint128_t
{
    uint64_t    au64[2];
} uint128_t;

