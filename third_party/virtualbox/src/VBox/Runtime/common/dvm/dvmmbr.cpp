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
#define LOG_GROUP RTLOGGROUP_FS
#include <iprt/types.h>
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/dvm.h>
#include <iprt/list.h>
#include <iprt/log.h>
#include <iprt/string.h>
#include "internal/dvm.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Checks if the partition type is an extended partition container. */
#define RTDVMMBR_IS_EXTENDED(a_bType) ((a_bType) == 0x05 || (a_bType) == 0x0f)


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** Pointer to a MBR sector. */
typedef struct RTDVMMBRSECTOR *PRTDVMMBRSECTOR;

/**
 * MBR entry.
 */
typedef struct RTDVMMBRENTRY
{
    /** Our entry in the in-use partition entry list (RTDVMMBRENTRY). */
    RTLISTNODE          ListEntry;
    /** Pointer to the MBR sector containing this entry. */
    PRTDVMMBRSECTOR     pSector;
    /** Pointer to the next sector in the extended partition table chain. */
    PRTDVMMBRSECTOR     pChain;
    /** The byte offset of the start of the partition (relative to disk). */
    uint64_t            offPart;
    /** Number of bytes for this partition. */
    uint64_t            cbPart;
    /** The partition/filesystem type. */
    uint8_t             bType;
    /** The partition flags. */
    uint8_t             fFlags;
    /** Bad entry. */
    bool                fBad;
} RTDVMMBRENTRY;
/** Pointer to an MBR entry. */
typedef RTDVMMBRENTRY *PRTDVMMBRENTRY;

/**
 * A MBR sector.
 */
typedef struct RTDVMMBRSECTOR
{
    /** Internal representation of the entries. */
    RTDVMMBRENTRY       aEntries[4];
    /** The byte offset of this MBR sector (relative to disk).
     * We keep this for detecting cycles now, but it will be needed if we start
     * updating the partition table at some point. */
    uint64_t            offOnDisk;
    /** Pointer to the previous sector if this isn't a primary one. */
    PRTDVMMBRENTRY      pPrevSector;
    /** Set if this is the primary MBR, cleared if an extended. */
    bool                fIsPrimary;
    /** Number of used entries. */
    uint8_t             cUsed;
    /** Number of extended entries. */
    uint8_t             cExtended;
    /** The extended entry we're following (we only follow one, except when
     *  fIsPrimary is @c true). UINT8_MAX if none. */
    uint8_t             idxExtended;
    /** The raw data. */
    uint8_t             abData[512];
} RTDVMMBRSECTOR;

/**
 * MBR volume manager data.
 */
typedef struct RTDVMFMTINTERNAL
{
    /** Pointer to the underlying disk. */
    PCRTDVMDISK         pDisk;
    /** Head of the list of in-use RTDVMMBRENTRY structures.  This excludes
     *  extended partition table entries. */
    RTLISTANCHOR        PartitionHead;
    /** The total number of partitions, not counting extended ones. */
    uint32_t            cPartitions;
    /** The actual primary MBR sector. */
    RTDVMMBRSECTOR      Primary;
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
    /** The MBR entry.    */
    PRTDVMMBRENTRY      pEntry;
} RTDVMVOLUMEFMTINTERNAL;
/** Pointer to an MBR volume. */
typedef RTDVMVOLUMEFMTINTERNAL *PRTDVMVOLUMEFMTINTERNAL;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Mapping of FS types to DVM volume types.
 *
 * @see https://en.wikipedia.org/wiki/Partition_type
 * @see http://www.win.tue.nl/~aeb/partitions/partition_types-1.html
 */
