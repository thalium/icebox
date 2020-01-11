/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"


static GLint crStateStencilBufferGetIdxAndCount(PCRStateTracker pState, CRStencilState *s, GLenum face, GLint *pIdx, GLint *pBitsIdx)
{
    switch (face)
    {
        case GL_FRONT_AND_BACK:
            *pIdx = 0;
            *pBitsIdx = CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK;
            return 2;
        case GL_FRONT:
            *pIdx = CRSTATE_STENCIL_BUFFER_ID_FRONT;
            *pBitsIdx = CRSTATE_STENCIL_BUFFER_REF_ID_FRONT;
            return 1;
        case GL_BACK:
            *pIdx = CRSTATE_STENCIL_BUFFER_ID_BACK;
            *pBitsIdx = CRSTATE_STENCIL_BUFFER_REF_ID_BACK;
            return 1;
        case 0:
            if (!s->stencilTwoSideEXT || s->activeStencilFace == GL_FRONT)
            {
                /* both front and back */
                *pIdx = 0;
                *pBitsIdx = CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK;
                return 2;
            }
            *pIdx = CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK;
            *pBitsIdx = CRSTATE_STENCIL_BUFFER_REF_ID_TWO_SIDE_BACK;
            return 1;
        default:
            crStateError(pState, __LINE__,__FILE__,GL_INVALID_ENUM, "crStateStencilBufferGetIdxAndCount");
            return 0;
    }
#ifndef VBOX /* unreachable */
    crError("should never be here!");
    return 0;
#endif
}

void crStateStencilBufferInit(CRStencilBufferState *s)
{
    s->func = GL_ALWAYS;
    s->mask = 0xFFFFFFFF;
    s->ref = 0;

    s->fail = GL_KEEP;
    s->passDepthFail = GL_KEEP;
    s->passDepthPass = GL_KEEP;
}

static void crStateStencilBufferRefBitsInit(CRContext *ctx, CRStencilBufferRefBits *sb)
{
    RESET(sb->func, ctx->bitid);
    RESET(sb->op, ctx->bitid);
}

void crStateStencilInit(CRContext *ctx)
{
	CRStencilState *s = &ctx->stencil;
	CRStateBits *stateb = GetCurrentBits(ctx->pStateTracker);
	CRStencilBits *sb = &(stateb->stencil);
	int i;

	s->stencilTest = GL_FALSE;
	RESET(sb->enable, ctx->bitid);

    s->stencilTwoSideEXT = GL_FALSE;
    RESET(sb->enableTwoSideEXT, ctx->bitid);

	s->activeStencilFace = GL_FRONT;
	RESET(sb->activeStencilFace, ctx->bitid);

	s->clearValue = 0;
	RESET(sb->clearValue, ctx->bitid);

	s->writeMask = 0xFFFFFFFF;
	RESET(sb->writeMask, ctx->bitid);

	RESET(sb->dirty, ctx->bitid);

	for (i = 0; i < CRSTATE_STENCIL_BUFFER_COUNT; ++i)
	{
	    crStateStencilBufferInit(&s->buffers[i]);
	}

    for (i = 0; i < CRSTATE_STENCIL_BUFFER_REF_COUNT; ++i)
    {
        crStateStencilBufferRefBitsInit(ctx, &sb->bufferRefs[i]);
    }
}

static void crStateStencilBufferFunc(CRContext *g, CRStencilBufferState *s, GLenum func, GLint ref, GLuint mask)
{
    (void)g;
    s->func = func;
    s->ref = ref;
    s->mask = mask;
}

static void crStateStencilFuncPerform(PCRStateTracker pState, GLenum face, GLenum func, GLint ref, GLuint mask)
{
    CRContext *g = GetCurrentContext(pState);
    CRStencilState *s = &(g->stencil);
    CRStateBits *stateb = GetCurrentBits(pState);
    CRStencilBits *sb = &(stateb->stencil);
    GLint idx, bitsIdx, count, i;


    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION,
            "glStencilFunc called in begin/end");
        return;
    }

    FLUSH();

    if (func != GL_NEVER &&
        func != GL_LESS &&
        func != GL_LEQUAL &&
        func != GL_GREATER &&
        func != GL_GEQUAL &&
        func != GL_EQUAL &&
        func != GL_NOTEQUAL &&
        func != GL_ALWAYS)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM,
            "glStencilFunc called with bogu func: %d", func);
        return;
    }

    count = crStateStencilBufferGetIdxAndCount(pState, s, face, &idx, &bitsIdx);
    if (count)
    {
        for (i = idx; i < idx + count; ++i)
        {
            crStateStencilBufferFunc(g, &s->buffers[i], func, ref, mask);
        }
        DIRTY(sb->bufferRefs[bitsIdx].func, g->neg_bitid);

        DIRTY(sb->dirty, g->neg_bitid);
    }
}

void STATE_APIENTRY crStateStencilFuncSeparate(PCRStateTracker pState, GLenum face, GLenum func, GLint ref, GLuint mask)
{
    if (!face)
    {
        /* crStateStencilFuncPerform accepts 0 value, while glStencilFuncSeparate does not,
         * filter it out here */
        crStateError(pState, __LINE__,__FILE__,GL_INVALID_ENUM, "crStateStencilFuncSeparate");
        return;
    }
    crStateStencilFuncPerform(pState, face, func, ref, mask);
}

