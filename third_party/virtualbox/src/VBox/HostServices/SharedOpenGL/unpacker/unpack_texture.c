/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_error.h"
#include "cr_protocol.h"
#include "cr_mem.h"
#include "cr_version.h"
 
#if defined( GL_EXT_texture3D ) 
void crUnpackTexImage3DEXT( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA( sizeof( int ) + 4, GLint );
    GLenum internalformat = READ_DATA( sizeof( int ) + 8, GLint );
    GLsizei width = READ_DATA( sizeof( int ) + 12, GLsizei );
    GLsizei height = READ_DATA( sizeof( int ) + 16, GLsizei );
    GLsizei depth = READ_DATA( sizeof( int ) + 20, GLsizei );
    GLint border = READ_DATA( sizeof( int ) + 24, GLint );
    GLenum format = READ_DATA( sizeof( int ) + 28, GLenum );
    GLenum type = READ_DATA( sizeof( int ) + 32, GLenum );
    int noimagedata = READ_DATA( sizeof( int ) + 36, int );
    GLvoid *pixels;

    /*If there's no imagedata send, it's either that passed pointer was NULL or
      there was GL_PIXEL_UNPACK_BUFFER_ARB bound, in both cases 4bytes of passed
      pointer would convert to either NULL or offset in the bound buffer.
     */
    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(sizeof(int)+40, GLint);
    else
        pixels = DATA_POINTER( sizeof( int ) + 44, GLvoid );

    cr_unpackDispatch.TexImage3DEXT(target, level, internalformat, width,
                                    height, depth, border, format, type,
                                    pixels);
    INCR_VAR_PTR();
}
#endif /* GL_EXT_texture3D */

#if defined( CR_OPENGL_VERSION_1_2 )
void crUnpackTexImage3D( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA( sizeof( int ) + 4, GLint );
    GLint internalformat = READ_DATA( sizeof( int ) + 8, GLint );
    GLsizei width = READ_DATA( sizeof( int ) + 12, GLsizei );
    GLsizei height = READ_DATA( sizeof( int ) + 16, GLsizei );
    GLsizei depth = READ_DATA( sizeof( int ) + 20, GLsizei );
    GLint border = READ_DATA( sizeof( int ) + 24, GLint );
    GLenum format = READ_DATA( sizeof( int ) + 28, GLenum );
    GLenum type = READ_DATA( sizeof( int ) + 32, GLenum );
    int noimagedata = READ_DATA( sizeof( int ) + 36, int );
    GLvoid *pixels;
    
    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(sizeof(int)+40, GLint);
    else
        pixels = DATA_POINTER( sizeof( int ) + 44, GLvoid );
    
    cr_unpackDispatch.TexImage3D( target, level, internalformat, width, height,
                                  depth, border, format, type, pixels );
    INCR_VAR_PTR();
}
#endif /* CR_OPENGL_VERSION_1_2 */

void crUnpackTexImage2D( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA( sizeof( int ) + 4, GLint );
    GLint internalformat = READ_DATA( sizeof( int ) + 8, GLint );
    GLsizei width = READ_DATA( sizeof( int ) + 12, GLsizei );
    GLsizei height = READ_DATA( sizeof( int ) + 16, GLsizei );
    GLint border = READ_DATA( sizeof( int ) + 20, GLint );
    GLenum format = READ_DATA( sizeof( int ) + 24, GLenum );
    GLenum type = READ_DATA( sizeof( int ) + 28, GLenum );
    int noimagedata = READ_DATA( sizeof( int ) + 32, int );
    GLvoid *pixels;

    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(sizeof(int)+36, GLint);
    else 
        pixels = DATA_POINTER( sizeof( int ) + 40, GLvoid );

    cr_unpackDispatch.TexImage2D( target, level, internalformat, width, height,
                          border, format, type, pixels );
    INCR_VAR_PTR();
}

