/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_error.h"
#include "cr_pixeldata.h"

#include "state/cr_bufferobject.h"

void crUnpackDrawPixels(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 20, GLint);

    GLsizei width  = READ_DATA(pState, sizeof( int ) + 0, GLsizei );
    GLsizei height = READ_DATA(pState, sizeof( int ) + 4, GLsizei );
    GLenum format  = READ_DATA(pState, sizeof( int ) + 8, GLenum );
    GLenum type    = READ_DATA(pState, sizeof( int ) + 12, GLenum );
    GLint noimagedata = READ_DATA(pState, sizeof( int ) + 16, GLint );
    GLvoid *pixels;

    if (noimagedata && !crStateIsBufferBound(pState->pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
        return;

    if (noimagedata)
        pixels = (void*) (uintptr_t) READ_DATA(pState, sizeof( int ) + 20, GLint);
    else
    {
        size_t cbImg = crImageSize( format, type, width, height );
        if (RT_UNLIKELY(cbImg == 0))
        {
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
        }

        pixels = DATA_POINTER(pState, sizeof( int ) + 24, GLvoid );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, cbImg, GLubyte);
    }

    pState->pDispatchTbl->PixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    pState->pDispatchTbl->DrawPixels( width, height, format, type, pixels );

    INCR_VAR_PTR(pState);
}

void crUnpackBitmap(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, sizeof(int) + 28, GLint);

    GLsizei width   = READ_DATA(pState, sizeof( int ) + 0, GLsizei );
    GLsizei height  = READ_DATA(pState,sizeof( int ) + 4, GLsizei );
    GLfloat xorig   = READ_DATA(pState, sizeof( int ) + 8, GLfloat );
    GLfloat yorig   = READ_DATA(pState, sizeof( int ) + 12, GLfloat );
    GLfloat xmove   = READ_DATA(pState, sizeof( int ) + 16, GLfloat );
    GLfloat ymove   = READ_DATA(pState, sizeof( int ) + 20, GLfloat );
    GLuint noimagedata = READ_DATA(pState, sizeof( int ) + 24, GLuint );
    GLubyte *bitmap;

    if (noimagedata && !crStateIsBufferBound(pState->pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB))
        return;

    if (noimagedata)
        bitmap = (GLubyte *) (uintptr_t) READ_DATA(pState,sizeof(int) + 28, GLint);
    else
    {
        /* Each pixel is one bit => 8 pixels per byte. */
        size_t cbImg = crImageSize(GL_COLOR_INDEX, GL_BITMAP, width, height);
        if (RT_UNLIKELY(cbImg == 0))
        {
            pState->rcUnpack = VERR_INVALID_PARAMETER;
            return;
        }

        bitmap = DATA_POINTER(pState, sizeof(int) + 32, GLubyte );
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, bitmap, cbImg, GLubyte);
    }

    pState->pDispatchTbl->PixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    pState->pDispatchTbl->Bitmap( width, height, xorig, yorig, xmove, ymove, bitmap );

    INCR_VAR_PTR(pState);
}

/*
 * ZPixCR  - compressed DrawPixels
 */
void crUnpackExtendZPixCR(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 32, GLint);

    GLsizei width   = READ_DATA(pState,   8, GLsizei );
    GLsizei height  = READ_DATA(pState,  12, GLsizei );
    GLenum  format  = READ_DATA(pState,  16, GLenum );
    GLenum  type    = READ_DATA(pState,  20, GLenum );
    GLenum  ztype   = READ_DATA(pState,  24, GLenum );
    GLint   zparm   = READ_DATA(pState,  28, GLuint );
    GLint   length  = READ_DATA(pState,  32, GLint );
    GLvoid  *pixels = DATA_POINTER(pState,  36, GLvoid );

    pState->pDispatchTbl->PixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_UNPACK_ALIGNMENT, 1 );

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, pixels, length, GLubyte);
    pState->pDispatchTbl->ZPixCR( width, height, format, type, ztype, zparm, length, pixels );

    /* Don't call INCR_VAR_PTR(); - it's done in crUnpackExtend() */
}
