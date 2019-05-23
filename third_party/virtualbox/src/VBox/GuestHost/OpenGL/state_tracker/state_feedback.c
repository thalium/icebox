/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "state.h"
#include "state_internals.h"
#include "state/cr_statetypes.h"
#include "state/cr_feedback.h"

/* 
 * This file is really a complement to the feedbackSPU and as such
 * has big dependencies upon it. We have to monitor a whole bunch 
 * of state in the feedbackSPU to be able to properly implement
 * full functionality. 
 *
 * We have to intercept glColor3f(v)/4f(v) to get state updates on 
 * color properties and also glTexCoord* too, as unlike the tilesortSPU
 * we don't have a pincher that pulls these out as they're passing 
 * through.
 *
 * - Alan.
 */


/*
 * Selection and feedback
 *
 * TODO:
 *   1. Implement lighting for vertex colors for feedback
 *   2. Implement user clip planes for points and lines
 */


/**********************************************************************/
/*****         Vertex Transformation and Clipping                 *****/
/**********************************************************************/

/*
 * Transform a point (column vector) by a matrix:   Q = M * P
 */
#define TRANSFORM_POINT( Q, M, P )					                    \
   Q.x = (M).m00 * P.x + (M).m10 * P.y + (M).m20 * P.z + (M).m30 * P.w; \
   Q.y = (M).m01 * P.x + (M).m11 * P.y + (M).m21 * P.z + (M).m31 * P.w; \
   Q.z = (M).m02 * P.x + (M).m12 * P.y + (M).m22 * P.z + (M).m32 * P.w; \
   Q.w = (M).m03 * P.x + (M).m13 * P.y + (M).m23 * P.z + (M).m33 * P.w;

#define TRANSFORM_POINTA( Q, M, P )					                    \
   Q.x = (M).m00 * (P)[0] + (M).m10 * (P)[1] + (M).m20 * (P)[2] + (M).m30 * (P)[3]; \
   Q.y = (M).m01 * (P)[0] + (M).m11 * (P)[1] + (M).m21 * (P)[2] + (M).m31 * (P)[3]; \
   Q.z = (M).m02 * (P)[0] + (M).m12 * (P)[1] + (M).m22 * (P)[2] + (M).m32 * (P)[3]; \
   Q.w = (M).m03 * (P)[0] + (M).m13 * (P)[1] + (M).m23 * (P)[2] + (M).m33 * (P)[3];

/*
 * clip coord to window coord mapping
 */
#define MAP_POINT( Q, P, VP )                                      \
   Q.x = (GLfloat) (((P.x / P.w) + 1.0) * VP.viewportW / 2.0 + VP.viewportX);  \
   Q.y = (GLfloat) (((P.y / P.w) + 1.0) * VP.viewportH / 2.0 + VP.viewportY);  \
   Q.z = (GLfloat) (((P.z / P.w) + 1.0) * (VP.farClip - VP.nearClip) / 2.0 + VP.nearClip);\
   Q.w = (GLfloat) P.w;


/*
 * Linear interpolation:
 */
#define INTERPOLATE(T, A, B)   ((A) + ((B) - (A)) * (T))


/*
 * Interpolate vertex position, color, texcoords, etc.
 */
static void
interpolate_vertex(GLfloat t,
									 const CRVertex *v0, const CRVertex *v1,
									 CRVertex *vOut)
{
	vOut->eyePos.x = INTERPOLATE(t, v0->eyePos.x, v1->eyePos.x);
	vOut->eyePos.y = INTERPOLATE(t, v0->eyePos.y, v1->eyePos.y);
	vOut->eyePos.z = INTERPOLATE(t, v0->eyePos.z, v1->eyePos.z);
	vOut->eyePos.w = INTERPOLATE(t, v0->eyePos.w, v1->eyePos.w);

	vOut->clipPos.x = INTERPOLATE(t, v0->clipPos.x, v1->clipPos.x);
	vOut->clipPos.y = INTERPOLATE(t, v0->clipPos.y, v1->clipPos.y);
	vOut->clipPos.z = INTERPOLATE(t, v0->clipPos.z, v1->clipPos.z);
	vOut->clipPos.w = INTERPOLATE(t, v0->clipPos.w, v1->clipPos.w);

	vOut->attrib[VERT_ATTRIB_COLOR0][0] = INTERPOLATE(t, v0->attrib[VERT_ATTRIB_COLOR0][0], v1->attrib[VERT_ATTRIB_COLOR0][0]);
	vOut->attrib[VERT_ATTRIB_COLOR0][1] = INTERPOLATE(t, v0->attrib[VERT_ATTRIB_COLOR0][1], v1->attrib[VERT_ATTRIB_COLOR0][1]);
	vOut->attrib[VERT_ATTRIB_COLOR0][2] = INTERPOLATE(t, v0->attrib[VERT_ATTRIB_COLOR0][2], v1->attrib[VERT_ATTRIB_COLOR0][2]);
	vOut->attrib[VERT_ATTRIB_COLOR0][3] = INTERPOLATE(t, v0->attrib[VERT_ATTRIB_COLOR0][3], v1->attrib[VERT_ATTRIB_COLOR0][3]);

	vOut->colorIndex = INTERPOLATE(t, v0->colorIndex, v1->colorIndex);

	vOut->attrib[VERT_ATTRIB_TEX0][0] = INTERPOLATE(t, v0->attrib[VERT_ATTRIB_TEX0][0], v1->attrib[VERT_ATTRIB_TEX0][0]);
	vOut->attrib[VERT_ATTRIB_TEX0][1] = INTERPOLATE(t, v0->attrib[VERT_ATTRIB_TEX0][1], v1->attrib[VERT_ATTRIB_TEX0][0]);
	vOut->attrib[VERT_ATTRIB_TEX0][2] = INTERPOLATE(t, v0->attrib[VERT_ATTRIB_TEX0][2], v1->attrib[VERT_ATTRIB_TEX0][0]);
	vOut->attrib[VERT_ATTRIB_TEX0][3] = INTERPOLATE(t, v0->attrib[VERT_ATTRIB_TEX0][3], v1->attrib[VERT_ATTRIB_TEX0][0]);
}




/* clip bit codes */
#define CLIP_LEFT    1
#define CLIP_RIGHT   2
#define CLIP_BOTTOM  4
#define CLIP_TOP     8
#define CLIP_NEAR   16
#define CLIP_FAR    32
#define CLIP_USER0  64
#define CLIP_USER1 128


/*
 * Apply clip testing to a point.
 * Return:  0 - visible
 *          non-zero - clip code mask (or of above CLIP_ bits)
 */
static GLuint
clip_point(const CRVertex *v)
{
	CRContext *g = GetCurrentContext();
	GLuint mask = 0;
	GLuint i;

	/* user-defined clip planes */
	for (i = 0; i < g->limits.maxClipPlanes; i++)
	{
	   if (g->transform.clip[i])
	   {
		   const GLvectord *plane = g->transform.clipPlane + i;
		   if (plane->x * v->eyePos.x + 
			   plane->y * v->eyePos.y +
			   plane->z * v->eyePos.z +
			   plane->w * v->eyePos.w < 0.0)
			   mask |= (CLIP_USER0 << i);
	   }
	}

	/* view volume clipping */
	if (v->clipPos.x > v->clipPos.w)
		mask |= CLIP_RIGHT;
	if (v->clipPos.x < -v->clipPos.w)
		mask |= CLIP_LEFT;
	if (v->clipPos.y > v->clipPos.w)
		mask |= CLIP_TOP;
	if (v->clipPos.y < -v->clipPos.w)
		mask |= CLIP_BOTTOM;
	if (v->clipPos.z > v->clipPos.w)
		mask |= CLIP_FAR;
	if (v->clipPos.z < -v->clipPos.w)
		mask |= CLIP_NEAR;
	return mask;
}


/*
 * Apply clipping to a line segment.
 * Input:  v0, v1 - incoming vertices
 * Output:  v0out, v1out - result/clipped vertices
 * Return:  GL_TRUE: visible
 *          GL_FALSE: totally clipped
 */
