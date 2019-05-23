/* $Id: glx.c $ */
/** @file
 * VBox OpenGL GLX implementation
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 * --------------------------------------------------------------------
 * Original copyright notice:
 *
 * Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

/* opengl_stub/glx.c */
#include "chromium.h"
#include "cr_error.h"
#include "cr_spu.h"
#include "cr_mem.h"
#include "cr_string.h"
#include "stub.h"
#include "dri_glx.h"
#include "GL/internal/glcore.h"
#include "cr_glstate.h"

#include <X11/Xregion.h>

/* Force full pixmap update if there're more damaged regions than this number*/
#define CR_MAX_DAMAGE_REGIONS_TRACKED 50

/* Force "bigger" update (full or clip) if it's reducing number of regions updated
 * but doesn't increase updated area more than given number
 */
#define CR_MIN_DAMAGE_PROFIT_SIZE 64*64

/** @todo combine it in some other place*/
/* Size of pack spu buffer - some delta for commands packing, see pack/packspu_config.c*/

/** Ramshankar: Solaris compiz fix */
#ifdef RT_OS_SOLARIS
# define CR_MAX_TRANSFER_SIZE 20*1024*1024
#else
# define CR_MAX_TRANSFER_SIZE 4*1024*1024
#endif

/** For optimizing glXMakeCurrent */
static Display *currentDisplay = NULL;
static GLXDrawable currentDrawable = 0;
static GLXDrawable currentReadDrawable = 0;

static void stubXshmUpdateImageRect(Display *dpy, GLXDrawable draw, GLX_Pixmap_t *pGlxPixmap, XRectangle *pRect);
static void stubQueryXDamageExtension(Display *dpy, ContextInfo *pContext);

static bool isGLXVisual(Display *dpy, XVisualInfo *vis)
{
    return vis->visualid == XVisualIDFromVisual(DefaultVisual(dpy, vis->screen));
}

static GLXFBConfig fbConfigFromVisual(Display *dpy, XVisualInfo *vis)
{
    (void)dpy;

    if (!isGLXVisual(dpy, vis))
        return 0;
    return (GLXFBConfig)vis->visualid;
}

static GLXFBConfig defaultFBConfigForScreen(Display *dpy, int screen)
{
    return (GLXFBConfig) XVisualIDFromVisual(DefaultVisual(dpy, screen));
}

static XVisualInfo *visualInfoFromFBConfig(Display *dpy, GLXFBConfig config)
{
    XVisualInfo info, *pret;
    int nret;

    info.visualid = (VisualID)config;
    pret = XGetVisualInfo(dpy, VisualIDMask, &info, &nret);
    if (nret == 1)
        return pret;
    XFree(pret);
    return NULL;
}

DECLEXPORT(XVisualInfo *)
VBOXGLXTAG(glXChooseVisual)( Display *dpy, int screen, int *attribList )
{
    bool useRGBA = false;
    int *attrib;
    XVisualInfo searchvis, *pret;
    int nvisuals;
    stubInit();

    for (attrib = attribList; *attrib != None; attrib++)
    {
        switch (*attrib)
        {
            case GLX_USE_GL:
                /* ignored, this is mandatory */
                break;

            case GLX_BUFFER_SIZE:
                /* this is for color-index visuals, which we don't support */
                attrib++;
                break;

            case GLX_LEVEL:
                if (attrib[1] != 0)
                    goto err_exit;
                attrib++;
                break;

            case GLX_RGBA:
                useRGBA = true;
                break;

            case GLX_STEREO:
                goto err_exit;
                /*
                crWarning( "glXChooseVisual: stereo unsupported" );
                return NULL;
                */
                break;

            case GLX_AUX_BUFFERS:
                if (attrib[1] != 0)
                    goto err_exit;
                attrib++;
                break;

            case GLX_RED_SIZE:
            case GLX_GREEN_SIZE:
            case GLX_BLUE_SIZE:
                if (attrib[1] > 8)
                    goto err_exit;
                attrib++;
                break;

            case GLX_ALPHA_SIZE:
                if (attrib[1] > 8)
                    goto err_exit;
                attrib++;
                break;

            case GLX_DEPTH_SIZE:
                if (attrib[1] > 24)
                    goto err_exit;
                attrib++;
                break;

            case GLX_STENCIL_SIZE:
                if (attrib[1] > 8)
                    goto err_exit;
                attrib++;
                break;

            case GLX_ACCUM_RED_SIZE:
            case GLX_ACCUM_GREEN_SIZE:
            case GLX_ACCUM_BLUE_SIZE:
            case GLX_ACCUM_ALPHA_SIZE:
                if (attrib[1] > 16)
                    goto err_exit;
                attrib++;
                break;

            case GLX_SAMPLE_BUFFERS_SGIS: /* aka GLX_SAMPLES_ARB */
                if (attrib[1] > 0)
                    goto err_exit;
                attrib++;
                break;
            case GLX_SAMPLES_SGIS: /* aka GLX_SAMPLES_ARB */
                if (attrib[1] > 0)
                    goto err_exit;
                attrib++;
                break;

            case GLX_DOUBLEBUFFER: /* @todo, check if we support it */
                break;

#ifdef GLX_VERSION_1_3
            case GLX_X_VISUAL_TYPE:
            case GLX_TRANSPARENT_TYPE_EXT:
            case GLX_TRANSPARENT_INDEX_VALUE_EXT:
            case GLX_TRANSPARENT_RED_VALUE_EXT:
            case GLX_TRANSPARENT_GREEN_VALUE_EXT:
            case GLX_TRANSPARENT_BLUE_VALUE_EXT:
            case GLX_TRANSPARENT_ALPHA_VALUE_EXT:
                /* ignore */
                crWarning("glXChooseVisual: ignoring attribute 0x%x", *attrib);
                attrib++;
                break;
#endif

            default:
                crWarning( "glXChooseVisual: bad attrib=0x%x, ignoring", *attrib );
                attrib++;
                //return NULL;
        }
    }

    if (!useRGBA)
        return NULL;

    XLOCK(dpy);
    searchvis.visualid = XVisualIDFromVisual(DefaultVisual(dpy, screen));
    pret = XGetVisualInfo(dpy, VisualIDMask, &searchvis, &nvisuals);
    XUNLOCK(dpy);

    if (nvisuals!=1) crWarning("glXChooseVisual: XGetVisualInfo returned %i visuals for %x", nvisuals, (unsigned int) searchvis.visualid);
    if (pret)
      crDebug("glXChooseVisual returned %x depth=%i", (unsigned int)pret->visualid, pret->depth);
    return pret;

err_exit:
    crDebug("glXChooseVisual returning NULL, due to attrib=0x%x, next=0x%x", attrib[0], attrib[1]);
    return NULL;
}

/**
 **  There is a problem with glXCopyContext.
 ** IRIX and Mesa both define glXCopyContext
 ** to have the mask argument being a
 ** GLuint.  XFree 4 and oss.sgi.com
 ** define it to be an unsigned long.
 ** Solution: We don't support
 ** glXCopyContext anyway so we'll just
 ** \#ifdef out the code.
 */
DECLEXPORT(void)
VBOXGLXTAG(glXCopyContext)( Display *dpy, GLXContext src, GLXContext dst,
#if defined(AIX) || defined(PLAYSTATION2)
GLuint mask
#elif defined(SunOS)
unsigned long mask
#else
unsigned long mask
#endif
)
{
    (void) dpy;
    (void) src;
    (void) dst;
    (void) mask;
    crWarning( "Unsupported GLX Call: glXCopyContext()" );
}


/**
 * Get the display string for the given display pointer.
 * Never return just ":0.0".  In that case, prefix with our host name.
 */
static void
stubGetDisplayString( Display *dpy, char *nameResult, int maxResult )
{
    const char *dpyName = DisplayString(dpy);
    char host[1000];

    host[0] = 0;
    if (crStrlen(host) + crStrlen(dpyName) >= maxResult - 1)
    {
        /* return null string */
        crWarning("Very long host / display name string in stubDisplayString!");
        nameResult[0] = 0;
    }
    else
    {
        /* return host concatenated with dpyName */
        crStrcpy(nameResult, host);
        crStrcat(nameResult, dpyName);
    }
}



DECLEXPORT(GLXContext)
VBOXGLXTAG(glXCreateContext)(Display *dpy, XVisualInfo *vis, GLXContext share, Bool direct)
{
    char dpyName[MAX_DPY_NAME];
    ContextInfo *context;
    int visBits = CR_RGB_BIT | CR_DOUBLE_BIT | CR_DEPTH_BIT; /* default vis */

    (void)vis;
    stubInit();

    CRASSERT(stub.contextTable);

    /*
    {
        int i, numExt;
        char **list;

        list = XListExtensions(dpy, &numExt);
        crDebug("X extensions [%i]:", numExt);
        for (i=0; i<numExt; ++i)
        {
            crDebug("%s", list[i]);
        }
        XFreeExtensionList(list);
    }
    */

    stubGetDisplayString(dpy, dpyName, MAX_DPY_NAME);

    context = stubNewContext(dpyName, visBits, UNDECIDED, (unsigned long) share);
    if (!context)
        return 0;

    context->dpy = dpy;
    context->direct = direct;

    stubQueryXDamageExtension(dpy, context);

    return (GLXContext) context->id;
}


