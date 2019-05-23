/* $Id: glx_c_exports.c $ */
/** @file
 * VirtualBox guest OpenGL DRI GLX C stubs
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

#include "stub.h"
#include "dri_glx.h"
#include "fakedri_drv.h"


#ifdef VBOXOGL_FAKEDRI
/*DECLEXPORT(void) VBOXGLXENTRYTAG(glXGetDriverConfig)(const char *driverName)
{
    return glxim.GetDriverConfig(driverName);
}*/

DECLEXPORT(void) VBOXGLXENTRYTAG(glXFreeMemoryMESA)(Display *dpy, int scrn, void *pointer)
{
    return glxim.FreeMemoryMESA(dpy, scrn, pointer);
}

DECLEXPORT(GLXContext) VBOXGLXENTRYTAG(glXImportContextEXT)(Display *dpy, GLXContextID contextID)
{
    return glxim.ImportContextEXT(dpy, contextID);
}

DECLEXPORT(GLXContextID) VBOXGLXENTRYTAG(glXGetContextIDEXT)(const GLXContext ctx)
{
    return glxim.GetContextIDEXT(ctx);
}

DECLEXPORT(Bool) VBOXGLXENTRYTAG(glXMakeCurrentReadSGI)(Display *display, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
    return glxim.MakeCurrentReadSGI(display, draw, read, ctx);
}


/*const char * VBOXGLXENTRYTAG(glXGetScreenDriver)(Display *dpy, int scrNum)
{
    return glxim.GetScreenDriver(dpy, scrNum);
}*/


DECLEXPORT(Display*) VBOXGLXENTRYTAG(glXGetCurrentDisplayEXT)(void)
{
    return glxim.GetCurrentDisplayEXT();
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXFreeContextEXT)(Display *dpy, GLXContext ctx)
{
    return glxim.FreeContextEXT(dpy, ctx);
}

/*Mesa internal*/
DECLEXPORT(int) VBOXGLXENTRYTAG(glXQueryContextInfoEXT)(Display *dpy, GLXContext ctx)
{
    return glxim.QueryContextInfoEXT(dpy, ctx);
}

DECLEXPORT(void *) VBOXGLXENTRYTAG(glXAllocateMemoryMESA)(Display *dpy, int scrn,
                                                       size_t size, float readFreq,
                                                       float writeFreq, float priority)
{
    return glxim.AllocateMemoryMESA(dpy, scrn, size, readFreq, writeFreq, priority);
}

DECLEXPORT(GLuint) VBOXGLXENTRYTAG(glXGetMemoryOffsetMESA)(Display *dpy, int scrn, const void *pointer )
{
    return glxim.GetMemoryOffsetMESA(dpy, scrn, pointer);
}


DECLEXPORT(GLXPixmap) VBOXGLXENTRYTAG(glXCreateGLXPixmapMESA)(Display *dpy, XVisualInfo *visual, Pixmap pixmap, Colormap cmap)
{
    return glxim.CreateGLXPixmapMESA(dpy, visual, pixmap, cmap);
}
#endif

/*Common glX functions*/
DECLEXPORT(void) VBOXGLXENTRYTAG(glXCopyContext)( Display *dpy, GLXContext src, GLXContext dst, unsigned long mask)
{
    return glxim.CopyContext(dpy, src, dst, mask);
}


DECLEXPORT(void) VBOXGLXENTRYTAG(glXUseXFont)(Font font, int first, int count, int listBase)
{
    return glxim.UseXFont(font, first, count, listBase);
}

DECLEXPORT(CR_GLXFuncPtr) VBOXGLXENTRYTAG(glXGetProcAddress)(const GLubyte *name)
{
    return glxim.GetProcAddress(name);
}

DECLEXPORT(Bool) VBOXGLXENTRYTAG(glXQueryExtension)(Display *dpy, int *errorBase, int *eventBase)
{
    return glxim.QueryExtension(dpy, errorBase, eventBase);
}

DECLEXPORT(Bool) VBOXGLXENTRYTAG(glXIsDirect)(Display *dpy, GLXContext ctx)
{
    return glxim.IsDirect(dpy, ctx);
}

DECLEXPORT(GLXPixmap) VBOXGLXENTRYTAG(glXCreateGLXPixmap)(Display *dpy, XVisualInfo *vis, Pixmap pixmap)
{
    return glxim.CreateGLXPixmap(dpy, vis, pixmap);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXSwapBuffers)(Display *dpy, GLXDrawable drawable)
{
    return glxim.SwapBuffers(dpy, drawable);
}


DECLEXPORT(GLXDrawable) VBOXGLXENTRYTAG(glXGetCurrentDrawable)(void)
{
    return glxim.GetCurrentDrawable();
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXWaitGL)(void)
{
    return glxim.WaitGL();
}

