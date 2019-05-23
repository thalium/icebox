/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "chromium.h"
#include "cr_error.h" 
#include "server_dispatch.h"
#include "server.h"
#include "cr_string.h"

const GLubyte * SERVER_DISPATCH_APIENTRY crServerDispatchGetString( GLenum name )
{
	const GLubyte *retval;
	retval = cr_server.head_spu->dispatch_table.GetString( name );
	if (retval)
		crServerReturnValue( retval, crStrlen((char *)retval) + 1 );
	else
		crServerReturnValue( "", 1 ); /* empty string */
	return retval; /* WILL PROBABLY BE IGNORED */
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetProgramStringNV( GLuint id, GLenum pname, GLubyte * program )
{
	crError( "glGetProgramStringNV isn't *ever* allowed to be on the wire!" );
	(void) id;
	(void) pname;
	(void) program;
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGetProgramStringARB( GLuint id, GLenum pname, void * program )
{
	crError( "glGetProgramStringARB isn't *ever* allowed to be on the wire!" );
	(void) id;
	(void) pname;
	(void) program;
}
