/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <float.h>
#include "cr_bbox.h"

/**
 * \mainpage Util 
 *
 * \section UtilIntroduction Introduction
 *
 * Chromium consists of all the top-level files in the cr
 * directory.  The util module basically takes care of API dispatch,
 * and OpenGL state management.
 *
 */
static float _vmult(const float *m, float x, float y, float z) 
{
	return m[0]*x + m[4]*y + m[8]*z + m[12];
}

void
crTransformBBox( float xmin, float ymin, float zmin,
                 float xmax, float ymax, float zmax,
                 const CRmatrix *m,
                 float *out_xmin, float *out_ymin, float *out_zmin,
                 float *out_xmax, float *out_ymax, float *out_zmax )
{
	float x[8], y[8], z[8], w[8];
	int i,j;
    
	/*  Here is the arrangement of the bounding box
	 *  
	 *           0 --- 1
	 *           |\    .\
	 *           | 2 --- 3 
	 *           | |   . |
	 *           | |   . |
	 *           4.|...5 |
	 *            \|    .|
	 *             6 --- 7
	 *  
	 *  c array contains the edge connectivity list
	 */

	static const int c[8][3] = {	
		{1, 2, 4}, 
		{0, 3, 5}, 
		{0, 3, 6}, 
		{1, 2, 7},
		{0, 5, 6}, 
		{1, 4, 7}, 
		{2, 4, 7}, 
		{3, 5, 6} 
	};

	x[0] = _vmult(&(m->m00), xmin, ymin, zmin);
	x[1] = _vmult(&(m->m00), xmax, ymin, zmin);
	x[2] = _vmult(&(m->m00), xmin, ymax, zmin);
	x[3] = _vmult(&(m->m00), xmax, ymax, zmin);
	x[4] = _vmult(&(m->m00), xmin, ymin, zmax);
	x[5] = _vmult(&(m->m00), xmax, ymin, zmax);
	x[6] = _vmult(&(m->m00), xmin, ymax, zmax);
	x[7] = _vmult(&(m->m00), xmax, ymax, zmax);

	y[0] = _vmult(&(m->m01), xmin, ymin, zmin);
	y[1] = _vmult(&(m->m01), xmax, ymin, zmin);
	y[2] = _vmult(&(m->m01), xmin, ymax, zmin);
	y[3] = _vmult(&(m->m01), xmax, ymax, zmin);
	y[4] = _vmult(&(m->m01), xmin, ymin, zmax);
	y[5] = _vmult(&(m->m01), xmax, ymin, zmax);
	y[6] = _vmult(&(m->m01), xmin, ymax, zmax);
	y[7] = _vmult(&(m->m01), xmax, ymax, zmax);

	z[0] = _vmult(&(m->m02), xmin, ymin, zmin);
	z[1] = _vmult(&(m->m02), xmax, ymin, zmin);
	z[2] = _vmult(&(m->m02), xmin, ymax, zmin);
	z[3] = _vmult(&(m->m02), xmax, ymax, zmin);
	z[4] = _vmult(&(m->m02), xmin, ymin, zmax);
	z[5] = _vmult(&(m->m02), xmax, ymin, zmax);
	z[6] = _vmult(&(m->m02), xmin, ymax, zmax);
	z[7] = _vmult(&(m->m02), xmax, ymax, zmax);

	w[0] = _vmult(&(m->m03), xmin, ymin, zmin);
	w[1] = _vmult(&(m->m03), xmax, ymin, zmin);
	w[2] = _vmult(&(m->m03), xmin, ymax, zmin);
	w[3] = _vmult(&(m->m03), xmax, ymax, zmin);
	w[4] = _vmult(&(m->m03), xmin, ymin, zmax);
	w[5] = _vmult(&(m->m03), xmax, ymin, zmax);
	w[6] = _vmult(&(m->m03), xmin, ymax, zmax);
	w[7] = _vmult(&(m->m03), xmax, ymax, zmax);

	/* Now, the object-space bbox has been transformed into 
	 * clip-space. */

	/* Find the 2D bounding box of the 3D bounding box */
	xmin = ymin = zmin = FLT_MAX;
	xmax = ymax = zmax = -FLT_MAX;

	for (i=0; i<8; i++) 
	{
		float xp = x[i];
		float yp = y[i];
		float zp = z[i];
		float wp = w[i];

		/* If corner is to be clipped... */
		if (zp < -wp) 
		{

			/* Point has three edges */
			for (j=0; j<3; j++) 
			{
				/* Handle the clipping... */
				int k = c[i][j];
				float xk = x[k];
				float yk = y[k];
				float zk = z[k];
				float wk = w[k];
				float t;

				if (zp+wp-zk-wk == 0.0)
					continue; /* avoid divide by zero */
				else 
					t = (wp + zp) / (zp+wp-zk-wk);

				if (t < 0.0f || t > 1.0f)
				{
					continue;
				}
				wp = wp + (wk-wp) * t;
				xp = xp + (xk-xp) * t;
				yp = yp + (yk-yp) * t;
				zp = -wp;

				xp /= wp;
				yp /= wp;
				zp /= wp;

				if (xp < xmin) xmin = xp;
				if (xp > xmax) xmax = xp;
				if (yp < ymin) ymin = yp;
				if (yp > ymax) ymax = yp;
				if (zp < zmin) zmin = zp;
				if (zp > zmax) zmax = zp;
			}
		} 
		else 
		{
			/* corner was not clipped.. */
			xp /= wp;
			yp /= wp;
			zp /= wp;
			if (xp < xmin) xmin = xp;
			if (xp > xmax) xmax = xp;
			if (yp < ymin) ymin = yp;
			if (yp > ymax) ymax = yp;
			if (zp < zmin) zmin = zp;
			if (zp > zmax) zmax = zp;
		}
	}

	/* Copy for export */
	if (out_xmin) *out_xmin = xmin;
	if (out_ymin) *out_ymin = ymin;
	if (out_zmin) *out_zmin = zmin;
	if (out_xmax) *out_xmax = xmax;
	if (out_ymax) *out_ymax = ymax;
	if (out_zmax) *out_zmax = zmax;
}

