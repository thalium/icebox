/* $Id: tstRTInlineAsm.cpp $ */
/** @file
 * IPRT Testcase - inline assembly.
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
#include <iprt/asm.h>
#include <iprt/asm-math.h>

/* See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=44018. Only gcc version 4.4
 * is affected. No harm for the VBox code: If the cpuid code compiles, it works
 * fine. */
#if defined(__GNUC__) && defined(RT_ARCH_X86) && defined(__PIC__)
# if __GNUC__ == 4 && __GNUC_MINOR__ == 4
#  define GCC44_32BIT_PIC
# endif
#endif

#if !defined(GCC44_32BIT_PIC) && (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86))
# include <iprt/asm-amd64-x86.h>
# include <iprt/x86.h>
#else
# include <iprt/time.h>
#endif
#include <iprt/rand.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/param.h>
#include <iprt/thread.h>
#include <iprt/test.h>
#include <iprt/time.h>



/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define CHECKVAL(val, expect, fmt) \
    do \
    { \
        if ((val) != (expect)) \
        { \
            RTTestFailed(g_hTest, "%s, %d: " #val ": expected " fmt " got " fmt "\n", __FUNCTION__, __LINE__, (expect), (val)); \
        } \
    } while (0)

#define CHECKOP(op, expect, fmt, type) \
    do \
    { \
        type val = op; \
        if (val != (type)(expect)) \
        { \
            RTTestFailed(g_hTest, "%s, %d: " #op ": expected " fmt " got " fmt "\n", __FUNCTION__, __LINE__, (type)(expect), val); \
        } \
    } while (0)

/**
 * Calls a worker function with different worker variable storage types.
 */
#define DO_SIMPLE_TEST(name, type) \
    do \
    { \
        RTTestISub(#name); \
        type StackVar; \
        tst ## name ## Worker(&StackVar); \
        \
        type *pVar = (type *)RTTestGuardedAllocHead(g_hTest, sizeof(type)); \
        RTTEST_CHECK_BREAK(g_hTest, pVar); \
        tst ## name ## Worker(pVar); \
        RTTestGuardedFree(g_hTest, pVar); \
        \
        pVar = (type *)RTTestGuardedAllocTail(g_hTest, sizeof(type)); \
        RTTEST_CHECK_BREAK(g_hTest, pVar); \
        tst ## name ## Worker(pVar); \
        RTTestGuardedFree(g_hTest, pVar); \
    } while (0)


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** The test instance. */
static RTTEST g_hTest;



#if !defined(GCC44_32BIT_PIC) && (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86))

const char *getCacheAss(unsigned u)
{
    if (u == 0)
        return "res0  ";
    if (u == 1)
        return "direct";
    if (u >= 256)
        return "???";

    char *pszRet;
    RTStrAPrintf(&pszRet, "%d way", u);     /* intentional leak! */
    return pszRet;
}


const char *getL2CacheAss(unsigned u)
{
    switch (u)
    {
        case 0:  return "off   ";
        case 1:  return "direct";
        case 2:  return "2 way ";
        case 3:  return "res3  ";
        case 4:  return "4 way ";
        case 5:  return "res5  ";
        case 6:  return "8 way ";
        case 7:  return "res7  ";
        case 8:  return "16 way";
        case 9:  return "res9  ";
        case 10: return "res10 ";
        case 11: return "res11 ";
        case 12: return "res12 ";
        case 13: return "res13 ";
        case 14: return "res14 ";
        case 15: return "fully ";
        default:
            return "????";
    }
}


/**
 * Test and dump all possible info from the CPUID instruction.
 *
 * @remark  Bits shared with the libc cpuid.c program. This all written by me, so no worries.
 * @todo transform the dumping into a generic runtime function. We'll need it for logging!
 */
void tstASMCpuId(void)
{
    RTTestISub("ASMCpuId");

    unsigned    iBit;
    struct
    {
        uint32_t    uEBX, uEAX, uEDX, uECX;
    } s;
    if (!ASMHasCpuId())
    {
        RTTestIPrintf(RTTESTLVL_ALWAYS, "warning! CPU doesn't support CPUID\n");
        return;
    }

    /*
     * Try the 0 function and use that for checking the ASMCpuId_* variants.
     */
    ASMCpuId(0, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);

    uint32_t u32;

    u32 = ASMCpuId_EAX(0);
    CHECKVAL(u32, s.uEAX, "%x");
    u32 = ASMCpuId_EBX(0);
    CHECKVAL(u32, s.uEBX, "%x");
    u32 = ASMCpuId_ECX(0);
    CHECKVAL(u32, s.uECX, "%x");
    u32 = ASMCpuId_EDX(0);
    CHECKVAL(u32, s.uEDX, "%x");

    uint32_t uECX2 = s.uECX - 1;
    uint32_t uEDX2 = s.uEDX - 1;
    ASMCpuId_ECX_EDX(0, &uECX2, &uEDX2);
    CHECKVAL(uECX2, s.uECX, "%x");
    CHECKVAL(uEDX2, s.uEDX, "%x");

    uint32_t uEAX2 = s.uEAX - 1;
    uint32_t uEBX2 = s.uEBX - 1;
    uECX2 = s.uECX - 1;
    uEDX2 = s.uEDX - 1;
    ASMCpuIdExSlow(0, 0, 0, 0, &uEAX2, &uEBX2, &uECX2, &uEDX2);
    CHECKVAL(uEAX2, s.uEAX, "%x");
    CHECKVAL(uEBX2, s.uEBX, "%x");
    CHECKVAL(uECX2, s.uECX, "%x");
    CHECKVAL(uEDX2, s.uEDX, "%x");

    /*
     * Done testing, dump the information.
     */
    RTTestIPrintf(RTTESTLVL_ALWAYS, "CPUID Dump\n");
    ASMCpuId(0, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
    const uint32_t cFunctions = s.uEAX;

    /* raw dump */
    RTTestIPrintf(RTTESTLVL_ALWAYS,
                  "\n"
                  "         RAW Standard CPUIDs\n"
                  "Function  eax      ebx      ecx      edx\n");
    for (unsigned iStd = 0; iStd <= cFunctions + 3; iStd++)
    {
        ASMCpuId_Idx_ECX(iStd, 0, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
        RTTestIPrintf(RTTESTLVL_ALWAYS, "%08x  %08x %08x %08x %08x%s\n",
                      iStd, s.uEAX, s.uEBX, s.uECX, s.uEDX, iStd <= cFunctions ? "" : "*");

        /* Some leafs output depend on the initial value of ECX.
         * The same seems to apply to invalid standard functions */
        if (iStd > cFunctions)
            continue;
        if (   iStd != 0x04 /* Deterministic Cache Parameters Leaf */
            && iStd != 0x07 /* Structured Extended Feature Flags */
            && iStd != 0x0b /* Extended Topology Enumeration Leafs */
            && iStd != 0x0d /* Extended State Enumeration Leafs */
            && iStd != 0x14 /* Trace Enumeration Leafs */)
        {
            u32 = ASMCpuId_EAX(iStd);
            CHECKVAL(u32, s.uEAX, "%x");

            uint32_t u32EbxMask = UINT32_MAX;
            if (iStd == 1)
                u32EbxMask = UINT32_C(0x00ffffff); /* Omit the local apic ID in case we're rescheduled. */
            u32 = ASMCpuId_EBX(iStd);
            CHECKVAL(u32 & u32EbxMask, s.uEBX & u32EbxMask, "%x");

            u32 = ASMCpuId_ECX(iStd);
            CHECKVAL(u32, s.uECX, "%x");
            u32 = ASMCpuId_EDX(iStd);
            CHECKVAL(u32, s.uEDX, "%x");

            uECX2 = s.uECX - 1;
            uEDX2 = s.uEDX - 1;
            ASMCpuId_ECX_EDX(iStd, &uECX2, &uEDX2);
            CHECKVAL(uECX2, s.uECX, "%x");
            CHECKVAL(uEDX2, s.uEDX, "%x");
        }

        if (iStd == 0x04)
            for (uint32_t uECX = 1; s.uEAX & 0x1f; uECX++)
            {
                ASMCpuId_Idx_ECX(iStd, uECX, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
                RTTestIPrintf(RTTESTLVL_ALWAYS, "    [%02x]  %08x %08x %08x %08x\n", uECX, s.uEAX, s.uEBX, s.uECX, s.uEDX);
                RTTESTI_CHECK_BREAK(uECX < 128);
            }
        else if (iStd == 0x07)
        {
            uint32_t uMax = s.uEAX;
            for (uint32_t uECX = 1; uECX < uMax; uECX++)
            {
                ASMCpuId_Idx_ECX(iStd, uECX, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
                RTTestIPrintf(RTTESTLVL_ALWAYS, "    [%02x]  %08x %08x %08x %08x\n", uECX, s.uEAX, s.uEBX, s.uECX, s.uEDX);
                RTTESTI_CHECK_BREAK(uECX < 128);
            }
        }
        else if (iStd == 0x0b)
            for (uint32_t uECX = 1; (s.uEAX & 0x1f) && (s.uEBX & 0xffff); uECX++)
            {
                ASMCpuId_Idx_ECX(iStd, uECX, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
                RTTestIPrintf(RTTESTLVL_ALWAYS, "    [%02x]  %08x %08x %08x %08x\n", uECX, s.uEAX, s.uEBX, s.uECX, s.uEDX);
                RTTESTI_CHECK_BREAK(uECX < 128);
            }
        else if (iStd == 0x0d)
            for (uint32_t uECX = 1; s.uEAX != 0 || s.uEBX != 0 || s.uECX != 0 || s.uEDX != 0; uECX++)
            {
                ASMCpuId_Idx_ECX(iStd, uECX, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
                RTTestIPrintf(RTTESTLVL_ALWAYS, "    [%02x]  %08x %08x %08x %08x\n", uECX, s.uEAX, s.uEBX, s.uECX, s.uEDX);
                RTTESTI_CHECK_BREAK(uECX < 128);
            }
    }

    /*
     * Understandable output
     */
    ASMCpuId(0, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
    RTTestIPrintf(RTTESTLVL_ALWAYS,
                  "Name:                            %.04s%.04s%.04s\n"
                  "Support:                         0-%u\n",
                  &s.uEBX, &s.uEDX, &s.uECX, s.uEAX);
    bool const fIntel = ASMIsIntelCpuEx(s.uEBX, s.uECX, s.uEDX);

    /*
     * Get Features.
     */
    if (cFunctions >= 1)
    {
        static const char * const s_apszTypes[4] = { "primary", "overdrive", "MP", "reserved" };
        ASMCpuId(1, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
        RTTestIPrintf(RTTESTLVL_ALWAYS,
                      "Family:                          %#x \tExtended: %#x \tEffective: %#x\n"
                      "Model:                           %#x \tExtended: %#x \tEffective: %#x\n"
                      "Stepping:                        %d\n"
                      "Type:                            %d (%s)\n"
                      "APIC ID:                         %#04x\n"
                      "Logical CPUs:                    %d\n"
                      "CLFLUSH Size:                    %d\n"
                      "Brand ID:                        %#04x\n",
                      (s.uEAX >> 8) & 0xf, (s.uEAX >> 20) & 0x7f, ASMGetCpuFamily(s.uEAX),
                      (s.uEAX >> 4) & 0xf, (s.uEAX >> 16) & 0x0f, ASMGetCpuModel(s.uEAX, fIntel),
                      ASMGetCpuStepping(s.uEAX),
                      (s.uEAX >> 12) & 0x3, s_apszTypes[(s.uEAX >> 12) & 0x3],
                      (s.uEBX >> 24) & 0xff,
                      (s.uEBX >> 16) & 0xff,
                      (s.uEBX >>  8) & 0xff,
                      (s.uEBX >>  0) & 0xff);

        RTTestIPrintf(RTTESTLVL_ALWAYS, "Features EDX:                   ");
        if (s.uEDX & RT_BIT(0))   RTTestIPrintf(RTTESTLVL_ALWAYS, " FPU");
        if (s.uEDX & RT_BIT(1))   RTTestIPrintf(RTTESTLVL_ALWAYS, " VME");
        if (s.uEDX & RT_BIT(2))   RTTestIPrintf(RTTESTLVL_ALWAYS, " DE");
        if (s.uEDX & RT_BIT(3))   RTTestIPrintf(RTTESTLVL_ALWAYS, " PSE");
        if (s.uEDX & RT_BIT(4))   RTTestIPrintf(RTTESTLVL_ALWAYS, " TSC");
        if (s.uEDX & RT_BIT(5))   RTTestIPrintf(RTTESTLVL_ALWAYS, " MSR");
        if (s.uEDX & RT_BIT(6))   RTTestIPrintf(RTTESTLVL_ALWAYS, " PAE");
        if (s.uEDX & RT_BIT(7))   RTTestIPrintf(RTTESTLVL_ALWAYS, " MCE");
        if (s.uEDX & RT_BIT(8))   RTTestIPrintf(RTTESTLVL_ALWAYS, " CX8");
        if (s.uEDX & RT_BIT(9))   RTTestIPrintf(RTTESTLVL_ALWAYS, " APIC");
        if (s.uEDX & RT_BIT(10))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 10");
        if (s.uEDX & RT_BIT(11))  RTTestIPrintf(RTTESTLVL_ALWAYS, " SEP");
        if (s.uEDX & RT_BIT(12))  RTTestIPrintf(RTTESTLVL_ALWAYS, " MTRR");
        if (s.uEDX & RT_BIT(13))  RTTestIPrintf(RTTESTLVL_ALWAYS, " PGE");
        if (s.uEDX & RT_BIT(14))  RTTestIPrintf(RTTESTLVL_ALWAYS, " MCA");
        if (s.uEDX & RT_BIT(15))  RTTestIPrintf(RTTESTLVL_ALWAYS, " CMOV");
        if (s.uEDX & RT_BIT(16))  RTTestIPrintf(RTTESTLVL_ALWAYS, " PAT");
        if (s.uEDX & RT_BIT(17))  RTTestIPrintf(RTTESTLVL_ALWAYS, " PSE36");
        if (s.uEDX & RT_BIT(18))  RTTestIPrintf(RTTESTLVL_ALWAYS, " PSN");
        if (s.uEDX & RT_BIT(19))  RTTestIPrintf(RTTESTLVL_ALWAYS, " CLFSH");
        if (s.uEDX & RT_BIT(20))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 20");
        if (s.uEDX & RT_BIT(21))  RTTestIPrintf(RTTESTLVL_ALWAYS, " DS");
        if (s.uEDX & RT_BIT(22))  RTTestIPrintf(RTTESTLVL_ALWAYS, " ACPI");
        if (s.uEDX & RT_BIT(23))  RTTestIPrintf(RTTESTLVL_ALWAYS, " MMX");
        if (s.uEDX & RT_BIT(24))  RTTestIPrintf(RTTESTLVL_ALWAYS, " FXSR");
        if (s.uEDX & RT_BIT(25))  RTTestIPrintf(RTTESTLVL_ALWAYS, " SSE");
        if (s.uEDX & RT_BIT(26))  RTTestIPrintf(RTTESTLVL_ALWAYS, " SSE2");
        if (s.uEDX & RT_BIT(27))  RTTestIPrintf(RTTESTLVL_ALWAYS, " SS");
        if (s.uEDX & RT_BIT(28))  RTTestIPrintf(RTTESTLVL_ALWAYS, " HTT");
        if (s.uEDX & RT_BIT(29))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 29");
        if (s.uEDX & RT_BIT(30))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 30");
        if (s.uEDX & RT_BIT(31))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 31");
        RTTestIPrintf(RTTESTLVL_ALWAYS, "\n");

        /** @todo check intel docs. */
        RTTestIPrintf(RTTESTLVL_ALWAYS, "Features ECX:                   ");
        if (s.uECX & RT_BIT(0))   RTTestIPrintf(RTTESTLVL_ALWAYS, " SSE3");
        for (iBit = 1; iBit < 13; iBit++)
            if (s.uECX & RT_BIT(iBit))
                RTTestIPrintf(RTTESTLVL_ALWAYS, " %d", iBit);
        if (s.uECX & RT_BIT(13))  RTTestIPrintf(RTTESTLVL_ALWAYS, " CX16");
        for (iBit = 14; iBit < 32; iBit++)
            if (s.uECX & RT_BIT(iBit))
                RTTestIPrintf(RTTESTLVL_ALWAYS, " %d", iBit);
        RTTestIPrintf(RTTESTLVL_ALWAYS, "\n");
    }

    /*
     * Extended.
     * Implemented after AMD specs.
     */
    /** @todo check out the intel specs. */
    ASMCpuId(0x80000000, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
    if (!s.uEAX && !s.uEBX && !s.uECX && !s.uEDX)
    {
        RTTestIPrintf(RTTESTLVL_ALWAYS, "No extended CPUID info? Check the manual on how to detect this...\n");
        return;
    }
    const uint32_t cExtFunctions = s.uEAX | 0x80000000;

    /* raw dump */
    RTTestIPrintf(RTTESTLVL_ALWAYS,
                  "\n"
                  "         RAW Extended CPUIDs\n"
                  "Function  eax      ebx      ecx      edx\n");
    for (unsigned iExt = 0x80000000; iExt <= cExtFunctions + 3; iExt++)
    {
        ASMCpuId(iExt, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
        RTTestIPrintf(RTTESTLVL_ALWAYS, "%08x  %08x %08x %08x %08x%s\n",
                      iExt, s.uEAX, s.uEBX, s.uECX, s.uEDX, iExt <= cExtFunctions ? "" : "*");

        if (iExt > cExtFunctions)
            continue;   /* Invalid extended functions seems change the value if ECX changes */
        if (iExt == 0x8000001d)
            continue;   /* Takes cache level in ecx. */

        u32 = ASMCpuId_EAX(iExt);
        CHECKVAL(u32, s.uEAX, "%x");
        u32 = ASMCpuId_EBX(iExt);
        CHECKVAL(u32, s.uEBX, "%x");
        u32 = ASMCpuId_ECX(iExt);
        CHECKVAL(u32, s.uECX, "%x");
        u32 = ASMCpuId_EDX(iExt);
        CHECKVAL(u32, s.uEDX, "%x");

        uECX2 = s.uECX - 1;
        uEDX2 = s.uEDX - 1;
        ASMCpuId_ECX_EDX(iExt, &uECX2, &uEDX2);
        CHECKVAL(uECX2, s.uECX, "%x");
        CHECKVAL(uEDX2, s.uEDX, "%x");
    }

    /*
     * Understandable output
     */
    ASMCpuId(0x80000000, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
    RTTestIPrintf(RTTESTLVL_ALWAYS,
                  "Ext Name:                        %.4s%.4s%.4s\n"
                  "Ext Supports:                    0x80000000-%#010x\n",
                  &s.uEBX, &s.uEDX, &s.uECX, s.uEAX);

    if (cExtFunctions >= 0x80000001)
    {
        ASMCpuId(0x80000001, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
        RTTestIPrintf(RTTESTLVL_ALWAYS,
                      "Family:                          %#x \tExtended: %#x \tEffective: %#x\n"
                      "Model:                           %#x \tExtended: %#x \tEffective: %#x\n"
                      "Stepping:                        %d\n"
                      "Brand ID:                        %#05x\n",
                      (s.uEAX >> 8) & 0xf, (s.uEAX >> 20) & 0x7f, ASMGetCpuFamily(s.uEAX),
                      (s.uEAX >> 4) & 0xf, (s.uEAX >> 16) & 0x0f, ASMGetCpuModel(s.uEAX, fIntel),
                      ASMGetCpuStepping(s.uEAX),
                      s.uEBX & 0xfff);

        RTTestIPrintf(RTTESTLVL_ALWAYS, "Features EDX:                   ");
        if (s.uEDX & RT_BIT(0))   RTTestIPrintf(RTTESTLVL_ALWAYS, " FPU");
        if (s.uEDX & RT_BIT(1))   RTTestIPrintf(RTTESTLVL_ALWAYS, " VME");
        if (s.uEDX & RT_BIT(2))   RTTestIPrintf(RTTESTLVL_ALWAYS, " DE");
        if (s.uEDX & RT_BIT(3))   RTTestIPrintf(RTTESTLVL_ALWAYS, " PSE");
        if (s.uEDX & RT_BIT(4))   RTTestIPrintf(RTTESTLVL_ALWAYS, " TSC");
        if (s.uEDX & RT_BIT(5))   RTTestIPrintf(RTTESTLVL_ALWAYS, " MSR");
        if (s.uEDX & RT_BIT(6))   RTTestIPrintf(RTTESTLVL_ALWAYS, " PAE");
        if (s.uEDX & RT_BIT(7))   RTTestIPrintf(RTTESTLVL_ALWAYS, " MCE");
        if (s.uEDX & RT_BIT(8))   RTTestIPrintf(RTTESTLVL_ALWAYS, " CMPXCHG8B");
        if (s.uEDX & RT_BIT(9))   RTTestIPrintf(RTTESTLVL_ALWAYS, " APIC");
        if (s.uEDX & RT_BIT(10))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 10");
        if (s.uEDX & RT_BIT(11))  RTTestIPrintf(RTTESTLVL_ALWAYS, " SysCallSysRet");
        if (s.uEDX & RT_BIT(12))  RTTestIPrintf(RTTESTLVL_ALWAYS, " MTRR");
        if (s.uEDX & RT_BIT(13))  RTTestIPrintf(RTTESTLVL_ALWAYS, " PGE");
        if (s.uEDX & RT_BIT(14))  RTTestIPrintf(RTTESTLVL_ALWAYS, " MCA");
        if (s.uEDX & RT_BIT(15))  RTTestIPrintf(RTTESTLVL_ALWAYS, " CMOV");
        if (s.uEDX & RT_BIT(16))  RTTestIPrintf(RTTESTLVL_ALWAYS, " PAT");
        if (s.uEDX & RT_BIT(17))  RTTestIPrintf(RTTESTLVL_ALWAYS, " PSE36");
        if (s.uEDX & RT_BIT(18))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 18");
        if (s.uEDX & RT_BIT(19))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 19");
        if (s.uEDX & RT_BIT(20))  RTTestIPrintf(RTTESTLVL_ALWAYS, " NX");
        if (s.uEDX & RT_BIT(21))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 21");
        if (s.uEDX & RT_BIT(22))  RTTestIPrintf(RTTESTLVL_ALWAYS, " MmxExt");
        if (s.uEDX & RT_BIT(23))  RTTestIPrintf(RTTESTLVL_ALWAYS, " MMX");
        if (s.uEDX & RT_BIT(24))  RTTestIPrintf(RTTESTLVL_ALWAYS, " FXSR");
        if (s.uEDX & RT_BIT(25))  RTTestIPrintf(RTTESTLVL_ALWAYS, " FastFXSR");
        if (s.uEDX & RT_BIT(26))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 26");
        if (s.uEDX & RT_BIT(27))  RTTestIPrintf(RTTESTLVL_ALWAYS, " RDTSCP");
        if (s.uEDX & RT_BIT(28))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 28");
        if (s.uEDX & RT_BIT(29))  RTTestIPrintf(RTTESTLVL_ALWAYS, " LongMode");
        if (s.uEDX & RT_BIT(30))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 3DNowExt");
        if (s.uEDX & RT_BIT(31))  RTTestIPrintf(RTTESTLVL_ALWAYS, " 3DNow");
        RTTestIPrintf(RTTESTLVL_ALWAYS, "\n");

        RTTestIPrintf(RTTESTLVL_ALWAYS, "Features ECX:                   ");
        if (s.uECX & RT_BIT(0))   RTTestIPrintf(RTTESTLVL_ALWAYS, " LahfSahf");
        if (s.uECX & RT_BIT(1))   RTTestIPrintf(RTTESTLVL_ALWAYS, " CmpLegacy");
        if (s.uECX & RT_BIT(2))   RTTestIPrintf(RTTESTLVL_ALWAYS, " SVM");
        if (s.uECX & RT_BIT(3))   RTTestIPrintf(RTTESTLVL_ALWAYS, " 3");
        if (s.uECX & RT_BIT(4))   RTTestIPrintf(RTTESTLVL_ALWAYS, " AltMovCr8");
        for (iBit = 5; iBit < 32; iBit++)
            if (s.uECX & RT_BIT(iBit))
                RTTestIPrintf(RTTESTLVL_ALWAYS, " %d", iBit);
        RTTestIPrintf(RTTESTLVL_ALWAYS, "\n");
    }

     char szString[4*4*3+1] = {0};
     if (cExtFunctions >= 0x80000002)
         ASMCpuId(0x80000002, &szString[0  + 0], &szString[0  + 4], &szString[0  + 8], &szString[0  + 12]);
     if (cExtFunctions >= 0x80000003)
         ASMCpuId(0x80000003, &szString[16 + 0], &szString[16 + 4], &szString[16 + 8], &szString[16 + 12]);
     if (cExtFunctions >= 0x80000004)
         ASMCpuId(0x80000004, &szString[32 + 0], &szString[32 + 4], &szString[32 + 8], &szString[32 + 12]);
     if (cExtFunctions >= 0x80000002)
         RTTestIPrintf(RTTESTLVL_ALWAYS, "Full Name:                       %s\n", szString);

     if (cExtFunctions >= 0x80000005)
     {
         ASMCpuId(0x80000005, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
         RTTestIPrintf(RTTESTLVL_ALWAYS,
                       "TLB 2/4M Instr/Uni:              %s %3d entries\n"
                       "TLB 2/4M Data:                   %s %3d entries\n",
                       getCacheAss((s.uEAX >>  8) & 0xff), (s.uEAX >>  0) & 0xff,
                       getCacheAss((s.uEAX >> 24) & 0xff), (s.uEAX >> 16) & 0xff);
         RTTestIPrintf(RTTESTLVL_ALWAYS,
                       "TLB 4K Instr/Uni:                %s %3d entries\n"
                       "TLB 4K Data:                     %s %3d entries\n",
                       getCacheAss((s.uEBX >>  8) & 0xff), (s.uEBX >>  0) & 0xff,
                       getCacheAss((s.uEBX >> 24) & 0xff), (s.uEBX >> 16) & 0xff);
         RTTestIPrintf(RTTESTLVL_ALWAYS,
                       "L1 Instr Cache Line Size:        %d bytes\n"
                       "L1 Instr Cache Lines Per Tag:    %d\n"
                       "L1 Instr Cache Associativity:    %s\n"
                       "L1 Instr Cache Size:             %d KB\n",
                       (s.uEDX >> 0) & 0xff,
                       (s.uEDX >> 8) & 0xff,
                       getCacheAss((s.uEDX >> 16) & 0xff),
                       (s.uEDX >> 24) & 0xff);
         RTTestIPrintf(RTTESTLVL_ALWAYS,
                       "L1 Data Cache Line Size:         %d bytes\n"
                       "L1 Data Cache Lines Per Tag:     %d\n"
                       "L1 Data Cache Associativity:     %s\n"
                       "L1 Data Cache Size:              %d KB\n",
                       (s.uECX >> 0) & 0xff,
                       (s.uECX >> 8) & 0xff,
                       getCacheAss((s.uECX >> 16) & 0xff),
                       (s.uECX >> 24) & 0xff);
     }

     if (cExtFunctions >= 0x80000006)
     {
         ASMCpuId(0x80000006, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
         RTTestIPrintf(RTTESTLVL_ALWAYS,
                       "L2 TLB 2/4M Instr/Uni:           %s %4d entries\n"
                       "L2 TLB 2/4M Data:                %s %4d entries\n",
                       getL2CacheAss((s.uEAX >> 12) & 0xf),  (s.uEAX >>  0) & 0xfff,
                       getL2CacheAss((s.uEAX >> 28) & 0xf),  (s.uEAX >> 16) & 0xfff);
         RTTestIPrintf(RTTESTLVL_ALWAYS,
                       "L2 TLB 4K Instr/Uni:             %s %4d entries\n"
                       "L2 TLB 4K Data:                  %s %4d entries\n",
                       getL2CacheAss((s.uEBX >> 12) & 0xf),  (s.uEBX >>  0) & 0xfff,
                       getL2CacheAss((s.uEBX >> 28) & 0xf),  (s.uEBX >> 16) & 0xfff);
         RTTestIPrintf(RTTESTLVL_ALWAYS,
                       "L2 Cache Line Size:              %d bytes\n"
                       "L2 Cache Lines Per Tag:          %d\n"
                       "L2 Cache Associativity:          %s\n"
                       "L2 Cache Size:                   %d KB\n",
                       (s.uEDX >> 0) & 0xff,
                       (s.uEDX >> 8) & 0xf,
                       getL2CacheAss((s.uEDX >> 12) & 0xf),
                       (s.uEDX >> 16) & 0xffff);
     }

     if (cExtFunctions >= 0x80000007)
     {
         ASMCpuId(0x80000007, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
         RTTestIPrintf(RTTESTLVL_ALWAYS, "APM Features:                   ");
         if (s.uEDX & RT_BIT(0))   RTTestIPrintf(RTTESTLVL_ALWAYS, " TS");
         if (s.uEDX & RT_BIT(1))   RTTestIPrintf(RTTESTLVL_ALWAYS, " FID");
         if (s.uEDX & RT_BIT(2))   RTTestIPrintf(RTTESTLVL_ALWAYS, " VID");
         if (s.uEDX & RT_BIT(3))   RTTestIPrintf(RTTESTLVL_ALWAYS, " TTP");
         if (s.uEDX & RT_BIT(4))   RTTestIPrintf(RTTESTLVL_ALWAYS, " TM");
         if (s.uEDX & RT_BIT(5))   RTTestIPrintf(RTTESTLVL_ALWAYS, " STC");
         if (s.uEDX & RT_BIT(6))   RTTestIPrintf(RTTESTLVL_ALWAYS, " 6");
         if (s.uEDX & RT_BIT(7))   RTTestIPrintf(RTTESTLVL_ALWAYS, " 7");
         if (s.uEDX & RT_BIT(8))   RTTestIPrintf(RTTESTLVL_ALWAYS, " TscInvariant");
         for (iBit = 9; iBit < 32; iBit++)
             if (s.uEDX & RT_BIT(iBit))
                 RTTestIPrintf(RTTESTLVL_ALWAYS, " %d", iBit);
         RTTestIPrintf(RTTESTLVL_ALWAYS, "\n");
     }

     if (cExtFunctions >= 0x80000008)
     {
         ASMCpuId(0x80000008, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
         RTTestIPrintf(RTTESTLVL_ALWAYS,
                       "Physical Address Width:          %d bits\n"
                       "Virtual Address Width:           %d bits\n"
                       "Guest Physical Address Width:    %d bits\n",
                       (s.uEAX >> 0) & 0xff,
                       (s.uEAX >> 8) & 0xff,
                       (s.uEAX >> 16) & 0xff);
         RTTestIPrintf(RTTESTLVL_ALWAYS,
                       "Physical Core Count:             %d\n",
                       ((s.uECX >> 0) & 0xff) + 1);
         if ((s.uECX >> 12) & 0xf)
             RTTestIPrintf(RTTESTLVL_ALWAYS, "ApicIdCoreIdSize:                %d bits\n", (s.uECX >> 12) & 0xf);
     }

     if (cExtFunctions >= 0x8000000a)
     {
         ASMCpuId(0x8000000a, &s.uEAX, &s.uEBX, &s.uECX, &s.uEDX);
         RTTestIPrintf(RTTESTLVL_ALWAYS,
                       "SVM Revision:                    %d (%#x)\n"
                       "Number of Address Space IDs:     %d (%#x)\n",
                       s.uEAX & 0xff, s.uEAX & 0xff,
                       s.uEBX, s.uEBX);
     }
}

# if 0
static void bruteForceCpuId(void)
{
    RTTestISub("brute force CPUID leafs");
    uint32_t auPrevValues[4] = { 0, 0, 0, 0};
    uint32_t uLeaf = 0;
    do
    {
        uint32_t auValues[4];
        ASMCpuIdExSlow(uLeaf, 0, 0, 0, &auValues[0], &auValues[1], &auValues[2], &auValues[3]);
        if (   (auValues[0] != auPrevValues[0] && auValues[0] != uLeaf)
            || (auValues[1] != auPrevValues[1] && auValues[1] != 0)
            || (auValues[2] != auPrevValues[2] && auValues[2] != 0)
            || (auValues[3] != auPrevValues[3] && auValues[3] != 0)
            || (uLeaf & (UINT32_C(0x08000000) - UINT32_C(1))) == 0)
        {
            RTTestIPrintf(RTTESTLVL_ALWAYS,
                          "%08x: %08x %08x %08x %08x\n", uLeaf,
                          auValues[0], auValues[1], auValues[2], auValues[3]);
        }
        auPrevValues[0] = auValues[0];
        auPrevValues[1] = auValues[1];
        auPrevValues[2] = auValues[2];
        auPrevValues[3] = auValues[3];

        //uint32_t uSubLeaf = 0;
        //do
        //{
        //
        //
        //} while (false);
    } while (uLeaf++ < UINT32_MAX);
}
# endif

#endif /* AMD64 || X86 */

DECLINLINE(void) tstASMAtomicXchgU8Worker(uint8_t volatile *pu8)
{
    *pu8 = 0;
    CHECKOP(ASMAtomicXchgU8(pu8, 1), 0, "%#x", uint8_t);
    CHECKVAL(*pu8, 1, "%#x");

    CHECKOP(ASMAtomicXchgU8(pu8, 0), 1, "%#x", uint8_t);
    CHECKVAL(*pu8, 0, "%#x");

    CHECKOP(ASMAtomicXchgU8(pu8, UINT8_C(0xff)), 0, "%#x", uint8_t);
    CHECKVAL(*pu8, 0xff, "%#x");

    CHECKOP(ASMAtomicXchgU8(pu8, UINT8_C(0x87)), UINT8_C(0xff), "%#x", uint8_t);
    CHECKVAL(*pu8, 0x87, "%#x");
}


static void tstASMAtomicXchgU8(void)
{
    DO_SIMPLE_TEST(ASMAtomicXchgU8, uint8_t);
}


DECLINLINE(void) tstASMAtomicXchgU16Worker(uint16_t volatile *pu16)
{
    *pu16 = 0;

    CHECKOP(ASMAtomicXchgU16(pu16, 1), 0, "%#x", uint16_t);
    CHECKVAL(*pu16, 1, "%#x");

    CHECKOP(ASMAtomicXchgU16(pu16, 0), 1, "%#x", uint16_t);
    CHECKVAL(*pu16, 0, "%#x");

    CHECKOP(ASMAtomicXchgU16(pu16, 0xffff), 0, "%#x", uint16_t);
    CHECKVAL(*pu16, 0xffff, "%#x");

    CHECKOP(ASMAtomicXchgU16(pu16, 0x8765), 0xffff, "%#x", uint16_t);
    CHECKVAL(*pu16, 0x8765, "%#x");
}


static void tstASMAtomicXchgU16(void)
{
    DO_SIMPLE_TEST(ASMAtomicXchgU16, uint16_t);
}


DECLINLINE(void) tstASMAtomicXchgU32Worker(uint32_t volatile *pu32)
{
    *pu32 = 0;

    CHECKOP(ASMAtomicXchgU32(pu32, 1), 0, "%#x", uint32_t);
    CHECKVAL(*pu32, 1, "%#x");

    CHECKOP(ASMAtomicXchgU32(pu32, 0), 1, "%#x", uint32_t);
    CHECKVAL(*pu32, 0, "%#x");

    CHECKOP(ASMAtomicXchgU32(pu32, ~UINT32_C(0)), 0, "%#x", uint32_t);
    CHECKVAL(*pu32, ~UINT32_C(0), "%#x");

    CHECKOP(ASMAtomicXchgU32(pu32, 0x87654321), ~UINT32_C(0), "%#x", uint32_t);
    CHECKVAL(*pu32, 0x87654321, "%#x");
}


static void tstASMAtomicXchgU32(void)
{
    DO_SIMPLE_TEST(ASMAtomicXchgU32, uint32_t);
}


DECLINLINE(void) tstASMAtomicXchgU64Worker(uint64_t volatile *pu64)
{
    *pu64 = 0;

    CHECKOP(ASMAtomicXchgU64(pu64, 1), UINT64_C(0), "%#llx", uint64_t);
    CHECKVAL(*pu64, UINT64_C(1), "%#llx");

    CHECKOP(ASMAtomicXchgU64(pu64, 0), UINT64_C(1), "%#llx", uint64_t);
    CHECKVAL(*pu64, UINT64_C(0), "%#llx");

    CHECKOP(ASMAtomicXchgU64(pu64, ~UINT64_C(0)), UINT64_C(0), "%#llx", uint64_t);
    CHECKVAL(*pu64, ~UINT64_C(0), "%#llx");

    CHECKOP(ASMAtomicXchgU64(pu64, UINT64_C(0xfedcba0987654321)),  ~UINT64_C(0), "%#llx", uint64_t);
    CHECKVAL(*pu64, UINT64_C(0xfedcba0987654321), "%#llx");
}


static void tstASMAtomicXchgU64(void)
{
    DO_SIMPLE_TEST(ASMAtomicXchgU64, uint64_t);
}


DECLINLINE(void) tstASMAtomicXchgPtrWorker(void * volatile *ppv)
{
    *ppv = NULL;

    CHECKOP(ASMAtomicXchgPtr(ppv, (void *)(~(uintptr_t)0)), NULL, "%p", void *);
    CHECKVAL(*ppv, (void *)(~(uintptr_t)0), "%p");

    CHECKOP(ASMAtomicXchgPtr(ppv, (void *)(uintptr_t)0x87654321), (void *)(~(uintptr_t)0), "%p", void *);
    CHECKVAL(*ppv, (void *)(uintptr_t)0x87654321, "%p");

    CHECKOP(ASMAtomicXchgPtr(ppv, NULL), (void *)(uintptr_t)0x87654321, "%p", void *);
    CHECKVAL(*ppv, NULL, "%p");
}


static void tstASMAtomicXchgPtr(void)
{
    DO_SIMPLE_TEST(ASMAtomicXchgPtr, void *);
}


DECLINLINE(void) tstASMAtomicCmpXchgU8Worker(uint8_t volatile *pu8)
{
    *pu8 = 0xff;

    CHECKOP(ASMAtomicCmpXchgU8(pu8, 0, 0), false, "%d", bool);
    CHECKVAL(*pu8, 0xff, "%x");

    CHECKOP(ASMAtomicCmpXchgU8(pu8, 0, 0xff), true, "%d", bool);
    CHECKVAL(*pu8, 0, "%x");

    CHECKOP(ASMAtomicCmpXchgU8(pu8, 0x79, 0xff), false, "%d", bool);
    CHECKVAL(*pu8, 0, "%x");

    CHECKOP(ASMAtomicCmpXchgU8(pu8, 0x97, 0), true, "%d", bool);
    CHECKVAL(*pu8, 0x97, "%x");
}


static void tstASMAtomicCmpXchgU8(void)
{
    DO_SIMPLE_TEST(ASMAtomicCmpXchgU8, uint8_t);
}


DECLINLINE(void) tstASMAtomicCmpXchgU32Worker(uint32_t volatile *pu32)
{
    *pu32 = UINT32_C(0xffffffff);

    CHECKOP(ASMAtomicCmpXchgU32(pu32, 0, 0), false, "%d", bool);
    CHECKVAL(*pu32, UINT32_C(0xffffffff), "%x");

    CHECKOP(ASMAtomicCmpXchgU32(pu32, 0, UINT32_C(0xffffffff)), true, "%d", bool);
    CHECKVAL(*pu32, 0, "%x");

    CHECKOP(ASMAtomicCmpXchgU32(pu32, UINT32_C(0x8008efd), UINT32_C(0xffffffff)), false, "%d", bool);
    CHECKVAL(*pu32, 0, "%x");

    CHECKOP(ASMAtomicCmpXchgU32(pu32, UINT32_C(0x8008efd), 0), true, "%d", bool);
    CHECKVAL(*pu32, UINT32_C(0x8008efd), "%x");
}


static void tstASMAtomicCmpXchgU32(void)
{
    DO_SIMPLE_TEST(ASMAtomicCmpXchgU32, uint32_t);
}



DECLINLINE(void) tstASMAtomicCmpXchgU64Worker(uint64_t volatile *pu64)
{
    *pu64 = UINT64_C(0xffffffffffffff);

    CHECKOP(ASMAtomicCmpXchgU64(pu64, 0, 0), false, "%d", bool);
    CHECKVAL(*pu64, UINT64_C(0xffffffffffffff), "%#llx");

    CHECKOP(ASMAtomicCmpXchgU64(pu64, 0, UINT64_C(0xffffffffffffff)), true, "%d", bool);
    CHECKVAL(*pu64, 0, "%x");

    CHECKOP(ASMAtomicCmpXchgU64(pu64, UINT64_C(0x80040008008efd), UINT64_C(0xffffffff)), false, "%d", bool);
    CHECKVAL(*pu64, 0, "%x");

    CHECKOP(ASMAtomicCmpXchgU64(pu64, UINT64_C(0x80040008008efd), UINT64_C(0xffffffff00000000)), false, "%d", bool);
    CHECKVAL(*pu64, 0, "%x");

    CHECKOP(ASMAtomicCmpXchgU64(pu64, UINT64_C(0x80040008008efd), 0), true, "%d", bool);
    CHECKVAL(*pu64, UINT64_C(0x80040008008efd), "%#llx");
}


static void tstASMAtomicCmpXchgU64(void)
{
    DO_SIMPLE_TEST(ASMAtomicCmpXchgU64, uint64_t);
}


DECLINLINE(void) tstASMAtomicCmpXchgExU32Worker(uint32_t volatile *pu32)
{
    *pu32           = UINT32_C(0xffffffff);
    uint32_t u32Old = UINT32_C(0x80005111);

    CHECKOP(ASMAtomicCmpXchgExU32(pu32, 0, 0, &u32Old), false, "%d", bool);
    CHECKVAL(*pu32,  UINT32_C(0xffffffff), "%x");
    CHECKVAL(u32Old, UINT32_C(0xffffffff), "%x");

    CHECKOP(ASMAtomicCmpXchgExU32(pu32, 0, UINT32_C(0xffffffff), &u32Old), true, "%d", bool);
    CHECKVAL(*pu32, 0, "%x");
    CHECKVAL(u32Old, UINT32_C(0xffffffff), "%x");

    CHECKOP(ASMAtomicCmpXchgExU32(pu32, UINT32_C(0x8008efd), UINT32_C(0xffffffff), &u32Old), false, "%d", bool);
    CHECKVAL(*pu32, 0, "%x");
    CHECKVAL(u32Old, 0, "%x");

    CHECKOP(ASMAtomicCmpXchgExU32(pu32, UINT32_C(0x8008efd), 0, &u32Old), true, "%d", bool);
    CHECKVAL(*pu32, UINT32_C(0x8008efd), "%x");
    CHECKVAL(u32Old, 0, "%x");

    CHECKOP(ASMAtomicCmpXchgExU32(pu32, 0, UINT32_C(0x8008efd), &u32Old), true, "%d", bool);
    CHECKVAL(*pu32, 0, "%x");
    CHECKVAL(u32Old, UINT32_C(0x8008efd), "%x");
}


static void tstASMAtomicCmpXchgExU32(void)
{
    DO_SIMPLE_TEST(ASMAtomicCmpXchgExU32, uint32_t);
}


DECLINLINE(void) tstASMAtomicCmpXchgExU64Worker(uint64_t volatile *pu64)
{
    *pu64 = UINT64_C(0xffffffffffffffff);
    uint64_t u64Old = UINT64_C(0x8000000051111111);

    CHECKOP(ASMAtomicCmpXchgExU64(pu64, 0, 0, &u64Old), false, "%d", bool);
    CHECKVAL(*pu64, UINT64_C(0xffffffffffffffff), "%llx");
    CHECKVAL(u64Old, UINT64_C(0xffffffffffffffff), "%llx");

    CHECKOP(ASMAtomicCmpXchgExU64(pu64, 0, UINT64_C(0xffffffffffffffff), &u64Old), true, "%d", bool);
    CHECKVAL(*pu64, UINT64_C(0), "%llx");
    CHECKVAL(u64Old, UINT64_C(0xffffffffffffffff), "%llx");

    CHECKOP(ASMAtomicCmpXchgExU64(pu64, UINT64_C(0x80040008008efd), 0xffffffff, &u64Old), false, "%d", bool);
    CHECKVAL(*pu64, UINT64_C(0), "%llx");
    CHECKVAL(u64Old, UINT64_C(0), "%llx");

    CHECKOP(ASMAtomicCmpXchgExU64(pu64, UINT64_C(0x80040008008efd), UINT64_C(0xffffffff00000000), &u64Old), false, "%d", bool);
    CHECKVAL(*pu64, UINT64_C(0), "%llx");
    CHECKVAL(u64Old, UINT64_C(0), "%llx");

    CHECKOP(ASMAtomicCmpXchgExU64(pu64, UINT64_C(0x80040008008efd), 0, &u64Old), true, "%d", bool);
    CHECKVAL(*pu64, UINT64_C(0x80040008008efd), "%llx");
    CHECKVAL(u64Old, UINT64_C(0), "%llx");

    CHECKOP(ASMAtomicCmpXchgExU64(pu64, 0, UINT64_C(0x80040008008efd), &u64Old), true, "%d", bool);
    CHECKVAL(*pu64, UINT64_C(0), "%llx");
    CHECKVAL(u64Old, UINT64_C(0x80040008008efd), "%llx");
}


static void tstASMAtomicCmpXchgExU64(void)
{
    DO_SIMPLE_TEST(ASMAtomicCmpXchgExU64, uint64_t);
}


DECLINLINE(void) tstASMAtomicReadU64Worker(uint64_t volatile *pu64)
{
    *pu64 = 0;

    CHECKOP(ASMAtomicReadU64(pu64), UINT64_C(0), "%#llx", uint64_t);
    CHECKVAL(*pu64, UINT64_C(0), "%#llx");

    *pu64 = ~UINT64_C(0);
    CHECKOP(ASMAtomicReadU64(pu64), ~UINT64_C(0), "%#llx", uint64_t);
    CHECKVAL(*pu64, ~UINT64_C(0), "%#llx");

    *pu64 = UINT64_C(0xfedcba0987654321);
    CHECKOP(ASMAtomicReadU64(pu64), UINT64_C(0xfedcba0987654321), "%#llx", uint64_t);
    CHECKVAL(*pu64, UINT64_C(0xfedcba0987654321), "%#llx");
}


static void tstASMAtomicReadU64(void)
{
    DO_SIMPLE_TEST(ASMAtomicReadU64, uint64_t);
}


DECLINLINE(void) tstASMAtomicUoReadU64Worker(uint64_t volatile *pu64)
{
    *pu64 = 0;

    CHECKOP(ASMAtomicUoReadU64(pu64), UINT64_C(0), "%#llx", uint64_t);
    CHECKVAL(*pu64, UINT64_C(0), "%#llx");

    *pu64 = ~UINT64_C(0);
    CHECKOP(ASMAtomicUoReadU64(pu64), ~UINT64_C(0), "%#llx", uint64_t);
    CHECKVAL(*pu64, ~UINT64_C(0), "%#llx");

    *pu64 = UINT64_C(0xfedcba0987654321);
    CHECKOP(ASMAtomicUoReadU64(pu64), UINT64_C(0xfedcba0987654321), "%#llx", uint64_t);
    CHECKVAL(*pu64, UINT64_C(0xfedcba0987654321), "%#llx");
}


static void tstASMAtomicUoReadU64(void)
{
    DO_SIMPLE_TEST(ASMAtomicUoReadU64, uint64_t);
}


DECLINLINE(void) tstASMAtomicAddS32Worker(int32_t *pi32)
{
    int32_t i32Rc;
    *pi32 = 10;
#define MYCHECK(op, rc, val) \
    do { \
        i32Rc = op; \
        if (i32Rc != (rc)) \
            RTTestFailed(g_hTest, "%s, %d: FAILURE: %s -> %d expected %d\n", __FUNCTION__, __LINE__, #op, i32Rc, rc); \
        if (*pi32 != (val)) \
            RTTestFailed(g_hTest, "%s, %d: FAILURE: %s => *pi32=%d expected %d\n", __FUNCTION__, __LINE__, #op, *pi32, val); \
    } while (0)
    MYCHECK(ASMAtomicAddS32(pi32, 1),               10,             11);
    MYCHECK(ASMAtomicAddS32(pi32, -2),              11,             9);
    MYCHECK(ASMAtomicAddS32(pi32, -9),              9,              0);
    MYCHECK(ASMAtomicAddS32(pi32, -0x7fffffff),     0,              -0x7fffffff);
    MYCHECK(ASMAtomicAddS32(pi32, 0),               -0x7fffffff,    -0x7fffffff);
    MYCHECK(ASMAtomicAddS32(pi32, 0x7fffffff),      -0x7fffffff,    0);
    MYCHECK(ASMAtomicAddS32(pi32, 0),               0,              0);
#undef MYCHECK
}


static void tstASMAtomicAddS32(void)
{
    DO_SIMPLE_TEST(ASMAtomicAddS32, int32_t);
}


DECLINLINE(void) tstASMAtomicUoIncU32Worker(uint32_t volatile *pu32)
{
    *pu32 = 0;

    CHECKOP(ASMAtomicUoIncU32(pu32), UINT32_C(1), "%#x", uint32_t);
    CHECKVAL(*pu32, UINT32_C(1), "%#x");

    *pu32 = ~UINT32_C(0);
    CHECKOP(ASMAtomicUoIncU32(pu32), 0, "%#x", uint32_t);
    CHECKVAL(*pu32, 0, "%#x");

    *pu32 = UINT32_C(0x7fffffff);
    CHECKOP(ASMAtomicUoIncU32(pu32), UINT32_C(0x80000000), "%#x", uint32_t);
    CHECKVAL(*pu32, UINT32_C(0x80000000), "%#x");
}


static void tstASMAtomicUoIncU32(void)
{
    DO_SIMPLE_TEST(ASMAtomicUoIncU32, uint32_t);
}


DECLINLINE(void) tstASMAtomicUoDecU32Worker(uint32_t volatile *pu32)
{
    *pu32 = 0;

    CHECKOP(ASMAtomicUoDecU32(pu32), ~UINT32_C(0), "%#x", uint32_t);
    CHECKVAL(*pu32, ~UINT32_C(0), "%#x");

    *pu32 = ~UINT32_C(0);
    CHECKOP(ASMAtomicUoDecU32(pu32), UINT32_C(0xfffffffe), "%#x", uint32_t);
    CHECKVAL(*pu32, UINT32_C(0xfffffffe), "%#x");

    *pu32 = UINT32_C(0x80000000);
    CHECKOP(ASMAtomicUoDecU32(pu32), UINT32_C(0x7fffffff), "%#x", uint32_t);
    CHECKVAL(*pu32, UINT32_C(0x7fffffff), "%#x");
}


static void tstASMAtomicUoDecU32(void)
{
    DO_SIMPLE_TEST(ASMAtomicUoDecU32, uint32_t);
}


DECLINLINE(void) tstASMAtomicAddS64Worker(int64_t volatile *pi64)
{
    int64_t i64Rc;
    *pi64 = 10;
#define MYCHECK(op, rc, val) \
    do { \
        i64Rc = op; \
        if (i64Rc != (rc)) \
            RTTestFailed(g_hTest, "%s, %d: FAILURE: %s -> %llx expected %llx\n", __FUNCTION__, __LINE__, #op, i64Rc, (int64_t)rc); \
        if (*pi64 != (val)) \
            RTTestFailed(g_hTest, "%s, %d: FAILURE: %s => *pi64=%llx expected %llx\n", __FUNCTION__, __LINE__, #op, *pi64, (int64_t)(val)); \
    } while (0)
    MYCHECK(ASMAtomicAddS64(pi64, 1),               10,             11);
    MYCHECK(ASMAtomicAddS64(pi64, -2),              11,             9);
    MYCHECK(ASMAtomicAddS64(pi64, -9),              9,              0);
    MYCHECK(ASMAtomicAddS64(pi64, -INT64_MAX),      0,              -INT64_MAX);
    MYCHECK(ASMAtomicAddS64(pi64, 0),               -INT64_MAX,     -INT64_MAX);
    MYCHECK(ASMAtomicAddS64(pi64, -1),              -INT64_MAX,     INT64_MIN);
    MYCHECK(ASMAtomicAddS64(pi64, INT64_MAX),       INT64_MIN,      -1);
    MYCHECK(ASMAtomicAddS64(pi64, 1),               -1,             0);
    MYCHECK(ASMAtomicAddS64(pi64, 0),               0,              0);
#undef MYCHECK
}


static void tstASMAtomicAddS64(void)
{
    DO_SIMPLE_TEST(ASMAtomicAddS64, int64_t);
}


DECLINLINE(void) tstASMAtomicDecIncS32Worker(int32_t volatile *pi32)
{
    int32_t i32Rc;
    *pi32 = 10;
#define MYCHECK(op, rc) \
    do { \
        i32Rc = op; \
        if (i32Rc != (rc)) \
            RTTestFailed(g_hTest, "%s, %d: FAILURE: %s -> %d expected %d\n", __FUNCTION__, __LINE__, #op, i32Rc, rc); \
        if (*pi32 != (rc)) \
            RTTestFailed(g_hTest, "%s, %d: FAILURE: %s => *pi32=%d expected %d\n", __FUNCTION__, __LINE__, #op, *pi32, rc); \
    } while (0)
    MYCHECK(ASMAtomicDecS32(pi32), 9);
    MYCHECK(ASMAtomicDecS32(pi32), 8);
    MYCHECK(ASMAtomicDecS32(pi32), 7);
    MYCHECK(ASMAtomicDecS32(pi32), 6);
    MYCHECK(ASMAtomicDecS32(pi32), 5);
    MYCHECK(ASMAtomicDecS32(pi32), 4);
    MYCHECK(ASMAtomicDecS32(pi32), 3);
    MYCHECK(ASMAtomicDecS32(pi32), 2);
    MYCHECK(ASMAtomicDecS32(pi32), 1);
    MYCHECK(ASMAtomicDecS32(pi32), 0);
    MYCHECK(ASMAtomicDecS32(pi32), -1);
    MYCHECK(ASMAtomicDecS32(pi32), -2);
    MYCHECK(ASMAtomicIncS32(pi32), -1);
    MYCHECK(ASMAtomicIncS32(pi32), 0);
    MYCHECK(ASMAtomicIncS32(pi32), 1);
    MYCHECK(ASMAtomicIncS32(pi32), 2);
    MYCHECK(ASMAtomicIncS32(pi32), 3);
    MYCHECK(ASMAtomicDecS32(pi32), 2);
    MYCHECK(ASMAtomicIncS32(pi32), 3);
    MYCHECK(ASMAtomicDecS32(pi32), 2);
    MYCHECK(ASMAtomicIncS32(pi32), 3);
#undef MYCHECK
}


static void tstASMAtomicDecIncS32(void)
{
    DO_SIMPLE_TEST(ASMAtomicDecIncS32, int32_t);
}


DECLINLINE(void) tstASMAtomicDecIncS64Worker(int64_t volatile *pi64)
{
    int64_t i64Rc;
    *pi64 = 10;
#define MYCHECK(op, rc) \
    do { \
        i64Rc = op; \
        if (i64Rc != (rc)) \
            RTTestFailed(g_hTest, "%s, %d: FAILURE: %s -> %lld expected %lld\n", __FUNCTION__, __LINE__, #op, i64Rc, rc); \
        if (*pi64 != (rc)) \
            RTTestFailed(g_hTest, "%s, %d: FAILURE: %s => *pi64=%lld expected %lld\n", __FUNCTION__, __LINE__, #op, *pi64, rc); \
    } while (0)
    MYCHECK(ASMAtomicDecS64(pi64), 9);
    MYCHECK(ASMAtomicDecS64(pi64), 8);
    MYCHECK(ASMAtomicDecS64(pi64), 7);
    MYCHECK(ASMAtomicDecS64(pi64), 6);
    MYCHECK(ASMAtomicDecS64(pi64), 5);
    MYCHECK(ASMAtomicDecS64(pi64), 4);
    MYCHECK(ASMAtomicDecS64(pi64), 3);
    MYCHECK(ASMAtomicDecS64(pi64), 2);
    MYCHECK(ASMAtomicDecS64(pi64), 1);
    MYCHECK(ASMAtomicDecS64(pi64), 0);
    MYCHECK(ASMAtomicDecS64(pi64), -1);
    MYCHECK(ASMAtomicDecS64(pi64), -2);
    MYCHECK(ASMAtomicIncS64(pi64), -1);
    MYCHECK(ASMAtomicIncS64(pi64), 0);
    MYCHECK(ASMAtomicIncS64(pi64), 1);
    MYCHECK(ASMAtomicIncS64(pi64), 2);
    MYCHECK(ASMAtomicIncS64(pi64), 3);
    MYCHECK(ASMAtomicDecS64(pi64), 2);
    MYCHECK(ASMAtomicIncS64(pi64), 3);
    MYCHECK(ASMAtomicDecS64(pi64), 2);
    MYCHECK(ASMAtomicIncS64(pi64), 3);
#undef MYCHECK
}


static void tstASMAtomicDecIncS64(void)
{
    DO_SIMPLE_TEST(ASMAtomicDecIncS64, int64_t);
}


DECLINLINE(void) tstASMAtomicAndOrU32Worker(uint32_t volatile *pu32)
{
    *pu32 = UINT32_C(0xffffffff);

    ASMAtomicOrU32(pu32, UINT32_C(0xffffffff));
    CHECKVAL(*pu32, UINT32_C(0xffffffff), "%x");

    ASMAtomicAndU32(pu32, UINT32_C(0xffffffff));
    CHECKVAL(*pu32, UINT32_C(0xffffffff), "%x");

    ASMAtomicAndU32(pu32, UINT32_C(0x8f8f8f8f));
    CHECKVAL(*pu32, UINT32_C(0x8f8f8f8f), "%x");

    ASMAtomicOrU32(pu32, UINT32_C(0x70707070));
    CHECKVAL(*pu32, UINT32_C(0xffffffff), "%x");

    ASMAtomicAndU32(pu32, UINT32_C(1));
    CHECKVAL(*pu32, UINT32_C(1), "%x");

    ASMAtomicOrU32(pu32, UINT32_C(0x80000000));
    CHECKVAL(*pu32, UINT32_C(0x80000001), "%x");

    ASMAtomicAndU32(pu32, UINT32_C(0x80000000));
    CHECKVAL(*pu32, UINT32_C(0x80000000), "%x");

    ASMAtomicAndU32(pu32, UINT32_C(0));
    CHECKVAL(*pu32, UINT32_C(0), "%x");

    ASMAtomicOrU32(pu32, UINT32_C(0x42424242));
    CHECKVAL(*pu32, UINT32_C(0x42424242), "%x");
}


static void tstASMAtomicAndOrU32(void)
{
    DO_SIMPLE_TEST(ASMAtomicAndOrU32, uint32_t);
}


DECLINLINE(void) tstASMAtomicAndOrU64Worker(uint64_t volatile *pu64)
{
    *pu64 = UINT64_C(0xffffffff);

    ASMAtomicOrU64(pu64, UINT64_C(0xffffffff));
    CHECKVAL(*pu64, UINT64_C(0xffffffff), "%x");

    ASMAtomicAndU64(pu64, UINT64_C(0xffffffff));
    CHECKVAL(*pu64, UINT64_C(0xffffffff), "%x");

    ASMAtomicAndU64(pu64, UINT64_C(0x8f8f8f8f));
    CHECKVAL(*pu64, UINT64_C(0x8f8f8f8f), "%x");

    ASMAtomicOrU64(pu64, UINT64_C(0x70707070));
    CHECKVAL(*pu64, UINT64_C(0xffffffff), "%x");

    ASMAtomicAndU64(pu64, UINT64_C(1));
    CHECKVAL(*pu64, UINT64_C(1), "%x");

    ASMAtomicOrU64(pu64, UINT64_C(0x80000000));
    CHECKVAL(*pu64, UINT64_C(0x80000001), "%x");

    ASMAtomicAndU64(pu64, UINT64_C(0x80000000));
    CHECKVAL(*pu64, UINT64_C(0x80000000), "%x");

    ASMAtomicAndU64(pu64, UINT64_C(0));
    CHECKVAL(*pu64, UINT64_C(0), "%x");

    ASMAtomicOrU64(pu64, UINT64_C(0x42424242));
    CHECKVAL(*pu64, UINT64_C(0x42424242), "%x");

    // Same as above, but now 64-bit wide.
    ASMAtomicAndU64(pu64, UINT64_C(0));
    CHECKVAL(*pu64, UINT64_C(0), "%x");

    ASMAtomicOrU64(pu64, UINT64_C(0xffffffffffffffff));
    CHECKVAL(*pu64, UINT64_C(0xffffffffffffffff), "%x");

    ASMAtomicAndU64(pu64, UINT64_C(0xffffffffffffffff));
    CHECKVAL(*pu64, UINT64_C(0xffffffffffffffff), "%x");

    ASMAtomicAndU64(pu64, UINT64_C(0x8f8f8f8f8f8f8f8f));
    CHECKVAL(*pu64, UINT64_C(0x8f8f8f8f8f8f8f8f), "%x");

    ASMAtomicOrU64(pu64, UINT64_C(0x7070707070707070));
    CHECKVAL(*pu64, UINT64_C(0xffffffffffffffff), "%x");

    ASMAtomicAndU64(pu64, UINT64_C(1));
    CHECKVAL(*pu64, UINT64_C(1), "%x");

    ASMAtomicOrU64(pu64, UINT64_C(0x8000000000000000));
    CHECKVAL(*pu64, UINT64_C(0x8000000000000001), "%x");

    ASMAtomicAndU64(pu64, UINT64_C(0x8000000000000000));
    CHECKVAL(*pu64, UINT64_C(0x8000000000000000), "%x");

    ASMAtomicAndU64(pu64, UINT64_C(0));
    CHECKVAL(*pu64, UINT64_C(0), "%x");

    ASMAtomicOrU64(pu64, UINT64_C(0x4242424242424242));
    CHECKVAL(*pu64, UINT64_C(0x4242424242424242), "%x");
}


static void tstASMAtomicAndOrU64(void)
{
    DO_SIMPLE_TEST(ASMAtomicAndOrU64, uint64_t);
}


DECLINLINE(void) tstASMAtomicUoAndOrU32Worker(uint32_t volatile *pu32)
{
    *pu32 = UINT32_C(0xffffffff);

    ASMAtomicUoOrU32(pu32, UINT32_C(0xffffffff));
    CHECKVAL(*pu32, UINT32_C(0xffffffff), "%#x");

    ASMAtomicUoAndU32(pu32, UINT32_C(0xffffffff));
    CHECKVAL(*pu32, UINT32_C(0xffffffff), "%#x");

    ASMAtomicUoAndU32(pu32, UINT32_C(0x8f8f8f8f));
    CHECKVAL(*pu32, UINT32_C(0x8f8f8f8f), "%#x");

    ASMAtomicUoOrU32(pu32, UINT32_C(0x70707070));
    CHECKVAL(*pu32, UINT32_C(0xffffffff), "%#x");

    ASMAtomicUoAndU32(pu32, UINT32_C(1));
    CHECKVAL(*pu32, UINT32_C(1), "%#x");

    ASMAtomicUoOrU32(pu32, UINT32_C(0x80000000));
    CHECKVAL(*pu32, UINT32_C(0x80000001), "%#x");

    ASMAtomicUoAndU32(pu32, UINT32_C(0x80000000));
    CHECKVAL(*pu32, UINT32_C(0x80000000), "%#x");

    ASMAtomicUoAndU32(pu32, UINT32_C(0));
    CHECKVAL(*pu32, UINT32_C(0), "%#x");

    ASMAtomicUoOrU32(pu32, UINT32_C(0x42424242));
    CHECKVAL(*pu32, UINT32_C(0x42424242), "%#x");
}


static void tstASMAtomicUoAndOrU32(void)
{
    DO_SIMPLE_TEST(ASMAtomicUoAndOrU32, uint32_t);
}


typedef struct
{
    uint8_t ab[PAGE_SIZE];
} TSTPAGE;


DECLINLINE(void) tstASMMemZeroPageWorker(TSTPAGE *pPage)
{
    for (unsigned j = 0; j < 16; j++)
    {
        memset(pPage, 0x11 * j, sizeof(*pPage));
        ASMMemZeroPage(pPage);
        for (unsigned i = 0; i < sizeof(pPage->ab); i++)
            if (pPage->ab[i])
                RTTestFailed(g_hTest, "ASMMemZeroPage didn't clear byte at offset %#x!\n", i);
    }
}


static void tstASMMemZeroPage(void)
{
    DO_SIMPLE_TEST(ASMMemZeroPage, TSTPAGE);
}


void tstASMMemIsZeroPage(RTTEST hTest)
{
    RTTestSub(hTest, "ASMMemIsZeroPage");

    void *pvPage1 = RTTestGuardedAllocHead(hTest, PAGE_SIZE);
    void *pvPage2 = RTTestGuardedAllocTail(hTest, PAGE_SIZE);
    RTTESTI_CHECK_RETV(pvPage1 && pvPage2);

    memset(pvPage1, 0, PAGE_SIZE);
    memset(pvPage2, 0, PAGE_SIZE);
    RTTESTI_CHECK(ASMMemIsZeroPage(pvPage1));
    RTTESTI_CHECK(ASMMemIsZeroPage(pvPage2));

    memset(pvPage1, 0xff, PAGE_SIZE);
    memset(pvPage2, 0xff, PAGE_SIZE);
    RTTESTI_CHECK(!ASMMemIsZeroPage(pvPage1));
    RTTESTI_CHECK(!ASMMemIsZeroPage(pvPage2));

    memset(pvPage1, 0, PAGE_SIZE);
    memset(pvPage2, 0, PAGE_SIZE);
    for (unsigned off = 0; off < PAGE_SIZE; off++)
    {
        ((uint8_t *)pvPage1)[off] = 1;
        RTTESTI_CHECK(!ASMMemIsZeroPage(pvPage1));
        ((uint8_t *)pvPage1)[off] = 0;

        ((uint8_t *)pvPage2)[off] = 0x80;
        RTTESTI_CHECK(!ASMMemIsZeroPage(pvPage2));
        ((uint8_t *)pvPage2)[off] = 0;
    }

    RTTestSubDone(hTest);
}


void tstASMMemFirstMismatchingU8(RTTEST hTest)
{
    RTTestSub(hTest, "ASMMemFirstMismatchingU8");

    uint8_t *pbPage1 = (uint8_t *)RTTestGuardedAllocHead(hTest, PAGE_SIZE);
    uint8_t *pbPage2 = (uint8_t *)RTTestGuardedAllocTail(hTest, PAGE_SIZE);
    RTTESTI_CHECK_RETV(pbPage1 && pbPage2);

    memset(pbPage1, 0, PAGE_SIZE);
    memset(pbPage2, 0, PAGE_SIZE);
    RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage1, PAGE_SIZE, 0) == NULL);
    RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage2, PAGE_SIZE, 0) == NULL);
    RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage1, PAGE_SIZE, 1) == pbPage1);
    RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage2, PAGE_SIZE, 1) == pbPage2);
    RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage1, PAGE_SIZE, 0x87) == pbPage1);
    RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage2, PAGE_SIZE, 0x87) == pbPage2);
    RTTESTI_CHECK(ASMMemIsZero(pbPage1, PAGE_SIZE));
    RTTESTI_CHECK(ASMMemIsZero(pbPage2, PAGE_SIZE));
    RTTESTI_CHECK(ASMMemIsAllU8(pbPage1, PAGE_SIZE, 0));
    RTTESTI_CHECK(ASMMemIsAllU8(pbPage2, PAGE_SIZE, 0));
    RTTESTI_CHECK(!ASMMemIsAllU8(pbPage1, PAGE_SIZE, 0x34));
    RTTESTI_CHECK(!ASMMemIsAllU8(pbPage2, PAGE_SIZE, 0x88));
    unsigned cbSub = 32;
    while (cbSub-- > 0)
    {
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(&pbPage1[PAGE_SIZE - cbSub], cbSub, 0) == NULL);
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(&pbPage2[PAGE_SIZE - cbSub], cbSub, 0) == NULL);
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage1, cbSub, 0) == NULL);
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage2, cbSub, 0) == NULL);

        RTTESTI_CHECK(ASMMemFirstMismatchingU8(&pbPage1[PAGE_SIZE - cbSub], cbSub, 0x34) == &pbPage1[PAGE_SIZE - cbSub] || !cbSub);
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(&pbPage2[PAGE_SIZE - cbSub], cbSub, 0x99) == &pbPage2[PAGE_SIZE - cbSub] || !cbSub);
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage1, cbSub, 0x42) == pbPage1 || !cbSub);
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage2, cbSub, 0x88) == pbPage2 || !cbSub);
    }

    memset(pbPage1, 0xff, PAGE_SIZE);
    memset(pbPage2, 0xff, PAGE_SIZE);
    RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage1, PAGE_SIZE, 0xff) == NULL);
    RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage2, PAGE_SIZE, 0xff) == NULL);
    RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage1, PAGE_SIZE, 0xfe) == pbPage1);
    RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage2, PAGE_SIZE, 0xfe) == pbPage2);
    RTTESTI_CHECK(!ASMMemIsZero(pbPage1, PAGE_SIZE));
    RTTESTI_CHECK(!ASMMemIsZero(pbPage2, PAGE_SIZE));
    RTTESTI_CHECK(ASMMemIsAllU8(pbPage1, PAGE_SIZE, 0xff));
    RTTESTI_CHECK(ASMMemIsAllU8(pbPage2, PAGE_SIZE, 0xff));
    RTTESTI_CHECK(!ASMMemIsAllU8(pbPage1, PAGE_SIZE, 0));
    RTTESTI_CHECK(!ASMMemIsAllU8(pbPage2, PAGE_SIZE, 0));
    cbSub = 32;
    while (cbSub-- > 0)
    {
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(&pbPage1[PAGE_SIZE - cbSub], cbSub, 0xff) == NULL);
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(&pbPage2[PAGE_SIZE - cbSub], cbSub, 0xff) == NULL);
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage1, cbSub, 0xff) == NULL);
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage2, cbSub, 0xff) == NULL);

        RTTESTI_CHECK(ASMMemFirstMismatchingU8(&pbPage1[PAGE_SIZE - cbSub], cbSub, 0xfe) == &pbPage1[PAGE_SIZE - cbSub] || !cbSub);
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(&pbPage2[PAGE_SIZE - cbSub], cbSub, 0xfe) == &pbPage2[PAGE_SIZE - cbSub] || !cbSub);
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage1, cbSub, 0xfe) == pbPage1 || !cbSub);
        RTTESTI_CHECK(ASMMemFirstMismatchingU8(pbPage2, cbSub, 0xfe) == pbPage2 || !cbSub);
    }


    /*
     * Various alignments and sizes.
     */
    uint8_t const  bFiller1 = 0x00;
    uint8_t const  bFiller2 = 0xf6;
    size_t const   cbBuf    = 128;
    uint8_t       *pbBuf1   = pbPage1;
    uint8_t       *pbBuf2   = &pbPage2[PAGE_SIZE - cbBuf]; /* Put it up against the tail guard */
    memset(pbPage1, ~bFiller1, PAGE_SIZE);
    memset(pbPage2, ~bFiller2, PAGE_SIZE);
    memset(pbBuf1, bFiller1, cbBuf);
    memset(pbBuf2, bFiller2, cbBuf);
    for (size_t offNonZero = 0; offNonZero < cbBuf; offNonZero++)
    {
        uint8_t bRand = (uint8_t)RTRandU32();
        pbBuf1[offNonZero] = bRand | 1;
        pbBuf2[offNonZero] = (0x80 | bRand) ^ 0xf6;

        for (size_t offStart = 0; offStart < 32; offStart++)
        {
            size_t const  cbMax = cbBuf - offStart;
            for (size_t cb = 0; cb < cbMax; cb++)
            {
                size_t const offEnd = offStart + cb;
                uint8_t bSaved1, bSaved2;
                if (offEnd < PAGE_SIZE)
                {
                    bSaved1 = pbBuf1[offEnd];
                    bSaved2 = pbBuf2[offEnd];
                    pbBuf1[offEnd] = 0xff;
                    pbBuf2[offEnd] = 0xff;
                }
#ifdef _MSC_VER /* simple stupid compiler warnings */
                else
                    bSaved1 = bSaved2 = 0;
#endif

                uint8_t *pbRet = (uint8_t *)ASMMemFirstMismatchingU8(pbBuf1 + offStart, cb, bFiller1);
                RTTESTI_CHECK(offNonZero - offStart < cb ? pbRet == &pbBuf1[offNonZero] : pbRet == NULL);

                pbRet = (uint8_t *)ASMMemFirstMismatchingU8(pbBuf2 + offStart, cb, bFiller2);
                RTTESTI_CHECK(offNonZero - offStart < cb ? pbRet == &pbBuf2[offNonZero] : pbRet == NULL);

                if (offEnd < PAGE_SIZE)
                {
                    pbBuf1[offEnd] = bSaved1;
                    pbBuf2[offEnd] = bSaved2;
                }
            }
        }

        pbBuf1[offNonZero] = 0;
        pbBuf2[offNonZero] = 0xf6;
    }

    RTTestSubDone(hTest);
}


