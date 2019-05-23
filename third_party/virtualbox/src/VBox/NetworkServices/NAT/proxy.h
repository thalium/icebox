/* $Id: proxy.h $ */
/** @file
 * NAT Network - common definitions and declarations.
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

#ifndef ___nat_proxy_h___
#define ___nat_proxy_h___

#if !defined(VBOX)
#include "vbox-compat.h"
#endif

#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "winutils.h"

/* forward */
struct netif;
struct tcpip_msg;
struct pbuf;
struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;

struct ip4_lomap
{
    ip_addr_t loaddr;
    uint32_t off;
};

struct ip4_lomap_desc
{
    const struct ip4_lomap *lomap;
    unsigned int num_lomap;
};

struct proxy_options {
    int ipv6_enabled;
    int ipv6_defroute;
    SOCKET icmpsock4;
    SOCKET icmpsock6;
    const char *tftp_root;
    const struct sockaddr_in *src4;
    const struct sockaddr_in6 *src6;
    const struct ip4_lomap_desc *lomap_desc;
    const char **nameservers;
};

extern volatile struct proxy_options *g_proxy_options;
extern struct netif *g_proxy_netif;

void proxy_init(struct netif *, struct proxy_options *);
SOCKET proxy_connected_socket(int, int, ipX_addr_t *, u16_t);
SOCKET proxy_bound_socket(int, int, struct sockaddr *);
void proxy_reset_socket(SOCKET);
int proxy_sendto(SOCKET, struct pbuf *, void *, size_t);
void proxy_lwip_post(struct tcpip_msg *);
const char *proxy_lwip_strerr(err_t);

/* proxy_rtadvd.c */
void proxy_rtadvd_start(struct netif *);
void proxy_rtadvd_do_quick(void *);

/* rtmon_*.c */
int rtmon_get_defaults(void);

/* proxy_dhcp6ds.c */
err_t dhcp6ds_init(struct netif *);

/* proxy_tftpd.c */
err_t tftpd_init(struct netif *, const char *);

/* pxtcp.c */
void pxtcp_init(void);

/* pxudp.c */
void pxudp_init(void);

/* pxdns.c */
err_t pxdns_init(struct netif *);
void pxdns_set_nameservers(void *);

/* pxping.c */
err_t pxping_init(struct netif *, SOCKET, SOCKET);


#if defined(RT_OS_LINUX) || defined(RT_OS_SOLARIS) || defined(RT_OS_WINDOWS)
# define HAVE_SA_LEN 0
#else
# define HAVE_SA_LEN 1
#endif

#define LWIP_ASSERT1(condition) LWIP_ASSERT(#condition, condition)

/*
 * TODO: DPRINTF0 should probably become LogRel but its usage needs to
 * be cleaned up a bit before.
 */
#define DPRINTF0(a) Log(a)

#define DPRINTF(a)  DPRINTF1(a)
#define DPRINTF1(a) Log2(a)
#define DPRINTF2(a) Log3(a)

#endif /* !___nat_proxy_h___ */

