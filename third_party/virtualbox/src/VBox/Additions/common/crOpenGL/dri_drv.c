/* $Id: dri_drv.c $ */

/** @file
 * VBox OpenGL DRI driver functions
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
 *
 * This file is based in part on the tdfx driver from X.Org/Mesa, with the
 * following copyright notice:
 *
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Original rewrite:
 *      Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *      Gareth Hughes <gareth@valinux.com>
 */

#include "cr_error.h"
#include "cr_gl.h"
#include "stub.h"
#include "dri_drv.h"
#include "DD_gl.h"

/** @todo some of those are or'ed with GL_VERSIONS and ain't needed here*/
#define need_GL_ARB_occlusion_query
#define need_GL_ARB_point_parameters
#define need_GL_NV_point_sprite
#define need_GL_ARB_texture_compression
#define need_GL_ARB_transpose_matrix
#define need_GL_ARB_vertex_buffer_object
#define need_GL_ARB_vertex_program
#define need_GL_ARB_window_pos
#define need_GL_EXT_blend_color
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_blend_func_separate
#define need_GL_EXT_fog_coord
#define need_GL_EXT_multi_draw_arrays
#define need_GL_EXT_secondary_color
#define need_GL_EXT_texture_object
#define need_GL_EXT_texture3D
#define need_GL_VERSION_1_3
#define need_GL_VERSION_1_4
#define need_GL_VERSION_1_5

#include "drivers/dri/common/extension_helper.h"

/** @todo add more which are supported by chromium like GL_NV_vertex_program etc.*/
static const struct dri_extension vbox_extensions[] = {
    { "GL_ARB_depth_texture",              NULL },
    { "GL_ARB_fragment_program",           NULL },
    { "GL_ARB_multitexture",               NULL },
    { "GL_ARB_occlusion_query",            GL_ARB_occlusion_query_functions },
    { "GL_ARB_point_parameters",           GL_ARB_point_parameters_functions },
    { "GL_NV_point_sprite",                GL_NV_point_sprite_functions },
    { "GL_ARB_shadow",                     NULL },
    { "GL_ARB_shadow_ambient",             NULL },
    { "GL_ARB_texture_border_clamp",       NULL },
    { "GL_ARB_texture_compression",        GL_ARB_texture_compression_functions },
    { "GL_ARB_texture_cube_map",           NULL },
    { "GL_ARB_texture_env_add",            NULL },
    { "GL_ARB_texture_env_combine",        NULL },
    { "GL_EXT_texture_env_combine",        NULL },
    { "GL_ARB_texture_env_crossbar",       NULL },
    { "GL_ARB_texture_env_dot3",           NULL },
    { "GL_EXT_texture_env_dot3",           NULL },
    { "GL_ARB_texture_mirrored_repeat",    NULL },
    { "GL_ARB_texture_non_power_of_two",   NULL },
    { "GL_ARB_transpose_matrix",           GL_ARB_transpose_matrix_functions },
    { "GL_ARB_vertex_buffer_object",       GL_ARB_vertex_buffer_object_functions },
    { "GL_ARB_vertex_program",             GL_ARB_vertex_program_functions },
    { "GL_ARB_window_pos",                 GL_ARB_window_pos_functions },
    { "GL_EXT_blend_color",                GL_EXT_blend_color_functions },
    { "GL_EXT_blend_minmax",               GL_EXT_blend_minmax_functions },
    { "GL_EXT_blend_func_separate",        GL_EXT_blend_func_separate_functions },
    { "GL_EXT_blend_subtract",             NULL },
    { "GL_EXT_texture_env_add",            NULL }, /** @todo that's an alias to GL_ARB version, remove it?*/
    { "GL_EXT_fog_coord",                  GL_EXT_fog_coord_functions },
    { "GL_EXT_multi_draw_arrays",          GL_EXT_multi_draw_arrays_functions },
    { "GL_EXT_secondary_color",            GL_EXT_secondary_color_functions },
    { "GL_EXT_shadow_funcs",               NULL },
    { "GL_EXT_stencil_wrap",               NULL },
    { "GL_EXT_texture_cube_map",           NULL }, /** @todo another alias*/
    { "GL_EXT_texture_edge_clamp",         NULL },
    { "GL_EXT_texture_filter_anisotropic", NULL },
    { "GL_EXT_texture_lod_bias",           NULL },
    { "GL_EXT_texture_object",             GL_EXT_texture_object_functions },
    { "GL_EXT_texture3D",                  GL_EXT_texture3D_functions },
    { "GL_NV_texgen_reflection",           NULL },
    { "GL_ARB_texture_rectangle",          NULL },
    { "GL_SGIS_generate_mipmap",           NULL },
    { "GL_SGIS_texture_edge_clamp",        NULL } /** @todo another alias*/
};

static void
vboxdriInitExtensions(GLcontext * ctx)
{
    /** @todo have to check extensions supported by host here first */
    driInitExtensions(ctx, vbox_extensions, GL_FALSE);
}

static GLvertexformat vboxdriTnlVtxFmt;


/* This callback tells us that Mesa's internal state has changed.  We probably
 * don't need to handle this ourselves, so just pass it on to other parts of
 * Mesa we may be using, as the swrast driver and others do */
static void
vboxDDUpdateState(GLcontext * ctx, GLuint new_state)
{
    _swrast_InvalidateState(ctx, new_state);
    _swsetup_InvalidateState(ctx, new_state);
    _vbo_InvalidateState(ctx, new_state);
    _tnl_InvalidateState(ctx, new_state);
}

#if 0  /* See comment in vboxdriInitFuncs */
static void
vboxDDGetBufferSize(GLframebuffer *buffer, GLuint *width, GLuint *height)
{
    /*do something, note it's obsolete*/
}