DECLEXPORT(void) VBOXGLXTAG(glXDestroyContext)( Display *dpy, GLXContext ctx )
{
    (void) dpy;
    stubDestroyContext( (unsigned long) ctx );
}

typedef struct _stubFindPixmapParms_t {
    ContextInfo *pCtx;
    GLX_Pixmap_t *pGlxPixmap;
    GLXDrawable draw;
} stubFindPixmapParms_t;

static void stubFindPixmapCB(unsigned long key, void *data1, void *data2)
{
    ContextInfo *pCtx = (ContextInfo *) data1;
    stubFindPixmapParms_t *pParms = (stubFindPixmapParms_t *) data2;
    GLX_Pixmap_t *pGlxPixmap = (GLX_Pixmap_t *) crHashtableSearch(pCtx->pGLXPixmapsHash, (unsigned int) pParms->draw);
    (void)key;

    if (pGlxPixmap)
    {
        pParms->pCtx = pCtx;
        pParms->pGlxPixmap = pGlxPixmap;
    }
}

DECLEXPORT(Bool) VBOXGLXTAG(glXMakeCurrent)( Display *dpy, GLXDrawable drawable, GLXContext ctx )
{
    ContextInfo *context;
    WindowInfo *window;
    Bool retVal;

    /*crDebug("glXMakeCurrent(%p, 0x%x, 0x%x)", (void *) dpy, (int) drawable, (int) ctx);*/

    /*check if passed drawable is GLXPixmap and not X Window*/
    if (drawable)
    {
        GLX_Pixmap_t *pGlxPixmap = (GLX_Pixmap_t *) crHashtableSearch(stub.pGLXPixmapsHash, (unsigned int) drawable);

        if (!pGlxPixmap)
        {
            stubFindPixmapParms_t parms;
            parms.pGlxPixmap = NULL;
            parms.draw = drawable;
            crHashtableWalk(stub.contextTable, stubFindPixmapCB, &parms);
            pGlxPixmap = parms.pGlxPixmap;
        }

        if (pGlxPixmap)
        {
            /** @todo */
            crWarning("Unimplemented glxMakeCurrent call with GLXPixmap passed, unexpected things might happen.");
        }
    }

    if (ctx && drawable)
    {
        crHashtableLock(stub.windowTable);
        crHashtableLock(stub.contextTable);

        context = (ContextInfo *) crHashtableSearch(stub.contextTable, (unsigned long) ctx);
        window = stubGetWindowInfo(dpy, drawable);

        if (context && context->type == UNDECIDED) {
            XLOCK(dpy);
            XSync(dpy, 0); /* sync to force window creation on the server */
            XUNLOCK(dpy);
        }
    }
    else
    {
        dpy = NULL;
        window = NULL;
        context = NULL;
    }

    currentDisplay = dpy;
    currentDrawable = drawable;

    retVal = stubMakeCurrent(window, context);

    if (ctx && drawable)
    {
        crHashtableUnlock(stub.contextTable);
        crHashtableUnlock(stub.windowTable);
    }

    return retVal;
}

DECLEXPORT(GLXPixmap) VBOXGLXTAG(glXCreateGLXPixmap)( Display *dpy, XVisualInfo *vis, Pixmap pixmap )
{
    stubInit();
    return VBOXGLXTAG(glXCreatePixmap)(dpy, fbConfigFromVisual(dpy, vis), pixmap, NULL);
}

DECLEXPORT(void) VBOXGLXTAG(glXDestroyGLXPixmap)( Display *dpy, GLXPixmap pix )
{
    VBOXGLXTAG(glXDestroyPixmap)(dpy, pix);
}

DECLEXPORT(int) VBOXGLXTAG(glXGetConfig)( Display *dpy, XVisualInfo *vis, int attrib, int *value )
{
    if (!vis) {
        /* SGI OpenGL Performer hits this */
        crWarning("glXGetConfig called with NULL XVisualInfo");
        return GLX_BAD_VISUAL;
    }

    stubInit();

    *value = 0;  /* For sanity */

    switch ( attrib ) {

        case GLX_USE_GL:
            *value = isGLXVisual(dpy, vis);
            break;

        case GLX_BUFFER_SIZE:
            *value = 32;
            break;

        case GLX_LEVEL:
            *value = 0;  /* for now */
            break;

        case GLX_RGBA:
            *value = 1;
            break;

        case GLX_DOUBLEBUFFER:
            *value = 1;
            break;

        case GLX_STEREO:
            *value = 1;
            break;

        case GLX_AUX_BUFFERS:
            *value = 0;
            break;

        case GLX_RED_SIZE:
            *value = 8;
            break;

        case GLX_GREEN_SIZE:
            *value = 8;
            break;

        case GLX_BLUE_SIZE:
            *value = 8;
            break;

        case GLX_ALPHA_SIZE:
            *value = 8;
            break;

        case GLX_DEPTH_SIZE:
            *value = 24;
            break;

        case GLX_STENCIL_SIZE:
            *value = 8;
            break;

        case GLX_ACCUM_RED_SIZE:
            *value = 16;
            break;

        case GLX_ACCUM_GREEN_SIZE:
            *value = 16;
            break;

        case GLX_ACCUM_BLUE_SIZE:
            *value = 16;
            break;

        case GLX_ACCUM_ALPHA_SIZE:
            *value = 16;
            break;

        case GLX_SAMPLE_BUFFERS_SGIS:
            *value = 0;  /* fix someday */
            break;

        case GLX_SAMPLES_SGIS:
            *value = 0;  /* fix someday */
            break;

        case GLX_VISUAL_CAVEAT_EXT:
            *value = GLX_NONE_EXT;
            break;
#if defined(SunOS) || 1
        /*
          I don't think this is even a valid attribute for glxGetConfig.
          No idea why this gets called under SunOS but we simply ignore it
          -- jw
        */
        case GLX_X_VISUAL_TYPE:
          crWarning ("Ignoring Unsupported GLX Call: glxGetConfig with attrib 0x%x", attrib);
          break;
#endif

        case GLX_TRANSPARENT_TYPE:
            *value = GLX_NONE_EXT;
            break;
        case GLX_TRANSPARENT_INDEX_VALUE:
            *value = 0;
            break;
        case GLX_TRANSPARENT_RED_VALUE:
            *value = 0;
            break;
        case GLX_TRANSPARENT_GREEN_VALUE:
            *value = 0;
            break;
        case GLX_TRANSPARENT_BLUE_VALUE:
            *value = 0;
            break;
        case GLX_TRANSPARENT_ALPHA_VALUE:
            *value = 0;
            break;
        case GLX_DRAWABLE_TYPE:
            *value = GLX_WINDOW_BIT;
            break;
        default:
            crWarning( "Unsupported GLX Call: glXGetConfig with attrib 0x%x, ignoring...", attrib );
            //return GLX_BAD_ATTRIBUTE;
            *value = 0;
    }

    return 0;
}

DECLEXPORT(GLXContext) VBOXGLXTAG(glXGetCurrentContext)( void )
{
    ContextInfo *context = stubGetCurrentContext();
    if (context)
        return (GLXContext) context->id;
    else
        return (GLXContext) NULL;
}

DECLEXPORT(GLXDrawable) VBOXGLXTAG(glXGetCurrentDrawable)(void)
{
    return currentDrawable;
}

DECLEXPORT(Display *) VBOXGLXTAG(glXGetCurrentDisplay)(void)
{
    return currentDisplay;
}

DECLEXPORT(Bool) VBOXGLXTAG(glXIsDirect)(Display *dpy, GLXContext ctx)
{
    (void) dpy;
    (void) ctx;
    crDebug("->glXIsDirect");
    return True;
}

DECLEXPORT(Bool) VBOXGLXTAG(glXQueryExtension)(Display *dpy, int *errorBase, int *eventBase)
{
    (void) dpy;
    (void) errorBase;
    (void) eventBase;
    return 1; /* You BET we do... */
}

DECLEXPORT(Bool) VBOXGLXTAG(glXQueryVersion)( Display *dpy, int *major, int *minor )
{
    (void) dpy;
    *major = 1;
    *minor = 3;
    return 1;
}

DECLEXPORT(void) VBOXGLXTAG(glXSwapBuffers)( Display *dpy, GLXDrawable drawable )
{
    WindowInfo *window = stubGetWindowInfo(dpy, drawable);
    stubSwapBuffers( window, 0 );
}

DECLEXPORT(void) VBOXGLXTAG(glXUseXFont)( Font font, int first, int count, int listBase )
{
    ContextInfo *context = stubGetCurrentContext();
    Display *dpy = context->dpy;
    if (dpy) {
        stubUseXFont( dpy, font, first, count, listBase );
    }
    else {
        dpy = XOpenDisplay(NULL);
        if (!dpy)
            return;
        stubUseXFont( dpy, font, first, count, listBase );
        XCloseDisplay(dpy);
    }
}

