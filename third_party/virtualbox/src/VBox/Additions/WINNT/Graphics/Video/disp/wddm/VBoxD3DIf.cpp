/* $Id: VBoxD3DIf.cpp $ */
/** @file
 * VBoxVideo Display D3D User mode dll
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "VBoxDispD3DCmn.h"

/* DDI2D3D */

D3DFORMAT vboxDDI2D3DFormat(D3DDDIFORMAT format)
{
    /** @todo check they are all equal */
    return (D3DFORMAT)format;
}

D3DMULTISAMPLE_TYPE vboxDDI2D3DMultiSampleType(D3DDDIMULTISAMPLE_TYPE enmType)
{
    /** @todo check they are all equal */
    return (D3DMULTISAMPLE_TYPE)enmType;
}

D3DPOOL vboxDDI2D3DPool(D3DDDI_POOL enmPool)
{
    /** @todo check they are all equal */
    switch (enmPool)
    {
    case D3DDDIPOOL_SYSTEMMEM:
        return D3DPOOL_SYSTEMMEM;
    case D3DDDIPOOL_VIDEOMEMORY:
    case D3DDDIPOOL_LOCALVIDMEM:
    case D3DDDIPOOL_NONLOCALVIDMEM:
        /** @todo what would be proper here? */
        return D3DPOOL_DEFAULT;
    default:
        Assert(0);
    }
    return D3DPOOL_DEFAULT;
}

D3DRENDERSTATETYPE vboxDDI2D3DRenderStateType(D3DDDIRENDERSTATETYPE enmType)
{
    /** @todo not entirely correct, need to check */
    return (D3DRENDERSTATETYPE)enmType;
}

VBOXWDDMDISP_TSS_LOOKUP vboxDDI2D3DTestureStageStateType(D3DDDITEXTURESTAGESTATETYPE enmType)
{
    static const VBOXWDDMDISP_TSS_LOOKUP lookup[] =
    {
        {FALSE, D3DTSS_FORCE_DWORD},             /*  0, D3DDDITSS_TEXTUREMAP */
        {FALSE, D3DTSS_COLOROP},                 /*  1, D3DDDITSS_COLOROP */
        {FALSE, D3DTSS_COLORARG1},               /*  2, D3DDDITSS_COLORARG1 */
        {FALSE, D3DTSS_COLORARG2},               /*  3, D3DDDITSS_COLORARG2 */
        {FALSE, D3DTSS_ALPHAOP},                 /*  4, D3DDDITSS_ALPHAOP */
        {FALSE, D3DTSS_ALPHAARG1},               /*  5, D3DDDITSS_ALPHAARG1 */
        {FALSE, D3DTSS_ALPHAARG2},               /*  6, D3DDDITSS_ALPHAARG2 */
        {FALSE, D3DTSS_BUMPENVMAT00},            /*  7, D3DDDITSS_BUMPENVMAT00 */
        {FALSE, D3DTSS_BUMPENVMAT01},            /*  8, D3DDDITSS_BUMPENVMAT01 */
        {FALSE, D3DTSS_BUMPENVMAT10},            /*  9, D3DDDITSS_BUMPENVMAT10 */
        {FALSE, D3DTSS_BUMPENVMAT11},            /* 10, D3DDDITSS_BUMPENVMAT11 */
        {FALSE, D3DTSS_TEXCOORDINDEX},           /* 11, D3DDDITSS_TEXCOORDINDEX */
        {FALSE, D3DTSS_FORCE_DWORD},             /* 12, unused */
        {TRUE, D3DSAMP_ADDRESSU},                /* 13, D3DDDITSS_ADDRESSU */
        {TRUE, D3DSAMP_ADDRESSV},                /* 14, D3DDDITSS_ADDRESSV */
        {TRUE, D3DSAMP_BORDERCOLOR},             /* 15, D3DDDITSS_BORDERCOLOR */
        {TRUE, D3DSAMP_MAGFILTER},               /* 16, D3DDDITSS_MAGFILTER */
        {TRUE, D3DSAMP_MINFILTER},               /* 17, D3DDDITSS_MINFILTER */
        {TRUE, D3DSAMP_MIPFILTER},               /* 18, D3DDDITSS_MIPFILTER */
        {TRUE, D3DSAMP_MIPMAPLODBIAS},           /* 19, D3DDDITSS_MIPMAPLODBIAS */
        {TRUE, D3DSAMP_MAXMIPLEVEL},             /* 20, D3DDDITSS_MAXMIPLEVEL */
        {TRUE, D3DSAMP_MAXANISOTROPY},           /* 21, D3DDDITSS_MAXANISOTROPY */
        {FALSE, D3DTSS_BUMPENVLSCALE},           /* 22, D3DDDITSS_BUMPENVLSCALE */
        {FALSE, D3DTSS_BUMPENVLOFFSET},          /* 23, D3DDDITSS_BUMPENVLOFFSET */
        {FALSE, D3DTSS_TEXTURETRANSFORMFLAGS},   /* 24, D3DDDITSS_TEXTURETRANSFORMFLAGS */
        {TRUE, D3DSAMP_ADDRESSW},                /* 25, D3DDDITSS_ADDRESSW */
        {FALSE, D3DTSS_COLORARG0},               /* 26, D3DDDITSS_COLORARG0 */
        {FALSE, D3DTSS_ALPHAARG0},               /* 27, D3DDDITSS_ALPHAARG0 */
        {FALSE, D3DTSS_RESULTARG},               /* 28, D3DDDITSS_RESULTARG */
        {TRUE, D3DSAMP_SRGBTEXTURE},             /* 29, D3DDDITSS_SRGBTEXTURE */
        {TRUE, D3DSAMP_ELEMENTINDEX},            /* 30, D3DDDITSS_ELEMENTINDEX */
        {TRUE, D3DSAMP_DMAPOFFSET},              /* 31, D3DDDITSS_DMAPOFFSET */
        {FALSE, D3DTSS_CONSTANT},                /* 32, D3DDDITSS_CONSTANT */
        {FALSE, D3DTSS_FORCE_DWORD},             /* 33, D3DDDITSS_DISABLETEXTURECOLORKEY */
        {FALSE, D3DTSS_FORCE_DWORD},             /* 34, D3DDDITSS_TEXTURECOLORKEYVAL */
    };

    Assert(enmType > 0);
    Assert(enmType < RT_ELEMENTS(lookup));
    Assert(lookup[enmType].dType != D3DTSS_FORCE_DWORD);

    return lookup[enmType];
}

