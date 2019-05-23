/* $Id: VBoxBs3Linker.cpp $ */
/** @file
 * VirtualBox Validation Kit - Boot Sector 3 "linker".
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iprt/types.h>
#include <iprt/assert.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
#pragma pack(1)
typedef struct BS3BOOTSECTOR
{
    uint8_t     abJmp[3];
    char        abOemId[8];
    /** @name EBPB, DOS 4.0 style.
     * @{  */
    uint16_t    cBytesPerSector;        /**< 00bh */
    uint8_t     cSectorsPerCluster;     /**< 00dh */
    uint16_t    cReservedSectors;       /**< 00eh */
    uint8_t     cFATs;                  /**< 010h */
    uint16_t    cRootDirEntries;        /**< 011h */
    uint16_t    cTotalSectors;          /**< 013h */
    uint8_t     bMediaDescriptor;       /**< 015h */
    uint16_t    cSectorsPerFAT;         /**< 016h */
    uint16_t    cPhysSectorsPerTrack;   /**< 018h */
    uint16_t    cHeads;                 /**< 01ah */
    uint32_t    cHiddentSectors;        /**< 01ch */
    uint32_t    cLargeTotalSectors;     /**< 020h - We (ab)use this to indicate the number of sectors to load. */
    uint8_t     bBootDrv;               /**< 024h */
    uint8_t     bFlagsEtc;              /**< 025h */
    uint8_t     bExtendedSignature;     /**< 026h */
    uint32_t    dwSerialNumber;         /**< 027h */
    char        abLabel[11];            /**< 02bh */
    char        abFSType[8];            /**< 036h */
    /** @} */
} BS3BOOTSECTOR;
#pragma pack()
typedef BS3BOOTSECTOR *PBS3BOOTSECTOR;

AssertCompileMemberOffset(BS3BOOTSECTOR, cLargeTotalSectors, 0x20);
AssertCompileMemberOffset(BS3BOOTSECTOR, abLabel, 0x2b);
AssertCompileMemberOffset(BS3BOOTSECTOR, abFSType, 0x36);

#define BS3_OEMID       "BS3Kit\n\n"
#define BS3_FSTYPE      "RawCode\n"
#define BS3_LABEL       "VirtualBox\n"
#define BS3_MAX_SIZE    UINT32_C(491520) /* 480KB */


int main(int argc, char **argv)
{
    const char  *pszOutput  = NULL;
    struct BS3LNKINPUT
    {
        const char *pszFile;
        FILE       *pFile;
        uint32_t    cbFile;
    }           *paInputs   = (struct BS3LNKINPUT *)calloc(sizeof(paInputs[0]), argc);
    unsigned     cInputs    = 0;
    uint32_t     cSectors   = 0;

    /*
     * Scan the arguments.
     */
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            const char *pszOpt = &argv[i][1];
            if (*pszOpt == '-')
            {
                /* Convert long options to short ones. */
                pszOpt--;
                if (!strcmp(pszOpt, "--output"))
                    pszOpt = "o";
                else if (!strcmp(pszOpt, "--version"))
                    pszOpt = "V";
                else if (!strcmp(pszOpt, "--help"))
                    pszOpt = "h";
                else
                {
                    fprintf(stderr, "syntax errro: Unknown options '%s'\n", pszOpt);
                    free(paInputs);
                    return 2;
                }
            }

            /* Process the list of short options. */
            while (*pszOpt)
            {
                switch (*pszOpt++)
                {
                    case 'o':
                    {
                        const char *pszValue = pszOpt;
                        pszOpt = strchr(pszOpt, '\0');
                        if (*pszValue == '=')
                            pszValue++;
                        else if (!*pszValue)
                        {
                            if (i + 1 >= argc)
                            {
                                fprintf(stderr, "syntax error: The --output option expects a filename.\n");
                                free(paInputs);
                                return 12;
                            }
                            pszValue = argv[++i];
                        }
                        if (pszOutput)
                        {
                            fprintf(stderr, "Only one output file is allowed. You've specified '%s' and '%s'\n",
                                    pszOutput, pszValue);
                            free(paInputs);
                            return 2;
                        }
                        pszOutput = pszValue;
                        pszOpt = "";
                        break;
                    }

                    case 'V':
                        printf("%s\n", "$Revision: 118412 $");
                        free(paInputs);
                        return 0;

                    case '?':
                    case 'h':
                        printf("usage: %s [options] -o <output> <input1> [input2 ... [inputN]]\n",
                               argv[0]);
                        free(paInputs);
                        return 0;
                }
            }
        }
        else
        {
            /*
             * Add to input file collection.
             */
            paInputs[cInputs].pszFile = argv[i];
#if defined(RT_OS_OS2) || defined(RT_OS_WINDOWS)
            FILE *pFile = fopen(paInputs[cInputs].pszFile, "rb");
#else
            FILE *pFile = fopen(paInputs[cInputs].pszFile, "r");
#endif
            if (pFile)
            {
                if (fseek(pFile, 0, SEEK_END) == 0)
                {
                    paInputs[cInputs].cbFile = (uint32_t)ftell(pFile);
                    if (fseek(pFile, 0, SEEK_SET) == 0)
                    {
                        if (cInputs != 0 || paInputs[cInputs].cbFile == 512)
                        {
                            cSectors += RT_ALIGN_32(paInputs[cInputs].cbFile, 512) / 512;
                            if (cSectors <= BS3_MAX_SIZE / 512)
                            {
                                if (cSectors > 0)
                                {
                                    paInputs[cInputs].pFile = pFile;
                                    pFile = NULL;
                                }
                                else
                                    fprintf(stderr, "error: empty input file: '%s'\n", paInputs[cInputs].pszFile);
                            }
                            else
                                fprintf(stderr, "error: input is too big: %u bytes, %u sectors (max %u bytes, %u sectors)\n"
                                        "info: detected loading '%s'\n",
                                        cSectors * 512, cSectors, BS3_MAX_SIZE, BS3_MAX_SIZE / 512,
                                        paInputs[cInputs].pszFile);
                        }
                        else
                            fprintf(stderr, "error: first input file (%s) must be exactly 512 bytes\n", paInputs[cInputs].pszFile);
                    }
                    else
                        fprintf(stderr, "error: seeking to start of '%s' failed\n", paInputs[cInputs].pszFile);
                }
                else
                    fprintf(stderr, "error: seeking to end of '%s' failed\n", paInputs[cInputs].pszFile);
            }
            else
                fprintf(stderr, "error: Failed to open input file '%s' for reading\n", paInputs[cInputs].pszFile);
            if (pFile)
            {
                free(paInputs);
                return 1;
            }
            cInputs++;
        }
    }

    if (!pszOutput)
    {
        fprintf(stderr, "syntax error: No output file was specified (-o or --output).\n");
        free(paInputs);
        return 2;
    }
    if (cInputs == 0)
    {
        fprintf(stderr, "syntax error: No input files was specified.\n");
        free(paInputs);
        return 2;
    }

    /*
     * Do the job.
     */
    /* Open the output file. */
