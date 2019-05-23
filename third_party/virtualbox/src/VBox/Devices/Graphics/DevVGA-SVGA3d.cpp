/* $Id: DevVGA-SVGA3d.cpp $ */
/** @file
 * DevSVGA3d - VMWare SVGA device, 3D parts - Common core code.
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
 * Implements the SVGA_3D_CMD_SURFACE_DEFINE_V2 and SVGA_3D_CMD_SURFACE_DEFINE
 * commands (fifo).
 *
 * @returns VBox status code (currently ignored).
 * @param   pThis               The VGA device instance data.
 * @param   sid                 The ID of the surface to (re-)define.
 * @param   surfaceFlags        .
 * @param   format              .
 * @param   face                .
 * @param   multisampleCount    .
 * @param   autogenFilter       .
 * @param   cMipLevels          .
 * @param   paMipLevelSizes     .
 */
int vmsvga3dSurfaceDefine(PVGASTATE pThis, uint32_t sid, uint32_t surfaceFlags, SVGA3dSurfaceFormat format,
                          SVGA3dSurfaceFace face[SVGA3D_MAX_SURFACE_FACES], uint32_t multisampleCount,
                          SVGA3dTextureFilter autogenFilter, uint32_t cMipLevels, SVGA3dSize *paMipLevelSizes)
{
    PVMSVGA3DSURFACE pSurface;
    PVMSVGA3DSTATE   pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSurfaceDefine: sid=%x surfaceFlags=%x format=%s (%x) multiSampleCount=%d autogenFilter=%d, cMipLevels=%d size=(%d,%d,%d)\n",
         sid, surfaceFlags, vmsvgaLookupEnum((int)format, &g_SVGA3dSurfaceFormat2String), format, multisampleCount, autogenFilter,
         cMipLevels, paMipLevelSizes->width, paMipLevelSizes->height, paMipLevelSizes->depth));

    AssertReturn(sid < SVGA3D_MAX_SURFACE_IDS, VERR_INVALID_PARAMETER);
    AssertReturn(cMipLevels >= 1, VERR_INVALID_PARAMETER);

    /* Number of faces (cFaces) is specified as the number of the first non-zero elements in the 'face' array.
     * Since only plain surfaces (cFaces == 1) and cubemaps (cFaces == 6) are supported
     * (see also SVGA3dCmdDefineSurface definition in svga3d_reg.h), we ignore anything else.
     */
    uint32_t cRemainingMipLevels = cMipLevels;
    uint32_t cFaces = 0;
    for (uint32_t i = 0; i < SVGA3D_MAX_SURFACE_FACES; ++i)
    {
        if (face[i].numMipLevels == 0)
            break;

        /* All SVGA3dSurfaceFace structures must have the same value of numMipLevels field */
        AssertReturn(face[i].numMipLevels == face[0].numMipLevels, VERR_INVALID_PARAMETER);

        /* numMipLevels value can't be greater than the number of remaining elements in the paMipLevelSizes array. */
        AssertReturn(face[i].numMipLevels <= cRemainingMipLevels, VERR_INVALID_PARAMETER);
        cRemainingMipLevels -= face[i].numMipLevels;

        ++cFaces;
    }
    for (uint32_t i = cFaces; i < SVGA3D_MAX_SURFACE_FACES; ++i)
        AssertReturn(face[i].numMipLevels == 0, VERR_INVALID_PARAMETER);

    /* cFaces must be 6 for a cubemap and 1 otherwise. */
    AssertReturn(cFaces == (uint32_t)((surfaceFlags & SVGA3D_SURFACE_CUBEMAP) ? 6 : 1), VERR_INVALID_PARAMETER);

    /* Sum of face[i].numMipLevels must be equal to cMipLevels. */
    AssertReturn(cRemainingMipLevels == 0, VERR_INVALID_PARAMETER);

    if (sid >= pState->cSurfaces)
    {
        /* Grow the array. */
        uint32_t cNew = RT_ALIGN(sid + 15, 16);
        void *pvNew = RTMemRealloc(pState->papSurfaces, sizeof(pState->papSurfaces[0]) * cNew);
        AssertReturn(pvNew, VERR_NO_MEMORY);
        pState->papSurfaces = (PVMSVGA3DSURFACE *)pvNew;
        while (pState->cSurfaces < cNew)
        {
            pSurface = (PVMSVGA3DSURFACE)RTMemAllocZ(sizeof(*pSurface));
            AssertReturn(pSurface, VERR_NO_MEMORY);
            pSurface->id = SVGA3D_INVALID_ID;
            pState->papSurfaces[pState->cSurfaces++] = pSurface;
        }
    }
    pSurface = pState->papSurfaces[sid];

    /* If one already exists with this id, then destroy it now. */
    if (pSurface->id != SVGA3D_INVALID_ID)
        vmsvga3dSurfaceDestroy(pThis, sid);

    RT_ZERO(*pSurface);
    pSurface->id                    = sid;
