/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_POINT_H
#define CR_STATE_POINT_H

#include "state/cr_statetypes.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	CRbitvalue enableSmooth[CR_MAX_BITARRAY];
	CRbitvalue size[CR_MAX_BITARRAY];
#ifdef CR_ARB_point_parameters
	CRbitvalue minSize[CR_MAX_BITARRAY];
	CRbitvalue maxSize[CR_MAX_BITARRAY];
	CRbitvalue fadeThresholdSize[CR_MAX_BITARRAY];
	CRbitvalue distanceAttenuation[CR_MAX_BITARRAY];
#endif
#ifdef CR_ARB_point_sprite
	CRbitvalue enableSprite[CR_MAX_BITARRAY];
	CRbitvalue coordReplacement[CR_MAX_TEXTURE_UNITS][CR_MAX_BITARRAY];
#endif
	CRbitvalue spriteCoordOrigin[CR_MAX_BITARRAY];
	CRbitvalue dirty[CR_MAX_BITARRAY];
} CRPointBits;

typedef struct {
	GLboolean	pointSmooth;
	GLfloat		pointSize;
#ifdef CR_ARB_point_parameters
	GLfloat		minSize, maxSize;
	GLfloat		fadeThresholdSize;
	GLfloat		distanceAttenuation[3];
#endif
#ifdef CR_ARB_point_sprite
	GLboolean pointSprite;
	GLboolean coordReplacement[CR_MAX_TEXTURE_UNITS];
#endif
    GLfloat spriteCoordOrigin;
    GLfloat reserved; /* added to make sure alignment of sttructures following CRPointState in CRContext does not change */
} CRPointState;

DECLEXPORT(void) crStatePointInit (CRContext *ctx);

DECLEXPORT(void) crStatePointDiff(CRPointBits *bb, CRbitvalue *bitID,
                                  CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStatePointSwitch(CRPointBits *bb, CRbitvalue *bitID, 
                                    CRContext *fromCtx, CRContext *toCtx);

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_POINT_H */
