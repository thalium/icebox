/* $Id: icd_drv.c $ */
/** @file
 * VBox OpenGL windows ICD driver functions
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "cr_error.h"
#include "icd_drv.h"
#include "cr_gl.h"
#include "stub.h"
#include "cr_mem.h"

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
# include <VBoxCrHgsmi.h>
# include <VBoxUhgsmi.h>
#endif

#include <iprt/win/windows.h>

/// @todo consider
/* We can modify chronium dispatch table functions order to match the one required by ICD,
 * but it'd render us incompatible with other chromium SPUs and require more changes.
 * In current state, we can use unmodified binary chromium SPUs. Question is do we need it?
*/

#define GL_FUNC(func) cr_gl##func

static ICDTABLE icdTable = { 336, {
#define ICD_ENTRY(func) (PROC)GL_FUNC(func),
#include "VBoxICDList.h"
#undef ICD_ENTRY
} };

/* Currently host part will misbehave re-creating context with proper visual bits
 * if contexts with alternative visual bits is requested.
 * For now we just report a superset of all visual bits to avoid that.
 * Better to it on the host side as well?
 * We could also implement properly multiple pixel formats,
 * which should be done by implementing offscreen rendering or multiple host contexts.
 * */
#define VBOX_CROGL_USE_VBITS_SUPERSET

#ifdef VBOX_CROGL_USE_VBITS_SUPERSET
static GLuint desiredVisual = CR_RGB_BIT | CR_ALPHA_BIT | CR_DEPTH_BIT | CR_STENCIL_BIT | CR_ACCUM_BIT | CR_DOUBLE_BIT;
#else
static GLuint desiredVisual = CR_RGB_BIT;
#endif

#ifndef VBOX_CROGL_USE_VBITS_SUPERSET
/**
 * Compute a mask of CR_*_BIT flags which reflects the attributes of
 * the pixel format of the given hdc.
 */
static GLuint ComputeVisBits( HDC hdc )
{
    PIXELFORMATDESCRIPTOR pfd;
    int iPixelFormat;
    GLuint b = 0;

    iPixelFormat = GetPixelFormat( hdc );

    DescribePixelFormat( hdc, iPixelFormat, sizeof(pfd), &pfd );

    if (pfd.cDepthBits > 0)
        b |= CR_DEPTH_BIT;
    if (pfd.cAccumBits > 0)
        b |= CR_ACCUM_BIT;
    if (pfd.cColorBits > 8)
        b |= CR_RGB_BIT;
    if (pfd.cStencilBits > 0)
        b |= CR_STENCIL_BIT;
    if (pfd.cAlphaBits > 0)
        b |= CR_ALPHA_BIT;
    if (pfd.dwFlags & PFD_DOUBLEBUFFER)
        b |= CR_DOUBLE_BIT;
    if (pfd.dwFlags & PFD_STEREO)
        b |= CR_STEREO_BIT;

    return b;
}
#endif

void APIENTRY DrvReleaseContext(HGLRC hglrc)
{
     CR_DDI_PROLOGUE();
    /*crDebug( "DrvReleaseContext(0x%x) called", hglrc );*/
    stubMakeCurrent( NULL, NULL );
}

BOOL APIENTRY DrvValidateVersion(DWORD version)
{
    CR_DDI_PROLOGUE();
    if (stubInit()) {
        crDebug("DrvValidateVersion %x -> TRUE\n", version);
        return TRUE;
    }

    crDebug("DrvValidateVersion %x -> FALSE, going to use system default opengl32.dll\n", version);
    return FALSE;
}

//we're not going to change icdTable at runtime, so callback is unused
PICDTABLE APIENTRY DrvSetContext(HDC hdc, HGLRC hglrc, void *callback)
{
    ContextInfo *pContext;
    WindowInfo  *pWindowInfo;
    BOOL ret = false;

    CR_DDI_PROLOGUE();

    (void) (callback);

    crHashtableLock(stub.windowTable);
    crHashtableLock(stub.contextTable);

    pContext = (ContextInfo *) crHashtableSearch(stub.contextTable, (unsigned long) hglrc);
    if (pContext)
    {
        pWindowInfo = stubGetWindowInfo(hdc);
        if (pWindowInfo)
            ret = stubMakeCurrent(pWindowInfo, pContext);
        else
            crError("no window info available.");
    }
    else
        crError("No context found.");

    crHashtableUnlock(stub.contextTable);
    crHashtableUnlock(stub.windowTable);

    return ret ? &icdTable : NULL;
}

