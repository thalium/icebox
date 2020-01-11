/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_error.h"
#include "cr_spu.h"
#include "cr_mem.h"
#include "stub.h"

/* I *know* most of the parameters are unused, dammit. */
#pragma warning( disable: 4100 )

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include <iprt/cdefs.h>
#include <iprt/env.h>

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

DECLEXPORT(int) WINAPI wglChoosePixelFormat_prox( HDC hdc, CONST PIXELFORMATDESCRIPTOR *pfd )
{
    DWORD okayFlags;

    CR_DDI_PROLOGUE();

    stubInit();

    /*
     * NOTE!!!
     * Here we're telling the renderspu not to use the GDI
     * equivalent's of ChoosePixelFormat/DescribePixelFormat etc
     * There are subtle differences in the use of these calls.
     */
    RTEnvSet("CR_WGL_DO_NOT_USE_GDI", "yes");

    if ( pfd->nSize != sizeof(*pfd) || pfd->nVersion != 1 ) {
        crError( "wglChoosePixelFormat: bad pfd\n" );
        return 0;
    }

    okayFlags = ( PFD_DRAW_TO_WINDOW        |
            PFD_SUPPORT_GDI           |
            PFD_SUPPORT_OPENGL        |
            PFD_DOUBLEBUFFER          |
            PFD_DOUBLEBUFFER_DONTCARE |
            PFD_SWAP_EXCHANGE         |
            PFD_SWAP_COPY             |
            /** @todo this is disabled due to VSG Open Inventor interop issues
             * it does not make any sense actually since reporting this
             * as well as choosing a pixel format with this cap would not do anything
             * since ICD stuff has its own pixelformat state var */
//            PFD_STEREO            |
            PFD_STEREO_DONTCARE       |
            PFD_DEPTH_DONTCARE        );
    if ( pfd->dwFlags & ~okayFlags ) {
        crWarning( "wglChoosePixelFormat: only support flags=0x%x, but you gave me flags=0x%x", okayFlags, pfd->dwFlags );
        return 0;
    }

    if ( pfd->iPixelType != PFD_TYPE_RGBA ) {
        crError( "wglChoosePixelFormat: only support RGBA\n" );
    }

    if ( pfd->cColorBits > 32 ||
            pfd->cRedBits   > 8  ||
            pfd->cGreenBits > 8  ||
            pfd->cBlueBits  > 8  ||
            pfd->cAlphaBits > 8 ) {
        crWarning( "wglChoosePixelFormat: too much color precision requested\n" );
    }

    if ( pfd->dwFlags & PFD_DOUBLEBUFFER )
        desiredVisual |= CR_DOUBLE_BIT;

    if ( pfd->dwFlags & PFD_STEREO )
        desiredVisual |= CR_STEREO_BIT;

    if ( pfd->cColorBits > 8)
        desiredVisual |= CR_RGB_BIT;

    if ( pfd->cAccumBits      > 0 ||
            pfd->cAccumRedBits   > 0 ||
            pfd->cAccumGreenBits > 0 ||
            pfd->cAccumBlueBits  > 0 ||
            pfd->cAccumAlphaBits > 0 ) {
        crWarning( "wglChoosePixelFormat: asked for accumulation buffer, ignoring\n" );
    }

    if ( pfd->cAccumBits > 0 )
        desiredVisual |= CR_ACCUM_BIT;

    if ( pfd->cDepthBits > 32 ) {
        crError( "wglChoosePixelFormat; asked for too many depth bits\n" );
    }

    if ( pfd->cDepthBits > 0 )
        desiredVisual |= CR_DEPTH_BIT;

    if ( pfd->cStencilBits > 8 ) {
        crError( "wglChoosePixelFormat: asked for too many stencil bits\n" );
    }

    if ( pfd->cStencilBits > 0 )
        desiredVisual |= CR_STENCIL_BIT;

    if ( pfd->cAuxBuffers > 0 ) {
        crError( "wglChoosePixelFormat: asked for aux buffers\n" );
    }

    if ( pfd->iLayerType != PFD_MAIN_PLANE ) {
        crError( "wglChoosePixelFormat: asked for a strange layer\n" );
    }

    return 1;
}

