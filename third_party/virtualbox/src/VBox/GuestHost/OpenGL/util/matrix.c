
#include <math.h>
#include <stdio.h>
#include "cr_matrix.h"
#include "cr_mem.h"

#ifndef M_PI
#define M_PI             3.14159265358979323846
#endif


static const CRmatrix identity_matrix = {
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0
};


/*
 * Initialize the given matrix to the identity.
 */
void
crMatrixInit(CRmatrix *m)
{
	*m = identity_matrix;
}


/*
 * Parse a string of 16 floats to initialize a matrix (row major order).
 * If there's a parsing error, initialize the matrix to the identity.
 */
void
crMatrixInitFromString(CRmatrix *m, const char *s)
{
	const char *fmt = "%f, %f, %f, %f,  %f, %f, %f, %f,  %f, %f, %f, %f,  %f, %f, %f, %f";
	const char *fmtb = "[ %f, %f, %f, %f,  %f, %f, %f, %f,  %f, %f, %f, %f,  %f, %f, %f, %f ]";
	int n = sscanf(s, (s[0] == '[' ? fmtb : fmt),
								 &m->m00, &m->m01, &m->m02, &m->m03,
								 &m->m10, &m->m11, &m->m12, &m->m13,
								 &m->m20, &m->m21, &m->m22, &m->m23,
								 &m->m30, &m->m31, &m->m32, &m->m33);
	if (n != 16) {
		/* insufficient parameters */
		crMatrixInit(m);
	}
}


/*
 * Initialize a matrix from an array of 16 values.
 */
void
crMatrixInitFromFloats(CRmatrix *m, const float *v)
{
	m->m00 = v[0];	
	m->m01 = v[1];		
	m->m02 = v[2];	
	m->m03 = v[3];	
	m->m10 = v[4];	
	m->m11 = v[5];		
	m->m12 = v[6];	
	m->m13 = v[7];	
	m->m20 = v[8];		
	m->m21 = v[9];	
	m->m22 = v[10];	
	m->m23 = v[11];	
	m->m30 = v[12];	
	m->m31 = v[13];	
	m->m32 = v[14];	
	m->m33 = v[15];	
}


void
crMatrixInitFromDoubles(CRmatrix *m, const double *v)
{
	m->m00 = (float) v[0];	
	m->m01 = (float) v[1];		
	m->m02 = (float) v[2];	
	m->m03 = (float) v[3];	
	m->m10 = (float) v[4];	
	m->m11 = (float) v[5];		
	m->m12 = (float) v[6];	
	m->m13 = (float) v[7];	
	m->m20 = (float) v[8];		
	m->m21 = (float) v[9];	
	m->m22 = (float) v[10];	
	m->m23 = (float) v[11];	
	m->m30 = (float) v[12];	
	m->m31 = (float) v[13];	
	m->m32 = (float) v[14];	
	m->m33 = (float) v[15];	
}


/* useful for debugging */
void
crMatrixPrint(const char *msg, const CRmatrix *m)
{
	printf("%s\n", msg);
	printf("  %f %f %f %f\n", m->m00, m->m10, m->m20, m->m30);
	printf("  %f %f %f %f\n", m->m01, m->m11, m->m21, m->m31);
	printf("  %f %f %f %f\n", m->m02, m->m12, m->m22, m->m32);
	printf("  %f %f %f %f\n", m->m03, m->m13, m->m23, m->m33);
}


void
crMatrixGetFloats(float *values, const CRmatrix *m)
{
	values[0] = m->m00;
	values[1] = m->m01;
	values[2] = m->m02;
	values[3] = m->m03;
	values[4] = m->m10;
	values[5] = m->m11;
	values[6] = m->m12;
	values[7] = m->m13;
	values[8] = m->m20;
	values[9] = m->m21;
	values[10] = m->m22;
	values[11] = m->m23;
	values[12] = m->m30;
	values[13] = m->m31;
	values[14] = m->m32;
	values[15] = m->m33;
}


/* Return 1 if the matrices are equal, return 0 otherwise.
 */
int
crMatrixIsEqual(const CRmatrix *m, const CRmatrix *n)
{
	return crMemcmp(m, n, sizeof(CRmatrix)) == 0;
}


/*
 * Test if matrix is identity
 */
int
crMatrixIsIdentity(const CRmatrix *m)
{
	return crMemcmp(m, &identity_matrix, sizeof(CRmatrix)) == 0;
}


/*
 * Test if matrix is orthographic projection matrix.
 */
