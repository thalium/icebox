/* $Id: NetIf-os2.cpp $ */
/** @file
 * Main - NetIfList, OS/2 implementation.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_MAIN

#include <iprt/err.h>
#include <list>

#include "HostNetworkInterfaceImpl.h"
#include "netif.h"

int NetIfList(std::list <ComObjPtr<HostNetworkInterface> > &list)
{
    return VERR_NOT_IMPLEMENTED;
}

int NetIfEnableStaticIpConfig(VirtualBox *pVBox, HostNetworkInterface * pIf, ULONG ip, ULONG mask)
{
    return VERR_NOT_IMPLEMENTED;
}

int NetIfEnableStaticIpConfigV6(VirtualBox *pVBox, HostNetworkInterface * pIf, IN_BSTR aIPV6Address, ULONG aIPV6MaskPrefixLength)
{
    return VERR_NOT_IMPLEMENTED;
}

int NetIfEnableDynamicIpConfig(VirtualBox *pVBox, HostNetworkInterface * pIf)
{
    return VERR_NOT_IMPLEMENTED;
}


int NetIfDhcpRediscover(VirtualBox *pVBox, HostNetworkInterface * pIf)
{
    return VERR_NOT_IMPLEMENTED;
}

