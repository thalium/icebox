/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackMaterialfv( void  )
{
	GLenum face = READ_DATA( sizeof( int ) + 0, GLenum );
	GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
	GLfloat *params = DATA_POINTER( sizeof( int ) + 8, GLfloat );

	cr_unpackDispatch.Materialfv( face, pname, params );
	INCR_VAR_PTR();
}

void crUnpackMaterialiv( void  )
{
	GLenum face = READ_DATA( sizeof( int ) + 0, GLenum );
	GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
	GLint *params = DATA_POINTER( sizeof( int ) + 8, GLint );

	cr_unpackDispatch.Materialiv( face, pname, params );
	INCR_VAR_PTR();
}