void crUnpackTexImage1D( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA( sizeof( int ) + 4, GLint );
    GLint internalformat = READ_DATA( sizeof( int ) + 8, GLint );
    GLsizei width = READ_DATA( sizeof( int ) + 12, GLsizei );
    GLint border = READ_DATA( sizeof( int ) + 16, GLint );
    GLenum format = READ_DATA( sizeof( int ) + 20, GLenum );
    GLenum type = READ_DATA( sizeof( int ) + 24, GLenum );
    int noimagedata = READ_DATA( sizeof( int ) + 28, int );
    GLvoid *pixels;

    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(sizeof(int)+32, GLint);
    else 
        pixels = DATA_POINTER( sizeof( int ) + 36, GLvoid );

    cr_unpackDispatch.TexImage1D( target, level, internalformat, width, border,
                          format, type, pixels );
    INCR_VAR_PTR();
}

void crUnpackDeleteTextures( void )
{
    GLsizei n = READ_DATA( sizeof( int ) + 0, GLsizei );
    GLuint *textures = DATA_POINTER( sizeof( int ) + 4, GLuint );

    cr_unpackDispatch.DeleteTextures( n, textures );
    INCR_VAR_PTR();
}


void crUnpackPrioritizeTextures( void )
{
    GLsizei n = READ_DATA( sizeof( int ) + 0, GLsizei );
    GLuint *textures = DATA_POINTER( sizeof( int ) + 4, GLuint );
    GLclampf *priorities = DATA_POINTER( sizeof( int ) + 4 + n*sizeof( GLuint ),
                                         GLclampf );

    cr_unpackDispatch.PrioritizeTextures( n, textures, priorities );
    INCR_VAR_PTR();
}

void crUnpackTexParameterfv( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
    GLfloat *params = DATA_POINTER( sizeof( int ) + 8, GLfloat );

    cr_unpackDispatch.TexParameterfv( target, pname, params );
    INCR_VAR_PTR();
}

void crUnpackTexParameteriv( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
    GLint *params = DATA_POINTER( sizeof( int ) + 8, GLint );

    cr_unpackDispatch.TexParameteriv( target, pname, params );
    INCR_VAR_PTR();
}

void crUnpackTexParameterf( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
    GLfloat param = READ_DATA( sizeof( int ) + 8, GLfloat );

    cr_unpackDispatch.TexParameterf( target, pname, param );
    INCR_VAR_PTR();
}

void crUnpackTexParameteri( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
    GLint param = READ_DATA( sizeof( int ) + 8, GLint );

    cr_unpackDispatch.TexParameteri( target, pname, param );
    INCR_VAR_PTR();
}

#if defined(CR_OPENGL_VERSION_1_2)
void crUnpackTexSubImage3D( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA( sizeof( int ) + 4, GLint );
    GLint xoffset = READ_DATA( sizeof( int ) + 8, GLint );
    GLint yoffset = READ_DATA( sizeof( int ) + 12, GLint );
    GLint zoffset = READ_DATA( sizeof( int ) + 16, GLint );
    GLsizei width = READ_DATA( sizeof( int ) + 20, GLsizei );
    GLsizei height = READ_DATA( sizeof( int ) + 24, GLsizei );
    GLsizei depth = READ_DATA( sizeof( int ) + 28, GLsizei );
    GLenum format = READ_DATA( sizeof( int ) + 32, GLenum );
    GLenum type = READ_DATA( sizeof( int ) + 36, GLenum );
    int noimagedata = READ_DATA( sizeof( int ) + 40, int );
    GLvoid *pixels;

    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(sizeof(int)+44, GLint);
    else 
        pixels = DATA_POINTER( sizeof( int ) + 48, GLvoid );

    cr_unpackDispatch.PixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    cr_unpackDispatch.TexSubImage3D(target, level, xoffset, yoffset, zoffset,
                                    width, height, depth, format, type, pixels);
    INCR_VAR_PTR();
}
#endif /* CR_OPENGL_VERSION_1_2 */

