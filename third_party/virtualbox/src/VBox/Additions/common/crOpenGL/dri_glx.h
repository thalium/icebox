/* $Id: dri_glx.h $ */

/** @file
 *
 * VirtualBox guest OpenGL DRI GLX header
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
 */

#ifndef ___CROPENGL_DRI_GLX_H
#define ___CROPENGL_DRI_GLX_H

#include "chromium.h"
#include "stub.h"

#if defined(VBOXOGL_FAKEDRI) || defined(VBOXOGL_DRI)
 #define VBOXGLXTAG(Func) vboxstub_##Func
 #define VBOXGLXENTRYTAG(Func) vbox_##Func
 #define VBOXGLTAG(Func) cr_##Func
#else
 #define VBOXGLXTAG(Func) Func
 #define VBOXGLXENTRYTAG(Func) Func
 #define VBOXGLTAG(Func) Func
#endif

#ifdef VBOXOGL_FAKEDRI
extern DECLEXPORT(const char *) VBOXGLXTAG(glXGetDriverConfig)(const char *driverName);
extern DECLEXPORT(void) VBOXGLXTAG(glXFreeMemoryMESA)(Display *dpy, int scrn, void *pointer);
extern DECLEXPORT(GLXContext) VBOXGLXTAG(glXImportContextEXT)(Display *dpy, GLXContextID contextID);
extern DECLEXPORT(GLXContextID) VBOXGLXTAG(glXGetContextIDEXT)(const GLXContext ctx);
extern DECLEXPORT(Bool) VBOXGLXTAG(glXMakeCurrentReadSGI)(Display *display, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
extern DECLEXPORT(const char *) VBOXGLXTAG(glXGetScreenDriver)(Display *dpy, int scrNum);
extern DECLEXPORT(Display *) VBOXGLXTAG(glXGetCurrentDisplayEXT)(void);
extern DECLEXPORT(void) VBOXGLXTAG(glXFreeContextEXT)(Display *dpy, GLXContext ctx);
/*Mesa internal*/
extern DECLEXPORT(int) VBOXGLXTAG(glXQueryContextInfoEXT)(Display *dpy, GLXContext ctx);
extern DECLEXPORT(void *) VBOXGLXTAG(glXAllocateMemoryMESA)(Display *dpy, int scrn,
                                                       size_t size, float readFreq,
                                                       float writeFreq, float priority);
extern DECLEXPORT(GLuint) VBOXGLXTAG(glXGetMemoryOffsetMESA)(Display *dpy, int scrn, const void *pointer );
extern DECLEXPORT(GLXPixmap) VBOXGLXTAG(glXCreateGLXPixmapMESA)(Display *dpy, XVisualInfo *visual, Pixmap pixmap, Colormap cmap);
#endif

/*Common glX functions*/
extern DECLEXPORT(void) VBOXGLXTAG(glXCopyContext)( Display *dpy, GLXContext src, GLXContext dst,
#if defined(SunOS)
unsigned long mask);
#else
unsigned long mask);
#endif
extern DECLEXPORT(void) VBOXGLXTAG(glXUseXFont)(Font font, int first, int count, int listBase);
extern DECLEXPORT(CR_GLXFuncPtr) VBOXGLXTAG(glXGetProcAddress)(const GLubyte *name);
extern DECLEXPORT(Bool) VBOXGLXTAG(glXQueryExtension)(Display *dpy, int *errorBase, int *eventBase);
extern DECLEXPORT(Bool) VBOXGLXTAG(glXIsDirect)(Display *dpy, GLXContext ctx);
extern DECLEXPORT(GLXPixmap) VBOXGLXTAG(glXCreateGLXPixmap)(Display *dpy, XVisualInfo *vis, Pixmap pixmap);
extern DECLEXPORT(void) VBOXGLXTAG(glXSwapBuffers)(Display *dpy, GLXDrawable drawable);
extern DECLEXPORT(GLXDrawable) VBOXGLXTAG(glXGetCurrentDrawable)(void);
extern DECLEXPORT(void) VBOXGLXTAG(glXWaitGL)(void);
extern DECLEXPORT(Display *) VBOXGLXTAG(glXGetCurrentDisplay)(void);
extern DECLEXPORT(const char *) VBOXGLXTAG(glXQueryServerString)(Display *dpy, int screen, int name);
extern DECLEXPORT(GLXContext) VBOXGLXTAG(glXCreateContext)(Display *dpy, XVisualInfo *vis, GLXContext share, Bool direct);
extern DECLEXPORT(int) VBOXGLXTAG(glXGetConfig)(Display *dpy, XVisualInfo *vis, int attrib, int *value);
extern DECLEXPORT(void) VBOXGLXTAG(glXWaitX)(void);
extern DECLEXPORT(GLXContext) VBOXGLXTAG(glXGetCurrentContext)(void);
extern DECLEXPORT(const char *) VBOXGLXTAG(glXGetClientString)(Display *dpy, int name);
extern DECLEXPORT(Bool) VBOXGLXTAG(glXMakeCurrent)(Display *dpy, GLXDrawable drawable, GLXContext ctx);
extern DECLEXPORT(void) VBOXGLXTAG(glXDestroyContext)(Display *dpy, GLXContext ctx);
extern DECLEXPORT(CR_GLXFuncPtr) VBOXGLXTAG(glXGetProcAddressARB)(const GLubyte *name);
extern DECLEXPORT(void) VBOXGLXTAG(glXDestroyGLXPixmap)(Display *dpy, GLXPixmap pix);
extern DECLEXPORT(Bool) VBOXGLXTAG(glXQueryVersion)(Display *dpy, int *major, int *minor);
extern DECLEXPORT(XVisualInfo *) VBOXGLXTAG(glXChooseVisual)(Display *dpy, int screen, int *attribList);
extern DECLEXPORT(const char *) VBOXGLXTAG(glXQueryExtensionsString)(Display *dpy, int screen);