DECLEXPORT(void) VBOXGLXTAG(glXWaitGL)( void )
{
    static int first_call = 1;

    if ( first_call )
    {
        crDebug( "Ignoring unsupported GLX call: glXWaitGL()" );
        first_call = 0;
    }
}

DECLEXPORT(void) VBOXGLXTAG(glXWaitX)( void )
{
    static int first_call = 1;

    if ( first_call )
    {
        crDebug( "Ignoring unsupported GLX call: glXWaitX()" );
        first_call = 0;
    }
}

DECLEXPORT(const char *) VBOXGLXTAG(glXQueryExtensionsString)( Display *dpy, int screen )
{
    /* XXX maybe also advertise GLX_SGIS_multisample? */

    static const char *retval = "GLX_ARB_multisample GLX_EXT_texture_from_pixmap GLX_SGIX_fbconfig GLX_ARB_get_proc_address";

    (void) dpy;
    (void) screen;

    crDebug("->glXQueryExtensionsString");
    return retval;
}

DECLEXPORT(const char *) VBOXGLXTAG(glXGetClientString)( Display *dpy, int name )
{
    const char *retval;
    (void) dpy;
    (void) name;

    switch ( name ) {

        case GLX_VENDOR:
            retval  = "Chromium";
            break;

        case GLX_VERSION:
            retval  = "1.3 Chromium";
            break;

        case GLX_EXTENSIONS:
            /** @todo should be a screen not a name...but it's not used anyway*/
            retval  = glXQueryExtensionsString(dpy, name);
            break;

        default:
            retval  = NULL;
    }

    return retval;
}

DECLEXPORT(const char *) VBOXGLXTAG(glXQueryServerString)( Display *dpy, int screen, int name )
{
    const char *retval;
    (void) dpy;
    (void) screen;

    switch ( name ) {

        case GLX_VENDOR:
            retval  = "Chromium";
            break;

        case GLX_VERSION:
            retval  = "1.3 Chromium";
            break;

        case GLX_EXTENSIONS:
            retval  = glXQueryExtensionsString(dpy, screen);
            break;

        default:
            retval  = NULL;
    }

    return retval;
}

DECLEXPORT(CR_GLXFuncPtr) VBOXGLXTAG(glXGetProcAddressARB)( const GLubyte *name )
{
    return (CR_GLXFuncPtr) crGetProcAddress( (const char *) name );
}

DECLEXPORT(CR_GLXFuncPtr) VBOXGLXTAG(glXGetProcAddress)( const GLubyte *name )
{
    return (CR_GLXFuncPtr) crGetProcAddress( (const char *) name );
}


#if GLX_EXTRAS

DECLEXPORT(GLXPbufferSGIX)
VBOXGLXTAG(glXCreateGLXPbufferSGIX)(Display *dpy, GLXFBConfigSGIX config,
                                    unsigned int width, unsigned int height,
                                    int *attrib_list)
{
    (void) dpy;
    (void) config;
    (void) width;
    (void) height;
    (void) attrib_list;
    crWarning("glXCreateGLXPbufferSGIX not implemented by Chromium");
    return 0;
}

DECLEXPORT(void) VBOXGLXTAG(glXDestroyGLXPbufferSGIX)(Display *dpy, GLXPbuffer pbuf)
{
    (void) dpy;
    (void) pbuf;
    crWarning("glXDestroyGLXPbufferSGIX not implemented by Chromium");
}

DECLEXPORT(void) VBOXGLXTAG(glXSelectEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
    (void) dpy;
    (void) drawable;
    (void) mask;
}

DECLEXPORT(void) VBOXGLXTAG(glXGetSelectedEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
    (void) dpy;
    (void) drawable;
    (void) mask;
}

DECLEXPORT(int) VBOXGLXTAG(glXQueryGLXPbufferSGIX)(Display *dpy, GLXPbuffer pbuf,
                                                   int attribute, unsigned int *value)
{
    (void) dpy;
    (void) pbuf;
    (void) attribute;
    (void) value;
    crWarning("glXQueryGLXPbufferSGIX not implemented by Chromium");
    return 0;
}

DECLEXPORT(int) VBOXGLXTAG(glXGetFBConfigAttribSGIX)(Display *dpy, GLXFBConfig config,
                                                     int attribute, int *value)
{
    return VBOXGLXTAG(glXGetFBConfigAttrib)(dpy, config, attribute, value);
}

DECLEXPORT(GLXFBConfigSGIX *)
VBOXGLXTAG(glXChooseFBConfigSGIX)(Display *dpy, int screen,
                                  int *attrib_list, int *nelements)
{
    return VBOXGLXTAG(glXChooseFBConfig)(dpy, screen, attrib_list, nelements);
}

DECLEXPORT(GLXPixmap)
VBOXGLXTAG(glXCreateGLXPixmapWithConfigSGIX)(Display *dpy,
                                             GLXFBConfig config,
                                             Pixmap pixmap)
{
    return VBOXGLXTAG(glXCreatePixmap)(dpy, config, pixmap, NULL);
}

DECLEXPORT(GLXContext)
VBOXGLXTAG(glXCreateContextWithConfigSGIX)(Display *dpy, GLXFBConfig config,
                                           int render_type,
                                           GLXContext share_list,
                                           Bool direct)
{
    (void)config;
    if (render_type!=GLX_RGBA_TYPE_SGIX)
    {
        crWarning("glXCreateContextWithConfigSGIX: Unsupported render type %i", render_type);
        return NULL;
    }
    return VBOXGLXTAG(glXCreateContext)(dpy, NULL, share_list, direct);
}

DECLEXPORT(XVisualInfo *)
VBOXGLXTAG(glXGetVisualFromFBConfigSGIX)(Display *dpy,
                                         GLXFBConfig config)
{
    return visualInfoFromFBConfig(dpy, config);
}

DECLEXPORT(GLXFBConfigSGIX)
VBOXGLXTAG(glXGetFBConfigFromVisualSGIX)(Display *dpy, XVisualInfo *vis)
{
    if (!vis)
    {
        return NULL;
    }
    /*Note: Caller is supposed to call XFree on returned value, so can't just return (GLXFBConfig)vis->visualid*/
    return (GLXFBConfigSGIX) visualInfoFromFBConfig(dpy, fbConfigFromVisual(dpy, vis));
}

/*
 * GLX 1.3 functions
 */
DECLEXPORT(GLXFBConfig *)
VBOXGLXTAG(glXChooseFBConfig)(Display *dpy, int screen, ATTRIB_TYPE *attrib_list, int *nelements)
{
    ATTRIB_TYPE *attrib;
    intptr_t fbconfig = 0;

    stubInit();

    if (!attrib_list)
    {
        return VBOXGLXTAG(glXGetFBConfigs)(dpy, screen, nelements);
    }

    for (attrib = attrib_list; *attrib != None; attrib++)
    {
        switch (*attrib)
        {
            case GLX_FBCONFIG_ID:
                fbconfig = attrib[1];
                attrib++;
                break;

            case GLX_BUFFER_SIZE:
                /* this is ignored except for color-index visuals, which we don't support */
                attrib++;
                break;

            case GLX_LEVEL:
                if (attrib[1] != 0)
                    goto err_exit;
                attrib++;
                break;

            case GLX_AUX_BUFFERS:
                if (attrib[1] != 0)
                    goto err_exit;
                attrib++;
                break;

            case GLX_DOUBLEBUFFER: /* @todo, check if we support it */
                attrib++;
                break;

            case GLX_STEREO:
                if (attrib[1] != 0)
                    goto err_exit;
                attrib++;
                break;

            case GLX_RED_SIZE:
            case GLX_GREEN_SIZE:
            case GLX_BLUE_SIZE:
            case GLX_ALPHA_SIZE:
                if (attrib[1] > 8)
                    goto err_exit;
                attrib++;
                break;

            case GLX_DEPTH_SIZE:
                if (attrib[1] > 24)
                    goto err_exit;
                attrib++;
                break;

            case GLX_STENCIL_SIZE:
                if (attrib[1] > 8)
                    goto err_exit;
                attrib++;
                break;

            case GLX_ACCUM_RED_SIZE:
            case GLX_ACCUM_GREEN_SIZE:
            case GLX_ACCUM_BLUE_SIZE:
            case GLX_ACCUM_ALPHA_SIZE:
                if (attrib[1] > 16)
                    goto err_exit;
                attrib++;
                break;

            case GLX_X_RENDERABLE:
            case GLX_CONFIG_CAVEAT:
                attrib++;
                break;

            case GLX_RENDER_TYPE:
                if (attrib[1]!=GLX_RGBA_BIT)
                    goto err_exit;
                attrib++;
                break;

            case GLX_DRAWABLE_TYPE:
                if (   !(attrib[1] & GLX_WINDOW_BIT)
                    && !(attrib[1] & GLX_PIXMAP_BIT))
                    goto err_exit;
                attrib++;
                break;

            case GLX_X_VISUAL_TYPE:
            case GLX_TRANSPARENT_TYPE_EXT:
            case GLX_TRANSPARENT_INDEX_VALUE_EXT:
            case GLX_TRANSPARENT_RED_VALUE_EXT:
            case GLX_TRANSPARENT_GREEN_VALUE_EXT:
            case GLX_TRANSPARENT_BLUE_VALUE_EXT:
            case GLX_TRANSPARENT_ALPHA_VALUE_EXT:
                /* ignore */
                crWarning("glXChooseVisual: ignoring attribute 0x%x", *attrib);
                attrib++;
                break;

                break;
            default:
                crWarning( "glXChooseVisual: bad attrib=0x%x, ignoring", *attrib );
                attrib++;
                break;
        }
    }

    if (fbconfig)
    {
        GLXFBConfig *pGLXFBConfigs;

        *nelements = 1;
        pGLXFBConfigs = (GLXFBConfig *) crAlloc(*nelements * sizeof(GLXFBConfig));
        pGLXFBConfigs[0] = (GLXFBConfig)fbconfig;
        return pGLXFBConfigs;
    }
    else
    {
        return VBOXGLXTAG(glXGetFBConfigs)(dpy, screen, nelements);
    }

err_exit:
    crWarning("glXChooseFBConfig returning NULL, due to attrib=0x%x, next=0x%x", attrib[0], attrib[1]);
    return NULL;
}

