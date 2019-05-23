/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

/* 
 * The majority of this file is pulled from Mesa 4.0.x with the
 * permission of Brian Paul
 */

#include <stdio.h>
#include "cr_mem.h"
#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"

/* free the 1-D evaluator map */
static void
free_1d_map(CRContext *ctx, GLenum map)
{
	CREvaluatorState *e = &ctx->eval;
	const GLint k = map - GL_MAP1_COLOR_4;

	crFree( e->eval1D[k].coeff );
}

/* free the 2-D evaluator map */
static void
free_2d_map(CRContext *ctx, GLenum map)
{
	CREvaluatorState *e = &ctx->eval;
	const GLint k = map - GL_MAP2_COLOR_4;

	crFree( e->eval2D[k].coeff );
}

/* Initialize a 1-D evaluator map */
static void
init_1d_map(CRContext *ctx, GLenum map, int n, const float *initial)
{
	CREvaluatorState *e = &ctx->eval;
	CRStateBits *sb = GetCurrentBits();
	CREvaluatorBits *eb = &(sb->eval);
	GLint i;
	const GLint k = map - GL_MAP1_COLOR_4;
	CRASSERT(k >= 0);
	CRASSERT(k < GLEVAL_TOT);
	e->eval1D[k].u1 = 0.0;
	e->eval1D[k].u2 = 1.0;
	e->eval1D[k].du = 0.0;
	e->eval1D[k].order = 1;
	e->eval1D[k].coeff = (GLfloat *) crAlloc(n * sizeof(GLfloat));
	for (i = 0; i < n; i++)
		e->eval1D[k].coeff[i] = initial[i];
	RESET(eb->eval1D[i], ctx->bitid);
}


/* Initialize a 2-D evaluator map */
static void
init_2d_map(CRContext *ctx, GLenum map, int n, const float *initial)
{
	CREvaluatorState *e = &ctx->eval;
	CRStateBits *sb = GetCurrentBits();
	CREvaluatorBits *eb = &(sb->eval);
	GLint i;
	const GLint k = map - GL_MAP2_COLOR_4;
	CRASSERT(k >= 0);
	CRASSERT(k < GLEVAL_TOT);
	e->eval2D[k].u1 = 0.0;
	e->eval2D[k].u2 = 1.0;
	e->eval2D[k].du = 0.0;
	e->eval2D[k].v1 = 0.0;
	e->eval2D[k].v2 = 1.0;
	e->eval2D[k].dv = 0.0;
	e->eval2D[k].uorder = 1;
	e->eval2D[k].vorder = 1;
	e->eval2D[k].coeff = (GLfloat *) crAlloc(n * sizeof(GLfloat));
	for (i = 0; i < n; i++)
		e->eval2D[k].coeff[i] = initial[i];
	RESET(eb->eval2D[i], ctx->bitid);
}

void
crStateEvaluatorDestroy(CRContext *ctx)
{
	free_1d_map(ctx, GL_MAP1_VERTEX_3);
	free_1d_map(ctx, GL_MAP1_VERTEX_4);
	free_1d_map(ctx, GL_MAP1_INDEX);
	free_1d_map(ctx, GL_MAP1_COLOR_4);
	free_1d_map(ctx, GL_MAP1_NORMAL);
	free_1d_map(ctx, GL_MAP1_TEXTURE_COORD_1);
	free_1d_map(ctx, GL_MAP1_TEXTURE_COORD_2);
	free_1d_map(ctx, GL_MAP1_TEXTURE_COORD_3);
	free_1d_map(ctx, GL_MAP1_TEXTURE_COORD_4);

	free_2d_map(ctx, GL_MAP2_VERTEX_3);
	free_2d_map(ctx, GL_MAP2_VERTEX_4);
	free_2d_map(ctx, GL_MAP2_INDEX);
	free_2d_map(ctx, GL_MAP2_COLOR_4);
	free_2d_map(ctx, GL_MAP2_NORMAL);
	free_2d_map(ctx, GL_MAP2_TEXTURE_COORD_1);
	free_2d_map(ctx, GL_MAP2_TEXTURE_COORD_2);
	free_2d_map(ctx, GL_MAP2_TEXTURE_COORD_3);
	free_2d_map(ctx, GL_MAP2_TEXTURE_COORD_4);
}

