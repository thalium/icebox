/* $Id: IEMAllInstructionsThree0f3a.cpp.h $ */
/** @file
 * IEM - Instruction Decoding and Emulation, 0x0f 0x3a map.
 *
 * @remarks IEMAllInstructionsVexMap3.cpp.h is a VEX mirror of this file.
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


/** @name Three byte opcodes with first two bytes 0x0f 0x3a
 * @{
 */

/** Opcode 0x66 0x0f 0x00 - invalid (vex only). */
/** Opcode 0x66 0x0f 0x01 - invalid (vex only). */
/** Opcode 0x66 0x0f 0x02 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x03 - invalid */
/** Opcode 0x66 0x0f 0x04 - invalid (vex only). */
/** Opcode 0x66 0x0f 0x05 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x06 - invalid (vex only) */
/*  Opcode 0x66 0x0f 0x07 - invalid */
/** Opcode 0x66 0x0f 0x08. */
FNIEMOP_STUB(iemOp_roundps_Vx_Wx_Ib);
/** Opcode 0x66 0x0f 0x09. */
FNIEMOP_STUB(iemOp_roundpd_Vx_Wx_Ib);
/** Opcode 0x66 0x0f 0x0a. */
FNIEMOP_STUB(iemOp_roundss_Vss_Wss_Ib);
/** Opcode 0x66 0x0f 0x0b. */
FNIEMOP_STUB(iemOp_roundsd_Vsd_Wsd_Ib);
/** Opcode 0x66 0x0f 0x0c. */
FNIEMOP_STUB(iemOp_blendps_Vx_Wx_Ib);
/** Opcode 0x66 0x0f 0x0d. */
FNIEMOP_STUB(iemOp_blendpd_Vx_Wx_Ib);
/** Opcode 0x66 0x0f 0x0e. */
FNIEMOP_STUB(iemOp_blendw_Vx_Wx_Ib);
/** Opcode      0x0f 0x0f. */
FNIEMOP_STUB(iemOp_palignr_Pq_Qq_Ib);
/** Opcode 0x66 0x0f 0x0f. */
FNIEMOP_STUB(iemOp_palignr_Vx_Wx_Ib);


/*  Opcode 0x66 0x0f 0x10 - invalid */
/*  Opcode 0x66 0x0f 0x11 - invalid */
/*  Opcode 0x66 0x0f 0x12 - invalid */
/*  Opcode 0x66 0x0f 0x13 - invalid */
/** Opcode 0x66 0x0f 0x14. */
FNIEMOP_STUB(iemOp_pextrb_RdMb_Vdq_Ib);
/** Opcode 0x66 0x0f 0x15. */
FNIEMOP_STUB(iemOp_pextrw_RdMw_Vdq_Ib);
/** Opcode 0x66 0x0f 0x16. */
FNIEMOP_STUB(iemOp_pextrd_q_RdMw_Vdq_Ib);
/** Opcode 0x66 0x0f 0x17. */
FNIEMOP_STUB(iemOp_extractps_Ed_Vdq_Ib);
/*  Opcode 0x66 0x0f 0x18 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x19 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x1a - invalid */
/*  Opcode 0x66 0x0f 0x1b - invalid */
/*  Opcode 0x66 0x0f 0x1c - invalid */
/*  Opcode 0x66 0x0f 0x1d - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x1e - invalid */
/*  Opcode 0x66 0x0f 0x1f - invalid */


