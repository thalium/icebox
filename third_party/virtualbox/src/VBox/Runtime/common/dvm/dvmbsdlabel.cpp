/* $Id: dvmbsdlabel.cpp $ */
/** @file
 * IPRT Disk Volume Management API (DVM) - BSD disklabel format backend.
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

#include <iprt/types.h>
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/dvm.h>
#include <iprt/string.h>
#include <iprt/asm.h>
#include "internal/dvm.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/*
 * Below are the on disk structures of a bsd disklabel as found in
 * /usr/include/sys/disklabel.h from a FreeBSD system.
 *
 * Everything is stored in little endian on the disk.
 */

/** BSD disklabel magic. */
#define RTDVM_BSDLBL_MAGIC          UINT32_C(0x82564557)
/** Maximum number of partitions in the label. */
#define RTDVM_BSDLBL_MAX_PARTITIONS 8

/**
 * A BSD disk label partition.
 */
#pragma pack(1)
typedef struct BsdLabelPartition
{
    /** Number of sectors in the partition. */
    uint32_t             cSectors;
    /** Start sector. */
    uint32_t             offSectorStart;
    /** Filesystem fragment size. */
    uint32_t             cbFsFragment;
    /** Filesystem type. */
    uint8_t              bFsType;
    /** Filesystem fragments per block. */
    uint8_t              cFsFragmentsPerBlock;
    /** Filesystem cylinders per group. */
    uint16_t             cFsCylPerGroup;
} BsdLabelPartition;
#pragma pack()
AssertCompileSize(BsdLabelPartition, 16);
/** Pointer to a BSD disklabel partition structure. */
typedef BsdLabelPartition *PBsdLabelPartition;

/**
 * On disk BSD label structure.
 */
#pragma pack(1)
typedef struct BsdLabel
{
    /** Magic identifying the BSD disk label. */
    uint32_t             u32Magic;
    /** Drive type */
    uint16_t             u16DriveType;
    /** Subtype depending on the drive type above. */
    uint16_t             u16SubType;
    /** Type name. */
    uint8_t              abTypeName[16];
    /** Pack identifier. */
    uint8_t              abPackName[16];
    /** Number of bytes per sector. */
    uint32_t             cbSector;
    /** Number of sectors per track. */
    uint32_t             cSectorsPerTrack;
    /** Number of tracks per cylinder. */
    uint32_t             cTracksPerCylinder;
    /** Number of data cylinders pre unit. */
    uint32_t             cDataCylindersPerUnit;
    /** Number of data sectors per cylinder. */
    uint32_t             cDataSectorsPerCylinder;
    /** Number of data sectors per unit (unit as in disk drive?). */
    uint32_t             cSectorsPerUnit;
    /** Number of spare sectors per track. */
    uint16_t             cSpareSectorsPerTrack;
    /** Number of spare sectors per cylinder. */
    uint16_t             cSpareSectorsPerCylinder;
    /** Number of alternate cylinders per unit. */
    uint32_t             cSpareCylindersPerUnit;
    /** Rotational speed of the disk drive in rotations per minute. */
    uint16_t             cRotationsPerMinute;
    /** Sector interleave. */
    uint16_t             uSectorInterleave;
    /** Sector 0 skew, per track. */
    uint16_t             uSectorSkewPerTrack;
    /** Sector 0 skew, per cylinder. */
    uint16_t             uSectorSkewPerCylinder;
    /** Head switch time in us. */
    uint32_t             usHeadSwitch;
    /** Time of a track-to-track seek in us. */
    uint32_t             usTrackSeek;
    /** Flags. */
    uint32_t             fFlags;
    /** Drive type sepcific information. */
    uint32_t             au32DriveData[5];
    /** Reserved. */
    uint32_t             au32Reserved[5];
    /** The magic number again. */
    uint32_t             u32Magic2;
    /** Checksum (xor of the whole structure). */
    uint16_t             u16ChkSum;
    /** Number of partitions in the array. */
    uint16_t             cPartitions;
    /** Boot area size in bytes. */
    uint32_t             cbBootArea;
    /** Maximum size of the filesystem super block. */
    uint32_t             cbFsSuperBlock;
    /** The partition array. */
    BsdLabelPartition    aPartitions[RTDVM_BSDLBL_MAX_PARTITIONS];
} BsdLabel;
#pragma pack()
AssertCompileSize(BsdLabel, 148 + RTDVM_BSDLBL_MAX_PARTITIONS * 16);
/** Pointer to a BSD disklabel structure. */
typedef BsdLabel *PBsdLabel;

