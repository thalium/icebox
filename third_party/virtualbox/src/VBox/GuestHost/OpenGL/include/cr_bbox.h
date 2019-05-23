/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_BBOX_H
#define CR_BBOX_H

#include "state/cr_statetypes.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

extern DECLEXPORT(void)
crTransformBBox(float xmin, float ymin, float zmin,
								float xmax, float ymax, float zmax,
								const CRmatrix *m,
								float *out_xmin, float *out_ymin, float *out_zmin,
								float *out_xmax, float *out_ymax, float *out_zmax);

extern DECLEXPORT(void)
crProjectBBox(const GLfloat modl[16], const GLfloat proj[16], 
							GLfloat *x1, GLfloat *y1, GLfloat *z1,
							GLfloat *x2, GLfloat *y2, GLfloat *z2);


extern DECLEXPORT(void)
crRectiUnion(CRrecti *result, const CRrecti *a, const CRrecti *b);


#ifdef __cplusplus
}
#endif

#endif /* CR_BBOX_H */
