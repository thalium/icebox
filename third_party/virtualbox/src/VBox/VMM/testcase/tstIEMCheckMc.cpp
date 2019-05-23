/* $Id: tstIEMCheckMc.cpp $ */
/** @file
 * IEM Testcase - Check the "Microcode".
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
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
#include <iprt/assert.h>
#include <iprt/rand.h>
#include <iprt/test.h>

#include <VBox/types.h>
#include <VBox/err.h>
#include <VBox/log.h>
#define IN_TSTVMSTRUCT 1
#include "../include/IEMInternal.h"
#include <VBox/vmm/vm.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
bool volatile       g_fRandom;
uint8_t volatile    g_bRandom;
RTUINT128U          g_u128Zero;


/** For hacks.  */
#define TST_IEM_CHECK_MC

#define CHK_TYPE(a_ExpectedType, a_Param) \
    do { a_ExpectedType const * pCheckType = &(a_Param); NOREF(pCheckType); } while (0)
#define CHK_PTYPE(a_ExpectedType, a_Param) \
    do { a_ExpectedType pCheckType = (a_Param); NOREF(pCheckType); } while (0)

#define CHK_CONST(a_ExpectedType, a_Const) \
    do { \
        AssertCompile(((a_Const) >> 1) == ((a_Const) >> 1)); \
        AssertCompile((a_ExpectedType)(a_Const) == (a_Const)); \
    } while (0)

#define CHK_SINGLE_BIT(a_ExpectedType, a_fBitMask) \
    do { \
        CHK_CONST(a_ExpectedType, a_fBitMask); \
        AssertCompile(RT_IS_POWER_OF_TWO(a_fBitMask)); \
    } while (0)

#define CHK_GCPTR(a_EffAddr) \
    CHK_TYPE(RTGCPTR, a_EffAddr)

#define CHK_SEG_IDX(a_iSeg) \
    do { \
        uint8_t iMySeg = (a_iSeg); NOREF(iMySeg); /** @todo const or variable. grr. */ \
    } while (0)

#define CHK_CALL_ARG(a_Name, a_iArg) \
    do { RT_CONCAT3(iArgCheck_,a_iArg,a_Name) = 1; } while (0)


/** @name Other stubs.
 * @{   */

typedef VBOXSTRICTRC (* PFNIEMOP)(PVMCPU pVCpu);
#define FNIEMOP_DEF(a_Name) \
    static VBOXSTRICTRC a_Name(PVMCPU pVCpu) RT_NO_THROW_DEF
#define FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) \
    static VBOXSTRICTRC a_Name(PVMCPU pVCpu, a_Type0 a_Name0) RT_NO_THROW_DEF
#define FNIEMOP_DEF_2(a_Name, a_Type0, a_Name0, a_Type1, a_Name1) \
    static VBOXSTRICTRC a_Name(PVMCPU pVCpu, a_Type0 a_Name0, a_Type1 a_Name1) RT_NO_THROW_DEF

typedef VBOXSTRICTRC (* PFNIEMOPRM)(PVMCPU pVCpu, uint8_t bRm);
#define FNIEMOPRM_DEF(a_Name) \
    static VBOXSTRICTRC a_Name(PVMCPU pVCpu, uint8_t bRm) RT_NO_THROW_DEF

#define IEM_NOT_REACHED_DEFAULT_CASE_RET()                  default: return VERR_IPE_NOT_REACHED_DEFAULT_CASE
#define IEM_RETURN_ASPECT_NOT_IMPLEMENTED()                 return IEM_RETURN_ASPECT_NOT_IMPLEMENTED
#define IEM_RETURN_ASPECT_NOT_IMPLEMENTED_LOG(a_LoggerArgs) return IEM_RETURN_ASPECT_NOT_IMPLEMENTED


#define IEM_OPCODE_GET_NEXT_U8(a_pu8)                       do { *(a_pu8)  = g_bRandom; CHK_PTYPE(uint8_t  *, a_pu8);  } while (0)
#define IEM_OPCODE_GET_NEXT_S8(a_pi8)                       do { *(a_pi8)  = g_bRandom; CHK_PTYPE(int8_t   *, a_pi8);  } while (0)
#define IEM_OPCODE_GET_NEXT_S8_SX_U16(a_pu16)               do { *(a_pu16) = g_bRandom; CHK_PTYPE(uint16_t *, a_pu16); } while (0)
#define IEM_OPCODE_GET_NEXT_S8_SX_U32(a_pu32)               do { *(a_pu32) = g_bRandom; CHK_PTYPE(uint32_t *, a_pu32); } while (0)
#define IEM_OPCODE_GET_NEXT_S8_SX_U64(a_pu64)               do { *(a_pu64) = g_bRandom; CHK_PTYPE(uint64_t *, a_pu64); } while (0)
#define IEM_OPCODE_GET_NEXT_U16(a_pu16)                     do { *(a_pu16) = g_bRandom; CHK_PTYPE(uint16_t *, a_pu16); } while (0)
#define IEM_OPCODE_GET_NEXT_U16_ZX_U32(a_pu32)              do { *(a_pu32) = g_bRandom; CHK_PTYPE(uint32_t *, a_pu32); } while (0)
#define IEM_OPCODE_GET_NEXT_U16_ZX_U64(a_pu64)              do { *(a_pu64) = g_bRandom; CHK_PTYPE(uint64_t *, a_pu64); } while (0)
#define IEM_OPCODE_GET_NEXT_S16(a_pi16)                     do { *(a_pi16) = g_bRandom; CHK_PTYPE(int16_t  *, a_pi16); } while (0)
#define IEM_OPCODE_GET_NEXT_U32(a_pu32)                     do { *(a_pu32) = g_bRandom; CHK_PTYPE(uint32_t *, a_pu32); } while (0)
#define IEM_OPCODE_GET_NEXT_U32_ZX_U64(a_pu64)              do { *(a_pu64) = g_bRandom; CHK_PTYPE(uint64_t *, a_pu64); } while (0)
#define IEM_OPCODE_GET_NEXT_S32(a_pi32)                     do { *(a_pi32) = g_bRandom; CHK_PTYPE(int32_t  *, a_pi32); } while (0)
#define IEM_OPCODE_GET_NEXT_S32_SX_U64(a_pu64)              do { *(a_pu64) = g_bRandom; CHK_PTYPE(uint64_t *, a_pu64); } while (0)
#define IEM_OPCODE_GET_NEXT_U64(a_pu64)                     do { *(a_pu64) = g_bRandom; CHK_PTYPE(uint64_t *, a_pu64); } while (0)
#define IEMOP_HLP_MIN_186()                                 do { } while (0)
#define IEMOP_HLP_MIN_286()                                 do { } while (0)
#define IEMOP_HLP_MIN_386()                                 do { } while (0)
#define IEMOP_HLP_MIN_386_EX(a_fTrue)                       do { } while (0)
#define IEMOP_HLP_MIN_486()                                 do { } while (0)
#define IEMOP_HLP_MIN_586()                                 do { } while (0)
#define IEMOP_HLP_MIN_686()                                 do { } while (0)
#define IEMOP_HLP_NO_REAL_OR_V86_MODE()                     do { } while (0)
#define IEMOP_HLP_NO_64BIT()                                do { } while (0)
#define IEMOP_HLP_ONLY_64BIT()                              do { } while (0)
#define IEMOP_HLP_64BIT_OP_SIZE()                           do { } while (0)
#define IEMOP_HLP_DEFAULT_64BIT_OP_SIZE()                   do { } while (0)
#define IEMOP_HLP_CLEAR_REX_NOT_BEFORE_OPCODE(a_szPrf)      do { } while (0)
#define IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX()            do { } while (0)
#define IEMOP_HLP_DONE_VEX_DECODING()                       do { } while (0)
#define IEMOP_HLP_DONE_VEX_DECODING_L0()                    do { } while (0)
#define IEMOP_HLP_DONE_VEX_DECODING_NO_VVVV()               do { } while (0)
#define IEMOP_HLP_DONE_VEX_DECODING_L0_AND_NO_VVVV()        do { } while (0)
#define IEMOP_HLP_DONE_DECODING_NO_LOCK_REPZ_OR_REPNZ_PREFIXES()                                    do { } while (0)


#define IEMOP_HLP_DONE_DECODING()                           do { } while (0)

#define IEMOP_HLP_SVM_CTRL_INTERCEPT(a_pVCpu, a_Intercept, a_uExitCode, a_uExitInfo1, a_uExitInfo2) do { } while (0)
#define IEMOP_HLP_SVM_READ_CR_INTERCEPT(a_pVCpu, a_uCr, a_uExitInfo1, a_uExitInfo2)                 do { } while (0)

#define IEMOP_HLP_DECODED_NL_1(a_uDisOpNo, a_fIemOpFlags, a_uDisParam0, a_fDisOpType)               do { } while (0)
#define IEMOP_HLP_DECODED_NL_2(a_uDisOpNo, a_fIemOpFlags, a_uDisParam0, a_uDisParam1, a_fDisOpType) do { } while (0)
#define IEMOP_RAISE_DIVIDE_ERROR()                          VERR_TRPM_ACTIVE_TRAP
#define IEMOP_RAISE_INVALID_OPCODE()                        VERR_TRPM_ACTIVE_TRAP
#define IEMOP_RAISE_INVALID_LOCK_PREFIX()                   VERR_TRPM_ACTIVE_TRAP
#define IEMOP_MNEMONIC(a_Stats, a_szMnemonic)               do { } while (0)
#define IEMOP_MNEMONIC0EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_fDisHints, a_fIemHints) do { } while (0)
#define IEMOP_MNEMONIC1EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_fDisHints, a_fIemHints) do { } while (0)
#define IEMOP_MNEMONIC2EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_fDisHints, a_fIemHints) do { } while (0)
#define IEMOP_MNEMONIC3EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_fDisHints, a_fIemHints) do { } while (0)
#define IEMOP_MNEMONIC4EX(a_Stats, a_szMnemonic, a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_Op4, a_fDisHints, a_fIemHints) do { } while (0)
#define IEMOP_MNEMONIC0(a_Form, a_Upper, a_Lower, a_fDisHints, a_fIemHints)                         do { } while (0)
#define IEMOP_MNEMONIC1(a_Form, a_Upper, a_Lower, a_Op1, a_fDisHints, a_fIemHints)                  do { } while (0)
#define IEMOP_MNEMONIC2(a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_fDisHints, a_fIemHints)           do { } while (0)
#define IEMOP_MNEMONIC3(a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_fDisHints, a_fIemHints)    do { } while (0)
#define IEMOP_MNEMONIC4(a_Form, a_Upper, a_Lower, a_Op1, a_Op2, a_Op3, a_fDisHints, a_fIemHints)    do { } while (0)
#define IEMOP_BITCH_ABOUT_STUB()                            do { } while (0)
#define FNIEMOP_STUB(a_Name) \
    FNIEMOP_DEF(a_Name) { return VERR_NOT_IMPLEMENTED; } \
    typedef int ignore_semicolon
