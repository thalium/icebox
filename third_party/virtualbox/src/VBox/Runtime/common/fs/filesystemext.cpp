/* $Id: filesystemext.cpp $ */
/** @file
 * IPRT Filesystem API (FileSys) - ext2/3 format.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEFAULT
#include <iprt/types.h>
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/filesystem.h>
#include <iprt/string.h>
#include <iprt/vfs.h>
#include "internal/filesystem.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/*
 * The filesystem structures are from http://wiki.osdev.org/Ext2 and
 * http://www.nongnu.org/ext2-doc/ext2.html
 */

/**
 * Ext superblock.
 *
 * Everything is stored little endian on the disk.
 */
#pragma pack(1)
typedef struct ExtSuperBlock
{
    /** Total number of inodes in the filesystem. */
    uint32_t    cInodesTotal;
    /** Total number of blocks in the filesystem. */
    uint32_t    cBlocksTotal;
    /** Number of blocks reserved for the super user. */
    uint32_t    cBlocksRsvdForSuperUser;
    /** Total number of unallocated blocks. */
    uint32_t    cBlocksUnallocated;
    /** Total number of unallocated inodes. */
    uint32_t    cInodesUnallocated;
    /** Block number of block containing the superblock. */
    uint32_t    iBlockOfSuperblock;
    /** Number of bits to shift 1024 to the left to get the block size */
    uint32_t    cBitsShiftLeftBlockSize;
    /** Number of bits to shift 1024 to the left to get the fragment size */
    uint32_t    cBitsShiftLeftFragmentSize;
    /** Number of blocks in each block group. */
    uint32_t    cBlocksPerGroup;
    /** Number of fragments in each block group. */
    uint32_t    cFragmentsPerBlockGroup;
    /** Number of inodes in each block group. */
    uint32_t    cInodesPerBlockGroup;
    /** Last mount time. */
    uint32_t    u32LastMountTime;
    /** Last written time. */
    uint32_t    u32LastWrittenTime;
    /** Number of times the volume was mounted since the last check. */
    uint16_t    cMountsSinceLastCheck;
    /** Number of mounts allowed before a consistency check. */
    uint16_t    cMaxMountsUntilCheck;
    /** Signature to identify a ext2 volume. */
    uint16_t    u16Signature;
    /** State of the filesystem (clean/errors) */
    uint16_t    u16FilesystemState;
    /** What to do on an error. */
    uint16_t    u16ActionOnError;
    /** Minor version field. */
    uint16_t    u16VersionMinor;
    /** Time of last check. */
    uint32_t    u32LastCheckTime;
    /** Interval between consistency checks. */
    uint32_t    u32CheckInterval;
    /** Operating system ID of the filesystem creator. */
    uint32_t    u32OsIdCreator;
    /** Major version field. */
    uint32_t    u32VersionMajor;
    /** User ID that is allowed to use reserved blocks. */
    uint16_t    u16UidReservedBlocks;
    /** Group ID that is allowed to use reserved blocks. */
    uint16_t    u16GidReservedBlocks;
    /** Reserved fields. */
    uint8_t     abReserved[940];
} ExtSuperBlock;
#pragma pack()
AssertCompileSize(ExtSuperBlock, 1024);
/** Pointer to an ext super block. */
typedef ExtSuperBlock *PExtSuperBlock;

/** Ext2 signature. */
#define RTFILESYSTEM_EXT2_SIGNATURE    (0xef53)
/** Clean filesystem state. */
#define RTFILESYSTEM_EXT2_STATE_CLEAN  (0x0001)
/** Error filesystem state. */
#define RTFILESYSTEM_EXT2_STATE_ERRORS (0x0002)

/**
 * Block group descriptor.
 */
#pragma pack(1)
typedef struct BlockGroupDesc
{
    /** Block address of the block bitmap. */
    uint32_t    offBlockBitmap;
    /** Block address of the inode bitmap. */
    uint32_t    offInodeBitmap;
    /** Start block address of the inode table. */
    uint32_t    offInodeTable;
    /** Number of unallocated blocks in group. */
    uint16_t    cBlocksUnallocated;
    /** Number of unallocated inodes in group. */
    uint16_t    cInodesUnallocated;
    /** Number of directories in the group. */
    uint16_t    cDirectories;
    /** Padding. */
    uint16_t    u16Padding;
    /** Reserved. */
    uint8_t     abReserved[12];
} BlockGroupDesc;
#pragma pack()
AssertCompileSize(BlockGroupDesc, 32);
/** Pointer to an ext block group descriptor. */
typedef BlockGroupDesc *PBlockGroupDesc;

/**
 * Cached block group descriptor data.
 */
typedef struct RTFILESYSTEMEXTBLKGRP
{
    /** Start offset (in bytes and from the start of the disk). */
    uint64_t    offStart;
    /** Last offset in the block group (inclusive). */
    uint64_t    offLast;
    /** Block bitmap - variable in size (depends on the block size
     * and number of blocks per group). */
    uint8_t     abBlockBitmap[1];
} RTFILESYSTEMEXTBLKGRP;
/** Pointer to block group descriptor data. */
typedef RTFILESYSTEMEXTBLKGRP *PRTFILESYSTEMEXTBLKGRP;