#ifdef VMSVGA3D_OPENGL
    pSurface->idWeakContextAssociation = SVGA3D_INVALID_ID;
    pSurface->oglId.buffer          = OPENGL_INVALID_ID;
#else /* VMSVGA3D_DIRECT3D */
    pSurface->idAssociatedContext   = SVGA3D_INVALID_ID;
    pSurface->hSharedObject         = NULL;
    pSurface->pSharedObjectTree     = NULL;
#endif

    /* The surface type is sort of undefined now, even though the hints and format can help to clear that up.
     * In some case we'll have to wait until the surface is used to create the D3D object.
     */
    switch (format)
    {
    case SVGA3D_Z_D32:
    case SVGA3D_Z_D16:
    case SVGA3D_Z_D24S8:
    case SVGA3D_Z_D15S1:
    case SVGA3D_Z_D24X8:
    case SVGA3D_Z_DF16:
    case SVGA3D_Z_DF24:
    case SVGA3D_Z_D24S8_INT:
        surfaceFlags |= SVGA3D_SURFACE_HINT_DEPTHSTENCIL;
        break;

    /* Texture compression formats */
    case SVGA3D_DXT1:
    case SVGA3D_DXT2:
    case SVGA3D_DXT3:
    case SVGA3D_DXT4:
    case SVGA3D_DXT5:
    /* Bump-map formats */
    case SVGA3D_BUMPU8V8:
    case SVGA3D_BUMPL6V5U5:
    case SVGA3D_BUMPX8L8V8U8:
    case SVGA3D_BUMPL8V8U8:
    case SVGA3D_V8U8:
    case SVGA3D_Q8W8V8U8:
    case SVGA3D_CxV8U8:
    case SVGA3D_X8L8V8U8:
    case SVGA3D_A2W10V10U10:
    case SVGA3D_V16U16:
    /* Typical render target formats; we should allow render target buffers to be used as textures. */
    case SVGA3D_X8R8G8B8:
    case SVGA3D_A8R8G8B8:
    case SVGA3D_R5G6B5:
    case SVGA3D_X1R5G5B5:
    case SVGA3D_A1R5G5B5:
    case SVGA3D_A4R4G4B4:
        surfaceFlags |= SVGA3D_SURFACE_HINT_TEXTURE;
        break;

    case SVGA3D_LUMINANCE8:
    case SVGA3D_LUMINANCE4_ALPHA4:
    case SVGA3D_LUMINANCE16:
    case SVGA3D_LUMINANCE8_ALPHA8:
    case SVGA3D_ARGB_S10E5:   /* 16-bit floating-point ARGB */
    case SVGA3D_ARGB_S23E8:   /* 32-bit floating-point ARGB */
    case SVGA3D_A2R10G10B10:
    case SVGA3D_ALPHA8:
    case SVGA3D_R_S10E5:
    case SVGA3D_R_S23E8:
    case SVGA3D_RG_S10E5:
    case SVGA3D_RG_S23E8:
    case SVGA3D_G16R16:
    case SVGA3D_A16B16G16R16:
    case SVGA3D_UYVY:
    case SVGA3D_YUY2:
    case SVGA3D_NV12:
    case SVGA3D_AYUV:
    case SVGA3D_BC4_UNORM:
    case SVGA3D_BC5_UNORM:
        break;

    /*
     * Any surface can be used as a buffer object, but SVGA3D_BUFFER is
     * the most efficient format to use when creating new surfaces
     * expressly for index or vertex data.
     */
    case SVGA3D_BUFFER:
        break;

    default:
        break;
    }

    pSurface->surfaceFlags      = surfaceFlags;
    pSurface->format            = format;
    memcpy(pSurface->faces, face, sizeof(pSurface->faces));
    pSurface->cFaces            = cFaces;
    pSurface->multiSampleCount  = multisampleCount;
    pSurface->autogenFilter     = autogenFilter;
    Assert(autogenFilter != SVGA3D_TEX_FILTER_FLATCUBIC);
    Assert(autogenFilter != SVGA3D_TEX_FILTER_GAUSSIANCUBIC);
    pSurface->cMipmapLevels     = cMipLevels;
    pSurface->pMipmapLevels     = (PVMSVGA3DMIPMAPLEVEL)RTMemAllocZ(cMipLevels * sizeof(VMSVGA3DMIPMAPLEVEL));
    AssertReturn(pSurface->pMipmapLevels, VERR_NO_MEMORY);

    for (uint32_t i=0; i < cMipLevels; i++)
        pSurface->pMipmapLevels[i].mipmapSize = paMipLevelSizes[i];

    pSurface->cbBlock = vmsvga3dSurfaceFormatSize(format, &pSurface->cxBlock, &pSurface->cyBlock);
    AssertReturn(pSurface->cbBlock, VERR_INVALID_PARAMETER);

