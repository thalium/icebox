/* $Id: fatvfs.cpp $ */
/** @file
 * IPRT - FAT Virtual Filesystem.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
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
#include "internal/iprt.h"
#include <iprt/fsvfs.h>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/file.h>
#include <iprt/err.h>
#include <iprt/log.h>
#include <iprt/mem.h>
#include <iprt/path.h>
#include <iprt/poll.h>
#include <iprt/rand.h>
#include <iprt/string.h>
#include <iprt/sg.h>
#include <iprt/thread.h>
#include <iprt/uni.h>
#include <iprt/vfs.h>
#include <iprt/vfslowlevel.h>
#include <iprt/zero.h>
#include <iprt/formats/fat.h>

#include "internal/fs.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/**
 * Gets the cluster from a directory entry.
 *
 * @param   a_pDirEntry Pointer to the directory entry.
 * @param   a_pVol      Pointer to the volume.
 */
#define RTFSFAT_GET_CLUSTER(a_pDirEntry, a_pVol) \
    (  (a_pVol)->enmFatType >= RTFSFATTYPE_FAT32 \
      ? RT_MAKE_U32((a_pDirEntry)->idxCluster, (a_pDirEntry)->u.idxClusterHigh) \
      : (a_pDirEntry)->idxCluster )

/**
 * Rotates a unsigned 8-bit value one bit to the right.
 *
 * @returns Rotated 8-bit value.
 * @param   a_bValue        The value to rotate.
 */
#define RTFSFAT_ROT_R1_U8(a_bValue) (((a_bValue) >> 1) | (uint8_t)((a_bValue) << 7))


/** Maximum number of characters we will create in a long file name. */
#define RTFSFAT_MAX_LFN_CHARS   255


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** Pointer to a FAT directory instance. */
typedef struct RTFSFATDIR *PRTFSFATDIR;


/** The number of entire in a chain part. */
#define RTFSFATCHAINPART_ENTRIES (256U - 4U)

/**
 * A part of the cluster chain covering up to 252 clusters.
 */
typedef struct RTFSFATCHAINPART
{
    /** List entry. */
    RTLISTNODE  ListEntry;
    /** Chain entries. */
    uint32_t    aEntries[RTFSFATCHAINPART_ENTRIES];
} RTFSFATCHAINPART;
AssertCompile(sizeof(RTFSFATCHAINPART) <= _1K);
typedef RTFSFATCHAINPART *PRTFSFATCHAINPART;
typedef RTFSFATCHAINPART const *PCRTFSFATCHAINPART;


/**
 * A FAT cluster chain.
 */
typedef struct RTFSFATCHAIN
{
    /** The chain size in bytes. */
    uint32_t        cbChain;
    /** The chain size in entries. */
    uint32_t        cClusters;
    /** The cluster size. */
    uint32_t        cbCluster;
    /** The shift count for converting between clusters and bytes. */
    uint8_t         cClusterByteShift;
    /** List of chain parts (RTFSFATCHAINPART). */
    RTLISTANCHOR    ListParts;
} RTFSFATCHAIN;
typedef RTFSFATCHAIN *PRTFSFATCHAIN;
typedef RTFSFATCHAIN const *PCRTFSFATCHAIN;


/**
 * FAT file system object (common part to files and dirs).
 */
typedef struct RTFSFATOBJ
{
    /** The parent directory keeps a list of open objects (RTFSFATOBJ). */
    RTLISTNODE          Entry;
    /** The parent directory (not released till all children are close). */
    PRTFSFATDIR         pParentDir;
    /** The byte offset of the directory entry in the parent dir.
     * This is set to UINT32_MAX for the root directory. */
    uint32_t            offEntryInDir;
    /** Attributes. */
    RTFMODE             fAttrib;
    /** The object size. */
    uint32_t            cbObject;
    /** The access time. */
    RTTIMESPEC          AccessTime;
    /** The modificaton time. */
    RTTIMESPEC          ModificationTime;
    /** The birth time. */
    RTTIMESPEC          BirthTime;
    /** Cluster chain. */
    RTFSFATCHAIN        Clusters;
    /** Pointer to the volume. */
    struct RTFSFATVOL  *pVol;
    /** Set if we've maybe dirtied the FAT. */
    bool                fMaybeDirtyFat;
    /** Set if we've maybe dirtied the directory entry. */
    bool                fMaybeDirtyDirEnt;
} RTFSFATOBJ;
typedef RTFSFATOBJ *PRTFSFATOBJ;

typedef struct RTFSFATFILE
{
    /** Core FAT object info.  */
    RTFSFATOBJ          Core;
    /** The current file offset. */
    uint32_t            offFile;
} RTFSFATFILE;
typedef RTFSFATFILE *PRTFSFATFILE;


/**
 * FAT directory.
 *
 * We work directories in one of two buffering modes.  If there are few entries
 * or if it's the FAT12/16 root directory, we map the whole thing into memory.
 * If it's too large, we use an inefficient sector buffer for now.
 *
 * Directory entry updates happens exclusively via the directory, so any open
 * files or subdirs have a parent reference for doing that.  The parent OTOH,
 * keeps a list of open children.
 */
typedef struct RTFSFATDIR
{
    /** Core FAT object info.  */
    RTFSFATOBJ          Core;
    /** The VFS handle for this directory (for reference counting). */
    RTVFSDIR            hVfsSelf;
    /** Open child objects (RTFSFATOBJ). */
    RTLISTNODE          OpenChildren;

    /** Number of directory entries. */
    uint32_t            cEntries;

    /** If fully buffered. */
    bool                fFullyBuffered;
    /** Set if this is a linear root directory. */
    bool                fIsLinearRootDir;
    /** The size of the memory paEntries points at. */
    uint32_t            cbAllocatedForEntries;

    /** Pointer to the directory buffer.
     * In fully buffering mode, this is the whole of the directory.  Otherwise it's
     * just a sector worth of buffers.  */
    PFATDIRENTRYUNION   paEntries;
    /** The disk offset corresponding to what paEntries points to.
     * UINT64_MAX if notthing read into paEntries yet. */
    uint64_t            offEntriesOnDisk;
    union
    {
        /** Data for the full buffered mode.
         * No need to messing around with clusters here, as we only uses this for
         * directories with a contiguous mapping on the disk.
         * So, if we grow a directory in a non-contiguous manner, we have to switch
         * to sector buffering on the fly. */
        struct
        {
            /** Number of sectors mapped by paEntries and pbDirtySectors. */
            uint32_t            cSectors;
            /** Number of dirty sectors. */
            uint32_t            cDirtySectors;
            /** Dirty sector map. */
            uint8_t            *pbDirtySectors;
        } Full;
        /** The simple sector buffering.
         * This only works for clusters, so no FAT12/16 root directory fun. */
        struct
        {
            /** The directory offset, UINT32_MAX if invalid. */
            uint32_t            offInDir;
            /** Dirty flag. */
            bool                fDirty;
        } Simple;
    } u;
} RTFSFATDIR;
/** Pointer to a FAT directory instance. */
typedef RTFSFATDIR *PRTFSFATDIR;


/**
 * File allocation table cache entry.
 */
typedef struct RTFSFATCLUSTERMAPENTRY
{
    /** The byte offset into the fat, UINT32_MAX if invalid entry. */
    uint32_t                offFat;
    /** Pointer to the data. */
    uint8_t                *pbData;
    /** Dirty bitmap.  Indexed by byte offset right shifted by
     * RTFSFATCLUSTERMAPCACHE::cDirtyShift. */
    uint64_t                bmDirty;
} RTFSFATCLUSTERMAPENTRY;
/** Pointer to a file allocation table cache entry.  */
typedef RTFSFATCLUSTERMAPENTRY *PRTFSFATCLUSTERMAPENTRY;

/**
 * File allocation table cache.
 */
typedef struct RTFSFATCLUSTERMAPCACHE
{
    /** Number of cache entries. */
    uint32_t                cEntries;
    /** The max size of data in a cache entry. */
    uint32_t                cbEntry;
    /** Dirty bitmap shift count. */
    uint32_t                cDirtyShift;
    /** The dirty cache line size (multiple of two). */
    uint32_t                cbDirtyLine;
    /** The cache name. */
    const char             *pszName;
    /** Cache entries. */
    RTFSFATCLUSTERMAPENTRY  aEntries[RT_FLEXIBLE_ARRAY];
} RTFSFATCLUSTERMAPCACHE;
/** Pointer to a FAT linear metadata cache. */
typedef RTFSFATCLUSTERMAPCACHE *PRTFSFATCLUSTERMAPCACHE;


/**
 * BPB version.
 */
typedef enum RTFSFATBPBVER
{
    RTFSFATBPBVER_INVALID = 0,
    RTFSFATBPBVER_NO_BPB,
    RTFSFATBPBVER_DOS_2_0,
    //RTFSFATBPBVER_DOS_3_2, - we don't try identify this one.
    RTFSFATBPBVER_DOS_3_31,
    RTFSFATBPBVER_EXT_28,
    RTFSFATBPBVER_EXT_29,
    RTFSFATBPBVER_FAT32_28,
    RTFSFATBPBVER_FAT32_29,
    RTFSFATBPBVER_END
} RTFSFATBPBVER;


/**
 * A FAT volume.
 */
typedef struct RTFSFATVOL
{
    /** Handle to itself. */
    RTVFS           hVfsSelf;
    /** The file, partition, or whatever backing the FAT volume. */
    RTVFSFILE       hVfsBacking;
    /** The size of the backing thingy. */
    uint64_t        cbBacking;
    /** Byte offset of the bootsector relative to the start of the file. */
    uint64_t        offBootSector;
    /** The UTC offset in nanoseconds to use for this file system (FAT traditionally
     * stores timestamps in local time).
     * @remarks This may need improving later. */
    int64_t         offNanoUTC;
    /** The UTC offset in minutes to use for this file system (FAT traditionally
     * stores timestamps in local time).
     * @remarks This may need improving later. */
    int32_t         offMinUTC;
    /** Set if read-only mode. */
    bool            fReadOnly;
    /** Media byte. */
    uint8_t         bMedia;
    /** Reserved sectors. */
    uint32_t        cReservedSectors;
    /** The BPB version.  Gives us an idea of the FAT file system version. */
    RTFSFATBPBVER   enmBpbVersion;

    /** Logical sector size. */
    uint32_t        cbSector;
    /** The shift count for converting between sectors and bytes. */
    uint8_t         cSectorByteShift;
    /** The shift count for converting between clusters and bytes. */
    uint8_t         cClusterByteShift;
    /** The cluster size in bytes. */
    uint32_t        cbCluster;
    /** The number of data clusters, including the two reserved ones. */
    uint32_t        cClusters;
    /** The offset of the first cluster. */
    uint64_t        offFirstCluster;
    /** The total size from the BPB, in bytes. */
    uint64_t        cbTotalSize;

    /** The FAT type. */
    RTFSFATTYPE     enmFatType;

    /** Number of FAT entries (clusters). */
    uint32_t        cFatEntries;
    /** The size of a FAT, in bytes. */
    uint32_t        cbFat;
    /** Number of FATs. */
    uint32_t        cFats;
    /** The end of chain marker used by the formatter (FAT entry \#2). */
    uint32_t        idxEndOfChain;
    /** The maximum last cluster supported by the FAT format. */
    uint32_t        idxMaxLastCluster;
    /** FAT byte offsets.  */
    uint64_t        aoffFats[8];
    /** Pointer to the FAT (cluster map) cache. */
    PRTFSFATCLUSTERMAPCACHE pFatCache;

    /** The root directory byte offset. */
    uint64_t        offRootDir;
    /** Root directory cluster, UINT32_MAX if not FAT32. */
    uint32_t        idxRootDirCluster;
    /** Number of root directory entries, if fixed.  UINT32_MAX for FAT32. */
    uint32_t        cRootDirEntries;
    /** The size of the root directory, rounded up to the nearest sector size. */
    uint32_t        cbRootDir;
    /** The root directory handle. */
    RTVFSDIR        hVfsRootDir;
    /** The root directory instance data. */
    PRTFSFATDIR     pRootDir;

    /** Serial number. */
    uint32_t        uSerialNo;
    /** The stripped volume label, if included in EBPB. */
    char            szLabel[12];
    /** The file system type from the EBPB (also stripped).  */
    char            szType[9];
    /** Number of FAT32 boot sector copies.   */
    uint8_t         cBootSectorCopies;
    /** FAT32 flags. */
    uint16_t        fFat32Flags;
    /** Offset of the FAT32 boot sector copies, UINT64_MAX if none. */
    uint64_t        offBootSectorCopies;

    /** The FAT32 info sector byte offset, UINT64_MAX if not present. */
    uint64_t        offFat32InfoSector;
    /** The FAT32 info sector if offFat32InfoSector isn't UINT64_MAX. */
    FAT32INFOSECTOR Fat32InfoSector;
} RTFSFATVOL;
/** Pointer to a FAT volume (VFS instance data). */
typedef RTFSFATVOL *PRTFSFATVOL;
/** Pointer to a const FAT volume (VFS instance data). */
typedef RTFSFATVOL const *PCRTFSFATVOL;



/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Codepage 437 translation table with invalid 8.3 characters marked as 0xffff or 0xfffe.
 *
 * The 0xfffe notation is used for characters that are valid in long file names but not short.
 *
 * @remarks The valid first 128 entries are 1:1 with unicode.
 * @remarks Lower case characters are all marked invalid.
 */
static RTUTF16 g_awchFatCp437Chars[] =
{ /*     0,      1,      2,      3,      4,      5,      6,      7,      8,      9,      a,      b,      c,      d,      e,      f */
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0xffff, 0xfffe, 0xfffe, 0x002d, 0xfffe, 0xffff,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0xffff, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xffff,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0xfffe, 0xffff, 0xfffe, 0x005e, 0x005f,
    0x0060, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe,
    0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xfffe, 0xffff, 0xffff, 0xffff, 0x007e, 0xffff,
    0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
    0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,
    0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba, 0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
    0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
    0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4, 0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,
    0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0
};
AssertCompileSize(g_awchFatCp437Chars, 256*2);


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void rtFsFatDir_AddOpenChild(PRTFSFATDIR pDir, PRTFSFATOBJ pChild);
static void rtFsFatDir_RemoveOpenChild(PRTFSFATDIR pDir, PRTFSFATOBJ pChild);
static int  rtFsFatDir_GetEntryForUpdate(PRTFSFATDIR pThis, uint32_t offEntryInDir, PFATDIRENTRY *ppDirEntry, uint32_t *puWriteLock);
static int  rtFsFatDir_PutEntryAfterUpdate(PRTFSFATDIR pThis, PFATDIRENTRY pDirEntry, uint32_t uWriteLock);
static int  rtFsFatDir_Flush(PRTFSFATDIR pThis);
static int  rtFsFatDir_New(PRTFSFATVOL pThis, PRTFSFATDIR pParentDir, PCFATDIRENTRY pDirEntry, uint32_t offEntryInDir,
                           uint32_t idxCluster, uint64_t offDisk, uint32_t cbDir, PRTVFSDIR phVfsDir, PRTFSFATDIR *ppDir);


/**
 * Convers a cluster to a disk offset.
 *
 * @returns Disk byte offset, UINT64_MAX on invalid cluster.
 * @param   pThis               The FAT volume instance.
 * @param   idxCluster          The cluster number.
 */
DECLINLINE(uint64_t) rtFsFatClusterToDiskOffset(PRTFSFATVOL pThis, uint32_t idxCluster)
{
    AssertReturn(idxCluster >= FAT_FIRST_DATA_CLUSTER, UINT64_MAX);
    AssertReturn(idxCluster < pThis->cClusters, UINT64_MAX);
    return (idxCluster - FAT_FIRST_DATA_CLUSTER) * (uint64_t)pThis->cbCluster
         + pThis->offFirstCluster;
}


#ifdef RT_STRICT
/**
 * Assert chain consistency.
 */
static bool rtFsFatChain_AssertValid(PCRTFSFATCHAIN pChain)
{
    bool                fRc = true;
    uint32_t            cParts = 0;
    PRTFSFATCHAINPART   pPart;
    RTListForEach(&pChain->ListParts, pPart, RTFSFATCHAINPART, ListEntry)
        cParts++;

    uint32_t cExpected = (pChain->cClusters + RTFSFATCHAINPART_ENTRIES - 1) / RTFSFATCHAINPART_ENTRIES;
    AssertMsgStmt(cExpected == cParts, ("cExpected=%#x cParts=%#x\n", cExpected, cParts), fRc = false);
    AssertMsgStmt(pChain->cbChain == (pChain->cClusters << pChain->cClusterByteShift),
                  ("cExpected=%#x cParts=%#x\n", cExpected, cParts), fRc = false);
    return fRc;
}
#endif /* RT_STRICT */


/**
 * Initializes an empty cluster chain.
 *
 * @param   pChain              The chain.
 * @param   pVol                The volume.
 */
static void rtFsFatChain_InitEmpty(PRTFSFATCHAIN pChain, PRTFSFATVOL pVol)
{
    pChain->cbCluster           = pVol->cbCluster;
    pChain->cClusterByteShift   = pVol->cClusterByteShift;
    pChain->cbChain             = 0;
    pChain->cClusters           = 0;
    RTListInit(&pChain->ListParts);
}


/**
 * Deletes a chain, freeing it's resources.
 *
 * @param   pChain              The chain.
 */
static void rtFsFatChain_Delete(PRTFSFATCHAIN pChain)
{
    Assert(RT_IS_POWER_OF_TWO(pChain->cbCluster));
    Assert(RT_BIT_32(pChain->cClusterByteShift) == pChain->cbCluster);

    PRTFSFATCHAINPART pPart = RTListRemoveLast(&pChain->ListParts, RTFSFATCHAINPART, ListEntry);
    while (pPart)
    {
        RTMemFree(pPart);
        pPart = RTListRemoveLast(&pChain->ListParts, RTFSFATCHAINPART, ListEntry);
    }

    pChain->cbChain   = 0;
    pChain->cClusters = 0;
}


/**
 * Appends a cluster to a cluster chain.
 *
 * @returns IPRT status code.
 * @param   pChain              The chain.
 * @param   idxCluster          The cluster to append.
 */
static int rtFsFatChain_Append(PRTFSFATCHAIN pChain, uint32_t idxCluster)
{
    PRTFSFATCHAINPART pPart;
    uint32_t idxLast = pChain->cClusters % RTFSFATCHAINPART_ENTRIES;
    if (idxLast != 0)
        pPart = RTListGetLast(&pChain->ListParts, RTFSFATCHAINPART, ListEntry);
    else
    {
        pPart = (PRTFSFATCHAINPART)RTMemAllocZ(sizeof(*pPart));
        if (!pPart)
            return VERR_NO_MEMORY;
        RTListAppend(&pChain->ListParts, &pPart->ListEntry);
    }
    pPart->aEntries[idxLast] = idxCluster;
    pChain->cClusters++;
    pChain->cbChain += pChain->cbCluster;
    return VINF_SUCCESS;
}


/**
 * Reduces the number of clusters in the chain to @a cClusters.
 *
 * @param   pChain          The chain.
 * @param   cClustersNew    The new cluster count.  Must be equal or smaller to
 *                          the current number of clusters.
 */
static void rtFsFatChain_Shrink(PRTFSFATCHAIN pChain, uint32_t cClustersNew)
{
    uint32_t cOldParts = (pChain->cClusters + RTFSFATCHAINPART_ENTRIES - 1) / RTFSFATCHAINPART_ENTRIES;
    uint32_t cNewParts = (cClustersNew      + RTFSFATCHAINPART_ENTRIES - 1) / RTFSFATCHAINPART_ENTRIES;
    Assert(cOldParts >= cNewParts);
    while (cOldParts-- > cNewParts)
        RTMemFree(RTListRemoveLast(&pChain->ListParts, RTFSFATCHAINPART, ListEntry));
    pChain->cClusters = cClustersNew;
    pChain->cbChain   = cClustersNew << pChain->cClusterByteShift;
    Assert(rtFsFatChain_AssertValid(pChain));
}



/**
 * Converts a file offset to a disk offset.
 *
 * The disk offset is only valid until the end of the cluster it is within.
 *
 * @returns Disk offset. UINT64_MAX if invalid file offset.
 * @param   pChain              The chain.
 * @param   offFile             The file offset.
 * @param   pVol                The volume.
 */
static uint64_t rtFsFatChain_FileOffsetToDiskOff(PCRTFSFATCHAIN pChain, uint32_t offFile, PCRTFSFATVOL pVol)
{
    uint32_t idxCluster = offFile >> pChain->cClusterByteShift;
    if (idxCluster < pChain->cClusters)
    {
        PRTFSFATCHAINPART pPart = RTListGetFirst(&pChain->ListParts, RTFSFATCHAINPART, ListEntry);
        while (idxCluster >= RTFSFATCHAINPART_ENTRIES)
        {
            idxCluster -= RTFSFATCHAINPART_ENTRIES;
            pPart = RTListGetNext(&pChain->ListParts, pPart, RTFSFATCHAINPART, ListEntry);
        }
        return pVol->offFirstCluster
             + ((uint64_t)(pPart->aEntries[idxCluster] - FAT_FIRST_DATA_CLUSTER) << pChain->cClusterByteShift)
             + (offFile & (pChain->cbCluster - 1));
    }
    return UINT64_MAX;
}


/**
 * Checks if the cluster chain is contiguous on the disk.
 *
 * @returns true / false.
 * @param   pChain              The chain.
 */
static bool rtFsFatChain_IsContiguous(PCRTFSFATCHAIN pChain)
{
    if (pChain->cClusters <= 1)
        return true;

    PRTFSFATCHAINPART   pPart   = RTListGetFirst(&pChain->ListParts, RTFSFATCHAINPART, ListEntry);
    uint32_t            idxNext = pPart->aEntries[0];
    uint32_t            cLeft   = pChain->cClusters;
    for (;;)
    {
        uint32_t const cInPart = RT_MIN(cLeft, RTFSFATCHAINPART_ENTRIES);
        for (uint32_t iPart = 0; iPart < cInPart; iPart++)
            if (pPart->aEntries[iPart] == idxNext)
                idxNext++;
            else
                return false;
        cLeft -= cInPart;
        if (!cLeft)
            return true;
        pPart = RTListGetNext(&pChain->ListParts, pPart, RTFSFATCHAINPART, ListEntry);
    }
}


/**
 * Gets a cluster array index.
 *
 * This works the chain thing as an indexed array.
 *
 * @returns The cluster number, UINT32_MAX if out of bounds.
 * @param   pChain              The chain.
 * @param   idx                 The index.
 */
static uint32_t rtFsFatChain_GetClusterByIndex(PCRTFSFATCHAIN pChain, uint32_t idx)
{
    if (idx < pChain->cClusters)
    {
        /*
         * In the first part?
         */
        PRTFSFATCHAINPART pPart;
        if (idx < RTFSFATCHAINPART_ENTRIES)
        {
            pPart = RTListGetFirst(&pChain->ListParts, RTFSFATCHAINPART, ListEntry);
            return pPart->aEntries[idx];
        }

        /*
         * In the last part?
         */
        uint32_t cParts    = (pChain->cClusters + RTFSFATCHAINPART_ENTRIES - 1) / RTFSFATCHAINPART_ENTRIES;
        uint32_t idxPart   = idx / RTFSFATCHAINPART_ENTRIES;
        uint32_t idxInPart = idx % RTFSFATCHAINPART_ENTRIES;
        if (idxPart + 1 == cParts)
            pPart = RTListGetLast(&pChain->ListParts, RTFSFATCHAINPART, ListEntry);
        else
        {
            /*
             * No, do linear search from the start, skipping the first part.
             */
            pPart = RTListGetFirst(&pChain->ListParts, RTFSFATCHAINPART, ListEntry);
            while (idxPart-- > 1)
                pPart = RTListGetNext(&pChain->ListParts, pPart, RTFSFATCHAINPART, ListEntry);
        }

        return pPart->aEntries[idxInPart];
    }
    return UINT32_MAX;
}


/**
 * Gets the first cluster.
 *
 * @returns The cluster number, UINT32_MAX if empty
 * @param   pChain              The chain.
 */
static uint32_t rtFsFatChain_GetFirstCluster(PCRTFSFATCHAIN pChain)
{
    if (pChain->cClusters > 0)
    {
        PRTFSFATCHAINPART pPart = RTListGetFirst(&pChain->ListParts, RTFSFATCHAINPART, ListEntry);
        return pPart->aEntries[0];
    }
    return UINT32_MAX;
}



/**
 * Gets the last cluster.
 *
 * @returns The cluster number, UINT32_MAX if empty
 * @param   pChain              The chain.
 */
static uint32_t rtFsFatChain_GetLastCluster(PCRTFSFATCHAIN pChain)
{
    if (pChain->cClusters > 0)
    {
        PRTFSFATCHAINPART pPart = RTListGetLast(&pChain->ListParts, RTFSFATCHAINPART, ListEntry);
        return pPart->aEntries[(pChain->cClusters - 1) % RTFSFATCHAINPART_ENTRIES];
    }
    return UINT32_MAX;
}


/**
 * Creates a cache for the file allocation table (cluster map).
 *
 * @returns Pointer to the cache.
 * @param   pThis               The FAT volume instance.
 * @param   pbFirst512FatBytes  The first 512 bytes of the first FAT.
 */
static int rtFsFatClusterMap_Create(PRTFSFATVOL pThis, uint8_t const *pbFirst512FatBytes, PRTERRINFO pErrInfo)
{
    Assert(RT_ALIGN_32(pThis->cbFat, pThis->cbSector) == pThis->cbFat);
    Assert(pThis->cbFat != 0);

    /*
     * Figure the cache size.  Keeping it _very_ simple for now as we just need
     * something that works, not anything the performs like crazy.
     */
    uint32_t cEntries;
    uint32_t cbEntry = pThis->cbFat;
    if (cbEntry <= _512K)
        cEntries = 1;
    else
    {
        Assert(pThis->cbSector < _512K / 8);
        cEntries = 8;
        cbEntry  = pThis->cbSector;
    }

    /*
     * Allocate and initialize it all.
     */
    PRTFSFATCLUSTERMAPCACHE pCache;
    pThis->pFatCache = pCache = (PRTFSFATCLUSTERMAPCACHE)RTMemAllocZ(RT_OFFSETOF(RTFSFATCLUSTERMAPCACHE, aEntries[cEntries]));
    if (!pCache)
        return RTErrInfoSet(pErrInfo, VERR_NO_MEMORY, "Failed to allocate FAT cache");
    pCache->cEntries  = cEntries;
    pCache->cbEntry   = cbEntry;

    unsigned i = cEntries;
    while (i-- > 0)
    {
        pCache->aEntries[i].pbData = (uint8_t *)RTMemAlloc(cbEntry);
        if (pCache->aEntries[i].pbData == NULL)
        {
            for (i++; i < cEntries; i++)
                RTMemFree(pCache->aEntries[i].pbData);
            RTMemFree(pCache);
            return RTErrInfoSetF(pErrInfo, VERR_NO_MEMORY, "Failed to allocate FAT cache entry (%#x bytes)", cbEntry);
        }

        pCache->aEntries[i].offFat  = UINT32_MAX;
        pCache->aEntries[i].bmDirty = 0;
    }

    /*
     * Calc the dirty shift factor.
     */
    cbEntry /= 64;
    if (cbEntry < pThis->cbSector)
        cbEntry = pThis->cbSector;

    pCache->cDirtyShift = 1;
    pCache->cbDirtyLine = 1;
    while (pCache->cbDirtyLine < cbEntry)
    {
        pCache->cDirtyShift++;
        pCache->cbDirtyLine <<= 1;
    }

    /*
     * Fill the cache if single entry or entry size is 512.
     */
    if (pCache->cEntries == 1 || pCache->cbEntry == 512)
    {
        memcpy(pCache->aEntries[0].pbData, pbFirst512FatBytes, RT_MIN(512, pCache->cbEntry));
        if (pCache->cbEntry > 512)
        {
            int rc = RTVfsFileReadAt(pThis->hVfsBacking, pThis->aoffFats[0] + 512,
                                     &pCache->aEntries[0].pbData[512], pCache->cbEntry - 512, NULL);
            if (RT_FAILURE(rc))
                return RTErrInfoSet(pErrInfo, rc, "Error reading FAT into memory");
        }
        pCache->aEntries[0].offFat  = 0;
        pCache->aEntries[0].bmDirty = 0;
    }

    return VINF_SUCCESS;
}


