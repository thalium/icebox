/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackPolygonStipple(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 0, int);

    int nodata = READ_DATA(pState, 0, int);

    if (nodata)
    {
        crError("crUnpackPolygonStipple: GL_PIXEL_UNPACK_BUFFER is not supported");
        INCR_DATA_PTR(pState, 8);
    }
    else
    {
        /* The mask is represented as a 32x32 array of 1bit color (32x4 bytes). */
        GLubyte *mask = DATA_POINTER(pState, 4, GLubyte);
        CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, mask, 32 * (32 / 8), GLubyte);

        pState->pDispatchTbl->PolygonStipple(mask);
        // Stipple mask consists of 32 * 32 bits
        INCR_DATA_PTR(pState, 4 + 32 * 32 / 8);
    }
}
