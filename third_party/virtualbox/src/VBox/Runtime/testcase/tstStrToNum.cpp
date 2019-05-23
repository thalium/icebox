/* $Id: tstStrToNum.cpp $ */
/** @file
 * IPRT Testcase - String To Number Conversion.
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

#include <iprt/initterm.h>
#include <iprt/string.h>
#include <iprt/stream.h>
#include <iprt/err.h>

struct TstI64
{
    const char *psz;
    unsigned    uBase;
    int         rc;
    int64_t     Result;
};

struct TstU64
{
    const char *psz;
    unsigned    uBase;
    int         rc;
    uint64_t    Result;
};

struct TstI32
{
    const char *psz;
    unsigned    uBase;
    int         rc;
    int32_t     Result;
};

struct TstU32
{
    const char *psz;
    unsigned    uBase;
    int         rc;
    uint32_t    Result;
};


#define TEST(Test, Type, Fmt, Fun, iTest) \
    do \
    { \
        Type Result; \
        int rc = Fun(Test.psz, NULL, Test.uBase, &Result); \
        if (Result != Test.Result) \
        { \
            RTPrintf("failure: '%s' -> " Fmt " expected " Fmt ". (%s/%u)\n", Test.psz, Result, Test.Result, #Fun, iTest); \
            cErrors++; \
        } \
        else if (rc != Test.rc) \
        { \
            RTPrintf("failure: '%s' -> rc=%Rrc expected %Rrc. (%s/%u)\n", Test.psz, rc, Test.rc, #Fun, iTest); \
            cErrors++; \
        } \
    } while (0)


#define RUN_TESTS(aTests, Type, Fmt, Fun) \
    do \
    { \
        for (unsigned iTest = 0; iTest < RT_ELEMENTS(aTests); iTest++) \
        { \
            TEST(aTests[iTest], Type, Fmt, Fun, iTest); \
        } \
    } while (0)

int main()
{
    RTR3InitExeNoArguments(0);

    int cErrors = 0;
    static const struct TstU64 aTstU64[] =
    {
        { "0",                      0,  VINF_SUCCESS,           0 },
        { "1",                      0,  VINF_SUCCESS,           1 },
        { "-1",                     0,  VWRN_NEGATIVE_UNSIGNED,  ~0ULL },
        { "0x",                     0,  VWRN_TRAILING_CHARS,    0 },
        { "0x1",                    0,  VINF_SUCCESS,           1 },
        { "0x0fffffffffffffff",     0,  VINF_SUCCESS,           0x0fffffffffffffffULL },
        { "0x0ffffffffffffffffffffff",0,  VWRN_NUMBER_TOO_BIG,  0xffffffffffffffffULL },
        { "asdfasdfasdf",           0,  VERR_NO_DIGITS,         0 },
        { "0x111111111",            0,  VINF_SUCCESS,           0x111111111ULL },
        { "4D9702C5CBD9B778",      16,  VINF_SUCCESS,           UINT64_C(0x4D9702C5CBD9B778) },
    };
    RUN_TESTS(aTstU64, uint64_t, "%#llx", RTStrToUInt64Ex);

    static const struct TstI64 aTstI64[] =
    {
        { "0",                      0,  VINF_SUCCESS,           0 },
        { "1",                      0,  VINF_SUCCESS,           1 },
        { "-1",                     0,  VINF_SUCCESS,          -1 },
        { "-1",                    10,  VINF_SUCCESS,          -1 },
        { "-31",                    0,  VINF_SUCCESS,          -31 },
        { "-31",                   10,  VINF_SUCCESS,          -31 },
        { "-32",                    0,  VINF_SUCCESS,          -32 },
        { "-33",                    0,  VINF_SUCCESS,          -33 },
        { "-64",                    0,  VINF_SUCCESS,          -64 },
        { "-127",                   0,  VINF_SUCCESS,          -127 },
        { "-128",                   0,  VINF_SUCCESS,          -128 },
        { "-129",                   0,  VINF_SUCCESS,          -129 },
        { "-254",                   0,  VINF_SUCCESS,          -254 },
        { "-255",                   0,  VINF_SUCCESS,          -255 },
        { "-256",                   0,  VINF_SUCCESS,          -256 },
        { "-257",                   0,  VINF_SUCCESS,          -257 },
        { "-511",                   0,  VINF_SUCCESS,          -511 },
        { "-512",                   0,  VINF_SUCCESS,          -512 },
        { "-513",                   0,  VINF_SUCCESS,          -513 },
        { "-1023",                  0,  VINF_SUCCESS,          -1023 },
        { "-1023",                  0,  VINF_SUCCESS,          -1023 },
        { "-1023",                  0,  VINF_SUCCESS,          -1023},
        { "-1023",                 10,  VINF_SUCCESS,          -1023 },
        { "-4564678",               0,  VINF_SUCCESS,          -4564678 },
        { "-4564678",              10,  VINF_SUCCESS,          -4564678 },
        { "-1234567890123456789",   0,  VINF_SUCCESS,          -1234567890123456789LL },
        { "-1234567890123456789",  10,  VINF_SUCCESS,          -1234567890123456789LL },
        { "0x",                     0,  VWRN_TRAILING_CHARS,    0 },
        { "0x1",                    0,  VINF_SUCCESS,           1 },
        { "0x1",                   10,  VWRN_TRAILING_CHARS,    0 },
        { "0x1",                   16,  VINF_SUCCESS,           1 },
        { "0x0fffffffffffffff",     0,  VINF_SUCCESS,           0x0fffffffffffffffULL },
        { "0x7fffffffffffffff",     0,  VINF_SUCCESS,           0x7fffffffffffffffULL },
        { "0xffffffffffffffff",     0,  VWRN_NUMBER_TOO_BIG,    -1 },
        { "0x01111111111111111111111",0,  VWRN_NUMBER_TOO_BIG,  0x1111111111111111ULL },
        { "0x02222222222222222222222",0,  VWRN_NUMBER_TOO_BIG,  0x2222222222222222ULL },
        { "0x03333333333333333333333",0,  VWRN_NUMBER_TOO_BIG,  0x3333333333333333ULL },
        { "0x04444444444444444444444",0,  VWRN_NUMBER_TOO_BIG,  0x4444444444444444ULL },
        { "0x07777777777777777777777",0,  VWRN_NUMBER_TOO_BIG,  0x7777777777777777ULL },
        { "0x07f7f7f7f7f7f7f7f7f7f7f",0,  VWRN_NUMBER_TOO_BIG,  0x7f7f7f7f7f7f7f7fULL },
        { "0x0ffffffffffffffffffffff",0,  VWRN_NUMBER_TOO_BIG,  (int64_t)0xffffffffffffffffULL },
        { "asdfasdfasdf",           0,  VERR_NO_DIGITS,         0 },
        { "0x111111111",            0,  VINF_SUCCESS,           0x111111111ULL },
    };
    RUN_TESTS(aTstI64, int64_t, "%#lld", RTStrToInt64Ex);



    static const struct TstI32 aTstI32[] =
    {
        { "0",                      0,  VINF_SUCCESS,           0 },
        { "1",                      0,  VINF_SUCCESS,           1 },
        { "-1",                     0,  VINF_SUCCESS,          -1 },
        { "-1",                    10,  VINF_SUCCESS,          -1 },
        { "-31",                    0,  VINF_SUCCESS,          -31 },
        { "-31",                   10,  VINF_SUCCESS,          -31 },
        { "-32",                    0,  VINF_SUCCESS,          -32 },
        { "-33",                    0,  VINF_SUCCESS,          -33 },
        { "-64",                    0,  VINF_SUCCESS,          -64 },
        { "-127",                   0,  VINF_SUCCESS,          -127 },
        { "-128",                   0,  VINF_SUCCESS,          -128 },
        { "-129",                   0,  VINF_SUCCESS,          -129 },
        { "-254",                   0,  VINF_SUCCESS,          -254 },
        { "-255",                   0,  VINF_SUCCESS,          -255 },
        { "-256",                   0,  VINF_SUCCESS,          -256 },
        { "-257",                   0,  VINF_SUCCESS,          -257 },
        { "-511",                   0,  VINF_SUCCESS,          -511 },
        { "-512",                   0,  VINF_SUCCESS,          -512 },
        { "-513",                   0,  VINF_SUCCESS,          -513 },
        { "-1023",                  0,  VINF_SUCCESS,          -1023 },
        { "-1023",                  0,  VINF_SUCCESS,          -1023 },
        { "-1023",                  0,  VINF_SUCCESS,          -1023},
        { "-1023",                 10,  VINF_SUCCESS,          -1023 },
        { "-4564678",               0,  VINF_SUCCESS,          -4564678 },
        { "-4564678",              10,  VINF_SUCCESS,          -4564678 },
        { "4564678",                0,  VINF_SUCCESS,          4564678 },
        { "4564678",               10,  VINF_SUCCESS,          4564678 },
        { "-1234567890123456789",   0,  VWRN_NUMBER_TOO_BIG,   (int32_t)((uint64_t)INT64_C(-1234567890123456789) & UINT32_MAX) },
        { "-1234567890123456789",  10,  VWRN_NUMBER_TOO_BIG,   (int32_t)((uint64_t)INT64_C(-1234567890123456789) & UINT32_MAX) },
        { "1234567890123456789",    0,  VWRN_NUMBER_TOO_BIG,   (int32_t)(INT64_C(1234567890123456789)            & UINT32_MAX) },
        { "1234567890123456789",   10,  VWRN_NUMBER_TOO_BIG,   (int32_t)(INT64_C(1234567890123456789)            & UINT32_MAX) },
        { "0x",                     0,  VWRN_TRAILING_CHARS,    0 },
        { "0x1",                    0,  VINF_SUCCESS,           1 },
        { "0x1",                   10,  VWRN_TRAILING_CHARS,    0 },
        { "0x1",                   16,  VINF_SUCCESS,           1 },
        { "0x7fffffff",             0,  VINF_SUCCESS,           0x7fffffff },
        { "0x80000000",             0,  VWRN_NUMBER_TOO_BIG,    INT32_MIN },
        { "0xffffffff",             0,  VWRN_NUMBER_TOO_BIG,    -1 },
        { "0x0fffffffffffffff",     0,  VWRN_NUMBER_TOO_BIG,    (int32_t)0xffffffff },
        { "0x01111111111111111111111",0,  VWRN_NUMBER_TOO_BIG,  0x11111111 },
        { "0x0ffffffffffffffffffffff",0,  VWRN_NUMBER_TOO_BIG,  (int32_t)0xffffffff },
        { "asdfasdfasdf",           0,  VERR_NO_DIGITS,         0 },
        { "0x1111111",              0,  VINF_SUCCESS,           0x01111111 },
    };
    RUN_TESTS(aTstI32, int32_t, "%#d", RTStrToInt32Ex);

    static const struct TstU32 aTstU32[] =
    {
        { "0",                      0,  VINF_SUCCESS,           0 },
        { "1",                      0,  VINF_SUCCESS,           1 },
        /// @todo { "-1",                     0,  VWRN_NEGATIVE_UNSIGNED, ~0 }, - no longer true. bad idea?
        { "-1",                     0,  VWRN_NUMBER_TOO_BIG,    ~0U },
        { "0x",                     0,  VWRN_TRAILING_CHARS,    0 },
        { "0x1",                    0,  VINF_SUCCESS,           1 },
        { "0x0fffffffffffffff",     0,  VWRN_NUMBER_TOO_BIG,    0xffffffffU },
        { "0x0ffffffffffffffffffffff",0,  VWRN_NUMBER_TOO_BIG,  0xffffffffU },
        { "asdfasdfasdf",           0,  VERR_NO_DIGITS,         0 },
        { "0x1111111",              0,  VINF_SUCCESS,           0x1111111 },
    };
    RUN_TESTS(aTstU32, uint32_t, "%#x", RTStrToUInt32Ex);

    /*
     * Summary.
     */
    if (!cErrors)
        RTPrintf("tstStrToNum: SUCCESS\n");
    else
        RTPrintf("tstStrToNum: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}