static const struct RTDVMMBRFS2VOLTYPE
{
    /** MBR FS Id. */
    uint8_t             bFsId;
    /** DVM volume type. */
    RTDVMVOLTYPE        enmVolType;
} g_aFs2DvmVolTypes[] =
{
    { 0x01, RTDVMVOLTYPE_FAT12 },
    { 0x04, RTDVMVOLTYPE_FAT16 },
    { 0x06, RTDVMVOLTYPE_FAT16 }, /* big FAT16 */
    { 0x07, RTDVMVOLTYPE_NTFS }, /* Simplification: Used for HPFS, exFAT, ++, too but NTFS is the more common one. */
    { 0x0b, RTDVMVOLTYPE_FAT32 },
    { 0x0c, RTDVMVOLTYPE_FAT32 },
    { 0x0e, RTDVMVOLTYPE_FAT16 },

    /* Hidden variants of the above: */
    { 0x11, RTDVMVOLTYPE_FAT12 },
    { 0x14, RTDVMVOLTYPE_FAT16 },
    { 0x16, RTDVMVOLTYPE_FAT16 },
    { 0x17, RTDVMVOLTYPE_NTFS },
    { 0x1b, RTDVMVOLTYPE_FAT32 },
    { 0x1c, RTDVMVOLTYPE_FAT32 },
    { 0x1e, RTDVMVOLTYPE_FAT16 },

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
    *puScore = RTDVM_MATCH_SCORE_UNSUPPORTED;
    if (pDisk->cbDisk >= 512)
    {
        /* Read from the disk and check for the 0x55aa signature at the end. */
        uint8_t abMbr[512];
        rc = rtDvmDiskRead(pDisk, 0, &abMbr[0], sizeof(abMbr));
        if (   RT_SUCCESS(rc)
            && abMbr[510] == 0x55
            && abMbr[511] == 0xaa)
            *puScore = RTDVM_MATCH_SCORE_SUPPORTED; /* Not perfect because GPTs have a protective MBR. */
    }

    return rc;
}


static void rtDvmFmtMbrDestroy(PRTDVMFMTINTERNAL pThis)
{
    /*
     * Delete chains of extended partitions.
     */
    for (unsigned i = 0; i < 4; i++)
    {
        PRTDVMMBRSECTOR pCur = pThis->Primary.aEntries[i].pChain;
        while (pCur)
        {
            PRTDVMMBRSECTOR pNext = pCur->idxExtended != UINT8_MAX ? pCur->aEntries[pCur->idxExtended].pChain : NULL;

            RT_ZERO(pCur->aEntries);
            pCur->pPrevSector = NULL;
            RTMemFree(pCur);

            pCur = pNext;
        }
    }

    /*
     * Now kill this.
     */
    pThis->pDisk = NULL;
    RT_ZERO(pThis->Primary.aEntries);
    RTMemFree(pThis);
}


