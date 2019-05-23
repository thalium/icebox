/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_error.h"
#include "cr_version.h"
#include "state.h"
#include "state/cr_statetypes.h"
#include "cr_extstring.h"
#include "cr_mem.h"
#include "cr_string.h"


const GLubyte * STATE_APIENTRY crStateGetString( GLenum name )
{
	CRContext *g = GetCurrentContext();
	if (!g)
		return NULL;

	switch( name )
	{
		case GL_VENDOR:
			return (const GLubyte *) CR_VENDOR;
		case GL_RENDERER:
			return (const GLubyte *) CR_RENDERER;
		case GL_VERSION:
			return (const GLubyte *) CR_OPENGL_VERSION_STRING " Chromium " CR_VERSION_STRING;
		case GL_EXTENSIONS:
			/* This shouldn't normally be queried - the relevant SPU should
			 * catch this query and do all the extension string merging/mucking.
			 */
			{
				static char *extensions = NULL;
				if (!extensions) {
					extensions = crAlloc(crStrlen(crExtensions) + crStrlen(crChromiumExtensions) + 2);
					crStrcpy(extensions, crExtensions);
					crStrcpy(extensions, " ");
					crStrcat(extensions, crChromiumExtensions);
				}
				return (const GLubyte *) extensions;
			}
#if defined(CR_ARB_vertex_program) || defined(CR_ARB_fragment_program)
   	case GL_PROGRAM_ERROR_STRING_ARB:
			return g->program.errorString;
#endif
		default:
			crStateError( __LINE__, __FILE__, GL_INVALID_ENUM,
									"calling glGetString() with invalid name" );
			return NULL;
	}

	(void) crAppOnlyExtensions;  /* silence warnings */
}