static GLboolean
clip_line(const CRVertex *v0in, const CRVertex *v1in,
					CRVertex *v0new, CRVertex *v1new)
{
	CRVertex v0, v1, vNew;
	GLfloat dx, dy, dz, dw, t;
	GLuint code0, code1;

	/* XXX need to do user-clip planes */

	code0 = clip_point(v0in);
	code1 = clip_point(v1in);
	if (code0 & code1)
		return GL_FALSE;  /* totally clipped */

	*v0new = *v0in;
	*v1new = *v1in;
	if (code0 == 0 && code1 == 0)
		return GL_TRUE;   /* no clipping needed */

	v0 = *v0in;
	v1 = *v1in;


/*
 * We use 6 instances of this code to clip agains the 6 planes.
 * For each plane, we define the OUTSIDE and COMPUTE_INTERSECTION
 * macros appropriately.
 */
#define GENERAL_CLIP                                                    \
   if (OUTSIDE(v0)) {                                                   \
      if (OUTSIDE(v1)) {                                                \
         /* both verts are outside ==> return 0 */                      \
         return 0;                                                      \
      }                                                                 \
      else {                                                            \
         /* v0 is outside, v1 is inside ==> clip */                     \
         COMPUTE_INTERSECTION( v1, v0, vNew )                           \
         interpolate_vertex(t, &v1, &v0, &vNew);                        \
         v0 = vNew;                                                     \
      }                                                                 \
   }                                                                    \
   else {                                                               \
      if (OUTSIDE(v1)) {                                                \
         /* v0 is inside, v1 is outside ==> clip */                     \
         COMPUTE_INTERSECTION( v0, v1, vNew )                           \
         interpolate_vertex(t, &v0, &v1, &vNew);                        \
         v1 = vNew;                                                     \
      }                                                                 \
      /* else both verts are inside ==> do nothing */                   \
   }

   /*** Clip against +X side ***/
#define OUTSIDE(V)      (V.clipPos.x > V.clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )                         \
        dx = OUT.clipPos.x - IN.clipPos.x;                           \
        dw = OUT.clipPos.w - IN.clipPos.w;                           \
        t = (IN.clipPos.x - IN.clipPos.w) / (dw-dx);
   GENERAL_CLIP
#undef OUTSIDE
#undef COMPUTE_INTERSECTION

   /*** Clip against -X side ***/
#define OUTSIDE(V)      (V.clipPos.x < -(V.clipPos.w))
#define COMPUTE_INTERSECTION( IN, OUT, NEW )                         \
        dx = OUT.clipPos.x - IN.clipPos.x;                           \
        dw = OUT.clipPos.w - IN.clipPos.w;                           \
        t = -(IN.clipPos.x + IN.clipPos.w) / (dw+dx);
   GENERAL_CLIP
#undef OUTSIDE
#undef COMPUTE_INTERSECTION

   /*** Clip against +Y side ***/
#define OUTSIDE(V)      (V.clipPos.y > V.clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )                         \
        dy = OUT.clipPos.y - IN.clipPos.y;                           \
        dw = OUT.clipPos.w - IN.clipPos.w;                           \
        t = (IN.clipPos.y - IN.clipPos.w) / (dw-dy);
   GENERAL_CLIP
#undef OUTSIDE
#undef COMPUTE_INTERSECTION

   /*** Clip against -Y side ***/
#define OUTSIDE(V)      (V.clipPos.y < -(V.clipPos.w))
#define COMPUTE_INTERSECTION( IN, OUT, NEW )                         \
        dy = OUT.clipPos.y - IN.clipPos.y;                           \
        dw = OUT.clipPos.w - IN.clipPos.w;                           \
        t = -(IN.clipPos.y + IN.clipPos.w) / (dw+dy);
   GENERAL_CLIP
#undef OUTSIDE
#undef COMPUTE_INTERSECTION

   /*** Clip against +Z side ***/
#define OUTSIDE(V)      (V.clipPos.z > V.clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )                         \
        dz = OUT.clipPos.z - IN.clipPos.z;                           \
        dw = OUT.clipPos.w - IN.clipPos.w;                           \
        t = (IN.clipPos.z - IN.clipPos.w) / (dw-dz);
   GENERAL_CLIP
#undef OUTSIDE
#undef COMPUTE_INTERSECTION

   /*** Clip against -Z side ***/
#define OUTSIDE(V)      (V.clipPos.z < -(V.clipPos.w))
#define COMPUTE_INTERSECTION( IN, OUT, NEW )                         \
        dz = OUT.clipPos.z - IN.clipPos.z;                           \
        dw = OUT.clipPos.w - IN.clipPos.w;                           \
        t = -(IN.clipPos.z + IN.clipPos.w) / (dw+dz);
   GENERAL_CLIP
#undef OUTSIDE
#undef COMPUTE_INTERSECTION

#undef GENERAL_CLIP

   *v0new = v0;
   *v1new = v1;
   return GL_TRUE;
}



/*
 * Apply clipping to a polygon.
 * Input: vIn - array of input vertices
 *        inCount - number of input vertices
 * Output: vOut - new vertices
 * Return: number of vertices in vOut
 */
static GLuint
clip_polygon(const CRVertex *vIn, unsigned int inCount,
						 CRVertex *vOut)
{
	CRVertex inlist[20], outlist[20];
	GLfloat dx, dy, dz, dw, t;
	GLuint incount, outcount, previ, curri, result;
	const CRVertex *currVert, *prevVert;
	CRVertex *newVert;

	/* XXX need to do user-clip planes */

#define GENERAL_CLIP(INCOUNT, INLIST, OUTCOUNT, OUTLIST)                \
   if (INCOUNT < 3)                                                     \
      return GL_FALSE;                                                  \
   previ = INCOUNT - 1;         /* let previous = last vertex */        \
   prevVert = INLIST + previ;                                           \
   OUTCOUNT = 0;                                                        \
   for (curri = 0; curri < INCOUNT; curri++) {                          \
      currVert = INLIST + curri;                                        \
      if (INSIDE(currVert)) {                                           \
         if (INSIDE(prevVert)) {                                        \
            /* both verts are inside ==> copy current to outlist */     \
            OUTLIST[OUTCOUNT] = *currVert;                              \
            OUTCOUNT++;                                                 \
         }                                                              \
         else {                                                         \
            newVert = OUTLIST + OUTCOUNT;                               \
            /* current is inside and previous is outside ==> clip */    \
            COMPUTE_INTERSECTION( currVert, prevVert, newVert )         \
            OUTCOUNT++;                                                 \
            /* Output current */                                        \
            OUTLIST[OUTCOUNT] = *currVert;                              \
            OUTCOUNT++;                                                 \
         }                                                              \
      }                                                                 \
      else {                                                            \
         if (INSIDE(prevVert)) {                                        \
            newVert = OUTLIST + OUTCOUNT;                               \
            /* current is outside and previous is inside ==> clip */    \
            COMPUTE_INTERSECTION( prevVert, currVert, newVert );        \
            OUTLIST[OUTCOUNT] = *newVert;                               \
            OUTCOUNT++;                                                 \
         }                                                              \
         /* else both verts are outside ==> do nothing */               \
      }                                                                 \
      /* let previous = current */                                      \
      previ = curri;                                                    \
      prevVert = currVert;                                              \
   }

/*
 * Clip against +X
 */
#define INSIDE(V)       (V->clipPos.x <= V->clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )              \
        dx = OUT->clipPos.x - IN->clipPos.x;              \
        dw = OUT->clipPos.w - IN->clipPos.w;              \
        t = (IN->clipPos.x - IN->clipPos.w) / (dw - dx);  \
        interpolate_vertex(t, IN, OUT, NEW );

   GENERAL_CLIP(inCount, vIn, outcount, outlist)

#undef INSIDE
#undef COMPUTE_INTERSECTION

/*
 * Clip against -X
 */
#define INSIDE(V)       (V->clipPos.x >= -V->clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )               \
        dx = OUT->clipPos.x - IN->clipPos.x;               \
        dw = OUT->clipPos.w - IN->clipPos.w;               \
        t = -(IN->clipPos.x + IN->clipPos.w) / (dw + dx);  \
        interpolate_vertex(t, IN, OUT, NEW );

   GENERAL_CLIP(outcount, outlist, incount, inlist)

#undef INSIDE
#undef COMPUTE_INTERSECTION

/*
 * Clip against +Y
 */
#define INSIDE(V)       (V->clipPos.y <= V->clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )              \
        dy = OUT->clipPos.y - IN->clipPos.y;              \
        dw = OUT->clipPos.w - IN->clipPos.w;              \
        t = (IN->clipPos.y - IN->clipPos.w) / (dw - dy);  \
        interpolate_vertex(t, IN, OUT, NEW );

   GENERAL_CLIP(incount, inlist, outcount, outlist)

#undef INSIDE
#undef COMPUTE_INTERSECTION

/*
 * Clip against -Y
 */
#define INSIDE(V)       (V->clipPos.y >= -V->clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )               \
        dy = OUT->clipPos.y - IN->clipPos.y;               \
        dw = OUT->clipPos.w - IN->clipPos.w;               \
        t = -(IN->clipPos.y + IN->clipPos.w) / (dw + dy);  \
        interpolate_vertex(t, IN, OUT, NEW );

   GENERAL_CLIP(outcount, outlist, incount, inlist)

#undef INSIDE
#undef COMPUTE_INTERSECTION

/*
 * Clip against +Z
 */
#define INSIDE(V)       (V->clipPos.z <= V->clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )              \
        dz = OUT->clipPos.z - IN->clipPos.z;              \
        dw = OUT->clipPos.w - IN->clipPos.w;              \
        t = (IN->clipPos.z - IN->clipPos.w) / (dw - dz);  \
        interpolate_vertex(t, IN, OUT, NEW );

   GENERAL_CLIP(incount, inlist, outcount, outlist)

#undef INSIDE
#undef COMPUTE_INTERSECTION

/*
 * Clip against -Z
 */
#define INSIDE(V)       (V->clipPos.z >= -V->clipPos.w)
#define COMPUTE_INTERSECTION( IN, OUT, NEW )               \
        dz = OUT->clipPos.z - IN->clipPos.z;               \
        dw = OUT->clipPos.w - IN->clipPos.w;               \
        t = -(IN->clipPos.z + IN->clipPos.w) / (dw + dz);  \
        interpolate_vertex(t, IN, OUT, NEW );

   GENERAL_CLIP(outcount, outlist, result, vOut)

#undef INSIDE
#undef COMPUTE_INTERSECTION

#undef GENERAL_CLIP

	return result;
}