/**
 * Ext2/3 filesystem data.
 */
typedef struct RTFILESYSTEMEXT
{
    /** VFS file handle. */
    RTVFSFILE                 hVfsFile;
    /** Block number of the superblock. */
    uint32_t                  iSbBlock;
    /** Size of one block. */
    size_t                    cbBlock;
    /** Number of blocks in one group. */
    unsigned                  cBlocksPerGroup;
    /** Number of blocks groups in the volume. */
    unsigned                  cBlockGroups;
    /** Cached block group descriptor data. */
    PRTFILESYSTEMEXTBLKGRP    pBlkGrpDesc;
} RTFILESYSTEMEXT;
/** Pointer to the ext filesystem data. */
typedef RTFILESYSTEMEXT *PRTFILESYSTEMEXT;


/*********************************************************************************************************************************
*   Methods                                                                                                                      *
*********************************************************************************************************************************/

/**
 * Loads the block descriptor of the given block group from the medium.
 *
 * @returns IPRT status code.
 * @param   pThis    EXT filesystem instance data.
 * @param   iBlkGrp  Block group number to load.
 */
static int rtFsExtLoadBlkGrpDesc(PRTFILESYSTEMEXT pThis, uint32_t iBlkGrp)
{
    int rc = VINF_SUCCESS;
    PRTFILESYSTEMEXTBLKGRP pBlkGrpDesc = pThis->pBlkGrpDesc;
    uint64_t offRead = (pThis->iSbBlock + 1) * pThis->cbBlock;
    BlockGroupDesc BlkDesc;
    size_t cbBlockBitmap;

    cbBlockBitmap = pThis->cBlocksPerGroup / 8;
    if (pThis->cBlocksPerGroup % 8)
        cbBlockBitmap++;

    if (!pBlkGrpDesc)
    {
        size_t cbBlkDesc = RT_OFFSETOF(RTFILESYSTEMEXTBLKGRP, abBlockBitmap[cbBlockBitmap]);
        pBlkGrpDesc = (PRTFILESYSTEMEXTBLKGRP)RTMemAllocZ(cbBlkDesc);
        if (!pBlkGrpDesc)
            return VERR_NO_MEMORY;
    }

    rc = RTVfsFileReadAt(pThis->hVfsFile, offRead, &BlkDesc, sizeof(BlkDesc), NULL);
    if (RT_SUCCESS(rc))
    {
        pBlkGrpDesc->offStart = pThis->iSbBlock + (uint64_t)iBlkGrp * pThis->cBlocksPerGroup * pThis->cbBlock;
        pBlkGrpDesc->offLast  = pBlkGrpDesc->offStart + pThis->cBlocksPerGroup * pThis->cbBlock;
        rc = RTVfsFileReadAt(pThis->hVfsFile, BlkDesc.offBlockBitmap * pThis->cbBlock,
                             &pBlkGrpDesc->abBlockBitmap[0], cbBlockBitmap, NULL);
    }

    pThis->pBlkGrpDesc = pBlkGrpDesc;

    return rc;
}

static bool rtFsExtIsBlockRangeInUse(PRTFILESYSTEMEXTBLKGRP pBlkGrpDesc,
                                     uint32_t offBlockStart, size_t cBlocks)
{
    bool fUsed = false;

    while (cBlocks)
    {
        uint32_t idxByte = offBlockStart / 8;
        uint32_t iBit = offBlockStart % 8;

        if (pBlkGrpDesc->abBlockBitmap[idxByte] & RT_BIT(iBit))
        {
            fUsed = true;
            break;
        }

        cBlocks--;
        offBlockStart++;
    }

    return fUsed;
}


static DECLCALLBACK(int) rtFsExtProbe(RTVFSFILE hVfsFile, uint32_t *puScore)
{
    int rc = VINF_SUCCESS;
    uint64_t cbMedium = 0;

    *puScore = RTFILESYSTEM_MATCH_SCORE_UNSUPPORTED;

    rc = RTVfsFileGetSize(hVfsFile, &cbMedium);
    if (RT_SUCCESS(rc))
    {
        if (cbMedium >= 2*sizeof(ExtSuperBlock))
        {
            ExtSuperBlock SuperBlock;

            rc = RTVfsFileReadAt(hVfsFile, 1024, &SuperBlock, sizeof(ExtSuperBlock), NULL);
            if (RT_SUCCESS(rc))
            {
#if defined(RT_BIGENDIAN)
                /** @todo Convert to host endianess. */
#endif
                if (SuperBlock.u16Signature == RTFILESYSTEM_EXT2_SIGNATURE)
                    *puScore = RTFILESYSTEM_MATCH_SCORE_SUPPORTED;
            }
        }
    }

    return rc;
}