DECLEXPORT(BOOL) WINAPI wglSetPixelFormat_prox( HDC hdc, int pixelFormat,
        CONST PIXELFORMATDESCRIPTOR *pdf )
{
    CR_DDI_PROLOGUE();

    if ( pixelFormat != 1 ) {
        crError( "wglSetPixelFormat: pixelFormat=%d?\n", pixelFormat );
    }

    return 1;
}

DECLEXPORT(BOOL) WINAPI wglDeleteContext_prox( HGLRC hglrc )
{
    CR_DDI_PROLOGUE();
    stubDestroyContext( (unsigned long) hglrc );
    return 1;
}

DECLEXPORT(BOOL) WINAPI wglMakeCurrent_prox( HDC hdc, HGLRC hglrc )
{
    ContextInfo *context;
    WindowInfo *window;
    BOOL ret;

    CR_DDI_PROLOGUE();

    crHashtableLock(stub.windowTable);
    crHashtableLock(stub.contextTable);

    context = (ContextInfo *) crHashtableSearch(stub.contextTable, (unsigned long) hglrc);
    window = stubGetWindowInfo(hdc);

    if (hglrc!=0 && !context)
    {
        crWarning("wglMakeCurrent got unexpected hglrc 0x%x", hglrc);
    }

    ret = stubMakeCurrent( window, context );

    crHashtableUnlock(stub.contextTable);
    crHashtableUnlock(stub.windowTable);

    return ret;
}

DECLEXPORT(HGLRC) WINAPI wglGetCurrentContext_prox( void )
{
    ContextInfo *context = stubGetCurrentContext();
    CR_DDI_PROLOGUE();
    return (HGLRC) (context ? context->id : 0);
}

DECLEXPORT(HDC) WINAPI wglGetCurrentDC_prox( void )
{
    ContextInfo *context = stubGetCurrentContext();
    CR_DDI_PROLOGUE();
    if (context && context->currentDrawable)
        return (HDC) context->currentDrawable->drawable;
    else
        return (HDC) NULL;
}

DECLEXPORT(int) WINAPI wglGetPixelFormat_prox( HDC hdc )
{
    CR_DDI_PROLOGUE();
    /* this is what we call our generic pixelformat, regardless of the HDC */
    return 1;
}

DECLEXPORT(int) WINAPI wglDescribePixelFormat_prox( HDC hdc, int pixelFormat, UINT nBytes,
        LPPIXELFORMATDESCRIPTOR pfd )
{
    CR_DDI_PROLOGUE();

/*  if ( pixelFormat != 1 ) {
 *      crError( "wglDescribePixelFormat: pixelFormat=%d?\n", pixelFormat );
 *      return 0;
 *  } */

    if ( !pfd ) {
        crWarning( "wglDescribePixelFormat: pfd=NULL\n" );
        return 1; /* There's only one, baby */
    }

    if ( nBytes != sizeof(*pfd) ) {
        crWarning( "wglDescribePixelFormat: nBytes=%u?\n", nBytes );
        return 1; /* There's only one, baby */
    }

    pfd->nSize           = sizeof(*pfd);
    pfd->nVersion        = 1;
    pfd->dwFlags         = ( PFD_DRAW_TO_WINDOW |
                 PFD_SUPPORT_GDI    |
                 PFD_SUPPORT_OPENGL |
                 PFD_DOUBLEBUFFER );
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

    /* the max PFD index */
    return 1;
}

DECLEXPORT(void) WINAPI VBoxCtxChromiumParameteriCR(HGLRC hglrc, GLenum param, GLint value)
{
    ContextInfo *context;

    CR_DDI_PROLOGUE();

//    crHashtableLock(stub.windowTable);
    crHashtableLock(stub.contextTable);

    context = (ContextInfo *) crHashtableSearch(stub.contextTable, (unsigned long) hglrc);

    if (context)
    {
        stubCtxCheckCreate(context);
        stubConChromiumParameteriCR(CR_CTX_CON(context), param, value);
    }
    else
        crWarning("invalid context %#x", hglrc);

    crHashtableUnlock(stub.contextTable);
//    crHashtableUnlock(stub.windowTable);
}

