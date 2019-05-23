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
#include "server_dispatch.h"
#include "server.h"

static GLint __sizeQuery( GLenum map )
{
    GLint get_values;
    /* Windows compiler gets mad if variables might be uninitialized */
    GLenum newmap = GL_PIXEL_MAP_I_TO_I_SIZE;

    switch( map )
    {
        case GL_PIXEL_MAP_I_TO_I: 
            newmap = GL_PIXEL_MAP_I_TO_I_SIZE;
            break;
        case GL_PIXEL_MAP_S_TO_S: 
            newmap = GL_PIXEL_MAP_S_TO_S_SIZE;
            break;
        case GL_PIXEL_MAP_I_TO_R: 
            newmap = GL_PIXEL_MAP_I_TO_R_SIZE;
            break;
        case GL_PIXEL_MAP_I_TO_G: 
            newmap = GL_PIXEL_MAP_I_TO_G_SIZE;
            break;
        case GL_PIXEL_MAP_I_TO_B: 
            newmap = GL_PIXEL_MAP_I_TO_B_SIZE;
            break;
        case GL_PIXEL_MAP_I_TO_A: 
            newmap = GL_PIXEL_MAP_I_TO_A_SIZE;
            break;
        case GL_PIXEL_MAP_R_TO_R: 
            newmap = GL_PIXEL_MAP_R_TO_R_SIZE;
            break;
        case GL_PIXEL_MAP_G_TO_G: 
            newmap = GL_PIXEL_MAP_G_TO_G_SIZE;
            break;
        case GL_PIXEL_MAP_B_TO_B: 
            newmap = GL_PIXEL_MAP_B_TO_B_SIZE;
            break;
        case GL_PIXEL_MAP_A_TO_A: 
            newmap = GL_PIXEL_MAP_A_TO_A_SIZE;
            break;
        default: 
            crError( "Bad map in crServerDispatchGetPixelMap: %d", map );
            break;
    }

    cr_server.head_spu->dispatch_table.GetIntegerv( newmap, &get_values );

    return get_values;
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetPixelMapfv( GLenum map, GLfloat *values )
{
#ifdef CR_ARB_pixel_buffer_object
    if (crStateIsBufferBound(GL_PIXEL_PACK_BUFFER_ARB))
    {
        GLvoid *pbo_offset;

        pbo_offset = (GLfloat*) ((uintptr_t) *((GLint*)values));

        cr_server.head_spu->dispatch_table.GetPixelMapfv( map, pbo_offset );
    }
    else
#endif
    {
        int size = sizeof( GLfloat );
        int tabsize = __sizeQuery( map );
        GLfloat *local_values;

        size *= tabsize;
        local_values = (GLfloat*)crAlloc( size );

        cr_server.head_spu->dispatch_table.GetPixelMapfv( map, local_values );
        crServerReturnValue( local_values, size );
        crFree( local_values );
    }
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetPixelMapuiv( GLenum map, GLuint *values )
{
#ifdef CR_ARB_pixel_buffer_object
    if (crStateIsBufferBound(GL_PIXEL_PACK_BUFFER_ARB))
    {
        GLvoid *pbo_offset;

        pbo_offset = (GLuint*) ((uintptr_t) *((GLint*)values));

        cr_server.head_spu->dispatch_table.GetPixelMapuiv( map, pbo_offset );
    }
    else
#endif
    {
        int size = sizeof( GLuint );
        int tabsize = __sizeQuery( map );
        GLuint *local_values;

        size *= tabsize;
        local_values = (GLuint*)crAlloc( size );

        cr_server.head_spu->dispatch_table.GetPixelMapuiv( map, local_values );
        crServerReturnValue( local_values, size );
        crFree( local_values );
    }
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetPixelMapusv( GLenum map, GLushort *values )
{
#ifdef CR_ARB_pixel_buffer_object
    if (crStateIsBufferBound(GL_PIXEL_PACK_BUFFER_ARB))
    {
        GLvoid *pbo_offset;

        pbo_offset = (GLushort*) ((uintptr_t) *((GLint*)values));

        cr_server.head_spu->dispatch_table.GetPixelMapusv( map, pbo_offset );
    }
    else
#endif
    {
        int size = sizeof( GLushort );
        int tabsize = __sizeQuery( map );
        GLushort *local_values;

        size *= tabsize;
        local_values = (GLushort*)crAlloc( size );

        cr_server.head_spu->dispatch_table.GetPixelMapusv( map, local_values );
        crServerReturnValue( local_values, size );
        crFree( local_values );
    }
}