/**
 * BSD disk label volume manager data.
 */
typedef struct RTDVMFMTINTERNAL
{
    /** Pointer to the underlying disk. */
    PCRTDVMDISK     pDisk;
    /** Number of used partitions. */
    uint32_t        cPartitions;
    /** Saved BSD disklabel structure. */
    BsdLabel        DiskLabel;
} RTDVMFMTINTERNAL;
/** Pointer to the MBR volume manager. */
typedef RTDVMFMTINTERNAL *PRTDVMFMTINTERNAL;

/**
 * MBR volume data.
 */
typedef struct RTDVMVOLUMEFMTINTERNAL
{
    /** Pointer to the volume manager. */
    PRTDVMFMTINTERNAL   pVolMgr;
    /** Partition table entry index. */
    uint32_t            idxEntry;
    /** Start offset of the volume. */
    uint64_t            offStart;
    /** Size of the volume. */
    uint64_t            cbVolume;
    /** Pointer to the raw partition table entry. */
    PBsdLabelPartition  pBsdPartitionEntry;
} RTDVMVOLUMEFMTINTERNAL;
/** Pointer to an MBR volume. */
typedef RTDVMVOLUMEFMTINTERNAL *PRTDVMVOLUMEFMTINTERNAL;

/** Converts a LBA number to the byte offset. */
#define RTDVM_BSDLBL_LBA2BYTE(lba, disk) ((lba) * (disk)->cbSector)
/** Converts a Byte offset to the LBA number. */
#define RTDVM_BSDLBL_BYTE2LBA(lba, disk) ((lba) / (disk)->cbSector)

/**
 * Calculates the checksum of the entire bsd disklabel structure.
 *
 * @returns The checksum.
 * @param   pBsdLabel    BSD disklabel to get the checksum for.
 */
static uint16_t rtDvmFmtBsdLblDiskLabelChkSum(PBsdLabel pBsdLabel)
{
    uint16_t uChkSum = 0;
    uint16_t *pCurr = (uint16_t *)pBsdLabel;
    uint16_t *pEnd  = (uint16_t *)&pBsdLabel->aPartitions[pBsdLabel->cPartitions];

    while (pCurr < pEnd)
        uChkSum ^= *pCurr++;

    return uChkSum;
}

/**
 * Converts a partition entry to the host endianness.
 *
 * @returns nothing.
 * @param   pPartition    The partition to decode.
 */
static void rtDvmFmtBsdLblDiskLabelDecodePartition(PBsdLabelPartition pPartition)
{
    pPartition->cSectors       = RT_LE2H_U32(pPartition->cSectors);
    pPartition->offSectorStart = RT_LE2H_U32(pPartition->offSectorStart);
    pPartition->cbFsFragment   = RT_LE2H_U32(pPartition->cbFsFragment);
    pPartition->cFsCylPerGroup = RT_LE2H_U16(pPartition->cFsCylPerGroup);
}

/**
 * Converts the on disk BSD label to the host endianness.
 *
 * @returns Whether the given label structure is a valid BSD disklabel.
 * @param   pBsdLabel    Pointer to the BSD disklabel to decode.
 */