DECLEXPORT(GLXContext)
VBOXGLXTAG(glXCreateNewContext)(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct)
{
    (void) dpy;
    (void) config;
    (void) render_type;
    (void) share_list;
    (void) direct;

    if (render_type != GLX_RGBA_TYPE)
    {
        crWarning("glXCreateNewContext, unsupported render_type %x", render_type);
        return NULL;
    }

    return VBOXGLXTAG(glXCreateContext)(dpy, NULL, share_list, direct);
}

DECLEXPORT(GLXPbuffer)
VBOXGLXTAG(glXCreatePbuffer)(Display *dpy, GLXFBConfig config, ATTRIB_TYPE *attrib_list)
{
    (void) dpy;
    (void) config;
    (void) attrib_list;
    crWarning("glXCreatePbuffer not implemented by Chromium");
    return 0;
}

/* Note: there're examples where glxpixmaps are created without current context, so can't do much of the work here.
 * Instead we'd do necessary initialization on first use of those pixmaps.
 */
DECLEXPORT(GLXPixmap)
VBOXGLXTAG(glXCreatePixmap)(Display *dpy, GLXFBConfig config, Pixmap pixmap, ATTRIB_TYPE *attrib_list)
{
    ATTRIB_TYPE *attrib;
    GLX_Pixmap_t *pGlxPixmap;
    (void) dpy;
    (void) config;

#if 0
    {
        int x, y;
        unsigned int w, h;
        unsigned int border;
        unsigned int depth;
        Window root;

        crDebug("glXCreatePixmap called for %lu", pixmap);

        XLOCK(dpy);
        if (!XGetGeometry(dpy, pixmap, &root, &x, &y, &w, &h, &border, &depth))
        {
            XSync(dpy, False);
            if (!XGetGeometry(dpy, pixmap, &root, &x, &y, &w, &h, &border, &depth))
            {
                crDebug("fail");
            }
        }
        crDebug("root: %lu, [%i,%i %u,%u]", root, x, y, w, h);
        XUNLOCK(dpy);
    }
#endif

    pGlxPixmap = crCalloc(sizeof(GLX_Pixmap_t));
    if (!pGlxPixmap)
    {
        crWarning("glXCreatePixmap failed to allocate memory");
        return 0;
    }

    pGlxPixmap->format = GL_RGBA;
    pGlxPixmap->target = GL_TEXTURE_2D;

    if (attrib_list)
    {
        for (attrib = attrib_list; *attrib != None; attrib++)
        {
            switch (*attrib)
            {
                case GLX_TEXTURE_FORMAT_EXT:
                    attrib++;
                    switch (*attrib)
                    {
                        case GLX_TEXTURE_FORMAT_RGBA_EXT:
                            pGlxPixmap->format = GL_RGBA;
                            break;
                        case GLX_TEXTURE_FORMAT_RGB_EXT:
                            pGlxPixmap->format = GL_RGB;
                            break;
                        default:
                            crDebug("Unexpected GLX_TEXTURE_FORMAT_EXT 0x%x", (unsigned int) *attrib);
                    }
                    break;
                case GLX_TEXTURE_TARGET_EXT:
                    attrib++;
                    switch (*attrib)
                    {
                        case GLX_TEXTURE_2D_EXT:
                            pGlxPixmap->target = GL_TEXTURE_2D;
                            break;
                        case GLX_TEXTURE_RECTANGLE_EXT:
                            pGlxPixmap->target = GL_TEXTURE_RECTANGLE_NV;
                            break;
                        default:
                            crDebug("Unexpected GLX_TEXTURE_TARGET_EXT 0x%x", (unsigned int) *attrib);
                    }
                    break;
                default: attrib++;
            }
        }
    }

    crHashtableAdd(stub.pGLXPixmapsHash, (unsigned int) pixmap, pGlxPixmap);
    return (GLXPixmap) pixmap;
}

DECLEXPORT(GLXWindow)
VBOXGLXTAG(glXCreateWindow)(Display *dpy, GLXFBConfig config, Window win, ATTRIB_TYPE *attrib_list)
{
    GLXFBConfig *realcfg;
    int nconfigs;
    (void) config;

    if (stub.wsInterface.glXGetFBConfigs)
    {
        realcfg = stub.wsInterface.glXGetFBConfigs(dpy, 0, &nconfigs);
        if (!realcfg || nconfigs<1)
        {
            crWarning("glXCreateWindow !realcfg || nconfigs<1");
            return 0;
        }
        else
        {
            return stub.wsInterface.glXCreateWindow(dpy, realcfg[0], win, attrib_list);
        }
    }
    else
    {
        if (attrib_list && *attrib_list!=None)
        {
            crWarning("Non empty attrib list in glXCreateWindow");
            return 0;
        }
        return (GLXWindow)win;
    }
}

DECLEXPORT(void) VBOXGLXTAG(glXDestroyPbuffer)(Display *dpy, GLXPbuffer pbuf)
{
    (void) dpy;
    (void) pbuf;
    crWarning("glXDestroyPbuffer not implemented by Chromium");
}

DECLEXPORT(void) VBOXGLXTAG(glXDestroyPixmap)(Display *dpy, GLXPixmap pixmap)
{
    stubFindPixmapParms_t parms;

    if (crHashtableSearch(stub.pGLXPixmapsHash, (unsigned int) pixmap))
    {
        /*it's valid but never used glxpixmap, so simple free stored ptr*/
        crHashtableDelete(stub.pGLXPixmapsHash, (unsigned int) pixmap, crFree);
        return;
    }
    else
    {
        /*it's either invalid glxpixmap or one which was already initialized, so it's stored in appropriate ctx hash*/
        parms.pCtx = NULL;
        parms.pGlxPixmap = NULL;
        parms.draw = pixmap;
        crHashtableWalk(stub.contextTable, stubFindPixmapCB, &parms);
    }

    if (!parms.pGlxPixmap)
    {
        crWarning("glXDestroyPixmap called for unknown glxpixmap 0x%x", (unsigned int) pixmap);
        return;
    }

    XLOCK(dpy);
    if (parms.pGlxPixmap->gc)
    {
        XFreeGC(dpy, parms.pGlxPixmap->gc);
    }

    if (parms.pGlxPixmap->hShmPixmap>0)
    {
        XFreePixmap(dpy, parms.pGlxPixmap->hShmPixmap);
    }
    XUNLOCK(dpy);

    if (parms.pGlxPixmap->hDamage>0)
    {
        //crDebug("Destroy: Damage for drawable 0x%x, handle 0x%x", (unsigned int) pixmap, (unsigned int) parms.pGlxPixmap->damage);
        XDamageDestroy(dpy, parms.pGlxPixmap->hDamage);
    }

    if (parms.pGlxPixmap->pDamageRegion)
    {
        XDestroyRegion(parms.pGlxPixmap->pDamageRegion);
    }

    crHashtableDelete(parms.pCtx->pGLXPixmapsHash, (unsigned int) pixmap, crFree);
}

DECLEXPORT(void) VBOXGLXTAG(glXDestroyWindow)(Display *dpy, GLXWindow win)
{
    (void) dpy;
    (void) win;
    /*crWarning("glXDestroyWindow not implemented by Chromium");*/
}

DECLEXPORT(GLXDrawable) VBOXGLXTAG(glXGetCurrentReadDrawable)(void)
{
    return currentReadDrawable;
}

