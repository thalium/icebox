/* Copyright (c) 2001, Stanford University
   All rights reserved.

   See the file LICENSE.txt for information on redistributing this software. */

#include "cr_packfunctions.h"
#include "cr_glstate.h"
#include "cr_pixeldata.h"
#include "cr_version.h"
#include "packspu.h"
#include "packspu_proto.h"

static GLboolean packspu_CheckTexImageFormat(GLenum format)
{
    if (format!=GL_COLOR_INDEX
        && format!=GL_RED
        && format!=GL_GREEN
        && format!=GL_BLUE
        && format!=GL_ALPHA
        && format!=GL_RGB
        && format!=GL_BGR
        && format!=GL_RGBA
        && format!=GL_BGRA
        && format!=GL_LUMINANCE
        && format!=GL_LUMINANCE_ALPHA
        && format!=GL_DEPTH_COMPONENT
        && format!=GL_DEPTH_STENCIL)
    {
        /*crWarning("crPackCheckTexImageFormat FAILED format 0x%x isn't valid", format);*/
        return GL_FALSE;
    }

    return GL_TRUE;
}

static GLboolean packspu_CheckTexImageType(GLenum type)
{
    if (type!=GL_UNSIGNED_BYTE
        && type!=GL_BYTE
        && type!=GL_BITMAP
        && type!=GL_UNSIGNED_SHORT
        && type!=GL_SHORT
        && type!=GL_UNSIGNED_INT
        && type!=GL_INT
        && type!=GL_FLOAT
        && type!=GL_UNSIGNED_BYTE_3_3_2
        && type!=GL_UNSIGNED_BYTE_2_3_3_REV
        && type!=GL_UNSIGNED_SHORT_5_6_5
        && type!=GL_UNSIGNED_SHORT_5_6_5_REV
        && type!=GL_UNSIGNED_SHORT_4_4_4_4
        && type!=GL_UNSIGNED_SHORT_4_4_4_4_REV
        && type!=GL_UNSIGNED_SHORT_5_5_5_1
        && type!=GL_UNSIGNED_SHORT_1_5_5_5_REV
        && type!=GL_UNSIGNED_INT_8_8_8_8
        && type!=GL_UNSIGNED_INT_8_8_8_8_REV
        && type!=GL_UNSIGNED_INT_10_10_10_2
        && type!=GL_UNSIGNED_INT_2_10_10_10_REV
        && type!=GL_UNSIGNED_INT_24_8)
    {
        /*crWarning("crPackCheckTexImageType FAILED type 0x%x isn't valid", type);*/
        return GL_FALSE;
    }

    return GL_TRUE;
}

