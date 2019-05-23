/* $Id: VBoxDispD3DIf.cpp $ */
/** @file
 * VBoxVideo Display D3D User mode dll
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

#include "VBoxDispD3DIf.h"
#include "VBoxDispD3DCmn.h"

#include <iprt/assert.h>

/** Convert a given FourCC code to a D3DDDIFORMAT enum. */
#define VBOXWDDM_D3DDDIFORMAT_FROM_FOURCC(_a, _b, _c, _d) \
    ((D3DDDIFORMAT)MAKEFOURCC(_a, _b, _c, _d))

void VBoxDispD3DClose(VBOXDISPD3D *pD3D)
{
    FreeLibrary(pD3D->hD3DLib);
    pD3D->hD3DLib = NULL;
}

/**
 * Loads a system DLL.
 *
 * @returns Module handle or NULL
 * @param   pszName             The DLL name.
 */
static HMODULE loadSystemDll(const char *pszName)
{
    /* Assumed here that DEBUG version of VBoxDispD3D.dll will not be installed system wide
       but will be loaded from local folder of an application being debugged. */
#ifndef DEBUG
    char   szPath[MAX_PATH];
    UINT   cchPath = GetSystemDirectoryA(szPath, sizeof(szPath));
    size_t cbName  = strlen(pszName) + 1;
    if (cchPath + 1 + cbName > sizeof(szPath))
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return NULL;
    }
    szPath[cchPath] = '\\';
    memcpy(&szPath[cchPath + 1], pszName, cbName);
    return LoadLibraryA(szPath);
#else
    return LoadLibraryA(pszName);
#endif
}

HRESULT VBoxDispD3DOpen(VBOXDISPD3D *pD3D)
{
#ifdef VBOX_WDDM_WOW64
    pD3D->hD3DLib = loadSystemDll("VBoxD3D9wddm-x86.dll");
#else
    pD3D->hD3DLib = loadSystemDll("VBoxD3D9wddm.dll");
#endif
    if (!pD3D->hD3DLib)
    {
        DWORD winErr = GetLastError(); NOREF(winErr);
        WARN((__FUNCTION__": LoadLibrary failed, winErr = (%d)", winErr));
        return E_FAIL;
    }

    do
    {
        pD3D->pfnDirect3DCreate9Ex = (PFNVBOXDISPD3DCREATE9EX)GetProcAddress(pD3D->hD3DLib, "Direct3DCreate9Ex");
        if (!pD3D->pfnDirect3DCreate9Ex)
        {
            WARN(("no Direct3DCreate9Ex"));
            break;
        }

        pD3D->pfnVBoxWineExD3DDev9CreateTexture = (PFNVBOXWINEEXD3DDEV9_CREATETEXTURE)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DDev9CreateTexture");
        if (!pD3D->pfnVBoxWineExD3DDev9CreateTexture)
        {
            WARN(("no VBoxWineExD3DDev9CreateTexture"));
            break;
        }

        pD3D->pfnVBoxWineExD3DDev9CreateCubeTexture = (PFNVBOXWINEEXD3DDEV9_CREATECUBETEXTURE)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DDev9CreateCubeTexture");
        if (!pD3D->pfnVBoxWineExD3DDev9CreateCubeTexture)
        {
            WARN(("no VBoxWineExD3DDev9CreateCubeTexture"));
            break;
        }

        pD3D->pfnVBoxWineExD3DDev9CreateVolumeTexture = (PFNVBOXWINEEXD3DDEV9_CREATEVOLUMETEXTURE)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DDev9CreateVolumeTexture");
        if (!pD3D->pfnVBoxWineExD3DDev9CreateVolumeTexture)
        {
            WARN(("no VBoxWineExD3DDev9CreateVolumeTexture"));
            break;
        }

        pD3D->pfnVBoxWineExD3DDev9Flush = (PFNVBOXWINEEXD3DDEV9_FLUSH)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DDev9Flush");
        if (!pD3D->pfnVBoxWineExD3DDev9Flush)
        {
            WARN(("no VBoxWineExD3DDev9Flush"));
            break;
        }

        pD3D->pfnVBoxWineExD3DDev9FlushToHost = (PFNVBOXWINEEXD3DDEV9_FLUSHTOHOST)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DDev9FlushToHost");
        if (!pD3D->pfnVBoxWineExD3DDev9FlushToHost)
        {
            WARN(("no VBoxWineExD3DDev9FlushToHost"));
            break;
        }

        pD3D->pfnVBoxWineExD3DDev9Finish = (PFNVBOXWINEEXD3DDEV9_FINISH)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DDev9Finish");
        if (!pD3D->pfnVBoxWineExD3DDev9Finish)
        {
            WARN(("no VBoxWineExD3DDev9Finish"));
            break;
        }

        pD3D->pfnVBoxWineExD3DDev9VolBlt = (PFNVBOXWINEEXD3DDEV9_VOLBLT)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DDev9VolBlt");
        if (!pD3D->pfnVBoxWineExD3DDev9VolBlt)
        {
            WARN(("no VBoxWineExD3DDev9VolBlt"));
            break;
        }

        pD3D->pfnVBoxWineExD3DDev9VolTexBlt = (PFNVBOXWINEEXD3DDEV9_VOLTEXBLT)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DDev9VolTexBlt");
        if (!pD3D->pfnVBoxWineExD3DDev9VolTexBlt)
        {
            WARN(("no VBoxWineExD3DDev9VolTexBlt"));
            break;
        }

        pD3D->pfnVBoxWineExD3DDev9Term = (PFNVBOXWINEEXD3DDEV9_TERM)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DDev9Term");
        if (!pD3D->pfnVBoxWineExD3DDev9Term)
        {
            WARN(("no VBoxWineExD3DDev9Term"));
            break;
        }

        pD3D->pfnVBoxWineExD3DSwapchain9Present = (PFNVBOXWINEEXD3DSWAPCHAIN9_PRESENT)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DSwapchain9Present");
        if (!pD3D->pfnVBoxWineExD3DSwapchain9Present)
        {
            WARN(("no VBoxWineExD3DSwapchain9Present"));
            break;
        }

        pD3D->pfnVBoxWineExD3DSurf9GetHostId = (PFNVBOXWINEEXD3DSURF9_GETHOSTID)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DSurf9GetHostId");
        if (!pD3D->pfnVBoxWineExD3DSurf9GetHostId)
        {
            WARN(("no VBoxWineExD3DSurf9GetHostId"));
            break;
        }

        pD3D->pfnVBoxWineExD3DSurf9SyncToHost = (PFNVBOXWINEEXD3DSURF9_SYNCTOHOST)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DSurf9SyncToHost");
        if (!pD3D->pfnVBoxWineExD3DSurf9SyncToHost)
        {
            WARN(("no VBoxWineExD3DSurf9SyncToHost"));
            break;
        }

        pD3D->pfnVBoxWineExD3DSwapchain9GetHostWinID = (PFNVBOXWINEEXD3DSWAPCHAIN9_GETHOSTWINID)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DSwapchain9GetHostWinID");
        if (!pD3D->pfnVBoxWineExD3DSwapchain9GetHostWinID)
        {
            WARN(("no VBoxWineExD3DSwapchain9GetHostWinID"));
            break;
        }

        pD3D->pfnVBoxWineExD3DDev9GetHostId = (PFNVBOXWINEEXD3DDEV9_GETHOSTID)GetProcAddress(pD3D->hD3DLib, "VBoxWineExD3DDev9GetHostId");
        if (!pD3D->pfnVBoxWineExD3DDev9GetHostId)
        {
            WARN(("no VBoxWineExD3DDev9GetHostId"));
            break;
        }

        return S_OK;

    } while (0);

    VBoxDispD3DClose(pD3D);

    return E_FAIL;
}



