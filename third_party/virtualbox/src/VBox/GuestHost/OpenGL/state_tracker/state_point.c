/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"

void crStatePointInit (CRContext *ctx)
{
	CRPointState *p = &ctx->point;
	CRStateBits *sb = GetCurrentBits();
	CRPointBits *pb = &(sb->point);
	int i;

	p->pointSmooth = GL_FALSE;
	RESET(pb->enableSmooth, ctx->bitid);
	p->pointSize = 1.0f;
	RESET(pb->size, ctx->bitid);
#ifdef CR_ARB_point_parameters
	p->minSize = 0.0f;
	RESET(pb->minSize, ctx->bitid);
	p->maxSize = CR_ALIASED_POINT_SIZE_MAX;
	RESET(pb->maxSize, ctx->bitid);
	p->fadeThresholdSize = 1.0f;
	RESET(pb->fadeThresholdSize, ctx->bitid);
	p->distanceAttenuation[0] = 1.0f;
	p->distanceAttenuation[1] = 0.0f;
	p->distanceAttenuation[2] = 0.0f;
	RESET(pb->distanceAttenuation, ctx->bitid);
#endif
#ifdef CR_ARB_point_sprite
	p->pointSprite = GL_FALSE;
	RESET(pb->enableSprite, ctx->bitid);
	for (i = 0; i < CR_MAX_TEXTURE_UNITS; i++) {
		p->coordReplacement[i] = GL_FALSE;
		RESET(pb->coordReplacement[i], ctx->bitid);
	}
#endif

	p->spriteCoordOrigin = (GLfloat)GL_UPPER_LEFT;
	RESET(pb->spriteCoordOrigin, ctx->bitid);

	RESET(pb->dirty, ctx->bitid);

	/*
	 *p->aliasedpointsizerange_min = c->aliasedpointsizerange_min; 
	 *p->aliasedpointsizerange_max = c->aliasedpointsizerange_max; 
	 *p->aliasedpointsizegranularity = c->aliasedpointsizegranularity; 
	 *p->smoothpointsizerange_min = c->smoothpointsizerange_min; 
	 *p->smoothpointsizerange_max = c->smoothpointsizerange_max; 
	 *p->smoothpointgranularity = c->smoothpointgranularity;
	 */
}

void STATE_APIENTRY crStatePointSize(GLfloat size) 
{
	CRContext *g = GetCurrentContext();
	CRPointState *p = &(g->point);
	CRStateBits *sb = GetCurrentBits();
	CRPointBits *pb = &(sb->point);

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glPointSize called in begin/end");
		return;
	}

	FLUSH();

	if (size <= 0.0f) 
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glPointSize called with size <= 0.0: %f", size);
		return;
	}
		
	p->pointSize = size;
	DIRTY(pb->size, g->neg_bitid);
	DIRTY(pb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStatePointParameterfARB(GLenum pname, GLfloat param)
{
	CRContext *g = GetCurrentContext();

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glPointParameterfARB called in begin/end");
		return;
	}

	FLUSH();

	crStatePointParameterfvARB(pname, &param);
}

