/* $Id: DevVGA-SVGA3d.cpp $ */
/** @file
 * DevSVGA3d - VMWare SVGA device, 3D parts - Common core code.
 */

/*
 * Copyright (C) 2013-2016 Oracle Corporation
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
#else
    pSurface->idAssociatedContext   = SVGA3D_INVALID_ID;
#endif
#ifdef VMSVGA3D_DIRECT3D
    pSurface->hSharedObject         = NULL;
    pSurface->pSharedObjectTree     = NULL;
#else
    pSurface->oglId.buffer = OPENGL_INVALID_ID;
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

    pSurface->flags             = surfaceFlags;
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
        pSurface->pMipmapLevels[i].size = paMipLevelSizes[i];

    pSurface->cbBlock = vmsvga3dSurfaceFormatSize(format);
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
    pSurface->fu32ActualUsageFlags = 0;
#else
    vmsvga3dSurfaceFormat2OGL(pSurface, format);
#endif

    switch (surfaceFlags & (SVGA3D_SURFACE_HINT_INDEXBUFFER | SVGA3D_SURFACE_HINT_VERTEXBUFFER | SVGA3D_SURFACE_HINT_TEXTURE | SVGA3D_SURFACE_HINT_RENDERTARGET | SVGA3D_SURFACE_HINT_DEPTHSTENCIL | SVGA3D_SURFACE_CUBEMAP))
    {
    case SVGA3D_SURFACE_CUBEMAP:
        Log(("SVGA3D_SURFACE_CUBEMAP\n"));
        break;

    case SVGA3D_SURFACE_HINT_INDEXBUFFER:
        Log(("SVGA3D_SURFACE_HINT_INDEXBUFFER\n"));
        /* else type unknown at this time; postpone buffer creation */
        break;

    case SVGA3D_SURFACE_HINT_VERTEXBUFFER:
        Log(("SVGA3D_SURFACE_HINT_VERTEXBUFFER\n"));
        /* Type unknown at this time; postpone buffer creation */
        break;

    case SVGA3D_SURFACE_HINT_TEXTURE:
        Log(("SVGA3D_SURFACE_HINT_TEXTURE\n"));
        break;

    case SVGA3D_SURFACE_HINT_RENDERTARGET:
        Log(("SVGA3D_SURFACE_HINT_RENDERTARGET\n"));
        break;

    case SVGA3D_SURFACE_HINT_DEPTHSTENCIL:
        Log(("SVGA3D_SURFACE_HINT_DEPTHSTENCIL\n"));
        break;

    default:
        /* Unknown; decide later. */
        break;
    }

    Assert(!VMSVGA3DSURFACE_HAS_HW_SURFACE(pSurface));

    /* Allocate buffer to hold the surface data until we can move it into a D3D object */
    uint32_t cbMemRemaining = SVGA3D_MAX_SURFACE_MEM_SIZE; /* Do not allow more than this for a surface. */
    for (uint32_t i = 0; i < cMipLevels; ++i)
    {
        PVMSVGA3DMIPMAPLEVEL pMipmapLevel = &pSurface->pMipmapLevels[i];
        LogFunc(("[%d] face %d mip level %d (%d,%d,%d) cbBlock=0x%x\n",
                 i, i / pSurface->faces[0].numMipLevels, i % pSurface->faces[0].numMipLevels,
                 pMipmapLevel->size.width, pMipmapLevel->size.height, pMipmapLevel->size.depth, pSurface->cbBlock));

        if (   pMipmapLevel->size.width == 0
            || pMipmapLevel->size.height == 0
            || pMipmapLevel->size.depth == 0)
            return VERR_INVALID_PARAMETER;

        const uint32_t cMaxWidth = cbMemRemaining / pSurface->cbBlock;
        if (pMipmapLevel->size.width > cMaxWidth)
            return VERR_INVALID_PARAMETER;
        const uint32_t cbSurfacePitch = pSurface->cbBlock * pMipmapLevel->size.width;
        LogFunc(("cbPitch=0x%x\n", cbSurfacePitch));

        const uint32_t cMaxHeight = cbMemRemaining / cbSurfacePitch;
        if (pMipmapLevel->size.height > cMaxHeight)
            return VERR_INVALID_PARAMETER;
        const uint32_t cbSurfacePlane = cbSurfacePitch * pMipmapLevel->size.height;

        const uint32_t cMaxDepth = cbMemRemaining / cbSurfacePlane;
        if (pMipmapLevel->size.depth > cMaxDepth)
            return VERR_INVALID_PARAMETER;
        const uint32_t cbSurface = cbSurfacePlane * pMipmapLevel->size.depth;

        pMipmapLevel->cbSurfacePitch = cbSurfacePitch;
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

    if (    sid < pState->cSurfaces
        &&  pState->papSurfaces[sid]->id == sid)
    {
        PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];

        Log(("vmsvga3dSurfaceDestroy id %x\n", sid));

        /* Check all contexts if this surface is used as a render target or active texture. */
        for (uint32_t cid = 0; cid < pState->cContexts; cid++)
        {
            PVMSVGA3DCONTEXT pContext = pState->papContexts[cid];
            if (pContext->id == cid)
            {
                for (uint32_t i = 0; i < RT_ELEMENTS(pContext->aSidActiveTexture); i++)
                    if (pContext->aSidActiveTexture[i] == sid)
                        pContext->aSidActiveTexture[i] = SVGA3D_INVALID_ID;
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
    }
    else
        AssertFailedReturn(VERR_INVALID_PARAMETER);

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

    uint32_t const sidSrc = pSrcSfcImg->sid;
    Assert(sidSrc < SVGA3D_MAX_SURFACE_IDS);
    AssertReturn(sidSrc < pState->cSurfaces, VERR_INVALID_PARAMETER);
    PVMSVGA3DSURFACE pSrcSurface  = pState->papSurfaces[sidSrc];
    AssertReturn(pSrcSurface && pSrcSurface->id == sidSrc, VERR_INVALID_PARAMETER);

    uint32_t const sidDst = pDstSfcImg->sid;
    Assert(sidDst < SVGA3D_MAX_SURFACE_IDS);
    AssertReturn(sidDst < pState->cSurfaces, VERR_INVALID_PARAMETER);
    PVMSVGA3DSURFACE pDstSurface = pState->papSurfaces[sidDst];
    AssertReturn(pDstSurface && pDstSurface->id == sidDst, VERR_INVALID_PARAMETER);

    Assert(pSrcSfcImg->face == 0);
    AssertReturn(pSrcSfcImg->mipmap < pSrcSurface->faces[0].numMipLevels, VERR_INVALID_PARAMETER);
    Assert(pDstSfcImg->face == 0);
    AssertReturn(pDstSfcImg->mipmap < pDstSurface->faces[0].numMipLevels, VERR_INVALID_PARAMETER);

    PVMSVGA3DCONTEXT pContext;
#ifdef VMSVGA3D_OPENGL
    Log(("vmsvga3dSurfaceStretchBlt: src sid=%x (%d,%d)(%d,%d) dest sid=%x (%d,%d)(%d,%d) mode=%x\n",
         sidSrc, pSrcBox->x, pSrcBox->y, pSrcBox->x + pSrcBox->w, pSrcBox->y + pSrcBox->h,
         sidDst, pDstBox->x, pDstBox->y, pDstBox->x + pDstBox->w, pDstBox->y + pDstBox->h, enmMode));
    pContext = &pState->SharedCtx;
    VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);
#else
    Log(("vmsvga3dSurfaceStretchBlt: src sid=%x cid=%x (%d,%d)(%d,%d) dest sid=%x cid=%x (%d,%d)(%d,%d) mode=%x\n",
         sidSrc, pSrcSurface->idAssociatedContext, pSrcBox->x, pSrcBox->y, pSrcBox->x + pSrcBox->w, pSrcBox->y + pSrcBox->h,
         sidDst, pDstSurface->idAssociatedContext, pDstBox->x, pDstBox->y, pDstBox->x + pDstBox->w, pDstBox->y + pDstBox->h, enmMode));

    /** @todo stricter checks for associated context */
    uint32_t cid = pDstSurface->idAssociatedContext;
    if (cid == SVGA3D_INVALID_ID)
        cid = pSrcSurface->idAssociatedContext;

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSurfaceStretchBlt invalid context id!\n"));
        AssertFailedReturn(VERR_INVALID_PARAMETER);
    }
    pContext = pState->papContexts[cid];