/**
 * Worker for rtFsFatClusterMap_Flush and rtFsFatClusterMap_FlushEntry.
 *
 * @returns IPRT status code.  On failure, we're currently kind of screwed.
 * @param   pThis       The FAT volume instance.
 * @param   iFirstEntry Entry to start flushing at.
 * @param   iLastEntry  Last entry to flush.
 */
static int rtFsFatClusterMap_FlushWorker(PRTFSFATVOL pThis, uint32_t const iFirstEntry, uint32_t const iLastEntry)
{
    PRTFSFATCLUSTERMAPCACHE pCache = pThis->pFatCache;


    /*
     * Walk the cache entries, accumulating segments to flush.
     */
    int      rc      = VINF_SUCCESS;
    uint64_t off     = UINT64_MAX;
    uint64_t offEdge = UINT64_MAX;
    RTSGSEG  aSgSegs[8];
    RTSGBUF  SgBuf;
    RTSgBufInit(&SgBuf, aSgSegs, RT_ELEMENTS(aSgSegs));
    SgBuf.cSegs = 0; /** @todo RTSgBuf API is stupid, make it smarter. */

    for (uint32_t iFatCopy = 0; iFatCopy < pThis->cFats; iFatCopy++)
    {
        for (uint32_t iEntry = iFirstEntry; iEntry <= iLastEntry; iEntry++)
        {
            uint64_t bmDirty = pCache->aEntries[iEntry].bmDirty;
            if (   bmDirty != 0
                && pCache->aEntries[iEntry].offFat != UINT32_MAX)
            {
                uint32_t offEntry   = 0;
                uint64_t iDirtyLine = 1;
                while (offEntry < pCache->cbEntry)
                {
                    if (pCache->aEntries[iEntry].bmDirty & iDirtyLine)
                    {
                        /*
                         * Found dirty cache line.
                         */
                        uint64_t offDirtyLine = pThis->aoffFats[iFatCopy] + pCache->aEntries[iEntry].offFat + offEntry;

                        /* Can we simply extend the last segment? */
                        if (   offDirtyLine == offEdge
                            && offEntry)
                        {
                            Assert(SgBuf.cSegs > 0);
                            Assert(   (uintptr_t)aSgSegs[SgBuf.cSegs - 1].pvSeg + aSgSegs[SgBuf.cSegs - 1].cbSeg
                                   == (uintptr_t)&pCache->aEntries[iEntry].pbData[offEntry]);
                            aSgSegs[SgBuf.cSegs - 1].cbSeg += pCache->cbDirtyLine;
                            offEdge += pCache->cbDirtyLine;
                        }
                        else
                        {
                            /* Starting new job? */
                            if (off == UINT64_MAX)
                            {
                                off = offDirtyLine;
                                Assert(SgBuf.cSegs == 0);
                            }
                            /* flush if not adjacent or if we're out of segments. */
                            else if (   offDirtyLine != offEdge
                                     || SgBuf.cSegs >= RT_ELEMENTS(aSgSegs))
                            {
                                int rc2 = RTVfsFileSgWrite(pThis->hVfsBacking, off, &SgBuf, true /*fBlocking*/, NULL);
                                if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
                                    rc = rc2;
                                RTSgBufReset(&SgBuf);
                                SgBuf.cSegs = 0;
                                off = offDirtyLine;
                            }

                            /* Append segment. */
                            aSgSegs[SgBuf.cSegs].cbSeg = pCache->cbDirtyLine;
                            aSgSegs[SgBuf.cSegs].pvSeg = &pCache->aEntries[iEntry].pbData[offEntry];
                            SgBuf.cSegs++;
                            offEdge = offDirtyLine + pCache->cbDirtyLine;
                        }

                        bmDirty &= ~iDirtyLine;
                        if (!bmDirty)
                            break;
                    }
                    iDirtyLine <<= 1;
                    offEntry += pCache->cbDirtyLine;
                }
                Assert(!bmDirty);
            }
        }
    }

    /*
     * Final flush job.
     */
    if (SgBuf.cSegs > 0)
    {
        int rc2 = RTVfsFileSgWrite(pThis->hVfsBacking, off, &SgBuf, true /*fBlocking*/, NULL);
        if (RT_FAILURE(rc2) && RT_SUCCESS(rc))
            rc = rc2;
    }

    /*
     * Clear the dirty flags on success.
     */
    if (RT_SUCCESS(rc))
        for (uint32_t iEntry = iFirstEntry; iEntry <= iLastEntry; iEntry++)
            pCache->aEntries[iEntry].bmDirty = 0;

    return rc;
}


/**
 * Flushes out all dirty lines in the entire file allocation table cache.
 *
 * @returns IPRT status code.  On failure, we're currently kind of screwed.
 * @param   pThis       The FAT volume instance.
 */
static int rtFsFatClusterMap_Flush(PRTFSFATVOL pThis)
{
    return rtFsFatClusterMap_FlushWorker(pThis, 0, pThis->pFatCache->cEntries - 1);
}


#if 0 /* unused */
/**
 * Flushes out all dirty lines in the file allocation table (cluster map) cache.
 *
 * This is typically called prior to reusing the cache entry.
 *
 * @returns IPRT status code.  On failure, we're currently kind of screwed.
 * @param   pThis       The FAT volume instance.
 * @param   iEntry      The cache entry to flush.
 */
static int rtFsFatClusterMap_FlushEntry(PRTFSFATVOL pThis, uint32_t iEntry)
{
    return rtFsFatClusterMap_FlushWorker(pThis, iEntry, iEntry);
}
#endif


/**
 * Destroys the file allcation table cache, first flushing any dirty lines.
 *
 * @returns IRPT status code from flush (we've destroyed it regardless of the
 *          status code).
 * @param   pThis       The FAT volume instance which cluster map shall be
 *                      destroyed.
 */
static int rtFsFatClusterMap_Destroy(PRTFSFATVOL pThis)
{
    int                     rc     = VINF_SUCCESS;
    PRTFSFATCLUSTERMAPCACHE pCache = pThis->pFatCache;
    if (pCache)
    {
        /* flush stuff. */
        rc = rtFsFatClusterMap_Flush(pThis);

        /* free everything. */
        uint32_t i = pCache->cEntries;
        while (i-- > 0)
        {
            RTMemFree(pCache->aEntries[i].pbData);
            pCache->aEntries[i].pbData = NULL;
        }
        pCache->cEntries = 0;
        RTMemFree(pCache);

        pThis->pFatCache = NULL;
    }

    return rc;
}


/**
 * Worker for rtFsFatClusterMap_ReadClusterChain handling FAT12.
 */
static int rtFsFatClusterMap_Fat12_ReadClusterChain(PRTFSFATCLUSTERMAPCACHE pFatCache, PRTFSFATVOL pVol, uint32_t idxCluster,
                                                    PRTFSFATCHAIN pChain)
{
    /* ASSUME that for FAT12 we cache the whole FAT in a single entry.  That
       way we don't need to deal with entries in different sectors and whatnot.  */
    AssertReturn(pFatCache->cEntries == 1, VERR_INTERNAL_ERROR_4);
    AssertReturn(pFatCache->cbEntry == pVol->cbFat, VERR_INTERNAL_ERROR_4);
    AssertReturn(pFatCache->aEntries[0].offFat == 0, VERR_INTERNAL_ERROR_4);

    /* Special case for empty files. */
    if (idxCluster == 0)
        return VINF_SUCCESS;

    /* Work cluster by cluster. */
    uint8_t const *pbFat = pFatCache->aEntries[0].pbData;
    for (;;)
    {
        /* Validate the cluster, checking for end of file. */
        if (   idxCluster >= pVol->cClusters
            || idxCluster <  FAT_FIRST_DATA_CLUSTER)
        {
            if (idxCluster >= FAT_FIRST_FAT12_EOC)
                return VINF_SUCCESS;
            return VERR_VFS_BOGUS_OFFSET;
        }

        /* Add cluster to chain.  */
        int rc = rtFsFatChain_Append(pChain, idxCluster);
        if (RT_FAILURE(rc))
            return rc;

        /* Next cluster. */
        bool     fOdd   = idxCluster & 1;
        uint32_t offFat = idxCluster * 3 / 2;
        idxCluster = RT_MAKE_U16(pbFat[offFat], pbFat[offFat + 1]);
        if (fOdd)
            idxCluster >>= 4;
        else
            idxCluster &= 0x0fff;
    }
}


/**
 * Worker for rtFsFatClusterMap_ReadClusterChain handling FAT16.
 */