DECLEXPORT(BOOL) WINAPI wglShareLists_prox( HGLRC hglrc1, HGLRC hglrc2 )
{
    ContextInfo *context1, *context2;
    GLint aSpuContexts[2];

    CR_DDI_PROLOGUE();

//    crHashtableLock(stub.windowTable);
    crHashtableLock(stub.contextTable);

    context1 = (ContextInfo *) crHashtableSearch(stub.contextTable, (unsigned long) hglrc1);

    if (!context1)
    {
        WARN(("invalid hglrc1"));
        return FALSE;
    }

    stubCtxCheckCreate(context1);

    context2 = (ContextInfo *) crHashtableSearch(stub.contextTable, (unsigned long) hglrc2);

    if (!context2)
    {
        WARN(("invalid hglrc2"));
        return FALSE;
    }

    stubCtxCheckCreate(context2);

    aSpuContexts[0] = context1->spuContext;
    aSpuContexts[1] = context2->spuContext;

    stubConChromiumParametervCR(CR_CTX_CON(context2), GL_SHARE_LISTS_CR, GL_INT, 2, aSpuContexts);

    crHashtableUnlock(stub.contextTable);

    return TRUE;
}

DECLEXPORT(HGLRC) WINAPI VBoxCreateContext( HDC hdc, struct VBOXUHGSMI *pHgsmi )
{
    char *dpyName;
    ContextInfo *context;

    CR_DDI_PROLOGUE();

    stubInit();

    CRASSERT(stub.contextTable);

    dpyName = crCalloc(MAX_DPY_NAME);
    if (dpyName)
    {
        crMemset(dpyName, 0, MAX_DPY_NAME);
        sprintf(dpyName, "%p", hdc);
    }
#ifndef VBOX_CROGL_USE_VBITS_SUPERSET
    if (stub.haveNativeOpenGL)
        desiredVisual |= ComputeVisBits( hdc );
#endif

    context = stubNewContext(dpyName, desiredVisual, UNDECIDED, 0
#if defined(VBOX_WITH_CRHGSMI) && defined(IN_GUEST)
        , pHgsmi
#else
        , NULL
#endif
            );
    /* Not needed any more. */
    crFree(dpyName);

    if (!context)
        return 0;

    return (HGLRC) context->id;
}

DECLEXPORT(GLint) WINAPI VBoxGetWindowId( HDC hdc )
{
    WindowInfo *window;
    GLint winid = 0;

    CR_DDI_PROLOGUE();

    crHashtableLock(stub.windowTable);

    window = stubGetWindowInfo(hdc);
    if (!window)
    {
        crWarning("stubGetWindowInfo: window not found!");
        goto end;
    }
    if (!window->spuWindow)
    {
        crWarning("stubGetWindowInfo: window is null!");
        goto end;
    }

    winid = window->spuWindow;

end:
    crHashtableUnlock(stub.windowTable);
    return winid;
}

DECLEXPORT(GLint) WINAPI VBoxGetContextId( HGLRC hglrc )
{
    ContextInfo *context;
    GLint ctxid = 0;

    CR_DDI_PROLOGUE();

//    crHashtableLock(stub.windowTable);
    crHashtableLock(stub.contextTable);

    context = (ContextInfo *) crHashtableSearch(stub.contextTable, (unsigned long) hglrc);
    if (!context)
    {
        crWarning("crHashtableSearch: context not found!");
        goto end;
    }

    if (context->type != CHROMIUM)
    {
        crWarning("unexpected context type %d", context->type);
        goto end;
    }

    if (context->spuContext <= 0)
    {
        crWarning("no spuSontext defined");
        goto end;
    }

    ctxid = context->spuContext;

end:
    crHashtableUnlock(stub.contextTable);
    return ctxid;
}


DECLEXPORT(HGLRC) WINAPI wglCreateContext_prox( HDC hdc )
{
    return VBoxCreateContext(hdc, NULL);
}