void tstASMMemZero32(void)
{
    RTTestSub(g_hTest, "ASMMemFill32");

    struct
    {
        uint64_t    u64Magic1;
        uint8_t     abPage[PAGE_SIZE - 32];
        uint64_t    u64Magic2;
    } Buf1, Buf2, Buf3;

    Buf1.u64Magic1 = UINT64_C(0xffffffffffffffff);
    memset(Buf1.abPage, 0x55, sizeof(Buf1.abPage));
    Buf1.u64Magic2 = UINT64_C(0xffffffffffffffff);
    Buf2.u64Magic1 = UINT64_C(0xffffffffffffffff);
    memset(Buf2.abPage, 0x77, sizeof(Buf2.abPage));
    Buf2.u64Magic2 = UINT64_C(0xffffffffffffffff);
    Buf3.u64Magic1 = UINT64_C(0xffffffffffffffff);
    memset(Buf3.abPage, 0x99, sizeof(Buf3.abPage));
    Buf3.u64Magic2 = UINT64_C(0xffffffffffffffff);
    ASMMemZero32(Buf1.abPage, sizeof(Buf1.abPage));
    ASMMemZero32(Buf2.abPage, sizeof(Buf2.abPage));
    ASMMemZero32(Buf3.abPage, sizeof(Buf3.abPage));
    if (    Buf1.u64Magic1 != UINT64_C(0xffffffffffffffff)
        ||  Buf1.u64Magic2 != UINT64_C(0xffffffffffffffff)
        ||  Buf2.u64Magic1 != UINT64_C(0xffffffffffffffff)
        ||  Buf2.u64Magic2 != UINT64_C(0xffffffffffffffff)
        ||  Buf3.u64Magic1 != UINT64_C(0xffffffffffffffff)
        ||  Buf3.u64Magic2 != UINT64_C(0xffffffffffffffff))
    {
        RTTestFailed(g_hTest, "ASMMemZero32 violated one/both magic(s)!\n");
    }
    for (unsigned i = 0; i < RT_ELEMENTS(Buf1.abPage); i++)
        if (Buf1.abPage[i])
            RTTestFailed(g_hTest, "ASMMemZero32 didn't clear byte at offset %#x!\n", i);
    for (unsigned i = 0; i < RT_ELEMENTS(Buf2.abPage); i++)
        if (Buf2.abPage[i])
            RTTestFailed(g_hTest, "ASMMemZero32 didn't clear byte at offset %#x!\n", i);
    for (unsigned i = 0; i < RT_ELEMENTS(Buf3.abPage); i++)
        if (Buf3.abPage[i])
            RTTestFailed(g_hTest, "ASMMemZero32 didn't clear byte at offset %#x!\n", i);
}


