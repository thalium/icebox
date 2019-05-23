/* $Id: DevVGA-SVGA3d-info.cpp $ */
/** @file
 * DevSVGA3d - VMWare SVGA device, 3D parts - Introspection and debugging.
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


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Enum value to string mappings for SVGA3dSurfaceFormat, prefix "SVGA3D_". */
static const VMSVGAINFOENUM g_aSVGA3dSurfaceFormats[] =
{
    { SVGA3D_FORMAT_INVALID     , "FORMAT_INVALID" },
    { SVGA3D_X8R8G8B8           , "X8R8G8B8" },
    { SVGA3D_A8R8G8B8           , "A8R8G8B8" },
    { SVGA3D_R5G6B5             , "R5G6B5" },
    { SVGA3D_X1R5G5B5           , "X1R5G5B5" },
    { SVGA3D_A1R5G5B5           , "A1R5G5B5" },
    { SVGA3D_A4R4G4B4           , "A4R4G4B4" },
    { SVGA3D_Z_D32              , "Z_D32" },
    { SVGA3D_Z_D16              , "Z_D16" },
    { SVGA3D_Z_D24S8            , "Z_D24S8" },
    { SVGA3D_Z_D15S1            , "Z_D15S1" },
    { SVGA3D_LUMINANCE8         , "LUMINANCE8" },
    { SVGA3D_LUMINANCE4_ALPHA4  , "LUMINANCE4_ALPHA4" },
    { SVGA3D_LUMINANCE16        , "LUMINANCE16" },
    { SVGA3D_LUMINANCE8_ALPHA8  , "LUMINANCE8_ALPHA8" },
    { SVGA3D_DXT1               , "DXT1" },
    { SVGA3D_DXT2               , "DXT2" },
    { SVGA3D_DXT3               , "DXT3" },
    { SVGA3D_DXT4               , "DXT4" },
    { SVGA3D_DXT5               , "DXT5" },
    { SVGA3D_BUMPU8V8           , "BUMPU8V8" },
    { SVGA3D_BUMPL6V5U5         , "BUMPL6V5U5" },
    { SVGA3D_BUMPX8L8V8U8       , "BUMPX8L8V8U8" },
    { SVGA3D_BUMPL8V8U8         , "BUMPL8V8U8" },
    { SVGA3D_ARGB_S10E5         , "ARGB_S10E5" },
    { SVGA3D_ARGB_S23E8         , "ARGB_S23E8" },
    { SVGA3D_A2R10G10B10        , "A2R10G10B10" },
    { SVGA3D_V8U8               , "V8U8" },
    { SVGA3D_Q8W8V8U8           , "Q8W8V8U8" },
    { SVGA3D_CxV8U8             , "CxV8U8" },
    { SVGA3D_X8L8V8U8           , "X8L8V8U8" },
    { SVGA3D_A2W10V10U10        , "A2W10V10U10" },
    { SVGA3D_ALPHA8             , "ALPHA8" },
    { SVGA3D_R_S10E5            , "R_S10E5" },
    { SVGA3D_R_S23E8            , "R_S23E8" },
    { SVGA3D_RG_S10E5           , "RG_S10E5" },
    { SVGA3D_RG_S23E8           , "RG_S23E8" },
    { SVGA3D_BUFFER             , "BUFFER" },
    { SVGA3D_Z_D24X8            , "Z_D24X8" },
    { SVGA3D_V16U16             , "V16U16" },
    { SVGA3D_G16R16             , "G16R16" },
    { SVGA3D_A16B16G16R16       , "A16B16G16R16" },
    { SVGA3D_UYVY               , "UYVY" },
    { SVGA3D_YUY2               , "YUY2" },
    { SVGA3D_NV12               , "NV12" },
    { SVGA3D_AYUV               , "AYUV" },
    { SVGA3D_BC4_UNORM          , "BC4_UNORM" },
    { SVGA3D_BC5_UNORM          , "BC5_UNORM" },
    { SVGA3D_Z_DF16             , "Z_DF16" },
    { SVGA3D_Z_DF24             , "Z_DF24" },
    { SVGA3D_Z_D24S8_INT        , "Z_D24S8_INT" },
};
VMSVGAINFOENUMMAP_MAKE(RT_NOTHING, g_SVGA3dSurfaceFormat2String, g_aSVGA3dSurfaceFormats, "SVGA3D_");

/** Values for SVGA3dTextureFilter, prefix SVGA3D_TEX_FILTER_. */
static const char * const g_apszTexureFilters[] =
{
    "NONE",
    "NEAREST",
    "LINEAR",
    "ANISOTROPIC",
    "FLATCUBIC",
    "GAUSSIANCUBIC",
    "PYRAMIDALQUAD",
    "GAUSSIANQUAD",
};

/** SVGA3dSurfaceFlags values, prefix SVGA3D_SURFACE_. */
static VMSVGAINFOFLAGS32 const g_aSvga3DSurfaceFlags[] =
{
    { SVGA3D_SURFACE_CUBEMAP            , "CUBEMAP" },
    { SVGA3D_SURFACE_HINT_STATIC        , "HINT_STATIC" },
    { SVGA3D_SURFACE_HINT_DYNAMIC       , "HINT_DYNAMIC" },
    { SVGA3D_SURFACE_HINT_INDEXBUFFER   , "HINT_INDEXBUFFER" },
    { SVGA3D_SURFACE_HINT_VERTEXBUFFER  , "HINT_VERTEXBUFFER" },
    { SVGA3D_SURFACE_HINT_TEXTURE       , "HINT_TEXTURE" },
    { SVGA3D_SURFACE_HINT_RENDERTARGET  , "HINT_RENDERTARGET" },
    { SVGA3D_SURFACE_HINT_DEPTHSTENCIL  , "HINT_DEPTHSTENCIL" },
    { SVGA3D_SURFACE_HINT_WRITEONLY     , "HINT_WRITEONLY" },
    { SVGA3D_SURFACE_MASKABLE_ANTIALIAS , "MASKABLE_ANTIALIAS" },
    { SVGA3D_SURFACE_AUTOGENMIPMAPS     , "AUTOGENMIPMAPS" },
};


#ifdef VMSVGA3D_DIRECT3D

/** Values for D3DFORMAT, prefix D3DFMT_. */
static VMSVGAINFOENUM const g_aD3DFormats[] =
{
    { D3DFMT_UNKNOWN        , "UNKNOWN" },
    { D3DFMT_R8G8B8         , "R8G8B8" },
    { D3DFMT_A8R8G8B8       , "A8R8G8B8" },
    { D3DFMT_X8R8G8B8       , "X8R8G8B8" },
    { D3DFMT_R5G6B5         , "R5G6B5" },
    { D3DFMT_X1R5G5B5       , "X1R5G5B5" },
    { D3DFMT_A1R5G5B5       , "A1R5G5B5" },
    { D3DFMT_A4R4G4B4       , "A4R4G4B4" },
    { D3DFMT_R3G3B2         , "R3G3B2" },
    { D3DFMT_A8             , "A8" },
    { D3DFMT_A8R3G3B2       , "A8R3G3B2" },
    { D3DFMT_X4R4G4B4       , "X4R4G4B4" },
    { D3DFMT_A2B10G10R10    , "A2B10G10R10" },
    { D3DFMT_A8B8G8R8       , "A8B8G8R8" },
    { D3DFMT_X8B8G8R8       , "X8B8G8R8" },
    { D3DFMT_G16R16         , "G16R16" },
    { D3DFMT_A2R10G10B10    , "A2R10G10B10" },
    { D3DFMT_A16B16G16R16   , "A16B16G16R16" },
    { D3DFMT_A8P8           , "A8P8" },
    { D3DFMT_P8             , "P8" },
    { D3DFMT_L8             , "L8" },
    { D3DFMT_A8L8           , "A8L8" },
    { D3DFMT_A4L4           , "A4L4" },
    { D3DFMT_V8U8           , "V8U8" },
    { D3DFMT_L6V5U5         , "L6V5U5" },
    { D3DFMT_X8L8V8U8       , "X8L8V8U8" },
    { D3DFMT_Q8W8V8U8       , "Q8W8V8U8" },
    { D3DFMT_V16U16         , "V16U16" },
    { D3DFMT_A2W10V10U10    , "A2W10V10U10" },
    { D3DFMT_D16_LOCKABLE   , "D16_LOCKABLE" },
    { D3DFMT_D32            , "D32" },
    { D3DFMT_D15S1          , "D15S1" },
    { D3DFMT_D24S8          , "D24S8" },
    { D3DFMT_D24X8          , "D24X8" },
    { D3DFMT_D24X4S4        , "D24X4S4" },
    { D3DFMT_D16            , "D16" },
    { D3DFMT_L16            , "L16" },
    { D3DFMT_D32F_LOCKABLE  , "D32F_LOCKABLE" },
    { D3DFMT_D24FS8         , "D24FS8" },
    { D3DFMT_VERTEXDATA     , "VERTEXDATA" },
    { D3DFMT_INDEX16        , "INDEX16" },
    { D3DFMT_INDEX32        , "INDEX32" },
    { D3DFMT_Q16W16V16U16   , "Q16W16V16U16" },
    { D3DFMT_R16F           , "R16F" },
    { D3DFMT_G16R16F        , "G16R16F" },
    { D3DFMT_A16B16G16R16F  , "A16B16G16R16F" },
    { D3DFMT_R32F           , "R32F" },
    { D3DFMT_G32R32F        , "G32R32F" },
    { D3DFMT_A32B32G32R32F  , "A32B32G32R32F" },
    { D3DFMT_CxV8U8         , "CxV8U8" },
    /* Fourcc values, MSB is in the right most char:  */
    { D3DFMT_MULTI2_ARGB8   , "MULTI2_ARGB8" },
    { D3DFMT_DXT1           , "DXT1" },
    { D3DFMT_DXT2           , "DXT2" },
    { D3DFMT_YUY2           , "YUY2" },
    { D3DFMT_DXT3           , "DXT3" },
    { D3DFMT_DXT4           , "DXT4" },
    { D3DFMT_DXT5           , "DXT5" },
    { D3DFMT_G8R8_G8B8      , "G8R8_G8B8" },
    { D3DFMT_R8G8_B8G8      , "R8G8_B8G8" },
    { D3DFMT_UYVY           , "UYVY" },
    { D3DFMT_FORCE_DWORD    , "FORCE_DWORD" }, /* UINT32_MAX */
};
VMSVGAINFOENUMMAP_MAKE(static, g_D3DFormat2String, g_aD3DFormats, "D3DFMT_");

/** Values for D3DMULTISAMPLE_TYPE, prefix D3DMULTISAMPLE_. */
static VMSVGAINFOENUM const g_aD3DMultiSampleTypes[] =
{
    { D3DMULTISAMPLE_NONE           , "NONE" },
    { D3DMULTISAMPLE_NONMASKABLE    , "NONMASKABLE" },
    { D3DMULTISAMPLE_2_SAMPLES      , "2_SAMPLES" },
    { D3DMULTISAMPLE_3_SAMPLES      , "3_SAMPLES" },
    { D3DMULTISAMPLE_4_SAMPLES      , "4_SAMPLES" },
    { D3DMULTISAMPLE_5_SAMPLES      , "5_SAMPLES" },
    { D3DMULTISAMPLE_6_SAMPLES      , "6_SAMPLES" },
    { D3DMULTISAMPLE_7_SAMPLES      , "7_SAMPLES" },
    { D3DMULTISAMPLE_8_SAMPLES      , "8_SAMPLES" },
    { D3DMULTISAMPLE_9_SAMPLES      , "9_SAMPLES" },
    { D3DMULTISAMPLE_10_SAMPLES     , "10_SAMPLES" },
    { D3DMULTISAMPLE_11_SAMPLES     , "11_SAMPLES" },
    { D3DMULTISAMPLE_12_SAMPLES     , "12_SAMPLES" },
    { D3DMULTISAMPLE_13_SAMPLES     , "13_SAMPLES" },
    { D3DMULTISAMPLE_14_SAMPLES     , "14_SAMPLES" },
    { D3DMULTISAMPLE_15_SAMPLES     , "15_SAMPLES" },
    { D3DMULTISAMPLE_16_SAMPLES     , "16_SAMPLES" },
    { D3DMULTISAMPLE_FORCE_DWORD    , "FORCE_DWORD" },
};
VMSVGAINFOENUMMAP_MAKE(static, g_D3DMultiSampleType2String, g_aD3DMultiSampleTypes, "D3DMULTISAMPLE_");

