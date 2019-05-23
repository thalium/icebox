/* $Id: ClientDataInt.h $ */
/** @file
 * Config.h
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

#ifndef __CLIENT_DATA_INT_H__
#define __CLIENT_DATA_INT_H__

class ClientData
{
public:
    ClientData()
    {
        m_address.u = 0;
        m_network.u = 0;
        fHasLease = false;
        fHasClient = false;
        fBinding = true;
        u64TimestampBindingStarted = 0;
        u64TimestampLeasingStarted = 0;
        u32LeaseExpirationPeriod = 0;
        u32BindExpirationPeriod = 0;
        pCfg = NULL;

    }
    ~ClientData(){}

    /* client information */
    RTNETADDRIPV4 m_address;
    RTNETADDRIPV4 m_network;
    RTMAC m_mac;

    bool fHasClient;

    /* Lease part */
    bool fHasLease;
    /** lease isn't commited */
    bool fBinding;

    /** Timestamp when lease commited. */
    uint64_t u64TimestampLeasingStarted;
    /** Period when lease is expired in secs. */
    uint32_t u32LeaseExpirationPeriod;

    /** timestamp when lease was bound */
    uint64_t u64TimestampBindingStarted;
    /* Period when binding is expired in secs. */
    uint32_t u32BindExpirationPeriod;

    MapOptionId2RawOption options;

    NetworkConfigEntity *pCfg;
};

#endif
