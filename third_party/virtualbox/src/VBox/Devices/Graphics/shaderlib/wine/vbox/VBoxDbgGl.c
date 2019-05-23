/* $Id: VBoxDbgGl.c $ */
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
#include <iprt/ctype.h>

#include <config.h>
#include "../wined3d/wined3d_private.h"

#include "VBoxDbgGl.h"

DWORD g_VBoxDbgGlFCheckDrawPrim = 0;
DWORD g_VBoxDbgGlFBreakDrawPrimIfCanNotMatch = 0;

void dbglFree(void *pvData)
{
    HeapFree(GetProcessHeap(),  0, pvData);
}

void* dbglAlloc(DWORD cbSize)
{
    return HeapAlloc(GetProcessHeap(),  0, cbSize);
}

GLint dbglFmtGetNumComponents(GLenum format)
{
    switch(format)
    {
        case GL_COLOR_INDEX:
        case GL_RED:
        case GL_GREEN:
        case GL_BLUE:
        case GL_ALPHA:
            return 1;
        case GL_RGB:
        case GL_BGR:
            return 3;
        case GL_RGBA:
        case GL_BGRA:
            return 4;
        case GL_LUMINANCE:
            return 1;
        case GL_LUMINANCE_ALPHA:
            return 2;
        default:
            Assert(0);
            return 1;
    }
}

GLint dbglFmtGetBpp(GLenum format, GLenum type)
{
    switch (type)
    {
        case GL_UNSIGNED_BYTE_3_3_2:
            return 8;
        case GL_UNSIGNED_BYTE_2_3_3_REV:
            return 8;
        case GL_UNSIGNED_SHORT_5_6_5:
            return 16;
        case GL_UNSIGNED_SHORT_5_6_5_REV:
            return 16;
        case GL_UNSIGNED_SHORT_4_4_4_4:
            return 16;
        case GL_UNSIGNED_SHORT_4_4_4_4_REV:
            return 16;
        case GL_UNSIGNED_SHORT_5_5_5_1:
            return 16;
        case GL_UNSIGNED_SHORT_1_5_5_5_REV:
            return 16;
        case GL_UNSIGNED_INT_8_8_8_8:
            return 32;
        case GL_UNSIGNED_INT_8_8_8_8_REV:
            return 32;
        case GL_UNSIGNED_INT_10_10_10_2:
            return 32;
        case GL_UNSIGNED_INT_2_10_10_10_REV:
            return 32;
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
            return dbglFmtGetNumComponents(format) * 8;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
            return dbglFmtGetNumComponents(format) * 16;
        case GL_INT:
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
            return dbglFmtGetNumComponents(format) * 32;
        case GL_DOUBLE:
            return dbglFmtGetNumComponents(format) * 64;
        case GL_2_BYTES:
            return dbglFmtGetNumComponents(format) * 16;
        case GL_3_BYTES:
            return dbglFmtGetNumComponents(format) * 24;
        case GL_4_BYTES:
            return dbglFmtGetNumComponents(format) * 32;
        default:
            Assert(0);
            return 8;
    }
}

#define DBGL_OP(_op) gl##_op
#define DBGL_FBO_OP(_op) gl_info->fbo_ops.gl##_op
#define DBGL_EXT_OP(_op)  gl_info->gl##_op

