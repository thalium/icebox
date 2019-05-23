/* $Id: kPrf2Read.cpp 77 2016-06-22 17:03:55Z bird $ */
/** @file
 * kProfiler Mark 2 - The reader and producer of statistics.
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <k/kDbg.h>


/** @def KPRF_OFF2PTR
 * Internal helper for converting a offset to a pointer.
 * @internal
 */
#define KPRF_OFF2PTR(TypePrefix, TypeName, off, pHdr) \
    ( (KPRF_TYPE(TypePrefix, TypeName)) ((off) + (KUPTR)pHdr) )

/** @def KPRF_ALIGN
 * The usual align macro.
 * @internal
 */
#define KPRF_ALIGN(n, align) ( ((n) + ( (align) - 1)) & ~((align) - 1) )

/** @def KPRF_OFFSETOF
 * My usual extended offsetof macro, except this returns KU32 and mangles the type name.
 * @internal
 */
#define KPRF_OFFSETOF(kPrfType, Member) ( (KU32)(KUPTR)&((KPRF_TYPE(P,kPrfType))0)->Member )

/** @def PRF_SIZEOF
 * Size of a kPrf type.
 * @internal
 */
#define KPRF_SIZEOF(kPrfType)           sizeof(KPRF_TYPE(,kPrfType))

#ifdef _MSC_VER
# define KPRF_FMT_U64   "I64u"
# define KPRF_FMT_X64   "I64x"
# define KPRF_FMT_I64   "I64d"
#else
# define KPRF_FMT_X64   "llx"
# define KPRF_FMT_U64   "llu"
# define KPRF_FMT_I64   "lld"
#endif


/*
 * Instantiate the readers.
 */
/* 32-bit */
#define KPRF_NAME(Suffix)               KPrf32##Suffix
#define KPRF_TYPE(Prefix,Suffix)        Prefix##KPRF32##Suffix
#define KPRF_BITS                       32
#define KPRF_FMT_UPTR                   "#010x"

#include "prfcore.h.h"
#include "prfreader.cpp.h"

#undef KPRF_FMT_UPTR
#undef KPRF_NAME
#undef KPRF_TYPE
#undef KPRF_BITS

/* 64-bit */
#define KPRF_NAME(Suffix)               KPrf64##Suffix
#define KPRF_TYPE(Prefix,Suffix)        Prefix##KPRF64##Suffix
#define KPRF_BITS                       64
#ifdef _MSC_VER
# define KPRF_FMT_UPTR                  "#018I64x"
#else
# define KPRF_FMT_UPTR                  "#018llx"
#endif

#include "prfcore.h.h"
#include "prfreader.cpp.h"

#undef KPRF_FMT_UPTR
#undef KPRF_NAME
#undef KPRF_TYPE
#undef KPRF_BITS


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Header union type.
 */
typedef union KPRFHDR
{
    KPRF32HDR   Hdr32;
    KPRF64HDR   Hdr64;
} KPRFHDR;
typedef KPRFHDR *PKPRFHDR;
typedef const KPRFHDR *PCKPRFHDR;



/**
 * Read the data set into memory.
 *
 * @returns Pointer to the loaded data set. (release using free()).
 *
 * @param   pszFilename         The path to the profiler data set.
 * @param   pcb                 Where to store the size of the data set.
 * @param   pOut                Where to write errors.
 */
PKPRFHDR kPrfLoad(const char *pszFilename, KU32 *pcb, FILE *pOut)
{
    FILE *pFile = fopen(pszFilename, "rb");
    if (!pFile)
    {
        fprintf(pOut, "Cannot open '%s' for reading!\n", pszFilename);
        return NULL;
    }

    /*
     * Read the file into memory.
     */
    long cbFile;
    if (    !fseek(pFile, 0, SEEK_END)
        &&  (cbFile = ftell(pFile)) >= 0
        &&  !fseek(pFile, 0, SEEK_SET)
        )
    {
        if (pcb)
            *pcb = cbFile;

        void *pvData = malloc(cbFile);
        if (pvData)
        {
            if (fread(pvData, cbFile, 1, pFile))
            {

                fclose(pFile);
                return (PKPRFHDR)pvData;
            }
            fprintf(pOut, "Failed reading '%s' into memory!\n", pszFilename);
            free(pvData);
        }
        else
            fprintf(pOut, "Failed to allocate %ld bytes of memory for reading the file '%s' into!\n", cbFile, pszFilename);
    }
    else
        fprintf(pOut, "Failed to determin the size of '%s'!\n", pszFilename);

    fclose(pFile);
    return NULL;
}