static GLboolean packspu_CheckTexImageInternalFormat(GLint internalformat)
{
    if (internalformat!=1
        && internalformat!=2
        && internalformat!=3
        && internalformat!=4
        && internalformat!=GL_ALPHA
        && internalformat!=GL_ALPHA4
        && internalformat!=GL_ALPHA8
        && internalformat!=GL_ALPHA12
        && internalformat!=GL_ALPHA16
        && internalformat!=GL_COMPRESSED_ALPHA
        && internalformat!=GL_COMPRESSED_LUMINANCE
        && internalformat!=GL_COMPRESSED_LUMINANCE_ALPHA
        && internalformat!=GL_COMPRESSED_INTENSITY
        && internalformat!=GL_COMPRESSED_RGB
        && internalformat!=GL_COMPRESSED_RGBA
        && internalformat!=GL_DEPTH_COMPONENT
        && internalformat!=GL_DEPTH_COMPONENT16
        && internalformat!=GL_DEPTH_COMPONENT24
        && internalformat!=GL_DEPTH_COMPONENT32
        && internalformat!=GL_DEPTH24_STENCIL8
        && internalformat!=GL_LUMINANCE
        && internalformat!=GL_LUMINANCE4
        && internalformat!=GL_LUMINANCE8
        && internalformat!=GL_LUMINANCE12
        && internalformat!=GL_LUMINANCE16
        && internalformat!=GL_LUMINANCE_ALPHA
        && internalformat!=GL_LUMINANCE4_ALPHA4
        && internalformat!=GL_LUMINANCE6_ALPHA2
        && internalformat!=GL_LUMINANCE8_ALPHA8
        && internalformat!=GL_LUMINANCE12_ALPHA4
        && internalformat!=GL_LUMINANCE12_ALPHA12
        && internalformat!=GL_LUMINANCE16_ALPHA16
        && internalformat!=GL_INTENSITY
        && internalformat!=GL_INTENSITY4
        && internalformat!=GL_INTENSITY8
        && internalformat!=GL_INTENSITY12
        && internalformat!=GL_INTENSITY16
        && internalformat!=GL_R3_G3_B2
        && internalformat!=GL_RGB
        && internalformat!=GL_RGB4
        && internalformat!=GL_RGB5
        && internalformat!=GL_RGB8
        && internalformat!=GL_RGB10
        && internalformat!=GL_RGB12
        && internalformat!=GL_RGB16
        && internalformat!=GL_RGBA
        && internalformat!=GL_RGBA2
        && internalformat!=GL_RGBA4
        && internalformat!=GL_RGB5_A1
        && internalformat!=GL_RGBA8
        && internalformat!=GL_RGB10_A2
        && internalformat!=GL_RGBA12
        && internalformat!=GL_RGBA16
        && internalformat!=GL_SLUMINANCE
        && internalformat!=GL_SLUMINANCE8
        && internalformat!=GL_SLUMINANCE_ALPHA
        && internalformat!=GL_SLUMINANCE8_ALPHA8
        && internalformat!=GL_SRGB
        && internalformat!=GL_SRGB8
        && internalformat!=GL_SRGB_ALPHA
        && internalformat!=GL_SRGB8_ALPHA8
#ifdef CR_EXT_texture_compression_s3tc
        && internalformat!=GL_COMPRESSED_RGB_S3TC_DXT1_EXT
        && internalformat!=GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
        && internalformat!=GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
        && internalformat!=GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
# ifdef CR_EXT_texture_sRGB
        && internalformat!=GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
        && internalformat!=GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
        && internalformat!=GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
        && internalformat!=GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
# endif
#endif
        /** @todo ARB_texture_float*/
        && internalformat!=GL_RGBA32F_ARB
        && internalformat!=GL_RGB32F_ARB
        && internalformat!=GL_ALPHA32F_ARB
        && internalformat!=GL_INTENSITY32F_ARB
        && internalformat!=GL_LUMINANCE32F_ARB
        && internalformat!=GL_LUMINANCE_ALPHA32F_ARB
        && internalformat!=GL_RGBA16F_ARB
        && internalformat!=GL_RGB16F_ARB
        && internalformat!=GL_ALPHA16F_ARB
        && internalformat!=GL_INTENSITY16F_ARB
        && internalformat!=GL_LUMINANCE16F_ARB
        && internalformat!=GL_LUMINANCE_ALPHA16F_ARB
        )
    {
        /*crWarning("crPackCheckTexImageInternalFormat FAILED internalformat 0x%x isn't valid", internalformat);*/
        return GL_FALSE;
    }

    return GL_TRUE;
}

static GLboolean packspu_CheckTexImageParams(GLint internalformat, GLenum format, GLenum type)
{
    return packspu_CheckTexImageFormat(format)
           && packspu_CheckTexImageType(type)
           && packspu_CheckTexImageInternalFormat(internalformat);
}

static GLboolean packspu_CheckTexImageFormatType(GLenum format, GLenum type)
{
    return packspu_CheckTexImageFormat(format)
           && packspu_CheckTexImageType(type);
}

static const CRPixelPackState _defaultPacking = {
   0, /* rowLength */
   0, /* skipRows */
   0, /* skipPixels */
   1, /* alignment */
   0, /* imageHeight */
   0, /* skipImages */
   GL_FALSE, /* swapBytes */
   GL_FALSE, /* psLSBFirst */
};

#define APPLY_IF_NEQ(state, field, enum)        \
    if (state.field != _defaultPacking.field)   \
    {                                           \
        crPackPixelStorei(enum, state.field);   \
    }

#define RESTORE_IF_NEQ(state, field, enum)              \
    if (state.field != _defaultPacking.field)           \
    {                                                   \
        crPackPixelStorei(enum, _defaultPacking.field); \
    }