void dbglGetTexImage2D(const struct wined3d_gl_info *gl_info, GLint texTarget, GLint texName, GLint level, GLvoid **ppvImage, GLint *pw, GLint *ph, GLenum format, GLenum type)
{
    GLint ppb, pub, dstw, dsth, otex;
    GLint pa, pr, psp, psr, ua, ur, usp, usr;
    GLvoid *pvImage;
    GLint rfb, dfb, rb, db;
    GLint bpp = dbglFmtGetBpp(format, type);

    DBGL_OP(GetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &rfb));
    DBGL_OP(GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &dfb));
    DBGL_OP(GetIntegerv(GL_READ_BUFFER, &rb));
    DBGL_OP(GetIntegerv(GL_DRAW_BUFFER, &db));

    DBGL_FBO_OP(BindFramebuffer)(GL_READ_FRAMEBUFFER_BINDING_EXT, 0);
    DBGL_FBO_OP(BindFramebuffer)(GL_DRAW_FRAMEBUFFER_BINDING_EXT, 0);
    DBGL_OP(ReadBuffer(GL_BACK));
    DBGL_OP(DrawBuffer(GL_BACK));

    DBGL_OP(GetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_ARB, &ppb));
    DBGL_OP(GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, &pub));
    DBGL_OP(GetIntegerv(GL_TEXTURE_BINDING_2D, &otex));

    DBGL_OP(GetIntegerv(GL_PACK_ROW_LENGTH, &pr));
    DBGL_OP(GetIntegerv(GL_PACK_ALIGNMENT, &pa));
    DBGL_OP(GetIntegerv(GL_PACK_SKIP_PIXELS, &psp));
    DBGL_OP(GetIntegerv(GL_PACK_SKIP_ROWS, &psr));

    DBGL_OP(GetIntegerv(GL_UNPACK_ROW_LENGTH, &ur));
    DBGL_OP(GetIntegerv(GL_UNPACK_ALIGNMENT, &ua));
    DBGL_OP(GetIntegerv(GL_UNPACK_SKIP_PIXELS, &usp));
    DBGL_OP(GetIntegerv(GL_UNPACK_SKIP_ROWS, &usr));

    DBGL_OP(BindTexture(texTarget, texName));
    DBGL_OP(GetTexLevelParameteriv(texTarget, level, GL_TEXTURE_WIDTH, &dstw));
    DBGL_OP(GetTexLevelParameteriv(texTarget, level, GL_TEXTURE_HEIGHT, &dsth));

    DBGL_OP(PixelStorei(GL_PACK_ROW_LENGTH, 0));
    DBGL_OP(PixelStorei(GL_PACK_ALIGNMENT, 1));
    DBGL_OP(PixelStorei(GL_PACK_SKIP_PIXELS, 0));
    DBGL_OP(PixelStorei(GL_PACK_SKIP_ROWS, 0));

    DBGL_OP(PixelStorei(GL_UNPACK_ROW_LENGTH, 0));
    DBGL_OP(PixelStorei(GL_UNPACK_ALIGNMENT, 1));
    DBGL_OP(PixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
    DBGL_OP(PixelStorei(GL_UNPACK_SKIP_ROWS, 0));

    DBGL_EXT_OP(BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0));
    DBGL_EXT_OP(BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));

    pvImage = dbglAlloc(((bpp*dstw + 7) >> 3)*dsth);
    DBGL_OP(GetTexImage(texTarget, level, format, type, pvImage));

    DBGL_OP(BindTexture(texTarget, otex));

    DBGL_OP(PixelStorei(GL_PACK_ROW_LENGTH, pr));
    DBGL_OP(PixelStorei(GL_PACK_ALIGNMENT, pa));
    DBGL_OP(PixelStorei(GL_PACK_SKIP_PIXELS, psp));
    DBGL_OP(PixelStorei(GL_PACK_SKIP_ROWS, psr));

    DBGL_OP(PixelStorei(GL_UNPACK_ROW_LENGTH, ur));
    DBGL_OP(PixelStorei(GL_UNPACK_ALIGNMENT, ua));
    DBGL_OP(PixelStorei(GL_UNPACK_SKIP_PIXELS, usp));
    DBGL_OP(PixelStorei(GL_UNPACK_SKIP_ROWS, usr));

    DBGL_EXT_OP(BindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, ppb));
    DBGL_EXT_OP(BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pub));

    DBGL_FBO_OP(BindFramebuffer)(GL_READ_FRAMEBUFFER_BINDING_EXT, rfb);
    DBGL_FBO_OP(BindFramebuffer)(GL_DRAW_FRAMEBUFFER_BINDING_EXT, dfb);
    DBGL_OP(ReadBuffer(rb));
    DBGL_OP(DrawBuffer(db));

    *ppvImage = pvImage;
    *pw = dstw;
    *ph = dsth;
}

