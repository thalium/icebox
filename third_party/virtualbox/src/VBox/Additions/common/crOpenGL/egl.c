/* $Id: egl.c $ */

/** @file
 * VBox OpenGL EGL implentation.
 */

/*
 * Copyright (C) 2009-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/cdefs.h>
#include <iprt/types.h>

#include <EGL/egl.h>
#include <GL/glx.h>
#include <X11/Xlib.h>

#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EGL_ASSERT(expr) \
    if (!(expr)) { printf("Assertion failed: %s\n", #expr); exit(1); }

/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/

struct VBEGLTLS
{
    /** The last EGL error. */
    EGLint cErr;
    /** The EGL API currently bound to this thread. */
    EGLenum enmAPI;
    /** The current context. */
    EGLContext hCurrent;
    /** The display bound to the current context. */
    EGLDisplay hCurrentDisplay;
    /** The draw surface bound to the current context. */
    EGLSurface hCurrentDraw;
    /** The read surface bound to the current context. */
    EGLSurface hCurrentRead;
};

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @note IDs returned for surfaces should always be lower than these constants.
 */
/** This is OR-ed with a surface ID to mark it as a window, as GLX needs to
 *  know. */
#define VBEGL_WINDOW_SURFACE  0x20000000
/** This is OR-ed with a surface ID to mark it as a pbuffer, as GLX needs to
 *  know. */
#define VBEGL_PBUFFER_SURFACE 0x40000000
/** This is OR-ed with a surface ID to mark it as a pixmap, as GLX needs to
 *  know. */
#define VBEGL_PIXMAP_SURFACE  0x80000000
#define VBEGL_ANY_SURFACE     (VBEGL_WINDOW_SURFACE | VBEGL_PBUFFER_SURFACE | VBEGL_PIXMAP_SURFACE)

/*******************************************************************************
*   Global variables                                                           *
*******************************************************************************/

static pthread_key_t  g_tls;
static pthread_once_t g_tlsOnce = PTHREAD_ONCE_INIT;
static Display       *g_pDefaultDisplay = NULL;
static pthread_once_t g_defaultDisplayOnce = PTHREAD_ONCE_INIT;

static void tlsInitOnce(void)
{
    pthread_key_create(&g_tls, NULL);
}

static struct VBEGLTLS *getTls(void)
{
    struct VBEGLTLS *pTls;

    pthread_once(&g_tlsOnce, tlsInitOnce);
    pTls = (struct VBEGLTLS *)pthread_getspecific(g_tls);
    if (RT_LIKELY(pTls))
        return pTls;
    pTls = (struct VBEGLTLS *)malloc(sizeof(*pTls));
    if (!pTls)
        return NULL;
    pTls->cErr = EGL_SUCCESS;
    pTls->enmAPI = EGL_NONE;
    pTls->hCurrent = EGL_NO_CONTEXT;
    pTls->hCurrentDisplay = EGL_NO_DISPLAY;
    pTls->hCurrentDraw = EGL_NO_SURFACE;
    pTls->hCurrentRead = EGL_NO_SURFACE;
    if (pthread_setspecific(g_tls, pTls) == 0)
        return pTls;
    free(pTls);
    return NULL;
}

static void defaultDisplayInitOnce(void)
{
    g_pDefaultDisplay = XOpenDisplay(NULL);
}

static EGLBoolean clearEGLError(void)
{
    struct VBEGLTLS *pTls = getTls();

    if (!VALID_PTR(pTls))
        return EGL_FALSE;
    pTls->cErr = EGL_SUCCESS;
    return EGL_TRUE;
}

static EGLBoolean setEGLError(EGLint cErr)
{
    struct VBEGLTLS *pTls = getTls();

    if (pTls)
        pTls->cErr = cErr;
    return EGL_FALSE;
}

static EGLBoolean testValidDisplay(EGLNativeDisplayType hDisplay)
{
    void *pSymbol = dlsym(NULL, "gbm_create_device");

    if (hDisplay == EGL_DEFAULT_DISPLAY)
        return EGL_TRUE;
    if ((void *)hDisplay == NULL)
        return EGL_FALSE;
    /* This is the test that Mesa uses to see if this is a GBM "display".  Not
     * very pretty, but since no one can afford to break Mesa it should be
     * safe.  We need this to detect when the X server tries to load us. */
    if (pSymbol != NULL && *(void **)hDisplay == pSymbol)
        return EGL_FALSE;
    return EGL_TRUE;
}

DECLEXPORT(EGLDisplay) eglGetDisplay(EGLNativeDisplayType hDisplay)
{
    Display *pDisplay;

    if (!testValidDisplay(hDisplay))
        return EGL_NO_DISPLAY;
    if (!clearEGLError())  /* Set up our tls. */
        return EGL_NO_DISPLAY;
    if (hDisplay != EGL_DEFAULT_DISPLAY)
        pDisplay = hDisplay;
    else
    {
        pthread_once(&g_defaultDisplayOnce, defaultDisplayInitOnce);
        pDisplay = g_pDefaultDisplay;
    }
    if (pDisplay && !strcmp(glXGetClientString(pDisplay, GLX_VENDOR), "Chromium"))
        return (EGLDisplay) pDisplay;
    return EGL_NO_DISPLAY;
}