static DECLCALLBACK(int) rtFsExtInit(void *pvThis, RTVFSFILE hVfsFile)
{
    int rc = VINF_SUCCESS;
    PRTFILESYSTEMEXT pThis = (PRTFILESYSTEMEXT)pvThis;
    ExtSuperBlock SuperBlock;

    pThis->hVfsFile    = hVfsFile;
    pThis->pBlkGrpDesc = NULL;

    rc = RTVfsFileReadAt(hVfsFile, 1024, &SuperBlock, sizeof(ExtSuperBlock), NULL);
    if (RT_SUCCESS(rc))
    {
#if defined(RT_BIGENDIAN)
        /** @todo Convert to host endianess. */
#endif
        if (SuperBlock.u16FilesystemState == RTFILESYSTEM_EXT2_STATE_ERRORS)
            rc = VERR_FILESYSTEM_CORRUPT;
        else
        {
            pThis->iSbBlock = SuperBlock.iBlockOfSuperblock;
            pThis->cbBlock = 1024 << SuperBlock.cBitsShiftLeftBlockSize;
            pThis->cBlocksPerGroup = SuperBlock.cBlocksPerGroup;
            pThis->cBlockGroups = SuperBlock.cBlocksTotal / pThis->cBlocksPerGroup;

            /* Load first block group descriptor. */
            rc = rtFsExtLoadBlkGrpDesc(pThis, 0);
        }
    }

    return rc;
}


/**
 * @interface_method_impl{RTVFSOBJOPS::Obj,pfnClose}
 */
static DECLCALLBACK(int) rtFsExtVol_Close(void *pvThis)
{
    PRTFILESYSTEMEXT pThis = (PRTFILESYSTEMEXT)pvThis;

    if (pThis->pBlkGrpDesc)
        RTMemFree(pThis->pBlkGrpDesc);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSOBJOPS::Obj,pfnQueryInfo}
 */
static DECLCALLBACK(int) rtFsExtVol_QueryInfo(void *pvThis, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    RT_NOREF(pvThis, pObjInfo, enmAddAttr);
    return VERR_WRONG_TYPE;
}


static DECLCALLBACK(int) rtFsExtOpenRoot(void *pvThis, PRTVFSDIR phVfsDir)
{
    NOREF(pvThis);
    NOREF(phVfsDir);
    return VERR_NOT_IMPLEMENTED;
}

static DECLCALLBACK(int) rtFsExtIsRangeInUse(void *pvThis, RTFOFF off, size_t cb,
                                             bool *pfUsed)
{
    int rc = VINF_SUCCESS;
    uint64_t offStart = (uint64_t)off;
    PRTFILESYSTEMEXT pThis = (PRTFILESYSTEMEXT)pvThis;

    *pfUsed = false;

    while (cb > 0)
    {
        bool fUsed;
        uint32_t offBlockStart = (uint32_t)(offStart / pThis->cbBlock);
        uint32_t iBlockGroup = (offBlockStart - pThis->iSbBlock) / pThis->cBlocksPerGroup;
        uint32_t offBlockRelStart = offBlockStart - iBlockGroup * pThis->cBlocksPerGroup;
        size_t cbThis = 0;

        if (   offStart < pThis->pBlkGrpDesc->offStart
            || offStart > pThis->pBlkGrpDesc->offLast)
        {
            /* Load new block descriptor. */
            rc = rtFsExtLoadBlkGrpDesc(pThis, iBlockGroup);
            if (RT_FAILURE(rc))
                break;
        }

        cbThis = RT_MIN(cb, pThis->pBlkGrpDesc->offLast - offStart + 1);
        fUsed = rtFsExtIsBlockRangeInUse(pThis->pBlkGrpDesc, offBlockRelStart,
                                         cbThis / pThis->cbBlock +
                                         (cbThis % pThis->cbBlock ? 1 : 0));

        if (fUsed)
        {
            *pfUsed = true;
            break;
        }

        cb       -= cbThis;
        offStart += cbThis;
    }

    return rc;
}


DECL_HIDDEN_CONST(RTFILESYSTEMDESC) const g_rtFsExt =
{
    /* .cbFs = */       sizeof(RTFILESYSTEMEXT),
    /* .VfsOps = */
    {
        /* .Obj = */
        {
            /* .uVersion = */       RTVFSOBJOPS_VERSION,
            /* .enmType = */        RTVFSOBJTYPE_VFS,
            /* .pszName = */        "ExtVol",
            /* .pfnClose = */       rtFsExtVol_Close,
            /* .pfnQueryInfo = */   rtFsExtVol_QueryInfo,
            /* .uEndMarker = */     RTVFSOBJOPS_VERSION
        },
        /* .uVersion = */           RTVFSOPS_VERSION,
        /* .fFeatures = */          0,
        /* .pfnOpenRoot = */        rtFsExtOpenRoot,
        /* .pfnIsRangeInUse = */    rtFsExtIsRangeInUse,
        /* .uEndMarker = */         RTVFSOPS_VERSION
    },
    /* .pfnProbe = */   rtFsExtProbe,
    /* .pfnInit = */    rtFsExtInit
};

