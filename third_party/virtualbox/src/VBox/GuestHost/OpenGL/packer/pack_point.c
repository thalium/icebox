/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_opcodes.h"
#include "cr_version.h"

#ifdef CR_ARB_point_parameters

static GLboolean __handlePointDataf( GLenum pname, const GLfloat *params )
{
    CR_GET_PACKER_CONTEXT(pc);
    int params_length = 0;
    int packet_length = 2 * sizeof( int ) + sizeof( pname );
    unsigned char *data_ptr;
    switch( pname )
    {
        case GL_POINT_SIZE_MIN_ARB:
        case GL_POINT_SIZE_MAX_ARB:
        case GL_POINT_FADE_THRESHOLD_SIZE_ARB:
            params_length = sizeof( *params );
            break;
        case GL_POINT_DISTANCE_ATTENUATION_ARB:
            params_length = 3 * sizeof( *params );
            break;
        default:
            {
                char msg[100];
                sprintf(msg, "Invalid pname in PointParameter: %d", (int) pname );
                __PackError( __LINE__, __FILE__, GL_INVALID_ENUM, msg);
            }                                
            return GL_FALSE;
    }
    packet_length += params_length;

    CR_GET_BUFFERED_POINTER(pc, packet_length );
    WRITE_DATA( 0, GLint, packet_length );
    WRITE_DATA( 4, GLenum, CR_POINTPARAMETERFVARB_EXTEND_OPCODE );
    WRITE_DATA( 8, GLenum, pname );
    WRITE_DATA( 12, GLfloat, params[0] );
    if (packet_length > 16) 
    {
        WRITE_DATA( 16, GLfloat, params[1] );
        WRITE_DATA( 20, GLfloat, params[2] );
    }
    return GL_TRUE;
}

void PACK_APIENTRY crPackPointParameterfvARB(GLenum pname, const GLfloat *params)
{
    CR_GET_PACKER_CONTEXT(pc);
    if (__handlePointDataf( pname, params ))
        WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

#endif

static GLboolean __handlePointDatai( GLenum pname, const GLint *params )
{
    CR_GET_PACKER_CONTEXT(pc);
    int params_length = 0;
    int packet_length = 2 * sizeof( int ) + sizeof( pname );
    unsigned char *data_ptr;
    switch( pname )
    {
        case GL_POINT_SIZE_MIN_ARB:
        case GL_POINT_SIZE_MAX_ARB:
        case GL_POINT_FADE_THRESHOLD_SIZE_ARB:
            params_length = sizeof( *params );
            break;
        case GL_POINT_DISTANCE_ATTENUATION_ARB:
            params_length = 3 * sizeof( *params );
            break;
        default:
            {
                char msg[100];
                sprintf(msg, "Invalid pname in PointParameter: %d", (int) pname );
                __PackError( __LINE__, __FILE__, GL_INVALID_ENUM, msg);
            }
            return GL_FALSE;
    }
    packet_length += params_length;

    CR_GET_BUFFERED_POINTER(pc, packet_length );
    WRITE_DATA( 0, GLint, packet_length );
    WRITE_DATA( 4, GLenum, CR_POINTPARAMETERIV_EXTEND_OPCODE );
    WRITE_DATA( 8, GLenum, pname );
    WRITE_DATA( 12, GLint, params[0] );
    if (packet_length > 16) 
    {
        WRITE_DATA( 16, GLint, params[1] );
        WRITE_DATA( 20, GLint, params[2] );
    }
    return GL_TRUE;
}

void PACK_APIENTRY crPackPointParameteriv(GLenum pname, const GLint *params)
{
    CR_GET_PACKER_CONTEXT(pc);
    if (__handlePointDatai( pname, params ))
        WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}
