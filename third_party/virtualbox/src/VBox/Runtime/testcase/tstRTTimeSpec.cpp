/* $Id: tstRTTimeSpec.cpp $ */
/** @file
 * IPRT - RTTimeSpec and PRTTIME tests.
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
#if !defined(RT_OS_WINDOWS)
# define RTTIME_INCL_TIMEVAL
# define RTTIME_INCL_TIMESPEC
# include <time.h>
# include <sys/time.h>
#endif
#include <iprt/time.h>

#include <iprt/test.h>
#include <iprt/string.h>


/**
 * Format the time into a string using a static buffer.
 */
char *ToString(PRTTIME pTime)
{
    static char szBuf[128];
    RTStrPrintf(szBuf, sizeof(szBuf), "%04d-%02d-%02dT%02u:%02u:%02u.%09u [YD%u WD%u UO%d F%#x]",
                pTime->i32Year,
                pTime->u8Month,
                pTime->u8MonthDay,
                pTime->u8Hour,
                pTime->u8Minute,
                pTime->u8Second,
                pTime->u32Nanosecond,
                pTime->u16YearDay,
                pTime->u8WeekDay,
                pTime->offUTC,
                pTime->fFlags);
    return szBuf;
}

#define CHECK_NZ(expr) do { if (!(expr)) { RTTestIFailed("at line %d: %#x\n", __LINE__, #expr); return RTTestSummaryAndDestroy(hTest); } } while (0)

#define TEST_NS(ns) do {\
        CHECK_NZ(RTTimeExplode(&T1, RTTimeSpecSetNano(&Ts1, ns))); \
        RTTestIPrintf(RTTESTLVL_ALWAYS, "%RI64 ns - %s\n", ns, ToString(&T1)); \
        CHECK_NZ(RTTimeImplode(&Ts2, &T1)); \
        if (!RTTimeSpecIsEqual(&Ts2, &Ts1)) \
            RTTestIFailed("FAILURE - %RI64 != %RI64, line no. %d\n", \
                          RTTimeSpecGetNano(&Ts2), RTTimeSpecGetNano(&Ts1), __LINE__); \
    } while (0)

#define TEST_SEC(sec) do {\
        CHECK_NZ(RTTimeExplode(&T1, RTTimeSpecSetSeconds(&Ts1, sec))); \
        RTTestIPrintf(RTTESTLVL_ALWAYS, "%RI64 sec - %s\n", sec, ToString(&T1)); \
        CHECK_NZ(RTTimeImplode(&Ts2, &T1)); \
        if (!RTTimeSpecIsEqual(&Ts2, &Ts1)) \
                RTTestIFailed("FAILURE - %RI64 != %RI64, line no. %d\n", \
                              RTTimeSpecGetNano(&Ts2), RTTimeSpecGetNano(&Ts1), __LINE__); \
    } while (0)

#define CHECK_TIME(pTime, _i32Year, _u8Month, _u8MonthDay, _u8Hour, _u8Minute, _u8Second, _u32Nanosecond, _u16YearDay, _u8WeekDay, _offUTC, _fFlags)\
    do { \
        if (    (pTime)->i32Year != (_i32Year) \
            ||  (pTime)->u8Month != (_u8Month) \
            ||  (pTime)->u8WeekDay != (_u8WeekDay) \
            ||  (pTime)->u16YearDay != (_u16YearDay) \
            ||  (pTime)->u8MonthDay != (_u8MonthDay) \
            ||  (pTime)->u8Hour != (_u8Hour) \
            ||  (pTime)->u8Minute != (_u8Minute) \
            ||  (pTime)->u8Second != (_u8Second) \
            ||  (pTime)->u32Nanosecond != (_u32Nanosecond) \
            ||  (pTime)->offUTC != (_offUTC) \
            ||  (pTime)->fFlags != (_fFlags) \
            ) \
        { \
            RTTestIFailed("   %s ; line no %d\n" \
                          "!= %04d-%02d-%02dT%02u-%02u-%02u.%09u [YD%u WD%u UO%d F%#x]\n", \
                          ToString(pTime), __LINE__, (_i32Year), (_u8Month), (_u8MonthDay), (_u8Hour), (_u8Minute), \
                          (_u8Second), (_u32Nanosecond), (_u16YearDay), (_u8WeekDay), (_offUTC), (_fFlags)); \
        } \
        else \
            RTTestIPrintf(RTTESTLVL_ALWAYS, "=> %s\n", ToString(pTime)); \
    } while (0)

