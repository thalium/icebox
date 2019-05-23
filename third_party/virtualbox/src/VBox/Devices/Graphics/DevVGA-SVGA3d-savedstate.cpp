/* $Id: DevVGA-SVGA3d-savedstate.cpp $ */
/** @file
 * DevSVGA3d - VMWare SVGA device, 3D parts - Saved state and assocated stuff.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_VMSVGA
#include <VBox/vmm/pdmdev.h>
#include <VBox/err.h>
#include <VBox/log.h>

#include <iprt/assert.h>
#include <iprt/mem.h>

#include <VBox/vmm/pgm.h> /* required by DevVGA.h */
#include <VBoxVideo.h> /* required by DevVGA.h */

/* should go BEFORE any other DevVGA include to make all DevVGA.h config defines be visible */
#include "DevVGA.h"

#include "DevVGA-SVGA.h"
#include "DevVGA-SVGA3d.h"
#define VMSVGA3D_INCL_STRUCTURE_DESCRIPTORS
#include "DevVGA-SVGA3d-internal.h"



/**
 * Reinitializes an active context.
 *
 * @returns VBox status code.
 * @param   pThis               The VMSVGA device state.
 * @param   pContext            The freshly loaded context to reinitialize.
 */
static int vmsvga3dLoadReinitContext(PVGASTATE pThis, PVMSVGA3DCONTEXT pContext)
{
    int      rc;
    uint32_t cid = pContext->id;
    Assert(cid != SVGA3D_INVALID_ID);

    /* First set the render targets as they change the internal state (reset viewport etc) */
    Log(("vmsvga3dLoadReinitContext: Recreate render targets BEGIN [cid=%#x]\n", cid));
    for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aRenderTargets); j++)
    {
        if (pContext->state.aRenderTargets[j] != SVGA3D_INVALID_ID)
        {
            SVGA3dSurfaceImageId target;

            target.sid      = pContext->state.aRenderTargets[j];
            target.face     = 0;
            target.mipmap   = 0;
            rc = vmsvga3dSetRenderTarget(pThis, cid, (SVGA3dRenderTargetType)j, target);
            AssertRCReturn(rc, rc);
        }
    }
    Log(("vmsvga3dLoadReinitContext: Recreate render targets END\n"));

    /* Recreate the render state */
    Log(("vmsvga3dLoadReinitContext: Recreate render state BEGIN\n"));
    for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aRenderState); j++)
    {
        SVGA3dRenderState *pRenderState = &pContext->state.aRenderState[j];

        if (pRenderState->state != SVGA3D_RS_INVALID)
            vmsvga3dSetRenderState(pThis, pContext->id, 1, pRenderState);
    }
    Log(("vmsvga3dLoadReinitContext: Recreate render state END\n"));

    /* Recreate the texture state */
    Log(("vmsvga3dLoadReinitContext: Recreate texture state BEGIN\n"));
    for (uint32_t iStage = 0; iStage < RT_ELEMENTS(pContext->state.aTextureStates); ++iStage)
    {
        for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aTextureStates[0]); ++j)
        {
            SVGA3dTextureState *pTextureState = &pContext->state.aTextureStates[iStage][j];

            if (pTextureState->name != SVGA3D_TS_INVALID)
                vmsvga3dSetTextureState(pThis, pContext->id, 1, pTextureState);
        }
    }
    Log(("vmsvga3dLoadReinitContext: Recreate texture state END\n"));

    /* Reprogram the clip planes. */
    for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aClipPlane); j++)
    {
        if (pContext->state.aClipPlane[j].fValid == true)
            vmsvga3dSetClipPlane(pThis, cid, j, pContext->state.aClipPlane[j].plane);
    }

    /* Reprogram the light data. */
    for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aLightData); j++)
    {
        if (pContext->state.aLightData[j].fValidData == true)
            vmsvga3dSetLightData(pThis, cid, j, &pContext->state.aLightData[j].data);
        if (pContext->state.aLightData[j].fEnabled)
            vmsvga3dSetLightEnabled(pThis, cid, j, true);
    }

    /* Recreate the transform state. */
    if (pContext->state.u32UpdateFlags & VMSVGA3D_UPDATE_TRANSFORM)
    {
        for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aTransformState); j++)
        {
            if (pContext->state.aTransformState[j].fValid == true)
                vmsvga3dSetTransform(pThis, cid, (SVGA3dTransformType)j, pContext->state.aTransformState[j].matrix);
        }
    }

    /* Reprogram the material data. */
    if (pContext->state.u32UpdateFlags & VMSVGA3D_UPDATE_MATERIAL)
    {
        for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aMaterial); j++)
        {
            if (pContext->state.aMaterial[j].fValid == true)
                vmsvga3dSetMaterial(pThis, cid, (SVGA3dFace)j, &pContext->state.aMaterial[j].material);
        }
    }

    if (pContext->state.u32UpdateFlags & VMSVGA3D_UPDATE_SCISSORRECT)
        vmsvga3dSetScissorRect(pThis, cid, &pContext->state.RectScissor);
    if (pContext->state.u32UpdateFlags & VMSVGA3D_UPDATE_ZRANGE)
        vmsvga3dSetZRange(pThis, cid, pContext->state.zRange);
    if (pContext->state.u32UpdateFlags & VMSVGA3D_UPDATE_VIEWPORT)
        vmsvga3dSetViewPort(pThis, cid, &pContext->state.RectViewPort);
    if (pContext->state.u32UpdateFlags & VMSVGA3D_UPDATE_VERTEXSHADER)
        vmsvga3dShaderSet(pThis, pContext, cid, SVGA3D_SHADERTYPE_VS, pContext->state.shidVertex);
    if (pContext->state.u32UpdateFlags & VMSVGA3D_UPDATE_PIXELSHADER)
        vmsvga3dShaderSet(pThis, pContext, cid, SVGA3D_SHADERTYPE_PS, pContext->state.shidPixel);

    Log(("vmsvga3dLoadReinitContext: returns [cid=%#x]\n", cid));
    return VINF_SUCCESS;
}

