/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_mem.h"

/* XXX duplicated in pack_context.c */
#define DISPLAY_NAME_LEN 256

#define READ_BYTES(a_pState, dest, offset, len ) \
    crMemcpy( dest, ((a_pState)->pbUnpackData + (offset)), len )

void crUnpackExtendCreateContext(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 24 + DISPLAY_NAME_LEN, sizeof(CRNetworkPointer));

	char dpyName[DISPLAY_NAME_LEN];
	GLint visBits = READ_DATA(pState, DISPLAY_NAME_LEN + 8, GLint );
	GLint shareCtx = READ_DATA(pState, DISPLAY_NAME_LEN + 12, GLint );
	GLint retVal;

	READ_BYTES(pState, dpyName, 8, DISPLAY_NAME_LEN );
	dpyName[DISPLAY_NAME_LEN - 1] = 0; /* NULL-terminate, just in case */

	SET_RETURN_PTR(pState, DISPLAY_NAME_LEN + 16 );
	SET_WRITEBACK_PTR(pState, DISPLAY_NAME_LEN + 24 );
	retVal = pState->pDispatchTbl->CreateContext( dpyName, visBits, shareCtx );
	(void) retVal;
}

void crUnpackExtendWindowCreate(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 20 + DISPLAY_NAME_LEN, sizeof(CRNetworkPointer));

	char dpyName[DISPLAY_NAME_LEN];
	GLint visBits = READ_DATA(pState, DISPLAY_NAME_LEN + 8, GLint );
	GLint retVal;

	READ_BYTES(pState, dpyName, 8, DISPLAY_NAME_LEN );
	dpyName[DISPLAY_NAME_LEN - 1] = 0; /* NULL-terminate, just in case */

	SET_RETURN_PTR(pState, DISPLAY_NAME_LEN + 12 );
	SET_WRITEBACK_PTR(pState, DISPLAY_NAME_LEN + 20 );
	retVal = pState->pDispatchTbl->WindowCreate( dpyName, visBits );
	(void) retVal;
}

