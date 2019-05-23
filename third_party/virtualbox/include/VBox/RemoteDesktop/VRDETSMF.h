/* @file
 * VBox Remote Desktop Extension (VRDE) - raw TSMF dynamic channel interface.
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

#ifndef ___VBox_RemoteDesktop_VRDETSMF_h
#define ___VBox_RemoteDesktop_VRDETSMF_h

#include <VBox/RemoteDesktop/VRDE.h>

/*
 * Interface creating a TSMF dynamic channel instances and sending/receving data.
 *
 * Async callbacks are used for providing feedback, reporting errors, etc.
 */

#define VRDE_TSMF_INTERFACE_NAME "TSMFRAW"

/* The VRDE server TSMF interface entry points. Interface version 1. */
typedef struct VRDETSMFINTERFACE
{
    /* The header. */
    VRDEINTERFACEHDR header;

    /* Create a new TSMF channel instance.
     * The channel is created only for one client, which is connected to the server.
     * The client is the first which supports dynamic RDP channels.
     *
     * If this method return success then the server will use the VRDE_TSMF_N_CREATE_*
     * notification to report the channel handle.
     *
     * @param hServer   The VRDE server instance.
     * @param pvChannel A context to be associated with the channel.
     * @param u32Flags  VRDE_TSMF_F_*
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDETSMFChannelCreate, (HVRDESERVER hServer,
                                                      void *pvChannel,
                                                      uint32_t u32Flags));

    /* Close a TSMF channel instance.
     *
     * @param hServer          The VRDE server instance.
     * @param u32ChannelHandle Which channel to close.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDETSMFChannelClose, (HVRDESERVER hServer,
                                                     uint32_t u32ChannelHandle));

    /* Send data to the TSMF channel instance.
     *
     * @param hServer          The VRDE server instance.
     * @param u32ChannelHandle Channel to send data.
     * @param pvData           Raw data to be sent, the format depends on which
     *                         u32Flags were specified in Create: TSMF message,
     *                         or channel header + TSMF message.
     * @param cbData           Size of data.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(int, VRDETSMFChannelSend, (HVRDESERVER hServer,
                                                    uint32_t u32ChannelHandle,
                                                    const void *pvData,
                                                    uint32_t cbData));
} VRDETSMFINTERFACE;

/* TSMF interface callbacks. */
typedef struct VRDETSMFCALLBACKS
{
    /* The header. */
    VRDEINTERFACEHDR header;

    /* Channel event notification.
     *
     * @param pvContext The callbacks context specified in VRDEGetInterface.
     * @param u32Notification The kind of the notification: VRDE_TSMF_N_*.
     * @param pvChannel   A context which was used in VRDETSMFChannelCreate.
     * @param pvParm    The notification specific data.
     * @param cbParm    The size of buffer pointed by pvParm.
     *
     * @return IPRT status code.
     */
    DECLR3CALLBACKMEMBER(void, VRDETSMFCbNotify, (void *pvContext,
                                                  uint32_t u32Notification,
                                                  void *pvChannel,
                                                  const void *pvParm,
                                                  uint32_t cbParm));
} VRDETSMFCALLBACKS;


/* VRDETSMFChannelCreate::u32Flags */
#define VRDE_TSMF_F_CHANNEL_HEADER 0x00000001


/* VRDETSMFCbNotify::u32Notification */
#define VRDE_TSMF_N_CREATE_ACCEPTED         1
#define VRDE_TSMF_N_CREATE_DECLINED         2
#define VRDE_TSMF_N_DATA                    3 /* Data received. */
#define VRDE_TSMF_N_DISCONNECTED            4 /* The channel is not connected anymore. */


/*
 * Notification parameters.
 */

/* VRDE_TSMF_N_CREATE_ACCEPTED */
typedef struct VRDETSMFNOTIFYCREATEACCEPTED
{
    uint32_t u32ChannelHandle;
} VRDETSMFNOTIFYCREATEACCEPTED;

/* VRDE_TSMF_N_EVENT_DATA */
typedef struct VRDETSMFNOTIFYDATA
{
    const void *pvData;
    uint32_t cbData; /* How many bytes available. */
} VRDETSMFNOTIFYDATA;

#endif
