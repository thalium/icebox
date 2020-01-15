/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_error.h"
#include "cr_pixeldata.h"
#include "cr_protocol.h"
#include "cr_mem.h"
#include "cr_version.h"
 
#if defined( GL_EXT_texture3D ) 
void crUnpackTexImage3DEXT(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 36, int);

    GLenum target = READ_DATA(pState,  sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA(pState,  sizeof( int ) + 4, GLint );
    GLenum internalformat = READ_DATA(pState,  sizeof( int ) + 8, GLint );
    GLsizei width = READ_DATA(pState,  sizeof( int ) + 12, GLsizei );
    GLsizei height = READ_DATA(pState,  sizeof( int ) + 16, GLsizei );
    GLsizei depth = READ_DATA(pState,  sizeof( int ) + 20, GLsizei );
    GLint border = READ_DATA(pState,  sizeof( int ) + 24, GLint );
    GLenum format = READ_DATA(pState,  sizeof( int ) + 28, GLenum );
    GLenum type = READ_DATA(pState,  sizeof( int ) + 32, GLenum );
    int noimagedata = READ_DATA(pState,  sizeof( int ) + 36, int );
    GLvoid *pixels;

    /*If there's no imagedata send, it's either that passed pointer was NULL or
      there was GL_PIXEL_UNPACK_BUFFER_ARB bound, in both cases 4bytes of passed
      pointer would convert to either NULL or offset in the bound buffer.
     */
    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof(int)+40, GLint);
    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(pState, sizeof(int)+40, GLint);
    else
    {
        size_t cbImg = crTextureSize(format, type, width, height, depth);
        if (RT_UNLIKELY(cbImg == 0))
        {
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
        }
        pixels = DATA_POINTER(pState,  sizeof( int ) + 44, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, cbImg, GLubyte);
    }

    pState->pDispatchTbl->TexImage3DEXT(target, level, internalformat, width,
                                    height, depth, border, format, type,
                                    pixels);
    INCR_VAR_PTR(pState);
}
#endif /* GL_EXT_texture3D */

#if defined( CR_OPENGL_VERSION_1_2 )
void crUnpackTexImage3D(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 36, int);

    GLenum target = READ_DATA(pState,  sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA(pState,  sizeof( int ) + 4, GLint );
    GLint internalformat = READ_DATA(pState,  sizeof( int ) + 8, GLint );
    GLsizei width = READ_DATA(pState,  sizeof( int ) + 12, GLsizei );
    GLsizei height = READ_DATA(pState,  sizeof( int ) + 16, GLsizei );
    GLsizei depth = READ_DATA(pState,  sizeof( int ) + 20, GLsizei );
    GLint border = READ_DATA(pState,  sizeof( int ) + 24, GLint );
    GLenum format = READ_DATA(pState,  sizeof( int ) + 28, GLenum );
    GLenum type = READ_DATA(pState,  sizeof( int ) + 32, GLenum );
    int noimagedata = READ_DATA(pState,  sizeof( int ) + 36, int );
    GLvoid *pixels;

    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof(int)+40, GLint);
    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(pState, sizeof(int)+40, GLint);
    else
    {
        size_t cbImg = crTextureSize(format, type, width, height, depth);
        if (RT_UNLIKELY(cbImg == 0))
        {
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
        }
        pixels = DATA_POINTER(pState,  sizeof( int ) + 44, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, cbImg, GLubyte);
    }

    pState->pDispatchTbl->TexImage3D( target, level, internalformat, width, height,
                                  depth, border, format, type, pixels );
    INCR_VAR_PTR(pState);
}
#endif /* CR_OPENGL_VERSION_1_2 */

