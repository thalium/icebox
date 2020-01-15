/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "state.h"
#include "cr_mem.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"

void crStateLightingInitBits (CRLightingBits *l)
{
	l->light = (CRLightBits *) crCalloc (sizeof(*(l->light)) * CR_MAX_LIGHTS);
}

void crStateLightingDestroyBits (CRLightingBits *l)
{
	crFree(l->light);
}

void crStateLightingDestroy (CRContext *ctx)
{
	crFree(ctx->lighting.light);
}

void crStateLightingInit (CRContext *ctx)
{
	CRLightingState *l = &ctx->lighting;
	CRStateBits *sb = GetCurrentBits(ctx->pStateTracker);
	CRLightingBits *lb = &(sb->lighting);
	int i;
	GLvectorf zero_vector	= {0.0f, 0.0f, 0.0f, 1.0f};
	GLcolorf zero_color	= {0.0f, 0.0f, 0.0f, 1.0f};
	GLcolorf ambient_color  = {0.2f, 0.2f, 0.2f, 1.0f};
	GLcolorf diffuse_color  = {0.8f, 0.8f, 0.8f, 1.0f};
	GLvectorf spot_vector	= {0.0f, 0.0f, -1.0f, 0.0f};
	GLcolorf one_color	= {1.0f, 1.0f, 1.0f, 1.0f};

	l->lighting = GL_FALSE;
	RESET(lb->enable, ctx->bitid);
	l->colorMaterial = GL_FALSE;
	RESET(lb->colorMaterial, ctx->bitid);
	l->shadeModel = GL_SMOOTH;
	RESET(lb->shadeModel, ctx->bitid);
	l->colorMaterialMode = GL_AMBIENT_AND_DIFFUSE;
	l->colorMaterialFace = GL_FRONT_AND_BACK;
	l->ambient[0] = ambient_color;
	l->diffuse[0] = diffuse_color;
	l->specular[0] = zero_color;
	l->emission[0] = zero_color;
	l->shininess[0] = 0.0f;
	l->indexes[0][0] = 0;
	l->indexes[0][1] = 1;
	l->indexes[0][2] = 1;
	l->ambient[1] = ambient_color;
	l->diffuse[1] = diffuse_color;
	l->specular[1] = zero_color;
	l->emission[1] = zero_color;
	l->shininess[1] = 0.0f;
	l->indexes[1][0] = 0;
	l->indexes[1][1] = 1;
	l->indexes[1][2] = 1;
	RESET(lb->material, ctx->bitid);
	l->lightModelAmbient = ambient_color;
	l->lightModelLocalViewer = GL_FALSE;
	l->lightModelTwoSide = GL_FALSE;
#if defined(CR_EXT_separate_specular_color)
	l->lightModelColorControlEXT = GL_SINGLE_COLOR_EXT;
#elif defined(CR_OPENGL_VERSION_1_2)
	l->lightModelColorControlEXT = GL_SINGLE_COLOR;
#endif
	RESET(lb->lightModel, ctx->bitid);
#if defined(CR_EXT_secondary_color)
	l->colorSumEXT = GL_FALSE;
#endif
	l->light = (CRLight *) crCalloc (sizeof (*(l->light)) * CR_MAX_LIGHTS);

	for (i=0; i<CR_MAX_LIGHTS; i++)
	{
		CRLightBits *ltb = lb->light + i;
		l->light[i].enable = GL_FALSE;
		RESET(ltb->enable, ctx->bitid);
		l->light[i].ambient = zero_color;
		RESET(ltb->ambient, ctx->bitid);
		l->light[i].diffuse = zero_color;
		RESET(ltb->diffuse, ctx->bitid);
		l->light[i].specular = zero_color;
		RESET(ltb->specular, ctx->bitid);
		l->light[i].position = zero_vector;
		l->light[i].position.z = 1.0f;
		l->light[i].position.w = 0.0f;
		l->light[i].objPosition = l->light[i].position;
		RESET(ltb->position, ctx->bitid);
		l->light[i].spotDirection = spot_vector;
		l->light[i].spotExponent = 0.0f;
		l->light[i].spotCutoff = 180.0f;
		RESET(ltb->spot, ctx->bitid);
		l->light[i].constantAttenuation= 1.0f;
		l->light[i].linearAttenuation= 0.0f;
		l->light[i].quadraticAttenuation = 0.0f;
		RESET(ltb->attenuation, ctx->bitid);
		RESET(ltb->dirty, ctx->bitid);
	}
	l->light[0].diffuse = one_color;
	l->light[0].specular = one_color;

	RESET(lb->dirty, ctx->bitid);
}

