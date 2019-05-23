/** @file
 * Drag and Drop service - Common header for host service and guest clients.
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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

/**
 * Protocol handling and notes:
 *     All client/server components should be backwards compatible.
 *
 ******************************************************************************
 *
 * Protocol changelog:
 *
 *     Protocol v1 (VBox < 5.0, deprecated):
 *         | Initial implementation which only implemented host to guest transfers.
 *         | For file transfers all file information such as the file name and file size were
 *           transferred with every file data chunk being sent.
 *
 *     Protocol v2 (VBox 5.0 - VBox 5.0.8, deprecated):
 *         + Added support for guest to host transfers.
 *         + Added protocol version support through VBOXDNDCONNECTMSG. The host takes the installed
 *           Guest Additions version as indicator which protocol to use for communicating with the guest.
 *           The guest itself uses VBOXDNDCONNECTMSG to report its supported protocol version to the DnD service.
 *
 *     Protocol v3 (VBox 5.0.10 and up, current):
 *         + Added VBOXDNDDISCONNECTMSG for being able to track client disconnects on host side (Main).
 *         + Added context IDs for every HGCM message. Not used yet and must be 0.
 *         + Added VBOXDNDSNDDATAHDR and VBOXDNDCBSNDDATAHDRDATA to support (simple) accounting of objects
 *           being transferred, along with supplying separate meta data size (which is part of the total size being sent).
 *         + Added new HOST_DND_HG_SND_DATA_HDR + GUEST_DND_GH_SND_DATA_HDR commands which now allow specifying an optional
 *           compression type and defining a checksum for the overall data transfer.
 *         + Enhannced VBOXDNDGHSENDDATAMSG to support (rolling) checksums for the supplied data block.
 *         + VBOXDNDHGSENDDATAMSG and VBOXDNDGHSENDDATAMSG can now contain an optional checksum for the current data block.
 *         | VBOXDNDHGSENDFILEDATAMSG and VBOXDNDGHSENDFILEDATAMSG are now sharing the same HGCM mesasge.
 *         - Removed unused HOST_DND_GH_RECV_DIR, HOST_DND_GH_RECV_FILE_DATA and HOST_DND_GH_RECV_FILE_HDR commands.
 */

#ifndef ___VBox_HostService_DragAndDropSvc_h
#define ___VBox_HostService_DragAndDropSvc_h

#include <VBox/hgcmsvc.h>
#include <VBox/VMMDevCoreTypes.h>
#include <VBox/VBoxGuestCoreTypes.h>

/*
 * The mode of operations.
 */
#define VBOX_DRAG_AND_DROP_MODE_OFF           0
#define VBOX_DRAG_AND_DROP_MODE_HOST_TO_GUEST 1
#define VBOX_DRAG_AND_DROP_MODE_GUEST_TO_HOST 2
#define VBOX_DRAG_AND_DROP_MODE_BIDIRECTIONAL 3

#define DND_IGNORE_ACTION     UINT32_C(0)
#define DND_COPY_ACTION       RT_BIT_32(0)
#define DND_MOVE_ACTION       RT_BIT_32(1)
#define DND_LINK_ACTION       RT_BIT_32(2)

#define hasDnDCopyAction(a)   ((a) & DND_COPY_ACTION)
#define hasDnDMoveAction(a)   ((a) & DND_MOVE_ACTION)
#define hasDnDLinkAction(a)   ((a) & DND_LINK_ACTION)

#define isDnDIgnoreAction(a)  ((a) == DND_IGNORE_ACTION)
#define isDnDCopyAction(a)    ((a) == DND_COPY_ACTION)
#define isDnDMoveAction(a)    ((a) == DND_MOVE_ACTION)
#define isDnDLinkAction(a)    ((a) == DND_LINK_ACTION)

/** @def VBOX_DND_FORMATS_DEFAULT
 * Default drag'n drop formats.
 * Note: If you add new entries here, make sure you test those
 *       with all supported guest OSes!
 */
#define VBOX_DND_FORMATS_DEFAULT                                                                \
    "text/uri-list",                                                                            \
    /* Text. */                                                                                 \
    "text/html",                                                                                \
    "text/plain;charset=utf-8",                                                                 \
    "text/plain;charset=utf-16",                                                                \
    "text/plain",                                                                               \
    "text/richtext",                                                                            \
    "UTF8_STRING",                                                                              \
    "TEXT",                                                                                     \
    "STRING",                                                                                   \
    /* OpenOffice formats. */                                                                   \
    /* See: https://wiki.openoffice.org/wiki/Documentation/DevGuide/OfficeDev/Common_Application_Features#OpenOffice.org_Clipboard_Data_Formats */ \
    "application/x-openoffice-embed-source-xml;windows_formatname=\"Star Embed Source (XML)\"", \
    "application/x-openoffice;windows_formatname=\"Bitmap\""