void tstASMMemFill32(void)
{
    RTTestSub(g_hTest, "ASMMemFill32");

    struct
    {
        uint64_t    u64Magic1;
        uint32_t    au32Page[PAGE_SIZE / 4];
        uint64_t    u64Magic2;
    } Buf1;
    struct
    {
        uint64_t    u64Magic1;
        uint32_t    au32Page[(PAGE_SIZE / 4) - 3];
        uint64_t    u64Magic2;
    } Buf2;
    struct
    {
        uint64_t    u64Magic1;
        uint32_t    au32Page[(PAGE_SIZE / 4) - 1];
        uint64_t    u64Magic2;
    } Buf3;

    Buf1.u64Magic1 = UINT64_C(0xffffffffffffffff);
    memset(Buf1.au32Page, 0x55, sizeof(Buf1.au32Page));
    Buf1.u64Magic2 = UINT64_C(0xffffffffffffffff);
    Buf2.u64Magic1 = UINT64_C(0xffffffffffffffff);
    memset(Buf2.au32Page, 0x77, sizeof(Buf2.au32Page));
    Buf2.u64Magic2 = UINT64_C(0xffffffffffffffff);
    Buf3.u64Magic1 = UINT64_C(0xffffffffffffffff);
    memset(Buf3.au32Page, 0x99, sizeof(Buf3.au32Page));
    Buf3.u64Magic2 = UINT64_C(0xffffffffffffffff);
    ASMMemFill32(Buf1.au32Page, sizeof(Buf1.au32Page), 0xdeadbeef);
    ASMMemFill32(Buf2.au32Page, sizeof(Buf2.au32Page), 0xcafeff01);
    ASMMemFill32(Buf3.au32Page, sizeof(Buf3.au32Page), 0xf00dd00f);
    if (    Buf1.u64Magic1 != UINT64_C(0xffffffffffffffff)
        ||  Buf1.u64Magic2 != UINT64_C(0xffffffffffffffff)
        ||  Buf2.u64Magic1 != UINT64_C(0xffffffffffffffff)
        ||  Buf2.u64Magic2 != UINT64_C(0xffffffffffffffff)
        ||  Buf3.u64Magic1 != UINT64_C(0xffffffffffffffff)
        ||  Buf3.u64Magic2 != UINT64_C(0xffffffffffffffff))
        RTTestFailed(g_hTest, "ASMMemFill32 violated one/both magic(s)!\n");
    for (unsigned i = 0; i < RT_ELEMENTS(Buf1.au32Page); i++)
        if (Buf1.au32Page[i] != 0xdeadbeef)
            RTTestFailed(g_hTest, "ASMMemFill32 %#x: %#x exepcted %#x\n", i, Buf1.au32Page[i], 0xdeadbeef);
    for (unsigned i = 0; i < RT_ELEMENTS(Buf2.au32Page); i++)
        if (Buf2.au32Page[i] != 0xcafeff01)
            RTTestFailed(g_hTest, "ASMMemFill32 %#x: %#x exepcted %#x\n", i, Buf2.au32Page[i], 0xcafeff01);
    for (unsigned i = 0; i < RT_ELEMENTS(Buf3.au32Page); i++)
        if (Buf3.au32Page[i] != 0xf00dd00f)
            RTTestFailed(g_hTest, "ASMMemFill32 %#x: %#x exepcted %#x\n", i, Buf3.au32Page[i], 0xf00dd00f);
}