DECLEXPORT(EGLint) eglGetError(void)
{
    struct VBEGLTLS *pTls = getTls();

    if (pTls)
        return pTls->cErr;
    return EGL_NOT_INITIALIZED;
}

DECLEXPORT(EGLBoolean) eglInitialize (EGLDisplay hDisplay, EGLint *pcMajor, EGLint *pcMinor)
{
    if (hDisplay == EGL_NO_DISPLAY)
        return EGL_FALSE;
    if (!VALID_PTR(hDisplay))
        return setEGLError(EGL_BAD_DISPLAY);
    if (pcMajor)
        *pcMajor = 1;
    if (pcMinor)
        *pcMinor = 4;
    return clearEGLError();
}

/** @todo This function should terminate all allocated resources. */
DECLEXPORT(EGLBoolean) eglTerminate(EGLDisplay hDisplay)
{
    if (!VALID_PTR(hDisplay))
        return EGL_FALSE;
    return EGL_TRUE;
}

DECLEXPORT(const char *) eglQueryString(EGLDisplay hDisplay, EGLint name)
{
    RT_NOREF(hDisplay);
    switch (name)
    {
        case EGL_CLIENT_APIS:
            return "OpenGL";
        case EGL_VENDOR:
            return "Chromium";
        case EGL_VERSION:
            return "1.4 Chromium";
        case EGL_EXTENSIONS:
            return "";
        default:
            return NULL;
    }
}

DECLEXPORT(EGLBoolean) eglGetConfigs (EGLDisplay hDisplay, EGLConfig *paConfigs, EGLint caConfigs, EGLint *pcaConfigs)
{
    Display *pDisplay = (Display *)hDisplay;
    GLXFBConfig *paFBConfigs;
    int caFBConfigs, i;

    if (!VALID_PTR(pDisplay))
        return setEGLError(EGL_NOT_INITIALIZED);
    if (!VALID_PTR(pcaConfigs))
        return setEGLError(EGL_BAD_PARAMETER);
    if (caConfigs > 0 && !VALID_PTR(paConfigs))
        return setEGLError(EGL_BAD_PARAMETER);
    paFBConfigs = glXGetFBConfigs(pDisplay, DefaultScreen(pDisplay), &caFBConfigs);
    if (!VALID_PTR(paFBConfigs))
        return setEGLError(EGL_BAD_PARAMETER);
    if (caFBConfigs > caConfigs)
        caFBConfigs = caConfigs;
    *pcaConfigs = caFBConfigs;
    for (i = 0; i < caFBConfigs; ++i)
        paConfigs[i] = (EGLConfig)paFBConfigs[i];
    XFree(paFBConfigs);
    return clearEGLError();
}

static int convertEGLAttribToGLX(EGLint a_EGLAttrib)
{
    switch (a_EGLAttrib)
    {
        case EGL_BUFFER_SIZE:
            return GLX_BUFFER_SIZE;
        case EGL_RED_SIZE:
            return GLX_RED_SIZE;
        case EGL_GREEN_SIZE:
            return GLX_GREEN_SIZE;
        case EGL_BLUE_SIZE:
            return GLX_BLUE_SIZE;
        case EGL_LUMINANCE_SIZE:
            return GLX_RED_SIZE;
        case EGL_ALPHA_SIZE:
            return GLX_ALPHA_SIZE;
        /* case EGL_ALPHA_MASK_SIZE: */
        /* case EGL_BIND_TO_TEXTURE_RGB: */
        /* case EGL_BIND_TO_TEXTURE_RGBA: */
        /* case EGL_COLOR_BUFFER_TYPE: */
        /* case EGL_CONFIG_CAVEAT: */
        case EGL_CONFIG_ID:
            return GLX_FBCONFIG_ID;
        /* case EGL_CONFORMANT: */
        case EGL_DEPTH_SIZE:
            return GLX_DEPTH_SIZE;
        case EGL_LEVEL:
            return GLX_LEVEL;
        case EGL_MAX_PBUFFER_WIDTH:
            return GLX_MAX_PBUFFER_WIDTH;
        case EGL_MAX_PBUFFER_HEIGHT:
            return GLX_MAX_PBUFFER_HEIGHT;
        case EGL_MAX_PBUFFER_PIXELS:
            return GLX_MAX_PBUFFER_PIXELS;
        /* case EGL_MATCH_NATIVE_PIXMAP: */
        /* case EGL_MAX_SWAP_INTERVAL: */
        /* case EGL_MIN_SWAP_INTERVAL: */
        case EGL_NATIVE_RENDERABLE:
            return GLX_X_RENDERABLE;
        case EGL_NATIVE_VISUAL_ID:
            return GLX_VISUAL_ID;
        /* case EGL_NATIVE_VISUAL_TYPE: */
        /* case EGL_RENDERABLE_TYPE: */
        case EGL_SAMPLE_BUFFERS:
            return GLX_SAMPLE_BUFFERS;
        case EGL_SAMPLES:
            return GLX_SAMPLES;
        case EGL_STENCIL_SIZE:
            return GLX_STENCIL_SIZE;
        /* case EGL_SURFACE_TYPE: */
        /* case EGL_TRANSPARENT_TYPE: */
        case EGL_TRANSPARENT_RED_VALUE:
            return GLX_TRANSPARENT_RED_VALUE;
        case EGL_TRANSPARENT_GREEN_VALUE:
            return GLX_TRANSPARENT_GREEN_VALUE;
        case EGL_TRANSPARENT_BLUE_VALUE:
            return GLX_TRANSPARENT_BLUE_VALUE;
        default:
            return None;
    }
}

