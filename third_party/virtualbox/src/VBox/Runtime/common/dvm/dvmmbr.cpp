/* $Id: dvmmbr.cpp $ */
/** @file
 * IPRT Disk Volume Management API (DVM) - MBR format backend.
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
#include "internal/dvm.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * MBR volume manager data.
 */
typedef struct RTDVMFMTINTERNAL
{
    /** Pointer to the underlying disk. */
    PCRTDVMDISK     pDisk;
    /** Number of initialized partitions. */
    uint32_t        cPartitions;
    /** The raw MBR data. */
    uint8_t         abMbr[512];
} RTDVMFMTINTERNAL;
/** Pointer to the MBR volume manager. */
typedef RTDVMFMTINTERNAL *PRTDVMFMTINTERNAL;

/**
 * MBR volume data.
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
    /** Pointer to the raw partition table entry. */
    uint8_t          *pbMbrEntry;
} RTDVMVOLUMEFMTINTERNAL;
/** Pointer to an MBR volume. */
typedef RTDVMVOLUMEFMTINTERNAL *PRTDVMVOLUMEFMTINTERNAL;

/**
 * MBR FS type to DVM volume type mapping entry.
 */

typedef struct RTDVMMBRFS2VOLTYPE
{
    /** MBR FS Id. */
    uint8_t           bFsId;
    /** DVM volume type. */
    RTDVMVOLTYPE      enmVolType;
} RTDVMMBRFS2VOLTYPE;
/** Pointer to a MBR FS Type to volume type mapping entry. */
typedef RTDVMMBRFS2VOLTYPE *PRTDVMMBRFS2VOLTYPE;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Mapping of FS types to DVM volume types.
 *
 * From http://www.win.tue.nl/~aeb/partitions/partition_types-1.html
 */
static const RTDVMMBRFS2VOLTYPE g_aFs2DvmVolTypes[] =
{
    { 0x06, RTDVMVOLTYPE_FAT16 },
    { 0x07, RTDVMVOLTYPE_NTFS }, /* Simplification: Used for HPFS, exFAT, ++, too but NTFS is the more common one. */
    { 0x0b, RTDVMVOLTYPE_FAT32 },
    { 0x0c, RTDVMVOLTYPE_FAT32 },
    { 0x82, RTDVMVOLTYPE_LINUX_SWAP },
    { 0x83, RTDVMVOLTYPE_LINUX_NATIVE },
    { 0x8e, RTDVMVOLTYPE_LINUX_LVM },
    { 0xa5, RTDVMVOLTYPE_FREEBSD },
    { 0xa9, RTDVMVOLTYPE_NETBSD },
    { 0xa6, RTDVMVOLTYPE_OPENBSD },
    { 0xaf, RTDVMVOLTYPE_MAC_OSX_HFS },
    { 0xbf, RTDVMVOLTYPE_SOLARIS },
    { 0xfd, RTDVMVOLTYPE_LINUX_SOFTRAID }
};

static DECLCALLBACK(int) rtDvmFmtMbrProbe(PCRTDVMDISK pDisk, uint32_t *puScore)
{
    int rc = VINF_SUCCESS;
    uint8_t abMbr[512];

    *puScore = RTDVM_MATCH_SCORE_UNSUPPORTED;

    if (pDisk->cbDisk >= 512)
    {
        /* Read from the disk and check for the 0x55aa signature at the end. */
        rc = rtDvmDiskRead(pDisk, 0, &abMbr[0], sizeof(abMbr));
        if (   RT_SUCCESS(rc)
            && abMbr[510] == 0x55
            && abMbr[511] == 0xaa)
            *puScore = RTDVM_MATCH_SCORE_SUPPORTED; /* Not perfect because GPTs have a protective MBR. */
    }

    return rc;
}