#ifdef VMSVGA3D_DIRECT3D
    /* Translate the format and usage flags to D3D. */
    pSurface->formatD3D         = vmsvga3dSurfaceFormat2D3D(format);
    pSurface->multiSampleTypeD3D= vmsvga3dMultipeSampleCount2D3D(multisampleCount);
    pSurface->fUsageD3D         = 0;
    if (surfaceFlags & SVGA3D_SURFACE_HINT_DYNAMIC)
        pSurface->fUsageD3D |= D3DUSAGE_DYNAMIC;
    if (surfaceFlags & SVGA3D_SURFACE_HINT_RENDERTARGET)
        pSurface->fUsageD3D |= D3DUSAGE_RENDERTARGET;
    if (surfaceFlags & SVGA3D_SURFACE_HINT_DEPTHSTENCIL)
        pSurface->fUsageD3D |= D3DUSAGE_DEPTHSTENCIL;
    if (surfaceFlags & SVGA3D_SURFACE_HINT_WRITEONLY)
        pSurface->fUsageD3D |= D3DUSAGE_WRITEONLY;
    if (surfaceFlags & SVGA3D_SURFACE_AUTOGENMIPMAPS)
        pSurface->fUsageD3D |= D3DUSAGE_AUTOGENMIPMAP;
    pSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_NONE;
    /* pSurface->u.pSurface = NULL; */
    /* pSurface->bounce.pTexture = NULL; */
#else
    vmsvga3dSurfaceFormat2OGL(pSurface, format);
#endif

    LogFunc(("surface hint(s):%s%s%s%s%s%s\n",
            (surfaceFlags & SVGA3D_SURFACE_HINT_INDEXBUFFER)  ? " SVGA3D_SURFACE_HINT_INDEXBUFFER"  : "",
            (surfaceFlags & SVGA3D_SURFACE_HINT_VERTEXBUFFER) ? " SVGA3D_SURFACE_HINT_VERTEXBUFFER"  : "",
            (surfaceFlags & SVGA3D_SURFACE_HINT_TEXTURE)      ? " SVGA3D_SURFACE_HINT_TEXTURE"  : "",
            (surfaceFlags & SVGA3D_SURFACE_HINT_DEPTHSTENCIL) ? " SVGA3D_SURFACE_HINT_DEPTHSTENCIL"  : "",
            (surfaceFlags & SVGA3D_SURFACE_HINT_RENDERTARGET) ? " SVGA3D_SURFACE_HINT_RENDERTARGET"  : "",
            (surfaceFlags & SVGA3D_SURFACE_CUBEMAP)           ? " SVGA3D_SURFACE_CUBEMAP"  : ""
           ));

    Assert(!VMSVGA3DSURFACE_HAS_HW_SURFACE(pSurface));

    /* Allocate buffer to hold the surface data until we can move it into a D3D object */
    uint32_t cbMemRemaining = SVGA3D_MAX_SURFACE_MEM_SIZE; /* Do not allow more than this for a surface. */
    for (uint32_t i = 0; i < cMipLevels; ++i)
    {
        PVMSVGA3DMIPMAPLEVEL pMipmapLevel = &pSurface->pMipmapLevels[i];
        LogFunc(("[%d] face %d mip level %d (%d,%d,%d) cbBlock=0x%x block %dx%d\n",
                 i, i / pSurface->faces[0].numMipLevels, i % pSurface->faces[0].numMipLevels,
                 pMipmapLevel->mipmapSize.width, pMipmapLevel->mipmapSize.height, pMipmapLevel->mipmapSize.depth,
                 pSurface->cbBlock, pSurface->cxBlock, pSurface->cyBlock));

        uint32_t cBlocksX;
        uint32_t cBlocksY;
        if (RT_LIKELY(pSurface->cxBlock == 1 && pSurface->cyBlock == 1))
        {
            cBlocksX = pMipmapLevel->mipmapSize.width;
            cBlocksY = pMipmapLevel->mipmapSize.height;
        }
        else
        {
            cBlocksX = pMipmapLevel->mipmapSize.width / pSurface->cxBlock;
            if (pMipmapLevel->mipmapSize.width % pSurface->cxBlock)
                ++cBlocksX;
            cBlocksY = pMipmapLevel->mipmapSize.height / pSurface->cyBlock;
            if (pMipmapLevel->mipmapSize.height % pSurface->cyBlock)
                ++cBlocksY;
        }

        if (   cBlocksX == 0
            || cBlocksY == 0
            || pMipmapLevel->mipmapSize.depth == 0)
            return VERR_INVALID_PARAMETER;

        const uint32_t cMaxBlocksX = cbMemRemaining / pSurface->cbBlock;
        if (cBlocksX > cMaxBlocksX)
            return VERR_INVALID_PARAMETER;
        const uint32_t cbSurfacePitch = pSurface->cbBlock * cBlocksX;
        LogFunc(("cbSurfacePitch=0x%x\n", cbSurfacePitch));

        const uint32_t cMaxBlocksY = cbMemRemaining / cbSurfacePitch;
        if (cBlocksY > cMaxBlocksY)
            return VERR_INVALID_PARAMETER;
        const uint32_t cbSurfacePlane = cbSurfacePitch * cBlocksY;

        const uint32_t cMaxDepth = cbMemRemaining / cbSurfacePlane;
        if (pMipmapLevel->mipmapSize.depth > cMaxDepth)
            return VERR_INVALID_PARAMETER;
        const uint32_t cbSurface = cbSurfacePlane * pMipmapLevel->mipmapSize.depth;

        pMipmapLevel->cBlocksX       = cBlocksX;
        pMipmapLevel->cBlocksY       = cBlocksY;
        pMipmapLevel->cbSurfacePitch = cbSurfacePitch;
        pMipmapLevel->cbSurfacePlane = cbSurfacePlane;
        pMipmapLevel->cbSurface      = cbSurface;
        pMipmapLevel->pSurfaceData   = RTMemAllocZ(cbSurface);
        AssertReturn(pMipmapLevel->pSurfaceData, VERR_NO_MEMORY);

        cbMemRemaining -= cbSurface;
    }
    return VINF_SUCCESS;
}