DWORD vboxDDI2D3DUsage(D3DDDI_RESOURCEFLAGS fFlags)
{
    DWORD fUsage = 0;
    if (fFlags.Dynamic)
        fUsage |= D3DUSAGE_DYNAMIC;
    if (fFlags.AutogenMipmap)
        fUsage |= D3DUSAGE_AUTOGENMIPMAP;
    if (fFlags.DMap)
        fUsage |= D3DUSAGE_DMAP;
    if (fFlags.WriteOnly)
        fUsage |= D3DUSAGE_WRITEONLY;
    if (fFlags.NPatches)
        fUsage |= D3DUSAGE_NPATCHES;
    if (fFlags.Points)
        fUsage |= D3DUSAGE_POINTS;
    if (fFlags.RenderTarget)
        fUsage |= D3DUSAGE_RENDERTARGET;
    if (fFlags.RtPatches)
        fUsage |= D3DUSAGE_RTPATCHES;
    if (fFlags.TextApi)
        fUsage |= D3DUSAGE_TEXTAPI;
    if (fFlags.WriteOnly)
        fUsage |= D3DUSAGE_WRITEONLY;
    //below are wddm 1.1-specific
//    if (fFlags.RestrictedContent)
//        fUsage |= D3DUSAGE_RESTRICTED_CONTENT;
//    if (fFlags.RestrictSharedAccess)
//        fUsage |= D3DUSAGE_RESTRICT_SHARED_RESOURCE;
    return fUsage;
}

DWORD vboxDDI2D3DLockFlags(D3DDDI_LOCKFLAGS fLockFlags)
{
    DWORD fFlags = 0;
    if (fLockFlags.Discard)
        fFlags |= D3DLOCK_DISCARD;
    if (fLockFlags.NoOverwrite)
        fFlags |= D3DLOCK_NOOVERWRITE;
    if (fLockFlags.ReadOnly)
        fFlags |= D3DLOCK_READONLY;
    if (fLockFlags.DoNotWait)
        fFlags |= D3DLOCK_DONOTWAIT;
    return fFlags;
}

D3DTEXTUREFILTERTYPE vboxDDI2D3DBltFlags(D3DDDI_BLTFLAGS fFlags)
{
    if (fFlags.Point)
    {
        /* no flags other than [Begin|Continue|End]PresentToDwm are set */
        Assert((fFlags.Value & (~(0x00000100 | 0x00000200 | 0x00000400))) == 1);
        return D3DTEXF_POINT;
    }
    if (fFlags.Linear)
    {
        /* no flags other than [Begin|Continue|End]PresentToDwm are set */
        Assert((fFlags.Value & (~(0x00000100 | 0x00000200 | 0x00000400))) == 2);
        return D3DTEXF_LINEAR;
    }
    /* no flags other than [Begin|Continue|End]PresentToDwm are set */
    Assert((fFlags.Value & (~(0x00000100 | 0x00000200 | 0x00000400))) == 0);
    return D3DTEXF_NONE;
}

D3DQUERYTYPE vboxDDI2D3DQueryType(D3DDDIQUERYTYPE enmType)
{
    return (D3DQUERYTYPE)enmType;
}

DWORD vboxDDI2D3DIssueQueryFlags(D3DDDI_ISSUEQUERYFLAGS Flags)
{
    DWORD fFlags = 0;
    if (Flags.Begin)
        fFlags |= D3DISSUE_BEGIN;
    if (Flags.End)
        fFlags |= D3DISSUE_END;
    return fFlags;
}

/**/

/* D3DIf API */
static HRESULT vboxDispD3DIfSurfSynchMem(PVBOXWDDMDISP_RESOURCE pRc)
{
    if (pRc->RcDesc.enmPool != D3DDDIPOOL_SYSTEMMEM)
    {
        return S_OK;
    }

    for (UINT i = 0; i < pRc->cAllocations; ++i)
    {
        D3DLOCKED_RECT Rect;
        HRESULT hr = VBoxD3DIfLockRect(pRc, i, &Rect, NULL, D3DLOCK_DISCARD);
        if (FAILED(hr))
        {
            WARN(("VBoxD3DIfLockRect failed, hr(0x%x)", hr));
            return hr;
        }

        PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[i];
        Assert(pAlloc->pvMem);

        VBoxD3DIfLockUnlockMemSynch(pAlloc, &Rect, NULL, true /*bool bToLockInfo*/);

        hr = VBoxD3DIfUnlockRect(pRc, i);
        Assert(SUCCEEDED(hr));
    }
    return S_OK;
}

