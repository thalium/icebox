/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_FOG_H 
#define CR_STATE_FOG_H 

#include "state/cr_statetypes.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	CRbitvalue dirty[CR_MAX_BITARRAY];
	CRbitvalue color[CR_MAX_BITARRAY];
	CRbitvalue index[CR_MAX_BITARRAY];
	CRbitvalue density[CR_MAX_BITARRAY];
	CRbitvalue start[CR_MAX_BITARRAY];
	CRbitvalue end[CR_MAX_BITARRAY];
	CRbitvalue mode[CR_MAX_BITARRAY];
	CRbitvalue enable[CR_MAX_BITARRAY];
#ifdef CR_NV_fog_distance
	CRbitvalue fogDistanceMode[CR_MAX_BITARRAY];
#endif
#ifdef CR_EXT_fog_coord
	CRbitvalue fogCoordinateSource[CR_MAX_BITARRAY];
#endif
} CRFogBits;

typedef struct {
	GLcolorf  color;
	GLint     index;
	GLfloat   density;
	GLfloat   start;
	GLfloat   end;
	GLint     mode;
	GLboolean enable;
#ifdef CR_NV_fog_distance
	GLenum fogDistanceMode;
#endif
#ifdef CR_EXT_fog_coord
	GLenum fogCoordinateSource;
#endif
} CRFogState;

DECLEXPORT(void) crStateFogInit(CRContext *ctx);

DECLEXPORT(void) crStateFogDiff(CRFogBits *bb, CRbitvalue *bitID,
                                CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStateFogSwitch(CRFogBits *bb, CRbitvalue *bitID, 
                                  CRContext *fromCtx, CRContext *toCtx);

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_FOG_H */
