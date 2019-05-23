/** @file
 * File splitter: splits soapC.cpp into manageable pieces. It is somewhat
 * intelligent and avoids splitting inside functions or similar places.
 */

/*
 * Copyright (C) 2009-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <iprt/types.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>


char *readfileIntoBuffer(const char *pszFile, size_t *pcbFile)
{
    FILE *pFileIn = fopen(pszFile, "rb");
    if (pFileIn)
    {
        int rc2 = fseek(pFileIn, 0, SEEK_END);
        long cbFileIn = ftell(pFileIn);
        int rc3 = fseek(pFileIn, 0, SEEK_SET);
        if (rc3 != -1 && rc2 != -1 && cbFileIn >= 0)
        {
            char *pBuffer = (char *)malloc(cbFileIn + 1);
            if (pBuffer)
            {
                size_t cbRead = fread(pBuffer, 1, cbFileIn, pFileIn);
                if (cbRead == (size_t)cbFileIn)
                {
                    pBuffer[cbFileIn] = '\0';
                    fclose(pFileIn);
                    *pcbFile = (size_t)cbFileIn;
                    return pBuffer;
                }

                fprintf(stderr, "split-soapC: Failed to read %ld bytes from input file.\n", cbFileIn);
                free(pBuffer);
            }
            else
                fprintf(stderr, "split-soapC: Failed to allocate %ld bytes.\n", cbFileIn);
        }
        else
            fprintf(stderr, "split-soapC: Seek failure.\n");
        fclose(pFileIn);
    }
    else
        fprintf(stderr, "split-soapC: Cannot open file \"%s\" for reading.\n", pszFile);
    return NULL;
}


int main(int argc, char *argv[])
{
    /*
     * Check argument count.
     */
    if (argc != 4)
    {
        fprintf(stderr, "split-soapC: Must be started with exactly three arguments,\n"
                        "1) the input file, 2) the directory where to put the output files and\n"
                        "3) the number chunks to create.\n");
        return RTEXITCODE_SYNTAX;
    }

    /*
     * Number of chunks (argv[3]).
     */
    char *pszEnd = NULL;
    unsigned long cChunks = strtoul(argv[3], &pszEnd, 0);
    if (cChunks == ULONG_MAX || cChunks == 0 || !argv[3] || *pszEnd)
    {
        fprintf(stderr, "split-soapC: Given argument \"%s\" is not a valid chunk count.\n", argv[3]);
        return RTEXITCODE_SYNTAX;
    }

    /*
     * Read the input file into a zero terminated memory buffer.
     */
    size_t cbFileIn;
    char *pszBuffer = readfileIntoBuffer(argv[1], &cbFileIn);
    if (!pszBuffer)
        return RTEXITCODE_FAILURE;

    /*
     * Split the file.
     */
    RTEXITCODE    rcExit = RTEXITCODE_SUCCESS;
    FILE         *pFileOut = NULL;
    const char   *pszLine = pszBuffer;
    size_t        cbChunk = cbFileIn / cChunks;
    unsigned long cFiles = 0;
    size_t        cbLimit = 0;
    size_t        cbWritten = 0;
    unsigned long cIfNesting = 0;
    unsigned long cBraceNesting = 0;
    unsigned long cLinesSinceStaticMap = ~0UL / 2;
    bool          fJustZero = false;

    do
    {
        if (!pFileOut)
        {
            /* construct output filename */
            char szFilename[1024];
            sprintf(szFilename, "%s/soapC-%lu.cpp", argv[2], ++cFiles);
            szFilename[sizeof(szFilename)-1] = '\0';
            printf("info: soapC-%lu.cpp\n", cFiles);

            /* create output file */
            pFileOut = fopen(szFilename, "wb");
            if (!pFileOut)
            {
                fprintf(stderr, "split-soapC: Failed to open file \"%s\" for writing\n", szFilename);
                rcExit = RTEXITCODE_FAILURE;
                break;
            }
            if (cFiles > 1)
                fprintf(pFileOut, "#include \"soapH.h\"%s\n",
#ifdef RT_OS_WINDOWS
                                  "\r"
#else
                                  ""
#endif
                       );
            cbLimit += cbChunk;
            cLinesSinceStaticMap = ~0UL / 2;
        }

        /* find begin of next line and print current line */
        const char *pszNextLine = strchr(pszLine, '\n');
        size_t cbLine;
        if (pszNextLine)
        {
            pszNextLine++;
            cbLine = pszNextLine - pszLine;
        }
        else
            cbLine = strlen(pszLine);
        if (fwrite(pszLine, 1, cbLine, pFileOut) != cbLine)
        {
            fprintf(stderr, "split-soapC: Failed to write to output file\n");
            rcExit = RTEXITCODE_FAILURE;
            break;
        }
        cbWritten += cbLine;

        /* process nesting depth information */
        if (!strncmp(pszLine, "#if", 3))
            cIfNesting++;
        else if (!strncmp(pszLine, "#endif", 6))
        {
            cIfNesting--;
            if (!cBraceNesting && !cIfNesting)
                fJustZero = true;
        }
        else
        {
            for (const char *p = pszLine; p < pszLine + cbLine; p++)
            {
                if (*p == '{')
                    cBraceNesting++;
                else if (*p == '}')
                {
                    cBraceNesting--;
                    if (!cBraceNesting && !cIfNesting)
                        fJustZero = true;
                }
            }
        }

        /* look for static variables used for enum conversion. */
        if (!strncmp(pszLine, "static const struct soap_code_map", sizeof("static const struct soap_code_map") - 1))
            cLinesSinceStaticMap = 0;
        else
            cLinesSinceStaticMap++;

        /* start a new output file if necessary and possible */
        if (   cbWritten >= cbLimit
            && cIfNesting == 0
            && fJustZero
            && cFiles < cChunks
            && cLinesSinceStaticMap > 150 /*hack!*/)
        {
            fclose(pFileOut);
            pFileOut = NULL;
        }

        fJustZero = false;
        pszLine = pszNextLine;
    } while (pszLine);

    printf("split-soapC: Created %lu files.\n", (unsigned long)cFiles);

    free(pszBuffer);
    if (pFileOut)
        fclose(pFileOut);

    return rcExit;
}