DECLEXPORT(Display *) VBOXGLXENTRYTAG(glXGetCurrentDisplay)(void)
{
    return glxim.GetCurrentDisplay();
}

DECLEXPORT(const char *) VBOXGLXENTRYTAG(glXQueryServerString)(Display *dpy, int screen, int name)
{
    return glxim.QueryServerString(dpy, screen, name);
}

DECLEXPORT(GLXContext) VBOXGLXENTRYTAG(glXCreateContext)(Display *dpy, XVisualInfo *vis, GLXContext share, Bool direct)
{
    return glxim.CreateContext(dpy, vis, share, direct);
}

DECLEXPORT(int) VBOXGLXENTRYTAG(glXGetConfig)(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
    return glxim.GetConfig(dpy, vis, attrib, value);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXWaitX)(void)
{
    return glxim.WaitX();
}

DECLEXPORT(GLXContext) VBOXGLXENTRYTAG(glXGetCurrentContext)(void)
{
    return glxim.GetCurrentContext();
}

DECLEXPORT(const char *) VBOXGLXENTRYTAG(glXGetClientString)(Display *dpy, int name)
{
    return glxim.GetClientString(dpy, name);
}

DECLEXPORT(Bool) VBOXGLXENTRYTAG(glXMakeCurrent)(Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
    return glxim.MakeCurrent(dpy, drawable, ctx);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXDestroyContext)(Display *dpy, GLXContext ctx)
{
    return glxim.DestroyContext(dpy, ctx);
}

DECLEXPORT(CR_GLXFuncPtr) VBOXGLXENTRYTAG(glXGetProcAddressARB)(const GLubyte *name)
{
    return glxim.GetProcAddressARB(name);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXDestroyGLXPixmap)(Display *dpy, GLXPixmap pix)
{
    return glxim.DestroyGLXPixmap(dpy, pix);
}

DECLEXPORT(Bool) VBOXGLXENTRYTAG(glXQueryVersion)(Display *dpy, int *major, int *minor)
{
    return glxim.QueryVersion(dpy, major, minor);
}

DECLEXPORT(XVisualInfo *) VBOXGLXENTRYTAG(glXChooseVisual)(Display *dpy, int screen, int *attribList)
{
    return glxim.ChooseVisual(dpy, screen, attribList);
}

DECLEXPORT(const char *) VBOXGLXENTRYTAG(glXQueryExtensionsString)(Display *dpy, int screen)
{
    return glxim.QueryExtensionsString(dpy, screen);
}

#if GLX_EXTRAS
DECLEXPORT(GLXPbufferSGIX) VBOXGLXENTRYTAG(glXCreateGLXPbufferSGIX)
(Display *dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height, int *attrib_list)
{
    return glxim.CreateGLXPbufferSGIX(dpy, config, width, height, attrib_list);
}

DECLEXPORT(int) VBOXGLXENTRYTAG(glXQueryGLXPbufferSGIX)
(Display *dpy, GLXPbuffer pbuf, int attribute, unsigned int *value)
{
    return glxim.QueryGLXPbufferSGIX(dpy, pbuf, attribute, value);
}

DECLEXPORT(GLXFBConfigSGIX *) VBOXGLXENTRYTAG(glXChooseFBConfigSGIX)
(Display *dpy, int screen, int *attrib_list, int *nelements)
{
    return glxim.ChooseFBConfigSGIX(dpy, screen, attrib_list, nelements);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXDestroyGLXPbufferSGIX)(Display *dpy, GLXPbuffer pbuf)
{
    return glxim.DestroyGLXPbufferSGIX(dpy, pbuf);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXSelectEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
    return glxim.SelectEventSGIX(dpy, drawable, mask);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXGetSelectedEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
    return glxim.GetSelectedEventSGIX(dpy, drawable, mask);
}

DECLEXPORT(GLXFBConfigSGIX) VBOXGLXENTRYTAG(glXGetFBConfigFromVisualSGIX)(Display *dpy, XVisualInfo *vis)
{
    return glxim.GetFBConfigFromVisualSGIX(dpy, vis);
}

DECLEXPORT(XVisualInfo *) VBOXGLXENTRYTAG(glXGetVisualFromFBConfigSGIX)(Display *dpy, GLXFBConfig config)
{
    return glxim.GetVisualFromFBConfigSGIX(dpy, config);
}

DECLEXPORT(GLXContext) VBOXGLXENTRYTAG(glXCreateContextWithConfigSGIX)
(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct)
{
    return glxim.CreateContextWithConfigSGIX(dpy, config, render_type, share_list, direct);
}

