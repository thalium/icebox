/* $Id: DevVGA-SVGA3d-win.cpp $ */
/** @file
 * DevVMWare - VMWare SVGA device
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
#include <VBox/version.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/vmm/pgm.h>

#include <iprt/assert.h>
#include <iprt/semaphore.h>
#include <iprt/uuid.h>
#include <iprt/mem.h>
#include <iprt/avl.h>

#include <VBoxVideo.h> /* required by DevVGA.h */

/* should go BEFORE any other DevVGA include to make all DevVGA.h config defines be visible */
#include "DevVGA.h"

#include "DevVGA-SVGA.h"
#include "DevVGA-SVGA3d.h"
#include "DevVGA-SVGA3d-internal.h"

/* Enable to disassemble defined shaders. */
#if defined(DEBUG) && 0 /* Disabled as we don't have the DirectX SDK avaible atm. */
#define DUMP_SHADER_DISASSEMBLY
#endif

#ifdef DUMP_SHADER_DISASSEMBLY
#include <d3dx9shader.h>
#endif


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/* Enable to render the result of DrawPrimitive in a seperate window. */
//#define DEBUG_GFX_WINDOW

#define FOURCC_INTZ     (D3DFORMAT)MAKEFOURCC('I', 'N', 'T', 'Z')
#define FOURCC_NULL     (D3DFORMAT)MAKEFOURCC('N', 'U', 'L', 'L')


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

typedef struct
{
    DWORD                   Usage;
    D3DRESOURCETYPE         ResourceType;
    SVGA3dFormatOp          FormatOp;
} VMSVGA3DFORMATSUPPORT;


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
static VMSVGA3DFORMATSUPPORT const g_aFormatSupport[] =
{
    {
        0,
        D3DRTYPE_SURFACE,
        SVGA3DFORMAT_OP_OFFSCREENPLAIN,
    },
    {
        D3DUSAGE_RENDERTARGET,
        D3DRTYPE_SURFACE,
        (SVGA3dFormatOp) (SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET | SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET),
    },
    {
        D3DUSAGE_AUTOGENMIPMAP,
        D3DRTYPE_TEXTURE,
        SVGA3DFORMAT_OP_AUTOGENMIPMAP,
    },
    {
        D3DUSAGE_DMAP,
        D3DRTYPE_TEXTURE,
        SVGA3DFORMAT_OP_DMAP,
    },
    {
        0,
        D3DRTYPE_TEXTURE,
        SVGA3DFORMAT_OP_TEXTURE,
    },
    {
        0,
        D3DRTYPE_CUBETEXTURE,
        SVGA3DFORMAT_OP_CUBETEXTURE,
    },
    {
        0,
        D3DRTYPE_VOLUMETEXTURE,
        SVGA3DFORMAT_OP_VOLUMETEXTURE,
    },
    {
        D3DUSAGE_QUERY_VERTEXTEXTURE,
        D3DRTYPE_TEXTURE,
        SVGA3DFORMAT_OP_VERTEXTEXTURE,
    },
    {
        D3DUSAGE_QUERY_LEGACYBUMPMAP,
        D3DRTYPE_TEXTURE,
        SVGA3DFORMAT_OP_BUMPMAP,
    },
    {
        D3DUSAGE_QUERY_SRGBREAD,
        D3DRTYPE_TEXTURE,
        SVGA3DFORMAT_OP_SRGBREAD,
    },
    {
        D3DUSAGE_QUERY_SRGBWRITE,
        D3DRTYPE_TEXTURE,
        SVGA3DFORMAT_OP_SRGBWRITE,
    }
};

static VMSVGA3DFORMATSUPPORT const  g_aFeatureReject[] =
{
    {
        D3DUSAGE_QUERY_WRAPANDMIP,
        D3DRTYPE_TEXTURE,
        SVGA3DFORMAT_OP_NOTEXCOORDWRAPNORMIP
    },
    {
        D3DUSAGE_QUERY_FILTER,
        D3DRTYPE_TEXTURE,
        SVGA3DFORMAT_OP_NOFILTER
    },
    {
        D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
        D3DRTYPE_TEXTURE, /* ?? */
        SVGA3DFORMAT_OP_NOALPHABLEND
    },
};


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void vmsvgaDumpD3DCaps(D3DCAPS9 *pCaps);


#define D3D_RELEASE(ptr) do { \
    if (ptr)                  \
    {                         \
        (ptr)->Release();     \
        (ptr) = 0;            \
    }                         \
} while (0)


int vmsvga3dInit(PVGASTATE pThis)
{
    PVMSVGA3DSTATE pState;
    int rc;

    pThis->svga.p3dState = pState = (PVMSVGA3DSTATE)RTMemAllocZ(sizeof(VMSVGA3DSTATE));
    AssertReturn(pThis->svga.p3dState, VERR_NO_MEMORY);

    /* Create event semaphore. */
    rc = RTSemEventCreate(&pState->WndRequestSem);
    if (RT_FAILURE(rc))
    {
        Log(("%s: Failed to create event semaphore for window handling.\n", __FUNCTION__));
        return rc;
    }

    /* Create the async IO thread. */
    rc = RTThreadCreate(&pState->pWindowThread, vmsvga3dWindowThread, pState->WndRequestSem, 0, RTTHREADTYPE_GUI, 0, "VMSVGA3DWND");
    if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("%s: Async IO Thread creation for 3d window handling failed rc=%d\n", __FUNCTION__, rc));
        return rc;
    }

    return VINF_SUCCESS;
}

int vmsvga3dPowerOn(PVGASTATE pThis)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pThis->svga.p3dState, VERR_NO_MEMORY);
    HRESULT hr;

    if (pState->pD3D9)
        return VINF_SUCCESS;    /* already initialized (load state) */

#ifdef VBOX_VMSVGA3D_WITH_WINE_OPENGL
    pState->pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
    AssertReturn(pState->pD3D9, VERR_INTERNAL_ERROR);
#else
    /* Direct3DCreate9Ex was introduced in Vista, so resolve it dynamically. */
    typedef HRESULT (WINAPI *PFNDIRECT3DCREATE9EX)(UINT, IDirect3D9Ex **);
    PFNDIRECT3DCREATE9EX pfnDirect3dCreate9Ex = (PFNDIRECT3DCREATE9EX)RTLdrGetSystemSymbol("d3d9.dll", "Direct3DCreate9Ex");
    if (!pfnDirect3dCreate9Ex)
        return PDMDevHlpVMSetError(pThis->CTX_SUFF(pDevIns), VERR_SYMBOL_NOT_FOUND, RT_SRC_POS,
                                   "vmsvga3d: Unable to locate Direct3DCreate9Ex. This feature requires Vista and later.");
    hr = pfnDirect3dCreate9Ex(D3D_SDK_VERSION, &pState->pD3D9);
    AssertReturn(hr == D3D_OK, VERR_INTERNAL_ERROR);
#endif
    hr = pState->pD3D9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &pState->caps);
    AssertReturn(hr == D3D_OK, VERR_INTERNAL_ERROR);

    vmsvgaDumpD3DCaps(&pState->caps);

    /* Check if INTZ is supported. */
    hr = pState->pD3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT,
                                          D3DDEVTYPE_HAL,
                                          D3DFMT_X8R8G8B8,    /* assume standard 32-bit display mode */
                                          0,
                                          D3DRTYPE_TEXTURE,
                                          FOURCC_INTZ);
    if (hr != D3D_OK)
    {
        /* INTZ support is essential to support depth surfaces used as textures. */
        LogRel(("VMSVGA: texture format INTZ not supported!!!\n"));
    }
    else
        pState->fSupportedSurfaceINTZ = true;

    /* Check if NULL is supported. */
    hr = pState->pD3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT,
                                          D3DDEVTYPE_HAL,
                                          D3DFMT_X8R8G8B8,    /* assume standard 32-bit display mode */
                                          D3DUSAGE_RENDERTARGET,
                                          D3DRTYPE_SURFACE,
                                          FOURCC_NULL);
    if (hr != D3D_OK)
    {
        /* NULL is a dummy surface which can be used as a render target to save memory. */
        LogRel(("VMSVGA: surface format NULL not supported!!!\n"));
    }
    else
        pState->fSupportedSurfaceNULL = true;


    /*  Check if DX9 depth stencil textures are supported */
    hr = pState->pD3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT,
                                          D3DDEVTYPE_HAL,
                                          D3DFMT_X8R8G8B8,    /* assume standard 32-bit display mode */
                                          D3DUSAGE_DEPTHSTENCIL,
                                          D3DRTYPE_TEXTURE,
                                          D3DFMT_D16);
    if (hr != D3D_OK)
    {
        LogRel(("VMSVGA: texture format D3DFMT_D16 not supported\n"));
    }

    hr = pState->pD3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT,
                                          D3DDEVTYPE_HAL,
                                          D3DFMT_X8R8G8B8,    /* assume standard 32-bit display mode */
                                          D3DUSAGE_DEPTHSTENCIL,
                                          D3DRTYPE_TEXTURE,
                                          D3DFMT_D24X8);
    if (hr != D3D_OK)
    {
        LogRel(("VMSVGA: texture format D3DFMT_D24X8 not supported\n"));
    }
    hr = pState->pD3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT,
                                          D3DDEVTYPE_HAL,
                                          D3DFMT_X8R8G8B8,    /* assume standard 32-bit display mode */
                                          D3DUSAGE_DEPTHSTENCIL,
                                          D3DRTYPE_TEXTURE,
                                          D3DFMT_D24S8);
    if (hr != D3D_OK)
    {
        LogRel(("VMSVGA: texture format D3DFMT_D24S8 not supported\n"));
    }
    return VINF_SUCCESS;
}

int vmsvga3dReset(PVGASTATE pThis)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pThis->svga.p3dState, VERR_NO_MEMORY);

    /* Destroy all leftover surfaces. */
    for (uint32_t i = 0; i < pState->cSurfaces; i++)
    {
        if (pState->papSurfaces[i]->id != SVGA3D_INVALID_ID)
            vmsvga3dSurfaceDestroy(pThis, pState->papSurfaces[i]->id);
    }

    /* Destroy all leftover contexts. */
    for (uint32_t i = 0; i < pState->cContexts; i++)
    {
        if (pState->papContexts[i]->id != SVGA3D_INVALID_ID)
            vmsvga3dContextDestroy(pThis, pState->papContexts[i]->id);
    }
    return VINF_SUCCESS;
}

int vmsvga3dTerminate(PVGASTATE pThis)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pThis->svga.p3dState, VERR_NO_MEMORY);

    int rc = vmsvga3dReset(pThis);
    AssertRCReturn(rc, rc);

    /* Terminate the window creation thread. */
    rc = vmsvga3dSendThreadMessage(pState->pWindowThread, pState->WndRequestSem, WM_VMSVGA3D_EXIT, 0, 0);
    AssertRCReturn(rc, rc);

    RTSemEventDestroy(pState->WndRequestSem);

    D3D_RELEASE(pState->pD3D9);

    return VINF_SUCCESS;
}

void vmsvga3dUpdateHostScreenViewport(PVGASTATE pThis, uint32_t idScreen, VMSVGAVIEWPORT const *pOldViewport)
{
    /** @todo Scroll the screen content without requiring the guest to redraw. */
    NOREF(pThis); NOREF(idScreen); NOREF(pOldViewport);
}

static uint32_t vmsvga3dGetSurfaceFormatSupport(PVMSVGA3DSTATE pState3D, uint32_t idx3dCaps, D3DFORMAT format)
{
    NOREF(idx3dCaps);
    HRESULT  hr;
    uint32_t result = 0;

    /* Try if the format can be used for the primary display. */
    hr = pState3D->pD3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT,
                                            D3DDEVTYPE_HAL,
                                            format,
                                            0,
                                            D3DRTYPE_SURFACE,
                                            format);

    for (unsigned i = 0; i < RT_ELEMENTS(g_aFormatSupport); i++)
    {
        hr = pState3D->pD3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT,
                                                D3DDEVTYPE_HAL,
                                                D3DFMT_X8R8G8B8,    /* assume standard 32-bit display mode */
                                                g_aFormatSupport[i].Usage,
                                                g_aFormatSupport[i].ResourceType,
                                                format);
        if (hr == D3D_OK)
            result |= g_aFormatSupport[i].FormatOp;
    }

    /* Check for features only if the format is supported in any form. */
    if (result)
    {
        for (unsigned i = 0; i < RT_ELEMENTS(g_aFeatureReject); i++)
        {
            hr = pState3D->pD3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT,
                                                    D3DDEVTYPE_HAL,
                                                    D3DFMT_X8R8G8B8,    /* assume standard 32-bit display mode */
                                                    g_aFeatureReject[i].Usage,
                                                    g_aFeatureReject[i].ResourceType,
                                                    format);
            if (hr != D3D_OK)
                result |= g_aFeatureReject[i].FormatOp;
        }
    }

    /** @todo missing:
     *
     * SVGA3DFORMAT_OP_PIXELSIZE
     */

    switch (idx3dCaps)
    {
    case SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8:
    case SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5:
    case SVGA3D_DEVCAP_SURFACEFMT_R5G6B5:
        result |= SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB
               |  SVGA3DFORMAT_OP_CONVERT_TO_ARGB
               |  SVGA3DFORMAT_OP_DISPLAYMODE           /* Should not be set for alpha formats. */
               |  SVGA3DFORMAT_OP_3DACCELERATION;       /* implies OP_DISPLAYMODE */
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8:
    case SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10:
    case SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5:
    case SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4:
        result |= SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB
               |  SVGA3DFORMAT_OP_CONVERT_TO_ARGB
               |  SVGA3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET;
        break;

    }
    Log(("CAPS: %s =\n%s\n", vmsvga3dGetCapString(idx3dCaps), vmsvga3dGet3dFormatString(result)));

    return result;
}

static uint32_t vmsvga3dGetDepthFormatSupport(PVMSVGA3DSTATE pState3D, uint32_t idx3dCaps, D3DFORMAT format)
{
    RT_NOREF(idx3dCaps);
    HRESULT  hr;
    uint32_t result = 0;

    hr = pState3D->pD3D9->CheckDeviceFormat(D3DADAPTER_DEFAULT,
                                            D3DDEVTYPE_HAL,
                                            D3DFMT_X8R8G8B8,    /* assume standard 32-bit display mode */
                                            D3DUSAGE_DEPTHSTENCIL,
                                            D3DRTYPE_SURFACE,
                                            format);
    if (hr == D3D_OK)
        result =  SVGA3DFORMAT_OP_ZSTENCIL
                | SVGA3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH
                | SVGA3DFORMAT_OP_TEXTURE /* Necessary for Ubuntu Unity */;

    Log(("CAPS: %s =\n%s\n", vmsvga3dGetCapString(idx3dCaps), vmsvga3dGet3dFormatString(result)));
    return result;
}


int vmsvga3dQueryCaps(PVGASTATE pThis, uint32_t idx3dCaps, uint32_t *pu32Val)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    D3DCAPS9 *pCaps = &pState->caps;
    int       rc = VINF_SUCCESS;

    *pu32Val = 0;

    switch (idx3dCaps)
    {
    case SVGA3D_DEVCAP_3D:
        *pu32Val = 1; /* boolean? */
        break;

    case SVGA3D_DEVCAP_MAX_LIGHTS:
        *pu32Val = pCaps->MaxActiveLights;
        break;

    case SVGA3D_DEVCAP_MAX_TEXTURES:
        *pu32Val = pCaps->MaxSimultaneousTextures;
        break;

    case SVGA3D_DEVCAP_MAX_CLIP_PLANES:
        *pu32Val = pCaps->MaxUserClipPlanes;
        break;

    case SVGA3D_DEVCAP_VERTEX_SHADER_VERSION:
        switch (pCaps->VertexShaderVersion)
        {
        case D3DVS_VERSION(1,1):
            *pu32Val = SVGA3DVSVERSION_11;
            break;
        case D3DVS_VERSION(2,0):
            *pu32Val = SVGA3DVSVERSION_20;
            break;
        case D3DVS_VERSION(3,0):
            *pu32Val = SVGA3DVSVERSION_30;
            break;
        case D3DVS_VERSION(4,0):
            *pu32Val = SVGA3DVSVERSION_40;
            break;
        default:
            LogRel(("VMSVGA: Unsupported vertex shader version %x\n", pCaps->VertexShaderVersion));
            break;
        }
        break;

    case SVGA3D_DEVCAP_VERTEX_SHADER:
        /* boolean? */
        *pu32Val = 1;
        break;

    case SVGA3D_DEVCAP_FRAGMENT_SHADER_VERSION:
        switch (pCaps->PixelShaderVersion)
        {
        case D3DPS_VERSION(1,1):
            *pu32Val = SVGA3DPSVERSION_11;
            break;
        case D3DPS_VERSION(1,2):
            *pu32Val = SVGA3DPSVERSION_12;
            break;
        case D3DPS_VERSION(1,3):
            *pu32Val = SVGA3DPSVERSION_13;
            break;
        case D3DPS_VERSION(1,4):
            *pu32Val = SVGA3DPSVERSION_14;
            break;
        case D3DPS_VERSION(2,0):
            *pu32Val = SVGA3DPSVERSION_20;
            break;
        case D3DPS_VERSION(3,0):
            *pu32Val = SVGA3DPSVERSION_30;
            break;
        case D3DPS_VERSION(4,0):
            *pu32Val = SVGA3DPSVERSION_40;
            break;
        default:
            LogRel(("VMSVGA: Unsupported pixel shader version %x\n", pCaps->PixelShaderVersion));
            break;
        }
        break;

    case SVGA3D_DEVCAP_FRAGMENT_SHADER:
        /* boolean? */
        *pu32Val = 1;
        break;

    case SVGA3D_DEVCAP_S23E8_TEXTURES:
    case SVGA3D_DEVCAP_S10E5_TEXTURES:
        /* Must be obsolete by now; surface format caps specify the same thing. */
        rc = VERR_INVALID_PARAMETER;
        break;

    case SVGA3D_DEVCAP_MAX_FIXED_VERTEXBLEND:
        break;

    /*
     *   2. The BUFFER_FORMAT capabilities are deprecated, and they always
     *      return TRUE. Even on physical hardware that does not support
     *      these formats natively, the SVGA3D device will provide an emulation
     *      which should be invisible to the guest OS.
     */
    case SVGA3D_DEVCAP_D16_BUFFER_FORMAT:
    case SVGA3D_DEVCAP_D24S8_BUFFER_FORMAT:
    case SVGA3D_DEVCAP_D24X8_BUFFER_FORMAT:
        *pu32Val = 1;
        break;

    case SVGA3D_DEVCAP_QUERY_TYPES:
        break;

    case SVGA3D_DEVCAP_TEXTURE_GRADIENT_SAMPLING:
        break;

    case SVGA3D_DEVCAP_MAX_POINT_SIZE:
        AssertCompile(sizeof(uint32_t) == sizeof(float));
        *(float *)pu32Val = pCaps->MaxPointSize;
        break;

    case SVGA3D_DEVCAP_MAX_SHADER_TEXTURES:
        /** @todo ?? */
        rc = VERR_INVALID_PARAMETER;
        break;

    case SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH:
        *pu32Val = pCaps->MaxTextureWidth;
        break;

    case SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT:
        *pu32Val = pCaps->MaxTextureHeight;
        break;

    case SVGA3D_DEVCAP_MAX_VOLUME_EXTENT:
        *pu32Val = pCaps->MaxVolumeExtent;
        break;

    case SVGA3D_DEVCAP_MAX_TEXTURE_REPEAT:
        *pu32Val = pCaps->MaxTextureRepeat;
        break;

    case SVGA3D_DEVCAP_MAX_TEXTURE_ASPECT_RATIO:
        *pu32Val = pCaps->MaxTextureAspectRatio;
        break;

    case SVGA3D_DEVCAP_MAX_TEXTURE_ANISOTROPY:
        *pu32Val = pCaps->MaxAnisotropy;
        break;

    case SVGA3D_DEVCAP_MAX_PRIMITIVE_COUNT:
        *pu32Val = pCaps->MaxPrimitiveCount;
        break;

    case SVGA3D_DEVCAP_MAX_VERTEX_INDEX:
        *pu32Val = pCaps->MaxVertexIndex;
        break;

    case SVGA3D_DEVCAP_MAX_VERTEX_SHADER_INSTRUCTIONS:
        *pu32Val = pCaps->MaxVertexShader30InstructionSlots;
        break;

    case SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_INSTRUCTIONS:
        *pu32Val = pCaps->MaxPixelShader30InstructionSlots;
        break;

    case SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS:
        *pu32Val = pCaps->VS20Caps.NumTemps;
        break;

    case SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS:
        *pu32Val = pCaps->PS20Caps.NumTemps;
        break;

    case SVGA3D_DEVCAP_TEXTURE_OPS:
        break;

    case SVGA3D_DEVCAP_MULTISAMPLE_NONMASKABLESAMPLES:
        break;

    case SVGA3D_DEVCAP_MULTISAMPLE_MASKABLESAMPLES:
        break;

    case SVGA3D_DEVCAP_ALPHATOCOVERAGE:
        break;

    case SVGA3D_DEVCAP_SUPERSAMPLE:
        break;

    case SVGA3D_DEVCAP_AUTOGENMIPMAPS:
        *pu32Val = !!(pCaps->Caps2 & D3DCAPS2_CANAUTOGENMIPMAP);
        break;

    case SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEXTURES:
        break;

    case SVGA3D_DEVCAP_MAX_RENDER_TARGETS:  /** @todo same thing? */
    case SVGA3D_DEVCAP_MAX_SIMULTANEOUS_RENDER_TARGETS:
        *pu32Val = pCaps->NumSimultaneousRTs;
        break;

    /*
     * This is the maximum number of SVGA context IDs that the guest
     * can define using SVGA_3D_CMD_CONTEXT_DEFINE.
     */
    case SVGA3D_DEVCAP_MAX_CONTEXT_IDS:
        *pu32Val = SVGA3D_MAX_CONTEXT_IDS;
        break;

    /*
     * This is the maximum number of SVGA surface IDs that the guest
     * can define using SVGA_3D_CMD_SURFACE_DEFINE*.
     */
    case SVGA3D_DEVCAP_MAX_SURFACE_IDS:
        *pu32Val = SVGA3D_MAX_SURFACE_IDS;
        break;

    /* Supported surface formats. */
    case SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_X8R8G8B8);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_A8R8G8B8);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_A2R10G10B10);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_X1R5G5B5);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_A1R5G5B5);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_A4R4G4B4);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_R5G6B5:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_A4R4G4B4);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE16:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_L16);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8_ALPHA8:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_A8L8);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_ALPHA8:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_A8);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_L8);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_Z_D16:
        *pu32Val = vmsvga3dGetDepthFormatSupport(pState, idx3dCaps, D3DFMT_D16);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8:
    case SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8_INT: /** @todo not correct */
        *pu32Val = vmsvga3dGetDepthFormatSupport(pState, idx3dCaps, D3DFMT_D24S8);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8:
        *pu32Val = vmsvga3dGetDepthFormatSupport(pState, idx3dCaps, D3DFMT_D24X8);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_Z_DF16:
        /** @todo supposed to be floating-point, but unable to find a match for D3D9... */
        *pu32Val = 0;
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_Z_DF24:
        *pu32Val = vmsvga3dGetDepthFormatSupport(pState, idx3dCaps, D3DFMT_D24FS8);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_DXT1:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_DXT1);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_DXT2:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_DXT2);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_DXT3:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_DXT3);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_DXT4:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_DXT4);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_DXT5:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_DXT5);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_BUMPX8L8V8U8:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_X8L8V8U8);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_A2W10V10U10:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_A2W10V10U10);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_BUMPU8V8:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_V8U8);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_Q8W8V8U8:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_Q8W8V8U8);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_CxV8U8:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_CxV8U8);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_R_S10E5:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_R16F);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_R_S23E8:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_R32F);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_RG_S10E5:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_G16R16F);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_RG_S23E8:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_G32R32F);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_ARGB_S10E5:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_A16B16G16R16F);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_ARGB_S23E8:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_A32B32G32R32F);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_V16U16:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_V16U16);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_G16R16:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_G16R16);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_A16B16G16R16:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_A16B16G16R16);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_UYVY:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_UYVY);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_YUY2:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, D3DFMT_YUY2);
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_NV12:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2'));
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_AYUV:
        *pu32Val = vmsvga3dGetSurfaceFormatSupport(pState, idx3dCaps, (D3DFORMAT)MAKEFOURCC('A', 'Y', 'U', 'V'));
        break;

    case SVGA3D_DEVCAP_SURFACEFMT_BC4_UNORM:
    case SVGA3D_DEVCAP_SURFACEFMT_BC5_UNORM:
        /* Unknown; only in DX10 & 11 */
        Log(("CAPS: Unknown CAP %s\n", vmsvga3dGetCapString(idx3dCaps)));
        rc = VERR_INVALID_PARAMETER;
        *pu32Val = 0;
        break;

    default:
        Log(("CAPS: Unexpected CAP %d\n", idx3dCaps));
        rc = VERR_INVALID_PARAMETER;
        break;
    }
#if 0
    /* Dump of VMWare Player caps (from their log); for debugging purposes */
    switch (idx3dCaps)
    {
    case 0:
        *pu32Val = 0x00000001;
        break;
    case 1:
        *pu32Val = 0x0000000a;
        break;
    case 2:
        *pu32Val = 0x00000008;
        break;
    case 3: *pu32Val = 0x00000006; break;
    case 4: *pu32Val = 0x00000007; break;
    case 5: *pu32Val = 0x00000001; break;
    case 6: *pu32Val = 0x0000000d; break;
    case 7: *pu32Val = 0x00000001; break;
    case 8: *pu32Val = 0x00000004; break;
    case 9: *pu32Val = 0x00000001; break;
    case 10: *pu32Val = 0x00000001; break;
    case 11: *pu32Val = 0x00000004; break;
    case 12: *pu32Val = 0x00000001; break;
    case 13: *pu32Val = 0x00000001; break;
    case 14: *pu32Val = 0x00000001; break;
    case 15: *pu32Val = 0x00000001; break;
    case 16: *pu32Val = 0x00000001; break;
    case 17: *pu32Val = (uint32_t)256.000000; break;
    case 18: *pu32Val = 0x00000014; break;
    case 19: *pu32Val = 0x00001000; break;
    case 20: *pu32Val = 0x00001000; break;
    case 21: *pu32Val = 0x00000800; break;
    case 22: *pu32Val = 0x00002000; break;
    case 23: *pu32Val = 0x00000800; break;
    case 24: *pu32Val = 0x00000010; break;
    case 25: *pu32Val = 0x000fffff; break;
    case 26: *pu32Val = 0x00ffffff; break;
    case 27: *pu32Val = 0xffffffff; break;
    case 28: *pu32Val = 0xffffffff; break;
    case 29: *pu32Val = 0x00000020; break;
    case 30: *pu32Val = 0x00000020; break;
    case 31: *pu32Val = 0x03ffffff; break;
    case 32: *pu32Val = 0x0098ec1f; break;
    case 33: *pu32Val = 0x0098e11f; break;
    case 34: *pu32Val = 0x0098e01f; break;
    case 35: *pu32Val = 0x012c2000; break;
    case 36: *pu32Val = 0x0098e11f; break;
    case 37: *pu32Val = 0x0090c11f; break;
    case 38: *pu32Val = 0x0098ec1f; break;
    case 39: *pu32Val = 0x00804007; break;
    case 40: *pu32Val = 0x0080c007; break;
    case 41: *pu32Val = 0x00804007; break;
    case 42: *pu32Val = 0x0080c007; break;
    case 43: *pu32Val = 0x000000c1; break;
    case 44: *pu32Val = 0x000000c1; break;
    case 45: *pu32Val = 0x000000c1; break;
    case 46: *pu32Val = 0x00808005; break;
    case 47: *pu32Val = 0x00808005; break;
    case 48: *pu32Val = 0x00808005; break;
    case 49: *pu32Val = 0x00808005; break;
    case 50: *pu32Val = 0x00808005; break;
    case 51: *pu32Val = 0x01240000; break;
    case 52: *pu32Val = 0x00814007; break;
    case 53: *pu32Val = 0x00814007; break;
    case 54: *pu32Val = 0x00814007; break;
    case 55: *pu32Val = 0x01240000; break;
    case 56: *pu32Val = 0x0080401f; break;
    case 57: *pu32Val = 0x0080401f; break;
    case 58: *pu32Val = 0x0080401f; break;
    case 59: *pu32Val = 0x0080401f; break;
    case 60: *pu32Val = 0x0080601f; break;
    case 61: *pu32Val = 0x0080401f; break;
    case 62: *pu32Val = 0x00000000; break;
    case 63: *pu32Val = 0x00000004; break;
    case 64: *pu32Val = 0x00000004; break;
    case 65: *pu32Val = 0x00814005; break;
    case 66: *pu32Val = 0x0080401f; break;
    case 67: *pu32Val = 0x0080601f; break;
    case 68: *pu32Val = 0x00006009; break;
    case 69: *pu32Val = 0x00006001; break;
    case 70: *pu32Val = 0x00000001; break;
    case 71: *pu32Val = 0x0000000b; break;
    case 72: *pu32Val = 0x00000001; break;
    case 73: *pu32Val = 0x00000000; break;
    case 74: *pu32Val = 0x00000000; break;
    case 75: *pu32Val = 0x01246000; break;
    case 76: *pu32Val = 0x00004009; break;
    case 77: *pu32Val = 0x00000100; break;
    case 78: *pu32Val = 0x00008000; break;
    case 79: *pu32Val = 0x000000c1; break;
    case 80: *pu32Val = 0x01240000; break;
    case 81: *pu32Val = 0x000000c1; break;
    case 82: *pu32Val = 0x00800005; break;
    case 83: *pu32Val = 0x00800005; break;
    case 84: *pu32Val = 0x00000000; break;
    case 85: *pu32Val = 0x00000000; break;
    case 86: *pu32Val = 0x00000000; break;
    case 87: *pu32Val = 0x00000000; break;
    case 88: *pu32Val = 0x00000000; break;
    case 89: *pu32Val = (uint32_t) 0.000000; break;
    case 90: *pu32Val = (uint32_t) 0.000000; break;
    case 91: *pu32Val = 0x00006009; break;
    default:
//        Log(("CAPS: Unexpected CAP %d\n", idx3dCaps));
//        rc = VERR_INVALID_PARAMETER;
        break;
    }
#endif
    Log(("CAPS: %d=%s - %x\n", idx3dCaps, vmsvga3dGetCapString(idx3dCaps), *pu32Val));
    return rc;
}

/**
 * Convert SVGA format value to its D3D equivalent
 */
D3DFORMAT vmsvga3dSurfaceFormat2D3D(SVGA3dSurfaceFormat format)
{
    switch (format)
    {
    case SVGA3D_X8R8G8B8:
        return D3DFMT_X8R8G8B8;
    case SVGA3D_A8R8G8B8:
        return D3DFMT_A8R8G8B8;
    case SVGA3D_R5G6B5:
        return D3DFMT_R5G6B5;
    case SVGA3D_X1R5G5B5:
        return D3DFMT_X1R5G5B5;
    case SVGA3D_A1R5G5B5:
        return D3DFMT_A1R5G5B5;
    case SVGA3D_A4R4G4B4:
        return D3DFMT_A4R4G4B4;

    case SVGA3D_R8G8B8A8_UNORM:
        return D3DFMT_A8B8G8R8;

    case SVGA3D_Z_D32:
        return D3DFMT_D32;
    case SVGA3D_Z_D16:
        return D3DFMT_D16;
    case SVGA3D_Z_D24S8_INT:    /** @todo not correct */
    case SVGA3D_Z_D24S8:
        return D3DFMT_D24S8;
    case SVGA3D_Z_D15S1:
        return D3DFMT_D15S1;
    case SVGA3D_Z_D24X8:
        return D3DFMT_D24X8;
    /* Advanced D3D9 depth formats. */
    case SVGA3D_Z_DF16:
        /** @todo supposed to be floating-point, but unable to find a match for D3D9... */
        AssertFailedReturn(D3DFMT_UNKNOWN);
    case SVGA3D_Z_DF24:
        return D3DFMT_D24FS8;

    case SVGA3D_LUMINANCE8:
        return D3DFMT_L8;
    case SVGA3D_LUMINANCE4_ALPHA4:
        return D3DFMT_A4L4;
    case SVGA3D_LUMINANCE16:
        return D3DFMT_L16;
    case SVGA3D_LUMINANCE8_ALPHA8:
        return D3DFMT_A8L8;

    case SVGA3D_DXT1:
        return D3DFMT_DXT1;
    case SVGA3D_DXT2:
        return D3DFMT_DXT2;
    case SVGA3D_DXT3:
        return D3DFMT_DXT3;
    case SVGA3D_DXT4:
        return D3DFMT_DXT4;
    case SVGA3D_DXT5:
        return D3DFMT_DXT5;

    /* Bump-map formats */
    case SVGA3D_BUMPU8V8:
        return D3DFMT_V8U8;
    case SVGA3D_BUMPL6V5U5:
        return D3DFMT_L6V5U5;
    case SVGA3D_BUMPX8L8V8U8:
        return D3DFMT_X8L8V8U8;
    case SVGA3D_BUMPL8V8U8:
        /* No corresponding D3D9 equivalent. */
        AssertFailedReturn(D3DFMT_UNKNOWN);
    /* signed bump-map formats */
    case SVGA3D_V8U8:
        return D3DFMT_V8U8;
    case SVGA3D_Q8W8V8U8:
        return D3DFMT_Q8W8V8U8;
    case SVGA3D_CxV8U8:
        return D3DFMT_CxV8U8;
    /* mixed bump-map formats */
    case SVGA3D_X8L8V8U8:
        return D3DFMT_X8L8V8U8;
    case SVGA3D_A2W10V10U10:
        return D3DFMT_A2W10V10U10;

    case SVGA3D_ARGB_S10E5:   /* 16-bit floating-point ARGB */
        return D3DFMT_A16B16G16R16F;
    case SVGA3D_ARGB_S23E8:   /* 32-bit floating-point ARGB */
        return D3DFMT_A32B32G32R32F;

    case SVGA3D_A2R10G10B10:
        return D3DFMT_A2R10G10B10;

    case SVGA3D_ALPHA8:
        return D3DFMT_A8;

    /* Single- and dual-component floating point formats */
    case SVGA3D_R_S10E5:
        return D3DFMT_R16F;
    case SVGA3D_R_S23E8:
        return D3DFMT_R32F;
    case SVGA3D_RG_S10E5:
        return D3DFMT_G16R16F;
    case SVGA3D_RG_S23E8:
        return D3DFMT_G32R32F;

    /*
     * Any surface can be used as a buffer object, but SVGA3D_BUFFER is
     * the most efficient format to use when creating new surfaces
     * expressly for index or vertex data.
     */
    case SVGA3D_BUFFER:
        return D3DFMT_UNKNOWN;

    case SVGA3D_V16U16:
        return D3DFMT_V16U16;

    case SVGA3D_G16R16:
        return D3DFMT_G16R16;
    case SVGA3D_A16B16G16R16:
        return D3DFMT_A16B16G16R16;

    /* Packed Video formats */
    case SVGA3D_UYVY:
        return D3DFMT_UYVY;
    case SVGA3D_YUY2:
        return D3DFMT_YUY2;

    /* Planar video formats */
    case SVGA3D_NV12:
        return (D3DFORMAT)MAKEFOURCC('N', 'V', '1', '2');

    /* Video format with alpha */
    case SVGA3D_AYUV:
        return (D3DFORMAT)MAKEFOURCC('A', 'Y', 'U', 'V');

    case SVGA3D_R8G8B8A8_SNORM:
        return D3DFMT_Q8W8V8U8;
    case SVGA3D_R16G16_UNORM:
        return D3DFMT_G16R16;

    case SVGA3D_BC4_UNORM:
    case SVGA3D_BC5_UNORM:
        /* Unknown; only in DX10 & 11 */
        break;

    case SVGA3D_FORMAT_MAX:     /* shut up MSC */
    case SVGA3D_FORMAT_INVALID:
        break;
    }
    AssertFailedReturn(D3DFMT_UNKNOWN);
}

