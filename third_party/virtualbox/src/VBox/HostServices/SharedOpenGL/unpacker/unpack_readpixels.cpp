/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_pixeldata.h"
#include "cr_mem.h"

void crUnpackReadPixels(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 48, CRNetworkPointer);

    GLint x        = READ_DATA(pState,  0, GLint );
    GLint y        = READ_DATA(pState,  4, GLint );
    GLsizei width  = READ_DATA(pState,  8, GLsizei );
    GLsizei height = READ_DATA(pState,  12, GLsizei );
    GLenum format  = READ_DATA(pState,  16, GLenum );
    GLenum type    = READ_DATA(pState,  20, GLenum );
    GLint stride   = READ_DATA(pState,  24, GLint );
    GLint alignment     = READ_DATA(pState,  28, GLint );
    GLint skipRows      = READ_DATA(pState,  32, GLint );
    GLint skipPixels    = READ_DATA(pState,  36, GLint );
    GLint bytes_per_row = READ_DATA(pState,  40, GLint );
    GLint rowLength     = READ_DATA(pState,  44, GLint );

    /* point <pixels> at the 8-byte network pointer where the result will be written to. */
    GLvoid *pixels = DATA_POINTER(pState,  48, GLvoid );

    (void) stride;
    (void) bytes_per_row;
    (void) alignment;
    (void) skipRows;
    (void) skipPixels;
    (void) rowLength;

    /* we always pack densely on the server side! */
    pState->pDispatchTbl->PixelStorei( GL_PACK_ROW_LENGTH, 0 );
    pState->pDispatchTbl->PixelStorei( GL_PACK_SKIP_PIXELS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_PACK_SKIP_ROWS, 0 );
    pState->pDispatchTbl->PixelStorei( GL_PACK_ALIGNMENT, 1 );

    pState->pDispatchTbl->ReadPixels( x, y, width, height, format, type, pixels);

    INCR_DATA_PTR(pState, 48+sizeof(CRNetworkPointer));
}
