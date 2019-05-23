/* $Id: VBoxD3DIf.h $ */
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

#ifndef ___VBoxDispD3DRcIf_h__
#define ___VBoxDispD3DRcIf_h__

#include "VBoxDispD3DCmn.h"

D3DFORMAT vboxDDI2D3DFormat(D3DDDIFORMAT format);
D3DMULTISAMPLE_TYPE vboxDDI2D3DMultiSampleType(D3DDDIMULTISAMPLE_TYPE enmType);
D3DPOOL vboxDDI2D3DPool(D3DDDI_POOL enmPool);
D3DRENDERSTATETYPE vboxDDI2D3DRenderStateType(D3DDDIRENDERSTATETYPE enmType);
VBOXWDDMDISP_TSS_LOOKUP vboxDDI2D3DTestureStageStateType(D3DDDITEXTURESTAGESTATETYPE enmType);
DWORD vboxDDI2D3DUsage(D3DDDI_RESOURCEFLAGS fFlags);
DWORD vboxDDI2D3DLockFlags(D3DDDI_LOCKFLAGS fLockFlags);
D3DTEXTUREFILTERTYPE vboxDDI2D3DBltFlags(D3DDDI_BLTFLAGS fFlags);
D3DQUERYTYPE vboxDDI2D3DQueryType(D3DDDIQUERYTYPE enmType);
DWORD vboxDDI2D3DIssueQueryFlags(D3DDDI_ISSUEQUERYFLAGS Flags);

HRESULT VBoxD3DIfCreateForRc(struct VBOXWDDMDISP_RESOURCE *pRc);
HRESULT VBoxD3DIfLockRect(struct VBOXWDDMDISP_RESOURCE *pRc, UINT iAlloc,
        D3DLOCKED_RECT * pLockedRect,
        CONST RECT *pRect,
        DWORD fLockFlags);
HRESULT VBoxD3DIfUnlockRect(struct VBOXWDDMDISP_RESOURCE *pRc, UINT iAlloc);
void VBoxD3DIfLockUnlockMemSynch(struct VBOXWDDMDISP_ALLOCATION *pAlloc, D3DLOCKED_RECT *pLockInfo, RECT *pRect, bool bToLockInfo);

IUnknown* vboxD3DIfCreateSharedPrimary(PVBOXWDDMDISP_ALLOCATION pAlloc);


/* NOTE: does NOT increment a ref counter! NO Release needed!! */
DECLINLINE(IUnknown*) vboxD3DIfGet(PVBOXWDDMDISP_ALLOCATION pAlloc)
{
    if (pAlloc->pD3DIf)
        return pAlloc->pD3DIf;

    if (pAlloc->enmType != VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE)
    {
        WARN(("dynamic creation is supported for VBOXWDDM_ALLOC_TYPE_STD_SHAREDPRIMARYSURFACE only!, current type is %d", pAlloc->enmType));
        return NULL;
    }

    return vboxD3DIfCreateSharedPrimary(pAlloc);
}

/* on success increments the surface ref counter,
 * i.e. one must call pSurf->Release() once the surface is not needed*/
DECLINLINE(HRESULT) VBoxD3DIfSurfGet(PVBOXWDDMDISP_RESOURCE pRc, UINT iAlloc, IDirect3DSurface9 **ppSurf)
{
    HRESULT hr = S_OK;
    Assert(pRc->cAllocations > iAlloc);
    *ppSurf = NULL;
    IUnknown* pD3DIf = vboxD3DIfGet(&pRc->aAllocations[iAlloc]);

    switch (pRc->aAllocations[0].enmD3DIfType)
    {
        case VBOXDISP_D3DIFTYPE_SURFACE:
        {
            IDirect3DSurface9 *pD3DIfSurf = (IDirect3DSurface9*)pD3DIf;
            Assert(pD3DIfSurf);
            pD3DIfSurf->AddRef();
            *ppSurf = pD3DIfSurf;
            break;
        }
        case VBOXDISP_D3DIFTYPE_TEXTURE:
        {
            /* @todo VBoxD3DIfSurfGet is typically used in Blt & ColorFill functions
             * in this case, if texture is used as a destination,
             * we should update sub-layers as well which is not done currently. */
            IDirect3DTexture9 *pD3DIfTex = (IDirect3DTexture9*)pD3DIf;
            IDirect3DSurface9 *pSurfaceLevel;
            Assert(pD3DIfTex);
            hr = pD3DIfTex->GetSurfaceLevel(iAlloc, &pSurfaceLevel);
            Assert(hr == S_OK);
            if (hr == S_OK)
            {
                *ppSurf = pSurfaceLevel;
            }
            break;
        }
        case VBOXDISP_D3DIFTYPE_CUBE_TEXTURE:
        {
            IDirect3DCubeTexture9 *pD3DIfCubeTex = (IDirect3DCubeTexture9*)pD3DIf;
            IDirect3DSurface9 *pSurfaceLevel;
            Assert(pD3DIfCubeTex);
            hr = pD3DIfCubeTex->GetCubeMapSurface(VBOXDISP_CUBEMAP_INDEX_TO_FACE(pRc, iAlloc),
                                                  VBOXDISP_CUBEMAP_INDEX_TO_LEVEL(pRc, iAlloc), &pSurfaceLevel);
            Assert(hr == S_OK);
            if (hr == S_OK)
            {
                *ppSurf = pSurfaceLevel;
            }
            break;
        }
        default:
        {
            WARN(("unexpected enmD3DIfType %d", pRc->aAllocations[0].enmD3DIfType));
            hr = E_FAIL;
            break;
        }
    }
    return hr;
}

VOID VBoxD3DIfFillPresentParams(D3DPRESENT_PARAMETERS *pParams, PVBOXWDDMDISP_RESOURCE pRc, UINT cRTs);
HRESULT VBoxD3DIfDeviceCreateDummy(PVBOXWDDMDISP_DEVICE pDevice);

DECLINLINE(IDirect3DDevice9*) VBoxD3DIfDeviceGet(PVBOXWDDMDISP_DEVICE pDevice)
{
    if (pDevice->pDevice9If)
        return pDevice->pDevice9If;

#ifdef VBOXWDDMDISP_DEBUG
    g_VBoxVDbgInternalDevice = pDevice;
#endif

    HRESULT hr = VBoxD3DIfDeviceCreateDummy(pDevice);
    Assert(hr == S_OK); NOREF(hr);
    Assert(pDevice->pDevice9If);
    return pDevice->pDevice9If;
}

#define VBOXDISPMODE_IS_3D(_p) (!!((_p)->D3D.pD3D9If))
#ifdef VBOXDISP_EARLYCREATEDEVICE
#define VBOXDISP_D3DEV(_p) (_p)->pDevice9If
#else
#define VBOXDISP_D3DEV(_p) VBoxD3DIfDeviceGet(_p)
#endif

#endif /* ___VBoxDispD3DRcIf_h__ */
