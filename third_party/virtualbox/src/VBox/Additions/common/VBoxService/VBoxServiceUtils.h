/* $Id: VBoxServiceUtils.h $ */
/** @file
 * VBoxServiceUtils - Guest Additions Services (Utilities).
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBoxServiceUtils_h
#define ___VBoxServiceUtils_h

#include "VBoxServiceInternal.h"

#ifdef VBOX_WITH_GUEST_PROPS
int VGSvcReadProp(uint32_t u32ClientId, const char *pszPropName, char **ppszValue, char **ppszFlags, uint64_t *puTimestamp);
int VGSvcReadPropUInt32(uint32_t u32ClientId, const char *pszPropName, uint32_t *pu32, uint32_t u32Min, uint32_t u32Max);
int VGSvcCheckPropExist(uint32_t u32ClientId, const char *pszPropName);
int VGSvcReadHostProp(uint32_t u32ClientId, const char *pszPropName, bool fReadOnly, char **ppszValue, char **ppszFlags,
                      uint64_t *puTimestamp);
int VGSvcWritePropF(uint32_t u32ClientId, const char *pszName, const char *pszValueFormat, ...);
#endif

#ifdef RT_OS_WINDOWS
int VGSvcUtilWinGetFileVersionString(const char *pszPath, const char *pszFileName, char *pszVersion, size_t cbVersion);
#endif

#endif