void
crStateEvaluatorInit(CRContext *ctx)
{
	CREvaluatorState *e = &ctx->eval;
	CRStateBits *sb = GetCurrentBits();
	CREvaluatorBits *eb = &(sb->eval);
	static GLfloat vertex[4] = { 0.0, 0.0, 0.0, 1.0 };
	static GLfloat normal[3] = { 0.0, 0.0, 1.0 };
	static GLfloat index[1] = { 1.0 };
	static GLfloat color[4] = { 1.0, 1.0, 1.0, 1.0 };
	static GLfloat texcoord[4] = { 0.0, 0.0, 0.0, 1.0 };

	e->autoNormal = GL_FALSE;
	RESET(eb->enable, ctx->bitid);

	init_1d_map(ctx, GL_MAP1_VERTEX_3, 3, vertex);
	init_1d_map(ctx, GL_MAP1_VERTEX_4, 4, vertex);
	init_1d_map(ctx, GL_MAP1_INDEX, 1, index);
	init_1d_map(ctx, GL_MAP1_COLOR_4, 4, color);
	init_1d_map(ctx, GL_MAP1_NORMAL, 3, normal);
	init_1d_map(ctx, GL_MAP1_TEXTURE_COORD_1, 1, texcoord);
	init_1d_map(ctx, GL_MAP1_TEXTURE_COORD_2, 2, texcoord);
	init_1d_map(ctx, GL_MAP1_TEXTURE_COORD_3, 3, texcoord);
	init_1d_map(ctx, GL_MAP1_TEXTURE_COORD_4, 4, texcoord);

	init_2d_map(ctx, GL_MAP2_VERTEX_3, 3, vertex);
	init_2d_map(ctx, GL_MAP2_VERTEX_4, 4, vertex);
	init_2d_map(ctx, GL_MAP2_INDEX, 1, index);
	init_2d_map(ctx, GL_MAP2_COLOR_4, 4, color);
	init_2d_map(ctx, GL_MAP2_NORMAL, 3, normal);
	init_2d_map(ctx, GL_MAP2_TEXTURE_COORD_1, 1, texcoord);
	init_2d_map(ctx, GL_MAP2_TEXTURE_COORD_2, 2, texcoord);
	init_2d_map(ctx, GL_MAP2_TEXTURE_COORD_3, 3, texcoord);
	init_2d_map(ctx, GL_MAP2_TEXTURE_COORD_4, 4, texcoord);

	e->un1D = 1;
	e->u11D = 0.0;
	e->u21D = 1.0;
	RESET(eb->grid1D, ctx->bitid);

	e->un2D = 1;
	e->vn2D = 1;
	e->u12D = 0.0;
	e->u22D = 1.0;
	e->v12D = 0.0;
	e->v22D = 1.0;
	RESET(eb->grid1D, ctx->bitid);

	RESET(eb->dirty, ctx->bitid);
}

const int gleval_sizes[] = { 4, 1, 3, 1, 2, 3, 4, 3, 4 };

/**********************************************************************/
/***            Copy and deallocate control points                  ***/
/**********************************************************************/


/*
 * Copy 1-parametric evaluator control points from user-specified
 * memory space to a buffer of contiguous control points.
 * Input:  see glMap1f for details
 * Return:  pointer to buffer of contiguous control points or NULL if out
 *          of memory.
 */
static GLfloat *
_copy_map_points1f(GLint size, GLint ustride, GLint uorder,
									 const GLfloat * points)
{
	GLfloat *buffer, *p;
	GLint i, k;

	if (!points || size == 0) {
		return NULL;
	}

	buffer = (GLfloat *) crAlloc(uorder * size * sizeof(GLfloat));

	if (buffer)
		for (i = 0, p = buffer; i < uorder; i++, points += ustride)
			for (k = 0; k < size; k++)
				*p++ = points[k];

	return buffer;
}



/*
 * Same as above but convert doubles to floats.
 */
static GLfloat *
_copy_map_points1d(GLint size, GLint ustride, GLint uorder,
									 const GLdouble * points)
{
	GLfloat *buffer, *p;
	GLint i, k;

	if (!points || size == 0) {
		return NULL;
	}

	buffer = (GLfloat *) crAlloc(uorder * size * sizeof(GLfloat));

	if (buffer)
		for (i = 0, p = buffer; i < uorder; i++, points += ustride)
			for (k = 0; k < size; k++)
				*p++ = (GLfloat) points[k];

	return buffer;
}



/*
 * Copy 2-parametric evaluator control points from user-specified
 * memory space to a buffer of contiguous control points.
 * Additional memory is allocated to be used by the Horner and
 * de Casteljau evaluation schemes.
 *
 * Input:  see glMap2f for details
 * Return:  pointer to buffer of contiguous control points or NULL if out
 *          of memory.
 */