void STATE_APIENTRY crStateStencilFunc(PCRStateTracker pState, GLenum func, GLint ref, GLuint mask)
{
    crStateStencilFuncPerform(pState, 0, func, ref, mask);
}

static void STATE_APIENTRY crStateStencilBufferOp (CRContext *g, CRStencilBufferState *s, GLenum fail, GLenum zfail, GLenum zpass)
{
    (void)g;
    s->fail = fail;
    s->passDepthFail = zfail;
    s->passDepthPass = zpass;
}

static void crStateStencilOpPerform (PCRStateTracker pState, GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
	CRContext *g = GetCurrentContext(pState);
	CRStencilState *s = &(g->stencil);
	CRStateBits *stateb = GetCurrentBits(pState);
	CRStencilBits *sb = &(stateb->stencil);
    GLint idx, bitsIdx, count, i;

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, 
			"glStencilOp called in begin/end");
		return;
	}

	FLUSH();

	switch (fail) {
	case GL_KEEP:
	case GL_ZERO:
	case GL_REPLACE:
	case GL_INCR:
	case GL_DECR:
	case GL_INVERT:
#ifdef CR_EXT_stencil_wrap
	case GL_INCR_WRAP_EXT:
	case GL_DECR_WRAP_EXT:
#endif
		break;
	default:
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, 
			"glStencilOp called with bogus fail: %d", fail);
		return;
	}

	switch (zfail) {
	case GL_KEEP:
	case GL_ZERO:
	case GL_REPLACE:
	case GL_INCR:
	case GL_DECR:
	case GL_INVERT:
#ifdef CR_EXT_stencil_wrap
	case GL_INCR_WRAP_EXT:
	case GL_DECR_WRAP_EXT:
#endif
		break;
	default:
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, 
			"glStencilOp called with bogus zfail: %d", zfail);
		return;
	}

	switch (zpass) {
	case GL_KEEP:
	case GL_ZERO:
	case GL_REPLACE:
	case GL_INCR:
	case GL_DECR:
	case GL_INVERT:
#ifdef CR_EXT_stencil_wrap
	case GL_INCR_WRAP_EXT:
	case GL_DECR_WRAP_EXT:
#endif
		break;
	default:
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, 
			"glStencilOp called with bogus zpass: %d", zpass);
		return;
	}

    count = crStateStencilBufferGetIdxAndCount(pState, s, face, &idx, &bitsIdx);
    if (count)
    {
        for (i = idx; i < idx + count; ++i)
        {
            crStateStencilBufferOp(g, &s->buffers[i], fail, zfail, zpass);
        }

        DIRTY(sb->bufferRefs[bitsIdx].op, g->neg_bitid);

        DIRTY(sb->dirty, g->neg_bitid);
    }
}

void STATE_APIENTRY crStateStencilOpSeparate (PCRStateTracker pState, GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
    if (!face)
    {
        /* crStateStencilOpPerform accepts 0 value, while glStencilOpSeparate does not,
         * filter it out here */
        crStateError(pState, __LINE__,__FILE__,GL_INVALID_ENUM, "crStateStencilOpSeparate");
        return;
    }
    crStateStencilOpPerform (pState, 0, fail, zfail, zpass);
}

void STATE_APIENTRY crStateStencilOp (PCRStateTracker pState, GLenum fail, GLenum zfail, GLenum zpass)
{
    crStateStencilOpPerform (pState, 0, fail, zfail, zpass);
}

void STATE_APIENTRY crStateClearStencil (PCRStateTracker pState, GLint c) 
{
	CRContext *g = GetCurrentContext(pState);
	CRStencilState *s = &(g->stencil);
	CRStateBits *stateb = GetCurrentBits(pState);
	CRStencilBits *sb = &(stateb->stencil);

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, 
			"glClearStencil called in begin/end");
		return;
	}

	FLUSH();

	s->clearValue = c;
	
	DIRTY(sb->clearValue, g->neg_bitid);
	DIRTY(sb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateStencilMask (PCRStateTracker pState, GLuint mask) 
{
	CRContext *g = GetCurrentContext(pState);
	CRStencilState *s = &(g->stencil);
	CRStateBits *stateb = GetCurrentBits(pState);
	CRStencilBits *sb = &(stateb->stencil);

	if (g->current.inBeginEnd)
	{
		crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, 
			"glStencilMask called in begin/end");
		return;
	}

	FLUSH();

	s->writeMask = mask;

	DIRTY(sb->writeMask, g->neg_bitid);
	DIRTY(sb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateActiveStencilFaceEXT (PCRStateTracker pState, GLenum face)
{
    CRContext *g = GetCurrentContext(pState);
    CRStencilState *s = &(g->stencil);
    CRStateBits *stateb = GetCurrentBits(pState);
    CRStencilBits *sb = &(stateb->stencil);

    switch (face)
    {
    case GL_FRONT:
    case GL_BACK:
        s->activeStencilFace = face;
        break;
    default:
        crStateError(pState, __LINE__,__FILE__,GL_INVALID_ENUM, "crStateActiveStencilFaceEXT");
        return;
    }

    DIRTY(sb->activeStencilFace, g->neg_bitid);
    DIRTY(sb->dirty, g->neg_bitid);
}

#ifdef CRSTATE_DEBUG_STENCIL_ERR
#define CRSTATE_CLEARERR() do { \
            while (pState->diff_api.GetError() != GL_NO_ERROR) {} \
        } while (0)

#define CRSTATE_CHECKGLERR(_op) do {\
            GLenum _glErr; \
            CRSTATE_CLEARERR(); \
            _op; \
            while ((_glErr = pState->diff_api.GetError()) != GL_NO_ERROR) { Assert(0);} \
        }while (0)