static int rtFsFatClusterMap_Fat16_ReadClusterChain(PRTFSFATCLUSTERMAPCACHE pFatCache, PRTFSFATVOL pVol, uint32_t idxCluster,
                                                    PRTFSFATCHAIN pChain)
{
    RT_NOREF(pFatCache, pVol, idxCluster, pChain);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * Worker for rtFsFatClusterMap_ReadClusterChain handling FAT32.
 */
static int rtFsFatClusterMap_Fat32_ReadClusterChain(PRTFSFATCLUSTERMAPCACHE pFatCache, PRTFSFATVOL pVol, uint32_t idxCluster,
                                                    PRTFSFATCHAIN pChain)
{
    RT_NOREF(pFatCache, pVol, idxCluster, pChain);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * Reads a cluster chain into memory
 *
 * @returns IPRT status code.
 * @param   pThis           The FAT volume instance.
 * @param   idxFirstCluster The first cluster.
 * @param   pChain          The chain element to read into (and thereby
 *                          initialize).
 */
static int rtFsFatClusterMap_ReadClusterChain(PRTFSFATVOL pThis, uint32_t idxFirstCluster, PRTFSFATCHAIN pChain)
{
    pChain->cbCluster           = pThis->cbCluster;
    pChain->cClusterByteShift   = pThis->cClusterByteShift;
    pChain->cClusters           = 0;
    pChain->cbChain             = 0;
    RTListInit(&pChain->ListParts);
    switch (pThis->enmFatType)
    {
        case RTFSFATTYPE_FAT12: return rtFsFatClusterMap_Fat12_ReadClusterChain(pThis->pFatCache, pThis, idxFirstCluster, pChain);
        case RTFSFATTYPE_FAT16: return rtFsFatClusterMap_Fat16_ReadClusterChain(pThis->pFatCache, pThis, idxFirstCluster, pChain);
        case RTFSFATTYPE_FAT32: return rtFsFatClusterMap_Fat32_ReadClusterChain(pThis->pFatCache, pThis, idxFirstCluster, pChain);
        default:
            AssertFailedReturn(VERR_INTERNAL_ERROR_2);
    }
}


/**
 * Sets bmDirty for entry @a iEntry.
 *
 * @param   pFatCache   The FAT cache.
 * @param   iEntry      The cache entry.
 * @param   offEntry    The offset into the cache entry that was dirtied.
 */
DECLINLINE(void) rtFsFatClusterMap_SetDirtyByte(PRTFSFATCLUSTERMAPCACHE pFatCache, uint32_t iEntry, uint32_t offEntry)
{
    uint8_t iLine = offEntry / pFatCache->cbDirtyLine;
    pFatCache->aEntries[iEntry].bmDirty |= RT_BIT_64(iLine);
}


/** Sets a FAT12 cluster value. */
static int rtFsFatClusterMap_SetCluster12(PRTFSFATCLUSTERMAPCACHE pFatCache, PRTFSFATVOL pVol,
                                          uint32_t idxCluster, uint32_t uValue)
{
    /* ASSUME that for FAT12 we cache the whole FAT in a single entry.  That
       way we don't need to deal with entries in different sectors and whatnot.  */
    AssertReturn(pFatCache->cEntries == 1, VERR_INTERNAL_ERROR_4);
    AssertReturn(pFatCache->cbEntry == pVol->cbFat, VERR_INTERNAL_ERROR_4);
    AssertReturn(pFatCache->aEntries[0].offFat == 0, VERR_INTERNAL_ERROR_4);
    AssertReturn(uValue < 0x1000, VERR_INTERNAL_ERROR_2);

    /* Make the change. */
    uint8_t *pbFat  = pFatCache->aEntries[0].pbData;
    uint32_t offFat = idxCluster * 3 / 2;
    if (idxCluster & 1)
    {
        pbFat[offFat]     = ((uint8_t)0x0f & pbFat[offFat]) | ((uint8_t)uValue << 4);
        pbFat[offFat + 1] = (uint8_t)(uValue >> 4);
    }
    else
    {
        pbFat[offFat]     = (uint8_t)uValue;
        pbFat[offFat + 1] = ((uint8_t)0xf0 & pbFat[offFat + 1]) | (uint8_t)(uValue >> 8);
    }

    /* Update the dirty bits. */
    rtFsFatClusterMap_SetDirtyByte(pFatCache, 0, offFat);
    rtFsFatClusterMap_SetDirtyByte(pFatCache, 0, offFat + 1);

    return VINF_SUCCESS;
}


/** Sets a FAT16 cluster value. */
static int rtFsFatClusterMap_SetCluster16(PRTFSFATCLUSTERMAPCACHE pFatCache, PRTFSFATVOL pVol,
                                          uint32_t idxCluster, uint32_t uValue)
{
    AssertReturn(uValue < 0x10000, VERR_INTERNAL_ERROR_2);
    RT_NOREF(pFatCache, pVol, idxCluster, uValue);
    return VERR_NOT_IMPLEMENTED;
}


/** Sets a FAT32 cluster value. */
static int rtFsFatClusterMap_SetCluster32(PRTFSFATCLUSTERMAPCACHE pFatCache, PRTFSFATVOL pVol,
                                          uint32_t idxCluster, uint32_t uValue)
{
    AssertReturn(uValue < 0x10000000, VERR_INTERNAL_ERROR_2);
    RT_NOREF(pFatCache, pVol, idxCluster, uValue);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * Marks the cluster @a idxCluster as the end of the cluster chain.
 *
 * @returns IPRT status code
 * @param   pThis           The FAT volume instance.
 * @param   idxCluster      The cluster to end the chain with.
 */
static int rtFsFatClusterMap_SetEndOfChain(PRTFSFATVOL pThis, uint32_t idxCluster)
{
    AssertReturn(idxCluster >= FAT_FIRST_DATA_CLUSTER, VERR_VFS_BOGUS_OFFSET);
    AssertMsgReturn(idxCluster < pThis->cClusters, ("idxCluster=%#x cClusters=%#x\n", idxCluster, pThis->cClusters),
                    VERR_VFS_BOGUS_OFFSET);
    switch (pThis->enmFatType)
    {
        case RTFSFATTYPE_FAT12: return rtFsFatClusterMap_SetCluster12(pThis->pFatCache, pThis, idxCluster, FAT_FIRST_FAT12_EOC);
        case RTFSFATTYPE_FAT16: return rtFsFatClusterMap_SetCluster16(pThis->pFatCache, pThis, idxCluster, FAT_FIRST_FAT16_EOC);
        case RTFSFATTYPE_FAT32: return rtFsFatClusterMap_SetCluster32(pThis->pFatCache, pThis, idxCluster, FAT_FIRST_FAT32_EOC);
        default: AssertFailedReturn(VERR_INTERNAL_ERROR_3);
    }
}


/**
 * Marks the cluster @a idxCluster as free.
 * @returns IPRT status code
 * @param   pThis           The FAT volume instance.
 * @param   idxCluster      The cluster to free.
 */
static int rtFsFatClusterMap_FreeCluster(PRTFSFATVOL pThis, uint32_t idxCluster)
{
    AssertReturn(idxCluster >= FAT_FIRST_DATA_CLUSTER, VERR_VFS_BOGUS_OFFSET);
    AssertReturn(idxCluster < pThis->cClusters, VERR_VFS_BOGUS_OFFSET);
    switch (pThis->enmFatType)
    {
        case RTFSFATTYPE_FAT12: return rtFsFatClusterMap_SetCluster12(pThis->pFatCache, pThis, idxCluster, 0);
        case RTFSFATTYPE_FAT16: return rtFsFatClusterMap_SetCluster16(pThis->pFatCache, pThis, idxCluster, 0);
        case RTFSFATTYPE_FAT32: return rtFsFatClusterMap_SetCluster32(pThis->pFatCache, pThis, idxCluster, 0);
        default: AssertFailedReturn(VERR_INTERNAL_ERROR_3);
    }
}


/**
 * Worker for rtFsFatClusterMap_AllocateCluster that handles FAT12.
 */
static int rtFsFatClusterMap_AllocateCluster12(PRTFSFATCLUSTERMAPCACHE pFatCache, PRTFSFATVOL pVol,
                                               uint32_t idxPrevCluster, uint32_t *pidxCluster)
{
    /* ASSUME that for FAT12 we cache the whole FAT in a single entry.  That
       way we don't need to deal with entries in different sectors and whatnot.  */
    AssertReturn(pFatCache->cEntries == 1, VERR_INTERNAL_ERROR_4);
    AssertReturn(pFatCache->cbEntry == pVol->cbFat, VERR_INTERNAL_ERROR_4);
    AssertReturn(pFatCache->aEntries[0].offFat == 0, VERR_INTERNAL_ERROR_4);

    /*
     * Check that the previous cluster is a valid chain end.
     */
    uint8_t *pbFat      = pFatCache->aEntries[0].pbData;
    uint32_t offFatPrev;
    if (idxPrevCluster != UINT32_MAX)
    {
        offFatPrev = idxPrevCluster * 3 / 2;
        uint32_t idxPrevValue;
        if (idxPrevCluster & 1)
            idxPrevValue = (pbFat[offFatPrev] >> 4) | ((uint32_t)pbFat[offFatPrev + 1] << 4);
        else
            idxPrevValue = pbFat[offFatPrev] | ((uint32_t)(pbFat[offFatPrev + 1] & 0x0f) << 8);
        AssertReturn(idxPrevValue >= FAT_FIRST_FAT12_EOC, VERR_VFS_BOGUS_OFFSET);
    }
    else
        offFatPrev = UINT32_MAX;

    /*
     * Search cluster by cluster from the start (it's small, so easy trumps
     * complicated optimizations).
     */
    uint32_t idxCluster = FAT_FIRST_DATA_CLUSTER;
    uint32_t offFat     = 3;
    while (idxCluster < pVol->cClusters)
    {
        if (idxCluster & 1)
        {
            if (   (pbFat[offFat] & 0xf0) != 0
                || pbFat[offFat + 1] != 0)
            {
                offFat += 2;
                idxCluster++;
                continue;
            }

            /* Set EOC. */
            pbFat[offFat]     |= 0xf0;
            pbFat[offFat + 1]  = 0xff;
        }
        else
        {
            if (   pbFat[offFat]
                || pbFat[offFat + 1] & 0x0f)
            {
                offFat += 1;
                idxCluster++;
                continue;
            }

            /* Set EOC. */
            pbFat[offFat]      = 0xff;
            pbFat[offFat + 1] |= 0x0f;
        }

        /* Update the dirty bits. */
        rtFsFatClusterMap_SetDirtyByte(pFatCache, 0, offFat);
        rtFsFatClusterMap_SetDirtyByte(pFatCache, 0, offFat + 1);

        /* Chain it on the previous cluster. */
        if (idxPrevCluster != UINT32_MAX)
        {
            if (idxPrevCluster & 1)
            {
                pbFat[offFatPrev]     = (pbFat[offFatPrev] & (uint8_t)0x0f) | (uint8_t)(idxCluster << 4);
                pbFat[offFatPrev + 1] = (uint8_t)(idxCluster >> 4);
            }
            else
            {
                pbFat[offFatPrev]     = (uint8_t)idxCluster;
                pbFat[offFatPrev + 1] = (pbFat[offFatPrev + 1] & (uint8_t)0xf0) | ((uint8_t)(idxCluster >> 8) & (uint8_t)0x0f);
            }
            rtFsFatClusterMap_SetDirtyByte(pFatCache, 0, offFatPrev);
            rtFsFatClusterMap_SetDirtyByte(pFatCache, 0, offFatPrev + 1);
        }

        *pidxCluster = idxCluster;
        return VINF_SUCCESS;
    }

    return VERR_DISK_FULL;
}


/**
 * Worker for rtFsFatClusterMap_AllocateCluster that handles FAT16.
 */
static int rtFsFatClusterMap_AllocateCluster16(PRTFSFATCLUSTERMAPCACHE pFatCache, PRTFSFATVOL pVol,
                                               uint32_t idxPrevCluster, uint32_t *pidxCluster)
{
    RT_NOREF(pFatCache, pVol, idxPrevCluster, pidxCluster);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * Worker for rtFsFatClusterMap_AllocateCluster that handles FAT32.
 */
static int rtFsFatClusterMap_AllocateCluster32(PRTFSFATCLUSTERMAPCACHE pFatCache, PRTFSFATVOL pVol,
                                               uint32_t idxPrevCluster, uint32_t *pidxCluster)
{
    RT_NOREF(pFatCache, pVol, idxPrevCluster, pidxCluster);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * Allocates a cluster an appends it to the chain given by @a idxPrevCluster.
 *
 * @returns IPRT status code.
 * @retval  VERR_DISK_FULL if no more available clusters.
 * @param   pThis           The FAT volume instance.
 * @param   idxPrevCluster  The previous cluster, UINT32_MAX if first.
 * @param   pidxCluster     Where to return the cluster number on success.
 */
static int rtFsFatClusterMap_AllocateCluster(PRTFSFATVOL pThis, uint32_t idxPrevCluster, uint32_t *pidxCluster)
{
    AssertReturn(idxPrevCluster == UINT32_MAX || (idxPrevCluster >= FAT_FIRST_DATA_CLUSTER && idxPrevCluster < pThis->cClusters),
                 VERR_INTERNAL_ERROR_5);
    *pidxCluster = UINT32_MAX;
    switch (pThis->enmFatType)
    {
        case RTFSFATTYPE_FAT12: return rtFsFatClusterMap_AllocateCluster12(pThis->pFatCache, pThis, idxPrevCluster, pidxCluster);
        case RTFSFATTYPE_FAT16: return rtFsFatClusterMap_AllocateCluster16(pThis->pFatCache, pThis, idxPrevCluster, pidxCluster);
        case RTFSFATTYPE_FAT32: return rtFsFatClusterMap_AllocateCluster32(pThis->pFatCache, pThis, idxPrevCluster, pidxCluster);
        default: AssertFailedReturn(VERR_INTERNAL_ERROR_3);
    }
}


/**
 * Allocates clusters.
 *
 * Will free the clusters if it fails to allocate all of them.
 *
 * @returns IPRT status code.
 * @param   pThis           The FAT volume instance.
 * @param   pChain          The chain.
 * @param   cClusters       Number of clusters to add to the chain.
 */
static int rtFsFatClusterMap_AllocateMoreClusters(PRTFSFATVOL pThis, PRTFSFATCHAIN pChain, uint32_t cClusters)
{
    int             rc                  = VINF_SUCCESS;
    uint32_t const  cOldClustersInChain = pChain->cClusters;
    uint32_t const  idxOldLastCluster   = rtFsFatChain_GetLastCluster(pChain);
    uint32_t        idxPrevCluster      = idxOldLastCluster;
    uint32_t        iCluster            = 0;
    while (iCluster < cClusters)
    {
        uint32_t idxCluster;
        rc = rtFsFatClusterMap_AllocateCluster(pThis, idxPrevCluster, &idxCluster);
        if (RT_SUCCESS(rc))
        {
            rc = rtFsFatChain_Append(pChain, idxCluster);
            if (RT_SUCCESS(rc))
            {
                /* next */
                iCluster++;
                continue;
            }

            /* Bail out, freeing any clusters we've managed to allocate by now. */
            rtFsFatClusterMap_FreeCluster(pThis, idxCluster);
        }
        if (idxOldLastCluster != UINT32_MAX)
            rtFsFatClusterMap_SetEndOfChain(pThis, idxOldLastCluster);
        while (iCluster-- > 0)
            rtFsFatClusterMap_FreeCluster(pThis, rtFsFatChain_GetClusterByIndex(pChain, cOldClustersInChain + iCluster));
        rtFsFatChain_Shrink(pChain, iCluster);
        break;
    }
    return rc;
}



/**
 * Converts a FAT timestamp into an IPRT timesspec.
 *
 * @param   pTimeSpec       Where to return the IRPT time.
 * @param   uDate           The date part of the FAT timestamp.
 * @param   uTime           The time part of the FAT timestamp.
 * @param   cCentiseconds   Centiseconds part if applicable (0 otherwise).
 * @param   pVol            The volume.
 */
static void rtFsFatDateTime2TimeSpec(PRTTIMESPEC pTimeSpec, uint16_t uDate, uint16_t uTime,
                                     uint8_t cCentiseconds, PCRTFSFATVOL pVol)
{
    RTTIME Time;
    Time.fFlags         = RTTIME_FLAGS_TYPE_UTC;
    Time.offUTC         = 0;
    Time.i32Year        = 1980 + (uDate >> 9);
    Time.u8Month        = RT_MAX((uDate >> 5) & 0xf, 1);
    Time.u8MonthDay     = RT_MAX(uDate & 0x1f, 1);
    Time.u8WeekDay      = UINT8_MAX;
    Time.u16YearDay     = 0;
    Time.u8Hour         = uTime >> 11;
    Time.u8Minute       = (uTime >> 5) & 0x3f;
    Time.u8Second       = (uTime & 0x1f) << 1;
    Time.u32Nanosecond  = 0;
    if (cCentiseconds > 0 && cCentiseconds < 200) /* screw complicated stuff for now. */
    {
        if (cCentiseconds >= 100)
        {
            cCentiseconds -= 100;
            Time.u8Second++;
        }
        Time.u32Nanosecond = cCentiseconds * UINT64_C(100000000);
    }

    RTTimeImplode(pTimeSpec, RTTimeNormalize(&Time));
    RTTimeSpecSubNano(pTimeSpec, pVol->offNanoUTC);
}


/**
 * Converts an IPRT timespec to a FAT timestamp.
 *
 * @returns The centiseconds part.
 * @param   pVol        The volume.
 * @param   pTimeSpec   The IPRT timespec to convert (UTC).
 * @param   puDate      Where to return the date part of the FAT timestamp.
 * @param   puTime      Where to return the time part of the FAT timestamp.
 */
static uint8_t rtFsFatTimeSpec2FatDateTime(PCRTFSFATVOL pVol, PCRTTIMESPEC pTimeSpec, uint16_t *puDate, uint16_t *puTime)
{
    RTTIMESPEC  TimeSpec = *pTimeSpec;
    RTTIME      Time;
    RTTimeExplode(&Time, RTTimeSpecSubNano(&TimeSpec, pVol->offNanoUTC));

    if (puDate)
        *puDate = ((uint16_t)(RT_MAX(Time.i32Year, 1980) - 1980) << 9)
                | (Time.u8Month << 5)
                | Time.u8MonthDay;
    if (puTime)
        *puTime = ((uint16_t)Time.u8Hour   << 11)
                | (Time.u8Minute << 5)
                | (Time.u8Second >> 1);
    return (Time.u8Second & 1) * 100 + Time.u32Nanosecond / 10000000;

}


/**
 * Gets the current FAT timestamp.
 *
 * @returns The centiseconds part.
 * @param   pVol     The volume.
 * @param   puDate   Where to return the date part of the FAT timestamp.
 * @param   puTime   Where to return the time part of the FAT timestamp.
 */
static uint8_t rtFsFatCurrentFatDateTime(PCRTFSFATVOL pVol, uint16_t *puDate, uint16_t *puTime)
{
    RTTIMESPEC TimeSpec;
    return rtFsFatTimeSpec2FatDateTime(pVol, RTTimeNow(&TimeSpec), puDate, puTime);
}


/**
 * Initialization of a RTFSFATOBJ structure from a FAT directory entry.
 *
 * @note    The RTFSFATOBJ::pParentDir and RTFSFATOBJ::Clusters members are
 *          properly initialized elsewhere.
 *
 * @param   pObj            The structure to initialize.
 * @param   pDirEntry       The directory entry.
 * @param   offEntryInDir   The offset in the parent directory.
 * @param   pVol            The volume.
 */
static void rtFsFatObj_InitFromDirEntry(PRTFSFATOBJ pObj, PCFATDIRENTRY pDirEntry, uint32_t offEntryInDir, PRTFSFATVOL pVol)
{
    RTListInit(&pObj->Entry);
    pObj->pParentDir        = NULL;
    pObj->pVol              = pVol;
    pObj->offEntryInDir     = offEntryInDir;
    pObj->fAttrib           = ((RTFMODE)pDirEntry->fAttrib << RTFS_DOS_SHIFT) & RTFS_DOS_MASK_OS2;
    pObj->fAttrib           = rtFsModeFromDos(pObj->fAttrib, (char *)&pDirEntry->achName[0], sizeof(pDirEntry->achName), 0);
    pObj->cbObject          = pDirEntry->cbFile;
    pObj->fMaybeDirtyFat    = false;
    pObj->fMaybeDirtyDirEnt = false;
    rtFsFatDateTime2TimeSpec(&pObj->ModificationTime, pDirEntry->uModifyDate, pDirEntry->uModifyTime, 0, pVol);
    rtFsFatDateTime2TimeSpec(&pObj->BirthTime, pDirEntry->uBirthDate, pDirEntry->uBirthTime, pDirEntry->uBirthCentiseconds, pVol);
    rtFsFatDateTime2TimeSpec(&pObj->AccessTime, pDirEntry->uAccessDate, 0, 0, pVol);
}


/**
 * Dummy initialization of a RTFSFATOBJ structure.
 *
 * @note    The RTFSFATOBJ::pParentDir and RTFSFATOBJ::Clusters members are
 *          properly initialized elsewhere.
 *
 * @param   pObj            The structure to initialize.
 * @param   cbObject        The object size.
 * @param   fAttrib         The attributes.
 * @param   pVol            The volume.
 */
static void rtFsFatObj_InitDummy(PRTFSFATOBJ pObj, uint32_t cbObject, RTFMODE fAttrib, PRTFSFATVOL pVol)
{
    RTListInit(&pObj->Entry);
    pObj->pParentDir        = NULL;
    pObj->pVol              = pVol;
    pObj->offEntryInDir     = UINT32_MAX;
    pObj->fAttrib           = fAttrib;
    pObj->cbObject          = cbObject;
    pObj->fMaybeDirtyFat    = false;
    pObj->fMaybeDirtyDirEnt = false;
    RTTimeSpecSetDosSeconds(&pObj->AccessTime, 0);
    RTTimeSpecSetDosSeconds(&pObj->ModificationTime, 0);
    RTTimeSpecSetDosSeconds(&pObj->BirthTime, 0);
}


/**
 * Flushes FAT object meta data.
 *
 * @returns IPRT status code
 * @param   pObj            The common object structure.
 */
static int rtFsFatObj_FlushMetaData(PRTFSFATOBJ pObj)
{
    int rc = VINF_SUCCESS;
    if (pObj->fMaybeDirtyFat)
    {
        rc = rtFsFatClusterMap_Flush(pObj->pVol);
        if (RT_SUCCESS(rc))
            pObj->fMaybeDirtyFat = false;
    }
    if (pObj->fMaybeDirtyDirEnt)
    {
        int rc2 = rtFsFatDir_Flush(pObj->pParentDir);
        if (RT_SUCCESS(rc2))
            pObj->fMaybeDirtyDirEnt = false;
        else if (RT_SUCCESS(rc))
            rc = rc2;
    }
    return rc;
}


/**
 * Worker for rtFsFatFile_Close and rtFsFatDir_Close that does common work.
 *
 * @returns IPRT status code.
 * @param   pObj            The common object structure.
 */
static int rtFsFatObj_Close(PRTFSFATOBJ pObj)
{
    int rc = rtFsFatObj_FlushMetaData(pObj);
    if (pObj->pParentDir)
        rtFsFatDir_RemoveOpenChild(pObj->pParentDir, pObj);
    rtFsFatChain_Delete(&pObj->Clusters);
    return rc;
}


/**
 * @interface_method_impl{RTVFSOBJOPS,pfnClose}
 */
static DECLCALLBACK(int) rtFsFatFile_Close(void *pvThis)
{
    PRTFSFATFILE pThis = (PRTFSFATFILE)pvThis;
    return rtFsFatObj_Close(&pThis->Core);
}


/**
 * @interface_method_impl{RTVFSOBJOPS,pfnQueryInfo}
 */
static DECLCALLBACK(int) rtFsFatObj_QueryInfo(void *pvThis, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    PRTFSFATOBJ pThis = (PRTFSFATOBJ)pvThis;

    pObjInfo->cbObject              = pThis->cbObject;
    pObjInfo->cbAllocated           = pThis->Clusters.cbChain;
    pObjInfo->AccessTime            = pThis->AccessTime;
    pObjInfo->ModificationTime      = pThis->ModificationTime;
    pObjInfo->ChangeTime            = pThis->ModificationTime;
    pObjInfo->BirthTime             = pThis->BirthTime;
    pObjInfo->Attr.fMode            = pThis->fAttrib;
    pObjInfo->Attr.enmAdditional    = enmAddAttr;

    switch (enmAddAttr)
    {
        case RTFSOBJATTRADD_NOTHING: RT_FALL_THRU();
        case RTFSOBJATTRADD_UNIX:
            pObjInfo->Attr.u.Unix.uid           = NIL_RTUID;
            pObjInfo->Attr.u.Unix.gid           = NIL_RTGID;
            pObjInfo->Attr.u.Unix.cHardlinks    = 1;
            pObjInfo->Attr.u.Unix.INodeIdDevice = 0;
            pObjInfo->Attr.u.Unix.INodeId       = 0; /* Could probably use the directory entry offset. */
            pObjInfo->Attr.u.Unix.fFlags        = 0;
            pObjInfo->Attr.u.Unix.GenerationId  = 0;
            pObjInfo->Attr.u.Unix.Device        = 0;
            break;
        case RTFSOBJATTRADD_UNIX_OWNER:
            pObjInfo->Attr.u.UnixOwner.uid       = 0;
            pObjInfo->Attr.u.UnixOwner.szName[0] = '\0';
            break;
        case RTFSOBJATTRADD_UNIX_GROUP:
            pObjInfo->Attr.u.UnixGroup.gid       = 0;
            pObjInfo->Attr.u.UnixGroup.szName[0] = '\0';
            break;
        case RTFSOBJATTRADD_EASIZE:
            pObjInfo->Attr.u.EASize.cb = 0;
            break;
        default:
            return VERR_INVALID_PARAMETER;
    }
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnRead}
 */
static DECLCALLBACK(int) rtFsFatFile_Read(void *pvThis, RTFOFF off, PCRTSGBUF pSgBuf, bool fBlocking, size_t *pcbRead)
{
    PRTFSFATFILE pThis = (PRTFSFATFILE)pvThis;
    AssertReturn(pSgBuf->cSegs != 0, VERR_INTERNAL_ERROR_3);
    RT_NOREF(fBlocking);

    /*
     * Check for EOF.
     */
    if (off == -1)
        off = pThis->offFile;
    if ((uint64_t)off >= pThis->Core.cbObject)
    {
        if (pcbRead)
        {
            *pcbRead = 0;
            return VINF_EOF;
        }
        return VERR_EOF;
    }

    /*
     * Do the reading cluster by cluster.
     */
    int      rc         = VINF_SUCCESS;
    uint32_t cbFileLeft = pThis->Core.cbObject - (uint32_t)off;
    uint32_t cbRead     = 0;
    size_t   cbLeft     = pSgBuf->paSegs[0].cbSeg;
    uint8_t *pbDst      = (uint8_t *)pSgBuf->paSegs[0].pvSeg;
    while (cbLeft > 0)
    {
        if (cbFileLeft > 0)
        {
            uint64_t offDisk = rtFsFatChain_FileOffsetToDiskOff(&pThis->Core.Clusters, (uint32_t)off, pThis->Core.pVol);
            if (offDisk != UINT64_MAX)
            {
                uint32_t cbToRead = pThis->Core.Clusters.cbCluster - ((uint32_t)off & (pThis->Core.Clusters.cbCluster - 1));
                if (cbToRead > cbLeft)
                    cbToRead = (uint32_t)cbLeft;
                if (cbToRead > cbFileLeft)
                    cbToRead = cbFileLeft;
                rc = RTVfsFileReadAt(pThis->Core.pVol->hVfsBacking, offDisk, pbDst, cbToRead, NULL);
                if (RT_SUCCESS(rc))
                {
                    off         += cbToRead;
                    pbDst       += cbToRead;
                    cbRead      += cbToRead;
                    cbFileLeft  -= cbToRead;
                    cbLeft      -= cbToRead;
                    continue;
                }
            }
            else
                rc = VERR_VFS_BOGUS_OFFSET;
        }
        else
            rc = pcbRead ? VINF_EOF : VERR_EOF;
        break;
    }

    /* Update the offset and return. */
    pThis->offFile = off;
    if (pcbRead)
        *pcbRead = cbRead;
    return rc;
}


/**
 * Changes the size of a file or directory FAT object.
 *
 * @returns IPRT status code
 * @param   pObj        The common object.
 * @param   cbFile      The new file size.
 */
static int rtFsFatObj_SetSize(PRTFSFATOBJ pObj, uint32_t cbFile)
{
    AssertReturn(   ((pObj->cbObject + pObj->Clusters.cbCluster - 1) >> pObj->Clusters.cClusterByteShift)
                 == pObj->Clusters.cClusters, VERR_INTERNAL_ERROR_3);

    /*
     * Do nothing if the size didn't change.
     */
    if (pObj->cbObject == cbFile)
        return VINF_SUCCESS;

    /*
     * Do we need to allocate or free clusters?
     */
    int rc = VINF_SUCCESS;
    uint32_t const cClustersNew = (cbFile + pObj->Clusters.cbCluster - 1) >> pObj->Clusters.cClusterByteShift;
    AssertReturn(pObj->pParentDir, VERR_INTERNAL_ERROR_2);
    if (pObj->Clusters.cClusters == cClustersNew)
    { /* likely when writing small bits at a time. */ }
    else if (pObj->Clusters.cClusters < cClustersNew)
    {
        /* Allocate and append new clusters. */
        do
        {
            uint32_t idxCluster;
            rc = rtFsFatClusterMap_AllocateCluster(pObj->pVol, rtFsFatChain_GetLastCluster(&pObj->Clusters), &idxCluster);
            if (RT_SUCCESS(rc))
                rc = rtFsFatChain_Append(&pObj->Clusters, idxCluster);
        } while (pObj->Clusters.cClusters < cClustersNew && RT_SUCCESS(rc));
        pObj->fMaybeDirtyFat = true;
    }
    else
    {
        /* Free clusters we don't need any more. */
        if (cClustersNew > 0)
            rc = rtFsFatClusterMap_SetEndOfChain(pObj->pVol, rtFsFatChain_GetClusterByIndex(&pObj->Clusters, cClustersNew - 1));
        if (RT_SUCCESS(rc))
        {
            uint32_t iClusterToFree = cClustersNew;
            while (iClusterToFree < pObj->Clusters.cClusters && RT_SUCCESS(rc))
            {
                rc = rtFsFatClusterMap_FreeCluster(pObj->pVol, rtFsFatChain_GetClusterByIndex(&pObj->Clusters, iClusterToFree));
                iClusterToFree++;
            }

            rtFsFatChain_Shrink(&pObj->Clusters, cClustersNew);
        }
        pObj->fMaybeDirtyFat = true;
    }
    if (RT_SUCCESS(rc))
    {
        /*
         * Update the object size, since we've got the right number of clusters backing it now.
         */
        pObj->cbObject = cbFile;

        /*
         * Update the directory entry.
         */
        uint32_t     uWriteLock;
        PFATDIRENTRY pDirEntry;
        rc = rtFsFatDir_GetEntryForUpdate(pObj->pParentDir, pObj->offEntryInDir, &pDirEntry, &uWriteLock);
        if (RT_SUCCESS(rc))
        {
            pDirEntry->cbFile = cbFile;
            uint32_t idxFirstCluster;
            if (cClustersNew == 0)
                idxFirstCluster = 0;  /** @todo figure out if setting the cluster to 0 is the right way to deal with empty files... */
            else
                idxFirstCluster = rtFsFatChain_GetFirstCluster(&pObj->Clusters);
            pDirEntry->idxCluster = (uint16_t)idxFirstCluster;
            if (pObj->pVol->enmFatType >= RTFSFATTYPE_FAT32)
                pDirEntry->u.idxClusterHigh = (uint16_t)(idxFirstCluster >> 16);

            rc = rtFsFatDir_PutEntryAfterUpdate(pObj->pParentDir, pDirEntry, uWriteLock);
            pObj->fMaybeDirtyDirEnt = true;
        }
    }
    return rc;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnWrite}
 */
static DECLCALLBACK(int) rtFsFatFile_Write(void *pvThis, RTFOFF off, PCRTSGBUF pSgBuf, bool fBlocking, size_t *pcbWritten)
{
    PRTFSFATFILE pThis = (PRTFSFATFILE)pvThis;
    PRTFSFATVOL  pVol  = pThis->Core.pVol;
    AssertReturn(pSgBuf->cSegs != 0, VERR_INTERNAL_ERROR_3);
    RT_NOREF(fBlocking);

    if (pVol->fReadOnly)
        return VERR_WRITE_PROTECT;

    if (off == -1)
        off = pThis->offFile;

    /*
     * Do the reading cluster by cluster.
     */
    int            rc         = VINF_SUCCESS;
    uint32_t       cbWritten  = 0;
    size_t         cbLeft     = pSgBuf->paSegs[0].cbSeg;
    uint8_t const *pbSrc      = (uint8_t const *)pSgBuf->paSegs[0].pvSeg;
    while (cbLeft > 0)
    {
        /* Figure out how much we can write.  Checking for max file size and such. */
        uint32_t cbToWrite = pThis->Core.Clusters.cbCluster - ((uint32_t)off & (pThis->Core.Clusters.cbCluster - 1));
        if (cbToWrite > cbLeft)
            cbToWrite = (uint32_t)cbLeft;
        uint64_t offNew = (uint64_t)off + cbToWrite;
        if (offNew < _4G)
        { /*likely*/ }
        else if ((uint64_t)off < _4G - 1U)
            cbToWrite = _4G - 1U - off;
        else
        {
            rc = VERR_FILE_TOO_BIG;
            break;
        }

        /* Grow the file? */
        if ((uint32_t)offNew > pThis->Core.cbObject)
        {
            rc = rtFsFatObj_SetSize(&pThis->Core, (uint32_t)offNew);
            if (RT_SUCCESS(rc))
            { /* likely */}
            else
                break;
        }

        /* Figure the disk offset. */
        uint64_t offDisk = rtFsFatChain_FileOffsetToDiskOff(&pThis->Core.Clusters, (uint32_t)off, pVol);
        if (offDisk != UINT64_MAX)
        {
            rc = RTVfsFileWriteAt(pVol->hVfsBacking, offDisk, pbSrc, cbToWrite, NULL);
            if (RT_SUCCESS(rc))
            {
                off         += cbToWrite;
                pbSrc       += cbToWrite;
                cbWritten   += cbToWrite;
                cbLeft      -= cbToWrite;
            }
            else
                break;
        }
        else
        {
            rc = VERR_VFS_BOGUS_OFFSET;
            break;
        }
    }

    /* Update the offset and return. */
    pThis->offFile = off;
    if (pcbWritten)
        *pcbWritten = cbWritten;
    return rc;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnFlush}
 */
static DECLCALLBACK(int) rtFsFatFile_Flush(void *pvThis)
{
    PRTFSFATFILE pThis = (PRTFSFATFILE)pvThis;
    int rc1 = rtFsFatObj_FlushMetaData(&pThis->Core);
    int rc2 = RTVfsFileFlush(pThis->Core.pVol->hVfsBacking);
    return RT_FAILURE(rc1) ? rc1 : rc2;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnPollOne}
 */
static DECLCALLBACK(int) rtFsFatFile_PollOne(void *pvThis, uint32_t fEvents, RTMSINTERVAL cMillies, bool fIntr,
                                             uint32_t *pfRetEvents)
{
    NOREF(pvThis);
    int rc;
    if (fEvents != RTPOLL_EVT_ERROR)
    {
        *pfRetEvents = fEvents & ~RTPOLL_EVT_ERROR;
        rc = VINF_SUCCESS;
    }
    else if (fIntr)
        rc = RTThreadSleep(cMillies);
    else
    {
        uint64_t uMsStart = RTTimeMilliTS();
        do
            rc = RTThreadSleep(cMillies);
        while (   rc == VERR_INTERRUPTED
               && !fIntr
               && RTTimeMilliTS() - uMsStart < cMillies);
        if (rc == VERR_INTERRUPTED)
            rc = VERR_TIMEOUT;
    }
    return rc;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnTell}
 */
static DECLCALLBACK(int) rtFsFatFile_Tell(void *pvThis, PRTFOFF poffActual)
{
    PRTFSFATFILE pThis = (PRTFSFATFILE)pvThis;
    *poffActual = pThis->offFile;
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnMode}
 */
static DECLCALLBACK(int) rtFsFatObj_SetMode(void *pvThis, RTFMODE fMode, RTFMODE fMask)
{
#if 0
    PRTFSFATFILE pThis = (PRTFSFATFILE)pvThis;
    if (fMask != ~RTFS_TYPE_MASK)
    {
        fMode |= ~fMask & ObjInfo.Attr.fMode;
    }
#else
    RT_NOREF(pvThis, fMode, fMask);
    return VERR_NOT_IMPLEMENTED;
#endif
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnSetTimes}
 */
static DECLCALLBACK(int) rtFsFatObj_SetTimes(void *pvThis, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
                                             PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime)
{
#if 0
    PRTFSFATFILE pThis = (PRTFSFATFILE)pvThis;
#else
    RT_NOREF(pvThis, pAccessTime, pModificationTime, pChangeTime, pBirthTime);
    return VERR_NOT_IMPLEMENTED;
#endif
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnSetOwner}
 */
static DECLCALLBACK(int) rtFsFatObj_SetOwner(void *pvThis, RTUID uid, RTGID gid)
{
    RT_NOREF(pvThis, uid, gid);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSFILEOPS,pfnSeek}
 */
static DECLCALLBACK(int) rtFsFatFile_Seek(void *pvThis, RTFOFF offSeek, unsigned uMethod, PRTFOFF poffActual)
{
    PRTFSFATFILE pThis = (PRTFSFATFILE)pvThis;
    RTFOFF offNew;
    switch (uMethod)
    {
        case RTFILE_SEEK_BEGIN:
            offNew = offSeek;
            break;
        case RTFILE_SEEK_END:
            offNew = (RTFOFF)pThis->Core.cbObject + offSeek;
            break;
        case RTFILE_SEEK_CURRENT:
            offNew = (RTFOFF)pThis->offFile + offSeek;
            break;
        default:
            return VERR_INVALID_PARAMETER;
    }
    if (offNew >= 0)
    {
        if (offNew <= _4G)
        {
            pThis->offFile = offNew;
            *poffActual    = offNew;
            return VINF_SUCCESS;
        }
        return VERR_OUT_OF_RANGE;
    }
    return VERR_NEGATIVE_SEEK;
}


/**
 * @interface_method_impl{RTVFSFILEOPS,pfnQuerySize}
 */
static DECLCALLBACK(int) rtFsFatFile_QuerySize(void *pvThis, uint64_t *pcbFile)
{
    PRTFSFATFILE pThis = (PRTFSFATFILE)pvThis;
    *pcbFile = pThis->Core.cbObject;
    return VINF_SUCCESS;
}


/**
 * FAT file operations.
 */
DECL_HIDDEN_CONST(const RTVFSFILEOPS) g_rtFsFatFileOps =
{
    { /* Stream */
        { /* Obj */
            RTVFSOBJOPS_VERSION,
            RTVFSOBJTYPE_FILE,
            "FatFile",
            rtFsFatFile_Close,
            rtFsFatObj_QueryInfo,
            RTVFSOBJOPS_VERSION
        },
        RTVFSIOSTREAMOPS_VERSION,
        RTVFSIOSTREAMOPS_FEAT_NO_SG,
        rtFsFatFile_Read,
        rtFsFatFile_Write,
        rtFsFatFile_Flush,
        rtFsFatFile_PollOne,
        rtFsFatFile_Tell,
        NULL /*pfnSkip*/,
        NULL /*pfnZeroFill*/,
        RTVFSIOSTREAMOPS_VERSION,
    },
    RTVFSFILEOPS_VERSION,
    0,
    { /* ObjSet */
        RTVFSOBJSETOPS_VERSION,
        RT_OFFSETOF(RTVFSFILEOPS, Stream.Obj) - RT_OFFSETOF(RTVFSFILEOPS, ObjSet),
        rtFsFatObj_SetMode,
        rtFsFatObj_SetTimes,
        rtFsFatObj_SetOwner,
        RTVFSOBJSETOPS_VERSION
    },
    rtFsFatFile_Seek,
    rtFsFatFile_QuerySize,
    RTVFSFILEOPS_VERSION
};


/**
 * Instantiates a new directory.
 *
 * @returns IPRT status code.
 * @param   pThis           The FAT volume instance.
 * @param   pParentDir      The parent directory.
 * @param   pDirEntry       The parent directory entry.
 * @param   offEntryInDir   The byte offset of the directory entry in the parent
 *                          directory.
 * @param   fOpen           RTFILE_O_XXX flags.
 * @param   phVfsFile       Where to return the file handle.
 */
static int rtFsFatFile_New(PRTFSFATVOL pThis, PRTFSFATDIR pParentDir, PCFATDIRENTRY pDirEntry, uint32_t offEntryInDir,
                           uint64_t fOpen, PRTVFSFILE phVfsFile)
{
    AssertPtr(pParentDir);
    Assert(!(offEntryInDir & (sizeof(FATDIRENTRY) - 1)));

    PRTFSFATFILE pNewFile;
    int rc = RTVfsNewFile(&g_rtFsFatFileOps, sizeof(*pNewFile), fOpen, pThis->hVfsSelf, NIL_RTVFSLOCK /*use volume lock*/,
                          phVfsFile, (void **)&pNewFile);
    if (RT_SUCCESS(rc))
    {
        /*
         * Initialize it all so rtFsFatFile_Close doesn't trip up in anyway.
         */
        rtFsFatObj_InitFromDirEntry(&pNewFile->Core, pDirEntry, offEntryInDir, pThis);
        pNewFile->offFile = 0;
        rc = rtFsFatClusterMap_ReadClusterChain(pThis, RTFSFAT_GET_CLUSTER(pDirEntry, pThis), &pNewFile->Core.Clusters);
        if (RT_SUCCESS(rc))
        {
            /*
             * Link into parent directory so we can use it to update
             * our directory entry.
             */
            rtFsFatDir_AddOpenChild(pParentDir, &pNewFile->Core);

            /*
             * Should we truncate the file or anything of that sort?
             */
            if (   (fOpen & RTFILE_O_TRUNCATE)
                || (fOpen & RTFILE_O_ACTION_MASK) == RTFILE_O_CREATE_REPLACE)
                rc = rtFsFatObj_SetSize(&pNewFile->Core, 0);
            if (RT_SUCCESS(rc))
                return VINF_SUCCESS;
        }

        RTVfsFileRelease(*phVfsFile);
    }
    *phVfsFile = NIL_RTVFSFILE;
    return rc;
}




/**
 * Flush directory changes when having a fully buffered directory.
 *
 * @returns IPRT status code
 * @param   pThis           The directory.
 */
static int rtFsFatDir_FlushFullyBuffered(PRTFSFATDIR pThis)
{
    Assert(pThis->fFullyBuffered);
    uint32_t const  cbSector    = pThis->Core.pVol->cbSector;
    RTVFSFILE const hVfsBacking = pThis->Core.pVol->hVfsBacking;
    int             rc          = VINF_SUCCESS;
    for (uint32_t i = 0; i < pThis->u.Full.cSectors; i++)
        if (pThis->u.Full.pbDirtySectors[i])
        {
            int rc2 = RTVfsFileWriteAt(hVfsBacking, pThis->offEntriesOnDisk + i * cbSector,
                                       (uint8_t *)pThis->paEntries + i * cbSector, cbSector, NULL);
            if (RT_SUCCESS(rc2))
                pThis->u.Full.pbDirtySectors[i] = false;
            else if (RT_SUCCESS(rc))
                rc = rc2;
        }
    return rc;
}


/**
 * Flush directory changes when using simple buffering.
 *
 * @returns IPRT status code
 * @param   pThis           The directory.
 */
static int rtFsFatDir_FlushSimple(PRTFSFATDIR pThis)
{
    Assert(!pThis->fFullyBuffered);
    int rc;
    if (   !pThis->u.Simple.fDirty
        || pThis->offEntriesOnDisk != UINT64_MAX)
        rc = VINF_SUCCESS;
    else
    {
        Assert(pThis->u.Simple.offInDir != UINT32_MAX);
        rc = RTVfsFileWriteAt(pThis->Core.pVol->hVfsBacking,  pThis->offEntriesOnDisk,
                              pThis->paEntries, pThis->Core.pVol->cbSector, NULL);
        if (RT_SUCCESS(rc))
            pThis->u.Simple.fDirty = false;
    }
    return rc;
}


/**
 * Flush directory changes.
 *
 * @returns IPRT status code
 * @param   pThis           The directory.
 */
static int rtFsFatDir_Flush(PRTFSFATDIR pThis)
{
    if (pThis->fFullyBuffered)
        return rtFsFatDir_FlushFullyBuffered(pThis);
    return rtFsFatDir_FlushSimple(pThis);
}


/**
 * Gets one or more entires at @a offEntryInDir.
 *
 * Common worker for rtFsFatDir_GetEntriesAt and rtFsFatDir_GetEntryForUpdate
 *
 * @returns IPRT status code.
 * @param   pThis               The directory.
 * @param   offEntryInDir       The directory offset in bytes.
 * @param   fForUpdate          Whether it's for updating.
 * @param   ppaEntries          Where to return pointer to the entry at
 *                              @a offEntryInDir.
 * @param   pcEntries           Where to return the number of entries
 *                              @a *ppaEntries points to.
 * @param   puBufferReadLock    Where to return the buffer read lock handle.
 *                              Call rtFsFatDir_ReleaseBufferAfterReading when
 *                              done.
 */
static int rtFsFatDir_GetEntriesAtCommon(PRTFSFATDIR pThis, uint32_t offEntryInDir, bool fForUpdate,
                                         PFATDIRENTRYUNION *ppaEntries, uint32_t *pcEntries, uint32_t *puLock)
{
    *puLock = UINT32_MAX;

    int rc;
    Assert(RT_ALIGN_32(offEntryInDir, sizeof(FATDIRENTRY)) == offEntryInDir);
    Assert(pThis->Core.cbObject / sizeof(FATDIRENTRY) == pThis->cEntries);
    uint32_t const idxEntryInDir = offEntryInDir / sizeof(FATDIRENTRY);
    if (idxEntryInDir < pThis->cEntries)
    {
        if (pThis->fFullyBuffered)
        {
            /*
             * Fully buffered: Return pointer to all the entires starting at offEntryInDir.
             */
            *ppaEntries = &pThis->paEntries[idxEntryInDir];
            *pcEntries  = pThis->cEntries - idxEntryInDir;
            *puLock     = !fForUpdate ? 1 : UINT32_C(0x80000001);
            rc = VINF_SUCCESS;
        }
        else
        {
            /*
             * Simple buffering: If hit, return the number of entries.
             */
            PRTFSFATVOL pVol = pThis->Core.pVol;
            uint32_t    off  = offEntryInDir - pThis->u.Simple.offInDir;
            if (off < pVol->cbSector)
            {
                *ppaEntries = &pThis->paEntries[off / sizeof(FATDIRENTRY)];
                *pcEntries  = (pVol->cbSector - off) / sizeof(FATDIRENTRY);
                *puLock     = !fForUpdate ? 1 : UINT32_C(0x80000001);
                rc = VINF_SUCCESS;
            }
            else
            {
                /*
                 * Simple buffering: Miss.
                 * Flush dirty. Read in new sector. Return entries in sector starting
                 * at offEntryInDir.
                 */
                if (!pThis->u.Simple.fDirty)
                    rc = VINF_SUCCESS;
                else
                    rc = rtFsFatDir_FlushSimple(pThis);
                if (RT_SUCCESS(rc))
                {
                    off                      =  offEntryInDir &  (pVol->cbSector - 1);
                    pThis->u.Simple.offInDir = (offEntryInDir & ~(pVol->cbSector - 1));
                    pThis->offEntriesOnDisk  = rtFsFatChain_FileOffsetToDiskOff(&pThis->Core.Clusters, pThis->u.Simple.offInDir,
                                                                                pThis->Core.pVol);
                    rc = RTVfsFileReadAt(pThis->Core.pVol->hVfsBacking, pThis->offEntriesOnDisk,
                                         pThis->paEntries, pVol->cbSector, NULL);
                    if (RT_SUCCESS(rc))
                    {
                        *ppaEntries = &pThis->paEntries[off / sizeof(FATDIRENTRY)];
                        *pcEntries  = (pVol->cbSector - off) / sizeof(FATDIRENTRY);
                        *puLock     = !fForUpdate ? 1 : UINT32_C(0x80000001);
                        rc = VINF_SUCCESS;
                    }
                    else
                    {
                        pThis->u.Simple.offInDir = UINT32_MAX;
                        pThis->offEntriesOnDisk  = UINT64_MAX;
                    }
                }
            }
        }
    }
    else
        rc = VERR_FILE_NOT_FOUND;
    return rc;
}


/**
 * Puts back a directory entry after updating it, releasing the write lock and
 * marking it dirty.
 *
 * @returns IPRT status code
 * @param   pThis           The directory.
 * @param   pDirEntry       The directory entry.
 * @param   uWriteLock      The write lock.
 */
static int rtFsFatDir_PutEntryAfterUpdate(PRTFSFATDIR pThis, PFATDIRENTRY pDirEntry, uint32_t uWriteLock)
{
    Assert(uWriteLock == UINT32_C(0x80000001));
    RT_NOREF(uWriteLock);
    if (pThis->fFullyBuffered)
    {
        uint32_t idxSector = ((uintptr_t)pDirEntry - (uintptr_t)pThis->paEntries) / pThis->Core.pVol->cbSector;
        pThis->u.Full.pbDirtySectors[idxSector] = true;
    }
    else
        pThis->u.Simple.fDirty = true;
    return VINF_SUCCESS;
}


/**
 * Gets the pointer to the given directory entry for the purpose of updating it.
 *
 * Call rtFsFatDir_PutEntryAfterUpdate afterwards.
 *
 * @returns IPRT status code.
 * @param   pThis           The directory.
 * @param   offEntryInDir   The byte offset of the directory entry, within the
 *                          directory.
 * @param   ppDirEntry      Where to return the pointer to the directory entry.
 * @param   puWriteLock     Where to return the write lock.
 */
static int rtFsFatDir_GetEntryForUpdate(PRTFSFATDIR pThis, uint32_t offEntryInDir, PFATDIRENTRY *ppDirEntry,
                                        uint32_t *puWriteLock)
{
    uint32_t cEntriesIgn;
    return rtFsFatDir_GetEntriesAtCommon(pThis, offEntryInDir, true /*fForUpdate*/, (PFATDIRENTRYUNION *)ppDirEntry,
                                         &cEntriesIgn, puWriteLock);
}


/**
 * Release a directory buffer after done reading from it.
 *
 * This is currently just a placeholder.
 *
 * @param   pThis           The directory.
 * @param   uBufferReadLock The buffer lock.
 */
static void rtFsFatDir_ReleaseBufferAfterReading(PRTFSFATDIR pThis, uint32_t uBufferReadLock)
{
    RT_NOREF(pThis, uBufferReadLock);
    Assert(uBufferReadLock == 1);
}


/**
 * Gets one or more entires at @a offEntryInDir.
 *
 * @returns IPRT status code.
 * @param   pThis               The directory.
 * @param   offEntryInDir       The directory offset in bytes.
 * @param   ppaEntries          Where to return pointer to the entry at
 *                              @a offEntryInDir.
 * @param   pcEntries           Where to return the number of entries
 *                              @a *ppaEntries points to.
 * @param   puBufferReadLock    Where to return the buffer read lock handle.
 *                              Call rtFsFatDir_ReleaseBufferAfterReading when
 *                              done.
 */
static int rtFsFatDir_GetEntriesAt(PRTFSFATDIR pThis, uint32_t offEntryInDir,
                                   PCFATDIRENTRYUNION *ppaEntries, uint32_t *pcEntries, uint32_t *puBufferReadLock)
{
    return rtFsFatDir_GetEntriesAtCommon(pThis, offEntryInDir, false /*fForUpdate*/, (PFATDIRENTRYUNION *)ppaEntries,
                                         pcEntries, puBufferReadLock);
}


/**
 * Translates a unicode codepoint to an uppercased CP437 index.
 *
 * @returns CP437 index if valie, UINT16_MAX if not.
 * @param   uc          The codepoint to convert.
 */
static uint16_t rtFsFatUnicodeCodepointToUpperCodepage(RTUNICP uc)
{
    /*
     * The first 128 chars have 1:1 translation for valid FAT chars.
     */
    if (uc < 128)
    {
        if (g_awchFatCp437Chars[uc] == uc)
            return (uint16_t)uc;
        if (RT_C_IS_LOWER(uc))
            return uc - 0x20;
        return UINT16_MAX;
    }

    /*
     * Try for uppercased, settle for lower case if no upper case variant in the table.
     * This is really expensive, btw.
     */
    RTUNICP ucUpper = RTUniCpToUpper(uc);
    for (unsigned i = 128; i < 256; i++)
        if (g_awchFatCp437Chars[i] == ucUpper)
            return i;
    if (ucUpper != uc)
        for (unsigned i = 128; i < 256; i++)
            if (g_awchFatCp437Chars[i] == uc)
                return i;
    return UINT16_MAX;
}


/**
 * Convert filename string to 8-dot-3 format, doing necessary ASCII uppercasing
 * and such.
 *
 * @returns true if 8.3 formattable name, false if not.
 * @param   pszName8Dot3    Where to return the 8-dot-3 name when returning
 *                          @c true.  Filled with zero on false.  8+3+1 bytes.
 * @param   pszName         The filename to convert.
 */
static bool rtFsFatDir_StringTo8Dot3(char *pszName8Dot3, const char *pszName)
{
    /*
     * Don't try convert names with more than 12 unicode chars in them.
     */
    size_t const cucName = RTStrUniLen(pszName);
    if (cucName <= 12 && cucName > 0)
    {
        /*
         * Recode the input string as CP437, uppercasing it, validating the
         * name, formatting it as a FAT directory entry string.
         */
        size_t offDst  = 0;
        bool   fExt    = false;
        for (;;)
        {
            RTUNICP uc;
            int rc = RTStrGetCpEx(&pszName, &uc);
            if (RT_SUCCESS(rc))
            {
                if (uc)
                {
                    if (offDst < 8+3)
                    {
                        uint16_t idxCp = rtFsFatUnicodeCodepointToUpperCodepage(uc);
                        if (idxCp != UINT16_MAX)
                        {
                            pszName8Dot3[offDst++] = (char)idxCp;
                            Assert(uc != '.');
                            continue;
                        }

                        /* Maybe the dot? */
                        if (   uc == '.'
                            && !fExt
                            && offDst <= 8)
                        {
                            fExt = true;
                            while (offDst < 8)
                                pszName8Dot3[offDst++] = ' ';
                            continue;
                        }
                    }
                }
                /* String terminator: Check length, pad and convert 0xe5. */
                else if (offDst <= (size_t)(fExt ? 8 + 3 : 8))
                {
                    while (offDst < 8 + 3)
                        pszName8Dot3[offDst++] = ' ';
                    Assert(offDst == 8 + 3);
                    pszName8Dot3[offDst] = '\0';

                    if ((uint8_t)pszName8Dot3[0] == FATDIRENTRY_CH0_DELETED)
                        pszName8Dot3[0] = FATDIRENTRY_CH0_ESC_E5;
                    return true;
                }
            }
            /* invalid */
            break;
        }
    }
    memset(&pszName8Dot3[0], 0, 8+3+1);
    return false;
}


/**
 * Calculates the checksum of a directory entry.
 * @returns Checksum.
 * @param   pDirEntry           The directory entry to checksum.
 */
static uint8_t rtFsFatDir_CalcChecksum(PCFATDIRENTRY pDirEntry)
{
    uint8_t bChecksum = pDirEntry->achName[0];
    for (uint8_t off = 1; off < RT_ELEMENTS(pDirEntry->achName); off++)
    {
        bChecksum = RTFSFAT_ROT_R1_U8(bChecksum);
        bChecksum += pDirEntry->achName[off];
    }
    return bChecksum;
}


/**
 * Locates a directory entry in a directory.
 *
 * @returns IPRT status code.
 * @retval  VERR_FILE_NOT_FOUND if not found.
 * @param   pThis           The directory to search.
 * @param   pszEntry        The entry to look for.
 * @param   poffEntryInDir  Where to return the offset of the directory
 *                          entry.
 * @param   pfLong          Where to return long name indicator.
 * @param   pDirEntry       Where to return a copy of the directory entry.
 */
static int rtFsFatDir_FindEntry(PRTFSFATDIR pThis, const char *pszEntry, uint32_t *poffEntryInDir, bool *pfLong,
                                PFATDIRENTRY pDirEntry)
{
    /* Set return values. */
    *pfLong         = false;
    *poffEntryInDir = UINT32_MAX;

    /*
     * Turn pszEntry into a 8.3 filename, if possible.
     */
    char    szName8Dot3[8+3+1];
    bool    fIs8Dot3Name = rtFsFatDir_StringTo8Dot3(szName8Dot3, pszEntry);

    /*
     * Scan the directory buffer by buffer.
     */
    RTUTF16             wszName[260+1];
    uint8_t             bChecksum       = UINT8_MAX;
    uint8_t             idNextSlot      = UINT8_MAX;
    size_t              cwcName         = 0;
    uint32_t            offEntryInDir   = 0;
    uint32_t const      cbDir           = pThis->Core.cbObject;
    Assert(RT_ALIGN_32(cbDir, sizeof(*pDirEntry)) == cbDir);
    AssertCompile(FATDIRNAMESLOT_MAX_SLOTS * FATDIRNAMESLOT_CHARS_PER_SLOT < RT_ELEMENTS(wszName));
    wszName[260] = '\0';

    while (offEntryInDir < cbDir)
    {
        /* Get chunk of entries starting at offEntryInDir. */
        uint32_t            uBufferLock = UINT32_MAX;
        uint32_t            cEntries    = 0;
        PCFATDIRENTRYUNION  paEntries   = NULL;
        int rc = rtFsFatDir_GetEntriesAt(pThis, offEntryInDir, &paEntries, &cEntries, &uBufferLock);
        if (RT_FAILURE(rc))
            return rc;

        /*
         * Now work thru each of the entries.
         */
        for (uint32_t iEntry = 0; iEntry < cEntries; iEntry++, offEntryInDir += sizeof(FATDIRENTRY))
        {
            switch ((uint8_t)paEntries[iEntry].Entry.achName[0])
            {
                default:
                    break;
                case FATDIRENTRY_CH0_DELETED:
                    cwcName = 0;
                    continue;
                case FATDIRENTRY_CH0_END_OF_DIR:
                    if (pThis->Core.pVol->enmBpbVersion >= RTFSFATBPBVER_DOS_2_0)
                    {
                        rtFsFatDir_ReleaseBufferAfterReading(pThis, uBufferLock);
                        return VERR_FILE_NOT_FOUND;
                    }
                    cwcName = 0;
                    break; /* Technically a valid entry before DOS 2.0, or so some claim. */
            }

            /*
             * Check for long filename slot.
             */
            if (   paEntries[iEntry].Slot.fAttrib == FAT_ATTR_NAME_SLOT
                && paEntries[iEntry].Slot.idxZero == 0
                && paEntries[iEntry].Slot.fZero   == 0
                && (paEntries[iEntry].Slot.idSlot & ~FATDIRNAMESLOT_FIRST_SLOT_FLAG) <= FATDIRNAMESLOT_HIGHEST_SLOT_ID
                && (paEntries[iEntry].Slot.idSlot & ~FATDIRNAMESLOT_FIRST_SLOT_FLAG) != 0)
            {
                /* New slot? */
                if (paEntries[iEntry].Slot.idSlot & FATDIRNAMESLOT_FIRST_SLOT_FLAG)
                {
                    idNextSlot = paEntries[iEntry].Slot.idSlot & ~FATDIRNAMESLOT_FIRST_SLOT_FLAG;
                    bChecksum  = paEntries[iEntry].Slot.bChecksum;
                    cwcName    = idNextSlot * FATDIRNAMESLOT_CHARS_PER_SLOT;
                    wszName[cwcName] = '\0';
                }
                /* Is valid next entry? */
                else if (   paEntries[iEntry].Slot.idSlot    == idNextSlot
                         && paEntries[iEntry].Slot.bChecksum == bChecksum)
                { /* likely */ }
                else
                    cwcName = 0;
                if (cwcName)
                {
                    idNextSlot--;
                    size_t offName = idNextSlot * FATDIRNAMESLOT_CHARS_PER_SLOT;
                    memcpy(&wszName[offName],         paEntries[iEntry].Slot.awcName0, sizeof(paEntries[iEntry].Slot.awcName0));
                    memcpy(&wszName[offName + 5],     paEntries[iEntry].Slot.awcName1, sizeof(paEntries[iEntry].Slot.awcName1));
                    memcpy(&wszName[offName + 5 + 6], paEntries[iEntry].Slot.awcName2, sizeof(paEntries[iEntry].Slot.awcName2));
                }
            }
            /*
             * Regular directory entry. Do the matching, first 8.3 then long name.
             */
            else if (   fIs8Dot3Name
                     && memcmp(paEntries[iEntry].Entry.achName, szName8Dot3, sizeof(paEntries[iEntry].Entry.achName)) == 0)
            {
                *poffEntryInDir = offEntryInDir;
                *pDirEntry      = paEntries[iEntry].Entry;
                *pfLong         = false;
                rtFsFatDir_ReleaseBufferAfterReading(pThis, uBufferLock);
                return VINF_SUCCESS;
            }
            else if (   cwcName != 0
                     && idNextSlot == 0
                     && rtFsFatDir_CalcChecksum(&paEntries[iEntry].Entry) == bChecksum
                     && RTUtf16ICmpUtf8(wszName, pszEntry) == 0)
            {
                *poffEntryInDir = offEntryInDir;
                *pDirEntry      = paEntries[iEntry].Entry;
                *pfLong         = true;
                rtFsFatDir_ReleaseBufferAfterReading(pThis, uBufferLock);
                return VINF_SUCCESS;
            }
            else
                cwcName = 0;
        }

        rtFsFatDir_ReleaseBufferAfterReading(pThis, uBufferLock);
    }

    return VERR_FILE_NOT_FOUND;
}


/**
 * Watered down version of rtFsFatDir_FindEntry that is used by the short name
 * generator to check for duplicates.
 *
 * @returns IPRT status code.
 * @retval  VERR_FILE_NOT_FOUND if not found.
 * @retval  VINF_SUCCESS if found.
 * @param   pThis           The directory to search.
 * @param   pszEntry        The entry to look for.
 */
static int rtFsFatDir_FindEntryShort(PRTFSFATDIR pThis, const char *pszName8Dot3)
{
    Assert(strlen(pszName8Dot3) == 8+3);

    /*
     * Scan the directory buffer by buffer.
     */
    uint32_t            offEntryInDir   = 0;
    uint32_t const      cbDir           = pThis->Core.cbObject;
    Assert(RT_ALIGN_32(cbDir, sizeof(FATDIRENTRY)) == cbDir);

    while (offEntryInDir < cbDir)
    {
        /* Get chunk of entries starting at offEntryInDir. */
        uint32_t            uBufferLock = UINT32_MAX;
        uint32_t            cEntries    = 0;
        PCFATDIRENTRYUNION  paEntries   = NULL;
        int rc = rtFsFatDir_GetEntriesAt(pThis, offEntryInDir, &paEntries, &cEntries, &uBufferLock);
        if (RT_FAILURE(rc))
            return rc;

        /*
         * Now work thru each of the entries.
         */
        for (uint32_t iEntry = 0; iEntry < cEntries; iEntry++, offEntryInDir += sizeof(FATDIRENTRY))
        {
            switch ((uint8_t)paEntries[iEntry].Entry.achName[0])
            {
                default:
                    break;
                case FATDIRENTRY_CH0_DELETED:
                    continue;
                case FATDIRENTRY_CH0_END_OF_DIR:
                    if (pThis->Core.pVol->enmBpbVersion >= RTFSFATBPBVER_DOS_2_0)
                    {
                        rtFsFatDir_ReleaseBufferAfterReading(pThis, uBufferLock);
                        return VERR_FILE_NOT_FOUND;
                    }
                    break; /* Technically a valid entry before DOS 2.0, or so some claim. */
            }

            /*
             * Skip long filename slots.
             */
            if (   paEntries[iEntry].Slot.fAttrib == FAT_ATTR_NAME_SLOT
                && paEntries[iEntry].Slot.idxZero == 0
                && paEntries[iEntry].Slot.fZero   == 0
                && (paEntries[iEntry].Slot.idSlot & ~FATDIRNAMESLOT_FIRST_SLOT_FLAG) <= FATDIRNAMESLOT_HIGHEST_SLOT_ID
                && (paEntries[iEntry].Slot.idSlot & ~FATDIRNAMESLOT_FIRST_SLOT_FLAG) != 0)
            { /* skipped */ }
            /*
             * Regular directory entry. Do the matching, first 8.3 then long name.
             */
            else if (memcmp(paEntries[iEntry].Entry.achName, pszName8Dot3, sizeof(paEntries[iEntry].Entry.achName)) == 0)
            {
                rtFsFatDir_ReleaseBufferAfterReading(pThis, uBufferLock);
                return VINF_SUCCESS;
            }
        }

        rtFsFatDir_ReleaseBufferAfterReading(pThis, uBufferLock);
    }

    return VERR_FILE_NOT_FOUND;
}


/**
 * Calculates the FATDIRENTRY::fCase flags for the given name.
 *
 * ASSUMES that the name is a 8.3 name.
 *
 * @returns Case flag mask.
 * @param   pszName             The name.
 */
static uint8_t rtFsFatDir_CalcCaseFlags(const char *pszName)
{
    uint8_t bRet      = FATDIRENTRY_CASE_F_LOWER_BASE | FATDIRENTRY_CASE_F_LOWER_EXT;
    uint8_t bCurrent  = FATDIRENTRY_CASE_F_LOWER_BASE;
    for (;;)
    {
        RTUNICP uc;
        int rc = RTStrGetCpEx(&pszName, &uc);
        if (RT_SUCCESS(rc))
        {
            if (uc != 0)
            {
                if (uc != '.')
                {
                    if (RTUniCpIsUpper(uc))
                    {
                        bRet &= ~bCurrent;
                        if (!bRet)
                            return 0;
                    }
                }
                else
                    bCurrent = FATDIRENTRY_CASE_F_LOWER_EXT;
            }
            else if (bCurrent == FATDIRENTRY_CASE_F_LOWER_BASE)
                return bRet & ~FATDIRENTRY_CASE_F_LOWER_EXT;
            else
                return bRet;
        }
        else
            return 0;
    }
}


/**
 * Checks if we need to generate a long name for @a pszEntry.
 *
 * @returns true if we need to, false if we don't.
 * @param   pszEntry        The UTF-8 directory entry entry name.
 * @param   fIs8Dot3Name    Whether we've managed to create a 8-dot-3 name.
 * @param   pDirEntry       The directory entry with the 8-dot-3 name when
 *                          fIs8Dot3Name is set.
 */
static bool rtFsFatDir_NeedLongName(const char *pszEntry, bool fIs8Dot3Name, PCFATDIRENTRY pDirEntry)
{
    /*
     * Check the easy ways out first.
     */

    /* If we couldn't make a straight 8-dot-3 name out of it, the we
       must do the long name thing.  No question. */
    if (!fIs8Dot3Name)
        return true;

    /* If both lower case flags are set, then the whole name must be
       lowercased, so we won't need a long entry. */
    if (pDirEntry->fCase == (FATDIRENTRY_CASE_F_LOWER_BASE | FATDIRENTRY_CASE_F_LOWER_EXT))
        return false;

    /*
     * Okay, check out the whole string then, part by part.  (This is code
     * similar to rtFsFatDir_CalcCaseFlags.)
     */
    uint8_t fCurrent = pDirEntry->fCase & FATDIRENTRY_CASE_F_LOWER_BASE;
    for (;;)
    {
        RTUNICP uc;
        int rc = RTStrGetCpEx(&pszEntry, &uc);
        if (RT_SUCCESS(rc))
        {
            if (uc != 0)
            {
                if (uc != '.')
                {
                    if (   fCurrent
                        || !RTUniCpIsLower(uc))
                    { /* okay */ }
                    else
                        return true;
                }
                else
                    fCurrent = pDirEntry->fCase & FATDIRENTRY_CASE_F_LOWER_EXT;
            }
            /* It checked out to the end, so we don't need a long name. */
            else
                return false;
        }
        else
            return true;
    }
}


/**
 * Checks if the given long name is valid for a long file name or not.
 *
 * Encoding, length and character set limitations are checked.
 *
 * @returns IRPT status code.
 * @param   pwszEntry       The long filename.
 * @param   cwc             The length of the filename in UTF-16 chars.
 */
static int rtFsFatDir_ValidateLongName(PCRTUTF16 pwszEntry, size_t cwc)
{
    /* Length limitation. */
    if (cwc <= RTFSFAT_MAX_LFN_CHARS)
    {
        /* Character set limitations. */
        for (size_t off = 0; off < cwc; off++)
        {
            RTUTF16 wc = pwszEntry[off];
            if (wc < 128)
            {
                if (g_awchFatCp437Chars[wc] <= UINT16_C(0xfffe))
                { /* likely */ }
                else
                    return VERR_INVALID_NAME;
            }
        }

        /* Name limitations. */
        if (   cwc == 1
            && pwszEntry[0] == '.')
            return VERR_INVALID_NAME;
        if (   cwc == 2
            && pwszEntry[0] == '.'
            && pwszEntry[1] == '.')
            return VERR_INVALID_NAME;

        /** @todo Check for more invalid names, also in the 8.3 case! */
        return VINF_SUCCESS;
    }
    return VERR_FILENAME_TOO_LONG;
}


/**
 * Worker for rtFsFatDir_GenerateShortName.
 */
static void rtFsFatDir_CopyShortName(char *pszDst, uint32_t cchDst, const char *pszSrc, size_t cchSrc, char chPad)
{
    /* Copy from source. */
    if (cchSrc > 0)
    {
        const char *pszSrcEnd = &pszSrc[cchSrc];
        while (cchDst > 0 && pszSrc != pszSrcEnd)
        {
            RTUNICP uc;
            int rc = RTStrGetCpEx(&pszSrc, &uc);
            if (RT_SUCCESS(rc))
            {
                if (uc < 128)
                {
                    if (g_awchFatCp437Chars[uc] != uc)
                    {
                        if (uc)
                        {
                            uc = RTUniCpToUpper(uc);
                            if (g_awchFatCp437Chars[uc] != uc)
                                uc = '_';
                        }
                        else
                            break;
                    }
                }
                else
                    uc = '_';
            }
            else
                uc = '_';

            *pszDst++ = (char)uc;
            cchDst--;
        }
    }

    /* Pad the remaining space. */
    while (cchDst-- > 0)
        *pszDst++ = chPad;
}


/**
 * Generates a short filename.
 *
 * @returns IPRT status code.
 * @param   pThis           The directory.
 * @param   pszEntry        The long name (UTF-8).
 * @param   pDirEntry       Where to put the short name.
 */
static int rtFsFatDir_GenerateShortName(PRTFSFATDIR pThis, const char *pszEntry, PFATDIRENTRY pDirEntry)
{
    /* Do some input parsing. */
    const char  *pszExt      = RTPathSuffix(pszEntry);
    size_t const cchBasename = pszExt ? pszExt - pszEntry : strlen(pszEntry);
    size_t const cchExt      = pszExt ? strlen(++pszExt)  : 0;

    /* Fill in the extension first. It stays the same. */
    char szShortName[8+3+1];
    rtFsFatDir_CopyShortName(&szShortName[8], 3, pszExt, cchExt, ' ');
    szShortName[8+3] = '\0';

    /*
     * First try single digit 1..9.
     */
    rtFsFatDir_CopyShortName(szShortName, 6, pszEntry, cchBasename, '_');
    szShortName[6] = '~';
    for (uint32_t iLastDigit = 1; iLastDigit < 10; iLastDigit++)
    {
        szShortName[7] = iLastDigit + '0';
        int rc = rtFsFatDir_FindEntryShort(pThis, szShortName);
        if (rc == VERR_FILE_NOT_FOUND)
        {
            memcpy(pDirEntry->achName, szShortName, sizeof(pDirEntry->achName));
            return VINF_SUCCESS;
        }
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * First try two digits 10..99.
     */
    szShortName[5] = '~';
    for (uint32_t iFirstDigit = 1; iFirstDigit < 10; iFirstDigit++)
        for (uint32_t iLastDigit = 0; iLastDigit < 10; iLastDigit++)
        {
            szShortName[6] = iFirstDigit + '0';
            szShortName[7] = iLastDigit  + '0';
            int rc = rtFsFatDir_FindEntryShort(pThis, szShortName);
            if (rc == VERR_FILE_NOT_FOUND)
            {
                memcpy(pDirEntry->achName, szShortName, sizeof(pDirEntry->achName));
                return VINF_SUCCESS;
            }
            if (RT_FAILURE(rc))
                return rc;
        }

    /*
     * Okay, do random numbers then.
     */
    szShortName[2] = '~';
    for (uint32_t i = 0; i < 8192; i++)
    {
        char    szHex[68];
        ssize_t cchHex = RTStrFormatU32(szHex, sizeof(szHex), RTRandU32(), 16, 5, 0, RTSTR_F_CAPITAL | RTSTR_F_WIDTH | RTSTR_F_ZEROPAD);
        AssertReturn(cchHex >= 5, VERR_NET_NOT_UNIQUE_NAME);
        szShortName[7] = szHex[cchHex - 1];
        szShortName[6] = szHex[cchHex - 2];
        szShortName[5] = szHex[cchHex - 3];
        szShortName[4] = szHex[cchHex - 4];
        szShortName[3] = szHex[cchHex - 5];
        int rc = rtFsFatDir_FindEntryShort(pThis, szShortName);
        if (rc == VERR_FILE_NOT_FOUND)
        {
            memcpy(pDirEntry->achName, szShortName, sizeof(pDirEntry->achName));
            return VINF_SUCCESS;
        }
        if (RT_FAILURE(rc))
            return rc;
    }

    return VERR_NET_NOT_UNIQUE_NAME;
}


/**
 * Considers whether we need to create a long name or not.
 *
 * If a long name is needed and the name wasn't 8-dot-3 compatible, a 8-dot-3
 * name will be generated and stored in *pDirEntry.
 *
 * @returns IPRT status code
 * @param   pThis           The directory.
 * @param   pszEntry        The name.
 * @param   fIs8Dot3Name    Whether we have a 8-dot-3 name already.
 * @param   pDirEntry       Where to return the generated 8-dot-3 name.
 * @param   paSlots         Where to return the long name entries.  The array
 *                          can hold at least FATDIRNAMESLOT_MAX_SLOTS entries.
 * @param   pcSlots         Where to return the actual number of slots used.
 */
static int rtFsFatDir_MaybeCreateLongNameAndShortAlias(PRTFSFATDIR pThis, const char *pszEntry, bool fIs8Dot3Name,
                                                       PFATDIRENTRY pDirEntry, PFATDIRNAMESLOT paSlots, uint32_t *pcSlots)
{
    RT_NOREF(pThis, pDirEntry, paSlots, pszEntry);

    /*
     * If we don't need to create a long name, return immediately.
     */
    if (!rtFsFatDir_NeedLongName(pszEntry, fIs8Dot3Name, pDirEntry))
    {
        *pcSlots = 0;
        return VINF_SUCCESS;
    }

    /*
     * Convert the name to UTF-16 and figure it's length (this validates the
     * input encoding).  Then do long name validation (length, charset limitation).
     */
    RTUTF16     wszEntry[FATDIRNAMESLOT_MAX_SLOTS * FATDIRNAMESLOT_CHARS_PER_SLOT + 4];
    PRTUTF16    pwszEntry = wszEntry;
    size_t      cwcEntry;
    int rc = RTStrToUtf16Ex(pszEntry, RTSTR_MAX, &pwszEntry, RT_ELEMENTS(wszEntry), &cwcEntry);
    if (RT_SUCCESS(rc))
        rc = rtFsFatDir_ValidateLongName(pwszEntry, cwcEntry);
    if (RT_SUCCESS(rc))
    {
        /*
         * Generate a short name if we need to.
         */
        if (!fIs8Dot3Name)
            rc = rtFsFatDir_GenerateShortName(pThis, pszEntry, pDirEntry);
        if (RT_SUCCESS(rc))
        {
            /*
             * Fill in the long name slots.  First we pad the wszEntry with 0xffff
             * until it is a multiple of of the slot count.  That way we can copy
             * the name straight into the entry without constaints.
             */
            memset(&wszEntry[cwcEntry + 1], 0xff,
                   RT_MIN(sizeof(wszEntry) - (cwcEntry + 1) * sizeof(RTUTF16),
                          FATDIRNAMESLOT_CHARS_PER_SLOT * sizeof(RTUTF16)));

            uint8_t const   bChecksum = rtFsFatDir_CalcChecksum(pDirEntry);
            size_t const    cSlots    = (cwcEntry + FATDIRNAMESLOT_CHARS_PER_SLOT - 1) / FATDIRNAMESLOT_CHARS_PER_SLOT;
            size_t          iSlot     = cSlots;
            PCRTUTF16       pwszSrc   = wszEntry;
            while (iSlot-- > 0)
            {
                memcpy(paSlots[iSlot].awcName0, pwszSrc, sizeof(paSlots[iSlot].awcName0));
                pwszSrc += RT_ELEMENTS(paSlots[iSlot].awcName0);
                memcpy(paSlots[iSlot].awcName1, pwszSrc, sizeof(paSlots[iSlot].awcName1));
                pwszSrc += RT_ELEMENTS(paSlots[iSlot].awcName1);
                memcpy(paSlots[iSlot].awcName2, pwszSrc, sizeof(paSlots[iSlot].awcName2));
                pwszSrc += RT_ELEMENTS(paSlots[iSlot].awcName2);

                paSlots[iSlot].idSlot    = (uint8_t)(cSlots - iSlot);
                paSlots[iSlot].fAttrib   = FAT_ATTR_NAME_SLOT;
                paSlots[iSlot].fZero     = 0;
                paSlots[iSlot].idxZero   = 0;
                paSlots[iSlot].bChecksum = bChecksum;
            }
            paSlots[0].idSlot |= FATDIRNAMESLOT_FIRST_SLOT_FLAG;
            *pcSlots = (uint32_t)cSlots;
            return VINF_SUCCESS;
        }
    }
    *pcSlots = UINT32_MAX;
    return rc;
}


/**
 * Searches the directory for a given number of free directory entries.
 *
 * The free entries must be consecutive of course.
 *
 * @returns IPRT status code.
 * @retval  VERR_DISK_FULL if no space was found, *pcFreeTail set.
 * @param   pThis           The directory to search.
 * @param   cEntriesNeeded  How many entries we need.
 * @param   poffEntryInDir  Where to return the offset of the first entry we
 *                          found.
 * @param   pcFreeTail      Where to return the number of free entries at the
 *                          end of the directory when VERR_DISK_FULL is
 *                          returned.
 */
static int rtFsFatChain_FindFreeEntries(PRTFSFATDIR pThis, uint32_t cEntriesNeeded,
                                        uint32_t *poffEntryInDir, uint32_t *pcFreeTail)
{
    /* First try make gcc happy. */
    *pcFreeTail     = 0;
    *poffEntryInDir = UINT32_MAX;

    /*
     * Scan the whole directory, buffer by buffer.
     */
    uint32_t            offStartFreeEntries = UINT32_MAX;
    uint32_t            cFreeEntries        = 0;
    uint32_t            offEntryInDir       = 0;
    uint32_t const      cbDir               = pThis->Core.cbObject;
    Assert(RT_ALIGN_32(cbDir, sizeof(FATDIRENTRY)) == cbDir);
    while (offEntryInDir < cbDir)
    {
        /* Get chunk of entries starting at offEntryInDir. */
        uint32_t            uBufferLock = UINT32_MAX;
        uint32_t            cEntries    = 0;
        PCFATDIRENTRYUNION  paEntries   = NULL;
        int rc = rtFsFatDir_GetEntriesAt(pThis, offEntryInDir, &paEntries, &cEntries, &uBufferLock);
        if (RT_FAILURE(rc))
            return rc;

        /*
         * Now work thru each of the entries.
         */
        for (uint32_t iEntry = 0; iEntry < cEntries; iEntry++, offEntryInDir += sizeof(FATDIRENTRY))
        {
            uint8_t const bFirst = paEntries[iEntry].Entry.achName[0];
            if (   bFirst == FATDIRENTRY_CH0_DELETED
                || bFirst == FATDIRENTRY_CH0_END_OF_DIR)
            {
                if (offStartFreeEntries != UINT32_MAX)
                    cFreeEntries++;
                else
                {
                    offStartFreeEntries = offEntryInDir;
                    cFreeEntries        = 1;
                }
                if (cFreeEntries >= cEntriesNeeded)
                {
                    *pcFreeTail     = cEntriesNeeded;
                    *poffEntryInDir = offStartFreeEntries;
                    rtFsFatDir_ReleaseBufferAfterReading(pThis, uBufferLock);
                    return VINF_SUCCESS;
                }

                if (bFirst == FATDIRENTRY_CH0_END_OF_DIR)
                {
                    if (pThis->Core.pVol->enmBpbVersion >= RTFSFATBPBVER_DOS_2_0)
                    {
                        rtFsFatDir_ReleaseBufferAfterReading(pThis, uBufferLock);
                        *pcFreeTail = cFreeEntries = (cbDir - offStartFreeEntries) / sizeof(FATDIRENTRY);
                        if (cFreeEntries >= cEntriesNeeded)
                        {
                            *poffEntryInDir = offStartFreeEntries;
                            rtFsFatDir_ReleaseBufferAfterReading(pThis, uBufferLock);
                            return VINF_SUCCESS;
                        }
                        return VERR_DISK_FULL;
                    }
                }
            }
            else if (offStartFreeEntries != UINT32_MAX)
            {
                offStartFreeEntries = UINT32_MAX;
                cFreeEntries        = 0;
            }
        }
        rtFsFatDir_ReleaseBufferAfterReading(pThis, uBufferLock);
    }
    *pcFreeTail = cFreeEntries;
    return VERR_DISK_FULL;
}


/**
 * Try grow the directory.
 *
 * This is not called on the root directory.
 *
 * @returns IPRT status code.
 * @retval  VERR_DISK_FULL if we failed to allocated new space.
 * @param   pThis           The directory to grow.
 * @param   cMinNewEntries  The minimum number of new entries to allocated.
 */
static int rtFsFatChain_GrowDirectory(PRTFSFATDIR pThis, uint32_t cMinNewEntries)
{
    RT_NOREF(pThis, cMinNewEntries);
    return VERR_DISK_FULL;
}


/**
 * Inserts a directory with zero of more long name slots preceeding it.
 *
 * @returns IPRT status code.
 * @param   pThis           The directory.
 * @param   pDirEntry       The directory entry.
 * @param   paSlots         The long name slots.
 * @param   cSlots          The number of long name slots.
 * @param   poffEntryInDir  Where to return the directory offset.
 */
static int rtFsFatChain_InsertEntries(PRTFSFATDIR pThis, PCFATDIRENTRY pDirEntry, PFATDIRNAMESLOT paSlots, uint32_t cSlots,
                                      uint32_t *poffEntryInDir)
{
    uint32_t const cTotalEntries = cSlots + 1;

    /*
     * Find somewhere to put the entries.  Try extend the directory if we're
     * not successful at first.
     */
    uint32_t cFreeTailEntries;
    uint32_t offFirstInDir;
    int rc = rtFsFatChain_FindFreeEntries(pThis, cTotalEntries, &offFirstInDir, &cFreeTailEntries);
    if (rc == VERR_DISK_FULL)
    {
        Assert(cFreeTailEntries < cTotalEntries);

        /* Try grow it and use the newly allocated space. */
        if (   pThis->Core.pParentDir
            && pThis->cEntries < _64K /* Don't grow beyond 64K entries */)
        {
            offFirstInDir = pThis->Core.cbObject - cFreeTailEntries * sizeof(FATDIRENTRY);
            rc = rtFsFatChain_GrowDirectory(pThis, cTotalEntries - cFreeTailEntries);
        }

        if (rc == VERR_DISK_FULL)
        {
            /** @todo Try compact the directory if we couldn't grow it. */
        }
    }
    if (RT_SUCCESS(rc))
    {
        /*
         * Update the directory.
         */
        uint32_t      offCurrent = offFirstInDir;
        for (uint32_t iSrcSlot   = 0; iSrcSlot < cTotalEntries; iSrcSlot++, offCurrent += sizeof(FATDIRENTRY))
        {
            uint32_t        uBufferLock;
            PFATDIRENTRY    pDstEntry;
            rc = rtFsFatDir_GetEntryForUpdate(pThis, offCurrent, &pDstEntry, &uBufferLock);
            if (RT_SUCCESS(rc))
            {
                if (iSrcSlot < cSlots)
                    memcpy(pDstEntry, &paSlots[iSrcSlot], sizeof(*pDstEntry));
                else
                    memcpy(pDstEntry, pDirEntry,          sizeof(*pDstEntry));
                rc = rtFsFatDir_PutEntryAfterUpdate(pThis, pDstEntry, uBufferLock);
                if (RT_SUCCESS(rc))
                    continue;

                /*
                 * Bail out: Try mark any edited entries as deleted.
                 */
                iSrcSlot++;
            }
            while (iSrcSlot-- > 0)
            {
                int rc2 = rtFsFatDir_GetEntryForUpdate(pThis, offFirstInDir + iSrcSlot * sizeof(FATDIRENTRY),
                                                       &pDstEntry, &uBufferLock);
                if (RT_SUCCESS(rc2))
                {
                    pDstEntry->achName[0] = FATDIRENTRY_CH0_DELETED;
                    rtFsFatDir_PutEntryAfterUpdate(pThis, pDstEntry, uBufferLock);
                }
            }
            *poffEntryInDir = UINT32_MAX;
            return rc;
        }
        AssertRC(rc);

        /*
         * Successfully inserted all.
         */
        *poffEntryInDir = offFirstInDir + cSlots * sizeof(FATDIRENTRY);
        return VINF_SUCCESS;
    }

    *poffEntryInDir = UINT32_MAX;
    return rc;
}



/**
 * Creates a new directory entry.
 *
 * @returns IPRT status code
 * @param   pThis           The directory.
 * @param   pszEntry        The name of the new entry.
 * @param   fAttrib         The attributes.
 * @param   cbInitial       The initialize size.
 * @param   poffEntryInDir  Where to return the offset of the directory entry.
 * @param   pDirEntry       Where to return a copy of the directory entry.
 *
 * @remarks ASSUMES caller has already called rtFsFatDir_FindEntry to make sure
 *          the entry doesn't exist.
 */
static int rtFsFatDir_CreateEntry(PRTFSFATDIR pThis, const char *pszEntry, uint8_t fAttrib, uint32_t cbInitial,
                                  uint32_t *poffEntryInDir, PFATDIRENTRY pDirEntry)
{
    PRTFSFATVOL pVol = pThis->Core.pVol;
    *poffEntryInDir = UINT32_MAX;
    if (pVol->fReadOnly)
        return VERR_WRITE_PROTECT;

    /*
     * Create the directory entries on the stack.
     */
    bool fIs8Dot3Name = rtFsFatDir_StringTo8Dot3((char *)pDirEntry->achName, pszEntry);
    pDirEntry->fAttrib            = fAttrib;
    pDirEntry->fCase              = fIs8Dot3Name ? rtFsFatDir_CalcCaseFlags(pszEntry) : 0;
    pDirEntry->uBirthCentiseconds = rtFsFatCurrentFatDateTime(pVol, &pDirEntry->uBirthDate, &pDirEntry->uBirthTime);
    pDirEntry->uAccessDate        = pDirEntry->uBirthDate;
    pDirEntry->uModifyDate        = pDirEntry->uBirthDate;
    pDirEntry->uModifyTime        = pDirEntry->uBirthTime;
    pDirEntry->idxCluster         = 0;    /* Will fill this in later if cbInitial is non-zero. */
    pDirEntry->u.idxClusterHigh   = 0;
    pDirEntry->cbFile             = cbInitial;

    /*
     * Create long filename slots if necessary.
     */
    uint32_t        cSlots = UINT32_MAX;
    FATDIRNAMESLOT  aSlots[FATDIRNAMESLOT_MAX_SLOTS];
    AssertCompile(RTFSFAT_MAX_LFN_CHARS < RT_ELEMENTS(aSlots) * FATDIRNAMESLOT_CHARS_PER_SLOT);
    int rc = rtFsFatDir_MaybeCreateLongNameAndShortAlias(pThis, pszEntry, fIs8Dot3Name, pDirEntry, aSlots, &cSlots);
    if (RT_SUCCESS(rc))
    {
        Assert(cSlots <= FATDIRNAMESLOT_MAX_SLOTS);

        /*
         * Allocate initial clusters if requested.
         */
        RTFSFATCHAIN Clusters;
        rtFsFatChain_InitEmpty(&Clusters, pVol);
        if (cbInitial > 0)
        {
            rc = rtFsFatClusterMap_AllocateMoreClusters(pVol, &Clusters,
                                                        (cbInitial + Clusters.cbCluster - 1) >> Clusters.cClusterByteShift);
            if (RT_SUCCESS(rc))
            {
                uint32_t idxFirstCluster = rtFsFatChain_GetFirstCluster(&Clusters);
                pDirEntry->idxCluster = (uint16_t)idxFirstCluster;
                if (pVol->enmFatType >= RTFSFATTYPE_FAT32)
                    pDirEntry->u.idxClusterHigh = (uint16_t)(idxFirstCluster >> 16);
            }
        }
        if (RT_SUCCESS(rc))
        {
            /*
             * Insert the directory entry and name slots.
             */
            rc = rtFsFatChain_InsertEntries(pThis, pDirEntry, aSlots, cSlots, poffEntryInDir);
            if (RT_SUCCESS(rc))
            {
                rtFsFatChain_Delete(&Clusters);
                return VINF_SUCCESS;
            }

            for (uint32_t iClusterToFree = 0; iClusterToFree < Clusters.cClusters; iClusterToFree++)
                rtFsFatClusterMap_FreeCluster(pVol, rtFsFatChain_GetClusterByIndex(&Clusters, iClusterToFree));
            rtFsFatChain_Delete(&Clusters);
        }
    }
    return rc;
}



/**
 * @interface_method_impl{RTVFSOBJOPS,pfnClose}
 */
static DECLCALLBACK(int) rtFsFatDir_Close(void *pvThis)
{
    PRTFSFATDIR pThis = (PRTFSFATDIR)pvThis;
    int rc;
    if (pThis->paEntries)
    {
        rc = rtFsFatDir_Flush(pThis);
        RTMemFree(pThis->paEntries);
        pThis->paEntries = NULL;
    }
    else
        rc = VINF_SUCCESS;

    rtFsFatObj_Close(&pThis->Core);
    return rc;
}


/**
 * @interface_method_impl{RTVFSOBJOPS,pfnTraversalOpen}
 */
static DECLCALLBACK(int) rtFsFatDir_TraversalOpen(void *pvThis, const char *pszEntry, PRTVFSDIR phVfsDir,
                                                  PRTVFSSYMLINK phVfsSymlink, PRTVFS phVfsMounted)
{
    /*
     * FAT doesn't do symbolic links and mounting file systems within others
     * haven't been implemented yet, I think, so only care if a directory is
     * asked for.
     */
    int rc;
    if (phVfsSymlink)
        *phVfsSymlink = NIL_RTVFSSYMLINK;
    if (phVfsMounted)
        *phVfsMounted = NIL_RTVFS;
    if (phVfsDir)
    {
        *phVfsDir = NIL_RTVFSDIR;

        PRTFSFATDIR pThis = (PRTFSFATDIR)pvThis;
        uint32_t    offEntryInDir;
        bool        fLong;
        FATDIRENTRY DirEntry;
        rc = rtFsFatDir_FindEntry(pThis, pszEntry, &offEntryInDir, &fLong, &DirEntry);
        if (RT_SUCCESS(rc))
        {
            switch (DirEntry.fAttrib & (FAT_ATTR_DIRECTORY | FAT_ATTR_VOLUME))
            {
                case FAT_ATTR_DIRECTORY:
                {
                    rc = rtFsFatDir_New(pThis->Core.pVol, pThis, &DirEntry, offEntryInDir,
                                        RTFSFAT_GET_CLUSTER(&DirEntry, pThis->Core.pVol), UINT64_MAX /*offDisk*/,
                                        DirEntry.cbFile, phVfsDir, NULL /*ppDir*/);
                    break;
                }
                case 0:
                    rc = VERR_NOT_A_DIRECTORY;
                    break;
                default:
                    rc = VERR_PATH_NOT_FOUND;
                    break;
            }
        }
        else if (rc == VERR_FILE_NOT_FOUND)
            rc = VERR_PATH_NOT_FOUND;
    }
    else
        rc = VERR_PATH_NOT_FOUND;
    return rc;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnOpenFile}
 */
static DECLCALLBACK(int) rtFsFatDir_OpenFile(void *pvThis, const char *pszFilename, uint32_t fOpen, PRTVFSFILE phVfsFile)
{
    PRTFSFATDIR pThis = (PRTFSFATDIR)pvThis;

    /*
     * Try open existing file.
     */
    uint32_t    offEntryInDir;
    bool        fLong;
    FATDIRENTRY DirEntry;
    int rc = rtFsFatDir_FindEntry(pThis, pszFilename, &offEntryInDir, &fLong, &DirEntry);
    if (RT_SUCCESS(rc))
    {
        switch (DirEntry.fAttrib & (FAT_ATTR_DIRECTORY | FAT_ATTR_VOLUME))
        {
            case 0:
                if (   !(DirEntry.fAttrib & FAT_ATTR_READONLY)
                    || !(fOpen & RTFILE_O_WRITE))
                {
                    if (   (fOpen & RTFILE_O_ACTION_MASK) == RTFILE_O_OPEN
                        || (fOpen & RTFILE_O_ACTION_MASK) == RTFILE_O_OPEN_CREATE
                        || (fOpen & RTFILE_O_ACTION_MASK) == RTFILE_O_CREATE_REPLACE)
                        rc = rtFsFatFile_New(pThis->Core.pVol, pThis, &DirEntry, offEntryInDir, fOpen, phVfsFile);
                    else
                        rc = VERR_ALREADY_EXISTS;
                }
                else
                    rc = VERR_ACCESS_DENIED;
                break;

            case FAT_ATTR_DIRECTORY:
                rc = VERR_NOT_A_FILE;
                break;
            default:
                rc = VERR_PATH_NOT_FOUND;
                break;
        }
    }
    /*
     * Create the file?
     */
    else if (   rc == VERR_FILE_NOT_FOUND
             && (   (fOpen & RTFILE_O_ACTION_MASK) == RTFILE_O_CREATE
                 || (fOpen & RTFILE_O_ACTION_MASK) == RTFILE_O_OPEN_CREATE
                 || (fOpen & RTFILE_O_ACTION_MASK) == RTFILE_O_CREATE_REPLACE) )
    {
        rc = rtFsFatDir_CreateEntry(pThis, pszFilename, FAT_ATTR_ARCHIVE, 0 /*cbInitial*/, &offEntryInDir, &DirEntry);
        if (RT_SUCCESS(rc))
            rc = rtFsFatFile_New(pThis->Core.pVol, pThis, &DirEntry, offEntryInDir, fOpen, phVfsFile);
    }
    return rc;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnOpenDir}
 */
static DECLCALLBACK(int) rtFsFatDir_OpenDir(void *pvThis, const char *pszSubDir, uint32_t fFlags, PRTVFSDIR phVfsDir)
{
    RT_NOREF(pvThis, pszSubDir, fFlags, phVfsDir);
RTAssertMsg2("%s: %s\n", __FUNCTION__, pszSubDir);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnCreateDir}
 */
static DECLCALLBACK(int) rtFsFatDir_CreateDir(void *pvThis, const char *pszSubDir, RTFMODE fMode, PRTVFSDIR phVfsDir)
{
    RT_NOREF(pvThis, pszSubDir, fMode, phVfsDir);
RTAssertMsg2("%s: %s\n", __FUNCTION__, pszSubDir);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnOpenSymlink}
 */
static DECLCALLBACK(int) rtFsFatDir_OpenSymlink(void *pvThis, const char *pszSymlink, PRTVFSSYMLINK phVfsSymlink)
{
    RT_NOREF(pvThis, pszSymlink, phVfsSymlink);
RTAssertMsg2("%s: %s\n", __FUNCTION__, pszSymlink);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnCreateSymlink}
 */
static DECLCALLBACK(int) rtFsFatDir_CreateSymlink(void *pvThis, const char *pszSymlink, const char *pszTarget,
                                                  RTSYMLINKTYPE enmType, PRTVFSSYMLINK phVfsSymlink)
{
    RT_NOREF(pvThis, pszSymlink, pszTarget, enmType, phVfsSymlink);
RTAssertMsg2("%s: %s\n", __FUNCTION__, pszSymlink);
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnQueryEntryInfo}
 */
static DECLCALLBACK(int) rtFsFatDir_QueryEntryInfo(void *pvThis, const char *pszEntry,
                                                   PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    /*
     * Try locate the entry.
     */
    PRTFSFATDIR pThis = (PRTFSFATDIR)pvThis;
    uint32_t    offEntryInDir;
    bool        fLong;
    FATDIRENTRY DirEntry;
    int rc = rtFsFatDir_FindEntry(pThis, pszEntry, &offEntryInDir, &fLong, &DirEntry);
    Log2(("rtFsFatDir_QueryEntryInfo: FindEntry(,%s,) -> %Rrc\n", pszEntry, rc));
    if (RT_SUCCESS(rc))
    {
        /*
         * To avoid duplicating code in rtFsFatObj_InitFromDirRec and
         * rtFsFatObj_QueryInfo, we create a dummy RTFSFATOBJ on the stack.
         */
        RTFSFATOBJ TmpObj;
        RT_ZERO(TmpObj);
        rtFsFatObj_InitFromDirEntry(&TmpObj, &DirEntry, offEntryInDir, pThis->Core.pVol);
        rc = rtFsFatObj_QueryInfo(&TmpObj, pObjInfo, enmAddAttr);
    }
    return rc;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnUnlinkEntry}
 */
static DECLCALLBACK(int) rtFsFatDir_UnlinkEntry(void *pvThis, const char *pszEntry, RTFMODE fType)
{
    RT_NOREF(pvThis, pszEntry, fType);
RTAssertMsg2("%s: %s\n", __FUNCTION__, pszEntry);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnRenameEntry}
 */
static DECLCALLBACK(int) rtFsFatDir_RenameEntry(void *pvThis, const char *pszEntry, RTFMODE fType, const char *pszNewName)
{
    RT_NOREF(pvThis, pszEntry, fType, pszNewName);
RTAssertMsg2("%s: %s\n", __FUNCTION__, pszEntry);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnRewindDir}
 */
static DECLCALLBACK(int) rtFsFatDir_RewindDir(void *pvThis)
{
    RT_NOREF(pvThis);
RTAssertMsg2("%s\n", __FUNCTION__);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * @interface_method_impl{RTVFSDIROPS,pfnReadDir}
 */
static DECLCALLBACK(int) rtFsFatDir_ReadDir(void *pvThis, PRTDIRENTRYEX pDirEntry, size_t *pcbDirEntry,
                                            RTFSOBJATTRADD enmAddAttr)
{
    RT_NOREF(pvThis, pDirEntry, pcbDirEntry, enmAddAttr);
RTAssertMsg2("%s\n", __FUNCTION__);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * FAT file operations.
 */
static const RTVFSDIROPS g_rtFsFatDirOps =
{
    { /* Obj */
        RTVFSOBJOPS_VERSION,
        RTVFSOBJTYPE_DIR,
        "FatDir",
        rtFsFatDir_Close,
        rtFsFatObj_QueryInfo,
        RTVFSOBJOPS_VERSION
    },
    RTVFSDIROPS_VERSION,
    0,
    { /* ObjSet */
        RTVFSOBJSETOPS_VERSION,
        RT_OFFSETOF(RTVFSDIROPS, Obj) - RT_OFFSETOF(RTVFSDIROPS, ObjSet),
        rtFsFatObj_SetMode,
        rtFsFatObj_SetTimes,
        rtFsFatObj_SetOwner,
        RTVFSOBJSETOPS_VERSION
    },
    rtFsFatDir_TraversalOpen,
    rtFsFatDir_OpenFile,
    rtFsFatDir_OpenDir,
    rtFsFatDir_CreateDir,
    rtFsFatDir_OpenSymlink,
    rtFsFatDir_CreateSymlink,
    rtFsFatDir_QueryEntryInfo,
    rtFsFatDir_UnlinkEntry,
    rtFsFatDir_RenameEntry,
    rtFsFatDir_RewindDir,
    rtFsFatDir_ReadDir,
    RTVFSDIROPS_VERSION,
};


/**
 * Adds an open child to the parent directory.
 *
 * Maintains an additional reference to the parent dir to prevent it from going
 * away.  If @a pDir is the root directory, it also ensures the volume is
 * referenced and sticks around until the last open object is gone.
 *
 * @param   pDir        The directory.
 * @param   pChild      The child being opened.
 * @sa      rtFsFatDir_RemoveOpenChild
 */
static void rtFsFatDir_AddOpenChild(PRTFSFATDIR pDir, PRTFSFATOBJ pChild)
{
    /* First child that gets opened retains the parent directory.  This is
       released by the final open child. */
    if (RTListIsEmpty(&pDir->OpenChildren))
    {
        uint32_t cRefs = RTVfsDirRetain(pDir->hVfsSelf);
        Assert(cRefs != UINT32_MAX); NOREF(cRefs);

        /* Root also retains the whole file system. */
        if (pDir->Core.pVol->pRootDir == pDir)
        {
            Assert(pDir->Core.pVol);
            Assert(pDir->Core.pVol == pChild->pVol);
            cRefs = RTVfsRetain(pDir->Core.pVol->hVfsSelf);
            Assert(cRefs != UINT32_MAX); NOREF(cRefs);
        }
    }
    RTListAppend(&pDir->OpenChildren, &pChild->Entry);
    pChild->pParentDir = pDir;
}


/**
 * Removes an open child to the parent directory.
 *
 * @param   pDir        The directory.
 * @param   pChild      The child being removed.
 *
 * @remarks This is the very last thing you do as it may cause a few other
 *          objects to be released recursively (parent dir and the volume).
 *
 * @sa      rtFsFatDir_AddOpenChild
 */
static void rtFsFatDir_RemoveOpenChild(PRTFSFATDIR pDir, PRTFSFATOBJ pChild)
{
    AssertReturnVoid(pChild->pParentDir == pDir);
    RTListNodeRemove(&pChild->Entry);
    pChild->pParentDir = NULL;

    /* Final child? If so, release directory. */
    if (RTListIsEmpty(&pDir->OpenChildren))
    {
        bool const fIsRootDir = pDir->Core.pVol->pRootDir == pDir;

        uint32_t cRefs = RTVfsDirRelease(pDir->hVfsSelf);
        Assert(cRefs != UINT32_MAX); NOREF(cRefs);

        /* Root directory releases the file system as well.  Since the volume
           holds a reference to the root directory, it will remain valid after
           the above release. */
        if (fIsRootDir)
        {
            Assert(cRefs > 0);
            Assert(pDir->Core.pVol);
            Assert(pDir->Core.pVol == pChild->pVol);
            cRefs = RTVfsRelease(pDir->Core.pVol->hVfsSelf);
            Assert(cRefs != UINT32_MAX); NOREF(cRefs);
        }
    }
}


/**
 * Instantiates a new directory.
 *
 * @returns IPRT status code.
 * @param   pThis           The FAT volume instance.
 * @param   pParentDir      The parent directory.  This is NULL for the root
 *                          directory.
 * @param   pDirEntry       The parent directory entry. This is NULL for the
 *                          root directory.
 * @param   offEntryInDir   The byte offset of the directory entry in the parent
 *                          directory.  UINT32_MAX if root directory.
 * @param   idxCluster      The cluster where the directory content is to be
 *                          found. This can be UINT32_MAX if a root FAT12/16
 *                          directory.
 * @param   offDisk         The disk byte offset of the FAT12/16 root directory.
 *                          This is UINT64_MAX if idxCluster is given.
 * @param   cbDir           The size of the directory.
 * @param   phVfsDir        Where to return the directory handle.
 * @param   ppDir           Where to return the FAT directory instance data.
 */
static int rtFsFatDir_New(PRTFSFATVOL pThis, PRTFSFATDIR pParentDir, PCFATDIRENTRY pDirEntry, uint32_t offEntryInDir,
                          uint32_t idxCluster, uint64_t offDisk, uint32_t cbDir, PRTVFSDIR phVfsDir, PRTFSFATDIR *ppDir)
{
    Assert((idxCluster == UINT32_MAX) != (offDisk == UINT64_MAX));
    Assert((pDirEntry == NULL) == (offEntryInDir == UINT32_MAX));
    if (ppDir)
        *ppDir = NULL;

    PRTFSFATDIR pNewDir;
    int rc = RTVfsNewDir(&g_rtFsFatDirOps, sizeof(*pNewDir), pParentDir ? 0 : RTVFSDIR_F_NO_VFS_REF,
                         pThis->hVfsSelf, NIL_RTVFSLOCK /*use volume lock*/, phVfsDir, (void **)&pNewDir);
    if (RT_SUCCESS(rc))
    {
        /*
         * Initialize it all so rtFsFatDir_Close doesn't trip up in anyway.
         */
        RTListInit(&pNewDir->OpenChildren);
        if (pDirEntry)
            rtFsFatObj_InitFromDirEntry(&pNewDir->Core, pDirEntry, offEntryInDir, pThis);
        else
            rtFsFatObj_InitDummy(&pNewDir->Core, cbDir, RTFS_TYPE_DIRECTORY | RTFS_DOS_DIRECTORY | RTFS_UNIX_ALL_PERMS, pThis);

        pNewDir->hVfsSelf           = *phVfsDir;
        pNewDir->cEntries           = cbDir / sizeof(FATDIRENTRY);
        pNewDir->fIsLinearRootDir   = idxCluster == UINT32_MAX;
        pNewDir->fFullyBuffered     = pNewDir->fIsLinearRootDir;
        pNewDir->paEntries          = NULL;
        pNewDir->offEntriesOnDisk   = UINT64_MAX;
        if (pNewDir->fFullyBuffered)
            pNewDir->cbAllocatedForEntries = RT_ALIGN_32(cbDir, pThis->cbSector);
        else
            pNewDir->cbAllocatedForEntries = pThis->cbSector;

        /*
         * If clustered backing, read the chain and see if we cannot still do the full buffering.
         */
        if (idxCluster != UINT32_MAX)
        {
            rc = rtFsFatClusterMap_ReadClusterChain(pThis, idxCluster, &pNewDir->Core.Clusters);
            if (RT_SUCCESS(rc))
            {
                if (   pNewDir->Core.Clusters.cClusters >= 1
                    && pNewDir->Core.Clusters.cbChain   <= _64K
                    && rtFsFatChain_IsContiguous(&pNewDir->Core.Clusters))
                {
                    Assert(pNewDir->Core.Clusters.cbChain >= cbDir);
                    pNewDir->cbAllocatedForEntries = pNewDir->Core.Clusters.cbChain;
                    pNewDir->fFullyBuffered = true;
                }
            }
        }
        else
            rtFsFatChain_InitEmpty(&pNewDir->Core.Clusters, pThis);
        if (RT_SUCCESS(rc))
        {
            /*
             * Allocate and initialize the buffering.  Fill the buffer.
             */
            pNewDir->paEntries = (PFATDIRENTRYUNION)RTMemAlloc(pNewDir->cbAllocatedForEntries);
            if (!pNewDir->paEntries)
            {
                if (pNewDir->fFullyBuffered && !pNewDir->fIsLinearRootDir)
                {
                    pNewDir->fFullyBuffered = false;
                    pNewDir->cbAllocatedForEntries = pThis->cbSector;
                    pNewDir->paEntries = (PFATDIRENTRYUNION)RTMemAlloc(pNewDir->cbAllocatedForEntries);
                }
                if (!pNewDir->paEntries)
                    rc = VERR_NO_MEMORY;
            }

            if (RT_SUCCESS(rc))
            {
                if (pNewDir->fFullyBuffered)
                {
                    pNewDir->u.Full.cDirtySectors   = 0;
                    pNewDir->u.Full.cSectors        = pNewDir->cbAllocatedForEntries / pThis->cbSector;
                    pNewDir->u.Full.pbDirtySectors  = (uint8_t *)RTMemAllocZ((pNewDir->u.Full.cSectors + 63) / 8);
                    if (pNewDir->u.Full.pbDirtySectors)
                        pNewDir->offEntriesOnDisk   = offDisk != UINT64_MAX ? offDisk
                                                    : rtFsFatClusterToDiskOffset(pThis, idxCluster);
                    else
                        rc = VERR_NO_MEMORY;
                }
                else
                {
                    pNewDir->offEntriesOnDisk       = rtFsFatClusterToDiskOffset(pThis, idxCluster);
                    pNewDir->u.Simple.offInDir      = 0;
                    pNewDir->u.Simple.fDirty        = false;
                }
                if (RT_SUCCESS(rc))
                    rc = RTVfsFileReadAt(pThis->hVfsBacking, pNewDir->offEntriesOnDisk,
                                         pNewDir->paEntries, pNewDir->cbAllocatedForEntries, NULL);
                if (RT_SUCCESS(rc))
                {
                    /*
                     * Link into parent directory so we can use it to update
                     * our directory entry.
                     */
                    if (pParentDir)
                        rtFsFatDir_AddOpenChild(pParentDir, &pNewDir->Core);
                    if (ppDir)
                        *ppDir = pNewDir;
                    return VINF_SUCCESS;
                }
            }

            /* Free the buffer on failure so rtFsFatDir_Close doesn't try do anything with it. */
            RTMemFree(pNewDir->paEntries);
            pNewDir->paEntries = NULL;
        }

        RTVfsDirRelease(*phVfsDir);
    }
    *phVfsDir = NIL_RTVFSDIR;
    if (ppDir)
        *ppDir = NULL;
    return rc;
}





/**
 * @interface_method_impl{RTVFSOBJOPS::Obj,pfnClose}
 */
static DECLCALLBACK(int) rtFsFatVol_Close(void *pvThis)
{
    PRTFSFATVOL pThis = (PRTFSFATVOL)pvThis;
    LogFlow(("rtFsFatVol_Close(%p)\n", pThis));
    int rc = rtFsFatClusterMap_Destroy(pThis);

    if (pThis->hVfsRootDir != NIL_RTVFSDIR)
    {
        Assert(RTListIsEmpty(&pThis->pRootDir->OpenChildren));
        uint32_t cRefs = RTVfsDirRelease(pThis->hVfsRootDir);
        Assert(cRefs == 0); NOREF(cRefs);
        pThis->hVfsRootDir = NIL_RTVFSDIR;
        pThis->pRootDir    = NULL;
    }

    RTVfsFileRelease(pThis->hVfsBacking);
    pThis->hVfsBacking = NIL_RTVFSFILE;

    return rc;
}


/**
 * @interface_method_impl{RTVFSOBJOPS::Obj,pfnQueryInfo}
 */
static DECLCALLBACK(int) rtFsFatVol_QueryInfo(void *pvThis, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    RT_NOREF(pvThis, pObjInfo, enmAddAttr);
    return VERR_WRONG_TYPE;
}


/**
 * @interface_method_impl{RTVFSOPS,pfnOpenRoo}
 */
static DECLCALLBACK(int) rtFsFatVol_OpenRoot(void *pvThis, PRTVFSDIR phVfsDir)
{
    PRTFSFATVOL pThis = (PRTFSFATVOL)pvThis;
    uint32_t cRefs = RTVfsDirRetain(pThis->hVfsRootDir);
    if (cRefs != UINT32_MAX)
    {
        *phVfsDir = pThis->hVfsRootDir;
        return VINF_SUCCESS;
    }
    return VERR_INTERNAL_ERROR_5;
}


/**
 * @interface_method_impl{RTVFSOPS,pfnIsRangeInUse}
 */
static DECLCALLBACK(int) rtFsFatVol_IsRangeInUse(void *pvThis, RTFOFF off, size_t cb, bool *pfUsed)
{
    RT_NOREF(pvThis, off, cb, pfUsed);
    return VERR_NOT_IMPLEMENTED;
}


DECL_HIDDEN_CONST(const RTVFSOPS) g_rtFsFatVolOps =
{
    { /* Obj */
        RTVFSOBJOPS_VERSION,
        RTVFSOBJTYPE_VFS,
        "FatVol",
        rtFsFatVol_Close,
        rtFsFatVol_QueryInfo,
        RTVFSOBJOPS_VERSION
    },
    RTVFSOPS_VERSION,
    0 /* fFeatures */,
    rtFsFatVol_OpenRoot,
    rtFsFatVol_IsRangeInUse,
    RTVFSOPS_VERSION
};


/**
 * Tries to detect a DOS 1.x formatted image and fills in the BPB fields.
 *
 * There is no BPB here, but fortunately, there isn't much variety.
 *
 * @returns IPRT status code.
 * @param   pThis       The FAT volume instance, BPB derived fields are filled
 *                      in on success.
 * @param   pBootSector The boot sector.
 * @param   pbFatSector Points to the FAT sector, or whatever is 512 bytes after
 *                      the boot sector.
 * @param   pErrInfo    Where to return additional error information.
 */
static int rtFsFatVolTryInitDos1x(PRTFSFATVOL pThis, PCFATBOOTSECTOR pBootSector, uint8_t const *pbFatSector,
                                  PRTERRINFO pErrInfo)
{
    /*
     * PC-DOS 1.0 does a 2fh byte short jump w/o any NOP following it.
     * Instead the following are three words and a 9 byte build date
     * string.  The remaining space is zero filled.
     *
     * Note! No idea how this would look like for 8" floppies, only got 5"1/4'.
     *
     * ASSUME all non-BPB disks are using this format.
     */
    if (   pBootSector->abJmp[0] != 0xeb /* jmp rel8 */
        || pBootSector->abJmp[1] <  0x2f
        || pBootSector->abJmp[1] >= 0x80
        || pBootSector->abJmp[2] == 0x90 /* nop */)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_UNKNOWN_FORMAT,
                             "No DOS v1.0 bootsector either - invalid jmp: %.3Rhxs", pBootSector->abJmp);
    uint32_t const offJump      = 2 + pBootSector->abJmp[1];
    uint32_t const offFirstZero = 2 /*jmp */ + 3 * 2 /* words */ + 9 /* date string */;
    Assert(offFirstZero >= RT_UOFFSETOF(FATBOOTSECTOR, Bpb));
    uint32_t const cbZeroPad    = RT_MIN(offJump - offFirstZero,
                                         sizeof(pBootSector->Bpb.Bpb20) - (offFirstZero - RT_OFFSETOF(FATBOOTSECTOR, Bpb)));

    if (!ASMMemIsAllU8((uint8_t const *)pBootSector + offFirstZero, cbZeroPad, 0))
        return RTErrInfoSetF(pErrInfo, VERR_VFS_UNKNOWN_FORMAT,
                             "No DOS v1.0 bootsector either - expected zero padding %#x LB %#x: %.*Rhxs",
                             offFirstZero, cbZeroPad, cbZeroPad, (uint8_t const *)pBootSector + offFirstZero);

    /*
     * Check the FAT ID so we can tell if this is double or single sided,
     * as well as being a valid FAT12 start.
     */
    if (   (pbFatSector[0] != 0xfe && pbFatSector[0] != 0xff)
        || pbFatSector[1] != 0xff
        || pbFatSector[2] != 0xff)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_UNKNOWN_FORMAT,
                             "No DOS v1.0 bootsector either - unexpected start of FAT: %.3Rhxs", pbFatSector);

    /*
     * Fixed DOS 1.0 config.
     */
    pThis->enmFatType       = RTFSFATTYPE_FAT12;
    pThis->enmBpbVersion    = RTFSFATBPBVER_NO_BPB;
    pThis->bMedia           = pbFatSector[0];
    pThis->cReservedSectors = 1;
    pThis->cbSector         = 512;
    pThis->cbCluster        = pThis->bMedia == 0xfe ? 1024 : 512;
    pThis->cFats            = 2;
    pThis->cbFat            = 512;
    pThis->aoffFats[0]      = pThis->offBootSector + pThis->cReservedSectors * 512;
    pThis->aoffFats[1]      = pThis->aoffFats[0] + pThis->cbFat;
    pThis->offRootDir       = pThis->aoffFats[1] + pThis->cbFat;
    pThis->cRootDirEntries  = 512;
    pThis->offFirstCluster  = pThis->offRootDir + RT_ALIGN_32(pThis->cRootDirEntries * sizeof(FATDIRENTRY),
                                                              pThis->cbSector);
    pThis->cbTotalSize      = pThis->bMedia == 0xfe ? 8 * 1 * 40 * 512 : 8 * 2 * 40 * 512;
    pThis->cClusters        = (pThis->cbTotalSize - (pThis->offFirstCluster - pThis->offBootSector)) / pThis->cbCluster;
    return VINF_SUCCESS;
}


/**
 * Worker for rtFsFatVolTryInitDos2Plus that handles remaining BPB fields.
 *
 * @returns IPRT status code.
 * @param   pThis       The FAT volume instance, BPB derived fields are filled
 *                      in on success.
 * @param   pBootSector The boot sector.
 * @param   fMaybe331   Set if it could be a DOS v3.31 BPB.
 * @param   pErrInfo    Where to return additional error information.
 */
static int rtFsFatVolTryInitDos2PlusBpb(PRTFSFATVOL pThis, PCFATBOOTSECTOR pBootSector, bool fMaybe331, PRTERRINFO pErrInfo)
{
    pThis->enmBpbVersion = RTFSFATBPBVER_DOS_2_0;

    /*
     * Figure total sector count.  Could both be zero, in which case we have to
     * fall back on the size of the backing stuff.
     */
    if (pBootSector->Bpb.Bpb20.cTotalSectors16 != 0)
        pThis->cbTotalSize = pBootSector->Bpb.Bpb20.cTotalSectors16 * pThis->cbSector;
    else if (   pBootSector->Bpb.Bpb331.cTotalSectors32 != 0
             && fMaybe331)
    {
        pThis->enmBpbVersion = RTFSFATBPBVER_DOS_3_31;
        pThis->cbTotalSize = pBootSector->Bpb.Bpb331.cTotalSectors32 * (uint64_t)pThis->cbSector;
    }
    else
        pThis->cbTotalSize = pThis->cbBacking - pThis->offBootSector;
    if (pThis->cReservedSectors * pThis->cbSector >= pThis->cbTotalSize)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT,
                             "Bogus FAT12/16 total or reserved sector count: %#x vs %#x",
                             pThis->cReservedSectors, pThis->cbTotalSize / pThis->cbSector);

    /*
     * The fat size.  Complete FAT offsets.
     */
    if (   pBootSector->Bpb.Bpb20.cSectorsPerFat == 0
        || ((uint32_t)pBootSector->Bpb.Bpb20.cSectorsPerFat * pThis->cFats + 1) * pThis->cbSector > pThis->cbTotalSize)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT, "Bogus FAT12/16 sectors per FAT: %#x (total sectors %#RX64)",
                             pBootSector->Bpb.Bpb20.cSectorsPerFat, pThis->cbTotalSize / pThis->cbSector);
    pThis->cbFat = pBootSector->Bpb.Bpb20.cSectorsPerFat * pThis->cbSector;

    AssertReturn(pThis->cFats < RT_ELEMENTS(pThis->aoffFats), VERR_VFS_BOGUS_FORMAT);
    for (unsigned iFat = 1; iFat <= pThis->cFats; iFat++)
        pThis->aoffFats[iFat] = pThis->aoffFats[iFat - 1] + pThis->cbFat;

    /*
     * Do root directory calculations.
     */
    pThis->idxRootDirCluster = UINT32_MAX;
    pThis->offRootDir        = pThis->aoffFats[pThis->cFats];
    if (pThis->cRootDirEntries == 0)
        return RTErrInfoSet(pErrInfo, VERR_VFS_BOGUS_FORMAT,  "Zero FAT12/16 root directory size");
    pThis->cbRootDir         = pThis->cRootDirEntries * sizeof(FATDIRENTRY);
    pThis->cbRootDir         = RT_ALIGN_32(pThis->cbRootDir, pThis->cbSector);

    /*
     * First cluster and cluster count checks and calcs.  Determin FAT type.
     */
    pThis->offFirstCluster = pThis->offRootDir + pThis->cbRootDir;
    uint64_t cbSystemStuff = pThis->offFirstCluster - pThis->offBootSector;
    if (cbSystemStuff >= pThis->cbTotalSize)
        return RTErrInfoSet(pErrInfo, VERR_VFS_BOGUS_FORMAT,  "Bogus FAT12/16 total size, root dir, or fat size");
    pThis->cClusters = (pThis->cbTotalSize - cbSystemStuff) / pThis->cbCluster;

    if (pThis->cClusters >= FAT_MAX_FAT16_DATA_CLUSTERS)
    {
        pThis->cClusters  = FAT_MAX_FAT16_DATA_CLUSTERS;
        pThis->enmFatType = RTFSFATTYPE_FAT16;
    }
    else if (pThis->cClusters >= FAT_MIN_FAT16_DATA_CLUSTERS)
        pThis->enmFatType = RTFSFATTYPE_FAT16;
    else
        pThis->enmFatType = RTFSFATTYPE_FAT12; /** @todo Not sure if this is entirely the right way to go about it... */

    uint32_t cClustersPerFat;
    if (pThis->enmFatType == RTFSFATTYPE_FAT16)
        cClustersPerFat = pThis->cbFat / 2;
    else
        cClustersPerFat = pThis->cbFat * 2 / 3;
    if (pThis->cClusters > cClustersPerFat)
        pThis->cClusters = cClustersPerFat;

    return VINF_SUCCESS;
}


/**
 * Worker for rtFsFatVolTryInitDos2Plus and rtFsFatVolTryInitDos2PlusFat32 that
 * handles common extended BPBs fields.
 *
 * @returns IPRT status code.
 * @param   pThis           The FAT volume instance.
 * @param   bExtSignature   The extended BPB signature.
 * @param   uSerialNumber   The serial number.
 * @param   pachLabel       Pointer to the volume label field.
 * @param   pachType        Pointer to the file system type field.
 */
static void rtFsFatVolInitCommonEbpbBits(PRTFSFATVOL pThis, uint8_t bExtSignature, uint32_t uSerialNumber,
                                         char const *pachLabel, char const *pachType)
{
    pThis->uSerialNo = uSerialNumber;
    if (bExtSignature == FATEBPB_SIGNATURE)
    {
        memcpy(pThis->szLabel, pachLabel, RT_SIZEOFMEMB(FATEBPB, achLabel));
        pThis->szLabel[RT_SIZEOFMEMB(FATEBPB, achLabel)] = '\0';
        RTStrStrip(pThis->szLabel);

        memcpy(pThis->szType, pachType, RT_SIZEOFMEMB(FATEBPB, achType));
        pThis->szType[RT_SIZEOFMEMB(FATEBPB, achType)] = '\0';
        RTStrStrip(pThis->szType);
    }
    else
    {
        pThis->szLabel[0] = '\0';
        pThis->szType[0] = '\0';
    }
}


/**
 * Worker for rtFsFatVolTryInitDos2Plus that deals with FAT32.
 *
 * @returns IPRT status code.
 * @param   pThis       The FAT volume instance, BPB derived fields are filled
 *                      in on success.
 * @param   pBootSector The boot sector.
 * @param   pErrInfo    Where to return additional error information.
 */
static int rtFsFatVolTryInitDos2PlusFat32(PRTFSFATVOL pThis, PCFATBOOTSECTOR pBootSector, PRTERRINFO pErrInfo)
{
    pThis->enmFatType    = RTFSFATTYPE_FAT32;
    pThis->enmBpbVersion = pBootSector->Bpb.Fat32Ebpb.bExtSignature == FATEBPB_SIGNATURE
                         ? RTFSFATBPBVER_FAT32_29 : RTFSFATBPBVER_FAT32_28;
    pThis->fFat32Flags   = pBootSector->Bpb.Fat32Ebpb.fFlags;

    if (pBootSector->Bpb.Fat32Ebpb.uVersion != FAT32EBPB_VERSION_0_0)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT, "Unsupported FAT32 version: %d.%d (%#x)",
                             RT_HI_U8(pBootSector->Bpb.Fat32Ebpb.uVersion),  RT_LO_U8(pBootSector->Bpb.Fat32Ebpb.uVersion),
                             pBootSector->Bpb.Fat32Ebpb.uVersion);

    /*
     * Figure total sector count.  We expected it to be filled in.
     */
    bool fUsing64BitTotalSectorCount = false;
    if (pBootSector->Bpb.Fat32Ebpb.Bpb.cTotalSectors16 != 0)
        pThis->cbTotalSize = pBootSector->Bpb.Fat32Ebpb.Bpb.cTotalSectors16 * pThis->cbSector;
    else if (pBootSector->Bpb.Fat32Ebpb.Bpb.cTotalSectors32 != 0)
        pThis->cbTotalSize = pBootSector->Bpb.Fat32Ebpb.Bpb.cTotalSectors32 * (uint64_t)pThis->cbSector;
    else if (   pBootSector->Bpb.Fat32Ebpb.u.cTotalSectors64 <= UINT64_MAX / 512
             && pBootSector->Bpb.Fat32Ebpb.u.cTotalSectors64 > 3
             && pBootSector->Bpb.Fat32Ebpb.bExtSignature != FATEBPB_SIGNATURE_OLD)
    {
        pThis->cbTotalSize = pBootSector->Bpb.Fat32Ebpb.u.cTotalSectors64 * pThis->cbSector;
        fUsing64BitTotalSectorCount = true;
    }
    else
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT, "FAT32 total sector count out of range: %#RX64",
                             pBootSector->Bpb.Fat32Ebpb.u.cTotalSectors64);
    if (pThis->cReservedSectors * pThis->cbSector >= pThis->cbTotalSize)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT,
                             "Bogus FAT32 total or reserved sector count: %#x vs %#x",
                             pThis->cReservedSectors, pThis->cbTotalSize / pThis->cbSector);

    /*
     * Fat size. We check the 16-bit field even if it probably should be zero all the time.
     */
    if (pBootSector->Bpb.Fat32Ebpb.Bpb.cSectorsPerFat != 0)
    {
        if (   pBootSector->Bpb.Fat32Ebpb.cSectorsPerFat32 != 0
            && pBootSector->Bpb.Fat32Ebpb.cSectorsPerFat32 != pBootSector->Bpb.Fat32Ebpb.Bpb.cSectorsPerFat)
            return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT,
                                 "Both 16-bit and 32-bit FAT size fields are set: %#RX16 vs %#RX32",
                                 pBootSector->Bpb.Fat32Ebpb.Bpb.cSectorsPerFat, pBootSector->Bpb.Fat32Ebpb.cSectorsPerFat32);
        pThis->cbFat = pBootSector->Bpb.Fat32Ebpb.Bpb.cSectorsPerFat * pThis->cbSector;
    }
    else
    {
        uint64_t cbFat = pBootSector->Bpb.Fat32Ebpb.cSectorsPerFat32 * (uint64_t)pThis->cbSector;
        if (   cbFat == 0
            || cbFat >= FAT_MAX_FAT32_TOTAL_CLUSTERS * 4 + pThis->cbSector * 16)
            return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT,
                                 "Bogus 32-bit FAT size: %#RX32", pBootSector->Bpb.Fat32Ebpb.cSectorsPerFat32);
        pThis->cbFat = (uint32_t)cbFat;
    }

    /*
     * Complete the FAT offsets and first cluster offset, then calculate number
     * of data clusters.
     */
    AssertReturn(pThis->cFats < RT_ELEMENTS(pThis->aoffFats), VERR_VFS_BOGUS_FORMAT);
    for (unsigned iFat = 1; iFat <= pThis->cFats; iFat++)
        pThis->aoffFats[iFat] = pThis->aoffFats[iFat - 1] + pThis->cbFat;
    pThis->offFirstCluster = pThis->aoffFats[pThis->cFats];

    if (pThis->offFirstCluster - pThis->offBootSector >= pThis->cbTotalSize)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT,
                             "Bogus 32-bit FAT size or total sector count: cFats=%d cbFat=%#x cbTotalSize=%#x",
                             pThis->cFats, pThis->cbFat, pThis->cbTotalSize);

    uint64_t cClusters = (pThis->cbTotalSize - (pThis->offFirstCluster - pThis->offBootSector)) / pThis->cbCluster;
    if (cClusters <= FAT_MAX_FAT32_DATA_CLUSTERS)
        pThis->cClusters = (uint32_t)cClusters;
    else
        pThis->cClusters = FAT_MAX_FAT32_DATA_CLUSTERS;
    if (pThis->cClusters > (pThis->cbFat / 4 - FAT_FIRST_DATA_CLUSTER))
        pThis->cClusters = (pThis->cbFat / 4 - FAT_FIRST_DATA_CLUSTER);

    /*
     * Root dir cluster.
     */
    if (   pBootSector->Bpb.Fat32Ebpb.uRootDirCluster < FAT_FIRST_DATA_CLUSTER
        || pBootSector->Bpb.Fat32Ebpb.uRootDirCluster >= pThis->cClusters)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT,
                             "Bogus FAT32 root directory cluster: %#x", pBootSector->Bpb.Fat32Ebpb.uRootDirCluster);
    pThis->idxRootDirCluster = pBootSector->Bpb.Fat32Ebpb.uRootDirCluster;
    pThis->offRootDir        = pThis->offFirstCluster
                             + (pBootSector->Bpb.Fat32Ebpb.uRootDirCluster - FAT_FIRST_DATA_CLUSTER) * pThis->cbCluster;

    /*
     * Info sector.
     */
    if (   pBootSector->Bpb.Fat32Ebpb.uInfoSectorNo == 0
        || pBootSector->Bpb.Fat32Ebpb.uInfoSectorNo == UINT16_MAX)
        pThis->offFat32InfoSector = UINT64_MAX;
    else if (pBootSector->Bpb.Fat32Ebpb.uInfoSectorNo >= pThis->cReservedSectors)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT,
                             "Bogus FAT32 info sector number: %#x (reserved sectors %#x)",
                             pBootSector->Bpb.Fat32Ebpb.uInfoSectorNo, pThis->cReservedSectors);
    else
    {
        pThis->offFat32InfoSector = pThis->cbSector * pBootSector->Bpb.Fat32Ebpb.uInfoSectorNo + pThis->offBootSector;
        int rc = RTVfsFileReadAt(pThis->hVfsBacking, pThis->offFat32InfoSector,
                                 &pThis->Fat32InfoSector, sizeof(pThis->Fat32InfoSector), NULL);
        if (RT_FAILURE(rc))
            return RTErrInfoSetF(pErrInfo, rc, "Failed to read FAT32 info sector at offset %#RX64", pThis->offFat32InfoSector);
        if (   pThis->Fat32InfoSector.uSignature1 != FAT32INFOSECTOR_SIGNATURE_1
            || pThis->Fat32InfoSector.uSignature2 != FAT32INFOSECTOR_SIGNATURE_2
            || pThis->Fat32InfoSector.uSignature3 != FAT32INFOSECTOR_SIGNATURE_3)
            return RTErrInfoSetF(pErrInfo, rc, "FAT32 info sector signature mismatch: %#x %#x %#x",
                                 pThis->Fat32InfoSector.uSignature1,  pThis->Fat32InfoSector.uSignature2,
                                 pThis->Fat32InfoSector.uSignature3);
    }

    /*
     * Boot sector copy.
     */
    if (   pBootSector->Bpb.Fat32Ebpb.uBootSectorCopySectorNo == 0
        || pBootSector->Bpb.Fat32Ebpb.uBootSectorCopySectorNo == UINT16_MAX)
    {
        pThis->cBootSectorCopies   = 0;
        pThis->offBootSectorCopies = UINT64_MAX;
    }
    else if (pBootSector->Bpb.Fat32Ebpb.uBootSectorCopySectorNo >= pThis->cReservedSectors)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT,
                             "Bogus FAT32 info boot sector copy location: %#x (reserved sectors %#x)",
                             pBootSector->Bpb.Fat32Ebpb.uBootSectorCopySectorNo, pThis->cReservedSectors);
    else
    {
        /** @todo not sure if cbSector is correct here. */
        pThis->cBootSectorCopies = 3;
        if (  (uint32_t)pBootSector->Bpb.Fat32Ebpb.uBootSectorCopySectorNo + pThis->cBootSectorCopies
            > pThis->cReservedSectors)
            pThis->cBootSectorCopies = (uint8_t)(pThis->cReservedSectors - pBootSector->Bpb.Fat32Ebpb.uBootSectorCopySectorNo);
        pThis->offBootSectorCopies = pBootSector->Bpb.Fat32Ebpb.uBootSectorCopySectorNo * pThis->cbSector + pThis->offBootSector;
        if (   pThis->offFat32InfoSector != UINT64_MAX
            && pThis->offFat32InfoSector - pThis->offBootSectorCopies < (uint64_t)(pThis->cBootSectorCopies * pThis->cbSector))
            return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT, "FAT32 info sector and boot sector copies overlap: %#x vs %#x",
                                 pBootSector->Bpb.Fat32Ebpb.uInfoSectorNo, pBootSector->Bpb.Fat32Ebpb.uBootSectorCopySectorNo);
    }

    /*
     * Serial number, label and type.
     */
    rtFsFatVolInitCommonEbpbBits(pThis, pBootSector->Bpb.Fat32Ebpb.bExtSignature, pBootSector->Bpb.Fat32Ebpb.uSerialNumber,
                                 pBootSector->Bpb.Fat32Ebpb.achLabel,
                                 fUsing64BitTotalSectorCount ? pBootSector->achOemName : pBootSector->Bpb.Fat32Ebpb.achLabel);
    if (pThis->szType[0] == '\0')
        memcpy(pThis->szType, "FAT32", 6);

    return VINF_SUCCESS;
}