int vmsvga3dLoadExec(PVGASTATE pThis, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    RT_NOREF(uPass);
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    int            rc;
    uint32_t       cContexts, cSurfaces;
    LogFlow(("vmsvga3dLoadExec:\n"));

#ifndef RT_OS_DARWIN /** @todo r=bird: this is normally done on the EMT, so for DARWIN we do that when loading saved state too now. See DevVGA-SVGA.cpp */
    /* Must initialize now as the recreation calls below rely on an initialized 3d subsystem. */
    vmsvga3dPowerOn(pThis);
#endif

    /* Get the generic 3d state first. */
    rc = SSMR3GetStructEx(pSSM, pState, sizeof(*pState), 0, g_aVMSVGA3DSTATEFields, NULL);
    AssertRCReturn(rc, rc);

    cContexts                           = pState->cContexts;
    cSurfaces                           = pState->cSurfaces;
    pState->cContexts                   = 0;
    pState->cSurfaces                   = 0;

    /* Fetch all active contexts. */
    for (uint32_t i = 0; i < cContexts; i++)
    {
        PVMSVGA3DCONTEXT pContext;
        uint32_t         cid;

        /* Get the context id */
        rc = SSMR3GetU32(pSSM, &cid);
        AssertRCReturn(rc, rc);

        if (cid != SVGA3D_INVALID_ID)
        {
            uint32_t cPixelShaderConst, cVertexShaderConst, cPixelShaders, cVertexShaders;
            LogFlow(("vmsvga3dLoadExec: Loading cid=%#x\n", cid));

#ifdef VMSVGA3D_OPENGL
            if (cid == VMSVGA3D_SHARED_CTX_ID)
            {
                i--; /* Not included in cContexts. */
                pContext = &pState->SharedCtx;
                if (pContext->id != VMSVGA3D_SHARED_CTX_ID)
                {
                    rc = vmsvga3dContextDefineOgl(pThis, VMSVGA3D_SHARED_CTX_ID, VMSVGA3D_DEF_CTX_F_SHARED_CTX);
                    AssertRCReturn(rc, rc);
                }
            }
            else
#endif
            {
                rc = vmsvga3dContextDefine(pThis, cid);
                AssertRCReturn(rc, rc);

                pContext = pState->papContexts[i];
            }
            AssertReturn(pContext->id == cid, VERR_INTERNAL_ERROR);

            rc = SSMR3GetStructEx(pSSM, pContext, sizeof(*pContext), 0, g_aVMSVGA3DCONTEXTFields, NULL);
            AssertRCReturn(rc, rc);

            cPixelShaders                       = pContext->cPixelShaders;
            cVertexShaders                      = pContext->cVertexShaders;
            cPixelShaderConst                   = pContext->state.cPixelShaderConst;
            cVertexShaderConst                  = pContext->state.cVertexShaderConst;
            pContext->cPixelShaders             = 0;
            pContext->cVertexShaders            = 0;
            pContext->state.cPixelShaderConst   = 0;
            pContext->state.cVertexShaderConst  = 0;

            /* Fetch all pixel shaders. */
            for (uint32_t j = 0; j < cPixelShaders; j++)
            {
                VMSVGA3DSHADER  shader;
                uint32_t        shid;

                /* Fetch the id first. */
                rc = SSMR3GetU32(pSSM, &shid);
                AssertRCReturn(rc, rc);

                if (shid != SVGA3D_INVALID_ID)
                {
                    uint32_t *pData;

                    /* Fetch a copy of the shader struct. */
                    rc = SSMR3GetStructEx(pSSM, &shader, sizeof(shader), 0, g_aVMSVGA3DSHADERFields, NULL);
                    AssertRCReturn(rc, rc);

                    pData = (uint32_t *)RTMemAlloc(shader.cbData);
                    AssertReturn(pData, VERR_NO_MEMORY);

                    rc = SSMR3GetMem(pSSM, pData, shader.cbData);
                    AssertRCReturn(rc, rc);

                    rc = vmsvga3dShaderDefine(pThis, cid, shid, shader.type, shader.cbData, pData);
                    AssertRCReturn(rc, rc);

                    RTMemFree(pData);
                }
            }

            /* Fetch all vertex shaders. */
            for (uint32_t j = 0; j < cVertexShaders; j++)
            {
                VMSVGA3DSHADER  shader;
                uint32_t        shid;

                /* Fetch the id first. */
                rc = SSMR3GetU32(pSSM, &shid);
                AssertRCReturn(rc, rc);

                if (shid != SVGA3D_INVALID_ID)
                {
                    uint32_t *pData;

                    /* Fetch a copy of the shader struct. */
                    rc = SSMR3GetStructEx(pSSM, &shader, sizeof(shader), 0, g_aVMSVGA3DSHADERFields, NULL);
                    AssertRCReturn(rc, rc);

                    pData = (uint32_t *)RTMemAlloc(shader.cbData);
                    AssertReturn(pData, VERR_NO_MEMORY);

                    rc = SSMR3GetMem(pSSM, pData, shader.cbData);
                    AssertRCReturn(rc, rc);

                    rc = vmsvga3dShaderDefine(pThis, cid, shid, shader.type, shader.cbData, pData);
                    AssertRCReturn(rc, rc);

                    RTMemFree(pData);
                }
            }

            /* Fetch pixel shader constants. */
            for (uint32_t j = 0; j < cPixelShaderConst; j++)
            {
                VMSVGASHADERCONST ShaderConst;

                rc = SSMR3GetStructEx(pSSM, &ShaderConst, sizeof(ShaderConst), 0, g_aVMSVGASHADERCONSTFields, NULL);
                AssertRCReturn(rc, rc);

                if (ShaderConst.fValid)
                {
                    rc = vmsvga3dShaderSetConst(pThis, cid, j, SVGA3D_SHADERTYPE_PS, ShaderConst.ctype, 1, ShaderConst.value);
                    AssertRCReturn(rc, rc);
                }
            }

            /* Fetch vertex shader constants. */
            for (uint32_t j = 0; j < cVertexShaderConst; j++)
            {
                VMSVGASHADERCONST ShaderConst;

                rc = SSMR3GetStructEx(pSSM, &ShaderConst, sizeof(ShaderConst), 0, g_aVMSVGASHADERCONSTFields, NULL);
                AssertRCReturn(rc, rc);

                if (ShaderConst.fValid)
                {
                    rc = vmsvga3dShaderSetConst(pThis, cid, j, SVGA3D_SHADERTYPE_VS, ShaderConst.ctype, 1, ShaderConst.value);
                    AssertRCReturn(rc, rc);
                }
            }

            if (uVersion >= VGA_SAVEDSTATE_VERSION_VMSVGA_TEX_STAGES)
            {
                /* Load texture stage and samplers state. */

                /* Number of stages/samplers. */
                uint32_t cStages;
                rc = SSMR3GetU32(pSSM, &cStages);
                AssertRCReturn(rc, rc);

                /* Number of states. */
                uint32_t cTextureStates;
                rc = SSMR3GetU32(pSSM, &cTextureStates);
                AssertRCReturn(rc, rc);

                for (uint32_t iStage = 0; iStage < cStages; ++iStage)
                {
                    for (uint32_t j = 0; j < cTextureStates; ++j)
                    {
                        SVGA3dTextureState textureState;
                        SSMR3GetU32(pSSM, &textureState.stage);
                        uint32_t u32Name;
                        SSMR3GetU32(pSSM, &u32Name);
                        textureState.name = (SVGA3dTextureStateName)u32Name;
                        rc = SSMR3GetU32(pSSM, &textureState.value);
                        AssertRCReturn(rc, rc);

                        if (   iStage < RT_ELEMENTS(pContext->state.aTextureStates)
                            && j < RT_ELEMENTS(pContext->state.aTextureStates[0]))
                        {
                            pContext->state.aTextureStates[iStage][j] = textureState;
                        }
                    }
                }
            }

#if 0 /** @todo */
            if (uVersion >= VGA_SAVEDSTATE_VERSION_VMSVGA_TEX_STAGES) /** @todo VGA_SAVEDSTATE_VERSION_VMSVGA_3D */
            {
                VMSVGA3DQUERY query;
                RT_ZERO(query);

                rc = SSMR3GetStructEx(pSSM, &query, sizeof(query), 0, g_aVMSVGA3DQUERYFields, NULL);
                AssertRCReturn(rc, rc);

                switch (query.enmQueryState)
                {
                    case VMSVGA3DQUERYSTATE_BUILDING:
                        /* Start collecting data. */
                        vmsvga3dQueryBegin(pThis, cid, SVGA3D_QUERYTYPE_OCCLUSION);
                        /* Partial result. */
                        pContext->occlusion.u32QueryResult = query.u32QueryResult;
                        break;

                    case VMSVGA3DQUERYSTATE_ISSUED:
                        /* Guest ended the query but did not read result. Result is restored. */
                        query.enmQueryState = VMSVGA3DQUERYSTATE_SIGNALED;
                        RT_FALL_THRU();
                    case VMSVGA3DQUERYSTATE_SIGNALED:
                        /* Create the query object. */
                        vmsvga3dOcclusionQueryCreate(pState, pContext);

                        /* Update result and state. */
                        pContext->occlusion.enmQueryState = query.enmQueryState;
                        pContext->occlusion.u32QueryResult = query.u32QueryResult;
                        break;

                    default:
                        AssertFailed();
                        RT_FALL_THRU();
                    case VMSVGA3DQUERYSTATE_NULL:
                        RT_ZERO(pContext->occlusion);
                        break;
                }
            }
#endif
        }
    }

#ifdef VMSVGA3D_OPENGL
    /* Make the shared context the current one. */
    if (pState->SharedCtx.id == VMSVGA3D_SHARED_CTX_ID)
        VMSVGA3D_SET_CURRENT_CONTEXT(pState, &pState->SharedCtx);
#endif

    /* Fetch all surfaces. */
    for (uint32_t i = 0; i < cSurfaces; i++)
    {
        uint32_t sid;

        /* Fetch the id first. */
        rc = SSMR3GetU32(pSSM, &sid);
        AssertRCReturn(rc, rc);

        if (sid != SVGA3D_INVALID_ID)
        {
            VMSVGA3DSURFACE  surface;
            LogFlow(("vmsvga3dLoadExec: Loading sid=%#x\n", sid));

            /* Fetch the surface structure first. */
            rc = SSMR3GetStructEx(pSSM, &surface, sizeof(surface), 0, g_aVMSVGA3DSURFACEFields, NULL);
            AssertRCReturn(rc, rc);

            {
                uint32_t             cMipLevels = surface.faces[0].numMipLevels * surface.cFaces;
                PVMSVGA3DMIPMAPLEVEL pMipmapLevel = (PVMSVGA3DMIPMAPLEVEL)RTMemAlloc(cMipLevels * sizeof(VMSVGA3DMIPMAPLEVEL));
                AssertReturn(pMipmapLevel, VERR_NO_MEMORY);
                SVGA3dSize *pMipmapLevelSize = (SVGA3dSize *)RTMemAlloc(cMipLevels * sizeof(SVGA3dSize));
                AssertReturn(pMipmapLevelSize, VERR_NO_MEMORY);

                /* Load the mip map level info. */
                for (uint32_t face=0; face < surface.cFaces; face++)
                {
                    for (uint32_t j = 0; j < surface.faces[0].numMipLevels; j++)
                    {
                        uint32_t idx = j + face * surface.faces[0].numMipLevels;
                        /* Load the mip map level struct. */
                        rc = SSMR3GetStructEx(pSSM, &pMipmapLevel[idx], sizeof(pMipmapLevel[idx]), 0, g_aVMSVGA3DMIPMAPLEVELFields, NULL);
                        AssertRCReturn(rc, rc);

                        pMipmapLevelSize[idx] = pMipmapLevel[idx].mipmapSize;
                    }
                }

                rc = vmsvga3dSurfaceDefine(pThis, sid, surface.surfaceFlags, surface.format, surface.faces, surface.multiSampleCount, surface.autogenFilter, cMipLevels, pMipmapLevelSize);
                AssertRCReturn(rc, rc);

                RTMemFree(pMipmapLevelSize);
                RTMemFree(pMipmapLevel);
            }

            PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];
            Assert(pSurface->id == sid);

            pSurface->fDirty = false;

            /* Load the mip map level data. */
            for (uint32_t j = 0; j < pSurface->faces[0].numMipLevels * pSurface->cFaces; j++)
            {
                PVMSVGA3DMIPMAPLEVEL pMipmapLevel = &pSurface->pMipmapLevels[j];
                bool fDataPresent = false;

                /* vmsvga3dSurfaceDefine already allocated the surface data buffer. */
                Assert(pMipmapLevel->cbSurface);
                AssertReturn(pMipmapLevel->pSurfaceData, VERR_INTERNAL_ERROR);

                /* Fetch the data present boolean first. */
                rc = SSMR3GetBool(pSSM, &fDataPresent);
                AssertRCReturn(rc, rc);

                Log(("Surface sid=%x: load mipmap level %d with %x bytes data (present=%d).\n", sid, j, pMipmapLevel->cbSurface, fDataPresent));

                if (fDataPresent)
                {
                    rc = SSMR3GetMem(pSSM, pMipmapLevel->pSurfaceData, pMipmapLevel->cbSurface);
                    AssertRCReturn(rc, rc);
                    pMipmapLevel->fDirty = true;
                    pSurface->fDirty     = true;
                }
                else
                {
                    pMipmapLevel->fDirty = false;
                }
            }
        }
    }