/** Opcode 0x66 0x0f 0x20. */
FNIEMOP_STUB(iemOp_pinsrb_Vdq_RyMb_Ib);
/** Opcode 0x66 0x0f 0x21, */
FNIEMOP_STUB(iemOp_insertps_Vdq_UdqMd_Ib);
/** Opcode 0x66 0x0f 0x22. */
FNIEMOP_STUB(iemOp_pinsrd_q_Vdq_Ey_Ib);
/*  Opcode 0x66 0x0f 0x23 - invalid */
/*  Opcode 0x66 0x0f 0x24 - invalid */
/*  Opcode 0x66 0x0f 0x25 - invalid */
/*  Opcode 0x66 0x0f 0x26 - invalid */
/*  Opcode 0x66 0x0f 0x27 - invalid */
/*  Opcode 0x66 0x0f 0x28 - invalid */
/*  Opcode 0x66 0x0f 0x29 - invalid */
/*  Opcode 0x66 0x0f 0x2a - invalid */
/*  Opcode 0x66 0x0f 0x2b - invalid */
/*  Opcode 0x66 0x0f 0x2c - invalid */
/*  Opcode 0x66 0x0f 0x2d - invalid */
/*  Opcode 0x66 0x0f 0x2e - invalid */
/*  Opcode 0x66 0x0f 0x2f - invalid */


/*  Opcode 0x66 0x0f 0x30 - invalid */
/*  Opcode 0x66 0x0f 0x31 - invalid */
/*  Opcode 0x66 0x0f 0x32 - invalid */
/*  Opcode 0x66 0x0f 0x33 - invalid */
/*  Opcode 0x66 0x0f 0x34 - invalid */
/*  Opcode 0x66 0x0f 0x35 - invalid */
/*  Opcode 0x66 0x0f 0x36 - invalid */
/*  Opcode 0x66 0x0f 0x37 - invalid */
/*  Opcode 0x66 0x0f 0x38 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x39 - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x3a - invalid */
/*  Opcode 0x66 0x0f 0x3b - invalid */
/*  Opcode 0x66 0x0f 0x3c - invalid */
/*  Opcode 0x66 0x0f 0x3d - invalid */
/*  Opcode 0x66 0x0f 0x3e - invalid */
/*  Opcode 0x66 0x0f 0x3f - invalid */


/** Opcode 0x66 0x0f 0x40. */
FNIEMOP_STUB(iemOp_dpps_Vx_Wx_Ib);
/** Opcode 0x66 0x0f 0x41, */
FNIEMOP_STUB(iemOp_dppd_Vdq_Wdq_Ib);
/** Opcode 0x66 0x0f 0x42. */
FNIEMOP_STUB(iemOp_mpsadbw_Vx_Wx_Ib);
/*  Opcode 0x66 0x0f 0x43 - invalid */
/** Opcode 0x66 0x0f 0x44. */
FNIEMOP_STUB(iemOp_pclmulqdq_Vdq_Wdq_Ib);
/*  Opcode 0x66 0x0f 0x45 - invalid */
/*  Opcode 0x66 0x0f 0x46 - invalid (vex only)  */
/*  Opcode 0x66 0x0f 0x47 - invalid */
/*  Opcode 0x66 0x0f 0x48 - invalid */
/*  Opcode 0x66 0x0f 0x49 - invalid */
/*  Opcode 0x66 0x0f 0x4a - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x4b - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x4c - invalid (vex only). */
/*  Opcode 0x66 0x0f 0x4d - invalid */
/*  Opcode 0x66 0x0f 0x4e - invalid */
/*  Opcode 0x66 0x0f 0x4f - invalid */


/*  Opcode 0x66 0x0f 0x50 - invalid */
/*  Opcode 0x66 0x0f 0x51 - invalid */
/*  Opcode 0x66 0x0f 0x52 - invalid */
/*  Opcode 0x66 0x0f 0x53 - invalid */
/*  Opcode 0x66 0x0f 0x54 - invalid */
/*  Opcode 0x66 0x0f 0x55 - invalid */
/*  Opcode 0x66 0x0f 0x56 - invalid */
/*  Opcode 0x66 0x0f 0x57 - invalid */
/*  Opcode 0x66 0x0f 0x58 - invalid */
/*  Opcode 0x66 0x0f 0x59 - invalid */
/*  Opcode 0x66 0x0f 0x5a - invalid */
/*  Opcode 0x66 0x0f 0x5b - invalid */
/*  Opcode 0x66 0x0f 0x5c - invalid */
/*  Opcode 0x66 0x0f 0x5d - invalid */
/*  Opcode 0x66 0x0f 0x5e - invalid */
/*  Opcode 0x66 0x0f 0x5f - invalid */