DECLEXPORT(void) WINAPI VBoxFlushToHost ( HGLRC hglrc )
{
    ContextInfo *context;

    CR_DDI_PROLOGUE();

//    crHashtableLock(stub.windowTable);
    crHashtableLock(stub.contextTable);

    context = (ContextInfo *) crHashtableSearch(stub.contextTable, (unsigned long) hglrc);

    if (context)
        stubConFlush(CR_CTX_CON(context));
    else
        crWarning("invalid context %#x", hglrc);

    crHashtableUnlock(stub.contextTable);
//    crHashtableUnlock(stub.windowTable);
}

DECLEXPORT(BOOL) WINAPI
wglSwapBuffers_prox( HDC hdc )
{
    WindowInfo *window = stubGetWindowInfo(hdc);
    CR_DDI_PROLOGUE();
    stubSwapBuffers( window, 0 );
    return 1;
}

DECLEXPORT(BOOL) WINAPI wglCopyContext_prox( HGLRC src, HGLRC dst, UINT mask )
{
    CR_DDI_PROLOGUE();
    crWarning( "wglCopyContext: unsupported" );
    return 0;
}

DECLEXPORT(HGLRC) WINAPI wglCreateLayerContext_prox( HDC hdc, int layerPlane )
{
    CR_DDI_PROLOGUE();
    stubInit();
    crWarning( "wglCreateLayerContext: unsupported" );
    return 0;
}

DECLEXPORT(PROC) WINAPI wglGetProcAddress_prox( LPCSTR name )
{
    CR_DDI_PROLOGUE();
    return (PROC) crGetProcAddress( name );
}

DECLEXPORT(BOOL) WINAPI wglUseFontBitmapsA_prox( HDC hdc, DWORD first, DWORD count, DWORD listBase )
{
    CR_DDI_PROLOGUE();
    crWarning( "wglUseFontBitmapsA: unsupported" );
    return 0;
}

DECLEXPORT(BOOL) WINAPI wglUseFontBitmapsW_prox( HDC hdc, DWORD first, DWORD count, DWORD listBase )
{
    CR_DDI_PROLOGUE();
    crWarning( "wglUseFontBitmapsW: unsupported" );
    return 0;
}

DECLEXPORT(BOOL) WINAPI wglDescribeLayerPlane_prox( HDC hdc, int pixelFormat, int layerPlane,
        UINT nBytes, LPLAYERPLANEDESCRIPTOR lpd )
{
    CR_DDI_PROLOGUE();
    crWarning( "wglDescribeLayerPlane: unimplemented" );
    return 0;
}

DECLEXPORT(int) WINAPI wglSetLayerPaletteEntries_prox( HDC hdc, int layerPlane, int start,
        int entries, CONST COLORREF *cr )
{
    CR_DDI_PROLOGUE();
    crWarning( "wglSetLayerPaletteEntries: unsupported" );
    return 0;
}

DECLEXPORT(int) WINAPI wglGetLayerPaletteEntries_prox( HDC hdc, int layerPlane, int start,
        int entries, COLORREF *cr )
{
    CR_DDI_PROLOGUE();
    crWarning( "wglGetLayerPaletteEntries: unsupported" );
    return 0;
}

DECLEXPORT(BOOL) WINAPI wglRealizeLayerPalette_prox( HDC hdc, int layerPlane, BOOL realize )
{
    CR_DDI_PROLOGUE();
    crWarning( "wglRealizeLayerPalette: unsupported" );
    return 0;
}

DECLEXPORT(DWORD) WINAPI wglSwapMultipleBuffers_prox( UINT a, CONST void *b )
{
    CR_DDI_PROLOGUE();
    crWarning( "wglSwapMultipleBuffer: unsupported" );
    return 0;
}

DECLEXPORT(BOOL) WINAPI wglUseFontOutlinesA_prox( HDC hdc, DWORD first, DWORD count, DWORD listBase,
        FLOAT deviation, FLOAT extrusion, int format,
        LPGLYPHMETRICSFLOAT gmf )
{
    CR_DDI_PROLOGUE();
    crWarning( "wglUseFontOutlinesA: unsupported" );
    return 0;
}

