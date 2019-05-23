/* $Id: dlm_error.c $ */
#include <stdio.h>
#include <stdarg.h>
#include "chromium.h"
#include "cr_mem.h"
#include "dlm.h"
#include "cr_environment.h"
#include "cr_error.h"

#define GLCLIENT_LIST_ALLOC 1024

void crdlmWarning( int line, char *file, GLenum error, char *format, ... )
{
    char errstr[8096];
    va_list args;

    if (crGetenv("CR_DEBUG")) {
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

	crWarning( "DLM error in %s, line %d: %s: %s\n",
	    file, line, glerr, errstr );
    }
}