/**
 * Convert SVGA multi sample count value to its D3D equivalent
 */
D3DMULTISAMPLE_TYPE vmsvga3dMultipeSampleCount2D3D(uint32_t multisampleCount)
{
    AssertCompile(D3DMULTISAMPLE_2_SAMPLES == 2);
    AssertCompile(D3DMULTISAMPLE_16_SAMPLES == 16);

    if (multisampleCount > 16)
        return D3DMULTISAMPLE_NONE;

    /** @todo exact same mapping as d3d? */
    return (D3DMULTISAMPLE_TYPE)multisampleCount;
}


/**
 * Destroy backend specific surface bits (part of SVGA_3D_CMD_SURFACE_DESTROY).
 *
 * @param   pState              The VMSVGA3d state.
 * @param   pSurface            The surface being destroyed.
 */
void vmsvga3dBackSurfaceDestroy(PVMSVGA3DSTATE pState, PVMSVGA3DSURFACE pSurface)
{
    RT_NOREF(pState);

    RTAvlU32Destroy(&pSurface->pSharedObjectTree, vmsvga3dSharedSurfaceDestroyTree, pSurface);
    Assert(pSurface->pSharedObjectTree == NULL);

    switch (pSurface->enmD3DResType)
    {
    case VMSVGA3D_D3DRESTYPE_SURFACE:
        D3D_RELEASE(pSurface->u.pSurface);
        break;

    case VMSVGA3D_D3DRESTYPE_TEXTURE:
        D3D_RELEASE(pSurface->u.pTexture);
        D3D_RELEASE(pSurface->bounce.pTexture);
        break;

    case VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE:
        D3D_RELEASE(pSurface->u.pCubeTexture);
        D3D_RELEASE(pSurface->bounce.pCubeTexture);
        break;

    case VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE:
        D3D_RELEASE(pSurface->u.pVolumeTexture);
        D3D_RELEASE(pSurface->bounce.pVolumeTexture);
        break;

    case VMSVGA3D_D3DRESTYPE_VERTEX_BUFFER:
        D3D_RELEASE(pSurface->u.pVertexBuffer);
        break;

    case VMSVGA3D_D3DRESTYPE_INDEX_BUFFER:
        D3D_RELEASE(pSurface->u.pIndexBuffer);
        break;

    default:
        AssertMsg(!VMSVGA3DSURFACE_HAS_HW_SURFACE(pSurface),
                  ("surfaceFlags=0x%x\n", (pSurface->surfaceFlags & VMSVGA3D_SURFACE_HINT_SWITCH_MASK)));
        break;
    }

    D3D_RELEASE(pSurface->pQuery);
}


/*
 * Release all shared surface objects.
 */
DECLCALLBACK(int) vmsvga3dSharedSurfaceDestroyTree(PAVLU32NODECORE pNode, void *pvParam)
{
    PVMSVGA3DSHAREDSURFACE pSharedSurface = (PVMSVGA3DSHAREDSURFACE)pNode;
    PVMSVGA3DSURFACE       pSurface = (PVMSVGA3DSURFACE)pvParam;

    switch (pSurface->enmD3DResType)
    {
    case VMSVGA3D_D3DRESTYPE_TEXTURE:
        LogFunc(("release shared texture object for context %d\n", pNode->Key));
        Assert(pSharedSurface->u.pTexture);
        D3D_RELEASE(pSharedSurface->u.pTexture);
        break;

    case VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE:
        LogFunc(("release shared cube texture object for context %d\n", pNode->Key));
        Assert(pSharedSurface->u.pCubeTexture);
        D3D_RELEASE(pSharedSurface->u.pCubeTexture);
        break;

    case VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE:
        LogFunc(("release shared volume texture object for context %d\n", pNode->Key));
        Assert(pSharedSurface->u.pVolumeTexture);
        D3D_RELEASE(pSharedSurface->u.pVolumeTexture);
        break;

    default:
        AssertFailed();
        break;
    }
    RTMemFree(pNode);
    return 0;
}

/* Get the shared surface copy or create a new one. */
static PVMSVGA3DSHAREDSURFACE vmsvga3dSurfaceGetSharedCopy(PVMSVGA3DCONTEXT pContext, PVMSVGA3DSURFACE pSurface)
{
    Assert(pSurface->hSharedObject);

    PVMSVGA3DSHAREDSURFACE pSharedSurface = (PVMSVGA3DSHAREDSURFACE)RTAvlU32Get(&pSurface->pSharedObjectTree, pContext->id);
    if (!pSharedSurface)
    {
        const uint32_t cWidth = pSurface->pMipmapLevels[0].mipmapSize.width;
        const uint32_t cHeight = pSurface->pMipmapLevels[0].mipmapSize.height;
        const uint32_t cDepth = pSurface->pMipmapLevels[0].mipmapSize.depth;
        const uint32_t numMipLevels = pSurface->faces[0].numMipLevels;

        LogFunc(("Create shared %stexture copy d3d (%d,%d,%d) cMip=%d usage %x format %x.\n",
                  pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE ? "volume " :
                  pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE ? "cube " :
                  pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_TEXTURE ? "" : "UNKNOWN!!!",
                  cWidth,
                  cHeight,
                  cDepth,
                  numMipLevels,
                  pSurface->fUsageD3D | D3DUSAGE_RENDERTARGET,
                  pSurface->formatD3D));

        pSharedSurface = (PVMSVGA3DSHAREDSURFACE)RTMemAllocZ(sizeof(*pSharedSurface));
        AssertReturn(pSharedSurface, NULL);

        pSharedSurface->Core.Key = pContext->id;
        bool ret = RTAvlU32Insert(&pSurface->pSharedObjectTree, &pSharedSurface->Core);
        AssertReturn(ret, NULL);

        /* Create shadow copy of the original shared texture.
         * Shared d3d resources require Vista+ and have some restrictions.
         * D3DUSAGE_RENDERTARGET is required for use as a StretchRect destination.
         */
        HRESULT hr;
        if (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE)
            hr = pContext->pDevice->CreateVolumeTexture(cWidth,
                                                        cHeight,
                                                        cDepth,
                                                        numMipLevels,
                                                        pSurface->fUsageD3D | D3DUSAGE_RENDERTARGET,
                                                        pSurface->formatD3D,
                                                        D3DPOOL_DEFAULT,
                                                        &pSharedSurface->u.pVolumeTexture,
                                                        &pSurface->hSharedObject);
        else if (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE)
            hr = pContext->pDevice->CreateCubeTexture(cWidth,
                                                      numMipLevels,
                                                      pSurface->fUsageD3D | D3DUSAGE_RENDERTARGET,
                                                      pSurface->formatD3D,
                                                      D3DPOOL_DEFAULT,
                                                      &pSharedSurface->u.pCubeTexture,
                                                      &pSurface->hSharedObject);
        else if (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_TEXTURE)
            hr = pContext->pDevice->CreateTexture(cWidth,
                                                  cHeight,
                                                  numMipLevels,
                                                  pSurface->fUsageD3D | D3DUSAGE_RENDERTARGET,
                                                  pSurface->formatD3D,
                                                  D3DPOOL_DEFAULT,
                                                  &pSharedSurface->u.pTexture,
                                                  &pSurface->hSharedObject);
        else
            hr = E_FAIL;

        if (RT_LIKELY(hr == D3D_OK))
            /* likely */;
        else
        {
            AssertMsgFailed(("CreateTexture type %d failed with %x\n", pSurface->enmD3DResType, hr));
            RTAvlU32Remove(&pSurface->pSharedObjectTree, pContext->id);
            RTMemFree(pSharedSurface);
            return NULL;
        }
    }
    return pSharedSurface;
}

/* Inject a query event into the D3D pipeline so we can check when usage of this surface has finished.
 * (D3D does not synchronize shared surface usage)
 */
static int vmsvga3dSurfaceTrackUsage(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, PVMSVGA3DSURFACE pSurface)
{
    RT_NOREF(pState);
#ifndef VBOX_VMSVGA3D_WITH_WINE_OPENGL
    Assert(pSurface->id != SVGA3D_INVALID_ID);

    /* Nothing to do if this surface hasn't been shared. */
    if (pSurface->pSharedObjectTree == NULL)
        return VINF_SUCCESS;

    LogFunc(("track usage of sid=%x (cid=%d) for cid=%d, pQuery %p\n", pSurface->id, pSurface->idAssociatedContext, pContext->id, pSurface->pQuery));

    /* Release the previous query object. */
    D3D_RELEASE(pSurface->pQuery);

    HRESULT hr = pContext->pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &pSurface->pQuery);
    AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSurfaceTrackUsage: CreateQuery failed with %x\n", hr), VERR_INTERNAL_ERROR);

    hr = pSurface->pQuery->Issue(D3DISSUE_END);
    AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSurfaceTrackUsage: Issue failed with %x\n", hr), VERR_INTERNAL_ERROR);
#endif /* !VBOX_VMSVGA3D_WITH_WINE_OPENGL */

    return VINF_SUCCESS;
}


/**
 * Surface ID based version of vmsvga3dSurfaceTrackUsage.
 *
 * @returns VBox status code.
 * @param   pState              The VMSVGA3d state.
 * @param   pContext            The context.
 * @param   sid                 The surface ID.
 */
static int vmsvga3dSurfaceTrackUsageById(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, uint32_t sid)
{
    Assert(sid < SVGA3D_MAX_SURFACE_IDS);
    AssertReturn(sid < pState->cSurfaces, VERR_INVALID_PARAMETER);
    PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];
    AssertReturn(pSurface && pSurface->id == sid, VERR_INVALID_PARAMETER);

    return vmsvga3dSurfaceTrackUsage(pState, pContext, pSurface);
}


/* Wait for all drawing, that uses this surface, to finish. */
int vmsvga3dSurfaceFlush(PVGASTATE pThis, PVMSVGA3DSURFACE pSurface)
{
    RT_NOREF(pThis);
#ifndef VBOX_VMSVGA3D_WITH_WINE_OPENGL
    HRESULT hr;

    if (!pSurface->pQuery)
    {
        LogFlow(("vmsvga3dSurfaceFlush: no query object\n"));
        return VINF_SUCCESS;    /* nothing to wait for */
    }
    Assert(pSurface->pSharedObjectTree);

    Log(("vmsvga3dSurfaceFlush: wait for draw to finish (sid=%x)\n", pSurface->id));
    while (true)
    {
        hr = pSurface->pQuery->GetData(NULL, 0, D3DGETDATA_FLUSH);
        if (hr != S_FALSE) break;

        RTThreadSleep(1);
    }
    AssertMsgReturn(hr == S_OK, ("vmsvga3dSurfaceFinishDrawing: GetData failed with %x\n", hr), VERR_INTERNAL_ERROR);

    D3D_RELEASE(pSurface->pQuery);
#endif /* !VBOX_VMSVGA3D_WITH_WINE_OPENGL */

    return VINF_SUCCESS;
}

/** Get IDirect3DSurface9 for the given face and mipmap.
 */
int vmsvga3dGetD3DSurface(PVMSVGA3DCONTEXT pContext,
                          PVMSVGA3DSURFACE pSurface,
                          uint32_t face,
                          uint32_t mipmap,
                          bool fLockable,
                          IDirect3DSurface9 **ppD3DSurf)
{
    AssertPtrReturn(pSurface->u.pSurface, VERR_INVALID_PARAMETER);

    IDirect3DBaseTexture9 *pTexture;
    if (fLockable && pSurface->bounce.pTexture)
       pTexture = pSurface->bounce.pTexture;
    else
       pTexture = pSurface->u.pTexture;

#ifndef VBOX_VMSVGA3D_WITH_WINE_OPENGL
    if (pSurface->idAssociatedContext != pContext->id)
    {
        AssertMsgReturn(!fLockable,
                        ("Lockable surface must be from the same context (surface cid = %d, req cid = %d)",
                         pSurface->idAssociatedContext, pContext->id),
                        VERR_INVALID_PARAMETER);

        if (   pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_TEXTURE
            || pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE)
        {
            LogFunc(("using texture sid=%x created for another context (%d vs %d)\n",
                     pSurface->id, pSurface->idAssociatedContext, pContext->id));

            PVMSVGA3DSHAREDSURFACE pSharedSurface = vmsvga3dSurfaceGetSharedCopy(pContext, pSurface);
            AssertReturn(pSharedSurface, VERR_INTERNAL_ERROR);

            pTexture = pSharedSurface->u.pTexture;
        }
        else
        {
            AssertMsgFailed(("surface sid=%x created for another context (%d vs %d)\n",
                            pSurface->id, pSurface->idAssociatedContext, pContext->id));
        }
    }
#else
    RT_NOREF(pContext);
#endif

    if (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE)
    {
        Assert(pSurface->cFaces == 6);

        IDirect3DCubeTexture9 *p = (IDirect3DCubeTexture9 *)pTexture;
        D3DCUBEMAP_FACES FaceType = vmsvga3dCubemapFaceFromIndex(face);
        HRESULT hr = p->GetCubeMapSurface(FaceType, mipmap, ppD3DSurf);
        AssertMsgReturn(hr == D3D_OK, ("GetCubeMapSurface failed with %x\n", hr), VERR_INTERNAL_ERROR);
    }
    else if (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_TEXTURE)
    {
        Assert(pSurface->cFaces == 1);
        Assert(face == 0);

        IDirect3DTexture9 *p = (IDirect3DTexture9 *)pTexture;
        HRESULT hr = p->GetSurfaceLevel(mipmap, ppD3DSurf);
        AssertMsgReturn(hr == D3D_OK, ("GetSurfaceLevel failed with %x\n", hr), VERR_INTERNAL_ERROR);
    }
    else if (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_SURFACE)
    {
        pSurface->u.pSurface->AddRef();
        *ppD3DSurf = pSurface->u.pSurface;
    }
    else
    {
        AssertMsgFailedReturn(("No surface for type %d\n", pSurface->enmD3DResType), VERR_INTERNAL_ERROR);
    }

    return VINF_SUCCESS;
}

int vmsvga3dSurfaceCopy(PVGASTATE pThis, SVGA3dSurfaceImageId dest, SVGA3dSurfaceImageId src,
                        uint32_t cCopyBoxes, SVGA3dCopyBox *pBox)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    const uint32_t sidSrc = src.sid;
    const uint32_t sidDest = dest.sid;
    int rc;

    PVMSVGA3DSURFACE pSurfaceSrc;
    rc = vmsvga3dSurfaceFromSid(pState, sidSrc, &pSurfaceSrc);
    AssertRCReturn(rc, rc);

    PVMSVGA3DSURFACE pSurfaceDest;
    rc = vmsvga3dSurfaceFromSid(pState, sidDest, &pSurfaceDest);
    AssertRCReturn(rc, rc);

    PVMSVGA3DMIPMAPLEVEL pMipmapLevelSrc;
    rc = vmsvga3dMipmapLevel(pSurfaceSrc, src.face, src.mipmap, &pMipmapLevelSrc);
    AssertRCReturn(rc, rc);

    PVMSVGA3DMIPMAPLEVEL pMipmapLevelDest;
    rc = vmsvga3dMipmapLevel(pSurfaceDest, dest.face, dest.mipmap, &pMipmapLevelDest);
    AssertRCReturn(rc, rc);

    /* If src is HW and dst is not, then create the dst texture. */
    if (   pSurfaceSrc->u.pSurface
        && !pSurfaceDest->u.pSurface
        && RT_BOOL(pSurfaceDest->surfaceFlags & SVGA3D_SURFACE_HINT_TEXTURE))
    {
        const uint32_t cid = pSurfaceSrc->idAssociatedContext;

        PVMSVGA3DCONTEXT pContext;
        rc = vmsvga3dContextFromCid(pState, cid, &pContext);
        AssertRCReturn(rc, rc);

        LogFunc(("sid=%x type=%x format=%d -> create texture\n", sidDest, pSurfaceDest->surfaceFlags, pSurfaceDest->format));
        rc = vmsvga3dBackCreateTexture(pState, pContext, cid, pSurfaceDest);
        AssertRCReturn(rc, rc);
    }

    Assert(pSurfaceSrc->enmD3DResType != VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE); /// @todo
    Assert(pSurfaceDest->enmD3DResType != VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE); /// @todo

    if (   pSurfaceSrc->u.pSurface
        && pSurfaceDest->u.pSurface)
    {
        const uint32_t cidDst = pSurfaceDest->idAssociatedContext;

        PVMSVGA3DCONTEXT pContextDst;
        rc = vmsvga3dContextFromCid(pState, cidDst, &pContextDst);
        AssertRCReturn(rc, rc);

        /* Must flush the other context's 3d pipeline to make sure all drawing is complete for the surface we're about to use. */
        vmsvga3dSurfaceFlush(pThis, pSurfaceSrc);
        vmsvga3dSurfaceFlush(pThis, pSurfaceDest);

        IDirect3DSurface9 *pSrc;
        rc = vmsvga3dGetD3DSurface(pContextDst, pSurfaceSrc, src.face, src.mipmap, false, &pSrc);
        AssertRCReturn(rc, rc);

        IDirect3DSurface9 *pDest;
        rc = vmsvga3dGetD3DSurface(pContextDst, pSurfaceDest, dest.face, dest.mipmap, false, &pDest);
        AssertRCReturnStmt(rc, D3D_RELEASE(pSrc), rc);

        for (uint32_t i = 0; i < cCopyBoxes; ++i)
        {
            HRESULT hr;

            SVGA3dCopyBox clipBox = pBox[i];
            vmsvgaClipCopyBox(&pMipmapLevelSrc->mipmapSize, &pMipmapLevelDest->mipmapSize, &clipBox);
            if (   !clipBox.w
                || !clipBox.h
                || !clipBox.d)
            {
                LogFunc(("Skipped empty box.\n"));
                continue;
            }

            RECT RectSrc;
            RectSrc.left    = clipBox.srcx;
            RectSrc.top     = clipBox.srcy;
            RectSrc.right   = clipBox.srcx + clipBox.w;   /* exclusive */
            RectSrc.bottom  = clipBox.srcy + clipBox.h;   /* exclusive */

            RECT RectDest;
            RectDest.left   = clipBox.x;
            RectDest.top    = clipBox.y;
            RectDest.right  = clipBox.x + clipBox.w;   /* exclusive */
            RectDest.bottom = clipBox.y + clipBox.h;   /* exclusive */

            LogFunc(("StretchRect copy src sid=%x face=%d mipmap=%d (%d,%d)(%d,%d) to dest sid=%x face=%d mipmap=%d (%d,%d)\n", sidSrc, src.face, src.mipmap, RectSrc.left, RectSrc.top, RectSrc.right, RectSrc.bottom, sidDest, dest.face, dest.mipmap, pBox[i].x, pBox[i].y));

            if (    sidSrc == sidDest
                &&  clipBox.srcx == clipBox.x
                &&  clipBox.srcy == clipBox.y)
            {
                LogFunc(("redundant copy to the same surface at the same coordinates. Ignore.\n"));
                continue;
            }
            Assert(sidSrc != sidDest);
            Assert(!clipBox.srcz && !clipBox.z);

            hr = pContextDst->pDevice->StretchRect(pSrc, &RectSrc, pDest, &RectDest, D3DTEXF_NONE);
            AssertMsgReturnStmt(hr == D3D_OK,
                                ("StretchRect failed with %x\n", hr),
                                D3D_RELEASE(pDest); D3D_RELEASE(pSrc),
                                VERR_INTERNAL_ERROR);
        }

        D3D_RELEASE(pDest);
        D3D_RELEASE(pSrc);

        /* Track the StretchRect operation. */
        vmsvga3dSurfaceTrackUsage(pState, pContextDst, pSurfaceSrc);
        vmsvga3dSurfaceTrackUsage(pState, pContextDst, pSurfaceDest);
    }
    else
    {
        /*
         * Copy from/to memory to/from a surface. Or mem->mem.
         * Use the context of existing HW surface, if any.
         */
        PVMSVGA3DCONTEXT pContext = NULL;
        IDirect3DSurface9 *pD3DSurf = NULL;

        if (pSurfaceSrc->u.pSurface)
        {
            AssertReturn(!pSurfaceDest->u.pSurface, VERR_INTERNAL_ERROR);

            rc = vmsvga3dContextFromCid(pState, pSurfaceSrc->idAssociatedContext, &pContext);
            AssertRCReturn(rc, rc);

            rc = vmsvga3dGetD3DSurface(pContext, pSurfaceSrc, src.face, src.mipmap, true, &pD3DSurf);
            AssertRCReturn(rc, rc);
        }
        else if (pSurfaceDest->u.pSurface)
        {
            AssertReturn(!pSurfaceSrc->u.pSurface, VERR_INTERNAL_ERROR);

            rc = vmsvga3dContextFromCid(pState, pSurfaceDest->idAssociatedContext, &pContext);
            AssertRCReturn(rc, rc);

            rc = vmsvga3dGetD3DSurface(pContext, pSurfaceDest, dest.face, dest.mipmap, true, &pD3DSurf);
            AssertRCReturn(rc, rc);
        }

        for (uint32_t i = 0; i < cCopyBoxes; ++i)
        {
            HRESULT        hr;

            SVGA3dCopyBox clipBox = pBox[i];
            vmsvgaClipCopyBox(&pMipmapLevelSrc->mipmapSize, &pMipmapLevelDest->mipmapSize, &clipBox);
            if (   !clipBox.w
                || !clipBox.h
                || !clipBox.d)
            {
                LogFunc(("Skipped empty box.\n"));
                continue;
            }

            RECT RectSrc;
            RectSrc.left    = clipBox.srcx;
            RectSrc.top     = clipBox.srcy;
            RectSrc.right   = clipBox.srcx + clipBox.w;   /* exclusive */
            RectSrc.bottom  = clipBox.srcy + clipBox.h;   /* exclusive */

            RECT RectDest;
            RectDest.left   = clipBox.x;
            RectDest.top    = clipBox.y;
            RectDest.right  = clipBox.x + clipBox.w;   /* exclusive */
            RectDest.bottom = clipBox.y + clipBox.h;   /* exclusive */

            LogFunc(("(manual) copy sid=%x face=%d mipmap=%d (%d,%d)(%d,%d) to sid=%x face=%d mipmap=%d (%d,%d)\n",
                     sidSrc, src.face, src.mipmap, RectSrc.left, RectSrc.top, RectSrc.right, RectSrc.bottom,
                     sidDest, dest.face, dest.mipmap, pBox[i].x, pBox[i].y));

            Assert(!clipBox.srcz && !clipBox.z);
            Assert(pSurfaceSrc->cbBlock == pSurfaceDest->cbBlock);
            Assert(pSurfaceSrc->cxBlock == pSurfaceDest->cxBlock);
            Assert(pSurfaceSrc->cyBlock == pSurfaceDest->cyBlock);

            uint32_t cBlocksX = (clipBox.w + pSurfaceSrc->cxBlock - 1) / pSurfaceSrc->cxBlock;
            uint32_t cBlocksY = (clipBox.h + pSurfaceSrc->cyBlock - 1) / pSurfaceSrc->cyBlock;

            D3DLOCKED_RECT LockedSrcRect;
            if (!pSurfaceSrc->u.pSurface)
            {
                uint32_t u32BlockX = clipBox.srcx / pSurfaceSrc->cxBlock;
                uint32_t u32BlockY = clipBox.srcy / pSurfaceSrc->cyBlock;
                Assert(u32BlockX * pSurfaceSrc->cxBlock == clipBox.srcx);
                Assert(u32BlockY * pSurfaceSrc->cyBlock == clipBox.srcy);

                LockedSrcRect.pBits = (uint8_t *)pMipmapLevelSrc->pSurfaceData +
                                      pMipmapLevelSrc->cbSurfacePitch * u32BlockY + pSurfaceSrc->cbBlock * u32BlockX;
                LockedSrcRect.Pitch = pMipmapLevelSrc->cbSurfacePitch;
            }
            else
            {
                /* Must flush the context's 3d pipeline to make sure all drawing is complete for the surface we're about to use. */
                vmsvga3dSurfaceFlush(pThis, pSurfaceSrc);

                hr = pD3DSurf->LockRect(&LockedSrcRect, &RectSrc, D3DLOCK_READONLY);
                AssertMsgReturnStmt(hr == D3D_OK, ("LockRect failed with %x\n", hr), D3D_RELEASE(pD3DSurf), VERR_INTERNAL_ERROR);
            }

            D3DLOCKED_RECT LockedDestRect;
            if (!pSurfaceDest->u.pSurface)
            {
                uint32_t u32BlockX = clipBox.x / pSurfaceDest->cxBlock;
                uint32_t u32BlockY = clipBox.y / pSurfaceDest->cyBlock;
                Assert(u32BlockX * pSurfaceDest->cxBlock == clipBox.x);
                Assert(u32BlockY * pSurfaceDest->cyBlock == clipBox.y);

                LockedDestRect.pBits = (uint8_t *)pMipmapLevelDest->pSurfaceData +
                                       pMipmapLevelDest->cbSurfacePitch * u32BlockY + pSurfaceDest->cbBlock * u32BlockX;
                LockedDestRect.Pitch = pMipmapLevelDest->cbSurfacePitch;
                pSurfaceDest->fDirty = true;
            }
            else
            {
                /* Must flush the context's 3d pipeline to make sure all drawing is complete for the surface we're about to use. */
                vmsvga3dSurfaceFlush(pThis, pSurfaceDest);

                hr = pD3DSurf->LockRect(&LockedDestRect, &RectDest, 0);
                AssertMsgReturnStmt(hr == D3D_OK, ("LockRect failed with %x\n", hr), D3D_RELEASE(pD3DSurf), VERR_INTERNAL_ERROR);
            }

            uint8_t *pDest = (uint8_t *)LockedDestRect.pBits;
            const uint8_t *pSrc = (uint8_t *)LockedSrcRect.pBits;
            for (uint32_t j = 0; j < cBlocksY; ++j)
            {
                memcpy(pDest, pSrc, cBlocksX * pSurfaceSrc->cbBlock);
                pDest += LockedDestRect.Pitch;
                pSrc  += LockedSrcRect.Pitch;
            }

            if (pD3DSurf)
            {
                hr = pD3DSurf->UnlockRect();
                AssertMsgReturn(hr == D3D_OK, ("Unlock failed with %x\n", hr), VERR_INTERNAL_ERROR);
            }
        }

        /* If the destination bounce texture has been used, then update the actual texture. */
        if (   pSurfaceDest->u.pTexture
            && pSurfaceDest->bounce.pTexture
            && (   pSurfaceDest->enmD3DResType == VMSVGA3D_D3DRESTYPE_TEXTURE
                || pSurfaceDest->enmD3DResType == VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE))
        {
            AssertMsgReturn(pContext, ("Context is NULL\n"), VERR_INTERNAL_ERROR);

            /* Copy the new content to the actual texture object. */
            IDirect3DBaseTexture9 *pSourceTexture;
            IDirect3DBaseTexture9 *pDestinationTexture;
            if (pSurfaceDest->enmD3DResType == VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE)
            {
                pSourceTexture = pSurfaceDest->bounce.pCubeTexture;
                pDestinationTexture = pSurfaceDest->u.pCubeTexture;
            }
            else
            {
                pSourceTexture = pSurfaceDest->bounce.pTexture;
                pDestinationTexture = pSurfaceDest->u.pTexture;
            }
            HRESULT hr2 = pContext->pDevice->UpdateTexture(pSourceTexture, pDestinationTexture);
            AssertMsg(hr2 == D3D_OK, ("UpdateTexture failed with %x\n", hr2)); RT_NOREF(hr2);
        }

        D3D_RELEASE(pD3DSurf);
    }

    return VINF_SUCCESS;
}


/**
 * Create D3D/OpenGL texture object for the specified surface.
 *
 * Surfaces are created when needed.
 *
 * @param   pState              The VMSVGA3d state.
 * @param   pContext            The context.
 * @param   idAssociatedContext Probably the same as pContext->id.
 * @param   pSurface            The surface to create the texture for.
 */
int vmsvga3dBackCreateTexture(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, uint32_t idAssociatedContext,
                              PVMSVGA3DSURFACE pSurface)