/**
 * Set this to 1 if you want to build stub functions for the
 * GL_SGIX_pbuffer and GLX_SGIX_fbconfig extensions.
 * This used to be disabled, due to "messy compilation issues",
 * according to the earlier comment; but they're needed just
 * to resolve symbols for OpenInventor applications, and I
 * haven't found any reference to exactly what the "messy compilation
 * issues" are, so I'm re-enabling the code by default.
 */
#define GLX_EXTRAS 1

#define GLX_SGIX_video_resize 1

/**
 * Prototypes, in case they're not in glx.h or glxext.h
 * Unfortunately, there's some inconsistency between the extension
 * specs, and the SGI, NVIDIA, XFree86 and common glxext.h header
 * files.
 */
#if defined(GLX_GLXEXT_VERSION)
/* match glxext.h, XFree86, Mesa */
#define ATTRIB_TYPE const int
#else
#define ATTRIB_TYPE int
#endif

#if GLX_EXTRAS
extern DECLEXPORT(GLXPbufferSGIX) VBOXGLXTAG(glXCreateGLXPbufferSGIX)
(Display *dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height, int *attrib_list);

extern DECLEXPORT(int) VBOXGLXTAG(glXQueryGLXPbufferSGIX)
(Display *dpy, GLXPbuffer pbuf, int attribute, unsigned int *value);

extern DECLEXPORT(GLXFBConfigSGIX *) VBOXGLXTAG(glXChooseFBConfigSGIX)
(Display *dpy, int screen, int *attrib_list, int *nelements);

extern DECLEXPORT(void) VBOXGLXTAG(glXDestroyGLXPbufferSGIX)(Display *dpy, GLXPbuffer pbuf);
extern DECLEXPORT(void) VBOXGLXTAG(glXSelectEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long mask);
extern DECLEXPORT(void) VBOXGLXTAG(glXGetSelectedEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long *mask);