#define FNIEMOP_STUB_1(a_Name, a_Type0, a_Name0) \
    FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) { return VERR_NOT_IMPLEMENTED; } \
    typedef int ignore_semicolon

#define FNIEMOP_UD_STUB(a_Name) \
    FNIEMOP_DEF(a_Name) { return IEMOP_RAISE_INVALID_OPCODE(); } \
    typedef int ignore_semicolon
#define FNIEMOP_UD_STUB_1(a_Name, a_Type0, a_Name0) \
    FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) { return IEMOP_RAISE_INVALID_OPCODE(); } \
    typedef int ignore_semicolon


#define FNIEMOP_CALL(a_pfn)                                 (a_pfn)(pVCpu)
#define FNIEMOP_CALL_1(a_pfn, a0)                           (a_pfn)(pVCpu, a0)
#define FNIEMOP_CALL_2(a_pfn, a0, a1)                       (a_pfn)(pVCpu, a0, a1)

#define IEM_IS_REAL_OR_V86_MODE(a_pVCpu)                    (g_fRandom)
#define IEM_IS_LONG_MODE(a_pVCpu)                           (g_fRandom)
#define IEM_IS_REAL_MODE(a_pVCpu)                           (g_fRandom)
#define IEM_IS_GUEST_CPU_AMD(a_pVCpu)                       (g_fRandom)
#define IEM_IS_GUEST_CPU_INTEL(a_pVCpu)                     (g_fRandom)
#define IEM_GET_GUEST_CPU_FEATURES(a_pVCpu)                 ((PCCPUMFEATURES)(uintptr_t)42)
#define IEM_GET_HOST_CPU_FEATURES(a_pVCpu)                  ((PCCPUMFEATURES)(uintptr_t)88)

#define iemRecalEffOpSize(a_pVCpu)                          do { } while (0)

IEMOPBINSIZES g_iemAImpl_add;
IEMOPBINSIZES g_iemAImpl_adc;
IEMOPBINSIZES g_iemAImpl_sub;
IEMOPBINSIZES g_iemAImpl_sbb;
IEMOPBINSIZES g_iemAImpl_or;
IEMOPBINSIZES g_iemAImpl_xor;
IEMOPBINSIZES g_iemAImpl_and;
IEMOPBINSIZES g_iemAImpl_cmp;
IEMOPBINSIZES g_iemAImpl_test;
IEMOPBINSIZES g_iemAImpl_bt;
IEMOPBINSIZES g_iemAImpl_btc;
IEMOPBINSIZES g_iemAImpl_btr;
IEMOPBINSIZES g_iemAImpl_bts;
IEMOPBINSIZES g_iemAImpl_bsf;
IEMOPBINSIZES g_iemAImpl_bsr;
IEMOPBINSIZES g_iemAImpl_imul_two;
PCIEMOPBINSIZES g_apIemImplGrp1[8];
IEMOPUNARYSIZES g_iemAImpl_inc;
IEMOPUNARYSIZES g_iemAImpl_dec;
IEMOPUNARYSIZES g_iemAImpl_neg;
IEMOPUNARYSIZES g_iemAImpl_not;
IEMOPSHIFTSIZES g_iemAImpl_rol;
IEMOPSHIFTSIZES g_iemAImpl_ror;
IEMOPSHIFTSIZES g_iemAImpl_rcl;
IEMOPSHIFTSIZES g_iemAImpl_rcr;
IEMOPSHIFTSIZES g_iemAImpl_shl;
IEMOPSHIFTSIZES g_iemAImpl_shr;
IEMOPSHIFTSIZES g_iemAImpl_sar;
IEMOPMULDIVSIZES g_iemAImpl_mul;
IEMOPMULDIVSIZES g_iemAImpl_imul;
IEMOPMULDIVSIZES g_iemAImpl_div;
IEMOPMULDIVSIZES g_iemAImpl_idiv;
IEMOPSHIFTDBLSIZES g_iemAImpl_shld;
IEMOPSHIFTDBLSIZES g_iemAImpl_shrd;
IEMOPMEDIAF1L1 g_iemAImpl_punpcklbw;
IEMOPMEDIAF1L1 g_iemAImpl_punpcklwd;
IEMOPMEDIAF1L1 g_iemAImpl_punpckldq;
IEMOPMEDIAF1L1 g_iemAImpl_punpcklqdq;
IEMOPMEDIAF1H1 g_iemAImpl_punpckhbw;
IEMOPMEDIAF1H1 g_iemAImpl_punpckhwd;
IEMOPMEDIAF1H1 g_iemAImpl_punpckhdq;
IEMOPMEDIAF1H1 g_iemAImpl_punpckhqdq;
IEMOPMEDIAF2 g_iemAImpl_pxor;
IEMOPMEDIAF2 g_iemAImpl_pcmpeqb;
IEMOPMEDIAF2 g_iemAImpl_pcmpeqw;
IEMOPMEDIAF2 g_iemAImpl_pcmpeqd;


#define iemAImpl_idiv_u8    ((PFNIEMAIMPLMULDIVU8)0)
#define iemAImpl_div_u8     ((PFNIEMAIMPLMULDIVU8)0)
#define iemAImpl_imul_u8    ((PFNIEMAIMPLMULDIVU8)0)
#define iemAImpl_mul_u8     ((PFNIEMAIMPLMULDIVU8)0)

#define iemAImpl_fpu_r32_to_r80         NULL
#define iemAImpl_fcom_r80_by_r32        NULL
#define iemAImpl_fadd_r80_by_r32        NULL
#define iemAImpl_fmul_r80_by_r32        NULL
#define iemAImpl_fsub_r80_by_r32        NULL
#define iemAImpl_fsubr_r80_by_r32       NULL
#define iemAImpl_fdiv_r80_by_r32        NULL
#define iemAImpl_fdivr_r80_by_r32       NULL

#define iemAImpl_fpu_r64_to_r80         NULL
#define iemAImpl_fadd_r80_by_r64        NULL
#define iemAImpl_fmul_r80_by_r64        NULL
#define iemAImpl_fcom_r80_by_r64        NULL
#define iemAImpl_fsub_r80_by_r64        NULL
#define iemAImpl_fsubr_r80_by_r64       NULL
#define iemAImpl_fdiv_r80_by_r64        NULL
#define iemAImpl_fdivr_r80_by_r64       NULL

#define iemAImpl_fadd_r80_by_r80        NULL
#define iemAImpl_fmul_r80_by_r80        NULL
#define iemAImpl_fsub_r80_by_r80        NULL
#define iemAImpl_fsubr_r80_by_r80       NULL
#define iemAImpl_fdiv_r80_by_r80        NULL
#define iemAImpl_fdivr_r80_by_r80       NULL
#define iemAImpl_fprem_r80_by_r80       NULL
#define iemAImpl_fprem1_r80_by_r80      NULL
#define iemAImpl_fscale_r80_by_r80      NULL

#define iemAImpl_fpatan_r80_by_r80      NULL
#define iemAImpl_fyl2x_r80_by_r80       NULL
#define iemAImpl_fyl2xp1_r80_by_r80     NULL

#define iemAImpl_fcom_r80_by_r80        NULL
#define iemAImpl_fucom_r80_by_r80       NULL
#define iemAImpl_fabs_r80               NULL
#define iemAImpl_fchs_r80               NULL
#define iemAImpl_ftst_r80               NULL
#define iemAImpl_fxam_r80               NULL
#define iemAImpl_f2xm1_r80              NULL
#define iemAImpl_fsqrt_r80              NULL
#define iemAImpl_frndint_r80            NULL
#define iemAImpl_fsin_r80               NULL
#define iemAImpl_fcos_r80               NULL

#define iemAImpl_fld1                   NULL
#define iemAImpl_fldl2t                 NULL
#define iemAImpl_fldl2e                 NULL
#define iemAImpl_fldpi                  NULL
#define iemAImpl_fldlg2                 NULL
#define iemAImpl_fldln2                 NULL
#define iemAImpl_fldz                   NULL

#define iemAImpl_fptan_r80_r80          NULL
#define iemAImpl_fxtract_r80_r80        NULL
#define iemAImpl_fsincos_r80_r80        NULL

#define iemAImpl_fiadd_r80_by_i16       NULL
#define iemAImpl_fimul_r80_by_i16       NULL
#define iemAImpl_fisub_r80_by_i16       NULL
#define iemAImpl_fisubr_r80_by_i16      NULL
#define iemAImpl_fidiv_r80_by_i16       NULL
#define iemAImpl_fidivr_r80_by_i16      NULL

#define iemAImpl_fiadd_r80_by_i32       NULL
#define iemAImpl_fimul_r80_by_i32       NULL
#define iemAImpl_fisub_r80_by_i32       NULL
#define iemAImpl_fisubr_r80_by_i32      NULL
#define iemAImpl_fidiv_r80_by_i32       NULL
#define iemAImpl_fidivr_r80_by_i32      NULL