static int rtDvmFmtMbrReadExtended(PRTDVMFMTINTERNAL pThis, PRTDVMMBRENTRY pPrimaryEntry)
{
    uint32_t const  cbExt       = pPrimaryEntry->cbPart;
    uint64_t const  offExtBegin = pPrimaryEntry->offPart;

    uint64_t        offCurBegin = offExtBegin;
    PRTDVMMBRENTRY  pCurEntry   = pPrimaryEntry;
    for (unsigned cTables = 1; ; cTables++)
    {
        /*
         * Do some sanity checking.
         */
        /* Check the address of the partition table. */
        if (offCurBegin - offExtBegin >= cbExt)
        {
            LogRel(("rtDvmFmtMbrReadExtended: offCurBegin=%#RX64 is outside the extended partition: %#RX64..%#RX64 (LB %#RX64)\n",
                    offCurBegin, offExtBegin, offExtBegin + cbExt - 1, cbExt));
            pCurEntry->fBad = true;
            return -VERR_OUT_OF_RANGE;
        }

        /* Limit the chain length. */
        if (cTables > 64)
        {
            LogRel(("rtDvmFmtMbrReadExtended: offCurBegin=%#RX64 is the %uth table, we stop here.\n", offCurBegin, cTables));
            pCurEntry->fBad = true;
            return -VERR_TOO_MANY_SYMLINKS;
        }

        /* Check for obvious cycles. */
        for (PRTDVMMBRENTRY pPrev = pCurEntry->pSector->pPrevSector; pPrev != NULL; pPrev = pPrev->pSector->pPrevSector)
            if (pPrev->offPart == offCurBegin)
            {
                LogRel(("rtDvmFmtMbrReadExtended: Cycle! We've seen offCurBegin=%#RX64 before\n", offCurBegin));
                pCurEntry->fBad = true;
                return -VERR_TOO_MANY_SYMLINKS;
            }

        /*
         * Allocate a new sector entry and read the sector with the table.
         */
        PRTDVMMBRSECTOR pNext = (PRTDVMMBRSECTOR)RTMemAllocZ(sizeof(*pNext));
        if (!pNext)
            return VERR_NO_MEMORY;
        pNext->offOnDisk    = offCurBegin;
        pNext->pPrevSector  = pCurEntry;
        //pNext->fIsPrimary = false;
        //pNext->cUsed      = 0;
        //pNext->cExtended  = 0;
        pNext->idxExtended  = UINT8_MAX;

        int rc = rtDvmDiskRead(pThis->pDisk, pNext->offOnDisk, &pNext->abData[0], sizeof(pNext->abData));
        if (   RT_FAILURE(rc)
            || pNext->abData[510] != 0x55
            || pNext->abData[511] != 0xaa)
        {
            if (RT_FAILURE(rc))
                LogRel(("rtDvmFmtMbrReadExtended: Error reading extended partition table at sector %#RX64: %Rrc\n", offCurBegin, rc));
            else
                LogRel(("rtDvmFmtMbrReadExtended: Extended partition table at sector %#RX64 does not have a valid DOS signature: %#x %#x\n",
                        offCurBegin, pNext->abData[510], pNext->abData[511]));
            RTMemFree(pNext);
            pCurEntry->fBad = true;
            return rc;
        }
        pCurEntry->pChain = pNext;

        /*
         * Process the table, taking down the first forward entry.
         *
         * As noted in the caller of this function, we only deal with one extended
         * partition entry at this level since noone really ever put more than one
         * here anyway.
         */
        PRTDVMMBRENTRY pEntry     = &pNext->aEntries[0];
        uint8_t       *pbMbrEntry = &pNext->abData[446];
        for (unsigned i = 0; i < 4; i++, pEntry++, pbMbrEntry += 16)
        {
            uint8_t const bType  = pbMbrEntry[4];
            pEntry->pSector = pNext;
            RTListInit(&pEntry->ListEntry);
            if (bType != 0)
            {
                pEntry->bType    = bType;
                pEntry->fFlags   = pbMbrEntry[0];
                pEntry->offPart  = RT_MAKE_U32_FROM_U8(pbMbrEntry[0x08],
                                                       pbMbrEntry[0x08 + 1],
                                                       pbMbrEntry[0x08 + 2],
                                                       pbMbrEntry[0x08 + 3]);
                pEntry->offPart *= 512;
                pEntry->cbPart   = RT_MAKE_U32_FROM_U8(pbMbrEntry[0x0c],
                                                       pbMbrEntry[0x0c + 1],
                                                       pbMbrEntry[0x0c + 2],
                                                       pbMbrEntry[0x0c + 3]);
                pEntry->cbPart  *= 512;
                if (!RTDVMMBR_IS_EXTENDED(bType))
                {
                    pEntry->offPart += offCurBegin;
                    pThis->cPartitions++;
                    RTListAppend(&pThis->PartitionHead, &pEntry->ListEntry);
                    Log2(("rtDvmFmtMbrReadExtended: %#012RX64::%u: vol%u bType=%#04x fFlags=%#04x offPart=%#012RX64 cbPart=%#012RX64\n",
                          offCurBegin, i, pThis->cPartitions - 1, pEntry->bType, pEntry->fFlags, pEntry->offPart, pEntry->cbPart));
                }
                else
                {
                    pEntry->offPart += offExtBegin;
                    pNext->cExtended++;
                    if (pNext->idxExtended == UINT8_MAX)
                        pNext->idxExtended = (uint8_t)i;
                    else
                    {
                        pEntry->fBad = true;
                        LogRel(("rtDvmFmtMbrReadExtended: Warning! Both #%u and #%u are extended partition table entries! Only following the former\n",
                                i, pNext->idxExtended));
                    }
                    Log2(("rtDvmFmtMbrReadExtended: %#012RX64::%u: ext%u bType=%#04x fFlags=%#04x offPart=%#012RX64 cbPart=%#012RX64\n",
                          offCurBegin, i, pNext->cExtended - 1, pEntry->bType, pEntry->fFlags, pEntry->offPart, pEntry->cbPart));
                }
                pNext->cUsed++;

            }
            /* else: unused */
        }

        /*
         * We're done if we didn't find any extended partition table entry.
         * Otherwise, advance to the next one.
         */
        if (!pNext->cExtended)
            return VINF_SUCCESS;
        pCurEntry   = &pNext->aEntries[pNext->idxExtended];
        offCurBegin = pCurEntry->offPart;
    }
}