void STATE_APIENTRY crStateShadeModel (PCRStateTracker pState, GLenum mode)
{
	CRContext *g = GetCurrentContext(pState);
	CRLightingState *l = &(g->lighting);
	CRStateBits *sb = GetCurrentBits(pState);
	CRLightingBits *lb = &(sb->lighting);

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "ShadeModel called in begin/end");
		return;
	}

	FLUSH();

	if (mode != GL_SMOOTH &&
			mode != GL_FLAT)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "ShadeModel: Bogus mode 0x%x", mode);
		return;
	}

	l->shadeModel = mode;
	DIRTY(lb->shadeModel, g->neg_bitid);
	DIRTY(lb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateColorMaterial (PCRStateTracker pState, GLenum face, GLenum mode)
{
	CRContext *g = GetCurrentContext(pState);
	CRLightingState *l = &(g->lighting);
	CRStateBits *sb = GetCurrentBits(pState);
	CRLightingBits *lb = &(sb->lighting);

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "ColorMaterial called in begin/end");
		return;
	}

	FLUSH();

	if (face != GL_FRONT &&
			face != GL_BACK &&
			face != GL_FRONT_AND_BACK)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "ColorMaterial: Bogus face &d", face);
		return;
	}

	if (mode != GL_EMISSION &&
			mode != GL_AMBIENT &&
			mode != GL_DIFFUSE &&
			mode != GL_SPECULAR &&
			mode != GL_AMBIENT_AND_DIFFUSE)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "ColorMaterial: Bogus mode &d", mode);
		return;
	}

	l->colorMaterialFace = face;
	l->colorMaterialMode = mode;
	/* XXX this could conceivably be needed here (BP) */
	/*
	crStateColorMaterialRecover();
	*/
	DIRTY(lb->colorMaterial, g->neg_bitid);
	DIRTY(lb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateLightModelfv (PCRStateTracker pState, GLenum pname, const GLfloat *param)
{
	CRContext *g = GetCurrentContext(pState);
	CRLightingState *l = &(g->lighting);
	CRStateBits *sb = GetCurrentBits(pState);
	CRLightingBits *lb = &(sb->lighting);

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "LightModelfv called in begin/end");
		return;
	}

	FLUSH();

	switch (pname)
	{
		case GL_LIGHT_MODEL_LOCAL_VIEWER:
			l->lightModelLocalViewer = (GLboolean) (*param==0.0f?GL_FALSE:GL_TRUE);
			break;
		case GL_LIGHT_MODEL_TWO_SIDE:
			l->lightModelTwoSide = (GLboolean) (*param==0.0f?GL_FALSE:GL_TRUE);
			break;
		case GL_LIGHT_MODEL_AMBIENT:
			l->lightModelAmbient.r = param[0];
			l->lightModelAmbient.g = param[1];
			l->lightModelAmbient.b = param[2];
			l->lightModelAmbient.a = param[3];
			break;
#if defined(CR_OPENGL_VERSION_1_2)
		case GL_LIGHT_MODEL_COLOR_CONTROL:
			if (param[0] == GL_SEPARATE_SPECULAR_COLOR || param[0] == GL_SINGLE_COLOR)
			{
				l->lightModelColorControlEXT = (GLenum) param[0];
			}
			else
			{
				crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "LightModel: Invalid param for LIGHT_MODEL_COLOR_CONTROL: 0x%x", param[0]);
				return;
			}
			break;
#else
#if defined(CR_EXT_separate_specular_color)
		case GL_LIGHT_MODEL_COLOR_CONTROL_EXT:
			if(g->extensions.EXT_separate_specular_color)
			{
				if (param[0] == GL_SEPARATE_SPECULAR_COLOR_EXT || param[0] == GL_SINGLE_COLOR_EXT)
				{
					l->lightModelColorControlEXT = (GLenum) param[0];
				}
				else
				{
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "LightModel: Invalid param for LIGHT_MODEL_COLOR_CONTROL: 0x%x", param[0]);
					return;
				}
			}
			else
			{
				crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "LightModel( LIGHT_MODEL_COLOR_CONTROL, ...) - EXT_separate_specular_color is unavailable.");
				return;
			}
			break;
#endif
#endif
		default:
			crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "LightModelfv: Invalid pname: 0x%x", pname);
			return;
	}
	DIRTY(lb->lightModel, g->neg_bitid);
	DIRTY(lb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateLightModeliv (PCRStateTracker pState, GLenum pname, const GLint *param)
{
	GLfloat f_param;
	GLcolor f_color;
#ifndef CR_OPENGL_VERSION_1_2
	CRContext *g = GetCurrentContext(pState);
#endif

	switch (pname)
	{
		case GL_LIGHT_MODEL_LOCAL_VIEWER:
		case GL_LIGHT_MODEL_TWO_SIDE:
			f_param = (GLfloat) (*param);
			crStateLightModelfv(pState, pname, &f_param);
			break;
		case GL_LIGHT_MODEL_AMBIENT:
			f_color.r = ((GLfloat)param[0])/CR_MAXINT;
			f_color.g = ((GLfloat)param[1])/CR_MAXINT;
			f_color.b = ((GLfloat)param[2])/CR_MAXINT;
			f_color.a = ((GLfloat)param[3])/CR_MAXINT;
			crStateLightModelfv(pState, pname, (GLfloat *) &f_color);
			break;
#if defined(CR_OPENGL_VERSION_1_2)
		case GL_LIGHT_MODEL_COLOR_CONTROL:
			f_param = (GLfloat) (*param);
			crStateLightModelfv(pState, pname, &f_param);
			break;
#else
#ifdef CR_EXT_separate_specular_color
		case GL_LIGHT_MODEL_COLOR_CONTROL_EXT:
			if (g->extensions.EXT_separate_specular_color) {
				f_param = (GLfloat) (*param);
				crStateLightModelfv(pState, pname, &f_param);
			} else {
				crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "LightModeliv(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, ...) - EXT_separate_specular_color not enabled!");
				return;
			}
			break;
#endif
#endif
		default:
			crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "LightModeliv: Invalid pname: 0x%x", pname);
			return;
	}
}

