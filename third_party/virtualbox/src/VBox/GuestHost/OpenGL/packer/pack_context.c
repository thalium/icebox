/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_opcodes.h"
#include "cr_mem.h"
#include "cr_string.h"


/* XXX duplicated in unpack_context.c */
#define DISPLAY_NAME_LEN 256

#define WRITE_BYTES( offset, data, len ) \
  crMemcpy( data_ptr + (offset), data, len )

void PACK_APIENTRY
crPackCreateContext( const char *dpyName, GLint visual, GLint shareCtx,
                     GLint *return_value, int *writeback )
{
	char displayName[DISPLAY_NAME_LEN];
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
        int len = DISPLAY_NAME_LEN + 32;

	/* clear the buffer, to silence valgrind */
	crMemZero(displayName, DISPLAY_NAME_LEN);

	if (dpyName) {
		crStrncpy( displayName, dpyName, DISPLAY_NAME_LEN );
		displayName[DISPLAY_NAME_LEN - 1] = 0;
	}
	else {
		displayName[0] = 0;
	}

	CR_GET_BUFFERED_POINTER(pc, len);
	WRITE_DATA( 0, GLint, len );
	WRITE_DATA( 4, GLenum, CR_CREATECONTEXT_EXTEND_OPCODE );
	WRITE_BYTES( 8, displayName, DISPLAY_NAME_LEN );
	WRITE_DATA( DISPLAY_NAME_LEN + 8, GLint, visual );
	WRITE_DATA( DISPLAY_NAME_LEN + 12, GLint, shareCtx );
	WRITE_NETWORK_POINTER( DISPLAY_NAME_LEN + 16, (void *) return_value );
	WRITE_NETWORK_POINTER( DISPLAY_NAME_LEN + 24, (void *) writeback );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
	CR_CMDBLOCK_CHECK_FLUSH(pc);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY
crPackCreateContextSWAP( const char *dpyName, GLint visual, GLint shareCtx,
                         GLint *return_value, int *writeback )
{
	char displayName[DISPLAY_NAME_LEN];
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;
        int len = DISPLAY_NAME_LEN + 32;

	/* clear the buffer, to silence valgrind */
	crMemZero(displayName, DISPLAY_NAME_LEN);

	if (dpyName) {
		crStrncpy( displayName, dpyName, DISPLAY_NAME_LEN );
		displayName[DISPLAY_NAME_LEN - 1] = 0;
	}
	else {
		displayName[0] = 0;
	}

	CR_GET_BUFFERED_POINTER(pc, len);
	WRITE_DATA( 0, GLint, SWAP32(len) );
	WRITE_DATA( 4, GLenum, SWAP32(CR_CREATECONTEXT_EXTEND_OPCODE) );
	WRITE_BYTES( 8, displayName, DISPLAY_NAME_LEN );
	WRITE_DATA( DISPLAY_NAME_LEN + 8, GLenum, SWAP32(visual) );
	WRITE_DATA( DISPLAY_NAME_LEN + 12, GLenum, SWAP32(shareCtx) );
	WRITE_NETWORK_POINTER( DISPLAY_NAME_LEN + 16, (void *) return_value );
	WRITE_NETWORK_POINTER( DISPLAY_NAME_LEN + 24, (void *) writeback );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
	CR_CMDBLOCK_CHECK_FLUSH(pc);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}


void PACK_APIENTRY crPackWindowCreate( const char *dpyName, GLint visBits, GLint *return_value, int *writeback )
{
	char displayName[DISPLAY_NAME_LEN];
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;

	/* clear the buffer, to silence valgrind */
	crMemZero(displayName, DISPLAY_NAME_LEN);

	if (dpyName) {
		crStrncpy( displayName, dpyName, DISPLAY_NAME_LEN );
		displayName[DISPLAY_NAME_LEN - 1] = 0;
	}
	else {
		displayName[0] = 0;
	}

	CR_GET_BUFFERED_POINTER(pc, DISPLAY_NAME_LEN + 28 );
	WRITE_DATA( 0, GLint, 28 );
	WRITE_DATA( 4, GLenum, CR_WINDOWCREATE_EXTEND_OPCODE );
	WRITE_BYTES( 8, displayName, DISPLAY_NAME_LEN );
	WRITE_DATA( DISPLAY_NAME_LEN + 8, GLint, visBits );
	WRITE_NETWORK_POINTER( DISPLAY_NAME_LEN + 12, (void *) return_value );
	WRITE_NETWORK_POINTER( DISPLAY_NAME_LEN + 20, (void *) writeback );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
	CR_CMDBLOCK_CHECK_FLUSH(pc);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackWindowCreateSWAP( const char *dpyName, GLint visBits, GLint *return_value, int *writeback )
{
	char displayName[DISPLAY_NAME_LEN];
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;

	/* clear the buffer, to silence valgrind */
	crMemZero(displayName, DISPLAY_NAME_LEN);

	if (dpyName) {
		crStrncpy( displayName, dpyName, DISPLAY_NAME_LEN );
		displayName[DISPLAY_NAME_LEN - 1] = 0;
	}
	else {
		displayName[0] = 0;
	}

	CR_GET_BUFFERED_POINTER(pc, DISPLAY_NAME_LEN + 28 );
	WRITE_DATA( 0, GLint, SWAP32(28) );
	WRITE_DATA( 4, GLenum, SWAP32(CR_WINDOWCREATE_EXTEND_OPCODE) );
	WRITE_BYTES( 8, displayName, DISPLAY_NAME_LEN );
	WRITE_DATA( DISPLAY_NAME_LEN + 8, GLint, SWAP32(visBits) );
	WRITE_NETWORK_POINTER( DISPLAY_NAME_LEN + 12, (void *) return_value );
	WRITE_NETWORK_POINTER( DISPLAY_NAME_LEN + 20, (void *) writeback );
	WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
	CR_CMDBLOCK_CHECK_FLUSH(pc);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}