#endif

    int rc;
    if (!VMSVGA3DSURFACE_HAS_HW_SURFACE(pSrcSurface))
    {
        /* Unknown surface type; turn it into a texture, which can be used for other purposes too. */
        Log(("vmsvga3dSurfaceStretchBlt: unknown src surface id=%x type=%d format=%d -> create texture\n", sidSrc, pSrcSurface->flags, pSrcSurface->format));
        rc = vmsvga3dBackCreateTexture(pState, pContext, pContext->id, pSrcSurface);
        AssertRCReturn(rc, rc);
    }

    if (!VMSVGA3DSURFACE_HAS_HW_SURFACE(pDstSurface))
    {
        /* Unknown surface type; turn it into a texture, which can be used for other purposes too. */
        Log(("vmsvga3dSurfaceStretchBlt: unknown dest surface id=%x type=%d format=%d -> create texture\n", sidDst, pDstSurface->flags, pDstSurface->format));
        rc = vmsvga3dBackCreateTexture(pState, pContext, pContext->id, pDstSurface);
        AssertRCReturn(rc, rc);
    }

    SVGA3dBox clipSrcBox = *pSrcBox;
    SVGA3dBox clipDstBox = *pDstBox;
    vmsvgaClipBox(&pSrcSurface->pMipmapLevels[pSrcSfcImg->mipmap].size, &clipSrcBox);
    vmsvgaClipBox(&pDstSurface->pMipmapLevels[pDstSfcImg->mipmap].size, &clipDstBox);

    return vmsvga3dBackSurfaceStretchBlt(pThis, pState,
                                         pDstSurface, pDstSfcImg->mipmap, &clipDstBox,
                                         pSrcSurface, pSrcSfcImg->mipmap, &clipSrcBox,
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
    int rc = VINF_SUCCESS;

    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    uint32_t sid = host.sid;
    Assert(sid < SVGA3D_MAX_SURFACE_IDS);
    AssertReturn(sid < pState->cSurfaces, VERR_INVALID_PARAMETER);
    PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];
    AssertReturn(pSurface && pSurface->id == sid, VERR_INVALID_PARAMETER);

    if (pSurface->flags & SVGA3D_SURFACE_HINT_TEXTURE)
        Log(("vmsvga3dSurfaceDMA TEXTURE guestptr gmr=%x offset=%x pitch=%x host sid=%x face=%d mipmap=%d transfer=%s cCopyBoxes=%d\n", guest.ptr.gmrId, guest.ptr.offset, guest.pitch, host.sid, host.face, host.mipmap, (transfer == SVGA3D_WRITE_HOST_VRAM) ? "READ" : "WRITE", cCopyBoxes));
    else
        Log(("vmsvga3dSurfaceDMA guestptr gmr=%x offset=%x pitch=%x host sid=%x face=%d mipmap=%d transfer=%s cCopyBoxes=%d\n", guest.ptr.gmrId, guest.ptr.offset, guest.pitch, host.sid, host.face, host.mipmap, (transfer == SVGA3D_WRITE_HOST_VRAM) ? "READ" : "WRITE", cCopyBoxes));

    AssertMsg(host.face == 0, ("host.face=%#x\n", host.face));
    AssertMsgReturn(pSurface->faces[0].numMipLevels > host.mipmap, ("numMipLevels %d, host.mipmap %d", pSurface->faces[0].numMipLevels, host.mipmap), VERR_INVALID_PARAMETER);
    PVMSVGA3DMIPMAPLEVEL pMipLevel = &pSurface->pMipmapLevels[host.mipmap];

    if (!VMSVGA3DSURFACE_HAS_HW_SURFACE(pSurface))
    {
        /*
         * Not realized in host hardware/library yet, we have to work with
         * the copy of the data we've got in VMSVGA3DMIMAPLEVEL::pvSurfaceData.
         */
        AssertReturn(pSurface->pMipmapLevels[host.mipmap].pSurfaceData, VERR_INTERNAL_ERROR);

        for (unsigned i = 0; i < cCopyBoxes; i++)
        {
            unsigned uDestOffset;
            unsigned cbSrcPitch;
            uint8_t *pBufferStart;

            Log(("Copy box %d (%d,%d,%d)(%d,%d,%d) dest (%d,%d)\n", i, paBoxes[i].srcx, paBoxes[i].srcy, paBoxes[i].srcz, paBoxes[i].w, paBoxes[i].h, paBoxes[i].d, paBoxes[i].x, paBoxes[i].y));

            /* Apparently we're supposed to clip it (gmr test sample) */
            SVGA3dCopyBox clipBox = paBoxes[i];
            vmsvgaClipCopyBox(&pMipLevel->size, &pMipLevel->size, &clipBox);
            if (   !clipBox.w
                || !clipBox.h
                || !clipBox.d)
            {
                Log(("Empty box; skip\n"));
                continue;
            }

            uDestOffset = clipBox.x * pSurface->cbBlock + clipBox.y * pMipLevel->cbSurfacePitch + clipBox.z * pMipLevel->size.height * pMipLevel->cbSurfacePitch;
            AssertReturn(uDestOffset + clipBox.w * pSurface->cbBlock * clipBox.h * clipBox.d <= pMipLevel->cbSurface, VERR_INTERNAL_ERROR);

            cbSrcPitch = (guest.pitch == 0) ? clipBox.w * pSurface->cbBlock : guest.pitch;
#ifdef MANUAL_FLIP_SURFACE_DATA
            pBufferStart =    (uint8_t *)pMipLevel->pSurfaceData
                            + clipBox.x * pSurface->cbBlock
                            + pMipLevel->cbSurface - clipBox.y * pMipLevel->cbSurfacePitch
                            - pMipLevel->cbSurfacePitch;      /* flip image during copy */
#else
            pBufferStart = (uint8_t *)pMipLevel->pSurfaceData + uDestOffset;
#endif
            rc = vmsvgaGMRTransfer(pThis,
                                   transfer,
                                   pBufferStart,
#ifdef MANUAL_FLIP_SURFACE_DATA
                                   -(int32_t)pMipLevel->cbSurfacePitch,
#else
                                   (int32_t)pMipLevel->cbSurfacePitch,
#endif
                                   guest.ptr,
                                   clipBox.srcx * pSurface->cbBlock + (clipBox.srcy + clipBox.srcz * clipBox.h) * cbSrcPitch,
                                   cbSrcPitch,
                                   clipBox.w * pSurface->cbBlock,
                                   clipBox.d * clipBox.h);

            Log4(("first line:\n%.*Rhxd\n", pMipLevel->cbSurfacePitch, pMipLevel->pSurfaceData));

            AssertRC(rc);
        }
        pSurface->pMipmapLevels[host.mipmap].fDirty = true;
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

        for (unsigned i = 0; i < cCopyBoxes; i++)
        {
            Log(("Copy box %d (%d,%d,%d)(%d,%d,%d) dest (%d,%d)\n", i, paBoxes[i].srcx, paBoxes[i].srcy, paBoxes[i].srcz, paBoxes[i].w, paBoxes[i].h, paBoxes[i].d, paBoxes[i].x, paBoxes[i].y));

            /** @todo Is d == 0 valid? */
            Assert((paBoxes[i].d == 1 || paBoxes[i].d == 0) && paBoxes[i].z == 0);

            /* Apparently we're supposed to clip it (gmr test sample) */
            SVGA3dCopyBox clipBox = paBoxes[i];
            vmsvgaClipCopyBox(&pMipLevel->size, &pMipLevel->size, &clipBox);
            if (   !clipBox.w
                || !clipBox.h
                || !clipBox.d)
            {
                Log(("Empty box; skip\n"));
                continue;
            }

            uint32_t cbSrcPitch = (guest.pitch == 0) ? clipBox.w * pSurface->cbBlock : guest.pitch;
            rc = vmsvga3dBackSurfaceDMACopyBox(pThis, pState, pSurface, host.mipmap, guest.ptr, cbSrcPitch, transfer,
                                               &clipBox, pContext, rc, i);
        }
    }

    return rc;
}

