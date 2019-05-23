/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_LINE_H
#define CR_STATE_LINE_H

#include "state/cr_statetypes.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	CRbitvalue enable[CR_MAX_BITARRAY];
	CRbitvalue width[CR_MAX_BITARRAY];
	CRbitvalue stipple[CR_MAX_BITARRAY];
	CRbitvalue dirty[CR_MAX_BITARRAY];
} CRLineBits;

typedef struct {
	GLboolean	lineSmooth;
	GLboolean	lineStipple;
	GLfloat		width;
	GLushort	pattern;
	GLint		repeat;
} CRLineState;

DECLEXPORT(void) crStateLineInit (CRContext *ctx);

DECLEXPORT(void) crStateLineDiff(CRLineBits *bb, CRbitvalue *bitID,
                                 CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStateLineSwitch(CRLineBits *bb, CRbitvalue *bitID, 
                                   CRContext *fromCtx, CRContext *toCtx);

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_LINE_H */
