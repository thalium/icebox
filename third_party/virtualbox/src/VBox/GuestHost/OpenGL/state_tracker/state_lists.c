/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "cr_mem.h"
#include "state.h"
#include "state/cr_statetypes.h"
#include "state_internals.h"

void crStateListsDestroy(CRContext *ctx)
{
    /* nothing - dlists are in shared state */
    (void)ctx;
}

void crStateListsInit(CRContext *ctx)
{
    CRListsState *l = &ctx->lists;
    CRStateBits *sb = GetCurrentBits(ctx->pStateTracker);
    CRListsBits *lb = &(sb->lists);

    l->newEnd = GL_FALSE;
    l->mode = 0;
    l->currentIndex = 0;
    l->base = 0;

    RESET(lb->base, ctx->bitid);
    RESET(lb->dirty, ctx->bitid);
}

/*#define CRSTATE_DEBUG_QUERY_HW_STATE*/

#ifndef CRSTATE_DEBUG_QUERY_HW_STATE
# define CRSTATE_SET_CAP(state, value, format) g->state=value
# define CR_STATE_SETTEX_MSG(state, st, hw)
# define CR_STATE_SETMAT_MSG(state, st, hw)
#else
# define CRSTATE_SET_CAP(state, value, format)                                                      \
    if (g->state!=value) {                                                                          \
        crDebug("crStateQueryHWState fixed %s from "format" to "format, #state, g->state, value);   \
        g->state=value;                                                                             \
    }
# define CR_STATE_SETTEX_MSG(state, st, hw) crDebug("crStateQueryHWState fixed %s from %i to %i", state, st, hw)
# define CR_STATE_SETMAT_MSG(state, st, hw)                                             \
    {                                                                                   \
    crDebug("crStateQueryHWState fixed %s", state);                                     \
    crDebug("st: [%f, %f, %f, %f] [%f, %f, %f, %f] [%f, %f, %f, %f] [%f, %f, %f, %f]",  \
            st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[7],                     \
            st[8], st[9], st[10], st[11], st[12], st[13], st[14], st[15]);              \
    crDebug("hw: [%f, %f, %f, %f] [%f, %f, %f, %f] [%f, %f, %f, %f] [%f, %f, %f, %f]",  \
            hw[0], hw[1], hw[2], hw[3], hw[4], hw[5], hw[6], hw[7],                     \
            hw[8], hw[9], hw[10], hw[11], hw[12], hw[13], hw[14], hw[15]);              \
    }
#endif

#define CRSTATE_SET_ENABLED(state, cap) CRSTATE_SET_CAP(state, pState->diff_api.IsEnabled(cap), "%u")

#define CRSTATE_SET_ENUM(state, cap) {GLenum _e=g->state; pState->diff_api.GetIntegerv(cap, (GLint *)&_e); CRSTATE_SET_CAP(state, _e, "%#x");}
#define CRSTATE_SET_FLOAT(state, cap) {GLfloat _f=g->state; pState->diff_api.GetFloatv(cap, &_f); CRSTATE_SET_CAP(state, _f, "%f");}
#define CRSTATE_SET_INT(state, cap) {GLint _i=g->state; pState->diff_api.GetIntegerv(cap, &_i); CRSTATE_SET_CAP(state, _i, "%i");}
#define CRSTATE_SET_BOOL(state, cap) {GLboolean _b=g->state; pState->diff_api.GetBooleanv(cap, &_b); CRSTATE_SET_CAP(state, _b, "%u");}

#define CRSTATE_SET_COLORF(state, cap)              \
    {                                               \
        GLfloat value[4];                           \
        value[0]=g->state.r;                        \
        value[1]=g->state.g;                        \
        value[2]=g->state.b;                        \
        value[3]=g->state.a;                        \
        pState->diff_api.GetFloatv(cap, &value[0]); \
        CRSTATE_SET_CAP(state.r, value[0], "%f");   \
        CRSTATE_SET_CAP(state.g, value[1], "%f");   \
        CRSTATE_SET_CAP(state.b, value[2], "%f");   \
        CRSTATE_SET_CAP(state.a, value[3], "%f");   \
    }

#define CRSTATE_SET_TEXTURE(state, cap, target)                                             \
    {                                                                                       \
        GLint _stex, _hwtex;                                                                \
        _stex = _hwtex = crStateGetTextureObjHWID(pState, g->state);                        \
        pState->diff_api.GetIntegerv(cap, &_hwtex);                                         \
        if (_stex!=_hwtex)                                                                  \
        {                                                                                   \
            CR_STATE_SETTEX_MSG(#state, _stex, _hwtex);                                     \
            crStateBindTexture(pState, target, crStateTextureHWIDtoID(pState, _hwtex));     \
        }                                                                                   \
    }

#define _CRSTATE_SET_4F_RGBA(state, p1, p2, func)   \
    {                                               \
        GLfloat value[4];                           \
        value[0]=g->state.r;                        \
        value[1]=g->state.g;                        \
        value[2]=g->state.b;                        \
        value[3]=g->state.a;                        \
        pState->diff_api.func(p1, p2, &value[0]);   \
        CRSTATE_SET_CAP(state.r, value[0], "%f");   \
        CRSTATE_SET_CAP(state.g, value[1], "%f");   \
        CRSTATE_SET_CAP(state.b, value[2], "%f");   \
        CRSTATE_SET_CAP(state.a, value[3], "%f");   \
    }

#define _CRSTATE_SET_4F_XYZW(state, p1, p2, func)   \
    {                                               \
        GLfloat value[4];                           \
        value[0]=g->state.x;                        \
        value[1]=g->state.y;                        \
        value[2]=g->state.z;                        \
        value[3]=g->state.w;                        \
        pState->diff_api.func(p1, p2, &value[0]);   \
        CRSTATE_SET_CAP(state.x, value[0], "%f");   \
        CRSTATE_SET_CAP(state.y, value[1], "%f");   \
        CRSTATE_SET_CAP(state.z, value[2], "%f");   \
        CRSTATE_SET_CAP(state.w, value[3], "%f");   \
    }

#define CRSTATE_SET_TEXGEN_4F(state, coord, pname) _CRSTATE_SET_4F_XYZW(state, coord, pname, GetTexGenfv)
#define CRSTATE_SET_TEXGEN_I(state, coord, pname) {GLint _i=g->state; pState->diff_api.GetTexGeniv(coord, pname, &_i); CRSTATE_SET_CAP(state, _i, "%i");}

#define CRSTATE_SET_TEXENV_I(state, target, pname) {GLint _i=g->state; pState->diff_api.GetTexEnviv(target, pname, &_i); CRSTATE_SET_CAP(state, _i, "%i");}
#define CRSTATE_SET_TEXENV_F(state, target, pname) {GLfloat _f=g->state; pState->diff_api.GetTexEnvfv(target, pname, &_f); CRSTATE_SET_CAP(state, _f, "%f");}
#define CRSTATE_SET_TEXENV_COLOR(state, target, pname) _CRSTATE_SET_4F_RGBA(state, target, pname, GetTexEnvfv)

#define CRSTATE_SET_MATERIAL_COLOR(state, face, pname) _CRSTATE_SET_4F_RGBA(state, face, pname, GetMaterialfv)
#define CRSTATE_SET_MATERIAL_F(state, face, pname) {GLfloat _f=g->state; pState->diff_api.GetMaterialfv(face, pname, &_f); CRSTATE_SET_CAP(state, _f, "%f");}

#define CRSTATE_SET_LIGHT_COLOR(state, light, pname) _CRSTATE_SET_4F_RGBA(state, light, pname, GetLightfv)
#define CRSTATE_SET_LIGHT_F(state, light, pname) {GLfloat _f=g->state; pState->diff_api.GetLightfv(light, pname, &_f); CRSTATE_SET_CAP(state, _f, "%f");}
#define CRSTATE_SET_LIGHT_4F(state, light, pname) _CRSTATE_SET_4F_XYZW(state, light, pname, GetLightfv)
#define CRSTATE_SET_LIGHT_3F(state, light, pname)       \
    {                                                   \
        GLfloat value[3];                               \
        value[0]=g->state.x;                            \
        value[1]=g->state.y;                            \
        value[2]=g->state.z;                            \
        pState->diff_api.GetLightfv(light, pname, &value[0]); \
        CRSTATE_SET_CAP(state.x, value[0], "%f");       \
        CRSTATE_SET_CAP(state.y, value[1], "%f");       \
        CRSTATE_SET_CAP(state.z, value[2], "%f");       \
    }

#define CRSTATE_SET_CLIPPLANE_4D(state, plane)      \
    {                                               \
        GLdouble value[4];                          \
        value[0]=g->state.x;                        \
        value[1]=g->state.y;                        \
        value[2]=g->state.z;                        \
        value[3]=g->state.w;                        \
        pState->diff_api.GetClipPlane(plane, &value[0]); \
        CRSTATE_SET_CAP(state.x, value[0], "%G");   \
        CRSTATE_SET_CAP(state.y, value[1], "%G");   \
        CRSTATE_SET_CAP(state.z, value[2], "%G");   \
        CRSTATE_SET_CAP(state.w, value[3], "%G");   \
    }

#define CRSTATE_SET_MATRIX(state, cap)                      \
    {                                                       \
        GLfloat f[16], sm[16];                              \
        crMatrixGetFloats(&f[0], g->state);                 \
        crMemcpy(&sm[0], &f[0], 16*sizeof(GLfloat));        \
        pState->diff_api.GetFloatv(cap, &f[0]);             \
        if (crMemcmp(&f[0], &sm[0], 16*sizeof(GLfloat)))    \
        {                                                   \
            CR_STATE_SETMAT_MSG(#state, sm, f);             \
            crMatrixInitFromFloats(g->state, &f[0]);        \
        }                                                   \
    }

void STATE_APIENTRY crStateQueryHWState(PCRStateTracker pState, GLuint fbFbo, GLuint bbFbo)
{
    CRContext *g = GetCurrentContext(pState);
    CRStateBits *sb = GetCurrentBits(pState);
    CRbitvalue /* *bitID=g->bitid, */ *negbitID=g->neg_bitid;

    CRASSERT(pState->fVBoxEnableDiffOnMakeCurrent);

    crStateSyncHWErrorState(g);

    if (CHECKDIRTY(sb->buffer.dirty, negbitID))
    {
        if (CHECKDIRTY(sb->buffer.enable, negbitID))
        {
            CRSTATE_SET_ENABLED(buffer.depthTest, GL_DEPTH_TEST);
            CRSTATE_SET_ENABLED(buffer.blend, GL_BLEND);
            CRSTATE_SET_ENABLED(buffer.alphaTest, GL_ALPHA_TEST);
            CRSTATE_SET_ENABLED(buffer.logicOp, GL_COLOR_LOGIC_OP);
            CRSTATE_SET_ENABLED(buffer.indexLogicOp, GL_INDEX_LOGIC_OP);
            CRSTATE_SET_ENABLED(buffer.dither, GL_DITHER);
        }

        if (CHECKDIRTY(sb->buffer.alphaFunc, negbitID))
        {
            CRSTATE_SET_ENUM(buffer.alphaTestFunc, GL_ALPHA_TEST_FUNC);
            CRSTATE_SET_FLOAT(buffer.alphaTestRef, GL_ALPHA_TEST_REF);
        }

        if (CHECKDIRTY(sb->buffer.depthFunc, negbitID))
        {
            CRSTATE_SET_ENUM(buffer.depthFunc, GL_DEPTH_FUNC);
        }

        if (CHECKDIRTY(sb->buffer.blendFunc, negbitID))
        {
            CRSTATE_SET_ENUM(buffer.blendSrcRGB, GL_BLEND_SRC);
            CRSTATE_SET_ENUM(buffer.blendDstRGB, GL_BLEND_DST);
        }

        if (CHECKDIRTY(sb->buffer.logicOp, negbitID))
        {
            CRSTATE_SET_ENUM(buffer.logicOpMode, GL_LOGIC_OP_MODE);
        }

/* seems to always match previous .logicOp
        if (CHECKDIRTY(sb->buffer.indexLogicOp, negbitID))
        {
            CRSTATE_SET_ENUM(buffer.logicOpMode, GL_LOGIC_OP_MODE);
        }
*/

        if (CHECKDIRTY(sb->buffer.drawBuffer, negbitID))
        {
            GLuint buf = 0;
            pState->diff_api.GetIntegerv(GL_DRAW_BUFFER, (GLint *)&buf);

            if (buf == GL_COLOR_ATTACHMENT0_EXT && (bbFbo || fbFbo))
            {
                GLuint binding = 0;
                pState->diff_api.GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint *)&binding);
                if (!binding)
                {
                    crWarning("HW state synch: GL_DRAW_FRAMEBUFFER_BINDING is NULL");
                }

                if (bbFbo && binding == bbFbo)
                {
                    g->buffer.drawBuffer = GL_BACK;
                }
                else if (fbFbo && binding == fbFbo)
                {
                    g->buffer.drawBuffer = GL_FRONT;
                }
                else
                {
                    g->buffer.drawBuffer = buf;
                }
            }
            else
            {
                g->buffer.drawBuffer = buf;
            }
        }

        if (CHECKDIRTY(sb->buffer.readBuffer, negbitID))
        {
            GLuint buf = 0;
            pState->diff_api.GetIntegerv(GL_READ_BUFFER, (GLint *)&buf);

            if (buf == GL_COLOR_ATTACHMENT0_EXT && (bbFbo || fbFbo))
            {
                GLuint binding = 0;
                pState->diff_api.GetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (GLint *)&binding);
                if (!binding)
                {
                    crWarning("HW state synch: GL_READ_FRAMEBUFFER_BINDING is NULL");
                }

                if (bbFbo && binding == bbFbo)
                {
                    g->buffer.readBuffer = GL_BACK;
                }
                else if (fbFbo && binding == fbFbo)
                {
                    g->buffer.readBuffer = GL_FRONT;
                }
                else
                {
                    g->buffer.readBuffer = buf;
                }
            }
            else
            {
                g->buffer.readBuffer = buf;
            }
        }

        if (CHECKDIRTY(sb->buffer.indexMask, negbitID))
        {
            CRSTATE_SET_INT(buffer.indexWriteMask, GL_INDEX_WRITEMASK);
        }

        if (CHECKDIRTY(sb->buffer.colorWriteMask, negbitID))
        {       
            GLboolean value[4];
            value[0]=g->buffer.colorWriteMask.r;
            value[1]=g->buffer.colorWriteMask.g;
            value[2]=g->buffer.colorWriteMask.b;
            value[3]=g->buffer.colorWriteMask.a;
            pState->diff_api.GetBooleanv(GL_COLOR_WRITEMASK, &value[0]);

            CRSTATE_SET_CAP(buffer.colorWriteMask.r, value[0], "%u");
            CRSTATE_SET_CAP(buffer.colorWriteMask.g, value[1], "%u");
            CRSTATE_SET_CAP(buffer.colorWriteMask.b, value[2], "%u");
            CRSTATE_SET_CAP(buffer.colorWriteMask.a, value[3], "%u");
        }

        if (CHECKDIRTY(sb->buffer.clearColor, negbitID))
        {
            CRSTATE_SET_COLORF(buffer.colorClearValue, GL_COLOR_CLEAR_VALUE);
        }

        if (CHECKDIRTY(sb->buffer.clearIndex, negbitID))
        {
            CRSTATE_SET_FLOAT(buffer.indexClearValue, GL_INDEX_CLEAR_VALUE);
        }

        if (CHECKDIRTY(sb->buffer.clearDepth, negbitID))
        {
            CRSTATE_SET_FLOAT(buffer.depthClearValue, GL_DEPTH_CLEAR_VALUE);
        }

        if (CHECKDIRTY(sb->buffer.clearAccum, negbitID))
        {
            CRSTATE_SET_COLORF(buffer.accumClearValue, GL_ACCUM_CLEAR_VALUE);
        }

        if (CHECKDIRTY(sb->buffer.depthMask, negbitID))
        {
            CRSTATE_SET_BOOL(buffer.depthMask, GL_DEPTH_WRITEMASK);
        }

#ifdef CR_EXT_blend_color
        if (CHECKDIRTY(sb->buffer.blendColor, negbitID))
        {
            CRSTATE_SET_COLORF(buffer.blendColor, GL_BLEND_COLOR);
        }
#endif
#if defined(CR_EXT_blend_minmax) || defined(CR_EXT_blend_subtract) || defined(CR_EXT_blend_logic_op)
        if (CHECKDIRTY(sb->buffer.blendEquation, negbitID))
        {
            CRSTATE_SET_ENUM(buffer.blendEquation, GL_BLEND_EQUATION_EXT);
        }
#endif
#if defined(CR_EXT_blend_func_separate)
        if (CHECKDIRTY(sb->buffer.blendFuncSeparate, negbitID))
        {
            CRSTATE_SET_ENUM(buffer.blendSrcRGB, GL_BLEND_SRC_RGB_EXT);
            CRSTATE_SET_ENUM(buffer.blendDstRGB, GL_BLEND_DST_RGB_EXT);
            CRSTATE_SET_ENUM(buffer.blendSrcA, GL_BLEND_SRC_ALPHA_EXT);
            CRSTATE_SET_ENUM(buffer.blendDstA, GL_BLEND_DST_ALPHA_EXT);
        }
#endif
    }

    if (CHECKDIRTY(sb->stencil.dirty, negbitID))
    {
        GLenum activeFace;
        GLboolean backIsSet = GL_FALSE, frontIsSet = GL_FALSE;

        if (CHECKDIRTY(sb->stencil.enable, negbitID))
        {
            CRSTATE_SET_ENABLED(stencil.stencilTest, GL_STENCIL_TEST);
        }

        if (CHECKDIRTY(sb->stencil.enableTwoSideEXT, negbitID))
        {
            CRSTATE_SET_ENABLED(stencil.stencilTwoSideEXT, GL_STENCIL_TEST_TWO_SIDE_EXT);
        }

        if (CHECKDIRTY(sb->stencil.activeStencilFace, negbitID))
        {
            CRSTATE_SET_ENUM(stencil.activeStencilFace, GL_ACTIVE_STENCIL_FACE_EXT);
        }

        activeFace = g->stencil.activeStencilFace;


#define CRSTATE_SET_STENCIL_FUNC(_idx, _suff) do { \
        CRSTATE_SET_ENUM(stencil.buffers[(_idx)].func, GL_STENCIL##_suff##FUNC); \
        CRSTATE_SET_INT(stencil.buffers[(_idx)].ref, GL_STENCIL##_suff##REF); \
        CRSTATE_SET_INT(stencil.buffers[(_idx)].mask, GL_STENCIL##_suff##VALUE_MASK); \
    } while (0)

#define CRSTATE_SET_STENCIL_OP(_idx, _suff) do { \
        CRSTATE_SET_ENUM(stencil.buffers[(_idx)].fail, GL_STENCIL##_suff##FAIL); \
        CRSTATE_SET_ENUM(stencil.buffers[(_idx)].passDepthFail, GL_STENCIL##_suff##PASS_DEPTH_FAIL); \
        CRSTATE_SET_ENUM(stencil.buffers[(_idx)].passDepthPass, GL_STENCIL##_suff##PASS_DEPTH_PASS); \
    } while (0)

        /* func */

        if (CHECKDIRTY(sb->stencil.bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_BACK].func, negbitID))
        {
            /* this if branch is not needed here actually, just in case ogl drivers misbehave */
            if (activeFace == GL_BACK)
            {
                pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                activeFace = GL_FRONT;
            }

            CRSTATE_SET_STENCIL_FUNC(CRSTATE_STENCIL_BUFFER_ID_BACK, _BACK_);
            backIsSet = GL_TRUE;
        }

        if (CHECKDIRTY(sb->stencil.bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT].func, negbitID))
        {
            if (activeFace == GL_BACK)
            {
                pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                activeFace = GL_FRONT;
            }
            CRSTATE_SET_STENCIL_FUNC(CRSTATE_STENCIL_BUFFER_ID_FRONT, _);
            frontIsSet = GL_TRUE;
        }

        if ((!frontIsSet || !backIsSet) && CHECKDIRTY(sb->stencil.bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].func, negbitID))
        {
            if (activeFace == GL_BACK)
            {
                pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                activeFace = GL_FRONT;
            }
            CRSTATE_SET_STENCIL_FUNC(CRSTATE_STENCIL_BUFFER_ID_FRONT, _);
            if (!backIsSet)
            {
                g->stencil.buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].func = g->stencil.buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func;
                g->stencil.buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].ref = g->stencil.buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref;
                g->stencil.buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].mask = g->stencil.buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask;
            }
        }

        /* op */
        backIsSet = GL_FALSE, frontIsSet = GL_FALSE;

        if (CHECKDIRTY(sb->stencil.bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_BACK].op, negbitID))
        {
            /* this if branch is not needed here actually, just in case ogl drivers misbehave */
            if (activeFace == GL_BACK)
            {
                pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                activeFace = GL_FRONT;
            }

            CRSTATE_SET_STENCIL_OP(CRSTATE_STENCIL_BUFFER_ID_BACK, _BACK_);
            backIsSet = GL_TRUE;
        }

        if (CHECKDIRTY(sb->stencil.bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT].op, negbitID))
        {
            if (activeFace == GL_BACK)
            {
                pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                activeFace = GL_FRONT;
            }
            CRSTATE_SET_STENCIL_OP(CRSTATE_STENCIL_BUFFER_ID_FRONT, _);
            frontIsSet = GL_TRUE;
        }

        if ((!frontIsSet || !backIsSet) && CHECKDIRTY(sb->stencil.bufferRefs[CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK].op, negbitID))
        {
            if (activeFace == GL_BACK)
            {
                pState->diff_api.ActiveStencilFaceEXT(GL_FRONT);
                activeFace = GL_FRONT;
            }
            CRSTATE_SET_STENCIL_OP(CRSTATE_STENCIL_BUFFER_ID_FRONT, _);
            if (!backIsSet)
            {
                g->stencil.buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].fail = g->stencil.buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail;
                g->stencil.buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthFail = g->stencil.buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail;
                g->stencil.buffers[CRSTATE_STENCIL_BUFFER_ID_BACK].passDepthPass = g->stencil.buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass;
            }
        }

        if (CHECKDIRTY(sb->stencil.clearValue, negbitID))
        {
            CRSTATE_SET_INT(stencil.clearValue, GL_STENCIL_CLEAR_VALUE);
        }

        if (CHECKDIRTY(sb->stencil.writeMask, negbitID))
        {
            CRSTATE_SET_INT(stencil.writeMask, GL_STENCIL_WRITEMASK);
        }
    }

    if (CHECKDIRTY(sb->texture.dirty, negbitID))
    {
        unsigned int i, activeUnit = g->texture.curTextureUnit;

        for (i=0; i<g->limits.maxTextureUnits; ++i)
        {
            if (CHECKDIRTY(sb->texture.enable[i], negbitID))
            {
                if (i!=activeUnit)
                {
                    pState->diff_api.ActiveTextureARB(i + GL_TEXTURE0_ARB);
                    activeUnit=i;
                }
                CRSTATE_SET_ENABLED(texture.unit[i].enabled1D, GL_TEXTURE_1D);
                CRSTATE_SET_ENABLED(texture.unit[i].enabled2D, GL_TEXTURE_2D);
#ifdef CR_OPENGL_VERSION_1_2
                CRSTATE_SET_ENABLED(texture.unit[i].enabled3D, GL_TEXTURE_3D);
#endif
#ifdef CR_ARB_texture_cube_map
                if (g->extensions.ARB_texture_cube_map)
                {
                    CRSTATE_SET_ENABLED(texture.unit[i].enabledCubeMap, GL_TEXTURE_CUBE_MAP_ARB);
                }
#endif
#ifdef CR_NV_texture_rectangle
                if (g->extensions.NV_texture_rectangle)
                {
                    CRSTATE_SET_ENABLED(texture.unit[i].enabledRect, GL_TEXTURE_RECTANGLE_NV);
                }
#endif

                CRSTATE_SET_ENABLED(texture.unit[i].textureGen.s, GL_TEXTURE_GEN_S);
                CRSTATE_SET_ENABLED(texture.unit[i].textureGen.t, GL_TEXTURE_GEN_T);
                CRSTATE_SET_ENABLED(texture.unit[i].textureGen.r, GL_TEXTURE_GEN_R);
                CRSTATE_SET_ENABLED(texture.unit[i].textureGen.q, GL_TEXTURE_GEN_Q);
            }

            if (CHECKDIRTY(sb->texture.current[i], negbitID))
            {
                if (i!=activeUnit)
                {
                    pState->diff_api.ActiveTextureARB(i + GL_TEXTURE0_ARB);
                    activeUnit=i;
                }

                CRSTATE_SET_TEXTURE(texture.unit[i].currentTexture1D, GL_TEXTURE_BINDING_1D, GL_TEXTURE_1D);
                CRSTATE_SET_TEXTURE(texture.unit[i].currentTexture2D, GL_TEXTURE_BINDING_2D, GL_TEXTURE_2D);
#ifdef CR_OPENGL_VERSION_1_2
                CRSTATE_SET_TEXTURE(texture.unit[i].currentTexture3D, GL_TEXTURE_BINDING_3D, GL_TEXTURE_3D);
#endif
#ifdef CR_ARB_texture_cube_map
                if (g->extensions.ARB_texture_cube_map)
                {
                    CRSTATE_SET_TEXTURE(texture.unit[i].currentTextureCubeMap, GL_TEXTURE_BINDING_CUBE_MAP_ARB, GL_TEXTURE_CUBE_MAP_ARB);
                }
#endif
#ifdef CR_NV_texture_rectangle
                if (g->extensions.NV_texture_rectangle)
                {
                    CRSTATE_SET_TEXTURE(texture.unit[i].currentTextureRect, GL_TEXTURE_BINDING_RECTANGLE_NV, GL_TEXTURE_RECTANGLE_NV);
                }
#endif
            }

            if (CHECKDIRTY(sb->texture.objGen[i], negbitID))
            {
                if (i!=activeUnit)
                {
                    pState->diff_api.ActiveTextureARB(i + GL_TEXTURE0_ARB);
                    activeUnit=i;
                }

                CRSTATE_SET_TEXGEN_4F(texture.unit[i].objSCoeff, GL_S, GL_OBJECT_PLANE);
                CRSTATE_SET_TEXGEN_4F(texture.unit[i].objTCoeff, GL_T, GL_OBJECT_PLANE);
                CRSTATE_SET_TEXGEN_4F(texture.unit[i].objRCoeff, GL_R, GL_OBJECT_PLANE);
                CRSTATE_SET_TEXGEN_4F(texture.unit[i].objQCoeff, GL_Q, GL_OBJECT_PLANE);
            }

            if (CHECKDIRTY(sb->texture.eyeGen[i], negbitID))
            {
                if (i!=activeUnit)
                {
                    pState->diff_api.ActiveTextureARB(i + GL_TEXTURE0_ARB);
                    activeUnit=i;
                }

                CRSTATE_SET_TEXGEN_4F(texture.unit[i].eyeSCoeff, GL_S, GL_EYE_PLANE);
                CRSTATE_SET_TEXGEN_4F(texture.unit[i].eyeTCoeff, GL_T, GL_EYE_PLANE);
                CRSTATE_SET_TEXGEN_4F(texture.unit[i].eyeRCoeff, GL_R, GL_EYE_PLANE);
                CRSTATE_SET_TEXGEN_4F(texture.unit[i].eyeQCoeff, GL_Q, GL_EYE_PLANE);
            }

            if (CHECKDIRTY(sb->texture.genMode[i], negbitID))
            {
                if (i!=activeUnit)
                {
                    pState->diff_api.ActiveTextureARB(i + GL_TEXTURE0_ARB);
                    activeUnit=i;
                }

                CRSTATE_SET_TEXGEN_I(texture.unit[i].gen.s, GL_S, GL_TEXTURE_GEN_MODE);
                CRSTATE_SET_TEXGEN_I(texture.unit[i].gen.t, GL_T, GL_TEXTURE_GEN_MODE);
                CRSTATE_SET_TEXGEN_I(texture.unit[i].gen.r, GL_R, GL_TEXTURE_GEN_MODE);
                CRSTATE_SET_TEXGEN_I(texture.unit[i].gen.q, GL_Q, GL_TEXTURE_GEN_MODE);
            }

            if (CHECKDIRTY(sb->texture.envBit[i], negbitID))
            {
                if (i!=activeUnit)
                {
                    pState->diff_api.ActiveTextureARB(i + GL_TEXTURE0_ARB);
                    activeUnit=i;
                }

                CRSTATE_SET_TEXENV_I(texture.unit[i].envMode, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE);
                CRSTATE_SET_TEXENV_COLOR(texture.unit[i].envColor, GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineModeRGB, GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineModeA, GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineSourceRGB[0], GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineSourceRGB[1], GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineSourceRGB[2], GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineSourceA[0], GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineSourceA[1], GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineSourceA[2], GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineOperandRGB[0], GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineOperandRGB[1], GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineOperandRGB[2], GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineOperandA[0], GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineOperandA[1], GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB);
                CRSTATE_SET_TEXENV_I(texture.unit[i].combineOperandA[2], GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB);
                CRSTATE_SET_TEXENV_F(texture.unit[i].combineScaleRGB, GL_TEXTURE_ENV, GL_RGB_SCALE_ARB);
                CRSTATE_SET_TEXENV_F(texture.unit[i].combineScaleA, GL_TEXTURE_ENV, GL_ALPHA_SCALE);
            }
        }
        if (activeUnit!=g->texture.curTextureUnit)
        {
            pState->diff_api.ActiveTextureARB(g->texture.curTextureUnit + GL_TEXTURE0_ARB);
        }
    }

    if (CHECKDIRTY(sb->lighting.dirty, negbitID))
    {
        int i;

        if (CHECKDIRTY(sb->lighting.enable, negbitID))
        {
            CRSTATE_SET_ENABLED(lighting.lighting, GL_LIGHTING);
            CRSTATE_SET_ENABLED(lighting.colorMaterial, GL_COLOR_MATERIAL);
            CRSTATE_SET_ENABLED(lighting.colorSumEXT, GL_COLOR_SUM_EXT);
        }

        if (CHECKDIRTY(sb->lighting.shadeModel, negbitID))
        {
            CRSTATE_SET_ENUM(lighting.shadeModel, GL_SHADE_MODEL);
        }

        if (CHECKDIRTY(sb->lighting.colorMaterial, negbitID))
        {
            CRSTATE_SET_ENUM(lighting.colorMaterialFace, GL_COLOR_MATERIAL_FACE);
            CRSTATE_SET_ENUM(lighting.colorMaterialMode, GL_COLOR_MATERIAL_PARAMETER);
        }

        if (CHECKDIRTY(sb->lighting.lightModel, negbitID))
        {
            CRSTATE_SET_COLORF(lighting.lightModelAmbient, GL_LIGHT_MODEL_AMBIENT);
            CRSTATE_SET_BOOL(lighting.lightModelLocalViewer, GL_LIGHT_MODEL_LOCAL_VIEWER);
            CRSTATE_SET_BOOL(lighting.lightModelTwoSide, GL_LIGHT_MODEL_TWO_SIDE);
            CRSTATE_SET_ENUM(lighting.lightModelColorControlEXT, GL_LIGHT_MODEL_COLOR_CONTROL);
        }

        if (CHECKDIRTY(sb->lighting.material, negbitID))
        {
            CRSTATE_SET_MATERIAL_COLOR(lighting.ambient[0], GL_FRONT, GL_AMBIENT);
            CRSTATE_SET_MATERIAL_COLOR(lighting.ambient[1], GL_BACK, GL_AMBIENT);
            CRSTATE_SET_MATERIAL_COLOR(lighting.diffuse[0], GL_FRONT, GL_DIFFUSE);
            CRSTATE_SET_MATERIAL_COLOR(lighting.diffuse[1], GL_BACK, GL_DIFFUSE);
            CRSTATE_SET_MATERIAL_COLOR(lighting.specular[0], GL_FRONT, GL_SPECULAR);
            CRSTATE_SET_MATERIAL_COLOR(lighting.specular[1], GL_BACK, GL_SPECULAR);
            CRSTATE_SET_MATERIAL_COLOR(lighting.emission[0], GL_FRONT, GL_EMISSION);
            CRSTATE_SET_MATERIAL_COLOR(lighting.emission[1], GL_BACK, GL_EMISSION);
            CRSTATE_SET_MATERIAL_F(lighting.shininess[0], GL_FRONT,  GL_SHININESS);
            CRSTATE_SET_MATERIAL_F(lighting.shininess[1], GL_BACK,  GL_SHININESS);
        }

        for (i=0; i<CR_MAX_LIGHTS; ++i)
        {
            if (CHECKDIRTY(sb->lighting.light[i].dirty, negbitID))
            {
                if (CHECKDIRTY(sb->lighting.light[i].enable, negbitID))
                {
                    CRSTATE_SET_ENABLED(lighting.light[i].enable, GL_LIGHT0+i);
                }

                if (CHECKDIRTY(sb->lighting.light[i].ambient, negbitID))
                {
                    CRSTATE_SET_LIGHT_COLOR(lighting.light[i].ambient, GL_LIGHT0+i, GL_AMBIENT);
                }

                if (CHECKDIRTY(sb->lighting.light[i].diffuse, negbitID))
                {
                    CRSTATE_SET_LIGHT_COLOR(lighting.light[i].diffuse, GL_LIGHT0+i, GL_DIFFUSE);
                }

                if (CHECKDIRTY(sb->lighting.light[i].specular, negbitID))
                {
                    CRSTATE_SET_LIGHT_COLOR(lighting.light[i].specular, GL_LIGHT0+i, GL_SPECULAR);
                }

                if (CHECKDIRTY(sb->lighting.light[i].position, negbitID))
                {
                    CRSTATE_SET_LIGHT_4F(lighting.light[i].position, GL_LIGHT0+i, GL_POSITION);
                }

                if (CHECKDIRTY(sb->lighting.light[i].attenuation, negbitID))
                {
                    CRSTATE_SET_LIGHT_F(lighting.light[i].constantAttenuation, GL_LIGHT0+i, GL_CONSTANT_ATTENUATION);
                    CRSTATE_SET_LIGHT_F(lighting.light[i].linearAttenuation, GL_LIGHT0+i, GL_LINEAR_ATTENUATION);
                    CRSTATE_SET_LIGHT_F(lighting.light[i].quadraticAttenuation, GL_LIGHT0+i, GL_QUADRATIC_ATTENUATION);
                }

                if (CHECKDIRTY(sb->lighting.light[i].spot, negbitID))
                {
                    CRSTATE_SET_LIGHT_3F(lighting.light[i].spotDirection, GL_LIGHT0+i, GL_SPOT_DIRECTION);
                    CRSTATE_SET_LIGHT_F(lighting.light[i].spotExponent, GL_LIGHT0+i, GL_SPOT_EXPONENT);
                    CRSTATE_SET_LIGHT_F(lighting.light[i].spotCutoff, GL_LIGHT0+i, GL_SPOT_CUTOFF);
                }
            }
        }
    }


    if (CHECKDIRTY(sb->transform.dirty, negbitID))
    {
        if (CHECKDIRTY(sb->transform.enable, negbitID))
        {
            CRSTATE_SET_ENABLED(transform.normalize, GL_NORMALIZE); 
#ifdef CR_OPENGL_VERSION_1_2
            CRSTATE_SET_ENABLED(transform.rescaleNormals, GL_RESCALE_NORMAL);
#endif
#ifdef CR_IBM_rasterpos_clip
            CRSTATE_SET_ENABLED(transform.rasterPositionUnclipped, GL_RASTER_POSITION_UNCLIPPED_IBM);
#endif
        }

        if (CHECKDIRTY(sb->transform.clipPlane, negbitID))
        {
            int i;
            for (i=0; i<CR_MAX_CLIP_PLANES; i++)
            {
                CRSTATE_SET_CLIPPLANE_4D(transform.clipPlane[i], GL_CLIP_PLANE0+i);
            }
        }

        if (CHECKDIRTY(sb->transform.modelviewMatrix, negbitID))
        {
            CRSTATE_SET_MATRIX(transform.modelViewStack.top, GL_MODELVIEW_MATRIX);
        }

        if (CHECKDIRTY(sb->transform.projectionMatrix, negbitID))
        {
            CRSTATE_SET_MATRIX(transform.projectionStack.top, GL_PROJECTION_MATRIX);
        }

        if (CHECKDIRTY(sb->transform.textureMatrix, negbitID))
        {
            unsigned int i;
            for (i=0; i<g->limits.maxTextureUnits; i++)
            {
                pState->diff_api.ActiveTextureARB(GL_TEXTURE0_ARB+i);
                CRSTATE_SET_MATRIX(transform.textureStack[i].top, GL_TEXTURE_MATRIX);
            }
            pState->diff_api.ActiveTextureARB(g->texture.curTextureUnit + GL_TEXTURE0_ARB);
        }

        if (CHECKDIRTY(sb->transform.colorMatrix, negbitID))
        {
            CRSTATE_SET_MATRIX(transform.colorStack.top, GL_COLOR_MATRIX);
        }

        if (CHECKDIRTY(sb->transform.matrixMode, negbitID))
        {
            CRSTATE_SET_ENUM(transform.matrixMode, GL_MATRIX_MODE);
        }
    }

    if (CHECKDIRTY(sb->viewport.dirty, negbitID))
    {
        if (CHECKDIRTY(sb->viewport.enable, negbitID))
        {
            CRSTATE_SET_ENABLED(viewport.scissorTest, GL_SCISSOR_TEST);
        }

        if (CHECKDIRTY(sb->viewport.s_dims, negbitID))
        {
            GLint value[4];
            value[0] = g->viewport.scissorX;
            value[1] = g->viewport.scissorY;
            value[2] = g->viewport.scissorW;
            value[3] = g->viewport.scissorH;
            pState->diff_api.GetIntegerv(GL_SCISSOR_BOX, &value[0]);
            CRSTATE_SET_CAP(viewport.scissorX, value[0], "%i");
            CRSTATE_SET_CAP(viewport.scissorY, value[1], "%i");
            CRSTATE_SET_CAP(viewport.scissorW, value[2], "%i");
            CRSTATE_SET_CAP(viewport.scissorH, value[3], "%i");
        }

        if (CHECKDIRTY(sb->viewport.v_dims, negbitID))
        {
            GLint value[4];
            value[0] = g->viewport.viewportX;
            value[1] = g->viewport.viewportY;
            value[2] = g->viewport.viewportW;
            value[3] = g->viewport.viewportH;
            pState->diff_api.GetIntegerv(GL_VIEWPORT, &value[0]);
            CRSTATE_SET_CAP(viewport.viewportX, value[0], "%i");
            CRSTATE_SET_CAP(viewport.viewportY, value[1], "%i");
            CRSTATE_SET_CAP(viewport.viewportW, value[2], "%i");
            CRSTATE_SET_CAP(viewport.viewportH, value[3], "%i");
        }

        if (CHECKDIRTY(sb->viewport.depth, negbitID))
        {
            GLfloat value[2];
            value[0] = g->viewport.nearClip;
            value[1] = g->viewport.farClip;
            pState->diff_api.GetFloatv(GL_DEPTH_RANGE, &value[0]);
            CRSTATE_SET_CAP(viewport.nearClip, value[0], "%f");
            CRSTATE_SET_CAP(viewport.farClip, value[1], "%f");
        }
    }

    if (CHECKDIRTY(sb->eval.dirty, negbitID))
    {
        int i;
        const int gleval_sizes_dup[] = {4, 1, 3, 1, 2, 3, 4, 3, 4};

        if (CHECKDIRTY(sb->eval.enable, negbitID))
        {
            CRSTATE_SET_ENABLED(eval.autoNormal, GL_AUTO_NORMAL);
        }

        for (i=0; i<GLEVAL_TOT; i++)
        {
            if (CHECKDIRTY(sb->eval.enable1D[i], negbitID))
            {
                CRSTATE_SET_ENABLED(eval.enable1D[i], i + GL_MAP1_COLOR_4);
            }

            if (CHECKDIRTY(sb->eval.enable2D[i], negbitID))
            {
                CRSTATE_SET_ENABLED(eval.enable2D[i], i + GL_MAP2_COLOR_4);
            }

            if (CHECKDIRTY(sb->eval.eval1D[i], negbitID) && g->eval.enable1D[i])
            {
                GLfloat *coeffs=NULL;
                GLint order;
                GLfloat uval[2];
                order = g->eval.eval1D[i].order;
                uval[0] = g->eval.eval1D[i].u1;
                uval[1] = g->eval.eval1D[i].u2;
                pState->diff_api.GetMapiv(i + GL_MAP1_COLOR_4, GL_ORDER, &order);
                pState->diff_api.GetMapfv(i + GL_MAP1_COLOR_4, GL_DOMAIN, &uval[0]);
                if (order>0)
                {
                    coeffs = crAlloc(order * gleval_sizes_dup[i] * sizeof(GLfloat));
                    if (!coeffs)
                    {
                        crWarning("crStateQueryHWState: out of memory, at eval1D[%i]", i);
                        continue;
                    }
                    pState->diff_api.GetMapfv(i + GL_MAP1_COLOR_4, GL_COEFF, coeffs);
                }

                CRSTATE_SET_CAP(eval.eval1D[i].order, order, "%i");
                CRSTATE_SET_CAP(eval.eval1D[i].u1, uval[0], "%f");
                CRSTATE_SET_CAP(eval.eval1D[i].u2, uval[1], "%f");
                if (g->eval.eval1D[i].coeff)
                {
                    crFree(g->eval.eval1D[i].coeff);
                }
                g->eval.eval1D[i].coeff = coeffs;

                if (uval[0]!=uval[1])
                {
                    g->eval.eval1D[i].du = 1.0f / (uval[0] - uval[1]);
                }
            }

            if (CHECKDIRTY(sb->eval.eval2D[i], negbitID) && g->eval.enable2D[i])
            {
                GLfloat *coeffs=NULL;
                GLint order[2];
                GLfloat uval[4];
                order[0] = g->eval.eval2D[i].uorder;
                order[1] = g->eval.eval2D[i].vorder;
                uval[0] = g->eval.eval2D[i].u1;
                uval[1] = g->eval.eval2D[i].u2;
                uval[2] = g->eval.eval2D[i].v1;
                uval[3] = g->eval.eval2D[i].v2;
                pState->diff_api.GetMapiv(i + GL_MAP2_COLOR_4, GL_ORDER, &order[0]);
                pState->diff_api.GetMapfv(i + GL_MAP2_COLOR_4, GL_DOMAIN, &uval[0]);
                if (order[0]>0 && order[1]>0)
                {
                    coeffs = crAlloc(order[0] * order[1] * gleval_sizes_dup[i] * sizeof(GLfloat));
                    if (!coeffs)
                    {
                        crWarning("crStateQueryHWState: out of memory, at eval2D[%i]", i);
                        continue;
                    }
                    pState->diff_api.GetMapfv(i + GL_MAP1_COLOR_4, GL_COEFF, coeffs);
                }
                CRSTATE_SET_CAP(eval.eval2D[i].uorder, order[0], "%i");
                CRSTATE_SET_CAP(eval.eval2D[i].vorder, order[1], "%i");
                CRSTATE_SET_CAP(eval.eval2D[i].u1, uval[0], "%f");
                CRSTATE_SET_CAP(eval.eval2D[i].u2, uval[1], "%f");
                CRSTATE_SET_CAP(eval.eval2D[i].v1, uval[2], "%f");
                CRSTATE_SET_CAP(eval.eval2D[i].v2, uval[3], "%f");
                if (g->eval.eval2D[i].coeff)
                {
                    crFree(g->eval.eval2D[i].coeff);
                }
                g->eval.eval2D[i].coeff = coeffs;

                if (uval[0]!=uval[1])
                {
                    g->eval.eval2D[i].du = 1.0f / (uval[0] - uval[1]);
                }
                if (uval[2]!=uval[3])
                {
                    g->eval.eval2D[i].dv = 1.0f / (uval[2] - uval[3]);
                }
            }
        }

        if (CHECKDIRTY(sb->eval.grid1D, negbitID))
        {
            GLfloat value[2];
            CRSTATE_SET_INT(eval.un1D, GL_MAP1_GRID_SEGMENTS);
            value[0] = g->eval.u11D;
            value[1] = g->eval.u21D;
            pState->diff_api.GetFloatv(GL_MAP1_GRID_DOMAIN, &value[0]);
            CRSTATE_SET_CAP(eval.u11D, value[0], "%f");
            CRSTATE_SET_CAP(eval.u21D, value[1], "%f");
        }

        if (CHECKDIRTY(sb->eval.grid2D, negbitID))
        {
            GLint iv[2];
            GLfloat value[4];
            iv[0] = g->eval.un2D;
            iv[1] = g->eval.vn2D;
            pState->diff_api.GetIntegerv(GL_MAP1_GRID_SEGMENTS, &iv[0]);
            CRSTATE_SET_CAP(eval.un2D, iv[0], "%i");
            CRSTATE_SET_CAP(eval.vn2D, iv[1], "%i");
            value[0] = g->eval.u12D;
            value[1] = g->eval.u22D;
            value[2] = g->eval.v12D;
            value[3] = g->eval.v22D;
            pState->diff_api.GetFloatv(GL_MAP2_GRID_DOMAIN, &value[0]);
            CRSTATE_SET_CAP(eval.u12D, value[0], "%f");
            CRSTATE_SET_CAP(eval.u22D, value[1], "%f");
            CRSTATE_SET_CAP(eval.v12D, value[2], "%f");
            CRSTATE_SET_CAP(eval.v22D, value[3], "%f");
        }
    }

    if (CHECKDIRTY(sb->fog.dirty, negbitID))
    {
        if (CHECKDIRTY(sb->fog.enable, negbitID))
        {
            CRSTATE_SET_ENABLED(fog.enable, GL_FOG);
        }

        if (CHECKDIRTY(sb->fog.color, negbitID))
        {
            CRSTATE_SET_COLORF(fog.color, GL_FOG_COLOR);
        }

        if (CHECKDIRTY(sb->fog.index, negbitID))
        {
            CRSTATE_SET_INT(fog.index, GL_FOG_INDEX);
        }

        if (CHECKDIRTY(sb->fog.density, negbitID))
        {
            CRSTATE_SET_FLOAT(fog.density, GL_FOG_DENSITY);
        }

        if (CHECKDIRTY(sb->fog.start, negbitID))
        {
            CRSTATE_SET_FLOAT(fog.start, GL_FOG_START);
        }

        if (CHECKDIRTY(sb->fog.end, negbitID))
        {
            CRSTATE_SET_FLOAT(fog.end, GL_FOG_END);
        }

        if (CHECKDIRTY(sb->fog.mode, negbitID))
        {
            CRSTATE_SET_INT(fog.mode, GL_FOG_MODE);
        }

#ifdef CR_NV_fog_distance
        if (CHECKDIRTY(sb->fog.fogDistanceMode, negbitID))
        {
            CRSTATE_SET_ENUM(fog.fogDistanceMode, GL_FOG_DISTANCE_MODE_NV);
        }
#endif
#ifdef CR_EXT_fog_coord
        if (CHECKDIRTY(sb->fog.fogCoordinateSource, negbitID))
        {
            CRSTATE_SET_ENUM(fog.fogCoordinateSource, GL_FOG_COORDINATE_SOURCE_EXT);
        }
#endif
    }

    if (CHECKDIRTY(sb->hint.dirty, negbitID))
    {
        if (CHECKDIRTY(sb->hint.perspectiveCorrection, negbitID))
        {
            CRSTATE_SET_ENUM(hint.perspectiveCorrection, GL_PERSPECTIVE_CORRECTION_HINT);
        }

        if (CHECKDIRTY(sb->hint.pointSmooth, negbitID))
        {
            CRSTATE_SET_ENUM(hint.pointSmooth, GL_POINT_SMOOTH_HINT);
        }

        if (CHECKDIRTY(sb->hint.lineSmooth, negbitID))
        {
            CRSTATE_SET_ENUM(hint.lineSmooth, GL_LINE_SMOOTH_HINT);
        }

        if (CHECKDIRTY(sb->hint.polygonSmooth, negbitID))
        {
            CRSTATE_SET_ENUM(hint.polygonSmooth, GL_POINT_SMOOTH_HINT);
        }

        if (CHECKDIRTY(sb->hint.fog, negbitID))
        {
            CRSTATE_SET_ENUM(hint.fog, GL_FOG_HINT);
        }

#ifdef CR_EXT_clip_volume_hint
        if (CHECKDIRTY(sb->hint.clipVolumeClipping, negbitID))
        {
            CRSTATE_SET_ENUM(hint.clipVolumeClipping, GL_CLIP_VOLUME_CLIPPING_HINT_EXT);
        }
#endif
#ifdef CR_ARB_texture_compression
        if (CHECKDIRTY(sb->hint.textureCompression, negbitID))
        {
            CRSTATE_SET_ENUM(hint.textureCompression, GL_TEXTURE_COMPRESSION_HINT_ARB);
        }
#endif
#ifdef CR_SGIS_generate_mipmap
        if (CHECKDIRTY(sb->hint.generateMipmap, negbitID))
        {
            CRSTATE_SET_ENUM(hint.generateMipmap, GL_GENERATE_MIPMAP_HINT_SGIS);
        }
#endif
    }

    if (CHECKDIRTY(sb->line.dirty, negbitID))
    {
        if (CHECKDIRTY(sb->line.enable, negbitID))
        {
            CRSTATE_SET_ENABLED(line.lineSmooth, GL_LINE_SMOOTH);
            CRSTATE_SET_ENABLED(line.lineStipple, GL_LINE_STIPPLE);
        }

        if (CHECKDIRTY(sb->line.width, negbitID))
        {
            CRSTATE_SET_FLOAT(line.width, GL_LINE_WIDTH);
        }

        if (CHECKDIRTY(sb->line.stipple, negbitID))
        {
            CRSTATE_SET_INT(line.repeat, GL_LINE_STIPPLE_REPEAT);
            CRSTATE_SET_INT(line.pattern, GL_LINE_STIPPLE_PATTERN);
        }
    }

    if (CHECKDIRTY(sb->multisample.dirty, negbitID))
    {
        if (CHECKDIRTY(sb->multisample.enable, negbitID))
        {
            CRSTATE_SET_ENABLED(multisample.enabled, GL_MULTISAMPLE_ARB);
            CRSTATE_SET_ENABLED(multisample.sampleAlphaToCoverage, GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
            CRSTATE_SET_ENABLED(multisample.sampleAlphaToOne, GL_SAMPLE_ALPHA_TO_ONE_ARB);
            CRSTATE_SET_ENABLED(multisample.sampleCoverage, GL_SAMPLE_COVERAGE_ARB);
        }

        if (CHECKDIRTY(sb->multisample.sampleCoverageValue, negbitID))
        {
            CRSTATE_SET_FLOAT(multisample.sampleCoverageValue, GL_SAMPLE_COVERAGE_VALUE_ARB)
            CRSTATE_SET_BOOL(multisample.sampleCoverageInvert, GL_SAMPLE_COVERAGE_INVERT_ARB)
        }
    }

    if (CHECKDIRTY(sb->point.dirty, negbitID))
    {
        if (CHECKDIRTY(sb->point.enableSmooth, negbitID))
        {
            CRSTATE_SET_ENABLED(point.pointSmooth, GL_POINT_SMOOTH);
        }

        if (CHECKDIRTY(sb->point.size, negbitID))
        {
            CRSTATE_SET_FLOAT(point.pointSize, GL_POINT_SIZE);
        }

#ifdef CR_ARB_point_parameters
        if (CHECKDIRTY(sb->point.minSize, negbitID))
        {
            CRSTATE_SET_FLOAT(point.minSize, GL_POINT_SIZE_MIN_ARB);
        }

        if (CHECKDIRTY(sb->point.maxSize, negbitID))
        {
            CRSTATE_SET_FLOAT(point.maxSize, GL_POINT_SIZE_MAX_ARB);
        }

        if (CHECKDIRTY(sb->point.fadeThresholdSize, negbitID))
        {
            CRSTATE_SET_FLOAT(point.fadeThresholdSize, GL_POINT_FADE_THRESHOLD_SIZE_ARB);
        }

        if (CHECKDIRTY(sb->point.distanceAttenuation, negbitID))
        {
            GLfloat value[3];
            value[0] = g->point.distanceAttenuation[0];
            value[1] = g->point.distanceAttenuation[1];
            value[2] = g->point.distanceAttenuation[2];
            pState->diff_api.GetFloatv(GL_POINT_DISTANCE_ATTENUATION, &value[0]);
            CRSTATE_SET_CAP(point.distanceAttenuation[0], value[0], "%f");
            CRSTATE_SET_CAP(point.distanceAttenuation[1], value[1], "%f");
            CRSTATE_SET_CAP(point.distanceAttenuation[2], value[2], "%f");
        }
#endif
#ifdef CR_ARB_point_sprite
        if (CHECKDIRTY(sb->point.enableSprite, negbitID))
        {
            CRSTATE_SET_ENABLED(point.pointSprite, GL_POINT_SPRITE_ARB);
        }

        {
            unsigned int i, activeUnit = g->texture.curTextureUnit;
            for (i=0; i<g->limits.maxTextureUnits; ++i)
            {
                if (CHECKDIRTY(sb->point.coordReplacement[i], negbitID))
                {
                    GLint val=g->point.coordReplacement[i];
                    if (activeUnit!=i)
                    {
                        pState->diff_api.ActiveTextureARB(i + GL_TEXTURE0_ARB);
                        activeUnit=i;
                    }
                    pState->diff_api.GetTexEnviv(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, &val);
                    CRSTATE_SET_CAP(point.coordReplacement[i], val, "%i");
                }
            }

            if (activeUnit!=g->texture.curTextureUnit)
            {
                pState->diff_api.ActiveTextureARB(g->texture.curTextureUnit + GL_TEXTURE0_ARB);
            }
        }
#endif
    }

    if (CHECKDIRTY(sb->polygon.dirty, negbitID))
    {
        if (CHECKDIRTY(sb->polygon.enable, negbitID))
        {
            CRSTATE_SET_ENABLED(polygon.polygonSmooth, GL_POLYGON_SMOOTH);
            CRSTATE_SET_ENABLED(polygon.polygonOffsetFill, GL_POLYGON_OFFSET_FILL);
            CRSTATE_SET_ENABLED(polygon.polygonOffsetLine, GL_POLYGON_OFFSET_LINE);
            CRSTATE_SET_ENABLED(polygon.polygonOffsetPoint, GL_POLYGON_OFFSET_POINT);
            CRSTATE_SET_ENABLED(polygon.polygonStipple, GL_POLYGON_STIPPLE);
            CRSTATE_SET_ENABLED(polygon.cullFace, GL_CULL_FACE);
        }

        if (CHECKDIRTY(sb->polygon.offset, negbitID))
        {
            CRSTATE_SET_FLOAT(polygon.offsetFactor, GL_POLYGON_OFFSET_FACTOR);
            CRSTATE_SET_FLOAT(polygon.offsetUnits, GL_POLYGON_OFFSET_UNITS);
        }

        if (CHECKDIRTY(sb->polygon.mode, negbitID))
        {
            GLint val[2];
            CRSTATE_SET_ENUM(polygon.frontFace, GL_FRONT_FACE);
            CRSTATE_SET_ENUM(polygon.cullFaceMode, GL_CULL_FACE_MODE);
            val[0] = g->polygon.frontMode;
            val[1] = g->polygon.backMode;
            pState->diff_api.GetIntegerv(GL_POLYGON_MODE, &val[0]);
            CRSTATE_SET_CAP(polygon.frontMode, val[0], "%#x");
            CRSTATE_SET_CAP(polygon.backMode, val[1], "%#x");


        }

        if (CHECKDIRTY(sb->polygon.stipple, negbitID))
        {
            GLint stipple[32];
            crMemcpy(&stipple[0], &g->polygon.stipple[0], sizeof(stipple));
            pState->diff_api.GetPolygonStipple((GLubyte*) &stipple[0]);
            if (crMemcmp(&stipple[0], &g->polygon.stipple[0], sizeof(stipple)))
            {
#ifdef CRSTATE_DEBUG_QUERY_HW_STATE
                {
                    crDebug("crStateQueryHWState fixed polygon.stipple");
                }
#endif
                crMemcpy(&g->polygon.stipple[0], &stipple[0], sizeof(stipple));
            }
        }
    }

    CR_STATE_CLEAN_HW_ERR_WARN(pState, "error on hw sync");
}