DECLEXPORT(EGLBoolean) eglChooseConfig (EGLDisplay hDisplay, const EGLint *paAttribs, EGLConfig *paConfigs, EGLint caConfigs,
                                        EGLint *pcConfigs)
{
    Display *pDisplay = (Display *)hDisplay;
    int aAttribList[256];  /* The list cannot be this long. */
    unsigned cAttribs = 0, i;
    const EGLint *pAttrib, *pAttrib2;
    EGLint cRenderableType = EGL_OPENGL_ES_BIT;
    unsigned cConfigCaveat = GLX_DONT_CARE, cConformant = GLX_DONT_CARE;
    GLXFBConfig *paFBConfigs;
    int caFBConfigs;

    if (!VALID_PTR(hDisplay))
        return setEGLError(EGL_NOT_INITIALIZED);
    if (!VALID_PTR(pcConfigs))
        return setEGLError(EGL_BAD_PARAMETER);
    if (caConfigs > 0 && !VALID_PTR(paConfigs))
        return setEGLError(EGL_BAD_PARAMETER);
    for (pAttrib = paAttribs; pAttrib != NULL && *pAttrib != EGL_NONE; pAttrib += 2)
    {
        bool fSkip = false;
        int cGLXAttrib;

        /* Check for illegal values. */
        if ((*pAttrib == EGL_LEVEL || *pAttrib == EGL_MATCH_NATIVE_PIXMAP) && pAttrib[1] == EGL_DONT_CARE)
            return setEGLError(EGL_BAD_ATTRIBUTE);
        /* Check for values we can't handle. */
        if (   (*pAttrib == EGL_ALPHA_MASK_SIZE)
            && pAttrib[1] != EGL_DONT_CARE && pAttrib[1] != 0)
            return setEGLError(EGL_BAD_ACCESS);
        /** @todo try creating a pixmap from a native one with the configurations returned. */
        if (*pAttrib == EGL_MATCH_NATIVE_PIXMAP)
            return setEGLError(EGL_BAD_ACCESS);
        if (   (   *pAttrib == EGL_MIN_SWAP_INTERVAL || *pAttrib == EGL_MAX_SWAP_INTERVAL
                || *pAttrib == EGL_BIND_TO_TEXTURE_RGB || *pAttrib == EGL_BIND_TO_TEXTURE_RGBA)
            && pAttrib[1] != EGL_DONT_CARE)
            return setEGLError(EGL_BAD_ACCESS);
        /* Ignore attributes which are repeated later. */
        for (pAttrib2 = pAttrib + 2; *pAttrib2 != EGL_NONE; pAttrib2 += 2)
            if (*pAttrib2 == *pAttrib)
                fSkip == true;
        if (fSkip)
            continue;
        cGLXAttrib = convertEGLAttribToGLX(*pAttrib);
        if (cGLXAttrib != None)
        {
            aAttribList[cAttribs] = cGLXAttrib;
            if (pAttrib[1] == EGL_DONT_CARE)
                aAttribList[cAttribs + 1] = GLX_DONT_CARE;
            else
                aAttribList[cAttribs + 1] = pAttrib[1];
            cAttribs += 2;
        }
        else
        {
            switch (*pAttrib)
            {
                case EGL_COLOR_BUFFER_TYPE:
                    aAttribList[cAttribs] = GLX_X_VISUAL_TYPE;
                    aAttribList[cAttribs + 1] =   pAttrib[1] == EGL_DONT_CARE ? GLX_DONT_CARE
                                                : pAttrib[1] == EGL_RGB_BUFFER ? GLX_TRUE_COLOR
                                                : pAttrib[1] == EGL_LUMINANCE_BUFFER ? GLX_GRAY_SCALE
                                                : GL_FALSE;
                    if (   *pAttrib == EGL_COLOR_BUFFER_TYPE
                        && pAttrib[1] != EGL_DONT_CARE && pAttrib[1] != EGL_RGB_BUFFER)
                        return setEGLError(EGL_BAD_ACCESS);
                    break;
                case EGL_CONFIG_CAVEAT:
                    cConfigCaveat =   pAttrib[1] == EGL_DONT_CARE ? GLX_DONT_CARE
                                    : pAttrib[1] == EGL_NONE ? GLX_NONE
                                    : pAttrib[1] == EGL_SLOW_CONFIG ? GLX_SLOW_CONFIG
                                    : pAttrib[1] == EGL_NON_CONFORMANT_CONFIG ? GLX_NON_CONFORMANT_CONFIG
                                    : GL_FALSE;
                    if (!cConfigCaveat)
                        return setEGLError(EGL_BAD_ATTRIBUTE);
                    cAttribs -= 2;
                    break;
                case EGL_CONFORMANT:
                    if (pAttrib[1] != EGL_OPENGL_BIT && pAttrib[1] != 0)
                        return setEGLError(EGL_BAD_ACCESS);
                    cConformant =   pAttrib[1] == EGL_OPENGL_BIT ? GL_TRUE : GL_FALSE;
                    cAttribs -= 2;
                    break;
                case EGL_NATIVE_VISUAL_TYPE:
                    aAttribList[cAttribs] = GLX_X_VISUAL_TYPE;
                    aAttribList[cAttribs + 1] =   pAttrib[1] == EGL_DONT_CARE ? GLX_DONT_CARE
                                                : pAttrib[1] == StaticGray ? GLX_STATIC_GRAY
                                                : pAttrib[1] == StaticColor ? GLX_STATIC_COLOR
                                                : pAttrib[1] == TrueColor ? GLX_TRUE_COLOR
                                                : pAttrib[1] == GrayScale ? GLX_GRAY_SCALE
                                                : pAttrib[1] == PseudoColor ? GLX_PSEUDO_COLOR
                                                : pAttrib[1] == DirectColor ? GLX_DIRECT_COLOR
                                                : GL_FALSE;
                    break;
                case EGL_RENDERABLE_TYPE:
                    cRenderableType = pAttrib[1];
                    cAttribs -= 2;  /* We did not add anything to the list. */
                    break;
                case EGL_SURFACE_TYPE:
                    if (pAttrib[1] & ~(EGL_PBUFFER_BIT | EGL_PIXMAP_BIT | EGL_WINDOW_BIT))
                        return setEGLError(EGL_BAD_ACCESS);
                    aAttribList[cAttribs] = GLX_DRAWABLE_TYPE;
                    aAttribList[cAttribs + 1] =   (pAttrib[1] & EGL_PBUFFER_BIT ? GLX_PBUFFER_BIT : 0)
                                                | (pAttrib[1] & EGL_PIXMAP_BIT ? GLX_PIXMAP_BIT : 0)
                                                | (pAttrib[1] & EGL_WINDOW_BIT ? GLX_WINDOW_BIT : 0);
                    break;
                case EGL_TRANSPARENT_TYPE:
                    aAttribList[cAttribs] = GLX_TRANSPARENT_TYPE;
                    aAttribList[cAttribs + 1] =   pAttrib[1] == EGL_DONT_CARE ? GLX_DONT_CARE
                                                : pAttrib[1] == EGL_NONE ? GLX_NONE
                                                : pAttrib[1] == EGL_TRANSPARENT_RGB ? GLX_TRANSPARENT_RGB
                                                : GL_FALSE;
                    break;
                default:
                    return setEGLError(EGL_BAD_ATTRIBUTE);
            }
            cAttribs += 2;
        }
    }
    if (cConfigCaveat != GLX_DONT_CARE || cConformant != GLX_DONT_CARE)
    {
        aAttribList[cAttribs] = GLX_CONFIG_CAVEAT;
        aAttribList[cAttribs + 1] =   cConformant == GL_FALSE ? GLX_NON_CONFORMANT_CONFIG
                                    : cConfigCaveat == EGL_SLOW_CONFIG ? GLX_SLOW_CONFIG
                                    : GLX_NONE;
        cAttribs += 2;
    }
    aAttribList[cAttribs] = GLX_RENDER_TYPE;
    aAttribList[cAttribs + 1] = GLX_RGBA_BIT;
    cAttribs += 2;
    if (paAttribs != NULL)
    {
        aAttribList[cAttribs] = None;
        EGL_ASSERT(cAttribs < RT_ELEMENTS(aAttribList));
        if (!(cRenderableType & EGL_OPENGL_BIT))
            return setEGLError(EGL_BAD_ACCESS);
    }
    paFBConfigs = glXChooseFBConfig(pDisplay, DefaultScreen(pDisplay), paAttribs != NULL ? aAttribList : NULL, &caFBConfigs);
    if (paFBConfigs == NULL)
        return setEGLError(EGL_BAD_ACCESS);
    *pcConfigs = caFBConfigs;
    for (i = 0; (GLint)i < caConfigs && (GLint)i < caFBConfigs; ++i)
        paConfigs[i] = (EGLConfig)paFBConfigs[i];
    XFree(paFBConfigs);
    return clearEGLError();
}

