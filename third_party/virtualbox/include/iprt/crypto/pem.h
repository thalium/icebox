/** @file
 * IPRT - Crypto - PEM-file Reader & Writer.
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

#ifndef ___iprt_crypto_pem_h
#define ___iprt_crypto_pem_h

#include <iprt/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_spc  RTCrPem - PEM-file Reader & Writer
 * @ingroup grp_rt_crypto
 * @{
 */


/**
 * One PEM marker word (use RT_STR_TUPLE to initialize).
 */
typedef struct RTCRPEMMARKERWORD
{
    /** The word string. */
    const char         *pszWord;
    /** The length. */
    uint32_t            cchWord;
} RTCRPEMMARKERWORD;
/** Pointer to a const marker word. */
typedef RTCRPEMMARKERWORD const *PCRTCRPEMMARKERWORD;


/**
 * A PEM marker.
 *
 * This is an array of words with lengths, optimized for avoid unnecessary
 * strlen() while searching the file content.  It is ASSUMED that all PEM
 * section markers starts with either 'BEGIN' or 'END', followed by the words
 * in the this structure.
 */
typedef struct RTCRPEMMARKER
{
    /** Pointer to an array of marker words. */
    PCRTCRPEMMARKERWORD paWords;
    /** Number of works in the array papszWords points to. */
    uint32_t            cWords;
} RTCRPEMMARKER;
/** Pointer to a const PEM marker. */
typedef RTCRPEMMARKER const *PCRTCRPEMMARKER;


/**
 * A PEM section.
 *
 * The API works on linked lists of these.
 */
typedef struct RTCRPEMSECTION
{
    /** Pointer to the next file section. */
    struct RTCRPEMSECTION const *pNext;
    /** The marker for this section.  NULL if binary file. */
    PCRTCRPEMMARKER     pMarker;
    /** Pointer to the binary data. */
    uint8_t            *pbData;
    /** The size of the binary data. */
    size_t              cbData;
    /** Additional text preceeding the binary data.  NULL if none. */
    char               *pszPreamble;
    /** The length of the preamble. */
    size_t              cchPreamble;
} RTCRPEMSECTION;
/** Pointer to a PEM section. */
typedef RTCRPEMSECTION *PRTCRPEMSECTION;
/** Pointer to a const PEM section. */
typedef RTCRPEMSECTION const *PCRTCRPEMSECTION;


/**
 * Frees sections returned by RTCrPemReadFile and RTCrPemParseContent.
 * @returns IPRT status code.
 * @param   pSectionHead        The first section.
 */
RTDECL(int) RTCrPemFreeSections(PCRTCRPEMSECTION pSectionHead);

/**
 * Parses the given data and returns a list of binary sections.
 *
 * If the file isn't an ASCII file or if no markers were found, the entire file
 * content is returned as one single section (with pMarker = NULL).
 *
 * @returns IPRT status code.
 * @retval  VINF_EOF if the file is empty.  The @a ppSectionHead value will be
 *          NULL.
 * @retval  VWRN_NOT_FOUND no section was found and RTCRPEMREADFILE_F_ONLY_PEM
 *          is specified.  The @a ppSectionHead value will be NULL.
 *
 * @param   pvContent       The content bytes to parse.
 * @param   cbContent       The number of content bytes.
 * @param   fFlags          RTCRPEMREADFILE_F_XXX.
 * @param   paMarkers       Array of one or more section markers to look for.
 * @param   cMarkers        Number of markers in the array.
 * @param   ppSectionHead   Where to return the head of the section list.  Call
 *                          RTCrPemFreeSections to free.
 * @param   pErrInfo        Where to return extend error info. Optional.
 */
RTDECL(int) RTCrPemParseContent(void const *pvContent, size_t cbContent, uint32_t fFlags,
                                PCRTCRPEMMARKER paMarkers, size_t cMarkers, PCRTCRPEMSECTION *ppSectionHead, PRTERRINFO pErrInfo);

/**
 * Reads the content of the given file and returns a list of binary sections
 * found in the file.
 *
 * If the file isn't an ASCII file or if no markers were found, the entire file
 * content is returned as one single section (with pMarker = NULL).
 *
 * @returns IPRT status code.
 * @retval  VINF_EOF if the file is empty.  The @a ppSectionHead value will be
 *          NULL.
 * @retval  VWRN_NOT_FOUND no section was found and RTCRPEMREADFILE_F_ONLY_PEM
 *          is specified.  The @a ppSectionHead value will be NULL.
 *
 * @param   pszFilename     The path to the file to read.
 * @param   fFlags          RTCRPEMREADFILE_F_XXX.
 * @param   paMarkers       Array of one or more section markers to look for.
 * @param   cMarkers        Number of markers in the array.
 * @param   ppSectionHead   Where to return the head of the section list. Call
 *                          RTCrPemFreeSections to free.
 * @param   pErrInfo        Where to return extend error info. Optional.
 */
RTDECL(int) RTCrPemReadFile(const char *pszFilename, uint32_t fFlags, PCRTCRPEMMARKER paMarkers, size_t cMarkers,
                            PCRTCRPEMSECTION *ppSectionHead, PRTERRINFO pErrInfo);
/** @name RTCRPEMREADFILE_F_XXX - Flags for RTCrPemReadFile and
 *        RTCrPemParseContent.
 * @{ */
/** Continue on encoding error. */
#define RTCRPEMREADFILE_F_CONTINUE_ON_ENCODING_ERROR    RT_BIT(0)
/** Only PEM sections, no binary fallback. */
#define RTCRPEMREADFILE_F_ONLY_PEM                      RT_BIT(1)
/** Valid flags. */
#define RTCRPEMREADFILE_F_VALID_MASK                    UINT32_C(0x00000003)
/** @} */

/**
 * Finds the beginning of first PEM section using the specified markers.
 *
 * This will not look any further than the first section.  Nor will it check for
 * binaries.
 *
 * @returns Pointer to the "-----BEGIN XXXX" sequence on success.
 *          NULL if not found.
 * @param   pvContent       The content bytes to parse.
 * @param   cbContent       The number of content bytes.
 * @param   paMarkers       Array of one or more section markers to look for.
 * @param   cMarkers        Number of markers in the array.
 */
RTDECL(const char *) RTCrPemFindFirstSectionInContent(void const *pvContent, size_t cbContent,
                                                      PCRTCRPEMMARKER paMarkers, size_t cMarkers);

/** @} */

RT_C_DECLS_END

#endif

