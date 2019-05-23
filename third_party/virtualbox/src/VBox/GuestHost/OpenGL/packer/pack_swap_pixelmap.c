/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"

static unsigned char * __gl_HandlePixelMapData( GLenum map, GLsizei mapsize, int size_of_value, const GLvoid *values )
{
    int i;

    int packet_length = 
        sizeof( map ) + 
        sizeof( mapsize ) + 
        mapsize*size_of_value;
    unsigned char *data_ptr = (unsigned char *) crPackAlloc( packet_length );

    WRITE_DATA( 0, GLenum, SWAP32(map) );
    WRITE_DATA( 4, GLsizei, SWAP32(mapsize) );

    for (i = 0 ; i < mapsize ; i++)
    {
        switch( size_of_value )
        {
            case 2:
                WRITE_DATA( 8 + i*sizeof(GLshort), GLshort, SWAP16(*((GLshort *)values + i) ));
                break;
            case 4:
                WRITE_DATA( 8 + i*sizeof(GLint), GLint, SWAP32(*((GLint *)values + i) ));
                break;
        }
    }
    return data_ptr;
}

void PACK_APIENTRY crPackPixelMapfvSWAP(GLenum map, GLsizei mapsize, 
        const GLfloat *values )
{
    unsigned char *data_ptr = __gl_HandlePixelMapData( map, mapsize, sizeof( *values ), values );

    crHugePacket( CR_PIXELMAPFV_OPCODE, data_ptr );
    crPackFree( data_ptr );
}

void PACK_APIENTRY crPackPixelMapuivSWAP(GLenum map, GLsizei mapsize, 
        const GLuint *values )
{
    unsigned char *data_ptr = __gl_HandlePixelMapData( map, mapsize, sizeof( *values ), values );

    crHugePacket( CR_PIXELMAPUIV_OPCODE, data_ptr );
    crPackFree( data_ptr );
}

void PACK_APIENTRY crPackPixelMapusvSWAP(GLenum map, GLsizei mapsize, 
        const GLushort *values )
{
    unsigned char *data_ptr = __gl_HandlePixelMapData( map, mapsize, sizeof( *values ), values );

    crHugePacket( CR_PIXELMAPUSV_OPCODE, data_ptr );
    crPackFree( data_ptr );
}