/**
 * Tries to detect a DOS 2.0+ formatted image and fills in the BPB fields.
 *
 * We ASSUME BPB here, but need to figure out which version of the BPB it is,
 * which is lots of fun.
 *
 * @returns IPRT status code.
 * @param   pThis       The FAT volume instance, BPB derived fields are filled
 *                      in on success.
 * @param   pBootSector The boot sector.
 * @param   pbFatSector Points to the FAT sector, or whatever is 512 bytes after
 *                      the boot sector.  On successful return it will contain
 *                      the first FAT sector.
 * @param   pErrInfo    Where to return additional error information.
 */
static int rtFsFatVolTryInitDos2Plus(PRTFSFATVOL pThis, PCFATBOOTSECTOR pBootSector, uint8_t *pbFatSector, PRTERRINFO pErrInfo)
{
    /*
     * Check if we've got a known jump instruction first, because that will
     * give us a max (E)BPB size hint.
     */
    uint8_t offJmp = UINT8_MAX;
    if (   pBootSector->abJmp[0] == 0xeb
        && pBootSector->abJmp[1] <= 0x7f)
        offJmp = pBootSector->abJmp[1] + 2;
    else if (   pBootSector->abJmp[0] == 0x90
             && pBootSector->abJmp[1] == 0xeb
             && pBootSector->abJmp[2] <= 0x7f)
        offJmp = pBootSector->abJmp[2] + 3;
    else if (   pBootSector->abJmp[0] == 0xe9
             && pBootSector->abJmp[2] <= 0x7f)
        offJmp = RT_MIN(127, RT_MAKE_U16(pBootSector->abJmp[1], pBootSector->abJmp[2]));
    uint8_t const cbMaxBpb = offJmp - RT_OFFSETOF(FATBOOTSECTOR, Bpb);

    /*
     * Do the basic DOS v2.0 BPB fields.
     */
    if (cbMaxBpb < sizeof(FATBPB20))
        return RTErrInfoSetF(pErrInfo, VERR_VFS_UNKNOWN_FORMAT,
                             "DOS signature, but jmp too short for any BPB: %#x (max %#x BPB)", offJmp, cbMaxBpb);

    if (pBootSector->Bpb.Bpb20.cFats == 0)
        return RTErrInfoSet(pErrInfo, VERR_VFS_UNKNOWN_FORMAT, "DOS signature, number of FATs is zero, so not FAT file system");
    if (pBootSector->Bpb.Bpb20.cFats > 4)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT, "DOS signature, too many FATs: %#x", pBootSector->Bpb.Bpb20.cFats);
    pThis->cFats = pBootSector->Bpb.Bpb20.cFats;

    if (!FATBPB_MEDIA_IS_VALID(pBootSector->Bpb.Bpb20.bMedia))
        return RTErrInfoSetF(pErrInfo, VERR_VFS_UNKNOWN_FORMAT,
                             "DOS signature, invalid media byte: %#x", pBootSector->Bpb.Bpb20.bMedia);
    pThis->bMedia = pBootSector->Bpb.Bpb20.bMedia;

    if (!RT_IS_POWER_OF_TWO(pBootSector->Bpb.Bpb20.cbSector))
        return RTErrInfoSetF(pErrInfo, VERR_VFS_UNKNOWN_FORMAT,
                             "DOS signature, sector size not power of two: %#x", pBootSector->Bpb.Bpb20.cbSector);
    if (   pBootSector->Bpb.Bpb20.cbSector != 512
        && pBootSector->Bpb.Bpb20.cbSector != 4096
        && pBootSector->Bpb.Bpb20.cbSector != 1024
        && pBootSector->Bpb.Bpb20.cbSector != 128)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_UNKNOWN_FORMAT,
                             "DOS signature, unsupported sector size: %#x", pBootSector->Bpb.Bpb20.cbSector);
    pThis->cbSector = pBootSector->Bpb.Bpb20.cbSector;

    if (   !RT_IS_POWER_OF_TWO(pBootSector->Bpb.Bpb20.cSectorsPerCluster)
        || !pBootSector->Bpb.Bpb20.cSectorsPerCluster)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_UNKNOWN_FORMAT, "DOS signature, cluster size not non-zero power of two: %#x",
                             pBootSector->Bpb.Bpb20.cSectorsPerCluster);
    pThis->cbCluster = pBootSector->Bpb.Bpb20.cSectorsPerCluster * pThis->cbSector;

    uint64_t const cMaxRoot = (pThis->cbBacking - pThis->offBootSector - 512) / sizeof(FATDIRENTRY); /* we'll check again later. */
    if (pBootSector->Bpb.Bpb20.cMaxRootDirEntries >= cMaxRoot)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT, "DOS signature, too many root entries: %#x (max %#RX64)",
                             pBootSector->Bpb.Bpb20.cSectorsPerCluster, cMaxRoot);
    pThis->cRootDirEntries = pBootSector->Bpb.Bpb20.cMaxRootDirEntries;

    if (   pBootSector->Bpb.Bpb20.cReservedSectors == 0
        || pBootSector->Bpb.Bpb20.cReservedSectors >= _32K)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT,
                             "DOS signature, bogus reserved sector count: %#x", pBootSector->Bpb.Bpb20.cReservedSectors);
    pThis->cReservedSectors = pBootSector->Bpb.Bpb20.cReservedSectors;
    pThis->aoffFats[0]      = pThis->offBootSector + pThis->cReservedSectors * pThis->cbSector;

    /*
     * Jump ahead and check for FAT32 EBPB.
     * If found, we simply ASSUME it's a FAT32 file system.
     */
    int rc;
    if (   (   sizeof(FAT32EBPB) <= cbMaxBpb
            && pBootSector->Bpb.Fat32Ebpb.bExtSignature == FATEBPB_SIGNATURE)
        || (   RT_OFFSETOF(FAT32EBPB, achLabel) <= cbMaxBpb
            && pBootSector->Bpb.Fat32Ebpb.bExtSignature == FATEBPB_SIGNATURE_OLD) )
    {
        rc = rtFsFatVolTryInitDos2PlusFat32(pThis, pBootSector, pErrInfo);
        if (RT_FAILURE(rc))
            return rc;
    }
    else
    {
        /*
         * Check for extended BPB, otherwise we'll have to make qualified guesses
         * about what kind of BPB we're up against based on jmp offset and zero fields.
         * ASSUMES either FAT16 or FAT12.
         */
        if (   (   sizeof(FATEBPB) <= cbMaxBpb
                && pBootSector->Bpb.Ebpb.bExtSignature == FATEBPB_SIGNATURE)
            || (   RT_OFFSETOF(FATEBPB, achLabel) <= cbMaxBpb
                && pBootSector->Bpb.Ebpb.bExtSignature == FATEBPB_SIGNATURE_OLD) )
        {
            rtFsFatVolInitCommonEbpbBits(pThis, pBootSector->Bpb.Ebpb.bExtSignature, pBootSector->Bpb.Ebpb.uSerialNumber,
                                         pBootSector->Bpb.Ebpb.achLabel, pBootSector->Bpb.Ebpb.achType);
            rc = rtFsFatVolTryInitDos2PlusBpb(pThis, pBootSector, true /*fMaybe331*/, pErrInfo);
            pThis->enmBpbVersion = pBootSector->Bpb.Ebpb.bExtSignature == FATEBPB_SIGNATURE
                                 ? RTFSFATBPBVER_EXT_29 : RTFSFATBPBVER_EXT_28;
        }
        else
            rc = rtFsFatVolTryInitDos2PlusBpb(pThis, pBootSector, cbMaxBpb >= sizeof(FATBPB331), pErrInfo);
        if (RT_FAILURE(rc))
            return rc;
        if (pThis->szType[0] == '\0')
            memcpy(pThis->szType, pThis->enmFatType == RTFSFATTYPE_FAT12 ? "FAT12" : "FAT16", 6);
    }

    /*
     * Check the FAT ID. May have to read a bit of the FAT into the buffer.
     */
    if (pThis->aoffFats[0] != pThis->offBootSector + 512)
    {
        rc = RTVfsFileReadAt(pThis->hVfsBacking, pThis->aoffFats[0], pbFatSector, 512, NULL);
        if (RT_FAILURE(rc))
            return RTErrInfoSet(pErrInfo, rc, "error reading first FAT sector");
    }
    if (pbFatSector[0] != pThis->bMedia)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT,
                             "Media byte and FAT ID mismatch: %#x vs %#x (%.7Rhxs)", pbFatSector[0], pThis->bMedia, pbFatSector);
    switch (pThis->enmFatType)
    {
        case RTFSFATTYPE_FAT12:
            if ((pbFatSector[1] & 0xf) != 0xf)
                return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT, "Bogus FAT ID patting (FAT12): %.3Rhxs", pbFatSector);
            pThis->idxMaxLastCluster = FAT_LAST_FAT12_DATA_CLUSTER;
            pThis->idxEndOfChain     = (pbFatSector[1] >> 4) | ((uint32_t)pbFatSector[2] << 4);
            break;

        case RTFSFATTYPE_FAT16:
            if (pbFatSector[1] != 0xff)
                return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT, "Bogus FAT ID patting (FAT16): %.4Rhxs", pbFatSector);
            pThis->idxMaxLastCluster = FAT_LAST_FAT16_DATA_CLUSTER;
            pThis->idxEndOfChain     = RT_MAKE_U16(pbFatSector[2], pbFatSector[3]);
            break;

        case RTFSFATTYPE_FAT32:
            if (   pbFatSector[1] != 0xff
                || pbFatSector[2] != 0xff
                || (pbFatSector[3] & 0x0f) != 0x0f)
                return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT, "Bogus FAT ID patting (FAT32): %.8Rhxs", pbFatSector);
            pThis->idxMaxLastCluster = FAT_LAST_FAT32_DATA_CLUSTER;
            pThis->idxEndOfChain     = RT_MAKE_U32_FROM_U8(pbFatSector[4], pbFatSector[5], pbFatSector[6], pbFatSector[7]);
            break;

        default: AssertFailedReturn(VERR_INTERNAL_ERROR_2);
    }
    if (pThis->idxEndOfChain <= pThis->idxMaxLastCluster)
        return RTErrInfoSetF(pErrInfo, VERR_VFS_BOGUS_FORMAT, "Bogus formatter end-of-chain value: %#x, must be above %#x",
                             pThis->idxEndOfChain, pThis->idxMaxLastCluster);

    RT_NOREF(pbFatSector);
    return VINF_SUCCESS;
}


