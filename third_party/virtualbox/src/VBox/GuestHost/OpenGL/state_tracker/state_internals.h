/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef STATE_INTERNALS_H
#define STATE_INTERNALS_H

#include "cr_spu.h"
#include "state/cr_statetypes.h"

/* Set the flush_func to NULL *before* it's called, so that we can 
 * call state functions from within flush without infinite recursion. 
 * Yucky, but "necessary" for color material. */

#define FLUSH() \
	if (g->flush_func != NULL) \
	{ \
		CRStateFlushFunc cached_ff = g->flush_func; \
		g->flush_func = NULL; \
		cached_ff( g->flush_arg ); \
	}

typedef void (SPU_APIENTRY *glAble)(GLenum);

#define GLCLIENT_BIT_ALLOC 1024

#endif
