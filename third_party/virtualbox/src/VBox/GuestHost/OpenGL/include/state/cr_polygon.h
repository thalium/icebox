/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_POLYGON_H
#define CR_STATE_POLYGON_H

#include "state/cr_statetypes.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	CRbitvalue enable[CR_MAX_BITARRAY];
	CRbitvalue offset[CR_MAX_BITARRAY];
	CRbitvalue mode[CR_MAX_BITARRAY];
	CRbitvalue stipple[CR_MAX_BITARRAY];
	CRbitvalue dirty[CR_MAX_BITARRAY];
} CRPolygonBits;

typedef struct {
	GLboolean	polygonSmooth;
	GLboolean polygonOffsetFill;
	GLboolean polygonOffsetLine;
	GLboolean polygonOffsetPoint;
	GLboolean	polygonStipple;
	GLboolean cullFace;
	GLfloat		offsetFactor;
	GLfloat		offsetUnits;
	GLenum		cullFaceMode;
	GLenum		frontFace;
	GLenum		frontMode;
	GLenum		backMode;
	GLint		  stipple[32];
} CRPolygonState;

DECLEXPORT(void) crStatePolygonInit(CRContext *ctx);

DECLEXPORT(void) crStatePolygonDiff(CRPolygonBits *bb, CRbitvalue *bitID,
                                    CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStatePolygonSwitch(CRPolygonBits *bb, CRbitvalue *bitID, 
                                      CRContext *fromCtx, CRContext *toCtx);

#ifdef __cplusplus
}
#endif

#endif