DECLEXPORT(BOOL) WINAPI wglUseFontOutlinesW_prox( HDC hdc, DWORD first, DWORD count, DWORD listBase,
        FLOAT deviation, FLOAT extrusion, int format,
        LPGLYPHMETRICSFLOAT gmf )
{
    CR_DDI_PROLOGUE();
    crWarning( "wglUseFontOutlinesW: unsupported" );
    return 0;
}

DECLEXPORT(BOOL) WINAPI wglSwapLayerBuffers_prox( HDC hdc, UINT planes )
{
    CR_DDI_PROLOGUE();
    if (planes == WGL_SWAP_MAIN_PLANE)
    {
        return wglSwapBuffers_prox(hdc);
    }
    else
    {
        crWarning( "wglSwapLayerBuffers: unsupported" );
        return 0;
    }
}

DECLEXPORT(BOOL) WINAPI wglChoosePixelFormatEXT_prox
(HDC hdc, const int *piAttributes, const FLOAT *pfAttributes, UINT nMaxFormats, int *piFormats, UINT *nNumFormats)
{
    int *pi;
    int wants_rgb = 0;

    CR_DDI_PROLOGUE();

    stubInit();

    /** @todo : Need to check pfAttributes too ! */

    for ( pi = (int *)piAttributes; *pi != 0; pi++ )
    {
        switch ( *pi )
        {
            case WGL_COLOR_BITS_EXT:
                if (pi[1] > 8)
                    wants_rgb = 1;
                pi++;
                break;

            case WGL_RED_BITS_EXT:
            case WGL_GREEN_BITS_EXT:
            case WGL_BLUE_BITS_EXT:
                if (pi[1] > 3)
                    wants_rgb = 1;
                pi++;
                break;

            case WGL_ACCUM_ALPHA_BITS_EXT:
            case WGL_ALPHA_BITS_EXT:
                if (pi[1] > 0)
                    desiredVisual |= CR_ALPHA_BIT;
                pi++;
                break;

            case WGL_DOUBLE_BUFFER_EXT:
                if (pi[1] > 0)
                    desiredVisual |= CR_DOUBLE_BIT;
                pi++;
                break;

            case WGL_STEREO_EXT:
                if (pi[1] > 0)
                {
                    /** @todo this is disabled due to VSG Open Inventor interop issues
                     * it does not make any sense actually since reporting this
                     * as well as choosing a pixel format with this cap would not do anything
                     * since ICD stuff has its own pixelformat state var */
                    crWarning("WGL_STEREO_EXT not supporteed!");
                    return 0;
//                    desiredVisual |= CR_STEREO_BIT;
                }
                pi++;
                break;

            case WGL_DEPTH_BITS_EXT:
                if (pi[1] > 0)
                    desiredVisual |= CR_DEPTH_BIT;
                pi++;
                break;

            case WGL_STENCIL_BITS_EXT:
                if (pi[1] > 0)
                    desiredVisual |= CR_STENCIL_BIT;
                pi++;
                break;

            case WGL_ACCUM_RED_BITS_EXT:
            case WGL_ACCUM_GREEN_BITS_EXT:
            case WGL_ACCUM_BLUE_BITS_EXT:
                if (pi[1] > 0)
                    desiredVisual |= CR_ACCUM_BIT;
                pi++;
                break;

            case WGL_SAMPLE_BUFFERS_EXT:
            case WGL_SAMPLES_EXT:
                if (pi[1] > 0)
                {
                    /** @todo this is disabled due to VSG Open Inventor interop issues
                     * it does not make any sense actually since reporting this
                     * as well as choosing a pixel format with this cap would not do anything
                     * since ICD stuff has its own pixelformat state var */
                    crWarning("WGL_SAMPLE_BUFFERS_EXT & WGL_SAMPLES_EXT not supporteed!");
                    return 0;
//                    desiredVisual |= CR_MULTISAMPLE_BIT;
                }
                pi++;
                break;

            case WGL_SUPPORT_OPENGL_ARB:
            case WGL_DRAW_TO_WINDOW_ARB:
            case WGL_ACCELERATION_ARB:
                pi++;
                break;

            case WGL_PIXEL_TYPE_ARB:
                if(pi[1]!=WGL_TYPE_RGBA_ARB)
                {
                    crWarning("WGL_PIXEL_TYPE 0x%x not supported!", pi[1]);
                    return 0;
                }
                pi++;
                break;

            default:
                crWarning( "wglChoosePixelFormatEXT: bad pi=0x%x", *pi );
                return 0;
        }
    }

    if (nNumFormats) *nNumFormats = 1;
    if (nMaxFormats>0 && piFormats)
    {
        piFormats[0] = 1;
    }

    return 1;
}

