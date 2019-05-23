/* $Id: bs3-cpu-basic-2-x0.c $ */
/** @file
 * BS3Kit - bs3-cpu-basic-2, C test driver code (16-bit).
 */

/*
 * Copyright (C) 2007-2017 Oracle Corporation
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
#define BS3_USE_X0_TEXT_SEG
#include <bs3kit.h>
#include <iprt/asm.h>
#include <iprt/asm-amd64-x86.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#undef  CHECK_MEMBER
#define CHECK_MEMBER(a_szName, a_szFmt, a_Actual, a_Expected) \
    do \
    { \
        if ((a_Actual) == (a_Expected)) { /* likely */ } \
        else bs3CpuBasic2_FailedF(a_szName "=" a_szFmt " expected " a_szFmt, (a_Actual), (a_Expected)); \
    } while (0)


/** Indicating that we've got operand size prefix and that it matters. */
#define BS3CB2SIDTSGDT_F_OPSIZE    UINT8_C(0x01)
/** Worker requires 386 or later. */
#define BS3CB2SIDTSGDT_F_386PLUS   UINT8_C(0x02)


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
typedef struct BS3CB2INVLDESCTYPE
{
    uint8_t u4Type;
    uint8_t u1DescType;
} BS3CB2INVLDESCTYPE;

typedef struct BS3CB2SIDTSGDT
{
    const char *pszDesc;
    FPFNBS3FAR  fpfnWorker;
    uint8_t     cbInstr;
    bool        fSs;
    uint8_t     bMode;
    uint8_t     fFlags;
} BS3CB2SIDTSGDT;


/*********************************************************************************************************************************
*   External Symbols                                                                                                             *
*********************************************************************************************************************************/
extern FNBS3FAR     bs3CpuBasic2_Int80;
extern FNBS3FAR     bs3CpuBasic2_Int81;
extern FNBS3FAR     bs3CpuBasic2_Int82;
extern FNBS3FAR     bs3CpuBasic2_Int83;

extern FNBS3FAR     bs3CpuBasic2_ud2;
#define             g_bs3CpuBasic2_ud2_FlatAddr BS3_DATA_NM(g_bs3CpuBasic2_ud2_FlatAddr)
extern uint32_t     g_bs3CpuBasic2_ud2_FlatAddr;

extern FNBS3FAR     bs3CpuBasic2_iret;
extern FNBS3FAR     bs3CpuBasic2_iret_opsize;
extern FNBS3FAR     bs3CpuBasic2_iret_rexw;

extern FNBS3FAR     bs3CpuBasic2_sidt_bx_ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_sidt_bx_ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_sidt_bx_ud2_c64;
extern FNBS3FAR     bs3CpuBasic2_sidt_ss_bx_ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_sidt_ss_bx_ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_sidt_rexw_bx_ud2_c64;
extern FNBS3FAR     bs3CpuBasic2_sidt_opsize_bx_ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_sidt_opsize_bx_ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_sidt_opsize_bx_ud2_c64;
extern FNBS3FAR     bs3CpuBasic2_sidt_opsize_ss_bx_ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_sidt_opsize_ss_bx_ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_sidt_opsize_rexw_bx_ud2_c64;

extern FNBS3FAR     bs3CpuBasic2_sgdt_bx_ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_sgdt_bx_ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_sgdt_bx_ud2_c64;
extern FNBS3FAR     bs3CpuBasic2_sgdt_ss_bx_ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_sgdt_ss_bx_ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_sgdt_rexw_bx_ud2_c64;
extern FNBS3FAR     bs3CpuBasic2_sgdt_opsize_bx_ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_sgdt_opsize_bx_ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_sgdt_opsize_bx_ud2_c64;
extern FNBS3FAR     bs3CpuBasic2_sgdt_opsize_ss_bx_ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_sgdt_opsize_ss_bx_ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_sgdt_opsize_rexw_bx_ud2_c64;

extern FNBS3FAR     bs3CpuBasic2_lidt_bx__sidt_es_di__lidt_es_si__ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_lidt_bx__sidt_es_di__lidt_es_si__ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_lidt_bx__sidt_es_di__lidt_es_si__ud2_c64;
extern FNBS3FAR     bs3CpuBasic2_lidt_ss_bx__sidt_es_di__lidt_es_si__ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_lidt_ss_bx__sidt_es_di__lidt_es_si__ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_lidt_rexw_bx__sidt_es_di__lidt_es_si__ud2_c64;
extern FNBS3FAR     bs3CpuBasic2_lidt_opsize_bx__sidt_es_di__lidt_es_si__ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_lidt_opsize_bx__sidt32_es_di__lidt_es_si__ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_lidt_opsize_bx__sidt_es_di__lidt_es_si__ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_lidt_opsize_bx__sidt_es_di__lidt_es_si__ud2_c64;
extern FNBS3FAR     bs3CpuBasic2_lidt_opsize_ss_bx__sidt_es_di__lidt_es_si__ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_lidt_opsize_ss_bx__sidt_es_di__lidt_es_si__ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_lidt_opsize_rexw_bx__sidt_es_di__lidt_es_si__ud2_c64;

extern FNBS3FAR     bs3CpuBasic2_lgdt_bx__sgdt_es_di__lgdt_es_si__ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_lgdt_bx__sgdt_es_di__lgdt_es_si__ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_lgdt_bx__sgdt_es_di__lgdt_es_si__ud2_c64;
extern FNBS3FAR     bs3CpuBasic2_lgdt_ss_bx__sgdt_es_di__lgdt_es_si__ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_lgdt_ss_bx__sgdt_es_di__lgdt_es_si__ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_lgdt_rexw_bx__sgdt_es_di__lgdt_es_si__ud2_c64;
extern FNBS3FAR     bs3CpuBasic2_lgdt_opsize_bx__sgdt_es_di__lgdt_es_si__ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_lgdt_opsize_bx__sgdt_es_di__lgdt_es_si__ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_lgdt_opsize_bx__sgdt_es_di__lgdt_es_si__ud2_c64;
extern FNBS3FAR     bs3CpuBasic2_lgdt_opsize_ss_bx__sgdt_es_di__lgdt_es_si__ud2_c16;
extern FNBS3FAR     bs3CpuBasic2_lgdt_opsize_ss_bx__sgdt_es_di__lgdt_es_si__ud2_c32;
extern FNBS3FAR     bs3CpuBasic2_lgdt_opsize_rexw_bx__sgdt_es_di__lgdt_es_si__ud2_c64;



/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static const char BS3_FAR  *g_pszTestMode = (const char *)1;
static uint8_t              g_bTestMode = 1;
static bool                 g_f16BitSys = 1;


/** SIDT test workers. */
static BS3CB2SIDTSGDT const g_aSidtWorkers[] =
{
    { "sidt [bx]",              bs3CpuBasic2_sidt_bx_ud2_c16,             3, false,   BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, 0 },
    { "sidt [ss:bx]",           bs3CpuBasic2_sidt_ss_bx_ud2_c16,          4, true,    BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, 0 },
    { "o32 sidt [bx]",          bs3CpuBasic2_sidt_opsize_bx_ud2_c16,      4, false,   BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, BS3CB2SIDTSGDT_F_386PLUS },
    { "o32 sidt [ss:bx]",       bs3CpuBasic2_sidt_opsize_ss_bx_ud2_c16,   5, true,    BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, BS3CB2SIDTSGDT_F_386PLUS },
    { "sidt [ebx]",             bs3CpuBasic2_sidt_bx_ud2_c32,             3, false,   BS3_MODE_CODE_32, 0 },
    { "sidt [ss:ebx]",          bs3CpuBasic2_sidt_ss_bx_ud2_c32,          4, true,    BS3_MODE_CODE_32, 0 },
    { "o16 sidt [ebx]",         bs3CpuBasic2_sidt_opsize_bx_ud2_c32,      4, false,   BS3_MODE_CODE_32, 0 },
    { "o16 sidt [ss:ebx]",      bs3CpuBasic2_sidt_opsize_ss_bx_ud2_c32,   5, true,    BS3_MODE_CODE_32, 0 },
    { "sidt [rbx]",             bs3CpuBasic2_sidt_bx_ud2_c64,             3, false,   BS3_MODE_CODE_64, 0 },
    { "o64 sidt [rbx]",         bs3CpuBasic2_sidt_rexw_bx_ud2_c64,        4, false,   BS3_MODE_CODE_64, 0 },
    { "o32 sidt [rbx]",         bs3CpuBasic2_sidt_opsize_bx_ud2_c64,      4, false,   BS3_MODE_CODE_64, 0 },
    { "o32 o64 sidt [rbx]",     bs3CpuBasic2_sidt_opsize_rexw_bx_ud2_c64, 5, false,   BS3_MODE_CODE_64, 0 },
};

/** SGDT test workers. */
static BS3CB2SIDTSGDT const g_aSgdtWorkers[] =
{
    { "sgdt [bx]",              bs3CpuBasic2_sgdt_bx_ud2_c16,             3, false,   BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, 0 },
    { "sgdt [ss:bx]",           bs3CpuBasic2_sgdt_ss_bx_ud2_c16,          4, true,    BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, 0 },
    { "o32 sgdt [bx]",          bs3CpuBasic2_sgdt_opsize_bx_ud2_c16,      4, false,   BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, BS3CB2SIDTSGDT_F_386PLUS },
    { "o32 sgdt [ss:bx]",       bs3CpuBasic2_sgdt_opsize_ss_bx_ud2_c16,   5, true,    BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, BS3CB2SIDTSGDT_F_386PLUS },
    { "sgdt [ebx]",             bs3CpuBasic2_sgdt_bx_ud2_c32,             3, false,   BS3_MODE_CODE_32, 0 },
    { "sgdt [ss:ebx]",          bs3CpuBasic2_sgdt_ss_bx_ud2_c32,          4, true,    BS3_MODE_CODE_32, 0 },
    { "o16 sgdt [ebx]",         bs3CpuBasic2_sgdt_opsize_bx_ud2_c32,      4, false,   BS3_MODE_CODE_32, 0 },
    { "o16 sgdt [ss:ebx]",      bs3CpuBasic2_sgdt_opsize_ss_bx_ud2_c32,   5, true,    BS3_MODE_CODE_32, 0 },
    { "sgdt [rbx]",             bs3CpuBasic2_sgdt_bx_ud2_c64,             3, false,   BS3_MODE_CODE_64, 0 },
    { "o64 sgdt [rbx]",         bs3CpuBasic2_sgdt_rexw_bx_ud2_c64,        4, false,   BS3_MODE_CODE_64, 0 },
    { "o32 sgdt [rbx]",         bs3CpuBasic2_sgdt_opsize_bx_ud2_c64,      4, false,   BS3_MODE_CODE_64, 0 },
    { "o32 o64 sgdt [rbx]",     bs3CpuBasic2_sgdt_opsize_rexw_bx_ud2_c64, 5, false,   BS3_MODE_CODE_64, 0 },
};

/** LIDT test workers. */
static BS3CB2SIDTSGDT const g_aLidtWorkers[] =
{
    { "lidt [bx]",              bs3CpuBasic2_lidt_bx__sidt_es_di__lidt_es_si__ud2_c16,             11, false,   BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, 0 },
    { "lidt [ss:bx]",           bs3CpuBasic2_lidt_ss_bx__sidt_es_di__lidt_es_si__ud2_c16,          12, true,    BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, 0 },
    { "o32 lidt [bx]",          bs3CpuBasic2_lidt_opsize_bx__sidt_es_di__lidt_es_si__ud2_c16,      12, false,   BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, BS3CB2SIDTSGDT_F_OPSIZE | BS3CB2SIDTSGDT_F_386PLUS },
    { "o32 lidt [bx]; sidt32",  bs3CpuBasic2_lidt_opsize_bx__sidt32_es_di__lidt_es_si__ud2_c16,    27, false,   BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, BS3CB2SIDTSGDT_F_OPSIZE | BS3CB2SIDTSGDT_F_386PLUS },
    { "o32 lidt [ss:bx]",       bs3CpuBasic2_lidt_opsize_ss_bx__sidt_es_di__lidt_es_si__ud2_c16,   13, true,    BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, BS3CB2SIDTSGDT_F_OPSIZE | BS3CB2SIDTSGDT_F_386PLUS },
    { "lidt [ebx]",             bs3CpuBasic2_lidt_bx__sidt_es_di__lidt_es_si__ud2_c32,             11, false,   BS3_MODE_CODE_32, 0 },
    { "lidt [ss:ebx]",          bs3CpuBasic2_lidt_ss_bx__sidt_es_di__lidt_es_si__ud2_c32,          12, true,    BS3_MODE_CODE_32, 0 },
    { "o16 lidt [ebx]",         bs3CpuBasic2_lidt_opsize_bx__sidt_es_di__lidt_es_si__ud2_c32,      12, false,   BS3_MODE_CODE_32, BS3CB2SIDTSGDT_F_OPSIZE },
    { "o16 lidt [ss:ebx]",      bs3CpuBasic2_lidt_opsize_ss_bx__sidt_es_di__lidt_es_si__ud2_c32,   13, true,    BS3_MODE_CODE_32, BS3CB2SIDTSGDT_F_OPSIZE },
    { "lidt [rbx]",             bs3CpuBasic2_lidt_bx__sidt_es_di__lidt_es_si__ud2_c64,              9, false,   BS3_MODE_CODE_64, 0 },
    { "o64 lidt [rbx]",         bs3CpuBasic2_lidt_rexw_bx__sidt_es_di__lidt_es_si__ud2_c64,        10, false,   BS3_MODE_CODE_64, 0 },
    { "o32 lidt [rbx]",         bs3CpuBasic2_lidt_opsize_bx__sidt_es_di__lidt_es_si__ud2_c64,      10, false,   BS3_MODE_CODE_64, 0 },
    { "o32 o64 lidt [rbx]",     bs3CpuBasic2_lidt_opsize_rexw_bx__sidt_es_di__lidt_es_si__ud2_c64, 11, false,   BS3_MODE_CODE_64, 0 },
};

/** LGDT test workers. */
static BS3CB2SIDTSGDT const g_aLgdtWorkers[] =
{
    { "lgdt [bx]",              bs3CpuBasic2_lgdt_bx__sgdt_es_di__lgdt_es_si__ud2_c16,             11, false,   BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, 0 },
    { "lgdt [ss:bx]",           bs3CpuBasic2_lgdt_ss_bx__sgdt_es_di__lgdt_es_si__ud2_c16,          12, true,    BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, 0 },
    { "o32 lgdt [bx]",          bs3CpuBasic2_lgdt_opsize_bx__sgdt_es_di__lgdt_es_si__ud2_c16,      12, false,   BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, BS3CB2SIDTSGDT_F_OPSIZE | BS3CB2SIDTSGDT_F_386PLUS },
    { "o32 lgdt [ss:bx]",       bs3CpuBasic2_lgdt_opsize_ss_bx__sgdt_es_di__lgdt_es_si__ud2_c16,   13, true,    BS3_MODE_CODE_16 | BS3_MODE_CODE_V86, BS3CB2SIDTSGDT_F_OPSIZE | BS3CB2SIDTSGDT_F_386PLUS },
    { "lgdt [ebx]",             bs3CpuBasic2_lgdt_bx__sgdt_es_di__lgdt_es_si__ud2_c32,             11, false,   BS3_MODE_CODE_32, 0 },
    { "lgdt [ss:ebx]",          bs3CpuBasic2_lgdt_ss_bx__sgdt_es_di__lgdt_es_si__ud2_c32,          12, true,    BS3_MODE_CODE_32, 0 },
    { "o16 lgdt [ebx]",         bs3CpuBasic2_lgdt_opsize_bx__sgdt_es_di__lgdt_es_si__ud2_c32,      12, false,   BS3_MODE_CODE_32, BS3CB2SIDTSGDT_F_OPSIZE },
    { "o16 lgdt [ss:ebx]",      bs3CpuBasic2_lgdt_opsize_ss_bx__sgdt_es_di__lgdt_es_si__ud2_c32,   13, true,    BS3_MODE_CODE_32, BS3CB2SIDTSGDT_F_OPSIZE },
    { "lgdt [rbx]",             bs3CpuBasic2_lgdt_bx__sgdt_es_di__lgdt_es_si__ud2_c64,              9, false,   BS3_MODE_CODE_64, 0 },
    { "o64 lgdt [rbx]",         bs3CpuBasic2_lgdt_rexw_bx__sgdt_es_di__lgdt_es_si__ud2_c64,        10, false,   BS3_MODE_CODE_64, 0 },
    { "o32 lgdt [rbx]",         bs3CpuBasic2_lgdt_opsize_bx__sgdt_es_di__lgdt_es_si__ud2_c64,      10, false,   BS3_MODE_CODE_64, 0 },
    { "o32 o64 lgdt [rbx]",     bs3CpuBasic2_lgdt_opsize_rexw_bx__sgdt_es_di__lgdt_es_si__ud2_c64, 11, false,   BS3_MODE_CODE_64, 0 },
};



#if 0
/** Table containing invalid CS selector types. */
static const BS3CB2INVLDESCTYPE g_aInvalidCsTypes[] =
{
    {   X86_SEL_TYPE_RO,            1 },
    {   X86_SEL_TYPE_RO_ACC,        1 },
    {   X86_SEL_TYPE_RW,            1 },
    {   X86_SEL_TYPE_RW_ACC,        1 },
    {   X86_SEL_TYPE_RO_DOWN,       1 },
    {   X86_SEL_TYPE_RO_DOWN_ACC,   1 },
    {   X86_SEL_TYPE_RW_DOWN,       1 },
    {   X86_SEL_TYPE_RW_DOWN_ACC,   1 },
    {   0,                          0 },
    {   1,                          0 },
    {   2,                          0 },
    {   3,                          0 },
    {   4,                          0 },
    {   5,                          0 },
    {   6,                          0 },
    {   7,                          0 },
    {   8,                          0 },
    {   9,                          0 },
    {   10,                         0 },
    {   11,                         0 },
    {   12,                         0 },
    {   13,                         0 },
    {   14,                         0 },
    {   15,                         0 },
};

/** Table containing invalid SS selector types. */
static const BS3CB2INVLDESCTYPE g_aInvalidSsTypes[] =
{
    {   X86_SEL_TYPE_EO,            1 },
    {   X86_SEL_TYPE_EO_ACC,        1 },
    {   X86_SEL_TYPE_ER,            1 },
    {   X86_SEL_TYPE_ER_ACC,        1 },
    {   X86_SEL_TYPE_EO_CONF,       1 },
    {   X86_SEL_TYPE_EO_CONF_ACC,   1 },
    {   X86_SEL_TYPE_ER_CONF,       1 },
    {   X86_SEL_TYPE_ER_CONF_ACC,   1 },
    {   0,                          0 },
    {   1,                          0 },
    {   2,                          0 },
    {   3,                          0 },
    {   4,                          0 },
    {   5,                          0 },
    {   6,                          0 },
    {   7,                          0 },
    {   8,                          0 },
    {   9,                          0 },
    {   10,                         0 },
    {   11,                         0 },
    {   12,                         0 },
    {   13,                         0 },
    {   14,                         0 },
    {   15,                         0 },
};
#endif


/**
 * Sets globals according to the mode.
 *
 * @param   bTestMode   The test mode.
 */
static void bs3CpuBasic2_SetGlobals(uint8_t bTestMode)
{
    g_bTestMode     = bTestMode;
    g_pszTestMode   = Bs3GetModeName(bTestMode);
    g_f16BitSys     = BS3_MODE_IS_16BIT_SYS(bTestMode);
    g_usBs3TestStep = 0;
}


/**
 * Wrapper around Bs3TestFailedF that prefixes the error with g_usBs3TestStep
 * and g_pszTestMode.
 */
static void bs3CpuBasic2_FailedF(const char *pszFormat, ...)
{
    va_list va;

    char szTmp[168];
    va_start(va, pszFormat);
    Bs3StrPrintfV(szTmp, sizeof(szTmp), pszFormat, va);
    va_end(va);

    Bs3TestFailedF("%u - %s: %s", g_usBs3TestStep, g_pszTestMode, szTmp);
}


#if 0
/**
 * Compares trap stuff.
 */
static void bs3CpuBasic2_CompareIntCtx1(PCBS3TRAPFRAME pTrapCtx, PCBS3REGCTX pStartCtx, uint8_t bXcpt)
{
    uint16_t const cErrorsBefore = Bs3TestSubErrorCount();
    CHECK_MEMBER("bXcpt",   "%#04x",    pTrapCtx->bXcpt,        bXcpt);
    CHECK_MEMBER("bErrCd",  "%#06RX64", pTrapCtx->uErrCd,       0);
    Bs3TestCheckRegCtxEx(&pTrapCtx->Ctx, pStartCtx, 2 /*int xx*/, 0 /*cbSpAdjust*/, 0 /*fExtraEfl*/, g_pszTestMode, g_usBs3TestStep);
    if (Bs3TestSubErrorCount() != cErrorsBefore)
    {
        Bs3TrapPrintFrame(pTrapCtx);
#if 1
        Bs3TestPrintf("Halting: g_uBs3CpuDetected=%#x\n", g_uBs3CpuDetected);
        Bs3TestPrintf("Halting in CompareTrapCtx1: bXcpt=%#x\n", bXcpt);
        ASMHalt();
#endif
    }
}
#endif


#if 0
/**
 * Compares trap stuff.
 */
static void bs3CpuBasic2_CompareTrapCtx2(PCBS3TRAPFRAME pTrapCtx, PCBS3REGCTX pStartCtx, uint16_t cbIpAdjust,
                                         uint8_t bXcpt, uint16_t uHandlerCs)
{
    uint16_t const cErrorsBefore = Bs3TestSubErrorCount();
    CHECK_MEMBER("bXcpt",   "%#04x",    pTrapCtx->bXcpt,        bXcpt);
    CHECK_MEMBER("bErrCd",  "%#06RX64", pTrapCtx->uErrCd,       0);
    CHECK_MEMBER("uHandlerCs", "%#06x", pTrapCtx->uHandlerCs,   uHandlerCs);
    Bs3TestCheckRegCtxEx(&pTrapCtx->Ctx, pStartCtx, cbIpAdjust, 0 /*cbSpAdjust*/, 0 /*fExtraEfl*/, g_pszTestMode, g_usBs3TestStep);
    if (Bs3TestSubErrorCount() != cErrorsBefore)
    {
        Bs3TrapPrintFrame(pTrapCtx);
#if 1
        Bs3TestPrintf("Halting: g_uBs3CpuDetected=%#x\n", g_uBs3CpuDetected);
        Bs3TestPrintf("Halting in CompareTrapCtx2: bXcpt=%#x\n", bXcpt);
        ASMHalt();
#endif
    }
}
#endif

