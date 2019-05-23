/* $Id: IEMAllInstructionsVexMap3.cpp.h $ */
/** @file
 * IEM - Instruction Decoding and Emulation, 0x0f 0x3a map.
 *
 * @remarks IEMAllInstructionsThree0f3a.cpp.h is a VEX mirror of this file.
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


/** @name VEX Opcode Map 3
 * @{
 */

/** Opcode VEX.66.0F3A 0x00. */
FNIEMOP_STUB(iemOp_vpermq_Vqq_Wqq_Ib);
/** Opcode VEX.66.0F3A 0x01. */
FNIEMOP_STUB(iemOp_vpermqd_Vqq_Wqq_Ib);
/** Opcode VEX.66.0F3A 0x02. */
FNIEMOP_STUB(iemOp_vpblendd_Vx_Wx_Ib);
/*  Opcode VEX.66.0F3A 0x03 - invalid */
/** Opcode VEX.66.0F3A 0x04. */
FNIEMOP_STUB(iemOp_vpermilps_Vx_Wx_Ib);
/** Opcode VEX.66.0F3A 0x05. */
FNIEMOP_STUB(iemOp_vpermilpd_Vx_Wx_Ib);
/** Opcode VEX.66.0F3A 0x06 (vex only) */
FNIEMOP_STUB(iemOp_vperm2f128_Vqq_Hqq_Wqq_Ib);
/*  Opcode VEX.66.0F3A 0x07 - invalid */
/** Opcode VEX.66.0F3A 0x08. */
FNIEMOP_STUB(iemOp_vroundps_Vx_Wx_Ib);
/** Opcode VEX.66.0F3A 0x09. */
FNIEMOP_STUB(iemOp_vroundpd_Vx_Wx_Ib);
/** Opcode VEX.66.0F3A 0x0a. */
FNIEMOP_STUB(iemOp_vroundss_Vss_Wss_Ib);
/** Opcode VEX.66.0F3A 0x0b. */
FNIEMOP_STUB(iemOp_vroundsd_Vsd_Wsd_Ib);
/** Opcode VEX.66.0F3A 0x0c. */
FNIEMOP_STUB(iemOp_vblendps_Vx_Hx_Wx_Ib);
/** Opcode VEX.66.0F3A 0x0d. */
FNIEMOP_STUB(iemOp_vblendpd_Vx_Hx_Wx_Ib);
/** Opcode VEX.66.0F3A 0x0e. */
FNIEMOP_STUB(iemOp_vblendw_Vx_Hx_Wx_Ib);
/** Opcode VEX.0F3A 0x0f - invalid. */
/** Opcode VEX.66.0F3A 0x0f. */
FNIEMOP_STUB(iemOp_vpalignr_Vx_Hx_Wx_Ib);


/*  Opcode VEX.66.0F3A 0x10 - invalid */
/*  Opcode VEX.66.0F3A 0x11 - invalid */
/*  Opcode VEX.66.0F3A 0x12 - invalid */
/*  Opcode VEX.66.0F3A 0x13 - invalid */
/** Opcode VEX.66.0F3A 0x14. */
FNIEMOP_STUB(iemOp_vpextrb_RdMb_Vdq_Ib);
/** Opcode VEX.66.0F3A 0x15. */
FNIEMOP_STUB(iemOp_vpextrw_RdMw_Vdq_Ib);
/** Opcode VEX.66.0F3A 0x16. */
FNIEMOP_STUB(iemOp_vpextrd_q_RdMw_Vdq_Ib);
/** Opcode VEX.66.0F3A 0x17. */
FNIEMOP_STUB(iemOp_vextractps_Ed_Vdq_Ib);
/** Opcode VEX.66.0F3A 0x18 (vex only). */
FNIEMOP_STUB(iemOp_vinsertf128_Vqq_Hqq_Wqq_Ib);
/** Opcode VEX.66.0F3A 0x19 (vex only). */
FNIEMOP_STUB(iemOp_vextractf128_Wdq_Vqq_Ib);
/*  Opcode VEX.66.0F3A 0x1a - invalid */
/*  Opcode VEX.66.0F3A 0x1b - invalid */
/*  Opcode VEX.66.0F3A 0x1c - invalid */
/** Opcode VEX.66.0F3A 0x1d (vex only). */
FNIEMOP_STUB(iemOp_vcvtps2ph_Wx_Vx_Ib);
/*  Opcode VEX.66.0F3A 0x1e - invalid */
/*  Opcode VEX.66.0F3A 0x1f - invalid */


