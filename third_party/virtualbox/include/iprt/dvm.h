/** @file
 * IPRT Disk Volume Management API (DVM).
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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

#ifndef ___iprt_dvm_h
#define ___iprt_dvm_h

#include <iprt/types.h>

RT_C_DECLS_BEGIN


/** @defgroup grp_dvm           IPRT Disk Volume Management
 * @{
 */

/**
 * Volume type.
 * Comparable to the FS type in MBR partition maps
 * or the partition type GUIDs in GPT tables.
 */
typedef enum RTDVMVOLTYPE
{
    /** Invalid. */
    RTDVMVOLTYPE_INVALID = 0,
    /** Unknown. */
    RTDVMVOLTYPE_UNKNOWN,
    /** Volume hosts a NTFS filesystem. */
    RTDVMVOLTYPE_NTFS,
    /** Volume hosts a FAT16 filesystem. */
    RTDVMVOLTYPE_FAT16,
    /** Volume hosts a FAT32 filesystem. */
    RTDVMVOLTYPE_FAT32,
    /** Volume hosts a Linux swap. */
    RTDVMVOLTYPE_LINUX_SWAP,
    /** Volume hosts a Linux filesystem. */
    RTDVMVOLTYPE_LINUX_NATIVE,
    /** Volume hosts a Linux LVM. */
    RTDVMVOLTYPE_LINUX_LVM,
    /** Volume hosts a Linux SoftRaid. */
    RTDVMVOLTYPE_LINUX_SOFTRAID,
    /** Volume hosts a FreeBSD disklabel. */
    RTDVMVOLTYPE_FREEBSD,
    /** Volume hosts a NetBSD disklabel. */
    RTDVMVOLTYPE_NETBSD,
    /** Volume hosts a OpenBSD disklabel. */
    RTDVMVOLTYPE_OPENBSD,
    /** Volume hosts a Mac OS X HFS or HFS+ filesystem. */
    RTDVMVOLTYPE_MAC_OSX_HFS,
    /** Volume hosts a Solaris volume. */
    RTDVMVOLTYPE_SOLARIS,
    /** End of the valid values. */
    RTDVMVOLTYPE_END,
    /** Usual 32bit hack. */
    RTDVMVOLTYPE_32BIT_HACK = 0x7fffffff
} RTDVMVOLTYPE;

/** @defgroup grp_dvm_flags     Flags used by RTDvmCreate.
 * @{ */
/** DVM flags - Blocks are always marked as unused if the volume has
 *              no block status callback set.
 *              The default is to mark them as used. */
#define DVM_FLAGS_NO_STATUS_CALLBACK_MARK_AS_UNUSED    RT_BIT_32(0)
/** DVM flags - Space which is unused in the map will be marked as used
 *              when calling RTDvmMapQueryBlockStatus(). */
#define DVM_FLAGS_UNUSED_SPACE_MARK_AS_USED            RT_BIT_32(1)
/** Mask of all valid flags. */
#define DVM_FLAGS_MASK (DVM_FLAGS_NO_STATUS_CALLBACK_MARK_AS_UNUSED | DVM_FLAGS_UNUSED_SPACE_MARK_AS_USED)
/** @}  */


/** @defgroup grp_dvm_vol_flags     Volume flags used by DVMVolumeGetFlags.
 * @{ */
/** Volume flags - Volume is bootable. */
#define DVMVOLUME_FLAGS_BOOTABLE    RT_BIT_64(0)
/** Volume flags - Volume is active. */
#define DVMVOLUME_FLAGS_ACTIVE      RT_BIT_64(1)
/** @}  */

/** A handle to a volume manager. */
typedef struct RTDVMINTERNAL       *RTDVM;
/** A pointer to a volume manager handle. */
typedef RTDVM                      *PRTDVM;
/** NIL volume manager handle. */
#define NIL_RTDVM                   ((RTDVM)~0)

/** A handle to a volume in a volume map. */
typedef struct RTDVMVOLUMEINTERNAL *RTDVMVOLUME;
/** A pointer to a volume handle. */
typedef RTDVMVOLUME                *PRTDVMVOLUME;
/** NIL volume handle. */
#define NIL_RTDVMVOLUME             ((RTDVMVOLUME)~0)

/**
 * Callback to read data from the underlying medium.
 *
 * @returns IPRT status code.
 * @param   pvUser    Opaque user data passed on creation.
 * @param   off       Offset to start reading from.
 * @param   pvBuf     Where to store the read data.
 * @param   cbRead    How many bytes to read.
 */
typedef DECLCALLBACK(int) FNDVMREAD(void *pvUser, uint64_t off, void *pvBuf, size_t cbRead);
/** Pointer to a read callback. */
typedef FNDVMREAD *PFNDVMREAD;

