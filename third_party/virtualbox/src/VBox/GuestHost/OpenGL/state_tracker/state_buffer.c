/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"

void crStateBufferInit (CRContext *ctx)
{
    CRBufferState *b = &ctx->buffer;
    CRStateBits *sb          = GetCurrentBits(ctx->pStateTracker);
    CRBufferBits *bb = &(sb->buffer);
    GLcolorf zero_colorf = {0.0f, 0.0f, 0.0f, 0.0f};

    b->width = 640;
    b->height = 480;
    b->storedWidth = 0;
    b->storedHeight = 0;
    b->pFrontImg = NULL;
    b->pBackImg = NULL;

    b->depthTest = GL_FALSE;
    b->blend     = GL_FALSE;
    b->alphaTest = GL_FALSE;
    b->dither    = GL_TRUE;
    RESET(bb->enable, ctx->bitid);

    b->logicOp   = GL_FALSE;
    RESET(bb->logicOp, ctx->bitid);
    b->indexLogicOp   = GL_FALSE;
    RESET(bb->indexLogicOp, ctx->bitid);
    b->depthMask = GL_TRUE;
    RESET(bb->depthMask, ctx->bitid);

    b->alphaTestFunc = GL_ALWAYS;
    b->alphaTestRef = 0;
    RESET(bb->alphaFunc, ctx->bitid);
    b->depthFunc = GL_LESS;
    RESET(bb->depthFunc, ctx->bitid);
    b->blendSrcRGB = GL_ONE;
    b->blendDstRGB = GL_ZERO;
    RESET(bb->blendFunc, ctx->bitid);
#ifdef CR_EXT_blend_func_separate
    b->blendSrcA = GL_ONE;
    b->blendDstA = GL_ZERO;
    RESET(bb->blendFuncSeparate, ctx->bitid);
#endif
    b->logicOpMode = GL_COPY;
    b->drawBuffer = GL_BACK;
    RESET(bb->drawBuffer, ctx->bitid);
    b->readBuffer = GL_BACK;
    RESET(bb->readBuffer, ctx->bitid);
    b->indexWriteMask = 0xffffffff;
    RESET(bb->indexMask, ctx->bitid);
    b->colorWriteMask.r = GL_TRUE;
    b->colorWriteMask.g = GL_TRUE;
    b->colorWriteMask.b = GL_TRUE;
    b->colorWriteMask.a = GL_TRUE;
    RESET(bb->colorWriteMask, ctx->bitid);
    b->colorClearValue = zero_colorf;
    RESET(bb->clearColor, ctx->bitid);
    b->indexClearValue = 0;
    RESET(bb->clearIndex, ctx->bitid);
    b->depthClearValue = (GLdefault) 1.0;
    RESET(bb->clearDepth, ctx->bitid);
    b->accumClearValue = zero_colorf;
    RESET(bb->clearAccum, ctx->bitid);

#ifdef CR_EXT_blend_color
    b->blendColor = zero_colorf;
    RESET(bb->blendColor, ctx->bitid);
#endif
#if defined(CR_EXT_blend_minmax) || defined(CR_EXT_blend_subtract) || defined(CR_EXT_blend_logic_op)
    b->blendEquation = GL_FUNC_ADD_EXT;
    RESET(bb->blendEquation, ctx->bitid);
#endif

    RESET(bb->dirty, ctx->bitid);
}

void STATE_APIENTRY crStateAlphaFunc (PCRStateTracker pState, GLenum func, GLclampf ref) 
{
    CRContext *g             = GetCurrentContext(pState);
    CRBufferState *b         = &(g->buffer);
    CRStateBits *sb          = GetCurrentBits(pState);
    CRBufferBits *bb = &(sb->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glAlphaFunc called in begin/end");
        return;
    }

    FLUSH();

    switch (func) 
    {
        case GL_NEVER:
        case GL_LESS:
        case GL_EQUAL:
        case GL_LEQUAL:
        case GL_GREATER:
        case GL_GEQUAL:
        case GL_NOTEQUAL:
        case GL_ALWAYS:
            break;
        default:
            crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glAlphaFunc:  Invalid func: %d", func);
            return;
    }

    if (ref < 0.0f) ref = 0.0f;
    if (ref > 1.0f) ref = 1.0f;

    b->alphaTestFunc = func;
    b->alphaTestRef = ref;
    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(bb->alphaFunc, g->neg_bitid);
}

