/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "state/cr_statetypes.h"

void crUnpackBoundsInfoCR(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC(pState, 24 + sizeof(GLbyte));

	CRrecti bounds;
	GLint len;
	GLuint num_opcodes;
	GLbyte *payload;

	len = READ_DATA(pState, 0, GLint );
	bounds.x1 = READ_DATA(pState, 4, GLint );
	bounds.y1 = READ_DATA(pState, 8, GLint );
	bounds.x2 = READ_DATA(pState, 12, GLint );
	bounds.y2 = READ_DATA(pState, 16, GLint );
	num_opcodes = READ_DATA(pState, 20, GLuint );
	payload = DATA_POINTER(pState, 24, GLbyte );

	pState->pDispatchTbl->BoundsInfoCR( &bounds, payload, len, num_opcodes );
	INCR_VAR_PTR(pState);
}
