/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "state.h"
#include "state/cr_statetypes.h"


#define UNUSED(x) ((void) (x))


void crStateRegCombinerInit( CRContext *ctx )
{
	CRRegCombinerState *reg = &ctx->regcombiner;
	CRStateBits *sb = GetCurrentBits();
	CRRegCombinerBits *rb = &(sb->regcombiner);
#ifndef CR_NV_register_combiners
	UNUSED(reg)
#else
	GLcolorf zero_color = {0.0f, 0.0f, 0.0f, 0.0f};
	int i;

	reg->enabledRegCombiners = GL_FALSE;
	RESET(rb->enable, ctx->bitid);
	reg->constantColor0 = zero_color;
	RESET(rb->regCombinerColor0, ctx->bitid);
	reg->constantColor1 = zero_color;
	RESET(rb->regCombinerColor1, ctx->bitid);
	for( i=0; i<CR_MAX_GENERAL_COMBINERS; i++ )
	{
	  /* RGB Portion */
		reg->rgb[i].a = GL_PRIMARY_COLOR_NV;
		reg->rgb[i].b = GL_ZERO;
		reg->rgb[i].c = GL_ZERO;
		reg->rgb[i].d = GL_ZERO;
		reg->rgb[i].aMapping = GL_UNSIGNED_IDENTITY_NV;
		reg->rgb[i].bMapping = GL_UNSIGNED_INVERT_NV;
		reg->rgb[i].cMapping = GL_UNSIGNED_IDENTITY_NV;
		reg->rgb[i].dMapping = GL_UNSIGNED_IDENTITY_NV;
		reg->rgb[i].aPortion = GL_RGB;
		reg->rgb[i].bPortion = GL_RGB;
		reg->rgb[i].cPortion = GL_RGB;
		reg->rgb[i].dPortion = GL_RGB;
		reg->rgb[i].scale = GL_NONE;
		reg->rgb[i].bias = GL_NONE;
		reg->rgb[i].abOutput = GL_DISCARD_NV;
		reg->rgb[i].cdOutput = GL_DISCARD_NV;
		reg->rgb[i].sumOutput = GL_SPARE0_NV;
		reg->rgb[i].abDotProduct = GL_FALSE;
		reg->rgb[i].cdDotProduct = GL_FALSE;
		reg->rgb[i].muxSum = GL_FALSE;

		/* Alpha Portion */
		reg->alpha[i].a = GL_PRIMARY_COLOR_NV;
		reg->alpha[i].b = GL_ZERO;
		reg->alpha[i].c = GL_ZERO;
		reg->alpha[i].d = GL_ZERO;
		reg->alpha[i].aMapping = GL_UNSIGNED_IDENTITY_NV;
		reg->alpha[i].bMapping = GL_UNSIGNED_INVERT_NV;
		reg->alpha[i].cMapping = GL_UNSIGNED_IDENTITY_NV;
		reg->alpha[i].dMapping = GL_UNSIGNED_IDENTITY_NV;
		reg->alpha[i].aPortion = GL_ALPHA;
		reg->alpha[i].bPortion = GL_ALPHA;
		reg->alpha[i].cPortion = GL_ALPHA;
		reg->alpha[i].dPortion = GL_ALPHA;
		reg->alpha[i].scale = GL_NONE;
		reg->alpha[i].bias = GL_NONE;
		reg->alpha[i].abOutput = GL_DISCARD_NV;
		reg->alpha[i].cdOutput = GL_DISCARD_NV;
		reg->alpha[i].sumOutput = GL_SPARE0_NV;
		reg->alpha[i].abDotProduct = GL_FALSE;
		reg->alpha[i].cdDotProduct = GL_FALSE;
		reg->alpha[i].muxSum = GL_FALSE;
		RESET(rb->regCombinerInput[i], ctx->bitid);
		RESET(rb->regCombinerOutput[i], ctx->bitid);
	}
	RESET(rb->regCombinerVars, ctx->bitid);
	reg->numGeneralCombiners = 1;
	reg->colorSumClamp = GL_TRUE;
	reg->a = GL_FOG;
	reg->b = GL_SPARE0_PLUS_SECONDARY_COLOR_NV;
	reg->c = GL_FOG;
	reg->d = GL_ZERO;
	reg->e = GL_ZERO;
	reg->f = GL_ZERO;
	reg->g = GL_SPARE0_NV;
	reg->aMapping = GL_UNSIGNED_IDENTITY_NV;
	reg->bMapping = GL_UNSIGNED_IDENTITY_NV;
	reg->cMapping = GL_UNSIGNED_IDENTITY_NV;
	reg->dMapping = GL_UNSIGNED_IDENTITY_NV;
	reg->eMapping = GL_UNSIGNED_IDENTITY_NV;
	reg->fMapping = GL_UNSIGNED_IDENTITY_NV;
	reg->gMapping = GL_UNSIGNED_IDENTITY_NV;
	reg->aPortion = GL_ALPHA;
	reg->bPortion = GL_RGB;
	reg->cPortion = GL_RGB;
	reg->dPortion = GL_RGB;
	reg->ePortion = GL_RGB;
	reg->fPortion = GL_RGB;
	reg->gPortion = GL_ALPHA;
	RESET(rb->regCombinerFinalInput, ctx->bitid);
#ifdef CR_NV_register_combiners2
	reg->enabledPerStageConstants = GL_FALSE;
	for( i=0; i<CR_MAX_GENERAL_COMBINERS; i++ )
	{
		reg->stageConstantColor0[i] = zero_color;
		reg->stageConstantColor1[i] = zero_color;
		RESET(rb->regCombinerStageColor0[i], ctx->bitid);
		RESET(rb->regCombinerStageColor1[i], ctx->bitid);
	}
#endif /* CR_NV_register_combiners2 */
#endif /* CR_NV_register_combiners */

	RESET(rb->dirty, ctx->bitid);
}

