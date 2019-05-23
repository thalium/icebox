/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackLightfv( void  )
{
	GLenum light = READ_DATA( sizeof( int ) + 0, GLenum );
	GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
	GLfloat *params = DATA_POINTER( sizeof( int ) + 8, GLfloat );

	cr_unpackDispatch.Lightfv( light, pname, params );
	INCR_VAR_PTR();
}

void crUnpackLightiv( void  )
{
	GLenum light = READ_DATA( sizeof( int ) + 0, GLenum );
	GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
	GLint *params = DATA_POINTER( sizeof( int ) + 8, GLint );

	cr_unpackDispatch.Lightiv( light, pname, params );
	INCR_VAR_PTR();
}

void crUnpackLightModelfv( void  )
{
	GLenum pname = READ_DATA( sizeof( int ) + 0, GLenum );
	GLfloat *params = DATA_POINTER( sizeof( int ) + 4, GLfloat );

	cr_unpackDispatch.LightModelfv( pname, params );
	INCR_VAR_PTR();
}

void crUnpackLightModeliv( void  )
{
	GLenum pname = READ_DATA( sizeof( int ) + 0, GLenum );
	GLint *params = DATA_POINTER( sizeof( int ) + 4, GLint );

	cr_unpackDispatch.LightModeliv( pname, params );
	INCR_VAR_PTR();
}