/** D3DUSAGE_XXX flag value, prefix D3DUSAGE_. */
static VMSVGAINFOFLAGS32 const g_aD3DUsageFlags[] =
{
    { D3DUSAGE_RENDERTARGET                     , "RENDERTARGET" },
    { D3DUSAGE_DEPTHSTENCIL                     , "DEPTHSTENCIL" },
    { D3DUSAGE_WRITEONLY                        , "WRITEONLY" },
    { D3DUSAGE_SOFTWAREPROCESSING               , "SOFTWAREPROCESSING" },
    { D3DUSAGE_DONOTCLIP                        , "DONOTCLIP" },
    { D3DUSAGE_POINTS                           , "POINTS" },
    { D3DUSAGE_RTPATCHES                        , "RTPATCHES" },
    { D3DUSAGE_NPATCHES                         , "NPATCHES" },
    { D3DUSAGE_DYNAMIC                          , "DYNAMIC" },
    { D3DUSAGE_AUTOGENMIPMAP                    , "AUTOGENMIPMAP" },
    { D3DUSAGE_RESTRICTED_CONTENT               , "RESTRICTED_CONTENT" },
    { D3DUSAGE_RESTRICT_SHARED_RESOURCE_DRIVER  , "RESTRICT_SHARED_RESOURCE_DRIVER" },
    { D3DUSAGE_RESTRICT_SHARED_RESOURCE         , "RESTRICT_SHARED_RESOURCE" },
    { D3DUSAGE_DMAP                             , "DMAP" },
    { D3DUSAGE_NONSECURE                        , "NONSECURE" },
    { D3DUSAGE_TEXTAPI                          , "TEXTAPI" },
};

#endif /* VMSVGA3D_DIRECT3D */


/**
 * Worker for vmsvga3dUpdateHeapBuffersForSurfaces.
 *
 * This will allocate heap buffers if necessary, thus increasing the memory
 * usage of the process.
 *
 * @todo Would be interesting to share this code with the saved state code.
 *
 * @returns VBox status code.
 * @param   pState              The 3D state structure.
 * @param   pSurface            The surface to refresh the heap buffers for.
 */
static int vmsvga3dSurfaceUpdateHeapBuffers(PVMSVGA3DSTATE pState, PVMSVGA3DSURFACE pSurface)
{
    /*
     * Currently we've got trouble retreving bit for DEPTHSTENCIL
     * surfaces both for OpenGL and D3D, so skip these here (don't
     * wast memory on them).
     */
    uint32_t const fSwitchFlags = pSurface->flags
                                & (  SVGA3D_SURFACE_HINT_INDEXBUFFER  | SVGA3D_SURFACE_HINT_VERTEXBUFFER
                                   | SVGA3D_SURFACE_HINT_TEXTURE      | SVGA3D_SURFACE_HINT_RENDERTARGET
                                   | SVGA3D_SURFACE_HINT_DEPTHSTENCIL | SVGA3D_SURFACE_CUBEMAP);
    if (   fSwitchFlags != SVGA3D_SURFACE_HINT_DEPTHSTENCIL
        && fSwitchFlags != (SVGA3D_SURFACE_HINT_DEPTHSTENCIL | SVGA3D_SURFACE_HINT_TEXTURE))
    {

#ifdef VMSVGA3D_OPENGL
        /*
         * Change OpenGL context to the one the surface is associated with.
         */
        PVMSVGA3DCONTEXT pContext = &pState->SharedCtx;
        VMSVGA3D_SET_CURRENT_CONTEXT(pState, pContext);
#endif

        /*
         * Work thru each mipmap level for each face.
         */
        for (uint32_t iFace = 0; iFace < pSurface->cFaces; iFace++)
        {
            Assert(pSurface->faces[iFace].numMipLevels <= pSurface->faces[0].numMipLevels);
            PVMSVGA3DMIPMAPLEVEL pMipmapLevel = &pSurface->pMipmapLevels[iFace * pSurface->faces[0].numMipLevels];
            for (uint32_t i = 0; i < pSurface->faces[iFace].numMipLevels; i++, pMipmapLevel++)
            {
#ifdef VMSVGA3D_DIRECT3D
                if (pSurface->u.pSurface)
#else
                if (pSurface->oglId.texture != OPENGL_INVALID_ID)
#endif
                {
                    Assert(pMipmapLevel->cbSurface);
                    Assert(pMipmapLevel->cbSurface == pMipmapLevel->cbSurfacePitch * pMipmapLevel->size.height); /* correct for depth stuff? */

                    /*
                     * Make sure we've got surface memory buffer.
                     */
                    uint8_t *pbDst = (uint8_t *)pMipmapLevel->pSurfaceData;
                    if (!pbDst)
                    {
                        pMipmapLevel->pSurfaceData = pbDst = (uint8_t *)RTMemAllocZ(pMipmapLevel->cbSurface);
                        AssertReturn(pbDst, VERR_NO_MEMORY);
                    }

#ifdef VMSVGA3D_DIRECT3D
                    /*
                     * D3D specifics.
                     */
                    HRESULT  hr;
                    switch (fSwitchFlags)
                    {
                        case SVGA3D_SURFACE_HINT_TEXTURE:
                        case SVGA3D_SURFACE_HINT_RENDERTARGET:
                        case SVGA3D_SURFACE_HINT_TEXTURE | SVGA3D_SURFACE_HINT_RENDERTARGET:
                        {
                            /*
                             * Lock the buffer and make it accessible to memcpy.
                             */
                            D3DLOCKED_RECT LockedRect;
                            if (fSwitchFlags & SVGA3D_SURFACE_HINT_TEXTURE)
                            {
                                if (pSurface->bounce.pTexture)
                                {
                                    if (    !pSurface->fDirty
                                        &&  fSwitchFlags == (SVGA3D_SURFACE_HINT_TEXTURE | SVGA3D_SURFACE_HINT_RENDERTARGET)
                                        &&  i == 0 /* only the first time */)
                                    {
                                        /** @todo stricter checks for associated context */
                                        uint32_t cid = pSurface->idAssociatedContext;
                                        if (   cid >= pState->cContexts
                                            || pState->papContexts[cid]->id != cid)
                                        {
                                            Log(("vmsvga3dSurfaceUpdateHeapBuffers: invalid context id (%x - %x)!\n", cid, (cid >= pState->cContexts) ? -1 : pState->papContexts[cid]->id));
                                            AssertFailedReturn(VERR_INVALID_PARAMETER);
                                        }
                                        PVMSVGA3DCONTEXT pContext = pState->papContexts[cid];

                                        IDirect3DSurface9 *pDst = NULL;
                                        hr = pSurface->bounce.pTexture->GetSurfaceLevel(i, &pDst);
                                        AssertMsgReturn(hr == D3D_OK, ("GetSurfaceLevel failed with %#x\n", hr), VERR_INTERNAL_ERROR);

                                        IDirect3DSurface9 *pSrc = NULL;
                                        hr = pSurface->u.pTexture->GetSurfaceLevel(i, &pSrc);
                                        AssertMsgReturn(hr == D3D_OK, ("GetSurfaceLevel failed with %#x\n", hr), VERR_INTERNAL_ERROR);

                                        hr = pContext->pDevice->GetRenderTargetData(pSrc, pDst);
                                        AssertMsgReturn(hr == D3D_OK, ("GetRenderTargetData failed with %#x\n", hr), VERR_INTERNAL_ERROR);

                                        pSrc->Release();
                                        pDst->Release();
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
                            AssertMsgReturn(hr == D3D_OK, ("LockRect failed with %x\n", hr), VERR_INTERNAL_ERROR);

                            /*
                             * Copy the data.  Take care in case the pitch differs.
                             */
                            if (pMipmapLevel->cbSurfacePitch == (uint32_t)LockedRect.Pitch)
                                memcpy(pbDst, LockedRect.pBits, pMipmapLevel->cbSurface);
                            else
                                for (uint32_t j = 0; j < pMipmapLevel->size.height; j++)
                                    memcpy(pbDst + j * pMipmapLevel->cbSurfacePitch,
                                           (uint8_t *)LockedRect.pBits + j * LockedRect.Pitch,
                                           pMipmapLevel->cbSurfacePitch);

                            /*
                             * Release the buffer.
                             */
                            if (fSwitchFlags & SVGA3D_SURFACE_HINT_TEXTURE)
                            {
                                if (pSurface->bounce.pTexture)
                                {
                                    hr = pSurface->bounce.pTexture->UnlockRect(i);
                                    AssertMsgReturn(hr == D3D_OK, ("UnlockRect failed with %#x\n", hr), VERR_INTERNAL_ERROR);
                                }
                                else
                                    hr = pSurface->u.pTexture->UnlockRect(i);
                            }
                            else
                                hr = pSurface->u.pSurface->UnlockRect();
                            AssertMsgReturn(hr == D3D_OK, ("UnlockRect failed with %#x\n", hr), VERR_INTERNAL_ERROR);
                            break;
                        }

                        case SVGA3D_SURFACE_HINT_VERTEXBUFFER:
                        {
                            void *pvD3DData = NULL;
                            hr = pSurface->u.pVertexBuffer->Lock(0, 0, &pvD3DData, D3DLOCK_READONLY);
                            AssertMsgReturn(hr == D3D_OK, ("Lock vertex failed with %x\n", hr), VERR_INTERNAL_ERROR);

                            memcpy(pbDst, pvD3DData, pMipmapLevel->cbSurface);

                            hr = pSurface->u.pVertexBuffer->Unlock();
                            AssertMsg(hr == D3D_OK, ("Unlock vertex failed with %x\n", hr));
                            break;
                        }

                        case SVGA3D_SURFACE_HINT_INDEXBUFFER:
                        {
                            void *pvD3DData = NULL;
                            hr = pSurface->u.pIndexBuffer->Lock(0, 0, &pvD3DData, D3DLOCK_READONLY);
                            AssertMsgReturn(hr == D3D_OK, ("Lock index failed with %x\n", hr), VERR_INTERNAL_ERROR);

                            memcpy(pbDst, pvD3DData, pMipmapLevel->cbSurface);

                            hr = pSurface->u.pIndexBuffer->Unlock();
                            AssertMsg(hr == D3D_OK, ("Unlock index failed with %x\n", hr));
                            break;
                        }

                        default:
                            AssertMsgFailed(("%#x\n", fSwitchFlags));
                    }

#elif defined(VMSVGA3D_OPENGL)
                    /*
                     * OpenGL specifics.
                     */
                    switch (fSwitchFlags)
                    {
                        case SVGA3D_SURFACE_HINT_TEXTURE:
                        case SVGA3D_SURFACE_HINT_RENDERTARGET:
                        case SVGA3D_SURFACE_HINT_TEXTURE | SVGA3D_SURFACE_HINT_RENDERTARGET:
                        {
                            GLint activeTexture;
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
                                          pbDst);
                            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);

                            vmsvga3dOglRestorePackParams(pState, pContext, pSurface, &SavedParams);

                            /* Restore the old active texture. */
                            glBindTexture(GL_TEXTURE_2D, activeTexture);
                            VMSVGA3D_CHECK_LAST_ERROR_WARN(pState, pContext);
                            break;
                        }

                        case SVGA3D_SURFACE_HINT_VERTEXBUFFER:
                        case SVGA3D_SURFACE_HINT_INDEXBUFFER:
                        {
                            pState->ext.glBindBuffer(GL_ARRAY_BUFFER, pSurface->oglId.buffer);
                            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

                            void *pvSrc = pState->ext.glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
                            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                            if (RT_VALID_PTR(pvSrc))
                                memcpy(pbDst, pvSrc, pMipmapLevel->cbSurface);
                            else
                                AssertPtr(pvSrc);

                            pState->ext.glUnmapBuffer(GL_ARRAY_BUFFER);
                            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);

                            pState->ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
                            VMSVGA3D_CHECK_LAST_ERROR(pState, pContext);
                            break;
                        }

                        default:
                            AssertMsgFailed(("%#x\n", fSwitchFlags));
                    }
#else
# error "misconfigured"
#endif
                }
                /* else: There is no data in hardware yet, so whatever we got is already current. */
            }
        }
    }

    return VINF_SUCCESS;
}