/**
 * Given a power of two value @a cb return exponent value.
 *
 * @returns Shift count
 * @param   cb              The value.
 */
static uint8_t rtFsFatVolCalcByteShiftCount(uint32_t cb)
{
    Assert(RT_IS_POWER_OF_TWO(cb));
    unsigned iBit = ASMBitFirstSetU32(cb);
    Assert(iBit >= 1);
    iBit--;
    return iBit;
}


/**
 * Worker for RTFsFatVolOpen.
 *
 * @returns IPRT status code.
 * @param   pThis           The FAT VFS instance to initialize.
 * @param   hVfsSelf        The FAT VFS handle (no reference consumed).
 * @param   hVfsBacking     The file backing the alleged FAT file system.
 *                          Reference is consumed (via rtFsFatVol_Destroy).
 * @param   fReadOnly       Readonly or readwrite mount.
 * @param   offBootSector   The boot sector offset in bytes.
 * @param   pErrInfo        Where to return additional error info.  Can be NULL.
 */
static int rtFsFatVolTryInit(PRTFSFATVOL pThis, RTVFS hVfsSelf, RTVFSFILE hVfsBacking,
                             bool fReadOnly, uint64_t offBootSector, PRTERRINFO pErrInfo)
{
    /*
     * First initialize the state so that rtFsFatVol_Destroy won't trip up.
     */
    pThis->hVfsSelf             = hVfsSelf;
    pThis->hVfsBacking          = hVfsBacking; /* Caller referenced it for us, we consume it; rtFsFatVol_Destroy releases it. */
    pThis->cbBacking            = 0;
    pThis->offBootSector        = offBootSector;
    pThis->offNanoUTC           = RTTimeLocalDeltaNano();
    pThis->offMinUTC            = pThis->offNanoUTC / RT_NS_1MIN;
    pThis->fReadOnly            = fReadOnly;
    pThis->cReservedSectors     = 1;

    pThis->cbSector             = 512;
    pThis->cbCluster            = 512;
    pThis->cClusters            = 0;
    pThis->offFirstCluster      = 0;
    pThis->cbTotalSize          = 0;

    pThis->enmFatType           = RTFSFATTYPE_INVALID;
    pThis->cFatEntries          = 0;
    pThis->cFats                = 0;
    pThis->cbFat                = 0;
    for (unsigned i = 0; i < RT_ELEMENTS(pThis->aoffFats); i++)
        pThis->aoffFats[i]      = UINT64_MAX;
    pThis->pFatCache            = NULL;

    pThis->offRootDir           = UINT64_MAX;
    pThis->idxRootDirCluster    = UINT32_MAX;
    pThis->cRootDirEntries      = UINT32_MAX;
    pThis->cbRootDir            = 0;
    pThis->hVfsRootDir          = NIL_RTVFSDIR;
    pThis->pRootDir             = NULL;

    pThis->uSerialNo            = 0;
    pThis->szLabel[0]           = '\0';
    pThis->szType[0]            = '\0';
    pThis->cBootSectorCopies    = 0;
    pThis->fFat32Flags          = 0;
    pThis->offBootSectorCopies  = UINT64_MAX;
    pThis->offFat32InfoSector   = UINT64_MAX;
    RT_ZERO(pThis->Fat32InfoSector);

    /*
     * Get stuff that may fail.
     */
    int rc = RTVfsFileGetSize(hVfsBacking, &pThis->cbBacking);
    if (RT_FAILURE(rc))
        return rc;
    pThis->cbTotalSize = pThis->cbBacking - pThis->offBootSector;

    /*
     * Read the boot sector and the following sector (start of the allocation
     * table unless it a FAT32 FS).  We'll then validate the boot sector and
     * start of the FAT, expanding the BPB into the instance data.
     */
    union
    {
        uint8_t             ab[512*2];
        uint16_t            au16[512*2 / 2];
        uint32_t            au32[512*2 / 4];
        FATBOOTSECTOR       BootSector;
        FAT32INFOSECTOR     InfoSector;
    } Buf;
    RT_ZERO(Buf);

    rc = RTVfsFileReadAt(hVfsBacking, offBootSector, &Buf.BootSector, 512 * 2, NULL);
    if (RT_FAILURE(rc))
        return RTErrInfoSet(pErrInfo, rc, "Unable to read bootsect");

    /*
     * Extract info from the BPB and validate the two special FAT entries.
     *
     * Check the DOS signature first.  The PC-DOS 1.0 boot floppy does not have
     * a signature and we ASSUME this is the case for all flopies formated by it.
     */
    if (Buf.BootSector.uSignature != FATBOOTSECTOR_SIGNATURE)
    {
        if (Buf.BootSector.uSignature != 0)
            return RTErrInfoSetF(pErrInfo, rc, "No DOS bootsector signature: %#06x", Buf.BootSector.uSignature);
        rc = rtFsFatVolTryInitDos1x(pThis, &Buf.BootSector, &Buf.ab[512], pErrInfo);
    }
    else
        rc = rtFsFatVolTryInitDos2Plus(pThis, &Buf.BootSector, &Buf.ab[512], pErrInfo);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Calc shift counts.
     */
    pThis->cSectorByteShift  = rtFsFatVolCalcByteShiftCount(pThis->cbSector);
    pThis->cClusterByteShift = rtFsFatVolCalcByteShiftCount(pThis->cbCluster);

    /*
     * Setup the FAT cache.
     */
    rc = rtFsFatClusterMap_Create(pThis, &Buf.ab[512], pErrInfo);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Create the root directory fun.
     */
    if (pThis->idxRootDirCluster == UINT32_MAX)
        rc = rtFsFatDir_New(pThis, NULL /*pParentDir*/, NULL /*pDirEntry*/, UINT32_MAX /*offEntryInDir*/,
                            UINT32_MAX, pThis->offRootDir, pThis->cbRootDir,
                            &pThis->hVfsRootDir, &pThis->pRootDir);
    else
        rc = rtFsFatDir_New(pThis, NULL /*pParentDir*/, NULL /*pDirEntry*/, UINT32_MAX /*offEntryInDir*/,
                            pThis->idxRootDirCluster, UINT64_MAX, pThis->cbRootDir,
                            &pThis->hVfsRootDir, &pThis->pRootDir);
    return rc;
}