#define iemCImpl_callf                  NULL
#define iemCImpl_FarJmp                 NULL

#define iemAImpl_pshufhw                NULL
#define iemAImpl_pshuflw                NULL
#define iemAImpl_pshufd                 NULL

/** @}  */


#define IEM_REPEAT_0(a_Callback, a_User)    do { } while (0)
#define IEM_REPEAT_1(a_Callback, a_User)                                      a_Callback##_CALLBACK(0, a_User)
#define IEM_REPEAT_2(a_Callback, a_User)    IEM_REPEAT_1(a_Callback, a_User); a_Callback##_CALLBACK(1, a_User)
#define IEM_REPEAT_3(a_Callback, a_User)    IEM_REPEAT_2(a_Callback, a_User); a_Callback##_CALLBACK(2, a_User)
#define IEM_REPEAT_4(a_Callback, a_User)    IEM_REPEAT_3(a_Callback, a_User); a_Callback##_CALLBACK(3, a_User)
#define IEM_REPEAT_5(a_Callback, a_User)    IEM_REPEAT_4(a_Callback, a_User); a_Callback##_CALLBACK(4, a_User)
#define IEM_REPEAT_6(a_Callback, a_User)    IEM_REPEAT_5(a_Callback, a_User); a_Callback##_CALLBACK(5, a_User)
#define IEM_REPEAT_7(a_Callback, a_User)    IEM_REPEAT_6(a_Callback, a_User); a_Callback##_CALLBACK(6, a_User)
#define IEM_REPEAT_8(a_Callback, a_User)    IEM_REPEAT_7(a_Callback, a_User); a_Callback##_CALLBACK(7, a_User)
#define IEM_REPEAT_9(a_Callback, a_User)    IEM_REPEAT_8(a_Callback, a_User); a_Callback##_CALLBACK(8, a_User)
#define IEM_REPEAT(a_cTimes, a_Callback, a_User) RT_CONCAT(IEM_REPEAT_,a_cTimes)(a_Callback, a_User)



/** @name Microcode test stubs
 * @{  */

#define IEM_ARG_CHECK_CALLBACK(a_idx, a_User) int RT_CONCAT(iArgCheck_,a_idx); NOREF(RT_CONCAT(iArgCheck_,a_idx))
#define IEM_MC_BEGIN(a_cArgs, a_cLocals) \
    { \
        const uint8_t cArgs   = (a_cArgs); NOREF(cArgs); \
        const uint8_t cLocals = (a_cArgs); NOREF(cLocals); \
        IEM_REPEAT(a_cArgs, IEM_ARG_CHECK, 0); \

#define IEM_MC_END() \
    }

#define IEM_MC_PAUSE()                                  do {} while (0)
#define IEM_MC_CONTINUE()                               do {} while (0)
#define IEM_MC_ADVANCE_RIP()                            do {} while (0)
#define IEM_MC_REL_JMP_S8(a_i8)                         CHK_TYPE(int8_t, a_i8)
#define IEM_MC_REL_JMP_S16(a_i16)                       CHK_TYPE(int16_t, a_i16)
#define IEM_MC_REL_JMP_S32(a_i32)                       CHK_TYPE(int32_t, a_i32)
#define IEM_MC_SET_RIP_U16(a_u16NewIP)                  CHK_TYPE(uint16_t, a_u16NewIP)
#define IEM_MC_SET_RIP_U32(a_u32NewIP)                  CHK_TYPE(uint32_t, a_u32NewIP)
#define IEM_MC_SET_RIP_U64(a_u64NewIP)                  CHK_TYPE(uint64_t, a_u64NewIP)
#define IEM_MC_RAISE_DIVIDE_ERROR()                     return VERR_TRPM_ACTIVE_TRAP
#define IEM_MC_MAYBE_RAISE_DEVICE_NOT_AVAILABLE()       do {} while (0)
#define IEM_MC_MAYBE_RAISE_WAIT_DEVICE_NOT_AVAILABLE()  do {} while (0)
#define IEM_MC_MAYBE_RAISE_FPU_XCPT()                   do {} while (0)
#define IEM_MC_MAYBE_RAISE_MMX_RELATED_XCPT()           do {} while (0)
#define IEM_MC_MAYBE_RAISE_MMX_RELATED_XCPT_CHECK_SSE_OR_MMXEXT() do {} while (0)
#define IEM_MC_MAYBE_RAISE_SSE_RELATED_XCPT()           do {} while (0)
#define IEM_MC_MAYBE_RAISE_SSE2_RELATED_XCPT()          do {} while (0)
#define IEM_MC_MAYBE_RAISE_SSE3_RELATED_XCPT()          do {} while (0)
#define IEM_MC_MAYBE_RAISE_SSE41_RELATED_XCPT()         do {} while (0)
#define IEM_MC_MAYBE_RAISE_AVX_RELATED_XCPT()           do {} while (0)
#define IEM_MC_MAYBE_RAISE_AVX2_RELATED_XCPT()          do {} while (0)
#define IEM_MC_RAISE_GP0_IF_CPL_NOT_ZERO()              do {} while (0)
#define IEM_MC_RAISE_GP0_IF_EFF_ADDR_UNALIGNED(a_EffAddr, a_cbAlign) \
    do { AssertCompile(RT_IS_POWER_OF_TWO(a_cbAlign)); CHK_TYPE(RTGCPTR,  a_EffAddr); } while (0)
#define IEM_MC_MAYBE_RAISE_FSGSBASE_XCPT()              do {} while (0)
#define IEM_MC_MAYBE_RAISE_NON_CANONICAL_ADDR_GP0(a_u64Addr)    do {} while (0)

#define IEM_MC_LOCAL(a_Type, a_Name) \
    a_Type a_Name; NOREF(a_Name)
#define IEM_MC_LOCAL_CONST(a_Type, a_Name, a_Value) \
    a_Type const a_Name = (a_Value); \
    NOREF(a_Name)
#define IEM_MC_REF_LOCAL(a_pRefArg, a_Local) \
    (a_pRefArg) = &(a_Local)

#define IEM_MC_ARG(a_Type, a_Name, a_iArg) \
    RT_CONCAT(iArgCheck_,a_iArg) = 1; NOREF(RT_CONCAT(iArgCheck_,a_iArg)); \
    int RT_CONCAT3(iArgCheck_,a_iArg,a_Name); NOREF(RT_CONCAT3(iArgCheck_,a_iArg,a_Name)); \
    AssertCompile((a_iArg) < cArgs); \
    a_Type a_Name; \
    NOREF(a_Name)
#define IEM_MC_ARG_CONST(a_Type, a_Name, a_Value, a_iArg) \
    RT_CONCAT(iArgCheck_, a_iArg) = 1; NOREF(RT_CONCAT(iArgCheck_,a_iArg)); \
    int RT_CONCAT3(iArgCheck_,a_iArg,a_Name); NOREF(RT_CONCAT3(iArgCheck_,a_iArg,a_Name)); \
    AssertCompile((a_iArg) < cArgs); \
    a_Type const a_Name = (a_Value); \
    NOREF(a_Name)
#define IEM_MC_ARG_XSTATE(a_Name, a_iArg) \
    IEM_MC_ARG_CONST(PX86XSAVEAREA, a_Name, NULL, a_iArg)

#define IEM_MC_ARG_LOCAL_REF(a_Type, a_Name, a_Local, a_iArg) \
    RT_CONCAT(iArgCheck_, a_iArg) = 1; NOREF(RT_CONCAT(iArgCheck_,a_iArg)); \
    int RT_CONCAT3(iArgCheck_,a_iArg,a_Name); NOREF(RT_CONCAT3(iArgCheck_,a_iArg,a_Name)); \
    AssertCompile((a_iArg) < cArgs); \
    a_Type const a_Name = &(a_Local); \
    NOREF(a_Name)
#define IEM_MC_ARG_LOCAL_EFLAGS(a_pName, a_Name, a_iArg) \
    RT_CONCAT(iArgCheck_, a_iArg) = 1; NOREF(RT_CONCAT(iArgCheck_,a_iArg)); \
    int RT_CONCAT3(iArgCheck_,a_iArg,a_pName); NOREF(RT_CONCAT3(iArgCheck_,a_iArg,a_pName)); \
    AssertCompile((a_iArg) < cArgs); \
    uint32_t a_Name; \
    uint32_t *a_pName = &a_Name; \
    NOREF(a_pName)

#define IEM_MC_COMMIT_EFLAGS(a_EFlags)                  CHK_TYPE(uint32_t, a_EFlags)
#define IEM_MC_ASSIGN(a_VarOrArg, a_CVariableOrConst)   (a_VarOrArg) = (0)
#define IEM_MC_ASSIGN_TO_SMALLER                        IEM_MC_ASSIGN