static void
vboxDDResizeBuffer( GLcontext *ctx, GLframebuffer *fb,
                          GLuint width, GLuint height)
{
    /*do something, note it's obsolete*/
}

static void
vboxDDError(GLcontext *ctx)
{
    //__GLcontextRec::ErrorValue contains the error value.
}
#endif

static void
vboxDDDrawPixels(GLcontext *ctx,
                 GLint x, GLint y, GLsizei width, GLsizei height,
                 GLenum format, GLenum type,
                 const struct gl_pixelstore_attrib *unpack,
                 const GLvoid *pixels)
{
}

static void
vboxDDReadPixels(GLcontext *ctx,
                 GLint x, GLint y, GLsizei width, GLsizei height,
                 GLenum format, GLenum type,
                 const struct gl_pixelstore_attrib *unpack,
                 GLvoid *dest)
{
}

static void
vboxDDCopyPixels(GLcontext *ctx, GLint srcx, GLint srcy,
           GLsizei width, GLsizei height,
           GLint dstx, GLint dsty, GLenum type)
{
}

static void
vboxDDBitmap(GLcontext *ctx,
             GLint x, GLint y, GLsizei width, GLsizei height,
             const struct gl_pixelstore_attrib *unpack,
             const GLubyte *bitmap)
{
}

static void
vboxDDTexImage1D(GLcontext *ctx, GLenum target, GLint level,
                 GLint internalFormat,
                 GLint width, GLint border,
                 GLenum format, GLenum type, const GLvoid *pixels,
                 const struct gl_pixelstore_attrib *packing,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
}

static void
vboxDDTexImage2D(GLcontext *ctx, GLenum target, GLint level,
                 GLint internalFormat,
                 GLint width, GLint height, GLint border,
                 GLenum format, GLenum type, const GLvoid *pixels,
                 const struct gl_pixelstore_attrib *packing,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
}

static void
vboxDDTexImage3D(GLcontext *ctx, GLenum target, GLint level,
                 GLint internalFormat,
                 GLint width, GLint height, GLint depth, GLint border,
                 GLenum format, GLenum type, const GLvoid *pixels,
                 const struct gl_pixelstore_attrib *packing,
                 struct gl_texture_object *texObj,
                 struct gl_texture_image *texImage)
{
}

static void
vboxDDTexSubImage1D(GLcontext *ctx, GLenum target, GLint level,
                    GLint xoffset, GLsizei width,
                    GLenum format, GLenum type,
                    const GLvoid *pixels,
                    const struct gl_pixelstore_attrib *packing,
                    struct gl_texture_object *texObj,
                    struct gl_texture_image *texImage)
{
}

static void
vboxDDTexSubImage2D(GLcontext *ctx, GLenum target, GLint level,
                    GLint xoffset, GLint yoffset,
                    GLsizei width, GLsizei height,
                    GLenum format, GLenum type,
                    const GLvoid *pixels,
                    const struct gl_pixelstore_attrib *packing,
                    struct gl_texture_object *texObj,
                    struct gl_texture_image *texImage)
{
}


static void
vboxDDTexSubImage3D(GLcontext *ctx, GLenum target, GLint level,
                    GLint xoffset, GLint yoffset, GLint zoffset,
                    GLsizei width, GLsizei height, GLint depth,
                    GLenum format, GLenum type,
                    const GLvoid *pixels,
                    const struct gl_pixelstore_attrib *packing,
                    struct gl_texture_object *texObj,
                    struct gl_texture_image *texImage)
{
}


static void
vboxDDGetTexImage(GLcontext *ctx, GLenum target, GLint level,
                  GLenum format, GLenum type, GLvoid *pixels,
                  struct gl_texture_object *texObj,
                  struct gl_texture_image *texImage)
{
}

static void
vboxDDBindTexture(GLcontext *ctx, GLenum target,
            struct gl_texture_object *tObj)
{
}

static GLboolean
vboxDDIsTextureResident(GLcontext *ctx, struct gl_texture_object *t)
{
}

static void
vboxDDPrioritizeTexture(GLcontext *ctx, struct gl_texture_object *t,
                        GLclampf priority)
{
}

static void
vboxDDBlendColor(GLcontext *ctx, const GLfloat color[4])
{
}

static void
vboxDDClearColor(GLcontext *ctx, const GLfloat color[4])
{
}

static void
vboxDDClearIndex(GLcontext *ctx, GLuint index)
{
}

static void
vboxDDClipPlane(GLcontext *ctx, GLenum plane, const GLfloat *equation)
{
}

/** @todo Enable or disable server-side gl capabilities, not related to glEnable? */
static void
vboxDDEnable(GLcontext *ctx, GLenum cap, GLboolean state)
{
    if (state)
        cr_glEnable(cap);
    else
        cr_glDisable(cap);
}

static void
vboxDDRenderMode(GLcontext *ctx, GLenum mode)
{
    cr_glRenderMode(mode);
}

static void
vboxDDTexParameter(GLcontext *ctx, GLenum target,
                     struct gl_texture_object *texObj,
                     GLenum pname, const GLfloat *params)
{
}

/*Note, checking glGetError before and after those calls is the only way
 *to return if we succeeded to get value or not, but it will add 2 sync calls and
 *will reset glGetError value returned in case application calls it explicitly
 */
static GLboolean
vboxDDGetBooleanv(GLcontext *ctx, GLenum pname, GLboolean *result)
{
    cr_glGetBooleanv(pname, result);
    return GL_TRUE;
}