{
    RT_NOREF(pState);
    HRESULT hr;

    Assert(pSurface->hSharedObject == NULL);
    Assert(pSurface->u.pTexture == NULL);
    Assert(pSurface->bounce.pTexture == NULL);
    Assert(pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_NONE);

    const uint32_t cWidth = pSurface->pMipmapLevels[0].mipmapSize.width;
    const uint32_t cHeight = pSurface->pMipmapLevels[0].mipmapSize.height;
    const uint32_t cDepth = pSurface->pMipmapLevels[0].mipmapSize.depth;
    const uint32_t numMipLevels = pSurface->faces[0].numMipLevels;

    /*
     * Create D3D texture object.
     */
    if (pSurface->surfaceFlags & SVGA3D_SURFACE_CUBEMAP)
    {
        Assert(pSurface->cFaces == 6);
        Assert(cWidth == cHeight);
        Assert(cDepth == 1);

        hr = pContext->pDevice->CreateCubeTexture(cWidth,
                                                  numMipLevels,
                                                  pSurface->fUsageD3D,
                                                  pSurface->formatD3D,
                                                  D3DPOOL_DEFAULT,
                                                  &pSurface->u.pCubeTexture,
                                                  &pSurface->hSharedObject);
        if (hr == D3D_OK)
        {
            /* Create another texture object to serve as a bounce buffer as the above texture surface can't be locked. */
            hr = pContext->pDevice->CreateCubeTexture(cWidth,
                                                      numMipLevels,
                                                      (pSurface->fUsageD3D & ~D3DUSAGE_RENDERTARGET) | D3DUSAGE_DYNAMIC /* Lockable */,
                                                      pSurface->formatD3D,
                                                      D3DPOOL_SYSTEMMEM,
                                                      &pSurface->bounce.pCubeTexture,
                                                      NULL);
            AssertMsgReturnStmt(hr == D3D_OK,
                                ("CreateCubeTexture (systemmem) failed with %x\n", hr),
                                D3D_RELEASE(pSurface->u.pCubeTexture),
                                VERR_INTERNAL_ERROR);
        }
        else
        {
            Log(("Format not accepted -> try old method\n"));
            /* The format was probably not accepted; fall back to our old mode. */
            hr = pContext->pDevice->CreateCubeTexture(cWidth,
                                                      numMipLevels,
                                                      (pSurface->fUsageD3D & ~D3DUSAGE_RENDERTARGET) |  D3DUSAGE_DYNAMIC /* Lockable */,
                                                      pSurface->formatD3D,
                                                      D3DPOOL_DEFAULT,
                                                      &pSurface->u.pCubeTexture,
                                                      &pSurface->hSharedObject);
            AssertMsgReturn(hr == D3D_OK, ("CreateCubeTexture (fallback) failed with %x\n", hr), VERR_INTERNAL_ERROR);
        }

        pSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE;
    }
    else if (   pSurface->formatD3D == D3DFMT_D24S8
             || pSurface->formatD3D == D3DFMT_D24X8)
    {
        Assert(pSurface->cFaces == 1);
        Assert(pSurface->faces[0].numMipLevels == 1);
        Assert(cDepth == 1);

        /* Use the INTZ format for a depth/stencil surface that will be used as a texture */
        hr = pContext->pDevice->CreateTexture(cWidth,
                                              cHeight,
                                              1, /* mip levels */
                                              D3DUSAGE_DEPTHSTENCIL,
                                              FOURCC_INTZ,
                                              D3DPOOL_DEFAULT,
                                              &pSurface->u.pTexture,
                                              &pSurface->hSharedObject /* might result in poor performance */);
        AssertMsgReturn(hr == D3D_OK, ("CreateTexture INTZ failed with %x\n", hr), VERR_INTERNAL_ERROR);

        pSurface->fStencilAsTexture = true;
        pSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_TEXTURE;
    }
    else
    {
        if (cDepth > 1)
        {
            hr = pContext->pDevice->CreateVolumeTexture(cWidth,
                                                        cHeight,
                                                        cDepth,
                                                        numMipLevels,
                                                        pSurface->fUsageD3D,
                                                        pSurface->formatD3D,
                                                        D3DPOOL_DEFAULT,
                                                        &pSurface->u.pVolumeTexture,
                                                        &pSurface->hSharedObject);
            if (hr == D3D_OK)
            {
                /* Create another texture object to serve as a bounce buffer as the above texture surface can't be locked. */
                hr = pContext->pDevice->CreateVolumeTexture(cWidth,
                                                            cHeight,
                                                            cDepth,
                                                            numMipLevels,
                                                            (pSurface->fUsageD3D & ~D3DUSAGE_RENDERTARGET) | D3DUSAGE_DYNAMIC /* Lockable */,
                                                            pSurface->formatD3D,
                                                            D3DPOOL_SYSTEMMEM,
                                                            &pSurface->bounce.pVolumeTexture,
                                                            NULL);
                AssertMsgReturnStmt(hr == D3D_OK,
                                    ("CreateVolumeTexture (systemmem) failed with %x\n", hr),
                                    D3D_RELEASE(pSurface->u.pVolumeTexture),
                                    VERR_INTERNAL_ERROR);
            }
            else
            {
                Log(("Format not accepted -> try old method\n"));
                /* The format was probably not accepted; fall back to our old mode. */
                hr = pContext->pDevice->CreateVolumeTexture(cWidth,
                                                            cHeight,
                                                            cDepth,
                                                            numMipLevels,
                                                            (pSurface->fUsageD3D & ~D3DUSAGE_RENDERTARGET) |  D3DUSAGE_DYNAMIC /* Lockable */,
                                                            pSurface->formatD3D,
                                                            D3DPOOL_DEFAULT,
                                                            &pSurface->u.pVolumeTexture,
                                                            &pSurface->hSharedObject);
                AssertMsgReturn(hr == D3D_OK, ("CreateVolumeTexture (fallback) failed with %x\n", hr), VERR_INTERNAL_ERROR);
            }

            pSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE;
        }
        else
        {
            Assert(pSurface->cFaces == 1);

            hr = pContext->pDevice->CreateTexture(cWidth,
                                                  cHeight,
                                                  numMipLevels,
                                                  pSurface->fUsageD3D | D3DUSAGE_RENDERTARGET /* required for use as a StretchRect destination */,
                                                  pSurface->formatD3D,
                                                  D3DPOOL_DEFAULT,
                                                  &pSurface->u.pTexture,
                                                  &pSurface->hSharedObject);
            if (hr == D3D_OK)
            {
                /* Create another texture object to serve as a bounce buffer as the above texture surface can't be locked. */
                hr = pContext->pDevice->CreateTexture(cWidth,
                                                      cHeight,
                                                      numMipLevels,
                                                      (pSurface->fUsageD3D & ~D3DUSAGE_RENDERTARGET) | D3DUSAGE_DYNAMIC /* Lockable */,
                                                      pSurface->formatD3D,
                                                      D3DPOOL_SYSTEMMEM,
                                                      &pSurface->bounce.pTexture,
                                                      NULL);
                AssertMsgReturn(hr == D3D_OK, ("CreateTexture (systemmem) failed with %x\n", hr), VERR_INTERNAL_ERROR);
            }
            else
            {
                Log(("Format not accepted -> try old method\n"));
                /* The format was probably not accepted; fall back to our old mode. */
                hr = pContext->pDevice->CreateTexture(cWidth,
                                                      cHeight,
                                                      numMipLevels,
                                                      (pSurface->fUsageD3D & ~D3DUSAGE_RENDERTARGET) |  D3DUSAGE_DYNAMIC /* Lockable */,
                                                      pSurface->formatD3D,
                                                      D3DPOOL_DEFAULT,
                                                      &pSurface->u.pTexture,
                                                      &pSurface->hSharedObject /* might result in poor performance */);
                AssertMsgReturn(hr == D3D_OK, ("CreateTexture failed with %x\n", hr), VERR_INTERNAL_ERROR);
            }

            pSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_TEXTURE;
        }
    }

    Assert(hr == D3D_OK);

    if (pSurface->autogenFilter != SVGA3D_TEX_FILTER_NONE)
    {
        /* Set the mip map generation filter settings. */
        IDirect3DBaseTexture9 *pBaseTexture;
        if (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE)
            pBaseTexture = pSurface->u.pVolumeTexture;
        else if (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE)
            pBaseTexture = pSurface->u.pCubeTexture;
        else
            pBaseTexture = pSurface->u.pTexture;
        hr = pBaseTexture->SetAutoGenFilterType((D3DTEXTUREFILTERTYPE)pSurface->autogenFilter);
        AssertMsg(hr == D3D_OK, ("vmsvga3dBackCreateTexture: SetAutoGenFilterType failed with %x\n", hr));
    }

    /*
     * Always initialize all mipmap levels using the in memory data
     * to make sure that the just created texture has the up-to-date content.
     * The OpenGL backend does this too.
     */
    Log(("vmsvga3dBackCreateTexture: sync texture\n"));

    if (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE)
    {
        IDirect3DVolumeTexture9 *pVolumeTexture = pSurface->bounce.pVolumeTexture ?
                                                      pSurface->bounce.pVolumeTexture :
                                                      pSurface->u.pVolumeTexture;

        for (uint32_t i = 0; i < numMipLevels; ++i)
        {
            D3DLOCKED_BOX LockedVolume;
            hr = pVolumeTexture->LockBox(i, &LockedVolume, NULL, D3DLOCK_DISCARD);
            AssertMsgBreak(hr == D3D_OK, ("LockBox failed with %x\n", hr));

            PVMSVGA3DMIPMAPLEVEL pMipLevel = &pSurface->pMipmapLevels[i];

            LogFunc(("sync volume texture mipmap level %d (pitch row %x vs %x, slice %x vs %x)\n",
                      i, LockedVolume.RowPitch, pMipLevel->cbSurfacePitch, LockedVolume.SlicePitch, pMipLevel->cbSurfacePlane));


            uint8_t *pDst = (uint8_t *)LockedVolume.pBits;
            const uint8_t *pSrc = (uint8_t *)pMipLevel->pSurfaceData;
            for (uint32_t d = 0; d < cDepth; ++d)
            {
                uint8_t *pRowDst = pDst;
                const uint8_t *pRowSrc = pSrc;
                for (uint32_t h = 0; h < pMipLevel->cBlocksY; ++h)
                {
                    memcpy(pRowDst, pRowSrc, pMipLevel->cbSurfacePitch);
                    pRowDst += LockedVolume.RowPitch;
                    pRowSrc += pMipLevel->cbSurfacePitch;
                }
                pDst += LockedVolume.SlicePitch;
                pSrc += pMipLevel->cbSurfacePlane;
            }

            hr = pVolumeTexture->UnlockBox(i);
            AssertMsgBreak(hr == D3D_OK, ("UnlockBox failed with %x\n", hr));

            pMipLevel->fDirty = false;
        }
    }
    else if (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE)
    {
        IDirect3DCubeTexture9 *pCubeTexture = pSurface->bounce.pCubeTexture ?
                                                  pSurface->bounce.pCubeTexture :
                                                  pSurface->u.pCubeTexture;

        for (uint32_t iFace = 0; iFace < 6; ++iFace)
        {
            const D3DCUBEMAP_FACES Face = vmsvga3dCubemapFaceFromIndex(iFace);

            for (uint32_t i = 0; i < numMipLevels; ++i)
            {
                D3DLOCKED_RECT LockedRect;
                hr = pCubeTexture->LockRect(Face,
                                            i, /* texture level */
                                            &LockedRect,
                                            NULL,   /* entire texture */
                                            0);
                AssertMsgBreak(hr == D3D_OK, ("LockRect failed with %x\n", hr));

                PVMSVGA3DMIPMAPLEVEL pMipLevel = &pSurface->pMipmapLevels[iFace * numMipLevels + i];

                LogFunc(("sync texture face %d mipmap level %d (pitch %x vs %x)\n",
                          iFace, i, LockedRect.Pitch, pMipLevel->cbSurfacePitch));

                uint8_t *pDest = (uint8_t *)LockedRect.pBits;
                const uint8_t *pSrc = (uint8_t *)pMipLevel->pSurfaceData;
                for (uint32_t j = 0; j < pMipLevel->cBlocksY; ++j)
                {
                    memcpy(pDest, pSrc, pMipLevel->cbSurfacePitch);

                    pDest += LockedRect.Pitch;
                    pSrc  += pMipLevel->cbSurfacePitch;
                }

                hr = pCubeTexture->UnlockRect(Face, i /* texture level */);
                AssertMsgBreak(hr == D3D_OK, ("UnlockRect failed with %x\n", hr));

                pMipLevel->fDirty = false;
            }

            if (hr != D3D_OK)
                break;
        }

        if (hr != D3D_OK)
        {
            D3D_RELEASE(pSurface->bounce.pCubeTexture);
            D3D_RELEASE(pSurface->u.pCubeTexture);
            return VERR_INTERNAL_ERROR;
        }
    }
    else
    {
        IDirect3DTexture9 *pTexture = pSurface->bounce.pTexture ? pSurface->bounce.pTexture : pSurface->u.pTexture;

        for (uint32_t i = 0; i < numMipLevels; ++i)
        {
            D3DLOCKED_RECT LockedRect;

            hr = pTexture->LockRect(i, /* texture level */
                                    &LockedRect,
                                    NULL,   /* entire texture */
                                    0);

            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dBackCreateTexture: LockRect failed with %x\n", hr), VERR_INTERNAL_ERROR);

            LogFunc(("sync texture mipmap level %d (pitch %x vs %x)\n", i, LockedRect.Pitch, pSurface->pMipmapLevels[i].cbSurfacePitch));

            uint8_t *pDest = (uint8_t *)LockedRect.pBits;
            const uint8_t *pSrc = (uint8_t *)pSurface->pMipmapLevels[i].pSurfaceData;
            for (uint32_t j = 0; j < pSurface->pMipmapLevels[i].cBlocksY; ++j)
            {
                memcpy(pDest, pSrc, pSurface->pMipmapLevels[i].cbSurfacePitch);

                pDest += LockedRect.Pitch;
                pSrc  += pSurface->pMipmapLevels[i].cbSurfacePitch;
            }

            hr = pTexture->UnlockRect(i /* texture level */);
            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dBackCreateTexture: UnlockRect failed with %x\n", hr), VERR_INTERNAL_ERROR);

            pSurface->pMipmapLevels[i].fDirty = false;
        }
    }

    if (pSurface->bounce.pTexture)
    {
        Log(("vmsvga3dBackCreateTexture: sync dirty texture from bounce buffer\n"));

        if (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE)
            hr = pContext->pDevice->UpdateTexture(pSurface->bounce.pVolumeTexture, pSurface->u.pVolumeTexture);
        else if (pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE)
            hr = pContext->pDevice->UpdateTexture(pSurface->bounce.pCubeTexture, pSurface->u.pCubeTexture);
        else
            hr = pContext->pDevice->UpdateTexture(pSurface->bounce.pTexture, pSurface->u.pTexture);
        AssertMsgReturn(hr == D3D_OK, ("UpdateTexture failed with %x\n", hr), VERR_INTERNAL_ERROR);

        /* We will now use the bounce texture for all memory accesses, so free our surface memory buffer. */
        for (uint32_t i = 0; i < pSurface->faces[0].numMipLevels; i++)
        {
            RTMemFree(pSurface->pMipmapLevels[i].pSurfaceData);
            pSurface->pMipmapLevels[i].pSurfaceData = NULL;
        }
    }
    pSurface->fDirty = false;

    Assert(pSurface->enmD3DResType != VMSVGA3D_D3DRESTYPE_NONE);

    pSurface->surfaceFlags       |= SVGA3D_SURFACE_HINT_TEXTURE;
    pSurface->idAssociatedContext = idAssociatedContext;
    return VINF_SUCCESS;
}


/**
 * Backend worker for implementing SVGA_3D_CMD_SURFACE_STRETCHBLT.
 *
 * @returns VBox status code.
 * @param   pThis               The VGA device instance.
 * @param   pState              The VMSVGA3d state.
 * @param   pDstSurface         The destination host surface.
 * @param   uDstFace            The destination face (valid).
 * @param   uDstMipmap          The destination mipmap level (valid).
 * @param   pDstBox             The destination box.
 * @param   pSrcSurface         The source host surface.
 * @param   uSrcFace            The destination face (valid).
 * @param   uSrcMipmap          The source mimap level (valid).
 * @param   pSrcBox             The source box.
 * @param   enmMode             The strecht blt mode .
 * @param   pContext            The VMSVGA3d context (already current for OGL).
 */
int vmsvga3dBackSurfaceStretchBlt(PVGASTATE pThis, PVMSVGA3DSTATE pState,
                                  PVMSVGA3DSURFACE pDstSurface, uint32_t uDstFace, uint32_t uDstMipmap, SVGA3dBox const *pDstBox,
                                  PVMSVGA3DSURFACE pSrcSurface, uint32_t uSrcFace, uint32_t uSrcMipmap, SVGA3dBox const *pSrcBox,
                                  SVGA3dStretchBltMode enmMode, PVMSVGA3DCONTEXT pContext)
{
    HRESULT hr;
    int rc;

    AssertReturn(pSrcSurface->enmD3DResType != VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE, VERR_NOT_IMPLEMENTED);
    AssertReturn(pDstSurface->enmD3DResType != VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE, VERR_NOT_IMPLEMENTED);

    /* Flush the drawing pipeline for this surface as it could be used in a shared context. */
    vmsvga3dSurfaceFlush(pThis, pSrcSurface);
    vmsvga3dSurfaceFlush(pThis, pDstSurface);

    RECT RectSrc;
    RectSrc.left    = pSrcBox->x;
    RectSrc.top     = pSrcBox->y;
    RectSrc.right   = pSrcBox->x + pSrcBox->w;  /* exclusive */
    RectSrc.bottom  = pSrcBox->y + pSrcBox->h;  /* exclusive */
    Assert(!pSrcBox->z);

    RECT RectDst;
    RectDst.left   = pDstBox->x;
    RectDst.top    = pDstBox->y;
    RectDst.right  = pDstBox->x + pDstBox->w;  /* exclusive */
    RectDst.bottom = pDstBox->y + pDstBox->h;  /* exclusive */
    Assert(!pDstBox->z);

    IDirect3DSurface9 *pSrc;
    rc = vmsvga3dGetD3DSurface(pContext, pSrcSurface, uSrcFace, uSrcMipmap, false, &pSrc);
    AssertRCReturn(rc, rc);

    IDirect3DSurface9 *pDst;
    rc = vmsvga3dGetD3DSurface(pContext, pDstSurface, uDstFace, uDstMipmap, false, &pDst);
    AssertRCReturn(rc, rc);

    D3DTEXTUREFILTERTYPE moded3d;
    switch (enmMode)
    {
    case SVGA3D_STRETCH_BLT_POINT:
        moded3d = D3DTEXF_POINT;
        break;

    case SVGA3D_STRETCH_BLT_LINEAR:
        moded3d = D3DTEXF_LINEAR;
        break;

    default:
        AssertFailed();
        moded3d = D3DTEXF_NONE;
        break;
    }

    hr = pContext->pDevice->StretchRect(pSrc, &RectSrc, pDst, &RectDst, moded3d);

    D3D_RELEASE(pDst);
    D3D_RELEASE(pSrc);

    AssertMsgReturn(hr == D3D_OK, ("StretchRect failed with %x\n", hr), VERR_INTERNAL_ERROR);

    /* Track the StretchRect operation. */
    vmsvga3dSurfaceTrackUsage(pState, pContext, pSrcSurface);
    vmsvga3dSurfaceTrackUsage(pState, pContext, pDstSurface);

    return VINF_SUCCESS;
}


/**
 * Backend worker for implementing SVGA_3D_CMD_SURFACE_DMA that copies one box.
 *
 * @returns Failure status code or @a rc.
 * @param   pThis               The VGA device instance data.
 * @param   pState              The VMSVGA3d state.
 * @param   pSurface            The host surface.
 * @param   pMipLevel           Mipmap level. The caller knows it already.
 * @param   uHostFace           The host face (valid).
 * @param   uHostMipmap         The host mipmap level (valid).
 * @param   GuestPtr            The guest pointer.
 * @param   cbGuestPitch        The guest pitch.
 * @param   transfer            The transfer direction.
 * @param   pBox                The box to copy (clipped, valid).
 * @param   pContext            The context (for OpenGL).
 * @param   rc                  The current rc for all boxes.
 * @param   iBox                The current box number (for Direct 3D).
 */
int vmsvga3dBackSurfaceDMACopyBox(PVGASTATE pThis, PVMSVGA3DSTATE pState, PVMSVGA3DSURFACE pSurface,
                                  PVMSVGA3DMIPMAPLEVEL pMipLevel, uint32_t uHostFace, uint32_t uHostMipmap,
                                  SVGAGuestPtr GuestPtr, uint32_t cbGuestPitch, SVGA3dTransferType transfer,
                                  SVGA3dCopyBox const *pBox, PVMSVGA3DCONTEXT pContext, int rc, int iBox)
{
    HRESULT hr = D3D_OK;
    const uint32_t u32SurfHints = pSurface->surfaceFlags & VMSVGA3D_SURFACE_HINT_SWITCH_MASK;
    const DWORD dwFlags = transfer == SVGA3D_READ_HOST_VRAM ? D3DLOCK_READONLY : 0;
    // if (u32SurfHints != 0x18 && u32SurfHints != 0x60) ASMBreakpoint();

    AssertReturn(pSurface->enmD3DResType != VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE, VERR_NOT_IMPLEMENTED);

    const bool fTexture =    pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_TEXTURE
                          || pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE;
    if (   pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_SURFACE
        || fTexture)
    {
        rc = vmsvga3dContextFromCid(pState, pSurface->idAssociatedContext, &pContext);
        AssertRCReturn(rc, rc);

        /* Get the surface involved in the transfer. */
        IDirect3DSurface9 *pSurf;
        rc = vmsvga3dGetD3DSurface(pContext, pSurface, uHostFace, uHostMipmap, true, &pSurf);
        AssertRCReturn(rc, rc);

        /** @todo inefficient for VRAM buffers!! */
        if (fTexture)
        {
            if (pSurface->bounce.pTexture)
            {
                if (   transfer == SVGA3D_READ_HOST_VRAM
                    && RT_BOOL(u32SurfHints & SVGA3D_SURFACE_HINT_RENDERTARGET)
                    && iBox == 0 /* only the first time */)
                {
                    /* Copy the texture mipmap level to the bounce texture. */

                    /* Source is the texture, destination is the bounce texture. */
                    IDirect3DSurface9 *pSrc;
                    rc = vmsvga3dGetD3DSurface(pContext, pSurface, uHostFace, uHostMipmap, false, &pSrc);
                    AssertRCReturn(rc, rc);

                    Assert(pSurf != pSrc);

                    hr = pContext->pDevice->GetRenderTargetData(pSrc, pSurf);
                    AssertMsgReturn(hr == D3D_OK, ("GetRenderTargetData failed with %x\n", hr), VERR_INTERNAL_ERROR);

                    D3D_RELEASE(pSrc);
                }
            }
        }

        RECT Rect;
        Rect.left   = pBox->x;
        Rect.top    = pBox->y;
        Rect.right  = pBox->x + pBox->w;   /* exclusive */
        Rect.bottom = pBox->y + pBox->h;   /* exclusive */

        D3DLOCKED_RECT LockedRect;
        hr = pSurf->LockRect(&LockedRect, &Rect, dwFlags);
        AssertMsgReturn(hr == D3D_OK, ("LockRect failed with %x\n", hr), VERR_INTERNAL_ERROR);

        LogFunc(("Lock sid=%x %s(bounce=%d) memory for rectangle (%d,%d)(%d,%d)\n",
                 pSurface->id, fTexture ? "TEXTURE " : "", RT_BOOL(pSurface->bounce.pTexture),
                 Rect.left, Rect.top, Rect.right, Rect.bottom));

        uint32_t u32BlockX = pBox->srcx / pSurface->cxBlock;
        uint32_t u32BlockY = pBox->srcy / pSurface->cyBlock;
        Assert(u32BlockX * pSurface->cxBlock == pBox->srcx);
        Assert(u32BlockY * pSurface->cyBlock == pBox->srcy);
        uint32_t cBlocksX = (pBox->w + pSurface->cxBlock - 1) / pSurface->cxBlock;
        uint32_t cBlocksY = (pBox->h + pSurface->cyBlock - 1) / pSurface->cyBlock;

        rc = vmsvgaGMRTransfer(pThis,
                               transfer,
                               (uint8_t *)LockedRect.pBits,
                               LockedRect.Pitch,
                               GuestPtr,
                               u32BlockX * pSurface->cbBlock + u32BlockY * cbGuestPitch,
                               cbGuestPitch,
                               cBlocksX * pSurface->cbBlock,
                               cBlocksY);
        AssertRC(rc);

        Log4(("first line:\n%.*Rhxd\n", cBlocksX * pSurface->cbBlock, LockedRect.pBits));

        hr = pSurf->UnlockRect();

        D3D_RELEASE(pSurf);

        AssertMsgReturn(hr == D3D_OK, ("UnlockRect failed with %x\n", hr), VERR_INTERNAL_ERROR);

        if (fTexture)
        {
            if (pSurface->bounce.pTexture)
            {
                if (transfer == SVGA3D_WRITE_HOST_VRAM)
                {
                    LogFunc(("Sync texture from bounce buffer\n"));

                    /* Copy the new contents to the actual texture object. */
                    hr = pContext->pDevice->UpdateTexture(pSurface->bounce.pTexture, pSurface->u.pTexture);
                    AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSurfaceDMA: UpdateTexture failed with %x\n", hr), VERR_INTERNAL_ERROR);

                    /* Track the copy operation. */
                    vmsvga3dSurfaceTrackUsage(pState, pContext, pSurface);
                }
            }
        }
    }
    else if (   pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_VERTEX_BUFFER
             || pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_INDEX_BUFFER)
    {
        /*
         * Mesa SVGA driver can use the same buffer either for vertex or index data.
         * But D3D distinguishes between index and vertex buffer objects.
         * Therefore it should be possible to switch the buffer type on the fly.
         *
         * Always save the data to the memory buffer in pSurface->pMipmapLevels and,
         * if necessary, recreate the corresponding D3D object with the data.
         */

        /* Buffers are uncompressed. */
        AssertReturn(pSurface->cxBlock == 1 && pSurface->cyBlock == 1, VERR_INTERNAL_ERROR);

#ifdef OLD_DRAW_PRIMITIVES
        /* Current type of the buffer. */
        const bool fVertex = pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_VERTEX_BUFFER;
#endif

        /* Caller already clipped pBox and buffers are 1-dimensional. */
        Assert(pBox->y == 0 && pBox->h == 1 && pBox->z == 0 && pBox->d == 1);

        const uint32_t uHostOffset = pBox->x * pSurface->cbBlock;
        const uint32_t cbWidth = pBox->w * pSurface->cbBlock;

        AssertReturn(uHostOffset < pMipLevel->cbSurface, VERR_INTERNAL_ERROR);
        AssertReturn(cbWidth <= pMipLevel->cbSurface, VERR_INTERNAL_ERROR);
        AssertReturn(uHostOffset <= pMipLevel->cbSurface - cbWidth, VERR_INTERNAL_ERROR);

        uint8_t *pu8HostData = (uint8_t *)pMipLevel->pSurfaceData + uHostOffset;

        const uint32_t uGuestOffset = pBox->srcx * pSurface->cbBlock;

        /* Copy data between the guest and the host buffer. */
        rc = vmsvgaGMRTransfer(pThis,
                               transfer,
                               pu8HostData,
                               pMipLevel->cbSurfacePitch,
                               GuestPtr,
                               uGuestOffset,
                               cbGuestPitch,
                               cbWidth,
                               1); /* Buffers are 1-dimensional */
        AssertRC(rc);

        Log4(("Buffer first line:\n%.*Rhxd\n", cbWidth, pu8HostData));

        /* Do not bother to copy the data to the D3D resource now. vmsvga3dDrawPrimitives will do that.
         * The SVGA driver may use the same surface for both index and vertex data.
         */

        /* Make sure that vmsvga3dDrawPrimitives fetches the new data. */
        pMipLevel->fDirty = true;
        pSurface->fDirty = true;

#ifdef OLD_DRAW_PRIMITIVES
        /* Also copy the data to the current D3D buffer object. */
        uint8_t *pu8Buffer = NULL;
        /** @todo lock only as much as we really need */
        if (fVertex)
            hr = pSurface->u.pVertexBuffer->Lock(0, 0, (void **)&pu8Buffer, dwFlags);
        else
            hr = pSurface->u.pIndexBuffer->Lock(0, 0, (void **)&pu8Buffer, dwFlags);
        AssertMsgReturn(hr == D3D_OK, ("Lock %s failed with %x\n", fVertex ? "vertex" : "index", hr), VERR_INTERNAL_ERROR);

        LogFunc(("Lock %s memory for rectangle (%d,%d)(%d,%d)\n", fVertex ? "vertex" : "index", pBox->x, pBox->y, pBox->x + pBox->w, pBox->y + pBox->h));

        const uint8_t *pu8Src = pu8HostData;
        uint8_t *pu8Dst = pu8Buffer + uHostOffset;
        memcpy(pu8Dst, pu8Src, cbWidth);

        if (fVertex)
            hr = pSurface->u.pVertexBuffer->Unlock();
        else
            hr = pSurface->u.pIndexBuffer->Unlock();
        AssertMsg(hr == D3D_OK, ("Unlock %s failed with %x\n", fVertex ? "vertex" : "index", hr));
#endif
    }
    else
    {
        AssertMsgFailed(("Unsupported surface hint 0x%08X, type %d\n", u32SurfHints, pSurface->enmD3DResType));
    }

    return rc;
}


int vmsvga3dSurfaceBlitToScreen(PVGASTATE pThis, uint32_t dest, SVGASignedRect destRect, SVGA3dSurfaceImageId src, SVGASignedRect srcRect, uint32_t cRects, SVGASignedRect *pRect)
{
    /* Requires SVGA_FIFO_CAP_SCREEN_OBJECT support */
    Log(("vmsvga3dSurfaceBlitToScreen: dest=%d (%d,%d)(%d,%d) sid=%x (face=%d, mipmap=%d) (%d,%d)(%d,%d) cRects=%d\n", dest, destRect.left, destRect.top, destRect.right, destRect.bottom, src.sid, src.face, src.mipmap, srcRect.left, srcRect.top, srcRect.right, srcRect.bottom, cRects));
    for (uint32_t i = 0; i < cRects; i++)
    {
        Log(("vmsvga3dSurfaceBlitToScreen: clipping rect %d (%d,%d)(%d,%d)\n", i, pRect[i].left, pRect[i].top, pRect[i].right, pRect[i].bottom));
    }

    /** @todo Only screen 0 for now. */
    AssertReturn(dest == 0, VERR_INTERNAL_ERROR);
    AssertReturn(src.mipmap == 0 && src.face == 0, VERR_INVALID_PARAMETER);
    /** @todo scaling */
    AssertReturn(destRect.right - destRect.left == srcRect.right - srcRect.left && destRect.bottom - destRect.top == srcRect.bottom - srcRect.top, VERR_INVALID_PARAMETER);

    if (cRects == 0)
    {
        /* easy case; no clipping */
        SVGA3dCopyBox        box;
        SVGA3dGuestImage     dest;

        box.x       = destRect.left;
        box.y       = destRect.top;
        box.z       = 0;
        box.w       = destRect.right - destRect.left;
        box.h       = destRect.bottom - destRect.top;
        box.d       = 1;
        box.srcx    = srcRect.left;
        box.srcy    = srcRect.top;
        box.srcz    = 0;

        dest.ptr.gmrId  = SVGA_GMR_FRAMEBUFFER;
        dest.ptr.offset = 0;
        dest.pitch      = pThis->svga.cbScanline;

        int rc = vmsvga3dSurfaceDMA(pThis, dest, src, SVGA3D_READ_HOST_VRAM, 1, &box);
        AssertRCReturn(rc, rc);

        vgaR3UpdateDisplay(pThis, box.x, box.y, box.w, box.h);
        return VINF_SUCCESS;
    }
    else
    {
        SVGA3dGuestImage dest;
        SVGA3dCopyBox    box;

        box.srcz    = 0;
        box.z       = 0;
        box.d       = 1;

        dest.ptr.gmrId  = SVGA_GMR_FRAMEBUFFER;
        dest.ptr.offset = 0;
        dest.pitch      = pThis->svga.cbScanline;

        /** @todo merge into one SurfaceDMA call */
        for (uint32_t i = 0; i < cRects; i++)
        {
            /* The clipping rectangle is relative to the top-left corner of srcRect & destRect. Adjust here. */
            box.srcx = srcRect.left + pRect[i].left;
            box.srcy = srcRect.top  + pRect[i].top;

            box.x    = pRect[i].left + destRect.left;
            box.y    = pRect[i].top  + destRect.top;
            box.z    = 0;
            box.w    = pRect[i].right - pRect[i].left;
            box.h    = pRect[i].bottom - pRect[i].top;

            int rc = vmsvga3dSurfaceDMA(pThis, dest, src, SVGA3D_READ_HOST_VRAM, 1, &box);
            AssertRCReturn(rc, rc);

            vgaR3UpdateDisplay(pThis, box.x, box.y, box.w, box.h);
        }

#if 0
        {
            PVMSVGA3DSTATE      pState = pThis->svga.p3dState;
            HRESULT hr;
            PVMSVGA3DSURFACE    pSurface;
            PVMSVGA3DCONTEXT    pContext;
            uint32_t            sid = src.sid;
            AssertReturn(sid < SVGA3D_MAX_SURFACE_IDS, VERR_INVALID_PARAMETER);
            AssertReturn(sid < pState->cSurfaces && pState->papSurfaces[sid]->id == sid, VERR_INVALID_PARAMETER);

            pSurface = pState->papSurfaces[sid];
            uint32_t            cid;

            /** @todo stricter checks for associated context */
            cid = pSurface->idAssociatedContext;

            if (    cid >= pState->cContexts
                ||  pState->papContexts[cid]->id != cid)
            {
                Log(("vmsvga3dGenerateMipmaps invalid context id!\n"));
                return VERR_INVALID_PARAMETER;
            }
            pContext = pState->papContexts[cid];

            if (pSurface->id == 0x5e)
            {
                IDirect3DSurface9 *pSrc;

                hr = pSurface->u.pTexture->GetSurfaceLevel(0/* Texture level */,
                                                           &pSrc);
                AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSurfaceStretchBlt: GetSurfaceLevel failed with %x\n", hr), VERR_INTERNAL_ERROR);

                pContext->pDevice->ColorFill(pSrc, NULL, (D3DCOLOR)0x11122255);
                D3D_RELEASE(pSrc);
            }
        }
#endif

        return VINF_SUCCESS;
    }
}

int vmsvga3dGenerateMipmaps(PVGASTATE pThis, uint32_t sid, SVGA3dTextureFilter filter)
{
    PVMSVGA3DSTATE      pState = pThis->svga.p3dState;
    PVMSVGA3DSURFACE    pSurface;
    int                 rc = VINF_SUCCESS;
    HRESULT             hr;

    AssertReturn(pState, VERR_NO_MEMORY);
    AssertReturn(sid < SVGA3D_MAX_SURFACE_IDS, VERR_INVALID_PARAMETER);
    AssertReturn(sid < pState->cSurfaces && pState->papSurfaces[sid]->id == sid, VERR_INVALID_PARAMETER);

    pSurface = pState->papSurfaces[sid];
    AssertReturn(pSurface->idAssociatedContext != SVGA3D_INVALID_ID, VERR_INTERNAL_ERROR);

    Assert(filter != SVGA3D_TEX_FILTER_FLATCUBIC);
    Assert(filter != SVGA3D_TEX_FILTER_GAUSSIANCUBIC);
    pSurface->autogenFilter = filter;

    Log(("vmsvga3dGenerateMipmaps: sid=%x filter=%d\n", sid, filter));

    if (!pSurface->u.pSurface)
    {
        PVMSVGA3DCONTEXT    pContext;
        uint32_t            cid;

        /** @todo stricter checks for associated context */
        cid = pSurface->idAssociatedContext;

        if (    cid >= pState->cContexts
            ||  pState->papContexts[cid]->id != cid)
        {
            Log(("vmsvga3dGenerateMipmaps invalid context id!\n"));
            return VERR_INVALID_PARAMETER;
        }
        pContext = pState->papContexts[cid];

        /* Unknown surface type; turn it into a texture. */
        LogFunc(("unknown src surface sid=%x type=%d format=%d -> create texture\n", sid, pSurface->surfaceFlags, pSurface->format));
        rc = vmsvga3dBackCreateTexture(pState, pContext, cid, pSurface);
        AssertRCReturn(rc, rc);
    }
    else
    {
        hr = pSurface->u.pTexture->SetAutoGenFilterType((D3DTEXTUREFILTERTYPE)filter);
        AssertMsg(hr == D3D_OK, ("SetAutoGenFilterType failed with %x\n", hr));
    }

    /* Generate the mip maps. */
    pSurface->u.pTexture->GenerateMipSubLevels();

    return VINF_SUCCESS;
}