DECLEXPORT(BOOL) WINAPI wglGetPixelFormatAttribivEXT_prox
(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, int *pValues)
{
    UINT i;

    CR_DDI_PROLOGUE();

    if (!pValues || !piAttributes) return 0;

    if ((nAttributes!=1) || (piAttributes && piAttributes[0]!=WGL_NUMBER_PIXEL_FORMATS_ARB))
    {
        if (iPixelFormat!=1)
        {
            crDebug("wglGetPixelFormatAttribivARB: bad pf:%i", iPixelFormat);
            return 0;
        }
    }

    for (i=0; i<nAttributes; ++i)
    {
        switch (piAttributes[i])
        {
            case WGL_NUMBER_PIXEL_FORMATS_ARB:
                pValues[i] = 1;
                break;
            case WGL_DRAW_TO_WINDOW_ARB:
            case WGL_SUPPORT_OPENGL_ARB:
            case WGL_DOUBLE_BUFFER_ARB:
                pValues[i] = 1;
                break;
            case WGL_STEREO_ARB:
                /** @todo this is disabled due to VSG Open Inventor interop issues
                 * it does not make any sense actually since reporting this
                 * as well as choosing a pixel format with this cap would not do anything
                 * since ICD stuff has its own pixelformat state var */
                pValues[i] = 0;
                break;
            case WGL_DRAW_TO_BITMAP_ARB:
            case WGL_NEED_PALETTE_ARB:
            case WGL_NEED_SYSTEM_PALETTE_ARB:
            case WGL_SWAP_LAYER_BUFFERS_ARB:
            case WGL_NUMBER_OVERLAYS_ARB:
            case WGL_NUMBER_UNDERLAYS_ARB:
            case WGL_TRANSPARENT_ARB:
            case WGL_TRANSPARENT_RED_VALUE_ARB:
            case WGL_TRANSPARENT_GREEN_VALUE_ARB:
            case WGL_TRANSPARENT_BLUE_VALUE_ARB:
            case WGL_TRANSPARENT_ALPHA_VALUE_ARB:
            case WGL_TRANSPARENT_INDEX_VALUE_ARB:
            case WGL_SHARE_DEPTH_ARB:
            case WGL_SHARE_STENCIL_ARB:
            case WGL_SHARE_ACCUM_ARB:
            case WGL_SUPPORT_GDI_ARB:
                pValues[i] = 0;
                break;
            case WGL_ACCELERATION_ARB:
                pValues[i] = WGL_FULL_ACCELERATION_ARB;
                break;
            case WGL_SWAP_METHOD_ARB:
                pValues[i] = WGL_SWAP_UNDEFINED_ARB;
                break;
            case WGL_PIXEL_TYPE_ARB:
                pValues[i] = WGL_TYPE_RGBA_ARB;
                break;
            case WGL_COLOR_BITS_ARB:
                pValues[i] = 32;
                break;
            case WGL_RED_BITS_ARB:
            case WGL_GREEN_BITS_ARB:
            case WGL_BLUE_BITS_ARB:
            case WGL_ALPHA_BITS_ARB:
                pValues[i] = 8;
                break;
            case WGL_RED_SHIFT_ARB:
                pValues[i] = 24;
                break;
            case WGL_GREEN_SHIFT_ARB:
                pValues[i] = 16;
                break;
            case WGL_BLUE_SHIFT_ARB:
                pValues[i] = 8;
                break;
            case WGL_ALPHA_SHIFT_ARB:
                pValues[i] = 0;
                break;
            case WGL_ACCUM_BITS_ARB:
                pValues[i] = 0;
                break;
            case WGL_ACCUM_RED_BITS_ARB:
                pValues[i] = 0;
                break;
            case WGL_ACCUM_GREEN_BITS_ARB:
                pValues[i] = 0;
                break;
            case WGL_ACCUM_BLUE_BITS_ARB:
                pValues[i] = 0;
                break;
            case WGL_ACCUM_ALPHA_BITS_ARB:
                pValues[i] = 0;
                break;
            case WGL_DEPTH_BITS_ARB:
                pValues[i] = 32;
                break;
            case WGL_STENCIL_BITS_ARB:
                pValues[i] = 8;
                break;
            case WGL_AUX_BUFFERS_ARB:
                pValues[i] = 0;
                break;
            case WGL_SAMPLE_BUFFERS_EXT:
                /** @todo this is disabled due to VSG Open Inventor interop issues
                 * it does not make any sense actually since reporting this
                 * as well as choosing a pixel format with this cap would not do anything
                 * since ICD stuff has its own pixelformat state var */
                pValues[i] = 0;
                break;
            case WGL_SAMPLES_EXT:
                /** @todo this is disabled due to VSG Open Inventor interop issues
                 * it does not make any sense actually since reporting this
                 * as well as choosing a pixel format with this cap would not do anything
                 * since ICD stuff has its own pixelformat state var */
                pValues[i] = 0;
                break;
            case 0x202d: /* <- WGL_DRAW_TO_PBUFFER_ARB this is to make VSG Open Inventor happy */
                pValues[i] = 0;
                break;
            default:
                crWarning("wglGetPixelFormatAttribivARB: bad attrib=0x%x", piAttributes[i]);
                return 0;
        }
    }

    return 1;
}

