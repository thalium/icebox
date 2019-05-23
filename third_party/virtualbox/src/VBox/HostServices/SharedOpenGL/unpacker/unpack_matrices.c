/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_mem.h"

void crUnpackMultMatrixd( void  )
{
	GLdouble m[16];
	crMemcpy( m, DATA_POINTER( 0, GLdouble ), sizeof(m) );

	cr_unpackDispatch.MultMatrixd( m );
	INCR_DATA_PTR( 16*sizeof( GLdouble ) );
}

void crUnpackMultMatrixf( void  )
{
	GLfloat *m = DATA_POINTER( 0, GLfloat );

	cr_unpackDispatch.MultMatrixf( m );
	INCR_DATA_PTR( 16*sizeof( GLfloat ) );
}

void crUnpackLoadMatrixd( void  )
{
	GLdouble m[16];
	crMemcpy( m, DATA_POINTER( 0, GLdouble ), sizeof(m) );

	cr_unpackDispatch.LoadMatrixd( m );
	INCR_DATA_PTR( 16*sizeof( GLdouble ) );
}

void crUnpackLoadMatrixf( void  )
{
	GLfloat *m = DATA_POINTER( 0, GLfloat );

	cr_unpackDispatch.LoadMatrixf( m );
	INCR_DATA_PTR( 16*sizeof( GLfloat ) );
}

void crUnpackExtendMultTransposeMatrixdARB( void  )
{
	GLdouble m[16];
	crMemcpy( m, DATA_POINTER( 8, GLdouble ), sizeof(m) );

	cr_unpackDispatch.MultTransposeMatrixdARB( m );
}

void crUnpackExtendMultTransposeMatrixfARB( void  )
{
	GLfloat *m = DATA_POINTER( 8, GLfloat );

	cr_unpackDispatch.MultTransposeMatrixfARB( m );
}

void crUnpackExtendLoadTransposeMatrixdARB( void  )
{
	GLdouble m[16];
	crMemcpy( m, DATA_POINTER( 8, GLdouble ), sizeof(m) );

	cr_unpackDispatch.LoadTransposeMatrixdARB( m );
}

void crUnpackExtendLoadTransposeMatrixfARB( void  )
{
	GLfloat *m = DATA_POINTER( 8, GLfloat );

	cr_unpackDispatch.LoadTransposeMatrixfARB( m );
}
