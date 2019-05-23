/* $Id: DevPcBios.h $ */
/** @file
 * DevPcBios - PC BIOS Device, header shared with the BIOS code.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef DEV_PCBIOS_H
#define DEV_PCBIOS_H

/** @def VBOX_DMI_TABLE_BASE */
#define VBOX_DMI_TABLE_BASE         0xe1000
#define VBOX_DMI_TABLE_VER          0x25

/** def VBOX_DMI_TABLE_SIZE
 *
 * The size should be at least 16-byte aligned for a proper alignment of
 * the MPS table.
 */
#define VBOX_DMI_TABLE_SIZE         768


/** @def VBOX_LANBOOT_SEG
 *
 * Should usually start right after the DMI BIOS page
 */
#define VBOX_LANBOOT_SEG            0xe200

#define VBOX_SMBIOS_MAJOR_VER       2
#define VBOX_SMBIOS_MINOR_VER       5
#define VBOX_SMBIOS_MAXSS           0xff   /* Not very accurate */

#endif