void STATE_APIENTRY crStateNewList (PCRStateTracker pState, GLuint list, GLenum mode) 
{
    CRContext *g = GetCurrentContext(pState);
    CRListsState *l = &(g->lists);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glNewList called in Begin/End");
        return;
    }

    if (list == 0)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_VALUE, "glNewList(list=0)");
        return;
    }

    if (l->currentIndex)
    {
        /* already building a list */
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glNewList called inside display list");
        return;
    }

    if (mode != GL_COMPILE && mode != GL_COMPILE_AND_EXECUTE)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_ENUM, "glNewList invalid mode");
        return;
    }

    FLUSH();

    /* Must log that this key is used */
    if (!crHashtableIsKeyUsed(g->shared->dlistTable, list)) {
        crHashtableAdd(g->shared->dlistTable, list, NULL);
    }

    /* Need this???
    crStateCurrentRecover();
    */

    l->currentIndex = list;
    l->mode = mode;
}

void STATE_APIENTRY crStateEndList (PCRStateTracker pState)
{
    CRContext *g = GetCurrentContext(pState);
    CRListsState *l = &(g->lists);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glEndList called in Begin/End");
        return;
    }

    if (!l->currentIndex)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glEndList called outside display list");
        return;
    }

    l->currentIndex = 0;
    l->mode = 0;
}