/**
 * Implements the SVGA_3D_CMD_SURFACE_DESTROY command (fifo).
 *
 * @returns VBox status code (currently ignored).
 * @param   pThis               The VGA device instance data.
 * @param   sid                 The ID of the surface to destroy.
 */
int vmsvga3dSurfaceDestroy(PVGASTATE pThis, uint32_t sid)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    PVMSVGA3DSURFACE pSurface;
    int rc = vmsvga3dSurfaceFromSid(pState, sid, &pSurface);
    AssertRCReturn(rc, rc);

    LogFunc(("sid=%x\n", sid));

    /* Check all contexts if this surface is used as a render target or active texture. */
    for (uint32_t cid = 0; cid < pState->cContexts; cid++)
    {
        PVMSVGA3DCONTEXT pContext = pState->papContexts[cid];
        if (pContext->id == cid)
        {
            for (uint32_t i = 0; i < RT_ELEMENTS(pContext->aSidActiveTextures); ++i)
                if (pContext->aSidActiveTextures[i] == sid)
                    pContext->aSidActiveTextures[i] = SVGA3D_INVALID_ID;
            if (pContext->sidRenderTarget == sid)
                pContext->sidRenderTarget = SVGA3D_INVALID_ID;
        }
    }

    vmsvga3dBackSurfaceDestroy(pState, pSurface);

    if (pSurface->pMipmapLevels)
    {
        for (uint32_t i = 0; i < pSurface->cMipmapLevels; ++i)
            RTMemFree(pSurface->pMipmapLevels[i].pSurfaceData);
        RTMemFree(pSurface->pMipmapLevels);
    }

    memset(pSurface, 0, sizeof(*pSurface));
    pSurface->id = SVGA3D_INVALID_ID;

    return VINF_SUCCESS;
}


/**
 * Implements the SVGA_3D_CMD_SURFACE_STRETCHBLT command (fifo).
 *
 * @returns VBox status code (currently ignored).
 * @param   pThis               The VGA device instance data.
 * @param   pDstSfcImg
 * @param   pDstBox
 * @param   pSrcSfcImg
 * @param   pSrcBox
 * @param   enmMode
 */
int vmsvga3dSurfaceStretchBlt(PVGASTATE pThis, SVGA3dSurfaceImageId const *pDstSfcImg, SVGA3dBox const *pDstBox,
                              SVGA3dSurfaceImageId const *pSrcSfcImg, SVGA3dBox const *pSrcBox, SVGA3dStretchBltMode enmMode)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    int rc;

    uint32_t const sidSrc = pSrcSfcImg->sid;
    PVMSVGA3DSURFACE pSrcSurface;
    rc = vmsvga3dSurfaceFromSid(pState, sidSrc, &pSrcSurface);
    AssertRCReturn(rc, rc);

    uint32_t const sidDst = pDstSfcImg->sid;
    PVMSVGA3DSURFACE pDstSurface;
    rc = vmsvga3dSurfaceFromSid(pState, sidDst, &pDstSurface);
    AssertRCReturn(rc, rc);

    /* Can use faces[0].numMipLevels, because numMipLevels is the same for all faces. */
    AssertReturn(pSrcSfcImg->face < pSrcSurface->cFaces, VERR_INVALID_PARAMETER);
    AssertReturn(pSrcSfcImg->mipmap < pSrcSurface->faces[0].numMipLevels, VERR_INVALID_PARAMETER);
    AssertReturn(pDstSfcImg->face < pDstSurface->cFaces, VERR_INVALID_PARAMETER);
    AssertReturn(pDstSfcImg->mipmap < pDstSurface->faces[0].numMipLevels, VERR_INVALID_PARAMETER);

    PVMSVGA3DCONTEXT pContext;