void crUnpackTexImage2D(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 32, int);

    GLenum target = READ_DATA(pState,  sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA(pState,  sizeof( int ) + 4, GLint );
    GLint internalformat = READ_DATA(pState,  sizeof( int ) + 8, GLint );
    GLsizei width = READ_DATA(pState, sizeof( int ) + 12, GLsizei );
    GLsizei height = READ_DATA(pState, sizeof( int ) + 16, GLsizei );
    GLint border = READ_DATA(pState, sizeof( int ) + 20, GLint );
    GLenum format = READ_DATA(pState, sizeof( int ) + 24, GLenum );
    GLenum type = READ_DATA(pState, sizeof( int ) + 28, GLenum );
    int noimagedata = READ_DATA(pState, sizeof( int ) + 32, int );
    GLvoid *pixels;

    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof(int)+36, GLint);
    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(pState, sizeof(int)+36, GLint);
    else
    {
        size_t cbImg = crImageSize(format, type, width, height);
        if (RT_UNLIKELY(cbImg == 0))
        {
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
        }
        pixels = DATA_POINTER(pState,  sizeof( int ) + 40, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, cbImg, GLubyte);
    }

    pState->pDispatchTbl->TexImage2D( target, level, internalformat, width, height,
                          border, format, type, pixels );
    INCR_VAR_PTR(pState);
}

void crUnpackTexImage1D(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 28, int);

    GLenum target = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA(pState, sizeof( int ) + 4, GLint );
    GLint internalformat = READ_DATA(pState, sizeof( int ) + 8, GLint );
    GLsizei width = READ_DATA(pState, sizeof( int ) + 12, GLsizei );
    GLint border = READ_DATA(pState, sizeof( int ) + 16, GLint );
    GLenum format = READ_DATA(pState, sizeof( int ) + 20, GLenum );
    GLenum type = READ_DATA(pState, sizeof( int ) + 24, GLenum );
    int noimagedata = READ_DATA(pState, sizeof( int ) + 28, int );
    GLvoid *pixels;

    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof(int)+32, GLint);
    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(pState, sizeof(int)+32, GLint);
    else
    {
        size_t cbImg = crImageSize(format, type, width, 1);
        if (RT_UNLIKELY(cbImg == 0))
        {
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
        }
        pixels = DATA_POINTER(pState,  sizeof( int ) + 36, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, cbImg, GLubyte);
    }

    pState->pDispatchTbl->TexImage1D(target, level, internalformat, width, border,
                                     format, type, pixels );
    INCR_VAR_PTR(pState);
}

void crUnpackDeleteTextures(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 0, GLsizei);

    GLsizei n = READ_DATA(pState, sizeof( int ) + 0, GLsizei );
    GLuint *textures = DATA_POINTER(pState, sizeof( int ) + 4, GLuint );

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, textures, n, GLuint);
    pState->pDispatchTbl->DeleteTextures( n, textures );
    INCR_VAR_PTR(pState);
}


void crUnpackPrioritizeTextures(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 0, GLsizei);

    GLsizei n = READ_DATA(pState, sizeof( int ) + 0, GLsizei );
    GLuint *textures = DATA_POINTER(pState, sizeof( int ) + 4, GLuint );
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, textures, n, GLuint);

    GLclampf *priorities = DATA_POINTER(pState, sizeof( int ) + 4 + n*sizeof( GLuint ),
                                         GLclampf );
    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, priorities, n, GLclampf);

    pState->pDispatchTbl->PrioritizeTextures( n, textures, priorities );
    INCR_VAR_PTR(pState);
}

static size_t crGetTexParameterParamsFromPname(GLenum pname)
{
    int cParams = 0;

    switch (pname)
    {
        case GL_TEXTURE_MIN_FILTER:
        case GL_TEXTURE_MAG_FILTER:
        case GL_TEXTURE_WRAP_R:
        case GL_TEXTURE_WRAP_S:
        case GL_TEXTURE_WRAP_T:
        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
        case GL_TEXTURE_MIN_LOD:
        case GL_TEXTURE_MAX_LOD:
        case GL_TEXTURE_BASE_LEVEL:
        case GL_TEXTURE_MAX_LEVEL:
#ifdef CR_ARB_shadow
        case GL_TEXTURE_COMPARE_MODE_ARB:
        case GL_TEXTURE_COMPARE_FUNC_ARB:
#endif
#ifdef GL_TEXTURE_PRIORITY
        case GL_TEXTURE_PRIORITY:
#endif
#ifdef CR_ARB_shadow_ambient
        case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
#endif
#ifdef CR_ARB_depth_texture
        case GL_DEPTH_TEXTURE_MODE_ARB:
#endif
#ifdef CR_SGIS_generate_mipmap
        case GL_GENERATE_MIPMAP_SGIS:
#endif
            cParams = 1;
            break;
        case GL_TEXTURE_BORDER_COLOR:
            cParams = 4;
            break;
        default:
            break; /* Error, handled in caller. */
    }

    return cParams;
}