#if defined(RT_OS_OS2) || defined(RT_OS_WINDOWS)
    FILE *pOutput = fopen(pszOutput, "wb");
#else
    FILE *pOutput = fopen(pszOutput, "w");
#endif
    if (!pOutput)
    {
        fprintf(stderr, "error: Failed to open output file '%s' for writing\n", pszOutput);
        free(paInputs);
        return 1;
    }

    /* Copy the input files to the output file, with sector padding applied. */
    int rcExit = 0;
    size_t off = 0;
    for (unsigned i = 0; i < cInputs && rcExit == 0; i++)
    {
        uint8_t  abBuf[4096]; /* Must be multiple of 512! */
        uint32_t cbToRead = paInputs[i].cbFile;
        while (cbToRead > 0)
        {
            /* Read a block from the input file. */
            uint32_t const cbThisRead = RT_MIN(cbToRead, sizeof(abBuf));
            size_t cbRead = fread(abBuf, sizeof(uint8_t), cbThisRead, paInputs[i].pFile);
            if (cbRead != cbThisRead)
            {
                fprintf(stderr, "error: Error reading '%s' (got %d bytes, wanted %u).\n",
                        paInputs[i].pszFile, (int)cbRead, (unsigned)cbThisRead);
                rcExit = 1;
                break;
            }
            cbToRead -= cbThisRead;

            /* Padd the end of the file if necessary. */
            if ((cbRead & 0x1ff) != 0)
            {
                memset(&abBuf[cbRead], 0, 4096 - cbRead);
                cbRead = (cbRead + 0x1ff) & ~0x1ffU;
            }

            /* Patch the BPB of the first file. */
            if (off == 0)
            {
                PBS3BOOTSECTOR pBs = (PBS3BOOTSECTOR)&abBuf[0];
                if (   memcmp(pBs->abLabel,  RT_STR_TUPLE(BS3_LABEL))  == 0
                    && memcmp(pBs->abFSType, RT_STR_TUPLE(BS3_FSTYPE)) == 0
                    && memcmp(pBs->abOemId,  RT_STR_TUPLE(BS3_OEMID))  == 0)
                    pBs->cLargeTotalSectors = cSectors;
                else
                {
                    fprintf(stderr, "error: Didn't find magic strings in the first file (%s).\n", paInputs[i].pszFile);
                    rcExit = 1;
                }
            }

            /* Write the block to the output file. */
            if (fwrite(abBuf, sizeof(uint8_t), cbRead, pOutput) == cbRead)
                off += cbRead;
            else
            {
                fprintf(stderr, "error: fwrite failed\n");
                rcExit = 1;
                break;
            }
        }

        if (ferror(paInputs[i].pFile))
        {
            fprintf(stderr, "error: Error reading '%s'.\n", paInputs[i].pszFile);
            rcExit = 1;
        }
    }

    /* Close the input files. */
    for (unsigned i = 0; i < cInputs && rcExit == 0; i++)
        fclose(paInputs[i].pFile);
    free(paInputs);

    /* Finally, close the output file (can fail because of buffered data). */
    if (fclose(stderr) != 0)
    {
        fprintf(stderr, "error: Error closing '%s'.\n", pszOutput);
        rcExit = 1;
    }

    fclose(pOutput);
    return rcExit;
}