void STATE_APIENTRY crStateLightModelf (PCRStateTracker pState, GLenum pname, GLfloat param)
{
	crStateLightModelfv(pState, pname, &param);
}

void STATE_APIENTRY crStateLightModeli (PCRStateTracker pState, GLenum pname, GLint param)
{
	GLfloat f_param = (GLfloat) param;
	crStateLightModelfv(pState, pname, &f_param);
}

void STATE_APIENTRY crStateLightfv (PCRStateTracker pState, GLenum light, GLenum pname, const GLfloat *param)
{
	CRContext *g = GetCurrentContext(pState);
	CRLightingState *l = &(g->lighting);
	CRTransformState *t = &(g->transform);
	CRLight *lt;
	unsigned int i;
	GLfloat x, y, z, w;
	CRmatrix inv;
	CRmatrix *mat;
	CRStateBits *sb = GetCurrentBits(pState);
	CRLightingBits *lb = &(sb->lighting);
	CRLightBits *ltb;

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glLightfv called in begin/end");
		return;
	}

	FLUSH();

	i = light - GL_LIGHT0;
	if (i>=g->limits.maxLights)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glLight: invalid light specified: 0x%x", light);
		return;
	}

	lt = l->light + i;
	ltb = lb->light + i;

	switch (pname)
	{
		case GL_AMBIENT:
			lt->ambient.r = param[0];
			lt->ambient.g = param[1];
			lt->ambient.b = param[2];
			lt->ambient.a = param[3];
			DIRTY(ltb->ambient, g->neg_bitid);
			break;
		case GL_DIFFUSE:
			lt->diffuse.r = param[0];
			lt->diffuse.g = param[1];
			lt->diffuse.b = param[2];
			lt->diffuse.a = param[3];
			DIRTY(ltb->diffuse, g->neg_bitid);
			break;
		case GL_SPECULAR:
			lt->specular.r = param[0];
			lt->specular.g = param[1];
			lt->specular.b = param[2];
			lt->specular.a = param[3];
			DIRTY(ltb->specular, g->neg_bitid);
			break;
		case GL_POSITION:
			x = param[0];
			y = param[1];
			z = param[2];
			w = param[3];
			mat = t->modelViewStack.top;
			lt->objPosition.x = x;
			lt->objPosition.y = y;
			lt->objPosition.z = z;
			lt->objPosition.w = w;

			lt->position.x = mat->m00*x + mat->m10*y + mat->m20*z + mat->m30*w;
			lt->position.y = mat->m01*x + mat->m11*y + mat->m21*z + mat->m31*w;
			lt->position.z = mat->m02*x + mat->m12*y + mat->m22*z + mat->m32*w;
			lt->position.w = mat->m03*x + mat->m13*y + mat->m23*z + mat->m33*w;

			DIRTY(ltb->position, g->neg_bitid);
			break;
		case GL_SPOT_DIRECTION:
			lt->spotDirection.x = param[0];
			lt->spotDirection.y = param[1];
			lt->spotDirection.z = param[2];
			lt->spotDirection.w = 0.0f;
			mat = t->modelViewStack.top;

			if (lt->objPosition.w != 0.0f)
			{
				lt->spotDirection.w = - ( (	lt->objPosition.x * lt->spotDirection.x +
							lt->objPosition.y * lt->spotDirection.y +
							lt->objPosition.z * lt->spotDirection.z ) /
						lt->objPosition.w );
			}

			crMatrixInvertTranspose(&inv, mat);
			crStateTransformXformPointMatrixf (&inv, &(lt->spotDirection));

			DIRTY(ltb->spot, g->neg_bitid);
			break;
		case GL_SPOT_EXPONENT:
			if (*param < 0.0f || *param > 180.0f)
			{
				crStateError(pState, __LINE__, __FILE__, GL_INVALID_VALUE, "glLight: spot exponent out of range: %f", *param);
				return;
			}
			lt->spotExponent = *param;
			DIRTY(ltb->spot, g->neg_bitid);
			break;
		case GL_SPOT_CUTOFF:
			if ((*param < 0.0f || *param > 90.0f) && *param != 180.0f)
			{
				crStateError(pState, __LINE__, __FILE__, GL_INVALID_VALUE, "glLight: spot cutoff out of range: %f", *param);
				return;
			}
			lt->spotCutoff = *param;
			DIRTY(ltb->spot, g->neg_bitid);
			break;
		case GL_CONSTANT_ATTENUATION:
			if (*param < 0.0f)
			{
				crStateError(pState, __LINE__, __FILE__, GL_INVALID_VALUE, "glLight: constant Attenuation negative: %f", *param);
				return;
			}
			lt->constantAttenuation = *param;
			DIRTY(ltb->attenuation, g->neg_bitid);
			break;
		case GL_LINEAR_ATTENUATION:
			if (*param < 0.0f)
			{
				crStateError(pState, __LINE__, __FILE__, GL_INVALID_VALUE, "glLight: linear Attenuation negative: %f", *param);
				return;
			}
			lt->linearAttenuation = *param;
			DIRTY(ltb->attenuation, g->neg_bitid);
			break;
		case GL_QUADRATIC_ATTENUATION:
			if (*param < 0.0f)
			{
				crStateError(pState, __LINE__, __FILE__, GL_INVALID_VALUE, "glLight: quadratic Attenuation negative: %f", *param);
				return;
			}
			lt->quadraticAttenuation = *param;
			DIRTY(ltb->attenuation, g->neg_bitid);
			break;
		default:
			crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glLight: invalid pname: 0x%x", pname);
			return;
	}
	DIRTY(ltb->dirty, g->neg_bitid);
	DIRTY(lb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateLightiv (PCRStateTracker pState, GLenum light, GLenum pname, const GLint *param)
{
	GLfloat f_param;
	GLcolor f_color;
	GLvector f_vector;

	switch (pname)
	{
		case GL_AMBIENT:
		case GL_DIFFUSE:
		case GL_SPECULAR:
			f_color.r = ((GLfloat)param[0])/CR_MAXINT;
			f_color.g = ((GLfloat)param[1])/CR_MAXINT;
			f_color.b = ((GLfloat)param[2])/CR_MAXINT;
			f_color.a = ((GLfloat)param[3])/CR_MAXINT;
			crStateLightfv(pState, light, pname, (GLfloat *) &f_color);
			break;
		case GL_POSITION:
		case GL_SPOT_DIRECTION:
			f_vector.x = (GLfloat) param[0];
			f_vector.y = (GLfloat) param[1];
			f_vector.z = (GLfloat) param[2];
			f_vector.w = (GLfloat) param[3];
			crStateLightfv(pState, light, pname, (GLfloat *) &f_vector);
			break;
		case GL_SPOT_EXPONENT:
		case GL_SPOT_CUTOFF:
		case GL_CONSTANT_ATTENUATION:
		case GL_LINEAR_ATTENUATION:
		case GL_QUADRATIC_ATTENUATION:
			f_param = (GLfloat) (*param);
			crStateLightfv(pState, light, pname, &f_param);
			break;
		default:
			crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glLight: invalid pname: 0x%x", pname);
			return;
	}
}

void STATE_APIENTRY crStateLightf (PCRStateTracker pState, GLenum light, GLenum pname, GLfloat param)
{
	crStateLightfv(pState, light, pname, &param);
}

void STATE_APIENTRY crStateLighti (PCRStateTracker pState, GLenum light, GLenum pname, GLint param)
{
	GLfloat f_param = (GLfloat) param;
	crStateLightfv(pState, light, pname, &f_param);
}

void STATE_APIENTRY crStateMaterialfv (PCRStateTracker pState, GLenum face, GLenum pname, const GLfloat *param)
{
	CRContext *g = GetCurrentContext(pState);
	CRLightingState *l = &(g->lighting);
	CRStateBits *sb = GetCurrentBits(pState);
	CRLightingBits *lb = &(sb->lighting);

	if (!g->current.inBeginEnd)
	{
		FLUSH();
	}

	switch (pname)
	{
		case GL_AMBIENT :
			switch (face)
			{
				case GL_FRONT:
					l->ambient[0].r = param[0];
					l->ambient[0].g = param[1];
					l->ambient[0].b = param[2];
					l->ambient[0].a = param[3];
					break;
				case GL_FRONT_AND_BACK:
					l->ambient[0].r = param[0];
					l->ambient[0].g = param[1];
					l->ambient[0].b = param[2];
					l->ambient[0].a = param[3];
					RT_FALL_THRU();
				case GL_BACK:
					l->ambient[1].r = param[0];
					l->ambient[1].g = param[1];
					l->ambient[1].b = param[2];
					l->ambient[1].a = param[3];
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glMaterialfv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_AMBIENT_AND_DIFFUSE :
			switch (face)
			{
				case GL_FRONT:
					l->ambient[0].r = param[0];
					l->ambient[0].g = param[1];
					l->ambient[0].b = param[2];
					l->ambient[0].a = param[3];
					break;
				case GL_FRONT_AND_BACK:
					l->ambient[0].r = param[0];
					l->ambient[0].g = param[1];
					l->ambient[0].b = param[2];
					l->ambient[0].a = param[3];
					RT_FALL_THRU();
				case GL_BACK:
					l->ambient[1].r = param[0];
					l->ambient[1].g = param[1];
					l->ambient[1].b = param[2];
					l->ambient[1].a = param[3];
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glMaterialfv: bad face: 0x%x", face);
					return;
			}
			RT_FALL_THRU();
		case GL_DIFFUSE :
			switch (face)
			{
				case GL_FRONT:
					l->diffuse[0].r = param[0];
					l->diffuse[0].g = param[1];
					l->diffuse[0].b = param[2];
					l->diffuse[0].a = param[3];
					break;
				case GL_FRONT_AND_BACK:
					l->diffuse[0].r = param[0];
					l->diffuse[0].g = param[1];
					l->diffuse[0].b = param[2];
					l->diffuse[0].a = param[3];
					RT_FALL_THRU();
				case GL_BACK:
					l->diffuse[1].r = param[0];
					l->diffuse[1].g = param[1];
					l->diffuse[1].b = param[2];
					l->diffuse[1].a = param[3];
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glMaterialfv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_SPECULAR :
			switch (face)
			{
				case GL_FRONT:
					l->specular[0].r = param[0];
					l->specular[0].g = param[1];
					l->specular[0].b = param[2];
					l->specular[0].a = param[3];
					break;
				case GL_FRONT_AND_BACK:
					l->specular[0].r = param[0];
					l->specular[0].g = param[1];
					l->specular[0].b = param[2];
					l->specular[0].a = param[3];
					RT_FALL_THRU();
				case GL_BACK:
					l->specular[1].r = param[0];
					l->specular[1].g = param[1];
					l->specular[1].b = param[2];
					l->specular[1].a = param[3];
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glMaterialfv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_EMISSION :
			switch (face)
			{
				case GL_FRONT:
					l->emission[0].r = param[0];
					l->emission[0].g = param[1];
					l->emission[0].b = param[2];
					l->emission[0].a = param[3];
					break;
				case GL_FRONT_AND_BACK:
					l->emission[0].r = param[0];
					l->emission[0].g = param[1];
					l->emission[0].b = param[2];
					l->emission[0].a = param[3];
					RT_FALL_THRU();
				case GL_BACK:
					l->emission[1].r = param[0];
					l->emission[1].g = param[1];
					l->emission[1].b = param[2];
					l->emission[1].a = param[3];
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glMaterialfv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_SHININESS:
			if (*param > 180.0f || *param < 0.0f)
			{
				crStateError(pState, __LINE__, __FILE__, GL_INVALID_VALUE, "glMaterialfv: param out of range: %f", param);
				return;
			}

			switch (face)
			{
				case GL_FRONT:
					l->shininess[0] = *param;
					break;
				case GL_FRONT_AND_BACK:
					l->shininess[0] = *param;
					RT_FALL_THRU();
				case GL_BACK:
					l->shininess[1] = *param;
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glMaterialfv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_COLOR_INDEXES :
			switch (face)
			{
				case GL_FRONT:
					l->indexes[0][0] = (GLint) param[0];
					l->indexes[0][1] = (GLint) param[1];
					l->indexes[0][2] = (GLint) param[2];
					break;
				case GL_FRONT_AND_BACK:
					l->indexes[0][0] = (GLint) param[0];
					l->indexes[0][1] = (GLint) param[1];
					l->indexes[0][2] = (GLint) param[2];
					RT_FALL_THRU();
				case GL_BACK:
					l->indexes[1][0] = (GLint) param[0];
					l->indexes[1][1] = (GLint) param[1];
					l->indexes[1][2] = (GLint) param[2];
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glMaterialfv: bad face: 0x%x", face);
					return;
			}
			break;
		default:
			crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glMaterialfv: bad pname: 0x%x", pname);
			return;
	}
	DIRTY(lb->material, g->neg_bitid);
	DIRTY(lb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateMaterialiv (PCRStateTracker pState, GLenum face, GLenum pname, const GLint *param)
{
	GLfloat f_param;
	GLcolor f_color;

	switch (pname)
	{
		case GL_AMBIENT :
		case GL_AMBIENT_AND_DIFFUSE :
		case GL_DIFFUSE :
		case GL_SPECULAR :
		case GL_EMISSION :
			f_color.r = ((GLfloat) param[0]) / ((GLfloat) CR_MAXINT);
			f_color.g = ((GLfloat) param[1]) / ((GLfloat) CR_MAXINT);
			f_color.b = ((GLfloat) param[2]) / ((GLfloat) CR_MAXINT);
			f_color.a = ((GLfloat) param[3]) / ((GLfloat) CR_MAXINT);
			crStateMaterialfv(pState, face, pname, (GLfloat *) &f_color);
			break;
		case GL_SHININESS:
			f_param = (GLfloat) (*param);
			crStateMaterialfv(pState, face, pname, (GLfloat *) &f_param);
			break;
		case GL_COLOR_INDEXES :
			f_param = (GLfloat) (*param);
			crStateMaterialfv(pState, face, pname, (GLfloat *) &f_param);
			break;
		default:
			crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glMaterialiv: bad pname: 0x%x", pname);
			return;
	}
}

void STATE_APIENTRY crStateMaterialf (PCRStateTracker pState, GLenum face, GLenum pname, GLfloat param)
{
    if (pname != GL_SHININESS)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "crStateMaterialf: bad pname: 0x%x", pname);
        return;
    }
    crStateMaterialfv(pState, face, pname, &param);
}

void STATE_APIENTRY crStateMateriali (PCRStateTracker pState, GLenum face, GLenum pname, GLint param)
{
    GLfloat f_param = (GLfloat) param;

    if (pname != GL_SHININESS)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "crStateMateriali: bad pname: 0x%x", pname);
        return;
    }

    crStateMaterialfv(pState, face, pname, &f_param);
}

void STATE_APIENTRY crStateGetLightfv (PCRStateTracker pState, GLenum light, GLenum pname, GLfloat *param)
{
	CRContext *g = GetCurrentContext(pState);
	CRLightingState *l = &(g->lighting);
	CRLight *lt;
	unsigned int i;

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION,
				"glGetLightfv called in begin/end");
		return;
	}

	i = light - GL_LIGHT0;
	if (i>=g->limits.maxLights)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
				"glGetLight: invalid light specified: 0x%x", light);
		return;
	}

	lt = l->light + i;

	switch (pname)
	{
		case GL_AMBIENT:
			param[0] = lt->ambient.r;
			param[1] = lt->ambient.g;
			param[2] = lt->ambient.b;
			param[3] = lt->ambient.a;
			break;
		case GL_DIFFUSE:
			param[0] = lt->diffuse.r;
			param[1] = lt->diffuse.g;
			param[2] = lt->diffuse.b;
			param[3] = lt->diffuse.a;
			break;
		case GL_SPECULAR:
			param[0] = lt->specular.r;
			param[1] = lt->specular.g;
			param[2] = lt->specular.b;
			param[3] = lt->specular.a;
			break;
		case GL_POSITION:
			param[0] = lt->position.x;
			param[1] = lt->position.y;
			param[2] = lt->position.z;
			param[3] = lt->position.w;
			break;
		case GL_SPOT_DIRECTION:
			param[0] = lt->spotDirection.x;
			param[1] = lt->spotDirection.y;
			param[2] = lt->spotDirection.z;
#if 0
			/* the w-component of the direction, although possibly (?)
				 useful to keep around internally, is not returned as part
				 of the get. */
			param[3] = lt->spotDirection.w;
#endif
			break;
		case GL_SPOT_EXPONENT:
			*param = lt->spotExponent;
			break;
		case GL_SPOT_CUTOFF:
			*param = lt->spotCutoff;
			break;
		case GL_CONSTANT_ATTENUATION:
			*param = lt->constantAttenuation;
			break;
		case GL_LINEAR_ATTENUATION:
			*param = lt->linearAttenuation;
			break;
		case GL_QUADRATIC_ATTENUATION:
			*param = lt->quadraticAttenuation;
			break;
		default:
			crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
					"glGetLight: invalid pname: 0x%x", pname);
			return;
	}
}