static GLboolean
vboxDDGetDoublev(GLcontext *ctx, GLenum pname, GLdouble *result)
{
    cr_glGetDoublev(pname, result);
    return GL_TRUE;
}

static GLboolean
vboxDDGetFloatv(GLcontext *ctx, GLenum pname, GLfloat *result)
{
    cr_glGetFloatv(pname, result);
    return GL_TRUE;
}

static GLboolean
vboxDDGetIntegerv(GLcontext *ctx, GLenum pname, GLint *result)
{
    cr_glGetIntegerv(pname, result);
    return GL_TRUE;
}

static GLboolean
vboxDDGetPointerv(GLcontext *ctx, GLenum pname, GLvoid **result)
{
    cr_glGetPointerv(pname, result);
    return GL_TRUE;
}

/** @todo
 * change stub's createcontext to reuse driver private part of mesa's ctx to store stub ctx info.
 */
#define VBOX_GL_FUNC(func) vboxDD_gl##func
static void
vboxdriInitFuncs(struct dd_function_table *driver)
{
   driver->GetString = VBOX_GL_FUNC(GetString);
   driver->UpdateState = vboxDDUpdateState;
#if 0
   /* I assume that we don't need to change these.  In that case, prefer the
    * default implementation over a stub. */
   driver->GetBufferSize = vboxDDGetBufferSize;
   driver->ResizeBuffers = vboxDDResizeBuffer;
   driver->Error = vboxDDError;
#endif

   driver->Finish = VBOX_GL_FUNC(Finish);
   driver->Flush = VBOX_GL_FUNC(Flush);

   /* framebuffer/image functions */
   driver->Clear = VBOX_GL_FUNC(Clear);
   driver->Accum = VBOX_GL_FUNC(Accum);
   // driver->RasterPos = VBOX_GL_FUNC(RasterPos);  /* No such element in *driver */
   driver->DrawPixels = vboxDDDrawPixels;
   driver->ReadPixels = vboxDDReadPixels;
   driver->CopyPixels = vboxDDCopyPixels;
   driver->Bitmap = vboxDDBitmap;

   /* Texture functions */
    /** @todo deal with texnames and gl_texture_object pointers which are passed here*/
   driver->ChooseTextureFormat = NULL;
   driver->TexImage1D = vboxDDTexImage1D;
   driver->TexImage2D = vboxDDTexImage2D;
   driver->TexImage3D = vboxDDTexImage3D;
   driver->TexSubImage1D = vboxDDTexSubImage1D;
   driver->TexSubImage2D = vboxDDTexSubImage2D;
   driver->TexSubImage3D = vboxDDTexSubImage3D;
   driver->GetTexImage = vboxDDGetTexImage;
   driver->CopyTexImage1D = VBOX_GL_FUNC(CopyTexImage1D);
   driver->CopyTexImage2D = VBOX_GL_FUNC(CopyTexImage2D);
   driver->CopyTexSubImage1D = VBOX_GL_FUNC(CopyTexSubImage1D);
   driver->CopyTexSubImage2D = VBOX_GL_FUNC(CopyTexSubImage2D);
   driver->CopyTexSubImage3D = VBOX_GL_FUNC(CopyTexSubImage3D);
   // driver->GenerateMipmap = VBOX_GL_FUNC(GenerateMipmap); /** @todo or NULL */
   // driver->TestProxyTexImage = vboxDDTestProxyTexImage; /** @todo just pass to glTexImage as we take  care or proxy textures there */
   // driver->CompressedTexImage1D = VBOX_GL_FUNC(CompressedTexImage1D);
   // driver->CompressedTexImage2D = VBOX_GL_FUNC(CompressedTexImage2D);
   // driver->CompressedTexImage3D = VBOX_GL_FUNC(CompressedTexImage3D);
   // driver->CompressedTexSubImage1D = VBOX_GL_FUNC(CompressedTexSubImage1D);
   // driver->CompressedTexSubImage2D = VBOX_GL_FUNC(CompressedTexSubImage2D);
   // driver->CompressedTexSubImage3D = VBOX_GL_FUNC(CompressedTexSubImage3D);
   // driver->GetCompressedTexImage = VBOX_GL_FUNC(GetCompressedTexImage);
   // driver->CompressedTextureSize = NULL; /** @todo */
   driver->BindTexture = vboxDDBindTexture;
   // driver->NewTextureObject = vboxDDNewTextureObject; /** @todo */
   // driver->DeleteTexture = vboxDDDeleteTexture; /** @todo */
   // driver->NewTextureImage = vboxDDNewTextureImage; /** @todo */
   // driver->FreeTexImageData = vboxDDFreeTexImageData; /** @todo */
   // driver->MapTexture = vboxDDMapTexture; /** @todo */
   // driver->UnmapTexture = vboxDDUnmapTexture; /** @todo */
   // driver->TextureMemCpy = vboxDDTextureMemCpy; /** @todo */
   driver->IsTextureResident = vboxDDIsTextureResident;
   driver->PrioritizeTexture = vboxDDPrioritizeTexture;
   driver->ActiveTexture = VBOX_GL_FUNC(ActiveTextureARB);
   // driver->UpdateTexturePalette = vboxDDUpdateTexturePalette; /** @todo */

   /* imaging */
   /*driver->CopyColorTable = _swrast_CopyColorTable;
   driver->CopyColorSubTable = _swrast_CopyColorSubTable;
   driver->CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   driver->CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;*/

   /* Vertex/fragment programs */
   driver->BindProgram = NULL;
   // driver->NewProgram = _mesa_new_program;  /** @todo */
   // driver->DeleteProgram = _mesa_delete_program;  /** @todo */
   driver->ProgramStringNotify = NULL;
#if FEATURE_MESA_program_debug
   // driver->GetProgramRegister = _mesa_get_program_register;  /** @todo */
#endif /* FEATURE_MESA_program_debug */
   driver->IsProgramNative = NULL;

   /* simple state commands */
   driver->AlphaFunc = VBOX_GL_FUNC(AlphaFunc);
   driver->BlendColor = vboxDDBlendColor;
   // driver->BlendEquationSeparate = VBOX_GL_FUNC(BlendEquationSeparate);  /** @todo */
   driver->BlendFuncSeparate = VBOX_GL_FUNC(BlendFuncSeparateEXT);
   driver->ClearColor = vboxDDClearColor;
   driver->ClearDepth = VBOX_GL_FUNC(ClearDepth);
   driver->ClearIndex = vboxDDClearIndex;
   driver->ClearStencil = VBOX_GL_FUNC(ClearStencil);
   driver->ClipPlane = vboxDDClipPlane;
   driver->ColorMask = VBOX_GL_FUNC(ColorMask);
   driver->ColorMaterial = VBOX_GL_FUNC(ColorMaterial);
   driver->CullFace = VBOX_GL_FUNC(CullFace);
   driver->DrawBuffer = VBOX_GL_FUNC(DrawBuffer);  /** @todo */
   // driver->DrawBuffers = VBOX_GL_FUNC(DrawBuffers);   /** @todo */
   driver->FrontFace = VBOX_GL_FUNC(FrontFace);
   driver->DepthFunc = VBOX_GL_FUNC(DepthFunc);
   driver->DepthMask = VBOX_GL_FUNC(DepthMask);
   driver->DepthRange = VBOX_GL_FUNC(DepthRange);
   driver->Enable = vboxDDEnable;
   driver->Fogfv = VBOX_GL_FUNC(Fogfv);
   driver->Hint = VBOX_GL_FUNC(Hint);
   driver->IndexMask = VBOX_GL_FUNC(IndexMask);
   driver->Lightfv = VBOX_GL_FUNC(Lightfv);
   driver->LightModelfv = VBOX_GL_FUNC(LightModelfv);
   driver->LineStipple = VBOX_GL_FUNC(LineStipple);
   driver->LineWidth = VBOX_GL_FUNC(LineWidth);
   // driver->LogicOpcode = VBOX_GL_FUNC(LogicOpcode);   /** @todo */
   driver->PointParameterfv = VBOX_GL_FUNC(PointParameterfvARB);
   driver->PointSize = VBOX_GL_FUNC(PointSize);
   driver->PolygonMode = VBOX_GL_FUNC(PolygonMode);
   driver->PolygonOffset = VBOX_GL_FUNC(PolygonOffset);
   driver->PolygonStipple = VBOX_GL_FUNC(PolygonStipple);
   driver->ReadBuffer = VBOX_GL_FUNC(ReadBuffer);
   driver->RenderMode = vboxDDRenderMode;
   driver->Scissor = VBOX_GL_FUNC(Scissor);
   driver->ShadeModel = VBOX_GL_FUNC(ShadeModel);
   // driver->StencilFuncSeparate = VBOX_GL_FUNC(StencilFuncSeparate);   /** @todo */
   // driver->StencilOpSeparate = VBOX_GL_FUNC(StencilOpSeparate);   /** @todo */
   // driver->StencilMaskSeparate = VBOX_GL_FUNC(StencilMaskSeparate);   /** @todo */
   driver->TexGen = VBOX_GL_FUNC(TexGenfv);
   driver->TexEnv = VBOX_GL_FUNC(TexEnvfv);
   driver->TexParameter = vboxDDTexParameter;
   // driver->TextureMatrix = VBOX_GL_FUNC(TextureMatrix);   /** @todo */
   driver->Viewport = VBOX_GL_FUNC(Viewport);

   /* vertex arrays */
   driver->VertexPointer = VBOX_GL_FUNC(VertexPointer);
   driver->NormalPointer = VBOX_GL_FUNC(NormalPointer);
   driver->ColorPointer = VBOX_GL_FUNC(ColorPointer);
   driver->FogCoordPointer = VBOX_GL_FUNC(FogCoordPointerEXT);
   driver->IndexPointer = VBOX_GL_FUNC(IndexPointer);
   driver->SecondaryColorPointer = VBOX_GL_FUNC(SecondaryColorPointerEXT);
   driver->TexCoordPointer = VBOX_GL_FUNC(TexCoordPointer);
   driver->EdgeFlagPointer = VBOX_GL_FUNC(EdgeFlagPointer);
   // driver->VertexAttribPointer = VBOX_GL_FUNC(VertexAttribPointer);   /** @todo */
   // driver->LockArraysEXT = VBOX_GL_FUNC(LockArraysEXT);   /** @todo */
   // driver->UnlockArraysEXT = VBOX_GL_FUNC(UnlockArraysEXT);   /** @todo */

   /* state queries */
   driver->GetBooleanv = vboxDDGetBooleanv;
   driver->GetDoublev = vboxDDGetDoublev;
   driver->GetFloatv = vboxDDGetFloatv;
   driver->GetIntegerv = vboxDDGetIntegerv;
   driver->GetPointerv = vboxDDGetPointerv;

/** @todo */
#if FEATURE_ARB_vertex_buffer_object
   // driver->NewBufferObject = _mesa_new_buffer_object;
   // driver->DeleteBuffer = _mesa_delete_buffer_object;
   // driver->BindBuffer = NULL;
   // driver->BufferData = _mesa_buffer_data;
   // driver->BufferSubData = _mesa_buffer_subdata;
   // driver->GetBufferSubData = _mesa_buffer_get_subdata;
   // driver->MapBuffer = _mesa_buffer_map;
   // driver->UnmapBuffer = _mesa_buffer_unmap;
#endif

/** @todo */
#if FEATURE_EXT_framebuffer_object
   // driver->NewFramebuffer = _mesa_new_framebuffer;
   // driver->NewRenderbuffer = _mesa_new_soft_renderbuffer;
   // driver->RenderTexture = _mesa_render_texture;
   // driver->FinishRenderTexture = _mesa_finish_render_texture;
   // driver->FramebufferRenderbuffer = _mesa_framebuffer_renderbuffer;
#endif

/** @todo */
#if FEATURE_EXT_framebuffer_blit
   // driver->BlitFramebuffer = _swrast_BlitFramebuffer;
#endif

   /* query objects */
   // driver->NewQueryObject = VBOX_GL_FUNC(NewQueryObject);   /** @todo */
   // driver->DeleteQuery = VBOX_GL_FUNC(DeleteQuery);   /** @todo */
   // driver->BeginQuery = VBOX_GL_FUNC(BeginQuery);   /** @todo */
   // driver->EndQuery = VBOX_GL_FUNC(EndQuery);   /** @todo */
   // driver->WaitQuery = VBOX_GL_FUNC(WaitQuery);   /** @todo */

   /* APPLE_vertex_array_object */
/*
   driver->NewArrayObject = _mesa_new_array_object;
   driver->DeleteArrayObject = _mesa_delete_array_object;
   driver->BindArrayObject = NULL;
*/

   /* T&L stuff */
   driver->NeedValidate = GL_FALSE;
   driver->ValidateTnlModule = NULL;
   driver->CurrentExecPrimitive = 0;
   driver->CurrentSavePrimitive = 0;
   driver->NeedFlush = 0;
   driver->SaveNeedFlush = 0;

   // driver->ProgramStringNotify = _tnl_program_string;   /** @todo */
   driver->FlushVertices = NULL;
   driver->SaveFlushVertices = NULL;
   driver->NotifySaveBegin = NULL;
   driver->LightingSpaceChange = NULL;

   /* display list */
   driver->NewList = VBOX_GL_FUNC(NewList);
   driver->EndList = VBOX_GL_FUNC(EndList);
   // driver->BeginCallList = VBOX_GL_FUNC(BeginCallList);   /** @todo */
   // driver->EndCallList = VBOX_GL_FUNC(EndCallList);   /** @todo */


   /* shaders */
   /*
   driver->AttachShader = _mesa_attach_shader;
   driver->BindAttribLocation = _mesa_bind_attrib_location;
   driver->CompileShader = _mesa_compile_shader;
   driver->CreateProgram = _mesa_create_program;
   driver->CreateShader = _mesa_create_shader;
   driver->DeleteProgram2 = _mesa_delete_program2;
   driver->DeleteShader = _mesa_delete_shader;
   driver->DetachShader = _mesa_detach_shader;
   driver->GetActiveAttrib = _mesa_get_active_attrib;
   driver->GetActiveUniform = _mesa_get_active_uniform;
   driver->GetAttachedShaders = _mesa_get_attached_shaders;
   driver->GetAttribLocation = _mesa_get_attrib_location;
   driver->GetHandle = _mesa_get_handle;
   driver->GetProgramiv = _mesa_get_programiv;
   driver->GetProgramInfoLog = _mesa_get_program_info_log;
   driver->GetShaderiv = _mesa_get_shaderiv;
   driver->GetShaderInfoLog = _mesa_get_shader_info_log;
   driver->GetShaderSource = _mesa_get_shader_source;
   driver->GetUniformfv = _mesa_get_uniformfv;
   driver->GetUniformiv = _mesa_get_uniformiv;
   driver->GetUniformLocation = _mesa_get_uniform_location;
   driver->IsProgram = _mesa_is_program;
   driver->IsShader = _mesa_is_shader;
   driver->LinkProgram = _mesa_link_program;
   driver->ShaderSource = _mesa_shader_source;
   driver->Uniform = _mesa_uniform;
   driver->UniformMatrix = _mesa_uniform_matrix;
   driver->UseProgram = _mesa_use_program;
   driver->ValidateProgram = _mesa_validate_program;
   */
}

