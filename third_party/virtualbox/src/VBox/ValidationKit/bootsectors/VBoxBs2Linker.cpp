/* $Id: VBoxBs2Linker.cpp $ */
/** @file
 * VirtualBox Validation Kit - Boot Sector 2 "linker".
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


int main(int argc, char **argv)
{
    const char  *pszOutput   = NULL;
    const char **papszInputs = (const char **)calloc(argc, sizeof(const char *));
    unsigned     cInputs     = 0;

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
                    free(papszInputs);
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
                                free(papszInputs);
                                return 12;
                            }
                            pszValue = argv[++i];
                        }
                        if (pszOutput)
                        {
                            fprintf(stderr, "Only one output file is allowed. You've specified '%s' and '%s'\n",
                                    pszOutput, pszValue);
                            free(papszInputs);
                            return 2;
                        }
                        pszOutput = pszValue;
                        pszOpt = "";
                        break;
                    }

                    case 'V':
                        printf("%s\n", "$Revision: 118412 $");
                        free(papszInputs);
                        return 0;

                    case '?':
                    case 'h':
                        printf("usage: %s [options] -o <output> <input1> [input2 ... [inputN]]\n", argv[0]);
                        free(papszInputs);
                        return 0;
                }
            }
        }
        else
            papszInputs[cInputs++] = argv[i];
    }

    if (!pszOutput)
    {
        fprintf(stderr, "syntax error: No output file was specified (-o or --output).\n");
        free(papszInputs);
        return 2;
    }
    if (cInputs == 0)
    {
        fprintf(stderr, "syntax error: No input files was specified.\n");
        free(papszInputs);
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
        free(papszInputs);
        return 1;
    }

    /* Copy the input files to the output file, with sector padding applied. */
    int rcExit = 0;
    size_t off = 0;
    for (unsigned i = 0; i < cInputs && rcExit == 0; i++)
    {
#if defined(RT_OS_OS2) || defined(RT_OS_WINDOWS)
        FILE *pInput = fopen(papszInputs[i], "rb");
#else
        FILE *pInput = fopen(papszInputs[i], "r");
#endif
        if (pInput)
        {
            for (;;)
            {
                /* Read a block from the input file. */
                uint8_t abBuf[4096];
                size_t cbRead = fread(abBuf, sizeof(uint8_t), 4096, pInput);
                if (!cbRead || ferror(pInput))
                    break;

                /* Padd the end of the file if necessary. */
                if (cbRead != 4096 && !feof(pInput))
                {
                    fprintf(stderr, "error: fread returned %u bytes, but we're not at the end of the file yet...\n",
                            (unsigned)cbRead);
                    rcExit = 1;
                    break;
                }
                if ((cbRead & 0x1ff) != 0)
                {
                    memset(&abBuf[cbRead], 0, 4096 - cbRead);
                    cbRead = (cbRead + 0x1ff) & ~0x1ffU;
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

            if (ferror(pInput))
            {
                fprintf(stderr, "error: Error reading '%s'.\n", papszInputs[i]);
                rcExit = 1;
            }
            fclose(pInput);
        }
        else
        {
            fprintf(stderr, "error: Failed to open '%s' for reading.\n", papszInputs[i]);
            rcExit = 1;
        }
    }

    /* Finally, close the output file (can fail because of buffered data). */
    if (fclose(stderr) != 0)
    {
        fprintf(stderr, "error: Error closing '%s'.\n", pszOutput);
        rcExit = 1;
    }

    fclose(pOutput);
    free(papszInputs);
    return rcExit;
}