DECLEXPORT(BOOL) WINAPI wglGetPixelFormatAttribfvEXT_prox
(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, float *pValues)
{
    UINT i;

    CR_DDI_PROLOGUE();

    if (!pValues || !piAttributes) return 0;

    if ((nAttributes!=1) || (piAttributes && piAttributes[0]!=WGL_NUMBER_PIXEL_FORMATS_ARB))
    {
        if (iPixelFormat!=1)
        {
            crDebug("wglGetPixelFormatAttribivARB: bad pf:%i", iPixelFormat);
            return 0;
        }
    }

    for (i=0; i<nAttributes; ++i)
    {
        switch (piAttributes[i])
        {
            case WGL_NUMBER_PIXEL_FORMATS_ARB:
                pValues[i] = 1.f;
                break;
            case WGL_DRAW_TO_WINDOW_ARB:
            case WGL_SUPPORT_OPENGL_ARB:
            case WGL_DOUBLE_BUFFER_ARB:
                pValues[i] = 1.f;
                break;
            case WGL_STEREO_ARB:
                /** @todo this is disabled due to VSG Open Inventor interop issues
                 * it does not make any sense actually since reporting this
                 * as well as choosing a pixel format with this cap would not do anything
                 * since ICD stuff has its own pixelformat state var */
                pValues[i] = 0.f;
                break;
            case WGL_DRAW_TO_BITMAP_ARB:
            case WGL_NEED_PALETTE_ARB:
            case WGL_NEED_SYSTEM_PALETTE_ARB:
            case WGL_SWAP_LAYER_BUFFERS_ARB:
            case WGL_NUMBER_OVERLAYS_ARB:
            case WGL_NUMBER_UNDERLAYS_ARB:
            case WGL_TRANSPARENT_ARB:
            case WGL_TRANSPARENT_RED_VALUE_ARB:
            case WGL_TRANSPARENT_GREEN_VALUE_ARB:
            case WGL_TRANSPARENT_BLUE_VALUE_ARB:
            case WGL_TRANSPARENT_ALPHA_VALUE_ARB:
            case WGL_TRANSPARENT_INDEX_VALUE_ARB:
            case WGL_SHARE_DEPTH_ARB:
            case WGL_SHARE_STENCIL_ARB:
            case WGL_SHARE_ACCUM_ARB:
            case WGL_SUPPORT_GDI_ARB:
                pValues[i] = 0.f;
                break;
            case WGL_ACCELERATION_ARB:
                pValues[i] = WGL_FULL_ACCELERATION_ARB;
                break;
            case WGL_SWAP_METHOD_ARB:
                pValues[i] = WGL_SWAP_UNDEFINED_ARB;
                break;
            case WGL_PIXEL_TYPE_ARB:
                pValues[i] = WGL_TYPE_RGBA_ARB;
                break;
            case WGL_COLOR_BITS_ARB:
                pValues[i] = 32.f;
                break;
            case WGL_RED_BITS_ARB:
            case WGL_GREEN_BITS_ARB:
            case WGL_BLUE_BITS_ARB:
            case WGL_ALPHA_BITS_ARB:
                pValues[i] = 8.f;
                break;
            case WGL_RED_SHIFT_ARB:
                pValues[i] = 24.f;
                break;
            case WGL_GREEN_SHIFT_ARB:
                pValues[i] = 16.f;
                break;
            case WGL_BLUE_SHIFT_ARB:
                pValues[i] = 8.f;
                break;
            case WGL_ALPHA_SHIFT_ARB:
                pValues[i] = 0.f;
                break;
            case WGL_ACCUM_BITS_ARB:
                pValues[i] = 0.f;
                break;
            case WGL_ACCUM_RED_BITS_ARB:
                pValues[i] = 0.f;
                break;
            case WGL_ACCUM_GREEN_BITS_ARB:
                pValues[i] = 0.f;
                break;
            case WGL_ACCUM_BLUE_BITS_ARB:
                pValues[i] = 0.f;
                break;
            case WGL_ACCUM_ALPHA_BITS_ARB:
                pValues[i] = 0.f;
                break;
            case WGL_DEPTH_BITS_ARB:
                pValues[i] = 32.f;
                break;
            case WGL_STENCIL_BITS_ARB:
                pValues[i] = 8.f;
                break;
            case WGL_AUX_BUFFERS_ARB:
                pValues[i] = 0.f;
                break;
            case WGL_SAMPLE_BUFFERS_EXT:
                /** @todo this is disabled due to VSG Open Inventor interop issues
                 * it does not make any sense actually since reporting this
                 * as well as choosing a pixel format with this cap would not do anything
                 * since ICD stuff has its own pixelformat state var */
                pValues[i] = 0.f;
                break;
            case WGL_SAMPLES_EXT:
                /** @todo this is disabled due to VSG Open Inventor interop issues
                 * it does not make any sense actually since reporting this
                 * as well as choosing a pixel format with this cap would not do anything
                 * since ICD stuff has its own pixelformat state var */
                pValues[i] = 0.f;
                break;
            case 0x202d: /* <- WGL_DRAW_TO_PBUFFER_ARB this is to make VSG Open Inventor happy */
                pValues[i] = 0.f;
                break;
            default:
                crWarning("wglGetPixelFormatAttribivARB: bad attrib=0x%x", piAttributes[i]);
                return 0;
        }
    }

    return 1;
}

DECLEXPORT(BOOL) WINAPI wglSwapIntervalEXT_prox(int interval)
{
    CR_DDI_PROLOGUE();
    return TRUE;
}

DECLEXPORT(int)  WINAPI wglGetSwapIntervalEXT_prox()
{
    CR_DDI_PROLOGUE();
    return 1;
}

static GLubyte *gsz_wgl_extensions = "WGL_EXT_pixel_format WGL_ARB_pixel_format WGL_ARB_multisample";

DECLEXPORT(const GLubyte *) WINAPI wglGetExtensionsStringEXT_prox()
{
    CR_DDI_PROLOGUE();
    return gsz_wgl_extensions;
}

DECLEXPORT(const GLubyte *) WINAPI wglGetExtensionsStringARB_prox(HDC hdc)
{
    CR_DDI_PROLOGUE();
    (void) hdc;

    return gsz_wgl_extensions;
}
