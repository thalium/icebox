/** @file
 * X86 (and AMD64) Local APIC registers (VMM,++).
 *
 * bios.mac is generated from this file by running 'kmk -f Maintenance.kmk incs'.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
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

#ifndef ___VBox_bios_h
#define ___VBox_bios_h

/** The BIOS shutdown port.
 * You write "Shutdown" byte by byte to shutdown the VM.
 * @sa VBOX_BIOS_OLD_SHUTDOWN_PORT  */
#define VBOX_BIOS_SHUTDOWN_PORT                 0x040f

/** The old shutdown port number.
 * Older versions of VirtualBox uses this as does Bochs.
 * @sa VBOX_BIOS_SHUTDOWN_PORT  */
#define VBOX_BIOS_OLD_SHUTDOWN_PORT             0x8900


#endif

