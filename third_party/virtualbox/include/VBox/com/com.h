/** @file
 * MS COM / XPCOM Abstraction Layer - COM initialization / shutdown.
 */

/*
 * Copyright (C) 2005-2017 Oracle Corporation
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

#ifndef ___VBox_com_com_h
#define ___VBox_com_com_h

#include "VBox/com/defs.h"

/** @defgroup grp_com   MS COM / XPCOM Abstraction Layer
 * @{
 */

namespace com
{

/**
 *  Initializes the COM runtime.
 *  Must be called on the main thread, before any COM activity in any thread, and by any thread
 *  willing to perform COM operations.
 *
 *  @param fGui             if call is performed on the GUI thread
 *  @param fAutoRegUpdate   if to do auto MS COM registration updates.
 *  @return COM result code
 */
HRESULT Initialize(bool fGui = false, bool fAutoRegUpdate = true);

/**
 *  Shuts down the COM runtime.
 *  Must be called on the main thread before termination.
 *  No COM calls may be made in any thread after this method returns.
 */
HRESULT Shutdown();

/**
 *  Resolves a given interface ID to a string containing the interface name.
 *  If, for some reason, the given IID cannot be resolved to a name, a NULL
 *  string is returned. A non-NULL string returned by this function must be
 *  freed using SysFreeString().
 *
 *  @param aIID     ID of the interface to get a name for
 *  @param aName    Resolved interface name or @c NULL on error
 */
void GetInterfaceNameByIID(const GUID &aIID, BSTR *aName);

/**
 *  Returns the VirtualBox user home directory.
 *
 *  On failure, this function will return a path that caused a failure (or
 *  NULL if the failure is not path-related).
 *
 *  On success, this function will try to create the returned directory if it
 *  doesn't exist yet. This may also fail with the corresponding status code.
 *
 *  If @a aDirLen is smaller than RTPATH_MAX then there is a great chance that
 *  this method will return VERR_BUFFER_OVERFLOW.
 *
 *  @param aDir        Buffer to store the directory string in UTF-8 encoding.
 *  @param aDirLen     Length of the supplied buffer including space for the
 *                     terminating null character, in bytes.
 *  @param fCreateDir  Flag whether to create the returned directory on success if it
 *                     doesn't exist.
 *  @return            VBox status code.
 */
int GetVBoxUserHomeDirectory(char *aDir, size_t aDirLen, bool fCreateDir = true);

/**
 *  Creates a release log file, used both in VBoxSVC and in API clients.
 *
 *  @param pcszEntity       Human readable name of the program.
 *  @param pcszLogFile      Name of the release log file.
 *  @param fFlags           Logger instance flags.
 *  @param pcszGroupSettings Group logging settings.
 *  @param pcszEnvVarBase   Base environment variable name for the logger.
 *  @param fDestFlags       Logger destination flags.
 *  @param cMaxEntriesPerGroup Limit for log entries per group. UINT32_MAX for no limit.
 *  @param cHistory         Number of old log files to keep.
 *  @param uHistoryFileTime Maximum amount of time to put in a log file.
 *  @param uHistoryFileSize Maximum size of a log file before rotating.
 *  @param pszError         In case of creation failure: buffer for error message.
 *  @param cbError          Size of error message buffer.
 *  @return         VBox status code.
 */
int VBoxLogRelCreate(const char *pcszEntity, const char *pcszLogFile,
                     uint32_t fFlags, const char *pcszGroupSettings,
                     const char *pcszEnvVarBase, uint32_t fDestFlags,
                     uint32_t cMaxEntriesPerGroup, uint32_t cHistory,
                     uint32_t uHistoryFileTime, uint64_t uHistoryFileSize,
                     char *pszError, size_t cbError);

} /* namespace com */

/** @} */
#endif