/**********************************************************************/
/*****                       Feedback                             *****/
/**********************************************************************/


#define FB_3D		0x01
#define FB_4D		0x02
#define FB_INDEX	0x04
#define FB_COLOR	0x08
#define FB_TEXTURE	0X10

#define FEEDBACK_TOKEN( T )                  \
  do {                                       \
    if (f->count < f->bufferSize) {          \
      f->buffer[f->count] = (GLfloat) (T);   \
    }							             \
    f->count++;                              \
  } while (0)

/*
 * Put a vertex into the feedback buffer.
 */
static void
feedback_vertex(const CRVertex *v)
{
	CRContext *g = GetCurrentContext();
	CRFeedbackState *f = &(g->feedback);
	CRTransformState *t = &(g->transform);

	FEEDBACK_TOKEN(v->winPos.x);
	FEEDBACK_TOKEN(v->winPos.y);

	if (f->mask & FB_3D)
	{
		FEEDBACK_TOKEN(v->winPos.z);
	}

	if (f->mask & FB_4D)
	{
		FEEDBACK_TOKEN(v->winPos.w);
	}

	/* We don't deal with color index in Chromium */
	if (f->mask & FB_INDEX)
	{
		FEEDBACK_TOKEN(v->colorIndex);
	}

	if (f->mask & FB_COLOR)
	{
		FEEDBACK_TOKEN(v->attrib[VERT_ATTRIB_COLOR0][0]);
		FEEDBACK_TOKEN(v->attrib[VERT_ATTRIB_COLOR0][1]);
		FEEDBACK_TOKEN(v->attrib[VERT_ATTRIB_COLOR0][2]);
		FEEDBACK_TOKEN(v->attrib[VERT_ATTRIB_COLOR0][3]);
	}

	if (f->mask & FB_TEXTURE)
	{
		GLvectorf coord, transCoord;
		/* Ugh, copy (s,t,r,q) to (x,y,z,w) */
		coord.x = v->attrib[VERT_ATTRIB_TEX0][0];
		coord.y = v->attrib[VERT_ATTRIB_TEX0][1];
		coord.z = v->attrib[VERT_ATTRIB_TEX0][2];
		coord.w = v->attrib[VERT_ATTRIB_TEX0][3];
		TRANSFORM_POINT(transCoord, *(t->textureStack[0].top), coord);
		FEEDBACK_TOKEN(transCoord.x);
		FEEDBACK_TOKEN(transCoord.y);
		FEEDBACK_TOKEN(transCoord.z);
		FEEDBACK_TOKEN(transCoord.w);
	}
}



static void
feedback_rasterpos(void)
{
	CRContext *g = GetCurrentContext();
	CRVertex *tv = g->vBuffer + g->vCount;
	CRVertex v;

	v.winPos.x = g->current.rasterAttrib[VERT_ATTRIB_POS][0];
	v.winPos.y = g->current.rasterAttrib[VERT_ATTRIB_POS][1];
	v.winPos.z = g->current.rasterAttrib[VERT_ATTRIB_POS][2];
	v.winPos.w = g->current.rasterAttrib[VERT_ATTRIB_POS][3];
	COPY_4V(v.attrib[VERT_ATTRIB_COLOR0] , g->current.rasterAttrib[VERT_ATTRIB_COLOR0]);  /* XXX need to apply lighting */
	COPY_4V(v.attrib[VERT_ATTRIB_COLOR1] , g->current.rasterAttrib[VERT_ATTRIB_COLOR1]);
	v.colorIndex = (GLfloat) g->current.rasterIndex;

	/* Don't do this, we're capturing TexCoord ourselves and
	 * we'd miss the conversion in RasterPosUpdate */
	/* v.texCoord[0] = g->current.rasterTexture; */

	/* So we do this instead, and pluck it from the current vertex */
	COPY_4V(v.attrib[VERT_ATTRIB_TEX0] , tv->attrib[VERT_ATTRIB_TEX0]);

	feedback_vertex(&v);
}


static void
feedback_point(const CRVertex *v)
{
	CRContext *g = GetCurrentContext();
	CRFeedbackState *f = &(g->feedback);
	if (clip_point(v) == 0)
	{
		CRVertex c = *v;
		MAP_POINT(c.winPos, c.clipPos, g->viewport);
		FEEDBACK_TOKEN((GLfloat) GL_POINT_TOKEN);
		feedback_vertex(&c);
	}
}


static void
feedback_line(const CRVertex *v0, const CRVertex *v1, GLboolean reset)
{
	CRContext *g = GetCurrentContext();
	CRFeedbackState *f = &(g->feedback);
	CRVertex c0, c1;
	if (clip_line(v0, v1, &c0, &c1))
	{
		MAP_POINT(c0.winPos, c0.clipPos, g->viewport);
		MAP_POINT(c1.winPos, c1.clipPos, g->viewport);
		if (reset)
			FEEDBACK_TOKEN((GLfloat) GL_LINE_RESET_TOKEN);
		else
			FEEDBACK_TOKEN((GLfloat) GL_LINE_TOKEN);
		feedback_vertex(&c0);
		feedback_vertex(&c1);
	}
}


static void
feedback_triangle(const CRVertex *v0, const CRVertex *v1, const CRVertex *v2)
{
	CRContext *g = GetCurrentContext();
	CRFeedbackState *f = &(g->feedback);
	CRVertex vlist[3], vclipped[8];
	GLuint i, n;

	vlist[0] = *v0;
	vlist[1] = *v1;
	vlist[2] = *v2;
	n = clip_polygon(vlist, 3, vclipped);
	FEEDBACK_TOKEN( (GLfloat) GL_POLYGON_TOKEN );
	FEEDBACK_TOKEN( (GLfloat) n );
	for (i = 0; i < n; i++) {
		MAP_POINT(vclipped[i].winPos, vclipped[i].clipPos, g->viewport);
		feedback_vertex(vclipped + i);
	}
}


