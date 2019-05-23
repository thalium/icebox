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
crStateRasterPosUpdate(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	CRContext *g = GetCurrentContext();
	CRCurrentState *c = &(g->current);
	CRTransformState *t = &(g->transform);
	CRViewportState *v = &(g->viewport);
	GLvectorf p;
	int i;

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "RasterPos called in Begin/End");
		return;
	}

	FLUSH();

	/* update current color, texcoord, etc from the CurrentStatePointers */
	crStateCurrentRecover();

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
static void RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	CRContext *g = GetCurrentContext();
	CRStateBits *sb = GetCurrentBits();
	CRCurrentBits *cb = &(sb->current);

	crStateRasterPosUpdate(x, y, z, w);

	DIRTY(cb->dirty, g->neg_bitid);
	DIRTY(cb->rasterPos, g->neg_bitid);
}

void STATE_APIENTRY crStateRasterPos2d(GLdouble x, GLdouble y)
{
	RasterPos4f((GLfloat) x, (GLfloat) y, 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos2f(GLfloat x, GLfloat y)
{
	RasterPos4f(x, y, 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos2i(GLint x, GLint y)
{
	RasterPos4f((GLfloat) x, (GLfloat) y, 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos2s(GLshort x, GLshort y)
{
	RasterPos4f((GLfloat) x, (GLfloat) y, 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
	RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0f);
}

void STATE_APIENTRY crStateRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
	RasterPos4f(x, y, z, 1.0f);
}

void STATE_APIENTRY crStateRasterPos3i(GLint x, GLint y, GLint z)
{
	RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0f);
}

void STATE_APIENTRY crStateRasterPos3s(GLshort x, GLshort y, GLshort z)
{
	RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0f);
}

void STATE_APIENTRY crStateRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void STATE_APIENTRY crStateRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	RasterPos4f(x, y, z, w);
}

void STATE_APIENTRY crStateRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
	RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void STATE_APIENTRY crStateRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
	RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void STATE_APIENTRY crStateRasterPos2dv(const GLdouble *v)
{
	RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos2fv(const GLfloat *v)
{
	RasterPos4f(v[0], v[1], 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos2iv(const GLint *v)
{
	RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos2sv(const GLshort *v)
{
	RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0f, 1.0f);
}

void STATE_APIENTRY crStateRasterPos3dv(const GLdouble *v)
{
	RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0f);
}

void STATE_APIENTRY crStateRasterPos3fv(const GLfloat *v)
{
	RasterPos4f(v[0], v[1], v[2], 1.0f);
}

void STATE_APIENTRY crStateRasterPos3iv(const GLint *v)
{
	RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0f);
}

void STATE_APIENTRY crStateRasterPos3sv(const GLshort *v)
{
	RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0f);
}

void STATE_APIENTRY crStateRasterPos4dv(const GLdouble *v)
{
	RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void STATE_APIENTRY crStateRasterPos4fv(const GLfloat *v)
{
	RasterPos4f(v[0], v[1], v[2], v[3]);
}

void STATE_APIENTRY crStateRasterPos4iv(const GLint *v)
{
	RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void STATE_APIENTRY crStateRasterPos4sv(const GLshort *v)
{
	RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}


/**********************************************************************/


static void
crStateWindowPosUpdate(GLfloat x, GLfloat y, GLfloat z)
{
	CRContext *g = GetCurrentContext();
	CRCurrentState *c = &(g->current);
	CRStateBits *sb = GetCurrentBits();
	CRCurrentBits *cb = &(sb->current);
	int i;

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "WindowPos called in Begin/End");
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


void STATE_APIENTRY crStateWindowPos2dARB (GLdouble x, GLdouble y)
{
	crStateWindowPosUpdate((GLfloat) x, (GLfloat) y, 0.0);
}

void STATE_APIENTRY crStateWindowPos2dvARB (const GLdouble *v)
{
	crStateWindowPosUpdate((GLfloat) v[0], (GLfloat) v[1], 0.0);
}

void STATE_APIENTRY crStateWindowPos2fARB (GLfloat x, GLfloat y)
{
	crStateWindowPosUpdate(x, y, 0.0);
}

void STATE_APIENTRY crStateWindowPos2fvARB (const GLfloat *v)
{
	crStateWindowPosUpdate(v[0], v[1], 0.0);
}

void STATE_APIENTRY crStateWindowPos2iARB (GLint x, GLint y)
{
	crStateWindowPosUpdate((GLfloat) x, (GLfloat) y, 0.0);
}

void STATE_APIENTRY crStateWindowPos2ivARB (const GLint *v)
{
	crStateWindowPosUpdate((GLfloat) v[0], (GLfloat) v[1], 0.0);
}

void STATE_APIENTRY crStateWindowPos2sARB (GLshort x, GLshort y)
{
	crStateWindowPosUpdate((GLfloat) x, (GLfloat) y, 0.0);
}

void STATE_APIENTRY crStateWindowPos2svARB (const GLshort *v)
{
	crStateWindowPosUpdate((GLfloat) v[0], (GLfloat) v[1], 0.0);
}

void STATE_APIENTRY crStateWindowPos3dARB (GLdouble x, GLdouble y, GLdouble z)
{
	crStateWindowPosUpdate((GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void STATE_APIENTRY crStateWindowPos3dvARB (const GLdouble *v)
{
	crStateWindowPosUpdate((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void STATE_APIENTRY crStateWindowPos3fARB (GLfloat x, GLfloat y, GLfloat z)
{
	crStateWindowPosUpdate(x, y, z);
}

void STATE_APIENTRY crStateWindowPos3fvARB (const GLfloat *v)
{
	crStateWindowPosUpdate(v[0], v[1], v[2]);
}

void STATE_APIENTRY crStateWindowPos3iARB (GLint x, GLint y, GLint z)
{
	crStateWindowPosUpdate((GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void STATE_APIENTRY crStateWindowPos3ivARB (const GLint *v)
{
	crStateWindowPosUpdate((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

void STATE_APIENTRY crStateWindowPos3sARB (GLshort x, GLshort y, GLshort z)
{
	crStateWindowPosUpdate((GLfloat) x, (GLfloat) y, (GLfloat) z);
}

void STATE_APIENTRY crStateWindowPos3svARB (const GLshort *v)
{
	crStateWindowPosUpdate((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