static FORMATOP gVBoxFormatOps3D[] = {
    {D3DDDIFMT_A8R8G8B8,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        FORMATOP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET|
        FORMATOP_CONVERT_TO_ARGB|FORMATOP_OFFSCREENPLAIN|FORMATOP_SRGBREAD|
        FORMATOP_MEMBEROFGROUP_ARGB|
        FORMATOP_SRGBWRITE|FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_X8R8G8B8,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        FORMATOP_DISPLAYMODE|FORMATOP_3DACCELERATION|
        FORMATOP_CONVERT_TO_ARGB|FORMATOP_OFFSCREENPLAIN|FORMATOP_SRGBREAD|
        FORMATOP_MEMBEROFGROUP_ARGB|
        FORMATOP_SRGBWRITE|FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_A2R10G10B10,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        0|
        FORMATOP_CONVERT_TO_ARGB|FORMATOP_OFFSCREENPLAIN|
        FORMATOP_MEMBEROFGROUP_ARGB|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_X1R5G5B5,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        0|
        FORMATOP_CONVERT_TO_ARGB|FORMATOP_OFFSCREENPLAIN|
        FORMATOP_MEMBEROFGROUP_ARGB|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_A1R5G5B5,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        FORMATOP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET|
        FORMATOP_CONVERT_TO_ARGB|FORMATOP_OFFSCREENPLAIN|
        FORMATOP_MEMBEROFGROUP_ARGB|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_A4R4G4B4,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        FORMATOP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_R5G6B5,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        FORMATOP_DISPLAYMODE|FORMATOP_3DACCELERATION|
        FORMATOP_CONVERT_TO_ARGB|FORMATOP_OFFSCREENPLAIN|
        FORMATOP_MEMBEROFGROUP_ARGB|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_L16,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_A8L8,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_A8,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_L8,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_D16,   FORMATOP_TEXTURE|FORMATOP_ZSTENCIL|FORMATOP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH, 0, 0, 0},
    {D3DDDIFMT_D24S8, FORMATOP_TEXTURE|FORMATOP_ZSTENCIL|FORMATOP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH, 0, 0, 0},
    {D3DDDIFMT_D24X8, FORMATOP_TEXTURE|FORMATOP_ZSTENCIL|FORMATOP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH, 0, 0, 0},
    {D3DDDIFMT_D16_LOCKABLE, FORMATOP_ZSTENCIL|FORMATOP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH, 0, 0, 0},
    {D3DDDIFMT_X8D24, FORMATOP_TEXTURE|FORMATOP_ZSTENCIL|FORMATOP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH, 0, 0, 0},
    {D3DDDIFMT_D32F_LOCKABLE, FORMATOP_TEXTURE|FORMATOP_ZSTENCIL|FORMATOP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH, 0, 0, 0},
    {D3DDDIFMT_S8D24, FORMATOP_TEXTURE|FORMATOP_ZSTENCIL|FORMATOP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH, 0, 0, 0},

    {D3DDDIFMT_DXT1,
        FORMATOP_TEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        FORMATOP_OFFSCREENPLAIN|FORMATOP_SRGBREAD|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_DXT2,
        FORMATOP_TEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        FORMATOP_OFFSCREENPLAIN|FORMATOP_SRGBREAD|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_DXT3,
        FORMATOP_TEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        FORMATOP_OFFSCREENPLAIN|FORMATOP_SRGBREAD|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_DXT4,
        FORMATOP_TEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        FORMATOP_OFFSCREENPLAIN|FORMATOP_SRGBREAD|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_DXT5,
        FORMATOP_TEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        FORMATOP_OFFSCREENPLAIN|FORMATOP_SRGBREAD|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_X8L8V8U8,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        0|
        FORMATOP_BUMPMAP|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_A2W10V10U10,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        0|
        FORMATOP_BUMPMAP|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_V8U8,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        0|
        FORMATOP_BUMPMAP|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_Q8W8V8U8,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        0|
        0|
        FORMATOP_OFFSCREENPLAIN|
        FORMATOP_BUMPMAP|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_CxV8U8, FORMATOP_NOFILTER|FORMATOP_NOALPHABLEND|FORMATOP_NOTEXCOORDWRAPNORMIP, 0, 0, 0},

    {D3DDDIFMT_R16F,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        0|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_R32F,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        0|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_G16R16F,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        0|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_G32R32F,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        0|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

// Floating-point formats are not implemented in Chromium.
    {D3DDDIFMT_A16B16G16R16F,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        0|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_A32B32G32R32F,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        0|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_G16R16,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        0|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_A16B16G16R16,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        0|
        FORMATOP_OFFSCREENPLAIN|
        0|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_V16U16,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        0|
        FORMATOP_BUMPMAP|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_P8, FORMATOP_DISPLAYMODE|FORMATOP_3DACCELERATION|FORMATOP_OFFSCREENPLAIN, 0, 0, 0},

    {D3DDDIFMT_UYVY,
        0|
        0|
        0|
        FORMATOP_CONVERT_TO_ARGB|FORMATOP_OFFSCREENPLAIN|
        FORMATOP_NOFILTER|
        FORMATOP_NOALPHABLEND|
        FORMATOP_NOTEXCOORDWRAPNORMIP, 0, 0, 0},

    {D3DDDIFMT_YUY2,
        0|
        0|
        0|
        FORMATOP_CONVERT_TO_ARGB|FORMATOP_OFFSCREENPLAIN|
        FORMATOP_NOFILTER|
        FORMATOP_NOALPHABLEND|
        FORMATOP_NOTEXCOORDWRAPNORMIP, 0, 0, 0},

    {VBOXWDDM_D3DDDIFORMAT_FROM_FOURCC('Y', 'V', '1', '2'),
        0|
        0|
        0|
        FORMATOP_CONVERT_TO_ARGB|FORMATOP_OFFSCREENPLAIN|
        FORMATOP_NOFILTER|
        FORMATOP_NOALPHABLEND|
        FORMATOP_NOTEXCOORDWRAPNORMIP, 0, 0, 0},

    {D3DDDIFMT_Q16W16V16U16,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        0|
        FORMATOP_OFFSCREENPLAIN|
        FORMATOP_BUMPMAP|FORMATOP_DMAP|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_X8B8G8R8,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        FORMATOP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET|
        FORMATOP_CONVERT_TO_ARGB|FORMATOP_OFFSCREENPLAIN|FORMATOP_SRGBREAD|
        FORMATOP_DMAP|FORMATOP_MEMBEROFGROUP_ARGB|
        FORMATOP_SRGBWRITE|FORMATOP_AUTOGENMIPMAP|FORMATOP_VERTEXTEXTURE|
        FORMATOP_OVERLAY, 0, 0, 0},

    {D3DDDIFMT_BINARYBUFFER, FORMATOP_OFFSCREENPLAIN, 0, 0, 0},

    {D3DDDIFMT_A4L4,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|
        0|
        0|
        FORMATOP_CONVERT_TO_ARGB|FORMATOP_OFFSCREENPLAIN|
        FORMATOP_DMAP|
        FORMATOP_VERTEXTEXTURE, 0, 0, 0},

    {D3DDDIFMT_A2B10G10R10,
        FORMATOP_TEXTURE|FORMATOP_VOLUMETEXTURE|FORMATOP_CUBETEXTURE|FORMATOP_OFFSCREEN_RENDERTARGET|
        FORMATOP_SAME_FORMAT_RENDERTARGET|
        0|
        FORMATOP_CONVERT_TO_ARGB|FORMATOP_OFFSCREENPLAIN|
        FORMATOP_DMAP|FORMATOP_MEMBEROFGROUP_ARGB|
        FORMATOP_AUTOGENMIPMAP|FORMATOP_VERTEXTEXTURE, 0, 0, 0},
};

static FORMATOP gVBoxFormatOpsBase[] = {
    {D3DDDIFMT_X8R8G8B8, FORMATOP_DISPLAYMODE, 0, 0, 0},

    {D3DDDIFMT_R8G8B8, FORMATOP_DISPLAYMODE, 0, 0, 0},

    {D3DDDIFMT_R5G6B5, FORMATOP_DISPLAYMODE, 0, 0, 0},

    {D3DDDIFMT_P8, FORMATOP_DISPLAYMODE, 0, 0, 0},
};

static DDSURFACEDESC gVBoxSurfDescsBase[] = {
        {
            sizeof (DDSURFACEDESC), /*    DWORD   dwSize;  */
            DDSD_CAPS | DDSD_PIXELFORMAT,    /* DWORD dwFlags;    */
            0,    /* DWORD dwHeight;   */
            0,    /* DWORD dwWidth;    */
            {
                0, /* Union             */
                   /*   LONG lPitch; */
                   /*   DWORD dwLinearSize; */
            },
            0,  /*    DWORD dwBackBufferCount; */
            {
                0, /* Union */
                   /*  DWORD dwMipMapCount; */
                   /*    DWORD dwZBufferBitDepth; */
                   /*   DWORD dwRefreshRate; */
            },
            0, /*    DWORD dwAlphaBitDepth; */
            0, /*   DWORD dwReserved; */
            NULL, /*   LPVOID lpSurface; */
            {
                0, /* DWORD dwColorSpaceLowValue; */
                0, /* DWORD dwColorSpaceHighValue; */
            }, /* DDCOLORKEY  ddckCKDestOverlay; */
            {
                0, /* DWORD dwColorSpaceLowValue; */
                0, /* DWORD dwColorSpaceHighValue; */
            }, /* DDCOLORKEY  ddckCKDestBlt; */
            {
                0, /* DWORD dwColorSpaceLowValue; */
                0, /* DWORD dwColorSpaceHighValue; */
            }, /* DDCOLORKEY  ddckCKSrcOverlay; */
            {
                0, /* DWORD dwColorSpaceLowValue; */
                0, /* DWORD dwColorSpaceHighValue; */
            }, /* DDCOLORKEY ddckCKSrcBlt; */
            {
                sizeof (DDPIXELFORMAT), /* DWORD dwSize; */
                DDPF_RGB, /* DWORD dwFlags; */
                0, /* DWORD dwFourCC; */
                {
                    32, /* union */
                       /* DWORD dwRGBBitCount; */
                       /* DWORD dwYUVBitCount; */
                       /* DWORD dwZBufferBitDepth; */
                       /* DWORD dwAlphaBitDepth; */
                       /* DWORD dwLuminanceBitCount; */
                       /* DWORD dwBumpBitCount; */
                },
                {
                    0xff0000, /* union */
                       /* DWORD dwRBitMask; */
                       /* DWORD dwYBitMask; */
                        /* DWORD dwStencilBitDepth; */
                        /* DWORD dwLuminanceBitMask; */
                        /* DWORD dwBumpDuBitMask; */
                },
                {
                    0xff00,
                        /* DWORD dwGBitMask; */
                        /* DWORD dwUBitMask; */
                        /* DWORD dwZBitMask; */
                        /* DWORD dwBumpDvBitMask; */
                },
                {
                    0xff,
                        /* DWORD dwBBitMask; */
                        /* DWORD dwVBitMask; */
                        /* DWORD dwStencilBitMask; */
                        /* DWORD dwBumpLuminanceBitMask; */
                },
                {
                    0,
                        /* DWORD dwRGBAlphaBitMask; */
                        /* DWORD dwYUVAlphaBitMask; */
                        /* DWORD dwLuminanceAlphaBitMask; */
                        /* DWORD dwRGBZBitMask; */
                        /* DWORD dwYUVZBitMask; */
                },
            }, /* DDPIXELFORMAT ddpfPixelFormat; */
            {
                DDSCAPS_BACKBUFFER
                | DDSCAPS_COMPLEX
                | DDSCAPS_FLIP
                | DDSCAPS_FRONTBUFFER
                | DDSCAPS_LOCALVIDMEM
                | DDSCAPS_PRIMARYSURFACE
                | DDSCAPS_VIDEOMEMORY
                | DDSCAPS_VISIBLE   /* DWORD dwCaps; */
            } /* DDSCAPS ddsCaps; */
        },
        {
            sizeof (DDSURFACEDESC), /*    DWORD   dwSize;  */
            DDSD_CAPS | DDSD_PIXELFORMAT,    /* DWORD dwFlags;    */
            0,    /* DWORD dwHeight;   */
            0,    /* DWORD dwWidth;    */
            {
                0, /* Union             */
                   /*   LONG lPitch; */
                   /*   DWORD dwLinearSize; */
            },
            0,  /*    DWORD dwBackBufferCount; */
            {
                0, /* Union */
                   /*  DWORD dwMipMapCount; */
                   /*    DWORD dwZBufferBitDepth; */
                   /*   DWORD dwRefreshRate; */
            },
            0, /*    DWORD dwAlphaBitDepth; */
            0, /*   DWORD dwReserved; */
            NULL, /*   LPVOID lpSurface; */
            {
                0, /* DWORD dwColorSpaceLowValue; */
                0, /* DWORD dwColorSpaceHighValue; */
            }, /* DDCOLORKEY  ddckCKDestOverlay; */
            {
                0, /* DWORD dwColorSpaceLowValue; */
                0, /* DWORD dwColorSpaceHighValue; */
            }, /* DDCOLORKEY  ddckCKDestBlt; */
            {
                0, /* DWORD dwColorSpaceLowValue; */
                0, /* DWORD dwColorSpaceHighValue; */
            }, /* DDCOLORKEY  ddckCKSrcOverlay; */
            {
                0, /* DWORD dwColorSpaceLowValue; */
                0, /* DWORD dwColorSpaceHighValue; */
            }, /* DDCOLORKEY ddckCKSrcBlt; */
            {
                sizeof (DDPIXELFORMAT), /* DWORD dwSize; */
                DDPF_RGB, /* DWORD dwFlags; */
                0, /* DWORD dwFourCC; */
                {
                    24, /* union */
                       /* DWORD dwRGBBitCount; */
                       /* DWORD dwYUVBitCount; */
                       /* DWORD dwZBufferBitDepth; */
                       /* DWORD dwAlphaBitDepth; */
                       /* DWORD dwLuminanceBitCount; */
                       /* DWORD dwBumpBitCount; */
                },
                {
                    0xff0000, /* union */
                       /* DWORD dwRBitMask; */
                       /* DWORD dwYBitMask; */
                        /* DWORD dwStencilBitDepth; */
                        /* DWORD dwLuminanceBitMask; */
                        /* DWORD dwBumpDuBitMask; */
                },
                {
                    0xff00,
                        /* DWORD dwGBitMask; */
                        /* DWORD dwUBitMask; */
                        /* DWORD dwZBitMask; */
                        /* DWORD dwBumpDvBitMask; */
                },
                {
                    0xff,
                        /* DWORD dwBBitMask; */
                        /* DWORD dwVBitMask; */
                        /* DWORD dwStencilBitMask; */
                        /* DWORD dwBumpLuminanceBitMask; */
                },
                {
                    0,
                        /* DWORD dwRGBAlphaBitMask; */
                        /* DWORD dwYUVAlphaBitMask; */
                        /* DWORD dwLuminanceAlphaBitMask; */
                        /* DWORD dwRGBZBitMask; */
                        /* DWORD dwYUVZBitMask; */
                },
            }, /* DDPIXELFORMAT ddpfPixelFormat; */
            {
                DDSCAPS_BACKBUFFER
                | DDSCAPS_COMPLEX
                | DDSCAPS_FLIP
                | DDSCAPS_FRONTBUFFER
                | DDSCAPS_LOCALVIDMEM
                | DDSCAPS_PRIMARYSURFACE
                | DDSCAPS_VIDEOMEMORY
                | DDSCAPS_VISIBLE  /* DWORD dwCaps; */
            } /* DDSCAPS ddsCaps; */
        },
        {
            sizeof (DDSURFACEDESC), /*    DWORD   dwSize;  */
            DDSD_CAPS | DDSD_PIXELFORMAT,    /* DWORD dwFlags;    */
            0,    /* DWORD dwHeight;   */
            0,    /* DWORD dwWidth;    */
            {
                0, /* Union             */
                   /*   LONG lPitch; */
                   /*   DWORD dwLinearSize; */
            },
            0,  /*    DWORD dwBackBufferCount; */
            {
                0, /* Union */
                   /*  DWORD dwMipMapCount; */
                   /*    DWORD dwZBufferBitDepth; */
                   /*   DWORD dwRefreshRate; */
            },
            0, /*    DWORD dwAlphaBitDepth; */
            0, /*   DWORD dwReserved; */
            NULL, /*   LPVOID lpSurface; */
            {
                0, /* DWORD dwColorSpaceLowValue; */
                0, /* DWORD dwColorSpaceHighValue; */
            }, /* DDCOLORKEY  ddckCKDestOverlay; */
            {
                0, /* DWORD dwColorSpaceLowValue; */
                0, /* DWORD dwColorSpaceHighValue; */
            }, /* DDCOLORKEY  ddckCKDestBlt; */
            {
                0, /* DWORD dwColorSpaceLowValue; */
                0, /* DWORD dwColorSpaceHighValue; */
            }, /* DDCOLORKEY  ddckCKSrcOverlay; */
            {
                0, /* DWORD dwColorSpaceLowValue; */
                0, /* DWORD dwColorSpaceHighValue; */
            }, /* DDCOLORKEY ddckCKSrcBlt; */
            {
                sizeof (DDPIXELFORMAT), /* DWORD dwSize; */
                DDPF_RGB, /* DWORD dwFlags; */
                0, /* DWORD dwFourCC; */
                {
                    16, /* union */
                       /* DWORD dwRGBBitCount; */
                       /* DWORD dwYUVBitCount; */
                       /* DWORD dwZBufferBitDepth; */
                       /* DWORD dwAlphaBitDepth; */
                       /* DWORD dwLuminanceBitCount; */
                       /* DWORD dwBumpBitCount; */
                },
                {
                    0xf800, /* union */
                       /* DWORD dwRBitMask; */
                       /* DWORD dwYBitMask; */
                        /* DWORD dwStencilBitDepth; */
                        /* DWORD dwLuminanceBitMask; */
                        /* DWORD dwBumpDuBitMask; */
                },
                {
                    0x7e0,
                        /* DWORD dwGBitMask; */
                        /* DWORD dwUBitMask; */
                        /* DWORD dwZBitMask; */
                        /* DWORD dwBumpDvBitMask; */
                },
                {
                    0x1f,
                        /* DWORD dwBBitMask; */
                        /* DWORD dwVBitMask; */
                        /* DWORD dwStencilBitMask; */
                        /* DWORD dwBumpLuminanceBitMask; */
                },
                {
                    0,
                        /* DWORD dwRGBAlphaBitMask; */
                        /* DWORD dwYUVAlphaBitMask; */
                        /* DWORD dwLuminanceAlphaBitMask; */
                        /* DWORD dwRGBZBitMask; */
                        /* DWORD dwYUVZBitMask; */
                },
            }, /* DDPIXELFORMAT ddpfPixelFormat; */
            {
                DDSCAPS_BACKBUFFER
                | DDSCAPS_COMPLEX
                | DDSCAPS_FLIP
                | DDSCAPS_FRONTBUFFER
                | DDSCAPS_LOCALVIDMEM
                | DDSCAPS_PRIMARYSURFACE
                | DDSCAPS_VIDEOMEMORY
                | DDSCAPS_VISIBLE /* DWORD dwCaps; */
            } /* DDSCAPS ddsCaps; */
        },
};

#ifdef VBOX_WITH_VIDEOHWACCEL

static void vboxVhwaPopulateOverlayFourccSurfDesc(DDSURFACEDESC *pDesc, uint32_t fourcc)
{
    memset(pDesc, 0, sizeof (DDSURFACEDESC));

    pDesc->dwSize = sizeof (DDSURFACEDESC);
    pDesc->dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT;
    pDesc->ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
    pDesc->ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    pDesc->ddpfPixelFormat.dwFourCC = fourcc;
    pDesc->ddsCaps.dwCaps = DDSCAPS_BACKBUFFER
            | DDSCAPS_COMPLEX
            | DDSCAPS_FLIP
            | DDSCAPS_FRONTBUFFER
            | DDSCAPS_LOCALVIDMEM
            | DDSCAPS_OVERLAY
            | DDSCAPS_VIDEOMEMORY
            | DDSCAPS_VISIBLE;
}

static bool vboxPixFormatMatch(DDPIXELFORMAT *pFormat1, DDPIXELFORMAT *pFormat2)
{
    return !memcmp(pFormat1, pFormat2, sizeof (DDPIXELFORMAT));
}

HRESULT vboxSurfDescMerge(DDSURFACEDESC *paDescs, uint32_t *pcDescs, uint32_t cMaxDescs, DDSURFACEDESC *pDesc)
{
    uint32_t cDescs = *pcDescs;

    Assert(cMaxDescs >= cDescs);
    Assert(pDesc->dwFlags == (DDSD_CAPS | DDSD_PIXELFORMAT));
    if (pDesc->dwFlags != (DDSD_CAPS | DDSD_PIXELFORMAT))
        return E_INVALIDARG;

    for (uint32_t i = 0; i < cDescs; ++i)
    {
        DDSURFACEDESC *pCur = &paDescs[i];
        if (vboxPixFormatMatch(&pCur->ddpfPixelFormat, &pDesc->ddpfPixelFormat))
        {
            if (pDesc->dwFlags & DDSD_CAPS)
            {
                pCur->dwFlags |= DDSD_CAPS;
                pCur->ddsCaps.dwCaps |= pDesc->ddsCaps.dwCaps;
            }
            return S_OK;
        }
    }

    if (cMaxDescs > cDescs)
    {
        paDescs[cDescs] = *pDesc;
        ++cDescs;
        *pcDescs = cDescs;
        return VINF_SUCCESS;
    }
    return E_FAIL;
}

HRESULT vboxFormatOpsMerge(FORMATOP *paOps, uint32_t *pcOps, uint32_t cMaxOps, FORMATOP *pOp)
{
    uint32_t cOps = *pcOps;

    Assert(cMaxOps >= cOps);

    for (uint32_t i = 0; i < cOps; ++i)
    {
        FORMATOP *pCur = &paOps[i];
        if (pCur->Format == pOp->Format)
        {
            pCur->Operations |= pOp->Operations;
            Assert(pCur->FlipMsTypes == pOp->FlipMsTypes);
            Assert(pCur->BltMsTypes == pOp->BltMsTypes);
            Assert(pCur->PrivateFormatBitCount == pOp->PrivateFormatBitCount);
            return S_OK;
        }
    }

    if (cMaxOps > cOps)
    {
        paOps[cOps] = *pOp;
        ++cOps;
        *pcOps = cOps;
        return VINF_SUCCESS;
    }
    return E_FAIL;
}

HRESULT VBoxDispD3DGlobal2DFormatsInit(PVBOXWDDMDISP_ADAPTER pAdapter)
{
    HRESULT hr = S_OK;
    memset(&pAdapter->D3D, 0, sizeof (pAdapter->D3D));
    memset(&pAdapter->Formats, 0, sizeof (pAdapter->Formats));

    /* just calc the max number of formats */
    uint32_t cFormats = RT_ELEMENTS(gVBoxFormatOpsBase);
    uint32_t cSurfDescs = RT_ELEMENTS(gVBoxSurfDescsBase);
    uint32_t cOverlayFormats = 0;
    for (uint32_t i = 0; i < pAdapter->cHeads; ++i)
    {
        VBOXDISPVHWA_INFO *pVhwa = &pAdapter->aHeads[i].Vhwa;
        if (pVhwa->Settings.fFlags & VBOXVHWA_F_ENABLED)
        {
            cOverlayFormats += pVhwa->Settings.cFormats;
        }
    }

    cFormats += cOverlayFormats;
    cSurfDescs += cOverlayFormats;

    uint32_t cbFormatOps = cFormats * sizeof (FORMATOP);
    cbFormatOps = (cbFormatOps + 7) & ~3;
    /* ensure the surf descs are 8 byte aligned */
    uint32_t offSurfDescs = (cbFormatOps + 7) & ~3;
    uint32_t cbSurfDescs = cSurfDescs * sizeof (DDSURFACEDESC);
    uint32_t cbBuf = offSurfDescs + cbSurfDescs;
    uint8_t* pvBuf = (uint8_t*)RTMemAllocZ(cbBuf);
    if (pvBuf)
    {
        pAdapter->Formats.paFormstOps = (FORMATOP*)pvBuf;
        memcpy ((void*)pAdapter->Formats.paFormstOps , gVBoxFormatOpsBase, sizeof (gVBoxFormatOpsBase));
        pAdapter->Formats.cFormstOps = RT_ELEMENTS(gVBoxFormatOpsBase);

        FORMATOP fo = {D3DDDIFMT_UNKNOWN, 0, 0, 0, 0};
        for (uint32_t i = 0; i < pAdapter->cHeads; ++i)
        {
            VBOXDISPVHWA_INFO *pVhwa = &pAdapter->aHeads[i].Vhwa;
            if (pVhwa->Settings.fFlags & VBOXVHWA_F_ENABLED)
            {
                for (uint32_t j = 0; j < pVhwa->Settings.cFormats; ++j)
                {
                    fo.Format = pVhwa->Settings.aFormats[j];
                    fo.Operations = FORMATOP_OVERLAY;
                    hr = vboxFormatOpsMerge((FORMATOP *)pAdapter->Formats.paFormstOps, &pAdapter->Formats.cFormstOps, cFormats, &fo);
                    if (FAILED(hr))
                    {
                        WARN(("vboxFormatOpsMerge failed, hr 0x%x", hr));
                    }
                }
            }
        }

        pAdapter->Formats.paSurfDescs = (DDSURFACEDESC*)(pvBuf + offSurfDescs);
        memcpy ((void*)pAdapter->Formats.paSurfDescs , gVBoxSurfDescsBase, sizeof (gVBoxSurfDescsBase));
        pAdapter->Formats.cSurfDescs = RT_ELEMENTS(gVBoxSurfDescsBase);

        DDSURFACEDESC sd;
        for (uint32_t i = 0; i < pAdapter->cHeads; ++i)
        {
            VBOXDISPVHWA_INFO *pVhwa = &pAdapter->aHeads[i].Vhwa;
            if (pVhwa->Settings.fFlags & VBOXVHWA_F_ENABLED)
            {
                for (uint32_t j = 0; j < pVhwa->Settings.cFormats; ++j)
                {
                    uint32_t fourcc = vboxWddmFormatToFourcc(pVhwa->Settings.aFormats[j]);
                    if (fourcc)
                    {
                        vboxVhwaPopulateOverlayFourccSurfDesc(&sd, fourcc);
                        hr = vboxSurfDescMerge((DDSURFACEDESC *)pAdapter->Formats.paSurfDescs, &pAdapter->Formats.cSurfDescs, cSurfDescs, &sd);
                        if (FAILED(hr))
                        {
                            WARN(("vboxFormatOpsMerge failed, hr 0x%x", hr));
                        }
                    }
                }
            }
        }
    }
    else
    {
        WARN(("RTMemAllocZ failed"));
        return E_FAIL;
    }
    return S_OK;
}

void VBoxDispD3DGlobal2DFormatsTerm(PVBOXWDDMDISP_ADAPTER pAdapter)
{
    if (pAdapter->Formats.paFormstOps)
        RTMemFree((void *)pAdapter->Formats.paFormstOps);
}

#endif

static CRITICAL_SECTION g_VBoxDispD3DGlobalCritSect;
static VBOXWDDMDISP_D3D g_VBoxDispD3DGlobalD3D;
static VBOXWDDMDISP_FORMATS g_VBoxDispD3DGlobalD3DFormats;
static uint32_t g_cVBoxDispD3DGlobalOpens;

void vboxDispD3DGlobalLock()
{
    EnterCriticalSection(&g_VBoxDispD3DGlobalCritSect);
}

void vboxDispD3DGlobalUnlock()
{
    LeaveCriticalSection(&g_VBoxDispD3DGlobalCritSect);
}

void VBoxDispD3DGlobalInit()
{
    g_cVBoxDispD3DGlobalOpens = 0;
    InitializeCriticalSection(&g_VBoxDispD3DGlobalCritSect);
}

void VBoxDispD3DGlobalTerm()
{
    DeleteCriticalSection(&g_VBoxDispD3DGlobalCritSect);
}

static void vboxDispD3DGlobalD3DFormatsInit(PVBOXWDDMDISP_FORMATS pFormats)
{
    memset(pFormats, 0, sizeof (*pFormats));
    pFormats->paFormstOps = gVBoxFormatOps3D;
    pFormats->cFormstOps = RT_ELEMENTS(gVBoxFormatOps3D);
}

#ifndef D3DCAPS2_CANRENDERWINDOWED
#define D3DCAPS2_CANRENDERWINDOWED UINT32_C(0x00080000)
#endif

#ifdef DEBUG
/*
 * Check capabilities reported by wine and log any which are not good enough for a D3D feature level.
 */

#define VBOX_D3D_CHECK_FLAGS(level, field, flags) do { \
        if (((field) & (flags)) != (flags)) \
        { \
            LogRel(("D3D level %s %s flags: 0x%08X -> 0x%08X\n", #level, #field, (field), (flags))); \
        } \
    } while (0)

#define VBOX_D3D_CHECK_VALUE(level, field, value) do { \
        if ((int64_t)(value) >= 0? (field) < (value): (field) > (value)) \
        { \
            LogRel(("D3D level %s %s value: %lld -> %lld\n", #level, #field, (int64_t)(field), (int64_t)(value))); \
        } \
    } while (0)

#define VBOX_D3D_CHECK_VALUE_HEX(level, field, value) do { \
        if ((field) < (value)) \
        { \
            LogRel(("D3D level %s %s value: 0x%08X -> 0x%08X\n", #level, #field, (field), (value))); \
        } \
    } while (0)

static void vboxDispCheckCapsLevel(const D3DCAPS9 *pCaps)
{
    /* Misc. */
    VBOX_D3D_CHECK_FLAGS(misc, pCaps->Caps,                     D3DCAPS_READ_SCANLINE);
    VBOX_D3D_CHECK_FLAGS(misc, pCaps->Caps2,                    D3DCAPS2_CANRENDERWINDOWED | D3DCAPS2_CANSHARERESOURCE);
    VBOX_D3D_CHECK_FLAGS(misc, pCaps->DevCaps,                  D3DDEVCAPS_FLOATTLVERTEX
                                                            /*| D3DDEVCAPS_HWVERTEXBUFFER | D3DDEVCAPS_HWINDEXBUFFER |  D3DDEVCAPS_SUBVOLUMELOCK */);
    VBOX_D3D_CHECK_FLAGS(misc, pCaps->PrimitiveMiscCaps,        D3DPMISCCAPS_INDEPENDENTWRITEMASKS /** @todo needs GL_EXT_draw_buffers2 */
                                                              | D3DPMISCCAPS_FOGINFVF
                                                              | D3DPMISCCAPS_SEPARATEALPHABLEND
                                                              | D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS);
    VBOX_D3D_CHECK_FLAGS(misc, pCaps->RasterCaps,               D3DPRASTERCAPS_SUBPIXEL
                                                              | D3DPRASTERCAPS_STIPPLE
                                                              | D3DPRASTERCAPS_ZBIAS
                                                              | D3DPRASTERCAPS_COLORPERSPECTIVE);
    VBOX_D3D_CHECK_FLAGS(misc, pCaps->TextureCaps,              D3DPTEXTURECAPS_TRANSPARENCY
                                                              | D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE);
    VBOX_D3D_CHECK_FLAGS(misc, pCaps->TextureAddressCaps,       D3DPTADDRESSCAPS_MIRRORONCE); /** @todo needs GL_ARB_texture_mirror_clamp_to_edge */
    VBOX_D3D_CHECK_FLAGS(misc, pCaps->VolumeTextureAddressCaps, D3DPTADDRESSCAPS_MIRRORONCE); /** @todo needs GL_ARB_texture_mirror_clamp_to_edge */
    VBOX_D3D_CHECK_FLAGS(misc, pCaps->StencilCaps,              D3DSTENCILCAPS_TWOSIDED);
    VBOX_D3D_CHECK_FLAGS(misc, pCaps->DeclTypes,                D3DDTCAPS_FLOAT16_2 | D3DDTCAPS_FLOAT16_4); /** @todo both need GL_ARB_half_float_vertex */
    VBOX_D3D_CHECK_FLAGS(misc, pCaps->VertexTextureFilterCaps,  D3DPTFILTERCAPS_MINFPOINT
                                                              | D3DPTFILTERCAPS_MAGFPOINT);
    VBOX_D3D_CHECK_VALUE(misc, pCaps->GuardBandLeft,  -8192.);
    VBOX_D3D_CHECK_VALUE(misc, pCaps->GuardBandTop,   -8192.);
    VBOX_D3D_CHECK_VALUE(misc, pCaps->GuardBandRight,  8192.);
    VBOX_D3D_CHECK_VALUE(misc, pCaps->GuardBandBottom, 8192.);
    VBOX_D3D_CHECK_VALUE(misc, pCaps->VS20Caps.DynamicFlowControlDepth, 24);
    VBOX_D3D_CHECK_VALUE(misc, pCaps->VS20Caps.NumTemps, D3DVS20_MAX_NUMTEMPS);
    VBOX_D3D_CHECK_VALUE(misc, pCaps->PS20Caps.DynamicFlowControlDepth, 24);
    VBOX_D3D_CHECK_VALUE(misc, pCaps->PS20Caps.NumTemps, D3DVS20_MAX_NUMTEMPS);

    /* 9_1 */
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->Caps2,                 D3DCAPS2_DYNAMICTEXTURES | D3DCAPS2_FULLSCREENGAMMA);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->PresentationIntervals, D3DPRESENT_INTERVAL_IMMEDIATE | D3DPRESENT_INTERVAL_ONE);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->PrimitiveMiscCaps,     D3DPMISCCAPS_COLORWRITEENABLE);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->ShadeCaps,             D3DPSHADECAPS_ALPHAGOURAUDBLEND | D3DPSHADECAPS_COLORGOURAUDRGB
                                                          | D3DPSHADECAPS_FOGGOURAUD | D3DPSHADECAPS_SPECULARGOURAUDRGB);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->TextureFilterCaps,     D3DPTFILTERCAPS_MINFLINEAR | D3DPTFILTERCAPS_MINFPOINT
                                                          | D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MAGFPOINT);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->TextureCaps,           D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_CUBEMAP
                                                          | D3DPTEXTURECAPS_MIPMAP | D3DPTEXTURECAPS_PERSPECTIVE);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->TextureAddressCaps,    D3DPTADDRESSCAPS_CLAMP | D3DPTADDRESSCAPS_INDEPENDENTUV
                                                          | D3DPTADDRESSCAPS_MIRROR | D3DPTADDRESSCAPS_WRAP);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->TextureOpCaps,         D3DTEXOPCAPS_DISABLE | D3DTEXOPCAPS_MODULATE
                                                          | D3DTEXOPCAPS_SELECTARG1 | D3DTEXOPCAPS_SELECTARG2);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->SrcBlendCaps,          D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_INVDESTCOLOR
                                                          | D3DPBLENDCAPS_INVSRCALPHA | D3DPBLENDCAPS_ONE
                                                          | D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_ZERO);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->DestBlendCaps,         D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_INVSRCALPHA
                                                          | D3DPBLENDCAPS_INVSRCCOLOR | D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_ZERO);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->StretchRectFilterCaps, D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MAGFPOINT
                                                          | D3DPTFILTERCAPS_MINFLINEAR | D3DPTFILTERCAPS_MINFPOINT);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->ZCmpCaps,              D3DPCMPCAPS_ALWAYS | D3DPCMPCAPS_LESSEQUAL);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->RasterCaps,            D3DPRASTERCAPS_DEPTHBIAS | D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS);
    VBOX_D3D_CHECK_FLAGS(9.1, pCaps->StencilCaps,           D3DSTENCILCAPS_TWOSIDED);

    VBOX_D3D_CHECK_VALUE(9.1, pCaps->MaxTextureWidth,         2048);
    VBOX_D3D_CHECK_VALUE(9.1, pCaps->MaxTextureHeight,        2048);
    VBOX_D3D_CHECK_VALUE(9.1, pCaps->NumSimultaneousRTs,      1);
    VBOX_D3D_CHECK_VALUE(9.1, pCaps->MaxSimultaneousTextures, 8);
    VBOX_D3D_CHECK_VALUE(9.1, pCaps->MaxTextureBlendStages,   8);
    VBOX_D3D_CHECK_VALUE_HEX(9.1, pCaps->PixelShaderVersion,  D3DPS_VERSION(2,0));
    VBOX_D3D_CHECK_VALUE(9.1, pCaps->MaxPrimitiveCount,       65535);
    VBOX_D3D_CHECK_VALUE(9.1, pCaps->MaxVertexIndex,          65534);
    VBOX_D3D_CHECK_VALUE(9.1, pCaps->MaxVolumeExtent,         256);
    VBOX_D3D_CHECK_VALUE(9.1, pCaps->MaxTextureRepeat,        128); /* Must be zero, or 128, or greater. */
    VBOX_D3D_CHECK_VALUE(9.1, pCaps->MaxAnisotropy,           2);
    VBOX_D3D_CHECK_VALUE(9.1, pCaps->MaxVertexW,              0.f);

    /* 9_2 */
    VBOX_D3D_CHECK_FLAGS(9.2, pCaps->PrimitiveMiscCaps,     D3DPMISCCAPS_SEPARATEALPHABLEND);
    VBOX_D3D_CHECK_FLAGS(9.2, pCaps->DevCaps2,              D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET);
    VBOX_D3D_CHECK_FLAGS(9.2, pCaps->TextureAddressCaps,    D3DPTADDRESSCAPS_MIRRORONCE);
    VBOX_D3D_CHECK_FLAGS(9.2, pCaps->VolumeTextureAddressCaps, D3DPTADDRESSCAPS_MIRRORONCE);
    VBOX_D3D_CHECK_VALUE(9.2, pCaps->MaxTextureWidth,         2048);
    VBOX_D3D_CHECK_VALUE(9.2, pCaps->MaxTextureHeight,        2048);
    VBOX_D3D_CHECK_VALUE(9.2, pCaps->MaxTextureRepeat,        2048); /* Must be zero, or 2048, or greater. */
    VBOX_D3D_CHECK_VALUE_HEX(9.2, pCaps->VertexShaderVersion, D3DVS_VERSION(2,0));
    VBOX_D3D_CHECK_VALUE(9.2, pCaps->MaxAnisotropy,           16);
    VBOX_D3D_CHECK_VALUE(9.2, pCaps->MaxPrimitiveCount,       1048575);
    VBOX_D3D_CHECK_VALUE(9.2, pCaps->MaxVertexIndex,          1048575);
    VBOX_D3D_CHECK_VALUE(9.2, pCaps->MaxVertexW,              10000000000.f);

    /* 9_3 */
    VBOX_D3D_CHECK_FLAGS(9.3, pCaps->PS20Caps.Caps,         D3DPS20CAPS_GRADIENTINSTRUCTIONS);
    VBOX_D3D_CHECK_FLAGS(9.3, pCaps->VS20Caps.Caps,         D3DVS20CAPS_PREDICATION);
    VBOX_D3D_CHECK_FLAGS(9.3, pCaps->PrimitiveMiscCaps,     D3DPMISCCAPS_INDEPENDENTWRITEMASKS | D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING);
    VBOX_D3D_CHECK_FLAGS(9.3, pCaps->TextureAddressCaps,    D3DPTADDRESSCAPS_BORDER);
    VBOX_D3D_CHECK_VALUE(9.3, pCaps->MaxTextureWidth,         4096);
    VBOX_D3D_CHECK_VALUE(9.3, pCaps->MaxTextureHeight,        4096);
    VBOX_D3D_CHECK_VALUE(9.3, pCaps->MaxTextureRepeat,        8192); /* Must be zero, or 8192, or greater. */
    VBOX_D3D_CHECK_VALUE(9.3, pCaps->NumSimultaneousRTs,      4);
    VBOX_D3D_CHECK_VALUE(9.3, pCaps->PS20Caps.NumInstructionSlots, 512); /* (Pixel Shader Version 2b) */
    VBOX_D3D_CHECK_VALUE(9.3, pCaps->PS20Caps.NumTemps,       32); /* (Pixel Shader Version 2b) */
    VBOX_D3D_CHECK_VALUE(9.3, pCaps->VS20Caps.NumTemps,       32); /* (Vertex Shader Version 2a) */
    VBOX_D3D_CHECK_VALUE(9.3, pCaps->VS20Caps.StaticFlowControlDepth, 4);
    VBOX_D3D_CHECK_VALUE(9.3, pCaps->MaxVertexShaderConst,    256); /* (Vertex Shader Version 2a); */
    VBOX_D3D_CHECK_VALUE(9.3, pCaps->MaxVertexShader30InstructionSlots, 512);
    VBOX_D3D_CHECK_VALUE_HEX(9.3, pCaps->VertexShaderVersion, D3DVS_VERSION(3,0));

    LogRel(("Capabilities check completed\n"));
}

