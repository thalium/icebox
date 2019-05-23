/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackExtendCombinerParameterfvNV( void  )
{
	GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
	GLfloat *params = DATA_POINTER( sizeof( int ) + 8, GLfloat );

	cr_unpackDispatch.CombinerParameterfvNV( pname, params );
        /* Don't call INCR_VAR_PTR(); - it's done in crUnpackExtend() */
}

void crUnpackExtendCombinerParameterivNV( void  )
{
	GLenum pname = READ_DATA( sizeof( int ) + 4, GLenum );
	GLint *params = DATA_POINTER( sizeof( int ) + 8, GLint );

	cr_unpackDispatch.CombinerParameterivNV( pname, params );
        /* Don't call INCR_VAR_PTR(); - it's done in crUnpackExtend() */
}

void crUnpackExtendCombinerStageParameterfvNV( void  )
{
	GLenum stage = READ_DATA( sizeof( int ) + 4, GLenum );
	GLenum pname = READ_DATA( sizeof( int ) + 8, GLenum );
	GLfloat *params = DATA_POINTER( sizeof( int ) + 12, GLfloat );

	cr_unpackDispatch.CombinerStageParameterfvNV( stage, pname, params );
        /* Don't call INCR_VAR_PTR(); - it's done in crUnpackExtend() */
}