DECLEXPORT(void) dbglPrint(const char *format, ... )
{
    va_list args;
    static char txt[8092];

    va_start( args, format );
    vsprintf( txt, format, args );

    OutputDebugString(txt);
}

void dbglDumpImage2D(const char* pszDesc, const void *pvData, uint32_t width, uint32_t height, uint32_t bpp, uint32_t pitch)
{
    dbglPrint("<?dml?><exec cmd=\"!vbvdbg.ms 0x%p 0n%d 0n%d 0n%d 0n%d\">%s</exec>, ( !vbvdbg.ms 0x%p 0n%d 0n%d 0n%d 0n%d )\n",
            pvData, width, height, bpp, pitch,
            pszDesc,
            pvData, width, height, bpp, pitch);
}

void dbglDumpTexImage2D(const struct wined3d_gl_info *gl_info, const char* pszDesc, GLint texTarget, GLint texName, GLint level, GLboolean fBreak)
{
    GLvoid *pvImage;
    GLint w, h;
    dbglGetTexImage2D(gl_info, texTarget, texName, level, &pvImage, &w, &h, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV);
    dbglPrint("%s target(%d), name(%d), width(%d), height(%d)\n", pszDesc, texTarget, texName, w, h);
    dbglDumpImage2D("texture data", pvImage, w, h, 32, (32 * w)/8);
    if (fBreak)
    {
        Assert(0);
    }
    dbglFree(pvImage);
}

void dbglCmpTexImage2D(const struct wined3d_gl_info *gl_info, const char* pszDesc, GLint texTarget, GLint texName, GLint level, void *pvImage,
        GLint width, GLint height, GLenum format, GLenum type)
{
    GLvoid *pvTexImg;
    GLint w, h;
    GLint bpp = dbglFmtGetBpp(format, type);
    GLint pitch, texPitch;

    dbglGetTexImage2D(gl_info, texTarget, texName, level, &pvTexImg, &w, &h, format, type);

    pitch = ((bpp * width + 7) >> 3)/8;
    texPitch = ((bpp * w + 7) >> 3)/8;

    if (w != width)
    {
        dbglPrint("width mismatch was %d, but expected %d\n", w, width);
        dbglDumpImage2D("expected texture data:", pvImage, width, height, bpp, pitch);
        dbglDumpImage2D("stored texture data:", pvTexImg, w, h, bpp, texPitch);

        Assert(0);

        dbglFree(pvTexImg);
        return;
    }
    if (h != height)
    {
        dbglPrint("height mismatch was %d, but expected %d\n", h, height);
        dbglDumpImage2D("expected texture data:", pvImage, width, height, bpp, pitch);
        dbglDumpImage2D("stored texture data:", pvTexImg, w, h, bpp, texPitch);

        Assert(0);

        dbglFree(pvTexImg);
        return;
    }

    if (memcmp(pvImage, pvTexImg, w * h * 4))
    {
        dbglPrint("tex data mismatch\n");
        dbglDumpImage2D("expected texture data:", pvImage, width, height, bpp, pitch);
        dbglDumpImage2D("stored texture data:", pvTexImg, w, h, bpp, texPitch);

        Assert(0);
    }

    dbglFree(pvTexImg);
}