void tstASMMath(void)
{
    RTTestSub(g_hTest, "Math");

    uint64_t u64 = ASMMult2xU32RetU64(UINT32_C(0x80000000), UINT32_C(0x10000000));
    CHECKVAL(u64, UINT64_C(0x0800000000000000), "%#018RX64");

    uint32_t u32 = ASMDivU64ByU32RetU32(UINT64_C(0x0800000000000000), UINT32_C(0x10000000));
    CHECKVAL(u32, UINT32_C(0x80000000), "%#010RX32");

    u32 = ASMMultU32ByU32DivByU32(UINT32_C(0x00000001), UINT32_C(0x00000001), UINT32_C(0x00000001));
    CHECKVAL(u32, UINT32_C(0x00000001), "%#018RX32");
    u32 = ASMMultU32ByU32DivByU32(UINT32_C(0x10000000), UINT32_C(0x80000000), UINT32_C(0x20000000));
    CHECKVAL(u32, UINT32_C(0x40000000), "%#018RX32");
    u32 = ASMMultU32ByU32DivByU32(UINT32_C(0x76543210), UINT32_C(0xffffffff), UINT32_C(0xffffffff));
    CHECKVAL(u32, UINT32_C(0x76543210), "%#018RX32");
    u32 = ASMMultU32ByU32DivByU32(UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff));
    CHECKVAL(u32, UINT32_C(0xffffffff), "%#018RX32");
    u32 = ASMMultU32ByU32DivByU32(UINT32_C(0xffffffff), UINT32_C(0xfffffff0), UINT32_C(0xffffffff));
    CHECKVAL(u32, UINT32_C(0xfffffff0), "%#018RX32");
    u32 = ASMMultU32ByU32DivByU32(UINT32_C(0x10359583), UINT32_C(0x58734981), UINT32_C(0xf8694045));
    CHECKVAL(u32, UINT32_C(0x05c584ce), "%#018RX32");
    u32 = ASMMultU32ByU32DivByU32(UINT32_C(0x10359583), UINT32_C(0xf8694045), UINT32_C(0x58734981));
    CHECKVAL(u32, UINT32_C(0x2d860795), "%#018RX32");