/**
 * Validates the data set
 *
 * @returns true if valid.
 * @returns false if invalid.
 *
 * @param   pHdr        Pointer to the data set.
 * @param   cb          The size of the data set.
 * @param   pOut        Where to write error messages.
 */
static bool kPrfIsValidate(PCKPRFHDR pHdr, KU32 cb, FILE *pOut)
{
    /*
     * We ASSUMES that the header is identicial with the exception
     * of the uBasePtr size. (this is padded out and the upper bits are all zero)
     */

    if (    pHdr->Hdr32.u32Magic != KPRF32HDR_MAGIC
        &&  pHdr->Hdr32.u32Magic != KPRF64HDR_MAGIC)
    {
        fprintf(pOut, "Invalid magic %#x\n", pHdr->Hdr32.u32Magic);
        return false;
    }

    if (    pHdr->Hdr32.cFormatBits != 32
        &&  pHdr->Hdr32.cFormatBits != 64)
    {
        fprintf(pOut, "Invalid/Unsupported bit count %u\n", pHdr->Hdr32.cFormatBits);
        return false;
    }

    if (pHdr->Hdr32.cb > cb)
    {
        fprintf(pOut, "Data set size mismatch. Header say %#x, input is %#x\n", pHdr->Hdr32.cb, cb);
        return false;
    }

#define KPRF_VALIDATE_SIZE(MemBaseName, cb32, cb64) do {\
        if (pHdr->Hdr32.cb##MemBaseName > (pHdr->Hdr32.cFormatBits == 32 ? cb32 : cb64)) \
        { \
            fprintf(pOut, "cb" #MemBaseName " was expected to be %#x but is %#x. Probably a format change, rebuild.\n", \
                    (unsigned)(pHdr->Hdr32.cFormatBits == 32 ? cb32 : cb64), pHdr->Hdr32.cb##MemBaseName); \
            return false; \
        }\
    } while (0)

    KPRF_VALIDATE_SIZE(Function, sizeof(KPRF32FUNC), sizeof(KPRF64FUNC));
    KPRF_VALIDATE_SIZE(Thread, sizeof(KPRF32THREAD), sizeof(KPRF64THREAD));
    KPRF_VALIDATE_SIZE(Stack,
                       (KU32)&((PKPRF32STACK)0)->aFrames[pHdr->Hdr32.cMaxStackFrames],
                       (KU32)&((PKPRF64STACK)0)->aFrames[pHdr->Hdr32.cMaxStackFrames]);

    KUPTR cbHeader = (KUPTR)&pHdr->Hdr32.aiFunctions[pHdr->Hdr32.cFunctions] - (KUPTR)pHdr;
    if (    cbHeader != (KU32)cbHeader
        ||  cbHeader >= cb)
    {
        fprintf(pOut, "cFunctions (%#x) is too large to fit the lookup table inside the data set.\n",
                pHdr->Hdr32.cFunctions);
        return false;
    }

    /* The space assignment is hereby required to be equal to the member order in the header. */
    KU32 offMin = cbHeader;
#define KPRF_VALIDATE_OFF(off, name) do {\
        if (    off > 0 \
            &&  off < offMin) \
        { \
            fprintf(pOut, #name " (%#x) is overlapping with other element or invalid space assignment order.\n", off); \
            return false; \
        }\
        if (off >= cb) \
        { \
            fprintf(pOut, #name " (%#x) is outside the data set (%#x)\n", off, cb); \
            return false; \
        }\
    } while (0)
#define KPRF_VALIDATE_MEM(MemBaseName) do {\
        KPRF_VALIDATE_OFF(pHdr->Hdr32.off##MemBaseName##s, off##MemBaseName##s); \
        if (    pHdr->Hdr32.off##MemBaseName##s \
            &&  (   pHdr->Hdr32.off##MemBaseName##s + pHdr->Hdr32.cb##MemBaseName * pHdr->Hdr32.cMax##MemBaseName##s > cb \
                 || pHdr->Hdr32.off##MemBaseName##s + pHdr->Hdr32.cb##MemBaseName * pHdr->Hdr32.cMax##MemBaseName##s < pHdr->Hdr32.off##MemBaseName##s)\
           ) \
        { \
            fprintf(pOut, #MemBaseName " (%#x) is outside the data set (%#x)\n", \
                    pHdr->Hdr32.off##MemBaseName##s + pHdr->Hdr32.cb##MemBaseName * pHdr->Hdr32.cMax##MemBaseName##s, cb); \
            return false; \
        }\
        if (pHdr->Hdr32.c##MemBaseName##s > pHdr->Hdr32.cMax##MemBaseName##s) \
        { \
            fprintf(pOut, "c" #MemBaseName " (%#x) higher than the max (%#x)\n", \
                    pHdr->Hdr32.c##MemBaseName##s, pHdr->Hdr32.cMax##MemBaseName##s); \
            return false; \
        } \
        if (pHdr->Hdr32.off##MemBaseName##s) \
            offMin += pHdr->Hdr32.cb##MemBaseName * pHdr->Hdr32.cMax##MemBaseName##s; \
    } while (0)

    KPRF_VALIDATE_MEM(Function);
    KPRF_VALIDATE_OFF(pHdr->Hdr32.offModSegs, offModSegs);
    if (pHdr->Hdr32.offModSegs)
        KPRF_VALIDATE_OFF(pHdr->Hdr32.offModSegs + pHdr->Hdr32.cbMaxModSegs, cbMaxModSegs);
    if (pHdr->Hdr32.cbModSegs > pHdr->Hdr32.cbMaxModSegs)
    {
        fprintf(pOut, "ccbModSegs (%#x) higher than the max (%#x)\n",
                pHdr->Hdr32.cbModSegs, pHdr->Hdr32.cbMaxModSegs);
        return false;
    }
    if (pHdr->Hdr32.offModSegs) \
        offMin += pHdr->Hdr32.cbMaxModSegs; \
    KPRF_VALIDATE_MEM(Thread);
    KPRF_VALIDATE_MEM(Stack);
    KPRF_VALIDATE_OFF(pHdr->Hdr32.offCommandLine, offCommandLine);
    KPRF_VALIDATE_OFF(pHdr->Hdr32.offCommandLine + pHdr->Hdr32.cchCommandLine, cchCommandLine);

    /*
     * Validate the function lookup table
     */
    for (KU32 i = 0; i < pHdr->Hdr32.cFunctions; i++)
        if (pHdr->Hdr32.aiFunctions[i] >= pHdr->Hdr32.cFunctions)
        {
            fprintf(pOut, "Function lookup entry %#x is invalid: index %#x, max is %#x\n",
                    i, pHdr->Hdr32.aiFunctions[i], pHdr->Hdr32.cFunctions);
            return false;
        }

    /*
     * Validate the functions.
     */
    switch (pHdr->Hdr32.cFormatBits)
    {
        case 32:
            return KPrf32IsValid(&pHdr->Hdr32, cb, pOut);

        case 64:
            return KPrf64IsValid(&pHdr->Hdr64, cb, pOut);
    }
    return false;
#undef KPRF_VALIDATE_SIZE
#undef KPRF_VALIDATE_MEM
#undef KPRF_VALIDATE_OFF
}


/**
 * Dumps a kProfiler 2 format file.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 *
 * @param   pszFilename         The path to the profiler data set.
 * @param   pOut                Where to write the output.
 */
int KPrfDumpFile(const char *pszFilename, FILE *pOut)
{
    /*
     * Load and validate the data set.
     */
    KU32 cb;
    PKPRFHDR pHdr = kPrfLoad(pszFilename, &cb, pOut);
    if (!pHdr)
        return -1;
    if (!kPrfIsValidate(pHdr, cb, pOut))
        return -1;

    /*
     * Switch to the appropirate dumper routine.
     */
    int rc;
    switch (pHdr->Hdr32.cFormatBits)
    {
        case 32:
            rc = KPrf32Dump(&pHdr->Hdr32, pOut);
            break;

        case 64:
            rc = KPrf64Dump(&pHdr->Hdr64, pOut);
            break;

        default:
            fprintf(stderr, "Unsupported bit count %d\n", pHdr->Hdr32.cFormatBits);
            rc = -1;
            break;
    }

    return rc;
}


/**
 * Creates a HTML report from a kProfiler 2 format file.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 *
 * @param   pszFilename         The path to the profiler data set.
 * @param   pOut                Where to write the output.
 */
int KPrfHtmlReport(const char *pszFilename, FILE *pOut)
{
    /*
     * Load and validate the data set.
     */
    KU32 cb;
    PKPRFHDR pHdr = kPrfLoad(pszFilename, &cb, pOut);
    if (!pHdr)
        return -1;
    if (!kPrfIsValidate(pHdr, cb, pOut))
        return -1;

    /*
     * Switch to the appropirate dumper routine.
     */
    int rc;
    switch (pHdr->Hdr32.cFormatBits)
    {
        case 32:
        {
            PKPRF32REPORT pReport;
            rc = KPrf32Analyse(&pHdr->Hdr32, &pReport);
            if (!rc)
            {
                rc = KPrf32WriteHtmlReport(pReport, pOut);
                if (rc)
                    fprintf(stderr, "Error while writing HTML report for '%s'\n", pszFilename);
                KPrf32DeleteReport(pReport);
            }
            else
                fprintf(stderr, "Analysis of '%s' failed!\n", pszFilename);
            break;
        }

        case 64:
        {
            PKPRF64REPORT pReport;
            rc = KPrf64Analyse(&pHdr->Hdr64, &pReport);
            if (!rc)
            {
                rc = KPrf64WriteHtmlReport(pReport, pOut);
                if (rc)
                    fprintf(stderr, "Error while writing HTML report for '%s'\n", pszFilename);
                KPrf64DeleteReport(pReport);
            }
            else
                fprintf(stderr, "Analysis of '%s' failed!\n", pszFilename);
            break;
        }

        default:
            fprintf(stderr, "Unsupported bit count %d\n", pHdr->Hdr32.cFormatBits);
            rc = -1;
            break;
    }

    return rc;
}



/**
 * Prints the usage.
 */
static int Usage(void)
{
    printf("kProfiler MK2 - Reader & Producer of Statistics\n"
           "usage: kPrf2Read [-r|-d] <file1> [[-r|-d] file2 []]\n"
           );
    return 1;
}


int main(int argc, char **argv)
{
    /*
     * Parse arguments.
     */
    if (argc <= 1)
        return Usage();
    enum { OP_DUMP, OP_HTML } enmOp = OP_DUMP;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
                case 'h':
                case 'H':
                case '?':
                case '-':
                    return Usage();

                case 'd':
                    enmOp = OP_DUMP;
                    break;

                case 'r':
                    enmOp = OP_HTML;
                    break;

                default:
                    printf("Syntax error: Unknown argument '%s'\n", argv[i]);
                    return 1;
            }
        }
        else
        {
            int rc;
            switch (enmOp)
            {
                case OP_DUMP:
                    rc = KPrfDumpFile(argv[i], stdout);
                    break;
                case OP_HTML:
                    rc = KPrfHtmlReport(argv[i], stdout);
                    break;
            }
            if (rc)
                return rc;
        }
    }

    return 0;
}