static GLfloat *
_copy_map_points2f(GLint size,
									 GLint ustride, GLint uorder,
									 GLint vstride, GLint vorder, const GLfloat * points)
{
	GLfloat *buffer, *p;
	GLint i, j, k, dsize, hsize;
	GLint uinc;

	if (!points || size == 0) {
		return NULL;
	}

	/* max(uorder, vorder) additional points are used in      */
	/* Horner evaluation and uorder*vorder additional */
	/* values are needed for de Casteljau                     */
	dsize = (uorder == 2 && vorder == 2) ? 0 : uorder * vorder;
	hsize = (uorder > vorder ? uorder : vorder) * size;

	if (hsize > dsize)
		buffer =
			(GLfloat *) crAlloc((uorder * vorder * size + hsize) * sizeof(GLfloat));
	else
		buffer =
			(GLfloat *) crAlloc((uorder * vorder * size + dsize) * sizeof(GLfloat));

	/* compute the increment value for the u-loop */
	uinc = ustride - vorder * vstride;

	if (buffer)
		for (i = 0, p = buffer; i < uorder; i++, points += uinc)
			for (j = 0; j < vorder; j++, points += vstride)
				for (k = 0; k < size; k++)
					*p++ = points[k];

	return buffer;
}



/*
 * Same as above but convert doubles to floats.
 */
static GLfloat *
_copy_map_points2d(GLint size,
									 GLint ustride, GLint uorder,
									 GLint vstride, GLint vorder, const GLdouble * points)
{
	GLfloat *buffer, *p;
	GLint i, j, k, hsize, dsize;
	GLint uinc;

	if (!points || size == 0) {
		return NULL;
	}

	/* max(uorder, vorder) additional points are used in      */
	/* Horner evaluation and uorder*vorder additional */
	/* values are needed for de Casteljau                     */
	dsize = (uorder == 2 && vorder == 2) ? 0 : uorder * vorder;
	hsize = (uorder > vorder ? uorder : vorder) * size;

	if (hsize > dsize)
		buffer =
			(GLfloat *) crAlloc((uorder * vorder * size + hsize) * sizeof(GLfloat));
	else
		buffer =
			(GLfloat *) crAlloc((uorder * vorder * size + dsize) * sizeof(GLfloat));

	/* compute the increment value for the u-loop */
	uinc = ustride - vorder * vstride;

	if (buffer)
		for (i = 0, p = buffer; i < uorder; i++, points += uinc)
			for (j = 0; j < vorder; j++, points += vstride)
				for (k = 0; k < size; k++)
					*p++ = (GLfloat) points[k];

	return buffer;
}




/**********************************************************************/
/***                      API entry points                          ***/
/**********************************************************************/


/*
 * This does the work of glMap1[fd].
 */
static void
map1(GLenum target, GLfloat u1, GLfloat u2, GLint ustride,
		 GLint uorder, const GLvoid * points, GLenum type)
{
	CRContext *g = GetCurrentContext();
	CREvaluatorState *e = &(g->eval);
	CRStateBits *sb = GetCurrentBits();
	CREvaluatorBits *eb = &(sb->eval);
	CRTextureState *t = &(g->texture);
	GLint i;
	GLint k;
	GLfloat *pnts;

	if (g->current.inBeginEnd) {
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "Map1d called in begin/end");
		return;
	}

	FLUSH();

	CRASSERT(type == GL_FLOAT || type == GL_DOUBLE);

	if (u1 == u2) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glMap1d(u1==u2)");
		return;
	}
	if (uorder < 1 || uorder > MAX_EVAL_ORDER) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glMap1d(bad uorder)");
		return;
	}
	if (!points) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
								 "glMap1d(null points)");
		return;
	}

	i = target - GL_MAP1_COLOR_4;

	k = gleval_sizes[i];

	if (k == 0) {
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glMap1d(k=0)");
		return;
	}

	if (ustride < k) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glMap1d(bad ustride");
		return;
	}

	if (t->curTextureUnit != 0) {
		/* See OpenGL 1.2.1 spec, section F.2.13 */
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "glMap1d(current texture unit must be zero)");
		return;
	}

	switch (target) {
	case GL_MAP1_VERTEX_3:
	case GL_MAP1_VERTEX_4:
	case GL_MAP1_INDEX:
	case GL_MAP1_COLOR_4:
	case GL_MAP1_NORMAL:
	case GL_MAP1_TEXTURE_COORD_1:
	case GL_MAP1_TEXTURE_COORD_2:
	case GL_MAP1_TEXTURE_COORD_3:
	case GL_MAP1_TEXTURE_COORD_4:
		break;
	default:
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glMap1d(bad target)");
		return;
	}

	/* make copy of the control points */
	if (type == GL_FLOAT)
		pnts = _copy_map_points1f(k, ustride, uorder, (GLfloat *) points);
	else
		pnts = _copy_map_points1d(k, ustride, uorder, (GLdouble *) points);

	e->eval1D[i].order = uorder;
	e->eval1D[i].u1 = u1;
	e->eval1D[i].u2 = u2;
	e->eval1D[i].du = 1.0f / (u2 - u1);
	if (e->eval1D[i].coeff)
		crFree(e->eval1D[i].coeff);
	e->eval1D[i].coeff = pnts;

	DIRTY(eb->dirty, g->neg_bitid);
	DIRTY(eb->eval1D[i], g->neg_bitid);
}