void STATE_APIENTRY
crStateFeedbackVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	CRContext *g = GetCurrentContext();
	CRTransformState *t = &(g->transform);
	CRPolygonState *p = &(g->polygon);
	CRVertex *v = g->vBuffer + g->vCount;

	/* store the vertex */
	v->attrib[VERT_ATTRIB_POS][0] = x;
	v->attrib[VERT_ATTRIB_POS][1] = y;
	v->attrib[VERT_ATTRIB_POS][2] = z;
	v->attrib[VERT_ATTRIB_POS][3] = w;
	COPY_4V(v->attrib[VERT_ATTRIB_COLOR0] , g->current.vertexAttrib[VERT_ATTRIB_COLOR0]);  /* XXX need to apply lighting */
	v->colorIndex = g->current.colorIndex;  /* XXX need to apply lighting */
	/* Don't do this, we're capturing TexCoord ourselves as 
	 * we don't have a pincher like the tilesortSPU */
	/* v->texCoord[0] = g->current.texCoord[0]; */

	/* transform to eye space, then clip space */
	TRANSFORM_POINTA(v->eyePos, *(t->modelViewStack.top), v->attrib[VERT_ATTRIB_POS]);
	TRANSFORM_POINT(v->clipPos, *(t->projectionStack.top), v->eyePos);

	switch (g->current.mode) {
	case GL_POINTS:
		CRASSERT(g->vCount == 0);
		feedback_point(v);
		break;
	case GL_LINES:
		if (g->vCount == 0)
		{
			g->vCount = 1;
		}
		else
		{
			CRASSERT(g->vCount == 1);
			feedback_line(g->vBuffer + 0, g->vBuffer + 1, g->lineReset);
			g->vCount = 0;
		}
		break;
	case GL_LINE_STRIP:
		if (g->vCount == 0)
		{
			g->vCount = 1;
		}
		else
		{
			CRASSERT(g->vCount == 1);
			feedback_line(g->vBuffer + 0, g->vBuffer + 1, g->lineReset);
			g->vBuffer[0] = g->vBuffer[1];
			g->lineReset = GL_FALSE;
			/* leave g->vCount at 1 */
		}
		break;
	case GL_LINE_LOOP:
		if (g->vCount == 0)
		{
			g->lineLoop = GL_FALSE;
			g->vCount = 1;
		}
		else if (g->vCount == 1)
		{
			feedback_line(g->vBuffer + 0, g->vBuffer + 1, g->lineReset);
			g->lineReset = GL_FALSE;
			g->lineLoop = GL_TRUE;
			g->vCount = 2;
		}
		else
		{
			CRASSERT(g->vCount == 2);
			CRASSERT(g->lineReset == GL_FALSE);
			g->lineLoop = GL_FALSE;
			feedback_line(g->vBuffer + 1, g->vBuffer + 2, g->lineReset);
			g->vBuffer[1] = g->vBuffer[2];
			/* leave g->vCount at 2 */
		}
		break;
	case GL_TRIANGLES:
		if (g->vCount == 0 || g->vCount == 1)
		{
			g->vCount++;
		}
		else
		{
			CRASSERT(g->vCount == 2);
			feedback_triangle(g->vBuffer + 0, g->vBuffer + 1, g->vBuffer + 2);
			g->vCount = 0;
		}
		break;
	case GL_TRIANGLE_STRIP:
		if (g->vCount == 0 || g->vCount == 1)
		{
			g->vCount++;
		}
		else if (g->vCount == 2)
		{
			feedback_triangle(g->vBuffer + 0, g->vBuffer + 1, g->vBuffer + 2);
			g->vCount = 3;
		}
		else
		{
			CRASSERT(g->vCount == 3);
			feedback_triangle(g->vBuffer + 1, g->vBuffer + 3, g->vBuffer + 2);
			g->vBuffer[0] = g->vBuffer[2];
			g->vBuffer[1] = g->vBuffer[3];
			g->vCount = 2;
		}
		break;		
	case GL_TRIANGLE_FAN:
		if (g->vCount == 0 || g->vCount == 1)
		{
			g->vCount++;
		}
		else
		{
			CRASSERT(g->vCount == 2);
			feedback_triangle(g->vBuffer + 0, g->vBuffer + 1, g->vBuffer + 2);
			g->vBuffer[1] = g->vBuffer[2];
			/* leave g->vCount = 2 */
		}
		break;
	case GL_QUADS:
		if (g->vCount < 3)
		{
			g->vCount++;
		}
		else
		{
			CRASSERT(g->vCount == 3);
			feedback_triangle(g->vBuffer + 0, g->vBuffer + 1, g->vBuffer + 2);
			feedback_triangle(g->vBuffer + 0, g->vBuffer + 2, g->vBuffer + 3);
			g->vCount = 0;
		}
		break;		
	case GL_QUAD_STRIP:
		if (g->vCount < 3)
		{
			g->vCount++;
		}
		else
		{
			CRASSERT(g->vCount == 3);
			feedback_triangle(g->vBuffer + 0, g->vBuffer + 1, g->vBuffer + 2);
			feedback_triangle(g->vBuffer + 1, g->vBuffer + 3, g->vBuffer + 2);
			g->vBuffer[0] = g->vBuffer[2];
			g->vBuffer[1] = g->vBuffer[3];
			g->vCount = 2;
		}
		break;		
	case GL_POLYGON:
		/* XXX need to observe polygon mode for the above TRI/QUAD prims */
		switch (p->frontMode) {
		case GL_POINT:
			CRASSERT(g->vCount == 0);
			feedback_point(v);
			break;
		case GL_LINE:
			if (g->vCount == 0)
			{
				g->lineLoop = GL_FALSE;
				g->vCount = 1;
			}
			else if (g->vCount == 1)
			{
				feedback_line(g->vBuffer + 0, g->vBuffer + 1, g->lineReset);
				g->lineReset = GL_FALSE;
				g->lineLoop = GL_TRUE;
				g->vCount = 2;
			}
			else
			{
				CRASSERT(g->vCount == 2);
				CRASSERT(g->lineReset == GL_FALSE);
				g->lineLoop = GL_FALSE;
				feedback_line(g->vBuffer + 1, g->vBuffer + 2, g->lineReset);
				g->vBuffer[1] = g->vBuffer[2];
				/* leave g->vCount at 2 */
			}
			break;
		case GL_FILL:
			/* draw as a tri-fan */
			if (g->vCount == 0 || g->vCount == 1)
			{
				g->vCount++;
			}
			else
			{
				CRASSERT(g->vCount == 2);
				feedback_triangle(g->vBuffer + 0, g->vBuffer + 1, g->vBuffer + 2);
				g->vBuffer[1] = g->vBuffer[2];
				/* leave g->vCount = 2 */
			}
			break;
		default:
			; /* impossible */
		}
		break;
	default:
		; /* impossible */
	}
}


void STATE_APIENTRY
crStateFeedbackBegin(GLenum mode)
{
	CRContext *g = GetCurrentContext();

	crStateBegin(mode);

	g->vCount = 0;
	g->lineReset = GL_TRUE;
	g->lineLoop = GL_FALSE;
}


void STATE_APIENTRY
crStateFeedbackEnd(void)
{
	CRContext *g = GetCurrentContext();

	if ( (g->current.mode == GL_LINE_LOOP ||
	     (g->current.mode == GL_POLYGON && g->polygon.frontMode == GL_LINE))
	     && g->vCount == 2 )
	{
		/* draw the last line segment */
		if (g->lineLoop)
			feedback_line(g->vBuffer + 1, g->vBuffer + 0, GL_FALSE);
		else
			feedback_line(g->vBuffer + 2, g->vBuffer + 0, GL_FALSE);
	}

	crStateEnd();
}


void STATE_APIENTRY
crStateFeedbackBuffer(GLsizei size, GLenum type, GLfloat * buffer)
{
	CRContext *g = GetCurrentContext();
	CRFeedbackState *f = &(g->feedback);

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "FeedbackBuffer called in begin/end");
		return;
	}

	if (g->renderMode == GL_FEEDBACK)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "Invalid Operation GL_FEEDBACK");
		return;
	}
	if (size < 0)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
								 "Invalid Value size < 0");
		return;
	}
	if (!buffer)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
								 "Invalid Value buffer = NULL");
		f->bufferSize = 0;
		return;
	}

	FLUSH();

	switch (type)
	{
	case GL_2D:
		f->mask = 0;
		break;
	case GL_3D:
		f->mask = FB_3D;
		break;
	case GL_3D_COLOR:
		f->mask = (FB_3D | FB_COLOR);	/* FB_INDEX ?? */
		break;
	case GL_3D_COLOR_TEXTURE:
		f->mask = (FB_3D | FB_COLOR | FB_TEXTURE);	/* FB_INDEX ?? */
		break;
	case GL_4D_COLOR_TEXTURE:
		f->mask = (FB_3D | FB_4D | FB_COLOR | FB_TEXTURE);	/* FB_INDEX ?? */
		break;
	default:
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "invalid type");
		return;
	}

	f->type = type;
	f->bufferSize = size;
	f->buffer = buffer;
	f->count = 0;
}


void STATE_APIENTRY
crStatePassThrough(GLfloat token)
{
	CRContext *g = GetCurrentContext();
	CRFeedbackState *f = &(g->feedback);

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "PassThrough called in begin/end");
		return;
	}

	FLUSH();

	if (g->renderMode == GL_FEEDBACK)
	{
		FEEDBACK_TOKEN((GLfloat) (GLint) GL_PASS_THROUGH_TOKEN);
		FEEDBACK_TOKEN(token);
	}
}


/* 
 * Although these functions are used by the feedbackSPU alone, 
 * I've left them here as they interface to the other functions.....
 */
void STATE_APIENTRY
crStateFeedbackGetBooleanv(GLenum pname, GLboolean * params)
{
	CRContext *g = GetCurrentContext();

	switch (pname)
	{

	case GL_FEEDBACK_BUFFER_SIZE:
		params[0] = (GLboolean) (g->feedback.bufferSize != 0);
		break;
	case GL_FEEDBACK_BUFFER_TYPE:
		params[0] = (GLboolean) (g->feedback.type != 0);
		break;
	case GL_SELECTION_BUFFER_SIZE:
		params[0] = (GLboolean) (g->selection.bufferSize != 0);
		break;
	default:
		break;
	}
}


void STATE_APIENTRY
crStateFeedbackGetDoublev(GLenum pname, GLdouble * params)
{
	CRContext *g = GetCurrentContext();

	switch (pname)
	{

	case GL_FEEDBACK_BUFFER_SIZE:
		params[0] = (GLdouble) g->feedback.bufferSize;
		break;
	case GL_FEEDBACK_BUFFER_TYPE:
		params[0] = (GLdouble) g->feedback.type;
		break;
	case GL_SELECTION_BUFFER_SIZE:
		params[0] = (GLdouble) g->selection.bufferSize;
		break;
	default:
		break;
	}
}


void STATE_APIENTRY
crStateFeedbackGetFloatv(GLenum pname, GLfloat * params)
{
	CRContext *g = GetCurrentContext();

	switch (pname)
	{

	case GL_FEEDBACK_BUFFER_SIZE:
		params[0] = (GLfloat) g->feedback.bufferSize;
		break;
	case GL_FEEDBACK_BUFFER_TYPE:
		params[0] = (GLfloat) g->feedback.type;
		break;
	case GL_SELECTION_BUFFER_SIZE:
		params[0] = (GLfloat) g->selection.bufferSize;
		break;
	default:
		break;
	}
}


