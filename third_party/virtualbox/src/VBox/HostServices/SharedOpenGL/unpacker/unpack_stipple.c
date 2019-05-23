/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackPolygonStipple( void  )
{
    int nodata = READ_DATA(0, int);
    GLubyte *mask;

    if (nodata)
        mask = (void*) (uintptr_t) READ_DATA(4, GLint);
    else
        mask = DATA_POINTER( 4, GLubyte );

    cr_unpackDispatch.PolygonStipple(mask);

    if (nodata)
        INCR_DATA_PTR(8);
    else
        INCR_DATA_PTR(4 + 32*32/8);
}