#else
#define CRSTATE_CHECKGLERR(_op) do { _op; } while (0)
#endif

#define CR_STATE_STENCIL_FUNC_MATCH(_s1, _i1, _s2, _i2) (\
    (_s1)->buffers[(_i1)].func == (_s2)->buffers[(_i2)].func && \
    (_s1)->buffers[(_i1)].ref  == (_s2)->buffers[(_i2)].ref  && \
    (_s1)->buffers[(_i1)].mask == (_s2)->buffers[(_i2)].mask)

#define CR_STATE_STENCIL_FUNC_COPY(_s1, _i1, _s2, _i2) do { \
        (_s1)->buffers[(_i1)].func = (_s2)->buffers[(_i2)].func; \
        (_s1)->buffers[(_i1)].ref  = (_s2)->buffers[(_i2)].ref; \
        (_s1)->buffers[(_i1)].mask = (_s2)->buffers[(_i2)].mask; \
    } while (0)


#define CR_STATE_STENCIL_OP_MATCH(_s1, _i1, _s2, _i2) (\
        (_s1)->buffers[(_i1)].fail == (_s2)->buffers[(_i2)].fail && \
        (_s1)->buffers[(_i1)].passDepthFail  == (_s2)->buffers[(_i2)].passDepthFail  && \
        (_s1)->buffers[(_i1)].passDepthPass == (_s2)->buffers[(_i2)].passDepthPass)

#define CR_STATE_STENCIL_OP_COPY(_s1, _i1, _s2, _i2) do { \
        (_s1)->buffers[(_i1)].fail = (_s2)->buffers[(_i2)].fail; \
        (_s1)->buffers[(_i1)].passDepthFail  = (_s2)->buffers[(_i2)].passDepthFail; \
        (_s1)->buffers[(_i1)].passDepthPass = (_s2)->buffers[(_i2)].passDepthPass; \
    } while (0)


void crStateStencilDiff(CRStencilBits *b, CRbitvalue *bitID,
        CRContext *fromCtx, CRContext *toCtx)
{
    PCRStateTracker pState = fromCtx->pStateTracker;
    CRStencilState *from = &(fromCtx->stencil);
    CRStencilState *to = &(toCtx->stencil);
    unsigned int j, i;
    GLenum activeFace;
    GLboolean backIsSet = GL_FALSE, frontIsSet = GL_FALSE, frontBackDirty, frontDirty, backDirty;
    GLchar frontMatch = -1, backMatch = -1, toFrontBackMatch = -1;
    CRbitvalue nbitID[CR_MAX_BITARRAY];

    CRASSERT(fromCtx->pStateTracker == toCtx->pStateTracker);

    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];
    i = 0; /* silence compiler */

    if (CHECKDIRTY(b->enable, bitID))
    {
        glAble able[2];
        able[0] = pState->diff_api.Disable;
        able[1] = pState->diff_api.Enable;
        if (from->stencilTest != to->stencilTest)
        {
            able[to->stencilTest](GL_STENCIL_TEST);
            from->stencilTest = to->stencilTest;
        }
        CLEARDIRTY(b->enable, nbitID);
    }

    if (CHECKDIRTY(b->enableTwoSideEXT, bitID))
    {
        glAble able[2];
        able[0] = pState->diff_api.Disable;
        able[1] = pState->diff_api.Enable;
        if (from->stencilTwoSideEXT != to->stencilTwoSideEXT)
        {
            able[to->stencilTwoSideEXT](GL_STENCIL_TEST_TWO_SIDE_EXT);
            from->stencilTwoSideEXT = to->stencilTwoSideEXT;
        }
        CLEARDIRTY(b->enableTwoSideEXT, nbitID);
    }

    if (CHECKDIRTY(b->clearValue, bitID))
    {
        if (from->clearValue != to->clearValue)
        {
            pState->diff_api.ClearStencil (to->clearValue);
            from->clearValue = to->clearValue;
        }
        CLEARDIRTY(b->clearValue, nbitID);
    }

    activeFace = to->activeStencilFace;


    /* func */
    frontBackDirty = CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].func, bitID);
    frontDirty = CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT].func, bitID);
    backDirty = CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_BACK].func, bitID);
#define CR_STATE_STENCIL_FUNC_FRONT_MATCH() ( \
        frontMatch >= 0 ? \
                frontMatch \
                : (frontMatch = CR_STATE_STENCIL_FUNC_MATCH(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT)))

