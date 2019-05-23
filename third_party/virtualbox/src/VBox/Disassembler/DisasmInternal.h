/* $Id: DisasmInternal.h $ */
/** @file
 * VBox disassembler - Internal header.
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
 */

#ifndef ___DisasmInternal_h___
#define ___DisasmInternal_h___

#include <VBox/types.h>
#include <VBox/dis.h>


/** @defgroup grp_dis_int Internals.
 * @ingroup grp_dis
 * @{
 */

/** @name Index into g_apfnCalcSize and g_apfnFullDisasm.
 * @{ */
enum IDX_Parse
{
  IDX_ParseNop = 0,
  IDX_ParseModRM,
  IDX_UseModRM,
  IDX_ParseImmByte,
  IDX_ParseImmBRel,
  IDX_ParseImmUshort,
  IDX_ParseImmV,
  IDX_ParseImmVRel,
  IDX_ParseImmAddr,
  IDX_ParseFixedReg,
  IDX_ParseImmUlong,
  IDX_ParseImmQword,
  IDX_ParseTwoByteEsc,
  IDX_ParseGrp1,
  IDX_ParseShiftGrp2,
  IDX_ParseGrp3,
  IDX_ParseGrp4,
  IDX_ParseGrp5,
  IDX_Parse3DNow,
  IDX_ParseGrp6,
  IDX_ParseGrp7,
  IDX_ParseGrp8,
  IDX_ParseGrp9,
  IDX_ParseGrp10,
  IDX_ParseGrp12,
  IDX_ParseGrp13,
  IDX_ParseGrp14,
  IDX_ParseGrp15,
  IDX_ParseGrp16,
  IDX_ParseModFence,
  IDX_ParseYv,
  IDX_ParseYb,
  IDX_ParseXv,
  IDX_ParseXb,
  IDX_ParseEscFP,
  IDX_ParseNopPause,
  IDX_ParseImmByteSX,
  IDX_ParseImmZ,
  IDX_ParseThreeByteEsc4,
  IDX_ParseThreeByteEsc5,
  IDX_ParseImmAddrF,
  IDX_ParseInvOpModRM,
  IDX_ParseVex2b,
  IDX_ParseVex3b,
  IDX_ParseVexDest,
  IDX_ParseMax
};
/** @}  */


/** @name Opcode maps.
 * @{ */
extern const DISOPCODE g_InvalidOpcode[1];

extern const DISOPCODE g_aOneByteMapX86[256];
extern const DISOPCODE g_aOneByteMapX64[256];
extern const DISOPCODE g_aTwoByteMapX86[256];

/** Two byte opcode map with prefix 0x66 */
extern const DISOPCODE g_aTwoByteMapX86_PF66[256];

/** Two byte opcode map with prefix 0xF2 */
extern const DISOPCODE g_aTwoByteMapX86_PFF2[256];

/** Two byte opcode map with prefix 0xF3 */
extern const DISOPCODE g_aTwoByteMapX86_PFF3[256];

/** Three byte opcode map (0xF 0x38) */
extern PCDISOPCODE const g_apThreeByteMapX86_0F38[16];

/** Three byte opcode map with prefix 0x66 (0xF 0x38) */
extern PCDISOPCODE const g_apThreeByteMapX86_660F38[16];

/** Three byte opcode map with prefix 0xF2 (0xF 0x38) */
extern PCDISOPCODE const g_apThreeByteMapX86_F20F38[16];

/** Three byte opcode map with prefix 0xF3 (0xF 0x38) */
extern PCDISOPCODE const g_apThreeByteMapX86_F30F38[16];

extern PCDISOPCODE const g_apThreeByteMapX86_0F3A[16];

/** Three byte opcode map with prefix 0x66 (0xF 0x3A) */
extern PCDISOPCODE const g_apThreeByteMapX86_660F3A[16];

/** Three byte opcode map with prefixes 0x66 0xF2 (0xF 0x38) */
extern PCDISOPCODE const g_apThreeByteMapX86_66F20F38[16];

/** VEX opcodes table defined by [VEX.m-mmmm - 1].
  * 0Fh, 0F38h, 0F3Ah correspondingly, VEX.pp = 00b */
extern PCDISOPCODE const g_aVexOpcodesMap[3];

/** VEX opcodes table defined by [VEX.m-mmmm - 1].
  * 0Fh, 0F38h, 0F3Ah correspondingly, VEX.pp = 01b (66h) */
extern PCDISOPCODE const g_aVexOpcodesMap_66H[3];

/** 0Fh, 0F38h, 0F3Ah correspondingly, VEX.pp = 10b (F3h) */
extern PCDISOPCODE const g_aVexOpcodesMap_F3H[3];

