/* $Id: renderspu_cocoa_helper.h $ */
/** @file
 * VirtualBox OpenGL Cocoa Window System Helper definition
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

#ifndef ___renderspu_cocoa_helper_h
#define ___renderspu_cocoa_helper_h

#include <iprt/cdefs.h>
#include <VBox/VBoxCocoa.h>
#include <OpenGL/OpenGL.h>
#ifdef IN_VMSVGA3D
# include "../../../GuestHost/OpenGL/include/cr_vreg.h"
# include "../../../GuestHost/OpenGL/include/cr_compositor.h"
#else
# include <cr_vreg.h>
# include <cr_compositor.h>
#endif


RT_C_DECLS_BEGIN

struct WindowInfo;

ADD_COCOA_NATIVE_REF(NSView);
ADD_COCOA_NATIVE_REF(NSOpenGLContext);

/** @name OpenGL context management
 * @{ */
void cocoaGLCtxCreate(NativeNSOpenGLContextRef *ppCtx, GLbitfield fVisParams, NativeNSOpenGLContextRef pSharedCtx);
void cocoaGLCtxDestroy(NativeNSOpenGLContextRef pCtx);
/** @} */

/** @name View management
 * @{ */
void cocoaViewCreate(NativeNSViewRef *ppView, struct WindowInfo *pWinInfo, NativeNSViewRef pParentView, GLbitfield fVisParams);
void cocoaViewReparent(NativeNSViewRef pView, NativeNSViewRef pParentView);
void cocoaViewDestroy(NativeNSViewRef pView);
void cocoaViewDisplay(NativeNSViewRef pView);
void cocoaViewShow(NativeNSViewRef pView, GLboolean fShowIt);
void cocoaViewSetPosition(NativeNSViewRef pView, NativeNSViewRef pParentView, int x, int y);
void cocoaViewSetSize(NativeNSViewRef pView, int cx, int cy);
void cocoaViewGetGeometry(NativeNSViewRef pView, int *px, int *py, int *pcx, int *pcy);
void cocoaViewMakeCurrentContext(NativeNSViewRef pView, NativeNSOpenGLContextRef pCtx);
void cocoaViewSetVisibleRegion(NativeNSViewRef pView, GLint cRects, const GLint *paRects);
GLboolean cocoaViewNeedsEmptyPresent(NativeNSViewRef pView);
void cocoaViewPresentComposition(NativeNSViewRef pView, const struct VBOXVR_SCR_COMPOSITOR_ENTRY *pChangedEntry);
/** @} */

RT_C_DECLS_END

#endif /* !___renderspu_cocoa_helper_h */

