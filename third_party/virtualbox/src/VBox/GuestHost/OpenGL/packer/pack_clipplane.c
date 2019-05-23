/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_opcodes.h"

void PACK_APIENTRY crPackClipPlane( GLenum plane, const GLdouble *equation )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    int packet_length = sizeof( plane ) + 4*sizeof(*equation);
    CR_GET_BUFFERED_POINTER(pc, packet_length );
    WRITE_DATA( 0, GLenum, plane );
    WRITE_DOUBLE( 4, equation[0] );
    WRITE_DOUBLE( 12, equation[1] );
    WRITE_DOUBLE( 20, equation[2] );
    WRITE_DOUBLE( 28, equation[3] );
    WRITE_OPCODE( pc, CR_CLIPPLANE_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}
