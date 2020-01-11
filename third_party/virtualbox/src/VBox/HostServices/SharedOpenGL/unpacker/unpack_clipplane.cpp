/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_mem.h"
#include "unpack_extend.h"

void crUnpackClipPlane(PCrUnpackerState pState)
{
	GLdouble equation[4];
    CHECK_BUFFER_SIZE_STATIC(pState, 4 + sizeof(equation));

	GLenum plane = READ_DATA(pState, 0, GLenum );
	crMemcpy( equation, DATA_POINTER(pState, 4, GLdouble ), sizeof(equation) );

	pState->pDispatchTbl->ClipPlane( plane, equation );
	INCR_DATA_PTR(pState, sizeof( GLenum ) + 4*sizeof( GLdouble ));
}