#define IEM_MC_FETCH_GREG_U8(a_u8Dst, a_iGReg)          do { (a_u8Dst)  = 0; CHK_TYPE(uint8_t,  a_u8Dst);  } while (0)
#define IEM_MC_FETCH_GREG_U8_ZX_U16(a_u16Dst, a_iGReg)  do { (a_u16Dst) = 0; CHK_TYPE(uint16_t, a_u16Dst); } while (0)
#define IEM_MC_FETCH_GREG_U8_ZX_U32(a_u32Dst, a_iGReg)  do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); } while (0)
#define IEM_MC_FETCH_GREG_U8_ZX_U64(a_u64Dst, a_iGReg)  do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); } while (0)
#define IEM_MC_FETCH_GREG_U8_SX_U16(a_u16Dst, a_iGReg)  do { (a_u16Dst) = 0; CHK_TYPE(uint16_t, a_u16Dst); } while (0)
#define IEM_MC_FETCH_GREG_U8_SX_U32(a_u32Dst, a_iGReg)  do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); } while (0)
#define IEM_MC_FETCH_GREG_U8_SX_U64(a_u64Dst, a_iGReg)  do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); } while (0)
#define IEM_MC_FETCH_GREG_U16(a_u16Dst, a_iGReg)        do { (a_u16Dst) = 0; CHK_TYPE(uint16_t, a_u16Dst); } while (0)
#define IEM_MC_FETCH_GREG_U16_ZX_U32(a_u32Dst, a_iGReg) do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); } while (0)
#define IEM_MC_FETCH_GREG_U16_ZX_U64(a_u64Dst, a_iGReg) do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); } while (0)
#define IEM_MC_FETCH_GREG_U16_SX_U32(a_u32Dst, a_iGReg) do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); } while (0)
#define IEM_MC_FETCH_GREG_U16_SX_U64(a_u64Dst, a_iGReg) do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); } while (0)
#define IEM_MC_FETCH_GREG_U32(a_u32Dst, a_iGReg)        do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); } while (0)
#define IEM_MC_FETCH_GREG_U32_ZX_U64(a_u64Dst, a_iGReg) do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); } while (0)
#define IEM_MC_FETCH_GREG_U32_SX_U64(a_u64Dst, a_iGReg) do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); } while (0)
#define IEM_MC_FETCH_GREG_U64(a_u64Dst, a_iGReg)        do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); } while (0)
#define IEM_MC_FETCH_GREG_U64_ZX_U64                    IEM_MC_FETCH_GREG_U64
#define IEM_MC_FETCH_SREG_U16(a_u16Dst, a_iSReg)        do { (a_u16Dst) = 0; CHK_TYPE(uint16_t, a_u16Dst); } while (0)
#define IEM_MC_FETCH_SREG_ZX_U32(a_u32Dst, a_iSReg)     do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); } while (0)
#define IEM_MC_FETCH_SREG_ZX_U64(a_u64Dst, a_iSReg)     do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); } while (0)
#define IEM_MC_FETCH_SREG_BASE_U64(a_u64Dst, a_iSReg)   do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); } while (0)
#define IEM_MC_FETCH_SREG_BASE_U32(a_u32Dst, a_iSReg)   do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); } while (0)
#define IEM_MC_FETCH_CR0_U16(a_u16Dst)                  do { (a_u16Dst) = 0; CHK_TYPE(uint16_t, a_u16Dst); } while (0)
#define IEM_MC_FETCH_CR0_U32(a_u32Dst)                  do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); } while (0)
#define IEM_MC_FETCH_CR0_U64(a_u64Dst)                  do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); } while (0)
#define IEM_MC_FETCH_LDTR_U16(a_u16Dst)                 do { (a_u16Dst) = 0; CHK_TYPE(uint16_t, a_u16Dst); } while (0)
#define IEM_MC_FETCH_LDTR_U32(a_u32Dst)                 do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); } while (0)
#define IEM_MC_FETCH_LDTR_U64(a_u64Dst)                 do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); } while (0)
#define IEM_MC_FETCH_TR_U16(a_u16Dst)                   do { (a_u16Dst) = 0; CHK_TYPE(uint16_t, a_u16Dst); } while (0)
#define IEM_MC_FETCH_TR_U32(a_u32Dst)                   do { (a_u32Dst) = 0; CHK_TYPE(uint32_t, a_u32Dst); } while (0)
#define IEM_MC_FETCH_TR_U64(a_u64Dst)                   do { (a_u64Dst) = 0; CHK_TYPE(uint64_t, a_u64Dst); } while (0)
#define IEM_MC_FETCH_EFLAGS(a_EFlags)                   do { (a_EFlags) = 0; CHK_TYPE(uint32_t, a_EFlags); } while (0)
#define IEM_MC_FETCH_EFLAGS_U8(a_EFlags)                do { (a_EFlags) = 0; CHK_TYPE(uint8_t,  a_EFlags); } while (0)
#define IEM_MC_FETCH_FSW(a_u16Fsw)                      do { (a_u16Fsw) = 0; CHK_TYPE(uint16_t, a_u16Fsw); (void)fFpuRead; } while (0)
#define IEM_MC_FETCH_FCW(a_u16Fcw)                      do { (a_u16Fcw) = 0; CHK_TYPE(uint16_t, a_u16Fcw); (void)fFpuRead; } while (0)
#define IEM_MC_STORE_GREG_U8(a_iGReg, a_u8Value)        do { CHK_TYPE(uint8_t, a_u8Value); } while (0)
#define IEM_MC_STORE_GREG_U16(a_iGReg, a_u16Value)      do { CHK_TYPE(uint16_t, a_u16Value); } while (0)
#define IEM_MC_STORE_GREG_U32(a_iGReg, a_u32Value)      do {  } while (0)
#define IEM_MC_STORE_GREG_U64(a_iGReg, a_u64Value)      do {  } while (0)
#define IEM_MC_STORE_GREG_U8_CONST(a_iGReg, a_u8C)      do { AssertCompile((uint8_t )(a_u8C)  == (a_u8C) ); } while (0)
#define IEM_MC_STORE_GREG_U16_CONST(a_iGReg, a_u16C)    do { AssertCompile((uint16_t)(a_u16C) == (a_u16C)); } while (0)
#define IEM_MC_STORE_GREG_U32_CONST(a_iGReg, a_u32C)    do { AssertCompile((uint32_t)(a_u32C) == (a_u32C)); } while (0)
#define IEM_MC_STORE_GREG_U64_CONST(a_iGReg, a_u64C)    do { AssertCompile((uint64_t)(a_u64C) == (a_u64C)); } while (0)
#define IEM_MC_STORE_FPUREG_R80_SRC_REF(a_iSt, a_pr80Src) do { CHK_PTYPE(PCRTFLOAT80U, a_pr80Src); Assert((a_iSt) < 8); } while (0)
#define IEM_MC_CLEAR_HIGH_GREG_U64(a_iGReg)             do {  } while (0)
#define IEM_MC_CLEAR_HIGH_GREG_U64_BY_REF(a_pu32Dst)    do { CHK_PTYPE(uint32_t *, a_pu32Dst); } while (0)
#define IEM_MC_STORE_SREG_BASE_U64(a_iSeg, a_u64Value)  do {  } while (0)
#define IEM_MC_STORE_SREG_BASE_U32(a_iSeg, a_u32Value)  do {  } while (0)
#define IEM_MC_REF_GREG_U8(a_pu8Dst, a_iGReg)           do { (a_pu8Dst)  = (uint8_t  *)((uintptr_t)0); CHK_PTYPE(uint8_t  *, a_pu8Dst);  } while (0)
#define IEM_MC_REF_GREG_U16(a_pu16Dst, a_iGReg)         do { (a_pu16Dst) = (uint16_t *)((uintptr_t)0); CHK_PTYPE(uint16_t *, a_pu16Dst); } while (0)
#define IEM_MC_REF_GREG_U32(a_pu32Dst, a_iGReg)         do { (a_pu32Dst) = (uint32_t *)((uintptr_t)0); CHK_PTYPE(uint32_t *, a_pu32Dst); } while (0)
#define IEM_MC_REF_GREG_U64(a_pu64Dst, a_iGReg)         do { (a_pu64Dst) = (uint64_t *)((uintptr_t)0); CHK_PTYPE(uint64_t *, a_pu64Dst); } while (0)
#define IEM_MC_REF_EFLAGS(a_pEFlags)                    do { (a_pEFlags) = (uint32_t *)((uintptr_t)0); CHK_PTYPE(uint32_t *, a_pEFlags); } while (0)

#define IEM_MC_ADD_GREG_U8(a_iGReg, a_u8Value)          do { CHK_CONST(uint8_t,  a_u8Value);  } while (0)
#define IEM_MC_ADD_GREG_U16(a_iGReg, a_u16Value)        do { CHK_CONST(uint16_t, a_u16Value); } while (0)
#define IEM_MC_ADD_GREG_U32(a_iGReg, a_u32Value)        do { CHK_CONST(uint32_t, a_u32Value); } while (0)
#define IEM_MC_ADD_GREG_U64(a_iGReg, a_u64Value)        do { CHK_CONST(uint64_t, a_u64Value); } while (0)
#define IEM_MC_SUB_GREG_U8(a_iGReg,  a_u8Value)         do { CHK_CONST(uint8_t,  a_u8Value);  } while (0)
#define IEM_MC_SUB_GREG_U16(a_iGReg, a_u16Value)        do { CHK_CONST(uint16_t, a_u16Value); } while (0)
#define IEM_MC_SUB_GREG_U32(a_iGReg, a_u32Value)        do { CHK_CONST(uint32_t, a_u32Value); } while (0)
#define IEM_MC_SUB_GREG_U64(a_iGReg, a_u64Value)        do { CHK_CONST(uint64_t, a_u64Value); } while (0)
#define IEM_MC_SUB_LOCAL_U16(a_u16Value, a_u16Const)    do { CHK_CONST(uint16_t, a_u16Const); } while (0)

#define IEM_MC_AND_GREG_U8(a_iGReg, a_u8Value)          do { CHK_CONST(uint8_t,  a_u8Value);  } while (0)
#define IEM_MC_AND_GREG_U16(a_iGReg, a_u16Value)        do { CHK_CONST(uint16_t, a_u16Value); } while (0)
#define IEM_MC_AND_GREG_U32(a_iGReg, a_u32Value)        do { CHK_CONST(uint32_t, a_u32Value); } while (0)
#define IEM_MC_AND_GREG_U64(a_iGReg, a_u64Value)        do { CHK_CONST(uint64_t, a_u64Value); } while (0)
#define IEM_MC_OR_GREG_U8(a_iGReg,  a_u8Value)          do { CHK_CONST(uint8_t,  a_u8Value);  } while (0)
#define IEM_MC_OR_GREG_U16(a_iGReg, a_u16Value)         do { CHK_CONST(uint16_t, a_u16Value); } while (0)
#define IEM_MC_OR_GREG_U32(a_iGReg, a_u32Value)         do { CHK_CONST(uint32_t, a_u32Value); } while (0)
#define IEM_MC_OR_GREG_U64(a_iGReg, a_u64Value)         do { CHK_CONST(uint64_t, a_u64Value); } while (0)