DECLEXPORT(EGLBoolean) eglGetConfigAttrib (EGLDisplay hDisplay, EGLConfig cConfig, EGLint cAttribute, EGLint *pValue)
{
    Display *pDisplay = (Display *)hDisplay;
    int cGLXAttribute = convertEGLAttribToGLX(cAttribute);
    int cValue;

    if (!VALID_PTR(hDisplay))
        return setEGLError(EGL_NOT_INITIALIZED);
    if (!VALID_PTR(pValue))
        return setEGLError(EGL_BAD_PARAMETER);
    if (glXGetFBConfigAttrib(pDisplay, (GLXFBConfig)cConfig, GLX_FBCONFIG_ID, &cValue))
        return setEGLError(EGL_BAD_CONFIG);
    if (cGLXAttribute != None)
    {
        if (glXGetFBConfigAttrib(pDisplay, (GLXFBConfig)cConfig, cGLXAttribute, &cValue))
            return setEGLError(EGL_BAD_ACCESS);
        *pValue = cValue;
        return clearEGLError();
    }
    switch (cAttribute)
    {
        case EGL_ALPHA_MASK_SIZE:
            *pValue = 0;
            return clearEGLError();
        case EGL_LUMINANCE_SIZE:
            if (glXGetFBConfigAttrib(pDisplay, (GLXFBConfig)cConfig, GLX_X_VISUAL_TYPE, &cValue))
                return setEGLError(EGL_BAD_ACCESS);
            if (cValue == GLX_STATIC_GRAY || cValue == GLX_GRAY_SCALE)
            {
                if (glXGetFBConfigAttrib(pDisplay, (GLXFBConfig)cConfig, GLX_RED_SIZE, &cValue))
                    return setEGLError(EGL_BAD_ACCESS);
                *pValue = cValue;
            }
            else
                *pValue = 0;
            return clearEGLError();
        case EGL_COLOR_BUFFER_TYPE:
            if (glXGetFBConfigAttrib(pDisplay, (GLXFBConfig)cConfig, GLX_X_VISUAL_TYPE, &cValue))
                return setEGLError(EGL_BAD_ACCESS);
            if (cValue == GLX_STATIC_GRAY || cValue == GLX_GRAY_SCALE)
                *pValue = EGL_LUMINANCE_BUFFER;
            else
                *pValue = EGL_RGB_BUFFER;
            return clearEGLError();
        case EGL_CONFIG_CAVEAT:
            if (glXGetFBConfigAttrib(pDisplay, (GLXFBConfig)cConfig, GLX_CONFIG_CAVEAT, &cValue))
                return setEGLError(EGL_BAD_ACCESS);
            *pValue = cValue == GLX_NONE ? EGL_NONE : cValue == GLX_SLOW_CONFIG ? EGL_SLOW_CONFIG : GLX_NON_CONFORMANT_CONFIG;
            return clearEGLError();
        case EGL_CONFORMANT:
            if (glXGetFBConfigAttrib(pDisplay, (GLXFBConfig)cConfig, GLX_CONFIG_CAVEAT, &cValue))
                return setEGLError(EGL_BAD_ACCESS);
            *pValue = cValue == GLX_NON_CONFORMANT_CONFIG ? 0 : EGL_OPENGL_BIT;
            return clearEGLError();
        case EGL_MATCH_NATIVE_PIXMAP:
        case EGL_MIN_SWAP_INTERVAL:
        case EGL_MAX_SWAP_INTERVAL:
            return setEGLError(EGL_BAD_ACCESS);
        case EGL_NATIVE_VISUAL_TYPE:
            if (glXGetFBConfigAttrib(pDisplay, (GLXFBConfig)cConfig, GLX_X_VISUAL_TYPE, &cValue))
                return setEGLError(EGL_BAD_ACCESS);
            *pValue =   cValue == GLX_STATIC_GRAY ? StaticGray
                      : cValue == GLX_STATIC_COLOR ? StaticColor
                      : cValue == GLX_TRUE_COLOR ? TrueColor
                      : cValue == GLX_GRAY_SCALE ? GrayScale
                      : cValue == GLX_PSEUDO_COLOR ? PseudoColor
                      : cValue == GLX_DIRECT_COLOR ? DirectColor
                      : -1;
            return clearEGLError();
        case EGL_RENDERABLE_TYPE:
            *pValue = EGL_OPENGL_BIT;
            return clearEGLError();
        case EGL_SURFACE_TYPE:
            if (glXGetFBConfigAttrib(pDisplay, (GLXFBConfig)cConfig, GLX_DRAWABLE_TYPE, &cValue))
                return setEGLError(EGL_BAD_ACCESS);
            *pValue =   (cValue & GLX_PBUFFER_BIT ? EGL_PBUFFER_BIT : 0)
                      | (cValue & GLX_PIXMAP_BIT ? EGL_PIXMAP_BIT : 0)
                      | (cValue & GLX_WINDOW_BIT ? EGL_WINDOW_BIT : 0);
            return clearEGLError();
        case EGL_TRANSPARENT_TYPE:
            if (glXGetFBConfigAttrib(pDisplay, (GLXFBConfig)cConfig, GLX_TRANSPARENT_TYPE, &cValue))
                return setEGLError(EGL_BAD_ACCESS);
            *pValue =  cValue == GLX_NONE ? EGL_NONE
                     : cValue == GLX_TRANSPARENT_RGB ? EGL_TRANSPARENT_RGB
                     : EGL_FALSE;
            return *pValue != EGL_FALSE ? clearEGLError() : setEGLError(EGL_BAD_ACCESS);
        default:
            return setEGLError(EGL_BAD_ATTRIBUTE);
    }
    return clearEGLError();
}

