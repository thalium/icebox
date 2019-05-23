/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_LIGHTING_H
#define CR_STATE_LIGHTING_H

#include "state/cr_statetypes.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	CRbitvalue dirty[CR_MAX_BITARRAY];
	CRbitvalue enable[CR_MAX_BITARRAY];
	CRbitvalue ambient[CR_MAX_BITARRAY];
	CRbitvalue diffuse[CR_MAX_BITARRAY];
	CRbitvalue specular[CR_MAX_BITARRAY];
	CRbitvalue position[CR_MAX_BITARRAY];
	CRbitvalue attenuation[CR_MAX_BITARRAY];
	CRbitvalue spot[CR_MAX_BITARRAY];
} CRLightBits;

typedef struct {
	CRbitvalue dirty[CR_MAX_BITARRAY];
	CRbitvalue shadeModel[CR_MAX_BITARRAY];
	CRbitvalue colorMaterial[CR_MAX_BITARRAY];
	CRbitvalue lightModel[CR_MAX_BITARRAY];
	CRbitvalue material[CR_MAX_BITARRAY];
	CRbitvalue enable[CR_MAX_BITARRAY];
	CRLightBits *light;
} CRLightingBits;

typedef struct {
	GLboolean	enable;
	GLcolorf	ambient;
	GLcolorf	diffuse;
	GLcolorf	specular;
	GLvectorf	position;
	GLvectorf	objPosition;
	GLfloat		constantAttenuation;
	GLfloat		linearAttenuation;
	GLfloat		quadraticAttenuation;
	GLvectorf	spotDirection;
	GLfloat 	spotExponent;
	GLfloat		spotCutoff;
} CRLight;

typedef struct {
	GLboolean	lighting;
	GLboolean	colorMaterial;
	GLenum		shadeModel;
	GLenum		colorMaterialMode;
	GLenum		colorMaterialFace;
	GLcolorf	ambient[2];     /* material front/back */
	GLcolorf	diffuse[2];     /* material front/back */
	GLcolorf	specular[2];     /* material front/back */
	GLcolorf	emission[2];     /* material front/back */
	GLfloat		shininess[2];     /* material front/back */
	GLint       indexes[2][3];    /* material front/back amb/diff/spec */
	GLcolorf	lightModelAmbient;
	GLboolean	lightModelLocalViewer;
	GLboolean	lightModelTwoSide;
#if defined(CR_EXT_separate_specular_color) || defined(CR_OPENGL_VERSION_1_2)
	GLenum		lightModelColorControlEXT; /* CR_EXT_separate_specular_color */
#endif
	GLboolean	colorSumEXT; /* CR_EXT_secondary_color */
	CRLight		*light;
} CRLightingState;

DECLEXPORT(void) crStateLightingInitBits (CRLightingBits *l);
DECLEXPORT(void) crStateLightingDestroyBits (CRLightingBits *l);
DECLEXPORT(void) crStateLightingInit (CRContext *ctx);
DECLEXPORT(void) crStateLightingDestroy (CRContext *ctx);

DECLEXPORT(void) crStateLightingDiff(CRLightingBits *bb, CRbitvalue *bitID,
                                     CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStateLightingSwitch(CRLightingBits *bb, CRbitvalue *bitID,
                                       CRContext *fromCtx, CRContext *toCtx);

DECLEXPORT(void) crStateColorMaterialRecover( void );

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_LIGHTING_H */
