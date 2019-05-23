/** @file
 * VD: common definitions for the registration, backend and interface structures.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
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

#ifndef __vd_common_h__
#define __vd_common_h__

#include <VBox/types.h>

/** Makes a VD structure version out of an unique magic value and major &
 * minor version numbers.
 *
 * @returns 32-bit structure version number.
 *
 * @param   uMagic      16-bit magic value.  This must be unique.
 * @param   uMajor      12-bit major version number.  Structures with different
 *                      major numbers are not compatible.
 * @param   uMinor      4-bit minor version number.  When only the minor version
 *                      differs, the structures will be 100% backwards
 *                      compatible.
 */
#define VD_VERSION_MAKE(uMagic, uMajor, uMinor) \
    ( ((uint32_t)(uMagic) << 16) | ((uint32_t)((uMajor) & 0xff) << 4) | ((uint32_t)((uMinor) & 0xf) << 0) )

/** Checks if @a uVerMagic1 is compatible with @a uVerMagic2.
 *
 * @returns true / false.
 * @param   uVerMagic1  Typically the runtime version of the struct.  This must
 *                      have the same magic and major version as @a uVerMagic2
 *                      and the minor version must be greater or equal to that
 *                      of @a uVerMagic2.
 * @param   uVerMagic2  Typically the version the code was compiled against.
 *
 * @remarks The parameters will be referenced more than once.
 */
#define VD_VERSION_ARE_COMPATIBLE(uVerMagic1, uVerMagic2) \
    (    (uVerMagic1) == (uVerMagic2) \
      || (   (uVerMagic1) >= (uVerMagic2) \
          && ((uVerMagic1) & UINT32_C(0xfffffff0)) == ((uVerMagic2) & UINT32_C(0xfffffff0)) ) \
    )

#endif