namespace DragAndDropSvc {

/******************************************************************************
* Typedefs, constants and inlines                                             *
******************************************************************************/

/**
 * The service functions which are callable by host.
 * Note: When adding new functions to this table, make sure that the actual ID
 *       does *not* overlap with the eGuestFn enumeration below!
 */
enum eHostFn
{
    /** The host just has set a new DnD mode. */
    HOST_DND_SET_MODE                  = 100,

    /*
     * Host -> Guest messages
     */

    /** The host entered the VM window for starting an actual
     *  DnD operation. */
    HOST_DND_HG_EVT_ENTER              = 200,
    /** The host's DnD cursor moved within the VM window. */
    HOST_DND_HG_EVT_MOVE               = 201,
    /** The host left the guest VM window. */
    HOST_DND_HG_EVT_LEAVE              = 202,
    /** The host issued a "drop" event, meaning that the host is
     *  ready to transfer data over to the guest. */
    HOST_DND_HG_EVT_DROPPED            = 203,
    /** The host requested to cancel the current DnD operation on
     *  the guest side. This can happen on user request on the host's
     *  UI side or due to some host error which has happened.
     *
     *  Note: This is a fire-and-forget message, as the host should
     *        not rely on an answer from the guest side in order to
     *        properly cancel the operation. */
    HOST_DND_HG_EVT_CANCEL             = 204,
    /** Sends the data header at the beginning of a (new)
     *  data transfer. */
    HOST_DND_HG_SND_DATA_HDR           = 210,
    /**
     * Sends the actual meta data, based on
     * the format(s) specified by HOST_DND_HG_EVT_ENTER.
     *
     * Protocol v1/v2: If the guest supplied buffer too small to send
     *                 the actual data, the host will send a HOST_DND_HG_SND_MORE_DATA
     *                 message as follow-up.
     * Protocol v3+:   The incoming meta data size is specified upfront in the
     *                 HOST_DND_HG_SND_DATA_HDR message and must be handled accordingly.
     */
    HOST_DND_HG_SND_DATA               = 205,
    /** Sent when the actual buffer for HOST_DND_HG_SND_DATA was too small. */
    /** @todo Deprecated function; do not use anymore. */
    HOST_DND_HG_SND_MORE_DATA          = 206,
    /** Directory entry to be sent to the guest. */
    HOST_DND_HG_SND_DIR                = 207,
    /** File data chunk to send to the guest. */
    HOST_DND_HG_SND_FILE_DATA          = 208,
    /** File header to send to the guest.
     *  Note: Only for protocol version 2 and up (>= VBox 5.0). */
    HOST_DND_HG_SND_FILE_HDR           = 209,

    /*
     * Guest -> Host messages
     */

    /** The host asks the guest whether a DnD operation
     *  is in progress when the mouse leaves the guest window. */
    HOST_DND_GH_REQ_PENDING            = 600,
    /** The host informs the guest that a DnD drop operation
     *  has been started and that the host wants the data in
     *  a specific MIME type. */
    HOST_DND_GH_EVT_DROPPED            = 601,
    /** Blow the type up to 32-bit. */
    HOST_DND_32BIT_HACK                = 0x7fffffff
};

/**
 * The service functions which are called by guest.
 * Note: When adding new functions to this table, make sure that the actual ID
 *       does *not* overlap with the eHostFn enumeration above!
 */
enum eGuestFn
{
    /* Guest sends a connection request to the HGCM service,
     * along with some additional information like supported
     * protocol version and flags.
     * Note: New since protocol version 2. */
    GUEST_DND_CONNECT                  = 10,

    /* Sent when a guest client disconnected from the HGCM service. */
    GUEST_DND_DISCONNECT               = 11,

    /**
     * Guest waits for a new message the host wants to process
     * on the guest side. This can be a blocking call.
     */
    GUEST_DND_GET_NEXT_HOST_MSG        = 300,

    /*
     * Host -> Guest operation messages.
     */

    /** The guest acknowledges that the pending DnD data from
     *  the host can be dropped on the currently selected source
     *  on the guest. */
    GUEST_DND_HG_ACK_OP                = 400,
    /** The guest requests the actual DnD data to be sent
     *  from the host. */
    GUEST_DND_HG_REQ_DATA              = 401,
    /** Reports back the guest's progress on a host -> guest operation. */
    GUEST_DND_HG_EVT_PROGRESS          = 402,

    /*
     * Guest -> Host operation messages.
     */

