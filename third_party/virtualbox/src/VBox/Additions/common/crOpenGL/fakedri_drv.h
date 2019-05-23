/* $Id: fakedri_drv.h $ */
/** @file
 * VirtualBox guest OpenGL DRI header
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

#ifndef ___CROPENGL_FAKEDRIDRV_H
#define ___CROPENGL_FAKEDRIDRV_H

#include "src/mesa/main/mtypes.h"
#include "src/mesa/main/dd.h"
#include "src/mesa/glapi/dispatch.h"
#include "src/mesa/glapi/glapi.h"
#include "src/mesa/glapi/glapitable.h"
#include "src/mesa/glapi/glapioffsets.h"
#include "src/mesa/drivers/dri/common/dri_util.h"
#include "GL/internal/dri_interface.h"

#include "glx_proto.h"

#ifdef VBOX_OGL_GLX_USE_CSTUBS
# include "dri_glx.h"
#endif

typedef struct _vbox_glxapi_table
{
    #define GLXAPI_ENTRY(Func) PGLXFUNC_##Func Func;
    #include "fakedri_glxfuncsList.h"
    #undef GLXAPI_ENTRY
} fakedri_glxapi_table;

extern fakedri_glxapi_table glxim;

#ifdef VBOX_OGL_GLX_USE_CSTUBS
/* Extern declarations for our C stubs */
extern void VBOXGLXENTRYTAG(glXFreeMemoryMESA)(Display *dpy, int scrn, void *pointer) ;
extern GLXContext VBOXGLXENTRYTAG(glXImportContextEXT)(Display *dpy, GLXContextID contextID) ;
extern GLXContextID VBOXGLXENTRYTAG(glXGetContextIDEXT)(const GLXContext ctx) ;
extern Bool VBOXGLXENTRYTAG(glXMakeCurrentReadSGI)(Display *display, GLXDrawable draw, GLXDrawable read, GLXContext ctx) ;
extern Display * VBOXGLXENTRYTAG(glXGetCurrentDisplayEXT)(void) ;
extern void VBOXGLXENTRYTAG(glXFreeContextEXT)(Display *dpy, GLXContext ctx) ;
extern int VBOXGLXENTRYTAG(glXQueryContextInfoEXT)(Display *dpy, GLXContext ctx) ;
extern void * VBOXGLXENTRYTAG(glXAllocateMemoryMESA)(Display *dpy, int scrn,
                                                     size_t size, float readFreq,
                                                     float writeFreq, float priority);