#define CR_STATE_STENCIL_FUNC_BACK_MATCH() ( \
        backMatch >= 0 ? \
                backMatch \
                : (backMatch = CR_STATE_STENCIL_FUNC_MATCH(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_BACK)))

#define CR_STATE_STENCIL_FUNC_TO_FRONT_BACK_MATCH() ( \
        toFrontBackMatch >= 0 ? \
                toFrontBackMatch \
                : (toFrontBackMatch = CR_STATE_STENCIL_FUNC_MATCH(to, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_BACK)))

    if (frontBackDirty)
    {
        if (!CR_STATE_STENCIL_FUNC_FRONT_MATCH()
                || !CR_STATE_STENCIL_FUNC_BACK_MATCH())
        {
            if (CR_STATE_STENCIL_FUNC_TO_FRONT_BACK_MATCH())
            {
                if (activeFace == GL_BACK)
                {
                    pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                    activeFace = GL_FRONT;
                }

                pState->diff_api.StencilFunc (to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask);

                CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);
                CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);
                frontIsSet = GL_TRUE;
                backIsSet = GL_TRUE;
            }
            else if (!CR_STATE_STENCIL_FUNC_FRONT_MATCH())
            {
                if (activeFace == GL_BACK)
                {
                    pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                    activeFace = GL_FRONT;
                }

                pState->diff_api.StencilFuncSeparate (GL_FRONT, to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask);

                CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);
                frontIsSet = GL_TRUE;
            }
            else if (!CR_STATE_STENCIL_FUNC_BACK_MATCH())
            {
                if (activeFace == GL_BACK)
                {
                    pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                    activeFace = GL_FRONT;
                }

                pState->diff_api.StencilFuncSeparate (GL_BACK, to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].func,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].ref,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].mask);

                CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_BACK);

                backIsSet = GL_TRUE;
            }
        }

        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].func, nbitID);
    }

    if (frontDirty)
    {
        if (!CR_STATE_STENCIL_FUNC_FRONT_MATCH())
        {
            if (CR_STATE_STENCIL_FUNC_TO_FRONT_BACK_MATCH())
            {
                if (!frontIsSet || !backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    pState->diff_api.StencilFunc (to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask);

                    CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);
                    CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);

                    frontIsSet = GL_TRUE;
                    backIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_FUNC_FRONT_MATCH())
            {
                if (!frontIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    pState->diff_api.StencilFuncSeparate (GL_FRONT, to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask);

                    CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);
                    frontIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_FUNC_BACK_MATCH())
            {
                if (!backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    pState->diff_api.StencilFuncSeparate (GL_BACK, to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].func,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].ref,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].mask);
                    CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_BACK);
                    backIsSet = GL_TRUE;
                }
            }
        }
        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT].func, nbitID);
    }


    if (backDirty)
    {
        if (!CR_STATE_STENCIL_FUNC_FRONT_MATCH())
        {
            if (CR_STATE_STENCIL_FUNC_TO_FRONT_BACK_MATCH())
            {
                if (!frontIsSet || !backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    pState->diff_api.StencilFunc (to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask);

                    CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);
                    CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);

                    frontIsSet = GL_TRUE;
                    backIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_FUNC_FRONT_MATCH())
            {
                if (!frontIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    pState->diff_api.StencilFuncSeparate (GL_FRONT, to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask);

                    CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);
                    frontIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_FUNC_BACK_MATCH())
            {
                if (!backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    pState->diff_api.StencilFuncSeparate (GL_BACK, to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].func,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].ref,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].mask);
                    CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_BACK);
                    backIsSet = GL_TRUE;
                }
            }
        }
        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_BACK].func, nbitID);
    }

    if (CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_TWO_SIDE_BACK].func, bitID))
    {
        if (!CR_STATE_STENCIL_FUNC_MATCH(from, CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK, to, CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK))
        {
            if (activeFace == GL_FRONT)
            {
                pState->diff_api.ActiveStencilFaceEXT(GL_BACK);
                activeFace = GL_BACK;
            }

            pState->diff_api.StencilFunc (to->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].func,
                to->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].ref,
                to->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].mask);
            CR_STATE_STENCIL_FUNC_COPY(from, CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK, to, CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK);
        }
        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_TWO_SIDE_BACK].func, nbitID);
    }

#undef CR_STATE_STENCIL_FUNC_FRONT_MATCH
#undef CR_STATE_STENCIL_FUNC_BACK_MATCH
#undef CR_STATE_STENCIL_FUNC_TO_FRONT_BACK_MATCH

    /* op */
    backIsSet = GL_FALSE, frontIsSet = GL_FALSE;
    frontMatch = -1, backMatch = -1, toFrontBackMatch = -1;
    frontBackDirty = CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].op, bitID);
    frontDirty = CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT].op, bitID);
    backDirty = CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_BACK].op, bitID);

#define CR_STATE_STENCIL_OP_FRONT_MATCH() ( \
        frontMatch >= 0 ? \
                frontMatch \
                : (frontMatch = CR_STATE_STENCIL_OP_MATCH(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT)))

