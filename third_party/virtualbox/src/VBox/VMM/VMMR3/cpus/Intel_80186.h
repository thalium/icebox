/* $Id: Intel_80186.h $ */
/** @file
 * CPU database entry "Intel 80186".
 * Handcrafted.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_CPUDB_Intel_80186
#define VBOX_CPUDB_Intel_80186

#ifndef CPUM_DB_STANDALONE
/**
 * Fake CPUID leaves for Intel(R) 80186.
 *
 * We fake these to keep the CPUM ignorant of CPUs wihtout CPUID leaves
 * and avoid having to seed CPUM::GuestFeatures filling with bits from the
 * CPUMDBENTRY.
 */
static CPUMCPUIDLEAF const g_aCpuIdLeaves_Intel_80186[] =
{
    { 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x756e6547, 0x6c65746e, 0x49656e69, 0 },
    { 0x00000001, 0x00000000, 0x00000000, 0x00000100, 0x00000100, 0x00000000, 0x00000000, 0 },
    { 0x80000000, 0x00000000, 0x00000000, 0x80000008, 0x00000000, 0x00000000, 0x00000000, 0 },
    { 0x80000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0 },
    { 0x80000002, 0x00000000, 0x00000000, 0x65746e49, 0x2952286c, 0x31303820, 0x20203638, 0 },
    { 0x80000003, 0x00000000, 0x00000000, 0x20202020, 0x20202020, 0x20202020, 0x20202020, 0 },
    { 0x80000004, 0x00000000, 0x00000000, 0x20202020, 0x20202020, 0x20202020, 0x20202020, 0 },
    { 0x80000005, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0 },
    { 0x80000006, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0 },
    { 0x80000007, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0 },
    { 0x80000008, 0x00000000, 0x00000000, 0x00001414, 0x00000000, 0x00000000, 0x00000000, 0 },
};
#endif /* !CPUM_DB_STANDALONE */

/**
 * Database entry for Intel(R) 80186.
 */
static CPUMDBENTRY const g_Entry_Intel_80186 =
{
    /*.pszName          = */ "Intel 80186",
    /*.pszFullName      = */ "Intel(R) 80186",
    /*.enmVendor        = */ CPUMCPUVENDOR_INTEL,
    /*.uFamily          = */ 2,
    /*.uModel           = */ 0,
    /*.uStepping        = */ 0,
    /*.enmMicroarch     = */ kCpumMicroarch_Intel_80186,
    /*.uScalableBusFreq = */ CPUM_SBUSFREQ_UNKNOWN,
    /*.fFlags           = */ CPUDB_F_EXECUTE_ALL_IN_IEM,
    /*.cMaxPhysAddrWidth= */ 20,
    /*.fMxCsrMask       = */ 0,
    /*.paCpuIdLeaves    = */ NULL_ALONE(g_aCpuIdLeaves_Intel_80186),
    /*.cCpuIdLeaves     = */ ZERO_ALONE(RT_ELEMENTS(g_aCpuIdLeaves_Intel_80186)),
    /*.enmUnknownCpuId  = */ CPUMUNKNOWNCPUID_DEFAULTS,
    /*.DefUnknownCpuId  = */ { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
    /*.fMsrMask         = */ 0,
    /*.cMsrRanges       = */ 0,
    /*.paMsrRanges      = */ NULL,
};

#endif /* !VBOX_CPUDB_Intel_80186 */

