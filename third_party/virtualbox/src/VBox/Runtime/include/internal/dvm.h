/* $Id: dvm.h $ */
/** @file
 * IPRT - Disk Volume Management Internals.
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

#ifndef ___internal_dvm_h
#define ___internal_dvm_h

#include <iprt/types.h>
#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/vfs.h>
#include "internal/magics.h"

RT_C_DECLS_BEGIN

/** Format specific volume manager handle. */
typedef struct RTDVMFMTINTERNAL *RTDVMFMT;
/** Pointer to a format specific volume manager handle. */
typedef RTDVMFMT                *PRTDVMFMT;
/** NIL volume manager handle. */
#define NIL_RTDVMFMT             ((RTDVMFMT)~0)

/** Format specific volume data handle. */
typedef struct RTDVMVOLUMEFMTINTERNAL *RTDVMVOLUMEFMT;
/** Pointer to a format specific volume data handle. */
typedef RTDVMVOLUMEFMT                *PRTDVMVOLUMEFMT;
/** NIL volume handle. */
#define NIL_RTDVMVOLUMEFMT             ((RTDVMVOLUMEFMT)~0)

/**
 * Disk descriptor.
 */
typedef struct RTDVMDISK
{
    /** Size of the disk in bytes. */
    uint64_t        cbDisk;
    /** Sector size. */
    uint64_t        cbSector;
    /** The VFS file handle if backed by such. */
    RTVFSFILE       hVfsFile;
} RTDVMDISK;
/** Pointer to a disk descriptor. */
typedef RTDVMDISK *PRTDVMDISK;
/** Pointer to a const descriptor. */
typedef const RTDVMDISK *PCRTDVMDISK;

/** Score to indicate that the backend can't handle the format at all */
#define RTDVM_MATCH_SCORE_UNSUPPORTED 0
/** Score to indicate that a backend supports the format
 * but there can be other backends. */
#define RTDVM_MATCH_SCORE_SUPPORTED   (UINT32_MAX/2)
/** Score to indicate a perfect match. */
#define RTDVM_MATCH_SCORE_PERFECT     UINT32_MAX

/**
 * Volume format operations.
 */