/**
 * Updates the heap buffers for all surfaces or one specific one.
 *
 * @param   pThis               The VGA device instance data.
 * @param   sid                 The surface ID, UINT32_MAX if all.
 * @thread  VMSVGAFIFO
 */
void vmsvga3dUpdateHeapBuffersForSurfaces(PVGASTATE pThis, uint32_t sid)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturnVoid(pState);

    if (sid == UINT32_MAX)
    {
        uint32_t cSurfaces = pState->cSurfaces;
        for (sid = 0; sid < cSurfaces; sid++)
        {
            PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];
            if (pSurface && pSurface->id == sid)
                vmsvga3dSurfaceUpdateHeapBuffers(pState, pSurface);
        }
    }
    else if (sid < pState->cSurfaces)
    {
        PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];
        if (pSurface && pSurface->id == sid)
            vmsvga3dSurfaceUpdateHeapBuffers(pState, pSurface);
    }
}




void vmsvga3dInfoU32Flags(PCDBGFINFOHLP pHlp, uint32_t fFlags, const char *pszPrefix, PCVMSVGAINFOFLAGS32 paFlags, uint32_t cFlags)
{
    for (uint32_t i = 0; i < cFlags; i++)
        if ((paFlags[i].fFlags & fFlags) == paFlags[i].fFlags)
        {
            Assert(paFlags[i].fFlags);
            pHlp->pfnPrintf(pHlp, " %s%s", pszPrefix, paFlags[i].pszJohnny);
            fFlags &= ~paFlags[i].fFlags;
            if (!fFlags)
                return;
        }
    if (fFlags)
        pHlp->pfnPrintf(pHlp, " UNKNOWN_%#x", fFlags);
}


/**
 * Worker for vmsvgaR3Info that display details of a host window.
 *
 * @param   pHlp            The output methods.
 * @param   idHostWindow    The host window handle/id/whatever.
 */
void vmsvga3dInfoHostWindow(PCDBGFINFOHLP pHlp, uint64_t idHostWindow)
{
#ifdef RT_OS_WINDOWS
    HWND hwnd = (HWND)(uintptr_t)idHostWindow;
    Assert((uintptr_t)hwnd == idHostWindow);
    if (hwnd != NULL)
    {
        WINDOWINFO Info;
        RT_ZERO(Info);
        Info.cbSize = sizeof(Info);
        if (GetWindowInfo(hwnd, &Info))
        {
            pHlp->pfnPrintf(pHlp, "     Window rect:   xLeft=%d, yTop=%d, xRight=%d, yBottom=%d (cx=%d, cy=%d)\n",
                            Info.rcWindow.left, Info.rcWindow.top, Info.rcWindow.right, Info.rcWindow.bottom,
                            Info.rcWindow.right - Info.rcWindow.left, Info.rcWindow.bottom - Info.rcWindow.top);
            pHlp->pfnPrintf(pHlp, "     Client rect:   xLeft=%d, yTop=%d, xRight=%d, yBottom=%d (cx=%d, cy=%d)\n",
                            Info.rcClient.left, Info.rcClient.top, Info.rcClient.right, Info.rcClient.bottom,
                            Info.rcClient.right - Info.rcClient.left, Info.rcClient.bottom - Info.rcClient.top);
            pHlp->pfnPrintf(pHlp, "     Style:         %#x", Info.dwStyle);
            static const VMSVGAINFOFLAGS32 g_aStyles[] =
            {
                { WS_POPUP        , "POPUP" },
                { WS_CHILD        , "CHILD" },
                { WS_MINIMIZE     , "MINIMIZE" },
                { WS_VISIBLE      , "VISIBLE" },
                { WS_DISABLED     , "DISABLED" },
                { WS_CLIPSIBLINGS , "CLIPSIBLINGS" },
                { WS_CLIPCHILDREN , "CLIPCHILDREN" },
                { WS_MAXIMIZE     , "MAXIMIZE" },
                { WS_BORDER       , "BORDER" },
                { WS_DLGFRAME     , "DLGFRAME" },
                { WS_VSCROLL      , "VSCROLL" },
                { WS_HSCROLL      , "HSCROLL" },
                { WS_SYSMENU      , "SYSMENU" },
                { WS_THICKFRAME   , "THICKFRAME" },
                { WS_GROUP        , "GROUP" },
                { WS_TABSTOP      , "TABSTOP" },
            };
            vmsvga3dInfoU32Flags(pHlp, Info.dwStyle, "", g_aStyles, RT_ELEMENTS(g_aStyles));
            pHlp->pfnPrintf(pHlp, "\n");

            pHlp->pfnPrintf(pHlp, "     ExStyle:       %#x", Info.dwExStyle);
            static const VMSVGAINFOFLAGS32 g_aExStyles[] =
            {
                { WS_EX_DLGMODALFRAME,   "DLGMODALFRAME" },
                { 0x00000002,            "DRAGDETECT" },
                { WS_EX_NOPARENTNOTIFY,  "NOPARENTNOTIFY" },
                { WS_EX_TOPMOST,         "TOPMOST" },
                { WS_EX_ACCEPTFILES,     "ACCEPTFILES" },
                { WS_EX_TRANSPARENT,     "TRANSPARENT" },
                { WS_EX_MDICHILD,        "MDICHILD" },
                { WS_EX_TOOLWINDOW,      "TOOLWINDOW" },
                { WS_EX_WINDOWEDGE,      "WINDOWEDGE" },
                { WS_EX_CLIENTEDGE,      "CLIENTEDGE" },
                { WS_EX_CONTEXTHELP,     "CONTEXTHELP" },
                { WS_EX_RIGHT,           "RIGHT" },
                /*{ WS_EX_LEFT,            "LEFT" }, = 0 */
                { WS_EX_RTLREADING,      "RTLREADING" },
                /*{ WS_EX_LTRREADING,      "LTRREADING" }, = 0 */
                { WS_EX_LEFTSCROLLBAR,   "LEFTSCROLLBAR" },
                /*{ WS_EX_RIGHTSCROLLBAR,  "RIGHTSCROLLBAR" }, = 0 */
                { WS_EX_CONTROLPARENT,   "CONTROLPARENT" },
                { WS_EX_STATICEDGE,      "STATICEDGE" },
                { WS_EX_APPWINDOW,       "APPWINDOW" },
                { WS_EX_LAYERED,         "LAYERED" },
                { WS_EX_NOINHERITLAYOUT, "NOINHERITLAYOUT" },
                { WS_EX_LAYOUTRTL,       "LAYOUTRTL" },
                { WS_EX_COMPOSITED,      "COMPOSITED" },
                { WS_EX_NOACTIVATE,      "NOACTIVATE" },
            };
            vmsvga3dInfoU32Flags(pHlp, Info.dwExStyle, "", g_aExStyles, RT_ELEMENTS(g_aExStyles));
            pHlp->pfnPrintf(pHlp, "\n");

            pHlp->pfnPrintf(pHlp, "     Window Status: %#x\n", Info.dwWindowStatus);
            if (Info.cxWindowBorders || Info.cyWindowBorders)
                pHlp->pfnPrintf(pHlp, "     Borders:       cx=%u, cy=%u\n", Info.cxWindowBorders, Info.cyWindowBorders);
            pHlp->pfnPrintf(pHlp, "     Window Type:   %#x\n", Info.atomWindowType);
            pHlp->pfnPrintf(pHlp, "     Creator Ver:   %#x\n", Info.wCreatorVersion);
        }
        else
            pHlp->pfnPrintf(pHlp, "     GetWindowInfo: last error %d\n", GetLastError());
    }

#elif defined(RT_OS_DARWIN)
    int rc = ExplicitlyLoadVBoxSVGA3DObjC(false /*fResolveAllImports*/, NULL /*pErrInfo*/);
    if (RT_SUCCESS(rc))
        vmsvga3dCocoaViewInfo(pHlp, (NativeNSViewRef)(uintptr_t)idHostWindow);
    else
        pHlp->pfnPrintf(pHlp, "    Windows info:   vmsvga3dCocoaViewInfo failed to load (%Rrc)\n", rc);

#else
    RT_NOREF(idHostWindow);
    pHlp->pfnPrintf(pHlp, "    Windows info:   Not implemented on this platform\n");
#endif
}


/**
 * Looks up an enum value in a translation table.
 *
 * @returns The value name.
 * @param   iValue              The value to name.
 * @param   pEnumMap            Enum value to string mapping.
 */
const char *vmsvgaLookupEnum(int32_t iValue, PCVMSVGAINFOENUMMAP pEnumMap)
{
    PCVMSVGAINFOENUM paValues = pEnumMap->paValues;

#ifdef VBOX_STRICT
    /*
     * Check that it's really sorted, or the binary lookup won't work right.
     */
    if (!*pEnumMap->pfAsserted)
    {
        *pEnumMap->pfAsserted = true;
        for (uint32_t i = 1; i < pEnumMap->cValues; i++)
            Assert(paValues[i - 1].iValue <= paValues[i].iValue);
    }
#endif

    /*
     * Binary search
     */
    uint32_t iStart = 0;
    uint32_t iEnd   = (uint32_t)pEnumMap->cValues;
    for (;;)
    {
        uint32_t i = iStart + (iEnd - iStart) / 2;
        if (iValue < paValues[i].iValue)
        {
            if (i > iStart)
                iEnd = i;
            else
                break;
        }
        else if (iValue > paValues[i].iValue)
        {
            i++;
            if (i < iEnd)
                iStart = i;
            else
                break;
        }
        else
            return paValues[i].pszName;
    }
    return NULL;
}


/**
 * Formats an enum value as a string, sparse mapping table.
 *
 * @returns pszBuffer.
 * @param   pszBuffer           The output buffer.
 * @param   cbBuffer            The size of the output buffer.
 * @param   pszName             The variable name, optional.
 * @param   iValue              The enum value.
 * @param   fPrefix             Whether to prepend the prefix or not.
 * @param   pEnumMap            Enum value to string mapping.
 */
char *vmsvgaFormatEnumValueEx(char *pszBuffer, size_t cbBuffer, const char *pszName, int32_t iValue,
                              bool fPrefix, PCVMSVGAINFOENUMMAP pEnumMap)
{
    const char *pszValueName = vmsvgaLookupEnum(iValue, pEnumMap);
    const char *pszPrefix    = fPrefix ? pEnumMap->pszPrefix : "";
    if (pszValueName)
    {
        if (pszName)
            RTStrPrintf(pszBuffer, cbBuffer, "%s = %s%s (%#x)", pszName, pszPrefix, pszValueName, iValue);
        else
            RTStrPrintf(pszBuffer, cbBuffer, "%s%s (%#x)", pszPrefix, pszValueName, iValue);
        return pszBuffer;
    }

    if (pszName)
        RTStrPrintf(pszBuffer, cbBuffer, "%s = %sUNKNOWN_%d (%#x)", pszName, pszPrefix, iValue, iValue);
    else
        RTStrPrintf(pszBuffer, cbBuffer, "%sUNKNOWN_%d (%#x)", pszPrefix, iValue, iValue);
    return pszBuffer;
}


/**
 * Formats an enum value as a string.
 *
 * @returns pszBuffer.
 * @param   pszBuffer           The output buffer.
 * @param   cbBuffer            The size of the output buffer.
 * @param   pszName             The variable name, optional.
 * @param   uValue              The enum value.
 * @param   pszPrefix           The prefix of the enum values.  Empty string if
 *                              none.  This helps reduce the memory footprint
 *                              as well as the source code size.
 * @param   papszValues         One to one string mapping of the enum values.
 * @param   cValues             The number of values in the mapping.
 */
char *vmsvgaFormatEnumValue(char *pszBuffer, size_t cbBuffer, const char *pszName, uint32_t uValue,
                            const char *pszPrefix, const char * const *papszValues, size_t cValues)
{
    if (uValue < cValues)
    {
        if (pszName)
            RTStrPrintf(pszBuffer, cbBuffer, "%s = %s%s (%#x)", pszName, pszPrefix, papszValues[uValue], uValue);
        else
            RTStrPrintf(pszBuffer, cbBuffer, "%s%s (%#x)", pszPrefix, papszValues[uValue], uValue);
    }
    else
    {
        if (pszName)
            RTStrPrintf(pszBuffer, cbBuffer, "%s = %sUNKNOWN_%d (%#x)", pszName, pszPrefix, uValue, uValue);
        else
            RTStrPrintf(pszBuffer, cbBuffer, "%sUNKNOWN_%d (%#x)", pszPrefix, uValue, uValue);
    }
    return pszBuffer;
}


