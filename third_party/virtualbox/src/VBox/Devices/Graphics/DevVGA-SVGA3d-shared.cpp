/* $Id: DevVGA-SVGA3d-shared.cpp $ */
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
#define VMSVGA3D_INCL_STRUCTURE_DESCRIPTORS
#include "DevVGA-SVGA3d-internal.h"


#ifdef RT_OS_WINDOWS
# define VMSVGA3D_WNDCLASSNAME  "VMSVGA3DWNDCLS"

static LONG WINAPI vmsvga3dWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


/**
 * Send a message to the async window thread and wait for a reply
 *
 * @returns VBox status code.
 * @param   pWindowThread   Thread handle
 * @param   WndRequestSem   Semaphore handle for waiting
 * @param   msg             Message id
 * @param   wParam          First parameter
 * @param   lParam          Second parameter
 */
int vmsvga3dSendThreadMessage(RTTHREAD pWindowThread, RTSEMEVENT WndRequestSem, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int  rc;
    BOOL ret;

    ret = PostThreadMessage(RTThreadGetNative(pWindowThread), msg, wParam, lParam);
    AssertMsgReturn(ret, ("PostThreadMessage %x failed with %d\n", RTThreadGetNative(pWindowThread), GetLastError()), VERR_INTERNAL_ERROR);

    rc = RTSemEventWait(WndRequestSem, RT_INDEFINITE_WAIT);
    Assert(RT_SUCCESS(rc));

    return rc;
}

/**
 * The async window handling thread
 *
 * @returns VBox status code.
 * @param   hThreadSelf     This thread.
 * @param   pvUser          Request sempahore handle.
 */