void dbglCheckTexUnits(const struct wined3d_gl_info *gl_info, const struct IWineD3DDeviceImpl *pDevice, BOOL fBreakIfCanNotMatch)
{
    int iStage;
    GLint ActiveTexUnit = 0;
    GLint CheckTexUnit = 0;
    DBGL_OP(GetIntegerv(GL_ACTIVE_TEXTURE, &ActiveTexUnit));

    Assert(ActiveTexUnit >= GL_TEXTURE0);
    Assert(ActiveTexUnit < GL_TEXTURE0 + RT_ELEMENTS(pDevice->stateBlock->textures));

    CheckTexUnit = ActiveTexUnit;

    for (iStage = 0; iStage < RT_ELEMENTS(pDevice->stateBlock->textures); ++iStage)
    {
        GLint curTex = 0;
        int iLevel;
        const IWineD3DTextureImpl *pTexture = (IWineD3DTextureImpl*)pDevice->stateBlock->textures[iStage];
        if (!pTexture) continue;

        Assert(!pTexture->baseTexture.is_srgb);

        for (iLevel = 0; iLevel < (int)pTexture->baseTexture.levels; ++iLevel)
        {
            IWineD3DSurfaceImpl *pSurf = (IWineD3DSurfaceImpl*)pTexture->surfaces[iLevel];
            Assert(pSurf);
            if (CheckTexUnit != iStage + GL_TEXTURE0)
            {
                DBGL_EXT_OP(ActiveTextureARB(iStage + GL_TEXTURE0));
                CheckTexUnit = iStage + GL_TEXTURE0;
            }

            Assert(pTexture->target == GL_TEXTURE_2D);

            DBGL_OP(GetIntegerv(GL_TEXTURE_BINDING_2D, &curTex));
            Assert(curTex);
            Assert(pSurf->texture_name);
            Assert(pSurf->texture_name == curTex);
            Assert(pSurf->texture_name == pTexture->baseTexture.texture_rgb.name);
            Assert(pSurf->texture_target == GL_TEXTURE_2D);
            Assert(pTexture->target == pSurf->texture_target);
            Assert(DBGL_OP(IsEnabled(pSurf->texture_target)));
            Assert(pSurf->Flags & SFLAG_INTEXTURE);
            if (pSurf->Flags & SFLAG_INSYSMEM && !pSurf->Flags & SFLAG_PBO)
            {
                Assert(pSurf->resource.allocatedMemory);
                /* we can match GPU & our state */
                dbglCmpTexImage2D(gl_info, "matching tex data state", pSurf->texture_target, pSurf->texture_name, iLevel, pSurf->resource.allocatedMemory,
                        pSurf->currentDesc.Width, pSurf->currentDesc.Height, pSurf->resource.format_desc->glFormat, pSurf->resource.format_desc->glType);
            }
            else
            {
                GLint w,h;
                if (fBreakIfCanNotMatch)
                {
                    GLvoid* pvImage;
                    dbglGetTexImage2D(gl_info, pSurf->texture_target, pSurf->texture_name, iLevel, &pvImage, &w, &h, GL_BGRA, GL_UNSIGNED_BYTE);
                    Assert(w == pSurf->currentDesc.Width);
                    Assert(h == pSurf->currentDesc.Height);
                    dbglDumpImage2D("matching texture data", pvImage, w, h, 32, (32 * w)/8);
                    Assert(0);
                    dbglFree(pvImage);
                }
                else
                {
                    /* just check width & height */
                    /* since we already asserted current texture binding, we can just quiry its parameters */
                    DBGL_OP(GetTexLevelParameteriv(pSurf->texture_target, iLevel, GL_TEXTURE_WIDTH, &w));
                    DBGL_OP(GetTexLevelParameteriv(pSurf->texture_target, iLevel, GL_TEXTURE_HEIGHT, &h));
                    Assert(w == pSurf->currentDesc.Width);
                    Assert(h == pSurf->currentDesc.Height);
                }
            }
        }
    }

    if (CheckTexUnit != ActiveTexUnit)
    {
        DBGL_EXT_OP(ActiveTextureARB(ActiveTexUnit));
    }
}
