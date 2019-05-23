/* $Id: VBoxNetAdp-win.h $ */
/** @file
 * VBoxNetAdp-win.h - Host-only Miniport Driver, Windows-specific code.
 */
/*
 * Copyright (C) 2014-2017 Oracle Corporation
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
#ifndef ___VBoxNetAdp_win_h___
#define ___VBoxNetAdp_win_h___

#define VBOXNETADP_VERSION_NDIS_MAJOR        6
#define VBOXNETADP_VERSION_NDIS_MINOR        0

#define VBOXNETADP_VERSION_MAJOR             1
#define VBOXNETADP_VERSION_MINOR             0

#define VBOXNETADP_VENDOR_NAME               "Oracle"
#define VBOXNETADP_VENDOR_ID                 0xFFFFFF
#define VBOXNETADP_MCAST_LIST_SIZE           32
#define VBOXNETADP_MAX_FRAME_SIZE            1518 // TODO: 14+4+1500

//#define VBOXNETADP_NAME_UNIQUE               L"{7af6b074-048d-4444-bfce-1ecc8bc5cb76}"
#define VBOXNETADP_NAME_SERVICE              L"VBoxNetAdp"

#define VBOXNETADP_NAME_LINK                 L"\\DosDevices\\Global\\VBoxNetAdp"
#define VBOXNETADP_NAME_DEVICE               L"\\Device\\VBoxNetAdp"

#define VBOXNETADPWIN_TAG                    'ANBV'

#define VBOXNETADPWIN_ATTR_FLAGS             NDIS_MINIPORT_ATTRIBUTES_NDIS_WDM | NDIS_MINIPORT_ATTRIBUTES_NO_HALT_ON_SUSPEND
#define VBOXNETADP_MAC_OPTIONS               NDIS_MAC_OPTION_NO_LOOPBACK
#define VBOXNETADP_SUPPORTED_FILTERS         (NDIS_PACKET_TYPE_DIRECTED | \
                                              NDIS_PACKET_TYPE_MULTICAST | \
                                              NDIS_PACKET_TYPE_BROADCAST | \
                                              NDIS_PACKET_TYPE_PROMISCUOUS | \
                                              NDIS_PACKET_TYPE_ALL_MULTICAST)
#define VBOXNETADPWIN_SUPPORTED_STATISTICS   0 //TODO!
#define VBOXNETADPWIN_HANG_CHECK_TIME        4

#endif /* #ifndef ___VBoxNetAdp_win_h___ */