DECLEXPORT(EGLSurface) eglCreateWindowSurface(EGLDisplay hDisplay, EGLConfig config, EGLNativeWindowType hWindow,
                                              const EGLint *paAttributes)
{
    Display *pDisplay = (Display *)hDisplay;
    GLXWindow hGLXWindow;

    if (!VALID_PTR(hDisplay))
    {
        setEGLError(EGL_NOT_INITIALIZED);
        return EGL_NO_SURFACE;
    }
    if (paAttributes != NULL)  /* Sanity test only. */
        while (*paAttributes != EGL_NONE)
        {
            if (*paAttributes != EGL_RENDER_BUFFER)
            {
                setEGLError(EGL_BAD_MATCH);
                return EGL_NO_SURFACE;
            }
            paAttributes += 2;
        }
    hGLXWindow = glXCreateWindow(pDisplay, (GLXFBConfig)config, (Window)hWindow, NULL);
    if (hGLXWindow == None)
    {
        setEGLError(EGL_BAD_ALLOC);
        return EGL_NO_SURFACE;
    }
    EGL_ASSERT(hGLXWindow < VBEGL_WINDOW_SURFACE);  /* Greater than the maximum XID. */
    clearEGLError();
    return (EGLSurface)(hGLXWindow | VBEGL_WINDOW_SURFACE);
}

