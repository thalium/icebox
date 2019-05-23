/* $Id: tstRTDigest-2.cpp $ */
/** @file
 * IPRT Testcase - Checksums and Digests.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
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
#include <iprt/crypto/digest.h>
#include <iprt/md2.h>
#include <iprt/md5.h>
#include <iprt/sha.h>

#include <iprt/asm.h>
#include <iprt/err.h>
#include <iprt/test.h>
#include <iprt/thread.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef struct TESTRTDIGEST
{
    /** Pointer to the input. */
    void const *pvInput;
    /** The size of the input. */
    size_t      cbInput;
    /** The expected digest. */
    const char *pszDigest;
    /** Clue what this is. Can be NULL. */
    const char *pszTest;
} TESTRTDIGEST;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
#include "72kb-random.h"


#define CHECK_STRING(a_pszActual, a_pszExpected) \
    do { \
        if (strcmp(a_pszActual, a_pszExpected)) \
            RTTestIFailed("line %u: Expected %s, got %s.", __LINE__, a_pszExpected, a_pszActual); \
    } while (0)


/**
 * Worker for testGeneric that finalizes the digest and compares it with what is
 * expected.
 *
 * @returns true on success, false on failure.
 * @param   hDigest             The digest to finalize and check.
 * @param   iTest               The test number (for reporting).
 * @param   pTest               The test.
 */
static bool testGenericCheckResult(RTCRDIGEST hDigest, uint32_t iTest, TESTRTDIGEST const *pTest)
{
    RTTESTI_CHECK_RC_RET(RTCrDigestFinal(hDigest, NULL, 0), VINF_SUCCESS, false);

    char szDigest[_4K * 2];
    RTTESTI_CHECK_RC_RET(RTStrPrintHexBytes(szDigest, sizeof(szDigest),
                                            RTCrDigestGetHash(hDigest), RTCrDigestGetHashSize(hDigest), 0 /*fFlags*/),
                         VINF_SUCCESS, false);

    if (strcmp(szDigest, pTest->pszDigest))
    {
        RTTestIFailed("sub-test %#u (%s) failed: Expected %s, got %s.", iTest, pTest->pszTest, pTest->pszDigest, szDigest);
        return false;
    }

    return true;
}


/**
 * Table driven generic digest test case.
 *
 * @param   pszDigestObjId  The object ID of the digest algorithm to test.
 * @param   paTests         The test table.
 * @param   cTests          The number of tests in the table.
 * @param   pszDigestName   The name of the digest.
 * @param   enmDigestType   The digest enum type value.
 */
static void testGeneric(const char *pszDigestObjId, TESTRTDIGEST const *paTests, size_t cTests, const char *pszDigestName,
                        RTDIGESTTYPE enmDigestType)
{
    /*
     * Execute the tests.
     */
    RTCRDIGEST hDigest;
    for (uint32_t iTest = 0; iTest < cTests; iTest++)
    {
        /*
         * The whole thing in one go.
         */
        RTTESTI_CHECK_RC_RETV(RTCrDigestCreateByObjIdString(&hDigest, pszDigestObjId), VINF_SUCCESS);
        uint32_t const cbHash = RTCrDigestGetHashSize(hDigest);
        RTTESTI_CHECK_RETV(cbHash > 0);
        RTTESTI_CHECK_RETV(cbHash < _1K);
        RTTESTI_CHECK_RC_RETV(RTCrDigestUpdate(hDigest, paTests[iTest].pvInput, paTests[iTest].cbInput), VINF_SUCCESS);
        if (!testGenericCheckResult(hDigest, iTest, &paTests[iTest]))
            continue; /* skip the remaining tests if this failed. */

        /*
         * Repeat the test feeding the input in smaller chunks.
         */
        static const uint32_t s_acbChunks[] = { 1, 2, 3, 7, 11, 16, 19, 31, 61, 127, 255, 511, 1023, 1024 };
        for (uint32_t i = 0; i < RT_ELEMENTS(s_acbChunks); i++)
        {
            uint32_t const cbChunk = s_acbChunks[i];

            RTTESTI_CHECK_RC_RETV(RTCrDigestReset(hDigest), VINF_SUCCESS);

            uint8_t const *pbInput = (uint8_t const *)paTests[iTest].pvInput;
            size_t         cbInput = paTests[iTest].cbInput;
            while (cbInput > 0)
            {
                size_t cbUpdate = RT_MIN(cbInput, cbChunk);
                RTTESTI_CHECK_RC_RETV(RTCrDigestUpdate(hDigest, pbInput, cbUpdate), VINF_SUCCESS);
                pbInput += cbUpdate;
                cbInput -= cbUpdate;
            }

            if (!testGenericCheckResult(hDigest, iTest, &paTests[iTest]))
                break; /* No need to test the other permutations if this fails. */
        }

        RTTESTI_CHECK_RC(RTCrDigestRelease(hDigest), 0);
    }

    /*
     * Do a quick benchmark.
     */
    RTTESTI_CHECK_RC_RETV(RTCrDigestCreateByObjIdString(&hDigest, pszDigestObjId), VINF_SUCCESS);

    /* Warmup. */
    uint32_t cChunks  = enmDigestType == RTDIGESTTYPE_MD2 ? 12 : 128;
    uint32_t cLeft    = cChunks;
    int      rc       = VINF_SUCCESS;
    RTThreadYield();
    uint64_t uStartTS = RTTimeNanoTS();
    while (cLeft-- > 0)
        rc |= RTCrDigestUpdate(hDigest, g_abRandom72KB, sizeof(g_abRandom72KB));
    uint64_t cNsPerChunk = (RTTimeNanoTS() - uStartTS) / cChunks;
    if (!cNsPerChunk)
        cNsPerChunk = 16000000 / cChunks; /* Time resolution kludge: 16ms. */
    RTTESTI_CHECK_RETV(rc == VINF_SUCCESS);

    /* Do it for real for about 2 seconds. */
    RTTESTI_CHECK_RC(RTCrDigestReset(hDigest), VINF_SUCCESS);
    cChunks = _2G32 / cNsPerChunk;
    cLeft   = cChunks;
    RTThreadYield();
    uStartTS = RTTimeNanoTS();
    while (cLeft-- > 0)
        rc |= RTCrDigestUpdate(hDigest, g_abRandom72KB, sizeof(g_abRandom72KB));
    uint64_t cNsElapsed = RTTimeNanoTS() - uStartTS;
    RTTESTI_CHECK(rc == VINF_SUCCESS);

    RTTestIValueF((uint64_t)cChunks * sizeof(g_abRandom72KB) / _1K / (0.000000001 * cNsElapsed), RTTESTUNIT_KILOBYTES_PER_SEC,
                  "%s throughput", pszDigestName);
    RTTESTI_CHECK_RC(RTCrDigestRelease(hDigest), 0);
}


/**
 * Tests MD2.
 */
static void testMd2(void)
{
    RTTestISub("MD2");

    /*
     * Some quick direct API tests.
     */
    uint8_t     abHash[RTMD2_HASH_SIZE];
    char        szDigest[RTMD2_DIGEST_LEN + 1];
    const char *pszString;

    pszString = "";
    RTMd2(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd2ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "8350e5a3e24c153df2275c9f80692773");

    pszString = "The quick brown fox jumps over the lazy dog";
    RTMd2(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd2ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "03d85a0d629d2c442e987525319fc471");

    pszString = "a";
    RTMd2(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd2ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "32ec01ec4a6dac72c0ab96fb34c0b5d1");

    pszString = "abc";
    RTMd2(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd2ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "da853b0d3f88d99b30283a69e6ded6bb");

    pszString = "message digest";
    RTMd2(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd2ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "ab4f496bfb2a530b219ff33031fe06b0");

    pszString = "abcdefghijklmnopqrstuvwxyz";
    RTMd2(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd2ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "4e8ddff3650292ab5a4108c3aa47940b");

    pszString = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    RTMd2(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd2ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "da33def2a42df13975352846c30338cd");

    pszString = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
    RTMd2(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd2ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "d5976f79d83d3a0dc9806c3c66f3efd8");


    /*
     * Generic API tests.
     */
    static TESTRTDIGEST const s_abTests[] =
    {
        { &g_abRandom72KB[0],          0, "8350e5a3e24c153df2275c9f80692773", "MD2 0 bytes" },
        { &g_abRandom72KB[0],          1, "142ca23308e886f4249d147914889156", "MD2 1 byte" },
        { &g_abRandom72KB[0],          2, "5e962a3590849aff8038032cb55e4491", "MD2 2 bytes" },
        { &g_abRandom72KB[0],          3, "0d866c59a8b5e1c305570be5ade02d5e", "MD2 3 bytes" },
        { &g_abRandom72KB[0],          4, "dfd68bdf6826b5c8bf55675a0f0b8703", "MD2 4 bytes" },
        { &g_abRandom72KB[0],          5, "33f83255dcceaf8abbafdca5327f9886", "MD2 5 bytes" },
        { &g_abRandom72KB[0],          6, "586a0dac9c469e938adca3e405ba0ac8", "MD2 6 bytes" },
        { &g_abRandom72KB[0],          7, "88427c5c074552dfd2ced41d4fe5e853", "MD2 7 bytes" },
        { &g_abRandom72KB[0],          8, "927ac332618d50e49aae2d5f64c025b4", "MD2 8 bytes" },
        { &g_abRandom72KB[0],          9, "9fcae6b2526d3f089a3cccc31aa8ebf1", "MD2 9 bytes" },
        { &g_abRandom72KB[0],         10, "3ad05f9fd70ad323a0bbe77d2c320456", "MD2 10 bytes" },
        { &g_abRandom72KB[0],         11, "d9bdb0ccacdd65694d19482521256e5d", "MD2 11 bytes" },
        { &g_abRandom72KB[0],         12, "b79cb3a39d25750fa380f6e82c3cc239", "MD2 12 bytes" },
        { &g_abRandom72KB[0],         13, "b7db14f7057edd00e8c769060a3a8cb3", "MD2 13 bytes" },
        { &g_abRandom72KB[0],         14, "c3d3e05a1cb241890cbb164c800c4576", "MD2 14 bytes" },
        { &g_abRandom72KB[0],         15, "08a7b38a5e0891b07a0ed1a69f6037b2", "MD2 15 bytes" },
        { &g_abRandom72KB[0],         16, "7131c0f3b904678579fe4a64b79095a4", "MD2 16 bytes" },
        { &g_abRandom72KB[0],         17, "f91ba6efd6069ff46d604ecfdec15ac9", "MD2 17 bytes" },
        { &g_abRandom72KB[0],         18, "bea228d4e8358268071b440f41e68d21", "MD2 18 bytes" },
        { &g_abRandom72KB[0],         19, "ca4c5c896ed5dc6ac52fb8b5aedcb044", "MD2 19 bytes" },
        { &g_abRandom72KB[0],         20, "9e66429cd582564328b86f72208460f3", "MD2 20 bytes" },
        { &g_abRandom72KB[0],         21, "a366dd52bcd8baf3578e5ccff819074b", "MD2 21 bytes" },
        { &g_abRandom72KB[0],         22, "4f909f0f5101e87bb8652036fcd54fea", "MD2 22 bytes" },
        { &g_abRandom72KB[0],         23, "f74233c43a71736be4ae4d0259658106", "MD2 23 bytes" },
        { &g_abRandom72KB[0],         24, "3a245315355acb62a402646f3eb789e7", "MD2 24 bytes" },
        { &g_abRandom72KB[0],         25, "f9bbc496a41f2f377b9e7a665a503fc4", "MD2 25 bytes" },
        { &g_abRandom72KB[0],         26, "b90cc3e523c58b2ddee36dabf6544961", "MD2 26 bytes" },
        { &g_abRandom72KB[0],         27, "99a035865fbd3fe595afc01140fa8c34", "MD2 27 bytes" },
        { &g_abRandom72KB[0],         28, "8196f2d6989d824b828b483c8bf21390", "MD2 28 bytes" },
        { &g_abRandom72KB[0],         29, "957754cec1095bb3eed25827282c00db", "MD2 29 bytes" },
        { &g_abRandom72KB[0],         30, "8c392b65a3824de6edf94d4c4df064cd", "MD2 30 bytes" },
        { &g_abRandom72KB[0],         31, "cb7276510e070c82e5391e682c262e1d", "MD2 31 bytes" },
        { &g_abRandom72KB[0],         32, "cb6ba252ddd841c483095a58f2c2c78c", "MD2 32 bytes" },
        { &g_abRandom72KB[0],         33, "a74fdcd7798bb3d9153d486e0c2049bf", "MD2 33 bytes" },
        { &g_abRandom72KB[0],         34, "30f1d91b79c01fe15dda2d018e05ce8c", "MD2 34 bytes" },
        { &g_abRandom72KB[0],         35, "d24c79922d3168fc5ea3896cf6517067", "MD2 35 bytes" },
        { &g_abRandom72KB[0],         36, "eea7cb5dc0716cda6a1b97db655db057", "MD2 36 bytes" },
        { &g_abRandom72KB[0],         37, "85223ec3f4b266bb9ef9546dd4c6fbe0", "MD2 37 bytes" },
        { &g_abRandom72KB[0],         38, "4f45bdd2390452ab7ea46448f3e08d72", "MD2 38 bytes" },
        { &g_abRandom72KB[0],         39, "c2e0e4b3fbaab93342e952ff1f45bb3e", "MD2 39 bytes" },
        { &g_abRandom72KB[0],         40, "08967efe427009b4b4f1d6d444ae2897", "MD2 40 bytes" },
        { &g_abRandom72KB[0],         41, "ef66a72fb184b64d07b525dc1a6e0376", "MD2 41 bytes" },
        { &g_abRandom72KB[0],         42, "8e08b6337537406770c27584293ea515", "MD2 42 bytes" },
        { &g_abRandom72KB[0],         43, "08fe64ee6d9a811154d9312c586c0a08", "MD2 43 bytes" },
        { &g_abRandom72KB[0],         44, "4f29656bf1fe579357240d1a364415c5", "MD2 44 bytes" },
        { &g_abRandom72KB[0],         45, "7998eb00e5774b175db8fa91de555aab", "MD2 45 bytes" },
        { &g_abRandom72KB[0],         46, "a7692c36d342ba2b750f1f26fecdac56", "MD2 46 bytes" },
        { &g_abRandom72KB[0],         47, "038f0bb2828f03fcabde6aa9dcb10e6d", "MD2 47 bytes" },
        { &g_abRandom72KB[0],         48, "b178ffa99a7c5caefef81b8601fbd992", "MD2 48 bytes" },
        { &g_abRandom72KB[0],         49, "f139777906b3ca8785a893f4aacd2489", "MD2 49 bytes" },
        { &g_abRandom72KB[0],         50, "e1a57dcc846d8c0ad3853fde8278094e", "MD2 50 bytes" },
        { &g_abRandom72KB[0],         51, "e754995fa0fd61ba9e10a54e12afde4f", "MD2 51 bytes" },
        { &g_abRandom72KB[0],         52, "ddf17522138d919678f44eaeeb1c0901", "MD2 52 bytes" },
        { &g_abRandom72KB[0],         53, "7b588ece72a7dca79fcabfda083b7ac0", "MD2 53 bytes" },
        { &g_abRandom72KB[0],         54, "8b23c8a9261faeb9ebb2853a03f3f2eb", "MD2 54 bytes" },
        { &g_abRandom72KB[0],         55, "89d700920d22d2f4a655add9fce30295", "MD2 55 bytes" },
        { &g_abRandom72KB[0],         56, "c3ece99f88613724c8e6d17af9e728a9", "MD2 56 bytes" },
        { &g_abRandom72KB[0],         57, "271a0f4e6e270fb0264348cff440f36d", "MD2 57 bytes" },
        { &g_abRandom72KB[0],         58, "4b01b8d1f7912dea17b5c8f5aa0654f2", "MD2 58 bytes" },
        { &g_abRandom72KB[0],         59, "ed399fc4cab3cbcc1d65dda49f9496b7", "MD2 59 bytes" },
        { &g_abRandom72KB[0],         60, "59eab0ff60a2da2935cdd548082c3cf9", "MD2 60 bytes" },
        { &g_abRandom72KB[0],         61, "88c5169745df6ef88088bd5d162ef2f9", "MD2 61 bytes" },
        { &g_abRandom72KB[0],         62, "c4711f410d141f7e3fc40464cf117a98", "MD2 62 bytes" },
        { &g_abRandom72KB[0],         63, "d9bf4e2e3692082cf712466c90c7548b", "MD2 63 bytes" },
        { &g_abRandom72KB[0],         64, "cd8e18bede03bbc4620539b3d41091a0", "MD2 64 bytes" },
        { &g_abRandom72KB[0],         65, "4ccbf9b18f0a47a958efd71f4bcff1d5", "MD2 65 bytes" },
        { &g_abRandom72KB[0],         66, "5f0356e384a3ace42ddaa0053494e932", "MD2 66 bytes" },
        { &g_abRandom72KB[0],         67, "c90f40c059da2d14c45c9a6f72fbc06a", "MD2 67 bytes" },
        { &g_abRandom72KB[0],         68, "29d000bccfc0df57d79da8d275c5766d", "MD2 68 bytes" },
        { &g_abRandom72KB[0],         69, "2fb908536b6e0e4ead7aa5e8552a10cb", "MD2 69 bytes" },
        { &g_abRandom72KB[0],         70, "df1e18111f62b07aae8e3f81d74385d1", "MD2 70 bytes" },
        { &g_abRandom72KB[0],         71, "1213eca29ce71f0037527f8347da4c1c", "MD2 71 bytes" },
        { &g_abRandom72KB[0],         72, "bd6686795f936446534646f7d7f35e03", "MD2 72 bytes" },
        { &g_abRandom72KB[0],         73, "b5181c0b712b3aa024e67430edbd816a", "MD2 73 bytes" },
        { &g_abRandom72KB[0],         74, "3b05c33b18ac294e35cd18337d31a842", "MD2 74 bytes" },
        { &g_abRandom72KB[0],         75, "1d6e83621f88667536242ff538872758", "MD2 75 bytes" },
        { &g_abRandom72KB[0],         76, "d830d59fe1cba1da97f19c8750a130d6", "MD2 76 bytes" },
        { &g_abRandom72KB[0],         77, "2d290278d7c502b59a68ca47f22e2ffd", "MD2 77 bytes" },
        { &g_abRandom72KB[0],         78, "7fcf7fa7e54c5189c76c3724811fe105", "MD2 78 bytes" },
        { &g_abRandom72KB[0],         79, "f41c1697d5cb26734108aace37d2f8ee", "MD2 79 bytes" },
        { &g_abRandom72KB[0],         80, "ff814e4823539973251969c88f0aabd2", "MD2 80 bytes" },
        { &g_abRandom72KB[0],         81, "eaf947ace4636dd9fdf5f540d48afa18", "MD2 81 bytes" },
        { &g_abRandom72KB[0],         82, "89b69633a8f41d2f318a59157f7b1135", "MD2 82 bytes" },
        { &g_abRandom72KB[0],         83, "22b6a01cf22dda9aecd29903eb551b96", "MD2 83 bytes" },
        { &g_abRandom72KB[0],         84, "2ab03e9c9674df73c9316ae471275b9e", "MD2 84 bytes" },
        { &g_abRandom72KB[0],         85, "f3c3b113995ad4bb6ac011bce4c8aaeb", "MD2 85 bytes" },
        { &g_abRandom72KB[0],         86, "c47408bcce2ad6d88c27644381829d17", "MD2 86 bytes" },
        { &g_abRandom72KB[0],         87, "6a7eb6a1d8d6e2d32fbfb5e18b4a7d78", "MD2 87 bytes" },
        { &g_abRandom72KB[0],         88, "573d2746448f32b9fb2e7e96c902df5d", "MD2 88 bytes" },
        { &g_abRandom72KB[0],         89, "88043ead96eb9ad170a2a5c04b31473e", "MD2 89 bytes" },
        { &g_abRandom72KB[0],         90, "e08bf4255a9aceec98195d98fe23ca3a", "MD2 90 bytes" },
        { &g_abRandom72KB[0],         91, "8c5d08ff1e6cd07b13ea5c8f2679be54", "MD2 91 bytes" },
        { &g_abRandom72KB[0],         92, "229e41d0dea6c9fc50385a04107d34e1", "MD2 92 bytes" },
        { &g_abRandom72KB[0],         93, "9ba3f41db14506d0d72d2477086fc3b0", "MD2 93 bytes" },
        { &g_abRandom72KB[0],         94, "6838e101797e4af9db93cce7b7f08497", "MD2 94 bytes" },
        { &g_abRandom72KB[0],         95, "a936e8484a013da8fb2f76b1a5e0e577", "MD2 95 bytes" },
        { &g_abRandom72KB[0],         96, "34140248d6f7c3cfe08c52460bd5ae85", "MD2 96 bytes" },
        { &g_abRandom72KB[0],         97, "65022916cd54ab0dfddd5b6a5d88daf6", "MD2 97 bytes" },
        { &g_abRandom72KB[0],         98, "929a96dfeaca781892e1ef0114429d07", "MD2 98 bytes" },
        { &g_abRandom72KB[0],         99, "f3c117eff8a7693a34f60d8364b32fb4", "MD2 99 bytes" },
        { &g_abRandom72KB[0],        100, "b20414388581bc1f001bad02d34db98f", "MD2 100 bytes" },
        { &g_abRandom72KB[0],        101, "2e649b9250edc6a717f7a72ba5ad245b", "MD2 101 bytes" },
        { &g_abRandom72KB[0],        102, "84c30f8dfb3555f24320d1da1fe41845", "MD2 102 bytes" },
        { &g_abRandom72KB[0],        103, "ce647df6e37517be82a63eb5bb06c010", "MD2 103 bytes" },
        { &g_abRandom72KB[0],        104, "e58b4587f8d8654446b495054ca9a3c8", "MD2 104 bytes" },
        { &g_abRandom72KB[0],        105, "d0e5067a2fcdd00cbb058228b9f23a09", "MD2 105 bytes" },
        { &g_abRandom72KB[0],        106, "2666a7d835c27bf8e2b88e5260fc4b08", "MD2 106 bytes" },
        { &g_abRandom72KB[0],        107, "ba5f7980c93a5f63a921d11f74aa0c0b", "MD2 107 bytes" },
        { &g_abRandom72KB[0],        108, "c03ca631eed8a88ddd6aadb6b138da41", "MD2 108 bytes" },
        { &g_abRandom72KB[0],        109, "84c273d3e262c7d0d18f8d32ce6c87ab", "MD2 109 bytes" },
        { &g_abRandom72KB[0],        110, "8abc96f5ef0dd75a64ed4aebdb529e45", "MD2 110 bytes" },
        { &g_abRandom72KB[0],        111, "85d93a90b19e9dcaf2794d61eaf6095f", "MD2 111 bytes" },
        { &g_abRandom72KB[0],        112, "d14a45c32e48b9deee617a4682a6e47a", "MD2 112 bytes" },
        { &g_abRandom72KB[0],        113, "171a52a9b9df939ce634a530e1fdb571", "MD2 113 bytes" },
        { &g_abRandom72KB[0],        114, "bcfd33c67597ea544a54266c5b971ad5", "MD2 114 bytes" },
        { &g_abRandom72KB[0],        115, "76799540a3dace08d09dc1c23e33d7db", "MD2 115 bytes" },
        { &g_abRandom72KB[0],        116, "83ef334faedec13d17f5df823d0f98a1", "MD2 116 bytes" },
        { &g_abRandom72KB[0],        117, "cafcb95eac55ba3a07d8a216dc89bf3c", "MD2 117 bytes" },
        { &g_abRandom72KB[0],        118, "042c1f9c066204e9cb43a8ba6a73195a", "MD2 118 bytes" },
        { &g_abRandom72KB[0],        119, "829e12d96e1a3ce4c51dd70e9774da42", "MD2 119 bytes" },
        { &g_abRandom72KB[0],        120, "162c221e43624b03dee871e2ddf9275a", "MD2 120 bytes" },
        { &g_abRandom72KB[0],        121, "272eba7f65e4704a8200039351f7b7ed", "MD2 121 bytes" },
        { &g_abRandom72KB[0],        122, "4418e4c4ef6b6c9f5d6e35f934c54dc2", "MD2 122 bytes" },
        { &g_abRandom72KB[0],        123, "cbb78508b71600c067923bf200abd2d2", "MD2 123 bytes" },
        { &g_abRandom72KB[0],        124, "133e55a0236279fe2c3c8a616e6a4ec1", "MD2 124 bytes" },
        { &g_abRandom72KB[0],        125, "5aba7624888166ecfcf88607e7bca4ff", "MD2 125 bytes" },
        { &g_abRandom72KB[0],        126, "99fe0c4302b79f84b236eb125af38c9f", "MD2 126 bytes" },
        { &g_abRandom72KB[0],        127, "1c6aacae3441ab225a7aec2deefa5f8a", "MD2 127 bytes" },
        { &g_abRandom72KB[0],        128, "a971703ab10735c6df4e19ff5052da40", "MD2 128 bytes" },
        { &g_abRandom72KB[0],        129, "e7fd473d1f062183a18af88e947d0096", "MD2 129 bytes" },
        { &g_abRandom72KB[0],       1024, "bdccc17a55ced048a6937a2735b09cc6", "MD2 1024 bytes" },
        { &g_abRandom72KB[0],      73001, "54e982ce469bc379a71e2ca755eabe31", "MD2 73001 bytes" },
        { &g_abRandom72KB[0],      73728, "2cf3570a90117130c8879cca30dafb39", "MD2 73728 bytes" },
        { &g_abRandom72KB[0x20c9],  9991, "bbba194efa81238e5b613e20e937144e", "MD2 8393 bytes @9991" },
    };
    testGeneric("1.2.840.113549.2.2", s_abTests, RT_ELEMENTS(s_abTests), "MD2", RTDIGESTTYPE_MD2);
}