void VBoxD3DIfLockUnlockMemSynch(PVBOXWDDMDISP_ALLOCATION pAlloc, D3DLOCKED_RECT *pLockInfo, RECT *pRect, bool bToLockInfo)
{
    Assert(pAlloc->SurfDesc.pitch);
    Assert(pAlloc->pvMem);

    if (!pRect)
    {
        if (pAlloc->SurfDesc.pitch == (UINT)pLockInfo->Pitch)
        {
            Assert(pAlloc->SurfDesc.cbSize);
            if (bToLockInfo)
                memcpy(pLockInfo->pBits, pAlloc->pvMem, pAlloc->SurfDesc.cbSize);
            else
                memcpy(pAlloc->pvMem, pLockInfo->pBits, pAlloc->SurfDesc.cbSize);
        }
        else
        {
            uint8_t *pvSrc, *pvDst;
            uint32_t srcPitch, dstPitch;
            if (bToLockInfo)
            {
                pvSrc = (uint8_t *)pAlloc->pvMem;
                pvDst = (uint8_t *)pLockInfo->pBits;
                srcPitch = pAlloc->SurfDesc.pitch;
                dstPitch = pLockInfo->Pitch;
            }
            else
            {
                pvDst = (uint8_t *)pAlloc->pvMem;
                pvSrc = (uint8_t *)pLockInfo->pBits;
                dstPitch = pAlloc->SurfDesc.pitch;
                srcPitch = (uint32_t)pLockInfo->Pitch;
            }

            uint32_t cRows = vboxWddmCalcNumRows(0, pAlloc->SurfDesc.height, pAlloc->SurfDesc.format);
            uint32_t pitch = RT_MIN(srcPitch, dstPitch);
            Assert(pitch);
            for (UINT j = 0; j < cRows; ++j)
            {
                memcpy(pvDst, pvSrc, pitch);
                pvSrc += srcPitch;
                pvDst += dstPitch;
            }
        }
    }
    else
    {
        uint8_t *pvSrc, *pvDst;
        uint32_t srcPitch, dstPitch;
        uint8_t * pvAllocMemStart = (uint8_t *)pAlloc->pvMem;
        uint32_t offAllocMemStart = vboxWddmCalcOffXYrd(pRect->left, pRect->top, pAlloc->SurfDesc.pitch, pAlloc->SurfDesc.format);
        pvAllocMemStart += offAllocMemStart;

        if (bToLockInfo)
        {
            pvSrc = (uint8_t *)pvAllocMemStart;
            pvDst = (uint8_t *)pLockInfo->pBits;
            srcPitch = pAlloc->SurfDesc.pitch;
            dstPitch = pLockInfo->Pitch;
        }
        else
        {
            pvDst = (uint8_t *)pvAllocMemStart;
            pvSrc = (uint8_t *)pLockInfo->pBits;
            dstPitch = pAlloc->SurfDesc.pitch;
            srcPitch = (uint32_t)pLockInfo->Pitch;
        }

        if (pRect->right - pRect->left == (LONG)pAlloc->SurfDesc.width && srcPitch == dstPitch)
        {
            uint32_t cbSize = vboxWddmCalcSize(pAlloc->SurfDesc.pitch, pRect->bottom - pRect->top, pAlloc->SurfDesc.format);
            memcpy(pvDst, pvSrc, cbSize);
        }
        else
        {
            uint32_t pitch = RT_MIN(srcPitch, dstPitch);
            uint32_t cbCopyLine = vboxWddmCalcRowSize(pRect->left, pRect->right, pAlloc->SurfDesc.format);
            Assert(pitch); NOREF(pitch);
            uint32_t cRows = vboxWddmCalcNumRows(pRect->top, pRect->bottom, pAlloc->SurfDesc.format);
            for (UINT j = 0; j < cRows; ++j)
            {
                memcpy(pvDst, pvSrc, cbCopyLine);
                pvSrc += srcPitch;
                pvDst += dstPitch;
            }
        }
    }
}

HRESULT VBoxD3DIfLockRect(PVBOXWDDMDISP_RESOURCE pRc, UINT iAlloc,
        D3DLOCKED_RECT * pLockedRect,
        CONST RECT *pRect,
        DWORD fLockFlags)
{
    HRESULT hr = E_FAIL;
    Assert(!pRc->aAllocations[iAlloc].LockInfo.cLocks);
    Assert(pRc->cAllocations > iAlloc);
    switch (pRc->aAllocations[0].enmD3DIfType)
    {
        case VBOXDISP_D3DIFTYPE_SURFACE:
        {
            IDirect3DSurface9 *pD3DIfSurf = (IDirect3DSurface9*)pRc->aAllocations[iAlloc].pD3DIf;
            Assert(pD3DIfSurf);
            hr = pD3DIfSurf->LockRect(pLockedRect, pRect, fLockFlags);
            Assert(hr == S_OK);
            break;
        }
        case VBOXDISP_D3DIFTYPE_TEXTURE:
        {
            IDirect3DTexture9 *pD3DIfTex = (IDirect3DTexture9*)pRc->aAllocations[0].pD3DIf;
            Assert(pD3DIfTex);
            hr = pD3DIfTex->LockRect(iAlloc, pLockedRect, pRect, fLockFlags);
            Assert(hr == S_OK);
            break;
        }
        case VBOXDISP_D3DIFTYPE_CUBE_TEXTURE:
        {
            IDirect3DCubeTexture9 *pD3DIfCubeTex = (IDirect3DCubeTexture9*)pRc->aAllocations[0].pD3DIf;
            Assert(pD3DIfCubeTex);
            hr = pD3DIfCubeTex->LockRect(VBOXDISP_CUBEMAP_INDEX_TO_FACE(pRc, iAlloc),
                    VBOXDISP_CUBEMAP_INDEX_TO_LEVEL(pRc, iAlloc), pLockedRect, pRect, fLockFlags);
            Assert(hr == S_OK);
            break;
        }
        case VBOXDISP_D3DIFTYPE_VERTEXBUFFER:
        {
            IDirect3DVertexBuffer9 *pD3D9VBuf = (IDirect3DVertexBuffer9*)pRc->aAllocations[iAlloc].pD3DIf;
            Assert(pD3D9VBuf);
            hr = pD3D9VBuf->Lock(pRect ? pRect->left : 0/* offset */,
                    pRect ? pRect->right : 0 /* size 2 lock - 0 means all */,
                    &pLockedRect->pBits, fLockFlags);
            Assert(hr == S_OK);
            pLockedRect->Pitch = pRc->aAllocations[iAlloc].SurfDesc.pitch;
            break;
        }
        case VBOXDISP_D3DIFTYPE_INDEXBUFFER:
        {
            IDirect3DIndexBuffer9 *pD3D9IBuf = (IDirect3DIndexBuffer9*)pRc->aAllocations[iAlloc].pD3DIf;
            Assert(pD3D9IBuf);
            hr = pD3D9IBuf->Lock(pRect ? pRect->left : 0/* offset */,
                    pRect ? pRect->right : 0 /* size 2 lock - 0 means all */,
                    &pLockedRect->pBits, fLockFlags);
            Assert(hr == S_OK);
            pLockedRect->Pitch = pRc->aAllocations[iAlloc].SurfDesc.pitch;
            break;
        }
        default:
            WARN(("uknown if type %d", pRc->aAllocations[0].enmD3DIfType));
            break;
    }
    return hr;
}