static void
vboxdriInitTnlVtxFmt(GLvertexformat *pVtxFmt)
{
   pVtxFmt->ArrayElement = VBOX_GL_FUNC(ArrayElement);
   pVtxFmt->Begin = VBOX_GL_FUNC(Begin);
   pVtxFmt->CallList = VBOX_GL_FUNC(CallList);
   pVtxFmt->CallLists = VBOX_GL_FUNC(CallLists);
   pVtxFmt->Color3f = VBOX_GL_FUNC(Color3f);
   pVtxFmt->Color3fv = VBOX_GL_FUNC(Color3fv);
   pVtxFmt->Color4f = VBOX_GL_FUNC(Color4f);
   pVtxFmt->Color4fv = VBOX_GL_FUNC(Color4fv);
   pVtxFmt->EdgeFlag = VBOX_GL_FUNC(EdgeFlag);
   pVtxFmt->End = VBOX_GL_FUNC(End);
   pVtxFmt->EvalCoord1f = VBOX_GL_FUNC(EvalCoord1f);
   pVtxFmt->EvalCoord1fv = VBOX_GL_FUNC(EvalCoord1fv);
   pVtxFmt->EvalCoord2f = VBOX_GL_FUNC(EvalCoord2f);
   pVtxFmt->EvalCoord2fv = VBOX_GL_FUNC(EvalCoord2fv);
   pVtxFmt->EvalPoint1 = VBOX_GL_FUNC(EvalPoint1);
   pVtxFmt->EvalPoint2 = VBOX_GL_FUNC(EvalPoint2);
   pVtxFmt->FogCoordfEXT = VBOX_GL_FUNC(FogCoordfEXT);
   pVtxFmt->FogCoordfvEXT = VBOX_GL_FUNC(FogCoordfvEXT);
   pVtxFmt->Indexf = VBOX_GL_FUNC(Indexf);
   pVtxFmt->Indexfv = VBOX_GL_FUNC(Indexfv);
   pVtxFmt->Materialfv = VBOX_GL_FUNC(Materialfv);
   pVtxFmt->MultiTexCoord1fARB = VBOX_GL_FUNC(MultiTexCoord1fARB);
   pVtxFmt->MultiTexCoord1fvARB = VBOX_GL_FUNC(MultiTexCoord1fvARB);
   pVtxFmt->MultiTexCoord2fARB = VBOX_GL_FUNC(MultiTexCoord2fARB);
   pVtxFmt->MultiTexCoord2fvARB = VBOX_GL_FUNC(MultiTexCoord2fvARB);
   pVtxFmt->MultiTexCoord3fARB = VBOX_GL_FUNC(MultiTexCoord3fARB);
   pVtxFmt->MultiTexCoord3fvARB = VBOX_GL_FUNC(MultiTexCoord3fvARB);
   pVtxFmt->MultiTexCoord4fARB = VBOX_GL_FUNC(MultiTexCoord4fARB);
   pVtxFmt->MultiTexCoord4fvARB = VBOX_GL_FUNC(MultiTexCoord4fvARB);
   pVtxFmt->Normal3f = VBOX_GL_FUNC(Normal3f);
   pVtxFmt->Normal3fv = VBOX_GL_FUNC(Normal3fv);
   pVtxFmt->SecondaryColor3fEXT = VBOX_GL_FUNC(SecondaryColor3fEXT);
   pVtxFmt->SecondaryColor3fvEXT = VBOX_GL_FUNC(SecondaryColor3fvEXT);
   pVtxFmt->TexCoord1f = VBOX_GL_FUNC(TexCoord1f);
   pVtxFmt->TexCoord1fv = VBOX_GL_FUNC(TexCoord1fv);
   pVtxFmt->TexCoord2f = VBOX_GL_FUNC(TexCoord2f);
   pVtxFmt->TexCoord2fv = VBOX_GL_FUNC(TexCoord2fv);
   pVtxFmt->TexCoord3f = VBOX_GL_FUNC(TexCoord3f);
   pVtxFmt->TexCoord3fv = VBOX_GL_FUNC(TexCoord3fv);
   pVtxFmt->TexCoord4f = VBOX_GL_FUNC(TexCoord4f);
   pVtxFmt->TexCoord4fv = VBOX_GL_FUNC(TexCoord4fv);
   pVtxFmt->Vertex2f = VBOX_GL_FUNC(Vertex2f);
   pVtxFmt->Vertex2fv = VBOX_GL_FUNC(Vertex2fv);
   pVtxFmt->Vertex3f = VBOX_GL_FUNC(Vertex3f);
   pVtxFmt->Vertex3fv = VBOX_GL_FUNC(Vertex3fv);
   pVtxFmt->Vertex4f = VBOX_GL_FUNC(Vertex4f);
   pVtxFmt->Vertex4fv = VBOX_GL_FUNC(Vertex4fv);
   pVtxFmt->VertexAttrib1fNV = VBOX_GL_FUNC(VertexAttrib1fARB);
   pVtxFmt->VertexAttrib1fvNV = VBOX_GL_FUNC(VertexAttrib1fvARB);
   pVtxFmt->VertexAttrib2fNV = VBOX_GL_FUNC(VertexAttrib2fARB);
   pVtxFmt->VertexAttrib2fvNV = VBOX_GL_FUNC(VertexAttrib2fvARB);
   pVtxFmt->VertexAttrib3fNV = VBOX_GL_FUNC(VertexAttrib3fARB);
   pVtxFmt->VertexAttrib3fvNV = VBOX_GL_FUNC(VertexAttrib3fvARB);
   pVtxFmt->VertexAttrib4fNV = VBOX_GL_FUNC(VertexAttrib4fARB);
   pVtxFmt->VertexAttrib4fvNV = VBOX_GL_FUNC(VertexAttrib4fvARB);
   pVtxFmt->VertexAttrib1fARB = VBOX_GL_FUNC(VertexAttrib1fARB);
   pVtxFmt->VertexAttrib1fvARB = VBOX_GL_FUNC(VertexAttrib1fvARB);
   pVtxFmt->VertexAttrib2fARB = VBOX_GL_FUNC(VertexAttrib2fARB);
   pVtxFmt->VertexAttrib2fvARB = VBOX_GL_FUNC(VertexAttrib2fvARB);
   pVtxFmt->VertexAttrib3fARB = VBOX_GL_FUNC(VertexAttrib3fARB);
   pVtxFmt->VertexAttrib3fvARB = VBOX_GL_FUNC(VertexAttrib3fvARB);
   pVtxFmt->VertexAttrib4fARB = VBOX_GL_FUNC(VertexAttrib4fARB);
   pVtxFmt->VertexAttrib4fvARB = VBOX_GL_FUNC(VertexAttrib4fvARB);

   pVtxFmt->EvalMesh1 = VBOX_GL_FUNC(EvalMesh1);
   pVtxFmt->EvalMesh2 = VBOX_GL_FUNC(EvalMesh2);
   pVtxFmt->Rectf = VBOX_GL_FUNC(Rectf);

   pVtxFmt->DrawArrays = VBOX_GL_FUNC(DrawArrays);
   pVtxFmt->DrawElements = VBOX_GL_FUNC(DrawElements);
   pVtxFmt->DrawRangeElements = VBOX_GL_FUNC(DrawRangeElements);
}
#undef VBOX_GL_FUNC

