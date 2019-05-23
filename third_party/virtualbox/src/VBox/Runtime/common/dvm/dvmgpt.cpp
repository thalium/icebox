/* $Id: dvmgpt.cpp $ */
/** @file
 * IPRT Disk Volume Management API (DVM) - GPT format backend.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/types.h>
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/dvm.h>
#include <iprt/string.h>
#include <iprt/uuid.h>
#include <iprt/asm.h>
#include "internal/dvm.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** The GPT signature. */
#define RTDVM_GPT_SIGNATURE "EFI PART"

/**
 * GPT on disk header.
 */
typedef struct GPTHDR
{
    /** 0x00: Signature ("EFI PART"). */
    char     abSignature[8];
    /** 0x08: Revision. */
    uint32_t u32Revision;
    /** 0x0c: Header size. */
    uint32_t cbHeader;
    /** 0x10: CRC of header. */
    uint32_t u32Crc;
} GPTHDR;
/** Pointer to a GPT header. */
typedef struct GPTHDR *PGPTHDR;
AssertCompileSize(GPTHDR, 20);

/**
 * Complete GPT table header for revision 1.0.
 */
#pragma pack(1)
typedef struct GPTHDRREV1
{
    /** 0x00: Header. */
    GPTHDR   Hdr;
    /** 0x14: Reserved. */
    uint32_t u32Reserved;
    /** 0x18: Current LBA. */
    uint64_t u64LbaCurrent;
    /** 0x20: Backup LBA. */
    uint64_t u64LbaBackup;
    /** 0x28:First usable LBA for partitions. */
    uint64_t u64LbaFirstPartition;
    /** 0x30: Last usable LBA for partitions. */
    uint64_t u64LbaLastPartition;
    /** 0x38: Disk UUID. */
    RTUUID   DiskUuid;
    /** 0x48: LBA of first partition entry. */
    uint64_t u64LbaPartitionEntries;
    /** 0x50: Number of partition entries. */
    uint32_t cPartitionEntries;
    /** 0x54: Partition entry size. */
    uint32_t cbPartitionEntry;
    /** 0x58: CRC of partition entries. */
    uint32_t u32CrcPartitionEntries;
} GPTHDRREV1;
/** Pointer to a revision 1.0 GPT header. */
typedef GPTHDRREV1 *PGPTHDRREV1;
#pragma pack()
AssertCompileSize(GPTHDRREV1, 92);

/**
 * GPT partition table entry.
 */
typedef struct GPTENTRY
{
    /** 0x00: Partition type UUID. */
    RTUUID   UuidType;
    /** 0x10: Partition UUID. */
    RTUUID   UuidPartition;
    /** 0x20: First LBA. */
    uint64_t u64LbaFirst;
    /** 0x28: Last LBA. */
    uint64_t u64LbaLast;
    /** 0x30: Attribute flags. */
    uint64_t u64Flags;
    /** 0x38: Partition name (UTF-16LE code units). */
    RTUTF16  aPartitionName[36];
} GPTENTRY;
/** Pointer to a GPT entry. */
typedef struct GPTENTRY *PGPTENTRY;
AssertCompileSize(GPTENTRY, 128);

/** Partition flags - System partition. */
#define RTDVM_GPT_ENTRY_SYSTEM          RT_BIT_64(0)
/** Partition flags - Partition is readonly. */
#define RTDVM_GPT_ENTRY_READONLY        RT_BIT_64(60)
/** Partition flags - Partition is hidden. */
#define RTDVM_GPT_ENTRY_HIDDEN          RT_BIT_64(62)
/** Partition flags - Don't automount this partition. */
#define RTDVM_GPT_ENTRY_NO_AUTOMOUNT    RT_BIT_64(63)

/**
 * GPT volume manager data.
 */
typedef struct RTDVMFMTINTERNAL
{
    /** Pointer to the underlying disk. */
    PCRTDVMDISK     pDisk;
    /** GPT header. */
    GPTHDRREV1      HdrRev1;
    /** GPT array. */
    PGPTENTRY       paGptEntries;
    /** Number of occupied partition entries. */
    uint32_t        cPartitions;
} RTDVMFMTINTERNAL;
/** Pointer to the MBR volume manager. */
typedef RTDVMFMTINTERNAL *PRTDVMFMTINTERNAL;