/** Opcode VEX.66.0F3A 0x20. */
FNIEMOP_STUB(iemOp_vpinsrb_Vdq_Hdq_RyMb_Ib);
/** Opcode VEX.66.0F3A 0x21, */
FNIEMOP_STUB(iemOp_vinsertps_Vdq_Hdq_UdqMd_Ib);
/** Opcode VEX.66.0F3A 0x22. */
FNIEMOP_STUB(iemOp_vpinsrd_q_Vdq_Hdq_Ey_Ib);
/*  Opcode VEX.66.0F3A 0x23 - invalid */
/*  Opcode VEX.66.0F3A 0x24 - invalid */
/*  Opcode VEX.66.0F3A 0x25 - invalid */
/*  Opcode VEX.66.0F3A 0x26 - invalid */
/*  Opcode VEX.66.0F3A 0x27 - invalid */
/*  Opcode VEX.66.0F3A 0x28 - invalid */
/*  Opcode VEX.66.0F3A 0x29 - invalid */
/*  Opcode VEX.66.0F3A 0x2a - invalid */
/*  Opcode VEX.66.0F3A 0x2b - invalid */
/*  Opcode VEX.66.0F3A 0x2c - invalid */
/*  Opcode VEX.66.0F3A 0x2d - invalid */
/*  Opcode VEX.66.0F3A 0x2e - invalid */
/*  Opcode VEX.66.0F3A 0x2f - invalid */


/*  Opcode VEX.66.0F3A 0x30 - invalid */
/*  Opcode VEX.66.0F3A 0x31 - invalid */
/*  Opcode VEX.66.0F3A 0x32 - invalid */
/*  Opcode VEX.66.0F3A 0x33 - invalid */
/*  Opcode VEX.66.0F3A 0x34 - invalid */
/*  Opcode VEX.66.0F3A 0x35 - invalid */
/*  Opcode VEX.66.0F3A 0x36 - invalid */
/*  Opcode VEX.66.0F3A 0x37 - invalid */
/** Opcode VEX.66.0F3A 0x38 (vex only). */
FNIEMOP_STUB(iemOp_vinserti128_Vqq_Hqq_Wqq_Ib);
/** Opcode VEX.66.0F3A 0x39 (vex only). */
FNIEMOP_STUB(iemOp_vextracti128_Wdq_Vqq_Ib);
/*  Opcode VEX.66.0F3A 0x3a - invalid */
/*  Opcode VEX.66.0F3A 0x3b - invalid */
/*  Opcode VEX.66.0F3A 0x3c - invalid */
/*  Opcode VEX.66.0F3A 0x3d - invalid */
/*  Opcode VEX.66.0F3A 0x3e - invalid */
/*  Opcode VEX.66.0F3A 0x3f - invalid */