    /**
     * The guests acknowledges that it currently has a drag'n drop
     * operation in progress on the guest, which eventually could be
     * dragged over to the host.
     */
    GUEST_DND_GH_ACK_PENDING           = 500,
    /** Sends the data header at the beginning of a (new)
     *  data transfer. */
    GUEST_DND_GH_SND_DATA_HDR          = 503,
    /**
     * Sends data of the requested format to the host. There can
     * be more than one message if the actual data does not fit
     * into one.
     */
    GUEST_DND_GH_SND_DATA              = 501,
    /** Reports an error back to the host. */
    GUEST_DND_GH_EVT_ERROR             = 502,
    /** Guest sends a directory entry to the host. */
    GUEST_DND_GH_SND_DIR               = 700,
    /** Guest sends file data to the host. */
    /*  Note: On protocol version 1 this also contains the file name
     *        and other attributes. */
    GUEST_DND_GH_SND_FILE_DATA         = 701,
    /** Guest sends a file header to the host, marking the
     *  beginning of a (new) file transfer.
     *  Note: Available since protocol version 2 (VBox 5.0). */
    GUEST_DND_GH_SND_FILE_HDR          = 702,
    /** Blow the type up to 32-bit. */
    GUEST_DND_32BIT_HACK               = 0x7fffffff
};

/**
 * DnD operation progress states.
 */
typedef enum DNDPROGRESS
{
    DND_PROGRESS_UNKNOWN               = 0,
    DND_PROGRESS_RUNNING               = 1,
    DND_PROGRESS_COMPLETE,
    DND_PROGRESS_CANCELLED,
    DND_PROGRESS_ERROR,
    /** Blow the type up to 32-bit. */
    DND_PROGRESS_32BIT_HACK            = 0x7fffffff
} DNDPROGRESS, *PDNDPROGRESS;

#pragma pack (1)

/*
 * Host events
 */

/**
 * Action message for telling the guest about the currently ongoing
 * drag and drop action when entering the guest's area, moving around in it
 * and dropping content into it from the host.
 *
 * Used by:
 * HOST_DND_HG_EVT_ENTER
 * HOST_DND_HG_EVT_MOVE
 * HOST_DND_HG_EVT_DROPPED
 */
typedef struct VBOXDNDHGACTIONMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        struct
        {
            HGCMFunctionParameter uScreenId;    /* OUT uint32_t */
            HGCMFunctionParameter uX;           /* OUT uint32_t */
            HGCMFunctionParameter uY;           /* OUT uint32_t */
            HGCMFunctionParameter uDefAction;   /* OUT uint32_t */
            HGCMFunctionParameter uAllActions;  /* OUT uint32_t */
            HGCMFunctionParameter pvFormats;    /* OUT ptr */
            HGCMFunctionParameter cbFormats;    /* OUT uint32_t */
        } v1;
        struct
        {
            /** Context ID. */
            HGCMFunctionParameter uContext;
            HGCMFunctionParameter uScreenId;    /* OUT uint32_t */
            HGCMFunctionParameter uX;           /* OUT uint32_t */
            HGCMFunctionParameter uY;           /* OUT uint32_t */
            HGCMFunctionParameter uDefAction;   /* OUT uint32_t */
            HGCMFunctionParameter uAllActions;  /* OUT uint32_t */
            HGCMFunctionParameter pvFormats;    /* OUT ptr */
            HGCMFunctionParameter cbFormats;    /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDHGACTIONMSG;

/**
 * Tells the guest that the host has left its drag and drop area on the guest.
 *
 * Used by:
 * HOST_DND_HG_EVT_LEAVE
 */
typedef struct VBOXDNDHGLEAVEMSG
{
    VBGLIOCHGCMCALL hdr;
    union
    {
        struct
        {
            /** Context ID. */
            HGCMFunctionParameter uContext;
        } v3;
    } u;
} VBOXDNDHGLEAVEMSG;

/**
 * Tells the guest that the host wants to cancel the current drag and drop operation.
 *
 * Used by:
 * HOST_DND_HG_EVT_CANCEL
 */
typedef struct VBOXDNDHGCANCELMSG
{
    VBGLIOCHGCMCALL hdr;
    union
    {
        struct
        {
            /** Context ID. */
            HGCMFunctionParameter uContext;
        } v3;
    } u;
} VBOXDNDHGCANCELMSG;

/**
 * Sends the header of an incoming (meta) data block.
 *
 * Used by:
 * HOST_DND_HG_SND_DATA_HDR
 * GUEST_DND_GH_SND_DATA_HDR
 *
 * New since protocol v3.
 */
typedef struct VBOXDNDHGSENDDATAHDRMSG
{
    VBGLIOCHGCMCALL hdr;

    /** Context ID. Unused at the moment. */
    HGCMFunctionParameter uContext;        /* OUT uint32_t */
    /** Data transfer flags. Not yet used and must be 0. */
    HGCMFunctionParameter uFlags;          /* OUT uint32_t */
    /** Screen ID where the data originates from. */
    HGCMFunctionParameter uScreenId;       /* OUT uint32_t */
    /** Total size (in bytes) to transfer. */
    HGCMFunctionParameter cbTotal;         /* OUT uint64_t */
    /**
     * Total meta data size (in bytes) to transfer.
     * This size also is part of cbTotal already, so:
     *
     * cbTotal = cbMeta + additional size for files etc.
     */
    HGCMFunctionParameter cbMeta;          /* OUT uint64_t */
    /** Meta data format. */
    HGCMFunctionParameter pvMetaFmt;       /* OUT ptr */
    /** Size (in bytes) of meta data format. */
    HGCMFunctionParameter cbMetaFmt;       /* OUT uint32_t */
    /* Number of objects (files/directories) to transfer. */
    HGCMFunctionParameter cObjects;        /* OUT uint64_t */
    /** Compression type. */
    HGCMFunctionParameter enmCompression;  /* OUT uint32_t */
    /** Checksum type. */
    HGCMFunctionParameter enmChecksumType; /* OUT uint32_t */
    /** Checksum buffer for the entire data to be transferred. */
    HGCMFunctionParameter pvChecksum;      /* OUT ptr */
    /** Size (in bytes) of checksum. */
    HGCMFunctionParameter cbChecksum;      /* OUT uint32_t */
} VBOXDNDHGSENDDATAHDRMSG;

/**
 * Sends a (meta) data block to the guest.
 *
 * Used by:
 * HOST_DND_HG_SND_DATA
 */
typedef struct VBOXDNDHGSENDDATAMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        struct
        {
            HGCMFunctionParameter uScreenId;    /* OUT uint32_t */
            HGCMFunctionParameter pvFormat;     /* OUT ptr */
            HGCMFunctionParameter cbFormat;     /* OUT uint32_t */
            HGCMFunctionParameter pvData;       /* OUT ptr */
            HGCMFunctionParameter cbData;       /* OUT uint32_t */
        } v1;
        /* No changes in v2. */
        struct
        {
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            /** Data block to send. */
            HGCMFunctionParameter pvData;       /* OUT ptr */
            /** Size (in bytes) of data block to send. */
            HGCMFunctionParameter cbData;       /* OUT uint32_t */
            /** Checksum of data block, based on the checksum
             *  type in the data header. Optional. */
            HGCMFunctionParameter pvChecksum;   /* OUT ptr */
            /** Size (in bytes) of checksum to send. */
            HGCMFunctionParameter cbChecksum;   /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDHGSENDDATAMSG;

/**
 * Sends more (meta) data in case the data didn't fit
 * into the current XXX_DND_HG_SND_DATA message.
 *
 ** @todo Deprecated since protocol v3. Don't use! Will be removed.
 *
 * Used by:
 * HOST_DND_HG_SND_MORE_DATA
 */
typedef struct VBOXDNDHGSENDMOREDATAMSG
{
    VBGLIOCHGCMCALL hdr;

    HGCMFunctionParameter pvData;       /* OUT ptr */
    HGCMFunctionParameter cbData;       /* OUT uint32_t */
} VBOXDNDHGSENDMOREDATAMSG;

/**
 * Directory entry event.
 *
 * Used by:
 * HOST_DND_HG_SND_DIR
 * GUEST_DND_GH_SND_DIR
 */
typedef struct VBOXDNDHGSENDDIRMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        struct
        {
            /** Directory name. */
            HGCMFunctionParameter pvName;       /* OUT ptr */
            /** Size (in bytes) of directory name. */
            HGCMFunctionParameter cbName;       /* OUT uint32_t */
            /** Directory mode. */
            HGCMFunctionParameter fMode;        /* OUT uint32_t */
        } v1;
        struct
        {
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            /** Directory name. */
            HGCMFunctionParameter pvName;       /* OUT ptr */
            /** Size (in bytes) of directory name. */
            HGCMFunctionParameter cbName;       /* OUT uint32_t */
            /** Directory mode. */
            HGCMFunctionParameter fMode;        /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDHGSENDDIRMSG;

/**
 * File header message, marking the start of transferring a new file.
 * Note: Only for protocol version 2 and up.
 *
 * Used by:
 * HOST_DND_HG_SND_FILE_HDR
 * GUEST_DND_GH_SND_FILE_HDR
 */
typedef struct VBOXDNDHGSENDFILEHDRMSG
{
    VBGLIOCHGCMCALL hdr;

    /** Context ID. Unused at the moment. */
    HGCMFunctionParameter uContext;     /* OUT uint32_t */
    /** File path. */
    HGCMFunctionParameter pvName;       /* OUT ptr */
    /** Size (in bytes) of file path. */
    HGCMFunctionParameter cbName;       /* OUT uint32_t */
    /** Optional flags; unused at the moment. */
    HGCMFunctionParameter uFlags;       /* OUT uint32_t */
    /** File creation mode. */
    HGCMFunctionParameter fMode;        /* OUT uint32_t */
    /** Total size (in bytes). */
    HGCMFunctionParameter cbTotal;      /* OUT uint64_t */
} VBOXDNDHGSENDFILEHDRMSG;

/**
 * HG: File data (chunk) event.
 *
 * Used by:
 * HOST_DND_HG_SND_FILE
 */
typedef struct VBOXDNDHGSENDFILEDATAMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        /* Note: Protocol v1 sends the file name + file mode
         *       every time a file data chunk is being sent. */
        struct
        {
            /** File name. */
            HGCMFunctionParameter pvName;       /* OUT ptr */
            /** Size (in bytes) of file name. */
            HGCMFunctionParameter cbName;       /* OUT uint32_t */
            /** Current data chunk. */
            HGCMFunctionParameter pvData;       /* OUT ptr */
            /** Size (in bytes) of current data chunk. */
            HGCMFunctionParameter cbData;       /* OUT uint32_t */
            /** File mode. */
            HGCMFunctionParameter fMode;        /* OUT uint32_t */
        } v1;
        struct
        {
            /** Note: pvName is now part of the VBOXDNDHGSENDFILEHDRMSG message. */
            /** Note: cbName is now part of the VBOXDNDHGSENDFILEHDRMSG message. */
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            /** Current data chunk. */
            HGCMFunctionParameter pvData;       /* OUT ptr */
            /** Size (in bytes) of current data chunk. */
            HGCMFunctionParameter cbData;       /* OUT uint32_t */
            /** Note: fMode is now part of the VBOXDNDHGSENDFILEHDRMSG message. */
        } v2;
        struct
        {
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            /** Current data chunk. */
            HGCMFunctionParameter pvData;       /* OUT ptr */
            /** Size (in bytes) of current data chunk. */
            HGCMFunctionParameter cbData;       /* OUT uint32_t */
            /** Checksum of data block, based on the checksum
             *  type in the data header. Optional. */
            HGCMFunctionParameter pvChecksum;   /* OUT ptr */
            /** Size (in bytes) of curren data chunk checksum. */
            HGCMFunctionParameter cbChecksum;   /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDHGSENDFILEDATAMSG;

/**
 * Asks the guest if a guest->host DnD operation is in progress.
 *
 * Used by:
 * HOST_DND_GH_REQ_PENDING
 */
typedef struct VBOXDNDGHREQPENDINGMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        struct
        {
            /** Screen ID. */
            HGCMFunctionParameter uScreenId;    /* OUT uint32_t */
        } v1;
        struct
        {
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            /** Screen ID. */
            HGCMFunctionParameter uScreenId;    /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDGHREQPENDINGMSG;

/**
 * Tells the guest that the host has dropped the ongoing guest->host
 * DnD operation on a valid target on the host.
 *
 * Used by:
 * HOST_DND_GH_EVT_DROPPED
 */
typedef struct VBOXDNDGHDROPPEDMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        struct
        {
            /** Requested format for sending the data. */
            HGCMFunctionParameter pvFormat;     /* OUT ptr */
            /** Size (in bytes) of requested format. */
            HGCMFunctionParameter cbFormat;     /* OUT uint32_t */
            /** Drop action peformed on the host. */
            HGCMFunctionParameter uAction;      /* OUT uint32_t */
        } v1;
        struct
        {
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            /** Requested format for sending the data. */
            HGCMFunctionParameter pvFormat;     /* OUT ptr */
            /** Size (in bytes) of requested format. */
            HGCMFunctionParameter cbFormat;     /* OUT uint32_t */
            /** Drop action peformed on the host. */
            HGCMFunctionParameter uAction;      /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDGHDROPPEDMSG;

/*
 * Guest events
 */

/**
 * Asks the host for the next command to process, along
 * with the needed amount of parameters and an optional blocking
 * flag.
 *
 * Used by:
 * GUEST_DND_GET_NEXT_HOST_MSG
 */
typedef struct VBOXDNDNEXTMSGMSG
{
    VBGLIOCHGCMCALL hdr;

    /** Message ID. */
    HGCMFunctionParameter uMsg;      /* OUT uint32_t */
    /** Number of parameters the message needs. */
    HGCMFunctionParameter cParms;    /* OUT uint32_t */
    /** Whether or not to block (wait) for a
     *  new message to arrive. */
    HGCMFunctionParameter fBlock;    /* OUT uint32_t */
} VBOXDNDNEXTMSGMSG;

/**
 * Guest connection request. Used to tell the DnD protocol
 * version to the (host) service.
 *
 * Used by:
 * GUEST_DND_CONNECT
 */
typedef struct VBOXDNDCONNECTMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        struct
        {
            /** Protocol version to use. */
            HGCMFunctionParameter uProtocol;     /* OUT uint32_t */
            /** Connection flags. Optional. */
            HGCMFunctionParameter uFlags;        /* OUT uint32_t */
        } v2;
        struct
        {
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            /** Protocol version to use. */
            HGCMFunctionParameter uProtocol;     /* OUT uint32_t */
            /** Connection flags. Optional. */
            HGCMFunctionParameter uFlags;        /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDCONNECTMSG;

/**
 * Acknowledges a host operation along with the allowed
 * action(s) on the guest.
 *
 * Used by:
 * GUEST_DND_HG_ACK_OP
 */
typedef struct VBOXDNDHGACKOPMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        struct
        {
            HGCMFunctionParameter uAction;      /* OUT uint32_t */
        } v1;
        struct
        {
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            HGCMFunctionParameter uAction;      /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDHGACKOPMSG;

/**
 * HG request for data event.
 *
 * Used by:
 * GUEST_DND_HG_REQ_DATA
 */
typedef struct VBOXDNDHGREQDATAMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        struct
        {
            HGCMFunctionParameter pvFormat;     /* OUT ptr */
        } v1;
        struct
        {
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            HGCMFunctionParameter pvFormat;     /* OUT ptr */
            HGCMFunctionParameter cbFormat;     /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDHGREQDATAMSG;

typedef struct VBOXDNDHGEVTPROGRESSMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        struct
        {
            HGCMFunctionParameter uStatus;      /* OUT uint32_t */
            HGCMFunctionParameter uPercent;     /* OUT uint32_t */
            HGCMFunctionParameter rc;           /* OUT uint32_t */
        } v1;
        struct
        {
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            HGCMFunctionParameter uStatus;      /* OUT uint32_t */
            HGCMFunctionParameter uPercent;     /* OUT uint32_t */
            HGCMFunctionParameter rc;           /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDHGEVTPROGRESSMSG;

/**
 * Acknowledges a pending drag and drop event
 * to the host.
 *
 * Used by:
 * GUEST_DND_GH_ACK_PENDING
 */
typedef struct VBOXDNDGHACKPENDINGMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        struct
        {
            HGCMFunctionParameter uDefAction;   /* OUT uint32_t */
            HGCMFunctionParameter uAllActions;  /* OUT uint32_t */
            HGCMFunctionParameter pvFormats;    /* OUT ptr */
        } v1;
        struct
        {
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            HGCMFunctionParameter uDefAction;   /* OUT uint32_t */
            HGCMFunctionParameter uAllActions;  /* OUT uint32_t */
            HGCMFunctionParameter pvFormats;    /* OUT ptr */
            HGCMFunctionParameter cbFormats;    /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDGHACKPENDINGMSG;

/**
 * Sends the header of an incoming data block
 * to the host.
 *
 * Used by:
 * GUEST_DND_GH_SND_DATA_HDR
 *
 * New since protocol v3.
 */
typedef struct VBOXDNDHGSENDDATAHDRMSG VBOXDNDGHSENDDATAHDRMSG;

/**
 * Sends a (meta) data block to the host.
 *
 * Used by:
 * GUEST_DND_GH_SND_DATA
 */
typedef struct VBOXDNDGHSENDDATAMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        struct
        {
            HGCMFunctionParameter pvData;       /* OUT ptr */
            /** Total bytes to send. This can be more than
             * the data block specified in pvData above, e.g.
             * when sending over file objects afterwards. */
            HGCMFunctionParameter cbTotalBytes; /* OUT uint32_t */
        } v1;
        struct
        {
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            /** Data block to send. */
            HGCMFunctionParameter pvData;       /* OUT ptr */
            /** Size (in bytes) of data block to send. */
            HGCMFunctionParameter cbData;       /* OUT uint32_t */
            /** (Rolling) Checksum, based on checksum type in data header. */
            HGCMFunctionParameter pvChecksum;   /* OUT ptr */
            /** Size (in bytes) of checksum. */
            HGCMFunctionParameter cbChecksum;   /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDGHSENDDATAMSG;

/**
 * Sends a directory entry to the host.
 *
 * Used by:
 * GUEST_DND_GH_SND_DIR
 */
typedef struct VBOXDNDHGSENDDIRMSG VBOXDNDGHSENDDIRMSG;

/**
 * Sends a file header to the host.
 *
 * Used by:
 * GUEST_DND_GH_SND_FILE_HDR
 *
 * New since protocol v2.
 */
typedef struct VBOXDNDHGSENDFILEHDRMSG VBOXDNDGHSENDFILEHDRMSG;

/**
 * Sends file data to the host.
 *
 * Used by:
 * GUEST_DND_GH_SND_FILE_DATA
 */
typedef struct VBOXDNDHGSENDFILEDATAMSG VBOXDNDGHSENDFILEDATAMSG;

/**
 * Sends a guest error event to the host.
 *
 * Used by:
 * GUEST_DND_GH_EVT_ERROR
 */
typedef struct VBOXDNDGHEVTERRORMSG
{
    VBGLIOCHGCMCALL hdr;

    union
    {
        struct
        {
            HGCMFunctionParameter rc;           /* OUT uint32_t */
        } v1;
        struct
        {
            /** Context ID. Unused at the moment. */
            HGCMFunctionParameter uContext;     /* OUT uint32_t */
            HGCMFunctionParameter rc;           /* OUT uint32_t */
        } v3;
    } u;
} VBOXDNDGHEVTERRORMSG;

#pragma pack()

/** Builds a callback magic out of the function ID and the version
 *  of the callback data. */
#define VBOX_DND_CB_MAGIC_MAKE(uFn, uVer) \
    RT_MAKE_U32(uVer, uFn)

/*
 * Callback magics.
 */
enum eDnDCallbackMagics
{
    CB_MAGIC_DND_CONNECT                   = VBOX_DND_CB_MAGIC_MAKE(GUEST_DND_CONNECT, 0),
    CB_MAGIC_DND_HG_GET_NEXT_HOST_MSG      = VBOX_DND_CB_MAGIC_MAKE(GUEST_DND_GET_NEXT_HOST_MSG, 0),
    CB_MAGIC_DND_HG_ACK_OP                 = VBOX_DND_CB_MAGIC_MAKE(GUEST_DND_HG_ACK_OP, 0),
    CB_MAGIC_DND_HG_REQ_DATA               = VBOX_DND_CB_MAGIC_MAKE(GUEST_DND_HG_REQ_DATA, 0),
    CB_MAGIC_DND_HG_EVT_PROGRESS           = VBOX_DND_CB_MAGIC_MAKE(GUEST_DND_HG_EVT_PROGRESS, 0),
    CB_MAGIC_DND_GH_ACK_PENDING            = VBOX_DND_CB_MAGIC_MAKE(GUEST_DND_GH_ACK_PENDING, 0),
    CB_MAGIC_DND_GH_SND_DATA               = VBOX_DND_CB_MAGIC_MAKE(GUEST_DND_GH_SND_DATA, 0),
    CB_MAGIC_DND_GH_SND_DATA_HDR           = VBOX_DND_CB_MAGIC_MAKE(GUEST_DND_GH_SND_DATA_HDR, 0),
    CB_MAGIC_DND_GH_SND_DIR                = VBOX_DND_CB_MAGIC_MAKE(GUEST_DND_GH_SND_DIR, 0),
    CB_MAGIC_DND_GH_SND_FILE_HDR           = VBOX_DND_CB_MAGIC_MAKE(GUEST_DND_GH_SND_FILE_HDR, 0),
    CB_MAGIC_DND_GH_SND_FILE_DATA          = VBOX_DND_CB_MAGIC_MAKE(GUEST_DND_GH_SND_FILE_DATA, 0),
    CB_MAGIC_DND_GH_EVT_ERROR              = VBOX_DND_CB_MAGIC_MAKE(GUEST_DND_GH_EVT_ERROR, 0)
};

typedef struct VBOXDNDCBHEADERDATA
{
    /** Magic number to identify the structure. */
    uint32_t                    uMagic;
    /** Context ID to identify callback data. */
    uint32_t                    uContextID;
} VBOXDNDCBHEADERDATA, *PVBOXDNDCBHEADERDATA;

typedef struct VBOXDNDCBCONNECTMSGDATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    uint32_t                    uProtocol;
    uint32_t                    uFlags;
} VBOXDNDCBCONNECTMSGDATA, *PVBOXDNDCBCONNECTMSGDATA;

typedef struct VBOXDNDCBDISCONNECTMSGDATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
} VBOXDNDCBDISCONNECTMSGDATA, *PVBOXDNDCBDISCONNECTMSGDATA;

typedef struct VBOXDNDCBHGGETNEXTHOSTMSG
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    uint32_t                    uMsg;
    uint32_t                    cParms;
} VBOXDNDCBHGGETNEXTHOSTMSG, *PVBOXDNDCBHGGETNEXTHOSTMSG;

typedef struct VBOXDNDCBHGGETNEXTHOSTMSGDATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    uint32_t                    uMsg;
    uint32_t                    cParms;
    PVBOXHGCMSVCPARM            paParms;
} VBOXDNDCBHGGETNEXTHOSTMSGDATA, *PVBOXDNDCBHGGETNEXTHOSTMSGDATA;

typedef struct VBOXDNDCBHGACKOPDATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    uint32_t                    uAction;
} VBOXDNDCBHGACKOPDATA, *PVBOXDNDCBHGACKOPDATA;

typedef struct VBOXDNDCBHGREQDATADATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    char                       *pszFormat;
    uint32_t                    cbFormat;
} VBOXDNDCBHGREQDATADATA, *PVBOXDNDCBHGREQDATADATA;

typedef struct VBOXDNDCBHGEVTPROGRESSDATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    uint32_t                    uPercentage;
    uint32_t                    uStatus;
    uint32_t                    rc;
} VBOXDNDCBHGEVTPROGRESSDATA, *PVBOXDNDCBHGEVTPROGRESSDATA;

typedef struct VBOXDNDCBGHACKPENDINGDATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    uint32_t                    uDefAction;
    uint32_t                    uAllActions;
    char                       *pszFormat;
    uint32_t                    cbFormat;
} VBOXDNDCBGHACKPENDINGDATA, *PVBOXDNDCBGHACKPENDINGDATA;

/**
 * Data header.
 * New since protocol v3.
 */
typedef struct VBOXDNDDATAHDR
{
    /** Data transfer flags. Not yet used and must be 0. */
    uint32_t                    uFlags;
    /** Screen ID where the data originates from. */
    uint32_t                    uScreenId;
    /** Total size (in bytes) to transfer. */
    uint64_t                    cbTotal;
    /** Meta data size (in bytes) to transfer.
     *  This size also is part of cbTotal already. */
    uint32_t                    cbMeta;
    /** Meta format buffer. */
    void                       *pvMetaFmt;
    /** Size (in bytes) of meta format buffer. */
    uint32_t                    cbMetaFmt;
    /** Number of objects (files/directories) to transfer. */
    uint64_t                    cObjects;
    /** Compression type. Currently unused, so specify 0.
     **@todo Add IPRT compression type enumeration as soon as it's available. */
    uint32_t                    enmCompression;
    /** Checksum type. Currently unused, so specify RTDIGESTTYPE_INVALID. */
    RTDIGESTTYPE                enmChecksumType;
    /** The actual checksum buffer for the entire data to be transferred,
     *  based on enmChksumType. If RTDIGESTTYPE_INVALID is specified,
     *  no checksum is being used and pvChecksum will be NULL. */
    void                       *pvChecksum;
    /** Size (in bytes) of checksum. */
    uint32_t                    cbChecksum;
} VBOXDNDDATAHDR, *PVBOXDNDSNDDATAHDR;

/* New since protocol v3. */
typedef struct VBOXDNDCBSNDDATAHDRDATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    /** Actual header data. */
    VBOXDNDDATAHDR              data;
} VBOXDNDCBSNDDATAHDRDATA, *PVBOXDNDCBSNDDATAHDRDATA;

typedef struct VBOXDNDSNDDATA
{
    union
    {
        struct
        {
            /** Data block buffer. */
            void                       *pvData;
            /** Size (in bytes) of data block. */
            uint32_t                    cbData;
            /** Total metadata size (in bytes). This is transmitted
             *  with every message because the size can change. */
            uint32_t                    cbTotalSize;
        } v1;
        /* Protocol v2: No changes. */
        struct
        {
            /** Data block buffer. */
            void                       *pvData;
            /** Size (in bytes) of data block. */
            uint32_t                    cbData;
            /** (Rolling) Checksum. Not yet implemented. */
            void                       *pvChecksum;
            /** Size (in bytes) of checksum. Not yet implemented. */
            uint32_t                    cbChecksum;
        } v3;
    } u;
} VBOXDNDSNDDATA, *PVBOXDNDSNDDATA;

typedef struct VBOXDNDCBSNDDATADATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    /** Actual data. */
    VBOXDNDSNDDATA              data;
} VBOXDNDCBSNDDATADATA, *PVBOXDNDCBSNDDATADATA;

typedef struct VBOXDNDCBSNDDIRDATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    /** Directory path. */
    char                       *pszPath;
    /** Size (in bytes) of path. */
    uint32_t                    cbPath;
    /** Directory creation mode. */
    uint32_t                    fMode;
} VBOXDNDCBSNDDIRDATA, *PVBOXDNDCBSNDDIRDATA;

/* Note: Only for protocol version 2 and up (>= VBox 5.0). */
typedef struct VBOXDNDCBSNDFILEHDRDATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    /** File path (name). */
    char                       *pszFilePath;
    /** Size (in bytes) of file path. */
    uint32_t                    cbFilePath;
    /** Total size (in bytes) of this file. */
    uint64_t                    cbSize;
    /** File (creation) mode. */
    uint32_t                    fMode;
    /** Additional flags. Not used at the moment. */
    uint32_t                    fFlags;
} VBOXDNDCBSNDFILEHDRDATA, *PVBOXDNDCBSNDFILEHDRDATA;

typedef struct VBOXDNDCBSNDFILEDATADATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    /** Current file data chunk. */
    void                       *pvData;
    /** Size (in bytes) of current data chunk. */
    uint32_t                    cbData;
    union
    {
        struct
        {
            /** File path (name). */
            char               *pszFilePath;
            /** Size (in bytes) of file path. */
            uint32_t            cbFilePath;
            /** File (creation) mode. */
            uint32_t            fMode;
        } v1;
        /* Protocol v2 + v3: Have the file attributes (name, size, mode, ...)
                             in the VBOXDNDCBSNDFILEHDRDATA structure. */
        struct
        {
            /** Checksum for current file data chunk. */
            void               *pvChecksum;
            /** Size (in bytes) of current data chunk. */
            uint32_t            cbChecksum;
        } v3;
    } u;
} VBOXDNDCBSNDFILEDATADATA, *PVBOXDNDCBSNDFILEDATADATA;

typedef struct VBOXDNDCBEVTERRORDATA
{
    /** Callback data header. */
    VBOXDNDCBHEADERDATA         hdr;
    int32_t                     rc;
} VBOXDNDCBEVTERRORDATA, *PVBOXDNDCBEVTERRORDATA;

} /* namespace DragAndDropSvc */

#endif  /* !___VBox_HostService_DragAndDropSvc_h */