int vmsvga3dCommandPresent(PVGASTATE pThis, uint32_t sid, uint32_t cRects, SVGA3dCopyRect *pRect)
{
    PVMSVGA3DSTATE      pState = pThis->svga.p3dState;
    PVMSVGA3DSURFACE    pSurface;
    PVMSVGA3DCONTEXT    pContext;
    HRESULT             hr;
    int                 rc;
    IDirect3DSurface9  *pBackBuffer;
    IDirect3DSurface9  *pSurfaceD3D;

    AssertReturn(pState, VERR_NO_MEMORY);

    rc = vmsvga3dSurfaceFromSid(pState, sid, &pSurface);
    AssertRCReturn(rc, rc);

    AssertReturn(pSurface->idAssociatedContext != SVGA3D_INVALID_ID, VERR_INTERNAL_ERROR);

    LogFunc(("sid=%x cRects=%d cid=%x\n", sid, cRects, pSurface->idAssociatedContext));
    for (uint32_t i = 0; i < cRects; ++i)
    {
        LogFunc(("rectangle %d src=(%d,%d) (%d,%d)(%d,%d)\n", i, pRect[i].srcx, pRect[i].srcy, pRect[i].x, pRect[i].y, pRect[i].x + pRect[i].w, pRect[i].y + pRect[i].h));
    }

    rc = vmsvga3dContextFromCid(pState, pSurface->idAssociatedContext, &pContext);
    AssertRCReturn(rc, rc);

    hr = pContext->pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    AssertMsgReturn(hr == D3D_OK, ("GetBackBuffer failed with %x\n", hr), VERR_INTERNAL_ERROR);

    rc = vmsvga3dGetD3DSurface(pContext, pSurface, 0, 0, false, &pSurfaceD3D);
    AssertRCReturn(rc, rc);

    /* Read the destination viewport specs in one go to try avoid some unnecessary update races. */
    VMSVGAVIEWPORT const DstViewport = pThis->svga.viewport;
    ASMCompilerBarrier(); /* paranoia */
    Assert(DstViewport.yHighWC >= DstViewport.yLowWC);

    /* If there are no recangles specified, just grab a screenful. */
    SVGA3dCopyRect DummyRect;
    if (cRects != 0)
    { /* likely */ }
    else
    {
        /** @todo Find the usecase for this or check what the original device does.
         *        The original code was doing some scaling based on the surface
         *        size... */
# ifdef DEBUG_bird
        AssertMsgFailed(("No rects to present. Who is doing that and what do they actually expect?\n"));
# endif
        DummyRect.x = DummyRect.srcx = 0;
        DummyRect.y = DummyRect.srcy = 0;
        DummyRect.w = pThis->svga.uWidth;
        DummyRect.h = pThis->svga.uHeight;
        cRects = 1;
        pRect  = &DummyRect;
    }

    /*
     * Blit the surface rectangle(s) to the back buffer.
     */
    Assert(pSurface->cxBlock == 1 && pSurface->cyBlock == 1);
    uint32_t const cxSurface = pSurface->pMipmapLevels[0].mipmapSize.width;
    uint32_t const cySurface = pSurface->pMipmapLevels[0].mipmapSize.height;
    for (uint32_t i = 0; i < cRects; i++)
    {
        SVGA3dCopyRect ClippedRect = pRect[i];

        /*
         * Do some sanity checking and limit width and height, all so we
         * don't need to think about wrap-arounds below.
         */
        if (RT_LIKELY(   ClippedRect.w
                      && ClippedRect.x    < VMSVGA_MAX_X
                      && ClippedRect.srcx < VMSVGA_MAX_X
                      && ClippedRect.h
                      && ClippedRect.y    < VMSVGA_MAX_Y
                      && ClippedRect.srcy < VMSVGA_MAX_Y
                         ))
        { /* likely */ }
        else
            continue;

        if (RT_LIKELY(ClippedRect.w < VMSVGA_MAX_Y))
        { /* likely */ }
        else
            ClippedRect.w = VMSVGA_MAX_Y;
        if (RT_LIKELY(ClippedRect.w < VMSVGA_MAX_Y))
        { /* likely */ }
        else
            ClippedRect.w = VMSVGA_MAX_Y;

        /*
         * Source surface clipping (paranoia). Straight forward.
         */
        if (RT_LIKELY(ClippedRect.srcx < cxSurface))
        { /* likely */ }
        else
            continue;
        if (RT_LIKELY(ClippedRect.srcx + ClippedRect.w <= cxSurface))
        { /* likely */ }
        else
        {
            AssertFailed(); /* remove if annoying. */
            ClippedRect.w = cxSurface - ClippedRect.srcx;
        }

        if (RT_LIKELY(ClippedRect.srcy < cySurface))
        { /* likely */ }
        else
            continue;
        if (RT_LIKELY(ClippedRect.srcy + ClippedRect.h <= cySurface))
        { /* likely */ }
        else
        {
            AssertFailed(); /* remove if annoying. */
            ClippedRect.h = cySurface - ClippedRect.srcy;
        }

        /*
         * Destination viewport clipping.
         *
         * This is very straight forward compared to OpenGL.  There is no Y
         * inversion anywhere and all the coordinate systems are the same.
         */
        /* X */
        if (ClippedRect.x >= DstViewport.x)
        {
            if (ClippedRect.x + ClippedRect.w <= DstViewport.xRight)
            { /* typical */ }
            else if (ClippedRect.x < DstViewport.xRight)
                ClippedRect.w = DstViewport.xRight - ClippedRect.x;
            else
                continue;
        }
        else
        {
            uint32_t cxAdjust = DstViewport.x - ClippedRect.x;
            if (cxAdjust < ClippedRect.w)
            {
                ClippedRect.w    -= cxAdjust;
                ClippedRect.x    += cxAdjust;
                ClippedRect.srcx += cxAdjust;
            }
            else
                continue;

            if (ClippedRect.x + ClippedRect.w <= DstViewport.xRight)
            { /* typical */ }
            else
                ClippedRect.w = DstViewport.xRight - ClippedRect.x;
        }

        /* Y */
        if (ClippedRect.y >= DstViewport.y)
        {
            if (ClippedRect.y + ClippedRect.h <= DstViewport.y + DstViewport.cy)
            { /* typical */ }
            else if (ClippedRect.x < DstViewport.y + DstViewport.cy)
                ClippedRect.h = DstViewport.y + DstViewport.cy - ClippedRect.y;
            else
                continue;
        }
        else
        {
            uint32_t cyAdjust = DstViewport.y - ClippedRect.y;
            if (cyAdjust < ClippedRect.h)
            {
                ClippedRect.h    -= cyAdjust;
                ClippedRect.y    += cyAdjust;
                ClippedRect.srcy += cyAdjust;
            }
            else
                continue;

            if (ClippedRect.y + ClippedRect.h <= DstViewport.y + DstViewport.cy)
            { /* typical */ }
            else
                ClippedRect.h = DstViewport.y + DstViewport.cy - ClippedRect.y;
        }

        /* Calc source rectangle. */
        RECT SrcRect;
        SrcRect.left   = ClippedRect.srcx;
        SrcRect.right  = ClippedRect.srcx + ClippedRect.w;
        SrcRect.top    = ClippedRect.srcy;
        SrcRect.bottom = ClippedRect.srcy + ClippedRect.h;

        /* Calc destination rectangle. */
        RECT DstRect;
        DstRect.left   = ClippedRect.x;
        DstRect.right  = ClippedRect.x + ClippedRect.w;
        DstRect.top    = ClippedRect.y;
        DstRect.bottom = ClippedRect.y + ClippedRect.h;

        /* Adjust for viewport. */
        DstRect.left   -= DstViewport.x;
        DstRect.right  -= DstViewport.x;
        DstRect.bottom -= DstViewport.y;
        DstRect.top    -= DstViewport.y;

        Log(("SrcRect: (%d,%d)(%d,%d) DstRect: (%d,%d)(%d,%d)\n",
             SrcRect.left, SrcRect.bottom, SrcRect.right, SrcRect.top,
             DstRect.left, DstRect.bottom, DstRect.right, DstRect.top));
        hr = pContext->pDevice->StretchRect(pSurfaceD3D, &SrcRect, pBackBuffer, &DstRect, D3DTEXF_NONE);
        AssertBreak(hr == D3D_OK);
    }

    D3D_RELEASE(pSurfaceD3D);
    D3D_RELEASE(pBackBuffer);

    AssertMsgReturn(hr == D3D_OK, ("StretchRect failed with %x\n", hr), VERR_INTERNAL_ERROR);

    hr = pContext->pDevice->Present(NULL, NULL, NULL, NULL);
    AssertMsgReturn(hr == D3D_OK, ("Present failed with %x\n", hr), VERR_INTERNAL_ERROR);

    return VINF_SUCCESS;
}


/**
 * Create a new 3d context
 *
 * @returns VBox status code.
 * @param   pThis           VGA device instance data.
 * @param   cid             Context id
 */
int vmsvga3dContextDefine(PVGASTATE pThis, uint32_t cid)
{
    int                     rc;
    PVMSVGA3DCONTEXT        pContext;
    HRESULT                 hr;
    D3DPRESENT_PARAMETERS   PresParam;
    PVMSVGA3DSTATE          pState = pThis->svga.p3dState;

    Log(("vmsvga3dContextDefine id %x\n", cid));

    AssertReturn(pState, VERR_NO_MEMORY);
    AssertReturn(cid < SVGA3D_MAX_CONTEXT_IDS, VERR_INVALID_PARAMETER);

    if (cid >= pState->cContexts)
    {
        /* Grow the array. */
        uint32_t cNew = RT_ALIGN(cid + 15, 16);
        void *pvNew = RTMemRealloc(pState->papContexts, sizeof(pState->papContexts[0]) * cNew);
        AssertReturn(pvNew, VERR_NO_MEMORY);
        pState->papContexts = (PVMSVGA3DCONTEXT *)pvNew;
        while (pState->cContexts < cNew)
        {
            pContext = (PVMSVGA3DCONTEXT)RTMemAllocZ(sizeof(*pContext));
            AssertReturn(pContext, VERR_NO_MEMORY);
            pContext->id = SVGA3D_INVALID_ID;
            pState->papContexts[pState->cContexts++] = pContext;
        }
    }
    /* If one already exists with this id, then destroy it now. */
    if (pState->papContexts[cid]->id != SVGA3D_INVALID_ID)
        vmsvga3dContextDestroy(pThis, cid);

    pContext = pState->papContexts[cid];
    memset(pContext, 0, sizeof(*pContext));
    pContext->id               = cid;
    for (uint32_t i = 0; i< RT_ELEMENTS(pContext->aSidActiveTextures); i++)
        pContext->aSidActiveTextures[i] = SVGA3D_INVALID_ID;
    pContext->sidRenderTarget  = SVGA3D_INVALID_ID;
    pContext->state.shidVertex = SVGA3D_INVALID_ID;
    pContext->state.shidPixel  = SVGA3D_INVALID_ID;

    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->state.aRenderTargets); i++)
        pContext->state.aRenderTargets[i] = SVGA3D_INVALID_ID;

    /* Create a context window. */
    CREATESTRUCT cs;

    AssertReturn(pThis->svga.u64HostWindowId, VERR_INTERNAL_ERROR);

    cs.lpCreateParams   = NULL;
    cs.dwExStyle        = WS_EX_NOACTIVATE | WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT;
#ifdef DEBUG_GFX_WINDOW
    cs.lpszName         = (char *)RTMemAllocZ(256);
    RTStrPrintf((char *)cs.lpszName, 256, "Context %d OpenGL Window", cid);
#else
    cs.lpszName         = NULL;
#endif
    cs.lpszClass        = NULL;
#ifdef DEBUG_GFX_WINDOW
    cs.style            = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_CAPTION;
#else
    cs.style            = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_DISABLED | WS_CHILD | WS_VISIBLE;
#endif
    cs.x                = 0;
    cs.y                = 0;
    cs.cx               = pThis->svga.uWidth;
    cs.cy               = pThis->svga.uHeight;
    cs.hwndParent       = (HWND)pThis->svga.u64HostWindowId;
    cs.hMenu            = NULL;
    cs.hInstance        = pState->hInstance;

    rc = vmsvga3dSendThreadMessage(pState->pWindowThread, pState->WndRequestSem, WM_VMSVGA3D_CREATEWINDOW, (WPARAM)&pContext->hwnd, (LPARAM)&cs);
    AssertRCReturn(rc, rc);

    /* Changed when the function returns. */
    PresParam.BackBufferWidth               = 0;
    PresParam.BackBufferHeight              = 0;
    PresParam.BackBufferFormat              = D3DFMT_UNKNOWN;
    PresParam.BackBufferCount               = 0;

    PresParam.MultiSampleType               = D3DMULTISAMPLE_NONE;
    PresParam.MultiSampleQuality            = 0;
    PresParam.SwapEffect                    = D3DSWAPEFFECT_FLIP;
    PresParam.hDeviceWindow                 = pContext->hwnd;
    PresParam.Windowed                      = TRUE;     /** @todo */
    PresParam.EnableAutoDepthStencil        = FALSE;
    PresParam.AutoDepthStencilFormat        = D3DFMT_UNKNOWN;   /* not relevant */
    PresParam.Flags                         = 0;
    PresParam.FullScreen_RefreshRateInHz    = 0;        /* windowed -> 0 */
    /** @todo consider using D3DPRESENT_DONOTWAIT so we don't wait for the GPU during Present calls. */
    PresParam.PresentationInterval          = D3DPRESENT_INTERVAL_IMMEDIATE;

#ifdef VBOX_VMSVGA3D_WITH_WINE_OPENGL
    hr = pState->pD3D9->CreateDevice(D3DADAPTER_DEFAULT,
                                     D3DDEVTYPE_HAL,
                                     pContext->hwnd,
                                     D3DCREATE_MULTITHREADED | D3DCREATE_MIXED_VERTEXPROCESSING, //D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                     &PresParam,
                                     &pContext->pDevice);
#else
    hr = pState->pD3D9->CreateDeviceEx(D3DADAPTER_DEFAULT,
                                       D3DDEVTYPE_HAL,
                                       pContext->hwnd,
                                       D3DCREATE_MULTITHREADED | D3DCREATE_MIXED_VERTEXPROCESSING, //D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                       &PresParam,
                                       NULL,
                                       &pContext->pDevice);
#endif
    AssertMsgReturn(hr == D3D_OK, ("vmsvga3dContextDefine: CreateDevice failed with %x\n", hr), VERR_INTERNAL_ERROR);

    Log(("vmsvga3dContextDefine: Backbuffer (%d,%d) count=%d format=%x\n", PresParam.BackBufferWidth, PresParam.BackBufferHeight, PresParam.BackBufferCount, PresParam.BackBufferFormat));
    return VINF_SUCCESS;
}

/**
 * Destroy an existing 3d context
 *
 * @returns VBox status code.
 * @param   pThis           VGA device instance data.
 * @param   cid             Context id
 */
int vmsvga3dContextDestroy(PVGASTATE pThis, uint32_t cid)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    AssertReturn(cid < SVGA3D_MAX_CONTEXT_IDS, VERR_INVALID_PARAMETER);

    if (    cid < pState->cContexts
        &&  pState->papContexts[cid]->id == cid)
    {
        PVMSVGA3DCONTEXT pContext = pState->papContexts[cid];

        Log(("vmsvga3dContextDestroy id %x\n", cid));

        /* Check for all surfaces that are associated with this context to remove all dependencies */
        for (uint32_t sid = 0; sid < pState->cSurfaces; sid++)
        {
            PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];
            if (    pSurface->id == sid
                &&  pSurface->idAssociatedContext == cid)
            {
                int rc;

                LogFunc(("Remove all dependencies for surface sid=%x\n", sid));

                uint32_t            surfaceFlags = pSurface->surfaceFlags;
                SVGA3dSurfaceFormat format = pSurface->format;
                SVGA3dSurfaceFace   face[SVGA3D_MAX_SURFACE_FACES];
                uint32_t            multisampleCount = pSurface->multiSampleCount;
                SVGA3dTextureFilter autogenFilter = pSurface->autogenFilter;
                SVGA3dSize         *pMipLevelSize;
                uint32_t            cFaces = pSurface->cFaces;

                pMipLevelSize = (SVGA3dSize *)RTMemAllocZ(pSurface->faces[0].numMipLevels * pSurface->cFaces * sizeof(SVGA3dSize));
                AssertReturn(pMipLevelSize, VERR_NO_MEMORY);

                for (uint32_t face=0; face < pSurface->cFaces; face++)
                {
                    for (uint32_t i = 0; i < pSurface->faces[0].numMipLevels; i++)
                    {
                        uint32_t idx = i + face * pSurface->faces[0].numMipLevels;
                        memcpy(&pMipLevelSize[idx], &pSurface->pMipmapLevels[idx].mipmapSize, sizeof(SVGA3dSize));
                    }
                }
                memcpy(face, pSurface->faces, sizeof(pSurface->faces));

                /* Recreate the surface with the original settings; destroys the contents, but that seems fairly safe since the context is also destroyed. */
                /** @todo not safe with shared objects */
                Assert(pSurface->pSharedObjectTree == NULL);

                rc = vmsvga3dSurfaceDestroy(pThis, sid);
                AssertRC(rc);

                rc = vmsvga3dSurfaceDefine(pThis, sid, surfaceFlags, format, face, multisampleCount, autogenFilter, face[0].numMipLevels * cFaces, pMipLevelSize);
                AssertRC(rc);

                Assert(!pSurface->u.pSurface);
            }
            else
            {
                /* Check for a shared surface object. */
                PVMSVGA3DSHAREDSURFACE pSharedSurface = (PVMSVGA3DSHAREDSURFACE)RTAvlU32Get(&pSurface->pSharedObjectTree, cid);
                if (pSharedSurface)
                {
                    LogFunc(("Remove shared dependency for surface sid=%x\n", sid));

                    switch (pSurface->enmD3DResType)
                    {
                    case VMSVGA3D_D3DRESTYPE_TEXTURE:
                        Assert(pSharedSurface->u.pTexture);
                        D3D_RELEASE(pSharedSurface->u.pTexture);
                        break;

                    case VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE:
                        Assert(pSharedSurface->u.pCubeTexture);
                        D3D_RELEASE(pSharedSurface->u.pCubeTexture);
                        break;

                    case VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE:
                        Assert(pSharedSurface->u.pVolumeTexture);
                        D3D_RELEASE(pSharedSurface->u.pVolumeTexture);
                        break;

                    default:
                        AssertFailed();
                        break;
                    }
                    RTAvlU32Remove(&pSurface->pSharedObjectTree, cid);
                    RTMemFree(pSharedSurface);
                }
            }
        }

        /* Destroy all leftover pixel shaders. */
        for (uint32_t i = 0; i < pContext->cPixelShaders; i++)
        {
            if (pContext->paPixelShader[i].id != SVGA3D_INVALID_ID)
                vmsvga3dShaderDestroy(pThis, pContext->paPixelShader[i].cid, pContext->paPixelShader[i].id, pContext->paPixelShader[i].type);
        }
        if (pContext->paPixelShader)
            RTMemFree(pContext->paPixelShader);

        /* Destroy all leftover vertex shaders. */
        for (uint32_t i = 0; i < pContext->cVertexShaders; i++)
        {
            if (pContext->paVertexShader[i].id != SVGA3D_INVALID_ID)
                vmsvga3dShaderDestroy(pThis, pContext->paVertexShader[i].cid, pContext->paVertexShader[i].id, pContext->paVertexShader[i].type);
        }
        if (pContext->paVertexShader)
            RTMemFree(pContext->paVertexShader);

        if (pContext->state.paVertexShaderConst)
            RTMemFree(pContext->state.paVertexShaderConst);
        if (pContext->state.paPixelShaderConst)
            RTMemFree(pContext->state.paPixelShaderConst);

        vmsvga3dOcclusionQueryDelete(pState, pContext);

        /* Release the D3D device object */
        D3D_RELEASE(pContext->pDevice);

        /* Destroy the window we've created. */
        int rc = vmsvga3dSendThreadMessage(pState->pWindowThread, pState->WndRequestSem, WM_VMSVGA3D_DESTROYWINDOW, (WPARAM)pContext->hwnd, 0);
        AssertRC(rc);

        memset(pContext, 0, sizeof(*pContext));
        pContext->id = SVGA3D_INVALID_ID;
    }
    else
        AssertFailed();

    return VINF_SUCCESS;
}

static int vmsvga3dContextTrackUsage(PVGASTATE pThis, PVMSVGA3DCONTEXT pContext)
{
#ifndef VBOX_VMSVGA3D_WITH_WINE_OPENGL
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    /* Inject fences to make sure we can track surface usage in case the client wants to reuse it in another context. */
    for (uint32_t i = 0; i < RT_ELEMENTS(pContext->aSidActiveTextures); ++i)
    {
        if (pContext->aSidActiveTextures[i] != SVGA3D_INVALID_ID)
            vmsvga3dSurfaceTrackUsageById(pState, pContext, pContext->aSidActiveTextures[i]);
    }
    if (pContext->sidRenderTarget != SVGA3D_INVALID_ID)
        vmsvga3dSurfaceTrackUsageById(pState, pContext, pContext->sidRenderTarget);
#endif
    return VINF_SUCCESS;
}

/* Handle resize */
int vmsvga3dChangeMode(PVGASTATE pThis)
{
    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    /* Resize all active contexts. */
    for (uint32_t i = 0; i < pState->cContexts; i++)
    {
        PVMSVGA3DCONTEXT pContext = pState->papContexts[i];
        uint32_t cid = pContext->id;

        if (cid != SVGA3D_INVALID_ID)
        {
            CREATESTRUCT          cs;
            D3DPRESENT_PARAMETERS PresParam;
            D3DVIEWPORT9          viewportOrg;
            HRESULT               hr;

#ifdef VMSVGA3D_DIRECT3D9_RESET
            /* Sync back all surface data as everything is lost after the Reset. */
            for (uint32_t sid = 0; sid < pState->cSurfaces; sid++)
            {
                PVMSVGA3DSURFACE pSurface = pState->papSurfaces[sid];
                if (    pSurface->id == sid
                    &&  pSurface->idAssociatedContext == cid
                    &&  pSurface->u.pSurface)
                {
                    Log(("vmsvga3dChangeMode: sync back data of surface sid=%x (fDirty=%d)\n", sid, pSurface->fDirty));

                    /* Reallocate our surface memory buffers. */
                    for (uint32_t i = 0; i < pSurface->cMipLevels; i++)
                    {
                        PVMSVGA3DMIPMAPLEVEL pMipmapLevel = &pSurface->pMipmapLevels[i];

                        pMipmapLevel->pSurfaceData   = RTMemAllocZ(pMipmapLevel->cbSurface);
                        AssertReturn(pMipmapLevel->pSurfaceData, VERR_NO_MEMORY);

                        if (!pSurface->fDirty)
                        {
                            D3DLOCKED_RECT LockedRect;

                            if (pSurface->bounce.pTexture)
                            {
                                IDirect3DSurface9 *pSrc, *pDest;

                                /** @todo only sync when something was actually rendered (since the last sync) */
                                Log(("vmsvga3dChangeMode: sync bounce buffer (level %d)\n", i));
                                hr = pSurface->bounce.pTexture->GetSurfaceLevel(i, &pDest);
                                AssertMsgReturn(hr == D3D_OK, ("vmsvga3dChangeMode: GetSurfaceLevel failed with %x\n", hr), VERR_INTERNAL_ERROR);

                                hr = pSurface->u.pTexture->GetSurfaceLevel(i, &pSrc);
                                AssertMsgReturn(hr == D3D_OK, ("vmsvga3dChangeMode: GetSurfaceLevel failed with %x\n", hr), VERR_INTERNAL_ERROR);

                                hr = pContext->pDevice->GetRenderTargetData(pSrc, pDest);
                                AssertMsgReturn(hr == D3D_OK, ("vmsvga3dChangeMode: GetRenderTargetData failed with %x\n", hr), VERR_INTERNAL_ERROR);

                                D3D_RELEASE(pSrc);
                                D3D_RELEASE(pDest);

                                hr = pSurface->bounce.pTexture->LockRect(i,
                                                                         &LockedRect,
                                                                         NULL,
                                                                         D3DLOCK_READONLY);
                            }
                            else
                                hr = pSurface->u.pTexture->LockRect(i,
                                                                    &LockedRect,
                                                                    NULL,
                                                                    D3DLOCK_READONLY);
                            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dChangeMode: LockRect failed with %x\n", hr), VERR_INTERNAL_ERROR);

                            /* Copy the data one line at a time in case the internal pitch is different. */
                            for (uint32_t j = 0; j < pMipmapLevel->size.height; j++)
                            {
                                memcpy((uint8_t *)pMipmapLevel->pSurfaceData + j * pMipmapLevel->cbSurfacePitch, (uint8_t *)LockedRect.pBits + j * LockedRect.Pitch, pMipmapLevel->cbSurfacePitch);
                            }

                            if (pSurface->bounce.pTexture)
                                hr = pSurface->bounce.pTexture->UnlockRect(i);
                            else
                                hr = pSurface->u.pTexture->UnlockRect(i);
                            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dChangeMode: UnlockRect failed with %x\n", hr), VERR_INTERNAL_ERROR);
                        }
                    }


                    switch (pSurface->flags & VMSVGA3D_SURFACE_HINT_SWITCH_MASK)
                    {
                    case SVGA3D_SURFACE_CUBEMAP:
                    case SVGA3D_SURFACE_CUBEMAP | SVGA3D_SURFACE_HINT_TEXTURE:
                    case SVGA3D_SURFACE_CUBEMAP | SVGA3D_SURFACE_HINT_TEXTURE | SVGA3D_SURFACE_HINT_RENDERTARGET:
                        D3D_RELEASE(pSurface->u.pCubeTexture);
                        D3D_RELEASE(pSurface->bounce.pCubeTexture);
                        break;

                    case SVGA3D_SURFACE_HINT_INDEXBUFFER | SVGA3D_SURFACE_HINT_VERTEXBUFFER:
                    case SVGA3D_SURFACE_HINT_INDEXBUFFER:
                    case SVGA3D_SURFACE_HINT_VERTEXBUFFER:
                        if (pSurface->fu32ActualUsageFlags == SVGA3D_SURFACE_HINT_VERTEXBUFFER)
                            D3D_RELEASE(pSurface->u.pVertexBuffer);
                        else if (pSurface->fu32ActualUsageFlags == SVGA3D_SURFACE_HINT_INDEXBUFFER)
                            D3D_RELEASE(pSurface->u.pIndexBuffer);
                        else
                            AssertMsg(pSurface->u.pVertexBuffer == NULL, ("fu32ActualUsageFlags %x\n", pSurface->fu32ActualUsageFlags));
                        break;

                    case SVGA3D_SURFACE_HINT_TEXTURE:
                    case SVGA3D_SURFACE_HINT_TEXTURE | SVGA3D_SURFACE_HINT_RENDERTARGET:
                        D3D_RELEASE(pSurface->u.pTexture);
                        D3D_RELEASE(pSurface->bounce.pTexture);
                        break;

                    case SVGA3D_SURFACE_HINT_RENDERTARGET:
                    case SVGA3D_SURFACE_HINT_DEPTHSTENCIL:
                        if (pSurface->fStencilAsTexture)
                            D3D_RELEASE(pSurface->u.pTexture);
                        else
                            D3D_RELEASE(pSurface->u.pSurface);
                        break;

                    default:
                        AssertFailed();
                        break;
                    }
                    RTAvlU32Destroy(&pSurface->pSharedObjectTree, vmsvga3dSharedSurfaceDestroyTree, pSurface);
                    Assert(pSurface->pSharedObjectTree == NULL);

                    pSurface->idAssociatedContext = SVGA3D_INVALID_ID;
                    pSurface->hSharedObject       = 0;
                }
            }
#endif /* #ifdef VMSVGA3D_DIRECT3D9_RESET */
            memset(&cs, 0, sizeof(cs));
            cs.cx = pThis->svga.uWidth;
            cs.cy = pThis->svga.uHeight;

            Log(("vmsvga3dChangeMode: Resize window %x of context %d to (%d,%d)\n", pContext->hwnd, pContext->id, cs.cx, cs.cy));

            AssertReturn(pContext->pDevice, VERR_INTERNAL_ERROR);
            hr = pContext->pDevice->GetViewport(&viewportOrg);
            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dChangeMode: GetViewport failed with %x\n", hr), VERR_INTERNAL_ERROR);

            Log(("vmsvga3dChangeMode: old viewport settings (%d,%d)(%d,%d) z=%d/%d\n", viewportOrg.X, viewportOrg.Y, viewportOrg.Width, viewportOrg.Height, (uint32_t)(viewportOrg.MinZ * 100.0), (uint32_t)(viewportOrg.MaxZ * 100.0)));

            /* Resize the window. */
            int rc = vmsvga3dSendThreadMessage(pState->pWindowThread, pState->WndRequestSem, WM_VMSVGA3D_RESIZEWINDOW, (WPARAM)pContext->hwnd, (LPARAM)&cs);
            AssertRC(rc);

            /* Changed when the function returns. */
            PresParam.BackBufferWidth               = 0;
            PresParam.BackBufferHeight              = 0;
            PresParam.BackBufferFormat              = D3DFMT_UNKNOWN;
            PresParam.BackBufferCount               = 0;

            PresParam.MultiSampleType               = D3DMULTISAMPLE_NONE;
            PresParam.MultiSampleQuality            = 0;
            PresParam.SwapEffect                    = D3DSWAPEFFECT_FLIP;
            PresParam.hDeviceWindow                 = pContext->hwnd;
            PresParam.Windowed                      = TRUE;     /** @todo */
            PresParam.EnableAutoDepthStencil        = FALSE;
            PresParam.AutoDepthStencilFormat        = D3DFMT_UNKNOWN;   /* not relevant */
            PresParam.Flags                         = 0;
            PresParam.FullScreen_RefreshRateInHz    = 0;        /* windowed -> 0 */
            /** @todo consider using D3DPRESENT_DONOTWAIT so we don't wait for the GPU during Present calls. */
            PresParam.PresentationInterval          = D3DPRESENT_INTERVAL_IMMEDIATE;;

#ifdef VBOX_VMSVGA3D_WITH_WINE_OPENGL
            hr = pContext->pDevice->Reset(&PresParam);
            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dChangeMode: Reset failed with %x\n", hr), VERR_INTERNAL_ERROR);
#else
            /* ResetEx does not trash the device state */
            hr = pContext->pDevice->ResetEx(&PresParam, NULL);
            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dChangeMode: Reset failed with %x\n", hr), VERR_INTERNAL_ERROR);
#endif
            Log(("vmsvga3dChangeMode: Backbuffer (%d,%d) count=%d format=%x\n", PresParam.BackBufferWidth, PresParam.BackBufferHeight, PresParam.BackBufferCount, PresParam.BackBufferFormat));

            /* ResetEx changes the viewport; restore it again. */
            hr = pContext->pDevice->SetViewport(&viewportOrg);
            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dChangeMode: SetViewport failed with %x\n", hr), VERR_INTERNAL_ERROR);

#ifdef LOG_ENABLED
            {
                D3DVIEWPORT9          viewport;
                hr = pContext->pDevice->GetViewport(&viewport);
                AssertMsgReturn(hr == D3D_OK, ("vmsvga3dChangeMode: GetViewport failed with %x\n", hr), VERR_INTERNAL_ERROR);

                Log(("vmsvga3dChangeMode: changed viewport settings (%d,%d)(%d,%d) z=%d/%d\n", viewport.X, viewport.Y, viewport.Width, viewport.Height, (uint32_t)(viewport.MinZ * 100.0), (uint32_t)(viewport.MaxZ * 100.0)));
            }
#endif

            /* First set the render targets as they change the internal state (reset viewport etc) */
            Log(("vmsvga3dChangeMode: Recreate render targets BEGIN\n"));
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

#ifdef VMSVGA3D_DIRECT3D9_RESET
            /* Recreate the render state */
            Log(("vmsvga3dChangeMode: Recreate render state BEGIN\n"));
            for (uint32_t i = 0; i < RT_ELEMENTS(pContext->state.aRenderState); i++)
            {
                SVGA3dRenderState *pRenderState = &pContext->state.aRenderState[i];

                if (pRenderState->state != SVGA3D_RS_INVALID)
                    vmsvga3dSetRenderState(pThis, pContext->id, 1, pRenderState);
            }
            Log(("vmsvga3dChangeMode: Recreate render state END\n"));

            /* Recreate the texture state */
            Log(("vmsvga3dChangeMode: Recreate texture state BEGIN\n"));
            for (uint32_t iStage = 0; iStage < RT_ELEMENTS(pContext->state.aTextureStates); iStage++)
            {
                for (uint32_t j = 0; j < RT_ELEMENTS(pContext->state.aTextureStates[0]); j++)
                {
                    SVGA3dTextureState *pTextureState = &pContext->state.aTextureStates[iStage][j];

                    if (pTextureState->name != SVGA3D_RS_INVALID)
                        vmsvga3dSetTextureState(pThis, pContext->id, 1, pTextureState);
                }
            }
            Log(("vmsvga3dChangeMode: Recreate texture state END\n"));

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
            /** @todo restore more state data */
#endif /* #ifdef VMSVGA3D_DIRECT3D9_RESET */
        }
    }
    return VINF_SUCCESS;
}


int vmsvga3dSetTransform(PVGASTATE pThis, uint32_t cid, SVGA3dTransformType type, float matrix[16])
{
    D3DTRANSFORMSTATETYPE d3dState;
    HRESULT               hr;
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetTransform %x %s\n", cid, vmsvgaTransformToString(type)));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetTransform invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];

    switch (type)
    {
    case SVGA3D_TRANSFORM_VIEW:
        d3dState = D3DTS_VIEW;
        break;
    case SVGA3D_TRANSFORM_PROJECTION:
        d3dState = D3DTS_PROJECTION;
        break;
    case SVGA3D_TRANSFORM_TEXTURE0:
        d3dState = D3DTS_TEXTURE0;
        break;
    case SVGA3D_TRANSFORM_TEXTURE1:
        d3dState = D3DTS_TEXTURE1;
        break;
    case SVGA3D_TRANSFORM_TEXTURE2:
        d3dState = D3DTS_TEXTURE2;
        break;
    case SVGA3D_TRANSFORM_TEXTURE3:
        d3dState = D3DTS_TEXTURE3;
        break;
    case SVGA3D_TRANSFORM_TEXTURE4:
        d3dState = D3DTS_TEXTURE4;
        break;
    case SVGA3D_TRANSFORM_TEXTURE5:
        d3dState = D3DTS_TEXTURE5;
        break;
    case SVGA3D_TRANSFORM_TEXTURE6:
        d3dState = D3DTS_TEXTURE6;
        break;
    case SVGA3D_TRANSFORM_TEXTURE7:
        d3dState = D3DTS_TEXTURE7;
        break;
    case SVGA3D_TRANSFORM_WORLD:
        d3dState = D3DTS_WORLD;
        break;
    case SVGA3D_TRANSFORM_WORLD1:
        d3dState = D3DTS_WORLD1;
        break;
    case SVGA3D_TRANSFORM_WORLD2:
        d3dState = D3DTS_WORLD2;
        break;
    case SVGA3D_TRANSFORM_WORLD3:
        d3dState = D3DTS_WORLD3;
        break;

    default:
        Log(("vmsvga3dSetTransform: unknown type!!\n"));
        return VERR_INVALID_PARAMETER;
    }

    /* Save this matrix for vm state save/restore. */
    pContext->state.aTransformState[type].fValid = true;
    memcpy(pContext->state.aTransformState[type].matrix, matrix, sizeof(pContext->state.aTransformState[type].matrix));
    pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_TRANSFORM;

    Log(("Matrix [%d %d %d %d]\n", (int)(matrix[0] * 10.0), (int)(matrix[1] * 10.0), (int)(matrix[2] * 10.0), (int)(matrix[3] * 10.0)));
    Log(("       [%d %d %d %d]\n", (int)(matrix[4] * 10.0), (int)(matrix[5] * 10.0), (int)(matrix[6] * 10.0), (int)(matrix[7] * 10.0)));
    Log(("       [%d %d %d %d]\n", (int)(matrix[8] * 10.0), (int)(matrix[9] * 10.0), (int)(matrix[10] * 10.0), (int)(matrix[11] * 10.0)));
    Log(("       [%d %d %d %d]\n", (int)(matrix[12] * 10.0), (int)(matrix[13] * 10.0), (int)(matrix[14] * 10.0), (int)(matrix[15] * 10.0)));
    hr = pContext->pDevice->SetTransform(d3dState, (const D3DMATRIX *)matrix);
    AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSetTransform: SetTransform failed with %x\n", hr), VERR_INTERNAL_ERROR);
    return VINF_SUCCESS;
}

int vmsvga3dSetZRange(PVGASTATE pThis, uint32_t cid, SVGA3dZRange zRange)
{
    D3DVIEWPORT9          viewport;
    HRESULT               hr;
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetZRange %x min=%d max=%d\n", cid, (uint32_t)(zRange.min * 100.0), (uint32_t)(zRange.max * 100.0)));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetZRange invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];
    pContext->state.zRange = zRange;
    pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_ZRANGE;

    hr = pContext->pDevice->GetViewport(&viewport);
    AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSetZRange: GetViewport failed with %x\n", hr), VERR_INTERNAL_ERROR);

    Log(("vmsvga3dSetZRange: old viewport settings (%d,%d)(%d,%d) z=%d/%d\n", viewport.X, viewport.Y, viewport.Width, viewport.Height, (uint32_t)(viewport.MinZ * 100.0), (uint32_t)(viewport.MaxZ * 100.0)));
    /** @todo convert the depth range from -1-1 to 0-1 although we shouldn't be getting such values in the first place... */
    if (zRange.min < 0.0)
        zRange.min = 0.0;
    if (zRange.max > 1.0)
        zRange.max = 1.0;

    viewport.MinZ = zRange.min;
    viewport.MaxZ = zRange.max;
    hr = pContext->pDevice->SetViewport(&viewport);
    AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSetZRange: SetViewport failed with %x\n", hr), VERR_INTERNAL_ERROR);
    return VINF_SUCCESS;
}

/**
 * Convert SVGA blend op value to its D3D equivalent
 */
static DWORD vmsvga3dBlendOp2D3D(uint32_t blendOp, DWORD defaultBlendOp)
{
    switch (blendOp)
    {
    case SVGA3D_BLENDOP_ZERO:
        return D3DBLEND_ZERO;
    case SVGA3D_BLENDOP_ONE:
        return D3DBLEND_ONE;
    case SVGA3D_BLENDOP_SRCCOLOR:
        return D3DBLEND_SRCCOLOR;
    case SVGA3D_BLENDOP_INVSRCCOLOR:
        return D3DBLEND_INVSRCCOLOR;
    case SVGA3D_BLENDOP_SRCALPHA:
        return D3DBLEND_SRCALPHA;
    case SVGA3D_BLENDOP_INVSRCALPHA:
        return D3DBLEND_INVSRCALPHA;
    case SVGA3D_BLENDOP_DESTALPHA:
        return D3DBLEND_DESTALPHA;
    case SVGA3D_BLENDOP_INVDESTALPHA:
        return D3DBLEND_INVDESTALPHA;
    case SVGA3D_BLENDOP_DESTCOLOR:
        return D3DBLEND_DESTCOLOR;
    case SVGA3D_BLENDOP_INVDESTCOLOR:
        return D3DBLEND_INVDESTCOLOR;
    case SVGA3D_BLENDOP_SRCALPHASAT:
        return D3DBLEND_SRCALPHASAT;
    case SVGA3D_BLENDOP_BLENDFACTOR:
        return D3DBLEND_BLENDFACTOR;
    case SVGA3D_BLENDOP_INVBLENDFACTOR:
        return D3DBLEND_INVBLENDFACTOR;
    default:
        AssertFailed();
        return defaultBlendOp;
    }
}