void STATE_APIENTRY
crStateFeedbackGetIntegerv(GLenum pname, GLint * params)
{
	CRContext *g = GetCurrentContext();

	switch (pname)
	{

	case GL_FEEDBACK_BUFFER_SIZE:
		params[0] = (GLint) g->feedback.bufferSize;
		break;
	case GL_FEEDBACK_BUFFER_TYPE:
		params[0] = (GLint) g->feedback.type;
		break;
	case GL_SELECTION_BUFFER_SIZE:
		params[0] = (GLint) g->selection.bufferSize;
		break;
	default:
		break;
	}
}


void STATE_APIENTRY
crStateFeedbackDrawPixels(GLsizei width, GLsizei height, GLenum format,
													GLenum type, const GLvoid * pixels)
{
	CRContext *g = GetCurrentContext();
	CRFeedbackState *f = &(g->feedback);

	(void) width;
	(void) height;
	(void) format;
	(void) type;
	(void) pixels;

	FEEDBACK_TOKEN((GLfloat) (GLint) GL_DRAW_PIXEL_TOKEN);

	feedback_rasterpos();
}

void STATE_APIENTRY
crStateFeedbackCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height,
													GLenum type)
{
	CRContext *g = GetCurrentContext();
	CRFeedbackState *f = &(g->feedback);

	(void) x;
	(void) y;
	(void) width;
	(void) height;
	(void) type;

	FEEDBACK_TOKEN((GLfloat) (GLint) GL_COPY_PIXEL_TOKEN);
	feedback_rasterpos();
}

void STATE_APIENTRY
crStateFeedbackBitmap(GLsizei width, GLsizei height, GLfloat xorig,
											GLfloat yorig, GLfloat xmove, GLfloat ymove,
											const GLubyte * bitmap)
{
	CRContext *g = GetCurrentContext();
	CRFeedbackState *f = &(g->feedback);

	(void) width;
	(void) height;
	(void) bitmap;
	(void) xorig;
	(void) yorig;

	FEEDBACK_TOKEN((GLfloat) (GLint) GL_BITMAP_TOKEN);

	feedback_rasterpos();

	if (g->current.rasterValid)
	{
		g->current.rasterAttrib[VERT_ATTRIB_POS][0] += xmove;
		g->current.rasterAttrib[VERT_ATTRIB_POS][1] += ymove;
	}
}


void STATE_APIENTRY
crStateFeedbackVertex4fv(const GLfloat * v)
{
	crStateFeedbackVertex4f(v[0], v[1], v[2], v[3]);
}

void STATE_APIENTRY
crStateFeedbackVertex4s(GLshort v0, GLshort v1, GLshort v2, GLshort v3)
{
	crStateFeedbackVertex4f((GLfloat) v0, (GLfloat) v1, (GLfloat) v2,
													(GLfloat) v3);
}

void STATE_APIENTRY
crStateFeedbackVertex4sv(const GLshort * v)
{
	crStateFeedbackVertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2],
													(GLfloat) v[3]);
}

void STATE_APIENTRY
crStateFeedbackVertex4i(GLint v0, GLint v1, GLint v2, GLint v3)
{
	crStateFeedbackVertex4f((GLfloat) v0, (GLfloat) v1, (GLfloat) v2,
													(GLfloat) v3);
}

void STATE_APIENTRY
crStateFeedbackVertex4iv(const GLint * v)
{
	crStateFeedbackVertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2],
													(GLfloat) v[3]);
}

void STATE_APIENTRY
crStateFeedbackVertex4d(GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3)
{
	crStateFeedbackVertex4f((GLfloat) v0, (GLfloat) v1, (GLfloat) v2,
													(GLfloat) v3);
}

void STATE_APIENTRY
crStateFeedbackVertex4dv(const GLdouble * v)
{
	crStateFeedbackVertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2],
													(GLfloat) v[3]);
}

