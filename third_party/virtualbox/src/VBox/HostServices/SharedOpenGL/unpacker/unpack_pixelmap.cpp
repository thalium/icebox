/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "state/cr_bufferobject.h"

void crUnpackPixelMapfv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 8, int);

    GLenum map = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLsizei mapsize = READ_DATA(pState, sizeof( int ) + 4, GLsizei );
    int nodata = READ_DATA(pState, sizeof(int) + 8, int);
    GLfloat *values;

    if (nodata && !crStateIsBufferBound(pState->pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
        return;

    if (nodata)
    {
        /* This is valid, see glPixelMap description for GL_PIXEL_UNPACK_BUFFER values is treated as a byte offset. */
        CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof( int ) + 12, GLfloat);
        values = (GLfloat*) (uintptr_t) READ_DATA(pState,sizeof(int) + 12, GLint);
    }
    else
    {
        CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof( int ) + 16, GLfloat);

        values = DATA_POINTER(pState, sizeof( int ) + 16, GLfloat );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, values, mapsize, GLfloat);
    }

    pState->pDispatchTbl->PixelMapfv( map, mapsize, values );
    INCR_VAR_PTR(pState);
}

void crUnpackPixelMapuiv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 8, int);

    GLenum map = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLsizei mapsize = READ_DATA(pState, sizeof( int ) + 4, GLsizei );
    int nodata = READ_DATA(pState, sizeof(int) + 8, int);
    GLuint *values;

    if (nodata && !crStateIsBufferBound(pState->pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
        return;

    if (nodata)
    {
        /* This is valid, see glPixelMap description for GL_PIXEL_UNPACK_BUFFER values is treated as a byte offset. */
        CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof( int ) + 12, GLfloat);
        values = (GLuint*) (uintptr_t) READ_DATA(pState,sizeof(int) + 12, GLint);
    }
    else
    {
        CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof( int ) + 16, GLfloat);

        values = DATA_POINTER(pState, sizeof( int ) + 16, GLuint );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, values, mapsize, GLuint);
    }

    pState->pDispatchTbl->PixelMapuiv( map, mapsize, values );
    INCR_VAR_PTR(pState);
}

void crUnpackPixelMapusv(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 8, int);

    GLenum map = READ_DATA(pState, sizeof( int ) + 0, GLenum );
    GLsizei mapsize = READ_DATA(pState, sizeof( int ) + 4, GLsizei );
    int nodata = READ_DATA(pState, sizeof(int) + 8, int);
    GLushort *values;

    if (nodata && !crStateIsBufferBound(pState->pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
        return;

    if (nodata)
    {
        /* This is valid, see glPixelMap description for GL_PIXEL_UNPACK_BUFFER values is treated as a byte offset. */
        CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof( int ) + 12, GLfloat);
        values = (GLushort*) (uintptr_t) READ_DATA(pState, sizeof(int) + 12, GLint);
    }
    else
    {
        CHECK_BUFFER_SIZE_STATIC_UPDATE_LAST(pState, sizeof( int ) + 16, GLfloat);

        values = DATA_POINTER(pState, sizeof( int ) + 16, GLushort );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, values, mapsize, GLushort);
    }

    pState->pDispatchTbl->PixelMapusv( map, mapsize, values );
    INCR_VAR_PTR(pState);
}