void STATE_APIENTRY
crStateMap1f(GLenum target, GLfloat u1, GLfloat u2,
						 GLint stride, GLint order, const GLfloat * points)
{
	map1(target, u1, u2, stride, order, points, GL_FLOAT);
}

void STATE_APIENTRY
crStateMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride,
						 GLint order, const GLdouble * points)
{
	map1(target, (GLfloat) u1, (GLfloat) u2, stride, order, points, GL_DOUBLE);
}

static void
map2(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
		 GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
		 const GLvoid * points, GLenum type)
{
	CRContext *g = GetCurrentContext();
	CRStateBits *sb = GetCurrentBits();
	CREvaluatorState *e = &(g->eval);
	CREvaluatorBits *eb = &(sb->eval);
#if 0
	CRTextureState *t = &(g->texture);
#endif
	GLint i;
	GLint k;
	GLfloat *pnts;

	if (g->current.inBeginEnd) {
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glMap2d()");
		return;
	}

	FLUSH();

	if (u1 == u2) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glMap2d()");
		return;
	}

	if (v1 == v2) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glMap2d()");
		return;
	}

	if (uorder < 1 || uorder > MAX_EVAL_ORDER) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glMap2d()");
		return;
	}

	if (vorder < 1 || vorder > MAX_EVAL_ORDER) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glMap2d()");
		return;
	}

	if (g->extensions.NV_vertex_program) {
/* XXX FIXME */
		i = target - GL_MAP2_COLOR_4;
	} else {
		i = target - GL_MAP2_COLOR_4;
	}

	k = gleval_sizes[i];

	if (k == 0) {
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glMap2d()");
		return;
	}

	if (ustride < k) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glMap2d()");
		return;
	}
	if (vstride < k) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glMap2d()");
		return;
	}

#if 00
	/* Disable this check for now - it looks like various OpenGL drivers
	 * don't do this error check.  So, a bunch of the NVIDIA demos
	 * generate errors/warnings.
	 */
	if (t->curTextureUnit != 0) {
		/* See OpenGL 1.2.1 spec, section F.2.13 */
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glMap2d()");
		return;
	}
#endif

	switch (target) {
	case GL_MAP2_VERTEX_3:
	case GL_MAP2_VERTEX_4:
	case GL_MAP2_INDEX:
	case GL_MAP2_COLOR_4:
	case GL_MAP2_NORMAL:
	case GL_MAP2_TEXTURE_COORD_1:
	case GL_MAP2_TEXTURE_COORD_2:
	case GL_MAP2_TEXTURE_COORD_3:
	case GL_MAP2_TEXTURE_COORD_4:
		break;
	default:
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glMap2d()");
		return;
	}

	/* make copy of the control points */
	if (type == GL_FLOAT)
		pnts = _copy_map_points2f(k, ustride, uorder,
															vstride, vorder, (GLfloat *) points);
	else
		pnts = _copy_map_points2d(k, ustride, uorder,
															vstride, vorder, (GLdouble *) points);

	e->eval2D[i].uorder = uorder;
	e->eval2D[i].u1 = u1;
	e->eval2D[i].u2 = u2;
	e->eval2D[i].du = 1.0f / (u2 - u1);
	e->eval2D[i].vorder = vorder;
	e->eval2D[i].v1 = v1;
	e->eval2D[i].v2 = v2;
	e->eval2D[i].dv = 1.0f / (v2 - v1);
	if (e->eval2D[i].coeff)
		crFree(e->eval2D[i].coeff);
	e->eval2D[i].coeff = pnts;

	DIRTY(eb->dirty, g->neg_bitid);
	DIRTY(eb->eval2D[i], g->neg_bitid);
}

void STATE_APIENTRY
crStateMap2f(GLenum target, GLfloat u1, GLfloat u2,
						 GLint ustride, GLint uorder,
						 GLfloat v1, GLfloat v2,
						 GLint vstride, GLint vorder, const GLfloat * points)
{
	map2(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder,
			 points, GL_FLOAT);
}


void STATE_APIENTRY
crStateMap2d(GLenum target, GLdouble u1, GLdouble u2,
						 GLint ustride, GLint uorder,
						 GLdouble v1, GLdouble v2,
						 GLint vstride, GLint vorder, const GLdouble * points)
{
	map2(target, (GLfloat) u1, (GLfloat) u2, ustride, uorder,
			 (GLfloat) v1, (GLfloat) v2, vstride, vorder, points, GL_DOUBLE);
}