void STATE_APIENTRY
crStateFeedbackVertex2i(GLint v0, GLint v1)
{
	crStateFeedbackVertex4f((GLfloat) v0, (GLfloat) v1, 0.0f, 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex2iv(const GLint * v)
{
	crStateFeedbackVertex4f((GLfloat) v[0], (GLfloat) v[1], 0.0f, 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex2s(GLshort v0, GLshort v1)
{
	crStateFeedbackVertex4f((GLfloat) v0, (GLfloat) v1, 0.0f, 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex2sv(const GLshort * v)
{
	crStateFeedbackVertex4f((GLfloat) v[0], (GLfloat) v[1], 0.0f, 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex2f(GLfloat v0, GLfloat v1)
{
	crStateFeedbackVertex4f(v0, v1, 0.0f, 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex2fv(const GLfloat * v)
{
	crStateFeedbackVertex4f(v[0], v[1], 0.0f, 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex2d(GLdouble v0, GLdouble v1)
{
	crStateFeedbackVertex4f((GLfloat) v0, (GLfloat) v1, 0.0f, 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex2dv(const GLdouble * v)
{
	crStateFeedbackVertex4f((GLfloat) v[0], (GLfloat) v[1], 0.0f, 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex3i(GLint v0, GLint v1, GLint v2)
{
	crStateFeedbackVertex4f((GLfloat) v0, (GLfloat) v1, (GLfloat) v2, 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex3iv(const GLint * v)
{
	crStateFeedbackVertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex3s(GLshort v0, GLshort v1, GLshort v2)
{
	crStateFeedbackVertex4f((GLfloat) v0, (GLfloat) v1, (GLfloat) v2, 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex3sv(const GLshort * v)
{
	crStateFeedbackVertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2],
													1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex3f(GLfloat v0, GLfloat v1, GLfloat v2)
{
	crStateFeedbackVertex4f(v0, v1, v2, 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex3fv(const GLfloat * v)
{
	crStateFeedbackVertex4f(v[0], v[1], v[2], 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex3d(GLdouble v0, GLdouble v1, GLdouble v2)
{
	crStateFeedbackVertex4f((GLfloat) v0, (GLfloat) v1, (GLfloat) v2, 1.0f);
}

void STATE_APIENTRY
crStateFeedbackVertex3dv(const GLdouble * v)
{
	crStateFeedbackVertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2],
													1.0f);
}

void STATE_APIENTRY
crStateFeedbackTexCoord4f( GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 )
{
	CRContext *g = GetCurrentContext();
	CRVertex *v = g->vBuffer + g->vCount;

	/* store the texCoord in the current vertex */
	v->attrib[VERT_ATTRIB_TEX0][0] = v0;
	v->attrib[VERT_ATTRIB_TEX0][1] = v1;
	v->attrib[VERT_ATTRIB_TEX0][2] = v2;
	v->attrib[VERT_ATTRIB_TEX0][3] = v3;
}
 
void STATE_APIENTRY
crStateFeedbackTexCoord4fv( const GLfloat *v )
{
	crStateFeedbackTexCoord4f( v[0], v[1], v[2], v[3] );
}

void STATE_APIENTRY
crStateFeedbackTexCoord4s( GLshort v0, GLshort v1, GLshort v2, GLshort v3 )
{
	crStateFeedbackTexCoord4f( (GLfloat)v0, (GLfloat)v1, (GLfloat)v2, (GLfloat)v3 );
}

void STATE_APIENTRY
crStateFeedbackTexCoord4sv( const GLshort *v )
{
	crStateFeedbackTexCoord4f( (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3] );
}

void STATE_APIENTRY
crStateFeedbackTexCoord4i( GLint v0, GLint v1, GLint v2, GLint v3 )
{
	crStateFeedbackTexCoord4f( (GLfloat)v0, (GLfloat)v1, (GLfloat)v2, (GLfloat)v3 );
}

void STATE_APIENTRY
crStateFeedbackTexCoord4iv( const GLint *v )
{
	crStateFeedbackTexCoord4f( (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3] );
}

void STATE_APIENTRY
crStateFeedbackTexCoord4d( GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3 )
{
	crStateFeedbackTexCoord4f( (GLfloat)v0, (GLfloat)v1, (GLfloat)v2, (GLfloat)v3 );
}

void STATE_APIENTRY
crStateFeedbackTexCoord4dv( const GLdouble *v )
{
	crStateFeedbackTexCoord4f( (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3] );
}

void STATE_APIENTRY
crStateFeedbackTexCoord1i( GLint v0 )
{
	crStateFeedbackTexCoord4f( (GLfloat)v0, 0.0f, 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord1iv( const GLint *v )
{
	crStateFeedbackTexCoord4f( (GLfloat)v[0], 0.0f, 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord1s( GLshort v0 )
{
	crStateFeedbackTexCoord4f( (GLfloat)v0, 0.0f, 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord1sv( const GLshort *v )
{
	crStateFeedbackTexCoord4f( (GLfloat)v[0], 0.0f, 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord1f( GLfloat v0 )
{
	crStateFeedbackTexCoord4f( v0, 0.0f, 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord1fv( const GLfloat *v )
{
	crStateFeedbackTexCoord4f( v[0], 0.0f, 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord1d( GLdouble v0 )
{
	crStateFeedbackTexCoord4f( (GLfloat)v0, 0.0f, 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord1dv( const GLdouble *v )
{
	crStateFeedbackTexCoord4f( (GLfloat)v[0], 0.0f, 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord2i( GLint v0, GLint v1 )
{
	crStateFeedbackTexCoord4f( (GLfloat)v0, (GLfloat)v1, 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord2iv( const GLint *v )
{
	crStateFeedbackTexCoord4f( (GLfloat)v[0], (GLfloat)v[1], 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord2s( GLshort v0, GLshort v1 )
{
	crStateFeedbackTexCoord4f( (GLfloat)v0, (GLfloat)v1, 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord2sv( const GLshort *v )
{
	crStateFeedbackTexCoord4f( (GLfloat)v[0], (GLfloat)v[1], 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord2f( GLfloat v0, GLfloat v1 )
{
	crStateFeedbackTexCoord4f( v0, v1, 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord2fv( const GLfloat *v )
{
	crStateFeedbackTexCoord4f( v[0], v[1], 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord2d( GLdouble v0, GLdouble v1 )
{
	crStateFeedbackTexCoord4f( (GLfloat)v0, (GLfloat)v1, 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord2dv( const GLdouble *v )
{
	crStateFeedbackTexCoord4f( (GLfloat)v[0], (GLfloat)v[1], 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord3i( GLint v0, GLint v1, GLint v2 )
{
	crStateFeedbackTexCoord4f( (GLfloat)v0, (GLfloat)v1, (GLfloat)v2, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord3iv( const GLint *v )
{
	crStateFeedbackTexCoord4f( (GLfloat)v[0], (GLfloat)v[1], 0.0f, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord3s( GLshort v0, GLshort v1, GLshort v2 )
{
	crStateFeedbackTexCoord4f( (GLfloat)v0, (GLfloat)v1, (GLfloat)v2, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord3sv( const GLshort *v )
{
	crStateFeedbackTexCoord4f( (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord3f( GLfloat v0, GLfloat v1, GLfloat v2 )
{
	crStateFeedbackTexCoord4f( v0, v1, v2, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord3fv( const GLfloat *v )
{
	crStateFeedbackTexCoord4f( v[0], v[1], v[2], 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord3d( GLdouble v0, GLdouble v1, GLdouble v2 )
{
	crStateFeedbackTexCoord4f( (GLfloat)v0, (GLfloat)v1, (GLfloat)v2, 1.0f );
}

void STATE_APIENTRY
crStateFeedbackTexCoord3dv( const GLdouble *v )
{
	crStateFeedbackTexCoord4f( (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], 1.0f );
}

void STATE_APIENTRY
crStateFeedbackRectf(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1)
{
   crStateFeedbackBegin(GL_QUADS);
   crStateFeedbackVertex2f(x0, y0);
   crStateFeedbackVertex2f(x0, y1);
   crStateFeedbackVertex2f(x1, y1);
   crStateFeedbackVertex2f(x1, y0);
   crStateFeedbackEnd();
}

void STATE_APIENTRY
crStateFeedbackRecti(GLint x0, GLint y0, GLint x1, GLint y1)
{
   crStateFeedbackRectf((GLfloat) x0, (GLfloat) y0, (GLfloat) x1, (GLfloat) y1);
}

void STATE_APIENTRY
crStateFeedbackRectd(GLdouble x0, GLdouble y0, GLdouble x1, GLdouble y1)
{
   crStateFeedbackRectf((GLfloat) x0, (GLfloat) y0, (GLfloat) x1, (GLfloat) y1);
}

void STATE_APIENTRY
crStateFeedbackRects(GLshort x0, GLshort y0, GLshort x1, GLshort y1)
{
   crStateFeedbackRectf((GLfloat) x0, (GLfloat) y0, (GLfloat) x1, (GLfloat) y1);
}

void STATE_APIENTRY
crStateFeedbackRectiv(const GLint *v0, const GLint *v1)
{
   crStateFeedbackRectf((GLfloat) v0[0], (GLfloat) v0[1], (GLfloat) v1[0], (GLfloat) v1[1]);
}

void STATE_APIENTRY
crStateFeedbackRectfv(const GLfloat *v0, const GLfloat *v1)
{
   crStateFeedbackRectf(v0[0], v0[1], v1[0], v1[1]);
}

void STATE_APIENTRY
crStateFeedbackRectdv(const GLdouble *v0, const GLdouble *v1)
{
   crStateFeedbackRectf((GLfloat) v0[0], (GLfloat) v0[1], (GLfloat) v1[0], (GLfloat) v1[1]);
}

void STATE_APIENTRY
crStateFeedbackRectsv(const GLshort *v0, const GLshort *v1)
{
   crStateFeedbackRectf((GLfloat) v0[0], (GLfloat) v0[1], (GLfloat) v1[0], (GLfloat) v1[1]);
}

/**********************************************************************/
/*****                       Selection                            *****/
/**********************************************************************/


void STATE_APIENTRY
crStateSelectBuffer(GLsizei size, GLuint * buffer)
{
	CRContext *g = GetCurrentContext();
	CRSelectionState *se = &(g->selection);

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "SelectBuffer called in begin/end");
		return;
	}

	if (g->renderMode == GL_SELECT)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "SelectBuffer called with RenderMode = GL_SELECT");
		return;
	}

	FLUSH();

	se->buffer = buffer;
	se->bufferSize = size;
	se->bufferCount = 0;
	se->hitFlag = GL_FALSE;
	se->hitMinZ = 1.0;
	se->hitMaxZ = 0.0;
}


#define WRITE_RECORD( V )					\
	if (se->bufferCount < se->bufferSize) {	\
	   se->buffer[se->bufferCount] = (V);	\
	}							\
	se->bufferCount++;


static void
write_hit_record(CRSelectionState * se)
{
	GLuint i;
	GLuint zmin, zmax, zscale = (~0u);

	/* hitMinZ and hitMaxZ are in [0,1].  Multiply these values by */
	/* 2^32-1 and round to nearest unsigned integer. */

	zmin = (GLuint) ((GLfloat) zscale * se->hitMinZ);
	zmax = (GLuint) ((GLfloat) zscale * se->hitMaxZ);

	WRITE_RECORD(se->nameStackDepth);
	WRITE_RECORD(zmin);
	WRITE_RECORD(zmax);
	for (i = 0; i < se->nameStackDepth; i++)
	{
		WRITE_RECORD(se->nameStack[i]);
	}

	se->hits++;
	se->hitFlag = GL_FALSE;
	se->hitMinZ = 1.0;
	se->hitMaxZ = -1.0;
}



void STATE_APIENTRY
crStateInitNames(void)
{
	CRContext *g = GetCurrentContext();
	CRSelectionState *se = &(g->selection);

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "InitNames called in begin/end");
		return;
	}

	FLUSH();

	/* Record the hit before the hitFlag is wiped out again. */
	if (g->renderMode == GL_SELECT)
	{
		if (se->hitFlag)
		{
			write_hit_record(se);
		}
	}

	se->nameStackDepth = 0;
	se->hitFlag = GL_FALSE;
	se->hitMinZ = 1.0;
	se->hitMaxZ = 0.0;
}


void STATE_APIENTRY
crStateLoadName(GLuint name)
{
	CRContext *g = GetCurrentContext();
	CRSelectionState *se = &(g->selection);

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "LoadName called in begin/end");
		return;
	}


	if (g->renderMode != GL_SELECT)
	{
		return;
	}

	if (se->nameStackDepth == 0)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "nameStackDepth = 0");
		return;
	}

	FLUSH();

	if (se->hitFlag)
	{
		write_hit_record(se);
	}
	if (se->nameStackDepth < MAX_NAME_STACK_DEPTH)
	{
		se->nameStack[se->nameStackDepth - 1] = name;
	}
	else
	{
		se->nameStack[MAX_NAME_STACK_DEPTH - 1] = name;
	}
}


void STATE_APIENTRY
crStatePushName(GLuint name)
{
	CRContext *g = GetCurrentContext();
	CRSelectionState *se = &(g->selection);

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "PushName called in begin/end");
		return;
	}


	if (g->renderMode != GL_SELECT)
	{
		return;
	}

	FLUSH();

	if (se->hitFlag)
	{
		write_hit_record(se);
	}
	if (se->nameStackDepth >= MAX_NAME_STACK_DEPTH)
	{
		crStateError(__LINE__, __FILE__, GL_STACK_OVERFLOW,
								 "nameStackDepth overflow");
	}
	else
		se->nameStack[se->nameStackDepth++] = name;
}

void STATE_APIENTRY
crStatePopName(void)
{
	CRContext *g = GetCurrentContext();
	CRSelectionState *se = &(g->selection);

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "PopName called in begin/end");
		return;
	}

	if (g->renderMode != GL_SELECT)
	{
		return;
	}

	FLUSH();

	if (se->hitFlag)
	{
		write_hit_record(se);
	}

	if (se->nameStackDepth == 0)
	{
		crStateError(__LINE__, __FILE__, GL_STACK_UNDERFLOW,
								 "nameStackDepth underflow");
	}
	else
		se->nameStackDepth--;
}


static void
update_hitflag(GLfloat z)
{
	CRContext *g = GetCurrentContext();
	CRSelectionState *se = &(g->selection);

	se->hitFlag = GL_TRUE;

	if (z < se->hitMinZ)
		se->hitMinZ = z;

	if (z > se->hitMaxZ)
		se->hitMaxZ = z;
}


static void
select_rasterpos(void)
{
	CRContext *g = GetCurrentContext();
   
	if (g->current.rasterValid)
		update_hitflag(g->current.rasterAttrib[VERT_ATTRIB_POS][2]);
}

static void
select_point(const CRVertex *v)
{
	CRContext *g = GetCurrentContext();
	if (clip_point(v) == 0)
	{
		CRVertex c = *v;
		MAP_POINT(c.winPos, c.clipPos, g->viewport);
		update_hitflag(c.winPos.z);
	}
}


static void
select_line(const CRVertex *v0, const CRVertex *v1)
{
	CRContext *g = GetCurrentContext();
	CRVertex c0, c1;
	if (clip_line(v0, v1, &c0, &c1))
	{
		MAP_POINT(c0.winPos, c0.clipPos, g->viewport);
		MAP_POINT(c1.winPos, c1.clipPos, g->viewport);
		update_hitflag(c0.winPos.z);
		update_hitflag(c1.winPos.z);
	}
}


static void
select_triangle(const CRVertex *v0,
								const CRVertex *v1,
								const CRVertex *v2)
{
	CRContext *g = GetCurrentContext();
	CRVertex vlist[3], vclipped[8];
	GLuint i, n;

	vlist[0] = *v0;
	vlist[1] = *v1;
	vlist[2] = *v2;
	n = clip_polygon(vlist, 3, vclipped);
	for (i = 0; i < n; i++) {
		MAP_POINT(vclipped[i].winPos, vclipped[i].clipPos, g->viewport);
		update_hitflag(vclipped[i].winPos.z);
	}
}


void STATE_APIENTRY
crStateSelectVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	CRContext *g = GetCurrentContext();
	CRTransformState *t = &(g->transform);
	CRVertex *v = g->vBuffer + g->vCount;

	/* store the vertex */
	v->attrib[VERT_ATTRIB_POS][0] = x;
	v->attrib[VERT_ATTRIB_POS][1] = y;
	v->attrib[VERT_ATTRIB_POS][2] = z;
	v->attrib[VERT_ATTRIB_POS][3] = w;
	COPY_4V(v->attrib[VERT_ATTRIB_COLOR0] , g->current.vertexAttrib[VERT_ATTRIB_COLOR0]);  /* XXX need to apply lighting */
	v->colorIndex = g->current.colorIndex;  /* XXX need to apply lighting */
	/* Don't do this, we're capturing TexCoord ourselves as 
	 * we don't have a pincher like the tilesortSPU */
	/* v->texCoord[0] = g->current.texCoord[0]; */

	/* transform to eye space, then clip space */
	TRANSFORM_POINTA(v->eyePos, *(t->modelViewStack.top), v->attrib[VERT_ATTRIB_POS]);
	TRANSFORM_POINT(v->clipPos, *(t->projectionStack.top), v->eyePos);

	switch (g->current.mode) {
	case GL_POINTS:
		CRASSERT(g->vCount == 0);
		select_point(v);
		break;
	case GL_LINES:
		if (g->vCount == 0)
		{
			g->vCount = 1;
		}
		else
		{
			CRASSERT(g->vCount == 1);
			select_line(g->vBuffer + 0, g->vBuffer + 1);
			g->vCount = 0;
		}
		break;
	case GL_LINE_STRIP:
		if (g->vCount == 0)
		{
			g->vCount = 1;
		}
		else
		{
			CRASSERT(g->vCount == 1);
			select_line(g->vBuffer + 0, g->vBuffer + 1);
			g->vBuffer[0] = g->vBuffer[1];
			/* leave g->vCount at 1 */
		}
		break;
	case GL_LINE_LOOP:
		if (g->vCount == 0)
		{
			g->vCount = 1;
			g->lineLoop = GL_FALSE;
		}
		else if (g->vCount == 1)
		{
			select_line(g->vBuffer + 0, g->vBuffer + 1);
			g->lineLoop = GL_TRUE;
			g->vCount = 2;
		}
		else
		{
			CRASSERT(g->vCount == 2);
			g->lineLoop = GL_FALSE;
			select_line(g->vBuffer + 1, g->vBuffer + 2);
			g->vBuffer[1] = g->vBuffer[2];
			/* leave g->vCount at 2 */
		}
		break;
	case GL_TRIANGLES:
		if (g->vCount == 0 || g->vCount == 1)
		{
			g->vCount++;
		}
		else
		{
			CRASSERT(g->vCount == 2);
			select_triangle(g->vBuffer + 0, g->vBuffer + 1, g->vBuffer + 2);
			g->vCount = 0;
		}
		break;
	case GL_TRIANGLE_STRIP:
		if (g->vCount == 0 || g->vCount == 1)
		{
			g->vCount++;
		}
		else if (g->vCount == 2)
		{
			select_triangle(g->vBuffer + 0, g->vBuffer + 1, g->vBuffer + 2);
			g->vCount = 3;
		}
		else
		{
			CRASSERT(g->vCount == 3);
			select_triangle(g->vBuffer + 1, g->vBuffer + 3, g->vBuffer + 2);
			g->vBuffer[0] = g->vBuffer[2];
			g->vBuffer[1] = g->vBuffer[3];
			g->vCount = 2;
		}
		break;		
	case GL_TRIANGLE_FAN:
		if (g->vCount == 0 || g->vCount == 1)
		{
			g->vCount++;
		}
		else
		{
			CRASSERT(g->vCount == 2);
			select_triangle(g->vBuffer + 0, g->vBuffer + 1, g->vBuffer + 2);
			g->vBuffer[1] = g->vBuffer[2];
			/* leave g->vCount = 2 */
		}
		break;
	case GL_QUADS:
		if (g->vCount < 3)
		{
			g->vCount++;
		}
		else
		{
			CRASSERT(g->vCount == 3);
			select_triangle(g->vBuffer + 0, g->vBuffer + 1, g->vBuffer + 2);
			select_triangle(g->vBuffer + 0, g->vBuffer + 2, g->vBuffer + 3);
			g->vCount = 0;
		}
		break;		
	case GL_QUAD_STRIP:
		if (g->vCount < 3)
		{
			g->vCount++;
		}
		else
		{
			CRASSERT(g->vCount == 3);
			select_triangle(g->vBuffer + 0, g->vBuffer + 1, g->vBuffer + 2);
			select_triangle(g->vBuffer + 1, g->vBuffer + 3, g->vBuffer + 2);
			g->vBuffer[0] = g->vBuffer[2];
			g->vBuffer[1] = g->vBuffer[3];
			g->vCount = 2;
		}
		break;		
	case GL_POLYGON:
		/* draw as a tri-fan */
		if (g->vCount == 0 || g->vCount == 1)
		{
			g->vCount++;
		}
		else
		{
			CRASSERT(g->vCount == 2);
			select_triangle(g->vBuffer + 0, g->vBuffer + 1, g->vBuffer + 2);
			g->vBuffer[1] = g->vBuffer[2];
			/* leave g->vCount = 2 */
		}
		break;
	default:
		; /* impossible */
	}
}

void STATE_APIENTRY
crStateSelectRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	crStateRasterPos4f( x, y, z, w );

	select_rasterpos();
}

void STATE_APIENTRY
crStateSelectBegin(GLenum mode)
{
	CRContext *g = GetCurrentContext();

	crStateBegin(mode);

	g->vCount = 0;
	g->lineReset = GL_TRUE;
	g->lineLoop = GL_FALSE;
}


void STATE_APIENTRY
crStateSelectEnd(void)
{
	CRContext *g = GetCurrentContext();

	if (g->current.mode == GL_LINE_LOOP && g->vCount == 2)
	{
		/* draw the last line segment */
		select_line(g->vBuffer + 1, g->vBuffer + 0);
	}

	crStateEnd();
}

void STATE_APIENTRY
crStateSelectVertex2d(GLdouble x, GLdouble y)
{
	crStateSelectVertex4f((GLfloat) x, (GLfloat) y, 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectVertex2dv(const GLdouble * v)
{
	crStateSelectVertex4f((GLfloat) v[0], (GLfloat) v[1], 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectVertex2f(GLfloat x, GLfloat y)
{
	crStateSelectVertex4f(x, y, 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectVertex2fv(const GLfloat * v)
{
	crStateSelectVertex4f(v[0], v[1], 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectVertex2i(GLint x, GLint y)
{
	crStateSelectVertex4f((GLfloat) x, (GLfloat) y, 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectVertex2iv(const GLint * v)
{
	crStateSelectVertex4f((GLfloat) v[0], (GLfloat) v[1], 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectVertex2s(GLshort x, GLshort y)
{
	crStateSelectVertex4f((GLfloat) x, (GLfloat) y, 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectVertex2sv(const GLshort * v)
{
	crStateSelectVertex4f((GLfloat) v[0], (GLfloat) v[1], 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
	crStateSelectVertex4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0);
}

void STATE_APIENTRY
crStateSelectVertex3dv(const GLdouble * v)
{
	crStateSelectVertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0);
}

void STATE_APIENTRY
crStateSelectVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	crStateSelectVertex4f(x, y, z, 1.0);
}

void STATE_APIENTRY
crStateSelectVertex3fv(const GLfloat * v)
{
	crStateSelectVertex4f(v[0], v[1], v[2], 1.0);
}

void STATE_APIENTRY
crStateSelectVertex3i(GLint x, GLint y, GLint z)
{
	crStateSelectVertex4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0);
}

void STATE_APIENTRY
crStateSelectVertex3iv(const GLint * v)
{
	crStateSelectVertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0);
}

void STATE_APIENTRY
crStateSelectVertex3s(GLshort x, GLshort y, GLshort z)
{
	crStateSelectVertex4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0);
}

void STATE_APIENTRY
crStateSelectVertex3sv(const GLshort * v)
{
	crStateSelectVertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0);
}

void STATE_APIENTRY
crStateSelectVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	crStateSelectVertex4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void STATE_APIENTRY
crStateSelectVertex4dv(const GLdouble * v)
{
	crStateSelectVertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void STATE_APIENTRY
crStateSelectVertex4fv(const GLfloat * v)
{
	crStateSelectVertex4f(v[0], v[1], v[2], v[3]);
}

void STATE_APIENTRY
crStateSelectVertex4i(GLint x, GLint y, GLint z, GLint w)
{
	crStateSelectVertex4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void STATE_APIENTRY
crStateSelectVertex4iv(const GLint * v)
{
	crStateSelectVertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void STATE_APIENTRY
crStateSelectVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
	crStateSelectVertex4f(x, y, z, w);
}

void STATE_APIENTRY
crStateSelectVertex4sv(const GLshort * v)
{
	crStateSelectVertex4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void STATE_APIENTRY
crStateSelectRasterPos2d(GLdouble x, GLdouble y)
{
	crStateSelectRasterPos4f((GLfloat) x, (GLfloat) y, 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos2dv(const GLdouble * v)
{
	crStateSelectRasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos2f(GLfloat x, GLfloat y)
{
	crStateSelectRasterPos4f(x, y, 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos2fv(const GLfloat * v)
{
	crStateSelectRasterPos4f(v[0], v[1], 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos2i(GLint x, GLint y)
{
	crStateSelectRasterPos4f((GLfloat) x, (GLfloat) y, 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos2iv(const GLint * v)
{
	crStateSelectRasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos2s(GLshort x, GLshort y)
{
	crStateSelectRasterPos4f((GLfloat) x, (GLfloat) y, 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos2sv(const GLshort * v)
{
	crStateSelectRasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0, 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
	crStateSelectRasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos3dv(const GLdouble * v)
{
	crStateSelectRasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] , 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
	crStateSelectRasterPos4f(x, y, z, 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos3fv(const GLfloat * v)
{
	crStateSelectRasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] , 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos3i(GLint x, GLint y, GLint z)
{
	crStateSelectRasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos3iv(const GLint * v)
{
	crStateSelectRasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] , 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos3s(GLshort x, GLshort y, GLshort z)
{
	crStateSelectRasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos3sv(const GLshort * v)
{
	crStateSelectRasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] , 1.0);
}

void STATE_APIENTRY
crStateSelectRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
	crStateSelectRasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void STATE_APIENTRY
crStateSelectRasterPos4dv(const GLdouble * v)
{
	crStateSelectRasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] , (GLfloat) v[3]);
}

void STATE_APIENTRY
crStateSelectRasterPos4fv(const GLfloat * v)
{
	crStateSelectRasterPos4f(v[0], v[1], v[2], v[3]);
}

void STATE_APIENTRY
crStateSelectRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
	crStateSelectRasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void STATE_APIENTRY
crStateSelectRasterPos4iv(const GLint * v)
{
	crStateSelectRasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

void STATE_APIENTRY
crStateSelectRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
	crStateSelectRasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void STATE_APIENTRY
crStateSelectRasterPos4sv(const GLshort * v)
{
	crStateSelectRasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] , (GLfloat) v[3]);
}

void STATE_APIENTRY
crStateSelectRectf(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1)
{
   crStateSelectBegin(GL_QUADS);
   crStateSelectVertex2f(x0, y0);
   crStateSelectVertex2f(x0, y1);
   crStateSelectVertex2f(x1, y1);
   crStateSelectVertex2f(x1, y0);
   crStateSelectEnd();
}

void STATE_APIENTRY
crStateSelectRecti(GLint x0, GLint y0, GLint x1, GLint y1)
{
   crStateSelectRectf((GLfloat) x0, (GLfloat) y0, (GLfloat) x1, (GLfloat) y1);
}

void STATE_APIENTRY
crStateSelectRectd(GLdouble x0, GLdouble y0, GLdouble x1, GLdouble y1)
{
   crStateSelectRectf((GLfloat) x0, (GLfloat) y0, (GLfloat) x1, (GLfloat) y1);
}

void STATE_APIENTRY
crStateSelectRects(GLshort x0, GLshort y0, GLshort x1, GLshort y1)
{
   crStateSelectRectf((GLfloat) x0, (GLfloat) y0, (GLfloat) x1, (GLfloat) y1);
}

void STATE_APIENTRY
crStateSelectRectiv(const GLint *v0, const GLint *v1)
{
   crStateSelectRectf((GLfloat) v0[0], (GLfloat) v0[1], (GLfloat) v1[0], (GLfloat) v1[1]);
}

void STATE_APIENTRY
crStateSelectRectfv(const GLfloat *v0, const GLfloat *v1)
{
   crStateSelectRectf(v0[0], v0[1], v1[0], v1[1]);
}

void STATE_APIENTRY
crStateSelectRectdv(const GLdouble *v0, const GLdouble *v1)
{
   crStateSelectRectf((GLfloat) v0[0], (GLfloat) v0[1], (GLfloat) v1[0], (GLfloat) v1[1]);
}

void STATE_APIENTRY
crStateSelectRectsv(const GLshort *v0, const GLshort *v1)
{
   crStateSelectRectf((GLfloat) v0[0], (GLfloat) v0[1], (GLfloat) v1[0], (GLfloat) v1[1]);
}


GLint STATE_APIENTRY
crStateRenderMode(GLenum mode)
{
	CRContext *g = GetCurrentContext();
	CRFeedbackState *f = &(g->feedback);
	CRSelectionState *se = &(g->selection);
	GLint result;

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
								 "RenderMode called in begin/end");
		return 0;
	}

	FLUSH();

	switch (g->renderMode)
	{
	case GL_RENDER:
		result = 0;
		break;
	case GL_SELECT:
		if (se->hitFlag)
		{
			write_hit_record(se);
		}

		if (se->bufferCount > se->bufferSize)
		{
			/* overflow */
			result = -1;
		}
		else
		{
			result = se->hits;
		}
		se->bufferCount = 0;
		se->hits = 0;
		se->nameStackDepth = 0;
		break;
	case GL_FEEDBACK:
		if (f->count > f->bufferSize)
		{
			/* overflow */
			result = -1;
		}
		else
		{
			result = f->count;
		}
		f->count = 0;
		break;
	default:
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "invalid rendermode");
		return 0;
	}

	switch (mode)
	{
	case GL_RENDER:
		break;
	case GL_SELECT:
		if (se->bufferSize == 0)
		{
			/* haven't called glSelectBuffer yet */
			crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
									 "buffersize = 0");
		}
		break;
	case GL_FEEDBACK:
		if (f->bufferSize == 0)
		{
			/* haven't called glFeedbackBuffer yet */
			crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
									 "buffersize = 0");
		}
		break;
	default:
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "invalid rendermode");
		return 0;
	}

	g->renderMode = mode;

	return result;
}


void STATE_APIENTRY
crStateSelectDrawPixels(GLsizei width, GLsizei height, GLenum format,
												GLenum type, const GLvoid * pixels)
{
	(void) width;
	(void) height;
	(void) format;
	(void) type;
	(void) pixels;
	select_rasterpos();
}

void STATE_APIENTRY
crStateSelectCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height,
												GLenum type)
{
	(void) x;
	(void) y;
	(void) width;
	(void) height;
	(void) type;
	select_rasterpos();
}

void STATE_APIENTRY
crStateSelectBitmap(GLsizei width, GLsizei height, GLfloat xorig,
										GLfloat yorig, GLfloat xmove, GLfloat ymove,
										const GLubyte * bitmap)
{
	CRContext *g = GetCurrentContext();
	(void) width;
	(void) height;
	(void) xorig;
	(void) yorig;
	(void) bitmap;

	select_rasterpos();
	if (g->current.rasterValid)
	{
		g->current.rasterAttrib[VERT_ATTRIB_POS][0] += xmove;
		g->current.rasterAttrib[VERT_ATTRIB_POS][1] += ymove;
	}
}