static DECLCALLBACK(int) rtDvmFmtMbrOpen(PCRTDVMDISK pDisk, PRTDVMFMT phVolMgrFmt)
{
    int rc;
    PRTDVMFMTINTERNAL pThis = (PRTDVMFMTINTERNAL)RTMemAllocZ(sizeof(RTDVMFMTINTERNAL));
    if (pThis)
    {
        pThis->pDisk            = pDisk;
        //pThis->cPartitions    = 0;
        RTListInit(&pThis->PartitionHead);
        //pThis->Primary.offOnDisk   = 0;
        //pThis->Primary.pPrevSector = NULL;
        pThis->Primary.fIsPrimary    = true;
        //pThis->Primary.cUsed       = 0;
        //pThis->Primary.cExtended   = 0;
        pThis->Primary.idxExtended   = UINT8_MAX;

        /*
         * Read the primary MBR.
         */
        rc = rtDvmDiskRead(pDisk, 0, &pThis->Primary.abData[0], sizeof(pThis->Primary.abData));
        if (RT_SUCCESS(rc))
        {
            Assert(pThis->Primary.abData[510] == 0x55 && pThis->Primary.abData[511] == 0xaa);

            /*
             * Setup basic data for the 4 entries.
             */
            PRTDVMMBRENTRY pEntry     = &pThis->Primary.aEntries[0];
            uint8_t       *pbMbrEntry = &pThis->Primary.abData[446];
            for (unsigned i = 0; i < 4; i++, pEntry++, pbMbrEntry += 16)
            {
                pEntry->pSector = &pThis->Primary;
                RTListInit(&pEntry->ListEntry);

                uint8_t const bType = pbMbrEntry[4];
                if (bType != 0)
                {
                    pEntry->offPart  = RT_MAKE_U32_FROM_U8(pbMbrEntry[0x08 + 0],
                                                           pbMbrEntry[0x08 + 1],
                                                           pbMbrEntry[0x08 + 2],
                                                           pbMbrEntry[0x08 + 3]);
                    pEntry->offPart *= 512;
                    pEntry->cbPart   = RT_MAKE_U32_FROM_U8(pbMbrEntry[0x0c + 0],
                                                           pbMbrEntry[0x0c + 1],
                                                           pbMbrEntry[0x0c + 2],
                                                           pbMbrEntry[0x0c + 3]);
                    pEntry->cbPart  *= 512;
                    pEntry->bType    = bType;
                    pEntry->fFlags   = pbMbrEntry[0];
                    if (!RTDVMMBR_IS_EXTENDED(bType))
                    {
                        pThis->cPartitions++;
                        RTListAppend(&pThis->PartitionHead, &pEntry->ListEntry);
                        Log2(("rtDvmFmtMbrOpen: %u: vol%u bType=%#04x fFlags=%#04x offPart=%#012RX64 cbPart=%#012RX64\n",
                              i, pThis->cPartitions - 1, pEntry->bType, pEntry->fFlags, pEntry->offPart, pEntry->cbPart));
                    }
                    else
                    {
                        pThis->Primary.cExtended++;
                        Log2(("rtDvmFmtMbrOpen: %u: ext%u bType=%#04x fFlags=%#04x offPart=%#012RX64 cbPart=%#012RX64\n",
                              i, pThis->Primary.cExtended - 1, pEntry->bType, pEntry->fFlags, pEntry->offPart, pEntry->cbPart));
                    }
                    pThis->Primary.cUsed++;
                }
                /* else: unused */
            }

            /*
             * Now read any extended partitions.  Since it's no big deal for us, we allow
             * the primary partition table to have more than one extended partition.  However
             * in the extended tables we only allow a single forward link to avoid having to
             * deal with recursion.
             */
            if (pThis->Primary.cExtended > 0)
                for (unsigned i = 0; i < 4; i++)
                    if (RTDVMMBR_IS_EXTENDED(pThis->Primary.aEntries[i].bType))
                    {
                        if (pThis->Primary.idxExtended == UINT8_MAX)
                            pThis->Primary.idxExtended = (uint8_t)i;
                        rc = rtDvmFmtMbrReadExtended(pThis, &pThis->Primary.aEntries[i]);
                        if (RT_FAILURE(rc))
                            break;
                    }
            if (RT_SUCCESS(rc))
            {
                *phVolMgrFmt = pThis;
                return rc;
            }

        }
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

static DECLCALLBACK(int) rtDvmFmtMbrInitialize(PCRTDVMDISK pDisk, PRTDVMFMT phVolMgrFmt)
{
    int rc;
    PRTDVMFMTINTERNAL pThis = (PRTDVMFMTINTERNAL)RTMemAllocZ(sizeof(RTDVMFMTINTERNAL));
    if (pThis)
    {
        pThis->pDisk            = pDisk;
        //pThis->cPartitions    = 0;
        RTListInit(&pThis->PartitionHead);
        //pThis->Primary.offOnDisk   = 0
        //pThis->Primary.pPrevSector = NULL;
        pThis->Primary.fIsPrimary    = true;
        //pThis->Primary.cUsed       = 0;
        //pThis->Primary.cExtended   = 0;
        pThis->Primary.idxExtended   = UINT8_MAX;

        /* Setup a new MBR and write it to the disk. */
        pThis->Primary.abData[510] = 0x55;
        pThis->Primary.abData[511] = 0xaa;
        rc = rtDvmDiskWrite(pDisk, 0, &pThis->Primary.abData[0], sizeof(pThis->Primary.abData));
        if (RT_SUCCESS(rc))
        {
            pThis->pDisk = pDisk;
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
    rtDvmFmtMbrDestroy(hVolMgrFmt);
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
 * @param   pThis       The MBR volume manager data.
 * @param   pEntry      The MBR entry to create a volume handle for.
 * @param   phVolFmt    Where to store the volume data on success.
 */
static int rtDvmFmtMbrVolumeCreate(PRTDVMFMTINTERNAL pThis, PRTDVMMBRENTRY pEntry, PRTDVMVOLUMEFMT phVolFmt)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = (PRTDVMVOLUMEFMTINTERNAL)RTMemAllocZ(sizeof(RTDVMVOLUMEFMTINTERNAL));
    if (pVol)
    {
        pVol->pVolMgr    = pThis;
        pVol->pEntry     = pEntry;
        *phVolFmt = pVol;
        return VINF_SUCCESS;
    }
    return VERR_NO_MEMORY;
}

static DECLCALLBACK(int) rtDvmFmtMbrQueryFirstVolume(RTDVMFMT hVolMgrFmt, PRTDVMVOLUMEFMT phVolFmt)
{
    PRTDVMFMTINTERNAL pThis = hVolMgrFmt;
    if (pThis->cPartitions != 0)
        return rtDvmFmtMbrVolumeCreate(pThis, RTListGetFirst(&pThis->PartitionHead, RTDVMMBRENTRY, ListEntry), phVolFmt);
    return VERR_DVM_MAP_EMPTY;
}

static DECLCALLBACK(int) rtDvmFmtMbrQueryNextVolume(RTDVMFMT hVolMgrFmt, RTDVMVOLUMEFMT hVolFmt, PRTDVMVOLUMEFMT phVolFmtNext)
{
    PRTDVMFMTINTERNAL       pThis   = hVolMgrFmt;
    PRTDVMVOLUMEFMTINTERNAL pCurVol = hVolFmt;
    if (pCurVol)
    {
        PRTDVMMBRENTRY pNextEntry = RTListGetNext(&pThis->PartitionHead, pCurVol->pEntry, RTDVMMBRENTRY, ListEntry);
        if (pNextEntry)
            return rtDvmFmtMbrVolumeCreate(pThis, pNextEntry, phVolFmtNext);
        return VERR_DVM_MAP_NO_VOLUME;
    }
    if (pThis->cPartitions != 0)
        return rtDvmFmtMbrVolumeCreate(pThis, RTListGetFirst(&pThis->PartitionHead, RTDVMMBRENTRY, ListEntry), phVolFmtNext);
    return VERR_DVM_MAP_EMPTY;
}

static DECLCALLBACK(void) rtDvmFmtMbrVolumeClose(RTDVMVOLUMEFMT hVolFmt)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    pVol->pVolMgr    = NULL;
    pVol->pEntry     = NULL;

    RTMemFree(pVol);
}

static DECLCALLBACK(uint64_t) rtDvmFmtMbrVolumeGetSize(RTDVMVOLUMEFMT hVolFmt)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    return pVol->pEntry->cbPart;
}

static DECLCALLBACK(int) rtDvmFmtMbrVolumeQueryName(RTDVMVOLUMEFMT hVolFmt, char **ppszVolName)
{
    NOREF(hVolFmt); NOREF(ppszVolName);
    return VERR_NOT_SUPPORTED;
}

static DECLCALLBACK(RTDVMVOLTYPE) rtDvmFmtMbrVolumeGetType(RTDVMVOLUMEFMT hVolFmt)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    uint8_t const bType = pVol->pEntry->bType;
    for (unsigned i = 0; i < RT_ELEMENTS(g_aFs2DvmVolTypes); i++)
        if (g_aFs2DvmVolTypes[i].bFsId == bType)
            return g_aFs2DvmVolTypes[i].enmVolType;

    return RTDVMVOLTYPE_UNKNOWN;
}

static DECLCALLBACK(uint64_t) rtDvmFmtMbrVolumeGetFlags(RTDVMVOLUMEFMT hVolFmt)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    uint64_t fFlags = 0;
    if (pVol->pEntry->bType & 0x80)
        fFlags |= DVMVOLUME_FLAGS_BOOTABLE | DVMVOLUME_FLAGS_ACTIVE;

    return fFlags;
}