/** 0Fh, 0F38h, 0F3Ah correspondingly, VEX.pp = 11b (F2h) */
extern PCDISOPCODE const g_aVexOpcodesMap_F2H[3];
/** @} */

/** @name Opcode extensions (Group tables)
 * @{ */
extern const DISOPCODE g_aMapX86_Group1[8*4];
extern const DISOPCODE g_aMapX86_Group2[8*6];
extern const DISOPCODE g_aMapX86_Group3[8*2];
extern const DISOPCODE g_aMapX86_Group4[8];
extern const DISOPCODE g_aMapX86_Group5[8];
extern const DISOPCODE g_aMapX86_Group6[8];
extern const DISOPCODE g_aMapX86_Group7_mem[8];
extern const DISOPCODE g_aMapX86_Group7_mod11_rm000[8];
extern const DISOPCODE g_aMapX86_Group7_mod11_rm001[8];
extern const DISOPCODE g_aMapX86_Group8[8];
extern const DISOPCODE g_aMapX86_Group9[8];
extern const DISOPCODE g_aMapX86_Group10[8];
extern const DISOPCODE g_aMapX86_Group11[8*2];
extern const DISOPCODE g_aMapX86_Group12[8*2];
extern const DISOPCODE g_aMapX86_Group13[8*2];
extern const DISOPCODE g_aMapX86_Group14[8*2];
extern const DISOPCODE g_aMapX86_Group15_mem[8];
extern const DISOPCODE g_aMapX86_Group15_mod11_rm000[8];
extern const DISOPCODE g_aMapX86_Group16[8];
extern const DISOPCODE g_aMapX86_NopPause[2];
/** @} */

/** 3DNow! map (0x0F 0x0F prefix) */
extern const DISOPCODE g_aTwoByteMapX86_3DNow[256];

/** Floating point opcodes starting with escape byte 0xDF
 * @{ */
extern const DISOPCODE g_aMapX86_EscF0_Low[8];
extern const DISOPCODE g_aMapX86_EscF0_High[16*4];
extern const DISOPCODE g_aMapX86_EscF1_Low[8];
extern const DISOPCODE g_aMapX86_EscF1_High[16*4];
extern const DISOPCODE g_aMapX86_EscF2_Low[8];
extern const DISOPCODE g_aMapX86_EscF2_High[16*4];
extern const DISOPCODE g_aMapX86_EscF3_Low[8];
extern const DISOPCODE g_aMapX86_EscF3_High[16*4];
extern const DISOPCODE g_aMapX86_EscF4_Low[8];
extern const DISOPCODE g_aMapX86_EscF4_High[16*4];
extern const DISOPCODE g_aMapX86_EscF5_Low[8];
extern const DISOPCODE g_aMapX86_EscF5_High[16*4];
extern const DISOPCODE g_aMapX86_EscF6_Low[8];
extern const DISOPCODE g_aMapX86_EscF6_High[16*4];
extern const DISOPCODE g_aMapX86_EscF7_Low[8];
extern const DISOPCODE g_aMapX86_EscF7_High[16*4];

extern const PCDISOPCODE g_apMapX86_FP_Low[8];
extern const PCDISOPCODE g_apMapX86_FP_High[8];
/** @} */

/** @def OP
 * Wrapper which initializes an DISOPCODE.
 * We must use this so that we can exclude unused fields in order
 * to save precious bytes in the GC version.
 *
 * @internal
 */
#ifndef DIS_CORE_ONLY
# define OP(pszOpcode, idxParse1, idxParse2, idxParse3, opcode, param1, param2, param3, optype) \
    { pszOpcode, idxParse1, idxParse2, idxParse3, 0, opcode, param1, param2, param3, 0, 0, optype }
# define OPVEX(pszOpcode, idxParse1, idxParse2, idxParse3, idxParse4, opcode, param1, param2, param3, param4, optype) \
    { pszOpcode, idxParse1, idxParse2, idxParse3, idxParse4, opcode, param1, param2, param3, param4, 0, optype | DISOPTYPE_SSE }
#else
# define OP(pszOpcode, idxParse1, idxParse2, idxParse3, opcode, param1, param2, param3, optype) \
    { idxParse1, idxParse2, idxParse3, 0, opcode, param1, param2, param3, 0, 0, optype }
# define OPVEX(pszOpcode, idxParse1, idxParse2, idxParse3, idxParse4, opcode, param1, param2, param3, param4, optype) \
    { idxParse1, idxParse2, idxParse3, idxParse4, opcode, param1, param2, param3, param4, 0, optype | DISOPTYPE_SSE}
#endif


size_t disFormatBytes(PCDISSTATE pDis, char *pszDst, size_t cchDst, uint32_t fFlags);

/** @} */
#endif

