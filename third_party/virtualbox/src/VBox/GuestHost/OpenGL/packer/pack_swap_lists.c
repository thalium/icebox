/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_error.h"
#include "cr_mem.h"

static int __gl_CallListsNumBytes( GLenum type )
{
    switch( type )
    {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
        case GL_2_BYTES:
            return 1;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        case GL_3_BYTES:
            return 2;
        case GL_INT:
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
        case GL_4_BYTES:
            return 4;
        default:
            return -1;
    }
}

void PACK_APIENTRY crPackCallListsSWAP(GLint n, GLenum type, 
        const GLvoid *lists )
{
    unsigned char *data_ptr;
    int packet_length;
    GLshort *shortPtr;
    GLint   *intPtr;
    int i;

    int bytesPerList = __gl_CallListsNumBytes( type );
    int numBytes = bytesPerList * n;

    if (numBytes < 0)
    {
        __PackError( __LINE__, __FILE__, GL_INVALID_ENUM,
                                 "crPackCallLists(bad type)" );
        return;
    }

    packet_length = sizeof( n ) + 
        sizeof( type ) + 
        numBytes;

    data_ptr = (unsigned char *) crPackAlloc( packet_length );
    WRITE_DATA( 0, GLint, SWAP32(n) );
    WRITE_DATA( 4, GLenum, SWAP32(type) );

    crMemcpy( data_ptr + 8, lists, numBytes );
    shortPtr = (GLshort *) (data_ptr + 8);
    intPtr   = (GLint *) (data_ptr + 8);

    if (bytesPerList > 1)
    {
        for ( i = 0 ; i < n ; i++)
        {
            switch( bytesPerList )
            {
                case 2:
                  *shortPtr = SWAP16(*shortPtr);
                  shortPtr+=1;
                    break;
                case 4:
                  *intPtr = SWAP32(*intPtr);
                  intPtr+=1;
                    break;
            }
        }
    }

    crHugePacket( CR_CALLLISTS_OPCODE, data_ptr );
    crPackFree( data_ptr );
}


void PACK_APIENTRY crPackNewListSWAP( GLuint list, GLenum mode )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    (void) pc;
    CR_CMDBLOCK_BEGIN( pc, CRPACKBLOCKSTATE_OP_NEWLIST );
    CR_GET_BUFFERED_POINTER_NO_BEGINEND_FLUSH( pc, 16, GL_FALSE );
    WRITE_DATA( 0, GLint, SWAP32(16) );
    WRITE_DATA( 4, GLenum, SWAP32(CR_NEWLIST_EXTEND_OPCODE) );
    WRITE_DATA( 8, GLuint, SWAP32(list) );
    WRITE_DATA( 12, GLenum, SWAP32(mode) );
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    pc->buffer.in_List = GL_TRUE;
    pc->buffer.holds_List = GL_TRUE;
    CR_UNLOCK_PACKER_CONTEXT(pc);
}


void PACK_APIENTRY crPackEndListSWAP( void )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    (void) pc;
    CR_GET_BUFFERED_POINTER( pc, 8 );
    WRITE_DATA( 0, GLint, SWAP32(8) );
    WRITE_DATA( 4, GLenum, SWAP32(CR_ENDLIST_EXTEND_OPCODE) );
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    pc->buffer.in_List = GL_FALSE;
    CR_CMDBLOCK_END( pc, CRPACKBLOCKSTATE_OP_NEWLIST );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}