/** Opcode VEX.66.0F3A 0x40. */
FNIEMOP_STUB(iemOp_vdpps_Vx_Hx_Wx_Ib);
/** Opcode VEX.66.0F3A 0x41, */
FNIEMOP_STUB(iemOp_vdppd_Vdq_Hdq_Wdq_Ib);
/** Opcode VEX.66.0F3A 0x42. */
FNIEMOP_STUB(iemOp_vmpsadbw_Vx_Hx_Wx_Ib);
/*  Opcode VEX.66.0F3A 0x43 - invalid */
/** Opcode VEX.66.0F3A 0x44. */
FNIEMOP_STUB(iemOp_vpclmulqdq_Vdq_Hdq_Wdq_Ib);
/*  Opcode VEX.66.0F3A 0x45 - invalid */
/** Opcode VEX.66.0F3A 0x46 (vex only)  */
FNIEMOP_STUB(iemOp_vperm2i128_Vqq_Hqq_Wqq_Ib);
/*  Opcode VEX.66.0F3A 0x47 - invalid */
/** Opcode VEX.66.0F3A 0x48 (AMD tables only). */
FNIEMOP_STUB(iemOp_vperlmilzz2ps_Vx_Hx_Wp_Lx);
/** Opcode VEX.66.0F3A 0x49 (AMD tables only). */
FNIEMOP_STUB(iemOp_vperlmilzz2pd_Vx_Hx_Wp_Lx);
/** Opcode VEX.66.0F3A 0x4a (vex only). */
FNIEMOP_STUB(iemOp_vblendvps_Vx_Hx_Wx_Lx);
/** Opcode VEX.66.0F3A 0x4b (vex only). */
FNIEMOP_STUB(iemOp_vblendvpd_Vx_Hx_Wx_Lx);
/** Opcode VEX.66.0F3A 0x4c (vex only). */
FNIEMOP_STUB(iemOp_vpblendvb_Vx_Hx_Wx_Lx);
/*  Opcode VEX.66.0F3A 0x4d - invalid */
/*  Opcode VEX.66.0F3A 0x4e - invalid */
/*  Opcode VEX.66.0F3A 0x4f - invalid */