static bool rtDvmFmtBsdLblDiskLabelDecode(PBsdLabel pBsdLabel)
{
    pBsdLabel->u32Magic                 = RT_LE2H_U32(pBsdLabel->u32Magic);
    pBsdLabel->u16DriveType             = RT_LE2H_U16(pBsdLabel->u16DriveType);
    pBsdLabel->u16SubType               = RT_LE2H_U16(pBsdLabel->u16SubType);
    pBsdLabel->cbSector                 = RT_LE2H_U32(pBsdLabel->cbSector);
    pBsdLabel->cSectorsPerTrack         = RT_LE2H_U32(pBsdLabel->cSectorsPerTrack);
    pBsdLabel->cTracksPerCylinder       = RT_LE2H_U32(pBsdLabel->cTracksPerCylinder);
    pBsdLabel->cDataCylindersPerUnit    = RT_LE2H_U32(pBsdLabel->cDataCylindersPerUnit);
    pBsdLabel->cDataSectorsPerCylinder  = RT_LE2H_U32(pBsdLabel->cDataSectorsPerCylinder);
    pBsdLabel->cSectorsPerUnit          = RT_LE2H_U32(pBsdLabel->cSectorsPerUnit);
    pBsdLabel->cSpareSectorsPerTrack    = RT_LE2H_U16(pBsdLabel->cSpareSectorsPerTrack);
    pBsdLabel->cSpareSectorsPerCylinder = RT_LE2H_U16(pBsdLabel->cSpareSectorsPerCylinder);
    pBsdLabel->cSpareCylindersPerUnit   = RT_LE2H_U32(pBsdLabel->cSpareCylindersPerUnit);
    pBsdLabel->cRotationsPerMinute      = RT_LE2H_U16(pBsdLabel->cRotationsPerMinute);
    pBsdLabel->uSectorInterleave        = RT_LE2H_U16(pBsdLabel->uSectorInterleave);
    pBsdLabel->uSectorSkewPerTrack      = RT_LE2H_U16(pBsdLabel->uSectorSkewPerTrack);
    pBsdLabel->uSectorSkewPerCylinder   = RT_LE2H_U16(pBsdLabel->uSectorSkewPerCylinder);
    pBsdLabel->usHeadSwitch             = RT_LE2H_U16(pBsdLabel->usHeadSwitch);
    pBsdLabel->usTrackSeek              = RT_LE2H_U16(pBsdLabel->usTrackSeek);
    pBsdLabel->fFlags                   = RT_LE2H_U32(pBsdLabel->fFlags);

    for (unsigned i = 0; i < RT_ELEMENTS(pBsdLabel->au32DriveData); i++)
        pBsdLabel->au32DriveData[i] = RT_LE2H_U32(pBsdLabel->au32DriveData[i]);
    for (unsigned i = 0; i < RT_ELEMENTS(pBsdLabel->au32Reserved); i++)
        pBsdLabel->au32Reserved[i] = RT_LE2H_U32(pBsdLabel->au32Reserved[i]);

    pBsdLabel->u32Magic2                = RT_LE2H_U32(pBsdLabel->u32Magic2);
    pBsdLabel->u16ChkSum                = RT_LE2H_U16(pBsdLabel->u16ChkSum);
    pBsdLabel->cPartitions              = RT_LE2H_U16(pBsdLabel->cPartitions);
    pBsdLabel->cbBootArea               = RT_LE2H_U32(pBsdLabel->cbBootArea);
    pBsdLabel->cbFsSuperBlock           = RT_LE2H_U32(pBsdLabel->cbFsSuperBlock);

    /* Check the magics now. */
    if (   pBsdLabel->u32Magic    != RTDVM_BSDLBL_MAGIC
        || pBsdLabel->u32Magic2   != RTDVM_BSDLBL_MAGIC
        || pBsdLabel->cPartitions != RTDVM_BSDLBL_MAX_PARTITIONS)
        return false;

    /* Convert the partitions array. */
    for (unsigned i = 0; i < RT_ELEMENTS(pBsdLabel->aPartitions); i++)
        rtDvmFmtBsdLblDiskLabelDecodePartition(&pBsdLabel->aPartitions[i]);

    /* Check the checksum now. */
    uint16_t u16ChkSumSaved = pBsdLabel->u16ChkSum;

    pBsdLabel->u16ChkSum = 0;
    if (u16ChkSumSaved != rtDvmFmtBsdLblDiskLabelChkSum(pBsdLabel))
        return false;

    pBsdLabel->u16ChkSum = u16ChkSumSaved;
    return true;
}