#define SET_TIME(pTime, _i32Year, _u8Month, _u8MonthDay, _u8Hour, _u8Minute, _u8Second, _u32Nanosecond, _u16YearDay, _u8WeekDay, _offUTC, _fFlags)\
    do { \
        (pTime)->i32Year = (_i32Year); \
        (pTime)->u8Month = (_u8Month); \
        (pTime)->u8WeekDay = (_u8WeekDay); \
        (pTime)->u16YearDay = (_u16YearDay); \
        (pTime)->u8MonthDay = (_u8MonthDay); \
        (pTime)->u8Hour = (_u8Hour); \
        (pTime)->u8Minute = (_u8Minute); \
        (pTime)->u8Second = (_u8Second); \
        (pTime)->u32Nanosecond = (_u32Nanosecond); \
        (pTime)->offUTC = (_offUTC); \
        (pTime)->fFlags = (_fFlags); \
        RTTestIPrintf(RTTESTLVL_ALWAYS, "   %s\n", ToString(pTime)); \
    } while (0)


int main()
{
    RTTIMESPEC      Now;
    RTTIMESPEC      Ts1;
    RTTIMESPEC      Ts2;
    RTTIME          T1;
    RTTIME          T2;
#ifdef RTTIME_INCL_TIMEVAL
    struct timeval  Tv1;
    struct timeval  Tv2;
    struct timespec Tsp1;
    struct timespec Tsp2;
#endif
    RTTEST          hTest;

    int rc = RTTestInitAndCreate("tstRTTimeSpec", &hTest);
    if (rc)
        return rc;

    /*
     * Simple test with current time.
     */
    RTTestSub(hTest, "Current time (UTC)");
    CHECK_NZ(RTTimeNow(&Now));
    CHECK_NZ(RTTimeExplode(&T1, &Now));
    RTTestIPrintf(RTTESTLVL_ALWAYS, "   %RI64 ns - %s\n", RTTimeSpecGetNano(&Now), ToString(&T1));
    CHECK_NZ(RTTimeImplode(&Ts1, &T1));
    if (!RTTimeSpecIsEqual(&Ts1, &Now))
        RTTestIFailed("%RI64 != %RI64\n", RTTimeSpecGetNano(&Ts1), RTTimeSpecGetNano(&Now));

    /*
     * Simple test with current local time.
     */
    RTTestSub(hTest, "Current time (local)");
    CHECK_NZ(RTTimeLocalNow(&Now));
    CHECK_NZ(RTTimeExplode(&T1, &Now));
    RTTestIPrintf(RTTESTLVL_ALWAYS, "   %RI64 ns - %s\n", RTTimeSpecGetNano(&Now), ToString(&T1));
    CHECK_NZ(RTTimeImplode(&Ts1, &T1));
    if (!RTTimeSpecIsEqual(&Ts1, &Now))
        RTTestIFailed("%RI64 != %RI64\n", RTTimeSpecGetNano(&Ts1), RTTimeSpecGetNano(&Now));

    /*
     * Some simple tests with fixed dates (just checking for smoke).
     */
    RTTestSub(hTest, "Smoke");
    TEST_NS(INT64_C(0));
    CHECK_TIME(&T1, 1970,01,01, 00,00,00,        0,   1, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);
    TEST_NS(INT64_C(86400000000000));
    CHECK_TIME(&T1, 1970,01,02, 00,00,00,        0,   2, 4, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    TEST_NS(INT64_C(1));
    CHECK_TIME(&T1, 1970,01,01, 00,00,00,        1,   1, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);
    TEST_NS(INT64_C(-1));
    CHECK_TIME(&T1, 1969,12,31, 23,59,59,999999999, 365, 2, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    /*
     * Test the limits.
     */
    RTTestSub(hTest, "Extremes");
    TEST_NS(INT64_MAX);
    TEST_NS(INT64_MIN);
    TEST_SEC(1095379198);
    CHECK_TIME(&T1, 2004, 9,16, 23,59,58,        0, 260, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_LEAP_YEAR);
    TEST_SEC(1095379199);
    CHECK_TIME(&T1, 2004, 9,16, 23,59,59,        0, 260, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_LEAP_YEAR);
    TEST_SEC(1095379200);
    CHECK_TIME(&T1, 2004, 9,17, 00,00,00,        0, 261, 4, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_LEAP_YEAR);
    TEST_SEC(1095379201);
    CHECK_TIME(&T1, 2004, 9,17, 00,00,01,        0, 261, 4, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_LEAP_YEAR);


    /*
     * Test normalization (UTC).
     */
    RTTestSub(hTest, "Normalization (UTC)");
    /* simple */
    CHECK_NZ(RTTimeNow(&Now));
    CHECK_NZ(RTTimeExplode(&T1, &Now));
    T2 = T1;
    CHECK_NZ(RTTimeNormalize(&T1));
    if (memcmp(&T1, &T2, sizeof(T1)))
        RTTestIFailed("simple normalization failed\n");
    CHECK_NZ(RTTimeImplode(&Ts1, &T1));
    CHECK_NZ(RTTimeSpecIsEqual(&Ts1, &Now));

    /* a few partial dates. */
    memset(&T1, 0, sizeof(T1));
    SET_TIME(  &T1, 1970,01,01, 00,00,00,        0,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1970,01,01, 00,00,00,        0,   1, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 1970,00,00, 00,00,00,        1,   1, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1970,01,01, 00,00,00,        1,   1, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 2007,12,06, 02,15,23,        1,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 2007,12,06, 02,15,23,        1, 340, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 1968,01,30, 00,19,24,        5,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1968,01,30, 00,19,24,        5,  30, 1, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_LEAP_YEAR);

    SET_TIME(  &T1, 1969,01,31, 00, 9, 2,        7,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1969,01,31, 00, 9, 2,        7,  31, 4, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 1969,03,31, 00, 9, 2,        7,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1969,03,31, 00, 9, 2,        7,  90, 0, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 1969,12,31, 00,00,00,        9,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1969,12,31, 00,00,00,        9, 365, 2, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 1969,12,30, 00,00,00,       30,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1969,12,30, 00,00,00,       30, 364, 1, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 1969,00,00, 00,00,00,       30, 363, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1969,12,29, 00,00,00,       30, 363, 0, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 1969,00,00, 00,00,00,       30, 362, 6, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1969,12,28, 00,00,00,       30, 362, 6, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 1969,12,27, 00,00,00,       30,   0, 5, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1969,12,27, 00,00,00,       30, 361, 5, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 1969,00,00, 00,00,00,       30, 360, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1969,12,26, 00,00,00,       30, 360, 4, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 1969,12,25, 00,00,00,       12,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1969,12,25, 00,00,00,       12, 359, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 1969,12,24, 00,00,00,       16,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1969,12,24, 00,00,00,       16, 358, 2, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    /* outside the year table range */
    SET_TIME(  &T1, 1200,01,30, 00,00,00,        2,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1200,01,30, 00,00,00,        2,  30, 6, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_LEAP_YEAR);

    SET_TIME(  &T1, 2555,11,29, 00,00,00,        2,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 2555,11,29, 00,00,00,        2, 333, 5, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 2555,00,00, 00,00,00,        3, 333, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 2555,11,29, 00,00,00,        3, 333, 5, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    /* time overflow */
    SET_TIME(  &T1, 1969,12,30, 255,255,255, UINT32_MAX, 364, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 1970,01, 9, 19,19,19,294967295,   9, 4, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    /* date overflow */
    SET_TIME(  &T1, 2007,11,36, 02,15,23,        1,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 2007,12,06, 02,15,23,        1, 340, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 2007,10,67, 02,15,23,        1,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 2007,12,06, 02,15,23,        1, 340, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 2007,10,98, 02,15,23,        1,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 2008,01,06, 02,15,23,        1,   6, 6, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_LEAP_YEAR);

    SET_TIME(  &T1, 2006,24,06, 02,15,23,        1,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 2007,12,06, 02,15,23,        1, 340, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    SET_TIME(  &T1, 2003,60,37, 02,15,23,        1,   0, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 2008,01,06, 02,15,23,        1,   6, 6, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_LEAP_YEAR);

    SET_TIME(  &T1, 2003,00,00, 02,15,23,        1,1801, 0, 0, 0);
    CHECK_NZ(RTTimeNormalize(&T1));
    CHECK_TIME(&T1, 2007,12,06, 02,15,23,        1, 340, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);

    /*
     * Conversions.
     */
#define CHECK_NSEC(Ts1, T2) \
    do { \
        RTTIMESPEC TsTmp; \
        RTTESTI_CHECK_MSG( RTTimeSpecGetNano(&(Ts1)) == RTTimeSpecGetNano(RTTimeImplode(&TsTmp, &(T2))), \
                          ("line %d: %RI64, %RI64\n", __LINE__, \
                           RTTimeSpecGetNano(&(Ts1)),   RTTimeSpecGetNano(RTTimeImplode(&TsTmp, &(T2)))) ); \
    } while (0)
    RTTestSub(hTest, "Conversions, positive");
    SET_TIME(&T1, 1980,01,01, 00,00,00,        0,   1, 1, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_LEAP_YEAR);
    RTTESTI_CHECK(RTTimeSpecSetDosSeconds(&Ts2,    0) == &Ts2);
    RTTESTI_CHECK(RTTimeSpecGetDosSeconds(&Ts2) == 0);
    CHECK_NSEC(Ts2, T1);

    SET_TIME(&T1, 1980,01,01, 00,00,00,        0,   1, 1, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_LEAP_YEAR);
    RTTESTI_CHECK(RTTimeSpecSetNtTime(&Ts2,    INT64_C(119600064000000000)) == &Ts2);
    RTTESTI_CHECK(RTTimeSpecGetNtTime(&Ts2) == INT64_C(119600064000000000));
    CHECK_NSEC(Ts2, T1);

    SET_TIME(&T1, 1970,01,01, 00,00,01,        0,   1, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);
    RTTESTI_CHECK(RTTimeSpecSetSeconds(&Ts2,    1) == &Ts2);
    RTTESTI_CHECK(RTTimeSpecGetSeconds(&Ts2) == 1);
    CHECK_NSEC(Ts2, T1);

    SET_TIME(&T1, 1970,01,01, 00,00,01,        0,   1, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);
    RTTESTI_CHECK(RTTimeSpecSetMilli(&Ts2,    1000) == &Ts2);
    RTTESTI_CHECK(RTTimeSpecGetMilli(&Ts2) == 1000);
    CHECK_NSEC(Ts2, T1);

    SET_TIME(&T1, 1970,01,01, 00,00,01,        0,   1, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);
    RTTESTI_CHECK(RTTimeSpecSetMicro(&Ts2,    1000000) == &Ts2);
    RTTESTI_CHECK(RTTimeSpecGetMicro(&Ts2) == 1000000);
    CHECK_NSEC(Ts2, T1);

    SET_TIME(&T1, 1970,01,01, 00,00,01,        0,   1, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);
    RTTESTI_CHECK(RTTimeSpecSetNano(&Ts2,    1000000000) == &Ts2);
    RTTESTI_CHECK(RTTimeSpecGetNano(&Ts2) == 1000000000);
    CHECK_NSEC(Ts2, T1);

#ifdef RTTIME_INCL_TIMEVAL
    SET_TIME(&T1, 1970,01,01, 00,00,01,     5000,   1, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);
    Tv1.tv_sec  = 1;
    Tv1.tv_usec = 5;
    RTTESTI_CHECK(RTTimeSpecSetTimeval(&Ts2, &Tv1) == &Ts2);
    RTTESTI_CHECK(RTTimeSpecGetMicro(&Ts2) == 1000005);
    CHECK_NSEC(Ts2, T1);
    RTTESTI_CHECK(RTTimeSpecGetTimeval(&Ts2, &Tv2) == &Tv2);
    RTTESTI_CHECK(Tv1.tv_sec == Tv2.tv_sec); RTTESTI_CHECK(Tv1.tv_usec == Tv2.tv_usec);
#endif

#ifdef RTTIME_INCL_TIMESPEC
    SET_TIME(&T1, 1970,01,01, 00,00,01,        5,   1, 3, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);
    Tsp1.tv_sec  = 1;
    Tsp1.tv_nsec = 5;
    RTTESTI_CHECK(RTTimeSpecSetTimespec(&Ts2, &Tsp1) == &Ts2);
    RTTESTI_CHECK(RTTimeSpecGetNano(&Ts2) == 1000000005);
    CHECK_NSEC(Ts2, T1);
    RTTESTI_CHECK(RTTimeSpecGetTimespec(&Ts2, &Tsp2) == &Tsp2);
    RTTESTI_CHECK(Tsp1.tv_sec == Tsp2.tv_sec); RTTESTI_CHECK(Tsp1.tv_nsec == Tsp2.tv_nsec);
#endif


    RTTestSub(hTest, "Conversions, negative");

#ifdef RTTIME_INCL_TIMEVAL
    SET_TIME(&T1, 1969,12,31, 23,59,58,999995000, 365, 2, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);
    Tv1.tv_sec  = -2;
    Tv1.tv_usec = 999995;
    RTTESTI_CHECK(RTTimeSpecSetTimeval(&Ts2, &Tv1) == &Ts2);
    RTTESTI_CHECK_MSG(RTTimeSpecGetMicro(&Ts2) == -1000005, ("%RI64\n", RTTimeSpecGetMicro(&Ts2)));
    CHECK_NSEC(Ts2, T1);
    RTTESTI_CHECK(RTTimeSpecGetTimeval(&Ts2, &Tv2) == &Tv2);
    RTTESTI_CHECK(Tv1.tv_sec == Tv2.tv_sec); RTTESTI_CHECK(Tv1.tv_usec == Tv2.tv_usec);
#endif

#ifdef RTTIME_INCL_TIMESPEC
    SET_TIME(&T1, 1969,12,31, 23,59,58,999999995, 365, 2, 0, RTTIME_FLAGS_TYPE_UTC | RTTIME_FLAGS_COMMON_YEAR);
    Tsp1.tv_sec  = -2;
    Tsp1.tv_nsec = 999999995;
    RTTESTI_CHECK(RTTimeSpecSetTimespec(&Ts2, &Tsp1) == &Ts2);
    RTTESTI_CHECK_MSG(RTTimeSpecGetNano(&Ts2) == -1000000005, ("%RI64\n", RTTimeSpecGetMicro(&Ts2)));
    CHECK_NSEC(Ts2, T1);
    RTTESTI_CHECK(RTTimeSpecGetTimespec(&Ts2, &Tsp2) == &Tsp2);
    RTTESTI_CHECK(Tsp1.tv_sec == Tsp2.tv_sec); RTTESTI_CHECK(Tsp1.tv_nsec == Tsp2.tv_nsec);
#endif

    /*
     * Summary
     */
    return RTTestSummaryAndDestroy(hTest);
}