static void packspu_ApplyUnpackState(void)
{
    GET_THREAD(thread);
    ContextInfo *ctx = thread->currentContext;
    CRClientState *clientState = &(ctx->clientState->client);

    APPLY_IF_NEQ(clientState->unpack, rowLength, GL_UNPACK_ROW_LENGTH);
    APPLY_IF_NEQ(clientState->unpack, skipRows, GL_UNPACK_SKIP_ROWS);
    APPLY_IF_NEQ(clientState->unpack, skipPixels, GL_UNPACK_SKIP_PIXELS);
    APPLY_IF_NEQ(clientState->unpack, alignment, GL_UNPACK_ALIGNMENT);
    APPLY_IF_NEQ(clientState->unpack, imageHeight, GL_UNPACK_IMAGE_HEIGHT);
    APPLY_IF_NEQ(clientState->unpack, skipImages, GL_UNPACK_SKIP_IMAGES);
    APPLY_IF_NEQ(clientState->unpack, swapBytes, GL_UNPACK_SWAP_BYTES);
    APPLY_IF_NEQ(clientState->unpack, psLSBFirst, GL_UNPACK_LSB_FIRST);
}

static void packspu_RestoreUnpackState(void)
{
    GET_THREAD(thread);
    ContextInfo *ctx = thread->currentContext;
    CRClientState *clientState = &(ctx->clientState->client);

    RESTORE_IF_NEQ(clientState->unpack, rowLength, GL_UNPACK_ROW_LENGTH);
    RESTORE_IF_NEQ(clientState->unpack, skipRows, GL_UNPACK_SKIP_ROWS);
    RESTORE_IF_NEQ(clientState->unpack, skipPixels, GL_UNPACK_SKIP_PIXELS);
    RESTORE_IF_NEQ(clientState->unpack, alignment, GL_UNPACK_ALIGNMENT);
    RESTORE_IF_NEQ(clientState->unpack, imageHeight, GL_UNPACK_IMAGE_HEIGHT);
    RESTORE_IF_NEQ(clientState->unpack, skipImages, GL_UNPACK_SKIP_IMAGES);
    RESTORE_IF_NEQ(clientState->unpack, swapBytes, GL_UNPACK_SWAP_BYTES);
    RESTORE_IF_NEQ(clientState->unpack, psLSBFirst, GL_UNPACK_LSB_FIRST);
}

static void packspu_ApplyPackState(void)
{
    GET_THREAD(thread);
    ContextInfo *ctx = thread->currentContext;
    CRClientState *clientState = &(ctx->clientState->client);

    APPLY_IF_NEQ(clientState->pack, rowLength, GL_PACK_ROW_LENGTH);
    APPLY_IF_NEQ(clientState->pack, skipRows, GL_PACK_SKIP_ROWS);
    APPLY_IF_NEQ(clientState->pack, skipPixels, GL_PACK_SKIP_PIXELS);
    APPLY_IF_NEQ(clientState->pack, alignment, GL_PACK_ALIGNMENT);
    APPLY_IF_NEQ(clientState->pack, imageHeight, GL_PACK_IMAGE_HEIGHT);
    APPLY_IF_NEQ(clientState->pack, skipImages, GL_PACK_SKIP_IMAGES);
    APPLY_IF_NEQ(clientState->pack, swapBytes, GL_PACK_SWAP_BYTES);
    APPLY_IF_NEQ(clientState->pack, psLSBFirst, GL_PACK_LSB_FIRST);
}

static void packspu_RestorePackState(void)
{
    GET_THREAD(thread);
    ContextInfo *ctx = thread->currentContext;
    CRClientState *clientState = &(ctx->clientState->client);

    RESTORE_IF_NEQ(clientState->pack, rowLength, GL_PACK_ROW_LENGTH);
    RESTORE_IF_NEQ(clientState->pack, skipRows, GL_PACK_SKIP_ROWS);
    RESTORE_IF_NEQ(clientState->pack, skipPixels, GL_PACK_SKIP_PIXELS);
    RESTORE_IF_NEQ(clientState->pack, alignment, GL_PACK_ALIGNMENT);
    RESTORE_IF_NEQ(clientState->pack, imageHeight, GL_PACK_IMAGE_HEIGHT);
    RESTORE_IF_NEQ(clientState->pack, skipImages, GL_PACK_SKIP_IMAGES);
    RESTORE_IF_NEQ(clientState->pack, swapBytes, GL_PACK_SWAP_BYTES);
    RESTORE_IF_NEQ(clientState->pack, psLSBFirst, GL_PACK_LSB_FIRST);
}

void PACKSPU_APIENTRY packspu_PixelStoref( GLenum pname, GLfloat param )
{
    /* NOTE: we do not send pixel store parameters to the server!
     * When we pack a glDrawPixels or glTexImage2D image we interpret
     * the user's pixel store parameters at that time and pack the
     * image in a canonical layout (see util/pixel.c).
     */
    crStatePixelStoref(&pack_spu.StateTracker, pname, param );
}

