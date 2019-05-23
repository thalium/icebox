/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackPolygonStipple( void  )
{
    int nodata = READ_DATA(0, int);

    if (nodata)
    {
        crError("crUnpackPolygonStipple: GL_PIXEL_UNPACK_BUFFER is not supported");
        INCR_DATA_PTR(8);
    }
    else
    {
        GLubyte *mask;

        mask = DATA_POINTER(4, GLubyte);
        cr_unpackDispatch.PolygonStipple(mask);
        // Stipple mask consists of 32 * 32 bits
        INCR_DATA_PTR(4 + 32 * 32 / 8);
    }
}
