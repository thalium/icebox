/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_string.h"
#include "cr_environment.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "feedbackspu.h"

#include <stdio.h>
#ifndef WINDOWS
#include <unistd.h>
#endif

static void __setDefaults( void )
{
	feedback_spu.render_mode = GL_RENDER;
}

static void set_default_viewport( void *foo, const char *response )
{
   (void) foo;
   sscanf( response, "%d", &(feedback_spu.default_viewport) );
}

/* option, type, nr, default, min, max, title, callback
 */
SPUOptions feedbackSPUOptions[] = {

   { "default_viewport", CR_BOOL, 1, "0", "0", "1",
     "Return default viewport parameters", (SPUOptionCB)set_default_viewport },

   { NULL, CR_BOOL, 0, NULL, NULL, NULL, NULL, NULL },

};


void feedbackspuGatherConfiguration( void )
{
	__setDefaults();
}
