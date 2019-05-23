/* $Id: state_snapshot.c $ */
/** @file
 * VBox Context state saving/loading used by VM snapshot
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "state.h"
#include "state_internals.h"
#include "state/cr_statetypes.h"
#include "state/cr_texture.h"
#include "cr_mem.h"
#include "cr_string.h"
#include "cr_pixeldata.h"
#include <stdio.h>

#include <iprt/assert.h>
#include <iprt/types.h>
#include <iprt/err.h>
#include <VBox/err.h>
#include <VBox/log.h>

/** @todo
 * We have two ways of saving/loading states.
 *
 * First which is being used atm, just pure saving/loading of structures.
 * The drawback is we have to deal with all the pointers around those structures,
 * we'd have to update this code if we'd change state tracking.
 * On the bright side it's fast, though it's not really needed as it's not that often operation.
 * It could also worth to split those functions into appropriate parts,
 * similar to the way context creation is being done.
 *
 * Second way would be to implement full dispatch api table and substitute diff_api during saving/loading.
 * Then if we implement that api in a similar way to packer/unpacker with a change to store/load
 * via provided pSSM handle instead of pack buffer,
 * saving state could be done by simple diffing against empty "dummy" context.
 * Restoring state in such case would look like unpacking commands from pSSM instead of network buffer.
 * This would be slower (who cares) but most likely will not require any code changes to support in future.
 * We will reduce amount of saved data as we'd save only changed state parts, but I doubt it'd be that much.
 * It could be done for the first way as well, but requires tons of bit checks.
 */

static int32_t crStateAllocAndSSMR3GetMem(PSSMHANDLE pSSM, void **pBuffer, size_t cbBuffer)
{
    CRASSERT(pSSM && pBuffer && cbBuffer>0);

    *pBuffer = crAlloc((unsigned int /* this case is just so stupid */)cbBuffer);
    if (!*pBuffer)
        return VERR_NO_MEMORY;

    return SSMR3GetMem(pSSM, *pBuffer, cbBuffer);
}

#define SHCROGL_GET_STRUCT_PART(_pPtr, _type, _from, _to) do { \
            rc = SSMR3GetMem(pSSM, &(_pPtr)->_from, RT_OFFSETOF(_type, _to) - RT_OFFSETOF(_type, _from)); \
            AssertRCReturn(rc, rc); \
        } while (0)

#define SHCROGL_GET_STRUCT_TAIL(_pPtr, _type, _from) do { \
            rc = SSMR3GetMem(pSSM, &(_pPtr)->_from, sizeof (_type) - RT_OFFSETOF(_type, _from)); \
            AssertRCReturn(rc, rc); \
        } while (0)

#define SHCROGL_GET_STRUCT_HEAD(_pPtr, _type, _to) do { \
            rc = SSMR3GetMem(pSSM, (_pPtr), RT_OFFSETOF(_type, _to)); \
            AssertRCReturn(rc, rc); \
        } while (0)

#define SHCROGL_CUT_FIELD_ALIGNMENT_SIZE(_type, _prevField, _field) (RT_OFFSETOF(_type, _field) - RT_OFFSETOF(_type, _prevField) - RT_SIZEOFMEMB(_type, _prevField))
#define SHCROGL_CUT_FIELD_ALIGNMENT(_type, _prevField, _field) do { \
            const int32_t cbAlignment = SHCROGL_CUT_FIELD_ALIGNMENT_SIZE(_type, _prevField, _field) ; \
            /*AssertCompile(SHCROGL_CUT_FIELD_ALIGNMENT_SIZE(_type, _prevField, _field) >= 0 && SHCROGL_CUT_FIELD_ALIGNMENT_SIZE(_type, _prevField, _field) < sizeof (void*));*/ \
            if (cbAlignment) { \
                rc = SSMR3Skip(pSSM, cbAlignment); \
            } \
        } while (0)

#define SHCROGL_ROUNDBOUND(_v, _b) (((_v) + ((_b) - 1)) & ~((_b) - 1))
#define SHCROGL_ALIGNTAILSIZE(_v, _b) (SHCROGL_ROUNDBOUND((_v),(_b)) - (_v))
#define SHCROGL_CUT_FOR_OLD_TYPE_TO_ENSURE_ALIGNMENT_SIZE(_type, _field, _oldFieldType, _nextFieldAllignment) (SHCROGL_ALIGNTAILSIZE(((RT_OFFSETOF(_type, _field) + sizeof (_oldFieldType))), (_nextFieldAllignment)))
#define SHCROGL_CUT_FOR_OLD_TYPE_TO_ENSURE_ALIGNMENT(_type, _field, _oldFieldType, _nextFieldAllignment)  do { \
        const int32_t cbAlignment = SHCROGL_CUT_FOR_OLD_TYPE_TO_ENSURE_ALIGNMENT_SIZE(_type, _field, _oldFieldType, _nextFieldAllignment); \
        /*AssertCompile(SHCROGL_CUT_TAIL_ALIGNMENT_SIZE(_type, _lastField) >= 0 && SHCROGL_CUT_TAIL_ALIGNMENT_SIZE(_type, _lastField) < sizeof (void*));*/ \
        if (cbAlignment) { \
            rc = SSMR3Skip(pSSM, cbAlignment); \
        } \
    } while (0)


#define SHCROGL_CUT_TAIL_ALIGNMENT_SIZE(_type, _lastField) (sizeof (_type) - RT_OFFSETOF(_type, _lastField) - RT_SIZEOFMEMB(_type, _lastField))
#define SHCROGL_CUT_TAIL_ALIGNMENT(_type, _lastField) do { \
            const int32_t cbAlignment = SHCROGL_CUT_TAIL_ALIGNMENT_SIZE(_type, _lastField); \
            /*AssertCompile(SHCROGL_CUT_TAIL_ALIGNMENT_SIZE(_type, _lastField) >= 0 && SHCROGL_CUT_TAIL_ALIGNMENT_SIZE(_type, _lastField) < sizeof (void*));*/ \
            if (cbAlignment) { \
                rc = SSMR3Skip(pSSM, cbAlignment); \
            } \
        } while (0)

static int32_t crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(CRTextureObj *pTexture, PSSMHANDLE pSSM)
{
    int32_t rc;
    uint32_t cbObj = RT_OFFSETOF(CRTextureObj, ctxUsage);
    cbObj = ((cbObj + sizeof (void*) - 1) & ~(sizeof (void*) - 1));
    rc = SSMR3GetMem(pSSM, pTexture, cbObj);
    AssertRCReturn(rc, rc);
    /* just make all bits are used so that we fall back to the pre-ctxUsage behavior,
     * i.e. all shared resources will be destructed on last shared context termination */
    FILLDIRTY(pTexture->ctxUsage);
    return rc;
}

static int32_t crStateLoadTextureUnit_v_BEFORE_CTXUSAGE_BITS(CRTextureUnit *t, PSSMHANDLE pSSM)
{
    int32_t rc;
    SHCROGL_GET_STRUCT_HEAD(t, CRTextureUnit, Saved1D);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->Saved1D, pSSM);
    AssertRCReturn(rc, rc);
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureUnit, Saved1D, Saved2D);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->Saved2D, pSSM);
    AssertRCReturn(rc, rc);
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureUnit, Saved2D, Saved3D);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->Saved3D, pSSM);
    AssertRCReturn(rc, rc);
#ifdef CR_ARB_texture_cube_map
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureUnit, Saved3D, SavedCubeMap);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->SavedCubeMap, pSSM);
    AssertRCReturn(rc, rc);
# define SHCROGL_INTERNAL_LAST_FIELD SavedCubeMap
#else
# define SHCROGL_INTERNAL_LAST_FIELD Saved3D
#endif
#ifdef CR_NV_texture_rectangle
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureUnit, SHCROGL_INTERNAL_LAST_FIELD, SavedRect);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->SavedRect, pSSM);
    AssertRCReturn(rc, rc);
# undef SHCROGL_INTERNAL_LAST_FIELD
# define SHCROGL_INTERNAL_LAST_FIELD SavedRect
#endif
    SHCROGL_CUT_TAIL_ALIGNMENT(CRTextureUnit, SHCROGL_INTERNAL_LAST_FIELD);
#undef SHCROGL_INTERNAL_LAST_FIELD
    return rc;
}

static int crStateLoadStencilPoint_v_37(CRPointState *pPoint, PSSMHANDLE pSSM)
{
    int rc = VINF_SUCCESS;
    SHCROGL_GET_STRUCT_HEAD(pPoint, CRPointState, spriteCoordOrigin);
    pPoint->spriteCoordOrigin = (GLfloat)GL_UPPER_LEFT;
    return rc;
}

static int32_t crStateLoadStencilState_v_33(CRStencilState *s, PSSMHANDLE pSSM)
{
    CRStencilState_v_33 stencilV33;
    int32_t rc = SSMR3GetMem(pSSM, &stencilV33, sizeof (stencilV33));
    AssertRCReturn(rc, rc);
    s->stencilTest = stencilV33.stencilTest;
    s->stencilTwoSideEXT = GL_FALSE;
    s->activeStencilFace = GL_FRONT;
    s->clearValue = stencilV33.clearValue;
    s->writeMask = stencilV33.writeMask;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func = stencilV33.func;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask = stencilV33.mask;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref = stencilV33.ref;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail = stencilV33.fail;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail = stencilV33.passDepthFail;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass = stencilV33.passDepthPass;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK] = s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT];
    crStateStencilBufferInit(&s->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK]);
    return VINF_SUCCESS;
}

static int32_t crStateLoadTextureState_v_BEFORE_CTXUSAGE_BITS(CRTextureState *t, PSSMHANDLE pSSM)
{
    GLint i;
    int32_t rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->base1D, pSSM);
    AssertRCReturn(rc, rc);
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureState, base1D, base2D);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->base2D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->base3D, pSSM);
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureState, base2D, base3D);
    AssertRCReturn(rc, rc);
#ifdef CR_ARB_texture_cube_map
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureState, base3D, baseCubeMap);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->baseCubeMap, pSSM);
    AssertRCReturn(rc, rc);
# define SHCROGL_INTERNAL_LAST_FIELD baseCubeMap
#else
# define SHCROGL_INTERNAL_LAST_FIELD base3D
#endif
#ifdef CR_NV_texture_rectangle
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureState, SHCROGL_INTERNAL_LAST_FIELD, baseRect);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->baseRect, pSSM);
    AssertRCReturn(rc, rc);
# undef SHCROGL_INTERNAL_LAST_FIELD
# define SHCROGL_INTERNAL_LAST_FIELD baseRect
#endif
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureState, SHCROGL_INTERNAL_LAST_FIELD, proxy1D);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->proxy1D, pSSM);
    AssertRCReturn(rc, rc);
#undef SHCROGL_INTERNAL_LAST_FIELD
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureState, proxy1D, proxy2D);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->proxy2D, pSSM);
    AssertRCReturn(rc, rc);
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureState, proxy2D, proxy3D);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->proxy3D, pSSM);
    AssertRCReturn(rc, rc);
#ifdef CR_ARB_texture_cube_map
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureState, proxy3D, proxyCubeMap);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->proxyCubeMap, pSSM);
    AssertRCReturn(rc, rc);
# define SHCROGL_INTERNAL_LAST_FIELD proxyCubeMap
#else
# define SHCROGL_INTERNAL_LAST_FIELD proxy3D
#endif
#ifdef CR_NV_texture_rectangle
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureState, SHCROGL_INTERNAL_LAST_FIELD, proxyRect);
    rc = crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(&t->proxyRect, pSSM);
    AssertRCReturn(rc, rc);
# undef SHCROGL_INTERNAL_LAST_FIELD
# define SHCROGL_INTERNAL_LAST_FIELD proxyRect
#endif
    SHCROGL_CUT_FIELD_ALIGNMENT(CRTextureState, SHCROGL_INTERNAL_LAST_FIELD, curTextureUnit);
# undef SHCROGL_INTERNAL_LAST_FIELD
    SHCROGL_GET_STRUCT_PART(t, CRTextureState, curTextureUnit, unit);

    for (i = 0; i < CR_MAX_TEXTURE_UNITS; ++i)
    {
        rc = crStateLoadTextureUnit_v_BEFORE_CTXUSAGE_BITS(&t->unit[i], pSSM);
        AssertRCReturn(rc, rc);
    }

    SHCROGL_CUT_TAIL_ALIGNMENT(CRTextureState, unit);

    return VINF_SUCCESS;
}

static int32_t crStateStencilBufferStack_v_33(CRStencilBufferStack *s, PSSMHANDLE pSSM)
{
    CRStencilBufferStack_v_33 stackV33;
    int32_t rc = SSMR3GetMem(pSSM, &stackV33, sizeof(stackV33));
    AssertLogRelReturn(rc, rc);

    s->stencilTest = stackV33.stencilTest;
    s->stencilTwoSideEXT = GL_FALSE;
    s->activeStencilFace = GL_FRONT;
    s->clearValue = stackV33.clearValue;
    s->writeMask = stackV33.writeMask;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].func = stackV33.func;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].mask = stackV33.mask;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].ref = stackV33.ref;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].fail = stackV33.fail;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthFail = stackV33.passDepthFail;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT].passDepthPass = stackV33.passDepthPass;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_BACK] = s->buffers[CRSTATE_STENCIL_BUFFER_ID_FRONT];

    s->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].func = GL_ALWAYS;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].mask = 0xFFFFFFFF;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].ref = 0;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].fail = GL_KEEP;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].passDepthFail = GL_KEEP;
    s->buffers[CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK].passDepthPass = GL_KEEP;

    return VINF_SUCCESS;
}

static int32_t crStateLoadAttribState_v_33(CRAttribState *t, PSSMHANDLE pSSM)
{
    int32_t i, rc;
    SHCROGL_GET_STRUCT_HEAD(t, CRAttribState, stencilBufferStack);
    for (i = 0; i < CR_MAX_ATTRIB_STACK_DEPTH; ++i)
    {
        rc = crStateStencilBufferStack_v_33(&t->stencilBufferStack[i], pSSM);
        AssertRCReturn(rc, rc);
    }
    SHCROGL_GET_STRUCT_TAIL(t, CRAttribState, textureStackDepth);
    return rc;
}

