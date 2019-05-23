/* $Id: proxy_pollmgr.h $ */
/** @file
 * NAT Network - poll manager, definitions and declarations.
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

#ifndef _PROXY_POLLMGR_H_
#define _PROXY_POLLMGR_H_

#ifndef RT_OS_WINDOWS
# include <unistd.h>             /* for ssize_t */
#endif
#include "lwip/sys.h"

enum pollmgr_slot_t {
    POLLMGR_CHAN_PXTCP_ADD,     /* new proxy tcp connection from guest */
    POLLMGR_CHAN_PXTCP_POLLIN,  /* free space in ringbuf, may POLLIN */
    POLLMGR_CHAN_PXTCP_POLLOUT, /* schedule one-shot POLLOUT callback */
    POLLMGR_CHAN_PXTCP_DEL,     /* delete pxtcp */
    POLLMGR_CHAN_PXTCP_RESET,   /* send RST and delete pxtcp */

    POLLMGR_CHAN_PXUDP_ADD,     /* new proxy udp conversation from guest */
    POLLMGR_CHAN_PXUDP_DEL,     /* delete pxudp from pollmgr */

    POLLMGR_CHAN_PORTFWD,       /* add/remove port forwarding rules */

    POLLMGR_CHAN_COUNT
};


struct pollmgr_handler;         /* forward */
typedef int (*pollmgr_callback)(struct pollmgr_handler *, SOCKET, int);

struct pollmgr_handler {
    pollmgr_callback callback;
    void *data;
    int slot;
};

struct pollmgr_refptr {
    struct pollmgr_handler *ptr;
    sys_mutex_t lock;
    size_t strong;
    size_t weak;
};

int pollmgr_init(void);

/* static named slots (aka "channels") */
SOCKET pollmgr_add_chan(int, struct pollmgr_handler *);
ssize_t pollmgr_chan_send(int, void *buf, size_t nbytes);
void *pollmgr_chan_recv_ptr(struct pollmgr_handler *, SOCKET, int);

/* dynamic slots */
int pollmgr_add(struct pollmgr_handler *, SOCKET, int);

/* special-purpose strong/weak references */
struct pollmgr_refptr *pollmgr_refptr_create(struct pollmgr_handler *);
void pollmgr_refptr_weak_ref(struct pollmgr_refptr *);
struct pollmgr_handler *pollmgr_refptr_get(struct pollmgr_refptr *);
void pollmgr_refptr_unref(struct pollmgr_refptr *);

void pollmgr_update_events(int, int);
void pollmgr_del_slot(int);

void pollmgr_thread(void *);

/* buffer for callbacks to receive udp without worrying about truncation */
extern u8_t pollmgr_udpbuf[64 * 1024];

#endif /* _PROXY_POLLMGR_H_ */