extern GLuint VBOXGLXENTRYTAG(glXGetMemoryOffsetMESA)(Display *dpy, int scrn, const void *pointer ) ;
extern GLXPixmap VBOXGLXENTRYTAG(glXCreateGLXPixmapMESA)(Display *dpy, XVisualInfo *visual, Pixmap pixmap, Colormap cmap) ;
extern void VBOXGLXENTRYTAG(glXCopyContext)( Display *dpy, GLXContext src, GLXContext dst, unsigned long mask);
extern void VBOXGLXENTRYTAG(glXUseXFont)(Font font, int first, int count, int listBase) ;
extern CR_GLXFuncPtr VBOXGLXENTRYTAG(glXGetProcAddress)(const GLubyte *name) ;
extern Bool VBOXGLXENTRYTAG(glXQueryExtension)(Display *dpy, int *errorBase, int *eventBase) ;
extern Bool VBOXGLXENTRYTAG(glXIsDirect)(Display *dpy, GLXContext ctx) ;
extern GLXPixmap VBOXGLXENTRYTAG(glXCreateGLXPixmap)(Display *dpy, XVisualInfo *vis, Pixmap pixmap) ;
extern void VBOXGLXENTRYTAG(glXSwapBuffers)(Display *dpy, GLXDrawable drawable) ;
extern GLXDrawable VBOXGLXENTRYTAG(glXGetCurrentDrawable)(void) ;
extern void VBOXGLXENTRYTAG(glXWaitGL)(void) ;
extern Display * VBOXGLXENTRYTAG(glXGetCurrentDisplay)(void) ;
extern const char * VBOXGLXENTRYTAG(glXQueryServerString)(Display *dpy, int screen, int name) ;
extern GLXContext VBOXGLXENTRYTAG(glXCreateContext)(Display *dpy, XVisualInfo *vis, GLXContext share, Bool direct) ;
extern int VBOXGLXENTRYTAG(glXGetConfig)(Display *dpy, XVisualInfo *vis, int attrib, int *value) ;
extern void VBOXGLXENTRYTAG(glXWaitX)(void) ;
extern GLXContext VBOXGLXENTRYTAG(glXGetCurrentContext)(void) ;
extern const char * VBOXGLXENTRYTAG(glXGetClientString)(Display *dpy, int name) ;
extern Bool VBOXGLXENTRYTAG(glXMakeCurrent)(Display *dpy, GLXDrawable drawable, GLXContext ctx) ;
extern void VBOXGLXENTRYTAG(glXDestroyContext)(Display *dpy, GLXContext ctx) ;
extern CR_GLXFuncPtr VBOXGLXENTRYTAG(glXGetProcAddressARB)(const GLubyte *name) ;
extern void VBOXGLXENTRYTAG(glXDestroyGLXPixmap)(Display *dpy, GLXPixmap pix) ;
extern Bool VBOXGLXENTRYTAG(glXQueryVersion)(Display *dpy, int *major, int *minor) ;
extern XVisualInfo * VBOXGLXENTRYTAG(glXChooseVisual)(Display *dpy, int screen, int *attribList) ;
extern const char * VBOXGLXENTRYTAG(glXQueryExtensionsString)(Display *dpy, int screen) ;
extern GLXPbufferSGIX VBOXGLXENTRYTAG(glXCreateGLXPbufferSGIX)(Display *dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height, int *attrib_list);
extern int VBOXGLXENTRYTAG(glXQueryGLXPbufferSGIX)(Display *dpy, GLXPbuffer pbuf, int attribute, unsigned int *value);
extern GLXFBConfigSGIX * VBOXGLXENTRYTAG(glXChooseFBConfigSGIX)(Display *dpy, int screen, int *attrib_list, int *nelements);
extern void VBOXGLXENTRYTAG(glXDestroyGLXPbufferSGIX)(Display *dpy, GLXPbuffer pbuf) ;
extern void VBOXGLXENTRYTAG(glXSelectEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long mask) ;
extern void VBOXGLXENTRYTAG(glXGetSelectedEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long *mask) ;
extern GLXFBConfigSGIX VBOXGLXENTRYTAG(glXGetFBConfigFromVisualSGIX)(Display *dpy, XVisualInfo *vis) ;
extern XVisualInfo * VBOXGLXENTRYTAG(glXGetVisualFromFBConfigSGIX)(Display *dpy, GLXFBConfig config) ;
extern GLXContext VBOXGLXENTRYTAG(glXCreateContextWithConfigSGIX)(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);
extern GLXPixmap VBOXGLXENTRYTAG(glXCreateGLXPixmapWithConfigSGIX)(Display *dpy, GLXFBConfig config, Pixmap pixmap) ;
extern int VBOXGLXENTRYTAG(glXGetFBConfigAttribSGIX)(Display *dpy, GLXFBConfig config, int attribute, int *value) ;
extern GLXFBConfig * VBOXGLXENTRYTAG(glXChooseFBConfig)(Display *dpy, int screen, ATTRIB_TYPE *attrib_list, int *nelements) ;
extern GLXPbuffer VBOXGLXENTRYTAG(glXCreatePbuffer)(Display *dpy, GLXFBConfig config, ATTRIB_TYPE *attrib_list) ;
extern GLXPixmap VBOXGLXENTRYTAG(glXCreatePixmap)(Display *dpy, GLXFBConfig config, Pixmap pixmap, const ATTRIB_TYPE *attrib_list) ;
extern GLXWindow VBOXGLXENTRYTAG(glXCreateWindow)(Display *dpy, GLXFBConfig config, Window win, ATTRIB_TYPE *attrib_list) ;
extern GLXContext VBOXGLXENTRYTAG(glXCreateNewContext)(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);
extern void VBOXGLXENTRYTAG(glXDestroyPbuffer)(Display *dpy, GLXPbuffer pbuf) ;
extern void VBOXGLXENTRYTAG(glXDestroyPixmap)(Display *dpy, GLXPixmap pixmap) ;
extern void VBOXGLXENTRYTAG(glXDestroyWindow)(Display *dpy, GLXWindow win) ;
extern GLXDrawable VBOXGLXENTRYTAG(glXGetCurrentReadDrawable)(void) ;
extern int VBOXGLXENTRYTAG(glXGetFBConfigAttrib)(Display *dpy, GLXFBConfig config, int attribute, int *value) ;
extern GLXFBConfig * VBOXGLXENTRYTAG(glXGetFBConfigs)(Display *dpy, int screen, int *nelements) ;
extern void VBOXGLXENTRYTAG(glXGetSelectedEvent)(Display *dpy, GLXDrawable draw, unsigned long *event_mask) ;
extern XVisualInfo * VBOXGLXENTRYTAG(glXGetVisualFromFBConfig)(Display *dpy, GLXFBConfig config) ;
extern Bool VBOXGLXENTRYTAG(glXMakeContextCurrent)(Display *display, GLXDrawable draw, GLXDrawable read, GLXContext ctx) ;
extern int VBOXGLXENTRYTAG(glXQueryContext)(Display *dpy, GLXContext ctx, int attribute, int *value) ;
extern void VBOXGLXENTRYTAG(glXQueryDrawable)(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value) ;
extern void VBOXGLXENTRYTAG(glXSelectEvent)(Display *dpy, GLXDrawable draw, unsigned long event_mask) ;
#else
/* Extern declarations for our asm stubs */
# define GLXAPI_ENTRY(Func) \
    extern void vbox_glX##Func;\
    extern void vbox_glX##Func##_EndProc;
# include "fakedri_glxfuncsList.h"
# undef GLXAPI_ENTRY
#endif

#endif /* !___CROPENGL_FAKEDRIDRV_H */