static void setAttribute(int *pcStoreIndex, int *pcCurIndex, int *paGLXAttributes, int cAttribute, int cValue)
{
    if (*pcStoreIndex < 0)
    {
        *pcStoreIndex = *pcCurIndex;
        *pcCurIndex += 2;
        paGLXAttributes[*pcStoreIndex] = cAttribute;
    }
    paGLXAttributes[*pcStoreIndex + 1] = cValue;
}

DECLEXPORT(EGLSurface) eglCreatePbufferSurface(EGLDisplay hDisplay, EGLConfig config, EGLint const *paAttributes)
{
    Display *pDisplay = (Display *)hDisplay;
    enum { CPS_WIDTH = 0, CPS_HEIGHT, CPS_LARGEST, CPS_PRESERVED, CPS_TEX_FORMAT, CPS_TEX_TARGET, CPS_MIPMAP_TEX, CPS_END };
    int acIndices[CPS_END];
    int aAttributes[CPS_END * 2];
    int cIndex = 0;
    unsigned i;
    GLXPbuffer hPbuffer;

    if (!VALID_PTR(hDisplay))
    {
        setEGLError(EGL_NOT_INITIALIZED);
        return EGL_NO_SURFACE;
    }
    for (i = 0; i < RT_ELEMENTS(acIndices); ++i)
        acIndices[i] = -1;
    if (paAttributes != NULL)
        while (*paAttributes != EGL_NONE)
        {
            switch (*paAttributes)
            {
                case EGL_WIDTH:
                    setAttribute(&acIndices[CPS_WIDTH], &cIndex, aAttributes, GLX_PBUFFER_WIDTH, paAttributes[1]);
                    break;
                case EGL_HEIGHT:
                    setAttribute(&acIndices[CPS_HEIGHT], &cIndex, aAttributes, GLX_LARGEST_PBUFFER, paAttributes[1]);
                    break;
                case EGL_LARGEST_PBUFFER:
                    setAttribute(&acIndices[CPS_LARGEST], &cIndex, aAttributes, GLX_PBUFFER_HEIGHT, paAttributes[1]);
                    break;
                case EGL_BUFFER_PRESERVED:
                    setAttribute(&acIndices[CPS_PRESERVED], &cIndex, aAttributes, GLX_PRESERVED_CONTENTS, paAttributes[1]);
                    break;
                case EGL_TEXTURE_FORMAT:
                    setAttribute(&acIndices[CPS_TEX_FORMAT], &cIndex, aAttributes, GLX_TEXTURE_FORMAT_EXT, paAttributes[1]);
                    break;
                case EGL_TEXTURE_TARGET:
                    setAttribute(&acIndices[CPS_TEX_TARGET], &cIndex, aAttributes, GLX_TEXTURE_TARGET_EXT, paAttributes[1]);
                    break;
                case EGL_MIPMAP_TEXTURE:
                    setAttribute(&acIndices[CPS_MIPMAP_TEX], &cIndex, aAttributes, GLX_MIPMAP_TEXTURE_EXT, paAttributes[1]);
                    break;
                case EGL_VG_ALPHA_FORMAT:
                case EGL_VG_COLORSPACE:
                {
                    setEGLError(EGL_BAD_MATCH);
                    return EGL_NO_SURFACE;
                }
            }
            paAttributes += 2;
        }
    EGL_ASSERT((unsigned)cIndex < RT_ELEMENTS(aAttributes) - 1U);
    aAttributes[cIndex + 1] = None;
    hPbuffer = glXCreatePbuffer(pDisplay, (GLXFBConfig)config, aAttributes);
    if (hPbuffer == None)
    {
        setEGLError(EGL_BAD_ALLOC);
        return EGL_NO_SURFACE;
    }
    EGL_ASSERT(hPbuffer < VBEGL_WINDOW_SURFACE);  /* Greater than the maximum XID. */
    clearEGLError();
    return (EGLSurface)(hPbuffer | VBEGL_PBUFFER_SURFACE);
}

DECLEXPORT(EGLSurface) eglCreatePixmapSurface(EGLDisplay hDisplay, EGLConfig config, EGLNativePixmapType hPixmap,
                                              const EGLint *paAttributes)
{
    Display *pDisplay = (Display *)hDisplay;
    GLXPixmap hGLXPixmap;

    if (!VALID_PTR(hDisplay))
    {
        setEGLError(EGL_NOT_INITIALIZED);
        return EGL_NO_SURFACE;
    }
    if (paAttributes != NULL)  /* Sanity test only. */
        if (*paAttributes != EGL_NONE)
        {
            if (*paAttributes == EGL_VG_COLORSPACE || *paAttributes == EGL_VG_ALPHA_FORMAT)
            {
                setEGLError(EGL_BAD_MATCH);
                return EGL_NO_SURFACE;
            }
            else
            {
                setEGLError(EGL_BAD_ATTRIBUTE);
                return EGL_NO_SURFACE;
            }
        }
    hGLXPixmap = glXCreatePixmap(pDisplay, (GLXFBConfig)config, (Pixmap)hPixmap, NULL);
    if (hGLXPixmap == None)
    {
        setEGLError(EGL_BAD_MATCH);
        return EGL_NO_SURFACE;
    }
    EGL_ASSERT(hGLXPixmap < VBEGL_WINDOW_SURFACE);  /* Greater than the maximum XID. */
    clearEGLError();
    return (EGLSurface)(hGLXPixmap | VBEGL_PIXMAP_SURFACE);
}