/**
 * Compares a CPU trap.
 */
static void bs3CpuBasic2_CompareCpuTrapCtx(PCBS3TRAPFRAME pTrapCtx, PCBS3REGCTX pStartCtx, uint16_t uErrCd,
                                           uint8_t bXcpt, bool f486ResumeFlagHint)
{
    uint16_t const cErrorsBefore = Bs3TestSubErrorCount();
    uint32_t fExtraEfl;

    CHECK_MEMBER("bXcpt",   "%#04x",    pTrapCtx->bXcpt,        bXcpt);
    CHECK_MEMBER("bErrCd",  "%#06RX16", (uint16_t)pTrapCtx->uErrCd, (uint16_t)uErrCd); /* 486 only writes a word */

    fExtraEfl = X86_EFL_RF;
    if (   g_f16BitSys
        || (   !f486ResumeFlagHint
            && (g_uBs3CpuDetected & BS3CPU_TYPE_MASK) <= BS3CPU_80486 ) )
        fExtraEfl = 0;
    else
        fExtraEfl = X86_EFL_RF;
#if 0 /** @todo Running on an AMD Phenom II X6 1100T under AMD-V I'm not getting good X86_EFL_RF results.  Enable this to get on with other work.  */
    fExtraEfl = pTrapCtx->Ctx.rflags.u32 & X86_EFL_RF;
#endif
    Bs3TestCheckRegCtxEx(&pTrapCtx->Ctx, pStartCtx, 0 /*cbIpAdjust*/, 0 /*cbSpAdjust*/, fExtraEfl, g_pszTestMode, g_usBs3TestStep);
    if (Bs3TestSubErrorCount() != cErrorsBefore)
    {
        Bs3TrapPrintFrame(pTrapCtx);
#if 1
        Bs3TestPrintf("Halting: g_uBs3CpuDetected=%#x\n", g_uBs3CpuDetected);
        Bs3TestPrintf("Halting: bXcpt=%#x uErrCd=%#x\n", bXcpt, uErrCd);
        ASMHalt();
#endif
    }
}


/**
 * Compares \#GP trap.
 */
static void bs3CpuBasic2_CompareGpCtx(PCBS3TRAPFRAME pTrapCtx, PCBS3REGCTX pStartCtx, uint16_t uErrCd)
{
    bs3CpuBasic2_CompareCpuTrapCtx(pTrapCtx, pStartCtx, uErrCd, X86_XCPT_GP, true /*f486ResumeFlagHint*/);
}

#if 0
/**
 * Compares \#NP trap.
 */
static void bs3CpuBasic2_CompareNpCtx(PCBS3TRAPFRAME pTrapCtx, PCBS3REGCTX pStartCtx, uint16_t uErrCd)
{
    bs3CpuBasic2_CompareCpuTrapCtx(pTrapCtx, pStartCtx, uErrCd, X86_XCPT_NP, true /*f486ResumeFlagHint*/);
}
#endif

/**
 * Compares \#SS trap.
 */
static void bs3CpuBasic2_CompareSsCtx(PCBS3TRAPFRAME pTrapCtx, PCBS3REGCTX pStartCtx, uint16_t uErrCd, bool f486ResumeFlagHint)
{
    bs3CpuBasic2_CompareCpuTrapCtx(pTrapCtx, pStartCtx, uErrCd, X86_XCPT_SS, f486ResumeFlagHint);
}

#if 0
/**
 * Compares \#TS trap.
 */
static void bs3CpuBasic2_CompareTsCtx(PCBS3TRAPFRAME pTrapCtx, PCBS3REGCTX pStartCtx, uint16_t uErrCd)
{
    bs3CpuBasic2_CompareCpuTrapCtx(pTrapCtx, pStartCtx, uErrCd, X86_XCPT_TS, false /*f486ResumeFlagHint*/);
}
#endif

/**
 * Compares \#PF trap.
 */
static void bs3CpuBasic2_ComparePfCtx(PCBS3TRAPFRAME pTrapCtx, PBS3REGCTX pStartCtx, uint16_t uErrCd, uint64_t uCr2Expected)
{
    uint64_t const uCr2Saved     = pStartCtx->cr2.u;
    pStartCtx->cr2.u = uCr2Expected;
    bs3CpuBasic2_CompareCpuTrapCtx(pTrapCtx, pStartCtx, uErrCd, X86_XCPT_PF, true /*f486ResumeFlagHint*/);
    pStartCtx->cr2.u = uCr2Saved;
}

/**
 * Compares \#UD trap.
 */
static void bs3CpuBasic2_CompareUdCtx(PCBS3TRAPFRAME pTrapCtx, PCBS3REGCTX pStartCtx)
{
    bs3CpuBasic2_CompareCpuTrapCtx(pTrapCtx, pStartCtx, 0 /*no error code*/, X86_XCPT_UD, true /*f486ResumeFlagHint*/);
}


#if 0 /* convert me */
static void bs3CpuBasic2_RaiseXcpt1Common(uint16_t const uSysR0Cs, uint16_t const uSysR0CsConf, uint16_t const uSysR0Ss,
                                          PX86DESC const paIdt, unsigned const cIdteShift)
{
    BS3TRAPFRAME    TrapCtx;
    BS3REGCTX       Ctx80;
    BS3REGCTX       Ctx81;
    BS3REGCTX       Ctx82;
    BS3REGCTX       Ctx83;
    BS3REGCTX       CtxTmp;
    BS3REGCTX       CtxTmp2;
    PBS3REGCTX      apCtx8x[4];
    unsigned        iCtx;
    unsigned        iRing;
    unsigned        iDpl;
    unsigned        iRpl;
    unsigned        i, j, k;
    uint32_t        uExpected;
    bool const      f486Plus = (g_uBs3CpuDetected & BS3CPU_TYPE_MASK) >= BS3CPU_80486;
# if TMPL_BITS == 16
    bool const      f386Plus = (g_uBs3CpuDetected & BS3CPU_TYPE_MASK) >= BS3CPU_80386;
    bool const      f286     = (g_uBs3CpuDetected & BS3CPU_TYPE_MASK) == BS3CPU_80286;
# else
    bool const      f286     = false;
    bool const      f386Plus = true;
    int             rc;
    uint8_t        *pbIdtCopyAlloc;
    PX86DESC        pIdtCopy;
    const unsigned  cbIdte = 1 << (3 + cIdteShift);
    RTCCUINTXREG    uCr0Saved = ASMGetCR0();
    RTGDTR          GdtrSaved;
# endif
    RTIDTR          IdtrSaved;
    RTIDTR          Idtr;

    ASMGetIDTR(&IdtrSaved);
# if TMPL_BITS != 16
    ASMGetGDTR(&GdtrSaved);
# endif

    /* make sure they're allocated  */
    Bs3MemZero(&TrapCtx, sizeof(TrapCtx));
    Bs3MemZero(&Ctx80, sizeof(Ctx80));
    Bs3MemZero(&Ctx81, sizeof(Ctx81));
    Bs3MemZero(&Ctx82, sizeof(Ctx82));
    Bs3MemZero(&Ctx83, sizeof(Ctx83));
    Bs3MemZero(&CtxTmp, sizeof(CtxTmp));
    Bs3MemZero(&CtxTmp2, sizeof(CtxTmp2));

    /* Context array. */
    apCtx8x[0] = &Ctx80;
    apCtx8x[1] = &Ctx81;
    apCtx8x[2] = &Ctx82;
    apCtx8x[3] = &Ctx83;

# if TMPL_BITS != 16
    /* Allocate memory for playing around with the IDT. */
    pbIdtCopyAlloc = NULL;
    if (BS3_MODE_IS_PAGED(g_bTestMode))
        pbIdtCopyAlloc = Bs3MemAlloc(BS3MEMKIND_FLAT32, 12*_1K);
# endif

    /*
     * IDT entry 80 thru 83 are assigned DPLs according to the number.
     * (We'll be useing more, but this'll do for now.)
     */
    paIdt[0x80 << cIdteShift].Gate.u2Dpl = 0;
    paIdt[0x81 << cIdteShift].Gate.u2Dpl = 1;
    paIdt[0x82 << cIdteShift].Gate.u2Dpl = 2;
    paIdt[0x83 << cIdteShift].Gate.u2Dpl = 3;

    Bs3RegCtxSave(&Ctx80);
    Ctx80.rsp.u -= 0x300;
    Ctx80.rip.u  = (uintptr_t)BS3_FP_OFF(&bs3CpuBasic2_Int80);
# if TMPL_BITS == 16
    Ctx80.cs = BS3_MODE_IS_RM_OR_V86(g_bTestMode) ? BS3_SEL_TEXT16 : BS3_SEL_R0_CS16;
# elif TMPL_BITS == 32
    g_uBs3TrapEipHint = Ctx80.rip.u32;
# endif
    Bs3MemCpy(&Ctx81, &Ctx80, sizeof(Ctx80));
    Ctx81.rip.u  = (uintptr_t)BS3_FP_OFF(&bs3CpuBasic2_Int81);
    Bs3MemCpy(&Ctx82, &Ctx80, sizeof(Ctx80));
    Ctx82.rip.u  = (uintptr_t)BS3_FP_OFF(&bs3CpuBasic2_Int82);
    Bs3MemCpy(&Ctx83, &Ctx80, sizeof(Ctx80));
    Ctx83.rip.u  = (uintptr_t)BS3_FP_OFF(&bs3CpuBasic2_Int83);

    /*
     * Check that all the above gates work from ring-0.
     */
    for (iCtx = 0; iCtx < RT_ELEMENTS(apCtx8x); iCtx++)
    {
        g_usBs3TestStep = iCtx;
# if TMPL_BITS == 32
        g_uBs3TrapEipHint = apCtx8x[iCtx]->rip.u32;
# endif
        Bs3TrapSetJmpAndRestore(apCtx8x[iCtx], &TrapCtx);
        bs3CpuBasic2_CompareIntCtx1(&TrapCtx, apCtx8x[iCtx], 0x80+iCtx /*bXcpt*/);
    }

    /*
     * Check that the gate DPL checks works.
     */
    g_usBs3TestStep = 100;
    for (iRing = 0; iRing <= 3; iRing++)
    {
        for (iCtx = 0; iCtx < RT_ELEMENTS(apCtx8x); iCtx++)
        {
            Bs3MemCpy(&CtxTmp, apCtx8x[iCtx], sizeof(CtxTmp));
            Bs3RegCtxConvertToRingX(&CtxTmp, iRing);
# if TMPL_BITS == 32
            g_uBs3TrapEipHint = CtxTmp.rip.u32;
# endif
            Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
            if (iCtx < iRing)
                bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, ((0x80 + iCtx) << X86_TRAP_ERR_SEL_SHIFT) | X86_TRAP_ERR_IDT);
            else
                bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &CtxTmp, 0x80 + iCtx /*bXcpt*/);
            g_usBs3TestStep++;
        }
    }

    /*
     * Modify the gate CS value and run the handler at a different CPL.
     * Throw RPL variations into the mix (completely ignored) together
     * with gate presence.
     *      1. CPL <= GATE.DPL
     *      2. GATE.P
     *      3. GATE.CS.DPL <= CPL (non-conforming segments)
     */
    g_usBs3TestStep = 1000;
    for (i = 0; i <= 3; i++)
    {
        for (iRing = 0; iRing <= 3; iRing++)
        {
            for (iCtx = 0; iCtx < RT_ELEMENTS(apCtx8x); iCtx++)
            {
# if TMPL_BITS == 32
                g_uBs3TrapEipHint = apCtx8x[iCtx]->rip.u32;
# endif
                Bs3MemCpy(&CtxTmp, apCtx8x[iCtx], sizeof(CtxTmp));
                Bs3RegCtxConvertToRingX(&CtxTmp, iRing);

                for (j = 0; j <= 3; j++)
                {
                    uint16_t const uCs = (uSysR0Cs | j) + (i << BS3_SEL_RING_SHIFT);
                    for (k = 0; k < 2; k++)
                    {
                        g_usBs3TestStep++;
                        /*Bs3TestPrintf("g_usBs3TestStep=%u iCtx=%u iRing=%u i=%u uCs=%04x\n", g_usBs3TestStep,  iCtx,  iRing, i, uCs);*/
                        paIdt[(0x80 + iCtx) << cIdteShift].Gate.u16Sel = uCs;
                        paIdt[(0x80 + iCtx) << cIdteShift].Gate.u1Present = k;
                        Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
                        /*Bs3TrapPrintFrame(&TrapCtx);*/
                        if (iCtx < iRing)
                            bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, ((0x80 + iCtx) << X86_TRAP_ERR_SEL_SHIFT) | X86_TRAP_ERR_IDT);
                        else if (k == 0)
                            bs3CpuBasic2_CompareNpCtx(&TrapCtx, &CtxTmp, ((0x80 + iCtx) << X86_TRAP_ERR_SEL_SHIFT) | X86_TRAP_ERR_IDT);
                        else if (i > iRing)
                            bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, uCs & X86_SEL_MASK_OFF_RPL);
                        else
                        {
                            uint16_t uExpectedCs = uCs & X86_SEL_MASK_OFF_RPL;
                            if (i <= iCtx && i <= iRing)
                                uExpectedCs |= i;
                            bs3CpuBasic2_CompareTrapCtx2(&TrapCtx, &CtxTmp, 2 /*int 8xh*/, 0x80 + iCtx /*bXcpt*/, uExpectedCs);
                        }
                    }
                }

                paIdt[(0x80 + iCtx) << cIdteShift].Gate.u16Sel = uSysR0Cs;
                paIdt[(0x80 + iCtx) << cIdteShift].Gate.u1Present = 1;
            }
        }
    }
    BS3_ASSERT(g_usBs3TestStep < 1600);

    /*
     * Various CS and SS related faults
     *
     * We temporarily reconfigure gate 80 and 83 with new CS selectors, the
     * latter have a CS.DPL of 2 for testing ring transisions and SS loading
     * without making it impossible to handle faults.
     */
    g_usBs3TestStep = 1600;
    Bs3GdteTestPage00 = Bs3Gdt[uSysR0Cs >> X86_SEL_SHIFT];
    Bs3GdteTestPage00.Gen.u1Present = 0;
    Bs3GdteTestPage00.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED;
    paIdt[0x80 << cIdteShift].Gate.u16Sel = BS3_SEL_TEST_PAGE_00;

    /* CS.PRESENT = 0 */
    Bs3TrapSetJmpAndRestore(&Ctx80, &TrapCtx);
    bs3CpuBasic2_CompareNpCtx(&TrapCtx, &Ctx80, BS3_SEL_TEST_PAGE_00);
    if (Bs3GdteTestPage00.Gen.u4Type & X86_SEL_TYPE_ACCESSED)
        bs3CpuBasic2_FailedF("selector was accessed");
    g_usBs3TestStep++;

    /* Check that GATE.DPL is checked before CS.PRESENT. */
    for (iRing = 1; iRing < 4; iRing++)
    {
        Bs3MemCpy(&CtxTmp, &Ctx80, sizeof(CtxTmp));
        Bs3RegCtxConvertToRingX(&CtxTmp, iRing);
        Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, (0x80 << X86_TRAP_ERR_SEL_SHIFT) | X86_TRAP_ERR_IDT);
        if (Bs3GdteTestPage00.Gen.u4Type & X86_SEL_TYPE_ACCESSED)
            bs3CpuBasic2_FailedF("selector was accessed");
        g_usBs3TestStep++;
    }

    /* CS.DPL mismatch takes precedence over CS.PRESENT = 0. */
    Bs3GdteTestPage00.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED;
    Bs3TrapSetJmpAndRestore(&Ctx80, &TrapCtx);
    bs3CpuBasic2_CompareNpCtx(&TrapCtx, &Ctx80, BS3_SEL_TEST_PAGE_00);
    if (Bs3GdteTestPage00.Gen.u4Type & X86_SEL_TYPE_ACCESSED)
        bs3CpuBasic2_FailedF("CS selector was accessed");
    g_usBs3TestStep++;
    for (iDpl = 1; iDpl < 4; iDpl++)
    {
        Bs3GdteTestPage00.Gen.u2Dpl = iDpl;
        Bs3TrapSetJmpAndRestore(&Ctx80, &TrapCtx);
        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx80, BS3_SEL_TEST_PAGE_00);
        if (Bs3GdteTestPage00.Gen.u4Type & X86_SEL_TYPE_ACCESSED)
            bs3CpuBasic2_FailedF("CS selector was accessed");
        g_usBs3TestStep++;
    }

    /* 1608: Check all the invalid CS selector types alone. */
    Bs3GdteTestPage00 = Bs3Gdt[uSysR0Cs >> X86_SEL_SHIFT];
    for (i = 0; i < RT_ELEMENTS(g_aInvalidCsTypes); i++)
    {
        Bs3GdteTestPage00.Gen.u4Type     = g_aInvalidCsTypes[i].u4Type;
        Bs3GdteTestPage00.Gen.u1DescType = g_aInvalidCsTypes[i].u1DescType;
        Bs3TrapSetJmpAndRestore(&Ctx80, &TrapCtx);
        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx80, BS3_SEL_TEST_PAGE_00);
        if (Bs3GdteTestPage00.Gen.u4Type != g_aInvalidCsTypes[i].u4Type)
            bs3CpuBasic2_FailedF("Invalid CS type %#x/%u -> %#x/%u\n",
                                 g_aInvalidCsTypes[i].u4Type, g_aInvalidCsTypes[i].u1DescType,
                                 Bs3GdteTestPage00.Gen.u4Type, Bs3GdteTestPage00.Gen.u1DescType);
        g_usBs3TestStep++;

        /* Incorrect CS.TYPE takes precedence over CS.PRESENT = 0. */
        Bs3GdteTestPage00.Gen.u1Present = 0;
        Bs3TrapSetJmpAndRestore(&Ctx80, &TrapCtx);
        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx80, BS3_SEL_TEST_PAGE_00);
        Bs3GdteTestPage00.Gen.u1Present = 1;
        g_usBs3TestStep++;
    }

    /* Fix CS again. */
    Bs3GdteTestPage00 = Bs3Gdt[uSysR0Cs >> X86_SEL_SHIFT];

    /* 1632: Test SS. */
    if (!BS3_MODE_IS_64BIT_SYS(g_bTestMode))
    {
        uint16_t BS3_FAR *puTssSs2    = BS3_MODE_IS_16BIT_SYS(g_bTestMode) ? &Bs3Tss16.ss2 : &Bs3Tss32.ss2;
        uint16_t const    uSavedSs2   = *puTssSs2;
        X86DESC const     SavedGate83 = paIdt[0x83 << cIdteShift];

        /* Make the handler execute in ring-2. */
        Bs3GdteTestPage02 = Bs3Gdt[(uSysR0Cs + (2 << BS3_SEL_RING_SHIFT)) >> X86_SEL_SHIFT];
        Bs3GdteTestPage02.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED;
        paIdt[0x83 << cIdteShift].Gate.u16Sel = BS3_SEL_TEST_PAGE_02 | 2;

        Bs3MemCpy(&CtxTmp, &Ctx83, sizeof(CtxTmp));
        Bs3RegCtxConvertToRingX(&CtxTmp, 3); /* yeah, from 3 so SS:xSP is reloaded. */
        Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
        bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &CtxTmp, 0x83);
        if (!(Bs3GdteTestPage02.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
            bs3CpuBasic2_FailedF("CS selector was not access");
        g_usBs3TestStep++;

        /* Create a SS.DPL=2 stack segment and check that SS2.RPL matters and
           that we get #SS if the selector isn't present. */
        i = 0; /* used for cycling thru invalid CS types */
        for (k = 0; k < 10; k++)
        {
            /* k=0: present,
               k=1: not-present,
               k=2: present but very low limit,
               k=3: not-present, low limit.
               k=4: present, read-only.
               k=5: not-present, read-only.
               k=6: present, code-selector.
               k=7: not-present, code-selector.
               k=8: present, read-write / no access + system (=LDT).
               k=9: not-present, read-write / no access + system (=LDT).
               */
            Bs3GdteTestPage03 = Bs3Gdt[(uSysR0Ss + (2 << BS3_SEL_RING_SHIFT)) >> X86_SEL_SHIFT];
            Bs3GdteTestPage03.Gen.u1Present  = !(k & 1);
            if (k >= 8)
            {
                Bs3GdteTestPage03.Gen.u1DescType = 0; /* system */
                Bs3GdteTestPage03.Gen.u4Type = X86_SEL_TYPE_RW; /* = LDT */
            }
            else if (k >= 6)
                Bs3GdteTestPage03.Gen.u4Type = X86_SEL_TYPE_ER;
            else if (k >= 4)
                Bs3GdteTestPage03.Gen.u4Type = X86_SEL_TYPE_RO;
            else if (k >= 2)
            {
                Bs3GdteTestPage03.Gen.u16LimitLow   = 0x400;
                Bs3GdteTestPage03.Gen.u4LimitHigh   = 0;
                Bs3GdteTestPage03.Gen.u1Granularity = 0;
            }

            for (iDpl = 0; iDpl < 4; iDpl++)
            {
                Bs3GdteTestPage03.Gen.u2Dpl = iDpl;

                for (iRpl = 0; iRpl < 4; iRpl++)
                {
                    *puTssSs2 = BS3_SEL_TEST_PAGE_03 | iRpl;
                    //Bs3TestPrintf("k=%u iDpl=%u iRpl=%u step=%u\n", k, iDpl, iRpl, g_usBs3TestStep);
                    Bs3GdteTestPage02.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED;
                    Bs3GdteTestPage03.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED;
                    Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
                    if (iRpl != 2 || iRpl != iDpl || k >= 4)
                        bs3CpuBasic2_CompareTsCtx(&TrapCtx, &CtxTmp, BS3_SEL_TEST_PAGE_03);
                    else if (k != 0)
                        bs3CpuBasic2_CompareSsCtx(&TrapCtx, &CtxTmp, BS3_SEL_TEST_PAGE_03,
                                                  k == 2 /*f486ResumeFlagHint*/);
                    else
                    {
                        bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &CtxTmp, 0x83);
                        if (TrapCtx.uHandlerSs != (BS3_SEL_TEST_PAGE_03 | 2))
                            bs3CpuBasic2_FailedF("uHandlerSs=%#x expected %#x\n", TrapCtx.uHandlerSs, BS3_SEL_TEST_PAGE_03 | 2);
                    }
                    if (!(Bs3GdteTestPage02.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
                        bs3CpuBasic2_FailedF("CS selector was not access");
                    if (   TrapCtx.bXcpt == 0x83
                        || (TrapCtx.bXcpt == X86_XCPT_SS && k == 2) )
                    {
                        if (!(Bs3GdteTestPage03.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
                            bs3CpuBasic2_FailedF("SS selector was not accessed");
                    }
                    else if (Bs3GdteTestPage03.Gen.u4Type & X86_SEL_TYPE_ACCESSED)
                        bs3CpuBasic2_FailedF("SS selector was accessed");
                    g_usBs3TestStep++;

                    /* +1: Modify the gate DPL to check that this is checked before SS.DPL and SS.PRESENT. */
                    paIdt[0x83 << cIdteShift].Gate.u2Dpl = 2;
                    Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, (0x83 << X86_TRAP_ERR_SEL_SHIFT) | X86_TRAP_ERR_IDT);
                    paIdt[0x83 << cIdteShift].Gate.u2Dpl = 3;
                    g_usBs3TestStep++;

                    /* +2: Check the CS.DPL check is done before the SS ones. Restoring the
                           ring-0 INT 83 context triggers the CS.DPL < CPL check. */
                    Bs3TrapSetJmpAndRestore(&Ctx83, &TrapCtx);
                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx83, BS3_SEL_TEST_PAGE_02);
                    g_usBs3TestStep++;

                    /* +3: Now mark the CS selector not present and check that that also triggers before SS stuff. */
                    Bs3GdteTestPage02.Gen.u1Present = 0;
                    Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
                    bs3CpuBasic2_CompareNpCtx(&TrapCtx, &CtxTmp, BS3_SEL_TEST_PAGE_02);
                    Bs3GdteTestPage02.Gen.u1Present = 1;
                    g_usBs3TestStep++;

                    /* +4: Make the CS selector some invalid type and check it triggers before SS stuff. */
                    Bs3GdteTestPage02.Gen.u4Type = g_aInvalidCsTypes[i].u4Type;
                    Bs3GdteTestPage02.Gen.u1DescType = g_aInvalidCsTypes[i].u1DescType;
                    Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, BS3_SEL_TEST_PAGE_02);
                    Bs3GdteTestPage02.Gen.u4Type = X86_SEL_TYPE_ER_ACC;
                    Bs3GdteTestPage02.Gen.u1DescType = 1;
                    g_usBs3TestStep++;

                    /* +5: Now, make the CS selector limit too small and that it triggers after SS trouble.
                           The 286 had a simpler approach to these GP(0). */
                    Bs3GdteTestPage02.Gen.u16LimitLow = 0;
                    Bs3GdteTestPage02.Gen.u4LimitHigh = 0;
                    Bs3GdteTestPage02.Gen.u1Granularity = 0;
                    Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
                    if (f286)
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, 0 /*uErrCd*/);
                    else if (iRpl != 2 || iRpl != iDpl || k >= 4)
                        bs3CpuBasic2_CompareTsCtx(&TrapCtx, &CtxTmp, BS3_SEL_TEST_PAGE_03);
                    else if (k != 0)
                        bs3CpuBasic2_CompareSsCtx(&TrapCtx, &CtxTmp, BS3_SEL_TEST_PAGE_03, k == 2 /*f486ResumeFlagHint*/);
                    else
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, 0 /*uErrCd*/);
                    Bs3GdteTestPage02 = Bs3Gdt[(uSysR0Cs + (2 << BS3_SEL_RING_SHIFT)) >> X86_SEL_SHIFT];
                    g_usBs3TestStep++;
                }
            }
        }

        /* Check all the invalid SS selector types alone. */
        Bs3GdteTestPage02 = Bs3Gdt[(uSysR0Cs + (2 << BS3_SEL_RING_SHIFT)) >> X86_SEL_SHIFT];
        Bs3GdteTestPage03 = Bs3Gdt[(uSysR0Ss + (2 << BS3_SEL_RING_SHIFT)) >> X86_SEL_SHIFT];
        *puTssSs2 = BS3_SEL_TEST_PAGE_03 | 2;
        Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
        bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &CtxTmp, 0x83);
        g_usBs3TestStep++;
        for (i = 0; i < RT_ELEMENTS(g_aInvalidSsTypes); i++)
        {
            Bs3GdteTestPage03.Gen.u4Type     = g_aInvalidSsTypes[i].u4Type;
            Bs3GdteTestPage03.Gen.u1DescType = g_aInvalidSsTypes[i].u1DescType;
            Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
            bs3CpuBasic2_CompareTsCtx(&TrapCtx, &CtxTmp, BS3_SEL_TEST_PAGE_03);
            if (Bs3GdteTestPage03.Gen.u4Type != g_aInvalidSsTypes[i].u4Type)
                bs3CpuBasic2_FailedF("Invalid SS type %#x/%u -> %#x/%u\n",
                                     g_aInvalidSsTypes[i].u4Type, g_aInvalidSsTypes[i].u1DescType,
                                     Bs3GdteTestPage03.Gen.u4Type, Bs3GdteTestPage03.Gen.u1DescType);
            g_usBs3TestStep++;
        }

        /*
         * Continue the SS experiments with a expand down segment.  We'll use
         * the same setup as we already have with gate 83h being DPL and
         * having CS.DPL=2.
         *
         * Expand down segments are weird. The valid area is practically speaking
         * reversed.  So, a 16-bit segment with a limit of 0x6000 will have valid
         * addresses from 0xffff thru 0x6001.
         *
         * So, with expand down segments we can more easily cut partially into the
         * pushing of the iret frame and trigger more interesting behavior than
         * with regular "expand up" segments where the whole pushing area is either
         * all fine or not not fine.
         */
        Bs3GdteTestPage02 = Bs3Gdt[(uSysR0Cs + (2 << BS3_SEL_RING_SHIFT)) >> X86_SEL_SHIFT];
        Bs3GdteTestPage03 = Bs3Gdt[(uSysR0Ss + (2 << BS3_SEL_RING_SHIFT)) >> X86_SEL_SHIFT];
        Bs3GdteTestPage03.Gen.u2Dpl = 2;
        Bs3GdteTestPage03.Gen.u4Type = X86_SEL_TYPE_RW_DOWN;
        *puTssSs2 = BS3_SEL_TEST_PAGE_03 | 2;

        /* First test, limit = max --> no bytes accessible --> #GP */
        Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
        bs3CpuBasic2_CompareSsCtx(&TrapCtx, &CtxTmp, BS3_SEL_TEST_PAGE_03, true /*f486ResumeFlagHint*/);

        /* Second test, limit = 0 --> all by zero byte accessible --> works */
        Bs3GdteTestPage03.Gen.u16LimitLow = 0;
        Bs3GdteTestPage03.Gen.u4LimitHigh = 0;
        Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
        bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &CtxTmp, 0x83);

        /* Modify the gate handler to be a dummy that immediately does UD2
           and triggers #UD, then advance the limit down till we get the #UD. */
        Bs3GdteTestPage03.Gen.u1Granularity = 0;

        Bs3MemCpy(&CtxTmp2, &CtxTmp, sizeof(CtxTmp2));  /* #UD result context */
        if (g_f16BitSys)
        {
            CtxTmp2.rip.u = g_bs3CpuBasic2_ud2_FlatAddr - BS3_ADDR_BS3TEXT16;
            Bs3Trap16SetGate(0x83, X86_SEL_TYPE_SYS_286_INT_GATE, 3, BS3_SEL_TEST_PAGE_02, CtxTmp2.rip.u16, 0 /*cParams*/);
            CtxTmp2.rsp.u = Bs3Tss16.sp2 - 2*5;
        }
        else
        {
            CtxTmp2.rip.u = g_bs3CpuBasic2_ud2_FlatAddr;
            Bs3Trap32SetGate(0x83, X86_SEL_TYPE_SYS_386_INT_GATE, 3, BS3_SEL_TEST_PAGE_02, CtxTmp2.rip.u32, 0 /*cParams*/);
            CtxTmp2.rsp.u = Bs3Tss32.esp2 - 4*5;
        }
        CtxTmp2.bMode = g_bTestMode; /* g_bBs3CurrentMode not changed by the UD2 handler. */
        CtxTmp2.cs = BS3_SEL_TEST_PAGE_02 | 2;
        CtxTmp2.ss = BS3_SEL_TEST_PAGE_03 | 2;
        CtxTmp2.bCpl = 2;

        /* test run. */
        Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
        bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxTmp2);
        g_usBs3TestStep++;

        /* Real run. */
        i = (g_f16BitSys ? 2 : 4) * 6 + 1;
        while (i-- > 0)
        {
            Bs3GdteTestPage03.Gen.u16LimitLow = CtxTmp2.rsp.u16 + i - 1;
            Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
            if (i > 0)
                bs3CpuBasic2_CompareSsCtx(&TrapCtx, &CtxTmp, BS3_SEL_TEST_PAGE_03, true /*f486ResumeFlagHint*/);
            else
                bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxTmp2);
            g_usBs3TestStep++;
        }

        /* Do a run where we do the same-ring kind of access.  */
        Bs3RegCtxConvertToRingX(&CtxTmp, 2);
        if (g_f16BitSys)
        {
            CtxTmp2.rsp.u32 = CtxTmp.rsp.u32 - 2*3;
            i = 2*3 - 1;
        }
        else
        {
            CtxTmp2.rsp.u32 = CtxTmp.rsp.u32 - 4*3;
            i = 4*3 - 1;
        }
        CtxTmp.ss = BS3_SEL_TEST_PAGE_03 | 2;
        CtxTmp2.ds = CtxTmp.ds;
        CtxTmp2.es = CtxTmp.es;
        CtxTmp2.fs = CtxTmp.fs;
        CtxTmp2.gs = CtxTmp.gs;
        while (i-- > 0)
        {
            Bs3GdteTestPage03.Gen.u16LimitLow = CtxTmp2.rsp.u16 + i - 1;
            Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
            if (i > 0)
                bs3CpuBasic2_CompareSsCtx(&TrapCtx, &CtxTmp, 0 /*BS3_SEL_TEST_PAGE_03*/, true /*f486ResumeFlagHint*/);
            else
                bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxTmp2);
            g_usBs3TestStep++;
        }

        *puTssSs2 = uSavedSs2;
        paIdt[0x83 << cIdteShift] = SavedGate83;
    }
    paIdt[0x80 << cIdteShift].Gate.u16Sel = uSysR0Cs;
    BS3_ASSERT(g_usBs3TestStep < 3000);

    /*
     * Modify the gate CS value with a conforming segment.
     */
    g_usBs3TestStep = 3000;
    for (i = 0; i <= 3; i++) /* cs.dpl */
    {
        for (iRing = 0; iRing <= 3; iRing++)
        {
            for (iCtx = 0; iCtx < RT_ELEMENTS(apCtx8x); iCtx++)
            {
                Bs3MemCpy(&CtxTmp, apCtx8x[iCtx], sizeof(CtxTmp));
                Bs3RegCtxConvertToRingX(&CtxTmp, iRing);
# if TMPL_BITS == 32
                g_uBs3TrapEipHint = CtxTmp.rip.u32;
# endif

                for (j = 0; j <= 3; j++) /* rpl */
                {
                    uint16_t const uCs = (uSysR0CsConf | j) + (i << BS3_SEL_RING_SHIFT);
                    /*Bs3TestPrintf("g_usBs3TestStep=%u iCtx=%u iRing=%u i=%u uCs=%04x\n", g_usBs3TestStep,  iCtx,  iRing, i, uCs);*/
                    paIdt[(0x80 + iCtx) << cIdteShift].Gate.u16Sel = uCs;
                    Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
                    //Bs3TestPrintf("%u/%u/%u/%u: cs=%04x hcs=%04x xcpt=%02x\n", i, iRing, iCtx, j, uCs, TrapCtx.uHandlerCs, TrapCtx.bXcpt);
                    /*Bs3TrapPrintFrame(&TrapCtx);*/
                    g_usBs3TestStep++;
                    if (iCtx < iRing)
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, ((0x80 + iCtx) << X86_TRAP_ERR_SEL_SHIFT) | X86_TRAP_ERR_IDT);
                    else if (i > iRing)
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, uCs & X86_SEL_MASK_OFF_RPL);
                    else
                        bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &CtxTmp, 0x80 + iCtx /*bXcpt*/);
                }
                paIdt[(0x80 + iCtx) << cIdteShift].Gate.u16Sel = uSysR0Cs;
            }
        }
    }
    BS3_ASSERT(g_usBs3TestStep < 3500);

    /*
     * The gates must be 64-bit in long mode.
     */
    if (cIdteShift != 0)
    {
        g_usBs3TestStep = 3500;
        for (i = 0; i <= 3; i++)
        {
            for (iRing = 0; iRing <= 3; iRing++)
            {
                for (iCtx = 0; iCtx < RT_ELEMENTS(apCtx8x); iCtx++)
                {
                    Bs3MemCpy(&CtxTmp, apCtx8x[iCtx], sizeof(CtxTmp));
                    Bs3RegCtxConvertToRingX(&CtxTmp, iRing);

                    for (j = 0; j < 2; j++)
                    {
                        static const uint16_t s_auCSes[2] = { BS3_SEL_R0_CS16, BS3_SEL_R0_CS32 };
                        uint16_t uCs = (s_auCSes[j] | i) + (i << BS3_SEL_RING_SHIFT);
                        g_usBs3TestStep++;
                        /*Bs3TestPrintf("g_usBs3TestStep=%u iCtx=%u iRing=%u i=%u uCs=%04x\n", g_usBs3TestStep,  iCtx,  iRing, i, uCs);*/
                        paIdt[(0x80 + iCtx) << cIdteShift].Gate.u16Sel = uCs;
                        Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
                        /*Bs3TrapPrintFrame(&TrapCtx);*/
                        if (iCtx < iRing)
                            bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, ((0x80 + iCtx) << X86_TRAP_ERR_SEL_SHIFT) | X86_TRAP_ERR_IDT);
                        else
                            bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, uCs & X86_SEL_MASK_OFF_RPL);
                    }
                    paIdt[(0x80 + iCtx) << cIdteShift].Gate.u16Sel = uSysR0Cs;
                }
            }
        }
        BS3_ASSERT(g_usBs3TestStep < 4000);
    }

    /*
     * IDT limit check.  The 286 does not access X86DESCGATE::u16OffsetHigh.
     */
    g_usBs3TestStep = 5000;
    i = (0x80 << (cIdteShift + 3)) - 1;
    j = (0x82 << (cIdteShift + 3)) - (!f286 ? 1 : 3);
    k = (0x83 << (cIdteShift + 3)) - 1;
    for (; i <= k; i++, g_usBs3TestStep++)
    {
        Idtr = IdtrSaved;
        Idtr.cbIdt  = i;
        ASMSetIDTR(&Idtr);
        Bs3TrapSetJmpAndRestore(&Ctx81, &TrapCtx);
        if (i < j)
            bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx81, (0x81 << X86_TRAP_ERR_SEL_SHIFT) | X86_TRAP_ERR_IDT);
        else
            bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &Ctx81, 0x81 /*bXcpt*/);
    }
    ASMSetIDTR(&IdtrSaved);
    BS3_ASSERT(g_usBs3TestStep < 5100);

