/* $Id: VBoxTrayMsg.h $ */
/** @file
 * VBoxTrayMsg - Globally registered messages (RPC) to/from VBoxTray.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBOXTRAY_MSG_H
#define ___VBOXTRAY_MSG_H

/** The IPC pipe's prefix (native).
 * Will be followed by the username VBoxTray runs under. */
#define VBOXTRAY_IPC_PIPE_PREFIX      "\\\\.\\pipe\\VBoxTrayIPC-"
/** The IPC header's magic. */
#define VBOXTRAY_IPC_HDR_MAGIC        0x19840804

enum VBOXTRAYIPCMSGTYPE
{
    /** Restarts VBoxTray. */
    VBOXTRAYIPCMSGTYPE_RESTART        = 10,
    /** Shows a balloon message in the tray area. */
    VBOXTRAYIPCMSGTYPE_SHOWBALLOONMSG = 100,
    /** Retrieves the current user's last input
     *  time. This will be the user VBoxTray is running
     *  under. No actual message for this command
     *  required. */
    VBOXTRAYIPCMSGTYPE_USERLASTINPUT  = 120
};

/* VBoxTray's IPC header. */
typedef struct VBOXTRAYIPCHEADER
{
    /** The header's magic. */
    uint32_t uMagic;
    /** Header version, must be 0 by now. */
    uint32_t uHdrVersion;
    /** Message type. Specifies a message
     *  of VBOXTRAYIPCMSGTYPE. */
    uint32_t uMsgType;
    /** Message length (in bytes). This must
     *  include the overall message length, including
     *  (eventual) dynamically allocated areas which
     *  are passed into the message structure.
     */
    uint32_t uMsgLen;

} VBOXTRAYIPCHEADER, *PVBOXTRAYIPCHEADER;

/**
 * Tells VBoxTray to show a balloon message in Windows'
 * tray area. This may or may not work depending on the
 * system's configuration / set user preference.
 */
typedef struct VBOXTRAYIPCMSG_SHOWBALLOONMSG
{
    /** Length of message body (in bytes). */
    uint32_t cbMsgContent;
    /** Length of message title (in bytes). */
    uint32_t cbMsgTitle;
    /** Message type. */
    uint32_t uType;
    /** Time to show the message (in ms). */
    uint32_t uShowMS;
    /** Dynamically allocated stuff.
     *
     *  Note: These must come at the end of the
     *  structure to not overwrite any important
     *  stuff above.
     */
    /** Message body. Can be up to 256 chars
     *  long. */
    char     szMsgContent[1];
        /** Message title. Can be up to 73 chars
     *  long. */
    char     szMsgTitle[1];
} VBOXTRAYIPCMSG_SHOWBALLOONMSG, *PVBOXTRAYIPCMSG_SHOWBALLOONMSG;

/**
 * Response telling the last input of the current user.
 */
typedef struct VBOXTRAYIPCRES_USERLASTINPUT
{
    /** Last occurred user input event (in seconds). */
    uint32_t uLastInput;
} VBOXTRAYIPCRES_USERLASTINPUT, *PVBOXTRAYIPCRES_USERLASTINPUT;

#endif /* !___VBOXTRAY_MSG_H */