#undef VBOX_D3D_CHECK_FLAGS
#undef VBOX_D3D_CHECK_VALUE
#undef VBOX_D3D_CHECK_VALUE_HEX

#endif /* DEBUG */

static HRESULT vboxWddmGetD3D9Caps(PVBOXWDDMDISP_D3D pD3D, D3DCAPS9 *pCaps)
{
    HRESULT hr = pD3D->pD3D9If->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, pCaps);
    if (FAILED(hr))
    {
        WARN(("GetDeviceCaps failed hr(0x%x)",hr));
        return hr;
    }

#ifdef DEBUG
    vboxDispCheckCapsLevel(pCaps);
#endif

    /* needed for Windows Media Player to work properly */
    pCaps->Caps |= D3DCAPS_READ_SCANLINE;
    pCaps->Caps2 |= 0x00080000 /*D3DCAPS2_CANRENDERWINDOWED*/;
    pCaps->Caps2 |= D3DCAPS2_CANSHARERESOURCE;
    pCaps->DevCaps |= D3DDEVCAPS_FLOATTLVERTEX /* <- must be set according to the docs */
            /*| D3DDEVCAPS_HWVERTEXBUFFER | D3DDEVCAPS_HWINDEXBUFFER |  D3DDEVCAPS_SUBVOLUMELOCK */;
    pCaps->PrimitiveMiscCaps |= D3DPMISCCAPS_INDEPENDENTWRITEMASKS
            | D3DPMISCCAPS_FOGINFVF
            | D3DPMISCCAPS_SEPARATEALPHABLEND | D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS;
    pCaps->RasterCaps |= D3DPRASTERCAPS_SUBPIXEL | D3DPRASTERCAPS_STIPPLE | D3DPRASTERCAPS_ZBIAS | D3DPRASTERCAPS_COLORPERSPECTIVE /* keep */;
    pCaps->TextureCaps |= D3DPTEXTURECAPS_TRANSPARENCY | D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE;
    pCaps->TextureAddressCaps |= D3DPTADDRESSCAPS_MIRRORONCE;
    pCaps->VolumeTextureAddressCaps |= D3DPTADDRESSCAPS_MIRRORONCE;
    pCaps->StencilCaps |= D3DSTENCILCAPS_TWOSIDED;
    pCaps->DeclTypes |= D3DDTCAPS_FLOAT16_2 | D3DDTCAPS_FLOAT16_4;
    pCaps->VertexTextureFilterCaps |= D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MAGFPOINT;
    pCaps->GuardBandLeft = -8192.;
    pCaps->GuardBandTop = -8192.;
    pCaps->GuardBandRight = 8192.;
    pCaps->GuardBandBottom = 8192.;
    pCaps->VS20Caps.DynamicFlowControlDepth = 24;
    pCaps->VS20Caps.NumTemps = D3DVS20_MAX_NUMTEMPS;
    pCaps->PS20Caps.DynamicFlowControlDepth = 24;
    pCaps->PS20Caps.NumTemps = D3DVS20_MAX_NUMTEMPS;

    /* workaround for wine not returning InstructionSlots correctly for  shaders v3.0 */
    if ((pCaps->VertexShaderVersion & 0xff00) == 0x0300)
    {
        pCaps->MaxVertexShader30InstructionSlots = RT_MIN(32768, pCaps->MaxVertexShader30InstructionSlots);
        pCaps->MaxPixelShader30InstructionSlots = RT_MIN(32768, pCaps->MaxPixelShader30InstructionSlots);
    }