static DECLCALLBACK(int) rtDvmFmtBsdLblProbe(PCRTDVMDISK pDisk, uint32_t *puScore)
{
    BsdLabel DiskLabel;
    int rc = VINF_SUCCESS;

    *puScore = RTDVM_MATCH_SCORE_UNSUPPORTED;

    if (pDisk->cbDisk >= sizeof(BsdLabel))
    {
        /* Read from the disk and check for the disk label structure. */
        rc = rtDvmDiskRead(pDisk, RTDVM_BSDLBL_LBA2BYTE(1, pDisk), &DiskLabel, sizeof(BsdLabel));
        if (   RT_SUCCESS(rc)
            && rtDvmFmtBsdLblDiskLabelDecode(&DiskLabel))
            *puScore = RTDVM_MATCH_SCORE_PERFECT;
    }
    return rc;
}

static DECLCALLBACK(int) rtDvmFmtBsdLblOpen(PCRTDVMDISK pDisk, PRTDVMFMT phVolMgrFmt)
{
    int rc = VINF_SUCCESS;
    PRTDVMFMTINTERNAL pThis = NULL;

    pThis = (PRTDVMFMTINTERNAL)RTMemAllocZ(sizeof(RTDVMFMTINTERNAL));
    if (pThis)
    {
        pThis->pDisk       = pDisk;
        pThis->cPartitions = 0;

        /* Read from the disk and check for the disk label structure. */
        rc = rtDvmDiskRead(pDisk, RTDVM_BSDLBL_LBA2BYTE(1, pDisk), &pThis->DiskLabel, sizeof(BsdLabel));
        if (   RT_SUCCESS(rc)
            && rtDvmFmtBsdLblDiskLabelDecode(&pThis->DiskLabel))
        {
            /* Count number of used entries. */
            for (unsigned i = 0; i < pThis->DiskLabel.cPartitions; i++)
                if (pThis->DiskLabel.aPartitions[i].cSectors)
                    pThis->cPartitions++;

            *phVolMgrFmt = pThis;
        }
        else
        {
            RTMemFree(pThis);
            rc = VERR_INVALID_MAGIC;
        }
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

static DECLCALLBACK(int) rtDvmFmtBsdLblInitialize(PCRTDVMDISK pDisk, PRTDVMFMT phVolMgrFmt)
{
    NOREF(pDisk); NOREF(phVolMgrFmt);
    return VERR_NOT_IMPLEMENTED;
}

static DECLCALLBACK(void) rtDvmFmtBsdLblClose(RTDVMFMT hVolMgrFmt)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;

    pThis->pDisk       = NULL;
    pThis->cPartitions = 0;
    memset(&pThis->DiskLabel, 0, sizeof(BsdLabel));
    RTMemFree(pThis);
}

static DECLCALLBACK(int) rtDvmFmtBsdLblQueryRangeUse(RTDVMFMT hVolMgrFmt,
                                                     uint64_t off, uint64_t cbRange,
                                                     bool *pfUsed)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;

    NOREF(cbRange);

    if (off <= RTDVM_BSDLBL_LBA2BYTE(1, pThis->pDisk))
        *pfUsed = true;
    else
        *pfUsed = false;

    return VINF_SUCCESS;
}

static DECLCALLBACK(uint32_t) rtDvmFmtBsdLblGetValidVolumes(RTDVMFMT hVolMgrFmt)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;
    return pThis->cPartitions;
}

