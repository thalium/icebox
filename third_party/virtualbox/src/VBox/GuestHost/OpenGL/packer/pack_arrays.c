/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_opcodes.h"
#include "cr_error.h"
#include "iprt/types.h"

#define UNUSED(x) ((void)(x))
/**
 * \mainpage Packer 
 *
 * \section PackerIntroduction Introduction
 *
 * Chromium consists of all the top-level files in the cr
 * directory.  The packer module basically takes care of API dispatch,
 * and OpenGL state management.
 *
 */

void PACK_APIENTRY crPackVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	/* Packing this command is only useful if we have server-side vertex
	 * arrays - GL_ARB_vertex_buffer_object.  Note that pointer will really
	 * be an offset into a server-side buffer.
     * @todo Because of that we'd only transfer lowest 32bit as there're no 4gb+VBOs (yet?).
     * Look at glgets regarding max vertices in arrays.
	 */
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
	int packet_length = 24;
	CR_GET_BUFFERED_POINTER( pc, packet_length );
	WRITE_DATA( 0, GLint, packet_length );
	WRITE_DATA( 4, GLenum, CR_VERTEXPOINTER_EXTEND_OPCODE );
	WRITE_DATA( 8, GLint, size );
	WRITE_DATA( 12, GLenum, type );
	WRITE_DATA( 16, GLsizei, stride );
	WRITE_DATA( 20, GLuint, (GLuint) ((uintptr_t) pointer) );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
	int packet_length = 24;
	CR_GET_BUFFERED_POINTER( pc, packet_length );
	WRITE_DATA( 0, GLint, packet_length );
	WRITE_DATA( 4, GLenum, CR_COLORPOINTER_EXTEND_OPCODE );
	WRITE_DATA( 8, GLint, size );
	WRITE_DATA( 12, GLenum, type );
	WRITE_DATA( 16, GLsizei, stride );
	WRITE_DATA( 20, GLuint, (GLuint) ((uintptr_t) pointer) );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackNormalPointer( GLenum type, GLsizei stride, const GLvoid *pointer )
{
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
	int packet_length = 20;
	CR_GET_BUFFERED_POINTER( pc, packet_length );
	WRITE_DATA( 0, GLint, packet_length );
	WRITE_DATA( 4, GLenum, CR_NORMALPOINTER_EXTEND_OPCODE );
	WRITE_DATA( 8, GLenum, type );
	WRITE_DATA( 12, GLsizei, stride );
	WRITE_DATA( 16, GLuint, (GLuint) ((uintptr_t) pointer) );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
	int packet_length = 24;
	CR_GET_BUFFERED_POINTER( pc, packet_length );
	WRITE_DATA( 0, GLint, packet_length );
	WRITE_DATA( 4, GLenum, CR_TEXCOORDPOINTER_EXTEND_OPCODE );
	WRITE_DATA( 8, GLint, size );
	WRITE_DATA( 12, GLenum, type );
	WRITE_DATA( 16, GLsizei, stride );
	WRITE_DATA( 20, GLuint, (GLuint) ((uintptr_t) pointer) );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackEdgeFlagPointer( GLsizei stride, const GLvoid *pointer )
{
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
	int packet_length = 16;
	CR_GET_BUFFERED_POINTER( pc, packet_length );
	WRITE_DATA( 0, GLint, packet_length );
	WRITE_DATA( 4, GLenum, CR_EDGEFLAGPOINTER_EXTEND_OPCODE );
	WRITE_DATA( 8, GLsizei, stride );
	WRITE_DATA( 12, GLuint, (GLuint) ((uintptr_t) pointer) );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackIndexPointer( GLenum type, GLsizei stride, const GLvoid *pointer )
{
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
	int packet_length = 20;
	CR_GET_BUFFERED_POINTER( pc, packet_length );
	WRITE_DATA( 0, GLint, packet_length );
	WRITE_DATA( 4, GLenum, CR_INDEXPOINTER_EXTEND_OPCODE );
	WRITE_DATA( 8, GLenum, type );
	WRITE_DATA( 12, GLsizei, stride );
	WRITE_DATA( 16, GLuint, (GLuint) ((uintptr_t) pointer) );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackSecondaryColorPointerEXT( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
	int packet_length = 24;
	CR_GET_BUFFERED_POINTER( pc, packet_length );
	WRITE_DATA( 0, GLint, packet_length );
	WRITE_DATA( 4, GLenum, CR_SECONDARYCOLORPOINTEREXT_EXTEND_OPCODE );
	WRITE_DATA( 8, GLint, size );
	WRITE_DATA( 12, GLenum, type );
	WRITE_DATA( 16, GLsizei, stride );
	WRITE_DATA( 20, GLuint, (GLuint) ((uintptr_t) pointer) );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackFogCoordPointerEXT( GLenum type, GLsizei stride, const GLvoid * pointer )
{
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
	int packet_length = 20;
	CR_GET_BUFFERED_POINTER( pc, packet_length );
	WRITE_DATA( 0, GLint, packet_length );
	WRITE_DATA( 4, GLenum, CR_FOGCOORDPOINTEREXT_EXTEND_OPCODE );
	WRITE_DATA( 8, GLenum, type );
	WRITE_DATA( 12, GLsizei, stride );
	WRITE_DATA( 16, GLuint, (GLuint) ((uintptr_t) pointer) );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackVertexAttribPointerARB( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer )
{
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
	int packet_length = 32;
	CR_GET_BUFFERED_POINTER( pc, packet_length );
	WRITE_DATA( 0, GLint, packet_length );
	WRITE_DATA( 4, GLenum, CR_VERTEXATTRIBPOINTERARB_EXTEND_OPCODE );
	WRITE_DATA( 8, GLint, index );
	WRITE_DATA( 12, GLint, size );
	WRITE_DATA( 16, GLenum, type );
	WRITE_DATA( 20, GLboolean, normalized );
	WRITE_DATA( 24, GLsizei, stride );
	WRITE_DATA( 28, GLuint, (GLuint) ((uintptr_t) pointer) );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackVertexAttribPointerNV( GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid *pointer )
{
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
	int packet_length = 28;
	CR_GET_BUFFERED_POINTER( pc, packet_length );
	WRITE_DATA( 0, GLint, packet_length );
	WRITE_DATA( 4, GLenum, CR_VERTEXATTRIBPOINTERNV_EXTEND_OPCODE );
	WRITE_DATA( 8, GLint, index );
	WRITE_DATA( 12, GLint, size );
	WRITE_DATA( 16, GLenum, type );
	WRITE_DATA( 20, GLsizei, stride );
	WRITE_DATA( 24, GLuint, (GLuint) ((uintptr_t) pointer) );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackInterleavedArrays( GLenum format, GLsizei stride, const GLvoid *pointer )
{
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
	int packet_length = 20;
	CR_GET_BUFFERED_POINTER( pc, packet_length );
	WRITE_DATA( 0, GLint, packet_length );
	WRITE_DATA( 4, GLenum, CR_INTERLEAVEDARRAYS_EXTEND_OPCODE );
	WRITE_DATA( 8, GLenum, format );
	WRITE_DATA( 12, GLsizei, stride );
	WRITE_DATA( 16, GLuint, (GLuint) ((uintptr_t) pointer) );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}


void PACK_APIENTRY crPackVertexArrayRangeNV( GLsizei length, const GLvoid *pointer )
{
	UNUSED( length );
	UNUSED( pointer );
	crWarning( "Unimplemented crPackVertexArrayRangeNV" );
}