/**
 * DBGF info printer for vmsvga3dAsciiPrint.
 *
 * @param   pszLine             The line to print.
 * @param   pvUser              The debug info helpers.
 */
DECLCALLBACK(void) vmsvga3dAsciiPrintlnInfo(const char *pszLine, void *pvUser)
{
    PCDBGFINFOHLP pHlp = (PCDBGFINFOHLP)pvUser;
    pHlp->pfnPrintf(pHlp, ">%s<\n", pszLine);
}


/**
 * Log printer for vmsvga3dAsciiPrint.
 *
 * @param   pszLine             The line to print.
 * @param   pvUser              Ignored.
 */
DECLCALLBACK(void) vmsvga3dAsciiPrintlnLog(const char *pszLine, void *pvUser)
{
    size_t cch = strlen(pszLine);
    while (cch > 0 && pszLine[cch - 1] == ' ')
        cch--;
    RTLogPrintf("%.*s\n", cch, pszLine);
    NOREF(pvUser);
}


void vmsvga3dAsciiPrint(PFMVMSVGAASCIIPRINTLN pfnPrintLine, void *pvUser, void const *pvImage, size_t cbImage,
                        uint32_t cx, uint32_t cy, uint32_t cbScanline, SVGA3dSurfaceFormat enmFormat, bool fInvY,
                        uint32_t cchMaxX, uint32_t cchMaxY)
{
    RT_NOREF(cbImage);

    /*
     * Skip stuff we can't or won't need to handle.
     */
    if (!cx || !cy)
        return;
    switch (enmFormat)
    {
        /* Compressed. */
        case SVGA3D_DXT1:
        case SVGA3D_DXT2:
        case SVGA3D_DXT3:
        case SVGA3D_DXT4:
        case SVGA3D_DXT5:
            return;
        /* Generic. */
        case SVGA3D_BUFFER:
            return;
        default:
            break; /* ok */
    }

    /*
     * Figure the pixel conversion factors.
     */
    uint32_t cxPerChar = cx / cchMaxX + 1;
    uint32_t cyPerChar = cy / cchMaxY + 1;
    /** @todo try keep aspect...   */
    uint32_t const cchLine = (cx + cxPerChar - 1) / cxPerChar;
    uint32_t const cbSrcPixel = vmsvga3dSurfaceFormatSize(enmFormat);

    /*
     * The very simple conversion we're doing in this function is based on
     * mapping a block of converted pixels to an ASCII character of similar
     * weigth.  We do that by summing up all the 8-bit gray scale pixels in
     * that block, applying a conversion factor and getting an index into an
     * array of increasingly weighty characters.
     */
    static const char       s_szPalette[] = "   ..`',:;icodxkO08XNWM";
    static const uint32_t   s_cchPalette  = sizeof(s_szPalette) - 1;
    uint32_t const          cPixelsWeightPerChar = cxPerChar * cyPerChar * 256;

    /*
     * Do the work
     */
    uint32_t *pauScanline = (uint32_t *)RTMemTmpAllocZ(sizeof(pauScanline[0]) * cchLine + cchLine + 1);
    if (!pauScanline)
        return;
    char *pszLine = (char *)&pauScanline[cchLine];
    RTCPTRUNION uSrc;
    uSrc.pv              = pvImage;
    if (fInvY)
        uSrc.pu8 += (cy - 1) * cbScanline;
    uint32_t cyLeft = cy;
    uint32_t cyLeftInScanline = cyPerChar;
    bool     fHitFormatAssert = false;
    for (;;)
    {
        /*
         * Process the scanline.  This is tedious because of all the
         * different formats.  We generally ignore alpha, unless it's
         * all we've got to work with.
         * Color to 8-bit grayscale conversion is done by averaging.
         */
#define CONVERT_SCANLINE(a_RdExpr, a_AddExpr) \
            do { \
                for (uint32_t xSrc = 0, xDst = 0, cxLeftInChar = cxPerChar; xSrc < cx; xSrc++) \
                { \
                    a_RdExpr; \
                    pauScanline[xDst] += (a_AddExpr) & 0xff; \
                    Assert(pauScanline[xDst] <= cPixelsWeightPerChar); \
                    if (--cxLeftInChar == 0) \
                    { \
                        xDst++; \
                        cxLeftInChar = cxPerChar; \
                    } \
                } \
            } while (0)

        switch (enmFormat)
        {
            /* Unsigned RGB and super/subsets. */
            case SVGA3D_X8R8G8B8:
            case SVGA3D_A8R8G8B8:
                CONVERT_SCANLINE(uint32_t const u32Tmp = uSrc.pu32[xSrc],
                                 (  ( u32Tmp        & 0xff) /* B */
                                  + ((u32Tmp >>  8) & 0xff) /* G */
                                  + ((u32Tmp >> 16) & 0xff) /* R */) / 3);
                break;
            case SVGA3D_R5G6B5:
                CONVERT_SCANLINE(uint16_t const u16Tmp = uSrc.pu16[xSrc],
                                 ( ( u16Tmp         & 0x1f) * 8
                                 + ((u16Tmp >>  5)  & 0x3f) * 4
                                 + ( u16Tmp >> 11)          * 8 ) / 3 );
                break;
            case SVGA3D_X1R5G5B5:
            case SVGA3D_A1R5G5B5:
                CONVERT_SCANLINE(uint16_t const u16Tmp = uSrc.pu16[xSrc],
                                 (  ( u16Tmp        & 0x1f) * 8
                                  + ((u16Tmp >> 5)  & 0x1f) * 8
                                  + ((u16Tmp >> 10) & 0x1f) * 8) / 3 );
                break;
            case SVGA3D_A4R4G4B4:
                CONVERT_SCANLINE(uint16_t const u16Tmp = uSrc.pu16[xSrc],
                                 (  ( u16Tmp        & 0xf) * 16
                                  + ((u16Tmp >> 4)  & 0xf) * 16
                                  + ((u16Tmp >> 8)  & 0xf) * 16) / 3 );
                break;
            case SVGA3D_A16B16G16R16:
                CONVERT_SCANLINE(uint64_t const u64Tmp = uSrc.pu64[xSrc],
                                 (  ((u64Tmp >>  8) & 0xff) /* R */
                                  + ((u64Tmp >> 24) & 0xff) /* G */
                                  + ((u64Tmp >> 40) & 0xff) /* B */ ) / 3);
                break;
            case SVGA3D_A2R10G10B10:
                CONVERT_SCANLINE(uint32_t const u32Tmp = uSrc.pu32[xSrc],
                                 (  ( u32Tmp        & 0x3ff) /* B */
                                  + ((u32Tmp >> 10) & 0x3ff) /* G */
                                  + ((u32Tmp >> 20) & 0x3ff) /* R */ ) / (3 * 4));
                break;
            case SVGA3D_G16R16:
                CONVERT_SCANLINE(uint32_t const u32Tmp = uSrc.pu32[xSrc],
                                 (  (u32Tmp & 0xffff) /* R */
                                  + (u32Tmp >>   16 ) /* G */) / 0x200);
                break;

            /* Depth. */
            case SVGA3D_Z_D32:
                CONVERT_SCANLINE(uint32_t const u32Tmp = ~((uSrc.pu32[xSrc] >> 1) | uSrc.pu32[xSrc]) & UINT32_C(0x44444444),
                                   (( u32Tmp >> (2 - 0)) & RT_BIT_32(0))
                                 | ((u32Tmp >> ( 6 - 1)) & RT_BIT_32(1))
                                 | ((u32Tmp >> (10 - 2)) & RT_BIT_32(2))
                                 | ((u32Tmp >> (14 - 3)) & RT_BIT_32(3))
                                 | ((u32Tmp >> (18 - 4)) & RT_BIT_32(4))
                                 | ((u32Tmp >> (22 - 5)) & RT_BIT_32(5))
                                 | ((u32Tmp >> (26 - 6)) & RT_BIT_32(6))
                                 | ((u32Tmp >> (30 - 7)) & RT_BIT_32(7)) );
                break;
            case SVGA3D_Z_D16:
                CONVERT_SCANLINE(uint16_t const u16Tmp = ~uSrc.pu16[xSrc],
                                   ((u16Tmp >> ( 1 - 0)) & RT_BIT_32(0))
                                 | ((u16Tmp >> ( 3 - 1)) & RT_BIT_32(1))
                                 | ((u16Tmp >> ( 5 - 2)) & RT_BIT_32(2))
                                 | ((u16Tmp >> ( 7 - 3)) & RT_BIT_32(3))
                                 | ((u16Tmp >> ( 9 - 4)) & RT_BIT_32(4))
                                 | ((u16Tmp >> (11 - 5)) & RT_BIT_32(5))
                                 | ((u16Tmp >> (13 - 6)) & RT_BIT_32(6))
                                 | ((u16Tmp >> (15 - 7)) & RT_BIT_32(7)) );
                break;
            case SVGA3D_Z_D24S8:
                CONVERT_SCANLINE(uint32_t const u32Tmp = uSrc.pu32[xSrc],
                                   (  u32Tmp        & 0xff) /* stencile */
                                 | ((~u32Tmp >> 18) & 0x3f));
                break;
            case SVGA3D_Z_D15S1:
                CONVERT_SCANLINE(uint16_t const u16Tmp = uSrc.pu16[xSrc],
                                   ( (u16Tmp & 0x01) << 7) /* stencile */
                                 | ((~u16Tmp >> 8) & 0x7f));
                break;

            /* Pure alpha. */
            case SVGA3D_ALPHA8:
                CONVERT_SCANLINE(RT_NOTHING, uSrc.pu8[xSrc]);
                break;

            /* Luminance */
            case SVGA3D_LUMINANCE8:
                CONVERT_SCANLINE(RT_NOTHING, uSrc.pu8[xSrc]);
                break;
            case SVGA3D_LUMINANCE4_ALPHA4:
                CONVERT_SCANLINE(RT_NOTHING, uSrc.pu8[xSrc] & 0xf0);
                break;
            case SVGA3D_LUMINANCE16:
                CONVERT_SCANLINE(RT_NOTHING, uSrc.pu16[xSrc] >> 8);
                break;
            case SVGA3D_LUMINANCE8_ALPHA8:
                CONVERT_SCANLINE(RT_NOTHING, uSrc.pu16[xSrc] >> 8);
                break;

            /* Not supported. */
            case SVGA3D_DXT1:
            case SVGA3D_DXT2:
            case SVGA3D_DXT3:
            case SVGA3D_DXT4:
            case SVGA3D_DXT5:
            case SVGA3D_BUFFER:
                AssertFailedBreak();

            /* Not considered for implementation yet. */
            case SVGA3D_BUMPU8V8:
            case SVGA3D_BUMPL6V5U5:
            case SVGA3D_BUMPX8L8V8U8:
            case SVGA3D_BUMPL8V8U8:
            case SVGA3D_ARGB_S10E5:
            case SVGA3D_ARGB_S23E8:
            case SVGA3D_V8U8:
            case SVGA3D_Q8W8V8U8:
            case SVGA3D_CxV8U8:
            case SVGA3D_X8L8V8U8:
            case SVGA3D_A2W10V10U10:
            case SVGA3D_R_S10E5:
            case SVGA3D_R_S23E8:
            case SVGA3D_RG_S10E5:
            case SVGA3D_RG_S23E8:
            case SVGA3D_Z_D24X8:
            case SVGA3D_V16U16:
            case SVGA3D_UYVY:
            case SVGA3D_YUY2:
            case SVGA3D_NV12:
            case SVGA3D_AYUV:
            case SVGA3D_BC4_UNORM:
            case SVGA3D_BC5_UNORM:
            case SVGA3D_Z_DF16:
            case SVGA3D_Z_DF24:
            case SVGA3D_Z_D24S8_INT:
                if (!fHitFormatAssert)
                {
                    AssertMsgFailed(("%s is not implemented\n", vmsvgaLookupEnum((int)enmFormat, &g_SVGA3dSurfaceFormat2String)));
                    fHitFormatAssert = true;
                }
                RT_FALL_THRU();
            default:
                /* Lazy programmer fallbacks. */
                if (cbSrcPixel == 4)
                    CONVERT_SCANLINE(uint32_t const u32Tmp = uSrc.pu32[xSrc],
                                     (  ( u32Tmp        & 0xff)
                                      + ((u32Tmp >>  8) & 0xff)
                                      + ((u32Tmp >> 16) & 0xff)
                                      + ((u32Tmp >> 24) & 0xff) ) / 4);
                else if (cbSrcPixel == 3)
                    CONVERT_SCANLINE(RT_NOTHING,
                                     (  (uint32_t)uSrc.pu8[xSrc * 4]
                                      + (uint32_t)uSrc.pu8[xSrc * 4 + 1]
                                      + (uint32_t)uSrc.pu8[xSrc * 4 + 2] ) / 3);
                else if (cbSrcPixel == 2)
                    CONVERT_SCANLINE(uint16_t const u16Tmp = uSrc.pu16[xSrc],
                                     (  ( u16Tmp        & 0xf)
                                      + ((u16Tmp >>  4) & 0xf)
                                      + ((u16Tmp >>  8) & 0xf)
                                      + ((u16Tmp >> 12) & 0xf) ) * 4 /* mul 16 div 4 */ );
                else if (cbSrcPixel == 1)
                    CONVERT_SCANLINE(RT_NOTHING, uSrc.pu8[xSrc]);
                else
                    AssertFailed();
                break;

        }

        /*
         * Print we've reached the end of a block in y direction or if we're at
         * the end of the image.
         */
        cyLeft--;
        if (--cyLeftInScanline == 0 || cyLeft == 0)
        {
            for (uint32_t i = 0; i < cchLine; i++)
            {
                uint32_t off = pauScanline[i] * s_cchPalette / cPixelsWeightPerChar; Assert(off < s_cchPalette);
                pszLine[i] = s_szPalette[off < sizeof(s_szPalette) - 1 ? off : sizeof(s_szPalette) - 1];
            }
            pszLine[cchLine] = '\0';
            pfnPrintLine(pszLine, pvUser);

            if (!cyLeft)
                break;
            cyLeftInScanline = cyPerChar;
            RT_BZERO(pauScanline, sizeof(pauScanline[0]) * cchLine);
        }

        /*
         * Advance.
         */
        if (!fInvY)
            uSrc.pu8 += cbScanline;
        else
            uSrc.pu8 -= cbScanline;
    }
}