void crUnpackTexParameterfv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLenum);

    GLenum target = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLfloat *params = DATA_POINTER(pState, sizeof( int ) + 8, GLfloat );

    size_t cParams = crGetTexParameterParamsFromPname(pname);
    if (RT_UNLIKELY(cParams == 0))
    {
        pState->rcUnpack = VERR_INVALID_PARAMETER;
        return;
    }

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, cParams, GLfloat);
    pState->pDispatchTbl->TexParameterfv( target, pname, params );
    INCR_VAR_PTR(pState);
}

void crUnpackTexParameteriv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLenum);

    GLenum target = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLint *params = DATA_POINTER(pState, sizeof( int ) + 8, GLint );

    size_t cParams = crGetTexParameterParamsFromPname(pname);
    if (RT_UNLIKELY(cParams == 0))
    {
        pState->rcUnpack = VERR_INVALID_PARAMETER;
        return;
    }

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, cParams, GLint);
    pState->pDispatchTbl->TexParameteriv( target, pname, params );
    INCR_VAR_PTR(pState);
}

void crUnpackTexParameterf(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 8, GLfloat);

    GLenum target = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLfloat param = READ_DATA(pState, sizeof( int ) + 8, GLfloat );

    pState->pDispatchTbl->TexParameterf( target, pname, param );
    INCR_VAR_PTR(pState);
}

void crUnpackTexParameteri(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 8, GLint);

    GLenum target = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLint param = READ_DATA(pState, sizeof( int ) + 8, GLint );

    pState->pDispatchTbl->TexParameteri( target, pname, param );
    INCR_VAR_PTR(pState);
}

#if defined(CR_OPENGL_VERSION_1_2)
void crUnpackTexSubImage3D(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 40, int);

    GLenum target = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA(pState, sizeof( int ) + 4, GLint );
    GLint xoffset = READ_DATA(pState, sizeof( int ) + 8, GLint );
    GLint yoffset = READ_DATA(pState, sizeof( int ) + 12, GLint );
    GLint zoffset = READ_DATA(pState, sizeof( int ) + 16, GLint );
    GLsizei width = READ_DATA(pState, sizeof( int ) + 20, GLsizei );
    GLsizei height = READ_DATA(pState, sizeof( int ) + 24, GLsizei );
    GLsizei depth = READ_DATA(pState, sizeof( int ) + 28, GLsizei );
    GLenum format = READ_DATA(pState, sizeof( int ) + 32, GLenum );
    GLenum type = READ_DATA(pState, sizeof( int ) + 36, GLenum );
    int noimagedata = READ_DATA(pState, sizeof( int ) + 40, int );
    GLvoid *pixels;

    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof(int)+44, GLint);
    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(pState, sizeof(int)+44, GLint);
    else
    {
        /*
         * Specifying a sub texture with zero width, height or depth is not an
         * error but has no effect.
         * See: https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glTexSubImage3D.xml
         */
        size_t cbImg = crTextureSize(format, type, width, height, depth);
        if (RT_UNLIKELY(   cbImg == 0
                        && width != 0
                        && height != 0
                        && depth != 0))
        {
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
        }
        pixels = DATA_POINTER(pState, sizeof( int ) + 48, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, cbImg, GLubyte);
    }

    pState->pDispatchTbl->PixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    pState->pDispatchTbl->TexSubImage3D(target, level, xoffset, yoffset, zoffset,
                                    width, height, depth, format, type, pixels);
    INCR_VAR_PTR(pState);
}
#endif /* CR_OPENGL_VERSION_1_2 */