DECLEXPORT(int) VBOXGLXTAG(glXGetFBConfigAttrib)(Display *dpy, GLXFBConfig config, int attribute, int *value)
{
    XVisualInfo * pVisual;
    const char * pExt;

    switch (attribute)
    {
        case GLX_DRAWABLE_TYPE:
            *value = GLX_PIXMAP_BIT | GLX_WINDOW_BIT;
            break;
        case GLX_BIND_TO_TEXTURE_TARGETS_EXT:
            *value = GLX_TEXTURE_2D_BIT_EXT;
            pExt = (const char *) stub.spu->dispatch_table.GetString(GL_EXTENSIONS);
            if (crStrstr(pExt, "GL_NV_texture_rectangle")
                || crStrstr(pExt, "GL_ARB_texture_rectangle")
                || crStrstr(pExt, "GL_EXT_texture_rectangle"))
            {
                *value |= GLX_TEXTURE_RECTANGLE_BIT_EXT;
            }
            break;
        case GLX_BIND_TO_TEXTURE_RGBA_EXT:
            *value = True;
            break;
        case GLX_BIND_TO_TEXTURE_RGB_EXT:
            *value = True;
            break;
        case GLX_DOUBLEBUFFER:
            //crDebug("attribute=GLX_DOUBLEBUFFER");
            *value = True;
            break;
        case GLX_Y_INVERTED_EXT:
            *value = True;
            break;
        case GLX_ALPHA_SIZE:
            //crDebug("attribute=GLX_ALPHA_SIZE");
            *value = 8;
            break;
        case GLX_BUFFER_SIZE:
            //crDebug("attribute=GLX_BUFFER_SIZE");
            *value = 32;
            break;
        case GLX_STENCIL_SIZE:
            //crDebug("attribute=GLX_STENCIL_SIZE");
            *value = 8;
            break;
        case GLX_DEPTH_SIZE:
            *value = 24;
            //crDebug("attribute=GLX_DEPTH_SIZE");
            break;
        case GLX_BIND_TO_MIPMAP_TEXTURE_EXT:
            *value = 0;
            break;
        case GLX_RENDER_TYPE:
            //crDebug("attribute=GLX_RENDER_TYPE");
            *value = GLX_RGBA_BIT;
            break;
        case GLX_CONFIG_CAVEAT:
            //crDebug("attribute=GLX_CONFIG_CAVEAT");
            *value = GLX_NONE;
            break;
        case GLX_VISUAL_ID:
            //crDebug("attribute=GLX_VISUAL_ID");
            pVisual = visualInfoFromFBConfig(dpy, config);
            if (!pVisual)
            {
                crWarning("glXGetFBConfigAttrib for %p, failed to get XVisualInfo", config);
                return GLX_BAD_ATTRIBUTE;
            }
            *value = pVisual->visualid;
            XFree(pVisual);
            break;
        case GLX_FBCONFIG_ID:
            *value = (int)(intptr_t)config;
            break;
        case GLX_RED_SIZE:
        case GLX_GREEN_SIZE:
        case GLX_BLUE_SIZE:
            *value = 8;
            break;
        case GLX_LEVEL:
            *value = 0;
            break;
        case GLX_STEREO:
            *value = false;
            break;
        case GLX_AUX_BUFFERS:
            *value = 0;
            break;
        case GLX_ACCUM_RED_SIZE:
        case GLX_ACCUM_GREEN_SIZE:
        case GLX_ACCUM_BLUE_SIZE:
        case GLX_ACCUM_ALPHA_SIZE:
            *value = 0;
            break;
        case GLX_X_VISUAL_TYPE:
            *value = GLX_TRUE_COLOR;
            break;
        case GLX_TRANSPARENT_TYPE:
            *value = GLX_NONE;
            break;
        case GLX_SAMPLE_BUFFERS:
        case GLX_SAMPLES:
            *value = 1;
            break;
        case GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT:
            *value = 0;
            break;
        default:
            crDebug("glXGetFBConfigAttrib: unknown attribute=0x%x", attribute);
            return GLX_BAD_ATTRIBUTE;
    }

    return Success;
}

DECLEXPORT(GLXFBConfig *) VBOXGLXTAG(glXGetFBConfigs)(Display *dpy, int screen, int *nelements)
{
    int i;

    GLXFBConfig *pGLXFBConfigs = crAlloc(sizeof(GLXFBConfig));

    *nelements = 1;
    XLOCK(dpy);
    *pGLXFBConfigs = defaultFBConfigForScreen(dpy, screen);
    XUNLOCK(dpy);

    crDebug("glXGetFBConfigs returned %i configs", *nelements);
    for (i=0; i<*nelements; ++i)
    {
        crDebug("glXGetFBConfigs[%i]=0x%x", i, (unsigned)(uintptr_t) pGLXFBConfigs[i]);
    }
    return pGLXFBConfigs;
}

DECLEXPORT(void) VBOXGLXTAG(glXGetSelectedEvent)(Display *dpy, GLXDrawable draw, unsigned long *event_mask)
{
    (void) dpy;
    (void) draw;
    (void) event_mask;
    crWarning("glXGetSelectedEvent not implemented by Chromium");
}

DECLEXPORT(XVisualInfo *) VBOXGLXTAG(glXGetVisualFromFBConfig)(Display *dpy, GLXFBConfig config)
{
    return visualInfoFromFBConfig(dpy, config);
}

DECLEXPORT(Bool) VBOXGLXTAG(glXMakeContextCurrent)(Display *display, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
    currentReadDrawable = read;
    return VBOXGLXTAG(glXMakeCurrent)(display, draw, ctx);
}

DECLEXPORT(int) VBOXGLXTAG(glXQueryContext)(Display *dpy, GLXContext ctx, int attribute, int *value)
{
    (void) dpy;
    (void) ctx;
    (void) attribute;
    (void) value;
    crWarning("glXQueryContext not implemented by Chromium");
    return 0;
}

DECLEXPORT(void) VBOXGLXTAG(glXQueryDrawable)(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value)
{
    (void) dpy;
    (void) draw;
    (void) attribute;
    (void) value;
    crWarning("glXQueryDrawable not implemented by Chromium");
}

DECLEXPORT(void) VBOXGLXTAG(glXSelectEvent)(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
    (void) dpy;
    (void) draw;
    (void) event_mask;
    crWarning("glXSelectEvent not implemented by Chromium");
}

#ifdef CR_EXT_texture_from_pixmap
/*typedef struct
{
    int x, y;
    unsigned int w, h, border, depth;
    Window root;
    void *data;
} pminfo;*/

static void stubInitXSharedMemory(Display *dpy)
{
    int vma, vmi;
    Bool pixmaps;

    if (stub.bShmInitFailed || stub.xshmSI.shmid>=0)
        return;

    stub.bShmInitFailed = GL_TRUE;

    /* Check for extension and pixmaps format */
    XLOCK(dpy);
    if (!XShmQueryExtension(dpy))
    {
        crWarning("No XSHM extension");
        XUNLOCK(dpy);
        return;
    }

    if (!XShmQueryVersion(dpy, &vma, &vmi, &pixmaps) || !pixmaps)
    {
        crWarning("XSHM extension doesn't support pixmaps");
        XUNLOCK(dpy);
        return;
    }

    if (XShmPixmapFormat(dpy)!=ZPixmap)
    {
        crWarning("XSHM extension doesn't support ZPixmap format");
        XUNLOCK(dpy);
        return;
    }
    XUNLOCK(dpy);

    /* Alloc shared memory, so far using hardcoded value...could fail for bigger displays one day */
    stub.xshmSI.readOnly = false;
    stub.xshmSI.shmid = shmget(IPC_PRIVATE, 4*4096*2048, IPC_CREAT | 0600);
    if (stub.xshmSI.shmid<0)
    {
        crWarning("XSHM Failed to create shared segment");
        return;
    }

    stub.xshmSI.shmaddr = (char*) shmat(stub.xshmSI.shmid, NULL, 0);
    if (stub.xshmSI.shmaddr==(void*)-1)
    {
        crWarning("XSHM Failed to attach shared segment");
        shmctl(stub.xshmSI.shmid, IPC_RMID, 0);
        return;
    }

    XLOCK(dpy);
    if (!XShmAttach(dpy, &stub.xshmSI))
    {
        crWarning("XSHM Failed to attach shared segment to XServer");
        shmctl(stub.xshmSI.shmid, IPC_RMID, 0);
        shmdt(stub.xshmSI.shmaddr);
        XUNLOCK(dpy);
        return;
    }
    XUNLOCK(dpy);

    stub.bShmInitFailed = GL_FALSE;
    crInfo("Using XSHM for GLX_EXT_texture_from_pixmap");

    /*Anyway mark to be deleted when our process detaches it, in case of segfault etc*/

/* Ramshankar: Solaris compiz fix */
#ifndef RT_OS_SOLARIS
    shmctl(stub.xshmSI.shmid, IPC_RMID, 0);
#endif
}

void stubQueryXDamageExtension(Display *dpy, ContextInfo *pContext)
{
    int erb, vma, vmi;

    CRASSERT(pContext);

    if (pContext->damageQueryFailed)
        return;

    pContext->damageQueryFailed = True;

    if (!XDamageQueryExtension(dpy, &pContext->damageEventsBase, &erb)
        || !XDamageQueryVersion(dpy, &vma, &vmi))
    {
        crWarning("XDamage not found or old version (%i.%i), going to run *very* slow", vma, vmi);
        return;
    }

    crDebug("XDamage %i.%i", vma, vmi);
    pContext->damageQueryFailed = False;
}