/** Opcode 0x66 0x0f 0x60. */
FNIEMOP_STUB(iemOp_pcmpestrm_Vdq_Wdq_Ib);
/** Opcode 0x66 0x0f 0x61, */
FNIEMOP_STUB(iemOp_pcmpestri_Vdq_Wdq_Ib);
/** Opcode 0x66 0x0f 0x62. */
FNIEMOP_STUB(iemOp_pcmpistrm_Vdq_Wdq_Ib);
/** Opcode 0x66 0x0f 0x63*/
FNIEMOP_STUB(iemOp_pcmpistri_Vdq_Wdq_Ib);
/*  Opcode 0x66 0x0f 0x64 - invalid */
/*  Opcode 0x66 0x0f 0x65 - invalid */
/*  Opcode 0x66 0x0f 0x66 - invalid */
/*  Opcode 0x66 0x0f 0x67 - invalid */
/*  Opcode 0x66 0x0f 0x68 - invalid */
/*  Opcode 0x66 0x0f 0x69 - invalid */
/*  Opcode 0x66 0x0f 0x6a - invalid */
/*  Opcode 0x66 0x0f 0x6b - invalid */
/*  Opcode 0x66 0x0f 0x6c - invalid */
/*  Opcode 0x66 0x0f 0x6d - invalid */
/*  Opcode 0x66 0x0f 0x6e - invalid */
/*  Opcode 0x66 0x0f 0x6f - invalid */

/*  Opcodes 0x0f 0x70 thru 0x0f 0xb0 are unused.  */


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
FNIEMOP_STUB(iemOp_sha1rnds4_Vdq_Wdq_Ib);
/*  Opcode      0x0f 0xcd - invalid */
/*  Opcode      0x0f 0xce - invalid */
/*  Opcode      0x0f 0xcf - invalid */


/*  Opcode 0x66 0x0f 0xd0 - invalid */
/*  Opcode 0x66 0x0f 0xd1 - invalid */
/*  Opcode 0x66 0x0f 0xd2 - invalid */
/*  Opcode 0x66 0x0f 0xd3 - invalid */
/*  Opcode 0x66 0x0f 0xd4 - invalid */
/*  Opcode 0x66 0x0f 0xd5 - invalid */
/*  Opcode 0x66 0x0f 0xd6 - invalid */
/*  Opcode 0x66 0x0f 0xd7 - invalid */
/*  Opcode 0x66 0x0f 0xd8 - invalid */
/*  Opcode 0x66 0x0f 0xd9 - invalid */
/*  Opcode 0x66 0x0f 0xda - invalid */
/*  Opcode 0x66 0x0f 0xdb - invalid */
/*  Opcode 0x66 0x0f 0xdc - invalid */
/*  Opcode 0x66 0x0f 0xdd - invalid */
/*  Opcode 0x66 0x0f 0xde - invalid */
/*  Opcode 0x66 0x0f 0xdf - (aeskeygenassist). */
FNIEMOP_STUB(iemOp_aeskeygen_Vdq_Wdq_Ib);


/*  Opcode 0xf2 0x0f 0xf0 - invalid (vex only) */


/**
 * Three byte opcode map, first two bytes are 0x0f 0x3a.
 * @sa      g_apfnVexMap2
 */