void crUnpackTexSubImage2D(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 32, int);

    GLenum target = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA(pState, sizeof( int ) + 4, GLint );
    GLint xoffset = READ_DATA(pState, sizeof( int ) + 8, GLint );
    GLint yoffset = READ_DATA(pState, sizeof( int ) + 12, GLint );
    GLsizei width = READ_DATA(pState, sizeof( int ) + 16, GLsizei );
    GLsizei height = READ_DATA(pState, sizeof( int ) + 20, GLsizei );
    GLenum format = READ_DATA(pState, sizeof( int ) + 24, GLenum );
    GLenum type = READ_DATA(pState, sizeof( int ) + 28, GLenum );
    int noimagedata = READ_DATA(pState, sizeof( int ) + 32, int );
    GLvoid *pixels;

    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof(int)+36, GLint);
    if ( noimagedata )
    {
        CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof(int)+36, GLint);
        pixels = (void*) (uintptr_t) READ_DATA(pState, sizeof(int)+36, GLint);
    }
    else
    {
        /*
         * Specifying a sub texture with zero width, height or depth is not an
         * error but has no effect.
         * See: https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glTexSubImage2D.xml
         */
        size_t cbImg = crImageSize(format, type, width, height);
        if (RT_UNLIKELY(   cbImg == 0
                        && width != 0
                        && height != 0))
        {
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
        }
        pixels = DATA_POINTER(pState, sizeof( int ) + 40, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, cbImg, GLubyte);
    }

    pState->pDispatchTbl->PixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    pState->pDispatchTbl->TexSubImage2D( target, level, xoffset, yoffset, width,
                                     height, format, type, pixels );
    INCR_VAR_PTR(pState);
}

void crUnpackTexSubImage1D(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 24, int);

    GLenum target = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA(pState, sizeof( int ) + 4, GLint );
    GLint xoffset = READ_DATA(pState, sizeof( int ) + 8, GLint );
    GLsizei width = READ_DATA(pState, sizeof( int ) + 12, GLsizei );
    GLenum format = READ_DATA(pState, sizeof( int ) + 16, GLenum );
    GLenum type = READ_DATA(pState, sizeof( int ) + 20, GLenum );
    int noimagedata = READ_DATA(pState, sizeof( int ) + 24, int );
    GLvoid *pixels;

    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof(int)+28, GLint);
    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(pState, sizeof(int)+28, GLint);
    else
    {
        /*
         * Specifying a sub texture with zero width, height or depth is not an
         * error but has no effect.
         * See: https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glTexSubImage1D.xml
         */
        size_t cbImg = crImageSize(format, type, width, 1);
        if (RT_UNLIKELY(   cbImg == 0
                        && width != 0))
        {
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
        }
        pixels = DATA_POINTER(pState, sizeof( int ) + 32, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, cbImg, GLubyte);
    }

    pState->pDispatchTbl->PixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    pState->pDispatchTbl->TexSubImage1D( target, level, xoffset, width, format,
                                     type, pixels );
    INCR_VAR_PTR(pState);
}


void crUnpackTexEnvfv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLenum);

    GLenum target = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLfloat *params = DATA_POINTER(pState, sizeof( int ) + 8, GLfloat );

    size_t cParams = 1;
    if (pname == GL_TEXTURE_ENV_COLOR)
        cParams = 4;

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, cParams, GLfloat);
    pState->pDispatchTbl->TexEnvfv( target, pname, params );
    INCR_VAR_PTR(pState);
}

void crUnpackTexEnviv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLenum);

    GLenum target = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLint *params = DATA_POINTER(pState, sizeof( int ) + 8, GLint );

    size_t cParams = 1;
    if (pname == GL_TEXTURE_ENV_COLOR)
        cParams = 4;

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, cParams, GLint);
    pState->pDispatchTbl->TexEnviv( target, pname, params );
    INCR_VAR_PTR(pState);
}