int vmsvga3dSetRenderState(PVGASTATE pThis, uint32_t cid, uint32_t cRenderStates, SVGA3dRenderState *pRenderState)
{
    DWORD                       val = 0; /* Shut up MSC */
    HRESULT                     hr;
    PVMSVGA3DCONTEXT            pContext;
    PVMSVGA3DSTATE              pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetRenderState cid=%x cRenderStates=%d\n", cid, cRenderStates));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetRenderState invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];

    for (unsigned i = 0; i < cRenderStates; i++)
    {
        D3DRENDERSTATETYPE renderState = D3DRS_FORCE_DWORD;

        Log(("vmsvga3dSetRenderState: state=%s (%d) val=%x\n", vmsvga3dGetRenderStateName(pRenderState[i].state), pRenderState[i].state, pRenderState[i].uintValue));
        /* Save the render state for vm state saving. */
        if (pRenderState[i].state < SVGA3D_RS_MAX)
            pContext->state.aRenderState[pRenderState[i].state] = pRenderState[i];

        switch (pRenderState[i].state)
        {
        case SVGA3D_RS_ZENABLE:                /* SVGA3dBool */
            renderState = D3DRS_ZENABLE;
            val = pRenderState[i].uintValue;
            Assert(val == D3DZB_FALSE || val == D3DZB_TRUE);
            break;

        case SVGA3D_RS_ZWRITEENABLE:           /* SVGA3dBool */
            renderState = D3DRS_ZWRITEENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_ALPHATESTENABLE:        /* SVGA3dBool */
            renderState = D3DRS_ALPHATESTENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_DITHERENABLE:           /* SVGA3dBool */
            renderState = D3DRS_DITHERENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_BLENDENABLE:            /* SVGA3dBool */
            renderState = D3DRS_ALPHABLENDENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_FOGENABLE:              /* SVGA3dBool */
            renderState = D3DRS_FOGENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_SPECULARENABLE:         /* SVGA3dBool */
            renderState = D3DRS_SPECULARENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_LIGHTINGENABLE:         /* SVGA3dBool */
            renderState = D3DRS_LIGHTING;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_NORMALIZENORMALS:       /* SVGA3dBool */
            renderState = D3DRS_NORMALIZENORMALS;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_POINTSPRITEENABLE:      /* SVGA3dBool */
            renderState = D3DRS_POINTSPRITEENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_POINTSCALEENABLE:       /* SVGA3dBool */
            renderState = D3DRS_POINTSCALEENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_POINTSIZE:              /* float */
            renderState = D3DRS_POINTSIZE;
            val = pRenderState[i].uintValue;
            Log(("SVGA3D_RS_POINTSIZE: %d\n", (uint32_t) (pRenderState[i].floatValue * 100.0)));
            break;

        case SVGA3D_RS_POINTSIZEMIN:           /* float */
            renderState = D3DRS_POINTSIZE_MIN;
            val = pRenderState[i].uintValue;
            Log(("SVGA3D_RS_POINTSIZEMIN: %d\n", (uint32_t) (pRenderState[i].floatValue * 100.0)));
            break;

        case SVGA3D_RS_POINTSIZEMAX:           /* float */
            renderState = D3DRS_POINTSIZE_MAX;
            val = pRenderState[i].uintValue;
            Log(("SVGA3D_RS_POINTSIZEMAX: %d\n", (uint32_t) (pRenderState[i].floatValue * 100.0)));
            break;

        case SVGA3D_RS_POINTSCALE_A:           /* float */
            renderState = D3DRS_POINTSCALE_A;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_POINTSCALE_B:           /* float */
            renderState = D3DRS_POINTSCALE_B;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_POINTSCALE_C:           /* float */
            renderState = D3DRS_POINTSCALE_C;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_AMBIENT:                /* SVGA3dColor - identical */
            renderState = D3DRS_AMBIENT;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_CLIPPLANEENABLE:        /* SVGA3dClipPlanes - identical */
            renderState = D3DRS_CLIPPLANEENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_FOGCOLOR:               /* SVGA3dColor - identical */
            renderState = D3DRS_FOGCOLOR;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_FOGSTART:               /* float */
            renderState = D3DRS_FOGSTART;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_FOGEND:                 /* float */
            renderState = D3DRS_FOGEND;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_FOGDENSITY:             /* float */
            renderState = D3DRS_FOGDENSITY;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_RANGEFOGENABLE:         /* SVGA3dBool */
            renderState = D3DRS_RANGEFOGENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_FOGMODE:                /* SVGA3dFogMode */
        {
            SVGA3dFogMode mode;
            mode.uintValue = pRenderState[i].uintValue;

            switch (mode.s.function)
            {
            case SVGA3D_FOGFUNC_INVALID:
                val = D3DFOG_NONE;
                break;
            case SVGA3D_FOGFUNC_EXP:
                val = D3DFOG_EXP;
                break;
            case SVGA3D_FOGFUNC_EXP2:
                val = D3DFOG_EXP2;
                break;
            case SVGA3D_FOGFUNC_LINEAR:
                val = D3DFOG_LINEAR;
                break;
            case SVGA3D_FOGFUNC_PER_VERTEX: /* unable to find a d3d9 equivalent */
                AssertMsgFailedReturn(("Unsupported fog function SVGA3D_FOGFUNC_PER_VERTEX\n"), VERR_INTERNAL_ERROR);
                break;
            default:
                AssertMsgFailedReturn(("Unexpected fog function %d\n", mode.s.function), VERR_INTERNAL_ERROR);
                break;
            }

            /* The fog type determines the render state. */
            switch (mode.s.type)
            {
            case SVGA3D_FOGTYPE_VERTEX:
                renderState = D3DRS_FOGVERTEXMODE;
                break;
            case SVGA3D_FOGTYPE_PIXEL:
                renderState = D3DRS_FOGTABLEMODE;
                break;
            default:
                AssertMsgFailedReturn(("Unexpected fog type %d\n", mode.s.type), VERR_INTERNAL_ERROR);
                break;
            }

            /* Set the fog base to depth or range. */
            switch (mode.s.base)
            {
            case SVGA3D_FOGBASE_DEPTHBASED:
                hr = pContext->pDevice->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE);
                AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSetRenderState: SetRenderState SVGA3D_FOGBASE_DEPTHBASED failed with %x\n", hr), VERR_INTERNAL_ERROR);
                break;
            case SVGA3D_FOGBASE_RANGEBASED:
                hr = pContext->pDevice->SetRenderState(D3DRS_RANGEFOGENABLE, TRUE);
                AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSetRenderState: SetRenderState SVGA3D_FOGBASE_RANGEBASED failed with %x\n", hr), VERR_INTERNAL_ERROR);
                break;
            default:
                /* ignore */
                AssertMsgFailed(("Unexpected fog base %d\n", mode.s.base));
                break;
            }
            break;
        }

        case SVGA3D_RS_FILLMODE:               /* SVGA3dFillMode */
        {
            SVGA3dFillMode mode;

            mode.uintValue = pRenderState[i].uintValue;

            switch (mode.s.mode)
            {
            case SVGA3D_FILLMODE_POINT:
                val = D3DFILL_POINT;
                break;
            case SVGA3D_FILLMODE_LINE:
                val = D3DFILL_WIREFRAME;
                break;
            case SVGA3D_FILLMODE_FILL:
                val = D3DFILL_SOLID;
                break;
            default:
                AssertMsgFailedReturn(("Unexpected fill mode %d\n", mode.s.mode), VERR_INTERNAL_ERROR);
                break;
            }
            /** @todo ignoring face for now. */
            renderState = D3DRS_FILLMODE;
            break;
        }

        case SVGA3D_RS_SHADEMODE:              /* SVGA3dShadeMode */
            renderState = D3DRS_SHADEMODE;
            AssertCompile(D3DSHADE_FLAT == SVGA3D_SHADEMODE_FLAT);
            val = pRenderState[i].uintValue;    /* SVGA3dShadeMode == D3DSHADEMODE */
            break;

        case SVGA3D_RS_LINEPATTERN:            /* SVGA3dLinePattern */
            /* No longer supported by d3d; mesagl comments suggest not all backends support it */
            /** @todo */
            Log(("WARNING: SVGA3D_RS_LINEPATTERN %x not supported!!\n", pRenderState[i].uintValue));
            /*
            renderState = D3DRS_LINEPATTERN;
            val = pRenderState[i].uintValue;
            */
            break;

        case SVGA3D_RS_SRCBLEND:               /* SVGA3dBlendOp */
            renderState = D3DRS_SRCBLEND;
            val = vmsvga3dBlendOp2D3D(pRenderState[i].uintValue, D3DBLEND_ONE /* default */);
            break;

        case SVGA3D_RS_DSTBLEND:               /* SVGA3dBlendOp */
            renderState = D3DRS_DESTBLEND;
            val = vmsvga3dBlendOp2D3D(pRenderState[i].uintValue, D3DBLEND_ZERO /* default */);
            break;

        case SVGA3D_RS_BLENDEQUATION:          /* SVGA3dBlendEquation - identical */
            AssertCompile(SVGA3D_BLENDEQ_MAXIMUM == D3DBLENDOP_MAX);
            renderState = D3DRS_BLENDOP;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_CULLMODE:               /* SVGA3dFace */
        {
            switch (pRenderState[i].uintValue)
            {
            case SVGA3D_FACE_NONE:
                val = D3DCULL_NONE;
                break;
            case SVGA3D_FACE_FRONT:
                val = D3DCULL_CW;
                break;
            case SVGA3D_FACE_BACK:
                val = D3DCULL_CCW;
                break;
            case SVGA3D_FACE_FRONT_BACK:
                AssertFailed();
                val = D3DCULL_CW;
                break;
            default:
                AssertMsgFailedReturn(("Unexpected cull mode %d\n", pRenderState[i].uintValue), VERR_INTERNAL_ERROR);
                break;
            }
            renderState = D3DRS_CULLMODE;
            break;
        }

        case SVGA3D_RS_ZFUNC:                  /* SVGA3dCmpFunc - identical */
            AssertCompile(SVGA3D_CMP_ALWAYS == D3DCMP_ALWAYS);
            renderState = D3DRS_ZFUNC;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_ALPHAFUNC:              /* SVGA3dCmpFunc - identical */
            renderState = D3DRS_ALPHAFUNC;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_STENCILENABLE:          /* SVGA3dBool */
            renderState = D3DRS_STENCILENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_STENCILREF:             /* uint32_t */
            renderState = D3DRS_STENCILREF;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_STENCILMASK:            /* uint32_t */
            renderState = D3DRS_STENCILMASK;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_STENCILWRITEMASK:       /* uint32_t */
            renderState = D3DRS_STENCILWRITEMASK;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_STENCILFUNC:            /* SVGA3dCmpFunc - identical */
            renderState = D3DRS_STENCILFUNC;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_STENCILFAIL:            /* SVGA3dStencilOp - identical */
            AssertCompile(D3DSTENCILOP_KEEP == SVGA3D_STENCILOP_KEEP);
            AssertCompile(D3DSTENCILOP_DECR == SVGA3D_STENCILOP_DECR);
            renderState = D3DRS_STENCILFAIL;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_STENCILZFAIL:           /* SVGA3dStencilOp - identical */
            renderState = D3DRS_STENCILZFAIL;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_STENCILPASS:            /* SVGA3dStencilOp - identical */
            renderState = D3DRS_STENCILPASS;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_ALPHAREF:               /* float (0.0 .. 1.0) */
            renderState = D3DRS_ALPHAREF;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_FRONTWINDING:           /* SVGA3dFrontWinding */
            Assert(pRenderState[i].uintValue == SVGA3D_FRONTWINDING_CW);
            /*
            renderState = D3DRS_FRONTWINDING; //D3DRS_TWOSIDEDSTENCILMODE
            val = pRenderState[i].uintValue;
            */
            break;

        case SVGA3D_RS_COORDINATETYPE:         /* SVGA3dCoordinateType */
            Assert(pRenderState[i].uintValue == SVGA3D_COORDINATE_LEFTHANDED);
            /** @todo setup a view matrix to scale the world space by -1 in the z-direction for right handed coordinates. */
            /*
            renderState = D3DRS_COORDINATETYPE;
            val = pRenderState[i].uintValue;
            */
            break;

        case SVGA3D_RS_ZBIAS:                  /* float */
            /** @todo unknown meaning; depth bias is not identical
            renderState = D3DRS_DEPTHBIAS;
            val = pRenderState[i].uintValue;
            */
            Log(("vmsvga3dSetRenderState: WARNING unsupported SVGA3D_RS_ZBIAS\n"));
            break;

        case SVGA3D_RS_SLOPESCALEDEPTHBIAS:    /* float */
            renderState = D3DRS_SLOPESCALEDEPTHBIAS;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_DEPTHBIAS:              /* float */
            renderState = D3DRS_DEPTHBIAS;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_COLORWRITEENABLE:       /* SVGA3dColorMask - identical to D3DCOLORWRITEENABLE_* */
            renderState = D3DRS_COLORWRITEENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_VERTEXMATERIALENABLE:   /* SVGA3dBool */
            //AssertFailed();
            renderState = D3DRS_INDEXEDVERTEXBLENDENABLE;       /* correct?? */
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_DIFFUSEMATERIALSOURCE:  /* SVGA3dVertexMaterial - identical */
            AssertCompile(D3DMCS_COLOR2 == SVGA3D_VERTEXMATERIAL_SPECULAR);
            renderState = D3DRS_DIFFUSEMATERIALSOURCE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_SPECULARMATERIALSOURCE: /* SVGA3dVertexMaterial - identical */
            renderState = D3DRS_SPECULARMATERIALSOURCE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_AMBIENTMATERIALSOURCE:  /* SVGA3dVertexMaterial - identical */
            renderState = D3DRS_AMBIENTMATERIALSOURCE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_EMISSIVEMATERIALSOURCE: /* SVGA3dVertexMaterial - identical */
            renderState = D3DRS_EMISSIVEMATERIALSOURCE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_TEXTUREFACTOR:          /* SVGA3dColor - identical */
            renderState = D3DRS_TEXTUREFACTOR;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_LOCALVIEWER:            /* SVGA3dBool */
            renderState = D3DRS_LOCALVIEWER;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_SCISSORTESTENABLE:      /* SVGA3dBool */
            renderState = D3DRS_SCISSORTESTENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_BLENDCOLOR:             /* SVGA3dColor - identical */
            renderState = D3DRS_BLENDFACTOR;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_STENCILENABLE2SIDED:    /* SVGA3dBool */
            renderState = D3DRS_TWOSIDEDSTENCILMODE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_CCWSTENCILFUNC:         /* SVGA3dCmpFunc - identical */
            renderState = D3DRS_CCW_STENCILFUNC;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_CCWSTENCILFAIL:         /* SVGA3dStencilOp - identical */
            renderState = D3DRS_CCW_STENCILFAIL;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_CCWSTENCILZFAIL:        /* SVGA3dStencilOp - identical */
            renderState = D3DRS_CCW_STENCILZFAIL;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_CCWSTENCILPASS:         /* SVGA3dStencilOp - identical */
            renderState = D3DRS_CCW_STENCILPASS;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_VERTEXBLEND:            /* SVGA3dVertexBlendFlags - identical */
            AssertCompile(SVGA3D_VBLEND_DISABLE == D3DVBF_DISABLE);
            renderState = D3DRS_VERTEXBLEND;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_OUTPUTGAMMA:            /* float */
            //AssertFailed();
            /*
            D3DRS_SRGBWRITEENABLE ??
            renderState = D3DRS_OUTPUTGAMMA;
            val = pRenderState[i].uintValue;
            */
            break;

        case SVGA3D_RS_ZVISIBLE:               /* SVGA3dBool */
            AssertFailed();
            /*
            renderState = D3DRS_ZVISIBLE;
            val = pRenderState[i].uintValue;
            */
            break;

        case SVGA3D_RS_LASTPIXEL:              /* SVGA3dBool */
            renderState = D3DRS_LASTPIXEL;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_CLIPPING:               /* SVGA3dBool */
            renderState = D3DRS_CLIPPING;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP0:                  /* SVGA3dWrapFlags - identical */
            Assert(SVGA3D_WRAPCOORD_3 == D3DWRAPCOORD_3);
            renderState = D3DRS_WRAP0;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP1:                  /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP1;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP2:                  /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP2;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP3:                  /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP3;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP4:                  /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP4;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP5:                  /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP5;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP6:                  /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP6;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP7:                  /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP7;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP8:                  /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP8;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP9:                  /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP9;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP10:                 /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP10;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP11:                 /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP11;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP12:                 /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP12;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP13:                 /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP13;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP14:                 /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP14;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_WRAP15:                 /* SVGA3dWrapFlags - identical */
            renderState = D3DRS_WRAP15;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_MULTISAMPLEANTIALIAS:   /* SVGA3dBool */
            renderState = D3DRS_MULTISAMPLEANTIALIAS;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_MULTISAMPLEMASK:        /* uint32_t */
            renderState = D3DRS_MULTISAMPLEMASK;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_INDEXEDVERTEXBLENDENABLE: /* SVGA3dBool */
            renderState = D3DRS_INDEXEDVERTEXBLENDENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_TWEENFACTOR:            /* float */
            renderState = D3DRS_TWEENFACTOR;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_ANTIALIASEDLINEENABLE:  /* SVGA3dBool */
            renderState = D3DRS_ANTIALIASEDLINEENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_COLORWRITEENABLE1:      /* SVGA3dColorMask - identical to D3DCOLORWRITEENABLE_* */
            renderState = D3DRS_COLORWRITEENABLE1;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_COLORWRITEENABLE2:      /* SVGA3dColorMask - identical to D3DCOLORWRITEENABLE_* */
            renderState = D3DRS_COLORWRITEENABLE2;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_COLORWRITEENABLE3:      /* SVGA3dColorMask - identical to D3DCOLORWRITEENABLE_* */
            renderState = D3DRS_COLORWRITEENABLE3;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_SEPARATEALPHABLENDENABLE: /* SVGA3dBool */
            renderState = D3DRS_SEPARATEALPHABLENDENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_SRCBLENDALPHA:          /* SVGA3dBlendOp */
            renderState = D3DRS_SRCBLENDALPHA;
            val = vmsvga3dBlendOp2D3D(pRenderState[i].uintValue, D3DBLEND_ONE /* default */);
            break;

        case SVGA3D_RS_DSTBLENDALPHA:          /* SVGA3dBlendOp */
            renderState = D3DRS_DESTBLENDALPHA;
            val = vmsvga3dBlendOp2D3D(pRenderState[i].uintValue, D3DBLEND_ZERO /* default */);
            break;

        case SVGA3D_RS_BLENDEQUATIONALPHA:     /* SVGA3dBlendEquation - identical */
            renderState = D3DRS_BLENDOPALPHA;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_TRANSPARENCYANTIALIAS:  /* SVGA3dTransparencyAntialiasType */
            AssertFailed();
            /*
            renderState = D3DRS_TRANSPARENCYANTIALIAS;
            val = pRenderState[i].uintValue;
            */
            break;

        case SVGA3D_RS_LINEAA:                 /* SVGA3dBool */
            renderState = D3DRS_ANTIALIASEDLINEENABLE;
            val = pRenderState[i].uintValue;
            break;

        case SVGA3D_RS_LINEWIDTH:              /* float */
            AssertFailed();
            /*
            renderState = D3DRS_LINEWIDTH;
            val = pRenderState[i].uintValue;
            */
            break;

        case SVGA3D_RS_MAX:                   /* shut up MSC */
        case SVGA3D_RS_INVALID:
            AssertFailedBreak();
        }

        if (renderState != D3DRS_FORCE_DWORD)
        {
            hr = pContext->pDevice->SetRenderState(renderState, val);
            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSetRenderState: SetRenderState failed with %x\n", hr), VERR_INTERNAL_ERROR);
        }
    }

    return VINF_SUCCESS;
}

int vmsvga3dSetRenderTarget(PVGASTATE pThis, uint32_t cid, SVGA3dRenderTargetType type, SVGA3dSurfaceImageId target)
{
    HRESULT                     hr;
    PVMSVGA3DCONTEXT            pContext;
    PVMSVGA3DSTATE              pState = pThis->svga.p3dState;
    PVMSVGA3DSURFACE            pRenderTarget;

    AssertReturn(pState, VERR_NO_MEMORY);
    AssertReturn(type < SVGA3D_RT_MAX, VERR_INVALID_PARAMETER);

    LogFunc(("cid=%x type=%x sid=%x\n", cid, type, target.sid));

    int rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    /* Save for vm state save/restore. */
    pContext->state.aRenderTargets[type] = target.sid;

    if (target.sid == SVGA3D_INVALID_ID)
    {
        /* Disable render target. */
        switch (type)
        {
        case SVGA3D_RT_DEPTH:
            hr = pContext->pDevice->SetDepthStencilSurface(NULL);
            AssertMsgReturn(hr == D3D_OK, ("SetDepthStencilSurface failed with %x\n", hr), VERR_INTERNAL_ERROR);
            break;

        case SVGA3D_RT_STENCIL:
            /* ignore; correct?? */
            break;

        case SVGA3D_RT_COLOR0:
        case SVGA3D_RT_COLOR1:
        case SVGA3D_RT_COLOR2:
        case SVGA3D_RT_COLOR3:
        case SVGA3D_RT_COLOR4:
        case SVGA3D_RT_COLOR5:
        case SVGA3D_RT_COLOR6:
        case SVGA3D_RT_COLOR7:
            pContext->sidRenderTarget = SVGA3D_INVALID_ID;

            if (pState->fSupportedSurfaceNULL)
            {
                /* Create a dummy render target to satisfy D3D. This path is usually taken only to render
                 * into a depth buffer without wishing to update an actual color render target.
                 */
                IDirect3DSurface9 *pDummyRenderTarget;
                hr = pContext->pDevice->CreateRenderTarget(pThis->svga.uWidth,
                                                           pThis->svga.uHeight,
                                                           FOURCC_NULL,
                                                           D3DMULTISAMPLE_NONE,
                                                           0,
                                                           FALSE,
                                                           &pDummyRenderTarget,
                                                           NULL);

                AssertMsgReturn(hr == D3D_OK, ("CreateRenderTarget failed with %x\n", hr), VERR_INTERNAL_ERROR);

                hr = pContext->pDevice->SetRenderTarget(type - SVGA3D_RT_COLOR0, pDummyRenderTarget);
                D3D_RELEASE(pDummyRenderTarget);
            }
            else
                hr = pContext->pDevice->SetRenderTarget(type - SVGA3D_RT_COLOR0, NULL);

            AssertMsgReturn(hr == D3D_OK, ("SetRenderTarget failed with %x\n", hr), VERR_INTERNAL_ERROR);
            break;

        default:
            AssertFailedReturn(VERR_INVALID_PARAMETER);
        }
        return VINF_SUCCESS;
    }

    rc = vmsvga3dSurfaceFromSid(pState, target.sid, &pRenderTarget);
    AssertRCReturn(rc, rc);

    switch (type)
    {
    case SVGA3D_RT_DEPTH:
    case SVGA3D_RT_STENCIL:
        AssertReturn(target.face == 0 && target.mipmap == 0, VERR_INVALID_PARAMETER);
        if (!pRenderTarget->u.pSurface)
        {
            DWORD cQualityLevels = 0;

            /* Query the nr of quality levels for this particular format */
            if (pRenderTarget->multiSampleTypeD3D != D3DMULTISAMPLE_NONE)
            {
                hr = pState->pD3D9->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,
                                                               D3DDEVTYPE_HAL,
                                                               pRenderTarget->formatD3D,
                                                               TRUE,        /* Windowed */
                                                               pRenderTarget->multiSampleTypeD3D,
                                                               &cQualityLevels);
                Assert(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE);
            }

            if (    pState->fSupportedSurfaceINTZ
                &&  pRenderTarget->multiSampleTypeD3D == D3DMULTISAMPLE_NONE
                &&  (   pRenderTarget->formatD3D == D3DFMT_D24S8
                     || pRenderTarget->formatD3D == D3DFMT_D24X8))
            {
                LogFunc(("Creating stencil surface as texture!\n"));
                int rc = vmsvga3dBackCreateTexture(pState, pContext, cid, pRenderTarget);
                AssertRC(rc);   /* non-fatal, will use CreateDepthStencilSurface */
            }

            if (!pRenderTarget->fStencilAsTexture)
            {
                Assert(!pRenderTarget->u.pSurface);

                LogFunc(("DEPTH/STENCIL; cQualityLevels=%d\n", cQualityLevels));
                hr = pContext->pDevice->CreateDepthStencilSurface(pRenderTarget->pMipmapLevels[0].mipmapSize.width,
                                                                  pRenderTarget->pMipmapLevels[0].mipmapSize.height,
                                                                  pRenderTarget->formatD3D,
                                                                  pRenderTarget->multiSampleTypeD3D,
                                                                  ((cQualityLevels >= 1) ? cQualityLevels - 1 : 0),   /* 0 - (levels-1) */
                                                                  FALSE,    /* not discardable */
                                                                  &pRenderTarget->u.pSurface,
                                                                  NULL);
                AssertMsgReturn(hr == D3D_OK, ("CreateDepthStencilSurface failed with %x\n", hr), VERR_INTERNAL_ERROR);
                pRenderTarget->enmD3DResType = VMSVGA3D_D3DRESTYPE_SURFACE;
            }

            pRenderTarget->idAssociatedContext = cid;

#if 0 /* doesn't work */
            if (    !pRenderTarget->fStencilAsTexture
                &&  pRenderTarget->fDirty)
            {
                Log(("vmsvga3dSetRenderTarget: sync dirty depth/stencil buffer\n"));
                Assert(pRenderTarget->faces[0].numMipLevels == 1);

                for (uint32_t i = 0; i < pRenderTarget->faces[0].numMipLevels; i++)
                {
                    if (pRenderTarget->pMipmapLevels[i].fDirty)
                    {
                        D3DLOCKED_RECT LockedRect;

                        hr = pRenderTarget->u.pSurface->LockRect(&LockedRect,
                                                                 NULL,   /* entire surface */
                                                                 0);

                        AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSetRenderTarget: LockRect failed with %x\n", hr), VERR_INTERNAL_ERROR);

                        Log(("vmsvga3dSetRenderTarget: sync dirty texture mipmap level %d (pitch %x vs %x)\n", i, LockedRect.Pitch, pRenderTarget->pMipmapLevels[i].cbSurfacePitch));

                        uint8_t *pDest = (uint8_t *)LockedRect.pBits;
                        uint8_t *pSrc  = (uint8_t *)pRenderTarget->pMipmapLevels[i].pSurfaceData;
                        for (uint32_t j = 0; j < pRenderTarget->pMipmapLevels[i].size.height; j++)
                        {
                            memcpy(pDest, pSrc, pRenderTarget->pMipmapLevels[i].cbSurfacePitch);

                            pDest += LockedRect.Pitch;
                            pSrc  += pRenderTarget->pMipmapLevels[i].cbSurfacePitch;
                        }

                        hr = pRenderTarget->u.pSurface->UnlockRect();
                        AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSetRenderTarget: UnlockRect failed with %x\n", hr), VERR_INTERNAL_ERROR);

                        pRenderTarget->pMipmapLevels[i].fDirty = false;
                    }
                }
            }
#endif
        }
        Assert(pRenderTarget->idAssociatedContext == cid);

        /** @todo Assert(!pRenderTarget->fDirty); */

        AssertReturn(pRenderTarget->u.pSurface, VERR_INVALID_PARAMETER);

        pRenderTarget->fUsageD3D            |= D3DUSAGE_DEPTHSTENCIL;
        pRenderTarget->surfaceFlags         |= SVGA3D_SURFACE_HINT_DEPTHSTENCIL;

        if (pRenderTarget->fStencilAsTexture)
        {
            IDirect3DSurface9 *pStencilSurface;

            hr = pRenderTarget->u.pTexture->GetSurfaceLevel(0, &pStencilSurface);
            AssertMsgReturn(hr == D3D_OK, ("GetSurfaceLevel failed with %x\n", hr), VERR_INTERNAL_ERROR);

            hr = pContext->pDevice->SetDepthStencilSurface(pStencilSurface);
            D3D_RELEASE(pStencilSurface);
            AssertMsgReturn(hr == D3D_OK, ("SetDepthStencilSurface failed with %x\n", hr), VERR_INTERNAL_ERROR);
        }
        else
        {
            hr = pContext->pDevice->SetDepthStencilSurface(pRenderTarget->u.pSurface);
            AssertMsgReturn(hr == D3D_OK, ("SetDepthStencilSurface failed with %x\n", hr), VERR_INTERNAL_ERROR);
        }
        break;

    case SVGA3D_RT_COLOR0:
    case SVGA3D_RT_COLOR1:
    case SVGA3D_RT_COLOR2:
    case SVGA3D_RT_COLOR3:
    case SVGA3D_RT_COLOR4:
    case SVGA3D_RT_COLOR5:
    case SVGA3D_RT_COLOR6:
    case SVGA3D_RT_COLOR7:
    {
        IDirect3DSurface9 *pSurface;
        bool fTexture = false;

        /* Must flush the other context's 3d pipeline to make sure all drawing is complete for the surface we're about to use. */
        vmsvga3dSurfaceFlush(pThis, pRenderTarget);

        if (pRenderTarget->surfaceFlags & SVGA3D_SURFACE_HINT_TEXTURE)
        {
            fTexture = true;

            /* A texture surface can be used as a render target to fill it and later on used as a texture. */
            if (!pRenderTarget->u.pTexture)
            {
                LogFunc(("Create texture to be used as render target; sid=%x type=%d format=%d -> create texture\n", target.sid, pRenderTarget->surfaceFlags, pRenderTarget->format));
                int rc = vmsvga3dBackCreateTexture(pState, pContext, cid, pRenderTarget);
                AssertRCReturn(rc, rc);
            }

            rc = vmsvga3dGetD3DSurface(pContext,  pRenderTarget, target.face, target.mipmap, false, &pSurface);
            AssertRCReturn(rc, rc);
        }
        else
        {
            AssertReturn(target.face == 0 && target.mipmap == 0, VERR_INVALID_PARAMETER);
            if (!pRenderTarget->u.pSurface)
            {
                DWORD cQualityLevels = 0;

                /* Query the nr of quality levels for this particular format */
                if (pRenderTarget->multiSampleTypeD3D != D3DMULTISAMPLE_NONE)
                {
                    hr = pState->pD3D9->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,
                                                                   D3DDEVTYPE_HAL,
                                                                   pRenderTarget->formatD3D,
                                                                   TRUE,        /* Windowed */
                                                                   pRenderTarget->multiSampleTypeD3D,
                                                                   &cQualityLevels);
                    Assert(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE);
                }

                LogFunc(("COLOR; cQualityLevels=%d\n", cQualityLevels));
                LogFunc(("Create rendertarget (%d,%d) formatD3D=%x multisample=%x\n",
                         pRenderTarget->pMipmapLevels[0].mipmapSize.width, pRenderTarget->pMipmapLevels[0].mipmapSize.height, pRenderTarget->formatD3D, pRenderTarget->multiSampleTypeD3D));

                hr = pContext->pDevice->CreateRenderTarget(pRenderTarget->pMipmapLevels[0].mipmapSize.width,
                                                           pRenderTarget->pMipmapLevels[0].mipmapSize.height,
                                                           pRenderTarget->formatD3D,
                                                           pRenderTarget->multiSampleTypeD3D,
                                                           ((cQualityLevels >= 1) ? cQualityLevels - 1 : 0),   /* 0 - (levels-1) */
                                                           TRUE,   /* lockable */
                                                           &pRenderTarget->u.pSurface,
                                                           NULL);
                AssertReturn(hr == D3D_OK, VERR_INTERNAL_ERROR);

                pRenderTarget->idAssociatedContext = cid;
                pRenderTarget->enmD3DResType       = VMSVGA3D_D3DRESTYPE_SURFACE;
            }
            else
                AssertReturn(pRenderTarget->fUsageD3D & D3DUSAGE_RENDERTARGET, VERR_INVALID_PARAMETER);

            Assert(pRenderTarget->idAssociatedContext == cid);
            Assert(pRenderTarget->enmD3DResType == VMSVGA3D_D3DRESTYPE_SURFACE);
            pSurface = pRenderTarget->u.pSurface;
        }

        AssertReturn(pRenderTarget->u.pSurface, VERR_INVALID_PARAMETER);
        Assert(!pRenderTarget->fDirty);

        pRenderTarget->fUsageD3D            |= D3DUSAGE_RENDERTARGET;
        pRenderTarget->surfaceFlags         |= SVGA3D_SURFACE_HINT_RENDERTARGET;

        hr = pContext->pDevice->SetRenderTarget(type - SVGA3D_RT_COLOR0, pSurface);
        if (fTexture)
            D3D_RELEASE(pSurface);    /* Release reference to texture level 0 */
        AssertMsgReturn(hr == D3D_OK, ("SetRenderTarget failed with %x\n", hr), VERR_INTERNAL_ERROR);

        pContext->sidRenderTarget = target.sid;

        /* Changing the render target resets the viewport; restore it here. */
        if (pContext->state.u32UpdateFlags & VMSVGA3D_UPDATE_VIEWPORT)
            vmsvga3dSetViewPort(pThis, cid, &pContext->state.RectViewPort);
        /* Changing the render target also resets the scissor rectangle; restore it as well. */
        if (pContext->state.u32UpdateFlags & VMSVGA3D_UPDATE_SCISSORRECT)
            vmsvga3dSetScissorRect(pThis, cid, &pContext->state.RectScissor);

        break;
    }

    default:
        AssertFailedReturn(VERR_INVALID_PARAMETER);
    }

    return VINF_SUCCESS;
}

/**
 * Convert SVGA texture combiner value to its D3D equivalent
 */