#ifdef VMSVGA3D_OPENGL
    LogFunc(("src sid=%x (%d,%d)(%d,%d) dest sid=%x (%d,%d)(%d,%d) mode=%x\n",
         sidSrc, pSrcBox->x, pSrcBox->y, pSrcBox->x + pSrcBox->w, pSrcBox->y + pSrcBox->h,
         sidDst, pDstBox->x, pDstBox->y, pDstBox->x + pDstBox->w, pDstBox->y + pDstBox->h, enmMode));
    pContext = &pState->SharedCtx;
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);
#else
    LogFunc(("src sid=%x cid=%x (%d,%d)(%d,%d) dest sid=%x cid=%x (%d,%d)(%d,%d) mode=%x\n",
         sidSrc, pSrcSurface->idAssociatedContext, pSrcBox->x, pSrcBox->y, pSrcBox->x + pSrcBox->w, pSrcBox->y + pSrcBox->h,
         sidDst, pDstSurface->idAssociatedContext, pDstBox->x, pDstBox->y, pDstBox->x + pDstBox->w, pDstBox->y + pDstBox->h, enmMode));

    uint32_t cid = pDstSurface->idAssociatedContext;
    if (cid == SVGA3D_INVALID_ID)
        cid = pSrcSurface->idAssociatedContext;

    /* At least one of surfaces must be in hardware. */
    AssertReturn(cid != SVGA3D_INVALID_ID, VERR_INVALID_PARAMETER);

    rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);
#endif

    if (!VMSVGA3DSURFACE_HAS_HW_SURFACE(pSrcSurface))
    {
        /* Unknown surface type; turn it into a texture, which can be used for other purposes too. */
        LogFunc(("unknown src sid=%x type=%d format=%d -> create texture\n", sidSrc, pSrcSurface->surfaceFlags, pSrcSurface->format));
        rc = vmsvga3dBackCreateTexture(pState, pContext, pContext->id, pSrcSurface);
        AssertRCReturn(rc, rc);
    }

    if (!VMSVGA3DSURFACE_HAS_HW_SURFACE(pDstSurface))
    {
        /* Unknown surface type; turn it into a texture, which can be used for other purposes too. */
        LogFunc(("unknown dest sid=%x type=%d format=%d -> create texture\n", sidDst, pDstSurface->surfaceFlags, pDstSurface->format));
        rc = vmsvga3dBackCreateTexture(pState, pContext, pContext->id, pDstSurface);
        AssertRCReturn(rc, rc);
    }

    PVMSVGA3DMIPMAPLEVEL pSrcMipmapLevel;
    rc = vmsvga3dMipmapLevel(pSrcSurface, pSrcSfcImg->face, pSrcSfcImg->mipmap, &pSrcMipmapLevel);
    AssertRCReturn(rc, rc);

    PVMSVGA3DMIPMAPLEVEL pDstMipmapLevel;
    rc = vmsvga3dMipmapLevel(pDstSurface, pDstSfcImg->face, pDstSfcImg->mipmap, &pDstMipmapLevel);
    AssertRCReturn(rc, rc);

    SVGA3dBox clipSrcBox = *pSrcBox;
    SVGA3dBox clipDstBox = *pDstBox;
    vmsvgaClipBox(&pSrcMipmapLevel->mipmapSize, &clipSrcBox);
    vmsvgaClipBox(&pDstMipmapLevel->mipmapSize, &clipDstBox);

    return vmsvga3dBackSurfaceStretchBlt(pThis, pState,
                                         pDstSurface, pDstSfcImg->face, pDstSfcImg->mipmap, &clipDstBox,
                                         pSrcSurface, pSrcSfcImg->face, pSrcSfcImg->mipmap, &clipSrcBox,
                                         enmMode, pContext);
}

/**
 * Implements the SVGA_3D_CMD_SURFACE_DMA command (fifo).
 *
 * @returns VBox status code (currently ignored).
 * @param   pThis               The VGA device instance data.
 * @param   guest               .
 * @param   host                .
 * @param   transfer            .
 * @param   cCopyBoxes          .
 * @param   paBoxes             .
 */