static int32_t crStateLoadTextureStack_v_BEFORE_CTXUSAGE_BITS(CRTextureStack *t, PSSMHANDLE pSSM)
{
    int32_t i, rc;
    SHCROGL_GET_STRUCT_HEAD(t, CRTextureStack, unit);
    for (i = 0; i < CR_MAX_TEXTURE_UNITS; ++i)
    {
        rc = crStateLoadTextureUnit_v_BEFORE_CTXUSAGE_BITS(&t->unit[i], pSSM);
        AssertRCReturn(rc, rc);
    }
    SHCROGL_CUT_TAIL_ALIGNMENT(CRTextureStack, unit);
    return rc;
}

static int32_t crStateLoadAttribState_v_BEFORE_CTXUSAGE_BITS(CRAttribState *t, PSSMHANDLE pSSM)
{
    int32_t i, rc;

    SHCROGL_GET_STRUCT_HEAD(t, CRAttribState, stencilBufferStack);
    for (i = 0; i < CR_MAX_ATTRIB_STACK_DEPTH; ++i)
    {
        rc = crStateStencilBufferStack_v_33(&t->stencilBufferStack[i], pSSM);
        AssertRCReturn(rc, rc);
    }
    SHCROGL_GET_STRUCT_PART(t, CRAttribState, textureStackDepth, textureStack);
    for (i = 0; i < CR_MAX_ATTRIB_STACK_DEPTH; ++i)
    {
        rc = crStateLoadTextureStack_v_BEFORE_CTXUSAGE_BITS(&t->textureStack[i], pSSM);
        AssertRCReturn(rc, rc);
    }
    SHCROGL_GET_STRUCT_TAIL(t, CRAttribState, transformStackDepth);
    return rc;
}


static int32_t crStateLoadTextureObj(CRTextureObj *pTexture, PSSMHANDLE pSSM, uint32_t u32Version)
{
    int32_t rc;
    if (u32Version == SHCROGL_SSM_VERSION_BEFORE_CTXUSAGE_BITS)
        return crStateLoadTextureObj_v_BEFORE_CTXUSAGE_BITS(pTexture, pSSM);
    rc = SSMR3GetMem(pSSM, pTexture, sizeof (*pTexture));
    AssertRCReturn(rc, rc);
    return rc;
}

static int32_t crStateLoadBufferObject(CRBufferObject *pBufferObj, PSSMHANDLE pSSM, uint32_t u32Version)
{
    int32_t rc;
    if (u32Version == SHCROGL_SSM_VERSION_BEFORE_CTXUSAGE_BITS)
    {
        uint32_t cbObj = RT_OFFSETOF(CRBufferObject, ctxUsage);
        cbObj = ((cbObj + sizeof (void*) - 1) & ~(sizeof (void*) - 1));
        rc = SSMR3GetMem(pSSM, pBufferObj, cbObj);
        AssertRCReturn(rc, rc);
        /* just make all bits are used so that we fall back to the pre-ctxUsage behavior,
         * i.e. all shared resources will be destructed on last shared context termination */
        FILLDIRTY(pBufferObj->ctxUsage);
    }
    else
    {
        rc = SSMR3GetMem(pSSM, pBufferObj, sizeof(*pBufferObj));
        AssertRCReturn(rc, rc);
    }
    return rc;
}

static int32_t crStateLoadFramebufferObject(CRFramebufferObject *pFBO, PSSMHANDLE pSSM, uint32_t u32Version)
{
    int32_t rc;
    if (u32Version == SHCROGL_SSM_VERSION_BEFORE_CTXUSAGE_BITS)
    {
        uint32_t cbObj = RT_OFFSETOF(CRFramebufferObject, ctxUsage);
        cbObj = ((cbObj + sizeof (void*) - 1) & ~(sizeof (void*) - 1));
        rc = SSMR3GetMem(pSSM, pFBO, cbObj);
        AssertRCReturn(rc, rc);
        /* just make all bits are used so that we fall back to the pre-ctxUsage behavior,
         * i.e. all shared resources will be destructed on last shared context termination */
        FILLDIRTY(pFBO->ctxUsage);
    }
    else
    {
        rc = SSMR3GetMem(pSSM, pFBO, sizeof(*pFBO));
        AssertRCReturn(rc, rc);
    }
    return rc;
}

static int32_t crStateLoadRenderbufferObject(CRRenderbufferObject *pRBO, PSSMHANDLE pSSM, uint32_t u32Version)
{
    int32_t rc;
    if (u32Version == SHCROGL_SSM_VERSION_BEFORE_CTXUSAGE_BITS)
    {
        uint32_t cbObj = RT_OFFSETOF(CRRenderbufferObject, ctxUsage);
        cbObj = ((cbObj + sizeof (void*) - 1) & ~(sizeof (void*) - 1));
        rc = SSMR3GetMem(pSSM, pRBO, cbObj);
        AssertRCReturn(rc, rc);
        /* just make all bits are used so that we fall back to the pre-ctxUsage behavior,
         * i.e. all shared resources will be destructed on last shared context termination */
        FILLDIRTY(pRBO->ctxUsage);
    }
    else
    {
        rc = SSMR3GetMem(pSSM, pRBO, sizeof(*pRBO));
        AssertRCReturn(rc, rc);
    }
    return rc;
}

static int32_t crStateSaveTextureObjData(CRTextureObj *pTexture, PSSMHANDLE pSSM)
{
    int32_t rc, face, i;
    GLint bound = 0;

    CRASSERT(pTexture && pSSM);

    crDebug("crStateSaveTextureObjData %u. START", pTexture->id);

    for (face = 0; face < 6; face++) {
        CRASSERT(pTexture->level[face]);

        for (i = 0; i < CR_MAX_MIPMAP_LEVELS; i++) {
            CRTextureLevel *ptl = &(pTexture->level[face][i]);
            rc = SSMR3PutMem(pSSM, ptl, sizeof(*ptl));
            AssertRCReturn(rc, rc);
            if (ptl->img)
            {
                CRASSERT(ptl->bytes);
                rc = SSMR3PutMem(pSSM, ptl->img, ptl->bytes);
                AssertRCReturn(rc, rc);
            }
#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
            /* Note, this is not a bug.
             * Even with CR_STATE_NO_TEXTURE_IMAGE_STORE defined, it's possible that ptl->img!=NULL.
             * For ex. we're saving snapshot right after it was loaded
             * and some context hasn't been used by the guest application yet
             * (pContext->shared->bTexResyncNeeded==GL_TRUE).
             */
            else if (ptl->bytes)
            {
                char *pImg;
                GLenum target;

                if (!bound)
                {
                    GLenum getEnum;
                    diff_api.BindTexture(pTexture->target, pTexture->hwid);
                    bound = 1;

                    /* osx nvidia drivers seem to have a bug that 1x1 TEXTURE_2D texture becmes inaccessible for some reason
                     * saw that for 1x1 dummy textures generated by wine
                     * to avoid crashes we skip texture data save if that is the case */
                    switch (pTexture->target)
                    {
                        case GL_TEXTURE_1D:
                            getEnum = GL_TEXTURE_BINDING_1D;
                            break;
                        case GL_TEXTURE_2D:
                            getEnum = GL_TEXTURE_BINDING_2D;
                            break;
                        case GL_TEXTURE_3D:
                            getEnum = GL_TEXTURE_BINDING_3D;
                            break;
                        case GL_TEXTURE_RECTANGLE_ARB:
                            getEnum = GL_TEXTURE_BINDING_RECTANGLE_ARB;
                            break;
                        case GL_TEXTURE_CUBE_MAP_ARB:
                            getEnum = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
                            break;
                        default:
                            crWarning("unknown texture target: 0x%x", pTexture->target);
                            getEnum = 0;
                            break;
                    }

                    if (getEnum)
                    {
                        GLint curTex;
                        diff_api.GetIntegerv(getEnum, &curTex);
                        if ((GLuint)curTex != pTexture->hwid)
                        {
                            crWarning("texture not bound properly: expected %d, but was %d. Texture state data: target(0x%x), id(%d), w(%d), h(%d)",
                                    pTexture->hwid, curTex,
                                    pTexture->target,
                                    pTexture->id,
                                    ptl->width,
                                    ptl->height);
                            bound = -1;
                        }
                    }

                }

                if (pTexture->target!=GL_TEXTURE_CUBE_MAP_ARB)
                {
                    target = pTexture->target;
                }
                else
                {
                    target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
                }

#ifdef DEBUG
                pImg = crAlloc(ptl->bytes+4);
#else
                pImg = crAlloc(ptl->bytes);
#endif
                if (!pImg) return VERR_NO_MEMORY;

#ifdef DEBUG
                *(int*)((char*)pImg+ptl->bytes) = 0xDEADDEAD;
#endif
                if (bound > 0)
                {
#ifdef DEBUG
                    {
                        GLint w,h=0;
                        crDebug("get image: compressed %i, face %i, level %i, width %i, height %i, bytes %i",
                                ptl->compressed, face, i, ptl->width, ptl->height, ptl->bytes);
                        diff_api.GetTexLevelParameteriv(target, i, GL_TEXTURE_WIDTH, &w);
                        diff_api.GetTexLevelParameteriv(target, i, GL_TEXTURE_HEIGHT, &h);
                        if (w!=ptl->width || h!=ptl->height)
                        {
                            crWarning("!!!tex size mismatch %i, %i!!!", w, h);
                        }
                    }
#endif

                    /** @todo ugly workaround for crashes inside ati driver,
                     *       they overwrite their own allocated memory in cases where texlevel >=4
                             and width or height <=2.
                     */
                    if (i<4 || (ptl->width>2 && ptl->height>2))
                    {
                        if (!ptl->compressed)
                        {
                            diff_api.GetTexImage(target, i, ptl->format, ptl->type, pImg);
                        }
                        else
                        {
                            diff_api.GetCompressedTexImageARB(target, i, pImg);
                        }
                    }
                }
                else
                {
                    crMemset(pImg, 0, ptl->bytes);
                }

#ifdef DEBUG
                if (*(int*)((char*)pImg+ptl->bytes) != 0xDEADDEAD)
                {
                    crWarning("Texture is bigger than expected!!!");
                }
#endif

                rc = SSMR3PutMem(pSSM, pImg, ptl->bytes);
                crFree(pImg);
                AssertRCReturn(rc, rc);
            }
#endif
        }
    }

    crDebug("crStateSaveTextureObjData %u. END", pTexture->id);

    return VINF_SUCCESS;
}

static int32_t crStateLoadTextureObjData(CRTextureObj *pTexture, PSSMHANDLE pSSM)
{
    int32_t rc, face, i;

    CRASSERT(pTexture && pSSM);

    for (face = 0; face < 6; face++) {
        CRASSERT(pTexture->level[face]);

        for (i = 0; i < CR_MAX_MIPMAP_LEVELS; i++) {
            CRTextureLevel *ptl = &(pTexture->level[face][i]);
            CRASSERT(!ptl->img);

            rc = SSMR3GetMem(pSSM, ptl, sizeof(*ptl));
            AssertRCReturn(rc, rc);
            if (ptl->img)
            {
                CRASSERT(ptl->bytes);

                ptl->img = crAlloc(ptl->bytes);
                if (!ptl->img) return VERR_NO_MEMORY;

                rc = SSMR3GetMem(pSSM, ptl->img, ptl->bytes);
                AssertRCReturn(rc, rc);
            }
#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
            /* Same story as in crStateSaveTextureObjData */
            else if (ptl->bytes)
            {
                ptl->img = crAlloc(ptl->bytes);
                if (!ptl->img) return VERR_NO_MEMORY;

                rc = SSMR3GetMem(pSSM, ptl->img, ptl->bytes);
                AssertRCReturn(rc, rc);
            }
#endif
            crStateTextureInitTextureFormat(ptl, ptl->internalFormat);
        }
    }

    return VINF_SUCCESS;
}

static void crStateSaveSharedTextureCB(unsigned long key, void *data1, void *data2)
{
    CRTextureObj *pTexture = (CRTextureObj *) data1;
    PSSMHANDLE pSSM = (PSSMHANDLE) data2;
    int32_t rc;

    CRASSERT(pTexture && pSSM);

    rc = SSMR3PutMem(pSSM, &key, sizeof(key));
    CRASSERT(rc == VINF_SUCCESS);
    rc = SSMR3PutMem(pSSM, pTexture, sizeof(*pTexture));
    CRASSERT(rc == VINF_SUCCESS);
    rc = crStateSaveTextureObjData(pTexture, pSSM);
    CRASSERT(rc == VINF_SUCCESS);
}

static int32_t crStateSaveMatrixStack(CRMatrixStack *pStack, PSSMHANDLE pSSM)
{
    return SSMR3PutMem(pSSM, pStack->stack, sizeof(CRmatrix) * pStack->maxDepth);
}

static int32_t crStateLoadMatrixStack(CRMatrixStack *pStack, PSSMHANDLE pSSM)
{
    int32_t rc;

    CRASSERT(pStack && pSSM);

    rc = SSMR3GetMem(pSSM, pStack->stack, sizeof(CRmatrix) * pStack->maxDepth);
    /* fixup stack top pointer */
    pStack->top = &pStack->stack[pStack->depth];
    return rc;
}

static int32_t crStateSaveTextureObjPtr(CRTextureObj *pTexture, PSSMHANDLE pSSM)
{
    /* Current texture pointer can't be NULL for real texture unit states,
     * but it could be NULL for unused attribute stack depths.
     */
    if (pTexture)
        return SSMR3PutU32(pSSM, pTexture->id);
    else
        return VINF_SUCCESS;
}