/*  Opcode VEX.66.0F3A 0x50 - invalid */
/*  Opcode VEX.66.0F3A 0x51 - invalid */
/*  Opcode VEX.66.0F3A 0x52 - invalid */
/*  Opcode VEX.66.0F3A 0x53 - invalid */
/*  Opcode VEX.66.0F3A 0x54 - invalid */
/*  Opcode VEX.66.0F3A 0x55 - invalid */
/*  Opcode VEX.66.0F3A 0x56 - invalid */
/*  Opcode VEX.66.0F3A 0x57 - invalid */
/*  Opcode VEX.66.0F3A 0x58 - invalid */
/*  Opcode VEX.66.0F3A 0x59 - invalid */
/*  Opcode VEX.66.0F3A 0x5a - invalid */
/*  Opcode VEX.66.0F3A 0x5b - invalid */
/** Opcode VEX.66.0F3A 0x5c (AMD tables only). */
FNIEMOP_STUB(iemOp_vfmaddsubps_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x5d (AMD tables only). */
FNIEMOP_STUB(iemOp_vfmaddsubpd_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x5e (AMD tables only). */
FNIEMOP_STUB(iemOp_vfmsubaddps_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x5f (AMD tables only). */
FNIEMOP_STUB(iemOp_vfmsubaddpd_Vx_Lx_Wx_Hx);


/** Opcode VEX.66.0F3A 0x60. */
FNIEMOP_STUB(iemOp_vpcmpestrm_Vdq_Wdq_Ib);
/** Opcode VEX.66.0F3A 0x61, */
FNIEMOP_STUB(iemOp_vpcmpestri_Vdq_Wdq_Ib);
/** Opcode VEX.66.0F3A 0x62. */
FNIEMOP_STUB(iemOp_vpcmpistrm_Vdq_Wdq_Ib);
/** Opcode VEX.66.0F3A 0x63*/
FNIEMOP_STUB(iemOp_vpcmpistri_Vdq_Wdq_Ib);
/*  Opcode VEX.66.0F3A 0x64 - invalid */
/*  Opcode VEX.66.0F3A 0x65 - invalid */
/*  Opcode VEX.66.0F3A 0x66 - invalid */
/*  Opcode VEX.66.0F3A 0x67 - invalid */
/** Opcode VEX.66.0F3A 0x68 (AMD tables only). */
FNIEMOP_STUB(iemOp_vfmaddps_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x69 (AMD tables only). */
FNIEMOP_STUB(iemOp_vfmaddpd_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x6a (AMD tables only). */
FNIEMOP_STUB(iemOp_vfmaddss_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x6b (AMD tables only). */
FNIEMOP_STUB(iemOp_vfmaddsd_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x6c (AMD tables only). */
FNIEMOP_STUB(iemOp_vfmsubps_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x6d (AMD tables only). */
FNIEMOP_STUB(iemOp_vfmsubpd_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x6e (AMD tables only). */
FNIEMOP_STUB(iemOp_vfmsubss_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x6f (AMD tables only). */
FNIEMOP_STUB(iemOp_vfmsubsd_Vx_Lx_Wx_Hx);

/*  Opcode VEX.66.0F3A 0x70 - invalid */
/*  Opcode VEX.66.0F3A 0x71 - invalid */
/*  Opcode VEX.66.0F3A 0x72 - invalid */
/*  Opcode VEX.66.0F3A 0x73 - invalid */
/*  Opcode VEX.66.0F3A 0x74 - invalid */
/*  Opcode VEX.66.0F3A 0x75 - invalid */
/*  Opcode VEX.66.0F3A 0x76 - invalid */
/*  Opcode VEX.66.0F3A 0x77 - invalid */
/** Opcode VEX.66.0F3A 0x78 (AMD tables only). */
FNIEMOP_STUB(iemOp_vfnmaddps_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x79 (AMD tables only). */
FNIEMOP_STUB(iemOp_vfnmaddpd_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x7a (AMD tables only). */
FNIEMOP_STUB(iemOp_vfnmaddss_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x7b (AMD tables only). */
FNIEMOP_STUB(iemOp_vfnmaddsd_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x7c (AMD tables only). */
FNIEMOP_STUB(iemOp_vfnmsubps_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x7d (AMD tables only). */
FNIEMOP_STUB(iemOp_vfnmsubpd_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x7e (AMD tables only). */
FNIEMOP_STUB(iemOp_vfnmsubss_Vx_Lx_Wx_Hx);
/** Opcode VEX.66.0F3A 0x7f (AMD tables only). */
FNIEMOP_STUB(iemOp_vfnmsubsd_Vx_Lx_Wx_Hx);

/*  Opcodes 0x0f 0x80 thru 0x0f 0xb0 are unused.  */


/*  Opcode      0x0f 0xc0 - invalid */
/*  Opcode      0x0f 0xc1 - invalid */
/*  Opcode      0x0f 0xc2 - invalid */
/*  Opcode      0x0f 0xc3 - invalid */
/*  Opcode      0x0f 0xc4 - invalid */
/*  Opcode      0x0f 0xc5 - invalid */
/*  Opcode      0x0f 0xc6 - invalid */
/*  Opcode      0x0f 0xc7 - invalid */
/*  Opcode      0x0f 0xc8 - invalid */
/*  Opcode      0x0f 0xc9 - invalid */
/*  Opcode      0x0f 0xca - invalid */
/*  Opcode      0x0f 0xcb - invalid */
/*  Opcode      0x0f 0xcc */
FNIEMOP_STUB(iemOp_vsha1rnds4_Vdq_Wdq_Ib);
/*  Opcode      0x0f 0xcd - invalid */
/*  Opcode      0x0f 0xce - invalid */
/*  Opcode      0x0f 0xcf - invalid */


/*  Opcode VEX.66.0F3A 0xd0 - invalid */
/*  Opcode VEX.66.0F3A 0xd1 - invalid */
/*  Opcode VEX.66.0F3A 0xd2 - invalid */
/*  Opcode VEX.66.0F3A 0xd3 - invalid */
/*  Opcode VEX.66.0F3A 0xd4 - invalid */
/*  Opcode VEX.66.0F3A 0xd5 - invalid */
/*  Opcode VEX.66.0F3A 0xd6 - invalid */
/*  Opcode VEX.66.0F3A 0xd7 - invalid */
/*  Opcode VEX.66.0F3A 0xd8 - invalid */
/*  Opcode VEX.66.0F3A 0xd9 - invalid */
/*  Opcode VEX.66.0F3A 0xda - invalid */
/*  Opcode VEX.66.0F3A 0xdb - invalid */
/*  Opcode VEX.66.0F3A 0xdc - invalid */
/*  Opcode VEX.66.0F3A 0xdd - invalid */
/*  Opcode VEX.66.0F3A 0xde - invalid */
/*  Opcode VEX.66.0F3A 0xdf - (aeskeygenassist). */
FNIEMOP_STUB(iemOp_vaeskeygen_Vdq_Wdq_Ib);


/*  Opcode VEX.F2.0F3A (vex only) */
FNIEMOP_STUB(iemOp_rorx_Gy_Ey_Ib);


/**
 * VEX opcode map \#3.
 *
 * @sa      g_apfnThreeByte0f3a
 */
IEM_STATIC const PFNIEMOP g_apfnVexMap3[] =
{
    /*          no prefix,                  066h prefix                 f3h prefix,                 f2h prefix */
    /* 0x00 */  iemOp_InvalidNeedRMImm8,    iemOp_vpermq_Vqq_Wqq_Ib,    iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x01 */  iemOp_InvalidNeedRMImm8,    iemOp_vpermqd_Vqq_Wqq_Ib,   iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x02 */  iemOp_InvalidNeedRMImm8,    iemOp_vpblendd_Vx_Wx_Ib,    iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x03 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x04 */  iemOp_InvalidNeedRMImm8,    iemOp_vpermilps_Vx_Wx_Ib,   iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x05 */  iemOp_InvalidNeedRMImm8,    iemOp_vpermilpd_Vx_Wx_Ib,   iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x06 */  iemOp_InvalidNeedRMImm8,    iemOp_vperm2f128_Vqq_Hqq_Wqq_Ib, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x07 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x08 */  iemOp_InvalidNeedRMImm8,    iemOp_vroundps_Vx_Wx_Ib,    iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x09 */  iemOp_InvalidNeedRMImm8,    iemOp_vroundpd_Vx_Wx_Ib,    iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x0a */  iemOp_InvalidNeedRMImm8,    iemOp_vroundss_Vss_Wss_Ib,  iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x0b */  iemOp_InvalidNeedRMImm8,    iemOp_vroundsd_Vsd_Wsd_Ib,  iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x0c */  iemOp_InvalidNeedRMImm8,    iemOp_vblendps_Vx_Hx_Wx_Ib, iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x0d */  iemOp_InvalidNeedRMImm8,    iemOp_vblendpd_Vx_Hx_Wx_Ib, iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x0e */  iemOp_InvalidNeedRMImm8,    iemOp_vblendw_Vx_Hx_Wx_Ib,  iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x0f */  iemOp_InvalidNeedRMImm8,    iemOp_vpalignr_Vx_Hx_Wx_Ib, iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,

    /* 0x10 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x11 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x12 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x13 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x14 */  iemOp_InvalidNeedRMImm8,    iemOp_vpextrb_RdMb_Vdq_Ib,  iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x15 */  iemOp_InvalidNeedRMImm8,    iemOp_vpextrw_RdMw_Vdq_Ib,  iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x16 */  iemOp_InvalidNeedRMImm8,    iemOp_vpextrd_q_RdMw_Vdq_Ib, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,
    /* 0x17 */  iemOp_InvalidNeedRMImm8,    iemOp_vextractps_Ed_Vdq_Ib, iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x18 */  iemOp_InvalidNeedRMImm8,    iemOp_vinsertf128_Vqq_Hqq_Wqq_Ib, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x19 */  iemOp_InvalidNeedRMImm8,    iemOp_vextractf128_Wdq_Vqq_Ib, iemOp_InvalidNeedRMImm8,  iemOp_InvalidNeedRMImm8,
    /* 0x1a */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x1b */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x1c */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x1d */  iemOp_InvalidNeedRMImm8,    iemOp_vcvtps2ph_Wx_Vx_Ib,   iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x1e */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x1f */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0x20 */  iemOp_InvalidNeedRMImm8,    iemOp_vpinsrb_Vdq_Hdq_RyMb_Ib, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x21 */  iemOp_InvalidNeedRMImm8,    iemOp_vinsertps_Vdq_Hdq_UdqMd_Ib, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x22 */  iemOp_InvalidNeedRMImm8,    iemOp_vpinsrd_q_Vdq_Hdq_Ey_Ib, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x23 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x24 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x25 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x26 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x27 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x28 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x29 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x2a */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x2b */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x2c */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x2d */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x2e */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x2f */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0x30 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x31 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x32 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x33 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x34 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x35 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x36 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x37 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x38 */  iemOp_InvalidNeedRMImm8,    iemOp_vinserti128_Vqq_Hqq_Wqq_Ib, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x39 */  iemOp_InvalidNeedRMImm8,    iemOp_vextracti128_Wdq_Vqq_Ib, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x3a */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x3b */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x3c */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x3d */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x3e */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x3f */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0x40 */  iemOp_InvalidNeedRMImm8,    iemOp_vdpps_Vx_Hx_Wx_Ib,    iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x41 */  iemOp_InvalidNeedRMImm8,    iemOp_vdppd_Vdq_Hdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x42 */  iemOp_InvalidNeedRMImm8,    iemOp_vmpsadbw_Vx_Hx_Wx_Ib, iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x43 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x44 */  iemOp_InvalidNeedRMImm8,    iemOp_vpclmulqdq_Vdq_Hdq_Wdq_Ib, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x45 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x46 */  iemOp_InvalidNeedRMImm8,    iemOp_vperm2i128_Vqq_Hqq_Wqq_Ib, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x47 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x48 */  iemOp_InvalidNeedRMImm8,    iemOp_vperlmilzz2ps_Vx_Hx_Wp_Lx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x49 */  iemOp_InvalidNeedRMImm8,    iemOp_vperlmilzz2pd_Vx_Hx_Wp_Lx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x4a */  iemOp_InvalidNeedRMImm8,    iemOp_vblendvps_Vx_Hx_Wx_Lx, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,
    /* 0x4b */  iemOp_InvalidNeedRMImm8,    iemOp_vblendvpd_Vx_Hx_Wx_Lx, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,
    /* 0x4c */  iemOp_InvalidNeedRMImm8,    iemOp_vpblendvb_Vx_Hx_Wx_Lx, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,
    /* 0x4d */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x4e */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x4f */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0x50 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x51 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x52 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x53 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x54 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x55 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x56 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x57 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x58 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x59 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x5a */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x5b */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x5c */  iemOp_InvalidNeedRMImm8,    iemOp_vfmaddsubps_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,
    /* 0x5d */  iemOp_InvalidNeedRMImm8,    iemOp_vfmaddsubpd_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,
    /* 0x5e */  iemOp_InvalidNeedRMImm8,    iemOp_vfmsubaddps_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,
    /* 0x5f */  iemOp_InvalidNeedRMImm8,    iemOp_vfmsubaddpd_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,

    /* 0x60 */  iemOp_InvalidNeedRMImm8,    iemOp_vpcmpestrm_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,
    /* 0x61 */  iemOp_InvalidNeedRMImm8,    iemOp_vpcmpestri_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,
    /* 0x62 */  iemOp_InvalidNeedRMImm8,    iemOp_vpcmpistrm_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,
    /* 0x63 */  iemOp_InvalidNeedRMImm8,    iemOp_vpcmpistri_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,
    /* 0x64 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x65 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x66 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x67 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x68 */  iemOp_InvalidNeedRMImm8,    iemOp_vfmaddps_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x69 */  iemOp_InvalidNeedRMImm8,    iemOp_vfmaddpd_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x6a */  iemOp_InvalidNeedRMImm8,    iemOp_vfmaddss_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x6b */  iemOp_InvalidNeedRMImm8,    iemOp_vfmaddsd_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x6c */  iemOp_InvalidNeedRMImm8,    iemOp_vfmsubps_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x6d */  iemOp_InvalidNeedRMImm8,    iemOp_vfmsubpd_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x6e */  iemOp_InvalidNeedRMImm8,    iemOp_vfmsubss_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x6f */  iemOp_InvalidNeedRMImm8,    iemOp_vfmsubsd_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,

    /* 0x70 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x71 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x72 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x73 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x74 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x75 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x76 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x77 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x78 */  iemOp_InvalidNeedRMImm8,    iemOp_vfnmaddps_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x79 */  iemOp_InvalidNeedRMImm8,    iemOp_vfnmaddpd_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x7a */  iemOp_InvalidNeedRMImm8,    iemOp_vfnmaddss_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x7b */  iemOp_InvalidNeedRMImm8,    iemOp_vfnmaddsd_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x7c */  iemOp_InvalidNeedRMImm8,    iemOp_vfnmsubps_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x7d */  iemOp_InvalidNeedRMImm8,    iemOp_vfnmsubpd_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x7e */  iemOp_InvalidNeedRMImm8,    iemOp_vfnmsubss_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,
    /* 0x7f */  iemOp_InvalidNeedRMImm8,    iemOp_vfnmsubsd_Vx_Lx_Wx_Hx, iemOp_InvalidNeedRMImm8, iemOp_InvalidNeedRMImm8,

    /* 0x80 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x81 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x82 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x83 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x84 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x85 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x86 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x87 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x88 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x89 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x8a */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x8b */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x8c */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x8d */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x8e */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x8f */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0x90 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x91 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x92 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x93 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x94 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x95 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x96 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x97 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x98 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x99 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x9a */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x9b */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x9c */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x9d */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x9e */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x9f */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0xa0 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xa1 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xa2 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xa3 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xa4 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xa5 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xa6 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xa7 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xa8 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xa9 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xaa */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xab */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xac */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xad */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xae */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xaf */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0xb0 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xb1 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xb2 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xb3 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xb4 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xb5 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xb6 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xb7 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xb8 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xb9 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xba */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xbb */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xbc */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xbd */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xbe */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xbf */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0xc0 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xc1 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xc2 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xc3 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xc4 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xc5 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xc6 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xc7 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xc8 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xc9 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xca */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xcb */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xcc */  iemOp_vsha1rnds4_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,  iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0xcd */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xce */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xcf */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0xd0 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xd1 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xd2 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xd3 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xd4 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xd5 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xd6 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xd7 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xd8 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xd9 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xda */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xdb */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xdc */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xdd */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xde */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xdf */  iemOp_vaeskeygen_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,  iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,

    /* 0xe0 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xe1 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xe2 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xe3 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xe4 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xe5 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xe6 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xe7 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xe8 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xe9 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xea */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xeb */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xec */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xed */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xee */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xef */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0xf0 */  iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,    iemOp_rorx_Gy_Ey_Ib,
    /* 0xf1 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xf2 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xf3 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xf4 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xf5 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xf6 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xf7 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xf8 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xf9 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xfa */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xfb */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xfc */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xfd */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xfe */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0xff */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
};
AssertCompile(RT_ELEMENTS(g_apfnVexMap3) == 1024);

/** @} */