static DECLCALLBACK(uint32_t) rtDvmFmtBsdLblGetMaxVolumes(RTDVMFMT hVolMgrFmt)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;
    return pThis->DiskLabel.cPartitions;
}

/**
 * Creates a new volume.
 *
 * @returns IPRT status code.
 * @param   pThis         The MBR volume manager data.
 * @param   pbBsdLblEntry    The raw MBR entry data.
 * @param   idx           The index in the partition table.
 * @param   phVolFmt      Where to store the volume data on success.
 */
static int rtDvmFmtBsdLblVolumeCreate(PRTDVMFMTINTERNAL pThis, PBsdLabelPartition pBsdPartitionEntry,
                                    uint32_t idx, PRTDVMVOLUMEFMT phVolFmt)
{
    int rc = VINF_SUCCESS;
    PRTDVMVOLUMEFMTINTERNAL pVol = (PRTDVMVOLUMEFMTINTERNAL)RTMemAllocZ(sizeof(RTDVMVOLUMEFMTINTERNAL));

    if (pVol)
    {
        pVol->pVolMgr            = pThis;
        pVol->idxEntry           = idx;
        pVol->pBsdPartitionEntry = pBsdPartitionEntry;
        pVol->offStart           = (uint64_t)pBsdPartitionEntry->offSectorStart * pThis->DiskLabel.cbSector;
        pVol->cbVolume           = (uint64_t)pBsdPartitionEntry->cSectors * pThis->DiskLabel.cbSector;

        *phVolFmt = pVol;
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

static DECLCALLBACK(int) rtDvmFmtBsdLblQueryFirstVolume(RTDVMFMT hVolMgrFmt, PRTDVMVOLUMEFMT phVolFmt)
{
    int rc = VINF_SUCCESS;
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;

    if (pThis->cPartitions != 0)
    {
        /* Search for the first non empty entry. */
        for (unsigned i = 0; i < pThis->DiskLabel.cPartitions; i++)
        {
            if (pThis->DiskLabel.aPartitions[i].cSectors)
            {
                rc = rtDvmFmtBsdLblVolumeCreate(pThis, &pThis->DiskLabel.aPartitions[i], i, phVolFmt);
                break;
            }
        }
    }
    else
        rc = VERR_DVM_MAP_EMPTY;

    return rc;
}

static DECLCALLBACK(int) rtDvmFmtBsdLblQueryNextVolume(RTDVMFMT hVolMgrFmt, RTDVMVOLUMEFMT hVolFmt, PRTDVMVOLUMEFMT phVolFmtNext)
{
    int rc = VERR_DVM_MAP_NO_VOLUME;
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;
    PBsdLabelPartition pBsdPartitionEntry = pVol->pBsdPartitionEntry + 1;

    for (unsigned i = pVol->idxEntry + 1; i < pThis->DiskLabel.cPartitions; i++)
    {
        if (pBsdPartitionEntry->cSectors)
        {
            rc = rtDvmFmtBsdLblVolumeCreate(pThis, pBsdPartitionEntry, i, phVolFmtNext);
            break;
        }
        pBsdPartitionEntry++;
    }

    return rc;
}

static DECLCALLBACK(void) rtDvmFmtBsdLblVolumeClose(RTDVMVOLUMEFMT hVolFmt)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    pVol->pVolMgr            = NULL;
    pVol->offStart           = 0;
    pVol->cbVolume           = 0;
    pVol->pBsdPartitionEntry = NULL;

    RTMemFree(pVol);
}

static DECLCALLBACK(uint64_t) rtDvmFmtBsdLblVolumeGetSize(RTDVMVOLUMEFMT hVolFmt)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    return pVol->cbVolume;
}

static DECLCALLBACK(int) rtDvmFmtBsdLblVolumeQueryName(RTDVMVOLUMEFMT hVolFmt, char **ppszVolName)
{
    NOREF(hVolFmt); NOREF(ppszVolName);
    return VERR_NOT_SUPPORTED;
}