HRESULT VBoxD3DIfUnlockRect(PVBOXWDDMDISP_RESOURCE pRc, UINT iAlloc)
{
    HRESULT hr = S_OK;
    Assert(pRc->cAllocations > iAlloc);
    switch (pRc->aAllocations[0].enmD3DIfType)
    {
        case VBOXDISP_D3DIFTYPE_SURFACE:
        {
            IDirect3DSurface9 *pD3DIfSurf = (IDirect3DSurface9*)pRc->aAllocations[iAlloc].pD3DIf;
            Assert(pD3DIfSurf);
            hr = pD3DIfSurf->UnlockRect();
            Assert(hr == S_OK);
            break;
        }
        case VBOXDISP_D3DIFTYPE_TEXTURE:
        {
            IDirect3DTexture9 *pD3DIfTex = (IDirect3DTexture9*)pRc->aAllocations[0].pD3DIf;
            Assert(pD3DIfTex);
            hr = pD3DIfTex->UnlockRect(iAlloc);
            Assert(hr == S_OK);
            break;
        }
        case VBOXDISP_D3DIFTYPE_CUBE_TEXTURE:
        {
            IDirect3DCubeTexture9 *pD3DIfCubeTex = (IDirect3DCubeTexture9*)pRc->aAllocations[0].pD3DIf;
            Assert(pD3DIfCubeTex);
            hr = pD3DIfCubeTex->UnlockRect(VBOXDISP_CUBEMAP_INDEX_TO_FACE(pRc, iAlloc),
                    VBOXDISP_CUBEMAP_INDEX_TO_LEVEL(pRc, iAlloc));
            Assert(hr == S_OK);
            break;
        }
        case VBOXDISP_D3DIFTYPE_VERTEXBUFFER:
        {
            IDirect3DVertexBuffer9 *pD3D9VBuf = (IDirect3DVertexBuffer9*)pRc->aAllocations[iAlloc].pD3DIf;
            Assert(pD3D9VBuf);
            hr = pD3D9VBuf->Unlock();
            Assert(hr == S_OK);
            break;
        }
        case VBOXDISP_D3DIFTYPE_INDEXBUFFER:
        {
            IDirect3DIndexBuffer9 *pD3D9IBuf = (IDirect3DIndexBuffer9*)pRc->aAllocations[iAlloc].pD3DIf;
            Assert(pD3D9IBuf);
            hr = pD3D9IBuf->Unlock();
            Assert(hr == S_OK);
            break;
        }
        default:
            WARN(("uknown if type %d", pRc->aAllocations[0].enmD3DIfType));
            hr = E_FAIL;
            break;
    }
    return hr;
}