/**
 * GPT volume data.
 */
typedef struct RTDVMVOLUMEFMTINTERNAL
{
    /** Pointer to the volume manager. */
    PRTDVMFMTINTERNAL pVolMgr;
    /** Partition table entry index. */
    uint32_t          idxEntry;
    /** Start offset of the volume. */
    uint64_t          offStart;
    /** Size of the volume. */
    uint64_t          cbVolume;
    /** Pointer to the GPT entry in the array. */
    PGPTENTRY         pGptEntry;
} RTDVMVOLUMEFMTINTERNAL;
/** Pointer to an MBR volume. */
typedef RTDVMVOLUMEFMTINTERNAL *PRTDVMVOLUMEFMTINTERNAL;

/**
 * GPT partition type to DVM volume type mapping entry.
 */

typedef struct RTDVMGPTPARTTYPE2VOLTYPE
{
    /** Type UUID. */
    const char       *pcszUuid;
    /** DVM volume type. */
    RTDVMVOLTYPE      enmVolType;
} RTDVMGPTPARTTYPE2VOLTYPE;
/** Pointer to a MBR FS Type to volume type mapping entry. */
typedef RTDVMGPTPARTTYPE2VOLTYPE *PRTDVMGPTPARTTYPE2VOLTYPE;

/** Converts a LBA number to the byte offset. */
#define RTDVM_GPT_LBA2BYTE(lba, disk) ((lba) * (disk)->cbSector)
/** Converts a Byte offset to the LBA number. */
#define RTDVM_GPT_BYTE2LBA(lba, disk) ((lba) / (disk)->cbSector)


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Mapping of partition types to DVM volume types.
 *
 * From http://en.wikipedia.org/wiki/GUID_Partition_Table
 */
static const RTDVMGPTPARTTYPE2VOLTYPE g_aPartType2DvmVolTypes[] =
{
    {"0657FD6D-A4AB-43C4-84E5-0933C84B4F4F", RTDVMVOLTYPE_LINUX_SWAP},
    {"EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", RTDVMVOLTYPE_LINUX_NATIVE},
    {"E6D6D379-F507-44C2-A23C-238F2A3DF928", RTDVMVOLTYPE_LINUX_LVM},
    {"A19D880F-05FC-4D3B-A006-743F0F84911E", RTDVMVOLTYPE_LINUX_SOFTRAID},

    {"83BD6B9D-7F41-11DC-BE0B-001560B84F0F", RTDVMVOLTYPE_FREEBSD}, /* Boot */
    {"516E7CB4-6ECF-11D6-8FF8-00022D09712B", RTDVMVOLTYPE_FREEBSD}, /* Data */
    {"516E7CB5-6ECF-11D6-8FF8-00022D09712B", RTDVMVOLTYPE_FREEBSD}, /* Swap */
    {"516E7CB6-6ECF-11D6-8FF8-00022D09712B", RTDVMVOLTYPE_FREEBSD}, /* UFS */
    {"516E7CB8-6ECF-11D6-8FF8-00022D09712B", RTDVMVOLTYPE_FREEBSD}, /* Vinum */
    {"516E7CBA-6ECF-11D6-8FF8-00022D09712B", RTDVMVOLTYPE_FREEBSD}, /* ZFS */

    {"49F48D32-B10E-11DC-B99B-0019D1879648", RTDVMVOLTYPE_NETBSD}, /* Swap */
    {"49F48D5A-B10E-11DC-B99B-0019D1879648", RTDVMVOLTYPE_NETBSD}, /* FFS */
    {"49F48D82-B10E-11DC-B99B-0019D1879648", RTDVMVOLTYPE_NETBSD}, /* LFS */
    {"49F48DAA-B10E-11DC-B99B-0019D1879648", RTDVMVOLTYPE_NETBSD}, /* Raid */
    {"2DB519C4-B10F-11DC-B99B-0019D1879648", RTDVMVOLTYPE_NETBSD}, /* Concatenated */
    {"2DB519EC-B10F-11DC-B99B-0019D1879648", RTDVMVOLTYPE_NETBSD}, /* Encrypted */

    {"48465300-0000-11AA-AA11-00306543ECAC", RTDVMVOLTYPE_MAC_OSX_HFS},

    {"6A82CB45-1DD2-11B2-99A6-080020736631", RTDVMVOLTYPE_SOLARIS}, /* Boot */
    {"6A85CF4D-1DD2-11B2-99A6-080020736631", RTDVMVOLTYPE_SOLARIS}, /* Root */
    {"6A87C46F-1DD2-11B2-99A6-080020736631", RTDVMVOLTYPE_SOLARIS}, /* Swap */
    {"6A8B642B-1DD2-11B2-99A6-080020736631", RTDVMVOLTYPE_SOLARIS}, /* Backup */
    {"6A898CC3-1DD2-11B2-99A6-080020736631", RTDVMVOLTYPE_SOLARIS}, /* /usr */
    {"6A8EF2E9-1DD2-11B2-99A6-080020736631", RTDVMVOLTYPE_SOLARIS}, /* /var */
    {"6A90BA39-1DD2-11B2-99A6-080020736631", RTDVMVOLTYPE_SOLARIS}, /* /home */
    {"6A9283A5-1DD2-11B2-99A6-080020736631", RTDVMVOLTYPE_SOLARIS}, /* Alternate sector */
};