static DWORD vmsvga3dTextureCombiner2D3D(uint32_t value)
{
    switch (value)
    {
    case SVGA3D_TC_DISABLE:
        return D3DTOP_DISABLE;
    case SVGA3D_TC_SELECTARG1:
        return D3DTOP_SELECTARG1;
    case SVGA3D_TC_SELECTARG2:
        return D3DTOP_SELECTARG2;
    case SVGA3D_TC_MODULATE:
        return D3DTOP_MODULATE;
    case SVGA3D_TC_ADD:
        return D3DTOP_ADD;
    case SVGA3D_TC_ADDSIGNED:
        return D3DTOP_ADDSIGNED;
    case SVGA3D_TC_SUBTRACT:
        return D3DTOP_SUBTRACT;
    case SVGA3D_TC_BLENDTEXTUREALPHA:
        return D3DTOP_BLENDTEXTUREALPHA;
    case SVGA3D_TC_BLENDDIFFUSEALPHA:
        return D3DTOP_BLENDDIFFUSEALPHA;
    case SVGA3D_TC_BLENDCURRENTALPHA:
        return D3DTOP_BLENDCURRENTALPHA;
    case SVGA3D_TC_BLENDFACTORALPHA:
        return D3DTOP_BLENDFACTORALPHA;
    case SVGA3D_TC_MODULATE2X:
        return D3DTOP_MODULATE2X;
    case SVGA3D_TC_MODULATE4X:
        return D3DTOP_MODULATE4X;
    case SVGA3D_TC_DSDT:
        AssertFailed(); /** @todo ??? */
        return D3DTOP_DISABLE;
    case SVGA3D_TC_DOTPRODUCT3:
        return D3DTOP_DOTPRODUCT3;
    case SVGA3D_TC_BLENDTEXTUREALPHAPM:
        return D3DTOP_BLENDTEXTUREALPHAPM;
    case SVGA3D_TC_ADDSIGNED2X:
        return D3DTOP_ADDSIGNED2X;
    case SVGA3D_TC_ADDSMOOTH:
        return D3DTOP_ADDSMOOTH;
    case SVGA3D_TC_PREMODULATE:
        return D3DTOP_PREMODULATE;
    case SVGA3D_TC_MODULATEALPHA_ADDCOLOR:
        return D3DTOP_MODULATEALPHA_ADDCOLOR;
    case SVGA3D_TC_MODULATECOLOR_ADDALPHA:
        return D3DTOP_MODULATECOLOR_ADDALPHA;
    case SVGA3D_TC_MODULATEINVALPHA_ADDCOLOR:
        return D3DTOP_MODULATEINVALPHA_ADDCOLOR;
    case SVGA3D_TC_MODULATEINVCOLOR_ADDALPHA:
        return D3DTOP_MODULATEINVCOLOR_ADDALPHA;
    case SVGA3D_TC_BUMPENVMAPLUMINANCE:
        return D3DTOP_BUMPENVMAPLUMINANCE;
    case SVGA3D_TC_MULTIPLYADD:
        return D3DTOP_MULTIPLYADD;
    case SVGA3D_TC_LERP:
        return D3DTOP_LERP;
    default:
        AssertFailed();
        return D3DTOP_DISABLE;
    }
}

/**
 * Convert SVGA texture arg data value to its D3D equivalent
 */
static DWORD vmsvga3dTextureArgData2D3D(uint32_t value)
{
    switch (value)
    {
    case SVGA3D_TA_CONSTANT:
        return D3DTA_CONSTANT;
    case SVGA3D_TA_PREVIOUS:
        return D3DTA_CURRENT;   /* current = previous */
    case SVGA3D_TA_DIFFUSE:
        return D3DTA_DIFFUSE;
    case SVGA3D_TA_TEXTURE:
        return D3DTA_TEXTURE;
    case SVGA3D_TA_SPECULAR:
        return D3DTA_SPECULAR;
    default:
        AssertFailed();
        return 0;
    }
}

/**
 * Convert SVGA texture transform flag value to its D3D equivalent
 */
static DWORD vmsvga3dTextTransformFlags2D3D(uint32_t value)
{
    switch (value)
    {
    case SVGA3D_TEX_TRANSFORM_OFF:
        return D3DTTFF_DISABLE;
    case SVGA3D_TEX_TRANSFORM_S:
        return D3DTTFF_COUNT1;      /** @todo correct? */
    case SVGA3D_TEX_TRANSFORM_T:
        return D3DTTFF_COUNT2;      /** @todo correct? */
    case SVGA3D_TEX_TRANSFORM_R:
        return D3DTTFF_COUNT3;      /** @todo correct? */
    case SVGA3D_TEX_TRANSFORM_Q:
        return D3DTTFF_COUNT4;      /** @todo correct? */
    case SVGA3D_TEX_PROJECTED:
        return D3DTTFF_PROJECTED;
    default:
        AssertFailed();
        return 0;
    }
}

static DWORD vmsvga3dSamplerIndex2D3D(uint32_t idxSampler)
{
    if (idxSampler < SVGA3D_MAX_SAMPLERS_PS)
        return idxSampler;
    return (idxSampler - SVGA3D_MAX_SAMPLERS_PS) + D3DDMAPSAMPLER;
}

int vmsvga3dSetTextureState(PVGASTATE pThis, uint32_t cid, uint32_t cTextureStates, SVGA3dTextureState *pTextureState)
{
    DWORD                       val = 0; /* Shut up MSC */
    HRESULT                     hr;
    PVMSVGA3DCONTEXT            pContext;
    PVMSVGA3DSTATE              pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    LogFunc(("%x cTextureState=%d\n", cid, cTextureStates));

    int rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    for (unsigned i = 0; i < cTextureStates; i++)
    {
        LogFunc(("cid=%x stage=%d type=%s (%x) val=%x\n", cid, pTextureState[i].stage, vmsvga3dTextureStateToString(pTextureState[i].name), pTextureState[i].name, pTextureState[i].value));

        if (pTextureState[i].name == SVGA3D_TS_BIND_TEXTURE)
        {
            /* Special case: binding a texture to a sampler. Stage is the sampler index. */
            const uint32_t sid = pTextureState[i].value;
            const uint32_t idxSampler = pTextureState[i].stage;

            if (RT_UNLIKELY(idxSampler >= SVGA3D_MAX_SAMPLERS))
            {
                AssertMsgFailed(("pTextureState[%d]: SVGA3D_TS_BIND_TEXTURE idxSampler=%d, sid=%x\n", i, idxSampler, sid));
                continue;
            }

            const DWORD d3dSampler = vmsvga3dSamplerIndex2D3D(idxSampler);
            if (sid == SVGA3D_INVALID_ID)
            {
                LogFunc(("SVGA3D_TS_BIND_TEXTURE: unbind sampler=%d\n", idxSampler));

                pContext->aSidActiveTextures[idxSampler] = SVGA3D_INVALID_ID;

                /* Unselect the currently associated texture. */
                hr = pContext->pDevice->SetTexture(d3dSampler, NULL);
                AssertMsgReturn(hr == D3D_OK, ("SetTexture failed with %x\n", hr), VERR_INTERNAL_ERROR);
            }
            else
            {
                PVMSVGA3DSURFACE pSurface;
                rc = vmsvga3dSurfaceFromSid(pState, sid, &pSurface);
                AssertRCReturn(rc, rc);

                LogFunc(("SVGA3D_TS_BIND_TEXTURE: bind idxSampler=%d, texture sid=%x (%d,%d)\n", idxSampler, sid, pSurface->pMipmapLevels[0].mipmapSize.width, pSurface->pMipmapLevels[0].mipmapSize.height));

                if (!pSurface->u.pTexture)
                {
                    Assert(pSurface->idAssociatedContext == SVGA3D_INVALID_ID);
                    LogFunc(("CreateTexture (%d,%d) level=%d fUsage=%x format=%x\n", pSurface->pMipmapLevels[0].mipmapSize.width, pSurface->pMipmapLevels[0].mipmapSize.height, pSurface->faces[0].numMipLevels, pSurface->fUsageD3D, pSurface->formatD3D));
                    rc = vmsvga3dBackCreateTexture(pState, pContext, cid, pSurface);
                    AssertRCReturn(rc, rc);
                }
                else
                {
                    Assert(   pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_TEXTURE
                           || pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_CUBE_TEXTURE
                           || pSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_VOLUME_TEXTURE);
                    /* Must flush the other context's 3d pipeline to make sure all drawing is complete for the surface we're about to use. */
                    vmsvga3dSurfaceFlush(pThis, pSurface);
                }

#ifndef VBOX_VMSVGA3D_WITH_WINE_OPENGL
                if (pSurface->idAssociatedContext != cid)
                {
                    LogFunc(("Using texture sid=%x created for another context (%d vs %d)\n", sid, pSurface->idAssociatedContext, cid));

                    PVMSVGA3DSHAREDSURFACE pSharedSurface = vmsvga3dSurfaceGetSharedCopy(pContext, pSurface);
                    AssertReturn(pSharedSurface, VERR_INTERNAL_ERROR);

                    hr = pContext->pDevice->SetTexture(d3dSampler, pSharedSurface->u.pTexture);
                }
                else
#endif
                    hr = pContext->pDevice->SetTexture(d3dSampler, pSurface->u.pTexture);

                AssertMsgReturn(hr == D3D_OK, ("SetTexture failed with %x\n", hr), VERR_INTERNAL_ERROR);

                pContext->aSidActiveTextures[idxSampler] = sid;
            }
            /* Finished; continue with the next one. */
            continue;
        }

        D3DTEXTURESTAGESTATETYPE textureType = D3DTSS_FORCE_DWORD;
        D3DSAMPLERSTATETYPE      samplerType = D3DSAMP_FORCE_DWORD;
        switch (pTextureState[i].name)
        {
        case SVGA3D_TS_COLOROP:                     /* SVGA3dTextureCombiner */
            textureType = D3DTSS_COLOROP;
            val = vmsvga3dTextureCombiner2D3D(pTextureState[i].value);
            break;

        case SVGA3D_TS_COLORARG0:                   /* SVGA3dTextureArgData */
            textureType = D3DTSS_COLORARG0;
            val = vmsvga3dTextureArgData2D3D(pTextureState[i].value);
            break;

        case SVGA3D_TS_COLORARG1:                   /* SVGA3dTextureArgData */
            textureType = D3DTSS_COLORARG1;
            val = vmsvga3dTextureArgData2D3D(pTextureState[i].value);
            break;

        case SVGA3D_TS_COLORARG2:                   /* SVGA3dTextureArgData */
            textureType = D3DTSS_COLORARG2;
            val = vmsvga3dTextureArgData2D3D(pTextureState[i].value);
            break;

        case SVGA3D_TS_ALPHAOP:                     /* SVGA3dTextureCombiner */
            textureType = D3DTSS_ALPHAOP;
            val = vmsvga3dTextureCombiner2D3D(pTextureState[i].value);
            break;

        case SVGA3D_TS_ALPHAARG0:                   /* SVGA3dTextureArgData */
            textureType = D3DTSS_ALPHAARG0;
            val = vmsvga3dTextureArgData2D3D(pTextureState[i].value);
            break;

        case SVGA3D_TS_ALPHAARG1:                   /* SVGA3dTextureArgData */
            textureType = D3DTSS_ALPHAARG1;
            val = vmsvga3dTextureArgData2D3D(pTextureState[i].value);
            break;

        case SVGA3D_TS_ALPHAARG2:                   /* SVGA3dTextureArgData */
            textureType = D3DTSS_ALPHAARG2;
            val = vmsvga3dTextureArgData2D3D(pTextureState[i].value);
            break;

        case SVGA3D_TS_BUMPENVMAT00:                /* float */
            textureType = D3DTSS_BUMPENVMAT00;
            val = pTextureState[i].value;
            break;

        case SVGA3D_TS_BUMPENVMAT01:                /* float */
            textureType = D3DTSS_BUMPENVMAT01;
            val = pTextureState[i].value;
            break;

        case SVGA3D_TS_BUMPENVMAT10:                /* float */
            textureType = D3DTSS_BUMPENVMAT10;
            val = pTextureState[i].value;
            break;

        case SVGA3D_TS_BUMPENVMAT11:                /* float */
            textureType = D3DTSS_BUMPENVMAT11;
            val = pTextureState[i].value;
            break;

        case SVGA3D_TS_TEXCOORDINDEX:               /* uint32_t */
            textureType = D3DTSS_TEXCOORDINDEX;
            val = pTextureState[i].value;
            break;

        case SVGA3D_TS_BUMPENVLSCALE:               /* float */
            textureType = D3DTSS_BUMPENVLSCALE;
            val = pTextureState[i].value;
            break;

        case SVGA3D_TS_BUMPENVLOFFSET:              /* float */
            textureType = D3DTSS_BUMPENVLOFFSET;
            val = pTextureState[i].value;
            break;

        case SVGA3D_TS_TEXTURETRANSFORMFLAGS:       /* SVGA3dTexTransformFlags */
            textureType = D3DTSS_TEXTURETRANSFORMFLAGS;
            val = vmsvga3dTextTransformFlags2D3D(pTextureState[i].value);
            break;

        case SVGA3D_TS_ADDRESSW:                    /* SVGA3dTextureAddress */
            samplerType = D3DSAMP_ADDRESSW;
            val = pTextureState[i].value;   /* Identical otherwise */
            Assert(pTextureState[i].value != SVGA3D_TEX_ADDRESS_EDGE);
            break;

        case SVGA3D_TS_ADDRESSU:                    /* SVGA3dTextureAddress */
            samplerType = D3DSAMP_ADDRESSU;
            val = pTextureState[i].value;   /* Identical otherwise */
            Assert(pTextureState[i].value != SVGA3D_TEX_ADDRESS_EDGE);
            break;

        case SVGA3D_TS_ADDRESSV:                    /* SVGA3dTextureAddress */
            samplerType = D3DSAMP_ADDRESSV;
            val = pTextureState[i].value;   /* Identical otherwise */
            Assert(pTextureState[i].value != SVGA3D_TEX_ADDRESS_EDGE);
            break;

        case SVGA3D_TS_MIPFILTER:                   /* SVGA3dTextureFilter */
            samplerType = D3DSAMP_MIPFILTER;
            val = pTextureState[i].value;   /* Identical otherwise */
            Assert(pTextureState[i].value != SVGA3D_TEX_FILTER_FLATCUBIC);
            Assert(pTextureState[i].value != SVGA3D_TEX_FILTER_GAUSSIANCUBIC);
            break;

        case SVGA3D_TS_MAGFILTER:                   /* SVGA3dTextureFilter */
            samplerType = D3DSAMP_MAGFILTER;
            val = pTextureState[i].value;   /* Identical otherwise */
            Assert(pTextureState[i].value != SVGA3D_TEX_FILTER_FLATCUBIC);
            Assert(pTextureState[i].value != SVGA3D_TEX_FILTER_GAUSSIANCUBIC);
            break;

        case SVGA3D_TS_MINFILTER:                   /* SVGA3dTextureFilter */
            samplerType = D3DSAMP_MINFILTER;
            val = pTextureState[i].value;   /* Identical otherwise */
            Assert(pTextureState[i].value != SVGA3D_TEX_FILTER_FLATCUBIC);
            Assert(pTextureState[i].value != SVGA3D_TEX_FILTER_GAUSSIANCUBIC);
            break;

        case SVGA3D_TS_BORDERCOLOR:                 /* SVGA3dColor */
            samplerType = D3DSAMP_BORDERCOLOR;
            val = pTextureState[i].value;   /* Identical */
            break;

        case SVGA3D_TS_TEXTURE_LOD_BIAS:            /* float */
            samplerType = D3DSAMP_MIPMAPLODBIAS;
            val = pTextureState[i].value;   /* Identical */
            break;

        case SVGA3D_TS_TEXTURE_MIPMAP_LEVEL:        /* uint32_t */
            samplerType = D3DSAMP_MAXMIPLEVEL;
            val = pTextureState[i].value;   /* Identical?? */
            break;

        case SVGA3D_TS_TEXTURE_ANISOTROPIC_LEVEL:   /* uint32_t */
            samplerType = D3DSAMP_MAXANISOTROPY;
            val = pTextureState[i].value;   /* Identical?? */
            break;

        case SVGA3D_TS_GAMMA:                       /* float */
            samplerType = D3DSAMP_SRGBTEXTURE;
            /* Boolean in D3D */
            if (pTextureState[i].floatValue == 1.0f)
                val = FALSE;
            else
                val = TRUE;
            break;

        /* Internal commands, that don't map directly to the SetTextureStageState API. */
        case SVGA3D_TS_TEXCOORDGEN:                 /* SVGA3dTextureCoordGen */
            AssertFailed();
            break;

        case SVGA3D_TS_MAX:                   /* shut up MSC */
        case SVGA3D_TS_INVALID:
        case SVGA3D_TS_BIND_TEXTURE:
            AssertFailedBreak();
        }

        const uint32_t currentStage = pTextureState[i].stage;
        /* Record the texture state for vm state saving. */
        if (   currentStage < RT_ELEMENTS(pContext->state.aTextureStates)
            && pTextureState[i].name < RT_ELEMENTS(pContext->state.aTextureStates[0]))
        {
            pContext->state.aTextureStates[currentStage][pTextureState[i].name] = pTextureState[i];
        }

        if (textureType != D3DTSS_FORCE_DWORD)
        {
            if (RT_UNLIKELY(currentStage >= SVGA3D_MAX_TEXTURE_STAGES))
            {
                AssertMsgFailed(("pTextureState[%d].stage=%#x name=%#x value=%#x\n", i, pTextureState[i].stage, pTextureState[i].name, pTextureState[i].value));
                continue;
            }

            hr = pContext->pDevice->SetTextureStageState(currentStage, textureType, val);
            AssertMsg(hr == D3D_OK, ("SetTextureStageState failed with %x\n", hr));
        }
        else if (samplerType != D3DSAMP_FORCE_DWORD)
        {
            if (RT_UNLIKELY(currentStage >= SVGA3D_MAX_SAMPLERS))
            {
                AssertMsgFailed(("pTextureState[%d].stage=%#x name=%#x value=%#x\n", i, pTextureState[i].stage, pTextureState[i].name, pTextureState[i].value));
                continue;
            }

            hr = pContext->pDevice->SetSamplerState(currentStage, samplerType, val);
            AssertMsg(hr == D3D_OK, ("SetSamplerState failed with %x\n", hr));
        }
        else
        {
            AssertFailed();
        }
    }

    return VINF_SUCCESS;
}

int vmsvga3dSetMaterial(PVGASTATE pThis, uint32_t cid, SVGA3dFace face, SVGA3dMaterial *pMaterial)
{
    HRESULT               hr;
    D3DMATERIAL9          material;
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    LogFunc(("cid=%x face %d\n", cid, face));

    int rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    AssertReturn(face < SVGA3D_FACE_MAX, VERR_INVALID_PARAMETER);

    /* Save for vm state save/restore. */
    pContext->state.aMaterial[face].fValid = true;
    pContext->state.aMaterial[face].material = *pMaterial;
    pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_MATERIAL;

    /* @note face not used for D3D9 */
    /** @todo ignore everything except SVGA3D_FACE_NONE? */
    //Assert(face == SVGA3D_FACE_NONE);
    if (face != SVGA3D_FACE_NONE)
        Log(("Unsupported face %d!!\n", face));

    material.Diffuse.r     = pMaterial->diffuse[0];
    material.Diffuse.g     = pMaterial->diffuse[1];
    material.Diffuse.b     = pMaterial->diffuse[2];
    material.Diffuse.a     = pMaterial->diffuse[3];
    material.Ambient.r     = pMaterial->ambient[0];
    material.Ambient.g     = pMaterial->ambient[1];
    material.Ambient.b     = pMaterial->ambient[2];
    material.Ambient.a     = pMaterial->ambient[3];
    material.Specular.r    = pMaterial->specular[0];
    material.Specular.g    = pMaterial->specular[1];
    material.Specular.b    = pMaterial->specular[2];
    material.Specular.a    = pMaterial->specular[3];
    material.Emissive.r    = pMaterial->emissive[0];
    material.Emissive.g    = pMaterial->emissive[1];
    material.Emissive.b    = pMaterial->emissive[2];
    material.Emissive.a    = pMaterial->emissive[3];
    material.Power         = pMaterial->shininess;

    hr = pContext->pDevice->SetMaterial(&material);
    AssertMsgReturn(hr == D3D_OK, ("SetMaterial failed with %x\n", hr), VERR_INTERNAL_ERROR);

    return VINF_SUCCESS;
}

int vmsvga3dSetLightData(PVGASTATE pThis, uint32_t cid, uint32_t index, SVGA3dLightData *pData)
{
    HRESULT               hr;
    D3DLIGHT9             light;
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetLightData %x index=%d\n", cid, index));

    int rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    switch (pData->type)
    {
    case SVGA3D_LIGHTTYPE_POINT:
        light.Type = D3DLIGHT_POINT;
        break;

    case SVGA3D_LIGHTTYPE_SPOT1:    /* 1-cone, in degrees */
        light.Type = D3DLIGHT_SPOT;
        break;

    case SVGA3D_LIGHTTYPE_DIRECTIONAL:
        light.Type = D3DLIGHT_DIRECTIONAL;
        break;

    case SVGA3D_LIGHTTYPE_SPOT2:    /* 2-cone, in radians */
    default:
        Log(("Unsupported light type!!\n"));
        return VERR_INVALID_PARAMETER;
    }

    /* Store for vm state save/restore */
    if (index < SVGA3D_MAX_LIGHTS)
    {
        pContext->state.aLightData[index].fValidData = true;
        pContext->state.aLightData[index].data = *pData;
    }
    else
        AssertFailed();

    light.Diffuse.r     = pData->diffuse[0];
    light.Diffuse.g     = pData->diffuse[1];
    light.Diffuse.b     = pData->diffuse[2];
    light.Diffuse.a     = pData->diffuse[3];
    light.Specular.r    = pData->specular[0];
    light.Specular.g    = pData->specular[1];
    light.Specular.b    = pData->specular[2];
    light.Specular.a    = pData->specular[3];
    light.Ambient.r     = pData->ambient[0];
    light.Ambient.g     = pData->ambient[1];
    light.Ambient.b     = pData->ambient[2];
    light.Ambient.a     = pData->ambient[3];
    light.Position.x    = pData->position[0];
    light.Position.y    = pData->position[1];
    light.Position.z    = pData->position[2];       /* @note 4th position not available in D3D9 */
    light.Direction.x   = pData->direction[0];
    light.Direction.y   = pData->direction[1];
    light.Direction.z   = pData->direction[2];      /* @note 4th position not available in D3D9 */
    light.Range         = pData->range;
    light.Falloff       = pData->falloff;
    light.Attenuation0  = pData->attenuation0;
    light.Attenuation1  = pData->attenuation1;
    light.Attenuation2  = pData->attenuation2;
    light.Theta         = pData->theta;
    light.Phi           = pData->phi;

    hr = pContext->pDevice->SetLight(index, &light);
    AssertMsgReturn(hr == D3D_OK, ("SetLight failed with %x\n", hr), VERR_INTERNAL_ERROR);

    return VINF_SUCCESS;
}

int vmsvga3dSetLightEnabled(PVGASTATE pThis, uint32_t cid, uint32_t index, uint32_t enabled)
{
    HRESULT               hr;
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetLightEnabled %x %d -> %d\n", cid, index, enabled));

    int rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    /* Store for vm state save/restore */
    if (index < SVGA3D_MAX_LIGHTS)
        pContext->state.aLightData[index].fEnabled = !!enabled;
    else
        AssertFailed();

    hr = pContext->pDevice->LightEnable(index, (BOOL)enabled);
    AssertMsgReturn(hr == D3D_OK, ("LightEnable failed with %x\n", hr), VERR_INTERNAL_ERROR);

    return VINF_SUCCESS;
}

int vmsvga3dSetViewPort(PVGASTATE pThis, uint32_t cid, SVGA3dRect *pRect)
{
    HRESULT               hr;
    D3DVIEWPORT9          viewPort;
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetViewPort %x (%d,%d)(%d,%d)\n", cid, pRect->x, pRect->y, pRect->w, pRect->h));

    int rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    /* Save for vm state save/restore. */
    pContext->state.RectViewPort = *pRect;
    pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_VIEWPORT;

    hr = pContext->pDevice->GetViewport(&viewPort);
    AssertMsgReturn(hr == D3D_OK, ("GetViewport failed with %x\n", hr), VERR_INTERNAL_ERROR);

    viewPort.X      = pRect->x;
    viewPort.Y      = pRect->y;
    viewPort.Width  = pRect->w;
    viewPort.Height = pRect->h;
    /* viewPort.MinZ & MaxZ are not changed from the current setting. */

    hr = pContext->pDevice->SetViewport(&viewPort);
    AssertMsgReturn(hr == D3D_OK, ("SetViewport failed with %x\n", hr), VERR_INTERNAL_ERROR);

    return VINF_SUCCESS;
}

int vmsvga3dSetClipPlane(PVGASTATE pThis, uint32_t cid, uint32_t index, float plane[4])
{
    HRESULT               hr;
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetClipPlane %x %d (%d,%d)(%d,%d)\n", cid, index, (unsigned)(plane[0] * 100.0), (unsigned)(plane[1] * 100.0), (unsigned)(plane[2] * 100.0), (unsigned)(plane[3] * 100.0)));
    AssertReturn(index < SVGA3D_CLIPPLANE_MAX, VERR_INVALID_PARAMETER);

    int rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    /* Store for vm state save/restore. */
    pContext->state.aClipPlane[index].fValid = true;
    memcpy(pContext->state.aClipPlane[index].plane, plane, sizeof(pContext->state.aClipPlane[index].plane));

    hr = pContext->pDevice->SetClipPlane(index, plane);
    AssertMsgReturn(hr == D3D_OK, ("SetClipPlane failed with %x\n", hr), VERR_INTERNAL_ERROR);
    return VINF_SUCCESS;
}

int vmsvga3dCommandClear(PVGASTATE pThis, uint32_t cid, SVGA3dClearFlag clearFlag, uint32_t color, float depth, uint32_t stencil, uint32_t cRects, SVGA3dRect *pRect)
{
    DWORD                 clearFlagD3D = 0;
    D3DRECT              *pRectD3D = NULL;
    HRESULT               hr;
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dCommandClear %x clearFlag=%x color=%x depth=%d stencil=%x cRects=%d\n", cid, clearFlag, color, (uint32_t)(depth * 100.0), stencil, cRects));

    int rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    if (clearFlag & SVGA3D_CLEAR_COLOR)
        clearFlagD3D |= D3DCLEAR_TARGET;
    if (clearFlag & SVGA3D_CLEAR_STENCIL)
        clearFlagD3D |= D3DCLEAR_STENCIL;
    if (clearFlag & SVGA3D_CLEAR_DEPTH)
        clearFlagD3D |= D3DCLEAR_ZBUFFER;

    if (cRects)
    {
        pRectD3D = (D3DRECT *)RTMemAlloc(sizeof(D3DRECT) * cRects);
        AssertReturn(pRectD3D, VERR_NO_MEMORY);

        for (unsigned i=0; i < cRects; i++)
        {
            Log(("vmsvga3dCommandClear: rect %d (%d,%d)(%d,%d)\n", i, pRect[i].x, pRect[i].y, pRect[i].x + pRect[i].w, pRect[i].y + pRect[i].h));
            pRectD3D[i].x1 = pRect[i].x;
            pRectD3D[i].y1 = pRect[i].y;
            pRectD3D[i].x2 = pRect[i].x + pRect[i].w;   /* exclusive */
            pRectD3D[i].y2 = pRect[i].y + pRect[i].h;   /* exclusive */
        }
    }

    hr = pContext->pDevice->Clear(cRects, pRectD3D, clearFlagD3D, (D3DCOLOR)color, depth, stencil);
    if (pRectD3D)
        RTMemFree(pRectD3D);

    AssertMsgReturn(hr == D3D_OK, ("Clear failed with %x\n", hr), VERR_INTERNAL_ERROR);

    /* Make sure we can track drawing usage of active render targets. */
    if (pContext->sidRenderTarget != SVGA3D_INVALID_ID)
        vmsvga3dSurfaceTrackUsageById(pState, pContext, pContext->sidRenderTarget);

    return VINF_SUCCESS;
}

/* Convert VMWare vertex declaration to its D3D equivalent. */
static int vmsvga3dVertexDecl2D3D(const SVGA3dVertexArrayIdentity &identity, D3DVERTEXELEMENT9 *pVertexElement)
{
    /* usage, method and type are identical; make sure. */
    AssertCompile(SVGA3D_DECLTYPE_FLOAT1 == D3DDECLTYPE_FLOAT1);
    AssertCompile(SVGA3D_DECLTYPE_FLOAT16_4 == D3DDECLTYPE_FLOAT16_4);
    AssertCompile(SVGA3D_DECLMETHOD_DEFAULT == D3DDECLMETHOD_DEFAULT);
    AssertCompile(SVGA3D_DECLMETHOD_LOOKUPPRESAMPLED == D3DDECLMETHOD_LOOKUPPRESAMPLED);
    AssertCompile(D3DDECLUSAGE_POSITION == SVGA3D_DECLUSAGE_POSITION);
    AssertCompile(D3DDECLUSAGE_SAMPLE == SVGA3D_DECLUSAGE_SAMPLE);

    pVertexElement->Stream      = 0;
    pVertexElement->Offset      = 0;
    pVertexElement->Type        = identity.type;
    pVertexElement->Method      = identity.method;
    pVertexElement->Usage       = identity.usage;
    pVertexElement->UsageIndex  = identity.usageIndex;
    return VINF_SUCCESS;
}

/* Convert VMWare primitive type to its D3D equivalent. */
static int vmsvga3dPrimitiveType2D3D(SVGA3dPrimitiveType PrimitiveType, D3DPRIMITIVETYPE *pPrimitiveTypeD3D)
{
    switch (PrimitiveType)
    {
    case SVGA3D_PRIMITIVE_TRIANGLELIST:
        *pPrimitiveTypeD3D = D3DPT_TRIANGLELIST;
        break;
    case SVGA3D_PRIMITIVE_POINTLIST:
        *pPrimitiveTypeD3D = D3DPT_POINTLIST;
        break;
    case SVGA3D_PRIMITIVE_LINELIST:
        *pPrimitiveTypeD3D = D3DPT_LINELIST;
        break;
    case SVGA3D_PRIMITIVE_LINESTRIP:
        *pPrimitiveTypeD3D = D3DPT_LINESTRIP;
        break;
    case SVGA3D_PRIMITIVE_TRIANGLESTRIP:
        *pPrimitiveTypeD3D = D3DPT_TRIANGLESTRIP;
        break;
    case SVGA3D_PRIMITIVE_TRIANGLEFAN:
        *pPrimitiveTypeD3D = D3DPT_TRIANGLEFAN;
        break;
    default:
        return VERR_INVALID_PARAMETER;
    }
    return VINF_SUCCESS;
}

#ifdef OLD_DRAW_PRIMITIVES /* Old vmsvga3dDrawPrimitives */

static int vmsvga3dDrawPrimitivesProcessVertexDecls(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, uint32_t numVertexDecls, SVGA3dVertexDecl *pVertexDecl, uint32_t idStream, D3DVERTEXELEMENT9 *pVertexElement)
{
    HRESULT               hr;
    int                   rc;
    uint32_t              uVertexMinOffset = 0xffffffff;
    uint32_t              uVertexMaxOffset = 0;

    /* Create a vertex declaration array */
    for (unsigned iVertex = 0; iVertex < numVertexDecls; iVertex++)
    {
        unsigned            sidVertex = pVertexDecl[iVertex].array.surfaceId;
        PVMSVGA3DSURFACE    pVertexSurface;

        AssertReturn(sidVertex < SVGA3D_MAX_SURFACE_IDS, VERR_INVALID_PARAMETER);
        AssertReturn(sidVertex < pState->cSurfaces && pState->papSurfaces[sidVertex]->id == sidVertex, VERR_INVALID_PARAMETER);

        pVertexSurface = pState->papSurfaces[sidVertex];
        Log(("vmsvga3dDrawPrimitives: vertex sid=%x stream %d\n", sidVertex, idStream));
        Log(("vmsvga3dDrawPrimitives: type=%s (%d) method=%s (%d) usage=%s (%d) usageIndex=%d stride=%d offset=%d\n", vmsvgaDeclType2String(pVertexDecl[iVertex].identity.type), pVertexDecl[iVertex].identity.type, vmsvgaDeclMethod2String(pVertexDecl[iVertex].identity.method), pVertexDecl[iVertex].identity.method, vmsvgaDeclUsage2String(pVertexDecl[iVertex].identity.usage), pVertexDecl[iVertex].identity.usage, pVertexDecl[iVertex].identity.usageIndex, pVertexDecl[iVertex].array.stride, pVertexDecl[iVertex].array.offset));

        rc = vmsvga3dVertexDecl2D3D(pVertexDecl[iVertex].identity, &pVertexElement[iVertex]);
        AssertRCReturn(rc, rc);
        pVertexElement[iVertex].Stream = idStream;

#ifdef LOG_ENABLED
        if (pVertexDecl[iVertex].array.stride == 0)
            Log(("vmsvga3dDrawPrimitives: stride == 0! Can be valid\n"));
#endif

        /* Find the min and max vertex offset to determine the right base offset to use in the vertex declaration. */
        if (pVertexDecl[iVertex].array.offset > uVertexMaxOffset)
            uVertexMaxOffset = pVertexDecl[iVertex].array.offset;
        if (pVertexDecl[iVertex].array.offset < uVertexMinOffset)
            uVertexMinOffset = pVertexDecl[iVertex].array.offset;

        if (   pVertexSurface->u.pSurface
            && pVertexSurface->enmD3DResType != VMSVGA3D_D3DRESTYPE_VERTEX_BUFFER)
        {
            /* The buffer object is not an vertex one. Switch type. */
            Assert(pVertexSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_INDEX_BUFFER);
            D3D_RELEASE(pVertexSurface->u.pIndexBuffer);
            pVertexSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_NONE;

            LogFunc(("index -> vertex buffer sid=%x\n", sidVertex));
        }

        if (!pVertexSurface->u.pVertexBuffer)
        {
            Assert(iVertex == 0);

            LogFunc(("create vertex buffer fDirty=%d\n", pVertexSurface->fDirty));
            hr = pContext->pDevice->CreateVertexBuffer(pVertexSurface->pMipmapLevels[0].cbSurface,
                                                       D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY /* possible severe performance penalty otherwise (according to d3d debug output) */,
                                                       0, /* non-FVF */
                                                       D3DPOOL_DEFAULT,
                                                       &pVertexSurface->u.pVertexBuffer,
                                                       NULL);
            AssertMsgReturn(hr == D3D_OK, ("CreateVertexBuffer failed with %x\n", hr), VERR_INTERNAL_ERROR);

            pVertexSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_VERTEX_BUFFER;
            pVertexSurface->idAssociatedContext = pContext->id;

            if (pVertexSurface->fDirty)
            {
                void *pData;

                hr = pVertexSurface->u.pVertexBuffer->Lock(0, 0, &pData, 0);
                AssertMsgReturn(hr == D3D_OK, ("Lock vertex failed with %x\n", hr), VERR_INTERNAL_ERROR);

                memcpy(pData, pVertexSurface->pMipmapLevels[0].pSurfaceData, pVertexSurface->pMipmapLevels[0].cbSurface);

                hr = pVertexSurface->u.pVertexBuffer->Unlock();
                AssertMsgReturn(hr == D3D_OK, ("Unlock vertex failed with %x\n", hr), VERR_INTERNAL_ERROR);
                pVertexSurface->pMipmapLevels[0].fDirty = false;
                pVertexSurface->fDirty = false;
            }
            pVertexSurface->surfaceFlags |= SVGA3D_SURFACE_HINT_VERTEXBUFFER;
        }
    }

    /* Set the right vertex offset values for each declaration. */
    for (unsigned iVertex = 0; iVertex < numVertexDecls; iVertex++)
    {
        pVertexElement[iVertex].Offset = pVertexDecl[iVertex].array.offset - uVertexMinOffset;
#ifdef LOG_ENABLED
        if (pVertexElement[iVertex].Offset >= pVertexDecl[0].array.stride)
            LogFunc(("WARNING: offset > stride!!\n"));
#endif

        LogFunc(("vertex %d offset = %d (stride %d) (min=%d max=%d)\n", iVertex, pVertexDecl[iVertex].array.offset, pVertexDecl[iVertex].array.stride, uVertexMinOffset, uVertexMaxOffset));
    }

    PVMSVGA3DSURFACE pVertexSurface;
    unsigned         sidVertex = pVertexDecl[0].array.surfaceId;
    unsigned         strideVertex = pVertexDecl[0].array.stride;

    pVertexSurface = pState->papSurfaces[sidVertex];

    LogFunc(("SetStreamSource %d min offset=%d stride=%d\n", idStream, uVertexMinOffset, strideVertex));
    hr = pContext->pDevice->SetStreamSource(idStream,
                                            pVertexSurface->u.pVertexBuffer,
                                            uVertexMinOffset,
                                            strideVertex);

    AssertMsgReturn(hr == D3D_OK, ("SetStreamSource failed with %x\n", hr), VERR_INTERNAL_ERROR);
    return VINF_SUCCESS;
}