void crUnpackTexGendv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLenum);

    GLenum coord = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLdouble *params = DATA_POINTER(pState, sizeof( int ) + 8, GLdouble );

    size_t cParams = 1;
    if (pname == GL_OBJECT_PLANE || pname == GL_EYE_PLANE)
        cParams = 4;

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, cParams, GLdouble);
    pState->pDispatchTbl->TexGendv( coord, pname, params );
    INCR_VAR_PTR(pState);
}

void crUnpackTexGenfv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLenum);

    GLenum coord = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLfloat *params = DATA_POINTER(pState, sizeof( int ) + 8, GLfloat );

    size_t cParams = 1;
    if (pname == GL_OBJECT_PLANE || pname == GL_EYE_PLANE)
        cParams = 4;

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, cParams, GLfloat);
    pState->pDispatchTbl->TexGenfv( coord, pname, params );
    INCR_VAR_PTR(pState);
}

void crUnpackTexGeniv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 4, GLenum);

    GLenum coord = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA(pState, sizeof( int ) + 4, GLenum );
    GLint *params = DATA_POINTER(pState, sizeof( int ) + 8, GLint );

    size_t cParams = 1;
    if (pname == GL_OBJECT_PLANE || pname == GL_EYE_PLANE)
        cParams = 4;

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, params, cParams, GLint);
    pState->pDispatchTbl->TexGeniv( coord, pname, params );
    INCR_VAR_PTR(pState);
}

void crUnpackExtendAreTexturesResident(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 8, GLsizei);

    GLsizei n = READ_DATA(pState, 8, GLsizei );
    const GLuint *textures = DATA_POINTER(pState, 12, const GLuint );

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, textures, n, GLuint);
    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, 20 + n * sizeof(GLuint), CRNetworkPointer);
    SET_RETURN_PTR(pState, 12 + n * sizeof(GLuint));
    SET_WRITEBACK_PTR(pState, 20 + n * sizeof(GLuint));
    (void) pState->pDispatchTbl->AreTexturesResident( n, textures, NULL );
}


void crUnpackExtendCompressedTexImage3DARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 4 + sizeof(int) + 32, int);

    GLenum  target         = READ_DATA(pState, 4 + sizeof(int) +  0, GLenum );
    GLint   level          = READ_DATA(pState, 4 + sizeof(int) +  4, GLint );
    GLenum  internalformat = READ_DATA(pState, 4 + sizeof(int) +  8, GLenum );
    GLsizei width          = READ_DATA(pState, 4 + sizeof(int) + 12, GLsizei );
    GLsizei height         = READ_DATA(pState, 4 + sizeof(int) + 16, GLsizei );
    GLsizei depth          = READ_DATA(pState, 4 + sizeof(int) + 20, GLsizei );
    GLint   border         = READ_DATA(pState, 4 + sizeof(int) + 24, GLint );
    GLsizei imagesize      = READ_DATA(pState, 4 + sizeof(int) + 28, GLsizei );
    int     noimagedata        = READ_DATA(pState, 4 + sizeof(int) + 32, int );
    GLvoid  *pixels;

    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, 4 + sizeof(int)+36, GLint);
    if( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(pState, 4+sizeof(int)+36, GLint);
    else
    {
        pixels = DATA_POINTER(pState, 4 + sizeof( int ) + 40, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, imagesize, GLubyte);
    }

    pState->pDispatchTbl->CompressedTexImage3DARB(target, level, internalformat,
                                              width, height, depth, border,
                                              imagesize, pixels);
}


