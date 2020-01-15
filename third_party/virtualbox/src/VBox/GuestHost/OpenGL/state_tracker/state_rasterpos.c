/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"


/*
 * Apply modelview, projection, viewport transformations to (x,y,z,w)
 * and update the current raster position attributes.
 * Do NOT set dirty state.
 */
void
crStateRasterPosUpdate(PCRStateTracker pState, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	CRContext *g = GetCurrentContext(pState);
	CRCurrentState *c = &(g->current);
	CRTransformState *t = &(g->transform);
	CRViewportState *v = &(g->viewport);
	GLvectorf p;
	int i;

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "RasterPos called in Begin/End");
		return;
	}

	FLUSH();

	/* update current color, texcoord, etc from the CurrentStatePointers */
	crStateCurrentRecover(pState);

	p.x = x;
	p.y = y;
	p.z = z;
	p.w = w;

	/* Apply modelview and projection matrix */
	crStateTransformXformPoint(t, &p);	

	/* clip test */
	if (p.x >  p.w || p.y >  p.w || p.z > p.w ||
		  p.x < -p.w || p.y < -p.w || p.z < -p.w) 
	{
		c->rasterValid = GL_FALSE;
		return;
	} 

	/* divide by W (perspective projection) */
	p.x /= p.w;
	p.y /= p.w;
	p.z /= p.w;
	p.w = 1.0f;

	/* map from NDC to window coords */
	crStateViewportApply(v, &p);

	c->rasterValid = GL_TRUE;
	ASSIGN_4V(c->rasterAttrib[VERT_ATTRIB_POS],    p.x, p.y, p.z, p.w);
	ASSIGN_4V(c->rasterAttribPre[VERT_ATTRIB_POS], p.x, p.y, p.z, p.w);
	for (i = 1; i < CR_MAX_VERTEX_ATTRIBS; i++) {
		COPY_4V(c->rasterAttrib[i] , c->vertexAttrib[i]);
	}

	/* XXX need to update raster distance... */
	/* from Mesa... */
#ifdef CR_EXT_fog_coord
	if (g->fog.fogCoordinateSource == GL_FOG_COORDINATE_EXT)
		 c->rasterAttrib[VERT_ATTRIB_FOG][0] = c->vertexAttrib[VERT_ATTRIB_FOG][0];
	else
#endif
		 c->rasterAttrib[VERT_ATTRIB_FOG][0] = 0.0; /*(GLfloat)
						sqrt( eye[0]*eye[0] + eye[1]*eye[1] + eye[2]*eye[2] );*/
}


/* As above, but set dirty flags */
static void RasterPos4f(PCRStateTracker pState, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	CRContext *g = GetCurrentContext(pState);
	CRStateBits *sb = GetCurrentBits(pState);
	CRCurrentBits *cb = &(sb->current);

	crStateRasterPosUpdate(pState, x, y, z, w);

	DIRTY(cb->dirty, g->neg_bitid);
	DIRTY(cb->rasterPos, g->neg_bitid);
}