void STATE_APIENTRY crStatePointParameterfvARB(GLenum pname, const GLfloat *params)
{
	CRContext *g = GetCurrentContext();
	CRPointState *p = &(g->point);
	CRStateBits *sb = GetCurrentBits();
	CRPointBits *pb = &(sb->point);

	if (g->current.inBeginEnd)
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "glPointParameterfvARB called in begin/end");
		return;
	}

	FLUSH();

	switch (pname) {
	case GL_DISTANCE_ATTENUATION_EXT:
		if (g->extensions.ARB_point_parameters) {
			p->distanceAttenuation[0] = params[0];
			p->distanceAttenuation[1] = params[1];
			p->distanceAttenuation[2] = params[2];
			DIRTY(pb->distanceAttenuation, g->neg_bitid);
		}
		else 
		{
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glPointParameterfvARB invalid enum: %f", pname);
			return;
		}
		break;
	case GL_POINT_SIZE_MIN_EXT:
		if (g->extensions.ARB_point_parameters) {
			if (params[0] < 0.0F) {
				crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glPointParameterfvARB invalid value: %f", params[0]);
				return;
			}
            		p->minSize = params[0];
			DIRTY(pb->minSize, g->neg_bitid);
         	}
		else
		{
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glPointParameterfvARB invalid enum: %f", pname);
			return;
		}
		break;
	case GL_POINT_SIZE_MAX_EXT:
		if (g->extensions.ARB_point_parameters) {
			if (params[0] < 0.0F) {
				crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glPointParameterfvARB invalid value: %f", params[0]);
				return;
			}
            		p->maxSize = params[0];
			DIRTY(pb->maxSize, g->neg_bitid);
         	}
		else
		{
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glPointParameterfvARB invalid enum: %f", pname);
			return;
		}
		break;
	case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
		if (g->extensions.ARB_point_parameters) {
			if (params[0] < 0.0F) {
				crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glPointParameterfvARB invalid value: %f", params[0]);
				return;
			}
            		p->fadeThresholdSize = params[0];
			DIRTY(pb->fadeThresholdSize, g->neg_bitid);
         	}
		else
		{
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glPointParameterfvARB invalid enum: %f", pname);
			return;
		}
		break;
	case GL_POINT_SPRITE_COORD_ORIGIN:
	{
	    GLenum enmVal = (GLenum)params[0];
        if (enmVal != GL_LOWER_LEFT && enmVal != GL_UPPER_LEFT) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glPointParameterfvARB invalid GL_POINT_SPRITE_COORD_ORIGIN value: %f", params[0]);
            return;
        }
        p->spriteCoordOrigin = params[0];
        DIRTY(pb->spriteCoordOrigin, g->neg_bitid);
	    break;
	}
	default:
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glPointParameterfvARB invalid enum: %f", pname);
		return;
	}

	DIRTY(pb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStatePointParameteri(GLenum pname, GLint param)
{
	GLfloat f_param = (GLfloat) param;
	crStatePointParameterfvARB( pname, &f_param );
}

void STATE_APIENTRY crStatePointParameteriv(GLenum pname, const GLint *params)
{
	GLfloat f_param = (GLfloat) (*params);
	crStatePointParameterfvARB( pname, &f_param );
}

void crStatePointDiff(CRPointBits *b, CRbitvalue *bitID,
        CRContext *fromCtx, CRContext *toCtx)
{
    CRPointState *from = &(fromCtx->point);
    CRPointState *to = &(toCtx->point);
    unsigned int j, i;
    CRbitvalue nbitID[CR_MAX_BITARRAY];
    Assert(0);
    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];
    i = 0; /* silence compiler */
    if (CHECKDIRTY(b->enableSmooth, bitID))
    {
        glAble able[2];
        able[0] = diff_api.Disable;
        able[1] = diff_api.Enable;
        if (from->pointSmooth != to->pointSmooth)
        {
            able[to->pointSmooth](GL_POINT_SMOOTH);
            from->pointSmooth = to->pointSmooth;
        }
        CLEARDIRTY(b->enableSmooth, nbitID);
    }
    if (CHECKDIRTY(b->size, bitID))
    {
        if (from->pointSize != to->pointSize)
        {
            diff_api.PointSize (to->pointSize);
            from->pointSize = to->pointSize;
        }
        CLEARDIRTY(b->size, nbitID);
    }
    if (CHECKDIRTY(b->minSize, bitID))
    {
        if (from->minSize != to->minSize)
        {
            diff_api.PointParameterfARB (GL_POINT_SIZE_MIN_ARB, to->minSize);
            from->minSize = to->minSize;
        }
        CLEARDIRTY(b->minSize, nbitID);
    }
    if (CHECKDIRTY(b->maxSize, bitID))
    {
        if (from->maxSize != to->maxSize)
        {
            diff_api.PointParameterfARB (GL_POINT_SIZE_MAX_ARB, to->maxSize);
            from->maxSize = to->maxSize;
        }
        CLEARDIRTY(b->maxSize, nbitID);
    }
    if (CHECKDIRTY(b->fadeThresholdSize, bitID))
    {
        if (from->fadeThresholdSize != to->fadeThresholdSize)
        {
            diff_api.PointParameterfARB (GL_POINT_FADE_THRESHOLD_SIZE_ARB, to->fadeThresholdSize);
            from->fadeThresholdSize = to->fadeThresholdSize;
        }
        CLEARDIRTY(b->fadeThresholdSize, nbitID);
    }
    if (CHECKDIRTY(b->spriteCoordOrigin, bitID))
    {
        if (from->spriteCoordOrigin != to->spriteCoordOrigin)
        {
            diff_api.PointParameterfARB (GL_POINT_SPRITE_COORD_ORIGIN, to->spriteCoordOrigin);
            from->spriteCoordOrigin = to->spriteCoordOrigin;
        }
        CLEARDIRTY(b->spriteCoordOrigin, nbitID);
    }
    if (CHECKDIRTY(b->distanceAttenuation, bitID))
    {
        if (from->distanceAttenuation[0] != to->distanceAttenuation[0] || from->distanceAttenuation[1] != to->distanceAttenuation[1] || from->distanceAttenuation[2] != to->distanceAttenuation[2]) {
            diff_api.PointParameterfvARB (GL_POINT_DISTANCE_ATTENUATION_ARB, to->distanceAttenuation);
            from->distanceAttenuation[0] = to->distanceAttenuation[0];
            from->distanceAttenuation[1] = to->distanceAttenuation[1];
            from->distanceAttenuation[2] = to->distanceAttenuation[2];
        }
        CLEARDIRTY(b->distanceAttenuation, nbitID);
    }
    if (CHECKDIRTY(b->enableSprite, bitID))
    {
        glAble able[2];
        able[0] = diff_api.Disable;
        able[1] = diff_api.Enable;
        if (from->pointSprite != to->pointSprite)
        {
            able[to->pointSprite](GL_POINT_SPRITE_ARB);
            from->pointSprite = to->pointSprite;
        }
        CLEARDIRTY(b->enableSprite, nbitID);
    }
    {
        unsigned int activeUnit = (unsigned int) -1;
        for (i = 0; i < CR_MAX_TEXTURE_UNITS; i++) {
            if (CHECKDIRTY(b->coordReplacement[i], bitID))
            {
                GLint replacement = to->coordReplacement[i];
                if (activeUnit != i) {
                     diff_api.ActiveTextureARB(i + GL_TEXTURE0_ARB );
                     activeUnit = i;
                }
                diff_api.TexEnviv(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, &replacement);
                from->coordReplacement[i] = to->coordReplacement[i];
                CLEARDIRTY(b->coordReplacement[i], nbitID);
            }
        }
        if (activeUnit != toCtx->texture.curTextureUnit)
           diff_api.ActiveTextureARB(GL_TEXTURE0 + toCtx->texture.curTextureUnit);
    }
    CLEARDIRTY(b->dirty, nbitID);
}

