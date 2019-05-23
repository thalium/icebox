/* $Id: glx_proto.h $ */
/** @file
 * VirtualBox guest OpenGL DRI GLX header C prototypes
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

#ifndef ___CROPENGL_GLX_PROTO_H
#define ___CROPENGL_GLX_PROTO_H

#include "chromium.h"
#include "stub.h"

#if defined(VBOXOGL_FAKEDRI) || defined(VBOXOGL_DRI)
typedef const char * (*PGLXFUNC_GetDriverConfig)(const char *driverName);
typedef void (*PGLXFUNC_FreeMemoryMESA)(Display *dpy, int scrn, void *pointer);
typedef GLXContext (*PGLXFUNC_ImportContextEXT)(Display *dpy, GLXContextID contextID);
typedef GLXContextID (*PGLXFUNC_GetContextIDEXT)(const GLXContext ctx);
typedef Bool (*PGLXFUNC_MakeCurrentReadSGI)(Display *display, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
typedef const char * (*PGLXFUNC_GetScreenDriver)(Display *dpy, int scrNum);
typedef Display * (*PGLXFUNC_GetCurrentDisplayEXT)(void);
typedef void (*PGLXFUNC_FreeContextEXT)(Display *dpy, GLXContext ctx);

/*Mesa internal*/
typedef int (*PGLXFUNC_QueryContextInfoEXT)(Display *dpy, GLXContext ctx);
typedef void * (*PGLXFUNC_AllocateMemoryMESA)(Display *dpy, int scrn,
                                                       size_t size, float readFreq,
                                                       float writeFreq, float priority);
typedef GLuint (*PGLXFUNC_GetMemoryOffsetMESA)(Display *dpy, int scrn, const void *pointer );
typedef GLXPixmap (*PGLXFUNC_CreateGLXPixmapMESA)(Display *dpy, XVisualInfo *visual, Pixmap pixmap, Colormap cmap);
#endif

/*Common glX functions*/
typedef void (*PGLXFUNC_CopyContext)(Display *dpy, GLXContext src, GLXContext dst,unsigned long mask);
typedef void (*PGLXFUNC_UseXFont)(Font font, int first, int count, int listBase);
typedef CR_GLXFuncPtr (*PGLXFUNC_GetProcAddress)(const GLubyte *name);
typedef Bool (*PGLXFUNC_QueryExtension)(Display *dpy, int *errorBase, int *eventBase);
typedef Bool (*PGLXFUNC_IsDirect)(Display *dpy, GLXContext ctx);
typedef GLXPixmap (*PGLXFUNC_CreateGLXPixmap)(Display *dpy, XVisualInfo *vis, Pixmap pixmap);
typedef void (*PGLXFUNC_SwapBuffers)(Display *dpy, GLXDrawable drawable);
typedef GLXDrawable (*PGLXFUNC_GetCurrentDrawable)(void);
typedef void (*PGLXFUNC_WaitGL)(void);
typedef Display * (*PGLXFUNC_GetCurrentDisplay)(void);
typedef const char * (*PGLXFUNC_QueryServerString)(Display *dpy, int screen, int name);
typedef GLXContext (*PGLXFUNC_CreateContext)(Display *dpy, XVisualInfo *vis, GLXContext share, Bool direct);
typedef int (*PGLXFUNC_GetConfig)(Display *dpy, XVisualInfo *vis, int attrib, int *value);
typedef void (*PGLXFUNC_WaitX)(void);
typedef GLXContext (*PGLXFUNC_GetCurrentContext)(void);
typedef const char * (*PGLXFUNC_GetClientString)(Display *dpy, int name);
typedef Bool (*PGLXFUNC_MakeCurrent)(Display *dpy, GLXDrawable drawable, GLXContext ctx);
typedef void (*PGLXFUNC_DestroyContext)(Display *dpy, GLXContext ctx);
typedef CR_GLXFuncPtr (*PGLXFUNC_GetProcAddressARB)(const GLubyte *name);
typedef void (*PGLXFUNC_DestroyGLXPixmap)(Display *dpy, GLXPixmap pix);
typedef Bool (*PGLXFUNC_QueryVersion)(Display *dpy, int *major, int *minor);
typedef XVisualInfo * (*PGLXFUNC_ChooseVisual)(Display *dpy, int screen, int *attribList);
typedef const char * (*PGLXFUNC_QueryExtensionsString)(Display *dpy, int screen);

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
typedef GLXPbufferSGIX (*PGLXFUNC_CreateGLXPbufferSGIX)
(Display *dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height, int *attrib_list);