static int32_t crStateLoadTextureObjPtr(CRTextureObj **pTexture, CRContext *pContext, GLenum target, PSSMHANDLE pSSM)
{
    uint32_t texName;
    int32_t rc;

    /* We're loading attrib stack with unused state */
    if (!*pTexture)
        return VINF_SUCCESS;

    rc = SSMR3GetU32(pSSM, &texName);
    AssertRCReturn(rc, rc);

    if (texName)
    {
        *pTexture = (CRTextureObj *) crHashtableSearch(pContext->shared->textureTable, texName);
    }
    else
    {
        switch (target)
        {
            case GL_TEXTURE_1D:
                *pTexture = &(pContext->texture.base1D);
                break;
            case GL_TEXTURE_2D:
                *pTexture = &(pContext->texture.base2D);
                break;
#ifdef CR_OPENGL_VERSION_1_2
            case GL_TEXTURE_3D:
                *pTexture = &(pContext->texture.base3D);
                break;
#endif
#ifdef CR_ARB_texture_cube_map
            case GL_TEXTURE_CUBE_MAP_ARB:
                *pTexture = &(pContext->texture.baseCubeMap);
                break;
#endif
#ifdef CR_NV_texture_rectangle
            case GL_TEXTURE_RECTANGLE_NV:
                *pTexture = &(pContext->texture.baseRect);
                break;
#endif
            default:
                crError("LoadTextureObjPtr: Unknown texture target %d", target);
        }
    }

    return rc;
}

