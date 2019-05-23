/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_error.h"

static void __handleMaterialData( GLenum face, GLenum pname, const GLfloat *params )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned int packet_length = sizeof( int ) + sizeof( face ) + sizeof( pname );
    unsigned int params_length = 0;
    unsigned char *data_ptr;
    switch( pname )
    {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_EMISSION:
        case GL_AMBIENT_AND_DIFFUSE:
            params_length = 4*sizeof( *params );
            break;
        case GL_COLOR_INDEXES:
            params_length = 3*sizeof( *params );
            break;
        case GL_SHININESS:
            params_length = sizeof( *params );
            break;
        default:
            __PackError(__LINE__, __FILE__, GL_INVALID_ENUM, "glMaterial(pname)");
            return;
    }
    packet_length += params_length;

    CR_GET_BUFFERED_POINTER(pc, packet_length );
    WRITE_DATA( 0, int, packet_length );
    WRITE_DATA( sizeof( int ) + 0, GLenum, face );
    WRITE_DATA( sizeof( int ) + 4, GLenum, pname );
    WRITE_DATA( sizeof( int ) + 8, GLfloat, params[0] );
    if (params_length > sizeof( *params )) 
    {
        WRITE_DATA( sizeof( int ) + 12, GLfloat, params[1] );
        WRITE_DATA( sizeof( int ) + 16, GLfloat, params[2] );
    }
    if (packet_length > 3*sizeof( *params ) ) 
    {
        WRITE_DATA( sizeof( int ) + 20, GLfloat, params[3] );
    }
}

void PACK_APIENTRY crPackMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    CR_GET_PACKER_CONTEXT(pc);
    __handleMaterialData( face, pname, params );
    WRITE_OPCODE( pc, CR_MATERIALFV_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
    /* floats and ints are the same size, so the packing should be the same */
    CR_GET_PACKER_CONTEXT(pc);
    __handleMaterialData( face, pname, (const GLfloat *) params );
    WRITE_OPCODE( pc, CR_MATERIALIV_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}