typedef struct RTDVMFMTOPS
{
    /** Name of the format. */
    const char         *pszFmt;
    /** The format type.   */
    RTDVMFORMATTYPE     enmFormat;

    /**
     * Probes the given disk for known structures.
     *
     * @returns IPRT status code.
     * @param   pDisk           Disk descriptor.
     * @param   puScore         Where to store the match score on success.
     */
    DECLCALLBACKMEMBER(int, pfnProbe)(PCRTDVMDISK pDisk, uint32_t *puScore);

    /**
     * Opens the format to set up all structures.
     *
     * @returns IPRT status code.
     * @param   pDisk           The disk descriptor.
     * @param   phVolMgrFmt     Where to store the volume format data on success.
     */
    DECLCALLBACKMEMBER(int, pfnOpen)(PCRTDVMDISK pDisk, PRTDVMFMT phVolMgrFmt);

    /**
     * Initializes a new volume map.
     *
     * @returns IPRT status code.
     * @param   pDisk           The disk descriptor.
     * @param   phVolMgrFmt     Where to store the volume format data on success.
     */
    DECLCALLBACKMEMBER(int, pfnInitialize)(PCRTDVMDISK pDisk, PRTDVMFMT phVolMgrFmt);

    /**
     * Closes the volume format.
     *
     * @returns nothing.
     * @param   hVolMgrFmt      The format specific volume manager handle.
     */
    DECLCALLBACKMEMBER(void, pfnClose)(RTDVMFMT hVolMgrFmt);

    /**
     * Returns whether the given range is in use by the volume manager.
     *
     * @returns IPRT status code.
     * @param   hVolMgrFmt      The format specific volume manager handle.
     * @param   offStart        Start offset of the range.
     * @param   cbRange         Size of the range to check in bytes.
     * @param   pfUsed          Where to store whether the range is in use by the
     *                          volume manager.
     */
    DECLCALLBACKMEMBER(int, pfnQueryRangeUse)(RTDVMFMT hVolMgrFmt,
                                              uint64_t off, uint64_t cbRange,
                                              bool *pfUsed);

    /**
     * Gets the number of valid volumes in the map.
     *
     * @returns Number of valid volumes in the map or UINT32_MAX on failure.
     * @param   hVolMgrFmt      The format specific volume manager handle.
     */
    DECLCALLBACKMEMBER(uint32_t, pfnGetValidVolumes)(RTDVMFMT hVolMgrFmt);

    /**
     * Gets the maximum number of volumes the map can have.
     *
     * @returns Maximum number of volumes in the map or 0 on failure.
     * @param   hVolMgrFmt      The format specific volume manager handle.
     */
    DECLCALLBACKMEMBER(uint32_t, pfnGetMaxVolumes)(RTDVMFMT hVolMgrFmt);

    /**
     * Get the first valid volume from a map.
     *
     * @returns IPRT status code.
     * @param   hVolMgrFmt      The format specific volume manager handle.
     * @param   phVolFmt        Where to store the volume handle to the first volume
     *                          on success.
     */
    DECLCALLBACKMEMBER(int, pfnQueryFirstVolume)(RTDVMFMT hVolMgrFmt, PRTDVMVOLUMEFMT phVolFmt);

    /**
     * Get the first valid volume from a map.
     *
     * @returns IPRT status code.
     * @param   hVolMgrFmt      The format specific volume manager handle.
     * @param   hVolFmt         The current volume.
     * @param   phVolFmtNext    Where to store the handle to the format specific
     *                          volume data of the next volume on success.
     */
    DECLCALLBACKMEMBER(int, pfnQueryNextVolume)(RTDVMFMT hVolMgrFmt, RTDVMVOLUMEFMT hVolFmt, PRTDVMVOLUMEFMT phVolFmtNext);

    /**
     * Closes a volume handle.
     *
     * @returns nothing.
     * @param   hVolFmt         The format specific volume handle.
     */
    DECLCALLBACKMEMBER(void, pfnVolumeClose)(RTDVMVOLUMEFMT hVolFmt);

    /**
     * Gets the size of the given volume.
     *
     * @returns Size of the volume in bytes or 0 on failure.
     * @param   hVolFmt         The format specific volume handle.
     */
    DECLCALLBACKMEMBER(uint64_t, pfnVolumeGetSize)(RTDVMVOLUMEFMT hVolFmt);

    /**
     * Queries the name of the given volume.
     *
     * @returns IPRT status code.
     * @param   hVolFmt         The format specific volume handle.
     * @param   ppszVolname     Where to store the name of the volume on success.
     */
    DECLCALLBACKMEMBER(int, pfnVolumeQueryName)(RTDVMVOLUMEFMT hVolFmt, char **ppszVolName);

    /**
     * Get the type of the given volume.
     *
     * @returns The volume type on success, DVMVOLTYPE_INVALID if hVol is invalid.
     * @param   hVolFmt         The format specific volume handle.
     */
    DECLCALLBACKMEMBER(RTDVMVOLTYPE, pfnVolumeGetType)(RTDVMVOLUMEFMT hVolFmt);

    /**
     * Get the flags of the given volume.
     *
     * @returns The volume flags or UINT64_MAX on failure.
     * @param   hVolFmt         The format specific volume handle.
     */
    DECLCALLBACKMEMBER(uint64_t, pfnVolumeGetFlags)(RTDVMVOLUMEFMT hVolFmt);

    /**
     * Returns whether the supplied range is at least partially intersecting
     * with the given volume.
     *
     * @returns whether the range intersects with the volume.
     * @param   hVolFmt         The format specific volume handle.
     * @param   offStart        Start offset of the range.
     * @param   cbRange         Size of the range to check in bytes.
     * @param   poffVol         Where to store the offset of the range from the
     *                          start of the volume if true is returned.
     * @param   pcbIntersect    Where to store the number of bytes intersecting
     *                          with the range if true is returned.
     */
    DECLCALLBACKMEMBER(bool, pfnVolumeIsRangeIntersecting)(RTDVMVOLUMEFMT hVolFmt,
                                                           uint64_t offStart, size_t cbRange,
                                                           uint64_t *poffVol,
                                                           uint64_t *pcbIntersect);

    /**
     * Read data from the given volume.
     *
     * @returns IPRT status code.
     * @param   hVolFmt         The format specific volume handle.
     * @param   off             Where to start reading from.
     * @param   pvBuf           Where to store the read data.
     * @param   cbRead          How many bytes to read.
     */
    DECLCALLBACKMEMBER(int, pfnVolumeRead)(RTDVMVOLUMEFMT hVolFmt, uint64_t off, void *pvBuf, size_t cbRead);

    /**
     * Write data to the given volume.
     *
     * @returns IPRT status code.
     * @param   hVolFmt         The format specific volume handle.
     * @param   off             Where to start writing to.
     * @param   pvBuf           The data to write.
     * @param   cbWrite         How many bytes to write.
     */
    DECLCALLBACKMEMBER(int, pfnVolumeWrite)(RTDVMVOLUMEFMT hVolFmt, uint64_t off, const void *pvBuf, size_t cbWrite);

} RTDVMFMTOPS;
/** Pointer to a DVM ops table. */
typedef RTDVMFMTOPS *PRTDVMFMTOPS;
/** Pointer to a const DVM ops table. */
typedef const RTDVMFMTOPS *PCRTDVMFMTOPS;