#ifdef VMSVGA3D_OPENGL
    /* Reinitialize the shared context. */
    LogFlow(("vmsvga3dLoadExec: pState->SharedCtx.id=%#x\n", pState->SharedCtx.id));
    if (pState->SharedCtx.id == VMSVGA3D_SHARED_CTX_ID)
    {
        rc = vmsvga3dLoadReinitContext(pThis, &pState->SharedCtx);
        AssertRCReturn(rc, rc);
    }
#endif

    /* Reinitialize all active contexts. */
    for (uint32_t i = 0; i < pState->cContexts; i++)
    {
        PVMSVGA3DCONTEXT pContext = pState->papContexts[i];
        if (pContext->id != SVGA3D_INVALID_ID)
        {
            rc = vmsvga3dLoadReinitContext(pThis, pContext);
            AssertRCReturn(rc, rc);
        }
    }

    LogFlow(("vmsvga3dLoadExec: return success\n"));
    return VINF_SUCCESS;
}


static int vmsvga3dSaveContext(PVGASTATE pThis, PSSMHANDLE pSSM, PVMSVGA3DCONTEXT pContext)
{
    RT_NOREF(pThis);
    uint32_t cid = pContext->id;

    /* Save the id first. */
    int rc = SSMR3PutU32(pSSM, cid);
    AssertRCReturn(rc, rc);

    if (cid != SVGA3D_INVALID_ID)
    {
        /* Save a copy of the context structure first. */
        rc = SSMR3PutStructEx(pSSM, pContext, sizeof(*pContext), 0, g_aVMSVGA3DCONTEXTFields, NULL);
        AssertRCReturn(rc, rc);

        /* Save all pixel shaders. */
        for (uint32_t j = 0; j < pContext->cPixelShaders; j++)
        {
            PVMSVGA3DSHADER pShader = &pContext->paPixelShader[j];

            /* Save the id first. */
            rc = SSMR3PutU32(pSSM, pShader->id);
            AssertRCReturn(rc, rc);

            if (pShader->id != SVGA3D_INVALID_ID)
            {
                uint32_t cbData = pShader->cbData;

                /* Save a copy of the shader struct. */
                rc = SSMR3PutStructEx(pSSM, pShader, sizeof(*pShader), 0, g_aVMSVGA3DSHADERFields, NULL);
                AssertRCReturn(rc, rc);

                Log(("Save pixelshader shid=%d with %x bytes code.\n", pShader->id, cbData));
                rc = SSMR3PutMem(pSSM, pShader->pShaderProgram, cbData);
                AssertRCReturn(rc, rc);
            }
        }

        /* Save all vertex shaders. */
        for (uint32_t j = 0; j < pContext->cVertexShaders; j++)
        {
            PVMSVGA3DSHADER pShader = &pContext->paVertexShader[j];

            /* Save the id first. */
            rc = SSMR3PutU32(pSSM, pShader->id);
            AssertRCReturn(rc, rc);

            if (pShader->id != SVGA3D_INVALID_ID)
            {
                uint32_t cbData = pShader->cbData;

                /* Save a copy of the shader struct. */
                rc = SSMR3PutStructEx(pSSM, pShader, sizeof(*pShader), 0, g_aVMSVGA3DSHADERFields, NULL);
                AssertRCReturn(rc, rc);

                Log(("Save vertex shader shid=%d with %x bytes code.\n", pShader->id, cbData));
                /* Fetch the shader code and save it. */
                rc = SSMR3PutMem(pSSM, pShader->pShaderProgram, cbData);
                AssertRCReturn(rc, rc);
            }
        }

        /* Save pixel shader constants. */
        for (uint32_t j = 0; j < pContext->state.cPixelShaderConst; j++)
        {
            rc = SSMR3PutStructEx(pSSM, &pContext->state.paPixelShaderConst[j], sizeof(pContext->state.paPixelShaderConst[j]), 0, g_aVMSVGASHADERCONSTFields, NULL);
            AssertRCReturn(rc, rc);
        }

        /* Save vertex shader constants. */
        for (uint32_t j = 0; j < pContext->state.cVertexShaderConst; j++)
        {
            rc = SSMR3PutStructEx(pSSM, &pContext->state.paVertexShaderConst[j], sizeof(pContext->state.paVertexShaderConst[j]), 0, g_aVMSVGASHADERCONSTFields, NULL);
            AssertRCReturn(rc, rc);
        }

        /* Save texture stage and samplers state. */

        /* Number of stages/samplers. */
        rc = SSMR3PutU32(pSSM, RT_ELEMENTS(pContext->state.aTextureStates));
        AssertRCReturn(rc, rc);

        /* Number of texture states. */
        rc = SSMR3PutU32(pSSM, RT_ELEMENTS(pContext->state.aTextureStates[0]));
        AssertRCReturn(rc, rc);

        for (uint32_t iStage = 0; iStage < RT_ELEMENTS(pContext->state.aTextureStates); ++iStage)
        {
            for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aTextureStates[0]); ++j)
            {
                SVGA3dTextureState *pTextureState = &pContext->state.aTextureStates[iStage][j];

                SSMR3PutU32(pSSM, pTextureState->stage);
                SSMR3PutU32(pSSM, pTextureState->name);
                rc = SSMR3PutU32(pSSM, pTextureState->value);
                AssertRCReturn(rc, rc);
            }
        }