#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)
    u64 = ASMMultU64ByU32DivByU32(UINT64_C(0x0000000000000001), UINT32_C(0x00000001), UINT32_C(0x00000001));
    CHECKVAL(u64, UINT64_C(0x0000000000000001), "%#018RX64");
    u64 = ASMMultU64ByU32DivByU32(UINT64_C(0x0000000100000000), UINT32_C(0x80000000), UINT32_C(0x00000002));
    CHECKVAL(u64, UINT64_C(0x4000000000000000), "%#018RX64");
    u64 = ASMMultU64ByU32DivByU32(UINT64_C(0xfedcba9876543210), UINT32_C(0xffffffff), UINT32_C(0xffffffff));
    CHECKVAL(u64, UINT64_C(0xfedcba9876543210), "%#018RX64");
    u64 = ASMMultU64ByU32DivByU32(UINT64_C(0xffffffffffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff));
    CHECKVAL(u64, UINT64_C(0xffffffffffffffff), "%#018RX64");
    u64 = ASMMultU64ByU32DivByU32(UINT64_C(0xffffffffffffffff), UINT32_C(0xfffffff0), UINT32_C(0xffffffff));
    CHECKVAL(u64, UINT64_C(0xfffffff0fffffff0), "%#018RX64");
    u64 = ASMMultU64ByU32DivByU32(UINT64_C(0x3415934810359583), UINT32_C(0x58734981), UINT32_C(0xf8694045));
    CHECKVAL(u64, UINT64_C(0x128b9c3d43184763), "%#018RX64");
    u64 = ASMMultU64ByU32DivByU32(UINT64_C(0x3415934810359583), UINT32_C(0xf8694045), UINT32_C(0x58734981));
    CHECKVAL(u64, UINT64_C(0x924719355cd35a27), "%#018RX64");