static DECLCALLBACK(int) rtDvmFmtGptProbe(PCRTDVMDISK pDisk, uint32_t *puScore)
{
    int rc = VINF_SUCCESS;
    GPTHDR Hdr;

    *puScore = RTDVM_MATCH_SCORE_UNSUPPORTED;

    if (rtDvmDiskGetSectors(pDisk) >= 2)
    {
        /* Read from the disk and check for the signature. */
        rc = rtDvmDiskRead(pDisk, RTDVM_GPT_LBA2BYTE(1, pDisk), &Hdr, sizeof(GPTHDR));
        if (   RT_SUCCESS(rc)
            && !strncmp(&Hdr.abSignature[0], RTDVM_GPT_SIGNATURE, RT_ELEMENTS(Hdr.abSignature))
            && RT_LE2H_U32(Hdr.u32Revision) == 0x00010000
            && RT_LE2H_U32(Hdr.cbHeader)    == sizeof(GPTHDRREV1))
            *puScore = RTDVM_MATCH_SCORE_PERFECT;
    }

    return rc;
}

static DECLCALLBACK(int) rtDvmFmtGptOpen(PCRTDVMDISK pDisk, PRTDVMFMT phVolMgrFmt)
{
    int rc = VINF_SUCCESS;
    PRTDVMFMTINTERNAL pThis = NULL;

    pThis = (PRTDVMFMTINTERNAL)RTMemAllocZ(sizeof(RTDVMFMTINTERNAL));
    if (pThis)
    {
        pThis->pDisk       = pDisk;
        pThis->cPartitions = 0;

        /* Read the complete GPT header and convert to host endianess. */
        rc = rtDvmDiskRead(pDisk, RTDVM_GPT_LBA2BYTE(1, pDisk), &pThis->HdrRev1, sizeof(pThis->HdrRev1));
        if (RT_SUCCESS(rc))
        {
            pThis->HdrRev1.Hdr.u32Revision        = RT_LE2H_U32(pThis->HdrRev1.Hdr.u32Revision);
            pThis->HdrRev1.Hdr.cbHeader           = RT_LE2H_U32(pThis->HdrRev1.Hdr.cbHeader);
            pThis->HdrRev1.Hdr.u32Crc             = RT_LE2H_U32(pThis->HdrRev1.Hdr.u32Crc);
            pThis->HdrRev1.u64LbaCurrent          = RT_LE2H_U64(pThis->HdrRev1.u64LbaCurrent);
            pThis->HdrRev1.u64LbaBackup           = RT_LE2H_U64(pThis->HdrRev1.u64LbaBackup);
            pThis->HdrRev1.u64LbaFirstPartition   = RT_LE2H_U64(pThis->HdrRev1.u64LbaFirstPartition);
            pThis->HdrRev1.u64LbaLastPartition    = RT_LE2H_U64(pThis->HdrRev1.u64LbaLastPartition);
            /** @todo Disk UUID */
            pThis->HdrRev1.u64LbaPartitionEntries = RT_LE2H_U64(pThis->HdrRev1.u64LbaPartitionEntries);
            pThis->HdrRev1.cPartitionEntries      = RT_LE2H_U32(pThis->HdrRev1.cPartitionEntries);
            pThis->HdrRev1.cbPartitionEntry       = RT_LE2H_U32(pThis->HdrRev1.cbPartitionEntry);
            pThis->HdrRev1.u32CrcPartitionEntries = RT_LE2H_U32(pThis->HdrRev1.u32CrcPartitionEntries);

            if (pThis->HdrRev1.cbPartitionEntry == sizeof(GPTENTRY))
            {
                pThis->paGptEntries = (PGPTENTRY)RTMemAllocZ(pThis->HdrRev1.cPartitionEntries * pThis->HdrRev1.cbPartitionEntry);
                if (pThis->paGptEntries)
                {
                    rc = rtDvmDiskRead(pDisk, RTDVM_GPT_LBA2BYTE(pThis->HdrRev1.u64LbaPartitionEntries, pDisk),
                                     pThis->paGptEntries, pThis->HdrRev1.cPartitionEntries * pThis->HdrRev1.cbPartitionEntry);
                    if (RT_SUCCESS(rc))
                    {
                        /* Count the occupied entries. */
                        for (unsigned i = 0; i < pThis->HdrRev1.cPartitionEntries; i++)
                            if (!RTUuidIsNull(&pThis->paGptEntries[i].UuidType))
                            {
                                /* Convert to host endianess. */
                                /** @todo Uuids */
                                pThis->paGptEntries[i].u64LbaFirst = RT_LE2H_U64(pThis->paGptEntries[i].u64LbaFirst);
                                pThis->paGptEntries[i].u64LbaLast  = RT_LE2H_U64(pThis->paGptEntries[i].u64LbaLast);
                                pThis->paGptEntries[i].u64Flags    = RT_LE2H_U64(pThis->paGptEntries[i].u64Flags);
                                for (unsigned cwc = 0; cwc < RT_ELEMENTS(pThis->paGptEntries[i].aPartitionName); cwc++)
                                    pThis->paGptEntries[i].aPartitionName[cwc] = RT_LE2H_U16(pThis->paGptEntries[i].aPartitionName[cwc]);

                                pThis->cPartitions++;
                            }
                    }

                    if (RT_FAILURE(rc))
                        RTMemFree(pThis->paGptEntries);
                }
                else
                    rc = VERR_NO_MEMORY;
            }
            else
                rc = VERR_NOT_SUPPORTED;

            if (RT_SUCCESS(rc))
                *phVolMgrFmt = pThis;
            else
                RTMemFree(pThis);
        }
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

static DECLCALLBACK(int) rtDvmFmtGptInitialize(PCRTDVMDISK pDisk, PRTDVMFMT phVolMgrFmt)
{
    NOREF(pDisk); NOREF(phVolMgrFmt);
    return VERR_NOT_IMPLEMENTED;
}

static DECLCALLBACK(void) rtDvmFmtGptClose(RTDVMFMT hVolMgrFmt)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;

    pThis->pDisk = NULL;
    memset(&pThis->HdrRev1, 0, sizeof(pThis->HdrRev1));
    RTMemFree(pThis->paGptEntries);

    pThis->paGptEntries = NULL;
    RTMemFree(pThis);
}

static DECLCALLBACK(int) rtDvmFmtGptQueryRangeUse(RTDVMFMT hVolMgrFmt,
                                                  uint64_t off, uint64_t cbRange,
                                                  bool *pfUsed)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;

    NOREF(cbRange);

    if (off < 33*pThis->pDisk->cbSector)
        *pfUsed = true;
    else
        *pfUsed = false;

    return VINF_SUCCESS;
}