extern DECLEXPORT(GLXFBConfigSGIX) VBOXGLXTAG(glXGetFBConfigFromVisualSGIX)(Display *dpy, XVisualInfo *vis);
extern DECLEXPORT(XVisualInfo *) VBOXGLXTAG(glXGetVisualFromFBConfigSGIX)(Display *dpy, GLXFBConfig config);
extern DECLEXPORT(GLXContext) VBOXGLXTAG(glXCreateContextWithConfigSGIX)
(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);

extern DECLEXPORT(GLXPixmap) VBOXGLXTAG(glXCreateGLXPixmapWithConfigSGIX)(Display *dpy, GLXFBConfig config, Pixmap pixmap);
extern DECLEXPORT(int) VBOXGLXTAG(glXGetFBConfigAttribSGIX)(Display *dpy, GLXFBConfig config, int attribute, int *value);

/*
 * GLX 1.3 functions
 */
extern DECLEXPORT(GLXFBConfig *) VBOXGLXTAG(glXChooseFBConfig)(Display *dpy, int screen, ATTRIB_TYPE *attrib_list, int *nelements);
extern DECLEXPORT(GLXPbuffer) VBOXGLXTAG(glXCreatePbuffer)(Display *dpy, GLXFBConfig config, ATTRIB_TYPE *attrib_list);
extern DECLEXPORT(GLXPixmap) VBOXGLXTAG(glXCreatePixmap)(Display *dpy, GLXFBConfig config, Pixmap pixmap, ATTRIB_TYPE *attrib_list);
extern DECLEXPORT(GLXWindow) VBOXGLXTAG(glXCreateWindow)(Display *dpy, GLXFBConfig config, Window win, ATTRIB_TYPE *attrib_list);
extern DECLEXPORT(GLXContext) VBOXGLXTAG(glXCreateNewContext)
(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);

extern DECLEXPORT(void) VBOXGLXTAG(glXDestroyPbuffer)(Display *dpy, GLXPbuffer pbuf);
extern DECLEXPORT(void) VBOXGLXTAG(glXDestroyPixmap)(Display *dpy, GLXPixmap pixmap);
extern DECLEXPORT(void) VBOXGLXTAG(glXDestroyWindow)(Display *dpy, GLXWindow win);
extern DECLEXPORT(GLXDrawable) VBOXGLXTAG(glXGetCurrentReadDrawable)(void);
extern DECLEXPORT(int) VBOXGLXTAG(glXGetFBConfigAttrib)(Display *dpy, GLXFBConfig config, int attribute, int *value);
extern DECLEXPORT(GLXFBConfig *) VBOXGLXTAG(glXGetFBConfigs)(Display *dpy, int screen, int *nelements);
extern DECLEXPORT(void) VBOXGLXTAG(glXGetSelectedEvent)(Display *dpy, GLXDrawable draw, unsigned long *event_mask);
extern DECLEXPORT(XVisualInfo *) VBOXGLXTAG(glXGetVisualFromFBConfig)(Display *dpy, GLXFBConfig config);
extern DECLEXPORT(Bool) VBOXGLXTAG(glXMakeContextCurrent)(Display *display, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
extern DECLEXPORT(int) VBOXGLXTAG(glXQueryContext)(Display *dpy, GLXContext ctx, int attribute, int *value);
extern DECLEXPORT(void) VBOXGLXTAG(glXQueryDrawable)(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value);
extern DECLEXPORT(void) VBOXGLXTAG(glXSelectEvent)(Display *dpy, GLXDrawable draw, unsigned long event_mask);

#ifdef CR_EXT_texture_from_pixmap
extern DECLEXPORT(void) VBOXGLXTAG(glXBindTexImageEXT)(Display *dpy, GLXDrawable draw, int buffer, const int *attrib_list);
extern DECLEXPORT(void) VBOXGLXTAG(glXReleaseTexImageEXT)(Display *dpy, GLXDrawable draw, int buffer);
#endif

#endif /* GLX_EXTRAS */

#endif //___CROPENGL_DRI_GLX_H
