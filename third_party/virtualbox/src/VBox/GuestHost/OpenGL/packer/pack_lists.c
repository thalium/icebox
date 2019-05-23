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

void PACK_APIENTRY crPackCallLists(GLint n, GLenum type, 
        const GLvoid *lists )
{
    unsigned char *data_ptr;
    int packet_length;

    int num_bytes = __gl_CallListsNumBytes( type ) * n;
    if (num_bytes < 0)
    {
        __PackError( __LINE__, __FILE__, GL_INVALID_ENUM,
                                 "crPackCallLists(bad type)" );
        return;
    }

    packet_length = 
        sizeof( n ) + 
        sizeof( type ) + 
        num_bytes;

    data_ptr = (unsigned char *) crPackAlloc( packet_length );
    WRITE_DATA( 0, GLint, n );
    WRITE_DATA( 4, GLenum, type );
    crMemcpy( data_ptr + 8, lists, num_bytes );

    crHugePacket( CR_CALLLISTS_OPCODE, data_ptr );
    crPackFree( data_ptr );
}

void PACK_APIENTRY crPackNewList( GLuint list, GLenum mode )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    (void) pc;

    if (CR_CMDBLOCK_IS_STARTED(pc, CRPACKBLOCKSTATE_OP_NEWLIST))
    {
        WARN(("recursive NewList?"));
        return;
    }

    CR_CMDBLOCK_BEGIN( pc, CRPACKBLOCKSTATE_OP_NEWLIST );
    CR_GET_BUFFERED_POINTER_NO_BEGINEND_FLUSH( pc, 16, GL_FALSE );
    WRITE_DATA( 0, GLint, 16 );
    WRITE_DATA( 4, GLenum, CR_NEWLIST_EXTEND_OPCODE );
    WRITE_DATA( 8, GLuint, list );
    WRITE_DATA( 12, GLenum, mode );
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    pc->buffer.in_List = GL_TRUE;
    pc->buffer.holds_List = GL_TRUE;
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackEndList( void )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    CR_GET_BUFFERED_POINTER( pc, 8 );
    WRITE_DATA( 0, GLint, 8 );
    WRITE_DATA( 4, GLenum, CR_ENDLIST_EXTEND_OPCODE );
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    pc->buffer.in_List = GL_FALSE;
    CR_CMDBLOCK_END( pc, CRPACKBLOCKSTATE_OP_NEWLIST );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}
