/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_BUFFER_H
#define CR_STATE_BUFFER_H

#include "state/cr_statetypes.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	CRbitvalue	dirty[CR_MAX_BITARRAY];
	CRbitvalue	enable[CR_MAX_BITARRAY];
	CRbitvalue	alphaFunc[CR_MAX_BITARRAY];
	CRbitvalue	depthFunc[CR_MAX_BITARRAY];
	CRbitvalue	blendFunc[CR_MAX_BITARRAY];
	CRbitvalue	logicOp[CR_MAX_BITARRAY];
	CRbitvalue	indexLogicOp[CR_MAX_BITARRAY];
	CRbitvalue	drawBuffer[CR_MAX_BITARRAY];
	CRbitvalue	readBuffer[CR_MAX_BITARRAY];
	CRbitvalue	indexMask[CR_MAX_BITARRAY];
	CRbitvalue	colorWriteMask[CR_MAX_BITARRAY];
	CRbitvalue	clearColor[CR_MAX_BITARRAY];
	CRbitvalue	clearIndex[CR_MAX_BITARRAY];
	CRbitvalue	clearDepth[CR_MAX_BITARRAY];
	CRbitvalue	clearAccum[CR_MAX_BITARRAY];
	CRbitvalue	depthMask[CR_MAX_BITARRAY];
#ifdef CR_EXT_blend_color
	CRbitvalue	blendColor[CR_MAX_BITARRAY];
#endif
#if defined(CR_EXT_blend_minmax) || defined(CR_EXT_blend_subtract) || defined(CR_EXT_blend_logic_op)
	CRbitvalue	blendEquation[CR_MAX_BITARRAY];
#endif
#if defined(CR_EXT_blend_func_separate)
	CRbitvalue	blendFuncSeparate[CR_MAX_BITARRAY];
#endif
} CRBufferBits;

typedef struct {
	GLboolean	depthTest;
	GLboolean	blend;
	GLboolean	alphaTest;
	GLboolean	logicOp;
	GLboolean	indexLogicOp;
	GLboolean	dither;
	GLboolean	depthMask;

	GLenum		alphaTestFunc;
	GLfloat		alphaTestRef;
	GLenum		depthFunc;
	GLenum		blendSrcRGB;
	GLenum		blendDstRGB;
	GLenum		blendSrcA;
	GLenum		blendDstA;
	GLenum		logicOpMode;
	GLenum		drawBuffer;
	GLenum		readBuffer;
	GLint		indexWriteMask;
	GLcolorb	colorWriteMask;
	GLcolorf	colorClearValue;
	GLfloat 	indexClearValue;
	GLdefault	depthClearValue;
	GLcolorf	accumClearValue;
#ifdef CR_EXT_blend_color
	GLcolorf	blendColor;
#endif
#if defined(CR_EXT_blend_minmax) || defined(CR_EXT_blend_subtract)
	GLenum		blendEquation;
#endif

    GLint       width, height;
    GLint       storedWidth, storedHeight;
    GLvoid      *pFrontImg;
    GLvoid      *pBackImg;
} CRBufferState;

DECLEXPORT(void) crStateBufferInit(CRContext *ctx);

DECLEXPORT(void) crStateBufferDiff(CRBufferBits *bb, CRbitvalue *bitID,
                                   CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStateBufferSwitch(CRBufferBits *bb, CRbitvalue *bitID, 
                                     CRContext *fromCtx, CRContext *toCtx);

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_BUFFER_H */