void STATE_APIENTRY crStateDepthFunc (PCRStateTracker pState, GLenum func) 
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sb = GetCurrentBits(pState);
    CRBufferBits *bb = &(sb->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glDepthFunc called in begin/end");
        return;
    }

    FLUSH();

    switch (func) 
    {
        case GL_NEVER:
        case GL_LESS:
        case GL_EQUAL:
        case GL_LEQUAL:
        case GL_GREATER:
        case GL_NOTEQUAL:
        case GL_GEQUAL:
        case GL_ALWAYS:
            break;
        default:
            crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glDepthFunc:  Invalid func: %d", func);
            return;
    }


    b->depthFunc = func;
    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(bb->depthFunc, g->neg_bitid);
}

void STATE_APIENTRY crStateBlendFunc (PCRStateTracker pState, GLenum sfactor, GLenum dfactor) 
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sb = GetCurrentBits(pState);
    CRBufferBits *bb = &(sb->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glBlendFunc called in begin/end");
        return;
    }

    FLUSH();

    switch (sfactor) 
    {
        case GL_ZERO:
        case GL_ONE:
        case GL_DST_COLOR:
        case GL_ONE_MINUS_DST_COLOR:
        case GL_SRC_ALPHA:
        case GL_ONE_MINUS_SRC_ALPHA:
        case GL_DST_ALPHA:
        case GL_ONE_MINUS_DST_ALPHA:
        case GL_SRC_ALPHA_SATURATE:
            break; /* OK */
#ifdef CR_EXT_blend_color
        case GL_CONSTANT_COLOR_EXT:
        case GL_ONE_MINUS_CONSTANT_COLOR_EXT:
        case GL_CONSTANT_ALPHA_EXT:
        case GL_ONE_MINUS_CONSTANT_ALPHA_EXT:
            if (g->extensions.EXT_blend_color)
                break; /* OK */
#endif
        RT_FALL_THRU();
        default:
            crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "Invalid sfactor passed to glBlendFunc: %d", sfactor);
            return;
    }

    switch (dfactor) 
    {
        case GL_ZERO:
        case GL_ONE:
        case GL_SRC_COLOR:
        case GL_ONE_MINUS_SRC_COLOR:
        case GL_SRC_ALPHA:
        case GL_ONE_MINUS_SRC_ALPHA:
        case GL_DST_ALPHA:
        case GL_ONE_MINUS_DST_ALPHA:
            break; /* OK */
#ifdef CR_EXT_blend_color
        case GL_CONSTANT_COLOR_EXT:
        case GL_ONE_MINUS_CONSTANT_COLOR_EXT:
        case GL_CONSTANT_ALPHA_EXT:
        case GL_ONE_MINUS_CONSTANT_ALPHA_EXT:
            if (g->extensions.EXT_blend_color)
                break; /* OK */
#endif
        RT_FALL_THRU();
        default:
            crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "Invalid dfactor passed to glBlendFunc: %d", dfactor);
            return;
    }

    b->blendSrcRGB = sfactor;
    b->blendDstRGB = dfactor;
    b->blendSrcA = sfactor;
    b->blendDstA = dfactor;
    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(bb->blendFunc, g->neg_bitid);
}

void STATE_APIENTRY crStateBlendColorEXT(PCRStateTracker pState, GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sb = GetCurrentBits(pState);
    CRBufferBits *bb = &(sb->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "BlendColorEXT called inside a Begin/End" );
        return;
    }

    b->blendColor.r = red;
    b->blendColor.g = green;
    b->blendColor.b = blue;
    b->blendColor.a = alpha;
    DIRTY(bb->blendColor, g->neg_bitid);
    DIRTY(bb->dirty, g->neg_bitid);
}