# if 0 /* bird: question is whether this should trap or not:
       *
       * frank: Of course it must trap:
       *
       *   0xfffffff8 * 0x77d7daf8 = 0x77d7daf441412840
       *
       * During the following division, the quotient must fit into a 32-bit register.
       * Therefore the smallest valid divisor is
       *
       *  (0x77d7daf441412840 >> 32) + 1 = 0x77d7daf5
       *
       * which is definitely greater than  0x3b9aca00.
       *
       * bird: No, the C version does *not* crash. So, the question is whether there's any
       * code depending on it not crashing.
       *
       * Of course the assembly versions of the code crash right now for the reasons you've
       * given, but the 32-bit MSC version does not crash.
       *
       * frank: The C version does not crash but delivers incorrect results for this case.
       * The reason is
       *
       *   u.s.Hi = (unsigned long)(u64Hi / u32C);
       *
       * Here the division is actually 64-bit by 64-bit but the 64-bit result is truncated
       * to 32 bit. If using this (optimized and fast) function we should just be sure that
       * the operands are in a valid range.
       */
    u64 = ASMMultU64ByU32DivByU32(UINT64_C(0xfffffff8c65d6731), UINT32_C(0x77d7daf8), UINT32_C(0x3b9aca00));
    CHECKVAL(u64, UINT64_C(0x02b8f9a2aa74e3dc), "%#018RX64");