HRESULT VBoxD3DIfCreateForRc(struct VBOXWDDMDISP_RESOURCE *pRc)
{
    PVBOXWDDMDISP_DEVICE pDevice = pRc->pDevice;
    HRESULT hr = E_FAIL;
    IDirect3DDevice9 * pDevice9If = VBOXDISP_D3DEV(pDevice);

    if (VBOXWDDMDISP_IS_TEXTURE(pRc->RcDesc.fFlags))
    {
        PVBOXWDDMDISP_ALLOCATION pAllocation = &pRc->aAllocations[0];
        IDirect3DBaseTexture9 *pD3DIfTex = NULL; /* Shut up MSC. */
        HANDLE hSharedHandle = pAllocation->hSharedHandle;
        void **pavClientMem = NULL;
        VBOXDISP_D3DIFTYPE enmD3DIfType = VBOXDISP_D3DIFTYPE_UNDEFINED;
        hr = S_OK;
        if (pRc->RcDesc.enmPool == D3DDDIPOOL_SYSTEMMEM)
        {
            pavClientMem = (void**)RTMemAlloc(sizeof (pavClientMem[0]) * pRc->cAllocations);
            Assert(pavClientMem);
            if (pavClientMem)
            {
                for (UINT i = 0; i < pRc->cAllocations; ++i)
                {
                    Assert(pRc->aAllocations[i].pvMem);
                    pavClientMem[i] = pRc->aAllocations[i].pvMem;
                }
            }
            else
                hr = E_FAIL;
        }

#ifdef DEBUG
        if (!pRc->RcDesc.fFlags.CubeMap)
        {
            PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[0];
            uint32_t tstW = pAlloc->SurfDesc.width;
            uint32_t tstH = pAlloc->SurfDesc.height;
            for (UINT i = 1; i < pRc->cAllocations; ++i)
            {
                tstW /= 2;
                tstH /= 2;
                pAlloc = &pRc->aAllocations[i];
                Assert((pAlloc->SurfDesc.width == tstW) || (!tstW && (pAlloc->SurfDesc.width==1)));
                Assert((pAlloc->SurfDesc.height == tstH) || (!tstH && (pAlloc->SurfDesc.height==1)));
            }
        }
#endif

        if (SUCCEEDED(hr))
        {
            if (pRc->RcDesc.fFlags.CubeMap)
            {
                if ( (pAllocation->SurfDesc.width!=pAllocation->SurfDesc.height)
                     || (pRc->cAllocations%6!=0))
                {
                    WARN(("unexpected cubemap texture config: (%d ; %d), allocs: %d",
                            pAllocation->SurfDesc.width, pAllocation->SurfDesc.height, pRc->cAllocations));
                    hr = E_INVALIDARG;
                }
                else
                {
                    hr = pDevice->pAdapter->D3D.D3D.pfnVBoxWineExD3DDev9CreateCubeTexture((IDirect3DDevice9Ex *)pDevice9If,
                                                pAllocation->SurfDesc.d3dWidth,
                                                VBOXDISP_CUBEMAP_LEVELS_COUNT(pRc),
                                                vboxDDI2D3DUsage(pRc->RcDesc.fFlags),
                                                vboxDDI2D3DFormat(pRc->RcDesc.enmFormat),
                                                vboxDDI2D3DPool(pRc->RcDesc.enmPool),
                                                (IDirect3DCubeTexture9**)&pD3DIfTex,
#ifdef VBOXWDDMDISP_DEBUG_NOSHARED
                                                NULL,
#else
                                                pRc->RcDesc.fFlags.SharedResource ? &hSharedHandle : NULL,
#endif
                                                pavClientMem);
                        Assert(hr == S_OK);
                        Assert(pD3DIfTex);
                        enmD3DIfType = VBOXDISP_D3DIFTYPE_CUBE_TEXTURE;
                }
            }
            else if (pRc->RcDesc.fFlags.Volume)
            {
                hr = pDevice->pAdapter->D3D.D3D.pfnVBoxWineExD3DDev9CreateVolumeTexture((IDirect3DDevice9Ex *)pDevice9If,
                                            pAllocation->SurfDesc.d3dWidth,
                                            pAllocation->SurfDesc.height,
                                            pAllocation->SurfDesc.depth,
                                            pRc->cAllocations,
                                            vboxDDI2D3DUsage(pRc->RcDesc.fFlags),
                                            vboxDDI2D3DFormat(pRc->RcDesc.enmFormat),
                                            vboxDDI2D3DPool(pRc->RcDesc.enmPool),
                                            (IDirect3DVolumeTexture9**)&pD3DIfTex,
#ifdef VBOXWDDMDISP_DEBUG_NOSHARED
                                            NULL,
#else
                                            pRc->RcDesc.fFlags.SharedResource ? &hSharedHandle : NULL,
#endif
                                            pavClientMem);
                Assert(hr == S_OK);
                Assert(pD3DIfTex);
                enmD3DIfType = VBOXDISP_D3DIFTYPE_VOLUME_TEXTURE;
            }
            else
            {
                hr = pDevice->pAdapter->D3D.D3D.pfnVBoxWineExD3DDev9CreateTexture((IDirect3DDevice9Ex *)pDevice9If,
                                            pAllocation->SurfDesc.d3dWidth,
                                            pAllocation->SurfDesc.height,
                                            pRc->cAllocations,
                                            vboxDDI2D3DUsage(pRc->RcDesc.fFlags),
                                            vboxDDI2D3DFormat(pRc->RcDesc.enmFormat),
                                            vboxDDI2D3DPool(pRc->RcDesc.enmPool),
                                            (IDirect3DTexture9**)&pD3DIfTex,
#ifdef VBOXWDDMDISP_DEBUG_NOSHARED
                                            NULL,
#else
                                            pRc->RcDesc.fFlags.SharedResource ? &hSharedHandle : NULL,
#endif
                                            pavClientMem);
                Assert(hr == S_OK);
                Assert(pD3DIfTex);
                enmD3DIfType = VBOXDISP_D3DIFTYPE_TEXTURE;
            }

            if (SUCCEEDED(hr))
            {
                Assert(pD3DIfTex);
                Assert(enmD3DIfType != VBOXDISP_D3DIFTYPE_UNDEFINED);
#ifndef VBOXWDDMDISP_DEBUG_NOSHARED
                Assert(!!(pRc->RcDesc.fFlags.SharedResource) == !!(hSharedHandle));
#endif
                for (UINT i = 0; i < pRc->cAllocations; ++i)
                {
                    PVBOXWDDMDISP_ALLOCATION pAlloc = &pRc->aAllocations[i];
                    pAlloc->enmD3DIfType = enmD3DIfType;
                    pAlloc->pD3DIf = pD3DIfTex;
                    pAlloc->hSharedHandle = hSharedHandle;
                    if (i > 0)
                        pD3DIfTex->AddRef();
                }
            }
        }

        if (pavClientMem)
            RTMemFree(pavClientMem);
    }
    else if (pRc->RcDesc.fFlags.RenderTarget || pRc->RcDesc.fFlags.Primary)
    {
        for (UINT i = 0; i < pRc->cAllocations; ++i)
        {
            PVBOXWDDMDISP_ALLOCATION pAllocation = &pRc->aAllocations[i];
            HANDLE hSharedHandle = pAllocation->hSharedHandle;
            IDirect3DSurface9 *pD3D9Surf = NULL; /* Shut up MSC. */
            if (
#ifdef VBOX_WITH_CROGL
                    (pDevice->pAdapter->u32VBox3DCaps & CR_VBOX_CAP_TEX_PRESENT) ||
#endif
                    pAllocation->enmType == VBOXWDDM_ALLOC_TYPE_UMD_RC_GENERIC)
            {
                hr = pDevice9If->CreateRenderTarget(pAllocation->SurfDesc.width,
                        pAllocation->SurfDesc.height,
                        vboxDDI2D3DFormat(pRc->RcDesc.enmFormat),
                        vboxDDI2D3DMultiSampleType(pRc->RcDesc.enmMultisampleType),
                        pRc->RcDesc.MultisampleQuality,
                        !pRc->RcDesc.fFlags.NotLockable /* BOOL Lockable */,
                        &pD3D9Surf,
#ifdef VBOXWDDMDISP_DEBUG_NOSHARED
                        NULL
#else
                        pRc->RcDesc.fFlags.SharedResource ? &hSharedHandle : NULL
#endif
                );
                Assert(hr == S_OK);
            }
            else if (pAllocation->enmType == VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE)
            {
                do {
                    BOOL bNeedPresent;
                    if (pRc->cAllocations != 1)
                    {
                        WARN(("unexpected config: more than one (%d) shared primary for rc", pRc->cAllocations));
                        hr = E_FAIL;
                        break;
                    }
                    PVBOXWDDMDISP_SWAPCHAIN pSwapchain = vboxWddmSwapchainFindCreate(pDevice, pAllocation, &bNeedPresent);
                    Assert(bNeedPresent);
                    if (!pSwapchain)
                    {
                        WARN(("vboxWddmSwapchainFindCreate failed"));
                        hr = E_OUTOFMEMORY;
                        break;
                    }

                    hr = vboxWddmSwapchainChkCreateIf(pDevice, pSwapchain);
                    if (!SUCCEEDED(hr))
                    {
                        WARN(("vboxWddmSwapchainChkCreateIf failed hr 0x%x", hr));
                        Assert(pAllocation->enmD3DIfType == VBOXDISP_D3DIFTYPE_UNDEFINED);
                        Assert(!pAllocation->pD3DIf);
                        vboxWddmSwapchainDestroy(pDevice, pSwapchain);
                        break;
                    }

                    Assert(pAllocation->enmD3DIfType == VBOXDISP_D3DIFTYPE_SURFACE);
                    Assert(pAllocation->pD3DIf);
                    pD3D9Surf = (IDirect3DSurface9*)pAllocation->pD3DIf;
                    break;
                } while (0);
            }
            else
            {
                WARN(("unexpected alloc type %d", pAllocation->enmType));
                hr = E_FAIL;
            }

            if (SUCCEEDED(hr))
            {
                Assert(pD3D9Surf);
                pAllocation->enmD3DIfType = VBOXDISP_D3DIFTYPE_SURFACE;
                pAllocation->pD3DIf = pD3D9Surf;
#ifndef VBOXWDDMDISP_DEBUG_NOSHARED
                Assert(!!(pRc->RcDesc.fFlags.SharedResource) == !!(hSharedHandle));
#endif
                pAllocation->hSharedHandle = hSharedHandle;
                hr = S_OK;
                continue;
#if 0 /* unreachable */
                /* fail branch */
                pD3D9Surf->Release();
#endif
            }

            for (UINT j = 0; j < i; ++j)
            {
                pRc->aAllocations[j].pD3DIf->Release();
            }
            break;
        }

        if (SUCCEEDED(hr))
        {
            if (pRc->RcDesc.enmPool == D3DDDIPOOL_SYSTEMMEM)
            {
                Assert(0);
                vboxDispD3DIfSurfSynchMem(pRc);
            }
        }
    }
    else if (pRc->RcDesc.fFlags.ZBuffer)
    {
        for (UINT i = 0; i < pRc->cAllocations; ++i)
        {
            PVBOXWDDMDISP_ALLOCATION pAllocation = &pRc->aAllocations[i];
            IDirect3DSurface9 *pD3D9Surf;
            hr = pDevice9If->CreateDepthStencilSurface(pAllocation->SurfDesc.width,
                    pAllocation->SurfDesc.height,
                    vboxDDI2D3DFormat(pRc->RcDesc.enmFormat),
                    vboxDDI2D3DMultiSampleType(pRc->RcDesc.enmMultisampleType),
                    pRc->RcDesc.MultisampleQuality,
                    TRUE /** @todo BOOL Discard */,
                    &pD3D9Surf,
                    NULL /*HANDLE* pSharedHandle*/);
            Assert(hr == S_OK);
            if (hr == S_OK)
            {
                Assert(pD3D9Surf);
                pAllocation->enmD3DIfType = VBOXDISP_D3DIFTYPE_SURFACE;
                pAllocation->pD3DIf = pD3D9Surf;
            }
            else
            {
                for (UINT j = 0; j < i; ++j)
                {
                    pRc->aAllocations[j].pD3DIf->Release();
                }
                break;
            }
        }

        if (SUCCEEDED(hr))
        {
            if (pRc->RcDesc.enmPool == D3DDDIPOOL_SYSTEMMEM)
            {
                vboxDispD3DIfSurfSynchMem(pRc);
            }
        }
    }
    else if (pRc->RcDesc.fFlags.VertexBuffer)
    {
        for (UINT i = 0; i < pRc->cAllocations; ++i)
        {
            PVBOXWDDMDISP_ALLOCATION pAllocation = &pRc->aAllocations[i];
            IDirect3DVertexBuffer9  *pD3D9VBuf;
            hr = pDevice9If->CreateVertexBuffer(pAllocation->SurfDesc.width,
                    vboxDDI2D3DUsage(pRc->RcDesc.fFlags)
                    & (~D3DUSAGE_DYNAMIC) /* <- avoid using dynamic to ensure wine does not switch do user buffer */
                    ,
                    pRc->RcDesc.Fvf,
                    vboxDDI2D3DPool(pRc->RcDesc.enmPool),
                    &pD3D9VBuf,
                    NULL /*HANDLE* pSharedHandle*/);
            Assert(hr == S_OK);
            if (hr == S_OK)
            {
                Assert(pD3D9VBuf);
                pAllocation->enmD3DIfType = VBOXDISP_D3DIFTYPE_VERTEXBUFFER;
                pAllocation->pD3DIf = pD3D9VBuf;
            }
            else
            {
                for (UINT j = 0; j < i; ++j)
                {
                    pRc->aAllocations[j].pD3DIf->Release();
                }
                break;
            }
        }

        if (SUCCEEDED(hr))
        {
            if (pRc->RcDesc.enmPool == D3DDDIPOOL_SYSTEMMEM)
            {
                vboxDispD3DIfSurfSynchMem(pRc);
            }
        }
    }
    else if (pRc->RcDesc.fFlags.IndexBuffer)
    {
        for (UINT i = 0; i < pRc->cAllocations; ++i)
        {
            PVBOXWDDMDISP_ALLOCATION pAllocation = &pRc->aAllocations[i];
            IDirect3DIndexBuffer9  *pD3D9IBuf;
            hr = pDevice9If->CreateIndexBuffer(pAllocation->SurfDesc.width,
                    vboxDDI2D3DUsage(pRc->RcDesc.fFlags),
                    vboxDDI2D3DFormat(pRc->RcDesc.enmFormat),
                    vboxDDI2D3DPool(pRc->RcDesc.enmPool),
                    &pD3D9IBuf,
                    NULL /*HANDLE* pSharedHandle*/
                  );
            Assert(hr == S_OK);
            if (hr == S_OK)
            {
                Assert(pD3D9IBuf);
                pAllocation->enmD3DIfType = VBOXDISP_D3DIFTYPE_INDEXBUFFER;
                pAllocation->pD3DIf = pD3D9IBuf;
            }
            else
            {
                for (UINT j = 0; j < i; ++j)
                {
                    pRc->aAllocations[j].pD3DIf->Release();
                }
                break;
            }
        }

        if (SUCCEEDED(hr))
        {
            if (pRc->RcDesc.enmPool == D3DDDIPOOL_SYSTEMMEM)
            {
                vboxDispD3DIfSurfSynchMem(pRc);
            }
        }
    }
    else
    {
        hr = E_FAIL;
        WARN(("unsupported rc flags, %d", pRc->RcDesc.fFlags.Value));
    }

    return hr;
}