#define IEM_MC_ADD_GREG_U8_TO_LOCAL(a_u16Value, a_iGReg)   do { (a_u8Value)  += 1; CHK_TYPE(uint8_t,  a_u8Value);  } while (0)
#define IEM_MC_ADD_GREG_U16_TO_LOCAL(a_u16Value, a_iGReg)  do { (a_u16Value) += 1; CHK_TYPE(uint16_t, a_u16Value); } while (0)
#define IEM_MC_ADD_GREG_U32_TO_LOCAL(a_u32Value, a_iGReg)  do { (a_u32Value) += 1; CHK_TYPE(uint32_t, a_u32Value); } while (0)
#define IEM_MC_ADD_GREG_U64_TO_LOCAL(a_u64Value, a_iGReg)  do { (a_u64Value) += 1; CHK_TYPE(uint64_t, a_u64Value); } while (0)
#define IEM_MC_ADD_LOCAL_S16_TO_EFF_ADDR(a_EffAddr, a_i16) do { (a_EffAddr) += (a_i16); CHK_GCPTR(a_EffAddr); } while (0)
#define IEM_MC_ADD_LOCAL_S32_TO_EFF_ADDR(a_EffAddr, a_i32) do { (a_EffAddr) += (a_i32); CHK_GCPTR(a_EffAddr); } while (0)
#define IEM_MC_ADD_LOCAL_S64_TO_EFF_ADDR(a_EffAddr, a_i64) do { (a_EffAddr) += (a_i64); CHK_GCPTR(a_EffAddr); } while (0)
#define IEM_MC_AND_LOCAL_U8(a_u8Local, a_u8Mask)        do { (a_u8Local)  &= (a_u8Mask);  CHK_TYPE(uint8_t,  a_u8Local);  CHK_CONST(uint8_t,  a_u8Mask);  } while (0)
#define IEM_MC_AND_LOCAL_U16(a_u16Local, a_u16Mask)     do { (a_u16Local) &= (a_u16Mask); CHK_TYPE(uint16_t, a_u16Local); CHK_CONST(uint16_t, a_u16Mask); } while (0)
#define IEM_MC_AND_LOCAL_U32(a_u32Local, a_u32Mask)     do { (a_u32Local) &= (a_u32Mask); CHK_TYPE(uint32_t, a_u32Local); CHK_CONST(uint32_t, a_u32Mask); } while (0)
#define IEM_MC_AND_LOCAL_U64(a_u64Local, a_u64Mask)     do { (a_u64Local) &= (a_u64Mask); CHK_TYPE(uint64_t, a_u64Local); CHK_CONST(uint64_t, a_u64Mask); } while (0)
#define IEM_MC_AND_ARG_U16(a_u16Arg, a_u16Mask)         do { (a_u16Arg)   &= (a_u16Mask); CHK_TYPE(uint16_t, a_u16Arg);   CHK_CONST(uint16_t, a_u16Mask); } while (0)
#define IEM_MC_AND_ARG_U32(a_u32Arg, a_u32Mask)         do { (a_u32Arg)   &= (a_u32Mask); CHK_TYPE(uint32_t, a_u32Arg);   CHK_CONST(uint32_t, a_u32Mask); } while (0)
#define IEM_MC_AND_ARG_U64(a_u64Arg, a_u64Mask)         do { (a_u64Arg)   &= (a_u64Mask); CHK_TYPE(uint64_t, a_u64Arg);   CHK_CONST(uint64_t, a_u64Mask); } while (0)
#define IEM_MC_OR_LOCAL_U8(a_u8Local, a_u8Mask)         do { (a_u8Local)  |= (a_u8Mask);  CHK_TYPE(uint8_t,  a_u8Local);  CHK_CONST(uint8_t,  a_u8Mask);  } while (0)
#define IEM_MC_OR_LOCAL_U16(a_u16Local, a_u16Mask)      do { (a_u16Local) |= (a_u16Mask); CHK_TYPE(uint16_t, a_u16Local); CHK_CONST(uint16_t, a_u16Mask); } while (0)
#define IEM_MC_OR_LOCAL_U32(a_u32Local, a_u32Mask)      do { (a_u32Local) |= (a_u32Mask); CHK_TYPE(uint32_t, a_u32Local); CHK_CONST(uint32_t, a_u32Mask); } while (0)
#define IEM_MC_SAR_LOCAL_S16(a_i16Local, a_cShift)      do { (a_i16Local) >>= (a_cShift); CHK_TYPE(int16_t, a_i16Local);  CHK_CONST(uint8_t,  a_cShift);  } while (0)
#define IEM_MC_SAR_LOCAL_S32(a_i32Local, a_cShift)      do { (a_i32Local) >>= (a_cShift); CHK_TYPE(int32_t, a_i32Local);  CHK_CONST(uint8_t,  a_cShift);  } while (0)
#define IEM_MC_SAR_LOCAL_S64(a_i64Local, a_cShift)      do { (a_i64Local) >>= (a_cShift); CHK_TYPE(int64_t, a_i64Local);  CHK_CONST(uint8_t,  a_cShift);  } while (0)
#define IEM_MC_SHL_LOCAL_S16(a_i16Local, a_cShift)      do { (a_i16Local) <<= (a_cShift); CHK_TYPE(int16_t, a_i16Local);  CHK_CONST(uint8_t,  a_cShift);  } while (0)
#define IEM_MC_SHL_LOCAL_S32(a_i32Local, a_cShift)      do { (a_i32Local) <<= (a_cShift); CHK_TYPE(int32_t, a_i32Local);  CHK_CONST(uint8_t,  a_cShift);  } while (0)
#define IEM_MC_SHL_LOCAL_S64(a_i64Local, a_cShift)      do { (a_i64Local) <<= (a_cShift); CHK_TYPE(int64_t, a_i64Local);  CHK_CONST(uint8_t,  a_cShift);  } while (0)
#define IEM_MC_AND_2LOCS_U32(a_u32Local, a_u32Mask)     do { (a_u32Local) &= (a_u32Mask); CHK_TYPE(uint32_t, a_u32Local); } while (0)
#define IEM_MC_OR_2LOCS_U32(a_u32Local, a_u32Mask)      do { (a_u32Local) |= (a_u32Mask); CHK_TYPE(uint32_t, a_u32Local); } while (0)
#define IEM_MC_SET_EFL_BIT(a_fBit)                      do { CHK_SINGLE_BIT(uint32_t, a_fBit); } while (0)
#define IEM_MC_CLEAR_EFL_BIT(a_fBit)                    do { CHK_SINGLE_BIT(uint32_t, a_fBit); } while (0)
#define IEM_MC_FLIP_EFL_BIT(a_fBit)                     do { CHK_SINGLE_BIT(uint32_t, a_fBit); } while (0)
#define IEM_MC_CLEAR_FSW_EX()                           do { } while (0)
#define IEM_MC_FPU_TO_MMX_MODE()                        do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_FROM_MMX_MODE()                      do { } while (0)

#define IEM_MC_FETCH_MREG_U64(a_u64Value, a_iMReg)          do { (a_u64Value) = 0; CHK_TYPE(uint64_t, a_u64Value); (void)fFpuRead; } while (0)
#define IEM_MC_FETCH_MREG_U32(a_u32Value, a_iMReg)          do { (a_u32Value) = 0; CHK_TYPE(uint32_t, a_u32Value); (void)fFpuRead; } while (0)
#define IEM_MC_STORE_MREG_U64(a_iMReg, a_u64Value)          do { CHK_TYPE(uint64_t, a_u64Value); (void)fFpuWrite; } while (0)
#define IEM_MC_STORE_MREG_U32_ZX_U64(a_iMReg, a_u32Value)   do { CHK_TYPE(uint32_t, a_u32Value); (void)fFpuWrite; } while (0)
#define IEM_MC_REF_MREG_U64(a_pu64Dst, a_iMReg)             do { (a_pu64Dst) = (uint64_t *)((uintptr_t)0); CHK_PTYPE(uint64_t *, a_pu64Dst);             (void)fFpuWrite; } while (0)
#define IEM_MC_REF_MREG_U64_CONST(a_pu64Dst, a_iMReg)       do { (a_pu64Dst) = (uint64_t const *)((uintptr_t)0); CHK_PTYPE(uint64_t const *, a_pu64Dst); (void)fFpuWrite; } while (0)
#define IEM_MC_REF_MREG_U32_CONST(a_pu32Dst, a_iMReg)       do { (a_pu32Dst) = (uint32_t const *)((uintptr_t)0); CHK_PTYPE(uint32_t const *, a_pu32Dst); (void)fFpuWrite; } while (0)

