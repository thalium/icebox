/* $Id: netif.h $ */
/** @file
 * Main - Network Interfaces.
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

#ifndef ___netif_h
#define ___netif_h

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/net.h>
/** @todo r=bird: The inlined code below that drags in asm.h here. I doubt
 *        speed is very important here, so move it into a .cpp file, please. */
#include <iprt/asm.h>

#ifndef RT_OS_WINDOWS
# include <arpa/inet.h>
# include <stdio.h>
#endif /* !RT_OS_WINDOWS */

#define VBOXNET_IPV4ADDR_DEFAULT      0x0138A8C0  /* 192.168.56.1 */
#define VBOXNET_IPV4MASK_DEFAULT      "255.255.255.0"

#define VBOXNET_MAX_SHORT_NAME 50

#if 1
/**
 * Encapsulation type.
 */
typedef enum NETIFTYPE
{
    NETIF_T_UNKNOWN,
    NETIF_T_ETHERNET,
    NETIF_T_PPP,
    NETIF_T_SLIP
} NETIFTYPE;

/**
 * Current state of the interface.
 */
typedef enum NETIFSTATUS
{
    NETIF_S_UNKNOWN,
    NETIF_S_UP,
    NETIF_S_DOWN
} NETIFSTATUS;

/**
 * Host Network Interface Information.
 */
typedef struct NETIFINFO
{
    NETIFINFO     *pNext;
    RTNETADDRIPV4  IPAddress;
    RTNETADDRIPV4  IPNetMask;
    RTNETADDRIPV6  IPv6Address;
    RTNETADDRIPV6  IPv6NetMask;
    BOOL           fDhcpEnabled;
    BOOL           fIsDefault;
    BOOL           fWireless;
    RTMAC          MACAddress;
    NETIFTYPE      enmMediumType;
    NETIFSTATUS    enmStatus;
    uint32_t       uSpeedMbits;
    RTUUID         Uuid;
    char           szShortName[VBOXNET_MAX_SHORT_NAME];
    char           szName[1];
} NETIFINFO;

/** Pointer to a network interface info. */
typedef NETIFINFO *PNETIFINFO;
/** Pointer to a const network interface info. */
typedef NETIFINFO const *PCNETIFINFO;
#endif

int NetIfList(std::list <ComObjPtr<HostNetworkInterface> > &list);
int NetIfEnableStaticIpConfig(VirtualBox *pVBox, HostNetworkInterface * pIf, ULONG aOldIp, ULONG aNewIp, ULONG aMask);
int NetIfEnableStaticIpConfigV6(VirtualBox *pVBox, HostNetworkInterface * pIf, IN_BSTR aOldIPV6Address, IN_BSTR aIPV6Address, ULONG aIPV6MaskPrefixLength);
int NetIfEnableDynamicIpConfig(VirtualBox *pVBox, HostNetworkInterface * pIf);
int NetIfCreateHostOnlyNetworkInterface(VirtualBox *pVBox, IHostNetworkInterface **aHostNetworkInterface, IProgress **aProgress, const char *pszName = NULL);
int NetIfRemoveHostOnlyNetworkInterface(VirtualBox *pVBox, IN_GUID aId, IProgress **aProgress);
int NetIfGetConfig(HostNetworkInterface * pIf, NETIFINFO *);
int NetIfGetConfigByName(PNETIFINFO pInfo);
int NetIfGetState(const char *pcszIfName, NETIFSTATUS *penmState);
int NetIfGetLinkSpeed(const char *pcszIfName, uint32_t *puMbits);
int NetIfDhcpRediscover(VirtualBox *pVBox, HostNetworkInterface * pIf);
int NetIfAdpCtlOut(const char *pszName, const char *pszCmd, char *pszBuffer, size_t cBufSize);

DECLINLINE(Bstr) getDefaultIPv4Address(Bstr bstrIfName)
{
    /* Get the index from the name */
    Utf8Str strTmp = bstrIfName;
    const char *pcszIfName = strTmp.c_str();
    int iInstance = 0;
    size_t iPos = strcspn(pcszIfName, "0123456789");
    if (pcszIfName[iPos])
        iInstance = RTStrToUInt32(pcszIfName + iPos);

    in_addr tmp;
#if defined(RT_OS_WINDOWS)
    tmp.S_un.S_addr = VBOXNET_IPV4ADDR_DEFAULT + (iInstance << 16);
#else
    tmp.s_addr = VBOXNET_IPV4ADDR_DEFAULT + (iInstance << 16);
#endif
    char *addr = inet_ntoa(tmp);
    return Bstr(addr);
}

#endif  /* !___netif_h */
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
