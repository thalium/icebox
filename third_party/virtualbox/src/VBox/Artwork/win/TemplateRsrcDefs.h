/* $Id: TemplateRsrcDefs.h $ */
/** @file
 * Defines for templates that does not have Windows.h handy.
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
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


#ifdef IN_ICON_FILE
IDI_ICON1 ICON IN_ICON_FILE
#endif

#define VS_VERSION_INFO         1
#define VS_FFI_FILEFLAGSMASK    0x3f
#define VS_FF_DEBUG             0x01
#define VS_FF_PRERELEASE        0x02
#define VS_FF_PATCHED           0x03
#define VS_FF_PRIVATEBUILD      0x08
#define VS_FF_INFOINFERRED      0x10
#define VS_FF_SPECIALBUILD      0x20
#define VFT_DRV                 3
#define VFT2_UNKNOWN            0
#define VOS_NT_WINDOWS32        0x40004
#define LANG_ENGLISH            0x409
#define SUBLANG_ENGLISH_US      1