/**
 * Tests MD5.
 */
static void testMd5(void)
{
    RTTestISub("MD5");

    /*
     * Some quick direct API tests.
     */
    uint8_t     abHash[RTMD5_HASH_SIZE];
    char        szDigest[RTMD5_DIGEST_LEN + 1];
    const char *pszString;

    pszString = "";
    RTMd5(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd5ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "d41d8cd98f00b204e9800998ecf8427e");

    pszString = "The quick brown fox jumps over the lazy dog";
    RTMd5(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd5ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "9e107d9d372bb6826bd81d3542a419d6");

    pszString = "a";
    RTMd5(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd5ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "0cc175b9c0f1b6a831c399e269772661");

    pszString = "abc";
    RTMd5(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd5ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "900150983cd24fb0d6963f7d28e17f72");

    pszString = "message digest";
    RTMd5(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd5ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "f96b697d7cb7938d525a2f31aaf161d0");

    pszString = "abcdefghijklmnopqrstuvwxyz";
    RTMd5(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd5ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "c3fcd3d76192e4007dfb496cca67e13b");

    pszString = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    RTMd5(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd5ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "d174ab98d277d9f5a5611c2c9f419d9f");

    pszString = "12345678901234567890123456789012345678901234567890123456789012345678901234567890";
    RTMd5(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTMd5ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "57edf4a22be3c955ac49da2e2107b67a");


    /*
     * Generic API tests.
     */
    static TESTRTDIGEST const s_abTests[] =
    {
        { &g_abRandom72KB[0],         0, "d41d8cd98f00b204e9800998ecf8427e", "MD5 0 bytes" },
        { &g_abRandom72KB[0],         1, "785d512be4316d578e6650613b45e934", "MD5 1 bytes" },
        { &g_abRandom72KB[0],         2, "87ed1a6013d72148da0b0294e763048d", "MD5 2 bytes" },
        { &g_abRandom72KB[0],         3, "822de939633727229045cb7789af7df5", "MD5 3 bytes" },
        { &g_abRandom72KB[0],         4, "86287bc09cb9260cf25c132979051304", "MD5 4 bytes" },
        { &g_abRandom72KB[0],         5, "d3d1ce75037d39cf5de6c00f0ed3ca78", "MD5 5 bytes" },
        { &g_abRandom72KB[0],         6, "3dd3a9b4fdf9afecac4ff3a81fa42487", "MD5 6 bytes" },
        { &g_abRandom72KB[0],         7, "15644ca265e05e9c6cf19f297fd061d4", "MD5 7 bytes" },
        { &g_abRandom72KB[0],         8, "da96c8b71cd59cb6af75358ca6c85d7f", "MD5 8 bytes" },
        { &g_abRandom72KB[0],         9, "c2ad718a365c3b3ea6452d18911c9731", "MD5 9 bytes" },
        { &g_abRandom72KB[0],        10, "b5075641b6951182c10596c1f2e4f77a", "MD5 10 bytes" },
        { &g_abRandom72KB[0],        11, "ceea2cb4481c655b7057e1b8dd5a0bd1", "MD5 11 bytes" },
        { &g_abRandom72KB[0],        12, "006e60df0db98aaa0c7a920ea3b663e3", "MD5 12 bytes" },
        { &g_abRandom72KB[0],        13, "3a2cfd399b70235b276203b03dd6eb0e", "MD5 13 bytes" },
        { &g_abRandom72KB[0],        14, "e7829cff3b8c15a6771b9ee8e9dd2084", "MD5 14 bytes" },
        { &g_abRandom72KB[0],        15, "4277413716da82ffee61f36fc25fd5b1", "MD5 15 bytes" },
        { &g_abRandom72KB[0],        16, "823753a79e0677c9b7e41b5725dbce8f", "MD5 16 bytes" },
        { &g_abRandom72KB[0],        17, "3af2c16a115519a00fa8c38f99bdd531", "MD5 17 bytes" },
        { &g_abRandom72KB[0],        18, "e0d640f347fd089338434c4ddc826538", "MD5 18 bytes" },
        { &g_abRandom72KB[0],        19, "d07554497ee83534affa84a5af86c04d", "MD5 19 bytes" },
        { &g_abRandom72KB[0],        20, "b3156aab604ab535832243ff1ce0d3ea", "MD5 20 bytes" },
        { &g_abRandom72KB[0],        21, "a4fb150aab01d4ac4859ecaecf221c63", "MD5 21 bytes" },
        { &g_abRandom72KB[0],        22, "fe6aa2d9c68e100ebaca1bede6be7025", "MD5 22 bytes" },
        { &g_abRandom72KB[0],        23, "db6c3662d9a8950ddcd1b8603a5ab040", "MD5 23 bytes" },
        { &g_abRandom72KB[0],        24, "c56df7b41f9b1420e9e114e82484bcdf", "MD5 24 bytes" },
        { &g_abRandom72KB[0],        25, "7194fb3612d4f53d8eeaa80d60618e49", "MD5 25 bytes" },
        { &g_abRandom72KB[0],        26, "3355f00eac0c85ea8e84c713f55bca3a", "MD5 26 bytes" },
        { &g_abRandom72KB[0],        27, "127b104d22b2cb437ba3b34f6c977e05", "MD5 27 bytes" },
        { &g_abRandom72KB[0],        28, "ce4911fab274a55254cc8989252b47a9", "MD5 28 bytes" },
        { &g_abRandom72KB[0],        29, "f9dd6d60777fee2d654835d5b239979b", "MD5 29 bytes" },
        { &g_abRandom72KB[0],        30, "213779ae9dd7ce0cf199c42ad719da5d", "MD5 30 bytes" },
        { &g_abRandom72KB[0],        31, "5e561c81405437e2205aa64eff81fad3", "MD5 31 bytes" },
        { &g_abRandom72KB[0],        32, "e41238f661fa29883be2f387e56621fb", "MD5 32 bytes" },
        { &g_abRandom72KB[0],        33, "d474457edf9bb331935109b905b2ac9c", "MD5 33 bytes" },
        { &g_abRandom72KB[0],        34, "9c60aa7bb5380442a96fcc960c54f4cd", "MD5 34 bytes" },
        { &g_abRandom72KB[0],        35, "6c27d085e88368d951d0be70bcb83daa", "MD5 35 bytes" },
        { &g_abRandom72KB[0],        36, "e743d29943ddee43e2d3b20373868ace", "MD5 36 bytes" },
        { &g_abRandom72KB[0],        37, "7917ff3a754410f5f3e6a1e34543ad3b", "MD5 37 bytes" },
        { &g_abRandom72KB[0],        38, "d9f6b9d5188e836fa851a5900ac20f3a", "MD5 38 bytes" },
        { &g_abRandom72KB[0],        39, "cef18b503ba9beb5ddf8a70112aaad88", "MD5 39 bytes" },
        { &g_abRandom72KB[0],        40, "39be72035e1058aee305b984373a6b16", "MD5 40 bytes" },
        { &g_abRandom72KB[0],        41, "5f8eda0e0084622bf6233594f06af754", "MD5 41 bytes" },
        { &g_abRandom72KB[0],        42, "c30e55ff2004e7b90009dee503964bbf", "MD5 42 bytes" },
        { &g_abRandom72KB[0],        43, "7a7f33277aa22a9199951022ab96c383", "MD5 43 bytes" },
        { &g_abRandom72KB[0],        44, "4f02d1a4e1ab98f7aed56ba12964af62", "MD5 44 bytes" },
        { &g_abRandom72KB[0],        45, "e0802f8b1739e5c284184d595624392e", "MD5 45 bytes" },
        { &g_abRandom72KB[0],        46, "50ea8f8f8a2bc14f7c07a9ec42826daa", "MD5 46 bytes" },
        { &g_abRandom72KB[0],        47, "365f247ba76a2024d2ff234b4e99bc48", "MD5 47 bytes" },
        { &g_abRandom72KB[0],        48, "cea411e3c67e77b48170699fe259e1c6", "MD5 48 bytes" },
        { &g_abRandom72KB[0],        49, "8453cb1977439f97279e39cbd8408ced", "MD5 49 bytes" },
        { &g_abRandom72KB[0],        50, "e6288244d7ae6f30dafa113146044f1b", "MD5 50 bytes" },
        { &g_abRandom72KB[0],        51, "f0c19a27d2474b5a2076ad0013ce966d", "MD5 51 bytes" },
        { &g_abRandom72KB[0],        52, "9265e6aea5bb3e16b4771dc0e15e9b23", "MD5 52 bytes" },
        { &g_abRandom72KB[0],        53, "3efcf10c3fd84a1999a11a8f2474fddd", "MD5 53 bytes" },
        { &g_abRandom72KB[0],        54, "cd6004b92196226fc754794bb185c09e", "MD5 54 bytes" },
        { &g_abRandom72KB[0],        55, "3ad247a37a75767374e3808930ff1240", "MD5 55 bytes" },
        { &g_abRandom72KB[0],        56, "11e8564e3db197beeaa8b9665b2ee2aa", "MD5 56 bytes" },
        { &g_abRandom72KB[0],        57, "790119e207ba5bf9b95768bd4acec278", "MD5 57 bytes" },
        { &g_abRandom72KB[0],        58, "2ff7a27055eb975c8d36f7105b905422", "MD5 58 bytes" },
        { &g_abRandom72KB[0],        59, "d1d5be70e576e9db7145b68bfaf4b8f7", "MD5 59 bytes" },
        { &g_abRandom72KB[0],        60, "d4383ffab62bda08cf2222186954abc8", "MD5 60 bytes" },
        { &g_abRandom72KB[0],        61, "69454edb58ddb72d7a715125ac5eefec", "MD5 61 bytes" },
        { &g_abRandom72KB[0],        62, "c2ea1754ec455e15e83c79630e726295", "MD5 62 bytes" },
        { &g_abRandom72KB[0],        63, "5c05c6ca2dc4ddbd52e447c2683aed47", "MD5 63 bytes" },
        { &g_abRandom72KB[0],        64, "a9f2d51e24b01bef5c96d4ab09b00f7b", "MD5 64 bytes" },
        { &g_abRandom72KB[0],        65, "f9ac6f5e2f43481d5966d7b9f946df6e", "MD5 65 bytes" },
        { &g_abRandom72KB[0],        66, "004d20523ee8c581da762700d4856e95", "MD5 66 bytes" },
        { &g_abRandom72KB[0],        67, "54180f89561ec585155c83cdb332eda7", "MD5 67 bytes" },
        { &g_abRandom72KB[0],        68, "e6e884c81250372d2f6d1e297a58fd3d", "MD5 68 bytes" },
        { &g_abRandom72KB[0],        69, "1ece0e157d215cec78ac5e7fc6096899", "MD5 69 bytes" },
        { &g_abRandom72KB[0],        70, "998fc35762eb99548110129d81873b4b", "MD5 70 bytes" },
        { &g_abRandom72KB[0],        71, "54e6dd0d33bc39c7bae536d20a070074", "MD5 71 bytes" },
        { &g_abRandom72KB[0],        72, "0eef8b9cf8f1e008cec190ab021e42d2", "MD5 72 bytes" },
        { &g_abRandom72KB[0],        73, "5519c0cdf891efe0bd9ead66cd20d9b4", "MD5 73 bytes" },
        { &g_abRandom72KB[0],        74, "14deef0ce4d7e2105875532f21da2212", "MD5 74 bytes" },
        { &g_abRandom72KB[0],        75, "938fdf08e55106e588fec9659ecc3f4b", "MD5 75 bytes" },
        { &g_abRandom72KB[0],        76, "7f4f6a2114cd8552aa948b20d7cfd725", "MD5 76 bytes" },
        { &g_abRandom72KB[0],        77, "5c34a0fe473b856d9665d789f7107146", "MD5 77 bytes" },
        { &g_abRandom72KB[0],        78, "c0fe660ac18254a65efdc8f71da77635", "MD5 78 bytes" },
        { &g_abRandom72KB[0],        79, "2c2670f20850aaa3f5e0d8a8fc07ae6e", "MD5 79 bytes" },
        { &g_abRandom72KB[0],        80, "8e74b2fb1edfd2135fd7240c62906fce", "MD5 80 bytes" },
        { &g_abRandom72KB[0],        81, "724214ccd4c5f1608cc577d80f1c1d63", "MD5 81 bytes" },
        { &g_abRandom72KB[0],        82, "2215de010fdcc3fe82a4bda76fc4c00c", "MD5 82 bytes" },
        { &g_abRandom72KB[0],        83, "f022e4fd762db1e713deff528eb8ab15", "MD5 83 bytes" },
        { &g_abRandom72KB[0],        84, "262c82ee993d73543fb86f60d43849bc", "MD5 84 bytes" },
        { &g_abRandom72KB[0],        85, "dac379497414e4135ea8e42ccbe39a11", "MD5 85 bytes" },
        { &g_abRandom72KB[0],        86, "c01cb2483bbdf778b536b07d7b12d31b", "MD5 86 bytes" },
        { &g_abRandom72KB[0],        87, "8f4753f5e64fa725f1a2a9a638e97686", "MD5 87 bytes" },
        { &g_abRandom72KB[0],        88, "6f53a21288b8a107a237df50b99fbc63", "MD5 88 bytes" },
        { &g_abRandom72KB[0],        89, "1df7274ecf95aecf4cef76070b4bc703", "MD5 89 bytes" },
        { &g_abRandom72KB[0],        90, "ebde27beebf8649892818f2c1b94dbac", "MD5 90 bytes" },
        { &g_abRandom72KB[0],        91, "c259c1aa0277ef8f3fda149d657f2958", "MD5 91 bytes" },
        { &g_abRandom72KB[0],        92, "47654004a1761a853d3a052cf3207e04", "MD5 92 bytes" },
        { &g_abRandom72KB[0],        93, "4e3011d42a53c359dfb7ed0cdd9fca3c", "MD5 93 bytes" },
        { &g_abRandom72KB[0],        94, "eab81b49b0efad606e623cad773f9bad", "MD5 94 bytes" },
        { &g_abRandom72KB[0],        95, "77a15147669b80b13cebf7f944865f7a", "MD5 95 bytes" },
        { &g_abRandom72KB[0],        96, "6975f970c6c7f8d11a52b8465df6f752", "MD5 96 bytes" },
        { &g_abRandom72KB[0],        97, "a5628bb324d1bd34bc41f81501c73c6d", "MD5 97 bytes" },
        { &g_abRandom72KB[0],        98, "c1b67f871130569c8dcd56b13997011b", "MD5 98 bytes" },
        { &g_abRandom72KB[0],        99, "4225c0cda6c16259a74c0733f3fa3025", "MD5 99 bytes" },
        { &g_abRandom72KB[0],       100, "39d1a9671f5eee3fd63f571e5700fb18", "MD5 100 bytes" },
        { &g_abRandom72KB[0],       101, "743bed2b5485505ebcd9c2dcaf61afd3", "MD5 101 bytes" },
        { &g_abRandom72KB[0],       102, "7a0d60739b87793168113a695257de4b", "MD5 102 bytes" },
        { &g_abRandom72KB[0],       103, "d483ae1c829dc3244ede1f46488c0f0c", "MD5 103 bytes" },
        { &g_abRandom72KB[0],       104, "1502b082b28c1b60decad1c3ec8d637b", "MD5 104 bytes" },
        { &g_abRandom72KB[0],       105, "df0d769bad97093d00560e4221023dbf", "MD5 105 bytes" },
        { &g_abRandom72KB[0],       106, "bd3641699ffe5adb1c8a3c8917abb1ff", "MD5 106 bytes" },
        { &g_abRandom72KB[0],       107, "b5e585c6da2c40a7e5aab059e7bd15ee", "MD5 107 bytes" },
        { &g_abRandom72KB[0],       108, "e3f8ec4a683a3512f73639cd6ef69638", "MD5 108 bytes" },
        { &g_abRandom72KB[0],       109, "a4a4b065603644c6d50dc0a4426badf6", "MD5 109 bytes" },
        { &g_abRandom72KB[0],       110, "7b22226bdb2504211fa8b99d2860b2c0", "MD5 110 bytes" },
        { &g_abRandom72KB[0],       111, "ab89296851f6ffc435431b82a3247b15", "MD5 111 bytes" },
        { &g_abRandom72KB[0],       112, "20249b22ba14b007555e54c9366ddabd", "MD5 112 bytes" },
        { &g_abRandom72KB[0],       113, "6bf5d9a3a30b5eb7af2a4092eda69ecd", "MD5 113 bytes" },
        { &g_abRandom72KB[0],       114, "f7d97a06da8494176e0fe01934e31e1f", "MD5 114 bytes" },
        { &g_abRandom72KB[0],       115, "2616a77b387bb99afdf8bb1f54f7a000", "MD5 115 bytes" },
        { &g_abRandom72KB[0],       116, "bf81d63f172e2ce9b958f0da5c92c344", "MD5 116 bytes" },
        { &g_abRandom72KB[0],       117, "8633ee1f631841cb1887a487f22a9f9f", "MD5 117 bytes" },
        { &g_abRandom72KB[0],       118, "0f3b0311c0cf9d9eba3240404cae137d", "MD5 118 bytes" },
        { &g_abRandom72KB[0],       119, "f79c00a97df8d45a211e6d01409c119b", "MD5 119 bytes" },
        { &g_abRandom72KB[0],       120, "4d88da4ff44c801f692b46869f2fbb98", "MD5 120 bytes" },
        { &g_abRandom72KB[0],       121, "15ca57bd78833831f54dfdcfbd5e4d29", "MD5 121 bytes" },
        { &g_abRandom72KB[0],       122, "0877ba42c2b57ab3d0041ddbc4bd930b", "MD5 122 bytes" },
        { &g_abRandom72KB[0],       123, "66086909e3740dd20004b0968e3316b5", "MD5 123 bytes" },
        { &g_abRandom72KB[0],       124, "bafe834041396ce465f0995677ea6fba", "MD5 124 bytes" },
        { &g_abRandom72KB[0],       125, "634eff105e618d75ab738512aff11048", "MD5 125 bytes" },
        { &g_abRandom72KB[0],       126, "850257fc17096cce1e2d6de664e7ec04", "MD5 126 bytes" },
        { &g_abRandom72KB[0],       127, "f12f54a3ecee91de840103aaa0d3726a", "MD5 127 bytes" },
        { &g_abRandom72KB[0],       128, "1a266528b8119279e9639418a2d85e77", "MD5 128 bytes" },
        { &g_abRandom72KB[0],       129, "e4791e35863addd6fa74ff1662d6a908", "MD5 129 bytes" },
        { &g_abRandom72KB[0],      1024, "310e55220bd80529e76a544209f8e532", "MD5 1024 bytes" },
        { &g_abRandom72KB[0],     73001, "f3d05b52be86f1db66a9ebf5ababaaa8", "MD5 73001 bytes" },
        { &g_abRandom72KB[0],     73728, "aef57c3b2ec6e560b51b8094fe34def7", "MD5 73728 bytes" },
        { &g_abRandom72KB[0x20c9], 9991, "6461339c6615d23c704298a313e07cf5", "MD5 8393 bytes @9991" },
    };
    testGeneric("1.2.840.113549.2.5", s_abTests, RT_ELEMENTS(s_abTests), "MD5", RTDIGESTTYPE_MD5);
}