/**
 * Opens a FAT file system volume.
 *
 * @returns IPRT status code.
 * @param   hVfsFileIn      The file or device backing the volume.
 * @param   fReadOnly       Whether to mount it read-only.
 * @param   offBootSector   The offset of the boot sector relative to the start
 *                          of @a hVfsFileIn.  Pass 0 for floppies.
 * @param   phVfs           Where to return the virtual file system handle.
 * @param   pErrInfo        Where to return additional error information.
 */
RTDECL(int) RTFsFatVolOpen(RTVFSFILE hVfsFileIn, bool fReadOnly, uint64_t offBootSector, PRTVFS phVfs, PRTERRINFO pErrInfo)
{
    /*
     * Quick input validation.
     */
    AssertPtrReturn(phVfs, VERR_INVALID_POINTER);
    *phVfs = NIL_RTVFS;

    uint32_t cRefs = RTVfsFileRetain(hVfsFileIn);
    AssertReturn(cRefs != UINT32_MAX, VERR_INVALID_HANDLE);

    /*
     * Create a new FAT VFS instance and try initialize it using the given input file.
     */
    RTVFS hVfs   = NIL_RTVFS;
    void *pvThis = NULL;
    int rc = RTVfsNew(&g_rtFsFatVolOps, sizeof(RTFSFATVOL), NIL_RTVFS, RTVFSLOCK_CREATE_RW, &hVfs, &pvThis);
    if (RT_SUCCESS(rc))
    {
        rc = rtFsFatVolTryInit((PRTFSFATVOL)pvThis, hVfs, hVfsFileIn, fReadOnly, offBootSector, pErrInfo);
        if (RT_SUCCESS(rc))
            *phVfs = hVfs;
        else
            RTVfsRelease(hVfs);
    }
    else
        RTVfsFileRelease(hVfsFileIn);
    return rc;
}




