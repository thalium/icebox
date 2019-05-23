/* $Id: IEMAllInstructionsThree0f38.cpp.h $ */
/** @file
 * IEM - Instruction Decoding and Emulation.
 *
 * @remarks IEMAllInstructionsVexMap2.cpp.h is a VEX mirror of this file.
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


/** @name Three byte opcodes with first two bytes 0x0f 0x38
 * @{
 */

/*  Opcode      0x0f 0x38 0x00. */
FNIEMOP_STUB(iemOp_pshufb_Pq_Qq);
/*  Opcode 0x66 0x0f 0x38 0x00. */
FNIEMOP_STUB(iemOp_pshufb_Vx_Wx);
/*  Opcode      0x0f 0x38 0x01. */
FNIEMOP_STUB(iemOp_phaddw_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x01. */
FNIEMOP_STUB(iemOp_phaddw_Vx_Wx);
/** Opcode      0x0f 0x38 0x02. */
FNIEMOP_STUB(iemOp_phaddd_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x02. */
FNIEMOP_STUB(iemOp_phaddd_Vx_Wx);
/** Opcode      0x0f 0x38 0x03. */
FNIEMOP_STUB(iemOp_phaddsw_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x03. */
FNIEMOP_STUB(iemOp_phaddsw_Vx_Wx);
/** Opcode      0x0f 0x38 0x04. */
FNIEMOP_STUB(iemOp_pmaddubsw_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x04. */
FNIEMOP_STUB(iemOp_pmaddubsw_Vx_Wx);
/** Opcode      0x0f 0x38 0x05. */
FNIEMOP_STUB(iemOp_phsubw_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x05. */
FNIEMOP_STUB(iemOp_phsubw_Vx_Wx);
/** Opcode      0x0f 0x38 0x06. */
FNIEMOP_STUB(iemOp_phsubd_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x06. */
FNIEMOP_STUB(iemOp_phsubdq_Vx_Wx);
/** Opcode      0x0f 0x38 0x07. */
FNIEMOP_STUB(iemOp_phsubsw_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x07. */
FNIEMOP_STUB(iemOp_phsubsw_Vx_Wx);
/** Opcode      0x0f 0x38 0x08. */
FNIEMOP_STUB(iemOp_psignb_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x08. */
FNIEMOP_STUB(iemOp_psignb_Vx_Wx);
/** Opcode      0x0f 0x38 0x09. */
FNIEMOP_STUB(iemOp_psignw_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x09. */
FNIEMOP_STUB(iemOp_psignw_Vx_Wx);
/** Opcode      0x0f 0x38 0x0a. */
FNIEMOP_STUB(iemOp_psignd_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x0a. */
FNIEMOP_STUB(iemOp_psignd_Vx_Wx);
/** Opcode      0x0f 0x38 0x0b. */
FNIEMOP_STUB(iemOp_pmulhrsw_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x0b. */
FNIEMOP_STUB(iemOp_pmulhrsw_Vx_Wx);
/*  Opcode      0x0f 0x38 0x0c - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x0c - invalid (vex only). */
/*  Opcode      0x0f 0x38 0x0d - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x0d - invalid (vex only). */
/*  Opcode      0x0f 0x38 0x0e - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x0e - invalid (vex only). */
/*  Opcode      0x0f 0x38 0x0f - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x0f - invalid (vex only). */


/*  Opcode      0x0f 0x38 0x10 - invalid */
/** Opcode 0x66 0x0f 0x38 0x10 (legacy only). */
FNIEMOP_STUB(iemOp_pblendvb_Vdq_Wdq);
/*  Opcode      0x0f 0x38 0x11 - invalid */
/*  Opcode 0x66 0x0f 0x38 0x11 - invalid */
/*  Opcode      0x0f 0x38 0x12 - invalid */
/*  Opcode 0x66 0x0f 0x38 0x12 - invalid */
/*  Opcode      0x0f 0x38 0x13 - invalid */
/*  Opcode 0x66 0x0f 0x38 0x13 - invalid (vex only). */
/*  Opcode      0x0f 0x38 0x14 - invalid */
/** Opcode 0x66 0x0f 0x38 0x14 (legacy only). */
FNIEMOP_STUB(iemOp_blendvps_Vdq_Wdq);
/*  Opcode      0x0f 0x38 0x15 - invalid */
/** Opcode 0x66 0x0f 0x38 0x15 (legacy only). */
FNIEMOP_STUB(iemOp_blendvpd_Vdq_Wdq);
/*  Opcode      0x0f 0x38 0x16 - invalid */
/*  Opcode 0x66 0x0f 0x38 0x16 - invalid (vex only). */
/*  Opcode      0x0f 0x38 0x17 - invalid */
/** Opcode 0x66 0x0f 0x38 0x17 - invalid */
FNIEMOP_STUB(iemOp_ptest_Vx_Wx);
/*  Opcode      0x0f 0x38 0x18 - invalid */
/*  Opcode 0x66 0x0f 0x38 0x18 - invalid (vex only). */
/*  Opcode      0x0f 0x38 0x19 - invalid */
/*  Opcode 0x66 0x0f 0x38 0x19 - invalid (vex only). */
/*  Opcode      0x0f 0x38 0x1a - invalid */
/*  Opcode 0x66 0x0f 0x38 0x1a - invalid (vex only). */
/*  Opcode      0x0f 0x38 0x1b - invalid */
/*  Opcode 0x66 0x0f 0x38 0x1b - invalid */
/** Opcode      0x0f 0x38 0x1c. */
FNIEMOP_STUB(iemOp_pabsb_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x1c. */
FNIEMOP_STUB(iemOp_pabsb_Vx_Wx);
/** Opcode      0x0f 0x38 0x1d. */
FNIEMOP_STUB(iemOp_pabsw_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x1d. */
FNIEMOP_STUB(iemOp_pabsw_Vx_Wx);
/** Opcode      0x0f 0x38 0x1e. */
FNIEMOP_STUB(iemOp_pabsd_Pq_Qq);
/** Opcode 0x66 0x0f 0x38 0x1e. */
FNIEMOP_STUB(iemOp_pabsd_Vx_Wx);
/*  Opcode      0x0f 0x38 0x1f - invalid */
/*  Opcode 0x66 0x0f 0x38 0x1f - invalid */


/** Opcode 0x66 0x0f 0x38 0x20. */
FNIEMOP_STUB(iemOp_pmovsxbw_Vx_UxMq);
/** Opcode 0x66 0x0f 0x38 0x21. */
FNIEMOP_STUB(iemOp_pmovsxbd_Vx_UxMd);
/** Opcode 0x66 0x0f 0x38 0x22. */
FNIEMOP_STUB(iemOp_pmovsxbq_Vx_UxMw);
/** Opcode 0x66 0x0f 0x38 0x23. */
FNIEMOP_STUB(iemOp_pmovsxwd_Vx_UxMq);
/** Opcode 0x66 0x0f 0x38 0x24. */
FNIEMOP_STUB(iemOp_pmovsxwq_Vx_UxMd);
/** Opcode 0x66 0x0f 0x38 0x25. */
FNIEMOP_STUB(iemOp_pmovsxdq_Vx_UxMq);
/*  Opcode 0x66 0x0f 0x38 0x26 - invalid */
/*  Opcode 0x66 0x0f 0x38 0x27 - invalid */
/** Opcode 0x66 0x0f 0x38 0x28. */
FNIEMOP_STUB(iemOp_pmuldq_Vx_Wx);
/** Opcode 0x66 0x0f 0x38 0x29. */
FNIEMOP_STUB(iemOp_pcmpeqq_Vx_Wx);

/**
 * @opcode      0x2a
 * @opcodesub   !11 mr/reg
 * @oppfx       0x66
 * @opcpuid     sse4.1
 * @opgroup     og_sse41_cachect
 * @opxcpttype  1
 * @optest      op1=-1 op2=2  -> op1=2
 * @optest      op1=0 op2=-42 -> op1=-42
 */
FNIEMOP_DEF(iemOp_movntdqa_Vdq_Mdq)
{
    IEMOP_MNEMONIC2(RM_MEM, MOVNTDQA, movntdqa, Vdq_WO, Mdq, DISOPTYPE_HARMLESS, IEMOPHINT_IGNORES_OP_SIZES);
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
    {
        /* Register, memory. */
        IEM_MC_BEGIN(0, 2);
        IEM_MC_LOCAL(RTUINT128U,                uSrc);
        IEM_MC_LOCAL(RTGCPTR,                   GCPtrEffSrc);

        IEM_MC_CALC_RM_EFF_ADDR(GCPtrEffSrc, bRm, 0);
        IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
        IEM_MC_MAYBE_RAISE_SSE41_RELATED_XCPT();
        IEM_MC_ACTUALIZE_SSE_STATE_FOR_CHANGE();

        IEM_MC_FETCH_MEM_U128_ALIGN_SSE(uSrc, pVCpu->iem.s.iEffSeg, GCPtrEffSrc);
        IEM_MC_STORE_XREG_U128(((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg, uSrc);

        IEM_MC_ADVANCE_RIP();
        IEM_MC_END();
        return VINF_SUCCESS;
    }

    /**
     * @opdone
     * @opmnemonic  ud660f382areg
     * @opcode      0x2a
     * @opcodesub   11 mr/reg
     * @oppfx       0x66
     * @opunused    immediate
     * @opcpuid     sse
     * @optest      ->
     */
    return IEMOP_RAISE_INVALID_OPCODE();
}

/** Opcode 0x66 0x0f 0x38 0x2b. */
FNIEMOP_STUB(iemOp_packusdw_Vx_Wx);
/*  Opcode 0x66 0x0f 0x38 0x2c - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x2d - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x2e - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x2f - invalid (vex only). */

/** Opcode 0x66 0x0f 0x38 0x30. */
FNIEMOP_STUB(iemOp_pmovzxbw_Vx_UxMq);
/** Opcode 0x66 0x0f 0x38 0x31. */
FNIEMOP_STUB(iemOp_pmovzxbd_Vx_UxMd);
/** Opcode 0x66 0x0f 0x38 0x32. */
FNIEMOP_STUB(iemOp_pmovzxbq_Vx_UxMw);
/** Opcode 0x66 0x0f 0x38 0x33. */
FNIEMOP_STUB(iemOp_pmovzxwd_Vx_UxMq);
/** Opcode 0x66 0x0f 0x38 0x34. */
FNIEMOP_STUB(iemOp_pmovzxwq_Vx_UxMd);
/** Opcode 0x66 0x0f 0x38 0x35. */
FNIEMOP_STUB(iemOp_pmovzxdq_Vx_UxMq);
/*  Opcode 0x66 0x0f 0x38 0x36 - invalid (vex only). */
/** Opcode 0x66 0x0f 0x38 0x37. */
FNIEMOP_STUB(iemOp_pcmpgtq_Vx_Wx);
/** Opcode 0x66 0x0f 0x38 0x38. */
FNIEMOP_STUB(iemOp_pminsb_Vx_Wx);
/** Opcode 0x66 0x0f 0x38 0x39. */
FNIEMOP_STUB(iemOp_pminsd_Vx_Wx);
/** Opcode 0x66 0x0f 0x38 0x3a. */
FNIEMOP_STUB(iemOp_pminuw_Vx_Wx);
/** Opcode 0x66 0x0f 0x38 0x3b. */
FNIEMOP_STUB(iemOp_pminud_Vx_Wx);
/** Opcode 0x66 0x0f 0x38 0x3c. */
FNIEMOP_STUB(iemOp_pmaxsb_Vx_Wx);
/** Opcode 0x66 0x0f 0x38 0x3d. */
FNIEMOP_STUB(iemOp_pmaxsd_Vx_Wx);
/** Opcode 0x66 0x0f 0x38 0x3e. */
FNIEMOP_STUB(iemOp_pmaxuw_Vx_Wx);
/** Opcode 0x66 0x0f 0x38 0x3f. */
FNIEMOP_STUB(iemOp_pmaxud_Vx_Wx);


/** Opcode 0x66 0x0f 0x38 0x40. */
FNIEMOP_STUB(iemOp_pmulld_Vx_Wx);
/** Opcode 0x66 0x0f 0x38 0x41. */
FNIEMOP_STUB(iemOp_phminposuw_Vdq_Wdq);
/*  Opcode 0x66 0x0f 0x38 0x42 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x43 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x44 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x45 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x46 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x47 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x48 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x49 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x4a - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x4b - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x4c - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x4d - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x4e - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x4f - invalid. */

/*  Opcode 0x66 0x0f 0x38 0x50 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x51 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x52 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x53 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x54 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x55 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x56 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x57 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x58 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x59 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x5a - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x5b - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x5c - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x5d - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x5e - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x5f - invalid. */

/*  Opcode 0x66 0x0f 0x38 0x60 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x61 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x62 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x63 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x64 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x65 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x66 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x67 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x68 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x69 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x6a - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x6b - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x6c - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x6d - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x6e - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x6f - invalid. */

/*  Opcode 0x66 0x0f 0x38 0x70 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x71 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x72 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x73 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x74 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x75 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x76 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x77 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x78 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x79 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x7a - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x7b - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x7c - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x7d - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x7e - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x7f - invalid. */

/** Opcode 0x66 0x0f 0x38 0x80. */
FNIEMOP_STUB(iemOp_invept_Gy_Mdq);
/** Opcode 0x66 0x0f 0x38 0x81. */
FNIEMOP_STUB(iemOp_invvpid_Gy_Mdq);
/** Opcode 0x66 0x0f 0x38 0x82. */
FNIEMOP_DEF(iemOp_invpcid_Gy_Mdq)
{
    IEMOP_MNEMONIC(invpcid, "invpcid Gy,Mdq");
    IEMOP_HLP_DONE_DECODING_NO_LOCK_PREFIX();
    uint8_t bRm; IEM_OPCODE_GET_NEXT_U8(&bRm);
    if ((bRm & X86_MODRM_MOD_MASK) != (3 << X86_MODRM_MOD_SHIFT))
    {
        /* Register, memory. */
        if (pVCpu->iem.s.enmEffOpSize == IEMMODE_64BIT)
        {
            IEM_MC_BEGIN(2, 0);
            IEM_MC_ARG(uint64_t, uInvpcidType,     0);
            IEM_MC_ARG(RTGCPTR,  GCPtrInvpcidDesc, 1);
            IEM_MC_FETCH_GREG_U64(uInvpcidType, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrInvpcidDesc, bRm, 0);
            IEM_MC_CALL_CIMPL_2(iemCImpl_invpcid, uInvpcidType, GCPtrInvpcidDesc);
            IEM_MC_END();
        }
        else
        {
            IEM_MC_BEGIN(2, 0);
            IEM_MC_ARG(uint32_t, uInvpcidType,     0);
            IEM_MC_ARG(RTGCPTR,  GCPtrInvpcidDesc, 1);
            IEM_MC_FETCH_GREG_U32(uInvpcidType, ((bRm >> X86_MODRM_REG_SHIFT) & X86_MODRM_REG_SMASK) | pVCpu->iem.s.uRexReg);
            IEM_MC_CALC_RM_EFF_ADDR(GCPtrInvpcidDesc, bRm, 0);
            IEM_MC_CALL_CIMPL_2(iemCImpl_invpcid, uInvpcidType, GCPtrInvpcidDesc);
            IEM_MC_END();
        }
    }
    Log(("iemOp_invpcid_Gy_Mdq: invalid encoding -> #UD\n"));
    return IEMOP_RAISE_INVALID_OPCODE();
}


/*  Opcode 0x66 0x0f 0x38 0x83 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x84 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x85 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x86 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x87 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x88 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x89 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x8a - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x8b - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x8c - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x8d - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x8e - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x8f - invalid. */

/*  Opcode 0x66 0x0f 0x38 0x90 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x91 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x92 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x93 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x94 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x95 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0x96 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x97 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x98 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x99 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x9a - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x9b - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x9c - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x9d - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x9e - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0x9f - invalid (vex only). */

/*  Opcode 0x66 0x0f 0x38 0xa0 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xa1 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xa2 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xa3 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xa4 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xa5 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xa6 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xa7 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xa8 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xa9 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xaa - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xab - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xac - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xad - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xae - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xaf - invalid (vex only). */

/*  Opcode 0x66 0x0f 0x38 0xb0 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xb1 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xb2 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xb3 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xb4 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xb5 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xb6 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xb7 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xb8 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xb9 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xba - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xbb - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xbc - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xbd - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xbe - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xbf - invalid (vex only). */

/*  Opcode      0x0f 0x38 0xc0 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xc0 - invalid. */
/*  Opcode      0x0f 0x38 0xc1 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xc1 - invalid. */
/*  Opcode      0x0f 0x38 0xc2 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xc2 - invalid. */
/*  Opcode      0x0f 0x38 0xc3 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xc3 - invalid. */
/*  Opcode      0x0f 0x38 0xc4 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xc4 - invalid. */
/*  Opcode      0x0f 0x38 0xc5 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xc5 - invalid. */
/*  Opcode      0x0f 0x38 0xc6 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xc6 - invalid. */
/*  Opcode      0x0f 0x38 0xc7 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xc7 - invalid. */
/** Opcode      0x0f 0x38 0xc8. */
FNIEMOP_STUB(iemOp_sha1nexte_Vdq_Wdq);
/*  Opcode 0x66 0x0f 0x38 0xc8 - invalid. */
/** Opcode      0x0f 0x38 0xc9. */
FNIEMOP_STUB(iemOp_sha1msg1_Vdq_Wdq);
/*  Opcode 0x66 0x0f 0x38 0xc9 - invalid. */
/** Opcode      0x0f 0x38 0xca. */
FNIEMOP_STUB(iemOp_sha1msg2_Vdq_Wdq);
/*  Opcode 0x66 0x0f 0x38 0xca - invalid. */
/** Opcode      0x0f 0x38 0xcb. */
FNIEMOP_STUB(iemOp_sha256rnds2_Vdq_Wdq);
/*  Opcode 0x66 0x0f 0x38 0xcb - invalid. */
/** Opcode      0x0f 0x38 0xcc. */
FNIEMOP_STUB(iemOp_sha256msg1_Vdq_Wdq);
/*  Opcode 0x66 0x0f 0x38 0xcc - invalid. */
/** Opcode      0x0f 0x38 0xcd. */
FNIEMOP_STUB(iemOp_sha256msg2_Vdq_Wdq);
/*  Opcode 0x66 0x0f 0x38 0xcd - invalid. */
/*  Opcode      0x0f 0x38 0xce - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xce - invalid. */
/*  Opcode      0x0f 0x38 0xcf - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xcf - invalid. */

/*  Opcode 0x66 0x0f 0x38 0xd0 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xd1 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xd2 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xd3 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xd4 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xd5 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xd6 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xd7 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xd8 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xd9 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xda - invalid. */
/** Opcode 0x66 0x0f 0x38 0xdb. */
FNIEMOP_STUB(iemOp_aesimc_Vdq_Wdq);
/** Opcode 0x66 0x0f 0x38 0xdc. */
FNIEMOP_STUB(iemOp_aesenc_Vdq_Wdq);
/** Opcode 0x66 0x0f 0x38 0xdd. */
FNIEMOP_STUB(iemOp_aesenclast_Vdq_Wdq);
/** Opcode 0x66 0x0f 0x38 0xde. */
FNIEMOP_STUB(iemOp_aesdec_Vdq_Wdq);
/** Opcode 0x66 0x0f 0x38 0xdf. */
FNIEMOP_STUB(iemOp_aesdeclast_Vdq_Wdq);

/*  Opcode 0x66 0x0f 0x38 0xe0 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xe1 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xe2 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xe3 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xe4 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xe5 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xe6 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xe7 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xe8 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xe9 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xea - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xeb - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xec - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xed - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xee - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xef - invalid. */


/** Opcode      0x0f 0x38 0xf0. */
FNIEMOP_STUB(iemOp_movbe_Gy_My);
/** Opcode 0x66 0x0f 0x38 0xf0. */
FNIEMOP_STUB(iemOp_movbe_Gw_Mw);
/*  Opcode 0xf3 0x0f 0x38 0xf0 - invalid. */
/** Opcode 0xf2 0x0f 0x38 0xf0. */
FNIEMOP_STUB(iemOp_crc32_Gb_Eb);

/** Opcode      0x0f 0x38 0xf1. */
FNIEMOP_STUB(iemOp_movbe_My_Gy);
/** Opcode 0x66 0x0f 0x38 0xf1. */
FNIEMOP_STUB(iemOp_movbe_Mw_Gw);
/*  Opcode 0xf3 0x0f 0x38 0xf1 - invalid. */
/** Opcode 0xf2 0x0f 0x38 0xf1. */
FNIEMOP_STUB(iemOp_crc32_Gv_Ev);

/*  Opcode      0x0f 0x38 0xf2 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xf2 - invalid. */
/*  Opcode 0xf3 0x0f 0x38 0xf2 - invalid. */
/*  Opcode 0xf2 0x0f 0x38 0xf2 - invalid. */

/*  Opcode      0x0f 0x38 0xf3 - invalid (vex only - group 17). */
/*  Opcode 0x66 0x0f 0x38 0xf3 - invalid (vex only - group 17). */
/*  Opcode 0xf3 0x0f 0x38 0xf3 - invalid (vex only - group 17). */
/*  Opcode 0xf2 0x0f 0x38 0xf3 - invalid (vex only - group 17). */

/*  Opcode      0x0f 0x38 0xf4 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xf4 - invalid. */
/*  Opcode 0xf3 0x0f 0x38 0xf4 - invalid. */
/*  Opcode 0xf2 0x0f 0x38 0xf4 - invalid. */

/*  Opcode      0x0f 0x38 0xf5 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xf5 - invalid. */
/*  Opcode 0xf3 0x0f 0x38 0xf5 - invalid (vex only). */
/*  Opcode 0xf2 0x0f 0x38 0xf5 - invalid (vex only). */

/*  Opcode      0x0f 0x38 0xf6 - invalid. */
/** Opcode 0x66 0x0f 0x38 0xf6. */
FNIEMOP_STUB(iemOp_adcx_Gy_Ey);
/** Opcode 0xf3 0x0f 0x38 0xf6. */
FNIEMOP_STUB(iemOp_adox_Gy_Ey);
/*  Opcode 0xf2 0x0f 0x38 0xf6 - invalid (vex only). */

/*  Opcode      0x0f 0x38 0xf7 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x38 0xf7 - invalid (vex only). */
/*  Opcode 0xf3 0x0f 0x38 0xf7 - invalid (vex only). */
/*  Opcode 0xf2 0x0f 0x38 0xf7 - invalid (vex only). */

/*  Opcode      0x0f 0x38 0xf8 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xf8 - invalid. */
/*  Opcode 0xf3 0x0f 0x38 0xf8 - invalid. */
/*  Opcode 0xf2 0x0f 0x38 0xf8 - invalid. */

/*  Opcode      0x0f 0x38 0xf9 - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xf9 - invalid. */
/*  Opcode 0xf3 0x0f 0x38 0xf9 - invalid. */
/*  Opcode 0xf2 0x0f 0x38 0xf9 - invalid. */

/*  Opcode      0x0f 0x38 0xfa - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xfa - invalid. */
/*  Opcode 0xf3 0x0f 0x38 0xfa - invalid. */
/*  Opcode 0xf2 0x0f 0x38 0xfa - invalid. */

/*  Opcode      0x0f 0x38 0xfb - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xfb - invalid. */
/*  Opcode 0xf3 0x0f 0x38 0xfb - invalid. */
/*  Opcode 0xf2 0x0f 0x38 0xfb - invalid. */

/*  Opcode      0x0f 0x38 0xfc - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xfc - invalid. */
/*  Opcode 0xf3 0x0f 0x38 0xfc - invalid. */
/*  Opcode 0xf2 0x0f 0x38 0xfc - invalid. */

/*  Opcode      0x0f 0x38 0xfd - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xfd - invalid. */
/*  Opcode 0xf3 0x0f 0x38 0xfd - invalid. */
/*  Opcode 0xf2 0x0f 0x38 0xfd - invalid. */

/*  Opcode      0x0f 0x38 0xfe - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xfe - invalid. */
/*  Opcode 0xf3 0x0f 0x38 0xfe - invalid. */
/*  Opcode 0xf2 0x0f 0x38 0xfe - invalid. */

/*  Opcode      0x0f 0x38 0xff - invalid. */
/*  Opcode 0x66 0x0f 0x38 0xff - invalid. */
/*  Opcode 0xf3 0x0f 0x38 0xff - invalid. */
/*  Opcode 0xf2 0x0f 0x38 0xff - invalid. */


/**
 * Three byte opcode map, first two bytes are 0x0f 0x38.
 * @sa      g_apfnVexMap2
 */
IEM_STATIC const PFNIEMOP g_apfnThreeByte0f38[] =
{
    /*          no prefix,                  066h prefix                 f3h prefix,                 f2h prefix */
    /* 0x00 */  iemOp_pshufb_Pq_Qq,         iemOp_pshufb_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x01 */  iemOp_phaddw_Pq_Qq,         iemOp_phaddw_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x02 */  iemOp_phaddd_Pq_Qq,         iemOp_phaddd_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x03 */  iemOp_phaddsw_Pq_Qq,        iemOp_phaddsw_Vx_Wx,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x04 */  iemOp_pmaddubsw_Pq_Qq,      iemOp_pmaddubsw_Vx_Wx,      iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x05 */  iemOp_phsubw_Pq_Qq,         iemOp_phsubw_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x06 */  iemOp_phsubd_Pq_Qq,         iemOp_phsubdq_Vx_Wx,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x07 */  iemOp_phsubsw_Pq_Qq,        iemOp_phsubsw_Vx_Wx,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x08 */  iemOp_psignb_Pq_Qq,         iemOp_psignb_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x09 */  iemOp_psignw_Pq_Qq,         iemOp_psignw_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x0a */  iemOp_psignd_Pq_Qq,         iemOp_psignd_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x0b */  iemOp_pmulhrsw_Pq_Qq,       iemOp_pmulhrsw_Vx_Wx,       iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x0c */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x0d */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x0e */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x0f */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0x10 */  iemOp_InvalidNeedRM,        iemOp_pblendvb_Vdq_Wdq,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x11 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x12 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x13 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x14 */  iemOp_InvalidNeedRM,        iemOp_blendvps_Vdq_Wdq,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x15 */  iemOp_InvalidNeedRM,        iemOp_blendvpd_Vdq_Wdq,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x16 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x17 */  iemOp_InvalidNeedRM,        iemOp_ptest_Vx_Wx,          iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x18 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x19 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x1a */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x1b */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x1c */  iemOp_pabsb_Pq_Qq,          iemOp_pabsb_Vx_Wx,          iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x1d */  iemOp_pabsw_Pq_Qq,          iemOp_pabsw_Vx_Wx,          iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x1e */  iemOp_pabsd_Pq_Qq,          iemOp_pabsd_Vx_Wx,          iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x1f */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0x20 */  iemOp_InvalidNeedRM,        iemOp_pmovsxbw_Vx_UxMq,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x21 */  iemOp_InvalidNeedRM,        iemOp_pmovsxbd_Vx_UxMd,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x22 */  iemOp_InvalidNeedRM,        iemOp_pmovsxbq_Vx_UxMw,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x23 */  iemOp_InvalidNeedRM,        iemOp_pmovsxwd_Vx_UxMq,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x24 */  iemOp_InvalidNeedRM,        iemOp_pmovsxwq_Vx_UxMd,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x25 */  iemOp_InvalidNeedRM,        iemOp_pmovsxdq_Vx_UxMq,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x26 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x27 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x28 */  iemOp_InvalidNeedRM,        iemOp_pmuldq_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x29 */  iemOp_InvalidNeedRM,        iemOp_pcmpeqq_Vx_Wx,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x2a */  iemOp_InvalidNeedRM,        iemOp_movntdqa_Vdq_Mdq,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x2b */  iemOp_InvalidNeedRM,        iemOp_packusdw_Vx_Wx,       iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x2c */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x2d */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x2e */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x2f */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0x30 */  iemOp_InvalidNeedRM,        iemOp_pmovzxbw_Vx_UxMq,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x31 */  iemOp_InvalidNeedRM,        iemOp_pmovzxbd_Vx_UxMd,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x32 */  iemOp_InvalidNeedRM,        iemOp_pmovzxbq_Vx_UxMw,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x33 */  iemOp_InvalidNeedRM,        iemOp_pmovzxwd_Vx_UxMq,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x34 */  iemOp_InvalidNeedRM,        iemOp_pmovzxwq_Vx_UxMd,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x35 */  iemOp_InvalidNeedRM,        iemOp_pmovzxdq_Vx_UxMq,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x36 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x37 */  iemOp_InvalidNeedRM,        iemOp_pcmpgtq_Vx_Wx,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x38 */  iemOp_InvalidNeedRM,        iemOp_pminsb_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x39 */  iemOp_InvalidNeedRM,        iemOp_pminsd_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x3a */  iemOp_InvalidNeedRM,        iemOp_pminuw_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x3b */  iemOp_InvalidNeedRM,        iemOp_pminud_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x3c */  iemOp_InvalidNeedRM,        iemOp_pmaxsb_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x3d */  iemOp_InvalidNeedRM,        iemOp_pmaxsd_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x3e */  iemOp_InvalidNeedRM,        iemOp_pmaxuw_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x3f */  iemOp_InvalidNeedRM,        iemOp_pmaxud_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,

    /* 0x40 */  iemOp_InvalidNeedRM,        iemOp_pmulld_Vx_Wx,         iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x41 */  iemOp_InvalidNeedRM,        iemOp_phminposuw_Vdq_Wdq,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x42 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x43 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x44 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x45 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x46 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x47 */  IEMOP_X4(iemOp_InvalidNeedRM),
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
    /* 0x58 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x59 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x5a */  IEMOP_X4(iemOp_InvalidNeedRM),
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
    /* 0x78 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x79 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x7a */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x7b */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x7c */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x7d */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x7e */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x7f */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0x80 */  iemOp_InvalidNeedRM,        iemOp_invept_Gy_Mdq,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x81 */  iemOp_InvalidNeedRM,        iemOp_invvpid_Gy_Mdq,       iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x82 */  iemOp_InvalidNeedRM,        iemOp_invpcid_Gy_Mdq,       iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0x83 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x84 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x85 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x86 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x87 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x88 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x89 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x8a */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x8b */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x8c */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x8d */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x8e */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x8f */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0x90 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x91 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x92 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x93 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x94 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x95 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x96 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x97 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x98 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x99 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x9a */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x9b */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x9c */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x9d */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x9e */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0x9f */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0xa0 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa1 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa2 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa3 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa4 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa5 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa6 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa7 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa8 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xa9 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xaa */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xab */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xac */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xad */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xae */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xaf */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0xb0 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb1 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb2 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb3 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb4 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb5 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb6 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb7 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb8 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xb9 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xba */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xbb */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xbc */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xbd */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xbe */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xbf */  IEMOP_X4(iemOp_InvalidNeedRM),

    /* 0xc0 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc1 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc2 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc3 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc4 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc5 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc6 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc7 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xc8 */  iemOp_sha1nexte_Vdq_Wdq,    iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xc9 */  iemOp_sha1msg1_Vdq_Wdq,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xca */  iemOp_sha1msg2_Vdq_Wdq,     iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xcb */  iemOp_sha256rnds2_Vdq_Wdq,  iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xcc */  iemOp_sha256msg1_Vdq_Wdq,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xcd */  iemOp_sha256msg2_Vdq_Wdq,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
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
    /* 0xdb */  iemOp_InvalidNeedRM,        iemOp_aesimc_Vdq_Wdq,       iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xdc */  iemOp_InvalidNeedRM,        iemOp_aesenc_Vdq_Wdq,       iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xdd */  iemOp_InvalidNeedRM,        iemOp_aesenclast_Vdq_Wdq,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xde */  iemOp_InvalidNeedRM,        iemOp_aesdec_Vdq_Wdq,       iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,
    /* 0xdf */  iemOp_InvalidNeedRM,        iemOp_aesdeclast_Vdq_Wdq,   iemOp_InvalidNeedRM,        iemOp_InvalidNeedRM,

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

    /* 0xf0 */  iemOp_movbe_Gy_My,          iemOp_movbe_Gw_Mw,          iemOp_InvalidNeedRM,        iemOp_crc32_Gb_Eb,
    /* 0xf1 */  iemOp_movbe_My_Gy,          iemOp_movbe_Mw_Gw,          iemOp_InvalidNeedRM,        iemOp_crc32_Gv_Ev,
    /* 0xf2 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xf3 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xf4 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xf5 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xf6 */  iemOp_InvalidNeedRM,        iemOp_adcx_Gy_Ey,           iemOp_adox_Gy_Ey,           iemOp_InvalidNeedRM,
    /* 0xf7 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xf8 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xf9 */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xfa */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xfb */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xfc */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xfd */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xfe */  IEMOP_X4(iemOp_InvalidNeedRM),
    /* 0xff */  IEMOP_X4(iemOp_InvalidNeedRM),
};
AssertCompile(RT_ELEMENTS(g_apfnThreeByte0f38) == 1024);

/** @} */