static DECLCALLBACK(uint32_t) rtDvmFmtGptGetValidVolumes(RTDVMFMT hVolMgrFmt)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;

    return pThis->cPartitions;
}

static DECLCALLBACK(uint32_t) rtDvmFmtGptGetMaxVolumes(RTDVMFMT hVolMgrFmt)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;

    return pThis->HdrRev1.cPartitionEntries;
}

/**
 * Creates a new volume.
 *
 * @returns IPRT status code.
 * @param   pThis         The MBR volume manager data.
 * @param   pGptEntry     The GPT entry.
 * @param   idx           The index in the partition array.
 * @param   phVolFmt      Where to store the volume data on success.
 */
static int rtDvmFmtMbrVolumeCreate(PRTDVMFMTINTERNAL pThis, PGPTENTRY pGptEntry,
                                 uint32_t idx, PRTDVMVOLUMEFMT phVolFmt)
{
    int rc = VINF_SUCCESS;
    PRTDVMVOLUMEFMTINTERNAL pVol = (PRTDVMVOLUMEFMTINTERNAL)RTMemAllocZ(sizeof(RTDVMVOLUMEFMTINTERNAL));

    if (pVol)
    {
        pVol->pVolMgr    = pThis;
        pVol->idxEntry   = idx;
        pVol->pGptEntry  = pGptEntry;
        pVol->offStart   = RTDVM_GPT_LBA2BYTE(pGptEntry->u64LbaFirst, pThis->pDisk);
        pVol->cbVolume   = RTDVM_GPT_LBA2BYTE(pGptEntry->u64LbaLast - pGptEntry->u64LbaFirst + 1, pThis->pDisk);

        *phVolFmt = pVol;
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

static DECLCALLBACK(int) rtDvmFmtGptQueryFirstVolume(RTDVMFMT hVolMgrFmt, PRTDVMVOLUMEFMT phVolFmt)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;

    if (pThis->cPartitions != 0)
    {
        PGPTENTRY pGptEntry = &pThis->paGptEntries[0];

        /* Search for the first non empty entry. */
        for (unsigned i = 0; i < pThis->HdrRev1.cPartitionEntries; i++)
        {
            if (!RTUuidIsNull(&pGptEntry->UuidType))
                return rtDvmFmtMbrVolumeCreate(pThis, pGptEntry, i, phVolFmt);
            pGptEntry++;
        }
        AssertFailed();
    }
    return VERR_DVM_MAP_EMPTY;
}

static DECLCALLBACK(int) rtDvmFmtGptQueryNextVolume(RTDVMFMT hVolMgrFmt, RTDVMVOLUMEFMT hVolFmt, PRTDVMVOLUMEFMT phVolFmtNext)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;
    PGPTENTRY pGptEntry = pVol->pGptEntry + 1;

    for (unsigned i = pVol->idxEntry + 1; i < pThis->HdrRev1.cPartitionEntries; i++)
    {
        if (!RTUuidIsNull(&pGptEntry->UuidType))
            return rtDvmFmtMbrVolumeCreate(pThis, pGptEntry, i, phVolFmtNext);
        pGptEntry++;
    }

    return VERR_DVM_MAP_NO_VOLUME;
}