/**
 * Fills a range in the file with zeros in the most efficient manner.
 *
 * @returns IPRT status code.
 * @param   hVfsFile            The file to write to.
 * @param   off                 Where to start filling with zeros.
 * @param   cbZeros             How many zero blocks to write.
 */
static int rtFsFatVolWriteZeros(RTVFSFILE hVfsFile, uint64_t off, uint32_t cbZeros)
{
    while (cbZeros > 0)
    {
        uint32_t cbToWrite = sizeof(g_abRTZero64K);
        if (cbToWrite > cbZeros)
            cbToWrite = cbZeros;
        int rc = RTVfsFileWriteAt(hVfsFile, off, g_abRTZero64K, cbToWrite, NULL);
        if (RT_FAILURE(rc))
            return rc;
        off     += cbToWrite;
        cbZeros -= cbToWrite;
    }
    return VINF_SUCCESS;
}


/**
 * Formats a FAT volume.
 *
 * @returns IRPT status code.
 * @param   hVfsFile            The volume file.
 * @param   offVol              The offset into @a hVfsFile of the file.
 *                              Typically 0.
 * @param   cbVol               The size of the volume.  Pass 0 if the rest of
 *                              hVfsFile should be used.
 * @param   fFlags              See RTFSFATVOL_FMT_F_XXX.
 * @param   cbSector            The logical sector size.  Must be power of two.
 *                              Optional, pass zero to use 512.
 * @param   cSectorsPerCluster  Number of sectors per cluster.  Power of two.
 *                              Optional, pass zero to auto detect.
 * @param   enmFatType          The FAT type (12, 16, 32) to use.
 *                              Optional, pass RTFSFATTYPE_INVALID for default.
 * @param   cHeads              The number of heads to report in the BPB.
 *                              Optional, pass zero to auto detect.
 * @param   cSectorsPerTrack    The number of sectors per track to put in the
 *                              BPB. Optional, pass zero to auto detect.
 * @param   bMedia              The media byte value and FAT ID to use.
 *                              Optional, pass zero to auto detect.
 * @param   cRootDirEntries     Number of root directory entries.
 *                              Optional, pass zero to auto detect.
 * @param   cHiddenSectors      Number of hidden sectors.  Pass 0 for
 *                              unpartitioned media.
 * @param   pErrInfo            Additional error information, maybe.  Optional.
 */
