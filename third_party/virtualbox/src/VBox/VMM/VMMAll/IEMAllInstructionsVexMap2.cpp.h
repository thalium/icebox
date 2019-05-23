/* $Id: IEMAllInstructionsVexMap2.cpp.h $ */
/** @file
 * IEM - Instruction Decoding and Emulation.
 *
 * @remarks IEMAllInstructionsThree0f38.cpp.h is a VEX mirror of this file.
 *          Any update here is likely needed in that file too.
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


/** @name VEX Opcode Map 2
 * @{
 */

/*  Opcode VEX.0F38 0x00 - invalid. */
/** Opcode VEX.66.0F38 0x00. */
FNIEMOP_STUB(iemOp_vpshufb_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x01 - invalid. */
/** Opcode VEX.66.0F38 0x01. */
FNIEMOP_STUB(iemOp_vphaddw_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x02 - invalid. */
/** Opcode VEX.66.0F38 0x02. */
FNIEMOP_STUB(iemOp_vphaddd_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x03 - invalid. */
/** Opcode VEX.66.0F38 0x03. */
FNIEMOP_STUB(iemOp_vphaddsw_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x04 - invalid. */
/** Opcode VEX.66.0F38 0x04. */
FNIEMOP_STUB(iemOp_vpmaddubsw_Vx_Hx_Wx);
/* Opcode VEX.0F38 0x05 - invalid. */
/** Opcode VEX.66.0F38 0x05. */
FNIEMOP_STUB(iemOp_vphsubw_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x06 - invalid. */
/** Opcode VEX.66.0F38 0x06. */
FNIEMOP_STUB(iemOp_vphsubdq_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x07 - invalid. */
/** Opcode VEX.66.0F38 0x07. */
FNIEMOP_STUB(iemOp_vphsubsw_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x08 - invalid. */
/** Opcode VEX.66.0F38 0x08. */
FNIEMOP_STUB(iemOp_vpsignb_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x09 - invalid. */
/** Opcode VEX.66.0F38 0x09. */
FNIEMOP_STUB(iemOp_vpsignw_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x0a - invalid. */
/** Opcode VEX.66.0F38 0x0a. */
FNIEMOP_STUB(iemOp_vpsignd_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x0b - invalid. */
/** Opcode VEX.66.0F38 0x0b. */
FNIEMOP_STUB(iemOp_vpmulhrsw_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x0c - invalid. */
/**  Opcode VEX.66.0F38 0x0c. */
FNIEMOP_STUB(iemOp_vpermilps_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x0d - invalid. */
/**  Opcode VEX.66.0F38 0x0d. */
FNIEMOP_STUB(iemOp_vpermilpd_Vx_Hx_Wx);
/*  Opcode VEX.0F38 0x0e - invalid. */
/**  Opcode VEX.66.0F38 0x0e. */
FNIEMOP_STUB(iemOp_vtestps_Vx_Wx);
/*  Opcode VEX.0F38 0x0f - invalid. */
/**  Opcode VEX.66.0F38 0x0f. */
FNIEMOP_STUB(iemOp_vtestpd_Vx_Wx);


/*  Opcode VEX.0F38 0x10 - invalid */
/*  Opcode VEX.66.0F38 0x10 - invalid (legacy only). */
/*  Opcode VEX.0F38 0x11 - invalid */
/*  Opcode VEX.66.0F38 0x11 - invalid */
/*  Opcode VEX.0F38 0x12 - invalid */
/*  Opcode VEX.66.0F38 0x12 - invalid */
/*  Opcode VEX.0F38 0x13 - invalid */
/*  Opcode VEX.66.0F38 0x13 - invalid (vex only). */
/*  Opcode VEX.0F38 0x14 - invalid */
/*  Opcode VEX.66.0F38 0x14 - invalid (legacy only). */
/*  Opcode VEX.0F38 0x15 - invalid */
/*  Opcode VEX.66.0F38 0x15 - invalid (legacy only). */
/*  Opcode VEX.0F38 0x16 - invalid */
/** Opcode VEX.66.0F38 0x16. */
FNIEMOP_STUB(iemOp_vpermps_Vqq_Hqq_Wqq);
/*  Opcode VEX.0F38 0x17 - invalid */
/** Opcode VEX.66.0F38 0x17 - invalid */
FNIEMOP_STUB(iemOp_vptest_Vx_Wx);
/*  Opcode VEX.0F38 0x18 - invalid */
/** Opcode VEX.66.0F38 0x18. */
FNIEMOP_STUB(iemOp_vbroadcastss_Vx_Wd);
/*  Opcode VEX.0F38 0x19 - invalid */
/** Opcode VEX.66.0F38 0x19. */
FNIEMOP_STUB(iemOp_vbroadcastsd_Vqq_Wq);
/*  Opcode VEX.0F38 0x1a - invalid */
/** Opcode VEX.66.0F38 0x1a. */
FNIEMOP_STUB(iemOp_vbroadcastf128_Vqq_Mdq);
/*  Opcode VEX.0F38 0x1b - invalid */
/*  Opcode VEX.66.0F38 0x1b - invalid */
/*  Opcode VEX.0F38 0x1c - invalid. */
/** Opcode VEX.66.0F38 0x1c. */
FNIEMOP_STUB(iemOp_vpabsb_Vx_Wx);
/*  Opcode VEX.0F38 0x1d - invalid. */
/** Opcode VEX.66.0F38 0x1d. */
FNIEMOP_STUB(iemOp_vpabsw_Vx_Wx);
/*  Opcode VEX.0F38 0x1e - invalid. */
/** Opcode VEX.66.0F38 0x1e. */
FNIEMOP_STUB(iemOp_vpabsd_Vx_Wx);
/*  Opcode VEX.0F38 0x1f - invalid */
/*  Opcode VEX.66.0F38 0x1f - invalid */


/** Opcode VEX.66.0F38 0x20. */
FNIEMOP_STUB(iemOp_vpmovsxbw_Vx_UxMq);
/** Opcode VEX.66.0F38 0x21. */
FNIEMOP_STUB(iemOp_vpmovsxbd_Vx_UxMd);
/** Opcode VEX.66.0F38 0x22. */
FNIEMOP_STUB(iemOp_vpmovsxbq_Vx_UxMw);
/** Opcode VEX.66.0F38 0x23. */
FNIEMOP_STUB(iemOp_vpmovsxwd_Vx_UxMq);
/** Opcode VEX.66.0F38 0x24. */
FNIEMOP_STUB(iemOp_vpmovsxwq_Vx_UxMd);
/** Opcode VEX.66.0F38 0x25. */
FNIEMOP_STUB(iemOp_vpmovsxdq_Vx_UxMq);
/*  Opcode VEX.66.0F38 0x26 - invalid */
/*  Opcode VEX.66.0F38 0x27 - invalid */
/** Opcode VEX.66.0F38 0x28. */
FNIEMOP_STUB(iemOp_vpmuldq_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x29. */
FNIEMOP_STUB(iemOp_vpcmpeqq_Vx_Hx_Wx);


FNIEMOP_DEF(iemOp_vmovntdqa_Vx_Mx)
{
    Assert(pVCpu->iem.s.uVexLength <= 1);
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
    {
        if (pVCpu->iem.s.uVexLength == 0)
        {
            /**
             * @opcode      0x2a
             * @opcodesub   !11 mr/reg vex.l=0
             * @oppfx       0x66
             * @opcpuid     avx
             * @opgroup     og_avx_cachect
             * @opxcpttype  1
             * @optest      op1=-1 op2=2  -> op1=2
             * @optest      op1=0 op2=-42 -> op1=-42
             */
            /* 128-bit: Memory, register. */
            IEMOP_MNEMONIC2EX(vmovntdqa_Vdq_WO_Mdq_L0, "vmovntdqa, Vdq_WO, Mdq", VEX_RM_MEM, VMOVNTDQA, vmovntdqa, Vx_WO, Mx,
                              DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(RTUINT128U,                uSrc);
            IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);

            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
            IEMOP_HLP_DONE_VEX_DECODING_NO_VVVV();
            IEM_MC_MAYBE_RAISE_AVX_RELATED_XCPT();
            IEM_MC_ACTUALIZE_AVX_STATE_FOR_CHANGE();

            IEM_MC_FETCH_MEM_U128_ALIGN_SSE(uSrc, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
            IEM_MC_STORE_YREG_U128_ZX_VLMAX(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg, uSrc);

            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
        }
        else
        {
            /**
             * @opdone
             * @opcode      0x2a
             * @opcodesub   !11 mr/reg vex.l=1
             * @oppfx       0x66
             * @opcpuid     avx2
             * @opgroup     og_avx2_cachect
             * @opxcpttype  1
             * @optest      op1=-1 op2=2  -> op1=2
             * @optest      op1=0 op2=-42 -> op1=-42
             */
            /* 256-bit: Memory, register. */
            IEMOP_MNEMONIC2EX(vmovntdqa_Vqq_WO_Mqq_L1, "vmovntdqa, Vqq_WO,Mqq", VEX_RM_MEM, VMOVNTDQA, vmovntdqa, Vx_WO, Mx,
                              DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
            IEM_MC_BEGIN(0, 2);
            IEM_MC_LOCAL(RTUINT256U,                uSrc);
            IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);

            IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
            IEMOP_HLP_DONE_VEX_DECODING_NO_VVVV();
            IEM_MC_MAYBE_RAISE_AVX2_RELATED_XCPT();
            IEM_MC_ACTUALIZE_AVX_STATE_FOR_CHANGE();

            IEM_MC_FETCH_MEM_U256_ALIGN_AVX(uSrc, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
            IEM_MC_STORE_YREG_U256_ZX_VLMAX(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg, uSrc);

            IEM_MC_ADVANCE_RIP();
            IEM_MC_END();
        }
        return VINF_SUCCESS;
    }

    /**
     * @opdone
     * @opmnemonic  udvex660f382arg
     * @opcode      0x2a
     * @opcodesub   11 mr/reg
     * @oppfx       0x66
     * @opunused    immediate
     * @opcpuid     avx
     * @optest      ->
     */
    return IEMOP_RAISE_INVALID_OPCODE();

}


/** Opcode VEX.66.0F38 0x2b. */
FNIEMOP_STUB(iemOp_vpackusdw_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x2c. */
FNIEMOP_STUB(iemOp_vmaskmovps_Vx_Hx_Mx);
/** Opcode VEX.66.0F38 0x2d. */
FNIEMOP_STUB(iemOp_vmaskmovpd_Vx_Hx_Mx);
/** Opcode VEX.66.0F38 0x2e. */
FNIEMOP_STUB(iemOp_vmaskmovps_Mx_Hx_Vx);
/** Opcode VEX.66.0F38 0x2f. */
FNIEMOP_STUB(iemOp_vmaskmovpd_Mx_Hx_Vx);

/** Opcode VEX.66.0F38 0x30. */
FNIEMOP_STUB(iemOp_vpmovzxbw_Vx_UxMq);
/** Opcode VEX.66.0F38 0x31. */
FNIEMOP_STUB(iemOp_vpmovzxbd_Vx_UxMd);
/** Opcode VEX.66.0F38 0x32. */
FNIEMOP_STUB(iemOp_vpmovzxbq_Vx_UxMw);
/** Opcode VEX.66.0F38 0x33. */
FNIEMOP_STUB(iemOp_vpmovzxwd_Vx_UxMq);
/** Opcode VEX.66.0F38 0x34. */
FNIEMOP_STUB(iemOp_vpmovzxwq_Vx_UxMd);
/** Opcode VEX.66.0F38 0x35. */
FNIEMOP_STUB(iemOp_vpmovzxdq_Vx_UxMq);
/*  Opcode VEX.66.0F38 0x36. */
FNIEMOP_STUB(iemOp_vpermd_Vqq_Hqq_Wqq);
/** Opcode VEX.66.0F38 0x37. */
FNIEMOP_STUB(iemOp_vpcmpgtq_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x38. */
FNIEMOP_STUB(iemOp_vpminsb_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x39. */
FNIEMOP_STUB(iemOp_vpminsd_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x3a. */
FNIEMOP_STUB(iemOp_vpminuw_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x3b. */
FNIEMOP_STUB(iemOp_vpminud_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x3c. */
FNIEMOP_STUB(iemOp_vpmaxsb_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x3d. */
FNIEMOP_STUB(iemOp_vpmaxsd_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x3e. */
FNIEMOP_STUB(iemOp_vpmaxuw_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x3f. */
FNIEMOP_STUB(iemOp_vpmaxud_Vx_Hx_Wx);


/** Opcode VEX.66.0F38 0x40. */
FNIEMOP_STUB(iemOp_vpmulld_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x41. */
FNIEMOP_STUB(iemOp_vphminposuw_Vdq_Wdq);
/*  Opcode VEX.66.0F38 0x42 - invalid. */
/*  Opcode VEX.66.0F38 0x43 - invalid. */
/*  Opcode VEX.66.0F38 0x44 - invalid. */
/** Opcode VEX.66.0F38 0x45. */
FNIEMOP_STUB(iemOp_vpsrlvd_q_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x46. */
FNIEMOP_STUB(iemOp_vsravd_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x47. */
FNIEMOP_STUB(iemOp_vpsllvd_q_Vx_Hx_Wx);
/*  Opcode VEX.66.0F38 0x48 - invalid. */
/*  Opcode VEX.66.0F38 0x49 - invalid. */
/*  Opcode VEX.66.0F38 0x4a - invalid. */
/*  Opcode VEX.66.0F38 0x4b - invalid. */
/*  Opcode VEX.66.0F38 0x4c - invalid. */
/*  Opcode VEX.66.0F38 0x4d - invalid. */
/*  Opcode VEX.66.0F38 0x4e - invalid. */
/*  Opcode VEX.66.0F38 0x4f - invalid. */

/*  Opcode VEX.66.0F38 0x50 - invalid. */
/*  Opcode VEX.66.0F38 0x51 - invalid. */
/*  Opcode VEX.66.0F38 0x52 - invalid. */
/*  Opcode VEX.66.0F38 0x53 - invalid. */
/*  Opcode VEX.66.0F38 0x54 - invalid. */
/*  Opcode VEX.66.0F38 0x55 - invalid. */
/*  Opcode VEX.66.0F38 0x56 - invalid. */
/*  Opcode VEX.66.0F38 0x57 - invalid. */
/** Opcode VEX.66.0F38 0x58. */
FNIEMOP_STUB(iemOp_vpbroadcastd_Vx_Wx);
/** Opcode VEX.66.0F38 0x59. */
FNIEMOP_STUB(iemOp_vpbroadcastq_Vx_Wx);
/** Opcode VEX.66.0F38 0x5a. */
FNIEMOP_STUB(iemOp_vbroadcasti128_Vqq_Mdq);
/*  Opcode VEX.66.0F38 0x5b - invalid. */
/*  Opcode VEX.66.0F38 0x5c - invalid. */
/*  Opcode VEX.66.0F38 0x5d - invalid. */
/*  Opcode VEX.66.0F38 0x5e - invalid. */
/*  Opcode VEX.66.0F38 0x5f - invalid. */

/*  Opcode VEX.66.0F38 0x60 - invalid. */
/*  Opcode VEX.66.0F38 0x61 - invalid. */
/*  Opcode VEX.66.0F38 0x62 - invalid. */
/*  Opcode VEX.66.0F38 0x63 - invalid. */
/*  Opcode VEX.66.0F38 0x64 - invalid. */
/*  Opcode VEX.66.0F38 0x65 - invalid. */
/*  Opcode VEX.66.0F38 0x66 - invalid. */
/*  Opcode VEX.66.0F38 0x67 - invalid. */
/*  Opcode VEX.66.0F38 0x68 - invalid. */
/*  Opcode VEX.66.0F38 0x69 - invalid. */
/*  Opcode VEX.66.0F38 0x6a - invalid. */
/*  Opcode VEX.66.0F38 0x6b - invalid. */
/*  Opcode VEX.66.0F38 0x6c - invalid. */
/*  Opcode VEX.66.0F38 0x6d - invalid. */
/*  Opcode VEX.66.0F38 0x6e - invalid. */
/*  Opcode VEX.66.0F38 0x6f - invalid. */

/*  Opcode VEX.66.0F38 0x70 - invalid. */
/*  Opcode VEX.66.0F38 0x71 - invalid. */
/*  Opcode VEX.66.0F38 0x72 - invalid. */
/*  Opcode VEX.66.0F38 0x73 - invalid. */
/*  Opcode VEX.66.0F38 0x74 - invalid. */
/*  Opcode VEX.66.0F38 0x75 - invalid. */
/*  Opcode VEX.66.0F38 0x76 - invalid. */
/*  Opcode VEX.66.0F38 0x77 - invalid. */
/** Opcode VEX.66.0F38 0x78. */
FNIEMOP_STUB(iemOp_vpboardcastb_Vx_Wx);
/** Opcode VEX.66.0F38 0x79. */
FNIEMOP_STUB(iemOp_vpboardcastw_Vx_Wx);
/*  Opcode VEX.66.0F38 0x7a - invalid. */
/*  Opcode VEX.66.0F38 0x7b - invalid. */
/*  Opcode VEX.66.0F38 0x7c - invalid. */
/*  Opcode VEX.66.0F38 0x7d - invalid. */
/*  Opcode VEX.66.0F38 0x7e - invalid. */
/*  Opcode VEX.66.0F38 0x7f - invalid. */

/*  Opcode VEX.66.0F38 0x80 - invalid (legacy only). */
/*  Opcode VEX.66.0F38 0x81 - invalid (legacy only). */
/*  Opcode VEX.66.0F38 0x82 - invalid (legacy only). */
/*  Opcode VEX.66.0F38 0x83 - invalid. */
/*  Opcode VEX.66.0F38 0x84 - invalid. */
/*  Opcode VEX.66.0F38 0x85 - invalid. */
/*  Opcode VEX.66.0F38 0x86 - invalid. */
/*  Opcode VEX.66.0F38 0x87 - invalid. */
/*  Opcode VEX.66.0F38 0x88 - invalid. */
/*  Opcode VEX.66.0F38 0x89 - invalid. */
/*  Opcode VEX.66.0F38 0x8a - invalid. */
/*  Opcode VEX.66.0F38 0x8b - invalid. */
/** Opcode VEX.66.0F38 0x8c. */
FNIEMOP_STUB(iemOp_vpmaskmovd_q_Vx_Hx_Mx);
/*  Opcode VEX.66.0F38 0x8d - invalid. */
/** Opcode VEX.66.0F38 0x8e. */
FNIEMOP_STUB(iemOp_vpmaskmovd_q_Mx_Vx_Hx);
/*  Opcode VEX.66.0F38 0x8f - invalid. */

/** Opcode VEX.66.0F38 0x90 (vex only). */
FNIEMOP_STUB(iemOp_vgatherdd_q_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x91 (vex only). */
FNIEMOP_STUB(iemOp_vgatherqd_q_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x92 (vex only). */
FNIEMOP_STUB(iemOp_vgatherdps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x93 (vex only). */
FNIEMOP_STUB(iemOp_vgatherqps_d_Vx_Hx_Wx);
/*  Opcode VEX.66.0F38 0x94 - invalid. */
/*  Opcode VEX.66.0F38 0x95 - invalid. */
/** Opcode VEX.66.0F38 0x96 (vex only). */
FNIEMOP_STUB(iemOp_vfmaddsub132ps_q_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x97 (vex only). */
FNIEMOP_STUB(iemOp_vfmsubadd132ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x98 (vex only). */
FNIEMOP_STUB(iemOp_vfmadd132ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x99 (vex only). */
FNIEMOP_STUB(iemOp_vfmadd132ss_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x9a (vex only). */
FNIEMOP_STUB(iemOp_vfmsub132ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x9b (vex only). */
FNIEMOP_STUB(iemOp_vfmsub132ss_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x9c (vex only). */
FNIEMOP_STUB(iemOp_vfnmadd132ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x9d (vex only). */
FNIEMOP_STUB(iemOp_vfnmadd132ss_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x9e (vex only). */
FNIEMOP_STUB(iemOp_vfnmsub132ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0x9f (vex only). */
FNIEMOP_STUB(iemOp_vfnmsub132ss_d_Vx_Hx_Wx);

/*  Opcode VEX.66.0F38 0xa0 - invalid. */
/*  Opcode VEX.66.0F38 0xa1 - invalid. */
/*  Opcode VEX.66.0F38 0xa2 - invalid. */
/*  Opcode VEX.66.0F38 0xa3 - invalid. */
/*  Opcode VEX.66.0F38 0xa4 - invalid. */
/*  Opcode VEX.66.0F38 0xa5 - invalid. */
/** Opcode VEX.66.0F38 0xa6 (vex only). */
FNIEMOP_STUB(iemOp_vfmaddsub213ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xa7 (vex only). */
FNIEMOP_STUB(iemOp_vfmsubadd213ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xa8 (vex only). */
FNIEMOP_STUB(iemOp_vfmadd213ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xa9 (vex only). */
FNIEMOP_STUB(iemOp_vfmadd213ss_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xaa (vex only). */
FNIEMOP_STUB(iemOp_vfmsub213ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xab (vex only). */
FNIEMOP_STUB(iemOp_vfmsub213ss_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xac (vex only). */
FNIEMOP_STUB(iemOp_vfnmadd213ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xad (vex only). */
FNIEMOP_STUB(iemOp_vfnmadd213ss_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xae (vex only). */
FNIEMOP_STUB(iemOp_vfnmsub213ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xaf (vex only). */
FNIEMOP_STUB(iemOp_vfnmsub213ss_d_Vx_Hx_Wx);

/*  Opcode VEX.66.0F38 0xb0 - invalid. */
/*  Opcode VEX.66.0F38 0xb1 - invalid. */
/*  Opcode VEX.66.0F38 0xb2 - invalid. */
/*  Opcode VEX.66.0F38 0xb3 - invalid. */
/*  Opcode VEX.66.0F38 0xb4 - invalid. */
/*  Opcode VEX.66.0F38 0xb5 - invalid. */
/** Opcode VEX.66.0F38 0xb6 (vex only). */
FNIEMOP_STUB(iemOp_vfmaddsub231ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xb7 (vex only). */
FNIEMOP_STUB(iemOp_vfmsubadd231ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xb8 (vex only). */
FNIEMOP_STUB(iemOp_vfmadd231ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xb9 (vex only). */
FNIEMOP_STUB(iemOp_vfmadd231ss_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xba (vex only). */
FNIEMOP_STUB(iemOp_vfmsub231ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xbb (vex only). */
FNIEMOP_STUB(iemOp_vfmsub231ss_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xbc (vex only). */
FNIEMOP_STUB(iemOp_vfnmadd231ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xbd (vex only). */
FNIEMOP_STUB(iemOp_vfnmadd231ss_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xbe (vex only). */
FNIEMOP_STUB(iemOp_vfnmsub231ps_d_Vx_Hx_Wx);
/** Opcode VEX.66.0F38 0xbf (vex only). */
FNIEMOP_STUB(iemOp_vfnmsub231ss_d_Vx_Hx_Wx);

/*  Opcode VEX.0F38 0xc0 - invalid. */
/*  Opcode VEX.66.0F38 0xc0 - invalid. */
/*  Opcode VEX.0F38 0xc1 - invalid. */
/*  Opcode VEX.66.0F38 0xc1 - invalid. */
/*  Opcode VEX.0F38 0xc2 - invalid. */
/*  Opcode VEX.66.0F38 0xc2 - invalid. */
/*  Opcode VEX.0F38 0xc3 - invalid. */
/*  Opcode VEX.66.0F38 0xc3 - invalid. */
/*  Opcode VEX.0F38 0xc4 - invalid. */
/*  Opcode VEX.66.0F38 0xc4 - invalid. */
/*  Opcode VEX.0F38 0xc5 - invalid. */
/*  Opcode VEX.66.0F38 0xc5 - invalid. */
/*  Opcode VEX.0F38 0xc6 - invalid. */
/*  Opcode VEX.66.0F38 0xc6 - invalid. */
/*  Opcode VEX.0F38 0xc7 - invalid. */
/*  Opcode VEX.66.0F38 0xc7 - invalid. */
/** Opcode VEX.0F38 0xc8. */
FNIEMOP_STUB(iemOp_vsha1nexte_Vdq_Wdq);
/*  Opcode VEX.66.0F38 0xc8 - invalid. */
/** Opcode VEX.0F38 0xc9. */
FNIEMOP_STUB(iemOp_vsha1msg1_Vdq_Wdq);
/*  Opcode VEX.66.0F38 0xc9 - invalid. */
/** Opcode VEX.0F38 0xca. */
FNIEMOP_STUB(iemOp_vsha1msg2_Vdq_Wdq);
/*  Opcode VEX.66.0F38 0xca - invalid. */
/** Opcode VEX.0F38 0xcb. */
FNIEMOP_STUB(iemOp_vsha256rnds2_Vdq_Wdq);
/*  Opcode VEX.66.0F38 0xcb - invalid. */
/** Opcode VEX.0F38 0xcc. */
FNIEMOP_STUB(iemOp_vsha256msg1_Vdq_Wdq);
/*  Opcode VEX.66.0F38 0xcc - invalid. */
/** Opcode VEX.0F38 0xcd. */
FNIEMOP_STUB(iemOp_vsha256msg2_Vdq_Wdq);
/*  Opcode VEX.66.0F38 0xcd - invalid. */
/*  Opcode VEX.0F38 0xce - invalid. */
/*  Opcode VEX.66.0F38 0xce - invalid. */
/*  Opcode VEX.0F38 0xcf - invalid. */
/*  Opcode VEX.66.0F38 0xcf - invalid. */

/*  Opcode VEX.66.0F38 0xd0 - invalid. */
/*  Opcode VEX.66.0F38 0xd1 - invalid. */
/*  Opcode VEX.66.0F38 0xd2 - invalid. */
/*  Opcode VEX.66.0F38 0xd3 - invalid. */
/*  Opcode VEX.66.0F38 0xd4 - invalid. */
/*  Opcode VEX.66.0F38 0xd5 - invalid. */
/*  Opcode VEX.66.0F38 0xd6 - invalid. */
/*  Opcode VEX.66.0F38 0xd7 - invalid. */
/*  Opcode VEX.66.0F38 0xd8 - invalid. */
/*  Opcode VEX.66.0F38 0xd9 - invalid. */
/*  Opcode VEX.66.0F38 0xda - invalid. */
/** Opcode VEX.66.0F38 0xdb. */
FNIEMOP_STUB(iemOp_vaesimc_Vdq_Wdq);
/** Opcode VEX.66.0F38 0xdc. */
FNIEMOP_STUB(iemOp_vaesenc_Vdq_Wdq);
/** Opcode VEX.66.0F38 0xdd. */
FNIEMOP_STUB(iemOp_vaesenclast_Vdq_Wdq);
/** Opcode VEX.66.0F38 0xde. */
FNIEMOP_STUB(iemOp_vaesdec_Vdq_Wdq);
/** Opcode VEX.66.0F38 0xdf. */
FNIEMOP_STUB(iemOp_vaesdeclast_Vdq_Wdq);

/*  Opcode VEX.66.0F38 0xe0 - invalid. */
/*  Opcode VEX.66.0F38 0xe1 - invalid. */
/*  Opcode VEX.66.0F38 0xe2 - invalid. */
/*  Opcode VEX.66.0F38 0xe3 - invalid. */
/*  Opcode VEX.66.0F38 0xe4 - invalid. */
/*  Opcode VEX.66.0F38 0xe5 - invalid. */
/*  Opcode VEX.66.0F38 0xe6 - invalid. */
/*  Opcode VEX.66.0F38 0xe7 - invalid. */
/*  Opcode VEX.66.0F38 0xe8 - invalid. */
/*  Opcode VEX.66.0F38 0xe9 - invalid. */
/*  Opcode VEX.66.0F38 0xea - invalid. */
/*  Opcode VEX.66.0F38 0xeb - invalid. */
/*  Opcode VEX.66.0F38 0xec - invalid. */
/*  Opcode VEX.66.0F38 0xed - invalid. */
/*  Opcode VEX.66.0F38 0xee - invalid. */
/*  Opcode VEX.66.0F38 0xef - invalid. */


/*  Opcode VEX.0F38 0xf0 - invalid (legacy only). */
/*  Opcode VEX.66.0F38 0xf0 - invalid (legacy only). */
/*  Opcode VEX.F3.0F38 0xf0 - invalid. */
/*  Opcode VEX.F2.0F38 0xf0 - invalid (legacy only). */

/*  Opcode VEX.0F38 0xf1 - invalid (legacy only). */
/*  Opcode VEX.66.0F38 0xf1 - invalid (legacy only). */
/*  Opcode VEX.F3.0F38 0xf1 - invalid. */
/*  Opcode VEX.F2.0F38 0xf1 - invalid (legacy only). */

/*  Opcode VEX.0F38 0xf2 - invalid (vex only). */
FNIEMOP_STUB(iemOp_andn_Gy_By_Ey);
/*  Opcode VEX.66.0F38 0xf2 - invalid. */
/*  Opcode VEX.F3.0F38 0xf2 - invalid. */
/*  Opcode VEX.F2.0F38 0xf2 - invalid. */


/*  Opcode VEX.0F38 0xf3 - invalid. */
/*  Opcode VEX.66.0F38 0xf3 - invalid. */

/*  Opcode VEX.F3.0F38 0xf3 /0 - invalid). */
/*  Opcode VEX.F3.0F38 0xf3 /1). */
FNIEMOP_STUB_1(iemOp_VGrp17_blsr_By_Ey, uint8_t, bRm);
/*  Opcode VEX.F3.0F38 0xf3 /2). */
FNIEMOP_STUB_1(iemOp_VGrp17_blsmsk_By_Ey, uint8_t, bRm);
/*  Opcode VEX.F3.0F38 0xf3 /3). */
FNIEMOP_STUB_1(iemOp_VGrp17_blsi_By_Ey, uint8_t, bRm);
/*  Opcode VEX.F3.0F38 0xf3 /4 - invalid). */
/*  Opcode VEX.F3.0F38 0xf3 /5 - invalid). */
/*  Opcode VEX.F3.0F38 0xf3 /6 - invalid). */
/*  Opcode VEX.F3.0F38 0xf3 /7 - invalid). */

/**
 * Group 17 jump table for the VEX.F3 variant..
 */
IEM_STATIC const PFNIEMOPRM g_apfnVexGroup17_f3[] =
{
    /* /0 */ iemOp_InvalidWithRM,
    /* /1 */ iemOp_VGrp17_blsr_By_Ey,
    /* /2 */ iemOp_VGrp17_blsmsk_By_Ey,
    /* /3 */ iemOp_VGrp17_blsi_By_Ey,
    /* /4 */ iemOp_InvalidWithRM,
    /* /5 */ iemOp_InvalidWithRM,
    /* /6 */ iemOp_InvalidWithRM,
    /* /7 */ iemOp_InvalidWithRM
};
AssertCompile(RT_ELEMENTS(g_apfnVexGroup17_f3) == 8);

/**  Opcode VEX.F3.0F38 0xf3 - invalid (vex only - group 17). */
FNIEMOP_DEF(iemOp_VGrp17_f3)
{
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    return FNIEMOP_CALL_1(g_apfnVexGroup17_f3[((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK)], bRm);

}

/*  Opcode VEX.F2.0F38 0xf3 - invalid (vex only - group 17). */


/*  Opcode VEX.0F38 0xf4 - invalid. */
/*  Opcode VEX.66.0F38 0xf4 - invalid. */
/*  Opcode VEX.F3.0F38 0xf4 - invalid. */
/*  Opcode VEX.F2.0F38 0xf4 - invalid. */

/** Opcode VEX.0F38 0xf5 (vex only). */
FNIEMOP_STUB(iemOp_bzhi_Gy_Ey_By);
/*  Opcode VEX.66.0F38 0xf5 - invalid. */
/** Opcode VEX.F3.0F38 0xf5 (vex only). */
FNIEMOP_STUB(iemOp_pext_Gy_By_Ey);
/** Opcode VEX.F2.0F38 0xf5 (vex only). */
FNIEMOP_STUB(iemOp_pdep_Gy_By_Ey);

/*  Opcode VEX.0F38 0xf6 - invalid. */
/*  Opcode VEX.66.0F38 0xf6 - invalid (legacy only). */
/*  Opcode VEX.F3.0F38 0xf6 - invalid (legacy only). */
/*  Opcode VEX.F2.0F38 0xf6 - invalid (vex only). */
FNIEMOP_STUB(iemOp_mulx_By_Gy_rDX_Ey);

/** Opcode VEX.0F38 0xf7 (vex only). */
FNIEMOP_STUB(iemOp_bextr_Gy_Ey_By);
/** Opcode VEX.66.0F38 0xf7 (vex only). */
FNIEMOP_STUB(iemOp_shlx_Gy_Ey_By);
/** Opcode VEX.F3.0F38 0xf7 (vex only). */
FNIEMOP_STUB(iemOp_sarx_Gy_Ey_By);
/** Opcode VEX.F2.0F38 0xf7 (vex only). */
FNIEMOP_STUB(iemOp_shrx_Gy_Ey_By);

/*  Opcode VEX.0F38 0xf8 - invalid. */
/*  Opcode VEX.66.0F38 0xf8 - invalid. */
/*  Opcode VEX.F3.0F38 0xf8 - invalid. */
/*  Opcode VEX.F2.0F38 0xf8 - invalid. */

/*  Opcode VEX.0F38 0xf9 - invalid. */
/*  Opcode VEX.66.0F38 0xf9 - invalid. */
/*  Opcode VEX.F3.0F38 0xf9 - invalid. */
/*  Opcode VEX.F2.0F38 0xf9 - invalid. */

/*  Opcode VEX.0F38 0xfa - invalid. */
/*  Opcode VEX.66.0F38 0xfa - invalid. */
/*  Opcode VEX.F3.0F38 0xfa - invalid. */
/*  Opcode VEX.F2.0F38 0xfa - invalid. */

/*  Opcode VEX.0F38 0xfb - invalid. */
/*  Opcode VEX.66.0F38 0xfb - invalid. */
/*  Opcode VEX.F3.0F38 0xfb - invalid. */
/*  Opcode VEX.F2.0F38 0xfb - invalid. */

/*  Opcode VEX.0F38 0xfc - invalid. */
/*  Opcode VEX.66.0F38 0xfc - invalid. */
/*  Opcode VEX.F3.0F38 0xfc - invalid. */
/*  Opcode VEX.F2.0F38 0xfc - invalid. */

/*  Opcode VEX.0F38 0xfd - invalid. */
/*  Opcode VEX.66.0F38 0xfd - invalid. */
/*  Opcode VEX.F3.0F38 0xfd - invalid. */
/*  Opcode VEX.F2.0F38 0xfd - invalid. */

/*  Opcode VEX.0F38 0xfe - invalid. */
/*  Opcode VEX.66.0F38 0xfe - invalid. */
/*  Opcode VEX.F3.0F38 0xfe - invalid. */
/*  Opcode VEX.F2.0F38 0xfe - invalid. */

/*  Opcode VEX.0F38 0xff - invalid. */
/*  Opcode VEX.66.0F38 0xff - invalid. */
/*  Opcode VEX.F3.0F38 0xff - invalid. */
/*  Opcode VEX.F2.0F38 0xff - invalid. */


/**
 * VEX opcode map \#2.
 *
 * @sa      g_apfnThreeByte0f38
 */
IEM_STATIC const PFNIEMOP g_apfnVexMap2[] =
{
    /*          no prefix,                  066h prefix                 f3h prefix,                 f2h prefix */
    /* 0x00 */  iemOp_InvalidNeedRM,        iemOp_vpshufb_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x01 */  iemOp_InvalidNeedRM,        iemOp_vphaddw_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x02 */  iemOp_InvalidNeedRM,        iemOp_vphaddd_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x03 */  iemOp_InvalidNeedRM,        iemOp_vphaddsw_Vx_Hx_Wx,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x04 */  iemOp_InvalidNeedRM,        iemOp_vpmaddubsw_Vx_Hx_Wx,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x05 */  iemOp_InvalidNeedRM,        iemOp_vphsubw_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x06 */  iemOp_InvalidNeedRM,        iemOp_vphsubdq_Vx_Hx_Wx,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x07 */  iemOp_InvalidNeedRM,        iemOp_vphsubsw_Vx_Hx_Wx,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x08 */  iemOp_InvalidNeedRM,        iemOp_vpsignb_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x09 */  iemOp_InvalidNeedRM,        iemOp_vpsignw_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x0a */  iemOp_InvalidNeedRM,        iemOp_vpsignd_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x0b */  iemOp_InvalidNeedRM,        iemOp_vpmulhrsw_Vx_Hx_Wx,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x0c */  iemOp_InvalidNeedRM,        iemOp_vpermilps_Vx_Hx_Wx,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x0d */  iemOp_InvalidNeedRM,        iemOp_vpermilpd_Vx_Hx_Wx,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x0e */  iemOp_InvalidNeedRM,        iemOp_vtestps_Vx_Wx,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x0f */  iemOp_InvalidNeedRM,        iemOp_vtestpd_Vx_Wx,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,

    /* 0x10 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x11 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x12 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x13 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x14 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x15 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x16 */  iemOp_InvalidNeedRM,        iemOp_vpermps_Vqq_Hqq_Wqq,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x17 */  iemOp_InvalidNeedRM,        iemOp_vptest_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x18 */  iemOp_InvalidNeedRM,        iemOp_vbroadcastss_Vx_Wd,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x19 */  iemOp_InvalidNeedRM,        iemOp_vbroadcastsd_Vqq_Wq,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x1a */  iemOp_InvalidNeedRM,        iemOp_vbroadcastf128_Vqq_Mdq, iemOp_InvalidNeedRM,      iemOp_InvalidNeedRM,
    /* 0x1b */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x1c */  iemOp_InvalidNeedRM,        iemOp_vpabsb_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x1d */  iemOp_InvalidNeedRM,        iemOp_vpabsw_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x1e */  iemOp_InvalidNeedRM,        iemOp_vpabsd_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x1f */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0x20 */  iemOp_InvalidNeedRM,        iemOp_vpmovsxbw_Vx_UxMq,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x21 */  iemOp_InvalidNeedRM,        iemOp_vpmovsxbd_Vx_UxMd,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x22 */  iemOp_InvalidNeedRM,        iemOp_vpmovsxbq_Vx_UxMw,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x23 */  iemOp_InvalidNeedRM,        iemOp_vpmovsxwd_Vx_UxMq,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x24 */  iemOp_InvalidNeedRM,        iemOp_vpmovsxwq_Vx_UxMd,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x25 */  iemOp_InvalidNeedRM,        iemOp_vpmovsxdq_Vx_UxMq,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x26 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x27 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x28 */  iemOp_InvalidNeedRM,        iemOp_vpmuldq_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x29 */  iemOp_InvalidNeedRM,        iemOp_vpcmpeqq_Vx_Hx_Wx,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x2a */  iemOp_InvalidNeedRM,        iemOp_vmovntdqa_Vx_Mx,      iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x2b */  iemOp_InvalidNeedRM,        iemOp_vpackusdw_Vx_Hx_Wx,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x2c */  iemOp_InvalidNeedRM,        iemOp_vmaskmovps_Vx_Hx_Mx,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x2d */  iemOp_InvalidNeedRM,        iemOp_vmaskmovpd_Vx_Hx_Mx,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x2e */  iemOp_InvalidNeedRM,        iemOp_vmaskmovps_Mx_Hx_Vx,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x2f */  iemOp_InvalidNeedRM,        iemOp_vmaskmovpd_Mx_Hx_Vx,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,

    /* 0x30 */  iemOp_InvalidNeedRM,        iemOp_vpmovzxbw_Vx_UxMq,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x31 */  iemOp_InvalidNeedRM,        iemOp_vpmovzxbd_Vx_UxMd,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x32 */  iemOp_InvalidNeedRM,        iemOp_vpmovzxbq_Vx_UxMw,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x33 */  iemOp_InvalidNeedRM,        iemOp_vpmovzxwd_Vx_UxMq,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x34 */  iemOp_InvalidNeedRM,        iemOp_vpmovzxwq_Vx_UxMd,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x35 */  iemOp_InvalidNeedRM,        iemOp_vpmovzxdq_Vx_UxMq,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x36 */  iemOp_InvalidNeedRM,        iemOp_vpermd_Vqq_Hqq_Wqq,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x37 */  iemOp_InvalidNeedRM,        iemOp_vpcmpgtq_Vx_Hx_Wx,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x38 */  iemOp_InvalidNeedRM,        iemOp_vpminsb_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x39 */  iemOp_InvalidNeedRM,        iemOp_vpminsd_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x3a */  iemOp_InvalidNeedRM,        iemOp_vpminuw_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x3b */  iemOp_InvalidNeedRM,        iemOp_vpminud_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x3c */  iemOp_InvalidNeedRM,        iemOp_vpmaxsb_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x3d */  iemOp_InvalidNeedRM,        iemOp_vpmaxsd_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x3e */  iemOp_InvalidNeedRM,        iemOp_vpmaxuw_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x3f */  iemOp_InvalidNeedRM,        iemOp_vpmaxud_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,

    /* 0x40 */  iemOp_InvalidNeedRM,        iemOp_vpmulld_Vx_Hx_Wx,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x41 */  iemOp_InvalidNeedRM,        iemOp_vphminposuw_Vdq_Wdq,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x42 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x43 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x44 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x45 */  iemOp_InvalidNeedRM,        iemOp_vpsrlvd_q_Vx_Hx_Wx,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x46 */  iemOp_InvalidNeedRM,        iemOp_vsravd_Vx_Hx_Wx,      iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x47 */  iemOp_InvalidNeedRM,        iemOp_vpsllvd_q_Vx_Hx_Wx,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x48 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x49 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x4a */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x4b */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x4c */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x4d */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x4e */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x4f */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0x50 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x51 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x52 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x53 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x54 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x55 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x56 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x57 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x58 */  iemOp_InvalidNeedRM,        iemOp_vpbroadcastd_Vx_Wx,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x59 */  iemOp_InvalidNeedRM,        iemOp_vpbroadcastq_Vx_Wx,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x5a */  iemOp_InvalidNeedRM,        iemOp_vbroadcasti128_Vqq_Mdq, iemOp_InvalidNeedRM,      iemOp_InvalidNeedRM,
    /* 0x5b */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x5c */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x5d */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x5e */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x5f */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0x60 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x61 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x62 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x63 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x64 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x65 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x66 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x67 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x68 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x69 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x6a */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x6b */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x6c */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x6d */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x6e */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x6f */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0x70 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x71 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x72 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x73 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x74 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x75 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x76 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x77 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x78 */  iemOp_InvalidNeedRM,        iemOp_vpboardcastb_Vx_Wx,   iemOp_InvalidNeedRM,      iemOp_InvalidNeedRM,
    /* 0x79 */  iemOp_InvalidNeedRM,        iemOp_vpboardcastw_Vx_Wx,   iemOp_InvalidNeedRM,      iemOp_InvalidNeedRM,
    /* 0x7a */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x7b */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x7c */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x7d */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x7e */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x7f */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0x80 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x81 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x82 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x83 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x84 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x85 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x86 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x87 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x88 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x89 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x8a */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x8b */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x8c */  iemOp_InvalidNeedRM,        iemOp_vpmaskmovd_q_Vx_Hx_Mx, iemOp_InvalidNeedRM,     iemOp_InvalidNeedRM,
    /* 0x8d */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x8e */  iemOp_InvalidNeedRM,        iemOp_vpmaskmovd_q_Mx_Vx_Hx, iemOp_InvalidNeedRM,     iemOp_InvalidNeedRM,
    /* 0x8f */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0x90 */  iemOp_InvalidNeedRM,        iemOp_vgatherdd_q_Vx_Hx_Wx, iemOp_InvalidNeedRM,      iemOp_InvalidNeedRM,
    /* 0x91 */  iemOp_InvalidNeedRM,        iemOp_vgatherqd_q_Vx_Hx_Wx, iemOp_InvalidNeedRM,      iemOp_InvalidNeedRM,
    /* 0x92 */  iemOp_InvalidNeedRM,        iemOp_vgatherdps_d_Vx_Hx_Wx, iemOp_InvalidNeedRM,     iemOp_InvalidNeedRM,
    /* 0x93 */  iemOp_InvalidNeedRM,        iemOp_vgatherqps_d_Vx_Hx_Wx, iemOp_InvalidNeedRM,     iemOp_InvalidNeedRM,
    /* 0x94 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x95 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x96 */  iemOp_InvalidNeedRM,        iemOp_vfmaddsub132ps_q_Vx_Hx_Wx, iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0x97 */  iemOp_InvalidNeedRM,        iemOp_vfmsubadd132ps_d_Vx_Hx_Wx, iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0x98 */  iemOp_InvalidNeedRM,        iemOp_vfmadd132ps_d_Vx_Hx_Wx,    iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0x99 */  iemOp_InvalidNeedRM,        iemOp_vfmadd132ss_d_Vx_Hx_Wx,    iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0x9a */  iemOp_InvalidNeedRM,        iemOp_vfmsub132ps_d_Vx_Hx_Wx,    iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0x9b */  iemOp_InvalidNeedRM,        iemOp_vfmsub132ss_d_Vx_Hx_Wx,    iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0x9c */  iemOp_InvalidNeedRM,        iemOp_vfnmadd132ps_d_Vx_Hx_Wx,   iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0x9d */  iemOp_InvalidNeedRM,        iemOp_vfnmadd132ss_d_Vx_Hx_Wx,   iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0x9e */  iemOp_InvalidNeedRM,        iemOp_vfnmsub132ps_d_Vx_Hx_Wx,   iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0x9f */  iemOp_InvalidNeedRM,        iemOp_vfnmsub132ss_d_Vx_Hx_Wx,   iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,

    /* 0xa0 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa1 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa2 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa3 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa4 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa5 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa6 */  iemOp_InvalidNeedRM,        iemOp_vfmaddsub213ps_d_Vx_Hx_Wx, iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xa7 */  iemOp_InvalidNeedRM,        iemOp_vfmsubadd213ps_d_Vx_Hx_Wx, iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xa8 */  iemOp_InvalidNeedRM,        iemOp_vfmadd213ps_d_Vx_Hx_Wx,    iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xa9 */  iemOp_InvalidNeedRM,        iemOp_vfmadd213ss_d_Vx_Hx_Wx,    iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xaa */  iemOp_InvalidNeedRM,        iemOp_vfmsub213ps_d_Vx_Hx_Wx,    iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xab */  iemOp_InvalidNeedRM,        iemOp_vfmsub213ss_d_Vx_Hx_Wx,    iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xac */  iemOp_InvalidNeedRM,        iemOp_vfnmadd213ps_d_Vx_Hx_Wx,   iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xad */  iemOp_InvalidNeedRM,        iemOp_vfnmadd213ss_d_Vx_Hx_Wx,   iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xae */  iemOp_InvalidNeedRM,        iemOp_vfnmsub213ps_d_Vx_Hx_Wx,   iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xaf */  iemOp_InvalidNeedRM,        iemOp_vfnmsub213ss_d_Vx_Hx_Wx,   iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,

    /* 0xb0 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb1 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb2 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb3 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb4 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb5 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb6 */  iemOp_InvalidNeedRM,        iemOp_vfmaddsub231ps_d_Vx_Hx_Wx, iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xb7 */  iemOp_InvalidNeedRM,        iemOp_vfmsubadd231ps_d_Vx_Hx_Wx, iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xb8 */  iemOp_InvalidNeedRM,        iemOp_vfmadd231ps_d_Vx_Hx_Wx,    iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xb9 */  iemOp_InvalidNeedRM,        iemOp_vfmadd231ss_d_Vx_Hx_Wx,    iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xba */  iemOp_InvalidNeedRM,        iemOp_vfmsub231ps_d_Vx_Hx_Wx,    iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xbb */  iemOp_InvalidNeedRM,        iemOp_vfmsub231ss_d_Vx_Hx_Wx,    iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xbc */  iemOp_InvalidNeedRM,        iemOp_vfnmadd231ps_d_Vx_Hx_Wx,   iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xbd */  iemOp_InvalidNeedRM,        iemOp_vfnmadd231ss_d_Vx_Hx_Wx,   iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xbe */  iemOp_InvalidNeedRM,        iemOp_vfnmsub231ps_d_Vx_Hx_Wx,   iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,
    /* 0xbf */  iemOp_InvalidNeedRM,        iemOp_vfnmsub231ss_d_Vx_Hx_Wx,   iemOp_InvalidNeedRM, iemOp_InvalidNeedRM,

    /* 0xc0 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc1 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc2 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc3 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc4 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc5 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc6 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc7 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc8 */  iemOp_vsha1nexte_Vdq_Wdq,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xc9 */  iemOp_vsha1msg1_Vdq_Wdq,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xca */  iemOp_vsha1msg2_Vdq_Wdq,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xcb */  iemOp_vsha256rnds2_Vdq_Wdq, iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xcc */  iemOp_vsha256msg1_Vdq_Wdq,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xcd */  iemOp_vsha256msg2_Vdq_Wdq,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xce */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xcf */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0xd0 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xd1 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xd2 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xd3 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xd4 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xd5 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xd6 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xd7 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xd8 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xd9 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xda */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xdb */  iemOp_InvalidNeedRM,        iemOp_vaesimc_Vdq_Wdq,      iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xdc */  iemOp_InvalidNeedRM,        iemOp_vaesenc_Vdq_Wdq,      iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xdd */  iemOp_InvalidNeedRM,        iemOp_vaesenclast_Vdq_Wdq,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xde */  iemOp_InvalidNeedRM,        iemOp_vaesdec_Vdq_Wdq,      iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xdf */  iemOp_InvalidNeedRM,        iemOp_vaesdeclast_Vdq_Wdq,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,

    /* 0xe0 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xe1 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xe2 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xe3 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xe4 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xe5 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xe6 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xe7 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xe8 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xe9 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xea */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xeb */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xec */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xed */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xee */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xef */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0xf0 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xf1 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xf2 */  iemOp_andn_Gy_By_Ey,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xf3 */  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_VGrp17_f3,            iemOp_InvalidNeedRM,
    /* 0xf4 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xf5 */  iemOp_bzhi_Gy_Ey_By,        iemOp_InvalidNeedRM,        iemOp_pext_Gy_By_Ey,        iemOp_pdep_Gy_By_Ey,
    /* 0xf6 */  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_mulx_By_Gy_rDX_Ey,
    /* 0xf7 */  iemOp_bextr_Gy_Ey_By,       iemOp_shlx_Gy_Ey_By,        iemOp_sarx_Gy_Ey_By,        iemOp_shrx_Gy_Ey_By,
    /* 0xf8 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xf9 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xfa */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xfb */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xfc */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xfd */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xfe */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xff */  IEMOP_X4(iemOp_InvalidNeedRM),
};
AssertCompile(RT_ELEMENTS(g_apfnVexMap2) == 1024);

/** @} */