DECLEXPORT(EGLBoolean) eglDestroySurface(EGLDisplay hDisplay, EGLSurface hSurface)
{
    Display *pDisplay = (Display *)hDisplay;

    if (!VALID_PTR(hDisplay))
        return setEGLError(EGL_NOT_INITIALIZED);
    switch ((GLXDrawable)hSurface & VBEGL_ANY_SURFACE)
    {
        case VBEGL_WINDOW_SURFACE:
            glXDestroyWindow(pDisplay, (GLXWindow)hSurface & ~VBEGL_WINDOW_SURFACE);
            return clearEGLError();
        case VBEGL_PBUFFER_SURFACE:
            glXDestroyPbuffer(pDisplay, (GLXPbuffer)hSurface & ~VBEGL_PBUFFER_SURFACE);
            return clearEGLError();
        case VBEGL_PIXMAP_SURFACE:
            glXDestroyPixmap(pDisplay, (GLXPixmap)hSurface & ~VBEGL_PIXMAP_SURFACE);
            return clearEGLError();
        default:
            return setEGLError(EGL_BAD_SURFACE);
    }
}

DECLEXPORT(EGLBoolean) eglSurfaceAttrib(EGLDisplay hDisplay, EGLSurface hSurface, EGLint cAttribute, EGLint cValue)
{
    NOREF(hDisplay);
    NOREF(hSurface);
    NOREF(cValue);
    switch (cAttribute)
    {
        case EGL_MIPMAP_LEVEL:
        case EGL_MULTISAMPLE_RESOLVE:
        case EGL_SWAP_BEHAVIOR:
            return setEGLError(EGL_BAD_MATCH);
        default:
            return setEGLError(EGL_BAD_ATTRIBUTE);
    }
}

DECLEXPORT(EGLBoolean) eglQuerySurface(EGLDisplay hDisplay, EGLSurface hSurface, EGLint cAttribute, EGLint *cValue)
{
    NOREF(hDisplay);
    NOREF(hSurface);
    NOREF(cAttribute);
    NOREF(cValue);
    return setEGLError(EGL_BAD_MATCH);
}

DECLEXPORT(EGLBoolean) eglBindTexImage(EGLDisplay hDisplay, EGLSurface hSurface, EGLint cBuffer)
{
    NOREF(hDisplay);
    NOREF(hSurface);
    NOREF(cBuffer);
    return setEGLError(EGL_BAD_MATCH);
}

DECLEXPORT(EGLBoolean) eglReleaseTexImage(EGLDisplay hDisplay, EGLSurface hSurface, EGLint cBuffer)
{
    NOREF(hDisplay);
    NOREF(hSurface);
    NOREF(cBuffer);
    return setEGLError(EGL_BAD_MATCH);
}

DECLEXPORT(EGLBoolean) eglBindAPI(EGLenum enmApi)
{
    return enmApi == EGL_OPENGL_API ? clearEGLError() : setEGLError(EGL_BAD_PARAMETER);
}

DECLEXPORT(EGLenum) eglQueryAPI(void)
{
    return EGL_OPENGL_API;
}

DECLEXPORT(EGLContext) eglCreateContext(EGLDisplay hDisplay, EGLConfig hConfig, EGLContext hSharedContext,
                                        const EGLint *paAttribs)
{
    Display *pDisplay = (Display *)hDisplay;
    GLXContext hNewContext;

    if (!VALID_PTR(hDisplay))
    {
        setEGLError(EGL_NOT_INITIALIZED);
        return EGL_NO_CONTEXT;
    }
    if (paAttribs != NULL && *paAttribs != EGL_NONE)
    {
        setEGLError(EGL_BAD_ATTRIBUTE);
        return EGL_NO_CONTEXT;
    }
    hNewContext = glXCreateNewContext(pDisplay, (GLXFBConfig)hConfig, GLX_RGBA_TYPE, (GLXContext)hSharedContext, true);
    if (hNewContext)
    {
        clearEGLError();
        return (EGLContext)hNewContext;
    }
    setEGLError(EGL_BAD_MATCH);
    return EGL_NO_CONTEXT;
}

DECLEXPORT(EGLBoolean) eglDestroyContext(EGLDisplay hDisplay, EGLContext hContext)
{
    Display *pDisplay = (Display *)hDisplay;

    if (!VALID_PTR(hDisplay))
        return setEGLError(EGL_NOT_INITIALIZED);
    glXDestroyContext(pDisplay, (GLXContext) hContext);
    return clearEGLError();
}

DECLEXPORT(EGLBoolean) eglMakeCurrent(EGLDisplay hDisplay, EGLSurface hDraw, EGLSurface hRead, EGLContext hContext)
{
    Display *pDisplay = (Display *)hDisplay;
    GLXDrawable hGLXDraw = hDraw == EGL_NO_SURFACE ? None : (GLXDrawable)hDraw & ~VBEGL_ANY_SURFACE;
    GLXDrawable hGLXRead = hRead == EGL_NO_SURFACE ? None : (GLXDrawable)hRead & ~VBEGL_ANY_SURFACE;
    GLXContext hGLXContext = hContext == EGL_NO_CONTEXT ? None : (GLXContext)hContext;
    struct VBEGLTLS *pTls = getTls();

    if (!VALID_PTR(hDisplay) || !VALID_PTR(pTls))
        return setEGLError(EGL_NOT_INITIALIZED);
    if (glXMakeContextCurrent(pDisplay, hGLXDraw, hGLXRead, hGLXContext))
    {
        pTls->hCurrent = hContext;
        pTls->hCurrentDraw = hDraw;
        pTls->hCurrentRead = hRead;
        return clearEGLError();
    }
    else
        return setEGLError(EGL_BAD_MATCH);
}

