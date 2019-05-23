/* @file
 *
 * Host Channel
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

#ifndef __VBOXHOSTCHANNEL__H
#define __VBOXHOSTCHANNEL__H

#include <iprt/list.h>

#define LOG_GROUP LOG_GROUP_HGCM
#include <VBox/log.h>
#include <VBox/HostServices/VBoxHostChannel.h>

#define HOSTCHLOG Log

#ifdef DEBUG_sunlover
# undef HOSTCHLOG
# define HOSTCHLOG LogRel
#endif /* DEBUG_sunlover */

struct VBOXHOSTCHCTX;
typedef struct VBOXHOSTCHCTX VBOXHOSTCHCTX;

typedef struct VBOXHOSTCHCLIENT
{
    RTLISTNODE nodeClient;

    VBOXHOSTCHCTX *pCtx;

    uint32_t u32ClientID;

    RTLISTANCHOR listChannels;
    uint32_t volatile u32HandleSrc;

    RTLISTANCHOR listContexts; /* Callback contexts. */

    RTLISTANCHOR listEvents;

    bool fAsync;        /* Guest is waiting for a message. */

    struct {
        VBOXHGCMCALLHANDLE callHandle;
        VBOXHGCMSVCPARM *paParms;
    } async;

} VBOXHOSTCHCLIENT;


/*
 * The service functions. Locking is between the service thread and the host channel provider thread.
 */
int vboxHostChannelLock(void);
void vboxHostChannelUnlock(void);

int vboxHostChannelInit(void);
void vboxHostChannelDestroy(void);

int vboxHostChannelClientConnect(VBOXHOSTCHCLIENT *pClient);
void vboxHostChannelClientDisconnect(VBOXHOSTCHCLIENT *pClient);

int vboxHostChannelAttach(VBOXHOSTCHCLIENT *pClient,
                          uint32_t *pu32Handle,
                          const char *pszName,
                          uint32_t u32Flags);
int vboxHostChannelDetach(VBOXHOSTCHCLIENT *pClient,
                          uint32_t u32Handle);

int vboxHostChannelSend(VBOXHOSTCHCLIENT *pClient,
                        uint32_t u32Handle,
                        const void *pvData,
                        uint32_t cbData);
int vboxHostChannelRecv(VBOXHOSTCHCLIENT *pClient,
                        uint32_t u32Handle,
                        void *pvData,
                        uint32_t cbData,
                        uint32_t *pu32DataReceived,
                        uint32_t *pu32DataRemaining);
int vboxHostChannelControl(VBOXHOSTCHCLIENT *pClient,
                           uint32_t u32Handle,
                           uint32_t u32Code,
                           void *pvParm,
                           uint32_t cbParm,
                           void *pvData,
                           uint32_t cbData,
                           uint32_t *pu32SizeDataReturned);

int vboxHostChannelEventWait(VBOXHOSTCHCLIENT *pClient,
                             bool *pfEvent,
                             VBOXHGCMCALLHANDLE callHandle,
                             VBOXHGCMSVCPARM *paParms);

int vboxHostChannelEventCancel(VBOXHOSTCHCLIENT *pClient);

int vboxHostChannelQuery(VBOXHOSTCHCLIENT *pClient,
                         const char *pszName,
                         uint32_t u32Code,
                         void *pvParm,
                         uint32_t cbParm,
                         void *pvData,
                         uint32_t cbData,
                         uint32_t *pu32SizeDataReturned);

int vboxHostChannelRegister(const char *pszName,
                            const VBOXHOSTCHANNELINTERFACE *pInterface,
                            uint32_t cbInterface);
int vboxHostChannelUnregister(const char *pszName);


void vboxHostChannelEventParmsSet(VBOXHGCMSVCPARM *paParms,
                                  uint32_t u32ChannelHandle,
                                  uint32_t u32Id,
                                  const void *pvEvent,
                                  uint32_t cbEvent);

void vboxHostChannelReportAsync(VBOXHOSTCHCLIENT *pClient,
                                uint32_t u32ChannelHandle,
                                uint32_t u32Id,
                                const void *pvEvent,
                                uint32_t cbEvent);

#endif /* __VBOXHOSTCHANNEL__H */
