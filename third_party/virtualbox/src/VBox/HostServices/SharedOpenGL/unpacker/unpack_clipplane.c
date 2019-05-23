/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_mem.h"
#include "unpack_extend.h"

void crUnpackClipPlane( void  )
{
	GLenum plane = READ_DATA( 0, GLenum );
	GLdouble equation[4];
	crMemcpy( equation, DATA_POINTER( 4, GLdouble ), sizeof(equation) );

	cr_unpackDispatch.ClipPlane( plane, equation );
	INCR_DATA_PTR( sizeof( GLenum ) + 4*sizeof( GLdouble ));
}