static DECLCALLBACK(bool) rtDvmFmtMbrVolumeIsRangeIntersecting(RTDVMVOLUMEFMT hVolFmt,
                                                               uint64_t offStart, size_t cbRange,
                                                               uint64_t *poffVol,
                                                               uint64_t *pcbIntersect)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;

    if (RTDVM_RANGE_IS_INTERSECTING(pVol->pEntry->offPart, pVol->pEntry->cbPart, offStart))
    {
        *poffVol      = offStart - pVol->pEntry->offPart;
        *pcbIntersect = RT_MIN(cbRange, pVol->pEntry->offPart + pVol->pEntry->cbPart - offStart);
        return true;
    }
    return false;
}

static DECLCALLBACK(int) rtDvmFmtMbrVolumeRead(RTDVMVOLUMEFMT hVolFmt, uint64_t off, void *pvBuf, size_t cbRead)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;
    AssertReturn(off + cbRead <= pVol->pEntry->cbPart, VERR_INVALID_PARAMETER);

    return rtDvmDiskRead(pVol->pVolMgr->pDisk, pVol->pEntry->offPart + off, pvBuf, cbRead);
}

static DECLCALLBACK(int) rtDvmFmtMbrVolumeWrite(RTDVMVOLUMEFMT hVolFmt, uint64_t off, const void *pvBuf, size_t cbWrite)
{
    PRTDVMVOLUMEFMTINTERNAL pVol = hVolFmt;
    AssertReturn(off + cbWrite <= pVol->pEntry->cbPart, VERR_INVALID_PARAMETER);

    return rtDvmDiskWrite(pVol->pVolMgr->pDisk, pVol->pEntry->offPart + off, pvBuf, cbWrite);
}

RTDVMFMTOPS g_rtDvmFmtMbr =
{
    /* pszFmt */
    "MBR",
    /* enmFormat */
    RTDVMFORMATTYPE_MBR,
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

