/* $Id: DevVGA-SVGA3d-hlp.cpp $ */
/** @file
 * DevVMWare - VMWare SVGA device helpers
 */

/*
 * Copyright (C) 2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_GROUP LOG_GROUP_DEV_VMSVGA
#include <VBox/AssertGuest.h>
#include <VBox/log.h>

#include <iprt/cdefs.h>
#include <iprt/err.h>
#include <iprt/types.h>

#include "vmsvga/svga3d_reg.h"
#include "vmsvga/svga3d_shaderdefs.h"

typedef struct VMSVGA3DSHADERPARSECONTEXT
{
    uint32_t type;
} VMSVGA3DSHADERPARSECONTEXT;

static int vmsvga3dShaderParseRegOffset(VMSVGA3DSHADERPARSECONTEXT *pCtx,
                                        bool fIsSrc,
                                        SVGA3dShaderRegType regType,
                                        uint32_t off)
{
    RT_NOREF(pCtx, fIsSrc);

    switch (regType)
    {
        case SVGA3DREG_TEMP:
            break;
        case SVGA3DREG_INPUT:
            break;
        case SVGA3DREG_CONST:
            break;
        case SVGA3DREG_ADDR /* also SVGA3DREG_TEXTURE */:
            break;
        case SVGA3DREG_RASTOUT:
            break;
        case SVGA3DREG_ATTROUT:
            break;
        case SVGA3DREG_TEXCRDOUT /* also SVGA3DREG_OUTPUT */:
            break;
        case SVGA3DREG_CONSTINT:
            break;
        case SVGA3DREG_COLOROUT:
            break;
        case SVGA3DREG_DEPTHOUT:
            break;
        case SVGA3DREG_SAMPLER:
            break;
        case SVGA3DREG_CONST2:
            break;
        case SVGA3DREG_CONST3:
            break;
        case SVGA3DREG_CONST4:
            break;
        case SVGA3DREG_CONSTBOOL:
            break;
        case SVGA3DREG_LOOP:
            break;
        case SVGA3DREG_TEMPFLOAT16:
            break;
        case SVGA3DREG_MISCTYPE:
            ASSERT_GUEST_RETURN(   off == SVGA3DMISCREG_POSITION
                                || off == SVGA3DMISCREG_FACE, VERR_PARSE_ERROR);
            break;
        case SVGA3DREG_LABEL:
            break;
        case SVGA3DREG_PREDICATE:
            break;
        default:
            ASSERT_GUEST_FAILED_RETURN(VERR_PARSE_ERROR);
    }

    return VINF_SUCCESS;
}

#if 0
static int vmsvga3dShaderParseSrcToken(VMSVGA3DSHADERPARSECONTEXT *pCtx, uint32_t const *pToken)
{
    RT_NOREF(pCtx);

    SVGA3dShaderSrcToken src;
    src.value = *pToken;

    SVGA3dShaderRegType const regType = (SVGA3dShaderRegType)(src.s.type_upper << 3 | src.s.type_lower);
    Log3(("Src: type %d, r0 %d, srcMod %d, swizzle 0x%x, r1 %d, relAddr %d, num %d\n",
          regType, src.s.reserved0, src.s.srcMod, src.s.swizzle, src.s.reserved1, src.s.relAddr, src.s.num));

    return vmsvga3dShaderParseRegOffset(pCtx, true, regType, src.s.num);
}
#endif

static int vmsvga3dShaderParseDestToken(VMSVGA3DSHADERPARSECONTEXT *pCtx, uint32_t const *pToken)
{
    RT_NOREF(pCtx);

    SVGA3dShaderDestToken dest;
    dest.value = *pToken;

    SVGA3dShaderRegType const regType = (SVGA3dShaderRegType)(dest.s.type_upper << 3 | dest.s.type_lower);
    Log3(("Dest: type %d, r0 %d, shfScale %d, dstMod %d, mask 0x%x, r1 %d, relAddr %d, num %d\n",
          regType, dest.s.reserved0, dest.s.shfScale, dest.s.dstMod, dest.s.mask, dest.s.reserved1, dest.s.relAddr, dest.s.num));

    return vmsvga3dShaderParseRegOffset(pCtx, false, regType, dest.s.num);
}