GLuint STATE_APIENTRY crStateGenLists(PCRStateTracker pState, GLsizei range)
{
    CRContext *g = GetCurrentContext(pState);
    GLuint start;

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glGenLists called in Begin/End");
        return 0;
    }

    if (range < 0)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_VALUE, "Negative range passed to glGenLists: %d", range);
        return 0;
    }

    start = crHashtableAllocKeys(g->shared->dlistTable, range);

    CRASSERT(start > 0);
    return start;
}
    
void STATE_APIENTRY crStateDeleteLists (PCRStateTracker pState, GLuint list, GLsizei range)
{
    CRContext *g = GetCurrentContext(pState);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION, "glDeleteLists called in Begin/End");
        return;
    }

    if (range < 0)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_VALUE, "Negative range passed to glDeleteLists: %d", range);
        return;
    }

    crHashtableDeleteBlock(g->shared->dlistTable, list, range, crFree); /* call crFree to delete list data */
}

GLboolean STATE_APIENTRY crStateIsList(PCRStateTracker pState, GLuint list)
{
    CRContext *g = GetCurrentContext(pState);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION,
            "GenLists called in Begin/End");
        return GL_FALSE;
    }

    if (list == 0)
        return GL_FALSE;

    return crHashtableIsKeyUsed(g->shared->dlistTable, list);
}
    