static DECLCALLBACK(RTDVMVOLTYPE) rtDvmFmtBsdLblVolumeGetType(RTDVMVOLUMEFMT hVolFmt)
{
    NOREF(hVolFmt);
    return RTDVMVOLTYPE_UNKNOWN;
}

static DECLCALLBACK(uint64_t) rtDvmFmtBsdLblVolumeGetFlags(RTDVMVOLUMEFMT hVolFmt)
{
    NOREF(hVolFmt);
    return 0;
}

static DECLCALLBACK(bool) rtDvmFmtBsdLblVolumeIsRangeIntersecting(RTDVMVOLUMEFMT hVolFmt,
                                                                  uint64_t offStart, size_t cbRange,
                                                                  uint64_t *poffVol,
                                                                  uint64_t *pcbIntersect)
{
    bool fIntersect = false;
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    if (RTDVM_RANGE_IS_INTERSECTING(pVol->offStart, pVol->cbVolume, offStart))
    {
        fIntersect    = true;
        *poffVol      = offStart - pVol->offStart;
        *pcbIntersect = RT_MIN(cbRange, pVol->offStart + pVol->cbVolume - offStart);
    }

    return fIntersect;
}

static DECLCALLBACK(int) rtDvmFmtBsdLblVolumeRead(RTDVMVOLUMEFMT hVolFmt, uint64_t off, void *pvBuf, size_t cbRead)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;
    AssertReturn(off + cbRead <= pVol->cbVolume, VERR_INVALID_PARAMETER);

    return rtDvmDiskRead(pVol->pVolMgr->pDisk, pVol->offStart + off, pvBuf, cbRead);
}

static DECLCALLBACK(int) rtDvmFmtBsdLblVolumeWrite(RTDVMVOLUMEFMT hVolFmt, uint64_t off, const void *pvBuf, size_t cbWrite)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;
    AssertReturn(off + cbWrite <= pVol->cbVolume, VERR_INVALID_PARAMETER);

    return rtDvmDiskWrite(pVol->pVolMgr->pDisk, pVol->offStart + off, pvBuf, cbWrite);
}

DECLHIDDEN(RTDVMFMTOPS) g_rtDvmFmtBsdLbl =
{
    /* pcszFmt */
    "BsdLabel",
    /* enmFormat, */
    RTDVMFORMATTYPE_BSD_LABLE,
    /* pfnProbe */
    rtDvmFmtBsdLblProbe,
    /* pfnOpen */
    rtDvmFmtBsdLblOpen,
    /* pfnInitialize */
    rtDvmFmtBsdLblInitialize,
    /* pfnClose */
    rtDvmFmtBsdLblClose,
    /* pfnQueryRangeUse */
    rtDvmFmtBsdLblQueryRangeUse,
    /* pfnGetValidVolumes */
    rtDvmFmtBsdLblGetValidVolumes,
    /* pfnGetMaxVolumes */
    rtDvmFmtBsdLblGetMaxVolumes,
    /* pfnQueryFirstVolume */
    rtDvmFmtBsdLblQueryFirstVolume,
    /* pfnQueryNextVolume */
    rtDvmFmtBsdLblQueryNextVolume,
    /* pfnVolumeClose */
    rtDvmFmtBsdLblVolumeClose,
    /* pfnVolumeGetSize */
    rtDvmFmtBsdLblVolumeGetSize,
    /* pfnVolumeQueryName */
    rtDvmFmtBsdLblVolumeQueryName,
    /* pfnVolumeGetType */
    rtDvmFmtBsdLblVolumeGetType,
    /* pfnVolumeGetFlags */
    rtDvmFmtBsdLblVolumeGetFlags,
    /* pfnVolumeIsRangeIntersecting */
    rtDvmFmtBsdLblVolumeIsRangeIntersecting,
    /* pfnVolumeRead */
    rtDvmFmtBsdLblVolumeRead,
    /* pfnVolumeWrite */
    rtDvmFmtBsdLblVolumeWrite
};

