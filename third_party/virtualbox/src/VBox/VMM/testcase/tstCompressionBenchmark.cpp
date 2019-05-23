/* $Id: tstCompressionBenchmark.cpp $ */
/** @file
 * Compression Benchmark for SSM and PGM.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/buildconfig.h>
#include <iprt/crc.h>
#include <iprt/ctype.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/md5.h>
#include <iprt/sha.h>
#include <iprt/mem.h>
#include <iprt/param.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/time.h>
#include <iprt/zip.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static size_t   g_cPages = 20*_1M / PAGE_SIZE;
static size_t   g_cbPages;
static uint8_t *g_pabSrc;

/** Buffer for the decompressed data (g_cbPages). */
static uint8_t *g_pabDecompr;

/** Buffer for the compressed data (g_cbComprAlloc). */
static uint8_t *g_pabCompr;
/** The current size of the compressed data, ComprOutCallback */
static size_t   g_cbCompr;
/** The current offset into the compressed data, DecomprInCallback. */
static size_t   g_offComprIn;
/** The amount of space allocated for compressed data. */
static size_t   g_cbComprAlloc;


/**
 * Store compressed data in the g_pabCompr buffer.
 */
static DECLCALLBACK(int) ComprOutCallback(void *pvUser, const void *pvBuf, size_t cbBuf)
{
    NOREF(pvUser);
    AssertReturn(g_cbCompr + cbBuf <= g_cbComprAlloc, VERR_BUFFER_OVERFLOW);
    memcpy(&g_pabCompr[g_cbCompr], pvBuf, cbBuf);
    g_cbCompr += cbBuf;
    return VINF_SUCCESS;
}

/**
 * Read compressed data from g_pabComrp.
 */
static DECLCALLBACK(int) DecomprInCallback(void *pvUser, void *pvBuf, size_t cbBuf, size_t *pcbBuf)
{
    NOREF(pvUser);
    size_t cb = RT_MIN(cbBuf, g_cbCompr - g_offComprIn);
    if (pcbBuf)
        *pcbBuf = cb;
//    AssertReturn(cb > 0, VERR_EOF);
    memcpy(pvBuf, &g_pabCompr[g_offComprIn], cb);
    g_offComprIn += cb;
    return VINF_SUCCESS;
}


/**
 * Benchmark RTCrc routines potentially relevant for SSM or PGM - All in one go.
 *
 * @param  pabSrc   Pointer to the test data.
 * @param  cbSrc    The size of the test data.
 */