/**
 * Given the coordinates of the two opposite corners of a bounding box
 * in object space (x1,y1,z1) and (x2,y2,z2), use the given modelview
 * and projection matrices to transform the coordinates to NDC space.
 * Find the 3D bounding bounding box of those eight coordinates and
 * return the min/max in (x1,y1,x1) and (x2,y2,z2).
 */
void
crProjectBBox(const GLfloat modl[16], const GLfloat proj[16], 
              GLfloat *x1, GLfloat *y1, GLfloat *z1,
              GLfloat *x2, GLfloat *y2, GLfloat *z2) 
{
	CRmatrix m;
	
	/* compute product of modl and proj matrices */
	m.m00  = proj[0] * modl[0]  + proj[4] * modl[1]  + proj[8]  * modl[2]  + proj[12] * modl[3];	
	m.m01  = proj[1] * modl[0]  + proj[5] * modl[1]  + proj[9]  * modl[2]  + proj[13] * modl[3];	
	m.m02  = proj[2] * modl[0]  + proj[6] * modl[1]  + proj[10] * modl[2]  + proj[14] * modl[3];	
	m.m03  = proj[3] * modl[0]  + proj[7] * modl[1]  + proj[11] * modl[2]  + proj[15] * modl[3];	
	m.m10  = proj[0] * modl[4]  + proj[4] * modl[5]  + proj[8]  * modl[6]  + proj[12] * modl[7];	
	m.m11  = proj[1] * modl[4]  + proj[5] * modl[5]  + proj[9]  * modl[6]  + proj[13] * modl[7];	
	m.m12  = proj[2] * modl[4]  + proj[6] * modl[5]  + proj[10] * modl[6]  + proj[14] * modl[7];	
	m.m13  = proj[3] * modl[4]  + proj[7] * modl[5]  + proj[11] * modl[6]  + proj[15] * modl[7];	
	m.m20  = proj[0] * modl[8]  + proj[4] * modl[9]  + proj[8]  * modl[10] + proj[12] * modl[11];	
	m.m21  = proj[1] * modl[8]  + proj[5] * modl[9]  + proj[9]  * modl[10] + proj[13] * modl[11];
	m.m22  = proj[2] * modl[8]  + proj[6] * modl[9]  + proj[10] * modl[10] + proj[14] * modl[11];	
	m.m23  = proj[3] * modl[8]  + proj[7] * modl[9]  + proj[11] * modl[10] + proj[15] * modl[11];	
	m.m30  = proj[0] * modl[12] + proj[4] * modl[13] + proj[8]  * modl[14] + proj[12] * modl[15];	
	m.m31  = proj[1] * modl[12] + proj[5] * modl[13] + proj[9]  * modl[14] + proj[13] * modl[15];	
	m.m32  = proj[2] * modl[12] + proj[6] * modl[13] + proj[10] * modl[14] + proj[14] * modl[15];	
	m.m33  = proj[3] * modl[12] + proj[7] * modl[13] + proj[11] * modl[14] + proj[15] * modl[15]; 
	
    crTransformBBox( *x1, *y1, *z1,
                     *x2, *y2, *z2,
                     &m,
                     x1, y1, z1,
                     x2, y2, z2 );
}


#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


/**
 * Compute union of a and b and put in result.
 */
void
crRectiUnion(CRrecti *result, const CRrecti *a, const CRrecti *b)
{
	result->x1 = MIN(a->x1, b->x1);
	result->x2 = MAX(a->x2, b->x2);
	result->y1 = MIN(a->y1, b->y1);
	result->y2 = MAX(a->y2, b->y2);
}