#define IEM_MC_FETCH_XREG_U128(a_u128Value, a_iXReg)        do { (a_u128Value) = g_u128Zero; CHK_TYPE(RTUINT128U, a_u128Value); (void)fSseRead;  } while (0)
#define IEM_MC_FETCH_XREG_U64(a_u64Value, a_iXReg)          do { (a_u64Value) = 0; CHK_TYPE(uint64_t, a_u64Value); (void)fSseRead; } while (0)
#define IEM_MC_FETCH_XREG_U32(a_u32Value, a_iXReg)          do { (a_u32Value) = 0; CHK_TYPE(uint32_t, a_u32Value); (void)fSseRead; } while (0)
#define IEM_MC_FETCH_XREG_HI_U64(a_u64Value, a_iXReg)       do { (a_u64Value) = 0; CHK_TYPE(uint64_t, a_u64Value); (void)fSseRead; } while (0)
#define IEM_MC_STORE_XREG_U128(a_iXReg, a_u128Value)        do { CHK_TYPE(RTUINT128U, a_u128Value); (void)fSseWrite; } while (0)
#define IEM_MC_STORE_XREG_U64(a_iXReg, a_u64Value)          do { CHK_TYPE(uint64_t,  a_u64Value);  (void)fSseWrite; } while (0)
#define IEM_MC_STORE_XREG_U64_ZX_U128(a_iXReg, a_u64Value)  do { CHK_TYPE(uint64_t,  a_u64Value);  (void)fSseWrite; } while (0)
#define IEM_MC_STORE_XREG_U32(a_iXReg, a_u32Value)          do { CHK_TYPE(uint32_t,  a_u32Value);  (void)fSseWrite; } while (0)
#define IEM_MC_STORE_XREG_U32_ZX_U128(a_iXReg, a_u32Value)  do { CHK_TYPE(uint32_t,  a_u32Value);  (void)fSseWrite; } while (0)
#define IEM_MC_STORE_XREG_HI_U64(a_iXReg, a_u64Value)       do { CHK_TYPE(uint64_t,  a_u64Value);  (void)fSseWrite; } while (0)
#define IEM_MC_REF_XREG_U128(a_pu128Dst, a_iXReg)           do { (a_pu128Dst) = (PRTUINT128U)((uintptr_t)0);        CHK_PTYPE(PRTUINT128U, a_pu128Dst);       (void)fSseWrite; } while (0)
#define IEM_MC_REF_XREG_U128_CONST(a_pu128Dst, a_iXReg)     do { (a_pu128Dst) = (PCRTUINT128U)((uintptr_t)0);  CHK_PTYPE(PCRTUINT128U, a_pu128Dst); (void)fSseWrite; } while (0)
#define IEM_MC_REF_XREG_U64_CONST(a_pu64Dst, a_iXReg)       do { (a_pu64Dst)  = (uint64_t const *)((uintptr_t)0);   CHK_PTYPE(uint64_t const *, a_pu64Dst);   (void)fSseWrite; } while (0)
#define IEM_MC_COPY_XREG_U128(a_iXRegDst, a_iXRegSrc)       do { (void)fSseWrite; } while (0)

#define IEM_MC_FETCH_YREG_U256(a_u256Value, a_iYRegSrc)           do { (a_u256Value).au64[0] = (a_u256Value).au64[1] = (a_u256Value).au64[2] = (a_u256Value).au64[3] = 0; CHK_TYPE(RTUINT256U, a_u256Value); (void)fAvxRead; } while (0)
#define IEM_MC_FETCH_YREG_U128(a_u128Value, a_iYRegSrc)           do { (a_u128Value).au64[0] = (a_u128Value).au64[1] = 0; CHK_TYPE(RTUINT128U, a_u128Value); (void)fAvxRead; } while (0)
#define IEM_MC_FETCH_YREG_U64(a_u64Value, a_iYRegSrc)             do { (a_u64Value) = UINT64_MAX; CHK_TYPE(uint64_t, a_u64Value); (void)fAvxRead; } while (0)
#define IEM_MC_FETCH_YREG_U32(a_u32Value, a_iYRegSrc)             do { (a_u32Value) = UINT32_MAX; CHK_TYPE(uint32_t, a_u32Value); (void)fAvxRead; } while (0)
#define IEM_MC_STORE_YREG_U32_ZX_VLMAX(a_iYRegDst, a_u32Value)    do { CHK_TYPE(uint32_t, a_u32Value); (void)fAvxWrite; } while (0)
#define IEM_MC_STORE_YREG_U64_ZX_VLMAX(a_iYRegDst, a_u64Value)    do { CHK_TYPE(uint64_t, a_u64Value); (void)fAvxWrite; } while (0)
#define IEM_MC_STORE_YREG_U128_ZX_VLMAX(a_iYRegDst, a_u128Value)  do { CHK_TYPE(RTUINT128U, a_u128Value); (void)fAvxWrite; } while (0)
#define IEM_MC_STORE_YREG_U256_ZX_VLMAX(a_iYRegDst, a_u256Value)  do { CHK_TYPE(RTUINT256U, a_u256Value); (void)fAvxWrite; } while (0)
#define IEM_MC_REF_YREG_U128(a_pu128Dst, a_iYReg)                 do { (a_pu128Dst) = (PRTUINT128U)((uintptr_t)0);      CHK_PTYPE(PRTUINT128U, a_pu128Dst);       (void)fAvxWrite; } while (0)
#define IEM_MC_REF_YREG_U128_CONST(a_pu128Dst, a_iYReg)           do { (a_pu128Dst) = (PCRTUINT128U)((uintptr_t)0);     CHK_PTYPE(PCRTUINT128U, a_pu128Dst);      (void)fAvxWrite; } while (0)
#define IEM_MC_REF_YREG_U64_CONST(a_pu64Dst, a_iYReg)             do { (a_pu64Dst)  = (uint64_t const *)((uintptr_t)0); CHK_PTYPE(uint64_t const *, a_pu64Dst);   (void)fAvxWrite; } while (0)
#define IEM_MC_CLEAR_YREG_128_UP(a_iYReg)                         do { (void)fAvxWrite; } while (0)
#define IEM_MC_COPY_YREG_U256_ZX_VLMAX(a_iYRegDst, a_iYRegSrc)    do { (void)fAvxWrite; } while (0)
#define IEM_MC_COPY_YREG_U128_ZX_VLMAX(a_iYRegDst, a_iYRegSrc)    do { (void)fAvxWrite; } while (0)
#define IEM_MC_COPY_YREG_U64_ZX_VLMAX(a_iYRegDst, a_iYRegSrc)     do { (void)fAvxWrite; } while (0)
#define IEM_MC_MERGE_YREG_U32_U96_ZX_VLMAX(a_iYRegDst, a_iYRegSrc32, a_iYRegSrcHx)      do { (void)fAvxWrite; (void)fAvxRead; } while (0)
#define IEM_MC_MERGE_YREG_U64_U64_ZX_VLMAX(a_iYRegDst, a_iYRegSrc64, a_iYRegSrcHx)      do { (void)fAvxWrite; (void)fAvxRead; } while (0)
#define IEM_MC_MERGE_YREG_U64HI_U64_ZX_VLMAX(a_iYRegDst, a_iYRegSrc64, a_iYRegSrcHx)    do { (void)fAvxWrite; (void)fAvxRead; } while (0)
#define IEM_MC_MERGE_YREG_U64LOCAL_U64_ZX_VLMAX(a_iYRegDst, a_u64Local, a_iYRegSrcHx)   do { (void)fAvxWrite; (void)fAvxRead; } while (0)

#define IEM_MC_FETCH_MEM_U8(a_u8Dst, a_iSeg, a_GCPtrMem)                do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM16_U8(a_u8Dst, a_iSeg, a_GCPtrMem16)            do { CHK_TYPE(uint16_t, a_GCPtrMem16); } while (0)
#define IEM_MC_FETCH_MEM32_U8(a_u8Dst, a_iSeg, a_GCPtrMem32)            do { CHK_TYPE(uint32_t, a_GCPtrMem32); } while (0)
#define IEM_MC_FETCH_MEM_U16(a_u16Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_I16(a_i16Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(int16_t, a_i16Dst); } while (0)
#define IEM_MC_FETCH_MEM_U32(a_u32Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_I32(a_i32Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(int32_t, a_i32Dst); } while (0)
#define IEM_MC_FETCH_MEM_S32_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)       do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U64(a_u64Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U64_ALIGN_U128(a_u64Dst, a_iSeg, a_GCPtrMem)   do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_I64(a_i64Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(int64_t, a_i64Dst); } while (0)

#define IEM_MC_FETCH_MEM_U8_DISP(a_u8Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    do { CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint8_t, a_offDisp); CHK_TYPE(uint8_t, a_u8Dst); } while (0)
#define IEM_MC_FETCH_MEM_U16_DISP(a_u16Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    do { CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint8_t, a_offDisp); CHK_TYPE(uint16_t, a_u16Dst); } while (0)
#define IEM_MC_FETCH_MEM_U32_DISP(a_u32Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    do { CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint8_t, a_offDisp); CHK_TYPE(uint32_t, a_u32Dst); } while (0)
#define IEM_MC_FETCH_MEM_U64_DISP(a_u64Dst, a_iSeg, a_GCPtrMem, a_offDisp) \
    do { CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint8_t, a_offDisp); CHK_TYPE(uint64_t, a_u64Dst); } while (0)

#define IEM_MC_FETCH_MEM_U8_ZX_U16(a_u16Dst, a_iSeg, a_GCPtrMem)        do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U8_ZX_U32(a_u32Dst, a_iSeg, a_GCPtrMem)        do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U8_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)        do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U16_ZX_U32(a_u32Dst, a_iSeg, a_GCPtrMem)       do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U16_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)       do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U32_ZX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)       do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U8_SX_U16(a_u16Dst, a_iSeg, a_GCPtrMem)        do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U8_SX_U32(a_u32Dst, a_iSeg, a_GCPtrMem)        do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U8_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)        do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U16_SX_U32(a_u32Dst, a_iSeg, a_GCPtrMem)       do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U16_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)       do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_U32_SX_U64(a_u64Dst, a_iSeg, a_GCPtrMem)       do { CHK_GCPTR(a_GCPtrMem); } while (0)
#define IEM_MC_FETCH_MEM_R32(a_r32Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTFLOAT32U, a_r32Dst);} while (0)
#define IEM_MC_FETCH_MEM_R64(a_r64Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTFLOAT64U, a_r64Dst);} while (0)
#define IEM_MC_FETCH_MEM_R80(a_r80Dst, a_iSeg, a_GCPtrMem)              do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTFLOAT80U, a_r80Dst);} while (0)
#define IEM_MC_FETCH_MEM_U128(a_u128Dst, a_iSeg, a_GCPtrMem)            do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT128U, a_u128Dst);} while (0)
#define IEM_MC_FETCH_MEM_U128_ALIGN_SSE(a_u128Dst, a_iSeg, a_GCPtrMem)  do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT128U, a_u128Dst);} while (0)
#define IEM_MC_FETCH_MEM_U256(a_u256Dst, a_iSeg, a_GCPtrMem)            do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT256U, a_u256Dst);} while (0)
#define IEM_MC_FETCH_MEM_U256_ALIGN_AVX(a_u256Dst, a_iSeg, a_GCPtrMem)  do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT256U, a_u256Dst);} while (0)