static void
vboxdriExtSetTexOffset(__DRIcontext *pDRICtx, GLint texname,
                       unsigned long long offset, GLint depth, GLuint pitch)
{
}


static void
vboxdriExtSetTexBuffer(__DRIcontext *pDRICtx, GLint target, __DRIdrawable *dPriv)
{
}

/** @todo not sure we need it from start*/
static const __DRItexOffsetExtension vboxdriTexOffsetExtension = {
    { __DRI_TEX_OFFSET },
    vboxdriExtSetTexOffset,
};

/* This DRI extension is required to support EXT_texture_from_pixmap,
 * which in turn is required by compiz.
 */
static const __DRItexBufferExtension vboxdriTexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
    vboxdriExtSetTexBuffer,
};

/* List of DRI extensions supported by VBox DRI driver */
static const __DRIextension *vboxdriExtensions[] = {
    &vboxdriTexOffsetExtension.base,
    &vboxdriTexBufferExtension.base,
    NULL
};

static __GLcontextModes *vboxdriFillInModes(unsigned pixel_bits,
                                            unsigned depth_bits,
                                            unsigned stencil_bits,
                                            GLboolean have_back_buffer)
{
        unsigned deep = (depth_bits > 17);

        /* Right now GLX_SWAP_COPY_OML isn't supported, but it would be easy
         * enough to add support.  Basically, if a context is created with an
         * fbconfig where the swap method is GLX_SWAP_COPY_OML, pageflipping
         * will never be used.
         */

        static const GLenum db_modes[2] = { GLX_NONE, GLX_SWAP_UNDEFINED_OML };
        uint8_t depth_bits_array[4];
        uint8_t stencil_bits_array[4];
        if(deep) {
                depth_bits_array[0] = 0;
                depth_bits_array[1] = 24;
                stencil_bits_array[0] = 0;
                stencil_bits_array[1] = 8;
        } else {
                depth_bits_array[0] = depth_bits;
                depth_bits_array[1] = 0;
                depth_bits_array[2] = depth_bits;
                depth_bits_array[3] = 0;
                stencil_bits_array[0] = 0;
                stencil_bits_array[1] = 0;
                stencil_bits_array[2] = 8;
                stencil_bits_array[3] = 8;
        }

        return driCreateConfigs(
                deep ? GL_RGBA : GL_RGB,
                deep ? GL_UNSIGNED_INT_8_8_8_8 : GL_UNSIGNED_SHORT_5_6_5,
                depth_bits_array,
                stencil_bits_array,
                deep ? 2 : 4,
                db_modes, 2);
}

