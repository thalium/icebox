/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_error.h"
#include "cr_string.h"
#include "cr_version.h"


void PACK_APIENTRY
crPackMapBufferARB( GLenum target, GLenum access,
            void * return_value, int * writeback )
{
     (void)writeback;
     (void)return_value;
     (void)target;
     (void)access;
     crWarning("Can't pack MapBufferARB command!");
}


void PACK_APIENTRY
crPackUnmapBufferARB( GLenum target, GLboolean* return_value, int * writeback )
{
     (void)target;
     (void)return_value;
     (void)writeback;
     crWarning("Can't pack UnmapBufferARB command!");
}


void PACK_APIENTRY
crPackBufferDataARB( GLenum target, GLsizeiptrARB size,
                                         const GLvoid * data, GLenum usage )
{
    unsigned char *data_ptr, *start_ptr;
    int packet_length;

    packet_length = sizeof(GLenum)
        + sizeof(target) + sizeof(GLuint) + sizeof(usage) + sizeof(GLint);

    /*Note: it's valid to pass a NULL pointer here, which tells GPU drivers to allocate memory for the VBO*/
    if (data) packet_length += size;

    start_ptr = data_ptr = (unsigned char *) crPackAlloc( packet_length );

    WRITE_DATA_AI(GLenum, CR_BUFFERDATAARB_EXTEND_OPCODE);
    WRITE_DATA_AI(GLenum, target);
    WRITE_DATA_AI(GLuint, (GLuint) size);
    WRITE_DATA_AI(GLenum, usage);
    WRITE_DATA_AI(GLint, (GLint) (data!=NULL));
    if (data)
         crMemcpy(data_ptr, data, size);

    crHugePacket( CR_EXTEND_OPCODE, start_ptr );
    crPackFree( start_ptr );
}


void PACK_APIENTRY
crPackBufferSubDataARB( GLenum target, GLintptrARB offset, GLsizeiptrARB size,
                                                const GLvoid * data )
{
    unsigned char *data_ptr, *start_ptr;
    int packet_length;

    if (!data)
        return;

    packet_length = sizeof(GLenum)
        + sizeof(target) + sizeof(GLuint) + sizeof(GLuint) + size;

    start_ptr = data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA_AI(GLenum, CR_BUFFERSUBDATAARB_EXTEND_OPCODE);
    WRITE_DATA_AI(GLenum, target);
    WRITE_DATA_AI(GLuint, (GLuint) offset);
    WRITE_DATA_AI(GLuint, (GLuint) size);
    crMemcpy(data_ptr, data, size);

    crHugePacket(CR_EXTEND_OPCODE, start_ptr);
    crPackFree(start_ptr);
}

void PACK_APIENTRY 
crPackGetBufferSubDataARB( GLenum target, GLintptrARB offset, GLsizeiptrARB size, void * data, int * writeback )
{
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
	(void) pc;
	CR_GET_BUFFERED_POINTER( pc, 36 );
	WRITE_DATA( 0, GLint, 36 );
	WRITE_DATA( 4, GLenum, CR_GETBUFFERSUBDATAARB_EXTEND_OPCODE );
	WRITE_DATA( 8, GLenum, target );
	WRITE_DATA( 12, GLuint, (GLuint) offset );
	WRITE_DATA( 16, GLuint, (GLuint) size );
	WRITE_NETWORK_POINTER( 20, (void *) data );
	WRITE_NETWORK_POINTER( 28, (void *) writeback );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
	CR_CMDBLOCK_CHECK_FLUSH(pc);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY
crPackDeleteBuffersARB(GLsizei n, const GLuint * buffers)
{
    unsigned char *data_ptr;
    int packet_length = sizeof(GLenum) + sizeof(n) + n * sizeof(*buffers);

    if (!buffers)
        return;

    data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA( 0, GLenum, CR_DELETEBUFFERSARB_EXTEND_OPCODE );
    WRITE_DATA( 4, GLsizei, n );
    crMemcpy( data_ptr + 8, buffers, n * sizeof(*buffers) );
    crHugePacket( CR_EXTEND_OPCODE, data_ptr );
    crPackFree( data_ptr );
}