BOOL APIENTRY DrvSetPixelFormat(HDC hdc, int iPixelFormat)
{
    CR_DDI_PROLOGUE();
    crDebug( "DrvSetPixelFormat(0x%x, %i) called.", hdc, iPixelFormat );

    if ( (iPixelFormat<1) || (iPixelFormat>2) ) {
        crError( "wglSetPixelFormat: iPixelFormat=%d?", iPixelFormat );
    }

    return 1;
}

HGLRC APIENTRY DrvCreateContext(HDC hdc)
{
    char dpyName[MAX_DPY_NAME];
    ContextInfo *context;
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    PVBOXUHGSMI pHgsmi = NULL;
#endif

    CR_DDI_PROLOGUE();

    crDebug( "DrvCreateContext(0x%x) called.", hdc);

    stubInit();

    CRASSERT(stub.contextTable);

    sprintf(dpyName, "%p", hdc);
#ifndef VBOX_CROGL_USE_VBITS_SUPERSET
    if (stub.haveNativeOpenGL)
        desiredVisual |= ComputeVisBits( hdc );
#endif

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    pHgsmi = VBoxCrHgsmiCreate();
#endif

    context = stubNewContext(dpyName, desiredVisual, UNDECIDED, 0
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        , pHgsmi
#endif
            );
    if (!context)
        return 0;

    return (HGLRC) context->id;
}

HGLRC APIENTRY DrvCreateLayerContext(HDC hdc, int iLayerPlane)
{
    CR_DDI_PROLOGUE();
    crDebug( "DrvCreateLayerContext(0x%x, %i) called.", hdc, iLayerPlane);
    //We don't support more than 1 layers.
    if (iLayerPlane == 0) {
        return DrvCreateContext(hdc);
    } else {
        crError( "DrvCreateLayerContext (%x,%x): unsupported", hdc, iLayerPlane);
        return NULL;
    }

}

BOOL APIENTRY DrvDescribeLayerPlane(HDC hdc,int iPixelFormat,
                                    int iLayerPlane, UINT nBytes,
                                    LPLAYERPLANEDESCRIPTOR plpd)
{
    CR_DDI_PROLOGUE();
    crWarning( "DrvDescribeLayerPlane: unimplemented" );
    CRASSERT(false);
    return 0;
}

int APIENTRY DrvGetLayerPaletteEntries(HDC hdc, int iLayerPlane,
                                       int iStart, int cEntries,
                                       COLORREF *pcr)
{
    CR_DDI_PROLOGUE();
    crWarning( "DrvGetLayerPaletteEntries: unsupported" );
    CRASSERT(false);
    return 0;
}