#ifdef CR_EXT_blend_func_separate
void STATE_APIENTRY crStateBlendFuncSeparateEXT(PCRStateTracker pState, GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorA, GLenum dfactorA )
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sb = GetCurrentBits(pState);
    CRBufferBits *bb = &(sb->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "BlendFuncSeparateEXT called inside a Begin/End" );
        return;
    }

    FLUSH();

    switch (sfactorRGB) 
    {
        case GL_ZERO:
        case GL_ONE:
        case GL_DST_COLOR:
        case GL_ONE_MINUS_DST_COLOR:
        case GL_SRC_ALPHA:
        case GL_ONE_MINUS_SRC_ALPHA:
        case GL_DST_ALPHA:
        case GL_ONE_MINUS_DST_ALPHA:
        case GL_SRC_ALPHA_SATURATE:
            break; /* OK */
#ifdef CR_EXT_blend_color
        case GL_CONSTANT_COLOR_EXT:
        case GL_ONE_MINUS_CONSTANT_COLOR_EXT:
        case GL_CONSTANT_ALPHA_EXT:
        case GL_ONE_MINUS_CONSTANT_ALPHA_EXT:
            if (g->extensions.EXT_blend_color)
                break; /* OK */
#endif
        RT_FALL_THRU();
        default:
            crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "Invalid sfactorRGB passed to glBlendFuncSeparateEXT: %d", sfactorRGB);
            return;
    }

    switch (sfactorA) 
    {
        case GL_ZERO:
        case GL_ONE:
        case GL_DST_COLOR:
        case GL_ONE_MINUS_DST_COLOR:
        case GL_SRC_ALPHA:
        case GL_ONE_MINUS_SRC_ALPHA:
        case GL_DST_ALPHA:
        case GL_ONE_MINUS_DST_ALPHA:
        case GL_SRC_ALPHA_SATURATE:
            break; /* OK */
#ifdef CR_EXT_blend_color
        case GL_CONSTANT_COLOR_EXT:
        case GL_ONE_MINUS_CONSTANT_COLOR_EXT:
        case GL_CONSTANT_ALPHA_EXT:
        case GL_ONE_MINUS_CONSTANT_ALPHA_EXT:
            if (g->extensions.EXT_blend_color)
                break; /* OK */
#endif
        RT_FALL_THRU();
        default:
            crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "Invalid sfactorA passed to glBlendFuncSeparateEXT: %d", sfactorA);
            return;
    }

    switch (dfactorRGB) 
    {
        case GL_ZERO:
        case GL_ONE:
        case GL_SRC_COLOR:
        case GL_DST_COLOR:
        case GL_ONE_MINUS_DST_COLOR:
        case GL_ONE_MINUS_SRC_COLOR:
        case GL_SRC_ALPHA:
        case GL_ONE_MINUS_SRC_ALPHA:
        case GL_DST_ALPHA:
        case GL_ONE_MINUS_DST_ALPHA:
        case GL_SRC_ALPHA_SATURATE:
            break; /* OK */
#ifdef CR_EXT_blend_color
        case GL_CONSTANT_COLOR_EXT:
        case GL_ONE_MINUS_CONSTANT_COLOR_EXT:
        case GL_CONSTANT_ALPHA_EXT:
        case GL_ONE_MINUS_CONSTANT_ALPHA_EXT:
            if (g->extensions.EXT_blend_color)
                break; /* OK */
#endif
        RT_FALL_THRU();
        default:
            crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "Invalid dfactorRGB passed to glBlendFuncSeparateEXT: %d", dfactorRGB);
            return;
    }

    switch (dfactorA) 
    {
        case GL_ZERO:
        case GL_ONE:
        case GL_DST_COLOR:
        case GL_SRC_COLOR:
        case GL_ONE_MINUS_SRC_COLOR:
        case GL_ONE_MINUS_DST_COLOR:
        case GL_SRC_ALPHA:
        case GL_ONE_MINUS_SRC_ALPHA:
        case GL_DST_ALPHA:
        case GL_ONE_MINUS_DST_ALPHA:
        case GL_SRC_ALPHA_SATURATE:
            break; /* OK */
#ifdef CR_EXT_blend_color
        case GL_CONSTANT_COLOR_EXT:
        case GL_ONE_MINUS_CONSTANT_COLOR_EXT:
        case GL_CONSTANT_ALPHA_EXT:
        case GL_ONE_MINUS_CONSTANT_ALPHA_EXT:
            if (g->extensions.EXT_blend_color)
                break; /* OK */
#endif
        RT_FALL_THRU();
        default:
            crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "Invalid dfactorA passed to glBlendFuncSeparateEXT: %d", dfactorA);
            return;
    }

    b->blendSrcRGB = sfactorRGB;
    b->blendDstRGB = dfactorRGB;
    b->blendSrcA = sfactorA;
    b->blendDstA = dfactorA;
    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(bb->blendFuncSeparate, g->neg_bitid);
}
#endif