static DECLCALLBACK(void) rtDvmFmtGptVolumeClose(RTDVMVOLUMEFMT hVolFmt)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    pVol->pVolMgr    = NULL;
    pVol->offStart   = 0;
    pVol->cbVolume   = 0;
    pVol->pGptEntry  = NULL;

    RTMemFree(pVol);
}

static DECLCALLBACK(uint64_t) rtDvmFmtGptVolumeGetSize(RTDVMVOLUMEFMT hVolFmt)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    return pVol->cbVolume;
}

static DECLCALLBACK(int) rtDvmFmtGptVolumeQueryName(RTDVMVOLUMEFMT hVolFmt, char **ppszVolName)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    *ppszVolName = NULL;
    return RTUtf16ToUtf8Ex(&pVol->pGptEntry->aPartitionName[0], RT_ELEMENTS(pVol->pGptEntry->aPartitionName),
                           ppszVolName, 0, NULL);
}

static DECLCALLBACK(RTDVMVOLTYPE) rtDvmFmtGptVolumeGetType(RTDVMVOLUMEFMT hVolFmt)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    for (unsigned i = 0; i < RT_ELEMENTS(g_aPartType2DvmVolTypes); i++)
        if (!RTUuidCompareStr(&pVol->pGptEntry->UuidType, g_aPartType2DvmVolTypes[i].pcszUuid))
            return g_aPartType2DvmVolTypes[i].enmVolType;

    return RTDVMVOLTYPE_UNKNOWN;
}

