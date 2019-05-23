/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackCallLists( void  )
{
	GLint n = READ_DATA( sizeof( int ) + 0, GLint );
	GLenum type = READ_DATA( sizeof( int ) + 4, GLenum );
	GLvoid *lists = DATA_POINTER( sizeof( int ) + 8, GLvoid );

	cr_unpackDispatch.CallLists( n, type, lists );
	INCR_VAR_PTR();
}