DECLCALLBACK(int) vmsvga3dWindowThread(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF(hThreadSelf);
    RTSEMEVENT      WndRequestSem = (RTSEMEVENT)pvUser;
    WNDCLASSEX      wc;

    /* Register our own window class. */
    wc.cbSize           = sizeof(wc);
    wc.style            = CS_OWNDC;
    wc.lpfnWndProc      = (WNDPROC) vmsvga3dWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = GetModuleHandle("VBoxDD.dll");    /** @todo hardcoded name.. */
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = NULL;
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = VMSVGA3D_WNDCLASSNAME;
    wc.hIconSm          = NULL;

    if (!RegisterClassEx(&wc))
    {
        Log(("RegisterClass failed with %x\n", GetLastError()));
        return VERR_INTERNAL_ERROR;
    }

    LogFlow(("vmsvga3dWindowThread: started loop\n"));
    while (true)
    {
        MSG msg;

        if (GetMessage(&msg, 0, 0, 0))
        {
            if (msg.message == WM_VMSVGA3D_EXIT)
            {
                /* Signal to the caller that we're done. */
                RTSemEventSignal(WndRequestSem);
                break;
            }

            if (msg.message == WM_VMSVGA3D_WAKEUP)
            {
                continue;
            }
            if (msg.message == WM_VMSVGA3D_CREATEWINDOW)
            {
                HWND          *pHwnd = (HWND *)msg.wParam;
                LPCREATESTRUCT pCS = (LPCREATESTRUCT) msg.lParam;

#ifdef DEBUG_GFX_WINDOW
                RECT rectClient;

                rectClient.left     = 0;
                rectClient.top      = 0;
                rectClient.right    = pCS->cx;
                rectClient.bottom   = pCS->cy;
                AdjustWindowRectEx(&rectClient, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_CAPTION, FALSE, WS_EX_NOACTIVATE | WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT);
                pCS->cx = rectClient.right - rectClient.left;
                pCS->cy = rectClient.bottom - rectClient.top;
#endif
                *pHwnd = CreateWindowEx(pCS->dwExStyle,
                                        VMSVGA3D_WNDCLASSNAME,
                                        pCS->lpszName,
                                        pCS->style,
#ifdef DEBUG_GFX_WINDOW
                                        0,
                                        0,
#else
                                        pCS->x,
                                        pCS->y,
#endif
                                        pCS->cx,
                                        pCS->cy,
#ifdef DEBUG_GFX_WINDOW
                                        0,
#else
                                        pCS->hwndParent,
#endif
                                        pCS->hMenu,
                                        pCS->hInstance,
                                        NULL);
                AssertMsg(*pHwnd, ("CreateWindowEx %x %s %s %x (%d,%d)(%d,%d), %x %x %x error=%x\n", pCS->dwExStyle, pCS->lpszName, VMSVGA3D_WNDCLASSNAME, pCS->style, pCS->x,
                                    pCS->y, pCS->cx, pCS->cy,pCS->hwndParent, pCS->hMenu, pCS->hInstance, GetLastError()));

                /* Signal to the caller that we're done. */
                RTSemEventSignal(WndRequestSem);
                continue;
            }
            if (msg.message == WM_VMSVGA3D_DESTROYWINDOW)
            {
                BOOL fRc = DestroyWindow((HWND)msg.wParam);
                Assert(fRc); NOREF(fRc);
                /* Signal to the caller that we're done. */
                RTSemEventSignal(WndRequestSem);
                continue;
            }
            if (msg.message == WM_VMSVGA3D_RESIZEWINDOW)
            {
                HWND hwnd = (HWND)msg.wParam;
                LPCREATESTRUCT pCS = (LPCREATESTRUCT) msg.lParam;

#ifdef DEBUG_GFX_WINDOW
                RECT rectClient;

                rectClient.left     = 0;
                rectClient.top      = 0;
                rectClient.right    = pCS->cx;
                rectClient.bottom   = pCS->cy;
                AdjustWindowRectEx(&rectClient, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_CAPTION, FALSE, WS_EX_NOACTIVATE | WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT);
                pCS->cx = rectClient.right - rectClient.left;
                pCS->cy = rectClient.bottom - rectClient.top;
#endif
                BOOL fRc = SetWindowPos(hwnd, 0, pCS->x, pCS->y, pCS->cx, pCS->cy, SWP_NOZORDER | SWP_NOMOVE);
                Assert(fRc); NOREF(fRc);

                /* Signal to the caller that we're done. */
                RTSemEventSignal(WndRequestSem);
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Log(("GetMessage failed with %x\n", GetLastError()));
            break;
        }
    }

    Log(("vmsvga3dWindowThread: end loop\n"));
    return VINF_SUCCESS;
}

/* Window procedure for our top level window overlays. */
static LONG WINAPI vmsvga3dWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            Log7(("vmsvga3dWndProc(%p): WM_CLOSE\n", hwnd));
            break;

        case WM_DESTROY:
            Log7(("vmsvga3dWndProc(%p): WM_DESTROY\n", hwnd));
            break;

        case WM_NCHITTEST:
            Log7(("vmsvga3dWndProc(%p): WM_NCHITTEST\n", hwnd));
            return HTNOWHERE;

# if 0 /* flicker experiment, no help here. */
        case WM_PAINT:
            Log7(("vmsvga3dWndProc(%p): WM_PAINT %p %p\n", hwnd, wParam, lParam));
            ValidateRect(hwnd, NULL);
            return 0;
        case WM_ERASEBKGND:
            Log7(("vmsvga3dWndProc(%p): WM_ERASEBKGND %p %p\n", hwnd, wParam, lParam));
            return TRUE;
        case WM_NCPAINT:
            Log7(("vmsvga3dWndProc(%p): WM_NCPAINT %p %p\n", hwnd, wParam, lParam));
            break;
        case WM_WINDOWPOSCHANGING:
        {
            PWINDOWPOS pPos = (PWINDOWPOS)lParam;
            Log7(("vmsvga3dWndProc(%p): WM_WINDOWPOSCHANGING %p %p pos=(%d,%d) size=(%d,%d) flags=%#x\n",
                  hwnd, wParam, lParam, pPos->x, pPos->y, pPos->cx, pPos->cy, pPos->flags));
            break;
        }
        case WM_WINDOWPOSCHANGED:
        {
            PWINDOWPOS pPos = (PWINDOWPOS)lParam;
            Log7(("vmsvga3dWndProc(%p): WM_WINDOWPOSCHANGED %p %p pos=(%d,%d) size=(%d,%d) flags=%#x\n",
                  hwnd, wParam, lParam, pPos->x, pPos->y, pPos->cx, pPos->cy, pPos->flags));
            break;
        }
        case WM_MOVE:
            Log7(("vmsvga3dWndProc(%p): WM_MOVE %p %p\n", hwnd, wParam, lParam));
            break;
        case WM_SIZE:
            Log7(("vmsvga3dWndProc(%p): WM_SIZE %p %p\n", hwnd, wParam, lParam));
            break;

        default:
            Log7(("vmsvga3dWndProc(%p): %#x %p %p\n", hwnd, uMsg, wParam, lParam));
# endif
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#endif /* RT_OS_WINDOWS */


/**
 * Calculate the size and dimensions of one block.
 */
uint32_t vmsvga3dSurfaceFormatSize(SVGA3dSurfaceFormat format,
                                   uint32_t *pcxBlock,
                                   uint32_t *pcyBlock)
{
    uint32_t u32 = 0;
    if (!pcxBlock) pcxBlock = &u32;
    if (!pcyBlock) pcyBlock = &u32;

    switch (format)
    {
    case SVGA3D_X8R8G8B8:
    case SVGA3D_A8R8G8B8:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 4;

    case SVGA3D_R5G6B5:
    case SVGA3D_X1R5G5B5:
    case SVGA3D_A1R5G5B5:
    case SVGA3D_A4R4G4B4:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 2;

    case SVGA3D_Z_D32:
    case SVGA3D_Z_D24S8:
    case SVGA3D_Z_D24X8:
    case SVGA3D_Z_DF24:
    case SVGA3D_Z_D24S8_INT:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 4;

    case SVGA3D_Z_D16:
    case SVGA3D_Z_DF16:
    case SVGA3D_Z_D15S1:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 2;

    case SVGA3D_LUMINANCE8:
    case SVGA3D_LUMINANCE4_ALPHA4:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 1;

    case SVGA3D_LUMINANCE16:
    case SVGA3D_LUMINANCE8_ALPHA8:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 2;

    case SVGA3D_DXT1:
    case SVGA3D_DXT2:
        *pcxBlock = 4;
        *pcyBlock = 4;
        return 8;

    case SVGA3D_DXT3:
    case SVGA3D_DXT4:
    case SVGA3D_DXT5:
        *pcxBlock = 4;
        *pcyBlock = 4;
        return 16;

    case SVGA3D_BUMPU8V8:
    case SVGA3D_BUMPL6V5U5:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 2;

    case SVGA3D_BUMPX8L8V8U8:
    case SVGA3D_Q8W8V8U8:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 4;

    case SVGA3D_V8U8:
    case SVGA3D_CxV8U8:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 2;

    case SVGA3D_X8L8V8U8:
    case SVGA3D_A2W10V10U10:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 4;

    case SVGA3D_ARGB_S10E5:   /* 16-bit floating-point ARGB */
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 2;
    case SVGA3D_ARGB_S23E8:   /* 32-bit floating-point ARGB */
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 4;

    case SVGA3D_A2R10G10B10:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 4;

    case SVGA3D_ALPHA8:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 1;

    case SVGA3D_R_S10E5:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 2;

    case SVGA3D_R_S23E8:
    case SVGA3D_RG_S10E5:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 4;

    case SVGA3D_RG_S23E8:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 8;

    /*
     * Any surface can be used as a buffer object, but SVGA3D_BUFFER is
     * the most efficient format to use when creating new surfaces
     * expressly for index or vertex data.
     */
    case SVGA3D_BUFFER:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 1;

    case SVGA3D_NV12:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 1;

    case SVGA3D_V16U16:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 4;

    case SVGA3D_G16R16:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 4;
    case SVGA3D_A16B16G16R16:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 8;
    case SVGA3D_R8G8B8A8_UNORM:
    case SVGA3D_R8G8B8A8_SNORM:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 4;
    case SVGA3D_R16G16_UNORM:
        *pcxBlock = 1;
        *pcyBlock = 1;
        return 4;

    default:
        *pcxBlock = 1;
        *pcyBlock = 1;
        AssertFailedReturn(4);
    }
}

#ifdef LOG_ENABLED

const char *vmsvga3dGetCapString(uint32_t idxCap)
{
    switch (idxCap)
    {
    case SVGA3D_DEVCAP_3D:
        return "SVGA3D_DEVCAP_3D";
    case SVGA3D_DEVCAP_MAX_LIGHTS:
        return "SVGA3D_DEVCAP_MAX_LIGHTS";
    case SVGA3D_DEVCAP_MAX_TEXTURES:
        return "SVGA3D_DEVCAP_MAX_TEXTURES";
    case SVGA3D_DEVCAP_MAX_CLIP_PLANES:
        return "SVGA3D_DEVCAP_MAX_CLIP_PLANES";
    case SVGA3D_DEVCAP_VERTEX_SHADER_VERSION:
        return "SVGA3D_DEVCAP_VERTEX_SHADER_VERSION";
    case SVGA3D_DEVCAP_VERTEX_SHADER:
        return "SVGA3D_DEVCAP_VERTEX_SHADER";
    case SVGA3D_DEVCAP_FRAGMENT_SHADER_VERSION:
        return "SVGA3D_DEVCAP_FRAGMENT_SHADER_VERSION";
    case SVGA3D_DEVCAP_FRAGMENT_SHADER:
        return "SVGA3D_DEVCAP_FRAGMENT_SHADER";
    case SVGA3D_DEVCAP_MAX_RENDER_TARGETS:
        return "SVGA3D_DEVCAP_MAX_RENDER_TARGETS";
    case SVGA3D_DEVCAP_S23E8_TEXTURES:
        return "SVGA3D_DEVCAP_S23E8_TEXTURES";
    case SVGA3D_DEVCAP_S10E5_TEXTURES:
        return "SVGA3D_DEVCAP_S10E5_TEXTURES";
    case SVGA3D_DEVCAP_MAX_FIXED_VERTEXBLEND:
        return "SVGA3D_DEVCAP_MAX_FIXED_VERTEXBLEND";
    case SVGA3D_DEVCAP_D16_BUFFER_FORMAT:
        return "SVGA3D_DEVCAP_D16_BUFFER_FORMAT";
    case SVGA3D_DEVCAP_D24S8_BUFFER_FORMAT:
        return "SVGA3D_DEVCAP_D24S8_BUFFER_FORMAT";
    case SVGA3D_DEVCAP_D24X8_BUFFER_FORMAT:
        return "SVGA3D_DEVCAP_D24X8_BUFFER_FORMAT";
    case SVGA3D_DEVCAP_QUERY_TYPES:
        return "SVGA3D_DEVCAP_QUERY_TYPES";
    case SVGA3D_DEVCAP_TEXTURE_GRADIENT_SAMPLING:
        return "SVGA3D_DEVCAP_TEXTURE_GRADIENT_SAMPLING";
    case SVGA3D_DEVCAP_MAX_POINT_SIZE:
        return "SVGA3D_DEVCAP_MAX_POINT_SIZE";
    case SVGA3D_DEVCAP_MAX_SHADER_TEXTURES:
        return "SVGA3D_DEVCAP_MAX_SHADER_TEXTURES";
    case SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH:
        return "SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH";
    case SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT:
        return "SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT";
    case SVGA3D_DEVCAP_MAX_VOLUME_EXTENT:
        return "SVGA3D_DEVCAP_MAX_VOLUME_EXTENT";
    case SVGA3D_DEVCAP_MAX_TEXTURE_REPEAT:
        return "SVGA3D_DEVCAP_MAX_TEXTURE_REPEAT";
    case SVGA3D_DEVCAP_MAX_TEXTURE_ASPECT_RATIO:
        return "SVGA3D_DEVCAP_MAX_TEXTURE_ASPECT_RATIO";
    case SVGA3D_DEVCAP_MAX_TEXTURE_ANISOTROPY:
        return "SVGA3D_DEVCAP_MAX_TEXTURE_ANISOTROPY";
    case SVGA3D_DEVCAP_MAX_PRIMITIVE_COUNT:
        return "SVGA3D_DEVCAP_MAX_PRIMITIVE_COUNT";
    case SVGA3D_DEVCAP_MAX_VERTEX_INDEX:
        return "SVGA3D_DEVCAP_MAX_VERTEX_INDEX";
    case SVGA3D_DEVCAP_MAX_VERTEX_SHADER_INSTRUCTIONS:
        return "SVGA3D_DEVCAP_MAX_VERTEX_SHADER_INSTRUCTIONS";
    case SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_INSTRUCTIONS:
        return "SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_INSTRUCTIONS";
    case SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS:
        return "SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS";
    case SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS:
        return "SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS";
    case SVGA3D_DEVCAP_TEXTURE_OPS:
        return "SVGA3D_DEVCAP_TEXTURE_OPS";
    case SVGA3D_DEVCAP_MULTISAMPLE_NONMASKABLESAMPLES:
        return "SVGA3D_DEVCAP_MULTISAMPLE_NONMASKABLESAMPLES";
    case SVGA3D_DEVCAP_MULTISAMPLE_MASKABLESAMPLES:
        return "SVGA3D_DEVCAP_MULTISAMPLE_MASKABLESAMPLES";
    case SVGA3D_DEVCAP_ALPHATOCOVERAGE:
        return "SVGA3D_DEVCAP_ALPHATOCOVERAGE";
    case SVGA3D_DEVCAP_SUPERSAMPLE:
        return "SVGA3D_DEVCAP_SUPERSAMPLE";
    case SVGA3D_DEVCAP_AUTOGENMIPMAPS:
        return "SVGA3D_DEVCAP_AUTOGENMIPMAPS";
    case SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEXTURES:
        return "SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEXTURES";
    case SVGA3D_DEVCAP_MAX_SIMULTANEOUS_RENDER_TARGETS:
        return "SVGA3D_DEVCAP_MAX_SIMULTANEOUS_RENDER_TARGETS";
    case SVGA3D_DEVCAP_MAX_CONTEXT_IDS:
        return "SVGA3D_DEVCAP_MAX_CONTEXT_IDS";
    case SVGA3D_DEVCAP_MAX_SURFACE_IDS:
        return "SVGA3D_DEVCAP_MAX_SURFACE_IDS";
    case SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8:
        return "SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8";
    case SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8:
        return "SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8";
    case SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10:
        return "SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10";
    case SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5:
        return "SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5";
    case SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5:
        return "SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5";
    case SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4:
        return "SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4";
    case SVGA3D_DEVCAP_SURFACEFMT_R5G6B5:
        return "SVGA3D_DEVCAP_SURFACEFMT_R5G6B5";
    case SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE16:
        return "SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE16";
    case SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8_ALPHA8:
        return "SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8_ALPHA8";
    case SVGA3D_DEVCAP_SURFACEFMT_ALPHA8:
        return "SVGA3D_DEVCAP_SURFACEFMT_ALPHA8";
    case SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8:
        return "SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8";
    case SVGA3D_DEVCAP_SURFACEFMT_Z_D16:
        return "SVGA3D_DEVCAP_SURFACEFMT_Z_D16";
    case SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8:
        return "SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8";
    case SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8:
        return "SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8";
    case SVGA3D_DEVCAP_SURFACEFMT_Z_DF16:
        return "SVGA3D_DEVCAP_SURFACEFMT_Z_DF16";
    case SVGA3D_DEVCAP_SURFACEFMT_Z_DF24:
        return "SVGA3D_DEVCAP_SURFACEFMT_Z_DF24";
    case SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8_INT:
        return "SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8_INT";
    case SVGA3D_DEVCAP_SURFACEFMT_DXT1:
        return "SVGA3D_DEVCAP_SURFACEFMT_DXT1";
    case SVGA3D_DEVCAP_SURFACEFMT_DXT2:
        return "SVGA3D_DEVCAP_SURFACEFMT_DXT2";
    case SVGA3D_DEVCAP_SURFACEFMT_DXT3:
        return "SVGA3D_DEVCAP_SURFACEFMT_DXT3";
    case SVGA3D_DEVCAP_SURFACEFMT_DXT4:
        return "SVGA3D_DEVCAP_SURFACEFMT_DXT4";
    case SVGA3D_DEVCAP_SURFACEFMT_DXT5:
        return "SVGA3D_DEVCAP_SURFACEFMT_DXT5";
    case SVGA3D_DEVCAP_SURFACEFMT_BUMPX8L8V8U8:
        return "SVGA3D_DEVCAP_SURFACEFMT_BUMPX8L8V8U8";
    case SVGA3D_DEVCAP_SURFACEFMT_A2W10V10U10:
        return "SVGA3D_DEVCAP_SURFACEFMT_A2W10V10U10";
    case SVGA3D_DEVCAP_SURFACEFMT_BUMPU8V8:
        return "SVGA3D_DEVCAP_SURFACEFMT_BUMPU8V8";
    case SVGA3D_DEVCAP_SURFACEFMT_Q8W8V8U8:
        return "SVGA3D_DEVCAP_SURFACEFMT_Q8W8V8U8";
    case SVGA3D_DEVCAP_SURFACEFMT_CxV8U8:
        return "SVGA3D_DEVCAP_SURFACEFMT_CxV8U8";
    case SVGA3D_DEVCAP_SURFACEFMT_R_S10E5:
        return "SVGA3D_DEVCAP_SURFACEFMT_R_S10E5";
    case SVGA3D_DEVCAP_SURFACEFMT_R_S23E8:
        return "SVGA3D_DEVCAP_SURFACEFMT_R_S23E8";
    case SVGA3D_DEVCAP_SURFACEFMT_RG_S10E5:
        return "SVGA3D_DEVCAP_SURFACEFMT_RG_S10E5";
    case SVGA3D_DEVCAP_SURFACEFMT_RG_S23E8:
        return "SVGA3D_DEVCAP_SURFACEFMT_RG_S23E8";
    case SVGA3D_DEVCAP_SURFACEFMT_ARGB_S10E5:
        return "SVGA3D_DEVCAP_SURFACEFMT_ARGB_S10E5";
    case SVGA3D_DEVCAP_SURFACEFMT_ARGB_S23E8:
        return "SVGA3D_DEVCAP_SURFACEFMT_ARGB_S23E8";
    case SVGA3D_DEVCAP_SURFACEFMT_V16U16:
        return "SVGA3D_DEVCAP_SURFACEFMT_V16U16";
    case SVGA3D_DEVCAP_SURFACEFMT_G16R16:
        return "SVGA3D_DEVCAP_SURFACEFMT_G16R16";
    case SVGA3D_DEVCAP_SURFACEFMT_A16B16G16R16:
        return "SVGA3D_DEVCAP_SURFACEFMT_A16B16G16R16";
    case SVGA3D_DEVCAP_SURFACEFMT_UYVY:
        return "SVGA3D_DEVCAP_SURFACEFMT_UYVY";
    case SVGA3D_DEVCAP_SURFACEFMT_YUY2:
        return "SVGA3D_DEVCAP_SURFACEFMT_YUY2";
    case SVGA3D_DEVCAP_SURFACEFMT_NV12:
        return "SVGA3D_DEVCAP_SURFACEFMT_NV12";
    case SVGA3D_DEVCAP_SURFACEFMT_AYUV:
        return "SVGA3D_DEVCAP_SURFACEFMT_AYUV";
    case SVGA3D_DEVCAP_SURFACEFMT_BC4_UNORM:
        return "SVGA3D_DEVCAP_SURFACEFMT_BC4_UNORM";
    case SVGA3D_DEVCAP_SURFACEFMT_BC5_UNORM:
        return "SVGA3D_DEVCAP_SURFACEFMT_BC5_UNORM";
    default:
        return "UNEXPECTED";
    }
}

const char *vmsvga3dGet3dFormatString(uint32_t format)
{
    static char szFormat[1024];

    szFormat[0] = 0;

    if (format & SVGA3DFORMAT_OP_TEXTURE)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_TEXTURE\n");
    if (format & SVGA3DFORMAT_OP_VOLUMETEXTURE)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_VOLUMETEXTURE\n");
    if (format & SVGA3DFORMAT_OP_CUBETEXTURE)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_CUBETEXTURE\n");
    if (format & SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET\n");
    if (format & SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET\n");
    if (format & SVGA3DFORMAT_OP_ZSTENCIL)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_ZSTENCIL\n");
    if (format & SVGA3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH\n");
    if (format & SVGA3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET\n");
    if (format & SVGA3DFORMAT_OP_DISPLAYMODE)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_DISPLAYMODE\n");
    if (format & SVGA3DFORMAT_OP_3DACCELERATION)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_3DACCELERATION\n");
    if (format & SVGA3DFORMAT_OP_PIXELSIZE)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_PIXELSIZE\n");
    if (format & SVGA3DFORMAT_OP_CONVERT_TO_ARGB)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_CONVERT_TO_ARGB\n");
    if (format & SVGA3DFORMAT_OP_OFFSCREENPLAIN)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_OFFSCREENPLAIN\n");
    if (format & SVGA3DFORMAT_OP_SRGBREAD)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_SRGBREAD\n");
    if (format & SVGA3DFORMAT_OP_BUMPMAP)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_BUMPMAP\n");
    if (format & SVGA3DFORMAT_OP_DMAP)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_DMAP\n");
    if (format & SVGA3DFORMAT_OP_NOFILTER)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_NOFILTER\n");
    if (format & SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB\n");
    if (format & SVGA3DFORMAT_OP_SRGBWRITE)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_SRGBWRITE\n");
    if (format & SVGA3DFORMAT_OP_NOALPHABLEND)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_NOALPHABLEND\n");
    if (format & SVGA3DFORMAT_OP_AUTOGENMIPMAP)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_AUTOGENMIPMAP\n");
    if (format & SVGA3DFORMAT_OP_VERTEXTEXTURE)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_VERTEXTEXTURE\n");
    if (format & SVGA3DFORMAT_OP_NOTEXCOORDWRAPNORMIP)
        strcat(szFormat, "   - SVGA3DFORMAT_OP_NOTEXCOORDWRAPNORMIP\n");
   return szFormat;
}

const char *vmsvga3dGetRenderStateName(uint32_t state)
{
    switch (state)
    {
    case SVGA3D_RS_ZENABLE:                /* SVGA3dBool */
        return "SVGA3D_RS_ZENABLE";
    case SVGA3D_RS_ZWRITEENABLE:           /* SVGA3dBool */
        return "SVGA3D_RS_ZWRITEENABLE";
    case SVGA3D_RS_ALPHATESTENABLE:        /* SVGA3dBool */
        return "SVGA3D_RS_ALPHATESTENABLE";
    case SVGA3D_RS_DITHERENABLE:           /* SVGA3dBool */
        return "SVGA3D_RS_DITHERENABLE";
    case SVGA3D_RS_BLENDENABLE:            /* SVGA3dBool */
        return "SVGA3D_RS_BLENDENABLE";
    case SVGA3D_RS_FOGENABLE:              /* SVGA3dBool */
        return "SVGA3D_RS_FOGENABLE";
    case SVGA3D_RS_SPECULARENABLE:         /* SVGA3dBool */
        return "SVGA3D_RS_SPECULARENABLE";
    case SVGA3D_RS_STENCILENABLE:          /* SVGA3dBool */
        return "SVGA3D_RS_STENCILENABLE";
    case SVGA3D_RS_LIGHTINGENABLE:         /* SVGA3dBool */
        return "SVGA3D_RS_LIGHTINGENABLE";
    case SVGA3D_RS_NORMALIZENORMALS:       /* SVGA3dBool */
        return "SVGA3D_RS_NORMALIZENORMALS";
    case SVGA3D_RS_POINTSPRITEENABLE:      /* SVGA3dBool */
        return "SVGA3D_RS_POINTSPRITEENABLE";
    case SVGA3D_RS_POINTSCALEENABLE:       /* SVGA3dBool */
        return "SVGA3D_RS_POINTSCALEENABLE";
    case SVGA3D_RS_STENCILREF:             /* uint32_t */
        return "SVGA3D_RS_STENCILREF";
    case SVGA3D_RS_STENCILMASK:            /* uint32_t */
        return "SVGA3D_RS_STENCILMASK";
    case SVGA3D_RS_STENCILWRITEMASK:       /* uint32_t */
        return "SVGA3D_RS_STENCILWRITEMASK";
    case SVGA3D_RS_POINTSIZE:              /* float */
        return "SVGA3D_RS_POINTSIZE";
    case SVGA3D_RS_POINTSIZEMIN:           /* float */
        return "SVGA3D_RS_POINTSIZEMIN";
    case SVGA3D_RS_POINTSIZEMAX:           /* float */
        return "SVGA3D_RS_POINTSIZEMAX";
    case SVGA3D_RS_POINTSCALE_A:           /* float */
        return "SVGA3D_RS_POINTSCALE_A";
    case SVGA3D_RS_POINTSCALE_B:           /* float */
        return "SVGA3D_RS_POINTSCALE_B";
    case SVGA3D_RS_POINTSCALE_C:           /* float */
        return "SVGA3D_RS_POINTSCALE_C";
    case SVGA3D_RS_AMBIENT:                /* SVGA3dColor - identical */
        return "SVGA3D_RS_AMBIENT";
    case SVGA3D_RS_CLIPPLANEENABLE:        /* SVGA3dClipPlanes - identical */
        return "SVGA3D_RS_CLIPPLANEENABLE";
    case SVGA3D_RS_FOGCOLOR:               /* SVGA3dColor - identical */
        return "SVGA3D_RS_FOGCOLOR";
    case SVGA3D_RS_FOGSTART:               /* float */
        return "SVGA3D_RS_FOGSTART";
    case SVGA3D_RS_FOGEND:                 /* float */
        return "SVGA3D_RS_FOGEND";
    case SVGA3D_RS_FOGDENSITY:             /* float */
        return "SVGA3D_RS_FOGDENSITY";
    case SVGA3D_RS_RANGEFOGENABLE:         /* SVGA3dBool */
        return "SVGA3D_RS_RANGEFOGENABLE";
    case SVGA3D_RS_FOGMODE:                /* SVGA3dFogMode */
        return "SVGA3D_RS_FOGMODE";
    case SVGA3D_RS_FILLMODE:               /* SVGA3dFillMode */
        return "SVGA3D_RS_FILLMODE";
    case SVGA3D_RS_SHADEMODE:              /* SVGA3dShadeMode */
        return "SVGA3D_RS_SHADEMODE";
    case SVGA3D_RS_LINEPATTERN:            /* SVGA3dLinePattern */
        return "SVGA3D_RS_LINEPATTERN";
    case SVGA3D_RS_SRCBLEND:               /* SVGA3dBlendOp */
        return "SVGA3D_RS_SRCBLEND";
    case SVGA3D_RS_DSTBLEND:               /* SVGA3dBlendOp */
        return "SVGA3D_RS_DSTBLEND";
    case SVGA3D_RS_BLENDEQUATION:          /* SVGA3dBlendEquation */
        return "SVGA3D_RS_BLENDEQUATION";
    case SVGA3D_RS_CULLMODE:               /* SVGA3dFace */
        return "SVGA3D_RS_CULLMODE";
    case SVGA3D_RS_ZFUNC:                  /* SVGA3dCmpFunc */
        return "SVGA3D_RS_ZFUNC";
    case SVGA3D_RS_ALPHAFUNC:              /* SVGA3dCmpFunc */
        return "SVGA3D_RS_ALPHAFUNC";
    case SVGA3D_RS_STENCILFUNC:            /* SVGA3dCmpFunc */
        return "SVGA3D_RS_STENCILFUNC";
    case SVGA3D_RS_STENCILFAIL:            /* SVGA3dStencilOp */
        return "SVGA3D_RS_STENCILFAIL";
    case SVGA3D_RS_STENCILZFAIL:           /* SVGA3dStencilOp */
        return "SVGA3D_RS_STENCILZFAIL";
    case SVGA3D_RS_STENCILPASS:            /* SVGA3dStencilOp */
        return "SVGA3D_RS_STENCILPASS";
    case SVGA3D_RS_ALPHAREF:               /* float (0.0 .. 1.0) */
        return "SVGA3D_RS_ALPHAREF";
    case SVGA3D_RS_FRONTWINDING:           /* SVGA3dFrontWinding */
        return "SVGA3D_RS_FRONTWINDING";
    case SVGA3D_RS_COORDINATETYPE:         /* SVGA3dCoordinateType */
        return "SVGA3D_RS_COORDINATETYPE";
    case SVGA3D_RS_ZBIAS:                  /* float */
        return "SVGA3D_RS_ZBIAS";
    case SVGA3D_RS_COLORWRITEENABLE:       /* SVGA3dColorMask */
        return "SVGA3D_RS_COLORWRITEENABLE";
    case SVGA3D_RS_VERTEXMATERIALENABLE:   /* SVGA3dBool */
        return "SVGA3D_RS_VERTEXMATERIALENABLE";
    case SVGA3D_RS_DIFFUSEMATERIALSOURCE:  /* SVGA3dVertexMaterial */
        return "SVGA3D_RS_DIFFUSEMATERIALSOURCE";
    case SVGA3D_RS_SPECULARMATERIALSOURCE: /* SVGA3dVertexMaterial */
        return "SVGA3D_RS_SPECULARMATERIALSOURCE";
    case SVGA3D_RS_AMBIENTMATERIALSOURCE:  /* SVGA3dVertexMaterial */
        return "SVGA3D_RS_AMBIENTMATERIALSOURCE";
    case SVGA3D_RS_EMISSIVEMATERIALSOURCE: /* SVGA3dVertexMaterial */
        return "SVGA3D_RS_EMISSIVEMATERIALSOURCE";
    case SVGA3D_RS_TEXTUREFACTOR:          /* SVGA3dColor */
        return "SVGA3D_RS_TEXTUREFACTOR";
    case SVGA3D_RS_LOCALVIEWER:            /* SVGA3dBool */
        return "SVGA3D_RS_LOCALVIEWER";
    case SVGA3D_RS_SCISSORTESTENABLE:      /* SVGA3dBool */
        return "SVGA3D_RS_SCISSORTESTENABLE";
    case SVGA3D_RS_BLENDCOLOR:             /* SVGA3dColor */
        return "SVGA3D_RS_BLENDCOLOR";
    case SVGA3D_RS_STENCILENABLE2SIDED:    /* SVGA3dBool */
        return "SVGA3D_RS_STENCILENABLE2SIDED";
    case SVGA3D_RS_CCWSTENCILFUNC:         /* SVGA3dCmpFunc */
        return "SVGA3D_RS_CCWSTENCILFUNC";
    case SVGA3D_RS_CCWSTENCILFAIL:         /* SVGA3dStencilOp */
        return "SVGA3D_RS_CCWSTENCILFAIL";
    case SVGA3D_RS_CCWSTENCILZFAIL:        /* SVGA3dStencilOp */
        return "SVGA3D_RS_CCWSTENCILZFAIL";
    case SVGA3D_RS_CCWSTENCILPASS:         /* SVGA3dStencilOp */
        return "SVGA3D_RS_CCWSTENCILPASS";
    case SVGA3D_RS_VERTEXBLEND:            /* SVGA3dVertexBlendFlags */
        return "SVGA3D_RS_VERTEXBLEND";
    case SVGA3D_RS_SLOPESCALEDEPTHBIAS:    /* float */
        return "SVGA3D_RS_SLOPESCALEDEPTHBIAS";
    case SVGA3D_RS_DEPTHBIAS:              /* float */
        return "SVGA3D_RS_DEPTHBIAS";
    case SVGA3D_RS_OUTPUTGAMMA:            /* float */
        return "SVGA3D_RS_OUTPUTGAMMA";
    case SVGA3D_RS_ZVISIBLE:               /* SVGA3dBool */
        return "SVGA3D_RS_ZVISIBLE";
    case SVGA3D_RS_LASTPIXEL:              /* SVGA3dBool */
        return "SVGA3D_RS_LASTPIXEL";
    case SVGA3D_RS_CLIPPING:               /* SVGA3dBool */
        return "SVGA3D_RS_CLIPPING";
    case SVGA3D_RS_WRAP0:                  /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP0";
    case SVGA3D_RS_WRAP1:                  /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP1";
    case SVGA3D_RS_WRAP2:                  /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP2";
    case SVGA3D_RS_WRAP3:                  /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP3";
    case SVGA3D_RS_WRAP4:                  /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP4";
    case SVGA3D_RS_WRAP5:                  /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP5";
    case SVGA3D_RS_WRAP6:                  /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP6";
    case SVGA3D_RS_WRAP7:                  /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP7";
    case SVGA3D_RS_WRAP8:                  /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP8";
    case SVGA3D_RS_WRAP9:                  /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP9";
    case SVGA3D_RS_WRAP10:                 /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP10";
    case SVGA3D_RS_WRAP11:                 /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP11";
    case SVGA3D_RS_WRAP12:                 /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP12";
    case SVGA3D_RS_WRAP13:                 /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP13";
    case SVGA3D_RS_WRAP14:                 /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP14";
    case SVGA3D_RS_WRAP15:                 /* SVGA3dWrapFlags */
        return "SVGA3D_RS_WRAP15";
    case SVGA3D_RS_MULTISAMPLEANTIALIAS:   /* SVGA3dBool */
        return "SVGA3D_RS_MULTISAMPLEANTIALIAS";
    case SVGA3D_RS_MULTISAMPLEMASK:        /* uint32_t */
        return "SVGA3D_RS_MULTISAMPLEMASK";
    case SVGA3D_RS_INDEXEDVERTEXBLENDENABLE: /* SVGA3dBool */
        return "SVGA3D_RS_INDEXEDVERTEXBLENDENABLE";
    case SVGA3D_RS_TWEENFACTOR:            /* float */
        return "SVGA3D_RS_TWEENFACTOR";
    case SVGA3D_RS_ANTIALIASEDLINEENABLE:  /* SVGA3dBool */
        return "SVGA3D_RS_ANTIALIASEDLINEENABLE";
    case SVGA3D_RS_COLORWRITEENABLE1:      /* SVGA3dColorMask */
        return "SVGA3D_RS_COLORWRITEENABLE1";
    case SVGA3D_RS_COLORWRITEENABLE2:      /* SVGA3dColorMask */
        return "SVGA3D_RS_COLORWRITEENABLE2";
    case SVGA3D_RS_COLORWRITEENABLE3:      /* SVGA3dColorMask */
        return "SVGA3D_RS_COLORWRITEENABLE3";
    case SVGA3D_RS_SEPARATEALPHABLENDENABLE: /* SVGA3dBool */
        return "SVGA3D_RS_SEPARATEALPHABLENDENABLE";
    case SVGA3D_RS_SRCBLENDALPHA:          /* SVGA3dBlendOp */
        return "SVGA3D_RS_SRCBLENDALPHA";
    case SVGA3D_RS_DSTBLENDALPHA:          /* SVGA3dBlendOp */
        return "SVGA3D_RS_DSTBLENDALPHA";
    case SVGA3D_RS_BLENDEQUATIONALPHA:     /* SVGA3dBlendEquation */
        return "SVGA3D_RS_BLENDEQUATIONALPHA";
    case SVGA3D_RS_TRANSPARENCYANTIALIAS:  /* SVGA3dTransparencyAntialiasType */
        return "SVGA3D_RS_TRANSPARENCYANTIALIAS";
    case SVGA3D_RS_LINEAA:                 /* SVGA3dBool */
        return "SVGA3D_RS_LINEAA";
    case SVGA3D_RS_LINEWIDTH:              /* float */
        return "SVGA3D_RS_LINEWIDTH";
    default:
        return "UNKNOWN";
    }
}

const char *vmsvga3dTextureStateToString(SVGA3dTextureStateName textureState)
{
    switch (textureState)
    {
    case SVGA3D_TS_BIND_TEXTURE:
        return "SVGA3D_TS_BIND_TEXTURE";
    case SVGA3D_TS_COLOROP:
        return "SVGA3D_TS_COLOROP";
    case SVGA3D_TS_COLORARG1:
        return "SVGA3D_TS_COLORARG1";
    case SVGA3D_TS_COLORARG2:
        return "SVGA3D_TS_COLORARG2";
    case SVGA3D_TS_ALPHAOP:
        return "SVGA3D_TS_ALPHAOP";
    case SVGA3D_TS_ALPHAARG1:
        return "SVGA3D_TS_ALPHAARG1";
    case SVGA3D_TS_ALPHAARG2:
        return "SVGA3D_TS_ALPHAARG2";
    case SVGA3D_TS_ADDRESSU:
        return "SVGA3D_TS_ADDRESSU";
    case SVGA3D_TS_ADDRESSV:
        return "SVGA3D_TS_ADDRESSV";
    case SVGA3D_TS_MIPFILTER:
        return "SVGA3D_TS_MIPFILTER";
    case SVGA3D_TS_MAGFILTER:
        return "SVGA3D_TS_MAGFILTER";
    case SVGA3D_TS_MINFILTER:
        return "SVGA3D_TS_MINFILTER";
    case SVGA3D_TS_BORDERCOLOR:
        return "SVGA3D_TS_BORDERCOLOR";
    case SVGA3D_TS_TEXCOORDINDEX:
        return "SVGA3D_TS_TEXCOORDINDEX";
    case SVGA3D_TS_TEXTURETRANSFORMFLAGS:
        return "SVGA3D_TS_TEXTURETRANSFORMFLAGS";
    case SVGA3D_TS_TEXCOORDGEN:
        return "SVGA3D_TS_TEXCOORDGEN";
    case SVGA3D_TS_BUMPENVMAT00:
        return "SVGA3D_TS_BUMPENVMAT00";
    case SVGA3D_TS_BUMPENVMAT01:
        return "SVGA3D_TS_BUMPENVMAT01";
    case SVGA3D_TS_BUMPENVMAT10:
        return "SVGA3D_TS_BUMPENVMAT10";
    case SVGA3D_TS_BUMPENVMAT11:
        return "SVGA3D_TS_BUMPENVMAT11";
    case SVGA3D_TS_TEXTURE_MIPMAP_LEVEL:
        return "SVGA3D_TS_TEXTURE_MIPMAP_LEVEL";
    case SVGA3D_TS_TEXTURE_LOD_BIAS:
        return "SVGA3D_TS_TEXTURE_LOD_BIAS";
    case SVGA3D_TS_TEXTURE_ANISOTROPIC_LEVEL:
        return "SVGA3D_TS_TEXTURE_ANISOTROPIC_LEVEL";
    case SVGA3D_TS_ADDRESSW:
        return "SVGA3D_TS_ADDRESSW";
    case SVGA3D_TS_GAMMA:
        return "SVGA3D_TS_GAMMA";
    case SVGA3D_TS_BUMPENVLSCALE:
        return "SVGA3D_TS_BUMPENVLSCALE";
    case SVGA3D_TS_BUMPENVLOFFSET:
        return "SVGA3D_TS_BUMPENVLOFFSET";
    case SVGA3D_TS_COLORARG0:
        return "SVGA3D_TS_COLORARG0";
    case SVGA3D_TS_ALPHAARG0:
        return "SVGA3D_TS_ALPHAARG0";
    default:
        return "UNKNOWN";
    }
}

const char *vmsvgaTransformToString(SVGA3dTransformType type)
{
    switch (type)
    {
    case SVGA3D_TRANSFORM_INVALID:
        return "SVGA3D_TRANSFORM_INVALID";
    case SVGA3D_TRANSFORM_WORLD:
        return "SVGA3D_TRANSFORM_WORLD";
    case SVGA3D_TRANSFORM_VIEW:
        return "SVGA3D_TRANSFORM_VIEW";
    case SVGA3D_TRANSFORM_PROJECTION:
        return "SVGA3D_TRANSFORM_PROJECTION";
    case SVGA3D_TRANSFORM_TEXTURE0:
        return "SVGA3D_TRANSFORM_TEXTURE0";
    case SVGA3D_TRANSFORM_TEXTURE1:
        return "SVGA3D_TRANSFORM_TEXTURE1";
    case SVGA3D_TRANSFORM_TEXTURE2:
        return "SVGA3D_TRANSFORM_TEXTURE2";
    case SVGA3D_TRANSFORM_TEXTURE3:
        return "SVGA3D_TRANSFORM_TEXTURE3";
    case SVGA3D_TRANSFORM_TEXTURE4:
        return "SVGA3D_TRANSFORM_TEXTURE4";
    case SVGA3D_TRANSFORM_TEXTURE5:
        return "SVGA3D_TRANSFORM_TEXTURE5";
    case SVGA3D_TRANSFORM_TEXTURE6:
        return "SVGA3D_TRANSFORM_TEXTURE6";
    case SVGA3D_TRANSFORM_TEXTURE7:
        return "SVGA3D_TRANSFORM_TEXTURE7";
    case SVGA3D_TRANSFORM_WORLD1:
        return "SVGA3D_TRANSFORM_WORLD1";
    case SVGA3D_TRANSFORM_WORLD2:
        return "SVGA3D_TRANSFORM_WORLD2";
    case SVGA3D_TRANSFORM_WORLD3:
        return "SVGA3D_TRANSFORM_WORLD3";
    default:
        return "UNKNOWN";
    }
}

const char *vmsvgaDeclUsage2String(SVGA3dDeclUsage usage)
{
    switch (usage)
    {
    case SVGA3D_DECLUSAGE_POSITION:
        return "SVGA3D_DECLUSAGE_POSITION";
    case SVGA3D_DECLUSAGE_BLENDWEIGHT:
        return "SVGA3D_DECLUSAGE_BLENDWEIGHT";
    case SVGA3D_DECLUSAGE_BLENDINDICES:
        return "SVGA3D_DECLUSAGE_BLENDINDICES";
    case SVGA3D_DECLUSAGE_NORMAL:
        return "SVGA3D_DECLUSAGE_NORMAL";
    case SVGA3D_DECLUSAGE_PSIZE:
        return "SVGA3D_DECLUSAGE_PSIZE";
    case SVGA3D_DECLUSAGE_TEXCOORD:
        return "SVGA3D_DECLUSAGE_TEXCOORD";
    case SVGA3D_DECLUSAGE_TANGENT:
        return "SVGA3D_DECLUSAGE_TANGENT";
    case SVGA3D_DECLUSAGE_BINORMAL:
        return "SVGA3D_DECLUSAGE_BINORMAL";
    case SVGA3D_DECLUSAGE_TESSFACTOR:
        return "SVGA3D_DECLUSAGE_TESSFACTOR";
    case SVGA3D_DECLUSAGE_POSITIONT:
        return "SVGA3D_DECLUSAGE_POSITIONT";
    case SVGA3D_DECLUSAGE_COLOR:
        return "SVGA3D_DECLUSAGE_COLOR";
    case SVGA3D_DECLUSAGE_FOG:
        return "SVGA3D_DECLUSAGE_FOG";
    case SVGA3D_DECLUSAGE_DEPTH:
        return "SVGA3D_DECLUSAGE_DEPTH";
    case SVGA3D_DECLUSAGE_SAMPLE:
        return "SVGA3D_DECLUSAGE_SAMPLE";
    default:
        return "UNKNOWN!!";
    }
}

const char *vmsvgaDeclMethod2String(SVGA3dDeclMethod method)
{
    switch (method)
    {
    case SVGA3D_DECLMETHOD_DEFAULT:
        return "SVGA3D_DECLMETHOD_DEFAULT";
    case SVGA3D_DECLMETHOD_PARTIALU:
        return "SVGA3D_DECLMETHOD_PARTIALU";
    case SVGA3D_DECLMETHOD_PARTIALV:
        return "SVGA3D_DECLMETHOD_PARTIALV";
    case SVGA3D_DECLMETHOD_CROSSUV:
        return "SVGA3D_DECLMETHOD_CROSSUV";
    case SVGA3D_DECLMETHOD_UV:
        return "SVGA3D_DECLMETHOD_UV";
    case SVGA3D_DECLMETHOD_LOOKUP:
        return "SVGA3D_DECLMETHOD_LOOKUP";
    case SVGA3D_DECLMETHOD_LOOKUPPRESAMPLED:
        return "SVGA3D_DECLMETHOD_LOOKUPPRESAMPLED";
    default:
        return "UNKNOWN!!";
    }
}

const char *vmsvgaDeclType2String(SVGA3dDeclType type)
{
    switch (type)
    {
    case SVGA3D_DECLTYPE_FLOAT1:
        return "SVGA3D_DECLTYPE_FLOAT1";
    case SVGA3D_DECLTYPE_FLOAT2:
        return "SVGA3D_DECLTYPE_FLOAT2";
    case SVGA3D_DECLTYPE_FLOAT3:
        return "SVGA3D_DECLTYPE_FLOAT3";
    case SVGA3D_DECLTYPE_FLOAT4:
        return "SVGA3D_DECLTYPE_FLOAT4";
    case SVGA3D_DECLTYPE_D3DCOLOR:
        return "SVGA3D_DECLTYPE_D3DCOLOR";
    case SVGA3D_DECLTYPE_UBYTE4:
        return "SVGA3D_DECLTYPE_UBYTE4";
    case SVGA3D_DECLTYPE_SHORT2:
        return "SVGA3D_DECLTYPE_SHORT2";
    case SVGA3D_DECLTYPE_SHORT4:
        return "SVGA3D_DECLTYPE_SHORT4";
    case SVGA3D_DECLTYPE_UBYTE4N:
        return "SVGA3D_DECLTYPE_UBYTE4N";
    case SVGA3D_DECLTYPE_SHORT2N:
        return "SVGA3D_DECLTYPE_SHORT2N";
    case SVGA3D_DECLTYPE_SHORT4N:
        return "SVGA3D_DECLTYPE_SHORT4N";
    case SVGA3D_DECLTYPE_USHORT2N:
        return "SVGA3D_DECLTYPE_USHORT2N";
    case SVGA3D_DECLTYPE_USHORT4N:
        return "SVGA3D_DECLTYPE_USHORT4N";
    case SVGA3D_DECLTYPE_UDEC3:
        return "SVGA3D_DECLTYPE_UDEC3";
    case SVGA3D_DECLTYPE_DEC3N:
        return "SVGA3D_DECLTYPE_DEC3N";
    case SVGA3D_DECLTYPE_FLOAT16_2:
        return "SVGA3D_DECLTYPE_FLOAT16_2";
    case SVGA3D_DECLTYPE_FLOAT16_4:
        return "SVGA3D_DECLTYPE_FLOAT16_4";
    default:
        return "UNKNOWN!!";
    }
}

const char *vmsvga3dPrimitiveType2String(SVGA3dPrimitiveType PrimitiveType)
{
    switch (PrimitiveType)
    {
    case SVGA3D_PRIMITIVE_TRIANGLELIST:
        return "SVGA3D_PRIMITIVE_TRIANGLELIST";
    case SVGA3D_PRIMITIVE_POINTLIST:
        return "SVGA3D_PRIMITIVE_POINTLIST";
    case SVGA3D_PRIMITIVE_LINELIST:
        return "SVGA3D_PRIMITIVE_LINELIST";
    case SVGA3D_PRIMITIVE_LINESTRIP:
        return "SVGA3D_PRIMITIVE_LINESTRIP";
    case SVGA3D_PRIMITIVE_TRIANGLESTRIP:
        return "SVGA3D_PRIMITIVE_TRIANGLESTRIP";
    case SVGA3D_PRIMITIVE_TRIANGLEFAN:
        return "SVGA3D_PRIMITIVE_TRIANGLEFAN";
    default:
        return "UNKNOWN";
    }
}

#endif /* LOG_ENABLED */

/** Unsigned coordinates in pBox. Clip to [0; pSizeSrc), [0;pSizeDest).
 *
 * @param pSizeSrc  Source surface dimensions.
 * @param pSizeDest Destination surface dimensions.
 * @param pBox      Coordinates to be clipped.
 */
void vmsvgaClipCopyBox(const SVGA3dSize *pSizeSrc,
                       const SVGA3dSize *pSizeDest,
                       SVGA3dCopyBox *pBox)
{
    /* Src x, w */
    if (pBox->srcx > pSizeSrc->width)
        pBox->srcx = pSizeSrc->width;
    if (pBox->w > pSizeSrc->width - pBox->srcx)
        pBox->w = pSizeSrc->width - pBox->srcx;

    /* Src y, h */
    if (pBox->srcy > pSizeSrc->height)
        pBox->srcy = pSizeSrc->height;
    if (pBox->h > pSizeSrc->height - pBox->srcy)
        pBox->h = pSizeSrc->height - pBox->srcy;

    /* Src z, d */
    if (pBox->srcz > pSizeSrc->depth)
        pBox->srcz = pSizeSrc->depth;
    if (pBox->d > pSizeSrc->depth - pBox->srcz)
        pBox->d = pSizeSrc->depth - pBox->srcz;

    /* Dest x, w */
    if (pBox->x > pSizeDest->width)
        pBox->x = pSizeDest->width;
    if (pBox->w > pSizeDest->width - pBox->x)
        pBox->w = pSizeDest->width - pBox->x;

    /* Dest y, h */
    if (pBox->y > pSizeDest->height)
        pBox->y = pSizeDest->height;
    if (pBox->h > pSizeDest->height - pBox->y)
        pBox->h = pSizeDest->height - pBox->y;

    /* Dest z, d */
    if (pBox->z > pSizeDest->depth)
        pBox->z = pSizeDest->depth;
    if (pBox->d > pSizeDest->depth - pBox->z)
        pBox->d = pSizeDest->depth - pBox->z;
}

/** Unsigned coordinates in pBox. Clip to [0; pSize).
 *
 * @param pSize     Source surface dimensions.
 * @param pBox      Coordinates to be clipped.
 */
void vmsvgaClipBox(const SVGA3dSize *pSize,
                   SVGA3dBox *pBox)
{
    /* x, w */
    if (pBox->x > pSize->width)
        pBox->x = pSize->width;
    if (pBox->w > pSize->width - pBox->x)
        pBox->w = pSize->width - pBox->x;

    /* y, h */
    if (pBox->y > pSize->height)
        pBox->y = pSize->height;
    if (pBox->h > pSize->height - pBox->y)
        pBox->h = pSize->height - pBox->y;

    /* z, d */
    if (pBox->z > pSize->depth)
        pBox->z = pSize->depth;
    if (pBox->d > pSize->depth - pBox->z)
        pBox->d = pSize->depth - pBox->z;
}