void STATE_APIENTRY
crStateGetMapdv(GLenum target, GLenum query, GLdouble * v)
{
	CRContext *g = GetCurrentContext();
	CRStateBits *sb = GetCurrentBits();
	CREvaluatorState *e = &(g->eval);
	GLint size;
	GLint i, j;
	(void) sb;

	if (g->current.inBeginEnd) {
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "Map1d called in begin/end");
		return;
	}

	FLUSH();

	i = target - GL_MAP1_COLOR_4;

	if (i < 0 || i >= GLEVAL_TOT) {
		i = target - GL_MAP2_COLOR_4;

		if (i < 0 || i >= GLEVAL_TOT) {
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
									 "GetMapdv: invalid target: %d", target);
			return;
		}

		switch (query) {
		case GL_COEFF:
			size = gleval_sizes[i] * e->eval2D[i].uorder * e->eval2D[i].vorder;
			for (j = 0; j < size; j++) {
				v[j] = e->eval2D[i].coeff[j];
			}
			break;
		case GL_ORDER:
			v[0] = (GLdouble) e->eval2D[i].uorder;
			v[1] = (GLdouble) e->eval2D[i].vorder;
			break;
		case GL_DOMAIN:
			v[0] = e->eval2D[i].u1;
			v[1] = e->eval2D[i].u2;
			v[2] = e->eval2D[i].v1;
			v[3] = e->eval2D[i].v2;
			break;
		default:
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
									 "GetMapdv: invalid target: %d", target);
			return;
		}
	}
	else {
		switch (query) {
		case GL_COEFF:
			size = gleval_sizes[i] * e->eval1D[i].order;
			for (j = 0; j < size; j++) {
				v[j] = e->eval1D[i].coeff[j];
			}
			break;
		case GL_ORDER:
			*v = (GLdouble) e->eval1D[i].order;
			break;
		case GL_DOMAIN:
			v[0] = e->eval1D[i].u1;
			v[1] = e->eval1D[i].u2;
			break;
		default:
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
									 "GetMapdv: invalid target: %d", target);
			return;
		}
	}
}

void STATE_APIENTRY
crStateGetMapfv(GLenum target, GLenum query, GLfloat * v)
{
	CRContext *g = GetCurrentContext();
	CRStateBits *sb = GetCurrentBits();
	CREvaluatorState *e = &(g->eval);
	GLint size;
	GLint i, j;
	(void) sb;

	if (g->current.inBeginEnd) {
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "Map1d called in begin/end");
		return;
	}

	FLUSH();

	i = target - GL_MAP1_COLOR_4;
	if (i < 0 || i >= GLEVAL_TOT) {
		i = target - GL_MAP2_COLOR_4;
		if (i < 0 || i >= GLEVAL_TOT) {
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
									 "GetMapfv: invalid target: %d", target);
			return;
		}
		switch (query) {
		case GL_COEFF:
			size = gleval_sizes[i] * e->eval2D[i].uorder * e->eval2D[i].vorder;
			for (j = 0; j < size; j++) {
				v[j] = (GLfloat) e->eval2D[i].coeff[j];
			}
			break;
		case GL_ORDER:
			v[0] = (GLfloat) e->eval2D[i].uorder;
			v[1] = (GLfloat) e->eval2D[i].vorder;
			break;
		case GL_DOMAIN:
			v[0] = (GLfloat) e->eval2D[i].u1;
			v[1] = (GLfloat) e->eval2D[i].u2;
			v[2] = (GLfloat) e->eval2D[i].v1;
			v[3] = (GLfloat) e->eval2D[i].v2;
			break;
		default:
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
									 "GetMapfv: invalid target: %d", target);
			return;
		}
	}
	else {
		switch (query) {
		case GL_COEFF:
			size = gleval_sizes[i] * e->eval1D[i].order;
			for (j = 0; j < size; j++) {
				v[j] = (GLfloat) e->eval1D[i].coeff[j];
			}
			break;
		case GL_ORDER:
			*v = (GLfloat) e->eval1D[i].order;
			break;
		case GL_DOMAIN:
			v[0] = (GLfloat) e->eval1D[i].u1;
			v[1] = (GLfloat) e->eval1D[i].u2;
			break;
		default:
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
									 "GetMapfv: invalid target: %d", target);
			return;
		}
	}
}