# if TMPL_BITS != 16 /* Only do the paging related stuff in 32-bit and 64-bit modes. */

    /*
     * IDT page not present. Placing the IDT copy such that 0x80 is on the
     * first page and 0x81 is on the second page.  We need proceed to move
     * it down byte by byte to check that any inaccessible byte means #PF.
     *
     * Note! We must reload the alternative IDTR for each run as any kind of
     *       printing to the string (like error reporting) will cause a switch
     *       to real mode and back, reloading the default IDTR.
     */
    g_usBs3TestStep = 5200;
    if (BS3_MODE_IS_PAGED(g_bTestMode) && pbIdtCopyAlloc)
    {
        uint32_t const uCr2Expected = Bs3SelPtrToFlat(pbIdtCopyAlloc) + _4K;
        for (j = 0; j < cbIdte; j++)
        {
            pIdtCopy = (PX86DESC)&pbIdtCopyAlloc[_4K - cbIdte * 0x81 - j];
            Bs3MemCpy(pIdtCopy, paIdt, cbIdte * 256);

            Idtr.cbIdt = IdtrSaved.cbIdt;
            Idtr.pIdt  = Bs3SelPtrToFlat(pIdtCopy);

            ASMSetIDTR(&Idtr);
            Bs3TrapSetJmpAndRestore(&Ctx81, &TrapCtx);
            bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &Ctx81, 0x81 /*bXcpt*/);
            g_usBs3TestStep++;

            ASMSetIDTR(&Idtr);
            Bs3TrapSetJmpAndRestore(&Ctx80, &TrapCtx);
            bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &Ctx80, 0x80 /*bXcpt*/);
            g_usBs3TestStep++;

            rc = Bs3PagingProtect(uCr2Expected, _4K, 0 /*fSet*/, X86_PTE_P /*fClear*/);
            if (RT_SUCCESS(rc))
            {
                ASMSetIDTR(&Idtr);
                Bs3TrapSetJmpAndRestore(&Ctx80, &TrapCtx);
                bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &Ctx80, 0x80 /*bXcpt*/);
                g_usBs3TestStep++;

                ASMSetIDTR(&Idtr);
                Bs3TrapSetJmpAndRestore(&Ctx81, &TrapCtx);
                if (f486Plus)
                    bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx81, 0 /*uErrCd*/, uCr2Expected);
                else
                    bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx81, X86_TRAP_PF_RW /*uErrCd*/, uCr2Expected + 4 - RT_MIN(j, 4));
                g_usBs3TestStep++;

                Bs3PagingProtect(uCr2Expected, _4K, X86_PTE_P /*fSet*/, 0 /*fClear*/);

                /* Check if that the entry type is checked after the whole IDTE has been cleared for #PF. */
                pIdtCopy[0x80 << cIdteShift].Gate.u4Type = 0;
                rc = Bs3PagingProtect(uCr2Expected, _4K, 0 /*fSet*/, X86_PTE_P /*fClear*/);
                if (RT_SUCCESS(rc))
                {
                    ASMSetIDTR(&Idtr);
                    Bs3TrapSetJmpAndRestore(&Ctx81, &TrapCtx);
                    if (f486Plus)
                        bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx81, 0 /*uErrCd*/, uCr2Expected);
                    else
                        bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx81, X86_TRAP_PF_RW /*uErrCd*/, uCr2Expected + 4 - RT_MIN(j, 4));
                    g_usBs3TestStep++;

                    Bs3PagingProtect(uCr2Expected, _4K, X86_PTE_P /*fSet*/, 0 /*fClear*/);
                }
            }
            else
                Bs3TestPrintf("Bs3PagingProtectPtr: %d\n", i);

            ASMSetIDTR(&IdtrSaved);
        }
    }

    /*
     * The read/write and user/supervisor bits the IDT PTEs are irrelevant.
     */
    g_usBs3TestStep = 5300;
    if (BS3_MODE_IS_PAGED(g_bTestMode) && pbIdtCopyAlloc)
    {
        Bs3MemCpy(pbIdtCopyAlloc, paIdt, cbIdte * 256);
        Idtr.cbIdt = IdtrSaved.cbIdt;
        Idtr.pIdt  = Bs3SelPtrToFlat(pbIdtCopyAlloc);

        ASMSetIDTR(&Idtr);
        Bs3TrapSetJmpAndRestore(&Ctx81, &TrapCtx);
        bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &Ctx81, 0x81 /*bXcpt*/);
        g_usBs3TestStep++;

        rc = Bs3PagingProtect(Idtr.pIdt, _4K, 0 /*fSet*/, X86_PTE_RW | X86_PTE_US /*fClear*/);
        if (RT_SUCCESS(rc))
        {
            ASMSetIDTR(&Idtr);
            Bs3TrapSetJmpAndRestore(&Ctx81, &TrapCtx);
            bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &Ctx81, 0x81 /*bXcpt*/);
            g_usBs3TestStep++;

            Bs3PagingProtect(Idtr.pIdt, _4K, X86_PTE_RW | X86_PTE_US /*fSet*/, 0 /*fClear*/);
        }
        ASMSetIDTR(&IdtrSaved);
    }

    /*
     * Check that CS.u1Accessed is set to 1. Use the test page selector #0 and #3 together
     * with interrupt gates 80h and 83h, respectively.
     */
