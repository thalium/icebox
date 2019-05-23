/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_REGCOMBINER_H
#define CR_STATE_REGCOMBINER_H

#include "state/cr_statetypes.h"
#include "state/cr_limits.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	GLenum a,        b,        c,        d;
	GLenum aMapping, bMapping, cMapping, dMapping;
	GLenum aPortion, bPortion, cPortion, dPortion;
	GLenum scale, bias;
	GLenum    abOutput,     cdOutput,     sumOutput;
	GLboolean abDotProduct, cdDotProduct, muxSum;
} CRRegCombinerPortionState;

typedef struct {
	GLboolean enabledRegCombiners;
	GLboolean enabledPerStageConstants;

	GLcolorf constantColor0;
	GLcolorf constantColor1;
	GLcolorf stageConstantColor0[CR_MAX_GENERAL_COMBINERS];
	GLcolorf stageConstantColor1[CR_MAX_GENERAL_COMBINERS];
	GLboolean colorSumClamp;
	GLint numGeneralCombiners;

	CRRegCombinerPortionState rgb[CR_MAX_GENERAL_COMBINERS];
	CRRegCombinerPortionState alpha[CR_MAX_GENERAL_COMBINERS];

	GLenum a,        b,        c,        d,        e,        f,        g;
	GLenum aMapping, bMapping, cMapping, dMapping, eMapping, fMapping, gMapping;
	GLenum aPortion, bPortion, cPortion, dPortion, ePortion, fPortion, gPortion;
} CRRegCombinerState;

typedef struct {
	CRbitvalue dirty[CR_MAX_BITARRAY];
	CRbitvalue enable[CR_MAX_BITARRAY];
	CRbitvalue regCombinerVars[CR_MAX_BITARRAY]; /* numGeneralCombiners, colorSumClamp */
	CRbitvalue regCombinerColor0[CR_MAX_BITARRAY];
	CRbitvalue regCombinerColor1[CR_MAX_BITARRAY];
	CRbitvalue regCombinerStageColor0[CR_MAX_GENERAL_COMBINERS][CR_MAX_BITARRAY];
	CRbitvalue regCombinerStageColor1[CR_MAX_GENERAL_COMBINERS][CR_MAX_BITARRAY];
	CRbitvalue regCombinerInput[CR_MAX_GENERAL_COMBINERS][CR_MAX_BITARRAY]; /* rgb/alpha[].a/b/c/d, .aMapping, .aPortion */
	CRbitvalue regCombinerOutput[CR_MAX_GENERAL_COMBINERS][CR_MAX_BITARRAY]; /* rgb/alpha[].abOutput, .cdOutput, .sumOutput, .scale, .bias, .abDotProduct, .cdDotProduct, .muxSum */
	CRbitvalue regCombinerFinalInput[CR_MAX_BITARRAY]; /* a/b/c/d/e/f/g, aMapping, aPortion */
} CRRegCombinerBits;

DECLEXPORT(void) crStateRegCombinerInit( CRContext *ctx );

DECLEXPORT(void) crStateRegCombinerDiff(CRRegCombinerBits *b, CRbitvalue *bitID,
                                        CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStateRegCombinerSwitch( CRRegCombinerBits *b, CRbitvalue *bitID, 
                                           CRContext *fromCtx, CRContext *toCtx );

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_REGCOMBINER_H */