/**
 * Formats a SVGA3dRenderState structure as a string.
 *
 * @returns pszBuffer.
 * @param   pszBuffer       Output string buffer.
 * @param   cbBuffer        Size of output buffer.
 * @param   pRenderState    The SVGA3d render state to format.
 */
char *vmsvga3dFormatRenderState(char *pszBuffer, size_t cbBuffer, SVGA3dRenderState const *pRenderState)
{
    /*
     * List of render state names with type prefix.
     *
     * First char in the name is a type indicator:
     *      - '*' = requires special handling.
     *      - 'f' = SVGA3dbool
     *      - 'd' = uint32_t
     *      - 'r' = float
     *      - 'b' = SVGA3dBlendOp
     *      - 'c' = SVGA3dColor, SVGA3dColorMask
     *      - 'e' = SVGA3dBlendEquation
     *      - 'm' = SVGA3dColorMask
     *      - 'p' = SVGA3dCmpFunc
     *      - 's' = SVGA3dStencilOp
     *      - 'v' = SVGA3dVertexMaterial
     *      - 'w' = SVGA3dWrapFlags
     */
    static const char * const s_apszRenderStateNamesAndType[] =
    {
       "*" "INVALID",                       /*  invalid  */
       "f" "ZENABLE",                       /*  SVGA3dBool  */
       "f" "ZWRITEENABLE",                  /*  SVGA3dBool  */
       "f" "ALPHATESTENABLE",               /*  SVGA3dBool  */
       "f" "DITHERENABLE",                  /*  SVGA3dBool  */
       "f" "BLENDENABLE",                   /*  SVGA3dBool  */
       "f" "FOGENABLE",                     /*  SVGA3dBool  */
       "f" "SPECULARENABLE",                /*  SVGA3dBool  */
       "f" "STENCILENABLE",                 /*  SVGA3dBool  */
       "f" "LIGHTINGENABLE",                /*  SVGA3dBool  */
       "f" "NORMALIZENORMALS",              /*  SVGA3dBool  */
       "f" "POINTSPRITEENABLE",             /*  SVGA3dBool  */
       "f" "POINTSCALEENABLE",              /*  SVGA3dBool  */
       "x" "STENCILREF",                    /*  uint32_t  */
       "x" "STENCILMASK",                   /*  uint32_t  */
       "x" "STENCILWRITEMASK",              /*  uint32_t  */
       "r" "FOGSTART",                      /*  float  */
       "r" "FOGEND",                        /*  float  */
       "r" "FOGDENSITY",                    /*  float  */
       "r" "POINTSIZE",                     /*  float  */
       "r" "POINTSIZEMIN",                  /*  float  */
       "r" "POINTSIZEMAX",                  /*  float  */
       "r" "POINTSCALE_A",                  /*  float  */
       "r" "POINTSCALE_B",                  /*  float  */
       "r" "POINTSCALE_C",                  /*  float  */
       "c" "FOGCOLOR",                      /*  SVGA3dColor  */
       "c" "AMBIENT",                       /*  SVGA3dColor  */
       "*" "CLIPPLANEENABLE",               /*  SVGA3dClipPlanes  */
       "*" "FOGMODE",                       /*  SVGA3dFogMode  */
       "*" "FILLMODE",                      /*  SVGA3dFillMode  */
       "*" "SHADEMODE",                     /*  SVGA3dShadeMode  */
       "*" "LINEPATTERN",                   /*  SVGA3dLinePattern  */
       "b" "SRCBLEND",                      /*  SVGA3dBlendOp  */
       "b" "DSTBLEND",                      /*  SVGA3dBlendOp  */
       "e" "BLENDEQUATION",                 /*  SVGA3dBlendEquation  */
       "*" "CULLMODE",                      /*  SVGA3dFace  */
       "p" "ZFUNC",                         /*  SVGA3dCmpFunc  */
       "p" "ALPHAFUNC",                     /*  SVGA3dCmpFunc  */
       "p" "STENCILFUNC",                   /*  SVGA3dCmpFunc  */
       "s" "STENCILFAIL",                   /*  SVGA3dStencilOp  */
       "s" "STENCILZFAIL",                  /*  SVGA3dStencilOp  */
       "s" "STENCILPASS",                   /*  SVGA3dStencilOp  */
       "r" "ALPHAREF",                      /*  float  */
       "*" "FRONTWINDING",                  /*  SVGA3dFrontWinding  */
       "*" "COORDINATETYPE",                /*  SVGA3dCoordinateType  */
       "r" "ZBIAS",                         /*  float  */
       "f" "RANGEFOGENABLE",                /*  SVGA3dBool  */
       "c" "COLORWRITEENABLE",              /*  SVGA3dColorMask  */
       "f" "VERTEXMATERIALENABLE",          /*  SVGA3dBool  */
       "v" "DIFFUSEMATERIALSOURCE",         /*  SVGA3dVertexMaterial  */
       "v" "SPECULARMATERIALSOURCE",        /*  SVGA3dVertexMaterial  */
       "v" "AMBIENTMATERIALSOURCE",         /*  SVGA3dVertexMaterial  */
       "v" "EMISSIVEMATERIALSOURCE",        /*  SVGA3dVertexMaterial  */
       "c" "TEXTUREFACTOR",                 /*  SVGA3dColor  */
       "f" "LOCALVIEWER",                   /*  SVGA3dBool  */
       "f" "SCISSORTESTENABLE",             /*  SVGA3dBool  */
       "c" "BLENDCOLOR",                    /*  SVGA3dColor  */
       "f" "STENCILENABLE2SIDED",           /*  SVGA3dBool  */
       "p" "CCWSTENCILFUNC",                /*  SVGA3dCmpFunc  */
       "s" "CCWSTENCILFAIL",                /*  SVGA3dStencilOp  */
       "s" "CCWSTENCILZFAIL",               /*  SVGA3dStencilOp  */
       "s" "CCWSTENCILPASS",                /*  SVGA3dStencilOp  */
       "*" "VERTEXBLEND",                   /*  SVGA3dVertexBlendFlags  */
       "r" "SLOPESCALEDEPTHBIAS",           /*  float  */
       "r" "DEPTHBIAS",                     /*  float  */
       "r" "OUTPUTGAMMA",                   /*  float  */
       "f" "ZVISIBLE",                      /*  SVGA3dBool  */
       "f" "LASTPIXEL",                     /*  SVGA3dBool  */
       "f" "CLIPPING",                      /*  SVGA3dBool  */
       "w" "WRAP0",                         /*  SVGA3dWrapFlags  */
       "w" "WRAP1",                         /*  SVGA3dWrapFlags  */
       "w" "WRAP2",                         /*  SVGA3dWrapFlags  */
       "w" "WRAP3",                         /*  SVGA3dWrapFlags  */
       "w" "WRAP4",                         /*  SVGA3dWrapFlags  */
       "w" "WRAP5",                         /*  SVGA3dWrapFlags  */
       "w" "WRAP6",                         /*  SVGA3dWrapFlags  */
       "w" "WRAP7",                         /*  SVGA3dWrapFlags  */
       "w" "WRAP8",                         /*  SVGA3dWrapFlags  */
       "w" "WRAP9",                         /*  SVGA3dWrapFlags  */
       "w" "WRAP10",                        /*  SVGA3dWrapFlags  */
       "w" "WRAP11",                        /*  SVGA3dWrapFlags  */
       "w" "WRAP12",                        /*  SVGA3dWrapFlags  */
       "w" "WRAP13",                        /*  SVGA3dWrapFlags  */
       "w" "WRAP14",                        /*  SVGA3dWrapFlags  */
       "w" "WRAP15",                        /*  SVGA3dWrapFlags  */
       "f" "MULTISAMPLEANTIALIAS",          /*  SVGA3dBool  */
       "x" "MULTISAMPLEMASK",               /*  uint32_t  */
       "f" "INDEXEDVERTEXBLENDENABLE",      /*  SVGA3dBool  */
       "r" "TWEENFACTOR",                   /*  float  */
       "f" "ANTIALIASEDLINEENABLE",         /*  SVGA3dBool  */
       "c" "COLORWRITEENABLE1",             /*  SVGA3dColorMask  */
       "c" "COLORWRITEENABLE2",             /*  SVGA3dColorMask  */
       "c" "COLORWRITEENABLE3",             /*  SVGA3dColorMask  */
       "f" "SEPARATEALPHABLENDENABLE",      /*  SVGA3dBool  */
       "b" "SRCBLENDALPHA",                 /*  SVGA3dBlendOp  */
       "b" "DSTBLENDALPHA",                 /*  SVGA3dBlendOp  */
       "e" "BLENDEQUATIONALPHA",            /*  SVGA3dBlendEquation  */
       "*" "TRANSPARENCYANTIALIAS",         /*  SVGA3dTransparencyAntialiasType  */
       "f" "LINEAA",                        /*  SVGA3dBool  */
       "r" "LINEWIDTH",                     /*  float  */
    };

    uint32_t iState = pRenderState->state;
    if (iState != SVGA3D_RS_INVALID)
    {
        if (iState < RT_ELEMENTS(s_apszRenderStateNamesAndType))
        {
            const char *pszName = s_apszRenderStateNamesAndType[iState];
            char const  chType  = *pszName++;

            union
            {
               uint32_t  u;
               float     r;
               SVGA3dColorMask Color;
            } uValue;
            uValue.u = pRenderState->uintValue;

            switch (chType)
            {
                case 'f':
                    if (uValue.u == 0)
                        RTStrPrintf(pszBuffer, cbBuffer, "%s = false", pszName);
                    else if (uValue.u == 1)
                        RTStrPrintf(pszBuffer, cbBuffer, "%s = true", pszName);
                    else
                        RTStrPrintf(pszBuffer, cbBuffer, "%s = true (%#x)", pszName, uValue.u);
                    break;
                case 'x':
                    RTStrPrintf(pszBuffer, cbBuffer, "%s = %#x (%d)", pszName, uValue.u, uValue.u);
                    break;
                case 'r':
                    RTStrPrintf(pszBuffer, cbBuffer, "%s = %d.%06u (%#x)",
                                pszName, (int)uValue.r, (unsigned)(uValue.r * 1000000) % 1000000U, uValue.u);
                    break;
                case 'c': //SVGA3dColor, SVGA3dColorMask
                    RTStrPrintf(pszBuffer, cbBuffer, "%s = RGBA(%d,%d,%d,%d) (%#x)", pszName,
                                uValue.Color.s.red, uValue.Color.s.green, uValue.Color.s.blue, uValue.Color.s.alpha, uValue.u);
                    break;
                case 'w': //SVGA3dWrapFlags
                    RTStrPrintf(pszBuffer, cbBuffer, "%s = %#x%s", pszName, uValue.u,
                                uValue.u <= SVGA3D_WRAPCOORD_ALL ? " (out of bounds" : "");
                    break;
                default:
                    AssertFailed();  RT_FALL_THRU();
                case 'b': //SVGA3dBlendOp
                case 'e': //SVGA3dBlendEquation
                case 'p': //SVGA3dCmpFunc
                case 's': //SVGA3dStencilOp
                case 'v': //SVGA3dVertexMaterial
                case '*':
                    RTStrPrintf(pszBuffer, cbBuffer, "%s = %#x", pszName, uValue.u);
                    break;
            }
        }
        else
            RTStrPrintf(pszBuffer, cbBuffer, "UNKNOWN_%d_%#x = %#x", iState, iState, pRenderState->uintValue);
    }
    else
        RTStrPrintf(pszBuffer, cbBuffer, "INVALID");
    return pszBuffer;
}