/** @todo Throw in SS.u1Accessed too. */
    g_usBs3TestStep = 5400;
    if (BS3_MODE_IS_PAGED(g_bTestMode) && pbIdtCopyAlloc)
    {
        Bs3GdteTestPage00 = Bs3Gdt[uSysR0Cs >> X86_SEL_SHIFT];
        Bs3GdteTestPage00.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED;
        paIdt[0x80 << cIdteShift].Gate.u16Sel   = BS3_SEL_TEST_PAGE_00;

        Bs3GdteTestPage03 = Bs3Gdt[(uSysR0Cs + (3 << BS3_SEL_RING_SHIFT)) >> X86_SEL_SHIFT];
        Bs3GdteTestPage03.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED;
        paIdt[0x83 << cIdteShift].Gate.u16Sel   = BS3_SEL_TEST_PAGE_03; /* rpl is ignored, so leave it as zero. */

        /* Check that the CS.A bit is being set on a general basis and that
           the special CS values work with out generic handler code. */
        Bs3TrapSetJmpAndRestore(&Ctx80, &TrapCtx);
        bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &Ctx80, 0x80 /*bXcpt*/);
        if (!(Bs3GdteTestPage00.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
            bs3CpuBasic2_FailedF("u4Type=%#x, not accessed", Bs3GdteTestPage00.Gen.u4Type);
        g_usBs3TestStep++;

        Bs3MemCpy(&CtxTmp, &Ctx83, sizeof(CtxTmp));
        Bs3RegCtxConvertToRingX(&CtxTmp, 3);
        Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
        bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &CtxTmp, 0x83 /*bXcpt*/);
        if (!(Bs3GdteTestPage03.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
            bs3CpuBasic2_FailedF("u4Type=%#x, not accessed!", Bs3GdteTestPage00.Gen.u4Type);
        if (TrapCtx.uHandlerCs != (BS3_SEL_TEST_PAGE_03 | 3))
            bs3CpuBasic2_FailedF("uHandlerCs=%#x, expected %#x", TrapCtx.uHandlerCs, (BS3_SEL_TEST_PAGE_03 | 3));
        g_usBs3TestStep++;

        /*
         * Now check that setting CS.u1Access to 1 does __NOT__ trigger a page
         * fault due to the RW bit being zero.
         * (We check both with with and without the WP bit if 80486.)
         */
        if ((g_uBs3CpuDetected & BS3CPU_TYPE_MASK) >= BS3CPU_80486)
            ASMSetCR0(uCr0Saved | X86_CR0_WP);

        Bs3GdteTestPage00.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED;
        Bs3GdteTestPage03.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED;
        rc = Bs3PagingProtect(GdtrSaved.pGdt + BS3_SEL_TEST_PAGE_00, 8, 0 /*fSet*/, X86_PTE_RW /*fClear*/);
        if (RT_SUCCESS(rc))
        {
            /* ring-0 handler */
            Bs3TrapSetJmpAndRestore(&Ctx80, &TrapCtx);
            bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &Ctx80, 0x80 /*bXcpt*/);
            if (!(Bs3GdteTestPage00.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
                bs3CpuBasic2_FailedF("u4Type=%#x, not accessed!", Bs3GdteTestPage00.Gen.u4Type);
            g_usBs3TestStep++;

            /* ring-3 handler */
            Bs3MemCpy(&CtxTmp, &Ctx83, sizeof(CtxTmp));
            Bs3RegCtxConvertToRingX(&CtxTmp, 3);
            Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
            bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &CtxTmp, 0x83 /*bXcpt*/);
            if (!(Bs3GdteTestPage03.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
                bs3CpuBasic2_FailedF("u4Type=%#x, not accessed!", Bs3GdteTestPage00.Gen.u4Type);
            g_usBs3TestStep++;

            /* clear WP and repeat the above. */
            if ((g_uBs3CpuDetected & BS3CPU_TYPE_MASK) >= BS3CPU_80486)
                ASMSetCR0(uCr0Saved & ~X86_CR0_WP);
            Bs3GdteTestPage00.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED; /* (No need to RW the page - ring-0, WP=0.) */
            Bs3GdteTestPage03.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED; /* (No need to RW the page - ring-0, WP=0.) */

            Bs3TrapSetJmpAndRestore(&Ctx80, &TrapCtx);
            bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &Ctx80, 0x80 /*bXcpt*/);
            if (!(Bs3GdteTestPage00.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
                bs3CpuBasic2_FailedF("u4Type=%#x, not accessed!", Bs3GdteTestPage00.Gen.u4Type);
            g_usBs3TestStep++;

            Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
            bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &CtxTmp, 0x83 /*bXcpt*/);
            if (!(Bs3GdteTestPage03.Gen.u4Type & X86_SEL_TYPE_ACCESSED))
                bs3CpuBasic2_FailedF("u4Type=%#x, not accessed!n", Bs3GdteTestPage03.Gen.u4Type);
            g_usBs3TestStep++;

            Bs3PagingProtect(GdtrSaved.pGdt + BS3_SEL_TEST_PAGE_00, 8, X86_PTE_RW /*fSet*/, 0 /*fClear*/);
        }

        ASMSetCR0(uCr0Saved);

        /*
         * While we're here, check that if the CS GDT entry is a non-present
         * page we do get a #PF with the rigth error code and CR2.
         */
        Bs3GdteTestPage00.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED; /* Just for fun, really a pointless gesture. */
        Bs3GdteTestPage03.Gen.u4Type &= ~X86_SEL_TYPE_ACCESSED;
        rc = Bs3PagingProtect(GdtrSaved.pGdt + BS3_SEL_TEST_PAGE_00, 8, 0 /*fSet*/, X86_PTE_P /*fClear*/);
        if (RT_SUCCESS(rc))
        {
            Bs3TrapSetJmpAndRestore(&Ctx80, &TrapCtx);
            if (f486Plus)
                bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx80, 0 /*uErrCd*/, GdtrSaved.pGdt + BS3_SEL_TEST_PAGE_00);
            else
                bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx80, X86_TRAP_PF_RW, GdtrSaved.pGdt + BS3_SEL_TEST_PAGE_00 + 4);
            g_usBs3TestStep++;

            /* Do it from ring-3 to check ErrCd, which doesn't set X86_TRAP_PF_US it turns out. */
            Bs3MemCpy(&CtxTmp, &Ctx83, sizeof(CtxTmp));
            Bs3RegCtxConvertToRingX(&CtxTmp, 3);
            Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);

            if (f486Plus)
                bs3CpuBasic2_ComparePfCtx(&TrapCtx, &CtxTmp, 0 /*uErrCd*/, GdtrSaved.pGdt + BS3_SEL_TEST_PAGE_03);
            else
                bs3CpuBasic2_ComparePfCtx(&TrapCtx, &CtxTmp, X86_TRAP_PF_RW, GdtrSaved.pGdt + BS3_SEL_TEST_PAGE_03 + 4);
            g_usBs3TestStep++;

            Bs3PagingProtect(GdtrSaved.pGdt + BS3_SEL_TEST_PAGE_00, 8, X86_PTE_P /*fSet*/, 0 /*fClear*/);
            if (Bs3GdteTestPage00.Gen.u4Type & X86_SEL_TYPE_ACCESSED)
                bs3CpuBasic2_FailedF("u4Type=%#x, accessed! #1", Bs3GdteTestPage00.Gen.u4Type);
            if (Bs3GdteTestPage03.Gen.u4Type & X86_SEL_TYPE_ACCESSED)
                bs3CpuBasic2_FailedF("u4Type=%#x, accessed! #2", Bs3GdteTestPage03.Gen.u4Type);
        }

        /* restore */
        paIdt[0x80 << cIdteShift].Gate.u16Sel = uSysR0Cs;
        paIdt[0x83 << cIdteShift].Gate.u16Sel = uSysR0Cs;// + (3 << BS3_SEL_RING_SHIFT) + 3;
    }

# endif /* 32 || 64*/

    /*
     * Check broad EFLAGS effects.
     */
    g_usBs3TestStep = 5600;
    for (iCtx = 0; iCtx < RT_ELEMENTS(apCtx8x); iCtx++)
    {
        for (iRing = 0; iRing < 4; iRing++)
        {
            Bs3MemCpy(&CtxTmp, apCtx8x[iCtx], sizeof(CtxTmp));
            Bs3RegCtxConvertToRingX(&CtxTmp, iRing);

            /* all set */
            CtxTmp.rflags.u32 &= X86_EFL_VM | X86_EFL_1;
            CtxTmp.rflags.u32 |= X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF /* | X86_EFL_TF */ /*| X86_EFL_IF*/
                               | X86_EFL_DF | X86_EFL_OF | X86_EFL_IOPL /* | X86_EFL_NT*/;
            if (f486Plus)
                CtxTmp.rflags.u32 |= X86_EFL_AC;
            if (f486Plus && !g_f16BitSys)
                CtxTmp.rflags.u32 |= X86_EFL_RF;
            if (g_uBs3CpuDetected & BS3CPU_F_CPUID)
                CtxTmp.rflags.u32 |= X86_EFL_VIF | X86_EFL_VIP;
            Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
            CtxTmp.rflags.u32 &= ~X86_EFL_RF;

            if (iCtx >= iRing)
                bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &CtxTmp, 0x80 + iCtx /*bXcpt*/);
            else
                bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, ((0x80 + iCtx) << X86_TRAP_ERR_SEL_SHIFT) | X86_TRAP_ERR_IDT);
            uExpected = CtxTmp.rflags.u32
                      & (  X86_EFL_1 |  X86_EFL_CF | X86_EFL_PF | X86_EFL_AF | X86_EFL_ZF | X86_EFL_SF | X86_EFL_DF
                         | X86_EFL_OF | X86_EFL_IOPL | X86_EFL_NT | X86_EFL_VM | X86_EFL_AC | X86_EFL_VIF | X86_EFL_VIP
                         | X86_EFL_ID /*| X86_EFL_TF*/ /*| X86_EFL_IF*/ /*| X86_EFL_RF*/ );
            if (TrapCtx.fHandlerRfl != uExpected)
                bs3CpuBasic2_FailedF("unexpected handler rflags value: %RX64 expected %RX32; CtxTmp.rflags=%RX64 Ctx.rflags=%RX64\n",
                                     TrapCtx.fHandlerRfl, uExpected, CtxTmp.rflags.u, TrapCtx.Ctx.rflags.u);
            g_usBs3TestStep++;

            /* all cleared */
            if ((g_uBs3CpuDetected & BS3CPU_TYPE_MASK) < BS3CPU_80286)
                CtxTmp.rflags.u32 = apCtx8x[iCtx]->rflags.u32 & (X86_EFL_RA1_MASK | UINT16_C(0xf000));
            else
                CtxTmp.rflags.u32 = apCtx8x[iCtx]->rflags.u32 & (X86_EFL_VM | X86_EFL_RA1_MASK);
            Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
            if (iCtx >= iRing)
                bs3CpuBasic2_CompareIntCtx1(&TrapCtx, &CtxTmp, 0x80 + iCtx /*bXcpt*/);
            else
                bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, ((0x80 + iCtx) << X86_TRAP_ERR_SEL_SHIFT) | X86_TRAP_ERR_IDT);
            uExpected = CtxTmp.rflags.u32;
            if (TrapCtx.fHandlerRfl != uExpected)
                bs3CpuBasic2_FailedF("unexpected handler rflags value: %RX64 expected %RX32; CtxTmp.rflags=%RX64 Ctx.rflags=%RX64\n",
                                     TrapCtx.fHandlerRfl, uExpected, CtxTmp.rflags.u, TrapCtx.Ctx.rflags.u);
            g_usBs3TestStep++;
        }
    }

/** @todo CS.LIMIT / canonical(CS)  */


    /*
     * Check invalid gate types.
     */
    g_usBs3TestStep = 32000;
    for (iRing = 0; iRing <= 3; iRing++)
    {
        static const uint16_t   s_auCSes[]        = { BS3_SEL_R0_CS16, BS3_SEL_R0_CS32, BS3_SEL_R0_CS64,
                                                      BS3_SEL_TSS16, BS3_SEL_TSS32, BS3_SEL_TSS64, 0, BS3_SEL_SPARE_1f };
        static uint16_t const   s_auInvlTypes64[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13,
                                                      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                                      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };
        static uint16_t const   s_auInvlTypes32[] = { 0, 1, 2, 3, 8, 9, 10, 11, 13,
                                                      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                                      0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
                                                      /*286:*/ 12, 14, 15 };
        uint16_t const * const  pauInvTypes       = cIdteShift != 0 ? s_auInvlTypes64 : s_auInvlTypes32;
        uint16_t const          cInvTypes         = cIdteShift != 0 ? RT_ELEMENTS(s_auInvlTypes64)
                                                  : f386Plus ? RT_ELEMENTS(s_auInvlTypes32) - 3 : RT_ELEMENTS(s_auInvlTypes32);


        for (iCtx = 0; iCtx < RT_ELEMENTS(apCtx8x); iCtx++)
        {
            unsigned iType;

            Bs3MemCpy(&CtxTmp, apCtx8x[iCtx], sizeof(CtxTmp));
            Bs3RegCtxConvertToRingX(&CtxTmp, iRing);
# if TMPL_BITS == 32
            g_uBs3TrapEipHint = CtxTmp.rip.u32;
# endif
            for (iType = 0; iType < cInvTypes; iType++)
            {
                uint8_t const bSavedType = paIdt[(0x80 + iCtx) << cIdteShift].Gate.u4Type;
                paIdt[(0x80 + iCtx) << cIdteShift].Gate.u1DescType = pauInvTypes[iType] >> 4;
                paIdt[(0x80 + iCtx) << cIdteShift].Gate.u4Type     = pauInvTypes[iType] & 0xf;

                for (i = 0; i < 4; i++)
                {
                    for (j = 0; j < RT_ELEMENTS(s_auCSes); j++)
                    {
                        uint16_t uCs = (unsigned)(s_auCSes[j] - BS3_SEL_R0_FIRST) < (unsigned)(4 << BS3_SEL_RING_SHIFT)
                                     ? (s_auCSes[j] | i) + (i << BS3_SEL_RING_SHIFT)
                                     : s_auCSes[j] | i;
                        /*Bs3TestPrintf("g_usBs3TestStep=%u iCtx=%u iRing=%u i=%u uCs=%04x type=%#x\n", g_usBs3TestStep, iCtx, iRing, i, uCs, pauInvTypes[iType]);*/
                        paIdt[(0x80 + iCtx) << cIdteShift].Gate.u16Sel = uCs;
                        Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
                        g_usBs3TestStep++;
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, ((0x80 + iCtx) << X86_TRAP_ERR_SEL_SHIFT) | X86_TRAP_ERR_IDT);

                        /* Mark it not-present to check that invalid type takes precedence. */
                        paIdt[(0x80 + iCtx) << cIdteShift].Gate.u1Present = 0;
                        Bs3TrapSetJmpAndRestore(&CtxTmp, &TrapCtx);
                        g_usBs3TestStep++;
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &CtxTmp, ((0x80 + iCtx) << X86_TRAP_ERR_SEL_SHIFT) | X86_TRAP_ERR_IDT);
                        paIdt[(0x80 + iCtx) << cIdteShift].Gate.u1Present = 1;
                    }
                }

                paIdt[(0x80 + iCtx) << cIdteShift].Gate.u16Sel     = uSysR0Cs;
                paIdt[(0x80 + iCtx) << cIdteShift].Gate.u4Type     = bSavedType;
                paIdt[(0x80 + iCtx) << cIdteShift].Gate.u1DescType = 0;
                paIdt[(0x80 + iCtx) << cIdteShift].Gate.u1Present  = 1;
            }
        }
    }
    BS3_ASSERT(g_usBs3TestStep < 62000U && g_usBs3TestStep > 32000U);


    /** @todo
     *  - Run \#PF and \#GP (and others?) at CPLs other than zero.
     *  - Quickly generate all faults.
     *  - All the peculiarities v8086.
     */

# if TMPL_BITS != 16
    Bs3MemFree(pbIdtCopyAlloc, 12*_1K);
# endif
}
#endif /* convert me */


/**
 * Executes one round of SIDT and SGDT tests using one assembly worker.
 *
 * This is written with driving everything from the 16-bit or 32-bit worker in
 * mind, i.e. not assuming the test bitcount is the same as the current.
 */