#if 0 /** @todo Enable later. */
        PVMSVGA3DSTATE pState = pThis->svga.p3dState;
        /* Occlusion query. */
        if (!VMSVGA3DQUERY_EXISTS(&pContext->occlusion))
        {
            pContext->occlusion.enmQueryState = VMSVGA3DQUERYSTATE_NULL;
        }

        switch (pContext->occlusion.enmQueryState)
        {
            case VMSVGA3DQUERYSTATE_BUILDING:
                /* Stop collecting data. Fetch partial result. Save result. */
                vmsvga3dOcclusionQueryEnd(pState, pContext);
                RT_FALL_THRU();
            case VMSVGA3DQUERYSTATE_ISSUED:
                /* Fetch result. Save result. */
                pContext->occlusion.u32QueryResult = 0;
                vmsvga3dOcclusionQueryGetData(pState, pContext, &pContext->occlusion.u32QueryResult);
                RT_FALL_THRU();
            case VMSVGA3DQUERYSTATE_SIGNALED:
                /* Save result. Nothing to do here. */
                break;

            default:
                AssertFailed();
                RT_FALL_THRU();
            case VMSVGA3DQUERYSTATE_NULL:
                pContext->occlusion.enmQueryState = VMSVGA3DQUERYSTATE_NULL;
                pContext->occlusion.u32QueryResult = 0;
                break;
        }

        rc = SSMR3PutStructEx(pSSM, &pContext->occlusion, sizeof(pContext->occlusion), 0, g_aVMSVGA3DQUERYFields, NULL);
        AssertRCReturn(rc, rc);