int
crMatrixIsOrthographic(const CRmatrix *m)
{
	return m->m33 != 0.0;
}


void
crMatrixCopy(CRmatrix *dest, const CRmatrix *src)
{
	crMemcpy(dest, src, sizeof(CRmatrix));
}


/*
 * Compute p = a * b
 */
void
crMatrixMultiply(CRmatrix *p, const CRmatrix *a, const CRmatrix *b)
{
	CRmatrix t; /* temporary result, in case p = a or p = b */
	t.m00 = a->m00 * b->m00 + a->m10 * b->m01 + a->m20 * b->m02 + a->m30 * b->m03;	
	t.m01 = a->m01 * b->m00 + a->m11 * b->m01 + a->m21 * b->m02 + a->m31 * b->m03;	
	t.m02 = a->m02 * b->m00 + a->m12 * b->m01 + a->m22 * b->m02 + a->m32 * b->m03;	
	t.m03 = a->m03 * b->m00 + a->m13 * b->m01 + a->m23 * b->m02 + a->m33 * b->m03;	
	t.m10 = a->m00 * b->m10 + a->m10 * b->m11 + a->m20 * b->m12 + a->m30 * b->m13;	
	t.m11 = a->m01 * b->m10 + a->m11 * b->m11 + a->m21 * b->m12 + a->m31 * b->m13;	
	t.m12 = a->m02 * b->m10 + a->m12 * b->m11 + a->m22 * b->m12 + a->m32 * b->m13;	
	t.m13 = a->m03 * b->m10 + a->m13 * b->m11 + a->m23 * b->m12 + a->m33 * b->m13;	
	t.m20 = a->m00 * b->m20 + a->m10 * b->m21 + a->m20 * b->m22 + a->m30 * b->m23;	
	t.m21 = a->m01 * b->m20 + a->m11 * b->m21 + a->m21 * b->m22 + a->m31 * b->m23;	
	t.m22 = a->m02 * b->m20 + a->m12 * b->m21 + a->m22 * b->m22 + a->m32 * b->m23;	
	t.m23 = a->m03 * b->m20 + a->m13 * b->m21 + a->m23 * b->m22 + a->m33 * b->m23;	
	t.m30 = a->m00 * b->m30 + a->m10 * b->m31 + a->m20 * b->m32 + a->m30 * b->m33;	
	t.m31 = a->m01 * b->m30 + a->m11 * b->m31 + a->m21 * b->m32 + a->m31 * b->m33;	
	t.m32 = a->m02 * b->m30 + a->m12 * b->m31 + a->m22 * b->m32 + a->m32 * b->m33;	
	t.m33 = a->m03 * b->m30 + a->m13 * b->m31 + a->m23 * b->m32 + a->m33 * b->m33;
	*p = t;
}


void
crMatrixTransformPointf(const CRmatrix *m, GLvectorf *p) 
{
	float x = p->x;
	float y = p->y;
	float z = p->z;
	float w = p->w;

	p->x = m->m00*x + m->m10*y + m->m20*z + m->m30*w;
	p->y = m->m01*x + m->m11*y + m->m21*z + m->m31*w;
	p->z = m->m02*x + m->m12*y + m->m22*z + m->m32*w;
	p->w = m->m03*x + m->m13*y + m->m23*z + m->m33*w;
}


void
crMatrixTransformPointd(const CRmatrix *m, GLvectord *p)
{
	double x = p->x;
	double y = p->y;
	double z = p->z;
	double w = p->w;

	p->x = (double) (m->m00*x + m->m10*y + m->m20*z + m->m30*w);
	p->y = (double) (m->m01*x + m->m11*y + m->m21*z + m->m31*w);
	p->z = (double) (m->m02*x + m->m12*y + m->m22*z + m->m32*w);
	p->w = (double) (m->m03*x + m->m13*y + m->m23*z + m->m33*w);
}


