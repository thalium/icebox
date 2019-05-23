/* $Id: NetworkManagerDhcp.cpp $ */
/** @file
 * NetworkManagerDhcp - Network Manager part handling Dhcp.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/asm.h>
#include <iprt/cdefs.h>
#include <iprt/getopt.h>
#include <iprt/net.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/time.h>
#include <iprt/string.h>

#include "../NetLib/shared_ptr.h"

#include <vector>
#include <list>
#include <string>
#include <map>

#include <VBox/sup.h>
#include <VBox/intnet.h>

#define BASE_SERVICES_ONLY
#include "../NetLib/VBoxNetBaseService.h"
#include "Config.h"
#include "ClientDataInt.h"

/**
 * The client is requesting an offer.
 *
 * @returns true.
 *
 * @param   pDhcpMsg    The message.
 * @param   cb          The message size.
 */
bool NetworkManager::handleDhcpReqDiscover(PCRTNETBOOTP pDhcpMsg, size_t cb)
{
    RawOption opt;
    RT_ZERO(opt);

    /* 1. Find client */
    ConfigurationManager *confManager = ConfigurationManager::getConfigurationManager();
    Client client = confManager->getClientByDhcpPacket(pDhcpMsg, cb);

    /* 2. Find/Bind lease for client */
    Lease lease = confManager->allocateLease4Client(client, pDhcpMsg, cb);
    AssertReturn(lease != Lease::NullLease, VINF_SUCCESS);

    int rc = ConfigurationManager::extractRequestList(pDhcpMsg, cb, opt);
    NOREF(rc); /** @todo check */

    /* 3. Send of offer */

    lease.bindingPhase(true);
    lease.phaseStart(RTTimeMilliTS());
    lease.setExpiration(300); /* 3 min. */
    offer4Client(client, pDhcpMsg->bp_xid, opt.au8RawOpt, opt.cbRawOpt);

    return true;
}


/**
 * The client is requesting an offer.
 *
 * @returns true.
 *
 * @param   pDhcpMsg    The message.
 * @param   cb          The message size.
 */
bool NetworkManager::handleDhcpReqRequest(PCRTNETBOOTP pDhcpMsg, size_t cb)
{
    ConfigurationManager *confManager = ConfigurationManager::getConfigurationManager();

    /* 1. find client */
    Client client = confManager->getClientByDhcpPacket(pDhcpMsg, cb);

    /* 2. find bound lease */
    Lease l = client.lease();
    if (l != Lease::NullLease)
    {

        if (l.isExpired())
        {
            /* send client to INIT state */
            Client c(client);
            nak(client, pDhcpMsg->bp_xid);
            confManager->expireLease4Client(c);
            return true;
        }
        /* XXX: Validate request */
        RawOption opt;
        RT_ZERO(opt);

        Client c(client);
        int rc = confManager->commitLease4Client(c);
        AssertRCReturn(rc, false);

        rc = ConfigurationManager::extractRequestList(pDhcpMsg, cb, opt);
        AssertRCReturn(rc, false);

        ack(client, pDhcpMsg->bp_xid, opt.au8RawOpt, opt.cbRawOpt);
    }
    else
    {
        nak(client, pDhcpMsg->bp_xid);
    }
    return true;
}


/**
 * The client is declining an offer we've made.
 *
 * @returns true.
 *
 * @param   pDhcpMsg    The message.
 * @param   cb          The message size.
 */
bool NetworkManager::handleDhcpReqDecline(PCRTNETBOOTP, size_t)
{
    /** @todo Probably need to match the server IP here to work correctly with
     *        other servers. */

    /*
     * The client is supposed to pass us option 50, requested address,
     * from the offer. We also match the lease state. Apparently the
     * MAC address is not supposed to be checked here.
     */

    /** @todo this is not required in the initial implementation, do it later. */
    return true;
}


/**
 * The client is releasing its lease - good boy.
 *
 * @returns true.
 *
 * @param   pDhcpMsg    The message.
 * @param   cb          The message size.
 */
bool NetworkManager::handleDhcpReqRelease(PCRTNETBOOTP, size_t)
{
    /** @todo Probably need to match the server IP here to work correctly with
     *        other servers. */

    /*
     * The client may pass us option 61, client identifier, which we should
     * use to find the lease by.
     *
     * We're matching MAC address and lease state as well.
     */

    /*
     * If no client identifier or if we couldn't find a lease by using it,
     * we will try look it up by the client IP address.
     */


    /*
     * If found, release it.
     */


    /** @todo this is not required in the initial implementation, do it later. */
    return true;
}