#define IEM_MC_STORE_MEM_U8(a_iSeg, a_GCPtrMem, a_u8Value)              do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(uint8_t,  a_u8Value); CHK_SEG_IDX(a_iSeg); } while (0)
#define IEM_MC_STORE_MEM_U16(a_iSeg, a_GCPtrMem, a_u16Value)            do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(uint16_t, a_u16Value);      } while (0)
#define IEM_MC_STORE_MEM_U32(a_iSeg, a_GCPtrMem, a_u32Value)            do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(uint32_t, a_u32Value);      } while (0)
#define IEM_MC_STORE_MEM_U64(a_iSeg, a_GCPtrMem, a_u64Value)            do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(uint64_t, a_u64Value);      } while (0)
#define IEM_MC_STORE_MEM_U8_CONST(a_iSeg, a_GCPtrMem, a_u8C)            do { CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint8_t,  a_u8C);          } while (0)
#define IEM_MC_STORE_MEM_U16_CONST(a_iSeg, a_GCPtrMem, a_u16C)          do { CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint16_t, a_u16C);         } while (0)
#define IEM_MC_STORE_MEM_U32_CONST(a_iSeg, a_GCPtrMem, a_u32C)          do { CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint32_t, a_u32C);         } while (0)
#define IEM_MC_STORE_MEM_U64_CONST(a_iSeg, a_GCPtrMem, a_u64C)          do { CHK_GCPTR(a_GCPtrMem); CHK_CONST(uint64_t, a_u64C);         } while (0)
#define IEM_MC_STORE_MEM_I8_CONST_BY_REF( a_pi8Dst,  a_i8C)             do { CHK_TYPE(int8_t *,  a_pi8Dst);  CHK_CONST(int8_t,  a_i8C);  } while (0)
#define IEM_MC_STORE_MEM_I16_CONST_BY_REF(a_pi16Dst, a_i16C)            do { CHK_TYPE(int16_t *, a_pi16Dst); CHK_CONST(int16_t, a_i16C); } while (0)
#define IEM_MC_STORE_MEM_I32_CONST_BY_REF(a_pi32Dst, a_i32C)            do { CHK_TYPE(int32_t *, a_pi32Dst); CHK_CONST(int32_t, a_i32C); } while (0)
#define IEM_MC_STORE_MEM_I64_CONST_BY_REF(a_pi64Dst, a_i64C)            do { CHK_TYPE(int64_t *, a_pi64Dst); CHK_CONST(int64_t, a_i64C); } while (0)
#define IEM_MC_STORE_MEM_NEG_QNAN_R32_BY_REF(a_pr32Dst)                 do { CHK_TYPE(PRTFLOAT32U, a_pr32Dst); } while (0)
#define IEM_MC_STORE_MEM_NEG_QNAN_R64_BY_REF(a_pr64Dst)                 do { CHK_TYPE(PRTFLOAT64U, a_pr64Dst); } while (0)
#define IEM_MC_STORE_MEM_NEG_QNAN_R80_BY_REF(a_pr80Dst)                 do { CHK_TYPE(PRTFLOAT80U, a_pr80Dst); } while (0)
#define IEM_MC_STORE_MEM_U128(a_iSeg, a_GCPtrMem, a_u128Src)            do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT128U, a_u128Src); CHK_SEG_IDX(a_iSeg);} while (0)
#define IEM_MC_STORE_MEM_U128_ALIGN_SSE(a_iSeg, a_GCPtrMem, a_u128Src)  do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT128U, a_u128Src); CHK_SEG_IDX(a_iSeg);} while (0)
#define IEM_MC_STORE_MEM_U256(a_iSeg, a_GCPtrMem, a_u256Src)            do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT256U, a_u256Src); CHK_SEG_IDX(a_iSeg);} while (0)
#define IEM_MC_STORE_MEM_U256_ALIGN_AVX(a_iSeg, a_GCPtrMem, a_u256Src)  do { CHK_GCPTR(a_GCPtrMem); CHK_TYPE(RTUINT256U, a_u256Src); CHK_SEG_IDX(a_iSeg);} while (0)

#define IEM_MC_PUSH_U16(a_u16Value)                                     do {} while (0)
#define IEM_MC_PUSH_U32(a_u32Value)                                     do {} while (0)
#define IEM_MC_PUSH_U32_SREG(a_u32Value)                                do {} while (0)
#define IEM_MC_PUSH_U64(a_u64Value)                                     do {} while (0)
#define IEM_MC_POP_U16(a_pu16Value)                                     do {} while (0)
#define IEM_MC_POP_U32(a_pu32Value)                                     do {} while (0)
#define IEM_MC_POP_U64(a_pu64Value)                                     do {} while (0)
#define IEM_MC_MEM_MAP(a_pMem, a_fAccess, a_iSeg, a_GCPtrMem, a_iArg)   do {} while (0)
#define IEM_MC_MEM_MAP_EX(a_pvMem, a_fAccess, a_cbMem, a_iSeg, a_GCPtrMem, a_iArg)  do {} while (0)
#define IEM_MC_MEM_COMMIT_AND_UNMAP(a_pvMem, a_fAccess)                             do {} while (0)
#define IEM_MC_MEM_COMMIT_AND_UNMAP_FOR_FPU_STORE(a_pvMem, a_fAccess, a_u16FSW)     do {} while (0)
#define IEM_MC_CALC_RM_EFF_ADDR(a_GCPtrEff, bRm, cbImm)                 do { (a_GCPtrEff) = 0; CHK_GCPTR(a_GCPtrEff); } while (0)
#define IEM_MC_CALL_VOID_AIMPL_0(a_pfn)                                 do {} while (0)
#define IEM_MC_CALL_VOID_AIMPL_1(a_pfn, a0) \
    do { CHK_CALL_ARG(a0, 0); } while (0)
#define IEM_MC_CALL_VOID_AIMPL_2(a_pfn, a0, a1) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); } while (0)
#define IEM_MC_CALL_VOID_AIMPL_3(a_pfn, a0, a1, a2) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); } while (0)
#define IEM_MC_CALL_VOID_AIMPL_4(a_pfn, a0, a1, a2, a3) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); CHK_CALL_ARG(a3, 3); } while (0)
#define IEM_MC_CALL_AIMPL_3(a_rc, a_pfn, a0, a1, a2) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2);  (a_rc) = VINF_SUCCESS; } while (0)
#define IEM_MC_CALL_AIMPL_4(a_rc, a_pfn, a0, a1, a2, a3) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); CHK_CALL_ARG(a3, 3);  (a_rc) = VINF_SUCCESS; } while (0)
#define IEM_MC_CALL_CIMPL_0(a_pfnCImpl)                                 do { } while (0)
#define IEM_MC_CALL_CIMPL_1(a_pfnCImpl, a0) \
    do { CHK_CALL_ARG(a0, 0);  } while (0)
#define IEM_MC_CALL_CIMPL_2(a_pfnCImpl, a0, a1) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); } while (0)
#define IEM_MC_CALL_CIMPL_3(a_pfnCImpl, a0, a1, a2) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); } while (0)
#define IEM_MC_CALL_CIMPL_4(a_pfnCImpl, a0, a1, a2, a3) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); CHK_CALL_ARG(a3, 3); } while (0)
#define IEM_MC_CALL_CIMPL_5(a_pfnCImpl, a0, a1, a2, a3, a4) \
    do { CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); CHK_CALL_ARG(a3, 3); CHK_CALL_ARG(a4, 4);  } while (0)
#define IEM_MC_DEFER_TO_CIMPL_0(a_pfnCImpl)                             (VINF_SUCCESS)
#define IEM_MC_DEFER_TO_CIMPL_1(a_pfnCImpl, a0)                         (VINF_SUCCESS)
#define IEM_MC_DEFER_TO_CIMPL_2(a_pfnCImpl, a0, a1)                     (VINF_SUCCESS)
#define IEM_MC_DEFER_TO_CIMPL_3(a_pfnCImpl, a0, a1, a2)                 (VINF_SUCCESS)

#define IEM_MC_CALL_FPU_AIMPL_1(a_pfnAImpl, a0) \
    do { (void)fFpuHost; (void)fFpuWrite; CHK_CALL_ARG(a0, 0);  } while (0)
#define IEM_MC_CALL_FPU_AIMPL_2(a_pfnAImpl, a0, a1) \
    do { (void)fFpuHost; (void)fFpuWrite; CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); } while (0)
#define IEM_MC_CALL_FPU_AIMPL_3(a_pfnAImpl, a0, a1, a2) \
    do { (void)fFpuHost; (void)fFpuWrite; CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); } while (0)
