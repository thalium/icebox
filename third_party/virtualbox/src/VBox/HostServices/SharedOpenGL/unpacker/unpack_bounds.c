/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "state/cr_statetypes.h"

void crUnpackBoundsInfoCR( void  )
{
	CRrecti bounds;
	GLint len;
	GLuint num_opcodes;
	GLbyte *payload;
	
	len = READ_DATA( 0, GLint );
	bounds.x1 = READ_DATA( 4, GLint );
	bounds.y1 = READ_DATA( 8, GLint );
	bounds.x2 = READ_DATA( 12, GLint );
	bounds.y2 = READ_DATA( 16, GLint );
	num_opcodes = READ_DATA( 20, GLuint );
	payload = DATA_POINTER( 24, GLbyte );

	cr_unpackDispatch.BoundsInfoCR( &bounds, payload, len, num_opcodes );
	INCR_VAR_PTR();
}
