/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_string.h"
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


void feedbackspuGatherConfiguration( void )
{
	__setDefaults();
}
