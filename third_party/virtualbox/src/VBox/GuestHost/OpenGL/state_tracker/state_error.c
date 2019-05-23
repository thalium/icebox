/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "state/cr_stateerror.h"
#include "state/cr_statetypes.h"
#include "state.h"
#include "cr_error.h"
#include "cr_environment.h"
#include <stdarg.h>
#include <stdio.h>

void crStateError( int line, const char *file, GLenum error, const char *format, ... )
{
	CRContext *g = GetCurrentContext();
	char errstr[8096];
	va_list args;

	CRASSERT(error != GL_NO_ERROR);

	if (g->error == GL_NO_ERROR)
	    g->error = error;

#ifndef DEBUG_misha
	if (crGetenv("CR_DEBUG"))
#endif
	{
		char *glerr;
		va_start( args, format );
		vsprintf( errstr, format, args );
		va_end( args );

		switch (error) {
		case GL_NO_ERROR:
			glerr = "GL_NO_ERROR";
			break;
		case GL_INVALID_VALUE:
			glerr = "GL_INVALID_VALUE";
			break;
		case GL_INVALID_ENUM:
			glerr = "GL_INVALID_ENUM";
			break;
		case GL_INVALID_OPERATION:
			glerr = "GL_INVALID_OPERATION";
			break;
		case GL_STACK_OVERFLOW:
			glerr = "GL_STACK_OVERFLOW";
			break;
		case GL_STACK_UNDERFLOW:
			glerr = "GL_STACK_UNDERFLOW";
			break;
		case GL_OUT_OF_MEMORY:
			glerr = "GL_OUT_OF_MEMORY";
			break;
		case GL_TABLE_TOO_LARGE:
			glerr = "GL_TABLE_TOO_LARGE";
			break;
		default:
			glerr = "unknown";
			break;
		}

		crWarning( "OpenGL error in %s, line %d: %s: %s\n",
							 file, line, glerr, errstr );
	}
}


GLenum STATE_APIENTRY crStateGetError(void)
{
	CRContext *g = GetCurrentContext();
	GLenum e = g->error;

	if (g->current.inBeginEnd)
	{
		crStateError( __LINE__, __FILE__, GL_INVALID_OPERATION,
									"glStateGetError() called between glBegin/glEnd" );
		return 0;
	}

	g->error = GL_NO_ERROR;

	return e;
}
