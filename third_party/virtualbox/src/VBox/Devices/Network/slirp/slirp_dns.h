/* $Id: slirp_dns.h $ */
/** @file
 * NAT - Slirp's dns header.
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
#ifndef _SLIRP_DNS_H_
#define _SLIRP_DNS_H_
int slirpInitializeDnsSettings(PNATState pData);
int slirpReleaseDnsSettings(PNATState pData);
#endif