VOID VBoxD3DIfFillPresentParams(D3DPRESENT_PARAMETERS *pParams, PVBOXWDDMDISP_RESOURCE pRc, UINT cRTs)
{
    Assert(cRTs);
    memset(pParams, 0, sizeof (D3DPRESENT_PARAMETERS));
    pParams->BackBufferWidth = pRc->aAllocations[0].SurfDesc.width;
    pParams->BackBufferHeight = pRc->aAllocations[0].SurfDesc.height;
    pParams->BackBufferFormat = vboxDDI2D3DFormat(pRc->aAllocations[0].SurfDesc.format);
    pParams->BackBufferCount = cRTs - 1;
    pParams->MultiSampleType = vboxDDI2D3DMultiSampleType(pRc->RcDesc.enmMultisampleType);
    pParams->MultiSampleQuality = pRc->RcDesc.MultisampleQuality;
#if 0 //def VBOXDISP_WITH_WINE_BB_WORKAROUND /* this does not work so far any way :( */
    if (cRTs == 1)
        pParams->SwapEffect = D3DSWAPEFFECT_COPY;
    else
#endif
    if (pRc->RcDesc.fFlags.DiscardRenderTarget)
        pParams->SwapEffect = D3DSWAPEFFECT_DISCARD;
    pParams->Windowed = TRUE;
}