void STATE_APIENTRY crStateCombinerParameterfvNV( GLenum pname, const GLfloat *params )
{
	CRContext *g = GetCurrentContext();
	CRRegCombinerState *r = &(g->regcombiner);
	CRStateBits *sb = GetCurrentBits();
	CRRegCombinerBits *rb = &(sb->regcombiner);

	switch( pname )
	{
		case GL_CONSTANT_COLOR0_NV:
			r->constantColor0.r = params[0];
			r->constantColor0.g = params[1];
			r->constantColor0.b = params[2];
			r->constantColor0.a = params[3];
			DIRTY(rb->regCombinerColor0, g->neg_bitid);
			break;
		case GL_CONSTANT_COLOR1_NV:
			r->constantColor1.r = params[0];
			r->constantColor1.g = params[1];
			r->constantColor1.b = params[2];
			r->constantColor1.a = params[3];
			DIRTY(rb->regCombinerColor1, g->neg_bitid);
			break;
		case GL_NUM_GENERAL_COMBINERS_NV:
			if( *params < 1 || *params > g->limits.maxGeneralCombiners )
			{
				crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "CombinerParameter passed invalid NUM_GENERAL_COMBINERS param: %d", (GLint)*params );
				return;
			}
			r->numGeneralCombiners = (GLint)*params;
			DIRTY(rb->regCombinerVars, g->neg_bitid);
			break;
		case GL_COLOR_SUM_CLAMP_NV:
			r->colorSumClamp = (GLboolean)*params;
			DIRTY(rb->regCombinerVars, g->neg_bitid);
			break;
		default:
			crStateError( __LINE__, __FILE__, GL_INVALID_ENUM, "CombinerParameter passed bogus pname: 0x%x", pname );
			return;
	}

	DIRTY(rb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateCombinerParameterivNV( GLenum pname, const GLint *params )
{
	GLfloat fparams[4];
	int i;

	if( pname == GL_CONSTANT_COLOR0_NV || pname == GL_CONSTANT_COLOR1_NV )
	{
		for( i=0; i<4; i++ )
		{
			fparams[i] = (GLfloat)params[i] * (GLfloat)(1.0/255.0);
		}
	}
	else
	{
		/* Only one parameter: */
		*fparams = (GLfloat) *params;
	}
	crStateCombinerParameterfvNV( pname, fparams );
}

void STATE_APIENTRY crStateCombinerParameterfNV( GLenum pname, GLfloat param )
{
	GLfloat fparam[1];
	*fparam = (GLfloat) param;
	if( pname == GL_CONSTANT_COLOR0_NV || pname == GL_CONSTANT_COLOR1_NV )
	{
		crStateError( __LINE__, __FILE__, GL_INVALID_ENUM, "Invalid pname (CONSTANT_COLOR%d) passed to CombinerParameterfNV: 0x%x", (GLint)param-GL_CONSTANT_COLOR0_NV, pname );
		return;
	}
	crStateCombinerParameterfvNV( pname, fparam );
}

void STATE_APIENTRY crStateCombinerParameteriNV( GLenum pname, GLint param )
{
	GLfloat fparam[1];
	*fparam = (GLfloat) param;
	if( pname == GL_CONSTANT_COLOR0_NV || pname == GL_CONSTANT_COLOR1_NV )
	{
		crStateError( __LINE__, __FILE__, GL_INVALID_ENUM, "Invalid pname (CONSTANT_COLOR%d) passed to CombinerParameteriNV: 0x%x", param-GL_CONSTANT_COLOR0_NV, pname );
		return;
	}
	crStateCombinerParameterfvNV( pname, fparam );
}

void STATE_APIENTRY crStateCombinerInputNV( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage )
{
	CRContext *g = GetCurrentContext();
	CRRegCombinerState *r = &(g->regcombiner);
	CRStateBits *sb = GetCurrentBits();
	CRRegCombinerBits *rb = &(sb->regcombiner);

	if( stage < GL_COMBINER0_NV || stage >= GL_COMBINER0_NV + g->limits.maxGeneralCombiners )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerInputNV passed bogus stage: 0x%x", stage );
		return;
	}
	if( input != GL_ZERO && input != GL_CONSTANT_COLOR0_NV && input != GL_CONSTANT_COLOR1_NV && input != GL_FOG && input != GL_PRIMARY_COLOR_NV && input != GL_SECONDARY_COLOR_NV && input != GL_SPARE0_NV && input != GL_SPARE1_NV && ( input < GL_TEXTURE0_ARB || input >= GL_TEXTURE0_ARB + g->limits.maxTextureUnits ))
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerInputNV passed bogus input: 0x%x", input );
		return;
	}
	if( mapping != GL_UNSIGNED_IDENTITY_NV && mapping != GL_UNSIGNED_INVERT_NV && mapping != GL_EXPAND_NORMAL_NV && mapping != GL_EXPAND_NEGATE_NV && mapping != GL_HALF_BIAS_NORMAL_NV && mapping != GL_HALF_BIAS_NEGATE_NV && mapping != GL_SIGNED_IDENTITY_NV && mapping != GL_SIGNED_NEGATE_NV )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerInputNV passed bogus mapping: 0x%x", mapping );
		return;
	}
	if( componentUsage != GL_RGB && componentUsage != GL_ALPHA && componentUsage != GL_BLUE )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerInputNV passed bogus componentUsage: 0x%x", componentUsage );
		return;
	}

	if(( componentUsage == GL_RGB && portion == GL_ALPHA )||( componentUsage == GL_BLUE && portion == GL_RGB ))
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "Incompatible portion and componentUsage passed to CombinerInputNV: portion = 0x%x, componentUsage = 0x%x", portion, componentUsage );
		return;
	}
	if( componentUsage == GL_ALPHA && input == GL_FOG )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "CombinerInputNV can not have input of GL_FOG if componentUsage is GL_ALPHA" );
		return;
	}

	stage -= GL_COMBINER0_NV;
	if( portion == GL_RGB )
	{
		switch( variable )
		{
			case GL_VARIABLE_A_NV:
				r->rgb[stage].a = input;
				r->rgb[stage].aMapping = mapping;
				r->rgb[stage].aPortion = componentUsage;
				break;
			case GL_VARIABLE_B_NV:
				r->rgb[stage].b = input;
				r->rgb[stage].bMapping = mapping;
				r->rgb[stage].bPortion = componentUsage;
				break;
			case GL_VARIABLE_C_NV:
				r->rgb[stage].c = input;
				r->rgb[stage].cMapping = mapping;
				r->rgb[stage].cPortion = componentUsage;
				break;
			case GL_VARIABLE_D_NV:
				r->rgb[stage].d = input;
				r->rgb[stage].dMapping = mapping;
				r->rgb[stage].dPortion = componentUsage;
				break;
			default:
				crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerInputNV passed bogus variable: 0x%x", variable );
				return;
		}
	}
	else if( portion == GL_ALPHA )
	{
		switch( variable )
		{
			case GL_VARIABLE_A_NV:
				r->alpha[stage].a = input;
				r->alpha[stage].aMapping = mapping;
				r->alpha[stage].aPortion = componentUsage;
				break;
			case GL_VARIABLE_B_NV:
				r->alpha[stage].b = input;
				r->alpha[stage].bMapping = mapping;
				r->alpha[stage].bPortion = componentUsage;
				break;
			case GL_VARIABLE_C_NV:
				r->alpha[stage].c = input;
				r->alpha[stage].cMapping = mapping;
				r->alpha[stage].cPortion = componentUsage;
				break;
			case GL_VARIABLE_D_NV:
				r->alpha[stage].d = input;
				r->alpha[stage].dMapping = mapping;
				r->alpha[stage].dPortion = componentUsage;
				break;
			default:
				crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerInputNV passed bogus variable: 0x%x", variable );
				return;
		}
	}
	else
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerInputNV passed bogus portion: 0x%x", portion );
		return;
	}

	DIRTY(rb->regCombinerInput[stage], g->neg_bitid);
	DIRTY(rb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateCombinerOutputNV( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum )
{
	CRContext *g = GetCurrentContext();
	CRRegCombinerState *r = &(g->regcombiner);
	CRStateBits *sb = GetCurrentBits();
	CRRegCombinerBits *rb = &(sb->regcombiner);

	/*
	  crDebug("%s(stage=0x%x portion=0x%x abOutput=0x%x cdOutput=0x%x "
               "sumOutput=0x%x scale=0x%x bias=0x%x abDotProduct=0x%x "
               "cdDotProduct=%d muxSum=%d)\n",
               __FUNCTION__,
               stage, portion, abOutput, cdOutput, sumOutput, scale, bias,
               abDotProduct, cdDotProduct, muxSum);
	*/
	if( stage < GL_COMBINER0_NV || stage >= GL_COMBINER0_NV + g->limits.maxGeneralCombiners )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerOutputNV passed bogus stage: 0x%x", stage );
		return;
	}
	if( abOutput != GL_DISCARD_NV && abOutput != GL_PRIMARY_COLOR_NV && abOutput != GL_SECONDARY_COLOR_NV && abOutput != GL_SPARE0_NV && abOutput != GL_SPARE1_NV && ( abOutput < GL_TEXTURE0_ARB || abOutput >= g->limits.maxTextureUnits + GL_TEXTURE0_ARB ))
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerOutputNV passed bogus abOutput: 0x%x", abOutput );
		return;
	}
	if( cdOutput != GL_DISCARD_NV && cdOutput != GL_PRIMARY_COLOR_NV && cdOutput != GL_SECONDARY_COLOR_NV && cdOutput != GL_SPARE0_NV && cdOutput != GL_SPARE1_NV && ( cdOutput < GL_TEXTURE0_ARB || cdOutput >= g->limits.maxTextureUnits + GL_TEXTURE0_ARB ))
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerOutputNV passed bogus cdOutput: 0x%x", cdOutput );
		return;
	}
	if( sumOutput != GL_DISCARD_NV && sumOutput != GL_PRIMARY_COLOR_NV && sumOutput != GL_SECONDARY_COLOR_NV && sumOutput != GL_SPARE0_NV && sumOutput != GL_SPARE1_NV && sumOutput != GL_TEXTURE0_ARB && sumOutput != GL_TEXTURE1_ARB )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerOutputNV passed bogus sumOutput: 0x%x", sumOutput );
		return;
	}
	if( scale != GL_NONE && scale != GL_SCALE_BY_TWO_NV && scale != GL_SCALE_BY_FOUR_NV && scale != GL_SCALE_BY_ONE_HALF_NV )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "CombinerOutputNV passed bogus scale: 0x%x", scale );
		return;
	}
	if( bias != GL_NONE && bias != GL_BIAS_BY_NEGATIVE_ONE_HALF_NV )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "CombinerOutputNV passed bogus bias: 0x%x", bias );
		return;
	}

	if( bias == GL_BIAS_BY_NEGATIVE_ONE_HALF_NV && ( scale == GL_SCALE_BY_ONE_HALF_NV || scale == GL_SCALE_BY_FOUR_NV ))
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "CombinerOutputNV can't accept bias of -1/2 if scale is by 1/2 or 4" );
		return;
	}
	if(( abOutput == cdOutput && abOutput != GL_DISCARD_NV )||( abOutput == sumOutput && abOutput != GL_DISCARD_NV )||( cdOutput == sumOutput && cdOutput != GL_DISCARD_NV ))
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "CombinerOutputNV register output names must be unique unless discarded: abOutput = 0x%x, cdOutput = 0x%x, sumOutput = 0x%x", abOutput, cdOutput, sumOutput );
		return;
	}
	if( abDotProduct || cdDotProduct )
	{
		if( portion == GL_ALPHA )
		{
			crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "CombinerOutputNV can not do Dot Products when portion = GL_ALPHA" );
			return;
		}
		if( sumOutput != GL_DISCARD_NV )
		{
			crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "CombinerOutputNV can not do Dot Products when sumOutput is not discarded" );
			return;
		}
	}

	stage -= GL_COMBINER0_NV;
	if( portion == GL_RGB )
	{
		r->rgb[stage].abOutput = abOutput;
		r->rgb[stage].cdOutput = cdOutput;
		r->rgb[stage].sumOutput = sumOutput;
		r->rgb[stage].scale = scale;
		r->rgb[stage].bias = bias;
		r->rgb[stage].abDotProduct = abDotProduct;
		r->rgb[stage].cdDotProduct = cdDotProduct;
		r->rgb[stage].muxSum = muxSum;
	}
	else if( portion == GL_ALPHA )
	{
		r->alpha[stage].abOutput = abOutput;
		r->alpha[stage].cdOutput = cdOutput;
		r->alpha[stage].sumOutput = sumOutput;
		r->alpha[stage].scale = scale;
		r->alpha[stage].bias = bias;
		r->alpha[stage].abDotProduct = abDotProduct;
		r->alpha[stage].cdDotProduct = cdDotProduct;
		r->alpha[stage].muxSum = muxSum;
	}
	else
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerOutputNV passed bogus portion: 0x%x", portion );
		return;
	}

	DIRTY(rb->regCombinerOutput[stage], g->neg_bitid);
	DIRTY(rb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateFinalCombinerInputNV( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage )
{
	CRContext *g = GetCurrentContext();
	CRRegCombinerState *r = &(g->regcombiner);
	CRStateBits *sb = GetCurrentBits();
	CRRegCombinerBits *rb = &(sb->regcombiner);

	if( input != GL_ZERO && input != GL_CONSTANT_COLOR0_NV && input != GL_CONSTANT_COLOR1_NV && input != GL_FOG && input != GL_PRIMARY_COLOR_NV && input != GL_SECONDARY_COLOR_NV && input != GL_SPARE0_NV && input != GL_SPARE1_NV && ( input < GL_TEXTURE0_ARB || input >= GL_TEXTURE0_ARB + g->limits.maxTextureUnits ) && input != GL_E_TIMES_F_NV && input != GL_SPARE0_PLUS_SECONDARY_COLOR_NV )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "FinalCombinerInputNV passed bogus input: 0x%x", input );
		return;
	}
	if( mapping != GL_UNSIGNED_IDENTITY_NV && mapping != GL_UNSIGNED_INVERT_NV && mapping != GL_EXPAND_NORMAL_NV && mapping != GL_EXPAND_NEGATE_NV && mapping != GL_HALF_BIAS_NORMAL_NV && mapping != GL_HALF_BIAS_NEGATE_NV && mapping != GL_SIGNED_IDENTITY_NV && mapping != GL_SIGNED_NEGATE_NV )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "FinalCombinerInputNV passed bogus mapping: 0x%x", mapping );
		return;
	}
	if( componentUsage != GL_RGB && componentUsage != GL_ALPHA && componentUsage != GL_BLUE )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "FinalCombinerInputNV passed bogus componentUsage: 0x%x", componentUsage );
		return;
	}

	if( componentUsage == GL_ALPHA && ( input == GL_E_TIMES_F_NV || input == GL_SPARE0_PLUS_SECONDARY_COLOR_NV ))
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION, "FinalCombinerInputNV does not allow componentUsage of ALPHA when input is E_TIMES_F or SPARE0_PLUS_SECONDARY_COLOR" );
		return;
	}

	switch( variable )
	{
		case GL_VARIABLE_A_NV:
			r->a = input;
			r->aMapping = mapping;
			r->aPortion = componentUsage;
			break;
		case GL_VARIABLE_B_NV:
			r->b = input;
			r->bMapping = mapping;
			r->bPortion = componentUsage;
			break;
		case GL_VARIABLE_C_NV:
			r->c = input;
			r->cMapping = mapping;
			r->cPortion = componentUsage;
			break;
		case GL_VARIABLE_D_NV:
			r->d = input;
			r->dMapping = mapping;
			r->dPortion = componentUsage;
			break;
		case GL_VARIABLE_E_NV:
			r->e = input;
			r->eMapping = mapping;
			r->ePortion = componentUsage;
			break;
		case GL_VARIABLE_F_NV:
			r->f = input;
			r->fMapping = mapping;
			r->fPortion = componentUsage;
			break;
		case GL_VARIABLE_G_NV:
			if( componentUsage != GL_ALPHA )
			{
				crStateError( __LINE__, __FILE__, GL_INVALID_OPERATION, "FinalCombinerInputNV can not have variable G when componentUsage is RGB or BLUE" );
				return;
			}
			r->g = input;
			r->gMapping = mapping;
			r->gPortion = componentUsage;
			break;
		default:
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerOutputNV passed bogus variable: 0x%x", variable );
			return;
	}

	DIRTY(rb->regCombinerFinalInput, g->neg_bitid);
	DIRTY(rb->dirty, g->neg_bitid);
}