static void bs3CpuBasic2_sidt_sgdt_One(BS3CB2SIDTSGDT const BS3_FAR *pWorker, uint8_t bTestMode, uint8_t bRing,
                                       uint8_t const *pbExpected)
{
    BS3TRAPFRAME        TrapCtx;
    BS3REGCTX           Ctx;
    BS3REGCTX           CtxUdExpected;
    BS3REGCTX           TmpCtx;
    uint8_t const       cbBuf = 8*2;         /* test buffer area  */
    uint8_t             abBuf[8*2 + 8 + 8];  /* test buffer w/ misalignment test space and some extra guard. */
    uint8_t BS3_FAR    *pbBuf  = abBuf;
    uint8_t const       cbIdtr = BS3_MODE_IS_64BIT_CODE(bTestMode) ? 2+8 : 2+4;
    bool const          f286   = (g_uBs3CpuDetected & BS3CPU_TYPE_MASK) == BS3CPU_80286;
    uint8_t             bFiller;
    int                 off;
    int                 off2;
    unsigned            cb;
    uint8_t BS3_FAR    *pbTest;

    /* make sure they're allocated  */
    Bs3MemZero(&Ctx, sizeof(Ctx));
    Bs3MemZero(&CtxUdExpected, sizeof(CtxUdExpected));
    Bs3MemZero(&TmpCtx, sizeof(TmpCtx));
    Bs3MemZero(&TrapCtx, sizeof(TrapCtx));
    Bs3MemZero(&abBuf, sizeof(abBuf));

    /* Create a context, give this routine some more stack space, point the context
       at our SIDT [xBX] + UD2 combo, and point DS:xBX at abBuf. */
    Bs3RegCtxSaveEx(&Ctx, bTestMode, 256 /*cbExtraStack*/);
    Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, abBuf);
    Bs3RegCtxSetRipCsFromLnkPtr(&Ctx, pWorker->fpfnWorker);
    if (BS3_MODE_IS_16BIT_SYS(bTestMode))
        g_uBs3TrapEipHint = Ctx.rip.u32;
    if (!BS3_MODE_IS_RM_OR_V86(bTestMode))
        Bs3RegCtxConvertToRingX(&Ctx, bRing);

    /* For successful SIDT attempts, we'll stop at the UD2. */
    Bs3MemCpy(&CtxUdExpected, &Ctx, sizeof(Ctx));
    CtxUdExpected.rip.u += pWorker->cbInstr;

    /*
     * Check that it works at all and that only bytes we expect gets written to.
     */
    /* First with zero buffer. */
    Bs3MemZero(abBuf, sizeof(abBuf));
    if (!ASMMemIsAllU8(abBuf, sizeof(abBuf), 0))
        Bs3TestFailedF("ASMMemIsAllU8 or Bs3MemZero is busted: abBuf=%.*Rhxs\n", sizeof(abBuf), pbBuf);
    if (!ASMMemIsZero(abBuf, sizeof(abBuf)))
        Bs3TestFailedF("ASMMemIsZero or Bs3MemZero is busted: abBuf=%.*Rhxs\n", sizeof(abBuf), pbBuf);
    Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
    bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
    if (f286 && abBuf[cbIdtr - 1] != 0xff)
        Bs3TestFailedF("286: Top base byte isn't 0xff (#1): %#x\n", abBuf[cbIdtr - 1]);
    if (!ASMMemIsZero(&abBuf[cbIdtr], cbBuf - cbIdtr))
        Bs3TestFailedF("Unexpected buffer bytes set (#1): cbIdtr=%u abBuf=%.*Rhxs\n", cbIdtr, cbBuf, pbBuf);
    if (Bs3MemCmp(abBuf, pbExpected, cbIdtr) != 0)
        Bs3TestFailedF("Mismatch (%s,#1): expected %.*Rhxs, got %.*Rhxs\n", pWorker->pszDesc, cbIdtr, pbExpected, cbIdtr, abBuf);
    g_usBs3TestStep++;

    /* Again with a buffer filled with a byte not occuring in the previous result. */
    bFiller = 0x55;
    while (Bs3MemChr(abBuf, bFiller, cbBuf) != NULL)
        bFiller++;
    Bs3MemSet(abBuf, bFiller, sizeof(abBuf));
    if (!ASMMemIsAllU8(abBuf, sizeof(abBuf), bFiller))
        Bs3TestFailedF("ASMMemIsAllU8 or Bs3MemSet is busted: bFiller=%#x abBuf=%.*Rhxs\n", bFiller, sizeof(abBuf), pbBuf);

    Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
    bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
    if (f286 && abBuf[cbIdtr - 1] != 0xff)
        Bs3TestFailedF("286: Top base byte isn't 0xff (#2): %#x\n", abBuf[cbIdtr - 1]);
    if (!ASMMemIsAllU8(&abBuf[cbIdtr], cbBuf - cbIdtr, bFiller))
        Bs3TestFailedF("Unexpected buffer bytes set (#2): cbIdtr=%u bFiller=%#x abBuf=%.*Rhxs\n", cbIdtr, bFiller, cbBuf, pbBuf);
    if (Bs3MemChr(abBuf, bFiller, cbIdtr) != NULL)
        Bs3TestFailedF("Not all bytes touched: cbIdtr=%u bFiller=%#x abBuf=%.*Rhxs\n", cbIdtr, bFiller, cbBuf, pbBuf);
    if (Bs3MemCmp(abBuf, pbExpected, cbIdtr) != 0)
        Bs3TestFailedF("Mismatch (%s,#2): expected %.*Rhxs, got %.*Rhxs\n", pWorker->pszDesc, cbIdtr, pbExpected, cbIdtr, abBuf);
    g_usBs3TestStep++;

    /*
     * Slide the buffer along 8 bytes to cover misalignment.
     */
    for (off = 0; off < 8; off++)
    {
        pbBuf = &abBuf[off];
        Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, &abBuf[off]);
        CtxUdExpected.rbx.u = Ctx.rbx.u;

        /* First with zero buffer. */
        Bs3MemZero(abBuf, sizeof(abBuf));
        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
        if (off > 0 && !ASMMemIsZero(abBuf, off))
            Bs3TestFailedF("Unexpected buffer bytes set before (#3): cbIdtr=%u off=%u abBuf=%.*Rhxs\n",
                           cbIdtr, off, off + cbBuf, abBuf);
        if (!ASMMemIsZero(&abBuf[off + cbIdtr], sizeof(abBuf) - cbIdtr - off))
            Bs3TestFailedF("Unexpected buffer bytes set after (#3): cbIdtr=%u off=%u abBuf=%.*Rhxs\n",
                           cbIdtr, off, off + cbBuf, abBuf);
        if (f286 && abBuf[off + cbIdtr - 1] != 0xff)
            Bs3TestFailedF("286: Top base byte isn't 0xff (#3): %#x\n", abBuf[off + cbIdtr - 1]);
        if (Bs3MemCmp(&abBuf[off], pbExpected, cbIdtr) != 0)
            Bs3TestFailedF("Mismatch (#3): expected %.*Rhxs, got %.*Rhxs\n", cbIdtr, pbExpected, cbIdtr, &abBuf[off]);
        g_usBs3TestStep++;

        /* Again with a buffer filled with a byte not occuring in the previous result. */
        Bs3MemSet(abBuf, bFiller, sizeof(abBuf));
        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
        if (off > 0 && !ASMMemIsAllU8(abBuf, off, bFiller))
            Bs3TestFailedF("Unexpected buffer bytes set before (#4): cbIdtr=%u off=%u bFiller=%#x abBuf=%.*Rhxs\n",
                           cbIdtr, off, bFiller, off + cbBuf, abBuf);
        if (!ASMMemIsAllU8(&abBuf[off + cbIdtr], sizeof(abBuf) - cbIdtr - off, bFiller))
            Bs3TestFailedF("Unexpected buffer bytes set after (#4): cbIdtr=%u off=%u bFiller=%#x abBuf=%.*Rhxs\n",
                           cbIdtr, off, bFiller, off + cbBuf, abBuf);
        if (Bs3MemChr(&abBuf[off], bFiller, cbIdtr) != NULL)
            Bs3TestFailedF("Not all bytes touched (#4): cbIdtr=%u off=%u bFiller=%#x abBuf=%.*Rhxs\n",
                           cbIdtr, off, bFiller, off + cbBuf, abBuf);
        if (f286 && abBuf[off + cbIdtr - 1] != 0xff)
            Bs3TestFailedF("286: Top base byte isn't 0xff (#4): %#x\n", abBuf[off + cbIdtr - 1]);
        if (Bs3MemCmp(&abBuf[off], pbExpected, cbIdtr) != 0)
            Bs3TestFailedF("Mismatch (#4): expected %.*Rhxs, got %.*Rhxs\n", cbIdtr, pbExpected, cbIdtr, &abBuf[off]);
        g_usBs3TestStep++;
    }
    pbBuf = abBuf;
    Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, abBuf);
    CtxUdExpected.rbx.u = Ctx.rbx.u;

    /*
     * Play with the selector limit if the target mode supports limit checking
     * We use BS3_SEL_TEST_PAGE_00 for this
     */
    if (   !BS3_MODE_IS_RM_OR_V86(bTestMode)
        && !BS3_MODE_IS_64BIT_CODE(bTestMode))
    {
        uint16_t cbLimit;
        uint32_t uFlatBuf = Bs3SelPtrToFlat(abBuf);
        Bs3GdteTestPage00 = Bs3Gdte_DATA16;
        Bs3GdteTestPage00.Gen.u2Dpl       = bRing;
        Bs3GdteTestPage00.Gen.u16BaseLow  = (uint16_t)uFlatBuf;
        Bs3GdteTestPage00.Gen.u8BaseHigh1 = (uint8_t)(uFlatBuf >> 16);
        Bs3GdteTestPage00.Gen.u8BaseHigh2 = (uint8_t)(uFlatBuf >> 24);

        if (pWorker->fSs)
            CtxUdExpected.ss = Ctx.ss = BS3_SEL_TEST_PAGE_00 | bRing;
        else
            CtxUdExpected.ds = Ctx.ds = BS3_SEL_TEST_PAGE_00 | bRing;

        /* Expand up (normal). */
        for (off = 0; off < 8; off++)
        {
            CtxUdExpected.rbx.u = Ctx.rbx.u = off;
            for (cbLimit = 0; cbLimit < cbIdtr*2; cbLimit++)
            {
                Bs3GdteTestPage00.Gen.u16LimitLow = cbLimit;
                Bs3MemSet(abBuf, bFiller, sizeof(abBuf));
                Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
                if (off + cbIdtr <= cbLimit + 1)
                {
                    bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                    if (Bs3MemChr(&abBuf[off], bFiller, cbIdtr) != NULL)
                        Bs3TestFailedF("Not all bytes touched (#5): cbIdtr=%u off=%u cbLimit=%u bFiller=%#x abBuf=%.*Rhxs\n",
                                       cbIdtr, off, cbLimit, bFiller, off + cbBuf, abBuf);
                    if (Bs3MemCmp(&abBuf[off], pbExpected, cbIdtr) != 0)
                        Bs3TestFailedF("Mismatch (#5): expected %.*Rhxs, got %.*Rhxs\n", cbIdtr, pbExpected, cbIdtr, &abBuf[off]);
                    if (f286 && abBuf[off + cbIdtr - 1] != 0xff)
                        Bs3TestFailedF("286: Top base byte isn't 0xff (#5): %#x\n", abBuf[off + cbIdtr - 1]);
                }
                else
                {
                    if (pWorker->fSs)
                        bs3CpuBasic2_CompareSsCtx(&TrapCtx, &Ctx, 0, false /*f486ResumeFlagHint*/);
                    else
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                    if (off + 2 <= cbLimit + 1)
                    {
                        if (Bs3MemChr(&abBuf[off], bFiller, 2) != NULL)
                            Bs3TestFailedF("Limit bytes not touched (#6): cbIdtr=%u off=%u cbLimit=%u bFiller=%#x abBuf=%.*Rhxs\n",
                                           cbIdtr, off, cbLimit, bFiller, off + cbBuf, abBuf);
                        if (Bs3MemCmp(&abBuf[off], pbExpected, 2) != 0)
                            Bs3TestFailedF("Mismatch (#6): expected %.2Rhxs, got %.2Rhxs\n", pbExpected, &abBuf[off]);
                        if (!ASMMemIsAllU8(&abBuf[off + 2], cbIdtr - 2, bFiller))
                            Bs3TestFailedF("Base bytes touched on #GP (#6): cbIdtr=%u off=%u cbLimit=%u bFiller=%#x abBuf=%.*Rhxs\n",
                                           cbIdtr, off, cbLimit, bFiller, off + cbBuf, abBuf);
                    }
                    else if (!ASMMemIsAllU8(abBuf, sizeof(abBuf), bFiller))
                        Bs3TestFailedF("Bytes touched on #GP: cbIdtr=%u off=%u cbLimit=%u bFiller=%#x abBuf=%.*Rhxs\n",
                                       cbIdtr, off, cbLimit, bFiller, off + cbBuf, abBuf);
                }

                if (off > 0 && !ASMMemIsAllU8(abBuf, off, bFiller))
                    Bs3TestFailedF("Leading bytes touched (#7): cbIdtr=%u off=%u cbLimit=%u bFiller=%#x abBuf=%.*Rhxs\n",
                                   cbIdtr, off, cbLimit, bFiller, off + cbBuf, abBuf);
                if (!ASMMemIsAllU8(&abBuf[off + cbIdtr], sizeof(abBuf) - off - cbIdtr, bFiller))
                    Bs3TestFailedF("Trailing bytes touched (#7): cbIdtr=%u off=%u cbLimit=%u bFiller=%#x abBuf=%.*Rhxs\n",
                                   cbIdtr, off, cbLimit, bFiller, off + cbBuf, abBuf);

                g_usBs3TestStep++;
            }
        }

        /* Expand down (weird).  Inverted valid area compared to expand up,
           so a limit of zero give us a valid range for 0001..0ffffh (instead of
           a segment with one valid byte at 0000h).  Whereas a limit of 0fffeh
           means one valid byte at 0ffffh, and a limit of 0ffffh means none
           (because in a normal expand up the 0ffffh means all 64KB are
           accessible). */
        Bs3GdteTestPage00.Gen.u4Type = X86_SEL_TYPE_RW_DOWN_ACC;
        for (off = 0; off < 8; off++)
        {
            CtxUdExpected.rbx.u = Ctx.rbx.u = off;
            for (cbLimit = 0; cbLimit < cbIdtr*2; cbLimit++)
            {
                Bs3GdteTestPage00.Gen.u16LimitLow = cbLimit;
                Bs3MemSet(abBuf, bFiller, sizeof(abBuf));
                Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);

                if (off > cbLimit)
                {
                    bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                    if (Bs3MemChr(&abBuf[off], bFiller, cbIdtr) != NULL)
                        Bs3TestFailedF("Not all bytes touched (#8): cbIdtr=%u off=%u cbLimit=%u bFiller=%#x abBuf=%.*Rhxs\n",
                                       cbIdtr, off, cbLimit, bFiller, off + cbBuf, abBuf);
                    if (Bs3MemCmp(&abBuf[off], pbExpected, cbIdtr) != 0)
                        Bs3TestFailedF("Mismatch (#8): expected %.*Rhxs, got %.*Rhxs\n", cbIdtr, pbExpected, cbIdtr, &abBuf[off]);
                    if (f286 && abBuf[off + cbIdtr - 1] != 0xff)
                        Bs3TestFailedF("286: Top base byte isn't 0xff (#8): %#x\n", abBuf[off + cbIdtr - 1]);
                }
                else
                {
                    if (pWorker->fSs)
                        bs3CpuBasic2_CompareSsCtx(&TrapCtx, &Ctx, 0, false /*f486ResumeFlagHint*/);
                    else
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                    if (!ASMMemIsAllU8(abBuf, sizeof(abBuf), bFiller))
                        Bs3TestFailedF("Bytes touched on #GP: cbIdtr=%u off=%u cbLimit=%u bFiller=%#x abBuf=%.*Rhxs\n",
                                       cbIdtr, off, cbLimit, bFiller, off + cbBuf, abBuf);
                }

                if (off > 0 && !ASMMemIsAllU8(abBuf, off, bFiller))
                    Bs3TestFailedF("Leading bytes touched (#9): cbIdtr=%u off=%u cbLimit=%u bFiller=%#x abBuf=%.*Rhxs\n",
                                   cbIdtr, off, cbLimit, bFiller, off + cbBuf, abBuf);
                if (!ASMMemIsAllU8(&abBuf[off + cbIdtr], sizeof(abBuf) - off - cbIdtr, bFiller))
                    Bs3TestFailedF("Trailing bytes touched (#9): cbIdtr=%u off=%u cbLimit=%u bFiller=%#x abBuf=%.*Rhxs\n",
                                   cbIdtr, off, cbLimit, bFiller, off + cbBuf, abBuf);

                g_usBs3TestStep++;
            }
        }

        Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, abBuf);
        CtxUdExpected.rbx.u = Ctx.rbx.u;
        CtxUdExpected.ss = Ctx.ss;
        CtxUdExpected.ds = Ctx.ds;
    }

    /*
     * Play with the paging.
     */
    if (   BS3_MODE_IS_PAGED(bTestMode)
        && (!pWorker->fSs || bRing == 3) /* SS.DPL == CPL, we'll get some tiled ring-3 selector here.  */
        && (pbTest = (uint8_t BS3_FAR *)Bs3MemGuardedTestPageAlloc(BS3MEMKIND_TILED)) != NULL)
    {
        RTCCUINTXREG uFlatTest = Bs3SelPtrToFlat(pbTest);

        /*
         * Slide the buffer towards the trailing guard page.  We'll observe the
         * first word being written entirely separately from the 2nd dword/qword.
         */
        for (off = X86_PAGE_SIZE - cbIdtr - 4; off < X86_PAGE_SIZE + 4; off++)
        {
            Bs3MemSet(&pbTest[X86_PAGE_SIZE - cbIdtr * 2], bFiller, cbIdtr * 2);
            Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, &pbTest[off]);
            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            if (off + cbIdtr <= X86_PAGE_SIZE)
            {
                CtxUdExpected.rbx = Ctx.rbx;
                CtxUdExpected.ss  = Ctx.ss;
                CtxUdExpected.ds  = Ctx.ds;
                bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                if (Bs3MemCmp(&pbTest[off], pbExpected, cbIdtr) != 0)
                    Bs3TestFailedF("Mismatch (#9): expected %.*Rhxs, got %.*Rhxs\n", cbIdtr, pbExpected, cbIdtr, &pbTest[off]);
            }
            else
            {
                bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx, X86_TRAP_PF_RW | (Ctx.bCpl == 3 ? X86_TRAP_PF_US : 0),
                                          uFlatTest + RT_MAX(off, X86_PAGE_SIZE));
                if (   off <= X86_PAGE_SIZE - 2
                    && Bs3MemCmp(&pbTest[off], pbExpected, 2) != 0)
                    Bs3TestFailedF("Mismatch (#10): Expected limit %.2Rhxs, got %.2Rhxs; off=%#x\n",
                                   pbExpected, &pbTest[off], off);
                if (   off < X86_PAGE_SIZE - 2
                    && !ASMMemIsAllU8(&pbTest[off + 2], X86_PAGE_SIZE - off - 2, bFiller))
                    Bs3TestFailedF("Wrote partial base on #PF (#10): bFiller=%#x, got %.*Rhxs; off=%#x\n",
                                   bFiller, X86_PAGE_SIZE - off - 2, &pbTest[off + 2], off);
                if (off == X86_PAGE_SIZE - 1 && pbTest[off] != bFiller)
                    Bs3TestFailedF("Wrote partial limit on #PF (#10): Expected %02x, got %02x\n", bFiller, pbTest[off]);
            }
            g_usBs3TestStep++;
        }

        /*
         * Now, do it the other way around. It should look normal now since writing
         * the limit will #PF first and nothing should be written.
         */
        for (off = cbIdtr + 4; off >= -cbIdtr - 4; off--)
        {
            Bs3MemSet(pbTest, bFiller, 48);
            Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, &pbTest[off]);
            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            if (off >= 0)
            {
                CtxUdExpected.rbx = Ctx.rbx;
                CtxUdExpected.ss  = Ctx.ss;
                CtxUdExpected.ds  = Ctx.ds;
                bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                if (Bs3MemCmp(&pbTest[off], pbExpected, cbIdtr) != 0)
                    Bs3TestFailedF("Mismatch (#11): expected %.*Rhxs, got %.*Rhxs\n", cbIdtr, pbExpected, cbIdtr, &pbTest[off]);
            }
            else
            {
                bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx, X86_TRAP_PF_RW | (Ctx.bCpl == 3 ? X86_TRAP_PF_US : 0), uFlatTest + off);
                if (   -off < cbIdtr
                    && !ASMMemIsAllU8(pbTest, cbIdtr + off, bFiller))
                    Bs3TestFailedF("Wrote partial content on #PF (#12): bFiller=%#x, found %.*Rhxs; off=%d\n",
                                   bFiller, cbIdtr + off, pbTest, off);
            }
            if (!ASMMemIsAllU8(&pbTest[RT_MAX(cbIdtr + off, 0)], 16, bFiller))
                Bs3TestFailedF("Wrote beyond expected area (#13): bFiller=%#x, found %.16Rhxs; off=%d\n",
                               bFiller, &pbTest[RT_MAX(cbIdtr + off, 0)], off);
            g_usBs3TestStep++;
        }

        /*
         * Combine paging and segment limit and check ordering.
         * This is kind of interesting here since it the instruction seems to
         * be doing two separate writes.
         */
        if (   !BS3_MODE_IS_RM_OR_V86(bTestMode)
            && !BS3_MODE_IS_64BIT_CODE(bTestMode))
        {
            uint16_t cbLimit;

            Bs3GdteTestPage00 = Bs3Gdte_DATA16;
            Bs3GdteTestPage00.Gen.u2Dpl       = bRing;
            Bs3GdteTestPage00.Gen.u16BaseLow  = (uint16_t)uFlatTest;
            Bs3GdteTestPage00.Gen.u8BaseHigh1 = (uint8_t)(uFlatTest >> 16);
            Bs3GdteTestPage00.Gen.u8BaseHigh2 = (uint8_t)(uFlatTest >> 24);

            if (pWorker->fSs)
                CtxUdExpected.ss = Ctx.ss = BS3_SEL_TEST_PAGE_00 | bRing;
            else
                CtxUdExpected.ds = Ctx.ds = BS3_SEL_TEST_PAGE_00 | bRing;

            /* Expand up (normal), approaching tail guard page. */
            for (off = X86_PAGE_SIZE - cbIdtr - 4; off < X86_PAGE_SIZE + 4; off++)
            {
                CtxUdExpected.rbx.u = Ctx.rbx.u = off;
                for (cbLimit = X86_PAGE_SIZE - cbIdtr*2; cbLimit < X86_PAGE_SIZE + cbIdtr*2; cbLimit++)
                {
                    Bs3GdteTestPage00.Gen.u16LimitLow = cbLimit;
                    Bs3MemSet(&pbTest[X86_PAGE_SIZE - cbIdtr * 2], bFiller, cbIdtr * 2);
                    Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
                    if (off + cbIdtr <= cbLimit + 1)
                    {
                        /* No #GP, but maybe #PF. */
                        if (off + cbIdtr <= X86_PAGE_SIZE)
                        {
                            bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                            if (Bs3MemCmp(&pbTest[off], pbExpected, cbIdtr) != 0)
                                Bs3TestFailedF("Mismatch (#14): expected %.*Rhxs, got %.*Rhxs\n",
                                               cbIdtr, pbExpected, cbIdtr, &pbTest[off]);
                        }
                        else
                        {
                            bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx, X86_TRAP_PF_RW | (Ctx.bCpl == 3 ? X86_TRAP_PF_US : 0),
                                                      uFlatTest + RT_MAX(off, X86_PAGE_SIZE));
                            if (   off <= X86_PAGE_SIZE - 2
                                && Bs3MemCmp(&pbTest[off], pbExpected, 2) != 0)
                                Bs3TestFailedF("Mismatch (#15): Expected limit %.2Rhxs, got %.2Rhxs; off=%#x\n",
                                               pbExpected, &pbTest[off], off);
                            cb = X86_PAGE_SIZE - off - 2;
                            if (   off < X86_PAGE_SIZE - 2
                                && !ASMMemIsAllU8(&pbTest[off + 2], cb, bFiller))
                                Bs3TestFailedF("Wrote partial base on #PF (#15): bFiller=%#x, got %.*Rhxs; off=%#x\n",
                                               bFiller, cb, &pbTest[off + 2], off);
                            if (off == X86_PAGE_SIZE - 1 && pbTest[off] != bFiller)
                                Bs3TestFailedF("Wrote partial limit on #PF (#15): Expected %02x, got %02x\n", bFiller, pbTest[off]);
                        }
                    }
                    else if (off + 2 <= cbLimit + 1)
                    {
                        /* [ig]tr.limit writing does not cause #GP, but may cause #PG, if not writing the base causes #GP. */
                        if (off <= X86_PAGE_SIZE - 2)
                        {
                            if (pWorker->fSs)
                                bs3CpuBasic2_CompareSsCtx(&TrapCtx, &Ctx, 0, false /*f486ResumeFlagHint*/);
                            else
                                bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                            if (Bs3MemCmp(&pbTest[off], pbExpected, 2) != 0)
                                Bs3TestFailedF("Mismatch (#16): Expected limit %.2Rhxs, got %.2Rhxs; off=%#x\n",
                                               pbExpected, &pbTest[off], off);
                            cb = X86_PAGE_SIZE - off - 2;
                            if (   off < X86_PAGE_SIZE - 2
                                && !ASMMemIsAllU8(&pbTest[off + 2], cb, bFiller))
                                Bs3TestFailedF("Wrote partial base with limit (#16): bFiller=%#x, got %.*Rhxs; off=%#x\n",
                                               bFiller, cb, &pbTest[off + 2], off);
                        }
                        else
                        {
                            bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx, X86_TRAP_PF_RW | (Ctx.bCpl == 3 ? X86_TRAP_PF_US : 0),
                                                      uFlatTest + RT_MAX(off, X86_PAGE_SIZE));
                            if (   off < X86_PAGE_SIZE
                                && !ASMMemIsAllU8(&pbTest[off], X86_PAGE_SIZE - off, bFiller))
                                Bs3TestFailedF("Mismatch (#16): Partial limit write on #PF: bFiller=%#x, got %.*Rhxs\n",
                                               bFiller, X86_PAGE_SIZE - off, &pbTest[off]);
                        }
                    }
                    else
                    {
                        /* #GP/#SS on limit. */
                        if (pWorker->fSs)
                            bs3CpuBasic2_CompareSsCtx(&TrapCtx, &Ctx, 0, false /*f486ResumeFlagHint*/);
                        else
                            bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                        if (   off < X86_PAGE_SIZE
                            && !ASMMemIsAllU8(&pbTest[off], X86_PAGE_SIZE - off, bFiller))
                            Bs3TestFailedF("Mismatch (#17): Partial write on #GP: bFiller=%#x, got %.*Rhxs\n",
                                           bFiller, X86_PAGE_SIZE - off, &pbTest[off]);
                    }

                    cb = RT_MIN(cbIdtr * 2, off - (X86_PAGE_SIZE - cbIdtr*2));
                    if (!ASMMemIsAllU8(&pbTest[X86_PAGE_SIZE - cbIdtr * 2], cb, bFiller))
                        Bs3TestFailedF("Leading bytes touched (#18): cbIdtr=%u off=%u cbLimit=%u bFiller=%#x pbTest=%.*Rhxs\n",
                                       cbIdtr, off, cbLimit, bFiller, cb, pbTest[X86_PAGE_SIZE - cbIdtr * 2]);

                    g_usBs3TestStep++;

                    /* Set DS to 0 and check that we get #GP(0). */
                    if (!pWorker->fSs)
                    {
                        Ctx.ds = 0;
                        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                        Ctx.ds = BS3_SEL_TEST_PAGE_00 | bRing;
                        g_usBs3TestStep++;
                    }
                }
            }

            /* Expand down. */
            pbTest    -= X86_PAGE_SIZE; /* Note! we're backing up a page to simplify things */
            uFlatTest -= X86_PAGE_SIZE;

            Bs3GdteTestPage00.Gen.u4Type = X86_SEL_TYPE_RW_DOWN_ACC;
            Bs3GdteTestPage00.Gen.u16BaseLow  = (uint16_t)uFlatTest;
            Bs3GdteTestPage00.Gen.u8BaseHigh1 = (uint8_t)(uFlatTest >> 16);
            Bs3GdteTestPage00.Gen.u8BaseHigh2 = (uint8_t)(uFlatTest >> 24);

            for (off = X86_PAGE_SIZE - cbIdtr - 4; off < X86_PAGE_SIZE + 4; off++)
            {
                CtxUdExpected.rbx.u = Ctx.rbx.u = off;
                for (cbLimit = X86_PAGE_SIZE - cbIdtr*2; cbLimit < X86_PAGE_SIZE + cbIdtr*2; cbLimit++)
                {
                    Bs3GdteTestPage00.Gen.u16LimitLow = cbLimit;
                    Bs3MemSet(&pbTest[X86_PAGE_SIZE], bFiller, cbIdtr * 2);
                    Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
                    if (cbLimit < off && off >= X86_PAGE_SIZE)
                    {
                        bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                        if (Bs3MemCmp(&pbTest[off], pbExpected, cbIdtr) != 0)
                            Bs3TestFailedF("Mismatch (#19): expected %.*Rhxs, got %.*Rhxs\n",
                                           cbIdtr, pbExpected, cbIdtr, &pbTest[off]);
                        cb = X86_PAGE_SIZE + cbIdtr*2 - off;
                        if (!ASMMemIsAllU8(&pbTest[off + cbIdtr], cb, bFiller))
                            Bs3TestFailedF("Trailing bytes touched (#20): cbIdtr=%u off=%u cbLimit=%u bFiller=%#x pbTest=%.*Rhxs\n",
                                           cbIdtr, off, cbLimit, bFiller, cb, pbTest[off + cbIdtr]);
                    }
                    else
                    {
                        if (cbLimit < off && off < X86_PAGE_SIZE)
                            bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx, X86_TRAP_PF_RW | (Ctx.bCpl == 3 ? X86_TRAP_PF_US : 0),
                                                      uFlatTest + off);
                        else if (pWorker->fSs)
                            bs3CpuBasic2_CompareSsCtx(&TrapCtx, &Ctx, 0, false /*f486ResumeFlagHint*/);
                        else
                            bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                        cb = cbIdtr*2;
                        if (!ASMMemIsAllU8(&pbTest[X86_PAGE_SIZE], cb, bFiller))
                            Bs3TestFailedF("Trailing bytes touched (#20): cbIdtr=%u off=%u cbLimit=%u bFiller=%#x pbTest=%.*Rhxs\n",
                                           cbIdtr, off, cbLimit, bFiller, cb, pbTest[X86_PAGE_SIZE]);
                    }
                    g_usBs3TestStep++;
                }
            }

            pbTest    += X86_PAGE_SIZE;
            uFlatTest += X86_PAGE_SIZE;
        }

        Bs3MemGuardedTestPageFree(pbTest);
    }

    /*
     * Check non-canonical 64-bit space.
     */
    if (   BS3_MODE_IS_64BIT_CODE(bTestMode)
        && (pbTest = (uint8_t BS3_FAR *)Bs3PagingSetupCanonicalTraps()) != NULL)
    {
        /* Make our references relative to the gap. */
        pbTest += g_cbBs3PagingOneCanonicalTrap;

        /* Hit it from below. */
        for (off = -cbIdtr - 8; off < cbIdtr + 8; off++)
        {
            Ctx.rbx.u = CtxUdExpected.rbx.u = UINT64_C(0x0000800000000000) + off;
            Bs3MemSet(&pbTest[-64], bFiller, 64*2);
            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            if (off + cbIdtr <= 0)
            {
                bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                if (Bs3MemCmp(&pbTest[off], pbExpected, cbIdtr) != 0)
                    Bs3TestFailedF("Mismatch (#21): expected %.*Rhxs, got %.*Rhxs\n", cbIdtr, pbExpected, cbIdtr, &pbTest[off]);
            }
            else
            {
                bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                if (off <= -2 && Bs3MemCmp(&pbTest[off], pbExpected, 2) != 0)
                    Bs3TestFailedF("Mismatch (#21): expected limit %.2Rhxs, got %.2Rhxs\n", pbExpected, &pbTest[off]);
                off2 = off <= -2 ? 2 : 0;
                cb   = cbIdtr - off2;
                if (!ASMMemIsAllU8(&pbTest[off + off2], cb, bFiller))
                    Bs3TestFailedF("Mismatch (#21): touched base %.*Rhxs, got %.*Rhxs\n",
                                   cb, &pbExpected[off], cb, &pbTest[off + off2]);
            }
            if (!ASMMemIsAllU8(&pbTest[off - 16], 16, bFiller))
                Bs3TestFailedF("Leading bytes touched (#21): bFiller=%#x, got %.16Rhxs\n", bFiller, &pbTest[off]);
            if (!ASMMemIsAllU8(&pbTest[off + cbIdtr], 16, bFiller))
                Bs3TestFailedF("Trailing bytes touched (#21): bFiller=%#x, got %.16Rhxs\n", bFiller, &pbTest[off + cbIdtr]);
        }

        /* Hit it from above. */
        for (off = -cbIdtr - 8; off < cbIdtr + 8; off++)
        {
            Ctx.rbx.u = CtxUdExpected.rbx.u = UINT64_C(0xffff800000000000) + off;
            Bs3MemSet(&pbTest[-64], bFiller, 64*2);
            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            if (off >= 0)
            {
                bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                if (Bs3MemCmp(&pbTest[off], pbExpected, cbIdtr) != 0)
                    Bs3TestFailedF("Mismatch (#22): expected %.*Rhxs, got %.*Rhxs\n", cbIdtr, pbExpected, cbIdtr, &pbTest[off]);
            }
            else
            {
                bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                if (!ASMMemIsAllU8(&pbTest[off], cbIdtr, bFiller))
                    Bs3TestFailedF("Mismatch (#22): touched base %.*Rhxs, got %.*Rhxs\n",
                                   cbIdtr, &pbExpected[off], cbIdtr, &pbTest[off]);
            }
            if (!ASMMemIsAllU8(&pbTest[off - 16], 16, bFiller))
                Bs3TestFailedF("Leading bytes touched (#22): bFiller=%#x, got %.16Rhxs\n", bFiller, &pbTest[off]);
            if (!ASMMemIsAllU8(&pbTest[off + cbIdtr], 16, bFiller))
                Bs3TestFailedF("Trailing bytes touched (#22): bFiller=%#x, got %.16Rhxs\n", bFiller, &pbTest[off + cbIdtr]);
        }

    }
}