void STATE_APIENTRY crStateGetLightiv (PCRStateTracker pState, GLenum light, GLenum pname, GLint *param)
{
	CRContext *g = GetCurrentContext(pState);
	CRLightingState *l = &(g->lighting);
	CRLight *lt;
	unsigned int i;

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION,
				"glGetLightiv called in begin/end");
		return;
	}

	i = light - GL_LIGHT0;
	if (i>=g->limits.maxLights)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
				"glGetLight: invalid light specified: 0x%x", light);
		return;
	}

	lt = l->light + i;

	switch (pname)
	{
		case GL_AMBIENT:
			param[0] = (GLint) (lt->ambient.r * CR_MAXINT);
			param[1] = (GLint) (lt->ambient.g * CR_MAXINT);
			param[2] = (GLint) (lt->ambient.b * CR_MAXINT);
			param[3] = (GLint) (lt->ambient.a * CR_MAXINT);
			break;
		case GL_DIFFUSE:
			param[0] = (GLint) (lt->diffuse.r * CR_MAXINT);
			param[1] = (GLint) (lt->diffuse.g * CR_MAXINT);
			param[2] = (GLint) (lt->diffuse.b * CR_MAXINT);
			param[3] = (GLint) (lt->diffuse.a * CR_MAXINT);
			break;
		case GL_SPECULAR:
			param[0] = (GLint) (lt->specular.r * CR_MAXINT);
			param[1] = (GLint) (lt->specular.g * CR_MAXINT);
			param[2] = (GLint) (lt->specular.b * CR_MAXINT);
			param[3] = (GLint) (lt->specular.a * CR_MAXINT);
			break;
		case GL_POSITION:
			param[0] = (GLint) (lt->position.x);
			param[1] = (GLint) (lt->position.y);
			param[2] = (GLint) (lt->position.z);
			param[3] = (GLint) (lt->position.w);
			break;
		case GL_SPOT_DIRECTION:
			param[0] = (GLint) (lt->spotDirection.x);
			param[1] = (GLint) (lt->spotDirection.y);
			param[2] = (GLint) (lt->spotDirection.z);
#if 0
			/* the w-component of the direction, although possibly (?)
				 useful to keep around internally, is not returned as part
				 of the get. */
			param[3] = (GLint) (lt->spotDirection.w);
#endif
			break;
		case GL_SPOT_EXPONENT:
			*param = (GLint) (lt->spotExponent);
			break;
		case GL_SPOT_CUTOFF:
			*param = (GLint) (lt->spotCutoff);
			break;
		case GL_CONSTANT_ATTENUATION:
			*param = (GLint) (lt->constantAttenuation);
			break;
		case GL_LINEAR_ATTENUATION:
			*param = (GLint) (lt->linearAttenuation);
			break;
		case GL_QUADRATIC_ATTENUATION:
			*param = (GLint) (lt->quadraticAttenuation);
			break;
		default:
			crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
					"glGetLight: invalid pname: 0x%x", pname);
			return;
	}
}

