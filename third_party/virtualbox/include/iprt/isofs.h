/** @file
 * IPRT - ISO 9660 file system handling.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
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

#ifndef ___iprt_isofs_h
#define ___iprt_isofs_h

#include <iprt/types.h>
#include <iprt/list.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_isofs    RTIsoFs - ISO 9660 Filesystem
 * @ingroup grp_rt
 * @{
 */

#define RTISOFS_MAX_SYSTEM_ID      32
#define RTISOFS_MAX_VOLUME_ID      32
#define RTISOFS_MAX_PUBLISHER_ID   128
#define RTISOFS_MAX_VOLUME_ID      32
#define RTISOFS_MAX_VOLUMESET_ID   128
#define RTISOFS_MAX_PREPARER_ID    128
#define RTISOFS_MAX_APPLICATION_ID 128
#define RTISOFS_MAX_STRING_LEN     255

/** Standard ID of volume descriptors. */
#define RTISOFS_STANDARD_ID        "CD001"

/** Default sector size. */
#define RTISOFS_SECTOR_SIZE        2048


#pragma pack(1)
typedef struct RTISOFSDATESHORT
{
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    int8_t  gmt_offset;
} RTISOFSDATESHORT, *PRTISOFSDATESHORT;

typedef struct RTISOFSDATELONG
{
    char year[4];
    char month[2];
    char day[2];
    char hour[2];
    char minute[2];
    char second[2];
    char hseconds[2];
    int8_t gmt_offset;
} RTISOFSDATELONG, *PRTISOFSDATELONG;

/* Directory Record. */
typedef struct RTISOFSDIRRECORD
{
    uint8_t record_length;
    uint8_t extented_attr_length;
    uint32_t extent_location;
    uint32_t extent_location_big;
    uint32_t extent_data_length; /* Number of bytes (file) / len (directory). */
    uint32_t extent_data_length_big;
    RTISOFSDATESHORT date;
    uint8_t flags;
    uint8_t interleave_unit_size;
    uint8_t interleave_gap_size;
    uint16_t volume_sequence_number;
    uint16_t volume_sequence_number_big;
    uint8_t name_len;
    /* Starting here there will be the actual directory entry name
     * and a padding of 1 byte if name_len is odd. */
} RTISOFSDIRRECORD, *PRTISOFSDIRRECORD;

/* Primary Volume Descriptor. */
typedef struct RTISOFSPRIVOLDESC
{
    uint8_t type;
    char name_id[6];
    uint8_t version;
    char system_id[RTISOFS_MAX_SYSTEM_ID];
    char volume_id[RTISOFS_MAX_VOLUME_ID];
    uint8_t unused2[8];
    uint32_t volume_space_size; /* Number of sectors, Little Endian. */
    uint32_t volume_space_size_big; /* Number of sectors Big Endian. */
    uint8_t unused3[32];
    uint16_t volume_set_size;
    uint16_t volume_set_size_big;
    uint16_t volume_sequence_number;
    uint16_t volume_sequence_number_big;
    uint16_t logical_block_size; /* 2048. */
    uint16_t logical_block_size_big;
    uint32_t path_table_size; /* Size in bytes. */
    uint32_t path_table_size_big; /* Size in bytes. */
    uint32_t path_table_start_first;
    uint32_t path_table_start_second;
    uint32_t path_table_start_first_big;
    uint32_t path_table_start_second_big;
    RTISOFSDIRRECORD root_directory_record;
    uint8_t directory_padding;
    char volume_set_id[RTISOFS_MAX_VOLUMESET_ID];
    char publisher_id[RTISOFS_MAX_PUBLISHER_ID];
    char preparer_id[RTISOFS_MAX_PREPARER_ID];
    char application_id[RTISOFS_MAX_APPLICATION_ID];
    char copyright_file_id[37];
    char abstract_file_id[37];
    char bibliographic_file_id[37];
    RTISOFSDATELONG creation_date;
    RTISOFSDATELONG modification_date;
    RTISOFSDATELONG expiration_date;
    RTISOFSDATELONG effective_date;
    uint8_t file_structure_version;
    uint8_t unused4[1];
    char application_data[512];
    uint8_t unused5[653];
} RTISOFSPRIVOLDESC, *PRTISOFSPRIVOLDESC;

typedef struct RTISOFSPATHTABLEHEADER
{
    uint8_t length;
    uint8_t extended_attr_sectors;
    /** Sector of starting directory table. */
    uint32_t sector_dir_table;
    /** Index of parent directory (1 for the root). */
    uint16_t parent_index;
    /* Starting here there will be the name of the directory,
     * specified by length above. */
} RTISOFSPATHTABLEHEADER, *PRTISOFSPATHTABLEHEADER;

typedef struct RTISOFSPATHTABLEENTRY
{
    char       *path;
    char       *path_full;
    RTISOFSPATHTABLEHEADER header;
    RTLISTNODE  Node;
} RTISOFSPATHTABLEENTRY, *PRTISOFSPATHTABLEENTRY;

typedef struct RTISOFSFILE
{
    RTFILE file;
    RTLISTANCHOR listPaths;
    RTISOFSPRIVOLDESC pvd;
} RTISOFSFILE, *PRTISOFSFILE;
#pragma pack()


#ifdef IN_RING3
/**
 * Opens an ISO file.
 *
 * The following limitations apply:
 * - Fixed sector size (2048 bytes).
 * - No extensions (Joliet, RockRidge etc.) support (yet).
 * - Only primary volume descriptor (PVD) handled.
 *
 * @return  IPRT status code.
 * @param   pFile           Pointer to ISO handle.
 * @param   pszFileName     Path to ISO file to open.
 */
RTR3DECL(int) RTIsoFsOpen(PRTISOFSFILE pFile, const char *pszFileName);

/**
 * Closes an ISO file.
 *
 * @param   pFile       Pointer to open ISO file returned by RTIsoFsOpen().
 */
RTR3DECL(void) RTIsoFsClose(PRTISOFSFILE pFile);

/**
 * Retrieves the offset + length (both in bytes) of a given file
 * stored in the ISO.
 *
 * @note    According to the standard, a file cannot be larger than 2^32-1 bytes.
 *          Therefore using size_t / uint32_t is not a problem.
 *
 * @return  IPRT status code.
 * @param   pFile       Pointer to open ISO file returned by RTIsoFsOpen().
 * @param   pszPath     Path of file within the ISO to retrieve information for.
 * @param   poffInIso   Wheter to store the file's absolute offset within the
 *                      ISO.
 * @param   pcbLength   Pointer to store the file's size.
 */
RTR3DECL(int) RTIsoFsGetFileInfo(PRTISOFSFILE pFile, const char *pszPath, uint32_t *poffInIso, size_t *pcbLength);

/**
 * Extracts a file from an ISO to the given destination.
 *
 * @return  IPRT status code.
 * @param   pFile       Pointer to open ISO file returned by RTIsoFsOpen().
 * @param   pszSrcPath  Path of file within the ISO to extract.
 * @param   pszDstPath  Where to store the extracted file.
 */
RTR3DECL(int) RTIsoFsExtractFile(PRTISOFSFILE pFile, const char *pszSrcPath, const char *pszDstPath);

#endif /* IN_RING3 */

/** @} */

RT_C_DECLS_END

#endif