HRESULT VBoxD3DIfDeviceCreateDummy(PVBOXWDDMDISP_DEVICE pDevice)
{
    VBOXWDDMDISP_RESOURCE Rc;
    vboxWddmResourceInit(&Rc, 1);

    Rc.RcDesc.enmFormat = D3DDDIFMT_A8R8G8B8;
    Rc.RcDesc.enmPool = D3DDDIPOOL_LOCALVIDMEM;
    Rc.RcDesc.enmMultisampleType = D3DDDIMULTISAMPLE_NONE;
    Rc.RcDesc.MultisampleQuality = 0;
    PVBOXWDDMDISP_ALLOCATION pAlloc = &Rc.aAllocations[0];
    pAlloc->enmD3DIfType = VBOXDISP_D3DIFTYPE_SURFACE;
    pAlloc->SurfDesc.width = 0x4;
    pAlloc->SurfDesc.height = 0x4;
    pAlloc->SurfDesc.format = D3DDDIFMT_A8R8G8B8;
    Assert(!pDevice->pDevice9If);
    VBOXWINEEX_D3DPRESENT_PARAMETERS Params;
    VBoxD3DIfFillPresentParams(&Params.Base, &Rc, 2);
#ifdef VBOX_WITH_CRHGSMI
    Params.pHgsmi = &pDevice->Uhgsmi.BasePrivate.Base;
#else
    Params.pHgsmi = NULL;
#endif
    DWORD fFlags =   D3DCREATE_HARDWARE_VERTEXPROCESSING
                   | D3DCREATE_FPU_PRESERVE; /* Do not allow Wine to mess with FPU control word. */
    PVBOXWDDMDISP_ADAPTER pAdapter = pDevice->pAdapter;
    IDirect3DDevice9 * pDevice9If = NULL;

    HRESULT hr = pAdapter->D3D.pD3D9If->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, fFlags, &Params.Base, &pDevice9If);
    if (SUCCEEDED(hr))
    {
        int32_t hostId = 0;
        hr = pAdapter->D3D.D3D.pfnVBoxWineExD3DDev9GetHostId((IDirect3DDevice9Ex*)pDevice9If, &hostId);
        if (SUCCEEDED(hr))
        {
            Assert(hostId);

            VBOXDISPIFESCAPE Data;
            Data.escapeCode = VBOXESC_SETCTXHOSTID;
            Data.u32CmdSpecific = (uint32_t)hostId;
            D3DDDICB_ESCAPE DdiEscape = {0};
            DdiEscape.hContext = pDevice->DefaultContext.ContextInfo.hContext;
            DdiEscape.hDevice = pDevice->hDevice;
        //    DdiEscape.Flags.Value = 0;
            DdiEscape.pPrivateDriverData = &Data;
            DdiEscape.PrivateDriverDataSize = sizeof (Data);
            hr = pDevice->RtCallbacks.pfnEscapeCb(pDevice->pAdapter->hAdapter, &DdiEscape);
            if (SUCCEEDED(hr))
            {
                pDevice->pDevice9If = pDevice9If;
                return S_OK;
            }
            else
                WARN(("pfnEscapeCb VBOXESC_SETCTXHOSTID failed hr 0x%x", hr));
        }
        else
            WARN(("pfnVBoxWineExD3DDev9GetHostId failed hr 0x%x", hr));

        pDevice->pAdapter->D3D.D3D.pfnVBoxWineExD3DDev9Term((IDirect3DDevice9Ex *)pDevice9If);
    }
    else
        WARN(("CreateDevice failed hr 0x%x", hr));

    return hr;
}