#endif
    }

    return VINF_SUCCESS;
}

int vmsvga3dSaveExec(PVGASTATE pThis, PSSMHANDLE pSSM)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    int            rc;

    /* Save a copy of the generic 3d state first. */
    rc = SSMR3PutStructEx(pSSM, pState, sizeof(*pState), 0, g_aVMSVGA3DSTATEFields, NULL);
    AssertRCReturn(rc, rc);

#ifdef VMSVGA3D_OPENGL
    /* Save the shared context. */
    if (pState->SharedCtx.id == VMSVGA3D_SHARED_CTX_ID)
    {
        rc = vmsvga3dSaveContext(pThis, pSSM, &pState->SharedCtx);
        AssertRCReturn(rc, rc);
    }
#endif

    /* Save all active contexts. */
    for (uint32_t i = 0; i < pState->cContexts; i++)
    {
        rc = vmsvga3dSaveContext(pThis, pSSM, pState->papContexts[i]);
        AssertRCReturn(rc, rc);
    }

    /* Save all active surfaces. */
    for (uint32_t sid = 0; sid < pState->cSurfaces; sid++)
    {
        PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];

        /* Save the id first. */
        rc = SSMR3PutU32(pSSM, pSurface->id);
        AssertRCReturn(rc, rc);

        if (pSurface->id != SVGA3D_INVALID_ID)
        {
            /* Save a copy of the surface structure first. */
            rc = SSMR3PutStructEx(pSSM, pSurface, sizeof(*pSurface), 0, g_aVMSVGA3DSURFACEFields, NULL);
            AssertRCReturn(rc, rc);

            /* Save the mip map level info. */
            for (uint32_t face=0; face < pSurface->cFaces; face++)
            {
                for (uint32_t i = 0; i < pSurface->faces[0].numMipLevels; i++)
                {
                    uint32_t idx = i + face * pSurface->faces[0].numMipLevels;
                    PVMSVGA3DMIPMAPLEVEL pMipmapLevel = &pSurface->pMipmapLevels[idx];

                    /* Save a copy of the mip map level struct. */
                    rc = SSMR3PutStructEx(pSSM, pMipmapLevel, sizeof(*pMipmapLevel), 0, g_aVMSVGA3DMIPMAPLEVELFields, NULL);
                    AssertRCReturn(rc, rc);
                }
            }

            /* Save the mip map level data. */
            for (uint32_t face=0; face < pSurface->cFaces; face++)
            {
                for (uint32_t i = 0; i < pSurface->faces[0].numMipLevels; i++)
                {
                    uint32_t idx = i + face * pSurface->faces[0].numMipLevels;
                    PVMSVGA3DMIPMAPLEVEL pMipmapLevel = &pSurface->pMipmapLevels[idx];

                    Log(("Surface sid=%d: save mipmap level %d with %x bytes data.\n", sid, i, pMipmapLevel->cbSurface));

#ifdef VMSVGA3D_DIRECT3D
                    if (!pSurface->u.pSurface)
#else
                    if (pSurface->oglId.texture == OPENGL_INVALID_ID)
#endif
                    {
                        if (pMipmapLevel->fDirty)
                        {
                            /* Data follows */
                            rc = SSMR3PutBool(pSSM, true);
                            AssertRCReturn(rc, rc);

                            Assert(pMipmapLevel->cbSurface);
                            rc = SSMR3PutMem(pSSM, pMipmapLevel->pSurfaceData, pMipmapLevel->cbSurface);
                            AssertRCReturn(rc, rc);
                        }
                        else
                        {
                            /* No data follows */
                            rc = SSMR3PutBool(pSSM, false);
                            AssertRCReturn(rc, rc);
                        }
                    }
                    else
                    {
#ifdef VMSVGA3D_DIRECT3D
                        void            *pData;
                        bool             fRenderTargetTexture = false;
                        bool             fTexture = false;
                        bool             fSkipSave = false;
                        HRESULT          hr;

                        Assert(pMipmapLevel->cbSurface);
                        pData = RTMemAllocZ(pMipmapLevel->cbSurface);
                        AssertReturn(pData, VERR_NO_MEMORY);

                        switch (pSurface->enmD3DResType)
                        {
                        case VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE:
                        case VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE:
                            AssertFailed(); /// @todo
                            fSkipSave = true;
                            break;
                        case VMSVGA3D_D3DRESTYPE_SURFACE:
                        case VMSVGA3D_D3DRESTYPE_TEXTURE:
                        {
                            if (pSurface->surfaceFlags & SVGA3D_SURFACE_HINT_DEPTHSTENCIL)
                            {
                               /** @todo unable to easily fetch depth surface data in d3d 9 */
                               fSkipSave = true;
                               break;
                            }

                            fTexture = (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_TEXTURE);
                            fRenderTargetTexture = fTexture && (pSurface->surfaceFlags & SVGA3D_SURFACE_HINT_RENDERTARGET);

                            D3DLOCKED_RECT LockedRect;

                            if (fTexture)
                            {
                                if (pSurface->bounce.pTexture)
                                {
                                    if (    !pSurface->fDirty
                                        &&  fRenderTargetTexture
                                        &&  i == 0 /* only the first time */)
                                    {
                                        IDirect3DSurface9 *pSrc, *pDest;

                                        /** @todo stricter checks for associated context */
                                        uint32_t cid = pSurface->idAssociatedContext;
                                        if (    cid >= pState->cContexts
                                            ||  pState->papContexts[cid]->id != cid)
                                        {
                                            Log(("vmsvga3dSaveExec invalid context id (%x - %x)!\n", cid, (cid >= pState->cContexts) ? -1 : pState->papContexts[cid]->id));
                                            AssertFailedReturn(VERR_INVALID_PARAMETER);
                                        }
                                        PVMSVGA3DCONTEXT pContext = pState->papContexts[cid];

                                        hr = pSurface->bounce.pTexture->GetSurfaceLevel(i, &pDest);
                                        AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSaveExec: GetSurfaceLevel failed with %x\n", hr), VERR_INTERNAL_ERROR);

                                        hr = pSurface->u.pTexture->GetSurfaceLevel(i, &pSrc);
                                        AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSaveExec: GetSurfaceLevel failed with %x\n", hr), VERR_INTERNAL_ERROR);

                                        hr = pContext->pDevice->GetRenderTargetData(pSrc, pDest);
                                        AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSaveExec: GetRenderTargetData failed with %x\n", hr), VERR_INTERNAL_ERROR);

                                        pSrc->Release();
                                        pDest->Release();
                                    }

                                    hr = pSurface->bounce.pTexture->LockRect(i, /* texture level */
                                                                             &LockedRect,
                                                                             NULL,
                                                                             D3DLOCK_READONLY);
                                }
                                else
                                    hr = pSurface->u.pTexture->LockRect(i, /* texture level */
                                                                        &LockedRect,
                                                                        NULL,
                                                                        D3DLOCK_READONLY);
                            }
                            else
                                hr = pSurface->u.pSurface->LockRect(&LockedRect,
                                                                    NULL,
                                                                    D3DLOCK_READONLY);
                            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSaveExec: LockRect failed with %x\n", hr), VERR_INTERNAL_ERROR);

                            /* Copy the data one line at a time in case the internal pitch is different. */
                            for (uint32_t j = 0; j < pMipmapLevel->cBlocksY; ++j)
                            {
                                uint8_t *pu8Dst = (uint8_t *)pData + j * pMipmapLevel->cbSurfacePitch;
                                const uint8_t *pu8Src = (uint8_t *)LockedRect.pBits + j * LockedRect.Pitch;
                                memcpy(pu8Dst, pu8Src, pMipmapLevel->cbSurfacePitch);
                            }

                            if (fTexture)
                            {
                                if (pSurface->bounce.pTexture)
                                {
                                    hr = pSurface->bounce.pTexture->UnlockRect(i);
                                    AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSaveExec: UnlockRect failed with %x\n", hr), VERR_INTERNAL_ERROR);
                                }
                                else
                                    hr = pSurface->u.pTexture->UnlockRect(i);
                            }
                            else
                                hr = pSurface->u.pSurface->UnlockRect();
                            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSaveExec: UnlockRect failed with %x\n", hr), VERR_INTERNAL_ERROR);
                            break;
                        }

                        case VMSVGA3D_D3DRESTYPE_VERTEX_BUFFER:
                        case VMSVGA3D_D3DRESTYPE_INDEX_BUFFER:
                        {
                            /* Current type of the buffer. */
                            const bool fVertex = (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_VERTEX_BUFFER);

                            uint8_t *pD3DData;

                            if (fVertex)
                                hr = pSurface->u.pVertexBuffer->Lock(0, 0, (void **)&pD3DData, D3DLOCK_READONLY);
                            else
                                hr = pSurface->u.pIndexBuffer->Lock(0, 0, (void **)&pD3DData, D3DLOCK_READONLY);
                            AssertMsg(hr == D3D_OK, ("vmsvga3dSaveExec: Lock %s failed with %x\n", (fVertex) ? "vertex" : "index", hr));

                            memcpy(pData, pD3DData, pMipmapLevel->cbSurface);

                            if (fVertex)
                                hr = pSurface->u.pVertexBuffer->Unlock();
                            else
                                hr = pSurface->u.pIndexBuffer->Unlock();
                            AssertMsg(hr == D3D_OK, ("vmsvga3dSaveExec: Unlock %s failed with %x\n", (fVertex) ? "vertex" : "index", hr));
                            break;
                        }

                        default:
                            AssertFailed();
                            break;
                        }

                        if (!fSkipSave)
                        {
                            /* Data follows */
                            rc = SSMR3PutBool(pSSM, true);
                            AssertRCReturn(rc, rc);

                            /* And write the surface data. */
                            rc = SSMR3PutMem(pSSM, pData, pMipmapLevel->cbSurface);
                            AssertRCReturn(rc, rc);
                        }
                        else
                        {
                            /* No data follows */
                            rc = SSMR3PutBool(pSSM, false);
                            AssertRCReturn(rc, rc);
                        }

                        RTMemFree(pData);
#elif defined(VMSVGA3D_OPENGL)
                        void *pData = NULL;

                        PVMSVGA3DCONTEXT pContext = &pState->SharedCtx;
                        VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);

                        Assert(pMipmapLevel->cbSurface);

                        switch (pSurface->surfaceFlags & VMSVGA3D_SURFACE_HINT_SWITCH_MASK)
                        {
                        default:
                            AssertFailed();
                            RT_FALL_THRU();
                        case SVGA3D_SURFACE_HINT_DEPTHSTENCIL:
                        case SVGA3D_SURFACE_HINT_DEPTHSTENCIL | SVGA3D_SURFACE_HINT_TEXTURE:
                            /** @todo fetch data from the renderbuffer */
                            /* No data follows */
                            rc = SSMR3PutBool(pSSM, false);
                            AssertRCReturn(rc, rc);
                            break;

                        case SVGA3D_SURFACE_HINT_TEXTURE | SVGA3D_SURFACE_HINT_RENDERTARGET:
                        case SVGA3D_SURFACE_HINT_TEXTURE:
                        case SVGA3D_SURFACE_HINT_RENDERTARGET:
                        {
                            GLint activeTexture;

                            pData = RTMemAllocZ(pMipmapLevel->cbSurface);
                            AssertReturn(pData, VERR_NO_MEMORY);

                            glGetIntegerv(GL_TEXTURE_BINDING_2D, &activeTexture);
                            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

                            glBindTexture(GL_TEXTURE_2D, pSurface->oglId.texture);
                            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

                            /* Set row length and alignment of the output data. */
                            VMSVGAPACKPARAMS SavedParams;
                            vmsvga3dOglSetPackParams(pState, pContext, pSurface, &SavedParams);

                            glGetTexImage(GL_TEXTURE_2D,
                                          i,
                                          pSurface->formatGL,
                                          pSurface->typeGL,
                                          pData);
                            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

                            vmsvga3dOglRestorePackParams(pState, pContext, pSurface, &SavedParams);

                            /* Data follows */
                            rc = SSMR3PutBool(pSSM, true);
                            AssertRCReturn(rc, rc);

                            /* And write the surface data. */
                            rc = SSMR3PutMem(pSSM, pData, pMipmapLevel->cbSurface);
                            AssertRCReturn(rc, rc);

                            /* Restore the old active texture. */
                            glBindTexture(GL_TEXTURE_2D, activeTexture);
                            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);
                            break;
                        }

                        case SVGA3D_SURFACE_HINT_VERTEXBUFFER | SVGA3D_SURFACE_HINT_INDEXBUFFER:
                        case SVGA3D_SURFACE_HINT_VERTEXBUFFER:
                        case SVGA3D_SURFACE_HINT_INDEXBUFFER:
                        {
                            uint8_t *pBufferData;

                            pState->ext.glBindBuffer(GL_ARRAY_BUFFER, pSurface->oglId.buffer);
                            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

                            pBufferData = (uint8_t *)pState->ext.glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
                            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                            Assert(pBufferData);

                            /* Data follows */
                            rc = SSMR3PutBool(pSSM, true);
                            AssertRCReturn(rc, rc);

                            /* And write the surface data. */
                            rc = SSMR3PutMem(pSSM, pBufferData, pMipmapLevel->cbSurface);
                            AssertRCReturn(rc, rc);

                            pState->ext.glUnmapBuffer(GL_ARRAY_BUFFER);
                            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

                            pState->ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
                            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

                        }
                        }
                        if (pData)
                            RTMemFree(pData);
#else
#error "Unexpected 3d backend"
#endif
                    }
                }
            }
        }
    }
    return VINF_SUCCESS;
}