void
crMatrixInvertTranspose(CRmatrix *inv, const CRmatrix *mat)
{
	/* Taken from Pomegranate code, trans.c.
	 * Note: We have our data structures reversed
	 */
	const float m00 = mat->m00;
	const float m01 = mat->m10;
	const float m02 = mat->m20;
	const float m03 = mat->m30;

	const float m10 = mat->m01;
	const float m11 = mat->m11;
	const float m12 = mat->m21;
	const float m13 = mat->m31;

	const float m20 = mat->m02;
	const float m21 = mat->m12;
	const float m22 = mat->m22;
	const float m23 = mat->m32;

	const float m30 = mat->m03;
	const float m31 = mat->m13;
	const float m32 = mat->m23;
	const float m33 = mat->m33;

#define det3x3(a1, a2, a3, b1, b2, b3, c1, c2, c3) \
	(a1 * (b2 * c3 - b3 * c2) + \
	 b1 * (c2 * a3 - a2 * c3) + \
	 c1 * (a2 * b3 - a3 * b2))

  const float cof00 =  det3x3( m11, m12, m13,
                             m21, m22, m23,
                             m31, m32, m33 );

  const float cof01 = -det3x3( m12, m13, m10,
                             m22, m23, m20,
                             m32, m33, m30 );

  const float cof02 = det3x3( m13, m10, m11,
                             m23, m20, m21,
                             m33, m30, m31 );

  const float cof03 = -det3x3( m10, m11, m12,
                             m20, m21, m22,
                             m30, m31, m32 );


  const float inv_det = 1.0f / ( m00 * cof00 + m01 * cof01 +
                                            m02 * cof02 + m03 * cof03 );


  const float cof10 = -det3x3( m21, m22, m23,
                             m31, m32, m33,
                             m01, m02, m03 );

  const float cof11 = det3x3( m22, m23, m20,
                             m32, m33, m30,
                             m02, m03, m00 );

  const float cof12 = -det3x3( m23, m20, m21,
                             m33, m30, m31,
                             m03, m00, m01 );

  const float cof13 = det3x3( m20, m21, m22,
                             m30, m31, m32,
                             m00, m01, m02 );



  const float cof20 = det3x3( m31, m32, m33,
                             m01, m02, m03,
                             m11, m12, m13 );

  const float cof21 = -det3x3( m32, m33, m30,
                             m02, m03, m00,
                             m12, m13, m10 );

  const float cof22 = det3x3( m33, m30, m31,
                             m03, m00, m01,
                             m13, m10, m11 );

  const float cof23 = -det3x3( m30, m31, m32,
                             m00, m01, m02,
                             m10, m11, m12 );


  const float cof30 = -det3x3( m01, m02, m03,
                             m11, m12, m13,
                             m21, m22, m23 );

  const float cof31 = det3x3( m02, m03, m00,
                             m12, m13, m10,
                             m22, m23, m20 );

  const float cof32 = -det3x3( m03, m00, m01,
                             m13, m10, m11,
                             m23, m20, m21 );

  const float cof33 = det3x3( m00, m01, m02,
                             m10, m11, m12,
                             m20, m21, m22 );

#undef det3x3

	/* Perform transpose in asignment */

	inv->m00 = cof00 * inv_det; inv->m10 = cof01 * inv_det;
	inv->m20 = cof02 * inv_det; inv->m30 = cof03 * inv_det;

	inv->m01 = cof10 * inv_det; inv->m11 = cof11 * inv_det;
	inv->m21 = cof12 * inv_det; inv->m31 = cof13 * inv_det;

	inv->m02 = cof20 * inv_det; inv->m12 = cof21 * inv_det;
	inv->m22 = cof22 * inv_det; inv->m32 = cof23 * inv_det;

	inv->m03 = cof30 * inv_det; inv->m13 = cof31 * inv_det;
	inv->m23 = cof32 * inv_det; inv->m33 = cof33 * inv_det;
}

void 
crMatrixTranspose(CRmatrix *t, const CRmatrix *m)
{
  CRmatrix c;

	c.m00 = m->m00;  c.m10 = m->m01;  c.m20 = m->m02;  c.m30 = m->m03;
	c.m01 = m->m10;  c.m11 = m->m11;  c.m21 = m->m12;  c.m31 = m->m13;
	c.m02 = m->m20;  c.m12 = m->m21;  c.m22 = m->m22;  c.m32 = m->m23;
	c.m03 = m->m30;  c.m13 = m->m31;  c.m23 = m->m32;  c.m33 = m->m33;

	*t = c;
}

/*
 * Apply a translation to the given matrix.
 */
void
crMatrixTranslate(CRmatrix *m, float x, float y, float z)
{
	m->m30 = m->m00 * x + m->m10 * y + m->m20 * z + m->m30;
	m->m31 = m->m01 * x + m->m11 * y + m->m21 * z + m->m31;
	m->m32 = m->m02 * x + m->m12 * y + m->m22 * z + m->m32;
	m->m33 = m->m03 * x + m->m13 * y + m->m23 * z + m->m33;
}