static DECLCALLBACK(uint64_t) rtDvmFmtGptVolumeGetFlags(RTDVMVOLUMEFMT hVolFmt)
{
    NOREF(hVolFmt); /* No supported flags for now. */
    return 0;
}

static DECLCALLBACK(bool) rtDvmFmtGptVolumeIsRangeIntersecting(RTDVMVOLUMEFMT hVolFmt,
                                                               uint64_t offStart, size_t cbRange,
                                                               uint64_t *poffVol,
                                                               uint64_t *pcbIntersect)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    if (RTDVM_RANGE_IS_INTERSECTING(pVol->offStart, pVol->cbVolume, offStart))
    {
        *poffVol      = offStart - pVol->offStart;
        *pcbIntersect = RT_MIN(cbRange, pVol->offStart + pVol->cbVolume - offStart);
        return true;
    }
    return false;
}

static DECLCALLBACK(int) rtDvmFmtGptVolumeRead(RTDVMVOLUMEFMT hVolFmt, uint64_t off, void *pvBuf, size_t cbRead)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;
    AssertReturn(off + cbRead <= pVol->cbVolume, VERR_INVALID_PARAMETER);

    return rtDvmDiskRead(pVol->pVolMgr->pDisk, pVol->offStart + off, pvBuf, cbRead);
}

static DECLCALLBACK(int) rtDvmFmtGptVolumeWrite(RTDVMVOLUMEFMT hVolFmt, uint64_t off, const void *pvBuf, size_t cbWrite)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;
    AssertReturn(off + cbWrite <= pVol->cbVolume, VERR_INVALID_PARAMETER);

    return rtDvmDiskWrite(pVol->pVolMgr->pDisk, pVol->offStart + off, pvBuf, cbWrite);
}

RTDVMFMTOPS g_rtDvmFmtGpt =
{
    /* pszFmt */
    "GPT",
    /* enmFormat, */
    RTDVMFORMATTYPE_GPT,
    /* pfnProbe */
    rtDvmFmtGptProbe,
    /* pfnOpen */
    rtDvmFmtGptOpen,
    /* pfnInitialize */
    rtDvmFmtGptInitialize,
    /* pfnClose */
    rtDvmFmtGptClose,
    /* pfnQueryRangeUse */
    rtDvmFmtGptQueryRangeUse,
    /* pfnGetValidVolumes */
    rtDvmFmtGptGetValidVolumes,
    /* pfnGetMaxVolumes */
    rtDvmFmtGptGetMaxVolumes,
    /* pfnQueryFirstVolume */
    rtDvmFmtGptQueryFirstVolume,
    /* pfnQueryNextVolume */
    rtDvmFmtGptQueryNextVolume,
    /* pfnVolumeClose */
    rtDvmFmtGptVolumeClose,
    /* pfnVolumeGetSize */
    rtDvmFmtGptVolumeGetSize,
    /* pfnVolumeQueryName */
    rtDvmFmtGptVolumeQueryName,
    /* pfnVolumeGetType */
    rtDvmFmtGptVolumeGetType,
    /* pfnVolumeGetFlags */
    rtDvmFmtGptVolumeGetFlags,
    /* pfnVolumeIsRangeIntersecting */
    rtDvmFmtGptVolumeIsRangeIntersecting,
    /* pfnVolumeRead */
    rtDvmFmtGptVolumeRead,
    /* pfnVolumeWrite */
    rtDvmFmtGptVolumeWrite
};