DECLEXPORT(EGLContext) eglGetCurrentContext(void)
{
    struct VBEGLTLS *pTls = getTls();

    if (!VALID_PTR(pTls))
        return EGL_NO_CONTEXT;
    clearEGLError();
    return pTls->hCurrent;
}

DECLEXPORT(EGLSurface) eglGetCurrentSurface(EGLint cOp)
{
    struct VBEGLTLS *pTls = getTls();

    if (!VALID_PTR(pTls))
        return EGL_NO_SURFACE;
    clearEGLError();
    switch (cOp)
    {
        case EGL_DRAW:
            return pTls->hCurrentDraw;
        case EGL_READ:
            return pTls->hCurrentRead;
        default:
            setEGLError(EGL_BAD_PARAMETER);
            return EGL_NO_SURFACE;
    }
}

DECLEXPORT(EGLDisplay) eglGetCurrentDisplay(void)
{
    struct VBEGLTLS *pTls;

    pTls = getTls();
    if (!VALID_PTR(pTls))
        return EGL_NO_DISPLAY;
    clearEGLError();
    return pTls->hCurrentDisplay;
}

DECLEXPORT(EGLBoolean) eglQueryContext(EGLDisplay hDisplay, EGLContext hContext, EGLint cAttribute, EGLint *pcValue)
{
    Display *pDisplay = (Display *)hDisplay;

    if (!VALID_PTR(hDisplay))
        return setEGLError(EGL_NOT_INITIALIZED);
    if (!VALID_PTR(pcValue))
        return setEGLError(EGL_BAD_PARAMETER);
    switch (cAttribute)
    {
        case EGL_CONFIG_ID:
        {
            int cValue = 0;

            if (glXQueryContext(pDisplay, (GLXContext)hContext, GLX_FBCONFIG_ID, &cValue) == Success)
            {
                *pcValue = cValue;
                return clearEGLError();
            }
            return setEGLError(EGL_BAD_MATCH);
        }
        case EGL_CONTEXT_CLIENT_TYPE:
            *pcValue = EGL_OPENGL_API;
            return clearEGLError();
        case EGL_CONTEXT_CLIENT_VERSION:
            *pcValue = 0;
            return clearEGLError();
        case EGL_RENDER_BUFFER:
            *pcValue = EGL_BACK_BUFFER;
            return clearEGLError();
        default:
            return setEGLError(EGL_BAD_ATTRIBUTE);
    }
}

DECLEXPORT(EGLBoolean) eglWaitClient(void)
{
    glXWaitGL();
    return clearEGLError();
}

DECLEXPORT(EGLBoolean) eglWaitGL(void)
{
    return setEGLError(EGL_BAD_PARAMETER);  /* OpenGL ES only. */
}

DECLEXPORT(EGLBoolean) eglWaitNative(EGLint cEngine)
{
    if (cEngine != EGL_CORE_NATIVE_ENGINE)
        return setEGLError(EGL_BAD_PARAMETER);
    glXWaitX();
    return clearEGLError();
}

DECLEXPORT(EGLBoolean) eglSwapBuffers(EGLDisplay hDisplay, EGLSurface hSurface)
{
    Display *pDisplay = (Display *)hDisplay;

    if (!VALID_PTR(hDisplay))
        return setEGLError(EGL_NOT_INITIALIZED);
    glXSwapBuffers(pDisplay, (GLXDrawable)hSurface & ~VBEGL_ANY_SURFACE);
    return clearEGLError();
}

/** @todo Work out how this fits over what Chromium has to offer. */
DECLEXPORT(EGLBoolean) eglCopyBuffers(EGLDisplay hDisplay, EGLSurface hSurface, EGLNativePixmapType hPixmap)
{
    Display *pDisplay = (Display *)hDisplay;

    if (!VALID_PTR(pDisplay))
        return setEGLError(EGL_NOT_INITIALIZED);

    NOREF(hSurface);
    NOREF(hPixmap);
    return setEGLError(EGL_BAD_MATCH);
}

DECLEXPORT(EGLBoolean) eglSwapInterval (EGLDisplay dpy, EGLint interval)
{
    NOREF(dpy);
    NOREF(interval);
    return EGL_TRUE;
}

typedef void (*VBEGLFuncPtr)(void);
DECLEXPORT(VBEGLFuncPtr)eglGetProcAddress(const char *pszName)
{
    clearEGLError();
    return glXGetProcAddress((const GLubyte *)pszName);
}

DECLEXPORT(EGLBoolean) eglReleaseThread()
{
    struct VBEGLTLS *pTls = getTls();

    if (!(pTls))
        return EGL_TRUE;
    free(pTls);
    /* Can this fail with ENOMEM? */
    pthread_setspecific(g_tls, NULL);
    return EGL_TRUE;
}