# endif
#endif /* AMD64 || X86 */

    u32 = ASMModU64ByU32RetU32(UINT64_C(0x0ffffff8c65d6731), UINT32_C(0x77d7daf8));
    CHECKVAL(u32, UINT32_C(0x3B642451), "%#010RX32");

    int32_t i32;
    i32 = ASMModS64ByS32RetS32(INT64_C(-11), INT32_C(-2));
    CHECKVAL(i32, INT32_C(-1), "%010RI32");
    i32 = ASMModS64ByS32RetS32(INT64_C(-11), INT32_C(2));
    CHECKVAL(i32, INT32_C(-1), "%010RI32");
    i32 = ASMModS64ByS32RetS32(INT64_C(11), INT32_C(-2));
    CHECKVAL(i32, INT32_C(1), "%010RI32");

    i32 = ASMModS64ByS32RetS32(INT64_C(92233720368547758), INT32_C(2147483647));
    CHECKVAL(i32, INT32_C(2104533974), "%010RI32");
    i32 = ASMModS64ByS32RetS32(INT64_C(-92233720368547758), INT32_C(2147483647));
    CHECKVAL(i32, INT32_C(-2104533974), "%010RI32");
}


void tstASMByteSwap(void)
{
    RTTestSub(g_hTest, "ASMByteSwap*");

    uint64_t u64In  = UINT64_C(0x0011223344556677);
    uint64_t u64Out = ASMByteSwapU64(u64In);
    CHECKVAL(u64In, UINT64_C(0x0011223344556677), "%#018RX64");
    CHECKVAL(u64Out, UINT64_C(0x7766554433221100), "%#018RX64");
    u64Out = ASMByteSwapU64(u64Out);
    CHECKVAL(u64Out, u64In, "%#018RX64");
    u64In  = UINT64_C(0x0123456789abcdef);
    u64Out = ASMByteSwapU64(u64In);
    CHECKVAL(u64In, UINT64_C(0x0123456789abcdef), "%#018RX64");
    CHECKVAL(u64Out, UINT64_C(0xefcdab8967452301), "%#018RX64");
    u64Out = ASMByteSwapU64(u64Out);
    CHECKVAL(u64Out, u64In, "%#018RX64");
    u64In  = 0;
    u64Out = ASMByteSwapU64(u64In);
    CHECKVAL(u64Out, u64In, "%#018RX64");
    u64In  = UINT64_MAX;
    u64Out = ASMByteSwapU64(u64In);
    CHECKVAL(u64Out, u64In, "%#018RX64");

    uint32_t u32In  = UINT32_C(0x00112233);
    uint32_t u32Out = ASMByteSwapU32(u32In);
    CHECKVAL(u32In, UINT32_C(0x00112233), "%#010RX32");
    CHECKVAL(u32Out, UINT32_C(0x33221100), "%#010RX32");
    u32Out = ASMByteSwapU32(u32Out);
    CHECKVAL(u32Out, u32In, "%#010RX32");
    u32In  = UINT32_C(0x12345678);
    u32Out = ASMByteSwapU32(u32In);
    CHECKVAL(u32In, UINT32_C(0x12345678), "%#010RX32");
    CHECKVAL(u32Out, UINT32_C(0x78563412), "%#010RX32");
    u32Out = ASMByteSwapU32(u32Out);
    CHECKVAL(u32Out, u32In, "%#010RX32");
    u32In  = 0;
    u32Out = ASMByteSwapU32(u32In);
    CHECKVAL(u32Out, u32In, "%#010RX32");
    u32In  = UINT32_MAX;
    u32Out = ASMByteSwapU32(u32In);
    CHECKVAL(u32Out, u32In, "%#010RX32");

    uint16_t u16In  = UINT16_C(0x0011);
    uint16_t u16Out = ASMByteSwapU16(u16In);
    CHECKVAL(u16In, UINT16_C(0x0011), "%#06RX16");
    CHECKVAL(u16Out, UINT16_C(0x1100), "%#06RX16");
    u16Out = ASMByteSwapU16(u16Out);
    CHECKVAL(u16Out, u16In, "%#06RX16");
    u16In  = UINT16_C(0x1234);
    u16Out = ASMByteSwapU16(u16In);
    CHECKVAL(u16In, UINT16_C(0x1234), "%#06RX16");
    CHECKVAL(u16Out, UINT16_C(0x3412), "%#06RX16");
    u16Out = ASMByteSwapU16(u16Out);
    CHECKVAL(u16Out, u16In, "%#06RX16");
    u16In  = 0;
    u16Out = ASMByteSwapU16(u16In);
    CHECKVAL(u16Out, u16In, "%#06RX16");
    u16In  = UINT16_MAX;
    u16Out = ASMByteSwapU16(u16In);
    CHECKVAL(u16Out, u16In, "%#06RX16");
}