int APIENTRY DrvDescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR pfd)
{
    CR_DDI_PROLOGUE();
    if ( !pfd ) {
        return 2;
    }

    if ( nBytes != sizeof(*pfd) ) {
        crWarning( "DrvDescribePixelFormat: nBytes=%u?", nBytes );
        return 2;
    }

    if (iPixelFormat==1)
    {
        crMemZero(pfd, sizeof(*pfd));

        pfd->nSize           = sizeof(*pfd);
        pfd->nVersion        = 1;
        pfd->dwFlags         = (PFD_DRAW_TO_WINDOW |
                                PFD_SUPPORT_OPENGL |
                                PFD_DOUBLEBUFFER);

        pfd->dwFlags         |= 0x8000; /* <- Needed for VSG Open Inventor to be happy */

        pfd->iPixelType      = PFD_TYPE_RGBA;
        pfd->cColorBits      = 32;
        pfd->cRedBits        = 8;
        pfd->cRedShift       = 24;
        pfd->cGreenBits      = 8;
        pfd->cGreenShift     = 16;
        pfd->cBlueBits       = 8;
        pfd->cBlueShift      = 8;
        pfd->cAlphaBits      = 8;
        pfd->cAlphaShift     = 0;
        pfd->cAccumBits      = 0;
        pfd->cAccumRedBits   = 0;
        pfd->cAccumGreenBits = 0;
        pfd->cAccumBlueBits  = 0;
        pfd->cAccumAlphaBits = 0;
        pfd->cDepthBits      = 32;
        pfd->cStencilBits    = 8;
        pfd->cAuxBuffers     = 0;
        pfd->iLayerType      = PFD_MAIN_PLANE;
        pfd->bReserved       = 0;
        pfd->dwLayerMask     = 0;
        pfd->dwVisibleMask   = 0;
        pfd->dwDamageMask    = 0;
    }
    else
    {
        crMemZero(pfd, sizeof(*pfd));
        pfd->nVersion        = 1;
        pfd->dwFlags         = (PFD_DRAW_TO_WINDOW|
                                PFD_SUPPORT_OPENGL);

        pfd->iPixelType      = PFD_TYPE_RGBA;
        pfd->cColorBits      = 32;
        pfd->cRedBits        = 8;
        pfd->cRedShift       = 16;
        pfd->cGreenBits      = 8;
        pfd->cGreenShift     = 8;
        pfd->cBlueBits       = 8;
        pfd->cBlueShift      = 0;
        pfd->cAlphaBits      = 0;
        pfd->cAlphaShift     = 0;
        pfd->cAccumBits      = 64;
        pfd->cAccumRedBits   = 16;
        pfd->cAccumGreenBits = 16;
        pfd->cAccumBlueBits  = 16;
        pfd->cAccumAlphaBits = 0;
        pfd->cDepthBits      = 16;
        pfd->cStencilBits    = 8;
        pfd->cAuxBuffers     = 0;
        pfd->iLayerType      = PFD_MAIN_PLANE;
        pfd->bReserved       = 0;
        pfd->dwLayerMask     = 0;
        pfd->dwVisibleMask   = 0;
        pfd->dwDamageMask    = 0;
    }

    /* the max PFD index */
    return 2;
}

BOOL APIENTRY DrvDeleteContext(HGLRC hglrc)
{
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    ContextInfo *pContext;
    PVBOXUHGSMI pHgsmi = NULL;
#endif

    CR_DDI_PROLOGUE();
    crDebug( "DrvDeleteContext(0x%x) called", hglrc );

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    crHashtableLock(stub.contextTable);

    pContext = (ContextInfo *) crHashtableSearch(stub.contextTable, (unsigned long) hglrc);
    if (pContext)
        pHgsmi = pContext->pHgsmi;

    crHashtableUnlock(stub.contextTable);
#endif

    stubDestroyContext( (unsigned long) hglrc );

#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
    if (pHgsmi)
        VBoxCrHgsmiDestroy(pHgsmi);
#endif

    return true;
}

BOOL APIENTRY DrvCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
    CR_DDI_PROLOGUE();
    crWarning( "DrvCopyContext: unsupported" );
    return 0;
}

DECLEXPORT(BOOL) WINAPI wglShareLists_prox( HGLRC hglrc1, HGLRC hglrc2 );

BOOL APIENTRY DrvShareLists(HGLRC hglrc1, HGLRC hglrc2)
{
    return wglShareLists_prox(hglrc1, hglrc2);
}

int APIENTRY DrvSetLayerPaletteEntries(HDC hdc, int iLayerPlane,
                                       int iStart, int cEntries,
                                       CONST COLORREF *pcr)
{
    CR_DDI_PROLOGUE();
    crWarning( "DrvSetLayerPaletteEntries: unsupported" );
    return 0;
}


BOOL APIENTRY DrvRealizeLayerPalette(HDC hdc, int iLayerPlane, BOOL bRealize)
{
    CR_DDI_PROLOGUE();
    crWarning( "DrvRealizeLayerPalette: unsupported" );
    return 0;
}

BOOL APIENTRY DrvSwapLayerBuffers(HDC hdc, UINT fuPlanes)
{
    CR_DDI_PROLOGUE();
    if (fuPlanes == 1)
    {
        return DrvSwapBuffers(hdc);
    }
    else
    {
        crWarning( "DrvSwapLayerBuffers: unsupported" );
        CRASSERT(false);
        return 0;
    }
}

BOOL APIENTRY DrvSwapBuffers(HDC hdc)
{
    WindowInfo *window;

    CR_DDI_PROLOGUE();
    /*crDebug( "DrvSwapBuffers(0x%x) called", hdc );*/
    window = stubGetWindowInfo(hdc);
    stubSwapBuffers( window, 0 );
    return 1;
}