void crUnpackTexSubImage2D( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA( sizeof( int ) + 4, GLint );
    GLint xoffset = READ_DATA( sizeof( int ) + 8, GLint );
    GLint yoffset = READ_DATA( sizeof( int ) + 12, GLint );
    GLsizei width = READ_DATA( sizeof( int ) + 16, GLsizei );
    GLsizei height = READ_DATA( sizeof( int ) + 20, GLsizei );
    GLenum format = READ_DATA( sizeof( int ) + 24, GLenum );
    GLenum type = READ_DATA( sizeof( int ) + 28, GLenum );
    int noimagedata = READ_DATA( sizeof( int ) + 32, int );
    GLvoid *pixels;

    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(sizeof(int)+36, GLint);
    else 
        pixels = DATA_POINTER( sizeof( int ) + 40, GLvoid );

    cr_unpackDispatch.PixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    cr_unpackDispatch.TexSubImage2D( target, level, xoffset, yoffset, width,
                                     height, format, type, pixels );
    INCR_VAR_PTR();
}

void crUnpackTexSubImage1D( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLint level = READ_DATA( sizeof( int ) + 4, GLint );
    GLint xoffset = READ_DATA( sizeof( int ) + 8, GLint );
    GLsizei width = READ_DATA( sizeof( int ) + 12, GLsizei );
    GLenum format = READ_DATA( sizeof( int ) + 16, GLenum );
    GLenum type = READ_DATA( sizeof( int ) + 20, GLenum );
    int noimagedata = READ_DATA( sizeof( int ) + 24, int );
    GLvoid *pixels;

    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(sizeof(int)+28, GLint);
    else 
        pixels = DATA_POINTER( sizeof( int ) + 32, GLvoid );

    cr_unpackDispatch.PixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    cr_unpackDispatch.TexSubImage1D( target, level, xoffset, width, format,
                                     type, pixels );
    INCR_VAR_PTR();
}


void crUnpackTexEnvfv( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
    GLfloat *params = DATA_POINTER( sizeof( int ) + 8, GLfloat );

    cr_unpackDispatch.TexEnvfv( target, pname, params );
    INCR_VAR_PTR();
}

