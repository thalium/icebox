/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_HINT_H 
#define CR_STATE_HINT_H 

#include "state/cr_statetypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	CRbitvalue dirty[CR_MAX_BITARRAY];
	CRbitvalue perspectiveCorrection[CR_MAX_BITARRAY];
	CRbitvalue pointSmooth[CR_MAX_BITARRAY];
	CRbitvalue lineSmooth[CR_MAX_BITARRAY];
	CRbitvalue polygonSmooth[CR_MAX_BITARRAY];
	CRbitvalue fog[CR_MAX_BITARRAY];
#ifdef CR_EXT_clip_volume_hint
	CRbitvalue clipVolumeClipping[CR_MAX_BITARRAY];
#endif
#ifdef CR_ARB_texture_compression
	CRbitvalue textureCompression[CR_MAX_BITARRAY];
#endif
#ifdef CR_SGIS_generate_mipmap
	CRbitvalue generateMipmap[CR_MAX_BITARRAY];
#endif
} CRHintBits;

typedef struct {
	GLenum perspectiveCorrection;
	GLenum pointSmooth;
	GLenum lineSmooth;
	GLenum polygonSmooth;
	GLenum fog;
#ifdef CR_EXT_clip_volume_hint
	GLenum clipVolumeClipping;
#endif
#ifdef CR_ARB_texture_compression
	GLenum textureCompression;
#endif
#ifdef CR_SGIS_generate_mipmap
	GLenum generateMipmap;
#endif
} CRHintState;

DECLEXPORT(void) crStateHintInit(CRContext *ctx);

DECLEXPORT(void) crStateHintDiff(CRHintBits *bb, CRbitvalue *bitID,
                                 CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStateHintSwitch(CRHintBits *bb, CRbitvalue *bitID, 
                                   CRContext *fromCtx, CRContext *toCtx);

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_HINT_H */