void STATE_APIENTRY crStateRasterPos2d(PCRStateTracker pState, GLdouble x, GLdouble y)
{
	RasterPos4f(pState, (GLfloat) x, (GLfloat) y, 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos2f(PCRStateTracker pState, GLfloat x, GLfloat y)
{
	RasterPos4f(pState, x, y, 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos2i(PCRStateTracker pState, GLint x, GLint y)
{
	RasterPos4f(pState, (GLfloat) x, (GLfloat) y, 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos2s(PCRStateTracker pState, GLshort x, GLshort y)
{
	RasterPos4f(pState, (GLfloat) x, (GLfloat) y, 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos3d(PCRStateTracker pState, GLdouble x, GLdouble y, GLdouble z)
{
	RasterPos4f(pState, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0f);
}

void STATE_APIENTRY crStateRasterPos3f(PCRStateTracker pState, GLfloat x, GLfloat y, GLfloat z)
{
	RasterPos4f(pState, x, y, z, 1.0f);
}

void STATE_APIENTRY crStateRasterPos3i(PCRStateTracker pState, GLint x, GLint y, GLint z)
{
	RasterPos4f(pState, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0f);
}

void STATE_APIENTRY crStateRasterPos3s(PCRStateTracker pState, GLshort x, GLshort y, GLshort z)
{
	RasterPos4f(pState, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0f);
}

void STATE_APIENTRY crStateRasterPos4d(PCRStateTracker pState, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	RasterPos4f(pState, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void STATE_APIENTRY crStateRasterPos4f(PCRStateTracker pState, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	RasterPos4f(pState, x, y, z, w);
}

void STATE_APIENTRY crStateRasterPos4i(PCRStateTracker pState, GLint x, GLint y, GLint z, GLint w)
{
	RasterPos4f(pState, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void STATE_APIENTRY crStateRasterPos4s(PCRStateTracker pState, GLshort x, GLshort y, GLshort z, GLshort w)
{
	RasterPos4f(pState, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void STATE_APIENTRY crStateRasterPos2dv(PCRStateTracker pState, const GLdouble *v)
{
	RasterPos4f(pState, (GLfloat) v[0], (GLfloat) v[1], 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos2fv(PCRStateTracker pState, const GLfloat *v)
{
	RasterPos4f(pState, v[0], v[1], 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos2iv(PCRStateTracker pState, const GLint *v)
{
	RasterPos4f(pState, (GLfloat) v[0], (GLfloat) v[1], 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos2sv(PCRStateTracker pState, const GLshort *v)
{
	RasterPos4f(pState, (GLfloat) v[0], (GLfloat) v[1], 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos3dv(PCRStateTracker pState, const GLdouble *v)
{
	RasterPos4f(pState, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0f);
}

void STATE_APIENTRY crStateRasterPos3fv(PCRStateTracker pState, const GLfloat *v)
{
	RasterPos4f(pState, v[0], v[1], v[2], 1.0f);
}

void STATE_APIENTRY crStateRasterPos3iv(PCRStateTracker pState, const GLint *v)
{
	RasterPos4f(pState, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0f);
}

void STATE_APIENTRY crStateRasterPos3sv(PCRStateTracker pState, const GLshort *v)
{
	RasterPos4f(pState, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0f);
}

void STATE_APIENTRY crStateRasterPos4dv(PCRStateTracker pState, const GLdouble *v)
{
	RasterPos4f(pState, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void STATE_APIENTRY crStateRasterPos4fv(PCRStateTracker pState, const GLfloat *v)
{
	RasterPos4f(pState, v[0], v[1], v[2], v[3]);
}

void STATE_APIENTRY crStateRasterPos4iv(PCRStateTracker pState, const GLint *v)
{
	RasterPos4f(pState, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void STATE_APIENTRY crStateRasterPos4sv(PCRStateTracker pState, const GLshort *v)
{
	RasterPos4f(pState, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}


/**********************************************************************/


static void
crStateWindowPosUpdate(PCRStateTracker pState, GLfloat x, GLfloat y, GLfloat z)
{
	CRContext *g = GetCurrentContext(pState);
	CRCurrentState *c = &(g->current);
	CRStateBits *sb = GetCurrentBits(pState);
	CRCurrentBits *cb = &(sb->current);
	int i;

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "WindowPos called in Begin/End");
		return;
	}

	FLUSH();
	DIRTY(cb->dirty, g->neg_bitid);
	DIRTY(cb->rasterPos, g->neg_bitid);

	c->rasterValid = GL_TRUE;
	ASSIGN_4V(c->rasterAttrib[VERT_ATTRIB_POS],    x, y , z, 1.0);
	ASSIGN_4V(c->rasterAttribPre[VERT_ATTRIB_POS], x, y, z, 1.0);
	for (i = 1; i < CR_MAX_VERTEX_ATTRIBS; i++) {
		COPY_4V(c->rasterAttrib[i] , c->vertexAttrib[i]);
	}
}


void STATE_APIENTRY crStateWindowPos2dARB (PCRStateTracker pState, GLdouble x, GLdouble y)
{
	crStateWindowPosUpdate(pState, (GLfloat) x, (GLfloat) y, 0.0);
}

void STATE_APIENTRY crStateWindowPos2dvARB (PCRStateTracker pState, const GLdouble *v)
{
	crStateWindowPosUpdate(pState, (GLfloat) v[0], (GLfloat) v[1], 0.0);
}

void STATE_APIENTRY crStateWindowPos2fARB (PCRStateTracker pState, GLfloat x, GLfloat y)
{
	crStateWindowPosUpdate(pState, x, y, 0.0);
}

void STATE_APIENTRY crStateWindowPos2fvARB (PCRStateTracker pState, const GLfloat *v)
{
	crStateWindowPosUpdate(pState, v[0], v[1], 0.0);
}

void STATE_APIENTRY crStateWindowPos2iARB (PCRStateTracker pState, GLint x, GLint y)
{
	crStateWindowPosUpdate(pState, (GLfloat) x, (GLfloat) y, 0.0);
}

void STATE_APIENTRY crStateWindowPos2ivARB (PCRStateTracker pState, const GLint *v)
{
	crStateWindowPosUpdate(pState, (GLfloat) v[0], (GLfloat) v[1], 0.0);
}

void STATE_APIENTRY crStateWindowPos2sARB (PCRStateTracker pState, GLshort x, GLshort y)
{
	crStateWindowPosUpdate(pState, (GLfloat) x, (GLfloat) y, 0.0);
}

void STATE_APIENTRY crStateWindowPos2svARB (PCRStateTracker pState, const GLshort *v)
{
	crStateWindowPosUpdate(pState, (GLfloat) v[0], (GLfloat) v[1], 0.0);
}

void STATE_APIENTRY crStateWindowPos3dARB (PCRStateTracker pState, GLdouble x, GLdouble y, GLdouble z)
{
	crStateWindowPosUpdate(pState, (GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void STATE_APIENTRY crStateWindowPos3dvARB (PCRStateTracker pState, const GLdouble *v)
{
	crStateWindowPosUpdate(pState, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void STATE_APIENTRY crStateWindowPos3fARB (PCRStateTracker pState, GLfloat x, GLfloat y, GLfloat z)
{
	crStateWindowPosUpdate(pState, x, y, z);
}

void STATE_APIENTRY crStateWindowPos3fvARB (PCRStateTracker pState, const GLfloat *v)
{
	crStateWindowPosUpdate(pState, v[0], v[1], v[2]);
}

void STATE_APIENTRY crStateWindowPos3iARB (PCRStateTracker pState, GLint x, GLint y, GLint z)
{
	crStateWindowPosUpdate(pState, (GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void STATE_APIENTRY crStateWindowPos3ivARB (PCRStateTracker pState, const GLint *v)
{
	crStateWindowPosUpdate(pState, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void STATE_APIENTRY crStateWindowPos3sARB (PCRStateTracker pState, GLshort x, GLshort y, GLshort z)
{
	crStateWindowPosUpdate(pState, (GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void STATE_APIENTRY crStateWindowPos3svARB (PCRStateTracker pState, const GLshort *v)
{
	crStateWindowPosUpdate(pState, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