#if defined(DEBUG)
    if ((pCaps->VertexShaderVersion & 0xff00) == 0x0300)
    {
        Assert(pCaps->MaxVertexShader30InstructionSlots >= 512);
        Assert(pCaps->MaxVertexShader30InstructionSlots <= 32768);
        Assert(pCaps->MaxPixelShader30InstructionSlots >= 512);
        Assert(pCaps->MaxPixelShader30InstructionSlots <= 32768);
    }
    else if ((pCaps->VertexShaderVersion & 0xff00) == 0x0200)
    {
        Assert(pCaps->MaxVertexShader30InstructionSlots == 0);
        Assert(pCaps->MaxPixelShader30InstructionSlots == 0);
    }
    else
    {
        WARN(("incorect shader caps!"));
    }
#endif

    pCaps->MaxVertexW = 10000000000.f; /* Required by D3D feature level 9.3. */

#ifdef DEBUG
    vboxDispCheckCapsLevel(pCaps);
#endif

    vboxDispDumpD3DCAPS9(pCaps);

    return S_OK;
}

static void vboxDispD3DGlobalDoClose(PVBOXWDDMDISP_D3D pD3D)
{
    pD3D->pD3D9If->Release();
    VBoxDispD3DClose(&pD3D->D3D);
}

static HRESULT vboxDispD3DGlobalDoOpen(PVBOXWDDMDISP_D3D pD3D)
{
    memset(pD3D, 0, sizeof (*pD3D));
    HRESULT hr = VBoxDispD3DOpen(&pD3D->D3D);
    if (SUCCEEDED(hr))
    {
        hr = pD3D->D3D.pfnDirect3DCreate9Ex(D3D_SDK_VERSION, &pD3D->pD3D9If);
        if (SUCCEEDED(hr))
        {
            hr = vboxWddmGetD3D9Caps(pD3D, &pD3D->Caps);
            if (SUCCEEDED(hr))
            {
                pD3D->cMaxSimRTs = pD3D->Caps.NumSimultaneousRTs;
                Assert(pD3D->cMaxSimRTs);
                Assert(pD3D->cMaxSimRTs < UINT32_MAX/2);
                LOG(("SUCCESS 3D Enabled, pD3D (0x%p)", pD3D));
                return S_OK;
            }
            else
            {
                WARN(("vboxWddmGetD3D9Caps failed hr = 0x%x", hr));
            }
            pD3D->pD3D9If->Release();
        }
        else
        {
            WARN(("pfnDirect3DCreate9Ex failed hr = 0x%x", hr));
        }
        VBoxDispD3DClose(&pD3D->D3D);
    }
    else
    {
        WARN(("VBoxDispD3DOpen failed hr = 0x%x", hr));
    }
    return hr;
}