IEM_STATIC const PFNIEMOP g_apfnThreeByte0f3a[] =
{
    /*          no prefix,                  066h prefix                 f3h prefix,                 f2h prefix */
    /* 0x00 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x01 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x02 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x03 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x04 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x05 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x06 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x07 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x08 */  iemOp_InvalidNeedRMImm8,    iemOp_roundps_Vx_Wx_Ib,     iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x09 */  iemOp_InvalidNeedRMImm8,    iemOp_roundpd_Vx_Wx_Ib,     iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x0a */  iemOp_InvalidNeedRMImm8,    iemOp_roundss_Vss_Wss_Ib,   iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x0b */  iemOp_InvalidNeedRMImm8,    iemOp_roundsd_Vsd_Wsd_Ib,   iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x0c */  iemOp_InvalidNeedRMImm8,    iemOp_blendps_Vx_Wx_Ib,     iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x0d */  iemOp_InvalidNeedRMImm8,    iemOp_blendpd_Vx_Wx_Ib,     iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x0e */  iemOp_InvalidNeedRMImm8,    iemOp_blendw_Vx_Wx_Ib,      iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x0f */  iemOp_palignr_Pq_Qq_Ib,     iemOp_palignr_Vx_Wx_Ib,     iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,

    /* 0x10 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x11 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x12 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x13 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x14 */  iemOp_InvalidNeedRMImm8,    iemOp_pextrb_RdMb_Vdq_Ib,   iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x15 */  iemOp_InvalidNeedRMImm8,    iemOp_pextrw_RdMw_Vdq_Ib,   iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x16 */  iemOp_InvalidNeedRMImm8,    iemOp_pextrd_q_RdMw_Vdq_Ib, iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x17 */  iemOp_InvalidNeedRMImm8,    iemOp_extractps_Ed_Vdq_Ib,  iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x18 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x19 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x1a */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x1b */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x1c */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x1d */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x1e */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x1f */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0x20 */  iemOp_InvalidNeedRMImm8,    iemOp_pinsrb_Vdq_RyMb_Ib,   iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x21 */  iemOp_InvalidNeedRMImm8,    iemOp_insertps_Vdq_UdqMd_Ib,iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x22 */  iemOp_InvalidNeedRMImm8,    iemOp_pinsrd_q_Vdq_Ey_Ib,   iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
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
    /* 0x38 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x39 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x3a */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x3b */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x3c */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x3d */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x3e */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x3f */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0x40 */  iemOp_InvalidNeedRMImm8,    iemOp_dpps_Vx_Wx_Ib,        iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x41 */  iemOp_InvalidNeedRMImm8,    iemOp_dppd_Vdq_Wdq_Ib,      iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x42 */  iemOp_InvalidNeedRMImm8,    iemOp_mpsadbw_Vx_Wx_Ib,     iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x43 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x44 */  iemOp_InvalidNeedRMImm8,    iemOp_pclmulqdq_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x45 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x46 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x47 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x48 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x49 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x4a */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x4b */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x4c */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
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
    /* 0x5c */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x5d */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x5e */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x5f */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0x60 */  iemOp_InvalidNeedRMImm8,    iemOp_pcmpestrm_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x61 */  iemOp_InvalidNeedRMImm8,    iemOp_pcmpestri_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x62 */  iemOp_InvalidNeedRMImm8,    iemOp_pcmpistrm_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x63 */  iemOp_InvalidNeedRMImm8,    iemOp_pcmpistri_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
    /* 0x64 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x65 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x66 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x67 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x68 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x69 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x6a */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x6b */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x6c */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x6d */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x6e */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x6f */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

    /* 0x70 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x71 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x72 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x73 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x74 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x75 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x76 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x77 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x78 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x79 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x7a */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x7b */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x7c */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x7d */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x7e */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
    /* 0x7f */  IEMOP_X4(iemOp_InvalidNeedRMImm8),

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
    /* 0xcc */  iemOp_sha1rnds4_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,
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
    /* 0xdf */  iemOp_aeskeygen_Vdq_Wdq_Ib, iemOp_InvalidNeedRMImm8,   iemOp_InvalidNeedRMImm8,    iemOp_InvalidNeedRMImm8,

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

    /* 0xf0 */  IEMOP_X4(iemOp_InvalidNeedRMImm8),
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
AssertCompile(RT_ELEMENTS(g_apfnThreeByte0f3a) == 1024);

/** @} */