/**
 * Callback to write data to the underlying medium.
 *
 * @returns IPRT status code.
 * @param   pvUser    Opaque user data passed on creation.
 * @param   off       Offset to start writing to.
 * @param   pvBuf     The data to write.
 * @param   cbRead    How many bytes to write.
 */
typedef DECLCALLBACK(int) FNDVMWRITE(void *pvUser, uint64_t off, const void *pvBuf, size_t cbWrite);
/** Pointer to a read callback. */
typedef FNDVMWRITE *PFNDVMWRITE;

/**
 * Callback for querying the block allocation status of a volume.
 *
 * @returns IPRT status code.
 * @param   pvUser         Opaque user data passed when setting the callback.
 * @param   off            Offset relative to the start of the volume.
 * @param   cb             Range to check in bytes.
 * @param   pfAllocated    Where to store the allocation status on success.
 */
typedef DECLCALLBACK(int) FNDVMVOLUMEQUERYBLOCKSTATUS(void *pvUser, uint64_t off,
                                                      uint64_t cb, bool *pfAllocated);
/** Pointer to a query block allocation status callback. */
typedef FNDVMVOLUMEQUERYBLOCKSTATUS *PFNDVMVOLUMEQUERYBLOCKSTATUS;

/**
 * Create a new volume manager.
 *
 * @returns IPRT status.
 * @param   phVolMgr    Where to store the handle to the volume manager on
 *                      success.
 * @param   pfnRead     Read callback for the underlying
 *                      disk/container/whatever.
 * @param   pfnWrite    Write callback for the underlying
 *                      disk/container/whatever.
 * @param   cbDisk      Size of the underlying disk in bytes.
 * @param   cbSector    Size of one sector in bytes.
 * @param   fFlags      Combination of RTDVM_FLAGS_*
 * @param   pvUser      Opaque user data passed to the callbacks.
 */
RTDECL(int) RTDvmCreate(PRTDVM phVolMgr, PFNDVMREAD pfnRead,
                        PFNDVMWRITE pfnWrite, uint64_t cbDisk,
                        uint64_t cbSector, uint32_t fFlags,
                        void *pvUser);

/**
 * Retain a given volume manager.
 *
 * @returns New reference count on success, UINT32_MAX on failure.
 * @param   hVolMgr     The volume manager to retain.
 */
RTDECL(uint32_t) RTDvmRetain(RTDVM hVolMgr);

/**
 * Releases a given volume manager.
 *
 * @returns New reference count on success (0 if closed), UINT32_MAX on failure.
 * @param   hVolMgr     The volume manager to release.
 */
RTDECL(uint32_t) RTDvmRelease(RTDVM hVolMgr);

/**
 * Probes the underyling disk for the best volume manager format handler
 * and opens it.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if no backend can handle the volume map on the disk.
 * @param   hVolMgr     The volume manager handle.
 */
RTDECL(int) RTDvmMapOpen(RTDVM hVolMgr);

/**
 * Initializes a new volume map using the given format handler.
 *
 * @returns IPRT status code.
 * @param   hVolMgr     The volume manager handle.
 * @param   pszFmt      The format to use for the new map.
 */
RTDECL(int) RTDvmMapInitialize(RTDVM hVolMgr, const char *pszFmt);

/**
 * Gets the currently used format of the disk map.
 *
 * @returns Name of the format.
 * @param   hVolMgr     The volume manager handle.
 */
RTDECL(const char *) RTDvmMapGetFormat(RTDVM hVolMgr);

/**
 * Gets the number of valid partitions in the map.
 *
 * @returns The number of valid volumes in the map or UINT32_MAX on failure.
 * @param   hVolMgr     The volume manager handle.
 */
RTDECL(uint32_t) RTDvmMapGetValidVolumes(RTDVM hVolMgr);

/**
 * Gets the maximum number of partitions the map can hold.
 *
 * @returns The maximum number of volumes in the map or UINT32_MAX on failure.
 * @param   hVolMgr         The volume manager handle.
 */
RTDECL(uint32_t) RTDvmMapGetMaxVolumes(RTDVM hVolMgr);

/**
 * Get the first valid volume from a map.
 *
 * @returns IPRT status code.
 * @param   hVolMgr         The volume manager handle.
 * @param   phVol           Where to store the handle to the first volume on
 *                          success. Release with RTDvmVolumeRelease().
 */
RTDECL(int) RTDvmMapQueryFirstVolume(RTDVM hVolMgr, PRTDVMVOLUME phVol);

/**
 * Get the first valid volume from a map.
 *
 * @returns IPRT status code.
 * @param   hVolMgr         The volume manager handle.
 * @param   hVol            Handle of the current volume.
 * @param   phVolNext       Where to store the handle to the next volume on
 *                          success. Release with RTDvmVolumeRelease().
 */
