/* $Id: vbsfshared.h $ */
/** @file
 *
 * VirtualBox Windows Guest Shared Folders
 *
 * File System Driver header file shared with the network provider dll
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

#ifndef VBSFSHARED_H
#define VBSFSHARED_H

/* The network provider name for shared folders. */
#define MRX_VBOX_PROVIDER_NAME_U L"VirtualBox Shared Folders"

/* The filesystem name for shared folders. */
#define MRX_VBOX_FILESYS_NAME_U L"VBoxSharedFolderFS"

/* The redirector device name. */
#define DD_MRX_VBOX_FS_DEVICE_NAME_U L"\\Device\\VBoxMiniRdr"

#define VBOX_VOLNAME_PREFIX     L"VBOX_"
#define VBOX_VOLNAME_PREFIX_SIZE  (sizeof(VBOX_VOLNAME_PREFIX) - sizeof(VBOX_VOLNAME_PREFIX[0]))

/* Name of symbolic link, which is used by the user mode dll to open the driver. */
#define DD_MRX_VBOX_USERMODE_SHADOW_DEV_NAME_U     L"\\??\\VBoxMiniRdrDN"
#define DD_MRX_VBOX_USERMODE_DEV_NAME_U            L"\\\\.\\VBoxMiniRdrDN"

#define IOCTL_MRX_VBOX_BASE FILE_DEVICE_NETWORK_FILE_SYSTEM

#define _MRX_VBOX_CONTROL_CODE(request, method, access) \
                CTL_CODE(IOCTL_MRX_VBOX_BASE, request, method, access)

/* VBoxSF IOCTL codes. */
#define IOCTL_MRX_VBOX_ADDCONN       _MRX_VBOX_CONTROL_CODE(100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_GETCONN       _MRX_VBOX_CONTROL_CODE(101, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_DELCONN       _MRX_VBOX_CONTROL_CODE(102, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_GETLIST       _MRX_VBOX_CONTROL_CODE(103, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_GETGLOBALLIST _MRX_VBOX_CONTROL_CODE(104, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_GETGLOBALCONN _MRX_VBOX_CONTROL_CODE(105, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_START         _MRX_VBOX_CONTROL_CODE(106, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MRX_VBOX_STOP          _MRX_VBOX_CONTROL_CODE(107, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif /* VBSFSHARED_H */