void crUnpackTexEnviv( void )
{
    GLenum target = READ_DATA( sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
    GLint *params = DATA_POINTER( sizeof( int ) + 8, GLint );

    cr_unpackDispatch.TexEnviv( target, pname, params );
    INCR_VAR_PTR();
}

#define DATA_POINTER_DOUBLE( offset )

void crUnpackTexGendv( void )
{
    GLenum coord = READ_DATA( sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
    GLdouble params[4];
    unsigned int n_param = READ_DATA( 0, int ) - ( sizeof(int) + 8 );

    if (n_param > sizeof(params))
    {
        crError("crUnpackTexGendv: n_param=%d, expected <= %d\n", n_param,
            (unsigned int)sizeof(params));
        return;
    }

    crMemcpy( params, DATA_POINTER( sizeof( int ) + 8, GLdouble ), n_param );

    cr_unpackDispatch.TexGendv( coord, pname, params );
    INCR_VAR_PTR();
}

void crUnpackTexGenfv( void )
{
    GLenum coord = READ_DATA( sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
    GLfloat *params = DATA_POINTER( sizeof( int ) + 8, GLfloat );

    cr_unpackDispatch.TexGenfv( coord, pname, params );
    INCR_VAR_PTR();
}

void crUnpackTexGeniv( void )
{
    GLenum coord = READ_DATA( sizeof( int ) + 0, GLenum );
    GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
    GLint *params = DATA_POINTER( sizeof( int ) + 8, GLint );

    cr_unpackDispatch.TexGeniv( coord, pname, params );
    INCR_VAR_PTR();
}

void crUnpackExtendAreTexturesResident( void )
{
    GLsizei n = READ_DATA( 8, GLsizei );
    const GLuint *textures = DATA_POINTER( 12, const GLuint );
    SET_RETURN_PTR(12 + n * sizeof(GLuint));
    SET_WRITEBACK_PTR(20 + n * sizeof(GLuint));
    (void) cr_unpackDispatch.AreTexturesResident( n, textures, NULL );
}


void crUnpackExtendCompressedTexImage3DARB( void )
{
    GLenum  target         = READ_DATA( 4 + sizeof(int) +  0, GLenum );
    GLint   level          = READ_DATA( 4 + sizeof(int) +  4, GLint );
    GLenum  internalformat = READ_DATA( 4 + sizeof(int) +  8, GLenum );
    GLsizei width          = READ_DATA( 4 + sizeof(int) + 12, GLsizei );
    GLsizei height         = READ_DATA( 4 + sizeof(int) + 16, GLsizei );
    GLsizei depth          = READ_DATA( 4 + sizeof(int) + 20, GLsizei );
    GLint   border         = READ_DATA( 4 + sizeof(int) + 24, GLint );
    GLsizei imagesize      = READ_DATA( 4 + sizeof(int) + 28, GLsizei );
    int     noimagedata        = READ_DATA( 4 + sizeof(int) + 32, int );
    GLvoid  *pixels;

    if( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(4+sizeof(int)+36, GLint);
    else
        pixels = DATA_POINTER( 4 + sizeof(int) + 40, GLvoid );

    cr_unpackDispatch.CompressedTexImage3DARB(target, level, internalformat,
                                              width, height, depth, border,
                                              imagesize, pixels);
}


void crUnpackExtendCompressedTexImage2DARB( void )
{
    GLenum target =         READ_DATA( 4 + sizeof( int ) + 0, GLenum );
    GLint level =           READ_DATA( 4 + sizeof( int ) + 4, GLint );
    GLenum internalformat = READ_DATA( 4 + sizeof( int ) + 8, GLenum );
    GLsizei width =         READ_DATA( 4 + sizeof( int ) + 12, GLsizei );
    GLsizei height =        READ_DATA( 4 + sizeof( int ) + 16, GLsizei );
    GLint border =          READ_DATA( 4 + sizeof( int ) + 20, GLint );
    GLsizei imagesize =     READ_DATA( 4 + sizeof( int ) + 24, GLsizei );
    int noimagedata =           READ_DATA( 4 + sizeof( int ) + 28, int );
    GLvoid *pixels;

    if ( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(4+sizeof(int)+32, GLint);
    else
        pixels = DATA_POINTER( 4 + sizeof( int ) + 36, GLvoid );

    cr_unpackDispatch.CompressedTexImage2DARB( target, level, internalformat,
                                               width, height, border, imagesize,
                                               pixels );
}


void crUnpackExtendCompressedTexImage1DARB( void )
{
    GLenum  target         = READ_DATA( 4 + sizeof(int) +  0, GLenum );
    GLint   level          = READ_DATA( 4 + sizeof(int) +  4, GLint );
    GLenum  internalformat = READ_DATA( 4 + sizeof(int) +  8, GLenum );
    GLsizei width          = READ_DATA( 4 + sizeof(int) + 12, GLsizei );
    GLint   border         = READ_DATA( 4 + sizeof(int) + 16, GLint );
    GLsizei imagesize      = READ_DATA( 4 + sizeof(int) + 20, GLsizei );
    int     noimagedata        = READ_DATA( 4 + sizeof(int) + 24, int );
    GLvoid  *pixels;

    if( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(4+sizeof(int)+28, GLint);
    else
        pixels = DATA_POINTER( 4 + sizeof(int) + 32, GLvoid );

    cr_unpackDispatch.CompressedTexImage1DARB(target, level, internalformat,
                                              width, border, imagesize, pixels);
}


void crUnpackExtendCompressedTexSubImage3DARB( void )
{
    GLenum  target    = READ_DATA( 4 + sizeof(int) +  0, GLenum );
    GLint   level     = READ_DATA( 4 + sizeof(int) +  4, GLint );
    GLint   xoffset   = READ_DATA( 4 + sizeof(int) +  8, GLint );
    GLint   yoffset   = READ_DATA( 4 + sizeof(int) + 12, GLint );
    GLint   zoffset   = READ_DATA( 4 + sizeof(int) + 16, GLint );
    GLsizei width     = READ_DATA( 4 + sizeof(int) + 20, GLsizei );
    GLsizei height    = READ_DATA( 4 + sizeof(int) + 24, GLsizei );
    GLsizei depth     = READ_DATA( 4 + sizeof(int) + 28, GLsizei );
    GLenum  format    = READ_DATA( 4 + sizeof(int) + 32, GLenum );
    GLsizei imagesize = READ_DATA( 4 + sizeof(int) + 36, GLsizei );
    int     noimagedata   = READ_DATA( 4 + sizeof(int) + 40, int );
    GLvoid  *pixels;

    if( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(4+sizeof(int)+44, GLint);
    else
        pixels = DATA_POINTER( 4 + sizeof(int) + 48, GLvoid );

    cr_unpackDispatch.CompressedTexSubImage3DARB(target, level, xoffset,
                                                 yoffset, zoffset, width,
                                                 height, depth, format,
                                                 imagesize, pixels);
}


void crUnpackExtendCompressedTexSubImage2DARB( void )
{
    GLenum  target    = READ_DATA( 4 + sizeof(int) +  0, GLenum );
    GLint   level     = READ_DATA( 4 + sizeof(int) +  4, GLint );
    GLint   xoffset   = READ_DATA( 4 + sizeof(int) +  8, GLint );
    GLint   yoffset   = READ_DATA( 4 + sizeof(int) + 12, GLint );
    GLsizei width     = READ_DATA( 4 + sizeof(int) + 16, GLsizei );
    GLsizei height    = READ_DATA( 4 + sizeof(int) + 20, GLsizei );
    GLenum  format    = READ_DATA( 4 + sizeof(int) + 24, GLenum );
    GLsizei imagesize = READ_DATA( 4 + sizeof(int) + 28, GLsizei );
    int     noimagedata   = READ_DATA( 4 + sizeof(int) + 32, int );
    GLvoid  *pixels;

    if( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(4+sizeof(int)+36, GLint);
    else
        pixels = DATA_POINTER( 4 + sizeof(int) + 40, GLvoid );

    cr_unpackDispatch.CompressedTexSubImage2DARB(target, level, xoffset,
                                                 yoffset, width, height,
                                                 format, imagesize, pixels);
}


void crUnpackExtendCompressedTexSubImage1DARB( void )
{
    GLenum  target    = READ_DATA( 4 + sizeof(int) +  0, GLenum );
    GLint   level     = READ_DATA( 4 + sizeof(int) +  4, GLint );
    GLint   xoffset   = READ_DATA( 4 + sizeof(int) +  8, GLint );
    GLsizei width     = READ_DATA( 4 + sizeof(int) + 12, GLsizei );
    GLenum  format    = READ_DATA( 4 + sizeof(int) + 16, GLenum );
    GLsizei imagesize = READ_DATA( 4 + sizeof(int) + 20, GLsizei );
    int     noimagedata   = READ_DATA( 4 + sizeof(int) + 24, int );
    GLvoid  *pixels;

    if( noimagedata )
        pixels = (void*) (uintptr_t) READ_DATA(4+sizeof(int)+28, GLint);
    else
        pixels = DATA_POINTER( 4 + sizeof(int) + 32, GLvoid );

    cr_unpackDispatch.CompressedTexSubImage1DARB(target, level, xoffset, width,
                                                 format, imagesize, pixels);
}

void crUnpackExtendGetTexImage(void)
{
    GLenum target = READ_DATA( 8, GLenum );
    GLint level   = READ_DATA( 12, GLint );
    GLenum format = READ_DATA( 16, GLenum );
    GLenum type   = READ_DATA( 20, GLenum );
    GLvoid *pixels;

    SET_RETURN_PTR(24);
    SET_WRITEBACK_PTR(32);
    pixels = DATA_POINTER(24, GLvoid);

    cr_unpackDispatch.GetTexImage(target, level, format, type, pixels);
}

void crUnpackExtendGetCompressedTexImageARB(void)
{
    GLenum target = READ_DATA( 8, GLenum );
    GLint level   = READ_DATA( 12, GLint );
    GLvoid *img;

    SET_RETURN_PTR( 16 );
    SET_WRITEBACK_PTR( 24 );
    img = DATA_POINTER(16, GLvoid);

    cr_unpackDispatch.GetCompressedTexImageARB( target, level, img );
}