int vmsvga3dSurfaceDMA(PVGASTATE pThis, SVGA3dGuestImage guest, SVGA3dSurfaceImageId host, SVGA3dTransferType transfer,
                       uint32_t cCopyBoxes, SVGA3dCopyBox *paBoxes)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    PVMSVGA3DSURFACE pSurface;
    int rc = vmsvga3dSurfaceFromSid(pState, host.sid, &pSurface);
    AssertRCReturn(rc, rc);

    LogFunc(("%sguestptr gmr=%x offset=%x pitch=%x host sid=%x face=%d mipmap=%d transfer=%s cCopyBoxes=%d\n",
             (pSurface->surfaceFlags & SVGA3D_SURFACE_HINT_TEXTURE) ? "TEXTURE " : "",
             guest.ptr.gmrId, guest.ptr.offset, guest.pitch,
             host.sid, host.face, host.mipmap, (transfer == SVGA3D_WRITE_HOST_VRAM) ? "READ" : "WRITE", cCopyBoxes));

    PVMSVGA3DMIPMAPLEVEL pMipLevel;
    rc = vmsvga3dMipmapLevel(pSurface, host.face, host.mipmap, &pMipLevel);
    AssertRCReturn(rc, rc);

    if (!VMSVGA3DSURFACE_HAS_HW_SURFACE(pSurface))
    {
        /*
         * Not realized in host hardware/library yet, we have to work with
         * the copy of the data we've got in VMSVGA3DMIMAPLEVEL::pSurfaceData.
         */
        AssertReturn(pMipLevel->pSurfaceData, VERR_INTERNAL_ERROR);

        for (uint32_t i = 0; i < cCopyBoxes; ++i)
        {
            Log(("Copy box (mem) %d (%d,%d,%d)(%d,%d,%d) dest (%d,%d)\n",
                  i, paBoxes[i].srcx, paBoxes[i].srcy, paBoxes[i].srcz, paBoxes[i].w, paBoxes[i].h, paBoxes[i].d, paBoxes[i].x, paBoxes[i].y));

            /* Apparently we're supposed to clip it (gmr test sample) */
            SVGA3dCopyBox clipBox = paBoxes[i];
            vmsvgaClipCopyBox(&pMipLevel->mipmapSize, &pMipLevel->mipmapSize, &clipBox);
            if (   !clipBox.w
                || !clipBox.h
                || !clipBox.d)
            {
                Log(("Skip empty box\n"));
                continue;
            }

            /* Calculate memory addresses of the image blocks for the transfer. */
            uint32_t u32HostBlockX;
            uint32_t u32HostBlockY;
            uint32_t u32GuestBlockX;
            uint32_t u32GuestBlockY;
            uint32_t cBlocksX;
            uint32_t cBlocksY;
            if (RT_LIKELY(pSurface->cxBlock == 1 && pSurface->cyBlock == 1))
            {
                u32HostBlockX = clipBox.x;
                u32HostBlockY = clipBox.y;

                u32GuestBlockX = clipBox.srcx;
                u32GuestBlockY = clipBox.srcy;

                cBlocksX = clipBox.w;
                cBlocksY = clipBox.h;
            }
            else
            {
                /* Pixels to blocks. */
                u32HostBlockX = clipBox.x / pSurface->cxBlock;
                u32HostBlockY = clipBox.y / pSurface->cyBlock;
                Assert(u32HostBlockX * pSurface->cxBlock == clipBox.x);
                Assert(u32HostBlockY * pSurface->cyBlock == clipBox.y);

                u32GuestBlockX = clipBox.srcx / pSurface->cxBlock;
                u32GuestBlockY = clipBox.srcy / pSurface->cyBlock;
                Assert(u32GuestBlockX * pSurface->cxBlock == clipBox.srcx);
                Assert(u32GuestBlockY * pSurface->cyBlock == clipBox.srcy);

                cBlocksX = (clipBox.w + pSurface->cxBlock - 1) / pSurface->cxBlock;
                cBlocksY = (clipBox.h + pSurface->cyBlock - 1) / pSurface->cyBlock;
            }

            uint32_t cbGuestPitch;
            if (guest.pitch == 0)
                cbGuestPitch = cBlocksX * pSurface->cbBlock;
            else
            {
                cbGuestPitch = guest.pitch; /* vmsvgaGMRTransfer will verify the value. */
                AssertReturn(cbGuestPitch <= SVGA3D_MAX_SURFACE_MEM_SIZE, VERR_INVALID_PARAMETER);
            }

            uint64_t uGuestOffset = u32GuestBlockX * pSurface->cbBlock +
                                    u32GuestBlockY * cbGuestPitch +
                                    clipBox.srcz * pMipLevel->mipmapSize.height * cbGuestPitch;
            AssertReturn(uGuestOffset < UINT32_MAX, VERR_INVALID_PARAMETER);

            /* vmsvga3dSurfaceDefine verifies the surface dimensions and clipBox is within them. */
            uint32_t uHostOffset = u32HostBlockX * pSurface->cbBlock +
                                   u32HostBlockY * pMipLevel->cbSurfacePitch +
                                   clipBox.z * pMipLevel->cbSurfacePlane;
            AssertReturn(uHostOffset < pMipLevel->cbSurface, VERR_INTERNAL_ERROR);

            for (uint32_t z = 0; z < clipBox.d; ++z)
            {
                rc = vmsvgaGMRTransfer(pThis,
                                       transfer,
                                       (uint8_t *)pMipLevel->pSurfaceData + uHostOffset,
                                       (int32_t)pMipLevel->cbSurfacePitch,
                                       guest.ptr,
                                       (uint32_t)uGuestOffset,
                                       cbGuestPitch,
                                       cBlocksX * pSurface->cbBlock,
                                       cBlocksY);
                AssertRC(rc);

                Log4(("first line [z=%d]:\n%.*Rhxd\n",
                      z, pMipLevel->cbSurfacePitch, (uint8_t *)pMipLevel->pSurfaceData + uHostOffset));

                uHostOffset += pMipLevel->cbSurfacePlane;
                uGuestOffset += pMipLevel->mipmapSize.height * cbGuestPitch;
            }
        }

        pMipLevel->fDirty = true;
        pSurface->fDirty = true;
    }
    else
    {
        /*
         * Because of the clipping below, we're doing a little more
         * here before calling the backend specific code.
         */
#ifdef VMSVGA3D_DIRECT3D
        /* Flush the drawing pipeline for this surface as it could be used in a shared context. */
        vmsvga3dSurfaceFlush(pThis, pSurface);
        PVMSVGA3DCONTEXT pContext = NULL;

#else /* VMSVGA3D_OPENGL */
        PVMSVGA3DCONTEXT pContext = &pState->SharedCtx;
        VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);