static void stubFetchDamageOnDrawable(Display *dpy, GLX_Pixmap_t *pGlxPixmap)
{
    Damage damage = pGlxPixmap->hDamage;

    if (damage)
    {
        XRectangle *returnRects;
        int        nReturnRects;

        /* Get the damage region as a server region */
        XserverRegion serverDamageRegion = XFixesCreateRegion (dpy, NULL, 0);

        /* Unite damage region with server region and clear damage region */
        XDamageSubtract (dpy,
                         damage,
                         None, /* subtract all damage from this region */
                         serverDamageRegion /* save in serverDamageRegion */);

        /* Fetch damage rectangles */
        returnRects = XFixesFetchRegion (dpy, serverDamageRegion, &nReturnRects);

        /* Delete region */
        XFixesDestroyRegion (dpy, serverDamageRegion);

        if (pGlxPixmap->pDamageRegion)
        {
            /* If it's dirty and regions are empty, it marked for full update, so do nothing.*/
            if (!pGlxPixmap->bPixmapImageDirty || !XEmptyRegion(pGlxPixmap->pDamageRegion))
            {
                int i = 0;
                for (; i < nReturnRects; ++i)
                {
                    if (CR_MAX_DAMAGE_REGIONS_TRACKED <= pGlxPixmap->pDamageRegion->numRects)
                    {
                        /* Mark for full update */
                        EMPTY_REGION(pGlxPixmap->pDamageRegion);
                    }
                    else
                    {
                        /* Add to damage regions */
                        XUnionRectWithRegion(&returnRects[i], pGlxPixmap->pDamageRegion, pGlxPixmap->pDamageRegion);
                    }
                }
            }
        }

        XFree(returnRects);

        pGlxPixmap->bPixmapImageDirty = True;
    }
}

static const CRPixelPackState defaultPacking =
{
    0,          /*rowLength*/
    0,          /*skipRows*/
    0,          /*skipPixels*/
    1,          /*alignment*/
    0,          /*imageHeight*/
    0,          /*skipImages*/
    GL_FALSE,   /*swapBytes*/
    GL_FALSE    /*lsbFirst*/
};

static void stubGetUnpackState(CRPixelPackState *pUnpackState)
{
    stub.spu->dispatch_table.GetIntegerv(GL_UNPACK_ROW_LENGTH, &pUnpackState->rowLength);
    stub.spu->dispatch_table.GetIntegerv(GL_UNPACK_SKIP_ROWS, &pUnpackState->skipRows);
    stub.spu->dispatch_table.GetIntegerv(GL_UNPACK_SKIP_PIXELS, &pUnpackState->skipPixels);
    stub.spu->dispatch_table.GetIntegerv(GL_UNPACK_ALIGNMENT, &pUnpackState->alignment);
    stub.spu->dispatch_table.GetBooleanv(GL_UNPACK_SWAP_BYTES, &pUnpackState->swapBytes);
    stub.spu->dispatch_table.GetBooleanv(GL_UNPACK_LSB_FIRST, &pUnpackState->psLSBFirst);
}

static void stubSetUnpackState(const CRPixelPackState *pUnpackState)
{
    stub.spu->dispatch_table.PixelStorei(GL_UNPACK_ROW_LENGTH, pUnpackState->rowLength);
    stub.spu->dispatch_table.PixelStorei(GL_UNPACK_SKIP_ROWS, pUnpackState->skipRows);
    stub.spu->dispatch_table.PixelStorei(GL_UNPACK_SKIP_PIXELS, pUnpackState->skipPixels);
    stub.spu->dispatch_table.PixelStorei(GL_UNPACK_ALIGNMENT, pUnpackState->alignment);
    stub.spu->dispatch_table.PixelStorei(GL_UNPACK_SWAP_BYTES, pUnpackState->swapBytes);
    stub.spu->dispatch_table.PixelStorei(GL_UNPACK_LSB_FIRST, pUnpackState->psLSBFirst);
}

