/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "state/cr_bufferobject.h"

void crUnpackPixelMapfv( void  )
{
    GLenum map = READ_DATA( sizeof( int ) + 0, GLenum );
    GLsizei mapsize = READ_DATA( sizeof( int ) + 4, GLsizei );
    int nodata = READ_DATA( sizeof(int) + 8, int);
    GLfloat *values;

    if (nodata && !crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))
        return;

    if (nodata)
        values = (GLfloat*) (uintptr_t) READ_DATA(sizeof(int) + 12, GLint);
    else
        values = DATA_POINTER( sizeof( int ) + 16, GLfloat );

    cr_unpackDispatch.PixelMapfv( map, mapsize, values );
    INCR_VAR_PTR();
}

void crUnpackPixelMapuiv( void  )
{
    GLenum map = READ_DATA( sizeof( int ) + 0, GLenum );
    GLsizei mapsize = READ_DATA( sizeof( int ) + 4, GLsizei );
    int nodata = READ_DATA( sizeof(int) + 8, int);
    GLuint *values;

    if (nodata && !crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))
        return;

    if (nodata)
        values = (GLuint*) (uintptr_t) READ_DATA(sizeof(int) + 12, GLint);
    else
        values = DATA_POINTER( sizeof( int ) + 16, GLuint );
        
    cr_unpackDispatch.PixelMapuiv( map, mapsize, values );
    INCR_VAR_PTR();
}

void crUnpackPixelMapusv( void  )
{
    GLenum map = READ_DATA( sizeof( int ) + 0, GLenum );
    GLsizei mapsize = READ_DATA( sizeof( int ) + 4, GLsizei );
    int nodata = READ_DATA( sizeof(int) + 8, int);
    GLushort *values;

    if (nodata && !crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB))
        return;

    if (nodata)
        values = (GLushort*) (uintptr_t) READ_DATA(sizeof(int) + 12, GLint);
    else
        values = DATA_POINTER( sizeof( int ) + 16, GLushort );

    cr_unpackDispatch.PixelMapusv( map, mapsize, values );
    INCR_VAR_PTR();
}