static void tstBenchmarkCRCsAllInOne(uint8_t const *pabSrc, size_t cbSrc)
{
    RTPrintf("Algorithm     Speed                  Time      Digest\n"
             "------------------------------------------------------------------------------\n");

    uint64_t NanoTS = RTTimeNanoTS();
    uint32_t u32Crc = RTCrc32(pabSrc, cbSrc);
    NanoTS = RTTimeNanoTS() - NanoTS;
    unsigned uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTPrintf("CRC-32    %'9u KB/s  %'15llu ns - %08x\n", uSpeed, NanoTS, u32Crc);


    NanoTS = RTTimeNanoTS();
    uint64_t u64Crc = RTCrc64(pabSrc, cbSrc);
    NanoTS = RTTimeNanoTS() - NanoTS;
    uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTPrintf("CRC-64    %'9u KB/s  %'15llu ns - %016llx\n", uSpeed, NanoTS, u64Crc);

    NanoTS = RTTimeNanoTS();
    u32Crc = RTCrcAdler32(pabSrc, cbSrc);
    NanoTS = RTTimeNanoTS() - NanoTS;
    uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTPrintf("Adler-32  %'9u KB/s  %'15llu ns - %08x\n", uSpeed, NanoTS, u32Crc);

    NanoTS = RTTimeNanoTS();
    uint8_t abMd5Hash[RTMD5HASHSIZE];
    RTMd5(pabSrc, cbSrc, abMd5Hash);
    NanoTS = RTTimeNanoTS() - NanoTS;
    uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    char szDigest[257];
    RTMd5ToString(abMd5Hash, szDigest, sizeof(szDigest));
    RTPrintf("MD5       %'9u KB/s  %'15llu ns - %s\n", uSpeed, NanoTS, szDigest);

    NanoTS = RTTimeNanoTS();
    uint8_t abSha1Hash[RTSHA1_HASH_SIZE];
    RTSha1(pabSrc, cbSrc, abSha1Hash);
    NanoTS = RTTimeNanoTS() - NanoTS;
    uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTSha1ToString(abSha1Hash, szDigest, sizeof(szDigest));
    RTPrintf("SHA-1     %'9u KB/s  %'15llu ns - %s\n", uSpeed, NanoTS, szDigest);

    NanoTS = RTTimeNanoTS();
    uint8_t abSha256Hash[RTSHA256_HASH_SIZE];
    RTSha256(pabSrc, cbSrc, abSha256Hash);
    NanoTS = RTTimeNanoTS() - NanoTS;
    uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTSha256ToString(abSha256Hash, szDigest, sizeof(szDigest));
    RTPrintf("SHA-256   %'9u KB/s  %'15llu ns - %s\n", uSpeed, NanoTS, szDigest);

    NanoTS = RTTimeNanoTS();
    uint8_t abSha512Hash[RTSHA512_HASH_SIZE];
    RTSha512(pabSrc, cbSrc, abSha512Hash);
    NanoTS = RTTimeNanoTS() - NanoTS;
    uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTSha512ToString(abSha512Hash, szDigest, sizeof(szDigest));
    RTPrintf("SHA-512   %'9u KB/s  %'15llu ns - %s\n", uSpeed, NanoTS, szDigest);
}


/**
 * Benchmark RTCrc routines potentially relevant for SSM or PGM - Page by page.
 *
 * @param  pabSrc   Pointer to the test data.
 * @param  cbSrc    The size of the test data.
 */
static void tstBenchmarkCRCsPageByPage(uint8_t const *pabSrc, size_t cbSrc)
{
    RTPrintf("Algorithm     Speed                  Time     \n"
             "----------------------------------------------\n");

    size_t const cPages = cbSrc / PAGE_SIZE;

    uint64_t NanoTS = RTTimeNanoTS();
    for (uint32_t iPage = 0; iPage < cPages; iPage++)
        RTCrc32(&pabSrc[iPage * PAGE_SIZE], PAGE_SIZE);
    NanoTS = RTTimeNanoTS() - NanoTS;
    unsigned uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTPrintf("CRC-32    %'9u KB/s  %'15llu ns\n", uSpeed, NanoTS);


    NanoTS = RTTimeNanoTS();
    for (uint32_t iPage = 0; iPage < cPages; iPage++)
        RTCrc64(&pabSrc[iPage * PAGE_SIZE], PAGE_SIZE);
    NanoTS = RTTimeNanoTS() - NanoTS;
    uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTPrintf("CRC-64    %'9u KB/s  %'15llu ns\n", uSpeed, NanoTS);

    NanoTS = RTTimeNanoTS();
    for (uint32_t iPage = 0; iPage < cPages; iPage++)
        RTCrcAdler32(&pabSrc[iPage * PAGE_SIZE], PAGE_SIZE);
    NanoTS = RTTimeNanoTS() - NanoTS;
    uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTPrintf("Adler-32  %'9u KB/s  %'15llu ns\n", uSpeed, NanoTS);

    NanoTS = RTTimeNanoTS();
    uint8_t abMd5Hash[RTMD5HASHSIZE];
    for (uint32_t iPage = 0; iPage < cPages; iPage++)
        RTMd5(&pabSrc[iPage * PAGE_SIZE], PAGE_SIZE, abMd5Hash);
    NanoTS = RTTimeNanoTS() - NanoTS;
    uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTPrintf("MD5       %'9u KB/s  %'15llu ns\n", uSpeed, NanoTS);

    NanoTS = RTTimeNanoTS();
    uint8_t abSha1Hash[RTSHA1_HASH_SIZE];
    for (uint32_t iPage = 0; iPage < cPages; iPage++)
        RTSha1(&pabSrc[iPage * PAGE_SIZE], PAGE_SIZE, abSha1Hash);
    NanoTS = RTTimeNanoTS() - NanoTS;
    uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTPrintf("SHA-1     %'9u KB/s  %'15llu ns\n", uSpeed, NanoTS);

    NanoTS = RTTimeNanoTS();
    uint8_t abSha256Hash[RTSHA256_HASH_SIZE];
    for (uint32_t iPage = 0; iPage < cPages; iPage++)
        RTSha256(&pabSrc[iPage * PAGE_SIZE], PAGE_SIZE, abSha256Hash);
    NanoTS = RTTimeNanoTS() - NanoTS;
    uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTPrintf("SHA-256   %'9u KB/s  %'15llu ns\n", uSpeed, NanoTS);

    NanoTS = RTTimeNanoTS();
    uint8_t abSha512Hash[RTSHA512_HASH_SIZE];
    for (uint32_t iPage = 0; iPage < cPages; iPage++)
        RTSha512(&pabSrc[iPage * PAGE_SIZE], PAGE_SIZE, abSha512Hash);
    NanoTS = RTTimeNanoTS() - NanoTS;
    uSpeed = (unsigned)(cbSrc / (long double)NanoTS * 1000000000.0 / 1024);
    RTPrintf("SHA-512   %'9u KB/s  %'15llu ns\n", uSpeed, NanoTS);
}


