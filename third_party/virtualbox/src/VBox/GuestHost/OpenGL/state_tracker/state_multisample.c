/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"

void crStateMultisampleInit (CRContext *ctx)
{
	CRMultisampleState *m = &ctx->multisample;
	CRStateBits *sb       = GetCurrentBits();
	CRMultisampleBits *mb = &(sb->multisample);

	m->enabled = GL_FALSE; /* TRUE if the visual supports it */
	m->sampleAlphaToCoverage = GL_FALSE;
	m->sampleAlphaToOne = GL_FALSE;
	m->sampleCoverage = GL_FALSE;
	RESET(mb->enable, ctx->bitid);

	m->sampleCoverageValue = 1.0F;
	m->sampleCoverageInvert = GL_FALSE;
	RESET(mb->sampleCoverageValue, ctx->bitid);

	RESET(mb->dirty, ctx->bitid);
}

void STATE_APIENTRY crStateSampleCoverageARB(GLclampf value, GLboolean invert)
{
	CRContext *g          = GetCurrentContext();
	CRMultisampleState *m = &(g->multisample);
	CRStateBits *sb       = GetCurrentBits();
	CRMultisampleBits *mb = &(sb->multisample);

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glStateSampleCoverageARB called in begin/end");
		return;
	}

	FLUSH();

	m->sampleCoverageValue = value;
	m->sampleCoverageInvert = invert;
	DIRTY(mb->dirty, g->neg_bitid);
	DIRTY(mb->sampleCoverageValue, g->neg_bitid);
}