#define CR_STATE_STENCIL_OP_BACK_MATCH() ( \
        backMatch >= 0 ? \
                backMatch \
                : (backMatch = CR_STATE_STENCIL_OP_MATCH(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_BACK)))

#define CR_STATE_STENCIL_OP_TO_FRONT_BACK_MATCH() ( \
        toFrontBackMatch >= 0 ? \
                toFrontBackMatch \
                : (toFrontBackMatch = CR_STATE_STENCIL_OP_MATCH(to, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_BACK)))

    if (frontBackDirty)
    {
        if (!CR_STATE_STENCIL_OP_FRONT_MATCH()
                || !CR_STATE_STENCIL_OP_BACK_MATCH())
        {
            if (CR_STATE_STENCIL_OP_TO_FRONT_BACK_MATCH())
            {
                if (activeFace == GL_BACK)
                {
                    pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                    activeFace = GL_FRONT;
                }

                pState->diff_api.StencilOp (to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass);

                CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);
                CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);

                frontIsSet = GL_TRUE;
                backIsSet = GL_TRUE;
            }
            else if (!CR_STATE_STENCIL_OP_FRONT_MATCH())
            {
                if (activeFace == GL_BACK)
                {
                    pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                    activeFace = GL_FRONT;
                }

                pState->diff_api.StencilOpSeparate (GL_FRONT, to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass);
                CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);
                frontIsSet = GL_TRUE;
            }
            else if (!CR_STATE_STENCIL_OP_BACK_MATCH())
            {
                if (activeFace == GL_BACK)
                {
                    pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                    activeFace = GL_FRONT;
                }

                pState->diff_api.StencilOpSeparate (GL_BACK, to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].fail,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthFail,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthPass);
                CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_BACK);
                backIsSet = GL_TRUE;
            }
        }

        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].op, nbitID);
    }

    if (frontDirty)
    {
        if (!CR_STATE_STENCIL_OP_FRONT_MATCH())
        {
            if (CR_STATE_STENCIL_OP_TO_FRONT_BACK_MATCH())
            {
                if (!frontIsSet || !backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    pState->diff_api.StencilOp (to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass);

                    CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);
                    CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);

                    frontIsSet = GL_TRUE;
                    backIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_OP_FRONT_MATCH())
            {
                if (!frontIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    pState->diff_api.StencilOpSeparate (GL_FRONT, to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass);

                    CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);

                    frontIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_OP_BACK_MATCH())
            {
                if (!backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    pState->diff_api.StencilOpSeparate (GL_BACK, to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].fail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthFail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthPass);
                    CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_BACK);
                    backIsSet = GL_TRUE;
                }
            }
        }
        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT].op, nbitID);
    }


    if (backDirty)
    {
        if (!CR_STATE_STENCIL_OP_FRONT_MATCH())
        {
            if (CR_STATE_STENCIL_OP_TO_FRONT_BACK_MATCH())
            {
                if (!frontIsSet || !backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    pState->diff_api.StencilOp (to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass);

                    CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);
                    CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);

                    frontIsSet = GL_TRUE;
                    backIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_OP_FRONT_MATCH())
            {
                if (!frontIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    pState->diff_api.StencilOpSeparate (GL_FRONT, to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass);

                    CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT);

                    frontIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_OP_BACK_MATCH())
            {
                if (!backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    pState->diff_api.StencilOpSeparate (GL_BACK, to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].fail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthFail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthPass);
                    CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_BACK);
                    backIsSet = GL_TRUE;
                }
            }
        }
        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_BACK].op, nbitID);
    }

    if (CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_TWO_SIDE_BACK].op, bitID))
    {
        if (!CR_STATE_STENCIL_OP_MATCH(from, CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK, to, CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK))
        {
            if (activeFace == GL_FRONT)
            {
                pState->diff_api.ActiveStencilFaceEXT(GL_BACK);
                activeFace = GL_BACK;
            }

            pState->diff_api.StencilOp (to->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].fail,
                                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].passDepthFail,
                                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].passDepthPass);
            CR_STATE_STENCIL_OP_COPY(from, CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK, to, CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK);
        }
        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_TWO_SIDE_BACK].op, nbitID);
    }

#undef CR_STATE_STENCIL_OP_FRONT_MATCH
#undef CR_STATE_STENCIL_OP_BACK_MATCH
#undef CR_STATE_STENCIL_OP_TO_FRONT_BACK_MATCH


    if (activeFace != to->activeStencilFace)
    {
        pState->diff_api.ActiveStencilFaceEXT(activeFace);
    }

    if (CHECKDIRTY(b->activeStencilFace, bitID))
    {
        if (from->activeStencilFace != to->activeStencilFace)
        {
            /* we already did it ( see above )*/
            /* diff_api.ActiveStencilFaceEXT(to->activeStencilFace); */
            from->activeStencilFace = to->activeStencilFace;
        }
        CLEARDIRTY(b->activeStencilFace, nbitID);
    }

    if (CHECKDIRTY(b->writeMask, bitID))
    {
        if (from->writeMask != to->writeMask)
        {
            pState->diff_api.StencilMask (to->writeMask);
            from->writeMask = to->writeMask;
        }
        CLEARDIRTY(b->writeMask, nbitID);
    }
    CLEARDIRTY(b->dirty, nbitID);
}

