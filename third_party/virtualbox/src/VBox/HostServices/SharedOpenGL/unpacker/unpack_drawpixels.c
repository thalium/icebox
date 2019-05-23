/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_error.h"

void crUnpackDrawPixels( void )
{
    GLsizei width  = READ_DATA( sizeof( int ) + 0, GLsizei );
    GLsizei height = READ_DATA( sizeof( int ) + 4, GLsizei );
    GLenum format  = READ_DATA( sizeof( int ) + 8, GLenum );
    GLenum type    = READ_DATA( sizeof( int ) + 12, GLenum );
    GLint noimagedata = READ_DATA( sizeof( int ) + 16, GLint );
    GLvoid *pixels;

    if (noimagedata)
        pixels = (void*) (uintptr_t) READ_DATA( sizeof( int ) + 20, GLint);
    else
        pixels = DATA_POINTER( sizeof( int ) + 24, GLvoid );

    cr_unpackDispatch.PixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    cr_unpackDispatch.DrawPixels( width, height, format, type, pixels );

    INCR_VAR_PTR( );
}

void crUnpackBitmap( void )
{   
    GLsizei width   = READ_DATA( sizeof( int ) + 0, GLsizei );
    GLsizei height  = READ_DATA( sizeof( int ) + 4, GLsizei );
    GLfloat xorig   = READ_DATA( sizeof( int ) + 8, GLfloat );
    GLfloat yorig   = READ_DATA( sizeof( int ) + 12, GLfloat );
    GLfloat xmove   = READ_DATA( sizeof( int ) + 16, GLfloat );
    GLfloat ymove   = READ_DATA( sizeof( int ) + 20, GLfloat );
    GLuint noimagedata = READ_DATA( sizeof( int ) + 24, GLuint );
    GLubyte *bitmap;

    if (noimagedata)
        bitmap = (void*) (uintptr_t) READ_DATA(sizeof(int) + 28, GLint);
    else
        bitmap = DATA_POINTER( sizeof(int) + 32, GLubyte );

    cr_unpackDispatch.PixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    cr_unpackDispatch.Bitmap( width, height, xorig, yorig, xmove, ymove, bitmap );

    INCR_VAR_PTR( );
}

/*
 * ZPixCR  - compressed DrawPixels
 */
void crUnpackExtendZPixCR( void )
{
    GLsizei width   = READ_DATA(   8, GLsizei );
    GLsizei height  = READ_DATA(  12, GLsizei );
    GLenum  format  = READ_DATA(  16, GLenum );
    GLenum  type    = READ_DATA(  20, GLenum );
    GLenum  ztype   = READ_DATA(  24, GLenum );
    GLint   zparm   = READ_DATA(  28, GLuint );
    GLint   length  = READ_DATA(  32, GLint );
    GLvoid  *pixels = DATA_POINTER(  36, GLvoid );

/*XXX JAG 
  crDebug("UnpackZPixCR: w = %d, h = %d, len = %d",
                                 width,  height, length);
*/
    cr_unpackDispatch.PixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    cr_unpackDispatch.PixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    cr_unpackDispatch.ZPixCR( width, height, format, type, ztype, zparm, length, pixels );

    /* Don't call INCR_VAR_PTR(); - it's done in crUnpackExtend() */
}