/* XXX Unfinished RegCombiner State functions */
void STATE_APIENTRY crStateGetCombinerOutputParameterfvNV( GLenum stage, GLenum portion, GLenum pname, GLfloat *params )
{
	(void) stage;
	(void) portion;
	(void) pname;
	(void) params;
}

void STATE_APIENTRY crStateGetCombinerOutputParameterivNV( GLenum stage, GLenum portion, GLenum pname, GLint *params )
{
	(void) stage;
	(void) portion;
	(void) pname;
	(void) params;
}

void STATE_APIENTRY crStateGetFinalCombinerInputParameterfvNV( GLenum variable, GLenum pname, GLfloat *params )
{
	(void) variable;
	(void) pname;
	(void) params;
}

void STATE_APIENTRY crStateGetFinalCombinerInputParameterivNV( GLenum variable, GLenum pname, GLint *params )
{
	(void) variable;
	(void) pname;
	(void) params;
}


void STATE_APIENTRY crStateGetCombinerInputParameterivNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params )
{
	CRContext *g = GetCurrentContext();
	CRRegCombinerState *r = &(g->regcombiner);
	int i = stage - GL_COMBINER0_NV;
	GLenum input = 0, mapping = 0, usage = 0;

	if (g->current.inBeginEnd) 
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
				"glGetCombinerParameterivNV called in begin/end");
		return;
	}

	if (i < 0 || i >= CR_MAX_GENERAL_COMBINERS) {
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
								 "GetCombinerInputParameterivNV(stage=0x%x)", stage);
		return;
	}

	if (portion == GL_RGB) {
		switch (variable) {
		case GL_VARIABLE_A_NV:
			input = r->rgb[i].a;
			mapping = r->rgb[i].aMapping;
			usage = r->rgb[i].aPortion;
			break;
		case GL_VARIABLE_B_NV:
			input = r->rgb[i].b;
			mapping = r->rgb[i].bMapping;
			usage = r->rgb[i].bPortion;
			break;
		case GL_VARIABLE_C_NV:
			input = r->rgb[i].c;
			mapping = r->rgb[i].cMapping;
			usage = r->rgb[i].cPortion;
			break;
		case GL_VARIABLE_D_NV:
			input = r->rgb[i].d;
			mapping = r->rgb[i].dMapping;
			usage = r->rgb[i].dPortion;
			break;
		default:
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
									 "glGetCombinerInputParameterivNV(variable=0x%x)", variable);
			return;
		}
	}
	else if (portion == GL_ALPHA) {
		switch (variable) {
		case GL_VARIABLE_A_NV:
			input = r->alpha[i].a;
			mapping = r->alpha[i].aMapping;
			usage = r->alpha[i].aPortion;
			break;
		case GL_VARIABLE_B_NV:
			input = r->alpha[i].b;
			mapping = r->alpha[i].bMapping;
			usage = r->alpha[i].bPortion;
			break;
		case GL_VARIABLE_C_NV:
			input = r->alpha[i].c;
			mapping = r->alpha[i].cMapping;
			usage = r->alpha[i].cPortion;
			break;
		case GL_VARIABLE_D_NV:
			input = r->alpha[i].d;
			mapping = r->alpha[i].dMapping;
			usage = r->alpha[i].dPortion;
			break;
		default:
			crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
									 "glGetCombinerInputParameterivNV(variable=0x%x)", variable);
			return;
		}
	}
	else {
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
								 "glGetCombinerInputParameterivNV(portion=0x%x)", portion);
	}
	switch (pname) {
	case GL_COMBINER_INPUT_NV:
		*params = input;
		return;
	case GL_COMBINER_MAPPING_NV:
		*params = mapping;
		return;
	case GL_COMBINER_COMPONENT_USAGE_NV:
		*params = usage;
		return;
	default:
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
								 "glGetCombinerInputParameterivNV(pname=0x%x)", pname);
		return;
	}
}