static void bs3CpuBasic2_sidt_sgdt_Common(uint8_t bTestMode, BS3CB2SIDTSGDT const BS3_FAR *paWorkers, unsigned cWorkers,
                                          uint8_t const *pbExpected)
{
    unsigned idx;
    unsigned bRing;
    unsigned iStep = 0;

    /* Note! We skip the SS checks for ring-0 since we badly mess up SS in the
             test and don't want to bother with double faults. */
    for (bRing = 0; bRing <= 3; bRing++)
    {
        for (idx = 0; idx < cWorkers; idx++)
            if (    (paWorkers[idx].bMode & (bTestMode & BS3_MODE_CODE_MASK))
                && (!paWorkers[idx].fSs || bRing != 0 /** @todo || BS3_MODE_IS_64BIT_SYS(bTestMode)*/ ))
            {
                g_usBs3TestStep = iStep;
                bs3CpuBasic2_sidt_sgdt_One(&paWorkers[idx], bTestMode, bRing, pbExpected);
                iStep += 1000;
            }
        if (BS3_MODE_IS_RM_OR_V86(bTestMode))
            break;
    }
}


BS3_DECL_FAR(uint8_t) BS3_CMN_FAR_NM(bs3CpuBasic2_sidt)(uint8_t bMode)
{
    union
    {
        RTIDTR  Idtr;
        uint8_t ab[16];
    } Expected;

    //if (bMode != BS3_MODE_LM64) return BS3TESTDOMODE_SKIPPED;
    bs3CpuBasic2_SetGlobals(bMode);

    /*
     * Pass to common worker which is only compiled once per mode.
     */
    Bs3MemZero(&Expected, sizeof(Expected));
    ASMGetIDTR(&Expected.Idtr);
    bs3CpuBasic2_sidt_sgdt_Common(bMode, g_aSidtWorkers, RT_ELEMENTS(g_aSidtWorkers), Expected.ab);

    /*
     * Re-initialize the IDT.
     */
    Bs3TrapReInit();
    return 0;
}


BS3_DECL_FAR(uint8_t) BS3_CMN_FAR_NM(bs3CpuBasic2_sgdt)(uint8_t bMode)
{
    uint64_t const uOrgAddr = Bs3Lgdt_Gdt.uAddr;
    uint64_t       uNew     = 0;
    union
    {
        RTGDTR  Gdtr;
        uint8_t ab[16];
    } Expected;

    //if (bMode != BS3_MODE_LM64) return BS3TESTDOMODE_SKIPPED;
    bs3CpuBasic2_SetGlobals(bMode);

    /*
     * If paged mode, try push the GDT way up.
     */
    Bs3MemZero(&Expected, sizeof(Expected));
    ASMGetGDTR(&Expected.Gdtr);
    if (BS3_MODE_IS_PAGED(bMode))
    {
/** @todo loading non-canonical base addresses.   */
        int rc;
        uNew  = BS3_MODE_IS_64BIT_SYS(bMode) ? UINT64_C(0xffff80fedcb70000) : UINT64_C(0xc2d28000);
        uNew |= uOrgAddr & X86_PAGE_OFFSET_MASK;
        rc = Bs3PagingAlias(uNew, uOrgAddr, Bs3Lgdt_Gdt.cb, X86_PTE_P | X86_PTE_RW | X86_PTE_US | X86_PTE_D | X86_PTE_A);
        if (RT_SUCCESS(rc))
        {
            Bs3Lgdt_Gdt.uAddr = uNew;
            Bs3UtilSetFullGdtr(Bs3Lgdt_Gdt.cb, uNew);
            ASMGetGDTR(&Expected.Gdtr);
            if (BS3_MODE_IS_64BIT_SYS(bMode) && ARCH_BITS != 64)
                *(uint32_t *)&Expected.ab[6] = (uint32_t)(uNew >> 32);
        }
    }

    /*
     * Pass to common worker which is only compiled once per mode.
     */
    bs3CpuBasic2_sidt_sgdt_Common(bMode, g_aSgdtWorkers, RT_ELEMENTS(g_aSgdtWorkers), Expected.ab);

    /*
     * Unalias the GDT.
     */
    if (uNew != 0)
    {
        Bs3Lgdt_Gdt.uAddr = uOrgAddr;
        Bs3UtilSetFullGdtr(Bs3Lgdt_Gdt.cb, uOrgAddr);
        Bs3PagingUnalias(uNew, Bs3Lgdt_Gdt.cb);
    }

    /*
     * Re-initialize the IDT.
     */
    Bs3TrapReInit();
    return 0;
}



/*
 * LIDT & LGDT
 */

/**
 * Executes one round of LIDT and LGDT tests using one assembly worker.
 *
 * This is written with driving everything from the 16-bit or 32-bit worker in
 * mind, i.e. not assuming the test bitcount is the same as the current.
 */