void crStatePointSwitch(CRPointBits *b, CRbitvalue *bitID,
        CRContext *fromCtx, CRContext *toCtx)
{
    CRPointState *from = &(fromCtx->point);
    CRPointState *to = &(toCtx->point);
    unsigned int j, i;
    GLboolean fEnabled;
    CRbitvalue nbitID[CR_MAX_BITARRAY];
    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];
    i = 0; /* silence compiler */
    if (CHECKDIRTY(b->enableSmooth, bitID))
    {
        glAble able[2];
        able[0] = diff_api.Disable;
        able[1] = diff_api.Enable;
        if (from->pointSmooth != to->pointSmooth)
        {
            able[to->pointSmooth](GL_POINT_SMOOTH);
            FILLDIRTY(b->enableSmooth);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->enableSmooth, nbitID);
    }
    if (CHECKDIRTY(b->size, bitID))
    {
        if (from->pointSize != to->pointSize)
        {
            diff_api.PointSize (to->pointSize);
            FILLDIRTY(b->size);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->size, nbitID);
    }
    if (CHECKDIRTY(b->minSize, bitID))
    {
        if (from->minSize != to->minSize)
        {
            diff_api.PointParameterfARB (GL_POINT_SIZE_MIN_ARB, to->minSize);
            FILLDIRTY(b->minSize);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->minSize, nbitID);
    }
    if (CHECKDIRTY(b->maxSize, bitID))
    {
        if (from->maxSize != to->maxSize)
        {
            diff_api.PointParameterfARB (GL_POINT_SIZE_MAX_ARB, to->maxSize);
            FILLDIRTY(b->maxSize);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->maxSize, nbitID);
    }
    if (CHECKDIRTY(b->fadeThresholdSize, bitID))
    {
        if (from->fadeThresholdSize != to->fadeThresholdSize)
        {
            diff_api.PointParameterfARB (GL_POINT_FADE_THRESHOLD_SIZE_ARB, to->fadeThresholdSize);
            FILLDIRTY(b->fadeThresholdSize);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->fadeThresholdSize, nbitID);
    }
    if (CHECKDIRTY(b->spriteCoordOrigin, bitID))
    {
        if (from->spriteCoordOrigin != to->spriteCoordOrigin)
        {
            diff_api.PointParameterfARB (GL_POINT_SPRITE_COORD_ORIGIN, to->spriteCoordOrigin);
            FILLDIRTY(b->spriteCoordOrigin);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->spriteCoordOrigin, nbitID);
    }
    if (CHECKDIRTY(b->distanceAttenuation, bitID))
    {
        if (from->distanceAttenuation[0] != to->distanceAttenuation[0] || from->distanceAttenuation[1] != to->distanceAttenuation[1] || from->distanceAttenuation[2] != to->distanceAttenuation[2]) {
            diff_api.PointParameterfvARB (GL_POINT_DISTANCE_ATTENUATION_ARB, to->distanceAttenuation);
            FILLDIRTY(b->distanceAttenuation);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->distanceAttenuation, nbitID);
    }
    fEnabled = from->pointSprite;
    {
        unsigned int activeUnit = (unsigned int) -1;
        for (i = 0; i < CR_MAX_TEXTURE_UNITS; i++) {
            if (CHECKDIRTY(b->coordReplacement[i], bitID))
            {
                if (!fEnabled)
                {
                    diff_api.Enable(GL_POINT_SPRITE_ARB);
                    fEnabled = GL_TRUE;
                }
#if 0
                /*don't set coord replacement, it will be set just before drawing points when necessary,
                 * to work around gpu driver bugs
                 * See crServerDispatch[Begin|End|Draw*] */
                GLint replacement = to->coordReplacement[i];
                if (activeUnit != i) {
                     diff_api.ActiveTextureARB(i + GL_TEXTURE0_ARB );
                     activeUnit = i;
                }
                diff_api.TexEnviv(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, &replacement);
#endif
                CLEARDIRTY(b->coordReplacement[i], nbitID);
            }
        }
        if (activeUnit != toCtx->texture.curTextureUnit)
           diff_api.ActiveTextureARB(GL_TEXTURE0 + toCtx->texture.curTextureUnit);
    }
    if (CHECKDIRTY(b->enableSprite, bitID))
    {
        glAble able[2];
        able[0] = diff_api.Disable;
        able[1] = diff_api.Enable;
        if (fEnabled != to->pointSprite)
        {
            able[to->pointSprite](GL_POINT_SPRITE_ARB);
            FILLDIRTY(b->enableSprite);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->enableSprite, nbitID);
    }
    else if (fEnabled != to->pointSprite)
    {
        glAble able[2];
        able[0] = diff_api.Disable;
        able[1] = diff_api.Enable;
        able[to->pointSprite](GL_POINT_SPRITE_ARB);
    }
    CLEARDIRTY(b->dirty, nbitID);
}