/** Checks whether a range is intersecting. */
#define RTDVM_RANGE_IS_INTERSECTING(start, size, off) ( (start) <= (off) && ((start) + (size)) > (off) )

/** Converts a LBA number to the byte offset. */
#define RTDVM_LBA2BYTE(lba, disk) ((lba) * (disk)->cbSector)
/** Converts a Byte offset to the LBA number. */
#define RTDVM_BYTE2LBA(off, disk) ((off) / (disk)->cbSector)

/**
 * Returns the number of sectors in the disk.
 *
 * @returns Number of sectors.
 * @param   pDisk   The disk descriptor.
 */
DECLINLINE(uint64_t) rtDvmDiskGetSectors(PCRTDVMDISK pDisk)
{
    return pDisk->cbDisk / pDisk->cbSector;
}

/**
 * Read from the disk at the given offset.
 *
 * @returns IPRT status code.
 * @param   pDisk    The disk descriptor to read from.
 * @param   off      Start offset.
 * @param   pvBuf    Destination buffer.
 * @param   cbRead   How much to read.
 */
DECLINLINE(int) rtDvmDiskRead(PCRTDVMDISK pDisk, uint64_t off, void *pvBuf, size_t cbRead)
{
    AssertPtrReturn(pDisk, VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf, VERR_INVALID_POINTER);
    AssertReturn(cbRead > 0, VERR_INVALID_PARAMETER);
    AssertReturn(off + cbRead <= pDisk->cbDisk, VERR_INVALID_PARAMETER);

    return RTVfsFileReadAt(pDisk->hVfsFile, off, pvBuf, cbRead, NULL /*pcbRead*/);
}

/**
 * Write to the disk at the given offset.
 *
 * @returns IPRT status code.
 * @param   pDisk    The disk descriptor to write to.
 * @param   off      Start offset.
 * @param   pvBuf    Source buffer.
 * @param   cbWrite  How much to write.
 */
DECLINLINE(int) rtDvmDiskWrite(PCRTDVMDISK pDisk, uint64_t off, const void *pvBuf, size_t cbWrite)
{
    AssertPtrReturn(pDisk, VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf, VERR_INVALID_POINTER);
    AssertReturn(cbWrite > 0, VERR_INVALID_PARAMETER);
    AssertReturn(off + cbWrite <= pDisk->cbDisk, VERR_INVALID_PARAMETER);

    return RTVfsFileWriteAt(pDisk->hVfsFile, off, pvBuf, cbWrite, NULL /*pcbWritten*/);
}

RT_C_DECLS_END

#endif

