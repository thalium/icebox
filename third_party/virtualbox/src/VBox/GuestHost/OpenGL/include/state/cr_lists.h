/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_LISTS_H
#define CR_STATE_LISTS_H

#include "cr_hash.h"
#include "state/cr_statetypes.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
	CRbitvalue dirty[CR_MAX_BITARRAY];
	CRbitvalue base[CR_MAX_BITARRAY];
} CRListsBits;

typedef struct {
	GLboolean newEnd;
	GLuint base;          /* set by glListBase */
	GLuint currentIndex;  /* list currently being built (or zero) */
	GLenum mode;          /* GL_COMPILE, GL_COMPILE_AND_EXECUTE or zero */
} CRListsState;


DECLEXPORT(void) crStateListsInit(CRContext *ctx);
DECLEXPORT(void) crStateListsDestroy(CRContext *ctx);

DECLEXPORT(void) crStateListsDiff(CRListsBits *bb, CRbitvalue *bitID,
                                  CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStateListsSwitch(CRListsBits *bb, CRbitvalue *bitID, 
                                    CRContext *fromCtx, CRContext *toCtx);

#ifdef __cplusplus
}
#endif

#endif