void STATE_APIENTRY crStateGetCombinerInputParameterfvNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params )
{
		GLint iparams;
		crStateGetCombinerInputParameterivNV(stage, portion, variable, pname, &iparams);
		*params = (GLfloat) iparams;
}


void STATE_APIENTRY crStateCombinerStageParameterfvNV( GLenum stage, GLenum pname, const GLfloat *params )
{
	CRContext *g = GetCurrentContext();
	CRRegCombinerState *r = &(g->regcombiner);
	CRStateBits *sb = GetCurrentBits();
	CRRegCombinerBits *rb = &(sb->regcombiner);
	
	stage -= GL_COMBINER0_NV;
	if( stage >= g->limits.maxGeneralCombiners )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "CombinerStageParameterfvNV passed bogus stage: 0x%x", stage+GL_COMBINER0_NV );
		return;
	}
	
	switch( pname )
	{
		case GL_CONSTANT_COLOR0_NV:
			r->stageConstantColor0[stage].r = params[0];
			r->stageConstantColor0[stage].g = params[1];
			r->stageConstantColor0[stage].b = params[2];
			r->stageConstantColor0[stage].a = params[3];
			DIRTY(rb->regCombinerStageColor0[stage], g->neg_bitid);
			break;
		case GL_CONSTANT_COLOR1_NV:
			r->stageConstantColor1[stage].r = params[0];
			r->stageConstantColor1[stage].g = params[1];
			r->stageConstantColor1[stage].b = params[2];
			r->stageConstantColor1[stage].a = params[3];
			DIRTY(rb->regCombinerStageColor1[stage], g->neg_bitid);
			break;
		default:
			crStateError( __LINE__, __FILE__, GL_INVALID_ENUM, "CombinerStageParameter passed bogus pname: 0x%x", pname );
			return;
	}

	DIRTY(rb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateGetCombinerStageParameterfvNV( GLenum stage, GLenum pname, GLfloat *params )
{
	CRContext *g = GetCurrentContext();
	CRRegCombinerState *r = &(g->regcombiner);

	stage -= GL_COMBINER0_NV;
	if( stage >= g->limits.maxGeneralCombiners )
	{
		crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "GetCombinerStageParameterfvNV passed bogus stage: 0x%x", stage+GL_COMBINER0_NV );
		return;
	}
	switch( pname )
	{
		case GL_CONSTANT_COLOR0_NV:
			params[0] = r->stageConstantColor0[stage].r;
			params[1] = r->stageConstantColor0[stage].g;
			params[2] = r->stageConstantColor0[stage].b;
			params[3] = r->stageConstantColor0[stage].a;
			break;
		case GL_CONSTANT_COLOR1_NV:
			params[0] = r->stageConstantColor1[stage].r;
			params[1] = r->stageConstantColor1[stage].g;
			params[2] = r->stageConstantColor1[stage].b;
			params[3] = r->stageConstantColor1[stage].a;
			break;
		default:
			crStateError( __LINE__, __FILE__, GL_INVALID_ENUM, "GetCombinerStageParameter passed bogus pname: 0x%x", pname );
			return;
	}
	return;
}