void STATE_APIENTRY crStateBlendEquationEXT(PCRStateTracker pState, GLenum mode )
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sb = GetCurrentBits(pState);
    CRBufferBits *bb = &(sb->buffer);

    if( g->current.inBeginEnd )
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "BlendEquationEXT called inside a Begin/End" );
        return;
    }
    switch( mode )
    {
#if defined(CR_EXT_blend_minmax) || defined(CR_EXT_blend_subtract) || defined(CR_EXT_blend_logic_op)
        case GL_FUNC_ADD_EXT:
#ifdef CR_EXT_blend_subtract
        case GL_FUNC_SUBTRACT_EXT:
        case GL_FUNC_REVERSE_SUBTRACT_EXT:
#endif /* CR_EXT_blend_subtract */
#ifdef CR_EXT_blend_minmax
        case GL_MIN_EXT:
        case GL_MAX_EXT:
#endif /* CR_EXT_blend_minmax */
#ifdef CR_EXT_blend_logic_op
        case GL_LOGIC_OP:
#endif /* CR_EXT_blend_logic_op */
            b->blendEquation = mode;
            break;
#endif /* defined(CR_EXT_blend_minmax) || defined(CR_EXT_blend_subtract) || defined(CR_EXT_blend_logic_op) */
        default:
            crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
                "BlendEquationEXT: mode called with illegal parameter: 0x%x", (GLenum) mode );
            return;
    }
    DIRTY(bb->blendEquation, g->neg_bitid);
    DIRTY(bb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateLogicOp (PCRStateTracker pState, GLenum opcode) 
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sb = GetCurrentBits(pState);
    CRBufferBits *bb = &(sb->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glLogicOp called in begin/end");
        return;
    }

    FLUSH();

    switch (opcode) 
    {
        case GL_CLEAR:
        case GL_SET:
        case GL_COPY:
        case GL_COPY_INVERTED:
        case GL_NOOP:
        case GL_INVERT:
        case GL_AND:
        case GL_NAND:
        case GL_OR:
        case GL_NOR:
        case GL_XOR:
        case GL_EQUIV:
        case GL_AND_REVERSE:
        case GL_AND_INVERTED:
        case GL_OR_REVERSE:
        case GL_OR_INVERTED:
            break;
        default:
            crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glLogicOp called with bogus opcode: %d", opcode);
            return;
    }

    b->logicOpMode = opcode;
    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(bb->logicOp, g->neg_bitid);
    DIRTY(bb->indexLogicOp, g->neg_bitid);
}

void STATE_APIENTRY crStateDrawBuffer (PCRStateTracker pState, GLenum mode) 
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sb = GetCurrentBits(pState);
    CRBufferBits *bb = &(sb->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glDrawBuffer called in begin/end");
        return;
    }

    FLUSH();

    switch (mode) 
    {
        case GL_NONE:
            break;
        case GL_FRONT_LEFT:
        case GL_FRONT_RIGHT:
        case GL_BACK_LEFT:
        case GL_BACK_RIGHT:
        case GL_FRONT:
        case GL_BACK:
        case GL_LEFT:
        case GL_RIGHT:
        case GL_FRONT_AND_BACK:
        case GL_AUX0:
        case GL_AUX1:
        case GL_AUX2:
        case GL_AUX3:
            if (g->framebufferobject.drawFB)
            {
                crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glDrawBuffer invalid mode while fbo is active");
                return;
            }
            break;
        default:
            if (mode>=GL_COLOR_ATTACHMENT0_EXT && mode<=GL_COLOR_ATTACHMENT15_EXT)
            {
                if (!g->framebufferobject.drawFB)
                {
                    crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glDrawBuffer invalid mode while fbo is inactive");
                    return;
                }
            }
            else
            {
                crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glDrawBuffer called with bogus mode: %d", mode);
                return;
            }
    }

    if (g->framebufferobject.drawFB)
    {
        g->framebufferobject.drawFB->drawbuffer[0] = mode;
    }
    else
    {
        b->drawBuffer = mode;
        DIRTY(bb->dirty, g->neg_bitid);
        DIRTY(bb->drawBuffer, g->neg_bitid);
    }
}

void STATE_APIENTRY crStateReadBuffer (PCRStateTracker pState, GLenum mode) 
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sb = GetCurrentBits(pState);
    CRBufferBits *bb = &(sb->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glReadBuffer called in begin/end");
        return;
    }

    FLUSH();

    switch (mode) 
    {
        case GL_NONE:
            break;
        case GL_FRONT_LEFT:
        case GL_FRONT_RIGHT:
        case GL_BACK_LEFT:
        case GL_BACK_RIGHT:
        case GL_FRONT:
        case GL_BACK:
        case GL_LEFT:
        case GL_RIGHT:
        case GL_FRONT_AND_BACK:
        case GL_AUX0:
        case GL_AUX1:
        case GL_AUX2:
        case GL_AUX3:
            if (g->framebufferobject.readFB)
            {
                crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glReadBuffer invalid mode while fbo is active");
                return;
            }
            break;
        default:
            if (mode>=GL_COLOR_ATTACHMENT0_EXT && mode<=GL_COLOR_ATTACHMENT15_EXT)
            {
                if (!g->framebufferobject.readFB)
                {
                    crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glReadBuffer invalid mode while fbo is inactive");
                    return;
                }
                else
                {
                    /*@todo, check if fbo binding is complete*/
                }
            }
            else
            {
                crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glReadBuffer called with bogus mode: %d", mode);
                return;
            }
    }

    if (g->framebufferobject.readFB)
    {
        g->framebufferobject.readFB->readbuffer = mode;
    }
    else
    {
        b->readBuffer = mode;
        DIRTY(bb->dirty, g->neg_bitid);
        DIRTY(bb->readBuffer, g->neg_bitid);
    }
}