void crStateStencilSwitch(CRStencilBits *b, CRbitvalue *bitID,
        CRContext *fromCtx, CRContext *toCtx)
{
    PCRStateTracker pState = fromCtx->pStateTracker;
    CRStencilState *from = &(fromCtx->stencil);
    CRStencilState *to = &(toCtx->stencil);
    unsigned int j, i;
    GLenum activeFace;
    GLboolean backIsSet = GL_FALSE, frontIsSet = GL_FALSE, frontBackDirty, frontDirty, backDirty;
    GLchar frontMatch = -1, backMatch = -1, toFrontBackMatch = -1;
    CRbitvalue nbitID[CR_MAX_BITARRAY];

    CRASSERT(fromCtx->pStateTracker == toCtx->pStateTracker);

    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];
    i = 0; /* silence compiler */

    if (CHECKDIRTY(b->enable, bitID))
    {
        glAble able[2];
        able[0] = pState->diff_api.Disable;
        able[1] = pState->diff_api.Enable;
        if (from->stencilTest != to->stencilTest)
        {
            CRSTATE_CHECKGLERR(able[to->stencilTest](GL_STENCIL_TEST));
            FILLDIRTY(b->enable);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->enable, nbitID);
    }
    if (CHECKDIRTY(b->enableTwoSideEXT, bitID))
    {
        glAble able[2];
        able[0] = pState->diff_api.Disable;
        able[1] = pState->diff_api.Enable;
        if (from->stencilTwoSideEXT != to->stencilTwoSideEXT)
        {
            CRSTATE_CHECKGLERR(able[to->stencilTwoSideEXT](GL_STENCIL_TEST_TWO_SIDE_EXT));
            FILLDIRTY(b->enableTwoSideEXT);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->enableTwoSideEXT, nbitID);
    }
    if (CHECKDIRTY(b->clearValue, bitID))
    {
        if (from->clearValue != to->clearValue)
        {
            CRSTATE_CHECKGLERR(pState->diff_api.ClearStencil (to->clearValue));
            FILLDIRTY(b->clearValue);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->clearValue, nbitID);
    }

    activeFace = from->activeStencilFace;

    /* func */
    frontBackDirty = CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].func, bitID);
    frontDirty = CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT].func, bitID);
    backDirty = CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_BACK].func, bitID);
#define CR_STATE_STENCIL_FUNC_FRONT_MATCH() ( \
        frontMatch >= 0 ? \
                frontMatch \
                : (frontMatch = CR_STATE_STENCIL_FUNC_MATCH(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT)))

#define CR_STATE_STENCIL_FUNC_BACK_MATCH() ( \
        backMatch >= 0 ? \
                backMatch \
                : (backMatch = CR_STATE_STENCIL_FUNC_MATCH(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_BACK)))

#define CR_STATE_STENCIL_FUNC_TO_FRONT_BACK_MATCH() ( \
        toFrontBackMatch >= 0 ? \
                toFrontBackMatch \
                : (toFrontBackMatch = CR_STATE_STENCIL_FUNC_MATCH(to, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_BACK)))

    if (frontBackDirty)
    {
        if (!CR_STATE_STENCIL_FUNC_FRONT_MATCH()
                || !CR_STATE_STENCIL_FUNC_BACK_MATCH())
        {
            if (CR_STATE_STENCIL_FUNC_TO_FRONT_BACK_MATCH())
            {
                if (activeFace == GL_BACK)
                {
                    pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                    activeFace = GL_FRONT;
                }

                CRSTATE_CHECKGLERR(pState->diff_api.StencilFunc (to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask));

                frontIsSet = GL_TRUE;
                backIsSet = GL_TRUE;
            }
            else if (!CR_STATE_STENCIL_FUNC_FRONT_MATCH())
            {
                if (activeFace == GL_BACK)
                {
                    pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                    activeFace = GL_FRONT;
                }

                CRSTATE_CHECKGLERR(pState->diff_api.StencilFuncSeparate (GL_FRONT, to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask));
                frontIsSet = GL_TRUE;
            }
            else if (!CR_STATE_STENCIL_FUNC_BACK_MATCH())
            {
                if (activeFace == GL_BACK)
                {
                    pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                    activeFace = GL_FRONT;
                }

                CRSTATE_CHECKGLERR(pState->diff_api.StencilFuncSeparate (GL_BACK, to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].func,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].ref,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].mask));
                backIsSet = GL_TRUE;
            }
            FILLDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].func);
            FILLDIRTY(b->dirty);
        }

        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].func, nbitID);
    }

    if (frontDirty)
    {
        if (!CR_STATE_STENCIL_FUNC_FRONT_MATCH())
        {
            if (CR_STATE_STENCIL_FUNC_TO_FRONT_BACK_MATCH())
            {
                if (!frontIsSet || !backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    CRSTATE_CHECKGLERR(pState->diff_api.StencilFunc (to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask));

                    frontIsSet = GL_TRUE;
                    backIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_FUNC_FRONT_MATCH())
            {
                if (!frontIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    CRSTATE_CHECKGLERR(pState->diff_api.StencilFuncSeparate (GL_FRONT, to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask));
                    frontIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_FUNC_BACK_MATCH())
            {
                if (!backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    CRSTATE_CHECKGLERR(pState->diff_api.StencilFuncSeparate (GL_BACK, to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].func,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].ref,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].mask));
                    backIsSet = GL_TRUE;
                }
            }
            FILLDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT].func);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT].func, nbitID);
    }


    if (backDirty)
    {
        if (!CR_STATE_STENCIL_FUNC_BACK_MATCH())
        {
            if (CR_STATE_STENCIL_FUNC_TO_FRONT_BACK_MATCH())
            {
                if (!frontIsSet || !backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    CRSTATE_CHECKGLERR(pState->diff_api.StencilFunc (to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask));

                    frontIsSet = GL_TRUE;
                    backIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_FUNC_FRONT_MATCH())
            {
                if (!frontIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    CRSTATE_CHECKGLERR(pState->diff_api.StencilFuncSeparate (GL_FRONT, to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask));
                    frontIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_FUNC_BACK_MATCH())
            {
                if (!backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                        activeFace = GL_FRONT;
                    }

                    CRSTATE_CHECKGLERR(pState->diff_api.StencilFuncSeparate (GL_BACK, to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].func,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].ref,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].mask));
                    backIsSet = GL_TRUE;
                }
            }
            FILLDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_BACK].func);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_BACK].func, nbitID);
    }

    if (CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_TWO_SIDE_BACK].func, bitID))
    {
        if (!CR_STATE_STENCIL_FUNC_MATCH(from, CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK, to, CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK))
        {
            if (activeFace == GL_FRONT)
            {
                CRSTATE_CHECKGLERR(pState->diff_api.ActiveStencilFaceEXT(GL_BACK));
                activeFace = GL_BACK;
            }

            CRSTATE_CHECKGLERR(pState->diff_api.StencilFunc (to->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].func,
                to->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].ref,
                to->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].mask));

            FILLDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_TWO_SIDE_BACK].func);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_TWO_SIDE_BACK].func, nbitID);
    }

