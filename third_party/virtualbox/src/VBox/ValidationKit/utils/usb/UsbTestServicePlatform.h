/* $Id: UsbTestServicePlatform.h $ */
/** @file
 * UsbTestServ - Remote USB test configuration and execution server, Platform specific helpers.
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

#ifndef ___UsbTestServicePlatform_h___
#define ___UsbTestServicePlatform_h___

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/**
 * Initializes the platform specific structures for UTS.
 *
 * @returns IPRT status code.
 */
DECLHIDDEN(int) utsPlatformInit(void);

/**
 * Frees all platform specific structures for UTS.
 */
DECLHIDDEN(void) utsPlatformTerm(void);

/**
 * Loads the specified kernel module on the platform.
 *
 * @returns IPRT status code.
 * @param   pszModule         The module to load.
 * @param   papszArgv         Array of arguments to pass to the module.
 * @param   cArgv             Number of argument array entries.
 */
DECLHIDDEN(int) utsPlatformModuleLoad(const char *pszModule, const char **papszArgv,
                                      unsigned cArgv);

/**
 * Unloads the specified kernel module on the platform.
 *
 * @returns IPRT status code.
 * @param   pszModule         The module to unload.
 */
DECLHIDDEN(int) utsPlatformModuleUnload(const char *pszModule);

#ifdef RT_OS_LINUX

/**
 * Acquires a free UDC to attach a gadget to.
 *
 * @returns IPRT status code.
 * @param   fSuperSpeed       Flag whether a super speed bus is required.
 * @param   ppszUdc           Where to store the pointer to the name of the UDC on success.
 *                            Free with RTStrFree().
 * @param   puBusId           Where to store the bus ID the UDC is attached to on the host side.
 */
DECLHIDDEN(int) utsPlatformLnxAcquireUDC(bool fSuperSpeed, char **ppszUdc, uint32_t *puBusId);

/**
 * Releases the given UDC for other use.
 *
 * @returns IPRT status code.
 * @param   pszUdc            The UDC to release.
 */
DECLHIDDEN(int) utsPlatformLnxReleaseUDC(const char *pszUdc);

#endif

RT_C_DECLS_END

#endif