/**
 * Formats a SVGA3dTextureState structure as a string.
 *
 * @returns pszBuffer.
 * @param   pszBuffer       Output string buffer.
 * @param   cbBuffer        Size of output buffer.
 * @param   pTextureState   The SVGA3d texture state to format.
 */
char *vmsvga3dFormatTextureState(char *pszBuffer, size_t cbBuffer, SVGA3dTextureState const *pTextureState)
{
    static const char * const s_apszTextureStateNamesAndType[] =
    {
        "*" "INVALID",                      /*  invalid  */
        "x" "BIND_TEXTURE",                 /*  SVGA3dSurfaceId  */
        "m" "COLOROP",                      /*  SVGA3dTextureCombiner  */
        "a" "COLORARG1",                    /*  SVGA3dTextureArgData  */
        "a" "COLORARG2",                    /*  SVGA3dTextureArgData  */
        "m" "ALPHAOP",                      /*  SVGA3dTextureCombiner  */
        "a" "ALPHAARG1",                    /*  SVGA3dTextureArgData  */
        "a" "ALPHAARG2",                    /*  SVGA3dTextureArgData  */
        "e" "ADDRESSU",                     /*  SVGA3dTextureAddress  */
        "e" "ADDRESSV",                     /*  SVGA3dTextureAddress  */
        "l" "MIPFILTER",                    /*  SVGA3dTextureFilter  */
        "l" "MAGFILTER",                    /*  SVGA3dTextureFilter  */
        "m" "MINFILTER",                    /*  SVGA3dTextureFilter  */
        "c" "BORDERCOLOR",                  /*  SVGA3dColor  */
        "r" "TEXCOORDINDEX",                /*  uint32_t  */
        "t" "TEXTURETRANSFORMFLAGS",        /*  SVGA3dTexTransformFlags  */
        "g" "TEXCOORDGEN",                  /*  SVGA3dTextureCoordGen  */
        "r" "BUMPENVMAT00",                 /*  float  */
        "r" "BUMPENVMAT01",                 /*  float  */
        "r" "BUMPENVMAT10",                 /*  float  */
        "r" "BUMPENVMAT11",                 /*  float  */
        "x" "TEXTURE_MIPMAP_LEVEL",         /*  uint32_t  */
        "r" "TEXTURE_LOD_BIAS",             /*  float  */
        "x" "TEXTURE_ANISOTROPIC_LEVEL",    /*  uint32_t  */
        "e" "ADDRESSW",                     /*  SVGA3dTextureAddress  */
        "r" "GAMMA",                        /*  float  */
        "r" "BUMPENVLSCALE",                /*  float  */
        "r" "BUMPENVLOFFSET",               /*  float  */
        "a" "COLORARG0",                    /*  SVGA3dTextureArgData  */
        "a" "ALPHAARG0"                     /*  SVGA3dTextureArgData */
    };

    /*
     * Format the stage first.
     */
    char  *pszRet    = pszBuffer;
    size_t cchPrefix = RTStrPrintf(pszBuffer, cbBuffer, "[%u] ", pTextureState->stage);
    if (cchPrefix < cbBuffer)
    {
        cbBuffer  -= cchPrefix;
        pszBuffer += cchPrefix;
    }
    else
        cbBuffer = 0;

    /*
     * Format the name and value.
     */
    uint32_t iName = pTextureState->name;
    if (iName != SVGA3D_TS_INVALID)
    {
        if (iName < RT_ELEMENTS(s_apszTextureStateNamesAndType))
        {
            const char *pszName = s_apszTextureStateNamesAndType[iName];
            char        chType  = *pszName++;

            union
            {
               uint32_t  u;
               float     r;
               SVGA3dColorMask Color;
            } uValue;
            uValue.u = pTextureState->value;

            switch (chType)
            {
                case 'x':
                    RTStrPrintf(pszBuffer, cbBuffer, "%s = %#x (%d)", pszName, uValue.u, uValue.u);
                    break;

                case 'r':
                    RTStrPrintf(pszBuffer, cbBuffer, "%s = %d.%06u (%#x)",
                                pszName, (int)uValue.r, (unsigned)(uValue.r * 1000000) % 1000000U, uValue.u);
                    break;

                case 'a': //SVGA3dTextureArgData
                {
                    static const char * const s_apszValues[] =
                    {
                        "INVALID", "CONSTANT", "PREVIOUS", "DIFFUSE", "TEXTURE", "SPECULAR"
                    };
                    vmsvgaFormatEnumValue(pszBuffer, cbBuffer, pszName, uValue.u,
                                          "SVGA3D_TA_", s_apszValues, RT_ELEMENTS(s_apszValues));
                    break;
                }

                case 'c': //SVGA3dColor, SVGA3dColorMask
                    RTStrPrintf(pszBuffer, cbBuffer, "%s = RGBA(%d,%d,%d,%d) (%#x)", pszName,
                                uValue.Color.s.red, uValue.Color.s.green, uValue.Color.s.blue, uValue.Color.s.alpha, uValue.u);
                    break;

                case 'e': //SVGA3dTextureAddress
                {
                    static const char * const s_apszValues[] =
                    {
                        "INVALID", "WRAP", "MIRROR", "CLAMP", "BORDER", "MIRRORONCE", "EDGE",
                    };
                    vmsvgaFormatEnumValue(pszBuffer, cbBuffer, pszName, uValue.u,
                                          "SVGA3D_TEX_ADDRESS_", s_apszValues, RT_ELEMENTS(s_apszValues));
                    break;
                }

                case 'l': //SVGA3dTextureFilter
                {
                    static const char * const s_apszValues[] =
                    {
                        "NONE", "NEAREST", "LINEAR",  "ANISOTROPIC", "FLATCUBIC", "GAUSSIANCUBIC", "PYRAMIDALQUAD", "GAUSSIANQUAD",
                    };
                    vmsvgaFormatEnumValue(pszBuffer, cbBuffer, pszName, uValue.u,
                                          "SVGA3D_TEX_FILTER_", s_apszValues, RT_ELEMENTS(s_apszValues));
                    break;
                }

                case 'g': //SVGA3dTextureCoordGen
                {
                    static const char * const s_apszValues[] =
                    {
                        "OFF", "EYE_POSITION", "EYE_NORMAL", "REFLECTIONVECTOR", "SPHERE",
                    };
                    vmsvgaFormatEnumValue(pszBuffer, cbBuffer, pszName, uValue.u,
                                          "SVGA3D_TEXCOORD_GEN_", s_apszValues, RT_ELEMENTS(s_apszValues));
                    break;
                }

                case 'm': //SVGA3dTextureCombiner
                {
                    static const char * const s_apszValues[] =
                    {
                        "INVALID", "DISABLE", "SELECTARG1", "SELECTARG2", "MODULATE", "ADD", "ADDSIGNED", "SUBTRACT",
                        "BLENDTEXTUREALPHA", "BLENDDIFFUSEALPHA", "BLENDCURRENTALPHA", "BLENDFACTORALPHA", "MODULATE2X",
                        "MODULATE4X", "DSDT", "DOTPRODUCT3", "BLENDTEXTUREALPHAPM", "ADDSIGNED2X", "ADDSMOOTH", "PREMODULATE",
                        "MODULATEALPHA_ADDCOLOR", "MODULATECOLOR_ADDALPHA", "MODULATEINVALPHA_ADDCOLOR",
                        "MODULATEINVCOLOR_ADDALPHA", "BUMPENVMAPLUMINANCE", "MULTIPLYADD", "LERP",
                    };
                    vmsvgaFormatEnumValue(pszBuffer, cbBuffer, pszName, uValue.u,
                                          "SVGA3D_TC_", s_apszValues, RT_ELEMENTS(s_apszValues));
                    break;
                }

                default:
                    AssertFailed();
                    RTStrPrintf(pszBuffer, cbBuffer, "%s = %#x\n", pszName, uValue.u);
                    break;
            }
        }
        else
            RTStrPrintf(pszBuffer, cbBuffer, "UNKNOWN_%d_%#x = %#x\n", iName, iName, pTextureState->value);
    }
    else
        RTStrPrintf(pszBuffer, cbBuffer, "INVALID");
    return pszRet;
}



static const char * const g_apszTransformTypes[] =
{
    "SVGA3D_TRANSFORM_INVALID",
    "SVGA3D_TRANSFORM_WORLD",
    "SVGA3D_TRANSFORM_VIEW",
    "SVGA3D_TRANSFORM_PROJECTION",
    "SVGA3D_TRANSFORM_TEXTURE0",
    "SVGA3D_TRANSFORM_TEXTURE1",
    "SVGA3D_TRANSFORM_TEXTURE2",
    "SVGA3D_TRANSFORM_TEXTURE3",
    "SVGA3D_TRANSFORM_TEXTURE4",
    "SVGA3D_TRANSFORM_TEXTURE5",
    "SVGA3D_TRANSFORM_TEXTURE6",
    "SVGA3D_TRANSFORM_TEXTURE7",
    "SVGA3D_TRANSFORM_WORLD1",
    "SVGA3D_TRANSFORM_WORLD2",
    "SVGA3D_TRANSFORM_WORLD3",
};

static const char * const g_apszFaces[] =
{
    "SVGA3D_FACE_INVALID",
    "SVGA3D_FACE_NONE",
    "SVGA3D_FACE_FRONT",
    "SVGA3D_FACE_BACK",
    "SVGA3D_FACE_FRONT_BACK",
};

static const char * const g_apszLightTypes[] =
{
    "SVGA3D_LIGHTTYPE_INVALID",
    "SVGA3D_LIGHTTYPE_POINT",
    "SVGA3D_LIGHTTYPE_SPOT1",
    "SVGA3D_LIGHTTYPE_SPOT2",
    "SVGA3D_LIGHTTYPE_DIRECTIONAL",
};

static const char * const g_apszRenderTargets[] =
{
    "SVGA3D_RT_DEPTH",
    "SVGA3D_RT_STENCIL",
    "SVGA3D_RT_COLOR0",
    "SVGA3D_RT_COLOR1",
    "SVGA3D_RT_COLOR2",
    "SVGA3D_RT_COLOR3",
    "SVGA3D_RT_COLOR4",
    "SVGA3D_RT_COLOR5",
    "SVGA3D_RT_COLOR6",
    "SVGA3D_RT_COLOR7",
};

