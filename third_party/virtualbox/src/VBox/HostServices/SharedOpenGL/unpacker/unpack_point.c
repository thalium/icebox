/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackExtendPointParameterfvARB( void  )
{
	GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
	GLfloat *params = DATA_POINTER( sizeof( int ) + 8, GLfloat );
	cr_unpackDispatch.PointParameterfvARB( pname, params );
}

#if 1
void crUnpackExtendPointParameteriv( void  )
{
	GLenum pname = READ_DATA( sizeof( int ) + 0, GLenum );
	GLint *params = DATA_POINTER( sizeof( int ) + 4, GLint );

	cr_unpackDispatch.PointParameteriv( pname, params );
}
#endif