#endif

        for (uint32_t i = 0; i < cCopyBoxes; ++i)
        {
            /** @todo Identical code to the above no-hw branch. Think about merging. */
            Log(("Copy box (hw) %d (%d,%d,%d)(%d,%d,%d) dest (%d,%d)\n",
                  i, paBoxes[i].srcx, paBoxes[i].srcy, paBoxes[i].srcz, paBoxes[i].w, paBoxes[i].h, paBoxes[i].d, paBoxes[i].x, paBoxes[i].y));

            /* Apparently we're supposed to clip it (gmr test sample) */
            SVGA3dCopyBox clipBox = paBoxes[i];
            vmsvgaClipCopyBox(&pMipLevel->mipmapSize, &pMipLevel->mipmapSize, &clipBox);
            if (   !clipBox.w
                || !clipBox.h
                || !clipBox.d)
            {
                Log(("Skip empty box\n"));
                continue;
            }

            /* Calculate memory addresses of the image blocks for the transfer. */
            uint32_t u32HostBlockX;
            uint32_t u32HostBlockY;
            uint32_t u32GuestBlockX;
            uint32_t u32GuestBlockY;
            uint32_t cBlocksX;
            uint32_t cBlocksY;
            if (RT_LIKELY(pSurface->cxBlock == 1 && pSurface->cyBlock == 1))
            {
                u32HostBlockX = clipBox.x;
                u32HostBlockY = clipBox.y;

                u32GuestBlockX = clipBox.srcx;
                u32GuestBlockY = clipBox.srcy;

                cBlocksX = clipBox.w;
                cBlocksY = clipBox.h;
            }
            else
            {
                /* Pixels to blocks. */
                u32HostBlockX = clipBox.x / pSurface->cxBlock;
                u32HostBlockY = clipBox.y / pSurface->cyBlock;
                Assert(u32HostBlockX * pSurface->cxBlock == clipBox.x);
                Assert(u32HostBlockY * pSurface->cyBlock == clipBox.y);

                u32GuestBlockX = clipBox.srcx / pSurface->cxBlock;
                u32GuestBlockY = clipBox.srcy / pSurface->cyBlock;
                Assert(u32GuestBlockX * pSurface->cxBlock == clipBox.srcx);
                Assert(u32GuestBlockY * pSurface->cyBlock == clipBox.srcy);

                cBlocksX = (clipBox.w + pSurface->cxBlock - 1) / pSurface->cxBlock;
                cBlocksY = (clipBox.h + pSurface->cyBlock - 1) / pSurface->cyBlock;
            }

            uint32_t cbGuestPitch;
            if (guest.pitch == 0)
                cbGuestPitch = cBlocksX * pSurface->cbBlock;
            else
            {
                cbGuestPitch = guest.pitch; /* vmsvgaGMRTransfer will verify the value. */
                AssertReturn(cbGuestPitch <= SVGA3D_MAX_SURFACE_MEM_SIZE, VERR_INVALID_PARAMETER);
            }

            rc = vmsvga3dBackSurfaceDMACopyBox(pThis, pState, pSurface, pMipLevel, host.face, host.mipmap,
                                               guest.ptr, cbGuestPitch, transfer,
                                               &clipBox, pContext, rc, i);
            AssertRC(rc);
        }
    }

    return rc;
}

