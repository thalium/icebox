/* $Id: VBoxDbgGl.h $ */
/** @file
 * VBox wine & ogl debugging stuff
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBoxDbgGl_h__
#define ___VBoxDbgGl_h__

//#include <windows.h>
//#include "../wined3d/wined3d_gl.h"

void dbglGetTexImage2D(const struct wined3d_gl_info *gl_info, GLint texTarget, GLint texName, GLint level, GLvoid **ppvImage, GLint *pw, GLint *ph, GLenum format, GLenum type);
void dbglDumpTexImage2D(const struct wined3d_gl_info *gl_info, const char* pszDesc, GLint texTarget, GLint texName, GLint level, GLboolean fBreak);
void dbglCmpTexImage2D(const struct wined3d_gl_info *gl_info, const char* pszDesc, GLint texTarget, GLint texName, GLint level, void *pvImage, GLint width, GLint height, GLenum format, GLenum type);
void dbglCheckTexUnits(const struct wined3d_gl_info *gl_info, const struct IWineD3DDeviceImpl *pDevice, BOOL fBreakIfCanNotMatch);

#ifdef DEBUG_misha
#define DBGL_CHECK_DRAWPRIM(_gl_info, _pDevice) do { \
        if (g_VBoxDbgGlFCheckDrawPrim) { \
            dbglCheckTexUnits((_gl_info), (_pDevice), g_VBoxDbgGlFBreakDrawPrimIfCanNotMatch); \
        } \
    } while (0)
#else
#define DBGL_CHECK_DRAWPRIM(_gl_info, _pDevice) do {} while (0)
#endif

extern DWORD g_VBoxDbgGlFCheckDrawPrim;
extern DWORD g_VBoxDbgGlFBreakDrawPrimIfCanNotMatch;

#endif /* #ifndef ___VBoxDbgGl_h__ */