#define IEM_MC_SET_FPU_RESULT(a_FpuData, a_FSW, a_pr80Value)            do { (void)fFpuWrite; } while (0)
#define IEM_MC_PUSH_FPU_RESULT(a_FpuData)                               do { (void)fFpuWrite; } while (0)
#define IEM_MC_PUSH_FPU_RESULT_MEM_OP(a_FpuData, a_iEffSeg, a_GCPtrEff) do { (void)fFpuWrite; } while (0)
#define IEM_MC_PUSH_FPU_RESULT_TWO(a_FpuDataTwo)                        do { (void)fFpuWrite; } while (0)
#define IEM_MC_STORE_FPU_RESULT(a_FpuData, a_iStReg)                    do { (void)fFpuWrite; } while (0)
#define IEM_MC_STORE_FPU_RESULT_THEN_POP(a_FpuData, a_iStReg)           do { (void)fFpuWrite; } while (0)
#define IEM_MC_STORE_FPU_RESULT_MEM_OP(a_FpuData, a_iStReg, a_iEffSeg, a_GCPtrEff)              do { (void)fFpuWrite; } while (0)
#define IEM_MC_STORE_FPU_RESULT_MEM_OP_THEN_POP(a_FpuData, a_iStReg, a_iEffSeg, a_GCPtrEff)     do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_STACK_UNDERFLOW(a_iStReg)                                                    do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP(a_iStReg, a_iEffSeg, a_GCPtrEff)                      do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP(a_iStReg)                                           do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_STACK_UNDERFLOW_MEM_OP_THEN_POP(a_iStReg, a_iEffSeg, a_GCPtrEff)             do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_STACK_UNDERFLOW_THEN_POP_POP()                                               do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_STACK_PUSH_UNDERFLOW()                                                       do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_STACK_PUSH_UNDERFLOW_TWO()                                                   do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_STACK_PUSH_OVERFLOW()                                                        do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_STACK_PUSH_OVERFLOW_MEM_OP(a_iEffSeg, a_GCPtrEff)                            do { (void)fFpuWrite; } while (0)
#define IEM_MC_UPDATE_FPU_OPCODE_IP()                                                           do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_STACK_DEC_TOP()                                                              do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_STACK_INC_TOP()                                                              do { (void)fFpuWrite; } while (0)
#define IEM_MC_FPU_STACK_FREE(a_iStReg)                                                         do { (void)fFpuWrite; } while (0)
#define IEM_MC_UPDATE_FSW(a_u16FSW)                                                             do { (void)fFpuWrite; } while (0)
#define IEM_MC_UPDATE_FSW_CONST(a_u16FSW)                                                       do { (void)fFpuWrite; } while (0)
#define IEM_MC_UPDATE_FSW_WITH_MEM_OP(a_u16FSW, a_iEffSeg, a_GCPtrEff)                          do { (void)fFpuWrite; } while (0)
#define IEM_MC_UPDATE_FSW_THEN_POP(a_u16FSW)                                                    do { (void)fFpuWrite; } while (0)
#define IEM_MC_UPDATE_FSW_WITH_MEM_OP_THEN_POP(a_u16FSW, a_iEffSeg, a_GCPtrEff)                 do { (void)fFpuWrite; } while (0)
#define IEM_MC_UPDATE_FSW_THEN_POP_POP(a_u16FSW)                                                do { (void)fFpuWrite; } while (0)
#define IEM_MC_PREPARE_FPU_USAGE() \
    const int fFpuRead = 1, fFpuWrite = 1, fFpuHost = 1, fSseRead = 1, fSseWrite = 1, fSseHost = 1, fAvxRead = 1, fAvxWrite = 1, fAvxHost = 1
#define IEM_MC_ACTUALIZE_FPU_STATE_FOR_READ()   const int fFpuRead = 1, fSseRead = 1
#define IEM_MC_ACTUALIZE_FPU_STATE_FOR_CHANGE() const int fFpuRead = 1, fFpuWrite = 1, fSseRead = 1, fSseWrite = 1
#define IEM_MC_PREPARE_SSE_USAGE()              const int fSseRead = 1, fSseWrite = 1, fSseHost = 1
#define IEM_MC_ACTUALIZE_SSE_STATE_FOR_READ()   const int fSseRead = 1
#define IEM_MC_ACTUALIZE_SSE_STATE_FOR_CHANGE() const int fSseRead = 1, fSseWrite = 1
#define IEM_MC_PREPARE_AVX_USAGE()              const int fAvxRead = 1, fAvxWrite = 1, fAvxHost = 1, fSseRead = 1, fSseWrite = 1, fSseHost = 1
#define IEM_MC_ACTUALIZE_AVX_STATE_FOR_READ()   const int fAvxRead = 1, fSseRead = 1
#define IEM_MC_ACTUALIZE_AVX_STATE_FOR_CHANGE() const int fAvxRead = 1, fAvxWrite = 1, fSseRead = 1, fSseWrite = 1

#define IEM_MC_CALL_MMX_AIMPL_2(a_pfnAImpl, a0, a1) \
    do { (void)fFpuHost; (void)fFpuWrite; CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); } while (0)
#define IEM_MC_CALL_MMX_AIMPL_3(a_pfnAImpl, a0, a1, a2) \
    do { (void)fFpuHost; (void)fFpuWrite; CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2);} while (0)
#define IEM_MC_CALL_SSE_AIMPL_2(a_pfnAImpl, a0, a1) \
    do { (void)fSseHost; (void)fSseWrite; CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); } while (0)
#define IEM_MC_CALL_SSE_AIMPL_3(a_pfnAImpl, a0, a1, a2) \
    do { (void)fSseHost; (void)fSseWrite; CHK_CALL_ARG(a0, 0); CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2);} while (0)
#define IEM_MC_IMPLICIT_AVX_AIMPL_ARGS() IEM_MC_ARG_CONST(PX86XSAVEAREA, pXState, (pVCpu)->iem.s.CTX_SUFF(pCtx)->CTX_SUFF(pXState), 0)
#define IEM_MC_CALL_AVX_AIMPL_2(a_pfnAImpl, a1, a2) \
    do { (void)fAvxHost; (void)fAvxWrite; CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); } while (0)
#define IEM_MC_CALL_AVX_AIMPL_3(a_pfnAImpl, a1, a2, a3) \
    do { (void)fAvxHost; (void)fAvxWrite; CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); CHK_CALL_ARG(a3, 3);} while (0)
#define IEM_MC_CALL_AVX_AIMPL_4(a_pfnAImpl, a1, a2, a3, a4) \
    do { (void)fAvxHost; (void)fAvxWrite; CHK_CALL_ARG(a1, 1); CHK_CALL_ARG(a2, 2); CHK_CALL_ARG(a3, 3); CHK_CALL_ARG(a4, 4);} while (0)

#define IEM_MC_IF_EFL_BIT_SET(a_fBit)                                   if (g_fRandom) {
#define IEM_MC_IF_EFL_BIT_NOT_SET(a_fBit)                               if (g_fRandom) {
#define IEM_MC_IF_EFL_ANY_BITS_SET(a_fBits)                             if (g_fRandom) {
#define IEM_MC_IF_EFL_NO_BITS_SET(a_fBits)                              if (g_fRandom) {
#define IEM_MC_IF_EFL_BITS_NE(a_fBit1, a_fBit2)                         if (g_fRandom) {
#define IEM_MC_IF_EFL_BITS_EQ(a_fBit1, a_fBit2)                         if (g_fRandom) {
#define IEM_MC_IF_EFL_BIT_SET_OR_BITS_NE(a_fBit, a_fBit1, a_fBit2)      if (g_fRandom) {
#define IEM_MC_IF_EFL_BIT_NOT_SET_AND_BITS_EQ(a_fBit, a_fBit1, a_fBit2) if (g_fRandom) {
#define IEM_MC_IF_CX_IS_NZ()                                            if (g_fRandom) {
#define IEM_MC_IF_ECX_IS_NZ()                                           if (g_fRandom) {
#define IEM_MC_IF_RCX_IS_NZ()                                           if (g_fRandom) {
#define IEM_MC_IF_CX_IS_NZ_AND_EFL_BIT_SET(a_fBit)                      if (g_fRandom) {
#define IEM_MC_IF_ECX_IS_NZ_AND_EFL_BIT_SET(a_fBit)                     if (g_fRandom) {
#define IEM_MC_IF_RCX_IS_NZ_AND_EFL_BIT_SET(a_fBit)                     if (g_fRandom) {
#define IEM_MC_IF_CX_IS_NZ_AND_EFL_BIT_NOT_SET(a_fBit)                  if (g_fRandom) {
#define IEM_MC_IF_ECX_IS_NZ_AND_EFL_BIT_NOT_SET(a_fBit)                 if (g_fRandom) {
#define IEM_MC_IF_RCX_IS_NZ_AND_EFL_BIT_NOT_SET(a_fBit)                 if (g_fRandom) {
#define IEM_MC_IF_LOCAL_IS_Z(a_Local)                                   if ((a_Local) == 0) {
#define IEM_MC_IF_GREG_BIT_SET(a_iGReg, a_iBitNo)                       if (g_fRandom) {
#define IEM_MC_IF_FPUREG_NOT_EMPTY(a_iSt)                               if (g_fRandom != fFpuRead) {
#define IEM_MC_IF_FPUREG_IS_EMPTY(a_iSt)                                if (g_fRandom != fFpuRead) {
#define IEM_MC_IF_FPUREG_NOT_EMPTY_REF_R80(a_pr80Dst, a_iSt) \
    a_pr80Dst = NULL; \
    if (g_fRandom != fFpuRead) {
#define IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80(p0, i0, p1, i1) \
    p0 = NULL; \
    p1 = NULL; \
    if (g_fRandom != fFpuRead) {
#define IEM_MC_IF_TWO_FPUREGS_NOT_EMPTY_REF_R80_FIRST(p0, i0, i1) \
    p0 = NULL; \
    if (g_fRandom != fFpuRead) {
#define IEM_MC_IF_FCW_IM()                                              if (g_fRandom != fFpuRead) {
#define IEM_MC_ELSE()                                                   } else {
#define IEM_MC_ENDIF()                                                  } do {} while (0)

/** @}  */

#include "../VMMAll/IEMAllInstructions.cpp.h"



/**
 * Formalities...
 */
int main()
{
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstIEMCheckMc", &hTest);
    if (rcExit == RTEXITCODE_SUCCESS)
    {
        RTTestBanner(hTest);
        RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "(this is only a compile test.)");
        rcExit = RTTestSummaryAndDestroy(hTest);
    }
    return rcExit;
}
