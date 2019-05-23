/* $Id: vbsfhlp.h $ */
/** @file
 * VirtualBox Windows Guest Shared Folders - File System Driver helpers
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

#ifndef __VBSFHLP__H
#define __VBSFHLP__H

#include <iprt/nt/nt.h> /* includes ntifs.h and wdm.h */
#include <iprt/win/ntverp.h>

#include <VBox/log.h>
#include <VBox/VBoxGuestLibSharedFolders.h>


void     vbsfHlpSleep(ULONG ulMillies);
NTSTATUS vbsfHlpCreateDriveLetter(WCHAR Letter, UNICODE_STRING *pDeviceName);
NTSTATUS vbsfHlpDeleteDriveLetter(WCHAR Letter);

/**
 * Convert VBox IRT file attributes to NT file attributes
 *
 * @returns NT file attributes
 * @param   fMode       IRT file attributes
 *
 */
uint32_t VBoxToNTFileAttributes(uint32_t fMode);

/**
 * Convert VBox IRT file attributes to NT file attributes
 *
 * @returns NT file attributes
 * @param   fMode       IRT file attributes
 *
 */
uint32_t NTToVBoxFileAttributes(uint32_t fMode);

/**
 * Convert VBox error code to NT status code
 *
 * @returns NT status code
 * @param   vboxRC          VBox error code
 *
 */
NTSTATUS VBoxErrorToNTStatus(int vboxRC);

PVOID    vbsfAllocNonPagedMem(ULONG ulSize);
void     vbsfFreeNonPagedMem(PVOID lpMem);

#if defined(DEBUG) || defined(LOG_ENABLED)
PCHAR MajorFunctionString(UCHAR MajorFunction, LONG MinorFunction);
#endif

NTSTATUS vbsfShflStringFromUnicodeAlloc(PSHFLSTRING *ppShflString, const WCHAR *pwc, uint16_t cb);

#endif /* !__VBSFHLP__H */