static GLX_Pixmap_t* stubInitGlxPixmap(GLX_Pixmap_t* pCreateInfoPixmap, Display *dpy, GLXDrawable draw, ContextInfo *pContext)
{
    int x, y;
    unsigned int w, h;
    unsigned int border;
    unsigned int depth;
    Window root;
    GLX_Pixmap_t *pGlxPixmap;

    CRASSERT(pContext && pCreateInfoPixmap);

    XLOCK(dpy);
    if (!XGetGeometry(dpy, (Pixmap)draw, &root, &x, &y, &w, &h, &border, &depth))
    {
        XSync(dpy, False);
        if (!XGetGeometry(dpy, (Pixmap)draw, &root, &x, &y, &w, &h, &border, &depth))
        {
            crWarning("stubInitGlxPixmap failed in call to XGetGeometry for 0x%x", (int) draw);
            XUNLOCK(dpy);
            return NULL;
        }
    }

    pGlxPixmap = crAlloc(sizeof(GLX_Pixmap_t));
    if (!pGlxPixmap)
    {
        crWarning("stubInitGlxPixmap failed to allocate memory");
        XUNLOCK(dpy);
        return NULL;
    }

    pGlxPixmap->x = x;
    pGlxPixmap->y = y;
    pGlxPixmap->w = w;
    pGlxPixmap->h = h;
    pGlxPixmap->border = border;
    pGlxPixmap->depth = depth;
    pGlxPixmap->root = root;
    pGlxPixmap->format = pCreateInfoPixmap->format;
    pGlxPixmap->target = pCreateInfoPixmap->target;

    /* Try to allocate shared memory
     * As we're allocating huge chunk of memory, do it in this function, only if this extension is really used
     */
    if (!stub.bShmInitFailed && stub.xshmSI.shmid<0)
    {
        stubInitXSharedMemory(dpy);
    }

    if (stub.xshmSI.shmid>=0)
    {
        XGCValues xgcv;
        xgcv.graphics_exposures = False;
        xgcv.subwindow_mode = IncludeInferiors;
        pGlxPixmap->gc = XCreateGC(dpy, (Pixmap)draw, GCGraphicsExposures|GCSubwindowMode, &xgcv);

        pGlxPixmap->hShmPixmap = XShmCreatePixmap(dpy, pGlxPixmap->root, stub.xshmSI.shmaddr, &stub.xshmSI,
                                                  pGlxPixmap->w, pGlxPixmap->h, pGlxPixmap->depth);
    }
    else
    {
        pGlxPixmap->gc = NULL;
        pGlxPixmap->hShmPixmap = 0;
    }
    XUNLOCK(dpy);

    /* If there's damage extension, then get handle for damage events related to this pixmap */
    if (!pContext->damageQueryFailed)
    {
        pGlxPixmap->hDamage = XDamageCreate(dpy, (Pixmap)draw, XDamageReportNonEmpty);
        /*crDebug("Create: Damage for drawable 0x%x, handle 0x%x (level=%i)",
                 (unsigned int) draw, (unsigned int) pGlxPixmap->damage, (int) XDamageReportRawRectangles);*/
        pGlxPixmap->pDamageRegion = XCreateRegion();
        if (!pGlxPixmap->pDamageRegion)
        {
            crWarning("stubInitGlxPixmap failed to create empty damage region for drawable 0x%x", (unsigned int) draw);
        }

        /*We have never seen this pixmap before, so mark it as dirty for first use*/
        pGlxPixmap->bPixmapImageDirty = True;
    }
    else
    {
        pGlxPixmap->hDamage = 0;
        pGlxPixmap->pDamageRegion = NULL;
    }

    /* glTexSubImage2D generates GL_INVALID_OP if texture array hasn't been defined by a call to glTexImage2D first.
     * It's fine for small textures which would be updated in stubXshmUpdateWholeImage, but we'd never call glTexImage2D for big ones.
     * Note that we're making empty texture by passing NULL as pixels pointer, so there's no overhead transferring data to host.*/
    if (CR_MAX_TRANSFER_SIZE < 4*pGlxPixmap->w*pGlxPixmap->h)
    {
        stub.spu->dispatch_table.TexImage2D(pGlxPixmap->target, 0, pGlxPixmap->format, pGlxPixmap->w, pGlxPixmap->h, 0,
                                            GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    }

    crHashtableAdd(pContext->pGLXPixmapsHash, (unsigned int) draw, pGlxPixmap);
    crHashtableDelete(stub.pGLXPixmapsHash, (unsigned int) draw, crFree);

    return pGlxPixmap;
}

static void stubXshmUpdateWholeImage(Display *dpy, GLXDrawable draw, GLX_Pixmap_t *pGlxPixmap)
{
    /* To limit the size of transferring buffer, split bigger texture into regions
     * which fit into connection buffer. Could be done in hgcm or packspu but implementation in this place allows to avoid
     * unnecessary memcpy.
     * This also workarounds guest driver failures when sending 6+mb texture buffers on linux.
     */
    if (CR_MAX_TRANSFER_SIZE < 4*pGlxPixmap->w*pGlxPixmap->h)
    {
        XRectangle rect;

        rect.x = pGlxPixmap->x;
        rect.y = pGlxPixmap->y;
        rect.width = pGlxPixmap->w;
        rect.height = CR_MAX_TRANSFER_SIZE/(4*pGlxPixmap->w);

        /*crDebug("Texture size too big, splitting in lower sized chunks. [%i,%i,%i,%i] (%i)",
                pGlxPixmap->x, pGlxPixmap->y, pGlxPixmap->w, pGlxPixmap->h, rect.height);*/

        for (; (rect.y+rect.height)<=(pGlxPixmap->y+(int)pGlxPixmap->h); rect.y+=rect.height)
        {
            stubXshmUpdateImageRect(dpy, draw, pGlxPixmap, &rect);
        }

        if (rect.y!=(pGlxPixmap->y+(int)pGlxPixmap->h))
        {
            rect.height=pGlxPixmap->h-rect.y;
            stubXshmUpdateImageRect(dpy, draw, pGlxPixmap, &rect);
        }
    }
    else
    {
        CRPixelPackState unpackState;

        XLOCK(dpy);
        XCopyArea(dpy, (Pixmap)draw, pGlxPixmap->hShmPixmap, pGlxPixmap->gc,
                  pGlxPixmap->x, pGlxPixmap->y, pGlxPixmap->w, pGlxPixmap->h, 0, 0);
        /* Have to make sure XCopyArea is processed */
        XSync(dpy, False);
        XUNLOCK(dpy);

        stubGetUnpackState(&unpackState);
        stubSetUnpackState(&defaultPacking);
        stub.spu->dispatch_table.TexImage2D(pGlxPixmap->target, 0, pGlxPixmap->format, pGlxPixmap->w, pGlxPixmap->h, 0,
                                            GL_BGRA, GL_UNSIGNED_BYTE, stub.xshmSI.shmaddr);
        stubSetUnpackState(&unpackState);
        /*crDebug("Sync texture for drawable 0x%x(dmg handle 0x%x) [%i,%i,%i,%i]",
                  (unsigned int) draw, (unsigned int)pGlxPixmap->hDamage,
                  pGlxPixmap->x, pGlxPixmap->y, pGlxPixmap->w, pGlxPixmap->h);*/
    }
}

static void stubXshmUpdateImageRect(Display *dpy, GLXDrawable draw, GLX_Pixmap_t *pGlxPixmap, XRectangle *pRect)
{
    /* See comment in stubXshmUpdateWholeImage */
    if (CR_MAX_TRANSFER_SIZE < 4*pRect->width*pRect->height)
    {
        XRectangle rect;

        rect.x = pRect->x;
        rect.y = pRect->y;
        rect.width = pRect->width;
        rect.height = CR_MAX_TRANSFER_SIZE/(4*pRect->width);

        /*crDebug("Region size too big, splitting in lower sized chunks. [%i,%i,%i,%i] (%i)",
                pRect->x, pRect->y, pRect->width, pRect->height, rect.height);*/

        for (; (rect.y+rect.height)<=(pRect->y+pRect->height); rect.y+=rect.height)
        {
            stubXshmUpdateImageRect(dpy, draw, pGlxPixmap, &rect);
        }

        if (rect.y!=(pRect->y+pRect->height))
        {
            rect.height=pRect->y+pRect->height-rect.y;
            stubXshmUpdateImageRect(dpy, draw, pGlxPixmap, &rect);
        }
    }
    else
    {
        CRPixelPackState unpackState;

        XLOCK(dpy);
        XCopyArea(dpy, (Pixmap)draw, pGlxPixmap->hShmPixmap, pGlxPixmap->gc,
                  pRect->x, pRect->y, pRect->width, pRect->height, 0, 0);
        /* Have to make sure XCopyArea is processed */
        XSync(dpy, False);
        XUNLOCK(dpy);

        stubGetUnpackState(&unpackState);
        stubSetUnpackState(&defaultPacking);
        if (pRect->width!=pGlxPixmap->w)
        {
            stub.spu->dispatch_table.PixelStorei(GL_UNPACK_ROW_LENGTH, pGlxPixmap->w);
        }
        stub.spu->dispatch_table.TexSubImage2D(pGlxPixmap->target, 0, pRect->x, pRect->y, pRect->width, pRect->height,
                                               GL_BGRA, GL_UNSIGNED_BYTE, stub.xshmSI.shmaddr);
        stubSetUnpackState(&unpackState);

        /*crDebug("Region sync texture for drawable 0x%x(dmg handle 0x%x) [%i,%i,%i,%i]",
                (unsigned int) draw, (unsigned int)pGlxPixmap->hDamage,
                pRect->x, pRect->y, pRect->width, pRect->height);*/
    }
}

#if 0
Bool checkevents(Display *display, XEvent *event, XPointer arg)
{
    //crDebug("got type: 0x%x", event->type);
    if (event->type==damage_evb+XDamageNotify)
    {
        ContextInfo *context = stubGetCurrentContext();
        XDamageNotifyEvent *e = (XDamageNotifyEvent *) event;
        /* we're interested in pixmaps only...and those have e->drawable set to 0 or other strange value for some odd reason
         * so have to walk glxpixmaps hashtable to find if we have damage event handle assigned to some pixmap
         */
        /*crDebug("Event: Damage for drawable 0x%x, handle 0x%x (level=%i) [%i,%i,%i,%i]",
                (unsigned int) e->drawable, (unsigned int) e->damage, (int) e->level,
                e->area.x, e->area.y, e->area.width, e->area.height);*/
        CRASSERT(context);
        crHashtableWalk(context->pGLXPixmapsHash, checkdamageCB, e);
    }
    return False;
}
#endif

/** @todo check what error codes could we throw for failures here*/
DECLEXPORT(void) VBOXGLXTAG(glXBindTexImageEXT)(Display *dpy, GLXDrawable draw, int buffer, const int *attrib_list)
{
    ContextInfo *context = stubGetCurrentContext();
    GLX_Pixmap_t *pGlxPixmap;
    RT_NOREF(buffer, attrib_list);

    if (!context)
    {
        crWarning("glXBindTexImageEXT called without current context");
        return;
    }

    pGlxPixmap = (GLX_Pixmap_t *) crHashtableSearch(context->pGLXPixmapsHash, (unsigned int) draw);
    if (!pGlxPixmap)
    {
        pGlxPixmap = (GLX_Pixmap_t *) crHashtableSearch(stub.pGLXPixmapsHash, (unsigned int) draw);
        if (!pGlxPixmap)
        {
            crDebug("Unknown drawable 0x%x in glXBindTexImageEXT!", (unsigned int) draw);
            return;
        }
        pGlxPixmap = stubInitGlxPixmap(pGlxPixmap, dpy, draw, context);
        if (!pGlxPixmap)
        {
            crDebug("glXBindTexImageEXT failed to get pGlxPixmap");
            return;
        }
    }

    /* If there's damage extension, then process incoming events as we need the information right now */
    if (!context->damageQueryFailed)
    {
        /* Sync connection */
        XLOCK(dpy);
        XSync(dpy, False);
        XUNLOCK(dpy);

        stubFetchDamageOnDrawable(dpy, pGlxPixmap);
    }

    /* No shared memory? Rollback to use slow x protocol then */
    if (stub.xshmSI.shmid<0)
    {
        /** @todo add damage support here too*/
        XImage *pxim;
        CRPixelPackState unpackState;

        XLOCK(dpy);
        pxim = XGetImage(dpy, (Pixmap)draw, pGlxPixmap->x, pGlxPixmap->y, pGlxPixmap->w, pGlxPixmap->h, AllPlanes, ZPixmap);
        XUNLOCK(dpy);
        /*if (pxim)
        {
            if (!ptextable)
            {
                ptextable = crAllocHashtable();
            }
            pm = crHashtableSearch(ptextable, (unsigned int) draw);
            if (!pm)
            {
                pm = crCalloc(sizeof(pminfo));
                crHashtableAdd(ptextable, (unsigned int) draw, pm);
            }
            pm->w = w;
            pm->h = h;
            if (pm->data) crFree(pm->data);
            pm->data = crAlloc(4*w*h);
            crMemcpy(pm->data, (void*)(&(pxim->data[0])), 4*w*h);
        }*/

        if (NULL==pxim)
        {
            crWarning("Failed, to get pixmap data for 0x%x", (unsigned int) draw);
            return;
        }

        stubGetUnpackState(&unpackState);
        stubSetUnpackState(&defaultPacking);
        stub.spu->dispatch_table.TexImage2D(pGlxPixmap->target, 0, pGlxPixmap->format, pxim->width, pxim->height, 0,
                                            GL_BGRA, GL_UNSIGNED_BYTE, (void*)(&(pxim->data[0])));
        stubSetUnpackState(&unpackState);
        XDestroyImage(pxim);
    }
    else /* Use shm to get pixmap data */
    {
        /* Check if we have damage extension */
        if (!context->damageQueryFailed)
        {
            if (pGlxPixmap->bPixmapImageDirty)
            {
                /* Either we failed to allocate damage region or this pixmap is marked for full update */
                if (!pGlxPixmap->pDamageRegion || XEmptyRegion(pGlxPixmap->pDamageRegion))
                {
                    /*crDebug("**FULL** update for 0x%x", (unsigned int)draw);*/
                    stubXshmUpdateWholeImage(dpy, draw, pGlxPixmap);
                }
                else
                {
                    long fullArea, damageArea=0, clipdamageArea, i;
                    XRectangle damageClipBox;

                    fullArea = pGlxPixmap->w * pGlxPixmap->h;
                    XClipBox(pGlxPixmap->pDamageRegion, &damageClipBox);
                    clipdamageArea = damageClipBox.width * damageClipBox.height;

                    //crDebug("FullSize [%i,%i,%i,%i]", pGlxPixmap->x, pGlxPixmap->y, pGlxPixmap->w, pGlxPixmap->h);
                    //crDebug("Clip [%i,%i,%i,%i]", damageClipBox.x, damageClipBox.y, damageClipBox.width, damageClipBox.height);

                    for (i=0; i<pGlxPixmap->pDamageRegion->numRects; ++i)
                    {
                        BoxPtr pBox = &pGlxPixmap->pDamageRegion->rects[i];
                        damageArea += (pBox->x2-pBox->x1)*(pBox->y2-pBox->y1);
                        //crDebug("Damage rect [%i,%i,%i,%i]", pBox->x1, pBox->y1, pBox->x2, pBox->y2);
                    }

                    if (damageArea>clipdamageArea || clipdamageArea>fullArea)
                    {
                        crWarning("glXBindTexImageEXT, damage regions seems to be broken, forcing full update");
                        /*crDebug("**FULL** update for 0x%x, numRect=%li, *FS*=%li, CS=%li, DS=%li",
                                (unsigned int)draw, pGlxPixmap->pDamageRegion->numRects, fullArea, clipdamageArea, damageArea);*/
                        stubXshmUpdateWholeImage(dpy, draw, pGlxPixmap);
                    }
                    else /*We have corect damage info*/
                    {
                        if (CR_MIN_DAMAGE_PROFIT_SIZE > (fullArea-damageArea))
                        {
                            /*crDebug("**FULL** update for 0x%x, numRect=%li, *FS*=%li, CS=%li, DS=%li",
                                    (unsigned int)draw, pGlxPixmap->pDamageRegion->numRects, fullArea, clipdamageArea, damageArea);*/
                            stubXshmUpdateWholeImage(dpy, draw, pGlxPixmap);
                        }
                        else if (CR_MIN_DAMAGE_PROFIT_SIZE > (clipdamageArea-damageArea))
                        {
                            /*crDebug("**PARTIAL** update for 0x%x, numRect=%li, FS=%li, *CS*=%li, DS=%li",
                                    (unsigned int)draw, pGlxPixmap->pDamageRegion->numRects, fullArea, clipdamageArea, damageArea);*/
                            stubXshmUpdateImageRect(dpy, draw, pGlxPixmap, &damageClipBox);
                        }
                        else
                        {
                            /*crDebug("**PARTIAL** update for 0x%x, numRect=*%li*, FS=%li, CS=%li, *DS*=%li",
                                    (unsigned int)draw, pGlxPixmap->pDamageRegion->numRects, fullArea, clipdamageArea, damageArea);*/
                            for (i=0; i<pGlxPixmap->pDamageRegion->numRects; ++i)
                            {
                                XRectangle rect;
                                BoxPtr pBox = &pGlxPixmap->pDamageRegion->rects[i];

                                rect.x = pBox->x1;
                                rect.y = pBox->y1;
                                rect.width = pBox->x2-pBox->x1;
                                rect.height = pBox->y2-pBox->y1;

                                stubXshmUpdateImageRect(dpy, draw, pGlxPixmap, &rect);
                            }
                        }
                    }
                }

                /* Clean dirty flag and damage region */
                pGlxPixmap->bPixmapImageDirty = False;
                if (pGlxPixmap->pDamageRegion)
                    EMPTY_REGION(pGlxPixmap->pDamageRegion);
            }
        }
        else
        {
            stubXshmUpdateWholeImage(dpy, draw, pGlxPixmap);
        }
    }
}

DECLEXPORT(void) VBOXGLXTAG(glXReleaseTexImageEXT)(Display *dpy, GLXDrawable draw, int buffer)
{
    (void) dpy;
    (void) draw;
    (void) buffer;
    //crDebug("glXReleaseTexImageEXT 0x%x", (unsigned int)draw);
}
#endif

#endif /* GLX_EXTRAS */


#ifdef GLX_SGIX_video_resize
/* more dummy funcs.  These help when linking with older GLUTs */

DECLEXPORT(int) VBOXGLXTAG(glXBindChannelToWindowSGIX)(Display *dpy, int scrn, int chan, Window w)
{
    (void) dpy;
    (void) scrn;
    (void) chan;
    (void) w;
    crDebug("glXBindChannelToWindowSGIX");
    return 0;
}

DECLEXPORT(int) VBOXGLXTAG(glXChannelRectSGIX)(Display *dpy, int scrn, int chan, int x , int y, int w, int h)
{
    (void) dpy;
    (void) scrn;
    (void) chan;
    (void) x;
    (void) y;
    (void) w;
    (void) h;
    crDebug("glXChannelRectSGIX");
    return 0;
}

DECLEXPORT(int) VBOXGLXTAG(glXQueryChannelRectSGIX)(Display *dpy, int scrn, int chan, int *x, int *y, int *w, int *h)
{
    (void) dpy;
    (void) scrn;
    (void) chan;
    (void) x;
    (void) y;
    (void) w;
    (void) h;
    crDebug("glXQueryChannelRectSGIX");
    return 0;
}

DECLEXPORT(int) VBOXGLXTAG(glXQueryChannelDeltasSGIX)(Display *dpy, int scrn, int chan, int *dx, int *dy, int *dw, int *dh)
{
    (void) dpy;
    (void) scrn;
    (void) chan;
    (void) dx;
    (void) dy;
    (void) dw;
    (void) dh;
    crDebug("glXQueryChannelDeltasSGIX");
    return 0;
}

DECLEXPORT(int) VBOXGLXTAG(glXChannelRectSyncSGIX)(Display *dpy, int scrn, int chan, GLenum synctype)
{
    (void) dpy;
    (void) scrn;
    (void) chan;
    (void) synctype;
    crDebug("glXChannelRectSyncSGIX");
    return 0;
}

#endif /* GLX_SGIX_video_resize */

#ifdef VBOXOGL_FAKEDRI
DECLEXPORT(const char *) VBOXGLXTAG(glXGetDriverConfig)(const char *driverName)
{
    return NULL;
}

DECLEXPORT(void) VBOXGLXTAG(glXFreeMemoryMESA)(Display *dpy, int scrn, void *pointer)
{
    (void) dpy;
    (void) scrn;
    (void) pointer;
}

DECLEXPORT(GLXContext) VBOXGLXTAG(glXImportContextEXT)(Display *dpy, GLXContextID contextID)
{
    (void) dpy;
    (void) contextID;
    return NULL;
}

DECLEXPORT(GLXContextID) VBOXGLXTAG(glXGetContextIDEXT)(const GLXContext ctx)
{
    (void) ctx;
    return 0;
}

DECLEXPORT(Bool) VBOXGLXTAG(glXMakeCurrentReadSGI)(Display *display, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
    return VBOXGLXTAG(glXMakeContextCurrent)(display, draw, read, ctx);
}

DECLEXPORT(const char *) VBOXGLXTAG(glXGetScreenDriver)(Display *dpy, int scrNum)
{
    static char *screendriver = "vboxvideo";
    return screendriver;
}

DECLEXPORT(Display *) VBOXGLXTAG(glXGetCurrentDisplayEXT)(void)
{
    return VBOXGLXTAG(glXGetCurrentDisplay());
}

DECLEXPORT(void) VBOXGLXTAG(glXFreeContextEXT)(Display *dpy, GLXContext ctx)
{
    VBOXGLXTAG(glXDestroyContext(dpy, ctx));
}

/*Mesa internal*/
DECLEXPORT(int) VBOXGLXTAG(glXQueryContextInfoEXT)(Display *dpy, GLXContext ctx)
{
    (void) dpy;
    (void) ctx;
    return 0;
}

DECLEXPORT(void *) VBOXGLXTAG(glXAllocateMemoryMESA)(Display *dpy, int scrn,
                                                     size_t size, float readFreq,
                                                     float writeFreq, float priority)
{
    (void) dpy;
    (void) scrn;
    (void) size;
    (void) readFreq;
    (void) writeFreq;
    (void) priority;
    return NULL;
}

DECLEXPORT(GLuint) VBOXGLXTAG(glXGetMemoryOffsetMESA)(Display *dpy, int scrn, const void *pointer)
{
    (void) dpy;
    (void) scrn;
    (void) pointer;
    return 0;
}

DECLEXPORT(GLXPixmap) VBOXGLXTAG(glXCreateGLXPixmapMESA)(Display *dpy, XVisualInfo *visual, Pixmap pixmap, Colormap cmap)
{
    (void) dpy;
    (void) visual;
    (void) pixmap;
    (void) cmap;
    return 0;
}

#endif /*VBOXOGL_FAKEDRI*/