static int32_t crStateSaveTexUnitCurrentTexturePtrs(CRTextureUnit *pTexUnit, PSSMHANDLE pSSM)
{
    int32_t rc;

    rc = crStateSaveTextureObjPtr(pTexUnit->currentTexture1D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateSaveTextureObjPtr(pTexUnit->currentTexture2D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateSaveTextureObjPtr(pTexUnit->currentTexture3D, pSSM);
    AssertRCReturn(rc, rc);
#ifdef CR_ARB_texture_cube_map
    rc = crStateSaveTextureObjPtr(pTexUnit->currentTextureCubeMap, pSSM);
    AssertRCReturn(rc, rc);
#endif
#ifdef CR_NV_texture_rectangle
    rc = crStateSaveTextureObjPtr(pTexUnit->currentTextureRect, pSSM);
    AssertRCReturn(rc, rc);
#endif

    return rc;
}

static int32_t crStateLoadTexUnitCurrentTexturePtrs(CRTextureUnit *pTexUnit, CRContext *pContext, PSSMHANDLE pSSM)
{
    int32_t rc;

    rc = crStateLoadTextureObjPtr(&pTexUnit->currentTexture1D, pContext, GL_TEXTURE_1D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadTextureObjPtr(&pTexUnit->currentTexture2D, pContext, GL_TEXTURE_1D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadTextureObjPtr(&pTexUnit->currentTexture3D, pContext, GL_TEXTURE_2D, pSSM);
    AssertRCReturn(rc, rc);
#ifdef CR_ARB_texture_cube_map
    rc = crStateLoadTextureObjPtr(&pTexUnit->currentTextureCubeMap, pContext, GL_TEXTURE_CUBE_MAP_ARB, pSSM);
    AssertRCReturn(rc, rc);
#endif
#ifdef CR_NV_texture_rectangle
    rc = crStateLoadTextureObjPtr(&pTexUnit->currentTextureRect, pContext, GL_TEXTURE_RECTANGLE_NV, pSSM);
    AssertRCReturn(rc, rc);
#endif

    return rc;
}

static int32_t crSateSaveEvalCoeffs1D(CREvaluator1D *pEval, PSSMHANDLE pSSM)
{
    int32_t rc, i;

    for (i=0; i<GLEVAL_TOT; ++i)
    {
        if (pEval[i].coeff)
        {
            rc = SSMR3PutMem(pSSM, pEval[i].coeff, pEval[i].order * gleval_sizes[i] * sizeof(GLfloat));
            AssertRCReturn(rc, rc);
        }
    }

    return VINF_SUCCESS;
}

static int32_t crSateSaveEvalCoeffs2D(CREvaluator2D *pEval, PSSMHANDLE pSSM)
{
    int32_t rc, i;

    for (i=0; i<GLEVAL_TOT; ++i)
    {
        if (pEval[i].coeff)
        {
            rc = SSMR3PutMem(pSSM, pEval[i].coeff, pEval[i].uorder * pEval[i].vorder * gleval_sizes[i] * sizeof(GLfloat));
            AssertRCReturn(rc, rc);
        }
    }

    return VINF_SUCCESS;
}

static int32_t crSateLoadEvalCoeffs1D(CREvaluator1D *pEval, GLboolean bReallocMem, PSSMHANDLE pSSM)
{
    int32_t rc, i;
    size_t size;

    for (i=0; i<GLEVAL_TOT; ++i)
    {
        if (pEval[i].coeff)
        {
            size = pEval[i].order * gleval_sizes[i] * sizeof(GLfloat);
            if (bReallocMem)
            {
                pEval[i].coeff = (GLfloat*) crAlloc((unsigned int /* this case is just so stupid */)size);
                if (!pEval[i].coeff) return VERR_NO_MEMORY;
            }
            rc = SSMR3GetMem(pSSM, pEval[i].coeff, size);
            AssertRCReturn(rc, rc);
        }
    }

    return VINF_SUCCESS;
}

static int32_t crSateLoadEvalCoeffs2D(CREvaluator2D *pEval, GLboolean bReallocMem, PSSMHANDLE pSSM)
{
    int32_t rc, i;
    size_t size;

    for (i=0; i<GLEVAL_TOT; ++i)
    {
        if (pEval[i].coeff)
        {
            size = pEval[i].uorder * pEval[i].vorder * gleval_sizes[i] * sizeof(GLfloat);
            if (bReallocMem)
            {
                pEval[i].coeff = (GLfloat*) crAlloc((unsigned int /* this case is just so stupid */)size);
                if (!pEval[i].coeff) return VERR_NO_MEMORY;
            }
            rc = SSMR3GetMem(pSSM, pEval[i].coeff, size);
            AssertRCReturn(rc, rc);
        }
    }

    return VINF_SUCCESS;
}

static void crStateCopyEvalPtrs1D(CREvaluator1D *pDst, CREvaluator1D *pSrc)
{
    int32_t i;

    for (i=0; i<GLEVAL_TOT; ++i)
        pDst[i].coeff = pSrc[i].coeff;

    /*
    pDst[GL_MAP1_VERTEX_3-GL_MAP1_COLOR_4].coeff = pSrc[GL_MAP1_VERTEX_3-GL_MAP1_COLOR_4].coeff;
    pDst[GL_MAP1_VERTEX_4-GL_MAP1_COLOR_4].coeff = pSrc[GL_MAP1_VERTEX_4-GL_MAP1_COLOR_4].coeff;
    pDst[GL_MAP1_INDEX-GL_MAP1_COLOR_4].coeff = pSrc[GL_MAP1_INDEX-GL_MAP1_COLOR_4].coeff;
    pDst[GL_MAP1_COLOR_4-GL_MAP1_COLOR_4].coeff = pSrc[GL_MAP1_COLOR_4-GL_MAP1_COLOR_4].coeff;
    pDst[GL_MAP1_NORMAL-GL_MAP1_COLOR_4].coeff = pSrc[GL_MAP1_NORMAL-GL_MAP1_COLOR_4].coeff;
    pDst[GL_MAP1_TEXTURE_COORD_1-GL_MAP1_COLOR_4].coeff = pSrc[GL_MAP1_TEXTURE_COORD_1-GL_MAP1_COLOR_4].coeff;
    pDst[GL_MAP1_TEXTURE_COORD_2-GL_MAP1_COLOR_4].coeff = pSrc[GL_MAP1_TEXTURE_COORD_2-GL_MAP1_COLOR_4].coeff;
    pDst[GL_MAP1_TEXTURE_COORD_3-GL_MAP1_COLOR_4].coeff = pSrc[GL_MAP1_TEXTURE_COORD_3-GL_MAP1_COLOR_4].coeff;
    pDst[GL_MAP1_TEXTURE_COORD_4-GL_MAP1_COLOR_4].coeff = pSrc[GL_MAP1_TEXTURE_COORD_4-GL_MAP1_COLOR_4].coeff;
    */
}

static void crStateCopyEvalPtrs2D(CREvaluator2D *pDst, CREvaluator2D *pSrc)
{
    int32_t i;

    for (i=0; i<GLEVAL_TOT; ++i)
        pDst[i].coeff = pSrc[i].coeff;

    /*
    pDst[GL_MAP2_VERTEX_3-GL_MAP2_COLOR_4].coeff = pSrc[GL_MAP2_VERTEX_3-GL_MAP2_COLOR_4].coeff;
    pDst[GL_MAP2_VERTEX_4-GL_MAP2_COLOR_4].coeff = pSrc[GL_MAP2_VERTEX_4-GL_MAP2_COLOR_4].coeff;
    pDst[GL_MAP2_INDEX-GL_MAP2_COLOR_4].coeff = pSrc[GL_MAP2_INDEX-GL_MAP2_COLOR_4].coeff;
    pDst[GL_MAP2_COLOR_4-GL_MAP2_COLOR_4].coeff = pSrc[GL_MAP2_COLOR_4-GL_MAP2_COLOR_4].coeff;
    pDst[GL_MAP2_NORMAL-GL_MAP2_COLOR_4].coeff = pSrc[GL_MAP2_NORMAL-GL_MAP2_COLOR_4].coeff;
    pDst[GL_MAP2_TEXTURE_COORD_1-GL_MAP2_COLOR_4].coeff = pSrc[GL_MAP2_TEXTURE_COORD_1-GL_MAP2_COLOR_4].coeff;
    pDst[GL_MAP2_TEXTURE_COORD_2-GL_MAP2_COLOR_4].coeff = pSrc[GL_MAP2_TEXTURE_COORD_2-GL_MAP2_COLOR_4].coeff;
    pDst[GL_MAP2_TEXTURE_COORD_3-GL_MAP2_COLOR_4].coeff = pSrc[GL_MAP2_TEXTURE_COORD_3-GL_MAP2_COLOR_4].coeff;
    pDst[GL_MAP2_TEXTURE_COORD_4-GL_MAP2_COLOR_4].coeff = pSrc[GL_MAP2_TEXTURE_COORD_4-GL_MAP2_COLOR_4].coeff;
    */
}

static void crStateSaveBufferObjectCB(unsigned long key, void *data1, void *data2)
{
    CRBufferObject *pBufferObj = (CRBufferObject *) data1;
    PSSMHANDLE pSSM = (PSSMHANDLE) data2;
    int32_t rc;

    CRASSERT(pBufferObj && pSSM);

    rc = SSMR3PutMem(pSSM, &key, sizeof(key));
    CRASSERT(rc == VINF_SUCCESS);
    rc = SSMR3PutMem(pSSM, pBufferObj, sizeof(*pBufferObj));
    CRASSERT(rc == VINF_SUCCESS);

    if (pBufferObj->data)
    {
        /*We could get here even though retainBufferData is false on host side, in case when we're taking snapshot
          after state load and before this context was ever made current*/
        CRASSERT(pBufferObj->size>0);
        rc = SSMR3PutMem(pSSM, pBufferObj->data, pBufferObj->size);
        CRASSERT(rc == VINF_SUCCESS);
    }
    else if (pBufferObj->id!=0 && pBufferObj->size>0)
    {
        diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, pBufferObj->hwid);
        pBufferObj->pointer = diff_api.MapBufferARB(GL_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB);
        rc = SSMR3PutMem(pSSM, &pBufferObj->pointer, sizeof(pBufferObj->pointer));
        CRASSERT(rc == VINF_SUCCESS);
        if (pBufferObj->pointer)
        {
            rc = SSMR3PutMem(pSSM, pBufferObj->pointer, pBufferObj->size);
            CRASSERT(rc == VINF_SUCCESS);
        }
        diff_api.UnmapBufferARB(GL_ARRAY_BUFFER_ARB);
        pBufferObj->pointer = NULL;
    }
}

static void crStateSaveProgramCB(unsigned long key, void *data1, void *data2)
{
    CRProgram *pProgram = (CRProgram *) data1;
    PSSMHANDLE pSSM = (PSSMHANDLE) data2;
    CRProgramSymbol *pSymbol;
    int32_t rc;

    CRASSERT(pProgram && pSSM);

    rc = SSMR3PutMem(pSSM, &key, sizeof(key));
    CRASSERT(rc == VINF_SUCCESS);
    rc = SSMR3PutMem(pSSM, pProgram, sizeof(*pProgram));
    CRASSERT(rc == VINF_SUCCESS);
    if (pProgram->string)
    {
        CRASSERT(pProgram->length);
        rc = SSMR3PutMem(pSSM, pProgram->string, pProgram->length);
        CRASSERT(rc == VINF_SUCCESS);
    }

    for (pSymbol = pProgram->symbolTable; pSymbol; pSymbol=pSymbol->next)
    {
        rc = SSMR3PutMem(pSSM, pSymbol, sizeof(*pSymbol));
        CRASSERT(rc == VINF_SUCCESS);
        if (pSymbol->name)
        {
            CRASSERT(pSymbol->cbName>0);
            rc = SSMR3PutMem(pSSM, pSymbol->name, pSymbol->cbName);
            CRASSERT(rc == VINF_SUCCESS);
        }
    }
}

static void crStateSaveFramebuffersCB(unsigned long key, void *data1, void *data2)
{
    CRFramebufferObject *pFBO = (CRFramebufferObject*) data1;
    PSSMHANDLE pSSM = (PSSMHANDLE) data2;
    int32_t rc;

    rc = SSMR3PutMem(pSSM, &key, sizeof(key));
    CRASSERT(rc == VINF_SUCCESS);

    rc = SSMR3PutMem(pSSM, pFBO, sizeof(*pFBO));
    CRASSERT(rc == VINF_SUCCESS);
}

static void crStateSaveRenderbuffersCB(unsigned long key, void *data1, void *data2)
{
    CRRenderbufferObject *pRBO = (CRRenderbufferObject*) data1;
    PSSMHANDLE pSSM = (PSSMHANDLE) data2;
    int32_t rc;

    rc = SSMR3PutMem(pSSM, &key, sizeof(key));
    CRASSERT(rc == VINF_SUCCESS);

    rc = SSMR3PutMem(pSSM, pRBO, sizeof(*pRBO));
    CRASSERT(rc == VINF_SUCCESS);
}

static int32_t crStateLoadProgram(CRProgram **ppProgram, PSSMHANDLE pSSM)
{
    CRProgramSymbol **ppSymbol;
    int32_t rc;
    unsigned long key;

    rc = SSMR3GetMem(pSSM, &key, sizeof(key));
    AssertRCReturn(rc, rc);

    /* we're loading default vertex or pixel program*/
    if (*ppProgram)
    {
        if (key!=0) return VERR_SSM_UNEXPECTED_DATA;
    }
    else
    {
        *ppProgram = (CRProgram*) crAlloc(sizeof(CRProgram));
        if (!ppProgram) return VERR_NO_MEMORY;
        if (key==0) return VERR_SSM_UNEXPECTED_DATA;
    }

    rc = SSMR3GetMem(pSSM, *ppProgram, sizeof(**ppProgram));
    AssertRCReturn(rc, rc);

    if ((*ppProgram)->string)
    {
        CRASSERT((*ppProgram)->length);
        (*ppProgram)->string = crAlloc((*ppProgram)->length);
        if (!(*ppProgram)->string) return VERR_NO_MEMORY;
        rc = SSMR3GetMem(pSSM, (void*) (*ppProgram)->string, (*ppProgram)->length);
        AssertRCReturn(rc, rc);
    }

    for (ppSymbol = &(*ppProgram)->symbolTable; *ppSymbol; ppSymbol=&(*ppSymbol)->next)
    {
        *ppSymbol = crAlloc(sizeof(CRProgramSymbol));
        if (!ppSymbol) return VERR_NO_MEMORY;

        rc = SSMR3GetMem(pSSM, *ppSymbol, sizeof(**ppSymbol));
        AssertRCReturn(rc, rc);

        if ((*ppSymbol)->name)
        {
            CRASSERT((*ppSymbol)->cbName>0);
            (*ppSymbol)->name = crAlloc((*ppSymbol)->cbName);
            if (!(*ppSymbol)->name) return VERR_NO_MEMORY;

            rc = SSMR3GetMem(pSSM, (void*) (*ppSymbol)->name, (*ppSymbol)->cbName);
            AssertRCReturn(rc, rc);
        }
    }

    return VINF_SUCCESS;
}

static void crStateSaveString(const char *pStr, PSSMHANDLE pSSM)
{
    int32_t len;
    int32_t rc;

    if (pStr)
    {
        len = crStrlen(pStr)+1;

        rc = SSMR3PutS32(pSSM, len);
        CRASSERT(rc == VINF_SUCCESS);

        rc = SSMR3PutMem(pSSM, pStr, len*sizeof(*pStr));
        CRASSERT(rc == VINF_SUCCESS);
    }
    else
    {
        rc = SSMR3PutS32(pSSM, 0);
        CRASSERT(rc == VINF_SUCCESS);
    }
}

static char* crStateLoadString(PSSMHANDLE pSSM)
{
    int32_t len, rc;
    char* pStr = NULL;

    rc = SSMR3GetS32(pSSM, &len);
    CRASSERT(rc == VINF_SUCCESS);

    if (len!=0)
    {
        pStr = crAlloc(len*sizeof(*pStr));

        rc = SSMR3GetMem(pSSM, pStr, len*sizeof(*pStr));
        CRASSERT(rc == VINF_SUCCESS);
    }

    return pStr;
}

static void crStateSaveGLSLShaderCB(unsigned long key, void *data1, void *data2)
{
    CRGLSLShader *pShader = (CRGLSLShader*) data1;
    PSSMHANDLE pSSM = (PSSMHANDLE) data2;
    int32_t rc;

    rc = SSMR3PutMem(pSSM, &key, sizeof(key));
    CRASSERT(rc == VINF_SUCCESS);

    rc = SSMR3PutMem(pSSM, pShader, sizeof(*pShader));
    CRASSERT(rc == VINF_SUCCESS);

    if (pShader->source)
    {
        crStateSaveString(pShader->source, pSSM);
    }
    else
    {
        GLint sLen=0;
        GLchar *source=NULL;

        diff_api.GetShaderiv(pShader->hwid, GL_SHADER_SOURCE_LENGTH, &sLen);
        if (sLen>0)
        {
            source = (GLchar*) crAlloc(sLen);
            diff_api.GetShaderSource(pShader->hwid, sLen, NULL, source);
        }

        crStateSaveString(source, pSSM);
        if (source) crFree(source);
    }
}

static CRGLSLShader* crStateLoadGLSLShader(PSSMHANDLE pSSM)
{
    CRGLSLShader *pShader;
    int32_t rc;
    unsigned long key;

    pShader = crAlloc(sizeof(*pShader));
    if (!pShader) return NULL;

    rc = SSMR3GetMem(pSSM, &key, sizeof(key));
    CRASSERT(rc == VINF_SUCCESS);

    rc = SSMR3GetMem(pSSM, pShader, sizeof(*pShader));
    CRASSERT(rc == VINF_SUCCESS);

    pShader->source = crStateLoadString(pSSM);

    return pShader;
}


static void crStateSaveGLSLShaderKeyCB(unsigned long key, void *data1, void *data2)
{
    //CRGLSLShader *pShader = (CRGLSLShader*) data1;
    PSSMHANDLE pSSM = (PSSMHANDLE) data2;
    int32_t rc;

    rc = SSMR3PutMem(pSSM, &key, sizeof(key));
    CRASSERT(rc == VINF_SUCCESS);
}

static void crStateSaveGLSLProgramAttribs(CRGLSLProgramState *pState, PSSMHANDLE pSSM)
{
    GLuint i;
    int32_t rc;

    for (i=0; i<pState->cAttribs; ++i)
    {
        rc = SSMR3PutMem(pSSM, &pState->pAttribs[i].index, sizeof(pState->pAttribs[i].index));
        CRASSERT(rc == VINF_SUCCESS);
        crStateSaveString(pState->pAttribs[i].name, pSSM);
    }
}

static void crStateSaveGLSLProgramCB(unsigned long key, void *data1, void *data2)
{
    CRGLSLProgram *pProgram = (CRGLSLProgram*) data1;
    PSSMHANDLE pSSM = (PSSMHANDLE) data2;
    int32_t rc;
    uint32_t ui32;
    GLint maxUniformLen, activeUniforms=0, uniformsCount=0, i, j;
    GLchar *name = NULL;
    GLenum type;
    GLint size, location;

    rc = SSMR3PutMem(pSSM, &key, sizeof(key));
    CRASSERT(rc == VINF_SUCCESS);

    rc = SSMR3PutMem(pSSM, pProgram, sizeof(*pProgram));
    CRASSERT(rc == VINF_SUCCESS);

    ui32 = crHashtableNumElements(pProgram->currentState.attachedShaders);
    rc = SSMR3PutU32(pSSM, ui32);
    CRASSERT(rc == VINF_SUCCESS);

    crHashtableWalk(pProgram->currentState.attachedShaders, crStateSaveGLSLShaderKeyCB, pSSM);

    if (pProgram->activeState.attachedShaders)
    {
        ui32 = crHashtableNumElements(pProgram->activeState.attachedShaders);
        rc = SSMR3PutU32(pSSM, ui32);
        CRASSERT(rc == VINF_SUCCESS);
        crHashtableWalk(pProgram->currentState.attachedShaders, crStateSaveGLSLShaderCB, pSSM);
    }

    crStateSaveGLSLProgramAttribs(&pProgram->currentState, pSSM);
    crStateSaveGLSLProgramAttribs(&pProgram->activeState, pSSM);

    diff_api.GetProgramiv(pProgram->hwid, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformLen);
    diff_api.GetProgramiv(pProgram->hwid, GL_ACTIVE_UNIFORMS, &activeUniforms);

    if (!maxUniformLen)
    {
        if (activeUniforms)
        {
            crWarning("activeUniforms (%d), while maxUniformLen is zero", activeUniforms);
            activeUniforms = 0;
        }
    }

    if (activeUniforms>0)
    {
        name = (GLchar *) crAlloc((maxUniformLen+8)*sizeof(GLchar));

        if (!name)
        {
            crWarning("crStateSaveGLSLProgramCB: out of memory");
            return;
        }
    }

    for (i=0; i<activeUniforms; ++i)
    {
        diff_api.GetActiveUniform(pProgram->hwid, i, maxUniformLen, NULL, &size, &type, name);
        uniformsCount += size;
    }
    CRASSERT(uniformsCount>=activeUniforms);

    rc = SSMR3PutS32(pSSM, uniformsCount);
    CRASSERT(rc == VINF_SUCCESS);

    if (activeUniforms>0)
    {
        GLfloat fdata[16];
        GLint idata[16];
        char *pIndexStr=NULL;

        for (i=0; i<activeUniforms; ++i)
        {
            diff_api.GetActiveUniform(pProgram->hwid, i, maxUniformLen, NULL, &size, &type, name);

            if (size>1)
            {
                pIndexStr = crStrchr(name, '[');
                if (!pIndexStr)
                {
                    pIndexStr = name+crStrlen(name);
                }
            }

            for (j=0; j<size; ++j)
            {
                if (size>1)
                {
                    sprintf(pIndexStr, "[%i]", j);
                }
                location = diff_api.GetUniformLocation(pProgram->hwid, name);

                rc = SSMR3PutMem(pSSM, &type, sizeof(type));
                CRASSERT(rc == VINF_SUCCESS);

                crStateSaveString(name, pSSM);

                if (crStateIsIntUniform(type))
                {
                    diff_api.GetUniformiv(pProgram->hwid, location, &idata[0]);
                    rc = SSMR3PutMem(pSSM, &idata[0], crStateGetUniformSize(type)*sizeof(idata[0]));
                    CRASSERT(rc == VINF_SUCCESS);
                }
                else
                {
                    diff_api.GetUniformfv(pProgram->hwid, location, &fdata[0]);
                    rc = SSMR3PutMem(pSSM, &fdata[0], crStateGetUniformSize(type)*sizeof(fdata[0]));
                    CRASSERT(rc == VINF_SUCCESS);
                }
            }
        }

        crFree(name);
    }
}

static int32_t crStateSaveClientPointer(CRVertexArrays *pArrays, int32_t index, PSSMHANDLE pSSM)
{
    int32_t rc;
    CRClientPointer *cp;

    cp = crStateGetClientPointerByIndex(index, pArrays);

    if (cp->buffer)
        rc = SSMR3PutU32(pSSM, cp->buffer->id);
    else
        rc = SSMR3PutU32(pSSM, 0);

    AssertRCReturn(rc, rc);

#ifdef CR_EXT_compiled_vertex_array
    if (cp->locked)
    {
        CRASSERT(cp->p);
        rc = SSMR3PutMem(pSSM, cp->p, cp->stride*(pArrays->lockFirst+pArrays->lockCount));
        AssertRCReturn(rc, rc);
    }
#endif

    return VINF_SUCCESS;
}

static int32_t crStateLoadClientPointer(CRVertexArrays *pArrays, int32_t index, CRContext *pContext, PSSMHANDLE pSSM)
{
    int32_t rc;
    uint32_t ui;
    CRClientPointer *cp;

    cp = crStateGetClientPointerByIndex(index, pArrays);

    rc = SSMR3GetU32(pSSM, &ui);
    AssertRCReturn(rc, rc);
    cp->buffer = ui==0 ? pContext->bufferobject.nullBuffer : crHashtableSearch(pContext->shared->buffersTable, ui);

    if (!cp->buffer)
    {
        crWarning("crStateLoadClientPointer: ui=%d loaded as NULL buffer!", ui);
    }

#ifdef CR_EXT_compiled_vertex_array
    if (cp->locked)
    {
        rc = crStateAllocAndSSMR3GetMem(pSSM, (void**)&cp->p, cp->stride*(pArrays->lockFirst+pArrays->lockCount));
        AssertRCReturn(rc, rc);
    }
#endif

    return VINF_SUCCESS;
}

static int32_t crStateSaveCurrentBits(CRStateBits *pBits, PSSMHANDLE pSSM)
{
    int32_t rc, i;

    rc = SSMR3PutMem(pSSM, pBits, sizeof(*pBits));
    AssertRCReturn(rc, rc);

    rc = SSMR3PutMem(pSSM, pBits->client.v, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    rc = SSMR3PutMem(pSSM, pBits->client.n, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    rc = SSMR3PutMem(pSSM, pBits->client.c, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    rc = SSMR3PutMem(pSSM, pBits->client.s, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    rc = SSMR3PutMem(pSSM, pBits->client.i, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    for (i=0; i<CR_MAX_TEXTURE_UNITS; i++)
    {
        rc = SSMR3PutMem(pSSM, pBits->client.t[i], GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
        AssertRCReturn(rc, rc);
    }
    rc = SSMR3PutMem(pSSM, pBits->client.e, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    rc = SSMR3PutMem(pSSM, pBits->client.f, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
#ifdef CR_NV_vertex_program
    for (i=0; i<CR_MAX_VERTEX_ATTRIBS; i++)
    {
        rc = SSMR3PutMem(pSSM, pBits->client.a[i], GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
        AssertRCReturn(rc, rc);
    }
#endif

    rc = SSMR3PutMem(pSSM, pBits->lighting.light, CR_MAX_LIGHTS*sizeof(pBits->lighting.light));
    AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}

static int32_t crStateLoadCurrentBits(CRStateBits *pBits, PSSMHANDLE pSSM)
{
    int32_t rc, i;
    CRClientBits client;
    CRLightingBits lighting;

    CRASSERT(pBits);

    client.v = pBits->client.v;
    client.n = pBits->client.n;
    client.c = pBits->client.c;
    client.s = pBits->client.s;
    client.i = pBits->client.i;
    client.e = pBits->client.e;
    client.f = pBits->client.f;
    for (i=0; i<CR_MAX_TEXTURE_UNITS; i++)
    {
        client.t[i] = pBits->client.t[i];
    }
#ifdef CR_NV_vertex_program
    for (i=0; i<CR_MAX_VERTEX_ATTRIBS; i++)
    {
        client.a[i] = pBits->client.a[i];
    }
#endif
    lighting.light = pBits->lighting.light;

    rc = SSMR3GetMem(pSSM, pBits, sizeof(*pBits));
    AssertRCReturn(rc, rc);

    pBits->client.v = client.v;
    rc = SSMR3GetMem(pSSM, pBits->client.v, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    pBits->client.n = client.n;
    rc = SSMR3GetMem(pSSM, pBits->client.n, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    pBits->client.c = client.c;
    rc = SSMR3GetMem(pSSM, pBits->client.c, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    pBits->client.s = client.s;
    rc = SSMR3GetMem(pSSM, pBits->client.s, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    pBits->client.i = client.i;
    rc = SSMR3GetMem(pSSM, pBits->client.i, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    pBits->client.e = client.e;
    rc = SSMR3GetMem(pSSM, pBits->client.e, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    pBits->client.f = client.f;
    rc = SSMR3GetMem(pSSM, pBits->client.f, GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
    AssertRCReturn(rc, rc);
    for (i=0; i<CR_MAX_TEXTURE_UNITS; i++)
    {
        pBits->client.t[i] = client.t[i];
        rc = SSMR3GetMem(pSSM, pBits->client.t[i], GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
        AssertRCReturn(rc, rc);
    }
#ifdef CR_NV_vertex_program
    for (i=0; i<CR_MAX_VERTEX_ATTRIBS; i++)
    {
        pBits->client.a[i] = client.a[i];
        rc = SSMR3GetMem(pSSM, pBits->client.a[i], GLCLIENT_BIT_ALLOC*sizeof(CRbitvalue));
        AssertRCReturn(rc, rc);
    }
#endif

    pBits->lighting.light = lighting.light;
    rc = SSMR3GetMem(pSSM, pBits->lighting.light, CR_MAX_LIGHTS*sizeof(pBits->lighting.light));
    AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}

static void crStateSaveKeysCB(unsigned long firstKey, unsigned long count, void *data)
{
    PSSMHANDLE pSSM = (PSSMHANDLE)data;
    int rc;
    CRASSERT(firstKey);
    CRASSERT(count);
    rc = SSMR3PutU32(pSSM, firstKey);
    CRASSERT(RT_SUCCESS(rc));
    rc = SSMR3PutU32(pSSM, count);
    CRASSERT(RT_SUCCESS(rc));
}

static int32_t crStateSaveKeys(CRHashTable *pHash, PSSMHANDLE pSSM)
{
    crHashtableWalkKeys(pHash, crStateSaveKeysCB , pSSM);
    /* use null terminator */
    SSMR3PutU32(pSSM, 0);
    return VINF_SUCCESS;
}

static int32_t crStateLoadKeys(CRHashTable *pHash, PSSMHANDLE pSSM, uint32_t u32Version)
{
    uint32_t u32Key, u32Count, i;
    int rc;
    for(;;)
    {
        rc = SSMR3GetU32(pSSM, &u32Key);
        AssertRCReturn(rc, rc);

        if (!u32Key)
            return rc;

        rc = SSMR3GetU32(pSSM, &u32Count);
        AssertRCReturn(rc, rc);

        CRASSERT(u32Count);

        if (u32Version > SHCROGL_SSM_VERSION_WITH_BUGGY_KEYS)
        {
            for (i = u32Key; i < u32Count + u32Key; ++i)
            {
                GLboolean fIsNew = crHashtableAllocRegisterKey(pHash, i); NOREF(fIsNew);
#if 0 //def DEBUG_misha
                CRASSERT(fIsNew);
#endif
            }
        }
    }
    /* not reached*/
}


int32_t crStateSaveContext(CRContext *pContext, PSSMHANDLE pSSM)
{
    int32_t rc, i;
    uint32_t ui32, j;
    GLboolean bSaveShared = GL_TRUE;

    CRASSERT(pContext && pSSM);

    CRASSERT(pContext->client.attribStackDepth == 0);

    /* this stuff is not used anymore, zero it up for sanity */
    pContext->buffer.storedWidth = 0;
    pContext->buffer.storedHeight = 0;

    CRASSERT(VBoxTlsRefIsFunctional(pContext));

    /* make sure the gl error state is captured by our state mechanism to store the correct gl  error value */
    crStateSyncHWErrorState(pContext);

    rc = SSMR3PutMem(pSSM, pContext, sizeof (*pContext));
    AssertRCReturn(rc, rc);

    if (crHashtableNumElements(pContext->shared->dlistTable)>0)
        crWarning("Saving state with %d display lists, unsupported", crHashtableNumElements(pContext->shared->dlistTable));

    if (crHashtableNumElements(pContext->program.programHash)>0)
        crDebug("Saving state with %d programs", crHashtableNumElements(pContext->program.programHash));

    rc = SSMR3PutS32(pSSM, pContext->shared->id);
    AssertRCReturn(rc, rc);

    rc = SSMR3PutS32(pSSM, crStateContextIsShared(pContext));
    AssertRCReturn(rc, rc);

    if (pContext->shared->refCount>1)
    {
        bSaveShared = pContext->shared->saveCount==0;

        ++pContext->shared->saveCount;
        if (pContext->shared->saveCount == pContext->shared->refCount)
        {
            pContext->shared->saveCount=0;
        }
    }

    /* Save transform state */
    rc = SSMR3PutMem(pSSM, pContext->transform.clipPlane, sizeof(GLvectord)*CR_MAX_CLIP_PLANES);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutMem(pSSM, pContext->transform.clip, sizeof(GLboolean)*CR_MAX_CLIP_PLANES);
    AssertRCReturn(rc, rc);
    rc = crStateSaveMatrixStack(&pContext->transform.modelViewStack, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateSaveMatrixStack(&pContext->transform.projectionStack, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateSaveMatrixStack(&pContext->transform.colorStack, pSSM);
    AssertRCReturn(rc, rc);
    for (i = 0 ; i < CR_MAX_TEXTURE_UNITS ; i++)
    {
        rc = crStateSaveMatrixStack(&pContext->transform.textureStack[i], pSSM);
        AssertRCReturn(rc, rc);
    }
    for (i = 0 ; i < CR_MAX_PROGRAM_MATRICES ; i++)
    {
        rc = crStateSaveMatrixStack(&pContext->transform.programStack[i], pSSM);
        AssertRCReturn(rc, rc);
    }

    /* Save textures */
    rc = crStateSaveTextureObjData(&pContext->texture.base1D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateSaveTextureObjData(&pContext->texture.base2D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateSaveTextureObjData(&pContext->texture.base3D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateSaveTextureObjData(&pContext->texture.proxy1D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateSaveTextureObjData(&pContext->texture.proxy2D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateSaveTextureObjData(&pContext->texture.proxy3D, pSSM);
#ifdef CR_ARB_texture_cube_map
    rc = crStateSaveTextureObjData(&pContext->texture.baseCubeMap, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateSaveTextureObjData(&pContext->texture.proxyCubeMap, pSSM);
    AssertRCReturn(rc, rc);
#endif
#ifdef CR_NV_texture_rectangle
    rc = crStateSaveTextureObjData(&pContext->texture.baseRect, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateSaveTextureObjData(&pContext->texture.proxyRect, pSSM);
    AssertRCReturn(rc, rc);
#endif

    /* Save shared textures */
    if (bSaveShared)
    {
        CRASSERT(pContext->shared && pContext->shared->textureTable);
        rc = crStateSaveKeys(pContext->shared->textureTable, pSSM);
        AssertRCReturn(rc, rc);
        ui32 = crHashtableNumElements(pContext->shared->textureTable);
        rc = SSMR3PutU32(pSSM, ui32);
        AssertRCReturn(rc, rc);
        crHashtableWalk(pContext->shared->textureTable, crStateSaveSharedTextureCB, pSSM);

#ifdef CR_STATE_NO_TEXTURE_IMAGE_STORE
        /* Restore previous texture bindings via diff_api */
        if (ui32)
        {
            CRTextureUnit *pTexUnit;

            pTexUnit = &pContext->texture.unit[pContext->texture.curTextureUnit];

            diff_api.BindTexture(GL_TEXTURE_1D, pTexUnit->currentTexture1D->hwid);
            diff_api.BindTexture(GL_TEXTURE_2D, pTexUnit->currentTexture2D->hwid);
            diff_api.BindTexture(GL_TEXTURE_3D, pTexUnit->currentTexture3D->hwid);
#ifdef CR_ARB_texture_cube_map
            diff_api.BindTexture(GL_TEXTURE_CUBE_MAP_ARB, pTexUnit->currentTextureCubeMap->hwid);
#endif
#ifdef CR_NV_texture_rectangle
            diff_api.BindTexture(GL_TEXTURE_RECTANGLE_NV, pTexUnit->currentTextureRect->hwid);
#endif
        }
#endif
    }

    /* Save current texture pointers */
    for (i=0; i<CR_MAX_TEXTURE_UNITS; ++i)
    {
        rc = crStateSaveTexUnitCurrentTexturePtrs(&pContext->texture.unit[i], pSSM);
        AssertRCReturn(rc, rc);
    }

    /* Save lights */
    CRASSERT(pContext->lighting.light);
    rc = SSMR3PutMem(pSSM, pContext->lighting.light, CR_MAX_LIGHTS * sizeof(*pContext->lighting.light));
    AssertRCReturn(rc, rc);

    /* Save attrib stack*/
    /** @todo could go up to used stack depth here?*/
    for ( i = 0 ; i < CR_MAX_ATTRIB_STACK_DEPTH ; i++)
    {
        if (pContext->attrib.enableStack[i].clip)
        {
            rc = SSMR3PutMem(pSSM, pContext->attrib.enableStack[i].clip,
                             pContext->limits.maxClipPlanes*sizeof(GLboolean));
            AssertRCReturn(rc, rc);
        }

        if (pContext->attrib.enableStack[i].light)
        {
            rc = SSMR3PutMem(pSSM, pContext->attrib.enableStack[i].light,
                             pContext->limits.maxLights*sizeof(GLboolean));
            AssertRCReturn(rc, rc);
        }

        if (pContext->attrib.lightingStack[i].light)
        {
            rc = SSMR3PutMem(pSSM, pContext->attrib.lightingStack[i].light,
                             pContext->limits.maxLights*sizeof(CRLight));
            AssertRCReturn(rc, rc);
        }

        for (j=0; j<pContext->limits.maxTextureUnits; ++j)
        {
            rc = crStateSaveTexUnitCurrentTexturePtrs(&pContext->attrib.textureStack[i].unit[j], pSSM);
            AssertRCReturn(rc, rc);
        }

        if (pContext->attrib.transformStack[i].clip)
        {
            rc = SSMR3PutMem(pSSM, pContext->attrib.transformStack[i].clip,
                             pContext->limits.maxClipPlanes*sizeof(GLboolean));
            AssertRCReturn(rc, rc);
        }

        if (pContext->attrib.transformStack[i].clipPlane)
        {
            rc = SSMR3PutMem(pSSM, pContext->attrib.transformStack[i].clipPlane,
                             pContext->limits.maxClipPlanes*sizeof(GLvectord));
            AssertRCReturn(rc, rc);
        }

        rc = crSateSaveEvalCoeffs1D(pContext->attrib.evalStack[i].eval1D, pSSM);
        AssertRCReturn(rc, rc);
        rc = crSateSaveEvalCoeffs2D(pContext->attrib.evalStack[i].eval2D, pSSM);
        AssertRCReturn(rc, rc);
    }

    /* Save evaluator coeffs */
    rc = crSateSaveEvalCoeffs1D(pContext->eval.eval1D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crSateSaveEvalCoeffs2D(pContext->eval.eval2D, pSSM);
    AssertRCReturn(rc, rc);

#ifdef CR_ARB_vertex_buffer_object
    /* Save buffer objects */
    if (bSaveShared)
    {
        rc = crStateSaveKeys(pContext->shared->buffersTable, pSSM);
        AssertRCReturn(rc, rc);
    }
    ui32 = bSaveShared? crHashtableNumElements(pContext->shared->buffersTable):0;
    rc = SSMR3PutU32(pSSM, ui32);
    AssertRCReturn(rc, rc);

    /* Save default one*/
    crStateSaveBufferObjectCB(0, pContext->bufferobject.nullBuffer, pSSM);

    if (bSaveShared)
    {
        /* Save all the rest */
        crHashtableWalk(pContext->shared->buffersTable, crStateSaveBufferObjectCB, pSSM);
    }

    /* Restore binding */
    diff_api.BindBufferARB(GL_ARRAY_BUFFER_ARB, pContext->bufferobject.arrayBuffer->hwid);

    /* Save pointers */
    rc = SSMR3PutU32(pSSM, pContext->bufferobject.arrayBuffer->id);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, pContext->bufferobject.elementsBuffer->id);
    AssertRCReturn(rc, rc);
#ifdef CR_ARB_pixel_buffer_object
    rc = SSMR3PutU32(pSSM, pContext->bufferobject.packBuffer->id);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, pContext->bufferobject.unpackBuffer->id);
    AssertRCReturn(rc, rc);
#endif
    /* Save clint pointers and buffer bindings*/
    for (i=0; i<CRSTATECLIENT_MAX_VERTEXARRAYS; ++i)
    {
        rc = crStateSaveClientPointer(&pContext->client.array, i, pSSM);
        AssertRCReturn(rc, rc);
    }

    crDebug("client.vertexArrayStackDepth %i", pContext->client.vertexArrayStackDepth);
    for (i=0; i<pContext->client.vertexArrayStackDepth; ++i)
    {
        CRVertexArrays *pArray = &pContext->client.vertexArrayStack[i];
        for (j=0; j<CRSTATECLIENT_MAX_VERTEXARRAYS; ++j)
        {
            rc = crStateSaveClientPointer(pArray, j, pSSM);
            AssertRCReturn(rc, rc);
        }
    }
#endif /*CR_ARB_vertex_buffer_object*/

    /* Save pixel/vertex programs */
    ui32 = crHashtableNumElements(pContext->program.programHash);
    rc = SSMR3PutU32(pSSM, ui32);
    AssertRCReturn(rc, rc);
    /* Save defaults programs */
    crStateSaveProgramCB(0, pContext->program.defaultVertexProgram, pSSM);
    crStateSaveProgramCB(0, pContext->program.defaultFragmentProgram, pSSM);
    /* Save all the rest */
    crHashtableWalk(pContext->program.programHash, crStateSaveProgramCB, pSSM);
    /* Save Pointers */
    rc = SSMR3PutU32(pSSM, pContext->program.currentVertexProgram->id);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, pContext->program.currentFragmentProgram->id);
    AssertRCReturn(rc, rc);
    /* This one is unused it seems*/
    CRASSERT(!pContext->program.errorString);

#ifdef CR_EXT_framebuffer_object
    /* Save FBOs */
    if (bSaveShared)
    {
        rc = crStateSaveKeys(pContext->shared->fbTable, pSSM);
        AssertRCReturn(rc, rc);
        ui32 = crHashtableNumElements(pContext->shared->fbTable);
        rc = SSMR3PutU32(pSSM, ui32);
        AssertRCReturn(rc, rc);
        crHashtableWalk(pContext->shared->fbTable, crStateSaveFramebuffersCB, pSSM);

        rc = crStateSaveKeys(pContext->shared->rbTable, pSSM);
        AssertRCReturn(rc, rc);
        ui32 = crHashtableNumElements(pContext->shared->rbTable);
        rc = SSMR3PutU32(pSSM, ui32);
        AssertRCReturn(rc, rc);
        crHashtableWalk(pContext->shared->rbTable, crStateSaveRenderbuffersCB, pSSM);
    }
    rc = SSMR3PutU32(pSSM, pContext->framebufferobject.drawFB?pContext->framebufferobject.drawFB->id:0);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, pContext->framebufferobject.readFB?pContext->framebufferobject.readFB->id:0);
    AssertRCReturn(rc, rc);
    rc = SSMR3PutU32(pSSM, pContext->framebufferobject.renderbuffer?pContext->framebufferobject.renderbuffer->id:0);
    AssertRCReturn(rc, rc);
#endif

#ifdef CR_OPENGL_VERSION_2_0
    /* Save GLSL related info */
    ui32 = crHashtableNumElements(pContext->glsl.shaders);
    rc = SSMR3PutU32(pSSM, ui32);
    AssertRCReturn(rc, rc);
    crHashtableWalk(pContext->glsl.shaders, crStateSaveGLSLShaderCB, pSSM);
    ui32 = crHashtableNumElements(pContext->glsl.programs);
    rc = SSMR3PutU32(pSSM, ui32);
    AssertRCReturn(rc, rc);
    crHashtableWalk(pContext->glsl.programs, crStateSaveGLSLProgramCB, pSSM);
    rc = SSMR3PutU32(pSSM, pContext->glsl.activeProgram?pContext->glsl.activeProgram->id:0);
    AssertRCReturn(rc, rc);
#endif

    return VINF_SUCCESS;
}

typedef struct _crFindSharedCtxParms {
    PFNCRSTATE_CONTEXT_GET pfnCtxGet;
    CRContext *pSrcCtx, *pDstCtx;
} crFindSharedCtxParms_t;

static void crStateFindSharedCB(unsigned long key, void *data1, void *data2)
{
    crFindSharedCtxParms_t *pParms = (crFindSharedCtxParms_t *) data2;
    CRContext *pContext = pParms->pfnCtxGet(data1);
    (void) key;

    if (pContext!=pParms->pSrcCtx && pContext->shared->id==pParms->pSrcCtx->shared->id)
    {
        pParms->pDstCtx->shared = pContext->shared;
    }
}

int32_t crStateSaveGlobals(PSSMHANDLE pSSM)
{
    /* don't need that for now */
#if 0
    CRStateBits *pBits;
    int rc;

    CRASSERT(g_cContexts >= 1);
    if (g_cContexts <= 1)
        return VINF_SUCCESS;

    pBits = GetCurrentBits();
#define CRSTATE_BITS_OP(_var, _size) \
        rc = SSMR3PutMem(pSSM, (pBits->_var), _size); \
        AssertRCReturn(rc, rc);
#include "state_bits_globalop.h"
#undef CRSTATE_BITS_OP
#endif
    return VINF_SUCCESS;
}

int32_t crStateLoadGlobals(PSSMHANDLE pSSM, uint32_t u32Version)
{
    CRStateBits *pBits;
    int rc;
    CRASSERT(g_cContexts >= 1);
    if (g_cContexts <= 1)
        return VINF_SUCCESS;

    pBits = GetCurrentBits();

    if (u32Version >= SHCROGL_SSM_VERSION_WITH_STATE_BITS)
    {
#define CRSTATE_BITS_OP(_var, _size) \
            rc = SSMR3GetMem(pSSM, (pBits->_var), _size); \
            AssertRCReturn(rc, rc);

        if (u32Version < SHCROGL_SSM_VERSION_WITH_FIXED_STENCIL)
        {
#define CRSTATE_BITS_OP_VERSION (SHCROGL_SSM_VERSION_WITH_FIXED_STENCIL - 1)
#define CRSTATE_BITS_OP_STENCIL_FUNC_V_33(_i, _var) do {} while (0)
#define CRSTATE_BITS_OP_STENCIL_OP_V_33(_i, _var) do {} while (0)
#include "state_bits_globalop.h"
#undef CRSTATE_BITS_OP_VERSION
#undef CRSTATE_BITS_OP_STENCIL_FUNC_V_33
#undef CRSTATE_BITS_OP_STENCIL_OP_V_33
        }
        else if (u32Version < SHCROGL_SSM_VERSION_WITH_SPRITE_COORD_ORIGIN)
        {
#define CRSTATE_BITS_OP_VERSION (SHCROGL_SSM_VERSION_WITH_SPRITE_COORD_ORIGIN - 1)
#include "state_bits_globalop.h"
#undef CRSTATE_BITS_OP_VERSION
        }
        else
        {
            /* we do not put dirty bits to state anymore,
             * nop */
//#include "state_bits_globalop.h"
        }
#undef CRSTATE_BITS_OP
        /* always dirty all bits */
        /* return VINF_SUCCESS; */
    }

#define CRSTATE_BITS_OP(_var, _size) FILLDIRTY(pBits->_var);
#include "state_bits_globalop.h"
#undef CRSTATE_BITS_OP
    return VINF_SUCCESS;
}


#define SLC_COPYPTR(ptr) pTmpContext->ptr = pContext->ptr
#define SLC_ASSSERT_NULL_PTR(ptr) CRASSERT(!pContext->ptr)

AssertCompile(VBOXTLSREFDATA_SIZE() <= CR_MAX_BITARRAY);
AssertCompile(VBOXTLSREFDATA_STATE_INITIALIZED != 0);
AssertCompile(RTASSERT_OFFSET_OF(CRContext, shared) >= VBOXTLSREFDATA_ASSERT_OFFSET(CRContext)
                                                     + VBOXTLSREFDATA_SIZE()
                                                     + RT_SIZEOFMEMB(CRContext, bitid)
                                                     + RT_SIZEOFMEMB(CRContext, neg_bitid));

int32_t crStateLoadContext(CRContext *pContext, CRHashTable * pCtxTable, PFNCRSTATE_CONTEXT_GET pfnCtxGet, PSSMHANDLE pSSM, uint32_t u32Version)
{
    CRContext* pTmpContext;
    int32_t rc, i, j;
    uint32_t uiNumElems, ui, k;
    unsigned long key;
    GLboolean bLoadShared = GL_TRUE;
    GLenum err;

    CRASSERT(pContext && pSSM);

    /* This one is rather big for stack allocation and causes macs to crash */
    pTmpContext = (CRContext*)crAlloc(sizeof(*pTmpContext));
    if (!pTmpContext)
        return VERR_NO_MEMORY;

    CRASSERT(VBoxTlsRefIsFunctional(pContext));

    if (u32Version <= SHCROGL_SSM_VERSION_WITH_INVALID_ERROR_STATE)
    {
        union {
            CRbitvalue bitid[CR_MAX_BITARRAY];
            struct {
                VBOXTLSREFDATA
            } tlsRef;
        } bitid;

        /* do not increment the saved state version due to VBOXTLSREFDATA addition to CRContext */
        rc = SSMR3GetMem(pSSM, pTmpContext, VBOXTLSREFDATA_OFFSET(CRContext));
        AssertRCReturn(rc, rc);

        /* VBox 4.1.8 had a bug that VBOXTLSREFDATA was also stored in the snapshot,
         * thus the saved state data format was changed w/o changing the saved state version.
         * here we determine whether the saved state contains VBOXTLSREFDATA, and if so, treat it accordingly */
        rc = SSMR3GetMem(pSSM, &bitid, sizeof (bitid));
        AssertRCReturn(rc, rc);

        /* the bitid array has one bit set only. this is why if bitid.tlsRef has both cTlsRefs
         * and enmTlsRefState non-zero - this is definitely NOT a bit id and is a VBOXTLSREFDATA */
        if (bitid.tlsRef.enmTlsRefState == VBOXTLSREFDATA_STATE_INITIALIZED
                && bitid.tlsRef.cTlsRefs)
        {
            /* VBOXTLSREFDATA is stored, skip it */
            crMemcpy(&pTmpContext->bitid, ((uint8_t*)&bitid) + VBOXTLSREFDATA_SIZE(), sizeof (bitid) - VBOXTLSREFDATA_SIZE());
            rc = SSMR3GetMem(pSSM, ((uint8_t*)&pTmpContext->bitid) + sizeof (pTmpContext->bitid) - VBOXTLSREFDATA_SIZE(), sizeof (pTmpContext->neg_bitid) + VBOXTLSREFDATA_SIZE());
            AssertRCReturn(rc, rc);

            ui = VBOXTLSREFDATA_OFFSET(CRContext) + VBOXTLSREFDATA_SIZE() + sizeof (pTmpContext->bitid) + sizeof (pTmpContext->neg_bitid);
            ui = RT_OFFSETOF(CRContext, shared) - ui;
        }
        else
        {
            /* VBOXTLSREFDATA is NOT stored */
            crMemcpy(&pTmpContext->bitid, &bitid, sizeof (bitid));
            rc = SSMR3GetMem(pSSM, &pTmpContext->neg_bitid, sizeof (pTmpContext->neg_bitid));
            AssertRCReturn(rc, rc);

            /* the pre-VBOXTLSREFDATA CRContext structure might have additional allignment bits before the CRContext::shared */
            ui = VBOXTLSREFDATA_OFFSET(CRContext) + sizeof (pTmpContext->bitid) + sizeof (pTmpContext->neg_bitid);

            ui &= (sizeof (void*) - 1);
        }

        if (ui)
        {
            void* pTmp = NULL;
            rc = SSMR3GetMem(pSSM, &pTmp, ui);
            AssertRCReturn(rc, rc);
        }

        if (u32Version == SHCROGL_SSM_VERSION_BEFORE_CTXUSAGE_BITS)
        {
            SHCROGL_GET_STRUCT_PART(pTmpContext, CRContext, shared, attrib);
            rc = crStateLoadAttribState_v_BEFORE_CTXUSAGE_BITS(&pTmpContext->attrib, pSSM);
            AssertRCReturn(rc, rc);
            SHCROGL_CUT_FIELD_ALIGNMENT(CRContext, attrib, buffer);
            SHCROGL_GET_STRUCT_PART(pTmpContext, CRContext, buffer, point);
            rc = crStateLoadStencilPoint_v_37(&pTmpContext->point, pSSM);
            AssertRCReturn(rc, rc);
            SHCROGL_GET_STRUCT_PART(pTmpContext, CRContext, polygon, stencil);
            rc = crStateLoadStencilState_v_33(&pTmpContext->stencil, pSSM);
            AssertRCReturn(rc, rc);
            SHCROGL_CUT_FOR_OLD_TYPE_TO_ENSURE_ALIGNMENT(CRContext, stencil, CRStencilState_v_33, sizeof (void*));
            rc = crStateLoadTextureState_v_BEFORE_CTXUSAGE_BITS(&pTmpContext->texture, pSSM);
            AssertRCReturn(rc, rc);
            SHCROGL_CUT_FIELD_ALIGNMENT(CRContext, texture, transform);
            SHCROGL_GET_STRUCT_TAIL(pTmpContext, CRContext, transform);
        }
        else
        {
            SHCROGL_GET_STRUCT_PART(pTmpContext, CRContext, shared, attrib);
            rc = crStateLoadAttribState_v_33(&pTmpContext->attrib, pSSM);
            AssertRCReturn(rc, rc);
            SHCROGL_CUT_FIELD_ALIGNMENT(CRContext, attrib, buffer);
            SHCROGL_GET_STRUCT_PART(pTmpContext, CRContext, buffer, point);
            rc = crStateLoadStencilPoint_v_37(&pTmpContext->point, pSSM);
            AssertRCReturn(rc, rc);
            SHCROGL_GET_STRUCT_PART(pTmpContext, CRContext, polygon, stencil);
            rc = crStateLoadStencilState_v_33(&pTmpContext->stencil, pSSM);
            AssertRCReturn(rc, rc);
            SHCROGL_CUT_FOR_OLD_TYPE_TO_ENSURE_ALIGNMENT(CRContext, stencil, CRStencilState_v_33, sizeof (void*));
            SHCROGL_GET_STRUCT_TAIL(pTmpContext, CRContext, texture);
        }

        pTmpContext->error = GL_NO_ERROR; /* <- the error state contained some random error data here
                                                   * treat as no error */
    }
    else if (u32Version < SHCROGL_SSM_VERSION_WITH_FIXED_STENCIL)
    {
        SHCROGL_GET_STRUCT_HEAD(pTmpContext, CRContext, attrib);
        rc = crStateLoadAttribState_v_33(&pTmpContext->attrib, pSSM);
        AssertRCReturn(rc, rc);
        SHCROGL_CUT_FIELD_ALIGNMENT(CRContext, attrib, buffer);
        SHCROGL_GET_STRUCT_PART(pTmpContext, CRContext, buffer, point);
        rc = crStateLoadStencilPoint_v_37(&pTmpContext->point, pSSM);
        AssertRCReturn(rc, rc);
        SHCROGL_GET_STRUCT_PART(pTmpContext, CRContext, polygon, stencil);
        rc = crStateLoadStencilState_v_33(&pTmpContext->stencil, pSSM);
        AssertRCReturn(rc, rc);
        SHCROGL_CUT_FOR_OLD_TYPE_TO_ENSURE_ALIGNMENT(CRContext, stencil, CRStencilState_v_33, sizeof (void*));
        SHCROGL_GET_STRUCT_TAIL(pTmpContext, CRContext, texture);
    }
    else if (u32Version < SHCROGL_SSM_VERSION_WITH_SPRITE_COORD_ORIGIN)
    {
        SHCROGL_GET_STRUCT_HEAD(pTmpContext, CRContext, point);
        crStateLoadStencilPoint_v_37(&pTmpContext->point, pSSM);
        SHCROGL_GET_STRUCT_TAIL(pTmpContext, CRContext, polygon);
    }
    else
    {
        rc = SSMR3GetMem(pSSM, pTmpContext, sizeof (*pTmpContext));
        AssertRCReturn(rc, rc);
    }

    /* preserve the error to restore it at the end of context creation,
     * it should not normally change, but just in case it it changed */
    err = pTmpContext->error;

    /* we will later do crMemcpy from entire pTmpContext to pContext,
     * for simplicity store the VBOXTLSREFDATA from the pContext to pTmpContext */
    VBOXTLSREFDATA_COPY(pTmpContext, pContext);

    /* Deal with shared state */
    {
        crFindSharedCtxParms_t parms;
        int32_t shared;

        rc = SSMR3GetS32(pSSM, &pContext->shared->id);
        AssertRCReturn(rc, rc);

        rc = SSMR3GetS32(pSSM, &shared);
        AssertRCReturn(rc, rc);

        pTmpContext->shared = NULL;
        parms.pfnCtxGet = pfnCtxGet;
        parms.pSrcCtx = pContext;
        parms.pDstCtx = pTmpContext;
        crHashtableWalk(pCtxTable, crStateFindSharedCB, &parms);

        if (pTmpContext->shared)
        {
            CRASSERT(pContext->shared->refCount==1);
            bLoadShared = GL_FALSE;
            crStateFreeShared(pContext, pContext->shared);
            pContext->shared = NULL;
            pTmpContext->shared->refCount++;
        }
        else
        {
            SLC_COPYPTR(shared);
        }

        if (bLoadShared && shared)
        {
            crStateSetSharedContext(pTmpContext);
        }
    }

    SLC_COPYPTR(flush_func);
    SLC_COPYPTR(flush_arg);

    /* We're supposed to be loading into an empty context, so those pointers should be NULL */
    for ( i = 0 ; i < CR_MAX_ATTRIB_STACK_DEPTH ; i++)
    {
        SLC_ASSSERT_NULL_PTR(attrib.enableStack[i].clip);
        SLC_ASSSERT_NULL_PTR(attrib.enableStack[i].light);

        SLC_ASSSERT_NULL_PTR(attrib.lightingStack[i].light);
        SLC_ASSSERT_NULL_PTR(attrib.transformStack[i].clip);
        SLC_ASSSERT_NULL_PTR(attrib.transformStack[i].clipPlane);

        for (j=0; j<GLEVAL_TOT; ++j)
        {
            SLC_ASSSERT_NULL_PTR(attrib.evalStack[i].eval1D[j].coeff);
            SLC_ASSSERT_NULL_PTR(attrib.evalStack[i].eval2D[j].coeff);
        }
    }

#ifdef CR_ARB_vertex_buffer_object
    SLC_COPYPTR(bufferobject.nullBuffer);
#endif

/*@todo, that should be removed probably as those should hold the offset values, so loading should be fine
  but better check*/
#if 0
#ifdef CR_EXT_compiled_vertex_array
    SLC_COPYPTR(client.array.v.prevPtr);
    SLC_COPYPTR(client.array.c.prevPtr);
    SLC_COPYPTR(client.array.f.prevPtr);
    SLC_COPYPTR(client.array.s.prevPtr);
    SLC_COPYPTR(client.array.e.prevPtr);
    SLC_COPYPTR(client.array.i.prevPtr);
    SLC_COPYPTR(client.array.n.prevPtr);
    for (i = 0 ; i < CR_MAX_TEXTURE_UNITS ; i++)
    {
        SLC_COPYPTR(client.array.t[i].prevPtr);
    }

# ifdef CR_NV_vertex_program
    for (i = 0; i < CR_MAX_VERTEX_ATTRIBS; i++)
    {
        SLC_COPYPTR(client.array.a[i].prevPtr);
    }
# endif
#endif
#endif

#ifdef CR_ARB_vertex_buffer_object
    /*That just sets those pointers to NULL*/
    SLC_COPYPTR(client.array.v.buffer);
    SLC_COPYPTR(client.array.c.buffer);
    SLC_COPYPTR(client.array.f.buffer);
    SLC_COPYPTR(client.array.s.buffer);
    SLC_COPYPTR(client.array.e.buffer);
    SLC_COPYPTR(client.array.i.buffer);
    SLC_COPYPTR(client.array.n.buffer);
    for (i = 0 ; i < CR_MAX_TEXTURE_UNITS ; i++)
    {
        SLC_COPYPTR(client.array.t[i].buffer);
    }
# ifdef CR_NV_vertex_program
    for (i = 0; i < CR_MAX_VERTEX_ATTRIBS; i++)
    {
        SLC_COPYPTR(client.array.a[i].buffer);
    }
# endif
#endif /*CR_ARB_vertex_buffer_object*/

    /** @todo CR_NV_vertex_program*/
    crStateCopyEvalPtrs1D(pTmpContext->eval.eval1D, pContext->eval.eval1D);
    crStateCopyEvalPtrs2D(pTmpContext->eval.eval2D, pContext->eval.eval2D);

    SLC_COPYPTR(feedback.buffer);  /** @todo */
    SLC_COPYPTR(selection.buffer); /** @todo */

    SLC_COPYPTR(lighting.light);

    /*This one could be tricky if we're loading snapshot on host with different GPU*/
    SLC_COPYPTR(limits.extensions);

#if CR_ARB_occlusion_query
    SLC_COPYPTR(occlusion.objects); /** @todo */
#endif

    SLC_COPYPTR(program.errorString);
    SLC_COPYPTR(program.programHash);
    SLC_COPYPTR(program.defaultVertexProgram);
    SLC_COPYPTR(program.defaultFragmentProgram);

    /* Texture pointers */
    for (i=0; i<6; ++i)
    {
        SLC_COPYPTR(texture.base1D.level[i]);
        SLC_COPYPTR(texture.base2D.level[i]);
        SLC_COPYPTR(texture.base3D.level[i]);
        SLC_COPYPTR(texture.proxy1D.level[i]);
        SLC_COPYPTR(texture.proxy2D.level[i]);
        SLC_COPYPTR(texture.proxy3D.level[i]);
#ifdef CR_ARB_texture_cube_map
        SLC_COPYPTR(texture.baseCubeMap.level[i]);
        SLC_COPYPTR(texture.proxyCubeMap.level[i]);
#endif
#ifdef CR_NV_texture_rectangle
        SLC_COPYPTR(texture.baseRect.level[i]);
        SLC_COPYPTR(texture.proxyRect.level[i]);
#endif
    }

    /* Load transform state */
    SLC_COPYPTR(transform.clipPlane);
    SLC_COPYPTR(transform.clip);
    /* Don't have to worry about pContext->transform.current as it'd be set in crStateSetCurrent call */
    /*SLC_COPYPTR(transform.currentStack);*/
    SLC_COPYPTR(transform.modelViewStack.stack);
    SLC_COPYPTR(transform.projectionStack.stack);
    SLC_COPYPTR(transform.colorStack.stack);

    for (i = 0 ; i < CR_MAX_TEXTURE_UNITS ; i++)
    {
        SLC_COPYPTR(transform.textureStack[i].stack);
    }
    for (i = 0 ; i < CR_MAX_PROGRAM_MATRICES ; i++)
    {
        SLC_COPYPTR(transform.programStack[i].stack);
    }

#ifdef CR_OPENGL_VERSION_2_0
    SLC_COPYPTR(glsl.shaders);
    SLC_COPYPTR(glsl.programs);
#endif

    /* Have to preserve original context id */
    CRASSERT(pTmpContext->id == pContext->id);
    CRASSERT(VBOXTLSREFDATA_EQUAL(pContext, pTmpContext));
    /* Copy ordinary state to real context */
    crMemcpy(pContext, pTmpContext, sizeof(*pTmpContext));
    crFree(pTmpContext);
    pTmpContext = NULL;

    /* Now deal with pointers */

    /* Load transform state */
    rc = SSMR3GetMem(pSSM, pContext->transform.clipPlane, sizeof(GLvectord)*CR_MAX_CLIP_PLANES);
    AssertRCReturn(rc, rc);
    rc = SSMR3GetMem(pSSM, pContext->transform.clip, sizeof(GLboolean)*CR_MAX_CLIP_PLANES);
    AssertRCReturn(rc, rc);
    rc = crStateLoadMatrixStack(&pContext->transform.modelViewStack, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadMatrixStack(&pContext->transform.projectionStack, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadMatrixStack(&pContext->transform.colorStack, pSSM);
    AssertRCReturn(rc, rc);
    for (i = 0 ; i < CR_MAX_TEXTURE_UNITS ; i++)
    {
        rc = crStateLoadMatrixStack(&pContext->transform.textureStack[i], pSSM);
        AssertRCReturn(rc, rc);
    }
    for (i = 0 ; i < CR_MAX_PROGRAM_MATRICES ; i++)
    {
        rc = crStateLoadMatrixStack(&pContext->transform.programStack[i], pSSM);
        AssertRCReturn(rc, rc);
    }

    /* Load Textures */
    rc = crStateLoadTextureObjData(&pContext->texture.base1D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadTextureObjData(&pContext->texture.base2D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadTextureObjData(&pContext->texture.base3D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadTextureObjData(&pContext->texture.proxy1D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadTextureObjData(&pContext->texture.proxy2D, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadTextureObjData(&pContext->texture.proxy3D, pSSM);
    AssertRCReturn(rc, rc);
#ifdef CR_ARB_texture_cube_map
    rc = crStateLoadTextureObjData(&pContext->texture.baseCubeMap, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadTextureObjData(&pContext->texture.proxyCubeMap, pSSM);
    AssertRCReturn(rc, rc);
#endif
#ifdef CR_NV_texture_rectangle
    rc = crStateLoadTextureObjData(&pContext->texture.baseRect, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadTextureObjData(&pContext->texture.proxyRect, pSSM);
    AssertRCReturn(rc, rc);
#endif

    if (bLoadShared)
    {
        /* Load shared textures */
        CRASSERT(pContext->shared && pContext->shared->textureTable);

        if (u32Version >= SHCROGL_SSM_VERSION_WITH_ALLOCATED_KEYS)
        {
            rc = crStateLoadKeys(pContext->shared->buffersTable, pSSM, u32Version);
            AssertRCReturn(rc, rc);
        }

        rc = SSMR3GetU32(pSSM, &uiNumElems);
        AssertRCReturn(rc, rc);
        for (ui=0; ui<uiNumElems; ++ui)
        {
            CRTextureObj *pTexture;

            rc = SSMR3GetMem(pSSM, &key, sizeof(key));
            AssertRCReturn(rc, rc);

            pTexture = (CRTextureObj *) crCalloc(sizeof(CRTextureObj));
            if (!pTexture) return VERR_NO_MEMORY;

            rc = crStateLoadTextureObj(pTexture, pSSM, u32Version);
            AssertRCReturn(rc, rc);

            pTexture->hwid = 0;

            /*allocate actual memory*/
            for (i=0; i<6; ++i) {
                pTexture->level[i] = (CRTextureLevel *) crCalloc(sizeof(CRTextureLevel) * CR_MAX_MIPMAP_LEVELS);
                if (!pTexture->level[i]) return VERR_NO_MEMORY;
            }

            rc = crStateLoadTextureObjData(pTexture, pSSM);
            AssertRCReturn(rc, rc);

            crHashtableAdd(pContext->shared->textureTable, key, pTexture);
        }
    }

    /* Load current texture pointers */
    for (i=0; i<CR_MAX_TEXTURE_UNITS; ++i)
    {
        rc = crStateLoadTexUnitCurrentTexturePtrs(&pContext->texture.unit[i], pContext, pSSM);
        AssertRCReturn(rc, rc);
    }

    /* Mark textures for resending to GPU */
    pContext->shared->bTexResyncNeeded = GL_TRUE;

    /* Load lights */
    CRASSERT(pContext->lighting.light);
    rc = SSMR3GetMem(pSSM, pContext->lighting.light, CR_MAX_LIGHTS * sizeof(*pContext->lighting.light));
    AssertRCReturn(rc, rc);

    /* Load attrib stack*/
    for ( i = 0 ; i < CR_MAX_ATTRIB_STACK_DEPTH ; i++)
    {
        if (pContext->attrib.enableStack[i].clip)
        {
            rc = crStateAllocAndSSMR3GetMem(pSSM, (void**)&pContext->attrib.enableStack[i].clip,
                                            pContext->limits.maxClipPlanes*sizeof(GLboolean));
            AssertRCReturn(rc, rc);
        }

        if (pContext->attrib.enableStack[i].light)
        {
            rc = crStateAllocAndSSMR3GetMem(pSSM, (void**)&pContext->attrib.enableStack[i].light,
                                            pContext->limits.maxLights*sizeof(GLboolean));
            AssertRCReturn(rc, rc);
        }

        if (pContext->attrib.lightingStack[i].light)
        {
            rc = crStateAllocAndSSMR3GetMem(pSSM, (void**)&pContext->attrib.lightingStack[i].light,
                                            pContext->limits.maxLights*sizeof(CRLight));
            AssertRCReturn(rc, rc);
        }

        for (k=0; k<pContext->limits.maxTextureUnits; ++k)
        {
            rc = crStateLoadTexUnitCurrentTexturePtrs(&pContext->attrib.textureStack[i].unit[k], pContext, pSSM);
            AssertRCReturn(rc, rc);
        }

        if (pContext->attrib.transformStack[i].clip)
        {
            rc = crStateAllocAndSSMR3GetMem(pSSM, (void*)&pContext->attrib.transformStack[i].clip,
                                            pContext->limits.maxClipPlanes*sizeof(GLboolean));
            AssertRCReturn(rc, rc);
        }

        if (pContext->attrib.transformStack[i].clipPlane)
        {
            rc = crStateAllocAndSSMR3GetMem(pSSM, (void**)&pContext->attrib.transformStack[i].clipPlane,
                                            pContext->limits.maxClipPlanes*sizeof(GLvectord));
            AssertRCReturn(rc, rc);
        }
        rc = crSateLoadEvalCoeffs1D(pContext->attrib.evalStack[i].eval1D, GL_TRUE, pSSM);
        AssertRCReturn(rc, rc);
        rc = crSateLoadEvalCoeffs2D(pContext->attrib.evalStack[i].eval2D, GL_TRUE, pSSM);
        AssertRCReturn(rc, rc);
    }

    /* Load evaluator coeffs */
    rc = crSateLoadEvalCoeffs1D(pContext->eval.eval1D, GL_FALSE, pSSM);
    AssertRCReturn(rc, rc);
    rc = crSateLoadEvalCoeffs2D(pContext->eval.eval2D, GL_FALSE, pSSM);
    AssertRCReturn(rc, rc);

    /* Load buffer objects */
#ifdef CR_ARB_vertex_buffer_object
    if (bLoadShared)
    {
        if (u32Version >= SHCROGL_SSM_VERSION_WITH_ALLOCATED_KEYS)
        {
            rc = crStateLoadKeys(pContext->shared->textureTable, pSSM, u32Version);
            AssertRCReturn(rc, rc);
        }
    }

    rc = SSMR3GetU32(pSSM, &uiNumElems);
    AssertRCReturn(rc, rc);
    for (ui=0; ui<=uiNumElems; ++ui) /*ui<=uiNumElems to load nullBuffer in same loop*/
    {
        CRBufferObject *pBufferObj;

        rc = SSMR3GetMem(pSSM, &key, sizeof(key));
        AssertRCReturn(rc, rc);

        /* default one should be already allocated */
        if (key==0)
        {
            pBufferObj = pContext->bufferobject.nullBuffer;
            if (!pBufferObj) return VERR_SSM_UNEXPECTED_DATA;
        }
        else
        {
            pBufferObj = (CRBufferObject *) crCalloc(sizeof(*pBufferObj));
            if (!pBufferObj) return VERR_NO_MEMORY;
        }

        rc = crStateLoadBufferObject(pBufferObj, pSSM, u32Version);
        AssertRCReturn(rc, rc);

        pBufferObj->hwid = 0;

        if (pBufferObj->data)
        {
            CRASSERT(pBufferObj->size>0);
            pBufferObj->data = crAlloc(pBufferObj->size);
            rc = SSMR3GetMem(pSSM, pBufferObj->data, pBufferObj->size);
            AssertRCReturn(rc, rc);
        }
        else if (pBufferObj->id!=0 && pBufferObj->size>0)
        {
            rc = SSMR3GetMem(pSSM, &pBufferObj->data, sizeof(pBufferObj->data));
            AssertRCReturn(rc, rc);

            if (pBufferObj->data)
            {
                pBufferObj->data = crAlloc(pBufferObj->size);
                rc = SSMR3GetMem(pSSM, pBufferObj->data, pBufferObj->size);
                AssertRCReturn(rc, rc);
            }
        }


        if (key!=0)
            crHashtableAdd(pContext->shared->buffersTable, key, pBufferObj);
    }
    /* Load pointers */
#define CRS_GET_BO(name) (((name)==0) ? (pContext->bufferobject.nullBuffer) : crHashtableSearch(pContext->shared->buffersTable, name))
    rc = SSMR3GetU32(pSSM, &ui);
    AssertRCReturn(rc, rc);
    pContext->bufferobject.arrayBuffer = CRS_GET_BO(ui);
    rc = SSMR3GetU32(pSSM, &ui);
    AssertRCReturn(rc, rc);
    pContext->bufferobject.elementsBuffer = CRS_GET_BO(ui);
#ifdef CR_ARB_pixel_buffer_object
    rc = SSMR3GetU32(pSSM, &ui);
    AssertRCReturn(rc, rc);
    pContext->bufferobject.packBuffer = CRS_GET_BO(ui);
    rc = SSMR3GetU32(pSSM, &ui);
    AssertRCReturn(rc, rc);
    pContext->bufferobject.unpackBuffer = CRS_GET_BO(ui);
#endif
#undef CRS_GET_BO

    /* Load client pointers and array buffer bindings*/
    for (i=0; i<CRSTATECLIENT_MAX_VERTEXARRAYS; ++i)
    {
        rc = crStateLoadClientPointer(&pContext->client.array, i, pContext, pSSM);
        AssertRCReturn(rc, rc);
    }
    for (j=0; j<pContext->client.vertexArrayStackDepth; ++j)
    {
        CRVertexArrays *pArray = &pContext->client.vertexArrayStack[j];
        for (i=0; i<CRSTATECLIENT_MAX_VERTEXARRAYS; ++i)
        {
            rc = crStateLoadClientPointer(pArray, i, pContext, pSSM);
            AssertRCReturn(rc, rc);
        }
    }

    pContext->shared->bVBOResyncNeeded = GL_TRUE;
#endif

    /* Load pixel/vertex programs */
    rc = SSMR3GetU32(pSSM, &uiNumElems);
    AssertRCReturn(rc, rc);
    /* Load defaults programs */
    rc = crStateLoadProgram(&pContext->program.defaultVertexProgram, pSSM);
    AssertRCReturn(rc, rc);
    rc = crStateLoadProgram(&pContext->program.defaultFragmentProgram, pSSM);
    AssertRCReturn(rc, rc);
    /* Load all the rest */
    for (ui=0; ui<uiNumElems; ++ui)
    {
        CRProgram *pProgram = NULL;
        rc = crStateLoadProgram(&pProgram, pSSM);
        AssertRCReturn(rc, rc);
        crHashtableAdd(pContext->program.programHash, pProgram->id, pProgram);
        //DIRTY(pProgram->dirtyProgram, pContext->neg_bitid);

    }
    /* Load Pointers */
    rc = SSMR3GetU32(pSSM, &ui);
    AssertRCReturn(rc, rc);
    pContext->program.currentVertexProgram = ui==0 ? pContext->program.defaultVertexProgram
                                                     : crHashtableSearch(pContext->program.programHash, ui);
    rc = SSMR3GetU32(pSSM, &ui);
    AssertRCReturn(rc, rc);
    pContext->program.currentFragmentProgram = ui==0 ? pContext->program.defaultFragmentProgram
                                                       : crHashtableSearch(pContext->program.programHash, ui);

    /* Mark programs for resending to GPU */
    pContext->program.bResyncNeeded = GL_TRUE;

#ifdef CR_EXT_framebuffer_object
    /* Load FBOs */
    if (bLoadShared)
    {
        if (u32Version >= SHCROGL_SSM_VERSION_WITH_ALLOCATED_KEYS)
        {
            rc = crStateLoadKeys(pContext->shared->fbTable, pSSM, u32Version);
            AssertRCReturn(rc, rc);
        }

        rc = SSMR3GetU32(pSSM, &uiNumElems);
        AssertRCReturn(rc, rc);
        for (ui=0; ui<uiNumElems; ++ui)
        {
            CRFramebufferObject *pFBO;
            pFBO = crAlloc(sizeof(*pFBO));
            if (!pFBO) return VERR_NO_MEMORY;

            rc = SSMR3GetMem(pSSM, &key, sizeof(key));
            AssertRCReturn(rc, rc);

            rc = crStateLoadFramebufferObject(pFBO, pSSM, u32Version);
            AssertRCReturn(rc, rc);

            Assert(key == pFBO->id);

            crHashtableAdd(pContext->shared->fbTable, key, pFBO);
        }

        if (u32Version >= SHCROGL_SSM_VERSION_WITH_ALLOCATED_KEYS)
        {
            rc = crStateLoadKeys(pContext->shared->rbTable, pSSM, u32Version);
            AssertRCReturn(rc, rc);
        }

        rc = SSMR3GetU32(pSSM, &uiNumElems);
        AssertRCReturn(rc, rc);
        for (ui=0; ui<uiNumElems; ++ui)
        {
            CRRenderbufferObject *pRBO;
            pRBO = crAlloc(sizeof(*pRBO));
            if (!pRBO) return VERR_NO_MEMORY;

            rc = SSMR3GetMem(pSSM, &key, sizeof(key));
            AssertRCReturn(rc, rc);

            rc = crStateLoadRenderbufferObject(pRBO, pSSM, u32Version);
            AssertRCReturn(rc, rc);

            crHashtableAdd(pContext->shared->rbTable, key, pRBO);
        }
    }

    rc = SSMR3GetU32(pSSM, &ui);
    AssertRCReturn(rc, rc);
    pContext->framebufferobject.drawFB = ui==0 ? NULL
                                               : crHashtableSearch(pContext->shared->fbTable, ui);

    rc = SSMR3GetU32(pSSM, &ui);
    AssertRCReturn(rc, rc);
    pContext->framebufferobject.readFB = ui==0 ? NULL
                                               : crHashtableSearch(pContext->shared->fbTable, ui);

    rc = SSMR3GetU32(pSSM, &ui);
    AssertRCReturn(rc, rc);
    pContext->framebufferobject.renderbuffer = ui==0 ? NULL
                                                     : crHashtableSearch(pContext->shared->rbTable, ui);

    /* Mark FBOs/RBOs for resending to GPU */
    pContext->shared->bFBOResyncNeeded = GL_TRUE;
#endif

#ifdef CR_OPENGL_VERSION_2_0
    /* Load GLSL related info */
    rc = SSMR3GetU32(pSSM, &uiNumElems);
    AssertRCReturn(rc, rc);

    for (ui=0; ui<uiNumElems; ++ui)
    {
        CRGLSLShader *pShader = crStateLoadGLSLShader(pSSM);
        GLboolean fNewKeyCheck;
        if (!pShader) return VERR_SSM_UNEXPECTED_DATA;
        fNewKeyCheck = crHashtableAllocRegisterKey(pContext->glsl.programs, pShader->id);
        CRASSERT(fNewKeyCheck);
        crHashtableAdd(pContext->glsl.shaders, pShader->id, pShader);
    }

    rc = SSMR3GetU32(pSSM, &uiNumElems);
    AssertRCReturn(rc, rc);

    for (ui=0; ui<uiNumElems; ++ui)
    {
        CRGLSLProgram *pProgram;
        uint32_t numShaders;

        pProgram = crAlloc(sizeof(*pProgram));
        if (!pProgram) return VERR_NO_MEMORY;

        rc = SSMR3GetMem(pSSM, &key, sizeof(key));
        AssertRCReturn(rc, rc);

        rc = SSMR3GetMem(pSSM, pProgram, sizeof(*pProgram));
        AssertRCReturn(rc, rc);

        crHashtableAdd(pContext->glsl.programs, key, pProgram);

        pProgram->currentState.attachedShaders = crAllocHashtable();

        rc = SSMR3GetU32(pSSM, &numShaders);
        AssertRCReturn(rc, rc);

        for (k=0; k<numShaders; ++k)
        {
            rc = SSMR3GetMem(pSSM, &key, sizeof(key));
            AssertRCReturn(rc, rc);
            crHashtableAdd(pProgram->currentState.attachedShaders, key, crHashtableSearch(pContext->glsl.shaders, key));
        }

        if (pProgram->activeState.attachedShaders)
        {
            pProgram->activeState.attachedShaders = crAllocHashtable();

            rc = SSMR3GetU32(pSSM, &numShaders);
            AssertRCReturn(rc, rc);

            for (k=0; k<numShaders; ++k)
            {
                CRGLSLShader *pShader = crStateLoadGLSLShader(pSSM);
                if (!pShader) return VERR_SSM_UNEXPECTED_DATA;
                crHashtableAdd(pProgram->activeState.attachedShaders, pShader->id, pShader);
            }
        }

        if (pProgram->currentState.cAttribs)
            pProgram->currentState.pAttribs = (CRGLSLAttrib*) crAlloc(pProgram->currentState.cAttribs*sizeof(CRGLSLAttrib));
        for (k=0; k<pProgram->currentState.cAttribs; ++k)
        {
            rc = SSMR3GetMem(pSSM, &pProgram->currentState.pAttribs[k].index, sizeof(pProgram->currentState.pAttribs[k].index));
            AssertRCReturn(rc, rc);
            pProgram->currentState.pAttribs[k].name = crStateLoadString(pSSM);
        }

        if (pProgram->activeState.cAttribs)
            pProgram->activeState.pAttribs = (CRGLSLAttrib*) crAlloc(pProgram->activeState.cAttribs*sizeof(CRGLSLAttrib));
        for (k=0; k<pProgram->activeState.cAttribs; ++k)
        {
            rc = SSMR3GetMem(pSSM, &pProgram->activeState.pAttribs[k].index, sizeof(pProgram->activeState.pAttribs[k].index));
            AssertRCReturn(rc, rc);
            pProgram->activeState.pAttribs[k].name = crStateLoadString(pSSM);
        }

        {
            int32_t cUniforms;
            rc = SSMR3GetS32(pSSM, &cUniforms);
            pProgram->cUniforms = cUniforms;
            AssertRCReturn(rc, rc);
        }

        if (pProgram->cUniforms)
        {
            pProgram->pUniforms = crAlloc(pProgram->cUniforms*sizeof(CRGLSLUniform));
            if (!pProgram->pUniforms) return VERR_NO_MEMORY;

            for (k=0; k<pProgram->cUniforms; ++k)
            {
                size_t itemsize, datasize;

                rc = SSMR3GetMem(pSSM, &pProgram->pUniforms[k].type, sizeof(GLenum));
                AssertRCReturn(rc, rc);
                pProgram->pUniforms[k].name = crStateLoadString(pSSM);

                if (crStateIsIntUniform(pProgram->pUniforms[k].type))
                {
                    itemsize = sizeof(GLint);
                } else itemsize = sizeof(GLfloat);

                datasize = crStateGetUniformSize(pProgram->pUniforms[k].type)*itemsize;
                pProgram->pUniforms[k].data = crAlloc((unsigned int /* this case is just so stupid */)datasize);
                if (!pProgram->pUniforms[k].data) return VERR_NO_MEMORY;

                rc = SSMR3GetMem(pSSM, pProgram->pUniforms[k].data, datasize);
                AssertRCReturn(rc, rc);
            }
        }
    }

    rc = SSMR3GetU32(pSSM, &ui);
    AssertRCReturn(rc, rc);
    pContext->glsl.activeProgram = ui==0 ? NULL
                                         : crHashtableSearch(pContext->glsl.programs, ui);

    /*Mark for resending to GPU*/
    pContext->glsl.bResyncNeeded = GL_TRUE;
#endif

    if (pContext->error != err)
    {
        crWarning("context error state changed on context restore, was 0x%x, but became 0x%x, resetting to its original value",
                err, pContext->error);
        pContext->error = err;
    }

    return VINF_SUCCESS;
}
