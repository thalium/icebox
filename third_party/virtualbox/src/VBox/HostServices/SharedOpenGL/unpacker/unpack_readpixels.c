/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_pixeldata.h"
#include "cr_mem.h"

void crUnpackReadPixels( void )
{
	GLint x        = READ_DATA( 0, GLint );
	GLint y        = READ_DATA( 4, GLint );
	GLsizei width  = READ_DATA( 8, GLsizei );
	GLsizei height = READ_DATA( 12, GLsizei );
	GLenum format  = READ_DATA( 16, GLenum );
	GLenum type    = READ_DATA( 20, GLenum );
	GLint stride   = READ_DATA( 24, GLint );
	GLint alignment     = READ_DATA( 28, GLint );
	GLint skipRows      = READ_DATA( 32, GLint );
	GLint skipPixels    = READ_DATA( 36, GLint );
	GLint bytes_per_row = READ_DATA( 40, GLint );
	GLint rowLength     = READ_DATA( 44, GLint );
	GLvoid *pixels;

	/* point <pixels> at the 8-byte network pointer */
	pixels = DATA_POINTER( 48, GLvoid );

	(void) stride;
	(void) bytes_per_row;
	(void) alignment;
	(void) skipRows;
	(void) skipPixels;
	(void) rowLength;

	/* we always pack densely on the server side! */
	cr_unpackDispatch.PixelStorei( GL_PACK_ROW_LENGTH, 0 );
	cr_unpackDispatch.PixelStorei( GL_PACK_SKIP_PIXELS, 0 );
	cr_unpackDispatch.PixelStorei( GL_PACK_SKIP_ROWS, 0 );
	cr_unpackDispatch.PixelStorei( GL_PACK_ALIGNMENT, 1 );

	cr_unpackDispatch.ReadPixels( x, y, width, height, format, type, pixels);

	INCR_DATA_PTR(48+sizeof(CRNetworkPointer));
}