RTDECL(int) RTDvmMapQueryNextVolume(RTDVM hVolMgr, RTDVMVOLUME hVol, PRTDVMVOLUME phVolNext);

/**
 * Returns whether the given block on the disk is in use.
 *
 * @returns IPRT status code.
 * @param   hVolMgr         The volume manager handler.
 * @param   off             The start offset to check for.
 * @param   cb              The range in bytes to check.
 * @param   pfAllocated     Where to store the status on success.
 *
 * @remark This method will return true even if a part of the range is not in use.
 */
RTDECL(int) RTDvmMapQueryBlockStatus(RTDVM hVolMgr, uint64_t off, uint64_t cb,
                                     bool *pfAllocated);

/**
 * Retains a valid volume handle.
 *
 * @returns New reference count on success, UINT32_MAX on failure.
 * @param   hVol            The volume to retain.
 */
RTDECL(uint32_t) RTDvmVolumeRetain(RTDVMVOLUME hVol);

/**
 * Releases a valid volume handle.
 *
 * @returns New reference count on success (0 if closed), UINT32_MAX on failure.
 * @param   hVol            The volume to release.
 */
RTDECL(uint32_t) RTDvmVolumeRelease(RTDVMVOLUME hVol);

/**
 * Sets the callback to query the block allocation status for a volume.
 * This overwrites any other callback set previously.
 *
 * @returns nothing.
 * @param   hVol                   The volume handle.
 * @param   pfnQueryBlockStatus    The callback to set. Can be NULL to disable
 *                                 a previous callback.
 * @param   pvUser                 Opaque user data passed in the callback.
 */
RTDECL(void) RTDvmVolumeSetQueryBlockStatusCallback(RTDVMVOLUME hVol,
                                                    PFNDVMVOLUMEQUERYBLOCKSTATUS pfnQueryBlockStatus,
                                                    void *pvUser);

/**
 * Get the size of a volume in bytes.
 *
 * @returns Size of the volume in bytes or 0 on failure.
 * @param   hVol            The volume handle.
 */
RTDECL(uint64_t) RTDvmVolumeGetSize(RTDVMVOLUME hVol);

/**
 * Gets the name of the volume if supported.
 *
 * @returns IPRT status code.
 * @param   hVol            The volume handle.
 * @param   ppszVolName     Where to store the name of the volume on success.
 *                          The string must be freed with RTStrFree().
 */
RTDECL(int) RTDvmVolumeQueryName(RTDVMVOLUME hVol, char **ppszVolName);

/**
 * Get the volume type of the volume if supported.
 *
 * @returns The volume type on success, DVMVOLTYPE_INVALID if hVol is invalid.
 * @param   hVol            The volume handle.
 */
RTDECL(RTDVMVOLTYPE) RTDvmVolumeGetType(RTDVMVOLUME hVol);

/**
 * Get the volume flags of the volume if supported.
 *
 * @returns The volume flags or UINT64_MAX on failure.
 * @param   hVol            The volume handle.
 */
RTDECL(uint64_t) RTDvmVolumeGetFlags(RTDVMVOLUME hVol);

/**
 * Reads data from the given volume.
 *
 * @returns IPRT status code.
 * @param   hVol            The volume handle.
 * @param   off             Where to start reading from - 0 is the beginning of
 *                          the volume.
 * @param   pvBuf           Where to store the read data.
 * @param   cbRead          How many bytes to read.
 */
RTDECL(int) RTDvmVolumeRead(RTDVMVOLUME hVol, uint64_t off, void *pvBuf, size_t cbRead);

/**
 * Writes data to the given volume.
 *
 * @returns IPRT status code.
 * @param   hVol            The volume handle.
 * @param   off             Where to start writing to - 0 is the beginning of
 *                          the volume.
 * @param   pvBuf           The data to write.
 * @param   cbWrite         How many bytes to write.
 */
RTDECL(int) RTDvmVolumeWrite(RTDVMVOLUME hVol, uint64_t off, const void *pvBuf, size_t cbWrite);

/**
 * Returns the description of a given volume type.
 *
 * @returns The description of the type.
 * @param   enmVolType    The volume type.
 */
RTDECL(const char *) RTDvmVolumeTypeGetDescr(RTDVMVOLTYPE enmVolType);

/**
 * Creates an VFS file from a volume handle.
 *
 * @returns IPRT status code.
 * @param   hVol            The volume handle.
 * @param   phVfsFileOut    Where to store the VFS file handle on success.
 */
RTDECL(int) RTDvmVolumeCreateVfsFile(RTDVMVOLUME hVol, PRTVFSFILE phVfsFileOut);

RT_C_DECLS_END

/** @} */

#endif