int vmsvga3dDrawPrimitives(PVGASTATE pThis, uint32_t cid, uint32_t numVertexDecls, SVGA3dVertexDecl *pVertexDecl,
                           uint32_t numRanges, SVGA3dPrimitiveRange *pRange,
                           uint32_t cVertexDivisor, SVGA3dVertexDivisor *pVertexDivisor)
{
    RT_NOREF(pVertexDivisor);
    PVMSVGA3DCONTEXT             pContext;
    PVMSVGA3DSTATE               pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_INTERNAL_ERROR);
    int                          rc = VINF_SUCCESS;
    HRESULT                      hr;
    uint32_t                     iCurrentVertex, iCurrentStreamId;
    IDirect3DVertexDeclaration9 *pVertexDeclD3D = NULL;
    D3DVERTEXELEMENT9           *pVertexElement = NULL;
    D3DVERTEXELEMENT9            VertexEnd = D3DDECL_END();

    LogFunc(("%x numVertexDecls=%d numRanges=%d, cVertexDivisor=%d\n", cid, numVertexDecls, numRanges, cVertexDivisor));

    /* Caller already check these, but it cannot hurt to check again... */
    AssertReturn(numVertexDecls && numVertexDecls <= SVGA3D_MAX_VERTEX_ARRAYS, VERR_INVALID_PARAMETER);
    AssertReturn(numRanges && numRanges <= SVGA3D_MAX_DRAW_PRIMITIVE_RANGES, VERR_INVALID_PARAMETER);
    AssertReturn(!cVertexDivisor || cVertexDivisor == numVertexDecls, VERR_INVALID_PARAMETER);

    rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    /* Begin a scene before rendering anything. */
    hr = pContext->pDevice->BeginScene();
    AssertMsgReturn(hr == D3D_OK, ("BeginScene failed with %x\n", hr), VERR_INTERNAL_ERROR);

    pVertexElement = (D3DVERTEXELEMENT9 *)RTMemAllocZ(sizeof(D3DVERTEXELEMENT9) * (numVertexDecls + 1));
    if (!pVertexElement)
    {
        Assert(pVertexElement);
        rc = VERR_INTERNAL_ERROR;
        goto internal_error;
    }

    /* Process all vertex declarations. Each vertex buffer is represented by one stream source id. */
    iCurrentVertex   = 0;
    iCurrentStreamId = 0;
    while (iCurrentVertex < numVertexDecls)
    {
        uint32_t sidVertex = SVGA_ID_INVALID;
        uint32_t iVertex;
        uint32_t uVertexMinOffset = 0xffffffff;

        for (iVertex = iCurrentVertex; iVertex < numVertexDecls; iVertex++)
        {
            if (    (   sidVertex != SVGA_ID_INVALID
                     && pVertexDecl[iVertex].array.surfaceId != sidVertex
                    )
                /* We must put vertex declarations that start at a different element in another stream as d3d only handles offsets < stride. */
                ||  (   uVertexMinOffset != 0xffffffff
                     && pVertexDecl[iVertex].array.offset >= uVertexMinOffset + pVertexDecl[iCurrentVertex].array.stride
                    )
               )
                break;
            sidVertex = pVertexDecl[iVertex].array.surfaceId;

            if (uVertexMinOffset > pVertexDecl[iVertex].array.offset)
                uVertexMinOffset = pVertexDecl[iVertex].array.offset;
        }

        rc = vmsvga3dDrawPrimitivesProcessVertexDecls(pState, pContext, iVertex - iCurrentVertex, &pVertexDecl[iCurrentVertex], iCurrentStreamId, &pVertexElement[iCurrentVertex]);
        if (RT_FAILURE(rc))
            goto internal_error;

        if (cVertexDivisor)
        {
            LogFunc(("SetStreamSourceFreq[%d]=%x\n", iCurrentStreamId, pVertexDivisor[iCurrentStreamId].value));
            HRESULT hr2 = pContext->pDevice->SetStreamSourceFreq(iCurrentStreamId, pVertexDivisor[iCurrentStreamId].value);
            Assert(SUCCEEDED(hr2)); RT_NOREF(hr2);
        }

        iCurrentVertex = iVertex;
        iCurrentStreamId++;
    }

    /* Mark the end. */
    memcpy(&pVertexElement[numVertexDecls], &VertexEnd, sizeof(VertexEnd));

    hr = pContext->pDevice->CreateVertexDeclaration(&pVertexElement[0],
                                                    &pVertexDeclD3D);
    if (hr != D3D_OK)
    {
        AssertMsg(hr == D3D_OK, ("vmsvga3dDrawPrimitives: CreateVertexDeclaration failed with %x\n", hr));
        rc = VERR_INTERNAL_ERROR;
        goto internal_error;
    }

    hr = pContext->pDevice->SetVertexDeclaration(pVertexDeclD3D);
    if (hr != D3D_OK)
    {
        AssertMsg(hr == D3D_OK, ("vmsvga3dDrawPrimitives: SetVertexDeclaration failed with %x\n", hr));
        rc = VERR_INTERNAL_ERROR;
        goto internal_error;
    }

    /* Now draw the primitives. */
    for (unsigned iPrimitive = 0; iPrimitive < numRanges; iPrimitive++)
    {
        D3DPRIMITIVETYPE PrimitiveTypeD3D;
        unsigned         sidIndex  = pRange[iPrimitive].indexArray.surfaceId;
        PVMSVGA3DSURFACE pIndexSurface = NULL;

        Log(("Primitive %d: type %s\n", iPrimitive, vmsvga3dPrimitiveType2String(pRange[iPrimitive].primType)));
        rc = vmsvga3dPrimitiveType2D3D(pRange[iPrimitive].primType, &PrimitiveTypeD3D);
        if (RT_FAILURE(rc))
        {
            AssertRC(rc);
            goto internal_error;
        }

        /* Triangle strips or fans with just one primitive don't make much sense and are identical to triangle lists.
         * Workaround for NVidia driver crash when encountering some of these.
         */
        if (    pRange[iPrimitive].primitiveCount == 1
            &&  (   PrimitiveTypeD3D == D3DPT_TRIANGLESTRIP
                 || PrimitiveTypeD3D == D3DPT_TRIANGLEFAN))
            PrimitiveTypeD3D = D3DPT_TRIANGLELIST;

        if (sidIndex != SVGA3D_INVALID_ID)
        {
            AssertMsg(pRange[iPrimitive].indexWidth == sizeof(uint32_t) || pRange[iPrimitive].indexWidth == sizeof(uint16_t), ("Unsupported primitive width %d\n", pRange[iPrimitive].indexWidth));

            rc = vmsvga3dSurfaceFromSid(pState, sidIndex, &pIndexSurface);
            if (RT_FAILURE(rc))
                goto internal_error;

            Log(("vmsvga3dDrawPrimitives: index sid=%x\n", sidIndex));

            if (   pIndexSurface->u.pSurface
                && pIndexSurface->enmD3DResType != VMSVGA3D_D3DRESTYPE_INDEX_BUFFER)
            {
                /* The buffer object is not an index one. Switch type. */
                Assert(pIndexSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_VERTEX_BUFFER);
                D3D_RELEASE(pIndexSurface->u.pVertexBuffer);
                pIndexSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_NONE;

                LogFunc(("vertex -> index buffer sid=%x\n", sidIndex));
            }

            if (!pIndexSurface->u.pIndexBuffer)
            {
                Log(("vmsvga3dDrawPrimitives: create index buffer fDirty=%d\n", pIndexSurface->fDirty));

                hr = pContext->pDevice->CreateIndexBuffer(pIndexSurface->pMipmapLevels[0].cbSurface,
                                                          D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY /* possible severe performance penalty otherwise (according to d3d debug output */,
                                                          (pRange[iPrimitive].indexWidth == sizeof(uint16_t)) ? D3DFMT_INDEX16 : D3DFMT_INDEX32,
                                                          D3DPOOL_DEFAULT,
                                                          &pIndexSurface->u.pIndexBuffer,
                                                          NULL);
                if (hr != D3D_OK)
                {
                    AssertMsg(hr == D3D_OK, ("vmsvga3dDrawPrimitives: CreateIndexBuffer failed with %x\n", hr));
                    rc = VERR_INTERNAL_ERROR;
                    goto internal_error;
                }
                pIndexSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_INDEX_BUFFER;

                if (pIndexSurface->fDirty)
                {
                    void *pData;

                    Log(("vmsvga3dDrawPrimitives: sync index buffer\n"));

                    hr = pIndexSurface->u.pIndexBuffer->Lock(0, 0, &pData, 0);
                    AssertMsg(hr == D3D_OK, ("vmsvga3dDrawPrimitives: Lock vertex failed with %x\n", hr));

                    memcpy(pData, pIndexSurface->pMipmapLevels[0].pSurfaceData, pIndexSurface->pMipmapLevels[0].cbSurface);

                    hr = pIndexSurface->u.pIndexBuffer->Unlock();
                    AssertMsg(hr == D3D_OK, ("vmsvga3dDrawPrimitives: Unlock vertex failed with %x\n", hr));

                    pIndexSurface->pMipmapLevels[0].fDirty = false;
                    pIndexSurface->fDirty = false;
                }
                pIndexSurface->surfaceFlags |= SVGA3D_SURFACE_HINT_INDEXBUFFER;
            }

            hr = pContext->pDevice->SetIndices(pIndexSurface->u.pIndexBuffer);
            AssertMsg(hr == D3D_OK, ("SetIndices vertex failed with %x\n", hr));
        }
        else
        {
            hr = pContext->pDevice->SetIndices(NULL);
            AssertMsg(hr == D3D_OK, ("SetIndices vertex (NULL) failed with %x\n", hr));
        }

        PVMSVGA3DSURFACE pVertexSurface;
        unsigned         sidVertex = pVertexDecl[0].array.surfaceId;
        unsigned         strideVertex = pVertexDecl[0].array.stride;

        pVertexSurface = pState->papSurfaces[sidVertex];

        if (!pIndexSurface)
        {
            /* Render without an index buffer */
            Log(("DrawPrimitive %x primitivecount=%d index index bias=%d stride=%d\n", PrimitiveTypeD3D, pRange[iPrimitive].primitiveCount,  pRange[iPrimitive].indexBias, strideVertex));

            hr = pContext->pDevice->DrawPrimitive(PrimitiveTypeD3D,
                                                  pRange[iPrimitive].indexBias,
                                                  pRange[iPrimitive].primitiveCount);
            if (hr != D3D_OK)
            {
                AssertMsg(hr == D3D_OK, ("vmsvga3dDrawPrimitives: DrawPrimitive failed with %x\n", hr));
                rc = VERR_INTERNAL_ERROR;
                goto internal_error;
            }
        }
        else
        {
            Assert(pRange[iPrimitive].indexBias >= 0);  /** @todo */

            UINT numVertices;

            if (pVertexDecl[0].rangeHint.last)
                numVertices = pVertexDecl[0].rangeHint.last - pVertexDecl[0].rangeHint.first + 1;
            else
                numVertices = pVertexSurface->pMipmapLevels[0].cbSurface / strideVertex - pVertexDecl[0].array.offset / strideVertex - pVertexDecl[0].rangeHint.first - pRange[iPrimitive].indexBias;

            /* Render with an index buffer */
            Log(("DrawIndexedPrimitive %x startindex=%d numVertices=%d, primitivecount=%d index format=%d index bias=%d stride=%d\n", PrimitiveTypeD3D, pVertexDecl[0].rangeHint.first,  numVertices, pRange[iPrimitive].primitiveCount,  (pRange[iPrimitive].indexWidth == sizeof(uint16_t)) ? D3DFMT_INDEX16 : D3DFMT_INDEX32, pRange[iPrimitive].indexBias, strideVertex));

            hr = pContext->pDevice->DrawIndexedPrimitive(PrimitiveTypeD3D,
                                                         pRange[iPrimitive].indexBias,      /* BaseVertexIndex */
                                                         0,                                 /* MinVertexIndex */
                                                         numVertices,
                                                         pRange[iPrimitive].indexArray.offset / pRange[iPrimitive].indexWidth,    /* StartIndex */
                                                         pRange[iPrimitive].primitiveCount);
            if (hr != D3D_OK)
            {
                AssertMsg(hr == D3D_OK, ("vmsvga3dDrawPrimitives: DrawIndexedPrimitive failed with %x\n", hr));
                rc = VERR_INTERNAL_ERROR;
                goto internal_error;
            }
        }
    }
    D3D_RELEASE(pVertexDeclD3D);
    RTMemFree(pVertexElement);

    hr = pContext->pDevice->EndScene();
    AssertMsgReturn(hr == D3D_OK, ("EndScene failed with %x\n", hr), VERR_INTERNAL_ERROR);

    /* Clear streams above 1 as they might accidentally be reused in the future. */
    if (iCurrentStreamId > 1)
    {
        for (uint32_t i = 1; i < iCurrentStreamId; i++)
        {
            Log(("vmsvga3dDrawPrimitives: clear stream %d\n", i));
            hr = pContext->pDevice->SetStreamSource(i, NULL, 0, 0);
            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dDrawPrimitives: SetStreamSource failed with %x\n", hr), VERR_INTERNAL_ERROR);
        }
    }

    if (cVertexDivisor)
    {
        /* "When you are finished rendering the instance data, be sure to reset the vertex stream frequency back..." */
        for (uint32_t i = 0; i < cVertexDivisor; ++i)
        {
            HRESULT hr2 = pContext->pDevice->SetStreamSourceFreq(i, 1);
            Assert(SUCCEEDED(hr2)); RT_NOREF(hr2);
        }
    }

#if 0 /* Flush queue */
    {
        IDirect3DQuery9        *pQuery;
        hr = pContext->pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &pQuery);
        AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSurfaceTrackUsage: CreateQuery failed with %x\n", hr), VERR_INTERNAL_ERROR);

        hr = pQuery->Issue(D3DISSUE_END);
        AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSurfaceTrackUsage: Issue failed with %x\n", hr), VERR_INTERNAL_ERROR);
        while (true)
        {
            hr = pQuery->GetData(NULL, 0, D3DGETDATA_FLUSH);
            if (hr != S_FALSE) break;

            RTThreadSleep(1);
        }
        AssertMsgReturn(hr == S_OK, ("vmsvga3dSurfaceFinishDrawing: GetData failed with %x\n", hr), VERR_INTERNAL_ERROR);

        D3D_RELEASE(pQuery);
    }
#endif

    /* Make sure we can track drawing usage of active render targets and textures. */
    vmsvga3dContextTrackUsage(pThis, pContext);

#ifdef DEBUG_GFX_WINDOW
    if (pContext->aSidActiveTexture[0] == 0x62)
////    if (pContext->sidActiveTexture == 0x3d)
    {
        SVGA3dCopyRect rect;

        rect.srcx = rect.srcy = rect.x = rect.y = 0;
        rect.w = 800;
        rect.h = 600;
        vmsvga3dCommandPresent(pThis, pContext->sidRenderTarget /*pContext->aSidActiveTexture[0] */, 0, &rect);
    }
#endif
    return rc;

internal_error:
    D3D_RELEASE(pVertexDeclD3D);
    if (pVertexElement)
        RTMemFree(pVertexElement);

    hr = pContext->pDevice->EndScene();
    AssertMsgReturn(hr == D3D_OK, ("EndScene failed with %x\n", hr), VERR_INTERNAL_ERROR);

    return rc;
}

#else /* New vmsvga3dDrawPrimitives */

static int vmsvga3dDrawPrimitivesSyncVertexBuffer(PVMSVGA3DCONTEXT pContext,
                                                  PVMSVGA3DSURFACE pVertexSurface)
{
    HRESULT hr;
    if (   pVertexSurface->u.pSurface
        && pVertexSurface->enmD3DResType != VMSVGA3D_D3DRESTYPE_VERTEX_BUFFER)
    {
        /* The buffer object is not an vertex one. Recreate the D3D resource. */
        Assert(pVertexSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_INDEX_BUFFER);
        D3D_RELEASE(pVertexSurface->u.pIndexBuffer);
        pVertexSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_NONE;

        LogFunc(("index -> vertex buffer sid=%x\n", pVertexSurface->id));
    }

    bool fSync = pVertexSurface->fDirty;
    if (!pVertexSurface->u.pVertexBuffer)
    {
        LogFunc(("Create vertex buffer fDirty=%d\n", pVertexSurface->fDirty));

        const DWORD Usage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY; /* possible severe performance penalty otherwise (according to d3d debug output */
        hr = pContext->pDevice->CreateVertexBuffer(pVertexSurface->pMipmapLevels[0].cbSurface,
                                                   Usage,
                                                   0, /* non-FVF */
                                                   D3DPOOL_DEFAULT,
                                                   &pVertexSurface->u.pVertexBuffer,
                                                   NULL);
        AssertMsgReturn(hr == D3D_OK, ("CreateVertexBuffer failed with %x\n", hr), VERR_INTERNAL_ERROR);

        pVertexSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_VERTEX_BUFFER;
        pVertexSurface->idAssociatedContext = pContext->id;
        pVertexSurface->surfaceFlags |= SVGA3D_SURFACE_HINT_VERTEXBUFFER;
        fSync = true;
    }

    if (fSync)
    {
        LogFunc(("sync vertex buffer\n"));
        Assert(pVertexSurface->u.pVertexBuffer);

        void *pvData;
        hr = pVertexSurface->u.pVertexBuffer->Lock(0, 0, &pvData, D3DLOCK_DISCARD);
        AssertMsgReturn(hr == D3D_OK, ("Lock vertex failed with %x\n", hr), VERR_INTERNAL_ERROR);

        memcpy(pvData, pVertexSurface->pMipmapLevels[0].pSurfaceData, pVertexSurface->pMipmapLevels[0].cbSurface);

        hr = pVertexSurface->u.pVertexBuffer->Unlock();
        AssertMsgReturn(hr == D3D_OK, ("Unlock vertex failed with %x\n", hr), VERR_INTERNAL_ERROR);
    }

    return VINF_SUCCESS;
}


static int vmsvga3dDrawPrimitivesSyncIndexBuffer(PVMSVGA3DCONTEXT pContext,
                                                 PVMSVGA3DSURFACE pIndexSurface,
                                                 uint32_t indexWidth)
{
    HRESULT hr;
    if (   pIndexSurface->u.pSurface
        && pIndexSurface->enmD3DResType != VMSVGA3D_D3DRESTYPE_INDEX_BUFFER)
    {
        /* The buffer object is not an index one. Must recreate the D3D resource. */
        Assert(pIndexSurface->enmD3DResType == VMSVGA3D_D3DRESTYPE_VERTEX_BUFFER);
        D3D_RELEASE(pIndexSurface->u.pVertexBuffer);
        pIndexSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_NONE;

        LogFunc(("vertex -> index buffer sid=%x\n", pIndexSurface->id));
    }

    bool fSync = pIndexSurface->fDirty;
    if (!pIndexSurface->u.pIndexBuffer)
    {
        LogFunc(("Create index buffer fDirty=%d\n", pIndexSurface->fDirty));

        const DWORD Usage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY; /* possible severe performance penalty otherwise (according to d3d debug output */
        const D3DFORMAT Format = (indexWidth == sizeof(uint16_t)) ? D3DFMT_INDEX16 : D3DFMT_INDEX32;
        hr = pContext->pDevice->CreateIndexBuffer(pIndexSurface->pMipmapLevels[0].cbSurface,
                                                  Usage,
                                                  Format,
                                                  D3DPOOL_DEFAULT,
                                                  &pIndexSurface->u.pIndexBuffer,
                                                  NULL);
        AssertMsgReturn(hr == D3D_OK, ("CreateIndexBuffer failed with %x\n", hr), VERR_INTERNAL_ERROR);

        pIndexSurface->enmD3DResType = VMSVGA3D_D3DRESTYPE_INDEX_BUFFER;
        pIndexSurface->idAssociatedContext = pContext->id;
        pIndexSurface->surfaceFlags |= SVGA3D_SURFACE_HINT_INDEXBUFFER;
        fSync = true;
    }

    if (fSync)
    {
        LogFunc(("sync index buffer\n"));
        Assert(pIndexSurface->u.pIndexBuffer);

        void *pvData;
        hr = pIndexSurface->u.pIndexBuffer->Lock(0, 0, &pvData, D3DLOCK_DISCARD);
        AssertMsgReturn(hr == D3D_OK, ("Lock index failed with %x\n", hr), VERR_INTERNAL_ERROR);

        memcpy(pvData, pIndexSurface->pMipmapLevels[0].pSurfaceData, pIndexSurface->pMipmapLevels[0].cbSurface);

        hr = pIndexSurface->u.pIndexBuffer->Unlock();
        AssertMsgReturn(hr == D3D_OK, ("Unlock index failed with %x\n", hr), VERR_INTERNAL_ERROR);
    }

    return VINF_SUCCESS;
}

static int vmsvga3dDrawPrimitivesProcessVertexDecls(const uint32_t numVertexDecls,
                                                    const SVGA3dVertexDecl *pVertexDecl,
                                                    const uint32_t idStream,
                                                    const uint32_t uVertexMinOffset,
                                                    const uint32_t uVertexMaxOffset,
                                                    D3DVERTEXELEMENT9 *pVertexElement)
{
    RT_NOREF(uVertexMaxOffset); /* Logging only. */
    Assert(numVertexDecls);

    /* Create a vertex declaration array */
    for (uint32_t iVertex = 0; iVertex < numVertexDecls; ++iVertex)
    {
        LogFunc(("vertex %d type=%s (%d) method=%s (%d) usage=%s (%d) usageIndex=%d stride=%d offset=%d (%d min=%d max=%d)\n",
                 iVertex,
                 vmsvgaDeclType2String(pVertexDecl[iVertex].identity.type), pVertexDecl[iVertex].identity.type,
                 vmsvgaDeclMethod2String(pVertexDecl[iVertex].identity.method), pVertexDecl[iVertex].identity.method,
                 vmsvgaDeclUsage2String(pVertexDecl[iVertex].identity.usage), pVertexDecl[iVertex].identity.usage,
                 pVertexDecl[iVertex].identity.usageIndex,
                 pVertexDecl[iVertex].array.stride,
                 pVertexDecl[iVertex].array.offset - uVertexMinOffset,
                 pVertexDecl[iVertex].array.offset,
                 uVertexMinOffset, uVertexMaxOffset));

        int rc = vmsvga3dVertexDecl2D3D(pVertexDecl[iVertex].identity, &pVertexElement[iVertex]);
        AssertRCReturn(rc, rc);

        pVertexElement[iVertex].Stream = idStream;
        pVertexElement[iVertex].Offset = pVertexDecl[iVertex].array.offset - uVertexMinOffset;

#ifdef LOG_ENABLED
        if (pVertexDecl[iVertex].array.stride == 0)
            LogFunc(("stride == 0! Can be valid\n"));

        if (pVertexElement[iVertex].Offset >= pVertexDecl[0].array.stride)
            LogFunc(("WARNING: offset > stride!!\n"));
#endif
    }

    return VINF_SUCCESS;
}

int vmsvga3dDrawPrimitives(PVGASTATE pThis, uint32_t cid, uint32_t numVertexDecls, SVGA3dVertexDecl *pVertexDecl,
                           uint32_t numRanges, SVGA3dPrimitiveRange *pRange,
                           uint32_t cVertexDivisor, SVGA3dVertexDivisor *pVertexDivisor)
{
    static const D3DVERTEXELEMENT9 sVertexEnd = D3DDECL_END();

    PVMSVGA3DSTATE pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_INTERNAL_ERROR);

    PVMSVGA3DCONTEXT pContext;
    int rc = vmsvga3dContextFromCid(pState, cid, &pContext);
    AssertRCReturn(rc, rc);

    HRESULT hr;

    /* SVGA driver may use the same surface for both index and vertex data. So we can not clear fDirty flag,
     * after updating a vertex buffer for example, because the same surface might be used for index buffer later.
     * So keep pointers to all used surfaces in the following two arrays and clear fDirty flag at the end.
     */
    PVMSVGA3DSURFACE aVertexSurfaces[SVGA3D_MAX_VERTEX_ARRAYS];
    PVMSVGA3DSURFACE aIndexSurfaces[SVGA3D_MAX_DRAW_PRIMITIVE_RANGES];
    RT_ZERO(aVertexSurfaces);
    RT_ZERO(aIndexSurfaces);

    LogFunc(("cid=%x numVertexDecls=%d numRanges=%d, cVertexDivisor=%d\n", cid, numVertexDecls, numRanges, cVertexDivisor));

    AssertReturn(numVertexDecls && numVertexDecls <= SVGA3D_MAX_VERTEX_ARRAYS, VERR_INVALID_PARAMETER);
    AssertReturn(numRanges && numRanges <= SVGA3D_MAX_DRAW_PRIMITIVE_RANGES, VERR_INVALID_PARAMETER);
    AssertReturn(!cVertexDivisor || cVertexDivisor == numVertexDecls, VERR_INVALID_PARAMETER);

    /*
     * Process all vertex declarations. Each vertex buffer surface is represented by one stream source id.
     */
    D3DVERTEXELEMENT9 aVertexElements[SVGA3D_MAX_VERTEX_ARRAYS + 1];

    uint32_t iCurrentVertex   = 0;
    uint32_t iCurrentStreamId = 0;
    while (iCurrentVertex < numVertexDecls)
    {
        const uint32_t sidVertex = pVertexDecl[iCurrentVertex].array.surfaceId;
        const uint32_t strideVertex = pVertexDecl[iCurrentVertex].array.stride;

        PVMSVGA3DSURFACE pVertexSurface;
        rc = vmsvga3dSurfaceFromSid(pState, sidVertex, &pVertexSurface);
        AssertRCBreak(rc);

        rc = vmsvga3dDrawPrimitivesSyncVertexBuffer(pContext, pVertexSurface);
        AssertRCBreak(rc);

        uint32_t uVertexMinOffset = UINT32_MAX;
        uint32_t uVertexMaxOffset = 0;

        uint32_t iVertex;
        for (iVertex = iCurrentVertex; iVertex < numVertexDecls; ++iVertex)
        {
            /* Remember, so we can mark it as not dirty later. */
            aVertexSurfaces[iVertex] = pVertexSurface;

            /* New surface id -> new stream id. */
            if (pVertexDecl[iVertex].array.surfaceId != sidVertex)
                break;

            const uint32_t uVertexOffset = pVertexDecl[iVertex].array.offset;
            const uint32_t uNewVertexMinOffset = RT_MIN(uVertexMinOffset, uVertexOffset);
            const uint32_t uNewVertexMaxOffset = RT_MAX(uVertexMaxOffset, uVertexOffset);

            /* We must put vertex declarations that start at a different element in another stream as d3d only handles offsets < stride. */
            if (   uNewVertexMaxOffset - uNewVertexMinOffset >= strideVertex
                && strideVertex != 0)
                break;

            uVertexMinOffset = uNewVertexMinOffset;
            uVertexMaxOffset = uNewVertexMaxOffset;
        }

        rc = vmsvga3dDrawPrimitivesProcessVertexDecls(iVertex - iCurrentVertex,
                                                      &pVertexDecl[iCurrentVertex],
                                                      iCurrentStreamId,
                                                      uVertexMinOffset,
                                                      uVertexMaxOffset,
                                                      &aVertexElements[iCurrentVertex]);
        AssertRCBreak(rc);

        LogFunc(("SetStreamSource vertex sid=%x stream %d min offset=%d stride=%d\n",
                 pVertexSurface->id, iCurrentStreamId, uVertexMinOffset, strideVertex));

        hr = pContext->pDevice->SetStreamSource(iCurrentStreamId,
                                                pVertexSurface->u.pVertexBuffer,
                                                uVertexMinOffset,
                                                strideVertex);
        AssertMsgBreakStmt(hr == D3D_OK, ("SetStreamSource failed with %x\n", hr), rc = VERR_INTERNAL_ERROR);

        if (cVertexDivisor)
        {
            LogFunc(("SetStreamSourceFreq[%d]=%x\n", iCurrentStreamId, pVertexDivisor[iCurrentStreamId].value));
            HRESULT hr2 = pContext->pDevice->SetStreamSourceFreq(iCurrentStreamId,
                                                                 pVertexDivisor[iCurrentStreamId].value);
            Assert(SUCCEEDED(hr2)); RT_NOREF(hr2);
        }

        iCurrentVertex = iVertex;
        ++iCurrentStreamId;
    }

    /* iCurrentStreamId is equal to the total number of streams and the value is used for cleanup at the function end. */

    AssertRCReturn(rc, rc);

    /* Mark the end. */
    memcpy(&aVertexElements[numVertexDecls], &sVertexEnd, sizeof(sVertexEnd));

    /* Create and set the vertex declaration. */
    IDirect3DVertexDeclaration9 *pVertexDeclD3D = NULL;
    hr = pContext->pDevice->CreateVertexDeclaration(&aVertexElements[0], &pVertexDeclD3D);
    AssertMsgReturn(hr == D3D_OK, ("CreateVertexDeclaration failed with %x\n", hr), VERR_INTERNAL_ERROR);

    hr = pContext->pDevice->SetVertexDeclaration(pVertexDeclD3D);
    AssertMsgReturnStmt(hr == D3D_OK, ("SetVertexDeclaration failed with %x\n", hr),
                        D3D_RELEASE(pVertexDeclD3D), VERR_INTERNAL_ERROR);

    /* Begin a scene before rendering anything. */
    hr = pContext->pDevice->BeginScene();
    AssertMsgReturnStmt(hr == D3D_OK, ("BeginScene failed with %x\n", hr),
                        D3D_RELEASE(pVertexDeclD3D), VERR_INTERNAL_ERROR);

    /* Now draw the primitives. */
    for (uint32_t iPrimitive = 0; iPrimitive < numRanges; ++iPrimitive)
    {
        Log(("Primitive %d: type %s\n", iPrimitive, vmsvga3dPrimitiveType2String(pRange[iPrimitive].primType)));

        const uint32_t sidIndex = pRange[iPrimitive].indexArray.surfaceId;
        PVMSVGA3DSURFACE pIndexSurface = NULL;

        D3DPRIMITIVETYPE PrimitiveTypeD3D;
        rc = vmsvga3dPrimitiveType2D3D(pRange[iPrimitive].primType, &PrimitiveTypeD3D);
        AssertRCBreak(rc);

        /* Triangle strips or fans with just one primitive don't make much sense and are identical to triangle lists.
         * Workaround for NVidia driver crash when encountering some of these.
         */
        if (    pRange[iPrimitive].primitiveCount == 1
            &&  (   PrimitiveTypeD3D == D3DPT_TRIANGLESTRIP
                 || PrimitiveTypeD3D == D3DPT_TRIANGLEFAN))
            PrimitiveTypeD3D = D3DPT_TRIANGLELIST;

        if (sidIndex != SVGA3D_INVALID_ID)
        {
            AssertMsg(pRange[iPrimitive].indexWidth == sizeof(uint32_t) || pRange[iPrimitive].indexWidth == sizeof(uint16_t),
                      ("Unsupported primitive width %d\n", pRange[iPrimitive].indexWidth));

            rc = vmsvga3dSurfaceFromSid(pState, sidIndex, &pIndexSurface);
            AssertRCBreak(rc);

            aIndexSurfaces[iPrimitive] = pIndexSurface;

            Log(("vmsvga3dDrawPrimitives: index sid=%x\n", sidIndex));

            rc = vmsvga3dDrawPrimitivesSyncIndexBuffer(pContext, pIndexSurface, pRange[iPrimitive].indexWidth);
            AssertRCBreak(rc);

            hr = pContext->pDevice->SetIndices(pIndexSurface->u.pIndexBuffer);
            AssertMsg(hr == D3D_OK, ("SetIndices vertex failed with %x\n", hr));
        }
        else
        {
            hr = pContext->pDevice->SetIndices(NULL);
            AssertMsg(hr == D3D_OK, ("SetIndices vertex (NULL) failed with %x\n", hr));
        }

        const uint32_t strideVertex = pVertexDecl[0].array.stride;

        if (!pIndexSurface)
        {
            /* Render without an index buffer */
            Log(("DrawPrimitive %x primitivecount=%d index index bias=%d stride=%d\n",
                 PrimitiveTypeD3D, pRange[iPrimitive].primitiveCount,  pRange[iPrimitive].indexBias, strideVertex));

            hr = pContext->pDevice->DrawPrimitive(PrimitiveTypeD3D,
                                                  pRange[iPrimitive].indexBias,
                                                  pRange[iPrimitive].primitiveCount);
            AssertMsgBreakStmt(hr == D3D_OK, ("DrawPrimitive failed with %x\n", hr), rc = VERR_INTERNAL_ERROR);
        }
        else
        {
            Assert(pRange[iPrimitive].indexBias >= 0);  /** @todo */

            UINT numVertices;
            if (pVertexDecl[0].rangeHint.last)
            {
                /* Both SVGA3dArrayRangeHint definition and the SVGA driver code imply that 'last' is exclusive,
                 * hence compute the difference.
                 */
                numVertices = pVertexDecl[0].rangeHint.last - pVertexDecl[0].rangeHint.first;
            }
            else
            {
                /* Range hint is not provided. */
                PVMSVGA3DSURFACE pVertexSurface = aVertexSurfaces[0];
                numVertices =   pVertexSurface->pMipmapLevels[0].cbSurface / strideVertex
                              - pVertexDecl[0].array.offset / strideVertex
                              - pVertexDecl[0].rangeHint.first
                              - pRange[iPrimitive].indexBias;
            }

            /* Render with an index buffer */
            Log(("DrawIndexedPrimitive %x startindex=%d numVertices=%d, primitivecount=%d index format=%s index bias=%d stride=%d\n",
                 PrimitiveTypeD3D, pVertexDecl[0].rangeHint.first,  numVertices, pRange[iPrimitive].primitiveCount,
                 (pRange[iPrimitive].indexWidth == sizeof(uint16_t)) ? "D3DFMT_INDEX16" : "D3DFMT_INDEX32",
                 pRange[iPrimitive].indexBias, strideVertex));

            hr = pContext->pDevice->DrawIndexedPrimitive(PrimitiveTypeD3D,
                                                         pRange[iPrimitive].indexBias,      /* BaseVertexIndex */
                                                         0,                                 /* MinVertexIndex */
                                                         numVertices,
                                                         pRange[iPrimitive].indexArray.offset / pRange[iPrimitive].indexWidth,    /* StartIndex */
                                                         pRange[iPrimitive].primitiveCount);
            AssertMsgBreakStmt(hr == D3D_OK, ("DrawIndexedPrimitive failed with %x\n", hr), rc = VERR_INTERNAL_ERROR);
        }
    }

    /* Release vertex declaration, end the scene and do some cleanup regardless of the rc. */
    D3D_RELEASE(pVertexDeclD3D);

    hr = pContext->pDevice->EndScene();
    AssertMsgReturn(hr == D3D_OK, ("EndScene failed with %x\n", hr), VERR_INTERNAL_ERROR);

    /* Cleanup. */
    uint32_t i;
    /* Clear streams above 1 as they might accidentally be reused in the future. */
    for (i = 1; i < iCurrentStreamId; ++i)
    {
        LogFunc(("clear stream %d\n", i));
        HRESULT hr2 = pContext->pDevice->SetStreamSource(i, NULL, 0, 0);
        AssertMsg(hr2 == D3D_OK, ("SetStreamSource(%d, NULL) failed with %x\n", i, hr2)); RT_NOREF(hr2);
    }

    if (cVertexDivisor)
    {
        /* "When you are finished rendering the instance data, be sure to reset the vertex stream frequency back..." */
        for (i = 0; i < iCurrentStreamId; ++i)
        {
            LogFunc(("reset stream freq %d\n", i));
            HRESULT hr2 = pContext->pDevice->SetStreamSourceFreq(i, 1);
            AssertMsg(hr2 == D3D_OK, ("SetStreamSourceFreq(%d, 1) failed with %x\n", i, hr2)); RT_NOREF(hr2);
        }
    }

    if (RT_SUCCESS(rc))
    {
        for (i = 0; i < numVertexDecls; ++i)
        {
            if (aVertexSurfaces[i])
            {
                aVertexSurfaces[i]->pMipmapLevels[0].fDirty = false;
                aVertexSurfaces[i]->fDirty = false;
            }
        }

        for (i = 0; i < numRanges; ++i)
        {
            if (aIndexSurfaces[i])
            {
                aIndexSurfaces[i]->pMipmapLevels[0].fDirty = false;
                aIndexSurfaces[i]->fDirty = false;
            }
        }

        /* Make sure we can track drawing usage of active render targets and textures. */
        vmsvga3dContextTrackUsage(pThis, pContext);
    }

    return rc;
}