RTDECL(int) RTFsFatVolFormat(RTVFSFILE hVfsFile, uint64_t offVol, uint64_t cbVol, uint32_t fFlags, uint16_t cbSector,
                             uint16_t cSectorsPerCluster, RTFSFATTYPE enmFatType, uint32_t cHeads, uint32_t cSectorsPerTrack,
                             uint8_t bMedia, uint16_t cRootDirEntries, uint32_t cHiddenSectors, PRTERRINFO pErrInfo)
{
    int         rc;
    uint32_t    cFats = 2;

    /*
     * Validate input.
     */
    if (!cbSector)
        cbSector = 512;
    else
        AssertMsgReturn(   cbSector == 128
                        || cbSector == 512
                        || cbSector == 1024
                        || cbSector == 4096,
                        ("cbSector=%#x\n", cbSector),
                        VERR_INVALID_PARAMETER);
    AssertMsgReturn(cSectorsPerCluster == 0 || (cSectorsPerCluster <= 128 && RT_IS_POWER_OF_TWO(cSectorsPerCluster)),
                    ("cSectorsPerCluster=%#x\n", cSectorsPerCluster), VERR_INVALID_PARAMETER);
    if (bMedia != 0)
    {
        AssertMsgReturn(FAT_ID_IS_VALID(bMedia),       ("bMedia=%#x\n", bMedia), VERR_INVALID_PARAMETER);
        AssertMsgReturn(FATBPB_MEDIA_IS_VALID(bMedia), ("bMedia=%#x\n", bMedia), VERR_INVALID_PARAMETER);
    }
    AssertReturn(!(fFlags & ~RTFSFATVOL_FMT_F_VALID_MASK), VERR_INVALID_FLAGS);
    AssertReturn(enmFatType >= RTFSFATTYPE_INVALID && enmFatType < RTFSFATTYPE_END, VERR_INVALID_PARAMETER);

    if (!cbVol)
    {
        uint64_t cbFile;
        rc = RTVfsFileGetSize(hVfsFile, &cbFile);
        AssertRCReturn(rc, rc);
        AssertMsgReturn(cbFile > offVol, ("cbFile=%#RX64 offVol=%#RX64\n", cbFile, offVol), VERR_INVALID_PARAMETER);
        cbVol = cbFile - offVol;
    }
    uint64_t const cSectorsInVol = cbVol / cbSector;

    /*
     * Guess defaults if necessary.
     */
    if (!cSectorsPerCluster || !cHeads || !cSectorsPerTrack || !bMedia || !cRootDirEntries)
    {
        static struct
        {
            uint64_t    cbVol;
            uint8_t     bMedia;
            uint8_t     cHeads;
            uint8_t     cSectorsPerTrack;
            uint8_t     cSectorsPerCluster;
            uint16_t    cRootDirEntries;
        } s_aDefaults[] =
        {
            /*                  cbVol, bMedia,              cHeads, cSectorsPTrk, cSectorsPClstr, cRootDirEntries */
            {                 163840,    0xfe, /* cyl:   40,*/   1,          8,                1,           64 },
            {                 184320,    0xfc, /* cyl:   40,*/   1,          9,                2,           64 },
            {                 327680,    0xff, /* cyl:   40,*/   2,          8,                2,          112 },
            {                 368640,    0xfd, /* cyl:   40,*/   2,          9,                2,          112 },
            {                 737280,    0xf9, /* cyl:   80,*/   2,          9,                2,          112 },
            {                1228800,    0xf9, /* cyl:   80,*/   2,         15,                2,          112 },
            {                1474560,    0xf0, /* cyl:   80,*/   2,         18,                1,          224 },
            {                2949120,    0xf0, /* cyl:   80,*/   2,         36,                2,          224 },
            {              528482304,    0xf8, /* cyl: 1024,*/  16,         63,                0,          512 }, // 504MB limit
            {   UINT64_C(7927234560),    0xf8, /* cyl: 1024,*/ 240,         63,                0,          512 }, // 7.3GB limit
            {   UINT64_C(8422686720),    0xf8, /* cyl: 1024,*/ 255,         63,                0,          512 }, // 7.84GB limit

        };
        uint32_t iDefault = 0;
        while (   iDefault < RT_ELEMENTS(s_aDefaults) - 1U
               && cbVol > s_aDefaults[iDefault].cbVol)
            iDefault++;
        if (!cHeads)
            cHeads              = s_aDefaults[iDefault].cHeads;
        if (!cSectorsPerTrack)
            cSectorsPerTrack    = s_aDefaults[iDefault].cSectorsPerTrack;
        if (!bMedia)
            bMedia              = s_aDefaults[iDefault].bMedia;
        if (!cRootDirEntries)
            cRootDirEntries     = s_aDefaults[iDefault].cRootDirEntries;
        if (!cSectorsPerCluster)
        {
            cSectorsPerCluster  = s_aDefaults[iDefault].cSectorsPerCluster;
            if (!cSectorsPerCluster)
            {
                uint32_t cbFat12Overhead = cbSector  /* boot sector */
                                         + RT_ALIGN_32(FAT_MAX_FAT12_TOTAL_CLUSTERS * 3 / 2, cbSector) * cFats /* FATs */
                                         + RT_ALIGN_32(cRootDirEntries * sizeof(FATDIRENTRY), cbSector) /* root dir */;
                uint32_t cbFat16Overhead = cbSector  /* boot sector */
                                         + RT_ALIGN_32(FAT_MAX_FAT16_TOTAL_CLUSTERS * 2, cbSector) * cFats /* FATs */
                                         + RT_ALIGN_32(cRootDirEntries * sizeof(FATDIRENTRY), cbSector) /* root dir */;

                if (   enmFatType == RTFSFATTYPE_FAT12
                    || cbVol <= cbFat12Overhead + FAT_MAX_FAT12_DATA_CLUSTERS * 4 * cbSector)
                {
                    enmFatType = RTFSFATTYPE_FAT12;
                    cSectorsPerCluster = 1;
                    while (   cSectorsPerCluster < 128
                           &&   cSectorsInVol
                              >   cbFat12Overhead / cbSector
                                + (uint32_t)cSectorsPerCluster * FAT_MAX_FAT12_DATA_CLUSTERS
                                + cSectorsPerCluster - 1)
                        cSectorsPerCluster <<= 1;
                }
                else if (   enmFatType == RTFSFATTYPE_FAT16
                         || cbVol <= cbFat16Overhead + FAT_MAX_FAT16_DATA_CLUSTERS * 128 * cbSector)
                {
                    enmFatType = RTFSFATTYPE_FAT16;
                    cSectorsPerCluster = 1;
                    while (   cSectorsPerCluster < 128
                           &&   cSectorsInVol
                              >   cbFat12Overhead / cbSector
                                + (uint32_t)cSectorsPerCluster * FAT_MAX_FAT16_DATA_CLUSTERS
                                + cSectorsPerCluster - 1)
                        cSectorsPerCluster <<= 1;
                }
                else
                {
                    /* The target here is keeping the FAT size below 8MB.  Seems windows
                       likes a minimum 4KB cluster size as wells as a max of 32KB (googling). */
                    enmFatType = RTFSFATTYPE_FAT32;
                    uint32_t cbFat32Overhead = cbSector * 32 /* boot sector, info sector, boot sector copies, reserved sectors */
                                             + _8M * cFats;
                    if (cbSector >= _4K)
                        cSectorsPerCluster = 1;
                    else
                        cSectorsPerCluster = _4K / cbSector;
                    while (   cSectorsPerCluster < 128
                           && cSectorsPerCluster * cbSector < _32K
                           && cSectorsInVol > cbFat32Overhead / cbSector + (uint64_t)cSectorsPerCluster * _2M)
                        cSectorsPerCluster <<= 1;
                }
            }
        }
    }
    Assert(cSectorsPerCluster);
    Assert(cRootDirEntries);
    uint32_t       cbRootDir    = RT_ALIGN_32(cRootDirEntries * sizeof(FATDIRENTRY), cbSector);
    uint32_t const cbCluster    = cSectorsPerCluster * cbSector;

    /*
     * If we haven't figured out the FAT type yet, do so.
     * The file system code determins the FAT based on cluster counts,
     * so we must do so here too.
     */
    if (enmFatType == RTFSFATTYPE_INVALID)
    {
        uint32_t cbFat12Overhead = cbSector  /* boot sector */
                                 + RT_ALIGN_32(FAT_MAX_FAT12_TOTAL_CLUSTERS * 3 / 2, cbSector) * cFats /* FATs */
                                 + RT_ALIGN_32(cRootDirEntries * sizeof(FATDIRENTRY), cbSector) /* root dir */;
        if (   cbVol <= cbFat12Overhead + cbCluster
            || (cbVol - cbFat12Overhead) / cbCluster <= FAT_MAX_FAT12_DATA_CLUSTERS)
            enmFatType = RTFSFATTYPE_FAT12;
        else
        {
            uint32_t cbFat16Overhead = cbSector  /* boot sector */
                                     + RT_ALIGN_32(FAT_MAX_FAT16_TOTAL_CLUSTERS * 2, cbSector) * cFats /* FATs */
                                     + cbRootDir;
            if (   cbVol <= cbFat16Overhead + cbCluster
                || (cbVol - cbFat16Overhead) / cbCluster <= FAT_MAX_FAT16_DATA_CLUSTERS)
                enmFatType = RTFSFATTYPE_FAT16;
            else
                enmFatType = RTFSFATTYPE_FAT32;
        }
    }
    if (enmFatType == RTFSFATTYPE_FAT32)
        cbRootDir = cbCluster;

    /*
     * Calculate the FAT size and number of data cluster.
     *
     * Since the FAT size depends on how many data clusters there are, we start
     * with a minimum FAT size and maximum clust count, then recalucate it. The
     * result isn't necessarily stable, so we will only retry stabalizing the
     * result a few times.
     */
    uint32_t cbReservedFixed = enmFatType == RTFSFATTYPE_FAT32 ? 32 * cbSector : cbSector + cbRootDir;
    uint32_t cbFat           = cbSector;
    if (cbReservedFixed + cbFat * cFats >= cbVol)
        return RTErrInfoSetF(pErrInfo, VERR_DISK_FULL, "volume is too small (cbVol=%#RX64 rsvd=%#x cbFat=%#x cFat=%#x)",
                             cbVol, cbReservedFixed, cbFat, cFats);
    uint32_t cMaxClusters    = enmFatType == RTFSFATTYPE_FAT12 ? FAT_MAX_FAT12_DATA_CLUSTERS
                             : enmFatType == RTFSFATTYPE_FAT16 ? FAT_MAX_FAT16_DATA_CLUSTERS
                             :                                   FAT_MAX_FAT12_DATA_CLUSTERS;
    uint32_t cClusters       = (uint32_t)RT_MIN((cbVol - cbReservedFixed - cbFat * cFats) / cbCluster, cMaxClusters);
    uint32_t cPrevClusters;
    uint32_t cTries          = 4;
    do
    {
        cPrevClusters = cClusters;
        switch (enmFatType)
        {
            case RTFSFATTYPE_FAT12:
                cbFat = (uint32_t)RT_MIN(FAT_MAX_FAT12_TOTAL_CLUSTERS, cClusters) * 3 / 2;
                break;
            case RTFSFATTYPE_FAT16:
                cbFat = (uint32_t)RT_MIN(FAT_MAX_FAT16_TOTAL_CLUSTERS, cClusters) * 2;
                break;
            case RTFSFATTYPE_FAT32:
                cbFat = (uint32_t)RT_MIN(FAT_MAX_FAT32_TOTAL_CLUSTERS, cClusters) * 4;
                cbFat = RT_ALIGN_32(cbFat, _4K);
                break;
            default:
                AssertFailedReturn(VERR_INTERNAL_ERROR_2);
        }
        cbFat = RT_ALIGN_32(cbFat, cbSector);
        if (cbReservedFixed + cbFat * cFats >= cbVol)
            return RTErrInfoSetF(pErrInfo, VERR_DISK_FULL, "volume is too small (cbVol=%#RX64 rsvd=%#x cbFat=%#x cFat=%#x)",
                                 cbVol, cbReservedFixed, cbFat, cFats);
        cClusters = (uint32_t)RT_MIN((cbVol - cbReservedFixed - cbFat * cFats) / cbCluster, cMaxClusters);
    } while (   cClusters != cPrevClusters
             && cTries-- > 0);
    uint64_t const cTotalSectors = cClusters * (uint64_t)cSectorsPerCluster + (cbReservedFixed + cbFat * cFats) / cbSector;

    /*
     * Check that the file system type and cluster count matches up.  If they
     * don't the type will be misdetected.
     *
     * Note! These assertions could trigger if the above calculations are wrong.
     */
    switch (enmFatType)
    {
        case RTFSFATTYPE_FAT12:
            AssertMsgReturn(cClusters >= FAT_MIN_FAT12_DATA_CLUSTERS && cClusters <= FAT_MAX_FAT12_DATA_CLUSTERS,
                            ("cClusters=%#x\n", cClusters), VERR_OUT_OF_RANGE);
            break;
        case RTFSFATTYPE_FAT16:
            AssertMsgReturn(cClusters >= FAT_MIN_FAT16_DATA_CLUSTERS && cClusters <= FAT_MAX_FAT16_DATA_CLUSTERS,
                            ("cClusters=%#x\n", cClusters), VERR_OUT_OF_RANGE);
            break;
        case RTFSFATTYPE_FAT32:
            AssertMsgReturn(cClusters >= FAT_MIN_FAT32_DATA_CLUSTERS && cClusters <= FAT_MAX_FAT32_DATA_CLUSTERS,
                            ("cClusters=%#x\n", cClusters), VERR_OUT_OF_RANGE);
            RT_FALL_THRU();
        default:
            AssertFailedReturn(VERR_INTERNAL_ERROR_2);
    }

    /*
     * Okay, create the boot sector.
     */
    size_t   cbBuf = RT_MAX(RT_MAX(_64K, cbCluster), cbSector * 2U);
    uint8_t *pbBuf = (uint8_t *)RTMemTmpAllocZ(cbBuf);
    AssertReturn(pbBuf, VERR_NO_TMP_MEMORY);

    const char *pszLastOp = "boot sector";
    PFATBOOTSECTOR pBootSector = (PFATBOOTSECTOR)pbBuf;
    pBootSector->abJmp[0] = 0xeb;
    pBootSector->abJmp[1] = RT_UOFFSETOF(FATBOOTSECTOR, Bpb)
                          + (enmFatType == RTFSFATTYPE_FAT32 ? sizeof(FAT32EBPB) : sizeof(FATEBPB)) - 2;
    pBootSector->abJmp[2] = 0x90;
    memcpy(pBootSector->achOemName, enmFatType == RTFSFATTYPE_FAT32 ? "FAT32   " : "IPRT 6.2", sizeof(pBootSector->achOemName));
    pBootSector->Bpb.Bpb331.cbSector            = (uint16_t)cbSector;
    pBootSector->Bpb.Bpb331.cSectorsPerCluster  = (uint8_t)cSectorsPerCluster;
    pBootSector->Bpb.Bpb331.cReservedSectors    = enmFatType == RTFSFATTYPE_FAT32 ? cbReservedFixed / cbSector : 1;
    pBootSector->Bpb.Bpb331.cFats               = (uint8_t)cFats;
    pBootSector->Bpb.Bpb331.cMaxRootDirEntries  = enmFatType == RTFSFATTYPE_FAT32 ?  0 : cRootDirEntries;
    pBootSector->Bpb.Bpb331.cTotalSectors16     = cTotalSectors <= UINT16_MAX     ? (uint16_t)cTotalSectors : 0;
    pBootSector->Bpb.Bpb331.bMedia              = bMedia;
    pBootSector->Bpb.Bpb331.cSectorsPerFat      = enmFatType == RTFSFATTYPE_FAT32 ?  0 : cbFat / cbSector;
    pBootSector->Bpb.Bpb331.cSectorsPerTrack    = cSectorsPerTrack;
    pBootSector->Bpb.Bpb331.cTracksPerCylinder  = cHeads;
    pBootSector->Bpb.Bpb331.cHiddenSectors      = cHiddenSectors;
    /* XP barfs if both cTotalSectors32 and cTotalSectors16 are set */
    pBootSector->Bpb.Bpb331.cTotalSectors32     = cTotalSectors <= UINT32_MAX && pBootSector->Bpb.Bpb331.cTotalSectors16 == 0
                                                ? (uint32_t)cTotalSectors : 0;
    if (enmFatType != RTFSFATTYPE_FAT32)
    {
        pBootSector->Bpb.Ebpb.bInt13Drive       = 0;
        pBootSector->Bpb.Ebpb.bReserved         = 0;
        pBootSector->Bpb.Ebpb.bExtSignature     = FATEBPB_SIGNATURE;
        pBootSector->Bpb.Ebpb.uSerialNumber     = RTRandU32();
        memset(pBootSector->Bpb.Ebpb.achLabel, ' ',  sizeof(pBootSector->Bpb.Ebpb.achLabel));
        memcpy(pBootSector->Bpb.Ebpb.achType, enmFatType == RTFSFATTYPE_FAT12 ? "FAT12   " : "FAT16   ",
               sizeof(pBootSector->Bpb.Ebpb.achType));
    }
    else
    {
        pBootSector->Bpb.Fat32Ebpb.cSectorsPerFat32         = cbFat / cbSector;
        pBootSector->Bpb.Fat32Ebpb.fFlags                   = 0;
        pBootSector->Bpb.Fat32Ebpb.uVersion                 = FAT32EBPB_VERSION_0_0;
        pBootSector->Bpb.Fat32Ebpb.uRootDirCluster          = FAT_FIRST_DATA_CLUSTER;
        pBootSector->Bpb.Fat32Ebpb.uInfoSectorNo            = 1;
        pBootSector->Bpb.Fat32Ebpb.uBootSectorCopySectorNo  = 6;
        RT_ZERO(pBootSector->Bpb.Fat32Ebpb.abReserved);

        pBootSector->Bpb.Fat32Ebpb.bInt13Drive   = 0;
        pBootSector->Bpb.Fat32Ebpb.bReserved     = 0;
        pBootSector->Bpb.Fat32Ebpb.bExtSignature = FATEBPB_SIGNATURE;
        pBootSector->Bpb.Fat32Ebpb.uSerialNumber = RTRandU32();
        memset(pBootSector->Bpb.Fat32Ebpb.achLabel, ' ',  sizeof(pBootSector->Bpb.Fat32Ebpb.achLabel));
        if (cTotalSectors > UINT32_MAX)
            pBootSector->Bpb.Fat32Ebpb.u.cTotalSectors64 = cTotalSectors;
        else
            memcpy(pBootSector->Bpb.Fat32Ebpb.u.achType, "FAT32   ", sizeof(pBootSector->Bpb.Fat32Ebpb.u.achType));
    }
    pbBuf[pBootSector->abJmp[1] + 2 + 0] = 0xcd; /* int 19h */
    pbBuf[pBootSector->abJmp[1] + 2 + 1] = 0x19;
    pbBuf[pBootSector->abJmp[1] + 2 + 2] = 0xcc; /* int3 */
    pbBuf[pBootSector->abJmp[1] + 2 + 3] = 0xcc;

    pBootSector->uSignature = FATBOOTSECTOR_SIGNATURE;
    if (cbSector != sizeof(*pBootSector))
        *(uint16_t *)&pbBuf[cbSector - 2] = FATBOOTSECTOR_SIGNATURE; /** @todo figure out how disks with non-512 byte sectors work! */

    rc = RTVfsFileWriteAt(hVfsFile, offVol, pBootSector, cbSector, NULL);
    uint32_t const offFirstFat = pBootSector->Bpb.Bpb331.cReservedSectors * cbSector;

    /*
     * Write the FAT32 info sector, 3 boot sector copies, and zero fill
     * the other reserved sectors.
     */
    if (RT_SUCCESS(rc) && enmFatType == RTFSFATTYPE_FAT32)
    {
        pszLastOp = "fat32 info sector";
        PFAT32INFOSECTOR pInfoSector = (PFAT32INFOSECTOR)&pbBuf[cbSector]; /* preserve the boot sector. */
        RT_ZERO(*pInfoSector);
        pInfoSector->uSignature1           = FAT32INFOSECTOR_SIGNATURE_1;
        pInfoSector->uSignature2           = FAT32INFOSECTOR_SIGNATURE_2;
        pInfoSector->uSignature3           = FAT32INFOSECTOR_SIGNATURE_3;
        pInfoSector->cFreeClusters         = cClusters - 1; /* ASSUMES 1 cluster for the root dir. */
        pInfoSector->cLastAllocatedCluster = FAT_FIRST_DATA_CLUSTER;
        rc = RTVfsFileWriteAt(hVfsFile, offVol + cbSector, pInfoSector, cbSector, NULL);

        uint32_t iSector = 2;
        if (RT_SUCCESS(rc))
        {
            pszLastOp = "fat32 unused reserved sectors";
            rc = rtFsFatVolWriteZeros(hVfsFile, offVol + iSector * cbSector,
                                      (pBootSector->Bpb.Fat32Ebpb.uBootSectorCopySectorNo - iSector) * cbSector);
            iSector = pBootSector->Bpb.Fat32Ebpb.uBootSectorCopySectorNo;
        }

        if (RT_SUCCESS(rc))
        {
            pszLastOp = "boot sector copy";
            for (uint32_t i = 0; i < 3 && RT_SUCCESS(rc); i++, iSector++)
                rc = RTVfsFileWriteAt(hVfsFile, offVol + iSector * cbSector, pBootSector, cbSector, NULL);
        }

        if (RT_SUCCESS(rc))
        {
            pszLastOp = "fat32 unused reserved sectors";
            rc = rtFsFatVolWriteZeros(hVfsFile, offVol + iSector * cbSector,
                                      (pBootSector->Bpb.Bpb331.cReservedSectors - iSector) * cbSector);
        }
    }

    /*
     * The FATs.
     */
    if (RT_SUCCESS(rc))
    {
        pszLastOp = "fat";
        pBootSector = NULL; /* invalid  */
        RT_BZERO(pbBuf, cbSector);
        switch (enmFatType)
        {
            case RTFSFATTYPE_FAT32:
                pbBuf[11] = 0x0f;       /* EOC for root dir*/
                pbBuf[10] = 0xff;
                pbBuf[9]  = 0xff;
                pbBuf[8]  = 0xff;
                pbBuf[7]  = 0x0f;       /* Formatter's EOC, followed by signed extend FAT ID. */
                pbBuf[6]  = 0xff;
                pbBuf[5]  = 0xff;
                pbBuf[4]  = 0xff;
                RT_FALL_THRU();
            case RTFSFATTYPE_FAT16:
                pbBuf[3]  = 0xff;
                RT_FALL_THRU();
            case RTFSFATTYPE_FAT12:
                pbBuf[2]  = 0xff;
                pbBuf[1]  = 0xff;
                pbBuf[0]  = bMedia;     /* FAT ID */
                break;
            default: AssertFailed();
        }
        for (uint32_t iFatCopy = 0; iFatCopy < cFats && RT_SUCCESS(rc); iFatCopy++)
        {
            rc = RTVfsFileWriteAt(hVfsFile, offVol + offFirstFat + cbFat * iFatCopy, pbBuf, cbSector, NULL);
            if (RT_SUCCESS(rc) && cbFat > cbSector)
                rc = rtFsFatVolWriteZeros(hVfsFile, offVol + offFirstFat + cbFat * iFatCopy + cbSector, cbFat - cbSector);
        }
    }

    /*
     * The root directory.
     */
    if (RT_SUCCESS(rc))
    {
        /** @todo any mandatory directory entries we need to fill in here? */
        pszLastOp = "root dir";
        rc = rtFsFatVolWriteZeros(hVfsFile, offVol + offFirstFat + cbFat * cFats, cbRootDir);
    }

    /*
     * If long format, fill the rest of the disk with 0xf6.
     */
    AssertCompile(RTFSFATVOL_FMT_F_QUICK != 0);
    if (RT_SUCCESS(rc) && !(fFlags & RTFSFATVOL_FMT_F_QUICK))
    {
        pszLastOp = "formatting data clusters";
        uint64_t offCur = offFirstFat + cbFat * cFats + cbRootDir;
        uint64_t cbLeft = cTotalSectors * cbSector;
        if (cbVol - cbLeft <= _256K) /* HACK ALERT! Format to end of volume if it's a cluster rounding thing.  */
            cbLeft = cbVol;
        if (cbLeft > offCur)
        {
            cbLeft -= offCur;
            offCur += offVol;

            memset(pbBuf, 0xf6, cbBuf);
            while (cbLeft > 0)
            {
                size_t cbToWrite = cbLeft >= cbBuf ? cbBuf : (size_t)cbLeft;
                rc = RTVfsFileWriteAt(hVfsFile, offCur, pbBuf, cbToWrite, NULL);
                if (RT_SUCCESS(rc))
                {
                    offCur += cbToWrite;
                    cbLeft -= cbToWrite;
                }
                else
                    break;
            }
        }
    }

    /*
     * Done.
     */
    RTMemTmpFree(pbBuf);
    if (RT_SUCCESS(rc))
        return rc;
    return RTErrInfoSet(pErrInfo, rc, pszLastOp);
}


/**
 * Formats a 1.44MB floppy image.
 *
 * @returns IPRT status code.
 * @param   hVfsFile            The image.
 */
RTDECL(int) RTFsFatVolFormat144(RTVFSFILE hVfsFile, bool fQuick)
{
    return RTFsFatVolFormat(hVfsFile, 0 /*offVol*/, 1474560, fQuick ? RTFSFATVOL_FMT_F_QUICK : RTFSFATVOL_FMT_F_FULL,
                            512 /*cbSector*/, 1 /*cSectorsPerCluster*/, RTFSFATTYPE_FAT12, 2 /*cHeads*/,  18 /*cSectors*/,
                            0xf0 /*bMedia*/, 224 /*cRootDirEntries*/, 0 /*cHiddenSectors*/, NULL /*pErrInfo*/);
}


/**
 * @interface_method_impl{RTVFSCHAINELEMENTREG,pfnValidate}
 */
static DECLCALLBACK(int) rtVfsChainFatVol_Validate(PCRTVFSCHAINELEMENTREG pProviderReg, PRTVFSCHAINSPEC pSpec,
                                                   PRTVFSCHAINELEMSPEC pElement, uint32_t *poffError, PRTERRINFO pErrInfo)
{
    RT_NOREF(pProviderReg);

    /*
     * Basic checks.
     */
    if (pElement->enmTypeIn != RTVFSOBJTYPE_FILE)
        return pElement->enmTypeIn == RTVFSOBJTYPE_INVALID ? VERR_VFS_CHAIN_CANNOT_BE_FIRST_ELEMENT : VERR_VFS_CHAIN_TAKES_FILE;
    if (   pElement->enmType != RTVFSOBJTYPE_VFS
        && pElement->enmType != RTVFSOBJTYPE_DIR)
        return VERR_VFS_CHAIN_ONLY_DIR_OR_VFS;
    if (pElement->cArgs > 1)
        return VERR_VFS_CHAIN_AT_MOST_ONE_ARG;

    /*
     * Parse the flag if present, save in pElement->uProvider.
     */
    bool fReadOnly = (pSpec->fOpenFile & RTFILE_O_ACCESS_MASK) == RTFILE_O_READ;
    if (pElement->cArgs > 0)
    {
        const char *psz = pElement->paArgs[0].psz;
        if (*psz)
        {
            if (!strcmp(psz, "ro"))
                fReadOnly = true;
            else if (!strcmp(psz, "rw"))
                fReadOnly = false;
            else
            {
                *poffError = pElement->paArgs[0].offSpec;
                return RTErrInfoSet(pErrInfo, VERR_VFS_CHAIN_INVALID_ARGUMENT, "Expected 'ro' or 'rw' as argument");
            }
        }
    }

    pElement->uProvider = fReadOnly;
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSCHAINELEMENTREG,pfnInstantiate}
 */
static DECLCALLBACK(int) rtVfsChainFatVol_Instantiate(PCRTVFSCHAINELEMENTREG pProviderReg, PCRTVFSCHAINSPEC pSpec,
                                                      PCRTVFSCHAINELEMSPEC pElement, RTVFSOBJ hPrevVfsObj,
                                                      PRTVFSOBJ phVfsObj, uint32_t *poffError, PRTERRINFO pErrInfo)
{
    RT_NOREF(pProviderReg, pSpec, poffError);

    int         rc;
    RTVFSFILE   hVfsFileIn = RTVfsObjToFile(hPrevVfsObj);
    if (hVfsFileIn != NIL_RTVFSFILE)
    {
        RTVFS hVfs;
        rc = RTFsFatVolOpen(hVfsFileIn, pElement->uProvider != false, 0, &hVfs, pErrInfo);
        RTVfsFileRelease(hVfsFileIn);
        if (RT_SUCCESS(rc))
        {
            *phVfsObj = RTVfsObjFromVfs(hVfs);
            RTVfsRelease(hVfs);
            if (*phVfsObj != NIL_RTVFSOBJ)
                return VINF_SUCCESS;
            rc = VERR_VFS_CHAIN_CAST_FAILED;
        }
    }
    else
        rc = VERR_VFS_CHAIN_CAST_FAILED;
    return rc;
}


/**
 * @interface_method_impl{RTVFSCHAINELEMENTREG,pfnCanReuseElement}
 */
static DECLCALLBACK(bool) rtVfsChainFatVol_CanReuseElement(PCRTVFSCHAINELEMENTREG pProviderReg,
                                                           PCRTVFSCHAINSPEC pSpec, PCRTVFSCHAINELEMSPEC pElement,
                                                           PCRTVFSCHAINSPEC pReuseSpec, PCRTVFSCHAINELEMSPEC pReuseElement)
{
    RT_NOREF(pProviderReg, pSpec, pReuseSpec);
    if (   pElement->paArgs[0].uProvider == pReuseElement->paArgs[0].uProvider
        || !pReuseElement->paArgs[0].uProvider)
        return true;
    return false;
}


/** VFS chain element 'file'. */
static RTVFSCHAINELEMENTREG g_rtVfsChainFatVolReg =
{
    /* uVersion = */            RTVFSCHAINELEMENTREG_VERSION,
    /* fReserved = */           0,
    /* pszName = */             "fat",
    /* ListEntry = */           { NULL, NULL },
    /* pszHelp = */             "Open a FAT file system, requires a file object on the left side.\n"
                                "First argument is an optional 'ro' (read-only) or 'rw' (read-write) flag.\n",
    /* pfnValidate = */         rtVfsChainFatVol_Validate,
    /* pfnInstantiate = */      rtVfsChainFatVol_Instantiate,
    /* pfnCanReuseElement = */  rtVfsChainFatVol_CanReuseElement,
    /* uEndMarker = */          RTVFSCHAINELEMENTREG_VERSION
};

RTVFSCHAIN_AUTO_REGISTER_ELEMENT_PROVIDER(&g_rtVfsChainFatVolReg, rtVfsChainFatVolReg);