void tstASMBench(void)
{
    /*
     * Make this static. We don't want to have this located on the stack.
     */
    static uint8_t  volatile s_u8;
    static int8_t   volatile s_i8;
    static uint16_t volatile s_u16;
    static int16_t  volatile s_i16;
    static uint32_t volatile s_u32;
    static int32_t  volatile s_i32;
    static uint64_t volatile s_u64;
    static int64_t  volatile s_i64;
    register unsigned i;
    const unsigned cRounds = _2M;       /* Must be multiple of 8 */
    register uint64_t u64Elapsed;

    RTTestSub(g_hTest, "Benchmarking");

#if 0 && !defined(GCC44_32BIT_PIC) && (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86))
# define BENCH(op, str) \
    do { \
        RTThreadYield(); \
        u64Elapsed = ASMReadTSC(); \
        for (i = cRounds; i > 0; i--) \
            op; \
        u64Elapsed = ASMReadTSC() - u64Elapsed; \
        RTTestValue(g_hTest, str, u64Elapsed / cRounds, RTTESTUNIT_TICKS_PER_CALL); \
    } while (0)
#else
# define BENCH(op, str) \
    do { \
        RTThreadYield(); \
        u64Elapsed = RTTimeNanoTS(); \
        for (i = cRounds / 8; i > 0; i--) \
        { \
            op; \
            op; \
            op; \
            op; \
            op; \
            op; \
            op; \
            op; \
        } \
        u64Elapsed = RTTimeNanoTS() - u64Elapsed; \
        RTTestValue(g_hTest, str, u64Elapsed / cRounds, RTTESTUNIT_NS_PER_CALL); \
    } while (0)
#endif
#if (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86)) && !defined(GCC44_32BIT_PIC)
# define BENCH_TSC(op, str) \
    do { \
        RTThreadYield(); \
        u64Elapsed = ASMReadTSC(); \
        for (i = cRounds / 8; i > 0; i--) \
        { \
            op; \
            op; \
            op; \
            op; \
            op; \
            op; \
            op; \
            op; \
        } \
        u64Elapsed = ASMReadTSC() - u64Elapsed; \
        RTTestValue(g_hTest, str, u64Elapsed / cRounds, /*RTTESTUNIT_TICKS_PER_CALL*/ RTTESTUNIT_NONE); \
    } while (0)
#else
# define BENCH_TSC(op, str) BENCH(op, str)
#endif

    BENCH(s_u32 = 0,                             "s_u32 = 0");
    BENCH(ASMAtomicUoReadU8(&s_u8),              "ASMAtomicUoReadU8");
    BENCH(ASMAtomicUoReadS8(&s_i8),              "ASMAtomicUoReadS8");
    BENCH(ASMAtomicUoReadU16(&s_u16),            "ASMAtomicUoReadU16");
    BENCH(ASMAtomicUoReadS16(&s_i16),            "ASMAtomicUoReadS16");
    BENCH(ASMAtomicUoReadU32(&s_u32),            "ASMAtomicUoReadU32");
    BENCH(ASMAtomicUoReadS32(&s_i32),            "ASMAtomicUoReadS32");
    BENCH(ASMAtomicUoReadU64(&s_u64),            "ASMAtomicUoReadU64");
    BENCH(ASMAtomicUoReadS64(&s_i64),            "ASMAtomicUoReadS64");
    BENCH(ASMAtomicReadU8(&s_u8),                "ASMAtomicReadU8");
    BENCH(ASMAtomicReadS8(&s_i8),                "ASMAtomicReadS8");
    BENCH(ASMAtomicReadU16(&s_u16),              "ASMAtomicReadU16");
    BENCH(ASMAtomicReadS16(&s_i16),              "ASMAtomicReadS16");
    BENCH(ASMAtomicReadU32(&s_u32),              "ASMAtomicReadU32");
    BENCH(ASMAtomicReadS32(&s_i32),              "ASMAtomicReadS32");
    BENCH(ASMAtomicReadU64(&s_u64),              "ASMAtomicReadU64");
    BENCH(ASMAtomicReadS64(&s_i64),              "ASMAtomicReadS64");
    BENCH(ASMAtomicUoWriteU8(&s_u8, 0),          "ASMAtomicUoWriteU8");
    BENCH(ASMAtomicUoWriteS8(&s_i8, 0),          "ASMAtomicUoWriteS8");
    BENCH(ASMAtomicUoWriteU16(&s_u16, 0),        "ASMAtomicUoWriteU16");
    BENCH(ASMAtomicUoWriteS16(&s_i16, 0),        "ASMAtomicUoWriteS16");
    BENCH(ASMAtomicUoWriteU32(&s_u32, 0),        "ASMAtomicUoWriteU32");
    BENCH(ASMAtomicUoWriteS32(&s_i32, 0),        "ASMAtomicUoWriteS32");
    BENCH(ASMAtomicUoWriteU64(&s_u64, 0),        "ASMAtomicUoWriteU64");
    BENCH(ASMAtomicUoWriteS64(&s_i64, 0),        "ASMAtomicUoWriteS64");
    BENCH(ASMAtomicWriteU8(&s_u8, 0),            "ASMAtomicWriteU8");
    BENCH(ASMAtomicWriteS8(&s_i8, 0),            "ASMAtomicWriteS8");
    BENCH(ASMAtomicWriteU16(&s_u16, 0),          "ASMAtomicWriteU16");
    BENCH(ASMAtomicWriteS16(&s_i16, 0),          "ASMAtomicWriteS16");
    BENCH(ASMAtomicWriteU32(&s_u32, 0),          "ASMAtomicWriteU32");
    BENCH(ASMAtomicWriteS32(&s_i32, 0),          "ASMAtomicWriteS32");
    BENCH(ASMAtomicWriteU64(&s_u64, 0),          "ASMAtomicWriteU64");
    BENCH(ASMAtomicWriteS64(&s_i64, 0),          "ASMAtomicWriteS64");
    BENCH(ASMAtomicXchgU8(&s_u8, 0),             "ASMAtomicXchgU8");
    BENCH(ASMAtomicXchgS8(&s_i8, 0),             "ASMAtomicXchgS8");
    BENCH(ASMAtomicXchgU16(&s_u16, 0),           "ASMAtomicXchgU16");
    BENCH(ASMAtomicXchgS16(&s_i16, 0),           "ASMAtomicXchgS16");
    BENCH(ASMAtomicXchgU32(&s_u32, 0),           "ASMAtomicXchgU32");
    BENCH(ASMAtomicXchgS32(&s_i32, 0),           "ASMAtomicXchgS32");
    BENCH(ASMAtomicXchgU64(&s_u64, 0),           "ASMAtomicXchgU64");
    BENCH(ASMAtomicXchgS64(&s_i64, 0),           "ASMAtomicXchgS64");
    BENCH(ASMAtomicCmpXchgU32(&s_u32, 0, 0),     "ASMAtomicCmpXchgU32");
    BENCH(ASMAtomicCmpXchgS32(&s_i32, 0, 0),     "ASMAtomicCmpXchgS32");
    BENCH(ASMAtomicCmpXchgU64(&s_u64, 0, 0),     "ASMAtomicCmpXchgU64");
    BENCH(ASMAtomicCmpXchgS64(&s_i64, 0, 0),     "ASMAtomicCmpXchgS64");
    BENCH(ASMAtomicCmpXchgU32(&s_u32, 0, 1),     "ASMAtomicCmpXchgU32/neg");
    BENCH(ASMAtomicCmpXchgS32(&s_i32, 0, 1),     "ASMAtomicCmpXchgS32/neg");
    BENCH(ASMAtomicCmpXchgU64(&s_u64, 0, 1),     "ASMAtomicCmpXchgU64/neg");
    BENCH(ASMAtomicCmpXchgS64(&s_i64, 0, 1),     "ASMAtomicCmpXchgS64/neg");
    BENCH(ASMAtomicIncU32(&s_u32),               "ASMAtomicIncU32");
    BENCH(ASMAtomicIncS32(&s_i32),               "ASMAtomicIncS32");
    BENCH(ASMAtomicDecU32(&s_u32),               "ASMAtomicDecU32");
    BENCH(ASMAtomicDecS32(&s_i32),               "ASMAtomicDecS32");
    BENCH(ASMAtomicAddU32(&s_u32, 5),            "ASMAtomicAddU32");
    BENCH(ASMAtomicAddS32(&s_i32, 5),            "ASMAtomicAddS32");
    BENCH(ASMAtomicUoIncU32(&s_u32),             "ASMAtomicUoIncU32");
    BENCH(ASMAtomicUoDecU32(&s_u32),             "ASMAtomicUoDecU32");
    BENCH(ASMAtomicUoAndU32(&s_u32, 0xffffffff), "ASMAtomicUoAndU32");
    BENCH(ASMAtomicUoOrU32(&s_u32, 0xffffffff),  "ASMAtomicUoOrU32");
    BENCH_TSC(ASMSerializeInstructionCpuId(),    "ASMSerializeInstructionCpuId");
    BENCH_TSC(ASMSerializeInstructionIRet(),     "ASMSerializeInstructionIRet");

    /* The Darwin gcc does not like this ... */
#if !defined(RT_OS_DARWIN) && !defined(GCC44_32BIT_PIC) && (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86))
    BENCH(s_u8 = ASMGetApicId(),                "ASMGetApicId");
#endif
#if !defined(GCC44_32BIT_PIC) && (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86))
    uint32_t uAux;
    if (   ASMHasCpuId()
        && ASMIsValidExtRange(ASMCpuId_EAX(0x80000000))
        && (ASMCpuId_EDX(0x80000001) & X86_CPUID_EXT_FEATURE_EDX_RDTSCP) )
    {
        BENCH_TSC(ASMSerializeInstructionRdTscp(), "ASMSerializeInstructionRdTscp");
        BENCH(s_u64 = ASMReadTscWithAux(&uAux),  "ASMReadTscWithAux");
    }
    BENCH(s_u64 = ASMReadTSC(),                 "ASMReadTSC");
    union
    {
        uint64_t    u64[2];
        RTIDTR      Unaligned;
        struct
        {
            uint16_t abPadding[3];
            RTIDTR   Aligned;
        } s;
    } uBuf;
    Assert(((uintptr_t)&uBuf.Unaligned.pIdt & (sizeof(uintptr_t) - 1)) != 0);
    BENCH(ASMGetIDTR(&uBuf.Unaligned),            "ASMGetIDTR/unaligned");
    Assert(((uintptr_t)&uBuf.s.Aligned.pIdt & (sizeof(uintptr_t) - 1)) == 0);
    BENCH(ASMGetIDTR(&uBuf.s.Aligned),            "ASMGetIDTR/aligned");
#endif

#undef BENCH
}


int main(int argc, char **argv)
{
    RT_NOREF_PV(argc); RT_NOREF_PV(argv);

    int rc = RTTestInitAndCreate("tstRTInlineAsm", &g_hTest);
    if (rc)
        return rc;
    RTTestBanner(g_hTest);

    /*
     * Execute the tests.
     */
#if !defined(GCC44_32BIT_PIC) && (defined(RT_ARCH_AMD64) || defined(RT_ARCH_X86))
    tstASMCpuId();
    //bruteForceCpuId();
#endif
#if 1
    tstASMAtomicXchgU8();
    tstASMAtomicXchgU16();
    tstASMAtomicXchgU32();
    tstASMAtomicXchgU64();
    tstASMAtomicXchgPtr();
    tstASMAtomicCmpXchgU8();
    tstASMAtomicCmpXchgU32();
    tstASMAtomicCmpXchgU64();
    tstASMAtomicCmpXchgExU32();
    tstASMAtomicCmpXchgExU64();
    tstASMAtomicReadU64();
    tstASMAtomicUoReadU64();

    tstASMAtomicAddS32();
    tstASMAtomicAddS64();
    tstASMAtomicDecIncS32();
    tstASMAtomicDecIncS64();
    tstASMAtomicAndOrU32();
    tstASMAtomicAndOrU64();

    tstASMAtomicUoIncU32();
    tstASMAtomicUoDecU32();
    tstASMAtomicUoAndOrU32();

    tstASMMemZeroPage();
    tstASMMemIsZeroPage(g_hTest);
    tstASMMemFirstMismatchingU8(g_hTest);
    tstASMMemZero32();
    tstASMMemFill32();

    tstASMMath();

    tstASMByteSwap();

    tstASMBench();
#endif

    /*
     * Show the result.
     */
    return RTTestSummaryAndDestroy(g_hTest);
}