static int vmsvga3dShaderParseDclArgs(VMSVGA3DSHADERPARSECONTEXT *pCtx, uint32_t const *pToken)
{
    SVGA3DOpDclArgs a;
    a.values[0] = pToken[0]; // declaration
    a.values[1] = pToken[1]; // dst

    return vmsvga3dShaderParseDestToken(pCtx, (uint32_t *)&a.s2.dst);
}

/* Parse the shader code
 * https://docs.microsoft.com/en-us/windows-hardware/drivers/display/shader-code-format
 */
int vmsvga3dShaderParse(uint32_t cbShaderData, uint32_t const *pShaderData)
{
    uint32_t const *paTokensStart = (uint32_t *)pShaderData;
    uint32_t const cTokens = cbShaderData / sizeof(uint32_t);

    /* 48KB is an arbitrary limit. */
    ASSERT_GUEST_RETURN(cTokens >= 2 && cTokens < (48 * _1K) / sizeof(paTokensStart[0]), VERR_INVALID_PARAMETER);

#ifdef LOG_ENABLED
    Log3(("Shader code:\n"));
    const uint32_t cTokensPerLine = 8;
    for (uint32_t iToken = 0; iToken < cTokens; ++iToken)
    {
        if ((iToken % cTokensPerLine) == 0)
        {
            if (iToken == 0)
                Log3(("0x%08X,", paTokensStart[iToken]));
            else
                Log3(("\n0x%08X,", paTokensStart[iToken]));
        }
        else
            Log3((" 0x%08X,", paTokensStart[iToken]));
    }
    Log3(("\n"));
#endif

    /* "The first token must be a version token." */
    SVGA3dShaderVersion const *pVersion = (SVGA3dShaderVersion const *)paTokensStart;
    ASSERT_GUEST_RETURN(   pVersion->s.type == SVGA3D_VS_TYPE
                        || pVersion->s.type == SVGA3D_PS_TYPE, VERR_PARSE_ERROR);

    VMSVGA3DSHADERPARSECONTEXT ctx;
    ctx.type = pVersion->s.type;

    /* Scan the tokens. Immediately return an error code on any unexpected data. */
    const uint32_t *paTokensEnd = &paTokensStart[cTokens];
    const uint32_t *pToken = &paTokensStart[1];
    while (pToken < paTokensEnd)
    {
        SVGA3dShaderInstToken const token = *(SVGA3dShaderInstToken *)pToken;

        /* Figure out the instruction length, which is how many tokens follow the instruction token. */
        uint32_t cInstLen;
        if (token.s1.op == SVGA3DOP_COMMENT)
            cInstLen = token.s.comment_size;
        else
            cInstLen = token.s1.size;

        Log3(("op %d, cInstLen %d\n", token.s1.op, cInstLen));

        ASSERT_GUEST_RETURN(cInstLen < (uint32_t)(paTokensEnd - pToken), VERR_PARSE_ERROR);

        if (token.s1.op == SVGA3DOP_END)
        {
            ASSERT_GUEST_RETURN(token.value == 0x0000FFFF, VERR_PARSE_ERROR);
            break;
        }

        int rc;
        switch (token.s1.op)
        {
            case SVGA3DOP_DCL:
                ASSERT_GUEST_RETURN(cInstLen == 2, VERR_PARSE_ERROR);
                rc = vmsvga3dShaderParseDclArgs(&ctx, &pToken[1]);
                if (RT_FAILURE(rc))
                    return rc;
                break;
            case SVGA3DOP_NOP:
            case SVGA3DOP_MOV:
            case SVGA3DOP_ADD:
            case SVGA3DOP_SUB:
            case SVGA3DOP_MAD:
            case SVGA3DOP_MUL:
            case SVGA3DOP_RCP:
            case SVGA3DOP_RSQ:
            case SVGA3DOP_DP3:
            case SVGA3DOP_DP4:
            case SVGA3DOP_MIN:
            case SVGA3DOP_MAX:
            case SVGA3DOP_SLT:
            case SVGA3DOP_SGE:
            case SVGA3DOP_EXP:
            case SVGA3DOP_LOG:
            case SVGA3DOP_LIT:
            case SVGA3DOP_DST:
            case SVGA3DOP_LRP:
            case SVGA3DOP_FRC:
            case SVGA3DOP_M4x4:
            case SVGA3DOP_M4x3:
            case SVGA3DOP_M3x4:
            case SVGA3DOP_M3x3:
            case SVGA3DOP_M3x2:
            case SVGA3DOP_CALL:
            case SVGA3DOP_CALLNZ:
            case SVGA3DOP_LOOP:
            case SVGA3DOP_RET:
            case SVGA3DOP_ENDLOOP:
            case SVGA3DOP_LABEL:
            case SVGA3DOP_POW:
            case SVGA3DOP_CRS:
            case SVGA3DOP_SGN:
            case SVGA3DOP_ABS:
            case SVGA3DOP_NRM:
            case SVGA3DOP_SINCOS:
            case SVGA3DOP_REP:
            case SVGA3DOP_ENDREP:
            case SVGA3DOP_IF:
            case SVGA3DOP_IFC:
            case SVGA3DOP_ELSE:
            case SVGA3DOP_ENDIF:
            case SVGA3DOP_BREAK:
            case SVGA3DOP_BREAKC:
            case SVGA3DOP_MOVA:
            case SVGA3DOP_DEFB:
            case SVGA3DOP_DEFI:
            case SVGA3DOP_TEXCOORD:
            case SVGA3DOP_TEXKILL:
            case SVGA3DOP_TEX:
            case SVGA3DOP_TEXBEM:
            case SVGA3DOP_TEXBEML:
            case SVGA3DOP_TEXREG2AR:
            case SVGA3DOP_TEXREG2GB:
            case SVGA3DOP_TEXM3x2PAD:
            case SVGA3DOP_TEXM3x2TEX:
            case SVGA3DOP_TEXM3x3PAD:
            case SVGA3DOP_TEXM3x3TEX:
            case SVGA3DOP_RESERVED0:
            case SVGA3DOP_TEXM3x3SPEC:
            case SVGA3DOP_TEXM3x3VSPEC:
            case SVGA3DOP_EXPP:
            case SVGA3DOP_LOGP:
            case SVGA3DOP_CND:
            case SVGA3DOP_DEF:
            case SVGA3DOP_TEXREG2RGB:
            case SVGA3DOP_TEXDP3TEX:
            case SVGA3DOP_TEXM3x2DEPTH:
            case SVGA3DOP_TEXDP3:
            case SVGA3DOP_TEXM3x3:
            case SVGA3DOP_TEXDEPTH:
            case SVGA3DOP_CMP:
            case SVGA3DOP_BEM:
            case SVGA3DOP_DP2ADD:
            case SVGA3DOP_DSX:
            case SVGA3DOP_DSY:
            case SVGA3DOP_TEXLDD:
            case SVGA3DOP_SETP:
            case SVGA3DOP_TEXLDL:
            case SVGA3DOP_BREAKP:
            case SVGA3DOP_PHASE:
            case SVGA3DOP_COMMENT:
                break;

            default:
                ASSERT_GUEST_FAILED_RETURN(VERR_PARSE_ERROR);
        }

        /* Next token. */
        pToken += cInstLen + 1;
    }

    return VINF_SUCCESS;
}

void vmsvga3dShaderLogRel(char const *pszMsg, SVGA3dShaderType type, uint32_t cbShaderData, uint32_t const *pShaderData)
{
    /* Dump the shader code. */
    static int scLogged = 0;
    if (scLogged < 8)
    {
        ++scLogged;

        LogRel(("VMSVGA: %s shader: %s:\n", (type == SVGA3D_SHADERTYPE_VS) ? "VERTEX" : "PIXEL", pszMsg));
        const uint32_t cTokensPerLine = 8;
        const uint32_t *paTokens = (uint32_t *)pShaderData;
        const uint32_t cTokens = cbShaderData / sizeof(uint32_t);
        for (uint32_t iToken = 0; iToken < cTokens; ++iToken)
        {
            if ((iToken % cTokensPerLine) == 0)
            {
                if (iToken == 0)
                    LogRel(("0x%08X,", paTokens[iToken]));
                else
                    LogRel(("\n0x%08X,", paTokens[iToken]));
            }
            else
                LogRel((" 0x%08X,", paTokens[iToken]));
        }
        LogRel(("\n"));
    }
}