void STATE_APIENTRY
crStateGetMapiv(GLenum target, GLenum query, GLint * v)
{
	CRContext *g = GetCurrentContext();
	CRStateBits *sb = GetCurrentBits();
	CREvaluatorState *e = &(g->eval);
	GLint size;
	GLint i, j;
	(void) sb;

	if (g->current.inBeginEnd) {
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "Map1d called in begin/end");
		return;
	}

	FLUSH();

	i = target - GL_MAP1_COLOR_4;
	if (i < 0 || i >= GLEVAL_TOT) {
		i = target - GL_MAP2_COLOR_4;
		if (i < 0 || i >= GLEVAL_TOT) {
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
									 "GetMapiv: invalid target: %d", target);
			return;
		}
		switch (query) {
		case GL_COEFF:
			size = gleval_sizes[i] * e->eval2D[i].uorder * e->eval2D[i].vorder;
			for (j = 0; j < size; j++) {
				v[j] = (GLint) e->eval2D[i].coeff[j];
			}
			break;
		case GL_ORDER:
			v[0] = e->eval2D[i].uorder;
			v[1] = e->eval2D[i].vorder;
			break;
		case GL_DOMAIN:
			v[0] = (GLint) e->eval2D[i].u1;
			v[1] = (GLint) e->eval2D[i].u2;
			v[2] = (GLint) e->eval2D[i].v1;
			v[3] = (GLint) e->eval2D[i].v2;
			break;
		default:
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
									 "GetMapiv: invalid target: %d", target);
			return;
		}
	}
	else {
		switch (query) {
		case GL_COEFF:
			size = gleval_sizes[i] * e->eval1D[i].order;
			for (j = 0; j < size; j++) {
				v[j] = (GLint) e->eval1D[i].coeff[j];
			}
			break;
		case GL_ORDER:
			*v = e->eval1D[i].order;
			break;
		case GL_DOMAIN:
			v[0] = (GLint) e->eval1D[i].u1;
			v[1] = (GLint) e->eval1D[i].u2;
			break;
		default:
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
									 "GetMapiv: invalid target: %d", target);
			return;
		}
	}
}

void STATE_APIENTRY
crStateMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
	CRContext *g = GetCurrentContext();
	CRStateBits *sb = GetCurrentBits();
	CREvaluatorState *e = &(g->eval);
	CREvaluatorBits *eb = &(sb->eval);

	if (g->current.inBeginEnd) {
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "Map1d called in begin/end");
		return;
	}

	FLUSH();

	if (un < 1) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glMapGrid1f(bad un)");
		return;
	}

	e->un1D = un;
	e->u11D = u1;
	e->u21D = u2;

	DIRTY(eb->dirty, g->neg_bitid);
	DIRTY(eb->grid1D, g->neg_bitid);
}

void STATE_APIENTRY
crStateMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
	crStateMapGrid1f(un, (GLfloat) u1, (GLfloat) u2);
}


void STATE_APIENTRY
crStateMapGrid2f(GLint un, GLfloat u1, GLfloat u2,
								 GLint vn, GLfloat v1, GLfloat v2)
{
	CRContext *g = GetCurrentContext();
	CRStateBits *sb = GetCurrentBits();
	CREvaluatorState *e = &(g->eval);
	CREvaluatorBits *eb = &(sb->eval);

	if (g->current.inBeginEnd) {
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "Map1d called in begin/end");
		return;
	}

	FLUSH();

	if (un < 1) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glMapGrid2f(bad un)");
		return;
	}
	if (vn < 1) {
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glMapGrid2f(bad vn)");
		return;
	}

	e->un2D = un;
	e->vn2D = vn;
	e->u12D = u1;
	e->u22D = u2;
	e->v12D = v1;
	e->v22D = v2;

	DIRTY(eb->dirty, g->neg_bitid);
	DIRTY(eb->grid2D, g->neg_bitid);
}

void STATE_APIENTRY
crStateMapGrid2d(GLint un, GLdouble u1, GLdouble u2,
								 GLint vn, GLdouble v1, GLdouble v2)
{
	crStateMapGrid2f(un, (GLfloat) u1, (GLfloat) u2,
									 vn, (GLfloat) v1, (GLfloat) v2);
}

