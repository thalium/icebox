/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"

void crStateFogInit (CRContext *ctx)
{
	CRFogState *f = &ctx->fog;
	CRStateBits *sb = GetCurrentBits();
	CRFogBits *fb = &(sb->fog);
	GLcolorf black = {0.0f, 0.0f, 0.0f, 0.0f};

	f->color = black;
	RESET(fb->color, ctx->bitid);
	f->density = 1.0f;
	RESET(fb->density, ctx->bitid);
	f->end = 1.0f;
	RESET(fb->end, ctx->bitid);
	f->start = 0.0f;
	RESET(fb->start, ctx->bitid);
	f->mode = GL_EXP;
	RESET(fb->mode, ctx->bitid);
	f->index = 0;
	RESET(fb->index, ctx->bitid);
	f->enable = GL_FALSE;
	RESET(fb->enable, ctx->bitid);

#ifdef CR_NV_fog_distance
	f->fogDistanceMode = GL_EYE_PLANE_ABSOLUTE_NV;
	RESET(fb->fogDistanceMode, ctx->bitid);
#endif
#ifdef CR_EXT_fog_coord
	f->fogCoordinateSource = GL_FRAGMENT_DEPTH_EXT;
	RESET(fb->fogCoordinateSource, ctx->bitid);
#endif
	RESET(fb->dirty, ctx->bitid);
}

void STATE_APIENTRY crStateFogf(GLenum pname, GLfloat param) 
{
	crStateFogfv( pname, &param );
}

void STATE_APIENTRY crStateFogi(GLenum pname, GLint param) 
{
	GLfloat f_param = (GLfloat) param;
	crStateFogfv( pname, &f_param );
}

void STATE_APIENTRY crStateFogiv(GLenum pname, const GLint *param) 
{
	GLcolor f_color;
	GLfloat f_param;
	switch (pname) 
	{
		case GL_FOG_MODE:
		case GL_FOG_DENSITY:
		case GL_FOG_START:
		case GL_FOG_END:
		case GL_FOG_INDEX:
			f_param = (GLfloat) (*param);
			crStateFogfv( pname, &f_param );
			break;
		case GL_FOG_COLOR:
			f_color.r = ((GLfloat) param[0]) / ((GLfloat) CR_MAXINT);
			f_color.g = ((GLfloat) param[1]) / ((GLfloat) CR_MAXINT);
			f_color.b = ((GLfloat) param[2]) / ((GLfloat) CR_MAXINT);
			f_color.a = ((GLfloat) param[3]) / ((GLfloat) CR_MAXINT);
			crStateFogfv( pname, (GLfloat *) &f_color );
			break;
#ifdef CR_NV_fog_distance
		case GL_FOG_DISTANCE_MODE_NV:
			f_param = (GLfloat) (*param);
			crStateFogfv( pname, &f_param );
			break;
#endif
#ifdef CR_EXT_fog_coord
		case GL_FOG_COORDINATE_SOURCE_EXT:
			f_param = (GLfloat) (*param);
			crStateFogfv( pname, &f_param );
			break;
#endif
		default:
			crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Invalid glFog Param: %d", param);
			return;
	}
}

void STATE_APIENTRY crStateFogfv(GLenum pname, const GLfloat *param) 
{
	CRContext *g = GetCurrentContext();
	CRFogState *f = &(g->fog);
	CRStateBits *sb = GetCurrentBits();
	CRFogBits *fb = &(sb->fog);

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glFogfv called in Begin/End");
		return;
	}

	FLUSH();

	switch (pname) 
	{
		case GL_FOG_MODE:
			{
				GLenum e = (GLenum) *param;
				if (e != GL_LINEAR && e != GL_EXP && e != GL_EXP2)
				{
					crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "Invalid param for glFog: %d", e);
					return;
				}
				f->mode = e;
				DIRTY(fb->mode, g->neg_bitid);
			}
			break;
		case GL_FOG_DENSITY:
			f->density = *param;
			if (f->density < 0.0f)
			{
				f->density = 0.0f;
			}
			DIRTY(fb->density, g->neg_bitid);
			break;
		case GL_FOG_START:
			f->start = *param;
			DIRTY(fb->start, g->neg_bitid);
			break;
		case GL_FOG_END:
			f->end = *param;
			DIRTY(fb->end, g->neg_bitid);
			break;
		case GL_FOG_INDEX:
			f->index = (GLint) *param;
			DIRTY(fb->index, g->neg_bitid);
			break;
		case GL_FOG_COLOR:
			f->color.r = param[0];
			f->color.g = param[1];
			f->color.b = param[2];
			f->color.a = param[3];
			DIRTY(fb->color, g->neg_bitid);
			break;
#ifdef CR_NV_fog_distance
		case GL_FOG_DISTANCE_MODE_NV:
			if (g->extensions.NV_fog_distance)
			{
				if (param[0] != GL_EYE_RADIAL_NV &&
					param[0] != GL_EYE_PLANE &&
					param[0] != GL_EYE_PLANE_ABSOLUTE_NV )
				{
					crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
						"Fogfv: GL_FOG_DISTANCE_MODE_NV called with illegal parameter: 0x%x", (GLenum) param[0]);
					return;
				}
				f->fogDistanceMode = (GLenum) param[0];
				DIRTY(fb->fogDistanceMode, g->neg_bitid);
			}
			else
			{
				crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Invalid glFog Param: %d", param);
				return;
			}
			break;
#endif
#ifdef CR_EXT_fog_coord
		case GL_FOG_COORDINATE_SOURCE_EXT:
			if (g->extensions.EXT_fog_coord)
			{
				if ((GLenum) param[0] != GL_FOG_COORDINATE_EXT &&
						(GLenum) param[0] != GL_FRAGMENT_DEPTH_EXT)
				{
					crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
						"Fogfv: GL_FOG_COORDINATE_SOURCE_EXT called with illegal parameter: 0x%x", (GLenum) param[0]);
					return;
				}
				f->fogCoordinateSource = (GLenum) param[0];
				DIRTY(fb->fogCoordinateSource, g->neg_bitid);
			}
			else
			{
				crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Invalid glFog Param: 0x%x", (GLint) param[0]);
				return;
			}
			break;
#endif
		default:
			crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "Invalid glFog Param: %d", param);
			return;
	}
	DIRTY(fb->dirty, g->neg_bitid);
}
