/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_mem.h"
#include "cr_glstate.h"

static unsigned char * __gl_HandlePixelMapData(GLenum map, GLsizei mapsize, int size_of_value, const GLvoid *values)
{
    int nodata = (values == NULL) || crStateIsBufferBound(GL_PIXEL_UNPACK_BUFFER_ARB);
    int packet_length = 
        sizeof( map ) + 
        sizeof( mapsize ) + sizeof(int) + sizeof(GLint);
    unsigned char *data_ptr;

    if (!nodata)
    {
        packet_length += mapsize*size_of_value;
    }

    data_ptr = (unsigned char *) crPackAlloc( packet_length );

    WRITE_DATA( 0, GLenum, map );
    WRITE_DATA( 4, GLsizei, mapsize );
    WRITE_DATA( 8, int, nodata);
    WRITE_DATA( 12, GLint, (GLint)(uintptr_t)values);

    if (!nodata)
    {
        crMemcpy( data_ptr + 16, values, mapsize*size_of_value );
    }

    return data_ptr;
}

void PACK_APIENTRY crPackPixelMapfv(GLenum map, GLsizei mapsize, 
                                    const GLfloat *values)
{
    unsigned char *data_ptr = __gl_HandlePixelMapData( map, mapsize, sizeof( *values ), values );

    crHugePacket( CR_PIXELMAPFV_OPCODE, data_ptr );
    crPackFree( data_ptr );
}

void PACK_APIENTRY crPackPixelMapuiv(GLenum map, GLsizei mapsize, 
                                     const GLuint *values)
{
    unsigned char *data_ptr = __gl_HandlePixelMapData( map, mapsize, sizeof( *values ), values );

    crHugePacket( CR_PIXELMAPUIV_OPCODE, data_ptr );
    crPackFree( data_ptr );
}

void PACK_APIENTRY crPackPixelMapusv(GLenum map, GLsizei mapsize, 
                                     const GLushort *values)
{
    unsigned char *data_ptr = __gl_HandlePixelMapData( map, mapsize, sizeof( *values ), values );

    crHugePacket( CR_PIXELMAPUSV_OPCODE, data_ptr );
    crPackFree( data_ptr );
}