/**
 * This is the driver specific part of the createNewScreen entry point.
 * Called when using legacy DRI.
 *
 * return the __GLcontextModes supported by this driver
 */
static const __DRIconfig **vboxdriInitScreen(__DRIscreenPrivate *psp)
{
    static const __DRIversion ddx_expected = { 1, 1, 0 };
    static const __DRIversion dri_expected = { 4, 0, 0 };
    static const __DRIversion drm_expected = { 1, 0, 0 };
    //PVBoxDRI = (PVBoxDRI) psp->pDevPrivate;

    /* Initialise our call table in chromium. */
    if (!stubInit())
    {
        crDebug("vboxdriInitScreen: stubInit failed");
        return NULL;
    }

    if ( ! driCheckDriDdxDrmVersions2( "tdfx",
                                       &psp->dri_version, & dri_expected,
                                       &psp->ddx_version, & ddx_expected,
                                       &psp->drm_version, & drm_expected ) )
        return NULL;

    /* Calling driInitExtensions here, with a NULL context pointer,
     * does not actually enable the extensions.  It just makes sure
     * that all the dispatch offsets for all the extensions that
     * *might* be enables are known.  This is needed because the
     * dispatch offsets need to be known when _mesa_context_create is
     * called, but we can't enable the extensions until we have a
     * context pointer.
     *
     * Hello chicken.  Hello egg.  How are you two today?
     */
    vboxdriInitExtensions(NULL);

    /** @todo check size of DRIRec (passed from X.Org driver), allocate private
     * structure if necessary, parse options if necessary, map VRAM if
     * necessary. */
    psp->extensions = vboxdriExtensions;

    /* Initialise VtxFmt call table. */
    vboxdriInitTnlVtxFmt(&vboxdriTnlVtxFmt);

    /*return (const __DRIconfig **)
           vboxdriFillInModes(psp, dri_priv->cpp * 8,
                              (dri_priv->cpp == 2) ? 16 : 24,
                              (dri_priv->cpp == 2) ? 0  : 8, 1);*/

    /** @todo This should depend on what the host can do, not the guest.
     *  However, we will probably want to discover that in the X.Org driver,
     *  not here. */
    return (const __DRIconfig **) vboxdriFillInModes(psp, 24, 24, 8, 1);
}