void
crStateEvaluatorSwitch(CREvaluatorBits *e, CRbitvalue * bitID,
											 CRContext *fromCtx, CRContext *toCtx)
{
	CREvaluatorState *from = &(fromCtx->eval);
	CREvaluatorState *to = &(toCtx->eval);
	int i, j;
	CRbitvalue nbitID[CR_MAX_BITARRAY];

	for (j = 0; j < CR_MAX_BITARRAY; j++)
		nbitID[j] = ~bitID[j];

	if (CHECKDIRTY(e->enable, bitID)) {
		if (from->autoNormal != to->autoNormal) {
			glAble able[2];
			able[0] = diff_api.Disable;
			able[1] = diff_api.Enable;
			able[to->autoNormal] (GL_AUTO_NORMAL);
			FILLDIRTY(e->enable);
			FILLDIRTY(e->dirty);
		}
		CLEARDIRTY(e->enable, nbitID);
	}
	for (i = 0; i < GLEVAL_TOT; i++) {
		if (CHECKDIRTY(e->eval1D[i], bitID)) {
			int size = from->eval1D[i].order * gleval_sizes[i] *
				sizeof(*from->eval1D[i].coeff);
			if (from->eval1D[i].order != to->eval1D[i].order ||
				from->eval1D[i].u1    != to->eval1D[i].u1 ||
				from->eval1D[i].u2    != to->eval1D[i].u2 ||
				crMemcmp((const void *) from->eval1D[i].coeff,
						 (const void *) to->eval1D[i].coeff, size)) {
				diff_api.Map1f(i + GL_MAP1_COLOR_4, to->eval1D[i].u1,
							   to->eval1D[i].u2, gleval_sizes[i], to->eval1D[i].order,
							   to->eval1D[i].coeff);
				FILLDIRTY(e->dirty);
				FILLDIRTY(e->eval1D[i]);
			}
			CLEARDIRTY(e->eval1D[i], nbitID);
		}
	}

	for (i = 0; i < GLEVAL_TOT; i++) {
		if (CHECKDIRTY(e->eval2D[i], bitID)) {
			int size = from->eval2D[i].uorder * from->eval2D[i].vorder
                     * gleval_sizes[i] * sizeof(*from->eval2D[i].coeff);
			if (from->eval2D[i].uorder != to->eval2D[i].uorder ||
				from->eval2D[i].vorder != to->eval2D[i].vorder ||
				from->eval2D[i].u1     != to->eval2D[i].u1 ||
				from->eval2D[i].u2     != to->eval2D[i].u2 ||
				from->eval2D[i].v1     != to->eval2D[i].v1 ||
				from->eval2D[i].v2     != to->eval2D[i].v2 ||
				crMemcmp((const void *) from->eval2D[i].coeff,
						 (const void *) to->eval2D[i].coeff, size)) {
				diff_api.Map2f(i + GL_MAP2_COLOR_4,
							   to->eval2D[i].u1, to->eval2D[i].u2,
							   gleval_sizes[i], to->eval2D[i].uorder,
                               to->eval2D[i].v1, to->eval2D[i].v2,
                               gleval_sizes[i], to->eval2D[i].vorder,
                               to->eval2D[i].coeff);
				FILLDIRTY(e->dirty);
				FILLDIRTY(e->eval2D[i]);
			}
			CLEARDIRTY(e->eval2D[i], nbitID);
		}
	}
	if (CHECKDIRTY(e->grid1D, bitID)) {
		if (from->u11D != to->u11D ||
				from->u21D != to->u21D ||
                from->un1D != to->un1D) {
			diff_api.MapGrid1d(to->un1D, to->u11D, to->u21D);
			FILLDIRTY(e->dirty);
			FILLDIRTY(e->grid1D);
		}
		CLEARDIRTY(e->grid1D, nbitID);
	}
	if (CHECKDIRTY(e->grid2D, bitID)) {
		if (from->u12D != to->u12D ||
			from->u22D != to->u22D ||
			from->un2D != to->un2D ||
			from->v12D != to->v12D ||
			from->v22D != to->v22D ||
            from->vn2D != to->vn2D) {
			diff_api.MapGrid2d(to->un2D, to->u12D, to->u22D,
							   to->vn2D, to->v12D, to->v22D);
			FILLDIRTY(e->dirty);
			FILLDIRTY(e->grid1D);
		}
		CLEARDIRTY(e->grid1D, nbitID);
	}
	CLEARDIRTY(e->dirty, nbitID);
}

