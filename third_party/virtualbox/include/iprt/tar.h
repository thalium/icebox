/** @file
 * IPRT - Tar archive I/O.
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

#ifndef ___iprt_tar_h
#define ___iprt_tar_h

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/time.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_tar    RTTar - Tar archive I/O
 * @ingroup grp_rt
 *
 * @deprecated  Only used for legacy code and writing.  Migrate new code to the
 *              VFS interface, add the write part when needed.
 *
 * @{
 */

/** A tar handle */
typedef R3PTRTYPE(struct RTTARINTERNAL *)        RTTAR;
/** Pointer to a RTTAR interface handle. */
typedef RTTAR                                   *PRTTAR;
/** Nil RTTAR interface handle. */
#define NIL_RTTAR                                ((RTTAR)0)

/** A tar file handle */
typedef R3PTRTYPE(struct RTTARFILEINTERNAL *)    RTTARFILE;
/** Pointer to a RTTARFILE interface handle. */
typedef RTTARFILE                               *PRTTARFILE;
/** Nil RTTARFILE interface handle. */
#define NIL_RTTARFILE                            ((RTTARFILE)0)

/** Maximum length of a tar filename, excluding the terminating '\0'. More
 * does not fit into a tar record. */
#define RTTAR_NAME_MAX                           99

/**
 * Creates a Tar archive.
 *
 * Use the mask to specify the access type.
 *
 * @returns IPRT status code.
 *
 * @param   phTar          Where to store the RTTAR handle.
 * @param   pszTarname     The file name of the tar archive to create.  Should
 *                         not exist.
 * @param   fMode          Open flags, i.e a combination of the RTFILE_O_* defines.
 *                         The ACCESS, ACTION and DENY flags are mandatory!
 */
RTR3DECL(int) RTTarOpen(PRTTAR phTar, const char *pszTarname, uint32_t fMode);

/**
 * Close the Tar archive.
 *
 * @returns IPRT status code.
 *
 * @param   hTar           Handle to the RTTAR interface.
 */
RTR3DECL(int) RTTarClose(RTTAR hTar);

/**
 * Open a file in the Tar archive.
 *
 * @returns IPRT status code.
 *
 * @param   hTar           The handle of the tar archive.
 * @param   phFile         Where to store the handle to the opened file.
 * @param   pszFilename    Path to the file which is to be opened. (UTF-8)
 * @param   fOpen          Open flags, i.e a combination of the RTFILE_O_* defines.
 *                         The ACCESS, ACTION flags are mandatory! DENY flags
 *                         are currently not supported.
 *
 * @remarks Write mode means append mode only. It is not possible to make
 *          changes to existing files.
 *
 * @remarks Currently it is not possible to open more than one file in write
 *          mode. Although open more than one file in read only mode (even when
 *          one file is opened in write mode) is always possible.
 */
RTR3DECL(int) RTTarFileOpen(RTTAR hTar, PRTTARFILE phFile, const char *pszFilename, uint32_t fOpen);

/**
 * Close the file opened by RTTarFileOpen.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          The file handle to close.
 */
RTR3DECL(int) RTTarFileClose(RTTARFILE hFile);

/**
 * Read bytes from a file at a given offset.
 * This function may modify the file position.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   off            Where to read.
 * @param   pvBuf          Where to put the bytes we read.
 * @param   cbToRead       How much to read.
 * @param   pcbRead        Where to return how much we actually read.  If NULL
 *                         an error will be returned for a partial read.
 */
RTR3DECL(int) RTTarFileReadAt(RTTARFILE hFile, uint64_t off, void *pvBuf, size_t cbToRead, size_t *pcbRead);

/**
 * Write bytes to a file at a given offset.
 * This function may modify the file position.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   off            Where to write.
 * @param   pvBuf          What to write.
 * @param   cbToWrite      How much to write.
 * @param   pcbWritten     Where to return how much we actually wrote.  If NULL
 *                         an error will be returned for a partial write.
 */
RTR3DECL(int) RTTarFileWriteAt(RTTARFILE hFile, uint64_t off, const void *pvBuf, size_t cbToWrite, size_t *pcbWritten);

/**
 * Query the size of the file.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   pcbSize        Where to store the filesize.
 */
RTR3DECL(int) RTTarFileGetSize(RTTARFILE hFile, uint64_t *pcbSize);

/**
 * Set the size of the file.
 *
 * @returns IPRT status code.
 *
 * @param   hFile          Handle to the file.
 * @param   cbSize         The new file size.
 */
RTR3DECL(int) RTTarFileSetSize(RTTARFILE hFile, uint64_t cbSize);

/** @} */

RT_C_DECLS_END

#endif