DECLEXPORT(GLXPixmap) VBOXGLXENTRYTAG(glXCreateGLXPixmapWithConfigSGIX)(Display *dpy, GLXFBConfig config, Pixmap pixmap)
{
    return glxim.CreateGLXPixmapWithConfigSGIX(dpy, config, pixmap);
}

DECLEXPORT(int) VBOXGLXENTRYTAG(glXGetFBConfigAttribSGIX)(Display *dpy, GLXFBConfig config, int attribute, int *value)
{
    return glxim.GetFBConfigAttribSGIX(dpy, config, attribute, value);
}


/*
 * GLX 1.3 functions
 */
DECLEXPORT(GLXFBConfig *) VBOXGLXENTRYTAG(glXChooseFBConfig)(Display *dpy, int screen, ATTRIB_TYPE *attrib_list, int *nelements)
{
    return glxim.ChooseFBConfig(dpy, screen, attrib_list, nelements);
}

DECLEXPORT(GLXPbuffer) VBOXGLXENTRYTAG(glXCreatePbuffer)(Display *dpy, GLXFBConfig config, ATTRIB_TYPE *attrib_list)
{
    return glxim.CreatePbuffer(dpy, config, attrib_list);
}

DECLEXPORT(GLXPixmap) VBOXGLXENTRYTAG(glXCreatePixmap)(Display *dpy, GLXFBConfig config, Pixmap pixmap, const ATTRIB_TYPE *attrib_list)
{
    return glxim.CreatePixmap(dpy, config, pixmap, attrib_list);
}

DECLEXPORT(GLXWindow) VBOXGLXENTRYTAG(glXCreateWindow)(Display *dpy, GLXFBConfig config, Window win, ATTRIB_TYPE *attrib_list)
{
    return glxim.CreateWindow(dpy, config, win, attrib_list);
}


DECLEXPORT(GLXContext) VBOXGLXENTRYTAG(glXCreateNewContext)
(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct)
{
    return glxim.CreateNewContext(dpy, config, render_type, share_list, direct);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXDestroyPbuffer)(Display *dpy, GLXPbuffer pbuf)
{
    return glxim.DestroyPbuffer(dpy, pbuf);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXDestroyPixmap)(Display *dpy, GLXPixmap pixmap)
{
    return glxim.DestroyPixmap(dpy, pixmap);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXDestroyWindow)(Display *dpy, GLXWindow win)
{
    return glxim.DestroyWindow(dpy, win);
}

DECLEXPORT(GLXDrawable) VBOXGLXENTRYTAG(glXGetCurrentReadDrawable)(void)
{
    return glxim.GetCurrentReadDrawable();
}

DECLEXPORT(int) VBOXGLXENTRYTAG(glXGetFBConfigAttrib)(Display *dpy, GLXFBConfig config, int attribute, int *value)
{
    return glxim.GetFBConfigAttrib(dpy, config, attribute, value);
}

DECLEXPORT(GLXFBConfig *) VBOXGLXENTRYTAG(glXGetFBConfigs)(Display *dpy, int screen, int *nelements)
{
    return glxim.GetFBConfigs(dpy, screen, nelements);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXGetSelectedEvent)(Display *dpy, GLXDrawable draw, unsigned long *event_mask)
{
    return glxim.GetSelectedEvent(dpy, draw, event_mask);
}

DECLEXPORT(XVisualInfo *) VBOXGLXENTRYTAG(glXGetVisualFromFBConfig)(Display *dpy, GLXFBConfig config)
{
    return glxim.GetVisualFromFBConfig(dpy, config);
}

DECLEXPORT(Bool) VBOXGLXENTRYTAG(glXMakeContextCurrent)(Display *display, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
    return glxim.MakeContextCurrent(display, draw, read, ctx);
}

DECLEXPORT(int) VBOXGLXENTRYTAG(glXQueryContext)(Display *dpy, GLXContext ctx, int attribute, int *value)
{
    return glxim.QueryContext(dpy, ctx, attribute, value);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXQueryDrawable)(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value)
{
    return glxim.QueryDrawable(dpy, draw, attribute, value);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXSelectEvent)(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
    return glxim.SelectEvent(dpy, draw, event_mask);
}

/*
#ifdef CR_EXT_texture_from_pixmap
DECLEXPORT(void) VBOXGLXENTRYTAG(glXBindTexImageEXT)(Display *dpy, GLXDrawable draw, int buffer, const int *attrib_list)
{
    return glxim.BindTexImageEXT(dpy, draw, buffer, attrib_list);
}

DECLEXPORT(void) VBOXGLXENTRYTAG(glXReleaseTexImageEXT)(Display *dpy, GLXDrawable draw, int buffer)
{
    return glxim.ReleaseTexImageEXT(dpy, draw, buffer);
}
#endif
*/

#endif /* GLX_EXTRAS */