void crUnpackExtendCompressedTexImage2DARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 4 + sizeof(int) + 28, int);

    GLenum target =         READ_DATA(pState, 4 + sizeof( int ) + 0, GLenum );
    GLint level =           READ_DATA(pState, 4 + sizeof( int ) + 4, GLint );
    GLenum internalformat = READ_DATA(pState, 4 + sizeof( int ) + 8, GLenum );
    GLsizei width =         READ_DATA(pState, 4 + sizeof( int ) + 12, GLsizei );
    GLsizei height =        READ_DATA(pState, 4 + sizeof( int ) + 16, GLsizei );
    GLint border =          READ_DATA(pState, 4 + sizeof( int ) + 20, GLint );
    GLsizei imagesize =     READ_DATA(pState, 4 + sizeof( int ) + 24, GLsizei );
    int noimagedata =           READ_DATA(pState, 4 + sizeof( int ) + 28, int );
    GLvoid *pixels;

    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, 4 + sizeof(int)+32, GLint);
    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(pState, 4+sizeof(int)+32, GLint);
    else
    {
        pixels = DATA_POINTER(pState, 4 + sizeof( int ) + 36, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, imagesize, GLubyte);
    }

    pState->pDispatchTbl->CompressedTexImage2DARB( target, level, internalformat,
                                               width, height, border, imagesize,
                                               pixels );
}


void crUnpackExtendCompressedTexImage1DARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 4 + sizeof(int) + 24, int);

    GLenum  target         = READ_DATA(pState, 4 + sizeof(int) +  0, GLenum );
    GLint   level          = READ_DATA(pState, 4 + sizeof(int) +  4, GLint );
    GLenum  internalformat = READ_DATA(pState, 4 + sizeof(int) +  8, GLenum );
    GLsizei width          = READ_DATA(pState, 4 + sizeof(int) + 12, GLsizei );
    GLint   border         = READ_DATA(pState, 4 + sizeof(int) + 16, GLint );
    GLsizei imagesize      = READ_DATA(pState, 4 + sizeof(int) + 20, GLsizei );
    int     noimagedata        = READ_DATA(pState, 4 + sizeof(int) + 24, int );
    GLvoid  *pixels;

    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, 4 + sizeof(int)+28, GLint);
    if( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(pState, 4+sizeof(int)+28, GLint);
    else
    {
        pixels = DATA_POINTER(pState, 4 + sizeof( int ) + 32, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, imagesize, GLubyte);
    }

    pState->pDispatchTbl->CompressedTexImage1DARB(target, level, internalformat,
                                              width, border, imagesize, pixels);
}


void crUnpackExtendCompressedTexSubImage3DARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 4 + sizeof(int) + 40, int);

    GLenum  target    = READ_DATA(pState, 4 + sizeof(int) +  0, GLenum );
    GLint   level     = READ_DATA(pState, 4 + sizeof(int) +  4, GLint );
    GLint   xoffset   = READ_DATA(pState, 4 + sizeof(int) +  8, GLint );
    GLint   yoffset   = READ_DATA(pState, 4 + sizeof(int) + 12, GLint );
    GLint   zoffset   = READ_DATA(pState, 4 + sizeof(int) + 16, GLint );
    GLsizei width     = READ_DATA(pState, 4 + sizeof(int) + 20, GLsizei );
    GLsizei height    = READ_DATA(pState, 4 + sizeof(int) + 24, GLsizei );
    GLsizei depth     = READ_DATA(pState, 4 + sizeof(int) + 28, GLsizei );
    GLenum  format    = READ_DATA(pState, 4 + sizeof(int) + 32, GLenum );
    GLsizei imagesize = READ_DATA(pState, 4 + sizeof(int) + 36, GLsizei );
    int     noimagedata   = READ_DATA(pState, 4 + sizeof(int) + 40, int );
    GLvoid  *pixels;

    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, 4 + sizeof(int)+44, GLint);
    if( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(pState, 4+sizeof(int)+44, GLint);
    else
    {
        pixels = DATA_POINTER(pState, 4 + sizeof( int ) + 48, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, imagesize, GLubyte);
    }

    pState->pDispatchTbl->CompressedTexSubImage3DARB(target, level, xoffset,
                                                 yoffset, zoffset, width,
                                                 height, depth, format,
                                                 imagesize, pixels);
}