static void bs3CpuBasic2_lidt_lgdt_One(BS3CB2SIDTSGDT const BS3_FAR *pWorker, uint8_t bTestMode, uint8_t bRing,
                                       uint8_t const *pbRestore, size_t cbRestore, uint8_t const *pbExpected)
{
    static const struct
    {
        bool        fGP;
        uint16_t    cbLimit;
        uint64_t    u64Base;
    } s_aValues64[] =
    {
        { false, 0x0000, UINT64_C(0x0000000000000000) },
        { false, 0x0001, UINT64_C(0x0000000000000001) },
        { false, 0x0002, UINT64_C(0x0000000000000010) },
        { false, 0x0003, UINT64_C(0x0000000000000123) },
        { false, 0x0004, UINT64_C(0x0000000000001234) },
        { false, 0x0005, UINT64_C(0x0000000000012345) },
        { false, 0x0006, UINT64_C(0x0000000000123456) },
        { false, 0x0007, UINT64_C(0x0000000001234567) },
        { false, 0x0008, UINT64_C(0x0000000012345678) },
        { false, 0x0009, UINT64_C(0x0000000123456789) },
        { false, 0x000a, UINT64_C(0x000000123456789a) },
        { false, 0x000b, UINT64_C(0x00000123456789ab) },
        { false, 0x000c, UINT64_C(0x0000123456789abc) },
        { false, 0x001c, UINT64_C(0x00007ffffeefefef) },
        { false, 0xffff, UINT64_C(0x00007fffffffffff) },
        {  true, 0xf3f1, UINT64_C(0x0000800000000000) },
        {  true, 0x0000, UINT64_C(0x0000800000000000) },
        {  true, 0x0000, UINT64_C(0x0000800000000333) },
        {  true, 0x00f0, UINT64_C(0x0001000000000000) },
        {  true, 0x0ff0, UINT64_C(0x0012000000000000) },
        {  true, 0x0eff, UINT64_C(0x0123000000000000) },
        {  true, 0xe0fe, UINT64_C(0x1234000000000000) },
        {  true, 0x00ad, UINT64_C(0xffff300000000000) },
        {  true, 0x0000, UINT64_C(0xffff7fffffffffff) },
        {  true, 0x00f0, UINT64_C(0xffff7fffffffffff) },
        { false, 0x5678, UINT64_C(0xffff800000000000) },
        { false, 0x2969, UINT64_C(0xffffffffffeefefe) },
        { false, 0x1221, UINT64_C(0xffffffffffffffff) },
        { false, 0x1221, UINT64_C(0xffffffffffffffff) },
    };
    static const struct
    {
        uint16_t    cbLimit;
        uint32_t    u32Base;
    } s_aValues32[] =
    {
        { 0xdfdf, UINT32_C(0xefefefef) },
        { 0x0000, UINT32_C(0x00000000) },
        { 0x0001, UINT32_C(0x00000001) },
        { 0x0002, UINT32_C(0x00000012) },
        { 0x0003, UINT32_C(0x00000123) },
        { 0x0004, UINT32_C(0x00001234) },
        { 0x0005, UINT32_C(0x00012345) },
        { 0x0006, UINT32_C(0x00123456) },
        { 0x0007, UINT32_C(0x01234567) },
        { 0x0008, UINT32_C(0x12345678) },
        { 0x0009, UINT32_C(0x80204060) },
        { 0x000a, UINT32_C(0xddeeffaa) },
        { 0x000b, UINT32_C(0xfdecdbca) },
        { 0x000c, UINT32_C(0x6098456b) },
        { 0x000d, UINT32_C(0x98506099) },
        { 0x000e, UINT32_C(0x206950bc) },
        { 0x000f, UINT32_C(0x9740395d) },
        { 0x0334, UINT32_C(0x64a9455e) },
        { 0xb423, UINT32_C(0xd20b6eff) },
        { 0x4955, UINT32_C(0x85296d46) },
        { 0xffff, UINT32_C(0x07000039) },
        { 0xefe1, UINT32_C(0x0007fe00) },
    };

    BS3TRAPFRAME        TrapCtx;
    BS3REGCTX           Ctx;
    BS3REGCTX           CtxUdExpected;
    BS3REGCTX           TmpCtx;
    uint8_t             abBufLoad[40];          /* Test buffer w/ misalignment test space and some (cbIdtr) extra guard. */
    uint8_t             abBufSave[32];          /* For saving the result after loading. */
    uint8_t             abBufRestore[24];       /* For restoring sane value (same seg as abBufSave!). */
    uint8_t             abExpectedFilled[32];   /* Same as pbExpected, except it's filled with bFiller2 instead of zeros. */
    uint8_t BS3_FAR    *pbBufSave;              /* Correctly aligned pointer into abBufSave. */
    uint8_t BS3_FAR    *pbBufRestore;           /* Correctly aligned pointer into abBufRestore. */
    uint8_t const       cbIdtr        = BS3_MODE_IS_64BIT_CODE(bTestMode) ? 2+8 : 2+4;
    uint8_t const       cbBaseLoaded  = BS3_MODE_IS_64BIT_CODE(bTestMode) ? 8
                                      : BS3_MODE_IS_16BIT_CODE(bTestMode) == !(pWorker->fFlags & BS3CB2SIDTSGDT_F_OPSIZE)
                                      ? 3 : 4;
    bool const          f286          = (g_uBs3CpuDetected & BS3CPU_TYPE_MASK) == BS3CPU_80286;
    uint8_t const       bTop16BitBase = f286 ? 0xff : 0x00;
    uint8_t             bFiller1;               /* For filling abBufLoad.  */
    uint8_t             bFiller2;               /* For filling abBufSave and expectations. */
    int                 off;
    uint8_t BS3_FAR    *pbTest;
    unsigned            i;

    /* make sure they're allocated  */
    Bs3MemZero(&Ctx, sizeof(Ctx));
    Bs3MemZero(&CtxUdExpected, sizeof(CtxUdExpected));
    Bs3MemZero(&TmpCtx, sizeof(TmpCtx));
    Bs3MemZero(&TrapCtx, sizeof(TrapCtx));
    Bs3MemZero(abBufSave, sizeof(abBufSave));
    Bs3MemZero(abBufLoad, sizeof(abBufLoad));
    Bs3MemZero(abBufRestore, sizeof(abBufRestore));

    /*
     * Create a context, giving this routine some more stack space.
     *  - Point the context at our LIDT [xBX] + SIDT [xDI] + LIDT [xSI] + UD2 combo.
     *  - Point DS/SS:xBX at abBufLoad.
     *  - Point ES:xDI at abBufSave.
     *  - Point ES:xSI at abBufRestore.
     */
    Bs3RegCtxSaveEx(&Ctx, bTestMode, 256 /*cbExtraStack*/);
    Bs3RegCtxSetRipCsFromLnkPtr(&Ctx, pWorker->fpfnWorker);
    if (BS3_MODE_IS_16BIT_SYS(bTestMode))
        g_uBs3TrapEipHint = Ctx.rip.u32;
    Ctx.rflags.u16 &= ~X86_EFL_IF;
    Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, abBufLoad);

    pbBufSave = abBufSave;
    if ((BS3_FP_OFF(pbBufSave) + 2) & 7)
        pbBufSave += 8 - ((BS3_FP_OFF(pbBufSave) + 2) & 7);
    Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rdi, &Ctx.es, pbBufSave);

    pbBufRestore = abBufRestore;
    if ((BS3_FP_OFF(pbBufRestore) + 2) & 7)
        pbBufRestore += 8 - ((BS3_FP_OFF(pbBufRestore) + 2) & 7);
    Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rsi, &Ctx.es, pbBufRestore);
    Bs3MemCpy(pbBufRestore, pbRestore, cbRestore);

    if (!BS3_MODE_IS_RM_OR_V86(bTestMode))
        Bs3RegCtxConvertToRingX(&Ctx, bRing);

    /* For successful SIDT attempts, we'll stop at the UD2. */
    Bs3MemCpy(&CtxUdExpected, &Ctx, sizeof(Ctx));
    CtxUdExpected.rip.u += pWorker->cbInstr;

    /*
     * Check that it works at all.
     */
    Bs3MemZero(abBufLoad, sizeof(abBufLoad));
    Bs3MemCpy(abBufLoad, pbBufRestore, cbIdtr);
    Bs3MemZero(abBufSave, sizeof(abBufSave));
    Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
    if (bRing != 0)
        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
    else
    {
        bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
        if (Bs3MemCmp(pbBufSave, pbExpected, cbIdtr * 2) != 0)
            Bs3TestFailedF("Mismatch (%s, #1): expected %.*Rhxs, got %.*Rhxs\n",
                           pWorker->pszDesc, cbIdtr*2, pbExpected, cbIdtr*2, pbBufSave);
    }
    g_usBs3TestStep++;

    /* Determine two filler bytes that doesn't appear in the previous result or our expectations. */
    bFiller1 = ~0x55;
    while (   Bs3MemChr(pbBufSave, bFiller1, cbIdtr) != NULL
           || Bs3MemChr(pbRestore, bFiller1, cbRestore) != NULL
           || bFiller1 == 0xff)
        bFiller1++;
    bFiller2 = 0x33;
    while (   Bs3MemChr(pbBufSave, bFiller2, cbIdtr) != NULL
           || Bs3MemChr(pbRestore, bFiller2, cbRestore) != NULL
           || bFiller2 == 0xff
           || bFiller2 == bFiller1)
        bFiller2++;
    Bs3MemSet(abExpectedFilled, bFiller2, sizeof(abExpectedFilled));
    Bs3MemCpy(abExpectedFilled, pbExpected, cbIdtr);

    /* Again with a buffer filled with a byte not occuring in the previous result. */
    Bs3MemSet(abBufLoad, bFiller1, sizeof(abBufLoad));
    Bs3MemCpy(abBufLoad, pbBufRestore, cbIdtr);
    Bs3MemSet(abBufSave, bFiller2, sizeof(abBufSave));
    Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
    if (bRing != 0)
        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
    else
    {
        bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
        if (Bs3MemCmp(pbBufSave, abExpectedFilled, cbIdtr * 2) != 0)
            Bs3TestFailedF("Mismatch (%s, #2): expected %.*Rhxs, got %.*Rhxs\n",
                           pWorker->pszDesc, cbIdtr*2, abExpectedFilled, cbIdtr*2, pbBufSave);
    }
    g_usBs3TestStep++;

    /*
     * Try loading a bunch of different limit+base value to check what happens,
     * especially what happens wrt the top part of the base in 16-bit mode.
     */
    if (BS3_MODE_IS_64BIT_CODE(bTestMode))
    {
        for (i = 0; i < RT_ELEMENTS(s_aValues64); i++)
        {
            Bs3MemSet(abBufLoad, bFiller1, sizeof(abBufLoad));
            Bs3MemCpy(&abBufLoad[0], &s_aValues64[i].cbLimit, 2);
            Bs3MemCpy(&abBufLoad[2], &s_aValues64[i].u64Base, 8);
            Bs3MemSet(abBufSave, bFiller2, sizeof(abBufSave));
            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            if (bRing != 0 || s_aValues64[i].fGP)
                bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
            else
            {
                bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                if (   Bs3MemCmp(&pbBufSave[0], &s_aValues64[i].cbLimit, 2) != 0
                    || Bs3MemCmp(&pbBufSave[2], &s_aValues64[i].u64Base, 8) != 0
                    || !ASMMemIsAllU8(&pbBufSave[10], cbIdtr, bFiller2))
                    Bs3TestFailedF("Mismatch (%s, #2): expected %04RX16:%016RX64, fillers %#x %#x, got %.*Rhxs\n",
                                   pWorker->pszDesc, s_aValues64[i].cbLimit, s_aValues64[i].u64Base,
                                   bFiller1, bFiller2, cbIdtr*2, pbBufSave);
            }
            g_usBs3TestStep++;
        }
    }
    else
    {
        for (i = 0; i < RT_ELEMENTS(s_aValues32); i++)
        {
            Bs3MemSet(abBufLoad, bFiller1, sizeof(abBufLoad));
            Bs3MemCpy(&abBufLoad[0], &s_aValues32[i].cbLimit, 2);
            Bs3MemCpy(&abBufLoad[2], &s_aValues32[i].u32Base, cbBaseLoaded);
            Bs3MemSet(abBufSave, bFiller2, sizeof(abBufSave));
            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            if (bRing != 0)
                bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
            else
            {
                bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                if (   Bs3MemCmp(&pbBufSave[0], &s_aValues32[i].cbLimit, 2) != 0
                    || Bs3MemCmp(&pbBufSave[2], &s_aValues32[i].u32Base, cbBaseLoaded) != 0
                    || (   cbBaseLoaded != 4
                        && pbBufSave[2+3] != bTop16BitBase)
                    || !ASMMemIsAllU8(&pbBufSave[8], cbIdtr, bFiller2))
                    Bs3TestFailedF("Mismatch (%s,#3): loaded %04RX16:%08RX32, fillers %#x %#x%s, got %.*Rhxs\n",
                                   pWorker->pszDesc, s_aValues32[i].cbLimit, s_aValues32[i].u32Base, bFiller1, bFiller2,
                                   f286 ? ", 286" : "", cbIdtr*2, pbBufSave);
            }
            g_usBs3TestStep++;
        }
    }

    /*
     * Slide the buffer along 8 bytes to cover misalignment.
     */
    for (off = 0; off < 8; off++)
    {
        Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, &abBufLoad[off]);
        CtxUdExpected.rbx.u = Ctx.rbx.u;

        Bs3MemSet(abBufLoad, bFiller1, sizeof(abBufLoad));
        Bs3MemCpy(&abBufLoad[off], pbBufRestore, cbIdtr);
        Bs3MemSet(abBufSave, bFiller2, sizeof(abBufSave));
        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        if (bRing != 0)
            bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
        else
        {
            bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
            if (Bs3MemCmp(pbBufSave, abExpectedFilled, cbIdtr * 2) != 0)
                Bs3TestFailedF("Mismatch (%s, #4): expected %.*Rhxs, got %.*Rhxs\n",
                               pWorker->pszDesc, cbIdtr*2, abExpectedFilled, cbIdtr*2, pbBufSave);
        }
        g_usBs3TestStep++;
    }
    Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, abBufLoad);
    CtxUdExpected.rbx.u = Ctx.rbx.u;

    /*
     * Play with the selector limit if the target mode supports limit checking
     * We use BS3_SEL_TEST_PAGE_00 for this
     */
    if (   !BS3_MODE_IS_RM_OR_V86(bTestMode)
        && !BS3_MODE_IS_64BIT_CODE(bTestMode))
    {
        uint16_t cbLimit;
        uint32_t uFlatBuf = Bs3SelPtrToFlat(abBufLoad);
        Bs3GdteTestPage00 = Bs3Gdte_DATA16;
        Bs3GdteTestPage00.Gen.u2Dpl       = bRing;
        Bs3GdteTestPage00.Gen.u16BaseLow  = (uint16_t)uFlatBuf;
        Bs3GdteTestPage00.Gen.u8BaseHigh1 = (uint8_t)(uFlatBuf >> 16);
        Bs3GdteTestPage00.Gen.u8BaseHigh2 = (uint8_t)(uFlatBuf >> 24);

        if (pWorker->fSs)
            CtxUdExpected.ss = Ctx.ss = BS3_SEL_TEST_PAGE_00 | bRing;
        else
            CtxUdExpected.ds = Ctx.ds = BS3_SEL_TEST_PAGE_00 | bRing;

        /* Expand up (normal). */
        for (off = 0; off < 8; off++)
        {
            CtxUdExpected.rbx.u = Ctx.rbx.u = off;
            for (cbLimit = 0; cbLimit < cbIdtr*2; cbLimit++)
            {
                Bs3GdteTestPage00.Gen.u16LimitLow = cbLimit;

                Bs3MemSet(abBufLoad, bFiller1, sizeof(abBufLoad));
                Bs3MemCpy(&abBufLoad[off], pbBufRestore, cbIdtr);
                Bs3MemSet(abBufSave, bFiller2, sizeof(abBufSave));
                Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
                if (bRing != 0)
                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                else if (off + cbIdtr <= cbLimit + 1)
                {
                    bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                    if (Bs3MemCmp(pbBufSave, abExpectedFilled, cbIdtr * 2) != 0)
                        Bs3TestFailedF("Mismatch (%s, #5): expected %.*Rhxs, got %.*Rhxs\n",
                                       pWorker->pszDesc, cbIdtr*2, abExpectedFilled, cbIdtr*2, pbBufSave);
                }
                else if (pWorker->fSs)
                    bs3CpuBasic2_CompareSsCtx(&TrapCtx, &Ctx, 0, false /*f486ResumeFlagHint*/);
                else
                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                g_usBs3TestStep++;

                /* Again with zero limit and messed up base (should trigger tripple fault if partially loaded). */
                abBufLoad[off] = abBufLoad[off + 1] = 0;
                abBufLoad[off + 2] |= 1;
                abBufLoad[off + cbIdtr - 2] ^= 0x5a;
                abBufLoad[off + cbIdtr - 1] ^= 0xa5;
                Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
                if (bRing != 0)
                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                else if (off + cbIdtr <= cbLimit + 1)
                    bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                else if (pWorker->fSs)
                    bs3CpuBasic2_CompareSsCtx(&TrapCtx, &Ctx, 0, false /*f486ResumeFlagHint*/);
                else
                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
            }
        }

        /* Expand down (weird).  Inverted valid area compared to expand up,
           so a limit of zero give us a valid range for 0001..0ffffh (instead of
           a segment with one valid byte at 0000h).  Whereas a limit of 0fffeh
           means one valid byte at 0ffffh, and a limit of 0ffffh means none
           (because in a normal expand up the 0ffffh means all 64KB are
           accessible). */
        Bs3GdteTestPage00.Gen.u4Type = X86_SEL_TYPE_RW_DOWN_ACC;
        for (off = 0; off < 8; off++)
        {
            CtxUdExpected.rbx.u = Ctx.rbx.u = off;
            for (cbLimit = 0; cbLimit < cbIdtr*2; cbLimit++)
            {
                Bs3GdteTestPage00.Gen.u16LimitLow = cbLimit;

                Bs3MemSet(abBufLoad, bFiller1, sizeof(abBufLoad));
                Bs3MemCpy(&abBufLoad[off], pbBufRestore, cbIdtr);
                Bs3MemSet(abBufSave, bFiller2, sizeof(abBufSave));
                Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
                if (bRing != 0)
                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                else if (off > cbLimit)
                {
                    bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                    if (Bs3MemCmp(pbBufSave, abExpectedFilled, cbIdtr * 2) != 0)
                        Bs3TestFailedF("Mismatch (%s, #6): expected %.*Rhxs, got %.*Rhxs\n",
                                       pWorker->pszDesc, cbIdtr*2, abExpectedFilled, cbIdtr*2, pbBufSave);
                }
                else if (pWorker->fSs)
                    bs3CpuBasic2_CompareSsCtx(&TrapCtx, &Ctx, 0, false /*f486ResumeFlagHint*/);
                else
                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
               g_usBs3TestStep++;

               /* Again with zero limit and messed up base (should trigger triple fault if partially loaded). */
               abBufLoad[off] = abBufLoad[off + 1] = 0;
               abBufLoad[off + 2] |= 3;
               abBufLoad[off + cbIdtr - 2] ^= 0x55;
               abBufLoad[off + cbIdtr - 1] ^= 0xaa;
               Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
               if (bRing != 0)
                   bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
               else if (off > cbLimit)
                   bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
               else if (pWorker->fSs)
                   bs3CpuBasic2_CompareSsCtx(&TrapCtx, &Ctx, 0, false /*f486ResumeFlagHint*/);
               else
                   bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
            }
        }

        Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, abBufLoad);
        CtxUdExpected.rbx.u = Ctx.rbx.u;
        CtxUdExpected.ss = Ctx.ss;
        CtxUdExpected.ds = Ctx.ds;
    }

    /*
     * Play with the paging.
     */
    if (   BS3_MODE_IS_PAGED(bTestMode)
        && (!pWorker->fSs || bRing == 3) /* SS.DPL == CPL, we'll get some tiled ring-3 selector here.  */
        && (pbTest = (uint8_t BS3_FAR *)Bs3MemGuardedTestPageAlloc(BS3MEMKIND_TILED)) != NULL)
    {
        RTCCUINTXREG uFlatTest = Bs3SelPtrToFlat(pbTest);

        /*
         * Slide the load buffer towards the trailing guard page.
         */
        Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, &pbTest[X86_PAGE_SIZE]);
        CtxUdExpected.ss = Ctx.ss;
        CtxUdExpected.ds = Ctx.ds;
        for (off = X86_PAGE_SIZE - cbIdtr - 4; off < X86_PAGE_SIZE + 4; off++)
        {
            Bs3MemSet(&pbTest[X86_PAGE_SIZE - cbIdtr * 2], bFiller1, cbIdtr*2);
            if (off < X86_PAGE_SIZE)
                Bs3MemCpy(&pbTest[off], pbBufRestore, RT_MIN(X86_PAGE_SIZE - off, cbIdtr));
            Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, &pbTest[off]);
            Bs3MemSet(abBufSave, bFiller2, sizeof(abBufSave));
            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            if (bRing != 0)
                bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
            else if (off + cbIdtr <= X86_PAGE_SIZE)
            {
                CtxUdExpected.rbx = Ctx.rbx;
                bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                if (Bs3MemCmp(pbBufSave, abExpectedFilled, cbIdtr*2) != 0)
                    Bs3TestFailedF("Mismatch (%s, #7): expected %.*Rhxs, got %.*Rhxs\n",
                                   pWorker->pszDesc, cbIdtr*2, abExpectedFilled, cbIdtr*2, pbBufSave);
            }
            else
                bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx, 0, uFlatTest + RT_MAX(off, X86_PAGE_SIZE));
            g_usBs3TestStep++;

            /* Again with zero limit and maybe messed up base as well (triple fault if buggy).
               The 386DX-40 here triple faults (or something) with off == 0xffe, nothing else. */
            if (   off < X86_PAGE_SIZE && off + cbIdtr > X86_PAGE_SIZE
                && (   off != X86_PAGE_SIZE - 2
                    || (g_uBs3CpuDetected & BS3CPU_TYPE_MASK) != BS3CPU_80386)
                )
            {
                pbTest[off] = 0;
                if (off + 1 < X86_PAGE_SIZE)
                    pbTest[off + 1] = 0;
                if (off + 2 < X86_PAGE_SIZE)
                    pbTest[off + 2] |= 7;
                Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
                if (bRing != 0)
                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                else
                    bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx, 0, uFlatTest + RT_MAX(off, X86_PAGE_SIZE));
                g_usBs3TestStep++;
            }
        }

        /*
         * Now, do it the other way around. It should look normal now since writing
         * the limit will #PF first and nothing should be written.
         */
        for (off = cbIdtr + 4; off >= -cbIdtr - 4; off--)
        {
            Bs3MemSet(pbTest, bFiller1, 48);
            if (off >= 0)
                Bs3MemCpy(&pbTest[off], pbBufRestore, cbIdtr);
            else if (off + cbIdtr > 0)
                Bs3MemCpy(pbTest, &pbBufRestore[-off], cbIdtr + off);
            Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rbx, pWorker->fSs ? &Ctx.ss : &Ctx.ds, &pbTest[off]);
            Bs3MemSet(abBufSave, bFiller2, sizeof(abBufSave));
            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            if (bRing != 0)
                bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
            else if (off >= 0)
            {
                CtxUdExpected.rbx = Ctx.rbx;
                bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                if (Bs3MemCmp(pbBufSave, abExpectedFilled, cbIdtr*2) != 0)
                    Bs3TestFailedF("Mismatch (%s, #8): expected %.*Rhxs, got %.*Rhxs\n",
                                   pWorker->pszDesc, cbIdtr*2, abExpectedFilled, cbIdtr*2, pbBufSave);
            }
            else
                bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx, 0, uFlatTest + off);
            g_usBs3TestStep++;

            /* Again with messed up base as well (triple fault if buggy). */
            if (off < 0 && off > -cbIdtr)
            {
                if (off + 2 >= 0)
                    pbTest[off + 2] |= 15;
                pbTest[off + cbIdtr - 1] ^= 0xaa;
                Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
                if (bRing != 0)
                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                else
                    bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx, 0, uFlatTest + off);
                g_usBs3TestStep++;
            }
        }

        /*
         * Combine paging and segment limit and check ordering.
         * This is kind of interesting here since it the instruction seems to
         * actually be doing two separate read, just like it's S[IG]DT counterpart.
         *
         * Note! My 486DX4 does a DWORD limit read when the operand size is 32-bit,
         *       that's what f486Weirdness deals with.
         */
        if (   !BS3_MODE_IS_RM_OR_V86(bTestMode)
            && !BS3_MODE_IS_64BIT_CODE(bTestMode))
        {
            bool const f486Weirdness = (g_uBs3CpuDetected & BS3CPU_TYPE_MASK) == BS3CPU_80486
                                    && BS3_MODE_IS_32BIT_CODE(bTestMode) == !(pWorker->fFlags & BS3CB2SIDTSGDT_F_OPSIZE);
            uint16_t   cbLimit;

            Bs3GdteTestPage00 = Bs3Gdte_DATA16;
            Bs3GdteTestPage00.Gen.u2Dpl       = bRing;
            Bs3GdteTestPage00.Gen.u16BaseLow  = (uint16_t)uFlatTest;
            Bs3GdteTestPage00.Gen.u8BaseHigh1 = (uint8_t)(uFlatTest >> 16);
            Bs3GdteTestPage00.Gen.u8BaseHigh2 = (uint8_t)(uFlatTest >> 24);

            if (pWorker->fSs)
                CtxUdExpected.ss = Ctx.ss = BS3_SEL_TEST_PAGE_00 | bRing;
            else
                CtxUdExpected.ds = Ctx.ds = BS3_SEL_TEST_PAGE_00 | bRing;

            /* Expand up (normal), approaching tail guard page. */
            for (off = X86_PAGE_SIZE - cbIdtr - 4; off < X86_PAGE_SIZE + 4; off++)
            {
                CtxUdExpected.rbx.u = Ctx.rbx.u = off;
                for (cbLimit = X86_PAGE_SIZE - cbIdtr*2; cbLimit < X86_PAGE_SIZE + cbIdtr*2; cbLimit++)
                {
                    Bs3GdteTestPage00.Gen.u16LimitLow = cbLimit;
                    Bs3MemSet(&pbTest[X86_PAGE_SIZE - cbIdtr * 2], bFiller1, cbIdtr * 2);
                    if (off < X86_PAGE_SIZE)
                        Bs3MemCpy(&pbTest[off], pbBufRestore, RT_MIN(cbIdtr, X86_PAGE_SIZE - off));
                    Bs3MemSet(abBufSave, bFiller2, sizeof(abBufSave));
                    Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
                    if (bRing != 0)
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                    else if (off + cbIdtr <= cbLimit + 1)
                    {
                        /* No #GP, but maybe #PF. */
                        if (off + cbIdtr <= X86_PAGE_SIZE)
                        {
                            bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                            if (Bs3MemCmp(pbBufSave, abExpectedFilled, cbIdtr * 2) != 0)
                                Bs3TestFailedF("Mismatch (%s, #9): expected %.*Rhxs, got %.*Rhxs\n",
                                               pWorker->pszDesc, cbIdtr*2, abExpectedFilled, cbIdtr*2, pbBufSave);
                        }
                        else
                            bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx, 0, uFlatTest + RT_MAX(off, X86_PAGE_SIZE));
                    }
                    /* No #GP/#SS on limit, but instead #PF? */
                    else if (  !f486Weirdness
                             ? off     < cbLimit && off >= 0xfff
                             : off + 2 < cbLimit && off >= 0xffd)
                        bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx, 0, uFlatTest + RT_MAX(off, X86_PAGE_SIZE));
                    /* #GP/#SS on limit or base. */
                    else if (pWorker->fSs)
                        bs3CpuBasic2_CompareSsCtx(&TrapCtx, &Ctx, 0, false /*f486ResumeFlagHint*/);
                    else
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);

                    g_usBs3TestStep++;

                    /* Set DS to 0 and check that we get #GP(0). */
                    if (!pWorker->fSs)
                    {
                        Ctx.ds = 0;
                        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                        Ctx.ds = BS3_SEL_TEST_PAGE_00 | bRing;
                        g_usBs3TestStep++;
                    }
                }
            }

            /* Expand down. */
            pbTest    -= X86_PAGE_SIZE; /* Note! we're backing up a page to simplify things */
            uFlatTest -= X86_PAGE_SIZE;

            Bs3GdteTestPage00.Gen.u4Type = X86_SEL_TYPE_RW_DOWN_ACC;
            Bs3GdteTestPage00.Gen.u16BaseLow  = (uint16_t)uFlatTest;
            Bs3GdteTestPage00.Gen.u8BaseHigh1 = (uint8_t)(uFlatTest >> 16);
            Bs3GdteTestPage00.Gen.u8BaseHigh2 = (uint8_t)(uFlatTest >> 24);

            for (off = X86_PAGE_SIZE - cbIdtr - 4; off < X86_PAGE_SIZE + 4; off++)
            {
                CtxUdExpected.rbx.u = Ctx.rbx.u = off;
                for (cbLimit = X86_PAGE_SIZE - cbIdtr*2; cbLimit < X86_PAGE_SIZE + cbIdtr*2; cbLimit++)
                {
                    Bs3GdteTestPage00.Gen.u16LimitLow = cbLimit;
                    Bs3MemSet(&pbTest[X86_PAGE_SIZE], bFiller1, cbIdtr * 2);
                    if (off >= X86_PAGE_SIZE)
                        Bs3MemCpy(&pbTest[off], pbBufRestore, cbIdtr);
                    else if (off > X86_PAGE_SIZE - cbIdtr)
                        Bs3MemCpy(&pbTest[X86_PAGE_SIZE], &pbBufRestore[X86_PAGE_SIZE - off], cbIdtr - (X86_PAGE_SIZE - off));
                    Bs3MemSet(abBufSave, bFiller2, sizeof(abBufSave));
                    Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
                    if (bRing != 0)
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                    else if (cbLimit < off && off >= X86_PAGE_SIZE)
                    {
                        bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                        if (Bs3MemCmp(pbBufSave, abExpectedFilled, cbIdtr * 2) != 0)
                            Bs3TestFailedF("Mismatch (%s, #10): expected %.*Rhxs, got %.*Rhxs\n",
                                           pWorker->pszDesc, cbIdtr*2, abExpectedFilled, cbIdtr*2, pbBufSave);
                    }
                    else if (cbLimit < off && off < X86_PAGE_SIZE)
                        bs3CpuBasic2_ComparePfCtx(&TrapCtx, &Ctx, 0, uFlatTest + off);
                    else if (pWorker->fSs)
                        bs3CpuBasic2_CompareSsCtx(&TrapCtx, &Ctx, 0, false /*f486ResumeFlagHint*/);
                    else
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
                    g_usBs3TestStep++;
                }
            }

            pbTest    += X86_PAGE_SIZE;
            uFlatTest += X86_PAGE_SIZE;
        }

        Bs3MemGuardedTestPageFree(pbTest);
    }

    /*
     * Check non-canonical 64-bit space.
     */
    if (   BS3_MODE_IS_64BIT_CODE(bTestMode)
        && (pbTest = (uint8_t BS3_FAR *)Bs3PagingSetupCanonicalTraps()) != NULL)
    {
        /* Make our references relative to the gap. */
        pbTest += g_cbBs3PagingOneCanonicalTrap;

        /* Hit it from below. */
        for (off = -cbIdtr - 8; off < cbIdtr + 8; off++)
        {
            Ctx.rbx.u = CtxUdExpected.rbx.u = UINT64_C(0x0000800000000000) + off;
            Bs3MemSet(&pbTest[-64], bFiller1, 64*2);
            Bs3MemCpy(&pbTest[off], pbBufRestore, cbIdtr);
            Bs3MemSet(abBufSave, bFiller2, sizeof(abBufSave));
            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            if (off + cbIdtr > 0 || bRing != 0)
                bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
            else
            {
                bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                if (Bs3MemCmp(pbBufSave, abExpectedFilled, cbIdtr * 2) != 0)
                    Bs3TestFailedF("Mismatch (%s, #11): expected %.*Rhxs, got %.*Rhxs\n",
                                   pWorker->pszDesc, cbIdtr*2, abExpectedFilled, cbIdtr*2, pbBufSave);
            }
        }

        /* Hit it from above. */
        for (off = -cbIdtr - 8; off < cbIdtr + 8; off++)
        {
            Ctx.rbx.u = CtxUdExpected.rbx.u = UINT64_C(0xffff800000000000) + off;
            Bs3MemSet(&pbTest[-64], bFiller1, 64*2);
            Bs3MemCpy(&pbTest[off], pbBufRestore, cbIdtr);
            Bs3MemSet(abBufSave, bFiller2, sizeof(abBufSave));
            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            if (off < 0 || bRing != 0)
                bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
            else
            {
                bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
                if (Bs3MemCmp(pbBufSave, abExpectedFilled, cbIdtr * 2) != 0)
                    Bs3TestFailedF("Mismatch (%s, #19): expected %.*Rhxs, got %.*Rhxs\n",
                                   pWorker->pszDesc, cbIdtr*2, abExpectedFilled, cbIdtr*2, pbBufSave);
            }
        }

    }
}


static void bs3CpuBasic2_lidt_lgdt_Common(uint8_t bTestMode, BS3CB2SIDTSGDT const BS3_FAR *paWorkers, unsigned cWorkers,
                                          void const *pvRestore, size_t cbRestore, uint8_t const *pbExpected)
{
    unsigned idx;
    unsigned bRing;
    unsigned iStep = 0;

    /* Note! We skip the SS checks for ring-0 since we badly mess up SS in the
             test and don't want to bother with double faults. */
    for (bRing = BS3_MODE_IS_V86(bTestMode) ? 3 : 0; bRing <= 3; bRing++)
    {
        for (idx = 0; idx < cWorkers; idx++)
            if (    (paWorkers[idx].bMode & (bTestMode & BS3_MODE_CODE_MASK))
                && (!paWorkers[idx].fSs || bRing != 0 /** @todo || BS3_MODE_IS_64BIT_SYS(bTestMode)*/ )
                && (   !(paWorkers[idx].fFlags & BS3CB2SIDTSGDT_F_386PLUS)
                    || (   bTestMode > BS3_MODE_PE16
                        || (   bTestMode == BS3_MODE_PE16
                            && (g_uBs3CpuDetected & BS3CPU_TYPE_MASK) >= BS3CPU_80386)) ) )
            {
                //Bs3TestPrintf("idx=%-2d fpfnWorker=%p fSs=%d cbInstr=%d\n",
                //              idx, paWorkers[idx].fpfnWorker, paWorkers[idx].fSs, paWorkers[idx].cbInstr);
                g_usBs3TestStep = iStep;
                bs3CpuBasic2_lidt_lgdt_One(&paWorkers[idx], bTestMode, bRing, pvRestore, cbRestore, pbExpected);
                iStep += 1000;
            }
        if (BS3_MODE_IS_RM_SYS(bTestMode))
            break;
    }
}