#undef CR_STATE_STENCIL_FUNC_FRONT_MATCH
#undef CR_STATE_STENCIL_FUNC_BACK_MATCH
#undef CR_STATE_STENCIL_FUNC_TO_FRONT_BACK_MATCH

    /* op */
    backIsSet = GL_FALSE, frontIsSet = GL_FALSE;
    frontMatch = -1, backMatch = -1, toFrontBackMatch = -1;
    frontBackDirty = CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].op, bitID);
    frontDirty = CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT].op, bitID);
    backDirty = CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_BACK].op, bitID);

#define CR_STATE_STENCIL_OP_FRONT_MATCH() ( \
        frontMatch >= 0 ? \
                frontMatch \
                : (frontMatch = CR_STATE_STENCIL_OP_MATCH(from, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_FRONT)))

#define CR_STATE_STENCIL_OP_BACK_MATCH() ( \
        backMatch >= 0 ? \
                backMatch \
                : (backMatch = CR_STATE_STENCIL_OP_MATCH(from, CRSTATE_STENCIL_BUFFER_ID_BACK, to, CRSTATE_STENCIL_BUFFER_ID_BACK)))

#define CR_STATE_STENCIL_OP_TO_FRONT_BACK_MATCH() ( \
        toFrontBackMatch >= 0 ? \
                toFrontBackMatch \
                : (toFrontBackMatch = CR_STATE_STENCIL_OP_MATCH(to, CRSTATE_STENCIL_BUFFER_ID_FRONT, to, CRSTATE_STENCIL_BUFFER_ID_BACK)))

    if (frontBackDirty)
    {
        if (!CR_STATE_STENCIL_OP_FRONT_MATCH()
                || !CR_STATE_STENCIL_OP_BACK_MATCH())
        {
            if (CR_STATE_STENCIL_OP_TO_FRONT_BACK_MATCH())
            {
                if (activeFace == GL_BACK)
                {
                    CRSTATE_CHECKGLERR(pState->diff_api.ActiveStencilFaceEXT(GL_FRONT));
                    activeFace = GL_FRONT;
                }

                CRSTATE_CHECKGLERR(pState->diff_api.StencilOp (to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass));

                frontIsSet = GL_TRUE;
                backIsSet = GL_TRUE;
            }
            else if (!CR_STATE_STENCIL_OP_FRONT_MATCH())
            {
                if (activeFace == GL_BACK)
                {
                    CRSTATE_CHECKGLERR(pState->diff_api.ActiveStencilFaceEXT(GL_FRONT));
                    activeFace = GL_FRONT;
                }

                CRSTATE_CHECKGLERR(pState->diff_api.StencilOpSeparate (GL_FRONT, to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass));
                frontIsSet = GL_TRUE;
            }
            else if (!CR_STATE_STENCIL_OP_BACK_MATCH())
            {
                if (activeFace == GL_BACK)
                {
                    CRSTATE_CHECKGLERR(pState->diff_api.ActiveStencilFaceEXT(GL_FRONT));
                    activeFace = GL_FRONT;
                }

                CRSTATE_CHECKGLERR(pState->diff_api.StencilOpSeparate (GL_BACK, to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].fail,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthFail,
                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthPass));
                backIsSet = GL_TRUE;
            }
            FILLDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].op);
            FILLDIRTY(b->dirty);
        }

        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].op, nbitID);
    }

    if (frontDirty)
    {
        if (!CR_STATE_STENCIL_OP_FRONT_MATCH())
        {
            if (CR_STATE_STENCIL_OP_TO_FRONT_BACK_MATCH())
            {
                if (!frontIsSet || !backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        CRSTATE_CHECKGLERR(pState->diff_api.ActiveStencilFaceEXT(GL_FRONT));
                        activeFace = GL_FRONT;
                    }

                    CRSTATE_CHECKGLERR(pState->diff_api.StencilOp (to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass));

                    frontIsSet = GL_TRUE;
                    backIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_OP_FRONT_MATCH())
            {
                if (!frontIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        CRSTATE_CHECKGLERR(pState->diff_api.ActiveStencilFaceEXT(GL_FRONT));
                        activeFace = GL_FRONT;
                    }

                    CRSTATE_CHECKGLERR(pState->diff_api.StencilOpSeparate (GL_FRONT, to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass));
                    frontIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_OP_BACK_MATCH())
            {
                if (!backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        CRSTATE_CHECKGLERR(pState->diff_api.ActiveStencilFaceEXT(GL_FRONT));
                        activeFace = GL_FRONT;
                    }

                    CRSTATE_CHECKGLERR(pState->diff_api.StencilOpSeparate (GL_BACK, to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].fail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthFail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthPass));
                    backIsSet = GL_TRUE;
                }
            }

            FILLDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT].op);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT].op, nbitID);
    }


    if (backDirty)
    {
        if (!CR_STATE_STENCIL_OP_BACK_MATCH())
        {
            if (CR_STATE_STENCIL_OP_TO_FRONT_BACK_MATCH())
            {
                if (!frontIsSet || !backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        CRSTATE_CHECKGLERR(pState->diff_api.ActiveStencilFaceEXT(GL_FRONT));
                        activeFace = GL_FRONT;
                    }

                    CRSTATE_CHECKGLERR(pState->diff_api.StencilOp (to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass));

                    frontIsSet = GL_TRUE;
                    backIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_OP_FRONT_MATCH())
            {
                if (!frontIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        CRSTATE_CHECKGLERR(pState->diff_api.ActiveStencilFaceEXT(GL_FRONT));
                        activeFace = GL_FRONT;
                    }

                    CRSTATE_CHECKGLERR(pState->diff_api.StencilOpSeparate (GL_FRONT, to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass));
                    frontIsSet = GL_TRUE;
                }
            }
            else if (!CR_STATE_STENCIL_OP_BACK_MATCH())
            {
                if (!backIsSet)
                {
                    if (activeFace == GL_BACK)
                    {
                        CRSTATE_CHECKGLERR(pState->diff_api.ActiveStencilFaceEXT(GL_FRONT));
                        activeFace = GL_FRONT;
                    }

                    CRSTATE_CHECKGLERR(pState->diff_api.StencilOpSeparate (GL_BACK, to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].fail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthFail,
                        to->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthPass));
                    backIsSet = GL_TRUE;
                }
            }

            FILLDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_BACK].op);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_BACK].op, nbitID);
    }

    if (CHECKDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_TWO_SIDE_BACK].op, bitID))
    {
        if (!CR_STATE_STENCIL_OP_MATCH(from, CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK, to, CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK))
        {
            if (activeFace == GL_FRONT)
            {
                CRSTATE_CHECKGLERR(pState->diff_api.ActiveStencilFaceEXT(GL_BACK));
                activeFace = GL_BACK;
            }

            CRSTATE_CHECKGLERR(pState->diff_api.StencilOp (to->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].fail,
                                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].passDepthFail,
                                    to->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].passDepthPass));

            FILLDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_TWO_SIDE_BACK].op);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_TWO_SIDE_BACK].op, nbitID);
    }

#undef CR_STATE_STENCIL_OP_FRONT_MATCH
#undef CR_STATE_STENCIL_OP_BACK_MATCH
#undef CR_STATE_STENCIL_OP_TO_FRONT_BACK_MATCH

    if (activeFace != to->activeStencilFace)
    {
        CRSTATE_CHECKGLERR(pState->diff_api.ActiveStencilFaceEXT(activeFace));
    }

    if (CHECKDIRTY(b->activeStencilFace, bitID))
    {
        if (from->activeStencilFace != to->activeStencilFace)
        {
            /* we already did it ( see above )*/
            /* diff_api.ActiveStencilFaceEXT(to->activeStencilFace); */
            FILLDIRTY(b->activeStencilFace);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->activeStencilFace, nbitID);
    }

    if (CHECKDIRTY(b->writeMask, bitID))
    {
        if (from->writeMask != to->writeMask)
        {
            CRSTATE_CHECKGLERR(pState->diff_api.StencilMask (to->writeMask));
            FILLDIRTY(b->writeMask);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->writeMask, nbitID);
    }

    CLEARDIRTY(b->dirty, nbitID);
}