#endif /* New vmsvga3dDrawPrimitives */

int vmsvga3dSetScissorRect(PVGASTATE pThis, uint32_t cid, SVGA3dRect *pRect)
{
    HRESULT               hr;
    RECT                  rect;
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dSetScissorRect %x (%d,%d)(%d,%d)\n", cid, pRect->x, pRect->y, pRect->w, pRect->h));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dSetScissorRect invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];

    /* Store for vm state save/restore. */
    pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_SCISSORRECT;
    pContext->state.RectScissor = *pRect;

    rect.left   = pRect->x;
    rect.top    = pRect->y;
    rect.right  = rect.left + pRect->w;  /* exclusive */
    rect.bottom = rect.top + pRect->h;   /* exclusive */

    hr = pContext->pDevice->SetScissorRect(&rect);
    AssertMsgReturn(hr == D3D_OK, ("vmsvga3dSetScissorRect: SetScissorRect failed with %x\n", hr), VERR_INTERNAL_ERROR);

    return VINF_SUCCESS;
}


int vmsvga3dShaderDefine(PVGASTATE pThis, uint32_t cid, uint32_t shid, SVGA3dShaderType type, uint32_t cbData, uint32_t *pShaderData)
{
    HRESULT               hr;
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSHADER       pShader;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dShaderDefine %x shid=%x type=%s cbData=%x\n", cid, shid, (type == SVGA3D_SHADERTYPE_VS) ? "VERTEX" : "PIXEL", cbData));
#ifdef LOG_ENABLED
    Log3(("Shader code:\n"));
    const uint32_t cTokensPerLine = 8;
    const uint32_t *paTokens = (uint32_t *)pShaderData;
    const uint32_t cTokens = cbData / sizeof(uint32_t);
    for (uint32_t iToken = 0; iToken < cTokens; ++iToken)
    {
        if ((iToken % cTokensPerLine) == 0)
        {
            if (iToken == 0)
                Log3(("0x%08X,", paTokens[iToken]));
            else
                Log3(("\n0x%08X,", paTokens[iToken]));
        }
        else
            Log3((" 0x%08X,", paTokens[iToken]));
    }
    Log3(("\n"));
#endif

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dShaderDefine invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];

    AssertReturn(shid < SVGA3D_MAX_SHADER_IDS, VERR_INVALID_PARAMETER);
    if (type == SVGA3D_SHADERTYPE_VS)
    {
        if (shid >= pContext->cVertexShaders)
        {
            void *pvNew = RTMemRealloc(pContext->paVertexShader, sizeof(VMSVGA3DSHADER) * (shid + 1));
            AssertReturn(pvNew, VERR_NO_MEMORY);
            pContext->paVertexShader = (PVMSVGA3DSHADER)pvNew;
            memset(&pContext->paVertexShader[pContext->cVertexShaders], 0, sizeof(VMSVGA3DSHADER) * (shid + 1 - pContext->cVertexShaders));
            for (uint32_t i = pContext->cVertexShaders; i < shid + 1; i++)
                pContext->paVertexShader[i].id = SVGA3D_INVALID_ID;
            pContext->cVertexShaders = shid + 1;
        }
        /* If one already exists with this id, then destroy it now. */
        if (pContext->paVertexShader[shid].id != SVGA3D_INVALID_ID)
            vmsvga3dShaderDestroy(pThis, cid, shid, pContext->paVertexShader[shid].type);

        pShader = &pContext->paVertexShader[shid];
    }
    else
    {
        Assert(type == SVGA3D_SHADERTYPE_PS);
        if (shid >= pContext->cPixelShaders)
        {
            void *pvNew = RTMemRealloc(pContext->paPixelShader, sizeof(VMSVGA3DSHADER) * (shid + 1));
            AssertReturn(pvNew, VERR_NO_MEMORY);
            pContext->paPixelShader = (PVMSVGA3DSHADER)pvNew;
            memset(&pContext->paPixelShader[pContext->cPixelShaders], 0, sizeof(VMSVGA3DSHADER) * (shid + 1 - pContext->cPixelShaders));
            for (uint32_t i = pContext->cPixelShaders; i < shid + 1; i++)
                pContext->paPixelShader[i].id = SVGA3D_INVALID_ID;
            pContext->cPixelShaders = shid + 1;
        }
        /* If one already exists with this id, then destroy it now. */
        if (pContext->paPixelShader[shid].id != SVGA3D_INVALID_ID)
            vmsvga3dShaderDestroy(pThis, cid, shid, pContext->paPixelShader[shid].type);

        pShader = &pContext->paPixelShader[shid];
    }

    memset(pShader, 0, sizeof(*pShader));
    pShader->id     = shid;
    pShader->cid    = cid;
    pShader->type   = type;
    pShader->cbData = cbData;
    pShader->pShaderProgram = RTMemAllocZ(cbData);
    AssertReturn(pShader->pShaderProgram, VERR_NO_MEMORY);
    memcpy(pShader->pShaderProgram, pShaderData, cbData);

#ifdef DUMP_SHADER_DISASSEMBLY
    LPD3DXBUFFER pDisassembly;
    hr = D3DXDisassembleShader((const DWORD *)pShaderData, FALSE, NULL, &pDisassembly);
    if (hr == D3D_OK)
    {
        Log(("Shader disassembly:\n%s\n", pDisassembly->GetBufferPointer()));
        D3D_RELEASE(pDisassembly);
    }
#endif

    switch (type)
    {
    case SVGA3D_SHADERTYPE_VS:
        hr = pContext->pDevice->CreateVertexShader((const DWORD *)pShaderData, &pShader->u.pVertexShader);
        break;

    case SVGA3D_SHADERTYPE_PS:
        hr = pContext->pDevice->CreatePixelShader((const DWORD *)pShaderData, &pShader->u.pPixelShader);
        break;

    default:
        AssertFailedReturn(VERR_INVALID_PARAMETER);
    }
    if (hr != D3D_OK)
    {
        RTMemFree(pShader->pShaderProgram);
        memset(pShader, 0, sizeof(*pShader));
        pShader->id = SVGA3D_INVALID_ID;
    }

    AssertMsgReturn(hr == D3D_OK, ("vmsvga3dShaderDefine: CreateVertex/PixelShader failed with %x\n", hr), VERR_INTERNAL_ERROR);
    return VINF_SUCCESS;
}

int vmsvga3dShaderDestroy(PVGASTATE pThis, uint32_t cid, uint32_t shid, SVGA3dShaderType type)
{
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    PVMSVGA3DSHADER       pShader = NULL;

    Log(("vmsvga3dShaderDestroy %x shid=%x type=%s\n", cid, shid, (type == SVGA3D_SHADERTYPE_VS) ? "VERTEX" : "PIXEL"));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dShaderDestroy invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];

    if (type == SVGA3D_SHADERTYPE_VS)
    {
        if (    shid < pContext->cVertexShaders
            &&  pContext->paVertexShader[shid].id == shid)
        {
            pShader = &pContext->paVertexShader[shid];
            D3D_RELEASE(pShader->u.pVertexShader);
        }
    }
    else
    {
        Assert(type == SVGA3D_SHADERTYPE_PS);
        if (    shid < pContext->cPixelShaders
            &&  pContext->paPixelShader[shid].id == shid)
        {
            pShader = &pContext->paPixelShader[shid];
            D3D_RELEASE(pShader->u.pPixelShader);
        }
    }

    if (pShader)
    {
        if (pShader->pShaderProgram)
            RTMemFree(pShader->pShaderProgram);

        memset(pShader, 0, sizeof(*pShader));
        pShader->id = SVGA3D_INVALID_ID;
    }
    else
        AssertFailedReturn(VERR_INVALID_PARAMETER);

    return VINF_SUCCESS;
}

int vmsvga3dShaderSet(PVGASTATE pThis, PVMSVGA3DCONTEXT pContext, uint32_t cid, SVGA3dShaderType type, uint32_t shid)
{
    PVMSVGA3DSTATE      pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);
    HRESULT             hr;

    Log(("vmsvga3dShaderSet %x type=%s shid=%d\n", cid, (type == SVGA3D_SHADERTYPE_VS) ? "VERTEX" : "PIXEL", shid));

    NOREF(pContext);
    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dShaderSet invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];

    if (type == SVGA3D_SHADERTYPE_VS)
    {
        /* Save for vm state save/restore. */
        pContext->state.shidVertex = shid;
        pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_VERTEXSHADER;

        if (    shid < pContext->cVertexShaders
            &&  pContext->paVertexShader[shid].id == shid)
        {
            PVMSVGA3DSHADER pShader = &pContext->paVertexShader[shid];
            Assert(type == pShader->type);

            hr = pContext->pDevice->SetVertexShader(pShader->u.pVertexShader);
            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dShaderSet: SetVertex/PixelShader failed with %x\n", hr), VERR_INTERNAL_ERROR);
        }
        else
        if (shid == SVGA_ID_INVALID)
        {
            /* Unselect shader. */
            hr = pContext->pDevice->SetVertexShader(NULL);
            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dShaderSet: SetVertex/PixelShader failed with %x\n", hr), VERR_INTERNAL_ERROR);
        }
        else
            AssertFailedReturn(VERR_INVALID_PARAMETER);
    }
    else
    {
        /* Save for vm state save/restore. */
        pContext->state.shidPixel = shid;
        pContext->state.u32UpdateFlags |= VMSVGA3D_UPDATE_PIXELSHADER;

        Assert(type == SVGA3D_SHADERTYPE_PS);
        if (    shid < pContext->cPixelShaders
            &&  pContext->paPixelShader[shid].id == shid)
        {
            PVMSVGA3DSHADER pShader = &pContext->paPixelShader[shid];
            Assert(type == pShader->type);

            hr = pContext->pDevice->SetPixelShader(pShader->u.pPixelShader);
            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dShaderSet: SetVertex/PixelShader failed with %x\n", hr), VERR_INTERNAL_ERROR);
        }
        else
        if (shid == SVGA_ID_INVALID)
        {
            /* Unselect shader. */
            hr = pContext->pDevice->SetPixelShader(NULL);
            AssertMsgReturn(hr == D3D_OK, ("vmsvga3dShaderSet: SetVertex/PixelShader failed with %x\n", hr), VERR_INTERNAL_ERROR);
        }
        else
            AssertFailedReturn(VERR_INVALID_PARAMETER);
    }

    return VINF_SUCCESS;
}

int vmsvga3dShaderSetConst(PVGASTATE pThis, uint32_t cid, uint32_t reg, SVGA3dShaderType type, SVGA3dShaderConstType ctype, uint32_t cRegisters, uint32_t *pValues)
{
    HRESULT               hr;
    PVMSVGA3DCONTEXT      pContext;
    PVMSVGA3DSTATE        pState = pThis->svga.p3dState;
    AssertReturn(pState, VERR_NO_MEMORY);

    Log(("vmsvga3dShaderSetConst %x reg=%x type=%s ctype=%x\n", cid, reg, (type == SVGA3D_SHADERTYPE_VS) ? "VERTEX" : "PIXEL", ctype));

    if (    cid >= pState->cContexts
        ||  pState->papContexts[cid]->id != cid)
    {
        Log(("vmsvga3dShaderSetConst invalid context id!\n"));
        return VERR_INVALID_PARAMETER;
    }
    pContext = pState->papContexts[cid];

    for (uint32_t i = 0; i < cRegisters; i++)
    {
        Log(("Constant %d: value=%x-%x-%x-%x\n", reg + i, pValues[i*4 + 0], pValues[i*4 + 1], pValues[i*4 + 2], pValues[i*4 + 3]));
        vmsvga3dSaveShaderConst(pContext, reg + i, type, ctype, pValues[i*4 + 0], pValues[i*4 + 1], pValues[i*4 + 2], pValues[i*4 + 3]);
    }

    switch (type)
    {
    case SVGA3D_SHADERTYPE_VS:
        switch (ctype)
        {
        case SVGA3D_CONST_TYPE_FLOAT:
            hr = pContext->pDevice->SetVertexShaderConstantF(reg, (const float *)pValues, cRegisters);
            break;

        case SVGA3D_CONST_TYPE_INT:
            hr = pContext->pDevice->SetVertexShaderConstantI(reg, (const int *)pValues, cRegisters);
            break;

        case SVGA3D_CONST_TYPE_BOOL:
            hr = pContext->pDevice->SetVertexShaderConstantB(reg, (const BOOL *)pValues, cRegisters);
            break;

        default:
            AssertFailedReturn(VERR_INVALID_PARAMETER);
        }
        AssertMsgReturn(hr == D3D_OK, ("vmsvga3dShaderSetConst: SetVertexShader failed with %x\n", hr), VERR_INTERNAL_ERROR);
        break;

    case SVGA3D_SHADERTYPE_PS:
        switch (ctype)
        {
        case SVGA3D_CONST_TYPE_FLOAT:
        {
            hr = pContext->pDevice->SetPixelShaderConstantF(reg, (const float *)pValues, cRegisters);
            break;
        }

        case SVGA3D_CONST_TYPE_INT:
            hr = pContext->pDevice->SetPixelShaderConstantI(reg, (const int *)pValues, cRegisters);
            break;

        case SVGA3D_CONST_TYPE_BOOL:
            hr = pContext->pDevice->SetPixelShaderConstantB(reg, (const BOOL *)pValues, cRegisters);
            break;

        default:
            AssertFailedReturn(VERR_INVALID_PARAMETER);
        }
        AssertMsgReturn(hr == D3D_OK, ("vmsvga3dShaderSetConst: SetPixelShader failed with %x\n", hr), VERR_INTERNAL_ERROR);
        break;

    default:
        AssertFailedReturn(VERR_INVALID_PARAMETER);
    }
    return VINF_SUCCESS;
}

int vmsvga3dOcclusionQueryCreate(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext)
{
    RT_NOREF(pState);
    HRESULT hr = pContext->pDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, &pContext->occlusion.pQuery);
    AssertMsgReturn(hr == D3D_OK, ("CreateQuery(D3DQUERYTYPE_OCCLUSION) failed with %x\n", hr), VERR_INTERNAL_ERROR);
    return VINF_SUCCESS;
}

int vmsvga3dOcclusionQueryDelete(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext)
{
    RT_NOREF(pState);
    D3D_RELEASE(pContext->occlusion.pQuery);
    return VINF_SUCCESS;
}

int vmsvga3dOcclusionQueryBegin(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext)
{
    RT_NOREF(pState);
    HRESULT hr = pContext->occlusion.pQuery->Issue(D3DISSUE_BEGIN);
    AssertMsgReturnStmt(hr == D3D_OK, ("D3DISSUE_BEGIN(D3DQUERYTYPE_OCCLUSION) failed with %x\n", hr),
                        D3D_RELEASE(pContext->occlusion.pQuery), VERR_INTERNAL_ERROR);
    return VINF_SUCCESS;
}

int vmsvga3dOcclusionQueryEnd(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext)
{
    RT_NOREF(pState);
    HRESULT hr = pContext->occlusion.pQuery->Issue(D3DISSUE_END);
    AssertMsgReturnStmt(hr == D3D_OK, ("D3DISSUE_END(D3DQUERYTYPE_OCCLUSION) failed with %x\n", hr),
                        D3D_RELEASE(pContext->occlusion.pQuery), VERR_INTERNAL_ERROR);
    return VINF_SUCCESS;
}

int vmsvga3dOcclusionQueryGetData(PVMSVGA3DSTATE pState, PVMSVGA3DCONTEXT pContext, uint32_t *pu32Pixels)
{
    RT_NOREF(pState);
    HRESULT hr = D3D_OK;
    /* Wait until the data becomes available. */
    DWORD dwPixels = 0;
    do
    {
        hr = pContext->occlusion.pQuery->GetData((void *)&dwPixels, sizeof(DWORD), D3DGETDATA_FLUSH);
    } while (hr == S_FALSE);

    AssertMsgReturnStmt(hr == D3D_OK, ("GetData(D3DQUERYTYPE_OCCLUSION) failed with %x\n", hr),
                        D3D_RELEASE(pContext->occlusion.pQuery), VERR_INTERNAL_ERROR);

    LogFunc(("Query result: dwPixels %d\n", dwPixels));
    *pu32Pixels = dwPixels;
    return VINF_SUCCESS;
}

static void vmsvgaDumpD3DCaps(D3DCAPS9 *pCaps)
{
    bool const fBufferingSaved = RTLogRelSetBuffering(true /*fBuffered*/);

    LogRel(("\nD3D device caps: DevCaps2:\n"));
    if (pCaps->DevCaps2 & D3DDEVCAPS2_ADAPTIVETESSRTPATCH)
        LogRel((" - D3DDEVCAPS2_ADAPTIVETESSRTPATCH\n"));
    if (pCaps->DevCaps2 & D3DDEVCAPS2_ADAPTIVETESSNPATCH)
        LogRel((" - D3DDEVCAPS2_ADAPTIVETESSNPATCH\n"));
    if (pCaps->DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES)
        LogRel((" - D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES\n"));
    if (pCaps->DevCaps2 & D3DDEVCAPS2_DMAPNPATCH)
        LogRel((" - D3DDEVCAPS2_DMAPNPATCH\n"));
    if (pCaps->DevCaps2 & D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH)
        LogRel((" - D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH\n"));
    if (pCaps->DevCaps2 & D3DDEVCAPS2_STREAMOFFSET)
        LogRel((" - D3DDEVCAPS2_STREAMOFFSET\n"));
    if (pCaps->DevCaps2 & D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET)
        LogRel((" - D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET\n"));

    LogRel(("\nCaps2:\n"));
    if (pCaps->Caps2 & D3DCAPS2_CANAUTOGENMIPMAP)
        LogRel((" - D3DCAPS2_CANAUTOGENMIPMAP\n"));
    if (pCaps->Caps2 & D3DCAPS2_CANCALIBRATEGAMMA)
        LogRel((" - D3DCAPS2_CANCALIBRATEGAMMA\n"));
    if (pCaps->Caps2 & D3DCAPS2_CANSHARERESOURCE)
        LogRel((" - D3DCAPS2_CANSHARERESOURCE\n"));
    if (pCaps->Caps2 & D3DCAPS2_CANMANAGERESOURCE)
        LogRel((" - D3DCAPS2_CANMANAGERESOURCE\n"));
    if (pCaps->Caps2 & D3DCAPS2_DYNAMICTEXTURES)
        LogRel((" - D3DCAPS2_DYNAMICTEXTURES\n"));
    if (pCaps->Caps2 & D3DCAPS2_FULLSCREENGAMMA)
        LogRel((" - D3DCAPS2_FULLSCREENGAMMA\n"));

    LogRel(("\nCaps3:\n"));
    if (pCaps->Caps3 & D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD)
        LogRel((" - D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD\n"));
    if (pCaps->Caps3 & D3DCAPS3_COPY_TO_VIDMEM)
        LogRel((" - D3DCAPS3_COPY_TO_VIDMEM\n"));
    if (pCaps->Caps3 & D3DCAPS3_COPY_TO_SYSTEMMEM)
        LogRel((" - D3DCAPS3_COPY_TO_SYSTEMMEM\n"));
    if (pCaps->Caps3 & D3DCAPS3_DXVAHD)
        LogRel((" - D3DCAPS3_DXVAHD\n"));
    if (pCaps->Caps3 & D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION)
        LogRel((" - D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION\n"));

    LogRel(("\nPresentationIntervals:\n"));
    if (pCaps->PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE)
        LogRel((" - D3DPRESENT_INTERVAL_IMMEDIATE\n"));
    if (pCaps->PresentationIntervals & D3DPRESENT_INTERVAL_ONE)
        LogRel((" - D3DPRESENT_INTERVAL_ONE\n"));
    if (pCaps->PresentationIntervals & D3DPRESENT_INTERVAL_TWO)
        LogRel((" - D3DPRESENT_INTERVAL_TWO\n"));
    if (pCaps->PresentationIntervals & D3DPRESENT_INTERVAL_THREE)
        LogRel((" - D3DPRESENT_INTERVAL_THREE\n"));
    if (pCaps->PresentationIntervals & D3DPRESENT_INTERVAL_FOUR)
        LogRel((" - D3DPRESENT_INTERVAL_FOUR\n"));

    LogRel(("\nDevcaps:\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_CANBLTSYSTONONLOCAL)
        LogRel((" - D3DDEVCAPS_CANBLTSYSTONONLOCAL\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_CANRENDERAFTERFLIP)
        LogRel((" - D3DDEVCAPS_CANRENDERAFTERFLIP\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_DRAWPRIMITIVES2)
        LogRel((" - D3DDEVCAPS_DRAWPRIMITIVES2\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_DRAWPRIMITIVES2EX)
        LogRel((" - D3DDEVCAPS_DRAWPRIMITIVES2EX\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_DRAWPRIMTLVERTEX)
        LogRel((" - D3DDEVCAPS_DRAWPRIMTLVERTEX\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_EXECUTESYSTEMMEMORY)
        LogRel((" - D3DDEVCAPS_EXECUTESYSTEMMEMORY\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_EXECUTEVIDEOMEMORY)
        LogRel((" - D3DDEVCAPS_EXECUTEVIDEOMEMORY\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_HWRASTERIZATION)
        LogRel((" - D3DDEVCAPS_HWRASTERIZATION\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
        LogRel((" - D3DDEVCAPS_HWTRANSFORMANDLIGHT\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_NPATCHES)
        LogRel((" - D3DDEVCAPS_NPATCHES\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_PUREDEVICE)
        LogRel((" - D3DDEVCAPS_PUREDEVICE\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_QUINTICRTPATCHES)
        LogRel((" - D3DDEVCAPS_QUINTICRTPATCHES\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_RTPATCHES)
        LogRel((" - D3DDEVCAPS_RTPATCHES\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_RTPATCHHANDLEZERO)
        LogRel((" - D3DDEVCAPS_RTPATCHHANDLEZERO\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_SEPARATETEXTUREMEMORIES)
        LogRel((" - D3DDEVCAPS_SEPARATETEXTUREMEMORIES\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM)
        LogRel((" - D3DDEVCAPS_TEXTURENONLOCALVIDMEM\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY)
        LogRel((" - D3DDEVCAPS_TEXTURESYSTEMMEMORY\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY)
        LogRel((" - D3DDEVCAPS_TEXTUREVIDEOMEMORY\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_TLVERTEXSYSTEMMEMORY)
        LogRel((" - D3DDEVCAPS_TLVERTEXSYSTEMMEMORY\n"));
    if (pCaps->DevCaps & D3DDEVCAPS_TLVERTEXVIDEOMEMORY)
        LogRel((" - D3DDEVCAPS_TLVERTEXVIDEOMEMORY\n"));

    LogRel(("\nTextureCaps:\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_ALPHA)
        LogRel((" - D3DPTEXTURECAPS_ALPHA\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE)
        LogRel((" - D3DPTEXTURECAPS_ALPHAPALETTE\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
        LogRel((" - D3DPTEXTURECAPS_CUBEMAP\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2)
        LogRel((" - D3DPTEXTURECAPS_CUBEMAP_POW2\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP)
        LogRel((" - D3DPTEXTURECAPS_MIPCUBEMAP\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_MIPMAP)
        LogRel((" - D3DPTEXTURECAPS_MIPMAP\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_MIPVOLUMEMAP)
        LogRel((" - D3DPTEXTURECAPS_MIPVOLUMEMAP\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)
        LogRel((" - D3DPTEXTURECAPS_NONPOW2CONDITIONAL\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_POW2)
        LogRel((" - D3DPTEXTURECAPS_POW2\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_NOPROJECTEDBUMPENV)
        LogRel((" - D3DPTEXTURECAPS_NOPROJECTEDBUMPENV\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_PERSPECTIVE)
        LogRel((" - D3DPTEXTURECAPS_PERSPECTIVE\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_POW2)
        LogRel((" - D3DPTEXTURECAPS_POW2\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_PROJECTED)
        LogRel((" - D3DPTEXTURECAPS_PROJECTED\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
        LogRel((" - D3DPTEXTURECAPS_SQUAREONLY\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE)
        LogRel((" - D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
        LogRel((" - D3DPTEXTURECAPS_VOLUMEMAP\n"));
    if (pCaps->TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP_POW2)
        LogRel((" - D3DPTEXTURECAPS_VOLUMEMAP_POW2\n"));

    LogRel(("\nTextureFilterCaps\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO)
        LogRel((" - D3DPTFILTERCAPS_CONVOLUTIONMONO\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT)
        LogRel((" - D3DPTFILTERCAPS_MAGFPOINT\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR)
        LogRel((" - D3DPTFILTERCAPS_MAGFLINEAR\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC)
        LogRel((" - D3DPTFILTERCAPS_MAGFANISOTROPIC\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD)
        LogRel((" - D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD)
        LogRel((" - D3DPTFILTERCAPS_MAGFGAUSSIANQUAD\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT)
        LogRel((" - D3DPTFILTERCAPS_MINFPOINT\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR)
        LogRel((" - D3DPTFILTERCAPS_MINFLINEAR\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC)
        LogRel((" - D3DPTFILTERCAPS_MINFANISOTROPIC\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD)
        LogRel((" - D3DPTFILTERCAPS_MINFPYRAMIDALQUAD\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD)
        LogRel((" - D3DPTFILTERCAPS_MINFGAUSSIANQUAD\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT)
        LogRel((" - D3DPTFILTERCAPS_MIPFPOINT\n"));
    if (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR)
        LogRel((" - D3DPTFILTERCAPS_MIPFLINEAR\n"));

    LogRel(("\nCubeTextureFilterCaps\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO)
        LogRel((" - D3DPTFILTERCAPS_CONVOLUTIONMONO\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT)
        LogRel((" - D3DPTFILTERCAPS_MAGFPOINT\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR)
        LogRel((" - D3DPTFILTERCAPS_MAGFLINEAR\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC)
        LogRel((" - D3DPTFILTERCAPS_MAGFANISOTROPIC\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD)
        LogRel((" - D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD)
        LogRel((" - D3DPTFILTERCAPS_MAGFGAUSSIANQUAD\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT)
        LogRel((" - D3DPTFILTERCAPS_MINFPOINT\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR)
        LogRel((" - D3DPTFILTERCAPS_MINFLINEAR\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC)
        LogRel((" - D3DPTFILTERCAPS_MINFANISOTROPIC\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD)
        LogRel((" - D3DPTFILTERCAPS_MINFPYRAMIDALQUAD\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD)
        LogRel((" - D3DPTFILTERCAPS_MINFGAUSSIANQUAD\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT)
        LogRel((" - D3DPTFILTERCAPS_MIPFPOINT\n"));
    if (pCaps->CubeTextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR)
        LogRel((" - D3DPTFILTERCAPS_MIPFLINEAR\n"));

    LogRel(("\nVolumeTextureFilterCaps\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_CONVOLUTIONMONO)
        LogRel((" - D3DPTFILTERCAPS_CONVOLUTIONMONO\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT)
        LogRel((" - D3DPTFILTERCAPS_MAGFPOINT\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR)
        LogRel((" - D3DPTFILTERCAPS_MAGFLINEAR\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC)
        LogRel((" - D3DPTFILTERCAPS_MAGFANISOTROPIC\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD)
        LogRel((" - D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_MAGFGAUSSIANQUAD)
        LogRel((" - D3DPTFILTERCAPS_MAGFGAUSSIANQUAD\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFPOINT)
        LogRel((" - D3DPTFILTERCAPS_MINFPOINT\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR)
        LogRel((" - D3DPTFILTERCAPS_MINFLINEAR\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC)
        LogRel((" - D3DPTFILTERCAPS_MINFANISOTROPIC\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFPYRAMIDALQUAD)
        LogRel((" - D3DPTFILTERCAPS_MINFPYRAMIDALQUAD\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_MINFGAUSSIANQUAD)
        LogRel((" - D3DPTFILTERCAPS_MINFGAUSSIANQUAD\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT)
        LogRel((" - D3DPTFILTERCAPS_MIPFPOINT\n"));
    if (pCaps->VolumeTextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR)
        LogRel((" - D3DPTFILTERCAPS_MIPFLINEAR\n"));

    LogRel(("\nTextureAddressCaps:\n"));
    if (pCaps->TextureAddressCaps & D3DPTADDRESSCAPS_BORDER)
        LogRel((" - D3DPTADDRESSCAPS_BORDER\n"));
    if (pCaps->TextureAddressCaps & D3DPTADDRESSCAPS_CLAMP)
        LogRel((" - D3DPTADDRESSCAPS_CLAMP\n"));
    if (pCaps->TextureAddressCaps & D3DPTADDRESSCAPS_INDEPENDENTUV)
        LogRel((" - D3DPTADDRESSCAPS_INDEPENDENTUV\n"));
    if (pCaps->TextureAddressCaps & D3DPTADDRESSCAPS_MIRROR)
        LogRel((" - D3DPTADDRESSCAPS_MIRROR\n"));
    if (pCaps->TextureAddressCaps & D3DPTADDRESSCAPS_MIRRORONCE)
        LogRel((" - D3DPTADDRESSCAPS_MIRRORONCE\n"));
    if (pCaps->TextureAddressCaps & D3DPTADDRESSCAPS_WRAP)
        LogRel((" - D3DPTADDRESSCAPS_WRAP\n"));

    LogRel(("\nTextureOpCaps:\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_DISABLE)
        LogRel((" - D3DTEXOPCAPS_DISABLE\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_SELECTARG1)
        LogRel((" - D3DTEXOPCAPS_SELECTARG1\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_SELECTARG2)
        LogRel((" - D3DTEXOPCAPS_SELECTARG2\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_MODULATE)
        LogRel((" - D3DTEXOPCAPS_MODULATE\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_MODULATE2X)
        LogRel((" - D3DTEXOPCAPS_MODULATE2X\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_MODULATE4X)
        LogRel((" - D3DTEXOPCAPS_MODULATE4X\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_ADD)
        LogRel((" - D3DTEXOPCAPS_ADD\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_ADDSIGNED)
        LogRel((" - D3DTEXOPCAPS_ADDSIGNED\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_ADDSIGNED2X)
        LogRel((" - D3DTEXOPCAPS_ADDSIGNED2X\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_SUBTRACT)
        LogRel((" - D3DTEXOPCAPS_SUBTRACT\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_ADDSMOOTH)
        LogRel((" - D3DTEXOPCAPS_ADDSMOOTH\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_BLENDDIFFUSEALPHA)
        LogRel((" - D3DTEXOPCAPS_BLENDDIFFUSEALPHA\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_BLENDTEXTUREALPHA)
        LogRel((" - D3DTEXOPCAPS_BLENDTEXTUREALPHA\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_BLENDFACTORALPHA)
        LogRel((" - D3DTEXOPCAPS_BLENDFACTORALPHA\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_BLENDTEXTUREALPHAPM)
        LogRel((" - D3DTEXOPCAPS_BLENDTEXTUREALPHAPM\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_BLENDCURRENTALPHA)
        LogRel((" - D3DTEXOPCAPS_BLENDCURRENTALPHA\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_PREMODULATE)
        LogRel((" - D3DTEXOPCAPS_PREMODULATE\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR)
        LogRel((" - D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA)
        LogRel((" - D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR)
        LogRel((" - D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA)
        LogRel((" - D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAP)
        LogRel((" - D3DTEXOPCAPS_BUMPENVMAP\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAPLUMINANCE)
        LogRel((" - D3DTEXOPCAPS_BUMPENVMAPLUMINANCE\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_DOTPRODUCT3)
        LogRel((" - D3DTEXOPCAPS_DOTPRODUCT3\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_MULTIPLYADD)
        LogRel((" - D3DTEXOPCAPS_MULTIPLYADD\n"));
    if (pCaps->TextureOpCaps & D3DTEXOPCAPS_LERP)
        LogRel((" - D3DTEXOPCAPS_LERP\n"));


    LogRel(("\n"));
    LogRel(("PixelShaderVersion:  %#x (%u.%u)\n", pCaps->PixelShaderVersion,
            D3DSHADER_VERSION_MAJOR(pCaps->PixelShaderVersion), D3DSHADER_VERSION_MINOR(pCaps->PixelShaderVersion)));
    LogRel(("VertexShaderVersion: %#x (%u.%u)\n", pCaps->VertexShaderVersion,
            D3DSHADER_VERSION_MAJOR(pCaps->VertexShaderVersion), D3DSHADER_VERSION_MINOR(pCaps->VertexShaderVersion)));

    LogRel(("\n"));
    RTLogRelSetBuffering(fBufferingSaved);
}