void STATE_APIENTRY crStateListBase (PCRStateTracker pState, GLuint base)
{
    CRContext *g = GetCurrentContext(pState);
    CRListsState *l = &(g->lists);
    CRStateBits *sb = GetCurrentBits(pState);
    CRListsBits *lb = &(sb->lists);

    if (g->current.inBeginEnd)
    {
        crStateError(pState, __LINE__, __FILE__, GL_INVALID_OPERATION,
            "ListBase called in Begin/End");
        return;
    }

    l->base = base;

    DIRTY(lb->base, g->neg_bitid);
    DIRTY(lb->dirty, g->neg_bitid);
}


void
crStateListsDiff( CRListsBits *b, CRbitvalue *bitID, 
                                    CRContext *fromCtx, CRContext *toCtx )
{
    PCRStateTracker pState = fromCtx->pStateTracker;
    CRListsState *from = &(fromCtx->lists);
    CRListsState *to = &(toCtx->lists);
    unsigned int j;
    CRbitvalue nbitID[CR_MAX_BITARRAY];

    CRASSERT(fromCtx->pStateTracker == toCtx->pStateTracker);

    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];

    if (CHECKDIRTY(b->base, bitID))
    {
        if (from->base != to->base) {
            pState->diff_api.ListBase(to->base);
            from->base = to->base;
        }
        CLEARDIRTY(b->base, nbitID);
    }

    CLEARDIRTY(b->dirty, nbitID);
}


void
crStateListsSwitch( CRListsBits *b, CRbitvalue *bitID, 
                                        CRContext *fromCtx, CRContext *toCtx )
{
    PCRStateTracker pState = fromCtx->pStateTracker;
    CRListsState *from = &(fromCtx->lists);
    CRListsState *to = &(toCtx->lists);
    unsigned int j;
    CRbitvalue nbitID[CR_MAX_BITARRAY];

    CRASSERT(fromCtx->pStateTracker == toCtx->pStateTracker);

    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];

    if (CHECKDIRTY(b->base, bitID))
    {
        if (from->base != to->base) {
            pState->diff_api.ListBase(to->base);
            FILLDIRTY(b->base);
            FILLDIRTY(b->dirty);
        }
        CLEARDIRTY(b->base, nbitID);
    }

    CLEARDIRTY(b->dirty, nbitID);
}
