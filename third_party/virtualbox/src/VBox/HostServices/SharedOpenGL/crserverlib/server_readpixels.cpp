/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_spu.h"
#include "chromium.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "cr_net.h"
#include "cr_pixeldata.h"
#include "cr_unpack.h"
#include "server_dispatch.h"
#include "server.h"


void SERVER_DISPATCH_APIENTRY
crServerDispatchReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                           GLenum format, GLenum type, GLvoid *pixels)
{
    CRASSERT(cr_server.pUnpackerState != NULL);
    CHECK_BUFFER_SIZE_STATIC(cr_server.pUnpackerState, 44 + sizeof(GLint));

    const GLint stride = READ_DATA(cr_server.pUnpackerState, 24, GLint );
    const GLint alignment = READ_DATA(cr_server.pUnpackerState, 28, GLint );
    const GLint skipRows = READ_DATA(cr_server.pUnpackerState, 32, GLint );
    const GLint skipPixels = READ_DATA(cr_server.pUnpackerState, 36, GLint );
    const GLint bytes_per_row = READ_DATA(cr_server.pUnpackerState, 40, GLint );
    const GLint rowLength = READ_DATA(cr_server.pUnpackerState, 44, GLint );

    CRASSERT(bytes_per_row > 0);

#ifdef CR_ARB_pixel_buffer_object
    if (crStateIsBufferBound(&cr_server.StateTracker, GL_PIXEL_PACK_BUFFER_ARB))
    {
        GLvoid *pbo_offset;

        /*pixels are actually a pointer to location of 8byte network pointer in hgcm buffer
          regardless of guest/host bitness we're using only 4lower bytes as there're no
          pbo>4gb (yet?)
         */
        pbo_offset = (GLvoid*) ((uintptr_t) *((GLint*)pixels));

        cr_server.head_spu->dispatch_table.ReadPixels(x, y, width, height,
                                                      format, type, pbo_offset);
    }
    else
#endif
    {
        CRMessageReadPixels *rp;
        uint32_t msg_len;

        if (bytes_per_row <= 0 || height <= 0 || bytes_per_row > INT32_MAX / height)
        {
            crError("crServerDispatchReadPixels: parameters out of range");
            return;
        }

        msg_len = sizeof(*rp) + (uint32_t)bytes_per_row * height;

        rp = (CRMessageReadPixels *) crAlloc( msg_len );
        if (!rp)
        {
            crError("crServerDispatchReadPixels: out of memory");
            return;
        }

        /* Note: the ReadPixels data gets densely packed into the buffer
         * (no skip pixels, skip rows, etc.  It's up to the receiver (pack spu,
         * tilesort spu, etc) to apply the real PixelStore packing parameters.
        */
        cr_server.head_spu->dispatch_table.ReadPixels(x, y, width, height,
                                                      format, type, rp + 1);

        rp->header.type = CR_MESSAGE_READ_PIXELS;
        rp->width = width;
        rp->height = height;
        rp->bytes_per_row = bytes_per_row;
        rp->stride = stride;
        rp->format = format;
        rp->type = type;
        rp->alignment = alignment;
        rp->skipRows = skipRows;
        rp->skipPixels = skipPixels;
        rp->rowLength = rowLength;

        /* <pixels> points to the 8-byte network pointer */
        crMemcpy( &rp->pixels, pixels, sizeof(rp->pixels) );
    
        crNetSend( cr_server.curClient->conn, NULL, rp, msg_len );
        crFree( rp );
    }
}
