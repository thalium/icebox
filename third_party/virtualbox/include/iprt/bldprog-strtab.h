/** @file
 * IPRT - Build Program - String Table Generator, Accessors.
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

#ifndef ___iprt_bldprog_strtab_h
#define ___iprt_bldprog_strtab_h

#include <iprt/string.h>
#include <iprt/assert.h>


/**
 * The default build program string table reference.
 */
typedef struct RTBLDPROGSTRREF
{
    /** Offset of the string in the string table. */
    uint32_t off : 22;
    /** The length of the string. */
    uint32_t cch : 10;
} RTBLDPROGSTRREF;
AssertCompileSize(RTBLDPROGSTRREF, sizeof(uint32_t));
/** Pointer to a build program string table reference. */
typedef RTBLDPROGSTRREF const *PCRTBLDPROGSTRREF;


typedef struct RTBLDPROGSTRTAB
{
    const char         *pchStrTab;
    uint32_t            cchStrTab;
    uint8_t             cCompDict;
    PCRTBLDPROGSTRREF   paCompDict;
} RTBLDPROGSTRTAB;
typedef const RTBLDPROGSTRTAB *PCRTBLDPROGSTRTAB;


/**
 * Retrieves the decompressed string.
 *
 * @returns The string size on success, IPRT status code on failure.
 * @param   pStrTab         The string table.
 * @param   offString       The offset of the string.
 * @param   cchString       The length of the string.
 * @param   pszDst          The return buffer.
 * @param   cbDst           The size of the return buffer.
 */
DECLINLINE(ssize_t) RTBldProgStrTabQueryString(PCRTBLDPROGSTRTAB pStrTab, uint32_t offString, size_t cchString,
                                               char *pszDst, size_t cbDst)
{
    AssertReturn(offString < pStrTab->cchStrTab, VERR_OUT_OF_RANGE);
    AssertReturn(offString + cchString <= pStrTab->cchStrTab, VERR_OUT_OF_RANGE);

    if (pStrTab->cCompDict)
    {
        /*
         * Could be compressed, decompress it.
         */
        char * const pchDstStart = pszDst;
        const char  *pchSrc = &pStrTab->pchStrTab[offString];
        while (cchString-- > 0)
        {
            unsigned char uch = *pchSrc++;
            if (!(uch & 0x80))
            {
                /*
                 * Plain text.
                 */
                AssertReturn(cbDst > 1, VERR_BUFFER_OVERFLOW);
                *pszDst++ = (char)uch;
                Assert(uch != 0);
            }
            else if (uch != 0xff)
            {
                /*
                 * Dictionary reference. (No UTF-8 unescaping necessary here.)
                 */
                PCRTBLDPROGSTRREF   pWord   = &pStrTab->paCompDict[uch & 0x7f];
                size_t const        cchWord = pWord->cch;
                AssertReturn((size_t)pWord->off + cchWord <= pStrTab->cchStrTab, VERR_INVALID_PARAMETER);
                AssertReturn(cbDst > cchWord, VERR_BUFFER_OVERFLOW);

                memcpy(pszDst, &pStrTab->pchStrTab[pWord->off], cchWord);
                pszDst += cchWord;
                cbDst  -= cchWord;
            }
            else
            {
                /*
                 * UTF-8 encoded unicode codepoint.
                 */
                size_t  cchCp;
                RTUNICP uc = ' ';
                int rc = RTStrGetCpNEx(&pchSrc, &cchString, &uc);
                AssertStmt(RT_SUCCESS(rc), (uc = '?', pchSrc++, cchString--));

                cchCp = RTStrCpSize(uc);
                AssertReturn(cbDst > cchCp, VERR_BUFFER_OVERFLOW);

                RTStrPutCp(pszDst, uc);
                pszDst += cchCp;
                cbDst  -= cchCp;
            }
        }
        AssertReturn(cbDst > 0, VERR_BUFFER_OVERFLOW);
        *pszDst = '\0';
        return pszDst - pchDstStart;
    }

    /*
     * Not compressed.
     */
    AssertReturn(cbDst > cchString, VERR_BUFFER_OVERFLOW);
    memcpy(pszDst, &pStrTab->pchStrTab[offString], cchString);
    pszDst[cchString] = '\0';
    return (ssize_t)cchString;
}


#endif

