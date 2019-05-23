/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_FEEDBACK_H 
#define CR_STATE_FEEDBACK_H 

#include "state/cr_statetypes.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NAME_STACK_DEPTH 64

typedef struct {
	CRbitvalue dirty[CR_MAX_BITARRAY];
} CRFeedbackBits;

typedef struct {
	CRbitvalue dirty[CR_MAX_BITARRAY];
} CRSelectionBits;

typedef struct {
	GLenum	type;
	GLuint	mask;
	GLfloat	*buffer;
	GLuint	bufferSize;
	GLuint	count;
} CRFeedbackState;

typedef struct {
   	GLuint *buffer;
   	GLuint bufferSize;
   	GLuint bufferCount;
   	GLuint hits;
   	GLuint nameStackDepth;
   	GLuint nameStack[MAX_NAME_STACK_DEPTH];
   	GLboolean hitFlag;
   	GLfloat hitMinZ, hitMaxZ;
} CRSelectionState;

extern DECLEXPORT(void) crStateFeedbackDiff(CRFeedbackState *from, CRFeedbackState *to,
                                            CRFeedbackBits *bb, CRbitvalue *bitID);
extern DECLEXPORT(void) crStateFeedbackSwitch(CRFeedbackBits *bb, CRbitvalue *bitID, 
                                              CRFeedbackState *from, CRFeedbackState *to);

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_FEEDBACK_H */