void PACKSPU_APIENTRY packspu_PixelStorei( GLenum pname, GLint param )
{
    crStatePixelStorei(&pack_spu.StateTracker, pname, param );
}

void PACKSPU_APIENTRY packspu_DrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels )
{
    GET_THREAD(thread);
    ContextInfo *ctx = thread->currentContext;
    CRClientState *clientState = &(ctx->clientState->client);

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackDrawPixels( width, height, format, type, pixels, &(clientState->unpack) );

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}

void PACKSPU_APIENTRY packspu_ReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels )
{
    GET_THREAD(thread);
    ContextInfo *ctx = thread->currentContext;
    CRClientState *clientState = &(ctx->clientState->client);
    int writeback;

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
    {
        packspu_ApplyPackState();
    }

    crPackReadPixels(x, y, width, height, format, type, pixels, &(clientState->pack), &writeback);

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
    {
        packspu_RestorePackState();
    }

#ifdef CR_ARB_pixel_buffer_object
    if (!crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
#endif
    {
        pack_spu.ReadPixels++;

        packspuFlush((void *) thread);
        CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
    }
}

/** @todo check with pbo's*/
void PACKSPU_APIENTRY packspu_CopyPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type )
{
    GET_THREAD(thread);
    crPackCopyPixels( x, y, width, height, type );
    /* XXX why flush here? */
    packspuFlush( (void *) thread );
}

void PACKSPU_APIENTRY packspu_Bitmap( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap )
{
    GET_CONTEXT(ctx);
    CRClientState *clientState = &(ctx->clientState->client);

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackBitmap(width, height, xorig, yorig, xmove, ymove, bitmap, &(clientState->unpack));

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}

void PACKSPU_APIENTRY packspu_TexImage1D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels )
{
    GET_CONTEXT(ctx);
    CRClientState *clientState = &(ctx->clientState->client);

    if (!packspu_CheckTexImageParams(internalformat, format, type))
    {
        if (pixels || crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
        {
            crWarning("packspu_TexImage1D invalid internalFormat(%x)/format(%x)/type(%x)", internalformat, format, type);
            return;
        }
        internalformat = packspu_CheckTexImageInternalFormat(internalformat) ? internalformat:GL_RGBA;
        format = packspu_CheckTexImageFormat(format) ? format:GL_RGBA;
        type = packspu_CheckTexImageType(type) ? type:GL_UNSIGNED_BYTE;
    }

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackTexImage1D( target, level, internalformat, width, border, format, type, pixels, &(clientState->unpack) );
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}

void PACKSPU_APIENTRY packspu_TexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels )
{
    GET_CONTEXT(ctx);
    CRClientState *clientState = &(ctx->clientState->client);

    if (!packspu_CheckTexImageParams(internalformat, format, type))
    {
        if (pixels || crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
        {
            crWarning("packspu_TexImage2D invalid internalFormat(%x)/format(%x)/type(%x)", internalformat, format, type);
            return;
        }
        internalformat = packspu_CheckTexImageInternalFormat(internalformat) ? internalformat:GL_RGBA;
        format = packspu_CheckTexImageFormat(format) ? format:GL_RGBA;
        type = packspu_CheckTexImageType(type) ? type:GL_UNSIGNED_BYTE;
    }

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackTexImage2D( target, level, internalformat, width, height, border, format, type, pixels, &(clientState->unpack) );
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}

#ifdef GL_EXT_texture3D
void PACKSPU_APIENTRY packspu_TexImage3DEXT( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels )
{
    GET_CONTEXT(ctx);
    CRClientState *clientState = &(ctx->clientState->client);

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackTexImage3DEXT( target, level, internalformat, width, height, depth, border, format, type, pixels, &(clientState->unpack) );
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}
#endif

#ifdef CR_OPENGL_VERSION_1_2
void PACKSPU_APIENTRY packspu_TexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels )
{
    GET_CONTEXT(ctx);
    CRClientState *clientState = &(ctx->clientState->client);

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackTexImage3D( target, level, internalformat, width, height, depth, border, format, type, pixels, &(clientState->unpack) );
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}
#endif /* CR_OPENGL_VERSION_1_2 */

void PACKSPU_APIENTRY packspu_TexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels )
{
    GET_CONTEXT(ctx);
    CRClientState *clientState = &(ctx->clientState->client);

    if (!packspu_CheckTexImageFormatType(format, type))
    {
        crWarning("packspu_TexSubImage1D invalid format(%x)/type(%x)", format, type);
        return;
    }

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackTexSubImage1D( target, level, xoffset, width, format, type, pixels, &(clientState->unpack) );
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}

