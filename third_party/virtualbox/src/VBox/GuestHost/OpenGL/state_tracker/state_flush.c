/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "state.h"
#include "cr_spu.h"

void crStateFlushFunc(PCRStateTracker pState, CRStateFlushFunc func )
{
	CRContext *g = GetCurrentContext(pState);

	g->flush_func = func;
}

void crStateFlushArg(PCRStateTracker pState, void *arg )
{
	CRContext *g = GetCurrentContext(pState);

	g->flush_arg = arg;
}

void crStateDiffAPI(PCRStateTracker pState, SPUDispatchTable *api )
{
	if (!pState->diff_api.AlphaFunc) 
	{
		/* Called when starting up Chromium */
		crSPUInitDispatchTable( &pState->diff_api );
	}
	crSPUCopyDispatchTable( &pState->diff_api, api );
}