static void
vboxdriDestroyScreen(__DRIscreenPrivate * sPriv)
{
    crDebug("vboxdriDestroyScreen");
#if 0  /* From the tdfx driver */
   /* free all option information */
   driDestroyOptionInfo (&sPriv->private->optionCache);
   FREE(sPriv->private);
   sPriv->private = NULL;
#endif
}

static GLboolean
vboxdriCreateContext(const __GLcontextModes * mesaVis,
                     __DRIcontextPrivate * driContextPriv,
                     void *sharedContextPrivate)
{
    //__DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
    struct dd_function_table functions;
    GLcontext *ctx, *shareCtx;

#if 0  /* We shouldn't need this sort of thing. */
    XVisualInfo *vis;
    vis->visual->visualid;
    context->dpy = dpy;
    context->visual = vis;

    GLXContext vboxctx = glXCreateContext(dpy, mesaVis->visualID, GL_TRUE);
#endif
    /* We should be allocating a private context structure, where we will
     * remember the Mesa context (ctx) among other things.  The TDFX driver
     * also saves importand information in driContextPriv in there - is this
     * not always available to us? */
    //driContextPriv->driverPrivate = vboxctx;

    /* Initialise the default driver functions then plug in our vbox ones,
     * which will actually replace most of the defaults. */
    /** @todo we should also pass some information from the visual back to the
     * host. */
    _mesa_init_driver_functions(&functions);
    vboxdriInitFuncs(&functions);

    /* Allocate context information for Mesa. */
    if (sharedContextPrivate)
      shareCtx = ((tdfxContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;
    /** @todo save ctx, or be more confident that we can don't need to. */
    ctx = _mesa_create_context(mesaVis, shareCtx, &functions,
                               driContextPriv->driverPrivate);
    if (!ctx)
    {
        crDebug("vboxdriCreateContext: _mesa_create_context failed");
        return GL_FALSE;
    }

    /* The TDFX driver parses its configuration files here, via
     * driParseConfigFiles.  We will probably get any information via guest
     * properties. */

   /* Set various context configuration.  We take these values from the
    * TDFX driver. */
   /** @r=Leonid, stub.spu->dispatch_table.GetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&value) etc.
    * Those would be cached where possible, see include/state/cr_limits.h, VBoxOGLgen/packspu_get.c
    * Note, that ctx->Const.MaxTextureImageUnits is *not* related to GL_MAX_TEXTURE_UNITS_ARB,
    * use GL_MAX_TEXTURE_IMAGE_UNITS_ARB instead.
    * Also, those could fail if we haven't made ctx in our stub yet.
    */
   ctx->Const.MaxTextureLevels = 9;
   ctx->Const.MaxTextureUnits = 1;
   ctx->Const.MaxTextureImageUnits = ctx->Const.MaxTextureUnits;
   ctx->Const.MaxTextureCoordUnits = ctx->Const.MaxTextureUnits;

   /* No wide points.
    */
   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 1.0;
   ctx->Const.MaxPointSizeAA = 1.0;

   /* Disable wide lines as we can't antialias them correctly in
    * hardware.
    */
   /** @note That applies to the TDFX, not to us, but as I don't yet know
    * what to use instead I am leaving the values for now. */
   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 1.0;
   ctx->Const.MaxLineWidthAA = 1.0;
   ctx->Const.LineWidthGranularity = 1.0;

   /* Initialize the software rasterizer and helper modules - again, TDFX */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );

   /* Install the customized pipeline, TDFX */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, tdfx_pipeline );

   /* Configure swrast and T&L to match hardware characteristics, TDFX */
   _swrast_allow_pixel_fog( ctx, GL_TRUE );
   _swrast_allow_vertex_fog( ctx, GL_FALSE );
   _tnl_allow_pixel_fog( ctx, GL_TRUE );
   _tnl_allow_vertex_fog( ctx, GL_FALSE );
    /*ctx->DriverCtx = ;*/

    /* This was *not* in the TDFX driver. */
    _mesa_install_exec_vtxfmt(ctx, &vboxdriTnlVtxFmt);

    vboxdriInitExtensions(ctx);

    return GL_TRUE;
}


