/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_opcodes.h"
#include "cr_mem.h"
#include "cr_glstate.h"

void PACK_APIENTRY crPackPolygonStipple( const GLubyte *mask )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int nodata = crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB);
    int packet_length = sizeof(int);

    if (nodata)
        packet_length += sizeof(GLint);
    else
        packet_length += 32*32/8;

    CR_GET_BUFFERED_POINTER(pc, packet_length );
    WRITE_DATA_AI(int, nodata);
    if (nodata)
    {
        WRITE_DATA_AI(GLint, (GLint)(uintptr_t)mask);
    }
    else
    {
       crMemcpy( data_ptr, mask, 32*32/8 );
    }
    WRITE_OPCODE( pc, CR_POLYGONSTIPPLE_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}