void STATE_APIENTRY crStateGetMaterialfv (PCRStateTracker pState, GLenum face, GLenum pname, GLfloat *param)
{
	CRContext *g = GetCurrentContext(pState);
	CRLightingState *l = &(g->lighting);

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION,
				"glGetMaterialfv called in begin/end");
		return;
	}

	switch (pname)
	{
		case GL_AMBIENT:
			switch (face)
			{
				case GL_FRONT:
					param[0] = l->ambient[0].r;
					param[1] = l->ambient[0].g;
					param[2] = l->ambient[0].b;
					param[3] = l->ambient[0].a;
					break;
				case GL_BACK:
					param[0] = l->ambient[1].r;
					param[1] = l->ambient[1].g;
					param[2] = l->ambient[1].b;
					param[3] = l->ambient[1].a;
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
							"glGetMaterialfv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_DIFFUSE:
			switch (face)
			{
				case GL_FRONT:
					param[0] = l->diffuse[0].r;
					param[1] = l->diffuse[0].g;
					param[2] = l->diffuse[0].b;
					param[3] = l->diffuse[0].a;
					break;
				case GL_BACK:
					param[0] = l->diffuse[1].r;
					param[1] = l->diffuse[1].g;
					param[2] = l->diffuse[1].b;
					param[3] = l->diffuse[1].a;
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
							"glGetMaterialfv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_SPECULAR :
			switch (face)
			{
				case GL_FRONT:
					param[0] = l->specular[0].r;
					param[1] = l->specular[0].g;
					param[2] = l->specular[0].b;
					param[3] = l->specular[0].a;
					break;
				case GL_BACK:
					param[0] = l->specular[1].r;
					param[1] = l->specular[1].g;
					param[2] = l->specular[1].b;
					param[3] = l->specular[1].a;
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
							"glGetMaterialfv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_EMISSION:
			switch (face)
			{
				case GL_FRONT:
					param[0] = l->emission[0].r;
					param[1] = l->emission[0].g;
					param[2] = l->emission[0].b;
					param[3] = l->emission[0].a;
					break;
				case GL_BACK:
					param[0] = l->emission[1].r;
					param[1] = l->emission[1].g;
					param[2] = l->emission[1].b;
					param[3] = l->emission[1].a;
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
							"glGetMaterialfv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_SHININESS:
			switch (face)
			{
				case GL_FRONT:
					*param = l->shininess[0];
					break;
				case GL_BACK:
					*param = l->shininess[1];
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
							"glGetMaterialfv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_COLOR_INDEXES :
			switch (face)
			{
				case GL_FRONT:
					param[0] = (GLfloat) l->indexes[0][0];
					param[1] = (GLfloat) l->indexes[0][1];
					param[2] = (GLfloat) l->indexes[0][2];
					break;
				case GL_BACK:
					param[0] = (GLfloat) l->indexes[1][0];
					param[1] = (GLfloat) l->indexes[1][1];
					param[2] = (GLfloat) l->indexes[1][2];
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
							"glGetMaterialfv: bad face: 0x%x", face);
					return;
			}
			return;
		default:
			crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
					"glGetMaterialfv: bad pname: 0x%x", pname);
			return;
	}
}


void STATE_APIENTRY crStateGetMaterialiv (PCRStateTracker pState, GLenum face, GLenum pname, GLint *param)
{
	CRContext *g = GetCurrentContext(pState);
	CRLightingState *l = &(g->lighting);

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION,
				"glGetMaterialiv called in begin/end");
		return;
	}

	switch (pname)
	{
		case GL_AMBIENT:
			switch (face)
			{
				case GL_FRONT:
					param[0] = (GLint) (l->ambient[0].r * CR_MAXINT);
					param[1] = (GLint) (l->ambient[0].g * CR_MAXINT);
					param[2] = (GLint) (l->ambient[0].b * CR_MAXINT);
					param[3] = (GLint) (l->ambient[0].a * CR_MAXINT);
					break;
				case GL_BACK:
					param[0] = (GLint) (l->ambient[1].r * CR_MAXINT);
					param[1] = (GLint) (l->ambient[1].g * CR_MAXINT);
					param[2] = (GLint) (l->ambient[1].b * CR_MAXINT);
					param[3] = (GLint) (l->ambient[1].a * CR_MAXINT);
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
							"glGetMaterialiv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_DIFFUSE:
			switch (face)
			{
				case GL_FRONT:
					param[0] = (GLint) (l->diffuse[0].r * CR_MAXINT);
					param[1] = (GLint) (l->diffuse[0].g * CR_MAXINT);
					param[2] = (GLint) (l->diffuse[0].b * CR_MAXINT);
					param[3] = (GLint) (l->diffuse[0].a * CR_MAXINT);
					break;
				case GL_BACK:
					param[0] = (GLint) (l->diffuse[1].r * CR_MAXINT);
					param[1] = (GLint) (l->diffuse[1].g * CR_MAXINT);
					param[2] = (GLint) (l->diffuse[1].b * CR_MAXINT);
					param[3] = (GLint) (l->diffuse[1].a * CR_MAXINT);
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
							"glGetMaterialiv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_SPECULAR:
			switch (face)
			{
				case GL_FRONT:
					param[0] = (GLint) (l->specular[0].r * CR_MAXINT);
					param[1] = (GLint) (l->specular[0].g * CR_MAXINT);
					param[2] = (GLint) (l->specular[0].b * CR_MAXINT);
					param[3] = (GLint) (l->specular[0].a * CR_MAXINT);
					break;
				case GL_BACK:
					param[0] = (GLint) (l->specular[1].r * CR_MAXINT);
					param[1] = (GLint) (l->specular[1].g * CR_MAXINT);
					param[2] = (GLint) (l->specular[1].b * CR_MAXINT);
					param[3] = (GLint) (l->specular[1].a * CR_MAXINT);
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
							"glGetMaterialiv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_EMISSION:
			switch (face)
			{
				case GL_FRONT:
					param[0] = (GLint) (l->emission[0].r * CR_MAXINT);
					param[1] = (GLint) (l->emission[0].g * CR_MAXINT);
					param[2] = (GLint) (l->emission[0].b * CR_MAXINT);
					param[3] = (GLint) (l->emission[0].a * CR_MAXINT);
					break;
				case GL_BACK:
					param[0] = (GLint) (l->emission[1].r * CR_MAXINT);
					param[1] = (GLint) (l->emission[1].g * CR_MAXINT);
					param[2] = (GLint) (l->emission[1].b * CR_MAXINT);
					param[3] = (GLint) (l->emission[1].a * CR_MAXINT);
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
							"glGetMaterialiv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_SHININESS:
			switch (face) {
				case GL_FRONT:
					*param = (GLint) l->shininess[0];
					break;
				case GL_BACK:
					*param = (GLint) l->shininess[1];
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
							"glGetMaterialiv: bad face: 0x%x", face);
					return;
			}
			break;
		case GL_COLOR_INDEXES :
			switch (face)
			{
				case GL_FRONT:
					param[0] = (GLint) l->indexes[0][0];
					param[1] = (GLint) l->indexes[0][1];
					param[2] = (GLint) l->indexes[0][2];
					break;
				case GL_BACK:
					param[0] = (GLint) l->indexes[1][0];
					param[1] = (GLint) l->indexes[1][1];
					param[2] = (GLint) l->indexes[1][2];
					break;
				default:
					crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
							"glGetMaterialiv: bad face: 0x%x", face);
					return;
			}
			return;
		default:
			crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
					"glGetMaterialiv: bad pname: 0x%x", pname);
			return;
	}
}

void crStateColorMaterialRecover(PCRStateTracker pState)
{
	CRContext *g = GetCurrentContext(pState);
	CRLightingState *l = &(g->lighting);
	CRCurrentState *c = &(g->current);

	/* Assuming that the "current" values are up to date,
	 * this function will extract them into the material
	 * values if COLOR_MATERIAL has been enabled on the
	 * client. */

	if (l->colorMaterial)
	{
		/* prevent recursion here (was in tilesortspu_flush.c's doFlush() for a
		 * short time.  Without this, kirchner_colormaterial fails.
		 */
		crStateFlushFunc(pState, NULL);

		crStateMaterialfv(pState, l->colorMaterialFace, l->colorMaterialMode, &(c->vertexAttrib[VERT_ATTRIB_COLOR0][0]));
	}
}