int vboxD3DIfSetHostId(PVBOXWDDMDISP_ALLOCATION pAlloc, uint32_t hostID, uint32_t *pHostID)
{
    struct VBOXWDDMDISP_RESOURCE *pRc = pAlloc->pRc;
    PVBOXWDDMDISP_DEVICE pDevice = pRc->pDevice;

    VBOXDISPIFESCAPE_SETALLOCHOSTID SetHostID = {0};
    SetHostID.EscapeHdr.escapeCode = VBOXESC_SETALLOCHOSTID;
    SetHostID.hostID = hostID;
    SetHostID.hAlloc = pAlloc->hAllocation;

    D3DDDICB_ESCAPE DdiEscape = {0};
    DdiEscape.hContext = pDevice->DefaultContext.ContextInfo.hContext;
    DdiEscape.hDevice = pDevice->hDevice;
    DdiEscape.Flags.Value = 0;
    DdiEscape.Flags.HardwareAccess = 1;
    DdiEscape.pPrivateDriverData = &SetHostID;
    DdiEscape.PrivateDriverDataSize = sizeof (SetHostID);
    HRESULT hr = pDevice->RtCallbacks.pfnEscapeCb(pDevice->pAdapter->hAdapter, &DdiEscape);
    if (SUCCEEDED(hr))
    {
        if (pHostID)
            *pHostID = SetHostID.EscapeHdr.u32CmdSpecific;

        return SetHostID.rc;
    }
    else
        WARN(("pfnEscapeCb VBOXESC_SETALLOCHOSTID failed hr 0x%x", hr));

    return VERR_GENERAL_FAILURE;
}

IUnknown* vboxD3DIfCreateSharedPrimary(PVBOXWDDMDISP_ALLOCATION pAlloc)
{
    IDirect3DSurface9 *pSurfIf;
    struct VBOXWDDMDISP_RESOURCE *pRc = pAlloc->pRc;
    PVBOXWDDMDISP_DEVICE pDevice = pRc->pDevice;

    HRESULT hr = VBoxD3DIfCreateForRc(pRc);
    if (!SUCCEEDED(hr))
    {
        WARN(("VBoxD3DIfCreateForRc failed, hr 0x%x", hr));
        return NULL;
    }

    Assert(pAlloc->pD3DIf);
    Assert(pAlloc->enmD3DIfType == VBOXDISP_D3DIFTYPE_SURFACE);
    Assert(pAlloc->pRc->RcDesc.fFlags.SharedResource);

    hr = VBoxD3DIfSurfGet(pRc, pAlloc->iAlloc, &pSurfIf);
    if (!SUCCEEDED(hr))
    {
        WARN(("VBoxD3DIfSurfGet failed hr %#x", hr));
        return NULL;
    }

    uint32_t hostID, usedHostId;
    hr = pDevice->pAdapter->D3D.D3D.pfnVBoxWineExD3DSurf9GetHostId(pSurfIf, &hostID);
    if (SUCCEEDED(hr))
    {
        Assert(hostID);
        int rc = vboxD3DIfSetHostId(pAlloc, hostID, &usedHostId);
        if (!RT_SUCCESS(rc))
        {
            if (rc == VERR_NOT_EQUAL)
            {
                WARN(("another hostId % is in use, using it instead", usedHostId));
                Assert(hostID != usedHostId);
                Assert(usedHostId);
                pSurfIf->Release();
                pSurfIf = NULL;
                for (UINT i = 0; i < pRc->cAllocations; ++i)
                {
                    PVBOXWDDMDISP_ALLOCATION pCurAlloc = &pRc->aAllocations[i];
                    if (pCurAlloc->pD3DIf)
                    {
                        pCurAlloc->pD3DIf->Release();
                        pCurAlloc->pD3DIf = NULL;
                    }
                }

                pAlloc->hSharedHandle = (HANDLE)(uintptr_t)usedHostId;

                hr = VBoxD3DIfCreateForRc(pRc);
                if (!SUCCEEDED(hr))
                {
                    WARN(("VBoxD3DIfCreateForRc failed, hr 0x%x", hr));
                    return NULL;
                }

                hr = VBoxD3DIfSurfGet(pRc, pAlloc->iAlloc, &pSurfIf);
                if (!SUCCEEDED(hr))
                {
                    WARN(("VBoxD3DIfSurfGet failed hr %#x", hr));
                    return NULL;
                }
            }
            else
            {
                WARN(("vboxD3DIfSetHostId failed %#x, ignoring", hr));
                hr = S_OK;
                hostID = 0;
                usedHostId = 0;
            }
        }
        else
        {
            Assert(hostID == usedHostId);
        }
    }
    else
        WARN(("pfnVBoxWineExD3DSurf9GetHostId failed, hr 0x%x", hr));

    pSurfIf->Release();
    pSurfIf = NULL;

    return pAlloc->pD3DIf;
}