/*
 * Apply a rotation to the given matrix.
 */
void
crMatrixRotate(CRmatrix *m, float angle, float x, float y, float z) 
{
	const float c = (float) cos(angle * M_PI / 180.0f);
	const float one_minus_c = 1.0f - c;	
	const float s = (float) sin(angle * M_PI / 180.0f);
	const float v_len = (float) sqrt (x*x + y*y + z*z);	
	float x_one_minus_c;	
	float y_one_minus_c;	
	float z_one_minus_c;	
	CRmatrix rot;

	/* Begin/end Checking and flushing will be done by MultMatrix. */

	if (v_len == 0.0f)
		return;

	/* Normalize the vector */	
	if (v_len != 1.0f) {	
		x /= v_len;				
		y /= v_len;					
		z /= v_len;				
	}							
	/* compute some common values */	
	x_one_minus_c = x * one_minus_c;	
	y_one_minus_c = y * one_minus_c;	
	z_one_minus_c = z * one_minus_c;	
	/* Generate the terms of the rotation matrix	
	 ** from pg 325 OGL 1.1 Blue Book.				
	 */												
	rot.m00 = x * x_one_minus_c + c;					
	rot.m01 = x * y_one_minus_c + z * s;				
	rot.m02 = x * z_one_minus_c - y * s;				
	rot.m03 = 0.0f;						
	rot.m10 = y * x_one_minus_c - z * s;				
	rot.m11 = y * y_one_minus_c + c;					
	rot.m12 = y * z_one_minus_c + x * s;				
	rot.m13 = 0.0f;						
	rot.m20 = z * x_one_minus_c + y * s;				
	rot.m21 = z * y_one_minus_c - x * s;				
	rot.m22 = z * z_one_minus_c + c;						
	rot.m23 = 0.0f;						
	rot.m30 = 0.0f;						
	rot.m31 = 0.0f;						
	rot.m32 = 0.0f;						
	rot.m33 = 1.0f;						
	crMatrixMultiply(m, m, &rot);
}


/*
 * Apply a scale to the given matrix.
 */
void
crMatrixScale(CRmatrix *m, float x, float y, float z)
{
	m->m00 *= x;	
	m->m01 *= x;	
	m->m02 *= x;		
	m->m03 *= x;	
	m->m10 *= y;	
	m->m11 *= y;	
	m->m12 *= y;	
	m->m13 *= y;	
	m->m20 *= z;	
	m->m21 *= z;	
	m->m22 *= z;	
	m->m23 *= z;	
}


/*
 * Make a projection matrix from frustum parameters.
 */
void
crMatrixFrustum(CRmatrix *m,
								float left, float right,
								float bottom, float top, 
								float zNear, float zFar)
{
	CRmatrix f;

	f.m00 = (2.0f * zNear) / (right - left);
	f.m01 = 0.0;
	f.m02 = 0.0;
	f.m03 = 0.0;

	f.m10 = 0.0;
	f.m11 = (2.0f * zNear) / (top - bottom);
	f.m12 = 0.0;
	f.m13 = 0.0;

	f.m20 = (right + left) / (right - left);
	f.m21 = (top + bottom) / (top - bottom);
	f.m22 = (-zNear - zFar) / (zFar - zNear);
	f.m23 = -1.0;

	f.m30 = 0.0;
	f.m31 = 0.0;
	f.m32 = (2.0f * zFar * zNear) / (zNear - zFar);
	f.m33 = 0.0;

	crMatrixMultiply(m, m, &f);
}


void
crMatrixOrtho(CRmatrix *m,
							float left, float right,
							float bottom, float top,
							float znear, float zfar)
{
	CRmatrix ortho;

	ortho.m00 = 2.0f / (right - left);
	ortho.m01 = 0.0;
	ortho.m02 = 0.0;
	ortho.m03 = 0.0;

	ortho.m10 = 0.0;
	ortho.m11 = 2.0f / (top - bottom);
	ortho.m12 = 0.0;
	ortho.m13 = 0.0;

	ortho.m20 = 0.0;
	ortho.m21 = 0.0;
	ortho.m22 = -2.0f / (zfar - znear);
	ortho.m23 = 0.0;

	ortho.m30 = -(right + left) / (right - left);
	ortho.m31 = -(top + bottom) / (top - bottom);
	ortho.m32= -(zfar + znear) / (zfar - znear);
	ortho.m33 = 1.0;

	crMatrixMultiply(m, m, &ortho);
}