static DECLCALLBACK(int) rtDvmFmtMbrOpen(PCRTDVMDISK pDisk, PRTDVMFMT phVolMgrFmt)
{
    int rc = VINF_SUCCESS;
    PRTDVMFMTINTERNAL pThis = NULL;

    pThis = (PRTDVMFMTINTERNAL)RTMemAllocZ(sizeof(RTDVMFMTINTERNAL));
    if (pThis)
    {
        pThis->pDisk       = pDisk;
        pThis->cPartitions = 0;

        /* Read the MBR and count the valid partition entries. */
        rc = rtDvmDiskRead(pDisk, 0, &pThis->abMbr[0], sizeof(pThis->abMbr));
        if (RT_SUCCESS(rc))
        {
            uint8_t *pbMbrEntry = &pThis->abMbr[446];

            Assert(pThis->abMbr[510] == 0x55 && pThis->abMbr[511] == 0xaa);

            for (unsigned i = 0; i < 4; i++)
            {
                /* The entry is unused if the type contains 0x00. */
                if (pbMbrEntry[4] != 0x00)
                    pThis->cPartitions++;

                pbMbrEntry += 16;
            }

            *phVolMgrFmt = pThis;
        }
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

static DECLCALLBACK(int) rtDvmFmtMbrInitialize(PCRTDVMDISK pDisk, PRTDVMFMT phVolMgrFmt)
{
    int rc = VINF_SUCCESS;
    PRTDVMFMTINTERNAL pThis = NULL;

    pThis = (PRTDVMFMTINTERNAL)RTMemAllocZ(sizeof(RTDVMFMTINTERNAL));
    if (pThis)
    {
        /* Setup a new MBR and write it to the disk. */
        memset(&pThis->abMbr[0], 0, sizeof(pThis->abMbr));
        pThis->abMbr[510] = 0x55;
        pThis->abMbr[511] = 0xaa;

        rc = rtDvmDiskWrite(pDisk, 0, &pThis->abMbr[0], sizeof(pThis->abMbr));

        if (RT_SUCCESS(rc))
        {
            pThis->pDisk       = pDisk;
            pThis->cPartitions = 0;
            *phVolMgrFmt = pThis;
        }
        else
            RTMemFree(pThis);
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

static DECLCALLBACK(void) rtDvmFmtMbrClose(RTDVMFMT hVolMgrFmt)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;

    pThis->pDisk       = NULL;
    pThis->cPartitions = 0;
    memset(&pThis->abMbr[0], 0, sizeof(pThis->abMbr));
    RTMemFree(pThis);
}

static DECLCALLBACK(int) rtDvmFmtMbrQueryRangeUse(RTDVMFMT hVolMgrFmt,
                                                  uint64_t off, uint64_t cbRange,
                                                  bool *pfUsed)
{
    NOREF(hVolMgrFmt);
    NOREF(cbRange);

    /* MBR uses the first sector only. */
    *pfUsed = off < 512;
    return VINF_SUCCESS;
}

static DECLCALLBACK(uint32_t) rtDvmFmtMbrGetValidVolumes(RTDVMFMT hVolMgrFmt)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;

    return pThis->cPartitions;
}

static DECLCALLBACK(uint32_t) rtDvmFmtMbrGetMaxVolumes(RTDVMFMT hVolMgrFmt)
{
    NOREF(hVolMgrFmt);
    return 4; /** @todo Add support for EBR? */
}

/**
 * Creates a new volume.
 *
 * @returns IPRT status code.
 * @param   pThis         The MBR volume manager data.
 * @param   pbMbrEntry    The raw MBR entry data.
 * @param   idx           The index in the partition table.
 * @param   phVolFmt      Where to store the volume data on success.
 */
static int rtDvmFmtMbrVolumeCreate(PRTDVMFMTINTERNAL pThis, uint8_t *pbMbrEntry,
                                 uint32_t idx, PRTDVMVOLUMEFMT phVolFmt)
{
    int rc = VINF_SUCCESS;
    PRTDVMVOLUMEFMTINTERNAL pVol = (PRTDVMVOLUMEFMTINTERNAL)RTMemAllocZ(sizeof(RTDVMVOLUMEFMTINTERNAL));

    if (pVol)
    {
        pVol->pVolMgr    = pThis;
        pVol->idxEntry   = idx;
        pVol->pbMbrEntry = pbMbrEntry;
        pVol->offStart   = *(uint32_t *)&pbMbrEntry[0x08] * pThis->pDisk->cbSector;
        pVol->cbVolume   = *(uint32_t *)&pbMbrEntry[0x0c] * pThis->pDisk->cbSector;

        *phVolFmt = pVol;
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

static DECLCALLBACK(int) rtDvmFmtMbrQueryFirstVolume(RTDVMFMT hVolMgrFmt, PRTDVMVOLUMEFMT phVolFmt)
{
    int rc = VINF_SUCCESS;
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;

    if (pThis->cPartitions != 0)
    {
        uint8_t *pbMbrEntry = &pThis->abMbr[446];

        /* Search for the first non empty entry. */
        for (unsigned i = 0; i < 4; i++)
        {
            if (pbMbrEntry[0x04] != 0x00)
            {
                rc = rtDvmFmtMbrVolumeCreate(pThis, pbMbrEntry, i, phVolFmt);
                break;
            }
            pbMbrEntry += 16;
        }
    }
    else
        rc = VERR_DVM_MAP_EMPTY;

    return rc;
}

static DECLCALLBACK(int) rtDvmFmtMbrQueryNextVolume(RTDVMFMT hVolMgrFmt, RTDVMVOLUMEFMT hVolFmt, PRTDVMVOLUMEFMT phVolFmtNext)
{
    int rc = VERR_DVM_MAP_NO_VOLUME;
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;
    uint8_t *pbMbrEntry = pVol->pbMbrEntry + 16;

    for (unsigned i = pVol->idxEntry + 1; i < 4; i++)
    {
        if (pbMbrEntry[0x04] != 0x00)
        {
            rc = rtDvmFmtMbrVolumeCreate(pThis, pbMbrEntry, i, phVolFmtNext);
            break;
        }
        pbMbrEntry += 16;
    }

    return rc;
}

static DECLCALLBACK(void) rtDvmFmtMbrVolumeClose(RTDVMVOLUMEFMT hVolFmt)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    pVol->pVolMgr    = NULL;
    pVol->offStart   = 0;
    pVol->cbVolume   = 0;
    pVol->pbMbrEntry = NULL;

    RTMemFree(pVol);
}

static DECLCALLBACK(uint64_t) rtDvmFmtMbrVolumeGetSize(RTDVMVOLUMEFMT hVolFmt)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    return pVol->cbVolume;
}

static DECLCALLBACK(int) rtDvmFmtMbrVolumeQueryName(RTDVMVOLUMEFMT hVolFmt, char **ppszVolName)
{
    NOREF(hVolFmt); NOREF(ppszVolName);
    return VERR_NOT_SUPPORTED;
}

static DECLCALLBACK(RTDVMVOLTYPE) rtDvmFmtMbrVolumeGetType(RTDVMVOLUMEFMT hVolFmt)
{
    RTDVMVOLTYPE enmVolType = RTDVMVOLTYPE_UNKNOWN;
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    for (unsigned i = 0; i < RT_ELEMENTS(g_aFs2DvmVolTypes); i++)
        if (pVol->pbMbrEntry[0x04] == g_aFs2DvmVolTypes[i].bFsId)
        {
            enmVolType = g_aFs2DvmVolTypes[i].enmVolType;
            break;
        }

    return enmVolType;
}

static DECLCALLBACK(uint64_t) rtDvmFmtMbrVolumeGetFlags(RTDVMVOLUMEFMT hVolFmt)
{
    uint64_t fFlags = 0;
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    if (pVol->pbMbrEntry[0x00] & 0x80)
        fFlags |= DVMVOLUME_FLAGS_BOOTABLE | DVMVOLUME_FLAGS_ACTIVE;

    return fFlags;
}

static DECLCALLBACK(bool) rtDvmFmtMbrVolumeIsRangeIntersecting(RTDVMVOLUMEFMT hVolFmt,
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

static DECLCALLBACK(int) rtDvmFmtMbrVolumeRead(RTDVMVOLUMEFMT hVolFmt, uint64_t off, void *pvBuf, size_t cbRead)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;
    AssertReturn(off + cbRead <= pVol->cbVolume, VERR_INVALID_PARAMETER);

    return rtDvmDiskRead(pVol->pVolMgr->pDisk, pVol->offStart + off, pvBuf, cbRead);
}

static DECLCALLBACK(int) rtDvmFmtMbrVolumeWrite(RTDVMVOLUMEFMT hVolFmt, uint64_t off, const void *pvBuf, size_t cbWrite)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;
    AssertReturn(off + cbWrite <= pVol->cbVolume, VERR_INVALID_PARAMETER);

    return rtDvmDiskWrite(pVol->pVolMgr->pDisk, pVol->offStart + off, pvBuf, cbWrite);
}

RTDVMFMTOPS g_rtDvmFmtMbr =
{
    /* pcszFmt */
    "MBR",
    /* pfnProbe */
    rtDvmFmtMbrProbe,
    /* pfnOpen */
    rtDvmFmtMbrOpen,
    /* pfnInitialize */
    rtDvmFmtMbrInitialize,
    /* pfnClose */
    rtDvmFmtMbrClose,
    /* pfnQueryRangeUse */
    rtDvmFmtMbrQueryRangeUse,
    /* pfnGetValidVolumes */
    rtDvmFmtMbrGetValidVolumes,
    /* pfnGetMaxVolumes */
    rtDvmFmtMbrGetMaxVolumes,
    /* pfnQueryFirstVolume */
    rtDvmFmtMbrQueryFirstVolume,
    /* pfnQueryNextVolume */
    rtDvmFmtMbrQueryNextVolume,
    /* pfnVolumeClose */
    rtDvmFmtMbrVolumeClose,
    /* pfnVolumeGetSize */
    rtDvmFmtMbrVolumeGetSize,
    /* pfnVolumeQueryName */
    rtDvmFmtMbrVolumeQueryName,
    /* pfnVolumeGetType */
    rtDvmFmtMbrVolumeGetType,
    /* pfnVolumeGetFlags */
    rtDvmFmtMbrVolumeGetFlags,
    /* pfnVOlumeIsRangeIntersecting */
    rtDvmFmtMbrVolumeIsRangeIntersecting,
    /* pfnVolumeRead */
    rtDvmFmtMbrVolumeRead,
    /* pfnVolumeWrite */
    rtDvmFmtMbrVolumeWrite
};