int vmsvga3dSaveShaderConst(PVMSVGA3DCONTEXT pContext, uint32_t reg, SVGA3dShaderType type, SVGA3dShaderConstType ctype,
                            uint32_t val1, uint32_t val2, uint32_t val3, uint32_t val4)
{
    /* Choose a sane upper limit. */
    AssertReturn(reg < _32K, VERR_INVALID_PARAMETER);

    if (type == SVGA3D_SHADERTYPE_VS)
    {
        if (pContext->state.cVertexShaderConst <= reg)
        {
            pContext->state.paVertexShaderConst = (PVMSVGASHADERCONST)RTMemRealloc(pContext->state.paVertexShaderConst, sizeof(VMSVGASHADERCONST) * (reg + 1));
            AssertReturn(pContext->state.paVertexShaderConst, VERR_NO_MEMORY);
            for (uint32_t i = pContext->state.cVertexShaderConst; i < reg + 1; i++)
                pContext->state.paVertexShaderConst[i].fValid = false;
            pContext->state.cVertexShaderConst = reg + 1;
        }

        pContext->state.paVertexShaderConst[reg].fValid   = true;
        pContext->state.paVertexShaderConst[reg].ctype    = ctype;
        pContext->state.paVertexShaderConst[reg].value[0] = val1;
        pContext->state.paVertexShaderConst[reg].value[1] = val2;
        pContext->state.paVertexShaderConst[reg].value[2] = val3;
        pContext->state.paVertexShaderConst[reg].value[3] = val4;
    }
    else
    {
        Assert(type == SVGA3D_SHADERTYPE_PS);
        if (pContext->state.cPixelShaderConst <= reg)
        {
            pContext->state.paPixelShaderConst = (PVMSVGASHADERCONST)RTMemRealloc(pContext->state.paPixelShaderConst, sizeof(VMSVGASHADERCONST) * (reg + 1));
            AssertReturn(pContext->state.paPixelShaderConst, VERR_NO_MEMORY);
            for (uint32_t i = pContext->state.cPixelShaderConst; i < reg + 1; i++)
                pContext->state.paPixelShaderConst[i].fValid = false;
            pContext->state.cPixelShaderConst = reg + 1;
        }

        pContext->state.paPixelShaderConst[reg].fValid   = true;
        pContext->state.paPixelShaderConst[reg].ctype    = ctype;
        pContext->state.paPixelShaderConst[reg].value[0] = val1;
        pContext->state.paPixelShaderConst[reg].value[1] = val2;
        pContext->state.paPixelShaderConst[reg].value[2] = val3;
        pContext->state.paPixelShaderConst[reg].value[3] = val4;
    }

    return VINF_SUCCESS;
}