HRESULT VBoxDispD3DGlobalOpen(PVBOXWDDMDISP_D3D pD3D, PVBOXWDDMDISP_FORMATS pFormats)
{
    vboxDispD3DGlobalLock();
    if (!g_cVBoxDispD3DGlobalOpens)
    {
        HRESULT hr = vboxDispD3DGlobalDoOpen(&g_VBoxDispD3DGlobalD3D);
        if (!SUCCEEDED(hr))
        {
            vboxDispD3DGlobalUnlock();
            WARN(("vboxDispD3DGlobalDoOpen failed hr = 0x%x", hr));
            return hr;
        }

        vboxDispD3DGlobalD3DFormatsInit(&g_VBoxDispD3DGlobalD3DFormats);
    }
    ++g_cVBoxDispD3DGlobalOpens;
    vboxDispD3DGlobalUnlock();

    *pD3D = g_VBoxDispD3DGlobalD3D;
    *pFormats = g_VBoxDispD3DGlobalD3DFormats;
    return S_OK;
}

void VBoxDispD3DGlobalClose(PVBOXWDDMDISP_D3D pD3D, PVBOXWDDMDISP_FORMATS pFormats)
{
    RT_NOREF(pD3D, pFormats);
    vboxDispD3DGlobalLock();
    --g_cVBoxDispD3DGlobalOpens;
    if (!g_cVBoxDispD3DGlobalOpens)
        vboxDispD3DGlobalDoClose(&g_VBoxDispD3DGlobalD3D);
    vboxDispD3DGlobalUnlock();
}