static void vmsvga3dInfoContextWorkerOne(PCDBGFINFOHLP pHlp, PVMSVGA3DCONTEXT pContext, bool fVerbose)
{
    RT_NOREF(fVerbose);
    char szTmp[128];

    pHlp->pfnPrintf(pHlp, "*** VMSVGA 3d context %#x (%d) ***\n", pContext->id, pContext->id);
#ifdef RT_OS_WINDOWS
    pHlp->pfnPrintf(pHlp, "hwnd:                    %p\n", pContext->hwnd);
    if (fVerbose)
        vmsvga3dInfoHostWindow(pHlp, (uintptr_t)pContext->hwnd);
# ifdef VMSVGA3D_DIRECT3D
    pHlp->pfnPrintf(pHlp, "pDevice:                 %p\n", pContext->pDevice);
# else
    pHlp->pfnPrintf(pHlp, "hdc:                     %p\n", pContext->hdc);
    pHlp->pfnPrintf(pHlp, "hglrc:                   %p\n", pContext->hglrc);
# endif

#elif defined(RT_OS_DARWIN)
    pHlp->pfnPrintf(pHlp, "cocoaView:               %p\n", pContext->cocoaView);
    if (pContext->cocoaView)
        vmsvga3dInfoHostWindow(pHlp, (uintptr_t)pContext->cocoaView);
    pHlp->pfnPrintf(pHlp, "cocoaContext:            %p\n", pContext->cocoaContext);
    if (pContext->fOtherProfile)
        pHlp->pfnPrintf(pHlp, "fOtherProfile:           true\n");

#else
    pHlp->pfnPrintf(pHlp, "window:                  %p\n", pContext->window);
    pHlp->pfnPrintf(pHlp, "fMapped:                 %RTbool\n", pContext->fMapped);
    if (pContext->window)
        vmsvga3dInfoHostWindow(pHlp, (uintptr_t)pContext->window);
    pHlp->pfnPrintf(pHlp, "glxContext:              %p\n", pContext->glxContext);

#endif
    pHlp->pfnPrintf(pHlp, "sidRenderTarget:         %#x\n", pContext->sidRenderTarget);

    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->aSidActiveTexture); i++)
        if (pContext->aSidActiveTexture[i] != SVGA3D_INVALID_ID)
            pHlp->pfnPrintf(pHlp, "aSidActiveTexture[%u]:    %#x\n", i, pContext->aSidActiveTexture[i]);

    pHlp->pfnPrintf(pHlp, "fUpdateFlags:            %#x\n", pContext->state.u32UpdateFlags);

    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->state.aRenderState); i++)
        if (pContext->state.aRenderState[i].state != SVGA3D_RS_INVALID)
            pHlp->pfnPrintf(pHlp, "aRenderState[%3d]: %s\n", i,
                            vmsvga3dFormatRenderState(szTmp, sizeof(szTmp), &pContext->state.aRenderState[i]));

    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->state.aTextureState); i++)
        for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aTextureState[i]); j++)
            if (pContext->state.aTextureState[i][j].name != SVGA3D_TS_INVALID)
                pHlp->pfnPrintf(pHlp, "aTextureState[%3d][%3d]: %s\n", i, j,
                                vmsvga3dFormatTextureState(szTmp, sizeof(szTmp), &pContext->state.aTextureState[i][j]));

    AssertCompile(RT_ELEMENTS(g_apszTransformTypes) == SVGA3D_TRANSFORM_MAX);
    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->state.aTransformState); i++)
        if (pContext->state.aTransformState[i].fValid)
        {
            pHlp->pfnPrintf(pHlp, "aTransformState[%s(%u)]:\n", g_apszTransformTypes[i], i);
            for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aTransformState[i].matrix); j++)
                pHlp->pfnPrintf(pHlp,
                                (j % 4) == 0 ? "    [ " FLOAT_FMT_STR : (j % 4) < 3  ? ", " FLOAT_FMT_STR : ", " FLOAT_FMT_STR "]\n",
                                FLOAT_FMT_ARGS(pContext->state.aTransformState[i].matrix[j]));
        }

    AssertCompile(RT_ELEMENTS(g_apszFaces) == SVGA3D_FACE_MAX);
    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->state.aMaterial); i++)
        if (pContext->state.aMaterial[i].fValid)
        {
            pHlp->pfnPrintf(pHlp, "aTransformState[%s(%u)]: shininess=" FLOAT_FMT_STR "\n",
                            g_apszFaces[i], i, FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.shininess));
            pHlp->pfnPrintf(pHlp, "    diffuse =[ " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR " ]\n",
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.diffuse[0]),
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.diffuse[1]),
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.diffuse[2]),
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.diffuse[3]));
            pHlp->pfnPrintf(pHlp, "    ambient =[ " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR " ]\n",
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.ambient[0]),
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.ambient[1]),
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.ambient[2]),
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.ambient[3]));
            pHlp->pfnPrintf(pHlp, "    specular=[ " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR " ]\n",
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.specular[0]),
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.specular[1]),
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.specular[2]),
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.specular[3]));
            pHlp->pfnPrintf(pHlp, "    emissive=[ " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR " ]\n",
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.emissive[0]),
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.emissive[1]),
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.emissive[2]),
                            FLOAT_FMT_ARGS(pContext->state.aMaterial[i].material.emissive[3]));
        }

    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->state.aClipPlane); i++)
        if (pContext->state.aClipPlane[i].fValid)
            pHlp->pfnPrintf(pHlp, "aClipPlane[%#04x]: [ " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR " ]\n",
                            i,
                            FLOAT_FMT_ARGS(pContext->state.aClipPlane[i].plane[0]),
                            FLOAT_FMT_ARGS(pContext->state.aClipPlane[i].plane[1]),
                            FLOAT_FMT_ARGS(pContext->state.aClipPlane[i].plane[2]),
                            FLOAT_FMT_ARGS(pContext->state.aClipPlane[i].plane[3]));

    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->state.aLightData); i++)
        if (pContext->state.aLightData[i].fValidData)
        {
            pHlp->pfnPrintf(pHlp, "aLightData[%#04x]: enabled=%RTbool inWorldSpace=%RTbool type=%s(%u)\n",
                            i,
                            pContext->state.aLightData[i].fEnabled,
                            pContext->state.aLightData[i].data.inWorldSpace,
                            (uint32_t)pContext->state.aLightData[i].data.type < RT_ELEMENTS(g_apszLightTypes)
                            ? g_apszLightTypes[pContext->state.aLightData[i].data.type] : "UNKNOWN",
                            pContext->state.aLightData[i].data.type);
            pHlp->pfnPrintf(pHlp, "    diffuse  =[ " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR " ]\n",
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.diffuse[0]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.diffuse[1]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.diffuse[2]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.diffuse[3]));
            pHlp->pfnPrintf(pHlp, "    specular =[ " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR " ]\n",
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.specular[0]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.specular[1]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.specular[2]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.specular[3]));
            pHlp->pfnPrintf(pHlp, "    ambient  =[ " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR " ]\n",
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.ambient[0]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.ambient[1]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.ambient[2]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.ambient[3]));
            pHlp->pfnPrintf(pHlp, "    position =[ " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR " ]\n",
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.position[0]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.position[1]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.position[2]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.position[3]));
            pHlp->pfnPrintf(pHlp, "    direction=[ " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR " ]\n",
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.direction[0]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.direction[1]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.direction[2]),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.direction[3]));
            pHlp->pfnPrintf(pHlp, "    range=" FLOAT_FMT_STR "  falloff=" FLOAT_FMT_STR "\n",
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.range),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.falloff));
            pHlp->pfnPrintf(pHlp, "    attenuation0=" FLOAT_FMT_STR "  attenuation1=" FLOAT_FMT_STR "  attenuation2=" FLOAT_FMT_STR "\n",
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.attenuation0),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.attenuation1),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.attenuation2));
            pHlp->pfnPrintf(pHlp, "    theta=" FLOAT_FMT_STR "  phi=" FLOAT_FMT_STR "\n",
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.theta),
                            FLOAT_FMT_ARGS(pContext->state.aLightData[i].data.phi));
        }

    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->state.aRenderTargets); i++)
        if (pContext->state.aRenderTargets[i] != SVGA3D_INVALID_ID)
            pHlp->pfnPrintf(pHlp, "aRenderTargets[%s/%u] = %#x (%d)\n",
                            i < RT_ELEMENTS(g_apszRenderTargets) ? g_apszRenderTargets[i] : "UNKNOWN", i,
                            pContext->state.aRenderTargets[i], pContext->state.aRenderTargets[i]);

    pHlp->pfnPrintf(pHlp, "RectScissor: (x,y,cx,cy)=(%u,%u,%u,%u)\n",
                    pContext->state.RectViewPort.x, pContext->state.RectViewPort.y,
                    pContext->state.RectViewPort.w, pContext->state.RectViewPort.h);
    pHlp->pfnPrintf(pHlp, "zRange:        (min,max)=(" FLOAT_FMT_STR ", " FLOAT_FMT_STR ")\n",
                    FLOAT_FMT_ARGS(pContext->state.zRange.min),
                    FLOAT_FMT_ARGS(pContext->state.zRange.max));
    pHlp->pfnPrintf(pHlp, "fUpdateFlags:            %#x\n", pContext->state.u32UpdateFlags);
    pHlp->pfnPrintf(pHlp, "shidPixel:               %#x (%d)\n", pContext->state.shidPixel, pContext->state.shidPixel);
    pHlp->pfnPrintf(pHlp, "shidVertex:              %#x (%d)\n", pContext->state.shidVertex, pContext->state.shidVertex);

    for (uint32_t iWhich = 0; iWhich < 2; iWhich++)
    {
        uint32_t            cConsts  = iWhich == 0 ? pContext->state.cPixelShaderConst   : pContext->state.cVertexShaderConst;
        PVMSVGASHADERCONST  paConsts = iWhich == 0 ? pContext->state.paPixelShaderConst  : pContext->state.paVertexShaderConst;
        const char         *pszName  = iWhich      ?                "paPixelShaderConst" :                "paVertexShaderConst";

        for (uint32_t i = 0; i < cConsts; i++)
            if (paConsts[i].fValid)
            {
                if (paConsts[i].ctype == SVGA3D_CONST_TYPE_FLOAT)
                    pHlp->pfnPrintf(pHlp, "%s[%#x(%u)] = [" FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR ", " FLOAT_FMT_STR "] ctype=FLOAT\n",
                                    pszName, i, i,
                                    FLOAT_FMT_ARGS(paConsts[i].value[0]), FLOAT_FMT_ARGS(paConsts[i].value[1]),
                                    FLOAT_FMT_ARGS(paConsts[i].value[2]), FLOAT_FMT_ARGS(paConsts[i].value[3]));
                else
                    pHlp->pfnPrintf(pHlp, "%s[%#x(%u)] = [%#x, %#x, %#x, %#x] ctype=%s\n",
                                    pszName, i, i,
                                    paConsts[i].value[0], paConsts[i].value[1],
                                    paConsts[i].value[2], paConsts[i].value[3],
                                    paConsts[i].ctype == SVGA3D_CONST_TYPE_INT ? "INT"
                                    : paConsts[i].ctype == SVGA3D_CONST_TYPE_BOOL ? "BOOL" : "UNKNOWN");
            }
    }

    for (uint32_t iWhich = 0; iWhich < 2; iWhich++)
    {
        uint32_t        cShaders  = iWhich == 0 ? pContext->cPixelShaders : pContext->cVertexShaders;
        PVMSVGA3DSHADER paShaders = iWhich == 0 ? pContext->paPixelShader : pContext->paVertexShader;
        const char     *pszName   = iWhich == 0 ? "paPixelShaders" : "paVertexShaders";
        for (uint32_t i = 0; i < cShaders; i++)
            if (paShaders[i].id == i)
            {
                pHlp->pfnPrintf(pHlp, "%s[%u]:   id=%#x cid=%#x type=%s(%d) cbData=%#x pvData=%p\n",
                                pszName, i,
                                paShaders[i].id,
                                paShaders[i].cid,
                                paShaders[i].type == SVGA3D_SHADERTYPE_VS ? "VS"
                                : paShaders[i].type == SVGA3D_SHADERTYPE_PS ? "PS" : "UNKNOWN",
                                paShaders[i].type,
                                paShaders[i].cbData,
                                paShaders[i].pShaderProgram);
            }
    }
}