/** Prints an error message and returns 1 for quick return from main use. */
static int Error(const char *pszMsgFmt, ...)
{
    RTStrmPrintf(g_pStdErr, "\nerror: ");
    va_list va;
    va_start(va, pszMsgFmt);
    RTStrmPrintfV(g_pStdErr, pszMsgFmt, va);
    va_end(va);
    return 1;
}


int main(int argc, char **argv)
{
    RTR3InitExe(argc, &argv, 0);

    /*
     * Parse arguments.
     */
    static const RTGETOPTDEF    s_aOptions[] =
    {
        { "--iterations",     'i', RTGETOPT_REQ_UINT32 },
        { "--num-pages",      'n', RTGETOPT_REQ_UINT32 },
        { "--page-at-a-time", 'c', RTGETOPT_REQ_UINT32 },
        { "--page-file",      'f', RTGETOPT_REQ_STRING },
        { "--offset",         'o', RTGETOPT_REQ_UINT64 },
    };

    const char     *pszPageFile = NULL;
    uint64_t        offPageFile = 0;
    uint32_t        cIterations = 1;
    uint32_t        cPagesAtATime = 1;
    RTGETOPTUNION   Val;
    RTGETOPTSTATE   State;
    int rc = RTGetOptInit(&State, argc, argv, &s_aOptions[0], RT_ELEMENTS(s_aOptions), 1, 0);
    AssertRCReturn(rc, 1);

    while ((rc = RTGetOpt(&State, &Val)))
    {
        switch (rc)
        {
            case 'n':
                g_cPages = Val.u32;
                if (g_cPages * PAGE_SIZE * 4 / (PAGE_SIZE * 4) != g_cPages)
                    return Error("The specified page count is too high: %#x (%#llx bytes)\n", g_cPages, (uint64_t)g_cPages * PAGE_SHIFT);
                if (g_cPages < 1)
                    return Error("The specified page count is too low: %#x\n", g_cPages);
                break;

            case 'i':
                cIterations = Val.u32;
                if (cIterations < 1)
                    return Error("The number of iterations must be 1 or higher\n");
                break;

            case 'c':
                cPagesAtATime = Val.u32;
                if (cPagesAtATime < 1 || cPagesAtATime > 10240)
                    return Error("The specified pages-at-a-time count is out of range: %#x\n", cPagesAtATime);
                break;

            case 'f':
                pszPageFile = Val.psz;
                break;

            case 'o':
                offPageFile = Val.u64;
                break;

            case 'O':
                offPageFile = Val.u64 * PAGE_SIZE;
                break;

            case 'h':
                RTPrintf("syntax: tstCompressionBenchmark [options]\n"
                         "\n"
                         "Options:\n"
                         "  -h, --help\n"
                         "    Show this help page\n"
                         "  -i, --iterations <num>\n"
                         "    The number of iterations.\n"
                         "  -n, --num-pages <pages>\n"
                         "    The number of pages.\n"
                         "  -c, --pages-at-a-time <pages>\n"
                         "    Number of pages at a time.\n"
                         "  -f, --page-file <filename>\n"
                         "    File or device to read the page from. The default\n"
                         "    is to generate some garbage.\n"
                         "  -o, --offset <file-offset>\n"
                         "    Offset into the page file to start reading at.\n");
                return 0;

            case 'V':
                RTPrintf("%sr%s\n", RTBldCfgVersion(), RTBldCfgRevisionStr());
                return 0;

            default:
                return RTGetOptPrintError(rc, &Val);
        }
    }

    g_cbPages = g_cPages * PAGE_SIZE;
    uint64_t cbTotal = (uint64_t)g_cPages * PAGE_SIZE * cIterations;
    uint64_t cbTotalKB = cbTotal / _1K;
    if (cbTotal / cIterations != g_cbPages)
        return Error("cPages * cIterations -> overflow\n");

    /*
     * Gather the test memory.
     */
    if (pszPageFile)
    {
        size_t cbFile;
        rc = RTFileReadAllEx(pszPageFile, offPageFile, g_cbPages, RTFILE_RDALL_O_DENY_NONE, (void **)&g_pabSrc, &cbFile);
        if (RT_FAILURE(rc))
            return Error("Error reading %zu bytes from %s at %llu: %Rrc\n", g_cbPages, pszPageFile, offPageFile, rc);
        if (cbFile != g_cbPages)
            return Error("Error reading %zu bytes from %s at %llu: got %zu bytes\n", g_cbPages, pszPageFile, offPageFile, cbFile);
    }
    else
    {
        g_pabSrc = (uint8_t *)RTMemAlloc(g_cbPages);
        if (g_pabSrc)
        {
            /* Just fill it with something - warn about the low quality of the something. */
            RTPrintf("tstCompressionBenchmark: WARNING! No input file was specified so the source\n"
                     "buffer will be filled with generated data of questionable quality.\n");
#ifdef RT_OS_LINUX
            RTPrintf("To get real RAM on linux: sudo dd if=/dev/mem ... \n");
#endif
            uint8_t *pb    = g_pabSrc;
            uint8_t *pbEnd = &g_pabSrc[g_cbPages];
            for (; pb != pbEnd; pb += 16)
            {
                char szTmp[17];
                RTStrPrintf(szTmp, sizeof(szTmp), "aaaa%08Xzzzz", (uint32_t)(uintptr_t)pb);
                memcpy(pb, szTmp, 16);
            }
        }
    }

    g_pabDecompr = (uint8_t *)RTMemAlloc(g_cbPages);
    g_cbComprAlloc = RT_MAX(g_cbPages * 2, 256 * PAGE_SIZE);
    g_pabCompr   = (uint8_t *)RTMemAlloc(g_cbComprAlloc);
    if (!g_pabSrc || !g_pabDecompr || !g_pabCompr)
        return Error("failed to allocate memory buffers (g_cPages=%#x)\n", g_cPages);

    /*
     * Double loop compressing and uncompressing the data, where the outer does
     * the specified number of iterations while the inner applies the different
     * compression algorithms.
     */
    struct
    {
        /** The time spent decompressing. */
        uint64_t    cNanoDecompr;
        /** The time spent compressing. */
        uint64_t    cNanoCompr;
        /** The size of the compressed data. */
        uint64_t    cbCompr;
        /** First error. */
        int         rc;
        /** The compression style: block or stream. */
        bool        fBlock;
        /** Compression type.  */
        RTZIPTYPE   enmType;
        /** Compression level.  */
        RTZIPLEVEL  enmLevel;
        /** Method name. */
        const char *pszName;
    } aTests[] =
    {
        { 0, 0, 0, VINF_SUCCESS, false, RTZIPTYPE_STORE, RTZIPLEVEL_DEFAULT, "RTZip/Store"      },
        { 0, 0, 0, VINF_SUCCESS, false, RTZIPTYPE_LZF,   RTZIPLEVEL_DEFAULT, "RTZip/LZF"        },
/*      { 0, 0, 0, VINF_SUCCESS, false, RTZIPTYPE_ZLIB,  RTZIPLEVEL_DEFAULT, "RTZip/zlib"       }, - slow plus it randomly hits VERR_GENERAL_FAILURE atm. */
        { 0, 0, 0, VINF_SUCCESS, true,  RTZIPTYPE_STORE, RTZIPLEVEL_DEFAULT, "RTZipBlock/Store" },
        { 0, 0, 0, VINF_SUCCESS, true,  RTZIPTYPE_LZF,   RTZIPLEVEL_DEFAULT, "RTZipBlock/LZF"   },
        { 0, 0, 0, VINF_SUCCESS, true,  RTZIPTYPE_LZJB,  RTZIPLEVEL_DEFAULT, "RTZipBlock/LZJB"  },
        { 0, 0, 0, VINF_SUCCESS, true,  RTZIPTYPE_LZO,   RTZIPLEVEL_DEFAULT, "RTZipBlock/LZO"   },
    };
    RTPrintf("tstCompressionBenchmark: TESTING..");
    for (uint32_t i = 0; i < cIterations; i++)
    {
        for (uint32_t j = 0; j < RT_ELEMENTS(aTests); j++)
        {
            if (RT_FAILURE(aTests[j].rc))
                continue;
            memset(g_pabCompr,   0xaa, g_cbComprAlloc);
            memset(g_pabDecompr, 0xcc, g_cbPages);
            g_cbCompr = 0;
            g_offComprIn = 0;
            RTPrintf("."); RTStrmFlush(g_pStdOut);

            /*
             * Compress it.
             */
            uint64_t NanoTS = RTTimeNanoTS();
            if (aTests[j].fBlock)
            {
                size_t          cbLeft    = g_cbComprAlloc;
                uint8_t const  *pbSrcPage = g_pabSrc;
                uint8_t        *pbDstPage = g_pabCompr;
                for (size_t iPage = 0; iPage < g_cPages; iPage += cPagesAtATime)
                {
                    AssertBreakStmt(cbLeft > PAGE_SIZE * 4, aTests[j].rc = rc = VERR_BUFFER_OVERFLOW);
                    uint32_t *pcb = (uint32_t *)pbDstPage;
                    pbDstPage    += sizeof(uint32_t);
                    cbLeft       -= sizeof(uint32_t);
                    size_t  cbSrc = RT_MIN(g_cPages - iPage, cPagesAtATime) * PAGE_SIZE;
                    size_t  cbDst;
                    rc = RTZipBlockCompress(aTests[j].enmType, aTests[j].enmLevel, 0 /*fFlags*/,
                                            pbSrcPage, cbSrc,
                                            pbDstPage, cbLeft, &cbDst);
                    if (RT_FAILURE(rc))
                    {
                        Error("RTZipBlockCompress failed for '%s' (#%u): %Rrc\n", aTests[j].pszName, j, rc);
                        aTests[j].rc = rc;
                        break;
                    }
                    *pcb       = (uint32_t)cbDst;
                    cbLeft    -= cbDst;
                    pbDstPage += cbDst;
                    pbSrcPage += cbSrc;
                }
                if (RT_FAILURE(rc))
                    continue;
                g_cbCompr = pbDstPage - g_pabCompr;
            }
            else
            {
                PRTZIPCOMP pZipComp;
                rc = RTZipCompCreate(&pZipComp, NULL, ComprOutCallback, aTests[j].enmType, aTests[j].enmLevel);
                if (RT_FAILURE(rc))
                {
                    Error("Failed to create the compressor for '%s' (#%u): %Rrc\n", aTests[j].pszName, j, rc);
                    aTests[j].rc = rc;
                    continue;
                }

                uint8_t const  *pbSrcPage = g_pabSrc;
                for (size_t iPage = 0; iPage < g_cPages; iPage += cPagesAtATime)
                {
                    size_t cb = RT_MIN(g_cPages - iPage, cPagesAtATime) * PAGE_SIZE;
                    rc = RTZipCompress(pZipComp, pbSrcPage, cb);
                    if (RT_FAILURE(rc))
                    {
                        Error("RTZipCompress failed for '%s' (#%u): %Rrc\n", aTests[j].pszName, j, rc);
                        aTests[j].rc = rc;
                        break;
                    }
                    pbSrcPage += cb;
                }
                if (RT_FAILURE(rc))
                    continue;
                rc = RTZipCompFinish(pZipComp);
                if (RT_FAILURE(rc))
                {
                    Error("RTZipCompFinish failed for '%s' (#%u): %Rrc\n", aTests[j].pszName, j, rc);
                    aTests[j].rc = rc;
                    break;
                }
                RTZipCompDestroy(pZipComp);
            }
            NanoTS = RTTimeNanoTS() - NanoTS;
            aTests[j].cbCompr    += g_cbCompr;
            aTests[j].cNanoCompr += NanoTS;

            /*
             * Decompress it.
             */
            NanoTS = RTTimeNanoTS();
            if (aTests[j].fBlock)
            {
                uint8_t const  *pbSrcPage = g_pabCompr;
                size_t          cbLeft    = g_cbCompr;
                uint8_t        *pbDstPage = g_pabDecompr;
                for (size_t iPage = 0; iPage < g_cPages; iPage += cPagesAtATime)
                {
                    size_t   cbDst = RT_MIN(g_cPages - iPage, cPagesAtATime) * PAGE_SIZE;
                    size_t   cbSrc = *(uint32_t *)pbSrcPage;
                    pbSrcPage     += sizeof(uint32_t);
                    cbLeft        -= sizeof(uint32_t);
                    rc = RTZipBlockDecompress(aTests[j].enmType, 0 /*fFlags*/,
                                              pbSrcPage, cbSrc, &cbSrc,
                                              pbDstPage, cbDst, &cbDst);
                    if (RT_FAILURE(rc))
                    {
                        Error("RTZipBlockDecompress failed for '%s' (#%u): %Rrc\n", aTests[j].pszName, j, rc);
                        aTests[j].rc = rc;
                        break;
                    }
                    pbDstPage += cbDst;
                    cbLeft    -= cbSrc;
                    pbSrcPage += cbSrc;
                }
                if (RT_FAILURE(rc))
                    continue;
            }
            else
            {
                PRTZIPDECOMP pZipDecomp;
                rc = RTZipDecompCreate(&pZipDecomp, NULL, DecomprInCallback);
                if (RT_FAILURE(rc))
                {
                    Error("Failed to create the decompressor for '%s' (#%u): %Rrc\n", aTests[j].pszName, j, rc);
                    aTests[j].rc = rc;
                    continue;
                }

                uint8_t *pbDstPage = g_pabDecompr;
                for (size_t iPage = 0; iPage < g_cPages; iPage += cPagesAtATime)
                {
                    size_t cb = RT_MIN(g_cPages - iPage, cPagesAtATime) * PAGE_SIZE;
                    rc = RTZipDecompress(pZipDecomp, pbDstPage, cb, NULL);
                    if (RT_FAILURE(rc))
                    {
                        Error("RTZipDecompress failed for '%s' (#%u): %Rrc\n", aTests[j].pszName, j, rc);
                        aTests[j].rc = rc;
                        break;
                    }
                    pbDstPage += cb;
                }
                RTZipDecompDestroy(pZipDecomp);
                if (RT_FAILURE(rc))
                    continue;
            }
            NanoTS = RTTimeNanoTS() - NanoTS;
            aTests[j].cNanoDecompr += NanoTS;

            if (memcmp(g_pabDecompr, g_pabSrc, g_cbPages))
            {
                Error("The compressed data doesn't match the source for '%s' (%#u)\n", aTests[j].pszName, j);
                aTests[j].rc = VERR_BAD_EXE_FORMAT;
                continue;
            }
        }
    }
    if (RT_SUCCESS(rc))
        RTPrintf("\n");

    /*
     * Report the results.
     */
    rc = 0;
    RTPrintf("tstCompressionBenchmark: BEGIN RESULTS\n");
    RTPrintf("%-20s           Compression                                             Decompression\n", "");
    RTPrintf("%-20s        In             Out      Ratio         Size                In             Out\n", "Method");
    RTPrintf("%.20s-----------------------------------------------------------------------------------------\n", "---------------------------------------------");
    for (uint32_t j = 0; j < RT_ELEMENTS(aTests); j++)
    {
        if (RT_SUCCESS(aTests[j].rc))
        {
            unsigned uComprSpeedIn    = (unsigned)(cbTotalKB         / (long double)aTests[j].cNanoCompr   * 1000000000.0);
            unsigned uComprSpeedOut   = (unsigned)(aTests[j].cbCompr / (long double)aTests[j].cNanoCompr   * 1000000000.0 / 1024);
            unsigned uRatio           = (unsigned)(aTests[j].cbCompr / cIterations * 100 / g_cbPages);
            unsigned uDecomprSpeedIn  = (unsigned)(aTests[j].cbCompr / (long double)aTests[j].cNanoDecompr * 1000000000.0 / 1024);
            unsigned uDecomprSpeedOut = (unsigned)(cbTotalKB         / (long double)aTests[j].cNanoDecompr * 1000000000.0);
            RTPrintf("%-20s %'9u KB/s  %'9u KB/s  %3u%%  %'11llu bytes   %'9u KB/s  %'9u KB/s",
                     aTests[j].pszName,
                     uComprSpeedIn,   uComprSpeedOut, uRatio, aTests[j].cbCompr / cIterations,
                     uDecomprSpeedIn, uDecomprSpeedOut);
#if 0
            RTPrintf("  [%'14llu / %'14llu ns]\n",
                     aTests[j].cNanoCompr / cIterations,
                     aTests[j].cNanoDecompr / cIterations);
#else
            RTPrintf("\n");
#endif
        }
        else
        {
            RTPrintf("%-20s: %Rrc\n", aTests[j].pszName, aTests[j].rc);
            rc = 1;
        }
    }
    if (pszPageFile)
        RTPrintf("Input: %'10zu pages from '%s' starting at offset %'lld (%#llx)\n"
                 "                                                           %'11zu bytes\n",
                 g_cPages, pszPageFile, offPageFile, offPageFile, g_cbPages);
    else
        RTPrintf("Input: %'10zu pages of generated rubbish               %'11zu bytes\n",
                 g_cPages, g_cbPages);

    /*
     * Count zero pages in the data set.
     */
    size_t cZeroPages = 0;
    for (size_t iPage = 0; iPage < g_cPages; iPage++)
    {
        if (ASMMemIsZeroPage(&g_pabSrc[iPage * PAGE_SIZE]))
            cZeroPages++;
    }
    RTPrintf("       %'10zu zero pages (%u %%)\n", cZeroPages, cZeroPages * 100 / g_cPages);

    /*
     * A little extension to the test, benchmark relevant CRCs.
     */
    RTPrintf("\n"
             "tstCompressionBenchmark: Hash/CRC - All In One\n");
    tstBenchmarkCRCsAllInOne(g_pabSrc, g_cbPages);

    RTPrintf("\n"
             "tstCompressionBenchmark: Hash/CRC - Page by Page\n");
    tstBenchmarkCRCsPageByPage(g_pabSrc, g_cbPages);

    RTPrintf("\n"
             "tstCompressionBenchmark: Hash/CRC - Zero Page Digest\n");
    static uint8_t s_abZeroPg[PAGE_SIZE];
    RT_ZERO(s_abZeroPg);
    tstBenchmarkCRCsAllInOne(s_abZeroPg, PAGE_SIZE);

    RTPrintf("\n"
             "tstCompressionBenchmark: Hash/CRC - Zero Half Page Digest\n");
    tstBenchmarkCRCsAllInOne(s_abZeroPg, PAGE_SIZE / 2);

    RTPrintf("tstCompressionBenchmark: END RESULTS\n");

    return rc;
}