static void
vboxdriDestroyContext(__DRIcontextPrivate *driContextPriv)
{
    // glXDestroyContext(driContextPriv->driverPrivate);
    //_mesa_destroy_context ?
}

/**
 * This is called when we need to set up GL rendering to a new X window.
 */
static GLboolean
vboxdriCreateBuffer(__DRIscreenPrivate * driScrnPriv,
                    __DRIdrawablePrivate * driDrawPriv,
                    const __GLcontextModes * mesaVis, GLboolean isPixmap)
{
    return GL_FALSE;
}

static void
vboxdriDestroyBuffer(__DRIdrawablePrivate * driDrawPriv)
{
}

static void
vboxdriSwapBuffers(__DRIdrawablePrivate * dPriv)
{
}


GLboolean
vboxdriMakeCurrent(__DRIcontextPrivate * driContextPriv,
                   __DRIdrawablePrivate * driDrawPriv,
                   __DRIdrawablePrivate * driReadPriv)
{
    return GL_FALSE;
}

GLboolean
vboxdriUnbindContext(__DRIcontextPrivate * driContextPriv)
{
    return GL_TRUE;
}


/* This structure is used by dri_util from mesa, don't rename it! */
DECLEXPORT(const struct __DriverAPIRec) driDriverAPI = {
    .InitScreen       = vboxdriInitScreen,
    .DestroyScreen    = vboxdriDestroyScreen,
    .CreateContext    = vboxdriCreateContext,
    .DestroyContext   = vboxdriDestroyContext,
    .CreateBuffer     = vboxdriCreateBuffer,
    .DestroyBuffer    = vboxdriDestroyBuffer,
    .SwapBuffers      = vboxdriSwapBuffers,
    .MakeCurrent      = vboxdriMakeCurrent,
    .UnbindContext    = vboxdriUnbindContext,
    .GetSwapInfo      = NULL,
    .WaitForMSC       = NULL,
    .WaitForSBC       = NULL,
    .SwapBuffersMSC   = NULL,
    .CopySubBuffer    = NULL,
    .GetDrawableMSC   = NULL,
    .InitScreen2      = NULL
};
