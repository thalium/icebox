/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_MULTISAMPLE_H
#define CR_STATE_MULTISAMPLE_H

#include "state/cr_statetypes.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	CRbitvalue	dirty[CR_MAX_BITARRAY];
	CRbitvalue	enable[CR_MAX_BITARRAY];
	CRbitvalue  sampleAlphaToCoverage[CR_MAX_BITARRAY];
	CRbitvalue  sampleAlphaToOne[CR_MAX_BITARRAY];
	CRbitvalue  sampleCoverage[CR_MAX_BITARRAY];
	CRbitvalue  sampleCoverageValue[CR_MAX_BITARRAY]; /* and invert */
} CRMultisampleBits;

typedef struct {
	GLboolean enabled;
	GLboolean sampleAlphaToCoverage;
	GLboolean sampleAlphaToOne;
	GLboolean sampleCoverage;
	GLfloat sampleCoverageValue;
	GLboolean sampleCoverageInvert;
} CRMultisampleState;

DECLEXPORT(void) crStateMultisampleInit(CRContext *ctx);

DECLEXPORT(void) crStateMultisampleDiff(CRMultisampleBits *bb, CRbitvalue *bitID,
                                        CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStateMultisampleSwitch(CRMultisampleBits *bb, CRbitvalue *bitID, 
                                          CRContext *fromCtx, CRContext *toCtx);

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_MULTISAMPLE_H */