void PACKSPU_APIENTRY packspu_TexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels )
{
    GET_CONTEXT(ctx);
    CRClientState *clientState = &(ctx->clientState->client);

    if (!packspu_CheckTexImageFormatType(format, type))
    {
        crWarning("packspu_TexSubImage2D invalid format(%x)/type(%x)", format, type);
        return;
    }

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels, &(clientState->unpack) );
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}

#ifdef CR_OPENGL_VERSION_1_2
void PACKSPU_APIENTRY packspu_TexSubImage3D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels )
{
    GET_CONTEXT(ctx);
    CRClientState *clientState = &(ctx->clientState->client);

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackTexSubImage3D( target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels, &(clientState->unpack) );
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}
#endif /* CR_OPENGL_VERSION_1_2 */

void PACKSPU_APIENTRY packspu_ZPixCR( GLsizei width, GLsizei height, GLenum format, GLenum type, GLenum ztype, GLint zparm, GLint length, const GLvoid *pixels )
{
    GET_CONTEXT(ctx);
    CRClientState *clientState = &(ctx->clientState->client);
    crPackZPixCR( width, height, format, type, ztype, zparm, length, pixels, &(clientState->unpack) );
}

void PACKSPU_APIENTRY packspu_GetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
    GET_THREAD(thread);
    ContextInfo *ctx = thread->currentContext;
    CRClientState *clientState = &(ctx->clientState->client);
    int writeback = 1;

    /* XXX note: we're not observing the pixel pack parameters here unless PACK PBO is bound
     * To do so, we'd have to allocate a temporary image buffer (how large???)
     * and copy the image to the user's buffer using the pixel pack params.
     */

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
    {
        packspu_ApplyPackState();
    }

    crPackGetTexImage( target, level, format, type, pixels, &(clientState->pack), &writeback );
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
    {
        packspu_RestorePackState();
    }

#ifdef CR_ARB_pixel_buffer_object
    if (!crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
#endif
    {
        packspuFlush( (void *) thread );
        CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
    }
}

void PACKSPU_APIENTRY packspu_GetCompressedTexImageARB( GLenum target, GLint level, GLvoid * img )
{
    GET_THREAD(thread);
    int writeback = 1;

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
    {
        packspu_ApplyPackState();
    }

    crPackGetCompressedTexImageARB( target, level, img, &writeback );
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
    {
        packspu_RestorePackState();
    }

#ifdef CR_ARB_pixel_buffer_object
    if (!crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
#endif
    {
        packspuFlush( (void *) thread );
        CRPACKSPU_WRITEBACK_WAIT(thread, writeback);
    }
}

void PACKSPU_APIENTRY
packspu_CompressedTexImage1DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                GLint border, GLsizei imagesize, const GLvoid *data)
{
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackCompressedTexImage1DARB(target, level, internalformat, width, border, imagesize, data);

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}

void PACKSPU_APIENTRY
packspu_CompressedTexImage2DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                GLsizei height, GLint border, GLsizei imagesize, const GLvoid *data)
{
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackCompressedTexImage2DARB(target, level, internalformat, width, height, border, imagesize, data);

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}

void PACKSPU_APIENTRY
packspu_CompressedTexImage3DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                GLsizei height, GLsizei depth, GLint border, GLsizei imagesize, const GLvoid *data)
{
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackCompressedTexImage3DARB(target, level, internalformat, width, height, depth, border, imagesize, data);

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}

void PACKSPU_APIENTRY
packspu_CompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset, GLsizei width,
                                   GLenum format, GLsizei imagesize, const GLvoid *data)
{
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackCompressedTexSubImage1DARB(target, level, xoffset, width, format, imagesize, data);

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}

void PACKSPU_APIENTRY
packspu_CompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                   GLsizei width, GLsizei height, GLenum format, GLsizei imagesize, const GLvoid *data)
{
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackCompressedTexSubImage2DARB(target, level, xoffset, yoffset, width, height, format, imagesize, data);

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}

void PACKSPU_APIENTRY
packspu_CompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                   GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format,
                                   GLsizei imagesize, const GLvoid *data)
{
    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_ApplyUnpackState();
    }

    crPackCompressedTexSubImage3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imagesize, data);

    if (crStateIsBufferBound(&pack_spu.StateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
    {
        packspu_RestoreUnpackState();
    }
}