typedef int (*PGLXFUNC_QueryGLXPbufferSGIX)
(Display *dpy, GLXPbuffer pbuf, int attribute, unsigned int *value);

typedef GLXFBConfigSGIX * (*PGLXFUNC_ChooseFBConfigSGIX)
(Display *dpy, int screen, int *attrib_list, int *nelements);

typedef void (*PGLXFUNC_DestroyGLXPbufferSGIX)(Display *dpy, GLXPbuffer pbuf);
typedef void (*PGLXFUNC_SelectEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long mask);
typedef void (*PGLXFUNC_GetSelectedEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long *mask);

typedef GLXFBConfigSGIX (*PGLXFUNC_GetFBConfigFromVisualSGIX)(Display *dpy, XVisualInfo *vis);
typedef XVisualInfo * (*PGLXFUNC_GetVisualFromFBConfigSGIX)(Display *dpy, GLXFBConfig config);
typedef GLXContext (*PGLXFUNC_CreateContextWithConfigSGIX)
(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);

typedef GLXPixmap (*PGLXFUNC_CreateGLXPixmapWithConfigSGIX)(Display *dpy, GLXFBConfig config, Pixmap pixmap);
typedef int (*PGLXFUNC_GetFBConfigAttribSGIX)(Display *dpy, GLXFBConfig config, int attribute, int *value);

/*
 * GLX 1.3 functions
 */
typedef GLXFBConfig * (*PGLXFUNC_ChooseFBConfig)(Display *dpy, int screen, ATTRIB_TYPE *attrib_list, int *nelements);
typedef GLXPbuffer (*PGLXFUNC_CreatePbuffer)(Display *dpy, GLXFBConfig config, ATTRIB_TYPE *attrib_list);
typedef GLXPixmap (*PGLXFUNC_CreatePixmap)(Display *dpy, GLXFBConfig config, Pixmap pixmap, const ATTRIB_TYPE *attrib_list);
typedef GLXWindow (*PGLXFUNC_CreateWindow)(Display *dpy, GLXFBConfig config, Window win, ATTRIB_TYPE *attrib_list);
typedef GLXContext (*PGLXFUNC_CreateNewContext)
(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);

typedef void (*PGLXFUNC_DestroyPbuffer)(Display *dpy, GLXPbuffer pbuf);
typedef void (*PGLXFUNC_DestroyPixmap)(Display *dpy, GLXPixmap pixmap);
typedef void (*PGLXFUNC_DestroyWindow)(Display *dpy, GLXWindow win);
typedef GLXDrawable (*PGLXFUNC_GetCurrentReadDrawable)(void);
typedef int (*PGLXFUNC_GetFBConfigAttrib)(Display *dpy, GLXFBConfig config, int attribute, int *value);
typedef GLXFBConfig * (*PGLXFUNC_GetFBConfigs)(Display *dpy, int screen, int *nelements);
typedef void (*PGLXFUNC_GetSelectedEvent)(Display *dpy, GLXDrawable draw, unsigned long *event_mask);
typedef XVisualInfo * (*PGLXFUNC_GetVisualFromFBConfig)(Display *dpy, GLXFBConfig config);
typedef Bool (*PGLXFUNC_MakeContextCurrent)(Display *display, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
typedef int (*PGLXFUNC_QueryContext)(Display *dpy, GLXContext ctx, int attribute, int *value);
typedef void (*PGLXFUNC_QueryDrawable)(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value);
typedef void (*PGLXFUNC_SelectEvent)(Display *dpy, GLXDrawable draw, unsigned long event_mask);

#ifdef CR_EXT_texture_from_pixmap
typedef void (*PGLXFUNC_BindTexImageEXT)(Display *dpy, GLXDrawable draw, int buffer, const int *attrib_list);
typedef void (*PGLXFUNC_ReleaseTexImageEXT)(Display *dpy, GLXDrawable draw, int buffer);
#endif

#endif /* GLX_EXTRAS */

#endif //___CROPENGL_GLX_PROTO_H