void crUnpackExtendCompressedTexSubImage2DARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 4 + sizeof(int) + 32, int);

    GLenum  target    = READ_DATA(pState, 4 + sizeof(int) +  0, GLenum );
    GLint   level     = READ_DATA(pState, 4 + sizeof(int) +  4, GLint );
    GLint   xoffset   = READ_DATA(pState, 4 + sizeof(int) +  8, GLint );
    GLint   yoffset   = READ_DATA(pState, 4 + sizeof(int) + 12, GLint );
    GLsizei width     = READ_DATA(pState, 4 + sizeof(int) + 16, GLsizei );
    GLsizei height    = READ_DATA(pState, 4 + sizeof(int) + 20, GLsizei );
    GLenum  format    = READ_DATA(pState, 4 + sizeof(int) + 24, GLenum );
    GLsizei imagesize = READ_DATA(pState, 4 + sizeof(int) + 28, GLsizei );
    int     noimagedata   = READ_DATA(pState, 4 + sizeof(int) + 32, int );
    GLvoid  *pixels;

    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, 4 + sizeof(int)+36, GLint);
    if( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(pState, 4+sizeof(int)+36, GLint);
    else
    {
        pixels = DATA_POINTER(pState, 4 + sizeof( int ) + 40, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, imagesize, GLubyte);
    }

    pState->pDispatchTbl->CompressedTexSubImage2DARB(target, level, xoffset,
                                                 yoffset, width, height,
                                                 format, imagesize, pixels);
}


void crUnpackExtendCompressedTexSubImage1DARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 4 + sizeof(int) + 24, int);

    GLenum  target    = READ_DATA(pState, 4 + sizeof(int) +  0, GLenum );
    GLint   level     = READ_DATA(pState, 4 + sizeof(int) +  4, GLint );
    GLint   xoffset   = READ_DATA(pState, 4 + sizeof(int) +  8, GLint );
    GLsizei width     = READ_DATA(pState, 4 + sizeof(int) + 12, GLsizei );
    GLenum  format    = READ_DATA(pState, 4 + sizeof(int) + 16, GLenum );
    GLsizei imagesize = READ_DATA(pState, 4 + sizeof(int) + 20, GLsizei );
    int     noimagedata   = READ_DATA(pState, 4 + sizeof(int) + 24, int );
    GLvoid  *pixels;

    CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, 4 + sizeof(int)+28, GLint);
    if( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(pState, 4+sizeof(int)+28, GLint);
    else
    {
        pixels = DATA_POINTER(pState, 4 + sizeof( int ) + 32, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, imagesize, GLubyte);
    }

    pState->pDispatchTbl->CompressedTexSubImage1DARB(target, level, xoffset, width,
                                                 format, imagesize, pixels);
}

void crUnpackExtendGetTexImage(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 32, CRNetworkPointer);

    GLenum target = READ_DATA(pState, 8, GLenum );
    GLint level   = READ_DATA(pState, 12, GLint );
    GLenum format = READ_DATA(pState, 16, GLenum );
    GLenum type   = READ_DATA(pState, 20, GLenum );
    GLvoid *pixels;

    SET_RETURN_PTR(pState, 24);
    SET_WRITEBACK_PTR(pState, 32);
    pixels = DATA_POINTER(pState, 24, GLvoid); /* Used for PBO offset. */

    /*
     * pixels is not written to, see crServerDispatchGetTexImage(). It is only used as an offset
     * The data is returned in the writeback pointer set above.
     */
    pState->pDispatchTbl->GetTexImage(target, level, format, type, pixels);
}

void crUnpackExtendGetCompressedTexImageARB(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 24, CRNetworkPointer);

    GLenum target = READ_DATA(pState, 8, GLenum );
    GLint level   = READ_DATA(pState, 12, GLint );
    GLvoid *img;

    SET_RETURN_PTR(pState, 16 );
    SET_WRITEBACK_PTR(pState, 24 );
    img = DATA_POINTER(pState, 16, GLvoid); /* Used for PBO offset. */

    /* see crUnpackExtendGetTexImage() for explanations regarding verification. */
    pState->pDispatchTbl->GetCompressedTexImageARB( target, level, img );
}