void
crStateEvaluatorDiff(CREvaluatorBits *e, CRbitvalue *bitID,
                     CRContext *fromCtx, CRContext *toCtx)
{
	CREvaluatorState *from = &(fromCtx->eval);
	CREvaluatorState *to = &(toCtx->eval);
	glAble able[2];
	int i, j;
	CRbitvalue nbitID[CR_MAX_BITARRAY];

	for (j = 0; j < CR_MAX_BITARRAY; j++)
		nbitID[j] = ~bitID[j];

	able[0] = diff_api.Disable;
	able[1] = diff_api.Enable;

	if (CHECKDIRTY(e->enable, bitID)) {
		if (from->autoNormal != to->autoNormal) {
			able[to->autoNormal] (GL_AUTO_NORMAL);
			from->autoNormal = to->autoNormal;
		}
		CLEARDIRTY(e->enable, nbitID);
	}
	for (i = 0; i < GLEVAL_TOT; i++) {
		if (CHECKDIRTY(e->enable1D[i], bitID)) {
			if (from->enable1D[i] != to->enable1D[i]) {
				able[to->enable1D[i]] (i + GL_MAP1_COLOR_4);
				from->enable1D[i] = to->enable1D[i];
			}
			CLEARDIRTY(e->enable1D[i], nbitID);
		}
		if (to->enable1D[i] && CHECKDIRTY(e->eval1D[i], bitID)) {
			int size = from->eval1D[i].order * gleval_sizes[i] *
				sizeof(*from->eval1D[i].coeff);
			if (from->eval1D[i].order != to->eval1D[i].order ||
				from->eval1D[i].u1    != to->eval1D[i].u1 ||
				from->eval1D[i].u2    != to->eval1D[i].u2 ||
				crMemcmp((const void *) from->eval1D[i].coeff,
						 (const void *) to->eval1D[i].coeff, size)) {
				diff_api.Map1f(i + GL_MAP1_COLOR_4, to->eval1D[i].u1,
							   to->eval1D[i].u2, gleval_sizes[i], to->eval1D[i].order,
							   to->eval1D[i].coeff);
				from->eval1D[i].order = to->eval1D[i].order;
				from->eval1D[i].u1    = to->eval1D[i].u1;
				from->eval1D[i].u2    = to->eval1D[i].u2;
				crMemcpy((void *) from->eval1D[i].coeff,
						 (const void *) to->eval1D[i].coeff, size);
			}
			CLEARDIRTY(e->eval1D[i], nbitID);
		}
	}

	for (i = 0; i < GLEVAL_TOT; i++) {
		if (CHECKDIRTY(e->enable2D[i], bitID)) {
			if (from->enable2D[i] != to->enable2D[i]) {
				able[to->enable2D[i]] (i + GL_MAP2_COLOR_4);
				from->enable2D[i] = to->enable2D[i];
			}
			CLEARDIRTY(e->enable2D[i], nbitID);
		}
		if (to->enable2D[i] && CHECKDIRTY(e->eval2D[i], bitID)) {
			int size = from->eval2D[i].uorder * from->eval2D[i].vorder
                     * gleval_sizes[i] * sizeof(*from->eval2D[i].coeff);
			if (from->eval2D[i].uorder != to->eval2D[i].uorder ||
				from->eval2D[i].vorder != to->eval2D[i].vorder ||
				from->eval2D[i].u1     != to->eval2D[i].u1 ||
				from->eval2D[i].u2     != to->eval2D[i].u2 ||
				from->eval2D[i].v1     != to->eval2D[i].v1 ||
				from->eval2D[i].v2     != to->eval2D[i].v2 ||
				crMemcmp((const void *) from->eval2D[i].coeff,
						 (const void *) to->eval2D[i].coeff, size)) {
				diff_api.Map2f(i + GL_MAP2_COLOR_4,
							   to->eval2D[i].u1, to->eval2D[i].u2,
                               gleval_sizes[i], to->eval2D[i].uorder,
                               to->eval2D[i].v1, to->eval2D[i].v2,
                               gleval_sizes[i], to->eval2D[i].vorder,
                               to->eval2D[i].coeff);
				from->eval2D[i].uorder = to->eval2D[i].uorder;
				from->eval2D[i].vorder = to->eval2D[i].vorder;
				from->eval2D[i].u1     = to->eval2D[i].u1;
				from->eval2D[i].u2     = to->eval2D[i].u2;
				from->eval2D[i].v1     = to->eval2D[i].v1;
				from->eval2D[i].v2     = to->eval2D[i].v2;
				crMemcpy((void *) from->eval2D[i].coeff,
						 (const void *) to->eval2D[i].coeff, size);
			}
			CLEARDIRTY(e->eval2D[i], nbitID);
		}
	}
	if (CHECKDIRTY(e->grid1D, bitID)) {
		if (from->u11D != to->u11D ||
			from->u21D != to->u21D ||
            from->un1D != to->un1D) {
			diff_api.MapGrid1d(to->un1D, to->u11D, to->u21D);
			from->u11D = to->u11D;
			from->u21D = to->u21D;
			from->un1D = to->un1D;
		}
		CLEARDIRTY(e->grid1D, nbitID);
	}
	if (CHECKDIRTY(e->grid2D, bitID)) {
		if (from->u12D != to->u12D ||
				from->u22D != to->u22D ||
				from->un2D != to->un2D ||
				from->v12D != to->v12D ||
				from->v22D != to->v22D ||
                from->vn2D != to->vn2D) {
			diff_api.MapGrid2d(to->un2D, to->u12D, to->u22D,
							   to->vn2D, to->v12D, to->v22D);
			from->u12D = to->u12D;
			from->u22D = to->u22D;
			from->un2D = to->un2D;
			from->v12D = to->v12D;
			from->v22D = to->v22D;
			from->vn2D = to->vn2D;
		}
		CLEARDIRTY(e->grid1D, nbitID);
	}
	CLEARDIRTY(e->dirty, nbitID);
}