BS3_DECL_FAR(uint8_t) BS3_CMN_FAR_NM(bs3CpuBasic2_lidt)(uint8_t bMode)
{
    union
    {
        RTIDTR  Idtr;
        uint8_t ab[32]; /* At least cbIdtr*2! */
    } Expected;

    //if (bMode != BS3_MODE_LM64) return 0;
    bs3CpuBasic2_SetGlobals(bMode);

    /*
     * Pass to common worker which is only compiled once per mode.
     */
    Bs3MemZero(&Expected, sizeof(Expected));
    ASMGetIDTR(&Expected.Idtr);

    if (BS3_MODE_IS_RM_SYS(bMode))
        bs3CpuBasic2_lidt_lgdt_Common(bMode, g_aLidtWorkers, RT_ELEMENTS(g_aLidtWorkers),
                                      &Bs3Lidt_Ivt, sizeof(Bs3Lidt_Ivt), Expected.ab);
    else if (BS3_MODE_IS_16BIT_SYS(bMode))
        bs3CpuBasic2_lidt_lgdt_Common(bMode, g_aLidtWorkers, RT_ELEMENTS(g_aLidtWorkers),
                                      &Bs3Lidt_Idt16, sizeof(Bs3Lidt_Idt16), Expected.ab);
    else if (BS3_MODE_IS_32BIT_SYS(bMode))
        bs3CpuBasic2_lidt_lgdt_Common(bMode, g_aLidtWorkers, RT_ELEMENTS(g_aLidtWorkers),
                                      &Bs3Lidt_Idt32, sizeof(Bs3Lidt_Idt32), Expected.ab);
    else
        bs3CpuBasic2_lidt_lgdt_Common(bMode, g_aLidtWorkers, RT_ELEMENTS(g_aLidtWorkers),
                                      &Bs3Lidt_Idt64, sizeof(Bs3Lidt_Idt64), Expected.ab);

    /*
     * Re-initialize the IDT.
     */
    Bs3TrapReInit();
    return 0;
}


BS3_DECL_FAR(uint8_t) BS3_CMN_FAR_NM(bs3CpuBasic2_lgdt)(uint8_t bMode)
{
    union
    {
        RTGDTR  Gdtr;
        uint8_t ab[32]; /* At least cbIdtr*2! */
    } Expected;

    //if (!BS3_MODE_IS_64BIT_SYS(bMode)) return 0;
    bs3CpuBasic2_SetGlobals(bMode);

    /*
     * Pass to common worker which is only compiled once per mode.
     */
    if (BS3_MODE_IS_RM_SYS(bMode))
        ASMSetGDTR((PRTGDTR)&Bs3LgdtDef_Gdt);
    Bs3MemZero(&Expected, sizeof(Expected));
    ASMGetGDTR(&Expected.Gdtr);

    bs3CpuBasic2_lidt_lgdt_Common(bMode, g_aLgdtWorkers, RT_ELEMENTS(g_aLgdtWorkers),
                                  &Bs3LgdtDef_Gdt, sizeof(Bs3LgdtDef_Gdt), Expected.ab);

    /*
     * Re-initialize the IDT.
     */
    Bs3TrapReInit();
    return 0;
}

typedef union IRETBUF
{
    uint64_t        au64[6];  /* max req is 5 */
    uint32_t        au32[12]; /* max req is 9 */
    uint16_t        au16[24]; /* max req is 5 */
    uint8_t         ab[48];
} IRETBUF;
typedef IRETBUF BS3_FAR *PIRETBUF;


static void iretbuf_SetupFrame(PIRETBUF pIretBuf, unsigned const cbPop,
                               uint16_t uCS, uint64_t uPC, uint32_t fEfl, uint16_t uSS, uint64_t uSP)
{
     if (cbPop == 2)
     {
         pIretBuf->au16[0] = (uint16_t)uPC;
         pIretBuf->au16[1] = uCS;
         pIretBuf->au16[2] = (uint16_t)fEfl;
         pIretBuf->au16[3] = (uint16_t)uSP;
         pIretBuf->au16[4] = uSS;
     }
     else if (cbPop != 8)
     {
         pIretBuf->au32[0]   = (uint32_t)uPC;
         pIretBuf->au16[1*2] = uCS;
         pIretBuf->au32[2]   = (uint32_t)fEfl;
         pIretBuf->au32[3]   = (uint32_t)uSP;
         pIretBuf->au16[4*2] = uSS;
     }
     else
     {
         pIretBuf->au64[0]   = uPC;
         pIretBuf->au16[1*4] = uCS;
         pIretBuf->au64[2]   = fEfl;
         pIretBuf->au64[3]   = uSP;
         pIretBuf->au16[4*4] = uSS;
     }
}

uint32_t ASMGetESP(void);
#pragma aux ASMGetESP = \
    ".386" \
    "mov ax, sp" \
    "mov edx, esp" \
    "shr edx, 16" \
    value [ax dx] \
    modify exact [ax dx];


static void bs3CpuBasic2_iret_Worker(uint8_t bTestMode, FPFNBS3FAR pfnIret, unsigned const cbPop,
                                     PIRETBUF pIretBuf, const char BS3_FAR *pszDesc)
{
    BS3TRAPFRAME        TrapCtx;
    BS3REGCTX           Ctx;
    BS3REGCTX           CtxUdExpected;
    BS3REGCTX           TmpCtx;
    BS3REGCTX           TmpCtxExpected;
    uint8_t             abLowUd[8];
    uint8_t             abLowIret[8];
    FPFNBS3FAR          pfnUdLow = (FPFNBS3FAR)abLowUd;
    FPFNBS3FAR          pfnIretLow = (FPFNBS3FAR)abLowIret;
    unsigned const      cbSameCplFrame = BS3_MODE_IS_64BIT_CODE(bTestMode) ? 5*cbPop : 3*cbPop;
    bool const          fUseLowCode = cbPop == 2 && !BS3_MODE_IS_16BIT_CODE(bTestMode);
    int                 iRingDst;
    int                 iRingSrc;
    uint16_t            uDplSs;
    uint16_t            uRplCs;
    uint16_t            uRplSs;
//    int                 i;
    uint8_t BS3_FAR    *pbTest;

    NOREF(abLowUd);
#define IRETBUF_SET_SEL(a_idx, a_uValue) \
        do { *(uint16_t)&pIretBuf->ab[a_idx * cbPop] = (a_uValue); } while (0)
#define IRETBUF_SET_REG(a_idx, a_uValue) \
        do { uint8_t const BS3_FAR *pbTmp = &pIretBuf->ab[a_idx * cbPop]; \
            if (cbPop == 2)       *(uint16_t)pbTmp = (uint16_t)(a_uValue); \
             else if (cbPop != 8) *(uint32_t)pbTmp = (uint32_t)(a_uValue); \
             else                 *(uint64_t)pbTmp = (a_uValue); \
         } while (0)

    /* make sure they're allocated  */
    Bs3MemZero(&Ctx, sizeof(Ctx));
    Bs3MemZero(&CtxUdExpected, sizeof(CtxUdExpected));
    Bs3MemZero(&TmpCtx, sizeof(TmpCtx));
    Bs3MemZero(&TmpCtxExpected, sizeof(TmpCtxExpected));
    Bs3MemZero(&TrapCtx, sizeof(TrapCtx));

    /*
     * When dealing with 16-bit irets in 32-bit or 64-bit mode, we must have
     * copies of both iret and ud in the first 64KB of memory.  The stack is
     * below 64KB, so we'll just copy the instructions onto the stack.
     */
    Bs3MemCpy(abLowUd, bs3CpuBasic2_ud2, 4);
    Bs3MemCpy(abLowIret, pfnIret, 4);

    /*
     * Create a context (stack is irrelevant, we'll mainly be using pIretBuf).
     *  - Point the context at our iret instruction.
     *  - Point SS:xSP at pIretBuf.
     */
    Bs3RegCtxSaveEx(&Ctx, bTestMode, 0);
    if (!fUseLowCode)
        Bs3RegCtxSetRipCsFromLnkPtr(&Ctx, pfnIret);
    else
        Bs3RegCtxSetRipCsFromCurPtr(&Ctx, pfnIretLow);
    if (BS3_MODE_IS_16BIT_SYS(bTestMode))
        g_uBs3TrapEipHint = Ctx.rip.u32;
    Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rsp, &Ctx.ss, pIretBuf);

    /*
     * The first success (UD) context keeps the same code bit-count as the iret.
     */
    Bs3MemCpy(&CtxUdExpected, &Ctx, sizeof(Ctx));
    if (!fUseLowCode)
        Bs3RegCtxSetRipCsFromLnkPtr(&CtxUdExpected, bs3CpuBasic2_ud2);
    else
        Bs3RegCtxSetRipCsFromCurPtr(&CtxUdExpected, pfnUdLow);
    CtxUdExpected.rsp.u += cbSameCplFrame;

    /*
     * Check that it works at all.
     */
    iretbuf_SetupFrame(pIretBuf, cbPop, CtxUdExpected.cs, CtxUdExpected.rip.u,
                       CtxUdExpected.rflags.u32, CtxUdExpected.ss, CtxUdExpected.rsp.u);

    Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
    bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
    g_usBs3TestStep++;

    if (!BS3_MODE_IS_RM_OR_V86(bTestMode))
    {
        /* Selectors are modified when switching rings, so we need to know
           what we're dealing with there. */
        if (   !BS3_SEL_IS_IN_R0_RANGE(Ctx.cs) || !BS3_SEL_IS_IN_R0_RANGE(Ctx.ss)
            || !BS3_SEL_IS_IN_R0_RANGE(Ctx.ds) || !BS3_SEL_IS_IN_R0_RANGE(Ctx.es))
            Bs3TestFailedF("Expected R0 CS, SS, DS and ES; not %#x, %#x, %#x and %#x\n", Ctx.cs, Ctx.ss, Ctx.ds, Ctx.es);
        if (Ctx.fs || Ctx.gs)
            Bs3TestFailed("Expected R0 FS and GS to be 0!\n");

        /*
         * Test returning to outer rings if protected mode.
         */
        Bs3MemCpy(&TmpCtx, &Ctx, sizeof(TmpCtx));
        Bs3MemCpy(&TmpCtxExpected, &CtxUdExpected, sizeof(TmpCtxExpected));
        for (iRingDst = 3; iRingDst >= 0; iRingDst--)
        {
            Bs3RegCtxConvertToRingX(&TmpCtxExpected, iRingDst);
            TmpCtxExpected.ds = iRingDst ? 0 : TmpCtx.ds;
            TmpCtx.es = TmpCtxExpected.es;
            iretbuf_SetupFrame(pIretBuf, cbPop, TmpCtxExpected.cs, TmpCtxExpected.rip.u,
                               TmpCtxExpected.rflags.u32, TmpCtxExpected.ss, TmpCtxExpected.rsp.u);
            Bs3TrapSetJmpAndRestore(&TmpCtx, &TrapCtx);
            bs3CpuBasic2_CompareUdCtx(&TrapCtx, &TmpCtxExpected);
            g_usBs3TestStep++;
        }

        /*
         * Check CS.RPL and SS.RPL.
         */
        for (iRingDst = 3; iRingDst >= 0; iRingDst--)
        {
            uint16_t const uDstSsR0 = (CtxUdExpected.ss & BS3_SEL_RING_SUB_MASK) + BS3_SEL_R0_FIRST;
            Bs3MemCpy(&TmpCtxExpected, &CtxUdExpected, sizeof(TmpCtxExpected));
            Bs3RegCtxConvertToRingX(&TmpCtxExpected, iRingDst);
            for (iRingSrc = 3; iRingSrc >= 0; iRingSrc--)
            {
                Bs3MemCpy(&TmpCtx, &Ctx, sizeof(TmpCtx));
                Bs3RegCtxConvertToRingX(&TmpCtx, iRingSrc);
                TmpCtx.es         = TmpCtxExpected.es;
                TmpCtxExpected.ds = iRingDst != iRingSrc ? 0 : TmpCtx.ds;
                for (uRplCs = 0; uRplCs <= 3; uRplCs++)
                {
                    uint16_t const uSrcEs = TmpCtx.es;
                    uint16_t const uDstCs = (TmpCtxExpected.cs & X86_SEL_MASK_OFF_RPL) | uRplCs;
                    //Bs3TestPrintf("dst=%d src=%d rplCS=%d\n", iRingDst, iRingSrc, uRplCs);

                    /* CS.RPL */
                    iretbuf_SetupFrame(pIretBuf, cbPop, uDstCs, TmpCtxExpected.rip.u, TmpCtxExpected.rflags.u32,
                                       TmpCtxExpected.ss, TmpCtxExpected.rsp.u);
                    Bs3TrapSetJmpAndRestore(&TmpCtx, &TrapCtx);
                    if (uRplCs == iRingDst && iRingDst >= iRingSrc)
                        bs3CpuBasic2_CompareUdCtx(&TrapCtx, &TmpCtxExpected);
                    else
                    {
                        if (iRingDst < iRingSrc)
                            TmpCtx.es = 0;
                        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &TmpCtx, uDstCs & X86_SEL_MASK_OFF_RPL);
                        TmpCtx.es = uSrcEs;
                    }
                    g_usBs3TestStep++;

                    /* SS.RPL */
                    if (iRingDst != iRingSrc || BS3_MODE_IS_64BIT_CODE(bTestMode))
                    {
                        uint16_t uSavedDstSs = TmpCtxExpected.ss;
                        for (uRplSs = 0; uRplSs <= 3; uRplSs++)
                        {
                            /* SS.DPL (iRingDst == CS.DPL) */
                            for (uDplSs = 0; uDplSs <= 3; uDplSs++)
                            {
                                uint16_t const uDstSs = ((uDplSs << BS3_SEL_RING_SHIFT) | uRplSs) + uDstSsR0;
                                //Bs3TestPrintf("dst=%d src=%d rplCS=%d rplSS=%d dplSS=%d dst %04x:%08RX64 %08RX32 %04x:%08RX64\n",
                                //              iRingDst, iRingSrc, uRplCs, uRplSs, uDplSs, uDstCs, TmpCtxExpected.rip.u,
                                //              TmpCtxExpected.rflags.u32, uDstSs, TmpCtxExpected.rsp.u);

                                iretbuf_SetupFrame(pIretBuf, cbPop, uDstCs, TmpCtxExpected.rip.u,
                                                   TmpCtxExpected.rflags.u32, uDstSs, TmpCtxExpected.rsp.u);
                                Bs3TrapSetJmpAndRestore(&TmpCtx, &TrapCtx);
                                if (uRplCs != iRingDst || iRingDst < iRingSrc)
                                {
                                    if (iRingDst < iRingSrc)
                                        TmpCtx.es = 0;
                                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &TmpCtx, uDstCs & X86_SEL_MASK_OFF_RPL);
                                }
                                else if (uRplSs != iRingDst || uDplSs != iRingDst)
                                    bs3CpuBasic2_CompareGpCtx(&TrapCtx, &TmpCtx, uDstSs & X86_SEL_MASK_OFF_RPL);
                                else
                                    bs3CpuBasic2_CompareUdCtx(&TrapCtx, &TmpCtxExpected);
                                TmpCtx.es = uSrcEs;
                                g_usBs3TestStep++;
                            }
                        }

                        TmpCtxExpected.ss = uSavedDstSs;
                    }
                }
            }
        }
    }

    /*
     * Special 64-bit checks.
     */
    if (BS3_MODE_IS_64BIT_CODE(bTestMode))
    {
        /* The VM flag is completely ignored. */
        iretbuf_SetupFrame(pIretBuf, cbPop, CtxUdExpected.cs, CtxUdExpected.rip.u,
                           CtxUdExpected.rflags.u32 | X86_EFL_VM, CtxUdExpected.ss, CtxUdExpected.rsp.u);
        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
        g_usBs3TestStep++;

        /* The NT flag can be loaded just fine. */
        CtxUdExpected.rflags.u32 |= X86_EFL_NT;
        iretbuf_SetupFrame(pIretBuf, cbPop, CtxUdExpected.cs, CtxUdExpected.rip.u,
                           CtxUdExpected.rflags.u32, CtxUdExpected.ss, CtxUdExpected.rsp.u);
        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        bs3CpuBasic2_CompareUdCtx(&TrapCtx, &CtxUdExpected);
        CtxUdExpected.rflags.u32 &= ~X86_EFL_NT;
        g_usBs3TestStep++;

        /* However, we'll #GP(0) if it's already set (in RFLAGS) when executing IRET. */
        Ctx.rflags.u32 |= X86_EFL_NT;
        iretbuf_SetupFrame(pIretBuf, cbPop, CtxUdExpected.cs, CtxUdExpected.rip.u,
                           CtxUdExpected.rflags.u32, CtxUdExpected.ss, CtxUdExpected.rsp.u);
        Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
        bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
        g_usBs3TestStep++;

        /* The NT flag #GP(0) should trump all other exceptions - pit it against #PF. */
        pbTest = (uint8_t BS3_FAR *)Bs3MemGuardedTestPageAlloc(BS3MEMKIND_TILED);
        if (pbTest != NULL)
        {
            Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rsp, &Ctx.ss, &pbTest[X86_PAGE_SIZE]);
            iretbuf_SetupFrame(pIretBuf, cbPop, CtxUdExpected.cs, CtxUdExpected.rip.u,
                               CtxUdExpected.rflags.u32, CtxUdExpected.ss, CtxUdExpected.rsp.u);
            Bs3TrapSetJmpAndRestore(&Ctx, &TrapCtx);
            bs3CpuBasic2_CompareGpCtx(&TrapCtx, &Ctx, 0);
            g_usBs3TestStep++;

            Bs3RegCtxSetGrpSegFromCurPtr(&Ctx, &Ctx.rsp, &Ctx.ss, pIretBuf);
            Bs3MemGuardedTestPageFree(pbTest);
        }
        Ctx.rflags.u32 &= ~X86_EFL_NT;
    }
}


BS3_DECL_FAR(uint8_t) BS3_CMN_FAR_NM(bs3CpuBasic2_iret)(uint8_t bMode)
{
    struct
    {
        uint8_t abExtraStack[4096]; /**< we've got ~30KB of stack, so 4KB for the trap handlers++ is not a problem. */
        IRETBUF IRetBuf;
        uint8_t abGuard[32];
    } uBuf;
    size_t cbUnused;

    //if (bMode != BS3_MODE_LM64) return BS3TESTDOMODE_SKIPPED;
    bs3CpuBasic2_SetGlobals(bMode);

    /*
     * Primary instruction form.
     */
    Bs3MemSet(&uBuf, 0xaa, sizeof(uBuf));
    Bs3MemSet(uBuf.abGuard, 0x88, sizeof(uBuf.abGuard));
    if (BS3_MODE_IS_16BIT_CODE(bMode))
        bs3CpuBasic2_iret_Worker(bMode, bs3CpuBasic2_iret,              2, &uBuf.IRetBuf, "iret");
    else if (BS3_MODE_IS_32BIT_CODE(bMode))
        bs3CpuBasic2_iret_Worker(bMode, bs3CpuBasic2_iret,              4, &uBuf.IRetBuf, "iretd");
    else
        bs3CpuBasic2_iret_Worker(bMode, bs3CpuBasic2_iret_rexw,         8, &uBuf.IRetBuf, "o64 iret");

    BS3_ASSERT(ASMMemIsAllU8(uBuf.abGuard, sizeof(uBuf.abGuard), 0x88));
    cbUnused = (uintptr_t)ASMMemFirstMismatchingU8(uBuf.abExtraStack, sizeof(uBuf.abExtraStack) + sizeof(uBuf.IRetBuf), 0xaa)
             - (uintptr_t)uBuf.abExtraStack;
    if (cbUnused < 2048)
        Bs3TestFailedF("cbUnused=%u #%u\n", cbUnused, 1);

    /*
     * Secondary variation: opsize prefixed.
     */
    Bs3MemSet(&uBuf, 0xaa, sizeof(uBuf));
    Bs3MemSet(uBuf.abGuard, 0x88, sizeof(uBuf.abGuard));
    if (BS3_MODE_IS_16BIT_CODE(bMode) && (g_uBs3CpuDetected & BS3CPU_TYPE_MASK) >= BS3CPU_80386)
        bs3CpuBasic2_iret_Worker(bMode, bs3CpuBasic2_iret_opsize,       4, &uBuf.IRetBuf, "o32 iret");
    else if (BS3_MODE_IS_32BIT_CODE(bMode))
        bs3CpuBasic2_iret_Worker(bMode, bs3CpuBasic2_iret_opsize,       2, &uBuf.IRetBuf, "o16 iret");
    else if (BS3_MODE_IS_64BIT_CODE(bMode))
        bs3CpuBasic2_iret_Worker(bMode, bs3CpuBasic2_iret,              4, &uBuf.IRetBuf, "iretd");
    BS3_ASSERT(ASMMemIsAllU8(uBuf.abGuard, sizeof(uBuf.abGuard), 0x88));
    cbUnused = (uintptr_t)ASMMemFirstMismatchingU8(uBuf.abExtraStack, sizeof(uBuf.abExtraStack) + sizeof(uBuf.IRetBuf), 0xaa)
             - (uintptr_t)uBuf.abExtraStack;
    if (cbUnused < 2048)
        Bs3TestFailedF("cbUnused=%u #%u\n", cbUnused, 2);

    /*
     * Third variation: 16-bit in 64-bit mode (truely unlikely)
     */
    if (BS3_MODE_IS_64BIT_CODE(bMode))
    {
        Bs3MemSet(&uBuf, 0xaa, sizeof(uBuf));
        Bs3MemSet(uBuf.abGuard, 0x88, sizeof(uBuf.abGuard));
        bs3CpuBasic2_iret_Worker(bMode, bs3CpuBasic2_iret_opsize,       2, &uBuf.IRetBuf, "o16 iret");
        BS3_ASSERT(ASMMemIsAllU8(uBuf.abGuard, sizeof(uBuf.abGuard), 0x88));
        cbUnused = (uintptr_t)ASMMemFirstMismatchingU8(uBuf.abExtraStack, sizeof(uBuf.abExtraStack) + sizeof(uBuf.IRetBuf), 0xaa)
                 - (uintptr_t)uBuf.abExtraStack;
        if (cbUnused < 2048)
            Bs3TestFailedF("cbUnused=%u #%u\n", cbUnused, 3);
    }

    return 0;
}