void STATE_APIENTRY crStateIndexMask (PCRStateTracker pState, GLuint mask) 
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sp = GetCurrentBits(pState);
    CRBufferBits *bb = &(sp->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glReadBuffer called in begin/end");
        return;
    }

    FLUSH();

    b->indexWriteMask = mask;
    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(bb->indexMask, g->neg_bitid);
}

void STATE_APIENTRY crStateColorMask (PCRStateTracker pState, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) 
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sp = GetCurrentBits(pState);
    CRBufferBits *bb = &(sp->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glReadBuffer called in begin/end");
        return;
    }

    FLUSH();

    b->colorWriteMask.r = red;
    b->colorWriteMask.g = green;
    b->colorWriteMask.b = blue;
    b->colorWriteMask.a = alpha;
    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(bb->colorWriteMask, g->neg_bitid);
}

void STATE_APIENTRY crStateClearColor (PCRStateTracker pState, GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) 
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sp = GetCurrentBits(pState);
    CRBufferBits *bb = &(sp->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glClearColor called in begin/end");
        return;
    }

    FLUSH();

    if (red < 0.0f) red = 0.0f;
    if (red > 1.0f) red = 1.0f;
    if (green < 0.0f) green = 0.0f;
    if (green > 1.0f) green = 1.0f;
    if (blue < 0.0f) blue = 0.0f;
    if (blue > 1.0f) blue = 1.0f;
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    b->colorClearValue.r = red;
    b->colorClearValue.g = green;
    b->colorClearValue.b = blue;
    b->colorClearValue.a = alpha;
    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(bb->clearColor, g->neg_bitid);
}

void STATE_APIENTRY crStateClearIndex (PCRStateTracker pState, GLfloat c) 
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sp = GetCurrentBits(pState);
    CRBufferBits *bb = &(sp->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glClearIndex called in begin/end");
        return;
    }

    b->indexClearValue = c;
    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(bb->clearIndex, g->neg_bitid);
}

void STATE_APIENTRY crStateClearDepth (PCRStateTracker pState, GLclampd depth) 
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sp = GetCurrentBits(pState);
    CRBufferBits *bb = &(sp->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glClearDepth called in begin/end");
        return;
    }

    FLUSH();

    if (depth < 0.0) depth = 0.0;
    if (depth > 1.0) depth = 1.0;

    b->depthClearValue = (GLdefault) depth;
    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(bb->clearDepth, g->neg_bitid);
}

void STATE_APIENTRY crStateClearAccum (PCRStateTracker pState, GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) 
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *b = &(g->buffer);
    CRStateBits *sp = GetCurrentBits(pState);
    CRBufferBits *bb = &(sp->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glClearAccum called in begin/end");
        return;
    }
    
    FLUSH();

    if (red < -1.0f) red = 0.0f;
    if (red > 1.0f) red = 1.0f;
    if (green < -1.0f) green = 0.0f;
    if (green > 1.0f) green = 1.0f;
    if (blue < -1.0f) blue = 0.0f;
    if (blue > 1.0f) blue = 1.0f;
    if (alpha < -1.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    b->accumClearValue.r = red;
    b->accumClearValue.g = green;
    b->accumClearValue.b = blue;
    b->accumClearValue.a = alpha;
    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(bb->clearAccum, g->neg_bitid);
}

void STATE_APIENTRY crStateDepthMask (PCRStateTracker pState, GLboolean b) 
{
    CRContext *g = GetCurrentContext(pState);
    CRBufferState *bs = &(g->buffer);
    CRStateBits *sp = GetCurrentBits(pState);
    CRBufferBits *bb = &(sp->buffer);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "DepthMask called in begin/end");
        return;
    }

    FLUSH();

    bs->depthMask = b;
    DIRTY(bb->dirty, g->neg_bitid);
    DIRTY(bb->depthMask, g->neg_bitid);
}
