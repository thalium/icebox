/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_opcodes.h"

static GLboolean __handleFogData( GLenum pname, const GLfloat *params )
{
    CR_GET_PACKER_CONTEXT(pc);
    int params_length = 0;
    int packet_length = sizeof( int ) + sizeof( pname );
    unsigned char *data_ptr;
    switch( pname )
    {
        case GL_FOG_MODE:
        case GL_FOG_DENSITY:
        case GL_FOG_START:
        case GL_FOG_END:
        case GL_FOG_INDEX:
            params_length = sizeof( *params );
            break;
        case GL_FOG_COLOR:
            params_length = 4*sizeof( *params );
            break;
        default:
            params_length = __packFogParamsLength( pname );
            if (!params_length)
            {
                char msg[100];
                sprintf(msg, "Invalid pname in Fog: %d", (int) pname );
                __PackError( __LINE__, __FILE__, GL_INVALID_ENUM, msg);
                return GL_FALSE;
            }
            break;
    }
    packet_length += params_length;

    CR_GET_BUFFERED_POINTER(pc, packet_length );
    WRITE_DATA( 0, int, packet_length );
    WRITE_DATA( 4, GLenum, pname );
    WRITE_DATA( 8, GLfloat, params[0] );
    if (packet_length > 12)
    {
        WRITE_DATA( 12, GLfloat, params[1] );
        WRITE_DATA( 16, GLfloat, params[2] );
        WRITE_DATA( 20, GLfloat, params[3] );
    }
    return GL_TRUE;
}

void PACK_APIENTRY crPackFogfv(GLenum pname, const GLfloat *params)
{
    CR_GET_PACKER_CONTEXT(pc);
    if (__handleFogData( pname, params ))
        WRITE_OPCODE( pc, CR_FOGFV_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackFogiv(GLenum pname, const GLint *params)
{
    CR_GET_PACKER_CONTEXT(pc);
    /* floats and ints are the same size, so the packing should be the same */
    if (__handleFogData( pname, (const GLfloat *) params ))
        WRITE_OPCODE( pc, CR_FOGIV_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}