static int vmsvga3dQueryWriteResult(PVGASTATE pThis, SVGAGuestPtr guestResult, SVGA3dQueryState enmState, uint32_t u32Result)
{
    SVGA3dQueryResult queryResult;
    queryResult.totalSize = sizeof(queryResult);    /* Set by guest before query is ended. */
    queryResult.state = enmState;                   /* Set by host or guest. See SVGA3dQueryState. */
    queryResult.result32 = u32Result;

    int rc = vmsvgaGMRTransfer(pThis, SVGA3D_READ_HOST_VRAM, (uint8_t *)&queryResult, sizeof(queryResult),
                               guestResult, 0, sizeof(queryResult), sizeof(queryResult), 1);
    AssertRC(rc);
    return rc;
}

int vmsvga3dQueryBegin(PVGASTATE pThis, uint32_t cid, SVGA3dQueryType type)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    LogFunc(("cid=%x type=%d\n", cid, type));

    PVMSVGA3DCONTEXT pContext;
    int rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    if (type == SVGA3D_QUERYTYPE_OCCLUSION)
    {
        VMSVGA3DQUERY *p = &pContext->occlusion;
        if (!VMSVGA3DQUERY_EXISTS(p))
        {
            /* Lazy creation of the query object. */
            rc = vmsvga3dOcclusionQueryCreate(pState, pContext);
            AssertRCReturn(rc, rc);
        }

        rc = vmsvga3dOcclusionQueryBegin(pState, pContext);
        AssertRCReturn(rc, rc);

        p->enmQueryState = VMSVGA3DQUERYSTATE_BUILDING;
        p->u32QueryResult = 0;

        return VINF_SUCCESS;
    }

    /* Nothing else for VGPU9. */
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}

int vmsvga3dQueryEnd(PVGASTATE pThis, uint32_t cid, SVGA3dQueryType type, SVGAGuestPtr guestResult)
{
    RT_NOREF(guestResult);
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    LogFunc(("cid=%x type=%d guestResult %d:0x%x\n", cid, type, guestResult.gmrId, guestResult.offset));

    PVMSVGA3DCONTEXT pContext;
    int rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    if (type == SVGA3D_QUERYTYPE_OCCLUSION)
    {
        VMSVGA3DQUERY *p = &pContext->occlusion;
        Assert(p->enmQueryState == VMSVGA3DQUERYSTATE_BUILDING);
        AssertMsgReturn(VMSVGA3DQUERY_EXISTS(p), ("Query is NULL\n"), VERR_INTERNAL_ERROR);

        rc = vmsvga3dOcclusionQueryEnd(pState, pContext);
        AssertRCReturn(rc, rc);

        p->enmQueryState = VMSVGA3DQUERYSTATE_ISSUED;

        /* Do not touch guestResult, because the guest will call WaitForQuery. */
        return VINF_SUCCESS;
    }

    /* Nothing else for VGPU9. */
    AssertFailedReturn(VERR_NOT_IMPLEMENTED);
}

int vmsvga3dQueryWait(PVGASTATE pThis, uint32_t cid, SVGA3dQueryType type, SVGAGuestPtr guestResult)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    LogFunc(("cid=%x type=%d guestResult GMR%d:0x%x\n", cid, type, guestResult.gmrId, guestResult.offset));

    PVMSVGA3DCONTEXT pContext;
    int rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    if (type == SVGA3D_QUERYTYPE_OCCLUSION)
    {
        VMSVGA3DQUERY *p = &pContext->occlusion;
        if (VMSVGA3DQUERY_EXISTS(p))
        {
            if (p->enmQueryState == VMSVGA3DQUERYSTATE_ISSUED)
            {
                /* Only if not already in SIGNALED state,
                 * i.e. not a second read from the guest or after restoring saved state.
                 */
                uint32_t u32Pixels = 0;
                rc = vmsvga3dOcclusionQueryGetData(pState, pContext, &u32Pixels);
                if (RT_SUCCESS(rc))
                {
                    p->enmQueryState = VMSVGA3DQUERYSTATE_SIGNALED;
                    p->u32QueryResult += u32Pixels; /* += because it might contain partial result from saved state. */
                }
            }

            if (RT_SUCCESS(rc))
            {
                /* Return data to the guest. */
                vmsvga3dQueryWriteResult(pThis, guestResult, SVGA3D_QUERYSTATE_SUCCEEDED, p->u32QueryResult);
                return VINF_SUCCESS;
            }
        }
        else
        {
            AssertMsgFailed(("GetData Query is NULL\n"));
        }

        rc = VERR_INTERNAL_ERROR;
    }
    else
    {
        rc = VERR_NOT_IMPLEMENTED;
    }

    vmsvga3dQueryWriteResult(pThis, guestResult, SVGA3D_QUERYSTATE_FAILED, 0);
    AssertFailedReturn(rc);
}
