/* $Id: VBoxNetSend.h $ */
/** @file
 * A place to share code and definitions between VBoxNetAdp and VBoxNetFlt host drivers.
 */

/*
 * Copyright (C) 2014-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/** @todo move this to src/VBox/HostDrivers/darwin as a .cpp file. */
#ifndef ___VBox_VBoxNetSend_h
#define ___VBox_VBoxNetSend_h

#if defined(RT_OS_DARWIN)

# include <iprt/err.h>
# include <iprt/assert.h>
# include <iprt/string.h>

# include <sys/socket.h>
# include <net/kpi_interface.h>
RT_C_DECLS_BEGIN /* Buggy 10.4 headers, fixed in 10.5. */
# include <sys/kpi_mbuf.h>
RT_C_DECLS_END
# include <net/if.h>

RT_C_DECLS_BEGIN

# if defined(IN_RING0)

/**
 * Constructs and submits a dummy packet to ifnet_input().
 *
 * This is a workaround for "stuck dock icon" issue. When the first packet goes
 * through the interface DLIL grabs a reference to the thread that submits the
 * packet and holds it until the interface is destroyed. By submitting this
 * dummy we make DLIL grab the thread of a non-GUI process.
 *
 * Most of this function was copied from vboxNetFltDarwinMBufFromSG().
 *
 * @returns VBox status code.
 * @param   pIfNet      The interface that will hold the reference to the calling
 *                      thread. We submit dummy as if it was coming from this interface.
 */
DECLINLINE(int) VBoxNetSendDummy(ifnet_t pIfNet)
{
    int rc = VINF_SUCCESS;

    size_t      cbTotal = 50; /* No Ethernet header */
    mbuf_how_t  How     = MBUF_WAITOK;
    mbuf_t      pPkt    = NULL;
    errno_t err = mbuf_allocpacket(How, cbTotal, NULL, &pPkt);
    if (!err)
    {
        /* Skip zero sized memory buffers (paranoia). */
        mbuf_t pCur = pPkt;
        while (pCur && !mbuf_maxlen(pCur))
            pCur = mbuf_next(pCur);
        Assert(pCur);

        /* Set the required packet header attributes. */
        mbuf_pkthdr_setlen(pPkt, cbTotal);
        mbuf_pkthdr_setheader(pPkt, mbuf_data(pCur));

        mbuf_setlen(pCur, cbTotal);
        memset(mbuf_data(pCur), 0, cbTotal);

        mbuf_pkthdr_setrcvif(pPkt, pIfNet); /* will crash without this. */

        err = ifnet_input(pIfNet, pPkt, NULL);
        if (err)
        {
            rc = RTErrConvertFromErrno(err);
            mbuf_freem(pPkt);
        }
    }
    else
        rc = RTErrConvertFromErrno(err);

    return rc;
}

# endif /* IN_RING0 */

RT_C_DECLS_END

#endif /* RT_OS_DARWIN */

#endif /* !___VBox_VBoxNetSend_h */