/**
 * Tests SHA-1
 */
static void testSha1(void)
{
    RTTestISub("SHA-1");

    /*
     * Some quick direct API tests.
     */
    uint8_t     abHash[RTSHA1_HASH_SIZE];
    char        szDigest[RTSHA1_DIGEST_LEN + 1];
    const char *pszString;

    pszString = "";
    RTSha1(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha1ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "da39a3ee5e6b4b0d3255bfef95601890afd80709");

    pszString = "The quick brown fox jumps over the lazy dog";
    RTSha1(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha1ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");

    pszString = "a";
    RTSha1(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha1ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "86f7e437faa5a7fce15d1ddcb9eaeaea377667b8");

    pszString = "abc";
    RTSha1(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha1ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "a9993e364706816aba3e25717850c26c9cd0d89d");


    /*
     * Generic API tests.
     */
    static TESTRTDIGEST const s_abTests[] =
    {
        { &g_abRandom72KB[0],         0, "da39a3ee5e6b4b0d3255bfef95601890afd80709", "SHA-1 0 bytes" },
        { &g_abRandom72KB[0],         1, "768aab37c292010133979e821ad5ac081ade388a", "SHA-1 1 bytes" },
        { &g_abRandom72KB[0],         2, "e1d72a3d75ec967fe2826670ed35ce5242204ad2", "SHA-1 2 bytes" },
        { &g_abRandom72KB[0],         3, "d76ef947caa78f5e786d4ea417edec1d8f5a399b", "SHA-1 3 bytes" },
        { &g_abRandom72KB[0],         4, "392932bb1bd20acce81b1c9df604cb149ca3089e", "SHA-1 4 bytes" },
        { &g_abRandom72KB[0],         5, "f13362588bcfe78afd6f2b4f264bfa10f9e18bcf", "SHA-1 5 bytes" },
        { &g_abRandom72KB[0],         6, "9f582f1afe75e3769d6afc95f266b114c1f42349", "SHA-1 6 bytes" },
        { &g_abRandom72KB[0],         7, "e9326429c1709dd2278220e72eab0de2eebcfa67", "SHA-1 7 bytes" },
        { &g_abRandom72KB[0],         8, "7882a61d6bd15b1377810d6a2083c20306b56b2d", "SHA-1 8 bytes" },
        { &g_abRandom72KB[0],         9, "71df6819243e6ff29be17628fd9e051d732bf26e", "SHA-1 9 bytes" },
        { &g_abRandom72KB[0],        10, "76f9029c260c99700fe14341d39067bcc746b766", "SHA-1 10 bytes" },
        { &g_abRandom72KB[0],        11, "b11df4de9916a760282f046f86d7e65ebc276daa", "SHA-1 11 bytes" },
        { &g_abRandom72KB[0],        12, "67af3a95329eb9d23b4db48e372befe02f93860d", "SHA-1 12 bytes" },
        { &g_abRandom72KB[0],        13, "add021eaaa1c315110708e307ff6cf838fac4764", "SHA-1 13 bytes" },
        { &g_abRandom72KB[0],        14, "b66297ac3211b967b27010316a4feeb87af377b1", "SHA-1 14 bytes" },
        { &g_abRandom72KB[0],        15, "8350ccd151d4fbc8fcaa76486854c82ce28bf1c2", "SHA-1 15 bytes" },
        { &g_abRandom72KB[0],        16, "e005dbbc5f955cf88e06ad92eb2fecb8d7abc829", "SHA-1 16 bytes" },
        { &g_abRandom72KB[0],        17, "d9b9101f8026e9c9e164370e11b6b14877642339", "SHA-1 17 bytes" },
        { &g_abRandom72KB[0],        18, "f999dfca047ee12be46b4141d9387d8694a8c03f", "SHA-1 18 bytes" },
        { &g_abRandom72KB[0],        19, "366324b23a2ef746fe5bedce961482c0e8816fd1", "SHA-1 19 bytes" },
        { &g_abRandom72KB[0],        20, "6f987e9f030a9b25790ec79b0c9c0a5d8085a565", "SHA-1 20 bytes" },
        { &g_abRandom72KB[0],        21, "799a3aa036d56809e459f757a4086a9b83549fd8", "SHA-1 21 bytes" },
        { &g_abRandom72KB[0],        22, "1fd879155acdd70d26b64533283b033db95cb264", "SHA-1 22 bytes" },
        { &g_abRandom72KB[0],        23, "888db854b475c3923f73090ad855d68c8708ecf4", "SHA-1 23 bytes" },
        { &g_abRandom72KB[0],        24, "c81e41a34ec7ae112a59bdcc1e31c9dd9f75cafc", "SHA-1 24 bytes" },
        { &g_abRandom72KB[0],        25, "a19edeb182e13e11c7371b6795f9f973544e25e2", "SHA-1 25 bytes" },
        { &g_abRandom72KB[0],        26, "7a98f96aa921d6932dd966d4f41f7afafef50992", "SHA-1 26 bytes" },
        { &g_abRandom72KB[0],        27, "847272961c5a5a1b10d2fefab9d95475bb246e6b", "SHA-1 27 bytes" },
        { &g_abRandom72KB[0],        28, "eb2b1dbc0ea2a23e308e1bf9d6e78167f62b9a5c", "SHA-1 28 bytes" },
        { &g_abRandom72KB[0],        29, "5050998e561c0887e3365d8c87f63ce00cc4084e", "SHA-1 29 bytes" },
        { &g_abRandom72KB[0],        30, "42278d4e8a70aeaa64f9194f1c90d827bba85a45", "SHA-1 30 bytes" },
        { &g_abRandom72KB[0],        31, "f87ad36842c6e36fbbe7b3edd6514d4670367b79", "SHA-1 31 bytes" },
        { &g_abRandom72KB[0],        32, "9faa3230afd8279a4b006d37b9f243b2f6430616", "SHA-1 32 bytes" },
        { &g_abRandom72KB[0],        33, "5ca875e82e84cfa9aee1adc5a54fdb8b276f9c9a", "SHA-1 33 bytes" },
        { &g_abRandom72KB[0],        34, "833d2bb01ec548b204750e0a3b9226059377aa66", "SHA-1 34 bytes" },
        { &g_abRandom72KB[0],        35, "aeb0843fc2978addc14cc42a184887a57678722b", "SHA-1 35 bytes" },
        { &g_abRandom72KB[0],        36, "77d4e5da16564461a0a44ce0ac4d0e885c46c127", "SHA-1 36 bytes" },
        { &g_abRandom72KB[0],        37, "dfe064bcfed5f8fa2fe8efca6a75a9ca51d2ef43", "SHA-1 37 bytes" },
        { &g_abRandom72KB[0],        38, "a89cf8c2af3a1ede699eb7421d0038030035b081", "SHA-1 38 bytes" },
        { &g_abRandom72KB[0],        39, "5f4eb0893c2ea7445fea51508573f9f8eab98862", "SHA-1 39 bytes" },
        { &g_abRandom72KB[0],        40, "2cf921e5e564b947c6ae98e709b49e9e7c7f9c2f", "SHA-1 40 bytes" },
        { &g_abRandom72KB[0],        41, "7788ea70ee17e475b360e4e59f3f7792137e836d", "SHA-1 41 bytes" },
        { &g_abRandom72KB[0],        42, "6506ab486671f2a2b7e8a19626da6ae531299f39", "SHA-1 42 bytes" },
        { &g_abRandom72KB[0],        43, "950096584b7a7aabd610ef466be73e544d9d0809", "SHA-1 43 bytes" },
        { &g_abRandom72KB[0],        44, "ff99e44ed9e8f30f81a9b50f61c376681ee16a91", "SHA-1 44 bytes" },
        { &g_abRandom72KB[0],        45, "f043b267447eaf18c89528e8f52fa0c8f33c0022", "SHA-1 45 bytes" },
        { &g_abRandom72KB[0],        46, "a9527c0a351fe0b4606eba0a8900b78c0f311f1a", "SHA-1 46 bytes" },
        { &g_abRandom72KB[0],        47, "579f1fea36188490d80bfe4f04bd5a0e625006d2", "SHA-1 47 bytes" },
        { &g_abRandom72KB[0],        48, "c43765e7b3565e7a78ccf96011a4afad18f7f781", "SHA-1 48 bytes" },
        { &g_abRandom72KB[0],        49, "a3130b7c92c6441e8c319c6768e611d74e419ce7", "SHA-1 49 bytes" },
        { &g_abRandom72KB[0],        50, "ef8fedfa73384d6c68c7e956ddc1c45239d40be0", "SHA-1 50 bytes" },
        { &g_abRandom72KB[0],        51, "a4dea447ae78523fec06a7e65d798f1a61c3b96b", "SHA-1 51 bytes" },
        { &g_abRandom72KB[0],        52, "86b4aa02ab3cc63badf544131087689ecaf25f24", "SHA-1 52 bytes" },
        { &g_abRandom72KB[0],        53, "a0faf581ceaa922d0530f90d76fbd93e93810d58", "SHA-1 53 bytes" },
        { &g_abRandom72KB[0],        54, "80ab7368ad003e5083d9804579a565de6a278c40", "SHA-1 54 bytes" },
        { &g_abRandom72KB[0],        55, "8b1bec05d4a56b1beb500f2d639b3f7887d68e81", "SHA-1 55 bytes" },
        { &g_abRandom72KB[0],        56, "66ba1322833fd92af27ae37e4382e95b525969b4", "SHA-1 56 bytes" },
        { &g_abRandom72KB[0],        57, "ecab320750baead21b14d31450512ec0584ccd8d", "SHA-1 57 bytes" },
        { &g_abRandom72KB[0],        58, "a4f13253e34d4dc4f65c4b4b3da694fdf8af3bf9", "SHA-1 58 bytes" },
        { &g_abRandom72KB[0],        59, "73f6b88c28af42df6a0ab207e33176078082f67d", "SHA-1 59 bytes" },
        { &g_abRandom72KB[0],        60, "d19f291905289ded6ed13b3dc8c47f94b999c100", "SHA-1 60 bytes" },
        { &g_abRandom72KB[0],        61, "bd0bef35c45892c13c27825de9920a8b9fc91d73", "SHA-1 61 bytes" },
        { &g_abRandom72KB[0],        62, "f62f8426087fea0bf3733f2b2f6d37fd155d01f6", "SHA-1 62 bytes" },
        { &g_abRandom72KB[0],        63, "22891a14ef0e27fabef93f768fd645e042254011", "SHA-1 63 bytes" },
        { &g_abRandom72KB[0],        64, "364a08a4a85f6fe61272125ae2e549a1c4cc3fe4", "SHA-1 64 bytes" },
        { &g_abRandom72KB[0],        65, "8bf19ebbe263845cd3c35b853f1892d959dc8bd4", "SHA-1 65 bytes" },
        { &g_abRandom72KB[0],        66, "ed3a0f2dc02fcc12e134bfd2f4e23a6294122e57", "SHA-1 66 bytes" },
        { &g_abRandom72KB[0],        67, "596308eb7eabf39431ec953d2c6bacb9bb7c6c70", "SHA-1 67 bytes" },
        { &g_abRandom72KB[0],        68, "d1d736e7d1a7ffe5613f6337248d67d7ebf8be01", "SHA-1 68 bytes" },
        { &g_abRandom72KB[0],        69, "6ebad117db6e9cdfedae52f44df3ba51eb95efa2", "SHA-1 69 bytes" },
        { &g_abRandom72KB[0],        70, "f5a05b4b156c07c23e8779f776a7f894922c148f", "SHA-1 70 bytes" },
        { &g_abRandom72KB[0],        71, "01aef4f832ad68e6708f31a97bc363d9da20b6ba", "SHA-1 71 bytes" },
        { &g_abRandom72KB[0],        72, "1bbf9dc55aa008a5e0c7fc4d34cb9e283f785b4e", "SHA-1 72 bytes" },
        { &g_abRandom72KB[0],        73, "1504867acdbe4b1db5015d272182d17a4620a8b9", "SHA-1 73 bytes" },
        { &g_abRandom72KB[0],        74, "7c96021e92a9bf98e5fb34ac17a3db487f1c7ac7", "SHA-1 74 bytes" },
        { &g_abRandom72KB[0],        75, "d53acd002a6fc89120bc1ae7698aa0996cfacf00", "SHA-1 75 bytes" },
        { &g_abRandom72KB[0],        76, "f8036f1ffdcaaadc1beb1542264f43060bf7c0c3", "SHA-1 76 bytes" },
        { &g_abRandom72KB[0],        77, "6bec2adc19d2229054499d035f9904b6a69ea89a", "SHA-1 77 bytes" },
        { &g_abRandom72KB[0],        78, "ad09365576f77c608afe0242caaae3604ce0fd17", "SHA-1 78 bytes" },
        { &g_abRandom72KB[0],        79, "8ef16b886a12c3ceadf67d6bf8ee016c72b4c02a", "SHA-1 79 bytes" },
        { &g_abRandom72KB[0],        80, "bd60ca67ea01e456de55bfbf0cc1095eadeda98a", "SHA-1 80 bytes" },
        { &g_abRandom72KB[0],        81, "6cb9bb8e6447fff7a04e46de9e8410bffe7185f9", "SHA-1 81 bytes" },
        { &g_abRandom72KB[0],        82, "2365ced0aa582dfdfe0b7ba0607e6953d64e1029", "SHA-1 82 bytes" },
        { &g_abRandom72KB[0],        83, "4597ee3f912cce76b81d05eb0d4941af76688995", "SHA-1 83 bytes" },
        { &g_abRandom72KB[0],        84, "287a99bcb83b395e9e0b1f36d492417f4bd25d6f", "SHA-1 84 bytes" },
        { &g_abRandom72KB[0],        85, "689caf3dd39ec1cf99baba13c549d9a25797e3e1", "SHA-1 85 bytes" },
        { &g_abRandom72KB[0],        86, "1fc2a6da9ba7a623f5eac73b56134772de374d1c", "SHA-1 86 bytes" },
        { &g_abRandom72KB[0],        87, "a2c3cb4c5b1dc6dfdf553614bf846ed794874f16", "SHA-1 87 bytes" },
        { &g_abRandom72KB[0],        88, "3e9b451561c66d9eb9e9052c3027d4d91f495771", "SHA-1 88 bytes" },
        { &g_abRandom72KB[0],        89, "a47b63438fdaaab0bb4ea5e31b84aaae118bc9a9", "SHA-1 89 bytes" },
        { &g_abRandom72KB[0],        90, "5c1b3fbc228add14796ed049b00924ed7140340d", "SHA-1 90 bytes" },
        { &g_abRandom72KB[0],        91, "b873c08cb5329f82bd6adcd134f81e29597c4964", "SHA-1 91 bytes" },
        { &g_abRandom72KB[0],        92, "f77066f9a0908d50e6018300cb82f436df9c8045", "SHA-1 92 bytes" },
        { &g_abRandom72KB[0],        93, "b4c97048c643c9e5b6355683d36a1faee6d023ac", "SHA-1 93 bytes" },
        { &g_abRandom72KB[0],        94, "d6b48d223bd97d3b53c0868c0528f18a3d5ebc88", "SHA-1 94 bytes" },
        { &g_abRandom72KB[0],        95, "9562095eccd53bf5a968bc3eda65f3e327326b8e", "SHA-1 95 bytes" },
        { &g_abRandom72KB[0],        96, "b394b8e175238834daf9c4d3b5fc8dbcfd982ae9", "SHA-1 96 bytes" },
        { &g_abRandom72KB[0],        97, "c0127c82cb667da52892e462b5e9cdafb4f0deaa", "SHA-1 97 bytes" },
        { &g_abRandom72KB[0],        98, "f51474c9995eb341748dea0f07f60cac46d5fa87", "SHA-1 98 bytes" },
        { &g_abRandom72KB[0],        99, "6ae3406d41332cfe86c40d275b5e1e394893361b", "SHA-1 99 bytes" },
        { &g_abRandom72KB[0],       100, "0d5823c081f69ad4e7fbd7ee0ed12092f6e2ed75", "SHA-1 100 bytes" },
        { &g_abRandom72KB[0],       101, "fc8527f9c789abf67e2bcc78e2048f4eda8f7d7d", "SHA-1 101 bytes" },
        { &g_abRandom72KB[0],       102, "a322eb5d5e65833310953a3a7bb1081e05b69318", "SHA-1 102 bytes" },
        { &g_abRandom72KB[0],       103, "5a4fac64b273263043c771a5f9bae1fb243cd7d6", "SHA-1 103 bytes" },
        { &g_abRandom72KB[0],       104, "bad4d3f3091817273dbac07b8712eec27b16cb6b", "SHA-1 104 bytes" },
        { &g_abRandom72KB[0],       105, "ca0721a40610ea6290b97b541806b195e659af19", "SHA-1 105 bytes" },
        { &g_abRandom72KB[0],       106, "cf3fb01cc0d95898e3b698ebdbd3c1e2624eacd2", "SHA-1 106 bytes" },
        { &g_abRandom72KB[0],       107, "e3509ada3e264733cd36e2dab301650797fa8351", "SHA-1 107 bytes" },
        { &g_abRandom72KB[0],       108, "045ca9690e7ac3336fd8907c28650c3ad489cbfe", "SHA-1 108 bytes" },
        { &g_abRandom72KB[0],       109, "44a6c85d7d9053679fde01139ee0cf6176754227", "SHA-1 109 bytes" },
        { &g_abRandom72KB[0],       110, "2ea6df640202b8799a885887a7c62a05247e60da", "SHA-1 110 bytes" },
        { &g_abRandom72KB[0],       111, "38b0a91b3b2949cf752d3273157b9fa911972ad3", "SHA-1 111 bytes" },
        { &g_abRandom72KB[0],       112, "962b950eab71062e7bc619e33cee36ceded923ee", "SHA-1 112 bytes" },
        { &g_abRandom72KB[0],       113, "ccdecb735b377f1023ac2ad1e5ef0cb264bccf63", "SHA-1 113 bytes" },
        { &g_abRandom72KB[0],       114, "4491b4e057d62e9c875b9153e9d76860657ab1a7", "SHA-1 114 bytes" },
        { &g_abRandom72KB[0],       115, "f2d6c27c001222592fe06ba3e8c408b7033d14e1", "SHA-1 115 bytes" },
        { &g_abRandom72KB[0],       116, "c9651c478f36ddacbca4cd680b37db6869473ed4", "SHA-1 116 bytes" },
        { &g_abRandom72KB[0],       117, "b69442fcaa3e3bd617d3d3a0d436eb170e580083", "SHA-1 117 bytes" },
        { &g_abRandom72KB[0],       118, "a48a7f361bf44d4e055a7e6aaa51f5b2f8de4419", "SHA-1 118 bytes" },
        { &g_abRandom72KB[0],       119, "257d756bebc29cd408394b23f1891739cfd00755", "SHA-1 119 bytes" },
        { &g_abRandom72KB[0],       120, "1ef9f2becc7217db45b97d7a0f47da9ed23f8957", "SHA-1 120 bytes" },
        { &g_abRandom72KB[0],       121, "06ac75dc3ffee76eeefa11bb10dae6f4fb94ece8", "SHA-1 121 bytes" },
        { &g_abRandom72KB[0],       122, "883c7780a8926ac22cb6dff4dbb8889c579ebb99", "SHA-1 122 bytes" },
        { &g_abRandom72KB[0],       123, "004e08413617c1bb262d95f66a5ed5fff82b718b", "SHA-1 123 bytes" },
        { &g_abRandom72KB[0],       124, "f68155afc2ee6292c881db5721f3e0f6e77a0bca", "SHA-1 124 bytes" },
        { &g_abRandom72KB[0],       125, "e9ec6e4b4b1123adceda9d0f7db99c10712af649", "SHA-1 125 bytes" },
        { &g_abRandom72KB[0],       126, "bcf570708b73c20d9805172f50935ab44d122e6b", "SHA-1 126 bytes" },
        { &g_abRandom72KB[0],       127, "4b5f949b1b8a0a2b3716aba8cad91f45ae0f9408", "SHA-1 127 bytes" },
        { &g_abRandom72KB[0],       128, "fd78703c7d7ce4a29bd79230898b7ac382f117cc", "SHA-1 128 bytes" },
        { &g_abRandom72KB[0],       129, "461d0063059b9d4b8255f621bd329dfecdfaedef", "SHA-1 129 bytes" },
        { &g_abRandom72KB[0],      1024, "2abd8ddb6c13b55596f31c74d96a04411ad39e8e", "SHA-1 1024 bytes" },
        { &g_abRandom72KB[0],     73001, "e8c68bf7f6bd3b5c2a482c8a2ca9849b1e5afff1", "SHA-1 73001 bytes" },
        { &g_abRandom72KB[0],     73728, "e89f36633ad2745ab2aac50ea7b2fe23e49ba69f", "SHA-1 73728 bytes" },
        { &g_abRandom72KB[0x20c9],  9991, "62001184bacacce3774566d916055d425a85eba5", "SHA-1 8393 bytes @9991" },
    };
    testGeneric("1.3.14.3.2.26", s_abTests, RT_ELEMENTS(s_abTests), "SHA-1", RTDIGESTTYPE_SHA1);
}


/**
 * Tests SHA-256
 */
static void testSha256(void)
{
    RTTestISub("SHA-256");

    /*
     * Some quick direct API tests.
     */
    uint8_t     abHash[RTSHA256_HASH_SIZE];
    char        szDigest[RTSHA256_DIGEST_LEN + 1];
    const char *pszString;

    pszString = "";
    RTSha256(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha256ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    pszString = "The quick brown fox jumps over the lazy dog";
    RTSha256(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha256ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");

    /*
     * Generic API tests.
     */
    static TESTRTDIGEST const s_abTests[] =
    {
        { &g_abRandom72KB[0],         0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "SHA-256 0 bytes" },
        { &g_abRandom72KB[0],         1, "e6f207509afa3908da116ce61a7576954248d9fe64a3c652b493cca57ce36e2e", "SHA-256 1 bytes" },
        { &g_abRandom72KB[0],         2, "51f3435c5f25f051c7e9209728c37a062d7d93c1d7c4075a5e383f2600cd1422", "SHA-256 2 bytes" },
        { &g_abRandom72KB[0],         3, "37cd6662639ecea8c1c26ae10cf307f6884b5ab6ddba99885a9fd92349044de1", "SHA-256 3 bytes" },
        { &g_abRandom72KB[0],         4, "a31bb0a349218352bb721a901b7bf8b9290d1b81eec4ac7e6005a2cfd5b3baa9", "SHA-256 4 bytes" },
        { &g_abRandom72KB[0],         5, "00a42e462920c577ae608ddcaedf9478b53d25541b696e454dd8d7fb14f61c35", "SHA-256 5 bytes" },
        { &g_abRandom72KB[0],         6, "6c80d81f45f18cfaaf6c3de33dc505936f7faa41f714cdc7b0c8a3bdba15ade4", "SHA-256 6 bytes" },
        { &g_abRandom72KB[0],         7, "540ea7cc49b1aa11cfd4004b0faebc1b72cfa4aa0bcd940c4341bbd9ac4f6913", "SHA-256 7 bytes" },
        { &g_abRandom72KB[0],         8, "b5822d2199e3b3d7e6260d083dbf8bd79aa5ea7e27c3b879997c90098cbf1f06", "SHA-256 8 bytes" },
        { &g_abRandom72KB[0],         9, "6d983a270c13e529d6a7b1d86f702c588884e6e109cfc975dfebce3b65afa538", "SHA-256 9 bytes" },
        { &g_abRandom72KB[0],        10, "66da7649b7bd8f814d8af92347c8fca9102f27449dacd72b7dfe05bb1937ff84", "SHA-256 10 bytes" },
        { &g_abRandom72KB[0],        11, "1fcec1b28467ffe6e80422a5150bf885ab58452b335a525e363aa06d5ae70c20", "SHA-256 11 bytes" },
        { &g_abRandom72KB[0],        12, "22ae8615a76d6e274a819cf2ee4a8af8dc3b6b985b59136218013e309c94a5d8", "SHA-256 12 bytes" },
        { &g_abRandom72KB[0],        13, "e4b439cdd6ed214dbc53d4312e65dea93129e4093e9dedea6c19f5d8961ee86c", "SHA-256 13 bytes" },
        { &g_abRandom72KB[0],        14, "8eee4ceba622ee53987910b3f8491e38094a569b9e9caafd857fd71626cc1066", "SHA-256 14 bytes" },
        { &g_abRandom72KB[0],        15, "c819dadb248aca10440334ba0d271705f83640c0a8a718679e6faaa48debdbd2", "SHA-256 15 bytes" },
        { &g_abRandom72KB[0],        16, "4019ab801bf80475e963c9d1458785cd4a24c9b00adda273fa1aa4f0c345ee8b", "SHA-256 16 bytes" },
        { &g_abRandom72KB[0],        17, "9fc7b643bfa14072f47ad05f96e6760dd78c15baee67c91c759cd49031d33138", "SHA-256 17 bytes" },
        { &g_abRandom72KB[0],        18, "9985cf02258c07b7134061bfde8434599e9e5e4d8342d8df659637236394d093", "SHA-256 18 bytes" },
        { &g_abRandom72KB[0],        19, "e345757094c316b6b5f0959da146586687fb282c1e415645c15f898932f47d9c", "SHA-256 19 bytes" },
        { &g_abRandom72KB[0],        20, "ae0d944c727221bc51da45956d40f3586ce69d76c8853c3a1ddf97cf64b562b4", "SHA-256 20 bytes" },
        { &g_abRandom72KB[0],        21, "95cc050b256d723d38357896c12fe6008e34f9dde4be67dbd057e34007956d33", "SHA-256 21 bytes" },
        { &g_abRandom72KB[0],        22, "f1aecdc3bca51bb2a9c4904d0c71d7acb61ddae166cfa748df1fc89b151844d8", "SHA-256 22 bytes" },
        { &g_abRandom72KB[0],        23, "269189cca3f6bb406782e7ed40beab1934b01af599daeb9bfdd68b102b11224d", "SHA-256 23 bytes" },
        { &g_abRandom72KB[0],        24, "70600ca44cdd0b8c81e20127768e8a42d4546c408681d7681854fa06a2953319", "SHA-256 24 bytes" },
        { &g_abRandom72KB[0],        25, "dc26c41fcbe49afbe6b05fb581484caad8782a4350959277cd4070fb46c798ac", "SHA-256 25 bytes" },
        { &g_abRandom72KB[0],        26, "ed78ad5525b97311df46d02db582bc951cc2671250912f045f5d29be87063552", "SHA-256 26 bytes" },
        { &g_abRandom72KB[0],        27, "6cc53426c84a5c449367fb9b1a9bb50411a1c4de858da1153fa1ccb7cf6646e3", "SHA-256 27 bytes" },
        { &g_abRandom72KB[0],        28, "ecb2ed45ceb2e0ce2415eb8dccfadd7353410cdffd3641541eb84f8f225ef748", "SHA-256 28 bytes" },
        { &g_abRandom72KB[0],        29, "335eacfe3cf7d86c4b4a83e0a7810371cc1f4773240790b56edc5e804fa5c3f8", "SHA-256 29 bytes" },
        { &g_abRandom72KB[0],        30, "070f657fa091df733f56565338bd9e2bce13246426ff2daba1c9c11c32f502f3", "SHA-256 30 bytes" },
        { &g_abRandom72KB[0],        31, "dfb7b82fd9524de158614fb1c0f232df6903a4247313d7e52891ea1a274bcad8", "SHA-256 31 bytes" },
        { &g_abRandom72KB[0],        32, "c21553469f65bc2bb449fa366467fd0ae16e5fc82c87bbba3ee59e9ccacf881d", "SHA-256 32 bytes" },
        { &g_abRandom72KB[0],        33, "a3532bf13ab6c55b001b32a3dd71204fcc5543c1e3c6afa4ec9d68b6d67557ae", "SHA-256 33 bytes" },
        { &g_abRandom72KB[0],        34, "7ea71b61c7419219d4fdafbff4dcc181b2db6a9cfa0bfd8389e1c679031f2458", "SHA-256 34 bytes" },
        { &g_abRandom72KB[0],        35, "07cbd9a6996f1f98d2734d818c2b833707fc11ef157517efd30dcffd33409b65", "SHA-256 35 bytes" },
        { &g_abRandom72KB[0],        36, "9347e18a41e55e4b199274583ad8ac8a6f72712b12ff08454dc9a94ed28f2405", "SHA-256 36 bytes" },
        { &g_abRandom72KB[0],        37, "b4d8a36d40ed9def9cc08fc64e0bb5792d4b2f7e6bdbb3003bae503f2ef2afd4", "SHA-256 37 bytes" },
        { &g_abRandom72KB[0],        38, "829c2b0c1785ebff89f5caf2f7f5b7529e1ccbbefb171e23b054058327c2c478", "SHA-256 38 bytes" },
        { &g_abRandom72KB[0],        39, "11d3c0ec52501e78f668a241957939c113c08b0a83420924397f97869b3d018a", "SHA-256 39 bytes" },
        { &g_abRandom72KB[0],        40, "476271f9371bf76d4aa627bafb5924959c033b0298e0b9ea4b5eb3012e095b4e", "SHA-256 40 bytes" },
        { &g_abRandom72KB[0],        41, "b818241e2f5c0b415ed49095abbfe465653946ddf67b78d1b0ebc9c2fa70371f", "SHA-256 41 bytes" },
        { &g_abRandom72KB[0],        42, "a02f6bc7f79a7b96dd16fa4f7ecbc0dfcc9719c5d41c51c4df9504c81b10cd56", "SHA-256 42 bytes" },
        { &g_abRandom72KB[0],        43, "f6f32fb00cdec042f70d38f20f92b73a6534d84b1af1fb4386a22cb1419f2f33", "SHA-256 43 bytes" },
        { &g_abRandom72KB[0],        44, "946d66e920b1f034186876cba8509b8e92dd6ddb41f29a48c9a9fb9a40ed27e6", "SHA-256 44 bytes" },
        { &g_abRandom72KB[0],        45, "b9e5f490e4505ce834759d0239e6b91499eafaedfe2e20f53b649ed719226f09", "SHA-256 45 bytes" },
        { &g_abRandom72KB[0],        46, "3dee1256de4dabbd8ae8eb463f294aaa187c6eb2a8b158e89bd01d24cc0e5ea6", "SHA-256 46 bytes" },
        { &g_abRandom72KB[0],        47, "f32ad8377e24bca4b664069e23d7e306d0ed0bec04b86834d245bea3b25e03b9", "SHA-256 47 bytes" },
        { &g_abRandom72KB[0],        48, "bd539ac8985f6f251d4ed3486dd7d45a3a316eecb872815873cf75858bbe90fc", "SHA-256 48 bytes" },
        { &g_abRandom72KB[0],        49, "685e1aaa3ca611b74c4bcfe62d6597f54c8f16236e1d990f21c61b5952a205f0", "SHA-256 49 bytes" },
        { &g_abRandom72KB[0],        50, "32d6fc76312316bf96cb57bb5f0f9c6a799ffcdc29571de437d5b2dd15ec788a", "SHA-256 50 bytes" },
        { &g_abRandom72KB[0],        51, "921be210954a8de9563c2dd652be14e1c9abf659b8485c5c7ac70fd381291ac6", "SHA-256 51 bytes" },
        { &g_abRandom72KB[0],        52, "848d5c2eafb58011f5f513735405c43e55fc6d6c23d1792cd891a921f69a74e3", "SHA-256 52 bytes" },
        { &g_abRandom72KB[0],        53, "052edfc879cb6a63ce33ef2a28da5ef418dd7ad20209ccdeb8247ca7325bbb97", "SHA-256 53 bytes" },
        { &g_abRandom72KB[0],        54, "862af02ee839897dde32d564b18316889176eac0e62b7a982cd79d5d3f9800d4", "SHA-256 54 bytes" },
        { &g_abRandom72KB[0],        55, "82042a5fcaa02dd245583b4fa198ddad31052a687979f76f0085d14c8ed22221", "SHA-256 55 bytes" },
        { &g_abRandom72KB[0],        56, "49869627a2ee03d8374d6fe5557dabb5211d59cac1186fe5502064eefe52e3e5", "SHA-256 56 bytes" },
        { &g_abRandom72KB[0],        57, "12f63c4012024e962996eb341be18c41f098e9b69739fe5262cb124567cb26ac", "SHA-256 57 bytes" },
        { &g_abRandom72KB[0],        58, "c8830941fdb38ccd160036d18e4969154361e615de37d8ac9edcf9b601727acd", "SHA-256 58 bytes" },
        { &g_abRandom72KB[0],        59, "9b953c0e044a932bd90c256dfc1c6fe1e49aaf15d3f6fe07b1b524da29c96d3e", "SHA-256 59 bytes" },
        { &g_abRandom72KB[0],        60, "590f234c6c5ab3ea01981e01468be82c13da5b07370991d92b8ecfd0e3d36030", "SHA-256 60 bytes" },
        { &g_abRandom72KB[0],        61, "6490bdb3fc554899f53705a6729d67008b20b802359fcb944fed95fe7e05caf5", "SHA-256 61 bytes" },
        { &g_abRandom72KB[0],        62, "c85c5c3d945b2c728388cb9af0913e28f6c74d907a01df3756748c4ef82635ea", "SHA-256 62 bytes" },
        { &g_abRandom72KB[0],        63, "46dcc81342ef03e4a313827e0bcdc72f5b97145483fd9fc280f2a39b9f2e6a0f", "SHA-256 63 bytes" },
        { &g_abRandom72KB[0],        64, "89eda27523b81a333fccd4be824a84a60f602a4bfe774ae7aa63c98a9b12ebf6", "SHA-256 64 bytes" },
        { &g_abRandom72KB[0],        65, "10ce93270e7fca1e7bdc0b7475845eeb3adcf39c47867aa2b36b41456b7627b0", "SHA-256 65 bytes" },
        { &g_abRandom72KB[0],        66, "3c4a3f92e8954ad710296d49227d85092249376b7e80f6c14056e961002a1811", "SHA-256 66 bytes" },
        { &g_abRandom72KB[0],        67, "64979dd99b6da8172ae79474bad1ccc8e91adfe803a47b2bb497f466a78cf95d", "SHA-256 67 bytes" },
        { &g_abRandom72KB[0],        68, "479f6c701cabd84516f25a45a3759e17a3b6ee56a439e08e03a682316651645c", "SHA-256 68 bytes" },
        { &g_abRandom72KB[0],        69, "7b401aba8fbcff05cdeb0ad35e66ba5d608a39c4f6542d46df439b2225e39a1e", "SHA-256 69 bytes" },
        { &g_abRandom72KB[0],        70, "4b397707574f2196e8023e86d2c1d060cbb0ab3bd9ce78d6ae971452f6b2cd36", "SHA-256 70 bytes" },
        { &g_abRandom72KB[0],        71, "ca6ec101132f05647f4aad51983dfbafc7b9044aafab1fa8dcfb395b767c2595", "SHA-256 71 bytes" },
        { &g_abRandom72KB[0],        72, "78605447fcbe1adecf6807c4a81ab0a756b09c777d3156f9993ad7b22f788ed6", "SHA-256 72 bytes" },
        { &g_abRandom72KB[0],        73, "ee529f31a4e0b71bf4bd182a45f800a5abb0e42169e8d875d725712306ad0fba", "SHA-256 73 bytes" },
        { &g_abRandom72KB[0],        74, "582bb8ec1c431e2468065a7d2b2dab2ed10b2a23e650cf2c295eb0d90bc4c6d5", "SHA-256 74 bytes" },
        { &g_abRandom72KB[0],        75, "faa6b7ec0cd4e13f8b53f9116675f3d91c90244eb8c84dadc81883c9421421e0", "SHA-256 75 bytes" },
        { &g_abRandom72KB[0],        76, "69e989716af62d1a255e53260e8bff7d680d507fdc432955dea7e616dc3a222a", "SHA-256 76 bytes" },
        { &g_abRandom72KB[0],        77, "619a27ee4575109a9880b2a7ff8f52f0b66346fe7281805e227390d24dc7f3e4", "SHA-256 77 bytes" },
        { &g_abRandom72KB[0],        78, "79efe7395bd9fbeb8964558c0a88be8a7293f75bf4513e0efa4cda0efb1621b6", "SHA-256 78 bytes" },
        { &g_abRandom72KB[0],        79, "361a1c3874a0145324c3ce6330e3321eef84fd46d2127e68c1e2596872d74983", "SHA-256 79 bytes" },
        { &g_abRandom72KB[0],        80, "42b3ec061a230faec1af95f685408d61c8025a30d8a9b04f7c9b2626f94d85e3", "SHA-256 80 bytes" },
        { &g_abRandom72KB[0],        81, "97aa260a9979f20997731c258dee85bc0936812cacae8325030b9df4e5c4762f", "SHA-256 81 bytes" },
        { &g_abRandom72KB[0],        82, "022d15b91d9e3849ca9e284bef29d5a2567c0bdd5af6145945705102165c3107", "SHA-256 82 bytes" },
        { &g_abRandom72KB[0],        83, "e45a484833a59bd0834dc2da045eac9747c1441f4318b1d535eb6e4c0bd869a3", "SHA-256 83 bytes" },
        { &g_abRandom72KB[0],        84, "ba0782193d5792d36b4a9cc5b1a47de9b661a7a05cbbcc1abd9334ee3778f6bd", "SHA-256 84 bytes" },
        { &g_abRandom72KB[0],        85, "f528b11135dc44642573857dbffcb361cb3fdeaefef8bb36eff4bdee1670fe59", "SHA-256 85 bytes" },
        { &g_abRandom72KB[0],        86, "0ba567b67c054bd794462540ca2049a008857b112d2fbd591ba2c56415d40924", "SHA-256 86 bytes" },
        { &g_abRandom72KB[0],        87, "21b09abfd9c2b78d081cd5d07aae172df9aea3c52b572fa96dbe107d5db02817", "SHA-256 87 bytes" },
        { &g_abRandom72KB[0],        88, "6cec2966f175b9bc5a15037a84cb2368b69837168368f316def4c75378ab5294", "SHA-256 88 bytes" },
        { &g_abRandom72KB[0],        89, "2d9628847f4638972646eb3265f45ebd8d4db4586a39cbf62e772ad2e0868436", "SHA-256 89 bytes" },
        { &g_abRandom72KB[0],        90, "5652de4228d477a5425dfde8d9652655d8c761480a57976dfa78bd88e4b11ff0", "SHA-256 90 bytes" },
        { &g_abRandom72KB[0],        91, "7a50a27207be3066ad1b349cf9c82e50d8610d0d95ec53d0fa0b677e0ef198c4", "SHA-256 91 bytes" },
        { &g_abRandom72KB[0],        92, "edcd28c7c6777bec4f9ff863554098055abcbc4ee6f777f669acf4c17e9b939e", "SHA-256 92 bytes" },
        { &g_abRandom72KB[0],        93, "f8e353f5033856bf1b3e29b1cf95acc977473e6e84c6aa7f467ff3a214a311f8", "SHA-256 93 bytes" },
        { &g_abRandom72KB[0],        94, "ff964737aaf19c5968393aa37d5a133bd42d26d49a1d342bc625cc23fbfcd3df", "SHA-256 94 bytes" },
        { &g_abRandom72KB[0],        95, "7b975c510c8d7eba8ba3688cd96452d18b3544bb5aed540845b8ed320862e6cb", "SHA-256 95 bytes" },
        { &g_abRandom72KB[0],        96, "39af3d95f2784e671171b02217344d41a50ca063db118940d940b103aa8f88df", "SHA-256 96 bytes" },
        { &g_abRandom72KB[0],        97, "a7f84a55605007267ab6b22478d82fc045b9ccdeb7bc29e2368b6d36ba5302ee", "SHA-256 97 bytes" },
        { &g_abRandom72KB[0],        98, "393755a20e107455a7d961494a23433b3aed585b6173231922ba83cd7980baba", "SHA-256 98 bytes" },
        { &g_abRandom72KB[0],        99, "555400c5ea1b138cf58c0941a4edd7733698ef35d9b7b599da1d27a4a1da9b56", "SHA-256 99 bytes" },
        { &g_abRandom72KB[0],       100, "f27524b39dff76ca3870c765955a86272f8136801a367638ab788a3ba9f57c04", "SHA-256 100 bytes" },
        { &g_abRandom72KB[0],       101, "4857c87e9c6477e57475b8ceff80a02de75a9c6a2c21cfa2a3ac35ef199b132d", "SHA-256 101 bytes" },
        { &g_abRandom72KB[0],       102, "9c41626db68168c7c3a1065fc507932ea87f6fe7f490343d2ed532ae244c756a", "SHA-256 102 bytes" },
        { &g_abRandom72KB[0],       103, "13c331f42ad5bb2216ccbfe8e9254111e97291da1ba242a21d99f62547720ab7", "SHA-256 103 bytes" },
        { &g_abRandom72KB[0],       104, "259dd0b292cac0f4bb6a26e5e8dce7cfde908561edda176b3d826228c7ec4836", "SHA-256 104 bytes" },
        { &g_abRandom72KB[0],       105, "5f90dce6a68f0ccf50e0ffbfbc1e7831ebe619ab053d59625d75a5805d1cfc91", "SHA-256 105 bytes" },
        { &g_abRandom72KB[0],       106, "9eedf570854b0e9b47fb9bddccdd7c02529b0ce1394f83fa968f8bd10061dc82", "SHA-256 106 bytes" },
        { &g_abRandom72KB[0],       107, "4dce09d513d26be436d43ab935164156441bfbe2a9ce39341b3c902071f2fb75", "SHA-256 107 bytes" },
        { &g_abRandom72KB[0],       108, "f1ba290596fedeabca8079a8fd8eafa0599751d677034e4d8c4c86f3542e8828", "SHA-256 108 bytes" },
        { &g_abRandom72KB[0],       109, "e7dfc612ca26e6fb1dbc7b75dff65bff2c6b134e8283a24b9434ad5d469cea09", "SHA-256 109 bytes" },
        { &g_abRandom72KB[0],       110, "7d3f395a5da0dd86fb14e29074c7e4f05cb32ae28ddf1bd4327a535df9f809fb", "SHA-256 110 bytes" },
        { &g_abRandom72KB[0],       111, "7d74c406261ff04c3c32498b3534ef70c6199adba0d1d91989a54a9f606ebeb5", "SHA-256 111 bytes" },
        { &g_abRandom72KB[0],       112, "ccc4acce4d2e89c2fe5cc5cc0e1e1b380de17095dee93516ee87f1d1fc6f5e01", "SHA-256 112 bytes" },
        { &g_abRandom72KB[0],       113, "301bb78408884937aed4eb3ff069a79c2b714ee18519339ccac4afb10bfb3421", "SHA-256 113 bytes" },
        { &g_abRandom72KB[0],       114, "2afe58676676f4a44a422cafd1c3ca187a7cf7dd54d4901448e59b683a7ce025", "SHA-256 114 bytes" },
        { &g_abRandom72KB[0],       115, "5b88845e99afb754ce84cc42d99ddfc9022b55175d3cda8c56d304450d403ff3", "SHA-256 115 bytes" },
        { &g_abRandom72KB[0],       116, "4255156c12f13ba85659d5d0b8872ae63e0c98075c06c64673ab6b1e253cab71", "SHA-256 116 bytes" },
        { &g_abRandom72KB[0],       117, "c5e0fe1632cd39d9bec9b7047fbdc66f6db3cb2b60d4eef863a4c5f43649f71d", "SHA-256 117 bytes" },
        { &g_abRandom72KB[0],       118, "e7be5a747eaf858c56ab45d1efd8317dddea74df01e188d2d899aeb00a747873", "SHA-256 118 bytes" },
        { &g_abRandom72KB[0],       119, "e4642107146d4b94dcede9a4fdcd4f13ab88cf49605e8a7cfe303306288bb685", "SHA-256 119 bytes" },
        { &g_abRandom72KB[0],       120, "39261dcb052d46f1f811f6edd9f90805e0a2ff0b86afbdc59da4632b5817e512", "SHA-256 120 bytes" },
        { &g_abRandom72KB[0],       121, "b1e2988090ddd589757939f2b0912515998b7ac9ec8534655f586764b350fe78", "SHA-256 121 bytes" },
        { &g_abRandom72KB[0],       122, "c21143977ad672e9458d403b1da4da2553ac113eb0d1bb905c781badca957c30", "SHA-256 122 bytes" },
        { &g_abRandom72KB[0],       123, "173a78a19a11875f87163c5f111be2ec7a39d1358051fd80141b12576f4a17c2", "SHA-256 123 bytes" },
        { &g_abRandom72KB[0],       124, "e499062db198b79950bb482e99c0a5001bf76a339b3f0da5cec09e3ec3f11599", "SHA-256 124 bytes" },
        { &g_abRandom72KB[0],       125, "fbdfd0d05db20c67fda9f2d86f686e29f9fac48c57a7decf38c379eb217768b1", "SHA-256 125 bytes" },
        { &g_abRandom72KB[0],       126, "d8435c9cf281b3fd3d77b3f0dcab2d3e752c78884b76d13c710999731e753e3b", "SHA-256 126 bytes" },
        { &g_abRandom72KB[0],       127, "038900394e6f2fd78bf015df757862fb7b2da9bde1fbde97976d99156e6e5f3c", "SHA-256 127 bytes" },
        { &g_abRandom72KB[0],       128, "37b31b204988fc35aacec89dad4c3308e1db3f337a55d0ce51ed551d8605c047", "SHA-256 128 bytes" },
        { &g_abRandom72KB[0],       129, "069753c44ea75cddc3f41c720e3a99b455796c376a6f7454328fad79d25c5ea8", "SHA-256 129 bytes" },
        { &g_abRandom72KB[0],      1024, "c4bce478ad241b8d66bb71cf68ab71b2dc6f2eb39ac5203db944f20a52cf66f4", "SHA-256 1024 bytes" },
        { &g_abRandom72KB[0],     73001, "92a185a3bfadca11eab70e5ccbad2d40f06bb9a3f0471d021f2cab2f5c00657b", "SHA-256 73001 bytes" },
        { &g_abRandom72KB[0],     73728, "930de9a012e41bcb650a12328a45e3b25f010c2e1b531376ffce4247b3b16faf", "SHA-256 73728 bytes" },
        { &g_abRandom72KB[0x20c9],  9991, "8bd4c6142e36f15385769ebdeb855dcdf542f72d067315472a52ff626946310e", "SHA-256 8393 bytes @9991" },
    };
    testGeneric("2.16.840.1.101.3.4.2.1", s_abTests, RT_ELEMENTS(s_abTests), "SHA-256", RTDIGESTTYPE_SHA256);
}


/**
 * Tests SHA-224
 */
static void testSha224(void)
{
    RTTestISub("SHA-224");

    /*
     * Some quick direct API tests.
     */
    uint8_t    *pabHash   = (uint8_t *)RTTestGuardedAllocTail(NIL_RTTEST, RTSHA224_HASH_SIZE);
    char       *pszDigest = (char    *)RTTestGuardedAllocTail(NIL_RTTEST, RTSHA224_DIGEST_LEN + 1);
    const char *pszString;

    pszString = "abc";
    RTSha224(pszString, strlen(pszString), pabHash);
    RTTESTI_CHECK_RC_RETV(RTSha224ToString(pabHash, pszDigest, RTSHA224_DIGEST_LEN + 1), VINF_SUCCESS);
    CHECK_STRING(pszDigest, "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7");


    pszString = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    RTSha224(pszString, strlen(pszString), pabHash);
    RTTESTI_CHECK_RC_RETV(RTSha224ToString(pabHash, pszDigest, RTSHA224_DIGEST_LEN + 1), VINF_SUCCESS);
    CHECK_STRING(pszDigest, "75388b16512776cc5dba5da1fd890150b0c6455cb4f58b1952522525");

    /*
     * Generic API tests.
     */
    static TESTRTDIGEST const s_abTests[] =
    {
        { RT_STR_TUPLE("abc"), "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7", "SHA-224 abc" },
        { RT_STR_TUPLE("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
          "75388b16512776cc5dba5da1fd890150b0c6455cb4f58b1952522525", "SHA-224 abcdbc..." },
    };
    testGeneric("2.16.840.1.101.3.4.2.4", s_abTests, RT_ELEMENTS(s_abTests), "SHA-224", RTDIGESTTYPE_SHA224);
}


/**
 * Tests SHA-512
 */
static void testSha512(void)
{
    RTTestISub("SHA-512");

    /*
     * Some quick direct API tests.
     */
    uint8_t     abHash[RTSHA512_HASH_SIZE];
    char        szDigest[RTSHA512_DIGEST_LEN + 1];
    const char *pszString;

    pszString = "";
    RTSha512(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha512ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");

    pszString = "The quick brown fox jumps over the lazy dog";
    RTSha512(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha512ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb642e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6");

    /*
     * Generic API tests.
     */
    static TESTRTDIGEST const s_abTests[] =
    {
        { &g_abRandom72KB[0],         0, "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e", "SHA-512 0 bytes" },
        { &g_abRandom72KB[0],         1, "a9e51cac9ab1a98a599a13d05bfefae0559fd8c46abae79bc15c830f0153ba5f05a7d8eb97578fc71594d872b12483a366125b2c71f27a9e3fb91af9c76e7606", "SHA-512 1 bytes" },
        { &g_abRandom72KB[0],         2, "0f69ca22d38f562e2221953cdf3ee0b360b034c5b644ee04e0f58567b462a488578b4b75b15ffab8622f591c5e74a314cc4539929f14941969bbd3bc6770f499", "SHA-512 2 bytes" },
        { &g_abRandom72KB[0],         3, "58bebd4e3f1e5e724a4b158db50c415ede9d870b52b86aa9f4e04eadc27881e8e0f0c48da1f2dc56bbf5885801267d10bdf29c68afabae29d43f72e005bcb113", "SHA-512 3 bytes" },
        { &g_abRandom72KB[0],         4, "e444e041d0f14e75f2cb073c3d6d75a5a5f805e7292b30dfa902a0df4191422a6ff7d7644d28715a9daf01c4580c4b5ea94523e11d858ffc3f4d9cbade1effb2", "SHA-512 4 bytes" },
        { &g_abRandom72KB[0],         5, "f950ebe051723a765edfa6653e9ebb034c62c93fcd059ba85b250236626bda33408204ff9dccc7a991ab4c29cedf02a995b8a418b65dd9f1b45a42d46240fdf0", "SHA-512 5 bytes" },
        { &g_abRandom72KB[0],         6, "c39979d4b972741bf90072ca4ce084693221370abcbfc12a07dbc248d195ad77d77d1fca2f4f4c98c07963d8a65d05a799456f98a9d3fc38f7bf19b913beccbb", "SHA-512 6 bytes" },
        { &g_abRandom72KB[0],         7, "ac5653cb0455277c9e3294a2ec5ad7f2ef8f9790a01e7cfabd9360d419a4c101d4567cf32b35d291e3cc73747e6f3b2d89a95c96421468db5eeae72845db236b", "SHA-512 7 bytes" },
        { &g_abRandom72KB[0],         8, "454a56d5f9df24303c163430d8ed19b624072e2cb082991e17abe46bb89c209fd53beecb5182f820feb13d0404ab862bb3abf219c7f32841f35930e2fdf6c858", "SHA-512 8 bytes" },
        { &g_abRandom72KB[0],         9, "e9841c44180cbe5130147161767d477d1a118b12fdf3791a896fdaeb02c9d547b4613a6a3d34bb1311ff68f6860f16f7bb341a2381a143daac3d3b803c25469d", "SHA-512 9 bytes" },
        { &g_abRandom72KB[0],        10, "450915d9d69e85ff87488f3c976962c32db18225ae5241a59e4c44739499dc073e1503f121065964c8b892c1aef4e98caf2a5eff10422ced179e1c8c58aeadf7", "SHA-512 10 bytes" },
        { &g_abRandom72KB[0],        11, "059d9629d61a58bede1ae2903aef1bcf1ef6b1175680a6e795def167163d05202167e22dba232750bc58a150764c13c048b2d83c0b077d819a1bd3a92432d97f", "SHA-512 11 bytes" },
        { &g_abRandom72KB[0],        12, "00e3ec8f76e1bbdeae3f051d20369b64759b9f45e15add04e4bb7590b90ce9c37706e36b68dfdbfaef8b8e8d41e42732de80314a1c58f89a8a26179d4ba144f2", "SHA-512 12 bytes" },
        { &g_abRandom72KB[0],        13, "dfde69e66f61bbd8c2d5c56493670d9c902df8b5fde0bdb61dbcc902390290c159ef1dfd988813a14c424c6bee49e62c17140918a4ad805edf9ac67ffe6af6ed", "SHA-512 13 bytes" },
        { &g_abRandom72KB[0],        14, "5d7908aada1445e9edecef34cbedd032baa646a88aafe4ebf3141008485b640539ee78e56b18d34bfcd298178412384d993d948e5711c61500a53085e63e3bf1", "SHA-512 14 bytes" },
        { &g_abRandom72KB[0],        15, "ab6102021cbf19cae1b0633ecb0757f4a280b8055fa353aa628ba8d21188af3daf08323367c315dd0eb9ba8b58f415decd231f319ad7cc9a88156229693c2061", "SHA-512 15 bytes" },
        { &g_abRandom72KB[0],        16, "75e2d97633b22af732caa606becf798945eb91be1d04f66b2ec86b8df98d12f7de8baa6db48b1294a713f17d792c89cc03726a7b03ae0d16837de1b4bfa5f249", "SHA-512 16 bytes" },
        { &g_abRandom72KB[0],        17, "e6bbf8ab9c68e99f569f6c68d93726eddab383e3695ab84383425617e73b6062970045fca422b393d479f3ba821ce205461ff76d0acf67fd4a83e5eeb5ccf594", "SHA-512 17 bytes" },
        { &g_abRandom72KB[0],        18, "68cfb17e1fe58ce145c3ce44efdb12b65a029d19ca684a410e0243166e86985b92f8e815de17f36f2b0f54e08c6a49c5446be14ebce6166c124d8605189adc24", "SHA-512 18 bytes" },
        { &g_abRandom72KB[0],        19, "fcaeefcae96683fb1337170f5f72247252843322029ee9edc7333f14386d619f2ecc12707ed4f1aea5f7c052c682d8c9e518a47c4fc9f347299688818baf0468", "SHA-512 19 bytes" },
        { &g_abRandom72KB[0],        20, "22dfd1b7af7118d93645f9b7baf861a317b4318a8881e4cb6a7669298560849c9169d86b57803c0de0c82f1a5e853ee571cae3e101cd8aad5d2045aec0805879", "SHA-512 20 bytes" },
        { &g_abRandom72KB[0],        21, "87de7e747b5ac2401e8540498ff8222e80b28bcb570c7f873332b01f5c6c0b672b8ac65d4986bd4463fc5f6f7f16656cc300bc9a5be9873295b1a0a13ce2aa28", "SHA-512 21 bytes" },
        { &g_abRandom72KB[0],        22, "deb01211010e06351e5215006bd0b09b353e6af5903a5afe9236f7418be7bd804119942be2be2cef9dd86b7d441300d8861ae564cad3dd5ae3eaea21de141f22", "SHA-512 22 bytes" },
        { &g_abRandom72KB[0],        23, "c7c62e3033755889d9b690f9b91288ea79e9225068b85981d4c3f2035ecde58a794fa59938b3206e0b9edea380fad3d9534bf40e889b59d8db52261f1e06e7cb", "SHA-512 23 bytes" },
        { &g_abRandom72KB[0],        24, "ae825238a7ac961c33b7b4e3faa131bca880bbd799657ab89e8e260fc5e81a160490137587c13b89af96cc494c884bac3e3c53a5e3838ffba7922c62f946fa77", "SHA-512 24 bytes" },
        { &g_abRandom72KB[0],        25, "a8bd5532033300fc15292726e8179c382735650e9c2430fedd45c6e0ec393f87e04a53901e76bf9ea089c41b9c73e34440f7c7503ed872ca61b61301aaacd71e", "SHA-512 25 bytes" },
        { &g_abRandom72KB[0],        26, "a2439151940845db0bfd7f4a955e0a7f19d594759917530fd2afa1f41ab30afa1122cc00875bbccf3cbe4c3afb94deaf6abcb23ccf379c6ee31ac1615a16b12a", "SHA-512 26 bytes" },
        { &g_abRandom72KB[0],        27, "5a0d49012aa539aef64a3f4e0933efdba3610b7aee6e8e2ca26a5550799141a3174001eff9669c8194eb2b60b7e264e02b73fb4bdeda61076423b26f50b210f1", "SHA-512 27 bytes" },
        { &g_abRandom72KB[0],        28, "09847f1c906c9f5507b499ae6a8543ee3dc04b81eee072148b0c55668ce363a139e54f74da9feb5fadddfa0c920bdf595ba68bf7ba21c6e131e6c4b598d444d9", "SHA-512 28 bytes" },
        { &g_abRandom72KB[0],        29, "80efb6be55a081fac36fc927326b5b6e53c91aadb9106d8b3861ecab9b08b7b6ebdea386b5132d6e6cd7d096ebe912577bccde6b709d5f2f2e8f2ce4cf8834cc", "SHA-512 29 bytes" },
        { &g_abRandom72KB[0],        30, "a052774739540e095f4e54ebc77b6b4a60253c20231d077224a9bfd69a882b5bb4db7ae52a8c475ef15a95d8fc2c33064fdaf356f151a429cd0625b218708eee", "SHA-512 30 bytes" },
        { &g_abRandom72KB[0],        31, "6501f635b4690ccd280443d2eeee36cc6d626df9f287a8e46e237a0528b82aafbce5a23d75a71ac611cee62c1c0423d5e4dcec14b5c31907fbe2f60406486bee", "SHA-512 31 bytes" },
        { &g_abRandom72KB[0],        32, "a7b4e870a1a4585439f62da7e99998f60daecc90849c16ce1f5b4bc15e68e567b8fb0c44d1294eea925bc14161bb9f9e724933bfe88d8aab9ab2dbf4e00f7b88", "SHA-512 32 bytes" },
        { &g_abRandom72KB[0],        33, "ce465a93568976550943c160907615974b5c7fbbeb05506905d516a56dbf6dd7f2c8f45cbcea10d1c85316595559e3ff8193629eadbcd941359e166874ae4058", "SHA-512 33 bytes" },
        { &g_abRandom72KB[0],        34, "b5f60488576dccbeca6924483f1732055af0383e63acefc6dfc13e4e6b77655a97f10eb5778a28d939ca3f49311f4c0d2920764f10aa2fdff6d60a24e8d68d04", "SHA-512 34 bytes" },
        { &g_abRandom72KB[0],        35, "9509b9052efcff757291296a58fbba6835ccfc1b4692307fbf683a22379242a4d6c8324dcea6a870e50c40960cd56e0f52c41102f25f357328d8abf1b9499441", "SHA-512 35 bytes" },
        { &g_abRandom72KB[0],        36, "dcaf59cf1dc3c5dfd74595e53bda617c313ab65dd4d04821e17bb65330458827d89efac7d64556bc439c12d326d4312d90789f39801d0d9a0faba59d23a57f40", "SHA-512 36 bytes" },
        { &g_abRandom72KB[0],        37, "fcd30053a81adf7a9e5135e55fed6f389b6e516b04a0e9972b18df387a3dcb064f2903ea58d4b3e2f51c30a2ff0d7353b9d744e13e215a3a63e486130acca9fe", "SHA-512 37 bytes" },
        { &g_abRandom72KB[0],        38, "36cc41b6dc5602f8d71a0ea7f7187a648c7b78d9386c3f4cb952153b57fe6c307957d2a39752a5f682dee98e68a4b4e99a6e681f76ad6b561be3ba2bb7409e88", "SHA-512 38 bytes" },
        { &g_abRandom72KB[0],        39, "09d0e8487f329c2b66618e8734e2a5e7a68654cd744e5941ceba04e2f7dcf92af1d3ac5ad3920abc4b3b0e121a68a4665e0b6ed6eaa3dbef06cd3c21217f0445", "SHA-512 39 bytes" },
        { &g_abRandom72KB[0],        40, "d24e246470f0fa260976b1063f5a46a73c54614881173f3793c70a47f925bb0d05feb376858bd838f550cd82f7122b02b438ad3facfc9049be3efd7e5f3b03f3", "SHA-512 40 bytes" },
        { &g_abRandom72KB[0],        41, "6e56434ce4c50a1989df0de44485697a53ca6b8560ea84290fbb54b4627a980412033f4ed2aa795cd653dbd0134d4e4e2f25b57a20d55ee1ae8aa47d6e8af7d4", "SHA-512 41 bytes" },
        { &g_abRandom72KB[0],        42, "f9cce63deb2f4291ef74c7b50cf573bd99500b6aeee2959fe3500a7b55b3693df4f5b4eb5c242271e5b9ada3049b2a601e85f32d0501b870fa1593ea9cfa6fb1", "SHA-512 42 bytes" },
        { &g_abRandom72KB[0],        43, "0781bf3344189b3c10dfc34e6bf3f822a78533f0a29b066c67f49753412c40075b6d988c2c0a338fb8372be0b66a97a73c382509613e7fb908bed5cce9804676", "SHA-512 43 bytes" },
        { &g_abRandom72KB[0],        44, "4af952c3a51e5b932b21ce3b335213235ed7d3a2a567d647d6faa7e32e647038d0dbde3b6638453a79badb9c882d75d5201c5b172989d3e8b1632a15773fd2aa", "SHA-512 44 bytes" },
        { &g_abRandom72KB[0],        45, "98e0b281d765b90360338afaa7cf1ec90d841dfa07c05e7db74dc96dc7bf53465eb78d6e8c915c53d273fb569593d51a81331b56bce69eb506a3a400c73b170e", "SHA-512 45 bytes" },
        { &g_abRandom72KB[0],        46, "50c7c00f254a13b2288c580ebb165be5669aa88ae247aaddc8f20750564b945849f5126a3cc0fdf7926a59b7c7b7866fe49174bf1dfbbe5734a2fd570ff26183", "SHA-512 46 bytes" },
        { &g_abRandom72KB[0],        47, "e5bcecb9ce32ccfd4916a267f64b9d478461d10066375f0088f7ffc2e43b393d09613e8f60c623268c863b91ef08059b0af491d15d040fff14ef3b2c89e84d46", "SHA-512 47 bytes" },
        { &g_abRandom72KB[0],        48, "f30c0d42f15fc467f454cc8b7d3fb3453150a57f28eacd9870fecc98cae43dc937b4e45cb3443e6c09eb8d82bfdd1c501870f827d5d792dbbaf3beb9dc7bfaba", "SHA-512 48 bytes" },
        { &g_abRandom72KB[0],        49, "12bb4030e966cf51ca3ead169bfdf399e1564b2c7a7636e8625d288473967643a85e932ed45d9b9ecf9a05001e34d259a8d4733faf7ee9093e1a64b57fe357a7", "SHA-512 49 bytes" },
        { &g_abRandom72KB[0],        50, "e21a552fe775cc337c4bccb934f4093d03c4c6c5e2e50ba6cdf0cd5ddd74d7b3de6690e8d3b2ff195b9015e3d137a6f4b6e683148e4a8c7914272767cb3fa68f", "SHA-512 50 bytes" },
        { &g_abRandom72KB[0],        51, "dd262372035812a20304a313866cbd9c6c9099d4c434630d3904618bee471967e4b715f7e3bc4f5d3283b4131b8885af5af645289faba8cfb095960883eb5d31", "SHA-512 51 bytes" },
        { &g_abRandom72KB[0],        52, "0541767fd127bd553915f6fc4e11336c734bee7409c972f014952128152658339bb641989b19eba9d73f7cbe2561fdf5c00e7ac6d7a3b4dedd3249cb11357a0c", "SHA-512 52 bytes" },
        { &g_abRandom72KB[0],        53, "6446470e96304a1e2ca9946de70d8c61c0c39e23900274db6262042e5555663af6b25d7d7dcd1f1f890e14e1a588b498e8cc26e8aba117bcd61ef74b25603645", "SHA-512 53 bytes" },
        { &g_abRandom72KB[0],        54, "946bb0ced082b07b965d57c3634011f13142b9dd84146103262483b7fb7c3413bd481ad1a3e01005e974792a6523a2c22eb2211ce7912c2e88378ac6d962f809", "SHA-512 54 bytes" },
        { &g_abRandom72KB[0],        55, "7d742669be7bef2a4df67e5853146cbd8e8ab9afb1b3429d5eac7e1fe66e050d7c08eac3e90b596eaad00ddb92ac8876a50eebb4fddc17b39abd79c83dc2523b", "SHA-512 55 bytes" },
        { &g_abRandom72KB[0],        56, "5b25df3a85495f600cc1cd92d53e969adbb3329a87cf8f32361fc2aea3331bedfed63c25a5e74a38790b2d4a96bf1e0e4df9f0ac8e8d07813197a575939cf444", "SHA-512 56 bytes" },
        { &g_abRandom72KB[0],        57, "9aefb636d982bf842ec37bf91f5c01f65796f954e26eb3cdd35515aef312b93f72ffc6fbbe3eb168fb07973471ebdd33b302e098c8b5d949d29afe129761137f", "SHA-512 57 bytes" },
        { &g_abRandom72KB[0],        58, "7deca25f1ca788598002368c2e07c5b65766bc28e66b1fe1b1a16dd38134a64f2a51be3a9c8930da939971e48ce5d9f25141a619386639eaaee11786c9c4df6f", "SHA-512 58 bytes" },
        { &g_abRandom72KB[0],        59, "27e4c254f1284dbece4f63aba4f61ff6cc23abe4ff2e86d24ccfd582a2cf65145849de27d8292a66391466f965e61b06772b1f8a7b5a5e69f8b0f0a6d4974d40", "SHA-512 59 bytes" },
        { &g_abRandom72KB[0],        60, "39e3a628a14f307028e3f690057ef24a02533d2141e0cf7070d4f2ba5d58e92d5c27e9d6dd8e07747915ea5535f8963ef350c424d6f3b3dc3850256a216b4ae0", "SHA-512 60 bytes" },
        { &g_abRandom72KB[0],        61, "8877a71d1e412e4fe0d60bbcc3ac61c458a221311b1425defbfa28096b74e18b2661ae03026da38d6ed7a42b49850fe235145bb177a7e0e99b12977bd5eb4ce5", "SHA-512 61 bytes" },
        { &g_abRandom72KB[0],        62, "a8623e4a43560950427020b64bd70f37ce354698d926c0292614b89100f6e30947db498bb88b165dc50da54439321a564739be36de02d134da893a4e239bdd01", "SHA-512 62 bytes" },
        { &g_abRandom72KB[0],        63, "62d7f6e428f8252723a265ce2311aa11fed41dfd07705cc50a24d744cf8ade4817cd3c5a22deae04bfdebe9292022ccd87e5fbe8fac717ee1ef01d2a336dcef3", "SHA-512 63 bytes" },
        { &g_abRandom72KB[0],        64, "1d356bccb9d9d089bec81a241818434ab1157bbafc7fa1fad78f17d085430bd6efffa409efdd8bf4306927407272e14f70f5344ee5085ccbc17aac16a9d40ae7", "SHA-512 64 bytes" },
        { &g_abRandom72KB[0],        65, "9c500cc115fbd8f609890b332ed6b933c2e1b60664ac57939776348b394a3af6ef55b740351cad611bed175bc932971d7803caa3802e765d14e795e83ef81727", "SHA-512 65 bytes" },
        { &g_abRandom72KB[0],        66, "862197dff8c67ea30846b25adeb191546c165cefa7c2fd3bebfdcb038884dac5bf0f5274417c2834a63751bfbc744193bd8bbfc2f261a01a9a3c2914d5069913", "SHA-512 66 bytes" },
        { &g_abRandom72KB[0],        67, "fadb5858a9071e696371c37287e0d7ee476fca005de6d1049162bded431bc9659c1f981c110fb8fade5495a6207af819260512d0160d11dea295856f4fb55d4c", "SHA-512 67 bytes" },
        { &g_abRandom72KB[0],        68, "029b208137092c9f38084ace884d371c34434e3268802aa7b6276697eca683d23082a9c4d81f8b871adb99cb2a4f73d1064c16feec7c3df282594045250eb594", "SHA-512 68 bytes" },
        { &g_abRandom72KB[0],        69, "a6f78eba388c4b96ae3b30a6204f0d4bc7ee6c3dadcaf18a938dba69a0a66fef1a6a2d6589da0e8990bea7bbe46cdbdae700a464f394c8f1c13821f49b9ce08b", "SHA-512 69 bytes" },
        { &g_abRandom72KB[0],        70, "f39c31993dd96a07cb0168c4d72a216ab957d56d2e5a73840ed7ea170659434c0ca6d5c4d70eae040ab0f488cd1b93453c85c398b3fb2cbece762e5eebeae3ae", "SHA-512 70 bytes" },
        { &g_abRandom72KB[0],        71, "90d60c76601deeb2f3c9c597a04a333724fa4efbca2fdbb09163bff812615841495b79225627b6da100aaec0ac7f532a782075308aed7c6760e530430f77c063", "SHA-512 71 bytes" },
        { &g_abRandom72KB[0],        72, "604e315e1db8ddf64b5d11151113c25b61b66d690046c1830bccfb0b92cee65ade2ed75691bb65be9df84d4d83bb9b0a3311453c9f7e30a04889882e74383d75", "SHA-512 72 bytes" },
        { &g_abRandom72KB[0],        73, "bfd93659349d3ee86f88ba312087c97cfb6989d33eef7f59ef9fd1ed650c4e10b5172b9cd90bf4982c85397c0de2fa691f5b49617e0bc168f45c084093cf3b41", "SHA-512 73 bytes" },
        { &g_abRandom72KB[0],        74, "6084b22bfbc488cc5c61ff382137fac5dea084d32e49aafcf2c9eec3d1cbdcff8f093df63913743b55a16304b0d8adb663dcc37e6d933a6212c1aa13e4acd2f5", "SHA-512 74 bytes" },
        { &g_abRandom72KB[0],        75, "2a9a4625797f6137b4200be28f07bc8b183e098e139d427c6f0b7b5ab19f5f0b4bad407424414e2475f05cf0a8055deefa0c8503bf1c09d545634c6cf4d4337a", "SHA-512 75 bytes" },
        { &g_abRandom72KB[0],        76, "88e7dd70a561537bb978e295cefd641a4bc653cc1dee9f8cafe653c934f99d2b7c18caf26abb803acbc8172eab34cc603aaa35aeca9444c5759016415f076430", "SHA-512 76 bytes" },
        { &g_abRandom72KB[0],        77, "78fbe62ea044c5601099b5605486bc911bb88e325077651722fd4790554f6a01c7cceaddd38d850925b05852616712118d356b7b023fdc7facae720a2b3008fb", "SHA-512 77 bytes" },
        { &g_abRandom72KB[0],        78, "1e35da455a47b0acc9ee2c6970de8376ee3ad22b53c39a6613651de23fb323aff796d9b9ee7c3f56684bca29bc16df9e2e4846d66ee4f6e720eb8c01b305f166", "SHA-512 78 bytes" },
        { &g_abRandom72KB[0],        79, "a1d26f27fc25892a1329434d6045384b62b32d61a645c06645493b32aaf0d6be7761828c04424d778214863db047fbc34865d0c4271f1d22206b60eb16cf92be", "SHA-512 79 bytes" },
        { &g_abRandom72KB[0],        80, "8ad44d72bb9a3a7436f26577275b97eef79a27a5de9eda9f1c5ebf740cb2e1198acb0ce774f395bba1962c570f19278eba8a5928f740a5cc3113cc6b6627d8e1", "SHA-512 80 bytes" },
        { &g_abRandom72KB[0],        81, "54556b5b7a14e090491256ab964cbb819d57e0f3fd8acf7fc9ff87ccb0a89f388e8083fe6e86eb3da3bb21c6dcaa291bb6e48e304628e9d1e2c13a3e907645e5", "SHA-512 81 bytes" },
        { &g_abRandom72KB[0],        82, "9997f6934072546a9ca5a1824589db692ac9b25b9be49f82b769efe094b8909e16037644ea88ca86501ff3ea533fdc5b81ce8e3456e07559b218aee682c151d5", "SHA-512 82 bytes" },
        { &g_abRandom72KB[0],        83, "5bc87e566b62eea1af01808cf4499dc7d0e06ced3b1903da2a807a87b9cd4c8ffd4d46e546a8a18e815efae0df5eb70191c8afbe44aefae02e2e2886593618f7", "SHA-512 83 bytes" },
        { &g_abRandom72KB[0],        84, "a3231d21aea1381fd85b475db976164dcae3cd9ce210285007b260b63797ca8d024becdd6b5b41ad1772170f915ecb03785e21224a574e118c5552e5689fcd47", "SHA-512 84 bytes" },
        { &g_abRandom72KB[0],        85, "d673935769d0ac3fe6f522b6bb537869c234828139a1a39e844c35592f361b4e39e55c7e49bd0dd7588ce8e30f7b6aeed6cf3ccfd951589a27dda37b1a2fadeb", "SHA-512 85 bytes" },
        { &g_abRandom72KB[0],        86, "a0fca2a6e0126fa9a9b84947167ea3a42aeaa69d8918a175e0bd0be20f8beba333e59dd3905029da37cb54740f94bc1bf688be4756e6c6769de9658566d07c90", "SHA-512 86 bytes" },
        { &g_abRandom72KB[0],        87, "a8730b6e7d1c850c40c797ec8d69dc4b26473e692cb0e3fe8781be355574bb921c4454a8d54fa2e607a0bc36be9a5ab324e0f9f439bf1cce93d0ebfd019154ed", "SHA-512 87 bytes" },
        { &g_abRandom72KB[0],        88, "77857fad35e564a8d5dcea9b981bcb074dc6aaed7626db8132e08555dae7f5445c0378a6dbfb24bf5f2c4c6afac09cb19ac1ab76ed4f41fe3a72d32ca1f39f00", "SHA-512 88 bytes" },
        { &g_abRandom72KB[0],        89, "6ac4c2044f4d9a983c1e41ae22daf47ef05a4ee2f013a174a3068d38c59994a19b8788541a29f47f3000b4c0491dacbf98e98dc0588a3a30fee42524697ab996", "SHA-512 89 bytes" },
        { &g_abRandom72KB[0],        90, "eb894f383342fed8371ec72403c636b1bcc7f6f39d6db1dbfc58f9fac8f41552af6dc0a5a968146c028c4db113b21831d80eb0ae166d68616ae3696832f4e563", "SHA-512 90 bytes" },
        { &g_abRandom72KB[0],        91, "8be91181345c9511f2c74df93db07ca739a586a2a84f38d5f063244d2e39fc4254c5747787e08b74a2a59916c9751aa30d0fe57a18d858d3346186facc86365d", "SHA-512 91 bytes" },
        { &g_abRandom72KB[0],        92, "43e007dc454a4303724a2baa860307083a3a1e1d52b587cc3b7a4b6bc252c8dccad0e9cf9f2b16ec178371e57033ac3c4e71dbcb2eb69cf7ef7f617c7ada8f76", "SHA-512 92 bytes" },
        { &g_abRandom72KB[0],        93, "2e2d6686c4e7a7919879ee03a4fd37ddc71a7e2ffa17a3cfe6eef24b29d8a1e7cc0fa5d8e9dab3c6c3190652ceb9e1ebe67a0ac7d92c6c205a9e8add91bbd2a7", "SHA-512 93 bytes" },
        { &g_abRandom72KB[0],        94, "c8b9e3825c4a81c63f79b88d91f61af15c714ccc611dc77635c1bf343e2c185caa2e0ef8eb76505f1544c8f78377c26d3a6f79c2b77abadba9906e583fb2e5c3", "SHA-512 94 bytes" },
        { &g_abRandom72KB[0],        95, "0bc63075ec108a5ee1f69a4e39ae03333da8c1f2d61a94fbe6357c143fa2dfeee44fb8cad0ee016fd42697f6848b7a174b4a77a268401f7cac4c4df1623cfc52", "SHA-512 95 bytes" },
        { &g_abRandom72KB[0],        96, "b4aa1d4a849840413e830e27c388baed9e8d4a0941048aad9a4b8751497d86e002e3e50b0197a9164ea440ec4324771229d0c5de04b9922c992d97ea736ff477", "SHA-512 96 bytes" },
        { &g_abRandom72KB[0],        97, "1c5142f15476f56abed2a0c5d9450e14f56a30af2d5d81f16e05ff0184d695fd1923488da707b570307370c4f1669a522230617f347c264465c12d82fa2018b8", "SHA-512 97 bytes" },
        { &g_abRandom72KB[0],        98, "7146b68a0670d5d901e312c78a728111975c312b8365a3cb9618cbe9c124c30d65cdf668902bd9ec76493caa0ff40e3c1f03ccb06e3b2380c69c154fd065d137", "SHA-512 98 bytes" },
        { &g_abRandom72KB[0],        99, "e591fc00af6733a6e308577e0043d5c12b81051848d8123e4350b82037350f27828ed6cdc0b1daf9ef57b30edd72b58370cd1851545d9e39ddeb00fbc66c8582", "SHA-512 99 bytes" },
        { &g_abRandom72KB[0],       100, "6328c48376ab29fbebd732db8b4073a96b2358de13c8a2a5a5ddc2502d8e0356822da65551bb079d4a90f3fefe5b8d00cd186696706900471348784a55009a0c", "SHA-512 100 bytes" },
        { &g_abRandom72KB[0],       101, "c90b01b14fd712be70ac318b21974181418365cd94a33d4121260c260d6f72e0819195d5c3f83b516e7e9aaf667957bc67c0c0a44a35d7756b41e33c3677d017", "SHA-512 101 bytes" },
        { &g_abRandom72KB[0],       102, "d23e5ca2c0f460d6831abd112b6a6f6d3aebdc500f7af96d887aebbaceae2e33f61f5a423deacee85276e796478d5002c0b94e85c0b2d55c75752a8a717785e2", "SHA-512 102 bytes" },
        { &g_abRandom72KB[0],       103, "e20df98010b277e188ddf6d3e953722287cffcf43c910e80ab9dceec32aff3c2059d0268bbbae414bc08919adb92d409035ab1a970a175aefeaced8df2395e13", "SHA-512 103 bytes" },
        { &g_abRandom72KB[0],       104, "774154db3993f0b4d05f03e187249cc9db94e736807691de26f734277149597ac68acdb412f8fdf3a71093cabcd0257d39f3c5e8eb7d3545474914a5b4ebc623", "SHA-512 104 bytes" },
        { &g_abRandom72KB[0],       105, "30df8e2d774fa706d8ffce6009237122540d20d4d92e5c9e2f19c4b9fc91ef9454254575e1a94cde8930439b679f2702ad22145c983418fc5b48d1b0d46e7677", "SHA-512 105 bytes" },
        { &g_abRandom72KB[0],       106, "091b5252043b7d47aab184c82c3317f2674ff8b0b76679de6c4e27ac15fc653597d635c8a0d3a0c0f271e86b7c9db86c622449bbb1044f6b26ebf7f681ab108f", "SHA-512 106 bytes" },
        { &g_abRandom72KB[0],       107, "152e06c5313eb9c58345878c73180ab82c0a66ab853653a54f1195f351eca82ebcbbb6c7705bd20440eb69e9b73df9a4696421205f9b2244e5765be5493b0fee", "SHA-512 107 bytes" },
        { &g_abRandom72KB[0],       108, "cdc5a8269c1aa916af26a6fefd395f338fd9ba7b7ad578fb5e7d92be956a83c43cc7cc8f590450878d45e28882d204bd244e2e5bf6309e425170024d8d307a87", "SHA-512 108 bytes" },
        { &g_abRandom72KB[0],       109, "6c120d70dda091f8de847241e6921e8540fcf0cca9927ac673bc9d2aa29bc5a782134a79250d9fa1bb436ebf73378ab0619a05140473ff0c2be7292f34e6d6a7", "SHA-512 109 bytes" },
        { &g_abRandom72KB[0],       110, "7094f3e32b2075db6290da8a3379b138675943f647d14444bc9299c01a5ea9c58706386030970f4d670132d9d7a2485064c901443f9f1050679ad09e576ae2df", "SHA-512 110 bytes" },
        { &g_abRandom72KB[0],       111, "a56e148f8cf9a20c96453ca2cd29aea8ec9b76c213f72d609a1052f81efba24b7de365214b21dd09447b8df272e1458f566f3af827d96ea866c921155ed5b85a", "SHA-512 111 bytes" },
        { &g_abRandom72KB[0],       112, "68ab4ac095d2aaa05c61ea622804c3164f27f8fb0adbd070906e75093cb09e2b283aeda64584cd856de8ef369f64a0900e0e191f08a7f729ac9b9077efd41c43", "SHA-512 112 bytes" },
        { &g_abRandom72KB[0],       113, "1ca365ab12d42b4ccd1561a2000b885fd38df9a60b0f8580c85e2547f40be8fcee465b24b7efd09fbac92c4aa74fe403fe7f00aacb7aec5d187403d34da4b6c2", "SHA-512 113 bytes" },
        { &g_abRandom72KB[0],       114, "17652cc2481a32c42f9cdc447bbbc3a95c76d7f7b2cd6ca93c29e0beeef219baf00f1d3514c3f9fba7a6c70177d3371a0e95638f1c210135f9bcb7322e513749", "SHA-512 114 bytes" },
        { &g_abRandom72KB[0],       115, "0d42ab4691599cdb280b61fd170a8dd7970639ab074109cdc91bbc94714ea257cb1c35a3c5c471d8853c02e66d90de0b66d9c04adeccbaed7e48d517d7b763b4", "SHA-512 115 bytes" },
        { &g_abRandom72KB[0],       116, "d410d63adf1dadec0eb6712f30ef2f3f56aef3e5e0ddd4fa3088516bdefe1af3d031869b9d4642013bd1d907a8687db86363a9e94d9e08bac6576b0cf0a3f877", "SHA-512 116 bytes" },
        { &g_abRandom72KB[0],       117, "adfa29d3e85a68a153a470cd271293a6fcf27b67b45b232cc6f1944375ae6254760bdc700ae33bfaaa26490a6a216982379e3973081d210034791f60a1e90259", "SHA-512 117 bytes" },
        { &g_abRandom72KB[0],       118, "35f41b486bae7cc4c6b2609dd5314deb8a5151da7e6ab1b0bfb1d7310dae8435152db8b75ebee106a583f16a4b0a3408492e15fba9f9f90c1fda7daae3b3dd71", "SHA-512 118 bytes" },
        { &g_abRandom72KB[0],       119, "dd9cc5aa2ee23e9bc94e80795d19f40adef09384365a805ee0c9b6c946734ca55e8c68c0baea51fb0693efbf04722f5236b9ebc8aed222ea4c47c2d2f20af1ef", "SHA-512 119 bytes" },
        { &g_abRandom72KB[0],       120, "6988719e25df98b8f6b779fe48f61d81f3e7f88ea4c3fed89b28f49b8113686dc5788e66da419c6dcb38a95b2fbd9b29259e7bb8754923913e3f8528ce884f59", "SHA-512 120 bytes" },
        { &g_abRandom72KB[0],       121, "b096a715acd6611dd57d66b547093c937ce73456e18f5d969c2bb1581f33b6ad89142d19850ae29648368adf53d0ff804ed661fb106f81dac514b75a7200d0df", "SHA-512 121 bytes" },
        { &g_abRandom72KB[0],       122, "3691d1ab30f41684f320a6161743b2113e3b452c23867248a8741a383410bb4c83f116404c649cd6983f79a156bf4933d1264d75662d93011a7764f5f962d26a", "SHA-512 122 bytes" },
        { &g_abRandom72KB[0],       123, "2b953227e77ad7415d5363a8f5eed23529b573e20dd6925fd357814daa184f4c4b69ef14a3599c476f589fcfd027c609ccb2fb247acd83f4812d8b72dd0800ff", "SHA-512 123 bytes" },
        { &g_abRandom72KB[0],       124, "639323ed76ab3ec433446d67b41a9f8b6c917627795caf266a58fe396c141988b73490a78d6e053d88ce8c2252051ca711ad2de4fa4fcfca9c26a6a8ea5f16b9", "SHA-512 124 bytes" },
        { &g_abRandom72KB[0],       125, "243b6196df1e1e5c6018644e71471d51e4320303f18749a2e7888430cf43e4f71598c394fbcb2f31b76acf5349233ee9614ab86b455364e54bd2013ee9e1bcbd", "SHA-512 125 bytes" },
        { &g_abRandom72KB[0],       126, "5dc477dcf848e23ba3a66c2ff9bccefdf83bc1134ff7f7a20fe7393269c7987939fbc264535a1ff0d3aba7201ee15a448d5545429a48c681ed5a8859857614f6", "SHA-512 126 bytes" },
        { &g_abRandom72KB[0],       127, "7ea868053923ca7112afb72d6d184ea4fa41191b5a2cfc30c4555ae4bc6223c7c6c834f9f34433947308838abee9d068cd18cd3021ca677141440fc03d5daddd", "SHA-512 127 bytes" },
        { &g_abRandom72KB[0],       128, "a2d7ee08394542488ee7c76954dc027826836161de10795eab31877dc0b56321ca0239a324985a5826a59ef60f70d591e543f56a5fa147d53f85d15ffc7f7791", "SHA-512 128 bytes" },
        { &g_abRandom72KB[0],       129, "e5df5295d00b085665aab5208d17d2c5b152984ca952a2f28599943fe613b38590e24b5552b9614ad38de16197599ac2464ba7a85b66b087b4162dbd8f1038e3", "SHA-512 129 bytes" },
        { &g_abRandom72KB[0],      1024, "213117257eedf07e76d9bd57f7b9b5fad2fbdccd8c9bf60a70e8b2feac5a30ccf83ca9041a07bb15727c81777d94ba75535f29a0bd92471b8899f5cd096e326a", "SHA-512 1024 bytes" },
        { &g_abRandom72KB[0],     73001, "73e458b0479638d5e0b89ed55ca34933fbca66dc2b8ec81490e4b0ee465d7045736a001bc37388e6f73b3acd5655a210092dd5533a88ba1679a6513fe0c70a74", "SHA-512 73001 bytes" },
        { &g_abRandom72KB[0],     73728, "80bd83278c0a26e0f2f952b44ff31057a33e971ea4d6d2f45097e1ff289c9b3c927152ec8ef972929b9b3222abecc3ed64bebc31779c6178b60b91e00a71f542", "SHA-512 73728 bytes" },
        { &g_abRandom72KB[0x20c9],  9991, "d6ac7c68664df2e34dc6be233b33f8dad196348350b70a4c2c5a78eb54d6e297c819771313d798de7552b7a3cb85370aab25087e189f3be8560d49406ebb6280", "SHA-512 8393 bytes @9991" },
    };
    testGeneric("2.16.840.1.101.3.4.2.3", s_abTests, RT_ELEMENTS(s_abTests), "SHA-512", RTDIGESTTYPE_SHA512);
}


#ifndef IPRT_WITHOUT_SHA512T224
/**
 * Tests SHA-512/224
 */
static void testSha512t224(void)
{
    RTTestISub("SHA-512/224");

    /*
     * Some quick direct API tests.
     */
    uint8_t     abHash[RTSHA512T224_HASH_SIZE];
    char        szDigest[RTSHA512T224_DIGEST_LEN + 1];
    const char *pszString;

    pszString = "abc";
    RTSha512t224(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha512t224ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "4634270f707b6a54daae7530460842e20e37ed265ceee9a43e8924aa");

    pszString = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    RTSha512t224(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha512t224ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "23fec5bb94d60b23308192640b0c453335d664734fe40e7268674af9");

    /*
     * Generic API tests.
     */
    static TESTRTDIGEST const s_abTests[] =
    {
        { RT_STR_TUPLE("abc"), "4634270f707b6a54daae7530460842e20e37ed265ceee9a43e8924aa", "SHA-512/224 abc" },
        { RT_STR_TUPLE("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"),
          "23fec5bb94d60b23308192640b0c453335d664734fe40e7268674af9", "SHA-512/256 abcdef..." },
    };
    testGeneric("2.16.840.1.101.3.4.2.5", s_abTests, RT_ELEMENTS(s_abTests), "SHA-512/224", RTDIGESTTYPE_SHA512T224);
}
#endif /* IPRT_WITHOUT_SHA512T224 */

#ifndef IPRT_WITHOUT_SHA512T256
/**
 * Tests SHA-512/256
 */
static void testSha512t256(void)
{
    RTTestISub("SHA-512/256");

    /*
     * Some quick direct API tests.
     */
    uint8_t     abHash[RTSHA512T256_HASH_SIZE];
    char        szDigest[RTSHA512T256_DIGEST_LEN + 1];
    const char *pszString;

    pszString = "abc";
    RTSha512t256(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha512t256ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "53048e2681941ef99b2e29b76b4c7dabe4c2d0c634fc6d46e0e2f13107e7af23");

    pszString = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    RTSha512t256(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha512t256ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "3928e184fb8690f840da3988121d31be65cb9d3ef83ee6146feac861e19b563a");

    /*
     * Generic API tests.
     */
    static TESTRTDIGEST const s_abTests[] =
    {
        { RT_STR_TUPLE("abc"), "53048e2681941ef99b2e29b76b4c7dabe4c2d0c634fc6d46e0e2f13107e7af23", "SHA-512/256 abc" },
        { RT_STR_TUPLE("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"),
          "3928e184fb8690f840da3988121d31be65cb9d3ef83ee6146feac861e19b563a", "SHA-512/256 abcdef..." },
    };
    testGeneric("2.16.840.1.101.3.4.2.6", s_abTests, RT_ELEMENTS(s_abTests), "SHA-512/256", RTDIGESTTYPE_SHA512T256);
}
#endif /* !IPRT_WITHOUT_SHA512T256 */

/**
 * Tests SHA-384
 */
static void testSha384(void)
{
    RTTestISub("SHA-384");

    /*
     * Some quick direct API tests.
     */
    uint8_t     abHash[RTSHA384_HASH_SIZE];
    char        szDigest[RTSHA384_DIGEST_LEN + 1];
    const char *pszString;

    pszString = "abc";
    RTSha384(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha384ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7");

    pszString = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    RTSha384(pszString, strlen(pszString), abHash);
    RTTESTI_CHECK_RC_RETV(RTSha384ToString(abHash, szDigest, sizeof(szDigest)), VINF_SUCCESS);
    CHECK_STRING(szDigest, "09330c33f71147e83d192fc782cd1b4753111b173b3b05d22fa08086e3b0f712fcc7c71a557e2db966c3e9fa91746039");

    /*
     * Generic API tests.
     */
    static TESTRTDIGEST const s_abTests[] =
    {
        { RT_STR_TUPLE("abc"), "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7", "SHA-384 abc" },
        { RT_STR_TUPLE("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"),
          "09330c33f71147e83d192fc782cd1b4753111b173b3b05d22fa08086e3b0f712fcc7c71a557e2db966c3e9fa91746039", "SHA-384 abcdef..." },
    };
    testGeneric("2.16.840.1.101.3.4.2.2", s_abTests, RT_ELEMENTS(s_abTests), "SHA-384", RTDIGESTTYPE_SHA384);
}


int main()
{
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstRTDigest-2", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    testMd2();
    testMd5();
    testSha1();
    testSha256();
    testSha224();
    testSha512();
    testSha384();
#ifndef IPRT_WITHOUT_SHA512T224
    testSha512t224();
#endif
#ifndef IPRT_WITHOUT_SHA512T256
    testSha512t256();
#endif

    return RTTestSummaryAndDestroy(hTest);
}

