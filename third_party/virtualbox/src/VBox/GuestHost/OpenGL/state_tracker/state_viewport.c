/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"

void crStateViewportInit(CRContext *ctx)
{
	CRViewportState *v = &ctx->viewport;
	CRStateBits *sb = GetCurrentBits();
	CRViewportBits *vb = &(sb->viewport);
	CRTransformBits *tb = &(sb->transform);

	v->scissorTest = GL_FALSE;
	RESET(vb->enable, ctx->bitid);
	
	v->viewportValid = GL_FALSE;
	v->viewportX = 0;
	v->viewportY = 0;
	/* These are defaults, the tilesort spu overrides when
	 * the context has been created */
	v->viewportW = 640;
	v->viewportH = 480;
	RESET(vb->v_dims, ctx->bitid);

	v->scissorValid = GL_FALSE;
	v->scissorX = 0;
	v->scissorY = 0;
	/* These are defaults, the tilesort spu overrides when
	 * the context has been created */
	v->scissorW = 640;
	v->scissorH = 480;
	RESET(vb->s_dims, ctx->bitid);

	v->farClip = 1.0;
	v->nearClip = 0.0;
	RESET(vb->depth, ctx->bitid);

	RESET(vb->dirty, ctx->bitid);

	/* XXX why are these here? */
	RESET(tb->base, ctx->bitid);
	RESET(tb->dirty, ctx->bitid);
}

void crStateViewportApply(CRViewportState *v, GLvectorf *p) 
{
	p->x = (p->x+1.0f)*(v->viewportW / 2.0f) + v->viewportX;
	p->y = (p->y+1.0f)*(v->viewportH / 2.0f) + v->viewportY;
   	p->z = (GLfloat) ((p->z+1.0f)*((v->farClip - v->nearClip) / 2.0f) + v->nearClip);
}

void STATE_APIENTRY crStateViewport(GLint x, GLint y, GLsizei width, 
			GLsizei height) 
{
	CRContext *g = GetCurrentContext();
	CRViewportState *v = &(g->viewport);
	CRStateBits *sb = GetCurrentBits();
	CRViewportBits *vb = &(sb->viewport);
	CRTransformBits *tb = &(sb->transform);
	
	if (g->current.inBeginEnd)
	{
		crStateError( __LINE__, __FILE__, GL_INVALID_OPERATION,
									"calling glViewport() between glBegin/glEnd" );
		return;
	}

	FLUSH();
	
	if (width < 0 || height < 0)
	{
		crStateError( __LINE__, __FILE__, GL_INVALID_VALUE,
									"glViewport(bad width or height)" );
		return;
	}

	if (x > g->limits.maxViewportDims[0]) x = g->limits.maxViewportDims[0];
	if (x < -g->limits.maxViewportDims[0]) x = -g->limits.maxViewportDims[0]; 
	if (y > g->limits.maxViewportDims[1])	y = g->limits.maxViewportDims[1]; 
	if (y < -g->limits.maxViewportDims[1])	y = -g->limits.maxViewportDims[1]; 
	if (width > g->limits.maxViewportDims[0])  width = g->limits.maxViewportDims[0]; 
	if (height > g->limits.maxViewportDims[1])  height = g->limits.maxViewportDims[1];

	v->viewportX = (GLint) (x);
	v->viewportY = (GLint) (y);
	v->viewportW = (GLint) (width);
	v->viewportH = (GLint) (height);

	v->viewportValid = GL_TRUE;

	DIRTY(vb->v_dims, g->neg_bitid);
	DIRTY(vb->dirty, g->neg_bitid);
	/* XXX why are these here? */
	DIRTY(tb->base, g->neg_bitid);
	DIRTY(tb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateDepthRange(GLclampd znear, GLclampd zfar) 
{
	CRContext *g = GetCurrentContext();
	CRViewportState *v = &(g->viewport);
	CRStateBits *sb = GetCurrentBits();
	CRViewportBits *vb = &(sb->viewport);
	CRTransformBits *tb = &(sb->transform);

	if (g->current.inBeginEnd)
	{	
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glDepthRange called in Begin/End");
		return;
	}

	FLUSH();

	v->nearClip = znear;
	v->farClip = zfar;
	if (v->nearClip < 0.0) v->nearClip = 0.0;
	if (v->nearClip > 1.0) v->nearClip = 1.0;
	if (v->farClip < 0.0) v->farClip = 0.0;
	if (v->farClip > 1.0) v->farClip = 1.0;

	DIRTY(vb->depth, g->neg_bitid);
	DIRTY(vb->dirty, g->neg_bitid);
	DIRTY(tb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateScissor (GLint x, GLint y, 
					 GLsizei width, GLsizei height) 
{
	CRContext *g = GetCurrentContext();
	CRViewportState *v = &(g->viewport);
	CRStateBits *sb = GetCurrentBits();
	CRViewportBits *vb = &(sb->viewport);

	if (g->current.inBeginEnd) 
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
			"glScissor called in begin/end");
		return;
	}

	FLUSH();

	if (width < 0 || height < 0)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
			"glScissor called with negative width/height: %d,%d",
			width, height);
		return;
	}

	v->scissorX = (GLint) (x);
	v->scissorY = (GLint) (y);
	v->scissorW = (GLint) (width);
	v->scissorH = (GLint) (height);

	v->scissorValid = GL_TRUE;

	DIRTY(vb->s_dims, g->neg_bitid);
	DIRTY(vb->dirty, g->neg_bitid);
}