void vmsvga3dInfoContextWorker(PVGASTATE pThis, PCDBGFINFOHLP pHlp, uint32_t cid, bool fVerbose)
{
    /* Warning! This code is currently racing papContexts reallocation! */
    /* Warning! This code is currently racing papContexts reallocation! */
    /* Warning! This code is currently racing papContexts reallocation! */
    VMSVGA3DSTATE volatile *pState = pThis->svga.p3dState;
    if (pState)
    {
        /*
         * Deal with a specific request first.
         */
        if (cid != UINT32_MAX)
        {
            if (cid < pState->cContexts)
            {
                PVMSVGA3DCONTEXT pContext = pState->papContexts[cid];
                if (pContext && pContext->id == cid)
                {
                    vmsvga3dInfoContextWorkerOne(pHlp, pContext, fVerbose);
                    return;
                }
            }
#ifdef VMSVGA3D_OPENGL
            else if (   cid == VMSVGA3D_SHARED_CTX_ID
                     && pState->SharedCtx.id == cid)
            {
                vmsvga3dInfoContextWorkerOne(pHlp, &((PVMSVGA3DSTATE)pState)->SharedCtx, fVerbose);
                return;
            }
#endif
            pHlp->pfnPrintf(pHlp, "Context ID %#x not found.\n", cid);
        }
        else
        {
#ifdef VMSVGA3D_OPENGL
            /*
             * Dump the shared context.
             */
            if (pState->SharedCtx.id == VMSVGA3D_SHARED_CTX_ID)
            {
                pHlp->pfnPrintf(pHlp, "Shared context:\n");
                vmsvga3dInfoContextWorkerOne(pHlp, &((PVMSVGA3DSTATE)pState)->SharedCtx, fVerbose);
            }
#endif

            /*
             * Dump the per-screen contexts.
             */
            /** @todo multi screen   */

            /*
             * Dump all.
             */
            uint32_t cContexts = pState->cContexts;
            pHlp->pfnPrintf(pHlp, "cContexts=%d\n", cContexts);
            for (cid = 0; cid < cContexts; cid++)
            {
                PVMSVGA3DCONTEXT pContext = pState->papContexts[cid];
                if (pContext && pContext->id == cid)
                {
                    pHlp->pfnPrintf(pHlp, "\n");
                    vmsvga3dInfoContextWorkerOne(pHlp, pContext, fVerbose);
                }
            }
        }
    }
}


#ifdef VMSVGA3D_DIRECT3D
/**
 * Release all shared surface objects.
 */
static DECLCALLBACK(int) vmsvga3dInfoSharedObjectCallback(PAVLU32NODECORE pNode, void *pvUser)
{
    PVMSVGA3DSHAREDSURFACE  pSharedSurface = (PVMSVGA3DSHAREDSURFACE)pNode;
    PCDBGFINFOHLP           pHlp           = (PCDBGFINFOHLP)pvUser;

    pHlp->pfnPrintf(pHlp, "Shared surface:          %#x  pv=%p\n", pSharedSurface->Core.Key, pSharedSurface->u.pCubeTexture);

    return 0;
}
#endif /* VMSVGA3D_DIRECT3D */


static void vmsvga3dInfoSurfaceWorkerOne(PCDBGFINFOHLP pHlp, PVMSVGA3DSURFACE pSurface,
                                         bool fVerbose, uint32_t cxAscii, bool fInvY)
{
    char szTmp[128];

    pHlp->pfnPrintf(pHlp, "*** VMSVGA 3d surface %#x (%d)%s ***\n", pSurface->id, pSurface->id, pSurface->fDirty ? " - dirty" : "");
#ifdef VMSVGA3D_OPENGL
    pHlp->pfnPrintf(pHlp, "idWeakContextAssociation: %#x\n", pSurface->idWeakContextAssociation);
#else
    pHlp->pfnPrintf(pHlp, "idAssociatedContext:     %#x\n", pSurface->idAssociatedContext);
#endif
    pHlp->pfnPrintf(pHlp, "Format:                  %s\n",
                    vmsvgaFormatEnumValueEx(szTmp, sizeof(szTmp), NULL, (int)pSurface->format, false, &g_SVGA3dSurfaceFormat2String));
    pHlp->pfnPrintf(pHlp, "Flags:                   %#x", pSurface->flags);
    vmsvga3dInfoU32Flags(pHlp, pSurface->flags, "SVGA3D_SURFACE_", g_aSvga3DSurfaceFlags, RT_ELEMENTS(g_aSvga3DSurfaceFlags));
    pHlp->pfnPrintf(pHlp, "\n");
    if (pSurface->cFaces == 0)
        pHlp->pfnPrintf(pHlp, "Faces:                   %u\n", pSurface->cFaces);
    for (uint32_t iFace = 0; iFace < pSurface->cFaces; iFace++)
    {
        Assert(pSurface->faces[iFace].numMipLevels <= pSurface->faces[0].numMipLevels);
        if (pSurface->faces[iFace].numMipLevels == 0)
            pHlp->pfnPrintf(pHlp, "Faces[%u] Mipmap levels:  %u\n", iFace, pSurface->faces[iFace].numMipLevels);

        uint32_t iMipmap = iFace * pSurface->faces[0].numMipLevels;
        for (uint32_t iLevel = 0; iLevel < pSurface->faces[iFace].numMipLevels; iLevel++, iMipmap++)
        {
            pHlp->pfnPrintf(pHlp, "Face #%u, mipmap #%u[%u]:%s  cx=%u, cy=%u, cz=%u, cbSurface=%#x, cbPitch=%#x",
                            iFace, iLevel, iMipmap, iMipmap < 10 ? " " : "",
                            pSurface->pMipmapLevels[iMipmap].size.width,
                            pSurface->pMipmapLevels[iMipmap].size.height,
                            pSurface->pMipmapLevels[iMipmap].size.depth,
                            pSurface->pMipmapLevels[iMipmap].cbSurface,
                            pSurface->pMipmapLevels[iMipmap].cbSurfacePitch);
            if (pSurface->pMipmapLevels[iMipmap].pSurfaceData)
                pHlp->pfnPrintf(pHlp, " pvData=%p", pSurface->pMipmapLevels[iMipmap].pSurfaceData);
            if (pSurface->pMipmapLevels[iMipmap].fDirty)
                pHlp->pfnPrintf(pHlp, " dirty");
            pHlp->pfnPrintf(pHlp, "\n");
        }
    }

    pHlp->pfnPrintf(pHlp, "cbBlock:                 %u (%#x)\n", pSurface->cbBlock, pSurface->cbBlock);
    pHlp->pfnPrintf(pHlp, "Multi-sample count:      %u\n", pSurface->multiSampleCount);
    pHlp->pfnPrintf(pHlp, "Autogen filter:          %s\n",
                    vmsvgaFormatEnumValue(szTmp, sizeof(szTmp), NULL, pSurface->autogenFilter,
                                          "SVGA3D_TEX_FILTER_", g_apszTexureFilters, RT_ELEMENTS(g_apszTexureFilters)));

#ifdef VMSVGA3D_DIRECT3D
    pHlp->pfnPrintf(pHlp, "formatD3D:               %s\n",
                    vmsvgaFormatEnumValueEx(szTmp, sizeof(szTmp), NULL, pSurface->formatD3D, true, &g_D3DFormat2String));
    pHlp->pfnPrintf(pHlp, "fUsageD3D:               %#x", pSurface->fUsageD3D);
    vmsvga3dInfoU32Flags(pHlp, pSurface->fUsageD3D, "D3DUSAGE_", g_aD3DUsageFlags, RT_ELEMENTS(g_aD3DUsageFlags));
    pHlp->pfnPrintf(pHlp, "\n");
    pHlp->pfnPrintf(pHlp, "multiSampleTypeD3D:      %s\n",
                    vmsvgaFormatEnumValueEx(szTmp, sizeof(szTmp), NULL, pSurface->multiSampleTypeD3D,
                                            true, &g_D3DMultiSampleType2String));
    if (pSurface->hSharedObject != NULL)
        pHlp->pfnPrintf(pHlp, "hSharedObject:           %p\n", pSurface->hSharedObject);
    if (pSurface->pQuery)
        pHlp->pfnPrintf(pHlp, "pQuery:                  %p\n", pSurface->pQuery);
    if (pSurface->u.pSurface)
        pHlp->pfnPrintf(pHlp, "u.pXxxx:                 %p\n", pSurface->u.pSurface);
    if (pSurface->bounce.pTexture)
        pHlp->pfnPrintf(pHlp, "bounce.pXxxx:            %p\n", pSurface->bounce.pTexture);
    RTAvlU32DoWithAll(&pSurface->pSharedObjectTree, true /*fFromLeft*/, vmsvga3dInfoSharedObjectCallback, (void *)pHlp);
    pHlp->pfnPrintf(pHlp, "fStencilAsTexture:       %RTbool\n", pSurface->fStencilAsTexture);

#elif defined(VMSVGA3D_OPENGL)
    /** @todo   */
#else
# error "Build config error."
#endif

    if (fVerbose)
        for (uint32_t iFace = 0; iFace < pSurface->cFaces; iFace++)
        {
            uint32_t iMipmap = iFace * pSurface->faces[0].numMipLevels;
            for (uint32_t iLevel = 0; iLevel < pSurface->faces[iFace].numMipLevels; iLevel++, iMipmap++)
                if (pSurface->pMipmapLevels[iMipmap].pSurfaceData)
                {
                    if (ASMMemIsZero(pSurface->pMipmapLevels[iMipmap].pSurfaceData,
                                     pSurface->pMipmapLevels[iMipmap].cbSurface))
                        pHlp->pfnPrintf(pHlp, "--- Face #%u, mipmap #%u[%u]: all zeros ---\n", iFace, iLevel, iMipmap);
                    else
                    {
                        pHlp->pfnPrintf(pHlp, "--- Face #%u, mipmap #%u[%u]: cx=%u, cy=%u, cz=%u ---\n",
                                        iFace, iLevel, iMipmap,
                                        pSurface->pMipmapLevels[iMipmap].size.width,
                                        pSurface->pMipmapLevels[iMipmap].size.height,
                                        pSurface->pMipmapLevels[iMipmap].size.depth);
                        vmsvga3dAsciiPrint(vmsvga3dAsciiPrintlnInfo, (void *)pHlp,
                                           pSurface->pMipmapLevels[iMipmap].pSurfaceData,
                                           pSurface->pMipmapLevels[iMipmap].cbSurface,
                                           pSurface->pMipmapLevels[iMipmap].size.width,
                                           pSurface->pMipmapLevels[iMipmap].size.height,
                                           pSurface->pMipmapLevels[iMipmap].cbSurfacePitch,
                                           pSurface->format,
                                           fInvY,
                                           cxAscii, cxAscii * 3 / 4);
                    }
                }
        }
}


void vmsvga3dInfoSurfaceWorker(PVGASTATE pThis, PCDBGFINFOHLP pHlp, uint32_t sid, bool fVerbose, uint32_t cxAscii, bool fInvY)
{
    /* Warning! This code is currently racing papSurfaces reallocation! */
    /* Warning! This code is currently racing papSurfaces reallocation! */
    /* Warning! This code is currently racing papSurfaces reallocation! */
    VMSVGA3DSTATE volatile *pState = pThis->svga.p3dState;
    if (pState)
    {
        /*
         * Deal with a specific request first.
         */
        if (sid != UINT32_MAX)
        {
            if (sid < pState->cSurfaces)
            {
                PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];
                if (pSurface && pSurface->id == sid)
                {
                    if (fVerbose)
                        vmsvga3dSurfaceUpdateHeapBuffersOnFifoThread(pThis, sid);
                    vmsvga3dInfoSurfaceWorkerOne(pHlp, pSurface, fVerbose, cxAscii, fInvY);
                    return;
                }
            }
            pHlp->pfnPrintf(pHlp, "Surface ID %#x not found.\n", sid);
        }
        else
        {
            /*
             * Dump all.
             */
            if (fVerbose)
                vmsvga3dSurfaceUpdateHeapBuffersOnFifoThread(pThis, UINT32_MAX);
            uint32_t cSurfaces = pState->cSurfaces;
            pHlp->pfnPrintf(pHlp, "cSurfaces=%d\n", cSurfaces);
            for (sid = 0; sid < cSurfaces; sid++)
            {
                PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];
                if (pSurface && pSurface->id == sid)
                {
                    pHlp->pfnPrintf(pHlp, "\n");
                    vmsvga3dInfoSurfaceWorkerOne(pHlp, pSurface, fVerbose, cxAscii, fInvY);
                }
            }
        }
    }

}

